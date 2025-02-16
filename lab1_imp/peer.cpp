// peer.cpp
/*
  PeerNode implements a TCP-socket–based peer node that supports:
    • Listening for connections (to receive gossip messages and pings).
    • Registering with a randomly selected subset of seeds (floor(n/2)+1) from config.
    • Retrieving a weighted (preferential) union of peer lists from seeds.
    • Selecting up to 3 neighbors (using duplicates to bias high-degree peers).
    • Establishing outgoing connections to neighbors.
    • Generating gossip messages every 5 seconds in the format:
          <timestamp>:<self.IP>:<self.Msg#>
    • Forwarding new gossip messages to neighbors, avoiding duplicates.
    • Pinging neighbors every 13 seconds with nonblocking I/O; if 3 consecutive pings fail, sending a
      DEAD message (format: Dead Node:<DeadNode.IP>:<DeadNode.Port>:<self.timestamp>:<self.IP>) to all seeds.
      
  Advanced error checking, nonblocking I/O, and additional security (e.g., TLS and message signing) are noted
  but only basic support is implemented.
*/

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>   // for fcntl
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <random>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <random>  
using namespace std;

class PeerNode {
public:
    string myIP;  // e.g., "127.0.0.1"
    string myPort; // e.g., "5000"
    // All seeds read from config.
    vector<pair<string,int>> allSeeds;
    // Seeds selected for registration (floor(n/2)+1).
    vector<pair<string,int>> chosenSeeds;
    // Connected neighbors in "IP:Port" format.
    unordered_set<string> connectedNeighbors;
    // Map from neighbor to its socket FD.
    unordered_map<string, int> neighborSock;
    // Tracking ping failures.
    unordered_map<string, int> pingFailures;
    // Processed gossip messages.
    unordered_set<string> messageHistory;
    mutex mtx;
    ofstream outputFile;

    PeerNode(const string &ip, const string &port, const vector<pair<string,int>> &seeds)
      : myIP(ip), myPort(port), allSeeds(seeds) {
        outputFile.open("outputfile.txt", ios::app);
        if(!outputFile.is_open())
            cerr << "Error opening outputfile.txt" << endl;
    }

    // Utility function: get current timestamp.
    string getCurrentTimestamp() {
        time_t now = time(nullptr);
        char timebuf[64];
        struct tm *timeinfo = localtime(&now);
        if (strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", timeinfo) == 0)
            return "00-00-00 00:00:00";
        return string(timebuf);
    }

    // ------------------------------
    // Peer Listener: Accept incoming connections.
    // ------------------------------
    void listenForPeerConnections() {
        int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(listen_fd < 0) {
            perror("Peer listener socket creation failed");
            exit(EXIT_FAILURE);
        }
        int opt = 1;
        if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt failed");
            close(listen_fd);
            exit(EXIT_FAILURE);
        }
        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(myIP.c_str());
        address.sin_port = htons(atoi(myPort.c_str()));
        if(bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("Peer bind failed");
            close(listen_fd);
            exit(EXIT_FAILURE);
        }
        if(listen(listen_fd, 10) < 0) {
            perror("Peer listen failed");
            close(listen_fd);
            exit(EXIT_FAILURE);
        }
        cout << "Peer listening on " << myIP << ":" << myPort << endl;
        while(true) {
            struct sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            int clientSock = accept(listen_fd, (struct sockaddr *)&clientAddr, &addrLen);
            if(clientSock < 0) {
                perror("Peer accept failed");
                continue;
            }
            thread t(&PeerNode::handleIncomingConnection, this, clientSock);
            t.detach();
        }
        close(listen_fd);
    }

