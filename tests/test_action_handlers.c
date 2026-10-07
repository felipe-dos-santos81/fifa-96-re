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
  puts("test_action_handlers: all assertions passed");
  return 0;
}
