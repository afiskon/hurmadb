/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <Socket.h>
#include <atomic>
#include <pcre.h>
#include <stdexcept>
#include <string>


/* For internal usage only! */


/*
class TcpWorker {
public:
    TcpWorker(Socket& socket, TcpHandlerListItem* handlersList);
    void run();

private:
    Socket& _socket;
    TcpHandlerListItem* _handlersList;

    void _TCPRequest(Socket& socket);
};
*/
class TcpServer {
public:
    TcpServer();
    ~TcpServer();
    void listen(const char* host, int port);
    void accept(const std::atomic_bool& terminate_flag);
    virtual void newThread(int accepted_socket){ throw std::runtime_error("HttpServer::accept() - malloc() call failed"+accepted_socket); };

    TcpServer(TcpServer const&) = delete;
    void operator=(TcpServer const&) = delete;

private:
    bool _listen_done;
    int _listen_socket;
    void _ignoreSigpipe();
};
