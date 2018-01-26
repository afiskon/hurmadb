/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <HttpRequestHandler.h>
#include <Socket.h>
#include <TcpServer.h>
#include <atomic>
#include <pcre.h>
#include <stdexcept>
#include <string>

class TcpServer {
public:
    TcpServer(void* (*workerThreadProc)(void*));
    ~TcpServer();
    void listen(const char* host, int port);
    void accept(const std::atomic_bool& terminate_flag);
    TcpServer(TcpServer const&) = delete;
    void operator=(TcpServer const&) = delete;

protected:
    virtual void* createWorkerThreadProcArg(int accepted_socket, std::atomic_int* workersCounter) = 0;

private:
    bool _listen_done;
    int _listen_socket;
    void _ignoreSigpipe();
    std::atomic_int _workersCounter;
    void* (*_tcpWorkerThreadProc)(void*);
};
