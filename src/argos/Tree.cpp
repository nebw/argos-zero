#include "Tree.h"
#include "Config.h"
#include "Network.h"
#include "Node.h"
#include "Position.h"
#include "Util.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <thread>

Tree::Tree()
    : _gen(_rd())
{
    const auto position = maybeAddPosition(_rootBoard);
    _rootNode = std::make_shared<Node>(position, Vertex::Invalid());

    purgeTranspositionTable();

    for (size_t i = 0; i < config::tree::numThreads; ++i) {
        _networks.emplace_back(config::networkPath.string());
        _networks[i].apply(_rootBoard);
    }
}

std::shared_ptr<Position> Tree::maybeAddPosition(const RawBoard& board)
{
    const auto hash = board.TranspositionHash();

    std::shared_ptr<Position> pos;
    if (_transpositionTable.find_fn(
            hash, [&pos](std::weak_ptr<Position> const& entry) { pos = entry.lock(); })) {
        assert(pos);

        return pos;
    }

    const auto position = std::make_shared<Position>(hash, board);
    const std::weak_ptr<Position> position_weak = position;
    _transpositionTable.insert(std::move(hash), std::move(position_weak));

    return position;
}

void Tree::evaluate(const std::chrono::milliseconds duration)
{
    const auto start = std::chrono::system_clock::now();
    beginEvaluation();

    std::atomic<bool> keepRunning = { true };

    std::vector<std::future<void>> threads;
    for (size_t i = 0; i < config::tree::numThreads; ++i) {
        Network* net = &_networks[i];
        auto f = std::async(std::launch::async, &Tree::playout, this, &keepRunning, net);
        threads.push_back(std::move(f));
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start);
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start);
    } while (elapsed < duration);

    keepRunning = false;

    for (auto& t : threads) {
        t.get();
    }
}

void Tree::evaluate(const size_t evaluations)
{
    beginEvaluation();

    std::atomic<bool> keepRunning = { true };

    std::vector<std::future<void>> threads;
    for (size_t i = 0; i < config::tree::numThreads; ++i) {
        Network* net = &_networks[i];
        auto f = std::async(std::launch::async, &Tree::playout, this, &keepRunning, net);
        threads.push_back(std::move(f));
    }

    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (static_cast<int>(_rootNode->statistics().num_evaluations.load()) - static_cast<int>(config::tree::numThreads * config::tree::virtualPlayouts) >= static_cast<int>(evaluations))
        {
            break;
        }
    }

    keepRunning = false;

    for (auto& t : threads) {
        t.get();
    }
}

void Tree::setKomi(float komi) { _rootBoard.SetKomi(komi); }

void Tree::visitNode(Node* node)
{
    node->statistics().num_evaluations += config::tree::virtualPlayouts;
    node->position()->statistics().num_evaluations += 1;
}

void Tree::playout(std::atomic<bool>* keepRunning, Network* net)
{
    do {
        NodeTrace trace;
        Board playoutBoard;
        playoutBoard.Load(_rootBoard);

        Node* node = _rootNode.get();
        visitNode(node);
        trace.Push(node);

        while (node->isExpanded() && node->isEvaluated() && !(node->children())->empty()) {
            node = node->getBestUCTChild().get();
            visitNode(node);
            playoutBoard.PlayLegal(playoutBoard.ActPlayer(), node->parentMove());
            trace.Push(node);
        }

        // if node is terminal
        if (node->isExpanded() && (node->children())->empty()) {
            updateStatistics(trace, playoutBoard.PlayoutWinner().ToScore());
            continue;
        }

        // expand node
        if (node->statistics().num_evaluations.load() >= config::tree::expandAt) {
            const bool isExpandingThread = node->expand(*this, playoutBoard, *net);
            if (isExpandingThread) {
                updateStatistics(trace, node->position()->statistics().value.load());
            } else {
                while (!trace.IsEmpty()) {
                    Node* node = trace.PopTop();
                    node->statistics().num_evaluations -= config::tree::virtualPlayouts;
                    node->position()->statistics().num_evaluations -= 1;
                }
            }
        } else {
            updateStatistics(trace, node->position()->statistics().value.load());
        }
    } while (keepRunning->load());
}

