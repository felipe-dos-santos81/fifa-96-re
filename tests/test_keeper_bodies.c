// tests/test_keeper_bodies.c — FU-79 keeper action-family bodies (docs/ghidra/FU79_keeper_bodies.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_keeper.h"

#define BODY_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

_Static_assert(offsetof(fifa96_keeper_point, x) == 0, "x");
_Static_assert(offsetof(fifa96_keeper_point, y) == 4, "y");
_Static_assert(offsetof(fifa96_keeper_point, z) == 8, "z");
_Static_assert(offsetof(fifa96_keeper_vec, distance) == 0, "distance");
_Static_assert(offsetof(fifa96_keeper_vec, dx) == 2, "dx");
_Static_assert(offsetof(fifa96_keeper_vec, dz) == 4, "dz");
_Static_assert(offsetof(fifa96_keeper_dive, target) == 0, "target");
_Static_assert(offsetof(fifa96_keeper_dive, install_code) == 12, "install_code");
_Static_assert(offsetof(fifa96_keeper_dive, invoke) == 13, "invoke");
_Static_assert(offsetof(fifa96_keeper_arm_out, stage) == 0, "stage");
_Static_assert(offsetof(fifa96_keeper_arm_out, event_code) == 1, "event_code");
_Static_assert(offsetof(fifa96_keeper_arm_out, install_code) == 2, "install_code");
_Static_assert(offsetof(fifa96_keeper_arm_out, invoke) == 3, "invoke");
_Static_assert(offsetof(fifa96_keeper_arm_out, copy_pos) == 4, "copy_pos");
_Static_assert(offsetof(fifa96_keeper_arm_out, run_handler) == 5, "run_handler");
_Static_assert(offsetof(fifa96_keeper_arm_out, reset) == 6, "reset");
_Static_assert(offsetof(fifa96_keeper_arm_out, flag_9e) == 7, "flag_9e");
_Static_assert(offsetof(fifa96_keeper_rep_a_out, install_code) == 0, "install_code");
_Static_assert(offsetof(fifa96_keeper_rep_a_out, reset) == 1, "reset");
_Static_assert(offsetof(fifa96_keeper_rep_a_out, stage_advance) == 2, "stage_advance");
_Static_assert(offsetof(fifa96_keeper_rep_a_out, flag_9e) == 3, "flag_9e");
_Static_assert(offsetof(fifa96_keeper_input, has_slot) == 0, "has_slot");
_Static_assert(offsetof(fifa96_keeper_input, phase) == 1, "phase");
_Static_assert(offsetof(fifa96_keeper_input, human_phase) == 2, "human_phase");
_Static_assert(offsetof(fifa96_keeper_input, event_flag) == 3, "event_flag");
_Static_assert(offsetof(fifa96_keeper_input, team_side) == 4, "team_side");
_Static_assert(offsetof(fifa96_keeper_input, type8) == 5, "type8");
_Static_assert(offsetof(fifa96_keeper_input, type_gate) == 6, "type_gate");
_Static_assert(offsetof(fifa96_keeper_input, is_actor) == 7, "is_actor");
_Static_assert(offsetof(fifa96_keeper_input, lane) == 8, "lane");
_Static_assert(offsetof(fifa96_keeper_input, cam_x) == 12, "cam_x");
_Static_assert(offsetof(fifa96_keeper_input, cam_y) == 16, "cam_y");
_Static_assert(offsetof(fifa96_keeper_input, cam_z) == 20, "cam_z");
_Static_assert(offsetof(fifa96_keeper_input, pos_x) == 24, "pos_x");
_Static_assert(offsetof(fifa96_keeper_input, pos_z) == 28, "pos_z");
_Static_assert(offsetof(fifa96_keeper_input, slot_dir_x) == 32, "slot_dir_x");
_Static_assert(offsetof(fifa96_keeper_input, slot_dir_z) == 33, "slot_dir_z");
_Static_assert(offsetof(fifa96_keeper_decision, install) == 0, "install");
_Static_assert(offsetof(fifa96_keeper_decision, invoke) == 1, "invoke");
_Static_assert(offsetof(fifa96_keeper_decision, copy_cam) == 2, "copy_cam");
_Static_assert(offsetof(fifa96_keeper_decision, copy_pos) == 3, "copy_pos");
_Static_assert(offsetof(fifa96_keeper_decision, clear_c5c) == 4, "clear_c5c");
_Static_assert(offsetof(fifa96_keeper_decision, target_x) == 8, "target_x");
_Static_assert(offsetof(fifa96_keeper_decision, target_y) == 12, "target_y");
_Static_assert(offsetof(fifa96_keeper_decision, target_z) == 16, "target_z");

