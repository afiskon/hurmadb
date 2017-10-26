/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <Socket.h>
#include <atomic>
#include <pcre.h>
#include <stdexcept>
#include <string>

typedef void (*HttpRequestHandler)();

/* For internal usage only! */
struct TcpHandlerListItem {
    TcpHandlerListItem* next;
    pcre* regexp;
    HttpRequestHandler handler;
};

class TcpWorker {
public:
    TcpWorker(Socket& socket, TcpHandlerListItem* handlersList);
    void run();

private:
    Socket& _socket;
    TcpHandlerListItem* _handlersList;

    void _TCPRequest(Socket& socket);
};

class TcpServer {
public:
    TcpServer();
    ~TcpServer();
    void listen(const char* host, int port);
    void accept(const std::atomic_bool& terminate_flag);

    TcpServer(TcpServer const&) = delete;
    void operator=(TcpServer const&) = delete;

private:
    bool _listen_done;
    int _listen_socket;
    std::atomic_int _workersCounter;
    TcpHandlerListItem* _handlers;

    void _ignoreSigpipe();
};
