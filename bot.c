#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "lib/bot.h"
#include "lib/board.h"
#include "lib/manager.h"
#include "lib/eval.h"

static const int PIECE_PT[64] = {
  -1, /*1*/ PAWN, /*2*/ KNIGHT, -1, /*4*/ BISHOP, -1, -1, -1, /*8*/ ROOK, -1, -1, -1, -1, -1, -1, -1,
  /*16*/ QUEEN, -1, -1, -1, -1, -1, -1, -1, -1,-1,-1,-1,-1,-1,-1,-1, /*32*/ KING, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static inline int equals(move_t a, move_t b) {
  return (a.from == b.from) && (a.to == b.to) && (a.piece == b.piece);
}

static void init_ordering_tables(void) {
  for (int v = 0; v < NUM_PIECES; ++v)
    for (int a = 0; a < NUM_PIECES; ++a)
      mvv_lva[v][a] = (value(v) << 4) - value(a);

  for (int p = 0; p < MAX_PLY; ++p) killer1[p].from = killer2[p].from = 255; // clear killers
  memset(history_tbl, 0, sizeof(history_tbl));
}

static inline int time_over(void) {
  if (time_flag) return 1;
  if ((nodes & NODE_CHECK) != 0) return 0;
  if (gtime() >= deadline) {
    time_flag = 1;
    return 1;
  }
  return 0;
}

bot *init_bot(board *B, int white, int depth, int limit) {
  bot *player = malloc(sizeof(bot));
  if (!player) {
    exit(1);
  }
  player->B = B;
  player->white = white;
  player->depth = depth;
  player->limit = limit;
  return player;
}

double gtime(void) {
  return (double)clock() / CLOCKS_PER_SEC;
}

int minimax(board *B, int depth, int max, int alpha, int beta, long *info, int ply) {
#ifdef DEBUG
  *info += 1;
  *(info + 1) += (depth == 0) ? 1 : 0;
#endif
  ++nodes;
  if (time_over()) return max ? alpha : beta;
  int old = B->white;
  B->white = max;
  if (depth == 0) {
    int v = quiesce(B, max, alpha, beta, info);
    B->white = old;
    return v;
  }
  int best = max ? INT32_MIN : INT32_MAX;
  move_t *moves;
  int move_count = movegen(B, max, &moves, 1);
  if (move_count == 0) {
    int v = check(B, max) ? (max ? -MATE + ply : +MATE - ply) : 0;
    B->white = old;
    free(moves);
    return v;
  }
  score_moves(B, moves, move_count, max, ply);
  int i;
  for (i = 0; i < move_count; ++i) {
    board_snapshot snap;
    save_snapshot(B, &snap);
    int mover = max;
    fast_execute(B, moves[i].piece, moves[i].from, moves[i].to, max);
    int eval = minimax(B, depth - 1, !max, alpha, beta, info, ply + 1);
    if (max) {
      best = (eval > best) ? eval : best;
      alpha = (eval > alpha) ? eval : alpha;
    } else {
      best = (eval < best) ? eval : best;
      beta = (eval < beta) ? eval : beta;
    }
    restore_snapshot(B, &snap);
    if (beta <= alpha) break;
  }
  free(moves);
  B->white = old;
  return best;
}

int quiesce(board *B, int side, int alpha, int beta, long *info) {
#ifdef DEBUG
  *(info + 2) += 1;
#endif
  // 1 = white (max), 0 = black (min)
  int stand = blended_eval(B);
  if (side) { // max
    if (stand >= beta)  return beta;
    if (stand > alpha)  alpha = stand;
  } else { // min
    if (stand <= alpha) return alpha;
    if (stand < beta)   beta = stand;
  }

  #define MAX_MOVES 256
  static move_t caps[MAX_MOVES];
  int n = 0;
  move_t *mv; int mcount = 0;
  movegen(B, side, &mv, 0); // pseudo legal

  // filter captures
  for (int i = 0; i < mcount; ++i) {
    uint64_t to_mask = 1ULL << mv[i].to;
    int is_cap = side ? ((B->blacks & to_mask)!=0) : ((B->whites & to_mask)!=0);
    if (is_cap) caps[n++] = mv[i];
    if (n == MAX_MOVES) break;
  }

  free(mv);

  for (int i = 0; i < n; ++i) {
    board_snapshot S; save_snapshot(B, &S);
    int next = side;
    fast_execute(B, caps[i].piece, caps[i].from, caps[i].to, next);
    if (check(B, !side)) {  // illegal
      restore_snapshot(B, &S);
      continue;
    }

    int score = quiesce(B, !side, alpha, beta, info);
    restore_snapshot(B, &S);

    if (side) {
      if (score >= beta) return beta;
      if (score > alpha) alpha = score;
    } else {
      if (score <= alpha) return alpha;
      if (score < beta)   beta = score;
    }
  }

  return side ? alpha : beta;
}

int find_move(bot *bot, int is_white, int limit) {
  long *info;
  double start = gtime();
  deadline = start + limit;
  time_flag = 0;
  init_ordering_tables();
#ifdef DEBUG
  printf("-------------STATS-------------\n");
  double debug_start = clock();
  printf("Minimax started with limit %d seconds\n", limit);
  info = (long*)calloc(sizeof(long), 3);
  if (!info) {
    fprintf(stderr, "Alloc failed\n");
    exit(1);
  }
#endif
  int move = -1;
  int best = is_white ? INT32_MIN : INT32_MAX;
  move_t *moves;
  int move_count = movegen(bot->B, is_white, &moves, 1);
  int depth;
  if (move_count == 0) return -1; // no legal moves
  for (depth = 1; depth <= bot->depth; ++depth) {
    nodes = 0;
    int i;
    int lbest = is_white ? INT32_MIN : INT32_MAX;
    int lmove = -1;
    for (i = 0; i < move_count; ++i) {
      board_snapshot snap;
      save_snapshot(bot->B, &snap);
      fast_execute(bot->B, moves[i].piece, moves[i].from, moves[i].to, is_white);
      bot->B->white = !is_white;
      int eval = minimax(bot->B, depth - 1, !is_white, INT32_MIN, INT32_MAX, info, 0);
      restore_snapshot(bot->B, &snap);
      if ((is_white && eval > lbest) || (!is_white && eval < lbest)) {
        lbest = eval;
        lmove = moves[i].from * 64 + moves[i].to;
      }
      if (time_over()) {
#ifdef DEBUG
        printf("Time limit reached...\n");
#endif
        if (move == -1) move = lmove;
        goto end_find;
      }
    }
    best = lbest;
    move = lmove;
#ifdef DEBUG
    printf("Depth %d ran in %lf seconds, best move: %d, eval: %d\n", depth, gtime() - start, move, best);
#endif
  }
end_find:
  free(moves);
#ifdef DEBUG
  clock_t debug_end = clock();
  double time = (double)(debug_end - debug_start) / CLOCKS_PER_SEC;
  printf("Time taken: %f seconds\n", time);
  printf("Visited nodes: %ld, leaf nodes: %ld, quiescence nodes %ld\n", *info, *(info + 1), *(info + 2));
  printf("Eval: %d, Mid Eval: %d, End Eval %d, Phase: %d, Scale: %d\n", best, mid_eval(bot->B), end_eval(bot->B), phase(bot->B), scale(bot->B, end_eval(bot->B)));
  printf("-------------------------------\n");
#endif
  return move;
}

static inline int is_capture(board *B, int side_to_move, const move_t *m) {
  uint64_t to_mask = 1ULL << m->to;
  return side_to_move ? (B->blacks & to_mask) != 0 : (B->whites & to_mask) != 0;
}

static inline int victim_square(board *B, int side_to_move, int sq) {
  uint64_t mask = 1ULL << sq;
  uint64_t *opp = side_to_move ? B->BLACK : B->WHITE;
  unsigned code = ((opp[PAWN] >> sq) & 1u) | 
                  (((opp[KNIGHT] >> sq) & 1u) << 1) | 
                  (((opp[BISHOP] >> sq) & 1u) << 2) | 
                  (((opp[ROOK] >> sq) & 1u) << 3) | 
                  (((opp[QUEEN] >> sq) & 1u) << 4) | 
                  (((opp[KING] >> sq) & 1u) << 5);
  return PIECE_PT[code];
}

static void move_sort(move_t *mv, int n) {
  for (int i = 1; i < n; ++i) {
    move_t key = mv[i];
    int j = i - 1;
    while (j >= 0 && mv[j].order < key.order) {
      mv[j + 1] = mv[j];
      --j;
    }
    mv[j + 1] = key;
  }
}

static void score_moves(board *B, move_t *mv, int n, int side_to_move, int ply) {
  for (int i = 0; i < n; ++i) {
    int score = 0;
    if (is_capture(B, side_to_move, &mv[i])) { // mvvlva
      int vic = victim_square(B, side_to_move, mv[i].to);
      score = (1 << 22) + (vic >= 0 ? mvv_lva[vic][mv[i].piece] : 0);
    } else {
      if (equals(killer1[ply], mv[i])) score = (1 << 21); // killers
      else if (equals(killer2[ply], mv[i])) score = (1 << 20);
      score += history_tbl[side_to_move][mv[i].piece][mv[i].to]; // history
    }
    mv[i].order = score;
  }
  move_sort(mv, n);
}