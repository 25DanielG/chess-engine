#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "lib/see.h"
#include "lib/board.h"
#include "lib/magic.h"

static const int SEE_VALUES[NUM_PIECES] = {
  SEE_PAWN, SEE_KNIGHT, SEE_BISHOP, SEE_ROOK, SEE_QUEEN, SEE_KING
};

static inline int pop_lsb_see(uint64_t *bb) {
  int sq = __builtin_ctzll(*bb);
  *bb &= *bb - 1;
  return sq;
}

static inline uint64_t pawn_attackers(int sq, uint64_t wpawns, uint64_t bpawns) {
  uint64_t sq_bb = 1ULL << sq;
  uint64_t attackers = 0;

  if (sq >= 9)  attackers |= wpawns & ((sq_bb >> 9) & ~FILE_H); // white pawn SW
  if (sq >= 7)  attackers |= wpawns & ((sq_bb >> 7) & ~FILE_A); // white pawn SE
  if (sq <= 54) attackers |= bpawns & ((sq_bb << 9) & ~FILE_A); // black pawn NE
  if (sq <= 56) attackers |= bpawns & ((sq_bb << 7) & ~FILE_H); // black pawn NW

  return attackers;
}

uint64_t attackers_to(const board *B, int sq, uint64_t occ) {
  uint64_t attackers = 0;

  uint64_t sq_bb = 1ULL << sq;
  attackers |= ((sq_bb >> 9) & ~FILE_H & B->WHITE[PAWN]);
  attackers |= ((sq_bb >> 7) & ~FILE_A & B->WHITE[PAWN]);
  attackers |= ((sq_bb << 9) & ~FILE_A & B->BLACK[PAWN]);
  attackers |= ((sq_bb << 7) & ~FILE_H & B->BLACK[PAWN]);

  uint64_t knight_attacks = B->jumps[sq];
  attackers |= knight_attacks & (B->WHITE[KNIGHT] | B->BLACK[KNIGHT]);

  uint64_t bishop_attacks = generate_bishop_attacks(sq, occ);
  attackers |= bishop_attacks & (B->WHITE[BISHOP] | B->BLACK[BISHOP] | B->WHITE[QUEEN] | B->BLACK[QUEEN]);

  uint64_t rook_attacks = generate_rook_attacks(sq, occ);
  attackers |= rook_attacks & (B->WHITE[ROOK] | B->BLACK[ROOK] | B->WHITE[QUEEN] | B->BLACK[QUEEN]);

  uint64_t king_attacks = circle(sq);
  attackers |= king_attacks & (B->WHITE[KING] | B->BLACK[KING]);

  return attackers;
}

int least_valuable_attacker(const board *B, uint64_t attackers, int side, int *sq_out) {
  uint64_t *pieces = side ? B->WHITE : B->BLACK;

  for (int pt = PAWN; pt <= KING; pt++) { // order of inc value
    uint64_t candidates = attackers & pieces[pt];
    if (candidates) {
      *sq_out = __builtin_ctzll(candidates);
      return pt;
    }
  }

  *sq_out = -1;
  return -1;
}

static inline int piece_on_square(const board *B, int sq, int side) {
  uint64_t mask = 1ULL << sq;
  uint64_t *pieces = side ? B->WHITE : B->BLACK;

  for (int pt = PAWN; pt <= KING; pt++) {
    if (pieces[pt] & mask) return pt;
  }
  return -1;
}

