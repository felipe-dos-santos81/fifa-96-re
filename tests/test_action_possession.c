// tests/test_action_possession.c — FU-78 possession block and tackle/duel gates
// (docs/ghidra/FU78_possession_tackle.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_action_handlers.h"

_Static_assert(offsetof(fifa96_action_possession, carrier) == 0, "carrier");
_Static_assert(offsetof(fifa96_action_possession, index) == 4, "index");
_Static_assert(offsetof(fifa96_action_possession, rotation) == 5, "rotation");
_Static_assert(offsetof(fifa96_action_possession, dir_x) == 6, "dir_x");
_Static_assert(offsetof(fifa96_action_possession, dir_z) == 7, "dir_z");
_Static_assert(offsetof(fifa96_action_possession, counter_c) == 8, "counter_c");
_Static_assert(offsetof(fifa96_action_possession, release_timer) == 9, "release_timer");
_Static_assert(offsetof(fifa96_action_possession, counter_e) == 10, "counter_e");
_Static_assert(offsetof(fifa96_action_possession, counter_f) == 11, "counter_f");
_Static_assert(sizeof(fifa96_action_possession) == 12, "possession size");

_Static_assert(offsetof(fifa96_action_dribble_dir, dir_x) == 0, "dir_x");
_Static_assert(offsetof(fifa96_action_dribble_dir, dir_z) == 1, "dir_z");
_Static_assert(offsetof(fifa96_action_dribble_dir, speed) == 2, "speed");
_Static_assert(offsetof(fifa96_action_dribble_dir, resolved) == 4, "resolved");

_Static_assert(offsetof(fifa96_action_receive, timer89) == 0, "timer89");
_Static_assert(offsetof(fifa96_action_receive, offset_word) == 4, "offset_word");
_Static_assert(offsetof(fifa96_action_receive, stage) == 6, "stage");
_Static_assert(offsetof(fifa96_action_receive, active) == 7, "active");
_Static_assert(offsetof(fifa96_action_receive, event_flag) == 8, "event_flag");
_Static_assert(offsetof(fifa96_action_receive, is_team_target) == 9, "is_team_target");

_Static_assert(offsetof(fifa96_action_receive_out, reset) == 0, "reset");
_Static_assert(offsetof(fifa96_action_receive_out, advance) == 1, "advance");
_Static_assert(offsetof(fifa96_action_receive_out, handoff) == 2, "handoff");
_Static_assert(offsetof(fifa96_action_receive_out, stage) == 3, "stage");

_Static_assert(offsetof(fifa96_action_tackle, pos_x) == 0, "pos_x");
_Static_assert(offsetof(fifa96_action_tackle, target_height) == 32, "target_height");
_Static_assert(offsetof(fifa96_action_tackle, phase) == 56, "phase");
_Static_assert(offsetof(fifa96_action_tackle, own_bound) == 42, "own_bound");
_Static_assert(offsetof(fifa96_action_tackle_out, reset) == 0, "reset");
_Static_assert(offsetof(fifa96_action_tackle_out, install_0e) == 1, "install_0e");
_Static_assert(offsetof(fifa96_action_tackle_out, install_0f) == 2, "install_0f");
_Static_assert(offsetof(fifa96_action_tackle_out, stage) == 3, "stage");

_Static_assert(offsetof(fifa96_action_duel, timer89) == 0, "timer89");
_Static_assert(offsetof(fifa96_action_duel, distance) == 12, "distance");
_Static_assert(offsetof(fifa96_action_duel, stage) == 18, "stage");
_Static_assert(offsetof(fifa96_action_duel, stride) == 21, "stride");
_Static_assert(offsetof(fifa96_action_duel_out, wait) == 0, "wait");
_Static_assert(offsetof(fifa96_action_duel_out, handoff) == 1, "handoff");
_Static_assert(offsetof(fifa96_action_duel_out, reset) == 2, "reset");
_Static_assert(offsetof(fifa96_action_duel_out, stage) == 3, "stage");

#define ACTION_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static fifa96_action_possession possession_dirty(void) {
  fifa96_action_possession p;
  p.carrier = 0x1234;
  p.index = 1;
  p.rotation = 2;
  p.dir_x = 3;
  p.dir_z = 4;
  p.counter_c = 5;
  p.release_timer = 6;
  p.counter_e = 7;
  p.counter_f = 8;
  return p;
}

