// seed.h
#pragma once
#include "config.h"
#include <vector>
#include <mutex>

class Seed {
public:
    Seed(const std::string& ip, int port);
    void start();
    void stop();

private:
    std::string ip;
    int port;
    int serverSocket;
    std::vector<NodeInfo> peerList;
    std::mutex peerListMutex;
    bool running;

    void handleRegistration(int clientSocket);
    void handlePeerListRequest(int clientSocket);
    void handleDeadNodeReport(const std::string& msg);
    void logMessage(const std::string& msg);
};