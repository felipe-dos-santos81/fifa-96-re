// tests/test_scene.c — FU-89 scene assembly model
// (docs/ghidra/FU89_scene_assembly.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_projection.h"
#include "fifa96_loader/fifa96_scene.h"

_Static_assert(offsetof(fifa96_scene_slot, clean) == 0, "slot clean");
_Static_assert(offsetof(fifa96_scene_slot, jitter) == 8, "slot jitter");
_Static_assert(offsetof(fifa96_scene_slot, clean_visible) == 16, "slot clean flag");
_Static_assert(offsetof(fifa96_scene_slot, jitter_visible) == 17, "slot jitter flag");
_Static_assert(FIFA96_SCENE_POSITION_DWORDS == 3, "position stride");
_Static_assert(FIFA96_SCENE_SLOT_STRIDE == 8, "slot stride");
_Static_assert(FIFA96_SCENE_HIDDEN_Y == -10000, "hidden marker");
_Static_assert(FIFA96_SCENE_LATERAL_MAX == 0x8E0, "lateral gate");

#define P_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static int32_t recip_x[FIFA96_PROJECTION_RECIP_COUNT];
static int32_t recip_y[FIFA96_PROJECTION_RECIP_COUNT];

static void test_build_keys(void) {
  const int32_t positions[12] = {1, 2, 10, 3, 4, 20, 5, 6, 30, 7, 8, 40};
  const uint32_t list[4] = {0, 3, 1, 2};
  int32_t keys[5];
  keys[0] = 0x7F;
  assert(fifa96_scene_build_keys(4, list, positions, 4, keys) == FIFA96_OK);
  assert(keys[0] == 0x7F);
  assert(keys[1] == 10 && keys[2] == 40 && keys[3] == 20 && keys[4] == 30);
  assert(fifa96_scene_build_keys(0, list, positions, 4, keys) == FIFA96_OK);
  assert(keys[0] == 0x7F && keys[1] == 10);
  const uint32_t bad[1] = {4};
  assert(fifa96_scene_build_keys(1, bad, positions, 4, keys) == P_INVALID);
  assert(fifa96_scene_build_keys(4, NULL, positions, 4, keys) == P_INVALID);
  assert(fifa96_scene_build_keys(4, list, NULL, 4, keys) == P_INVALID);
  assert(fifa96_scene_build_keys(4, list, positions, 4, NULL) == P_INVALID);
}

static void test_sort(void) {
  int32_t keys[4] = {4, 2, 9, 1};
  uint32_t values[4] = {10, 11, 12, 13};
  assert(fifa96_scene_sort(4, keys, values) == FIFA96_OK);
  const int32_t sorted[4] = {9, 4, 2, 1};
  const uint32_t expect[4] = {12, 10, 11, 13};
  assert(memcmp(keys, sorted, sizeof(keys)) == 0);
  assert(memcmp(values, expect, sizeof(values)) == 0);
  int32_t eq[3] = {5, 5, 5};
  uint32_t eqv[3] = {7, 8, 9};
  assert(fifa96_scene_sort(3, eq, eqv) == FIFA96_OK);
  assert(eqv[0] == 7 && eqv[1] == 8 && eqv[2] == 9);
  assert(fifa96_scene_sort(0, NULL, NULL) == FIFA96_OK);
  assert(fifa96_scene_sort(1, NULL, NULL) == FIFA96_OK);
  assert(fifa96_scene_sort(2, NULL, values) == P_INVALID);
  assert(fifa96_scene_sort(2, keys, NULL) == P_INVALID);
}

static void test_slot_gate(void) {
  uint8_t visible;
  assert(fifa96_scene_slot_gate(80, 80, 0, 0x8E0, &visible) == FIFA96_OK);
  assert(visible == 1);
  assert(fifa96_scene_slot_gate(81, 80, 0, 0, &visible) == FIFA96_OK);
  assert(visible == 0);
  assert(fifa96_scene_slot_gate(0, 0, FIFA96_SCENE_HIDDEN_Y, 0, &visible) == FIFA96_OK);
  assert(visible == 0);
  assert(fifa96_scene_slot_gate(0, 0, 0, 0x8E1, &visible) == FIFA96_OK);
  assert(visible == 0);
  assert(fifa96_scene_slot_gate(0, 0, 0, 0, NULL) == P_INVALID);
}

static void test_depth_override(void) {
  int32_t y;
  uint8_t draw;
  assert(fifa96_scene_depth_override(0, 0, 0, 100, 40, 100, 100, &y, &draw) == FIFA96_OK);
  assert(draw == 0 && y == 100);
  assert(fifa96_scene_depth_override(0, 50, 0, 100, 40, 100, 50, &y, &draw) == FIFA96_OK);
  assert(draw == 0 && y == 100);
  assert(fifa96_scene_depth_override(0, 50, 200, 100, 80, 100, 50, &y, &draw) == FIFA96_OK);
  assert(draw == 1 && y == 50);
  assert(fifa96_scene_depth_override(0, 50, 200, 100, 80, 300, 50, &y, &draw) == FIFA96_OK);
  assert(draw == 0 && y == 100);
  assert(fifa96_scene_depth_override(400, 50, 200, 100, 80, 250, 50, &y, &draw) == FIFA96_OK);
  assert(draw == 0 && y == 100);
  assert(fifa96_scene_depth_override(400, 50, 200, 100, 80, 259, 50, &y, &draw) == FIFA96_OK);
  assert(draw == 0 && y == 100);
  assert(fifa96_scene_depth_override(0, 0, 0, 0, 0, 0, 0, NULL, &draw) == P_INVALID);
  assert(fifa96_scene_depth_override(0, 0, 0, 0, 0, 0, 0, &y, NULL) == P_INVALID);
}

