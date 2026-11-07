#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "include/board.h"
#include "include/eval.h"
#include "include/utils.h"

const int PIECE_VALUES[NUM_PIECES] = {100, 320, 330, 500, 900, 20000};
const int END_VALUES[NUM_PIECES] = {120, 310, 340, 500, 900, 20000};
const int TOTAL_PHASE = ((KNIGHT_PHASE * 4) + (BISHOP_PHASE * 4) + (ROOK_PHASE * 4) + (QUEEN_PHASE * 2));

uint64_t wmoves;
uint64_t bmoves;

int tapered(const board *B) {
  wmoves = white_moves(B);
  bmoves = black_moves(B);

  int mg = mid_eval(B);
  int eg = end_eval(B);
  int p = phase(B);
  eg = eg * scale(B, eg) / 64;
  int v = (((mg * p + (eg * (128 - p))) / 128) << 0);
  return v;
}

int mat_eval(const board *B) {
  int score = PIECE_VALUES[PAWN] * (__builtin_popcountll(B->WHITE[PAWN]) - __builtin_popcountll(B->BLACK[PAWN]));
  score += PIECE_VALUES[KNIGHT] * (__builtin_popcountll(B->WHITE[KNIGHT]) - __builtin_popcountll(B->BLACK[KNIGHT]));
  score += PIECE_VALUES[BISHOP] * (__builtin_popcountll(B->WHITE[BISHOP]) - __builtin_popcountll(B->BLACK[BISHOP]));
  score += PIECE_VALUES[ROOK] * (__builtin_popcountll(B->WHITE[ROOK]) - __builtin_popcountll(B->BLACK[ROOK]));
  score += PIECE_VALUES[QUEEN] * (__builtin_popcountll(B->WHITE[QUEEN]) - __builtin_popcountll(B->BLACK[QUEEN]));
  score += PIECE_VALUES[KING] * (__builtin_popcountll(B->WHITE[KING]) - __builtin_popcountll(B->BLACK[KING]));
  return score;
}

int center_control(const board *B) {
  const uint64_t CENTER = (FILE_D | FILE_E) & (RANK_4 | RANK_5);
  uint64_t wcont = B->whites & CENTER;
  uint64_t bcont = B->blacks & CENTER;
  return 2 * (__builtin_popcountll(wcont) - __builtin_popcountll(bcont));
}

int king_safe(const board *B) {
  uint64_t wking = wk_moves(B->WHITE[KING], B->whites);
  uint64_t bking = bk_moves(B->BLACK[KING], B->blacks);
  int wsafe = __builtin_popcountll(wking & bmoves);
  int bsafe = __builtin_popcountll(bking & wmoves);
  return (bsafe - wsafe);
}

int mobility(const board *B) {
  int wmobile = __builtin_popcountll(wmoves);
  int bmobile = __builtin_popcountll(bmoves);
  return (wmobile - bmobile) / 2;
}

int mid_eval(const board *B) {
  return mat_eval(B) + center_control(B) + king_safe(B) + mobility(B);
}

int phase(const board *B) {
  int phase = 0;
  phase += __builtin_popcountll(B->WHITE[KNIGHT]) * KNIGHT_PHASE;
  phase += __builtin_popcountll(B->BLACK[KNIGHT]) * KNIGHT_PHASE;
  phase += __builtin_popcountll(B->WHITE[BISHOP]) * BISHOP_PHASE;
  phase += __builtin_popcountll(B->BLACK[BISHOP]) * BISHOP_PHASE;
  phase += __builtin_popcountll(B->WHITE[ROOK]) * ROOK_PHASE;
  phase += __builtin_popcountll(B->BLACK[ROOK]) * ROOK_PHASE;
  phase += __builtin_popcountll(B->WHITE[QUEEN]) * QUEEN_PHASE;
  phase += __builtin_popcountll(B->BLACK[QUEEN]) * QUEEN_PHASE;

  if (phase > TOTAL_PHASE)
    phase = TOTAL_PHASE;
  return (phase * 128) / TOTAL_PHASE;
}

int scale(const board *B, int eg_score) {
  int no_wpawns = (__builtin_popcountll(B->WHITE[PAWN]) == 0);
  int no_bpawns = (__builtin_popcountll(B->BLACK[PAWN]) == 0);
  if (no_wpawns && no_bpawns) return HALF_SCALE;
  int wking_mobile = B->WHITE[KING] & ~RANK_1 & ~RANK_2;
  int bking_mobile = B->BLACK[KING] & ~RANK_8 & ~RANK_7;
  if (wking_mobile && bking_mobile) return MAX_SCALE;
  return MOD_SCALE;
}

