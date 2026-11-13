// port/tests/test_camera.c — FU-71 match-camera follow port (port/spec/FU71_camera_track.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_entity_update.h"
#include "fifa96_loader/fifa96_camera.h"
#include "fifa96_loader/fifa96_projection.h"
#include "fifa96_loader/fifa96_rng.h"

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
  /* FUN_000700F4 0x7011C/0x7013D/0x70146 (the 0x1104AB image-default row):
   * decay 10, hi 16, lo = 16 + (0x20 - 16)/2 = 24; 0x70258 clears tracked. */
  assert(cam.pan_decay == 10 && cam.pan_rate_hi == 16 && cam.pan_rate_lo == 24);
  assert(cam.tracked == FIFA96_CAMERA_TRACKED_NONE);
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
  cam.timer_limit = 0x7FFF;   /* keep the timer below the pan-step trigger */
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
  cam.timer_limit = 0x7FFF;   /* keep the pan-step trigger out */
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
  cam.timer_limit = 0x7FFF;   /* keep the pan-step trigger out */
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
  cam.timer_limit = 0x7FFF;   /* keep the pan-step trigger out */
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
  cam.pos_y = 0;              /* no landing pan in this rate-gate fixture */
  cam.vel_x = 1;
  cam.speed = 1;
  cam.anchor_time = 0x7FFF;
  cam.anchor2_time = 0x7FFF;
  cam.rate_x = 2;
  cam.event_param = 0x10;
  cam.event_cursor = 0x70;
  cam.timer_limit = 0x7FFF;   /* keep the pan-step trigger out */
  assert(fifa96_camera_update(&cam, 1, 0, 0) == FIFA96_OK);
  assert(cam.event_cursor == 0x70);
  assert(cam.target_x == 10);
  assert(cam.rate_x == 2);
}

static void test_update_rate_zero_gate(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 10, 20, 30);
  cam.pos_y = 0;              /* no landing pan in this rate-gate fixture */
  cam.vel_x = 1;
  cam.speed = 1;
  cam.anchor_time = 0x7FFF;
  cam.anchor2_time = 0x7FFF;
  cam.event_param = 0x20;
  cam.event_cursor = 0x70;
  cam.timer_limit = 0x7FFF;   /* keep the pan-step trigger out */
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
  cam.timer_limit = 0x7FFF;   /* keep the pan-step trigger out */
  assert(fifa96_camera_update(&cam, 1, 0, 0) == FIFA96_OK);
  assert(cam.step == 10);
  assert(cam.event_cursor == 0x6A);
  assert(cam.target_x == 0);
  /* cursor_high = the native [0x14C2F6] != 0 selector: the threshold is
   * 0x90, so 0x6F still accumulates. */
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
  cam.timer_limit = 0x7FFF;   /* keep the pan-step trigger out */
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
  /* timer > timer_limit: the native FUN_000736AC pan step fires first
   * (0x737da) and re-arms the event, so the negative-remaining interpolation
   * only remains reachable on the pan re-arm's own F6 value. Here the pan
   * resets the timer and zeroes the velocity (the raw fixture has no rate
   * bytes staged). The phase-53 T1 camera init now stages the native pan
   * constants (decay 10), so the re-arm height is 0x20 * 10 / 0x20 = 0xA
   * (not the old zero-pan idle arm): F2 = ramp(0xA) = 13, timer_limit =
   * 2 * 13 = 26, F6 = 0 and the 16-scaled slow path keeps the zero
   * velocity. */
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
  assert(cam.timer == 0 && cam.timer_limit == 26);    /* the height re-arm */
  assert(cam.vel_x == 0 && cam.vel_z == 0);
  assert(cam.event_param == 0x0A);                    /* 0x20 * 10 / 0x20 */
  assert(cam.pan_counter == 1);
  assert(cam.target_x == 0 && cam.anchor_x == 0);     /* origin := pos */
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

/* ---- FU-148 §2.1(a)/§6.2 (S4): the FUN_000505D0 pose feed ---- */

static void test_pose_apply_fields(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 1, 2, 3);
  int32_t yaw = 0, pitch = 0, ratio = 0;
  const fifa96_camera_pose pose = {10, 20, 30, 40, 50, 60};
  assert(fifa96_camera_pose_apply(&cam, &yaw, &pitch, &ratio, &pose) == FIFA96_OK);
  assert(cam.pos_x == 10 && cam.pos_y == 20 && cam.pos_z == 30);
  assert(yaw == 40 && pitch == 50 && ratio == 60);
}

static void test_pose_feed_mode1(void) {
  /* Mode 1/0x12 record 0 of behavior block 0: the image default is the +0x4C
   * array 0x107F1C (the [0x1590CC] team flags are zero -> FUN_0004B7D0 falls
   * to +0x4C), first-hand record 0 = 0x107F1C. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 7, 8, 9);
  fifa96_camera_pose_args args;
  memset(&args, 0, sizeof args);
  args.view_mode = 1;
  int32_t yaw = 0, pitch = 0, ratio = 0;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_x == -100 && cam.pos_y == 488 && cam.pos_z == 1100);
  assert(yaw == 33900 && pitch == 3900 && ratio == 4608);
}

static void test_pose_feed_mode1_selector1_mirror(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  fifa96_camera_pose_args args;
  memset(&args, 0, sizeof args);
  args.view_mode = 0x12;
  args.selector = 1;
  args.mirror = 1;
  int32_t yaw = 0, pitch = 0, ratio = 0;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_x == -100 && cam.pos_z == -1100);
  assert(yaw == (0x18000 - 33900) && pitch == 3900 && ratio == 4608);
  /* without the mirror bit the pose is untouched */
  fifa96_camera_init(&cam, 0, 0, 0);
  args.mirror = 0;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_z == 1100 && yaw == 33900);
}

static void test_pose_feed_mode3_variant(void) {
  /* mode 3 record = 4 | 3 from FUN_000504E0(x, class): 3 iff
   * (class == 2 && x >= 1) || (class != 2 && x < 0). Block 2 class 3. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_pose_args args;
  memset(&args, 0, sizeof args);
  args.view_mode = 3;
  args.block = 2;
  int32_t yaw = 0, pitch = 0, ratio = 0;
  args.variant_x = -1;                 /* class 3, x < 0 -> record 3 */
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_x == -2710 && cam.pos_y == 640 && cam.pos_z == -118);
  assert(yaw == 49340 && pitch == 3931 && ratio == 4608);
  args.variant_x = 0;                  /* -> record 4 */
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_x == -959 && cam.pos_y == 668 && cam.pos_z == -77);
  assert(yaw == 49404 && pitch == 4251 && ratio == 4608);
}

static void test_pose_feed_mode4_variant(void) {
  /* mode 4 record = 2 | 1 from FUN_00050518. Block 1 class 1. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_pose_args args;
  memset(&args, 0, sizeof args);
  args.view_mode = 4;
  args.block = 1;
  int32_t yaw = 0, pitch = 0, ratio = 0;
  args.variant_x = 3;                  /* class 1, x >= 0 -> record 2 */
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_x == -1873 && cam.pos_y == 712 && cam.pos_z == 2013);
  assert(yaw == 51827 && pitch == 4512 && ratio == 4608);
  args.variant_x = -3;                 /* -> record 1 */
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_x == -2727 && cam.pos_y == 740 && cam.pos_z == 2693);
  assert(yaw == 46675 && pitch == 4749 && ratio == 4608);
}

static void test_pose_feed_mode6_record7(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_pose_args args;
  memset(&args, 0, sizeof args);
  args.view_mode = 0x10;
  int32_t yaw = 0, pitch = 0, ratio = 0;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_x == -83 && cam.pos_y == 320 && cam.pos_z == 3631);
  assert(yaw == 33732 && pitch == 2203 && ratio == 3712);
  /* the replay sub < 0 mirror: z negated, yaw folded */
  fifa96_camera_init(&cam, 0, 0, 0);
  args.sub = -1;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_z == -3631 && yaw == (0x18000 - 33732));
}

static void test_pose_feed_mode8_sub(void) {
  /* mode 8, default selector: record 6 when sub < 1, else record 5. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_pose_args args;
  memset(&args, 0, sizeof args);
  args.view_mode = 8;
  int32_t yaw = 0, pitch = 0, ratio = 0;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_x == -4 && cam.pos_y == 276 && cam.pos_z == 1502);
  assert(yaw == 32800 && pitch == 1742 && ratio == 3545);
  args.sub = 1;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_x == -171 && cam.pos_y == 920 && cam.pos_z == 3829);
  assert(yaw == 32320 && pitch == 4550 && ratio == 4608);
}

static void test_pose_feed_mode15(void) {
  /* mode 0x15 fixed record 0x108714 with the native yaw fold and z negate
   * (the camera+0x3c clamp is a leg). */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_pose_args args;
  memset(&args, 0, sizeof args);
  args.view_mode = 0x15;
  int32_t yaw = 0, pitch = 0, ratio = 0;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_x == -66 && cam.pos_y == 4300 && cam.pos_z == -6700);
  assert(yaw == 0 && pitch == 5900 && ratio == 1696);   /* 0x18000-32768 = 0x10000 -> 0 */
}

static void test_pose_feed_default_and_off(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 1, 2, 3);
  fifa96_camera_pose_args args;
  memset(&args, 0, sizeof args);
  int32_t yaw = 4, pitch = 5, ratio = 6;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 0);
  assert(cam.pos_x == 1 && yaw == 4 && ratio == 6);
  args.view_mode = 0x20;               /* out of the switch band */
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 0);
  assert(cam.pos_x == 1 && yaw == 4 && ratio == 6);
}

static void test_pose_feed_invalid(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_pose_args args;
  memset(&args, 0, sizeof args);
  args.view_mode = 1;
  int32_t v = 0;
  assert(fifa96_camera_pose_feed(NULL, &v, &v, &v, &args) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_pose_feed(&cam, NULL, &v, &v, &args) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_pose_feed(&cam, &v, NULL, &v, &args) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_pose_feed(&cam, &v, &v, NULL, &args) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_pose_feed(&cam, &v, &v, &v, NULL) == -FIFA96_ERR_INVALID);
  args.block = 6;
  assert(fifa96_camera_pose_feed(&cam, &v, &v, &v, &args) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_pose_apply(NULL, &v, &v, &v, NULL) == -FIFA96_ERR_INVALID);
}

/* ---- FU-148 §2.1(c)/§5.2 (S4): FUN_00071C94 + FUN_00070544 event setter ---- */

static void test_ramp_values(void) {
  assert(fifa96_camera_ramp(0x640) == 0x94);
  assert(fifa96_camera_ramp(0x700) == 0x94);
  assert(fifa96_camera_ramp(0) == 3);
  assert(fifa96_camera_ramp(1) == 8);
  assert(fifa96_camera_ramp(0x30) == 25);
  assert(fifa96_camera_ramp(0x40) == 29);
  assert(fifa96_camera_ramp(0x50) == 33);
  assert(fifa96_camera_ramp(0x70) == 39);
  assert(fifa96_camera_ramp(-5) == 3);
}

static void test_event_set_window_pan(void) {
  /* pos (0,0,0xB00), seed step (0, 2000), height 0x30:
   * F2 = ramp(0x30) = 25, timer_limit = 50, F6 = F2 - ramp(0x30) = 0,
   * timer = 0 (native [0x1577FA] = F6, the corrected cell), divisor
   * F8 = 50, vel_z = 2000/50 = 40 -> the > 0x19 atan walk caps the bearing
   * to 0x19 and rescales: vel_z = 25, speed = 25. anchor_z = 0xB00 + 25*50 =
   * 0xC2A, anchor_time = anchor2_time = 50. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0xB00);
  assert(fifa96_camera_event_set(&cam, 0, 2000, 0x30, 0) == FIFA96_OK);
  assert(cam.event_param == 0x30);
  assert(cam.timer == 0 && cam.timer_limit == 50);
  assert(cam.ramp_divisor == 50);
  assert(cam.vel_x == 0 && cam.vel_z == 25);
  assert(cam.speed == 25);
  assert(cam.anchor_x == 0 && cam.anchor_z == 0xB00 + 25 * 50);
  assert(cam.anchor_time == 50 && cam.anchor2_time == 50);
  assert(cam.anchor2_x == 0 && cam.anchor2_z == 0xB00 + 25 * 50);
  assert(cam.rate_x == 0 && cam.rate_z == 0 && cam.event_cursor == 0);
  /* FUN_000700F4 image defaults: decay 10, k-hi 16, k-lo = 16 + (0x20-16)/2 */
  assert(cam.pan_decay == 10 && cam.pan_rate_hi == 16 && cam.pan_rate_lo == 24);
  /* the accepted integrator then moves the camera: one delta-2 frame */
  assert(fifa96_camera_update(&cam, 2, 0, 0) == FIFA96_OK);
  assert(cam.pos_z == 0xB32);
  assert(cam.timer == 2);
  assert(fifa96_camera_out_of_bounds(cam.pos_x, cam.pos_z) == 1);
}

static void test_event_set_idle_height(void) {
  /* height < 1 takes the F2=6 / timer 0 branch. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  assert(fifa96_camera_event_set(&cam, 0x30, 0, 0, 0) == FIFA96_OK);
  assert(cam.event_param == 0);
  assert(cam.timer == 0 && cam.timer_limit == 0xC);
  assert(cam.vel_x == 4 && cam.vel_z == 0);
  assert(cam.speed == 4);
}

static void test_event_set_height_clamps(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, -10, 0);
  assert(fifa96_camera_event_set(&cam, 0, 0, -20, 0) == FIFA96_OK);
  /* clamped to the target y, then zeroed by the native F2<1 branch */
  assert(cam.event_param == 0);
  assert(cam.timer == 0);
  fifa96_camera_init(&cam, 0, 0, 0);
  assert(fifa96_camera_event_set(&cam, 0, 0, 0x700, 0) == FIFA96_OK);
  assert(cam.event_param == 0x640);
  assert(cam.timer_limit == 0x94 * 2 && cam.timer == 0);
  assert(cam.ramp_divisor == 0x94 * 2);
}

static void test_event_set_anchor2_computed(void) {
  /* height 0x60 > 0x4F: a2_time = F2 + ramp(0x10) = 36 + 15 = 51; with ty = 0,
   * F6 = F2 - F2 = 0 and span = 51 >= 1, so anchor2 = origin + vel * 51.
   * F2 = ramp(0x60) = 36, divisor 72; seed 2160 / 72 = vel 30 -> the walk
   * caps it to 25 in the same direction. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 100, 0, 0);
  assert(fifa96_camera_event_set(&cam, 0, 2160, 0x60, 0) == FIFA96_OK);
  assert(cam.timer == 0 && cam.timer_limit == 72);
  assert(cam.vel_z == 25);
  assert(cam.anchor2_time == 51);
  assert(cam.anchor2_x == 100 && cam.anchor2_z == 25 * 51);
  assert(cam.anchor_time == 72 && cam.anchor_z == 25 * 72);
}

static void test_event_set_bail_gate(void) {
  /* FUN_00071C94 0x71c99: a nonzero [0x157A6C] returns before the reset. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 5, 6, 7);
  cam.event_suspended = 1;
  cam.vel_z = 9;
  assert(fifa96_camera_event_set(&cam, 0, 2000, 0x30, 0) == 1);
  assert(cam.pos_x == 5 && cam.pos_y == 6 && cam.pos_z == 7);
  assert(cam.vel_z == 9);              /* nothing reset */
  cam.event_suspended = 0;
  assert(fifa96_camera_event_set(&cam, 0, 2000, 0x30, 0) == FIFA96_OK);
  assert(cam.vel_z == 25);
}

static void test_event_set_ramp_param(void) {
  /* The FUN_00070544 ramp param selects the F6 sign:
   * param 0 -> F6 = F2 - ramp(h - ty); param != 0 -> F6 = F2 + ramp(h - ty).
   * pos y 0x10, height 0x60: F2 = 36, ramp(0x50) = 33.
   * param 0: F6 = 3, divisor 69; param 1: F6 = 69, divisor 3. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0x10, 0);
  assert(fifa96_camera_event_set(&cam, 0, 207, 0x60, 0) == FIFA96_OK);
  assert(cam.timer == 3 && cam.timer_limit == 72 && cam.ramp_divisor == 69);
  assert(cam.vel_z == 3);              /* 207 / 69 */
  fifa96_camera_init(&cam, 0, 0x10, 0);
  assert(fifa96_camera_event_set(&cam, 0, 207, 0x60, 1) == FIFA96_OK);
  assert(cam.timer == 69 && cam.timer_limit == 72 && cam.ramp_divisor == 3);
  assert(cam.vel_z == 25);             /* 207 / 3 -> the walk caps to 25 */
}

static void test_event_set_resets_pan_counter(void) {
  /* FUN_000700F4 clears [0x157821], so an event set always starts the fast
   * path; the slow (k-scaled) path is only reachable from FUN_000709D0's
   * re-arm, which increments the counter first. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  cam.vel_z = 40;
  cam.pan_counter = 3;
  assert(fifa96_camera_event_set(&cam, 0, 2000, 0x30, 0) == FIFA96_OK);
  assert(cam.pan_counter == 0);
  assert(cam.vel_z == 25);             /* seed/50 = 40 -> the walk caps to 25 */
  assert(cam.timer == 0);
}

static void test_event_set_tail_clears(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  cam.event_cursor = 0x1234;
  cam.acc_x = 0xAAAA;
  cam.acc_z = 0xBBBB;
  assert(fifa96_camera_event_set(&cam, 0, 0x30, 0, 0) == FIFA96_OK);
  assert(cam.event_cursor == 0 && cam.acc_x == 0 && cam.acc_z == 0);
}

static void test_event_set_invalid(void) {
  assert(fifa96_camera_event_set(NULL, 0, 0, 0, 0) == -FIFA96_ERR_INVALID);
}

/* ---- FU-152 §2.9 (T2): FUN_000709D0 pan step ---- */

static void test_pan_step_band_decay_rearm(void) {
  /* event height 0x30 -> band (2, (0x30+0x50)/2 = 0x40); the counter
   * advances; the height decays by the [0x157819] rate (image 10) / 0x20;
   * the re-arm (pan in progress -> the slow path, k = [0x15781A] = 16)
   * scales the velocity and re-runs the height ramp at the decayed height:
   * h 15 -> F2 = ramp(15) = 15, F4 = 30, F6 = 0, divisor 30. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 100, 0, 0);
  cam.event_param = 0x30;
  cam.pan_decay = 10;
  cam.pan_rate_hi = 16;
  cam.pan_rate_lo = 24;
  cam.vel_z = 20;
  cam.ramp_divisor = 50;
  cam.event_step_x = 0;
  cam.event_step_z = 20 * 50;
  assert(cam.pan_counter == 0);
  int32_t class_of = 0, param = 0;
  assert(fifa96_camera_pan_step(&cam, NULL, 0, &class_of, &param) == FIFA96_OK);
  assert(class_of == 2 && param == 0x40);
  assert(cam.pan_counter == 1);
  assert(cam.event_param == 15);
  assert(cam.vel_z == 10);             /* 20 * 16 / 0x20 */
  assert(cam.timer == 0 && cam.timer_limit == 30);
  assert(cam.ramp_divisor == 30);
  assert(cam.event_step_z == 10 * 30);
}

static void test_pan_step_idle_height_band_skipped(void) {
  /* height 0 skips the band sink but still advances the counter and decays. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  cam.vel_z = 12;
  cam.ramp_divisor = 12;
  cam.event_step_z = 12 * 12;
  cam.pan_rate_lo = 24;                /* FUN_000700F4: hi + (0x20-hi)/2 */
  int32_t class_of = -1, param = -1;
  assert(fifa96_camera_pan_step(&cam, NULL, 0, &class_of, &param) == FIFA96_OK);
  assert(class_of == -1 && param == -1);
  assert(cam.pan_counter == 1);
  assert(cam.event_param == 0);
  assert(cam.timer == 0 && cam.timer_limit == 0xC);
  assert(cam.vel_z == 12 * 24 / 0x20);  /* height <= 0 -> k = [0x15781B] = 24 */
}

static void test_pan_step_walk(void) {
  /* (> 0x19 bearing with the [0x14C1D4|D6]&4 gate and an rng): the velocity
   * is rebuilt from the atan direction + `rng_low - 0x80` (native 0x70b07),
   * magnitude kept, then the slow-path re-arm shrinks it. Seed 0x1234's first
   * draw low byte is 0x01: angle = atan(3,4)=105 + 1 - 128 = -22 ->
   * vel (5*sin(-22), 5*cos(-22)) >> 16 = (-1, 4) -> k=16 slow path -> (0, 2). */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  cam.event_param = 0x30;
  cam.pan_decay = 10;
  cam.pan_rate_hi = 16;
  cam.vel_x = 3;
  cam.vel_z = 4;
  cam.ramp_divisor = 1;
  cam.event_step_x = 3;
  cam.event_step_z = 4;
  struct fifa96_rng rng;
  assert(fifa96_rng_seed(&rng, 0x1234u) == FIFA96_OK);
  assert(fifa96_camera_pan_step(&cam, &rng, 4, NULL, NULL) == FIFA96_OK);
  assert(cam.vel_x == 0 && cam.vel_z == 2);
  assert(cam.speed == 2);
  assert(cam.event_param == 15);
  assert(cam.event_step_x == 0 && cam.event_step_z == 60);
  assert(cam.pan_counter == 1);
}

