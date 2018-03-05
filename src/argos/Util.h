#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <random>
#include <string>

#include "ego.hpp"

class Node;

void printTree(Node* node, Player const& player, std::ostream& out = std::cout,
               float fractionPrint = 0.f);

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

// This implementation of Beta distribution is from
//  http://stackoverflow.com/questions/15165202/random-number-generator-with-beta-distribution
template <typename RealType = double>
class beta_distribution {
public:
    typedef RealType result_type;

    class param_type {
    public:
        typedef beta_distribution distribution_type;

        explicit param_type(RealType a = 2.0, RealType b = 2.0) : a_param(a), b_param(b) {}

        RealType a() const { return a_param; }
        RealType b() const { return b_param; }

        bool operator==(const param_type& other) const {
            return (a_param == other.a_param && b_param == other.b_param);
        }

        bool operator!=(const param_type& other) const { return !(*this == other); }

    private:
        RealType a_param, b_param;
    };

    explicit beta_distribution(RealType a = 2.0, RealType b = 2.0) : a_gamma(a), b_gamma(b) {}
    explicit beta_distribution(const param_type& param) : a_gamma(param.a()), b_gamma(param.b()) {}

    void reset() {}

    param_type param() const { return param_type(a(), b()); }

    void param(const param_type& param) {
        a_gamma = gamma_dist_type(param.a());
        b_gamma = gamma_dist_type(param.b());
    }

    template <typename URNG>
    result_type operator()(URNG& engine) {
        return generate(engine, a_gamma, b_gamma);
    }

    template <typename URNG>
    result_type operator()(URNG& engine, const param_type& param) {
        gamma_dist_type a_param_gamma(param.a()), b_param_gamma(param.b());
        return generate(engine, a_param_gamma, b_param_gamma);
    }

    result_type min() const { return 0.0; }
    result_type max() const { return 1.0; }

    result_type a() const { return a_gamma.alpha(); }
    result_type b() const { return b_gamma.alpha(); }

    bool operator==(const beta_distribution<result_type>& other) const {
        return (param() == other.param() && a_gamma == other.a_gamma && b_gamma == other.b_gamma);
    }

    bool operator!=(const beta_distribution<result_type>& other) const { return !(*this == other); }

private:
    typedef std::gamma_distribution<result_type> gamma_dist_type;

    gamma_dist_type a_gamma, b_gamma;

    template <typename URNG>
    result_type generate(URNG& engine, gamma_dist_type& x_gamma, gamma_dist_type& y_gamma) {
        result_type x = x_gamma(engine);
        return x / (x + y_gamma(engine));
    }
};
