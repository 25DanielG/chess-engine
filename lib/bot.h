#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "tt.h"
#include "see.h"

#define BOARD_SIZE (64)
#define MATE (32000)

#define TT_ENABLED (1)
#define TT_SIZE_MB (64)  // size in MB
#define OUTPUT_LINES (1)
#define NODE_CHECK (2047)
#define MAX_PLY (128)
#define MAX_QPLY (16)
#define NONE_PIECE (255)
#define MAX_MOVES (256)
#define MAX_PV (128)

#define ROOT_QUIESCENCE_ENABLED (1)

#define LMR_ENABLED (1)
#define LMR_MIN_DEPTH (3)
#define LMR_BASE_REDUCTION (1)

#define NMP_ENABLED (1)
#define NMP_MIN_DEPTH (3)
#define NMP_BASE_REDUCTION (2)
#define NMP_EXTRA_REDUCTION(depth) ((depth) / 3)
#define NMP_MARGIN (2 * PAWN_VALUE) // stand eval above beta by margin

#define LMP_ENABLED (1)
#define LMP_MAX_DEPTH (3) // only shallow depth
#define LMP_SKIP_BASE (3) // skip quiet moves after 8th

#define FUT_ENABLED (0)
#define FUT_NODE_MAX_DEPTH (2) // depth <= []
#define FUT_BASE_MARGIN (2 * PAWN_VALUE) // base fut margin
#define FUT_MOVE_MAX_DEPTH (3)
#define FUT_MOVE_MARGIN (1 * PAWN_VALUE) // quite move margin

#define DELTA_PRUNE_ENABLED (1)
#define DELTA_MARGIN SEE_PAWN // delta margin in centipawns

#define RAZOR_ENABLED (1)
#define RAZOR_MAX_DEPTH (2) // only depth 1–2
#define RAZOR_MARGIN1 (1 * PAWN_VALUE) // first stage
#define RAZOR_MARGIN2 (2 * PAWN_VALUE)

#define PVS_ENABLED (1)
#define WINDOW_IS_PV(alpha, beta) ((beta) - (alpha) > 1)

#define CAPPRUNE_ENABLED (1) // quiescence capture pruning

#define CHECK_EXTENSION_ENABLED (1)
#define CHECK_EXTENSION (1) // extend by 1 ply

#define SEE_PRUNE_DEPTH (3) // prune bad captures to depth
#define SEE_PRUNE_MARGIN SEE_PAWN // prune if SEE < -margin * depth

struct bot_header {
  board *B;
  int white;
  int depth;
  int limit;
};

typedef struct bot_header bot;

extern move_t pv_table[MAX_PLY][MAX_PLY];
extern int pv_length[MAX_PLY];

extern move_t move_stack[MAX_PLY][MAX_MOVES];
extern move_t qmove_stack[MAX_QPLY][MAX_MOVES];
extern move_t last_move[MAX_PLY];
extern move_t counter_move[2][64][64]; // best reply side, from, to

static double deadline;
static long nodes;
static int time_flag;

static move_t killer1[MAX_PLY];
static move_t killer2[MAX_PLY];
static int history_tbl[2][NUM_PIECES][64]; // side, piece, to
static int mvv_lva[NUM_PIECES][NUM_PIECES]; // mvvlva table

static inline int equals(move_t a, move_t b);
extern int value(int piece);
static void init_ordering_tables(void);
static inline int time_over(void);
bot *init_bot(board *B, int white, int depth, int limit);
double gtime(void);
int minimax(board *B, int depth, int max, int alpha, int beta, long *info, int ply);
int quiesce(board *B, int side, int alpha, int beta, long *info, int qply);
int oneply_check(board *B, int side, int alpha, int beta, long *info, int ply);
int find_move(bot *bot, int is_white, int limit);
static inline int is_capture(const board *B, int side_to_move, const move_t *m);
static inline int victim_square(const board *B, int side_to_move, int sq);
static void move_sort(move_t *mv, int n);
static void score_moves(const board *B, move_t *mv, int n, int side_to_move, int ply, uint16_t tt_move);