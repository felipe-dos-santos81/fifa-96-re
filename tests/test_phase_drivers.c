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

/* FU-143: the derived phase drivers (docs/ghidra/FU143_phase_rows.md). */

static void test_phase_rows(void) {
  static const uint32_t handlers[0x23] = {
      0x6DE34u, 0x6E1D0u, 0x6DCC8u, 0x6DE44u, 0x6DE44u, 0x6E05Cu, 0x6DD9Cu,
      0x6DE44u, 0x6DD6Cu, 0x6DD6Cu, 0x6DE34u, 0x6DE34u, 0x6DF4Cu, 0x6DE34u,
      0x6DE34u, 0x6DE34u, 0x6E004u, 0x6E1C8u, 0x6E1D0u, 0x6E244u, 0x6E244u,
      0x6DCC8u, 0x000000u, 0x88DC8u, 0x8922Cu, 0x89FA4u, 0x89620u, 0x890ECu,
      0x89110u, 0x89868u, 0x8A798u, 0x88F4Cu, 0x8B688u, 0x8B874u, 0x8B900u};
  static const uint8_t classes[0x23] = {
      2, 0, 1, 2, 2, 0, 2, 2, 2, 2, 0, 0, 0, 2, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0};
  uint32_t i;
  assert(fifa96_action_phase_row(0x23) == NULL);
  for (i = 0; i < 0x23u; i++) {
    const fifa96_action_phase_row_desc *row = fifa96_action_phase_row((uint8_t)i);
    assert(row != NULL);
    assert(row->handler == handlers[i]);
    assert(row->clock_class == classes[i]);
  }
  assert(fifa96_action_phase_row(0x00)->family == FIFA96_ACTION_PHASE_FAMILY_PLACEMENT);
  assert(fifa96_action_phase_row(0x15)->family == FIFA96_ACTION_PHASE_FAMILY_PLACEMENT);
  assert(fifa96_action_phase_row(0x16)->family == FIFA96_ACTION_PHASE_FAMILY_NONE);
  assert(fifa96_action_phase_row(0x16)->handler == 0u);
  assert(fifa96_action_phase_row(0x17)->family == FIFA96_ACTION_PHASE_FAMILY_TIMELINE);
  assert(fifa96_action_phase_row(0x22)->family == FIFA96_ACTION_PHASE_FAMILY_TIMELINE);
}

static void test_phase_act(void) {
  uint8_t phase = 0xAAu;
  assert(fifa96_action_phase_act(0, &phase) == FIFA96_OK);
  assert(phase == 0x16u);
  assert(fifa96_action_phase_act(1, &phase) == FIFA96_OK);
  assert(phase == 0x17u);
  assert(fifa96_action_phase_act(0xA, &phase) == FIFA96_OK);
  assert(phase == 0x20u);
  assert(fifa96_action_phase_act(0xC, &phase) == FIFA96_OK);
  assert(phase == 0x22u);
  assert(fifa96_action_phase_act(0xD, &phase) == PHASE_INVALID);
  assert(phase == 0x22u);
  assert(fifa96_action_phase_act(0, NULL) == PHASE_INVALID);
}

static void test_phase_situations(void) {
  fifa96_action_phase_situation_out out;
  assert(fifa96_action_phase_situation(0x0D, &out) == PHASE_INVALID);
  assert(fifa96_action_phase_situation(0, &out) == FIFA96_OK);
  assert(out.phase == 0x11u && out.act == 0x0Au && out.stage == 0u && out.flags == 0u);
  assert(fifa96_action_phase_situation(1, &out) == FIFA96_OK);
  assert(out.phase == FIFA96_ACTION_PHASE_NONE && out.act == 1u && out.stage == 0u);
  assert(fifa96_action_phase_situation(2, &out) == FIFA96_OK);
  assert(out.phase == 3u && out.act == FIFA96_ACTION_PHASE_NONE);
  assert(fifa96_action_phase_situation(3, &out) == FIFA96_OK);
  assert(out.phase == 4u && out.act == FIFA96_ACTION_PHASE_NONE);
  assert(fifa96_action_phase_situation(4, &out) == FIFA96_OK);
  assert(out.phase == 8u && out.act == FIFA96_ACTION_PHASE_NONE);
  assert(fifa96_action_phase_situation(5, &out) == FIFA96_OK);
  assert(out.phase == 9u && out.act == FIFA96_ACTION_PHASE_NONE);
  assert(fifa96_action_phase_situation(6, &out) == FIFA96_OK);
  assert(out.phase == 5u && out.act == FIFA96_ACTION_PHASE_NONE);
  assert((out.flags & FIFA96_ACTION_PHASE_SITUATION_EXTRA_HOLD) != 0u);
  assert((out.flags & FIFA96_ACTION_PHASE_SITUATION_OPEN_LEG) != 0u);
  assert(fifa96_action_phase_situation(7, &out) == FIFA96_OK);
  assert(out.phase == 0x0Du && out.act == FIFA96_ACTION_PHASE_NONE);
  assert(fifa96_action_phase_situation(8, &out) == FIFA96_OK);
  assert(out.phase == FIFA96_ACTION_PHASE_NONE && out.act == 7u && out.stage == 0u);
  assert((out.flags & FIFA96_ACTION_PHASE_SITUATION_OPEN_LEG) != 0u);
  assert(fifa96_action_phase_situation(9, &out) == FIFA96_OK);
  assert(out.phase == FIFA96_ACTION_PHASE_NONE && out.act == 2u && out.stage == 0u);
  assert(fifa96_action_phase_situation(0xA, &out) == FIFA96_OK);
  assert(out.phase == FIFA96_ACTION_PHASE_NONE && out.act == 2u && out.stage == 1u);
  assert(fifa96_action_phase_situation(0xB, &out) == FIFA96_OK);
  assert(out.phase == 2u && out.act == FIFA96_ACTION_PHASE_NONE && out.stage == 0u);
  assert(fifa96_action_phase_situation(0xC, &out) == FIFA96_OK);
  assert(out.phase == FIFA96_ACTION_PHASE_NONE && out.act == 9u && out.stage == 0u);
  assert(fifa96_action_phase_situation(0, NULL) == PHASE_INVALID);
}

