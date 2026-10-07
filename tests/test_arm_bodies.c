/* tests/test_arm_bodies.c — M2 arms-and-wiring Tasks 3/4/5/6/7 / FU-142b/c/d:
 * the cluster-G row bodies (`0x866F4` row 26, `0x86820` row 27, `0x84598` row
 * 2C, `0x874E4` row 29, `0x870E8` row 28) and the shared 0x36200 stub.
 *
 * Evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix C (Ghidra
 * read-only /FIFA96.EXE: disassemble_bytes 0x866F4, 304 B; read_memory 0x110778
 * proving action-table row 0x26 = 0x866F4; read_memory 0x10F394 for the
 * 0x157A38 table; disassemble_bytes 0x36200), Appendix D (row 27), Appendix E
 * (row 2C), Appendix F (row 29: disassemble_bytes 0x874E4, 596 B;
 * read_memory 0x110784 proving action-table row 0x29 = 0x874E4; the 0x8DE8C /
 * 0x6E1D0 helper windows) and Appendix G (row 28: disassemble_bytes 0x870E8,
 * 1024 B; read_memory 0x110780 proving action-table row 0x28 = 0x870E8;
 * read_memory 0x870D8 = the 4-arm table; disassemble_bytes 0x87014; the
 * 0x7D8B0/C0 id tables and the 257-dword 0x114E04 table). */
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

/* The install request is per call: a stale request from a previous call is
 * cleared on every successful path, because Task 9's binder/drain consumes
 * the field (a surviving stale `install = 3` would double-install). */
static void test_arm_29_install_cleared_per_call(void) {
  struct fifa96_arm_record rec = arm_rec();
  fifa96_entity_candidate cand[11];
  struct fifa96_rng rng;
  memset(cand, 0, sizeof cand);
  memset(&rng, 0, sizeof rng);
  /* occupied reset path: the stale request is not re-issued and is cleared */
  rec.phase = 4;
  rec.skip_9a = 1;
  rec.install = 3;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.install == 0);
  /* phase-5 latch path: the stale request never survives the call either */
  rec = arm_rec();
  rec.phase = 5;
  rec.rng = &rng;
  rec.team_candidates = cand;
  rec.stage92 = 2;
  rec.install = 3;
  assert(fifa96_arm_29_step(&rec) == FIFA96_OK);
  assert(rec.install == 0);
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

/* --- Row 0x28 body `0x870E8..0x874E3` (294 instructions, Appendix G) ------
 * The 4-arm jump table `0x870D8` = {0x87147, 0x87276, 0x87364, 0x874DA}; the
 * prologue runs the `0x8DCD4` out triple + `0x79C50` face before the stage92
 * switch. Arm 0 builds target = ([0x10F364], [0x10F368]) (+ side-0 negate when
 * [0x157AC2] < 4) and adds the `0x114E04` folds of (int8)active * 22.5 degrees:
 * x += 144*cos, z += 144*sin (first-hand fold pins below were generated from a
 * literal transcription of the native byte-shift idiom over the first-hand
 * 257-dword table; they agree with `fifa96_projection_sincos` for all 256
 * active values). Arm 1 waits on `+0xA2`, then either target = pos (flag830
 * clear, id 1) or the `0x87014` setup and the `+0xAA`/`+0xAE`/`+0xA0`/`+0xA1`
 * draws with the latch advancing to 2. Arm 2 runs the `+0xAA` gate, the
 * `0x7D8B0` id table, the `[0x10F35C]`/chosen-record branch (target = chosen
 * position + `+0xA2`/`+0xA6`, zero velocity, the `0x7D8C0` id table) and the
 * `0xCC0`/`+0xA6` chase target with id 0x15. RNG expectations below are pinned
 * for `fifa96_rng_seed(seed)` (the derived transcription of
 * `FUN_00092AA0`/`FUN_00092AC8`); each comment names the seed. */

/* arm 3 (and > 3) is the shared epilogue: the prologue still runs (face writes
 * the dx=+0x200 octant), and no stage/scratch/timer state changes. */
static void test_arm_28_arm3_epilogue_only(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 3;
  rec.timer89 = 7;
  rec.delta = 3;
  rec.pos.x = 0;
  rec.target.x = 0x200;          /* dx = +0x200, dz = 0 */
  rec.target.z = 0;
  rec.type = 0x55;
  assert(fifa96_arm_28_step(&rec, 3) == FIFA96_OK);
  assert(rec.type == 2);         /* +x octant (0x100 - atan[0]) */
  assert(rec.timer89 == 7);
  assert(rec.stage92 == 3);
  assert(rec.target.x == 0x200 && rec.target.z == 0);
  assert(rec.scratch_a2 == 0 && rec.scratch_aa == 0);
  /* > 3 takes the same epilogue; the prologue tolerates a NULL rng. */
  assert(fifa96_arm_28_step(&rec, 4) == FIFA96_OK);
  assert(rec.timer89 == 7);
  assert(fifa96_arm_28_step(&rec, 0xFF) == FIFA96_OK);
  assert(rec.stage92 == 3);
}

/* arm 0 with the distance gate open (prologue distance 0x200 > 0x60): the
 * set-piece target + folds are written, the constant id 0x15 is selected and
 * the call returns before any RNG draw (NULL rng is fine). */
