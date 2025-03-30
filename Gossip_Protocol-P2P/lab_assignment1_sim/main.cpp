#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include "peer.cpp"
using namespace std;

// Reads seed information from a file (one "IP:Port" per line)
vector<string> readConfig(const string &filename) {
vector<string> seedInfos;
ifstream infile(filename);
if (!infile.is_open()) {
cerr << "Error: cannot open config file " << filename << endl;
return seedInfos;
}
string line;
while (getline(infile, line)) {
if (!line.empty())
seedInfos.push_back(line);
}
infile.close();
return seedInfos;
}

// Prints protocol information header to both the console and outputfile.txt.
void printProtocolInfo() {
ofstream output("outputfile.txt", ios::app);
ostringstream oss;
oss << "Gossip Message format:" << "\n"
<< " <self.timestamp>:<self.IP>:<self.Msg#>" << "\n\n"
<< "Gossip protocol:" << "\n"
<< " After a node (peer) generates a message M, it transmits M to all its adjacent nodes." << "\n"
<< " On receipt of a message for the first time, a node records it in its Message List (ML)," << "\n"
<< " and forwards it to all peers except the sender." << "\n"
<< " On receiving the same message subsequently, the node ignores it." << "\n\n"
<< "Reporting the node as ‘Dead’:" << "\n"
<< " When 3 consecutive ping messages do not receive a reply, the peer sends a message of the format:" << "\n"
<< " Dead Node:<DeadNode.IP>:<DeadNode.Port>:<self.timestamp>:<self.IP>" << "\n\n"
<< "Program Output:" << "\n"
<< " Each seed logs connection requests and dead-node notifications." << "\n"
<< " Each peer logs the list of neighbors obtained and each gossip message (with its timestamp and sender info)." << "\n\n";
cout << oss.str() << endl;
output << oss.str() << endl;
output.close();
}

int main() {
// Clear the output file at the start.
ofstream ofs("outputfile.txt", ios::out);
ofs.close();
// Print header information.
printProtocolInfo();

// Read the seed configuration from config.txt.
vector<string> seedInfos = readConfig("config.txt");
vector<SeedNode*> seeds;
for (auto &info : seedInfos) {
    SeedNode* seed = new SeedNode(info);
    seeds.push_back(seed);
}

// Create 5 peer nodes with sample IP addresses and ports.
vector<PeerNode*> peers;
peers.push_back(new PeerNode("192.168.1.101", "5000", seeds));
peers.push_back(new PeerNode("192.168.1.102", "5001", seeds));
peers.push_back(new PeerNode("192.168.1.103", "5002", seeds));
peers.push_back(new PeerNode("192.168.1.104", "5003", seeds));
peers.push_back(new PeerNode("192.168.1.105", "5004", seeds));

// Each peer registers with exactly floor(n/2)+1 (i.e. 2 for 3 seeds) randomly chosen seeds.
for (auto p : peers)
    p->registerWithSeeds();

// Start threads for each peer:
//  - one thread generates gossip messages (stops after 10 messages)
//  - one thread continuously checks for liveness (runs indefinitely; these may be detached)
vector<thread> threads;
for (auto p : peers) {
    threads.push_back(thread(&PeerNode::generateMessages, p));
    threads.push_back(thread(&PeerNode::checkLiveness, p)); // runs indefinitely
}

// Join the finite threads (gossip message generation).
for (auto &t : threads) {
    if (t.joinable())
        t.join();
}

// Cleanup: delete allocated PeerNode and SeedNode objects.
for (auto p : peers)
    delete p;
for (auto s : seeds)
    delete s;

return 0;
}