static void test_phase_period_end(void) {
  static const uint8_t p0000[4] = {0, 0, 0, 0};
  static const uint8_t p1000[4] = {1, 0, 0, 0};
  static const uint8_t p0100[4] = {0, 1, 0, 0};
  static const uint8_t p0010[4] = {0, 0, 1, 0};
  static const uint8_t p0001[4] = {0, 0, 0, 1};
  fifa96_action_phase_period_end_out out;
  assert(fifa96_action_phase_period_end(0, 0, 0, 0, 1, 0x10, 0x11, 0, 0, p0000, &out) == FIFA96_OK);
  assert(out.phase == 0x0Cu && out.side == 1u && out.act == 0x0Bu);
  assert(fifa96_action_phase_period_end(1, 0, 3, 1, 0, 0x10, 0x11, 0, 0, p0000, &out) == FIFA96_OK);
  assert(out.phase == 0x0Cu && out.side == 0u && out.act == 0x0Bu);
  assert(fifa96_action_phase_period_end(2, 0, 0, 0, 1, 0, 0, 0, 0, p0000, &out) == FIFA96_OK);
  assert(out.phase == 0x0Cu && out.act == 0x0Bu);
  assert(fifa96_action_phase_period_end(3, 0, 0, 0, 1, 0, 0, 0, 0, p0000, &out) == FIFA96_OK);
  assert(out.phase == 0x0Cu && out.act == 0x0Bu);
  assert(fifa96_action_phase_period_end(4, 0, 1, 0, 1, 0, 0, 0, 0, p0000, &out) == FIFA96_OK);
  assert(out.phase == FIFA96_ACTION_PHASE_NONE && out.act == 0x0Bu);

  assert(fifa96_action_phase_period_end(1, 1, 2, 1, 0, 0x21, 0x22, 0, 0, p0000, &out) == FIFA96_OK);
  assert(out.phase == 0x13u && out.side == 0x21u && out.act == 0x0Cu);
  assert(fifa96_action_phase_period_end(1, 1, 2, 1, 0, 0x21, 0x22, 0, 0, p1000, &out) == FIFA96_OK);
  assert(out.phase == 0x14u && out.side == 0x21u && out.act == 0x0Cu);
  assert(fifa96_action_phase_period_end(1, 1, 1, 2, 0, 0x21, 0x22, 0, 0, p0000, &out) == FIFA96_OK);
  assert(out.phase == 0x13u && out.side == 0x22u);
  assert(fifa96_action_phase_period_end(1, 1, 1, 2, 0, 0x21, 0x22, 0, 0, p0100, &out) == FIFA96_OK);
  assert(out.phase == 0x14u && out.side == 0x22u);
  assert(fifa96_action_phase_period_end(1, 1, 1, 1, 7, 0x21, 0x22, 0, 0, p0000, &out) == FIFA96_OK);
  assert(out.phase == 0x13u && out.side == 7u);
  assert(fifa96_action_phase_period_end(1, 1, 1, 1, 7, 0x21, 0x22, 0, 0, p0010, &out) == FIFA96_OK);
  assert(out.phase == 0x14u && out.side == 0x21u);
  assert(fifa96_action_phase_period_end(1, 1, 1, 1, 7, 0x21, 0x22, 0, 0, p0001, &out) == FIFA96_OK);
  assert(out.phase == 0x14u && out.side == 0x22u);
  assert(fifa96_action_phase_period_end(4, 1, 0, 0, 3, 0x21, 0x22, 5, 5, p0000, &out) == FIFA96_OK);
  assert(out.phase == 0x13u && out.side == 0x22u);
  assert(fifa96_action_phase_period_end(4, 1, 0, 0, 3, 0x21, 0x22, 6, 5, p0000, &out) == FIFA96_OK);
  assert(out.phase == 0x13u && out.side == 0x21u);
  assert(fifa96_action_phase_period_end(4, 1, 0, 0, 3, 0x21, 0x22, -1, 0, p0000, &out) == FIFA96_OK);
  assert(out.phase == 0x13u && out.side == 0x22u);
  assert(fifa96_action_phase_period_end(0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, &out) == PHASE_INVALID);
  assert(fifa96_action_phase_period_end(0, 0, 0, 0, 0, 0, 0, 0, 0, p0000, NULL) == PHASE_INVALID);
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
  test_phase_rows();
  test_phase_act();
  test_phase_situations();
  test_phase_period_end();
  puts("test_phase_drivers: all assertions passed");
  return 0;
}
