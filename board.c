#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "lib/board.h"
#include "lib/manager.h"
#include "lib/magic.h"
#include "lib/utils.h"

static inline int lsb(uint64_t x) {
  /* if (!x) {
    fprintf(stderr, "lsb() called with 0!\n");
    abort();
  } */
  return __builtin_ctzll(x);
}

board *init_board(void) {
  board *B = malloc(sizeof(board));
  if (!B) {
    exit(1);
  }

  B->white = true;

  B->WHITE = calloc(sizeof(uint64_t), NUM_PIECES);
  B->BLACK = calloc(sizeof(uint64_t), NUM_PIECES);
  
  B->castle = WKS | WQS | BKS | BQS;
  B->cc = 0x0;

  if (!B->WHITE || !B->BLACK) {
    fprintf(stderr, "Alloc failed\n");
    exit(1);
  }

  // init pieces
  B->WHITE[PAWN] = 0x000000000000FF00;
  B->WHITE[KNIGHT] = 0x0000000000000042;
  B->WHITE[BISHOP] = 0x0000000000000024;
  B->WHITE[ROOK] = 0x0000000000000081;
  B->WHITE[QUEEN] = 0x0000000000000008;
  B->WHITE[KING] = 0x0000000000000010;
  
  B->BLACK[PAWN] = 0x00FF000000000000;
  B->BLACK[KNIGHT] = 0x4200000000000000;
  B->BLACK[BISHOP] = 0x2400000000000000;
  B->BLACK[ROOK] = 0x8100000000000000;
  B->BLACK[QUEEN] = 0x0800000000000000;
  B->BLACK[KING] = 0x1000000000000000;

  B->whites = B->WHITE[PAWN] | B->WHITE[KNIGHT] | B->WHITE[BISHOP] | B->WHITE[ROOK] | B->WHITE[QUEEN] | B->WHITE[KING];
  B->blacks = B->BLACK[PAWN] | B->BLACK[KNIGHT] | B->BLACK[BISHOP] | B->BLACK[ROOK] | B->BLACK[QUEEN] | B->BLACK[KING];

  B->jumps = (uint64_t *)malloc(sizeof(uint64_t) * 64);
  if (!B->jumps) {
    fprintf(stderr, "Alloc failed\n");
    exit(1);
  }
  init_jump_table(B->jumps);
  
  return B;
}

board *preset_board(uint64_t wpawns, uint64_t bpawns, uint64_t wknights, uint64_t bknights, uint64_t wbishops, uint64_t bbishops, uint64_t wrooks, uint64_t brooks, uint64_t wqueens, uint64_t bqueens, uint64_t wkings, uint64_t bkings, uint8_t castling, uint8_t complete) {
  board *B = malloc(sizeof(board));
  if (!B) {
    exit(1);
  }

  B->WHITE = calloc(sizeof(uint64_t), NUM_PIECES);
  B->BLACK = calloc(sizeof(uint64_t), NUM_PIECES);

  if (!B->WHITE || !B->BLACK) {
    fprintf(stderr, "Alloc failed\n");
    exit(1);
  }

  // init pieces
  B->WHITE[PAWN] = wpawns;
  B->WHITE[KNIGHT] = wknights;
  B->WHITE[BISHOP] = wbishops;
  B->WHITE[ROOK] = wrooks;
  B->WHITE[QUEEN] = wqueens;
  B->WHITE[KING] = wkings;
  
  B->BLACK[PAWN] = bpawns;
  B->BLACK[KNIGHT] = bknights;
  B->BLACK[BISHOP] = bbishops;
  B->BLACK[ROOK] = brooks;
  B->BLACK[QUEEN] = bqueens;
  B->BLACK[KING] = bkings;

  B->whites = B->WHITE[PAWN] | B->WHITE[KNIGHT] | B->WHITE[BISHOP] | B->WHITE[ROOK] | B->WHITE[QUEEN] | B->WHITE[KING];
  B->blacks = B->BLACK[PAWN] | B->BLACK[KNIGHT] | B->BLACK[BISHOP] | B->BLACK[ROOK] | B->BLACK[QUEEN] | B->BLACK[KING];

  B->castle = castling;
  B->cc = complete;

  B->jumps = (uint64_t *)malloc(sizeof(uint64_t) * 64);
  if (!B->jumps) {
    fprintf(stderr, "Alloc failed\n");
    exit(1);
  }
  init_jump_table(B->jumps);
  
  return B;
}

uint64_t white_moves(const board *B) {
  return wp_moves(B->WHITE[PAWN], B->whites, B->blacks) | wn_moves(B->WHITE[KNIGHT], B->whites, B->jumps) | wb_moves(B->WHITE[BISHOP], B->whites, B->blacks) | wr_moves(B->WHITE[ROOK], B->whites, B->blacks) | wq_moves(B->WHITE[QUEEN], B->whites, B->blacks) | wk_moves(B->WHITE[KING], B->whites);
}

uint64_t black_moves(const board *B) {
  return bp_moves(B->BLACK[PAWN], B->whites, B->blacks) | bn_moves(B->BLACK[KNIGHT], B->blacks, B->jumps) | bb_moves(B->BLACK[BISHOP], B->whites, B->blacks) | br_moves(B->BLACK[ROOK], B->whites, B->blacks) | bq_moves(B->BLACK[QUEEN], B->whites, B->blacks) | bk_moves(B->BLACK[KING], B->blacks);
}

uint64_t wp_moves(uint64_t p, uint64_t w, uint64_t b) {
  uint64_t na = ~FILE_A;
  uint64_t nh = ~FILE_H;
  uint64_t e = ~(w | b);

  uint64_t s_moves = (p << 8) & e;
  uint64_t d_moves = ((p << 16) & e & (e << 8) & RANK_4);
  uint64_t left = (p << 9) & b & na;
  uint64_t right = (p << 7) & b & nh;
  return (s_moves | d_moves | left | right);
}

uint64_t bp_moves(uint64_t p, uint64_t w, uint64_t b) {
  uint64_t na = ~FILE_A;
  uint64_t nh = ~FILE_H;
  uint64_t e = ~(w | b);

  uint64_t s_moves = (p >> 8) & e;
  uint64_t d_moves = ((p >> 16) & e & (e >> 8)) & RANK_5;
  uint64_t left = (p >> 9) & w & nh;
  uint64_t right = (p >> 7) & w & na;
  return (s_moves | d_moves | left | right);
}

