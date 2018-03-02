#pragma once

#include <boost/filesystem.hpp>
#include <thread>
#include <chrono>


namespace ArgosConfig
{
    enum MXNET_DEVICE_TYPE
    {
        CPU = 1,
        GPU = 2,
        CPU_PINNED = 3
    };

    class Tree final
    {
    public:
        static const constexpr size_t DEFAULT_NUM_THREADS = 4;
        static const constexpr size_t DEFAULT_RANDOMIZE_FIRST_N_MOVES = 2;
        static const constexpr size_t DEFAULT_NUM_LAST_ROOT_NODES = 3;
        static const constexpr size_t DEFAULT_VIRTUAL_PLAYOUTS = 5;
        static const constexpr size_t DEFAULT_EXPAND_AT = DEFAULT_VIRTUAL_PLAYOUTS + 1;
        static const constexpr float DEFAULT_PRIOR_C = 5;
        static const constexpr bool DEFAULT_NETWORK_ROLLOUTS = false;

    public:
        class Builder final
        {
            friend class Tree;
        private:
            size_t _numThreads = DEFAULT_NUM_THREADS;
            size_t _randomizeFirstNMoves = DEFAULT_RANDOMIZE_FIRST_N_MOVES;
            size_t _numLastRootNodes = DEFAULT_NUM_LAST_ROOT_NODES;
            size_t _virtualPlayouts = DEFAULT_VIRTUAL_PLAYOUTS;
            size_t _expandAt = DEFAULT_EXPAND_AT;
            float _priorC = DEFAULT_PRIOR_C;
            bool _networkRollouts = DEFAULT_NETWORK_ROLLOUTS;
        public:
            Builder numThreads(const size_t& val) { _numThreads = val; return *this; }
            Builder randomizeFirstNMoves(const size_t& val) { _randomizeFirstNMoves = val; return *this; }
            Builder numLastRootNodes(const size_t& val) { _numLastRootNodes = val; return *this; }
            Builder virtualPlayouts(const size_t& val) { _virtualPlayouts = val; return *this; }
            Builder expandAt(const size_t& val) { _expandAt = val; return *this; }
            Builder priorC(const float& val) { _priorC = val; return *this; }
            Builder networkRollouts(const bool& val) { _networkRollouts = val; return *this; }
            Tree build() { return Tree(*this); }
        };

    private:
        Tree(const Builder& builder) :
                numThreads(builder._numThreads),
                randomizeFirstNMoves(builder._randomizeFirstNMoves),
                numLastRootNodes(builder._numLastRootNodes),
                virtualPlayouts(builder._virtualPlayouts),
                expandAt(builder._expandAt),
                priorC(builder._priorC),
                networkRollouts(builder._networkRollouts) {}

    public:
        const size_t numThreads;
        const size_t randomizeFirstNMoves;
        const size_t numLastRootNodes;
        const size_t virtualPlayouts;
        const size_t expandAt;
        const float priorC;
        const bool networkRollouts;
    };

    class Time final
    {

    public:
        static const constexpr int DEFAULT_C = 80;
        static const constexpr int DEFAULT_MAX_PLY = 80;
        static const constexpr int DEFAULT_DELAY = 10;
    public:
        class Builder final
        {
            friend class Time;
        private:
            int _C = DEFAULT_C;
            int _maxPly = DEFAULT_MAX_PLY;
            std::chrono::milliseconds _delay = std::chrono::milliseconds(DEFAULT_DELAY);
        public:
            Builder C(const int& C) { _C = C; return *this; }
            Builder maxPly(const int& maxPly) { _maxPly = maxPly; return *this; }
            Builder delay(const std::chrono::milliseconds& delay) { _delay = delay; return *this; }
            Time build() { return Time(*this); }
        };
    private:
        Time(const Builder& builder) :
                C(builder._C),
                maxPly(builder._maxPly),
                delay(builder._delay) {}
    public:
        const int C;
        const int maxPly;
        const std::chrono::milliseconds delay;
    };

    class Engine final
    {

    public:
        static const constexpr float DEFAULT_RESIGN_THRESHOLD = 0.1f;
        static const constexpr int DEFAULT_TOTAL_TIME = 1000 * 60 * 10;
    public:
        class Builder final
        {
            friend class Engine;
        private:
            float _resignThreshold = DEFAULT_RESIGN_THRESHOLD;
            std::chrono::milliseconds _totalTime = std::chrono::milliseconds(DEFAULT_TOTAL_TIME);
        public:
            Builder resignThreshold(const float& resignThreshold) { _resignThreshold = resignThreshold; return *this; }
            Builder totalTime(const std::chrono::milliseconds& totalTime) { _totalTime = totalTime; return *this; }
            Engine build() { return Engine(*this); }
        };
    private:
        Engine(const Builder& builder) :
                resignThreshold(builder._resignThreshold),
                totalTime(builder._totalTime) {}
    public:
        const float resignThreshold;
        const std::chrono::milliseconds totalTime;
    };

    static const Tree DEFAULT_TREE = Tree::Builder().build();
    static const Time DEFAULT_TIME = Time::Builder().build();
    static const Engine DEFAULT_ENGINE = Engine::Builder().build();

    class Config final
    {
    public:
        static const constexpr size_t DEFAULT_BOARD_SIZE = 19;
        static const constexpr MXNET_DEVICE_TYPE DEFAULT_DEVICE_TYPE = CPU;
    public:
        class Builder final
        {
            friend class Config;
        private:
            const boost::filesystem::path _networkPath;
            const boost::filesystem::path _logFilePath;
            Tree* _tree = (Tree*) &DEFAULT_TREE;
            Time* _time = (Time*) &DEFAULT_TIME;
            Engine* _engine = (Engine*) &DEFAULT_ENGINE;
            size_t _boardSize = DEFAULT_BOARD_SIZE;
            MXNET_DEVICE_TYPE _deviceType = DEFAULT_DEVICE_TYPE;
        public:
            Builder(const boost::filesystem::path& networkPath, const boost::filesystem::path& logFilePath) : _networkPath(networkPath), _logFilePath(logFilePath) {}
            Builder tree(Tree& tree) { _tree = &tree; return *this; }
            Builder time(Time& time) { _time = &time; return *this; }
            Builder engine(Engine& engine) { _engine = &engine; return *this; }
            Builder boardSize(const size_t& boardSize) { _boardSize = boardSize; return *this; }
            Builder deviceType(const MXNET_DEVICE_TYPE& deviceType) { _deviceType = deviceType; return *this; }
            Config build() { return Config(*this); }
        };
    private:
        Config(const Builder& builder) :
                networkPath(builder._networkPath),
                logFilePath(builder._logFilePath),
                tree(*builder._tree),
                time(*builder._time),
                engine(*builder._engine),
                boardSize(builder._boardSize),
                deviceType(builder._deviceType) {}
    public:
        const boost::filesystem::path networkPath;
        const boost::filesystem::path logFilePath;
        const Tree tree;
        const Time time;
        const Engine engine;
        const size_t boardSize;
        const MXNET_DEVICE_TYPE deviceType;
    };

    Config initializeConfig(int argc, const char** argv);
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
