#ifndef COLLECTOR_H
#define COLLECTOR_H

#include "Tree.h"

class Collector
{
public:
    Collector(string host, int port);
    void collectMove(const Tree &tree);
    void sendData();
private:
    //all variables from Franzis code here
    int connectToServer(string host, int port);
    string _server;
    int _port;
};

#endif // COLLECTOR_H
