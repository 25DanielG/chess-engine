#include <SDL.h>
#include <SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include "lib/board.h"
#include "lib/magic.h"
#include "lib/manager.h"
#include "lib/eval.h"
#include "lib/ui_sdl.h"
#include "lib/utils.h"
#include "lib/opening.h"

const char* TEX_PATHS[TEX_COUNT] = {
  "assets/wP.gif", "assets/wN.gif", "assets/wB.gif",
  "assets/wR.gif", "assets/wQ.gif", "assets/wK.gif",
  "assets/bP.gif", "assets/bN.gif", "assets/bB.gif", 
  "assets/bR.gif", "assets/bQ.gif", "assets/bK.gif"
};

static inline int rank_pixel(int x, int y, int* rank, int* file) { // y = 0 -> rank 7
  x -= MARGIN;
  y -= MARGIN;
  if (x < 0 || y < 0 || x >= BOARD_PIX || y >= BOARD_PIX)
    return 0;
  
  *file = x / TILE;
  *rank = 7 - (y / TILE);

  return 1;
}

static int piece_tex(const board* B, int index, TexId* out) {
  uint64_t mask = 1ULL << index;

  if (B->WHITE[PAWN] & mask)
    *out = wP;
  else if (B->WHITE[KNIGHT] & mask)
    *out = wN;
  else if (B->WHITE[BISHOP] & mask)
    *out = wB;
  else if (B->WHITE[ROOK] & mask)
    *out = wR;
  else if (B->WHITE[QUEEN] & mask)
    *out = wQ;
  else if (B->WHITE[KING] & mask)
    *out = wK;
  else if (B->BLACK[PAWN] & mask)
    *out=bP;
  else if (B->BLACK[KNIGHT] & mask)
    *out = bN;
  else if (B->BLACK[BISHOP] & mask)
    *out = bB;
  else if (B->BLACK[ROOK] & mask)
    *out = bR;
  else if (B->BLACK[QUEEN] & mask)
    *out = bQ;
  else if (B->BLACK[KING] & mask)
    *out = bK;
  else
    return 0;
  
  return 1;
}

static void draw_board(SDL_Renderer* R) {
  for (int r = 0; r < 8; ++r) {
    for (int f = 0; f < 8; ++f) {
      int x = MARGIN + f * TILE;
      int y = MARGIN + (7 - r) * TILE;
      int dark = !((r + f) & 1);
      int shade = dark ? 118 : 238; // light = 238, dark = 118
      SDL_SetRenderDrawColor(R, shade, shade, shade, 255);
      SDL_Rect sq = { x, y, TILE, TILE };
      SDL_RenderFillRect(R, &sq);
    }
  }
}

static void draw_highlight(SDL_Renderer* R, int index, SDL_Color c) {
  if (index < 0)
    return;
  
  int r = index / 8, f = index % 8;
  int x = MARGIN + f * TILE;
  int y = MARGIN + (7 - r) * TILE;
  SDL_Rect rect = { x, y, TILE, TILE };
  SDL_SetRenderDrawColor(R, c.r, c.g, c.b, c.a);
  SDL_RenderDrawRect(R, &rect);
  rect.x += 1; // thick border
  rect.y += 1;
  rect.w -= 2;
  rect.h -= 2;
  SDL_RenderDrawRect(R, &rect);
}

static void draw_pieces(SDL_Renderer* R, board* B, SDL_Texture* tex[TEX_COUNT]) {
  for(int r = 0; r < 8; ++r) {
    for(int f = 0; f < 8; ++f) {
      int i = index_rank(r,f);
      TexId t;
      if(!piece_tex(B, i, &t))
        continue;
      SDL_Rect dst = {
        MARGIN + f * TILE, MARGIN + (7 - r) * TILE, TILE, TILE
      };
      SDL_RenderCopy(R, tex[t], NULL, &dst);
    }
  }
}

static int piece_sdlkey(SDL_Keycode key) {
  switch (key) {
    case SDLK_q: return QUEEN;
    case SDLK_r: return ROOK;
    case SDLK_b: return BISHOP;
    case SDLK_n: return KNIGHT;
    default: return -1;
  }
}

static int sdl_prompt_promotion(SDL_Window* W) {
  SDL_ShowSimpleMessageBox(
    SDL_MESSAGEBOX_INFORMATION,
    "Promotion",
    "Promote to q , r, b, or n",
    W
  );

  SDL_Event e;
  while (SDL_WaitEvent(&e)) {
    if (e.type == SDL_QUIT) {
      return QUEEN; // default if close window
    }
    if (e.type == SDL_KEYDOWN) {
      SDL_Keycode key = e.key.keysym.sym;
      if (key >= 'A' && key <= 'Z') key = key - 'A' + 'a';

      int p = piece_sdlkey(key);
      if (p != -1) return p;
    }
  }

  return QUEEN;
}

