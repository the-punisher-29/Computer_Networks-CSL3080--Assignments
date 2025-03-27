#include "Client.h"
#include <sstream>

Define_Module(Client);

void Client::initialize() {
    numSubtasks = par("numSubtasks").intValue();
    if (numSubtasks <= 0)
        numSubtasks = gateSize("toServer");
    currentTaskID = 0;
    repliesReceived = 0;
    subtaskReplies.resize(numSubtasks);

    // Open output file for logging
    string fileName = string("client_") + getClientID() + string("_output.txt");
    outFile.open(fileName.c_str());
    outFile << "Client " << getClientID() << " started at " << simTime() << "\n";

    // Schedule the start of the first task
    scheduleAt(simTime() + 1.0, new cMessage("startTask"));
}

void Client::handleMessage(cMessage *msg) {
    string msgName = msg->getName();
    if (msgName == "startTask") {
        // New task: reset counters and data structures
        currentTaskID++;
        repliesReceived = 0;
        for (auto &vec : subtaskReplies)
            vec.clear();
        outFile << "Starting Task " << currentTaskID << " at " << simTime() << "\n";
        distributeTasks();
    }
    else if (msgName == "subtaskResult") {
        processSubtaskResult(msg);
    }
    else if (msgName == "gossipMessage") {
        handleGossipMessage(msg);
    }
    delete msg;
}

void Client::distributeTasks() {
    // For each subtask, send to (n/2 + 1) randomly selected servers.
    int n = gateSize("toServer");
    int serversPerSubtask = n / 2 + 1;
    for (int i = 0; i < numSubtasks; i++) {
        for (int j = 0; j < serversPerSubtask; j++) {
            cMessage *subMsg = new cMessage("subtask");
            subMsg->setKind(i); // use kind field to denote subtask ID
            int gateIndex = intuniform(0, n - 1);
            send(subMsg->dup(), "toServer", gateIndex);
        }
    }
}

void Client::processSubtaskResult(cMessage *msg) {
    int subtaskId = msg->getKind();
    int result = msg->par("result").longValue();
    string serverID = msg->par("serverID").stringValue();


    subtaskReplies[subtaskId].push_back(result);
    // Increment server score if result is considered correct (>= 0)
    if (result >= 0)
        serverScores[serverID]++;

    outFile << "Received subtask result for subtask " << subtaskId
            << " from server " << serverID << " result: " << result
            << " at " << simTime() << "\n";
    EV << "Client " << getClientID() << " received result for subtask "
       << subtaskId << " from server " << serverID << ": " << result << "\n";

    repliesReceived++;
    // Check if each subtask has at least one reply
    bool allSubtasksReceived = true;
    for (auto &vec : subtaskReplies) {
        if (vec.empty()) { allSubtasksReceived = false; break; }
    }
    if (allSubtasksReceived)
        completeTask();
}

int Client::majorityResult(const vector<int>& results) {
    // For this example, we simply choose the maximum value among the received results.
    if (results.empty())
        return -1;
    int maxVal = results[0];
    for (int val : results)
        if (val > maxVal)
            maxVal = val;
    return maxVal;
}

int Client::computeLocalTaskResult() {
    // The final result is the maximum of the majority results for each subtask.
    int finalRes = -1;
    for (int i = 0; i < numSubtasks; i++) {
        int maj = majorityResult(subtaskReplies[i]);
        if (maj > finalRes)
            finalRes = maj;
    }
    return finalRes;
}

void Client::completeTask() {
    vector<int> majorityResults;
    for (int i = 0; i < numSubtasks; i++) {
        int maj = majorityResult(subtaskReplies[i]);
        majorityResults.push_back(maj);
    }
    int finalResult = computeLocalTaskResult();
    outFile << "Final consolidated result for task " << currentTaskID
            << ": " << finalResult << " at " << simTime() << "\n";
    EV << "Client " << getClientID() << " final result for task " << currentTaskID
       << ": " << finalResult << "\n";

    outFile << "Server Scores for task " << currentTaskID << ":\n";
    for (auto &entry : serverScores) {
        outFile << "Server " << entry.first << " Score: " << entry.second << "\n";
        EV << "Server " << entry.first << " Score: " << entry.second << "\n";
    }
    // After completing the task, broadcast a gossip message.
    sendGossipMessage();

    // For this example, schedule a second task.
    if (currentTaskID < 2)
        scheduleAt(simTime() + 5.0, new cMessage("startTask"));
}

void Client::sendGossipMessage() {
    // Create a gossip message in the format: <timestamp>:<self.IP>:<Score#>
    stringstream ss;
    ss << simTime() << ":" << getClientID() << ":" << serverScores.size();
    string gossipContent = ss.str();

    cMessage *gossipMsg = new cMessage("gossipMessage");
    gossipMsg->addPar("content") = gossipContent.c_str();

    outFile << "Sending gossip message: " << gossipContent << " at " << simTime() << "\n";
    EV << "Client " << getClientID() << " sending gossip: " << gossipContent << "\n";

    send(gossipMsg->dup(), "toClient");
}

void Client::handleGossipMessage(cMessage *msg) {
    string content = msg->par("content").stringValue();
    if (gossipLog.find(content) == gossipLog.end()) {
        gossipLog.insert(content);
        string sender = msg->getSenderModule()->getFullPath();
        stringstream ss;
        ss << "Received gossip: " << content << " from " << sender << " at " << simTime();
        outFile << ss.str() << "\n";
        EV << ss.str() << "\n";
        // Forward to all other client modules.
        send(msg->dup(), "toClient");
    }
}

string Client::getClientID() {
    return getName();
}
