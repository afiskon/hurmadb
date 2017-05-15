/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <pcre.h>
#include <stdexcept>
#include <string>
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

class HttpWorkerException { };

class HttpWorker {
    public:
        HttpWorker(Socket& socket, HttpHandlerListItem* handlersList);
        void run();

    private:
        Socket& _socket;
        HttpHandlerListItem* _handlersList;

        HttpRequestHandler _chooseHandler(HttpRequest& req);
        void _deserializeHttpRequest(Socket& socket, HttpRequest& req);
        void _serializeHttpResponse(Socket& socket, /* const */ HttpResponse& resp);
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
        HttpHandlerListItem* _handlers;

        void _ignoreSigpipe();
};