static void test_arm_28_arm0_setpiece_target_and_fold(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 0;
  rec.pos.x = 0x200;             /* prologue distance 0x200 > 0x60 */
  rec.pos.z = 0x10;
  rec.target.x = 0;
  rec.target.z = 0;
  rec.active = 1;                /* 22.5 degrees: x +133, z +55 */
  rec.global_10f364 = 0x100;
  rec.global_10f368 = 0x200;
  rec.global_157ac2 = 4;         /* >= 4: no side-0 negation */
  rec.timer89 = 5;
  assert(fifa96_arm_28_step(&rec, 0) == FIFA96_OK);
  assert(rec.target.x == 0x100 + 133);
  assert(rec.target.z == 0x200 + 55);
  assert(rec.anim_sel == 0x15);  /* constant-id select (0x87225) */
  assert(rec.timer89 == 5);      /* returned at the distance gate */
  assert(rec.stage92 == 0);
  assert(rec.lane == -0x10);     /* 0x8DCD4 out[2] = target.z - pos.z */
}

/* `[0x157AC2] < 4` with a side-0 team negates the z target before the folds
 * (0x87175..0x87181); side 1 and mode >= 4 keep the sign. */
static void test_arm_28_arm0_mode_negates_side0_z(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 0;
  rec.pos.x = 0x200;
  rec.target.x = 0;
  rec.target.z = 0;
  rec.active = 0;                /* folds: x +144, z 0 */
  rec.global_10f364 = 0x100;
  rec.global_10f368 = 0x200;
  rec.global_157ac2 = 3;         /* < 4 */
  rec.side = 0;
  assert(fifa96_arm_28_step(&rec, 0) == FIFA96_OK);
  assert(rec.target.x == 0x100 + 144);
  assert(rec.target.z == -0x200);
  rec.target.z = 0;
  rec.side = 1;
  assert(fifa96_arm_28_step(&rec, 0) == FIFA96_OK);
  assert(rec.target.z == 0x200);
  rec.target.z = 0;
  rec.side = 0;
  rec.global_157ac2 = 4;         /* >= 4: no negation even for side 0 */
  assert(fifa96_arm_28_step(&rec, 0) == FIFA96_OK);
  assert(rec.target.z == 0x200);
}

/* The 0x114E04 fold pins (x = 144*cos, z = 144*sin of active * 22.5 degrees),
 * read through the set-piece base (0, 0). */
static void test_arm_28_arm0_fold_table_pins(void) {
  static const struct { uint8_t active; int32_t x, z; } cases[] = {
    { 0x00, 144, 0 },      /* 0 deg */
    { 0x01, 133, 55 },     /* 22.5 deg */
    { 0x02, 102, 102 },    /* 45 deg */
    { 0x03, 55, 133 },     /* 67.5 deg */
    { 0x04, 0, 144 },      /* 90 deg */
    { 0xFF, 133, -55 },    /* -22.5 deg */
    { 0x08, -144, 0 },     /* 180 deg */
  };
  for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
    struct fifa96_arm_record rec = arm_rec();
    rec.stage92 = 0;
    rec.pos.x = 0x200;
    rec.target.x = 0;
    rec.target.z = 0;
    rec.active = cases[i].active;
    rec.global_157ac2 = 4;
    assert(fifa96_arm_28_step(&rec, 0) == FIFA96_OK);
    assert(rec.target.x == cases[i].x);
    assert(rec.target.z == cases[i].z);
  }
}

/* Arm 0 inside the distance gate falls through into arm 1: one draw seeds
 * +0xA2 = (RNG & 0x7F) + 0x20 and the latch advances to 1; arm 1 then adds the
 * delta and waits below the gate. seed 0: draw 512 -> 0x20. */
static void test_arm_28_arm0_falls_into_arm1(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_rng_seed(&rng, 0);
  rec.rng = &rng;
  rec.stage92 = 0;
  rec.pos.x = 0;                 /* distance 0, gate open */
  rec.target.x = 0;
  rec.target.z = 0;
  rec.active = 0;
  rec.global_157ac2 = 4;
  rec.timer89 = 5;
  rec.delta = 0;
  assert(fifa96_arm_28_step(&rec, 0) == FIFA96_OK);
  assert(rec.target.x == 144);   /* arm 0 fold */
  assert(rec.stage92 == 1);      /* arm 0 latch, arm 1 wait */
  assert(rec.timer89 == 0);
  assert(rec.scratch_a2 == 0x20);
  assert(rec.anim_sel == 0x15);
}

/* Arm 1 with flag830 clear and the delta pushing past +0xA2: one re-arm draw,
 * target = pos and the constant id 1, latch stays 1 (0x8733A). seed 0:
 * d1 512 -> +0xA2 0x20 (arm 0), d2 1829 -> +0xA2 0x45. */
static void test_arm_28_arm1_flag830_clear_syncs_pos(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_rng_seed(&rng, 0);
  rec.rng = &rng;
  rec.stage92 = 0;
  rec.pos.x = 0x11;
  rec.pos.z = 0x22;
  rec.target.x = 0;
  rec.target.z = 0;
  rec.active = 0;
  rec.global_157ac2 = 4;
  rec.delta = 0xFFFF;            /* timer89 0 + delta >> gate */
  rec.flag830 = 0;
  assert(fifa96_arm_28_step(&rec, 0) == FIFA96_OK);
  assert(rec.scratch_a2 == 0x45);
  assert(rec.timer89 == 0);
  assert(rec.stage92 == 1);
  assert(rec.target.x == 0x11 && rec.target.z == 0x22);
  assert(rec.anim_sel == 1);
  assert(rec.scratch_a6 == 0 && rec.scratch_aa == 0 && rec.scratch_ae == 0);
}

