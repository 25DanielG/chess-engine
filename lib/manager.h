#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"

#define BOARD_SIZE (64)
#define MAX_INPUT_SIZE (10)
#define MAX_GAME_MOVES (512)

// CONFIG

#define GRAPHICS (1) // graphics
#define WHITE_BOT (0)
#define BLACK_BOT (1)
#define WHITE_DEPTH (15)
#define BLACK_DEPTH (15)
#define WHITE_LIMIT (5) // sec
#define BLACK_LIMIT (10) // sec

// ARRAYS

char *move_history[MAX_GAME_MOVES];
extern int history_len;

// FUNCTIONS

void start(void);
int run(void);
void out(board *B, int mode); // 0 letter, 1 piece
int parse(const char *input, int *from, int *to);
int execute(board *B, int from, int to, int *white);
uint64_t imove(int piece, uint64_t from_mask, board *B, int *white);
void test_position(board *B);
void add_history(int pt, int from, int to, int capture);
void free_history(void);