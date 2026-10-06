// tests/test_action_locomotion.c — FU-77 shared locomotion integrator and per-code
// target arms (docs/ghidra/FU77_locomotion.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_action_handlers.h"

_Static_assert(offsetof(fifa96_action_locomotion, pos_x) == 0, "pos_x");
_Static_assert(offsetof(fifa96_action_locomotion, pos_z) == 4, "pos_z");
_Static_assert(offsetof(fifa96_action_locomotion, target_x) == 8, "target_x");
_Static_assert(offsetof(fifa96_action_locomotion, target_z) == 12, "target_z");
_Static_assert(offsetof(fifa96_action_locomotion, delta_x) == 16, "delta_x");
_Static_assert(offsetof(fifa96_action_locomotion, delta_z) == 18, "delta_z");
_Static_assert(offsetof(fifa96_action_locomotion, distance) == 20, "distance");
_Static_assert(offsetof(fifa96_action_locomotion, facing) == 22, "facing");
_Static_assert(offsetof(fifa96_action_locomotion, desired_facing) == 24, "desired_facing");
_Static_assert(offsetof(fifa96_action_locomotion, speed) == 26, "speed");
_Static_assert(offsetof(fifa96_action_locomotion, vel_x) == 28, "vel_x");
_Static_assert(offsetof(fifa96_action_locomotion, vel_z) == 30, "vel_z");
_Static_assert(offsetof(fifa96_action_locomotion, face_x) == 32, "face_x");
_Static_assert(offsetof(fifa96_action_locomotion, face_z) == 34, "face_z");
_Static_assert(offsetof(fifa96_action_locomotion, move_attr) == 36, "move_attr");
_Static_assert(offsetof(fifa96_action_locomotion, stride_rate) == 40, "stride_rate");
_Static_assert(offsetof(fifa96_action_locomotion, heading) == 44, "heading");
_Static_assert(offsetof(fifa96_action_locomotion, has_slot) == 45, "has_slot");
_Static_assert(offsetof(fifa96_action_locomotion, direct_face) == 46, "direct_face");
_Static_assert(offsetof(fifa96_action_locomotion, body_timer) == 47, "body_timer");
_Static_assert(offsetof(fifa96_action_locomotion, stride) == 48, "stride");
_Static_assert(offsetof(fifa96_action_locomotion, delta) == 50, "delta");

#define ACTION_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static uint8_t heading[32];
static int16_t stride_table[0x500];

static fifa96_action_locomotion loco(void) {
  fifa96_action_locomotion m;
  memset(&m, 0, sizeof(m));
  m.heading = 0xEE;
  for (int i = 0; i < 32; i++) heading[i] = (uint8_t)(0x10 + i);
  memset(stride_table, 0, sizeof(stride_table));
  return m;
}

static void test_step_target_and_distance(void) {
  fifa96_action_locomotion m = loco();
  m.target_x = 0x100;
  m.facing = 0x100;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.delta_x == 0x100 && m.delta_z == 0);
  assert(m.distance == 0x100);
  assert(m.desired_facing == 0x100);
  m = loco();
  m.pos_x = 0x1000;
  m.pos_z = 0x2000;
  m.target_x = 0x1000;
  m.target_z = 0x2000;
  m.facing = 0x33;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.delta_x == 0 && m.delta_z == 0 && m.distance == 0);
  assert(m.desired_facing == 0x33);
  assert(m.body_timer == 0 && m.vel_x == 0 && m.vel_z == 0 && m.speed == 0);
  assert(fifa96_action_locomotion_step(NULL, heading, stride_table) == ACTION_INVALID);
  assert(fifa96_action_locomotion_step(&m, NULL, stride_table) == ACTION_INVALID);
  assert(fifa96_action_locomotion_step(&m, heading, NULL) == ACTION_INVALID);
}

static void test_step_turn_clamp(void) {
  fifa96_action_locomotion m = loco();
  m.target_x = 1;
  m.direct_face = 1;
  m.move_attr = 0x00700000;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.facing == 0x40 && m.desired_facing == 0x100 && m.heading == 0x12);
  m = loco();
  m.target_x = 1;
  m.direct_face = 1;
  m.has_slot = 1;
  m.move_attr = 0x00700000;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.facing == 0x100 && m.heading == 0x18);
  m = loco();
  m.target_x = 1;
  m.direct_face = 1;
  m.move_attr = 0x00700000;
  m.speed = 0x10;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.facing == 0 && m.heading == 0x10);
  m = loco();
  m.target_x = 1;
  m.direct_face = 1;
  m.move_attr = 0x00700000;
  m.speed = 0x20;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.facing == -0x40 && m.heading == 0x2E);
}

static void test_step_angle_wrap(void) {
  fifa96_action_locomotion m = loco();
  m.facing = 0x100;
  m.target_z = 1;
  m.direct_face = 1;
  m.move_attr = 0x00700000;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.desired_facing == 0 && m.facing == 0xC0 && m.heading == 0x16);
}