/* Arm 1 with flag830 set runs the 0x87014 setup (six draws) plus the arm's
 * four draws and latches to 2; seed 1 pins every scratch cell, the flag830=1
 * path stops after +0xA0 = 1 (the +0xA1 draw is conditional). Expected:
 * +0xA2 -72, +0xA6 -187, +0xAA 354, +0xAE 35, +0xA0 1, +0xA1 0. */
static void test_arm_28_arm1_flag830_set_scratch_machine(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_rng_seed(&rng, 1);
  rec.rng = &rng;
  rec.stage92 = 1;
  rec.timer89 = 0;
  rec.delta = 0xFFFF;
  rec.scratch_a2 = 1;            /* gate fires immediately */
  rec.flag830 = 1;
  rec.target.x = 0xAA; rec.target.z = 0xBB;
  assert(fifa96_arm_28_step(&rec, 1) == FIFA96_OK);
  assert(rec.scratch_a2 == -0x48);
  assert(rec.scratch_a6 == -187);
  assert(rec.scratch_aa == 354);
  assert(rec.scratch_ae == 35);
  assert(rec.scratch_a0 == 1);
  assert(rec.scratch_a1 == 0);
  assert(rec.timer89 == 0);
  assert(rec.stage92 == 2);
  assert(rec.target.x == 0xAA && rec.target.z == 0xBB);
}

/* The +0xA1 draw happens only when +0xA0 == 0 (0x8730C..0x87317): seed 4 has
 * d10 even / d11 odd, so +0xA1 = 1 and +0xA6 stays positive (d5 even).
 * Expected: +0xA2 -72, +0xA6 187, +0xAA 356, +0xAE 38, +0xA0 0, +0xA1 1. */
static void test_arm_28_arm1_flag830_set_a1_draw(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_rng_seed(&rng, 4);
  rec.rng = &rng;
  rec.stage92 = 1;
  rec.timer89 = 0;
  rec.delta = 0xFFFF;
  rec.scratch_a2 = 1;
  rec.flag830 = 1;
  assert(fifa96_arm_28_step(&rec, 1) == FIFA96_OK);
  assert(rec.scratch_a2 == -0x48);
  assert(rec.scratch_a6 == 187);
  assert(rec.scratch_aa == 356);
  assert(rec.scratch_ae == 38);
  assert(rec.scratch_a0 == 0);
  assert(rec.scratch_a1 == 1);
  assert(rec.stage92 == 2);
}

/* Arm 2 above the +0xAA gate with [0x10F358] == 0: timer89 = 0, the gate
 * re-roll, the 0x87014 setup and the 0x7D8B0 id table. seed 0: +0xAA 210,
 * +0xA2 -72, +0xA6 -187, +0xAE 309, id draw & 3 = 1 -> 0x19. The [0x10F35C]
 * chase flag then blocks the second block (timer89 0 not > +0xAE). */
static void test_arm_28_arm2_aa_gate_vb_table(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_rng_seed(&rng, 0);
  rec.rng = &rng;
  rec.stage92 = 2;
  rec.timer89 = 0;
  rec.delta = 0xFFFF;            /* timer89 > +0xAA (1) */
  rec.scratch_aa = 1;
  rec.global_10f358 = 0;
  rec.global_10f35c = 1;
  rec.scratch_ae = 0x7FFF;       /* second block waits */
  rec.target.x = 0xAA; rec.target.z = 0xBB;
  assert(fifa96_arm_28_step(&rec, 2) == FIFA96_OK);
  assert(rec.scratch_aa == 210);
  assert(rec.scratch_a2 == -0x48);
  assert(rec.scratch_a6 == -187);
  assert(rec.scratch_ae == 309);
  assert(rec.anim_sel == 0x19);  /* vb draw & 3 = 1 */
  assert(rec.timer89 == 0);
  assert(rec.stage92 == 2);
  assert(rec.target.x == 0xAA && rec.target.z == 0xBB);
  /* with [0x10F358] != 0 the re-roll block is skipped (0x87384 JNZ) and the
   * [0x10F35C] chase gate compares the raw timer (signed) against +0xAE */
  rec = arm_rec();
  fifa96_rng_seed(&rng, 0);
  rec.rng = &rng;
  rec.stage92 = 2;
  rec.timer89 = 0;
  rec.delta = 0x1000;            /* timer89 = 0x1000 */
  rec.scratch_aa = 1;
  rec.global_10f358 = 1;
  rec.global_10f35c = 1;
  rec.scratch_ae = 0x2000;       /* 0x1000 not > 0x2000 -> waits */
  assert(fifa96_arm_28_step(&rec, 2) == FIFA96_OK);
  assert(rec.timer89 == 0x1000);
  assert(rec.scratch_aa == 1);
  assert(rec.stage92 == 2);
}

/* Arm 2 with [0x10F35C] != 0 above the +0xAE gate: target = (0xCC0, +0xA6),
 * the constant id 0x15, timer89 = 0 and the latch advances to 3 (0x873EA). */
