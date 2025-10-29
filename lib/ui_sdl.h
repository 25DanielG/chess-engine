#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include "board.h"

#define TILE (50) // pixel size of square
#define MARGIN (32) // window padding
#define BOARD_PIX (TILE * 8)
#define WIN_W (BOARD_PIX + MARGIN * 2)
#define WIN_H (BOARD_PIX + MARGIN * 2)

typedef enum {
    wP, wN, wB, wR, wQ, wK,
    bP, bN, bB, bR, bQ, bK,
    TEX_COUNT
} TexId;

static inline int index_rank(int rank, int file);
static inline int rank_pixel(int x, int y, int* rank, int* file);
static int piece_tex(const board* B, int index, TexId* out);
static void draw_board(SDL_Renderer* R);
static void draw_highlight(SDL_Renderer* R, int index, SDL_Color c);
static void draw_pieces(SDL_Renderer* R, board* B, SDL_Texture* tex[TEX_COUNT]);
int run_sdl(void);