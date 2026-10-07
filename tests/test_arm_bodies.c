/* tests/test_arm_bodies.c — M2 arms-and-wiring Tasks 3/4/5/6 / FU-142b/c: the
 * cluster-G row bodies (`0x866F4` row 26, `0x86820` row 27, `0x84598` row 2C,
 * `0x874E4` row 29) and the shared 0x36200 stub.
 *
 * Evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix C (Ghidra
 * read-only /FIFA96.EXE: disassemble_bytes 0x866F4, 304 B; read_memory 0x110778
 * proving action-table row 0x26 = 0x866F4; read_memory 0x10F394 for the
 * 0x157A38 table; disassemble_bytes 0x36200), Appendix D (row 27), Appendix E
 * (row 2C) and Appendix F (row 29: disassemble_bytes 0x874E4, 596 B;
 * read_memory 0x110784 proving action-table row 0x29 = 0x874E4; the 0x8DE8C /
 * 0x6E1D0 helper windows). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_loader/fifa96_arm_bodies.h"
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_entity_update.h"
#include "fifa96_loader/fifa96_rng.h"

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

/* --- Row 0x2C body `0x84598..0x8462D` (48 instructions, Appendix E) --------
 * Stage latch: 0 with active==0 -> reset immediately (0x845B7..0x845BE ->
 * 0x84622); 0 with active!=0 -> timer89=0, stage 1, then the stage-1 select
 * and the stage-2 countdown run in the same call (0x845C0..0x84620); 1 ->
 * select + countdown; 2 -> countdown; >= 3 -> return (0x845AE). The select is
 * `0x6E598` with the constant id 0x5D (`EDX=0x5D`, `EBX=0`; the `ECX` load is
 * dead — FU-84 §1 says ECX is not an input) resolved by
 * `fifa96_arm_anim_select` into `anim_sel`. The countdown subtracts the
 * zero-extended `[0x157A64]` word (`MOV CX` after `XOR ECX,ECX`); a result not
 * strictly positive calls `FUN_0007DAB4` (`fifa96_arm_reset`: stage92=0xFF,
 * timer89=0, code=0). */

/* stage 0 with active 0 jumps straight to the reset; the anim select is
 * skipped (anim_sel untouched). */
static void test_arm_2c_stage0_inactive_resets(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 0;
  rec.active = 0;
  rec.timer89 = 0x55;
  rec.code = 0x2C;
  rec.anim_sel = 0x11;
  assert(fifa96_arm_2c_step(&rec) == FIFA96_OK);
  assert(rec.stage92 == 0xFF);
  assert(rec.timer89 == 0);
  assert(rec.code == 0);
  assert(rec.anim_sel == 0x11);
}

/* stage 0 with active != 0 runs the constant-id select in the same call
 * (anim_sel = 0x5D) and the countdown then drives the reset. */
static void test_arm_2c_stage0_active_selects_then_resets(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 0;
  rec.active = 1;
  rec.timer89 = 0x55;
  rec.delta = 3;
  rec.code = 0x2C;
  rec.anim_sel = 0x11;
  assert(fifa96_arm_2c_step(&rec) == FIFA96_OK);
  assert(rec.anim_sel == 0x5D);
  assert(rec.stage92 == 0xFF);
  assert(rec.timer89 == 0);
  assert(rec.code == 0);
}

/* stage 1 selects 0x5D, zeroes timer89 and advances to 2; the same call then
 * counts that zero down into the reset. */
static void test_arm_2c_stage1_selects_then_resets(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 1;
  rec.active = 0;
  rec.delta = 1;
  rec.code = 0x2C;
  rec.anim_sel = 0x11;
  assert(fifa96_arm_2c_step(&rec) == FIFA96_OK);
  assert(rec.anim_sel == 0x5D);
  assert(rec.stage92 == 0xFF);
  assert(rec.timer89 == 0);
  assert(rec.code == 0);
}

/* stage 2 above the delta: timer89 -= delta stays positive, so the row waits
 * (stage stays 2, no select, no reset). */
static void test_arm_2c_stage2_above_delta_waits(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.timer89 = 0x100;
  rec.delta = 1;
  rec.code = 0x2C;
  rec.anim_sel = 0x11;
  assert(fifa96_arm_2c_step(&rec) == FIFA96_OK);
  assert(rec.timer89 == 0xFF);
  assert(rec.stage92 == 2);
  assert(rec.code == 0x2C);
  assert(rec.anim_sel == 0x11);
}

