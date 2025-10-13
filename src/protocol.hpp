#pragma once
#include <cstdint>
#include <string>
#include <cstring>

enum class CommandType : uint8_t {
    GET = 0x01,
    SET = 0x02,
    DELETE = 0x03
};

constexpr uint32_t REQUEST_MAGIC = 0x4B565354;

struct RequestHeader {
    uint32_t magic;
    CommandType command;
    uint32_t key_length;
    uint32_t value_length;

    static RequestHeader parse(const uint8_t* buffer) {
        RequestHeader h;
        std::memcpy(&h.magic, buffer, sizeof(uint32_t));
        h.command = static_cast<CommandType>(buffer[4]);
        std::memcpy(&h.key_length, buffer + 8, sizeof(uint32_t));
        std::memcpy(&h.value_length, buffer + 12, sizeof(uint32_t));
        return h;
    }
};