static void test_pan_step_invalid(void) {
  assert(fifa96_camera_pan_step(NULL, NULL, 0, NULL, NULL) ==
         -FIFA96_ERR_INVALID);
}

/* ---- M2 phase-9 T3: the walk gate, the tracked record, the follow ---------- */

/* FU-148 §12.2 / FU-145 §1.8: FUN_000709D0's random walk only runs when
 * `(word[0x14C1D4] | word[0x14C1D6]) & 4` is set. `fifa96_camera_update_walk`
 * is the native FUN_000736AC in-line call (0x737b9..0x737da) with the gate and
 * the match RNG threaded in; the plain `fifa96_camera_update` is the gate-0
 * frame path. */
static void test_update_walk_gate(void) {
  fifa96_camera a = fresh_camera();
  fifa96_camera b = fresh_camera();
  struct fifa96_rng rng;
  fifa96_camera_init(&a, 0, 0, 0);
  fifa96_camera_init(&b, 0, 0, 0);
  a.event_param = 0x30;
  b.event_param = 0x30;
  a.pan_decay = 10;
  b.pan_decay = 10;
  a.pan_rate_hi = 16;
  b.pan_rate_hi = 16;
  a.vel_x = 3;
  a.vel_z = 4;
  b.vel_x = 3;
  b.vel_z = 4;
  a.ramp_divisor = 1;
  b.ramp_divisor = 1;
  a.event_step_x = 3;
  a.event_step_z = 4;
  b.event_step_x = 3;
  b.event_step_z = 4;
  a.timer = 0x100;
  b.timer = 0x100;
  a.timer_limit = 0;
  b.timer_limit = 0;
  assert(fifa96_rng_seed(&rng, 0x1234u) == FIFA96_OK);
  /* gate 4: the pan step runs the walk (the same seed/steps as
   * test_pan_step_walk -> the rebuilt pair lands (0, 2) after the re-arm). */
  assert(fifa96_camera_update_walk(&a, &rng, 2, 0, 0, 4) == FIFA96_OK);
  assert(a.pan_counter == 1);
  assert(a.vel_x == 0 && a.vel_z == 2);
  /* gate 0: the identical camera keeps the slow-path scale only. */
  assert(fifa96_camera_update_walk(&b, &rng, 2, 0, 0, 0) == FIFA96_OK);
  assert(b.pan_counter == 1);
  assert(b.vel_x == 1 && b.vel_z == 2);
  assert(fifa96_camera_update_walk(NULL, &rng, 1, 0, 0, 0) == -FIFA96_ERR_INVALID);
}

/* FU316-L2: `0x737C8..0x737DA` admits the pan step when the camera lands
 * (`prev_y > 0 && pos_y <= 0`) even though the timer has not passed its
 * limit. The 0x73756/0x7375b copy seeds prev_y from the entering pos_y; the
 * 0x70B94 arc then drops pos_y to zero. */
static void test_update_walk_landing_pan(void) {
  fifa96_camera a = fresh_camera();
  fifa96_camera b = fresh_camera();
  fifa96_camera_init(&a, 0, 0x40, 0);
  fifa96_camera_init(&b, 0, 0, 0);
  a.event_param = 0x20;
  b.event_param = 0x20;
  a.event_peak = 0;                 /* arc height 0 at timer 0 */
  b.event_peak = 0;
  a.speed = 1;
  b.speed = 1;
  a.timer = 0;
  b.timer = 0;
  a.timer_limit = 0x10;
  b.timer_limit = 0x10;
  assert(fifa96_camera_update_walk(&a, NULL, 2, 0, 0, 0) == FIFA96_OK);
  assert(a.prev_y == 0x40);
  assert(a.pos_y == 0);
  assert(a.pan_counter == 1);       /* the landing arm ran */

  assert(fifa96_camera_update_walk(&b, NULL, 2, 0, 0, 0) == FIFA96_OK);
  assert(b.prev_y == 0);
  assert(b.pos_y == 0);
  assert(b.pan_counter == 0);       /* no landing edge */
}

/* FU-71 leg 9.6 (phase-51 (FU-71)): `fifa96_camera_wall_arm` — the native
 * `0x718A9..0x71B98` far-wall damp/event arm (first-hand
 * `disassemble_bytes 0x719A0`, 148 instructions, plus the shared 0x71972 tail).
 * Every fixture below pins an exact native branch edge; the expected velocity
 * damps are `0x8DC50(NEG,1)` values and the expected speeds are
 * `0xCD514` (`fifa96_entity_hypot`) over the stored pair. */
static fifa96_camera wall_camera(int32_t x, int32_t y, int32_t z, int16_t vx,
                                 int16_t vz, uint16_t speed, uint16_t param) {
  fifa96_camera cam = fresh_camera();
  cam.pos_x = x;
  cam.pos_y = y;
  cam.pos_z = z;
  cam.vel_x = vx;
  cam.vel_z = vz;
  cam.speed = speed;
  cam.event_param = param;
  return cam;
}

/* 0x718A9 inner word gate + 0x719A0 head gate. */
static void test_wall_arm_gate_and_head(void) {
  fifa96_camera_wall_event ev;
  fifa96_camera cam;

  /* 0x718A9: (int16)|camZ|_w > 0xB10 || (int16)|camX|_w > 0x720. */
  cam = wall_camera(0, 0, 0xB10, 0, 0x10, 3, 1);
  memset(&ev, 0xAA, sizeof ev);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_z == 0x10 && cam.speed == 3);    /* no tail store either */
  assert(ev.seed_x == 0 && ev.seed_z == 0 && ev.height == 0 && ev.announce == 0);
  cam = wall_camera(0x720, 0, 0, 0, 0, 3, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.speed == 3);

  /* 0x719A0: both cells zero -> return before the flag/seed init (no store). */
  cam = wall_camera(0, 0, 0xB11, 7, 9, 0, 0);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_x == 7 && cam.vel_z == 9 && cam.speed == 0);

  /* Either live cell passes the head to the flag-0 tail: chain A/B both
   * skipped at |z|=0xB11 / |x|=0, so the same pair is stored and the speed
   * is recomputed by 0x71988. */
  cam = wall_camera(0, 0, 0xB11, 0, 0, 0, 1);     /* event_param live */
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.speed == 0);                          /* hypot(0,0) */
  cam = wall_camera(0, 0, 0xB11, 0, 5, 5, 0);     /* speed live */
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_z == 5 && cam.speed == 5);
}

/* Chain A (Z), 0x719CF..0x71A5A: the length gate, the height/prev corridor
 * damp, the threshold formula and both sign arms. */
static void test_wall_arm_z_chain_edges(void) {
  fifa96_camera_wall_event ev;
  fifa96_camera cam;

  /* |z| = 0xE73 is below the 0xE74 length gate: the chain is skipped. */
  cam = wall_camera(0, 0x10, 0xE73, 0, 8, 8, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_z == 8 && cam.speed == 8);

  /* |z| = 0xE74, height = 0x24: threshold, t = ((0)>>1)+0x24 = 0x24 = height
   * (0x71A32 JL is strict), so the event fires; the damp registers are dead. */
  cam = wall_camera(0, 0x24, 0xE74, 0, 8, 8, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
  assert(ev.seed_z == -0x180 && ev.seed_x == 0);
  assert(ev.height == 0x64);
  assert(cam.vel_z == 8 && cam.speed == 8);       /* vel/speed not stored */

  /* t = 0x24 at |z| = 0xE75; height 0x25 misses by one, height 0x24 hits. */
  cam = wall_camera(0, 0x25, 0xE75, 0, 8, 8, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_z == 8 && cam.speed == 8);
  cam = wall_camera(0, 0x24, 0xE75, 0, 8, 8, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
  assert(ev.seed_z == -0x180 && ev.height == 0x64);

  /* t = ((2)>>1)+0x24 = 0x25 at |z| = 0xE76 (height 0x25 hits exactly). */
  cam = wall_camera(0, 0x25, 0xE76, 0, 8, 8, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
  assert(ev.seed_z == -0x180 && ev.height == 0x65);

  /* height < 0x24: the corridor damp 0x719F5 runs only while |prevZ| < 0xE74
   * (vel_z = shift_step(-8,1) = -4, then speed = hypot(0,-4) = 4). */
  cam = wall_camera(0, 0x23, 0xE80, 0, 8, 8, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_z == -4 && cam.speed == 4);
  cam.prev_z = 0xE73;
  cam.vel_z = 8;
  cam.speed = 8;
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_z == -4 && cam.speed == 4);

  /* |prevZ| = 0xE74 takes the threshold instead, even at height < 0x24. */
  cam = wall_camera(0, 0x23, 0xE80, 0, 8, 8, 1);
  cam.prev_z = 0xE74;
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
  assert(ev.seed_z == -0x180);
  cam.prev_z = -0xE74;
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);

  /* height = 0x24 bypasses the prev check (0x719E1 JGE, chain A's `>=`). */
  cam = wall_camera(0, 0x24, 0xE80, 0, 8, 8, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
  assert(ev.seed_z == -0x180 && ev.height == 0x64);

  /* Negative camera Z: t is the fixed 0x20 and the seed flips to +0x180.
   * prevZ = 0xE74 selects the threshold branch at height < 0x24. */
  cam = wall_camera(0, 0x20, -0xE74, 0, 8, 8, 1);
  cam.prev_z = 0xE74;
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
  assert(ev.seed_z == 0x180 && ev.height == 0x60);
  cam = wall_camera(0, 0x22, -0xE74, 0, 8, 8, 1);   /* t = 0x20 < 0x22 */
  cam.prev_z = 0xE74;
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_z == 8 && cam.speed == 8);

  /* The 32-bit magnitude is re-read as its sign-extended low word: |z| with a
   * 0x0E74 low word passes the gate and subtracts to zero. */
  cam = wall_camera(0, 0x24, 0x10E74, 0, 8, 8, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
  assert(ev.seed_z == -0x180 && ev.height == 0x64);
  /* Low word 0x0E00 < 0xE74 -> skipped. */
  cam = wall_camera(0, 0x24, 0x10E00, 0, 8, 8, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
}

/* Chain B (X), 0x71A5D..0x71AF0 and the 0x71AF2 small-X overshoot: the strict
 * `height > 0x24` corridor test, the positive-side fixed 0x20, both seeds and
 * the 0x6D4 lower band. */
static void test_wall_arm_x_chain_edges(void) {
  fifa96_camera_wall_event ev;
  fifa96_camera cam;

  /* |x| = 0x8BC, height 0x24: 0x71A76 JG is false, |prevX| = 0 < 0x8BC, so
   * the 0x71B1B damp runs (vel_x = shift_step(-0x10,1) = -8). */
  cam = wall_camera(0x8BC, 0x24, 0, 0x10, 0, 0x10, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_x == -8 && cam.speed == 8);

  /* height = 0x25 takes the threshold: t = 0x20 (camX >= 0) < 0x25, no event
   * and no damp -> the same pair is stored. */
  cam = wall_camera(0x8BC, 0x25, 0, 0x10, 0, 0x10, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_x == 0x10 && cam.speed == 0x10);

  /* |prevX| = 0x8BC takes the threshold at height 0x20: t = 0x20 = height. */
  cam = wall_camera(0x8BC, 0x20, 0, 0x10, 0, 0x10, 1);
  cam.prev_x = 0x8BC;
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
  assert(ev.seed_x == -0x180 && ev.seed_z == 0);
  assert(ev.height == 0x60);
  assert(cam.vel_x == 0x10 && cam.speed == 0x10);   /* dead damp registers */
  cam.prev_x = -0x8BC;
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);

  /* t = 0x20 < height, no event, no damp (the threshold arm does not damp). */
  cam = wall_camera(0xBC0, 0x26, 0, 0x10, 0, 0x10, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_x == 0x10 && cam.speed == 0x10);

  /* Negative camera X: the threshold uses the distance formula and the seed
   * flips to +0x180. |x| = 0x8BC -> t = 0x24; |x| = 0x8C0 -> t = 0x26. */
  cam = wall_camera(-0x8BC, 0x24, 0, 0x10, 0, 0x10, 1);
  cam.prev_x = 0x8BC;                              /* threshold, not damp */
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
  assert(ev.seed_x == 0x180 && ev.height == 0x64);
  cam = wall_camera(-0x8C0, 0x26, 0, 0x10, 0, 0x10, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);  /* t = 0x24+2 = 0x26 */
  assert(ev.seed_x == 0x180 && ev.height == 0x66);
  cam = wall_camera(-0x8C0, 0x27, 0, 0x10, 0, 0x10, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);  /* t = 0x26 < 0x27 */
  assert(cam.vel_x == 0x10 && cam.speed == 0x10);

  /* Small-X 0x71AF2: 0x6D4 <= |x| < 0x8BC, height < 0x1E, |prevX| < 0x6D4.
   * The 0x718A9 gate needs |z| > 0xB10 on this band (|x| <= 0x720 there). */
  cam = wall_camera(0x6D4, 0x1D, 0xB20, 0x10, 0, 0x10, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_x == -8 && cam.speed == 8);
  cam = wall_camera(0x6D4, 0x1E, 0xB20, 0x10, 0, 0x10, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_x == 0x10 && cam.speed == 0x10);
  cam = wall_camera(0x6D4, 0x1D, 0xB20, 0x10, 0, 0x10, 1);
  cam.prev_x = 0x6D4;
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_x == 0x10 && cam.speed == 0x10);
  cam = wall_camera(0x6D3, 0x1D, 0xB20, 0x10, 0, 0x10, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_x == 0x10 && cam.speed == 0x10);
  cam = wall_camera(0x8BB, 0x1D, 0, 0x10, 0, 0x10, 1);
  cam.prev_x = 0x6D4;                              /* 0x6D4 is not < 0x6D4 */
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_x == 0x10 && cam.speed == 0x10);
}

/* Both chains can flag the same frame: two laterals, one event, and the
 * entry speed still drives the announce bucket. */
static void test_wall_arm_both_chains(void) {
  fifa96_camera_wall_event ev;
  fifa96_camera cam = wall_camera(0x900, 0x20, -0xE80, 0x10, 0x10, 0x20, 0);
  cam.prev_x = 0x8BC;            /* chain B threshold instead of the damp */
  cam.prev_z = 0xE74;            /* chain A threshold instead of the damp */
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
  assert(ev.seed_x == -0x180 && ev.seed_z == 0x180);
  assert(ev.height == 0x60);
  assert(ev.announce == 3);      /* entry speed 0x20 > 0xA */
  assert(cam.vel_x == 0x10 && cam.vel_z == 0x10);   /* no store on flag 1 */
}

/* 0x71B37..0x71B5B: `speed <= 5` skips the 0x974DC call; the args are 1, 2, 3
 * for 6..8, 9..0xA, >0xA. The bucket reads the entry speed word (sign
 * extended), never the damped pair. */
static void test_wall_arm_announce_buckets(void) {
  fifa96_camera_wall_event ev;
  fifa96_camera cam;
  static const struct {
    uint16_t speed;
    uint8_t announce;
  } buckets[] = {
      {0, 0}, {5, 0}, {6, 1}, {8, 1}, {9, 2}, {0xA, 2}, {0xB, 3}, {0x30, 3},
      {0x8005, 0},   /* the native MOVSX DI reads the word as negative */
  };
  for (size_t i = 0; i < sizeof buckets / sizeof buckets[0]; i++) {
    cam = wall_camera(0x8BC, 0x20, 0, 0, 0, buckets[i].speed, 1);
    cam.prev_x = 0x8BC;          /* the threshold arm -> an event */
    assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
    assert(ev.announce == buckets[i].announce);
  }
  /* A flag-0 damp never announces, and the stored speed is the recomputed
   * length of the damped pair, not the entry speed. */
  cam = wall_camera(0x8BC, 0x10, 0, 0x10, 0, 9, 0);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(ev.announce == 0);
  assert(cam.vel_x == -8 && cam.speed == 8);
}

/* The word truncation traps: a 32-bit magnitude whose low word sign-extends
 * negative is < every positive bound, and the prev checks use the full dword. */
static void test_wall_arm_width_traps(void) {
  fifa96_camera_wall_event ev;
  fifa96_camera cam;

  /* |z| word 0x8000 is negative -> chain A skipped; chain B still fires. */
  cam = wall_camera(0x8BC, 0x20, 0x18000, 0, 0, 0x10, 1);
  cam.prev_x = 0x8BC;
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
  assert(ev.seed_x == -0x180 && ev.seed_z == 0);

  /* |x| word 0x8000 is negative -> chain B (and small-X) skipped. */
  cam = wall_camera(0x18000, 0x24, 0xE80, 0, 0, 0x10, 1);
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
  assert(ev.seed_z == -0x180 && ev.seed_x == 0);

  /* |prevZ| = 0x80000000 negates to itself and reads negative -> the corridor
   * damp runs; 0x7FFFFFFF passes the threshold branch instead. */
  cam = wall_camera(0, 0x10, 0xE80, 0, 8, 8, 1);
  cam.prev_z = (int32_t)0x80000000;
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_z == -4 && cam.speed == 4);
  cam = wall_camera(0, 0x10, 0xE80, 0, 8, 8, 1);
  cam.prev_z = 0x7FFFFFFF;
  assert(fifa96_camera_wall_arm(&cam, &ev) == 1);
  assert(ev.seed_z == -0x180);

  /* The same dword rule in chain B's prev check. */
  cam = wall_camera(0x8BC, 0x10, 0, 0x10, 0, 0x10, 1);
  cam.prev_x = (int32_t)0x80000000;
  assert(fifa96_camera_wall_arm(&cam, &ev) == 0);
  assert(cam.vel_x == -8 && cam.speed == 8);
}

static void test_wall_arm_invalid(void) {
  fifa96_camera_wall_event ev;
  fifa96_camera cam = wall_camera(0, 0, 0x1000, 0, 8, 8, 1);
  assert(fifa96_camera_wall_arm(NULL, &ev) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_wall_arm(&cam, NULL) == -FIFA96_ERR_INVALID);
}

/* 0x73756/0x7375b..0x7375d: the integrator copies the live triple to
 * 0x157758/5C/60, so prev_x/prev_z join prev_y unconditionally (before the
 * pause test). */
static void test_update_walk_captures_prev_triple(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 1, 2, 3);
  assert(cam.prev_x == 0 && cam.prev_y == 0 && cam.prev_z == 0);
  cam.paused = 1;                   /* the capture precedes the pause return */
  cam.pos_x = 0x11;
  cam.pos_y = 0x22;
  cam.pos_z = 0x33;
  assert(fifa96_camera_update(&cam, 4, 0, 0) == FIFA96_OK);
  assert(cam.prev_x == 0x11 && cam.prev_y == 0x22 && cam.prev_z == 0x33);
}

/* Slice-B Task 5: the native `0x73728..0x73750` word shadows, copied after the
 * `0x73723 CALL 0x92864` commit:
 *   [0x157825] = word[0x1577BE] (`speed`), [0x157827] = word[0x1577C0]
 *   (`vel_x`), [0x157829] = word[0x1577C2] (`vel_z`).
 * All three are plain 16-bit MOVs (no sign/width widening). `FUN_00092864`'s
 * ring append (the non-camera half) is a numbered leg, T5-L1. */
static void test_camera_commit_shadows(void) {
  fifa96_camera cam;
  memset(&cam, 0xAA, sizeof cam);
  assert(fifa96_camera_init(&cam, 0, 0, 0) == FIFA96_OK);
  /* init zeroes the three shadow cells ([0x157825]/[0x157827]/[0x157829]). */
  assert(cam.shadow_speed_157825 == 0);
  assert(cam.shadow_vel_157827 == 0);
  assert(cam.shadow_vel_157829 == 0);
  cam.speed = 0xAB12;
  cam.vel_x = -12345;
  cam.vel_z = 0x2345;
  assert(fifa96_camera_commit(&cam) == FIFA96_OK);
  assert(cam.shadow_speed_157825 == cam.speed);
  assert(cam.shadow_vel_157827 == cam.vel_x);
  assert(cam.shadow_vel_157829 == cam.vel_z);
  /* Pin the word width/sign: a top-bit speed copies as the raw unsigned word,
   * and a negative velocity copies as the raw signed 16-bit value. */
  cam.speed = 0x8000;
  cam.vel_x = 0x7FFF;
  cam.vel_z = -32768;
  assert(fifa96_camera_commit(&cam) == FIFA96_OK);
  assert(cam.shadow_speed_157825 == 0x8000);
  assert(cam.shadow_vel_157827 == 0x7FFF);
  assert(cam.shadow_vel_157829 == -32768);
  assert(fifa96_camera_commit(NULL) == -FIFA96_ERR_INVALID);
}

