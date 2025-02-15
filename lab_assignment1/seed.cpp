#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <string>
#include <sstream>
using namespace std;

class SeedNode {
private:
// Map peer "IP:Port" -> status ("Alive")
unordered_map<string, string> peerList;
mutex mtx;
string seedID; // For example, "192.168.1.10:6000"
ofstream outputFile; // Log file stream

public:
// Constructor: opens outputfile.txt in append mode.
SeedNode(const string &id) : seedID(id) {
outputFile.open("outputfile.txt", ios::app);
if (!outputFile.is_open())
cerr << "Error opening outputfile.txt" << endl;
}
~SeedNode() {
if (outputFile.is_open())
outputFile.close();
}
// Registers a new peer.
void registerPeer(const string &ip, const string &port) {
    lock_guard<mutex> lock(mtx);
    string key = ip + ":" + port;
    peerList[key] = "Alive";
    string logMsg = "Seed " + seedID + " - Peer registered: " + key;
    cout << logMsg << endl;
    outputFile << logMsg << endl;
}

// Removes a dead peer.
void removePeer(const string &ip, const string &port) {
    lock_guard<mutex> lock(mtx);
    string key = ip + ":" + port;
    if (peerList.erase(key)) {
        string logMsg = "Seed " + seedID + " - Dead peer removed: " + key;
        cout << logMsg << endl;
        outputFile << logMsg << endl;
    }
}

// Returns the current Peer List.
vector<string> getPeerList() {
    lock_guard<mutex> lock(mtx);
    vector<string> peers;
    for (auto &entry : peerList)
        peers.push_back(entry.first);
    return peers;
}
};