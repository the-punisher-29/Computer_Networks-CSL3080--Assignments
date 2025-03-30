import sys
n,c = sys.argv[1:]
n = int(n)
c = int(c)

x = f'''
#include <vector>
#include <sstream>
#include <random>
#include <stdio.h>
#include <omnetpp.h>
#include <queue>
#include <chrono>
#include <fstream>

using namespace omnetpp;

class client : public cSimpleModule {'{'}
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
    virtual int isClientGate(cGate* g){'{'}
        for(int i=1;i<c;i++){'{'}
            char testGate[4];
            std::sprintf(testGate,"cin%d",i);
            if(g==gate(testGate)) return 1;
        {'}'}
        return 0;


    {'}'}
{'}'};

Define_Module(client);



int trueMax(std::vector<int> vec){'{'}
    int max = 1;
    for(int y : vec) if(max<y) max = y;
    return max;
{'}'}

std::vector<int> generateRandomVector(int size, int min, int max) {'{'}
    std::vector<int> randomVector;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);

    for (int i = 0; i < size; ++i)
        randomVector.push_back(dis(gen));
    return randomVector;
{'}'}

std::vector<int> NonRepeatingRandomArray(int size, int min, int max) {'{'}
    std::vector<int> randomArray;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    std::set<int> uniqueNumbers;

    while (uniqueNumbers.size() < size) {'{'}
        int randomNumber = dis(gen);
        if (uniqueNumbers.find(randomNumber) == uniqueNumbers.end()) {'{'}
            uniqueNumbers.insert(randomNumber);
            randomArray.push_back(randomNumber);
        {'}'}
    {'}'}
    return randomArray;
{'}'}

void client::initialize() {'{'}
    state = 0;
    n = {n}; x = 4*n;
    k = x/n;            // 4
    surveySize = n/2+1; // 6
    scoresReceived = 0;
    c = {c};

    outputFile.open("output.txt", std::ios_base::app);

    scheduleAt(simTime() + 1.0, new cMessage("SelfMessage"));

{'}'}

void client::handleMessage(cMessage *msg){'{'}

    if(msg->isSelfMessage()){'{'}
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

        for(int taskID=0;taskID<n;taskID++){'{'}
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
        {'}'}
    state = 1;

    {'}'}

//    else if(msg->getArrivalGate()==gate("cin1") or msg->getArrivalGate()==gate("cin2")){'{'}
    else if(isClientGate(msg->getArrivalGate())){'{'}

        std::istringstream iss1(msg->getFullName()); double sc;

        std::cout  <<getName()<<" -received gossip \\\""<<msg->getFullName()<<'\\\"'<<std::endl;
        outputFile <<getName()<<" -received gossip \\\""<<msg->getFullName()<<'\\\"'<<std::endl;

        int i = 0;
        std::string buff; iss1>>buff;
        while(iss1>>sc)
            avgScore[i++] += sc/(c+0.0);
        scoresReceived += 1;

    {'}'}
    else{'{'}
        std::istringstream iss(msg->getFullName()); int num;
            iss>>num;
            maximaList.push_back(num);
            int correctMax = actualMaxima.front();
            actualMaxima.pop();
            if(num==correctMax){'{'}
                int srcID;
                for(srcID=0;srcID<n;srcID++){'{'}
                    char ipgate[4];
                    std::sprintf(ipgate,"in%d",srcID);
                    if(msg->getArrivalGate()==gate(ipgate)) break;
                {'}'}
                correctReplies[srcID]+=1;
            {'}'}


            if(maximaList.size()==n){'{'}
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


                for(int s:correctReplies) {'{'}

                    if(totalRequests[i]==0) selfScoring[i] = 0.0;
                    else selfScoring[i] = s/totalRequests[i];

                    oss<<selfScoring[i]<<' ';

                    i++;
                {'}'}


                maximaList.clear();

                cMessage *scoreList1 = new cMessage(oss.str().c_str());
                send(scoreList1,"cout1");
                cMessage *scoreList2 = new cMessage(oss.str().c_str());
                send(scoreList2,"cout2");

                if(state==2) state = 3;

            {'}'}


    {'}'}


    // """Round 2"""


    if(scoresReceived==2 and state==3){'{'}             // sent yours as well as received others, proceed to round 2

        for(int i=0;i<n;i++) totalRequests[i] = 0.0;

        auto cmp = [](std::pair<int,double> x, std::pair<int,double> y){'{'}return x.second>y.second;{'}'};
        std::priority_queue<std::pair<int,double>,std::vector<std::pair<int,double>>,
                            decltype(cmp)> pq(cmp);
        int i=0;

        std::cout  <<"client "<<getName()<<" : "<<"computed average scoring : ";
        outputFile <<"client "<<getName()<<" : "<<"computed average scoring : ";

        for(double s:avgScore){'{'}

            std::cout  <<s<<' ';
            outputFile <<s<<' ';

            pq.push({'{'}i++,s{'}'});
        {'}'}
        std::cout  <<std::endl;;
        outputFile <<std::endl;

        integerList = generateRandomVector(40, 1, 50);

        serverList.clear();
        int j=surveySize;
        while(j--){'{'}
            int s = pq.top().first;
            pq.pop();
            serverList.push_back(s);
        {'}'}


        for(int taskID=0;taskID<n;taskID++){'{'}
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
        {'}'}

        state = 4;



    {'}'}


    delete msg;
{'}'}






'''