/* ---- Slice-B Task 6: the walk head jitter (0x736B2..0x73723) ---- */

/* 0x736ba/0x736bd and 0x736c3/0x736cd: `speed <= 0xC` or `pos_y <= 0xA0` is a
 * full no-op — no velocity change and no match-RNG draw. `NULL` cam is
 * -FIFA96_ERR_INVALID (a NULL rng on the accepted path is the same error). */
static void test_camera_walk_jitter_gates(void) {
  struct fifa96_rng rng;
  struct fifa96_rng snap;
  fifa96_camera cam = fresh_camera();

  /* speed 0xC (the JLE boundary) -> no-op even with pos_y above the band. */
  cam.pos_y = 0xB0;
  cam.speed = 0xC;
  cam.vel_x = 7;
  cam.vel_z = 9;
  assert(fifa96_rng_seed(&rng, 0x1234u) == FIFA96_OK);
  snap = rng;
  assert(fifa96_camera_walk_jitter(&cam, &rng) == FIFA96_OK);
  assert(cam.vel_x == 7 && cam.vel_z == 9);
  assert(memcmp(&rng, &snap, sizeof rng) == 0);   /* no draw */

  /* pos_y 0xA0 (the JLE boundary) -> no-op even with speed above 0xC. */
  cam.speed = 0x1E;
  cam.pos_y = 0xA0;
  cam.vel_x = 7;
  cam.vel_z = 9;
  assert(fifa96_rng_seed(&rng, 0x1234u) == FIFA96_OK);
  snap = rng;
  assert(fifa96_camera_walk_jitter(&cam, &rng) == FIFA96_OK);
  assert(cam.vel_x == 7 && cam.vel_z == 9);
  assert(memcmp(&rng, &snap, sizeof rng) == 0);   /* no draw */

  assert(fifa96_camera_walk_jitter(NULL, &rng) == -FIFA96_ERR_INVALID);
}

/* 0x736cf..0x73722: the draw chain. Each `draw` is one `fifa96_rng_step`
 * (FUN_00092AC8's new w[0] word). Seeded draws (the additive generator):
 *   seed 0x0000: 0x200 0x725 0x133F 0x2BBB 0x584D ...
 *   seed 0x002C: 0xBA01 0xD728 0x6344 0x13C3 0xA059 ...
 *   seed 0x1234: 0x0A01 0xB728 0x4345 ...
 * speed 0x10 (< 0x19): the modulo draw runs with EBX = 0x21 - 0x10 = 0x11.
 *   seed 0x2C: 0xBA01 % 0x11 == 0 -> proceed; 0xD728 & 7 == 0; 0x6344 & 1 == 0
 *   -> vel_x -= 1; 0x13C3 & 1 == 1 -> vel_z += 1 (four draws consumed).
 *   seed 0x1234: 0x0A01 % 0x11 == 11 != 0 -> no-op (one draw consumed).
 * speed 0x20 (>= 0x19): the modulo draw is skipped; seed 0: 0x200 & 7 == 0;
 *   0x725 & 1 == 1 -> vel_x += 1; 0x133F & 1 == 1 -> vel_z += 1 (three draws). */
static void test_camera_walk_jitter_draws(void) {
  struct fifa96_rng rng;
  uint16_t next;
  fifa96_camera cam = fresh_camera();

  /* < 0x19 accepted path: vel_x -1, vel_z +1, four draws consumed. */
  cam.pos_y = 0xB0;
  cam.speed = 0x10;
  cam.vel_x = 10;
  cam.vel_z = -10;
  assert(fifa96_rng_seed(&rng, 0x2Cu) == FIFA96_OK);
  assert(fifa96_camera_walk_jitter(&cam, &rng) == FIFA96_OK);
  assert(cam.vel_x == 9 && cam.vel_z == -9);
  assert(fifa96_rng_step(&rng, &next) == FIFA96_OK);
  assert(next == 0xA059);   /* the fifth draw: all four were consumed */

  /* < 0x19 nonzero-remainder no-op: only the modulo draw is consumed. */
  cam.pos_y = 0xB0;
  cam.speed = 0x10;
  cam.vel_x = 10;
  cam.vel_z = -10;
  assert(fifa96_rng_seed(&rng, 0x1234u) == FIFA96_OK);
  assert(fifa96_camera_walk_jitter(&cam, &rng) == FIFA96_OK);
  assert(cam.vel_x == 10 && cam.vel_z == -10);
  assert(fifa96_rng_step(&rng, &next) == FIFA96_OK);
  assert(next == 0xB728);   /* the second draw: only one was consumed */

  /* >= 0x19 accepted path: no modulo draw, three draws (the & 7 and two & 1). */
  cam.pos_y = 0xB0;
  cam.speed = 0x20;
  cam.vel_x = 10;
  cam.vel_z = -10;
  assert(fifa96_rng_seed(&rng, 0u) == FIFA96_OK);
  assert(fifa96_camera_walk_jitter(&cam, &rng) == FIFA96_OK);
  assert(cam.vel_x == 11 && cam.vel_z == -9);
  assert(fifa96_rng_step(&rng, &next) == FIFA96_OK);
  assert(next == 0x2BBB);   /* the fourth draw: three were consumed */
}

/* M2 phase-10 T2 (FU-147 §3.3 leg 7 / FU-152 §2.9): FUN_00070C08 — the
 * per-record camera event FUN_0007BF20's tail (0x7C8FC) runs for every
 * record. First-hand body 0x70C08..0x70DDD (disassemble_bytes this task). */
static void test_record_event_reflect_and_event(void) {
  fifa96_camera cam = fresh_camera();
  struct fifa96_rng rng;
  fifa96_camera_record_event_in in;
  fifa96_camera_record_event_out out;
  memset(&in, 0, sizeof in);
  assert(fifa96_camera_init(&cam, 0x100, 0, 0x200) == FIFA96_OK);
  cam.target_x = 0x100;                       /* origin == target (delta 0) */
  cam.target_z = 0x200;
  cam.event_step_x = 30;                      /* [0x1577BA] step pair */
  cam.event_step_z = 40;                      /* [0x1577BC] */
  cam.vel_x = 3;                              /* [0x1577C0] */
  cam.vel_z = 4;                              /* [0x1577C2] */
  cam.speed = 5;                              /* [0x1577BE] bearing */
  cam.event_param = 0x20;                     /* [0x1577F0] */
  assert(fifa96_rng_seed(&rng, 0x1234u) == FIFA96_OK);
  in.skip_9a = 0;
  in.action_91 = 3;
  in.height_5d = 0;
  in.ball_height = 0x10;
  /* v1 = 30>>2 = 7, v2 = 40>>2 = 10; sext(0 + 0x68) = 0x68 > ball 0x10 ->
   * reflect (0x70CFA..0x70D03): e1 = |vel_x| 3 (0x70D4D dword[0x1577BE]>>16 =
   * word[0x1577C0]) <= e2 = |vel_z| 4 (0x70D6D dword[0x1577C0]>>16 =
   * word[0x1577C2]) -> negate v2 (0x70D81 CMP; JLE). The speed 5 staging
   * pins that [0x1577BE] is not the e1 magnitude. */
  assert(fifa96_camera_record_event(&cam, &rng, &in, &out) == 1);
  assert(out.applied == 1);
  assert(out.sink == 1 && out.sink_code == 0x1C);
  assert(out.vec_1 == 7 && out.vec_2 == -10);
  assert(out.distance == fifa96_entity_distance(7, -10));
  assert(cam.event_param == 0x10);            /* event_param := ball height */
  assert(cam.timer_limit != 0);               /* the FUN_00070544 ramp ran */
  /* the |vel_x| > |vel_z| case (a negative vel_x pins the sign-gate
   * absolute value: 0x70D43 CMP word[0x1577C0],0; JL -> NEG) negates v1. */
  assert(fifa96_camera_init(&cam, 0x100, 0, 0x200) == FIFA96_OK);
  cam.target_x = 0x100;
  cam.target_z = 0x200;
  cam.event_step_x = 30;
  cam.event_step_z = 40;
  cam.vel_x = -5;
  cam.vel_z = 3;
  cam.speed = 5;
  cam.event_param = 0x20;
  assert(fifa96_camera_record_event(&cam, &rng, &in, &out) == 1);
  assert(out.vec_1 == -7 && out.vec_2 == 10);
  assert(cam.event_param == 0x10);
}

static void test_record_event_height_ramp(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_record_event_in in;
  fifa96_camera_record_event_out out;
  memset(&in, 0, sizeof in);
  assert(fifa96_camera_init(&cam, 0x100, 0, 0x200) == FIFA96_OK);
  cam.target_x = 0x100;
  cam.target_z = 0x200;
  cam.event_step_x = 30;
  cam.event_step_z = 40;
  cam.vel_x = 3;
  cam.vel_z = 4;
  cam.speed = 5;
  in.skip_9a = 0;
  in.action_91 = 3;
  in.ball_height = 0x100;
  in.height_5d = 0x90;                /* ball - 0x70: the ramp band's lower edge */
  cam.event_param = 0x20;
  assert(fifa96_camera_record_event(&cam, NULL, &in, &out) == 1);
  /* sext(0x90 + 0x68) = 0xF8 <= ball -> the ramp arm toward 0x90 + 0x70 =
   * 0x100: new = 0x100 + ((0x20 - 0x100) >> 1) = 0x90; the second step is
   * skipped (0x100 < 0x90 false) and the seeds are NOT reflected. */
  assert(out.vec_1 == 7 && out.vec_2 == 10);
  assert(cam.event_param == 0x90);
  assert(out.applied == 1);
  /* the band's upper edge (ball - 0x68) still ramps: 0x98 + 0x68 == ball. */
  assert(fifa96_camera_init(&cam, 0x100, 0, 0x200) == FIFA96_OK);
  cam.target_x = 0x100;
  cam.target_z = 0x200;
  cam.event_step_x = 30;
  cam.event_step_z = 40;
  cam.vel_x = 3;
  cam.vel_z = 4;
  cam.speed = 5;
  cam.event_param = 0x180;
  in.height_5d = 0x98;
  assert(fifa96_camera_record_event(&cam, NULL, &in, &out) == 1);
  /* target = 0x98 + 0x70 = 0x108; the two-step case:
   * new = 0x108 + ((0x180 - 0x108) >> 1) = 0x144; target 0x108 < 0x144 ->
   * the second step: 0x144 + ((0x144 - 0x108) >> 1) = 0x162. */
  assert(cam.event_param == 0x162);
  assert(out.vec_1 == 7 && out.vec_2 == 10);
  /* one past the upper edge reflects: 0x99 + 0x68 = 0x101 > ball. */
  assert(fifa96_camera_init(&cam, 0x100, 0, 0x200) == FIFA96_OK);
  cam.target_x = 0x100;
  cam.target_z = 0x200;
  cam.event_step_x = 30;
  cam.event_step_z = 40;
  cam.vel_x = 3;
  cam.vel_z = 4;
  cam.speed = 5;
  cam.event_param = 0x20;
  in.height_5d = 0x99;
  assert(fifa96_camera_record_event(&cam, NULL, &in, &out) == 1);
  /* e1 = |vel_x| 3 <= e2 = |vel_z| 4 negates v2; event_param := ball. */
  assert(out.vec_1 == 7 && out.vec_2 == -10);
  assert(cam.event_param == 0x100);
}

/* M2 phase-29 T4 (FU306 §5.2 item 1 / erratum 1, closes FU306-L2): the
 * chaser's FUN_00071C94 call passes `ECX = 1` (native `0x70DBA MOV ECX,1`,
 * preserved by the `0x92820` sink's PUSH/POP ECX). The ramp sign is
 * observable in F6/F8: camY 0x10 and the reflected event height 0x20 give
 * F2 = ramp(0x20) = 21, ramp(0x10) = 15, so ECX=1 gives
 * F6 = 21 + 15 = 36, F8 = 42 - 36 = 6; the old ECX=0 gives F6 = 6, F8 = 36
 * (and the two seeds floor to zero velocity at the F8 = 36 divide). */
static void test_record_event_chaser_ramp_param_one(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_record_event_in in;
  fifa96_camera_record_event_out out;
  memset(&in, 0, sizeof in);
  assert(fifa96_camera_init(&cam, 0x100, 0x10, 0x200) == FIFA96_OK);
  cam.target_x = 0x100;
  cam.target_z = 0x200;
  cam.event_step_x = 30;                      /* v1 = 7 */
  cam.event_step_z = 40;                      /* v2 = 10, reflected to -10 */
  cam.event_param = 0x20;
  in.action_91 = 3;
  in.height_5d = 0;
  in.ball_height = 0x20;                      /* 0x68 > 0x20 -> reflect arm */
  assert(fifa96_camera_record_event(&cam, NULL, &in, &out) == 1);
  assert(out.applied == 1);
  assert(out.vec_1 == 7 && out.vec_2 == -10);
  assert(cam.event_param == 0x20);            /* re-seeded to the ball height */
  assert(cam.event_peak == 21);
  assert(cam.timer == 36);                    /* F6 = F2 + ramp(0x10) */
  assert(cam.timer_limit == 42);              /* F4 = 2 * F2 */
  assert(cam.ramp_divisor == 6);              /* F8 = F4 - F6 */
  assert(cam.band_1577f6 == 36);              /* [0x1577F6] = F6 */
  assert(cam.vel_x == 1 && cam.vel_z == -1);  /* the seeds at F8 = 6 */
}

/* M2 phase-29 T4 (FU306 §5.2 item 3, closes FU306-L4): the native stores the
 * seed difference as a word and 0x8DC50 sign-tests AX (CWDE), so the
 * 16-bit wrap applies before the shift. 0x7FFF - (0 - 0x8000) is 0xFFFF
 * (0x8DC50(-1) = 0); 0x0001 - (0 - 0x8000) is 0x8001 -> 0x8DC50(-0x7FFF) =
 * -0x1FFF. The un-wrapped 32-bit port returns 0x3FFF / 0x2000. */
static void test_record_event_seed_word_wrap(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_record_event_in in;
  fifa96_camera_record_event_out out;
  memset(&in, 0, sizeof in);
  in.action_91 = 3;
  in.height_5d = 0;
  in.ball_height = 0x100;                     /* the ramp arm: no reflection */
  assert(fifa96_camera_init(&cam, 0, 0, 0) == FIFA96_OK);
  cam.target_x = 0x8000;                      /* camX - targetX = -0x8000 */
  cam.event_step_x = 0x7FFF;
  cam.event_step_z = 40;                      /* v2 = 10 (non-zero: no jitter) */
  cam.event_param = 0x20;
  assert(fifa96_camera_record_event(&cam, NULL, &in, &out) == 1);
  assert(out.vec_1 == 0);
  assert(out.vec_2 == 10);
  assert(fifa96_camera_init(&cam, 0, 0, 0) == FIFA96_OK);
  cam.target_x = 0x8000;                      /* camX - targetX = -0x8000 */
  cam.event_step_x = 0x0001;                  /* 1 - (-0x8000) wraps to 0x8001 */
  cam.event_step_z = 40;
  cam.event_param = 0x20;
  assert(fifa96_camera_record_event(&cam, NULL, &in, &out) == 1);
  assert(out.vec_1 == -0x1FFF);
  assert(out.vec_2 == 10);
}

