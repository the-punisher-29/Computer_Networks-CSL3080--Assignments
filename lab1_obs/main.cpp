// main.cpp
#include "peer.h"
#include "seed.h"
#include <iostream>
#include <string>

void printUsage() {
    std::cout << "Usage:\n";
    std::cout << "For seed node: ./p2p seed <ip> <port>\n";
    std::cout << "For peer node: ./p2p peer <ip> <port>\n";
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printUsage();
        return 1;
    }

    std::string nodeType = argv[1];
    std::string ip = argv[2];
    int port = std::stoi(argv[3]);

    try {
        if (nodeType == "seed") {
            Seed seed(ip, port);
            std::cout << "Starting seed node at " << ip << ":" << port << std::endl;
            seed.start();
        } 
        else if (nodeType == "peer") {
            Peer peer(ip, port);
            std::cout << "Starting peer node at " << ip << ":" << port << std::endl;
            peer.start();
        }
        else {
            printUsage();
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    // Keep main thread running
    std::string input;
    std::cout << "Enter 'quit' to stop the node: ";
    while (std::getline(std::cin, input)) {
        if (input == "quit") {
            break;
        }
    }

    return 0;
}