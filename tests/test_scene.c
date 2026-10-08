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
_Static_assert(FIFA96_SCENE_FORMATION_RECORDS == 11, "formation records");
_Static_assert(FIFA96_SCENE_FORMATION_STRIDE == 4, "formation stride");
_Static_assert(FIFA96_SCENE_FORMATION_BYTES == 44, "formation size");
_Static_assert(FIFA96_SCENE_FORMATION_X_SCALE == 38, "placement x scale");
_Static_assert(FIFA96_SCENE_FORMATION_Z_SCALE == 33, "placement z scale");
_Static_assert(offsetof(fifa96_scene_formation, bytes) == 0, "formation bytes");
_Static_assert(offsetof(fifa96_scene_formation, loaded) == 44, "formation flag");

#define P_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static int32_t recip_x[FIFA96_PROJECTION_RECIP_COUNT];
static int32_t recip_y[FIFA96_PROJECTION_RECIP_COUNT];

/* First-hand 0x57798..0x577AC: the store is `[EAX+0x14E7AC]` after
 * `ADD EAX,4`, i.e. keys[k] = z(position[list[k]]) for k = 0..count-1; the
 * values are 1-based slot ids and 0 is the empty sentinel. */
static void test_build_keys(void) {
  const int32_t positions[12] = {1, 2, 10, 3, 4, 20, 5, 6, 30, 7, 8, 40};
  const uint32_t list[4] = {0, 3, 1, 2};
  int32_t keys[5];
  keys[4] = 0x7F;
  assert(fifa96_scene_build_keys(4, list, positions, 4, keys) == FIFA96_OK);
  assert(keys[4] == 0x7F);   /* the output domain is [0, count) */
  assert(keys[0] == 10 && keys[1] == 40 && keys[2] == 20 && keys[3] == 30);
  assert(fifa96_scene_build_keys(0, list, positions, 4, keys) == FIFA96_OK);
  assert(keys[0] == 10);     /* zero count writes nothing */
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

static void test_clip_edges(void) {
  int32_t y;
  uint8_t draw;
  assert(fifa96_scene_clip_edges(0, 0, 0, 100, 40, 100, 100, &y, &draw) == FIFA96_OK);
  assert(draw == 0 && y == 100);
  assert(fifa96_scene_clip_edges(0, 50, 0, 100, 40, 100, 50, &y, &draw) == FIFA96_OK);
  assert(draw == 0 && y == 100);
  assert(fifa96_scene_clip_edges(0, 50, 200, 100, 80, 100, 50, &y, &draw) == FIFA96_OK);
  assert(draw == 1 && y == 50);
  assert(fifa96_scene_clip_edges(0, 50, 200, 100, 80, 300, 50, &y, &draw) == FIFA96_OK);
  assert(draw == 0 && y == 100);
  assert(fifa96_scene_clip_edges(400, 50, 200, 100, 80, 250, 50, &y, &draw) == FIFA96_OK);
  assert(draw == 0 && y == 100);
  assert(fifa96_scene_clip_edges(400, 50, 200, 100, 80, 259, 50, &y, &draw) == FIFA96_OK);
  assert(draw == 0 && y == 100);
  assert(fifa96_scene_clip_edges(0, 0, 0, 0, 0, 0, 0, NULL, &draw) == P_INVALID);
  assert(fifa96_scene_clip_edges(0, 0, 0, 0, 0, 0, 0, &y, NULL) == P_INVALID);
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

/* ---- FU-89 §11 / OL-T11-8: formation load + placement ----------------------
 *
 * First-hand on /FIFA96.EXE this slice:
 *  - `FUN_0004A6BC` (`0x4A6BC..0x4A82C`) opens `art/gameart0.pvi` through the
 *    `0x1C64`/`0x1C6C` archive pair and resolves one name per table slot with
 *    the formats at `0x101C80` (`%s.fmt`, slots 0..29), `0x101C88` (`%s.dat`,
 *    30..38), `0x101C94` (`%s.%s` + `0x101C90` "lfsh", 39..55) and `0x101C9C`
 *    (`%s.qfs`, 56..62), with the names from the `0x107370` pointer table;
 *  - the four `x.fmt` files of each formation family are BIGF entries inside
 *    `art/gameart0.pvi` (first-hand BIGF directory: entry 0 `352ko.fmt`, 44 B;
 *    families 352/442/sw/424/433 and the shared `freekick.fmt` at entry 25);
 *  - `FUN_0006E1D0` (`0x6E1D0..0x6E241`) reads `[rec+8]` (+2 for the
 *    controlled side) and writes `x = (int8)byte[0] * 0x26`, `y = 0`,
 *    `z = (int8)byte[1] * 0x21`, negating both for team side != 0.
 * The 44 fixture bytes are the extracted `352ko.fmt` content. */

static void put_be32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v >> 24);
  p[1] = (uint8_t)(v >> 16);
  p[2] = (uint8_t)(v >> 8);
  p[3] = (uint8_t)v;
}

