#include "../include/RedisDatabase.h"
#include <string>
#include <mutex>
#include <fstream>
#include <sstream>

/*
    MEMORY -> FILE - DUMP
    FILE -> MEMORY - LOAD
*/
RedisDatabase& RedisDatabase::getInstance(){
    static RedisDatabase instance;
    return instance;
}

// Common Comands
bool RedisDatabase::flushAll() {
    std::lock_guard<std::mutex> lock(RDB_mutex);
    Key_Value_Store.clear();
    return true;
}

// Key/Value Operations
void RedisDatabase::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(RDB_mutex);
    Key_Value_Store[key] = value;
}

bool RedisDatabase::get(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(RDB_mutex);
    purgeExpired();
    auto it = Key_Value_Store.find(key);
    if (it != Key_Value_Store.end()) {
        value = it->second;
        return true;
    }
    return false;
}

std::vector<std::string> RedisDatabase::keys() {
    std::lock_guard<std::mutex> lock(RDB_mutex);
    purgeExpired();
    std::vector<std::string> result;
    for (const auto& pair : Key_Value_Store) {
        result.push_back(pair.first);
    }
    return result;
}

std::string RedisDatabase::type(const std::string& key) {
    std::lock_guard<std::mutex> lock(RDB_mutex);
    purgeExpired();
    if (Key_Value_Store.find(key) != Key_Value_Store.end()) 
        return "string";
    else return "none";    
}

bool RedisDatabase::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(RDB_mutex);
    purgeExpired();
    bool erased = Key_Value_Store.erase(key) > 0;
    return erased;
}

bool RedisDatabase::expire(const std::string& key, const std::string& seconds) {
    std::lock_guard<std::mutex> lock(RDB_mutex);
    purgeExpired();
    bool exists = (Key_Value_Store.find(key) != Key_Value_Store.end());
    if (!exists)
        return false;
    
    Expiry_Map[key] = std::chrono::steady_clock::now() + std::chrono::seconds(std::stoi(seconds));
    return true;
}

void RedisDatabase::purgeExpired() {
    auto now = std::chrono::steady_clock::now();
    for (auto it = Expiry_Map.begin(); it != Expiry_Map.end(); ) {
        if (now > it->second) {
            // Remove from all stores
            Key_Value_Store.erase(it->first);
            it = Expiry_Map.erase(it);
        } else {
            ++it;
        }
    }
}

bool RedisDatabase::rename(const std::string& oldKey, const std::string& newKey) {
    std::lock_guard<std::mutex> lock(RDB_mutex);
    purgeExpired();
    bool found = false;

    auto itKv = Key_Value_Store.find(oldKey);
    if (itKv != Key_Value_Store.end()) {
        Key_Value_Store[newKey] = itKv->second;
        Key_Value_Store.erase(itKv);
        found = true;
    }

    auto itExpire = Expiry_Map.find(oldKey);
    if (itExpire != Expiry_Map.end()) {
        Expiry_Map[newKey] = itExpire->second;
        Expiry_Map.erase(itExpire);
    }

    return found;
}


bool RedisDatabase::dump(const std::string& filename){
    std::lock_guard<std::mutex>lock(RDB_mutex);
    std::ofstream ofs(filename, std::ios::binary);
    if(!ofs)    return false;

    for(const auto& Key_Value : Key_Value_Store){
        ofs << "[Key-Value] " << Key_Value.first << " " << Key_Value.second << "\n";
    }
    return true;
}

bool RedisDatabase::load(const std::string& filename){
    std::lock_guard<std::mutex>lock(RDB_mutex);
    std::ifstream ifs(filename, std::ios::binary);
    if(!ifs)    return false;

    Key_Value_Store.clear();

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.rfind("[Key-Value] ", 0) == 0) { // line starts with "[Key-Value] "
            std::string key_value_part = line.substr(12); // skip "[Key-Value] "
            std::istringstream kv_iss(key_value_part);
            std::string key, value;
            kv_iss >> key >> value;
            if (!key.empty())
                Key_Value_Store[key] = value;
        }
    }
    return true;
}