static void test_step_direct_face(void) {
  fifa96_action_locomotion m = loco();
  m.target_x = 1;
  m.direct_face = 1;
  m.move_attr = 0x00500000;
  m.face_x = -1;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.desired_facing == 0x100 && m.facing == -0x40);
  m = loco();
  m.target_x = 1;
  m.direct_face = 1;
  m.move_attr = 0x00700000;
  m.face_x = -1;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.facing == 0x40);
  m = loco();
  m.target_x = 1;
  m.direct_face = 1;
  m.move_attr = 0x00500000;
  m.facing = 0x55;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.facing == 0x55 && m.heading == 0xEE);
  m = loco();
  m.target_x = 1;
  m.facing = 0;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.facing == 0 && m.heading == 0xEE);
}

static void test_step_stride_ramp(void) {
  fifa96_action_locomotion m = loco();
  m.target_x = 0x100;
  m.facing = 0x100;
  m.move_attr = 0x00080000;
  m.stride = 0x0F;
  m.body_timer = 3;
  m.delta = 10;
  stride_table[0x101] = 0x20;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.body_timer == 0 && m.vel_x == 0x10 && m.vel_z == 0 && m.speed == 0x10);
  assert(m.pos_x == 0xA0 && m.pos_z == 0);
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.vel_x == 0x18 && m.speed == 0x18 && m.pos_x == 0x190);
  m = loco();
  m.target_x = 0x100;
  m.facing = 0x100;
  m.move_attr = 0x00080000;
  m.stride = 0x0F;
  m.vel_x = 0x1F;
  m.body_timer = 3;
  m.delta = 10;
  stride_table[0x101] = 0x20;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.vel_x == 0x20 && m.speed == 0x20 && m.pos_x == 0x140);
  m = loco();
  m.target_x = 0x100;
  m.facing = 0x100;
  m.move_attr = 0x00080000;
  m.stride = 0x0F;
  m.body_timer = 3;
  m.delta = 10;
  stride_table[0x101] = -5;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.vel_x == -2 && m.speed == 2 && m.pos_x == -20);
  m = loco();
  m.target_x = 0x100;
  m.facing = 0x100;
  m.stride = 0x0F;
  m.body_timer = 3;
  m.delta = 10;
  stride_table[0x100] = 0x30;
  stride_table[0x101] = 0x20;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.vel_x == 0x18);
  m = loco();
  m.target_x = 1;
  m.facing = 0;
  m.move_attr = 0x00080000;
  m.body_timer = 3;
  m.delta = 10;
  stride_table[0x101] = 0x40;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.vel_x == 0x20);
}

static void test_step_speed_gate(void) {
  fifa96_action_locomotion m = loco();
  m.target_x = 0x100;
  m.facing = 0x100;
  m.move_attr = 0x00080000;
  m.stride = 0x0F;
  m.vel_x = 0x20;
  m.vel_z = 0x30;
  m.speed = 0x7B;
  m.body_timer = 3;
  m.delta = 0;
  stride_table[0x101] = 0x20;
  stride_table[0x201] = 0x30;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.vel_x == 0x20 && m.vel_z == 0x30 && m.speed == 0x7B && m.pos_x == 0);
}

static void test_step_body_timer_boundary(void) {
  fifa96_action_locomotion m = loco();
  m.target_x = 1;
  m.body_timer = 2;
  m.delta = 0;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.body_timer == 2 && m.vel_x == 0);
  m = loco();
  m.target_x = 1;
  m.body_timer = 2;
  m.delta = 1;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.body_timer == 0);
  m = loco();
  m.target_x = 1;
  m.body_timer = 2;
  m.delta = 1;
  m.stride_rate = 0x20000;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.body_timer == 3);
  m = loco();
  m.target_x = 1;
  m.body_timer = 2;
  m.delta = 2;
  m.stride_rate = 0x20000;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.body_timer == 0);
  m = loco();
  m.body_timer = 100;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.body_timer == 100);
}

static void test_step_integration(void) {
  fifa96_action_locomotion m = loco();
  m.pos_x = 0x1000;
  m.pos_z = -0x1000;
  m.target_x = 0x1000;
  m.target_z = -0x1000;
  m.speed = 1;
  m.vel_x = 3;
  m.vel_z = -4;
  m.body_timer = 0;
  m.delta = 5;
  stride_table[0] = 3;
  stride_table[0x100] = -4;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.pos_x == 0x1000 + 15 && m.pos_z == -0x1000 - 20);
  assert(m.speed == 1);
}