uint64_t wn_moves(uint64_t p, uint64_t w, uint64_t *jumps) {
  uint64_t legals = 0;
  while (p) {
    int pos = lsb(p);
    p &= p - 1;
    legals |= jumps[pos] & ~w;
  }
  return legals;
}

uint64_t bn_moves(uint64_t p, uint64_t b, uint64_t *jumps) {
  uint64_t legals = 0;
  while (p) {
    int pos = lsb(p);
    p &= p - 1;
    legals |= jumps[pos] & ~b;
  }
  return legals;
}

uint64_t wb_moves(uint64_t p, uint64_t w, uint64_t b) {
  uint64_t legals = 0ULL;
  const uint64_t occ = w | b;
  while (p) {
    int sq = lsb(p);
    p &= p - 1;
    uint64_t attacks = generate_bishop_attacks(sq, occ);
    legals |= attacks & ~w;
  }
  return legals;
}

uint64_t bb_moves(uint64_t p, uint64_t w, uint64_t b) {
  uint64_t legals = 0ULL;
  const uint64_t occ = w | b;
  while (p) {
    int sq = lsb(p);
    p &= p - 1;
    uint64_t attacks = generate_bishop_attacks(sq, occ);
    legals |= attacks & ~b;
  }
  return legals;
}

uint64_t wr_moves(uint64_t p, uint64_t w, uint64_t b) {
  uint64_t legals = 0ULL;
  const uint64_t occ = w | b;
  while (p) {
    int sq = lsb(p);
    p &= p - 1;
    uint64_t attacks = generate_rook_attacks(sq, occ);
    legals |= attacks & ~w;
  }
  return legals;
}

uint64_t br_moves(uint64_t p, uint64_t w, uint64_t b) {
  uint64_t legals = 0ULL;
  const uint64_t occ = w | b;
  while (p) {
    int sq = lsb(p);
    p &= p - 1;
    uint64_t attacks = generate_rook_attacks(sq, occ);
    legals |= attacks & ~b;
  }
  return legals;
}

uint64_t wq_moves(uint64_t p, uint64_t w, uint64_t b) {
  uint64_t legals = 0ULL;
  const uint64_t occ = w | b;
  while (p) {
    int sq = lsb(p);
    p &= p - 1;
    uint64_t attacks = generate_rook_attacks(sq, occ) | generate_bishop_attacks(sq, occ);
    legals |= attacks & ~w;
  }
  return legals;
}

uint64_t bq_moves(uint64_t p, uint64_t w, uint64_t b) {
  uint64_t legals = 0ULL;
  const uint64_t occ = w | b;
  while (p) {
    int sq = lsb(p);
    p &= p - 1;
    uint64_t attacks = generate_rook_attacks(sq, occ) | generate_bishop_attacks(sq, occ);
    legals |= attacks & ~b;
  }
  return legals;
}

uint64_t wk_moves(uint64_t p, uint64_t w) {
  int pos = lsb(p);
  uint64_t moves = circle(pos);
  moves &= ~w;
  return moves;
}

uint64_t bk_moves(uint64_t p, uint64_t b) {
  int pos = lsb(p);
  uint64_t moves = circle(pos);
  moves &= ~b;
  return moves;
}


uint64_t whites(const board *B) {
  return B->WHITE[PAWN] | B->WHITE[KNIGHT] | B->WHITE[BISHOP] | B->WHITE[ROOK] | B->WHITE[QUEEN] | B->WHITE[KING];
}

uint64_t blacks(const board *B) {
  return B->BLACK[PAWN] | B->BLACK[KNIGHT] | B->BLACK[BISHOP] | B->BLACK[ROOK] | B->BLACK[QUEEN] | B->BLACK[KING];
}

void print_board(const board *B) {
  uint64_t board = B->whites | B->blacks;
  binary_print(board);
}

void print_snapshot(const char *label, const board_snapshot *S) {
  const char *piece_names[NUM_PIECES] = {
    "PAWN", "KNIGHT", "BISHOP", "ROOK", "QUEEN", "KING"
  };

  printf("==== Snapshot: %s ====\n", label);
  printf("Side to move (white?): %d\n", S->white);
  printf("Castling rights: 0x%02x\n", S->castle);
  printf("CC flags : 0x%02x\n", S->cc);
  printf("Whites bb : 0x%016llx\n", (unsigned long long)S->whites);
  printf("Blacks bb : 0x%016llx\n", (unsigned long long)S->blacks);

  for (int p = 0; p < NUM_PIECES; ++p) {
    printf("WHITE[%s]: 0x%016llx\n", piece_names[p], (unsigned long long)S->WHITE[p]);
  }
  for (int p = 0; p < NUM_PIECES; ++p) {
    printf("BLACK[%s]: 0x%016llx\n", piece_names[p], (unsigned long long)S->BLACK[p]);
  }

  printf("Board (whites | blacks):\n");
  uint64_t all = S->whites | S->blacks;
  binary_print(all);
  printf("==== End Snapshot: %s ====\n", label);
}

void binary_print(uint64_t N) {
  int i;
  for (i = BOARD_SIZE - 1; i >= 0; --i) {
    uint64_t mask = ((uint64_t)1 << i);
    printf("%llu ", ((N & mask) >> i));
    if (i % 8 == 0) {
      printf("\n");
    }
  }
}

void free_board(board *B) {
  free(B->WHITE);
  free(B->BLACK);
  free (B->jumps);
  free(B);
}

