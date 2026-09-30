# HollowShellEngine

A UCI chess engine written in C++, built on top of [Disservin's `chess.hpp`](https://github.com/Disservin/chess-library) for move generation and board representation. It can be plugged into any UCI-compatible GUI (Arena, Cute Chess, etc.) and runs as a bot on Lichess.

## Features

- Alpha-beta search (`Search.cpp`)
- Custom evaluation function (`Eval.cpp`)
- Opening book support (`gm2001.bin`, Polyglot format)
- Syzygy endgame tablebase probing via [Fathom](https://github.com/jdart1/Fathom)
- UCI protocol support

## Project Structure

| File | Purpose |
|------|---------|
| `Engine.cpp` | Entry point and UCI loop |
| `Search.cpp` / `Search.h` | Search algorithm |
| `Eval.cpp` / `Eval.h` | Position evaluation |
| `Config.h` | Engine settings and constants |
| `chess.hpp` | Board and move generation library |
| `engine.py` | Python wrapper script |
| `build.ps1` | Windows build script |
| `run_engine.bat` | Launches the engine |
| `gm2001.bin` | Opening book |
| `3-4-5endgame/` | Syzygy 3-4-5 piece tablebases |
| `Fathom/` | Tablebase probing library |

## Building

**Requirements:** a C++17 compiler (g++ / MinGW-w64 or MSVC) on Windows. 

```powershell
git clone https://github.com/J-Scott16/Chess-bot.git
cd Chess-bot
./build.ps1
```

This produces `engine.exe`.

## Running

Launch directly:

```powershell
./run_engine.bat
```

Or load `engine.exe` in a UCI GUI as a new engine. To test it manually, type:

```
uci
isready
position startpos
go movetime 1000
```

## Tablebases and Opening Book

- Point the engine at the `3-4-5endgame/` folder to enable Syzygy tablebase probing.
- The opening book (`gm2001.bin`) is used for the first moves of the game.

If you're missing either file, Syzygy tablebases can be downloaded from [syzygy-tables.info](https://syzygy-tables.info/) and Polyglot books are widely available online.

## Testing

Match results are tested against other engines (e.g. Stockfish with limited strength) using tools such as Cute Chess and BayesElo to estimate Elo.

## Credits

- [Disservin's chess-library](https://github.com/Disservin/chess-library) for board and move generation
- [Fathom](https://github.com/jdart1/Fathom) for Syzygy tablebase probing
- [Stockfish](https://stockfishchess.org/) for testing opponents

