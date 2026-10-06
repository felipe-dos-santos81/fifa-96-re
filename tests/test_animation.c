// tests/test_animation.c — FU-84 animation row/driver model
// (docs/ghidra/FU84_animation.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_animation.h"

_Static_assert(offsetof(fifa96_anim_row, index) == 0, "index");
_Static_assert(offsetof(fifa96_anim_row, frame_table) == 8, "frame_table");
_Static_assert(offsetof(fifa96_anim_row, sprite_bank) == 12, "sprite_bank");
_Static_assert(offsetof(fifa96_anim_advance, timer) == 0, "timer");
_Static_assert(offsetof(fifa96_anim_advance, frame_index) == 4, "frame_index");
_Static_assert(offsetof(fifa96_anim_advance, turn) == 5, "turn");

#define ANIM_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static const uint8_t fixture_rows[2 * FIFA96_ANIM_ROW_SIZE] = {
    0, 0, 0x07, 0, 0x00, 0xE2, 0x00, 0x00, 0,
    1, 0x0B, 0x07, 1, 0x06, 0xE2, 0x00, 0x00, 1,
};

static void test_row_lookup(void) {
  fifa96_anim_row row;
  uint8_t rows[FIFA96_ANIM_ROW_COUNT * FIFA96_ANIM_ROW_SIZE];
  uint32_t i;
  for (i = 0; i < FIFA96_ANIM_ROW_COUNT; i++) {
    uint32_t o = i * FIFA96_ANIM_ROW_SIZE;
    uint32_t table = 0x10E200u + i * FIFA96_ANIM_FRAME_SIZE;
    rows[o + 0] = (uint8_t)i;
    rows[o + 1] = (uint8_t)(i & 0x0Fu);
    rows[o + 2] = 0x07u;
    rows[o + 3] = (uint8_t)(i == 0 ? 0 : 1);
    rows[o + 4] = (uint8_t)table;
    rows[o + 5] = (uint8_t)(table >> 8);
    rows[o + 6] = (uint8_t)(table >> 16);
    rows[o + 7] = (uint8_t)(table >> 24);
    rows[o + 8] = (uint8_t)(i & 7u);
  }
  assert(fifa96_animation_row_lookup(rows, FIFA96_ANIM_ROW_COUNT, 0, &row) == FIFA96_OK);
  assert(row.index == 0);
  assert(row.anim_id == 0);
  assert(row.last_frame == 0);
  assert(row.flags == 0x07);
  assert(row.next_id == 0);
  assert(row.frame_table == 0x10E200u);
  assert(row.sprite_bank == 0);
  assert(fifa96_animation_row_lookup(rows, FIFA96_ANIM_ROW_COUNT, 0x0E, &row) == FIFA96_OK);
  assert(row.index == 0x0E);
  assert(row.anim_id == 0x0E);
  assert(row.frame_table == 0x10E200u + 0x0E * 5u);
  assert(row.sprite_bank == 6);
  assert(fifa96_animation_row_lookup(rows, FIFA96_ANIM_ROW_COUNT, 0x6E, &row) == FIFA96_OK);
  assert(row.index == 0x6E);
  assert(row.anim_id == 0x6E);
  assert(fifa96_animation_row_lookup(rows, FIFA96_ANIM_ROW_COUNT, 0x6F, &row) == FIFA96_OK);
  assert(row.index == 0);
  assert(fifa96_animation_row_lookup(rows, FIFA96_ANIM_ROW_COUNT, -1, &row) == FIFA96_OK);
  assert(row.index == 0);
  assert(fifa96_animation_row_lookup(rows, FIFA96_ANIM_ROW_COUNT, 0xFFFF, &row) == FIFA96_OK);
  assert(row.index == 0);
  assert(fifa96_animation_row_lookup(rows, FIFA96_ANIM_ROW_COUNT, 0x7FFF, &row) == FIFA96_OK);
  assert(row.index == 0);
  assert(fifa96_animation_row_lookup(rows, FIFA96_ANIM_ROW_COUNT, 0x1000F, &row) == FIFA96_OK);
  assert(row.index == 0x0F);
  assert(fifa96_animation_row_lookup(rows, 0x6E, 0x6E, &row) == ANIM_INVALID);
  assert(fifa96_animation_row_lookup(fixture_rows, 2, 1, &row) == FIFA96_OK);
  assert(row.index == 1);
  assert(row.anim_id == 1);
  assert(row.last_frame == 0x0B);
  assert(row.flags == 0x07);
  assert(row.next_id == 1);
  assert(row.frame_table == 0xE206u);
  assert(row.sprite_bank == 1);
  assert(fifa96_animation_row_lookup(NULL, 2, 0, &row) == ANIM_INVALID);
  assert(fifa96_animation_row_lookup(fixture_rows, 0, 0, &row) == ANIM_INVALID);
  assert(fifa96_animation_row_lookup(fixture_rows, 2, 0, NULL) == ANIM_INVALID);
}

