#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"

#define BOARD_SIZE (64)
#define MATE (32000)
#define OUTPUT_LINES (1)
#define NODE_CHECK (2047)
#define MAX_PLY (128)
#define MAX_QPLY (16)
#define NONE_PIECE (255)
#define MAX_MOVES (256)

#define LMR_ENABLED (1)
#define LMR_MIN_DEPTH (3)
#define LMR_LATE_MOVE_IDX (3) // reduce from 4th move and on
#define LMR_REDUCTION (1)

#define NMP_ENABLED (1)
#define NMP_MIN_DEPTH (3)
#define NMP_REDUCTION (2) // nmp depth reduction

#define LMP_ENABLED (1)
#define LMP_MAX_DEPTH (3) // only shallow depth
#define LMP_SKIP (8) // skip quiet moves after 8th

#define FUT_ENABLED (0)
#define FUT_NODE_MAX_DEPTH (2) // depth <= []
#define FUT_BASE_MARGIN (150) // base fut margin
#define FUT_PHASE_SCALE (0)
#define FUT_MOVE_MAX_DEPTH (2)
#define FUT_MOVE_MARGIN 120 // quite move margin
#define DELTA_MARGIN (80) // allowed material swing

#define PVS_ENABLED (1)

#define CAPPRUNE_ENABLED (1) // quiescence capture pruning

struct bot_header {
  board *B;
  int white;
  int depth;
  int limit;
};

typedef struct bot_header bot;

extern move_t move_stack[MAX_PLY][MAX_MOVES];
extern move_t qmove_stack[MAX_QPLY][MAX_MOVES];

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
static void score_moves(const board *B, move_t *mv, int n, int side_to_move, int ply);