# Gossip Protocol P2P Network

This project implements a Gossip protocol over a peer-to-peer (P2P) network, focusing on efficient message broadcasting and monitoring the liveness (availability) of connected peers. The design follows a robust, scalable, and fault-tolerant approach by leveraging seed nodes, a power-law degree distribution for network formation, and regular liveness checks.

## Key Concepts

### Seed Nodes
- **Role:** Serve as bootstrap points holding information about other peers (IP addresses and ports).
- **Redundancy:** Multiple seed nodes are used for redundancy and reliability.
- **Configuration:** Their details are stored in the `config.txt` file.
- **Functionality in Code:**  
  - In [`seed.py`](seed.py), seed nodes create a server socket, listen for incoming connections from peers, and maintain a record of connected peers and their degree using functions like `create_server_socket()`, `bind_server_socket()`, and `process_peer_connection()`.
  - Functions such as `increase_peer_degree()`, `update_peer()`, and `handle_dead_node()` help manage and update peer states.

### Peers
- **Role:** Regular nodes that connect to other peers and participate in gossip-based message broadcasting.
- **Connection Strategy:** Each peer connects to a subset of other peers to form a connected network graph.
- **Degree Management:** Connections follow a power-law distribution where most peers have only a few connections, while a few have many.
- **Functionality in Code:**  
  - In [`peer.py`](peer.py), each peer:
    - Loads seed information using `load_seed_config()` and computes the count via `count_seeds()`.
    - Randomly selects a subset of seed nodes to register with via `register_with_seeds()`.
    - Establishes connections to other peers using functions like `connect_to_peers()`, `join_top_peers()`, and `merge_peer_lists()`.
    - Uses the `Peer` class to track each connected node.

### Liveness Monitoring
- **Regular Pings:** Peers send periodic "ping" (liveness) messages to their connected neighbors.
- **Timeout and Reporting:** If a peer fails to respond within a defined period, it is marked as dead and reported to seed nodes using the `announce_dead()` function.
- **Code Implementation:**  
  - The `monitor_liveness()` function in [`peer.py`](peer.py) iterates over active peers, sends liveness requests, and removes those that do not respond after multiple failures.

### Power-law Degree Distribution
- **Goal:** Ensure that the number of connections (degree) each peer has follows a power-law distribution.
- **Mechanism:**  
  - During network bootstrapping, peers select a random subset of potential connections.
  - The `join_top_peers()` function sorts peer entries by degree and connects to the top half, helping ensure that some peers naturally form hubs.
- **Advantage:** This distribution mimics real-world networks (e.g., social networks) and provides robustness and efficient message dissemination.

## Network Formation

1. **Seed Nodes Configuration:**  
   Details of seed nodes are listed in [`config.txt`](config.txt). Each peer must connect to at least ⌊n/2⌋+1 seed nodes, where _n_ is the total number of seeds.

2. **Bootstrapping Process:**  
   - A new peer first connects to the configured seed nodes.
   - It retrieves a list of other peers, using the response strings processed by `merge_peer_lists()`, and then selects a subset to establish direct connections using `connect_to_peers()` and `join_top_peers()`.

3. **Connected Graph:**  
   The overall network is guaranteed to be connected by ensuring that every peer can reach at least one other peer, facilitating the propagation of messages across the network.

## Message Broadcasting

- **Gossip Protocol:**  
  - Once connected, peers disseminate messages using the Gossip protocol.
  - A peer broadcasts its message to all connected neighbors using `broadcast_gossip()`.
  - On receiving a gossip message, the peer relays the message further via the `relay_gossip()` function after checking for duplicates (using computed message hashes via `compute_hash()`).

## Liveness Checking

- **Periodic Pings:**  
  The `monitor_liveness()` function periodically sends "Liveness Request" messages to all active peers.
  
- **Failure Handling:**  
  - If no response is received, the peer's failure count increments.
  - After three successive failures, the peer is deemed dead and reported back to the seed nodes using `announce_dead()`.
  
- **Seed Update:**  
  Seed nodes update their records of active peers based on these liveness messages, ensuring current network topology.

## Summary of Implemented Functionalities

### In [`peer.py`](peer.py)
- **Seed Configuration & Connection:**  
  - `load_seed_config()` reads seed node details from [`config.txt`](config.txt).  
  - `count_seeds()` computes the number of unique seed nodes.
  - `register_with_seeds()` selects and connects to a subset of seed nodes.
  
- **Connection Management:**  
  - `connect_to_peers()` establishes connections with other peers using a structured connection request.
  - `join_top_peers()` chooses peers with the highest degree for connection.
  - `merge_peer_lists()` processes received peer lists to update network information.

- **Gossip Broadcasting:**  
  - `broadcast_gossip()` periodically sends gossip messages.
  - `relay_gossip()` prevents duplicate messaging and forwards received gossip.

- **Liveness Monitoring:**  
  - `monitor_liveness()` pings all connected peers.
  - `announce_dead()` notifies seed nodes when a peer is unreachable.

- **Job Scheduling:**  
  - Worker threads are managed using `create_worker_threads()`, and jobs are queued with `enqueue_jobs()`.  
  - Each worker executes a task based on its job type in `job_executor()`.

### In [`seed.py`](seed.py)
- **Server Setup:**  
  - `create_server_socket()` and `bind_server_socket()` create and bind the socket.
  - `start_server()` listens for incoming connections from peers.
  
- **Peer Management:**  
  - The global `connected_peers` dictionary maintains peer addresses and degree.
  - `process_peer_connection()` handles messages from peers and updates degrees.
  - Functions such as `increase_peer_degree()`, `update_peer()`, and `peer_dict_to_string()` manage and communicate peer status.
  - `handle_dead_node()` processes notifications about dead peers and updates the network view.

## Running the Project

1. **For Seed Nodes:**
   - Run [`seed.py`](seed.py) and enter the desired port number when prompted.
   - The seed node will start listening and handling peer connections.

2. **For Peers:**
   - Run [`peer.py`](peer.py) and input a listening port when prompted.
   - The peer loads seed information from [`config.txt`](config.txt), registers with a subset of seeds, and starts the rounds of liveness checks and gossip broadcasts.

This design ensures that messages are efficiently disseminated throughout the network while continuously monitoring peer availability. The Gossip protocol, combined with seed node bootstrapping and power-law degree distribution, provides a solid framework for creating scalable and resilient P2P networks.