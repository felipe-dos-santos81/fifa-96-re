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

int main(void) {
  test_init();
  test_set();
  test_define_full();
  test_expand();
  puts("test_window: all assertions passed");
  return 0;
}
