#include "TimeControl.h"

#include "Config.h"

ms BasicTimeControl::getTimeForMove(const Board &board) {
    const int div =
        _config.C + std::max(_config.maxPly - static_cast<int>(board.MoveCount()), 0);
    return (_remainingTime / div) - _config.delay;
}

void TimeControl::setRemainingTime(ms remainingTime) { _remainingTime = remainingTime; }

ms TimeControl::getRemainingTime() const { return _remainingTime; }
