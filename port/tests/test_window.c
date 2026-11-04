#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_window.h"

#define W_INVALID (-FIFA96_ERR_INVALID)

static void test_init(void) {
  fifa96_window win;
  assert(fifa96_window_init(&win, 320, 200) == FIFA96_OK);
  assert(win.screen_w == 320 && win.screen_h == 200);
  assert(win.box.x0 == 0 && win.box.x1 == 0);
  assert(fifa96_window_init(NULL, 320, 200) == W_INVALID);
}

static void test_set(void) {
  fifa96_window win;
  fifa96_window_init(&win, 320, 200);
  assert(fifa96_window_set(&win, 0, 0, 160, 100) == FIFA96_OK);
  assert(win.box.x0 == 0 && win.box.y0 == 0);
  assert(win.box.x1 == 160 && win.box.y1 == 100);
  assert(win.box.w == 160 && win.box.h == 100);
  assert(win.box.x0_fix == 0 && win.box.y0_fix == 0);
  assert(win.box.x1_fix == (160 << 16) && win.box.y1_fix == (100 << 16));
  assert(win.src_x == 0 && win.src_y == 0 && win.src_w == 160 && win.src_h == 100);
  assert(win.cx_fix == (80 << 16) && win.cy_fix == (50 << 16));

  assert(fifa96_window_set(&win, 0, 0, 4, 4) == FIFA96_OK);
  assert(win.box.w == 8 && win.box.h == 8);
  assert(win.box.x1 == 8 && win.box.y1 == 8);

  assert(fifa96_window_set(&win, -10, -10, 320, 200) == FIFA96_OK);
  assert(win.box.x0 == 0 && win.box.y0 == 0);
  assert(win.box.x1 == 320 && win.box.y1 == 200);

  assert(fifa96_window_set(&win, 1000, 1000, 160, 100) == FIFA96_OK);
  assert(win.box.x0 == 160 && win.box.y0 == 100);
  assert(win.box.x1 == 320 && win.box.y1 == 200);

  assert(fifa96_window_set(&win, 0, 0, 1000, 1000) == FIFA96_OK);
  assert(win.box.w == 320 && win.box.h == 200);
  assert(win.box.x0 == 0 && win.box.y0 == 0);
  assert(win.box.x1 == 320 && win.box.y1 == 200);

  assert(fifa96_window_set(NULL, 0, 0, 160, 100) == W_INVALID);
}

static void test_define_full(void) {
  fifa96_window win;
  fifa96_window_init(&win, 320, 200);
  assert(fifa96_window_define_full(&win, 160, 100) == FIFA96_OK);
  assert(win.screen_w == 160 && win.screen_h == 100);
  assert(win.box.x0 == 0 && win.box.y0 == 0);
  assert(win.box.x1 == 160 && win.box.y1 == 100);
  assert(win.src_w == 160 && win.src_h == 100);
  assert(fifa96_window_define_full(NULL, 160, 100) == W_INVALID);
}

static void test_expand(void) {
  fifa96_window win;
  fifa96_window_box saved;
  fifa96_window_init(&win, 320, 200);
  fifa96_window_set(&win, 10, 10, 100, 50);
  const int32_t pts[4] = {5, 8, 200, 70};
  assert(fifa96_window_expand(&win, &saved, pts, 2) == FIFA96_OK);
  assert(saved.x0 == 10 && saved.y0 == 10);
  assert(saved.x1 == 200 && saved.y1 == 70);
  assert(saved.w == 100 && saved.h == 50);
  assert(win.box.x1 == 110 && win.box.y1 == 60);

  assert(fifa96_window_expand(&win, &saved, pts, 1) == FIFA96_OK);
  assert(saved.x0 == 10 && saved.y0 == 10);
  assert(saved.x1 == 110 && saved.y1 == 60);

  assert(fifa96_window_expand(&win, &saved, NULL, 0) == FIFA96_OK);
  assert(saved.x0 == 10 && saved.x1 == 110);

  assert(fifa96_window_expand(NULL, &saved, pts, 2) == W_INVALID);
  assert(fifa96_window_expand(&win, NULL, pts, 2) == W_INVALID);
  assert(fifa96_window_expand(&win, &saved, pts, 5) == W_INVALID);
  assert(fifa96_window_expand(&win, &saved, NULL, 2) == W_INVALID);
  assert(fifa96_window_expand(&win, &saved, pts, -1) == W_INVALID);
}

