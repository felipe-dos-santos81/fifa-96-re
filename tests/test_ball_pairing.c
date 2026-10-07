// tests/test_ball_pairing.c — FU-73 ball-pairing/possession port (docs/ghidra/FU73_ball_pairing.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_entity_update.h"
#include "fifa96_loader/fifa96_ball_pairing.h"

_Static_assert(offsetof(fifa96_ball_pair_vector, x) == 0, "x");
_Static_assert(offsetof(fifa96_ball_pair_vector, height) == 2, "height");
_Static_assert(offsetof(fifa96_ball_pair_vector, z) == 4, "z");
_Static_assert(offsetof(fifa96_ball_pair_actor, position) == 0, "position");
_Static_assert(offsetof(fifa96_ball_pair_actor, velocity_x) == 6, "velocity_x");
_Static_assert(offsetof(fifa96_ball_pair_actor, velocity_z) == 8, "velocity_z");
_Static_assert(offsetof(fifa96_ball_pair_actor, has_ball) == 10, "has_ball");
_Static_assert(offsetof(fifa96_ball_pair_actor, kind) == 11, "kind");
_Static_assert(offsetof(fifa96_ball_pair_actor, action) == 12, "action");
_Static_assert(offsetof(fifa96_ball_pair_actor, flag) == 13, "flag");
_Static_assert(offsetof(fifa96_ball_pair_delta, distance) == 0, "distance");
_Static_assert(offsetof(fifa96_ball_pair_delta, dx) == 2, "dx");
_Static_assert(offsetof(fifa96_ball_pair_delta, dz) == 4, "dz");
_Static_assert(offsetof(fifa96_ball_pair_targets, team0_target) == 0, "team0_target");
_Static_assert(offsetof(fifa96_ball_pair_targets, team0_second) == 4, "team0_second");
_Static_assert(offsetof(fifa96_ball_pair_targets, team1_target) == 8, "team1_target");
_Static_assert(offsetof(fifa96_ball_pair_targets, team1_second) == 12, "team1_second");
_Static_assert(offsetof(fifa96_ball_pair_vec3i, x) == 0, "vec3i x");
_Static_assert(offsetof(fifa96_ball_pair_vec3i, y) == 4, "vec3i y");
_Static_assert(offsetof(fifa96_ball_pair_vec3i, z) == 8, "vec3i z");
_Static_assert(offsetof(fifa96_ball_pair_state, actor) == 0, "actor");
_Static_assert(offsetof(fifa96_ball_pair_state, receiver) == 4, "receiver");
_Static_assert(offsetof(fifa96_ball_pair_state, vector) == 8, "vector");
_Static_assert(offsetof(fifa96_ball_pair_state, traj) == 14, "traj");
_Static_assert(offsetof(fifa96_ball_pair_state, angle) == 16, "angle");
_Static_assert(offsetof(fifa96_ball_pair_state, flags) == 18, "flags");
_Static_assert(offsetof(fifa96_ball_pair_state, code) == 19, "code");
_Static_assert(offsetof(fifa96_ball_pair_state, sub_code) == 20, "sub_code");
_Static_assert(offsetof(fifa96_ball_pair_state, reserved45) == 21, "reserved45");
_Static_assert(offsetof(fifa96_ball_pair_state, ack) == 22, "ack");

static fifa96_ball_pair_vector vector(int16_t x, int16_t height, int16_t z) {
  fifa96_ball_pair_vector v;
  v.x = x;
  v.height = height;
  v.z = z;
  return v;
}

static fifa96_ball_pair_actor actor_at(int16_t x, int16_t z) {
  fifa96_ball_pair_actor a;
  memset(&a, 0, sizeof a);
  a.position = vector(x, 0, z);
  return a;
}

static fifa96_entity_candidate candidate(int16_t x, int16_t y, uint8_t s98, uint8_t s9a) {
  fifa96_entity_candidate c;
  c.x = x;
  c.y = y;
  c.skip_98 = s98;
  c.skip_9a = s9a;
  return c;
}