static fifa96_action_tackle tackle_base(void) {
  fifa96_action_tackle t;
  memset(&t, 0, sizeof t);
  t.pos_x = 0x100;
  t.pos_z = 0x700;
  t.camera_x = 0;
  t.target_x = 0x180;
  t.target_z = 0x700;
  t.target_height = 0x20;
  t.cam_f8 = 0x14;
  t.close_word = 0x100;
  t.facing = 0;
  t.phase = 2;
  t.is_tracked = 1;
  t.active = 1;
  t.side = 0;
  t.timer89 = 0;
  t.opp_close = 0x100;
  t.opp_bound = 0x200;
  t.own_bound = 0x200;
  t.vector_x = 0x100;
  t.vector_z = 0x700;
  t.cam_f2 = 0;
  t.cam_fa = 1;
  t.cam_100 = 1;
  t.cam_fe = 0x14;
  t.stage = 1;
  t.has_slot = 1;
  t.is_own = 1;
  return t;
}

static void test_possession_reset(void) {
  fifa96_action_possession p = possession_dirty();
  assert(fifa96_action_possession_reset(&p) == FIFA96_OK);
  assert(p.carrier == 0);
  assert(p.index == 0 && p.rotation == 0 && p.dir_x == 0 && p.dir_z == 0);
  assert(p.counter_c == 0 && p.release_timer == 0 && p.counter_e == 0 && p.counter_f == 0);
  assert(fifa96_action_possession_reset(NULL) == ACTION_INVALID);
}

static void test_possession_claim(void) {
  fifa96_action_possession p = possession_dirty();
  int claimed = -1;
  assert(fifa96_action_possession_claim(&p, 0x1234, &claimed) == FIFA96_OK);
  assert(claimed == 0);
  assert(p.carrier == 0x1234 && p.index == 1 && p.dir_x == 3);
  assert(fifa96_action_possession_claim(&p, 0x99, &claimed) == FIFA96_OK);
  assert(claimed == 1);
  assert(p.carrier == 0x99);
  assert(p.index == 0 && p.rotation == 0 && p.dir_x == 0 && p.dir_z == 0);
  assert(p.counter_c == 0 && p.release_timer == 0 && p.counter_e == 0 && p.counter_f == 0);
  assert(fifa96_action_possession_claim(NULL, 1, &claimed) == ACTION_INVALID);
  assert(fifa96_action_possession_claim(&p, 1, NULL) == ACTION_INVALID);
}

static void test_possession_timer(void) {
  int32_t timer = 0;
  assert(fifa96_action_possession_timer(&timer, 10) == FIFA96_OK);
  assert(timer == 10);
  timer = 0x4AF;
  assert(fifa96_action_possession_timer(&timer, 1) == FIFA96_OK);
  assert(timer == 0x4B0);
  assert(fifa96_action_possession_timer(&timer, 1) == FIFA96_OK);
  assert(timer == 0x4B0);
  timer = 0x4B1;
  assert(fifa96_action_possession_timer(&timer, 0xFFFF) == FIFA96_OK);
  assert(timer == 0x4B1);
  timer = 0x4B0;
  assert(fifa96_action_possession_timer(&timer, 0) == FIFA96_OK);
  assert(timer == 0x4B0);
  assert(fifa96_action_possession_timer(NULL, 1) == ACTION_INVALID);
}

