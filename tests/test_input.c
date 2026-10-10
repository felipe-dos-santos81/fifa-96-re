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

/* M2 phase-9 T3 (FU-148 §12.2 / FU-145 §1.8): the FUN_0001C9BC match-init
 * builder for the per-side input/range words [0x14C1D4]/[0x14C1D6]. The
 * eight config dwords are the cells 0x105278/0x10527C/0x105288/0x10528C/
 * 0x105290/0x10529C/0x1052A0/0x1052A4 (image BSS-zero; their only reader is
 * FUN_0001C9BC, no static writer exists). Bit map (first-hand 0x1C9BC):
 *   word0: 0x105278==1 -> 2; ==2 -> |4; 0x10527C!=0 -> |1;
 *          0x105288==1 -> high byte := 2 (|= 0x200);
 *          0x10528C 1/2/3/4 -> |0x20/0x80/0x40/0x8; 0x105290==1 -> |0x10
 *   word1: 0x10529C==1 -> =0x200;
 *          0x1052A0 1/2/3/4 -> low byte |0x20/0x80, |=0x40/0x8; 0x1052A4==1
 *          -> |0x10
 * The consumers derive `input_bit0 = (w0|w1)&1`, the camera interpolation bit
 * `&2` and the pan-step walk gate `&4`. */
static void test_range_words_builder(void) {
  int32_t cells[8];
  uint16_t words[2];
  for (int i = 0; i < 8; i++) cells[i] = 0;
  words[0] = 0xffff;
  words[1] = 0xffff;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[0] == 0 && words[1] == 0);          /* the image default */

  cells[0] = 1;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[0] == 2);                            /* [0x105278]==1 -> bit 1 */
  cells[0] = 2;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[0] == 4);                            /* ==2 -> bit 2 (the walk) */
  cells[0] = 0;
  cells[1] = 7;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[0] == 1);                            /* button != 0 -> bit 0 */
  cells[1] = 0;
  cells[2] = 1;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[0] == 0x200);                        /* high byte := 2 */
  cells[2] = 0;
  cells[3] = 1;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[0] == 0x20);
  cells[3] = 2;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[0] == 0x80);
  cells[3] = 3;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[0] == 0x40);
  cells[3] = 4;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[0] == 8);
  cells[3] = 0;
  cells[4] = 1;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[0] == 0x10);
  cells[4] = 0;

  cells[0] = 2;                                     /* the combined arm */
  cells[1] = 1;
  cells[2] = 1;
  cells[3] = 1;
  cells[4] = 1;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[0] == (uint16_t)(0x200 | 0x20 | 0x10 | 4 | 1));
  cells[0] = cells[1] = cells[2] = cells[3] = cells[4] = 0;

  cells[5] = 1;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[1] == 0x200);
  cells[5] = 0;
  cells[6] = 1;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[1] == 0x20);
  cells[6] = 2;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[1] == 0x80);
  cells[6] = 3;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[1] == 0x40);
  cells[6] = 4;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[1] == 8);
  cells[6] = 0;
  cells[7] = 1;
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert(words[1] == 0x10);

  /* null/untouched contracts */
  words[0] = 0x1234;
  words[1] = 0x5678;
  assert(fifa96_input_range_words(NULL, 0, words) == -FIFA96_ERR_INVALID);
  assert(words[0] == 0x1234 && words[1] == 0x5678);
  assert(fifa96_input_range_words(cells, 0, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_input_range_words(cells, 1, words) == 0);   /* the gate refuses */
  assert(words[0] == 0x1234 && words[1] == 0x5678);
}

/* M2 phase-10 T2 (FU-148 §13.1): the 0x105274 config-block producers. The
 * image block is BSS 0 and the sole static writer is the front-end options
 * editor FUN_0001C728 (`0x1C728..0x1C965`): the selected row i increments
 * `(&0x105274)[i]` and wraps to 0 when the new value reaches the row max
 * `*(int*)(&0x105108 + i*0x1C)`. The 13-entry max table is first-hand
 * `read_memory 0x105108` (364 B): {4,3,2,3,5,2,5,2,3,5,2,5,2}.
 * FUN_0001CAEC (`0x1CAEC..0x1CBBD`) is the read-side query of the same block
 * (used by the team/kit screens). */
