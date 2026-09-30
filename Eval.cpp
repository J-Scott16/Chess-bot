#include "Eval.h"
#include "Config.h"
#include "chess.hpp"
#include <array>

namespace Eval {

// -----------------------------------------------------------------------
// Piece Values (Centipawns)
// -----------------------------------------------------------------------
constexpr int PAWN_VALUE   = 100;
constexpr int KNIGHT_VALUE = 320;
constexpr int BISHOP_VALUE = 330;
constexpr int ROOK_VALUE   = 500;
constexpr int QUEEN_VALUE  = 900;

constexpr int MAX_NON_PAWN_MATERIAL = 24;

// -----------------------------------------------------------------------
// Piece-Square Tables (White perspective, rank 1 to 8)
// -----------------------------------------------------------------------
constexpr std::array<int, 64> PawnPST = {
      0,  0,  0,  0,  0,  0,  0,  0,
     50, 50, 50, 50, 50, 50, 50, 50,
     10, 10, 20, 30, 30, 20, 10, 10,
      5,  5, 10, 25, 25, 10,  5,  5,
      0,  0,  0, 20, 20,  0,  0,  0,
      5, -5,-10,  0,  0,-10, -5,  5,
      5, 10, 10,-20,-20, 10, 10,  5,
      0,  0,  0,  0,  0,  0,  0,  0
};

constexpr std::array<int, 64> KnightPST = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

constexpr std::array<int, 64> BishopPST = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

constexpr std::array<int, 64> RookPST = {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0
};

constexpr std::array<int, 64> QueenPST = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

constexpr std::array<int, 64> KingMiddlegamePST = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

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
) {
    while (bb) {
        int sq = bb.pop(); 

        if constexpr (Config::ENABLE_MATERIAL) {
            material += baseValue;
        }

        if constexpr (Config::ENABLE_PST) {
            int pstIndex = (color == chess::Color::WHITE) ? sq : (sq ^ 56);
            pstScore += pst[pstIndex];
        }

        if constexpr (Config::ENABLE_MOBILITY) {
            chess::Bitboard attacks;
            if (pt == chess::PieceType::KNIGHT) {
                attacks = chess::attacks::knight(chess::Square(sq));
                mobilityScore += attacks.count();
            } else if (pt == chess::PieceType::BISHOP) {
                attacks = chess::attacks::bishop(chess::Square(sq), board.occ());
                mobilityScore += attacks.count();
            } else if (pt == chess::PieceType::ROOK) {
                attacks = chess::attacks::rook(chess::Square(sq), board.occ());
                mobilityScore += attacks.count();
            } else if (pt == chess::PieceType::QUEEN) {
                attacks = chess::attacks::queen(chess::Square(sq), board.occ());
                mobilityScore += attacks.count();
            }
        }
    }
}

