
#include <vector>
#include <sstream>
#include <random>
#include <stdio.h>
#include <omnetpp.h>
#include <queue>
#include <chrono>
#include <fstream>

using namespace omnetpp;

class client : public cSimpleModule {
    int state,n,k,x,c,surveySize;
    std::ofstream outputFile;

    std::vector<int> integerList;
    std::vector<int> maximaList;
    std::vector<int> serverList;

    std::vector<double> totalRequests;
    std::vector<int> correctReplies;
    std::vector<double> selfScoring;

    std::queue<int> actualMaxima;
    std::vector<double> avgScore;

    int scoresReceived;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual int isClientGate(cGate* g){
        for(int i=1;i<c;i++){
            char testGate[4];
            std::sprintf(testGate,"cin%d",i);
            if(g==gate(testGate)) return 1;
        }
        return 0;


    }
};

Define_Module(client);



int trueMax(std::vector<int> vec){
    int max = 1;
    for(int y : vec) if(max<y) max = y;
    return max;
}

std::vector<int> generateRandomVector(int size, int min, int max) {
    std::vector<int> randomVector;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);

    for (int i = 0; i < size; ++i)
        randomVector.push_back(dis(gen));
    return randomVector;
}

std::vector<int> NonRepeatingRandomArray(int size, int min, int max) {
    std::vector<int> randomArray;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    std::set<int> uniqueNumbers;

    while (uniqueNumbers.size() < size) {
        int randomNumber = dis(gen);
        if (uniqueNumbers.find(randomNumber) == uniqueNumbers.end()) {
            uniqueNumbers.insert(randomNumber);
            randomArray.push_back(randomNumber);
        }
    }
    return randomArray;
}

void client::initialize() {
    state = 0;
    n = 6; x = 4*n;
    k = x/n;            // 4
    surveySize = n/2+1; // 6
    scoresReceived = 0;
    c = 4;

    outputFile.open("output.txt", std::ios_base::app);

    scheduleAt(simTime() + 1.0, new cMessage("SelfMessage"));

}

void client::handleMessage(cMessage *msg){

    if(msg->isSelfMessage()){
//        state = 0;
//        n = 10; x = 40;
//        k = x/n;            // 4
//        surveySize = n/2+1; // 6
//        scoresReceived = 0;

        integerList = generateRandomVector(40, 1, 50);
        serverList = NonRepeatingRandomArray(surveySize,0,n-1);

        for(int i=0;i<n;i++) correctReplies.push_back(0);
        for(int i=0;i<n;i++) avgScore.push_back(0.0);
        for(int i=0;i<n;i++) selfScoring.push_back(0.0);
        for(int i=0;i<n;i++) totalRequests.push_back(0.0);

        for(int taskID=0;taskID<n;taskID++){
            auto start = integerList.begin()+k*taskID;
            auto end = integerList.begin()+k*(taskID+1);
            std::vector<int> subarray(start,end);

            actualMaxima.push(trueMax(subarray));

            int serverID = serverList[taskID%surveySize];
            totalRequests[serverID] +=1.0;


            std::ostringstream oss;
            oss << taskID << ' ';
            for (int i : subarray) oss << i << " ";
            cMessage *message = new cMessage(oss.str().c_str());

            char opgate[4];
            std::sprintf(opgate,"out%d",serverID);
            send(message, opgate);
        }
    state = 1;

    }

//    else if(msg->getArrivalGate()==gate("cin1") or msg->getArrivalGate()==gate("cin2")){
    else if(isClientGate(msg->getArrivalGate())){

        std::istringstream iss1(msg->getFullName()); double sc;

        std::cout  <<getName()<<" -received gossip \""<<msg->getFullName()<<'\"'<<std::endl;
        outputFile <<getName()<<" -received gossip \""<<msg->getFullName()<<'\"'<<std::endl;

        int i = 0;
        std::string buff; iss1>>buff;
        while(iss1>>sc)
            avgScore[i++] += sc/(c+0.0);
        scoresReceived += 1;

    }
    else{
        std::istringstream iss(msg->getFullName()); int num;
            iss>>num;
            maximaList.push_back(num);
            int correctMax = actualMaxima.front();
            actualMaxima.pop();
            if(num==correctMax){
                int srcID;
                for(srcID=0;srcID<n;srcID++){
                    char ipgate[4];
                    std::sprintf(ipgate,"in%d",srcID);
                    if(msg->getArrivalGate()==gate(ipgate)) break;
                }
                correctReplies[srcID]+=1;
            }


            if(maximaList.size()==n){
                if(state==1) state = 2;
                int max = 1;
                for(int e : maximaList) if(e>max) max = e;

                std::cout  << "client "<<getName()<<" Round"<<state/2<<" computed global maxima "<<max<<std::endl;
                outputFile << "client "<<getName()<<" Round"<<state/2<<" computed global maxima "<<max<<std::endl;

                auto currentTime = std::chrono::system_clock::now();
                std::time_t t = std::chrono::system_clock::to_time_t(currentTime);

                std::ostringstream oss;
                int i = 0;
                oss<<t<<':'<<getName()<<": ";


                for(int s:correctReplies) {

                    if(totalRequests[i]==0) selfScoring[i] = 0.0;
                    else selfScoring[i] = s/totalRequests[i];

                    oss<<selfScoring[i]<<' ';

                    i++;
                }


                maximaList.clear();

                cMessage *scoreList1 = new cMessage(oss.str().c_str());
                send(scoreList1,"cout1");
                cMessage *scoreList2 = new cMessage(oss.str().c_str());
                send(scoreList2,"cout2");

                if(state==2) state = 3;

            }


    }


    // """Round 2"""


    if(scoresReceived==2 and state==3){             // sent yours as well as received others, proceed to round 2

        for(int i=0;i<n;i++) totalRequests[i] = 0.0;

        auto cmp = [](std::pair<int,double> x, std::pair<int,double> y){return x.second>y.second;};
        std::priority_queue<std::pair<int,double>,std::vector<std::pair<int,double>>,
                            decltype(cmp)> pq(cmp);
        int i=0;

        std::cout  <<"client "<<getName()<<" : "<<"computed average scoring : ";
        outputFile <<"client "<<getName()<<" : "<<"computed average scoring : ";

        for(double s:avgScore){

            std::cout  <<s<<' ';
            outputFile <<s<<' ';

            pq.push({i++,s});
        }
        std::cout  <<std::endl;;
        outputFile <<std::endl;

        integerList = generateRandomVector(40, 1, 50);

        serverList.clear();
        int j=surveySize;
        while(j--){
            int s = pq.top().first;
            pq.pop();
            serverList.push_back(s);
        }


        for(int taskID=0;taskID<n;taskID++){
            auto start = integerList.begin()+k*taskID;
            auto end = integerList.begin()+k*(taskID+1);
            std::vector<int> subarray(start,end);

            int serverID = serverList[taskID%surveySize];
            totalRequests[serverID] +=1.0;                                //


            std::ostringstream oss;
            oss<<taskID<<' ';
            for (int i : subarray) oss << i << " ";
            cMessage *message = new cMessage(oss.str().c_str());

            char opgate[4];
            std::sprintf(opgate,"out%d",serverID);
            send(message, opgate);
        }

        state = 4;



    }


    delete msg;
}






