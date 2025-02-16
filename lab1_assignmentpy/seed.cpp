#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

using namespace std;

map<string, int> peer_list;
mutex peer_mutex;
int PORT;

void write_output_to_file(const string& output) {
    ofstream file("outputseed.txt", ios::app);
    if (!file) {
        cout << "Write Failed" << endl;
        return;
    }
    file << output << "\n";
    file.close();
}

string list_to_string() {
    string PeerList = "";
    for (auto& p : peer_list) {
        PeerList += p.first + ":" + to_string(p.second) + ";";
    }
    return PeerList;
}

void remove_dead_node(const string& message) {
    size_t pos1 = message.find(":");
    size_t pos2 = message.rfind(":");
    if (pos1 == string::npos || pos2 == string::npos || pos1 == pos2) return;
    
    string dead_node = message.substr(pos1 + 1, pos2 - pos1 - 1);
    string informer_node = message.substr(pos2 + 1);
    
    lock_guard<mutex> lock(peer_mutex);
    
    if (peer_list.count(dead_node)) {
        peer_list.erase(dead_node);
        cout << "Removed dead node: " << dead_node << endl;
    } else {
        cout << "Dead node " << dead_node << " not found in peer list" << endl;
    }
    
    if (peer_list.count(informer_node)) {
        peer_list[informer_node] = max(0, peer_list[informer_node] - 1);
        cout << "Reduced degree of informer node " << informer_node << " to " << peer_list[informer_node] << endl;
    } else {
        cout << "Informer node " << informer_node << " not found in peer list" << endl;
    }
}

void handle_peer(int conn) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(conn, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) break;
        
        string message(buffer);
        if (message.find("Dead Node") != string::npos) {
            remove_dead_node(message);
        } else {
            istringstream iss(message);
            string addr, port, degree;
            getline(iss, addr, ':');
            getline(iss, port, ':');
            getline(iss, degree, ':');
            
            string peer_address = addr + ":" + port;
            
            lock_guard<mutex> lock(peer_mutex);
            peer_list[peer_address] = stoi(degree);
            
            string output = "Received Connection from " + peer_address + " with initial degree " + degree;
            cout << output << endl;
            write_output_to_file(output);
            
            string PeerList = list_to_string();
            send(conn, PeerList.c_str(), PeerList.size(), 0);
        }
    }
    close(conn);
}

void start_server() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        cerr << "Socket Creation Error" << endl;
        return;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Socket Binding Error" << endl;
        return;
    }
    
    if (listen(server_fd, 5) < 0) {
        cerr << "Listening Error" << endl;
        return;
    }
    
    cout << "Seed is Listening" << endl;
    
    while (true) {
        int addrlen = sizeof(address);
        int new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) continue;
        thread(handle_peer, new_socket).detach();
    }
}

int main() {
    cout << "Enter Port Number: ";
    cin >> PORT;
    start_server();
    return 0;
}
