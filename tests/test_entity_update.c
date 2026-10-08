// tests/test_entity_update.c — FU-67 entity distance/nearest-candidate port (docs/ghidra/FU67_entity_update.md).
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_entity_update.h"

static void test_distance_zero_and_axes(void) {
  assert(fifa96_entity_distance(0, 0) == 0);
  assert(fifa96_entity_distance(100, 0) == 100);
  assert(fifa96_entity_distance(0, 100) == 100);
  assert(fifa96_entity_distance(-100, 0) == 100);
  assert(fifa96_entity_distance(0, -100) == 100);
  assert(fifa96_entity_distance(1, 0) == 1);
  assert(fifa96_entity_distance(0, 1) == 1);
}

static void test_distance_equal_axes(void) {
  assert(fifa96_entity_distance(100, 100) == 137);
  assert(fifa96_entity_distance(20, 20) == 27);
  assert(fifa96_entity_distance(5, 5) == 6);
  assert(fifa96_entity_distance(32767, 32767) == 45054);
}

static void test_distance_octagonal_branches(void) {
  assert(fifa96_entity_distance(100, 60) == 122);
  assert(fifa96_entity_distance(60, 100) == 122);
  assert(fifa96_entity_distance(100, 40) == 110);
  assert(fifa96_entity_distance(100, 50) == 112);
  assert(fifa96_entity_distance(100, 51) == 118);
  assert(fifa96_entity_distance(100, 49) == 112);
  assert(fifa96_entity_distance(3, 4) == 4);
  assert(fifa96_entity_distance(4, 3) == 4);
}

static void test_distance_signed_branches_match(void) {
  assert(fifa96_entity_distance(-100, 60) == 122);
  assert(fifa96_entity_distance(100, -60) == 122);
  assert(fifa96_entity_distance(-100, -60) == 122);
}

static void test_distance_word_semantics(void) {
  assert(fifa96_entity_distance(32767, 0) == 32767);
  assert(fifa96_entity_distance(-32768, 0) == -8192);
  assert(fifa96_entity_distance(0x10000, 0) == 0);
  assert(fifa96_entity_distance(0x18000, 0) == -8192);
}

static void test_nearest_picks_metric_minimum(void) {
  const fifa96_entity_candidate c[3] = {
    {0, 100, 0, 0},
    {100, 0, 0, 0},
    {30, 30, 0, 0},
  };
  int16_t best = 0;
  assert(fifa96_entity_find_nearest(c, 3, 0xFFFFFFFFu, 0, 0, &best) == 2);
  assert(best == 41);
}

static void test_nearest_skip_index(void) {
  const fifa96_entity_candidate c[3] = {
    {0, 100, 0, 0},
    {100, 0, 0, 0},
    {30, 30, 0, 0},
  };
  int16_t best = 0;
  assert(fifa96_entity_find_nearest(c, 3, 2, 0, 0, &best) == 0);
  assert(best == 100);
}

static void test_nearest_skips_flagged_records(void) {
  fifa96_entity_candidate c[3] = {
    {0, 100, 0, 0},
    {100, 0, 0, 0},
    {30, 30, 0, 0},
  };
  int16_t best = 0;
  c[2].skip_9a = 1;
  assert(fifa96_entity_find_nearest(c, 3, 0xFFFFFFFFu, 0, 0, &best) == 0);
  assert(best == 100);
  c[2].skip_9a = 0;
  c[2].skip_98 = 1;
  assert(fifa96_entity_find_nearest(c, 3, 0xFFFFFFFFu, 0, 0, &best) == 0);
  assert(best == 100);
}

static void test_nearest_equal_distances_keep_first(void) {
  const fifa96_entity_candidate c[2] = {
    {100, 0, 0, 0},
    {0, 100, 0, 0},
  };
  int16_t best = 0;
  assert(fifa96_entity_find_nearest(c, 2, 0xFFFFFFFFu, 0, 0, &best) == 0);
  assert(best == 100);
}

