/* tests/test_arm_bodies.c — M2 arms-and-wiring Task 3 / FU-142b: the row 0x26
 * record machine (`0x866F4..0x8681C`, 90 instructions) and the shared 0x36200
 * stub.
 *
 * Evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix C (Ghidra
 * read-only /FIFA96.EXE: disassemble_bytes 0x866F4, 304 B; read_memory 0x110778
 * proving action-table row 0x26 = 0x866F4; read_memory 0x10F394 for the
 * 0x157A38 table; disassemble_bytes 0x36200). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_loader/fifa96_arm_bodies.h"
#include "fifa96_loader/fifa96_err.h"

#define ARM_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static struct fifa96_arm_record arm_rec(void) {
  struct fifa96_arm_record rec;
  memset(&rec, 0, sizeof rec);
  return rec;
}

/* 0x36200: `MOV [0x105FC4],EAX; RET` (5 bytes). The store target is the
 * FUN_00036208 camera-step gate (FU-62 §3.3); the derived engine does not model
 * that global, so the stub is a documented no-op (Appendix C.4). */
static void test_arm_stub_36200(void) {
  assert(fifa96_arm_stub_36200() == FIFA96_OK);
  assert(fifa96_arm_stub_36200() == FIFA96_OK);
}

/* stage92 > 1: the prologue still runs (timer89 += delta, timer7b table read),
 * then the record returns untouched (0x86739..0x8673D). */
static void test_arm_26_stage_past(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.timer89 = 5;
  rec.delta = 3;
  rec.player_d = 0;              /* table[0] = 6 -> timer7b = 3 */
  rec.target.x = 0x123;
  rec.target.z = 0x456;
  rec.lane = 0x789;
  assert(fifa96_arm_26_step(&rec) == FIFA96_OK);
  assert(rec.timer89 == 8);
  assert(rec.timer7b == 3);
  assert(rec.stage92 == 2);
  assert(rec.target.x == 0x123);
  assert(rec.target.z == 0x456);
  assert(rec.lane == 0x789);
}

/* stage 0 below the 15 * P[+0xE] gate: the record waits, timers still advance
 * (0x86746..0x86764). */
static void test_arm_26_stage0_waits(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 0;
  rec.timer89 = 10;
  rec.delta = 5;
  rec.player_e = 2;              /* gate = 30 */
  assert(fifa96_arm_26_step(&rec) == FIFA96_OK);
  assert(rec.timer89 == 15);
  assert(rec.stage92 == 0);
  assert(rec.timer7b == 3);
}

/* stage 0 at the gate: timer89 reset, stage92 -> 1, and the placement runs in
 * the same call (0x8676A..0x86782..). active = 0x15: sign 1, magnitude 0x15>>1
 * = 10 -> target.z = +60; pos.z = 0 -> lane 60 >= 0x20 so the latch stays 1. */
static void test_arm_26_stage0_advances_to_placement(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 0;
  rec.timer89 = 30;
  rec.delta = 0;
  rec.player_e = 2;              /* gate = 30, timer89 == gate -> advance */
  rec.active = 0x15;
  assert(fifa96_arm_26_step(&rec) == FIFA96_OK);
  assert(rec.stage92 == 1);
  assert(rec.timer89 == 0);
  assert(rec.target.x == 0x780);
  assert(rec.target.z == 60);
  assert(rec.lane == 60);
}

/* stage 1, small positive lane: target x = 0x780, z = +6 (active 3: sign 1,
 * magnitude 1); |lane| = 6 < 0x20 -> retarget (0xCC0, 0) and latch 2
 * (0x867F2..0x86812). */
static void test_arm_26_stage1_small_lane_positive(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 1;
  rec.timer89 = 7;
  rec.active = 3;
  assert(fifa96_arm_26_step(&rec) == FIFA96_OK);
  assert(rec.target.x == 0xCC0);
  assert(rec.target.z == 0);
  assert(rec.timer89 == 0);
  assert(rec.stage92 == 2);
  assert(rec.lane == 6);
  assert(rec.timer7b == 3);
}

/* active 2: sign bit clear, magnitude 1 -> target.z = -6; lane keeps the sign. */
static void test_arm_26_stage1_small_lane_negative(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 1;
  rec.active = 2;
  assert(fifa96_arm_26_step(&rec) == FIFA96_OK);
  assert(rec.target.x == 0xCC0);
  assert(rec.target.z == 0);
  assert(rec.stage92 == 2);
  assert(rec.lane == -6);
}

