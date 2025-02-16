#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <mutex>
#include <sstream>
#include <algorithm>
#include <random>
#include <ctime>
#include "seed.cpp" // For simplicity, we include seed.cpp directly.
using namespace std;

class PeerNode {
private:
string ip; // This peer's IP address
string port; // This peer's port number
// Set of connected peer IDs (each in "IP:Port" format)
unordered_set<string> connectedPeers;
// Set to record processed messages (to avoid duplicate forwarding)
unordered_set<string> messageList;
// List of all available seed nodes (read from config)
vector<SeedNode*> seedNodes;
ofstream outputFile; // Log file stream (appended to outputfile.txt)
mutex mtx;
// Map to count consecutive ping failures per connected peer.
unordered_map<string, int> pingMissCount;

public:
// Constructor: initializes the peer's IP, port and the seed node pointers.
PeerNode(const string &ip, const string &port, const vector<SeedNode*> &seeds)
: ip(ip), port(port), seedNodes(seeds) {
outputFile.open("outputfile.txt", ios::app);
if (!outputFile.is_open())
cerr << "Error opening outputfile.txt" << endl;
}
~PeerNode() {
if (outputFile.is_open())
outputFile.close();
}
// Returns the current timestamp as a string.
string getCurrentTimestamp() {
    time_t now = time(0);
    char buf[100];
    struct tm *timeinfo = localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);
    return string(buf);
}

// Registers with exactly floor(n/2)+1 randomly chosen seeds and uses ONLY those seeds’
// peer lists to create its union of peers. Then randomly (up to 3 peers) connects.
void registerWithSeeds() {
    int n = seedNodes.size();
    int required = (n / 2) + 1;  // For three seeds, required = 2.
    vector<SeedNode*> chosenSeeds;
    // Build chosenSeeds from seedNodes (randomly)
    for (auto seed : seedNodes)
        chosenSeeds.push_back(seed);
    random_device rd;
    mt19937 g(rd());
    shuffle(chosenSeeds.begin(), chosenSeeds.end(), g);
    chosenSeeds.resize(required);

    // Log which seeds were selected.
    ostringstream seedList;
    seedList << "Peer " << ip << ":" << port << " registering with seeds: ";
    for (auto seed : chosenSeeds)
        seedList << seed->getPeerList().empty() ? " (Empty) " : "";
    // (Alternatively, you may want to print the seedID if you modify SeedNode to provide it.)
    // For brevity, here we simply register with the chosen seeds:
    for (auto seed : chosenSeeds) {
        seed->registerPeer(ip, port);
    }
    
    // Retrieve the union of peer lists from the chosen seeds.
    unordered_set<string> globalPeerList;
    for (auto seed : chosenSeeds) {
        vector<string> pl = seed->getPeerList();
        for (const auto &p : pl)
            if (p != (ip + ":" + port))
                globalPeerList.insert(p);
    }
    
    // Randomly select up to 3 peers from union to connect with.
    vector<string> peersVec(globalPeerList.begin(), globalPeerList.end());
    shuffle(peersVec.begin(), peersVec.end(), g);
    int connectionLimit = min(3, (int)peersVec.size());
    for (int i = 0; i < connectionLimit; i++) {
        connectedPeers.insert(peersVec[i]);
        pingMissCount[peersVec[i]] = 0;
    }
    
    // Log the connected peers.
    ostringstream ss;
    ss << "Peer " << ip << ":" << port << " - Connected peers: ";
    for (auto p : connectedPeers)
        ss << p << " ";
    string logMsg = ss.str();
    cout << logMsg << endl;
    outputFile << logMsg << endl;
}

// Generates 10 gossip messages (one every 5 seconds).  
// Each message is printed in exactly this format:
// <self.timestamp>:<self.IP>:<self.Msg#>
void generateMessages() {
    // The first gossip message is generated only after registration and neighbor selection.
    for (int i = 1; i <= 10; i++) {
        string timestamp = getCurrentTimestamp();
        string message = timestamp + ":" + ip + ":Msg#" + to_string(i);
        // Output gossip message in required format.
        cout << message << endl;
        outputFile << message << endl;
        
        // Transmit (broadcast) the message to all adjacent peers.
        broadcastMessage(message, "");
        this_thread::sleep_for(chrono::seconds(5));
    }
}

// For every connected peer except the one specified by fromPeer, forward the message
// if it has not yet been processed.
void broadcastMessage(const string &message, const string &fromPeer) {
    lock_guard<mutex> lock(mtx);
    if (messageList.find(message) != messageList.end())
        return;
    messageList.insert(message);
    for (auto peer : connectedPeers) {
        if (peer == fromPeer)
            continue;
        // When a peer receives a message for the first time, it logs:
        // <self.timestamp>:<self.IP>:<self.Msg#>
        cout << message << endl;
        outputFile << message << endl;
    }
}

// Simulates receiving a gossip message from a neighboring peer. When first received,
// it prints the message (with a local timestamp and the IP of the sender) and then forwards it.
void receiveMessage(const string &message, const string &fromPeer) {
    lock_guard<mutex> lock(mtx);
    if (messageList.find(message) != messageList.end())
        return;
    messageList.insert(message);
    // Log reception from a neighbor (the sender's IP is included in message).
    cout << message << endl;
    outputFile << message << endl;
    broadcastMessage(message, fromPeer);
}

// Periodically pings each connected peer (every 13 seconds). If a peer fails 3 consecutive pings,
// it sends a dead-node report message to each seed that it connected with.
void checkLiveness() {
    while (true) {
        this_thread::sleep_for(chrono::seconds(13));
        vector<string> deadPeers;
        for (auto peer : connectedPeers) {
            bool pingSuccess = simulatePing();
            if (!pingSuccess) {
                pingMissCount[peer]++;
                // (Optional) You may log ping failures here.
                if (pingMissCount[peer] >= 3) {
                    reportDeadPeer(peer);
                    deadPeers.push_back(peer);
                }
            } else {
                pingMissCount[peer] = 0;
            }
        }
        for (auto dp : deadPeers) {
            connectedPeers.erase(dp);
            pingMissCount.erase(dp);
        }
    }
}

// Simulates a ping with about 70% chance of success.
bool simulatePing() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1, 10);
    return (dis(gen) > 3);
}

// When a peer is reported dead (after 3 missed pings), formats and outputs the dead-node message:
// Dead Node:<DeadNode.IP>:<DeadNode.Port>:<self.timestamp>:<self.IP>
// Then it notifies each seed by calling its removePeer() function.
void reportDeadPeer(const string &deadPeer) {
    string timestamp = getCurrentTimestamp();
    size_t pos = deadPeer.find(":");
    string deadIP = deadPeer.substr(0, pos);
    string deadPort = deadPeer.substr(pos + 1);
    string deadMsg = "Dead Node:" + deadIP + ":" + deadPort + ":" + timestamp + ":" + ip;
    cout << deadMsg << endl;
    outputFile << deadMsg << endl;
    for (auto seed : seedNodes)
        seed->removePeer(deadIP, deadPort);
}
};