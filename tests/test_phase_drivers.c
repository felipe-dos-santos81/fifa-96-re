// tests/test_phase_drivers.c — FU-83 phase-driver dispatch/install model
// (docs/ghidra/FU83_phase_drivers.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_action_handlers.h"

_Static_assert(offsetof(fifa96_action_phase_record, handler) == 0, "handler");
_Static_assert(offsetof(fifa96_action_phase_record, active) == 4, "active");

#define PHASE_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static uint32_t phase_table[0x23];

static void init_phase_table(void) {
  uint32_t i;
  for (i = 0; i < 0x23u; i++) phase_table[i] = 0x05000000u + i;
  phase_table[0] = 0x05DE34u;
  phase_table[1] = 0x05E1D0u;
  phase_table[0x15] = 0x05E244u;
  phase_table[0x16] = 0u;
  phase_table[0x22] = 0x07B900u;
}

static void test_phase_install(void) {
  fifa96_action_phase_record recs[3];
  uint32_t i;
  for (i = 0; i < 3; i++) {
    recs[i].handler = 0xDEADBEEFu;
    recs[i].active = 0;
  }
  assert(fifa96_action_phase_install(recs, 3, 0, phase_table, 0x23) == FIFA96_OK);
  for (i = 0; i < 3; i++) assert(recs[i].handler == 0x05DE34u);
  assert(fifa96_action_phase_install(recs, 3, 0x15, phase_table, 0x23) == FIFA96_OK);
  for (i = 0; i < 3; i++) assert(recs[i].handler == 0x05E244u);
  assert(fifa96_action_phase_install(recs, 3, 0x16, phase_table, 0x23) == FIFA96_OK);
  for (i = 0; i < 3; i++) assert(recs[i].handler == 0u);
  assert(fifa96_action_phase_install(recs, 3, 0x22, phase_table, 0x23) == FIFA96_OK);
  for (i = 0; i < 3; i++) assert(recs[i].handler == 0x07B900u);
  assert(fifa96_action_phase_install(recs, 3, 0x23, phase_table, 0x23) == PHASE_INVALID);
  for (i = 0; i < 3; i++) assert(recs[i].handler == 0x07B900u);
  assert(fifa96_action_phase_install(NULL, 0, 0, phase_table, 0x23) == FIFA96_OK);
  assert(fifa96_action_phase_install(NULL, 1, 0, phase_table, 0x23) == PHASE_INVALID);
  assert(fifa96_action_phase_install(recs, 3, 0, NULL, 0x23) == PHASE_INVALID);
  assert(fifa96_action_phase_install(recs, 3, 0, phase_table, 0) == PHASE_INVALID);
}

static void test_phase_drive(void) {
  uint8_t drive = 0xAAu;
  assert(fifa96_action_phase_drive(0, &drive) == FIFA96_OK);
  assert(drive == 1u);
  assert(fifa96_action_phase_drive(1, &drive) == FIFA96_OK);
  assert(drive == 0u);
  assert(fifa96_action_phase_drive(0xFF, &drive) == FIFA96_OK);
  assert(drive == 0u);
  assert(fifa96_action_phase_drive(0, NULL) == PHASE_INVALID);
}

static void test_phase_cell(void) {
  fifa96_action_vec3 out = {0x11111111, 0x22222222, 0x33333333};
  assert(fifa96_action_phase_cell(2, 3, 0, &out) == FIFA96_OK);
  assert(out.x == 0x4C);
  assert(out.y == 0);
  assert(out.z == 0x63);
  assert(fifa96_action_phase_cell(2, 3, 1, &out) == FIFA96_OK);
  assert(out.x == -0x4C);
  assert(out.y == 0);
  assert(out.z == -0x63);
  assert(fifa96_action_phase_cell(-2, -3, 0, &out) == FIFA96_OK);
  assert(out.x == -0x4C);
  assert(out.z == -0x63);
  assert(fifa96_action_phase_cell(-2, -3, 1, &out) == FIFA96_OK);
  assert(out.x == 0x4C);
  assert(out.z == 0x63);
  assert(fifa96_action_phase_cell(0, 0, 0, NULL) == PHASE_INVALID);
}

static void test_phase_slot(void) {
  fifa96_action_vec3 out = {0, 0, 0};
  assert(fifa96_action_phase_slot(5, 6, 0, &out) == FIFA96_OK);
  assert(out.x == 5 + (5 >> 2));
  assert(out.y == 0);
  assert(out.z == 6 + (6 >> 2));
  assert(fifa96_action_phase_slot(5, 6, 1, &out) == FIFA96_OK);
  assert(out.x == -(5 + (5 >> 2)));
  assert(out.z == -(6 + (6 >> 2)));
  assert(fifa96_action_phase_slot(-5, -6, 0, &out) == FIFA96_OK);
  assert(out.x == -5 + (-5 >> 2));
  assert(out.z == -6 + (-6 >> 2));
  assert(fifa96_action_phase_slot(-5, -6, 1, &out) == FIFA96_OK);
  assert(out.x == -(-5 + (-5 >> 2)));
  assert(out.z == -(-6 + (-6 >> 2)));
  assert(fifa96_action_phase_slot(0, 0, 0, NULL) == PHASE_INVALID);
}

