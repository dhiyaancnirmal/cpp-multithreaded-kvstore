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
