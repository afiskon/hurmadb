/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <Storage.h>
#include <rocksdb/db.h>
#include <string>
#include <vector>
#include <deque>

using namespace std;

class PersistentStorage : public Storage {
public:
    /* un-shadow parent class methods */
    using Storage::del;
    using Storage::get;

    PersistentStorage();
    ~PersistentStorage();
    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key, bool* found);
    std::string getRangeJson(const std::string& key_from, const std::string& key_to);
    deque<vector<string>> getRange(const std::string& key_from, const std::string& key_to);
    void del(const std::string& key, bool* found);

    PersistentStorage(PersistentStorage const&) = delete;
    void operator=(PersistentStorage const&) = delete;

private:
    rocksdb::DB* _db;
};
