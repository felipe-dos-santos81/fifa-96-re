#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_WINDOW_MIN 8
#define FIFA96_WINDOW_POINTS_MAX 4

typedef struct fifa96_window_box {
  int32_t x0, y0, x1, y1;
  int32_t w, h;
  int32_t x0_fix, y0_fix, x1_fix, y1_fix;
} fifa96_window_box;

typedef struct fifa96_window {
  int32_t screen_w, screen_h;
  fifa96_window_box box;
  int32_t src_x, src_y, src_w, src_h;
  int32_t cx_fix, cy_fix;
} fifa96_window;

int fifa96_window_init(fifa96_window *win, int32_t screen_w, int32_t screen_h);
int fifa96_window_define_full(fifa96_window *win, int32_t w, int32_t h);
int fifa96_window_set(fifa96_window *win, int32_t x, int32_t y, int32_t w, int32_t h);
int fifa96_window_expand(const fifa96_window *win, fifa96_window_box *saved,
                         const int32_t *pts, int count);
