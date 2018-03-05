#include "argos/Config.h"
#include <iostream>

int main(int argc, const char **argv) {
    using namespace std;

    auto config = argos::config::parse(argc, argv);

    cout << config.networkPath << endl;
    cout << config.logFilePath << endl;
    cout << config.tree.networkRollouts << endl;
    cout << config.time.delay.count() << endl;

    return 0;
}
