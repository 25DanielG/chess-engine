#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "lib/board.h"
#include "lib/eval.h"

const int PIECE_VALUES[NUM_PIECES] = {100, 320, 330, 500, 900, 20000};
const int END_VALUES[NUM_PIECES] = {120, 310, 340, 500, 900, 20000};
const int TOTAL_PHASE = ((KNIGHT_PHASE * 4) + (BISHOP_PHASE * 4) + (ROOK_PHASE * 4) + (QUEEN_PHASE * 2));

uint64_t wmoves;
uint64_t bmoves;

int tapered(board *B) {
  wmoves = white_moves(B);
  bmoves = black_moves(B);

  int mg = mid_eval(B);
  int eg = end_eval(B);
  int p = phase(B);
  eg = eg * scale(B, eg) / 64;
  int v = (((mg * p + (eg * (128 - p))) / 128) << 0);
  return v;
}

int mat_eval(board *B) {
  int score = PIECE_VALUES[PAWN] * (__builtin_popcountll(B->WHITE[PAWN]) - __builtin_popcountll(B->BLACK[PAWN]));
  score += PIECE_VALUES[KNIGHT] * (__builtin_popcountll(B->WHITE[KNIGHT]) - __builtin_popcountll(B->BLACK[KNIGHT]));
  score += PIECE_VALUES[BISHOP] * (__builtin_popcountll(B->WHITE[BISHOP]) - __builtin_popcountll(B->BLACK[BISHOP]));
  score += PIECE_VALUES[ROOK] * (__builtin_popcountll(B->WHITE[ROOK]) - __builtin_popcountll(B->BLACK[ROOK]));
  score += PIECE_VALUES[QUEEN] * (__builtin_popcountll(B->WHITE[QUEEN]) - __builtin_popcountll(B->BLACK[QUEEN]));
  score += PIECE_VALUES[KING] * (__builtin_popcountll(B->WHITE[KING]) - __builtin_popcountll(B->BLACK[KING]));
  return score;
}

int center_control(board *B) {
  const uint64_t CENTER = (FILE_D | FILE_E) & (RANK_4 | RANK_5);
  uint64_t wcont = B->whites & CENTER;
  uint64_t bcont = B->blacks & CENTER;
  return 2 * (__builtin_popcountll(wcont) - __builtin_popcountll(bcont));
}

int king_safe(board *B) {
  uint64_t wking = wk_moves(B->WHITE[KING], B->whites);
  uint64_t bking = bk_moves(B->BLACK[KING], B->blacks);
  int wsafe = __builtin_popcountll(wking & bmoves);
  int bsafe = __builtin_popcountll(bking & wmoves);
  return (bsafe - wsafe);
}

int mobility(board *B) {
  int wmobile = __builtin_popcountll(wmoves);
  int bmobile = __builtin_popcountll(bmoves);
  return (wmobile - bmobile) / 2;
}

int mid_eval(board *B) {
  return mat_eval(B) + center_control(B) + king_safe(B) + mobility(B);
}

