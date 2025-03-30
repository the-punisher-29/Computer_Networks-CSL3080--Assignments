// network_builder.hpp
#pragma once
#include <random>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <memory>
#include <cmath>
#include <algorithm>

class Node {
public:
    enum class Type { SEED, PEER };
    
    Node(const std::string& id, Type type) 
        : id_(id), type_(type) {}
    
    void add_connection(std::shared_ptr<Node> node) {
        connections_.insert(node);
    }
    
    void remove_connection(std::shared_ptr<Node> node) {
        connections_.erase(node);
    }
    
    size_t degree() const { return connections_.size(); }
    const std::string& id() const { return id_; }
    Type type() const { return type_; }
    const std::unordered_set<std::shared_ptr<Node>>& connections() const { 
        return connections_; 
    }

private:
    std::string id_;
    Type type_;
    std::unordered_set<std::shared_ptr<Node>> connections_;
};

class NetworkBuilder {
public:
    NetworkBuilder(double alpha = 2.5, size_t min_connections = 2, 
                  size_t max_connections = 10)
        : alpha_(alpha), min_connections_(min_connections), 
          max_connections_(max_connections), random_engine_(random_device_()) {}
    
    // Add seed nodes to the network
    void add_seed_nodes(const std::vector<std::string>& seed_ids) {
        for (const auto& id : seed_ids) {
            auto seed = std::make_shared<Node>(id, Node::Type::SEED);
            nodes_[id] = seed;
            seed_nodes_.push_back(seed);
        }
    }
    
    // Add a new peer to the network using random walks and preferential attachment
    void add_peer(const std::string& peer_id) {
        auto peer = std::make_shared<Node>(peer_id, Node::Type::PEER);
        nodes_[peer_id] = peer;
        
        // Connect to required number of seed nodes
        connect_to_seeds(peer);
        
        // Use random walks for further connections
        while (peer->degree() < min_connections_ || 
               !follows_power_law_distribution()) {
            if (peer->degree() >= max_connections_) break;
            
            auto target = random_walk(peer);
            if (target && target != peer && 
                !are_connected(peer, target)) {
                connect_nodes(peer, target);
            }
        }
    }
    
    // Check if the network follows power-law distribution
    bool follows_power_law_distribution() const {
        if (nodes_.size() < 10) return true; // Too few nodes to verify
        
        std::vector<size_t> degrees;
        for (const auto& pair : nodes_) {
            degrees.push_back(pair.second->degree());
        }
        
        return verify_power_law(degrees);
    }
    
    // Get current network statistics
    std::unordered_map<size_t, size_t> get_degree_distribution() const {
        std::unordered_map<size_t, size_t> distribution;
        for (const auto& pair : nodes_) {
            distribution[pair.second->degree()]++;
        }
        return distribution;
    }

private:
    double alpha_;  // Power-law exponent
    size_t min_connections_;
    size_t max_connections_;
    std::random_device random_device_;
    std::mt19937 random_engine_;
    
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes_;
    std::vector<std::shared_ptr<Node>> seed_nodes_;
    
    // Connect a peer to required number of seed nodes
    void connect_to_seeds(std::shared_ptr<Node> peer) {
        size_t required_seeds = (seed_nodes_.size() / 2) + 1;
        std::vector<size_t> indices(seed_nodes_.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), random_engine_);
        
        for (size_t i = 0; i < required_seeds && i < seed_nodes_.size(); ++i) {
            connect_nodes(peer, seed_nodes_[indices[i]]);
        }
    }
    
    // Perform a random walk in the network
    std::shared_ptr<Node> random_walk(std::shared_ptr<Node> start) {
        const size_t max_steps = 10;
        std::shared_ptr<Node> current = start;
        
        for (size_t step = 0; step < max_steps; ++step) {
            // Choose next node with probability proportional to degree
            std::vector<std::shared_ptr<Node>> neighbors(
                current->connections().begin(), 
                current->connections().end());
            
            if (neighbors.empty()) break;
            
            std::vector<double> probabilities;
            double sum_degrees = 0;
            for (const auto& neighbor : neighbors) {
                sum_degrees += neighbor->degree();
            }
            
            for (const auto& neighbor : neighbors) {
                probabilities.push_back(
                    static_cast<double>(neighbor->degree()) / sum_degrees);
            }
            
            std::discrete_distribution<> dist(
                probabilities.begin(), probabilities.end());
            current = neighbors[dist(random_engine_)];
        }
        
        return current;
    }
    
    // Connect two nodes
    void connect_nodes(std::shared_ptr<Node> a, std::shared_ptr<Node> b) {
        if (a && b && a != b) {
            a->add_connection(b);
            b->add_connection(a);
        }
    }
    
    // Check if two nodes are connected
    bool are_connected(std::shared_ptr<Node> a, std::shared_ptr<Node> b) const {
        return a->connections().find(b) != a->connections().end();
    }
    
    // Verify power-law distribution using Kolmogorov-Smirnov test
    bool verify_power_law(const std::vector<size_t>& degrees) const {
        if (degrees.empty()) return false;
        
        // Calculate empirical distribution
        std::vector<double> empirical;
        for (size_t degree : degrees) {
            empirical.push_back(
                static_cast<double>(degree) / degrees.size());
        }
        std::sort(empirical.begin(), empirical.end());
        
        // Calculate theoretical distribution
        std::vector<double> theoretical;
        for (size_t i = 0; i < degrees.size(); ++i) {
            double x = static_cast<double>(i + 1) / degrees.size();
            theoretical.push_back(std::pow(x, -1.0 / alpha_));
        }
        
        // Calculate KS statistic
        double max_diff = 0.0;
        for (size_t i = 0; i < degrees.size(); ++i) {
            max_diff = std::max(max_diff, 
                std::abs(empirical[i] - theoretical[i]));
        }
        
        // Critical value for 95% confidence
        double critical_value = 1.36 / std::sqrt(degrees.size());
        return max_diff <= critical_value;
    }
};
