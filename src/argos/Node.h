#pragma once

#include <boost/optional.hpp>
#include <memory>
#include <mutex>
#include <vector>

#include "SpinLock.h"
#include "Statistics.h"
#include "Network.h"
#include "ego.hpp"

class Position;
class Tree;

class Node {
public:
    typedef std::shared_ptr<Node> NodeSPtr;
    typedef std::vector<NodeSPtr> NodeStack;

    Node(std::shared_ptr<Position> position, Vertex const& parentMove)
        : _position(position), _parentMove(parentMove), _isEvaluated(false) {}

    inline bool isExpanded() const { return _children.is_initialized(); }
    bool expand(Tree& tree, Board const& board, Network& network);

    inline NodeStatistics& statistics() { return _statistics; }
    inline boost::optional<NodeStack> const& children() const { return _children; }
    inline std::shared_ptr<Position> const& position() const { return _position; }
    inline Vertex const& parentMove() const { return _parentMove; }
    inline bool isEvaluated() const { return _isEvaluated; }

    float getUCTValue(Node& parent) const;
    NodeSPtr const& getBestUCTChild();
    NodeSPtr const& getBestWinrateChild();
    NodeSPtr const& getMostVisitsChild();

    float winrate(Player const& player) const;
    void addEvaluation(float score);

    float getPrior();
    void setPrior(float prior);

private:
    std::shared_ptr<Position> _position;
    NodeStatistics _statistics;
    boost::optional<NodeStack> _children;
    Vertex _parentMove;
    SpinLock _expandLock;
    bool _isEvaluated;

    void addPrior(float prior);
};