static fifa96_keeper_point point(int32_t x, int32_t y, int32_t z) {
  fifa96_keeper_point p;
  p.x = x;
  p.y = y;
  p.z = z;
  return p;
}

static int16_t dist16(int16_t x, int16_t z) {
  int16_t d = -1;
  assert(fifa96_keeper_distance(x, z, &d) == FIFA96_OK);
  return d;
}

static void test_distance(void) {
  assert(dist16(0, 0) == 0);
  assert(dist16(7, 0) == 7);
  assert(dist16(0, -9) == 9);
  assert(dist16(4, 4) == 5);
  assert(dist16(3, 5) == 5);
  assert(dist16(5, 3) == 5);
  assert(dist16(8, 8) == 11);
  assert(dist16(-8, -8) == 11);
  assert(dist16(0x4000, 0) == 0x4000);
  assert(fifa96_keeper_distance(0, 0, NULL) == BODY_INVALID);
}

static void test_vec_from_delta(void) {
  fifa96_keeper_vec v;
  fifa96_keeper_point from = point(10, 0, 20);
  fifa96_keeper_point to = point(4, 0, 28);
  assert(fifa96_keeper_vec_from_delta(&from, &to, &v) == FIFA96_OK);
  assert(v.dx == -6 && v.dz == 8 && v.distance == 10);
  assert(fifa96_keeper_vec_from_delta(&from, &from, &v) == FIFA96_OK);
  assert(v.dx == 0 && v.dz == 0 && v.distance == 0);
  from = point(0, 0, 0x7FFF);
  to = point(0, 0, -0x8000);
  assert(fifa96_keeper_vec_from_delta(&from, &to, &v) == FIFA96_OK);
  assert(v.dx == 0 && v.dz == 1 && v.distance == 1);
  assert(fifa96_keeper_vec_from_delta(NULL, &to, &v) == BODY_INVALID);
  assert(fifa96_keeper_vec_from_delta(&from, NULL, &v) == BODY_INVALID);
  assert(fifa96_keeper_vec_from_delta(&from, &to, NULL) == BODY_INVALID);
}

static void test_clear_vector(void) {
  fifa96_keeper_vec v;
  assert(fifa96_keeper_clear_vector(0, 0, 0, 0, &v) == FIFA96_OK);
  assert(v.dx == -0x3C0 && v.dz == 0x3C0 && v.distance == 0x528);
  assert(fifa96_keeper_clear_vector(0x77F, 0, 0, 0, &v) == FIFA96_OK);
  assert(v.dx == 0x3BF);
  assert(fifa96_keeper_clear_vector(0x780, 0, 0, 0, &v) == FIFA96_OK);
  assert(v.dx == -0x3C0);
  assert(fifa96_keeper_clear_vector(0, 0x7FF, 0x40, 0, &v) == FIFA96_OK);
  assert(v.dz == 0xBBF);
  assert(fifa96_keeper_clear_vector(0, 0, 0, 1, &v) == FIFA96_OK);
  assert(v.dz == -0x3C0);
  assert(fifa96_keeper_clear_vector(0, 0x7FF, 0x40, 1, &v) == FIFA96_OK);
  assert(v.dz == -0xBBF);
  assert(fifa96_keeper_clear_vector(0, 0x77D, -1, 0, &v) == FIFA96_OK);
  assert(v.dz == 0xB3D);
  assert(fifa96_keeper_clear_vector(0, 0, 0, 0, NULL) == BODY_INVALID);
}