Player Tree::rollout(Board playoutBoard, Network* net)
{
    while (!playoutBoard.BothPlayerPass()) {
        Player pl = playoutBoard.ActPlayer();

        const Network::Result result = net->apply(playoutBoard);

        std::vector<float> probabilites;
        std::vector<Vertex> moves;

        empty_v_for_each_and_pass(&playoutBoard, v, {
            if (playoutBoard.IsLegal(pl, v) && !playoutBoard.IsEyelike(pl, v)) {
                size_t posIdx;
                if (v == Vertex::Pass()) {
                    posIdx = 19 * 19;
                } else {
                    posIdx = v.GetRow() * 19 + v.GetColumn();
                }
                probabilites.push_back(result.candidates[posIdx].prior);
                moves.push_back(v);
            }
        });

        std::discrete_distribution<size_t> d(probabilites.begin(), probabilites.end());
        const Vertex v = moves[d(_gen)];

        playoutBoard.PlayLegal(pl, v);
    }

    return playoutBoard.PlayoutWinner();
}

Vertex Tree::bestMove()
{
    assert(_rootNode->isExpanded());

    if (_rootBoard.MoveCount() < config::tree::randomizeFirstNMoves) {
        std::vector<float> probabilites;
        probabilites.reserve(_rootNode->children().get().size());
        for (const std::shared_ptr<Node>& child : _rootNode->children().get()) {
            probabilites.push_back(child->statistics().num_evaluations.load());
        }
        std::discrete_distribution<size_t> d(probabilites.begin(), probabilites.end());

        return _rootNode->children().get()[d(_gen)]->parentMove();
    } else {
        Node::NodeStack const& children = _rootNode->children().value();
        size_t bestIdx = 0;
        float maxRollouts = -1.f;
        for (size_t i = 0; i < children.size(); ++i) {
            if (children[i]->statistics().num_evaluations > maxRollouts) {
                maxRollouts = children[i]->statistics().num_evaluations;
                bestIdx = i;
            }
        }

        return children[bestIdx]->parentMove();
    }
}

void Tree::playMove(const Vertex& vertex)
{
    if (!_rootNode->isExpanded()) {
        _rootNode->expand(*this, _rootBoard, _networks[0]);
    }
    assert(_rootBoard.IsLegal(_rootBoard.ActPlayer(), vertex));
    _rootBoard.PlayLegal(_rootBoard.ActPlayer(), vertex);

    setRootNode(vertex);
    purgeTranspositionTable();
}

void Tree::beginEvaluation()
{
    if (!_rootNode->isExpanded()) {
        visitNode(_rootNode.get());
        _rootNode->expand(*this, _rootBoard, _networks[0]);
        NodeTrace traceCpy;
        traceCpy.Push(_rootNode.get());
        updateStatistics(traceCpy, _rootNode->position()->statistics().value.load());
    }
}

void Tree::updateStatistics(NodeTrace& trace, float score) const
{
    while (!trace.IsEmpty()) {
        Node* node = trace.PopTop();
        node->addEvaluation(score);
        node->statistics().num_evaluations -= (config::tree::virtualPlayouts - 1);
    }
}

void Tree::setRootNode(const Vertex& vertex)
{
    _lastRootNodes.push(_rootNode);
    while (_lastRootNodes.size() > config::tree::numLastRootNodes) {
        _lastRootNodes.pop();
    }

    Node::NodeStack const& children = _rootNode->children().value();
#ifdef DEBUG
    bool found = false;
#endif
    for (size_t i = 0; i < children.size(); ++i) {
        if (children[i]->parentMove() == vertex) {
            _rootNode = children[i];
#ifdef DEBUG
            found = true;
#endif
            break;
        }
    }
    assert(found);
}

void Tree::purgeTranspositionTable()
{
    auto lt = _transpositionTable.lock_table();
    for (const auto& it : lt) {
        if (it.second.expired()) {
            lt.erase(it.first);
        }
    }
}
