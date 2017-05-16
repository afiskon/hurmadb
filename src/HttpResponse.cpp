/* vim: set ai et ts=4 sw=4: */

#include <HttpResponse.h>

HttpStatus::HttpStatus() {
    _code = 0;
    _descr = "Undefined";
}

HttpStatus::HttpStatus(int code, const char* descr) {
    _code = code;
    _descr = descr;
}

int HttpStatus::getCode() const {
    return _code;
}

const char* HttpStatus::getDescr() const {
    return _descr;
}

/* 2xx */
const HttpStatus HTTP_STATUS_OK(200, "OK");

/* 3xx */

/* 4xx */
const HttpStatus HTTP_STATUS_BAD_REQUEST(400, "Bad Request");
const HttpStatus HTTP_STATUS_FORBIDDEN(403, "Forbidden");
const HttpStatus HTTP_STATUS_NOT_FOUND(404, "Not Found");

/* 5xx */
const HttpStatus HTTP_STATUS_INTERNAL_ERROR(500, "Internal Server Error");

HttpResponse::HttpResponse() {
    /* do nothing yet */
}

const HttpStatus& HttpResponse::getStatus() const {
    return _httpStatus;
}

void HttpResponse::setStatus(const HttpStatus& status) {
    _httpStatus = status;
}

bool HttpResponse::headerDefined(const std::string& name) {
    return _headers.find(name) != _headers.end();
}

void HttpResponse::emplaceHeader(std::string&& name, std::string&& value) {
    _headers.emplace(std::make_pair(name, value));
}

const std::map< std::string, std::string >& HttpResponse::getHeaders() {
    return _headers;
}

void HttpResponse::setBody(const std::string& body) {
    _body = body; /* create a copy */
}

const std::string& HttpResponse::getBody() const {
    return _body;
}
