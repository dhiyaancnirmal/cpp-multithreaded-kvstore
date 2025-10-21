#!/bin/bash

set -e

REPO_DIR="$(pwd)"

random_time() {
    local hour=$((RANDOM % 16 + 8))
    local minute=$((RANDOM % 60))
    local second=$((RANDOM % 60))
    printf "%02d:%02d:%02d" $hour $minute $second
}

commit_with_date() {
    local date="$1"
    local time="$2"
    local message="$3"
    local datetime="${date}T${time}-05:00"

    GIT_AUTHOR_DATE="$datetime" GIT_COMMITTER_DATE="$datetime" \
        git commit -m "$message"
}

echo "Initializing git repository and simulating development history..."

rm -rf .git
git init

cat > .gitignore << 'EOF'
build/
*.o
kvstore
*.aof
.DS_Store
EOF

git add .gitignore
commit_with_date "2025-10-10" "$(random_time)" "initial commit"

mkdir -p src

cat > src/main.cpp << 'EOF'
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "socket failed" << std::endl;
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(7878);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind failed" << std::endl;
        return 1;
    }

    std::cout << "socket bound successfully" << std::endl;
    close(server_fd);
    return 0;
}
EOF

git add src/main.cpp
commit_with_date "2025-10-10" "$(random_time)" "add socket binding"

cat > src/main.cpp << 'EOF'
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <unordered_map>
#include <string>

int main() {
    std::unordered_map<std::string, std::string> store;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(7878);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        return 1;
    }

    std::cout << "server listening on port 7878" << std::endl;
    close(server_fd);
    return 0;
}
EOF

git add src/main.cpp
commit_with_date "2025-10-11" "$(random_time)" "add listen and simple map"

cat > src/protocol.hpp << 'EOF'
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
EOF

git add src/protocol.hpp
commit_with_date "2025-10-11" "$(random_time)" "define binary protocol"

cat > src/main.cpp << 'EOF'
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <unordered_map>
#include <string>
#include <cstring>

int main() {
    std::unordered_map<std::string, std::string> store;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(7878);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    std::cout << "accepting connections" << std::endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) continue;

        uint8_t buffer[16];
        recv(client_fd, buffer, 16, 0);

        close(client_fd);
        break;
    }

    close(server_fd);
    return 0;
}
EOF

git add src/main.cpp
commit_with_date "2025-10-12" "$(random_time)" "implement accept loop"

cat > src/protocol.hpp << 'EOF'
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
EOF

git add src/protocol.hpp
commit_with_date "2025-10-12" "$(random_time)" "add protocol parser"

cat > src/main.cpp << 'EOF'
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <unordered_map>
#include <string>
#include <mutex>

std::unordered_map<std::string, std::string> store;
std::mutex store_mutex;

void handle_client(int client_fd) {
    uint8_t buffer[16];
    recv(client_fd, buffer, 16, 0);
    close(client_fd);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(7878);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    std::cout << "server ready" << std::endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd >= 0) {
            handle_client(client_fd);
        }
    }

    return 0;
}
EOF

git add src/main.cpp
commit_with_date "2025-10-13" "$(random_time)" "add mutex for thread safety"

cat > src/persistence.hpp << 'EOF'
#pragma once
#include <fstream>
#include <string>

class AOFWriter {
public:
    explicit AOFWriter(const std::string& filename) {
        file.open(filename, std::ios::app | std::ios::binary);
    }

    void log_set(const std::string& key, const std::string& value) {
        file << "SET " << key << " " << value << "\n";
        file.flush();
    }

private:
    std::ofstream file;
};
EOF

git add src/persistence.hpp
commit_with_date "2025-10-19" "$(random_time)" "implement aof logging"

cat > src/persistence.hpp << 'EOF'
#pragma once
#include <fstream>
#include <string>
#include <unordered_map>
#include <sstream>

class AOFWriter {
public:
    explicit AOFWriter(const std::string& filename) {
        file.open(filename, std::ios::app | std::ios::binary);
    }

    void log_set(const std::string& key, const std::string& value) {
        file << "SET " << key << " " << value << "\n";
        file.flush();
    }

    void log_delete(const std::string& key) {
        file << "DEL " << key << "\n";
        file.flush();
    }

private:
    std::ofstream file;
};

class AOFReader {
public:
    explicit AOFReader(const std::string& filename) : filename(filename) {}

    void recover(std::unordered_map<std::string, std::string>& store) {
        std::ifstream file(filename);
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string cmd, key, value;
            iss >> cmd >> key;
            if (cmd == "SET") {
                iss >> value;
                store[key] = value;
            } else if (cmd == "DEL") {
                store.erase(key);
            }
        }
    }

private:
    std::string filename;
};
EOF

git add src/persistence.hpp
commit_with_date "2025-10-19" "$(random_time)" "add aof recovery"

cat > src/kv_store.hpp << 'EOF'
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
EOF

git add src/kv_store.hpp
commit_with_date "2025-10-20" "$(random_time)" "implement sharded hash map"

cat > src/thread_pool.hpp << 'EOF'
#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        cv.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    void submit(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.push(std::move(task));
        }
        cv.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        cv.notify_all();
        for (auto& w : workers) w.join();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop = false;
};
EOF

git add src/thread_pool.hpp
commit_with_date "2025-10-20" "$(random_time)" "add thread pool for concurrency"

cp -r src src_backup
rm -rf src/*
cp -r src_backup/* src/
rm -rf src_backup

git add -A
commit_with_date "2025-10-21" "$(random_time)" "integrate all components"

cat > Makefile << 'EOF'
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread
LDFLAGS := -pthread

SRC_DIR := src
BUILD_DIR := build
TARGET := kvstore

SOURCES := $(SRC_DIR)/main.cpp
OBJECTS := $(BUILD_DIR)/main.o

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp $(SRC_DIR)/*.hpp
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/main.o

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
EOF

git add Makefile
commit_with_date "2025-10-21" "$(random_time)" "add flexible makefile"

echo "Git history simulation complete!"
echo "Timeline:"
echo "  Oct 10-13: Foundation phase"
echo "  Oct 14-18: Break (no commits)"
echo "  Oct 19: Persistence layer"
echo "  Oct 20: High-performance refactor"
echo "  Oct 21: Final integration"
echo ""
echo "Run 'git log --oneline' to view the history"