static void test_claim_place(void) {
  fifa96_keeper_point pos = point(100, 20, 200);
  fifa96_keeper_point place = point(-1, -1, -1);
  uint8_t helper = 0xAA;
  uint8_t claimed = 0xAA;
  assert(fifa96_keeper_claim_place(&pos, -3, 2, 1, 0, 1, &place, &helper, &claimed) == FIFA96_OK);
  assert(place.x == 52 && place.y == 76 && place.z == 232);
  assert(helper == 0 && claimed == 1);
  place = point(-1, -1, -1);
  assert(fifa96_keeper_claim_place(&pos, 1, 1, 2, 0, 0, &place, &helper, &claimed) == FIFA96_OK);
  assert(helper == 1 && claimed == 1 && place.x == 116 && place.z == 216);
  place = point(-1, -1, -1);
  assert(fifa96_keeper_claim_place(&pos, 1, 1, 3, 0, 0, &place, &helper, &claimed) == FIFA96_OK);
  assert(helper == 1 && claimed == 0 && place.x == -1 && place.y == -1 && place.z == -1);
  assert(fifa96_keeper_claim_place(&pos, 1, 1, 1, 1, 1, &place, &helper, &claimed) == FIFA96_OK);
  assert(helper == 0 && claimed == 0);
  assert(fifa96_keeper_claim_place(&pos, 1, 1, 6, 0, 0, &place, &helper, &claimed) == FIFA96_OK);
  assert(helper == 0 && claimed == 0);
  assert(fifa96_keeper_claim_place(&pos, -128, -128, 0, 0, 0, &place, &helper, &claimed) == FIFA96_OK);
  assert(place.x == 100 - 2048 && place.z == 200 - 2048 && claimed == 1);
  assert(fifa96_keeper_claim_place(NULL, 0, 0, 0, 0, 1, &place, &helper, &claimed) == BODY_INVALID);
  assert(fifa96_keeper_claim_place(&pos, 0, 0, 0, 0, 1, NULL, &helper, &claimed) == BODY_INVALID);
  assert(fifa96_keeper_claim_place(&pos, 0, 0, 0, 0, 1, &place, NULL, &claimed) == BODY_INVALID);
  assert(fifa96_keeper_claim_place(&pos, 0, 0, 0, 0, 1, &place, &helper, NULL) == BODY_INVALID);
}

static void test_hold_track(void) {
  assert(fifa96_keeper_hold_track(5, 4) == 1);
  assert(fifa96_keeper_hold_track(5, 5) == 0);
  assert(fifa96_keeper_hold_track(5, 6) == 0);
  assert(fifa96_keeper_hold_track(-5, -6) == 1);
  assert(fifa96_keeper_hold_track(-1, INT16_MIN) == 1);
  assert(fifa96_keeper_hold_track(INT16_MAX, INT16_MIN) == 1);
}

static void test_hold_guard(void) {
  fifa96_keeper_point cam = point(1, 2, 0xAE0);
  fifa96_keeper_point out;
  assert(fifa96_keeper_hold_guard(&cam, 0, &out) == FIFA96_OK);
  assert(out.x == 1 && out.y == 2 && out.z == 0xAE0);
  cam = point(3, 4, 0xAE1);
  assert(fifa96_keeper_hold_guard(&cam, 0, &out) == FIFA96_OK);
  assert(out.x == 3 && out.y == 4 && out.z == -0xAE0);
  assert(fifa96_keeper_hold_guard(&cam, 1, &out) == FIFA96_OK);
  assert(out.z == 0xAE0);
  cam = point(3, 4, -0xAE1);
  assert(fifa96_keeper_hold_guard(&cam, 0, &out) == FIFA96_OK);
  assert(out.z == -0xAE0);
  assert(fifa96_keeper_hold_guard(&cam, 1, &out) == FIFA96_OK);
  assert(out.z == 0xAE0);
  cam = point(3, 4, -0xAE0);
  assert(fifa96_keeper_hold_guard(&cam, 0, &out) == FIFA96_OK);
  assert(out.z == -0xAE0);
  assert(fifa96_keeper_hold_guard(NULL, 0, &out) == BODY_INVALID);
  assert(fifa96_keeper_hold_guard(&cam, 0, NULL) == BODY_INVALID);
}

