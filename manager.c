#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lib/board.h"
#include "lib/manager.h"
#include "lib/bot.h"
#include "lib/eval.h"
#include "lib/magic.h"
#include "lib/ui_sdl.h"
#include "lib/utils.h"
#include "lib/opening.h"

int history_len = 0;

void start(void) {
  if (GRAPHICS) {
    run_sdl();
  } else {
    run();
  }
}

int run(void) {
  init_attack_tables();
  init_pesto_tables();
  init_opening_book();
  if (!load_openings("high_elo_opening.csv")) {
    fprintf(stderr, "WARNING: failed to load opening book, continuing without it.\n");
  }
#ifdef DEBUG
  printf("Initialized attack tables.\n");
#endif
  board *B = init_board();
  // test_position(B); // for testing hardcoded positions
  char input[MAX_INPUT_SIZE];
  int from, to, ply = 0, move = 0, mode = 0;
  bot *bwhite = NULL, *bblack = NULL;
  if (WHITE_BOT)
    bwhite = init_bot(B, 1, WHITE_DEPTH, WHITE_LIMIT);
  if (BLACK_BOT)
    bblack = init_bot(B, 0, BLACK_DEPTH, BLACK_LIMIT);

  printf("Notation (e2 e4).\n");

  while (true) {
    out(B, mode);
    printf("White: %d, Move %d\n", B->white, move);
#ifdef DEBUG
    if (check(B, 1)) {
      printf("Black in check.\n");
    }
    if (check(B, 0)) {
      printf("White in check.\n");
    }
#endif
    board_snapshot snap;
    save_snapshot(B, &snap);
    if (B->white) {
      if (WHITE_BOT) {
        int best = -1;
        int from = -1, to = -1;
        int type = -1;

        open_node *book_node = get_book_move(move_history, history_len);
        if (book_node && book_node->move_code >= 0) {
          best = book_node->move_code;
          from = best / 64;
          to = best % 64;
          type = 0; // opening book
        }

        if (best == -1) {
          best = find_move(bwhite, B->white, bwhite->limit);
          if (best == -1) {
            printf("WHITE_BOT has no legal moves. Game over.\n");
            break;
          }
          from = best / 64;
          to = best % 64;
          type = 1; // bot move
        }

        uint64_t to_mask = 1ULL << to;
        int capture = (B->blacks & to_mask) != 0;
        int pt = piece_at(B, from);
        printf("Played %c%c to %c%c\n", 'a' + (from % 8), '1' + (from / 8), 'a' + (to % 8), '1' + (to / 8));
#ifdef DEBUG
        char *move_type = (type == 0) ? "opening book" : "bot calculation";
        printf("Move type: %s\n", move_type);
#endif
        if (execute(B, from, to, &B->white)) {
          add_history(pt, from, to, capture); // track history
          ++ply;
          move = ply / 2;
          B->white = !B->white;
#ifdef DEBUG
          const char *line_name = NULL;
          int matched = get_line_info(move_history, history_len, &line_name);
          if (matched == history_len && line_name) {
            printf("Current line: %s\n", line_name);
          } else if (matched > 0 && line_name) {
            printf("Current line (up to move %d): %s (now off book)\n", matched, line_name);
          } else if (matched == 0) {
            printf("No book line for this game so far.\n");
          }
#endif
        } else {
          printf("WHITE_BOT invalid move.\n");
          exit(1);
        }
      } else {
        printf("Enter move (Quit q): ");
        fgets(input, MAX_INPUT_SIZE, stdin);
        if (input[0] == 'q') break;
        if (input[0] == 's') mode = !mode;
        if (!parse(input, &from, &to)) {
          printf("Invalid input. Try again.\n");
          continue;
        }
        printf("From: %d, To: %d\n", from, to);

        uint64_t to_mask = 1ULL << to;
        int capture = (B->blacks & to_mask) != 0;
        int pt = piece_at(B, from);

        if (execute(B, from, to, &B->white)) {
          add_history(pt, from, to, capture); // track history
          ++ply;
          move = ply / 2;
          B->white = !B->white;
        } else {
          printf("Invalid move. Try again.\n");
          restore_snapshot(B, &snap); // invalid move, restore board
        }
      }
    } else {
      if (BLACK_BOT) {
        int best = -1;
        int from = -1, to = -1;
        int type = -1;

        open_node *book_node = get_book_move(move_history, history_len);
        if (book_node && book_node->move_code >= 0) {
          best = book_node->move_code;
          from = best / 64;
          to = best % 64;
          type = 0; // opening book
        }

        if (best == -1) {
          int side_to_move = B->white; 
          best = find_move(bblack, side_to_move, bblack->limit);
          if (best == -1) {
            printf("BLACK_BOT has no legal moves. Game over.\n");
            break;
          }
          from = best / 64;
          to = best % 64;
          type = 1; // bot move
        }

        uint64_t to_mask = 1ULL << to;
        int capture = (B->whites & to_mask) != 0;
        int pt = piece_at(B, from);

        printf("Played %c%c to %c%c\n", 'a' + (from % 8), '1' + (from / 8), 'a' + (to % 8), '1' + (to / 8));
#ifdef DEBUG
        char *move_type = (type == 0) ? "opening book" : "bot calculation";
        printf("Move type: %s\n", move_type);
#endif
        if (execute(B, from, to, &B->white)) {
          add_history(pt, from, to, capture); // track history
          ++ply;
          move = ply / 2;
          B->white = !B->white;
#ifdef DEBUG
          const char *line_name = NULL;
          int matched = get_line_info(move_history, history_len, &line_name);
          if (matched == history_len && line_name) {
            printf("Current line: %s\n", line_name);
          } else if (matched > 0 && line_name) {
            printf("Current line (up to move %d): %s (now off book)\n", matched, line_name);
          } else if (matched == 0) {
            printf("No book line for this game so far.\n");
          }
#endif
        } else {
          printf("BLACK_BOT invalid move.\n");
          exit(1);
        }
      } else {
        printf("Enter move (Quit q): ");
        fgets(input, MAX_INPUT_SIZE, stdin);
        if (input[0] == 'q') break;
        if (input[0] == 's') mode = !mode;
        if (!parse(input, &from, &to)) {
          printf("Invalid input. Try again.\n");
          continue;
        }
        printf("From: %d, To: %d\n", from, to);

        uint64_t to_mask = 1ULL << to;
        int capture = (B->blacks & to_mask) != 0;
        int pt = piece_at(B, from);

        if (execute(B, from, to, &B->white)) {
          add_history(pt, from, to, capture); // track history
          ++ply;
          move = ply / 2;
          B->white = !B->white;
        } else {
          printf("Invalid move. Try again.\n");
          restore_snapshot(B, &snap); // invalid move, restore board
        }
      }
    }
  }

  free_history();
  free_opening_book();
  free_board(B);
  return 0;
}