int phase(board *B) {
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

int scale(board *B, int eg_score) {
  int no_wpawns = (__builtin_popcountll(B->WHITE[PAWN]) == 0);
  int no_bpawns = (__builtin_popcountll(B->BLACK[PAWN]) == 0);
  if (no_wpawns || no_bpawns) return HALF_SCALE;
  int wking_mobile = B->WHITE[KING] & ~RANK_1 & ~RANK_2;
  int bking_mobile = B->BLACK[KING] & ~RANK_8 & ~RANK_7;
  if (wking_mobile && bking_mobile) return MAX_SCALE;
  return MOD_SCALE;
}

int end_mat_eval(board *B) {
  int score = END_VALUES[PAWN] * (__builtin_popcountll(B->WHITE[PAWN]) - __builtin_popcountll(B->BLACK[PAWN]));
  score += END_VALUES[KNIGHT] * (__builtin_popcountll(B->WHITE[KNIGHT]) - __builtin_popcountll(B->BLACK[KNIGHT]));
  score += END_VALUES[BISHOP] * (__builtin_popcountll(B->WHITE[BISHOP]) - __builtin_popcountll(B->BLACK[BISHOP]));
  score += END_VALUES[ROOK] * (__builtin_popcountll(B->WHITE[ROOK]) - __builtin_popcountll(B->BLACK[ROOK]));
  score += END_VALUES[QUEEN] * (__builtin_popcountll(B->WHITE[QUEEN]) - __builtin_popcountll(B->BLACK[QUEEN]));
  score += END_VALUES[KING] * (__builtin_popcountll(B->WHITE[KING]) - __builtin_popcountll(B->BLACK[KING]));
  return score;
}

int king_activity(board *B) {
  uint64_t mid_ranks = RANK_3 | RANK_4 | RANK_5 | RANK_6;
  int wking = (B->WHITE[KING] & mid_ranks) != 0;
  int bking = (B->BLACK[KING] & mid_ranks) != 0;
  int w = wking ? (ACTIVE_KING) : 0;
  int b = bking ? (ACTIVE_KING) : 0;
  return (w - b);
}

int pawn_structure(board *B) {
  const uint64_t WPROMOTE = RANK_7;
  const uint64_t BPROMOTE = RANK_2;
  uint64_t wp = B->WHITE[PAWN] & WPROMOTE;
  uint64_t bp = B->BLACK[PAWN] & BPROMOTE;
  return ((__builtin_popcountll(wp) - __builtin_popcountll(bp)) * (PAWN_PROMOTE));
}

int passed_pawns(board *B) {
  uint64_t wp = B->WHITE[PAWN];
  uint64_t bp = B->BLACK[PAWN];
  uint64_t wpass = sided_passed_pawns(wp, bp, 1);
  uint64_t bpass = sided_passed_pawns(bp, wp, 0);
  return (PAWN_PASSED * (__builtin_popcountll(wpass) - __builtin_popcountll(bpass)));
}

int development(board *B) {
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

int end_eval(board *B) {
  return end_mat_eval(B) + king_activity(B) + pawn_structure(B) + passed_pawns(B);
}

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
      mg_table[wpc][sq] = mg_value[pt] + (pt == PAWN ? mg_pawn_table[sq] : pt == KNIGHT?mg_knight_table[sq] : pt == BISHOP ? mg_bishop_table[sq] : pt == ROOK ? mg_rook_table[sq] : pt == QUEEN ? mg_queen_table[sq] : mg_king_table[sq]);
      eg_table[wpc][sq] = eg_value[pt] + (pt==PAWN ? eg_pawn_table[sq] : pt==KNIGHT ? eg_knight_table[sq] : pt==BISHOP ? eg_bishop_table[sq] : pt==ROOK ? eg_rook_table[sq] : pt==QUEEN ? eg_queen_table[sq] : eg_king_table[sq]);

      // flip for black
      int fsq = FLIP(sq);
      mg_table[bpc][sq] = mg_value[pt] + (pt==PAWN ? mg_pawn_table[fsq] : pt==KNIGHT ? mg_knight_table[fsq] : pt==BISHOP ? mg_bishop_table[fsq] : pt==ROOK ? mg_rook_table[fsq] : pt==QUEEN ? mg_queen_table[fsq] : mg_king_table[fsq]);
      eg_table[bpc][sq] = eg_value[pt] + (pt==PAWN ? eg_pawn_table[fsq] : pt==KNIGHT ? eg_knight_table[fsq] : pt==BISHOP ? eg_bishop_table[fsq] : pt==ROOK ? eg_rook_table[fsq] : pt==QUEEN ? eg_queen_table[fsq] : eg_king_table[fsq]);
    }
  }
}

void pesto_terms(board *B, int *mg, int *eg, int *p24) {
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

int blended_eval(board *B) {
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

  // feature sums
  int mg_feats = (MOBILITY_MG * mob) + (CENTER_MG * ctr) + (KING_SAFETY_MG * ks) + (CASTLE_PT * castle) + (DEV_MG * dev);;
  int eg_feats = (MOBILITY_EG * mob) + (CENTER_EG * ctr) + (KING_ACTIVITY_EG * ka) + (PSTRUCT_EG * pstr) + (PASSED_EG * pps);

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