// tests/test_keeper_machines.c — FU-151 P3: the row-1E ten-stage claim machine
// and the row-1D five-stage close-down machine (docs/ghidra/FU151_keeper_ai.md
// §2.3..§2.6/§Port contract items 1-2; first-hand /FIFA96.EXE).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_keeper.h"

static fifa96_keeper_claim claim_state(void) {
  fifa96_keeper_claim s;
  memset(&s, 0, sizeof s);
  s.stage92 = 0;
  s.pos.x = 100;
  s.pos.y = 20;
  s.pos.z = 200;
  s.offset_x = 3;
  s.offset_z = 4;
  return s;
}

/* Head + stage 0 -> 1 fall-through: the claim triple, the saved
 * point/gauge/latch seeds, the camera z offset and the 0x27 event; the exit
 * updates the saved point with a zero distance (no movement). */
static void test_claim_head_stage0_1(void) {
  fifa96_keeper_claim s = claim_state();
  fifa96_keeper_claim_out out;
  s.row44 = 1;
  s.delta = 5;
  s.side = 0;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(out.claimed == 1 && out.controlled == 1 && out.helper == 1);
  assert(out.place == 1);
  assert(out.place_x == 148 && out.place_y == 76 && out.place_z == 264);
  assert(s.has_ball == 1);
  /* stage 0 then falls into stage 1: timer 5 -> 0 at the stage-0 tail, the
   * stage-1 camera write (pos + 0x20 by side), face and event. */
  assert(s.stage92 == 1);
  assert(s.timer89 == 0);
  assert(s.timer7b == 3);
  assert(s.saved_x == 100 && s.saved_z == 200);
  assert(s.gauge == 0);
  assert(s.latch_157ab2 == 1);
  assert(s.cam_x == 100 && s.cam_y == 20 && s.cam_z == 232);
  assert(s.sector == 5);              /* face(-100,-200): angle -0x1C0 */
  assert(out.event == 0x27);
  assert((out.ui & 0x400u) != 0u);    /* 0x760df latch camera call */

  /* row44 == 0 at stage 0 takes the plain epilogue: no advance. */
  s = claim_state();
  s.row44 = 0;
  s.delta = 5;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(s.stage92 == 0 && s.timer89 == 5 && s.gauge == 0);
  assert(out.claimed == 1);           /* the head runs before the stage */
  assert(out.event == 0);
}

/* Stage 1 with a slot jumps straight to stage 3 without an RNG draw. */
static void test_claim_stage1_slot_to_stage3(void) {
  fifa96_keeper_claim s = claim_state();
  fifa96_keeper_claim_out out;
  struct fifa96_rng rng;
  struct fifa96_rng untouched;
  s.stage92 = 1;
  s.timer89 = 0x40;
  s.has_ball = 1;
  s.has_slot = 1;
  s.slot_edge = 0;
  s.latch_157ab2 = 1;                 /* the stage-3 hold-follow ladder arm */
  s.rng = &rng;
  assert(fifa96_rng_seed(&rng, 0) == FIFA96_OK);
  untouched = rng;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(memcmp(&rng, &untouched, sizeof rng) == 0);   /* no draw */
  assert(s.stage92 == 3);             /* stage 3 ran in the same dispatch */
  assert(out.ran == 1);               /* timer89 0x40 > 2 */
  assert(out.event == 0x27);          /* stage 1 event before the stage-3 run */
  assert(s.timer7b == 2);             /* the stage-3 override */
  /* stage 3 with has_ball: the hold-follow overwrites 0x15774C with pos
   * (slot dir 0) and calls 0x700F4; then the ball exit at 0x760df. */
  assert(out.place == 1);
  assert(s.cam_x == 100 && s.cam_y == 0x38 && s.cam_z == 216);
  assert(out.guard == 1);
  assert(out.held_exit == 1);
  assert(out.install == 0 && out.reset == 0 && out.situation_0b == 0);
}

/* Stage 1 with no slot draws exactly one RNG word; the derived branch matches
 * the native bit (0 -> straight to stage 3 with event 0x27, 1 -> the 0x32
 * face + stage 2). Both arms are pinned: fifa96_rng_seed(0) yields first
 * draw 0x200 (bit 0); seed 32 yields 0x4201 (bit 1). */