static void test_offset_zero_and_axes(void) {
  fifa96_ball_pair_delta out;
  assert(fifa96_ball_pair_offset(&(fifa96_ball_pair_vector){0, 0, 0},
                                 &(fifa96_ball_pair_vector){0, 0, 0}, &out) == FIFA96_OK);
  assert(out.distance == 0);
  assert(out.dx == 0);
  assert(out.dz == 0);
  assert(fifa96_ball_pair_offset(&(fifa96_ball_pair_vector){0, 0, 0},
                                 &(fifa96_ball_pair_vector){100, 0, 0}, &out) == FIFA96_OK);
  assert(out.distance == 100);
  assert(out.dx == 100);
  assert(out.dz == 0);
  assert(fifa96_ball_pair_offset(&(fifa96_ball_pair_vector){0, 0, 0},
                                 &(fifa96_ball_pair_vector){0, 0, 100}, &out) == FIFA96_OK);
  assert(out.distance == 100);
  assert(out.dx == 0);
  assert(out.dz == 100);
}

static void test_offset_metric_branches(void) {
  fifa96_ball_pair_delta out;
  assert(fifa96_ball_pair_offset(&(fifa96_ball_pair_vector){10, 0, 20},
                                 &(fifa96_ball_pair_vector){110, 0, 70}, &out) == FIFA96_OK);
  assert(out.distance == 112);
  assert(out.dx == 100);
  assert(out.dz == 50);
  assert(fifa96_ball_pair_offset(&(fifa96_ball_pair_vector){0, 0, 0},
                                 &(fifa96_ball_pair_vector){51, 0, 100}, &out) == FIFA96_OK);
  assert(out.distance == 118);
  assert(fifa96_ball_pair_offset(&(fifa96_ball_pair_vector){0, 0, 0},
                                 &(fifa96_ball_pair_vector){100, 0, 100}, &out) == FIFA96_OK);
  assert(out.distance == 137);
  assert(fifa96_ball_pair_offset(&(fifa96_ball_pair_vector){0, 0, 0},
                                 &(fifa96_ball_pair_vector){50, 0, 100}, &out) == FIFA96_OK);
  assert(out.distance == 112);
}

static void test_offset_signed_deltas(void) {
  fifa96_ball_pair_delta out;
  assert(fifa96_ball_pair_offset(&(fifa96_ball_pair_vector){20, 0, 30},
                                 &(fifa96_ball_pair_vector){10, 0, 20}, &out) == FIFA96_OK);
  assert(out.dx == -10);
  assert(out.dz == -10);
  assert(out.distance == 13);
  assert(fifa96_ball_pair_offset(&(fifa96_ball_pair_vector){20, 0, 5},
                                 &(fifa96_ball_pair_vector){-30, 0, -45}, &out) == FIFA96_OK);
  assert(out.dx == -50);
  assert(out.dz == -50);
  assert(out.distance == 68);
}

static void test_offset_word_semantics(void) {
  fifa96_ball_pair_delta out;
  assert(fifa96_ball_pair_offset(&(fifa96_ball_pair_vector){0, 0, 0},
                                 &(fifa96_ball_pair_vector){-32768, 0, 0}, &out) == FIFA96_OK);
  assert(out.dx == -32768);
  assert(out.dz == 0);
  assert(out.distance == -8192);
  assert(fifa96_ball_pair_offset(&(fifa96_ball_pair_vector){32767, 0, 0},
                                 &(fifa96_ball_pair_vector){-32768, 0, 0}, &out) == FIFA96_OK);
  assert(out.dx == 1);
  assert(out.distance == 1);
}

static void test_offset_invalid(void) {
  fifa96_ball_pair_delta out;
  assert(fifa96_ball_pair_offset(NULL, &(fifa96_ball_pair_vector){0, 0, 0}, &out) == -FIFA96_ERR_INVALID);
  assert(fifa96_ball_pair_offset(&(fifa96_ball_pair_vector){0, 0, 0}, NULL, &out) == -FIFA96_ERR_INVALID);
  assert(fifa96_ball_pair_offset(&(fifa96_ball_pair_vector){0, 0, 0},
                                 &(fifa96_ball_pair_vector){0, 0, 0}, NULL) == -FIFA96_ERR_INVALID);
}

