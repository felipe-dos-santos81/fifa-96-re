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
  /* FU319 §3 (P-TURN, phase-33): the focus arm's z-only case — face_x = 0,
   * face_z = -1 -> the guard (action_handlers.c:223) fires on face_z, the
   * arm's internal face_target is kick_angle(0,-1) = 0x200, and the facing
   * steps to the limit (+0x40, speed 0, no slot); the octant cell follows
   * heading_table[((0x40 & 0x3FF) >> 5)]. (desired_facing stays the
   * target-derived angle: the arm's face_target is internal.) The
   * pre-existing pin only exercised face_x. */
  m = loco();
  m.target_x = 1;
  m.direct_face = 1;
  m.move_attr = 0x00500000;
  m.face_z = -1;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.facing == 0x40 && m.heading == heading[(0x40u & 0x3FFu) >> 5]);
  /* FU319 §2.2 (P-TURN): the native-value-2 equivalence — the native byte is
   * 2 (row_flags & 0x02) and the consumer 0x8E29C is a zero/nonzero test, so
   * direct_face = 2 must produce the identical outputs to the = 1 case
   * above (target branch, move_attr 0x00700000). */
  m = loco();
  m.target_x = 1;
  m.direct_face = 2;
  m.move_attr = 0x00700000;
  m.face_x = -1;
  m.delta = 10;
  assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
  assert(m.facing == 0x40);
}

/* FU324 §5/§6 + FU319 §4 (phase-33 task 2): the move_attr metric contract
 * pin. The mover stages `s.move_attr` as the native `dword[rec+0x63]` —
 * high word = the block-A distance (`FUN_0008DC68`, whose verbatim C port
 * is `fifa96_entity_distance`) over the record's low-word target-minus-pos,
 * low word unwritten (0; FU324-L2) — and the locomotion consumers extract
 * `>>16` (the turn's >0x60 branch, fifa96_action_handlers.c:221) and `>>19`
 * (the stride attr = dist>>3 under the stride_len clamp,
 * fifa96_action_handlers.c:260-262).
 *
 * GREEN at commit (the discipline ruling): the port already satisfies the
 * contract numerically — the native 0x8DC68/0x8DCD4 metric cores are the
 * SAME computation (4 000 800 sampled pairs, 0 diffs, FU324 §3.3) and the
 * metric-only patch moved 0 frames alone and 0 frames composed on the latch
 * build (FU324 §5). The pin's RED story is FU324's RECORDED MUTANT RUN
 * (`max + min/3`, a genuine metric change): it fires the stride arm 37 219
 * times in the M2 window yet moves 0 frames, because
 * `attr = min(dist>>3, stride_len <= 3)` masks every metric difference at
 * the current `+0x7B` nibble staging; the sensitivity returns only with the
 * nibble staging (FU324-L5) and the mutant protocol must re-run then. No
 * natural RED tuple exists (no real tuple separates the two metric values),
 * so none is manufactured here. */
/* The mover's staging expression (fifa96_match_run.c, the mover block; the
 * mover site carries the reciprocal flag): the high word is the metric's
 * 16-bit result, the low word 0. Keep this shadow in lockstep with that
 * staging — it re-states the expression because the value is never
 * persisted. */
static int32_t staged_shape(int16_t dx, int16_t dz) {
  return (int32_t)((uint32_t)(uint16_t)fifa96_entity_distance(dx, dz) << 16);
}

