#include "peer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstring>
Peer::Peer(const std::string& ip, int port) : ip(ip), port(port), 
    messageCount(0), running(false) {
    initializeServer();
}

void Peer::start() {
    running = true;
    connectToSeeds();
    
    // Start threads for different tasks
    threads.emplace_back(&Peer::handleIncomingConnections, this);
    threads.emplace_back(&Peer::generateAndBroadcastMessages, this);
    threads.emplace_back(&Peer::pingPeers, this);
}

void Peer::stop() {
    running = false;
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    close(serverSocket);
}

void Peer::initializeServer() {
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

void Peer::connectToSeeds() {
    Config config = Config::loadFromFile("config.txt");
    
    // Randomly select ⌊(n/2)⌋ + 1 seeds
    std::vector<NodeInfo> availableSeeds = config.seeds;
    int numSeedsRequired = (availableSeeds.size() / 2) + 1;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(availableSeeds.begin(), availableSeeds.end(), gen);
    
    for (int i = 0; i < numSeedsRequired && i < availableSeeds.size(); ++i) {
        int seedSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (seedSocket < 0) continue;
        
        sockaddr_in seedAddr{};
        seedAddr.sin_family = AF_INET;
        seedAddr.sin_addr.s_addr = inet_addr(availableSeeds[i].ip.c_str());
        seedAddr.sin_port = htons(availableSeeds[i].port);
        
        if (connect(seedSocket, (struct sockaddr*)&seedAddr, sizeof(seedAddr)) >= 0) {
            // Register with seed
            std::string regMsg = "Register:" + ip + ":" + std::to_string(port);
            send(seedSocket, regMsg.c_str(), regMsg.length(), 0);
            
            // Get peer list from seed
            std::string getPeersMsg = "GetPeers";
            send(seedSocket, getPeersMsg.c_str(), getPeersMsg.length(), 0);
            
            char buffer[4096] = {0};
            int bytesRead = recv(seedSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead > 0) {
                std::vector<NodeInfo> seedPeers = parsePeerList(std::string(buffer));
                connectToPeers(seedPeers);
            }
            
            connectedSeeds.push_back(availableSeeds[i]);
        }
        close(seedSocket);
    }
}

void Peer::generateAndBroadcastMessages() {
    while (running && messageCount < 10) {
        Message msg{time(nullptr), ip, messageCount++};
        broadcastMessage(msg.toString());
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void Peer::pingPeers() {
    while (running) {
        std::lock_guard<std::mutex> lock(peersMutex);
        
        for (auto it = connectedPeers.begin(); it != connectedPeers.end();) {
            std::string cmd = "ping -c 1 " + it->ip + " >/dev/null 2>&1";
            if (system(cmd.c_str()) != 0) {
                it->missedPings++;
                if (it->missedPings >= 3) {
                    reportDeadNode(*it);
                    close(it->socket);
                    it = connectedPeers.erase(it);
                    continue;
                }
            } else {
                it->missedPings = 0;
            }
            ++it;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(13));
    }
}

bool Peer::isPowerLawDistribution(const std::vector<NodeInfo>& peers) {
    if (peers.empty()) return false;
    
    // Calculate current degree distribution
    std::unordered_map<std::string, int> degrees;
    for (const auto& peer : connectedPeers) {
        degrees[peer.ip + ":" + std::to_string(peer.port)]++;
    }
    
    // Sort degrees in descending order
    std::vector<int> degreeValues;
    for (const auto& [node, degree] : degrees) {
        degreeValues.push_back(degree);
    }
    std::sort(degreeValues.begin(), degreeValues.end(), std::greater<>());
    
    // Calculate degree frequency
    std::unordered_map<int, int> degreeFreq;
    for (int degree : degreeValues) {
        degreeFreq[degree]++;
    }
    
    // Calculate log values for power law verification
    std::vector<double> logDegree, logFreq;
    for (const auto& [degree, freq] : degreeFreq) {
        if (degree > 0 && freq > 0) {
            logDegree.push_back(log(degree));
            logFreq.push_back(log(freq));
        }
    }
    
    if (logDegree.size() < 2) return false;
    
    // Linear regression on log-log values
    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    int n = logDegree.size();
    
    for (int i = 0; i < n; i++) {
        sumX += logDegree[i];
        sumY += logFreq[i];
        sumXY += logDegree[i] * logFreq[i];
        sumX2 += logDegree[i] * logDegree[i];
    }
    
    // Calculate correlation coefficient
    double r = (n * sumXY - sumX * sumY) / 
               sqrt((n * sumX2 - sumX * sumX) * (n * sumY * sumY - sumY * sumY));
    
    return r * r > 0.8;  // Check if correlation is strong enough
}

void Peer::connectToPeers(const std::vector<NodeInfo>& availablePeers) {
    std::lock_guard<std::mutex> lock(peersMutex);
    
    // Filter out already connected peers and self
    std::vector<NodeInfo> newPeers;
    for (const auto& peer : availablePeers) {
        bool alreadyConnected = false;
        for (const auto& connected : connectedPeers) {
            if (peer.ip == connected.ip && peer.port == connected.port) {
                alreadyConnected = true;
                break;
            }
        }
        if (!alreadyConnected && (peer.ip != ip || peer.port != port)) {
            newPeers.push_back(peer);
        }
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    
    while (!newPeers.empty() && !isPowerLawDistribution(newPeers)) {
        std::uniform_int_distribution<> dis(0, newPeers.size() - 1);
        int idx = dis(gen);
        
        int peerSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (peerSocket < 0) continue;
        
        sockaddr_in peerAddr{};
        peerAddr.sin_family = AF_INET;
        peerAddr.sin_addr.s_addr = inet_addr(newPeers[idx].ip.c_str());
        peerAddr.sin_port = htons(newPeers[idx].port);
        
        if (connect(peerSocket, (struct sockaddr*)&peerAddr, sizeof(peerAddr)) >= 0) {
            ConnectedPeer newPeer{
                newPeers[idx].ip,
                newPeers[idx].port,
                peerSocket,
                0  // Initial missed pings
            };
            connectedPeers.push_back(newPeer);
            
            threads.emplace_back([this, newPeer]() {
                handlePeerMessages(newPeer);
            });
        }
        
        newPeers.erase(newPeers.begin() + idx);
    }
}

void Peer::handlePeerMessages(const ConnectedPeer& peer) {
    char buffer[4096];
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(peer.socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesRead <= 0) {
            std::lock_guard<std::mutex> lock(peersMutex);
            auto it = std::find_if(connectedPeers.begin(), connectedPeers.end(),
                [&peer](const ConnectedPeer& p) {
                    return p.ip == peer.ip && p.port == peer.port;
                });
            if (it != connectedPeers.end()) {
                close(it->socket);
                connectedPeers.erase(it);
            }
            break;
        }
        
        std::string msg(buffer);
        handleMessage(msg, peer.ip + ":" + std::to_string(peer.port));
    }
}

void Peer::reportDeadNode(const ConnectedPeer& peer) {
    std::string msg = Message::createDeadNodeMessage(peer.ip, peer.port, ip);
    logMessage("Reporting dead node: " + msg);
    
    for (const auto& seed : connectedSeeds) {
        int seedSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (seedSocket < 0) continue;
        
        sockaddr_in seedAddr{};
        seedAddr.sin_family = AF_INET;
        seedAddr.sin_addr.s_addr = inet_addr(seed.ip.c_str());
        seedAddr.sin_port = htons(seed.port);
        
        if (connect(seedSocket, (struct sockaddr*)&seedAddr, sizeof(seedAddr)) >= 0) {
            send(seedSocket, msg.c_str(), msg.length(), 0);
        }
        close(seedSocket);
    }
}

void Peer::logMessage(const std::string& msg) {
    std::cout << msg << std::endl;
    std::ofstream outFile("peer_" + ip + "_" + std::to_string(port) + ".txt", 
                         std::ios::app);
    outFile << msg << std::endl;
}

std::vector<NodeInfo> Peer::parsePeerList(const std::string& peerListStr) {
    std::vector<NodeInfo> peers;
    std::istringstream iss(peerListStr);
    std::string line;
    
    while (std::getline(iss, line)) {
        std::istringstream lineStream(line);
        std::string ip;
        int port;
        
        if (std::getline(lineStream, ip, ':') && lineStream >> port) {
            peers.push_back({ip, port});
        }
    }
    
    return peers;
}
void Peer::handleIncomingConnections() {
    while (running) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket >= 0) {
            std::string clientIP = inet_ntoa(clientAddr.sin_addr);
            int clientPort = ntohs(clientAddr.sin_port);
            
            ConnectedPeer newPeer{clientIP, clientPort, clientSocket, 0};
            
            {
                std::lock_guard<std::mutex> lock(peersMutex);
                connectedPeers.push_back(newPeer);
            }
            
            threads.emplace_back([this, newPeer]() {
                handlePeerMessages(newPeer);
            });
        }
    }
}

void Peer::broadcastMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(peersMutex);
    size_t msgHash = Message::hash(msg);
    
    for (const auto& peer : connectedPeers) {
        send(peer.socket, msg.c_str(), msg.length(), 0);
    }
    
    messageList[msgHash].insert(ip + ":" + std::to_string(port));
}

void Peer::handleMessage(const std::string& msg, const std::string& senderIP) {
    size_t msgHash = Message::hash(msg);
    
    {
        std::lock_guard<std::mutex> lock(peersMutex);
        if (messageList[msgHash].find(senderIP) != messageList[msgHash].end()) {
            return; // Already received this message
        }
        
        messageList[msgHash].insert(senderIP);
    }
    
    logMessage("Received: " + msg + " from " + senderIP);
    broadcastMessage(msg); // Forward to other peers
}