#ifndef CLIENT_H
#define CLIENT_H

#include <omnetpp.h>
#include <fstream>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>

using namespace omnetpp;
using namespace std;

class Client : public cSimpleModule {
  private:
    int numSubtasks; // number of subtasks per task
    int currentTaskID;
    // For each subtask, store the results received from servers
    vector<vector<int>> subtaskReplies;
    // Server scores (server id -> score)
    map<string, int> serverScores;
    // Log of received gossip messages (to prevent duplicate forwarding)
    set<string> gossipLog;
    // Output file stream for logging
    ofstream outFile;
    int repliesReceived;
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    void distributeTasks();
    void processSubtaskResult(cMessage *msg);
    void completeTask();
    void sendGossipMessage();
    void handleGossipMessage(cMessage *msg);
    int majorityResult(const vector<int>& results);
    int computeLocalTaskResult();
    string getClientID();
};

#endif
