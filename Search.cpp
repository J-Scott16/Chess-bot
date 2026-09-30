#include "Search.h"
#include "chess.hpp"
#include "Eval.h"
#include "Config.h"
#include <limits>
#include <cstdint>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <array>
#include <string>
#include <random>
#include <chrono>
#include <thread>
#include <future>
#include <iostream>
#include <fstream>
extern "C" {
#include "fathom/src/tbprobe.h"
}

namespace Search {

// -----------------------------------------------------------------------
// Opening Book Structures & Logic
// -----------------------------------------------------------------------
constexpr std::size_t TT_SIZE = 4'000'000;
constexpr int MAX_PLY = 64;



// -----------------------------------------------------------------------
// Globals & Time Management
// -----------------------------------------------------------------------

std::uint64_t nodeCount = 0;
int lastSearchScoreCp = 0;

int TIME = 5000; // fallback
int WHITE_TIME = 0;
int BLACK_TIME = 0;
int WHITE_INC = 0;
int BLACK_INC = 0;
int MOVES_TO_GO = 0;
bool USE_MOVETIME = false;






bool abortSearch = false;
std::chrono::steady_clock::time_point searchStartTime;
std::chrono::milliseconds searchTimeLimit;
static chess::Move killerMoves[MAX_PLY][2];
static int historyTable[2][64][64];

constexpr int TT_MOVE_SCORE         = 1'000'000;
constexpr int MVV_LVA_BASE          = 100'000;
constexpr int FIRST_KILLER_SCORE   = 90'000;
constexpr int SECOND_KILLER_SCORE  = 80'000;

struct PolyglotEntry {
    std::uint64_t key;
    std::uint16_t move;
    std::uint16_t weight;
    std::uint32_t learn;
};

int calculateTimeLimit(const chess::Board& board) {
    const bool white = board.sideToMove() == chess::Color::WHITE;

    int timeLeft = white ? WHITE_TIME : BLACK_TIME;
    int increment = white ? WHITE_INC : BLACK_INC;

    if (timeLeft <= 0)
        return 100;

    // If movestogo is provided, spread the remaining time.
    int movesToGo = MOVES_TO_GO > 0 ? MOVES_TO_GO : 30;

    // Base allocation + a portion of the increment.
    int allocated = timeLeft / movesToGo;
    allocated += increment * 3 / 4;

    // Never spend essentially all remaining time.
    allocated = std::min(allocated, timeLeft * 8 / 10);

    // Reasonable bounds.
    allocated = std::max(allocated, 50);

    return allocated;
}

bool initTablebases() {
    unsigned success = tb_init(Config::SYZYGY_PATH);;

    if (!success) {
        std::cerr << "info string Syzygy tablebases FAILED to load\n";
        return false;
    }

    std::cout << "info string Syzygy tablebases loaded\n";
    return true;
}

int getPieceCount(const chess::Board& board) {
    return static_cast<int>(board.occ().count());
}

std::uint64_t getPieces(
    const chess::Board& board,
    chess::PieceType type
) {
    return board.pieces(type).getBits();
}

std::uint64_t getColorPieces(
    const chess::Board& board,
    chess::Color color
) {
    return board.us(color).getBits();
}

int probeSyzygyWDL(chess::Board& board) {
    if (getPieceCount(board) > static_cast<int>(TB_LARGEST))
        return std::numeric_limits<int>::min();

    const std::uint64_t white =
        getColorPieces(board, chess::Color::WHITE);

    const std::uint64_t black =
        getColorPieces(board, chess::Color::BLACK);

    const std::uint64_t kings =
        getPieces(board, chess::PieceType::KING);

    const std::uint64_t queens =
        getPieces(board, chess::PieceType::QUEEN);

    const std::uint64_t rooks =
        getPieces(board, chess::PieceType::ROOK);

    const std::uint64_t bishops =
        getPieces(board, chess::PieceType::BISHOP);

    const std::uint64_t knights =
        getPieces(board, chess::PieceType::KNIGHT);

    const std::uint64_t pawns =
        getPieces(board, chess::PieceType::PAWN);

    unsigned result = tb_probe_wdl(
    white,
    black,
    kings,
    queens,
    rooks,
    bishops,
    knights,
    pawns,
    board.halfMoveClock(), // rule50
    0,                      // en passant
    0,                      // castling
    board.sideToMove() == chess::Color::WHITE
);

    if (result == TB_RESULT_FAILED)
        return std::numeric_limits<int>::min();

    switch (TB_GET_WDL(result)) {
        case TB_WIN:
            return Config::MATE_SCORE - 100;

        case TB_CURSED_WIN:
            return Config::MATE_SCORE - 200;

        case TB_DRAW:
            return 0;

        case TB_BLESSED_LOSS:
            return -Config::MATE_SCORE + 200;

        case TB_LOSS:
            return -Config::MATE_SCORE + 100;

        default:
            return std::numeric_limits<int>::min();
    }
}

chess::Move probeSyzygyRoot(chess::Board& board) {
    if (getPieceCount(board) > static_cast<int>(TB_LARGEST))
        return chess::Move::NO_MOVE;

    const std::uint64_t white =
        getColorPieces(board, chess::Color::WHITE);

    const std::uint64_t black =
        getColorPieces(board, chess::Color::BLACK);

    const std::uint64_t kings =
        getPieces(board, chess::PieceType::KING);

    const std::uint64_t queens =
        getPieces(board, chess::PieceType::QUEEN);

    const std::uint64_t rooks =
        getPieces(board, chess::PieceType::ROOK);

    const std::uint64_t bishops =
        getPieces(board, chess::PieceType::BISHOP);

    const std::uint64_t knights =
        getPieces(board, chess::PieceType::KNIGHT);

    const std::uint64_t pawns =
        getPieces(board, chess::PieceType::PAWN);

    unsigned result = tb_probe_root(
        white,
        black,
        kings,
        queens,
        rooks,
        bishops,
        knights,
        pawns,
        0, // rule50
        0, // castling
        0, // en passant
        board.sideToMove() == chess::Color::WHITE,
        nullptr
    );

    if (result == TB_RESULT_FAILED ||
        result == TB_RESULT_CHECKMATE ||
        result == TB_RESULT_STALEMATE) {
        return chess::Move::NO_MOVE;
    }

    int from = TB_GET_FROM(result);
    int to = TB_GET_TO(result);

    chess::Movelist moves;
    chess::movegen::legalmoves(moves, board);

    for (const auto& move : moves) {
        if (move.from().index() == from &&
            move.to().index() == to) {
            return move;
        }
    }

    return chess::Move::NO_MOVE;
}

std::string formatScoreForUci(int score) {
    if (score >= Config::MATE_SCORE - 1000) {
        int pliesToMate = Config::MATE_SCORE - score;
        int mateIn = (pliesToMate + 1) / 2;
        return "mate " + std::to_string(mateIn);
    } 
    else if (score <= -Config::MATE_SCORE + 1000) {
        int pliesToMate = Config::MATE_SCORE + score;
        int mateIn = (pliesToMate + 1) / 2;
        return "mate -" + std::to_string(mateIn);
    } 
    else {
        return "cp " + std::to_string(score);
    }
}

// Convert a Polyglot 16-bit encoded move into a chess.hpp Move object
chess::Move polyglotToChessMove(const chess::Board& board, std::uint16_t polyMove) {
    int fromFile = (polyMove >> 6) & 7;
    int fromRank = (polyMove >> 9) & 7;
    int toFile   = (polyMove >> 0) & 7;
    int toRank   = (polyMove >> 3) & 7;
    int promotion = (polyMove >> 12) & 7;

    chess::Square from(fromRank * 8 + fromFile);
    chess::Square to(toRank * 8 + toFile);

    chess::Movelist legalMoves;
    chess::movegen::legalmoves(legalMoves, board);

    for (const auto& m : legalMoves) {
        if (m.from() == from && m.to() == to) {
            if (promotion > 0) {
                if (m.typeOf() == chess::Move::PROMOTION) {
                    return m;
                }
            } else {
                return m;
            }
        }
    }
    return chess::Move::NO_MOVE;
}

std::uint64_t getPolyglotKey(const chess::Board& board) {
    return board.hash(); 
}

chess::Move getBookMove(chess::Board& board) {
    if constexpr (!Config::ENABLE_OPENING_BOOK) {
        return chess::Move::NO_MOVE;
    }

    std::ifstream file(Config::OPENING_BOOK_FILE, std::ios::binary);
    if (!file.is_open()) {
        return chess::Move::NO_MOVE; 
    }

    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    std::size_t numEntries = fileSize / sizeof(PolyglotEntry);
    file.seekg(0, std::ios::beg);

    std::uint64_t targetKey = getPolyglotKey(board);

    std::size_t left = 0;
    std::size_t right = numEntries > 0 ? numEntries - 1 : 0;
    
    std::vector<std::pair<chess::Move, int>> candidateMoves; 
    int foundIndex = -1;

    auto readBigEndian64 = [](std::ifstream& f) {
        std::uint64_t val = 0;
        for (int i = 0; i < 8; ++i) val = (val << 8) | f.get();
        return val;
    };
    auto readBigEndian16 = [](std::ifstream& f) {
        std::uint16_t val = 0;
        for (int i = 0; i < 2; ++i) val = (val << 8) | f.get();
        return val;
    };
    auto readBigEndian32 = [](std::ifstream& f) {
        std::uint32_t val = 0;
        for (int i = 0; i < 4; ++i) val = (val << 8) | f.get();
        return val;
    };

    while (left <= right) {
        std::size_t mid = left + (right - left) / 2;
        file.seekg(mid * sizeof(PolyglotEntry), std::ios::beg);

        std::uint64_t entryKey = readBigEndian64(file);

        if (entryKey == targetKey) {
            foundIndex = static_cast<int>(mid);
            break;
        } else if (entryKey < targetKey) {
            if (mid == 0) break;
            left = mid + 1;
        } else {
            if (mid == 0) break;
            right = mid - 1;
        }
    }

    if (foundIndex != -1) {
        int scanIdx = foundIndex;
        while (scanIdx >= 0) {
            file.seekg(scanIdx * sizeof(PolyglotEntry), std::ios::beg);
            std::uint64_t k = readBigEndian64(file);
            if (k != targetKey) break;

            std::uint16_t m = readBigEndian16(file);
            std::uint16_t w = readBigEndian16(file);
            
            chess::Move chessMove = polyglotToChessMove(board, m);
            if (chessMove != chess::Move::NO_MOVE) {
                candidateMoves.push_back({chessMove, w});
            }
            if (scanIdx == 0) break;
            scanIdx--;
        }

        scanIdx = foundIndex + 1;
        while (scanIdx < static_cast<int>(numEntries)) {
            file.seekg(scanIdx * sizeof(PolyglotEntry), std::ios::beg);
            std::uint64_t k = readBigEndian64(file);
            if (k != targetKey) break;

            std::uint16_t m = readBigEndian16(file);
            std::uint16_t w = readBigEndian16(file);
            
            chess::Move chessMove = polyglotToChessMove(board, m);
            if (chessMove != chess::Move::NO_MOVE) {
                candidateMoves.push_back({chessMove, w});
            }
            scanIdx++;
        }
    }

    file.close();

    if (!candidateMoves.empty()) {
        int totalWeight = 0;
        for (const auto& cm : candidateMoves) totalWeight += cm.second;

        if (totalWeight > 0) {
            static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
            std::uniform_int_distribution<int> dist(0, totalWeight - 1);
            int choice = dist(rng);

            int currentSum = 0;
            for (const auto& cm : candidateMoves) {
                currentSum += cm.second;
                if (choice < currentSum) {
                    std::cout << "info string Polyglot book move played." << std::endl;
                    return cm.first;
                }
            }
            return candidateMoves[0].first;
        }
    }

    return chess::Move::NO_MOVE;
}

// -----------------------------------------------------------------------
// Transposition Table & Core Structs
// -----------------------------------------------------------------------

enum TTFlag {
    TT_EXACT,
    TT_LOWERBOUND,
    TT_UPPERBOUND
};

struct TTEntry {
    std::uint64_t hash = 0;
    int depth = -1;
    int score = 0;
    chess::Move bestMove = chess::Move::NO_MOVE;
    TTFlag flag = TT_EXACT;
};

static TTEntry transpositionTable[TT_SIZE];

struct RootMove {
    chess::Move move;
    int score;
};



int hashfullPermille() {
    int filled = 0;
    for (std::size_t i = 0; i < 1000 && i < TT_SIZE; ++i) {
        if (transpositionTable[i].hash != 0) {
            filled++;
        }
    }
    return filled;
}



inline void checkTime() {
    if ((nodeCount & 2047) == 0) { 
        if (std::chrono::steady_clock::now() - searchStartTime >= searchTimeLimit) {
            abortSearch = true;
        }
    }
}

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

int pieceValue(chess::PieceType type) {
    switch (type.internal()) {
        case chess::PieceType::PAWN:   return 100;
        case chess::PieceType::KNIGHT: return 300;
        case chess::PieceType::BISHOP: return 300;
        case chess::PieceType::ROOK:   return 500;
        case chess::PieceType::QUEEN:  return 900;
        case chess::PieceType::KING:   return 10000;
        default: return 0;
    }
}

int mvvLvaScore(const chess::Board& board, const chess::Move& move) {
    if (!board.isCapture(move)) return 0;
    if (board.at(move.from()) == chess::Piece::NONE || board.at(move.to()) == chess::Piece::NONE) return 0;

    chess::Piece attacker = board.at(move.from());
    chess::Piece victim = board.at(move.to());

    return pieceValue(victim.type()) * 10 - pieceValue(attacker.type());
}

int scoreMove(
    const chess::Board& board, 
    const chess::Move& move, 
    const chess::Move& ttMove, 
    int ply
) {
    if (move.from().index() < 0 || move.from().index() >= 64 || move.to().index() < 0 || move.to().index() >= 64) {
        return 0;
    }

    if constexpr (Config::ENABLE_TRANSPOSITION_TABLE) {
        if (move == ttMove) {
            return TT_MOVE_SCORE;
        }
    }

    if (board.isCapture(move)) {
        return MVV_LVA_BASE + mvvLvaScore(board, move);
    }

    if constexpr (Config::ENABLE_KILLER_MOVES) {
        if (ply < MAX_PLY) {
            if (move == killerMoves[ply][0]) return FIRST_KILLER_SCORE;
            if (move == killerMoves[ply][1]) return SECOND_KILLER_SCORE;
        }
    }

    if constexpr (Config::ENABLE_HISTORY_HEURISTIC) {
        int color = static_cast<int>(board.sideToMove());
        int from = move.from().index();
        int to = move.to().index();
        return historyTable[color][from][to];
    }

    return 0;
}

// -----------------------------------------------------------------------
// Quiescence Search
// -----------------------------------------------------------------------

int quiescence(chess::Board& board, int alpha, int beta, int qdepth) {
    nodeCount++;
    checkTime();

    if (abortSearch)
        return 0;

    // Prevent runaway q-search.
    if (qdepth >= 8)
        return (board.sideToMove() == chess::Color::WHITE)
            ? Eval::evaluate(board)
            : -Eval::evaluate(board);

    const bool inCheck = board.inCheck();

    chess::Movelist moves;

    // If we're in check, we MUST search all legal evasions.
    // Stand-pat is illegal while in check.
    if (inCheck) {
        chess::movegen::legalmoves<
            chess::movegen::MoveGenType::ALL
        >(moves, board);
    } else {
        // Normal quiescence: captures only.
        chess::movegen::legalmoves<
            chess::movegen::MoveGenType::CAPTURE
        >(moves, board);

        const int rawEval = Eval::evaluate(board);
        const int standPat =
            (board.sideToMove() == chess::Color::WHITE)
                ? rawEval
                : -rawEval;

        if constexpr (Config::ENABLE_ALPHA_BETA) {
            if (standPat >= beta)
                return beta;

            if (standPat > alpha)
                alpha = standPat;
        } else {
            alpha = std::max(alpha, standPat);
        }
    }

    struct ScoredMove {
        chess::Move move;
        int score;
    };

    std::array<ScoredMove, 256> scoredMoves;
    int moveCount = 0;

    for (std::size_t i = 0; i < moves.size(); ++i) {
        const auto move = moves[i];

        if (board.at(move.from()) == chess::Piece::NONE)
            continue;

        int score = 0;

        if (inCheck) {
            // Prioritize captures when escaping check.
            if (move.typeOf() == chess::Move::NORMAL &&
                board.at(move.to()) != chess::Piece::NONE) {
                score = mvvLvaScore(board, move);
            }

            // Promotions are tactically important.
            if (move.typeOf() == chess::Move::PROMOTION)
                score += 10000;
        } else {
            score = mvvLvaScore(board, move);

            if (move.typeOf() == chess::Move::PROMOTION)
                score += 10000;
        }

        scoredMoves[moveCount++] = { move, score };
    }

    if constexpr (Config::ENABLE_MOVE_ORDERING) {
        std::sort(
            scoredMoves.begin(),
            scoredMoves.begin() + moveCount,
            [](const ScoredMove& a, const ScoredMove& b) {
                return a.score > b.score;
            }
        );
    }

    // If we're in check and have no legal moves, it's checkmate.
    if (inCheck && moveCount == 0) {
        return -Config::MATE_SCORE + qdepth;
    }

    for (int i = 0; i < moveCount; ++i) {
        const chess::Move move = scoredMoves[i].move;

        board.makeMove(move);

        const int score =
            -quiescence(board, -beta, -alpha, qdepth + 1);

        board.unmakeMove(move);

        if (score >= beta)
            return beta;

        if (score > alpha)
            alpha = score;
    }

    return alpha;
}

// -----------------------------------------------------------------------
// Negamax Search
// -----------------------------------------------------------------------

int negamax(chess::Board& board, int depth, int alpha, int beta, int ply) {
    nodeCount++;
    checkTime();
    if (getPieceCount(board) <= static_cast<int>(TB_LARGEST)) {
        int tbScore = probeSyzygyWDL(board);

        if (tbScore != std::numeric_limits<int>::min()) {
            return tbScore;
        }
    }
    if (abortSearch) return 0;
    chess::Movelist moves;
    chess::movegen::legalmoves(moves, board);

    if (moves.empty()) {
        if (board.inCheck()) {
            return -Config::MATE_SCORE + ply;
        }
        return 0;
        }

    if (depth <= 0) {
        if constexpr (Config::ENABLE_QUIESCENCE) {
            return quiescence(board, alpha, beta, 0);
        } else {
            int rawEval = Eval::evaluate(board);
            return (board.sideToMove() == chess::Color::WHITE) ? rawEval : -rawEval;
        }
    }

    const std::uint64_t hash = board.hash();
    const std::size_t index = hash % TT_SIZE;
    TTEntry& entry = transpositionTable[index];

    chess::Move ttMove = chess::Move::NO_MOVE;
    if constexpr (Config::ENABLE_TRANSPOSITION_TABLE) {
        if (entry.hash == hash) {
            ttMove = entry.bestMove;
            if (entry.depth >= depth) {
                if (entry.flag == TT_EXACT) return entry.score;
                if (entry.flag == TT_LOWERBOUND && entry.score >= beta) return entry.score;
                if (entry.flag == TT_UPPERBOUND && entry.score <= alpha) return entry.score;
            }
        }
    }

    const int originalAlpha = alpha;

    // Null-move pruning
if constexpr (Config::ENABLE_NULL_MOVE_PRUNING) {
    // Don't use null move in check, at very shallow depths,
    // or when we're near mate scores.
    if (depth >= 3 &&
        !board.inCheck() &&
        beta < 99000 &&
        alpha > -99000) {

        // Make sure there is non-pawn material.
        bool hasNonPawnMaterial = false;

        for (int sq = 0; sq < 64; ++sq) {
            chess::Piece piece = board.at(chess::Square(sq));

            if (piece != chess::Piece::NONE &&
                piece.color() == board.sideToMove() &&
                piece.type() != chess::PieceType::PAWN &&
                piece.type() != chess::PieceType::KING) {
                hasNonPawnMaterial = true;
                break;
            }
        }

        if (hasNonPawnMaterial) {
            board.makeNullMove();

            // R = 2 is a good conservative starting point.
            int reduction = 2;

            int score = -negamax(
                board,
                depth - 1 - reduction,
                -beta,
                -beta + 1,
                ply + 1
            );

            board.unmakeNullMove();

            if (abortSearch) return 0;

            if (score >= beta) {
                return score;
            }
        }
    }
}

    

    if (board.isRepetition() || board.isHalfMoveDraw() || board.isInsufficientMaterial()) {
        return 0;
    }

    if constexpr (Config::ENABLE_MOVE_ORDERING) {
        std::sort(
            moves.begin(),
            moves.end(),
            [&](const chess::Move& a, const chess::Move& b) {
                return scoreMove(board, a, ttMove, ply) > scoreMove(board, b, ttMove, ply);
            }
        );
    }

    int bestScore = std::numeric_limits<int>::min() + 1000;
    chess::Move bestMove = chess::Move::NO_MOVE;

    for (const auto& move : moves) {
        if (board.at(move.from()) == chess::Piece::NONE) continue;

        board.makeMove(move);
        int score = -negamax(board, depth - 1, -beta, -alpha, ply + 1);
        board.unmakeMove(move);

        if (abortSearch) return 0;

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
        if (score > alpha) {
            alpha = score;
        }

        if constexpr (Config::ENABLE_ALPHA_BETA) {
            if (alpha >= beta) {
                if (!board.isCapture(move) && ply < MAX_PLY) {
                    if constexpr (Config::ENABLE_KILLER_MOVES) {
                        if (killerMoves[ply][0] != move) {
                            killerMoves[ply][1] = killerMoves[ply][0];
                            killerMoves[ply][0] = move;
                        }
                    }

                    if constexpr (Config::ENABLE_HISTORY_HEURISTIC) {
                        int color = static_cast<int>(board.sideToMove());
                        int from = move.from().index();
                        int to = move.to().index();
                        if (from >= 0 && from < 64 && to >= 0 && to < 64) {
                            historyTable[color][from][to] += depth * depth;
                        }
                    }
                }
                break;
            }
        }
    }

    if constexpr (Config::ENABLE_TRANSPOSITION_TABLE) {
        if (!abortSearch) {
            TTFlag flag;
            if (bestScore <= originalAlpha) {
                flag = TT_UPPERBOUND;
            } else if (bestScore >= beta) {
                flag = TT_LOWERBOUND;
            } else {
                flag = TT_EXACT;
            }

            entry.hash = hash;
            entry.depth = depth;
            entry.score = bestScore;
            entry.bestMove = bestMove;
            entry.flag = flag;
        }
    }

    return bestScore;
}

// -----------------------------------------------------------------------
// Root Search (Iterative Deepening with Opening Book & 2s Hard Limit)
// -----------------------------------------------------------------------

chess::Move findBestMove(chess::Board& board, int maxDepthInput) {
    nodeCount = 0;
    abortSearch = false;

    // ---------------------------------------------------------------
    // 1. Syzygy tablebase
    // ---------------------------------------------------------------
    if (getPieceCount(board) <= static_cast<int>(TB_LARGEST)) {
        chess::Move tbMove = probeSyzygyRoot(board);

        if (tbMove != chess::Move::NO_MOVE) {
            std::cout << "info string Syzygy move played.\n";
            return tbMove;
        }
    }

    // ---------------------------------------------------------------
    // 2. Opening book
    // ---------------------------------------------------------------
    chess::Move bookMove = getBookMove(board);

    if (bookMove != chess::Move::NO_MOVE) {
        return bookMove;
    }

    // ---------------------------------------------------------------
    // 3. Generate legal moves
    // ---------------------------------------------------------------
    chess::Movelist moves;
    chess::movegen::legalmoves(moves, board);

    if (moves.empty()) {
        return chess::Move::NO_MOVE;
    }

    chess::Move bestOverallMove = moves[0];

    // ---------------------------------------------------------------
    // 4. Time management
    // ---------------------------------------------------------------
    searchStartTime = std::chrono::steady_clock::now();

    if (!USE_MOVETIME) {
        TIME = calculateTimeLimit(board);
    }

    // TIME is already set by handleGo() for movetime searches.
    searchTimeLimit = std::chrono::milliseconds(TIME);

    // ---------------------------------------------------------------
    // 5. Iterative deepening
    // ---------------------------------------------------------------
    for (int currentDepth = 1;
         currentDepth <= maxDepthInput;
         ++currentDepth) {

        if (abortSearch)
            break;

        const int INF = std::numeric_limits<int>::max() - 1000;

        std::vector<RootMove> evaluatedMoves;
        evaluatedMoves.reserve(moves.size());

        chess::Board searchBoard = board;

        // -----------------------------------------------------------
        // Search every root move
        // -----------------------------------------------------------
        for (const auto& move : moves) {

            if (abortSearch)
                break;

            const int from = move.from().index();

            if (from < 0 || from >= 64 ||
                searchBoard.at(move.from()) == chess::Piece::NONE) {
                continue;
            }

            searchBoard.makeMove(move);

            int score;

            if (searchBoard.isRepetition() ||
                searchBoard.isHalfMoveDraw() ||
                searchBoard.isInsufficientMaterial()) {

                score = 0;
            }
            else {
                score = -negamax(
                    searchBoard,
                    currentDepth - 1,
                    -INF,
                    INF,
                    1
                );
            }

            searchBoard.unmakeMove(move);

            if (abortSearch)
                break;

            evaluatedMoves.push_back({ move, score });
        }

        // -----------------------------------------------------------
        // Search was interrupted by time limit
        // -----------------------------------------------------------
        if (abortSearch || evaluatedMoves.empty())
            break;

        // -----------------------------------------------------------
        // Best root move
        // -----------------------------------------------------------
        std::sort(
            evaluatedMoves.begin(),
            evaluatedMoves.end(),
            [](const RootMove& a, const RootMove& b) {
                return a.score > b.score;
            }
        );

        bestOverallMove = evaluatedMoves.front().move;
        lastSearchScoreCp = evaluatedMoves.front().score;

        // -----------------------------------------------------------
        // UCI info
        // -----------------------------------------------------------
        const auto durationMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - searchStartTime
            ).count();

        std::cout
            << "info depth " << currentDepth
            << " score " << formatScoreForUci(lastSearchScoreCp)
            << " nodes " << nodeCount
            << " time " << durationMs
            << " hashfull " << hashfullPermille()
            << " pv " << chess::uci::moveToUci(bestOverallMove)
            << '\n';

        // -----------------------------------------------------------
        // Time limit reached
        // -----------------------------------------------------------
        if (durationMs >= TIME)
            break;
    }

    return bestOverallMove;
}

void clearTT() {
    for (auto& entry : transpositionTable) {
        entry = TTEntry();
    }
}

} // namespace Search