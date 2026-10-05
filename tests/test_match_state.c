// tests/test_match_state.c — FU-62 match phase/clock state (docs/ghidra/FU62_match_update.md).
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_match_pace.h"
#include "fifa96_loader/fifa96_match_state.h"

static void frames(struct fifa96_match_state *s, int n, int halt, int *ended_last) {
  int ended = 0;
  for (int i = 0; i < n; i++) {
    ended = 0;
    assert(fifa96_match_state_frame(s, FIFA96_MATCH_STATE_STEP, halt, &ended) == 0);
  }
  if (ended_last) *ended_last = ended;
}

static void test_init_zeroes(void) {
  struct fifa96_match_state s;
  s.frame_acc = 1;
  s.frame_delta = 1;
  s.tick_total = 1;
  s.period_seconds = 1;
  s.total_seconds = 1;
  s.aux_seconds = 1;
  s.aux_tick = 1;
  s.period_length = 1;
  s.extra_length = 1;
  s.second_acc = 1;
  s.period = 1;
  s.phase = 1;
  s.prev_phase = 1;
  s.aux_flag = 1;
  fifa96_match_state_init(&s);
  assert(s.frame_acc == 0);
  assert(s.frame_delta == 0);
  assert(s.tick_total == 0);
  assert(s.period_seconds == 0);
  assert(s.total_seconds == 0);
  assert(s.aux_seconds == 0);
  assert(s.aux_tick == 0);
  assert(s.period_length == 0);
  assert(s.extra_length == 0);
  assert(s.second_acc == 0);
  assert(s.period == 0);
  assert(s.phase == 0);
  assert(s.prev_phase == 0);
  assert(s.aux_flag == 0);
}

static void test_phase_class_table(void) {
  assert(fifa96_match_state_phase_class(0x00) == 2);
  assert(fifa96_match_state_phase_class(0x01) == 0);
  assert(fifa96_match_state_phase_class(0x02) == 1);
  assert(fifa96_match_state_phase_class(0x03) == 2);
  assert(fifa96_match_state_phase_class(0x04) == 2);
  assert(fifa96_match_state_phase_class(0x05) == 0);
  assert(fifa96_match_state_phase_class(0x06) == 2);
  assert(fifa96_match_state_phase_class(0x07) == 2);
  assert(fifa96_match_state_phase_class(0x08) == 2);
  assert(fifa96_match_state_phase_class(0x09) == 2);
  assert(fifa96_match_state_phase_class(0x0A) == 0);
  assert(fifa96_match_state_phase_class(0x0B) == 0);
  assert(fifa96_match_state_phase_class(0x0C) == 0);
  assert(fifa96_match_state_phase_class(0x0D) == 2);
  assert(fifa96_match_state_phase_class(0x0E) == 0);
  assert(fifa96_match_state_phase_class(0x13) == 0);
  assert(fifa96_match_state_phase_class(0x14) == 0);
  assert(fifa96_match_state_phase_class(0x15) == 0);
  assert(fifa96_match_state_phase_class(0x17) == 1);
  assert(fifa96_match_state_phase_class(0x1A) == 1);
  assert(fifa96_match_state_phase_class(0x1C) == 1);
  assert(fifa96_match_state_phase_class(0x1D) == 1);
  assert(fifa96_match_state_phase_class(0x1E) == 1);
  assert(fifa96_match_state_phase_class(0x2F) == 0);
  assert(fifa96_match_state_phase_class(0x30) == 0);
  assert(fifa96_match_state_phase_class(0xFF) == 0);
}

static void test_set_phase_saves_previous(void) {
  struct fifa96_match_state s;
  fifa96_match_state_init(&s);
  assert(fifa96_match_state_set_phase(&s, 2) == 0);
  assert(s.phase == 2);
  assert(s.prev_phase == 0);
  assert(fifa96_match_state_set_phase(&s, 0x0A) == 0);
  assert(s.phase == 0x0A);
  assert(s.prev_phase == 2);
}

static void test_step_split_and_totals(void) {
  struct fifa96_match_state s;
  fifa96_match_state_init(&s);
  s.phase = 2;
  s.period_length = 90;
  frames(&s, 1, 0, NULL);
  assert(s.frame_acc == 0);
  assert(s.frame_delta == 2);
  assert(s.tick_total == 2);
  assert(s.second_acc == 2);
  assert(s.period_seconds == 0);
  frames(&s, 29, 0, NULL);
  assert(s.frame_acc == 0);
  assert(s.tick_total == 60);
  assert(s.second_acc == 0);
  assert(s.period_seconds == 1);
  assert(s.total_seconds == 1);
}

static void test_small_step_accumulates(void) {
  struct fifa96_match_state s;
  fifa96_match_state_init(&s);
  s.phase = 2;
  s.period_length = 90;
  for (int i = 0; i < 15; i++) {
    int ended = 0;
    assert(fifa96_match_state_frame(&s, 0x10, 0, &ended) == 0);
  }
  assert(s.frame_delta == 0);
  assert(s.frame_acc == 15 * 0x10u);
  assert(s.tick_total == 0);
  int ended = 0;
  assert(fifa96_match_state_frame(&s, 0x10, 0, &ended) == 0);
  assert(s.frame_delta == 1);
  assert(s.frame_acc == 0);
  assert(s.tick_total == 1);
  assert(s.second_acc == 1);
}

static void test_large_step_rolls_one_second(void) {
  struct fifa96_match_state s;
  fifa96_match_state_init(&s);
  s.phase = 2;
  s.period_length = 90;
  int ended = 0;
  assert(fifa96_match_state_frame(&s, 0x3C00u, 0, &ended) == 0);
  assert(s.frame_delta == 0x3C);
  assert(s.tick_total == 0x3C);
  assert(s.second_acc == 0);
  assert(s.period_seconds == 1);
  assert(ended == 0);
}

