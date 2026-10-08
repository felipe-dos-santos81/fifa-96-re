// tests/test_action_handlers.c — FU-76 action-handler movement/angle/kick math
// (docs/ghidra/FU76_action_handlers.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_action_handlers.h"
#include "fifa96_loader/fifa96_entity_update.h"
#include "fifa96_loader/fifa96_rng.h"

_Static_assert(offsetof(fifa96_action_vec3, x) == 0, "x");
_Static_assert(offsetof(fifa96_action_vec3, y) == 4, "y");
_Static_assert(offsetof(fifa96_action_vec3, z) == 8, "z");
_Static_assert(offsetof(fifa96_action_move_state, timer89) == 0, "timer89");
_Static_assert(offsetof(fifa96_action_move_state, timer81) == 4, "timer81");
_Static_assert(offsetof(fifa96_action_move_state, delta) == 6, "delta");
_Static_assert(offsetof(fifa96_action_move_state, phase) == 8, "phase");
_Static_assert(offsetof(fifa96_action_move_state, active) == 9, "active");
_Static_assert(offsetof(fifa96_action_move_state, has_slot) == 10, "has_slot");
_Static_assert(offsetof(fifa96_action_move_state, dir_x) == 11, "dir_x");
_Static_assert(offsetof(fifa96_action_move_state, dir_z) == 12, "dir_z");
_Static_assert(offsetof(fifa96_action_move_out, move) == 0, "move");
_Static_assert(offsetof(fifa96_action_move_out, install) == 1, "install");
_Static_assert(offsetof(fifa96_action_move_out, code) == 2, "code");
_Static_assert(offsetof(fifa96_action_kick_row, lo) == 0, "lo");
_Static_assert(offsetof(fifa96_action_kick_row, hi) == 2, "hi");
_Static_assert(offsetof(fifa96_action_kick_row, traj_add) == 4, "traj_add");
_Static_assert(offsetof(fifa96_action_kick_ball, x) == 0, "x");
_Static_assert(offsetof(fifa96_action_kick_ball, z) == 2, "z");
_Static_assert(offsetof(fifa96_action_kick_ball, traj) == 4, "traj");
_Static_assert(offsetof(fifa96_action_kick_ball, comp_z) == 6, "comp_z");
_Static_assert(offsetof(fifa96_action_kick_event, code) == 0, "code");
_Static_assert(offsetof(fifa96_action_kick_event, subtype) == 1, "subtype");
_Static_assert(offsetof(fifa96_action_kick_event, sector_byte) == 2, "sector_byte");
_Static_assert(offsetof(fifa96_action_kick_event, active) == 3, "active");
_Static_assert(offsetof(fifa96_action_kick_event, has_slot) == 4, "has_slot");
_Static_assert(offsetof(fifa96_action_kick_event, slot_counter) == 5, "slot_counter");
_Static_assert(offsetof(fifa96_action_kick_event, phase) == 6, "phase");
_Static_assert(offsetof(fifa96_action_kick_event, x) == 8, "x");
_Static_assert(offsetof(fifa96_action_kick_event, z) == 10, "z");
_Static_assert(offsetof(fifa96_action_kick_event, ball_height) == 12, "ball_height");
_Static_assert(offsetof(fifa96_action_kick_event, height) == 16, "height");
_Static_assert(offsetof(fifa96_action_kick_event_out, found) == 0, "found");
_Static_assert(offsetof(fifa96_action_kick_event_out, table) == 1, "table");
_Static_assert(offsetof(fifa96_action_kick_event_out, index) == 4, "index");
_Static_assert(offsetof(fifa96_action_kick_append, direct) == 0, "direct");
_Static_assert(offsetof(fifa96_action_kick_append, code) == 1, "code");
_Static_assert(offsetof(fifa96_action_score, score) == 0, "score");
_Static_assert(offsetof(fifa96_action_score, last_side) == 4, "last_side");
_Static_assert(offsetof(fifa96_action_score, tracked_side) == 8, "tracked_side");
_Static_assert(offsetof(fifa96_action_score, max_diff) == 12, "max_diff");
_Static_assert(offsetof(fifa96_action_score_out, posted) == 0, "posted");
_Static_assert(offsetof(fifa96_action_score_out, post_id) == 1, "post_id");

#define ACTION_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static void test_move_target(void) {
  fifa96_action_vec3 out;
  memset(&out, 0xAB, sizeof(out));
  assert(fifa96_action_move_target(0x100, 0x200, 0, 0, &out) == FIFA96_OK);
  assert(out.x == 0x100 && out.y == 0 && out.z == 0x200);
  assert(fifa96_action_move_target(0, 0, 1, 2, &out) == FIFA96_OK);
  assert(out.x == 128 && out.y == 0 && out.z == 256);
  assert(fifa96_action_move_target(0, 0, -1, -1, &out) == FIFA96_OK);
  assert(out.x == -128 && out.z == -128);
  assert(fifa96_action_move_target(0x700, 0xB00, 2, 2, &out) == FIFA96_OK);
  assert(out.x == 0x720 && out.z == 0xB10);
  assert(fifa96_action_move_target(-0x700, -0xB00, -2, -2, &out) == FIFA96_OK);
  assert(out.x == -0x720 && out.z == -0xB10);
  assert(fifa96_action_move_target(0x710, 0, 127, 0, &out) == FIFA96_OK);
  assert(out.x == 0x720);
  assert(fifa96_action_move_target(0, 0xB0F, 0, 127, &out) == FIFA96_OK);
  assert(out.z == 0xB10);
  assert(fifa96_action_move_target(0, 0, 0, 0, NULL) == ACTION_INVALID);
}

static fifa96_action_move_state move_state(int32_t timer89, uint16_t timer81, uint8_t phase,
                                           uint8_t active, uint8_t has_slot) {
  fifa96_action_move_state s;
  s.timer89 = timer89;
  s.timer81 = timer81;
  s.delta = 10;
  s.phase = phase;
  s.active = active;
  s.has_slot = has_slot;
  s.dir_x = 1;
  s.dir_z = 2;
  return s;
}

static void test_move_step(void) {
  fifa96_action_move_state s;
  fifa96_action_move_out out;
  s = move_state(100, 0, 2, 0, 0);
  assert(fifa96_action_move_step(&s, &out) == FIFA96_OK);
  assert(s.timer89 == 90 && out.move == 0 && out.install == 0);
  s = move_state(5, 0, 2, 0, 0);
  assert(fifa96_action_move_step(&s, &out) == FIFA96_OK);
  assert(s.timer89 == -5 && out.install == 1 && out.code == 0x19);
  s = move_state(0, 0, 2, 1, 1);
  assert(fifa96_action_move_step(&s, &out) == FIFA96_OK);
  assert(out.move == 1 && out.install == 1 && out.code == 3);
  s = move_state(0, 0, 2, 0x80, 1);
  assert(fifa96_action_move_step(&s, &out) == FIFA96_OK);
  assert(out.code == 3);
  s = move_state(0, 0, 2, 0, 0);
  assert(fifa96_action_move_step(&s, &out) == FIFA96_OK);
  assert(out.move == 0 && out.install == 1 && out.code == 0x19);
  s = move_state(0, 1, 2, 1, 1);
  assert(fifa96_action_move_step(&s, &out) == FIFA96_OK);
  assert(out.install == 0);
  s = move_state(100, 0, 2, 1, 1);
  assert(fifa96_action_move_step(&s, &out) == FIFA96_OK);
  assert(out.install == 0);
  s = move_state(0, 0, 6, 1, 1);
  assert(fifa96_action_move_step(&s, &out) == FIFA96_OK);
  assert(out.move == 0 && out.install == 0);
  s = move_state(0, 0, 1, 1, 1);
  assert(fifa96_action_move_step(&s, &out) == FIFA96_OK);
  assert(out.move == 1 && out.install == 0);
  s = move_state(0, 0, 2, 1, 1);
  s.delta = 0;
  assert(fifa96_action_move_step(&s, &out) == FIFA96_OK);
  assert(s.timer89 == 0 && out.install == 1);
  assert(fifa96_action_move_step(NULL, &out) == ACTION_INVALID);
  s = move_state(0, 0, 2, 1, 1);
  assert(fifa96_action_move_step(&s, NULL) == ACTION_INVALID);
}

static void test_kick_angle(void) {
  int32_t a;
  assert(fifa96_action_kick_angle(0, 0, &a) == FIFA96_OK && a == 0x80);
  assert(fifa96_action_kick_angle(1, 0, &a) == FIFA96_OK && a == 0x100);
  assert(fifa96_action_kick_angle(0, 1, &a) == FIFA96_OK && a == 0);
  assert(fifa96_action_kick_angle(1, 1, &a) == FIFA96_OK && a == 0x80);
  assert(fifa96_action_kick_angle(-1, 0, &a) == FIFA96_OK && a == -0x100);
  assert(fifa96_action_kick_angle(0, -1, &a) == FIFA96_OK && a == 0x200);
  assert(fifa96_action_kick_angle(1, -1, &a) == FIFA96_OK && a == 0x180);
  assert(fifa96_action_kick_angle(-1, -1, &a) == FIFA96_OK && a == -0x180);
  assert(fifa96_action_kick_angle(2, 1, &a) == FIFA96_OK && a == 0xB4);
  assert(fifa96_action_kick_angle(1, 2, &a) == FIFA96_OK && a == 0x4C);
  assert(fifa96_action_kick_angle(3, 5, &a) == FIFA96_OK && a == 0x58);
  assert(fifa96_action_kick_angle(5, 3, &a) == FIFA96_OK && a == 0xA8);
  assert(fifa96_action_kick_angle(257, 256, &a) == FIFA96_OK && a == 0x80);
  assert(fifa96_action_kick_angle(256, 257, &a) == FIFA96_OK && a == 0x80);
  assert(fifa96_action_kick_angle(0xFFFFFF, 0x1000000, &a) == FIFA96_OK && a == 0x80);
  assert(fifa96_action_kick_angle(1, 0, NULL) == ACTION_INVALID);
}

static fifa96_action_kick_ball kick_ball(int16_t x, int16_t traj, int16_t comp_z) {
  fifa96_action_kick_ball b;
  b.x = x;
  b.z = 0;
  b.traj = traj;
  b.comp_z = comp_z;
  return b;
}

