#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H

#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>

class RedisDatabase {
public:
    static RedisDatabase& getInstance();

    // Common Commands
    bool flushAll();

    // Key Value Operations
    void set(const std::string& key, const std::string& value);
    bool get(const std::string& key, std::string& value);
    std::vector<std::string> keys();
    std::string type(const std::string& key);
    bool del(const std::string& key);
    
    // Expire
    bool expire(const std::string& key, const std::string& seconds);
    
    // Rename 
    bool rename(const std::string& oldKey, const std::string& newKey);

    void purgeExpired();

    // Persistance : Dump/Load from the saved file
    bool dump(const std::string& filename);
    bool load(const std::string& filename);

private:
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    RedisDatabase(const RedisDatabase&) = delete;
    RedisDatabase& operator=(const RedisDatabase&) = delete;

    std::mutex RDB_mutex;
    std::unordered_map<std::string, std::string> Key_Value_Store;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> Expiry_Map;
};

#endif