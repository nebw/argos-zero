#include <iostream>
#include "argos/Config.h"

int main() {
    ArgosConfig::get().networkPath("test/path");
    std::cout << ArgosConfig::get().networkPath() << std::endl;
}