void init_jump_table(uint64_t *jumps) {
  jumps[0] = 1ULL << 10ULL | 1ULL << 17ULL;
  jumps[1] = 1ULL << 11ULL | 1ULL << 16ULL | 1ULL << 18ULL;
  jumps[2] = 1ULL << 8ULL | 1ULL << 12ULL | 1ULL << 17ULL | 1ULL << 19ULL;
  jumps[3] = 1ULL << 9ULL | 1ULL << 13ULL | 1ULL << 18ULL | 1ULL << 20ULL;
  jumps[4] = 1ULL << 10ULL | 1ULL << 14ULL | 1ULL << 19ULL | 1ULL << 21ULL;
  jumps[5] = 1ULL << 11ULL | 1ULL << 15ULL | 1ULL << 20ULL | 1ULL << 22ULL;
  jumps[6] = 1ULL << 12ULL | 1ULL << 21ULL | 1ULL << 23ULL;
  jumps[7] = 1ULL << 13ULL | 1ULL << 22ULL;
  jumps[8] = 1ULL << 2ULL | 1ULL << 18ULL | 1ULL << 25ULL;
  jumps[9] = 1ULL << 3ULL | 1ULL << 19ULL | 1ULL << 24ULL | 1ULL << 26ULL;
  jumps[10] = 1ULL | 1ULL << 4ULL | 1ULL << 16ULL | 1ULL << 20ULL | 1ULL << 25ULL | 1ULL << 27ULL;
  jumps[11] = 1ULL << 1ULL | 1ULL << 5ULL | 1ULL << 17ULL | 1ULL << 21ULL | 1ULL << 26ULL | 1ULL << 28ULL;
  jumps[12] = 1ULL << 2ULL | 1ULL << 6ULL | 1ULL << 18ULL | 1ULL << 22ULL | 1ULL << 27ULL | 1ULL << 29ULL;
  jumps[13] = 1ULL << 3ULL | 1ULL << 7ULL | 1ULL << 19ULL | 1ULL << 23ULL | 1ULL << 28ULL | 1ULL << 30ULL;
  jumps[14] = 1ULL << 4ULL | 1ULL << 20ULL | 1ULL << 29ULL | 1ULL << 31ULL;
  jumps[15] = 1ULL << 5ULL | 1ULL << 21ULL | 1ULL << 30ULL;
  jumps[16] = 1ULL << 1ULL | 1ULL << 10ULL | 1ULL << 26ULL | 1ULL << 33ULL;
  jumps[17] = 1ULL | 1ULL << 2ULL | 1ULL << 11ULL | 1ULL << 27ULL | 1ULL << 32ULL | 1ULL << 34ULL;
  jumps[18] = 1ULL << 1ULL | 1ULL << 3ULL | 1ULL << 8ULL | 1ULL << 12ULL | 1ULL << 24ULL | 1ULL << 28ULL | 1ULL << 33ULL | 1ULL << 35ULL;
  jumps[19] = 1ULL << 2ULL | 1ULL << 4ULL | 1ULL << 9ULL | 1ULL << 13ULL | 1ULL << 25ULL | 1ULL << 29ULL | 1ULL << 34ULL | 1ULL << 36ULL;
  jumps[20] = 1ULL << 3ULL | 1ULL << 5ULL | 1ULL << 10ULL | 1ULL << 14ULL | 1ULL << 26ULL | 1ULL << 30ULL | 1ULL << 35ULL | 1ULL << 37ULL;
  jumps[21] = 1ULL << 4ULL | 1ULL << 6ULL | 1ULL << 11ULL | 1ULL << 15ULL | 1ULL << 27ULL | 1ULL << 31ULL | 1ULL << 36ULL | 1ULL << 38ULL;
  jumps[22] = 1ULL << 5ULL | 1ULL << 7ULL | 1ULL << 12ULL | 1ULL << 28ULL | 1ULL << 37ULL | 1ULL << 39ULL;
  jumps[23] = 1ULL << 6ULL | 1ULL << 13ULL | 1ULL << 29ULL | 1ULL << 38ULL;
  jumps[24] = 1ULL << 9ULL | 1ULL << 18ULL | 1ULL << 34ULL | 1ULL << 41ULL;
  jumps[25] = 1ULL << 8ULL | 1ULL << 10ULL | 1ULL << 19ULL | 1ULL << 35ULL | 1ULL << 40ULL | 1ULL << 42ULL;
  jumps[26] = 1ULL << 9ULL | 1ULL << 11ULL | 1ULL << 16ULL | 1ULL << 20ULL | 1ULL << 32ULL | 1ULL << 36ULL | 1ULL << 41ULL | 1ULL << 43ULL;
  jumps[27] = 1ULL << 10ULL | 1ULL << 12ULL | 1ULL << 17ULL | 1ULL << 21ULL | 1ULL << 33ULL | 1ULL << 37ULL | 1ULL << 42ULL | 1ULL << 44ULL;
  jumps[28] = 1ULL << 11ULL | 1ULL << 13ULL | 1ULL << 18ULL | 1ULL << 22ULL | 1ULL << 34ULL | 1ULL << 38ULL | 1ULL << 43ULL | 1ULL << 45ULL;
  jumps[29] = 1ULL << 12ULL | 1ULL << 14ULL | 1ULL << 19ULL | 1ULL << 23ULL | 1ULL << 35ULL | 1ULL << 39ULL | 1ULL << 44ULL | 1ULL << 46ULL;
  jumps[30] = 1ULL << 13ULL | 1ULL << 15ULL | 1ULL << 20ULL | 1ULL << 36ULL | 1ULL << 45ULL | 1ULL << 47ULL;
  jumps[31] = 1ULL << 14ULL | 1ULL << 21ULL | 1ULL << 37ULL | 1ULL << 46ULL;
  jumps[32] = 1ULL << 17ULL | 1ULL << 26ULL | 1ULL << 42ULL | 1ULL << 49ULL;
  jumps[33] = 1ULL << 16ULL | 1ULL << 18ULL | 1ULL << 27ULL | 1ULL << 43ULL | 1ULL << 48ULL | 1ULL << 50ULL;
  jumps[34] = 1ULL << 17ULL | 1ULL << 19ULL | 1ULL << 24ULL | 1ULL << 28ULL | 1ULL << 40ULL | 1ULL << 44ULL | 1ULL << 49ULL | 1ULL << 51ULL;
  jumps[35] = 1ULL << 18ULL | 1ULL << 20ULL | 1ULL << 25ULL | 1ULL << 29ULL | 1ULL << 41ULL | 1ULL << 45ULL | 1ULL << 50ULL | 1ULL << 52ULL;
  jumps[36] = 1ULL << 19ULL | 1ULL << 21ULL | 1ULL << 26ULL | 1ULL << 30ULL | 1ULL << 42ULL | 1ULL << 46ULL | 1ULL << 51ULL | 1ULL << 53ULL;
  jumps[37] = 1ULL << 20ULL | 1ULL << 22ULL | 1ULL << 27ULL | 1ULL << 31ULL | 1ULL << 43ULL | 1ULL << 47ULL | 1ULL << 52ULL | 1ULL << 54ULL;
  jumps[38] = 1ULL << 21ULL | 1ULL << 23ULL | 1ULL << 28ULL | 1ULL << 44ULL | 1ULL << 53ULL | 1ULL << 55ULL;
  jumps[39] = 1ULL << 22ULL | 1ULL << 29ULL | 1ULL << 45ULL | 1ULL << 54ULL;
  jumps[40] = 1ULL << 25ULL | 1ULL << 34ULL | 1ULL << 50ULL | 1ULL << 57ULL;
  jumps[41] = 1ULL << 24ULL | 1ULL << 26ULL | 1ULL << 35ULL | 1ULL << 51ULL | 1ULL << 56ULL | 1ULL << 58ULL;
  jumps[42] = 1ULL << 25ULL | 1ULL << 27ULL | 1ULL << 32ULL | 1ULL << 36ULL | 1ULL << 48ULL | 1ULL << 52ULL | 1ULL << 57ULL | 1ULL << 59ULL;
  jumps[43] = 1ULL << 26ULL | 1ULL << 28ULL | 1ULL << 33ULL | 1ULL << 37ULL | 1ULL << 49ULL | 1ULL << 53ULL | 1ULL << 58ULL | 1ULL << 60ULL;
  jumps[44] = 1ULL << 27ULL | 1ULL << 29ULL | 1ULL << 34ULL | 1ULL << 38ULL | 1ULL << 50ULL | 1ULL << 54ULL | 1ULL << 59ULL | 1ULL << 61ULL;
  jumps[45] = 1ULL << 28ULL | 1ULL << 30ULL | 1ULL << 35ULL | 1ULL << 39ULL | 1ULL << 51ULL | 1ULL << 55ULL | 1ULL << 60ULL | 1ULL << 62ULL;
  jumps[46] = 1ULL << 29ULL | 1ULL << 31ULL | 1ULL << 36ULL | 1ULL << 52ULL | 1ULL << 61ULL | 1ULL << 63ULL;
  jumps[47] = 1ULL << 30ULL | 1ULL << 37ULL | 1ULL << 53ULL | 1ULL << 62ULL;
  jumps[48] = 1ULL << 33ULL | 1ULL << 42ULL | 1ULL << 58ULL;
  jumps[49] = 1ULL << 32ULL | 1ULL << 34ULL | 1ULL << 43ULL | 1ULL << 59ULL;
  jumps[50] = 1ULL << 33ULL | 1ULL << 35ULL | 1ULL << 40ULL | 1ULL << 44ULL | 1ULL << 56ULL | 1ULL << 60ULL;
  jumps[51] = 1ULL << 34ULL | 1ULL << 36ULL | 1ULL << 41ULL | 1ULL << 45ULL | 1ULL << 57ULL | 1ULL << 61ULL;
  jumps[52] = 1ULL << 35ULL | 1ULL << 37ULL | 1ULL << 42ULL | 1ULL << 46ULL | 1ULL << 58ULL | 1ULL << 62ULL;
  jumps[53] = 1ULL << 36ULL | 1ULL << 38ULL | 1ULL << 43ULL | 1ULL << 47ULL | 1ULL << 59ULL | 1ULL << 63ULL;
  jumps[54] = 1ULL << 37ULL | 1ULL << 39ULL | 1ULL << 44ULL | 1ULL << 60ULL;
  jumps[55] = 1ULL << 38ULL | 1ULL << 45ULL | 1ULL << 61ULL;
  jumps[56] = 1ULL << 41ULL | 1ULL << 50ULL;
  jumps[57] = 1ULL << 40ULL | 1ULL << 42ULL | 1ULL << 51ULL;
  jumps[58] = 1ULL << 41ULL | 1ULL << 43ULL | 1ULL << 48ULL | 1ULL << 52ULL;
  jumps[59] = 1ULL << 42ULL | 1ULL << 44ULL | 1ULL << 49ULL | 1ULL << 53ULL;
  jumps[60] = 1ULL << 43ULL | 1ULL << 45ULL | 1ULL << 50ULL | 1ULL << 54ULL;
  jumps[61] = 1ULL << 44ULL | 1ULL << 46ULL | 1ULL << 51ULL | 1ULL << 55ULL;
  jumps[62] = 1ULL << 45ULL | 1ULL << 47ULL | 1ULL << 52ULL;
  jumps[63] = 1ULL << 46ULL | 1ULL << 53ULL;
}