static void test_arm_28_arm2_chase_cc0_anim15(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.timer89 = 5;
  rec.delta = 0;
  rec.scratch_aa = 0x7FFF;       /* first block skipped */
  rec.global_10f35c = 1;
  rec.scratch_ae = 0;
  rec.scratch_a6 = -0x22;
  rec.target.x = 7; rec.target.z = 9;
  assert(fifa96_arm_28_step(&rec, 2) == FIFA96_OK);
  assert(rec.target.x == 0xCC0);
  assert(rec.target.z == -0x22);
  assert(rec.anim_sel == 0x15);
  assert(rec.timer89 == 0);
  assert(rec.stage92 == 3);
}

/* Arm 2 with [0x10F35C] == 0: target = the chosen record position + the two
 * gate offsets (0x87433..0x87462). With the prologue distance < 0x20 and
 * [0x10F358] == 0 the call returns there. */
static void test_arm_28_arm2_chosen_target_offsets(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.timer89 = 5;
  rec.delta = 0;
  rec.scratch_aa = 0x7FFF;
  rec.global_10f35c = 0;
  rec.global_10f358 = 0;
  rec.chosen_ok = 1;
  rec.chosen_pos.x = 0x100;
  rec.chosen_pos.y = 0x33;
  rec.chosen_pos.z = 0x200;
  rec.scratch_a2 = 5;
  rec.scratch_a6 = -7;
  rec.target.x = 0; rec.target.z = 0;   /* prologue distance 0 */
  assert(fifa96_arm_28_step(&rec, 2) == FIFA96_OK);
  assert(rec.target.x == 0x105);
  assert(rec.target.y == 0x33);         /* the copy takes the x/y/z triple */
  assert(rec.target.z == 0x1F9);
  assert(rec.timer89 == 5);
  assert(rec.stage92 == 2);
}

/* A missing chosen record (OL-58 bounded model) leaves the target triple and
 * still applies the two gate offsets; no crash, no invented position. */
static void test_arm_28_arm2_chosen_missing_bounded(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.delta = 0;
  rec.scratch_aa = 0x7FFF;
  rec.global_10f35c = 0;
  rec.global_10f358 = 0;
  rec.chosen_ok = 0;
  rec.chosen_pos.y = 0x77;
  rec.scratch_a2 = 5;
  rec.scratch_a6 = -7;
  rec.target.x = 0x10; rec.target.y = 0x44; rec.target.z = 0x20;
  assert(fifa96_arm_28_step(&rec, 2) == FIFA96_OK);
  assert(rec.target.x == 0x15);
  assert(rec.target.y == 0x44);         /* no chosen triple -> y untouched */
  assert(rec.target.z == 0x19);
}

/* Arm 2 hard approach ([0x10F358] != 0, distance < 0x20): target = pos, the
 * velocity words zero, and below the +0xAE gate it returns (0x8749A JGE). */
static void test_arm_28_arm2_hard_approach_syncs(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.timer89 = 5;
  rec.delta = 0;
  rec.scratch_aa = 0x7FFF;
  rec.global_10f35c = 0;
  rec.global_10f358 = 1;
  rec.chosen_ok = 1;
  rec.chosen_pos.x = 0;
  rec.chosen_pos.z = 0;
  rec.scratch_a6 = -7;
  rec.scratch_ae = 0x7FFF;       /* timer89 5 not > +0xAE */
  rec.pos.x = 0x31; rec.pos.z = 0x42;
  rec.target.x = 0x31; rec.target.z = 0x42;  /* prologue distance 0 */
  rec.vel_x = 9; rec.vel_z = 9;
  assert(fifa96_arm_28_step(&rec, 2) == FIFA96_OK);
  assert(rec.target.x == 0x31 && rec.target.z == 0x42);
  assert(rec.vel_x == 0 && rec.vel_z == 0);
  assert(rec.timer89 == 5);
  assert(rec.stage92 == 2);
}

/* Arm 2 hard approach above the +0xAE gate: the +0xAE re-roll and the 0x7D8C0
 * id table (0x874A6..0x874D5). seed 0: d1 512 -> +0xAE 0x20, d2 1829 & 3 = 1
 * -> 0x58. */
static void test_arm_28_arm2_hard_approach_reroll_vc(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_rng_seed(&rng, 0);
  rec.rng = &rng;
  rec.stage92 = 2;
  rec.timer89 = 5;
  rec.delta = 0;
  rec.scratch_aa = 0x7FFF;
  rec.global_10f35c = 0;
  rec.global_10f358 = 1;
  rec.chosen_ok = 1;
  rec.chosen_pos.x = 0;
  rec.chosen_pos.z = 0;
  rec.scratch_ae = 0;            /* timer89 5 > +0xAE */
  rec.vel_x = 9; rec.vel_z = 9;
  assert(fifa96_arm_28_step(&rec, 2) == FIFA96_OK);
  assert(rec.scratch_ae == 0x20);
  assert(rec.anim_sel == 0x58);
  assert(rec.timer89 == 0);
  assert(rec.vel_x == 0 && rec.vel_z == 0);
  assert(rec.stage92 == 2);
}

/* The arm-0 row-byte gate (0x87213..0x87237): anim_sel already 0x15 skips the
 * constant-id select. */
