#include <chrono>
#include "argos/Config.h"
#include "argos/Network.h"
#include "argos/Tree.h"
#include "ego.hpp"

int main(int argc, const char** argv) {
    static const size_t numSeconds = 10;

    auto config = argos::config::parse(argc, argv);
    Tree tree(config);

    // initialize root node
    tree.evaluate(0);

    const size_t initialVisits = tree.rootNode()->statistics().num_evaluations.load();

    auto start = chrono::steady_clock::now();
    tree.evaluate(std::chrono::seconds(numSeconds));
    auto end = chrono::steady_clock::now();

    const size_t visits = tree.rootNode()->statistics().num_evaluations.load() - initialVisits;
    const float rate = visits / std::chrono::duration<double>(end - start).count();

    std::cout << visits << " visits [" << rate << "v/s]" << std::endl;
}