static void test_possession_dribble_dir(void) {
  const int8_t type_x[16] = {0, 1, 1, 1, 0, -1, -1, -1, 1, 1, 0, -1, -1, -1, 0, 1};
  const int8_t type_z[16] = {1, 1, 0, -1, -1, -1, 0, 1, 0, 1, 1, 1, 0, -1, -1, 0};
  fifa96_action_dribble_dir out;
  assert(fifa96_action_possession_dribble_dir(5, 0x39, 0, 0, 0, type_x, type_z, &out) == FIFA96_OK);
  assert(out.dir_x == -1 && out.dir_z == -1 && out.speed == 0x60 && out.resolved == 1);
  assert(fifa96_action_possession_dribble_dir(15, 0x39, 0, 0, 0, type_x, type_z, &out) == FIFA96_OK);
  assert(out.dir_x == 1 && out.dir_z == 0);
  memset(&out, 0xAA, sizeof out);
  assert(fifa96_action_possession_dribble_dir(0, 0x38, 1, -7, 9, type_x, type_z, &out) == FIFA96_OK);
  assert(out.dir_x == -7 && out.dir_z == 9 && out.speed == 0x30 && out.resolved == 1);
  assert(fifa96_action_possession_dribble_dir(0, 0x38, 0, 0, 0, type_x, type_z, &out) == FIFA96_OK);
  assert(out.dir_x == 0 && out.dir_z == 0 && out.speed == 0 && out.resolved == 0);
  assert(fifa96_action_possession_dribble_dir(0, 0, 0, 0, 0, type_x, type_z, &out) == FIFA96_OK);
  assert(out.resolved == 0);
  assert(fifa96_action_possession_dribble_dir(0, 0x39, 0, 0, 0, NULL, type_z, &out) == ACTION_INVALID);
  assert(fifa96_action_possession_dribble_dir(0, 0x39, 0, 0, 0, type_x, NULL, &out) == ACTION_INVALID);
  assert(fifa96_action_possession_dribble_dir(0, 0x39, 0, 0, 0, type_x, type_z, NULL) == ACTION_INVALID);
}

static void test_receive_step(void) {
  fifa96_action_receive r;
  fifa96_action_receive_out out;
  memset(&r, 0, sizeof r);
  r.stage = 0;
  r.active = 0;
  assert(fifa96_action_receive_step(&r, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.advance == 0 && out.handoff == 0);
  r.active = 1;
  r.offset_word = 0x41;
  r.timer89 = 0x3C;
  assert(fifa96_action_receive_step(&r, &out) == FIFA96_OK);
  assert(out.reset == 0 && out.advance == 0);
  r.timer89 = 0x3D;
  r.is_team_target = 1;
  assert(fifa96_action_receive_step(&r, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.handoff == 1);
  r.is_team_target = 0;
  assert(fifa96_action_receive_step(&r, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.handoff == 0);
  memset(&r, 0, sizeof r);
  r.stage = 0;
  r.active = 1;
  r.offset_word = 0x40;
  r.timer89 = 0x100;
  assert(fifa96_action_receive_step(&r, &out) == FIFA96_OK);
  assert(out.advance == 1 && out.stage == 1 && out.reset == 0);
  assert(r.stage == 1 && r.timer89 == 0);
  assert(fifa96_action_receive_step(&r, &out) == FIFA96_OK);
  assert(out.reset == 0);
  r.event_flag = 1;
  r.is_team_target = 1;
  assert(fifa96_action_receive_step(&r, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.handoff == 1);
  r.event_flag = 0;
  r.offset_word = 0x41;
  assert(fifa96_action_receive_step(&r, &out) == FIFA96_OK);
  assert(out.reset == 1);
  memset(&r, 0, sizeof r);
  r.stage = 2;
  assert(fifa96_action_receive_step(&r, &out) == FIFA96_OK);
  assert(out.reset == 0 && out.advance == 0 && out.handoff == 0);
  assert(fifa96_action_receive_step(NULL, &out) == ACTION_INVALID);
  assert(fifa96_action_receive_step(&r, NULL) == ACTION_INVALID);
}

static void test_tackle_attempt(void) {
  fifa96_action_tackle t = tackle_base();
  int install = -1;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK);
  assert(install == 1);
  t.phase = 1;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 0);
  t = tackle_base();
  t.is_tracked = 0;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 0);
  t = tackle_base();
  t.cam_f8 = 0x13;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 0);
  t = tackle_base();
  t.close_word = 0x181;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 0);
  t = tackle_base();
  t.target_height = 0x1F;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 0);
  t.target_height = 0x61;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 0);
  t = tackle_base();
  t.target_x = t.pos_x + 0xF1;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 0);
  t = tackle_base();
  t.pos_x = -0x211;
  t.camera_x = -0x300;
  t.target_x = t.pos_x + 1;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 0);
  t = tackle_base();
  t.pos_x = 0x211;
  t.target_x = t.pos_x + 0x80;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 0);
  t = tackle_base();
  t.pos_z = 0x68F;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 0);
  t = tackle_base();
  t.side = 1;
  t.pos_z = -0x68F;
  t.target_z = t.pos_z;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 0);
  t = tackle_base();
  t.side = 1;
  t.pos_z = -0x700;
  t.target_z = t.pos_z + 0x80;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 0);
  t = tackle_base();
  t.side = 1;
  t.pos_z = -0x700;
  t.target_z = -0x700;
  t.target_x = t.pos_x + 1;
  t.facing = 0x100;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 1);
  t.facing = 0x200;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 1);
  t.facing = 0x201;
  assert(fifa96_action_tackle_attempt(&t, &install) == FIFA96_OK && install == 0);
  assert(fifa96_action_tackle_attempt(NULL, &install) == ACTION_INVALID);
  assert(fifa96_action_tackle_attempt(&t, NULL) == ACTION_INVALID);
}