uint64_t slide(uint64_t blocks, int square) {
  uint64_t attacks = 0;
  uint64_t mask;
  for (mask = (1ULL << (square + 9)); mask && (mask & ~FILE_H) && (mask & ~RANK_1); mask <<= 9) { // up-right
    attacks |= mask;
    if (mask & blocks) break;
  }
  for (mask = (1ULL << (square + 7)); mask && (mask & ~FILE_A) && (mask & ~RANK_1); mask <<= 7) { // up-left
    attacks |= mask;
    if (mask & blocks) break;
  }
  for (mask = (1ULL << (square - 7)); mask && (mask & ~FILE_H) && (mask & ~RANK_8); mask >>= 7) { // down-right
    attacks |= mask;
    if (mask & blocks) break;
  }
  for (mask = (1ULL << (square - 9)); mask && (mask & ~FILE_A) && (mask & ~RANK_8); mask >>= 9) { // down-left
    attacks |= mask;
    if (mask & blocks) break;
  }
  return attacks;
}

uint64_t translate(uint64_t blockers, int square) {
  uint64_t attacks = 0, mask;
  for (mask = (1ULL << (square + 1)) & ~FILE_H; mask; mask <<= 1) { // right
    attacks |= mask;
    if (mask & blockers) break;
  }
  for (mask = (1ULL << (square - 1)) & ~FILE_A; mask; mask >>= 1) { // left
    attacks |= mask;
    if (mask & blockers) break;
  }
  for (mask = (1ULL << (square + 8)) & ~RANK_1; mask; mask <<= 8) { // up
    attacks |= mask;
    if (mask & blockers) break;
  }
  for (mask = (1ULL << (square - 8)) & ~RANK_8; mask; mask >>= 8) { // down
    attacks |= mask;
    if (mask & blockers) break;
  }
  return attacks;
}

