#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <array>
#include <optional>
#include <functional>

namespace kvstore {

constexpr size_t NUM_SHARDS = 256;

class Shard {
public:
    bool get(const std::string& key, std::string& value) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = data_.find(key);
        if (it == data_.end()) {
            return false;
        }
        value = it->second;
        return true;
    }

    void set(const std::string& key, const std::string& value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        data_[key] = value;
    }

    bool remove(const std::string& key) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        return data_.erase(key) > 0;
    }

    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return data_.size();
    }

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
};

class ShardedHashMap {
public:
    ShardedHashMap() = default;

    std::optional<std::string> get(const std::string& key) const {
        const auto& shard = get_shard(key);
        std::string value;
        if (shard.get(key, value)) {
            return value;
        }
        return std::nullopt;
    }

    void set(const std::string& key, const std::string& value) {
        auto& shard = get_shard(key);
        shard.set(key, value);
    }

    bool remove(const std::string& key) {
        auto& shard = get_shard(key);
        return shard.remove(key);
    }

    size_t total_size() const {
        size_t total = 0;
        for (const auto& shard : shards_) {
            total += shard.size();
        }
        return total;
    }

private:
    size_t get_shard_index(const std::string& key) const {
        return std::hash<std::string>{}(key) & (NUM_SHARDS - 1);
    }

    Shard& get_shard(const std::string& key) {
        return shards_[get_shard_index(key)];
    }

    const Shard& get_shard(const std::string& key) const {
        return shards_[get_shard_index(key)];
    }

    std::array<Shard, NUM_SHARDS> shards_;
};

}