static void test_record_event_jitter_and_gates(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_record_event_in in;
  fifa96_camera_record_event_out out;
  struct fifa96_rng rng;
  struct fifa96_rng expect;
  memset(&in, 0, sizeof in);
  uint16_t d1 = 0, d2 = 0, d3 = 0;
  int16_t t;
  assert(fifa96_camera_init(&cam, 0, 0, 0) == FIFA96_OK);
  cam.target_x = 0;
  cam.target_z = 0;
  cam.event_step_x = 0;                       /* v1 == v2 == 0 -> the jitter */
  cam.event_step_z = 0;
  cam.speed = 5;
  in.skip_9a = 0;
  in.action_91 = 3;
  in.height_5d = 0;
  in.ball_height = 0x10;
  assert(fifa96_rng_seed(&rng, 0x1234u) == FIFA96_OK);
  assert(fifa96_rng_seed(&expect, 0x1234u) == FIFA96_OK);
  assert(fifa96_camera_record_event(&cam, &rng, &in, &out) == 1);
  assert(fifa96_rng_step(&expect, &d1) == FIFA96_OK);
  assert(fifa96_rng_step(&expect, &d2) == FIFA96_OK);
  assert(fifa96_rng_step(&expect, &d3) == FIFA96_OK);
  t = (int16_t)((int32_t)(d1 & 0x3Fu) + 0x30);
  /* t = (draw1 & 0x3F) + 0x30 with the draw2/draw3 signs; then the reflect
   * arm (the 0/0 corner: e1 = |vel_x| 0, e2 = |vel_z| 0; 0 > 0 false ->
   * negate v2). */
  assert(out.vec_1 == ((d2 & 1u) ? t : (int16_t)-t));
  assert(out.vec_2 == -((d3 & 1u) ? t : (int16_t)-t));
  assert(out.vec_1 != 0 || out.vec_2 != 0);
  /* a NULL rng on the jitter path is an error. */
  assert(fifa96_camera_init(&cam, 0, 0, 0) == FIFA96_OK);
  assert(fifa96_camera_record_event(&cam, NULL, &in, &out) ==
         -FIFA96_ERR_INVALID);
  /* the record gates. */
  assert(fifa96_camera_init(&cam, 0, 0, 0) == FIFA96_OK);
  in.skip_9a = 1;
  assert(fifa96_camera_record_event(&cam, &rng, &in, &out) == 0);
  assert(out.applied == 0);
  in.skip_9a = 0;
  in.action_91 = 0x22;
  in.ball_height = 0x1F;
  assert(fifa96_camera_record_event(&cam, &rng, &in, &out) == 0);
  in.ball_height = 0x20;                      /* the boundary passes */
  assert(fifa96_camera_record_event(&cam, &rng, &in, &out) == 1);
  assert(fifa96_camera_record_event(NULL, &rng, &in, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_record_event(&cam, &rng, NULL, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_record_event(&cam, &rng, &in, NULL) ==
         -FIFA96_ERR_INVALID);
}

/* FU-152 §2.9 / FU-71 (T3): the tracked record ([0x1577CA]) is reset by the
 * event reset FUN_000700F4 0x70258 (inside fifa96_camera_event_set) and set by
 * FUN_00071C94 0x71D27. The engine binds the event record through
 * fifa96_camera_set_tracked. */
static void test_tracked_record(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  assert(cam.tracked == FIFA96_CAMERA_TRACKED_NONE);
  assert(fifa96_camera_set_tracked(&cam, 9) == FIFA96_OK);
  assert(cam.tracked == 9);
  assert(fifa96_camera_set_tracked(&cam, FIFA96_CAMERA_TRACKED_NONE) == FIFA96_OK);
  assert(cam.tracked == FIFA96_CAMERA_TRACKED_NONE);
  assert(fifa96_camera_set_tracked(&cam, -2) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_set_tracked(NULL, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_set_tracked(&cam, 9) == FIFA96_OK);
  assert(fifa96_camera_event_set(&cam, 0, 2000, 0x30, 0) == FIFA96_OK);
  assert(cam.tracked == FIFA96_CAMERA_TRACKED_NONE);   /* the reset clears it */
}

/* ---- FU-152 §2.9 (T2): FUN_00070DE0 boundary reposition (derived core) ---- */

static void test_reposition_flag_overlap_noop(void) {
  /* Both triples classify inside the same zone: flags overlap -> return
   * without touching the camera. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 10, 0, 20);
  uint8_t events = 0;
  uint8_t code = 0;
  assert(fifa96_camera_reposition(&cam, 0, 0, 0, NULL, NULL, &events, &code,
                                  NULL) == 0);
  assert(cam.pos_x == 10 && cam.pos_z == 20);
  assert(events == 0 && code == 0);
}

static void test_reposition_steps_back(void) {
  /* The current z is beyond the mouth band (flags 4) while the previous
   * triple was in it (flags 0): the flags are disjoint and bit 8 is clear,
   * so the step path walks from the previous point toward the current one
   * while the candidate keeps the previous (mouth) class — landing just
   * under 0xB90. The bit-clear path emits no stat event and no corner
   * request (both live only in the bit-0x8-set boundary arm). */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0xBA0);
  uint8_t events = 0xFF;
  uint8_t code = 0xFF;
  fifa96_camera_stat_events stats;
  memset(&stats, 0, sizeof stats);
  assert(fifa96_camera_reposition(&cam, 0, 0, 0xB20, NULL, NULL, &events, &code,
                                  &stats) == 1);
  assert(cam.pos_z == 0xB8F);          /* 0xB20 + 0x40 + 0x20 + 0x8 + 0x4 + 0x2 + 0x1 */
  assert(cam.pos_z < 0xB90 && cam.pos_z > 0xB20);
  assert(events == 0 && code == 0);    /* no corner request on the bit-clear path */
  assert(stats.count == 0 && stats.overflow == 0);   /* no stat event either */
}

static void test_reposition_invalid(void) {
  uint8_t events = 0;
  uint8_t code = 0;
  assert(fifa96_camera_reposition(NULL, 0, 0, 0, NULL, NULL, &events, &code,
                                  NULL) == -FIFA96_ERR_INVALID);
}

/* Slice-B Task 3 (M2 m 42 ruling): the native same-zone guard. Both triples
 * classify to the same zone (inside the mouth band [0xB10,0xB90): flags 0 ==
 * 0), so the native `0x71101 CMP AX,word[ESP+0x34]; 0x71106 JZ 0x71302`
 * skips the step-back entirely and the camera stays at the current point. */
static void test_reposition_same_zone_no_step(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0xB28);
  uint8_t events = 0;
  uint8_t code = 0;
  assert(fifa96_camera_reposition(&cam, 0, 0, 0xB20, NULL, NULL, &events, &code,
                                  NULL) == 1);
  assert(cam.pos_x == 0 && cam.pos_y == 0 && cam.pos_z == 0xB28);  /* no step */
  assert(events == 0 && code == 0);
}

/* Slice-B Task 4: the bit-0x8 boundary arm and its two +3 tracked events. */

/* 0x70f2c CMP [ESP+0x28],0x90 (boundary_y > 0x90) -> 0x70f44. prev (0,0x90,0xB00)
 * is inside (flags 8); cur (0,0xA0,0xB40) is in the mouth band (flags 0), so
 * changed = 8 and the boundary arm runs. target_z = 0xB11 (prev not inside),
 * dz = 0x40, dz_word = 0x11 -> bnd = (0, 0x90 + 0x10*0x11/0x40 = 0x94, 0xB11),
 * which classifies 0 (clean), so the high arm emits {0x70F44,3,TRACKED} and
 * repositions to (0, 0x94, 0xAD4). */
static void test_reposition_boundary_stat_high(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_stat_events stats;
  uint8_t events = 0, code = 0;
  memset(&stats, 0, sizeof stats);
  fifa96_camera_init(&cam, 0, 0xA0, 0xB40);
  assert(fifa96_camera_reposition(&cam, 0, 0x90, 0xB00, NULL, NULL, &events,
                                  &code, &stats) == 1);
  assert(stats.count == 1 && stats.overflow == 0);
  assert(stats.event[0].site == 0x70F44);
  assert(stats.event[0].delta == 3);
  assert(stats.event[0].entity == FIFA96_CAMERA_STAT_TRACKED);
  assert(events == 0x49 && code == 0x40);   /* the 0x8f188(0x49, tracked, 0x40) */
  assert(cam.pos_x == 0 && cam.pos_y == 0x94 && cam.pos_z == 0xAD4);
  /* 0x70f6b..0x70fa1: |vel_z|=0 -> max(4, 0) = 4, cur_z >= 0 negates -> -4. */
  assert(cam.vel_z == -4);
}

/* 0x70fac arm: boundary_y <= 0x90 && boundary_x < -0xC0 -> 0x70fce. prev
 * (0,0x80,0xB00) flags 8; cur (-0x300,0x80,0xB40) flags 1 (x < -0xD0), so
 * changed = 9. bnd = (-0x300*0x11/0x40 = -0xCC, 0x80, 0xB11), which classifies
 * 0 (x -0xCC is >= -0xD0), so the low arm emits {0x70FCE,3,TRACKED} and
 * repositions to (-0xCC, 0x80, 0xAD4). */
static void test_reposition_boundary_stat_low(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_stat_events stats;
  uint8_t events = 0, code = 0;
  memset(&stats, 0, sizeof stats);
  fifa96_camera_init(&cam, -0x300, 0x80, 0xB40);
  assert(fifa96_camera_reposition(&cam, 0, 0x80, 0xB00, NULL, NULL, &events,
                                  &code, &stats) == 1);
  assert(stats.count == 1 && stats.overflow == 0);
  assert(stats.event[0].site == 0x70FCE);
  assert(stats.event[0].delta == 3);
  assert(stats.event[0].entity == FIFA96_CAMERA_STAT_TRACKED);
  assert(events == 0x48 && code == 0x40);   /* the 0x8f188(0x48, tracked, 0x40) */
  assert(cam.pos_x == -0xCC && cam.pos_y == 0x80 && cam.pos_z == 0xAD4);
}

/* boundary_y <= 0x90 with boundary_x inside [-0xC0, 0xC1]: no stat event. prev
 * (0,0x80,0xB00) flags 8; cur (0,0x80,0xB40) flags 0 -> changed = 8. bnd =
 * (0, 0x80, 0xB11) classifies 0, neither arm fires, so the 0x710df fallback
 * reclassifies bnd as the new prev and the step-back sees same-zone (0 == 0) ->
 * no step, camera stays at (0,0x80,0xB40). */
static void test_reposition_boundary_no_event(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_stat_events stats;
  uint8_t events = 0, code = 0;
  memset(&stats, 0, sizeof stats);
  fifa96_camera_init(&cam, 0, 0x80, 0xB40);
  assert(fifa96_camera_reposition(&cam, 0, 0x80, 0xB00, NULL, NULL, &events,
                                  &code, &stats) == 1);
  assert(stats.count == 0);
  assert(events == 0 && code == 0);
  assert(cam.pos_x == 0 && cam.pos_y == 0x80 && cam.pos_z == 0xB40);
}

/* ---- FU-152 §2.9 (T2) + FU303/FU294-L2 (phase-28 T8): FUN_00071DF4's
 * table/keeper arm — the 0x10E169 compass + 0x11042B/0x11042C rate pair ---- */

static void test_rate_table_arm(void) {
  /* The arm resolves row = 0x10E169[dir_x + 4*dir_z], idx = (int8_t)(row +
   * (rate_byte << 3)) and rate = 0x11042B[2*idx]; the keeper gate is
   * event_byte == 2, height >= 0xC1, |anchor_x| <= 0x23F, rate_z != 0 ->
   * 0x1D when approaching, else 0x1E. Gates: rate_byte >= 0, timer <= 0x1E.
   * (The pre-T8 body wrote the caller pair; the DIRS below are the live
   * sub-object +0x20/+0x21 bytes.) */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  uint8_t keeper = 0;
  /* dir byte 0, slot dir (1,1) -> row 1, idx 1 -> pair (1,0). */
  assert(fifa96_camera_rate_table(&cam, 0xC1, 0, 2, 1, 1, 1, 2, 0x23F, -1, 1,
                                  &keeper) == 1);
  assert(cam.rate_x == 1 && cam.rate_z == 0);
  assert(keeper == 0);                 /* rate_z == 0 -> no keeper event */
  /* dir byte 1, slot dir (1,0) -> row 2, idx 10 -> pair (0,-1); pos_z < 0
   * and vel_z >= 1 is the approaching arm -> 0x1D. */
  keeper = 0;
  assert(fifa96_camera_rate_table(&cam, 0xC1, 1, 2, 1, 1, 0, 2, 0x23F, -1, 1,
                                  &keeper) == 1);
  assert(cam.rate_x == 0 && cam.rate_z == -1);
  assert(keeper == 0x1D);
  /* A diagonal row is the (0,0) pair: dir byte 2, slot dir (1,0) -> row 2,
   * idx 18 -> (0,0); the write still applies but no keeper code. */
  keeper = 0;
  assert(fifa96_camera_rate_table(&cam, 0xC1, 2, 2, 1, 1, 0, 2, 0x23F, -1, 1,
                                  &keeper) == 1);
  assert(cam.rate_x == 0 && cam.rate_z == 0);
  assert(keeper == 0);
  /* dir byte 7, slot dir (0,1) -> row 0, idx 56 -> pair (0,1); the keeper
   * gate's non-approaching arm (pos_z == 0) -> 0x1E. */
  keeper = 0;
  assert(fifa96_camera_rate_table(&cam, 0xC1, 7, 2, 1, 0, 1, 2, 0x23F, 0, 1,
                                  &keeper) == 1);
  assert(cam.rate_x == 0 && cam.rate_z == 1);
  assert(keeper == 0x1E);
  /* The (0,0) sub-object pair has no compass row -> no rate write. */
  keeper = 0;
  assert(fifa96_camera_rate_table(&cam, 0xC1, 0, 2, 1, 0, 0, 2, 0x23F, -1, 1,
                                  &keeper) == 0);
  /* An out-of-domain pair has no compass row -> no rate write (FU303-L11:
   * the native reads neighbouring image data for such an index). */
  cam.rate_x = 3;
  cam.rate_z = 4;
  keeper = 0;
  assert(fifa96_camera_rate_table(&cam, 0xC1, 0, 2, 1, 2, 0, 2, 0x23F, 0, 0,
                                  &keeper) == 0);
  assert(cam.rate_x == 3 && cam.rate_z == 4);
  /* The signed dir-byte gate and the timer gate precede the lookup. */
  assert(fifa96_camera_rate_table(&cam, 0xC1, -1, 2, 1, 1, 0, 2, 0x23F, -1, 1,
                                  &keeper) == 0);
  assert(fifa96_camera_rate_table(&cam, 0xC1, 0, 0x1F, 1, 1, 0, 2, 0x23F, -1, 1,
                                  &keeper) == 0);
  /* Keeper gates: event byte, height, |anchor_x|. */
  keeper = 0;
  assert(fifa96_camera_rate_table(&cam, 0xC1, 1, 2, 1, 1, 0, 1, 0x23F, -1, 1,
                                  &keeper) == 1);
  assert(keeper == 0);
  keeper = 0;
  assert(fifa96_camera_rate_table(&cam, 0xC0, 1, 2, 1, 1, 0, 2, 0x23F, -1, 1,
                                  &keeper) == 1);
  assert(keeper == 0);
  keeper = 0;
  assert(fifa96_camera_rate_table(&cam, 0xC1, 1, 2, 1, 1, 0, 2, 0x240, -1, 1,
                                  &keeper) == 1);
  assert(keeper == 0);
  /* subobj_present == 0 skips the rate lookup/write but still runs the keeper
   * gate over the STORED `[0x157817]` cell (0x71fac reads the cell, not a
   * local): stage a stale positive rate_z and check it is untouched. */
  cam.rate_x = 5;
  cam.rate_z = 5;
  keeper = 0;
  assert(fifa96_camera_rate_table(&cam, 0xC1, 1, 2, 0, 1, 0, 2, 0x23F, 1, -1,
                                  &keeper) == 1);
  assert(cam.rate_x == 5 && cam.rate_z == 5);
  assert(keeper == 0x1D);              /* rate_z > 0, pos_z > 0, vel_z < 0 */
  assert(fifa96_camera_rate_table(NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL) ==
         -FIFA96_ERR_INVALID);
}

/* FU303/FU294-L2 (T8): FUN_00071C94's actor dir arm (0x71d70..0x71ddc). */
static void test_event_dir_byte(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  /* no actor (`[rec+0x20] == 0`): the 0x71d6a default 0 stands. */
  assert(fifa96_camera_event_dir(&cam, 0, 1, 1) == FIFA96_OK);
  assert(cam.dir_157815 == 0);
  /* the 0x10E169 compass rows: (1,1)->1, (1,-1)->3, (0,-1)->4,
   * (-1,0)->6, (-1,1)->7. */
  assert(fifa96_camera_event_dir(&cam, 1, 1, 1) == FIFA96_OK);
  assert(cam.dir_157815 == 1);
  assert(fifa96_camera_event_dir(&cam, 1, 1, -1) == FIFA96_OK);
  assert(cam.dir_157815 == 3);
  assert(fifa96_camera_event_dir(&cam, 1, 0, -1) == FIFA96_OK);
  assert(cam.dir_157815 == 4);
  assert(fifa96_camera_event_dir(&cam, 1, -1, 0) == FIFA96_OK);
  assert(cam.dir_157815 == 6);
  assert(fifa96_camera_event_dir(&cam, 1, -1, 1) == FIFA96_OK);
  assert(cam.dir_157815 == 7);
  /* an out-of-domain pair resolves to the 0xFF marker: the live slot writer
   * never yields one (T1 0x10E1DC has no path to T3's dead entries 9..15, so
   * +0x20/+0x21 stay in {-1,0,1}); FU303-L11 residual. */
  assert(fifa96_camera_event_dir(&cam, 1, 2, 0) == FIFA96_OK);
  assert((uint8_t)cam.dir_157815 == 0xFF);
  /* the (0,0) pair with a live bearing word: the 0x8DD70 oct over the
   * post-ramp velocity pair. angle(25,0) = 0x100 -> ((0x100+0x40)&0x3FF)>>7
   * = 2. */
  cam.vel_x = 25;
  cam.vel_z = 0;
  cam.speed = 25;
  assert(fifa96_camera_event_dir(&cam, 1, 0, 0) == FIFA96_OK);
  assert(cam.dir_157815 == 2);
  /* the (0,0) pair with a zero bearing word: the 0x71db1 0xFF store. */
  cam.speed = 0;
  assert(fifa96_camera_event_dir(&cam, 1, 0, 0) == FIFA96_OK);
  assert((uint8_t)cam.dir_157815 == 0xFF);
  /* (1,0) -> 2; the setter's default store clears the cell. */
  assert(fifa96_camera_event_dir(&cam, 1, 1, 0) == FIFA96_OK);
  assert(cam.dir_157815 == 2);
  assert(fifa96_camera_event_set(&cam, 0, 0, 0x30, 0) == FIFA96_OK);
  assert(cam.dir_157815 == 0);
  assert(fifa96_camera_event_dir(NULL, 0, 0, 0) == -FIFA96_ERR_INVALID);
}

/* FU303/FU292-L2 (T8): F6 = timer_limit - ramp_divisor (the 0x705e8
 * `F8 = F4 - F6` identity), both arms. */
static void test_event_band_f6(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0x20, 0);
  assert(fifa96_camera_event_set(&cam, 0, 0, 0x60, 0) == FIFA96_OK);
  /* F2 = 0x24, F4 = 0x48; F6 = 0x24 - ramp(0x60-0x20=0x40) = 0x24 - 0x1D = 7,
   * F8 = 0x48 - 7 = 0x41. */
  assert(cam.timer_limit == 0x48);
  assert(cam.ramp_divisor == 0x41);
  assert(cam.timer == 7);
  assert(cam.band_1577f6 == 7);
  assert(cam.band_1577f6 ==
         (int16_t)((int16_t)cam.timer_limit - (int16_t)cam.ramp_divisor));
  /* the idle arm (height clamps to 0 with the camera at y 0): F2=6,
   * F4=F8=0xC, F6=0. */
  fifa96_camera_init(&cam, 0, 0, 0);
  assert(fifa96_camera_event_set(&cam, 0, 0, 0, 0) == FIFA96_OK);
  assert(cam.timer_limit == 0xC && cam.ramp_divisor == 0xC);
  assert(cam.band_1577f6 == 0);
}

/* ---- FU-152 §2.8 (P4): camera type mapping and the handler bodies ---- */

static void test_camera_type_mapping(void) {
  /* The first-hand 0x107508 record fields {+4 handler, +0xC behavior index}:
   * idx0 {0,0}, idx1 {1,1}, idx2 {2,2}, idx3 {-1,-1}, idx4 {-1,-1},
   * idx5 {3,-1}. The native maps +4==3 -> behavior 5 and clamps a negative
   * +4 to 0; the behavior -1 image values are the degenerate read the engine
   * clamps to 0 (slice §2.8 leg 8). */
  const fifa96_camera_type_record records[6] = {
      {0, 0}, {1, 1}, {2, 2}, {-1, -1}, {-1, -1}, {3, -1},
  };
  int32_t handler = -9, behavior = -9;
  assert(fifa96_camera_type(records, 6, 0, &handler, &behavior) == FIFA96_OK);
  assert(handler == 0 && behavior == 0);
  assert(fifa96_camera_type(records, 6, 1, &handler, &behavior) == FIFA96_OK);
  assert(handler == 1 && behavior == 1);
  assert(fifa96_camera_type(records, 6, 2, &handler, &behavior) == FIFA96_OK);
  assert(handler == 2 && behavior == 2);
  /* selector 3: handler clamps -1 -> 0; the behavior record index is the
   * remapped +4 = -1, clamped to record 0 -> behavior -1 -> 0. */
  assert(fifa96_camera_type(records, 6, 3, &handler, &behavior) == FIFA96_OK);
  assert(handler == 0 && behavior == 0);
  assert(fifa96_camera_type(records, 6, 4, &handler, &behavior) == FIFA96_OK);
  assert(handler == 0 && behavior == 0);
  /* selector 5: handler 3; behavior record index 3 -> 5, whose +0xC is -1
   * -> clamp 0. */
  assert(fifa96_camera_type(records, 6, 5, &handler, &behavior) == FIFA96_OK);
  assert(handler == 3 && behavior == 0);
  assert(fifa96_camera_type(records, 6, 6, &handler, &behavior) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_type(NULL, 6, 0, &handler, &behavior) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_type(records, 6, 0, NULL, &behavior) ==
         -FIFA96_ERR_INVALID);
}