/* active 0xFF: (int8)-1 >> 1 = -1 -> magnitude -6*1, sign bit set -> -6. */
static void test_arm_26_stage1_active_ff(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 1;
  rec.active = 0xFF;
  assert(fifa96_arm_26_step(&rec) == FIFA96_OK);
  assert(rec.target.x == 0xCC0);
  assert(rec.target.z == 0);
  assert(rec.stage92 == 2);
  assert(rec.lane == -6);
}

/* stage 1, |lane| >= 0x20: the target stays (0x780, +60) and the latch stays 1
 * (0x867ED/0x867F0). */
static void test_arm_26_stage1_large_lane(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 1;
  rec.timer89 = 7;
  rec.active = 0x15;
  assert(fifa96_arm_26_step(&rec) == FIFA96_OK);
  assert(rec.target.x == 0x780);
  assert(rec.target.z == 60);
  assert(rec.timer89 == 7);
  assert(rec.stage92 == 1);
  assert(rec.lane == 60);
}

/* The lane gate measures target.z - pos.z, not the raw offset: pos.z = 6 with
 * active 3 gives lane 0 (already on the line) -> the latch completes. */
static void test_arm_26_lane_uses_position(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 1;
  rec.active = 3;                /* target.z = +6 */
  rec.pos.z = 6;                 /* lane = 0 */
  assert(fifa96_arm_26_step(&rec) == FIFA96_OK);
  assert(rec.lane == 0);
  assert(rec.stage92 == 2);
  assert(rec.target.x == 0xCC0);
}

/* 0x10F394[P[+0xD]] >> 1 first-hand table values (Appendix C.2). */
static void test_arm_26_timer7b_table(void) {
  static const struct { int8_t d; uint16_t want; } cases[] = {
    { 0, 3 },   /* 6 >> 1 */
    { 7, 3 },
    { 8, 3 },   /* 7 >> 1 */
    { 15, 3 },
    { 16, 7 },  /* 0x0F >> 1 */
    { 19, 7 },  /* 0x0E >> 1 */
    { 20, 6 },  /* 0x0D >> 1 */
    { 24, 5 },  /* 0x0B >> 1 */
    { 28, 4 },  /* 0x08 >> 1 */
    { 31, 3 },  /* 0x06 >> 1 */
  };
  for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
    struct fifa96_arm_record rec = arm_rec();
    rec.stage92 = 2;             /* past the latch: prologue only */
    rec.player_d = cases[i].d;
    assert(fifa96_arm_26_step(&rec) == FIFA96_OK);
    assert(rec.timer7b == cases[i].want);
  }
}

static void test_arm_26_invalid(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.player_d = 32;             /* outside the 32-byte table (hardening) */
  assert(fifa96_arm_26_step(NULL) == ARM_INVALID);
  assert(fifa96_arm_26_step(&rec) == ARM_INVALID);
  assert(rec.timer89 == 0);      /* validation precedes every write */
}

/* --- Row 0x27 body `0x86820..0x86A02` (136 instructions, Appendix D) -------
 * Fixtures below use the first-hand 0x1103CB 24-pair table read in Appendix
 * D.2 (pairs `{id, time}`; e.g. pair 0 = {0x6B, 0x168}, pair 3 = {3, 0x1E},
 * pair 4 = {0x16, -1}, pair 23 = {3, 0xF0}) and the row base 3*cycle. */

/* active 3: prologue timer89 += delta, placement target.z = +6 (sign bit 1,
 * magnitude 3>>1 = 1), |lane| = 6 < 0x20 retargets (0xCC0, 0); an active
 * record returns after the placement (0x868b7..0x868c6) without touching the
 * face/animation state. */
static void test_arm_27_active_returns_after_placement(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 1;
  rec.timer89 = 5;
  rec.delta = 7;
  rec.active = 3;
  rec.pos.x = 0x780;
  rec.pos.z = 0;
  rec.type = 0x55;
  rec.anim_cycle = 4;
  rec.anim_sel = 0x33;
  assert(fifa96_arm_27_step(&rec) == FIFA96_OK);
  assert(rec.timer89 == 12);
  assert(rec.target.x == 0xCC0);
  assert(rec.target.z == 0);
  assert(rec.lane == 6);
  assert(rec.stage92 == 1);
  assert(rec.type == 0x55);
  assert(rec.anim_cycle == 4);
  assert(rec.anim_sel == 0x33);
}

