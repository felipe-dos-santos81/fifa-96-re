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
  puts("test_action_possession: all assertions passed");
  return 0;
}
