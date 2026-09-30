#pragma once

namespace Config {
    // --- Search Settings ---
    constexpr int MATE_SCORE = 100000;

    // --- Existing Config Switches ---
    constexpr bool ENABLE_ALPHA_BETA = true;
    constexpr bool ENABLE_QUIESCENCE = true;
    constexpr bool ENABLE_TRANSPOSITION_TABLE = true;
    constexpr bool ENABLE_MOVE_ORDERING = true;
    constexpr bool ENABLE_KILLER_MOVES = true;
    constexpr bool ENABLE_HISTORY_HEURISTIC = true;
    constexpr bool ENABLE_MATERIAL = true;
    constexpr bool ENABLE_PST = true;
    constexpr bool ENABLE_MOBILITY = true;
    constexpr bool ENABLE_TAPERED_PAWN_EVAL = true;
    constexpr bool ENABLE_RANDOM_TIEBREAK = true;
    constexpr bool ENABLE_NULL_MOVE_PRUNING = true;

    // --- Opening Book Settings ---
    constexpr bool ENABLE_OPENING_BOOK = true;
    const char* const OPENING_BOOK_FILE = "gm2001.bin";
    constexpr const char* SYZYGY_PATH =
    R"(D:\random projects\Chess bot\3-4-5endgame)";
}