static void test_claim_stage1_rng_branch(void) {
  fifa96_keeper_claim s;
  fifa96_keeper_claim_out out;
  struct fifa96_rng rng;
  uint16_t value = 0;

  /* bit 0 arm (seed 0). */
  s = claim_state();
  s.stage92 = 1;
  s.timer89 = 0x40;
  s.has_ball = 0;
  s.has_slot = 0;
  s.row44 = 0;
  s.rng = &rng;
  assert(fifa96_rng_seed(&rng, 0) == FIFA96_OK);
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(s.has_ball == 1);            /* the head claim ran (stage < 3) */
  assert(out.event == 0x27);          /* no reroll */
  assert(s.stage92 == 3);             /* straight to stage 3, then the hold exit */

  /* bit 1 arm (seed 32): the 0x32 reroll face + stage 2 (row44 1 advances). */
  s = claim_state();
  s.stage92 = 1;
  s.timer89 = 0x40;
  s.has_ball = 0;
  s.has_slot = 0;
  s.row44 = 1;
  s.rng = &rng;
  assert(fifa96_rng_seed(&rng, 32) == FIFA96_OK);
  assert(fifa96_rng_step(&rng, &value) == FIFA96_OK);
  assert((value & 1) == 1);           /* the pinned seed's first bit */
  assert(fifa96_rng_seed(&rng, 32) == FIFA96_OK);
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(out.event == 0x32);
  assert(s.stage92 == 3);             /* 2 -> 3 through the row44 gate */
}

/* Stage 3 no-ball tail (`0x75B26..0x75B74`): reset + situation 0xB + install
 * code 5, with the gauge < 0x90 target/camera nudges. No exit-gauge run. */
static void test_claim_stage3_restart(void) {
  fifa96_keeper_claim s = claim_state();
  fifa96_keeper_claim_out out;
  s.stage92 = 3;
  s.timer89 = 5;
  s.gauge = 0x80;
  s.latch_157ab2 = 1;
  s.side = 0;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(out.helper == 1);            /* stage < 6, no slot */
  assert(out.slot_fill == 1);         /* no slot: 0x744d4 */
  assert(out.reset == 1);
  assert(out.situation_0b == 1);
  assert(out.install == 5);
  assert(out.held_exit == 0);
  assert(s.has_ball == 0);
  assert(s.stage92 == 3);
  assert(s.timer7b == 2);
  assert(s.target_x == 100 && s.target_z == 200 + 0x10);   /* gauge<0x90 nudge */
  assert(s.cam_x == 100 && s.cam_y == 0x38 && s.cam_z == 200 + 0x10);
  assert(s.gauge == 0x80);            /* the restart RET skips the exit gauge */
}

/* Stage 3 branch topology (`0x757BE`): slot == 0 skips the whole 0x758CD
 * block (no hold-follow, no latch ladder) and enters the 0x75992 ladder
 * regardless of +0x9B; slot != 0 runs the latch ladder and, with +0x9B == 0,
 * jumps straight to 0x759EA with no timer/gauge ladder. */
static void test_claim_stage3_slot_paths(void) {
  fifa96_keeper_claim s;
  fifa96_keeper_claim_out out;

  /* slotless, timer89 > 0x78: [EBP-8]=1 -> stage 4 (no place, no +0x200). */
  s = claim_state();
  s.stage92 = 3;
  s.timer89 = 0x79;
  s.has_ball = 1;
  s.has_slot = 0;
  s.latch_157ab2 = 1;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(out.clear_vec == 1);         /* stage 4 ran */
  assert(out.place == 0);             /* the hold-follow is slot-only */
  assert((out.ui & 0x200u) == 0u);    /* 0x4C31C belongs to the slot path */
  assert(out.held_exit == 0);
  assert(s.stage92 == 8);             /* 3 -> 4 -> ... -> 8 (row44 0 exit) */

  /* slotless, timer89 <= 0x78 and gauge < 0x90: the target.z nudge, no place. */
  s = claim_state();
  s.stage92 = 3;
  s.timer89 = 0x10;
  s.gauge = 0x80;
  s.has_ball = 1;
  s.has_slot = 0;
  s.latch_157ab2 = 1;
  s.side = 0;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(s.target_x == 100 && s.target_z == 200 + 0x10);
  assert(out.place == 0);
  assert(out.held_exit == 1);         /* still holding at 0x75B29 */
  assert(s.cam_x == 100 && s.cam_y == 0x38 && s.cam_z == 200 + 0x10);

  /* slot, latch set, no ball: straight to 0x759EA (no timer/gauge ladder);
   * timer89 0x79 > 0x78 must NOT advance to stage 4. */
  s = claim_state();
  s.stage92 = 3;
  s.timer89 = 0x79;
  s.gauge = 0x80;
  s.has_ball = 0;
  s.has_slot = 1;
  s.latch_157ab2 = 1;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.situation_0b == 1 && out.install == 5);
  assert(s.stage92 == 3);             /* the restart tail RETs */
  assert(s.target_x == 100 && s.target_z == 200);   /* no gauge nudge at all */
}