    void handleIncomingConnection(int sock) {
        char buf[1024];
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        getpeername(sock, (struct sockaddr*)&addr, &addr_len);
        char sender_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), sender_ip, INET_ADDRSTRLEN);
        
        while(true) {
            memset(buf, 0, sizeof(buf));
            int bytes = read(sock, buf, sizeof(buf)-1);
            if(bytes <= 0)
                break;
            buf[bytes] = '\0';
            string recvMsg(buf);
            
            if(recvMsg.find("PING") == 0) {
                string pong = "PONG";
                if(send(sock, pong.c_str(), pong.size(), 0) < 0)
                    perror("Error sending PONG");
            } else if(recvMsg.find("PONG") == 0) {
                // do nothing for PONG
            } else { // gossip message
                {
                    lock_guard<mutex> lock(mtx);
                    if(messageHistory.find(recvMsg) != messageHistory.end())
                        continue;
                    messageHistory.insert(recvMsg);
                    
                    string timestamp = getCurrentTimestamp();
                    string logMsg = timestamp + " - Received new gossip from " + string(sender_ip) + ": " + recvMsg;
                    cout << logMsg << endl;
                    outputFile << logMsg << endl;
                    forwardGossip(recvMsg, sock);
                }
            }
        }
        close(sock);
    }

    void forwardGossip(const string &msg, int senderSock) {
        lock_guard<mutex> lock(mtx);
        for(auto &entry : neighborSock) {
            int s = entry.second;
            if(s != senderSock)
                send(s, msg.c_str(), msg.size(), 0);
        }
    }

    // ------------------------------
    // Outgoing Connections & Registration
    // ------------------------------
    int connectToPeer(const string &peerIP, const string &peerPort) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0) {
            perror("socket() failed in connectToPeer");
            return -1;
        }
        // Set nonblocking mode.
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
        struct sockaddr_in servAddr;
        memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(atoi(peerPort.c_str()));
        if(inet_pton(AF_INET, peerIP.c_str(), &servAddr.sin_addr) <= 0) {
            cerr << "Invalid address in connectToPeer" << endl;
            close(sockfd);
            return -1;
        }
        int ret = connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr));
        if(ret < 0 && errno != EINPROGRESS) {
            perror("Connect failed in connectToPeer");
            close(sockfd);
            return -1;
        }
        // Wait for connection in nonblocking mode.
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(sockfd, &wfds);
        struct timeval tv = {5, 0};  // 5-second timeout
        int sel = select(sockfd+1, nullptr, &wfds, nullptr, &tv);
        if(sel <= 0) {
            cerr << "Connection timeout for peer " << peerIP << ":" << peerPort << endl;
            close(sockfd);
            return -1;
        }
        // Restore socket to blocking mode.
        flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK);
        return sockfd;
    }

    // Registers with a random subset of seeds (floor(n/2)+1) and retrieves union of their peer lists
    // to select up to 3 neighbors using a weighted (preferential) method.
    void registerWithSeeds() {
        int n = allSeeds.size();
        int required = (n / 2) + 1;
        vector<pair<string,int>> seedsCopy = allSeeds;
        random_device rd;
        mt19937 g(rd());
        shuffle(seedsCopy.begin(), seedsCopy.end(), g);
        seedsCopy.resize(required);
        chosenSeeds = seedsCopy;
        // Accumulate peers (allow duplicates) from chosen seeds.
        vector<string> accumulatedPeers;
        for(auto &seed : chosenSeeds) {
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if(sockfd < 0) { perror("Socket failed for seed registration"); continue; }
            struct sockaddr_in servAddr;
            memset(&servAddr, 0, sizeof(servAddr));
            servAddr.sin_family = AF_INET;
            servAddr.sin_port = htons(seed.second);
            if(inet_pton(AF_INET, seed.first.c_str(), &servAddr.sin_addr) <= 0) {
                cerr << "Invalid seed address " << seed.first << endl;
                close(sockfd);
                continue;
            }
            if(connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
                perror("Connect failed for seed registration");
                close(sockfd);
                continue;
            }
            // Send REGISTER.
            string regCmd = "REGISTER " + myIP + " " + myPort;
            send(sockfd, regCmd.c_str(), regCmd.size(), 0);
            // Send GET_PEERS.
            string getCmd = "GET_PEERS";
            send(sockfd, getCmd.c_str(), getCmd.size(), 0);
            char buffer[1024] = {0};
            int len = read(sockfd, buffer, sizeof(buffer)-1);
            if(len > 0) {
                buffer[len] = '\0';
                string peersStr(buffer);
                istringstream iss(peersStr);
                string token;
                while(getline(iss, token, ',')) {
                    if(token != (myIP + ":" + myPort) && !token.empty())
                        accumulatedPeers.push_back(token);
                }
            }
            else {
                cerr << "Failed to read peer list from seed " << seed.first << ":" << seed.second << endl;
            }
            close(sockfd);
        }
        // Preferential attachment: duplicates in accumulatedPeers increase chance of selection.
        unordered_set<string> selectedNeighbors;
        shuffle(accumulatedPeers.begin(), accumulatedPeers.end(), g);
        for(auto &p : accumulatedPeers) {
            if(selectedNeighbors.find(p) == selectedNeighbors.end()) {
                selectedNeighbors.insert(p);
                if(selectedNeighbors.size() >= 3)
                    break;
            }
        }
        {
            lock_guard<mutex> lock(mtx);
            for(auto &nbr : selectedNeighbors)
                connectedNeighbors.insert(nbr);
        }
        // Establish outgoing connections.
        for(auto &nbr : connectedNeighbors) {
            size_t pos = nbr.find(":");
            if(pos == string::npos) continue;
            string peerIP = nbr.substr(0, pos);
            string peerPort = nbr.substr(pos+1);
            int sock = connectToPeer(peerIP, peerPort);
            if(sock != -1) {
                lock_guard<mutex> lock(mtx);
                neighborSock[nbr] = sock;
                pingFailures[nbr] = 0;
            }
        }
        ostringstream oss;
        oss << "Peer " << myIP << ":" << myPort << " - Connected neighbors: ";
        for(auto &nbr : connectedNeighbors)
            oss << nbr << " ";
        cout << oss.str() << endl;
        outputFile << oss.str() << endl;
    }

    // ------------------------------
    // Gossip Generation & Forwarding
    // ------------------------------
    // Generates 10 gossip messages (every 5 seconds) in the format:
    // <timestamp>:<self.IP>:Msg#<number>
    void generateGossip() {
        for (int i = 1; i <= 10; i++) {
            string timestamp = getCurrentTimestamp();
            string message = timestamp + ":" + myIP + ":Msg#" + to_string(i);
            cout << message << endl;
            outputFile << message << endl;
            {
                lock_guard<mutex> lock(mtx);
                messageHistory.insert(message);
            }
            lock_guard<mutex> lock(mtx);
            for(auto &entry : neighborSock) {
                int sock = entry.second;
                if(send(sock, message.c_str(), message.size(), 0) < 0)
                    perror("Error sending gossip message");
            }
            this_thread::sleep_for(chrono::seconds(5));
        }
    }

    // ------------------------------
    // Liveness (Ping/Pong) Check
    // ------------------------------
    void checkLiveness() {
        while(true) {
            this_thread::sleep_for(chrono::seconds(13));
            lock_guard<mutex> lock(mtx);
            vector<string> neighs;
            for(auto &entry : neighborSock)
                neighs.push_back(entry.first);
            for(auto &nbr : neighs) {
                int sock = neighborSock[nbr];
                string pingMsg = "PING";
                if(send(sock, pingMsg.c_str(), pingMsg.size(), 0) < 0)
                    perror("Error sending PING");
                int flags = fcntl(sock, F_GETFL, 0);
                fcntl(sock, F_SETFL, flags | O_NONBLOCK);
                char reply[64] = {0};
                int n = recv(sock, reply, sizeof(reply)-1, MSG_DONTWAIT);
                fcntl(sock, F_SETFL, flags); // Restore flags
                if(n <= 0) {
                    pingFailures[nbr]++;
                    if(pingFailures[nbr] >= 3)
                        reportDeadNeighbor(nbr, sock);
                } else {
                    reply[n] = '\0';
                    string rpl(reply);
                    if(rpl.find("PONG") == 0)
                        pingFailures[nbr] = 0;
                }
            }
        }
    }

    // Reports a dead neighbor using the format:
    // Dead Node:<DeadNode.IP>:<DeadNode.Port>:<self.timestamp>:<self.IP>
    // Then notifies all seeds.
    void reportDeadNeighbor(const string &nbr, int sock) {
        string timestamp = getCurrentTimestamp();
        string deadMsg = "Dead Node:" + nbr + ":" + timestamp + ":" + myIP;
        cout << deadMsg << endl;
        outputFile << deadMsg << endl;
        for(auto &seed : chosenSeeds) {
            int sfd = socket(AF_INET, SOCK_STREAM, 0);
            if(sfd < 0) continue;
            struct sockaddr_in servAddr;
            memset(&servAddr, 0, sizeof(servAddr));
            servAddr.sin_family = AF_INET;
            servAddr.sin_port = htons(seed.second);
            if(inet_pton(AF_INET, seed.first.c_str(), &servAddr.sin_addr) <= 0) {
                close(sfd);
                continue;
            }
            if(connect(sfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
                close(sfd);
                continue;
            }
            size_t pos = nbr.find(":");
            string nbrIP = nbr.substr(0, pos);
            string nbrPort = nbr.substr(pos+1);
            string report = "DEAD " + nbrIP + " " + nbrPort + " " + timestamp + " " + myIP;
            send(sfd, report.c_str(), report.size(), 0);
            close(sfd);
        }
        close(sock);
        neighborSock.erase(nbr);
        connectedNeighbors.erase(nbr);
        pingFailures.erase(nbr);
    }

    // ------------------------------
    // Start Listener Thread
    // ------------------------------
    void startListener() {
        thread t(&PeerNode::listenForPeerConnections, this);
        t.detach();
    }
};
