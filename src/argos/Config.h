#pragma once

#include <boost/filesystem.hpp>
#include <memory>
#include <thread>
#include <chrono>
#include <utility>

// #define DEBUG

namespace argos {
    namespace config {
        namespace defaults
        {
            static const std::string DEFAULT_SERVER = "127.0.0.1";
            namespace time
            {
                static const constexpr std::chrono::milliseconds DEFAULT_DELAY = std::chrono::milliseconds(10);
            }
            namespace engine
            {
                static const constexpr std::chrono::milliseconds DEFAULT_TOTAL_TIME = std::chrono::milliseconds(1000 * 60 * 10);
            }
        }
        enum MXNET_DEVICE_TYPE {
            CPU         = 1,
            GPU         = 2,
            CPU_PINNED  = 3
        };

        class Tree final
        {
        public:
            static const constexpr size_t DEFAULT_BATCH_SIZE                = 32;
            static const constexpr size_t DEFAULT_NUM_EVALUATION_THREADS    = 2;
            static const constexpr size_t DEFAULT_NUM_THREADS               = DEFAULT_BATCH_SIZE * DEFAULT_NUM_EVALUATION_THREADS;
            static const constexpr size_t DEFAULT_RANDOMIZE_FIRST_N_MOVES   = 10;
            static const constexpr size_t DEFAULT_NUM_LAST_ROOT_NODES       = 3;
            static const constexpr size_t DEFAULT_VIRTUAL_PLAYOUTS          = 0;
            static const constexpr size_t DEFAULT_EXPAND_AT                 = DEFAULT_VIRTUAL_PLAYOUTS + 1;
            static const constexpr float DEFAULT_PRIOR_C                    = 5;
            static const constexpr bool DEFAULT_NETWORK_ROLLOUTS            = false;
            static const constexpr bool DEFAULT_TRAINING_MODE               = false;
        private:
            explicit Tree(const size_t &batchSize,
                          const size_t &numEvaluationThreads,
                          const size_t &numThreads,
                          const size_t &randomizeFirstNMoves,
                          const size_t &numLastRootNodes,
                          const size_t &virtualPlayouts,
                          const size_t &expandAt,
                          const float &priorC,
                          const bool &networkRollouts,
                          const bool &trainingMode) :
                    batchSize(batchSize),
                    numEvaluationThreads(numEvaluationThreads),
                    numThreads(numThreads),
                    randomizeFirstNMoves(randomizeFirstNMoves),
                    numLastRootNodes(numLastRootNodes),
                    virtualPlayouts(virtualPlayouts),
                    expandAt(expandAt),
                    priorC(priorC),
                    networkRollouts(networkRollouts),
                    trainingMode(trainingMode) {}
        public:
            class Builder;
            const size_t batchSize;
            const size_t numEvaluationThreads;
            const size_t numThreads;
            const size_t randomizeFirstNMoves;
            const size_t numLastRootNodes;
            const size_t virtualPlayouts;
            const size_t expandAt;
            const float priorC;
            const bool networkRollouts;
            const bool trainingMode;
        };

        class Tree::Builder final
        {
        private:
            size_t _batchSize               = DEFAULT_BATCH_SIZE;
            size_t _numEvaluationThreads    = DEFAULT_NUM_EVALUATION_THREADS;
            size_t _numThreads              = DEFAULT_NUM_THREADS;
            size_t _randomizeFirstNMoves    = DEFAULT_RANDOMIZE_FIRST_N_MOVES;
            size_t _numLastRootNodes        = DEFAULT_NUM_LAST_ROOT_NODES;
            size_t _virtualPlayouts         = DEFAULT_VIRTUAL_PLAYOUTS;
            size_t _expandAt                = DEFAULT_EXPAND_AT;
            float _priorC                   = DEFAULT_PRIOR_C;
            bool _networkRollouts           = DEFAULT_NETWORK_ROLLOUTS;
            bool _trainingMode              = DEFAULT_TRAINING_MODE;
        public:
            Builder batchSize(const size_t &val) { _batchSize = val; return *this; }
            Builder numEvaluationThreads(const size_t &val) { _numEvaluationThreads = val; return *this; }
            Builder numThreads(const size_t &val) { _numThreads = val; return *this; }
            Builder randomizeFirstNMoves(const size_t &val) { _randomizeFirstNMoves = val; return *this; }
            Builder numLastRootNodes(const size_t &val) { _numLastRootNodes = val; return *this; }
            Builder virtualPlayouts(const size_t &val) { _virtualPlayouts = val; return *this; }
            Builder expandAt(const size_t &val) { _expandAt = val; return *this; }
            Builder priorC(const float &val) { _priorC = val; return *this; }
            Builder networkRollouts(const bool &val) { _networkRollouts = val; return *this; }
            Builder trainingMode(const bool &val) { _trainingMode = val; return *this; }
            Tree build() {
                return Tree(_batchSize,
                            _numEvaluationThreads,
                            _numThreads,
                            _randomizeFirstNMoves,
                            _numLastRootNodes,
                            _virtualPlayouts,
                            _expandAt,
                            _priorC,
                            _networkRollouts,
                            _trainingMode);
            }
        };

        class Time final
        {
        public:
            static const constexpr int DEFAULT_C = 80;
            static const constexpr int DEFAULT_MAX_PLY = 80;
        private:
            explicit Time(const int &C,
                          const int &maxPly,
                          const std::chrono::milliseconds &delay) :
                    C(C),
                    maxPly(maxPly),
                    delay(delay) {}