static void test_nearest_none_found(void) {
  const fifa96_entity_candidate c[2] = {
    {10, 10, 1, 0},
    {20, 20, 0, 1},
  };
  int16_t best = 0;
  assert(fifa96_entity_find_nearest(c, 2, 0xFFFFFFFFu, 0, 0, &best) == -1);
  assert(best == (int16_t)0xFFFF);
}

static void test_nearest_empty_count(void) {
  const fifa96_entity_candidate c[1] = {{10, 10, 0, 0}};
  int16_t best = 0;
  assert(fifa96_entity_find_nearest(c, 0, 0xFFFFFFFFu, 0, 0, &best) == -1);
  assert(best == (int16_t)0xFFFF);
}

static void test_nearest_target_wraps_to_word(void) {
  const fifa96_entity_candidate c[1] = {{32767, 0, 0, 0}};
  int16_t best = 0;
  assert(fifa96_entity_find_nearest(c, 1, 0xFFFFFFFFu, -32768, 0, &best) == 0);
  assert(best == 1);
}

static void test_nearest_invalid_arguments(void) {
  const fifa96_entity_candidate c[1] = {{0, 0, 0, 0}};
  int16_t best = 0;
  assert(fifa96_entity_find_nearest(NULL, 1, 0, 0, 0, &best) == -FIFA96_ERR_INVALID);
  assert(fifa96_entity_find_nearest(c, 1, 0, 0, 0, NULL) == -FIFA96_ERR_INVALID);
}

/* M2 Task 14 / OL-41: the moved-down `0xCD474`/`0x114E04` primitives. Values
 * are the native algorithm's (atan table flat 0x14072C; sine fold flat
 * 0x114E04), cross-checked against the previous in-place implementations. */