uint64_t queen(uint64_t blockers, int square) {
  uint64_t hm = translate(blockers, square);
  uint64_t dm = slide(blockers, square);
  return hm | dm;
}

uint64_t circle(int square) {
  uint64_t pos_mask = (1ULL << square);
  uint64_t moves = 0;
  if (pos_mask & ~RANK_8)
    moves |= (pos_mask << 8); // up
  if (pos_mask & ~RANK_1)
    moves |= (pos_mask >> 8); // down
  if (pos_mask & ~FILE_A) {
    moves |= (pos_mask << 7); // up left
    moves |= (pos_mask >> 1); // left
    moves |= (pos_mask >> 9); // down left
  }
  if (pos_mask & ~FILE_H) {
    moves |= (pos_mask << 9); // up right
    moves |= (pos_mask << 1); // right
    moves |= (pos_mask >> 7); // down right
  }
  return moves;
}

int piece_at(const board *B, int square) {
  int p;
  for (p = 0; p < NUM_PIECES; ++p)
    if ((B->WHITE[p] & (1ULL << square)) || (B->BLACK[p] & (1ULL << square))) return p;
  return -1;
}

int movegen(board* B, int white, move_t** move_list, int check_legal) {
  int max_moves = 256;
  *move_list = malloc(max_moves * sizeof(move_t));
  if (!*move_list) exit(1);
  int count = 0;
  uint64_t pieces = white ? B->whites : B->blacks;
  while (pieces) {
    int from = lsb(pieces);
    pieces &= pieces - 1;

    int piece = piece_at(B, from);
    uint64_t from_mask = 1ULL << from;
    uint64_t to_moves = imove(piece, from_mask, B, &white);
    while (to_moves) {
      int to = lsb(to_moves);
      to_moves &= to_moves - 1;

      undo_t u;
      int legal = 1;

      if (check_legal) {
        make_move(B, &(move_t){piece, from, to}, white, & u);
        if (check(B, !white)) {
          legal = 0; // king in check
        }
        unmake_move(B, &(move_t){piece, from, to}, white, & u);
      }

      if (!legal) continue;

      (*move_list)[count++] = (move_t){ piece, from, to };

      if (count >= max_moves) {
        max_moves *= 2;
        *move_list = realloc(*move_list, max_moves * sizeof(move_t));
        if (!*move_list) exit(1);
      }
    }
  }
  add_castles(B, white, move_list, &count, &max_moves);
  // qsort_r(*move_list, count, sizeof(move_t), (void*)B, &mvvlva_comp);
  return count;
}

int movegen_ply(board* B, int white, int check_legal, int ply, move_t** out, move_t(*move_stack)[MAX_MOVES], int max_moves) {
  move_t* list = move_stack[ply];
  int count = 0;
  uint64_t pieces = white ? B->whites : B->blacks;

  while (pieces) {
    int from = lsb(pieces);
    pieces &= pieces - 1;

    int piece = piece_at(B, from);
    uint64_t from_mask = 1ULL << from;
    uint64_t to_moves = imove(piece, from_mask, B, &white);

    while (to_moves && count < max_moves) {
      int to = lsb(to_moves);
      to_moves &= to_moves - 1;

      undo_t u;
      int legal = 1;
      if (check_legal) {
        make_move(B, &(move_t){piece, from, to}, white, & u);
        if (check(B, !white))
          legal = 0;
        unmake_move(B, &(move_t){piece, from, to}, white, & u);
      }

      if (!legal) continue;

      list[count++] = (move_t){ piece, from, to };
    }
  }

  add_castles_nalloc(B, white, list, &count, max_moves);
  *out = list;
  return count;
}

board *clone(board *B) {
  board* clone = (board*)malloc(sizeof(board));
  if (!clone) {
    fprintf(stderr, "Alloc failed\n");
    exit(1);
  }
  memcpy(clone, B, sizeof(board));

  clone->WHITE = (uint64_t*)malloc(sizeof(uint64_t) * NUM_PIECES);
  clone->BLACK = (uint64_t*)malloc(sizeof(uint64_t) * NUM_PIECES);
  clone->jumps = (uint64_t*)malloc(sizeof(uint64_t) * 64);
  if (!clone->WHITE || !clone->BLACK || !clone->jumps) {
    fprintf(stderr, "Alloc failed\n");
    free(clone->WHITE);
    free(clone->BLACK);
    free(clone->jumps);
    free(clone);
    exit(1);
  }
  
  memcpy(clone->WHITE, B->WHITE, sizeof(uint64_t) * NUM_PIECES);
  memcpy(clone->BLACK, B->BLACK, sizeof(uint64_t) * NUM_PIECES);
  memcpy(clone->jumps, B->jumps, sizeof(uint64_t) * 64);

  clone->white = B->white;
  clone->whites = B->whites;
  clone->blacks = B->blacks;
  return clone;
}

uint64_t sided_passed_pawns(uint64_t friend, uint64_t opp, int white) {
  uint64_t blockers;
  if (white) {
    blockers = (opp | (opp >> 1 & ~FILE_A) | (opp << 1 & ~FILE_H));
    blockers = blockers >> 8;
    blockers |= blockers >> 8;
    blockers |= blockers >> 16;
    blockers |= blockers >> 32;
    return friend & ~blockers;
  } else {
    blockers = (opp | (opp >> 1 & ~FILE_A) | (opp << 1 & ~FILE_H));
    blockers = blockers << 8;
    blockers |= blockers << 8;
    blockers |= blockers << 16;
    blockers |= blockers << 32;
    return friend & ~blockers;
  }
}

int value(int piece) {
  switch (piece) {
    case PAWN: return PAWN_VALUE;
    case KNIGHT: return KNIGHT_VALUE;
    case BISHOP: return BISHOP_VALUE;
    case ROOK: return ROOK_VALUE;
    case QUEEN: return QUEEN_VALUE;
    case KING: return KING_VALUE;
    default: return 0;
  }
}

