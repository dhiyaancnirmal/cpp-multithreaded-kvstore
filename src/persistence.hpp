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
