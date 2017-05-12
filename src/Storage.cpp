/* vim: set ai et ts=4 sw=4: */

#include <Storage.h>

std::string Storage::get(const std::string& key) {
    bool found; /* will be discarded */
    return get(key, &found);
}


void Storage::del(const std::string& key) {
    bool found; /* will be discarded */
    return del(key, &found);
}
