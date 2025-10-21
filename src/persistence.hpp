#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <cstring>
#include <chrono>
#include <arpa/inet.h>
#include "kv_store.hpp"

namespace kvstore {

constexpr uint32_t AOF_MAGIC = 0x414F4631;

enum class AOFCommand : uint8_t {
    SET = 0x01,
    DELETE = 0x02
};

class AOFWriter {
public:
    explicit AOFWriter(const std::string& filename)
        : filename_(filename) {
        file_.open(filename_, std::ios::binary | std::ios::app);
    }

    ~AOFWriter() {
        if (file_.is_open()) {
            file_.flush();
            file_.close();
        }
    }

    void log_set(const std::string& key, const std::string& value) {
        std::unique_lock<std::mutex> lock(write_mutex_);
        write_entry(AOFCommand::SET, key, value);
    }

    void log_delete(const std::string& key) {
        std::unique_lock<std::mutex> lock(write_mutex_);
        write_entry(AOFCommand::DELETE, key, "");
    }

    void flush() {
        std::unique_lock<std::mutex> lock(write_mutex_);
        file_.flush();
    }

private:
    void write_entry(AOFCommand cmd, const std::string& key, const std::string& value) {
        auto now = std::chrono::system_clock::now();
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();

        uint32_t magic_net = htonl(AOF_MAGIC);
        uint8_t command = static_cast<uint8_t>(cmd);
        uint8_t flags = 0;
        uint64_t timestamp = static_cast<uint64_t>(nanos);
        uint16_t key_len_net = htons(static_cast<uint16_t>(key.size()));
        uint32_t value_len_net = htonl(static_cast<uint32_t>(value.size()));

        file_.write(reinterpret_cast<const char*>(&magic_net), sizeof(uint32_t));
        file_.write(reinterpret_cast<const char*>(&command), sizeof(uint8_t));
        file_.write(reinterpret_cast<const char*>(&flags), sizeof(uint8_t));
        file_.write(reinterpret_cast<const char*>(&timestamp), sizeof(uint64_t));
        file_.write(reinterpret_cast<const char*>(&key_len_net), sizeof(uint16_t));
        file_.write(reinterpret_cast<const char*>(&value_len_net), sizeof(uint32_t));
        file_.write(key.data(), key.size());
        if (!value.empty()) {
            file_.write(value.data(), value.size());
        }

        file_.flush();
    }

    std::string filename_;
    std::ofstream file_;
    std::mutex write_mutex_;
};

class AOFReader {
public:
    explicit AOFReader(const std::string& filename)
        : filename_(filename) {}

    bool recover(ShardedHashMap& store) {
        std::ifstream file(filename_, std::ios::binary);
        if (!file.is_open()) {
            return true;
        }

        while (file.good() && !file.eof()) {
            uint32_t magic;
            file.read(reinterpret_cast<char*>(&magic), sizeof(uint32_t));
            if (file.eof()) break;

            magic = ntohl(magic);
            if (magic != AOF_MAGIC) {
                return false;
            }

            uint8_t command;
            uint8_t flags;
            uint64_t timestamp;
            uint16_t key_len;
            uint32_t value_len;

            file.read(reinterpret_cast<char*>(&command), sizeof(uint8_t));
            file.read(reinterpret_cast<char*>(&flags), sizeof(uint8_t));
            file.read(reinterpret_cast<char*>(&timestamp), sizeof(uint64_t));
            file.read(reinterpret_cast<char*>(&key_len), sizeof(uint16_t));
            file.read(reinterpret_cast<char*>(&value_len), sizeof(uint32_t));

            key_len = ntohs(key_len);
            value_len = ntohl(value_len);

            std::string key(key_len, '\0');
            file.read(&key[0], key_len);

            if (command == static_cast<uint8_t>(AOFCommand::SET)) {
                std::string value(value_len, '\0');
                file.read(&value[0], value_len);
                store.set(key, value);
            } else if (command == static_cast<uint8_t>(AOFCommand::DELETE)) {
                store.remove(key);
            }
        }

        return true;
    }

private:
    std::string filename_;
};

}