int evaluate(const chess::Board& board) {
    int whiteMaterial = 0, blackMaterial = 0;
    int whitePST = 0,      blackPST = 0;
    int whiteMobility = 0, blackMobility = 0;

    // 1. WHITE PIECES
    evaluatePieceType<chess::Color::WHITE>(
        board.pieces(chess::PieceType::PAWN, chess::Color::WHITE),
        PAWN_VALUE, PawnPST, whiteMaterial, whitePST, whiteMobility, board, chess::PieceType::PAWN
    );
    evaluatePieceType<chess::Color::WHITE>(
        board.pieces(chess::PieceType::KNIGHT, chess::Color::WHITE),
        KNIGHT_VALUE, KnightPST, whiteMaterial, whitePST, whiteMobility, board, chess::PieceType::KNIGHT
    );
    evaluatePieceType<chess::Color::WHITE>(
        board.pieces(chess::PieceType::BISHOP, chess::Color::WHITE),
        BISHOP_VALUE, BishopPST, whiteMaterial, whitePST, whiteMobility, board, chess::PieceType::BISHOP
    );
    evaluatePieceType<chess::Color::WHITE>(
        board.pieces(chess::PieceType::ROOK, chess::Color::WHITE),
        ROOK_VALUE, RookPST, whiteMaterial, whitePST, whiteMobility, board, chess::PieceType::ROOK
    );
    evaluatePieceType<chess::Color::WHITE>(
        board.pieces(chess::PieceType::QUEEN, chess::Color::WHITE),
        QUEEN_VALUE, QueenPST, whiteMaterial, whitePST, whiteMobility, board, chess::PieceType::QUEEN
    );

    // 2. BLACK PIECES
    evaluatePieceType<chess::Color::BLACK>(
        board.pieces(chess::PieceType::PAWN, chess::Color::BLACK),
        PAWN_VALUE, PawnPST, blackMaterial, blackPST, blackMobility, board, chess::PieceType::PAWN
    );
    evaluatePieceType<chess::Color::BLACK>(
        board.pieces(chess::PieceType::KNIGHT, chess::Color::BLACK),
        KNIGHT_VALUE, KnightPST, blackMaterial, blackPST, blackMobility, board, chess::PieceType::KNIGHT
    );
    evaluatePieceType<chess::Color::BLACK>(
        board.pieces(chess::PieceType::BISHOP, chess::Color::BLACK),
        BISHOP_VALUE, BishopPST, blackMaterial, blackPST, blackMobility, board, chess::PieceType::BISHOP
    );
    evaluatePieceType<chess::Color::BLACK>(
        board.pieces(chess::PieceType::ROOK, chess::Color::BLACK),
        ROOK_VALUE, RookPST, blackMaterial, blackPST, blackMobility, board, chess::PieceType::ROOK
    );
    evaluatePieceType<chess::Color::BLACK>(
        board.pieces(chess::PieceType::QUEEN, chess::Color::BLACK),
        QUEEN_VALUE, QueenPST, blackMaterial, blackPST, blackMobility, board, chess::PieceType::QUEEN
    );

    // 3. KING EVALUATION
    int wKingSq = board.kingSq(chess::Color::WHITE).index();
    int bKingSq = board.kingSq(chess::Color::BLACK).index();

    if constexpr (Config::ENABLE_PST) {
        whitePST += KingMiddlegamePST[wKingSq];
        blackPST += KingMiddlegamePST[bKingSq ^ 56];
    }

    // 4. TAPERED PAWN EVALUATION
    if constexpr (Config::ENABLE_TAPERED_PAWN_EVAL) {
        int whiteNonPawn = (whiteMaterial - (board.pieces(chess::PieceType::PAWN, chess::Color::WHITE).count() * PAWN_VALUE));
        int blackNonPawn = (blackMaterial - (board.pieces(chess::PieceType::PAWN, chess::Color::BLACK).count() * PAWN_VALUE));
        
        int currentPhase = (whiteNonPawn + blackNonPawn) / 300; 
        if (currentPhase > MAX_NON_PAWN_MATERIAL) currentPhase = MAX_NON_PAWN_MATERIAL;

        int endgameWeight = MAX_NON_PAWN_MATERIAL - currentPhase;
        
        chess::Bitboard wPawns = board.pieces(chess::PieceType::PAWN, chess::Color::WHITE);
        while (wPawns) {
            int sq = wPawns.pop();
            int rank = sq / 8;
            whiteMaterial += (rank * endgameWeight * 2);
        }

        chess::Bitboard bPawns = board.pieces(chess::PieceType::PAWN, chess::Color::BLACK);
        while (bPawns) {
            int sq = bPawns.pop();
            int rank = 7 - (sq / 8);
            blackMaterial += (rank * endgameWeight * 2);
        }
    }

    // Absolute score relative to White
    int totalWhite = whiteMaterial + whitePST + (whiteMobility * 4);
    int totalBlack = blackMaterial + blackPST + (blackMobility * 4);

    return totalWhite - totalBlack;
}

} // namespace Eval