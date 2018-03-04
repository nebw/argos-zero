#include "Tree.h"
#include "BatchProcessor.h"
#include "Config.h"
#include "Network.h"
#include "Node.h"
#include "Position.h"
#include "Util.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <random>
#include <thread>

Tree::Tree()
    : _evaluationQueue(
          ConcurrentNodeQueue(config::tree::batchSize * 4, 2 * config::tree::numThreads, 0)),
      _evaluationThreadKeepRunning(true),
      _evaluationThread(evaluationQueueConsumer, &_evaluationQueue, &_evaluationThreadKeepRunning),
      _gen(_rd()) {
    const auto position = maybeAddPosition(_rootBoard);
    _rootNode = std::make_shared<Node>(position, Vertex::Invalid());

    purgeTranspositionTable();

    moodycamel::ProducerToken token(_evaluationQueue);
    for (size_t i = 0; i < config::tree::numThreads; ++i) {
        EvaluationJob job(_rootBoard.getFeatures().getPlanes());
        auto future = job.result.get_future();
        _evaluationQueue.enqueue(token, std::move(job));
        future.wait();
    }
}

Tree::~Tree() {
    _evaluationThreadKeepRunning = {false};
    _evaluationThread.join();
}

std::shared_ptr<Position> Tree::maybeAddPosition(const RawBoard& board) {
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

void Tree::evaluate(const std::chrono::milliseconds duration) {
    const auto start = std::chrono::system_clock::now();
    beginEvaluation();

    std::atomic<bool> keepRunning = {true};

    std::vector<std::future<void>> threads;
    for (size_t i = 0; i < config::tree::numThreads; ++i) {
        auto f = std::async(std::launch::async, &Tree::playout, this, &keepRunning);
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

void Tree::evaluate(const size_t evaluations) {
    beginEvaluation();

    std::atomic<bool> keepRunning = {true};

    std::vector<std::future<void>> threads;
    for (size_t i = 0; i < config::tree::numThreads; ++i) {
        auto f = std::async(std::launch::async, &Tree::playout, this, &keepRunning);
        threads.push_back(std::move(f));
    }

    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (static_cast<int>(_rootNode->statistics().num_evaluations.load()) -
                static_cast<int>(config::tree::numThreads * config::tree::virtualPlayouts) >=
            static_cast<int>(evaluations)) {
            break;
        }
    }

    keepRunning = false;

    for (auto& t : threads) {
        t.get();
    }
}

void Tree::setKomi(float komi) { _rootBoard.SetKomi(komi); }

void Tree::visitNode(Node* node) {
    node->statistics().num_evaluations += config::tree::virtualPlayouts + 1;
    node->position()->statistics().num_evaluations += 1;
}

void Tree::playout(std::atomic<bool>* keepRunning) {
    resetThreadAffinity();

    moodycamel::ProducerToken token(_evaluationQueue);

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
        if (node->isExpanded() && node->isTerminal()) {
            updateStatistics(trace, node->statistics().playout_score.load());
            continue;
        }

        // expand node
        if (node->statistics().num_evaluations.load() >= config::tree::expandAt) {
            const bool isExpandingThread =
                node->expand(*this, playoutBoard, _evaluationQueue, token);
            if (isExpandingThread) {
                if (node->isTerminal()) {
                    updateStatistics(trace, node->statistics().playout_score.load());
                } else {
                    updateStatistics(trace, node->position()->statistics().value.load());
                }
            } else {
                while (!trace.IsEmpty()) {
                    Node* node = trace.PopTop();
                    node->statistics().num_evaluations -= config::tree::virtualPlayouts;
                    node->position()->statistics().num_evaluations -= 1;
                }
            }
        } else {
            assert(false);
            updateStatistics(trace, node->position()->statistics().value.load());
        }
    } while (keepRunning->load());
}

Player Tree::rollout(Board playoutBoard, ConcurrentNodeQueue& queue,
                     moodycamel::ProducerToken const& token) {
    while (!playoutBoard.BothPlayerPass()) {
        Player pl = playoutBoard.ActPlayer();

        EvaluationJob job(playoutBoard.getFeatures().getPlanes());
        auto future = job.result.get_future();
        queue.enqueue(token, std::move(job));
        const Network::Result result = future.get();

        std::vector<float> probabilites;
        std::vector<Vertex> moves;

        empty_v_for_each_and_pass(&playoutBoard, v, {
            if (playoutBoard.IsLegal(pl, v) && !playoutBoard.IsEyelike(pl, v)) {
                size_t posIdx;
                if (v == Vertex::Pass()) {
                    posIdx = config::boardSize * config::boardSize;
                    probabilites.push_back(result.candidates[posIdx].prior);
                } else {
                    posIdx = v.GetRow() * config::boardSize + v.GetColumn();
                    probabilites.push_back(result.candidates[posIdx].prior);
                }
                moves.push_back(v);
            }
        });

        std::discrete_distribution<size_t> d(probabilites.begin(), probabilites.end());
        const Vertex v = moves[d(_gen)];

        playoutBoard.PlayLegal(pl, v);
    }

    return playoutBoard.PlayoutWinner();
}

Vertex Tree::bestMove() {
    assert(_rootNode->isExpanded());
    if ((_rootBoard.MoveCount() < config::tree::randomizeFirstNMoves) &&
        config::tree::trainingMode) {
        std::vector<float> probabilites;
        probabilites.reserve(_rootNode->children().get().size());
        for (const std::shared_ptr<Node>& child : _rootNode->children().get()) {
            size_t NumEvaluations = child->statistics().num_evaluations.load();
            probabilites.push_back(NumEvaluations);
        }
        for (size_t i = 0; i < probabilites.size(); ++i) {
            probabilites[i] /= _rootNode->statistics().num_evaluations.load();
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

void Tree::playMove(const Vertex& vertex) {
    moodycamel::ProducerToken token(_evaluationQueue);

    if (!_rootNode->isExpanded()) { _rootNode->expand(*this, _rootBoard, _evaluationQueue, token); }
    assert(_rootBoard.IsLegal(_rootBoard.ActPlayer(), vertex));
    _rootBoard.PlayLegal(_rootBoard.ActPlayer(), vertex);

    setRootNode(vertex);

    purgeTranspositionTable();
}

void Tree::beginEvaluation() {
    // one shared token for tree
    moodycamel::ProducerToken token(_evaluationQueue);

    if (!_rootNode->isExpanded()) {
        visitNode(_rootNode.get());
        _rootNode->expand(*this, _rootBoard, _evaluationQueue, token);
        NodeTrace traceCpy;
        traceCpy.Push(_rootNode.get());
        updateStatistics(traceCpy, _rootNode->position()->statistics().value.load());
    }

    // Add dirichlet noise
    if (config::tree::trainingMode) { addDirichletNoise(0.25f, 0.03f); }
}

void Tree::addDirichletNoise(const float amount, const float distribution) {
    auto children = _rootNode->children().get();
    size_t child_cnt = children.size();

    auto dirichlet_vector = std::vector<float>{};

    std::gamma_distribution<float> gamma(distribution, 1.0f);

    for (size_t i = 0; i < child_cnt; i++) {
        dirichlet_vector.emplace_back(gamma(_gen));
    }

    auto sample_sum = std::accumulate(begin(dirichlet_vector), end(dirichlet_vector), 0.0f);
    // std::cout << "new vector" << endl;
    // std::cout << child_cnt << endl;
    for (auto& v : dirichlet_vector) {
        v /= sample_sum;
        // std::cout << v << std::endl;
    }

    for (size_t i = 0; i != child_cnt; i++) {
        // Add dirichlet distribution to each prior probability
        float prior = children[i]->getPrior();
        children[i]->setPrior(((1 - amount) * prior) + (amount * dirichlet_vector[i]));
    }
}

void Tree::updateStatistics(NodeTrace& trace, float score) const {
    while (!trace.IsEmpty()) {
        Node* node = trace.PopTop();
        node->addEvaluation(score);
        node->statistics().num_evaluations -= (config::tree::virtualPlayouts);
    }
}

void Tree::setRootNode(const Vertex& vertex) {
    _lastRootNodes.push(_rootNode);
    while (_lastRootNodes.size() > config::tree::numLastRootNodes) {
        _lastRootNodes.pop();
    }

    Node::NodeStack const& children = _rootNode->children().value();
#ifndef NDEBUG
    bool found = false;
#endif
    for (size_t i = 0; i < children.size(); ++i) {
        if (children[i]->parentMove() == vertex) {
            _rootNode = children[i];
#ifndef NDEBUG
            found = true;
#endif
            break;
        }
    }
    assert(found);
}

void Tree::purgeTranspositionTable() {
    auto lt = _transpositionTable.lock_table();
    for (const auto& it : lt) {
        if (it.second.expired()) { lt.erase(it.first); }
    }
}
