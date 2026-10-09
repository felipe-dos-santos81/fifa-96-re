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
int fifa96_window_scale(const fifa96_window *win, int32_t *scale_x, int32_t *scale_y);
int fifa96_window_zoomed(const fifa96_window *win, int *zoomed);

/* FU-152 §2.6/§4.1 (P4): the substitution-strip layout FUN_00053240 writes
 * into the 0x14E53C block (stride 0x1C per side). First-hand 0x53240 disasm
 * tail: y0 = box y0; height = the Frames.fsh frame-5 height `[+2]>>16`,
 * doubled when the settings-4 video flag (FUN_00044BE0) is set; row 0 x =
 * x0 + ((4*scale_x + 0x8000) >> 16); row 1 x = x1 - ((4*scale_x + 0x8000) >>
 * 16) - ((name_width*scale_x + 0x8000) >> 16); y_text = y0 + ((12*scale_y +
 * 0x8000) >> 16). Native cells: 0x14E53C/40/44/48 (side 0) and
 * 0x14E558/5C/60/64 (side 1). */
typedef struct fifa96_window_strip_row {
  int32_t y0;       /* 0x14E53C + side*0x1C */
  int32_t height;   /* 0x14E540 + side*0x1C */
  int32_t x;        /* 0x14E544 (side 0) / 0x14E560 (side 1) */
  int32_t y_text;   /* 0x14E548 + side*0x1C */
} fifa96_window_strip_row;

typedef struct fifa96_window_strip {
  fifa96_window_strip_row row[2];
} fifa96_window_strip;

/* Compute the strip layout for a window whose zoom comes from
 * `fifa96_window_scale`. `frame5_height` is the Frames.fsh frame-5 height
 * (the native `[0x14E638+2]` high word), `name_width` the staged measured
 * team-name width (the engine analog of the native `[0x14E63A]` word), `wide`
 * the settings-4 predicate. Returns FIFA96_OK or -FIFA96_ERR_INVALID. */
int fifa96_window_strip_layout(const fifa96_window *win, int32_t frame5_height,
                               int32_t name_width, int wide,
                               fifa96_window_strip *out);
