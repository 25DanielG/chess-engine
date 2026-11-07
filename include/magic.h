#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "board.h"
#include "bot.h"

extern const uint64_t BISHOP_MAGICS[BOARD_SIZE];
extern const uint64_t ROOK_MAGICS[BOARD_SIZE];
extern const uint64_t BISHOP_SHIFTS[BOARD_SIZE];
extern const uint64_t ROOK_SHIFTS[BOARD_SIZE];

uint64_t ROOK_MASKS[64];
uint64_t BISHOP_MASKS[64];

uint64_t bishopAttacks[64][1 << 9];
uint64_t rookAttacks[64][1 << 12];

uint64_t apply(uint64_t occupancy, uint64_t magic, int shift);
uint64_t generate_rook_mask(int square);
uint64_t generate_bishop_mask(int square);
void init_attack_tables(void);
uint64_t set_occupancy(int index, uint64_t mask);
uint64_t compute_rook_attacks(int square, uint64_t occupancy);
uint64_t compute_bishop_attacks(int square, uint64_t occupancy);
uint64_t generate_rook_attacks(int square, uint64_t occupancy);
uint64_t generate_bishop_attacks(int square, uint64_t occupancy);
uint64_t generate_queen_attacks(int square, uint64_t occupancy);
void test_magic_bitboards(void);