int run_sdl(void) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "sdl init: %s\n", SDL_GetError());
    return 1;
  }

  int imgflags = IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_TIF | IMG_INIT_WEBP; // need to load gif
  if (!(IMG_Init(imgflags) & imgflags)) {
    fprintf(stderr, "sdl image wanted flags=%d, got=%d\n", imgflags, IMG_Init(0));
  }

  SDL_Window* W = SDL_CreateWindow("Chess", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H, SDL_WINDOW_SHOWN);
  if (!W) {
    fprintf(stderr, "create window: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Renderer* R = SDL_CreateRenderer(W, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!R) {
    fprintf(stderr, "create renderer: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Texture* tex[TEX_COUNT ]= {0}; // load textures
  for (int i = 0; i < TEX_COUNT; ++i) {
    SDL_Surface* s = IMG_Load(TEX_PATHS[i]);
    if (!s) {
      fprintf(stderr, "image load %s: %s\n", TEX_PATHS[i], IMG_GetError());
    } else {
      tex[i] = SDL_CreateTextureFromSurface(R, s);
      SDL_FreeSurface(s);
      if (!tex[i])
        fprintf(stderr, "create texture %s: %s\n", TEX_PATHS[i], SDL_GetError());
    }
  }

  init_attack_tables();
  init_pesto_tables();
  if (OPENING_BOOK)
    init_opening_book();
  if (!load_openings("high_elo_opening.csv")) {
    fprintf(stderr, "WARNING: failed to load opening book, continuing without it.\n");
  }
#ifdef DEBUG
  printf("Initialized attack & pesto tables and opening book.\n");
#endif
  board* B = init_board();
  // test_position(B, "3q1rk1/2ppbppp/1p6/1N2Qp2/1P6/3B4/rB1P1PPP/n2K2NR b K - 0 1"); // for testing hardcoded positions
  // test_position(B, "3q1rk1/2pp1ppp/1p3b2/1N3Q2/1P6/3B4/rB1P1PPP/n2K2NR b - - 0 2"); // for testing hardcoded positions
  // test_position(B, "k7/1p6/8/8/4KP2/8/8/8 w - - 0 1"); // for testing hardcoded positions
  bot *bwhite = NULL, *bblack = NULL;
  if (WHITE_BOT)
    bwhite = init_bot(B, 1, WHITE_DEPTH, WHITE_LIMIT);
  if (BLACK_BOT)
    bblack = init_bot(B, 0, BLACK_DEPTH, BLACK_LIMIT);

  bool running = true; // ui
  int selected = -1; // selected source square index
  int hover = -1;
  int ply = 0, move = 0;

  while (running) {
    if (B->white && WHITE_BOT) {
      int best = -1;
      int from = -1, to = -1;
      int type = -1;

      if (OPENING_BOOK) {
        open_node *book_node = get_book_move(move_history, history_len);
        if (book_node && book_node->move_code >= 0) {
          best = book_node->move_code;
          from = best / 64;
          to = best % 64;
          type = 0; // opening book
        }
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

      if (!execute(B, from, to, &B->white)) {
        printf("WHITE_BOT invalid move.\n");
        break;
      }
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
    } else if (!B->white && BLACK_BOT) {
      int best = -1;
      int from = -1, to = -1;
      int type = -1;

      if (OPENING_BOOK) {
        open_node *book_node = get_book_move(move_history, history_len);
        if (book_node && book_node->move_code >= 0) {
          best = book_node->move_code;
          from = best / 64;
          to = best % 64;
          type = 0; // opening book
        }
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

      if (!execute(B, from, to, &B->white)) {
        printf("BLACK_BOT invalid!\n");
        break;
      }
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
    }

    SDL_Event e;
    while(SDL_PollEvent(&e)){
      if (e.type == SDL_QUIT)
        running=false;
      else if(e.type == SDL_MOUSEMOTION) {
        int r, f;
        hover = rank_pixel(e.motion.x, e.motion.y, &r, &f) ? index_rank(r,f) : -1;
      } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int r, f;
        if (rank_pixel(e.button.x, e.button.y, &r, &f)) {
          int idx = index_rank(r, f);
          if(selected == -1) {
            TexId t; // pick source if own piece
            if (piece_tex(B, idx, &t)) {
              bool white = (t <= wK);
              if ((B->white && white) || (!B->white && !white))
                selected = idx;
            }
          } else {
            board_snapshot snap; save_snapshot(B, &snap); // attempt move
            int from = selected, to = idx;
            uint64_t to_mask = 1ULL << to;
            int capture = (B->blacks & to_mask) != 0;
            int pt = piece_at(B, from);
            if (from != to) {
              board_snapshot snap;
              save_snapshot(B, &snap);

              int side = B->white;
              uint64_t to_mask = 1ULL << to;
              int capture = side ? ((B->blacks & to_mask) != 0) : ((B->whites & to_mask) != 0);
              int pt = piece_at(B, from);
              if (execute(B, from, to, &B->white)) {
                if (promotion_move(side, pt, to)) {
                  int new_piece = sdl_prompt_promotion(W);
                  apply_promotion(B, side, to, new_piece);
                }

                add_history(pt, from, to, capture);
                ++ply;
                move = ply / 2;
                selected = -1;
                B->white = !B->white;
              } else {
                restore_snapshot(B, &snap);
                TexId t;
                if (piece_tex(B, idx, &t)) {
                  bool white = (t <= wK);
                  if ((B->white && white) || (!B->white && !white))
                    selected = idx;
                }
              }
            }
          }
        } else {
          selected = -1;
        }
      }
    }

    SDL_SetRenderDrawColor(R, 30,30,30,255);
    SDL_RenderClear(R);
    draw_board(R);
    draw_pieces(R, B, tex);

    if (hover >= 0)
      draw_highlight(R, hover, (SDL_Color){ 255, 255, 0, 255 });
    if (selected >= 0)
      draw_highlight(R, selected, (SDL_Color){0, 200, 255, 255});

    SDL_RenderPresent(R);
  }

  free_history();
  free_opening_book();
  free_board(B);
  for (int i = 0; i < TEX_COUNT; ++i)
    if(tex[i])
      SDL_DestroyTexture(tex[i]);
  SDL_DestroyRenderer(R);
  SDL_DestroyWindow(W);
  IMG_Quit();
  SDL_Quit();
  return 0;
}