static void test_tackle_step(void) {
  fifa96_action_tackle t = tackle_base();
  fifa96_action_tackle_out out;
  t.stage = 0;
  t.delta = 10;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK);
  assert(out.reset == 0 && out.install_0e == 1 && out.install_0f == 0);
  assert(t.timer89 == 0 && t.stage == 1);
  t = tackle_base();
  t.stage = 0;
  t.active = 0;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.install_0e == 0);
  t = tackle_base();
  t.phase = 1;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK);
  assert(out.reset == 1);
  t = tackle_base();
  t.is_tracked = 0;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK);
  assert(out.reset == 0 && out.install_0e == 0 && out.install_0f == 1);
  t = tackle_base();
  t.lob = 1;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.install_0f == 0);
  t = tackle_base();
  t.is_tracked = 0;
  t.timer89 = 0x78;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK);
  assert(out.reset == 0 && out.install_0f == 1);
  t.timer89 = 0x79;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK);
  assert(out.reset == 1);
  t = tackle_base();
  t.is_tracked = 0;
  t.slot_button_40 = 1;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK);
  assert(out.install_0f == 0 && out.reset == 0);
  t = tackle_base();
  t.is_tracked = 0;
  t.flag99 = 1;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK && out.install_0f == 0);
  t = tackle_base();
  t.is_tracked = 0;
  t.field5d = 1;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK && out.install_0f == 0);
  t = tackle_base();
  t.is_tracked = 0;
  t.cam_fa = 0x10;
  t.cam_f2 = 0x10;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK && out.install_0f == 0);
  t = tackle_base();
  t.is_tracked = 0;
  t.is_own = 0;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK && out.install_0f == 0);
  t = tackle_base();
  t.is_tracked = 0;
  t.opp_close = 0x120;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK && out.install_0f == 0);
  t = tackle_base();
  t.is_tracked = 0;
  t.opp_close = 0x201;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK && out.install_0f == 0);
  t = tackle_base();
  t.is_tracked = 0;
  t.cam_f8 = 0x10;
  t.cam_100 = 1;
  t.cam_fe = 0x12;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK && out.install_0f == 0);
  t = tackle_base();
  t.is_tracked = 0;
  t.vector_x = t.pos_x + 0x60;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK && out.install_0f == 0);
  t = tackle_base();
  t.is_tracked = 0;
  t.flag99 = 1;
  t.close_word = 0x201;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.install_0f == 0);
  t = tackle_base();
  t.stage = 2;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK);
  assert(out.reset == 0 && out.install_0e == 0 && out.install_0f == 0);
  t.close_word = 0x201;
  assert(fifa96_action_tackle_step(&t, &out) == FIFA96_OK);
  assert(out.reset == 1);
  assert(fifa96_action_tackle_step(NULL, &out) == ACTION_INVALID);
  assert(fifa96_action_tackle_step(&t, NULL) == ACTION_INVALID);
}