static void test_math_primitives(void) {
  int32_t angle = 0x7FFF;
  assert(fifa96_entity_angle(0x100, 0, &angle) == FIFA96_OK && angle == 0x100);
  assert(fifa96_entity_angle(0, 0x100, &angle) == FIFA96_OK && angle == 0);
  assert(fifa96_entity_angle(0x100, 0x100, &angle) == FIFA96_OK && angle == 0x80);
  assert(fifa96_entity_angle(-0x100, 0, &angle) == FIFA96_OK && angle == -0x100);
  assert(fifa96_entity_angle(0, -0x100, &angle) == FIFA96_OK && angle == 0x200);
  assert(fifa96_entity_angle(0x200, 0x100, &angle) == FIFA96_OK && angle == 180);
  assert(fifa96_entity_angle(0, 0, &angle) == FIFA96_OK && angle == 0x80);
  assert(fifa96_entity_angle(0x100, 0, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_entity_sine(0) == 0);
  assert(fifa96_entity_sine(0x40) == 25079);
  assert(fifa96_entity_sine(0x80) == 46340);
  assert(fifa96_entity_sine(0x100) == 65536);
  assert(fifa96_entity_sine(0x180) == 46340);
  assert(fifa96_entity_sine(0x200) == 0);
  assert(fifa96_entity_sine(0x280) == -46340);
  assert(fifa96_entity_sine(0x300) == -65536);
  assert(fifa96_entity_sine(0x1FF) == 402);
}

/* M2 Task 14 / OL-41: `FUN_0008D824` (`0x8D824..0x8D8EB`). */
static void test_intercept_bind(void) {
  fifa96_entity_intercept_target t = {0x7, 0x7, 0x7};
  assert(fifa96_entity_intercept_bind(0, 0, 0, 0, &t) == FIFA96_OK);
  assert(t.x == 0x240 && t.y == 0 && t.z == 0x468);
  /* side 1 negates z (`0x8D87C NEG [EBX+8]`) */
  assert(fifa96_entity_intercept_bind(0, 1, 0, 0, &t) == FIFA96_OK);
  assert(t.x == 0x240 && t.z == -0x468);
  /* |actor.x| < 0x180, actor.x > 0: x - 0x240 (`0x8D896`) */
  assert(fifa96_entity_intercept_bind(0x100, 0, 0, 0, &t) == FIFA96_OK);
  assert(t.x == -0x140);
  /* |actor.x| < 0x180, actor.x <= 0: 0x240 - x (`0x8D8A3`) */
  assert(fifa96_entity_intercept_bind(-0x100, 0, 0, 0, &t) == FIFA96_OK);
  assert(t.x == 0x340);
  assert(fifa96_entity_intercept_bind(0, 0, 0, 0, &t) == FIFA96_OK);
  assert(t.x == 0x240);
  /* |actor.x| >= 0x180: (x + 0x180) / 3 + 0xC0, IDIV truncation (`0x8D8CE`) */
  assert(fifa96_entity_intercept_bind(0x200, 0, 0, 0, &t) == FIFA96_OK);
  assert(t.x == 0x1EA);
  assert(fifa96_entity_intercept_bind(0x180, 0, 0, 0, &t) == FIFA96_OK);
  assert(t.x == 0x1C0);
  assert(fifa96_entity_intercept_bind(-0x200, 0, 0, 0, &t) == FIFA96_OK);
  assert(t.x == 0x96);
  /* z from |nearest.z| (`0x8D836..0x8D86E`); nearest.x is not read by a live
   * path (the 0x8D8AC arm tests |nearest.x|, which is never negative). */
  assert(fifa96_entity_intercept_bind(0, 0, 0x1234, 0x400, &t) == FIFA96_OK);
  assert(t.z == 0x668);
  assert(fifa96_entity_intercept_bind(0, 0, -0x1234, -0x400, &t) == FIFA96_OK);
  assert(t.z == 0x668);
  assert(fifa96_entity_intercept_bind(0, 0, 0, 0, NULL) == -FIFA96_ERR_INVALID);
}

/* M2 Task 14 / OL-41: `FUN_000795B4` (`0x795B4..0x795F0`) + `FUN_000CD514`
 * (`0xCD514..0xCD563`). Expectations derive from the native algorithm
 * (atan + 0x114E04 divide; verified against the native idiom transcription). */
static void test_intercept_band(void) {
  fifa96_entity_intercept_band_out b = {0x7, 0x7, 0x7};
  assert(fifa96_entity_intercept_band(0, 0, 0x100, 0, &b) == FIFA96_OK);
  assert(b.band == 0x100 && b.dx == 0x100 && b.dz == 0);
  assert(fifa96_entity_intercept_band(0, 0, 0, 0x100, &b) == FIFA96_OK);
  assert(b.band == 0 && b.dx == 0 && b.dz == 0x100);
  assert(fifa96_entity_intercept_band(0, 0, 0x100, 0x100, &b) == FIFA96_OK);
  assert(b.band == 0x16A && b.dx == 0x100 && b.dz == 0x100);
  assert(fifa96_entity_intercept_band(0, 0, -0x100, 0, &b) == FIFA96_OK);
  assert(b.band == 0x100 && b.dx == -0x100 && b.dz == 0);
  assert(fifa96_entity_intercept_band(0, 0, 0x240, 0x468, &b) == FIFA96_OK);
  assert(b.band == 0x286 && b.dx == 0x240 && b.dz == 0x468);
  assert(fifa96_entity_intercept_band(0x10, 0x10, 0, 0, &b) == FIFA96_OK);
  assert(b.dx == -0x10 && b.dz == -0x10);
  assert(fifa96_entity_intercept_band(0, 0, 0x10, 0, &b) == FIFA96_OK);
  assert(b.band == 0x10);
  assert(fifa96_entity_intercept_band(0, 0, 0, 0, NULL) == -FIFA96_ERR_INVALID);
}

int main(void) {
  test_distance_zero_and_axes();
  test_distance_equal_axes();
  test_distance_octagonal_branches();
  test_distance_signed_branches_match();
  test_distance_word_semantics();
  test_nearest_picks_metric_minimum();
  test_nearest_skip_index();
  test_nearest_skips_flagged_records();
  test_nearest_equal_distances_keep_first();
  test_nearest_none_found();
  test_nearest_empty_count();
  test_nearest_target_wraps_to_word();
  test_nearest_invalid_arguments();
  test_math_primitives();
  test_intercept_bind();
  test_intercept_band();
  puts("test_entity_update: ok");
  return 0;
}
