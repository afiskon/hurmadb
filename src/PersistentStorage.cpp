/* vim: set ai et ts=4 sw=4: */

#include "defer.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include <PersistentStorage.h>
#include <rapidjson/writer.h>
#include <stdexcept>
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include <rapidjson/writer.h>

using namespace rapidjson;
using namespace rocksdb;

PersistentStorage::PersistentStorage() {
    _db = nullptr;

    Options options;

    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;

    Status s = DB::Open(options, "hurma_data", &_db);
    if(!s.ok())
        throw std::runtime_error("PersistentStorage::PersistentStorage() - DB::Open failed");
}

PersistentStorage::~PersistentStorage() {
    if(_db != nullptr)
        delete _db;
}

void PersistentStorage::set(const std::string& key, const std::string& value) {
    Document val;

    if(val.Parse(value.c_str()).HasParseError())
        throw std::runtime_error("PersistentStore::set() - validation failed");

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

// TODO: impelemt more efficient interation for wide ranges
std::string PersistentStorage::getRange(const std::string& key_from, const std::string& key_to) {
    Iterator* it = _db->NewIterator(ReadOptions());
    defer(delete it);

    Document result;
    result.SetObject();

    for(it->Seek(key_from); it->Valid() && it->key().ToString() <= key_to; it->Next()) {
        std::string key = it->key().ToString();
        std::string value = it->value().ToString();

        // Add "key": { ... value object ... } to the resulting document
        Value k(key.c_str(), result.GetAllocator());
        Document val_doc;
        val_doc.Parse(value.c_str());
        Value v(val_doc, result.GetAllocator());
        result.AddMember(k, v, result.GetAllocator());
    }

    // Check for any errors found during the scan
    if(!it->status().ok())
        throw std::runtime_error("PersistentStore::getRange() - error during the scan");

    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    result.Accept(writer);

    return sb.GetString();
}

void PersistentStorage::del(const std::string& key, bool* found) {

    std::string value = "";
    Status s = _db->Get(ReadOptions(), key, &value);
    *found = s.ok();
    if(!*found)
        return;
    s = _db->Delete(WriteOptions(), key);

    *found = s.ok();
}
