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
   * F2 = ramp(0x30) = 25, timer_limit = 50, F6 = 25 - ramp(0x30) = 0,
   * timer = 50, vel_z = 2000/50 = 40, speed = 40,
   * anchor_z = 0xB00 + 40*50 = 0x12D0, anchor_time = anchor2_time = 50. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0xB00);
  assert(fifa96_camera_event_set(&cam, 0, 2000, 0x30) == FIFA96_OK);
  assert(cam.event_param == 0x30);
  assert(cam.timer == 50 && cam.timer_limit == 50);
  assert(cam.vel_x == 0 && cam.vel_z == 40);
  assert(cam.speed == 40);
  assert(cam.anchor_x == 0 && cam.anchor_z == 0x12D0);
  assert(cam.anchor_time == 50 && cam.anchor2_time == 50);
  assert(cam.anchor2_x == 0 && cam.anchor2_z == 0x12D0);
  assert(cam.rate_x == 0 && cam.rate_z == 0 && cam.event_cursor == 0);
  /* the accepted integrator then moves the camera: one delta-2 frame */
  assert(fifa96_camera_update(&cam, 2, 0, 0) == FIFA96_OK);
  assert(cam.pos_z == 0xB50);
  assert(cam.timer == 52);
  assert(fifa96_camera_out_of_bounds(cam.pos_x, cam.pos_z) == 1);
}

static void test_event_set_idle_height(void) {
  /* height < 1 takes the F2=6 / timer 0xC branch. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, 0, 0);
  assert(fifa96_camera_event_set(&cam, 0x30, 0, 0) == FIFA96_OK);
  assert(cam.event_param == 0);
  assert(cam.timer == 0xC && cam.timer_limit == 0xC);
  assert(cam.vel_x == 4 && cam.vel_z == 0);
  assert(cam.speed == 4);
}

static void test_event_set_height_clamps(void) {
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 0, -10, 0);
  assert(fifa96_camera_event_set(&cam, 0, 0, -20) == FIFA96_OK);
  /* clamped to the target y, then zeroed by the native F2<1 branch */
  assert(cam.event_param == 0);
  assert(cam.timer == 0xC);
  fifa96_camera_init(&cam, 0, 0, 0);
  assert(fifa96_camera_event_set(&cam, 0, 0, 0x700) == FIFA96_OK);
  assert(cam.event_param == 0x640);
  assert(cam.timer_limit == 0x94 * 2 && cam.timer == 0x94 * 2);
}

static void test_event_set_anchor2_computed(void) {
  /* height 0x60 > 0x4F: a2_time = F2 + ramp(0x10) = 36 + 15 = 51; with ty = 0,
   * F6 = F2 - F2 = 0 and span = 51 >= 1, so anchor2 = origin + vel * 51.
   * F2 = ramp(0x60) = 36, timer = 72; seed 2160 / 72 = vel 30. */
  fifa96_camera cam = fresh_camera();
  fifa96_camera_init(&cam, 100, 0, 0);
  assert(fifa96_camera_event_set(&cam, 0, 2160, 0x60) == FIFA96_OK);
  assert(cam.timer == 72 && cam.timer_limit == 72);
  assert(cam.vel_z == 30);
  assert(cam.anchor2_time == 51);
  assert(cam.anchor2_x == 100 && cam.anchor2_z == 30 * 51);
  assert(cam.anchor_time == 72 && cam.anchor_z == 30 * 72);
}

