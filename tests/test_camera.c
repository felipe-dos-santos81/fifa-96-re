// tests/test_camera.c — FU-71 match-camera follow port (docs/ghidra/FU71_camera_track.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_entity_update.h"
#include "fifa96_loader/fifa96_camera.h"

static fifa96_camera fresh_camera(void) {
  fifa96_camera cam;
  memset(&cam, 0, sizeof cam);
  return cam;
}

static void test_init_fields(void) {
  fifa96_camera cam;
  memset(&cam, 0xAA, sizeof cam);
  assert(fifa96_camera_init(&cam, 10, -20, 30) == FIFA96_OK);
  assert(cam.pos_x == 10 && cam.pos_y == -20 && cam.pos_z == 30);
  assert(cam.target_x == 10 && cam.target_y == -20 && cam.target_z == 30);
  assert(cam.anchor_x == 10 && cam.anchor_y == -20 && cam.anchor_z == 30);
  assert(cam.anchor2_x == 10 && cam.anchor2_y == -20 && cam.anchor2_z == 30);
  assert(cam.vel_x == 0 && cam.vel_z == 0);
  assert(cam.step == 0 && cam.acc_x == 0 && cam.acc_z == 0);
  assert(cam.speed == 0);
  assert(cam.timer == 0 && cam.timer_limit == 0);
  assert(cam.event_param == 0 && cam.event_cursor == 0);
  assert(cam.anchor_time == 0 && cam.anchor2_time == 0);
  assert(cam.rate_x == 0 && cam.rate_z == 0);
  assert(cam.paused == 0);
}

static void test_init_invalid(void) {
  assert(fifa96_camera_init(NULL, 0, 0, 0) == -FIFA96_ERR_INVALID);
}

static void test_update_paused_noop(void) {
  fifa96_camera cam = fresh_camera();
  cam.paused = 1;
  cam.speed = 5;
  cam.vel_x = 3;
  cam.pos_x = 100;
  cam.timer = 9;
  assert(fifa96_camera_update(&cam, 4, 0, 0) == FIFA96_OK);
  assert(cam.pos_x == 100);
  assert(cam.timer == 9);
  assert(cam.speed == 5);
}

static void test_update_idle_no_timer(void) {
  fifa96_camera cam = fresh_camera();
  cam.pos_x = 7;
  assert(fifa96_camera_update(&cam, 5, 0, 0) == FIFA96_OK);
  assert(cam.timer == 0);
  assert(cam.pos_x == 7);
  assert(cam.speed == 0);
}

static void test_update_timer_advances_with_event_param(void) {
  fifa96_camera cam = fresh_camera();
  cam.event_param = 0x20;
  cam.pos_x = 7;
  cam.timer = 0xFFFE;
  assert(fifa96_camera_update(&cam, 3, 0, 0) == FIFA96_OK);
  assert(cam.timer == 1);
  assert(cam.pos_x == 7);
  assert(cam.acc_x == 0 && cam.acc_z == 0);
  assert(cam.speed == 0);
}

static void test_update_integrates_position(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 100, 200, 300);
  cam.vel_x = 4;
  cam.vel_z = -2;
  cam.speed = 4;
  cam.anchor_time = 0x7FFF;
  cam.anchor2_time = 0x7FFF;
  assert(fifa96_camera_update(&cam, 3, 0, 0) == FIFA96_OK);
  assert(cam.timer == 3);
  assert(cam.acc_x == 12);
  assert(cam.acc_z == (uint16_t)-6);
  assert(cam.step == 13);
  assert(cam.pos_x == 112);
  assert(cam.pos_y == 200);
  assert(cam.pos_z == 294);
  assert(cam.speed == 4);
}

static void test_update_zero_delta_clears_acc(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 10, 20, 30);
  cam.vel_x = 7;
  cam.vel_z = 7;
  cam.speed = 7;
  cam.acc_x = 0xAAAA;
  cam.acc_z = 0xBBBB;
  cam.anchor_time = 0x7FFF;
  cam.anchor2_time = 0x7FFF;
  assert(fifa96_camera_update(&cam, 0, 0, 0) == FIFA96_OK);
  assert(cam.acc_x == 0 && cam.acc_z == 0);
  assert(cam.step == 0);
  assert(cam.pos_x == 10 && cam.pos_z == 30);
  assert(cam.timer == 0);
}

