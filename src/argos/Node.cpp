#include "Node.h"
#include "BatchProcessor.h"
#include "Config.h"
#include "Position.h"
#include "Tree.h"
#include "Util.h"

#include <algorithm>
#include <cassert>

bool Node::expand(Tree& tree, Board& board, ConcurrentNodeQueue& queue,
                  moodycamel::ProducerToken const& token) {
    std::unique_lock<SpinLock> lock(_expandLock, std::try_to_lock);
    if (!lock.owns_lock()) {
        // if another thread is already expanding the node, don't bother waiting
        return false;
    }
    if (isExpanded()) return false;

    NodeStack children;
    if (!board.BothPlayerPass()) {
        EvaluationJob job(board.getFeatures().getPlanes());
        auto future = job.result.get_future();
        queue.enqueue(token, std::move(job));
        const Network::Result result = future.get();

        if (tree.configuration().tree.networkRollouts) {
            const float rolloutValue = tree.rollout(board, queue, token).ToScore();
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
                posIdx = config::boardSize * config::boardSize;
            } else {
                posIdx = vertex.GetRow() * config::boardSize + vertex.GetColumn();
            }
            child->setPrior(result.candidates[posIdx].prior);

            children.push_back(child);
        }

        if (legalMoves.empty()) { _isTerminalNode = true; }
    } else {
        _isTerminalNode = true;
    }

    _statistics.playout_score = {static_cast<float>(board.PlayoutWinner().ToScore())};

    _children = children;
    return true;
}

float Node::getPrior() { return statistics().prior; }

void Node::setPrior(float prior) { _statistics.prior = prior; }

float Node::getUCTValue(Node& parent) const {
    const float winRate = winrate(parent.position()->actPlayer());
    const float parentVisits = static_cast<float>(parent.statistics().num_evaluations.load());
    const float nodeVisits = static_cast<float>(_statistics.num_evaluations.load());

    assert(parentVisits > 0);

    const float prior = _statistics.prior.load();
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

    float value;
    if (_isTerminalNode) {
        value = (_statistics.playout_score.load() + 1.f) / 2.f;
    } else {
        const float sum_eval = static_cast<float>(p.sum_value_evaluations.load());
        const float num_eval = static_cast<float>(p.num_evaluations.load());
        value = ((sum_eval / std::max(1.f, num_eval)) + 1.f) / 2.f;
    }

    if (player == Player::White()) { value = 1.f - value; }

    return value;
}

void Node::addEvaluation(const float score) {
    atomic_addf(_position->statistics().sum_value_evaluations, score);

    _isEvaluated = true;
}