/* The countdown return is `> 0` (JG): exactly delta reaches 0 and resets. */
static void test_arm_2c_stage2_equal_delta_resets(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.timer89 = 5;
  rec.delta = 5;
  rec.code = 0x2C;
  assert(fifa96_arm_2c_step(&rec) == FIFA96_OK);
  assert(rec.stage92 == 0xFF);
  assert(rec.timer89 == 0);
  assert(rec.code == 0);
}

/* `[0x157A64]` is a zero-extended word (`XOR ECX,ECX; MOV CX,...`): delta
 * 0x8000 subtracts +0x8000, so timer89 0x4000 goes negative and resets (a
 * sign-extended model would keep 0xC000 and wait); timer89 0x10000 with delta
 * 0xFFFF wraps to 1 and waits. */
static void test_arm_2c_delta_zero_extended(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.timer89 = 0x4000;
  rec.delta = 0x8000;
  rec.code = 0x2C;
  assert(fifa96_arm_2c_step(&rec) == FIFA96_OK);
  assert(rec.stage92 == 0xFF);
  rec = arm_rec();
  rec.stage92 = 2;
  rec.timer89 = 0x10000;
  rec.delta = 0xFFFF;
  rec.code = 0x2C;
  assert(fifa96_arm_2c_step(&rec) == FIFA96_OK);
  assert(rec.timer89 == 1);
  assert(rec.stage92 == 2);
  assert(rec.code == 0x2C);
}

/* delta 0 subtracts nothing and the reset still runs (the countdown's `JG`
 * needs strictly positive). */
static void test_arm_2c_delta_zero_resets(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.timer89 = 0;
  rec.delta = 0;
  rec.code = 0x2C;
  assert(fifa96_arm_2c_step(&rec) == FIFA96_OK);
  assert(rec.stage92 == 0xFF);
  assert(rec.timer89 == 0);
  assert(rec.code == 0);
}

/* stage >= 3 returns at the prologue pop (0x845AE): every field, timer89
 * included, is untouched. 0xFF is the reset sentinel and follows the same
 * path. */
static void test_arm_2c_stage_past_untouched(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 3;
  rec.timer89 = 0x1234;
  rec.delta = 9;
  rec.code = 0x2C;
  rec.anim_sel = 0x11;
  assert(fifa96_arm_2c_step(&rec) == FIFA96_OK);
  assert(rec.stage92 == 3);
  assert(rec.timer89 == 0x1234);
  assert(rec.code == 0x2C);
  assert(rec.anim_sel == 0x11);
  rec.stage92 = 0xFF;
  rec.delta = 0;
  assert(fifa96_arm_2c_step(&rec) == FIFA96_OK);
  assert(rec.timer89 == 0x1234);
  assert(rec.code == 0x2C);
}

static void test_arm_2c_invalid(void) {
  assert(fifa96_arm_2c_step(NULL) == ARM_INVALID);
}

/* --- Row 0x29 body `0x874E4..0x87738` (187 instructions, Appendix F) -------
 * The body's phase gate: `[0x157A4A]>>24` (the record's `phase` field) must be
 * 5 for the stage machine; otherwise the record syncs target=pos, zeroes the
 * velocity, runs the `fifa96_arm_reset` subset and requests install code 3
 * when `+0x9A == 0` (0x87524 pre-check). In phase 5 the stage latch runs:
 *   stage 0: the 0x8DE8C nearest search over `[rec+0]`'s 11 records (skip
 *     index = `byte[[0x157A9F]+0x8D]` = `ball_skip`, exclusions +0x98/+0x9A,
 *     target = the `[0x157A9F]` ball position at +0x59/+0x61). When the
 *     nearest record is this one (`team_index`): `[0x157AA3] = [rec+0]`
 *     (unmodeled, OL-55), `0x36200(EAX=2)` (no-op stub, OL-51), target=pos,
 *     vel=0, `[0x10F36C] = rec` (the derived per-record `chase` bit), one
 *     0x92AC8 RNG draw (`& 1` -> 0x5D else 0x46) through
 *     `fifa96_arm_anim_select`. Either way timer89=0, stage92=1, then the
 *     stage-1 block runs in the same call (0x87608 falls through to 0x87620).
 *   stage 1: if `chase`, sync and return (flag44 != 0 first re-selects); else
 *     the `((P[+0xE] << 4) - P[+0xE]) << 3 >> 4` gate; below it `active != 0`
 *     runs the 0x6E1D0 phase cell on the `[rec+8]` descriptor pair selected by
 *     `side == side_controlled` (negated for side != 0) into the target, while
 *     `active == 0` syncs target=pos; then the 0x8DCD4 distance gate (> 0x20
 *     returns), the chase re-check, then timer89=0, stage92=2.
 *   stage 2: sync target=pos, zero velocity.
 * The entry probes (Appendix F.3) find no static installer of 0x29, so the row
 * stays unwired (OL-48); the body is exercised directly here. */

