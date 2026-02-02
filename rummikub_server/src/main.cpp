#include "../include/Server.hpp"
#include "../include/Shared.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;

    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    std::cout << "\t[DEBUG] Starting Rummikub Server on port " << port << "..." << std::endl;
    
    GameServer server(port);
    server.run();

    return 0;
}