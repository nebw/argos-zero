#include "Config.h"
#include "Node.h"
#include "Position.h"
#include "Tree.h"
#include "Util.h"

#include <algorithm>
#include <cassert>

bool Node::expand(Tree& tree, Board const& board, Network& network) {
    std::unique_lock<SpinLock> lock(_expandLock, std::try_to_lock);
    if (!lock.owns_lock()) {
        // if another thread is already expanding the node, don't bother waiting
        return false;
    }
    if (isExpanded()) return false;

    NodeStack children;
    if (!board.BothPlayerPass()) {
        const Network::Result result = network.apply(board);

        if (config::tree::networkRollouts) {
            const float rolloutValue = tree.rollout(board, &network).ToScore();
            _position->statistics().value = rolloutValue;
        } else {
            _position->statistics().value = result.value;
        }

        const auto& legalMoves = _position->legalMoves(board);
        children.reserve(legalMoves.size());
        Player const& actPlayer = _position->actPlayer();

        RawBoard tempBoard;
        for (Vertex const& vertex : legalMoves) {
            tempBoard.Load(board);
            tempBoard.PlayLegal(actPlayer, vertex);

            auto position = tree.maybeAddPosition(tempBoard);
            auto child = std::make_shared<Node>(position, vertex);

            size_t posIdx;
            if (vertex == Vertex::Pass()) {
                posIdx = 19 * 19;
            } else {
                posIdx = vertex.GetRow() * 19 + vertex.GetColumn();
            }
            child->addPrior(result.candidates[posIdx].prior);

            children.push_back(child);
        }
    }

    _children = children;
    return true;
}

float Node::getPrior() {
    return _position->statistics().prior;
}

void Node::setPrior(float prior) {
    _position->statistics().prior = prior;
}

void Node::addPrior(float prior) {
    _position->statistics().prior = prior;
}

float Node::getUCTValue(Node& parent) const {
    const float winRate = winrate(parent.position()->actPlayer());
    const float parentVisits = static_cast<float>(parent.statistics().num_evaluations.load());
    const float nodeVisits = static_cast<float>(_statistics.num_evaluations.load());

    assert(parentVisits > 0);

    const float prior = _position->statistics().prior.load();
    return winRate + config::tree::priorC * prior * (sqrt(parentVisits) / (1 + nodeVisits));
}

const std::shared_ptr<Node>& Node::getBestUCTChild() {
    assert(isExpanded());
    assert(!(*_children).empty());

    float bestScore = std::numeric_limits<float>::lowest();
    size_t bestIdx = 0;
    for (size_t i = 0; i < (*_children).size(); ++i) {
        const auto& child = (*_children)[i];
        const auto score = child->getUCTValue(*this);
        if (score > bestScore) {
            bestScore = score;
            bestIdx = i;
        }
    }

    return (*_children)[bestIdx];
}

const std::shared_ptr<Node>& Node::getBestWinrateChild() {
    assert(isExpanded());
    assert(!(*_children).empty());

    float bestScore = std::numeric_limits<float>::lowest();
    size_t bestIdx = 0;
    for (size_t i = 0; i < (*_children).size(); ++i) {
        const auto& child = (*_children)[i];
        const auto score = child->winrate(position()->actPlayer());
        if (score > bestScore) {
            bestScore = score;
            bestIdx = i;
        }
    }

    return (*_children)[bestIdx];
}

const std::shared_ptr<Node>& Node::getMostVisitsChild() {
    assert(isExpanded());
    assert(!(*_children).empty());

    float bestScore = std::numeric_limits<float>::lowest();
    size_t bestIdx = 0;
    for (size_t i = 0; i < (*_children).size(); ++i) {
        const auto& child = (*_children)[i];
        const auto score = child->statistics().num_evaluations.load();
        if (score > bestScore) {
            bestScore = score;
            bestIdx = i;
        }
    }

    return (*_children)[bestIdx];
}

float Node::winrate(const Player& player) const {
    const auto& p = _position.get()->statistics();

    auto value = ((static_cast<float>(p.sum_value_evaluations.load()) /
                   std::max(1.f, static_cast<float>(p.num_evaluations.load()))) +
                  1.) /
                 2.;
    if (player == Player::White()) { value = 1.f - value; }
    return value;
}

void Node::addEvaluation(const float score) {
    atomic_addf(_position->statistics().sum_value_evaluations, score);

    _isEvaluated = true;
}