static void test_kick_apply(void) {
  fifa96_action_kick_row row;
  fifa96_action_kick_ball b;
  row.lo = 0x100;
  row.hi = 0x300;
  row.traj_add = 0x20;
  b = kick_ball(0x200, 0x100, 0x100);
  assert(fifa96_action_kick_apply(&b, &row, 0, 0, 0) == FIFA96_OK);
  assert(b.x == 0x200 && b.traj == 0x120);
  b = kick_ball(0x400, 0, 0);
  assert(fifa96_action_kick_apply(&b, &row, 0, 0, 0) == FIFA96_OK);
  assert(b.x == 0x300);
  b = kick_ball(0x50, 0, 0);
  assert(fifa96_action_kick_apply(&b, &row, 0, 0, 0) == FIFA96_OK);
  assert(b.x == 0x100);
  b = kick_ball(0x300, 0, 0);
  assert(fifa96_action_kick_apply(&b, &row, 0, 0, 0) == FIFA96_OK);
  assert(b.x == 0x300);
  b = kick_ball(0x400, 0, 0);
  assert(fifa96_action_kick_apply(&b, &row, 0, 0, 1) == FIFA96_OK);
  assert(b.x == 0x300);
  b = kick_ball(0x100, 0, 0);
  assert(fifa96_action_kick_apply(&b, &row, 0, 1, 1) == FIFA96_OK);
  assert(b.x == 0x180);
  b = kick_ball(0x500, 0, 0);
  assert(fifa96_action_kick_apply(&b, &row, 0, 3, 1) == FIFA96_OK);
  assert(b.x == 0x480);
  b = kick_ball(0x400, 0, 0);
  assert(fifa96_action_kick_apply(&b, &row, 0, 1, 1) == FIFA96_OK);
  assert(b.x == 0x400);
  b = kick_ball(0x400, 0, 0);
  assert(fifa96_action_kick_apply(&b, &row, 0, 2, 1) == FIFA96_OK);
  assert(b.x == 0x300);
  b = kick_ball(0x700, 0, 0);
  assert(fifa96_action_kick_apply(&b, &row, 0x30, 0, 0) == FIFA96_OK);
  assert(b.x == 0x700 && b.traj == 0x30);
  b = kick_ball(0x100, 0, 0);
  assert(fifa96_action_kick_apply(&b, &row, 0x30, 0, 0) == FIFA96_OK);
  assert(b.x == 0x1C8);
  b = kick_ball(0, 0xFFF0, 0);
  assert(fifa96_action_kick_apply(&b, &row, 0, 0, 0) == FIFA96_OK);
  assert(b.traj == 0x0010);
  b = kick_ball(0, 0x100, 0x461);
  assert(fifa96_action_kick_apply(&b, &row, 0, 0, 0) == FIFA96_OK);
  assert(b.traj == 0x460);
  b = kick_ball(0, 0x100, 0x460);
  assert(fifa96_action_kick_apply(&b, &row, 0, 0, 0) == FIFA96_OK);
  assert(b.traj == 0x120);
  b = kick_ball(0x200, 0, 0);
  assert(fifa96_action_kick_apply(NULL, &row, 0, 0, 0) == ACTION_INVALID);
  assert(fifa96_action_kick_apply(&b, NULL, 0, 0, 0) == ACTION_INVALID);
}

// FU-138 action cluster A rows 01/02/03/0D derived helpers.
static void test_stage_wait(void) {
  uint8_t ready;
  assert(fifa96_action_stage_wait(0x3B, 0x3C, &ready) == FIFA96_OK && ready == 0);
  assert(fifa96_action_stage_wait(0x3C, 0x3C, &ready) == FIFA96_OK && ready == 1);
  assert(fifa96_action_stage_wait(0x78, 0x78, &ready) == FIFA96_OK && ready == 1);
  assert(fifa96_action_stage_wait(-1, 0, &ready) == FIFA96_OK && ready == 0);
  assert(fifa96_action_stage_wait(0, -1, &ready) == FIFA96_OK && ready == 1);
  assert(fifa96_action_stage_wait(0, 0, NULL) == ACTION_INVALID);
}

static void test_sequence_marker_target(void) {
  fifa96_action_vec3 pos, out;
  pos.x = 0x100;
  pos.y = 0x50;
  pos.z = -0x200;
  memset(&out, 0xAB, sizeof(out));
  assert(fifa96_action_sequence_marker_target(2, &pos, 0x77, &out) == FIFA96_OK);
  assert(out.x == 0x100 && out.y == 0x50 && out.z == -0x200);
  assert(fifa96_action_sequence_marker_target(0xFF, &pos, -5, &out) == FIFA96_OK);
  assert(out.x == 0x100 && out.y == 0x50 && out.z == -0x200);
  out.y = 0x5A;
  assert(fifa96_action_sequence_marker_target(1, &pos, 5, &out) == FIFA96_OK);
  assert(out.x == 0x30 && out.y == 0x5A && out.z == 0);
  out.y = 0x5A;
  assert(fifa96_action_sequence_marker_target(0, &pos, -1, &out) == FIFA96_OK);
  assert(out.x == -0x30 && out.y == 0x5A && out.z == 0);
  assert(fifa96_action_sequence_marker_target(1, &pos, 0, &out) == FIFA96_OK);
  assert(out.x == 0x30);
  assert(fifa96_action_sequence_marker_target(1, NULL, 0, &out) == ACTION_INVALID);
  assert(fifa96_action_sequence_marker_target(1, &pos, 0, NULL) == ACTION_INVALID);
}

static void test_locomotion_restart_wait(void) {
  uint8_t ready, reset;
  assert(fifa96_action_locomotion_restart_wait(0x40, 0x9, &ready, &reset) == FIFA96_OK);
  assert(ready == 0 && reset == 0);
  assert(fifa96_action_locomotion_restart_wait(0x40, 0xA, &ready, &reset) == FIFA96_OK);
  assert(ready == 1 && reset == 0);
  assert(fifa96_action_locomotion_restart_wait(0x41, 0x77, &ready, &reset) == FIFA96_OK);
  assert(ready == 0 && reset == 0);
  assert(fifa96_action_locomotion_restart_wait(0x41, 0x78, &ready, &reset) == FIFA96_OK);
  assert(ready == 1 && reset == 1);
  assert(fifa96_action_locomotion_restart_wait(-1, 0xA, &ready, &reset) == FIFA96_OK);
  assert(ready == 1 && reset == 0);
  assert(fifa96_action_locomotion_restart_wait(0x40, 0, NULL, &reset) == ACTION_INVALID);
  assert(fifa96_action_locomotion_restart_wait(0x40, 0, &ready, NULL) == ACTION_INVALID);
}

static void test_locomotion_placement_counter(void) {
  uint16_t counter;
  assert(fifa96_action_locomotion_placement_counter(0, 5, &counter) == FIFA96_OK);
  assert(counter == 1);
  assert(fifa96_action_locomotion_placement_counter(4 << 22, 3, &counter) == FIFA96_OK);
  assert(counter == 3);
  assert(fifa96_action_locomotion_placement_counter(0xFF << 22, 0xFFFF, &counter) == FIFA96_OK);
  assert(counter == 0x100);
  assert(fifa96_action_locomotion_placement_counter(-1, 5, &counter) == FIFA96_OK);
  assert(counter == 0);
  assert(fifa96_action_locomotion_placement_counter(0, 0, &counter) == FIFA96_OK);
  assert(counter == 0);
  assert(fifa96_action_locomotion_placement_counter(0, 5, NULL) == ACTION_INVALID);
}

static void test_locomotion_phase1_clamp(void) {
  int32_t z;
  z = -0x20;
  assert(fifa96_action_locomotion_phase1_clamp(0, &z) == FIFA96_OK && z == -0x20);
  z = -0x21;
  assert(fifa96_action_locomotion_phase1_clamp(0, &z) == FIFA96_OK && z == -0x21);
  z = 0;
  assert(fifa96_action_locomotion_phase1_clamp(0, &z) == FIFA96_OK && z == -0x20);
  z = 0x100;
  assert(fifa96_action_locomotion_phase1_clamp(0, &z) == FIFA96_OK && z == -0x20);
  z = 0x20;
  assert(fifa96_action_locomotion_phase1_clamp(1, &z) == FIFA96_OK && z == 0x20);
  z = 0x21;
  assert(fifa96_action_locomotion_phase1_clamp(1, &z) == FIFA96_OK && z == 0x21);
  z = 0;
  assert(fifa96_action_locomotion_phase1_clamp(1, &z) == FIFA96_OK && z == 0x20);
  z = -0x100;
  assert(fifa96_action_locomotion_phase1_clamp(1, &z) == FIFA96_OK && z == 0x20);
  assert(fifa96_action_locomotion_phase1_clamp(0, NULL) == ACTION_INVALID);
}

static void test_sequence_velocity_scale(void) {
  int16_t vx, vz, speed;
  assert(fifa96_action_sequence_velocity_scale(0, 0, &vx, &vz, &speed) == FIFA96_OK);
  assert(vx == 0 && vz == 0 && speed == 0);
  assert(fifa96_action_sequence_velocity_scale(0x10, 0, &vx, &vz, &speed) == FIFA96_OK);
  assert(vx == 0x40 && vz == 0 && speed == 0x40);
  assert(fifa96_action_sequence_velocity_scale(0x10, 0x20, &vx, &vz, &speed) == FIFA96_OK);
  assert(vx == 0x30 && vz == 0x60 && speed == 0x6C);
  assert(fifa96_action_sequence_velocity_scale(-1, 0x10, &vx, &vz, &speed) == FIFA96_OK);
  assert(vx == -3 && vz == 0x30 && speed == 0x30);
  assert(fifa96_action_sequence_velocity_scale(-1, 0, &vx, &vz, &speed) == FIFA96_OK);
  assert(vx == -4 && vz == 0 && speed == 4);
  assert(fifa96_action_sequence_velocity_scale(0x4000, 0x4000, &vx, &vz, &speed) == FIFA96_OK);
  assert(vx == (int16_t)0xC000 && vz == (int16_t)0xC000 && speed == 0x5800);
  assert(fifa96_action_sequence_velocity_scale(1, 1, NULL, &vz, &speed) == ACTION_INVALID);
  assert(fifa96_action_sequence_velocity_scale(1, 1, &vx, NULL, &speed) == ACTION_INVALID);
  assert(fifa96_action_sequence_velocity_scale(1, 1, &vx, &vz, NULL) == ACTION_INVALID);
}

/* FU-139 §3.4: the FUN_0007B9C4 negative-mode range band (native
 * 0x7BBE4..0x7BC15): an already non-negative mode is returned unchanged, a
 * negative one becomes 0x20 below 0x5A0, 0x30 below 0x780, else 0x10. */
