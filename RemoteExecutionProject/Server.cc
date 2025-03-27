#include "Server.h"
#include <sstream>

Define_Module(Server);

void Server::initialize() {
    // Decide randomly if this server will behave maliciously (approx. 25% chance)
    isMalicious = (intuniform(0, 3) == 0);
    string fileName = string("server_") + getName() + string("_output.txt");
    outFile.open(fileName.c_str());
    outFile << "Server " << getName() << " started at " << simTime() << "\n";
}

void Server::handleMessage(cMessage *msg) {
    if (strcmp(msg->getName(), "subtask") == 0) {
        processSubtask(msg);
    }
    delete msg;
}

void Server::processSubtask(cMessage *msg) {
    int subtaskId = msg->getKind();
    int result = computeResult();

    cMessage *response = new cMessage("subtaskResult");
    response->setKind(subtaskId);
    response->addPar("result") = result;
    response->addPar("serverID") = getName();

    outFile << "Processed subtask " << subtaskId << " with result: " << result
            << " at " << simTime() << "\n";
    EV << "Server " << getName() << " processed subtask " << subtaskId
       << " with result: " << result << "\n";

    send(response, "toClient");
}

int Server::computeResult() {
    // Simulate computing the maximum element in a subarray by generating a random number.
    int correctResult = intuniform(10, 100);
    if (isMalicious) {
        // Malicious behavior: return an incorrect result.
        int faulty = correctResult - intuniform(1, 5);
        return faulty;
    }
    else {
        return correctResult;
    }
}
