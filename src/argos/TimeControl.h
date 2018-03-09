#pragma once

#include <chrono>
#include "Config.h"
#include "ego.hpp"

namespace {
    typedef std::chrono::milliseconds ms;
}

class TimeControl {
public:
    TimeControl(ms totalTime, const argos::config::Time &config) : _remainingTime(totalTime), _config(config) {}

    void setRemainingTime(ms remainingTime);

    ms getRemainingTime() const;

    template<typename F>
    void timedAction(F action) {
        using namespace std::chrono;
        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        action();
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        _remainingTime -= duration_cast<ms>(t2 - t1);
    }

    virtual ms getTimeForMove(Board const &board) = 0;

    virtual ~TimeControl() = default;

protected:
    ms _remainingTime;
    argos::config::Time _config;
};

class BasicTimeControl : public TimeControl {
public:
    BasicTimeControl(ms totalTime, const argos::config::Time &config) : TimeControl(totalTime, config) {}

    virtual ms getTimeForMove(Board const &board) override;

    virtual ~BasicTimeControl() override = default;
};