static void test_option_producer_max_table(void) {
  static const int32_t maxes[FIFA96_INPUT_OPTION_COUNT] = {
    4, 3, 2, 3, 5, 2, 5, 2, 3, 5, 2, 5, 2
  };
  for (uint32_t i = 0; i < FIFA96_INPUT_OPTION_COUNT; i++)
    assert(fifa96_input_option_max(i) == maxes[i]);
  assert(fifa96_input_option_max(FIFA96_INPUT_OPTION_COUNT) == 0);
}

static void test_option_producer_step_wrap(void) {
  int32_t options[FIFA96_INPUT_OPTION_COUNT];
  for (uint32_t i = 0; i < FIFA96_INPUT_OPTION_COUNT; i++) options[i] = 0;
  /* max 3: 0 -> 1 -> 2 -> wrap 0 (0x1C93A..0x1C94B: INC then max <= next). */
  assert(fifa96_input_option_step(options, 1) == FIFA96_OK);
  assert(options[1] == 1);
  assert(fifa96_input_option_step(options, 1) == FIFA96_OK);
  assert(options[1] == 2);
  assert(fifa96_input_option_step(options, 1) == FIFA96_OK);
  assert(options[1] == 0);
  /* max 2 (index 12): 0 -> 1 -> 0. */
  assert(fifa96_input_option_step(options, 12) == FIFA96_OK);
  assert(options[12] == 1);
  assert(fifa96_input_option_step(options, 12) == FIFA96_OK);
  assert(options[12] == 0);
  /* bounds/NULL. */
  assert(fifa96_input_option_step(options, FIFA96_INPUT_OPTION_COUNT) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_input_option_step(NULL, 0) == -FIFA96_ERR_INVALID);
}

/* The eight cells FUN_0001C9BC consumes are option indices 1/2/5/6/7/10/11/12
 * (0x105278/7C/88/8C/90/9C/A0/A4), in the range-builder's argument order. */
static void test_option_producer_cells_map(void) {
  int32_t options[FIFA96_INPUT_OPTION_COUNT];
  int32_t cells[8];
  uint16_t words[2];
  for (uint32_t i = 0; i < FIFA96_INPUT_OPTION_COUNT; i++) options[i] = 0;
  assert(fifa96_input_options_to_cells(options, cells) == FIFA96_OK);
  for (int i = 0; i < 8; i++) assert(cells[i] == 0);
  options[1] = 2;    /* walk gate (0x105278 == 2 -> word bit 2) */
  options[2] = 1;    /* reflect (0x10527C != 0 -> word bit 0) */
  options[5] = 1;    /* side-0 0x200 (0x105288) */
  options[6] = 4;    /* side-0 nibble 0x8 (0x10528C) */
  options[7] = 1;    /* side-0 0x10 (0x105290) */
  options[10] = 1;   /* side-1 0x200 (0x10529C) */
  options[11] = 3;   /* side-1 nibble 0x40 (0x1052A0) */
  options[12] = 1;   /* side-1 0x10 (0x1052A4) */
  assert(fifa96_input_options_to_cells(options, cells) == FIFA96_OK);
  assert(cells[0] == 2 && cells[1] == 1 && cells[2] == 1 && cells[3] == 4);
  assert(cells[4] == 1 && cells[5] == 1 && cells[6] == 3 && cells[7] == 1);
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert((words[0] & 7u) == 5u);                    /* reflect + walk bits */
  assert((words[0] & 0x200u) != 0 && (words[0] & 8u) != 0 &&
         (words[0] & 0x10u) != 0);
  assert((words[1] & 0x200u) != 0 && (words[1] & 0x40u) != 0 &&
         (words[1] & 0x10u) != 0);
  /* the walk-gate arm alone: option 1 == 1 -> word bit 1 (interpolation). */
  options[1] = 1;
  options[2] = 0;
  assert(fifa96_input_options_to_cells(options, cells) == FIFA96_OK);
  assert(fifa96_input_range_words(cells, 0, words) == 1);
  assert((words[0] & 6u) == 2u);
  assert(fifa96_input_options_to_cells(NULL, cells) == -FIFA96_ERR_INVALID);
  assert(fifa96_input_options_to_cells(options, NULL) == -FIFA96_ERR_INVALID);
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
  test_range_words_builder();
  test_option_producer_max_table();
  test_option_producer_step_wrap();
  test_option_producer_cells_map();
  puts("test_input: ok");
  return 0;
}