static void test_update_negative_delta_clears_acc(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 10, 20, 30);
  cam.vel_x = 7;
  cam.vel_z = 9;
  cam.speed = 7;
  cam.acc_x = 0xAAAA;
  cam.acc_z = 0xBBBB;
  cam.anchor_time = 0x7FFF;
  cam.anchor2_time = 0x7FFF;
  assert(fifa96_camera_update(&cam, -1, 0, 0) == FIFA96_OK);
  assert(cam.acc_x == 0 && cam.acc_z == 0);
  assert(cam.step == 0);
  assert(cam.pos_x == 10 && cam.pos_z == 30);
  assert(cam.timer == 0xFFFF);
}

static void test_update_bucket_snapshots(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 100, 200, 300);
  cam.vel_x = 1;
  cam.speed = 1;
  cam.anchor_time = 0;
  cam.anchor2_time = 0;
  assert(fifa96_camera_update(&cam, 1, 0, 0) == FIFA96_OK);
  assert(cam.timer == 1);
  assert(cam.anchor_x == 101 && cam.anchor_y == 200 && cam.anchor_z == 300);
  assert(cam.anchor2_x == 101 && cam.anchor2_y == 200 && cam.anchor2_z == 300);
  assert(cam.anchor_time == 1 && cam.anchor2_time == 1);
  assert(fifa96_camera_update(&cam, 1, 0, 0) == FIFA96_OK);
  assert(cam.anchor_x == 102 && cam.anchor2_x == 102);
  assert(cam.anchor_time == 2 && cam.anchor2_time == 2);
}

static void test_update_bucket_strict_and_signed(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 10, 20, 30);
  cam.vel_x = 1;
  cam.speed = 1;
  cam.anchor_time = 5;
  cam.anchor2_time = 4;
  assert(fifa96_camera_update(&cam, 5, 0, 0) == FIFA96_OK);
  assert(cam.timer == 5);
  assert(cam.anchor_x == 10);
  assert(cam.anchor_time == 5);
  assert(cam.anchor2_x == 15);
  assert(cam.anchor2_time == 5);
}

static void test_update_bucket_negative_timer(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 10, 20, 30);
  cam.vel_x = 1;
  cam.speed = 1;
  cam.timer = 0xFFFF;
  cam.anchor_time = 0x7FFF;
  cam.anchor2_time = 0xFFFF;
  assert(fifa96_camera_update(&cam, 1, 0, 0) == FIFA96_OK);
  assert(cam.timer == 0);
  assert(cam.anchor_x == 10);
  assert(cam.anchor2_x == 11);
  assert(cam.anchor2_time == 0);
}

static void test_update_rate_param_gates(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 10, 20, 30);
  cam.vel_x = 1;
  cam.speed = 1;
  cam.anchor_time = 0x7FFF;
  cam.anchor2_time = 0x7FFF;
  cam.rate_x = 2;
  cam.event_param = 0x10;
  cam.event_cursor = 0x70;
  assert(fifa96_camera_update(&cam, 1, 0, 0) == FIFA96_OK);
  assert(cam.event_cursor == 0x70);
  assert(cam.target_x == 10);
  assert(cam.rate_x == 2);
}

static void test_update_rate_zero_gate(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 10, 20, 30);
  cam.vel_x = 1;
  cam.speed = 1;
  cam.anchor_time = 0x7FFF;
  cam.anchor2_time = 0x7FFF;
  cam.event_param = 0x20;
  cam.event_cursor = 0x70;
  assert(fifa96_camera_update(&cam, 1, 0, 0) == FIFA96_OK);
  assert(cam.event_cursor == 0x70);
  assert(cam.target_x == 10);
}

