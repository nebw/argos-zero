#pragma once

#include "SpinLock.h"
#include "Statistics.h"
#include "ego.hpp"

class Position {
public:
    Position(Hash const& hash, RawBoard const& board);

    inline Player const& actPlayer() { return _actPlayer; }
    inline PositionStatistics& statistics() { return _statistics; }
    inline Hash const& transpositionHash() { return _transpositionHash; }
    std::vector<Vertex> const& legalMoves(const Board& board);

private:
    void expand(const RawBoard &board);

    Hash _transpositionHash;
    PositionStatistics _statistics;
    SpinLock _expansionLock;
    std::atomic<bool> _isExpanded;
    std::vector<Vertex> _legalMoves;
    Player _actPlayer;
};
