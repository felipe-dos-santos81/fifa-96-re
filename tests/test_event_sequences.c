// tests/test_event_sequences.c — FU-82 event/sequence family progression model
// (docs/ghidra/FU82_event_sequences.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_action_handlers.h"

_Static_assert(offsetof(fifa96_action_sequence_lane_out, z) == 0, "z");
_Static_assert(offsetof(fifa96_action_sequence_lane_out, x) == 4, "x");
_Static_assert(offsetof(fifa96_action_sequence_lane_out, threshold) == 8, "threshold");
_Static_assert(offsetof(fifa96_action_sequence_duel, reset) == 0, "reset");
_Static_assert(offsetof(fifa96_action_sequence_duel, event_id) == 1, "event_id");
_Static_assert(offsetof(fifa96_action_sequence_press, fire) == 0, "fire");
_Static_assert(offsetof(fifa96_action_sequence_press, reset) == 1, "reset");

#define SEQ_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static void test_sequence_select(void) {
  static const uint32_t arms4[4] = {0x7DC6Bu, 0x7DCAFu, 0x7DD29u, 0x7DFB2u};
  static const uint32_t arms7[7] = {0x881EDu, 0x8847Eu, 0x88568u, 0x8859Bu,
                                    0x885FEu, 0x88645u, 0x88699u};
  uint32_t arm = 0xDEADBEEFu;
  assert(fifa96_action_sequence_select(0, arms4, 4, &arm) == FIFA96_OK);
  assert(arm == 0x7DC6Bu);
  assert(fifa96_action_sequence_select(3, arms4, 4, &arm) == FIFA96_OK);
  assert(arm == 0x7DFB2u);
  assert(fifa96_action_sequence_select(4, arms4, 4, &arm) == SEQ_INVALID);
  assert(arm == 0x7DFB2u);
  assert(fifa96_action_sequence_select(0, arms7, 7, &arm) == FIFA96_OK);
  assert(arm == 0x881EDu);
  assert(fifa96_action_sequence_select(6, arms7, 7, &arm) == FIFA96_OK);
  assert(arm == 0x88699u);
  assert(fifa96_action_sequence_select(7, arms7, 7, &arm) == SEQ_INVALID);
  assert(fifa96_action_sequence_select(0xFF, arms7, 7, &arm) == SEQ_INVALID);
  assert(fifa96_action_sequence_select(0, NULL, 4, &arm) == SEQ_INVALID);
  assert(fifa96_action_sequence_select(0, arms4, 0, &arm) == SEQ_INVALID);
  assert(fifa96_action_sequence_select(0, arms4, 4, NULL) == SEQ_INVALID);
}

static void test_sequence_event(void) {
  uint8_t post = 0xAA;
  assert(fifa96_action_sequence_event(0x48, 0x4F, &post) == FIFA96_OK);
  assert(post == 1);
  assert(fifa96_action_sequence_event(0x4F, 0x4F, &post) == FIFA96_OK);
  assert(post == 0);
  assert(fifa96_action_sequence_event(0x00, 0x00, &post) == FIFA96_OK);
  assert(post == 0);
  assert(fifa96_action_sequence_event(0xFF, 0x00, &post) == FIFA96_OK);
  assert(post == 1);
  assert(fifa96_action_sequence_event(0, 0, NULL) == SEQ_INVALID);
}

static void test_sequence_marker(void) {
  uint8_t match = 0xAA;
  assert(fifa96_action_sequence_marker(0x48, 0x48, &match) == FIFA96_OK);
  assert(match == 1);
  assert(fifa96_action_sequence_marker(0x47, 0x48, &match) == FIFA96_OK);
  assert(match == 0);
  assert(fifa96_action_sequence_marker(0x00, 0x48, &match) == FIFA96_OK);
  assert(match == 0);
  assert(fifa96_action_sequence_marker(0x48, 0x00, &match) == FIFA96_OK);
  assert(match == 0);
  assert(fifa96_action_sequence_marker(0x48, 0x48, NULL) == SEQ_INVALID);
}

static void test_sequence_rng_event(void) {
  uint8_t id = 0xAA;
  assert(fifa96_action_sequence_rng_event(0x10u, 0x55, 0x6A, &id) == FIFA96_OK);
  assert(id == 0x55);
  assert(fifa96_action_sequence_rng_event(0x11u, 0x55, 0x6A, &id) == FIFA96_OK);
  assert(id == 0x6A);
  assert(fifa96_action_sequence_rng_event(0xFFFFFFFFu, 0x46, 0x47, &id) == FIFA96_OK);
  assert(id == 0x47);
  assert(fifa96_action_sequence_rng_event(0x00u, 0x03, 0x15, &id) == FIFA96_OK);
  assert(id == 0x03);
  assert(fifa96_action_sequence_rng_event(0x00u, 0x00, 0x00, NULL) == SEQ_INVALID);
}

