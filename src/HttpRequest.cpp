/* vim: set ai et ts=4 sw=4: */

#include <HttpRequest.h>
#include <map>
#include <stdexcept>
#include <string.h>
#include <utility>

HttpRequest::HttpRequest() {
    _httpMethod = HTTP_GET;
}

HttpRequest::~HttpRequest() {
}

HttpMethod HttpRequest::getMethod() const {
    return _httpMethod;
}

void HttpRequest::setMethod(HttpMethod method) {
    _httpMethod = method;
}

const std::string& HttpRequest::getVersion() const {
    return _version;
}

void HttpRequest::setVersion(const std::string& version) {
    _version = version;
}

const std::string& HttpRequest::getQuery() const {
    return _query;
}

void HttpRequest::setQuery(const std::string& query) {
    _query = query;
}

void HttpRequest::addQueryMatch(const char* query, size_t len) {
    _matchVector.emplace_back(std::string(query, len));
}

size_t HttpRequest::getQueryMatchesNumber() const {
    return _matchVector.size();
}

const std::string& HttpRequest::getQueryMatch(size_t pos) const {
    return _matchVector[pos];
}

void HttpRequest::addHeader(const char* name, size_t name_len, const char* val, size_t val_len) {
    _headers.emplace(std::make_pair(std::string(name, name_len), std::string(val, val_len)));
}

// TODO: rewrite to std::optional introduced in GCC 7.x
const std::string& HttpRequest::getHeader(const std::string& name, bool* found) const {
    static std::string empty("");

    auto it = _headers.find(name);
    if(it == _headers.end()) {
        *found = false;
        return empty;
    } else {
        *found = true;
        return it->second;
    }
}

const std::string& HttpRequest::getHeader(const std::string& name) const {
    bool found; /* will be discarded */
    return getHeader(name, &found);
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
    return _headers;
}

void HttpRequest::setBody(const std::string& body) {
    _body = body; /* create a copy */
}

const std::string& HttpRequest::getBody() const {
    return _body;
}

// According https://tools.ietf.org/html/rfc7230#page-52
// the connection is persistent if:
// 1. 'Connection: close' header is not present
// 2. If version is HTTP/1.1 or higher
// 3. If version is HTTP/1.0 and 'Connection: keep-alive' is present
bool HttpRequest::isPersistent() const {
    bool result;
    if(getHeader("Connection") == "close") {
        result = false;
    } else if(getVersion() == "HTTP/1.1") {
        result = true;
    } else if(getVersion() == "HTTP/1.0" && getHeader("Connection") == "keep-alive") {
        result = true;
    } else {
        result = false;
    }
    return result;
}
