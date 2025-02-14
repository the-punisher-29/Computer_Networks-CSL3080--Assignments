#include "seed.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <sstream>

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


void Seed::handleRegistration(int clientSocket) {
    char buffer[1024] = {0};
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        logMessage("Error receiving registration data");
        return;
    }
    
    std::string msg(buffer);
    size_t pos = msg.find("Register:");
    if (pos != 0) {
        logMessage("Invalid registration format");
        return;
    }
    
    try {
        std::string peerInfo = msg.substr(9); // Skip "Register:"
        std::istringstream iss(peerInfo);
        std::string ip;
        std::string portStr;
        
        if (std::getline(iss, ip, ':') && std::getline(iss, portStr)) {
            int port = std::stoi(portStr);
            if (port <= 0 || port > 65535) {
                throw std::runtime_error("Invalid port number");
            }
            
            {
                std::lock_guard<std::mutex> lock(peerListMutex);
                peerList.push_back({ip, port});
                std::string response = "Registration successful\n";
                send(clientSocket, response.c_str(), response.length(), 0);
                logMessage("Registered new peer: " + ip + ":" + std::to_string(port));
            }
        } else {
            throw std::runtime_error("Invalid peer info format");
        }
    } catch (const std::exception& e) {
        std::string error = "Registration failed: " + std::string(e.what()) + "\n";
        send(clientSocket, error.c_str(), error.length(), 0);
        logMessage(error);
    }
}

void Seed::handlePeerListRequest(int clientSocket) {
    std::string peerListStr;
    {
        std::lock_guard<std::mutex> lock(peerListMutex);
        for (const auto& peer : peerList) {
            peerListStr += peer.ip + ":" + std::to_string(peer.port) + "\n";
        }
    }
    
    send(clientSocket, peerListStr.c_str(), peerListStr.length(), 0);
}