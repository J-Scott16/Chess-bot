import sys
import chess  # python-chess library
import chess.polyglot
import chess.syzygy
BOOK_PATH = r"D:\random projects\Chess bot\gm2001.bin"
TABLEBASE_PATH = r"D:\random projects\Chess bot\3-4-5endgame"
board = chess.Board()
move2 = True
depth = 4
score = 0
bestmove = None
tablebase = chess.syzygy.open_tablebase(
    TABLEBASE_PATH,
    load_wdl=True,
    load_dtz=True
)
TT = {}
QUEEN_TABLE = [
    -20, -10, -10,   0,   0, -10, -10, -20,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -10,   5,   5,   5,   5,   5,   0, -10,
      0,   0,   5,   5,   5,   5,   0,  -5,
     -5,   0,   5,   5,   5,   5,   0,  -5,
    -10,   0,   5,   5,   5,   5,   5, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10,   0,   0, -10, -10, -20
]
KNIGHT_TABLE = [
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -30,   5,  10,  15, 15, 10,   5, -30,
    -30,   0,  15,  20, 20, 15,   0, -30,
    -30,   5,  15,  20, 20, 15,   5, -30,
    -30,   0,  10,  15, 15, 10,   0, -30,
    -40, -20,   0,   0,   0,  0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
]
PAWN_TABLE = [
     0,   0,   0,   0,   0,   0,   0,   0,
     5,  10,  10, -20, -20,  10,  10,   5,
     5,  -5, -10,   0,   0, -10,  -5,   5,
     0,   0,   0,  20,  20,   0,   0,   0,
     5,   5,  10,  25,  25,  10,   5,   5,
    10,  10,  20,  30,  30,  20,  10,  10,
    50,  50,  50,  50,  50,  50,  50,  50,
     0,   0,   0,   0,   0,   0,   0,   0
]
BISHOP_TABLE = [
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
]
ROOK_TABLE = [
     0,   0,   0,   5,   5,   0,   0,   0,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    10,  10,  10,  10,  10,  10,  10,  10,
     0,   0,   10,   15,   15,   10,   0,   0
]
KING_TABLE = [
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
     20,  20,   0,   0,   0,   0,  20,  20,
     20,  60,  10,   0,   0,  10,  60,  20
]
def quiescence(board, alpha, beta, qdepth=0):

    if qdepth >= 4:
        return evaluate(board)

    stand_pat = evaluate(board)

    if board.turn == chess.WHITE:

        if stand_pat >= beta:
            return beta

        if stand_pat > alpha:
            alpha = stand_pat

    else:

        if stand_pat <= alpha:
            return alpha

        if stand_pat < beta:
            beta = stand_pat

    moves = []

    for move in board.legal_moves:
        if board.is_capture(move):
            moves.append(move)

    moves.sort(
        key=lambda move: moveorder(board, move),
        reverse=True
    )

    if board.turn == chess.WHITE:

        for move in moves:

            board.push(move)

            score = quiescence(
                board,
                alpha,
                beta,
                qdepth + 1
            )

            board.pop()

            if score >= beta:
                return beta

            if score > alpha:
                alpha = score

    else:

        for move in moves:

            board.push(move)

            score = quiescence(
                board,
                alpha,
                beta,
                qdepth + 1
            )

            board.pop()

            if score <= alpha:
                return alpha

            if score < beta:
                beta = score

    return alpha if board.turn == chess.WHITE else beta
def moveorder(board, move):
    score = 0

    values = {
        chess.PAWN: 100,
        chess.KNIGHT: 320,
        chess.BISHOP: 330,
        chess.ROOK: 500,
        chess.QUEEN: 900,
        chess.KING: 20000
    }

    if board.is_capture(move):

        victim = board.piece_at(move.to_square)
        attacker = board.piece_at(move.from_square)

        if victim:
            score += 10 * values[victim.piece_type]
            score -= values[attacker.piece_type]

    if move.promotion:
        score += 900

    return score
PIECE_VALUES = {
    chess.PAWN: 100,
    chess.KNIGHT: 320,
    chess.BISHOP: 330,
    chess.ROOK: 500,
    chess.QUEEN: 900,
    chess.KING: 20000
}

PIECE_TABLES = {
    chess.PAWN: PAWN_TABLE,
    chess.KNIGHT: KNIGHT_TABLE,
    chess.BISHOP: BISHOP_TABLE,
    chess.ROOK: ROOK_TABLE,
    chess.QUEEN: QUEEN_TABLE,
    chess.KING: KING_TABLE
}


def evaluate(board):
    score = 0

    for square, piece in board.piece_map().items():

        value = PIECE_VALUES[piece.piece_type]
        table = PIECE_TABLES[piece.piece_type]

        if piece.color == chess.WHITE:
            score += value
            score += table[square]
        else:
            score -= value
            score -= table[chess.square_mirror(square)]

    return score
