#include "Position.h"

#include <mutex>

Position::Position(const Hash &hash, const RawBoard &board)
    : _transpositionHash(hash), _isExpanded({false}), _actPlayer(board.ActPlayer()) {}

const std::vector<Vertex> &Position::legalMoves(const Board &board) {
    if (_isExpanded.load()) return _legalMoves;

    std::lock_guard<SpinLock> lock(_expansionLock);
    if (_isExpanded.load()) return _legalMoves;

    expand(board);
    _isExpanded = true;

    return _legalMoves;
}

void Position::expand(const RawBoard &board) {
    assert(!_isExpanded);

    if (!board.BothPlayerPass()) {
        empty_v_for_each_and_pass(&board, v, {
            // TODO: superko nodes have to be removed from the tree later
            if (board.IsLegal(_actPlayer, v)) { _legalMoves.push_back(v); }
        });
    }
}
