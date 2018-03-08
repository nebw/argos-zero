#ifndef COLLECTOR_H
#define COLLECTOR_H

#include "Tree.h"

class Collector {
public:
    Collector(const char* server, int port);
    void collectMove(const Tree& tree);
    void collectWinner(const Player& winner);
    void sendData(const Tree& tree, bool noResignMode);

private:
    // all variables from Franzis code here
    int connectToServer(const char* server, int port);
    const char* _server = nullptr;
    int _port;
    std::vector<std::array<double, BOARDSIZE * BOARDSIZE + 1>> _probabilities;
    std::vector<NetworkFeatures::Planes> _states;
    std::vector<double> _winrates;
    Player _winner;
};

#endif  // COLLECTOR_H