def get_book_move(board):

    try:
        with chess.polyglot.open_reader(BOOK_PATH) as reader:

            entries = list(reader.find_all(board))

            if not entries:
                return None

            # Pick the move with the highest book weight
            best_entry = max(entries, key=lambda entry: entry.weight)

            return best_entry.move

    except Exception as e:
        print(f"Opening book error: {e}", file=sys.stderr)
        return None

def minmax(board, depth, alpha, beta):
    if len(board.piece_map()) <= 5:
     try:
        wdl = tablebase.probe_wdl(board)

        if wdl > 0:
            return 100000 if board.turn == chess.WHITE else -100000

        elif wdl < 0:
            return -100000 if board.turn == chess.WHITE else 100000

        else:
            return 0

     except chess.syzygy.MissingTableError:
        pass

    alpha_original = alpha
    beta_original = beta

    # Transposition table
    key = board._transposition_key()

    if key in TT:
        stored_depth, stored_score, flag, tt_move = TT[key]

        if stored_depth >= depth:

            if flag == "EXACT":
                return stored_score

            elif flag == "LOWER":
                alpha = max(alpha, stored_score)

            elif flag == "UPPER":
                beta = min(beta, stored_score)

            if alpha >= beta:
                return stored_score

    # Draw conditions
    if board.is_repetition(3):
        return 0

    if board.is_insufficient_material():
        return 0

    if board.is_stalemate():
        return 0

    if board.is_fifty_moves():
        return 0

    # Checkmate
    if board.is_checkmate():
        if board.turn == chess.WHITE:
            return -999999
        else:
            return 999999

    # Leaf node
    if depth == 0:
        return quiescence(board, alpha, beta)

    best_move = None

    # Generate moves
    moves = list(board.legal_moves)

    moves.sort(
        key=lambda move: moveorder(board, move),
        reverse=True
    )

    # WHITE MAXIMIZES
    if board.turn == chess.WHITE:

        best_score = -999999

        for move in moves:

            board.push(move)

            score = minmax(
                board,
                depth - 1,
                alpha,
                beta
            )

            board.pop()

            if score > best_score:
                best_score = score
                best_move = move

            alpha = max(alpha, best_score)

            # Alpha-beta cutoff
            if beta <= alpha:
                break

    # BLACK MINIMIZES
    else:

        best_score = 999999

        for move in moves:

            board.push(move)

            score = minmax(
                board,
                depth - 1,
                alpha,
                beta
            )

            board.pop()

            if score < best_score:
                best_score = score
                best_move = move

            beta = min(beta, best_score)

            # Alpha-beta cutoff
            if beta <= alpha:
                break

    # Determine TT flag
    if best_score <= alpha_original:
        flag = "UPPER"

    elif best_score >= beta_original:
        flag = "LOWER"

    else:
        flag = "EXACT"

    # ALWAYS STORE 4 VALUES
    TT[key] = (
        depth,
        best_score,
        flag,
        best_move
    )

    return best_score
def choose_move(board):
      # Game already over
    if board.is_checkmate() or board.is_stalemate():
        return None
    # Check opening book first
    book_move = get_book_move(board)

    if book_move is not None:
        return book_move

    # No book move -> use normal search

    bestmove = None
    global move2

    bestmove = None

    if board.turn == chess.WHITE:
        score = -999999
    else:
        score = 999999

    moves = list(board.legal_moves)
    moves.sort(
        key=lambda move: moveorder(board, move),
        reverse=True
    )
    

    alpha = -999999
    beta = 999999

    for move in moves:

        board.push(move)

        tempscore = minmax(
            board,
            depth - 1,
            alpha,
            beta
        )

        board.pop()

        if board.turn == chess.WHITE:
            if tempscore > score:
                score = tempscore
                bestmove = move

            alpha = max(alpha, score)

        else:
            if tempscore < score:
                score = tempscore
                bestmove = move

            beta = min(beta, score)

    return bestmove
while True:
    line = input()
    tokens = line.split()
    if not tokens:
        continue
    cmd = tokens[0]

    if cmd == "uci":
        print("id name HisEngine")
        print("id author HisName")
        print("uciok")
        sys.stdout.flush()

    elif cmd == "isready":
        print("readyok")
        sys.stdout.flush()

    elif cmd == "ucinewgame":
        board = chess.Board()

    elif cmd == "position":
        if tokens[1] == "startpos":
            board = chess.Board()
            moves_idx = 3 if len(tokens) > 2 and tokens[2] == "moves" else -1
        elif tokens[1] == "fen":
            fen = " ".join(tokens[2:8])
            board = chess.Board(fen)
            moves_idx = 9 if len(tokens) > 8 and tokens[8] == "moves" else -1

        if moves_idx != -1:
            for move_str in tokens[moves_idx:]:
                board.push_uci(move_str)

    elif cmd == "go":
     move = choose_move(board)

     if move is None:
        print("bestmove 0000")
     else:
        print(f"bestmove {move.uci()}")

     sys.stdout.flush()
    elif cmd == "quit":
        break