static void test_arm_29_phase_not5_resets_and_requests_install(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  memset(&rng, 0, sizeof rng);
  rec.phase = 4;
  rec.rng = &rng;                    /* not drawn on this path */
  rec.pos.x = 1; rec.pos.y = 2; rec.pos.z = 3;
  rec.target.x = 9; rec.target.y = 9; rec.target.z = 9;
  rec.vel_x = 5; rec.vel_z = 6;
  rec.code = 0x2C;
  rec.stage92 = 2;
  rec.timer89 = 0x1234;
  rec.skip_9a = 0;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.timer7b == 2);          /* 0x874EF */
  assert(rec.target.x == 1 && rec.target.y == 2 && rec.target.z == 3);
  assert(rec.vel_x == 0 && rec.vel_z == 0);
  assert(rec.stage92 == 0xFF);       /* fifa96_arm_reset */
  assert(rec.timer89 == 0);
  assert(rec.code == 0);             /* the reset's code-0 re-install */
  assert(rec.install == 3);          /* 0x87531..0x8753C */
}

/* The reset path does not need the phase-5 inputs: no rng/candidates. */
static void test_arm_29_phase_not5_ignores_null_pointers(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.phase = 4;
  rec.rng = NULL;
  rec.team_candidates = NULL;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.stage92 == 0xFF);
  assert(rec.install == 3);
}

/* `CMP byte [rec+0x9A],0 / JNZ` (0x87524..0x8752B): an occupied record still
 * resets but the code-3 install is not requested. */
static void test_arm_29_phase_not5_occupied_skips_install(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.phase = 4;
  rec.skip_9a = 1;
  rec.code = 0x2C;
  rec.stage92 = 2;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.stage92 == 0xFF);
  assert(rec.code == 0);
  assert(rec.install == 0);
}

/* phase 5 stage 0, nearest == this record: the chase bit, the RNG odd id
 * (0x5D), the stage-1 chase return (flag44 == 0) in the same call. */
static void test_arm_29_stage0_nearest_self_rng_odd(void) {
  struct fifa96_arm_record rec = arm_rec();
  fifa96_entity_candidate cand[11];
  struct fifa96_rng rng;
  memset(cand, 0, sizeof cand);
  memset(&rng, 0, sizeof rng);
  cand[2].x = 100; cand[2].y = 100;          /* self at the ball */
  cand[5].x = 1000; cand[5].y = 0;
  rng.w[0] = 1;                              /* step result w[0] = 1 (odd) */
  rec.phase = 5;
  rec.rng = &rng;
  rec.team_candidates = cand;
  rec.team_index = 2;
  rec.ball_pos.x = 100; rec.ball_pos.z = 100;
  rec.ball_skip = 0;
  rec.stage92 = 0;
  rec.timer89 = 7;
  rec.delta = 0;
  rec.pos.x = 0x11; rec.pos.y = 0x22; rec.pos.z = 0x33;
  rec.target.x = 1; rec.target.y = 1; rec.target.z = 1;
  rec.vel_x = 2; rec.vel_z = 3;
  rec.anim_sel = 0x10;
  rec.flag44 = 0;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.chase == 1);
  assert(rec.anim_sel == 0x5D);
  assert(rec.stage92 == 1);                  /* stage-1 chase return */
  assert(rec.timer89 == 0);
  assert(rec.target.x == 0x11 && rec.target.z == 0x33);
  assert(rec.vel_x == 0 && rec.vel_z == 0);
  assert(rec.install == 0);
}

/* The even RNG draw picks 0x46; flag44 != 0 makes the stage-1 chase branch
 * re-run the selector (identity for an in-domain id). */
