/* vim: set ai et ts=4 sw=4: */

#pragma once

#include <string>

class Storage {
public:
    virtual void set(const std::string& key, const std::string& value, bool* append) = 0;
    virtual std::string get(const std::string& key, bool* found) = 0;
    virtual std::string get(const std::string& key);
    virtual std::string getRange(const std::string& key_from, const std::string& key_to) = 0;
    virtual void del(const std::string& key, bool* found) = 0;
    virtual void del(const std::string& key);
};
