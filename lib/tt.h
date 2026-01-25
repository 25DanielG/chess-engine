#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"

#define TT_DEFAULT_SIZE_MB (64) // 64mb default
#define TT_BUCKET_SIZE (4) // entries per bucket

typedef enum {
  TT_NONE  = 0,
  TT_EXACT = 1, // PV node exact score
  TT_LOWER = 2, // fail-high lower bound score
  TT_UPPER = 3 // fail-low: upper bound score
} tt_flag_t;

typedef struct {
  uint32_t key; // upper 32 bits of hash
  int16_t score; // eval
  int16_t eval; // static eval
  uint16_t move; // best move, 0 none
  uint8_t depth;
  uint8_t flag; // bound type and age
} tt_entry_t;

typedef struct {
  tt_entry_t entries[TT_BUCKET_SIZE];
} tt_bucket_t;

typedef struct {
  tt_bucket_t *buckets;
  uint64_t num_buckets;
  uint64_t mask;
  uint8_t age;
} tt_table_t;

extern tt_table_t *g_tt;

tt_table_t *tt_create(size_t size_mb);
void tt_free(tt_table_t *tt);
void tt_clear(tt_table_t *tt);
void tt_new_search(tt_table_t *tt);  // increment age
int tt_probe(tt_table_t *tt, uint64_t hash, int depth, int alpha, int beta, int *score, uint16_t *best_move, int ply); // 1 if hit (fill score, move, flag), 0 miss
void tt_store(tt_table_t *tt, uint64_t hash, int depth, int score, tt_flag_t flag, uint16_t best_move, int ply);
uint16_t tt_get_move(tt_table_t *tt, uint64_t hash); // move without probing

static inline uint16_t tt_encode_move(int from, int to, int promo) {
  // bits: 0-5: from, 6-11: to, 12-15: promo
  return (uint16_t)((from & 0x3F) | ((to & 0x3F) << 6) | ((promo & 0xF) << 12));
}

static inline void tt_decode_move(uint16_t encoded, int *from, int *to, int *promo) {
  *from = encoded & 0x3F;
  *to = (encoded >> 6) & 0x3F;
  *promo = (encoded >> 12) & 0xF;
}

static inline int tt_score_to_tt(int score, int ply) { // storing mate scores relative to root
  if (score > 30000) return score + ply; // winning
  if (score < -30000) return score - ply; // losing
  return score;
}

static inline int tt_score_from_tt(int score, int ply) {
  if (score > 30000) return score - ply;
  if (score < -30000) return score + ply;
  return score;
}