/* Stage 5 release (`0x75D17`): row 0x45 frame >= 3 clears +0x9B and falls
 * through 6/7/8/9 to the common exit. */
static void test_claim_release_chain(void) {
  fifa96_keeper_claim s = claim_state();
  fifa96_keeper_claim_out out;
  s.stage92 = 5;
  s.has_ball = 1;
  s.anim_row = 0x45;
  s.frame = 3;
  s.row44 = 1;
  s.offset_x = 1;
  s.offset_z = 2;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(out.released == 1);
  assert(s.has_ball == 0);
  assert(out.controlled == 1 && out.scenario == 1);
  assert(s.cam_x == 100 + (1 << 6) && s.cam_y == 0x50 && s.cam_z == 200 + (2 << 6));
  /* stage 6 wrote the 0x45 row gate (3) and cleared both flags; frame 3 is
   * not < 3, so it advanced. */
  assert(out.flag_write == 1 && out.flag_157820 == 0 && out.flag_157822 == 0);
  /* stage 7: the else arm stages event 1 via 0x7A490 and runs situation 0xB. */
  assert(out.ball_stage == 1 && out.ball_event == 0x01);
  assert(out.situation_0b == 1);
  assert(s.latch_157ab2 == 0);
  assert(s.stage92 == 9);
  assert(out.snap == 1);
  assert(out.reset == 0);             /* timer89 0 <= 0x3c at stage 9 */

  /* frame 2 at stage 5 row 0x45 exits through 0x760df without advancing. */
  s = claim_state();
  s.stage92 = 5;
  s.has_ball = 1;
  s.anim_row = 0x45;
  s.frame = 2;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(s.stage92 == 5 && s.has_ball == 1);
  assert(out.released == 0 && out.snap == 0);

  /* stage 6 row 0x44 needs frame >= 5; frame 4 exits. */
  s = claim_state();
  s.stage92 = 6;
  s.anim_row = 0x44;
  s.frame = 4;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(s.stage92 == 6);
  assert(out.flag_157820 == 1 && out.flag_157822 == 1);

  /* stage 7 row 0x44 subtracts the `0x8DC50` trunc-toward-zero quarter from
   * both vector words (a non-multiple pins the truncation). */
  s = claim_state();
  s.stage92 = 7;
  s.anim_row = 0x44;
  s.frame = 0;
  s.row44 = 0;
  s.vec_dx = -1001;
  s.vec_dz = 1003;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(s.vec_dx == -751);           /* -1001 - trunc(-1001/4 = -250) */
  assert(s.vec_dz == 753);            /* 1003 - 250 */
  assert(s.stage92 == 8);             /* falls to 8, exit on row44 0 */
}

/* Stage 4 outlet (`0x75B90`): the residual vector selects the event by band
 * (0x44 / 0x2F / 0x45) and the slot arm requests the 0x7B878 vector build. */
static void test_claim_stage4_outlet(void) {
  fifa96_keeper_claim s = claim_state();
  fifa96_keeper_claim_out out;
  s.stage92 = 4;
  s.has_slot = 1;
  s.latch_157ab2 = 1;
  s.reset_x = 500;
  s.reset_z = 200;
  s.vec_band = 0;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(out.vector_build == 1);      /* latch set -> 0x7B878 (leg) */
  assert(out.clear_vec == 0);
  assert(out.event == 0x44);          /* band 0 < 0x5a0 */
  /* stage 4 falls through 5/6/7; row44 0 exits at stage 8. */
  assert(s.stage92 == 8);
  assert(out.ball_stage == 1 && out.ball_event == 0x01);
  assert(out.situation_0b == 1);

  /* no slot: the clear-vector request, band < 0x5a0 -> event 0x44. */
  s = claim_state();
  s.stage92 = 4;
  s.has_slot = 0;
  s.latch_157ab2 = 0;
  s.reset_x = 500;
  s.reset_z = 200;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(out.clear_vec == 1);
  assert(out.event == 0x44);
  assert(s.stage92 == 8);

  /* slot + latch clear: 0x8DCD4(pos, reset) with band >= 0x780 -> event 0x45. */
  s = claim_state();
  s.stage92 = 4;
  s.has_slot = 1;
  s.latch_157ab2 = 0;
  s.pos.x = 0;
  s.pos.z = 0;
  s.reset_x = 0x800;
  s.reset_z = 0;
  assert(fifa96_keeper_claim_step(&s, &out) == FIFA96_OK);
  assert(out.event == 0x45);
  assert((uint16_t)s.vec_band == 0x800);
}