/* A raw BIGF directory with two entries: "skip.fmt" (4 bytes) and
 * "352ko.fmt" (payload_len bytes). Layout: "BIGF", BE32 total size, BE32
 * count, BE32 table_end, then {BE32 off, BE32 size, name NUL} records. */
static size_t build_formation_bigf(uint8_t *dst, const uint8_t *payload,
                                   size_t payload_len) {
  static const char name0[] = "skip.fmt";
  static const char name1[] = "352ko.fmt";
  size_t table_end = 0x10 + (8 + sizeof name0) + (8 + sizeof name1);
  size_t total = table_end + 4 + payload_len;
  memset(dst, 0, total);
  memcpy(dst, "BIGF", 4);
  put_be32(dst + 4, (uint32_t)total);
  put_be32(dst + 8, 2);
  put_be32(dst + 0x0c, (uint32_t)table_end);
  put_be32(dst + 0x10, (uint32_t)table_end);
  put_be32(dst + 0x14, 4);
  memcpy(dst + 0x18, name0, sizeof name0);
  size_t p = 0x18 + sizeof name0;
  put_be32(dst + p, (uint32_t)(table_end + 4));
  put_be32(dst + p + 4, (uint32_t)payload_len);
  memcpy(dst + p + 8, name1, sizeof name1);
  memset(dst + table_end, 0xA5, 4);
  memcpy(dst + table_end + 4, payload, payload_len);
  return total;
}

static const uint8_t formation_352ko[FIFA96_SCENE_FORMATION_BYTES] = {
  0x00, 0xB4, 0x00, 0xB8, 0xE8, 0xD8, 0xE8, 0xDA, 0x00, 0xD4, 0x00, 0xD8,
  0x18, 0xD8, 0x18, 0xDA, 0xDF, 0xFB, 0xE0, 0xFE, 0xEC, 0xF0, 0xED, 0xED,
  0xFF, 0xE6, 0xFF, 0xEB, 0x14, 0xF1, 0x13, 0xED, 0x20, 0xFC, 0x21, 0xFE,
  0xFA, 0xF8, 0xFD, 0xFE, 0x06, 0xF8, 0x02, 0xFE,
};

static void test_formation_load(void) {
  uint8_t container[0x400];
  fifa96_scene_formation f;
  memset(&f, 0x5A, sizeof f);
  size_t len = build_formation_bigf(container, formation_352ko,
                                    sizeof formation_352ko);
  assert(fifa96_scene_formation_load(container, len, "352ko.fmt", &f) == FIFA96_OK);
  assert(f.loaded == 1);
  assert(memcmp(f.bytes, formation_352ko, sizeof formation_352ko) == 0);

  /* the name search skips earlier entries; a miss is NOT_FOUND */
  assert(fifa96_scene_formation_load(container, len, "442ko.fmt", &f) ==
         FIFA96_ERR_NOT_FOUND);
  assert(f.loaded == 1);   /* untouched on failure */
  assert(fifa96_scene_formation_load(container, len, "skip.fmt", &f) ==
         FIFA96_ERR_TRUNCATED);   /* 4-byte payload, not a formation */

  /* a non-container form is UNSUPPORTED (raw non-BIGF and mislabelled alike) */
  assert(fifa96_scene_formation_load((const uint8_t *)"ABCDEFGH", 8, "352ko.fmt",
                                     &f) == FIFA96_ERR_UNSUPPORTED);
  uint8_t bad[0x400];
  memcpy(bad, container, len);
  memcpy(bad, "BIGX", 4);
  assert(fifa96_scene_formation_load(bad, len, "352ko.fmt", &f) ==
         FIFA96_ERR_UNSUPPORTED);

  assert(fifa96_scene_formation_load(NULL, len, "352ko.fmt", &f) == P_INVALID);
  assert(fifa96_scene_formation_load(container, len, NULL, &f) == P_INVALID);
  assert(fifa96_scene_formation_load(container, len, "352ko.fmt", NULL) == P_INVALID);
}

