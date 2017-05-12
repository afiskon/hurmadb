/* vim: set ai et ts=4 sw=4: */

#include <HttpRequest.h>
#include <string.h>
#include <stdexcept>
#include <map>
#include <utility>

HttpRequest::HttpRequest() {
    _httpMethod = HTTP_GET;
}

HttpRequest::~HttpRequest() {
    /* do nothing yet */
}

HttpMethod HttpRequest::getMethod() const {
    return _httpMethod;
}

void HttpRequest::setMethod(HttpMethod method) {
    _httpMethod = method;
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
    _headers.emplace(std::make_pair(
                std::string(name, name_len),
                std::string(val, val_len)
                ));
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

const std::map< std::string, std::string >& HttpRequest::getHeaders() const {
    return _headers;
}

void HttpRequest::setBody(const std::string& body) {
    _body = body; /* create a copy */
}

const std::string& HttpRequest::getBody() const {
    return _body;
}
