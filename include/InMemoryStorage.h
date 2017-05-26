/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <Storage.h>
#include <map>
#include <pthread.h>
#include <string>

class InMemoryStorage : public Storage {
public:
    /* un-shadow parent class methods */
    using Storage::get;
    using Storage::del;

    InMemoryStorage();
    ~InMemoryStorage();
    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key, bool* found);
    void del(const std::string& key, bool* found);

    InMemoryStorage(InMemoryStorage const&) = delete;
    void operator=(InMemoryStorage const&) = delete;

private:
    pthread_rwlock_t _rwlock;
    std::map<std::string, std::string> _kv;
};
