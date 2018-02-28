#pragma once

#include <boost/filesystem.hpp>
#include <thread>

namespace config {
enum MXNET_DEVICE_TYPE { CPU = 1, GPU = 2, CPU_PINNED = 3 };

static const MXNET_DEVICE_TYPE defaultDevice = CPU;

static const boost::filesystem::path networkPath("/home/ben/tmp/expertnet_small");
static const boost::filesystem::path logFilePath("/home/ben/tmp/argos-dbg.log");

static const size_t boardSize = BOARDSIZE;

namespace tree {
    static const size_t numThreads = std::thread::hardware_concurrency() == 0 ? 4 : std::thread::hardware_concurrency();
    static const size_t randomizeFirstNMoves = 2;
    static const size_t numLastRootNodes = 3;
    static const size_t virtualPlayouts = 5;
    static const size_t expandAt = virtualPlayouts + 1;
    static const float priorC = 5;
    static const bool networkRollouts = false;
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

class ArgosConfig {
public:
    static ArgosConfig& get()
    {
        static ArgosConfig instance;
        return instance;
    }
    /* setters */
    void networkPath(std::string const& networkPath) {
        boost::filesystem::path boostNetworkPath(networkPath);
        _networkPath = boostNetworkPath;
    }
    void logFilePath(std::string const& logFilePath) {
        boost::filesystem::path boostLogFilePath(logFilePath);
        _logFilePath = boostLogFilePath;
    }
    /* getters */
    boost::filesystem::path const& networkPath() const { return _networkPath; }
    boost::filesystem::path const& logFilePath() const { return _logFilePath; }
private:
    ArgosConfig(){};
    ArgosConfig(const ArgosConfig&);
    ArgosConfig& operator=(const ArgosConfig&);

    boost::filesystem::path _networkPath;
    boost::filesystem::path _logFilePath;
};