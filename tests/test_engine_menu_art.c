/* tests/test_engine_menu_art.c — pins a deterministic menu frame hash */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_menu_art.h"
#include "fifa96_engine/fifa96_surface.h"

static uint64_t draw_hash(struct fifa96_surface *s, uint32_t entry_state, int row, int cursor) {
  fifa96_surface_clear(s, 0);
  struct fifa96_menu_state st = {.entry_state = entry_state, .selected_row = row,
                                 .cursor_on = cursor};
  fifa96_menu_art_draw(s, &st);
  return fifa96_surface_hash(s);
}

static void test_fallback_hash(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  assert(fifa96_menu_art_init(NULL) == 0);
  uint64_t h = draw_hash(s, 0, 0, 1);
  printf("FIFA96_MENU_FALLBACK_HASH = 0x%016llx\n", (unsigned long long)h);
  fflush(stdout);
  assert(h == FIFA96_MENU_FALLBACK_HASH);
  fifa96_surface_destroy(s);
}

static void test_fallback_deterministic(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  assert(fifa96_menu_art_init(NULL) == 0);
  uint64_t a = draw_hash(s, 0, 0, 1);
  uint64_t b = draw_hash(s, 0, 0, 1);
  assert(a == b);
  fifa96_surface_destroy(s);
}

static void test_state_changes_frame(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  assert(fifa96_menu_art_init(NULL) == 0);
  uint64_t cursor_on = draw_hash(s, 0, 0, 1);
  uint64_t cursor_off = draw_hash(s, 0, 0, 0);
  uint64_t row1 = draw_hash(s, 0, 1, 1);
  uint64_t row0 = draw_hash(s, 0, 0, 1);
  assert(cursor_on != cursor_off);
  assert(row1 != row0);
  fifa96_surface_destroy(s);
}

static void test_draw_differs_from_clear(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  fifa96_surface_clear(s, 0);
  uint64_t cleared = fifa96_surface_hash(s);
  assert(fifa96_menu_art_init(NULL) == 0);
  struct fifa96_menu_state st = {.entry_state = 17, .selected_row = 3, .cursor_on = 1};
  fifa96_menu_art_draw(s, &st);
  assert(fifa96_surface_hash(s) != cleared);
  fifa96_surface_destroy(s);
}

static void test_empty_asset_table_falls_back(void) {
  struct fifa96_asset_table t;
  memset(&t, 0, sizeof t);
  assert(fifa96_menu_art_init(&t) == 0);
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  uint64_t h = draw_hash(s, 0, 0, 1);
  assert(h == FIFA96_MENU_FALLBACK_HASH);
  fifa96_surface_destroy(s);
}

static void test_null_args(void) {
  assert(fifa96_menu_art_init(NULL) == 0);
  struct fifa96_menu_state st = {0, 0, 0};
  fifa96_menu_art_draw(NULL, &st);
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  fifa96_menu_art_draw(s, NULL);
  fifa96_surface_destroy(s);
}

int main(void) {
  test_fallback_hash();
  test_fallback_deterministic();
  test_state_changes_frame();
  test_draw_differs_from_clear();
  test_empty_asset_table_falls_back();
  test_null_args();
  puts("test_engine_menu_art OK");
  return 0;
}