static void test_decide_pairs_and_offsets(void) {
  fifa96_ball_pair_actor a = actor_at(0, 0);
  fifa96_ball_pair_actor b = actor_at(30, 0);
  fifa96_ball_pair_vector out;
  a.velocity_x = 10;
  assert(fifa96_ball_pair_decide(&a, &b, 2, &out) == 1);
  assert(out.x == 30 - 0x40);
  assert(out.height == 0);
  assert(out.z == 0 + 0x40);
}

static void test_decide_does_not_pair_when_not_closing(void) {
  fifa96_ball_pair_actor a = actor_at(0, 0);
  fifa96_ball_pair_actor b = actor_at(10, 0);
  fifa96_ball_pair_vector out = vector(7, 8, 9);
  assert(fifa96_ball_pair_decide(&a, &b, 5, &out) == 0);
  assert(out.x == 7);
  assert(out.height == 8);
  assert(out.z == 9);
}

static void test_decide_distance_threshold(void) {
  fifa96_ball_pair_actor a = actor_at(0, 0);
  fifa96_ball_pair_actor b = actor_at(103, 0);
  fifa96_ball_pair_vector out = vector(7, 8, 9);
  a.velocity_x = 20;
  assert(fifa96_ball_pair_decide(&a, &b, 2, &out) == 1);
  assert(out.x == 103 - 0x40);
  b.position.x = 104;
  out = vector(7, 8, 9);
  assert(fifa96_ball_pair_decide(&a, &b, 2, &out) == 0);
  assert(out.x == 7);
}

static void test_decide_z_axis_offsets(void) {
  fifa96_ball_pair_actor a = actor_at(0, 0);
  fifa96_ball_pair_actor b = actor_at(0, 103);
  fifa96_ball_pair_vector out = vector(7, 8, 9);
  a.velocity_z = 20;
  assert(fifa96_ball_pair_decide(&a, &b, 2, &out) == 1);
  assert(out.x == 0 + 0x40);
  assert(out.z == 103 - 0x40);
}

static void test_decide_invalid(void) {
  fifa96_ball_pair_actor a = actor_at(0, 0);
  fifa96_ball_pair_actor b = actor_at(0, 0);
  fifa96_ball_pair_vector out;
  assert(fifa96_ball_pair_decide(NULL, &b, 1, &out) == -FIFA96_ERR_INVALID);
  assert(fifa96_ball_pair_decide(&a, NULL, 1, &out) == -FIFA96_ERR_INVALID);
  assert(fifa96_ball_pair_decide(&a, &b, 1, NULL) == -FIFA96_ERR_INVALID);
}

static void test_receive_nearest_and_skip_zero(void) {
  fifa96_entity_candidate c[3] = {
      candidate(0, 0, 0, 0),
      candidate(10, 0, 0, 0),
      candidate(20, 0, 0, 0),
  };
  fifa96_ball_pair_actor actor = actor_at(0, 0);
  int32_t receiver = -77;
  assert(fifa96_ball_pair_receive(&actor, c, 3, 0, 0, &receiver) == 1);
  assert(receiver == 1);
  c[2].x = 0;
  assert(fifa96_ball_pair_receive(&actor, c, 3, 0, 0, &receiver) == 1);
  assert(receiver == 2);
  c[1].x = 10;
  c[2].x = 10;
  assert(fifa96_ball_pair_receive(&actor, c, 3, 0, 0, &receiver) == 1);
  assert(receiver == 1);
}

