#pragma once

#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <random>
#include "libcuckoo/cuckoohash_map.hh"
#include "moodycamel/BlockingConcurrentQueue.h"

#include "Config.h"
#include "Network.h"
#include "Node.h"
#include "Position.h"
#include "SpinLock.h"
#include "ThreadPool.h"
#include "ego.hpp"

typedef cuckoohash_map<Hash, std::weak_ptr<Position>, Hasher> TranspositionTable;
typedef FastStack<Node*, 19 * 19 * 3> NodeTrace;

class Tree {
public:
    Tree();

    std::shared_ptr<Position> maybeAddPosition(const RawBoard &board);
    void evaluate(const std::chrono::milliseconds duration);
    void evaluate(const size_t evaluations);

    inline std::shared_ptr<Node> const& rootNode() const { return _rootNode; }
    inline Board const& rootBoard() const { return _rootBoard; }

    void setKomi(float komi);
    float getKomi() const { return _rootBoard.Komi(); }

    Vertex bestMove();
    void playMove(Vertex const& vertex);

    Player rollout(Board playoutBoard, Network* net);

private:
    TranspositionTable _transpositionTable;
    std::queue<std::shared_ptr<Node>> _lastRootNodes;
    std::shared_ptr<Node> _rootNode;
    Board _rootBoard;
    std::vector<Network> _networks;
    std::random_device _rd;
    std::mt19937 _gen;

    void beginEvaluation();
    void playout(std::atomic<bool>* keepRunning, Network *net);
    void visitNode(Node* node);
    void updateStatistics(NodeTrace& trace, float score) const;
    void setRootNode(Vertex const& vertex);
    void resetEvaluationQueue();
    void purgeTranspositionTable();
    void addDirichletNoise(const float amount, const float distribution);
};