static void test_flags(void) {
  fifa96_anim_flags flags;
  assert(fifa96_animation_flags(0, &flags) == FIFA96_OK);
  assert(flags.terminal == 0 && flags.bit1 == 0 && flags.bit2 == 0 && flags.lean == 0);
  assert(fifa96_animation_flags(0x01, &flags) == FIFA96_OK);
  assert(flags.terminal == 1 && flags.bit1 == 0 && flags.bit2 == 0 && flags.lean == 0);
  assert(fifa96_animation_flags(0x02, &flags) == FIFA96_OK);
  assert(flags.bit1 == 2);
  assert(fifa96_animation_flags(0x04, &flags) == FIFA96_OK);
  assert(flags.bit2 == 4);
  assert(fifa96_animation_flags(0x07, &flags) == FIFA96_OK);
  assert(flags.terminal == 1 && flags.bit1 == 2 && flags.bit2 == 4);
  assert(fifa96_animation_flags(0x10, &flags) == FIFA96_OK);
  assert(flags.lean == 2);
  assert(fifa96_animation_flags(0x20, &flags) == FIFA96_OK);
  assert(flags.lean == -2);
  assert(fifa96_animation_flags(0x30, &flags) == FIFA96_OK);
  assert(flags.lean == 2);
  assert(fifa96_animation_flags(0xFF, &flags) == FIFA96_OK);
  assert(flags.terminal == 1 && flags.bit1 == 2 && flags.bit2 == 4 && flags.lean == 2);
  assert(fifa96_animation_flags(0, NULL) == ANIM_INVALID);
}

static void test_frame(void) {
  uint8_t frames[3 * FIFA96_ANIM_FRAME_SIZE] = {
      0x40, 0x00, 0x0A, 0x00, 0x00,
      0x80, 0x00, 0x07, 0x00, 0x05,
      0xC0, 0x00, 0x02, 0x03, 0x0C,
  };
  fifa96_anim_frame frame;
  assert(fifa96_animation_frame(frames, 1, 2, &frame) == FIFA96_OK);
  assert(frame.index == 1);
  assert(frame.duration == 0x80);
  assert(frame.aux == 0x0007);
  assert(frame.sprite == 5);
  assert(fifa96_animation_frame(frames, 2, 2, &frame) == FIFA96_OK);
  assert(frame.index == 2);
  assert(frame.duration == 0xC0);
  assert(frame.aux == 0x0302);
  assert(frame.sprite == 0x0C);
  assert(fifa96_animation_frame(frames, -1, 2, &frame) == FIFA96_OK);
  assert(frame.index == 2);
  assert(frame.duration == 0xC0);
  assert(fifa96_animation_frame(frames, 3, 2, &frame) == FIFA96_OK);
  assert(frame.index == 0);
  assert(frame.duration == 0x40);
  assert(fifa96_animation_frame(frames, 0x7FFFFFFF, 2, &frame) == FIFA96_OK);
  assert(frame.index == 0);
  assert(fifa96_animation_frame(frames, 0, 2, NULL) == ANIM_INVALID);
  assert(fifa96_animation_frame(NULL, 0, 2, &frame) == ANIM_INVALID);
}

