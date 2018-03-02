#ifndef COLLECTOR_H
#define COLLECTOR_H

#include "Tree.h"

class Collector
{
public:
    Collector(const char* server, int port);
    void collectMove(const Tree &tree);
    void sendData(const Tree& tree);
private:
    //all variables from Franzis code here
    int connectToServer(const char* server, int port);
    const char* _server;
    int _port;
    std::vector<std::array<double, 362>> _probabilities;
    std::vector<NetworkFeatures::Planes> _states;
};

#endif // COLLECTOR_H
