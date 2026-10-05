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
  puts("test_entity_update: ok");
  return 0;
}