static void test_claim_errors(void) {
  fifa96_keeper_claim s = claim_state();
  fifa96_keeper_claim_out out;
  assert((int)fifa96_keeper_claim_step(NULL, &out) == -FIFA96_ERR_INVALID);
  assert((int)fifa96_keeper_claim_step(&s, NULL) == -FIFA96_ERR_INVALID);
}

static fifa96_keeper_closedown closedown_state(void) {
  fifa96_keeper_closedown s;
  memset(&s, 0, sizeof s);
  s.pos.x = 100;
  s.pos.z = 200;
  s.lane = 0;
  return s;
}

/* Stage 0 -> 1: the place/commit with the per-side z offset, event 0x26, the
 * latch, and stage 1's place/commit + 0x79B6C tail when no slot is bound. */
static void test_closedown_stage0_1(void) {
  fifa96_keeper_closedown s = closedown_state();
  fifa96_keeper_closedown_out out;
  s.row44 = 1;
  s.timer89 = 0x1E;
  s.cam_x = 10;
  s.cam_y = 20;
  s.cam_z = 30;
  s.cam_off_z = 0x40;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(out.controlled == 1 && out.helper == 1);
  assert(out.place == 1 && out.place_x == 10 && out.place_y == 20 && out.place_z == 30);
  assert(out.commit == 1);
  assert(out.event == 0x26);
  assert(s.latch_157ab2 == 1);
  assert(s.pos.x == 10 && s.pos.y == 20 && s.pos.z == 30 + 0x40);
  assert(s.stage92 == 1);
  assert(out.slot_fill == 1);         /* stage 1, no slot */
  assert(out.tail_hold == 1);         /* latch set, stage 1 > 0 */

  /* row44 == 0 leaves stage 0 untouched. */
  s = closedown_state();
  s.row44 = 0;
  s.timer89 = 0x1E;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(s.stage92 == 0 && out.place == 0 && out.controlled == 1);

  /* the session gate admits the place below timer 0x1e. */
  s = closedown_state();
  s.row44 = 1;
  s.timer89 = 0;
  s.session_gate = 1;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(out.place == 1 && s.stage92 == 1);
}

/* Stage 1 branch topology: the slotless arm advances on timer89 > 0xB4
 * (`0x75216`) and never runs the slot-only 0x4B0/0x4C31C block; the latch
 * toggle (`0x75137`) reads byte[slot+4] (pressed), not byte[slot+6]. */
static void test_closedown_stage1_slot_paths(void) {
  fifa96_keeper_closedown s;
  fifa96_keeper_closedown_out out;

  /* slotless, timer89 > 0xB4 -> flag4 -> stage 2 (native 0x75216). */
  s = closedown_state();
  s.stage92 = 1;
  s.timer89 = 0xC0;
  s.row44 = 0;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(out.slot_fill == 1);
  assert(s.stage92 == 4);             /* 1 -> 2 -> 3 -> 4 then the tail */
  assert(out.sink_4b0 == 0);          /* slot-only block */
  assert((out.ui & 0x100u) == 0u);    /* slot-only 0x4C31C(0) */
  assert((out.ui & 0x200u) == 0u);    /* slot-only 0x4C31C(1) */

  /* slotless, the 0x4B0 boundary must not produce the slot-only sink. */
  s = closedown_state();
  s.stage92 = 1;
  s.timer89 = 0x4B1;
  s.delta = 2;
  s.row44 = 0;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(out.sink_4b0 == 0);
  assert((out.ui & 0x100u) == 0u);

  /* slot + pressed byte[slot+4] bit 0x20: the latch toggles (0 -> 1). */
  s = closedown_state();
  s.stage92 = 1;
  s.timer89 = 0x10;
  s.has_slot = 1;
  s.slot_pressed = 0x20;
  s.slot_edge = 0;
  s.latch_157ab2 = 0;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(s.latch_157ab2 == 1);
  assert((out.ui & 0x40u) != 0u);     /* the latch-set call block */
  assert((out.ui & 0x100u) != 0u);    /* 0x4B9..0x4C31C(0) after the edge */
  assert((out.ui & 0x200u) == 0u);    /* latch 1 -> no 0x4C31C(1) */
  assert(s.stage92 == 1);             /* flag4 stays clear */

  /* slot + released byte[slot+6] bit 0x20 alone must NOT toggle the latch
   * (the pre-fix read); the latch-0 slot path requests 0x4C31C(1). */
  s = closedown_state();
  s.stage92 = 1;
  s.timer89 = 0x10;
  s.has_slot = 1;
  s.slot_pressed = 0;
  s.slot_edge = 0x20;
  s.latch_157ab2 = 0;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(s.latch_157ab2 == 0);
  assert((out.ui & 0x200u) != 0u);
  assert((out.ui & 0x100u) == 0u);
}

