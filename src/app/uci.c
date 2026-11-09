#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/board.h"
#include "include/bot.h"
#include "include/eval.h"
#include "include/magic.h"
#include "include/tt.h"

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define MAX_DEPTH (64)
#define DEFAULT_MOVES_TO_GO (30)
#define BENCH_DEPTH (8)
#define NO_LIMIT_MS (24 * 60 * 60 * 1000)

static const char *BENCH_FENS[] = {
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
  "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
  "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
  "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
  "r1bq1rk1/pp2bppp/2n1pn2/3p4/2PP4/2N1PN2/PP1B1PPP/R2QKB1R b KQ - 0 8",
  "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4",
  "rnbqkb1r/pp3ppp/4pn2/2pp4/3P4/2PBPN2/PP3PPP/RNBQK2R b KQkq - 0 5",
  "r2q1rk1/pb1nbppp/1p2pn2/2pp4/2PP4/1PN1PN2/PB2BPPP/R2Q1RK1 w - - 0 10",
  "2rq1rk1/pp1bppbp/3p1np1/4n3/3NP3/1BN1BP2/PPPQ2PP/2KR3R w - - 0 13",
  "r1b2rk1/2q1bppp/p2p1n2/np2p3/3PP3/5N1P/PPBN1PP1/R1BQR1K1 w - - 0 13",
  "4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
  "r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15",
  "6k1/6p1/6Pp/ppp5/3pn2P/1P3K2/1PP2P2/3N4 b - - 0 1",
  "3b4/5kp1/1p1p1p1p/pP1PpP1P/P1P1P3/3KN3/8/8 w - - 0 1",
  "2K5/p7/7P/5pR1/8/5k2/r7/8 w - - 0 1",
  "8/6pk/1p6/8/PP3p1p/5P2/4KP1q/3Q4 w - - 0 1",
  "7k/3p2pp/4q3/8/4Q3/5Kp1/P6b/8 w - - 0 1",
  "8/2p5/8/2kPKp1p/2p4P/2P5/3P4/8 w - - 0 1",
  "8/1p3pp1/7p/5P1P/2k3P1/8/2K2P2/8 w - - 0 1",
  "8/pp2r1k1/2p1p3/3pP2p/1P1P1P1P/P5KR/8/8 w - - 0 1",
  "5k2/7R/4P2p/5K2/p1r2P1p/8/8/8 b - - 0 1",
  "6k1/6p1/P6p/r1N5/5p2/7P/1b3PP1/4R1K1 w - - 0 1",
  "1r3k2/4q3/2Pp3b/3Bp3/2Q2p2/1p1P2P1/1P2KP2/3N4 w - - 0 1",
  "6k1/4pp1p/3p2p1/P1pPb3/R7/1r2P1PP/3B1P2/6K1 w - - 0 1",
  "8/3p3B/5p2/5P2/p7/PP5b/k7/6K1 w - - 0 1",
  "8/8/8/8/5kp1/P7/8/1K1N4 w - - 0 1",
  "8/8/1P6/5pr1/8/4R3/7k/2K5 w - - 0 1",
  "8/2p4P/8/kr6/6R1/8/8/1K6 w - - 0 1",
};

static board *B;
static move_t uci_moves[1][MAX_MOVES];

static int parse_square(const char *s) {
  if (s[0] < 'a' || s[0] > 'h' || s[1] < '1' || s[1] > '8') return -1;
  return (s[1] - '1') * 8 + (s[0] - 'a');
}

static int parse_promo(char c) {
  switch (c) {
    case 'n': return KNIGHT;
    case 'b': return BISHOP;
    case 'r': return ROOK;
    case 'q': return QUEEN;
    default: return 0;
  }
}

// find the move in the legal move list and play it; 0 if not legal
static int play_move(const char *str) {
  int from = parse_square(str);
  int to = parse_square(str + 2);
  int promo = parse_promo(str[4]);
  if (from < 0 || to < 0) return 0;

  int side = B->white;
  move_t *moves;
  int n = movegen_ply(B, side, 1, 0, &moves, uci_moves, MAX_MOVES);
  for (int i = 0; i < n; ++i) {
    if (moves[i].from == from && moves[i].to == to && moves[i].promo == promo) {
      undo_t u;
      make_move(B, &moves[i], side, &u);
      B->white = !side;
      return 1;
    }
  }
  return 0;
}

