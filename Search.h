#pragma once
#include "chess.hpp"
#include <cstdint>

namespace Search {

extern std::uint64_t nodeCount;
extern int lastSearchScoreCp;

extern int TIME;
extern int WHITE_TIME;
extern int BLACK_TIME;
extern int WHITE_INC;
extern int BLACK_INC;
extern int MOVES_TO_GO;
extern bool USE_MOVETIME;

chess::Move findBestMove(chess::Board& board, int maxDepthInput);

chess::Move findBestMove(chess::Board& board, int maxDepth);
int quiescence(chess::Board& board, int alpha, int beta, int qdepth = 0);
int negamax(chess::Board& board, int depth, int alpha, int beta, int ply);
void clearTT();
bool initTablebases();
int probeSyzygy(chess::Board& board);
} // namespace Search