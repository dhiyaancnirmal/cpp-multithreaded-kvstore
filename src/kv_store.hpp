#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <array>
#include <string>

constexpr size_t NUM_SHARDS = 256;

class Shard {
public:
    bool get(const std::string& key, std::string& value) {
        std::shared_lock<std::shared_mutex> lock(mutex);
        auto it = data.find(key);
        if (it == data.end()) return false;
        value = it->second;
        return true;
    }

    void set(const std::string& key, const std::string& value) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        data[key] = value;
    }

private:
    std::shared_mutex mutex;
    std::unordered_map<std::string, std::string> data;
};

class ShardedHashMap {
public:
    bool get(const std::string& key, std::string& value) {
        return shards[shard_index(key)].get(key, value);
    }

    void set(const std::string& key, const std::string& value) {
        shards[shard_index(key)].set(key, value);
    }

private:
    size_t shard_index(const std::string& key) {
        return std::hash<std::string>{}(key) & (NUM_SHARDS - 1);
    }

    std::array<Shard, NUM_SHARDS> shards;
};