static void test_class0_holds_clock(void) {
  struct fifa96_match_state s;
  fifa96_match_state_init(&s);
  s.phase = 1;
  s.period_length = 90;
  frames(&s, 30, 0, NULL);
  assert(s.frame_delta == 2);
  assert(s.tick_total == 60);
  assert(s.second_acc == 0);
  assert(s.period_seconds == 0);
  assert(s.total_seconds == 0);
}

static void test_class2_runs_and_halt_blocks(void) {
  struct fifa96_match_state s;
  fifa96_match_state_init(&s);
  s.phase = 0;
  s.period_length = 90;
  frames(&s, 30, 0, NULL);
  assert(s.period_seconds == 1);
  fifa96_match_state_init(&s);
  s.phase = 0;
  s.period_length = 90;
  frames(&s, 30, 1, NULL);
  assert(s.frame_delta == 2);
  assert(s.tick_total == 60);
  assert(s.second_acc == 0);
  assert(s.period_seconds == 0);
}

static void test_period_end_and_rollover(void) {
  struct fifa96_match_state s;
  int ended = 0;
  fifa96_match_state_init(&s);
  s.phase = 2;
  s.period_length = 1;
  s.extra_length = 1;
  frames(&s, 30, 0, &ended);
  assert(ended == 1);
  assert(s.period == 1);
  assert(s.period_seconds == 0);
  assert(s.total_seconds == 1);
  frames(&s, 30, 0, &ended);
  assert(ended == 1);
  assert(s.period == 2);
  assert(s.period_seconds == 0);
  assert(s.total_seconds == 2);
  frames(&s, 29, 0, &ended);
  assert(ended == 0);
  assert(s.period == 2);
  assert(s.period_seconds == 0);
}

static void test_period_end_with_aux(void) {
  struct fifa96_match_state s;
  int ended = 0;
  fifa96_match_state_init(&s);
  s.phase = 2;
  s.period_length = 1;
  s.aux_seconds = 1;
  frames(&s, 30, 0, &ended);
  assert(ended == 0);
  assert(s.aux_flag == 1);
  assert(s.period == 0);
  assert(s.period_seconds == 1);
  frames(&s, 30, 0, &ended);
  assert(ended == 1);
  assert(s.period == 1);
  assert(s.period_seconds == 0);
  assert(s.total_seconds == 2);
}

static void test_aux_update_on_class2(void) {
  struct fifa96_match_state s;
  fifa96_match_state_init(&s);
  s.phase = 0;
  s.period_length = 90;
  frames(&s, 30, 0, NULL);
  assert(s.aux_seconds == 0);
  assert(s.aux_tick == 1);
  frames(&s, 870, 0, NULL);
  assert(s.aux_seconds == 0);
  assert(s.aux_tick == 30);
  frames(&s, 30, 0, NULL);
  assert(s.aux_seconds == 1);
  assert(s.aux_tick == 30);
}

static void test_advance_period_manual(void) {
  struct fifa96_match_state s;
  fifa96_match_state_init(&s);
  s.period = 3;
  s.period_seconds = 42;
  assert(fifa96_match_state_advance_period(&s) == 0);
  assert(s.period == 4);
  assert(s.period_seconds == 0);
  assert(s.total_seconds == 0);
}

static void test_tick_wiring_pace(void) {
  struct fifa96_match_state s;
  struct fifa96_match_pace p;
  int ended = 0;
  fifa96_match_state_init(&s);
  s.phase = 2;
  s.period_length = 90;
  fifa96_match_pace_init(&p);
  for (int i = 0; i < 3; i++) {
    assert(fifa96_match_state_tick(&s, &p, 0, 0, &ended) == 0);
    assert(ended == 0);
  }
  assert(s.tick_total == 0);
  assert(fifa96_match_state_tick(&s, &p, 0, 0, &ended) == 0);
  assert(s.tick_total == 2);
  assert(s.frame_delta == 2);
}

static void test_null_arguments(void) {
  struct fifa96_match_state s;
  struct fifa96_match_pace p;
  int ended = 0;
  fifa96_match_state_init(NULL);
  assert(fifa96_match_state_set_phase(NULL, 1) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_state_advance_period(NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_state_frame(NULL, FIFA96_MATCH_STATE_STEP, 0, &ended) == -FIFA96_ERR_INVALID);
  fifa96_match_state_init(&s);
  assert(fifa96_match_state_frame(&s, FIFA96_MATCH_STATE_STEP, 0, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_state_frame(&s, 0, 0, &ended) == 0);
  assert(s.frame_delta == 0);
  fifa96_match_pace_init(&p);
  assert(fifa96_match_state_tick(NULL, &p, 0, 0, &ended) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_state_tick(&s, NULL, 0, 0, &ended) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_state_tick(&s, &p, 0, 0, NULL) == -FIFA96_ERR_INVALID);
}

int main(void) {
  test_init_zeroes();
  test_phase_class_table();
  test_set_phase_saves_previous();
  test_step_split_and_totals();
  test_small_step_accumulates();
  test_large_step_rolls_one_second();
  test_class0_holds_clock();
  test_class2_runs_and_halt_blocks();
  test_period_end_and_rollover();
  test_period_end_with_aux();
  test_aux_update_on_class2();
  test_advance_period_manual();
  test_tick_wiring_pace();
  test_null_arguments();
  puts("test_match_state: ok");
  return 0;
}