static void test_arm_29_stage0_nearest_self_rng_even_reselect(void) {
  struct fifa96_arm_record rec = arm_rec();
  fifa96_entity_candidate cand[11];
  struct fifa96_rng rng;
  memset(cand, 0, sizeof cand);
  memset(&rng, 0, sizeof rng);
  cand[3].x = 10; cand[3].y = 10;
  rng.w[0] = 2;                              /* step result w[0] = 2 (even) */
  rec.phase = 5;
  rec.rng = &rng;
  rec.team_candidates = cand;
  rec.team_index = 3;
  rec.ball_pos.x = 10; rec.ball_pos.z = 10;
  rec.stage92 = 0;
  rec.delta = 0;
  rec.anim_sel = 0x10;
  rec.flag44 = 1;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.chase == 1);
  assert(rec.anim_sel == 0x46);
  assert(rec.stage92 == 1);
}

/* The nearest record is another record: no chase, no RNG/anim; stage 1 runs in
 * the same call and the `player_e = 1` gate (15*8>>4 = 7 > timer89 0) syncs
 * target=pos (0x87714). */
static void test_arm_29_stage0_nearest_other_gate_waits(void) {
  struct fifa96_arm_record rec = arm_rec();
  fifa96_entity_candidate cand[11];
  struct fifa96_rng rng;
  memset(cand, 0, sizeof cand);
  memset(&rng, 0, sizeof rng);
  for (int i = 0; i < 11; i++) { cand[i].x = 1000; cand[i].y = 0; }
  cand[3].x = 1; cand[3].y = 0;              /* nearest, but not this record */
  rec.phase = 5;
  rec.rng = &rng;
  rec.team_candidates = cand;
  rec.team_index = 2;
  rec.ball_pos.x = 0; rec.ball_pos.z = 0;
  rec.stage92 = 0;
  rec.delta = 0;
  rec.player_e = 1;
  rec.pos.x = 7; rec.pos.z = 9;
  rec.target.x = 1; rec.target.z = 1;
  rec.vel_x = 4; rec.vel_z = 5;
  rec.anim_sel = 0x10;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.chase == 0);
  assert(rec.anim_sel == 0x10);
  assert(rec.stage92 == 1);
  assert(rec.timer89 == 0);
  assert(rec.target.x == 7 && rec.target.z == 9);
  assert(rec.vel_x == 0 && rec.vel_z == 0);
  assert(rec.install == 0);
}

/* stage 1 gate: gate 15 > timer89 14 -> the wait sync (0x87694 -> 0x87714)
 * returns with the latch and timer untouched. */
static void test_arm_29_stage1_gate_waits(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_entity_candidate cand[11];
  memset(&rng, 0, sizeof rng);
  memset(cand, 0, sizeof cand);
  rec.phase = 5;
  rec.rng = &rng;
  rec.team_candidates = cand;
  rec.stage92 = 1;
  rec.chase = 0;
  rec.player_e = 2;                 /* gate = (2*120)>>4 = 15 */
  rec.timer89 = 14;
  rec.delta = 0;
  rec.pos.x = 3; rec.pos.z = 4;
  rec.target.x = 0xAAA; rec.target.z = 0xBBB;
  rec.vel_x = 9; rec.vel_z = 9;
  rec.anim_sel = 0x11;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.timer89 == 14);
  assert(rec.stage92 == 1);
  assert(rec.target.x == 3 && rec.target.z == 4);
  assert(rec.vel_x == 0 && rec.vel_z == 0);
  assert(rec.anim_sel == 0x11);
}

/* stage 1 at the gate (timer89 == gate proceeds: `JG` is strict), active == 0
 * syncs target=pos and the zero distance advances the latch to 2. */
static void test_arm_29_stage1_active0_advances(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_entity_candidate cand[11];
  memset(&rng, 0, sizeof rng);
  memset(cand, 0, sizeof cand);
  rec.phase = 5;
  rec.rng = &rng;
  rec.team_candidates = cand;
  rec.stage92 = 1;
  rec.chase = 0;
  rec.active = 0;
  rec.player_e = 2;                 /* gate 15 */
  rec.timer89 = 15;
  rec.delta = 0;
  rec.pos.x = 10; rec.pos.z = 20;
  rec.target.x = 0x11; rec.target.z = 0x22;
  rec.vel_x = 1; rec.vel_z = 2;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.stage92 == 2);
  assert(rec.timer89 == 0);
  assert(rec.target.x == 10 && rec.target.z == 20);
  assert(rec.vel_x == 0 && rec.vel_z == 0);
  assert(rec.lane == 0);
}

/* stage 1 active != 0: the 0x6E1D0 phase cell selects the `[rec+8]+2` pair
 * (side == side_controlled) and writes 38*x / 33*z into the target; the
 * distance (174 > 0x20) then returns without advancing. */