static void assert_move_attr_turn(int16_t dx, int16_t dz, int facing_focus) {
  int32_t d = fifa96_entity_distance(dx, dz);
  int32_t word = (int16_t)(uint16_t)d; /* the native AX store at word +0x65 */
  int32_t staged = staged_shape(dx, dz);
  fifa96_action_locomotion ref, real;
  int k;
  /* The staged dword shape: high word = the metric's 16-bit result (the
   * native block-A AX store), low word 0 (the native +0x63..+0x64 has no
   * writer and no word-width reader, FU324-L2). For every tuple with both
   * |dx|,|dz| < 0x8000 the word IS the metric value; the -0x8000 edges
   * truncate the rare negative overflow exactly as the native's AX store
   * does (e.g. (-0x8000,-0x8000) -> word +0x5000). */
  assert((staged >> 16) == word);
  assert((staged >> 19) == (word >> 3)); /* the native SAR 0x13 stride attr */
  ref = loco();
  ref.target_x = dx;
  ref.target_z = dz;
  ref.direct_face = 2;
  ref.face_x = -1;
  ref.delta = 0;
  ref.move_attr = 0x7FFF0000; /* forced: word 0x7FFF > 0x60 -> the target arm */
  real = ref;
  real.move_attr = staged; /* staged exactly as the mover stages it */
  for (k = 0; k < 12; k++) {
    assert(fifa96_action_locomotion_step(&ref, heading, stride_table) == FIFA96_OK);
    assert(fifa96_action_locomotion_step(&real, heading, stride_table) == FIFA96_OK);
  }
  /* The turn's branch decision must track the metric word at the tuple:
   * the staged-attr facing converges to the target arm's facing iff
   * word > 0x60, else to the forced-focus arm's (the two forced refs
   * bracket the decision; body_timer 0/delta 0 keeps the stride arm
   * unfired in all three runs). */
  assert(real.facing == (word > 0x60 ? ref.facing : facing_focus));
}

