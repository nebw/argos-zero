#include "argos/Config.h"
#include <iostream>

int main(int argc, const char **argv) {
    using namespace std;

    try {
        ArgosConfig::Config config = ArgosConfig::initializeConfig(argc, argv);

        cout << config.networkPath << endl;
        cout << config.logFilePath << endl;
        cout << config.tree.networkRollouts << endl;
        cout << config.time.delay.count() << endl;
    } catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
    }

    return 0;
}