static void test_sequence_countdown(void) {
  uint16_t count = 0xAAAA;
  assert(fifa96_action_sequence_countdown(0x00u, &count) == FIFA96_OK);
  assert(count == 0x20);
  assert(fifa96_action_sequence_countdown(0x7Fu, &count) == FIFA96_OK);
  assert(count == 0x9F);
  assert(fifa96_action_sequence_countdown(0x80u, &count) == FIFA96_OK);
  assert(count == 0x20);
  assert(fifa96_action_sequence_countdown(0xFFFFFFFFu, &count) == FIFA96_OK);
  assert(count == 0x9F);
  assert(fifa96_action_sequence_countdown(0x00u, NULL) == SEQ_INVALID);
}

static void test_sequence_anim_byte(void) {
  static const uint8_t table[4] = {0x00u, 0x02u, 0x05u, 0xFFu};
  uint16_t value = 0xAAAAu;
  assert(fifa96_action_sequence_anim_byte(table, 0, &value) == FIFA96_OK);
  assert(value == 0xFFFEu);
  assert(fifa96_action_sequence_anim_byte(table, 1, &value) == FIFA96_OK);
  assert(value == 0x0000u);
  assert(fifa96_action_sequence_anim_byte(table, 2, &value) == FIFA96_OK);
  assert(value == 0x0003u);
  assert(fifa96_action_sequence_anim_byte(table, 3, &value) == FIFA96_OK);
  assert(value == 0x00FDu);
  assert(fifa96_action_sequence_anim_byte(NULL, 0, &value) == SEQ_INVALID);
  assert(fifa96_action_sequence_anim_byte(table, 0, NULL) == SEQ_INVALID);
}

static void test_sequence_lane(void) {
  fifa96_action_sequence_lane_out out;
  assert(fifa96_action_sequence_lane(2, 1, (int32_t)3 << 25, &out) == FIFA96_OK);
  assert(out.z == 12);
  assert(out.x == 0x7E0 + 12);
  assert(out.threshold == ((2 * (3 * 60)) >> 4) + 60);
  assert(fifa96_action_sequence_lane(2, 0, (int32_t)3 << 25, &out) == FIFA96_OK);
  assert(out.z == -12);
  assert(out.x == 0x7E0 + 12);
  assert(out.threshold == (2 * (3 * 60)) >> 4);
  assert(fifa96_action_sequence_lane(0, 1, 0, &out) == FIFA96_OK);
  assert(out.z == 0 && out.x == 0x7E0 && out.threshold == 60);
  assert(fifa96_action_sequence_lane(0, 0, 0, &out) == FIFA96_OK);
  assert(out.z == 0 && out.x == 0x7E0 && out.threshold == 0);
  assert(fifa96_action_sequence_lane(2, 1, (int32_t)0xFE000000, &out) == FIFA96_OK);
  assert(out.threshold == ((-120) >> 4) + 60);
  assert(fifa96_action_sequence_lane(1, 0, (int32_t)0x01FFFFFF, &out) == FIFA96_OK);
  assert(out.z == -6);
  assert(out.x == 0x7E0 + 6);
  assert(out.threshold == 0);
  assert(fifa96_action_sequence_lane(2, 1, 0, NULL) == SEQ_INVALID);
}

