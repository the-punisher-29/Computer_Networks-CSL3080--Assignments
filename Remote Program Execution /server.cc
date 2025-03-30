
#include <omnetpp.h>
#include <vector>
#include <sstream>
#include <random>
#include <stdio.h>
#include <fstream>
#include <map>

using namespace omnetpp;

class server : public cSimpleModule {
    int n,c;
    std::ofstream outputFile;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(server);

void server::initialize() {

    outputFile.open("output.txt", std::ios_base::app);
    n = 6; c = 4;

}

void server::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        // Handle self messages if any
    } else {
        std::istringstream iss(msg->getFullName());
        int num;
        int max = 1;
        std::vector<int> receivedList;
        int taskID; iss>>taskID;
        while (iss >> num) {
            receivedList.push_back(num);
            if(max<num) max = num;
        }


        if(getId()<2+n/4){
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 1);
            int toss = dis(gen);
            if(toss) max -= 5;
        }

        cGate *arrivalGate = msg->getArrivalGate();
        char* opgate;


         if (arrivalGate == gate("in0")) opgate = "out0"; else if (arrivalGate == gate("in1")) opgate = "out1"; else if (arrivalGate == gate("in2")) opgate = "out2"; else if (arrivalGate == gate("in3")) opgate = "out3"; 

        std::ostringstream oss; oss<<max;
        cMessage *rep = new cMessage(oss.str().c_str());
        send(rep, opgate);

        std::cout  <<getName()<<" - task ID "<<taskID<<" : computed maxima = "<<max<<std::endl;;
        outputFile <<getName()<<" - task ID "<<taskID<<" : computed maxima = "<<max<<std::endl;;



    }
    delete msg;
}


