/* vim: set ai et ts=4 sw=4: */

#include <stdexcept>
#include <pthread.h>
#include <defer.h>
#include <InMemoryStorage.h>

InMemoryStorage::InMemoryStorage() {
    if(pthread_rwlock_init(&_rwlock, NULL) != 0)
        throw std::runtime_error("InMemoryStorage::InMemoryStorage() - pthread_rwlock_init() failed");
}

InMemoryStorage::~InMemoryStorage() {
    pthread_rwlock_destroy(&_rwlock);
}

void InMemoryStorage::set(const std::string& key, const std::string& value) {
    pthread_rwlock_wrlock(&_rwlock);
    defer( pthread_rwlock_unlock(&_rwlock) );

    _kv[key] = value; /* insert or update */
}

std::string InMemoryStorage::get(const std::string& key, bool* found) {
    pthread_rwlock_rdlock(&_rwlock);
    defer( pthread_rwlock_unlock(&_rwlock) );

    auto it = _kv.find(key);
    if(it == _kv.end()) {
        *found = false;
        return "";
    } else {
        *found = true;
        return it->second;
    }
}

void InMemoryStorage::del(const std::string& key, bool* found) {
    pthread_rwlock_wrlock(&_rwlock);
    defer( pthread_rwlock_unlock(&_rwlock) );

    *found = (_kv.erase(key) > 0);
}

