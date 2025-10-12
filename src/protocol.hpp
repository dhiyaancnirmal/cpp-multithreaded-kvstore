#pragma once
#include <cstdint>

enum class CommandType : uint8_t {
    GET = 0x01,
    SET = 0x02,
    DELETE = 0x03
};

struct RequestHeader {
    uint32_t magic;
    CommandType command;
    uint32_t key_length;
    uint32_t value_length;
};