static void test_restart_target(void) {
  int32_t tx = 0x7B;
  int32_t tz = 0x7B;
  assert(fifa96_action_locomotion_restart_target(-0x1234, &tx, &tz) == FIFA96_OK);
  assert(tx == 0x1234 && tz == 0);
  assert(fifa96_action_locomotion_restart_target(0x100, &tx, &tz) == FIFA96_OK);
  assert(tx == -0x100 && tz == 0);
  assert(fifa96_action_locomotion_restart_target(0, NULL, &tz) == ACTION_INVALID);
  assert(fifa96_action_locomotion_restart_target(0, &tx, NULL) == ACTION_INVALID);
}

static void test_hold(void) {
  fifa96_action_vec3 pos = {0x11, 0x22, 0x33};
  fifa96_action_vec3 out = {-1, -1, -1};
  uint8_t held = 0xAA;
  assert(fifa96_action_locomotion_hold(0x10, &pos, &out, &held) == FIFA96_OK);
  assert(held == 1 && out.x == 0x11 && out.y == 0x22 && out.z == 0x33);
  out.x = out.y = out.z = -1;
  held = 0;
  assert(fifa96_action_locomotion_hold(0x11, &pos, &out, &held) == FIFA96_OK);
  assert(held == 1 && out.x == 0x11);
  assert(fifa96_action_locomotion_hold(0x12, &pos, &out, &held) == FIFA96_OK);
  assert(held == 1 && out.z == 0x33);
  out.x = out.y = out.z = -1;
  held = 0xAA;
  assert(fifa96_action_locomotion_hold(0x0F, &pos, &out, &held) == FIFA96_OK);
  assert(held == 0 && out.x == -1 && out.y == -1 && out.z == -1);
  assert(fifa96_action_locomotion_hold(0x10, NULL, &out, &held) == ACTION_INVALID);
  assert(fifa96_action_locomotion_hold(0x10, &pos, NULL, &held) == ACTION_INVALID);
  assert(fifa96_action_locomotion_hold(0x10, &pos, &out, NULL) == ACTION_INVALID);
}

static void test_camera_lead(void) {
  fifa96_action_vec3 out;
  memset(&out, 0, sizeof(out));
  assert(fifa96_action_locomotion_camera_lead(0x100, 0x200, 0x300, 3, -4, &out) == FIFA96_OK);
  assert(out.x == 0x10C && out.y == 0x200 && out.z == 0x2F0);
  assert(fifa96_action_locomotion_camera_lead(0, 0, 0, -1, -1, &out) == FIFA96_OK);
  assert(out.x == -4 && out.z == -4);
  assert(fifa96_action_locomotion_camera_lead(0, 0, 0, 0, 0, NULL) == ACTION_INVALID);
}

static void test_clamp_placement(void) {
  int32_t z = 100;
  assert(fifa96_action_locomotion_clamp_placement(&z, 200, 150, 300, 0, 0, 1) == FIFA96_OK);
  assert(z == 150);
  z = 100;
  assert(fifa96_action_locomotion_clamp_placement(&z, 50, 0, 300, 0, 0, 1) == FIFA96_OK);
  assert(z == 100);
  z = 100;
  assert(fifa96_action_locomotion_clamp_placement(&z, 50, 0, 80, 0, 0, 1) == FIFA96_OK);
  assert(z == 80);
  z = 100;
  assert(fifa96_action_locomotion_clamp_placement(&z, 50, 200, 300, 0, 0, 1) == FIFA96_OK);
  assert(z == 100);
  z = 100;
  assert(fifa96_action_locomotion_clamp_placement(&z, 0, 0, 0x7FFF, 50, 0, 0) == FIFA96_OK);
  assert(z == -46);
  z = 50;
  assert(fifa96_action_locomotion_clamp_placement(&z, 0, 0, 0x7FFF, 50, 0, 0) == FIFA96_OK);
  assert(z == 50);
  z = 50;
  assert(fifa96_action_locomotion_clamp_placement(&z, 0, 0, 0x7FFF, 100, 1, 0) == FIFA96_OK);
  assert(z == 0xC4);
  z = 100;
  assert(fifa96_action_locomotion_clamp_placement(&z, 0, 0, 0x7FFF, 50, 1, 0) == FIFA96_OK);
  assert(z == 100);
  z = 100;
  assert(fifa96_action_locomotion_clamp_placement(&z, 0, 0, 0x7FFF, 50, 0, 1) == FIFA96_OK);
  assert(z == 100);
  assert(fifa96_action_locomotion_clamp_placement(NULL, 0, 0, 0, 0, 0, 0) == ACTION_INVALID);
}

int main(void) {
  test_step_target_and_distance();
  test_step_turn_clamp();
  test_step_angle_wrap();
  test_step_direct_face();
  test_step_stride_ramp();
  test_step_speed_gate();
  test_step_body_timer_boundary();
  test_step_integration();
  test_restart_target();
  test_hold();
  test_camera_lead();
  test_clamp_placement();
  puts("test_action_locomotion: all assertions passed");
  return 0;
}