static void test_kick_range_band(void) {
  uint8_t band;
  assert(fifa96_action_kick_range_band(0x30, 0x100, &band) == FIFA96_OK);
  assert(band == 0x30);
  assert(fifa96_action_kick_range_band(0, 0x100, &band) == FIFA96_OK);
  assert(band == 0);
  assert(fifa96_action_kick_range_band((int8_t)0x80, 0x59F, &band) == FIFA96_OK);
  assert(band == 0x20);
  assert(fifa96_action_kick_range_band((int8_t)0x80, 0x5A0, &band) == FIFA96_OK);
  assert(band == 0x30);
  assert(fifa96_action_kick_range_band((int8_t)0x80, 0x77F, &band) == FIFA96_OK);
  assert(band == 0x30);
  assert(fifa96_action_kick_range_band((int8_t)0x80, 0x780, &band) == FIFA96_OK);
  assert(band == 0x10);
  assert(fifa96_action_kick_range_band((int8_t)0xFF, -1, &band) == FIFA96_OK);
  assert(band == 0x20);
  assert(fifa96_action_kick_range_band((int8_t)0x80, 0x100, NULL) == ACTION_INVALID);
}

/* FU-139 §3.5: the FUN_0007AE70 event-row resolver (native 0x7AE79..0x7B018).
 * Each fixture names the branch it pins in the comment. */
static const uint8_t sector_bits[8] = {0x38, 0x70, 0xE0, 0xC1,
                                       0x83, 0x07, 0x0E, 0x1C};

static fifa96_action_kick_event kick_event(void) {
  fifa96_action_kick_event e;
  memset(&e, 0, sizeof e);
  return e;
}

static void test_kick_event_row_height_table(void) {
  fifa96_action_kick_event e = kick_event();
  fifa96_action_kick_event_out row;
  e.code = 0x40; /* class 2, idx 0 */
  e.height = 0x1000;
  e.ball_height = 0x1050; /* d = 0x50 >= 0x38 */
  assert(fifa96_action_kick_event_row(&e, sector_bits, &row) == FIFA96_OK);
  assert(row.found == 1);
  assert(row.table == FIFA96_ACTION_KICK_EVENT_TABLE_HEIGHT);
  assert(row.index == 4); /* 2*class + idx = 4 */
}

static void test_kick_event_row_carry_fast_path(void) {
  fifa96_action_kick_event e = kick_event();
  fifa96_action_kick_event_out row;
  e.code = 0; /* class 1 */
  e.has_slot = 1;
  e.active = 1;
  e.phase = 2;
  e.slot_counter = 3; /* < 7 */
  e.sector_byte = 0;
  assert(fifa96_action_kick_event_row(&e, sector_bits, &row) == FIFA96_OK);
  assert(row.found == 1);
  assert(row.table == FIFA96_ACTION_KICK_EVENT_TABLE_CARRY);
  assert(row.index == 0); /* idx from sector_bits[0]=0x38, sector 0 */
  e.slot_counter = 7; /* fast path closed, counter must be < 7 */
  e.ball_height = 0x50; /* band 1 */
  assert(fifa96_action_kick_event_row(&e, sector_bits, &row) == FIFA96_OK);
  assert(row.found == 1);
  assert(row.table == FIFA96_ACTION_KICK_EVENT_TABLE_ACTIVE);
  assert(row.index == 8); /* band 1: 2*1 + 6*1 + 0 */
}

static void test_kick_event_row_code60(void) {
  fifa96_action_kick_event e = kick_event();
  fifa96_action_kick_event_out row;
  e.code = 0x60; /* class 1 */
  e.has_slot = 1;
  e.active = 1;
  e.phase = 2;
  e.slot_counter = 7; /* fast path closed */
  e.sector_byte = 0;
  assert(fifa96_action_kick_event_row(&e, sector_bits, &row) == FIFA96_OK);
  assert(row.found == 1);
  assert(row.table == FIFA96_ACTION_KICK_EVENT_TABLE_CARRY);
  assert(row.index == 2); /* idx + 2 */
}

static void test_kick_event_row_band_and_state(void) {
  fifa96_action_kick_event e = kick_event();
  fifa96_action_kick_event_out row;
  e.code = 0x10; /* class 0 */
  e.ball_height = 0x80; /* d = 0x80 -> band 2 */
  e.sector_byte = 0;
  assert(fifa96_action_kick_event_row(&e, sector_bits, &row) == FIFA96_OK);
  assert(row.found == 1);
  assert(row.table == FIFA96_ACTION_KICK_EVENT_TABLE_IDLE);
  assert(row.index == 4); /* 2*2 + 6*0 + 0 */
  e.active = 1;
  assert(fifa96_action_kick_event_row(&e, sector_bits, &row) == FIFA96_OK);
  assert(row.table == FIFA96_ACTION_KICK_EVENT_TABLE_ACTIVE);
  e.ball_height = 0x50; /* band 1 */
  assert(fifa96_action_kick_event_row(&e, sector_bits, &row) == FIFA96_OK);
  assert(row.index == 2);
  e.ball_height = 0x10; /* band 0 */
  assert(fifa96_action_kick_event_row(&e, sector_bits, &row) == FIFA96_OK);
  assert(row.index == 0);
}

static void test_kick_event_row_angle_sector(void) {
  fifa96_action_kick_event e = kick_event();
  fifa96_action_kick_event_out row;
  static const uint8_t sector_one[1] = {0x04};
  e.code = 0; /* class 1 */
  e.active = 1;
  e.x = 1;
  e.z = 0; /* angle 0x100 -> sector 2 */
  assert(fifa96_action_kick_event_row(&e, sector_one, &row) == FIFA96_OK);
  assert(row.found == 1);
  assert(row.table == FIFA96_ACTION_KICK_EVENT_TABLE_ACTIVE);
  assert(row.index == 7); /* band 0: 0 + 6*1 + idx(1) */
}

/* FU-139 §7: the fallback sector is the sign-extended actor[+0x8E] byte and
 * the native `SAR EAX,CL` masks the shift count to 5 bits, so sector 0xE1
 * (-31) shifts by 1, not out of range. */
static void test_kick_event_row_sector_mask(void) {
  fifa96_action_kick_event e = kick_event();
  fifa96_action_kick_event_out row;
  static const uint8_t sector_shift[1] = {0x02};
  e.code = 0x10; /* class 0 */
  e.sector_byte = 0xE1;
  e.height = 0x1000;
  e.ball_height = 0x1050; /* height table, d >= 0x38 */
  assert(fifa96_action_kick_event_row(&e, sector_shift, &row) == FIFA96_OK);
  assert(row.found == 1);
  assert(row.table == FIFA96_ACTION_KICK_EVENT_TABLE_HEIGHT);
  assert(row.index == 1); /* 2*class(0) + ((0x02 >> (0xE1 & 0x1F)) & 1) */
}

static void test_kick_event_row_none_and_invalid(void) {
  fifa96_action_kick_event e = kick_event();
  fifa96_action_kick_event_out row;
  e.code = 0x40;
  e.height = 0x1000;
  e.ball_height = 0x10; /* d = -0xFF0 < 0x38 while height != 0 */
  assert(fifa96_action_kick_event_row(&e, sector_bits, &row) == FIFA96_OK);
  assert(row.found == 0);
  assert(fifa96_action_kick_event_row(NULL, sector_bits, &row) == ACTION_INVALID);
  assert(fifa96_action_kick_event_row(&e, NULL, &row) == ACTION_INVALID);
  assert(fifa96_action_kick_event_row(&e, sector_bits, NULL) == ACTION_INVALID);
}

/* FU-139 §3.6: the FUN_0007AE70 tail append selector (native
 * 0x7B01A..0x7B09F, jump table flat 0x7AE38). */
