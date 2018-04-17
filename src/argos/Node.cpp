#include "Node.h"
#include "BatchProcessor.h"
#include "Config.h"
#include "Position.h"
#include "Tree.h"
#include "Util.h"

#include <algorithm>
#include <cassert>

bool Node::expand(Tree& tree, Board& board, ConcurrentNodeQueue& queue,
                  moodycamel::ProducerToken const& token, HashTrace const& hashTrace) {
    std::unique_lock<std::mutex> lock(_expandLock, std::try_to_lock);
    if (!lock.owns_lock()) {
        RAIITempInc<int> inc(tree.threadsWaiting);
        std::unique_lock<std::mutex> locker(_expandLock);
    }
    if (isExpanded()) return false;

    size_t superkoMoves = 0;

    NodeStack children;
    if (!board.BothPlayerPass()) {
        EvaluationJob job(board.getFeatures().getPlanes());
        auto future = job.result.get_future();
        queue.enqueue(token, std::move(job));
        const Network::Result result = future.get();

        if (_config.networkRollouts) {
            const float rolloutValue =
                tree.rollout(board, queue, token).TrompTaylorWinner().ToScore();
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

            // don't add superko moves
            if (hashTrace.count(tempBoard.TranspositionHash().Data()) == 0) {
                auto position = tree.maybeAddPosition(tempBoard);
                auto child = std::make_shared<Node>(position, vertex, _config,
                                                    _position->statistics().value.load());

                size_t posIdx;
                if (vertex == Vertex::Pass()) {
                    posIdx = config::boardSize * config::boardSize;
                } else {
                    posIdx = vertex.GetRow() * config::boardSize + vertex.GetColumn();
                }
                child->setPrior(result.candidates[posIdx].prior);

                children.push_back(child);
            } else {
                superkoMoves += 1;
            }
        }

        if ((legalMoves.size() - superkoMoves) == 0) {
            _isTerminalNode = true;
            _statistics.playout_score = static_cast<float>(board.TrompTaylorWinner().ToScore());
        }
    } else {
        _isTerminalNode = true;
        _statistics.playout_score = static_cast<float>(board.TrompTaylorWinner().ToScore());
    }

    _children = children;
    return true;
}

float Node::getPrior() { return statistics().prior; }

void Node::setPrior(float prior) { _statistics.prior = prior; }

float Node::getUCTValue(Node& parent, std::mt19937&) const {
    const float winRate = winrate(parent.position()->actPlayer());
    const float parentVisits = static_cast<float>(parent.statistics().num_evaluations.load());
    const float nodeVisits = static_cast<float>(_statistics.num_evaluations.load());

    assert(parentVisits > 0);

    const float prior = _statistics.prior.load();
    return winRate + _config.priorC * prior * (sqrt(parentVisits) / (1 + nodeVisits));
}

float Node::getBetaValue(Node& parent, std::mt19937& engine) const {
    const float numPriorEvals = _config.betaPrior;
    const float prior = _statistics.prior.load();
    const float winRate = winrate(parent.position()->actPlayer());
    const float nodeVisits = static_cast<float>(_statistics.num_evaluations.load());

    auto beta =
        beta_distribution<float>(winRate * nodeVisits + numPriorEvals * prior,
                                 (1.f - winRate) * nodeVisits + numPriorEvals * (1.f - prior));

    return beta(engine);
}

const std::shared_ptr<Node>& Node::getBestUCTChild(std::mt19937& engine) {
    assert(isExpanded());
    assert(!(*_children).empty());

    float bestScore = std::numeric_limits<float>::lowest();
    size_t bestIdx = 0;
    for (size_t i = 0; i < (*_children).size(); ++i) {
        const auto& child = (*_children)[i];
        const auto score = child->getUCTValue(*this, engine);
        // const auto score = child->getBetaValue(*this, engine);
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
        value = _statistics.playout_score.load();
    } else {
        const float sum_eval = static_cast<float>(p.sum_value_evaluations.load());
        const float num_eval = static_cast<float>(p.num_evaluations.load());

        if (num_eval < 1.f) {
            value = _statistics.initial_value;
        } else {
            value = sum_eval / num_eval;
        }
    }
    value = (value + 1.f) / 2.f;

    if (player == Player::White()) { value = 1.f - value; }

    if (p.num_evaluations.load() == 0) {
        value = std::max<float>(0.f, value - _config.fpuReduction);
    }

    return value;
}

void Node::addEvaluation(const float score) {
    atomic_addf(_position->statistics().sum_value_evaluations, score);

    _isEvaluated = true;
}
