#include "BatchProcessor.h"
#include "Node.h"
#include "Util.h"

void evaluationQueueConsumer(ConcurrentNodeQueue* evaluationQueue, std::atomic<bool>* keepRunning,
                             const argos::config::Config& configuration,
                             std::atomic<int>* threadsWaiting) {
    moodycamel::ConsumerToken token(*evaluationQueue);

    Network net(configuration.networkPath.string(), configuration);

    while (keepRunning->load()) {
        std::array<EvaluationJob, config::tree::batchSize> items;

        auto start = chrono::steady_clock::now();
        while (
            (evaluationQueue->size_approx() < config::tree::batchSize) &&
            (threadsWaiting->load() < (config::tree::batchSize - evaluationQueue->size_approx()))) {
            auto diff = chrono::steady_clock::now() - start;
            if (chrono::duration<double, milli>(diff).count() > 1) break;
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }

        const auto numItems =
            evaluationQueue->try_dequeue_bulk(token, items.begin(), config::tree::batchSize);

        if (numItems == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            continue;
        }

        std::array<NetworkFeatures::Planes, config::tree::batchSize> inputs;
        for (size_t i = 0; i < numItems; ++i) {
            inputs[i] = items[i].features;
        }

        const std::array<Network::Result, config::tree::batchSize> results = net.apply(inputs);
        for (size_t i = 0; i < numItems; ++i) {
            auto& promise = items[i].result;
            promise.set_value(results[i]);
        }
    }
}
