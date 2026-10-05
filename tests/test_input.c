// tests/test_input.c — FU-61 match input mapping and edge detection (docs/ghidra/FU61_match_input.md).
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_input.h"

static const uint8_t row_identity[16] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};
static const uint8_t row_swap01[16] = {
  0, 2, 1, 3, 8, 10, 9, 11, 4, 6, 5, 7, 12, 14, 13, 15
};
static const uint8_t row_rot[16] = {
  0, 4, 8, 12, 2, 6, 10, 14, 1, 5, 9, 13, 3, 7, 11, 15
};
static const uint8_t row_rev[16] = {
  0, 8, 4, 12, 1, 9, 5, 13, 2, 10, 6, 14, 3, 11, 7, 15
};

static void test_init_zeroes(void) {
  struct fifa96_input in;
  in.prev[0] = 1;
  in.prev[1] = 1;
  in.prev[2] = 1;
  in.prev[3] = 1;
  in.edge[0] = 1;
  in.held = 1;
  in.fresh = 1;
  in.held_latch = 1;
  in.fresh_latch = 1;
  fifa96_input_init(&in);
  assert(in.prev[0] == 0 && in.prev[1] == 0 && in.prev[2] == 0 && in.prev[3] == 0);
  assert(in.edge[0] == 0 && in.edge[1] == 0 && in.edge[2] == 0 && in.edge[3] == 0);
  assert(in.held == 0);
  assert(in.fresh == 0);
  assert(in.held_latch == 0);
  assert(in.fresh_latch == 0);
}

static void test_map_preserves_high_nibble(void) {
  uint8_t out = 0;
  assert(fifa96_input_map(0x00, row_identity, &out) == 0);
  assert(out == 0x00);
  assert(fifa96_input_map(0x3f, row_identity, &out) == 0);
  assert(out == 0x3f);
  assert(fifa96_input_map(0xff, row_identity, &out) == 0);
  assert(out == 0xff);
  assert(fifa96_input_map(0x05, row_identity, &out) == 0);
  assert(out == 0x05);
}

static void test_map_applies_rows(void) {
  uint8_t out = 0;
  assert(fifa96_input_map(0x01, row_swap01, &out) == 0);
  assert(out == 0x02);
  assert(fifa96_input_map(0x92, row_swap01, &out) == 0);
  assert(out == 0x91);
  assert(fifa96_input_map(0x0f, row_swap01, &out) == 0);
  assert(out == 0x0f);
  assert(fifa96_input_map(0x02, row_rot, &out) == 0);
  assert(out == 0x08);
  assert(fifa96_input_map(0x08, row_rot, &out) == 0);
  assert(out == 0x01);
  assert(fifa96_input_map(0x01, row_rev, &out) == 0);
  assert(out == 0x08);
  assert(fifa96_input_map(0x74, row_rev, &out) == 0);
  assert(out == 0x71);
}

