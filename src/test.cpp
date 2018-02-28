#include <iostream>
#include "argos/Config.h"

int main() {
    ArgosConfig::get().networkPath("test/networkPath");
    ArgosConfig::get().logFilePath("test/logFilePath");
    ArgosConfig::get().networkRollouts(false);

    std::cout << ArgosConfig::get().networkPath() << std::endl;
    std::cout << ArgosConfig::get().logFilePath() << std::endl;
    std::cout << ArgosConfig::get().networkRollouts() << std::endl;
}
