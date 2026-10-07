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
  puts("test_arm_bodies OK");
  return 0;
}
