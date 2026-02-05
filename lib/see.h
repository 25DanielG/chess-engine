#pragma once

#include <stdint.h>
#include "board.h"

// SEE piece values centipawns
#define SEE_PAWN   100
#define SEE_KNIGHT 320
#define SEE_BISHOP 330
#define SEE_ROOK   500
#define SEE_QUEEN  900
#define SEE_KING   20000

uint64_t attackers_to(const board *B, int sq, uint64_t occ); // all attackers both sides on a square
int least_valuable_attacker(const board *B, uint64_t attackers, int side, int *sq_out); // *sq_out = square of least val attacker
int see(const board *B, const move_t *m, int side); // gain/loss from a capture (centipawns) pos = winning for side
int see_ge(const board *B, const move_t *m, int side, int threshold); // optimized see, 1 if SEE(move) >= threshold
int see_value(int piece);