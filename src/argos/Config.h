#pragma once

#include <boost/filesystem.hpp>
#include <thread>

namespace config {
enum MXNET_DEVICE_TYPE { CPU = 1, GPU = 2, CPU_PINNED = 3 };

static const MXNET_DEVICE_TYPE defaultDevice = CPU;


static const boost::filesystem::path networkPath("/home/julianstastny/Documents/Softwareprojekt/expertnet_small");
static const boost::filesystem::path logFilePath("/home/julianstastny/Documents/Softwareprojekt/log");

static const size_t boardSize = BOARDSIZE;


namespace tree {
    static const size_t numThreads = std::thread::hardware_concurrency() == 0 ? 4 : std::thread::hardware_concurrency();
    static const size_t randomizeFirstNMoves = 2;
    static const size_t numLastRootNodes = 3;
    static const size_t virtualPlayouts = 5;
    static const size_t expandAt = virtualPlayouts + 1;
    static const float priorC = 5;
    static const bool networkRollouts = false;
    static const bool trainingMode = true;
}  // namespace tree

namespace time {
    static const int C = 80;
    static const int maxPly = 80;
    static const auto delay = std::chrono::milliseconds(10);
}  // namespace time

namespace engine {
    static const auto totalTime = std::chrono::milliseconds(1000 * 60 * 10);
    static const float resignThreshold = 0.1f;
}  // namespace engine
}  // namespace config