static void test_advance(void) {
  fifa96_anim_advance state = {0, 1, 0, 1};
  fifa96_anim_advance_out out;
  assert(fifa96_animation_advance(&state, 0x40, 11, &out) == FIFA96_OK);
  assert(out.advanced == 0 && out.frame_index == 0 && out.timer == 0x10);
  assert(fifa96_animation_advance(&state, 0x40, 11, &out) == FIFA96_OK);
  assert(out.advanced == 0 && out.timer == 0x20);
  assert(fifa96_animation_advance(&state, 0x40, 11, &out) == FIFA96_OK);
  assert(out.advanced == 0 && out.timer == 0x30);
  assert(fifa96_animation_advance(&state, 0x40, 11, &out) == FIFA96_OK);
  assert(out.advanced == 0 && out.frame_index == 0 && out.timer == 0x40);
  assert(fifa96_animation_advance(&state, 0x40, 11, &out) == FIFA96_OK);
  assert(out.advanced == 1 && out.frame_index == 1 && out.timer == 0x10);
  state.timer = 0x40;
  state.frame_index = 11;
  state.turn = 1;
  assert(fifa96_animation_advance(&state, 0x40, 11, &out) == FIFA96_OK);
  assert(out.advanced == 1 && out.frame_index == 0 && out.timer == 0x10);
  state.timer = 0x40;
  state.frame_index = 0;
  state.turn = -1;
  assert(fifa96_animation_advance(&state, 0x40, 11, &out) == FIFA96_OK);
  assert(out.advanced == 1 && out.frame_index == 11 && out.timer == 0x10);
  state.timer = 0xFFF0;
  state.delta = 0x10;
  state.frame_index = 5;
  state.turn = 1;
  assert(fifa96_animation_advance(&state, 0xFFFF, 11, &out) == FIFA96_OK);
  assert(out.advanced == 0 && out.timer == 0x00F0);
  state.timer = 0;
  state.delta = 0;
  assert(fifa96_animation_advance(&state, 0, 11, &out) == FIFA96_OK);
  assert(out.advanced == 1 && out.frame_index == 6 && out.timer == 0);
  state.timer = 0;
  assert(fifa96_animation_advance(&state, 0, 11, &out) == FIFA96_OK);
  assert(out.advanced == 1 && out.frame_index == 7);
  assert(fifa96_animation_advance(NULL, 0, 11, &out) == ANIM_INVALID);
  assert(fifa96_animation_advance(&state, 0, 11, NULL) == ANIM_INVALID);
}

static void test_turn(void) {
  int8_t turn = 0;
  assert(fifa96_animation_turn(0x1000, 0x1000, &turn) == FIFA96_OK);
  assert(turn == 1);
  assert(fifa96_animation_turn(0x1000, 0x0F01, &turn) == FIFA96_OK);
  assert(turn == 1);
  assert(fifa96_animation_turn(0x1000, 0x0F00, &turn) == FIFA96_OK);
  assert(turn == -1);
  assert(fifa96_animation_turn(0x1000, 0x0E01, &turn) == FIFA96_OK);
  assert(turn == -1);
  assert(fifa96_animation_turn(0x1000, 0x0DFF, &turn) == FIFA96_OK);
  assert(turn == -1);
  assert(fifa96_animation_turn(0x1000, 0x0D00, &turn) == FIFA96_OK);
  assert(turn == -1);
  assert(fifa96_animation_turn(0x1000, 0x0CFF, &turn) == FIFA96_OK);
  assert(turn == 1);
  assert(fifa96_animation_turn(0x1000, 0x0CB0, &turn) == FIFA96_OK);
  assert(turn == 1);
  assert(fifa96_animation_turn(0x1000, 0x1100, &turn) == FIFA96_OK);
  assert(turn == -1);
  assert(fifa96_animation_turn(0, 0, NULL) == ANIM_INVALID);
}

static void test_successor(void) {
  uint8_t anim_id = 0xAAu;
  uint8_t face = 0xAAu;
  assert(fifa96_animation_successor(FIFA96_ANIM_NEXT_REPEAT, 5, &anim_id, &face) == FIFA96_OK);
  assert(anim_id == 5 && face == 0);
  assert(fifa96_animation_successor(0, 5, &anim_id, &face) == FIFA96_OK);
  assert(anim_id == 0 && face == 0);
  assert(fifa96_animation_successor(3, 5, &anim_id, &face) == FIFA96_OK);
  assert(anim_id == 3 && face == 0);
  assert(fifa96_animation_successor(0x6E, 5, &anim_id, &face) == FIFA96_OK);
  assert(anim_id == 0x6E && face == 0);
  assert(fifa96_animation_successor(0x70, 5, &anim_id, &face) == FIFA96_OK);
  assert(anim_id == 0 && face == 1);
  assert(fifa96_animation_successor(0x7E, 5, &anim_id, &face) == FIFA96_OK);
  assert(anim_id == 0x0E && face == 1);
  assert(fifa96_animation_successor(0x7F, 5, &anim_id, &face) == FIFA96_OK);
  assert(anim_id == 0x0F && face == 1);
  assert(fifa96_animation_successor(0x80, 5, &anim_id, &face) == FIFA96_OK);
  assert(anim_id == 0x10 && face == 1);
  assert(fifa96_animation_successor(0xFF, 5, &anim_id, &face) == FIFA96_OK);
  assert(anim_id == 0x8F && face == 1);
  assert(fifa96_animation_successor(0, 5, NULL, &face) == ANIM_INVALID);
  assert(fifa96_animation_successor(0, 5, &anim_id, NULL) == ANIM_INVALID);
}

int main(void) {
  test_row_lookup();
  test_flags();
  test_frame();
  test_advance();
  test_turn();
  test_successor();
  puts("test_animation: all assertions passed");
  return 0;
}
