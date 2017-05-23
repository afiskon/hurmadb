#include <stdexcept>
#include <RegexCache.h>
#include <pcre.h>

const RegexCache& RegexCache::getInstance() {
    static RegexCache cache;
    return cache;
}

RegexCache::RegexCache() {
    int erroroffset;
    const char* error;
    static const char req_pattern[] = "^(GET|POST|PUT|DELETE) ([^ ?]+)[^ ]* (HTTP/1.[01])$";
    static const char header_pattern[] = "^([a-zA-Z0-9_-]+):[ ]+(.+)$";

    _httpRequestPattern = nullptr;
    _httpHeaderPattern = nullptr;

    _httpRequestPattern = pcre_compile(req_pattern, 0, &error, &erroroffset, nullptr);
    if(_httpRequestPattern == nullptr)
        throw std::runtime_error("RegexCache::RegexCache() - failed to compile _httpRequestPattern");

    _httpHeaderPattern = pcre_compile(header_pattern, 0, &error, &erroroffset, nullptr);
    if(_httpHeaderPattern == nullptr)
        throw std::runtime_error("RegexCache::RegexCache() - failed to compile _httpHeaderPattern");
}

RegexCache::~RegexCache() {
    if(_httpRequestPattern != nullptr)
        pcre_free(_httpRequestPattern);

    if(_httpHeaderPattern != nullptr)
        pcre_free(_httpHeaderPattern);
}

const pcre* RegexCache::getHttpRequestPattern() const {
    return _httpRequestPattern;
}

const pcre* RegexCache::getHttpHeaderPattern() const {
    return _httpHeaderPattern;
}
