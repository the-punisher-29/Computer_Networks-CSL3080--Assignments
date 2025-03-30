// main.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <cstdlib>
#include "seed.cpp"
#include "peer.cpp"
using namespace std;

// Reads seed information from config.txt (each line: "IP:Port") and returns a vector of (IP, port) pairs.
vector<pair<string,int>> readSeedConfig(const string &filename) {
    vector<pair<string,int>> seeds;
    ifstream infile(filename);
    if(!infile.is_open()) {
        cerr << "Error: cannot open " << filename << endl;
        return seeds;
    }
    string line;
    while(getline(infile, line)) {
        if(line.empty())
            continue;
        size_t pos = line.find(":");
        if(pos == string::npos)
            continue;
        string ip = line.substr(0, pos);
        int port = atoi(line.substr(pos+1).c_str());
        seeds.push_back(make_pair(ip, port));
    }
    infile.close();
    return seeds;
}

int main() {
    // Clear previous output.
    ofstream ofs("outputfile.txt", ios::out);
    ofs.close();

    // Read seeds from config.txt.
    vector<pair<string,int>> seeds = readSeedConfig("config.txt");

    // Start seed servers (one per seed from config.txt).
    vector<thread> seedThreads;
    vector<SeedServer*> seedServers;
    for(auto &s : seeds) {
        SeedServer *server = new SeedServer(s.first + ":" + to_string(s.second), s.second);
        seedServers.push_back(server);
        seedThreads.push_back(thread(&SeedServer::run, server));
    }

    // For simulation, create multiple PeerNode instances.
    // In a real deployment, these would be launched separately (possibly on different machines).
    PeerNode peer1("127.0.0.1", "5000", seeds);
    PeerNode peer2("127.0.0.1", "5001", seeds);
    // Start each peer's listener.
    peer1.startListener();
    peer2.startListener();
    // Register with seeds and discover neighbors.
    peer1.registerWithSeeds();
    peer2.registerWithSeeds();
    // Start gossip generation and liveness checking for each peer.
    thread t1(&PeerNode::generateGossip, &peer1);
    thread t2(&PeerNode::checkLiveness, &peer1);
    thread t3(&PeerNode::generateGossip, &peer2);
    thread t4(&PeerNode::checkLiveness, &peer2);
    t1.join();
    t3.join();
    // The liveness threads run indefinitely; in testing, terminate after some time.

    // Optionally join seed server threads (which run indefinitely).
    for(auto &t : seedThreads) {
        if(t.joinable())
            t.join();
    }

    // Cleanup: delete seed server instances.
    for(auto s : seedServers)
        delete s;
    return 0;
}