void fast_execute(board* B, int piece, int from, int to, int white) {
  uint64_t from_mask = 1ULL << from;
  uint64_t to_mask = 1ULL << to;

  if (white) { // castle rights
    if (piece == KING) {
      B->castle &= ~(WKS | WQS);
    }
    else if (piece == ROOK) {
      if (from == H1) B->castle &= ~WKS;
      else if (from == A1) B->castle &= ~WQS;
    }
  } else {
    if (piece == KING) {
      B->castle &= ~(BKS | BQS);
    }
    else if (piece == ROOK) {
      if (from == H8) B->castle &= ~BKS;
      else if (from == A8) B->castle &= ~BQS;
    }
  }

  if (white) {
    // white moved, is black rook captured
    if (B->BLACK[ROOK] & to_mask) {
      if (to == H8) B->castle &= ~BKS;
      else if (to == A8) B->castle &= ~BQS;
    }
  }
  else {
    // black moved, is white rook captured
    if (B->WHITE[ROOK] & to_mask) {
      if (to == H1) B->castle &= ~WKS;
      else if (to == A1) B->castle &= ~WQS;
    }
  }

  if (white) {
    B->WHITE[piece] &= ~from_mask;
  } else {
    B->BLACK[piece] &= ~from_mask;
  }

  if (white) {
    for (int i = 0; i < NUM_PIECES; ++i) {
      B->BLACK[i] &= ~to_mask;
    }
  } else {
    for (int i = 0; i < NUM_PIECES; ++i) {
      B->WHITE[i] &= ~to_mask;
    }
  }

  if (white) {
    B->WHITE[piece] |= to_mask;
  } else {
    B->BLACK[piece] |= to_mask;
  }

  if (piece == KING && (from / 8 == to / 8) && (abs(to - from) == 2)) {
    if (white) {
      if (to == G1) { // white king side h1 -> f1
        B->cc |= WKS;
        B->WHITE[ROOK] &= ~(1ULL << H1);
        B->WHITE[ROOK] |= (1ULL << F1);
      } else { // white queen side a1 -> d1
        B->cc |= WQS;
        B->WHITE[ROOK] &= ~(1ULL << A1);
        B->WHITE[ROOK] |= (1ULL << D1);
      }
    } else {
      if (to == G8) { // black king side h8 -> f8
        B->cc |= BKS;
        B->BLACK[ROOK] &= ~(1ULL << H8);
        B->BLACK[ROOK] |= (1ULL << F8);
      } else { // black queen side a8 -> d8
        B->cc |= BQS;
        B->BLACK[ROOK] &= ~(1ULL << A8);
        B->BLACK[ROOK] |= (1ULL << D8);
      }
    }
  }

  B->whites = whites(B);
  B->blacks = blacks(B);
}

void make_move(board* B, const move_t* m, int side, undo_t* u) {
  uint64_t from_mask = 1ULL << m->from;
  uint64_t to_mask   = 1ULL << m->to;

  u->moved_piece = m->piece;
  u->from = m->from;
  u->to = m->to;

  u->prev_castle = B->castle;
  u->prev_cc = B->cc;

  u->captured_piece = -1;
  u->captured_square = m->to;

  if (side) { // white moved
    for (int i = 0; i < NUM_PIECES; ++i) {
      if (B->BLACK[i] & to_mask) {
        u->captured_piece = i;
        break;
      }
    }
  } else { // black moved
    for (int i = 0; i < NUM_PIECES; ++i) {
      if (B->WHITE[i] & to_mask) {
        u->captured_piece = i;
        break;
      }
    }
  }

  fast_execute(B, m->piece, m->from, m->to, side);
}

void unmake_move(board* B, const move_t* m, int side, const undo_t* u) {
  uint64_t from_mask = 1ULL << u->from;
  uint64_t to_mask   = 1ULL << u->to;

  B->castle = u->prev_castle;
  B->cc = u->prev_cc;

  if (side) { // white moved
    B->WHITE[u->moved_piece] &= ~to_mask;
    B->WHITE[u->moved_piece] |= from_mask;

    if (u->captured_piece != -1) {
      B->BLACK[u->captured_piece] |= (1ULL << u->captured_square);
    }
  } else { // black moved
    B->BLACK[u->moved_piece] &= ~to_mask;
    B->BLACK[u->moved_piece] |= from_mask;

    if (u->captured_piece != -1) {
      B->WHITE[u->captured_piece] |= (1ULL << u->captured_square);
    }
  }

  if (u->moved_piece == KING &&
    (u->from / 8 == u->to / 8) &&
    (abs(u->to - u->from) == 2)) {

    if (side) { // white castled
      if (u->to == G1) {
        B->WHITE[ROOK] &= ~(1ULL << F1);
        B->WHITE[ROOK] |= (1ULL << H1);
      }
      else if (u->to == C1) {
        B->WHITE[ROOK] &= ~(1ULL << D1);
        B->WHITE[ROOK] |= (1ULL << A1);
      }
    } else { // black castled
      if (u->to == G8) {
        B->BLACK[ROOK] &= ~(1ULL << F8);
        B->BLACK[ROOK] |= (1ULL << H8);
      }
      else if (u->to == C8) {
        B->BLACK[ROOK] &= ~(1ULL << D8);
        B->BLACK[ROOK] |= (1ULL << A8);
      }
    }
  }

  B->whites = whites(B);
  B->blacks = blacks(B);
}

uint64_t hash_board(const board* B) {
  uint64_t hash = 0;

  for (int i = 0; i < NUM_PIECES; ++i) {
    hash ^= B->WHITE[i] * 0x9e3779b97f4a7c15ULL;
    hash ^= B->BLACK[i] * 0xc3a5c85c97cb3127ULL;
  }

  hash ^= B->whites * 0x4cf5ad432745937fULL;
  hash ^= B->blacks * 0x94d049bb133111ebULL;
  hash ^= ((uint64_t)B->white) << 1;
  hash ^= ((uint64_t)B->castle) << 2;
  hash ^= ((uint64_t)B->cc) << 3;

  return hash;
}

