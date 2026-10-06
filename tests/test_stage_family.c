// tests/test_stage_family.c — FU-81 stage/transition model and phase-table
// resolution (docs/ghidra/FU81_stage_family.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_action_handlers.h"

_Static_assert(offsetof(fifa96_action_stage, phase) == 0, "phase");
_Static_assert(offsetof(fifa96_action_stage, stage) == 1, "stage");
_Static_assert(offsetof(fifa96_action_stage, active) == 2, "active");
_Static_assert(offsetof(fifa96_action_stage, occupied) == 3, "occupied");
_Static_assert(offsetof(fifa96_action_stage, timer89) == 4, "timer89");
_Static_assert(offsetof(fifa96_action_stage, delta) == 8, "delta");
_Static_assert(offsetof(fifa96_action_stage_out, allowed) == 0, "allowed");
_Static_assert(offsetof(fifa96_action_stage_out, reset) == 1, "reset");
_Static_assert(offsetof(fifa96_action_stage_out, advance) == 2, "advance");
_Static_assert(offsetof(fifa96_action_stage_out, stage) == 3, "stage");

#define STAGE_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static fifa96_action_stage stage_state(uint8_t phase, uint8_t stage, int32_t timer89,
                                       uint16_t delta, uint8_t occupied) {
  fifa96_action_stage s;
  s.phase = phase;
  s.stage = stage;
  s.active = 1;
  s.occupied = occupied;
  s.timer89 = timer89;
  s.delta = delta;
  return s;
}

static void test_stage_enter(void) {
  static const uint8_t gate1[] = {1};
  static const uint8_t gate2[] = {2, 3};
  static const uint8_t gate3[] = {2, 4};
  fifa96_action_stage s;
  fifa96_action_stage_out out;
  s = stage_state(1, 0, 0, 1, 0);
  assert(fifa96_action_stage_enter(&s, gate1, 1, &out) == FIFA96_OK);
  assert(out.allowed == 1 && out.reset == 0);
  s = stage_state(2, 0, 0, 1, 0);
  assert(fifa96_action_stage_enter(&s, gate1, 1, &out) == FIFA96_OK);
  assert(out.allowed == 0 && out.reset == 1);
  s = stage_state(3, 0, 0, 1, 0);
  assert(fifa96_action_stage_enter(&s, gate2, 2, &out) == FIFA96_OK);
  assert(out.allowed == 1 && out.reset == 0);
  s = stage_state(2, 0, 0, 1, 0);
  assert(fifa96_action_stage_enter(&s, gate3, 2, &out) == FIFA96_OK);
  assert(out.allowed == 1 && out.reset == 0);
  s = stage_state(4, 0, 0, 1, 0);
  assert(fifa96_action_stage_enter(&s, gate3, 2, &out) == FIFA96_OK);
  assert(out.allowed == 1 && out.reset == 0);
  s = stage_state(0, 0, 0, 1, 0);
  assert(fifa96_action_stage_enter(&s, gate2, 2, &out) == FIFA96_OK);
  assert(out.allowed == 0 && out.reset == 1);
  s = stage_state(0x13, 0, 0, 1, 0);
  assert(fifa96_action_stage_enter(&s, gate2, 2, &out) == FIFA96_OK);
  assert(out.allowed == 0 && out.reset == 1);
  assert(fifa96_action_stage_enter(NULL, gate2, 2, &out) == STAGE_INVALID);
  assert(fifa96_action_stage_enter(&s, NULL, 2, &out) == STAGE_INVALID);
  assert(fifa96_action_stage_enter(&s, gate2, 0, &out) == STAGE_INVALID);
  assert(fifa96_action_stage_enter(&s, gate2, 2, NULL) == STAGE_INVALID);
}

static void test_stage_tick(void) {
  fifa96_action_stage s;
  s = stage_state(2, 0, 10, 5, 0);
  assert(fifa96_action_stage_tick(&s) == FIFA96_OK);
  assert(s.timer89 == 15);
  s = stage_state(2, 0, 10, 0, 0);
  assert(fifa96_action_stage_tick(&s) == FIFA96_OK);
  assert(s.timer89 == 10);
  s = stage_state(2, 0, 10, 0xFFF6, 0);
  assert(fifa96_action_stage_tick(&s) == FIFA96_OK);
  assert(s.timer89 == 0x10000);
  s = stage_state(2, 0, -10, 0xFFF6, 0);
  assert(fifa96_action_stage_tick(&s) == FIFA96_OK);
  assert(s.timer89 == 0xFFEC);
  s = stage_state(2, 0, 0x7FFFFFFF, 1, 0);
  assert(fifa96_action_stage_tick(&s) == FIFA96_OK);
  assert(s.timer89 == (int32_t)0x80000000u);
  s = stage_state(2, 0, (int32_t)0x80000000u, 0xFFFF, 0);
  assert(fifa96_action_stage_tick(&s) == FIFA96_OK);
  assert((uint32_t)s.timer89 == 0x8000FFFFu);
  assert(fifa96_action_stage_tick(NULL) == STAGE_INVALID);
}

