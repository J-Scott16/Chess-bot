// Eval.h
#pragma once

#include "chess.hpp"

namespace Eval {

// Returns a static evaluation of the position, in centipawns (or whatever
// unit you choose), from the perspective of the side to move.
// Positive = good for side to move, negative = bad.
int evaluate(const chess::Board& board);


template <chess::Color::underlying color>
inline void evaluatePieceType(
    chess::Bitboard bb,
    int baseValue,
    const std::array<int, 64>& pst,
    int& material,
    int& pstScore,
    int& mobilityScore,
    const chess::Board& board,
    chess::PieceType pt
);
} // namespace Eval
