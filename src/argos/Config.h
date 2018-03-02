#pragma once

#include <boost/filesystem.hpp>
#include <thread>

namespace config {
enum MXNET_DEVICE_TYPE { CPU = 1, GPU = 2, CPU_PINNED = 3 };

static const MXNET_DEVICE_TYPE defaultDevice = GPU;

<<<<<<< HEAD
static const boost::filesystem::path networkPath("/home/franziska/Downloads/randomnet_small_9x9");
static const boost::filesystem::path logFilePath("/home/franziska/Documents/Master/sem1/argos-dbg.log");

=======
static const boost::filesystem::path networkPath("/home/ben/tmp/randomnet_small_9x9");
static const boost::filesystem::path logFilePath("/home/ben/tmp/argos-dbg.log");
>>>>>>> batched-inference

static const size_t boardSize = BOARDSIZE;


namespace tree {
    static const size_t batchSize = 64;
    static const size_t numThreads = std::max<size_t>(
        batchSize,
        std::thread::hardware_concurrency() == 0 ? 4 : std::thread::hardware_concurrency());
    static const size_t randomizeFirstNMoves = 0;
    static const size_t numLastRootNodes = 3;
    static const size_t virtualPlayouts = 5;
    static const size_t expandAt = virtualPlayouts + 1;
    static const float priorC = 5;
<<<<<<< HEAD
    static const bool networkRollouts = false;
    static const bool trainingMode = true;
=======
    static const bool networkRollouts = true;
>>>>>>> batched-inference
}  // namespace tree

namespace time {
    static const int C = 80;
    static const int maxPly = 80;
    static const auto delay = std::chrono::milliseconds(10);
}  // namespace time

namespace engine {
    static const auto totalTime = std::chrono::milliseconds(1000 * 60 * 5);
    static const float resignThreshold = 0.1f;
}  // namespace engine
}  // namespace config
