#include "msg.h"
#include <sstream>
#include <functional>
std::string Message::toString() const {
    std::ostringstream oss;
    oss << timestamp << ":" << sourceIP << ":" << messageNumber;
    return oss.str();
}

std::string Message::createDeadNodeMessage(const std::string& deadIP, 
                                         int deadPort,
                                         const std::string& reporterIP) {
    std::ostringstream oss;
    oss << "Dead Node:" << deadIP << ":" << deadPort << ":" 
        << time(nullptr) << ":" << reporterIP;
    return oss.str();
}

size_t Message::hash(const std::string& msg) {
    return std::hash<std::string>{}(msg);
}