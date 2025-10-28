#ifndef _BOT_H_
#define _BOT_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"

#define BOARD_SIZE (64)
#define MATE (32000)
#define OUTPUT_LINES (1)
#define NODE_CHECK (2047)
#define MAX_PLY 128
#define NONE_PIECE 255

struct bot_header {
    board *B;
    int white;
    int depth;
    int limit;
};

typedef struct bot_header bot;

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
int quiesce(board *B, int side, int alpha, int beta, long *info);
int find_move(bot *bot, int is_white, int limit);
static inline int is_capture(board *B, int side_to_move, const move_t *m);
static inline int victim_square(board *B, int side_to_move, int sq);
static void move_sort(move_t *mv, int n);
static void score_moves(board *B, move_t *mv, int n, int side_to_move, int ply);

#endif