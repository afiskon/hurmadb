#include <HttpRequest.h>
#include <HttpResponse.h>

typedef void (*HttpRequestHandler)(const HttpRequest& req, HttpResponse& resp);