static void test_arm_28_arm0_row_byte_gate(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 0;
  rec.pos.x = 0x200;
  rec.active = 0;
  rec.global_157ac2 = 4;
  rec.anim_sel = 0x15;
  assert(fifa96_arm_28_step(&rec, 0) == FIFA96_OK);
  assert(rec.anim_sel == 0x15);
}

/* NULL rec and a draw on a NULL rng are invalid; a no-draw path tolerates a
 * NULL rng. */
static void test_arm_28_invalid(void) {
  struct fifa96_arm_record rec = arm_rec();
  assert(fifa96_arm_28_step(NULL, 0) == ARM_INVALID);
  /* arm 0 inside the distance gate draws -> NULL rng aborts */
  rec.stage92 = 0;
  rec.pos.x = 0;
  rec.target.x = 0;
  rec.target.z = 0;
  rec.global_157ac2 = 4;
  rec.rng = NULL;
  assert(fifa96_arm_28_step(&rec, 0) == ARM_INVALID);
  /* arm 1 below the +0xA2 gate draws nothing -> NULL rng is fine */
  rec = arm_rec();
  rec.stage92 = 1;
  rec.timer89 = 0;
  rec.delta = 0;
  rec.scratch_a2 = 10;
  rec.rng = NULL;
  assert(fifa96_arm_28_step(&rec, 1) == FIFA96_OK);
  assert(rec.timer89 == 0);
}

/* --- Row 0x2A body `0x86A34..0x87010` (409 instructions, Appendix H) ------
 * The 12-dword jump table `0x86A04` = {0x86A91, 0x86AFE, 0x86B5F, 0x86BA2,
 * 0x86BDC, 0x86C7D, 0x86D1E, 0x86DBF, 0x86E60, 0x86F2F, 0x86F83, 0x8700A};
 * entry 11 (0x8700A) is the shared epilogue RET. The prologue always adds the
 * zero-extended `[0x157A64]` delta to timer89; when the signed stage92 byte
 * is > 2 it also writes timer7b = 4 and runs the 0x36200 stub (native EAX=2),
 * then stage92 > 0xB (unsigned) goes to the epilogue. The native `+0x65`
 * distance word (the unported FUN_0008D098 pre-switch walk 0x8D11E) is the
 * `distance` input; arm 8's four 0x92AC8 draws are pinned for seed 63
 * (0x7801, 0xEB29, 0xD746, 0x3DC7). The 0x79C50 face calls use explicit
 * direction words: arm 0 passes (0, 0) (the zero-direction no-op) and arm 10
 * passes (0, -100) (octant 4). */

/* Arm 0 (0x86A91): target = (-0x720, 0), team flag830 = 0, both globals = 0,
 * the (0,0) face no-op leaves +0x8E, id 0x60, timer89 = 0, latch 0 -> 1. */
static void test_arm_2a_arm0_target_and_clears(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 0;
  rec.timer89 = 5;
  rec.delta = 3;
  rec.target.x = 0x111;
  rec.target.z = 0x222;
  rec.type = 0x77;
  rec.flag830 = 7;
  rec.global_10f358 = 9;
  rec.global_10f35c = 9;
  rec.anim_sel = 0x99;
  assert(fifa96_arm_2a_step(&rec, 0) == FIFA96_OK);
  assert(rec.target.x == -0x720);
  assert(rec.target.z == 0);
  assert(rec.flag830 == 0);
  assert(rec.global_10f358 == 0);
  assert(rec.global_10f35c == 0);
  assert(rec.type == 0x77);      /* (dx,dz) = (0,0): no face write */
  assert(rec.anim_sel == 0x60);  /* constant id 0x60 */
  assert(rec.timer89 == 0);
  assert(rec.stage92 == 1);
}

/* Arm 1 (0x86AFE): the distance gate `> 0x20` returns after the prologue;
 * `<= 0x20` selects id 0x61, sets flag830 = 1, target = (-0x540, 0),
 * timer89 = 0 and advances the latch. */
static void test_arm_2a_arm1_distance_gate(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 1;
  rec.distance = 0x21;
  rec.delta = 0;
  rec.timer89 = 7;
  rec.target.x = 0x111;
  rec.target.z = 0x222;
  rec.flag830 = 3;
  rec.anim_sel = 0x99;
  assert(fifa96_arm_2a_step(&rec, 1) == FIFA96_OK);
  assert(rec.target.x == 0x111 && rec.target.z == 0x222);
  assert(rec.flag830 == 3 && rec.anim_sel == 0x99);
  assert(rec.timer89 == 7 && rec.stage92 == 1);

  rec.distance = 0x20;
  assert(fifa96_arm_2a_step(&rec, 1) == FIFA96_OK);
  assert(rec.target.x == -0x540 && rec.target.z == 0);
  assert(rec.flag830 == 1 && rec.anim_sel == 0x61);
  assert(rec.timer89 == 0 && rec.stage92 == 2);
}

/* Arm 2 (0x86B5F): the distance gate then the 0x513EC camera stop (the derived
 * no-op), timer89 = 0 and the latch advance. */
