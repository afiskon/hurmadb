/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <Socket.h>
#include <atomic>
#include <pcre.h>
#include <stdexcept>
#include <string>

class TcpServer {
public:
    TcpServer(void* (*WorkerThreadProc)(void* rawArg));
    ~TcpServer();
    void listen(const char* host, int port);
    void accept(const std::atomic_bool& terminate_flag);
    virtual void* createWTPArg(int accepted_socket, std::atomic_int* workersCounter) = 0;
    TcpServer(TcpServer const&) = delete;
    void operator=(TcpServer const&) = delete;

private:
    bool _listen_done;
    int _listen_socket;
    void _ignoreSigpipe();
    std::atomic_int _workersCounter;
    void* (*TcpWorkerThreadProc)(void*);
};
