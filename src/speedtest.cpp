#include <chrono>
#include "ego.hpp"
#include "argos/Config.h"
#include "argos/Network.h"

using namespace std;
using namespace mxnet::cpp;

int main()
{
    const size_t num_warmup = 10000;
    const size_t num_evaluations = 10000;

    auto net = Network(config::networkPath.string());
    Board board;

    for (size_t i=0; i < num_warmup; ++i)
    {
        net.apply(board);
    }

    auto start = chrono::steady_clock::now();
    for (size_t i=0; i < num_evaluations; ++i)
    {
        net.apply(board);
    }
    auto end = chrono::steady_clock::now();
    auto diff = end - start;
    std::cout << chrono::duration <double, milli> (diff).count() / num_evaluations << " ms" << std::endl;
}