static void test_hold_intercept(void) {
  fifa96_keeper_point cam = point(1000, 0, 500);
  fifa96_keeper_point out;
  assert(fifa96_keeper_hold_intercept(&cam, 0x200000, 3, -4, &out) == FIFA96_OK);
  assert(out.x == 1006 && out.y == 0 && out.z == 492);
  assert(fifa96_keeper_hold_intercept(&cam, 0x1FFFFF, 3, -4, &out) == FIFA96_OK);
  assert(out.x == 1003 && out.z == 496);
  assert(fifa96_keeper_hold_intercept(&cam, 0, 3, -4, &out) == FIFA96_OK);
  assert(out.x == 1000 && out.z == 500);
  assert(fifa96_keeper_hold_intercept(&cam, -0x100000, 3, -4, &out) == FIFA96_OK);
  assert(out.x == 997 && out.z == 504);
  assert(fifa96_keeper_hold_intercept(NULL, 0, 0, 0, &out) == BODY_INVALID);
  assert(fifa96_keeper_hold_intercept(&cam, 0, 0, 0, NULL) == BODY_INVALID);
}

static void test_guard_clamp(void) {
  fifa96_keeper_point p = point(0x400, 7, 0x800);
  assert(fifa96_keeper_guard_clamp(&p, 0) == FIFA96_OK);
  assert(p.x == 0x390 && p.y == 7 && p.z == -0x7E0);
  p = point(-0x400, 7, -0xB00);
  assert(fifa96_keeper_guard_clamp(&p, 0) == FIFA96_OK);
  assert(p.x == -0x390 && p.z == -0xAE0);
  p = point(0, 0, 0);
  assert(fifa96_keeper_guard_clamp(&p, 1) == FIFA96_OK);
  assert(p.z == 0x7E0);
  p = point(0, 0, 0xAE1);
  assert(fifa96_keeper_guard_clamp(&p, 1) == FIFA96_OK);
  assert(p.z == 0xAE0);
  p = point(0, 0, 0x7DF);
  assert(fifa96_keeper_guard_clamp(&p, 1) == FIFA96_OK);
  assert(p.z == 0x7E0);
  p = point(0x100, 0, 0xA00);
  assert(fifa96_keeper_guard_clamp(&p, 1) == FIFA96_OK);
  assert(p.x == 0x100 && p.z == 0xA00);
  assert(fifa96_keeper_guard_clamp(NULL, 0) == BODY_INVALID);
}

static void test_dive_target(void) {
  fifa96_keeper_point pos = point(0, 0, 0);
  fifa96_keeper_dive d;
  assert(fifa96_keeper_dive_target(&pos, 1, 0x64, 0, 5, 0, 0, &d) == FIFA96_OK);
  assert(d.target.x == 0x30 && d.target.y == 0x20 && d.target.z == 0x30);
  assert(d.install_code == 0x1B && d.invoke == 1);
  assert(fifa96_keeper_dive_target(&pos, 1, 0x64, 0, -5, 0, 0, &d) == FIFA96_OK);
  assert(d.target.x == -0x30);
  assert(fifa96_keeper_dive_target(&pos, 0x19, 0x64, 1, 5, 0x7F, 0x2F, &d) == FIFA96_OK);
  assert(d.target.x == 0x6F && d.target.y == 0x4F);
  assert(fifa96_keeper_dive_target(&pos, 0x19, 0x64, 2, 5, 0, 0x30, &d) == FIFA96_OK);
  assert(d.target.x == -0x30 && d.target.y == 0x20);
  pos = point(0, 0, 5);
  assert(fifa96_keeper_dive_target(&pos, 1, 0x64, 0, 1, 0, 0, &d) == FIFA96_OK);
  assert(d.target.z == -0x2B);
  pos = point(0, 0, -5);
  assert(fifa96_keeper_dive_target(&pos, 1, 0x64, 0, 1, 0, 0, &d) == FIFA96_OK);
  assert(d.target.z == 0x2B);
  assert(fifa96_keeper_dive_target(&pos, 1, 0, 0, 1, 0, 0, &d) == BODY_INVALID);
  assert(fifa96_keeper_dive_target(NULL, 1, 1, 0, 1, 0, 0, &d) == BODY_INVALID);
  assert(fifa96_keeper_dive_target(&pos, 1, 1, 0, 1, 0, 0, NULL) == BODY_INVALID);
}