int end_mat_eval(const board *B) {
  int score = END_VALUES[PAWN] * (__builtin_popcountll(B->WHITE[PAWN]) - __builtin_popcountll(B->BLACK[PAWN]));
  score += END_VALUES[KNIGHT] * (__builtin_popcountll(B->WHITE[KNIGHT]) - __builtin_popcountll(B->BLACK[KNIGHT]));
  score += END_VALUES[BISHOP] * (__builtin_popcountll(B->WHITE[BISHOP]) - __builtin_popcountll(B->BLACK[BISHOP]));
  score += END_VALUES[ROOK] * (__builtin_popcountll(B->WHITE[ROOK]) - __builtin_popcountll(B->BLACK[ROOK]));
  score += END_VALUES[QUEEN] * (__builtin_popcountll(B->WHITE[QUEEN]) - __builtin_popcountll(B->BLACK[QUEEN]));
  score += END_VALUES[KING] * (__builtin_popcountll(B->WHITE[KING]) - __builtin_popcountll(B->BLACK[KING]));
  return score;
}

int king_activity(const board *B) {
  uint64_t mid_ranks = RANK_3 | RANK_4 | RANK_5 | RANK_6;
  int wking = (B->WHITE[KING] & mid_ranks) != 0;
  int bking = (B->BLACK[KING] & mid_ranks) != 0;
  int w = wking ? (ACTIVE_KING) : 0;
  int b = bking ? (ACTIVE_KING) : 0;
  return (w - b);
}

int pawn_structure(const board *B) {
  const uint64_t WPROMOTE = RANK_7;
  const uint64_t BPROMOTE = RANK_2;
  uint64_t wp = B->WHITE[PAWN] & WPROMOTE;
  uint64_t bp = B->BLACK[PAWN] & BPROMOTE;
  return ((__builtin_popcountll(wp) - __builtin_popcountll(bp)) * (PAWN_PROMOTE));
}

int passed_pawns(const board *B) {
  uint64_t wp = B->WHITE[PAWN];
  uint64_t bp = B->BLACK[PAWN];
  uint64_t wpass = sided_passed_pawns(wp, bp, 1);
  uint64_t bpass = sided_passed_pawns(bp, wp, 0);
  return (PAWN_PASSED * (__builtin_popcountll(wpass) - __builtin_popcountll(bpass)));
}

int development(const board *B) {
  int score = 0;
  uint64_t wminors = B->WHITE[KNIGHT] | B->WHITE[BISHOP]; // white minor pieces
  int w_undeveloped = __builtin_popcountll(wminors & RANK_1);
  uint64_t bminors = B->BLACK[KNIGHT] | B->BLACK[BISHOP]; // black minor pieces
  int b_undeveloped = __builtin_popcountll(bminors & RANK_8);
  score -= DEV_PENALTY * w_undeveloped;
  score += DEV_PENALTY * b_undeveloped;
  return score;
}

int castle_eval(const board *B) {
  int score = 0;
  if (B->cc & (WKS | WQS)) // white castled
    score += CASTLE_PT;
  if (B->cc & (BKS | BQS)) // black castled
    score -= CASTLE_PT;
  return score;
}

int end_eval(const board *B) {
  return end_mat_eval(B) + king_activity(B) + pawn_structure(B) + passed_pawns(B);
}

static const uint64_t FILES[8] = { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };
static const uint64_t ADJ_FILES[8] = {
  FILE_B,
  FILE_A | FILE_C,
  FILE_B | FILE_D,
  FILE_C | FILE_E,
  FILE_D | FILE_F,
  FILE_E | FILE_G,
  FILE_F | FILE_H,
  FILE_G
};

static inline int pop_lsb(uint64_t *bb) {
  int s = __builtin_ctzll(*bb);
  *bb &= (*bb - 1);
  return s;
}

