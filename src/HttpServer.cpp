/* vim: set ai et ts=4 sw=4: */

#include <stdexcept>
#include <vector>
#include <tuple>
#include <utility>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <pcre.h>
#include <RegexCache.h>
#include <HttpServer.h>
#include <Socket.h>
#include <defer.h>

/*
 **********************************************************************
 * HttpWorker
 **********************************************************************
 */

#define MAX_BODY_SIZE 1024*1024 /* 1 Mb should probably be enough */

struct HttpWorkerThreadProcArg {
    int socket;
    HttpHandlerListItem* handlers;
};

static void _httpBadRequestHandler(const HttpRequest&, HttpResponse& resp) {
    resp.setStatus(HTTP_STATUS_BAD_REQUEST);
}

static void _httpNotFoundHandler(const HttpRequest&, HttpResponse& resp) {
    resp.setStatus(HTTP_STATUS_NOT_FOUND);
}

static void* _httpWorkerThreadProc(void* rawArg) {
    HttpWorkerThreadProcArg* arg = (HttpWorkerThreadProcArg*)rawArg;
    Socket socket(arg->socket);
    HttpWorker worker(socket, arg->handlers);
    delete arg;

    /* No other thread is going to join() this one */
    pthread_detach(pthread_self());
    worker.run();
    pthread_exit(nullptr);
}


HttpWorker::HttpWorker(Socket& socket, HttpHandlerListItem* handlersList):
    _socket(socket),
    _handlersList(handlersList) {
    }

void HttpWorker::_deserializeHttpRequest(Socket& socket, HttpRequest& req) {
    int mvector[32];
    char buf[256];

    /* Step 1 - process query */
    {
        const pcre* req_re = RegexCache::getInstance().getHttpRequestPattern();
        int len = socket.readLine(buf, sizeof(buf));
        int rc = pcre_exec(req_re, nullptr, buf, len, 0, 0, mvector, sizeof(mvector)/sizeof(mvector[0]));
        if(rc < 0) /* no match */
            throw HttpWorkerException();

        if(buf[0] == 'G')
            req.setMethod(HTTP_GET);
        else if(buf[0] == 'D')
            req.setMethod(HTTP_DELETE);
        else if(buf[1] == 'O')
            req.setMethod(HTTP_POST);
        else
            req.setMethod(HTTP_PUT);

        int qstart = mvector[4];
        int qend = mvector[5];

        buf[qend] = '\0';

        req.setQuery(&buf[qstart]);
    }

    /* Step 2 - process headers */
    {
        const pcre* header_re = RegexCache::getInstance().getHttpHeaderPattern();
        for(;;) {
            size_t len = socket.readLine(buf, sizeof(buf));
            if(len == 0) break;

            int rc = pcre_exec(header_re, nullptr, buf, len, 0, 0, mvector, sizeof(mvector)/sizeof(mvector[0]));
            if(rc < 0) continue; /* no match - ignore garbage in headers */

            int name_start = mvector[2];
            int name_end = mvector[3];
            int val_start = mvector[4];
            int val_end = mvector[5];
            req.addHeader(
                buf + name_start, name_end - name_start,
                buf + val_start, val_end - val_start
            );
        }
    }

    /* Step 3 - read the body */
    {
        bool found;
        const std::string contentLengthStr = req.getHeader("Content-Length", &found);
        if(found) {
            ssize_t contentLength = std::stoi(contentLengthStr);
            if(contentLength < 0 || contentLength > MAX_BODY_SIZE)
                throw HttpWorkerException(); /* wrong query */

            char* tmp_buff = new char[ contentLength + 1 ];
            defer( delete[] tmp_buff );

            socket.read(tmp_buff, contentLength);
            tmp_buff[contentLength] = '\0';
            req.setBody(tmp_buff);
        }
    }
}

void HttpWorker::_serializeHttpResponse(Socket& socket, /* const */ HttpResponse& resp) {
    const HttpStatus& httpStatus = resp.getStatus();
    char codeAndDescr[128];

    snprintf(codeAndDescr, sizeof(codeAndDescr), "HTTP/1.1 %3u %s\r\n", httpStatus.getCode(), httpStatus.getDescr());
    socket.write(codeAndDescr, strlen(codeAndDescr));

    if(!resp.headerDefined("Connection"))
        resp.emplaceHeader("Connection", "close"); /* here is why HttpResponse is not `const` */

    if(!resp.headerDefined("Content-Length"))
        resp.emplaceHeader("Content-Length", std::to_string(resp.getBody().size()));

    for(auto const &entry: resp.getHeaders()) {
        static const std::string headerSplitter(": ");
        socket.write(entry.first);
        socket.write(headerSplitter);
        socket.write(entry.second);
        socket.writeEOL();
    }

    socket.writeEOL();
    socket.write(resp.getBody());
}