static void test_update_event_cursor_accumulates(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  cam.vel_x = 10;
  cam.speed = 10;
  cam.anchor_time = 0x7FFF;
  cam.anchor2_time = 0x7FFF;
  cam.rate_x = 2;
  cam.event_param = 0x20;
  cam.event_cursor = 0x60;
  assert(fifa96_camera_update(&cam, 1, 0, 0) == FIFA96_OK);
  assert(cam.step == 10);
  assert(cam.event_cursor == 0x6A);
  assert(cam.target_x == 0);
  cam.event_cursor = 0x6F;
  assert(fifa96_camera_update(&cam, 1, 1, 0) == FIFA96_OK);
  assert(cam.event_cursor == 0x79);
  assert(cam.target_x == 0);
}

static void test_update_event_cursor_signed(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  cam.vel_x = 2;
  cam.speed = 2;
  cam.anchor_time = 0x7FFF;
  cam.anchor2_time = 0x7FFF;
  cam.rate_x = 1;
  cam.event_param = 0x20;
  cam.event_cursor = 0xFFFF;
  assert(fifa96_camera_update(&cam, 1, 0, 0) == FIFA96_OK);
  assert(cam.event_cursor == 1);
  assert(cam.target_x == 0);
}

static void test_update_event_interpolates(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  cam.target_x = 100, cam.target_y = 200, cam.target_z = 300;
  cam.anchor_x = 50, cam.anchor_y = -1, cam.anchor_z = 60;
  cam.vel_x = 4;
  cam.vel_z = 5;
  cam.speed = 6;
  cam.timer = 10;
  cam.timer_limit = 20;
  cam.anchor_time = 0x7FFF;
  cam.anchor2_time = 0x7FFF;
  cam.rate_x = 2;
  cam.rate_z = -3;
  cam.event_param = 0x20;
  cam.event_cursor = 0x70;
  assert(fifa96_camera_update(&cam, 1, 0, 0) == FIFA96_OK);
  assert(cam.vel_x == 6);
  assert(cam.vel_z == 2);
  assert(cam.target_x == 118);
  assert(cam.target_y == 200);
  assert(cam.target_z == 273);
  assert(cam.anchor_x == 68);
  assert(cam.anchor_y == -1);
  assert(cam.anchor_z == 33);
  assert(cam.event_cursor == 0);
  assert(cam.speed == 6);
}

static void test_update_event_interpolates_negative_remaining(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  cam.target_x = 100;
  cam.target_z = 100;
  cam.anchor_x = 100;
  cam.anchor_z = 100;
  cam.timer = 25;
  cam.timer_limit = 20;
  cam.speed = 1;
  cam.anchor_time = 0x7FFF;
  cam.anchor2_time = 0x7FFF;
  cam.rate_x = 2;
  cam.event_param = 0x20;
  cam.event_cursor = 0x70;
  assert(fifa96_camera_update(&cam, 0, 0, 0) == FIFA96_OK);
  assert(cam.target_x == 90);
  assert(cam.anchor_x == 90);
}

static void test_update_input_bit_forces_interpolate(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  cam.target_x = 10;
  cam.anchor_x = 10;
  cam.speed = 1;
  cam.timer = 0;
  cam.timer_limit = 4;
  cam.anchor_time = 0x7FFF;
  cam.anchor2_time = 0x7FFF;
  cam.rate_x = 1;
  cam.event_param = 0x20;
  cam.event_cursor = 0;
  assert(fifa96_camera_update(&cam, 0, 0, 1) == FIFA96_OK);
  assert(cam.event_cursor == 0);
  assert(cam.target_x == 14);
  assert(cam.vel_x == 1);
}

static void test_update_invalid(void) {
  assert(fifa96_camera_update(NULL, 1, 0, 0) == -FIFA96_ERR_INVALID);
}

