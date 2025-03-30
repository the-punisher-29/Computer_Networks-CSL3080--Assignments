# **Remote Program Execution Simulation using OMNeT++**  

## **Introduction**  

This project simulates a **remote program execution system** using **OMNeT++**, a discrete event simulator. The system involves multiple **clients and servers**, where clients distribute computational tasks among multiple servers and aggregate the results using **majority voting** to determine the correct outcome.  

The primary task simulated in this project is **finding the maximum element in an array**. The client splits the array into smaller **subtasks**, sends them to multiple servers, and collects the results. The final result is determined based on the majority response.  

Additionally, we incorporate a **Gossip Protocol**, where clients exchange server performance scores to improve task distribution in subsequent rounds. This ensures that **malicious servers** (which provide incorrect results) are identified and assigned fewer tasks over time.  

---

## **Network Components & Structure**  

The network consists of two types of nodes:  

### **1. Server Nodes**  
- Each **server node** executes a task assigned by a client.  
- Tasks involve finding the maximum value in a given sub-array.  
- Some servers may act **honestly** (returning correct results), while others may act **maliciously** (returning incorrect results).  
- **At most `n/4` servers** in the network can be malicious (`f < n/4`).  
- Each server computes and returns results for the assigned subtask.  

### **2. Client Nodes**  
- **Each client manages task execution** by breaking down the array into `n` parts and distributing them.  
- It sends each subtask to **`n/2 + 1` servers** (randomly selected).  
- The client collects results and determines the **majority value** as the correct answer.  
- Each client assigns a **score** to servers based on their correctness:  
  - Honest result → **Score = 1**  
  - Malicious result → **Score = 0**  
- After completing a task, clients share **server scores with other clients** using a **Gossip Protocol**.  

---

## **Task Execution Process**  

### **Step 1: Network Initialization**  
- The network is **dynamically generated** using OMNeT++.  
- The **topology is stored in `topo.txt`**, which we can modify before running the simulation.  

### **Step 2: Task Assignment (Round 1)**  
1. **Client splits the array** into `n` sub-arrays (each having at least 2 elements).  
2. Each **sub-array (subtask) is sent to at least `n/2 + 1` different servers**.  
3. Servers **process the subtask** and compute the maximum value.  
4. Each **server returns its computed maximum** to the client.  

### **Step 3: Result Aggregation**  
1. The client **collects results from multiple servers** for each subtask.  
2. It determines the **majority response** and finalizes the subtask result.  
3. The final task result is determined by taking the **maximum of all subtasks' maxima**.  

### **Step 4: Server Scoring**  
- The client assigns scores to each server based on correctness.  
- These scores are **recorded and stored locally** for future reference.  

### **Step 5: Gossip Protocol & Score Sharing**  
- Clients **broadcast their scores** to other clients using a **Gossip Protocol**.  
- Each message follows this format:  
  ```
  <timestamp>:<client-IP>:<server-score>
  ```  
- Each client keeps a **Message Log (ML)** to prevent duplicate messages and looping.  
- Clients **update their server scores based on received messages**.  

### **Step 6: Optimized Task Assignment (Round 2)**  
- In the second round, each client **selects the top `n/2 + 1` servers with the highest scores**.  
- The process repeats with the **optimized set of servers**, improving reliability.  

---

## **How to Run the Simulation**  

### **Step 1: Setup**  
1. Ensure that **OMNeT++** is installed and configured correctly.  
2. Place the **topology file (`topo.txt`)** in the project directory.  
3. Verify that all **NED (`.ned`), C++ (`.cc`), and header (`.h`) files** are present.  

### **Step 2: Compile the Code**  
Run the following command to build the simulation:  
```
make
```

### **Step 3: Start the Simulation**  
Execute the program with:  
```
./run
```

### **Step 4: View the Output**  
- The **console** displays the computed maxima for each subtask.  
- Detailed logs are stored in **`output.txt`**.  
- This file contains:
  - **Subtask results from each server**  
  - **Final computed result for each client**  
  - **Gossip messages exchanged between clients**  
  - **Server scores after Round 1**  

---

## **Understanding the Output (`output.txt`)**  

### **Sample Output**  
```
server3 - task ID 0 : computed maxima = 50  
server1 - task ID 1 : computed maxima = 37  
server4 - task ID 2 : computed maxima = 49  
server3 - task ID 3 : computed maxima = 38  
server1 - task ID 4 : computed maxima = 32  
server4 - task ID 0 : computed maxima = 39  
server3 - task ID 1 : computed maxima = 41  
server1 - task ID 2 : computed maxima = 41  
server4 - task ID 3 : computed maxima = 44  
server3 - task ID 4 : computed maxima = 43  
```

### **Detailed Explanation**  
Each line follows this format:  
```
<server_name> - task ID <task_number> : computed maxima = <max_value>
```
- **Example 1**:  
  ```
  server3 - task ID 0 : computed maxima = 50  
  server4 - task ID 0 : computed maxima = 39  
  ```
  - Task **ID 0** was sent to **server3 and server4**.  
  - Server3 computed **50**, while Server4 computed **39**.  
  - The client will use **majority voting** to finalize the maximum.  

- **Example 2**:  
  ```
  server1 - task ID 1 : computed maxima = 37  
  server3 - task ID 1 : computed maxima = 41  
  ```
  - Task **ID 1** was sent to **server1 and server3**.  
  - The majority value will be determined as the final result.  

- **Final Computation**  
  - The final result for the **entire array** is determined by taking the **maximum of all aggregated maxima**.  

### **Gossip Protocol Messages**  
Clients exchange messages like:  
```
2025-03-30 10:15:23: client1: server3_score=1  
2025-03-30 10:15:25: client2: server4_score=0  
```
- **Client1 recorded that Server3 returned correct results** (score = 1).  
- **Client2 found Server4 to be malicious** (score = 0).  
- These messages help update **server scores** for optimized task allocation in the next round.  

---

## **Modifying the Network Topology (`topo.txt`)**  

The `topo.txt` file defines the **network structure**. Example:  
```
clients: client1 client2  
servers: server1 server2 server3 server4  
connections:  
client1 -> server1, server2, server3  
client2 -> server2, server3, server4  
client1 <-> client2  
```
- **Clients and servers are connected as defined.**  
- Clients **share server scores** via the **Gossip Protocol**.  

---

## **Conclusion**  

This project demonstrates how **distributed computing** can be improved using **redundancy, fault tolerance, and reputation-based task allocation**. By dynamically selecting **high-performing servers** and filtering out **malicious ones**, we enhance the **accuracy and reliability** of remote task execution.  

For further insights, check the **output logs in `output.txt`**. 🚀
