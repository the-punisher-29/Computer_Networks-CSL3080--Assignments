# Gossip Protocol P2P Network

This project implements a Gossip protocol over a peer-to-peer (P2P) network, focusing on efficient message broadcasting and monitoring the liveness (availability) of connected peers. The design follows a robust, scalable, and fault-tolerant approach by leveraging seed nodes, a power-law degree distribution for network formation, and regular liveness checks.

## Concepts used:

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

#### Detailed Example: 3 Seeds and 5 Peers

Consider a network with the following components:
- **Seeds:** Seed1, Seed2, Seed3 (stored in the configuration file)
- **Peers:** PeerA, PeerB, PeerC, PeerD, PeerE

**Bootstrapping Process:**
1. **Peer Registration:**  
   - Each peer randomly connects to at least ⌊3/2⌋+1 = 2 seed nodes and registers its details.
   - For instance, PeerA may register with Seed1 and Seed2; PeerB with Seed2 and Seed3; and so on.

2. **Peer Discovery:**  
   - Each peer retrieves a list of registered peers (with their connection degrees) from its connected seeds.
   - Suppose the initial degrees are low and relatively uniform.

3. **Forming Connections:**  
   - Using `merge_peer_lists()`, each peer aggregates peer lists from the seeds.
   - Then, the `join_top_peers()` function is called to select among the available peers:
     - PeerA might observe that PeerC and PeerD have slightly higher degrees (more connections).
     - Hence, PeerA establishes direct connections with these higher-degree peers to leverage their hub properties.
   - Repeating this process across the network increasingly reinforces a power-law distribution:
     - For example, PeerC might end up with 3 or 4 connections, acting as a hub, while several peers might have 1 or 2.

**Network Outcome:**
- A few peers (e.g., PeerC) develop into highly connected hubs.
- Other peers maintain a few connections, ensuring the network remains scalable.
- This topology speeds up the gossip message propagation because hubs quickly spread messages to many nodes.

## Network Formation

1. **Seed Nodes Configuration:**  
   Details of seed nodes are listed in [`config.txt`](config.txt). Each peer must connect to at least ⌊n/2⌋+1 seed nodes, where _n_ is the total number of seeds.

2. **Bootstrapping Process:**  
   - A new peer first connects to the configured seed nodes.
   - It retrieves a list of other peers, using the response strings processed by `merge_peer_lists()`, and then selects a subset to establish direct connections using `connect_to_peers()` and `join_top_peers()`.

3. **Connected Graph:**  
   The overall network is guaranteed to be connected by ensuring that every peer can reach at least one other peer, facilitating the propagation of messages across the network.

## Key Functionalities

### 1. Gossip Message Broadcasting

**Message Format:**  
`<timestamp>:<sender.IP>:<Msg#>`

**Process:**  
- Each peer generates a gossip message every 5 seconds (up to 10 messages).
- The generated message is sent to all directly connected peers.
- On receiving a new gossip message, a peer:
  - Checks its Message List (ML) to avoid duplicate processing.
  - Adds the new message (using a computed hash) to its ML.
  - Forwards the message to all its neighbors, except the sender.

*Key Functions in Code:*  
- `broadcast_gossip()` – Periodically sends out gossip messages.
- `relay_gossip()` – Relays received gossip messages to other peers, ensuring duplicates are ignored via hash comparison.

### 2. Liveness Checking

**Mechanism:**  
- Peers periodically send "ping" or "Liveness Request" messages (every 13 seconds) to their connected neighbors.
- Each ping helps verify that the neighbor is active.

**Failure Handling:**  
- If a peer does not respond to a ping for three consecutive cycles, it is considered dead.
- The peer reporting the failure notifies all connected seed nodes using the format:  
  `Dead Node:<DeadNode.IP>:<DeadNode.Port>:<Timestamp>:<Notifier.IP>`
- Seed nodes update their PL by removing the dead peer and adjusting degrees for the notifying peer if necessary.

*Key Functions in Code:*  
- `monitor_liveness()` – Checks all connected peers for liveness and initiates the failure reporting process.
- `announce_dead()` – Notifies seed nodes when a peer is detected to be unresponsive.

### 3. Power-law Degree Distribution

**Objective:**  
- Ensure that most peers have a few connections, while a few peers act as hubs with many connections.  
- This mimics real-world networks (e.g., social networks) and enhances both scalability and robustness.

**Mechanism:**  
- During bootstrapping, each peer selects a random subset of other peers.  
- The `join_top_peers()` function sorts peer entries based on their degree and connects to the top portion, thus naturally forming hub nodes.

*Key Functions in Code:*  
- `join_top_peers()` – Helps maintaining the power-law distribution by selecting peers with higher degrees.
- `connect_to_peers()` – Establishes TCP connections with selected peers.

### 4. Job Scheduling

**Functionality:**  
- Worker threads are responsible for tasks such as handling incoming connections, liveness checks, and message broadcasting.
- The job queue (`job_queue`) schedules tasks to be executed by workers.

*Key Functions in Code:*  
- `create_worker_threads()` – Initializes worker threads.
- `job_executor()` – Executes jobs from the queue.
- `enqueue_jobs()` – Queues tasks for processing.

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
   - Run [`seed.py`](seed.py) and enter one of the port number as per config file when prompted.
   - The seed node will start listening and handling peer connections.

2. **For Peers:**
   - Run [`peer.py`](peer.py) and input a listening port(don't put the ports used by seeds) when prompted.
   - The peer loads seed information from [`config.txt`](config.txt), registers with a subset of seeds ensuring power-law, and starts the rounds of liveness checks and gossip broadcasts.

This design ensures that messages are efficiently disseminated throughout the network while continuously monitoring peer availability. The Gossip protocol, combined with seed node bootstrapping and power-law degree distribution, provides a solid framework for creating scalable and resilient P2P networks.

## Future Improvements

- **Scalability Enhancements:** Optimizing the network to handle larger numbers of peers while maintaining efficient communication and message dissemination.
- **Security Management:** Implementing encryption for messages and authentication mechanisms to prevent malicious nodes from disrupting the network.
- **Fault Tolerance:** Enhancing failure recovery strategies to better handle peer departures and network partitions.

## References

- [Gossip Protocol Implementation by EthanCornell](https://github.com/EthanCornell/Gossip-protocol)
- [Creating a P2P Network from Scratch](https://dev.to/lxchurbakov/create-a-p2p-network-with-node-from-scratch-1pah)



