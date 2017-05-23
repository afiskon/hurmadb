/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <pcre.h>
#include <stdexcept>
#include <string>
#include <atomic>
#include <Socket.h>
#include <HttpRequest.h>
#include <HttpResponse.h>

typedef void (*HttpRequestHandler)(const HttpRequest& req, HttpResponse& resp);

/* For internal usage only! */
struct HttpHandlerListItem {
    HttpHandlerListItem* next;
    HttpMethod method;
    pcre* regexp;
    HttpRequestHandler handler;
};

class HttpWorkerException : public std::exception {
public:
    explicit HttpWorkerException(const char* msg):
        _msg(msg)
    {
    }

    virtual const char* what() const throw () {
        return _msg.c_str();
    }

private:
    std::string _msg;
};

class HttpWorker {
    public:
        HttpWorker(Socket& socket, HttpHandlerListItem* handlersList);
        void run();

    private:
        Socket& _socket;
        HttpHandlerListItem* _handlersList;

        HttpRequestHandler _chooseHandler(HttpRequest& req);
        void _deserializeHttpRequest(Socket& socket, HttpRequest& req);
};

class HttpServer {
    public:
        HttpServer();
        ~HttpServer();
        void addHandler(HttpMethod method, const char* regexpStr, HttpRequestHandler handler);
        void listen(const char* host, int port);
        void accept();

        HttpServer(HttpServer const &) = delete;
        void operator=(HttpServer const &) = delete;

    private:
        bool _listen_done;
        int _listen_socket;
        std::atomic_int _workersCounter;
        HttpHandlerListItem* _handlers;

        void _ignoreSigpipe();
};
