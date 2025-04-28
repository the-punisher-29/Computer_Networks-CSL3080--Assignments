# P2PChord: Peer-to-Peer Task Execution Simulator in OMNeT++

**Last updated:** April 2025

## Overview

P2PChord is an OMNeT++ simulation that demonstrates how a set of peers can collaboratively execute a distributed task—in this case, finding the maximum element in an array. Each node:

1. Splits an array of size `2·x` into `x` equal subtasks (each ≥ 2 elements)
2. Routes each subtask to node `(subtaskID % N)` in _O(log N)_ hops via a hybrid ring + Chord®‐style overlay
3. Computes its local maximum and returns the partial result along the same efficient path
4. Once all subtasks are aggregated, floods a lightweight gossip message to signal global completion

This project illustrates:
- Dynamic topology construction in OMNeT++ (ring + finger links)
- Custom C++ message‐routing logic
- A divide‐and‐conquer workload
- A simple gossip protocol for termination detection

## Project Structure

```
P2PChord/
├── src/
│   ├── PeerClient.ned   # Definition of the PeerClient simple module
│   ├── P2PChordNetwork.ned  # Network: ring + Chord fingers
│   └── PeerClient.cc    # C++ logic: task division, routing, gossip
├── omnetpp.ini         # Simulation configuration
└── README.md           # This file
```

## Prerequisites

- **OMNeT++ 6.0+** (Tkenv or Cmdenv)
- C++17‐capable compiler (GCC, Clang)
- (Optional) INET framework for realistic channel models

## Build & Run

1. **Open** the `P2PChord` folder as an OMNeT++ project in your IDE (Eclipse)
2. **Build** the project (Project → Build All)
3. **Configure** your run in `omnetpp.ini`:
   - Choose `[Config Basic]` or `[Config Large]`
   - Adjust `**.numClients` and `**.numSubtasks` as desired
4. **Execute** under Tkenv to watch the animation or Cmdenv for batch mode:

```
./out/clang-debug/bin/P2PChord -u Cmdenv -c Basic
```

## Key Configuration Parameters

| Parameter            | Description                              | Default (Basic) |
|----------------------|------------------------------------------|-----------------|
| `**.numClients`      | Number of peer nodes (N)                 | 8               |
| `**.numSubtasks`     | Number of subtasks per task (x)          | 4               |
| `sim-time-limit`     | Maximum simulation time                  | 100 s           |
| `seed-set`           | RNG seed set                             | 0               |

Feel free to add new `[Config …]` sections in `omnetpp.ini` for parameter sweeps.

## How It Works

### 1. Topology Setup
- The `.ned` file links each `client[i]` to `client[(i+1)%N]` in a ring
- It also adds "finger" links at distances 2^j (j = 0..⌈log₂ N⌉–1) for fast lookup

### 2. PeerClient Module
- On initialization, each node schedules a `START_TASK` message
- When delivered, it splits the array into `SUBTASK` messages and routes them to the appropriate owners
- Recipients compute a local maximum and send back a `SUBRESULT`
- The originator aggregates all partial maxima, computes the global max, then floods a `GOSSIP` message containing its timestamp and ID
- Each node logs the first receipt of a given gossip and rebroadcasts exactly once

### 3. Routing Logic (`routeMessage()`)
- Measures circular distance to the destination
- Picks the finger that reduces the distance most; otherwise steps to the next ring neighbor
- Achieves _O(log N)_ average‐case hops per message

## Output & Logging

**Console (EV)**
- Subtask dispatch and receipt
- Subresult receipt and final maximum
- Gossip send/receive events

**Per-node log files** (`client_<ID>.log`)
- Timestamps for every significant event
- Useful for post-simulation analysis and verification

## Extending the Simulation

- Swap in real payloads or serialized data structures
- Use INET's channel models (Ethernet, wireless) for realism
- Simulate node churn, failures, or adversarial behavior
- Collect metrics: end-to-end latency, message counts, load balancing
- Visualize in Tkenv or export traces for external analysis

## References

- OMNeT++ Manual, Section 4.13: Dynamic module creation
- "Chord: A Scalable Peer-to-Peer Lookup Service for Internet Applications" (Stoica et al.)
- OMNeT++ API: `cSimpleModule`, `cMessage`, `cMsgPar`

Enjoy experimenting with distributed task execution and overlay routing in OMNeT++! Pull requests and issues are welcome.
