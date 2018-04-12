#pragma once

#include <memory>
#include <queue>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "libcuckoo/cuckoohash_map.hh"
#include "moodycamel/BlockingConcurrentQueue.h"

#include "BatchProcessor.h"
#include "Config.h"
#include "Network.h"
#include "Node.h"
#include "Position.h"
#include "SpinLock.h"
#include "ThreadPool.h"
#include "ego.hpp"

typedef cuckoohash_map<Hash, std::weak_ptr<Position>, Hasher> TranspositionTable;
typedef FastStack<Node*, config::boardSize * config::boardSize * 3> NodeTrace;
typedef std::set<uint64> HashTrace;

class Tree {
public:
    Tree(const argos::config::Config& config);
    ~Tree();

    std::shared_ptr<Position> maybeAddPosition(const RawBoard& board);
    void evaluate(const std::chrono::milliseconds duration);
    void evaluate(const size_t evaluations);

    inline std::shared_ptr<Node> const& rootNode() const { return _rootNode; }
    inline Board const& rootBoard() const { return _rootBoard; }
    inline argos::config::Config const& configuration() const { return _config; }

    void setKomi(float komi);
    float getKomi() const { return _rootBoard.Komi(); }

    Vertex bestMove();
    void playMove(Vertex const& vertex);

    Player rollout(Board playoutBoard, ConcurrentNodeQueue& queue,
                   moodycamel::ProducerToken const& token);

private:
    const argos::config::Config _config;
    TranspositionTable _transpositionTable;
    std::queue<std::shared_ptr<Node>> _lastRootNodes;
    ConcurrentNodeQueue _evaluationQueue;
    std::shared_ptr<Node> _rootNode;
    Board _rootBoard;
    HashTrace _hashTrace;
    std::vector<Network> _networks;
    moodycamel::ProducerToken _token;
    std::atomic<bool> _evaluationThreadKeepRunning;
    std::vector<std::thread> _evaluationThreads;
    std::random_device _rd;
    std::mt19937 _gen;

    void beginEvaluation();
    void playout(std::atomic<bool>* keepRunning);
    void visitNode(Node* node);
    void updateStatistics(NodeTrace& trace, float score);
    void setRootNode(Vertex const& vertex);
    void purgeTranspositionTable();
    void addDirichletNoise(const float amount, const float distribution);
};
