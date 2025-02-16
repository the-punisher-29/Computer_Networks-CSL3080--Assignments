// main.cpp
#include "pl.hpp"
#include <iostream>

int main() {
    NetworkBuilder builder(2.5, 2, 10);
    
    // Add seed nodes
    std::vector<std::string> seed_ids = {"Seed1", "Seed2", "Seed3"};
    builder.add_seed_nodes(seed_ids);
    
    // Add peers incrementally
    std::vector<std::string> peer_ids = {
        "PeerA", "PeerB", "PeerC", "PeerD", "PeerE"
    };
    
    for (const auto& peer_id : peer_ids) {
        std::cout << "Adding " << peer_id << "...\n";
        builder.add_peer(peer_id);
        
        // Print current degree distribution
        auto distribution = builder.get_degree_distribution();
        std::cout << "Current degree distribution:\n";
        for (const auto& pair : distribution) {
            std::cout << "Degree " << pair.first << ": " 
                     << pair.second << " nodes\n";
        }
        std::cout << "Follows power-law: " 
                  << (builder.follows_power_law_distribution() ? "Yes" : "No") 
                  << "\n\n";
    }
    
    return 0;
}