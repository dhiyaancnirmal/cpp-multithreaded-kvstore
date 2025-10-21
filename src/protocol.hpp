#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>
#include <arpa/inet.h>

namespace kvstore {

enum class CommandType : uint8_t {
    GET = 0x01,
    SET = 0x02,
    DELETE = 0x03,
    PING = 0x04
};

enum class StatusCode : uint8_t {
    OK = 0x00,
    KEY_NOT_FOUND = 0x01,
    INVALID_COMMAND = 0x02,
    PROTOCOL_ERROR = 0x03,
    INTERNAL_ERROR = 0x04
};

constexpr uint32_t REQUEST_MAGIC = 0x4B565354;
constexpr uint32_t RESPONSE_MAGIC = 0x4B565352;
constexpr size_t HEADER_SIZE = 16;

struct RequestHeader {
    uint32_t magic;
    CommandType command;
    uint8_t flags;
    uint32_t key_length;
    uint32_t value_length;
    uint16_t sequence_id;

    static RequestHeader from_bytes(const uint8_t* buffer) {
        RequestHeader header;
        size_t offset = 0;

        std::memcpy(&header.magic, buffer + offset, sizeof(uint32_t));
        header.magic = ntohl(header.magic);
        offset += sizeof(uint32_t);

        header.command = static_cast<CommandType>(buffer[offset++]);
        header.flags = buffer[offset++];

        std::memcpy(&header.key_length, buffer + offset, sizeof(uint32_t));
        header.key_length = ntohl(header.key_length);
        offset += sizeof(uint32_t);

        std::memcpy(&header.value_length, buffer + offset, sizeof(uint32_t));
        header.value_length = ntohl(header.value_length);
        offset += sizeof(uint32_t);

        std::memcpy(&header.sequence_id, buffer + offset, sizeof(uint16_t));
        header.sequence_id = ntohs(header.sequence_id);

        return header;
    }

    bool is_valid() const {
        return magic == REQUEST_MAGIC &&
               (command == CommandType::GET ||
                command == CommandType::SET ||
                command == CommandType::DELETE ||
                command == CommandType::PING);
    }
};

struct ResponseHeader {
    uint32_t magic;
    StatusCode status;
    uint8_t flags;
    uint32_t data_length;
    uint16_t sequence_id;

    std::vector<uint8_t> to_bytes() const {
        std::vector<uint8_t> buffer(HEADER_SIZE, 0);
        size_t offset = 0;

        uint32_t magic_net = htonl(magic);
        std::memcpy(buffer.data() + offset, &magic_net, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        buffer[offset++] = static_cast<uint8_t>(status);
        buffer[offset++] = flags;

        uint32_t length_net = htonl(data_length);
        std::memcpy(buffer.data() + offset, &length_net, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        offset += sizeof(uint32_t);

        uint16_t seq_net = htons(sequence_id);
        std::memcpy(buffer.data() + offset, &seq_net, sizeof(uint16_t));

        return buffer;
    }
};

struct Request {
    RequestHeader header;
    std::string key;
    std::string value;
};

struct Response {
    ResponseHeader header;
    std::string data;

    static Response create_success(uint16_t seq_id, const std::string& data = "") {
        Response resp;
        resp.header.magic = RESPONSE_MAGIC;
        resp.header.status = StatusCode::OK;
        resp.header.flags = 0;
        resp.header.data_length = data.size();
        resp.header.sequence_id = seq_id;
        resp.data = data;
        return resp;
    }

    static Response create_error(uint16_t seq_id, StatusCode status) {
        Response resp;
        resp.header.magic = RESPONSE_MAGIC;
        resp.header.status = status;
        resp.header.flags = 0;
        resp.header.data_length = 0;
        resp.header.sequence_id = seq_id;
        return resp;
    }

    std::vector<uint8_t> to_bytes() const {
        auto header_bytes = header.to_bytes();
        std::vector<uint8_t> result(header_bytes.begin(), header_bytes.end());
        result.insert(result.end(), data.begin(), data.end());
        return result;
    }
};

}
