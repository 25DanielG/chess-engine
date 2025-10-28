#ifndef _MANAGER_H_
#define _MANAGER_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"

#define BOARD_SIZE (64)
#define MAX_INPUT_SIZE (10)

// CONFIG

#define WHITE_BOT (0)
#define BLACK_BOT (1)
#define WHITE_DEPTH (15)
#define BLACK_DEPTH (15)
#define WHITE_LIMIT (5) // sec
#define BLACK_LIMIT (10) // sec

int run(void);
void out(board *B, int mode); // 0 letter, 1 piece
int parse(const char *input, int *from, int *to);
int execute(board *B, int from, int to, int *white);
int pi(char file, char rank);
void ip(int index, char *file, char *rank);
uint64_t imove(int piece, uint64_t from_mask, board *B, int *white);
void test_position(board *B);

#endif