static void test_arm_29_stage1_phase_cell_target(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_entity_candidate cand[11];
  memset(&rng, 0, sizeof rng);
  memset(cand, 0, sizeof cand);
  rec.phase = 5;
  rec.rng = &rng;
  rec.team_candidates = cand;
  rec.stage92 = 1;
  rec.chase = 0;
  rec.active = 1;
  rec.side = 0;
  rec.side_controlled = 0;
  rec.cell[0][0] = 1; rec.cell[0][1] = 1;
  rec.cell[1][0] = 3; rec.cell[1][1] = 4;
  rec.player_e = 0;
  rec.timer89 = 0;
  rec.delta = 0;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.target.x == 3 * 0x26);
  assert(rec.target.y == 0);
  assert(rec.target.z == 4 * 0x21);
  assert(rec.lane == 4 * 0x21);
  assert(rec.stage92 == 1);
  assert(rec.timer89 == 0);
}

/* side != side_controlled selects `[rec+8]+0`; side != 0 negates both axes
 * (0x6E225/0x6E227). */
static void test_arm_29_stage1_phase_cell_side_negates(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_entity_candidate cand[11];
  memset(&rng, 0, sizeof rng);
  memset(cand, 0, sizeof cand);
  rec.phase = 5;
  rec.rng = &rng;
  rec.team_candidates = cand;
  rec.stage92 = 1;
  rec.active = 1;
  rec.side = 1;
  rec.side_controlled = 0;
  rec.cell[0][0] = 1; rec.cell[0][1] = 0;
  rec.cell[1][0] = 9; rec.cell[1][1] = 9;
  rec.player_e = 0;
  rec.delta = 0;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.target.x == -(1 * 0x26));
  assert(rec.target.z == 0);
  assert(rec.stage92 == 1);          /* |dx| 38 > 0x20 */
}

/* A zero phase-cell pair gives distance 0 and advances the latch. */
static void test_arm_29_stage1_phase_cell_zero_advances(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_entity_candidate cand[11];
  memset(&rng, 0, sizeof rng);
  memset(cand, 0, sizeof cand);
  rec.phase = 5;
  rec.rng = &rng;
  rec.team_candidates = cand;
  rec.stage92 = 1;
  rec.active = 1;
  rec.side = 0;
  rec.side_controlled = 0;
  rec.cell[1][0] = 0; rec.cell[1][1] = 0;
  rec.player_e = 0;
  rec.delta = 0;
  rec.target.x = 0x123; rec.target.z = 0x456;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.target.x == 0 && rec.target.z == 0);
  assert(rec.stage92 == 2);
  assert(rec.timer89 == 0);
}

/* The chase re-check (0x876EE..0x876FA): a chased record returns before the
 * latch advance even with an in-range distance. */
static void test_arm_29_stage1_chase_blocks_advance(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_entity_candidate cand[11];
  memset(&rng, 0, sizeof rng);
  memset(cand, 0, sizeof cand);
  rec.phase = 5;
  rec.rng = &rng;
  rec.team_candidates = cand;
  rec.stage92 = 1;
  rec.chase = 1;
  rec.flag44 = 0;
  rec.active = 0;
  rec.player_e = 0;
  rec.timer89 = 5;
  rec.delta = 0;
  rec.pos.x = 1; rec.pos.z = 2;
  rec.target.x = 8; rec.target.z = 9;
  rec.vel_x = 1; rec.vel_z = 1;
  rec.anim_sel = 0x10;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.stage92 == 1);
  assert(rec.timer89 == 5);
  assert(rec.target.x == 1 && rec.target.z == 2);
  assert(rec.vel_x == 0 && rec.vel_z == 0);
  assert(rec.anim_sel == 0x10);
}

/* stage 2: only the target sync (0x87714..0x8772B) after the timer add. */
static void test_arm_29_stage2_syncs_target(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_entity_candidate cand[11];
  memset(&rng, 0, sizeof rng);
  memset(cand, 0, sizeof cand);
  rec.phase = 5;
  rec.rng = &rng;
  rec.team_candidates = cand;
  rec.stage92 = 2;
  rec.timer89 = 5;
  rec.delta = 3;
  rec.pos.x = 7; rec.pos.y = 8; rec.pos.z = 9;
  rec.target.x = 1; rec.target.y = 2; rec.target.z = 3;
  rec.vel_x = 4; rec.vel_z = 5;
  rec.anim_sel = 0x12;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.timer89 == 8);
  assert(rec.target.x == 7 && rec.target.y == 8 && rec.target.z == 9);
  assert(rec.vel_x == 0 && rec.vel_z == 0);
  assert(rec.stage92 == 2);
  assert(rec.anim_sel == 0x12);
}

