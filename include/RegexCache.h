/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <pcre.h>

class RegexCache {
public:
    static const RegexCache& getInstance();

    RegexCache(RegexCache const&) = delete;
    void operator=(RegexCache const&) = delete;

    ~RegexCache();
    const pcre* getHttpRequestPattern() const;
    const pcre* getHttpHeaderPattern() const;

private:
    RegexCache(); /* keep the constructor private! */
    pcre* _httpRequestPattern;
    pcre* _httpHeaderPattern;
};
