#include "BatchProcessor.h"
#include "Config.h"
#include "Util.h"

void evaluationQueueConsumer(ConcurrentNodeQueue* evaluationQueue, std::atomic<bool>* keepRunning) {
    resetThreadAffinity();

    moodycamel::ConsumerToken token(*evaluationQueue);

    Network net(config::networkPath.string());

    while (keepRunning->load()) {
        std::array<EvaluationJob, config::tree::batchSize> items;

        const auto numItems =
            evaluationQueue->try_dequeue_bulk(token, items.begin(), config::tree::batchSize);

        if (numItems == 0) {
            // std::this_thread::sleep_for(std::chrono::microseconds(10));
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