static void test_map_null_arguments(void) {
  uint8_t out = 0x5a;
  assert(fifa96_input_map(0x01, NULL, &out) == -FIFA96_ERR_INVALID);
  assert(out == 0x5a);
  assert(fifa96_input_map(0x01, row_identity, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_input_map(0x01, NULL, NULL) == -FIFA96_ERR_INVALID);
}

static void test_pack_direction_and_button(void) {
  uint8_t one[1] = { 0x03 };
  uint32_t packed = 0xffffffffu;
  assert(fifa96_input_pack(one, 1, &packed) == 0);
  assert(packed == 0u);
  one[0] = 0x05;
  assert(fifa96_input_pack(one, 1, &packed) == 0);
  assert(packed == 1u);
  one[0] = 0x0c;
  assert(fifa96_input_pack(one, 1, &packed) == 0);
  assert(packed == 3u);
  one[0] = 0x10;
  assert(fifa96_input_pack(one, 1, &packed) == 0);
  assert(packed == 4u);
  one[0] = 0x13;
  assert(fifa96_input_pack(one, 1, &packed) == 0);
  assert(packed == 4u);
  one[0] = 0xff;
  assert(fifa96_input_pack(one, 1, &packed) == 0);
  assert(packed == 7u);
}

static void test_pack_multiple_devices(void) {
  uint8_t states[3] = { 0x04, 0x20, 0x07 };
  uint32_t packed = 0;
  assert(fifa96_input_pack(states, 3, &packed) == 0);
  assert(packed == (1u | (4u << 4) | (1u << 8)));
}

static void test_pack_bounds_and_errors(void) {
  uint8_t states[8] = { 0, 0, 0, 0, 0, 0, 0, 0xff };
  uint32_t packed = 0xdeadbeefu;
  assert(fifa96_input_pack(states, 0, &packed) == 0);
  assert(packed == 0u);
  assert(fifa96_input_pack(states, 8, &packed) == 0);
  assert(packed == (7u << 28));
  packed = 0xdeadbeefu;
  assert(fifa96_input_pack(states, 9, &packed) == -FIFA96_ERR_INVALID);
  assert(packed == 0xdeadbeefu);
  assert(fifa96_input_pack(states, -1, &packed) == -FIFA96_ERR_INVALID);
  assert(packed == 0xdeadbeefu);
  assert(fifa96_input_pack(NULL, 1, &packed) == -FIFA96_ERR_INVALID);
  assert(fifa96_input_pack(states, 1, NULL) == -FIFA96_ERR_INVALID);
}

static void test_edge_rising_and_held(void) {
  struct fifa96_input in;
  uint8_t cur[4] = { 0, 0, 0, 0 };
  fifa96_input_init(&in);
  assert(fifa96_input_update(&in, cur) == 0);
  assert(in.fresh == 0 && in.held == 0);
  cur[1] = 0x11;
  assert(fifa96_input_update(&in, cur) == 0);
  assert(in.edge[1] == 0x11);
  assert(in.fresh == 0x11);
  assert(in.held == 0x11);
  assert(in.fresh_latch == 0x11);
  assert(in.held_latch == 0x11);
  assert(fifa96_input_update(&in, cur) == 0);
  assert(in.edge[1] == 0);
  assert(in.fresh == 0);
  assert(in.held == 0x11);
  assert(in.fresh_latch == 0x11);
  assert(in.held_latch == 0x11);
}

static void test_edge_release_and_repress(void) {
  struct fifa96_input in;
  uint8_t cur[4] = { 0x40, 0, 0, 0 };
  fifa96_input_init(&in);
  assert(fifa96_input_update(&in, cur) == 0);
  assert(in.edge[0] == 0x40);
  cur[0] = 0x40;
  assert(fifa96_input_update(&in, cur) == 0);
  assert(in.edge[0] == 0);
  cur[0] = 0;
  assert(fifa96_input_update(&in, cur) == 0);
  assert(in.edge[0] == 0 && in.held == 0);
  cur[0] = 0x40;
  assert(fifa96_input_update(&in, cur) == 0);
  assert(in.edge[0] == 0x40);
}

static void test_edge_nonzero_to_nonzero_suppressed(void) {
  struct fifa96_input in;
  uint8_t cur[4] = { 0x01, 0, 0, 0 };
  fifa96_input_init(&in);
  assert(fifa96_input_update(&in, cur) == 0);
  assert(in.edge[0] == 0x01);
  cur[0] = 0x02;
  assert(fifa96_input_update(&in, cur) == 0);
  assert(in.edge[0] == 0);
  assert(in.held == 0x02);
}

static void test_edge_aggregates_all_players(void) {
  struct fifa96_input in;
  uint8_t cur[4] = { 0x10, 0x20, 0x30, 0 };
  fifa96_input_init(&in);
  assert(fifa96_input_update(&in, cur) == 0);
  assert(in.held == 0x30);
  assert(in.fresh == 0x30);
  assert(in.edge[0] == 0x10 && in.edge[1] == 0x20 && in.edge[2] == 0x30 && in.edge[3] == 0);
}

static void test_update_null(void) {
  struct fifa96_input in;
  uint8_t cur[4] = { 1, 1, 1, 1 };
  fifa96_input_init(&in);
  assert(fifa96_input_update(NULL, cur) == -FIFA96_ERR_INVALID);
  assert(fifa96_input_update(&in, NULL) == -FIFA96_ERR_INVALID);
  assert(in.prev[0] == 0 && in.held == 0 && in.fresh == 0);
}

static void test_event_lookup(void) {
  uint8_t flags[FIFA96_INPUT_CODES];
  for (int i = 0; i < FIFA96_INPUT_CODES; i++) flags[i] = 0;
  flags[2] = 1;
  flags[0x11] = 1;
  assert(fifa96_input_event(flags, 2, 2) == 1);
  assert(fifa96_input_event(flags, 0x11, 2) == 1);
  assert(fifa96_input_event(flags, 5, 2) == 0);
}

static void test_event_gates(void) {
  uint8_t flags[FIFA96_INPUT_CODES];
  for (int i = 0; i < FIFA96_INPUT_CODES; i++) flags[i] = 0;
  flags[1] = 1;
  flags[3] = 1;
  assert(fifa96_input_event(flags, 1, 1) == 0);
  assert(fifa96_input_event(flags, 1, 2) == 1);
  assert(fifa96_input_event(flags, 3, 1) == 0);
  assert(fifa96_input_event(flags, 3, 2) == 1);
}

static void test_event_bounds_and_null(void) {
  uint8_t flags[FIFA96_INPUT_CODES];
  for (int i = 0; i < FIFA96_INPUT_CODES; i++) flags[i] = 1;
  assert(fifa96_input_event(flags, -1, 2) == 0);
  assert(fifa96_input_event(flags, FIFA96_INPUT_CODES, 2) == 0);
  assert(fifa96_input_event(NULL, 2, 2) == -FIFA96_ERR_INVALID);
}

static void test_code_player(void) {
  uint32_t bitmap[FIFA96_INPUT_CODES];
  for (int i = 0; i < FIFA96_INPUT_CODES; i++) bitmap[i] = 0;
  bitmap[4] = 0x5;
  assert(fifa96_input_code_player(bitmap, 4, 2) == 0);
  assert(fifa96_input_code_player(bitmap, 4, 3) == 0);
  bitmap[4] = 0x4;
  assert(fifa96_input_code_player(bitmap, 4, 2) == 0);
  assert(fifa96_input_code_player(bitmap, 4, 3) == 2);
  bitmap[4] = 0x8;
  assert(fifa96_input_code_player(bitmap, 4, 4) == 3);
  assert(fifa96_input_code_player(bitmap, 0, 4) == 0);
  assert(fifa96_input_code_player(bitmap, -1, 4) == 0);
  assert(fifa96_input_code_player(bitmap, FIFA96_INPUT_CODES, 4) == 0);
  assert(fifa96_input_code_player(NULL, 4, 4) == -FIFA96_ERR_INVALID);
}

static void test_lockout(void) {
  uint8_t state = 0x35;
  int frames = 2;
  assert(fifa96_input_lockout(&state, &frames) == 0);
  assert(state == 0);
  assert(frames == 1);
  state = 0x35;
  assert(fifa96_input_lockout(&state, &frames) == 0);
  assert(state == 0);
  assert(frames == 0);
  state = 0x35;
  assert(fifa96_input_lockout(&state, &frames) == 0);
  assert(state == 0x35);
  assert(frames == 0);
  frames = -1;
  state = 0x35;
  assert(fifa96_input_lockout(&state, &frames) == 0);
  assert(state == 0x35);
  assert(frames == -1);
  assert(fifa96_input_lockout(NULL, &frames) == -FIFA96_ERR_INVALID);
  assert(fifa96_input_lockout(&state, NULL) == -FIFA96_ERR_INVALID);
}

int main(void) {
  test_init_zeroes();
  test_map_preserves_high_nibble();
  test_map_applies_rows();
  test_map_null_arguments();
  test_pack_direction_and_button();
  test_pack_multiple_devices();
  test_pack_bounds_and_errors();
  test_edge_rising_and_held();
  test_edge_release_and_repress();
  test_edge_nonzero_to_nonzero_suppressed();
  test_edge_aggregates_all_players();
  test_update_null();
  test_event_lookup();
  test_event_gates();
  test_event_bounds_and_null();
  test_code_player();
  test_lockout();
  puts("test_input: ok");
  return 0;
}