static void test_kick_event_append(void) {
  fifa96_action_kick_append out;
  /* actor action prefix table takes precedence over row[0] */
  assert(fifa96_action_kick_event_append(0x11, 0xFF, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 2);
  assert(fifa96_action_kick_event_append(0x12, 0xFF, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 7);
  assert(fifa96_action_kick_event_append(0x13, 0xFF, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 8);
  assert(fifa96_action_kick_event_append(0x20, 0xFF, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 8);
  assert(fifa96_action_kick_event_append(1, 0xFF, &out) == FIFA96_OK);
  assert(out.direct == 1 && out.code == 1);
  assert(fifa96_action_kick_event_append(0x10, 0xFF, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 3);
  /* fallback: row[0]-1 through flat 0x7AE38 */
  assert(fifa96_action_kick_event_append(2, 0x02, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 0x0F);
  assert(fifa96_action_kick_event_append(0, 0x01, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 0x0D);
  assert(fifa96_action_kick_event_append(0, 0x03, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 0x11);
  assert(fifa96_action_kick_event_append(0, 0x04, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 0x0A);
  assert(fifa96_action_kick_event_append(0, 0x05, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 0x09);
  assert(fifa96_action_kick_event_append(0, 0x06, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 0x14);
  assert(fifa96_action_kick_event_append(0, 0x07, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 0x13);
  assert(fifa96_action_kick_event_append(0, 0x08, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 0x16);
  assert(fifa96_action_kick_event_append(0, 0x09, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 0x16);
  assert(fifa96_action_kick_event_append(0, 0x0A, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 3);
  assert(fifa96_action_kick_event_append(0, 0x0B, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 4);
  assert(fifa96_action_kick_event_append(0, 0x0C, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 4);
  assert(fifa96_action_kick_event_append(0, 0x0D, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 0x10);
  assert(fifa96_action_kick_event_append(0, 0x0E, &out) == FIFA96_OK);
  assert(out.direct == 0 && out.code == 0x0C);
  assert(fifa96_action_kick_event_append(0, 0, &out) == FIFA96_OK);
  assert(out.direct == 1 && out.code == 0);
  assert(fifa96_action_kick_event_append(0, 0x0F, &out) == FIFA96_OK);
  assert(out.direct == 1 && out.code == 0);
  assert(fifa96_action_kick_event_append(0, 1, NULL) == ACTION_INVALID);
}

/* FU-139 §3.7: row 07 stage-0 target (native 0x8154C..0x815B5): the slot
 * word 0x60/0x8000 arm writes x/z from the camera plus the type-offset tables
 * shifted left 4; otherwise the camera triple is copied. */
static void test_kick_stage_target(void) {
  fifa96_action_vec3 camera;
  fifa96_action_vec3 out;
  static const int8_t off_x[2] = {2, -1};
  static const int8_t off_z[2] = {-3, 4};
  uint8_t resolved;
  camera.x = 0x100;
  camera.y = 0x200;
  camera.z = 0x300;
  out.x = 0x777;
  out.y = 0x777;
  out.z = 0x777;
  assert(fifa96_action_kick_stage_target(1, 0x60, 0, &camera, off_x, off_z, &out,
                                         &resolved) == FIFA96_OK);
  assert(resolved == 1);
  assert(out.x == 0x100 + 0x20 && out.z == 0x300 - 0x30);
  assert(out.y == 0x777); /* untouched by the native arm */
  out.y = 0x777;
  assert(fifa96_action_kick_stage_target(1, 0x8000, 1, &camera, off_x, off_z, &out,
                                         &resolved) == FIFA96_OK);
  assert(resolved == 1);
  assert(out.x == 0x100 - 0x10 && out.z == 0x300 + 0x40);
  assert(fifa96_action_kick_stage_target(1, 0x61, 0, &camera, off_x, off_z, &out,
                                         &resolved) == FIFA96_OK);
  assert(resolved == 0);
  assert(out.x == 0x100 && out.y == 0x200 && out.z == 0x300);
  assert(fifa96_action_kick_stage_target(0, 0x60, 0, &camera, off_x, off_z, &out,
                                         &resolved) == FIFA96_OK);
  assert(resolved == 0);
  assert(fifa96_action_kick_stage_target(1, 0x60, 0, NULL, off_x, off_z, &out,
                                         &resolved) == ACTION_INVALID);
  assert(fifa96_action_kick_stage_target(1, 0x60, 0, &camera, NULL, off_z, &out,
                                         &resolved) == ACTION_INVALID);
  assert(fifa96_action_kick_stage_target(1, 0x60, 0, &camera, off_x, NULL, &out,
                                         &resolved) == ACTION_INVALID);
  assert(fifa96_action_kick_stage_target(1, 0x60, 0, &camera, off_x, off_z, NULL,
                                         &resolved) == ACTION_INVALID);
  assert(fifa96_action_kick_stage_target(1, 0x60, 0, &camera, off_x, off_z, &out,
                                         NULL) == ACTION_INVALID);
}

/* The plan's cluster-B acceptance at library level (FU-139 §3.5/§3.8): the
 * resolver picks a row (index 0 of the height table here), and that row's
 * fields then move the ball through the row application. The synthetic 10-byte
 * row is {event code, sub, lo word, hi word, traj add word, divisor, sub}. */
static void test_kick_resolver_selects_row_and_moves_ball(void) {
  static const uint8_t row10[10] = {0x02, 0x00, 0x00, 0x01, 0x00, 0x03,
                                    0x20, 0x00, 0x00, 0x00};
  fifa96_action_kick_event e = kick_event();
  fifa96_action_kick_event_out row;
  fifa96_action_kick_row krow;
  fifa96_action_kick_ball ball;
  e.code = 0x10; /* class 0 */
  e.height = 0x1000;
  e.ball_height = 0x1050; /* height table, d >= 0x38 */
  e.sector_byte = 0;      /* sector_bits[0] = 0x38 -> idx 0 at sector 0 */
  assert(fifa96_action_kick_event_row(&e, sector_bits, &row) == FIFA96_OK);
  assert(row.found == 1);
  assert(row.table == FIFA96_ACTION_KICK_EVENT_TABLE_HEIGHT);
  assert(row.index == 0);
  krow.lo = (uint16_t)(row10[2] | ((uint16_t)row10[3] << 8));
  krow.hi = (uint16_t)(row10[4] | ((uint16_t)row10[5] << 8));
  krow.traj_add = (uint16_t)(row10[6] | ((uint16_t)row10[7] << 8));
  ball = kick_ball(0x400, 0x100, 0x100);
  assert(fifa96_action_kick_apply(&ball, &krow, 0, row10[0], 0) == FIFA96_OK);
  assert(ball.x == 0x300);   /* hi word clamps 0x400 */
  assert(ball.traj == 0x120); /* 0x100 + 0x20 */
}


/* ===== FU-139 §9 (Task 11): rows 07/0F kick machine fixtures ===== */

static void kick_state_init(fifa96_action_kick *s, uint8_t row) {
  memset(s, 0, sizeof *s);
  s->row = row;
  s->phase = 2;
  s->side = 0;
  s->type8 = 0;
  s->type = 0;
}

static void test_kick_machine_07_reset_and_stage0(void) {
  fifa96_action_kick s;
  fifa96_action_kick_out out;
  static const int8_t offx[1] = {5};
  static const int8_t offz[1] = {-3};

  /* phase != 2 -> FUN_0007DAB4 reset. */
  kick_state_init(&s, 0x07);
  s.phase = 1;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.reset_install == 1 && out.reset_code == 0);
  assert(s.stage92 == 0xFF && s.timer89 == 0 && out.stage == 0xFF);

  /* stage 0 wait: lane > 0x40 and the timer under 0x3C does not advance. */
  kick_state_init(&s, 0x07);
  s.stage92 = 0;
  s.lane_word = 0x41;
  s.timer89 = 0x3B;
  s.delta = 1;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.ran == 1);
  assert(out.reset == 0);
  assert(s.stage92 == 0 && s.timer89 == 0x3C);

  /* stage 0 target: lane <= 0x40, t >= ball height, slot word 0x60. */
  kick_state_init(&s, 0x07);
  s.stage92 = 0;
  s.lane_word = 0;
  s.pos_y_word = 0;
  s.ball_height = 0x40;
  s.has_slot = 1;
  s.slot_word6 = 0x60;
  s.type8 = 0;
  s.type_off_x = offx;
  s.type_off_z = offz;
  s.camera_x = 1;
  s.camera_y = 2;
  s.camera_z = 3;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(s.target_resolved == 1);
  assert(s.target_x == 1 + (5 << 4));
  assert(s.target_y == 0);   /* the resolved arm leaves +0x51 untouched */
  assert(s.target_z == 3 + (-3) * 16);
  assert(s.timer89 == 0 && s.stage92 == 1);
}

static void test_kick_machine_07_decision_and_post(void) {
  fifa96_action_kick s;
  fifa96_action_kick_out out;

  /* The 0x7E600 decision fires (type 0, lane 0x100, predictor inside the
   * 0x20..0x60 band and 0xF0/lane distance, pos_z 0x7B0, angle inside
   * +/-0x100 and the face gate). */
  kick_state_init(&s, 0x07);
  s.stage92 = 1;
  s.is_team_cb = 1;          /* SI = 0x40 -> the 0x7E600 decision runs */
  s.type = 0;
  s.lane_word = 0x100;
  s.pos_x = 0;
  s.pos_z = 0x7B0;
  s.predictor_x = 0x10;
  s.predictor_y = 0x30;
  s.predictor_z = 0x7B0;
  s.face_word7d = 0x100;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.defender_install == 1);
  assert(out.kick == 0);
  assert(s.stage92 == 1);

  /* The gate byte is the +0x91 action code (FU-139 §9 erratum): code 7 has
   * flat[7]&1 == 0, so the same passing state requests the kick instead. */
  kick_state_init(&s, 0x07);
  s.stage92 = 1;
  s.is_team_cb = 1;
  s.type = 7;
  s.lane_word = 0x100;
  s.pos_x = 0;
  s.pos_z = 0x7B0;
  s.predictor_x = 0x10;
  s.predictor_y = 0x30;
  s.predictor_z = 0x7B0;
  s.face_word7d = 0x100;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.defender_install == 0);
  assert(out.kick == 1);

  /* The 0x7E6C8 camera-x gate is one-sided: pos_x 0x200 with camera_x 0x100
   * passes (camera_x <= pos_x), camera_x 0x300 refuses. */
  kick_state_init(&s, 0x07);
  s.stage92 = 1;
  s.is_team_cb = 1;
  s.type = 0;
  s.lane_word = 0x100;
  s.pos_x = 0x200;
  s.pos_z = 0x7B0;
  s.camera_x = 0x100;
  s.predictor_x = 0x210;
  s.predictor_y = 0x30;
  s.predictor_z = 0x7B0;
  s.face_word7d = 0x100;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.defender_install == 1);
  s.camera_x = 0x300;   /* camera_x > pos_x -> 0x7E6D0 fails */
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.defender_install == 0);
  assert(out.kick == 1);

  /* A refused decision (lane > 0x180) requests the kick; the
   * [0x14C32A]/[0x15B680]==4 downgrade turns SI 0x40 into 0x20. */
  kick_state_init(&s, 0x07);
  s.stage92 = 1;
  s.is_team_cb = 1;
  s.lane_word = 0x200;
  s.downgrade_gate = 1;
  s.downgrade_word = 4;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.defender_install == 0);
  assert(out.kick == 1 && out.kick_mode == 0x20);
  /* post-kick: staged traj 0x20, timer 0, the opponent type-6 no-slot record
   * with the (word[+0x6D], word[+0x6F]) angle matching word[+0x7D]. */
  s.kick_done = 1;
  s.kick_staged = 1;
  s.kick_traj = 0x20;
  s.timer89 = 0;
  s.opp_present = 1;
  s.opp_type = 6;
  s.opp_has_slot = 0;
  s.opp_lane_word = 0x80;
  s.opp_angle_x = 1;    /* kick_angle(1, 0) == 0x100 */
  s.opp_angle_z = 0;
  s.opp_face_word = 0x100;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.opponent_invoke == 1);
  assert(s.timer89 == 0 && s.stage92 == 2);

  /* The staged traj >= 0x30 blocks the invoke. */
  s.kick_done = 1;
  s.kick_staged = 1;
  s.kick_traj = 0x30;
  s.opp_present = 1;
  s.opp_type = 6;
  s.opp_has_slot = 0;
  s.opp_lane_word = 0x80;
  s.opp_angle_x = 1;
  s.opp_angle_z = 0;
  s.opp_face_word = 0x100;
  s.stage92 = 1;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.opponent_invoke == 0 && s.stage92 == 2);

  /* stage 2 with +0x44 set runs the tail (reset + ball install 4 + receiver). */
  kick_state_init(&s, 0x07);
  s.stage92 = 2;
  s.byte44 = 1;
  s.is_team_target = 1;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.reset == 1);
  assert(out.ball_install == 1 && out.receiver_timer == 1);
  assert(s.stage92 == 0xFF);
}

static void test_kick_machine_07_reset_decision(void) {
  fifa96_action_kick s;
  fifa96_action_kick_out out;
  /* The 0x7C990 forced-decision on the team target with a carrying opponent
   * target installs code 6; the type-5 non-carrying case installs nothing.
   * The tail is entered with phase 2 + active through the +0x81 gate. */
  kick_state_init(&s, 0x07);
  s.timer81 = 1;
  s.active = 1;
  s.is_team_target = 1;
  s.opp_target_present = 1;
  s.opp_target_carrier = 1;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.reset_install == 1 && out.reset_code == 6);
  assert(out.ball_install == 1 && out.receiver_timer == 1);

  kick_state_init(&s, 0x07);
  s.timer81 = 1;
  s.active = 1;
  s.is_team_target = 1;
  s.type = 5;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.reset_install == 0);

  /* The `s.type` byte is the native +0x91 action code (FU-139 §9 erratum):
   * code 7 takes the non-carrier code-4 install, not the code-5 keep. */
  kick_state_init(&s, 0x07);
  s.timer81 = 1;
  s.active = 1;
  s.is_team_target = 1;
  s.type = 7;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.reset_install == 1 && out.reset_code == 4);
}

static void test_kick_machine_0F_stage0(void) {
  fifa96_action_kick s;
  fifa96_action_kick_out out;
  kick_state_init(&s, 0x0F);
  s.stage92 = 0;
  s.active = 1;
  s.pos_x = 0x100;
  s.pos_z = 0x200;
  s.stage_target_x = 0x110;
  s.stage_target_z = 0x220;
  s.camera_x = 0x100;
  s.camera_z = 0x300;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  /* distance 0x30 < 0x50 -> the half-vector nudge. */
  assert(s.pos_x == 0x100 + 8);
  assert(s.pos_z == 0x200 + 0x10);
  assert(out.ran == 1 && out.camera_face == 1);
  assert(s.stage92 == 1 && s.timer89 == 0);
  assert(s.target_x == s.pos_x && s.target_z == s.pos_z);
  /* camera z 0x300 - pos z 0x210 = 0xF0 -> kick_angle(0, 0xF0) == 0 -> the
   * octant is ((0 + 0x40) & 0x3FF) >> 7 == 0. */
  assert(s.facing == 0);

  /* inactive or word[+0x85] -> reset. */
  kick_state_init(&s, 0x0F);
  s.stage92 = 0;
  s.active = 0;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.reset == 1 && s.stage92 == 0xFF);
  kick_state_init(&s, 0x0F);
  s.stage92 = 0;
  s.active = 1;
  s.word85 = 1;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.reset == 1);
}

static void test_kick_machine_0F_reload_and_corner(void) {
  fifa96_action_kick s;
  fifa96_action_kick_out out;
  struct fifa96_rng rng;

  /* The reload (0x82DA7) only runs when a kick ran (BX != 0 at 0x82DA2):
   * kick 1 first, then re-entry with kick_done = 1. word85 0x10, word87 4 ->
   * 2*0x10 - 4 + 0x1E = 0x3A. */
  kick_state_init(&s, 0x0F);
  s.stage92 = 1;
  s.pos_y_word = 0x40;
  s.ball_height = 0x40;
  s.lane_word = 0x20;
  s.bound_word = 0x40;
  s.timer81 = 0;
  s.has_slot = 1;
  s.slot_word6 = 0x60;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.snap == 1);
  assert(out.slot_restore == 1);
  assert(out.kick == 1 && out.kick_mode == 0x60);
  assert(out.timer81_set == 0);
  s.kick_done = 1;
  s.word85 = 0x10;
  s.word87 = 4;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.timer81_set == 1);
  assert(out.timer81_reload == 0x3A);
  assert(s.timer81 == 0x3A);

  /* No kick and predictor distance <= lane: BX == 0 -> 0x82DA5 returns
   * without the reload. */
  kick_state_init(&s, 0x0F);
  s.stage92 = 1;
  s.pos_y_word = 0x40;
  s.ball_height = 0x40;
  s.lane_word = 0x20;
  s.bound_word = 0x40;
  s.timer81 = 7;
  s.predictor_x = 0x10;
  s.predictor_z = 0;
  s.pos_x = 0;
  s.pos_z = 0;
  s.word85 = 0x10;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.timer81_set == 0);
  assert(s.timer81 == 7);

  /* No slot with the merge gates and a near predictor: slot_merge, no
   * reload (the 0x7876C request is independent of the kick). */
  kick_state_init(&s, 0x0F);
  s.stage92 = 1;
  s.pos_y_word = 0x40;
  s.ball_height = 0x40;
  s.lane_word = 0x20;
  s.bound_word = 0x40;
  s.team_slot_pool = 1;
  s.merge_gate_1586d7 = 0;
  s.predictor_x = 0x10;
  s.predictor_z = 0;
  s.pos_x = 0;
  s.pos_z = 0;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.slot_merge == 1);
  assert(out.timer81_set == 0);

  /* A far predictor and a corner: code 2 with a seed-0 first draw 0x200 ->
   * (rng & 7) == 0 -> no face, mode 0x20. */
  kick_state_init(&s, 0x0F);
  s.stage92 = 1;
  s.pos_y_word = 0x40;
  s.ball_height = 0x40;
  s.lane_word = 0x20;
  s.bound_word = 0x40;
  s.predictor_x = 0x400;
  s.predictor_z = 0;
  s.corner_x = 0x100;
  s.corner_z = 0x200;
  s.corner_code = 2;
  s.camera_x = 0;
  s.camera_z = 0;
  s.rng = &rng;
  assert(fifa96_rng_seed(&rng, 0) == FIFA96_OK);
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.corner_kick == 1);
  assert(out.corner_kick_mode == 0x20);
  assert(out.corner_face == 0);
  assert(s.kick_vec_height == 0x100);
  assert(s.kick_vec_z == 0x200);
  assert(s.kick_vec_x == (int16_t)fifa96_entity_distance(0x100, 0x200));
  /* after the corner kick (BX = 1) the reload runs. */
  s.kick_done = 2;
  s.word85 = 0x10;
  s.word87 = 0;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.timer81_set == 1);
  assert(out.timer81_reload == 0x20 + 0x1E);

  /* code 3 -> mode 0x40 without a draw. */
  kick_state_init(&s, 0x0F);
  s.stage92 = 1;
  s.pos_y_word = 0x40;
  s.ball_height = 0x40;
  s.lane_word = 0x20;
  s.bound_word = 0x40;
  s.predictor_x = 0x400;
  s.corner_x = 0x50;
  s.corner_z = 0x60;
  s.corner_code = 3;
  assert(fifa96_action_kick_machine(&s, &out) == FIFA96_OK);
  assert(out.corner_kick == 1 && out.corner_kick_mode == 0x40);
}