static void test_step_move_attr_metric_contract(void) {
  /* The octagon breakpoint set of FU324 §6 item 1, sign-crossed pairs
   * (both coordinates range over the word set; -0x8000 is +0x8000's word).
   * delta = 0 / body_timer = 0: the stride arm never fires in the turn
   * runs. Negative metric words (the -0x8000 edges) are shape-asserted and
   * turn-branched but never drive a stride index here. */
  static const int16_t v[] = {
      0,       1,       -1,      7,       -7,      8,       -8,      9,
      -9,      0x1FFE,  -0x1FFE, 0x1FFF,  -0x1FFF, 0x2000,  -0x2000, 0x2001,
      -0x2001, 0x2002,  -0x2002, 0x3FFE,  -0x3FFE, 0x3FFF,  -0x3FFF, 0x4000,
      -0x4000, 0x7FFE,  -0x7FFE, 0x7FFF,  -0x7FFF, -0x8000};
  /* The stride-consumer tuples: the 8-boundary ladder (attr = dist>>3
   * unclamped at stride_len 15) and the clamp family (>= 0x80 pins attr at
   * 15 — the boundary 15->16 masked, FU324 §5's clamp argument), the
   * metric's fold diagonal, and FU319's recorded row (-78,+51): 96 under
   * BOTH metric cores, the FU324-L1 erratum's own measurement. */
  static const int16_t st[][2] = {
      {0, 0},    {7, 0},    {8, 0},    {9, 0},    {0x3F, 0}, {0x40, 0},
      {0x41, 0}, {0x47, 0}, {0x48, 0}, {0x5F, 0}, {0x60, 0}, {0x61, 0},
      {0x6F, 0}, {0x70, 0}, {0x77, 0}, {0x78, 0}, {0x79, 0}, {0x7F, 0},
      {0x80, 0}, {0x81, 0}, {0xFF, 0}, {0x100, 0}, {0x7FFF, 0},
      {0x40, 0x40}, {-78, 51}};
  const int n = (int)(sizeof(v) / sizeof(v[0]));
  int i, j, k;
  int facing_focus;
  {
    fifa96_action_locomotion m = loco();
    m.direct_face = 2;
    m.face_x = -1;
    m.delta = 0;
    m.move_attr = 0; /* forced: word 0 <= 0x60 -> the focus arm */
    for (k = 0; k < 12; k++)
      assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
    facing_focus = m.facing;
  }
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      assert_move_attr_turn(v[i], v[j], facing_focus);
  /* FU319 §7-L3's two recorded rows, re-measured 96/96 by FU324 §5: the
   * boundary pair the (voided) straddle prediction named. */
  assert_move_attr_turn(-78, 51, facing_focus);
  assert_move_attr_turn(-78, 52, facing_focus);
  /* The >>19 contract through the real stride consumer: one fired stride
   * row (body_timer 3 > threshold 2, delta 0 -> no position advance), the
   * table filled so index (desired & 0x3F0) | attr reads 2*attr -> the
   * velocity ramp lands vel_x = vel_z = attr from rest (dv = 2a > 1). The
   * stride tuples keep attr >= 0: negative metric words would drive the
   * index negative — the native reads its table at the same wild index;
   * unclassified here (FU324 §6's clamp scope). */
  (void)loco();
  for (i = 0; i < 0x500; i++) stride_table[i] = (int16_t)(2 * (i & 0xF));
  for (i = 0; i < (int)(sizeof(st) / sizeof(st[0])); i++) {
    int16_t dx = st[i][0], dz = st[i][1];
    int32_t d = fifa96_entity_distance(dx, dz);
    int32_t word = (int16_t)(uint16_t)d;
    int32_t attr = word >> 3;
    int32_t expected = attr > 15 ? 15 : attr;
    int32_t angle = 0;
    fifa96_action_locomotion m;
    assert((staged_shape(dx, dz) >> 19) == (word >> 3));
    assert(expected >= 0);
    memset(&m, 0, sizeof m);
    m.target_x = dx;
    m.target_z = dz;
    if (dx != 0 || dz != 0)
      assert(fifa96_action_kick_angle(dx, dz, &angle) == FIFA96_OK);
    m.facing = (int16_t)angle; /* preset to desired -> stride_len = 15 */
    m.direct_face = 2;
    m.stride = 0x0F;
    m.body_timer = 3;
    m.delta = 0;
    m.move_attr = staged_shape(dx, dz);
    assert(fifa96_action_locomotion_step(&m, heading, stride_table) == FIFA96_OK);
    /* attr = min(move_attr >> 19, stride_len): the unclamped >>19 equals
     * dist>>3 exactly where the clamp allows it to be seen. */
    assert(m.vel_x == expected && m.vel_z == expected);
  }
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

/* FU-147 S1 / first-hand /FIFA96.EXE 0x7C776..0x7C7AF (FUN_0007BF20 lane
 * block, re-verified this slice): `+0x77` keeps the old lane, `+0x6D/+0x6F`
 * are the camera-minus-position words and `+0x6B` = 0x8DC68(dx, dz) =
 * fifa96_entity_distance. The companion helper 0x795B4 fill shape (same
 * dx/dz store order, first-hand FU-142 Appendix K.2). */
static void test_track_lane_producer(void) {
  int16_t lane = 0x1111, dx = 0x2222, dz = 0x3333;
  assert(fifa96_action_locomotion_track(0x100, 0x200, 0x180, 0x280, &lane, &dx, &dz) ==
         FIFA96_OK);
  assert(dx == 0x80 && dz == 0x80);
  assert(lane == (int16_t)fifa96_entity_distance(0x80, 0x80));
  /* Native reads the camera/position dwords' low words; the deltas truncate
   * to 16 bits and are sign-extended only for the metric call. */
  assert(fifa96_action_locomotion_track(0, 0, -0x40, 0x100, &lane, &dx, &dz) ==
         FIFA96_OK);
  assert(dx == -0x40 && dz == 0x100);
  assert(lane == (int16_t)fifa96_entity_distance(-0x40, 0x100));
  /* Equal points -> zero lane; the metric stays a signed-word result. */
  assert(fifa96_action_locomotion_track(0x1234, -0x2222, 0x1234, -0x2222, &lane, &dx,
                                        &dz) == FIFA96_OK);
  assert(dx == 0 && dz == 0 && lane == 0);
  /* 16-bit wrap: the native SUB is on zero-extended low words, truncated. */
  assert(fifa96_action_locomotion_track(0x10000, 0, 0x10, 0x10000, &lane, &dx, &dz) ==
         FIFA96_OK);
  assert(dx == 0x10 && dz == 0);
  assert(fifa96_action_locomotion_track(0, 0, 0, 0, NULL, &dx, &dz) == ACTION_INVALID);
  assert(fifa96_action_locomotion_track(0, 0, 0, 0, &lane, NULL, &dz) == ACTION_INVALID);
  assert(fifa96_action_locomotion_track(0, 0, 0, 0, &lane, &dx, NULL) == ACTION_INVALID);
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
  test_step_move_attr_metric_contract();
  test_step_stride_ramp();
  test_step_speed_gate();
  test_step_body_timer_boundary();
  test_step_integration();
  test_restart_target();
  test_hold();
  test_camera_lead();
  test_track_lane_producer();
  test_clamp_placement();
  puts("test_action_locomotion: all assertions passed");
  return 0;
}