/* active 0x41: placement target.z = +0xC0 (65>>1 = 32, 6*32); |lane| = 0xC0
 * >= 0x20 keeps the (0x780, 0xC0) target; active still returns before the
 * stage machine. */
static void test_arm_27_active_large_lane_keeps_target(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 1;
  rec.timer89 = 9;
  rec.delta = 1;
  rec.active = 0x41;
  rec.pos.x = 0x780;
  rec.pos.z = 0;
  assert(fifa96_arm_27_step(&rec) == FIFA96_OK);
  assert(rec.timer89 == 10);
  assert(rec.target.x == 0x780);
  assert(rec.target.z == 0xC0);
  assert(rec.lane == 0xC0);
  assert(rec.stage92 == 1);
}

/* active 0, stage 0: timer89 reset, stage 1 runs in the same call — face with
 * the zero direction keeps +0x8E, the [0x158782] cycle wraps 0 -> 1, the row-1
 * pair 3 id 3 is selected, cursor 0, stage -> 2, and the stage-2 walk reads
 * pair 3 time 0x1E against the reset timer (no fire). */
static void test_arm_27_stage0_runs_face_and_select(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 0;
  rec.timer89 = 99;
  rec.delta = 1;
  rec.active = 0;
  rec.pos.x = 0x780;
  rec.pos.z = 0;
  rec.type = 0x55;
  rec.anim_cycle = 0;
  rec.anim_sel = 0x11;
  assert(fifa96_arm_27_step(&rec) == FIFA96_OK);
  assert(rec.timer89 == 0);
  assert(rec.stage92 == 2);
  assert(rec.anim_cycle == 1);
  assert(rec.anim_sel == 3);     /* pair 3 id */
  assert(rec.anim_cursor == 0);
  assert(rec.type == 0x55);      /* zero-direction face guard */
  assert(rec.lane == 0);
  assert(rec.target.x == 0xCC0); /* |0| < 0x20 retarget */
  assert(rec.anim_overflow == 0);
}

/* active 0, stage 1: pos.z = -100 -> lane 100, the direction (dx 0, dz 100)
 * faces +z, so the face call writes octant 0 into +0x8E (type). */
static void test_arm_27_face_writes_octant(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 1;
  rec.active = 0;
  rec.pos.x = 0x780;
  rec.pos.z = -100;
  rec.type = 0x55;
  rec.anim_cycle = 0;
  assert(fifa96_arm_27_step(&rec) == FIFA96_OK);
  assert(rec.type == 0);         /* +z octant */
  assert(rec.lane == 100);
  assert(rec.target.x == 0x780); /* |lane| >= 0x20 keeps the target */
  assert(rec.anim_sel == 3);     /* pair 3 (cycle 1) */
}

/* stage 2, cycle 1, cursor 0: pair 3 time 0x1E; timer89 31 > 30 fires, resets
 * the timer, advances the cursor and selects pair 4 id 0x16 (0x869c6..0x869f8). */
static void test_arm_27_stage2_fires_positive_time(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.active = 0;
  rec.anim_cycle = 1;
  rec.anim_cursor = 0;
  rec.timer89 = 31;
  rec.delta = 0;
  rec.anim_sel = 0x11;
  assert(fifa96_arm_27_step(&rec) == FIFA96_OK);
  assert(rec.anim_sel == 0x16);
  assert(rec.anim_cursor == 1);
  assert(rec.timer89 == 0);
  assert(rec.stage92 == 2);
  assert(rec.anim_overflow == 0);
}

/* timer89 == the threshold does not fire (the native `CMP thr,timer; JGE`
 * skips); timer89 < threshold does not fire either. */
static void test_arm_27_stage2_equal_time_no_fire(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.active = 0;
  rec.anim_cycle = 1;
  rec.timer89 = 30;
  rec.anim_sel = 0x11;
  assert(fifa96_arm_27_step(&rec) == FIFA96_OK);
  assert(rec.anim_sel == 0x11);
  assert(rec.anim_cursor == 0);
  assert(rec.timer89 == 30);
  rec.timer89 = 10;
  assert(fifa96_arm_27_step(&rec) == FIFA96_OK);
  assert(rec.anim_sel == 0x11);
  assert(rec.anim_cursor == 0);
}

/* stage 2, cursor 1: pair 4 time -1 — the negative threshold fires only when
 * the record's +0x44 row flag is set (0x869af..0x869c6); the timer is
 * ignored. */