static void test_arm_step(void) {
  fifa96_keeper_arm_out out;
  assert(fifa96_keeper_arm_step(0, 0, 0, 0, 0, 0, 0, 0, &out) == FIFA96_OK);
  assert(out.stage == 0 && out.event_code == 0 && out.flag_9e == 1);
  assert(fifa96_keeper_arm_step(0, 1, 0, 0, 0, 0, 0, 0, &out) == FIFA96_OK);
  assert(out.stage == 1 && out.event_code == 0x28 && out.flag_9e == 1);
  assert(fifa96_keeper_arm_step(1, 1, 0, 1, 1, 0, 0, 0, &out) == FIFA96_OK);
  assert(out.stage == 1 && out.copy_pos == 0);
  assert(fifa96_keeper_arm_step(1, 1, 1, 0, 1, 0, 0, 0, &out) == FIFA96_OK);
  assert(out.stage == 1);
  assert(fifa96_keeper_arm_step(1, 1, 1, 1, 0, 0, 0, 0, &out) == FIFA96_OK);
  assert(out.stage == 2 && out.install_code == 0x1B && out.invoke == 1 && out.copy_pos == 0);
  assert(fifa96_keeper_arm_step(1, 1, 1, 1, 1, 0, 0, 0, &out) == FIFA96_OK);
  assert(out.stage == 3 && out.copy_pos == 1 && out.install_code == 0);
  assert(fifa96_keeper_arm_step(2, 0, 0, 0, 1, 1, 0x1F, 0, &out) == FIFA96_OK);
  assert(out.stage == 3 && out.copy_pos == 1 && out.run_handler == 1 && out.reset == 1);
  assert(fifa96_keeper_arm_step(2, 0, 0, 0, 1, 0, 0x1E, 0, &out) == FIFA96_OK);
  assert(out.run_handler == 0 && out.reset == 0);
  assert(fifa96_keeper_arm_step(3, 0, 0, 0, 1, 0, 0x1F, 1, &out) == FIFA96_OK);
  assert(out.stage == 3 && out.copy_pos == 1 && out.reset == 0);
  assert(fifa96_keeper_arm_step(4, 1, 1, 1, 1, 1, 0x1F, 0, &out) == FIFA96_OK);
  assert(out.stage == 4 && out.copy_pos == 0 && out.event_code == 0 && out.flag_9e == 0);
  assert(fifa96_keeper_arm_step(0, 0, 0, 0, 0, 0, 0, 0, NULL) == BODY_INVALID);
}

static void test_reposition_a_gate(void) {
  fifa96_keeper_rep_a_out out;
  assert(fifa96_keeper_reposition_a_gate(1, &out) == FIFA96_OK);
  assert(out.install_code == 4 && out.reset == 1 && out.stage_advance == 0 && out.flag_9e == 0);
  assert(fifa96_keeper_reposition_a_gate(0, &out) == FIFA96_OK);
  assert(out.install_code == 0 && out.reset == 0 && out.stage_advance == 2 && out.flag_9e == 1);
  assert(fifa96_keeper_reposition_a_gate(0, NULL) == BODY_INVALID);
}

static void test_reposition_b_finish(void) {
  uint8_t install = 0xAA;
  uint8_t notify = 0xAA;
  assert(fifa96_keeper_reposition_b_finish(0, 0, &install, &notify) == FIFA96_OK);
  assert(install == 0x19 && notify == 0);
  assert(fifa96_keeper_reposition_b_finish(1, 1, &install, &notify) == FIFA96_OK);
  assert(install == 0 && notify == 0);
  assert(fifa96_keeper_reposition_b_finish(1, 0, &install, &notify) == FIFA96_OK);
  assert(install == 0 && notify == 5);
  assert(fifa96_keeper_reposition_b_finish(0, 0, NULL, &notify) == BODY_INVALID);
  assert(fifa96_keeper_reposition_b_finish(0, 0, &install, NULL) == BODY_INVALID);
}

