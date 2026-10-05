// tests/test_competition_gate.c — FU-65 competition screen gate (docs/ghidra/FU65_competition_screens.md).
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_competition_gate.h"

static void test_init_zeroes(void) {
  struct fifa96_competition_gate gate;
  gate.one_shot = 7;
  gate.last_input = 7;
  gate.confirm = 7;
  fifa96_competition_gate_init(&gate);
  assert(gate.one_shot == 0);
  assert(gate.last_input == 0);
  assert(gate.confirm == 0);
}

static void test_initial_one_shot_state6(void) {
  struct fifa96_competition_gate gate;
  uint32_t state = 0xdeadbeefu;
  fifa96_competition_gate_init(&gate);
  gate.one_shot = 1;
  assert(fifa96_competition_gate_initial(&gate, &state) == 1);
  assert(state == 6);
  assert(gate.one_shot == 0);
  assert(fifa96_competition_gate_initial(&gate, &state) == 1);
  assert(state == 9);
}

static void test_initial_last_input_selects_8_or_9(void) {
  struct fifa96_competition_gate gate;
  uint32_t state = 0;
  fifa96_competition_gate_init(&gate);
  gate.last_input = 1;
  assert(fifa96_competition_gate_initial(&gate, &state) == 1);
  assert(state == 8);
  gate.last_input = 2;
  assert(fifa96_competition_gate_initial(&gate, &state) == 1);
  assert(state == 9);
  gate.last_input = 0xFFFFFFFFu;
  assert(fifa96_competition_gate_initial(&gate, &state) == 1);
  assert(state == 9);
}

static void test_after_input_mapping(void) {
  struct fifa96_competition_gate gate;
  uint32_t state = 0;
  fifa96_competition_gate_init(&gate);
  assert(fifa96_competition_gate_after_input(&gate, 2, &state) == 1);
  assert(state == 7);
  assert(gate.last_input == 2);
  state = 0;
  assert(fifa96_competition_gate_after_input(&gate, 0, &state) == 1);
  assert(state == 8);
  assert(gate.last_input == 0);
  state = 0;
  assert(fifa96_competition_gate_after_input(&gate, 1, &state) == 1);
  assert(state == 9);
  state = 0;
  assert(fifa96_competition_gate_after_input(&gate, -1, &state) == 1);
  assert(state == 9);
  state = 0xdeadbeefu;
  assert(fifa96_competition_gate_after_input(&gate, INT32_MIN, &state) == 1);
  assert(state == 9);
  assert(gate.last_input == 0x80000000u);
}

static void test_after_input_confirm_suppresses_dispatch(void) {
  struct fifa96_competition_gate gate;
  uint32_t state = 0xdeadbeefu;
  fifa96_competition_gate_init(&gate);
  gate.confirm = 1;
  assert(fifa96_competition_gate_after_input(&gate, 2, &state) == 0);
  assert(state == 0xdeadbeefu);
  assert(gate.last_input == 2);
}

static void test_after_state10(void) {
  struct fifa96_competition_gate gate;
  uint32_t state = 0xdeadbeefu;
  fifa96_competition_gate_init(&gate);
  assert(fifa96_competition_gate_after_state10(&gate, &state) == 1);
  assert(state == 11);
  state = 0xdeadbeefu;
  gate.confirm = 1;
  assert(fifa96_competition_gate_after_state10(&gate, &state) == 0);
  assert(state == 0xdeadbeefu);
}

static void test_initial_ignores_confirm(void) {
  struct fifa96_competition_gate gate;
  uint32_t state = 0;
  fifa96_competition_gate_init(&gate);
  gate.confirm = 1;
  gate.one_shot = 1;
  assert(fifa96_competition_gate_initial(&gate, &state) == 1);
  assert(state == 6);
  assert(gate.one_shot == 0);
}

static void test_sequence_matches_original_flow(void) {
  struct fifa96_competition_gate gate;
  uint32_t state = 0;
  fifa96_competition_gate_init(&gate);
  gate.one_shot = 1;
  assert(fifa96_competition_gate_initial(&gate, &state) == 1);
  assert(state == 6);
  assert(fifa96_competition_gate_after_input(&gate, 2, &state) == 1);
  assert(state == 7);
  assert(fifa96_competition_gate_initial(&gate, &state) == 1);
  assert(state == 9);
  assert(fifa96_competition_gate_after_input(&gate, 0, &state) == 1);
  assert(state == 8);
  assert(fifa96_competition_gate_after_input(&gate, 1, &state) == 1);
  assert(state == 9);
  assert(fifa96_competition_gate_after_state10(&gate, &state) == 1);
  assert(state == 11);
}

static void test_null_arguments(void) {
  struct fifa96_competition_gate gate;
  uint32_t state = 0xdeadbeefu;
  fifa96_competition_gate_init(&gate);
  fifa96_competition_gate_init(NULL);
  assert(fifa96_competition_gate_initial(NULL, &state) == -FIFA96_ERR_INVALID);
  assert(fifa96_competition_gate_initial(&gate, NULL) == -FIFA96_ERR_INVALID);
  assert(state == 0xdeadbeefu);
  assert(fifa96_competition_gate_after_input(NULL, 2, &state) == -FIFA96_ERR_INVALID);
  assert(fifa96_competition_gate_after_input(&gate, 2, NULL) == -FIFA96_ERR_INVALID);
  assert(state == 0xdeadbeefu);
  assert(gate.last_input == 0);
  assert(fifa96_competition_gate_after_state10(NULL, &state) == -FIFA96_ERR_INVALID);
  assert(fifa96_competition_gate_after_state10(&gate, NULL) == -FIFA96_ERR_INVALID);
}

int main(void) {
  test_init_zeroes();
  test_initial_one_shot_state6();
  test_initial_last_input_selects_8_or_9();
  test_after_input_mapping();
  test_after_input_confirm_suppresses_dispatch();
  test_after_state10();
  test_initial_ignores_confirm();
  test_sequence_matches_original_flow();
  test_null_arguments();
  puts("test_competition_gate: ok");
  return 0;
}