with open('client.cc','w') as f:
    f.write(x)

y = f'''
#include <omnetpp.h>
#include <vector>
#include <sstream>
#include <random>
#include <stdio.h>
#include <fstream>
#include <map>

using namespace omnetpp;

class server : public cSimpleModule {'{'}
    int n,c;
    std::ofstream outputFile;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
{'}'};

Define_Module(server);

void server::initialize() {'{'}

    outputFile.open("output.txt", std::ios_base::app);
    n = {n}; c = {c};

{'}'}

void server::handleMessage(cMessage *msg) {'{'}
    if (msg->isSelfMessage()) {'{'}
        // Handle self messages if any
    {'}'} else {'{'}
        std::istringstream iss(msg->getFullName());
        int num;
        int max = 1;
        std::vector<int> receivedList;
        int taskID; iss>>taskID;
        while (iss >> num) {'{'}
            receivedList.push_back(num);
            if(max<num) max = num;
        {'}'}


        if(getId()<2+n/4){'{'}
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 1);
            int toss = dis(gen);
            if(toss) max -= 5;
        {'}'}

        cGate *arrivalGate = msg->getArrivalGate();
        char* opgate;


        {'else'.join([f' if (arrivalGate == gate("in{i}")) opgate = "out{i}"; ' for i in range(c)])}

        std::ostringstream oss; oss<<max;
        cMessage *rep = new cMessage(oss.str().c_str());
        send(rep, opgate);

        std::cout  <<getName()<<" - task ID "<<taskID<<" : computed maxima = "<<max<<std::endl;;
        outputFile <<getName()<<" - task ID "<<taskID<<" : computed maxima = "<<max<<std::endl;;



    {'}'}
    delete msg;
{'}'}


'''

with open('server.cc', 'w') as f:
    f.write(y)

z = f'''

simple server
{'{'}
    gates:

        {' '.join(['input' + ' in' + str(i) + ';' for i in range(c)])}

        {' '.join(['output' + ' out' + str(i) + ';' for i in range(c)])}

{'}'}
'''

with open('server.ned', 'w') as f:
    f.write(z)

a = f'''
simple client
{'{'}
    gates:
    
        {' '.join(['input' + ' in' + str(i) + ';' for i in range(n)])}
        
        {' '.join(['input' + ' cin' + str(i) + ';' for i in range(1,c)])}

        
        {' '.join(['output' + ' out' + str(i) + ';' for i in range(n)])}
        
        {' '.join(['output' + ' cout' + str(i) + ';' for i in range(1,c)])}
        
        

{'}'}
'''

with open('client.ned','w') as f:
    f.write(a)

servers = [
              'server0: server { @display("p=107,63"); }\n',
              'server1: server { @display("p=165,63"); }\n',
              'server2: server { @display("p=229,63"); }\n',
              'server3: server { @display("p=297,63"); }\n',
              'server4: server { @display("p=368,63"); }\n',
              'server5: server { @display("p=439,63"); }\n',
              'server6: server { @display("p=517,63"); }\n',
              'server7: server { @display("p=589,63"); }\n',
              'server8: server { @display("p=655,63"); }\n',
              'server9: server { @display("p=719,63"); }\n',
          ][:n]

clients = [
              'client0: client { @display("p=190,175"); }\n',
              'client1: client { @display("p=277,230"); }\n',
              'client2: client { @display("p=390,245"); }\n',
              'client3: client { @display("p=505,230"); }\n',
              'client4: client { @display("p=607,175"); }\n',
          ][:c]

connectionsCS = [
    f'client{C}.out{S} --> IdealChannel --> server{S}.in{C};'
    + f'\nserver{S}.out{C} --> IdealChannel --> client{C}.in{S};\n'

    for C in range(c) for S in range(n)
]

connectionsCC = []

for src in range(0, c - 1):
    for i, dst in enumerate([d for d in range(c) if d != src]):

        x = f'client{src}.cout{i + 1} --> IdealChannel --> client{dst}.cin{src + 1};\n'
        connectionsCC.append(x)

src = c - 1
for dst in range(0, c - 1):
    x = f'client{src}.cout{dst+1} --> IdealChannel --> client{dst}.cin{dst + 1};\n'
    connectionsCC.append(x)

b = f'''

import ned.IdealChannel;


network Network
{'{'}
    submodules:
        {' '.join(servers)}
        {' '.join(clients)}

    connections:
        {' '.join(connectionsCS)}
        {' '.join(connectionsCC)}

{'}'}

'''

with open('network.ned','w') as f:
    f.write(b)

with open('output.txt','w') as f:
    pass