static void test_formation_place(void) {
  fifa96_scene_formation f;
  int32_t x = 0, y = 0, z = 0;
  memset(&f, 0, sizeof f);
  assert(fifa96_scene_formation_place(&f, 0, 0, 0, &x, &y, &z) ==
         FIFA96_ERR_NOT_FOUND);
  memcpy(f.bytes, formation_352ko, sizeof formation_352ko);
  f.loaded = 1;

  /* record 0: own (0xB8 = -72 -> z = -2376), opp (0xB4 = -76 -> +2508) */
  assert(fifa96_scene_formation_place(&f, 0, 0, 0, &x, &y, &z) == FIFA96_OK);
  assert(x == 0 && y == 0 && z == -2376);
  assert(fifa96_scene_formation_place(&f, 0, 1, 0, &x, &y, &z) == FIFA96_OK);
  assert(x == 0 && y == 0 && z == 2508);
  /* controlled side 1: the pair selection flips but the side sign does not */
  assert(fifa96_scene_formation_place(&f, 0, 0, 1, &x, &y, &z) == FIFA96_OK);
  assert(z == -2508);
  assert(fifa96_scene_formation_place(&f, 0, 1, 1, &x, &y, &z) == FIFA96_OK);
  assert(z == 2376);
  /* record 8: own (0x21, 0xFE) -> (1254, -66); opp (0x20, 0xFC) -> (-1216, 132) */
  assert(fifa96_scene_formation_place(&f, 8, 0, 0, &x, &y, &z) == FIFA96_OK);
  assert(x == 1254 && y == 0 && z == -66);
  assert(fifa96_scene_formation_place(&f, 8, 1, 0, &x, &y, &z) == FIFA96_OK);
  assert(x == -1216 && y == 0 && z == 132);
  /* record 10: own (0x02, 0xFE) -> (76, -66); opp (0x06, 0xF8) -> (-228, 264) */
  assert(fifa96_scene_formation_place(&f, 10, 0, 0, &x, &y, &z) == FIFA96_OK);
  assert(x == 76 && z == -66);
  assert(fifa96_scene_formation_place(&f, 10, 1, 0, &x, &y, &z) == FIFA96_OK);
  assert(x == -228 && z == 264);

  /* sign/width: 0x80 is -128; the products stay in the 16-bit target words.
   * The own pair is +2, so side 1 reads it only when it is the controlled
   * side; the negation then applies to the same pair. */
  f.bytes[2] = 0x80;
  f.bytes[3] = 0x7F;
  assert(fifa96_scene_formation_place(&f, 0, 0, 0, &x, &y, &z) == FIFA96_OK);
  assert(x == -4864 && z == 4191);
  assert(fifa96_scene_formation_place(&f, 0, 1, 1, &x, &y, &z) == FIFA96_OK);
  assert(x == 4864 && z == -4191);

  assert(fifa96_scene_formation_place(&f, 11, 0, 0, &x, &y, &z) == P_INVALID);
  assert(fifa96_scene_formation_place(NULL, 0, 0, 0, &x, &y, &z) == P_INVALID);
  assert(fifa96_scene_formation_place(&f, 0, 0, 0, NULL, &y, &z) == P_INVALID);
  assert(fifa96_scene_formation_place(&f, 0, 0, 0, &x, NULL, &z) == P_INVALID);
  assert(fifa96_scene_formation_place(&f, 0, 0, 0, &x, &y, NULL) == P_INVALID);
}

int main(void) {
  test_build_keys();
  test_sort();
  test_slot_gate();
  test_clip_edges();
  test_threshold();
  test_reproject();
  test_slot_project();
  test_formation_load();
  test_formation_place();
  puts("test_scene: all assertions passed");
  return 0;
}