static void test_arm_27_stage2_negative_time_gate(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.active = 0;
  rec.anim_cycle = 1;
  rec.anim_cursor = 1;
  rec.timer89 = 0x7FFF;
  rec.delta = 0;
  rec.flag44 = 0;
  rec.anim_sel = 0x11;
  assert(fifa96_arm_27_step(&rec) == FIFA96_OK);
  assert(rec.anim_sel == 0x11);
  assert(rec.anim_cursor == 1);
  assert(rec.timer89 == 0x7FFF);
  rec.flag44 = 1;
  assert(fifa96_arm_27_step(&rec) == FIFA96_OK);
  assert(rec.anim_sel == 3);     /* pair 5 id */
  assert(rec.anim_cursor == 2);
  assert(rec.timer89 == 0);
}

/* The cycle counter wraps 7 -> 0 before the row is read ([0x158782] INC then
 * `>= 8 -> 0`, 0x86921..0x86933), selecting row 0 pair 0 id 0x6B. */
static void test_arm_27_cycle_wraps_to_row0(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 1;
  rec.active = 0;
  rec.pos.x = 0x780;
  rec.pos.z = 0;
  rec.anim_cycle = 7;
  assert(fifa96_arm_27_step(&rec) == FIFA96_OK);
  assert(rec.anim_cycle == 0);
  assert(rec.anim_sel == 0x6B);
  assert(rec.anim_cursor == 0);
}

/* The walk is bounded to the 24-pair 0x1103CB table: cycle 7 cursor 2 (pair
 * 23, time 0xF0) fires and the next pair (24) leaves the table, so the derived
 * model flags anim_overflow instead of reading the adjacent 0x11042B data; an
 * in-bounds walk clears the flag. */
static void test_arm_27_pair_walk_overflow_bounded(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.active = 0;
  rec.anim_cycle = 7;
  rec.anim_cursor = 2;
  rec.timer89 = 0x100;
  rec.delta = 0;
  rec.anim_sel = 0x11;
  assert(fifa96_arm_27_step(&rec) == FIFA96_OK);
  assert(rec.anim_overflow == 1);
  assert(rec.anim_cursor == 3);
  assert(rec.anim_sel == 0x11);
  assert(rec.timer89 == 0);
  rec.anim_cursor = 0;
  rec.timer89 = 0;
  assert(fifa96_arm_27_step(&rec) == FIFA96_OK);
  assert(rec.anim_overflow == 0);
}

/* stage > 2 runs only the prologue + placement (0x868e0 JMP 0x869fd). */
static void test_arm_27_stage_past_prologue_only(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 5;
  rec.active = 0;
  rec.timer89 = 9;
  rec.delta = 2;
  rec.pos.x = 0x780;
  rec.pos.z = 0;
  rec.type = 0x55;
  rec.anim_cycle = 4;
  rec.anim_sel = 0x33;
  assert(fifa96_arm_27_step(&rec) == FIFA96_OK);
  assert(rec.timer89 == 11);
  assert(rec.target.x == 0xCC0);
  assert(rec.target.z == 0);
  assert(rec.stage92 == 5);
  assert(rec.anim_cycle == 4);
  assert(rec.anim_sel == 0x33);
  assert(rec.type == 0x55);
}

static void test_arm_27_invalid(void) {
  assert(fifa96_arm_27_step(NULL) == ARM_INVALID);
}

int main(void) {
  test_arm_stub_36200();
  test_arm_26_stage_past();
  test_arm_26_stage0_waits();
  test_arm_26_stage0_advances_to_placement();
  test_arm_26_stage1_small_lane_positive();
  test_arm_26_stage1_small_lane_negative();
  test_arm_26_stage1_active_ff();
  test_arm_26_stage1_large_lane();
  test_arm_26_lane_uses_position();
  test_arm_26_timer7b_table();
  test_arm_26_invalid();
  test_arm_27_active_returns_after_placement();
  test_arm_27_active_large_lane_keeps_target();
  test_arm_27_stage0_runs_face_and_select();
  test_arm_27_face_writes_octant();
  test_arm_27_stage2_fires_positive_time();
  test_arm_27_stage2_equal_time_no_fire();
  test_arm_27_stage2_negative_time_gate();
  test_arm_27_cycle_wraps_to_row0();
  test_arm_27_pair_walk_overflow_bounded();
  test_arm_27_stage_past_prologue_only();
  test_arm_27_invalid();
  puts("test_arm_bodies OK");
  return 0;
}