void out(board *B, int mode) { // 0: letter, 1: piece
  uint64_t mask;
  printf("\n  a b c d e f g h\n");
  int rank;
  for (rank = 7; rank >= 0; --rank) {
    printf("%d ", rank + 1);
    int file;
    for (file = 0; file < 8; ++file) {
      int index = rank * 8 + file;
      mask = 1ULL << index;
      if (B->WHITE[PAWN] & mask) printf(mode ? "♟ " : "P ");
      else if (B->WHITE[KNIGHT] & mask) printf(mode ? "♞ " : "N ");
      else if (B->WHITE[BISHOP] & mask) printf(mode ? "♝ " : "B ");
      else if (B->WHITE[ROOK] & mask) printf(mode ? "♜ " : "R ");
      else if (B->WHITE[QUEEN] & mask) printf(mode ? "♛ " : "Q ");
      else if (B->WHITE[KING] & mask) printf(mode ? "♚ " : "K ");
      else if (B->BLACK[PAWN] & mask) printf(mode ? "♙ " : "p ");
      else if (B->BLACK[KNIGHT] & mask) printf(mode ? "♘ " : "n ");
      else if (B->BLACK[BISHOP] & mask) printf(mode ? "♗ " : "b ");
      else if (B->BLACK[ROOK] & mask) printf(mode ? "♖ " : "r ");
      else if (B->BLACK[QUEEN] & mask) printf(mode ? "♕ " : "q ");
      else if (B->BLACK[KING] & mask) printf(mode ? "♔ " : "k ");
      else printf(". ");
    }
    printf("%d\n", rank + 1);
  }
  printf("  a b c d e f g h\n\n");
}

