/* vim: set ai et ts=4 sw=4: */

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include <PersistentStorage.h>
#include <rapidjson/writer.h>
#include <stdexcept>
#include <string>

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

    Document val;

    if(!val.Parse(value.c_str()).HasParseError()) {

        StringBuffer sb;
        Writer<StringBuffer> writer(sb);

        val.Accept(writer);

        Status s = _db->Put(WriteOptions(), key, sb.GetString());
        *append = s.ok();
        if(!s.ok())
            throw std::runtime_error("PersistentStore::set() - _db->Put failed");
    } else
        *append = false;
}

std::string PersistentStorage::get(const std::string& key, bool* found) {
    std::string value = "";
    Status s = _db->Get(ReadOptions(), key, &value);
    *found = s.ok();
    return value;
}

// TODO: impelemt more efficient interation for wide ranges
std::string PersistentStorage::getRange(const std::string& key_from, const std::string& key_to) {
    std::string key = "";
    rocksdb::Iterator* it = _db->NewIterator(rocksdb::ReadOptions());

    Document result;
    result.SetObject();

    for(it->Seek(key_from); it->Valid() && it->key().ToString() <= key_to; it->Next()) {
        key = it->key().ToString();
        Value k(key.c_str(), result.GetAllocator());
        Document v;
        v.Parse(it->value().ToString().c_str());
        Value val(v, result.GetAllocator());
        result.AddMember(k, val, result.GetAllocator());
    }

    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    result.Accept(writer);

    return sb.GetString();
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
