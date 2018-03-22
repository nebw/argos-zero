#pragma once

#include <boost/optional.hpp>
#include <memory>
#include <mutex>
#include <random>

#include <vector>

#include "BatchProcessor.h"
#include "Config.h"
#include "Network.h"
#include "SpinLock.h"
#include "Statistics.h"
#include "ego.hpp"

class Position;
class Tree;

class Node {
public:
    typedef std::shared_ptr<Node> NodeSPtr;
    typedef std::vector<NodeSPtr> NodeStack;

    Node(std::shared_ptr<Position> position, Vertex const& parentMove,
         const argos::config::Tree& config, const float initialValue)
        : _position(position),
          _parentMove(parentMove),
          _isEvaluated(false),
          _isTerminalNode(false),
          _config(config) {
        _statistics.initial_value = initialValue;
    }

    inline bool isExpanded() const { return _children.is_initialized(); }
    bool expand(Tree& tree, Board& board, ConcurrentNodeQueue& queue,
                moodycamel::ProducerToken const& token);

    inline NodeStatistics& statistics() { return _statistics; }
    inline boost::optional<NodeStack> const& children() const { return _children; }
    inline std::shared_ptr<Position> const& position() const { return _position; }
    inline Vertex const& parentMove() const { return _parentMove; }
    inline bool isEvaluated() const { return _isEvaluated; }
    inline bool isTerminal() const { return _isTerminalNode; }

    float getUCTValue(Node& parent, mt19937&) const;
    float getBetaValue(Node& parent, std::mt19937& engine) const;

    NodeSPtr const& getBestUCTChild(std::mt19937& engine);
    NodeSPtr const& getBestWinrateChild();
    NodeSPtr const& getMostVisitsChild();

    float winrate(Player const& player) const;
    void addEvaluation(float score);

    float getPrior();
    void setPrior(float prior);

    SpinLock& getLock() { return _expandLock; }

private:
    std::shared_ptr<Position> _position;
    NodeStatistics _statistics;
    boost::optional<NodeStack> _children;
    Vertex _parentMove;
    SpinLock _expandLock;
    bool _isEvaluated;
    bool _isTerminalNode;
    argos::config::Tree _config;
};
