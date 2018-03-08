#pragma once

#include <atomic>
#include <cassert>
#include "ego.hpp"

struct PositionStatistics {
    std::atomic<float> value = {0.f};

    std::atomic<float> sum_value_evaluations = {0};
    std::atomic<size_t> num_evaluations = {0};
};

struct NodeStatistics {
    std::atomic<float> prior = {0.f};

    std::atomic<size_t> num_evaluations = {0};
    std::atomic<float> playout_score = {0.f};
};