static void test_arm_2a_arm2_latch(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 2;
  rec.distance = 0x21;
  rec.delta = 0;
  rec.timer89 = 7;
  rec.target.x = 0x111;
  assert(fifa96_arm_2a_step(&rec, 2) == FIFA96_OK);
  assert(rec.timer89 == 7 && rec.stage92 == 2 && rec.target.x == 0x111);

  rec.distance = 0x20;
  assert(fifa96_arm_2a_step(&rec, 2) == FIFA96_OK);
  assert(rec.timer89 == 0 && rec.stage92 == 3);
  assert(rec.target.x == 0x111);   /* the body writes no target in arm 2 */
}

/* Arm 3 (0x86BA2): timer89 < 0x78 (signed, negatives included) returns after
 * the pre-dispatch block (timer7b = 4); >= 0x78 writes (-0x540, -0x930). */
static void test_arm_2a_arm3_timer_gate(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 3;
  rec.delta = 0;
  rec.timer89 = 0x77;
  rec.timer7b = 0;
  rec.target.x = 0x111;
  rec.target.z = 0x222;
  assert(fifa96_arm_2a_step(&rec, 3) == FIFA96_OK);
  assert(rec.timer7b == 4);        /* stage92 3 > 2 pre-dispatch block */
  assert(rec.timer89 == 0x77 && rec.stage92 == 3);
  assert(rec.target.x == 0x111 && rec.target.z == 0x222);

  rec.timer89 = -1;                /* signed JL: negative waits too */
  assert(fifa96_arm_2a_step(&rec, 3) == FIFA96_OK);
  assert(rec.timer89 == -1 && rec.stage92 == 3);

  rec.timer89 = 0x78;
  assert(fifa96_arm_2a_step(&rec, 3) == FIFA96_OK);
  assert(rec.target.x == -0x540 && rec.target.z == -0x930);
  assert(rec.timer89 == 0 && rec.stage92 == 4);
}

/* Arm 4 (0x86BDC): pos.z thresholds (0x930 - |pos.z|) add 0x90/0x120/0x1B0/
 * 0x240 into target.x; the tail continues to (0x540, -0x930) only when
 * pos.z <= -0x900. */
static void test_arm_2a_arm4_posz_branches(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 4;
  rec.delta = 0;
  rec.distance = 0;
  rec.target.z = 0x222;

  rec.distance = 0x241;
  rec.target.x = 0x111;
  assert(fifa96_arm_2a_step(&rec, 4) == FIFA96_OK);
  assert(rec.target.x == 0x111 && rec.target.z == 0x222);
  assert(rec.stage92 == 4);

  rec.distance = 0;
  /* d = 0x930 > 0x180 -> +0x90; pos.z = 0 > -0x900 -> return */
  rec.pos.z = 0;
  rec.target.x = 0x111;
  assert(fifa96_arm_2a_step(&rec, 4) == FIFA96_OK);
  assert(rec.target.x == -0x4B0);
  assert(rec.target.z == 0x222 && rec.stage92 == 4);

  /* d = 0x130 (0xC0 < d <= 0x180) -> +0x120 */
  rec.pos.z = 0x800;
  assert(fifa96_arm_2a_step(&rec, 4) == FIFA96_OK);
  assert(rec.target.x == -0x420);

  /* d = 0x90 (0x60 < d <= 0xC0) -> +0x1B0 */
  rec.pos.z = 0x8A0;
  assert(fifa96_arm_2a_step(&rec, 4) == FIFA96_OK);
  assert(rec.target.x == -0x390);

  /* d = 0x60 -> +0x240 */
  rec.pos.z = 0x8D0;
  assert(fifa96_arm_2a_step(&rec, 4) == FIFA96_OK);
  assert(rec.target.x == -0x300);

  /* tail: pos.z = -0x900 is not > -0x900 -> (-0x540, -0x930), latch */
  rec.pos.z = -0x900;
  assert(fifa96_arm_2a_step(&rec, 4) == FIFA96_OK);
  assert(rec.target.x == 0x540 && rec.target.z == -0x930);
  assert(rec.timer89 == 0 && rec.stage92 == 5);
}

/* Arm 5 (0x86C7D): pos.x thresholds (0x540 - |pos.x|) add into target.z; the
 * tail continues to (0x540, 0x930) at pos.x >= 0x510 (exactly 0x510 passes). */
static void test_arm_2a_arm5_posx_branches(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 5;
  rec.delta = 0;
  rec.distance = 0;
  rec.pos.x = 0;
  rec.target.x = 0x111;
  rec.target.z = 0x222;
  assert(fifa96_arm_2a_step(&rec, 5) == FIFA96_OK);
  assert(rec.target.z == -0x8A0);          /* -0x930 + 0x90 */
  assert(rec.target.x == 0x111);           /* pos.x < 0x510 -> return */
  assert(rec.stage92 == 5);

  rec.pos.x = 0x510;                       /* d = 0x30 -> +0x240, tail passes */
  assert(fifa96_arm_2a_step(&rec, 5) == FIFA96_OK);
  assert(rec.target.x == 0x540 && rec.target.z == 0x930);
  assert(rec.timer89 == 0 && rec.stage92 == 6);
}

/* Arm 6 (0x86D1E): pos.z thresholds (0x930 - |pos.z|) subtract from target.x;
 * the tail continues to (-0x540, 0x930) at pos.z >= 0x900. */
