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
 * TcpServer
 **********************************************************************
 */

TcpServer::TcpServer()
  : _listen_done(false)
  , _listen_socket(-1)
{}

TcpServer::~TcpServer() {
    if(_listen_done)
        close(_listen_socket);
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
        throw std::runtime_error("HttpServer::accept() - accept() call failed");

    newThread(accepted_socket);
}