static void test_threshold(void) {
  int32_t out;
  assert(fifa96_scene_threshold(10, 0, 0, 0, &out) == FIFA96_OK);
  assert(out == 0x10000);
  assert(fifa96_scene_threshold(100, 0, 0, 0, &out) == FIFA96_OK);
  assert(out == 6553);
  assert(fifa96_scene_threshold(1000, 0, 0, 0, &out) == FIFA96_OK);
  assert(out == 655);
  assert(fifa96_scene_threshold(10000, 0, 0, 0, &out) == FIFA96_OK);
  assert(out == 0x78);
  assert(fifa96_scene_threshold(10, 0, 0x1000000, 10, &out) == FIFA96_OK);
  assert(out == 0x1000000);
  assert(fifa96_scene_threshold(10, 0, 1, 10, &out) == FIFA96_OK);
  assert(out == 0x78);
  assert(fifa96_scene_threshold(-1, 0, 0, 0, &out) == FIFA96_OK);
  assert(out == 0x78);
  assert(fifa96_scene_threshold(0, 0, 0, 0, &out) == P_INVALID);
  assert(fifa96_scene_threshold(10, 0, 0, 0, NULL) == P_INVALID);
}

static void test_reproject(void) {
  const int32_t prev[3] = {100, 200, 300};
  const int32_t same[3] = {100, 200, 300};
  const int32_t moved[3] = {100, 200, 301};
  const int32_t cached[6] = {1, 2, 3, 4, 5, 6};
  const int32_t angles[2] = {1, 2};
  const int32_t other[4] = {3, 4, 5, 6};
  const int32_t bad_other[4] = {3, 4, 5, 7};
  uint8_t reproject;
  assert(fifa96_scene_reproject(prev, same, 1, cached, angles, other, &reproject) == FIFA96_OK);
  assert(reproject == 0);
  assert(fifa96_scene_reproject(prev, moved, 1, cached, angles, other, &reproject) == FIFA96_OK);
  assert(reproject == 1);
  assert(fifa96_scene_reproject(prev, same, 0, cached, angles, other, &reproject) == FIFA96_OK);
  assert(reproject == 1);
  assert(fifa96_scene_reproject(prev, same, 1, cached, angles, bad_other, &reproject) == FIFA96_OK);
  assert(reproject == 1);
  assert(fifa96_scene_reproject(prev, same, 1, cached, angles, other, NULL) == P_INVALID);
  assert(fifa96_scene_reproject(NULL, same, 1, cached, angles, other, &reproject) == P_INVALID);
  assert(fifa96_scene_reproject(prev, same, 1, NULL, angles, other, &reproject) == P_INVALID);
}

static void test_slot_project(void) {
  fifa96_projection_point center;
  fifa96_projection_vec clean;
  fifa96_projection_vec jitter;
  fifa96_scene_slot slot;
  fifa96_projection_point expect;
  uint8_t visible;
  center.x = 160 << 16;
  center.y = 100 << 16;
  assert(fifa96_projection_reciprocal(320, recip_x) == FIFA96_OK);
  assert(fifa96_projection_reciprocal(200, recip_y) == FIFA96_OK);
  clean.x = 513;
  clean.y = 0;
  clean.z = 0x2000;
  jitter.x = 513;
  jitter.y = 0x100;
  jitter.z = 0x2000;
  slot.clean.x = -1;
  slot.clean.y = -1;
  slot.jitter.x = -1;
  slot.jitter.y = -1;
  assert(fifa96_scene_slot_project(recip_x, recip_y, &center, &clean, &jitter, &slot) == FIFA96_OK);
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &clean, &expect, &visible) ==
         FIFA96_OK);
  assert(slot.clean_visible == visible && slot.clean.x == expect.x && slot.clean.y == expect.y);
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &jitter, &expect, &visible) ==
         FIFA96_OK);
  assert(slot.jitter_visible == visible && slot.jitter.x == expect.x && slot.jitter.y == expect.y);
  clean.z = 4;
  visible = 9;
  slot.clean.x = 0x1234;
  slot.clean.y = 0x5678;
  assert(fifa96_scene_slot_project(recip_x, recip_y, &center, &clean, &jitter, &slot) == FIFA96_OK);
  assert(slot.clean_visible == 0);
  assert(slot.clean.x == 0x1234 && slot.clean.y == 0x5678);
  assert(fifa96_scene_slot_project(NULL, recip_y, &center, &clean, &jitter, &slot) == P_INVALID);
  assert(fifa96_scene_slot_project(recip_x, recip_y, &center, NULL, &jitter, &slot) == P_INVALID);
  assert(fifa96_scene_slot_project(recip_x, recip_y, &center, &clean, &jitter, NULL) == P_INVALID);
}

int main(void) {
  test_build_keys();
  test_sort();
  test_slot_gate();
  test_depth_override();
  test_threshold();
  test_reproject();
  test_slot_project();
  puts("test_scene: all assertions passed");
  return 0;
}
