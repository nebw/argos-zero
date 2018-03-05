#include "argos/Config.h"
#include <iostream>

int main(int argc, const char **argv) {
    using namespace std;

    auto config = argos::config::parse(argc, argv);

    cout << "config.networkPath: " << config.networkPath << endl;
    cout << "config.logFilePath: " << config.logFilePath << endl;
    cout << "config.deviceType: " << config.deviceType << endl;
    cout << "config.boardSize: " << config.boardSize << endl;
    cout << "config.server: " << config.server << endl;
    cout << "config.port: " << config.port << endl;
    cout << "config.tree.batchSize: " << config.tree.batchSize << endl;
    cout << "config.tree.numEvaluationThreads: " << config.tree.numEvaluationThreads << endl;
    cout << "config.tree.numThreads: " << config.tree.numThreads << endl;
    cout << "config.tree.randomizeFirstNMoves: " << config.tree.randomizeFirstNMoves << endl;
    cout << "config.tree.numLastRootNodes: " << config.tree.numLastRootNodes << endl;
    cout << "config.tree.virtualPlayouts: " << config.tree.virtualPlayouts << endl;
    cout << "config.tree.expandAt: " << config.tree.expandAt << endl;
    cout << "config.tree.priorC: " << config.tree.priorC << endl;
    cout << "config.tree.networkRollouts: " << config.tree.networkRollouts << endl;
    cout << "config.tree.trainingMode: " << config.tree.trainingMode << endl;
    cout << "config.time.C: " << config.time.C << endl;
    cout << "config.time.maxPly: " << config.time.maxPly << endl;
    cout << "config.time.delay: " << config.time.delay.count() << endl;
    cout << "config.engine.totalTime: " << config.engine.totalTime.count() << endl;
    cout << "config.engine.resignThreshold: " << config.engine.resignThreshold << endl;

    return 0;
}