int see(const board *B, const move_t *m, int side) { // full static exchange evaluation, expected material change centipanwns
  int from = m->from;
  int to = m->to;
  int attacker = m->piece;

  int victim = piece_on_square(B, to, !side);
  if (victim < 0) {
    return 0;
  }

  int gain[32]; // swap list (depth limit 31) track gains
  int depth = 0;

  gain[depth] = SEE_VALUES[victim];

  if (m->promo != 0) { // promotion handling
    gain[depth] += SEE_VALUES[m->promo] - SEE_VALUES[PAWN];
    attacker = m->promo;
  }

  uint64_t occ = B->whites | B->blacks; // capture sequence
  uint64_t from_bb = 1ULL << from;
  uint64_t to_bb = 1ULL << to;

  occ ^= from_bb;
  uint64_t attackers = attackers_to(B, to, occ);
  uint64_t used = from_bb;
  int stm = !side;
  int piece_on_sq = attacker;

  while (1) {
    depth++;

    gain[depth] = SEE_VALUES[piece_on_sq] - gain[depth - 1]; // previous loss + value of cap
    uint64_t stm_attackers = attackers & (stm ? B->whites : B->blacks) & ~used;

    if (!stm_attackers) break; // no more

    // max(-gain[d-1], gain[d]) where gain[d] = piece - gain[d-1]
    // gain[d] < 0 && -gain[d-1] < 0, neither side wants to continue
    if (gain[depth] < 0 && -gain[depth - 1] < 0) break;

    int atk_sq;
    int atk_piece = least_valuable_attacker(B, stm_attackers, stm, &atk_sq);

    if (atk_piece < 0) break;
    if (atk_piece == KING) { // handle king capture and check
      uint64_t opp_attackers = attackers & (stm ? B->blacks : B->whites) & ~used;
      if (opp_attackers) break;
    }

    used |= (1ULL << atk_sq);
    occ ^= (1ULL << atk_sq);

    if (atk_piece == PAWN || atk_piece == BISHOP || atk_piece == QUEEN) {
      attackers |= generate_bishop_attacks(to, occ) & (B->WHITE[BISHOP] | B->BLACK[BISHOP] | B->WHITE[QUEEN] | B->BLACK[QUEEN]);
    }
    if (atk_piece == ROOK || atk_piece == QUEEN) {
      attackers |= generate_rook_attacks(to, occ) & (B->WHITE[ROOK] | B->BLACK[ROOK] | B->WHITE[QUEEN] | B->BLACK[QUEEN]);
    }

    piece_on_sq = atk_piece;
    stm = !stm;

    if (depth >= 31) break; // limit in case
  }

  // negamax the swap list
  while (--depth) {
    gain[depth - 1] = -(-gain[depth - 1] > gain[depth] ? -gain[depth - 1] : gain[depth]);
  }

  return gain[0];
}

int see_ge(const board *B, const move_t *m, int side, int threshold) { // optimized see threshold
  int from = m->from;
  int to = m->to;
  int attacker = m->piece;

  int victim = piece_on_square(B, to, !side);
  int initial_gain = (victim >= 0) ? SEE_VALUES[victim] : 0;

  if (m->promo != 0) {
    initial_gain += SEE_VALUES[m->promo] - SEE_VALUES[PAWN];
    attacker = m->promo;
  }

  int balance = initial_gain - threshold; // fail if capture gain < threshold
  if (balance < 0) return 0;
  balance -= SEE_VALUES[attacker]; // opp captures, meet threshold success
  if (balance >= 0) return 1;

  uint64_t occ = (B->whites | B->blacks) ^ (1ULL << from);
  uint64_t attackers = attackers_to(B, to, occ);

  uint64_t bishops = B->WHITE[BISHOP] | B->BLACK[BISHOP];
  uint64_t rooks = B->WHITE[ROOK] | B->BLACK[ROOK];
  uint64_t queens = B->WHITE[QUEEN] | B->BLACK[QUEEN];

  int stm = !side;

  while (1) {
    uint64_t stm_attackers = attackers & (stm ? B->whites : B->blacks);

    if (!stm_attackers) { // prev side wins exchange
      // if stm == side, opp wins return 0
      // if stm != side, we win return 1
      return stm != side;
    }

    int atk_sq = -1;
    int atk_piece = -1;
    uint64_t *pieces = stm ? B->WHITE : B->BLACK;

    for (int pt = PAWN; pt <= KING; pt++) {
      uint64_t candidates = stm_attackers & pieces[pt];
      if (candidates) {
        atk_sq = __builtin_ctzll(candidates);
        atk_piece = pt;
        break;
      }
    }

    if (atk_piece < 0) {
      return stm != side;
    }

    if (atk_piece == KING) {
      if (attackers & (stm ? B->blacks : B->whites)) {
        return stm != side;
      }
    }

    occ ^= (1ULL << atk_sq);

    if (atk_piece == PAWN || atk_piece == BISHOP || atk_piece == QUEEN) {
      attackers |= generate_bishop_attacks(to, occ) & (bishops | queens);
    }
    if (atk_piece == ROOK || atk_piece == QUEEN) {
      attackers |= generate_rook_attacks(to, occ) & (rooks | queens);
    }

    attackers &= occ;
    stm = !stm;
    balance = -balance - 1 - SEE_VALUES[atk_piece];

    if (balance >= 0) { // negamax pruning, cur side at threshold
      return stm == side;
    }
  }
}

int see_value(int piece) {
  if (piece < 0 || piece >= NUM_PIECES) return 0;
  return SEE_VALUES[piece];
}