static void test_lunge_track(void) {
  fifa96_keeper_vec delta;
  fifa96_keeper_point pos = point(100, 5, 200);
  uint8_t on_target = 0xAA;
  uint8_t event_code = 0xAA;
  int16_t steer_x = 0;
  int16_t steer_z = 0;
  delta.distance = 10;
  delta.dx = 8;
  delta.dz = -6;
  assert(fifa96_keeper_lunge_track(&delta, 0x54, &pos, &on_target, &steer_x, &steer_z,
                                   &event_code) == FIFA96_OK);
  assert(on_target == 1 && pos.x == 104 && pos.y == 5 && pos.z == 197);
  assert(event_code == 0x3C && steer_x == 0x80 && steer_z == 0x40);
  pos = point(100, 5, 200);
  assert(fifa96_keeper_lunge_track(&delta, 0x55, &pos, &on_target, &steer_x, &steer_z,
                                   &event_code) == FIFA96_OK);
  assert(on_target == 0 && pos.x == 100 && pos.z == 200);
  assert(event_code == 0 && steer_x == 0 && steer_z == 0);
  delta.dx = -7;
  delta.dz = 7;
  assert(fifa96_keeper_lunge_track(&delta, -0x54, &pos, &on_target, &steer_x, &steer_z,
                                   &event_code) == FIFA96_OK);
  assert(on_target == 1 && pos.x == 97 && pos.z == 203);
  assert(fifa96_keeper_lunge_track(NULL, 0, &pos, &on_target, &steer_x, &steer_z,
                                   &event_code) == BODY_INVALID);
  assert(fifa96_keeper_lunge_track(&delta, 0, NULL, &on_target, &steer_x, &steer_z,
                                   &event_code) == BODY_INVALID);
  assert(fifa96_keeper_lunge_track(&delta, 0, &pos, NULL, &steer_x, &steer_z,
                                   &event_code) == BODY_INVALID);
  assert(fifa96_keeper_lunge_track(&delta, 0, &pos, &on_target, NULL, &steer_z,
                                   &event_code) == BODY_INVALID);
  assert(fifa96_keeper_lunge_track(&delta, 0, &pos, &on_target, &steer_x, NULL,
                                   &event_code) == BODY_INVALID);
  assert(fifa96_keeper_lunge_track(&delta, 0, &pos, &on_target, &steer_x, &steer_z,
                                   NULL) == BODY_INVALID);
}