static void test_phase_ball_entry(void) {
  uint32_t index = 0xDEADBEEFu;
  assert(fifa96_action_phase_ball_entry(1, 0, &index) == FIFA96_OK);
  assert(index == 1u);
  assert(fifa96_action_phase_ball_entry(-1, 0, &index) == FIFA96_OK);
  assert(index == 0u);
  assert(fifa96_action_phase_ball_entry(0, 0, &index) == FIFA96_OK);
  assert(index == 0u);
  assert(fifa96_action_phase_ball_entry(1, 1, &index) == FIFA96_OK);
  assert(index == 0u);
  assert(fifa96_action_phase_ball_entry(-1, 1, &index) == FIFA96_OK);
  assert(index == 1u);
  assert(fifa96_action_phase_ball_entry(0, 1, &index) == FIFA96_OK);
  assert(index == 0u);
  assert(fifa96_action_phase_ball_entry(0, 0, NULL) == PHASE_INVALID);
}

static void test_phase_ball_line(void) {
  static const int16_t entries[4] = {5, 6, 9, 10};
  fifa96_action_vec3 out = {0, 0, 0};
  assert(fifa96_action_phase_ball_line(entries, 1, 0, &out) == FIFA96_OK);
  assert(out.x == 9 + (9 >> 2));
  assert(out.y == 0);
  assert(out.z == 10 + (10 >> 2) - 0x60);
  assert(fifa96_action_phase_ball_line(entries, 1, 1, &out) == FIFA96_OK);
  assert(out.x == -(5 + (5 >> 2)));
  assert(out.z == -(6 + (6 >> 2)) - 0x60);
  assert(fifa96_action_phase_ball_line(entries, -1, 0, &out) == FIFA96_OK);
  assert(out.x == 5 + (5 >> 2));
  assert(out.z == 6 + (6 >> 2) + 0x60);
  assert(fifa96_action_phase_ball_line(entries, -1, 1, &out) == FIFA96_OK);
  assert(out.x == -(9 + (9 >> 2)));
  assert(out.z == -(10 + (10 >> 2)) + 0x60);
  assert(fifa96_action_phase_ball_line(entries, 0, 0, &out) == FIFA96_OK);
  assert(out.x == 5 + (5 >> 2));
  assert(out.z == 6 + (6 >> 2) + 0x60);
  assert(fifa96_action_phase_ball_line(NULL, 1, 0, &out) == PHASE_INVALID);
  assert(fifa96_action_phase_ball_line(entries, 1, 0, NULL) == PHASE_INVALID);
}

static void test_phase_line_timer(void) {
  int32_t timer = 0;
  uint8_t ready = 0xAAu;
  assert(fifa96_action_phase_line_timer(&timer, 0x20, &ready) == FIFA96_OK);
  assert(timer == 0x20);
  assert(ready == 0u);
  assert(fifa96_action_phase_line_timer(&timer, 0x1C, &ready) == FIFA96_OK);
  assert(timer == 0x3C);
  assert(ready == 1u);
  timer = 0xFFFFFFF0;
  assert(fifa96_action_phase_line_timer(&timer, 0x20, &ready) == FIFA96_OK);
  assert(timer == 0x10);
  assert(ready == 0u);
  assert(fifa96_action_phase_line_timer(NULL, 1, &ready) == PHASE_INVALID);
  assert(fifa96_action_phase_line_timer(&timer, 1, NULL) == PHASE_INVALID);
}

static void test_phase_restart_line(void) {
  fifa96_action_vec3 out = {0x11111111, 0x22222222, 0x33333333};
  assert(fifa96_action_phase_restart_line(1, 0x04000000, 0x20, &out) == FIFA96_OK);
  assert(out.x == 0x780);
  assert(out.y == 0x22222222);
  assert(out.z == 10);
  assert(fifa96_action_phase_restart_line(0, 0x04000000, 0x20, &out) == FIFA96_OK);
  assert(out.x == 0x780);
  assert(out.z == -10);
  assert(fifa96_action_phase_restart_line(1, -0x04000000, -0x21, &out) == FIFA96_OK);
  assert(out.x == 0x780);
  assert(out.z == -10);
  assert(fifa96_action_phase_restart_line(1, -0x01800000, 0x20, &out) == FIFA96_OK);
  assert(out.x == 0x780);
  assert(out.z == -5);
  assert(fifa96_action_phase_restart_line(1, -0x01800000, 0, &out) == FIFA96_OK);
  assert(out.x == 0xCC0);
  assert(out.z == 0);
  assert(fifa96_action_phase_restart_line(0, 0x04000000, 0x1F, &out) == FIFA96_OK);
  assert(out.x == 0xCC0);
  assert(out.z == 0);
  assert(fifa96_action_phase_restart_line(0, 0, 0, NULL) == PHASE_INVALID);
}

int main(void) {
  init_phase_table();
  test_phase_install();
  test_phase_drive();
  test_phase_cell();
  test_phase_slot();
  test_phase_ball_entry();
  test_phase_ball_line();
  test_phase_line_timer();
  test_phase_restart_line();
  puts("test_phase_drivers: all assertions passed");
  return 0;
}