static void test_kick_machine_invalid(void) {
  fifa96_action_kick s;
  fifa96_action_kick_out out;
  assert(fifa96_action_kick_machine(NULL, &out) == ACTION_INVALID);
  assert(fifa96_action_kick_machine(&s, NULL) == ACTION_INVALID);
  kick_state_init(&s, 0x00);
  assert(fifa96_action_kick_machine(&s, &out) == ACTION_INVALID);
}

/* ===== FU-139 §11 (M2 arms-and-wiring Task 13 / OL-30): row 06 pursuit =====
 *
 * First-hand /FIFA96.EXE row 06 `0x801B4..0x809EF` (~597 insns; the row-09
 * handler at 0x80A00 is the action-table slot 0x1106E0[9] and is out of scope).
 * Every fixture expectation is hand-computed from the cited native sites (the
 * 0x114E04 fold values through the FU-139 §9 table, the 0x8DC68 octagonal
 * distance and the seed-0 `FUN_00092AC8` draw chain); none is read from the
 * port. */
static void pursuit_mates_init(fifa96_action_pursuit_mate *mates, uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    mates[i].x = 0x1000;      /* far, positive-signed 8DC68 distance (0x1600) */
    mates[i].z = 0x1000;
    mates[i].pos_z = 0x1000;
    mates[i].skip_98 = 0;
    mates[i].skip_9a = 0;
  }
}

static void pursuit_init(fifa96_action_pursuit *s, struct fifa96_rng *rng) {
  memset(s, 0, sizeof *s);
  s->phase = 2;
  s->active = 1;
  s->carrier = 0;          /* [0x158724] present (identity 0) */
  s->carrier_lane = 0;
  s->carrier_speed = 5;    /* word[carrier+0x71] > 4 (0x806BE) */
  s->byte90 = 3;           /* 3 - (int8)+0x90 = 0 (0x80731..0x80742) */
  s->byte99 = 0;
  s->byte9d = 0;
  s->desc_e = 0x0F;        /* (desc[+0xE] | +0x9D) install-9 base (0x80721) */
  s->rng = rng;
  s->team_target = FIFA96_ACTION_PURSUIT_NONE;
  s->team_second = FIFA96_ACTION_PURSUIT_NONE;
  s->self_index = 1;
  s->ball_height = 0;
  s->parity = 0;           /* [0x157A4F] even frame */
}

/* Entry, the phase/active resets and the carrier gate install 4. */
static void test_pursuit_entry_and_carrier_gate(void) {
  fifa96_action_pursuit s;
  fifa96_action_pursuit_out out;
  struct fifa96_rng rng;
  fifa96_action_pursuit_mate mates[2];
  pursuit_mates_init(mates, 2);
  pursuit_init(&s, &rng);

  /* 0x801BF: the +0x9E latch runs before every gate. */
  s.phase = 1;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.ran == 1);
  assert(out.reset == 1);
  assert(out.install == 0 && out.target_set == 0);

  /* 0x801E5: active == 0 -> reset + clear team+0x7B2/+0x7B6 when they are rec. */
  pursuit_init(&s, &rng);
  s.active = 0;
  s.actor = 5;
  s.team_target = 5;
  s.team_second = 5;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.ran == 1 && out.reset == 1);
  assert(out.clear_target == 1 && out.clear_second == 1);
  pursuit_init(&s, &rng);
  s.active = 0;
  s.actor = 5;
  s.team_target = 6;
  s.team_second = FIFA96_ACTION_PURSUIT_NONE;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.clear_target == 0 && out.clear_second == 0);

  /* 0x8022D..0x80263: carrier absent / carrier word +0x6B > 0x90 / the
   * [0x157750] ball height > 0x70 all install code 4 (invoke) and return. */
  pursuit_init(&s, &rng);
  s.carrier = FIFA96_ACTION_PURSUIT_NONE;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.install == 4 && out.target_set == 0 && out.reset == 0);
  pursuit_init(&s, &rng);
  s.carrier_lane = 0x91;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.install == 4);
  /* 0x90 is inside the gate; the run continues to the V4 install 8. */
  pursuit_init(&s, &rng);
  s.carrier_lane = 0x90;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.install == 8);
  pursuit_init(&s, &rng);
  s.ball_height = 0x71;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.install == 4);
  /* 0x70 is inside the carrier gate; the no-slot height gate (0x38) then
   * returns without a target or install. */
  pursuit_init(&s, &rng);
  s.ball_height = 0x70;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.install == 0);
  assert(out.target_set == 0);

  /* NULL arguments. */
  assert(fifa96_action_pursuit_step(NULL, mates, 2, &out) == ACTION_INVALID);
  assert(fifa96_action_pursuit_step(&s, mates, 2, NULL) == ACTION_INVALID);
}

