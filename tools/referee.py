"""Plays matches between UCI engines to estimate ELO.

Usage:
    python3 tools/referee.py build/uci /opt/homebrew/bin/stockfish --games 100 --movetime 100 --opponent-elo 2000
"""
import argparse
import ast
import csv
import math
import queue
import subprocess
import threading
import time
import chess
import chess.pgn

class Engine:
    """Wraps one UCI engine running as a child process."""

    def __init__(self, path, options=None):
        self.path = path
        self.options = options or {} # {"UCI_LimitStrength": "true", "UCI_Elo": "1800"}
        self.proc = None
        self.lines = queue.Queue() # background reader thread

    def start(self):
        self.proc = subprocess.Popen([self.path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self):
        for line in self.proc.stdout:
            self.lines.put(line.rstrip("\n"))
        self.lines.put(None)  # EOF marker

    def send(self, line):
        self.proc.stdin.write(line + "\n")
        self.proc.stdin.flush()

    def read_until(self, prefix, timeout=None):
        deadline = None if timeout is None else time.monotonic() + timeout
        while True:
            try:
                remaining = None if deadline is None else max(deadline - time.monotonic(), 0)
                line = self.lines.get(timeout=remaining)
            except queue.Empty:
                raise TimeoutError(f"engine did not respond within {timeout} seconds")
            if line is None:
                raise EOFError("engine process ended unexpectedly")
            if line.startswith(prefix):
                return line

    def handshake(self):
        self.send("uci")
        self.read_until("uciok")
        
        for name, value in self.options.items():
            self.send(f"setoption name {name} value {value}")
        self.send("isready")
        self.read_until("readyok")

    def new_game(self):
        self.send("ucinewgame")
        self.send("isready")
        self.read_until("readyok")

    def best_move(self, moves, movetime_ms):
        position = "position startpos"
        if moves:
            position += " moves " + " ".join(moves)
        self.send(position)
        self.send(f"go movetime {movetime_ms}")
        response = self.read_until("bestmove", timeout=movetime_ms / 1000 * 5 + 1)
        return response.split()[1]

    def quit(self):
        self.send("quit")
        try:
            self.proc.wait(timeout=1)
        except subprocess.TimeoutExpired:
            self.proc.kill()
            self.proc.wait()

class Referee:
    """Keeps the real game state and checks legality / game end."""

    def __init__(self, start_fen=None):
        self.board = chess.Board(start_fen) if start_fen is not None else chess.Board()

    def is_legal(self, move):
        return move in self.board.legal_moves
        
    def push(self, move):
        self.board.push(move)

    def result(self):
        if self.board.is_checkmate():
            return "0-1" if self.board.turn else "1-0"
        if self.board.is_stalemate() or self.board.is_insufficient_material():
            return "1/2-1/2"
        if self.board.is_repetition(3) or self.board.halfmove_clock >= 100:
            return "1/2-1/2"
        return None

def load_openings(path, count):
    openings = []
    with open(path, newline="") as handle:
        for row in csv.DictReader(handle):
            san_moves = [m.split(".")[-1] for m in ast.literal_eval(row["moves_list"])]
            board = chess.Board()
            moves = []
            for san in san_moves:
                moves.append(board.push_san(san).uci())  # convert to UCI for play_game
            if moves:
                openings.append(moves)
            if len(openings) >= count:
                break
    return openings

def play_game(white, black, opening, movetime_ms, max_plies):
    referee = Referee()
    moves = []
    for move_uci in opening:
        move = chess.Move.from_uci(move_uci)
        if not referee.is_legal(move):
            raise ValueError(f"illegal opening move {move_uci} in {opening}")
        referee.push(move)
        moves.append(move_uci)

    while len(moves) < max_plies:
        engine = white if referee.board.turn else black
        try:
            move_uci = engine.best_move(moves, movetime_ms)
            move = chess.Move.from_uci(move_uci)
        except (TimeoutError, EOFError, ValueError, IndexError): # hang, crash, bad move
            return "0-1" if referee.board.turn else "1-0", moves
        if not referee.is_legal(move):
            return "0-1" if referee.board.turn else "1-0", moves
        referee.push(move)
        moves.append(move_uci)
        result = referee.result()
        if result is not None:
            return result, moves

    return "1/2-1/2", moves

def play_match(engine_a, engine_b, openings, games, movetime_ms, max_plies):
    wins = draws = losses = 0
    all_games = []
    for game_index in range(games):
        opening = openings[(game_index // 2) % len(openings)] # same opening, colors swapped
        if game_index % 2 == 0:
            white, black = engine_a, engine_b
        else:
            white, black = engine_b, engine_a
        engine_a.new_game()
        engine_b.new_game()
        result, moves = play_game(white, black, opening, movetime_ms, max_plies)
        all_games.append((result, moves))
        if result == "1-0":
            if game_index % 2 == 0:
                wins += 1
            else:
                losses += 1
        elif result == "0-1":
            if game_index % 2 == 0:
                losses += 1
            else:
                wins += 1
        else:
            draws += 1
        print(f"Game {game_index + 1}: {result} | W {wins} D {draws} L {losses}")
    return wins, draws, losses, all_games

def write_pgn(path, games):
    with open(path, "w") as handle:
        for result, moves in games:
            board = chess.Board()
            for move_uci in moves:
                board.push(chess.Move.from_uci(move_uci))
            game = chess.pgn.Game.from_board(board)
            game.headers["Result"] = result
            print(game, file=handle)

def score(wins, draws, losses):
    total = wins + draws + losses
    return (wins + 0.5 * draws) / total if total else 0.0

def elo_diff(p):
    if p <= 0.0:
        return -400.0
    if p >= 1.0:
        return 400.0
    return -400.0 * math.log10(1.0 / p - 1.0)

def error_margin(wins, draws, losses):
    total = wins + draws + losses
    if total == 0:
        return 0.0
    p = score(wins, draws, losses)
    if p <= 0.0 or p >= 1.0:
        return math.inf  # all wins or all losses: margin can't be estimated
    variance = (wins * (1 - p) ** 2 + draws * (0.5 - p) ** 2 + losses * p ** 2) / total
    return 1.96 * math.sqrt(variance / total) * 400.0 / (math.log(10) * p * (1 - p))

def sprt_llr(wins, draws, losses, elo0=0, elo1=5):
    total = wins + draws + losses
    if total == 0:
        return 0.0
    s0 = 1.0 / (1.0 + 10 ** (-elo0 / 400.0)) # expected score if the true difference is elo0
    s1 = 1.0 / (1.0 + 10 ** (-elo1 / 400.0))
    p = score(wins, draws, losses)
    variance = (wins * (1 - p) ** 2 + draws * (0.5 - p) ** 2 + losses * p ** 2) / total
    if variance == 0:
        return 0.0
    return total * (s1 - s0) * (2 * p - s0 - s1) / (2 * variance)

def parse_args():
    ap = argparse.ArgumentParser(description="Play UCI engines against each other and estimate Elo")
    ap.add_argument("engine_a", help="path to the engine being tested")
    ap.add_argument("engine_b", help="path to the opponent engine")
    ap.add_argument("--games", type=int, default=100)
    ap.add_argument("--movetime", type=int, default=100, help="ms per move")
    ap.add_argument("--maxplies", type=int, default=300)
    ap.add_argument("--opponent-elo", type=int, default=None, help="set UCI_Elo on engine_b (Stockfish)")
    ap.add_argument("--book", default="data/high_elo_opening.csv")
    ap.add_argument("--pgn", default=None, help="write all games to this file")
    return ap.parse_args()

def estimate():
    args = parse_args()
    challenger = Engine(args.engine_a)
    options = {}
    if args.opponent_elo is not None:
        options = {"UCI_LimitStrength": "true", "UCI_Elo": str(args.opponent_elo)}
    defender = Engine(args.engine_b, options)

    # load openings (one per pair of games)
    openings = load_openings(args.book, args.games // 2 + 1)

    for engine in (challenger, defender):
        engine.start()
        engine.handshake()
    try:
        wins, draws, losses, games = play_match(challenger, defender, openings, args.games, args.movetime, args.maxplies)
    finally:
        challenger.quit()
        defender.quit()

    if args.pgn:
        write_pgn(args.pgn, games)
    p = score(wins, draws, losses)
    margin = error_margin(wins, draws, losses)
    print(f"W/D/L: {wins}/{draws}/{losses}")
    if p <= 0.0 or p >= 1.0:
        print(f"Elo difference: {'>= +400' if p >= 1.0 else '<= -400'} (Sweep: play a closer opponent or more games)")
    else:
        print(f"Elo difference: {elo_diff(p):+.2f} +/- {margin:.2f}")

if __name__ == "__main__":
    estimate()
