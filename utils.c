#include "lib/utils.h"
#include "lib/board.h"
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

int pi(char file, char rank) {
  if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return -1;
  return (rank - '1') * 8 + (file - 'a');
}

void ip(int index, char *file, char *rank) {
  *file = 'a' + (index % 8);
  *rank = '1' + (index / 8);
}

int index_rank(int rank, int file) {
  return (rank * 8) + file;
}

void print_move_eval(const char *label, int packed, int eval) {
  int from = packed / 64;
  int to = packed % 64;
  char ff, fr, tf, tr;
  ip(from, &ff, &fr);
  ip(to, &tf, &tr);
  printf("%s: %c%c -> %c%c (from=%d, to=%d), eval=%d\n", label, ff, fr, tf, tr, from, to, eval);
}

int uci_to_from_to(const char *uci, int *from, int *to) {
  if (!uci)
    return 0;
  size_t len = strlen(uci);
  if (len < 4)
    return 0;

  int ff = uci[0] - 'a';
  int fr = uci[1] - '1';
  int tf = uci[2] - 'a';
  int tr = uci[3] - '1';

  if (ff < 0 || ff > 7 || tf < 0 || tf > 7 || fr < 0 || fr > 7 || tr < 0 || tr > 7) {
    return 0;
  }

  *from = fr * 8 + ff;
  *to = tr * 8 + tf;
  return 1;
}

void san_from_move(int pt, int from, int to, int capture, char *out, size_t out_size) { // make a san string from move
  int file_from = from % 8;
  int rank_from = from / 8;
  int file_to   = to   % 8;
  int rank_to   = to   / 8;

  int idx = 0;

 if (pt == KING && (file_to - file_from == 2 || file_from - file_to == 2)) { // castling
    if (file_to > file_from) {
      // O-O -> "OO"
      out[idx++] = 'O';
      out[idx++] = 'O';
    } else {
      // O-O-O -> "OOO"
      out[idx++] = 'O';
      out[idx++] = 'O';
      out[idx++] = 'O';
    }
    out[idx] = '\0';
    return;
  }

  if (pt == PAWN) {
    if (capture) {
      out[idx++] = 'a' + file_from; // dxe5 style
      out[idx++] = 'x';
    }
    out[idx++] = 'a' + file_to;
    out[idx++] = '1' + rank_to;
  } else {
    char pchar = '?';
    switch (pt) {
      case KNIGHT: pchar = 'N'; break;
      case BISHOP: pchar = 'B'; break;
      case ROOK: pchar = 'R'; break;
      case QUEEN: pchar = 'Q'; break;
      case KING: pchar = 'K'; break;
      default: pchar = '?'; break;
    }
    out[idx++] = pchar;
    if (capture) {
      out[idx++] = 'x';
    }
    out[idx++] = 'a' + file_to;
    out[idx++] = '1' + rank_to;
  }
  out[idx] = '\0';
}