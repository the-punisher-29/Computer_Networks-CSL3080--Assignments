#pragma once
#include <string>
#include <ctime>
using namespace std;
struct Message {
    time_t timestamp;
    string sourceIP;
    int messageNumber;
    string toString() const;
    static string createDeadNodeMessage(const string& deadIP, 
                                           int deadPort,
                                           const string& reporterIP);
    static size_t hash(const string& msg);
};