static void test_scale(void) {
  fifa96_window win;
  int32_t sx = 0, sy = 0;
  int zoomed = -1;
  fifa96_window_init(&win, 320, 200);

  fifa96_window_set(&win, 0, 0, 320, 200);
  assert(fifa96_window_scale(&win, &sx, &sy) == FIFA96_OK);
  assert(sx == 0x10000 && sy == 0x10000);
  assert(fifa96_window_zoomed(&win, &zoomed) == FIFA96_OK && zoomed == 0);

  fifa96_window_set(&win, 0, 0, 160, 100);
  assert(fifa96_window_scale(&win, &sx, &sy) == FIFA96_OK);
  assert(sx == 0x8000 && sy == 0x8000);
  assert(fifa96_window_zoomed(&win, &zoomed) == FIFA96_OK && zoomed == 1);

  fifa96_window_set(&win, 0, 0, 200, 150);
  assert(fifa96_window_scale(&win, &sx, &sy) == FIFA96_OK);
  assert(sx == (200 * 65536) / 320 && sy == (150 * 65536) / 200);

  assert(fifa96_window_scale(NULL, &sx, &sy) == W_INVALID);
  assert(fifa96_window_scale(&win, NULL, &sy) == W_INVALID);
  assert(fifa96_window_scale(&win, &sx, NULL) == W_INVALID);
  assert(fifa96_window_zoomed(NULL, &zoomed) == W_INVALID);
  assert(fifa96_window_zoomed(&win, NULL) == W_INVALID);
}

/* FU-152 §2.6/§4.1 (P4): the FUN_00053240 substitution-strip layout block
 * (0x14E53C + side*0x1C): y0, row height (frame-5 height, doubled when the
 * settings-4 "wide" flag is set), x0+4*scale / x1-4*scale-name_width*scale,
 * y0+12*scale, all fixed-point rounded `(v*scale + 0x8000) >> 16`. */
static void test_strip_layout(void) {
  fifa96_window win;
  fifa96_window_strip strip;
  fifa96_window_init(&win, 320, 200);

  fifa96_window_set(&win, 0, 0, 320, 200);   /* scale 0x10000 */
  assert(fifa96_window_strip_layout(&win, 12, 40, 0, &strip) == FIFA96_OK);
  assert(strip.row[0].y0 == 0 && strip.row[1].y0 == 0);
  assert(strip.row[0].height == 12 && strip.row[1].height == 12);
  assert(strip.row[0].x == 4);               /* x0 + 4*scale */
  assert(strip.row[1].x == 320 - 4 - 40);    /* x1 - 4*scale - 40*scale */
  assert(strip.row[0].y_text == 12);         /* y0 + 12*scale */
  assert(strip.row[1].y_text == 12);

  /* Zoomed window (10,20,160,100): scale 0x8000; 4*scale rounds to 2,
   * 12*scale rounds to 6, 40*scale rounds to 20. */
  fifa96_window_set(&win, 10, 20, 160, 100);
  assert(fifa96_window_strip_layout(&win, 12, 40, 0, &strip) == FIFA96_OK);
  assert(strip.row[0].y0 == 20 && strip.row[1].y0 == 20);
  assert(strip.row[0].height == 12 && strip.row[1].height == 12);
  assert(strip.row[0].x == 10 + 2);
  assert(strip.row[1].x == 170 - 2 - 20);
  assert(strip.row[0].y_text == 20 + 6);
  assert(strip.row[1].y_text == 20 + 6);

  /* The wide branch doubles the frame-5 height for both side rows. */
  assert(fifa96_window_strip_layout(&win, 12, 40, 1, &strip) == FIFA96_OK);
  assert(strip.row[0].height == 24 && strip.row[1].height == 24);

  assert(fifa96_window_strip_layout(NULL, 12, 40, 0, &strip) == W_INVALID);
  assert(fifa96_window_strip_layout(&win, 12, 40, 0, NULL) == W_INVALID);
}

int main(void) {
  test_init();
  test_set();
  test_define_full();
  test_expand();
  test_scale();
  test_strip_layout();
  puts("test_window: all assertions passed");
  return 0;
}
