// seed.cpp
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstring>
#include <cstdlib>
using namespace std;

class SeedServer {
public:
    string seedID; // e.g., "127.0.0.1:6000"
    int port;
    int server_fd;
    unordered_map<string, string> peerList; // Maps peer "IP:Port" -> "Alive"
    mutex mtx;
    ofstream outputFile;

    SeedServer(const string &id, int port) : seedID(id), port(port) {
        outputFile.open("outputfile.txt", ios::app);
        if (!outputFile.is_open()) {
            cerr << "Error opening outputfile.txt" << endl;
        }
        // Create socket in seed.cpp
server_fd = socket(AF_INET, SOCK_STREAM, 0);
if (server_fd == 0) {
    perror("Socket failed");
    exit(EXIT_FAILURE);
}
int opt = 1;
if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
}

        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr("127.0.0.1");
        address.sin_port = htons(port);
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("Bind failed");
            exit(EXIT_FAILURE);
        }
        if (listen(server_fd, 10) < 0) {
            perror("Listen");
            exit(EXIT_FAILURE);
        }
        cout << "SeedServer " << seedID << " listening on port " << port << endl;
        outputFile << "SeedServer " << seedID << " listening on port " << port << endl;
    }

    void addPeer(const string &ip, const string &peerPort) {
        lock_guard<mutex> lock(mtx);
        string key = ip + ":" + peerPort;
        peerList[key] = "Alive";
        string logMsg = "Seed " + seedID + " - Peer registered: " + key;
        cout << logMsg << endl;
        outputFile << logMsg << endl;
    }

    void removePeer(const string &ip, const string &peerPort) {
        lock_guard<mutex> lock(mtx);
        string key = ip + ":" + peerPort;
        if (peerList.erase(key)) {
            string logMsg = "Seed " + seedID + " - Dead peer removed: " + key;
            cout << logMsg << endl;
            outputFile << logMsg << endl;
        }
    }

    // Returns a comma-delimited list of current peers.
    string getPeerList() {
        lock_guard<mutex> lock(mtx);
        ostringstream oss;
        for (auto &entry : peerList)
            oss << entry.first << ",";
        string list = oss.str();
        if (!list.empty()) list.pop_back();
        return list;
    }

    // Handles an individual peer connection.
    void handleClient(int client_sock) {
        char buffer[1024] = {0};
        int valread;
        while ((valread = read(client_sock, buffer, 1024)) > 0) {
            buffer[valread] = '\0';
            string request(buffer);
            istringstream iss(request);
            string cmd;
            iss >> cmd;
            if (cmd == "REGISTER") {
                string ip, port;
                iss >> ip >> port;
                addPeer(ip, port);
            } else if (cmd == "GET_PEERS") {
                string list = getPeerList();
                send(client_sock, list.c_str(), list.size(), 0);
            } else if (cmd == "DEAD") {
                // Format: DEAD <DeadNode.IP> <DeadNode.Port> <timestamp> <reporter.IP>
                string ip, port, timestamp, reporter;
                iss >> ip >> port >> timestamp >> reporter;
                removePeer(ip, port);
            }
            memset(buffer, 0, 1024);
        }
        close(client_sock);
    }

    void run() {
        while (true) {
            struct sockaddr_in clientAddr;
            socklen_t addrlen = sizeof(clientAddr);
            int client_sock = accept(server_fd, (struct sockaddr *)&clientAddr, &addrlen);
            if (client_sock < 0) {
                perror("Accept failed");
                continue;
            }
            thread t(&SeedServer::handleClient, this, client_sock);
            t.detach();
        }
    }
};