static void test_receive_skip_rule(void) {
  fifa96_entity_candidate c[3] = {
      candidate(50, 0, 0, 0),
      candidate(10, 0, 0, 0),
      candidate(0, 0, 0, 0),
  };
  fifa96_ball_pair_actor actor = actor_at(0, 0);
  int32_t receiver = -77;
  actor.kind = 2;
  actor.action = 0;
  actor.flag = 2;
  assert(fifa96_ball_pair_receive(&actor, c, 3, 0, 0, &receiver) == 1);
  assert(receiver == 2);
  actor.kind = 1;
  assert(fifa96_ball_pair_receive(&actor, c, 3, 0, 0, &receiver) == 1);
  assert(receiver == 1);
  actor.kind = 0;
  actor.action = 0x12;
  assert(fifa96_ball_pair_receive(&actor, c, 3, 0, 0, &receiver) == 1);
  assert(receiver == 1);
  actor.action = 0x11;
  assert(fifa96_ball_pair_receive(&actor, c, 3, 0, 0, &receiver) == 1);
  assert(receiver == 1);
  actor.action = 0x10;
  assert(fifa96_ball_pair_receive(&actor, c, 3, 0, 0, &receiver) == 1);
  assert(receiver == 1);
  actor.action = 0x0F;
  assert(fifa96_ball_pair_receive(&actor, c, 3, 0, 0, &receiver) == 1);
  assert(receiver == 2);
  actor.action = 0x12;
  actor.flag = 0x80;
  assert(fifa96_ball_pair_receive(&actor, c, 3, 0, 0, &receiver) == 1);
  assert(receiver == 2);
}

static void test_receive_exclusions(void) {
  fifa96_entity_candidate c[3] = {
      candidate(0, 0, 1, 0),
      candidate(10, 0, 0, 0),
      candidate(20, 0, 0, 1),
  };
  fifa96_ball_pair_actor actor = actor_at(0, 0);
  int32_t receiver = -77;
  assert(fifa96_ball_pair_receive(&actor, c, 3, 0, 0, &receiver) == 1);
  assert(receiver == 1);
  c[1].skip_9a = 1;
  assert(fifa96_ball_pair_receive(&actor, c, 3, 0, 0, &receiver) == 0);
  assert(receiver == -1);
}

static void test_receive_target_word_wrap(void) {
  fifa96_entity_candidate c[2] = {candidate(0, 0, 0, 0),
                                  candidate(-32768, 0, 0, 0)};
  fifa96_ball_pair_actor actor = actor_at(0, 0);
  int32_t receiver = -77;
  assert(fifa96_ball_pair_receive(&actor, c, 2, 32767, 0, &receiver) == 1);
  assert(receiver == 1);
}

static void test_receive_count_zero_and_invalid(void) {
  fifa96_entity_candidate c[1] = {candidate(0, 0, 0, 0)};
  fifa96_ball_pair_actor actor = actor_at(0, 0);
  int32_t receiver = -77;
  assert(fifa96_ball_pair_receive(&actor, c, 0, 0, 0, &receiver) == 0);
  assert(receiver == -1);
  assert(fifa96_ball_pair_receive(NULL, c, 1, 0, 0, &receiver) == -FIFA96_ERR_INVALID);
  assert(fifa96_ball_pair_receive(&actor, NULL, 1, 0, 0, &receiver) == -FIFA96_ERR_INVALID);
  assert(fifa96_ball_pair_receive(&actor, c, 1, 0, 0, NULL) == -FIFA96_ERR_INVALID);
}

static void test_assign_teams(void) {
  fifa96_ball_pair_targets t = {5, 5, 5, 5};
  assert(fifa96_ball_pair_assign(&t, 0, 7) == FIFA96_OK);
  assert(t.team0_target == 7);
  assert(t.team0_second == 0);
  assert(t.team1_target == 0);
  assert(t.team1_second == 0);
  t = (fifa96_ball_pair_targets){5, 5, 5, 5};
  assert(fifa96_ball_pair_assign(&t, 1, 9) == FIFA96_OK);
  assert(t.team0_target == 0);
  assert(t.team0_second == 0);
  assert(t.team1_target == 9);
  assert(t.team1_second == 0);
}

static void test_assign_invalid(void) {
  fifa96_ball_pair_targets t = {5, 5, 5, 5};
  assert(fifa96_ball_pair_assign(&t, 2, 1) == -FIFA96_ERR_INVALID);
  assert(t.team0_target == 5);
  assert(fifa96_ball_pair_assign(NULL, 0, 1) == -FIFA96_ERR_INVALID);
}

