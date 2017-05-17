/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <string>
#include <rocksdb/db.h>
#include <Storage.h>

class PersistentStorage: public Storage {
    public:
        /* un-shadow parent class methods */
        using Storage::get;
        using Storage::del;

        PersistentStorage();
        ~PersistentStorage();
        void set(const std::string& key, const std::string& value);
        std::string get(const std::string& key, bool* found);
        void del(const std::string& key, bool* found);

        PersistentStorage(PersistentStorage const &) = delete;
        void operator=(PersistentStorage const &) = delete;
    private:
        rocksdb::DB* _db;
};
