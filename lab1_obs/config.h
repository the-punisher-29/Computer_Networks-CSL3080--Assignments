#pragma once
#include <string>
#include <vector>
using namespace std;
struct NodeInfo {
    string ip;
    int port;
};
struct Config {
    vector<NodeInfo> seeds;
    static Config loadFromFile(const std::string& filename);
};