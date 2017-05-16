/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <string>
#include <map>

class HttpStatus {
    public:
        HttpStatus();
        HttpStatus(int code, const char* descr);
        int getCode() const;
        const char* getDescr() const;
        const char* getCodeAndDescr() const;

        HttpStatus(HttpStatus const &) = delete;

        /* Allow to copy */
        /* void operator=(HttpStatus const &) = delete; */

    private:
        int _code;
        const char* _descr;
};

/* 2xx */
extern const HttpStatus HTTP_STATUS_OK;
/* 3xx */

/* 4xx */
extern const HttpStatus HTTP_STATUS_BAD_REQUEST;
extern const HttpStatus HTTP_STATUS_FORBIDDEN;
extern const HttpStatus HTTP_STATUS_NOT_FOUND;

/* 5xx */
extern const HttpStatus HTTP_STATUS_INTERNAL_ERROR;

class HttpResponse {
    public:
        HttpResponse();
        const HttpStatus& getStatus() const;
        void setStatus(const HttpStatus& status);

        void setBody(const std::string& body);
        const std::string& getBody() const;

        const std::map< std::string, std::string >& getHeaders();
        void emplaceHeader(std::string&& name, std::string&& value);
        bool headerDefined(const std::string& name);

        HttpResponse(HttpResponse const &) = delete;
        void operator=(HttpResponse const &) = delete;

    private:
        HttpStatus _httpStatus;
        std::map< std::string, std::string > _headers;
        std::string _body;
};


