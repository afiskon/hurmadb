/* vim: set ai et ts=4 sw=4: */

#include <PersistentStorage.h>
#include <stdexcept>
#include <string>

using namespace rocksdb;

PersistentStorage::PersistentStorage() {
    _db = nullptr;

    Options options;

    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;

    Status s = rocksdb::DB::Open(options, "hurma_data", &_db);
    if(!s.ok())
        throw std::runtime_error("PersistentStorage::PersistentStorage() - DB::Open failed");
}

PersistentStorage::~PersistentStorage() {
    if(_db != nullptr)
        delete _db;
}

void PersistentStorage::set(const std::string& key, const std::string& value) {
    Status s = _db->Put(WriteOptions(), key, value);
    if(!s.ok())
        throw std::runtime_error("PersistentStore::set() - _db->Put failed");
}

std::string PersistentStorage::get(const std::string& key, bool* found) {
    std::string value = "";
    Status s = _db->Get(ReadOptions(), key, &value);
    *found = s.ok();
    return value;
}

void PersistentStorage::del(const std::string& key, bool* found) {
    Status s = _db->Delete(WriteOptions(), key);
    *found = s.ok(); // always returns true. TODO fix this + fix the test
}
