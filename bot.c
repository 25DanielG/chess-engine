#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include "lib/bot.h"
#include "lib/board.h"
#include "lib/manager.h"
#include "lib/eval.h"
#include "lib/utils.h"

move_t pv_table[MAX_PLY][MAX_PLY];
int pv_length[MAX_PLY];
move_t best_pv[MAX_PLY];
int best_pv_len = 0;

move_t move_stack[MAX_PLY][MAX_MOVES];
move_t qmove_stack[MAX_QPLY][MAX_MOVES];
move_t last_move[MAX_PLY];
move_t counter_move[2][64][64]; // best reply side, from, to

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

  for (int p = 0; p < MAX_PLY; ++p) {
    killer1[p].from = 255;
    killer2[p].from = 255; // clear killers
    last_move[p].from = 255; // no move
  }

  for (int s = 0; s < 2; ++s)
    for (int f = 0; f < 64; ++f)
      for (int t = 0; t < 64; ++t)
        counter_move[s][f][t].from = 255; // empty

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
  pv_length[ply] = 0;
  if (ply >= MAX_PLY) return blended_eval(B);
  if (time_over()) return blended_eval(B);
  int old = B->white;
  B->white = max;
  if (depth == 0) {
    int ch = check(B, !max);
    if (ch) {
      int v = oneply_check(B, max, alpha, beta, info, ply);
      B->white = old;
      return v;
    }

    int v = ROOT_QUIESCENCE_ENABLED ? quiesce(B, max, alpha, beta, info, 0) : blended_eval(B);
    B->white = old;
    return v;
  }

  int in_check = check(B, !max);
  int pv_node = WINDOW_IS_PV(alpha, beta);
  int near_root = (ply <= 2);
  int stand_eval = 0;
  int have_stand = 0;
  const int MATE_BOUND = MATE - 2 * QUEEN_VALUE; // near mate bound

  if (NMP_ENABLED && !pv_node && !near_root && !in_check && depth >= NMP_MIN_DEPTH && ply > 0) {
    if (!have_stand) {
      stand_eval = blended_eval(B);
      have_stand = 1;
    }

    if (abs(stand_eval) < MATE_BOUND && ((max && stand_eval >= beta - NMP_MARGIN) || (!max && stand_eval <= alpha + NMP_MARGIN))) { // avoid mate positions
      int R = NMP_BASE_REDUCTION + NMP_EXTRA_REDUCTION(depth);
      int nmdepth = depth - 1 - R;
      if (nmdepth < 0) nmdepth = 0;

      B->white = !max; // give side to opp
      int nmeval = minimax(B, nmdepth, !max, alpha, beta, info, ply + 1);
      B->white = max;

      if (max) {
        if (nmeval >= beta) {
          B->white = old;
          return nmeval;
        }
      } else {
        if (nmeval <= alpha) {
          B->white = old;
          return nmeval;
        }
      }
    }
  }

  if (RAZOR_ENABLED && !pv_node && !near_root && !in_check && depth <= RAZOR_MAX_DEPTH && ply > 0) { // not at root
    if (!have_stand) {
      stand_eval = blended_eval(B);
      have_stand = 1;
    }

    if (abs(stand_eval) < MATE_BOUND) { // avoid mate positions
      int margin1 = RAZOR_MARGIN1; // first stage razor, quiesce
      if (max) {
        if (stand_eval + margin1 <= alpha) {
          int q = quiesce(B, max, alpha, beta, info, 0);
          if (q <= alpha) {
            B->white = old;
            return q;
          }
        }
      } else {
        if (stand_eval - margin1 >= beta) {
          int q = quiesce(B, max, alpha, beta, info, 0);
          if (q >= beta) {
            B->white = old;
            return q;
          }
        }
      }

      if (depth == 2) { // second stage razor, reduced search
        int margin2 = RAZOR_MARGIN2;
        if (max) {
          if (stand_eval + margin2 <= alpha) {
            int r = minimax(B, depth - 1, max, alpha, beta, info, ply);
            if (r <= alpha) {
              B->white = old;
              return r;
            }
          }
        } else {
          if (stand_eval - margin2 >= beta) {
            int r = minimax(B, depth - 1, max, alpha, beta, info, ply);
            if (r >= beta) {
              B->white = old;
              return r;
            }
          }
        }
      }
    }
  }

  if (FUT_ENABLED && !pv_node && !in_check && depth <= FUT_NODE_MAX_DEPTH && ply > 0) { // shallow, not check
    if (!have_stand) {
      stand_eval = blended_eval(B);
      have_stand = 1;
    }

    if (abs(stand_eval) < MATE_BOUND) { // avoid mate positions
      int margin = FUT_BASE_MARGIN * depth;
      if (max) {
        if (stand_eval + margin <= alpha) { // static + margin <= alpha: fail low
          B->white = old;
          return stand_eval;
        }
      } else {
        if (stand_eval - margin >= beta) { // static - margin >= beta: fail high
          B->white = old;
          return stand_eval;
        }
      }
    }
  }

  int best = max ? INT32_MIN : INT32_MAX; 
  move_t *moves;
  int move_count = movegen_ply(B, max, 1, ply, &moves, move_stack, MAX_MOVES);
  if (move_count == 0) {
    int in_mate = check(B, !max); // opponent attacking side to move
    int v = in_mate ? (max ? -MATE + ply : +MATE - ply) : 0;
    B->white = old;
    return v;
  }
  score_moves(B, moves, move_count, max, ply);

  if (FUT_ENABLED && !pv_node && !in_check && depth <= FUT_MOVE_MAX_DEPTH && ply > 0 && !have_stand) {
    stand_eval = blended_eval(B);
    have_stand = 1;
  }

  int i;
  for (i = 0; i < move_count; ++i) {
    undo_t u;
    int cap = is_capture(B, max, &moves[i]);

    if (LMP_ENABLED && !pv_node && !near_root && !in_check && !cap && depth <= LMP_MAX_DEPTH && ply > 0 && i >= LMP_SKIP_BASE + depth) { // not check, not root
      continue; // prune
    }

    if (FUT_ENABLED && !pv_node && !in_check && depth <= FUT_MOVE_MAX_DEPTH && !cap && ply > 0 && i > 0 && have_stand && abs(stand_eval) < MATE_BOUND) { // not at root, not first move
      int margin = FUT_MOVE_MARGIN * depth; // move futility pruning
      if (max) {
        if (stand_eval + margin <= alpha) {
          continue;
        }
      } else {
        if (stand_eval - margin >= beta) {
          continue;
        }
      }
    }

    make_move(B, &moves[i], max, &u);
    last_move[ply] = moves[i]; // last move
    int gives_check = check(B, max);

    int eval;
    int lmr = 0;
    if (LMR_ENABLED && !pv_node && depth >= LMR_MIN_DEPTH && ply > 0 && i >= 1 && !cap && !gives_check && !in_check) {
      int R = LMR_BASE_REDUCTION;

      // move scaling
      if (depth >= 5) R++;
      if (i >= 8) R++;
      if (R > depth - 1) R = depth - 1;
      int red_depth = depth - 1 - R;
      if (red_depth < 1) red_depth = 1;
      lmr = 1;
      int nalpha = alpha; // reduced search window
      int nbeta = alpha + 1;
      if (!max) {
        nbeta = beta;
        nalpha = beta - 1;
      }

      eval = minimax(B, red_depth, !max, nalpha, nbeta, info, ply + 1);

      if (max ? (eval > alpha) : (eval < beta)) { // re-search full window
        eval = minimax(B, depth - 1, !max, alpha, beta, info, ply + 1);
      }
    } else {
      if (!PVS_ENABLED || i == 0) {
        eval = minimax(B, depth - 1, !max, alpha, beta, info, ply + 1);
      } else {
        int pvs_alpha = alpha; // null window
        int pvs_beta  = alpha + 1;
        if (!max) {
          pvs_beta  = beta;
          pvs_alpha = beta - 1;
        }

        eval = minimax(B, depth - 1, !max, pvs_alpha, pvs_beta, info, ply + 1);

        if (max ? (eval > alpha && eval < beta) : (eval < beta && eval > alpha)) {
          eval = minimax(B, depth - 1, !max, alpha, beta, info, ply + 1);
        }
      }
    }

    unmake_move(B, &moves[i], max, &u);
    int better = (max ? (eval > best) : (eval < best));

    if (better) {
      best = eval;

      if (max) {
        if (best > alpha) alpha = best;
      } else {
        if (best < beta) beta = best;
      }

      pv_table[ply][0] = moves[i];
      int clen = pv_length[ply + 1];
      if (clen > MAX_PLY - 1) clen = MAX_PLY - 1;
      for (int k = 0; k < clen; ++k)
        pv_table[ply][1 + k] = pv_table[ply + 1][k];
      pv_length[ply] = 1 + clen;
    } else {
      if (max) {
        if (eval > alpha) alpha = eval;
      }
      else {
        if (eval < beta) beta = eval;
      }
    }

    if (beta <= alpha) {
      if (!cap) { // update killers, history, countermove for quiet moves
        if (!equals(killer1[ply], moves[i])) {
          killer2[ply] = killer1[ply];
          killer1[ply] = moves[i];
        }
        history_tbl[max][moves[i].piece][moves[i].to] += depth * depth;

        if (ply > 0) {
          move_t prev = last_move[ply - 1]; // move before this node
          if (prev.from != 255) {
            int prev_side = max ^ 1; // prev move played by opp
            counter_move[prev_side][prev.from][prev.to] = moves[i];
          }
        }
      }
      break;
    }
  }
  B->white = old;
  return best;
}

