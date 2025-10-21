#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <stdexcept>
#include <memory>
#include <vector>
#include "protocol.hpp"
#include "kv_store.hpp"
#include "persistence.hpp"
#include "thread_pool.hpp"

namespace kvstore {

class Server {
public:
    Server(int port, ShardedHashMap& store, AOFWriter& aof_writer)
        : port_(port), store_(store), aof_writer_(aof_writer),
          socket_fd_(-1), thread_pool_(std::thread::hardware_concurrency()) {}

    ~Server() {
        stop();
    }

    void start() {
        socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd_ < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        int opt = 1;
        setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);

        if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            close(socket_fd_);
            throw std::runtime_error("Failed to bind socket");
        }

        if (listen(socket_fd_, 128) < 0) {
            close(socket_fd_);
            throw std::runtime_error("Failed to listen on socket");
        }
    }

    void accept_loop() {
        while (!should_stop_) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);

            int client_fd = accept(socket_fd_,
                                  reinterpret_cast<sockaddr*>(&client_addr),
                                  &client_len);

            if (client_fd < 0) {
                if (should_stop_) break;
                continue;
            }

            thread_pool_.submit([this, client_fd]() {
                handle_client(client_fd);
            });
        }
    }

    void stop() {
        should_stop_ = true;
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }
        thread_pool_.shutdown();
    }

private:
    void handle_client(int client_fd) {
        while (true) {
            uint8_t header_buffer[HEADER_SIZE];
            ssize_t n = recv(client_fd, header_buffer, HEADER_SIZE, MSG_WAITALL);

            if (n <= 0) {
                break;
            }

            if (n != HEADER_SIZE) {
                send_error(client_fd, 0, StatusCode::PROTOCOL_ERROR);
                break;
            }

            auto header = RequestHeader::from_bytes(header_buffer);

            if (!header.is_valid()) {
                send_error(client_fd, header.sequence_id, StatusCode::INVALID_COMMAND);
                break;
            }

            std::string key;
            std::string value;

            if (header.key_length > 0) {
                key.resize(header.key_length);
                n = recv(client_fd, &key[0], header.key_length, MSG_WAITALL);
                if (n != static_cast<ssize_t>(header.key_length)) {
                    send_error(client_fd, header.sequence_id, StatusCode::PROTOCOL_ERROR);
                    break;
                }
            }

            if (header.value_length > 0) {
                value.resize(header.value_length);
                n = recv(client_fd, &value[0], header.value_length, MSG_WAITALL);
                if (n != static_cast<ssize_t>(header.value_length)) {
                    send_error(client_fd, header.sequence_id, StatusCode::PROTOCOL_ERROR);
                    break;
                }
            }

            handle_command(client_fd, header, key, value);
        }

        close(client_fd);
    }

    void handle_command(int client_fd, const RequestHeader& header,
                       const std::string& key, const std::string& value) {
        switch (header.command) {
            case CommandType::GET: {
                auto result = store_.get(key);
                if (result.has_value()) {
                    send_success(client_fd, header.sequence_id, result.value());
                } else {
                    send_error(client_fd, header.sequence_id, StatusCode::KEY_NOT_FOUND);
                }
                break;
            }
            case CommandType::SET: {
                store_.set(key, value);
                aof_writer_.log_set(key, value);
                send_success(client_fd, header.sequence_id);
                break;
            }
            case CommandType::DELETE: {
                bool removed = store_.remove(key);
                if (removed) {
                    aof_writer_.log_delete(key);
                    send_success(client_fd, header.sequence_id);
                } else {
                    send_error(client_fd, header.sequence_id, StatusCode::KEY_NOT_FOUND);
                }
                break;
            }
            case CommandType::PING: {
                send_success(client_fd, header.sequence_id);
                break;
            }
            default:
                send_error(client_fd, header.sequence_id, StatusCode::INVALID_COMMAND);
                break;
        }
    }

    void send_success(int client_fd, uint16_t seq_id, const std::string& data = "") {
        auto response = Response::create_success(seq_id, data);
        auto bytes = response.to_bytes();
        send(client_fd, bytes.data(), bytes.size(), 0);
    }

    void send_error(int client_fd, uint16_t seq_id, StatusCode status) {
        auto response = Response::create_error(seq_id, status);
        auto bytes = response.to_bytes();
        send(client_fd, bytes.data(), bytes.size(), 0);
    }

    int port_;
    ShardedHashMap& store_;
    AOFWriter& aof_writer_;
    int socket_fd_;
    ThreadPool thread_pool_;
    bool should_stop_ = false;
};

}
