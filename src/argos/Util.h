#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string>

#include "ego.hpp"

class Node;
class Node;

void printTree(Node* node, Player const& player, std::ostream& out = std::cout,
               float fractionPrint = 0.15);

void printScoreEstimate(NatMap<Vertex, double> const& influence, double komi, std::ostream& out,
                        double threshold = .25);

/*
 * Perform an atomic addition to the float via spin-locking
 * on compare_exchange_weak. Memory ordering is release on write
 * consume on read
 */
inline float atomic_addf(std::atomic<float>& f, float d) {
    float old = f.load(std::memory_order_consume);
    float desired = old + d;
    while (!f.compare_exchange_weak(old, desired, std::memory_order_release,
                                    std::memory_order_consume)) {
        desired = old + d;
    }
    return desired;
}

template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <std::size_t N, typename FunctionType, std::size_t I>
class repeat_t {
public:
    repeat_t(FunctionType function) : function_(function) {}
    FunctionType operator()() {
        function_(I);
        return repeat_t<N, FunctionType, I + 1>(function_)();
    }

private:
    FunctionType function_;
};

template <std::size_t N, typename FunctionType>
class repeat_t<N, FunctionType, N> {
public:
    repeat_t(FunctionType function) : function_(function) {}
    FunctionType operator()() { return function_; }

private:
    FunctionType function_;
};

// https://stackoverflow.com/a/15275701
template <std::size_t N, typename FunctionType>
repeat_t<N, FunctionType, 0> repeat(FunctionType function) {
    return repeat_t<N, FunctionType, 0>(function);
}

// https://stackoverflow.com/a/42139394
template <class... Durations, class DurationIn>
std::tuple<Durations...> break_down_durations(DurationIn d) {
    std::tuple<Durations...> retval;
    using discard = int[];
    (void)discard{
        0, (void(((std::get<Durations>(retval) = std::chrono::duration_cast<Durations>(d)),
                  (d -= std::chrono::duration_cast<DurationIn>(std::get<Durations>(retval))))),
            0)...};
    return retval;
}

template <class D>
std::string format_duration(D duration) {
    auto clean_duration =
        break_down_durations<std::chrono::hours, std::chrono::minutes, std::chrono::seconds,
                             std::chrono::milliseconds>(duration);

    std::stringstream sstream;
    sstream << std::setfill('0') << std::setw(2) << std::get<0>(clean_duration).count() << ":"
            << std::setw(2) << std::get<1>(clean_duration).count() << ":" << std::setw(2)
            << std::get<2>(clean_duration).count() << "." << std::setw(3)
            << std::get<3>(clean_duration).count();
    return sstream.str();
}

void resetThreadAffinity();