static void test_duel_step(void) {
  fifa96_action_duel d;
  fifa96_action_duel_out out;
  memset(&d, 0, sizeof d);
  d.pos_x = 0x800;
  d.pos_z = 0x100;
  d.stage = 0;
  d.animation = 0x55;
  assert(fifa96_action_duel_step(&d, 5, 0, &out) == FIFA96_OK);
  assert(d.stride == 2 && d.timer89 == 5);
  assert(out.wait == 0 && out.handoff == 0 && out.reset == 0 && out.stage == 0);
  d.animation = 0x6A;
  assert(fifa96_action_duel_step(&d, 5, 0, &out) == FIFA96_OK);
  assert(out.stage == 0 && out.handoff == 0);
  d.animation = 0;
  assert(fifa96_action_duel_step(&d, 5, 0, &out) == FIFA96_OK);
  assert(out.stage == 2 && d.stage == 2 && d.timer89 == 0);
  assert(d.delta_x == 0x100 && d.delta_z == -0x100);
  assert(d.distance == 0x160);
  assert(out.wait == 1 && out.handoff == 0 && out.reset == 0);
  d.timer89 = 0x76;
  assert(fifa96_action_duel_step(&d, 1, 0, &out) == FIFA96_OK);
  assert(out.wait == 1 && out.handoff == 0 && out.reset == 0);
  d.timer89 = 0x77;
  d.distance = 0x20;
  d.has_slot = 1;
  assert(fifa96_action_duel_step(&d, 1, 0, &out) == FIFA96_OK);
  assert(out.wait == 1);
  d.timer89 = 0x78;
  d.distance = 0x1F;
  assert(fifa96_action_duel_step(&d, 1, 0, &out) == FIFA96_OK);
  assert(out.handoff == 1 && out.reset == 1 && out.wait == 0);
  d = (fifa96_action_duel){0};
  d.stage = 2;
  d.timer89 = 0x78;
  d.distance = 0x20;
  assert(fifa96_action_duel_step(&d, 1, 0x40, &out) == FIFA96_OK);
  assert(out.handoff == 0 && out.reset == 1);
  d = (fifa96_action_duel){0};
  d.stage = 2;
  d.timer89 = 0x12D;
  d.distance = 0xFF;
  d.has_slot = 1;
  assert(fifa96_action_duel_step(&d, 1, 0, &out) == FIFA96_OK);
  assert(out.handoff == 1 && out.reset == 1);
  d = (fifa96_action_duel){0};
  d.stage = 2;
  d.timer89 = 0x78;
  d.distance = 0x20;
  assert(fifa96_action_duel_step(&d, 1, 0x0F, &out) == FIFA96_OK);
  assert(out.wait == 1);
  assert(fifa96_action_duel_step(NULL, 1, 0, &out) == ACTION_INVALID);
  assert(fifa96_action_duel_step(&d, 1, 0, NULL) == ACTION_INVALID);
}

/* FU-139 §8 (Task 10): the row-05 carrier machine (`0x7F194..0x7F665`,
 * first-hand). `fifa96_action_carrier_arm` covers the bounded entries: the
 * phase/claim/timer head, the stage-0 gates with the ported dribble-dir
 * helper, stages 1-3 (0x92820/0x6E598/0x79B1C/0x79C50/0x7D9A4 surfaces) and
 * the derived requests for the unported stage-0 tail and `FUN_0007F7E0`
 * fallback (OL-63). */
static fifa96_action_carrier carrier_base(void) {
  fifa96_action_carrier c;
  memset(&c, 0, sizeof c);
  c.actor = 0x77;
  c.phase = 2;
  c.active = 1;
  c.facing = 0x5;
  c.type8 = 3;
  c.stage92 = 0;
  return c;
}

static const int8_t carrier_type_x[16] = {0, 1, 0, -1};
static const int8_t carrier_type_z[16] = {1, 0, -1, 0};

/* `0x7F1A6`: a non-phase-2 record resets (`fifa96_arm_reset`). */
static void test_carrier_phase_reset(void) {
  fifa96_action_carrier c = carrier_base();
  fifa96_action_carrier_out out;
  fifa96_action_possession p = possession_dirty();
  c.phase = 1;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.reset == 1);
  assert(out.ran_set == 1);   /* 0x7F19F runs before the phase gate */
  assert(out.claim == 0 && out.stage == 0 && out.tail == 0 && out.handoff == 0);
  assert(p.carrier == 0x1234);   /* no claim on the reset path */
}

