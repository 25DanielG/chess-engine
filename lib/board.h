#pragma once

#include <stdint.h>
#include <stdlib.h>

#define NUM_PIECES (6)
#define BOARD_SIZE (64)

#define FILE_A (0x0101010101010101ULL)
#define FILE_B (0x0202020202020202ULL)
#define FILE_C (0x0404040404040404ULL)
#define FILE_D (0x0808080808080808ULL)
#define FILE_E (0x1010101010101010ULL)
#define FILE_F (0x2020202020202020ULL)
#define FILE_G (0x4040404040404040ULL)
#define FILE_H (0x8080808080808080ULL)
#define RANK_1 (0x00000000000000FFULL)
#define RANK_2 (0x000000000000FF00ULL)
#define RANK_3 (0x0000000000FF0000ULL)
#define RANK_4 (0x00000000FF000000ULL)
#define RANK_5 (0x000000FF00000000ULL)
#define RANK_6 (0x0000FF0000000000ULL)
#define RANK_7 (0x00FF000000000000ULL)
#define RANK_8 (0xFF00000000000000ULL)

#define PAWN_VALUE (1)
#define KNIGHT_VALUE (3)
#define BISHOP_VALUE (3)
#define ROOK_VALUE (5)
#define QUEEN_VALUE (9)
#define KING_VALUE (50)
#define MAX_MOVES (256)

// castling
#define WKS 0x1  // white king side
#define WQS 0x2  // white queen side
#define BKS 0x4  // black king side
#define BQS 0x8  // black queen side

enum pieces {
  PAWN, KNIGHT, BISHOP,
  ROOK, QUEEN, KING
};

enum {
  A1=0,  B1,  C1,  D1,  E1,  F1,  G1,  H1,
  A2=8,  B2,  C2,  D2,  E2,  F2,  G2,  H2,
  A3=16, B3,  C3,  D3,  E3,  F3,  G3,  H3,
  A4=24, B4,  C4,  D4,  E4,  F4,  G4,  H4,
  A5=32, B5,  C5,  D5,  E5,  F5,  G5,  H5,
  A6=40, B6,  C6,  D6,  E6,  F6,  G6,  H6,
  A7=48, B7,  C7,  D7,  E7,  F7,  G7,  H7,
  A8=56, B8,  C8,  D8,  E8,  F8,  G8,  H8
};

struct board_header {
  uint64_t *WHITE;
  uint64_t *BLACK;
  uint64_t *jumps;
  int white;
  uint64_t whites;
  uint64_t blacks;
  uint8_t castle;
  uint8_t cc; // castle completed
};

struct snapshot_header {
  uint64_t WHITE[NUM_PIECES];
  u_int64_t BLACK[NUM_PIECES];
  u_int64_t whites;
  u_int64_t blacks;
  int white;
  uint8_t castle;
  uint8_t cc; // castle completed
};

typedef struct {
  int piece;
  int from; // 0 to 63
  int to; // 0 to 63
  int order; // cached ordering score
} move_t;

typedef struct {
  int moved_piece;
  int from;
  int to;
  int captured_piece; // -1 if no capture
  int captured_square; // usually == to
  uint8_t prev_castle;
  uint8_t prev_cc;
} undo_t;

typedef struct board_header board;
typedef struct snapshot_header board_snapshot;

static inline int encode_move(move_t m) { return m.from * 64 + m.to; }

board *init_board(void);
board *preset_board(uint64_t wpawns, uint64_t bpawns, uint64_t wknights, uint64_t bknights, uint64_t wbishops, uint64_t bbishops, uint64_t wrooks, uint64_t brooks, uint64_t wqueens, uint64_t bqueens, uint64_t wkings, uint64_t bkings, uint8_t castling, uint8_t complete);
uint64_t white_moves(const board *B);
uint64_t black_moves(const board *B);
uint64_t wp_moves(uint64_t p, uint64_t w, uint64_t b);
uint64_t bp_moves(uint64_t p, uint64_t w, uint64_t b);
uint64_t wn_moves(uint64_t p, uint64_t w, uint64_t *jumps);
uint64_t bn_moves(uint64_t p, uint64_t b, uint64_t *jumps);
uint64_t wb_moves(uint64_t p, uint64_t w, uint64_t b);
uint64_t bb_moves(uint64_t p, uint64_t w, uint64_t b);
uint64_t wr_moves(uint64_t p, uint64_t w, uint64_t b);
uint64_t br_moves(uint64_t p, uint64_t w, uint64_t b);
uint64_t wq_moves(uint64_t p, uint64_t w, uint64_t b);
uint64_t bq_moves(uint64_t p, uint64_t w, uint64_t b);
uint64_t wk_moves(uint64_t p, uint64_t w);
uint64_t bk_moves(uint64_t p, uint64_t b);
uint64_t whites(const board *B);
uint64_t blacks(const board *B);
void print_board(const board *B);
void print_snapshot(const char *label, const board_snapshot *S);
void binary_print(uint64_t N);
void free_board(board *B);
void init_jump_table(uint64_t *jumps);
uint64_t slide(uint64_t blocks, int square);
uint64_t translate(uint64_t blockers, int square);
uint64_t queen(uint64_t blockers, int square);
uint64_t circle(int square);
int piece_at(const board *B, int square);
int movegen(board *B, int white, move_t **move_list, int check_legal);
int movegen_ply(board *B, int white, int check_legal, int ply, move_t **out, move_t (*move_stack)[MAX_MOVES], int max_moves);
board *clone(board *B);
uint64_t sided_passed_pawns(uint64_t friend, uint64_t opp, int white);
int value(int piece);
void fast_execute(board *B, int piece, int from, int to, int white);
void assert_board(const board *B);
void make_move(board *B, const move_t *m, int white, undo_t *u);
void unmake_move(board *B, const move_t *m, int white, const undo_t *u);
uint64_t hash_board(const board *B);
uint64_t hash_snapshot(const board_snapshot* S);
void save_snapshot(const board *B, board_snapshot *S);
void restore_snapshot(board *B, const board_snapshot *S);
int check(const board *B, int side);
void load_position(board *B, const uint64_t *WHITE, const uint64_t *BLACK, int white);
static inline uint64_t white_attacks(const board *B);
static inline uint64_t black_attacks(const board *B);
static inline void add_castles(board *B, int white, move_t **list, int *count, int *max_moves);
static inline void add_castles_nalloc(board *B, int white, move_t *list, int *count, int max_moves);
int square_attacker(const board *B_in, int square, int by_black);