int parse(const char *input, int *from, int *to) {
  char from_file, from_rank, to_file, to_rank;
  if (sscanf(input, "%c%c %c%c", &from_file, &from_rank, &to_file, &to_rank) != 4) {
    return 0;
  }
  *from = pi(from_file, from_rank);
  *to = pi(to_file, to_rank);
  return (*from >= 0 && *to >= 0);
}

int execute(board *B, int from, int to, int *white) {
  const int side = *white;
  const uint64_t from_mask = 1ULL << from;
  const uint64_t to_mask   = 1ULL << to;
  const uint64_t home  = side ? B->whites : B->blacks;
  const uint64_t enemy = side ? B->blacks : B->whites;
  if ((from_mask & home) == 0) return 0;
  if (from_mask & enemy) return 0;
  if (to_mask & home) return 0;
  int pt = piece_at(B, from);
  if (pt < 0) return 0;
  if ((imove(pt, from_mask, B, white) & to_mask) == 0) return 0;

  board_snapshot S;
  save_snapshot(B, &S);

  // clear opponent castle rights if take rook
  if (side) { // white move
    if (B->BLACK[ROOK] & to_mask) {
      if (to == H8) B->castle &= ~BKS;
      else if (to == A8) B->castle &= ~BQS;
    }
  } else { // black move
    if (B->WHITE[ROOK] & to_mask) {
      if (to == H1) B->castle &= ~WKS;
      else if (to == A1) B->castle &= ~WQS;
    }
  }

  int castle = (pt == KING) && (from/8 == to/8) && ( (to - from == 2) || (from - to == 2) );

  if (castle) {
    // move king
    if (side) { // white castle
      B->WHITE[KING] ^= from_mask; B->WHITE[KING] |= to_mask;
      if (to == G1) { // white king side h1 to f1
        B->WHITE[ROOK] ^= (1ULL << H1);
        B->WHITE[ROOK] |= (1ULL << F1);
      } else { // white queen side a1 to d1
        B->WHITE[ROOK] ^= (1ULL << A1);
        B->WHITE[ROOK] |= (1ULL << D1);
      }
      B->castle &= ~(WKS | WQS);
    } else { // black castle
      B->BLACK[KING] ^= from_mask; B->BLACK[KING] |= to_mask;
      if (to == G8) { // black king side h8 to f8
        B->BLACK[ROOK] ^= (1ULL << H8);
        B->BLACK[ROOK] |= (1ULL << F8);
      } else { // black queen side a8 to d8
        B->BLACK[ROOK] ^= (1ULL << A8);
        B->BLACK[ROOK] |= (1ULL << D8);
      }
      B->castle &= ~(BKS | BQS);
    }
  } else {
    if (side) {
      B->WHITE[pt] ^= from_mask;
      B->WHITE[pt] |= to_mask;
      for (int i = 0; i < NUM_PIECES; ++i) B->BLACK[i] &= ~to_mask;
      // update castling rights king rook move
      if (pt == KING) B->castle &= ~(WKS | WQS);
      else if (pt == ROOK) {
        if (from == H1) B->castle &= ~WKS;
        else if (from == A1) B->castle &= ~WQS;
      }
    } else {
      B->BLACK[pt] ^= from_mask;
      B->BLACK[pt] |= to_mask;
      for (int i = 0; i < NUM_PIECES; ++i) B->WHITE[i] &= ~to_mask;

      if (pt == KING) B->castle &= ~(BKS | BQS);
      else if (pt == ROOK) {
        if (from == H8) B->castle &= ~BKS;
        else if (from == A8) B->castle &= ~BQS;
      }
    }
  }

  B->whites = whites(B);
  B->blacks = blacks(B);

  if (check(B, !side)) { restore_snapshot(B, &S); return 0; }

  return 1;
}

