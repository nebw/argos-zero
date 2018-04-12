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

Tree::Tree(const argos::config::Config& config)
    : _config(config),
      _evaluationQueue(
          ConcurrentNodeQueue(config::tree::batchSize * 4, 1 + 2 * _config.tree.numThreads, 0)),
      _token(_evaluationQueue),
      _evaluationThreadKeepRunning(true),
      _gen(_rd()) {
    for (size_t i = 0; i < config.tree.numEvaluationThreads; ++i) {
        _evaluationThreads.emplace_back(evaluationQueueConsumer, &_evaluationQueue,
                                        &_evaluationThreadKeepRunning, _config);
    }

    const auto position = maybeAddPosition(_rootBoard);

    purgeTranspositionTable();

    float rootValue = 0.f;
    for (size_t i = 0; i < _config.tree.numThreads; ++i) {
        EvaluationJob job(_rootBoard.getFeatures().getPlanes());
        auto future = job.result.get_future();
        _evaluationQueue.enqueue(_token, std::move(job));
        const Network::Result result = future.get();
        rootValue = result.value;
    }

    _rootNode = std::make_shared<Node>(position, Vertex::Invalid(), config.tree, rootValue);
}

Tree::~Tree() {
    _evaluationThreadKeepRunning = {false};

    for (size_t i = 0; i < _config.tree.numEvaluationThreads; ++i) {
        _evaluationThreads[i].join();
    }
}

std::shared_ptr<Position> Tree::maybeAddPosition(const RawBoard& board) {
    const auto hash = board.TranspositionHash();

    /*
    std::shared_ptr<Position> pos;
    if (_transpositionTable.find_fn(
            hash, [&pos](std::weak_ptr<Position> const& entry) { pos = entry.lock(); })) {
        assert(pos);

        return pos;
    }
    */

    const auto position = std::make_shared<Position>(hash, board);

    /*
    const std::weak_ptr<Position> position_weak = position;
    _transpositionTable.insert(std::move(hash), std::move(position_weak));
    */

    return position;
}

void Tree::evaluate(const std::chrono::milliseconds duration) {
    const auto start = std::chrono::system_clock::now();
    beginEvaluation();

    std::atomic<bool> keepRunning = {true};

    std::vector<std::future<void>> threads;
    for (size_t i = 0; i < _config.tree.numThreads; ++i) {
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
    for (size_t i = 0; i < _config.tree.numThreads; ++i) {
        auto f = std::async(std::launch::async, &Tree::playout, this, &keepRunning);
        threads.push_back(std::move(f));
    }

    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (static_cast<int>(_rootNode->statistics().num_evaluations.load()) -
                static_cast<int>(_config.tree.numThreads * _config.tree.virtualPlayouts) >=
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
    node->statistics().num_evaluations += _config.tree.virtualPlayouts + 1;
    node->position()->statistics().num_evaluations += 1;
}

void Tree::playout(std::atomic<bool>* keepRunning) {
    moodycamel::ProducerToken token(_evaluationQueue);
    std::mt19937 randomEngine(std::time(0));

    do {
        NodeTrace trace;
        Board playoutBoard;
        playoutBoard.Load(_rootBoard);

        Node* node = _rootNode.get();
        visitNode(node);
        trace.Push(node);
        HashTrace playoutTrace(_hashTrace);

        while (true) {
            while (node->isExpanded() && node->isEvaluated() && !(node->children())->empty()) {
                node = node->getBestUCTChild(randomEngine).get();
                playoutBoard.PlayLegal(playoutBoard.ActPlayer(), node->parentMove());
                visitNode(node);
                trace.Push(node);
                playoutTrace.insert(playoutBoard.TranspositionHash().Data());
            }

            // if node is terminal
            if (node->isExpanded() && node->isTerminal()) {
                // we don't need to evaluate terminal nodes, but this introduces a delay that
                // prevents biasing the tree search towards terminal nodes due to faster sampling
                EvaluationJob job(playoutBoard.getFeatures().getPlanes());
                auto future = job.result.get_future();
                _evaluationQueue.enqueue(token, std::move(job));
                future.wait();

                updateStatistics(trace, node->statistics().playout_score.load());
                break;
            }

            // expand node
            const bool isExpandingThread =
                node->expand(*this, playoutBoard, _evaluationQueue, token, playoutTrace);
            if (isExpandingThread) {
                if (node->isTerminal()) {
                    updateStatistics(trace, node->statistics().playout_score.load());
                    break;
                } else {
                    updateStatistics(trace, node->position()->statistics().value.load());
                    break;
                }
            }
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
                    probabilites.push_back(0.f);
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

    return playoutBoard.TrompTaylorWinner();
}

Vertex Tree::bestMove() {
    assert(_rootNode->isExpanded());
    if ((_rootBoard.MoveCount() < _config.tree.randomizeFirstNMoves) && _config.tree.trainingMode) {
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

    if (!_rootNode->isExpanded()) {
        _rootNode->expand(*this, _rootBoard, _evaluationQueue, token, _hashTrace);
    }
    assert(_rootBoard.IsLegal(_rootBoard.ActPlayer(), vertex));
    _rootBoard.PlayLegal(_rootBoard.ActPlayer(), vertex);

    setRootNode(vertex);
    _hashTrace.insert(_rootBoard.TranspositionHash().Data());

    purgeTranspositionTable();
}

void Tree::beginEvaluation() {
    if (!_rootNode->isExpanded()) {
        _rootNode->expand(*this, _rootBoard, _evaluationQueue, _token, _hashTrace);
        NodeTrace traceCpy;
        traceCpy.Push(_rootNode.get());
        updateStatistics(traceCpy, _rootNode->position()->statistics().value.load());
    }

    // Add dirichlet noise
    if (_config.tree.trainingMode) { addDirichletNoise(0.25f, 0.03f); }
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
        if (children[i]->parentMove() != Vertex::Pass()) {
            // Add dirichlet distribution to each prior probability
            float prior = children[i]->getPrior();
            children[i]->setPrior(((1 - amount) * prior) + (amount * dirichlet_vector[i]));
        }
    }
}

void Tree::updateStatistics(NodeTrace& trace, float score) {
    while (!trace.IsEmpty()) {
        Node* node = trace.PopTop();
        // visitNode(node);
        node->addEvaluation(score);
        node->statistics().num_evaluations -= (_config.tree.virtualPlayouts);
    }
}

void Tree::setRootNode(const Vertex& vertex) {
    _lastRootNodes.push(_rootNode);
    while (_lastRootNodes.size() > _config.tree.numLastRootNodes) {
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