/* Stage 2: lane > 0x40 with timer89 > 0xB4 resets; else it advances. */
static void test_closedown_stage2_reset_and_advance(void) {
  fifa96_keeper_closedown s = closedown_state();
  fifa96_keeper_closedown_out out;
  s.stage92 = 2;
  s.lane = 0x41;
  s.timer89 = 0xC0;
  s.latch_157ab2 = 1;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(out.reset == 1);
  assert(s.latch_157ab2 == 0);
  assert(s.stage92 == 2);             /* the tail does not advance */
  assert(out.tail_hold == 0);         /* latch cleared before the tail */
  assert(s.target_x == s.cam_x && s.target_z == s.cam_z);

  s = closedown_state();
  s.stage92 = 2;
  s.lane = 0x41;
  s.timer89 = 0xB0;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(out.reset == 0);
  assert(s.stage92 == 4);             /* falls into the stage-3 clearance */
  assert(out.ball_stage == 1);

  s = closedown_state();
  s.stage92 = 2;
  s.lane = 0x40;                      /* lane <= 0x40 always advances */
  s.timer89 = 0xC0;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(s.stage92 == 4);
}

/* Stage 3 clearance: slot -> 0x8DCD4(pos, reset); a band < 0x3C0 rotates to
 * the 0x3C0 kick vector (event 0x31 low + ring 0x22); the restart tail. */
static void test_closedown_stage3_clearance(void) {
  fifa96_keeper_closedown s = closedown_state();
  fifa96_keeper_closedown_out out;
  s.stage92 = 3;
  s.has_slot = 1;
  s.pos.x = 100;
  s.pos.z = 0;
  s.reset_x = 0;
  s.reset_z = 0;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(s.vec.distance == 0x3C0);
  assert(s.vec.dx == -0x3C0 && s.vec.dz == 0);
  assert(out.ball_stage == 1 && out.clearance_event == 0x30);
  assert(out.ring == 0);              /* the ring is on the low arm only */
  assert(out.situation_0b == 1);
  assert(s.latch_157ab2 == 0);
  assert(s.stage92 == 4);
  assert(out.snap == 1);
  assert(out.tail_hold == 0);         /* latch cleared at the restart tail */
  assert(out.helper == 0);            /* stage 3 >= 3 */

  /* a clear-vector arm (no slot) and a band >= 0x3C0 low arm. */
  s = closedown_state();
  s.stage92 = 3;
  s.has_slot = 0;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(out.clear_vec == 1);
  assert(s.vec.distance == 0x3C0);    /* zeroed vector -> the 0x3C0 rotation */
  assert(s.vec.dx == 679 && s.vec.dz == 679);       /* angle(0,0) = 0x80 (45 deg) */
  assert(out.clearance_event == 0x30);
  assert(out.ring == 0);

  /* stage 4 row44 == 1 resets; row44 == 0 only snaps. */
  s = closedown_state();
  s.stage92 = 4;
  s.row44 = 1;
  s.latch_157ab2 = 1;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(out.snap == 1 && out.reset == 1);
  assert(s.latch_157ab2 == 0);
  assert(out.tail_hold == 0);

  s = closedown_state();
  s.stage92 = 4;
  s.row44 = 0;
  s.latch_157ab2 = 1;
  assert(fifa96_keeper_closedown_step(&s, &out) == FIFA96_OK);
  assert(out.snap == 1 && out.reset == 0);
  assert(out.tail_hold == 1);
}

static void test_closedown_errors(void) {
  fifa96_keeper_closedown s = closedown_state();
  fifa96_keeper_closedown_out out;
  assert((int)fifa96_keeper_closedown_step(NULL, &out) == -FIFA96_ERR_INVALID);
  assert((int)fifa96_keeper_closedown_step(&s, NULL) == -FIFA96_ERR_INVALID);
}

int main(void) {
  test_claim_head_stage0_1();
  test_claim_stage1_slot_to_stage3();
  test_claim_stage1_rng_branch();
  test_claim_stage3_restart();
  test_claim_stage3_slot_paths();
  test_claim_release_chain();
  test_claim_stage4_outlet();
  test_claim_errors();
  test_closedown_stage0_1();
  test_closedown_stage1_slot_paths();
  test_closedown_stage2_reset_and_advance();
  test_closedown_stage3_clearance();
  test_closedown_errors();
  printf("test_keeper_machines: all tests passed\n");
  return 0;
}