// position startpos [moves ...] | position fen <fen> [moves ...]
static void cmd_position(char *line) {
  char *moves = strstr(line, " moves ");
  if (moves) *moves = '\0';

  char *fen = strstr(line, "fen ");
  if (!fen || !load_fen(B, fen + 4)) load_fen(B, START_FEN);

  rep_reset();
  rep_add(hash_board(B));
  if (!moves) return;

  for (char *tok = strtok(moves + 7, " \r\n"); tok; tok = strtok(NULL, " \r\n")) {
    if (!play_move(tok)) {
      printf("info string illegal move %s\n", tok);
      return;
    }
    rep_add(hash_board(B));
  }
}

static int arg(const char *line, const char *name, int def) {
  const char *p = strstr(line, name);
  return p ? atoi(p + strlen(name)) : def;
}

// go movetime <ms> | go wtime <ms> btime <ms> [winc <ms>] [binc <ms>] [movestogo <n>] | go depth <n> | go nodes <n>
static void cmd_go(const char *line) {
  int depth = arg(line, "depth ", MAX_DEPTH);
  int ms = arg(line, "movetime ", -1);
  bot_node_limit = arg(line, "nodes ", 0);

  if (ms < 0) {
    int time_left = arg(line, B->white ? "wtime " : "btime ", -1);
    int inc = arg(line, B->white ? "winc " : "binc ", 0);
    int moves_to_go = arg(line, "movestogo ", DEFAULT_MOVES_TO_GO);
    if (time_left >= 0) {
      ms = time_left / moves_to_go + inc * 3 / 4;
      if (ms > time_left - 50) ms = time_left - 50;
      if (ms < 10) ms = 10;
    } else {
      ms = NO_LIMIT_MS; // no time given: depth or node limited
    }
  }

  bot *b = init_bot(B, B->white, depth, ms);
  int packed = find_move_ms(b, B->white, ms);
  free(b);

  if (packed < 0) {
    printf("bestmove 0000\n");
    return;
  }

  int from = packed / 64, to = packed % 64;
  const char *promo = bot_last_promo ? (const char *[]){ "", "n", "b", "r", "q" }[bot_last_promo] : "";
  printf("bestmove %c%c%c%c%s\n", 'a' + from % 8, '1' + from / 8, 'a' + to % 8, '1' + to / 8, promo);
}

// search over BENCH_FENS
static void cmd_bench(int depth) {
  int count = sizeof(BENCH_FENS) / sizeof(BENCH_FENS[0]);
  long total = 0;
  bot_uci_info = 0;
  bot_node_limit = 0;
  double start = gtime();
  for (int i = 0; i < count; ++i) {
    load_fen(B, BENCH_FENS[i]);
    if (g_tt) tt_clear(g_tt);
    rep_reset();
    rep_add(hash_board(B));
    bot *b = init_bot(B, B->white, depth, NO_LIMIT_MS);
    find_move_ms(b, B->white, NO_LIMIT_MS);
    free(b);
    total += bot_nodes;
  }
  double elapsed = gtime() - start;
  printf("%ld nodes %ld nps\n", total, (long)(total / (elapsed > 0 ? elapsed : 1)));
  bot_uci_info = 1;
  load_fen(B, START_FEN);
}

int main(int argc, char **argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  bot_verbose = 0;
  bot_uci_info = 1;
  init_attack_tables();
  init_pesto_tables();
  B = init_board();
  load_fen(B, START_FEN);

  if (argc > 1 && !strcmp(argv[1], "bench")) {
    cmd_bench(argc > 2 ? atoi(argv[2]) : BENCH_DEPTH);
    free_board(B);
    return 0;
  }

  char line[16384];
  while (fgets(line, sizeof(line), stdin)) {
    if (!strncmp(line, "ucinewgame", 10)) {
      if (g_tt) tt_clear(g_tt);
      load_fen(B, START_FEN);
    } else if (!strncmp(line, "uci", 3)) {
      printf("id name X1\nid author Daniel Gergov\nuciok\n");
    } else if (!strncmp(line, "isready", 7)) {
      printf("readyok\n");
    } else if (!strncmp(line, "position", 8)) {
      cmd_position(line);
    } else if (!strncmp(line, "bench", 5)) {
      cmd_bench(arg(line, "bench ", BENCH_DEPTH));
    } else if (!strncmp(line, "go", 2)) {
      cmd_go(line);
    } else if (!strncmp(line, "quit", 4)) {
      break;
    }
  }

  free_board(B);
  return 0;
}