static void test_arm_2a_arm6_posz_subtract(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 6;
  rec.delta = 0;
  rec.distance = 0;
  rec.pos.z = 0;
  rec.target.x = 0x555;
  rec.target.z = 0x222;
  assert(fifa96_arm_2a_step(&rec, 6) == FIFA96_OK);
  assert(rec.target.x == 0x4B0);           /* 0x540 - 0x90 */
  assert(rec.target.z == 0x222 && rec.stage92 == 6);

  rec.pos.z = 0x900;                       /* d = 0x30 -> -0x240, tail passes */
  assert(fifa96_arm_2a_step(&rec, 6) == FIFA96_OK);
  assert(rec.target.x == -0x540 && rec.target.z == 0x930);
  assert(rec.timer89 == 0 && rec.stage92 == 7);
}

/* Arm 7 (0x86DBF): pos.x thresholds (0x540 - |pos.x|) subtract from target.z;
 * the tail continues to (-0x540, 0x588) at pos.x <= -0x510. */
static void test_arm_2a_arm7_posx_subtract(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 7;
  rec.delta = 0;
  rec.distance = 0;
  rec.pos.x = 0;
  rec.target.x = 0x111;
  rec.target.z = 0x999;
  assert(fifa96_arm_2a_step(&rec, 7) == FIFA96_OK);
  assert(rec.target.z == 0x8A0);           /* 0x930 - 0x90 */
  assert(rec.target.x == 0x111 && rec.stage92 == 7);

  rec.pos.x = -0x510;                      /* d = 0x30 -> -0x240, tail passes */
  assert(fifa96_arm_2a_step(&rec, 7) == FIFA96_OK);
  assert(rec.target.x == -0x540 && rec.target.z == 0x588);
  assert(rec.timer89 == 0 && rec.stage92 == 8);
}

/* Arm 8 (0x86E60): the distance and pos.z gates, then four RNG draws. seed 63:
 * d1 0x7801 -> |x| 1, d2 0xEB29 -> |z| 0x129, d3 0xD746 bit 0 clear -> x = -1,
 * d4 0x3DC7 bit 0 set -> z = +297. The whole path needs a non-NULL rng; the
 * pos.z tail `> 0x5B8` returns before the first draw. */
static void test_arm_2a_arm8_rng_target(void) {
  struct fifa96_arm_record rec = arm_rec();
  struct fifa96_rng rng;
  fifa96_rng_seed(&rng, 63);
  rec.rng = &rng;
  rec.stage92 = 8;
  rec.delta = 0;
  rec.timer89 = 5;
  rec.distance = 0;
  rec.pos.z = 0;                           /* <= 0x5B8: the draws run */
  assert(fifa96_arm_2a_step(&rec, 8) == FIFA96_OK);
  assert(rec.target.x == -1);
  assert(rec.target.z == 297);
  assert(rec.timer89 == 0 && rec.stage92 == 9);

  rec = arm_rec();
  rec.stage92 = 8;
  rec.distance = 0;
  rec.pos.z = 0x5B9;                       /* > 0x5B8: no draws; the x branch
                                            * already wrote target.x */
  rec.target.x = 0x11;
  rec.target.z = 0x22;
  assert(fifa96_arm_2a_step(&rec, 8) == FIFA96_OK);
  assert(rec.target.x == -0x4B0);          /* -0x540 + 0x90 */
  assert(rec.target.z == 0x22);
  assert(rec.stage92 == 8);

  rec = arm_rec();
  rec.stage92 = 8;
  rec.distance = 0x241;                    /* distance gate first */
  rec.target.x = 0x33;
  assert(fifa96_arm_2a_step(&rec, 8) == FIFA96_OK);
  assert(rec.target.x == 0x33);
}

/* Arm 9 (0x86F2F): the distance gate `<= 0x20` syncs the full target triple to
 * pos, zeroes the velocity pair, timer89 = 0 and sets [0x10F358] = 1. */
static void test_arm_2a_arm9_sync_global(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 9;
  rec.delta = 0;
  rec.distance = 0x21;
  rec.timer89 = 5;
  rec.target.x = 0x111;
  assert(fifa96_arm_2a_step(&rec, 9) == FIFA96_OK);
  assert(rec.target.x == 0x111 && rec.timer89 == 5 && rec.stage92 == 9);
  assert(rec.global_10f358 == 0);

  rec.distance = 0x20;
  rec.pos.x = 0x11;
  rec.pos.y = 0x22;
  rec.pos.z = 0x33;
  rec.target.x = 0x44;
  rec.target.y = 0x55;
  rec.target.z = 0x66;
  rec.vel_x = 7;
  rec.vel_z = 8;
  assert(fifa96_arm_2a_step(&rec, 9) == FIFA96_OK);
  assert(rec.target.x == 0x11 && rec.target.y == 0x22 && rec.target.z == 0x33);
  assert(rec.vel_x == 0 && rec.vel_z == 0);
  assert(rec.timer89 == 0 && rec.stage92 == 10);
  assert(rec.global_10f358 == 1);
}

/* Arm 10 (0x86F83): id 0x60 and the (0, -100) face octant run before the
 * timer89 >= 0x708 gate; below it the call returns (face already stored). At
 * or above it: id 0x64, target = (0xCC0, 0), id 0x61, timer89 = 0 and
 * [0x10F35C] = 1. */