/* The camera metric, the has-slot offside fold, the no-slot V1 target and the
 * install-9 RNG gate.
 *
 * Camera (-0x100, 0x11, 0xB10), side 1 -> V2 = {0,0,0xB10}; dx = 0x100,
 * dz = 0 -> dist 0x100 (0x8DC68), scaled 0x100>>3 = 0x20 (0x802E7),
 * angle = 0x100 (0x8DD70/0xCD474: +x quadrant). pos_z == cam_z takes the
 * 0x80316/0x80340 c0 path (scaled = 0xC0). No slot, actor == team+0x7B2:
 * timer89 += delta (0), clamp 0x20 -> 0x30 (0x803F5), fold V1:
 * x += fold(0x30, 0x100) = 0x30 -> -0xD0; z += fold(0x30, 0x200) = 0
 * -> 0xB10. flag54 fires (side 1: V1.z 0xB10 > 0x5A0 and > teammate 0), so
 * the receiver timer runs; target = V1, clamped (0x7D3E4). The V4 metric
 * 8DC68(0x61, 0) = 0x61 > 0x60 with parity 0 skips the adjust arm and reaches
 * the install-9 gate: base = 0xF << 0 = 0xF; the seed-0 first FUN_00092AC8
 * draw is 0x200 (the first "ArCaDe" chain step), so (rng & 0x1FF) = 0 < 0xF
 * -> install 9. The anim gate:
 * row byte 0, speed 0 < 3, lane word 0x80 < 0x90 -> id 0x1C. */
static void test_pursuit_fold_and_install_gate(void) {
  fifa96_action_pursuit s;
  fifa96_action_pursuit_out out;
  struct fifa96_rng rng;
  fifa96_action_pursuit_mate mates[2];
  pursuit_mates_init(mates, 2);
  pursuit_init(&s, &rng);
  assert(fifa96_rng_seed(&rng, 0) == FIFA96_OK);
  s.has_slot = 0;
  s.side = 1;
  s.actor = 7;
  s.team_target = 7;
  s.teammate_z = 0;
  s.camera_x = -0x100;
  s.camera_y = 0x11;
  s.camera_z = 0xB10;
  s.pos_x = 0x123;
  s.pos_z = 0xB10;
  s.word6d = 0x61;
  s.word6f = 0;
  s.lane_dword = (int32_t)((uint32_t)0x80u << 16);   /* word +0x6B = 0x80 */
  s.vel_x = 0;
  s.delta = 0;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.ran == 1 && out.reset == 0);
  assert(out.install == 9);
  assert(out.target_set == 1);
  assert(out.target_x == -0xD0);
  assert(out.target_y == 0x11);   /* the V1 triple copy carries camera y */
  assert(out.target_z == 0xB10);
  assert(out.receiver_timer == 1);
  assert(out.anim_set == 1 && out.anim == 0x1C);

  /* The same gate with a zero base (desc[+0xE] == 0) draws the RNG but
   * installs nothing (0x80791..0x807A2). */
  pursuit_init(&s, &rng);
  assert(fifa96_rng_seed(&rng, 0) == FIFA96_OK);
  s.has_slot = 0;
  s.side = 1;
  s.actor = 7;
  s.team_target = 7;
  s.camera_z = 0;
  s.pos_z = 0;
  s.word6d = 0x61;
  s.desc_e = 0;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.install == 0);

  /* The +0x99 byte blocks the install gates entirely (0x806A7). */
  pursuit_init(&s, &rng);
  s.has_slot = 0;
  s.actor = 7;
  s.team_target = 7;
  s.word6d = 0x61;
  s.byte99 = 1;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.install == 0);

  /* The parity + byte[0x15872F] adjust arm (0x806C7..0x8070C) draws first and
   * shifts the target x by byte[0x15872A]<<6 when (rng & 0xF) > gate. With
   * parity 1, byte 0x15872F = 0, desc[+0xC] = 0 and +0x9D = 0 the seed-0 first
   * draw 0x200 has (0x200 & 0xF) = 0, not > 0, so the x stays; the install
   * gate then draws 0x725, (0x725 & 0x1FF) = 0x125 >= base 0xF -> no install. */
  pursuit_init(&s, &rng);
  assert(fifa96_rng_seed(&rng, 0) == FIFA96_OK);
  s.has_slot = 0;
  s.actor = 7;
  s.team_target = 7;
  s.parity = 1;
  s.byte_15872f = 0;
  s.adjust_x = 2;
  s.word6d = 0x61;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.target_x == 0);   /* no adjust from the low-nibble-0 draw */
  assert(out.install == 0 && out.swap == 0);
}

/* Actor == team+0x7B6: the V0 block (0x80482..0x805B8). Camera (0,0,0),
 * side 1 -> V2 = {0,0,0xB10}; dist 0xB10, scaled 0xB10>>4 = 0xB1, angle 0.
 * No slot, flag54 clear, actor == team+0x7B2 false -> the fold puts V1 at
 * (0,0,0xB1); the V0 metric 8DC68(0, 0xB10-0xB1 = 0xA5F) and the speed-0x60
 * fold produce V0 = (0, 0, 0xB1 + fold(0x60,0x100) = 0x111), the target. */
static void test_pursuit_second_record_v0(void) {
  fifa96_action_pursuit s;
  fifa96_action_pursuit_out out;
  struct fifa96_rng rng;
  fifa96_action_pursuit_mate mates[2];
  pursuit_mates_init(mates, 2);
  pursuit_init(&s, &rng);
  s.has_slot = 0;
  s.side = 1;
  s.actor = 8;
  s.team_target = 7;
  s.team_second = 8;
  s.camera_x = 0;
  s.camera_y = 0;
  s.camera_z = 0;
  s.pos_x = 0;
  s.pos_z = 0;
  s.word6d = 0;
  s.word6f = 0;
  s.lane_dword = 0;
  s.vel_x = 0;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.install == 8);
  assert(out.target_set == 1);
  assert(out.target_x == 0);
  assert(out.target_y == 0);
  assert(out.target_z == 0x111);
  assert(out.receiver_timer == 0);
  assert(out.anim_set == 1 && out.anim == 0x1C);
}

/* The has-slot target arms (0x805BC..0x805F9): the slot byte +0x10 & 0x30
 * picks the camera triple, else FUN_00079C20 writes pos + dir*0x80 (y = 0)
 * and 0x7D3E4 clamps x to +-0x720 / z to +-0xB10. */
static void test_pursuit_slot_targets(void) {
  fifa96_action_pursuit s;
  fifa96_action_pursuit_out out;
  struct fifa96_rng rng;
  fifa96_action_pursuit_mate mates[2];
  pursuit_mates_init(mates, 2);
  pursuit_init(&s, &rng);
  s.has_slot = 1;
  s.slot_gate = 1;
  s.camera_x = 0x300;
  s.camera_y = 0x10;
  s.camera_z = 0x400;
  s.pos_x = 0x700;
  s.pos_z = 0xB00;
  s.word6d = 0;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.target_x == 0x300 && out.target_y == 0x10 && out.target_z == 0x400);

  pursuit_init(&s, &rng);
  s.has_slot = 1;
  s.slot_gate = 0;
  s.pos_x = 0x700;
  s.pos_z = 0xB00;
  s.slot_dir_x = 2;
  s.slot_dir_z = 1;
  s.camera_x = 0x111;   /* not the target on this arm */
  s.camera_y = 0x22;
  s.camera_z = 0x333;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.target_x == 0x720);   /* 0x700 + 2*0x80 = 0x800 -> clamp */
  assert(out.target_y == 0);
  assert(out.target_z == 0xB10);   /* 0xB00 + 1*0x80 = 0xB80 -> clamp */

  /* The 0x6E598 id gate: row byte 0x1C with speed > 4 (0x807DD) requests 2. */
  pursuit_init(&s, &rng);
  s.row_byte = 0x1C;
  s.vel_x = 5;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.anim_set == 1 && out.anim == 2);
  /* speed == 4 is not > 4 and lane 0 is not > 0xC0 -> no id. */
  pursuit_init(&s, &rng);
  s.row_byte = 0x1C;
  s.vel_x = 4;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.anim_set == 0);
  /* Other row bytes: speed == 3 is not < 3 -> no id. */
  pursuit_init(&s, &rng);
  s.vel_x = 3;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.anim_set == 0);
}

/* The no-slot early returns: word[+0x81] != 0 copies the position triple and
 * returns without the clamp/V4/anim arms (0x805FE..0x80611); the ball height
 * dword > 0x38 returns without a target (0x8061B..0x80622). */
static void test_pursuit_early_returns(void) {
  fifa96_action_pursuit s;
  fifa96_action_pursuit_out out;
  struct fifa96_rng rng;
  fifa96_action_pursuit_mate mates[2];
  pursuit_mates_init(mates, 2);
  pursuit_init(&s, &rng);
  s.has_slot = 0;
  s.timer81 = 1;
  s.pos_x = 0x800;   /* beyond +-0x720: the early return leaves it unclamped */
  s.pos_y = 0x55;
  s.pos_z = -0x900;
  s.camera_z = 0;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.target_set == 1);
  assert(out.target_x == 0x800 && out.target_y == 0x55 && out.target_z == -0x900);
  assert(out.install == 0 && out.receiver_timer == 0 && out.anim_set == 0);

  pursuit_init(&s, &rng);
  s.has_slot = 0;
  s.timer81 = 0;
  s.ball_height = 0x39;
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.target_set == 0);
  assert(out.install == 0 && out.anim_set == 0);
}

/* The parity claim arm (0x8082B..0x809A2) and the 0x79CCC/0x6DA64 tail.
 * Camera (0,0,0), side 1, no slot: V1 = (0,0,0xB1). Parity 1, actor ==
 * team+0x7B2: the 0x8DE8C search (skip index 0) picks mate 2 at (0,0xB1)
 * (distance 0; self at (0,0) is 0xB1) -> team+0x7B2 = mate 2, team+0x7B6 = 0.
 * The 0x79CCC callback search (self's +0x9A latched, skip 0) picks mate 2 at
 * 0xB1 < 0xC0 -> the 0x6DA64 swap request. */
static void test_pursuit_claim_arm(void) {
  fifa96_action_pursuit s;
  fifa96_action_pursuit_out out;
  struct fifa96_rng rng;
  fifa96_action_pursuit_mate mates[11];
  pursuit_mates_init(mates, 11);
  pursuit_init(&s, &rng);
  s.has_slot = 0;
  s.side = 1;
  s.actor = 3;
  s.team_target = 3;
  s.self_index = 3;
  s.parity = 1;
  s.pos_x = 0;
  s.pos_z = 0;
  s.camera_x = 0;
  s.camera_y = 0;
  s.camera_z = 0;
  /* Mate 0 sits at the search target too: the native skip index 0 must keep it
   * out of both searches (the 0x8DE8C/0x79CCC skip argument); mate 2 wins the
   * strict `<` tie. */
  mates[0].x = 0;
  mates[0].z = 0xB1;
  mates[0].pos_z = 0xB1;
  mates[3].x = 0;
  mates[3].z = 0;
  mates[3].pos_z = 0;
  mates[2].x = 0;
  mates[2].z = 0xB1;
  mates[2].pos_z = 0xB1;
  assert(fifa96_action_pursuit_step(&s, mates, 11, &out) == FIFA96_OK);
  assert(out.team_target_set == 1);
  assert(out.team_target_index == 2);
  assert(out.team_second_set == 1);
  assert(out.team_second_index == FIFA96_ACTION_PURSUIT_NONE);
  assert(out.swap == 1 && out.swap_index == 2);
}