static void test_stage_advance(void) {
  fifa96_action_stage s;
  fifa96_action_stage_out out;
  s = stage_state(2, 5, 9, 1, 0);
  assert(fifa96_action_stage_advance(&s, &out) == FIFA96_OK);
  assert(s.timer89 == 0 && s.stage == 6);
  assert(out.advance == 1 && out.stage == 6 && out.reset == 0);
  s = stage_state(2, 0xFF, 9, 1, 0);
  assert(fifa96_action_stage_advance(&s, &out) == FIFA96_OK);
  assert(s.stage == 0 && out.stage == 0);
  s = stage_state(2, 0, 9, 1, 0);
  assert(fifa96_action_stage_advance(&s, &out) == FIFA96_OK);
  assert(s.stage == 1 && out.stage == 1);
  assert(fifa96_action_stage_advance(NULL, &out) == STAGE_INVALID);
  assert(fifa96_action_stage_advance(&s, NULL) == STAGE_INVALID);
}

static void test_stage_finish(void) {
  fifa96_action_stage s;
  fifa96_action_stage_out out;
  s = stage_state(2, 4, 5, 1, 1);
  assert(fifa96_action_stage_finish(&s, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.stage == 4 && out.advance == 0);
  s = stage_state(2, 4, 5, 1, 0);
  assert(fifa96_action_stage_finish(&s, &out) == FIFA96_OK);
  assert(out.reset == 0);
  assert(fifa96_action_stage_finish(NULL, &out) == STAGE_INVALID);
  assert(fifa96_action_stage_finish(&s, NULL) == STAGE_INVALID);
}

static void test_stage_marker(void) {
  uint8_t leader, hold;
  assert(fifa96_action_stage_marker(0, &leader, &hold) == FIFA96_OK);
  assert(leader == 1 && hold == 1);
  assert(fifa96_action_stage_marker(2, &leader, &hold) == FIFA96_OK);
  assert(leader == 1 && hold == 1);
  assert(fifa96_action_stage_marker(3, &leader, &hold) == FIFA96_OK);
  assert(leader == 0 && hold == 1);
  assert(fifa96_action_stage_marker(4, &leader, &hold) == FIFA96_OK);
  assert(leader == 0 && hold == 1);
  assert(fifa96_action_stage_marker(5, &leader, &hold) == FIFA96_OK);
  assert(leader == 0 && hold == 0);
  assert(fifa96_action_stage_marker(0x7FFFFFFF, &leader, &hold) == FIFA96_OK);
  assert(leader == 0 && hold == 0);
  assert(fifa96_action_stage_marker(-1, &leader, &hold) == FIFA96_OK);
  assert(leader == 1 && hold == 1);
  assert(fifa96_action_stage_marker(0, NULL, &hold) == STAGE_INVALID);
  assert(fifa96_action_stage_marker(0, &leader, NULL) == STAGE_INVALID);
}

static void test_phase_select(void) {
  static const uint32_t table[3] = {0x05DE34u, 0x05E1D0u, 0x00000000u};
  static const uint32_t one[1] = {0x05DE34u};
  static uint32_t full[0x23];
  uint32_t entry = 0xDEADBEEFu;
  uint32_t i;
  for (i = 0; i < 0x23u; i++) full[i] = 0x05000000u + i;
  full[0x22] = 0x07B900u;
  assert(fifa96_action_phase_select(0, table, 3, &entry) == FIFA96_OK);
  assert(entry == 0x05DE34u);
  assert(fifa96_action_phase_select(1, table, 3, &entry) == FIFA96_OK);
  assert(entry == 0x05E1D0u);
  assert(fifa96_action_phase_select(2, table, 3, &entry) == FIFA96_OK);
  assert(entry == 0);
  assert(fifa96_action_phase_select(3, table, 3, &entry) == STAGE_INVALID);
  assert(fifa96_action_phase_select(0x22, full, 0x23, &entry) == FIFA96_OK);
  assert(entry == 0x07B900u);
  assert(fifa96_action_phase_select(0x23, full, 0x23, &entry) == STAGE_INVALID);
  assert(fifa96_action_phase_select(0, one, 1, &entry) == FIFA96_OK);
  assert(entry == 0x05DE34u);
  assert(fifa96_action_phase_select(1, one, 1, &entry) == STAGE_INVALID);
  assert(fifa96_action_phase_select(0, NULL, 1, &entry) == STAGE_INVALID);
  assert(fifa96_action_phase_select(0, table, 0, &entry) == STAGE_INVALID);
  assert(fifa96_action_phase_select(0, table, 3, NULL) == STAGE_INVALID);
}

int main(void) {
  test_stage_enter();
  test_stage_tick();
  test_stage_advance();
  test_stage_finish();
  test_stage_marker();
  test_phase_select();
  puts("test_stage_family: all assertions passed");
  return 0;
}