void init_pesto_tables(void) {
  for (int pt = PAWN; pt <= KING; ++pt) {
    int wpc = PCODE(pt, CONST_WHITE);
    int bpc = PCODE(pt, CONST_BLACK);
    for (int sq = 0; sq < 64; ++sq) {
      // Tables are stored rank-8-first; white needs FLIP(sq) to get the correct rank.
      int wsq = FLIP(sq);
      mg_table[wpc][sq] = mg_value[pt] + (pt == PAWN ? mg_pawn_table[wsq] : pt == KNIGHT ? mg_knight_table[wsq] : pt == BISHOP ? mg_bishop_table[wsq] : pt == ROOK ? mg_rook_table[wsq] : pt == QUEEN ? mg_queen_table[wsq] : mg_king_table[wsq]);
      eg_table[wpc][sq] = eg_value[pt] + (pt == PAWN ? eg_pawn_table[wsq] : pt == KNIGHT ? eg_knight_table[wsq] : pt == BISHOP ? eg_bishop_table[wsq] : pt == ROOK ? eg_rook_table[wsq] : pt == QUEEN ? eg_queen_table[wsq] : eg_king_table[wsq]);

      // Black uses sq directly (rank-8-first = black's near-promotion rank).
      mg_table[bpc][sq] = mg_value[pt] + (pt == PAWN ? mg_pawn_table[sq] : pt == KNIGHT ? mg_knight_table[sq] : pt == BISHOP ? mg_bishop_table[sq] : pt == ROOK ? mg_rook_table[sq] : pt == QUEEN ? mg_queen_table[sq] : mg_king_table[sq]);
      eg_table[bpc][sq] = eg_value[pt] + (pt == PAWN ? eg_pawn_table[sq] : pt == KNIGHT ? eg_knight_table[sq] : pt == BISHOP ? eg_bishop_table[sq] : pt == ROOK ? eg_rook_table[sq] : pt == QUEEN ? eg_queen_table[sq] : eg_king_table[sq]);
    }
  }
}

void pesto_terms(const board *B, int *mg, int *eg, int *p24) {
  int mgW = 0, mgB = 0;
  int egW = 0, egB = 0;
  int gp = 0;

  for (int pt = PAWN; pt <= KING; ++pt) {
    // white
    uint64_t bb = B->WHITE[pt];
    int wpc = PCODE(pt, CONST_WHITE);
    while (bb) {
      int sq = pop_lsb(&bb);
      mgW += mg_table[wpc][sq];
      egW += eg_table[wpc][sq];
      gp  += gpi[wpc];
    }
    // black
    bb = B->BLACK[pt];
    int bpc = PCODE(pt, CONST_BLACK);
    while (bb) {
      int sq = pop_lsb(&bb);
      mgB += mg_table[bpc][sq];
      egB += eg_table[bpc][sq];
      gp  += gpi[bpc];
    }
  }

  if (gp > 24) gp = 24;
  *mg = (mgW - mgB);
  *eg = (egW - egB);
  *p24 = gp;
}

int mating_bonus(const board *B) {
  int bksq = __builtin_ctzll(B->BLACK[KING]);
  int wksq = __builtin_ctzll(B->WHITE[KING]);
  int bkr = bksq / 8, bkf = bksq % 8;
  int wkr = wksq / 8, wkf = wksq % 8;
  int bmin = (bkr < 4 ? bkr : 7 - bkr) + (bkf < 4 ? bkf : 7 - bkf);
  // reward black king near edge regardless of white king position
  int edge = MATING_EDGE_BONUS * (6 - bmin);
  int prox = MATING_PROX_BONUS * (14 - (abs(bkr - wkr) + abs(bkf - wkf))) / 7;
  return edge + prox;
}

int rook_file_bonus(const board *B) {
  int score = 0;
  for (int i = 0; i < 8; i++) {
    int wrook = (B->WHITE[ROOK] & FILES[i]) != 0;
    int brook = (B->BLACK[ROOK] & FILES[i]) != 0;
    if (!wrook && !brook) continue;
    int wpawn = (B->WHITE[PAWN] & FILES[i]) != 0;
    int bpawn = (B->BLACK[PAWN] & FILES[i]) != 0;
    if (wrook) {
      if (!wpawn && !bpawn) score += ROOK_OPEN_FILE;
      else if (!wpawn) score += ROOK_SEMI_OPEN;
    }
    if (brook) {
      if (!wpawn && !bpawn) score -= ROOK_OPEN_FILE;
      else if (!bpawn) score -= ROOK_SEMI_OPEN;
    }
  }
  return score;
}

int pawn_weaknesses(const board *B) {
  int score = 0;
  for (int i = 0; i < 8; i++) {
    int wc = __builtin_popcountll(B->WHITE[PAWN] & FILES[i]);
    int bc = __builtin_popcountll(B->BLACK[PAWN] & FILES[i]);
    if (wc > 1) score -= DOUBLED_PAWN_PENALTY * (wc - 1);
    if (bc > 1) score += DOUBLED_PAWN_PENALTY * (bc - 1);
    int wpawn = (B->WHITE[PAWN] & FILES[i]) != 0;
    int bpawn = (B->BLACK[PAWN] & FILES[i]) != 0;
    if (wpawn && !(B->WHITE[PAWN] & ADJ_FILES[i])) score -= ISOLATED_PAWN_PENALTY;
    if (bpawn && !(B->BLACK[PAWN] & ADJ_FILES[i])) score += ISOLATED_PAWN_PENALTY;
  }
  return score;
}