/* The carrier mirror (`0x808AD CMP CX,[ESP+0x24]; 0x808B2 JL 0x80903`): the
 * mirror at `0x808B4` runs when the self distance is >= the carrier distance.
 * Common derivation: camera (0,0,0x600), side 1 -> dist 0x510, scaled 0xA2,
 * angle 0, fold V1 = (0,0,0x6A2); flag54 (V1.z > 0x5A0 and > teammate z);
 * target = V1, receiver timer. */
static void test_pursuit_carrier_mirror_equal(void) {
  fifa96_action_pursuit s;
  fifa96_action_pursuit_out out;
  struct fifa96_rng rng;
  fifa96_action_pursuit_mate mates[11];
  pursuit_mates_init(mates, 11);
  pursuit_init(&s, &rng);
  s.has_slot = 0;
  s.side = 1;
  s.actor = 3;
  s.team_target = 3;
  s.self_index = 3;
  s.parity = 1;
  s.teammate_z = 0;
  s.pos_x = 0;
  s.pos_z = 0x6A2;            /* self sits exactly on V1: self_dist 0 */
  s.camera_x = 0;
  s.camera_y = 0;
  s.camera_z = 0x600;
  s.carrier_pos_x = 0;
  s.carrier_pos_z = 0x6A2;    /* cdist 0 == self_dist: the >= mirror runs */
  mates[3].x = 0;
  mates[3].z = 0x6A2;
  mates[3].pos_z = 0x6A2;
  /* The mirror delta is 0, so V1' = V1; the 0x808EB search (self latched)
   * picks mate 5 at V1+(0,0x100) over mate 6 at the {0,0,0xB10} fallback
   * origin (0x34E). The height gate then skips (0x7A2+0x60 >= 0x6A2), so a
   * wrongly taken re-search would pick mate 6 instead. */
  mates[5].x = 0;
  mates[5].z = 0x7A2;
  mates[5].pos_z = 0x7A2;
  mates[6].x = 0;
  mates[6].z = 0xB10;
  mates[6].pos_z = 0xB10;
  assert(fifa96_action_pursuit_step(&s, mates, 11, &out) == FIFA96_OK);
  assert(out.target_set == 1);
  assert(out.target_z == 0x6A2);
  assert(out.receiver_timer == 1);
  assert(out.team_target_set == 1);
  assert(out.team_target_index == FIFA96_ACTION_PURSUIT_SELF);
  assert(out.team_second_set == 1);
  assert(out.team_second_index == 5);
  assert(out.swap == 0);
}

/* self_dist 0x5E < cdist 0x95E (carrier at (0,0x1000)): `0x808B2 JL 0x80903`
 * clears team+0x7B6 and skips the mirror and the 0x808EB search; flag54 is 0
 * (V1.z 0x6A2 <= teammate 0x700), so the second write stays NONE. Mate 5 at
 * the would-be mirrored point (0,0,-0x2BC) would be picked by a mirroring
 * port, so this fixture pins the direction. */
static void test_pursuit_carrier_no_mirror(void) {
  fifa96_action_pursuit s;
  fifa96_action_pursuit_out out;
  struct fifa96_rng rng;
  fifa96_action_pursuit_mate mates[11];
  pursuit_mates_init(mates, 11);
  pursuit_init(&s, &rng);
  s.has_slot = 0;
  s.side = 1;
  s.actor = 3;
  s.team_target = 3;
  s.self_index = 3;
  s.parity = 1;
  s.teammate_z = 0x700;
  s.pos_x = 0;
  s.pos_z = 0x700;
  s.camera_x = 0;
  s.camera_y = 0;
  s.camera_z = 0x600;
  s.carrier_pos_x = 0;
  s.carrier_pos_z = 0x1000;
  mates[3].x = 0;
  mates[3].z = 0x700;
  mates[3].pos_z = 0x700;
  mates[5].x = 0;
  mates[5].z = -0x2BC;
  mates[5].pos_z = -0x2BC;
  assert(fifa96_action_pursuit_step(&s, mates, 11, &out) == FIFA96_OK);
  assert(out.target_z == 0x6A2);
  assert(out.receiver_timer == 0);
  assert(out.team_target_index == FIFA96_ACTION_PURSUIT_SELF);
  assert(out.team_second_set == 1);
  assert(out.team_second_index == FIFA96_ACTION_PURSUIT_NONE);
  assert(out.swap == 0);
}

/* The flag54 height gate research (`0x80952` fails, `0x80956..0x809A2`):
 * self at (0,0x4A2) and carrier at (0,0x8A2) give self_dist == cdist == 0x200,
 * so the `>=` mirror runs and V1 (0,0,0x6A2) maps to V1' = (0,0,0x4A2). Mate 5
 * sits exactly on V1' (first-search distance 0x200 ties self; the strict `<`
 * keeps the earlier self index 3) and wins the 0x808EB search; the gate then
 * fails (|0x4A2| + 0x60 < |0x8A2|), so the re-search from {0,0,0xB10}
 * (camera_z 0x600 >= 0) picks mate 6. The 0x79CCC search from the self
 * position finds mate 5 at distance 0 -> the 0x6DA64 swap request. */
static void test_pursuit_height_gate_research(void) {
  fifa96_action_pursuit s;
  fifa96_action_pursuit_out out;
  struct fifa96_rng rng;
  fifa96_action_pursuit_mate mates[11];
  pursuit_mates_init(mates, 11);
  pursuit_init(&s, &rng);
  s.has_slot = 0;
  s.side = 1;
  s.actor = 3;
  s.team_target = 3;
  s.self_index = 3;
  s.parity = 1;
  s.teammate_z = 0;
  s.pos_x = 0;
  s.pos_z = 0x4A2;
  s.camera_x = 0;
  s.camera_y = 0;
  s.camera_z = 0x600;
  s.carrier_pos_x = 0;
  s.carrier_pos_z = 0x8A2;
  mates[3].x = 0;
  mates[3].z = 0x4A2;
  mates[3].pos_z = 0x4A2;
  mates[5].x = 0;
  mates[5].z = 0x4A2;
  mates[5].pos_z = 0x4A2;
  mates[6].x = 0;
  mates[6].z = 0xB10;
  mates[6].pos_z = 0xB10;
  assert(fifa96_action_pursuit_step(&s, mates, 11, &out) == FIFA96_OK);
  assert(out.target_set == 1);
  assert(out.target_z == 0x6A2);
  assert(out.receiver_timer == 1);
  assert(out.team_target_index == FIFA96_ACTION_PURSUIT_SELF);
  assert(out.team_second_index == 6);
  assert(out.swap == 1 && out.swap_index == 5);
}

/* The conditional second teammate-timer subtraction (`0x803BE..0x803EE`):
 * score[idx(side)] < score[idx(side^1)] (unsigned 16-bit, `0x803EC JNC`)
 * subtracts the +0x89 word again. Camera (0,0,0), side 1 -> scaled 0xB1,
 * angle 0, fold puts the target z at the final scaled word; timer89 0x10 and
 * delta 0. Own < other -> 0xB1 - 0x10 - 0x10 = 0x91; own >= other -> 0xA1. */
static void test_pursuit_score_second_subtraction(void) {
  fifa96_action_pursuit s;
  fifa96_action_pursuit_out out;
  struct fifa96_rng rng;
  fifa96_action_pursuit_mate mates[2];
  pursuit_mates_init(mates, 2);
  pursuit_init(&s, &rng);
  s.has_slot = 0;
  s.side = 1;
  s.actor = 1;
  s.team_target = 1;
  s.timer89 = 0x10;
  s.delta = 0;
  s.score_own = 0;
  s.score_other = 1;   /* own < other: the second subtraction runs */
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.target_set == 1);
  assert(out.target_z == 0x91);
  assert(out.install == 8);

  pursuit_init(&s, &rng);
  s.has_slot = 0;
  s.side = 1;
  s.actor = 1;
  s.team_target = 1;
  s.timer89 = 0x10;
  s.score_own = 1;
  s.score_other = 1;   /* equal: JNC skips */
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.target_z == 0xA1);

  pursuit_init(&s, &rng);
  s.has_slot = 0;
  s.side = 1;
  s.actor = 1;
  s.team_target = 1;
  s.timer89 = 0x10;
  s.score_own = 1;
  s.score_other = 0x8000;   /* unsigned 1 < 0x8000 (a signed compare skips) */
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.target_z == 0x91);
}

/* The x-adjust gate byte is signed (`0x806D0 MOV EAX,[0x15872C]; SAR 0x18` ->
 * `(int8)byte[0x15872F] < 2` at `0x806D8/0x806DB JGE`). The seed-0 chain
 * draws 0x200, 0x725, 0x133F; the fixture warms up one step so the parity arm
 * consumes 0x725 -- (0x725 & 0xF) = 5 > gate 0 -- and shifts the V0 target x
 * by adjust_x<<6 = 0x80. The install gate then consumes 0x133F,
 * (0x133F & 0x1FF) = 0x13F >= base 0xF, so no install. With the byte 0xFF
 * (-1) read unsigned (`0xFF < 2u` false) the arm would be skipped and the
 * target x would stay 0. */
static void test_pursuit_adjust_gate_signed(void) {
  fifa96_action_pursuit s;
  fifa96_action_pursuit_out out;
  struct fifa96_rng rng;
  fifa96_action_pursuit_mate mates[2];
  uint16_t warm = 0;
  pursuit_mates_init(mates, 2);
  pursuit_init(&s, &rng);
  assert(fifa96_rng_seed(&rng, 0) == FIFA96_OK);
  assert(fifa96_rng_step(&rng, &warm) == FIFA96_OK);
  assert(warm == 0x200);   /* hand-derived first draw */
  s.has_slot = 0;
  s.actor = 0;
  s.team_target = FIFA96_ACTION_PURSUIT_NONE;
  s.parity = 1;
  s.byte_15872f = (int8_t)-1;
  s.adjust_x = 2;
  s.desc_c = 0;
  s.word6d = 0x61;   /* metric in (0x60,0x90]: reaches the adjust arm */
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
  assert(out.target_set == 1);
  assert(out.target_x == 0x80);
  assert(out.install == 0);
}

