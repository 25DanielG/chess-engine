#pragma once

#include "board.h"
#include "utils.h"
#include <stdio.h>

typedef struct open_node {
  char move[10]; // "e4", "Nf6" etc.
  int move_code;
  char *opening_name;
  int white_wins;
  int black_wins;
  int draws;
  struct open_node *child;
  struct open_node *sibling;
} open_node;

extern open_node *root;

open_node *create_node(const char *move);
void add_sequence(char **moves, int *codes, int move_count, const char *op_name, int w_wins, int b_wins);
void free_nodes(open_node *node);

void init_opening_book();
void free_opening_book();
int parse_csv_line(FILE *stream, char *buffer, int size);
int parse_move_string(char *list_str, char **move_array, int max_moves);
int load_openings(const char *filename);
open_node *get_book_move(char **move_history, int history_count); // history of moves "e4", "e5", etc: return null or random book move
int get_line_info(char **moves, int move_count, const char **out_name);
void print_tree_stats();