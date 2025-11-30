#pragma once

#include <stddef.h>
#include <string.h>

int pi(char file, char rank);
void ip(int index, char *file, char *rank);
int index_rank(int rank, int file);
void print_move_eval(const char *label, int packed, int eval);
int uci_to_from_to(const char *uci, int *from, int *to);
void san_from_move(int pt, int from, int to, int capture, char *out, size_t out_size);