/* Missing candidate records for a search arm is an error (the native reads the
 * caller's team block), and a NULL mate array is only invalid when a search
 * arm runs. */
static void test_pursuit_invalid(void) {
  fifa96_action_pursuit s;
  fifa96_action_pursuit_out out;
  struct fifa96_rng rng;
  fifa96_action_pursuit_mate mates[2];
  pursuit_mates_init(mates, 2);
  pursuit_init(&s, &rng);
  s.has_slot = 0;
  s.actor = 3;
  s.team_target = 3;
  s.self_index = 0;   /* no search arm needs the array at parity 0 */
  assert(fifa96_action_pursuit_step(&s, NULL, 0, &out) == FIFA96_OK);
  s.parity = 1;
  assert(fifa96_action_pursuit_step(&s, NULL, 0, &out) == ACTION_INVALID);
  assert(fifa96_action_pursuit_step(&s, mates, 2, &out) == FIFA96_OK);
}

/* C3-OL2 / FU-72 §2.4 errata: the derived FUN_00093944 goal writer.
 * Hand-computed from the first-hand body `/FIFA96.EXE` 0x93944..0x93B7D
 * (instruction sites quoted in the test comments). */
static void score_init(fifa96_action_score *s, uint16_t s0, uint16_t s1, int32_t tracked,
                       int32_t max_diff) {
  memset(s, 0, sizeof *s);
  s->score[0] = s0;
  s->score[1] = s1;
  s->tracked_side = tracked;
  s->max_diff = max_diff;
  s->last_side = -1;
}

/* The tracked-side -1 sentinel (0x9395E/0x93961): the writer is the plain FU-72
 * increment plus the last-side record; no diff bookkeeping and no event posts,
 * even at a threshold score. The word pair wraps (0x9394B INC word). */
static void test_score_event_untracked(void) {
  fifa96_action_score s;
  fifa96_action_score_out out;
  score_init(&s, 0, 0, -1, 0);
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 1 && s.score[1] == 0);
  assert(s.last_side == 0);
  assert(s.max_diff == 0);                  /* no 0x93967..0x93992 bookkeeping */
  assert(out.posted == 0 && out.post_id == 0);

  score_init(&s, 8, 0, -1, 0);              /* 9-0 would post 0xA0 when tracked */
  assert(fifa96_action_score_event(&s, 0, 3, &out) == FIFA96_OK);
  assert(s.score[0] == 9 && s.max_diff == 0 && out.posted == 0);

  score_init(&s, 0xFFFF, 0, -1, 0);
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 0);                  /* 16-bit wrap */
  assert(s.last_side == 0);
}

/* The tracked arm (0x93997 JZ 0x93AA6): side == tracked_side. 0x9A fires on
 * `(int16)diff + 3 == max_diff && max_diff > 3` (0x93AA6..0x93AC0). */
static void test_score_event_tracked_9a(void) {
  fifa96_action_score s;
  fifa96_action_score_out out;
  /* tracked 0, other = score[1] = 4; after the increment diff = 4 - 3 = 1,
   * max_diff stays 4 (1 < 4), 1 + 3 == 4 > 3 -> post 0x9A. */
  score_init(&s, 2, 4, 0, 4);
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 3 && s.score[1] == 4);
  assert(s.max_diff == 4 && s.last_side == 0);
  assert(out.posted == 1 && out.post_id == 0x9A);

  /* The 0x93AB6 `CMP EBP,3 / JLE` guard: the same equality with max_diff 3
   * must NOT fire. */
  score_init(&s, 2, 3, 0, 3);               /* diff = 3 - 3 = 0; 0 + 3 == 3 */
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 3 && out.posted == 0);
}

/* Tracked arm thresholds 0x9B (3-0), 0x9C (5 with other < 3), 0x9D (9 with
 * other < 5), and the miss arm (0x93B02..0x93B78). */
static void test_score_event_tracked_thresholds(void) {
  fifa96_action_score s;
  fifa96_action_score_out out;

  score_init(&s, 2, 0, 0, 0);               /* -> 3-0 */
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 3 && out.posted == 1 && out.post_id == 0x9B);

  score_init(&s, 4, 2, 0, 0);               /* -> 5-2, other 2 < 3 */
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 5 && out.posted == 1 && out.post_id == 0x9C);

  score_init(&s, 4, 3, 0, 0);               /* -> 5-3, other 3 not < 3 */
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 5 && out.posted == 0);

  score_init(&s, 8, 4, 0, 0);               /* -> 9-4, other 4 < 5 */
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 9 && out.posted == 1 && out.post_id == 0x9D);

  score_init(&s, 8, 5, 0, 0);               /* -> 9-5, other 5 not < 5 */
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 9 && out.posted == 0);

  /* 3-0 with tracked 1 mirrors onto side 1 (other = score[0]). */
  score_init(&s, 0, 2, 1, 0);
  assert(fifa96_action_score_event(&s, 1, 0, &out) == FIFA96_OK);
  assert(s.score[1] == 3 && s.last_side == 1 && out.posted == 1 && out.post_id == 0x9B);

  /* The signed max-diff update (0x9398B MOVSX): diff -3 raises max_diff -5. */
  score_init(&s, 2, 0, 0, -5);              /* -> 3-0, diff = 0 - 3 = -3 */
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.max_diff == -3 && out.post_id == 0x9B);
}

/* The non-tracked arm (side != tracked): the 0xD3 probe arm (0x939B0..0x939DF)
 * and the 0x9E/0x9F/0xA0 thresholds (0x939E4..0x93AA0). Every posting arm
 * returns (the native epilogue jump); the 0xCBC4C probe is consumed only on
 * the score == 1 && other < 3 path (native short-circuit). */
static void test_score_event_untracked_thresholds(void) {
  fifa96_action_score s;
  fifa96_action_score_out out;

  /* 0xD3: score[side] 0 -> 1, other 0 < 3, probe & 3 != 0. */
  score_init(&s, 0, 0, 1, 0);
  assert(fifa96_action_score_event(&s, 0, 3, &out) == FIFA96_OK);
  assert(s.score[0] == 1 && s.last_side == 0);
  assert(out.posted == 1 && out.post_id == 0xD3);

  /* TEST AL,3 (0x939D6): probe bits 0-1 clear falls through the 4/7/9 checks
   * with score[0] == 1 (probe 4 has bit 2 set, low two clear). */
  score_init(&s, 0, 0, 1, 0);
  assert(fifa96_action_score_event(&s, 0, 4, &out) == FIFA96_OK);
  assert(s.score[0] == 1 && out.posted == 0);

  /* The probe is not consumed when other >= 3: probe 3 still posts nothing. */
  score_init(&s, 0, 3, 1, 0);
  assert(fifa96_action_score_event(&s, 0, 3, &out) == FIFA96_OK);
  assert(s.score[0] == 1 && out.posted == 0);

  /* 0x9E: 3 -> 4 with other < 2. */
  score_init(&s, 3, 1, 1, 0);
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 4 && s.max_diff == 3);
  assert(out.posted == 1 && out.post_id == 0x9E);

  score_init(&s, 3, 2, 1, 0);               /* -> 4-2: other not < 2 */
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 4 && out.posted == 0);

  /* 0x9F: 6 -> 7 with other < 3. */
  score_init(&s, 6, 2, 1, 0);
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 7 && out.posted == 1 && out.post_id == 0x9F);

  score_init(&s, 6, 3, 1, 0);               /* -> 7-3: other not < 3 */
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 7 && out.posted == 0);

  /* 0xA0: 8 -> 9 with other < 4. */
  score_init(&s, 8, 3, 1, 0);
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 9 && out.posted == 1 && out.post_id == 0xA0);

  score_init(&s, 8, 4, 1, 0);               /* -> 9-4: other not < 4 */
  assert(fifa96_action_score_event(&s, 0, 0, &out) == FIFA96_OK);
  assert(s.score[0] == 9 && out.posted == 0);

  /* Non-tracked side 1 mirror of 0x9E (other = score[0]). */
  score_init(&s, 1, 3, 0, 0);
  assert(fifa96_action_score_event(&s, 1, 0, &out) == FIFA96_OK);
  assert(s.score[1] == 4 && out.posted == 1 && out.post_id == 0x9E);
}

static void test_score_event_invalid(void) {
  fifa96_action_score s;
  fifa96_action_score_out out;
  score_init(&s, 0, 0, -1, 0);
  assert(fifa96_action_score_event(NULL, 0, 0, &out) == ACTION_INVALID);
  assert(fifa96_action_score_event(&s, 0, 0, NULL) == ACTION_INVALID);
  assert(fifa96_action_score_event(&s, 2, 0, &out) == ACTION_INVALID);
  s.tracked_side = 2;                       /* native reads garbage; hardened */
  assert(fifa96_action_score_event(&s, 0, 0, &out) == ACTION_INVALID);
  assert(s.score[0] == 0 && s.score[1] == 0);
  assert(s.last_side == -1);
}

int main(void) {
  test_move_target();
  test_move_step();
  test_kick_angle();
  test_kick_apply();
  test_stage_wait();
  test_sequence_marker_target();
  test_locomotion_restart_wait();
  test_locomotion_placement_counter();
  test_locomotion_phase1_clamp();
  test_sequence_velocity_scale();
  test_kick_range_band();
  test_kick_event_row_height_table();
  test_kick_event_row_carry_fast_path();
  test_kick_event_row_code60();
  test_kick_event_row_band_and_state();
  test_kick_event_row_angle_sector();
  test_kick_event_row_sector_mask();
  test_kick_event_row_none_and_invalid();
  test_kick_event_append();
  test_kick_stage_target();
  test_kick_resolver_selects_row_and_moves_ball();
  test_kick_machine_07_reset_and_stage0();
  test_kick_machine_07_decision_and_post();
  test_kick_machine_07_reset_decision();
  test_kick_machine_0F_stage0();
  test_kick_machine_0F_reload_and_corner();
  test_kick_machine_invalid();
  test_pursuit_entry_and_carrier_gate();
  test_pursuit_fold_and_install_gate();
  test_pursuit_second_record_v0();
  test_pursuit_slot_targets();
  test_pursuit_early_returns();
  test_pursuit_claim_arm();
  test_pursuit_carrier_mirror_equal();
  test_pursuit_carrier_no_mirror();
  test_pursuit_height_gate_research();
  test_pursuit_score_second_subtraction();
  test_pursuit_adjust_gate_signed();
  test_pursuit_invalid();
  test_score_event_untracked();
  test_score_event_tracked_9a();
  test_score_event_tracked_thresholds();
  test_score_event_untracked_thresholds();
  test_score_event_invalid();
  puts("test_action_handlers: all assertions passed");
  return 0;
}
