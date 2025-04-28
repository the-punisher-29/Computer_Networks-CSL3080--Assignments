#include <omnetpp.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <algorithm>
#include <climits>

using namespace omnetpp;

enum MessageType { START_TASK = 100, SUBTASK, SUBRESULT, GOSSIP };

class PeerClient : public cSimpleModule
{
  private:
    int clientId, numClients, numSubtasks;
    int subtasksSent = 0, resultsReceived = 0;
    std::vector<int> partialResults;
    std::ofstream logFile;
    std::set<std::string> receivedGossips;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    void startTask();
    void divideTask();
    void handleSubtask(cMessage *msg);
    void handleSubresult(cMessage *msg);
    void sendGossip();
    void handleGossip(cMessage *msg);
    int calculateMax(int start, int length);
    void routeMessage(cMessage *msg, int destination);
};

Define_Module(PeerClient);

void PeerClient::initialize()
{
    clientId = par("clientId");
    numClients = par("numClients");
    numSubtasks = par("numSubtasks");
    partialResults.assign(numSubtasks, INT_MIN);

    logFile.open("client_" + std::to_string(clientId) + ".log");
    scheduleAt(simTime() + exponential(2), new cMessage("startTask", START_TASK));
}

void PeerClient::handleMessage(cMessage *msg)
{
    switch (msg->getKind()) {
        case START_TASK: startTask(); break;
        case SUBTASK: handleSubtask(msg); break;
        case SUBRESULT: handleSubresult(msg); break;
        case GOSSIP: handleGossip(msg); break;
        default: EV_ERROR << "Unknown msg type." << endl; break;
    }
    delete msg;
}

void PeerClient::startTask()
{
    EV_INFO << "Client " << clientId << ": Starting task." << endl;
    logFile << simTime() << ": Starting task." << std::endl;
    divideTask();
}

void PeerClient::divideTask()
{
    for (int i = 0; i < numSubtasks; ++i) {
        auto *msg = new cMessage("subtask", SUBTASK);
        msg->addPar("taskId").setIntValue(i);
        msg->addPar("startIdx").setIntValue(i * 2);
        msg->addPar("length").setIntValue(2);
        msg->addPar("originId").setIntValue(clientId);

        int target = i % numClients;
        EV_INFO << "Sending subtask " << i << " to client " << target << endl;
        logFile << simTime() << ": Subtask " << i << " -> Client " << target << std::endl;
        routeMessage(msg, target);
        subtasksSent++;
    }
}

void PeerClient::handleSubtask(cMessage *msg)
{
    int start = msg->par("startIdx"), len = msg->par("length");
    int maxVal = calculateMax(start, len);

    auto *res = new cMessage("subresult", SUBRESULT);
    res->addPar("taskId").setIntValue(msg->par("taskId"));
    res->addPar("result").setIntValue(maxVal);
    res->addPar("originId").setIntValue(msg->par("originId"));

    EV_INFO << "Processed subtask " << msg->par("taskId") << ", result: " << maxVal << endl;
    routeMessage(res, msg->par("originId"));
}

void PeerClient::handleSubresult(cMessage *msg)
{
    int taskId = msg->par("taskId");
    partialResults[taskId] = msg->par("result");
    resultsReceived++;

    if (resultsReceived == numSubtasks) {
        int finalMax = *std::max_element(partialResults.begin(), partialResults.end());
        EV_INFO << "All results collected. Final max: " << finalMax << endl;
        logFile << simTime() << ": Final max: " << finalMax << std::endl;
        sendGossip();
    }
}

void PeerClient::sendGossip()
{
    std::string gossip = std::to_string(simTime().dbl()) + ":" + getFullPath();
    if (!receivedGossips.insert(gossip).second) return;

    auto *msg = new cMessage("gossip", GOSSIP);
    msg->addPar("gossip").setStringValue(gossip.c_str());

    send(msg->dup(), "outRing");
    send(msg, "outRing");
}

void PeerClient::handleGossip(cMessage *msg)
{
    std::string gossip = msg->par("gossip");
    if (receivedGossips.insert(gossip).second) {
        send(msg->dup(), "outRing");
        for (int i = 0; i < numClients; ++i) send(msg->dup(), "outFinger", i);
    }
    delete msg;
}

int PeerClient::calculateMax(int start, int length)
{
    int maxVal = INT_MIN;
    for (int i = start; i < start + length; ++i)
        maxVal = std::max(maxVal, i);
    return maxVal;
}

void PeerClient::routeMessage(cMessage *msg, int destination)
{
    if (destination == clientId) {
        handleMessage(msg);
        return;
    }

    int best = -1, minDist = numClients + 1;
    for (int i = 0; i < gateSize("outFinger"); ++i) {
        int id = (clientId + (1 << i)) % numClients;
        int dist = (destination - id + numClients) % numClients;
        if (dist < minDist) {
            minDist = dist;
            best = i;
        }
    }

    if (best != -1 && minDist < (destination - clientId + numClients) % numClients)
        send(msg, "outFinger", best);
    else
        send(msg, "outRing");
}
