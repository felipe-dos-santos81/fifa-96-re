// tests/test_action_handlers.c — FU-76 action-handler movement/angle/kick math
// (docs/ghidra/FU76_action_handlers.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_action_handlers.h"

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
  puts("test_action_handlers: all assertions passed");
  return 0;
}
