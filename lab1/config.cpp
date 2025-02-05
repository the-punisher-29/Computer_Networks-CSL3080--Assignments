#include "config.h"
#include <fstream>
#include <sstream>
Config Config::loadFromFile(const std::string& filename) {
    Config config;
    std::ifstream file(filename);
    std::string line;
    
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string ip;
        int port;
        
        if (std::getline(iss, ip, ':') && iss >> port) {
            config.seeds.push_back({ip, port});
        }
    }
    
    return config;
}