static void test_possess_and_release(void) {
  fifa96_ball_pair_actor a = actor_at(0, 0);
  assert(fifa96_ball_pair_possess(&a) == FIFA96_OK);
  assert(a.has_ball == 1);
  assert(fifa96_ball_pair_release(&a) == FIFA96_OK);
  assert(a.has_ball == 0);
  assert(fifa96_ball_pair_possess(NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_ball_pair_release(NULL) == -FIFA96_ERR_INVALID);
}

/* FU-139 §3.1: FU-73 §1 block clear (native 0x7A028..0x7A081): pointer/vector/
 * trajectory/angle words zero, flags 0x20, code 2, the three tail bytes 0. */
static void test_state_clear(void) {
  fifa96_ball_pair_state s;
  memset(&s, 0xAB, sizeof s);
  assert(fifa96_ball_pair_clear(&s) == FIFA96_OK);
  assert(s.actor == 0 && s.receiver == 0);
  assert(s.vector.x == 0 && s.vector.height == 0 && s.vector.z == 0);
  assert(s.traj == 0 && s.angle == 0);
  assert(s.flags == 0x20 && s.code == 2);
  assert(s.sub_code == 0 && s.reserved45 == 0 && s.ack == 0);
  assert(fifa96_ball_pair_clear(NULL) == -FIFA96_ERR_INVALID);
}

/* FU-139 §3.1: FU-73 §1 stage core (native 0x7A4D3..0x7A4EA): actor, the
 * 6-byte vector, the trajectory word and the event code byte; the flag bytes
 * are untouched by the core. */
static void test_state_stage(void) {
  fifa96_ball_pair_state s;
  fifa96_ball_pair_vector v = vector(0x123, -0x456, 0x789);
  memset(&s, 0, sizeof s);
  s.flags = 0x11;
  s.ack = 1;
  assert(fifa96_ball_pair_stage(&s, 0x42, &v, 0x99, 7) == FIFA96_OK);
  assert(s.actor == 0x42 && s.receiver == 0);
  assert(s.vector.x == 0x123 && s.vector.height == -0x456);
  assert(s.vector.z == 0x789);
  assert(s.traj == 0x99 && s.code == 7);
  assert(s.flags == 0x11 && s.ack == 1);
  assert(fifa96_ball_pair_stage(NULL, 1, &v, 1, 1) == -FIFA96_ERR_INVALID);
  assert(fifa96_ball_pair_stage(&s, 1, NULL, 1, 1) == -FIFA96_ERR_INVALID);
}

/* FU-139 §3.3: FU-73 §3.3 reception lead (native 0x7A2FF..0x7A331): the base
 * triple copied and x/z advanced by the camera-velocity words' high halves
 * scaled by 0x20; y untouched. */
static void test_receive_target(void) {
  fifa96_ball_pair_vec3i base;
  fifa96_ball_pair_vec3i out;
  base.x = 100;
  base.y = 200;
  base.z = 300;
  assert(fifa96_ball_pair_receive_target(&base, 0x00010000, 0xFFFF0000, &out) ==
         FIFA96_OK);
  assert(out.x == 100 + 0x20);
  assert(out.y == 200);
  assert(out.z == 300 - 0x20);
  assert(fifa96_ball_pair_receive_target(&base, 0x8000, 0x7FFF, &out) ==
         FIFA96_OK);
  assert(out.x == 100 && out.y == 200 && out.z == 300);
  assert(fifa96_ball_pair_receive_target(NULL, 0, 0, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_ball_pair_receive_target(&base, 0, 0, NULL) ==
         -FIFA96_ERR_INVALID);
}

/* FU-139 §8 (Task 10): the `FUN_0007A490` code-keyed staging tail
 * (`0x7A8D1..0x7AA2F`, first-hand). The caller-supplied recompute table is the
 * native eligibility byte `0x1104BB[code]` read this slice:
 * `{0,1,1,1,0,0,1,0,0,0,0,0,0,0,1}` for codes 0..0xE (codes 1/2/3/6/0xE take
 * the nudge/camera arm; everything else the native `FUN_0007A084` call). */
static const uint8_t stage_tail_recompute[15] = {0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1};

static fifa96_ball_stage_tail_actor stage_actor(void) {
  fifa96_ball_stage_tail_actor a;
  memset(&a, 0, sizeof a);
  a.pos_x = 0x1000;
  a.pos_z = 0x2000;
  a.nudge_x = 0x20000;    /* dword[+0x6B] */
  a.nudge_z = 0xFFFE0000; /* dword[+0x6D] */
  a.lane_gate = 0x5F;
  a.active = 1;
  a.type8 = 3;
  a.facing = 0x77;
  return a;
}

/* `0x7A8D1..0x7A93B`: the eligible codes take the recompute arm
 * (`[+0x69]>>16 < 0x60` then the two `>>17` dword adds, then the camera-velocity
 * zero); every other signed code calls the unported reception `FUN_0007A084`
 * (`receive == 1`, no nudge, no camera zero). */
static void test_stage_tail_recompute_and_receive(void) {
  fifa96_ball_pair_state s;
  fifa96_ball_stage_tail_actor a;
  fifa96_ball_stage_tail_out out;

  memset(&s, 0, sizeof s);
  s.code = 1;
  a = stage_actor();
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(out.recompute == 1 && out.receive == 0);
  assert(out.nudge == 1 && out.camera_zero == 1);
  assert(a.pos_x == 0x1000 + 1 && a.pos_z == 0x2000 - 1);

  memset(&s, 0, sizeof s);
  s.code = 0xE;
  a = stage_actor();
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(out.recompute == 1 && out.receive == 0);

  memset(&s, 0, sizeof s);
  s.code = 0;
  a = stage_actor();
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(out.receive == 1 && out.recompute == 0);
  assert(out.nudge == 0 && out.camera_zero == 0);
  assert(a.pos_x == 0x1000 && a.pos_z == 0x2000);

  memset(&s, 0, sizeof s);
  s.code = 0x0F;
  a = stage_actor();
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(out.receive == 1 && out.recompute == 0);

  memset(&s, 0, sizeof s);
  s.code = (uint8_t)-1;   /* signed -1 -> the native MAS < 0 arm */
  a = stage_actor();
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(out.receive == 1 && out.recompute == 0);

  /* lane gate at 0x60 suppresses the nudge but keeps the camera zero. */
  memset(&s, 0, sizeof s);
  s.code = 2;
  a = stage_actor();
  a.lane_gate = 0x60;
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(out.nudge == 0 && out.camera_zero == 1);
  assert(a.pos_x == 0x1000 && a.pos_z == 0x2000);
  assert(fifa96_ball_pair_stage_tail(NULL, &a, stage_tail_recompute, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_ball_pair_stage_tail(&s, NULL, stage_tail_recompute, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_ball_pair_stage_tail(&s, &a, NULL, &out) == -FIFA96_ERR_INVALID);
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, NULL) ==
         -FIFA96_ERR_INVALID);
}

/* `0x7A934..0x7A9D9`: an inactive actor with code 2 latches sub-code 0x30,
 * codes 1/3/6 latch 0x31, codes 4/5/7 run the whole-block reset
 * (`fifa96_ball_pair_clear`), and every other inactive code falls through to
 * the active face/anim arm. An active actor never rewrites the sub-code. */
static void test_stage_tail_inactive_subcode_and_clear(void) {
  fifa96_ball_pair_state s;
  fifa96_ball_stage_tail_actor a;
  fifa96_ball_stage_tail_out out;

  memset(&s, 0, sizeof s);
  s.code = 2;
  a = stage_actor();
  a.active = 0;
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(s.sub_code == 0x30 && out.cleared == 0);

  memset(&s, 0, sizeof s);
  s.code = 1;
  a = stage_actor();
  a.active = 0;
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(s.sub_code == 0x31);
  s.sub_code = 0;
  s.code = 3;
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(s.sub_code == 0x31);
  s.sub_code = 0;
  s.code = 6;
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(s.sub_code == 0x31);

  /* Active code 2: the sub-code write is skipped (native JNZ 0x7A9DE). */
  memset(&s, 0, sizeof s);
  s.code = 2;
  s.sub_code = 0x11;
  a = stage_actor();
  a.active = 1;
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(s.sub_code == 0x11);

  /* Inactive 4/5/7 -> the 0x7A97F..0x7A9D9 whole-block reset. */
  for (uint8_t code = 4; code <= 7; code++) {
    if (code == 6) continue;   /* 6 latches 0x31 instead */
    memset(&s, 0xAB, sizeof s);
    s.code = code;
    a = stage_actor();
    a.active = 0;
    assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
    assert(out.cleared == 1);
    assert(s.actor == 0 && s.receiver == 0);
    assert(s.vector.x == 0 && s.vector.height == 0 && s.vector.z == 0);
    assert(s.traj == 0 && s.angle == 0);
    assert(s.flags == 0x20 && s.code == 2);
    assert(s.sub_code == 0 && s.reserved45 == 0 && s.ack == 0);
    assert(out.anim == 0);
  }

  /* Inactive code 0 falls through to the anim arm without a sub-code write. */
  memset(&s, 0, sizeof s);
  s.code = 0;
  s.sub_code = 0x22;
  a = stage_actor();
  a.active = 0;
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(out.cleared == 0 && s.sub_code == 0x22);
  assert(out.anim == 0x22);
}

/* `0x7A9DE..0x7AA2B`: codes 1/2/3/6 take the 0x79C50 face with
 * `dx = word[0x15873A]` and `dz = word[0x15873C]` (the staged block's second
 * and third words; `state->vector.height`/`state->vector.z`), every code takes
 * the 0x6E598 animation resolution with `kind = sub_code`, and a live control
 * slot runs the unported `0x78B00` callback. */
static void test_stage_tail_face_anim_and_slot(void) {
  fifa96_ball_pair_state s;
  fifa96_ball_stage_tail_actor a;
  fifa96_ball_stage_tail_out out;

  memset(&s, 0, sizeof s);
  s.code = 1;
  s.vector.height = 0x100;   /* dx -> +x octant 2 */
  s.vector.z = 0;
  s.sub_code = 0x33;
  a = stage_actor();
  a.active = 1;
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(out.face == 2);
  assert(a.facing == 2);
  assert(out.anim == 0x33);
  assert(out.slot_cb == 0);

  /* dz-only -> octant 0; the zero-direction guard keeps the seed. */
  memset(&s, 0, sizeof s);
  s.code = 2;
  s.vector.height = 0;
  s.vector.z = 0x100;
  a = stage_actor();
  a.active = 1;
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(out.face == 0);
  memset(&s, 0, sizeof s);
  s.code = 6;
  s.vector.height = 0;
  s.vector.z = 0;
  a = stage_actor();
  a.active = 1;
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(out.face == 0x77);

  /* Non-face code with a slot: only the anim resolution + the slot callback. */
  memset(&s, 0, sizeof s);
  s.code = 4;
  s.sub_code = 0x40;
  a = stage_actor();
  a.active = 1;
  a.has_slot = 1;
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(out.face == 0x77);
  assert(out.anim == 0x40);
  assert(out.slot_cb == 1);

  /* kind 0 resolves through the derived row stand-in. */
  memset(&s, 0, sizeof s);
  s.code = 0;
  s.sub_code = 0;
  a = stage_actor();
  a.active = 1;
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(out.anim == 0);

  /* The 0x6F clamp. */
  memset(&s, 0, sizeof s);
  s.code = 4;
  s.sub_code = 0x6F;
  a = stage_actor();
  a.active = 1;
  assert(fifa96_ball_pair_stage_tail(&s, &a, stage_tail_recompute, &out) == FIFA96_OK);
  assert(out.anim == 0);
}

int main(void) {
  test_offset_zero_and_axes();
  test_offset_metric_branches();
  test_offset_signed_deltas();
  test_offset_word_semantics();
  test_offset_invalid();
  test_decide_pairs_and_offsets();
  test_decide_does_not_pair_when_not_closing();
  test_decide_distance_threshold();
  test_decide_z_axis_offsets();
  test_decide_invalid();
  test_receive_nearest_and_skip_zero();
  test_receive_skip_rule();
  test_receive_exclusions();
  test_receive_target_word_wrap();
  test_receive_count_zero_and_invalid();
  test_assign_teams();
  test_assign_invalid();
  test_possess_and_release();
  test_state_clear();
  test_state_stage();
  test_receive_target();
  test_stage_tail_recompute_and_receive();
  test_stage_tail_inactive_subcode_and_clear();
  test_stage_tail_face_anim_and_slot();
  puts("test_ball_pairing: ok");
  return 0;
}
