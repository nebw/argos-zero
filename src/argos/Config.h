#pragma once

#include <boost/filesystem.hpp>
#include <thread>

namespace argosConfig {
    enum MXNET_DEVICE_TYPE
    {
        CPU         = 1,
        GPU         = 2,
        CPU_PINNED  = 3
    };

    struct Config {
        struct Tree {
            size_t numThreads = std::thread::hardware_concurrency() == 0 ? 4 : std::thread::hardware_concurrency();
            size_t randomizeFirstNMoves = 2;
            size_t numLastRootNodes = 3;
            size_t virtualPlayouts = 5;
            size_t expandAt = virtualPlayouts + 1;
            float priorC = 5;
            bool networkRollouts = true;
        };
        struct Time {
            int C = 80;
            int maxPly = 80;
            std::chrono::milliseconds delay = std::chrono::milliseconds(10);
        };
        struct Engine {
            std::chrono::milliseconds totalTime = std::chrono::milliseconds(1000 * 60 * 10);
            float resignThreshold = 0.1f;
        };
        boost::filesystem::path networkPath;
        boost::filesystem::path logFilePath;
        MXNET_DEVICE_TYPE deviceType = CPU;
        size_t boardSize = BOARDSIZE;
        Tree tree;
        Time time;
        Engine engine;
    };

    static Config& globalConfig() {
        static Config global;
        return global;
    }
}

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
    void networkRollouts(bool const& networkRollouts) {
      _networkRollouts = networkRollouts;
    }
    /* getters */
    boost::filesystem::path const& networkPath() const { return _networkPath; }
    boost::filesystem::path const& logFilePath() const { return _logFilePath; }
    bool const& networkRollouts() const { return _networkRollouts; }
private:
    ArgosConfig(){};
    ArgosConfig(const ArgosConfig&);
    ArgosConfig& operator=(const ArgosConfig&);

    boost::filesystem::path _networkPath;
    boost::filesystem::path _logFilePath;
    bool _networkRollouts;
};