uint64_t imove(int piece, uint64_t from_mask, board *B, int *white) {
  uint64_t moves;
  switch (piece)
  {
  case PAWN: return (*white ? wp_moves(from_mask, B->whites, B->blacks) : bp_moves(from_mask, B->whites, B->blacks));
  case KNIGHT: return (*white ? wn_moves(from_mask, B->whites, B->jumps) : bn_moves(from_mask, B->blacks, B->jumps));
  case BISHOP: return (*white ? wb_moves(from_mask, B->whites, B->blacks) : bb_moves(from_mask, B->whites, B->blacks));
  case ROOK: return (*white ? wr_moves(from_mask, B->whites, B->blacks) : br_moves(from_mask, B->whites, B->blacks));
  case QUEEN: return (*white ? wq_moves(from_mask, B->whites, B->blacks) : bq_moves(from_mask, B->whites, B->blacks));
  case KING: {
    uint64_t moves = *white ? wk_moves(from_mask, B->whites) : bk_moves(from_mask, B->blacks);
    const uint64_t occ = B->whites | B->blacks;
    if (*white) { // e1 to castle
      if (from_mask == (1ULL << E1)) { // e1 to g1 king side
        if ((B->castle & WKS) && !(occ & ((1ULL << F1) | (1ULL << G1))) &&
            !check(B, 0) && // not in check
            !square_attacker(B, F1, 1) && // attacked by black
            !square_attacker(B, G1, 1)) { // attacked by black
          moves |= (1ULL << G1);
        }
        if ((B->castle & WQS) && // e1 to c1 queen side
            !(occ & ((1ULL << D1) | (1ULL << C1) | (1ULL << B1))) && !check(B, 0) &&
            !square_attacker(B, D1, 1) &&
            !square_attacker(B, C1, 1)) {
          moves |= (1ULL << C1);
        }
      }
    } else { // e8 to castle
      if (from_mask == (1ULL << E8)) { // e8 to g8 king side
        if ((B->castle & BKS) && !(occ & ((1ULL << F8) | (1ULL << G8))) &&
            !check(B, 1) &&
            !square_attacker(B, F8, 0) && // attacked by white
            !square_attacker(B, G8, 0)) { // attacked by black
          moves |= (1ULL << G8);
        }
        if ((B->castle & BQS) && // e8 to c8 queen side
            !(occ & ((1ULL << D8) | (1ULL << C8) | (1ULL << B8))) && !check(B, 1) &&
            !square_attacker(B, D8, 0) &&
            !square_attacker(B, C8, 0)) {
          moves |= (1ULL << C8);
        }
      }
    }
    return moves;
  }
  default:
#ifdef DEBUG
    printf("Received unknown piece index (imove): %d, white: %d\n", piece, *white);
    printf("Attempting to move from %c%c\n", 'a' + (__builtin_ctzll(from_mask) % 8), '1' + (__builtin_ctzll(from_mask) / 8));
#endif
    exit(1);
  }
}

void test_position(board *B) {
  uint64_t white[NUM_PIECES] = {
    0x0000000006000000ULL, // p
    0x0000080800000000ULL, // k
    0x0000000000000000ULL, // b
    0x0000000000000010ULL, // r
    0x0000000000000000ULL, // q
    0x0000000000000100ULL  // king
  };
  uint64_t black[NUM_PIECES] = {
    0x00E2020000000000ULL, // p
    0x0000000008000000ULL, // k
    0x0000000000000000ULL, // b
    0x8000000000002000ULL, // r
    0x0000000000000000ULL, // q
    0x2000000000000000ULL  // king
  };
  int w = 1;
  load_position(B, white, black, w);
}

void add_history(int pt, int from, int to, int capture) {
  if (history_len >= MAX_GAME_MOVES) return;
  char buf[10];
  san_from_move(pt, from, to, capture, buf, sizeof(buf));
  move_history[history_len] = strdup(buf);
  ++history_len;
}

void free_history(void) {
  for (int i = 0; i < history_len; ++i) {
    free(move_history[i]);
  }
  history_len = 0;
}