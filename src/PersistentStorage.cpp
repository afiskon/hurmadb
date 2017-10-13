/* vim: set ai et ts=4 sw=4: */

#include <PersistentStorage.h>
#include <stdexcept>
#include <string>
#include "rapidjson/document.h"

using namespace rapidjson;
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

void PersistentStorage::set(const std::string& key, const std::string& value, bool* append) {
    
    std::string json = "{ \"" + key + "\": " + value + " }";
    Document document;

    if(!document.Parse(json.c_str()).HasParseError()){
        Status s = _db->Put(WriteOptions(), key, value);
        *append = s.ok();
        if(!s.ok())
            throw std::runtime_error("PersistentStore::set() - _db->Put failed");
    }
    else
        *append = false;    
}

std::string PersistentStorage::get(const std::string& key, bool* found) {
    std::string value = "";
    Status s = _db->Get(ReadOptions(), key, &value);
    *found = s.ok();
    return value;
}

std::string PersistentStorage::getRange(const std::string& key_from, const std::string& key_to) {
    std::string result = "";
    std::string key = "";
    rocksdb::Iterator* it = _db->NewIterator(rocksdb::ReadOptions());
    bool isMoreOne = false;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        key = it->key().ToString();
        if((strcmp(key.c_str(), key_from.c_str()) >= 0) && (strcmp(key.c_str(), key_to.c_str()) <= 0))
        {
            if(isMoreOne) result += ",";

            result += "{ \"" + key + "\": " + it->value().ToString() + " }";
            isMoreOne = true;
        }
    } 

    return "[" + result + "]";
}

void PersistentStorage::del(const std::string& key, bool* found) {
    
    std::string value = "";
    Status check = _db->Get(ReadOptions(), key, &value);
    *found = check.ok();
    if(!check.ok())
        return;
    Status s = _db->Delete(WriteOptions(), key);
   
    *found = s.ok();
}
