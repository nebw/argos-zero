#include "TimeControl.h"

#include "Config.h"

ms BasicTimeControl::getTimeForMove(const Board &board) {
    const int div =
        config::time::C + std::max(config::time::maxPly - static_cast<int>(board.MoveCount()), 0);
    return (_remainingTime / div) - config::time::delay;
}

void TimeControl::setRemainingTime(ms remainingTime) { _remainingTime = remainingTime; }

ms TimeControl::getRemainingTime() const { return _remainingTime; }
