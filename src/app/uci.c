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

// go movetime <ms> | go wtime <ms> btime <ms> [winc <ms>] [binc <ms>] [movestogo <n>] | go depth <n>
static void cmd_go(const char *line) {
  int depth = arg(line, "depth ", MAX_DEPTH);
  int ms = arg(line, "movetime ", -1);

  if (ms < 0) {
    int time_left = arg(line, B->white ? "wtime " : "btime ", -1);
    int inc = arg(line, B->white ? "winc " : "binc ", 0);
    int moves_to_go = arg(line, "movestogo ", DEFAULT_MOVES_TO_GO);
    if (time_left >= 0) {
      ms = time_left / moves_to_go + inc * 3 / 4;
      if (ms > time_left - 50) ms = time_left - 50;
      if (ms < 10) ms = 10;
    } else {
      ms = 24 * 60 * 60 * 1000; // no time given: depth limited
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

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  bot_verbose = 0;
  init_attack_tables();
  init_pesto_tables();
  B = init_board();
  load_fen(B, START_FEN);

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
    } else if (!strncmp(line, "go", 2)) {
      cmd_go(line);
    } else if (!strncmp(line, "quit", 4)) {
      break;
    }
  }

  free_board(B);
  return 0;
}