static void test_behavior_steady(void) {
  /* FUN_0004E834: the yaw step FUN_0004DF34 and the native horizon jitter
   * (-11 * block[0]) >> 3 + 0xE09 / (-13 * block[0]) >> 3 + 0xAA1. The
   * pitch/y/ratio arms are pinned by test_behavior_steady_pitch. */
  fifa96_camera_handler_state st;
  fifa96_camera_handler_block block;
  fifa96_camera_handler_out out;
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  memset(&out, 0, sizeof out);
  block.target0 = 0x340;
  block.target1 = 0;
  block.class_of = 3;
  block.const20 = 8;
  block.const24 = 8;
  block.const28 = 0;
  st.yaw = 0;
  assert(fifa96_camera_behavior_steady(&st, &block, 0, &out) == FIFA96_OK);
  assert(st.yaw == 0);
  assert(out.horizon_a == 0x991 && out.horizon_b == 0x559);

  /* FUN_0004DF34: target +x, class-3 point z = -0xB10, ratio 1000, rate 8,
   * tick 8. The blend is 0xBE0C, delta -0x41F4, ramp 0x1111 -> -1126, then
   * the ratio/2+ratio/4 catch-up band (15634) returns -15634. */
  block.target1 = 1000;
  block.const28 = 0;   /* the two slides are no-ops for this arm */
  st.pos_x = 0;
  st.pos_z = 0;
  st.track_x = 1000;
  st.track_z = 0;
  st.ratio = 1000;
  st.yaw = 0;
  assert(fifa96_camera_behavior_steady(&st, &block, 8, &out) == FIFA96_OK);
  assert(st.yaw == 0xC2EE);   /* (-15634) & 0xffff */
  assert(st.pos_x == 0 && st.pos_z == 0);

  /* Target +z gives atan1 = 0 and blend 500; tick 1/rate 8 gives step 4,
   * which the native minimum raises to +30. */
  st.track_x = 0;
  st.track_z = 1000;
  st.yaw = 0;
  assert(fifa96_camera_behavior_steady(&st, &block, 1, &out) == FIFA96_OK);
  assert(st.yaw == 30);

  /* ratio 0 collapses the blend to atan1 = 0: the delta is inside the
   * +/-30 deadband and the yaw cell stays put. */
  st.ratio = 0;
  st.yaw = 0;
  assert(fifa96_camera_behavior_steady(&st, &block, 8, &out) == FIFA96_OK);
  assert(st.yaw == 0);

  assert(fifa96_camera_behavior_steady(NULL, &block, 0, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_behavior_steady(&st, NULL, 0, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_behavior_steady(&st, &block, 0, NULL) ==
         -FIFA96_ERR_INVALID);
}

static void test_behavior_sidetrack(void) {
  /* FUN_0004E3A8 (fresh 363-instruction disassembly). The full body runs the
   * shared pan accumulator, the projected yaw point (+-0x68 class bias), the
   * FUN_0004DF34 yaw step, the four snap arms, the block[0] horizon pair,
   * the class-2/non-class-2 pitch target split, the E05C pitch arm, the two
   * D698 slides, the rate-8 y/ratio eases and the history copy. */
  fifa96_camera_handler_state st;
  fifa96_camera_handler_block block;
  fifa96_camera_handler_out out;

  assert(fifa96_camera_behavior_sidetrack(NULL, &block, 0, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_behavior_sidetrack(&st, NULL, 0, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_behavior_sidetrack(&st, &block, 0, NULL) ==
         -FIFA96_ERR_INVALID);

  /* Pan: tick 120 makes the 20-unit factor 160, so pan 100 -> -60 -> the
   * +-const3c 20 clamp. The class-2 x slide then clamps at C[1] = 2400. */
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  memset(&out, 0, sizeof out);
  block.class_of = 2;
  block.const3c = 20;
  block.const40 = 80;
  block.const28 = 8;
  block.const20 = 8;
  block.const24 = 8;
  block.c0 = 600;
  block.c1 = 2400;
  block.c2 = 1500;
  block.c3 = 1500;
  block.sub10 = 0x437;
  block.pan = 200;
  st.pos_x = 1000;
  st.track_x = 0x100000;
  assert(fifa96_camera_behavior_sidetrack(&st, &block, 120, &out) ==
         FIFA96_OK);
  assert(block.pan == 20 && block.pan_step == -160);
  assert(block.cell18 == 0 && block.cell1c == 0);   /* handler 1 leaves them */
  assert(st.pos_x == 2400);
  assert(out.horizon_a == 0xE3B && out.horizon_b == 0xAD3);

  /* The non-class-2 x arm reverses the bounds: lo=-C[1], hi=-C[0]. */
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  block.class_of = 1;
  block.const3c = 20;
  block.const40 = 80;
  block.const28 = 8;
  block.const20 = 8;
  block.const24 = 8;
  block.c0 = 600;
  block.c1 = 2400;
  block.c2 = 1500;
  block.c3 = 1500;
  block.sub10 = 0x437;
  st.pos_x = -1000;
  st.track_x = 0x100000;
  assert(fifa96_camera_behavior_sidetrack(&st, &block, 120, &out) ==
         FIFA96_OK);
  assert(st.pos_x == -600);
  st.pos_x = -1000;
  st.track_x = -0x100000;
  assert(fifa96_camera_behavior_sidetrack(&st, &block, 120, &out) ==
         FIFA96_OK);
  assert(st.pos_x == -2400);

  /* The camera-relative band clamps the pan after the +-const3c clamp. */
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  block.class_of = 1;
  block.const3c = 100;
  block.const40 = 80;
  block.pan = 0x20;
  st.track_z = 0xB00;
  assert(fifa96_camera_behavior_sidetrack(&st, &block, 0, &out) == FIFA96_OK);
  assert(block.pan == 0x10 && block.pan_step == 0);

  /* Sub-record +0x10 is the x history offset: class 2 passes -sub10, every
   * other class +sub10. The z slide passes the mutable +0x18 cell. */
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  block.class_of = 2;
  block.const28 = 8;
  block.c0 = -5000;
  block.c1 = 5000;
  block.c2 = 5000;
  block.c3 = 5000;
  block.sub10 = 1000;
  block.cell18 = 500;
  assert(fifa96_camera_behavior_sidetrack(&st, &block, 120, &out) ==
         FIFA96_OK);
  assert(st.pos_x == 1000 && st.pos_z == -500);
  assert(block.const10 == 1000);   /* 0x4E740 stores sub+0x10 */
  memset(&st, 0, sizeof st);
  block.class_of = 1;
  assert(fifa96_camera_behavior_sidetrack(&st, &block, 120, &out) ==
         FIFA96_OK);
  assert(st.pos_x == -1000 && st.pos_z == -500);

  /* The two pitch-target formulas (fresh 0x4E642..0x4E700 / 0x4F042..0x4F12E).
   * hA=0xE3B, hB=0xAD3; the low-32-bit product and the +0x8000 SHRD are
   * visible in the negative and positive arms. */
  assert(fifa96_camera_pitch_target_a(-0x100, 0xE3B, 0xAD3) == -2899);
  assert(fifa96_camera_pitch_target_a(0, 0xE3B, 0xAD3) == -2771);
  assert(fifa96_camera_pitch_target_a(0x100, 0xE3B, 0xAD3) == -2634);
  assert(fifa96_camera_pitch_target_a(0x2000, 0xE3B, 0xAD3) == 1606);
  assert(fifa96_camera_pitch_target_b(-0x100, 0xE3B, 0xAD3) == 2634);
  assert(fifa96_camera_pitch_target_b(0, 0xE3B, 0xAD3) == 2771);
  assert(fifa96_camera_pitch_target_b(0x100, 0xE3B, 0xAD3) == 2899);
  assert(fifa96_camera_pitch_target_b(0x2000, 0xE3B, 0xAD3) == 6867);

  /* Snap arms (0x4E4A0..0x4E60E), isolated from the yaw step. */
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  block.class_of = 1;
  st.track_x = -0x400;
  st.pos_x = -0x720;
  st.ratio = 0x1000;
  fifa96_camera_sidetrack_snap(&st, &block);
  assert(st.yaw == 0xF000);      /* outside band: angle - ratio */

  memset(&st, 0, sizeof st);
  block.class_of = 1;
  st.track_x = -0x720;
  st.pos_x = -0x720;
  st.yaw = 0x8000;
  st.ratio = 0x1000;
  fifa96_camera_sidetrack_snap(&st, &block);
  assert(st.yaw == 0x9000);      /* inside band: angle + ratio */

  memset(&st, 0, sizeof st);
  block.class_of = 3;
  st.pos_x = 0x720;
  st.yaw = 0;
  st.ratio = 0x1000;
  fifa96_camera_sidetrack_snap(&st, &block);
  assert(st.yaw == 0x1000);      /* outside band: angle + ratio */

  memset(&st, 0, sizeof st);
  block.class_of = 3;
  st.pos_x = 0x720;
  st.yaw = 0x8000;
  st.ratio = 0x1000;
  fifa96_camera_sidetrack_snap(&st, &block);
  assert(st.yaw == 0x7000);      /* inside band: angle - ratio */

  /* The track_x gates: class 1 needs < -0x360, every other class < 0x360. */
  memset(&st, 0, sizeof st);
  block.class_of = 1;
  st.track_x = 0;
  st.yaw = 0x1234;
  fifa96_camera_sidetrack_snap(&st, &block);
  assert(st.yaw == 0x1234);
  block.class_of = 3;
  st.track_x = 0x360;
  fifa96_camera_sidetrack_snap(&st, &block);
  assert(st.yaw == 0x1234);
}

static void test_behavior_staged(void) {
  /* FUN_0004EC9C (fresh 425-instruction disassembly). The full body runs the
   * FUN_000A1A60 sin/cos scale into block+0x10 and the sub-record +0x18, the
   * shared pan accumulator, the block[3] class gate (stored back), the z/x
   * D698 slides, the DF34 yaw step and four snap arms, the block[0] horizon
   * pair, the class-3/4 pitch target split (clamped +-0x1620), the E05C pitch
   * arm, the rate-8 y/eases and the history copy. */
  fifa96_camera_handler_state st;
  fifa96_camera_handler_block block;
  fifa96_camera_handler_out out;

  assert(fifa96_camera_behavior_staged(NULL, &block, 0, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_behavior_staged(&st, NULL, 0, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_behavior_staged(&st, &block, 0, NULL) ==
         -FIFA96_ERR_INVALID);

  /* A1A60 scale: angle 0 gives sin=0, cos=0x10000, so sub14 1000 lands as
   * +0 (sub-record +0x18) and 1000 (block+0x10). field3=0 is class 4. */
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  memset(&out, 0, sizeof out);
  block.field3 = 0;
  block.sub14 = 1000;
  block.sub1c = 2000;
  block.cell18 = 0x500;
  assert(fifa96_camera_behavior_staged(&st, &block, 0, &out) == FIFA96_OK);
  assert(block.sub18 == 0 && block.const10 == 1000);
  assert(block.cell18 == 0x500 && block.cell1c == 0);
  assert(block.class_of == 4 && out.class_of == 4);
  assert(out.horizon_a == 0xE3B && out.horizon_b == 0xAD3);

  /* angle 0x4000 (90 degrees) swaps the pair; the pan cell then clamps up
   * to the freshly written sub-record +0x18. */
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  memset(&out, 0, sizeof out);
  block.field3 = 0x4000;
  block.sub14 = 1000;
  block.sub1c = 2000;
  block.cell18 = 0;
  assert(fifa96_camera_behavior_staged(&st, &block, 0, &out) == FIFA96_OK);
  assert(block.sub18 == 1000 && block.const10 == 0);
  assert(block.cell18 == 1000);
  assert(block.class_of == 3 && out.class_of == 3);

  /* The gate stores class 4 outside 0x4000..0xC000. */
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  block.field3 = 0xD000;
  assert(fifa96_camera_behavior_staged(&st, &block, 0, &out) == FIFA96_OK);
  assert(block.class_of == 4 && out.class_of == 4);

  /* x slide split at block[3] 0x8000. Below the split: lo=C[0], hi=C[1].
   * At/above: lo=-C[1], hi=-C[0]. Tick 120 makes the slide factor 1.0. */
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  block.field3 = 0x5000;
  block.c0 = 600;
  block.c1 = 2400;
  block.c2 = 900;
  block.c3 = 3750;
  block.const28 = 8;
  st.pos_x = 1000;
  st.track_x = 0x100000;
  assert(fifa96_camera_behavior_staged(&st, &block, 120, &out) == FIFA96_OK);
  assert(st.pos_x == 2400);
  memset(&st, 0, sizeof st);
  block.field3 = 0xA000;
  st.pos_x = -1000;
  st.track_x = 0x100000;
  assert(fifa96_camera_behavior_staged(&st, &block, 120, &out) == FIFA96_OK);
  assert(st.pos_x == -600);

  /* z slide: class 3 uses lo=-C[2], hi=C[3]; class 4 swaps them. */
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  memset(&out, 0, sizeof out);
  block.field3 = 0x5000;
  block.c0 = 600;
  block.c1 = 2400;
  block.c2 = 900;
  block.c3 = 3750;
  block.const28 = 8;
  st.track_z = 0x100000;
  assert(fifa96_camera_behavior_staged(&st, &block, 120, &out) == FIFA96_OK);
  assert(st.pos_z == 3750);
  memset(&st, 0, sizeof st);
  block.field3 = 0x1000;
  st.track_z = 0x100000;
  assert(fifa96_camera_behavior_staged(&st, &block, 120, &out) == FIFA96_OK);
  assert(st.pos_z == 900);

  /* Staged snap arms: the point is (0x720,0xB10) outside 0x2000..0xA000 and
   * (-0x720,-0xB10) inside; class 3 and class 4 swap the half-ratio sign. */
  memset(&st, 0, sizeof st);
  st.pos_x = 0x720;
  st.ratio = 0x1000;
  fifa96_camera_staged_snap(&st, 3);
  assert(st.yaw == 0xF800);      /* outside: angle - ratio/2 */
  memset(&st, 0, sizeof st);
  st.pos_x = -0x720;
  st.yaw = 0x8000;
  st.ratio = 0x1000;
  fifa96_camera_staged_snap(&st, 3);
  assert(st.yaw == 0x8800);      /* inside: angle + ratio/2 */
  memset(&st, 0, sizeof st);
  st.pos_x = 0x720;
  st.ratio = 0x1000;
  fifa96_camera_staged_snap(&st, 4);
  assert(st.yaw == 0x0800);      /* outside: angle + ratio/2 */
  memset(&st, 0, sizeof st);
  st.pos_x = -0x720;
  st.yaw = 0x8000;
  st.ratio = 0x1000;
  fifa96_camera_staged_snap(&st, 4);
  assert(st.yaw == 0x7800);      /* inside: angle - ratio/2 */
}

static void test_behavior_action(void) {
  /* FUN_0004DB38: the class-3 chase camera. The fixed point for a level
   * target is the 500-unit side offset: target (0,80,0), camera
   * (-353,434,0), yaw 0xC000, pitch 0x2000. The Y is 434 because the exact
   * 45-degree offset truncates to -354. */
  fifa96_camera_handler_state st;
  fifa96_camera_handler_out out;
  memset(&st, 0, sizeof st);
  memset(&out, 0, sizeof out);
  st.track_x = 0;
  st.track_y = 80;
  st.track_z = 0;
  st.pos_x = -353;
  st.pos_y = 433;
  st.pos_z = 0;
  st.yaw = 0xC000;
  st.pitch = 0x2000;
  assert(fifa96_camera_behavior_action(&st, 1, 500, 8, &out) == FIFA96_OK);
  assert(out.speed == 500);
  assert(st.pos_x == -353);
  assert(st.pos_y == 434);
  assert(st.pos_z == 0);
  assert(st.yaw == 0xC000 && st.pitch == 0x2000);
  assert(st.hist_x == 0 && st.hist_y == 80 && st.hist_z == 0);

  /* Q16 slew: a 0x4000 yaw error at tick 8 moves by the native 0x1111
   * (0x4DCCA/0x4DD2C); the pre-fix tick/divisor form left it at zero. The
   * pitch rate uses the 0x3C0000 divisor and moves 0x111 here too. */
  memset(&st, 0, sizeof st);
  st.track_x = -100;
  st.yaw = 0;
  st.pitch = 0;
  assert(fifa96_camera_behavior_action(&st, 1, 500, 8, &out) == FIFA96_OK);
  assert(st.yaw == 0x1111);
  assert(st.pitch == 0x111);

  /* The same rate on a negative 0x4000 yaw delta moves by -0x1111. */
  memset(&st, 0, sizeof st);
  st.track_x = 100;
  st.yaw = 0;
  st.pitch = 0x2000;
  assert(fifa96_camera_behavior_action(&st, 1, 500, 8, &out) == FIFA96_OK);
  assert(st.yaw == 0xEEEF);

  /* The native slew shift is 32-bit: a tick that wraps it to zero leaves
   * the stored angles unchanged. */
  memset(&st, 0, sizeof st);
  st.track_x = -100;
  st.yaw = 0;
  st.pitch = 0x2000;
  assert(fifa96_camera_behavior_action(&st, 1, 500, 0x3C0000, &out) ==
         FIFA96_OK);
  assert(st.yaw == 0 && st.pitch == 0x2000);

  /* Replay is 100 units farther; the exact fixed point is y=505. */
  memset(&st, 0, sizeof st);
  st.track_y = 80;
  st.pos_x = -353;
  st.pos_y = 433;
  st.yaw = 0xC000;
  st.pitch = 0x2000;
  assert(fifa96_camera_behavior_action(&st, 0, 500, 0, &out) == FIFA96_OK);
  assert(out.speed == 600);
  assert(st.pos_x == -424);
  assert(st.pos_y == 505);

  /* Replay Y floor: a target below the eye still clamps at 120. */
  memset(&st, 0, sizeof st);
  st.track_y = -0x1000;
  assert(fifa96_camera_behavior_action(&st, 0, 500, 0, &out) == FIFA96_OK);
  assert(st.pos_y == 120);

  /* Position clamp: a target far along +x puts the camera at x=+0x420. */
  memset(&st, 0, sizeof st);
  st.track_x = 0x2000;
  assert(fifa96_camera_behavior_action(&st, 1, 500, 0, &out) == FIFA96_OK);
  assert(st.pos_x == 0x420 && st.pos_z == 0);
  assert(st.pos_y == 100);

  /* Old-eye Y arm (0x4DBCA..0x4DBF3): the same target with the old X just
   * inside the clamp stores 345; on the boundary the off_y >> 3 arm stores
   * 306. */
  memset(&st, 0, sizeof st);
  st.track_y = -623;
  st.pos_x = 0x41F;
  st.pos_y = 433;
  st.yaw = 0xC000;
  st.pitch = 0x2000;
  assert(fifa96_camera_behavior_action(&st, 1, 500, 0, &out) == FIFA96_OK);
  assert(st.pos_y == 345);
  st.pos_x = 0x420;
  st.pos_y = 433;
  st.pos_z = 0;
  assert(fifa96_camera_behavior_action(&st, 1, 500, 0, &out) == FIFA96_OK);
  assert(st.pos_y == 306);

  /* Post-slew pitch snap: (0x8000,0xF618) -> 0xF618. A zero tick keeps the
   * 0x8001 stored pitch, and the snap runs after the slew. */
  memset(&st, 0, sizeof st);
  st.pitch = 0x8001;
  assert(fifa96_camera_behavior_action(&st, 1, 500, 0, &out) == FIFA96_OK);
  assert(st.pitch == 0xF618);

  assert(fifa96_camera_behavior_action(NULL, 1, 0, 0, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_behavior_action(&st, 1, 0, 0, NULL) ==
         -FIFA96_ERR_INVALID);
}

static void test_pose_axis_a(void) {
  /* FUN_0004B8D4 0x4B8D4..0x4B979: ac2 > 3 -> -1; the event-slot band
   * 0xB..0x15 takes +1 only when ac2 != 0; outside the band a negative
   * event means no actor and the track delta decides. The latch gets the
   * returned value. */
  int32_t latch = 0x7F;
  assert(fifa96_camera_pose_axis_a(0, 0, 4, &latch) == -1 && latch == -1);
  assert(fifa96_camera_pose_axis_a(0, 0xB, 0, &latch) == -1 && latch == -1);
  assert(fifa96_camera_pose_axis_a(0, 0xB, 1, &latch) == 1 && latch == 1);
  assert(fifa96_camera_pose_axis_a(0, 0x15, 3, &latch) == 1 && latch == 1);
  assert(fifa96_camera_pose_axis_a(0, 0x16, 0, &latch) == 1 && latch == 1);
  assert(fifa96_camera_pose_axis_a(0, 0xA, 0, &latch) == 1 && latch == 1);
  assert(fifa96_camera_pose_axis_a(0, 5, 1, &latch) == -1 && latch == -1);
  assert(fifa96_camera_pose_axis_a(-7, -1, 0, &latch) == -1 && latch == -1);
  assert(fifa96_camera_pose_axis_a(7, -1, 0, &latch) == 1 && latch == 1);
  assert(fifa96_camera_pose_axis_a(0, -1, 0, &latch) == 0 && latch == 0);
  assert(fifa96_camera_pose_axis_a(0, -1, 0, NULL) == 0);
}

static void test_pose_feed_default_handler_arm(void) {
  /* The default arm now dispatches the 0x108B80 handler when the caller
   * stages the camera-record subset + behavior block (the unported default
   * still returns 0 with a NULL handler state). */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_pose_args args;
  fifa96_camera_handler_state st;
  fifa96_camera_handler_block block;
  fifa96_camera_handler_out out;
  memset(&args, 0, sizeof args);
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  memset(&out, 0, sizeof out);
  args.view_mode = 0;
  args.selector = 0;            /* handler 0: steady */
  args.handler_state = &st;
  args.handler_block = &block;
  args.handler_out = &out;
  args.handler_tick = 0;
  st.pos_x = 100;
  st.pos_z = -50;
  st.yaw = 0;
  st.pitch = 0x2000;
  int32_t yaw = 7, pitch = 7, ratio = 7;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_x == 100 && cam.pos_z == -50);
  assert(yaw == 0 && pitch == 0x2000 && ratio == 0);
  assert(out.horizon_a == 0xE09 && out.horizon_b == 0xAA1);

  /* FU351-L2c-1: the pose feed stages FUN_0004B8D4's result in pan_axis_a
   * and the 0x1577C2 lead word in pan_axis_b. axis_side=1 + ac2=1 resolves
   * the controlled slot to 0xB, whose band returns +1. */
  st.track_z = 0;
  st.hist_z = 0;
  st.axis_present = 1;
  st.axis_roster = 0;
  st.axis_side = 0;   /* ac2 & 1 adds 0xB to side 0 while ac2 < 4 */
  st.axis_ac2 = 1;
  st.axis_lead_z = 0;
  block.const3c = 1000;
  block.const40 = 80;
  block.pan = 0;
  block.pan_step = 0;
  args.handler_tick = 8;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(st.pan_axis_a == 1 && st.pan_axis_b == 0);
  assert(st.axis_latch == 1);
  assert(block.pan_step == 17 && block.pan == 17);

  /* Opposite lead signs take the 32-step arm with the negative sign. */
  st.axis_lead_z = -1;
  block.pan = 0;
  block.pan_step = 0;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(st.pan_axis_a == 1 && st.pan_axis_b == -1);
  assert(block.pan_step == -17 && block.pan == -17);

  /* The 0x51047 zero-guard case: the handler is skipped, but the
   * FUN_0004B8D4 axis/latch still run before the call. */
  args.handler_skip = 1;
  block.pan = 100;
  block.pan_step = 123;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 0);
  assert(st.pan_axis_a == 1 && st.axis_latch == 1);
  assert(block.pan == 100 && block.pan_step == 123);
  args.handler_skip = 0;

  /* selector 3 = the action handler (unsigned live flag + speed). With the
   * camera at +0x500 and the target at the origin the class-3 geometry keeps
   * the 500-unit distance along +x (z=0). */
  memset(&st, 0, sizeof st);
  args.selector = 3;
  args.handler_live = 0;
  args.handler_speed = 500;
  args.handler_tick = 0;
  st.pos_x = 0x500;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(out.speed == 600 && cam.pos_x == 600 && cam.pos_z == 0);

  /* Without a staged handler state the default arm stays unported (0). */
  args.handler_state = NULL;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 0);
}

/* ---- FU-152 §2.9 (P4): FUN_00070074 classifier / FUN_000709D0 band /
 * FUN_00071DF4 rate event ---- */

static void test_classifier_bits(void) {
  /* FUN_00070074 first-hand: |z| < 0xB10 -> 8, < 0xB90 -> 0, else 4;
   * x < -0xD0 -> |1, x > 0xCF -> |2; the z-window edge (|z| within 0x30 of
   * 0xB10, threshold 0xA0 shrunken past it) sets 0x10 when y exceeds it. */
  uint16_t flags = 0xFFFF;
  const int32_t p0[3] = {0, 0, 0};
  assert(fifa96_camera_classify(p0, &flags) == 0 && flags == 8);
  const int32_t p1[3] = {0, 0xA1, 0};
  assert(fifa96_camera_classify(p1, &flags) == 0 && flags == (8 | 0x10));
  const int32_t p2[3] = {-0xD1, 0, 0};
  assert(fifa96_camera_classify(p2, &flags) == 0 && flags == (8 | 1));
  const int32_t p3[3] = {0xD0, 0, 0};
  assert(fifa96_camera_classify(p3, &flags) == 0 && flags == (8 | 2));
  /* |z| = 0xB90: base 4; edge 0x80 -> threshold 0x50; y 0x60 sets 0x10. */
  const int32_t p4[3] = {0, 0x60, 0xB90};
  assert(fifa96_camera_classify(p4, &flags) == 0 && flags == (4 | 0x10));
  const int32_t p5[3] = {0, 0x40, 0xB90};
  assert(fifa96_camera_classify(p5, &flags) == 0 && flags == 4);
  /* The zero mask is the "inside" result (the native returns flags == 0). */
  const int32_t p6[3] = {0, 0xA0, 0xB10};
  assert(fifa96_camera_classify(p6, &flags) == 1 && flags == 0);
  assert(fifa96_camera_classify(NULL, &flags) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_classify(p0, NULL) == -FIFA96_ERR_INVALID);
}

static void test_pan_band(void) {
  /* FUN_000709D0 band ladder (first-hand decompile): h < 0x29 ->
   * (class 1, 2h+0x14); h < 0x65 -> (class 2, (h+0x50)/2); h < 0x12D ->
   * (class 3, (h+100)/4); else (class 3, 100). */
  int32_t class_of = 0, param = 0;
  fifa96_camera_pan_band(1, &class_of, &param);
  assert(class_of == 1 && param == 2 + 0x14);
  fifa96_camera_pan_band(0x28, &class_of, &param);
  assert(class_of == 1 && param == 0x50 + 0x14);
  fifa96_camera_pan_band(0x29, &class_of, &param);
  assert(class_of == 2 && param == (0x29 + 0x50) / 2);
  fifa96_camera_pan_band(0x64, &class_of, &param);
  assert(class_of == 2 && param == (0x64 + 0x50) / 2);
  fifa96_camera_pan_band(0x65, &class_of, &param);
  assert(class_of == 3 && param == (0x65 + 100) / 4);
  fifa96_camera_pan_band(0x12C, &class_of, &param);
  assert(class_of == 3 && param == (0x12C + 100) / 4);
  fifa96_camera_pan_band(0x12D, &class_of, &param);
  assert(class_of == 3 && param == 100);
}

static void test_rate_event(void) {
  /* FUN_00071DF4 first arm: ball/event height > 0xF0 -> the sub-object rate
   * words * 15 clamped +-15; below the gate nothing is touched. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  assert(fifa96_camera_rate_event(&cam, 1, -2, 0xF1) == 1);
  assert(cam.vel_x == 15 && cam.vel_z == -15);
  assert(cam.speed != 0);
  cam.vel_x = 0;
  cam.vel_z = 0;
  assert(fifa96_camera_rate_event(&cam, 1, 2, 0xF0) == 0);
  assert(cam.vel_x == 0 && cam.vel_z == 0);
  assert(fifa96_camera_rate_event(NULL, 1, 2, 0xF1) == -FIFA96_ERR_INVALID);
}

/* M2 phase-11 T1 (FU-148 §14.5 / L14.7): the FUN_00070B94 camera-height
 * producer behind the native `word[0x157750]`. First-hand
 * `disassemble_bytes 0x73783..0x737b4` (FUN_000736AC: the timer accumulates
 * `word[0x1577FA] += delta`; when `word[0x1577F0]` (the event height) is
 * nonzero the Y is `FUN_00070B94(timer)`) and `0x70B94..0x70C05`:
 *   timer >= limit -> 0; height <= 0 -> 0;
 *   y = max(0, T16[0x94 - |F2 - timer|] - T16[0x94 - F2])
 * with T16 the 149-word table at flat 0x10F3C4 (first-hand `read_memory`) and
 * F2 = `word[0x1577F2]` = fifa96_camera_ramp(event height) (the ramp table's
 * `guard 3`/`0x94` arms included). The engine stores F2 as `event_peak` at
 * the event set and the arc as `pos_y` each update. */
static void test_camera_height_producer(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera idle = fresh_camera();
  assert(fifa96_camera_init(&cam, 0, 0, 0) == FIFA96_OK);
  assert(fifa96_camera_init(&idle, 0, 0, 0) == FIFA96_OK);
  /* idle: no event -> the native store is skipped (the Y never moves). */
  for (int i = 0; i < 40; i++)
    assert(fifa96_camera_update(&cam, 2, 0, 0) == FIFA96_OK);
  assert(cam.pos_y == 0);
  assert(cam.event_peak == 0);
  /* a live height 0x100 event: F2 = ramp(0x100) = 0x3B, F4 = 0x76, F6 = 0
   * (the NEG path: F6 = F2 - ramp(h - ty) with ty = pos_y = 0). */
  assert(fifa96_camera_event_set(&cam, 0, 0x200, 0x100, 0) == FIFA96_OK);
  assert(cam.event_peak == 59);
  assert(cam.timer == 0 && cam.timer_limit == 118);
  /* The first-hand arc at delta 2: t=2 -> 17, t=4 -> 33 ... t=12 -> 93. */
  static const int16_t arc[6] = {17, 33, 49, 64, 79, 93};
  for (int i = 0; i < 6; i++) {
    assert(fifa96_camera_update(&cam, 2, 0, 0) == FIFA96_OK);
    assert(cam.pos_y == arc[i]);
  }
  for (int i = 6; i < 29; i++)
    assert(fifa96_camera_update(&cam, 2, 0, 0) == FIFA96_OK);
  assert(cam.pos_y == 253);            /* t = 58, the arc's crest */
  for (int i = 29; i < 58; i++)
    assert(fifa96_camera_update(&cam, 2, 0, 0) == FIFA96_OK);
  assert(cam.pos_y == 17);             /* t = 116, the descending arc */
  assert(fifa96_camera_update(&cam, 2, 0, 0) == FIFA96_OK);
  assert(cam.pos_y == 0);              /* t = 118 >= limit: the guard arm */
  /* The landing arm ran the pan step and rewrote the event cells; restore
   * the arc's own peak/height for the direct unit read below. */
  cam.event_peak = 59;
  cam.event_param = 0x100;
  cam.timer_limit = 118;
  /* the arc's direct read (the unit contract). */
  cam.timer = 58;
  cam.pos_y = 0;
  assert(fifa96_camera_height(&cam) == 253);
  cam.timer = 118;
  assert(fifa96_camera_height(&cam) == 0);
  cam.timer = 58;
  cam.event_param = 0;
  assert(fifa96_camera_height(&cam) == 0);
  assert(fifa96_camera_height(NULL) == 0);
  /* the height-0 event discriminator: the idle arm stores F2 = 6 but
   * event_param = 0 gates the store, so the Y stays 0. */
  assert(fifa96_camera_event_set(&idle, 0, 0, 0, 0) == FIFA96_OK);
  assert(idle.event_peak == 6);
  for (int i = 0; i < 40; i++)
    assert(fifa96_camera_update(&idle, 2, 0, 0) == FIFA96_OK);
  assert(idle.pos_y == 0);
}

/* ---- FU-152 §2.3 (P4): the FUN_0004D134 replay camera selector ---- */

static void test_replay_select(void) {
  int32_t kind = -1, sub = -1;
  /* 0 -> the [0x107968 + [0x107DD0]*0x70] record, pose sub 0. */
  assert(fifa96_camera_replay_select(0, &kind, &sub) == 1);
  assert(kind == FIFA96_CAMERA_REPLAY_POSE && sub == 0);
  /* 1/2/3/6 -> 0x107CE8 with sub 5/6/7/8. */
  assert(fifa96_camera_replay_select(1, &kind, &sub) == 1);
  assert(kind == FIFA96_CAMERA_REPLAY_VIEW && sub == 5);
  assert(fifa96_camera_replay_select(2, &kind, &sub) == 1);
  assert(kind == FIFA96_CAMERA_REPLAY_VIEW && sub == 6);
  assert(fifa96_camera_replay_select(3, &kind, &sub) == 1);
  assert(kind == FIFA96_CAMERA_REPLAY_VIEW && sub == 7);
  assert(fifa96_camera_replay_select(6, &kind, &sub) == 1);
  assert(kind == FIFA96_CAMERA_REPLAY_VIEW && sub == 8);
  /* 4 -> 0x107B98 (the action handler record), 5 -> 0x107B28. */
  assert(fifa96_camera_replay_select(4, &kind, &sub) == 1);
  assert(kind == FIFA96_CAMERA_REPLAY_ACTION && sub == 0);
  assert(fifa96_camera_replay_select(5, &kind, &sub) == 1);
  assert(kind == FIFA96_CAMERA_REPLAY_BALL && sub == 0);
  /* The native default arm only records the index. */
  assert(fifa96_camera_replay_select(7, &kind, &sub) == 0);
  assert(fifa96_camera_replay_select(0, NULL, &sub) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_replay_select(0, &kind, NULL) == -FIFA96_ERR_INVALID);
}

static void test_camera_replay_blend(void) {
  /* FUN_0004F8C8 / FUN_0004F830 with the FUN_0004CF7C 0xA/0x1E times:
   * in_time = 100, out_time = 300, total 400 ticks. */
  fifa96_camera_pose6 pose = {100, 200, 300, 0x1000, 0x2000, 4000};
  const fifa96_camera_pose6 start = {0, 0, 0, 0, 0, 0};
  const fifa96_camera_pose6 target = {600, 0, 0, 0x2000, 0, 6000};
  fifa96_camera_replay_blend state;
  fifa96_camera_pose6 snapshot;
  memset(&state, 0, sizeof state);
  memset(&snapshot, 0, sizeof snapshot);

  /* FUN_0004CF60 snapshots the active camera record for the 0x14E460
   * target block. */
  fifa96_camera_replay_snapshot(&snapshot, &target);
  assert(snapshot.x == target.x && snapshot.y == target.y);
  assert(snapshot.z == target.z && snapshot.yaw == target.yaw);
  assert(snapshot.pitch == target.pitch && snapshot.ratio == target.ratio);
  fifa96_camera_replay_snapshot(NULL, &target);
  fifa96_camera_replay_snapshot(&snapshot, NULL);

  assert(fifa96_camera_replay_ease(600, 100, 300, 0) == 0);
  assert(fifa96_camera_replay_ease(600, 100, 300, 50) == 25);
  assert(fifa96_camera_replay_ease(600, 100, 300, 100) == 100);
  assert(fifa96_camera_replay_ease(600, 100, 300, 200) == 300);
  assert(fifa96_camera_replay_ease(600, 100, 300, 300) == 500);
  assert(fifa96_camera_replay_ease(600, 100, 300, 350) == 575);
  assert(fifa96_camera_replay_ease(600, 100, 300, 400) == 600);
  assert(fifa96_camera_replay_ease(600, 100, 300, 500) == 500);
  assert(fifa96_camera_replay_ease(600, 0, 0, 0) == 0);

  /* FUN_0004CF7C's head gate: screen 7 skips the blend. */
  memset(&state, 0, sizeof state);
  assert(fifa96_camera_replay_begin_gated(&pose, &start, &target, &state, 7) ==
         0);
  assert(state.active == 0);
  assert(fifa96_camera_replay_begin_gated(&pose, &start, &target, &state, 0) ==
         FIFA96_OK);
  assert(state.active == 1);

  assert(fifa96_camera_replay_begin(&pose, &start, &target, &state) ==
         FIFA96_OK);
  assert(pose.x == 0 && pose.y == 0 && pose.z == 0);
  assert(pose.yaw == 0 && pose.pitch == 0 && pose.ratio == 0);
  assert(state.start.x == 0 && state.dx == 600 && state.dyaw == 0x2000);
  assert(state.dpitch == 0 && state.dratio == 6000);
  assert(state.in_time == 100 && state.out_time == 300 && state.elapsed == 0);
  assert(state.active == 1);

  assert(fifa96_camera_replay_step(&pose, 100, 0, 0, &state) == 0);
  assert(state.elapsed == 100);
  assert(pose.x == 100 && pose.y == 0 && pose.z == 0);
  assert(pose.yaw == 0x0555 && pose.pitch == 0 && pose.ratio == 1000);

  assert(fifa96_camera_replay_step(&pose, 100, 0, 0, &state) == 0);
  assert(state.elapsed == 200);
  assert(pose.x == 300 && pose.yaw == 0x1000 && pose.pitch == 0);
  assert(pose.ratio == 3000);

  assert(fifa96_camera_replay_step(&pose, 100, 0, 0, &state) == 0);
  assert(state.elapsed == 300);
  assert(pose.x == 500 && pose.yaw == 0x1AAA && pose.pitch == 0);
  assert(pose.ratio == 5000);

  assert(fifa96_camera_replay_step(&pose, 100, 0, 0, &state) == 0);
  assert(state.elapsed == 400);
  assert(pose.x == 600 && pose.y == 0 && pose.z == 0);
  assert(pose.yaw == 0x2000 && pose.pitch == 0 && pose.ratio == 6000);
  assert(fifa96_camera_replay_step(&pose, 1, 0, 0, &state) == 1);
  assert(state.active == 0);

  /* A NULL start snapshots the current pose before computing the deltas. */
  memset(&pose, 0, sizeof pose);
  pose.x = 10;
  pose.y = 20;
  pose.z = 30;
  pose.yaw = 0x100;
  pose.pitch = 0x200;
  pose.ratio = 300;
  memset(&state, 0, sizeof state);
  assert(fifa96_camera_replay_begin(&pose, NULL, &target, &state) ==
         FIFA96_OK);
  assert(state.start.x == 10 && state.start.yaw == 0x100);
  assert(state.dx == 590 && state.dyaw == 0x1F00);
  assert(state.dpitch == -0x200 && state.dratio == 5700);

  /* FUN_00045069: the high-nibble input latch. */
  {
    fifa96_camera_replay_input input;
    memset(&input, 0, sizeof input);
    assert(fifa96_camera_replay_cancel(&input) == 0 && input.latch == 0);
    input.input = 0x0F;
    assert(fifa96_camera_replay_cancel(&input) == 0 && input.latch == 0);
    input.input = 0x10;
    assert(fifa96_camera_replay_cancel(&input) == 0x10 && input.latch == 5);
    input.input = 0xF0;
    assert(fifa96_camera_replay_cancel(&input) == 0 && input.latch == 5);
    input.latch = 0;
    assert(fifa96_camera_replay_cancel(&input) == 0xF0 && input.latch == 5);
    assert(fifa96_camera_replay_cancel(NULL) == 0);
  }

  /* The 0x4F8C8 cancel gate: only runs while the latch is clear and the
   * view mode is not 0x14. */
  state.active = 0;
  state.elapsed = 0;
  assert(fifa96_camera_replay_step(&pose, 1, 1, 0, &state) == 1);
  assert(state.in_time == 0 && state.out_time == 0);
  assert(state.elapsed == 0 && state.active == 0);
  assert(fifa96_camera_replay_begin(&pose, &start, &target, &state) ==
         FIFA96_OK);
  state.active = 0;
  state.elapsed = 0;
  assert(fifa96_camera_replay_step(&pose, 1, 1, 0x14, &state) == 0);
  assert(state.elapsed == 1 && state.in_time == 100);

  assert(fifa96_camera_replay_begin(NULL, &start, &target, &state) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_replay_begin(&pose, &start, NULL, &state) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_replay_step(NULL, 0, 0, 0, &state) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_replay_step(&pose, 0, 0, 0, NULL) ==
         -FIFA96_ERR_INVALID);
}

static void test_camera_action_table(void) {
  /* FUN_000505D0 state 0xB tail: FUN_0004BAD8 -> FUN_0004BE44 slot, the
   * 0x155AB0 12-byte entry copied to camera+0x34/38/3C, then FUN_00036ACC. */
  int32_t table[FIFA96_CAMERA_ACTION_SLOTS][3];
  fifa96_camera_handler_state st;
  fifa96_camera_pose6 pose;
  fifa96_camera_replay_blend blend;
  memset(table, 0, sizeof table);
  table[0xB][0] = 100;
  table[0xB][1] = 200;
  table[0xB][2] = 0;
  memset(&st, 0, sizeof st);
  st.ratio = 0x1000;
  st.yaw = 0xD000;
  assert(fifa96_camera_action_target(&st, table, 1, 0, 0, 1) == 1);
  assert(st.track_x == 100 && st.track_y == 200 && st.track_z == 0);
  /* The point (100,0) is angle 0xC000; the 0xC00 step leaves a -0x400
   * remainder, so yaw moves from 0xD000 to 0xCC00. */
  assert(st.yaw == 0xCC00);

  /* The positive-delta arm: point (0,-100) is angle 0x8000, yaw 0x7000. */
  st.track_x = 0;
  st.track_z = -100;
  st.yaw = 0x7000;
  fifa96_camera_yaw_approach(&st);
  assert(st.yaw == 0x7400);

  /* No event -> no slot, no target change. */
  st.track_x = 7;
  st.track_z = 9;
  assert(fifa96_camera_action_target(&st, table, 0, 0, 0, 0) == 0);
  assert(st.track_x == 7 && st.track_z == 9);
  assert(fifa96_camera_action_target(NULL, table, 1, 0, 0, 1) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_action_target(&st, NULL, 1, 0, 0, 1) ==
         -FIFA96_ERR_INVALID);

  /* The composed state: a finished blend applies the target immediately. */
  memset(&pose, 0, sizeof pose);
  memset(&blend, 0, sizeof blend);
  memset(&st, 0, sizeof st);
  st.ratio = 0x1000;
  st.yaw = 0xD000;
  assert(fifa96_camera_action_step(&st, table, 1, 0, 0, 1, &pose, 0, 0, 0,
                                   &blend) == 1);
  assert(st.track_x == 100 && st.track_z == 0);
  assert(st.yaw == 0xCC00);

  /* An unfinished blend returns 0 without touching the target. */
  memset(&blend, 0, sizeof blend);
  blend.in_time = 100;
  blend.out_time = 300;
  blend.active = 1;
  st.track_x = 7;
  st.track_z = 9;
  assert(fifa96_camera_action_step(&st, table, 1, 0, 0, 1, &pose, 100, 0, 0,
                                   &blend) == 0);
  assert(st.track_x == 7 && st.track_z == 9);
  assert(fifa96_camera_action_step(NULL, table, 1, 0, 0, 1, &pose, 0, 0, 0,
                                   &blend) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_action_step(&st, table, 1, 0, 0, 1, NULL, 0, 0, 0,
                                   &blend) == -FIFA96_ERR_INVALID);
}

static void test_camera_pan_target(void) {
  /* FUN_00036208's input mapping, mode clamps and the 0x105FC8 offsets. */
  fifa96_camera_pan_target pan;
  memset(&pan, 0, sizeof pan);
  assert(fifa96_camera_pan_target_step(&pan, 0, 1, 1, 10, 0, 0, 0, 0) == 1);
  assert(pan.x == 10 && pan.z == 0 && pan.dx == 10 && pan.dz == 0);
  memset(&pan, 0, sizeof pan);
  assert(fifa96_camera_pan_target_step(&pan, 0, 1, 2, 10, 0, 0, 0, 0) == 1);
  assert(pan.x == -10 && pan.dx == -10);
  memset(&pan, 0, sizeof pan);
  assert(fifa96_camera_pan_target_step(&pan, 0, 1, 4, 10, 0, 0, 0, 0) == 1);
  assert(pan.z == -10 && pan.dz == -10);
  memset(&pan, 0, sizeof pan);
  assert(fifa96_camera_pan_target_step(&pan, 0, 1, 8, 10, 0, 0, 0, 0) == 1);
  assert(pan.z == 10 && pan.dz == 10);
  memset(&pan, 0, sizeof pan);
  assert(fifa96_camera_pan_target_step(&pan, 0, 1, 0x10, 10, 0, 0, 0, 0) == 1);
  assert(pan.x == 0 && pan.z == 0 && pan.dx == 0 && pan.dz == 0);
  memset(&pan, 0, sizeof pan);
  assert(fifa96_camera_pan_target_step(&pan, -1, 1, 1, 10, 0, 0, 0, 0) == 0);
  assert(fifa96_camera_pan_target_step(&pan, 0, 0, 1, 10, 0, 0, 0, 0) == 0);
  assert(fifa96_camera_pan_target_step(NULL, 0, 1, 1, 10, 0, 0, 0, 0) ==
         -FIFA96_ERR_INVALID);

  /* Mode 0 clamps the 0x12C0 z window to +-0x9C0 and x to +-0x660. */
  memset(&pan, 0, sizeof pan);
  pan.x = 0x1000;
  pan.z = 0x3000;
  assert(fifa96_camera_pan_target_step(&pan, 0, 1, 0, 0, 0, 0, 0, 0) == 1);
  assert(pan.x == 0x660 && pan.z == 0x9C0);
  assert(pan.dx == 0 && pan.dz == 0);

  /* Phase 0x15 option 3 -> mode 5 (x +-0x720, z 0..0xB10). */
  memset(&pan, 0, sizeof pan);
  assert(fifa96_camera_pan_target_step(&pan, 0, 1, 1, 10, 0x15, 0, 3, 0) == 1);
  assert(pan.x == 0x720 && pan.z == 0 && pan.dx == 0 && pan.dz == 0);
  /* Option 0 -> mode 6. */
  memset(&pan, 0, sizeof pan);
  assert(fifa96_camera_pan_target_step(&pan, 0, 1, 1, 10, 0x15, 0, 0, 0) == 1);
  assert(pan.x == 0x720 && pan.z == 0xB10);
  /* Option 1 -> mode 8 (x +-0x630, z 0..0x9F0, the 0x7AF rail). */
  memset(&pan, 0, sizeof pan);
  pan.z = 0x800;
  assert(fifa96_camera_pan_target_step(&pan, 0, 1, 0, 0, 0x15, 0, 1, 0) == 1);
  assert(pan.x == 0 && pan.z == 0x7AF && pan.dx == 0 && pan.dz == 0);
}

static void test_camera_replay_choice(void) {
  fifa96_camera_replay_target t;
  fifa96_camera_replay_activity a;
  fifa96_camera_replay_choice c;
  memset(&t, 0, sizeof t);
  memset(&a, 0, sizeof a);
  for (int side = 0; side < 2; side++) {
    for (int slot = 0; slot < 16; slot++) {
      t.order_id[side][slot] = side * 16 + slot;
      a.id[side][slot] = side * 16 + slot;
    }
  }

  /* Tied score with no events: the zero table picks side 0 / slot 0. */
  assert(fifa96_camera_replay_choice_select(&t, &a, &c) == FIFA96_OK);
  assert(c.side == 0 && c.slot == 0);
  assert(c.target_index == 0 && c.source_index == 0 && c.copy_slot == -1);

  /* A 1-0 goal is not an equaliser: the ring starts at 0-0, so FUN_0004BBFC
   * leaves the event index -1 and does not bump the scorer. With a zero
   * activity table the choice is side 0 / slot 0. */
  memset(&a, 0, sizeof a);
  for (int side = 0; side < 2; side++)
    for (int slot = 0; slot < 16; slot++)
      a.id[side][slot] = side * 16 + slot;
  t.score[0] = 1;
  t.score[1] = 0;
  t.event_count = 1;
  t.event_side[0] = 0;
  t.event_id[0] = 5;
  assert(fifa96_camera_replay_choice_select(&t, &a, &c) == FIFA96_OK);
  assert(c.side == 0 && c.slot == 0);
  assert(c.target_index == 0 && c.source_index == 0 && c.copy_slot == -1);
  assert(a.count[0][5] == 0);

  /* The same 1-0 score with a pre-existing activity count picks that entry
   * without the goal bump. */
  a.count[0][5] = 100;
  assert(fifa96_camera_replay_choice_select(&t, &a, &c) == FIFA96_OK);
  assert(c.side == 0 && c.slot == 5 && c.order_value == 5);
  assert(c.target_index == 5 && c.source_index == 1 && c.copy_slot == 1);
  assert(a.count[0][5] == 100);

  /* The ring 0,1,0 makes the third event the first one after the 1-1 level:
   * FUN_0004BBFC's selected index is 2, so side 0 / id 7 is bumped. */
  memset(&a, 0, sizeof a);
  for (int side = 0; side < 2; side++)
    for (int slot = 0; slot < 16; slot++)
      a.id[side][slot] = side * 16 + slot;
  t.score[0] = 2;
  t.score[1] = 1;
  t.event_count = 3;
  t.event_side[0] = 0;
  t.event_id[0] = 5;
  t.event_side[1] = 1;
  t.event_id[1] = 19;
  t.event_side[2] = 0;
  t.event_id[2] = 7;
  assert(fifa96_camera_replay_choice_select(&t, &a, &c) == FIFA96_OK);
  assert(c.side == 0 && c.slot == 7 && c.order_value == 7);
  assert(c.target_index == 7 && c.source_index == 1 && c.copy_slot == 1);
  assert(a.count[0][7] == 0x32);

  /* The mirrored ring 0,1,1 with a 1-2 score selects side 1 / id 21. */
  memset(&a, 0, sizeof a);
  for (int side = 0; side < 2; side++)
    for (int slot = 0; slot < 16; slot++)
      a.id[side][slot] = side * 16 + slot;
  t.score[0] = 1;
  t.score[1] = 2;
  t.event_count = 3;
  t.event_side[2] = 1;
  t.event_id[2] = 21;
  assert(fifa96_camera_replay_choice_select(&t, &a, &c) == FIFA96_OK);
  assert(c.side == 1 && c.slot == 5 && c.order_value == 21);
  assert(c.target_index == 16 && c.source_index == 12 && c.copy_slot == 1);
  assert(a.count[1][5] == 0x32);

  /* Side 1 / slot 0 is the 0x10 special: source 0xB, no order copy. */
  memset(&a, 0, sizeof a);
  for (int side = 0; side < 2; side++)
    for (int slot = 0; slot < 16; slot++)
      a.id[side][slot] = side * 16 + slot;
  t.score[0] = 1;
  t.score[1] = 2;
  t.event_count = 3;
  t.event_side[2] = 1;
  t.event_id[2] = 16;
  assert(fifa96_camera_replay_choice_select(&t, &a, &c) == FIFA96_OK);
  assert(c.side == 1 && c.slot == 0);
  assert(c.target_index == 11 && c.source_index == 0xB && c.copy_slot == -1);

  /* A pre-existing side-1 / slot-2 count wins when the score is tied. */
  memset(&a, 0, sizeof a);
  for (int side = 0; side < 2; side++)
    for (int slot = 0; slot < 16; slot++)
      a.id[side][slot] = side * 16 + slot;
  a.count[1][2] = 100;
  t.score[0] = 0;
  t.score[1] = 0;
  t.event_count = 0;
  assert(fifa96_camera_replay_choice_select(&t, &a, &c) == FIFA96_OK);
  assert(c.side == 1 && c.slot == 2);
  assert(c.target_index == 13 && c.source_index == 12 && c.copy_slot == 1);

  assert(fifa96_camera_replay_choice_select(NULL, &a, &c) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_replay_choice_select(&t, NULL, &c) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_replay_choice_select(&t, &a, NULL) ==
         -FIFA96_ERR_INVALID);
}

static void test_camera_target_select(void) {
  /* FUN_000365B0's [0x105FC4] switch and the early replay fallback. */
  const int32_t focus[3] = {1, 2, 3};
  const int32_t pan[3] = {4, 5, 6};
  const int32_t record[3] = {7, 8, 9};
  const int32_t fallback[3] = {0, 0, 0};
  int32_t table[FIFA96_CAMERA_ACTION_SLOTS][3];
  memset(table, 0, sizeof table);
  table[21][0] = 21;
  table[22][0] = 22;

  /* force_focus = 0, replay_active = 0; the active view record class is -1. */
  assert(fifa96_camera_target_select(0, 0, 0, -1, focus, pan, record, fallback,
                                     table, 0) == focus);
  assert(fifa96_camera_target_select(1, 0, 0, -1, focus, pan, record, fallback,
                                     table, 0) == pan);
  assert(fifa96_camera_target_select(2, 0, 0, -1, focus, pan, record, fallback,
                                     table, 0) == record);
  assert(fifa96_camera_target_select(3, 0, 0, -1, focus, pan, record, fallback,
                                     table, 0) == table[22]);
  assert(fifa96_camera_target_select(4, 0, 0, -1, focus, pan, record, fallback,
                                     table, 21) == table[21]);
  assert(fifa96_camera_target_select(4, 0, 0, -1, focus, pan, record, fallback,
                                     table, 22) == focus);
  assert(fifa96_camera_target_select(4, 0, 0, -1, focus, pan, record, fallback,
                                     table, -1) == focus);
  assert(fifa96_camera_target_select(9, 0, 0, -1, focus, pan, record, fallback,
                                     table, 0) == focus);
  /* FUN_00063FD0 forces focus; FUN_00037AE4 + class -1 takes the fallback. */
  assert(fifa96_camera_target_select(1, 1, 0, -1, focus, pan, record, fallback,
                                     table, 0) == focus);
  assert(fifa96_camera_target_select(0, 0, 1, -1, focus, pan, record, fallback,
                                     table, 0) == fallback);
  assert(fifa96_camera_target_select(0, 0, 1, 0, focus, pan, record, fallback,
                                     table, 0) == focus);
  assert(fifa96_camera_target_select(3, 0, 0, -1, focus, pan, record, fallback,
                                     NULL, 0) == NULL);
  assert(fifa96_camera_target_select(4, 0, 0, -1, focus, pan, record, fallback,
                                     NULL, 0) == NULL);
}

static void test_behavior_table(void) {
  /* First-hand 0x10896C (504 B) rows {target0, target1, class, field3(+0xC),
   * +0x10, +0x18, +0x24, +0x28, const30, const34, sub-record pointer(+0x50)}:
   * classes {3,1,3,3,1,3}; the +0xC class-gate cells are
   * {0,0,0xA7F8,0,0,0xA21C} so blocks 2/5 take FUN_0004EC9C's class-3 arm
   * (0x4000..0xC000) and 0/1/3/4 the class-4 arm; every +0x34 is zero
   * (handler 1's yaw target is the bare track z). The three distinct
   * sub-records are 0x1085AC/0x10862C/0x1085EC. */
  assert(FIFA96_CAMERA_BEHAVIOR_BLOCKS == 6);
  const int32_t classes[6] = {3, 1, 3, 3, 1, 3};
  const int32_t gate[6] = {0, 0, 0xA7F8, 0, 0, 0xA21C};
  const int32_t c10[6] = {0xC8, 0x4B0, 0, 0xC8, 0x4B0, 0};
  const int32_t c18[6] = {0x578, 0, 0x578, 0x578, 0, 0x578};
  const int32_t c24[6] = {8, 8, 8, 8, 8, 8};
  const int32_t c28[6] = {8, 0x20, 8, 8, 0x20, 8};
  const int32_t c1c[6] = {32, 0, 20, 32, 0, 20};
  const int32_t c3c[6] = {600, 600, 550, 600, 600, 550};
  const int32_t c40[6] = {80, 80, 60, 80, 80, 60};
  const int32_t s10[6] = {0, 0x437, 0, 0, 0x437, 0};
  const int32_t s14[6] = {0, 0x720, 0x5DC, 0, 0x720, 0x5DC};
  const int32_t s18[6] = {1330, 0, 1330, 1330, 0, 1330};
  const int32_t s1c[6] = {1750, 0, 2000, 1750, 0, 2000};
  const int32_t v1c[6][4] = {
      {450, 450, 1600, 3430}, {-200, 2100, 1600, 1600},
      {200, 1860, 1600, 3400}, {450, 450, 1600, 3430},
      {-200, 2100, 1600, 1600}, {200, 1860, 1600, 3400},
  };
  const int32_t v1s10[6] = {0, 700, 0, 0, 700, 0};
  const int32_t v1s14[6] = {0, 1150, 1200, 0, 1150, 1200};
  const int32_t v1s18[6] = {700, 0, 0, 700, 0, 0};
  const int32_t v1s1c[6] = {1150, 0, 0, 1150, 0, 0};
  const int32_t c_rec[6][4] = {
      {150, 150, 1000, 3800}, {600, 2400, 1500, 1500},
      {900, 1900, 900, 3750}, {150, 150, 1000, 3800},
      {600, 2400, 1500, 1500}, {900, 1900, 900, 3750},
  };
  for (int i = 0; i < 6; i++) {
    assert(fifa96_camera_behavior_blocks[i].class_of == classes[i]);
    assert(fifa96_camera_behavior_blocks[i].field3 == gate[i]);
    assert(fifa96_camera_behavior_blocks[i].const34 == 0);
    assert(fifa96_camera_behavior_blocks[i].target0 == 0x340);
    assert(fifa96_camera_behavior_blocks[i].const10 == c10[i]);
    assert(fifa96_camera_behavior_blocks[i].const18 == c18[i]);
    assert(fifa96_camera_behavior_blocks[i].const1c == c1c[i]);
    assert(fifa96_camera_behavior_blocks[i].const20 == 8);
    assert(fifa96_camera_behavior_blocks[i].const24 == c24[i]);
    assert(fifa96_camera_behavior_blocks[i].const28 == c28[i]);
    assert(fifa96_camera_behavior_blocks[i].const3c == c3c[i]);
    assert(fifa96_camera_behavior_blocks[i].const40 == c40[i]);
    assert(fifa96_camera_behavior_blocks[i].sub10 == s10[i]);
    assert(fifa96_camera_behavior_blocks[i].sub14 == s14[i]);
    assert(fifa96_camera_behavior_blocks[i].sub18 == s18[i]);
    assert(fifa96_camera_behavior_blocks[i].sub1c == s1c[i]);
    assert(fifa96_camera_behavior_blocks[i].v1_c0 == v1c[i][0]);
    assert(fifa96_camera_behavior_blocks[i].v1_c1 == v1c[i][1]);
    assert(fifa96_camera_behavior_blocks[i].v1_c2 == v1c[i][2]);
    assert(fifa96_camera_behavior_blocks[i].v1_c3 == v1c[i][3]);
    assert(fifa96_camera_behavior_blocks[i].v1_sub10 == v1s10[i]);
    assert(fifa96_camera_behavior_blocks[i].v1_sub14 == v1s14[i]);
    assert(fifa96_camera_behavior_blocks[i].v1_sub18 == v1s18[i]);
    assert(fifa96_camera_behavior_blocks[i].v1_sub1c == v1s1c[i]);
    assert(fifa96_camera_behavior_blocks[i].cell18 == c18[i]);
    assert(fifa96_camera_behavior_blocks[i].cell1c == c1c[i]);
    assert(fifa96_camera_behavior_blocks[i].pan == 0);
    assert(fifa96_camera_behavior_blocks[i].pan_step == 0);
    assert(fifa96_camera_behavior_blocks[i].c0 == c_rec[i][0]);
    assert(fifa96_camera_behavior_blocks[i].c1 == c_rec[i][1]);
    assert(fifa96_camera_behavior_blocks[i].c2 == c_rec[i][2]);
    assert(fifa96_camera_behavior_blocks[i].c3 == c_rec[i][3]);
  }
  assert(fifa96_camera_behavior_blocks[0].target1 == 0xFA0);
  assert(fifa96_camera_behavior_blocks[0].const30 == 0xEA6);
  assert(fifa96_camera_behavior_blocks[1].target1 == 0x1200);
  assert(fifa96_camera_behavior_blocks[1].const30 == 0x578);
  assert(fifa96_camera_behavior_blocks[2].target1 == 0xD48);
  assert(fifa96_camera_behavior_blocks[2].const30 == 0x1130);
  assert(fifa96_camera_behavior_blocks[3].target1 == 0xFA0);
  assert(fifa96_camera_behavior_blocks[4].target1 == 0x1200);
  assert(fifa96_camera_behavior_blocks[5].target1 == 0xD48);

  /* The image block 5 through the staged handler: gate 0xA21C is inside
   * 0x4000..0xC000 -> class 3; the A1A60 scale comes from sub14 0x5DC. */
  fifa96_camera_handler_block block;
  fifa96_camera_handler_state st;
  fifa96_camera_handler_out out;
  int32_t sin16, cos16, scaled;
  memset(&block, 0, sizeof block);
  block.target0 = fifa96_camera_behavior_blocks[5].target0;
  block.target1 = fifa96_camera_behavior_blocks[5].target1;
  block.class_of = fifa96_camera_behavior_blocks[5].class_of;
  block.field3 = fifa96_camera_behavior_blocks[5].field3;
  block.const10 = fifa96_camera_behavior_blocks[5].const10;
  block.const18 = fifa96_camera_behavior_blocks[5].const18;
  block.const1c = fifa96_camera_behavior_blocks[5].const1c;
  block.const20 = fifa96_camera_behavior_blocks[5].const20;
  block.const24 = fifa96_camera_behavior_blocks[5].const24;
  block.const28 = fifa96_camera_behavior_blocks[5].const28;
  block.const30 = fifa96_camera_behavior_blocks[5].const30;
  block.const34 = fifa96_camera_behavior_blocks[5].const34;
  block.const38 = fifa96_camera_behavior_blocks[5].const38;
  block.const3c = fifa96_camera_behavior_blocks[5].const3c;
  block.const40 = fifa96_camera_behavior_blocks[5].const40;
  block.c0 = fifa96_camera_behavior_blocks[5].c0;
  block.c1 = fifa96_camera_behavior_blocks[5].c1;
  block.c2 = fifa96_camera_behavior_blocks[5].c2;
  block.c3 = fifa96_camera_behavior_blocks[5].c3;
  block.sub10 = fifa96_camera_behavior_blocks[5].sub10;
  block.sub14 = fifa96_camera_behavior_blocks[5].sub14;
  block.sub18 = fifa96_camera_behavior_blocks[5].sub18;
  block.sub1c = fifa96_camera_behavior_blocks[5].sub1c;
  block.cell18 = fifa96_camera_behavior_blocks[5].cell18;
  block.cell1c = fifa96_camera_behavior_blocks[5].cell1c;
  block.pan = fifa96_camera_behavior_blocks[5].pan;
  block.pan_step = fifa96_camera_behavior_blocks[5].pan_step;
  memset(&st, 0, sizeof st);
  memset(&out, 0, sizeof out);
  assert(fifa96_projection_sincos(block.field3, &sin16, &cos16) == FIFA96_OK);
  assert(fifa96_camera_behavior_staged(&st, &block, 0, &out) == FIFA96_OK);
  assert(block.class_of == 3 && out.class_of == 3);
  scaled = (int32_t)(((int64_t)block.sub14 * sin16 + 0x8000) >> 16);
  assert(block.sub18 == (scaled < 0 ? -scaled : scaled));
  scaled = (int32_t)(((int64_t)block.sub14 * cos16 + 0x8000) >> 16);
  assert(block.const10 == (scaled < 0 ? -scaled : scaled));
  assert(out.horizon_a == 0x9C3 && out.horizon_b == 0x58B);

  /* The mutable handler-block copy used by the live view selector. */
  memset(&block, 0xAA, sizeof block);
  assert(fifa96_camera_behavior_block(&block, 5) == FIFA96_OK);
  assert(block.class_of == fifa96_camera_behavior_blocks[5].class_of);
  assert(block.target0 == fifa96_camera_behavior_blocks[5].target0);
  assert(block.field3 == fifa96_camera_behavior_blocks[5].field3);
  assert(block.const40 == fifa96_camera_behavior_blocks[5].const40);
  assert(block.c0 == fifa96_camera_behavior_blocks[5].c0);
  assert(block.c3 == fifa96_camera_behavior_blocks[5].c3);
  assert(block.sub10 == fifa96_camera_behavior_blocks[5].sub10);
  assert(block.sub14 == fifa96_camera_behavior_blocks[5].sub14);
  assert(block.cell18 == fifa96_camera_behavior_blocks[5].cell18);
  assert(block.pan == 0 && block.pan_step == 0);
  assert(fifa96_camera_behavior_block(&block, 6) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_behavior_block(NULL, 0) == -FIFA96_ERR_INVALID);

  /* FUN_0004F1C8: variant 1 selects target0 0xB4 and the sub-record +0x20
   * half (row 4 / slot 1). Variant 0 keeps the +0x00 half. */
  memset(&block, 0, sizeof block);
  assert(fifa96_camera_behavior_block_variant(&block, 4, 1) == FIFA96_OK);
  assert(block.target0 == 0xB4);
  assert(block.c0 == -200 && block.c1 == 2100);
  assert(block.c2 == 1600 && block.c3 == 1600);
  assert(block.sub10 == 700 && block.sub14 == 1150);
  assert(block.sub18 == 0 && block.sub1c == 0);
  assert(block.class_of == 1);
  memset(&block, 0, sizeof block);
  assert(fifa96_camera_behavior_block_variant(&block, 4, 0) == FIFA96_OK);
  assert(block.target0 == 0x340);
  assert(block.c0 == 600 && block.c1 == 2400);
  assert(block.c2 == 1500 && block.c3 == 1500);
  assert(fifa96_camera_behavior_block_variant(NULL, 0, 0) ==
         -FIFA96_ERR_INVALID);
}

static void test_focus_clamp(void) {
  /* FU-351-L1 / FUN_00036C70 head: ±6000 x/z fallback replaces all three;
   * the Y band is 0..16000. */
  fifa96_camera_focus f;
  assert(fifa96_camera_focus_clamp(123, 456, -789, &f) == FIFA96_OK);
  assert(f.x == 123 && f.y == 456 && f.z == -789);
  assert(fifa96_camera_focus_clamp(6000, 16000, -6000, &f) == FIFA96_OK);
  assert(f.x == 6000 && f.y == 16000 && f.z == -6000);
  assert(fifa96_camera_focus_clamp(6001, 42, 0, &f) == FIFA96_OK);
  assert(f.x == -4000 && f.y == 0 && f.z == -4000);
  assert(fifa96_camera_focus_clamp(0, 42, -6001, &f) == FIFA96_OK);
  assert(f.x == -4000 && f.y == 0 && f.z == -4000);
  assert(fifa96_camera_focus_clamp(1, -1, 2, &f) == FIFA96_OK);
  assert(f.x == 1 && f.y == 0 && f.z == 2);
  assert(fifa96_camera_focus_clamp(1, 16001, 2, &f) == FIFA96_OK);
  assert(f.x == 1 && f.y == 0 && f.z == 2);
  assert(fifa96_camera_focus_clamp(0, 0, 0, NULL) == -FIFA96_ERR_INVALID);

  /* FUN_0004BE44: the away-bank addend follows [0x157AC2] & 1 and the
   * >= 4 gate (FUN_0004B678). Side 1 is the default addend side. */
  assert(fifa96_camera_target_slot(4, 0, 0) == 4);
  assert(fifa96_camera_target_slot(4, 1, 0) == 15);
  assert(fifa96_camera_target_slot(4, 0, 1) == 15);
  assert(fifa96_camera_target_slot(4, 1, 1) == 4);
  assert(fifa96_camera_target_slot(4, 0, 3) == 15);
  assert(fifa96_camera_target_slot(4, 1, 3) == 4);
  assert(fifa96_camera_target_slot(4, 0, 4) == 4);
  assert(fifa96_camera_target_slot(4, 1, 4) == 15);
  assert(fifa96_camera_target_slot(4, 1, 8) == 15);
  assert(fifa96_camera_target_slot(10, 0, 0) == 10);
  assert(fifa96_camera_target_slot(10, 1, 0) == 21);
}

static void test_camera_slide(void) {
  /* FU-351-L2 / FUN_0004D668: the Q16 ramp factor. */
  assert(fifa96_camera_ramp_factor(8, 2) == 0x4444);
  assert(fifa96_camera_ramp_factor(8, 8) == 0x1111);
  assert(fifa96_camera_ramp_factor(0, 2) == 0);
  assert(fifa96_camera_ramp_factor(8, 0) == 0);
  assert(fifa96_camera_ramp_factor(-1, 2) == 0);

  /* FUN_0004D698 mode 2 inside the bounds: clamped target 150, rate 8,
   * tick 8 -> step 10. The negative target mirrors to -10. */
  assert(fifa96_camera_slide(0, 1000, 0, 2, -150, 150, 0, 8, 8) == 10);
  assert(fifa96_camera_slide(0, -1000, 0, 2, -150, 150, 0, 8, 8) == -10);
  /* Low/high clamps. The inside branch uses the block rate, so rate 1 with
   * tick 1000 overshoots the clamped target and hits the bound. */
  assert(fifa96_camera_slide(0, -1000, 0, 2, -5, 150, 0, 1, 1000) == -5);
  assert(fifa96_camera_slide(0, 1000, 0, 2, -150, 5, 0, 1, 1000) == 5);
  /* The mode -1 / mode 1 history-direction gates return the old cell. */
  assert(fifa96_camera_slide(0, 1000, 1200, -1, -150, 150, 0, 8, 8) == 0);
  assert(fifa96_camera_slide(0, -1000, -1200, 1, -150, 150, 0, 8, 8) == 0);
  /* Unlisted modes are identity. */
  assert(fifa96_camera_slide(7, 1000, 0, 0, -150, 150, 0, 8, 8) == 7);
  assert(fifa96_camera_slide(7, 1000, 0, 3, -150, 150, 0, 8, 8) == 7);
  /* Outside (lo, hi): the native no-history branch uses rate 8 and does not
   * clamp to hi. */
  assert(fifa96_camera_slide(200, 1000, 0, 2, -150, 150, 0, 8, 8) == 197);
  assert(fifa96_camera_slide(-200, 1000, 0, 2, -150, 150, 0, 8, 8) == -177);
  /* hist_offset is the native target subtraction. */
  assert(fifa96_camera_slide(0, 1000, 0, 2, -150, 150, 900, 8, 8) == 7);
  /* FUN_0004D698 0x4D6D4 loads rate 8 on the outside branch; 0x4D705 loads
   * the block rate on the strict inside branch. */
  assert(fifa96_camera_slide(0, 10, 0, 2, -150, 150, 0, 1, 100) == 67);
  assert(fifa96_camera_slide(200, 1000, 0, 2, -150, 150, 0, 1, 100) == 158);
}

static void test_behavior_steady_slides(void) {
  /* FUN_0004E834's two FUN_0004D698 calls. Block 0 (type 0) carries
   * +0x10=0xC8, +0x18=0x578, +0x28=8, C record {150,150,1000,3800}.
   * Class 3 takes lo=-C[2], hi=C[3] and hist_offset=-cell18. */
  fifa96_camera_handler_state st;
  fifa96_camera_handler_block block;
  fifa96_camera_handler_out out;
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  memset(&out, 0, sizeof out);
  block.class_of = 3;
  block.target0 = 0x340;
  block.target1 = 4000;
  block.const10 = 0xC8;
  block.const18 = 0x578;
  block.const20 = 8;
  block.const24 = 8;
  block.const28 = 8;
  block.const30 = 0;
  block.const34 = 0;
  block.const3c = 600;
  block.const40 = 80;
  block.c0 = 150;
  block.c1 = 150;
  block.c2 = 1000;
  block.c3 = 3800;
  block.sub18 = 1330;
  block.sub1c = 1750;
  block.cell18 = 0x578;

  st.track_x = 1000;
  st.track_z = 0;
  assert(fifa96_camera_behavior_steady(&st, &block, 8, &out) == FIFA96_OK);
  assert(st.pos_x == 10);
  assert(st.pos_z == 93);

  /* The z mode-1 history gate: target -1600, history -1200 gives a positive
   * direction and a negative step; the native returns the old cell. */
  st.pos_z = 500;
  st.track_z = -1000;
  st.hist_z = -1200;
  assert(fifa96_camera_behavior_steady(&st, &block, 8, &out) == FIFA96_OK);
  assert(st.pos_z == 500);

  /* The class-3 high bound is C[3] (3800), not C[2] (1000): a 5000 target
   * with the -cell18 history offset clamps to 3800 and steps 253. */
  block.cell18 = 0x578;
  st.pos_z = 0;
  st.track_z = 5000;
  st.hist_z = 0;
  assert(fifa96_camera_behavior_steady(&st, &block, 8, &out) == FIFA96_OK);
  assert(st.pos_z == 253);

  /* Class 4 swaps the bounds and the history sign: target -3000 with
   * +cell18 clamps to -C[3] = -3800 and steps -253. */
  block.class_of = 4;
  block.cell18 = 0x578;
  st.pos_z = 0;
  st.track_z = -3000;
  st.hist_z = 0;
  assert(fifa96_camera_behavior_steady(&st, &block, 8, &out) == FIFA96_OK);
  assert(st.pos_z == -253);
}

static void test_camera_pan_step(void) {
  /* FUN_0004E248 (`0x4E248..0x4E3A5`): the zero-input view-driver call decays
   * the cell by the 20*0x1111 Q16 step (the native returns -pan/-f20). */
  assert(fifa96_camera_steady_pan_step(0, 0, 100, 80, 0, 8) == -11);
  assert(fifa96_camera_steady_pan_step(0, 0, 5, 80, 0, 8) == -5);
  assert(fifa96_camera_steady_pan_step(0, 0, -100, 80, 0, 8) == 11);
  assert(fifa96_camera_steady_pan_step(0, 0, 0, 80, 0, 8) == 0);
  /* One-sided input returns the 32*0x1111 step with its sign. */
  assert(fifa96_camera_steady_pan_step(1, 0, 0, 80, 0, 8) == 17);
  assert(fifa96_camera_steady_pan_step(-1, 0, 0, 80, 0, 8) == -17);
  /* Opposite signs use the 32*0x1111 step only while |track_z| <= 0x587. */
  assert(fifa96_camera_steady_pan_step(1, -1, 0, 80, 0, 8) == -17);
  assert(fifa96_camera_steady_pan_step(-1, 1, 0, 80, 0, 8) == 17);
  assert(fifa96_camera_steady_pan_step(1, -1, 0, 80, 0x588, 8) == 0);
  /* Same signs use the block-rate step. */
  assert(fifa96_camera_steady_pan_step(1, 1, 0, 80, 0, 8) == 43);
  assert(fifa96_camera_steady_pan_step(-1, -1, 0, 80, 0, 8) == -43);
}

static void test_behavior_steady_cells(void) {
  /* FUN_0004E834 0x4E84A..0x4E8C8: add the pan step to +0x34, clamp it to
   * +-block[0xF] and to the camera-relative band, then add the same step to
   * +0x18 and clamp that cell to the +0x50 record +0x18/+0x1C. */
  fifa96_camera_handler_state st;
  fifa96_camera_handler_block block;
  fifa96_camera_handler_out out;
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  memset(&out, 0, sizeof out);
  block.class_of = 3;
  block.target0 = 0x340;
  block.target1 = 4000;
  block.const20 = 8;
  block.const24 = 8;
  block.const28 = 8;
  block.const30 = 0;
  block.const3c = 600;
  block.const40 = 80;
  block.sub18 = 1330;
  block.sub1c = 1750;
  block.cell18 = 1400;
  block.pan = 100;

  assert(fifa96_camera_behavior_steady(&st, &block, 8, &out) == FIFA96_OK);
  assert(block.pan_step == -11);
  assert(block.pan == 89);
  assert(block.cell1c == -11);
  assert(block.cell18 == 1389);

  /* The +-0x3C pad clamp. */
  block.pan = 600;
  block.cell18 = 1745;
  assert(fifa96_camera_behavior_steady(&st, &block, 8, &out) == FIFA96_OK);
  assert(block.pan == 589);
  assert(block.cell18 == 1734);

  /* A negative pan returns +f20, so the +0x18 cell reaches the record's
   * +0x1C = 1750 high clamp. */
  block.pan = -100;
  block.cell18 = 1745;
  assert(fifa96_camera_behavior_steady(&st, &block, 8, &out) == FIFA96_OK);
  assert(block.pan == -89);
  assert(block.cell18 == 1750);

  /* The camera-relative +-0xB10 band is applied after the +-0x3C clamp. */
  block.pan = 100;
  block.cell18 = 1400;
  st.track_z = 0xB00;
  assert(fifa96_camera_behavior_steady(&st, &block, 8, &out) == FIFA96_OK);
  assert(block.pan == 16);
}

static void test_behavior_steady_pitch(void) {
  /* FUN_0004E05C: the two look-ats, the ratio/8 midpoint, the D5F4
   * param4=1 half-step, the ratio correction, and the D668 rate. Eye height
   * 400 skips the low-altitude ground-point guard. */
  fifa96_camera_handler_state st;
  fifa96_camera_handler_block block;
  fifa96_camera_handler_out out;
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  memset(&out, 0, sizeof out);
  block.class_of = 3;
  block.target0 = 0x340;
  block.target1 = 3000;
  block.const10 = 200;
  block.const20 = 8;
  block.const24 = 8;
  block.const28 = 8;
  block.const30 = 0;
  block.const3c = 600;
  block.const40 = 80;
  block.c0 = 150;
  block.c1 = 150;
  block.c2 = 1000;
  block.c3 = 3800;
  block.sub18 = 0;
  block.sub1c = 0;
  block.cell18 = 0;
  st.pos_y = 400;
  st.ratio = 4000;
  st.pitch = 0;

  assert(fifa96_camera_behavior_steady(&st, &block, 8, &out) == FIFA96_OK);
  assert(st.pitch == 1317);
  assert(st.ratio == 3933);
  assert(st.pos_y == 429);
  assert(st.track_y == 0);
  assert(st.hist_y == 0);
  assert(out.horizon_a == 0x991 && out.horizon_b == 0x559);

  /* The low-altitude guard: eye height 0 takes the ground-point C77C with
   * point y=0x140, raises the blend to ratio/2, then the rate step is 133;
   * the y ease moves 0 -> 55. */
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  block.class_of = 3;
  block.target0 = 0x340;
  block.target1 = 4000;
  block.const10 = 200;
  block.const20 = 8;
  block.const24 = 8;
  block.const28 = 8;
  block.const30 = 0;
  block.const3c = 600;
  block.const40 = 80;
  block.c0 = 150;
  block.c1 = 150;
  block.c2 = 1000;
  block.c3 = 3800;
  block.sub18 = 0;
  block.sub1c = 0;
  block.cell18 = 0;
  st.ratio = 4000;
  st.pitch = 0;
  assert(fifa96_camera_behavior_steady(&st, &block, 8, &out) == FIFA96_OK);
  assert(st.pitch == 133);
  assert(st.pos_y == 55);
  assert(st.ratio == 4000);

  /* [0x107DD4] == 1 (the FUN_0004D04C variant): floor 0x2EE and point y =
   * camera_y << 5 instead of the 0x140 arm. */
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  block.class_of = 3;
  block.target0 = 0x340;
  block.target1 = 4000;
  block.const10 = 200;
  block.const20 = 8;
  block.const24 = 8;
  block.const28 = 8;
  block.const3c = 600;
  block.const40 = 80;
  block.c0 = 150;
  block.c1 = 150;
  block.c2 = 1000;
  block.c3 = 3800;
  st.view_variant = 1;
  st.ratio = 4000;
  assert(fifa96_camera_behavior_steady(&st, &block, 8, &out) == FIFA96_OK);
  assert(st.pitch == 267);
  assert(st.pos_y == 55);
  assert(st.ratio == 4000);
}

static void test_camera_yaw_snap(void) {
  /* FUN_0004E834 0x4E9A2..0x4EA9B: the class-3 yaw snap and the
   * non-class-3 mirror. The native calls FUN_0004C7D0 (the negated atan)
   * and stores the raw 32-bit target. */
  assert(fifa96_camera_yaw_snap(3, 0x720, 0, 0x8A1, 0xFF00, 1000) == -1000);
  assert(fifa96_camera_yaw_snap(3, -0x720, 0, 0x8A1, 0x1000, 1000) == 0x1000);
  assert(fifa96_camera_yaw_snap(3, -0x720, 0xC60, 0x8A1, 0x8000, 1000) ==
         0x83E8);
  assert(fifa96_camera_yaw_snap(3, 0x720, 0, 0x8A0, 0xFF00, 1000) == 0xFF00);
  /* Nonzero dx catches the FUN_0004C7D0 negation: without it the target
   * would land in the snap half, but the native target is behind 0x9000. */
  assert(fifa96_camera_yaw_snap(3, 0, 0, 0x8A1, 0x9000, 1000) == 0x9000);
  assert(fifa96_camera_yaw_snap(4, 0x720, -0xC00, -0x8A1, 0x9001, 1000) ==
         0x1F4);
  assert(fifa96_camera_yaw_snap(4, -0x720, -0xC00, -0x8A1, 0x1000, 1000) ==
         -500);
  assert(fifa96_camera_yaw_snap(4, 0x720, -0xC00, -0x8A0, 0x9001, 1000) ==
         0x9001);
}


static void test_camera_view_select(void) {
  /* FUN_0004B308 phase/option/0x15B680 presets + the FUN_0004D04C
   * {slot,variant} table and the selected camera-record class. */
  fifa96_camera_view v;
  assert(fifa96_camera_view_select(3, 1, 0, &v) == FIFA96_OK);
  assert(v.preset == 1 && v.slot == 1 && v.variant == 1 && v.class_of == 1);
  assert(fifa96_camera_view_select(3, 0, 0, &v) == FIFA96_OK);
  assert(v.preset == 2 && v.slot == 0 && v.variant == 0 && v.class_of == 0);
  assert(fifa96_camera_view_select(4, 1, 0, &v) == FIFA96_OK);
  assert(v.preset == 3 && v.slot == 0 && v.variant == 1 && v.class_of == 0);
  assert(fifa96_camera_view_select(4, 0, 0, &v) == FIFA96_OK);
  assert(v.preset == 4 && v.slot == 2 && v.variant == 0 && v.class_of == 2);
  assert(fifa96_camera_view_select(0x15, 0, 3, &v) == FIFA96_OK);
  assert(v.preset == 5 && v.slot == 2 && v.variant == 1 && v.class_of == 2);
  assert(fifa96_camera_view_select(0x15, 0, 0, &v) == FIFA96_OK);
  assert(v.preset == 6 && v.slot == 5 && v.variant == 1 && v.class_of == 3);
  assert(fifa96_camera_view_select(0x15, 0, 1, &v) == FIFA96_OK);
  assert(v.preset == 0 && v.slot == 1 && v.variant == 0 && v.class_of == 1);
  assert(fifa96_camera_view_select(0, 0, 0, &v) == FIFA96_OK);
  assert(v.preset == 0 && v.slot == 1 && v.variant == 0 && v.class_of == 1);
  assert(fifa96_camera_view_select(3, 2, 0, &v) == FIFA96_OK);
  assert(v.preset == 2 && v.slot == 0 && v.variant == 0);
  assert(fifa96_camera_view_select(0, 0, 0, NULL) == -FIFA96_ERR_INVALID);
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
  test_pose_apply_fields();
  test_pose_feed_mode1();
  test_pose_feed_mode1_selector1_mirror();
  test_pose_feed_mode3_variant();
  test_pose_feed_mode4_variant();
  test_pose_feed_mode6_record7();
  test_pose_feed_mode8_sub();
  test_pose_feed_mode15();
  test_pose_feed_default_and_off();
  test_pose_feed_invalid();
  test_ramp_values();
  test_event_set_window_pan();
  test_event_set_idle_height();
  test_event_set_height_clamps();
  test_event_set_anchor2_computed();
  test_event_set_bail_gate();
  test_event_set_ramp_param();
  test_event_set_resets_pan_counter();
  test_event_set_tail_clears();
  test_event_set_invalid();
  test_pan_step_band_decay_rearm();
  test_pan_step_idle_height_band_skipped();
  test_pan_step_walk();
  test_pan_step_invalid();
  test_update_walk_gate();
  test_update_walk_landing_pan();
  test_update_walk_captures_prev_triple();
  test_camera_commit_shadows();
  test_camera_walk_jitter_gates();
  test_camera_walk_jitter_draws();
  test_wall_arm_gate_and_head();
  test_wall_arm_z_chain_edges();
  test_wall_arm_x_chain_edges();
  test_wall_arm_both_chains();
  test_wall_arm_announce_buckets();
  test_wall_arm_width_traps();
  test_wall_arm_invalid();
  test_record_event_reflect_and_event();
  test_record_event_height_ramp();
  test_record_event_chaser_ramp_param_one();
  test_record_event_seed_word_wrap();
  test_record_event_jitter_and_gates();
  test_tracked_record();
  test_reposition_flag_overlap_noop();
  test_reposition_steps_back();
  test_reposition_same_zone_no_step();
  test_reposition_invalid();
  test_reposition_boundary_stat_high();
  test_reposition_boundary_stat_low();
  test_reposition_boundary_no_event();
  test_rate_table_arm();
  test_event_dir_byte();
  test_event_band_f6();
  test_camera_type_mapping();
  test_behavior_table();
  test_behavior_steady();
  test_behavior_steady_slides();
  test_camera_pan_step();
  test_behavior_steady_cells();
  test_behavior_steady_pitch();
  test_camera_yaw_snap();
  test_focus_clamp();
  test_camera_slide();
  test_camera_view_select();
  test_behavior_sidetrack();
  test_behavior_staged();
  test_behavior_action();
  test_pose_axis_a();
  test_pose_feed_default_handler_arm();
  test_classifier_bits();
  test_pan_band();
  test_rate_event();
  test_camera_height_producer();
  test_replay_select();
  test_camera_replay_blend();
  test_camera_action_table();
  test_camera_pan_target();
  test_camera_replay_choice();
  test_camera_target_select();
  puts("test_camera: ok");
  return 0;
}