int bishop_pair_bonus(const board *B) {
  int score = 0;
  if (__builtin_popcountll(B->WHITE[BISHOP]) >= 2) score += BISHOP_PAIR_BONUS;
  if (__builtin_popcountll(B->BLACK[BISHOP]) >= 2) score -= BISHOP_PAIR_BONUS;
  return score;
}

int rook_seventh_bonus(const board *B) {
  int score = 0;
  if (B->WHITE[ROOK] & RANK_7) score += ROOK_SEVENTH_BONUS;
  if (B->BLACK[ROOK] & RANK_2) score -= ROOK_SEVENTH_BONUS;
  return score;
}

int king_pawn_shield(const board *B) {
  int score = 0;
  // white king: only if not on starting square (already moved/castled)
  if (B->WHITE[KING] && !(B->WHITE[KING] & (1ULL << E1))) {
    int ksq = __builtin_ctzll(B->WHITE[KING]);
    int kr = ksq / 8;
    if (kr <= 1) { // king on rank 1 or 2 (kingside/queenside castle area)
      int kf = ksq % 8;
      for (int df = -1; df <= 1; df++) {
        int f = kf + df;
        if (f < 0 || f > 7) continue;
        if (B->WHITE[PAWN] & FILES[f] & RANK_2) score += KING_SHIELD_RANK1;
        else if (B->WHITE[PAWN] & FILES[f] & RANK_3) score += KING_SHIELD_RANK2;
        else score -= KING_OPEN_FILE_PEN;  // open file next to castled king
      }
    }
  }
  // black king: only if not on starting square
  if (B->BLACK[KING] && !(B->BLACK[KING] & (1ULL << E8))) {
    int ksq = __builtin_ctzll(B->BLACK[KING]);
    int kr = ksq / 8;
    if (kr >= 6) { // king on rank 7 or 8 (kingside/queenside castle area)
      int kf = ksq % 8;
      for (int df = -1; df <= 1; df++) {
        int f = kf + df;
        if (f < 0 || f > 7) continue;
        if (B->BLACK[PAWN] & FILES[f] & RANK_7) score -= KING_SHIELD_RANK1;
        else if (B->BLACK[PAWN] & FILES[f] & RANK_6) score -= KING_SHIELD_RANK2;
        else score += KING_OPEN_FILE_PEN;  // open file next to castled king
      }
    }
  }
  return score;
}

int blended_eval(const board *B) {
  wmoves = white_moves(B);
  bmoves = black_moves(B);

  // PeSTO
  int mg_psqt, eg_psqt, phase24;
  pesto_terms(B, &mg_psqt, &eg_psqt, &phase24);
  int mgPhase = phase24;
  int egPhase = 24 - mgPhase;

  // heuristics
  int mob = mobility(B);
  int ctr = center_control(B);
  int ks = king_safe(B);
  int ka = king_activity(B);
  int pstr = pawn_structure(B);
  int pps = passed_pawns(B);
  int castle = castle_eval(B);
  int dev = development(B);
  int mat = mating_bonus(B);
  int rfile = rook_file_bonus(B);
  int pweak = pawn_weaknesses(B);
  int bpair = bishop_pair_bonus(B);
  int r7th = rook_seventh_bonus(B);
  int kshield = king_pawn_shield(B);

  // feature sums
  int mg_feats = (MOBILITY_MG * mob) + (CENTER_MG * ctr) + (KING_SAFETY_MG * ks) + castle + (DEV_MG * dev) + rfile + kshield + pweak;
  int eg_feats = (MOBILITY_EG * mob) + (CENTER_EG * ctr) + (KING_ACTIVITY_EG * ka) + (PSTRUCT_EG * pstr) + (PASSED_EG * pps) + mat + bpair + r7th + pweak;

  int mg_total = mg_psqt + mg_feats;
  int eg_total = eg_psqt + eg_feats;

  // scale
  int eg_scaled = (eg_total * scale(B, eg_total)) / 64;
  int score = (mg_total * mgPhase + eg_scaled * egPhase) / 24;

  // tempo
  if (B->white) score += TEMPO_BONUS;
  else score -= TEMPO_BONUS;

  return score;
}