int quiesce(board* B, int side, int alpha, int beta, long* info, int qply) {
#ifdef DEBUG
  * (info + 2) += 1;
#endif
  if (time_over()) return blended_eval(B);

  if (qply >= MAX_QPLY)
    return blended_eval(B);

  // 1 = white (max), 0 = black (min)
  int stand = blended_eval(B);
  if (side) { // max
    if (stand >= beta)  return beta;
    if (stand > alpha)  alpha = stand;
  } else { // min
    if (stand <= alpha) return alpha;
    if (stand < beta)   beta = stand;
  }

  move_t caps[MAX_MOVES];
  int n = 0;
  move_t* mv;
  int mcount = movegen_ply(B, side, 0, qply, &mv, qmove_stack, MAX_MOVES); // pseudo legal

  // filter captures
  for (int i = 0; i < mcount; ++i) {
    uint64_t to_mask = 1ULL << mv[i].to;
    int is_cap = side ? ((B->blacks & to_mask) != 0) : ((B->whites & to_mask) != 0);
    if (is_cap) caps[n++] = mv[i];
    if (n == MAX_MOVES) break;
  }

  for (int i = 0; i < n; ++i) {
    int piecev = victim_square(B, side, caps[i].to);
    int valv = (piecev >= 0 ? value(piecev) : 0);

    if (CAPPRUNE_ENABLED && piecev >= 0) {
      int piecea = caps[i].piece;
      int vala = value(piecea);

      if (valv + PAWN_VALUE < vala) { // capture pruning, bad trade
        continue;
      }
    }

    if (DELTA_PRUNE_ENABLED && DELTA_MARGIN > 0 && piecev >= 0 && abs(stand) < MATE - 2 * QUEEN_VALUE) { // avoid near mate
      int max_gain = valv; // TODO: add promotion bonus
      if (side) {
        if (stand + max_gain + DELTA_MARGIN <= alpha) {
          continue;
        }
      } else {
        if (stand - max_gain - DELTA_MARGIN >= beta) {
          continue;
        }
      }
    }

    undo_t u;
    make_move(B, &caps[i], side, &u);

    if (check(B, !side)) {  // illegal
      unmake_move(B, &caps[i], side, &u);
      continue;
    }
    int score = quiesce(B, !side, alpha, beta, info, qply + 1);
    unmake_move(B, &caps[i], side, &u);

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

int oneply_check(board *B, int side, int alpha, int beta, long *info, int ply) { // assumes B->white == side and side is in check
  if (ply >= MAX_PLY) return blended_eval(B);
  move_t *moves;
  int move_count = movegen_ply(B, side, 1, ply, &moves, move_stack, MAX_MOVES);  // legal moves only
  int m = 0;

  if (move_count == 0) {
    int score = side ? -MATE + ply : +MATE - ply;
    return score;
  }

  int best = side ? INT32_MIN : INT32_MAX;

  for (int i = 0; i < move_count; ++i) {
    undo_t u;
    make_move(B, &moves[i], side, &u);
    int child = minimax(B, 0, !side, alpha, beta, info, ply + 1);
    unmake_move(B, &moves[i], side, &u);

    if (side) {
      if (child > best) best = child;
      if (child > alpha) alpha = child;
    } else {
      if (child < best) best = child;
      if (child < beta)  beta  = child;
    }

    if (beta <= alpha) break;
  }

  return best;
}

int find_move(bot *bot, int is_white, int limit) {
  long *info;
  double start = gtime();
  deadline = start + limit;
  time_flag = 0;
  init_ordering_tables();
  for (int i = 0; i < MAX_PLY; ++i) {
    pv_length[i] = 0;
  }
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
  int move_count = movegen_ply(bot->B, is_white, 1, 0, &moves, move_stack, MAX_MOVES);
  int depth;
  int comp_depth = 0;
  if (move_count == 0) return -1; // no legal moves
  for (depth = 1; depth <= bot->depth; ++depth) {
    nodes = 0;
    int i;
    int lbest = is_white ? INT32_MIN : INT32_MAX;
    int lmove = -1;
    for (i = 0; i < move_count; ++i) {
      undo_t u;
      make_move(bot->B, &moves[i], is_white, &u);
      last_move[0] = moves[i]; // last move
      bot->B->white = !is_white;
      int eval = minimax(bot->B, depth - 1, !is_white, INT32_MIN, INT32_MAX, info, 1);
      bot->B->white = is_white;
      unmake_move(bot->B, &moves[i], is_white, &u);
      int packed = moves[i].from * 64 + moves[i].to;
      if ((is_white && eval > lbest) || (!is_white && eval < lbest)) {
        lbest = eval;
        lmove = packed;

        pv_table[0][0] = moves[i];
        int clen = pv_length[1];
        if (clen > MAX_PLY - 1)
          clen = MAX_PLY - 1;
        for (int k = 0; k < clen; ++k)
          pv_table[0][1 + k] = pv_table[1][k];
        pv_length[0] = 1 + clen;
      }
      if (time_over()) {
#ifdef DEBUG
        printf("Time limit reached...\n");
#endif
        if (move == -1) {
          move = lmove;
          best = lbest;
        }
        pv_length[0] = best_pv_len;
        for (int k = 0; k < best_pv_len; ++k)
          pv_table[0][k] = best_pv[k];
        goto end_find;
      }
    }
    best = lbest;
    move = lmove;
    best_pv_len = pv_length[0]; // save pv line
    for (int k = 0; k < best_pv_len; ++k)
      best_pv[k] = pv_table[0][k];
#ifdef DEBUG
    printf("Depth %d ran in %lf seconds, best move: %d, eval: %d\n", depth, gtime() - start, move, best);
#endif
    printf("Depth %d best: ", depth);
    print_move_eval("", move, best);
  }
end_find:
#ifdef DEBUG
  clock_t debug_end = clock();
  double time = (double)(debug_end - debug_start) / CLOCKS_PER_SEC;
  printf("Time taken: %f seconds\n", time);
  printf("Visited nodes: %ld, leaf nodes: %ld, quiescence nodes %ld\n", *info, *(info + 1), *(info + 2));
  printf("Eval: %d, Mid Eval: %d, End Eval %d, Phase: %d, Scale: %d\n", best, mid_eval(bot->B), end_eval(bot->B), phase(bot->B), scale(bot->B, end_eval(bot->B)));
  printf("Main PV line: ");
    for (int i = 0; i < pv_length[0]; ++i) {
      int from = pv_table[0][i].from;
      int to = pv_table[0][i].to;
      printf("%c%c%c%c", 'a' + (from % 8), '1' + (from / 8), 'a' + (to   % 8), '1' + (to   / 8));
      if (i < pv_length[0] - 1) printf(" ");
    }
    printf("\n");
  printf("-------------------------------\n");
#endif
  return move;
}

static inline int is_capture(const board *B, int side_to_move, const move_t *m) {
  uint64_t to_mask = 1ULL << m->to;
  return side_to_move ? (B->blacks & to_mask) != 0 : (B->whites & to_mask) != 0;
}

static inline int victim_square(const board *B, int side_to_move, int sq) {
  uint64_t mask = 1ULL << sq;
  uint64_t *opp = side_to_move ? B->BLACK : B->WHITE;
  unsigned code = ((opp[PAWN] >> sq) & 1u) | (((opp[KNIGHT] >> sq) & 1u) << 1) | (((opp[BISHOP] >> sq) & 1u) << 2) | 
                  (((opp[ROOK] >> sq) & 1u) << 3) | (((opp[QUEEN] >> sq) & 1u) << 4) | (((opp[KING] >> sq) & 1u) << 5);
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

static void score_moves(const board *B, move_t *mv, int n, int side_to_move, int ply) {
  move_t prev = { .from = 255 }; // prev move
  move_t cm = { .from = 255 };

  if (ply > 0) {
    prev = last_move[ply - 1];
    if (prev.from != 255) {
      int opp = side_to_move ^ 1; // side that played prev move
      cm = counter_move[opp][prev.from][prev.to];
    }
  }

  for (int i = 0; i < n; ++i) {
    int score = 0;
    if (is_capture(B, side_to_move, &mv[i])) { // mvvlva
      int vic = victim_square(B, side_to_move, mv[i].to);
      score = (1 << 22) + (vic >= 0 ? mvv_lva[vic][mv[i].piece] : 0);
    } else {
      // killers
      if (equals(killer1[ply], mv[i])) score = (1 << 21);
      else if (equals(killer2[ply], mv[i])) score = (1 << 20);

      if (cm.from != 255 && equals(cm, mv[i])) { // countermove
        score += (1 << 19); // under killer1
      }

      // history
      score += history_tbl[side_to_move][mv[i].piece][mv[i].to];
    }
    mv[i].order = score;
  }

  move_sort(mv, n);
}