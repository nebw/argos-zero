#pragma once

#include <atomic>
#include <cassert>
#include "ego.hpp"

struct PositionStatistics {
    std::atomic<float> prior = {1e-8f};
    std::atomic<float> value = {1e-8f};

    std::atomic<float> sum_value_evaluations = {0};
    std::atomic<size_t> num_evaluations = {0};
};

struct NodeStatistics {
    std::atomic<size_t> num_evaluations = {0};
};
