/* vim: set ai et ts=4 sw=4: */

#include <TcpServer.h>
#include <HttpServer.h>
#include <RegexCache.h>
#include <Socket.h>
#include <arpa/inet.h>
#include <atomic>
#include <defer.h>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pcre.h>
#include <pthread.h>
#include <signal.h>
#include <stdexcept>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <tuple>
#include <unistd.h>
#include <utility>
#include <vector>

/*
 **********************************************************************
 * HttpWorker
 **********************************************************************
 */

#define MAX_BODY_SIZE 1024 * 1024 /* 1 Mb should probably be enough */

struct HttpWorkerThreadProcArg {
    int socket;
    HttpHandlerListItem* handlers;
    std::atomic_int* workersCounter;
};

static void _httpBadRequestHandler(const HttpRequest&, HttpResponse& resp) {
    resp.setStatus(HTTP_STATUS_BAD_REQUEST);
}

static void _httpNotFoundHandler(const HttpRequest&, HttpResponse& resp) {
    resp.setStatus(HTTP_STATUS_NOT_FOUND);
}

static void* _httpWorkerThreadProc(void* rawArg) {
    HttpWorkerThreadProcArg* arg = (HttpWorkerThreadProcArg*)rawArg;
    std::atomic_int& workersCounter = *arg->workersCounter;

    defer(workersCounter--); // already incremented in the parent process

    Socket socket(arg->socket);
    HttpWorker worker(socket, arg->handlers);
    delete arg;

    /* No other thread is going to join() this one */
    pthread_detach(pthread_self());

    worker.run();

    pthread_exit(nullptr);
}

HttpWorker::HttpWorker(Socket& socket, HttpHandlerListItem* handlersList)
  : _socket(socket)
  , _handlersList(handlersList) {
}

// TODO: refactor, make it HttpRequest method simialar to serialize is a method
// of HttpResponse
void HttpWorker::_deserializeHttpRequest(Socket& socket, HttpRequest& req) {
    int mvector[32];
    char buf[256];

    /* Step 1 - process query */
    {
        const pcre* req_re = RegexCache::getInstance().getHttpRequestPattern();
        int len = socket.readLine(buf, sizeof(buf));

        int rc = pcre_exec(req_re, nullptr, buf, len, 0, 0, mvector, sizeof(mvector) / sizeof(mvector[0]));
        if(rc < 0) /* no match */
            throw HttpWorkerException("HttpWorker::_deserializeHttpRequest() - no query match");

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

        qstart = mvector[6];
        qend = mvector[7];
        buf[qend] = '\0';
        req.setVersion(&buf[qstart]);
    }

    /* Step 2 - process headers */
    {
        const pcre* header_re = RegexCache::getInstance().getHttpHeaderPattern();
        for(;;) {
            size_t len = socket.readLine(buf, sizeof(buf));
            if(len == 0) { // end of headers
                break;
            }

            int rc = pcre_exec(header_re, nullptr, buf, len, 0, 0, mvector, sizeof(mvector) / sizeof(mvector[0]));
            if(rc < 0)
                continue; /* no match - ignore garbage in headers */

            int name_start = mvector[2];
            int name_end = mvector[3];
            int val_start = mvector[4];
            int val_end = mvector[5];
            req.addHeader(buf + name_start, name_end - name_start, buf + val_start, val_end - val_start);
        }
    }

    /* Step 3 - read the body */
    {
        bool found;
        const std::string contentLengthStr = req.getHeader("Content-Length", &found);
        if(found) {
            ssize_t contentLength = std::stoi(contentLengthStr);
            if(contentLength < 0 || contentLength > MAX_BODY_SIZE)
                throw HttpWorkerException("HttpWorker::_deserializeHttpRequest() - wrong content length");

            char* tmp_buff = new char[contentLength + 1];
            defer(delete[] tmp_buff);

            socket.read(tmp_buff, contentLength);
            tmp_buff[contentLength] = '\0';
            req.setBody(tmp_buff);
        }
    }
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
                    req.addQueryMatch(query + mvector[i * 2], mvector[i * 2 + 1] - mvector[i * 2]);

                return listItem->handler;
            }
        }
        listItem = listItem->next;
    }
    std::cerr << "_chooseHandler(): handler not found for query " << req.getQuery() << std::endl;
    return _httpNotFoundHandler;
}

void HttpWorker::run() {
    bool isPersistent;
    do {
        HttpRequest req;
        HttpResponse resp;
        HttpRequestHandler handler = _httpBadRequestHandler;

        try {
            _deserializeHttpRequest(_socket, req);
            handler = _chooseHandler(req);
        } catch(const HttpWorkerException& e) {
            // TODO: don't report "Socket::read() - client closed connection"
            std::cerr << "HttpWorker::run(): " << e.what() << std::endl;
            handler = _httpBadRequestHandler;
        } catch(const std::runtime_error& e) {
            std::cerr << "HttpWorker::run(): " << e.what() << std::endl;
            break;
        }
        isPersistent = req.isPersistent();

        try {
            handler(req, resp);

            if(!isPersistent)
                resp.emplaceHeader("Connection", "close");

            _socket.write(resp.serialize());
        } catch(const std::exception& e) {
            // TODO: report internal server error
            std::cerr << "HttpWorker::run() terminated with an exception: " << e.what() << std::endl;
        }
    } while(isPersistent);
}

/*
 **********************************************************************
 * HttpServer
 **********************************************************************
 */

HttpServer::HttpServer(): TcpServer(&_httpWorkerThreadProc)  
  , _handlers(nullptr) {

}


HttpServer::~HttpServer() {
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

void* HttpServer::createWorkerThreadProcArg(int accepted_socket, std::atomic_int* workersCounter){
    auto arg = new(std::nothrow) HttpWorkerThreadProcArg();
    if(arg == nullptr) {
        close(accepted_socket);
        throw std::runtime_error("HttpServer::createWTPArg() - malloc() call failed");
    }
    arg->socket = accepted_socket;
    arg->handlers = _handlers;
    arg->workersCounter = workersCounter;

    return arg;
}