uint64_t hash_snapshot(const board_snapshot* S) {
  uint64_t hash = 0;

  for (int i = 0; i < NUM_PIECES; ++i) {
    hash ^= S->WHITE[i] * 0x9e3779b97f4a7c15ULL;
    hash ^= S->BLACK[i] * 0xc3a5c85c97cb3127ULL;
  }

  hash ^= S->whites * 0x4cf5ad432745937fULL;
  hash ^= S->blacks * 0x94d049bb133111ebULL;
  hash ^= ((uint64_t)S->white) << 1;
  hash ^= ((uint64_t)S->castle) << 2;
  hash ^= ((uint64_t)S->cc) << 3;

  return hash;
}

void save_snapshot(const board *B, board_snapshot *S) {
  int i;
  for (i = 0; i < NUM_PIECES; ++i) {
    S->WHITE[i] = B->WHITE[i];
    S->BLACK[i] = B->BLACK[i];
  }
  S->whites = B->whites;
  S->blacks = B->blacks;
  S->white = B->white;
  S->castle = B->castle;
  S->cc = B->cc;
}

void restore_snapshot(board *B, const board_snapshot *S) {
  int i;
  for (i = 0; i < NUM_PIECES; i++) {
    B->WHITE[i] = S->WHITE[i];
    B->BLACK[i] = S->BLACK[i];
  }
  B->whites = S->whites;
  B->blacks = S->blacks;
  B->white = S->white;
  B->castle = S->castle;
  B->cc = S->cc;
}

int check(const board *B, int side) {
  uint64_t all = B->whites | B->blacks;
  uint64_t k = side ? B->BLACK[KING] : B->WHITE[KING];
  int ksq = lsb(k);

  // pawn
  uint64_t atkP = side ? ((B->WHITE[PAWN] & ~FILE_A) << 9) | ((B->WHITE[PAWN] & ~FILE_H) << 7) : ((B->BLACK[PAWN] & ~FILE_H) >> 7) | ((B->BLACK[PAWN] & ~FILE_A) >> 9);
  if (atkP & (1ULL << ksq)) return 1;

  // knight
  uint64_t knights = side ? B->WHITE[KNIGHT] : B->BLACK[KNIGHT];
  while (knights) {
    int sq = lsb(knights);
    knights &= knights - 1;
    if (B->jumps[sq] & (1ULL << ksq)) return 1;
  }

  // diagonal
  uint64_t sliders = (side ? B->WHITE[BISHOP] : B->BLACK[BISHOP]) | (side ? B->WHITE[QUEEN] : B->BLACK[QUEEN]);
  while (sliders) {
    int sq = lsb(sliders);
    sliders &= sliders - 1;
    if (generate_bishop_attacks(sq, all) & (1ULL << ksq)) return 1;
  }

  // orthogonal
  sliders = (side ? B->WHITE[ROOK] : B->BLACK[ROOK]) | (side ? B->WHITE[QUEEN] : B->BLACK[QUEEN]);
  while (sliders) {
    int sq = lsb(sliders);
    sliders &= sliders - 1;
    if (generate_rook_attacks(sq, all) & (1ULL << ksq)) return 1;
  }

  // king
  uint64_t oppk = side ? B->WHITE[KING] : B->BLACK[KING];
  int tsq = lsb(oppk);
  if (circle(tsq) & (1ULL << ksq)) return 1;

  return 0;
}

void restore_position(board *B, uint64_t *wpos, uint64_t *bpos, int *white) {
  int i;
  for (i = 0; i < NUM_PIECES; ++i) {
    B->WHITE[i] = wpos[i];
    B->BLACK[i] = bpos[i];
  }
  B->whites = whites(B);
  B->blacks = blacks(B);
  B->white = *white;
}

void load_position(board *B, const uint64_t *WHITE, const uint64_t *BLACK, int white) {
  for (int i = 0; i < NUM_PIECES; ++i) {
    B->WHITE[i] = 0ULL;
    B->BLACK[i] = 0ULL;
  }
  if (WHITE)
    for (int i = 0; i < NUM_PIECES; ++i) B->WHITE[i] = WHITE[i];
  if (BLACK)
    for (int i = 0; i < NUM_PIECES; ++i) B->BLACK[i] = BLACK[i];
  uint64_t w = 0ULL, b = 0ULL;
  for (int i = 0; i < NUM_PIECES; ++i) {
    w |= B->WHITE[i];
    b |= B->BLACK[i];
  }
  B->whites = w;
  B->blacks = b;
  B->white = white;
  assert((B->whites & B->blacks) == 0ULL);
  assert(B->WHITE[KING] && !(B->WHITE[KING] & (B->WHITE[KING]-1)));
  assert(B->BLACK[KING] && !(B->BLACK[KING] & (B->BLACK[KING]-1)));
}

static inline uint64_t white_attacks(const board *B) {
  const uint64_t occ = B->whites | B->blacks;
  uint64_t atk = 0ULL;

  // pawn
  uint64_t wp = B->WHITE[PAWN];
  atk |= ((wp & ~FILE_A) << 9) | ((wp & ~FILE_H) << 7);

  // knight
  uint64_t n = B->WHITE[KNIGHT];
  while (n) { int s = lsb(n); n &= n-1; atk |= B->jumps[s]; }

  // diagonal
  uint64_t bq = B->WHITE[BISHOP] | B->WHITE[QUEEN];
  while (bq) { int s = lsb(bq); bq &= bq-1; atk |= generate_bishop_attacks(s, occ); }

  // files/ranks
  uint64_t rq = B->WHITE[ROOK] | B->WHITE[QUEEN];
  while (rq) { int s = lsb(rq); rq &= rq-1; atk |= generate_rook_attacks(s, occ); }

  // king
  int ks = lsb(B->WHITE[KING]);
  atk |= circle(ks);

  return atk;
}