static void test_target_buckets(void) {
  fifa96_camera cam = fresh_camera();
  cam.timer = 5;
  cam.anchor_time = 10;
  cam.anchor2_time = 20;
  cam.anchor_x = 0x12340001;
  cam.anchor_z = 0x56780002;
  cam.anchor2_x = 0x11110003;
  cam.anchor2_z = 0x22220004;
  cam.target_x = 100;
  cam.target_z = 300;
  cam.vel_x = 3;
  cam.vel_z = -2;
  int16_t x = -1, z = -1;
  assert(fifa96_camera_target(&cam, &x, &z) == 0);
  assert(x == 1 && z == 2);
  cam.timer = 10;
  assert(fifa96_camera_target(&cam, &x, &z) == 1);
  assert(x == 3 && z == 4);
  cam.timer = 20;
  assert(fifa96_camera_target(&cam, &x, &z) == 2);
  assert(x == 196 && z == 236);
}

static void test_target_signed_boundaries(void) {
  fifa96_camera cam = fresh_camera();
  cam.timer = 0x8000;
  cam.anchor_time = 1;
  cam.anchor2_time = 0x7FFF;
  cam.anchor_x = 7;
  cam.anchor_z = 9;
  int16_t x = -1, z = -1;
  assert(fifa96_camera_target(&cam, &x, &z) == 0);
  assert(x == 7 && z == 9);
  cam.timer = 1;
  assert(fifa96_camera_target(&cam, &x, &z) == 1);
}

static void test_target_wires_entity_search(void) {
  fifa96_camera cam = fresh_camera();
  cam.timer = 100;
  cam.anchor_time = 1;
  cam.anchor2_time = 2;
  cam.target_x = 0;
  cam.target_z = 0;
  cam.vel_x = 0;
  cam.vel_z = 0;
  const fifa96_entity_candidate candidates[3] = {
      {100, 100, 0, 0},
      {5, 0, 0, 0},
      {0, 9, 0, 0},
  };
  int16_t x = -1, z = -1;
  int16_t best = -1;
  assert(fifa96_camera_target(&cam, &x, &z) == 2);
  assert(x == 0 && z == 0);
  assert(fifa96_entity_find_nearest(candidates, 3, 0, x, z, &best) == 1);
  assert(best == 5);
}

static void test_target_invalid(void) {
  fifa96_camera cam = fresh_camera();
  int16_t x = 0, z = 0;
  assert(fifa96_camera_target(NULL, &x, &z) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_target(&cam, NULL, &z) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_target(&cam, &x, NULL) == -FIFA96_ERR_INVALID);
}

static void test_out_of_bounds(void) {
  assert(fifa96_camera_out_of_bounds(0, 0) == 0);
  assert(fifa96_camera_out_of_bounds(0x6C0, 0xAB0) == 0);
  assert(fifa96_camera_out_of_bounds(-0x6C0, -0xAB0) == 0);
  assert(fifa96_camera_out_of_bounds(0x6C1, 0) == 1);
  assert(fifa96_camera_out_of_bounds(-0x6C1, 0) == 1);
  assert(fifa96_camera_out_of_bounds(0, 0xAB1) == 1);
  assert(fifa96_camera_out_of_bounds(0, -0xAB1) == 1);
  assert(fifa96_camera_out_of_bounds(INT32_MIN, INT32_MIN) == 0);
  assert(fifa96_camera_out_of_bounds(INT32_MAX, 0) == 1);
}

static void test_reflect_in_bounds(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0x720, 0, 0xB10);
  assert(fifa96_camera_reflect(&cam, 1, 0) == 0);
  assert(cam.pos_x == 0x720 && cam.pos_z == 0xB10);
  cam.pos_x = 0x6C1;
  assert(fifa96_camera_reflect(&cam, 1, 0) == 0);
  assert(cam.pos_x == 0x6C1);
}

static void test_reflect_mirrors_x(void) {
  fifa96_camera cam = fresh_camera();
  cam.pos_x = 0x800;
  cam.pos_z = 0x100;
  cam.vel_x = 7;
  cam.vel_z = 9;
  assert(fifa96_camera_reflect(&cam, 1, 0) == 1);
  assert(cam.pos_x == 0x640);
  assert(cam.pos_z == 0x100);
  assert(cam.vel_x == -7);
  assert(cam.vel_z == 9);
  cam.pos_x = -0x800;
  cam.vel_x = 7;
  assert(fifa96_camera_reflect(&cam, 1, 0) == 1);
  assert(cam.pos_x == -0x640);
  assert(cam.vel_x == -7);
  assert(cam.vel_z == 9);
}