/* `0x7F1BF..0x7F23A`: claim the possession block, write the team pointers,
 * cap/add the timer, and bail before the stage switch while `+0x81 != 0`. */
static void test_carrier_claim_timer_and_timer81(void) {
  fifa96_action_carrier c = carrier_base();
  fifa96_action_carrier_out out;
  fifa96_action_possession p;
  memset(&p, 0, sizeof p);
  p.carrier = 0x11;
  c.timer89 = 0x4AF;
  c.delta = 1;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.claim == 1 && p.carrier == 0x77);
  assert(out.ran_set == 1);
  assert(out.team_target == 1);
  assert(c.timer89 == 0x4B0);
  assert(out.target_camera == 1);
  /* same carrier -> claimed == 0 */
  c.timer81 = 1;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.claim == 0);
  assert(out.target_camera == 0);   /* early return before the camera copy */
  c.timer81 = 0;
  c.timer89 = 0x4B0;
  c.delta = 5;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(c.timer89 == 0x4B0);
  assert(fifa96_action_carrier_arm(NULL, &c, carrier_type_x, carrier_type_z, &out) == ACTION_INVALID);
  assert(fifa96_action_carrier_arm(&p, NULL, carrier_type_x, carrier_type_z, &out) == ACTION_INVALID);
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, NULL) == ACTION_INVALID);
}

/* `0x7F274..0x7F3A0`: stage-0 head gates and the dribble-dir arms. */
static void test_carrier_stage0_gates(void) {
  fifa96_action_carrier c = carrier_base();
  fifa96_action_carrier_out out;
  fifa96_action_possession p = possession_dirty();

  /* lane > 0x40 clears the controlled actor and returns. */
  c.lane = 0x41;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.clear_control == 1 && out.set_control == 0 && out.tail == 0);
  assert(out.team_target == 1);

  /* lane <= 0x40 sets the controlled actor; close > bound waits. */
  c = carrier_base();
  c.lane = 0x40;
  c.close_word = 0x40;
  c.bound_word = 0x3F;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.set_control == 1 && out.tail == 0);

  /* release countdown active -> wait. */
  c.close_word = 0x20;
  c.bound_word = 0x3F;
  p.release_timer = 1;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.set_control == 1 && out.tail == 0 && out.dirs == 0);

  /* airborne -> wait. */
  p.release_timer = 0;
  c.airborne = 1;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.tail == 0);

  /* Type arm: `[0x157750] > 0x38` -> per-type dir bytes, speed 0x60. */
  c = carrier_base();
  c.ball_height = 0x39;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.dirs == 1 && out.dir_x == (uint8_t)carrier_type_x[3] && out.dir_z == (uint8_t)carrier_type_z[3]);
  assert(out.tail == 1 && out.fallback == 0 && out.slot_merge == 0);

  /* Slot arm: `[0x157750] <= 0x38` with a slot -> slot bytes, speed 0x30. */
  c = carrier_base();
  c.ball_height = 0x38;
  c.has_slot = 1;
  c.slot_dir_x = 0x12;
  c.slot_dir_z = 0xF0;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.dirs == 1 && out.dir_x == 0x12 && out.dir_z == 0xF0);
  assert(out.tail == 1);

  /* No type, no slot, team gates pass -> the 0x7876C merge request; the
   * continuation (slot arm/tail vs FUN_0007F7E0 + type8 gate) belongs to the
   * merge result, so `tail` is not claimed. */
  c = carrier_base();
  c.ball_height = 0x38;
  c.team_slot_pool = 1;
  c.team_chosen = 0;
  c.active = 1;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.slot_merge == 1 && out.fallback == 0 && out.tail == 0);

  /* Team gates fail -> the FUN_0007F7E0 fallback; 0x7F374..0x7F380 only lets
   * type8 == 5 continue into the tail. */
  c = carrier_base();
  c.ball_height = 0x38;
  c.team_slot_pool = 0;
  c.type8 = 4;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.fallback == 1 && out.slot_merge == 0 && out.tail == 0);
  c.type8 = 5;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.fallback == 1 && out.tail == 1);

  /* The type tables are only required when the type arm is taken. */
  c = carrier_base();
  c.ball_height = 0x39;
  assert(fifa96_action_carrier_arm(&p, &c, NULL, carrier_type_z, &out) == ACTION_INVALID);
  c.ball_height = 0x38;
  c.has_slot = 1;
  assert(fifa96_action_carrier_arm(&p, &c, NULL, NULL, &out) == FIFA96_OK);
}

