#include <iostream>
#include "argos/Config.h"

int main() {
    ArgosConfig::get().networkPath("test/networkPath");
    ArgosConfig::get().logFilePath("test/logFilePath");

    std::cout << ArgosConfig::get().networkPath() << std::endl;
    std::cout << ArgosConfig::get().logFilePath() << std::endl;
}