static void test_reflect_mirrors_z(void) {
  fifa96_camera cam = fresh_camera();
  cam.pos_x = 0;
  cam.pos_z = 0x1200;
  cam.vel_x = 7;
  cam.vel_z = 9;
  assert(fifa96_camera_reflect(&cam, 1, 0) == 1);
  assert(cam.pos_x == 0);
  assert(cam.pos_z == 0x420);
  assert(cam.vel_x == 7);
  assert(cam.vel_z == -9);
  cam.pos_z = -0x1200;
  cam.vel_z = 9;
  assert(fifa96_camera_reflect(&cam, 1, 0) == 1);
  assert(cam.pos_z == -0x420);
  assert(cam.vel_z == -9);
  assert(cam.vel_x == 7);
}

static void test_reflect_both_axes(void) {
  fifa96_camera cam = fresh_camera();
  cam.pos_x = 0x800;
  cam.pos_z = 0x1200;
  cam.vel_x = 1;
  cam.vel_z = 2;
  assert(fifa96_camera_reflect(&cam, 1, 0) == 1);
  assert(cam.pos_x == 0x640);
  assert(cam.pos_z == 0x420);
  assert(cam.vel_x == -1);
  assert(cam.vel_z == -2);
}

static void test_reflect_boundaries(void) {
  fifa96_camera cam = fresh_camera();
  cam.pos_x = 0x721;
  cam.pos_z = 0;
  assert(fifa96_camera_reflect(&cam, 1, 0) == 1);
  assert(cam.pos_x == 0xE40 - 0x721);
  cam.pos_x = 0;
  cam.pos_z = 0xB11;
  assert(fifa96_camera_reflect(&cam, 1, 0) == 1);
  assert(cam.pos_z == 0x1620 - 0xB11);
}

static void test_reflect_gates(void) {
  fifa96_camera cam = fresh_camera();
  cam.pos_x = 0x800;
  cam.pos_z = 0x1200;
  cam.vel_x = 1;
  cam.vel_z = 2;
  assert(fifa96_camera_reflect(&cam, 0, 0) == 0);
  assert(cam.pos_x == 0x800 && cam.pos_z == 0x1200);
  assert(cam.vel_x == 1 && cam.vel_z == 2);
  assert(fifa96_camera_reflect(&cam, 1, 1) == 0);
  assert(cam.pos_x == 0x800 && cam.pos_z == 0x1200);
  assert(cam.vel_x == 1 && cam.vel_z == 2);
}

static void test_reflect_invalid(void) {
  assert(fifa96_camera_reflect(NULL, 1, 0) == -FIFA96_ERR_INVALID);
}

int main(void) {
  test_init_fields();
  test_init_invalid();
  test_update_paused_noop();
  test_update_idle_no_timer();
  test_update_timer_advances_with_event_param();
  test_update_integrates_position();
  test_update_zero_delta_clears_acc();
  test_update_negative_delta_clears_acc();
  test_update_bucket_snapshots();
  test_update_bucket_strict_and_signed();
  test_update_bucket_negative_timer();
  test_update_rate_param_gates();
  test_update_rate_zero_gate();
  test_update_event_cursor_accumulates();
  test_update_event_cursor_signed();
  test_update_event_interpolates();
  test_update_event_interpolates_negative_remaining();
  test_update_input_bit_forces_interpolate();
  test_update_invalid();
  test_target_buckets();
  test_target_signed_boundaries();
  test_target_wires_entity_search();
  test_target_invalid();
  test_out_of_bounds();
  test_reflect_in_bounds();
  test_reflect_mirrors_x();
  test_reflect_mirrors_z();
  test_reflect_both_axes();
  test_reflect_boundaries();
  test_reflect_gates();
  test_reflect_invalid();
  puts("test_camera: ok");
  return 0;
}