static void test_arm_2a_arm10_anim_face_gate(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 10;
  rec.delta = 0;
  rec.timer89 = 0x707;
  rec.type = 0x77;
  rec.anim_sel = 0x99;
  rec.target.x = 0x111;
  rec.target.z = 0x222;
  rec.global_10f35c = 0;
  assert(fifa96_arm_2a_step(&rec, 10) == FIFA96_OK);
  assert(rec.anim_sel == 0x60);
  assert(rec.type == 4);                 /* (dx,dz) = (0,-100) octant */
  assert(rec.target.x == 0x111 && rec.target.z == 0x222);
  assert(rec.timer89 == 0x707 && rec.stage92 == 10);
  assert(rec.global_10f35c == 0);

  rec.timer89 = 0x708;
  assert(fifa96_arm_2a_step(&rec, 10) == FIFA96_OK);
  assert(rec.anim_sel == 0x61);          /* 0x60, 0x64, then 0x61 */
  assert(rec.type == 4);
  assert(rec.target.x == 0xCC0 && rec.target.z == 0);
  assert(rec.timer89 == 0 && rec.stage92 == 11);
  assert(rec.global_10f35c == 1);
}

/* The shared epilogue (table entry 11 = 0x8700A) and the unsigned > 0xB exit:
 * the prologue delta still lands; the pre-dispatch block (timer7b = 4) runs
 * for signed selectors 3..127 and is skipped for 0..2 and the negative bytes
 * 0x80..0xFF. */
static void test_arm_2a_arm11_epilogue(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 11;
  rec.timer89 = 5;
  rec.delta = 3;
  rec.timer7b = 0;
  rec.target.x = 0x111;
  assert(fifa96_arm_2a_step(&rec, 11) == FIFA96_OK);
  assert(rec.timer89 == 8);
  assert(rec.timer7b == 4);
  assert(rec.target.x == 0x111 && rec.stage92 == 11);

  assert(fifa96_arm_2a_step(&rec, 12) == FIFA96_OK);
  assert(rec.timer89 == 11 && rec.timer7b == 4);

  rec.timer7b = 0x77;
  rec.delta = 1;
  assert(fifa96_arm_2a_step(&rec, 0xFF) == FIFA96_OK);
  assert(rec.timer89 == 12);
  assert(rec.timer7b == 0x77);           /* (int8)0xFF = -1: no pre-block */
  assert(rec.stage92 == 11);
  assert(fifa96_arm_2a_step(&rec, 0x80) == FIFA96_OK);
  assert(rec.timer7b == 0x77);
}

/* The prologue adds the zero-extended [0x157A64] delta word (0xFFFF adds
 * 0xFFFF, not -1) and writes nothing else for selectors 0..2. */
static void test_arm_2a_prologue_delta(void) {
  struct fifa96_arm_record rec = arm_rec();
  rec.stage92 = 0;
  rec.timer89 = 5;
  rec.delta = 0xFFFF;
  rec.timer7b = 0x77;
  rec.distance = 0x21;                   /* arm 2 returns at the gate */
  assert(fifa96_arm_2a_step(&rec, 2) == FIFA96_OK);
  assert(rec.timer89 == 5 + 0xFFFF);
  assert(rec.timer7b == 0x77);           /* 2 is not > 2 */
  assert(rec.stage92 == 0);
}

static void test_arm_2a_invalid(void) {
  struct fifa96_arm_record rec = arm_rec();
  assert(fifa96_arm_2a_step(NULL, 0) == ARM_INVALID);
  /* arm 8 inside both gates with a NULL rng aborts at the first draw */
  rec.stage92 = 8;
  rec.distance = 0;
  rec.pos.z = 0;
  rec.rng = NULL;
  assert(fifa96_arm_2a_step(&rec, 8) == ARM_INVALID);
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
  test_arm_29_install_cleared_per_call();
  test_arm_29_invalid();
  test_arm_28_arm3_epilogue_only();
  test_arm_28_arm0_setpiece_target_and_fold();
  test_arm_28_arm0_mode_negates_side0_z();
  test_arm_28_arm0_fold_table_pins();
  test_arm_28_arm0_falls_into_arm1();
  test_arm_28_arm1_flag830_clear_syncs_pos();
  test_arm_28_arm1_flag830_set_scratch_machine();
  test_arm_28_arm1_flag830_set_a1_draw();
  test_arm_28_arm2_aa_gate_vb_table();
  test_arm_28_arm2_chase_cc0_anim15();
  test_arm_28_arm2_chosen_target_offsets();
  test_arm_28_arm2_chosen_missing_bounded();
  test_arm_28_arm2_hard_approach_syncs();
  test_arm_28_arm2_hard_approach_reroll_vc();
  test_arm_28_arm0_row_byte_gate();
  test_arm_28_invalid();
  test_arm_2a_arm0_target_and_clears();
  test_arm_2a_arm1_distance_gate();
  test_arm_2a_arm2_latch();
  test_arm_2a_arm3_timer_gate();
  test_arm_2a_arm4_posz_branches();
  test_arm_2a_arm5_posx_branches();
  test_arm_2a_arm6_posz_subtract();
  test_arm_2a_arm7_posx_subtract();
  test_arm_2a_arm8_rng_target();
  test_arm_2a_arm9_sync_global();
  test_arm_2a_arm10_anim_face_gate();
  test_arm_2a_arm11_epilogue();
  test_arm_2a_prologue_delta();
  test_arm_2a_invalid();
  puts("test_arm_bodies OK");
  return 0;
}