        public:
            class Builder;
            const int C;
            const int maxPly;
            const std::chrono::milliseconds delay;
        };

        class Time::Builder final
        {
        private:
            int _C                              = DEFAULT_C;
            int _maxPly                         = DEFAULT_MAX_PLY;
            std::chrono::milliseconds _delay    = defaults::time::DEFAULT_DELAY;
        public:
            Builder C(const int &C) { _C = C; return *this; }
            Builder maxPly(const int &maxPly) { _maxPly = maxPly; return *this; }
            Builder delay(const std::chrono::milliseconds &delay) { _delay = delay; return *this; }
            Time build() { return Time(_C,
                                       _maxPly,
                                       _delay); }
        };

        class Engine final
        {
        public:
            static const constexpr float DEFAULT_RESIGN_THRESHOLD               = 0.1f;
        private:
            explicit Engine(const float &resignThreshold,
                            const std::chrono::milliseconds &totalTime) :
                    resignThreshold(resignThreshold),
                    totalTime(totalTime) {}
        public:
            class Builder;
            const float resignThreshold;
            const std::chrono::milliseconds totalTime;
        };

        class Engine::Builder final
        {
        private:
            float _resignThreshold                  = DEFAULT_RESIGN_THRESHOLD;
            std::chrono::milliseconds _totalTime    = defaults::engine::DEFAULT_TOTAL_TIME;
        public:
            Builder resignThreshold(const float &resignThreshold) { _resignThreshold = resignThreshold; return *this; }
            Builder totalTime(const std::chrono::milliseconds &totalTime) { _totalTime = totalTime; return *this; }
            Engine build() { return Engine(_resignThreshold,
                                           _totalTime); }
        };

        static const Tree DEFAULT_TREE      = Tree::Builder().build();
        static const Time DEFAULT_TIME      = Time::Builder().build();
        static const Engine DEFAULT_ENGINE  = Engine::Builder().build();

        class Config final
        {
        public:

            static const constexpr size_t DEFAULT_BOARD_SIZE                = BOARDSIZE;
            static const constexpr MXNET_DEVICE_TYPE DEFAULT_DEVICE_TYPE    = GPU;
            static const constexpr int DEFAULT_PORT                         = 18000;

        private:
            explicit Config(const boost::filesystem::path &networkPath,
                            const boost::filesystem::path &logFilePath,
                            const Tree &tree,
                            Time time,
                            Engine engine,
                            const size_t &boardSize,
                            const MXNET_DEVICE_TYPE &deviceType,
                            const int &port,
                            const std::string &server) :
                    networkPath(networkPath),
                    logFilePath(logFilePath),
                    tree(tree),
                    time(std::move(time)),
                    engine(std::move(engine)),
                    boardSize(boardSize),
                    deviceType(deviceType),
                    port(port),
                    server(server) {}
        public:
            class Builder;
            const boost::filesystem::path networkPath;
            const boost::filesystem::path logFilePath;
            const Tree tree;
            const Time time;
            const Engine engine;
            const size_t boardSize;
            const MXNET_DEVICE_TYPE deviceType;
            const int port;
            const std::string server;
        };

        class Config::Builder final
        {
        private:
            const boost::filesystem::path _networkPath;
            const boost::filesystem::path _logFilePath;
            std::shared_ptr<Tree> _tree;
            std::shared_ptr<Time> _time;
            std::shared_ptr<Engine> _engine;
            size_t _boardSize                           = DEFAULT_BOARD_SIZE;
            MXNET_DEVICE_TYPE _deviceType               = DEFAULT_DEVICE_TYPE;
            int _port                                   = DEFAULT_PORT;
            std::string _server                         = defaults::DEFAULT_SERVER;
        public:
            Builder(const boost::filesystem::path &networkPath,
                    const boost::filesystem::path &logFilePath) :
                    _networkPath(networkPath),
                    _logFilePath(logFilePath) {}
            Builder tree(const Tree &tree) { _tree = std::make_shared<Tree>(tree); return *this; }
            Builder time(const Time &time) { _time = std::make_shared<Time>(time); return *this; }
            Builder engine(const Engine &engine) { _engine = std::make_shared<Engine>(engine); return *this; }
            Builder boardSize(const size_t &boardSize) { _boardSize = boardSize; return *this; }
            Builder deviceType(const MXNET_DEVICE_TYPE &deviceType) { _deviceType = deviceType; return *this; }
            Builder port(const int &port) { _port = port; return *this; }
            Builder server(const std::string &server) { _server = server; return *this; }
            Config build() { return Config(
                        _networkPath,
                        _logFilePath,
                        _tree ? *_tree : DEFAULT_TREE,
                        _time ? *_time : DEFAULT_TIME,
                        _engine ? *_engine : DEFAULT_ENGINE,
                        _boardSize,
                        _deviceType,
                        _port,
                        _server); }
        };

        Config parse(int argc, const char **argv);
    }
}

// TODO deprecated, use config object instead and delete unused parameters
namespace config {
    static const size_t boardSize = BOARDSIZE; // TODO used during compilation

    namespace tree {
        static const size_t batchSize = argos::config::Tree::DEFAULT_BATCH_SIZE;  // TODO used in array initialization...
    } // namespace tree
}  // namespace config
