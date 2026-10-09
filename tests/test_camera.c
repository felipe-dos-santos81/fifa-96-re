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
  puts("test_camera: ok");
  return 0;
}
