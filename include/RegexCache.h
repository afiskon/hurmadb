/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <pcre.h>

/**
 * @brief Singleton class that stores cached PCRE regular expressions.
 *
 * Currently the standard regex class is not stable and fast enough,
 * so we can't just write `const regex foo("...")`.
 */
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