static void test_hold_fallback(void) {
  fifa96_keeper_point cam = point(800, 7, 100);
  fifa96_keeper_point out;
  assert(fifa96_keeper_hold_fallback(&cam, 0, 0, 0x780, 0x9F1, 1, 0, 0, &out) == FIFA96_OK);
  assert(out.x == 175 && out.y == 0 && out.z == -0x9F0);
  assert(fifa96_keeper_hold_fallback(&cam, 1, 0, 0x780, 0x9F1, 1, 0, 0, &out) == FIFA96_OK);
  assert(out.z == 0x9F0);
  assert(fifa96_keeper_hold_fallback(&cam, 0, 0, 0, 0x9F1, 0, 0, 0, &out) == FIFA96_OK);
  assert(out.z == -0x9F0);
  assert(fifa96_keeper_hold_fallback(&cam, 1, 0, 0, 0x9F0, 1, 0, 0, &out) == FIFA96_OK);
  assert(out.x == 175 && out.z == 0x9F0);
  assert(fifa96_keeper_hold_fallback(&cam, 1, 0, 0x77F, 0x9F1, 0, 0, 0, &out) == FIFA96_OK);
  assert(out.x == 175);
  cam = point(-800, 7, 100);
  assert(fifa96_keeper_hold_fallback(&cam, 0, 0, 0, 0, 0, 0, 0, &out) == FIFA96_OK);
  assert(out.x == -175);
  cam = point(0x8000, 0, 0);
  assert(fifa96_keeper_hold_fallback(&cam, 0, 0, 0, 0, 0, 0, 0, &out) == FIFA96_OK);
  assert(out.x == 0x1C00);
  assert(fifa96_keeper_hold_fallback(&cam, 1, 0x10000000, 0, 0, 0, 0, 0, &out) == FIFA96_OK);
  assert(out.z == 0xAE0);
  assert(fifa96_keeper_hold_fallback(&cam, 0, (int32_t)0xF0000000, 0, 0, 0, 0, 0,
                                     &out) == FIFA96_OK);
  assert(out.z == -0xAE0);
  assert(fifa96_keeper_hold_fallback(&cam, 0, (int32_t)0xFFF00000, 0, 0, 0, 0, 0,
                                     &out) == FIFA96_OK);
  assert(out.z == -0x9F1);

  cam = point(10, 0, 0x9D0);
  assert(fifa96_keeper_hold_fallback(&cam, 1, 0, 0x77F, 0x9F1, 1, 0x00010000, 0x00030000,
                                     &out) == FIFA96_OK);
  assert(out.x == 106 && out.y == 0 && out.z == 0x9F0);
  cam = point(0x1000, 0, -0x10);
  assert(fifa96_keeper_hold_fallback(&cam, 1, 0, 0x77F, 0x9F1, 1, 0x00010000, 0x00010000,
                                     &out) == FIFA96_OK);
  assert(out.x == 0xC0);
  cam = point(100, 0, -0x10);
  assert(fifa96_keeper_hold_fallback(&cam, 1, 0, 0x77F, 0x9F1, 1, 0x00010000,
                                     (int32_t)0xFFFF0000, &out) == FIFA96_OK);
  assert(out.x == -0xC0);
  cam = point(0, 0, 0);
  assert(fifa96_keeper_hold_fallback(&cam, 1, 0, 0x77F, 0x9F1, 1, 0x0000FFFF, 0,
                                     &out) == BODY_INVALID);
  assert(fifa96_keeper_hold_fallback(NULL, 0, 0, 0, 0, 0, 0, 0, &out) == BODY_INVALID);
  assert(fifa96_keeper_hold_fallback(&cam, 0, 0, 0, 0, 0, 0, 0, NULL) == BODY_INVALID);
}

static void test_arm_camera(void) {
  fifa96_keeper_point target = point(-1, -1, -1);
  uint8_t hook = 0xAA;
  assert(fifa96_keeper_arm_camera(0, 0, 0, &target, &hook) == FIFA96_OK);
  assert(target.x == 0 && target.y == 0 && target.z == -0xB10 && hook == 1);
  assert(fifa96_keeper_arm_camera(0, 0, 0x10, &target, &hook) == FIFA96_OK);
  assert(target.z == 0xB10 && hook == 1);
  assert(fifa96_keeper_arm_camera(0, 1, 0, &target, &hook) == FIFA96_OK);
  assert(target.z == 0xB10 && hook == 1);
  assert(fifa96_keeper_arm_camera(1, 0, 0, &target, &hook) == FIFA96_OK);
  assert(target.z == -0xB10 && hook == 0);
  target = point(-1, -1, -1);
  assert(fifa96_keeper_arm_camera(2, 0, 0, &target, &hook) == FIFA96_OK);
  assert(target.x == -1 && target.y == -1 && target.z == -1 && hook == 0);
  assert(fifa96_keeper_arm_camera(0, 0, 0, NULL, &hook) == BODY_INVALID);
  assert(fifa96_keeper_arm_camera(0, 0, 0, &target, NULL) == BODY_INVALID);
}

static fifa96_keeper_input input_slot(uint8_t phase, uint8_t side) {
  fifa96_keeper_input in;
  in.has_slot = 1;
  in.phase = phase;
  in.human_phase = 0;
  in.event_flag = 0;
  in.team_side = side;
  in.type8 = 0x19;
  in.type_gate = 1;
  in.is_actor = 0;
  in.lane = 0;
  in.cam_x = 0;
  in.cam_y = 0;
  in.cam_z = 0;
  in.pos_x = 0;
  in.pos_z = 0;
  in.slot_dir_x = 0;
  in.slot_dir_z = 0;
  return in;
}