/* `0x7F57C..0x7F5E8`: stage 1 runs the 0x92820 sink, zeros the camera
 * velocity, resolves the animation (active -> 6, inactive -> 0x30), clears the
 * timer and advances the latch. */
static void test_carrier_stage1(void) {
  fifa96_action_carrier c = carrier_base();
  fifa96_action_carrier_out out;
  fifa96_action_possession p = possession_dirty();
  c.stage92 = 1;
  c.active = 1;
  c.timer89 = 0x55;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.sink == 1 && out.camera_zero == 1);
  assert(out.anim == 6);
  assert(c.timer89 == 0);
  assert(out.stage == 2 && out.snap == 0 && out.handoff == 0);
  c = carrier_base();
  c.stage92 = 1;
  c.active = 0;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.anim == 0x30);
}

/* `0x7F5E9..0x7F665`: stage 2 snaps, faces by the slot direction and loops to
 * stage 0 while the slot word is live; stage 3 resets the latch and hands the
 * ball actor to code 4 only for the team target. */
static void test_carrier_stage2_and_stage3(void) {
  fifa96_action_carrier c = carrier_base();
  fifa96_action_carrier_out out;
  fifa96_action_possession p = possession_dirty();

  c.stage92 = 2;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.snap == 0 && out.stage == 2);

  c.has_slot = 1;
  c.slot_live = 0;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.snap == 1 && out.face == 0x5);   /* zero direction keeps the seed */
  assert(out.stage == 2);

  c.slot_live = 1;
  c.slot_dir_x = 0x64;
  c.slot_dir_z = 0;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.snap == 1 && out.face == 2 && out.stage == 0);

  c = carrier_base();
  c.stage92 = 3;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.stage == 3 && out.handoff == 0);
  c.event_flag44 = 1;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.stage == 0 && out.handoff == 0);
  c.is_team_target = 1;
  c.stage92 = 3;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.stage == 0 && out.handoff == 1);

  /* stages > 3 fall straight through (native CMP AL,3 / JA). */
  c = carrier_base();
  c.stage92 = 4;
  assert(fifa96_action_carrier_arm(&p, &c, carrier_type_x, carrier_type_z, &out) == FIFA96_OK);
  assert(out.stage == 4 && out.sink == 0 && out.snap == 0);
}

static void test_duel_split(void) {
  uint8_t own = 0;
  uint8_t opp = 0;
  assert(fifa96_action_duel_split(0x100, 0x100, 1, &own, &opp) == FIFA96_OK);
  assert(own == 5 && opp == 6);
  assert(fifa96_action_duel_split(0x101, 0x100, 1, &own, &opp) == FIFA96_OK);
  assert(own == 5 && opp == 0);
  assert(fifa96_action_duel_split(0x100, 0x100, 0, &own, &opp) == FIFA96_OK);
  assert(own == 5 && opp == 0);
  assert(fifa96_action_duel_split(-1, -2, 1, &own, &opp) == FIFA96_OK);
  assert(own == 5 && opp == 0);
  assert(fifa96_action_duel_split(-0x8000, 0x7FFF, 1, &own, &opp) == FIFA96_OK);
  assert(own == 5 && opp == 6);
  assert(fifa96_action_duel_split(0, 0, 1, NULL, &opp) == ACTION_INVALID);
  assert(fifa96_action_duel_split(0, 0, 1, &own, NULL) == ACTION_INVALID);
}

int main(void) {
  test_possession_reset();
  test_possession_claim();
  test_possession_timer();
  test_possession_dribble_dir();
  test_receive_step();
  test_tackle_attempt();
  test_tackle_step();
  test_duel_step();
  test_duel_split();
  test_carrier_phase_reset();
  test_carrier_claim_timer_and_timer81();
  test_carrier_stage0_gates();
  test_carrier_stage1();
  test_carrier_stage2_and_stage3();
  puts("test_action_possession: all assertions passed");
  return 0;
}
