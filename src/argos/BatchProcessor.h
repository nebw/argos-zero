#pragma once

#include <moodycamel/ConcurrentQueue.h>
#include <future>
#include "Network.h"
#include "Config.h"

struct EvaluationJob {
    NetworkFeatures::Planes features;
    std::promise<Network::Result> result;

    EvaluationJob() {}
    EvaluationJob(NetworkFeatures::Planes&& planes) : features(planes) {}
};

typedef moodycamel::ConcurrentQueue<EvaluationJob> ConcurrentNodeQueue;

void evaluationQueueConsumer(ConcurrentNodeQueue* evaluationQueue,
                             std::atomic<bool>* keepRunning,
                             const argos::config::Config &configuration);