static void test_input_decide_gates(void) {
  fifa96_keeper_decision d;
  fifa96_keeper_input in = input_slot(2, 0);
  in.has_slot = 0;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0 && d.invoke == 0 && d.copy_cam == 0 && d.copy_pos == 0);
  in = input_slot(2, 0);
  in.lane = 0x61;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0);
  in.lane = 0x60;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 4 && d.invoke == 1 && d.target_x == 0 && d.target_z == 0);
  in = input_slot(2, 0);
  in.cam_x = 0x421;
  in.lane = 0x60;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 4);
  in = input_slot(5, 0);
  in.lane = 0x60;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0);
  in = input_slot(5, 0);
  in.human_phase = 1;
  in.event_flag = 1;
  in.lane = 0x40;
  in.cam_x = 0x111;
  in.cam_y = 0x222;
  in.cam_z = 0x333;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0x19 && d.invoke == 1 && d.copy_cam == 1);
  assert(d.target_x == 0x111 && d.target_y == 0x222 && d.target_z == 0x333);
  in.human_phase = 0;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0);
  in.human_phase = 1;
  in.event_flag = 0;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0);
}

static void test_input_decide_camera_box(void) {
  fifa96_keeper_decision d;
  fifa96_keeper_input in = input_slot(2, 0);
  in.cam_x = 0x420;
  in.cam_z = -0x7B0;
  in.lane = 0x40;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0x19 && d.copy_cam == 1);
  in.cam_z = -0x7AF;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 4);
  in = input_slot(2, 1);
  in.cam_z = 0x7B0;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0x19);
  in.cam_z = 0x7AF;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 4);
  in = input_slot(2, 0);
  in.cam_x = -0x420;
  in.cam_z = -0x7B0;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0x19);
  in.cam_x = -0x421;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 4);
}

static void test_input_decide_type_and_actor_gates(void) {
  fifa96_keeper_decision d;
  fifa96_keeper_input in = input_slot(2, 0);
  in.cam_z = -0x7B0;
  in.lane = 0x40;
  in.type8 = 0x0A;
  in.type_gate = 0;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0);
  in.type_gate = 1;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0x19);
  in.type8 = 0x1F;
  in.type_gate = 0;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0x19);
  in.type8 = 0x19;
  in.type_gate = 1;
  in.is_actor = 1;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0);
  in.is_actor = 0;
  in.type8 = 5;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0);
}

static void test_input_decide_slot_direction(void) {
  fifa96_keeper_decision d;
  fifa96_keeper_input in = input_slot(2, 0);
  in.cam_z = -0x7B0;
  in.lane = 0x41;
  in.pos_x = 0x100;
  in.pos_z = 0x200;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0x1C && d.copy_pos == 1 && d.copy_cam == 0);
  assert(d.target_x == 0x100 && d.target_y == 0xA0 && d.target_z == 0x200);
  in.slot_dir_z = 1;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0x1C && d.copy_pos == 0 && d.clear_c5c == 1);
  assert(d.target_x == 0x100 && d.target_y == 0x40 && d.target_z == 0x270);
  in.slot_dir_z = -1;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.install == 0x1C);
  assert(d.target_z == 0x190);
  in.pos_z = 0xAA0;
  in.slot_dir_z = 1;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.target_z == 0xAF0 && d.install == 0x1B);
  in.slot_dir_x = -2;
  in.pos_x = 0x100;
  in.pos_z = 0x200;
  in.slot_dir_z = 0;
  assert(fifa96_keeper_input_decide(&in, &d) == FIFA96_OK);
  assert(d.target_x == 0x20 && d.install == 0x1C);
  assert(fifa96_keeper_input_decide(NULL, &d) == BODY_INVALID);
  assert(fifa96_keeper_input_decide(&in, NULL) == BODY_INVALID);
}

int main(void) {
  test_distance();
  test_vec_from_delta();
  test_clear_vector();
  test_claim_place();
  test_hold_track();
  test_hold_guard();
  test_hold_intercept();
  test_guard_clamp();
  test_dive_target();
  test_arm_step();
  test_reposition_a_gate();
  test_reposition_b_finish();
  test_lunge_track();
  test_hold_fallback();
  test_arm_camera();
  test_input_decide_gates();
  test_input_decide_camera_box();
  test_input_decide_type_and_actor_gates();
  test_input_decide_slot_direction();
  puts("test_keeper_bodies: all assertions passed");
  return 0;
}
