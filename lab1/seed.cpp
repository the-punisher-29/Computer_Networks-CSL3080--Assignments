#include "seed.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <thread>

Seed::Seed(const std::string& ip, int port) : ip(ip), port(port), running(false) {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverSocket, 10) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

void Seed::start() {
    running = true;
    while (running) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, 
                                &clientLen);
        
        if (clientSocket >= 0) {
            std::thread([this, clientSocket]() {
                char buffer[1024] = {0};
                int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (bytesRead > 0) {
                    std::string msg(buffer);
                    if (msg.find("Register") == 0) {
                        handleRegistration(clientSocket);
                    } else if (msg.find("GetPeers") == 0) {
                        handlePeerListRequest(clientSocket);
                    } else if (msg.find("Dead Node:") == 0) {
                        handleDeadNodeReport(msg);
                    }
                }
                close(clientSocket);
            }).detach();
        }
    }
}

void Seed::stop() {
    running = false;
    close(serverSocket);
}

void Seed::handleDeadNodeReport(const std::string& msg) {
    std::lock_guard<std::mutex> lock(peerListMutex);
    // Parse dead node IP and port from message
    // Remove from peer list
    logMessage("Received dead node report: " + msg);
}

void Seed::logMessage(const std::string& msg) {
    std::cout << msg << std::endl;
    std::ofstream outFile("seed_" + ip + "_" + std::to_string(port) + ".txt", 
                         std::ios::app);
    outFile << msg << std::endl;
}