/* stage >= 3 (and the 0xFF reset sentinel) run only the prologue. */
static void test_arm_29_stage_past_prologue_only(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_entity_candidate cand[11];
  memset(&rng, 0, sizeof rng);
  memset(cand, 0, sizeof cand);
  rec.phase = 5;
  rec.rng = &rng;
  rec.team_candidates = cand;
  rec.stage92 = 3;
  rec.timer89 = 5;
  rec.delta = 2;
  rec.pos.x = 1; rec.pos.z = 2;
  rec.target.x = 0xAA; rec.target.z = 0xBB;
  rec.vel_x = 3; rec.vel_z = 4;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.timer89 == 7);
  assert(rec.target.x == 0xAA && rec.target.z == 0xBB);
  assert(rec.vel_x == 3 && rec.vel_z == 4);
  assert(rec.stage92 == 3);
  rec.stage92 = 0xFF;
  rec.delta = 0;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.timer89 == 7);
  assert(rec.stage92 == 0xFF);
}

/* The nearest search honors the +0x98 exclusion: a closer excluded candidate
 * loses to the farther self record. */
static void test_arm_29_nearest_respects_exclusions(void) {
  struct fifa96_arm_record rec = arm_rec();
  fifa96_entity_candidate cand[11];
  struct fifa96_rng rng;
  memset(cand, 0, sizeof cand);
  memset(&rng, 0, sizeof rng);
  for (int i = 0; i < 11; i++) { cand[i].x = 1000; cand[i].y = 0; }
  cand[2].x = 1; cand[2].y = 0; cand[2].skip_98 = 1;
  cand[4].x = 50; cand[4].y = 0;
  rng.w[0] = 1;
  rec.phase = 5;
  rec.rng = &rng;
  rec.team_candidates = cand;
  rec.team_index = 4;
  rec.ball_pos.x = 0; rec.ball_pos.z = 0;
  rec.ball_skip = 0;
  rec.stage92 = 0;
  rec.delta = 0;
  rec.anim_sel = 0x10;
  rec.flag44 = 0;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.chase == 1);
  assert(rec.anim_sel == 0x5D);
}

static void test_arm_29_invalid(void) {
  struct fifa96_arm_record rec = arm_rec();
  fifa96_entity_candidate cand[11];
  struct fifa96_rng rng;
  memset(cand, 0, sizeof cand);
  memset(&rng, 0, sizeof rng);
  rec.phase = 5;
  rec.team_candidates = cand;
  rec.rng = NULL;
  assert(fifa96_arm_29_step(NULL) == ARM_INVALID);
  assert(fifa96_arm_29_step(&rec) == ARM_INVALID);
  assert(rec.timer7b == 0);          /* validation precedes the prologue */
  rec.rng = &rng;
  rec.team_candidates = NULL;
  assert(fifa96_arm_29_step(&rec) == ARM_INVALID);
  assert(rec.timer7b == 0);
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
  test_arm_2c_stage0_inactive_resets();
  test_arm_2c_stage0_active_selects_then_resets();
  test_arm_2c_stage1_selects_then_resets();
  test_arm_2c_stage2_above_delta_waits();
  test_arm_2c_stage2_equal_delta_resets();
  test_arm_2c_delta_zero_extended();
  test_arm_2c_delta_zero_resets();
  test_arm_2c_stage_past_untouched();
  test_arm_2c_invalid();
  test_arm_29_phase_not5_resets_and_requests_install();
  test_arm_29_phase_not5_ignores_null_pointers();
  test_arm_29_phase_not5_occupied_skips_install();
  test_arm_29_stage0_nearest_self_rng_odd();
  test_arm_29_stage0_nearest_self_rng_even_reselect();
  test_arm_29_stage0_nearest_other_gate_waits();
  test_arm_29_stage1_gate_waits();
  test_arm_29_stage1_active0_advances();
  test_arm_29_stage1_phase_cell_target();
  test_arm_29_stage1_phase_cell_side_negates();
  test_arm_29_stage1_phase_cell_zero_advances();
  test_arm_29_stage1_chase_blocks_advance();
  test_arm_29_stage2_syncs_target();
  test_arm_29_stage_past_prologue_only();
  test_arm_29_nearest_respects_exclusions();
  test_arm_29_invalid();
  puts("test_arm_bodies OK");
  return 0;
}
