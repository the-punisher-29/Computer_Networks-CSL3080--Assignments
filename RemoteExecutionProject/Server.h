#ifndef SERVER_H
#define SERVER_H

#include <omnetpp.h>
#include <fstream>
#include <string>
using namespace omnetpp;
using namespace std;

class Server : public cSimpleModule {
  private:
    bool isMalicious;
    ofstream outFile;
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    void processSubtask(cMessage *msg);
    int computeResult();
};

#endif
