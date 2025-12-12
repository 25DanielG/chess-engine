#include "lib/opening.h"
#include "lib/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>

open_node *root = NULL;

open_node *create_node(const char *move) {
  open_node *node = (open_node *)malloc(sizeof(open_node));
  if (!node) {
    perror("create_node malloc");
    exit(1);
  }
  strncpy(node->move, move, 9);
  node->move[9] = '\0';
  node->move_code = -1; // initialized in add_sequence
  node->opening_name = NULL;
  node->white_wins = 0;
  node->black_wins = 0;
  node->draws = 0;
  node->child = NULL;
  node->sibling = NULL;
  return node;
}

void add_sequence(char **moves, int *codes, int move_count, const char *op_name, int w_wins, int b_wins) {
  if (root == NULL)
    root = create_node("root");

  open_node *current = root;
  for (int i = 0; i < move_count; ++i) {
    const char *clean_move = moves[i]; // cleaned SAN

    open_node *child = current->child;
    open_node *prev  = NULL;
    open_node *found = NULL;

    while (child != NULL) {
      if (strcmp(child->move, clean_move) == 0) {
        found = child;
        break;
      }
      prev = child;
      child = child->sibling;
    }

    if (!found) {
      found = create_node(clean_move);
      found->move_code = codes[i]; // from * 64 + to
      if (prev == NULL) {
        current->child = found;
      } else {
        prev->sibling = found;
      }
    } else {
      if (found->move_code == -1) {
        found->move_code = codes[i];
      }
    }
    current = found;
  }

  if (current->opening_name == NULL && op_name != NULL) {
    current->opening_name = strdup(op_name);
  }
  current->white_wins += w_wins;
  current->black_wins += b_wins;
}

void free_nodes(open_node *node) {
  if (!node)
    return;
  free_nodes(node->child);
  free_nodes(node->sibling);
  if (node->opening_name)
    free(node->opening_name);
  free(node);
}

void init_opening_book() {
  srand(time(NULL));
  root = create_node("root");
}

void free_opening_book() {
  free_nodes(root);
  root = NULL;
}

int parse_csv_line(FILE *stream, char *buffer, int size) { // 1 on success, 0 on EOF
  int ch, i = 0;
  int in_quote = 0;

  while ((ch = fgetc(stream)) != EOF && ch != '\n') {
    if (i < size - 1) {
      if (ch == '"')
        in_quote = !in_quote;
      buffer[i++] = ch;
    }
  }
  buffer[i] = '\0';
  if (ch == EOF && i == 0)
    return 0;
  return 1;
}

int parse_move_string(char *list_str, char **move_array, int max_moves) { // clean list string
  int count = 0;
  char *ptr = list_str;

  while (*ptr && (*ptr == '[' || *ptr == '"' || *ptr == '\''))
    ++ptr;

  char *token = strtok(ptr, ",");
  while (token && count < max_moves) {
    char clean[20];
    int c_idx = 0;
    for (int i = 0; token[i]; ++i) {
      if (isalnum(token[i]) || token[i] == '.') {
        clean[c_idx++] = token[i];
      }
    }
    clean[c_idx] = '\0';

    if (c_idx > 0) {
      move_array[count++] = strdup(clean);
    }

    token = strtok(NULL, ",");
  }
  return count;
}