static void test_event_set_invalid(void) {
  assert(fifa96_camera_event_set(NULL, 0, 0, 0) == -FIFA96_ERR_INVALID);
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
  /* FUN_0004E834: yaw accumulator clamped into the block sub-record bounds
   * [0x18,0x1C], pitch clamp +-0x1620, horizon jitter rnd>>3 + 0xE09/0xAA1. */
  fifa96_camera_handler_state st;
  fifa96_camera_handler_block block;
  fifa96_camera_handler_out out;
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  memset(&out, 0, sizeof out);
  block.yaw_lo = -100;
  block.yaw_hi = 100;
  st.yaw = 0;
  st.pitch = 0x2000;
  assert(fifa96_camera_behavior_steady(&st, &block, 0x40, &out) == FIFA96_OK);
  assert(st.yaw == 0);
  assert(st.pitch == 0x1620);
  assert(out.horizon_a == 0xE09 + 8 && out.horizon_b == 0xAA1 + 8);
  st.yaw = -500;
  assert(fifa96_camera_behavior_steady(&st, &block, 0, &out) == FIFA96_OK);
  assert(st.yaw == -100 && out.horizon_a == 0xE09);
  st.yaw = 500;
  assert(fifa96_camera_behavior_steady(&st, &block, 0, &out) == FIFA96_OK);
  assert(st.yaw == 100);
  st.pitch = -0x2000;
  assert(fifa96_camera_behavior_steady(&st, &block, 0, &out) == FIFA96_OK);
  assert(st.pitch == -0x1620);
  assert(fifa96_camera_behavior_steady(NULL, &block, 0, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_behavior_steady(&st, NULL, 0, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_camera_behavior_steady(&st, &block, 0, NULL) ==
         -FIFA96_ERR_INVALID);
}

static void test_behavior_sidetrack(void) {
  /* FUN_0004E3A8: +-0x68 vertical bias by class (2 -> +, else -), yaw target
   * track_z + block[0xD] clamped +-0xB10, pitch clamp +-0xE40, horizon
   * rnd>>3 + 0xE3B/0xAD3. */
  fifa96_camera_handler_state st;
  fifa96_camera_handler_block block;
  fifa96_camera_handler_out out;
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  st.track_y = 1000;
  st.track_z = 0xB00;
  block.const34 = 0x100;
  block.class_of = 2;
  st.pitch = 0x2000;
  assert(fifa96_camera_behavior_sidetrack(&st, &block, 8, &out) == FIFA96_OK);
  assert(st.track_y == 1000 + 0x68);
  assert(st.yaw == 0xB10);               /* 0xC00 clamped */
  assert(st.pitch == 0xE40);
  assert(out.horizon_a == 1 + 0xE3B && out.horizon_b == 1 + 0xAD3);

  memset(&st, 0, sizeof st);
  block.class_of = 3;
  st.track_y = 1000;
  st.track_z = -0xB00;
  block.const34 = -0x100;
  assert(fifa96_camera_behavior_sidetrack(&st, &block, 0, &out) == FIFA96_OK);
  assert(st.track_y == 1000 - 0x68);
  assert(st.yaw == -0xB10);
}

static void test_behavior_staged(void) {
  /* FUN_0004EC9C (fresh decompile): the class gate is block[3] (dword +0xC),
   * <0x4000 || >0xC000 -> class 4 else class 3; the pitch target is the
   * active record's +0x18 (the staged pos_z) plus block[0xC] in the class-4
   * arm / minus it in the class-3 arm, clamped +-0x1620; the horizon jitter
   * is rnd>>3 + 0xE3B/0xAD3. The native yaw-band snap (accumulated
   * camera+0x58 + roll vs 0x2000/0xA000, 0x720/0xB10 snap targets) stays
   * leg 9. */
  fifa96_camera_handler_state st;
  fifa96_camera_handler_block block;
  fifa96_camera_handler_out out;
  memset(&st, 0, sizeof st);
  memset(&block, 0, sizeof block);
  memset(&out, 0, sizeof out);
  block.field3 = 0x5000;      /* inside 0x4000..0xC000 -> class 3 */
  block.const30 = 0x100;
  st.pitch = 0x9000;
  assert(fifa96_camera_behavior_staged(&st, &block, 8, &out) == FIFA96_OK);
  assert(out.class_of == 3);
  assert(st.pitch == -0x100);                    /* 0 - 0x100 */
  assert(out.horizon_a == 1 + 0xE3B && out.horizon_b == 1 + 0xAD3);
  /* outside the band -> class 4, plus-const30 target */
  block.field3 = 0x1000;
  st.pos_z = 0x1000;
  block.const30 = 0x200;
  assert(fifa96_camera_behavior_staged(&st, &block, 0, &out) == FIFA96_OK);
  assert(out.class_of == 4);
  assert(st.pitch == 0x1200);
  /* the +-0x1620 clamp on both arms */
  block.const30 = 0x2000;
  assert(fifa96_camera_behavior_staged(&st, &block, 0, &out) == FIFA96_OK);
  assert(st.pitch == 0x1620);
  block.field3 = 0x5000;
  st.pos_z = 0;
  assert(fifa96_camera_behavior_staged(&st, &block, 0, &out) == FIFA96_OK);
  assert(st.pitch == -0x1620);
  assert(fifa96_camera_behavior_staged(NULL, &block, 0, &out) ==
         -FIFA96_ERR_INVALID);
}

static void test_behavior_action(void) {
  /* FUN_0004DB38: `if (FUN_0006400C() == 0) param_4 += 100;`, pos x +-0x420,
   * pos z +-0x810, pitch clamp [0x2000,63000], the brake words zeroed. */
  fifa96_camera_handler_state st;
  fifa96_camera_handler_out out;
  memset(&st, 0, sizeof st);
  memset(&out, 0, sizeof out);
  st.pos_x = 0x500;
  st.pos_z = -0x900;
  st.pitch = 0x1000;
  st.brake_lo = 5;
  st.brake_hi = 6;
  assert(fifa96_camera_behavior_action(&st, 0, 500, &out) == FIFA96_OK);
  assert(out.speed == 600);
  assert(st.pos_x == 0x420 && st.pos_z == -0x810);
  assert(st.pitch == 0x2000);
  assert(st.brake_lo == 0 && st.brake_hi == 0);
  st.pitch = 0x10000;
  assert(fifa96_camera_behavior_action(&st, 1, 500, &out) == FIFA96_OK);
  assert(out.speed == 500);
  assert(st.pitch == 63000);
  assert(fifa96_camera_behavior_action(NULL, 1, 0, &out) == -FIFA96_ERR_INVALID);
  assert(fifa96_camera_behavior_action(&st, 1, 0, NULL) == -FIFA96_ERR_INVALID);
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
  args.handler_rnd = 0x40;
  st.pos_x = 100;
  st.pos_z = -50;
  st.yaw = 0;
  st.pitch = 0x2000;
  block.yaw_lo = -100;
  block.yaw_hi = 100;
  int32_t yaw = 7, pitch = 7, ratio = 7;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(cam.pos_x == 100 && cam.pos_z == -50);
  assert(yaw == 0 && pitch == 0x1620);
  assert(out.horizon_a == 0xE09 + 8 && out.horizon_b == 0xAA1 + 8);

  /* selector 3 = the action handler (unsigned live flag + speed). */
  memset(&st, 0, sizeof st);
  args.selector = 3;
  args.handler_live = 0;
  args.handler_speed = 500;
  st.pos_x = 0x500;
  assert(fifa96_camera_pose_feed(&cam, &yaw, &pitch, &ratio, &args) == 1);
  assert(out.speed == 600 && cam.pos_x == 0x420);

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

static void test_behavior_table(void) {
  /* First-hand 0x10896C (504 B) rows {target0, target1, class, field3(+0xC),
   * const30, const34}: classes {3,1,3,3,1,3}; the +0xC class-gate cells are
   * {0,0,0xA7F8,0,0,0xA21C} so blocks 2/5 take FUN_0004EC9C's class-3 arm
   * (0x4000..0xC000) and 0/1/3/4 the class-4 arm; every +0x34 is zero
   * (handler 1's yaw target is the bare track z). */
  assert(FIFA96_CAMERA_BEHAVIOR_BLOCKS == 6);
  const int32_t classes[6] = {3, 1, 3, 3, 1, 3};
  const int32_t gate[6] = {0, 0, 0xA7F8, 0, 0, 0xA21C};
  for (int i = 0; i < 6; i++) {
    assert(fifa96_camera_behavior_blocks[i].class_of == classes[i]);
    assert(fifa96_camera_behavior_blocks[i].field3 == gate[i]);
    assert(fifa96_camera_behavior_blocks[i].const34 == 0);
    assert(fifa96_camera_behavior_blocks[i].target0 == 0x340);
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
   * 0x4000..0xC000 -> class 3 (pitch target pos_z - const30). Class 3 also
   * drives the sidetrack -0x68 bias. */
  fifa96_camera_handler_block block;
  memset(&block, 0, sizeof block);
  block.class_of = fifa96_camera_behavior_blocks[5].class_of;
  block.field3 = fifa96_camera_behavior_blocks[5].field3;
  block.const30 = fifa96_camera_behavior_blocks[5].const30;
  block.const34 = fifa96_camera_behavior_blocks[5].const34;
  fifa96_camera_handler_state st;
  fifa96_camera_handler_out out;
  memset(&st, 0, sizeof st);
  memset(&out, 0, sizeof out);
  assert(fifa96_camera_behavior_staged(&st, &block, 0, &out) == FIFA96_OK);
  assert(out.class_of == 3);
  assert(st.pitch == -0x1130);   /* pos_z 0 - const30 */
  st.track_y = 0;
  assert(fifa96_camera_behavior_sidetrack(&st, &block, 0, &out) == FIFA96_OK);
  assert(st.track_y == -0x68);
  assert(st.yaw == 0);   /* const34 0 */
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
  test_event_set_invalid();
  test_camera_type_mapping();
  test_behavior_table();
  test_behavior_steady();
  test_behavior_sidetrack();
  test_behavior_staged();
  test_behavior_action();
  test_pose_feed_default_handler_arm();
  test_classifier_bits();
  test_pan_band();
  test_rate_event();
  test_replay_select();
  puts("test_camera: ok");
  return 0;
}
