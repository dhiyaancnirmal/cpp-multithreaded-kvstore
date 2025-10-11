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
