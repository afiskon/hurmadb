/* vim: set ai et ts=4 sw=4: */

#include <TcpServer.h>
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

struct TcpWorkerThreadProcArg {
    int socket;
    TcpHandlerListItem* handlers;
    std::atomic_int* workersCounter;
};

static void* _tcpWorkerThreadProc(void* rawArg) {
    TcpWorkerThreadProcArg* arg = (TcpWorkerThreadProcArg*)rawArg;
    std::atomic_int& workersCounter = *arg->workersCounter;

    defer(workersCounter--); // already incremented in the parent process

    Socket socket(arg->socket);
   
    TcpWorker worker(socket, arg->handlers);

    delete arg;

    /* No other thread is going to join() this one */
    pthread_detach(pthread_self());

    worker.run();

    pthread_exit(nullptr);
}

TcpWorker::TcpWorker(Socket& socket, TcpHandlerListItem* handlersList)
  : _socket(socket)
  , _handlersList(handlersList) {
}


void TcpWorker::run() {
    
    bool close = true;

    do {

        try {
            ssize_t len;
            char buf[256];
            len = socket.readLine(buf, sizeof(buf));
            
            printf("%s\n", buf);
            printf("%zu\n", len);
            throw HttpWorkerException("HttpWorker::_TCPRequest() - no query match");

        }  catch(const std::runtime_error& e) {
            std::cerr << "TcpWorker::run(): " << e.what() << std::endl;
            break;
        }

        try {
            //handler
            close =false;
        } catch(const std::exception& e) {
            std::cerr << "TcpWorker::run() terminated with an exception: " << e.what() << std::endl;
        }
    } while(close);
}

/*
 **********************************************************************
 * TcpServer
 **********************************************************************
 */

TcpServer::TcpServer()
  : _listen_done(false)
  , _listen_socket(-1)
  , _workersCounter(0)
  , _handlers(nullptr) {
}

TcpServer::~TcpServer() {
    // TODO: add timeout here and kill workers using pthread_kill if necessary

    /* wait for running workers */
    while(_workersCounter.load() > 0) {
        usleep(10 * 1000); /* wait 10 ms */
    }

    if(_listen_done)
        close(_listen_socket);

    while(_handlers != nullptr) {
        TcpHandlerListItem* next = _handlers->next;
        pcre_free(_handlers->regexp);
        delete _handlers;
        _handlers = next;
    }
}

void TcpServer::_ignoreSigpipe() {
    sigset_t msk;
    sigemptyset(&msk);
    sigaddset(&msk, SIGPIPE);
    if(pthread_sigmask(SIG_BLOCK, &msk, nullptr) != 0)
        throw std::runtime_error("TcpServer::_ignoreSigpipe() - pthread_sigmask() call failed");
}

void TcpServer::listen(const char* host, int port) {
    struct sockaddr_in addr;

    _ignoreSigpipe();

    _listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(_listen_socket == -1)
        throw std::runtime_error("TCPServer::listen() - socket() call failed");

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);

    if(bind(_listen_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        throw std::runtime_error("TcpServer::listen() - bind() call failed");

    if(::listen(_listen_socket, 100) == -1)
        throw std::runtime_error("TcpServer::listen() - listen() call failed");

    _listen_done = true;
}

/* One iteration of accept loop */
void TcpServer::accept(const std::atomic_bool& terminate_flag) {
    for(;;) {
        struct timeval tv; /* we have to re-initialize these structures every time */
        tv.tv_sec = 1L;
        tv.tv_usec = 0;

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(_listen_socket, &rfds);

        int res = select(_listen_socket + 1, &rfds, (fd_set*)0, (fd_set*)0, &tv);
        if(res > 0) /* ready to accept() */
            break;

        if(res < 0) { /* error */
            if(errno == EINTR)
                continue;
            throw std::runtime_error("TcpServer::accept() - select() call failed: " + std::string(strerror(errno)));
        }

        /* timeout - check a terminate flag */
        if(terminate_flag.load())
            return;
    }

    int accepted_socket = ::accept(_listen_socket, 0, 0);
    if(accepted_socket == -1)
        throw std::runtime_error("TcpServer::accept() - accept() call failed");

    // disable TCP Nagle's algorithm
    int val = 1;
    if(setsockopt(accepted_socket, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) < 0) {
        close(accepted_socket);
        throw std::runtime_error("TcpServer::accept() - setsockopt(2) error");
    }

    auto arg = new(std::nothrow) TcpWorkerThreadProcArg();
    if(arg == nullptr) {
        close(accepted_socket);
        throw std::runtime_error("TcpServer::accept() - malloc() call failed");
    }

    arg->socket = accepted_socket;
    arg->handlers = _handlers;
    arg->workersCounter = &_workersCounter;

    /*
     * We need to increase workersCounter in the parent process to prevent race
     * condition. Otherwise it's possible that parent process will terminate
     * before child process increments a counter.
     */
    _workersCounter++;

    pthread_t thr;
    if(pthread_create(&thr, nullptr, _tcpWorkerThreadProc, (void*)arg) != 0) {
        _workersCounter--;
        close(accepted_socket);
        delete arg;
        throw std::runtime_error("TcpServer::accept() - pthread_create() call failed");
    }
}
