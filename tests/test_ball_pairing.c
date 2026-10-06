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
  puts("test_ball_pairing: ok");
  return 0;
}