int load_openings(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("failed to open opening book file");
    return 0;
  }

  char line[4096];
  parse_csv_line(file, line, sizeof(line)); // header
  int loaded_count = 0;

  while (parse_csv_line(file, line, sizeof(line))) {
    char *cols[30];
    int col_idx = 0;
    char *cursor = line;

    while (*cursor && col_idx < 30) {
      if (*cursor == '"') {
        ++cursor;
        cols[col_idx++] = cursor;
        while (*cursor && *cursor != '"')
          ++cursor;
        *cursor = '\0';
        ++cursor;
        if (*cursor == ',')
          ++cursor;
      } else {
        cols[col_idx++] = cursor;
        while (*cursor && *cursor != ',')
          ++cursor;
        if (*cursor == ',') {
          *cursor = '\0';
          ++cursor;
        }
      }
    }

    if (col_idx < 11)
      continue; // bad line

    char *name = cols[0];
    char *moves_str = cols[10];
    char *w_wins_str = cols[22];
    char *b_wins_str = cols[23];

    char *raw_moves[100];
    int move_count = parse_move_string(moves_str, raw_moves, 100);
    if (move_count == 0) {
      continue;
    }

    char *san_moves[100]; // clean SAN moves
    for (int i = 0; i < move_count; ++i) {
      char *p = raw_moves[i];
      char *dot = strchr(p, '.');
      if (dot) p = dot + 1;  // skip "1."
      san_moves[i] = strdup(p);
    }

    int codes[100];
    int valid = 1;

    board *B = init_board();
    int side = 1; // 1 = white, 0 = black

    for (int i = 0; i < move_count && valid; ++i) {
      move_t *moves;
      int mcount = movegen(B, side, &moves, 1); // legal moves
      int found = 0;

      for (int j = 0; j < mcount; ++j) {
        int pt = moves[j].piece;
        int mf = moves[j].from;
        int mt = moves[j].to;
        char tmp[16];
        uint64_t to_mask = 1ULL << mt;
        int capture = side ? ((B->blacks & to_mask) != 0) : ((B->whites & to_mask) != 0);
        san_from_move(pt, mf, mt, capture, tmp, sizeof(tmp));
        if (strcmp(tmp, san_moves[i]) == 0) {
          codes[i] = mf * 64 + mt;
          fast_execute(B, pt, mf, mt, side, 0); // apply move to test board, no promotion in opening
          side = !side;
          found = 1;
          break;
        }
      }

      free(moves);

      if (!found) {
#ifdef DEBUG
        fprintf(stderr, "Failed to resolve SAN '%s' for opening '%s'; skipping line.\n", san_moves[i], name);
#endif
        valid = 0;
      }
    }

    free_board(B);

    if (valid) {
      int w_wins = atoi(w_wins_str);
      int b_wins = atoi(b_wins_str);
      add_sequence(san_moves, codes, move_count, name, w_wins, b_wins);
      ++loaded_count;
    }

    for (int i = 0; i < move_count; ++i) {
      free(raw_moves[i]);
      free(san_moves[i]);
    }
  }

  fclose(file);
#ifdef DEBUG
  printf("Loaded %d openings\n", loaded_count);
#endif
  return 1;
}

open_node *get_book_move(char **move_history, int history_count) {
  if (!root)
    return NULL;

  open_node *curr = root;
  for (int i = 0; i < history_count; ++i) { // walk down tree
    open_node *child = curr->child;
    int found = 0;
    while (child) {
      if (strcmp(child->move, move_history[i]) == 0) {
        curr = child;
        found = 1;
        break;
      }
      child = child->sibling;
    }
    if (!found) return NULL; // off the book
  }

  open_node *candidates[50]; // random child
  int count = 0;
  open_node *child = curr->child;
  while (child && count < 50) {
    candidates[count++] = child;
    child = child->sibling;
  }

  if (count == 0) return NULL; // end of line

  int pick = rand() % count;
  return candidates[pick];
}

int get_line_info(char **moves, int move_count, const char **out_name) {
  if (!root) {
    if (out_name)
      *out_name = NULL;
    return 0;
  }

  open_node *curr = root;
  const char *last_name = NULL;
  int matched = 0;

  for (int i = 0; i < move_count; ++i) {
    open_node *child = curr->child;
    int found = 0;

    while (child) {
      if (strcmp(child->move, moves[i]) == 0) {
        curr = child;
        ++matched;
        if (curr->opening_name) {
          last_name = curr->opening_name;
        }
        found = 1;
        break;
      }
      child = child->sibling;
    }

    if (!found) {
      break;
    }
  }

  if (out_name) {
    *out_name = last_name;
  }
  return matched;
}