/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <string>
#include <vector>
#include <deque>

using namespace std;

/**
 * @brief Ordered key-value storage interface.
 *
 * Various storage implementations (persistent, in-memory, compressed, encrypted storages, etc)
 * should implement this interface.
 *
 * Storage is not copyable.
 *
 */
class Storage {
public:
    virtual void set(const string& key, const string& value) = 0;
    virtual string get(const string& key, bool* found) = 0;
    virtual string get(const string& key);
    /** @deprecated use Storage::getRange instead */
    virtual string getRangeJson(const string& key_from, const string& key_to) = 0;
    virtual deque<vector<string>> getRange(const string& key_from, const string& key_to) = 0;
    virtual void del(const string& key, bool* found) = 0;
    virtual void del(const string& key);

    Storage(Storage const&) = delete;
    void operator=(Storage const&) = delete;
};