HttpRequestHandler HttpWorker::_chooseHandler(HttpRequest& req) {
    const char* query = req.getQuery().c_str();
    int query_len = strlen(query);
    HttpHandlerListItem* listItem = _handlersList;

    while(listItem) {
        if(listItem->method == req.getMethod()) {
            int mvector[32];
            const size_t mvectorItems = sizeof(mvector) / sizeof(mvector[0]);
            int rc = pcre_exec(listItem->regexp, nullptr, query, query_len, 0, 0, mvector, mvectorItems);
            if(rc >= 0) { /* match! */
                /*
                 * Copy matched substrings of the query.
                 *
                 * See "How pcre_exec() returns captured substrings" section
                 * of `man pcreapi`.
                 */
                for(int i = 1; i < rc; i++)
                    req.addQueryMatch(query + mvector[i*2], mvector[i*2 + 1] - mvector[i*2]);

                return listItem->handler;
            }
        }
        listItem = listItem->next;
    }

    return _httpNotFoundHandler;
}

// TODO: support Connection: keep-alive
void HttpWorker::run() {
    HttpRequest req;
    HttpResponse resp;
    HttpRequestHandler handler = _httpBadRequestHandler;

    try {
        _deserializeHttpRequest(_socket, req);
        handler = _chooseHandler(req);
    } catch (const HttpWorkerException& e) {
        handler = _httpBadRequestHandler;
    }

    try {
        handler(req, resp);
        _serializeHttpResponse(_socket, resp);
    } catch (const std::exception& e) {
        // TODO: report internal server error
        std::cerr << "HttpWorker::run() terminated with an exception: " << e.what() << std::endl;
    }
}

/*
 **********************************************************************
 * HttpServer
 **********************************************************************
 */

HttpServer::HttpServer() {
    _handlers = nullptr;
    _listen_done = false;
    _listen_socket = -1;
}

HttpServer::~HttpServer() {
    if(_listen_done)
        close(_listen_socket);

    while(_handlers != nullptr) {
        HttpHandlerListItem* next = _handlers->next;
        pcre_free(_handlers->regexp);
        delete _handlers;
        _handlers = next;
    }
}

void HttpServer::addHandler(HttpMethod method, const char* regexpStr, HttpRequestHandler handler) {
    const char* error;
    int erroroffset;
    pcre* re = pcre_compile(regexpStr, 0, &error, &erroroffset, nullptr);
    if(re == nullptr)
        throw std::runtime_error("HttpServer::addHandler() - PCRE compilation failed");

    HttpHandlerListItem* item = new(std::nothrow) HttpHandlerListItem();
    if(item == nullptr) {
        pcre_free(re);
        throw std::runtime_error("HttpServer::addHandler() - out of memory");
    }

    item->method = method;
    item->regexp = re;
    item->handler = handler;
    item->next = _handlers;
    _handlers = item;
}

/*
 * If client will close a connection recv() will just return -1
 * instead of killing a process with SIGPIPE.
 */
void HttpServer::_ignoreSigpipe() {
    sigset_t msk;
    sigemptyset (&msk);
    sigaddset(&msk, SIGPIPE);
    if(pthread_sigmask(SIG_BLOCK, &msk, nullptr) != 0)
        throw std::runtime_error("HttpServer::_ignoreSigpipe() - pthread_sigmask() call failed");
}


void HttpServer::listen(const char* host, int port) {
    struct sockaddr_in addr;

    _ignoreSigpipe();

    _listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(_listen_socket == -1)
        throw std::runtime_error("HttpServer::listen() - socket() call failed");

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);
    if(bind(_listen_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        throw std::runtime_error("HttpServer::listen() - bind() call failed");

    if(::listen(_listen_socket, 100) == -1)
        throw std::runtime_error("HttpServer::listen() - listen() call failed");

    _listen_done = true;
}

/* One iteration of accept loop */
void HttpServer::accept() {
    int accepted_socket = ::accept(_listen_socket, 0, 0);
    if(accepted_socket == -1)
        throw std::runtime_error("HttpServer::accept() - accept() call failed");

    auto arg = new(std::nothrow) HttpWorkerThreadProcArg();
    if(arg == nullptr) {
        close(accepted_socket);
        throw std::runtime_error("HttpServer::accept() - malloc() call failed");
    }

    arg->socket = accepted_socket;
    arg->handlers = _handlers;
    pthread_t thr;
    if(pthread_create(&thr, nullptr, _httpWorkerThreadProc, (void*)arg) != 0)
        throw std::runtime_error("HttpServer::accept() - pthread_create() call failed");
}
