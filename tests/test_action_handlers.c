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

int main(void) {
  test_move_target();
  test_move_step();
  test_kick_angle();
  test_kick_apply();
  puts("test_action_handlers: all assertions passed");
  return 0;
}