static inline uint64_t black_attacks(const board *B) {
  const uint64_t occ = B->whites | B->blacks;
  uint64_t atk = 0ULL;

  // pawn
  uint64_t bp = B->BLACK[PAWN];
  atk |= ((bp & ~FILE_H) >> 7) | ((bp & ~FILE_A) >> 9);

  // knights
  uint64_t n = B->BLACK[KNIGHT];
  while (n) { int s = lsb(n); n &= n-1; atk |= B->jumps[s]; }

  // diagonal
  uint64_t bq = B->BLACK[BISHOP] | B->BLACK[QUEEN];
  while (bq) { int s = lsb(bq); bq &= bq-1; atk |= generate_bishop_attacks(s, occ); }

  // files/ranks
  uint64_t rq = B->BLACK[ROOK] | B->BLACK[QUEEN];
  while (rq) { int s = lsb(rq); rq &= rq-1; atk |= generate_rook_attacks(s, occ); }

  // king
  int ks = lsb(B->BLACK[KING]);
  atk |= circle(ks);

  return atk;
}

static inline void add_castles(board *B, int white, move_t **list, int *count, int *max_moves) {
  const uint64_t occ = B->whites | B->blacks;
  if (white) {
    if (!(B->WHITE[KING] & (1ULL << E1))) return; // king not on e1
    const uint64_t opp = black_attacks(B);
    // white king side: e1 to g1
    if ((B->castle & WKS) && (B->WHITE[ROOK] & (1ULL << H1)) && /* rook on h1 */ !(occ & ((1ULL << F1) | (1ULL << G1))) && /* f1, g1 empty */ 
        !(opp & ((1ULL << E1) | (1ULL << F1) | (1ULL << G1)))) /* none attacked */ {
      if (*count >= *max_moves) {
        *max_moves *= 2;
        *list = realloc(*list, *max_moves * sizeof(move_t));
        if (!*list) exit(1);
      }
      (*list)[(*count)++] = (move_t){ KING, E1, G1 };
    }

    // white queen side: e1 to c1
    if ((B->castle & WQS) && (B->WHITE[ROOK] & (1ULL << A1)) && /* rook on a1 */ !(occ & ((1ULL << D1) | (1ULL << C1) | (1ULL << B1))) && /* d1, c1, b1 empty */
        !(opp & ((1ULL << E1) | (1ULL << D1) | (1ULL << C1)))) /* none attacked */ {
      if (*count >= *max_moves) {
        *max_moves *= 2;
        *list = realloc(*list, *max_moves * sizeof(move_t));
        if (!*list) exit(1);
      }
      (*list)[(*count)++] = (move_t){ KING, E1, C1 };
    }
  } else {
    if (!(B->BLACK[KING] & (1ULL << E8))) return; // king not on e8
    const uint64_t opp = white_attacks(B);

    // black king side: e8 to g8
    if ((B->castle & BKS) && (B->BLACK[ROOK] & (1ULL << H8)) && !(occ & ((1ULL << F8) | (1ULL << G8))) &&
        !(opp & ((1ULL << E8) | (1ULL << F8) | (1ULL << G8)))) {
      if (*count >= *max_moves) {
        *max_moves *= 2;
        *list = realloc(*list, *max_moves * sizeof(move_t));
        if (!*list) exit(1);
      }
      (*list)[(*count)++] = (move_t){ KING, E8, G8 };
    }

    // black queen side: e8 to c8
    if ((B->castle & BQS) && (B->BLACK[ROOK] & (1ULL << A8)) && !(occ & ((1ULL << D8) | (1ULL << C8) | (1ULL << B8))) &&
        !(opp & ((1ULL << E8) | (1ULL << D8) | (1ULL << C8)))) {
      if (*count >= *max_moves) {
        *max_moves *= 2;
        *list = realloc(*list, *max_moves * sizeof(move_t));
        if (!*list) exit(1);
      }
      (*list)[(*count)++] = (move_t){ KING, E8, C8 };
    }
  }
}

static inline void add_castles_nalloc(board *B, int white, move_t *list, int *count, int max_moves) {
  const uint64_t occ = B->whites | B->blacks;

  if (white) {
    if (!(B->WHITE[KING] & (1ULL << E1))) return;
    const uint64_t opp = black_attacks(B);

    // white king side
    if ((B->castle & WKS) &&
      (B->WHITE[ROOK] & (1ULL << H1)) &&
      !(occ & ((1ULL << F1) | (1ULL << G1))) &&
      !(opp & ((1ULL << E1) | (1ULL << F1) | (1ULL << G1)))) {

      if (*count < max_moves) {
        list[(*count)++] = (move_t){ KING, E1, G1 };
      }
    }

    // white queen side
    if ((B->castle & WQS) &&
      (B->WHITE[ROOK] & (1ULL << A1)) &&
      !(occ & ((1ULL << D1) | (1ULL << C1) | (1ULL << B1))) &&
      !(opp & ((1ULL << E1) | (1ULL << D1) | (1ULL << C1)))) {

      if (*count < max_moves) {
        list[(*count)++] = (move_t){ KING, E1, C1 };
      }
    }
  } else {
    if (!(B->BLACK[KING] & (1ULL << E8))) return;
    const uint64_t opp = white_attacks(B);

    // black king side
    if ((B->castle & BKS) &&
      (B->BLACK[ROOK] & (1ULL << H8)) &&
      !(occ & ((1ULL << F8) | (1ULL << G8))) &&
      !(opp & ((1ULL << E8) | (1ULL << F8) | (1ULL << G8)))) {

      if (*count < max_moves) {
        list[(*count)++] = (move_t){ KING, E8, G8 };
      }
    }

    // black queen side
    if ((B->castle & BQS) &&
      (B->BLACK[ROOK] & (1ULL << A8)) &&
      !(occ & ((1ULL << D8) | (1ULL << C8) | (1ULL << B8))) &&
      !(opp & ((1ULL << E8) | (1ULL << D8) | (1ULL << C8)))) {

      if (*count < max_moves) {
        list[(*count)++] = (move_t){ KING, E8, C8 };
      }
    }
  }
}

int square_attacker(const board *B_in, int square, int by_black) {
  board B = *B_in;
  board_snapshot S; save_snapshot(B_in, &S);
  if (by_black) {
    B.WHITE[KING] = (1ULL << square);
    B.whites = whites(&B);
    int attacked = check(&B, 0); // test white king
    restore_snapshot((board*)B_in, &S);
    return attacked;
  } else {
    B.BLACK[KING] = (1ULL << square);
    B.blacks = blacks(&B);
    int attacked = check(&B, 1); // test black king
    restore_snapshot((board*)B_in, &S);
    return attacked;
  }
}