static void test_sequence_scatter_celebration(void) {
  static const uint32_t rng[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  fifa96_action_vec3 base;
  fifa96_action_vec3 p[5];
  base.x = 0x100;
  base.y = 0x40;
  base.z = 0x200;
  assert(fifa96_action_sequence_scatter_celebration(&base, 1, -1, rng, p) == FIFA96_OK);
  assert(p[0].x == 0x100 && p[0].y == 0x40 && p[0].z == 0x200);
  assert(p[1].x == 0x1A0 && p[1].y == 0x40 && p[1].z == 0x15F);
  assert(p[2].x == 0x2E2 && p[2].y == 0x40 && p[2].z == 0x13C);
  assert(p[3].x == 0x5DC && p[3].y == 0x40 && p[3].z == -0x9);
  assert(p[4].x == 0x632 && p[4].y == 0x40 && p[4].z == -0x290);
  base.x = 0x700;
  base.y = 0;
  base.z = -0xB00;
  assert(fifa96_action_sequence_scatter_celebration(&base, 1, -1, rng, p) == FIFA96_OK);
  assert(p[1].x == 0x720);
  assert(p[1].z == -0xB10);
  assert(p[2].x == 0x720);
  assert(p[3].x == 0x5DC);
  assert(p[4].x == 0x632);
  assert(fifa96_action_sequence_scatter_celebration(&base, 0, 0, rng, p) == FIFA96_OK);
  assert(p[1].x == 0x700 && p[1].z == -0xB00);
  assert(p[3].x == 0 && p[4].x == 0);
  base.z = 0xB00;
  assert(fifa96_action_sequence_scatter_celebration(&base, 1, 1, rng, p) == FIFA96_OK);
  assert(p[1].z == 0xB10);
  assert(p[4].z == 0xB10);
  assert(fifa96_action_sequence_scatter_celebration(NULL, 1, 1, rng, p) == SEQ_INVALID);
  assert(fifa96_action_sequence_scatter_celebration(&base, 1, 1, NULL, p) == SEQ_INVALID);
  assert(fifa96_action_sequence_scatter_celebration(&base, 1, 1, rng, NULL) == SEQ_INVALID);
}

static void test_sequence_scatter_stats(void) {
  static const uint32_t rng[7] = {0xF1u, 1, 2, 3, 4, 5, 6};
  fifa96_action_vec3 base;
  fifa96_action_vec3 p[5];
  base.x = 0x100;
  base.y = 7;
  base.z = 0x200;
  assert(fifa96_action_sequence_scatter_stats(&base, 1, -1, rng, p) == FIFA96_OK);
  assert(p[0].x == 0x100 && p[0].y == 7 && p[0].z == 0x200);
  assert(p[1].x == 1 && p[1].y == 7 && p[1].z == 0x1F);
  assert(p[2].x == 0x23 && p[2].y == 7 && p[2].z == -0x124);
  assert(p[3].x == 0x23 && p[3].y == 7 && p[3].z == -0x4E8);
  assert(p[4].x == 0x78 && p[4].y == 7 && p[4].z == -0x8AE);
  base.z = 0xB00;
  assert(fifa96_action_sequence_scatter_stats(&base, 1, 1, rng, p) == FIFA96_OK);
  assert(p[1].z == 0xB10);
  assert(p[3].z == 0xB10);
  assert(p[4].z == 0xB10);
  base.x = 0x700;
  assert(fifa96_action_sequence_scatter_stats(&base, 1, 1, rng, p) == FIFA96_OK);
  assert(p[1].x == 1);
  assert(p[2].x == 0x23);
  assert(p[3].x == 0x23);
  assert(p[4].x == 0x78);
  assert(fifa96_action_sequence_scatter_stats(NULL, 1, 1, rng, p) == SEQ_INVALID);
  assert(fifa96_action_sequence_scatter_stats(&base, 1, 1, NULL, p) == SEQ_INVALID);
  assert(fifa96_action_sequence_scatter_stats(&base, 1, 1, rng, NULL) == SEQ_INVALID);
}

static void test_sequence_duel_event(void) {
  fifa96_action_sequence_duel out;
  assert(fifa96_action_sequence_duel_event(0x21, 5, 0, 0, &out) == FIFA96_OK);
  assert(out.reset == 0 && out.event_id == 0x59);
  assert(fifa96_action_sequence_duel_event(0x20, 5, 0, 0, &out) == FIFA96_OK);
  assert(out.reset == 1);
  assert(fifa96_action_sequence_duel_event(0x6F, 5, 0, 0, &out) == FIFA96_OK);
  assert(out.reset == 0);
  assert(fifa96_action_sequence_duel_event(0x70, 5, 0, 0, &out) == FIFA96_OK);
  assert(out.reset == 1);
  assert(fifa96_action_sequence_duel_event(0x00, 5, 0, 0, &out) == FIFA96_OK);
  assert(out.reset == 1);
  assert(fifa96_action_sequence_duel_event(0x21, 4, 0, 0, &out) == FIFA96_OK);
  assert(out.reset == 0 && out.event_id == 0x0C);
  assert(fifa96_action_sequence_duel_event(0x21, 5, 0x1000, 0, &out) == FIFA96_OK);
  assert(out.event_id == 0x0C);
  assert(fifa96_action_sequence_duel_event(0x21, 5, 0x0FFF, 0, &out) == FIFA96_OK);
  assert(out.event_id == 0x59);
  assert(fifa96_action_sequence_duel_event(0x21, 5, -0x0FFF, 0, &out) == FIFA96_OK);
  assert(out.event_id == 0x59);
  assert(fifa96_action_sequence_duel_event(0x21, 5, -0x1000, 0, &out) == FIFA96_OK);
  assert(out.event_id == 0x0C);
  assert(fifa96_action_sequence_duel_event(0x21, 5, 0, 0, NULL) == SEQ_INVALID);
}

static void test_sequence_press_event(void) {
  fifa96_action_sequence_press out;
  assert(fifa96_action_sequence_press_event(0x30, 0, &out) == FIFA96_OK);
  assert(out.fire == 1 && out.reset == 0);
  assert(fifa96_action_sequence_press_event(0x31, 0x1E, &out) == FIFA96_OK);
  assert(out.fire == 0 && out.reset == 0);
  assert(fifa96_action_sequence_press_event(0x31, 0x1F, &out) == FIFA96_OK);
  assert(out.fire == 0 && out.reset == 1);
  assert(fifa96_action_sequence_press_event(-1, 0x7FFFFFFF, &out) == FIFA96_OK);
  assert(out.fire == 1 && out.reset == 0);
  assert(fifa96_action_sequence_press_event(0x7FFF, 0x1E, &out) == FIFA96_OK);
  assert(out.fire == 0 && out.reset == 0);
  assert(fifa96_action_sequence_press_event(0, 0, NULL) == SEQ_INVALID);
}

static void test_sequence_event_ids(void) {
  static const uint8_t t344[2] = {0x22u, 0x23u};
  static const uint8_t t346[3] = {0x31u, 0x32u, 0x33u};
  static const uint8_t t349[3] = {0x41u, 0x42u, 0x43u};
  static const uint8_t t34c[9] = {0, 1, 2, 3, 4, 0x5Bu, 6, 7, 8};
  static const uint32_t rng[7] = {1, 0, 5, 4, 0, 0, 0};
  uint8_t ids[5];
  assert(fifa96_action_sequence_event_ids(t344, t346, t349, t34c, rng, ids) == FIFA96_OK);
  assert(ids[0] == 0x23 && ids[1] == 0x23 && ids[2] == 0x43 && ids[3] == 4 && ids[4] == 4);
  {
    static const uint32_t rng67[7] = {0, 1, 0, 0, 0, 0, 0};
    static const uint8_t t67[2] = {0x67u, 0x67u};
    assert(fifa96_action_sequence_event_ids(t67, t346, t349, t34c, rng67, ids) == FIFA96_OK);
    assert(ids[0] == 0x67 && ids[1] == 0x67 && ids[2] == 0x67 && ids[3] == 0x67 &&
           ids[4] == 0x67);
  }
  {
    static const uint32_t rngset[7] = {1, 0, 0, 5, 0, 0, 4};
    assert(fifa96_action_sequence_event_ids(t344, t346, t349, t34c, rngset, ids) == FIFA96_OK);
    assert(ids[3] == 0x5B && ids[4] == 0x68);
  }
  {
    static const uint32_t rngovr[7] = {1, 0, 0, 0, 1, 5, 1};
    assert(fifa96_action_sequence_event_ids(t344, t346, t349, t34c, rngovr, ids) == FIFA96_OK);
    assert(ids[1] == 0x33);
    assert(ids[3] == 0 && ids[4] == 0);
  }
  assert(fifa96_action_sequence_event_ids(NULL, t346, t349, t34c, rng, ids) == SEQ_INVALID);
  assert(fifa96_action_sequence_event_ids(t344, NULL, t349, t34c, rng, ids) == SEQ_INVALID);
  assert(fifa96_action_sequence_event_ids(t344, t346, NULL, t34c, rng, ids) == SEQ_INVALID);
  assert(fifa96_action_sequence_event_ids(t344, t346, t349, NULL, rng, ids) == SEQ_INVALID);
  assert(fifa96_action_sequence_event_ids(t344, t346, t349, t34c, NULL, ids) == SEQ_INVALID);
  assert(fifa96_action_sequence_event_ids(t344, t346, t349, t34c, rng, NULL) == SEQ_INVALID);
}

int main(void) {
  test_sequence_select();
  test_sequence_event();
  test_sequence_marker();
  test_sequence_rng_event();
  test_sequence_countdown();
  test_sequence_anim_byte();
  test_sequence_lane();
  test_sequence_scatter_celebration();
  test_sequence_scatter_stats();
  test_sequence_duel_event();
  test_sequence_press_event();
  test_sequence_event_ids();
  puts("test_event_sequences: all assertions passed");
  return 0;
}
