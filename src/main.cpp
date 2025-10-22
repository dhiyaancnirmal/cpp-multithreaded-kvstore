#include <iostream>
#include <csignal>
#include <atomic>
#include <memory>
#include "kv_store.hpp"
#include "persistence.hpp"
#include "networking.hpp"

std::atomic<bool> g_shutdown_requested{false};
kvstore::Server* g_server = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_shutdown_requested.store(true);
        if (g_server) {
            g_server->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    int port = 7878;
    std::string aof_file = "store.aof";

    if (argc > 1) {
        port = std::stoi(argv[1]);
    }
    if (argc > 2) {
        aof_file = argv[2];
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        kvstore::ShardedHashMap store;

        std::cout << "Recovering from AOF: " << aof_file << std::endl;
        kvstore::AOFReader reader(aof_file);
        if (!reader.recover(store)) {
            std::cerr << "AOF recovery failed, starting with empty store" << std::endl;
        } else {
            std::cout << "Recovered " << store.total_size() << " keys" << std::endl;
        }

        kvstore::AOFWriter aof_writer(aof_file);
        kvstore::Server server(port, store, aof_writer);
        g_server = &server;

        std::cout << "Starting server on port " << port << std::endl;
        server.start();

        std::cout << "Server listening, press Ctrl+C to stop" << std::endl;
        server.accept_loop();

        std::cout << "Shutting down gracefully..." << std::endl;
        aof_writer.flush();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
