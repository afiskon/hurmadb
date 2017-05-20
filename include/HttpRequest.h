/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <string>
#include <vector>
#include <map>

enum HttpMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE
};

class HttpRequest {
    public:
        HttpRequest();
        ~HttpRequest();

        HttpMethod getMethod() const;
        void setMethod(HttpMethod method);

        const std::string& getVersion() const;
        void setVersion(const std::string& version); 

        const std::string& getQuery() const;
        void setQuery(const std::string& query);

        void addQueryMatch(const char* query, size_t len);
        size_t getQueryMatchesNumber() const;
        const std::string& getQueryMatch(size_t pos) const;

        // TODO: rewrite getHeader() using std::optional introduced in GCC 7.x
        void addHeader(const char* name, size_t name_len, const char* val, size_t val_len);
        const std::string& getHeader(const std::string& name, bool* found) const;
        const std::string& getHeader(const std::string& name) const;
        const std::map< std::string, std::string >& getHeaders() const;

        // TODO: use move semantics, perfect forwarding and stuff
        // see https://www.codeproject.com/Articles/397492/Move-Semantics-and-Perfect-Forwarding-in-Cplusplus
        void setBody(const std::string& body);
        const std::string& getBody() const;

        // Returns true if connection is persistent
        bool isPersistent() const;

        HttpRequest(HttpRequest const &) = delete;
        void operator=(HttpRequest const &) = delete;

    private:
        HttpMethod _httpMethod;
        std::string _version;
        std::string _query;
        std::map< std::string, std::string > _headers;
        std::string _body;

        /* Matched substrings of the query string. */
        std::vector<std::string> _matchVector;
};

