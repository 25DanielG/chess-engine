#include <stdio.h>
#include "lib/tt.h"

tt_table_t *g_tt = NULL;

tt_table_t *tt_create(size_t size_mb) {
  tt_table_t *tt = malloc(sizeof(tt_table_t));
  if (!tt) return NULL;
  size_t num_bytes = size_mb * 1024 * 1024;
  tt->num_buckets = num_bytes / sizeof(tt_bucket_t);

  uint64_t n = tt->num_buckets; // round down
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n |= n >> 32;
  tt->num_buckets = (n + 1) >> 1;  // largest 2^x <= original
  tt->mask = tt->num_buckets - 1;
  tt->age = 0;

  tt->buckets = calloc(tt->num_buckets, sizeof(tt_bucket_t));
  if (!tt->buckets) {
    free(tt);
    return NULL;
  }

  size_t actual_mb = (tt->num_buckets * sizeof(tt_bucket_t)) / (1024 * 1024);
  printf("TT: allocated %zu MB (%llu buckets, %d entries/bucket)\n", actual_mb, (unsigned long long)tt->num_buckets, TT_BUCKET_SIZE);
  return tt;
}

void tt_free(tt_table_t *tt) {
  if (tt) {
    free(tt->buckets);
    free(tt);
  }
}

void tt_clear(tt_table_t *tt) {
  if (tt && tt->buckets) {
    memset(tt->buckets, 0, tt->num_buckets * sizeof(tt_bucket_t));
    tt->age = 0;
  }
}

void tt_new_search(tt_table_t *tt) {
  if (tt) {
    tt->age = (tt->age + 1) & 0x3F;  // 6-bit age
  }
}

int tt_probe(tt_table_t *tt, uint64_t hash, int depth, int alpha, int beta, int *score, uint16_t *best_move, int ply) {
  if (!tt) return 0;

  uint64_t idx = hash & tt->mask;
  uint32_t key = (uint32_t)(hash >> 32);
  tt_bucket_t *bucket = &tt->buckets[idx];

  for (int i = 0; i < TT_BUCKET_SIZE; ++i) {
    tt_entry_t *e = &bucket->entries[i];

    if (e->key == key) {
      *best_move = e->move; // found entry

      if (e->depth >= depth) { // use score if depth sufficient
        int s = tt_score_from_tt(e->score, ply);
        tt_flag_t flag = (tt_flag_t)(e->flag & 0x3);

        switch (flag) {
          case TT_EXACT:
            *score = s;
            return 1;
          case TT_LOWER:  // fail-high
            if (s >= beta) {
              *score = s;
              return 1;
            }
            break;
          case TT_UPPER:  // fail-low
            if (s <= alpha) {
              *score = s;
              return 1;
            }
            break;
          default:
            break;
        }
      }
      return 0; // cant use score
    }
  }

  *best_move = 0;
  return 0;
}

void tt_store(tt_table_t *tt, uint64_t hash, int depth, int score, tt_flag_t flag, uint16_t best_move, int ply) {
  if (!tt) return;

  uint64_t idx = hash & tt->mask;
  uint32_t key = (uint32_t)(hash >> 32);
  tt_bucket_t *bucket = &tt->buckets[idx];

  tt_entry_t *replace = &bucket->entries[0]; // empty, same pos, best
  int replace_score = INT32_MAX;

  for (int i = 0; i < TT_BUCKET_SIZE; ++i) {
    tt_entry_t *e = &bucket->entries[i];
    if (e->flag == TT_NONE) { // empty
      replace = e;
      break;
    }
    if (e->key == key) { // same position
      if (depth >= e->depth || (e->flag >> 2) != tt->age) {
        replace = e;
        break;
      }
      return;
    }

    int entry_age = (e->flag >> 2); // upper 6 bits
    int age_diff = (tt->age - entry_age) & 0x3F;
    int rs = e->depth - 4 * age_diff; 

    if (rs < replace_score) {
      replace_score = rs;
      replace = e;
    }
  }

  replace->key = key;
  replace->score = (int16_t)tt_score_to_tt(score, ply);
  replace->eval = 0;  // not used yet
  replace->move = best_move;
  replace->depth = (uint8_t)depth;
  replace->flag = (uint8_t)(flag | (tt->age << 2));
}

uint16_t tt_get_move(tt_table_t *tt, uint64_t hash) {
  if (!tt) return 0;

  uint64_t idx = hash & tt->mask;
  uint32_t key = (uint32_t)(hash >> 32);
  tt_bucket_t *bucket = &tt->buckets[idx];

  for (int i = 0; i < TT_BUCKET_SIZE; ++i) {
    if (bucket->entries[i].key == key) {
      return bucket->entries[i].move;
    }
  }

  return 0;
}
