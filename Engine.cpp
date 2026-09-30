#include "chess.hpp"
#include "Search.h"
#include "Eval.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>
#include <chrono>

using namespace chess;

static Board board;
static std::uint64_t totalNodes = 0;
static std::uint64_t totalTimeMs = 0;
static int DEFAULT_DEPTH = 6;


void handleUci() {
    std::cout << "id name HollowShellEngine\n";
    std::cout << "id author YourNameHere\n";
    std::cout << "uciok\n" << std::flush;
}

void handleIsReady() {
    std::cout << "readyok\n" << std::flush;
}

void handleUciNewGame() {
    board = Board();
    Search::clearTT();
}

void handlePosition(std::istringstream& iss) {
    std::string token;
    iss >> token;

    if (token == "startpos") {
        board = Board();
        iss >> token; 
    } else if (token == "fen") {
        std::string fen;
        std::string part;
        while (iss >> part && part != "moves") {
            if (!fen.empty()) fen += " ";
            fen += part;
        }
        board = Board(fen);
        token = part;
    }

    if (token == "moves") {
        std::string moveStr;
        while (iss >> moveStr) {
            Move m = uci::uciToMove(board, moveStr);
            if (m == Move::NO_MOVE) {
                break;
            }
            board.makeMove(m);
        }
    }
}

void handleGo(std::istringstream& iss) {
    int searchDepth = 64;

    int moveTimeMs = 0;
    int wtime = 0;
    int btime = 0;
    int winc = 0;
    int binc = 0;
    int movestogo = 0;

    std::string token;

    while (iss >> token) {
        if (token == "depth") {
            iss >> searchDepth;
        }
        else if (token == "movetime") {
            iss >> moveTimeMs;
        }
        else if (token == "wtime") {
            iss >> wtime;
        }
        else if (token == "btime") {
            iss >> btime;
        }
        else if (token == "winc") {
            iss >> winc;
        }
        else if (token == "binc") {
            iss >> binc;
        }
        else if (token == "movestogo") {
            iss >> movestogo;
        }
    }

    // Store clock information.
    Search::WHITE_TIME = wtime;
    Search::BLACK_TIME = btime;
    Search::WHITE_INC = winc;
    Search::BLACK_INC = binc;
    Search::MOVES_TO_GO = movestogo;

    // movetime takes priority over normal clock management.
    if (moveTimeMs > 0) {
        Search::USE_MOVETIME = true;

        // Leave a small safety margin so the engine doesn't flag.
        Search::TIME = std::max(1, moveTimeMs * 95 / 100);
    }
    else {
        Search::USE_MOVETIME = false;
        Search::TIME = 5000; // fallback; findBestMove recalculates it
    }

    Move best = Search::findBestMove(board, searchDepth);

    if (best == Move::NO_MOVE) {
        std::cout << "bestmove 0000\n" << std::flush;
        return;
    }

    std::cout << "bestmove "
              << uci::moveToUci(best)
              << '\n'
              << std::flush;
}

void handleEval() {
    int evalScore = Eval::evaluate(board)/100;
    std::cout << "info string Evaluation score: " << evalScore << "\n" << std::flush;
}

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    Search::initTablebases();
    
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "uci") {
            handleUci();
        } else if (command == "isready") {
            handleIsReady();
        } else if (command == "ucinewgame") {
            handleUciNewGame();
        } else if (command == "position") {
            handlePosition(iss);
        } else if (command == "go") {
            handleGo(iss);
        } else if (command == "quit") {
            break;
        } else if (command == "eval") {
            handleEval();
        }else if (command == "setoption") {
            std::string token, name, value;
            iss >> token; // "name"
            while (iss >> token && token != "value") {
                if (!name.empty()) name += " ";
                name += token;
            }
            while (iss >> token) {
                if (!value.empty()) value += " ";
                value += token;
            }
            // handle specific options here, e.g.:
            // if (name == "Hash") { ttSizeMB = std::stoi(value); /* resize TT */ }
        }
            }

    return 0;
}