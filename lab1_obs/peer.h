#pragma once
#include "config.h"
#include "msg.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <thread>
#include <mutex>
#include <string>

class Peer {
public:
    Peer(const std::string& ip, int port);
    void start();
    void stop();

private:
    struct ConnectedPeer {
        std::string ip;
        int port;
        int socket;
        int missedPings;
    };

    // Member variables
    std::string ip;
    int port;
    int serverSocket;
    std::vector<NodeInfo> connectedSeeds;
    std::vector<ConnectedPeer> connectedPeers;
    std::unordered_map<size_t, std::unordered_set<std::string>> messageList; // hash -> set of peer IPs
    int messageCount;
    bool running;
    std::mutex peersMutex;
    std::vector<std::thread> threads;

    // Server initialization and connection handling
    void initializeServer();
    void handleIncomingConnections();
    void handlePeerMessages(const ConnectedPeer& peer);

    // Seed and peer connection management
    void connectToSeeds();
    void connectToPeers(const std::vector<NodeInfo>& availablePeers);
    std::vector<NodeInfo> parsePeerList(const std::string& peerListStr);
    bool isPowerLawDistribution(const std::vector<NodeInfo>& peers);

    // Message handling and broadcasting
    void generateAndBroadcastMessages();
    void broadcastMessage(const std::string& msg);
    void handleMessage(const std::string& msg, const std::string& senderIP);

    // Peer monitoring and status reporting
    void pingPeers();
    void reportDeadNode(const ConnectedPeer& peer);

    // Utility functions
    void logMessage(const std::string& msg);
};