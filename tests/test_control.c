// tests/test_control.c — FU-70 control-slot/selection port (docs/ghidra/FU70_control_slots.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_entity_update.h"
#include "fifa96_loader/fifa96_control.h"

_Static_assert(offsetof(fifa96_control_slot, entity) == 0x00, "entity");
_Static_assert(offsetof(fifa96_control_slot, pressed) == 0x04, "pressed");
_Static_assert(offsetof(fifa96_control_slot, released) == 0x06, "released");
_Static_assert(offsetof(fifa96_control_slot, held) == 0x0C, "held");
_Static_assert(offsetof(fifa96_control_slot, held_prev) == 0x0E, "held_prev");
_Static_assert(offsetof(fifa96_control_slot, prev_mapped) == 0x10, "prev_mapped");
_Static_assert(offsetof(fifa96_control_slot, raw) == 0x12, "raw");
_Static_assert(offsetof(fifa96_control_slot, player) == 0x1C, "player");
_Static_assert(offsetof(fifa96_control_slot, ordinal) == 0x1D, "ordinal");
_Static_assert(offsetof(fifa96_control_slot, map_select) == 0x1E, "map_select");
_Static_assert(offsetof(fifa96_control_slot, anim_a) == 0x1F, "anim_a");
_Static_assert(offsetof(fifa96_control_slot, anim_b) == 0x20, "anim_b");
_Static_assert(offsetof(fifa96_control_slot, anim_c) == 0x21, "anim_c");
_Static_assert(offsetof(fifa96_control_slot, active) == 0x22, "active");
_Static_assert(offsetof(fifa96_control_slot, counter) == 0x23, "counter");

_Static_assert(offsetof(fifa96_control_candidate, rank) == 0x00, "rank");
_Static_assert(offsetof(fifa96_control_candidate, skip_98) == 0x02, "skip_98");
_Static_assert(offsetof(fifa96_control_candidate, skip_9a) == 0x03, "skip_9a");

static const uint8_t identity16[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static const uint8_t anim_a_tbl[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static const uint8_t anim_b_tbl[16] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};
static const uint8_t anim_c_tbl[16] = {30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45};

static fifa96_control_slot fresh_slot(void) {
  fifa96_control_slot slot;
  memset(&slot, 0, sizeof slot);
  slot.entity = -1;
  slot.map_select = 1;
  return slot;
}

static void test_slot_init_fields(void) {
  fifa96_control_slot slot;
  memset(&slot, 0xAA, sizeof slot);
  assert(fifa96_control_slot_init(&slot, 2, 1, 3, 5) == FIFA96_OK);
  assert(slot.entity == -1);
  assert(slot.player == 2);
  assert(slot.map_select == 1);
  assert(slot.ordinal == 3);
  assert(slot.active == 5);
  assert(slot.raw == 0xAAAA);
  assert(slot.prev_mapped == 0xAAAA);
}

static void test_slot_init_invalid(void) {
  assert(fifa96_control_slot_init(NULL, 0, 0, 0, 0) == -FIFA96_ERR_INVALID);
}

static void test_slot_update_passes_raw_when_map_select(void) {
  fifa96_control_slot slot = fresh_slot();
  assert(fifa96_control_slot_update(&slot, 0x5A, 0, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.raw == 0x5A);
  assert(slot.prev_mapped == 0x5A);
  assert(slot.pressed == 0x5A);
  assert(slot.released == 0);
  assert(slot.anim_a == 10);
  assert(slot.anim_b == 20);
  assert(slot.anim_c == 40);
}

static void test_slot_update_maps_direction_nibble(void) {
  fifa96_control_slot slot = fresh_slot();
  uint8_t map[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 3, 11, 12, 13, 14, 15};
  slot.map_select = 0;
  assert(fifa96_control_slot_update(&slot, 0x5A, 0, map, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.raw == 0x5A);
  assert(slot.prev_mapped == 0x53);
  assert(slot.anim_a == 3);
  assert(slot.anim_b == 13);
  assert(slot.anim_c == 33);
}

static void test_slot_update_identity_map(void) {
  fifa96_control_slot slot = fresh_slot();
  slot.map_select = 0;
  assert(fifa96_control_slot_update(&slot, 0x0F, 0, identity16, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.prev_mapped == 0x0F);
  assert(slot.anim_a == 15);
}

static void test_slot_update_press_release(void) {
  fifa96_control_slot slot = fresh_slot();
  assert(fifa96_control_slot_update(&slot, 0x10, 0, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.pressed == 0x10);
  assert(slot.released == 0);
  assert(slot.held == 0);
  assert(fifa96_control_slot_update(&slot, 0x00, 0, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.pressed == 0);
  assert(slot.released == 0x10);
}

static void test_slot_update_hold_machine(void) {
  fifa96_control_slot slot = fresh_slot();
  assert(fifa96_control_slot_update(&slot, 0x20, 0, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(fifa96_control_slot_update(&slot, 0x41, 0, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.pressed == 0x41);
  assert(slot.released == 0);
  assert(slot.held == 0x41);
  assert(slot.held_prev == 0x20);
  assert(fifa96_control_slot_update(&slot, 0x01, 0, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.held == 0x01);
  assert(slot.released == 0);
  assert(fifa96_control_slot_update(&slot, 0x00, 0, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.held == 0);
  assert(slot.released == 0x20);
}

static void test_slot_update_hold_prev_reports_previous_buttons(void) {
  fifa96_control_slot slot = fresh_slot();
  assert(fifa96_control_slot_update(&slot, 0x30, 0, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(fifa96_control_slot_update(&slot, 0x40, 0, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.held == 0x40);
  assert(slot.held_prev == 0x30);
  assert(slot.released == 0);
  assert(fifa96_control_slot_update(&slot, 0x00, 0, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.released == 0x30);
}

static void test_slot_update_counter_accumulates_and_resets(void) {
  fifa96_control_slot slot = fresh_slot();
  assert(fifa96_control_slot_update(&slot, 0x01, 5, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.counter == 5);
  assert(fifa96_control_slot_update(&slot, 0x01, 5, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.counter == 10);
  assert(fifa96_control_slot_update(&slot, 0x00, 5, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.counter == 0);
}

static void test_slot_update_counter_not_reset_on_release(void) {
  fifa96_control_slot slot = fresh_slot();
  assert(fifa96_control_slot_update(&slot, 0x20, 7, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.counter == 7);
  assert(fifa96_control_slot_update(&slot, 0x00, 3, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.released == 0x20);
  assert(slot.counter == 7);
}

static void test_slot_update_counter_cap_and_wrap(void) {
  fifa96_control_slot slot = fresh_slot();
  slot.counter = 0xF9;
  assert(fifa96_control_slot_update(&slot, 0x01, 0x10, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.counter == 0x09);
  slot.counter = 0xFA;
  assert(fifa96_control_slot_update(&slot, 0x01, 5, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
  assert(slot.counter == 0xFA);
}

static void test_slot_update_counter_adds_delta_low_byte(void) {
  fifa96_control_slot slot = fresh_slot();
  assert(fifa96_control_slot_update(&slot, 0x01, (uint8_t)0x100, NULL, anim_a_tbl, anim_b_tbl,
                                    anim_c_tbl) == FIFA96_OK);
  assert(slot.counter == 0);
  assert(fifa96_control_slot_update(&slot, 0x01, (uint8_t)0x1FF, NULL, anim_a_tbl, anim_b_tbl,
                                    anim_c_tbl) == FIFA96_OK);
  assert(slot.counter == 0xFF);
}

static void test_slot_update_invalid_arguments(void) {
  fifa96_control_slot slot = fresh_slot();
  slot.prev_mapped = 0x1234;
  assert(fifa96_control_slot_update(NULL, 0, 0, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == -FIFA96_ERR_INVALID);
  assert(fifa96_control_slot_update(&slot, 0, 0, NULL, NULL, anim_b_tbl, anim_c_tbl) == -FIFA96_ERR_INVALID);
  assert(fifa96_control_slot_update(&slot, 0, 0, NULL, anim_a_tbl, NULL, anim_c_tbl) == -FIFA96_ERR_INVALID);
  assert(fifa96_control_slot_update(&slot, 0, 0, NULL, anim_a_tbl, anim_b_tbl, NULL) == -FIFA96_ERR_INVALID);
  slot.map_select = 0;
  assert(fifa96_control_slot_update(&slot, 0, 0, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == -FIFA96_ERR_INVALID);
  assert(slot.prev_mapped == 0x1234);
  slot.map_select = 1;
  assert(fifa96_control_slot_update(&slot, 0x01, 0, NULL, anim_a_tbl, anim_b_tbl, anim_c_tbl) == FIFA96_OK);
}

static void test_pick_ranked_minimum(void) {
  const fifa96_control_candidate c[3] = {{5, 0, 0}, {2, 0, 0}, {9, 0, 0}};
  assert(fifa96_control_pick_ranked(c, 3, -1) == 1);
}

static void test_pick_ranked_skip_index(void) {
  const fifa96_control_candidate c[3] = {{5, 0, 0}, {2, 0, 0}, {9, 0, 0}};
  assert(fifa96_control_pick_ranked(c, 3, 1) == 0);
  assert(fifa96_control_pick_ranked(c, 3, 0) == 1);
}

static void test_pick_ranked_exclusions(void) {
  const fifa96_control_candidate c[3] = {{2, 1, 0}, {5, 0, 0}, {9, 0, 1}};
  assert(fifa96_control_pick_ranked(c, 3, -1) == 1);
  const fifa96_control_candidate d[1] = {{2, 0, 1}};
  assert(fifa96_control_pick_ranked(d, 1, -1) == -1);
}

static void test_pick_ranked_first_wins_tie(void) {
  const fifa96_control_candidate c[2] = {{3, 0, 0}, {3, 0, 0}};
  assert(fifa96_control_pick_ranked(c, 2, -1) == 0);
}

static void test_pick_ranked_word_max_not_selectable(void) {
  const fifa96_control_candidate c[2] = {{0xFFFF, 0, 0}, {0xFFFF, 0, 0}};
  assert(fifa96_control_pick_ranked(c, 2, -1) == -1);
}

static void test_pick_ranked_invalid_and_empty(void) {
  const fifa96_control_candidate c[1] = {{1, 0, 0}};
  assert(fifa96_control_pick_ranked(NULL, 1, -1) == -FIFA96_ERR_INVALID);
  assert(fifa96_control_pick_ranked(c, 0, -1) == -1);
}

static void test_reselect_keeps_when_controlled_and_ball_present(void) {
  const fifa96_entity_candidate c[2] = {{0, 0, 0, 0}, {100, 0, 0, 0}};
  int32_t index = -77;
  int16_t distance = -77;
  assert(fifa96_control_reselect(2, 1, 1, c, 2, 0, 0, &index, &distance) == 0);
  assert(index == -77);
  assert(distance == -77);
}

static void test_reselect_phase_gate(void) {
  const fifa96_entity_candidate c[2] = {{0, 0, 0, 0}, {100, 0, 0, 0}};
  int32_t index = -77;
  int16_t distance = -77;
  assert(fifa96_control_reselect(0, 0, 0, c, 2, 0, 0, &index, &distance) == 0);
  assert(fifa96_control_reselect(1, 0, 0, c, 2, 0, 0, &index, &distance) == 0);
  assert(fifa96_control_reselect(0x10, 0, 0, c, 2, 0, 0, &index, &distance) == 0);
  assert(index == -77);
  assert(distance == -77);
}

static void test_reselect_skips_record_zero(void) {
  const fifa96_entity_candidate c[2] = {{0, 0, 0, 0}, {100, 0, 0, 0}};
  int32_t index = -77;
  int16_t distance = -77;
  assert(fifa96_control_reselect(2, 0, 1, c, 2, 0, 0, &index, &distance) == 1);
  assert(index == 1);
  assert(distance == 100);
}

static void test_reselect_runs_when_either_missing(void) {
  const fifa96_entity_candidate c[2] = {{0, 0, 0, 0}, {100, 0, 0, 0}};
  int32_t index = -77;
  int16_t distance = -77;
  assert(fifa96_control_reselect(2, 1, 0, c, 2, 0, 0, &index, &distance) == 1);
  assert(index == 1);
  index = -77;
  assert(fifa96_control_reselect(2, 0, 0, c, 2, 0, 0, &index, &distance) == 1);
  assert(index == 1);
}

static void test_reselect_none_eligible(void) {
  fifa96_entity_candidate c[2] = {{0, 0, 0, 0}, {100, 0, 1, 0}};
  int32_t index = -77;
  int16_t distance = -77;
  assert(fifa96_control_reselect(2, 0, 1, c, 2, 0, 0, &index, &distance) == 1);
  assert(index == -1);
  assert(distance == (int16_t)0xFFFF);
}

static void test_reselect_count_zero(void) {
  const fifa96_entity_candidate c[1] = {{0, 0, 0, 0}};
  int32_t index = -77;
  int16_t distance = -77;
  assert(fifa96_control_reselect(2, 0, 1, c, 0, 0, 0, &index, &distance) == 1);
  assert(index == -1);
  assert(distance == (int16_t)0xFFFF);
}

static void test_reselect_target_word_wrap(void) {
  const fifa96_entity_candidate c[2] = {{0, 0, 0, 0}, {32767, 0, 0, 0}};
  int32_t index = -77;
  int16_t distance = -77;
  assert(fifa96_control_reselect(2, 0, 1, c, 2, -32768, 0, &index, &distance) == 1);
  assert(index == 1);
  assert(distance == 1);
}

static void test_reselect_invalid_arguments(void) {
  const fifa96_entity_candidate c[1] = {{0, 0, 0, 0}};
  int32_t index = -77;
  int16_t distance = -77;
  assert(fifa96_control_reselect(2, 0, 1, c, 1, 0, 0, NULL, &distance) == -FIFA96_ERR_INVALID);
  assert(fifa96_control_reselect(2, 0, 1, c, 1, 0, 0, &index, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_control_reselect(2, 0, 1, NULL, 1, 0, 0, &index, &distance) == -FIFA96_ERR_INVALID);
}

static void test_target_bucket_boundaries(void) {
  assert(fifa96_control_target_bucket(-1, 0, 10) == 0);
  assert(fifa96_control_target_bucket(0, 0, 10) == 1);
  assert(fifa96_control_target_bucket(9, 0, 10) == 1);
  assert(fifa96_control_target_bucket(10, 0, 10) == 2);
  assert(fifa96_control_target_bucket(100, 0, 10) == 2);
  assert(fifa96_control_target_bucket(-32768, -32767, 0) == 0);
  assert(fifa96_control_target_bucket(32767, 0, -1) == 2);
}

int main(void) {
  test_slot_init_fields();
  test_slot_init_invalid();
  test_slot_update_passes_raw_when_map_select();
  test_slot_update_maps_direction_nibble();
  test_slot_update_identity_map();
  test_slot_update_press_release();
  test_slot_update_hold_machine();
  test_slot_update_hold_prev_reports_previous_buttons();
  test_slot_update_counter_accumulates_and_resets();
  test_slot_update_counter_not_reset_on_release();
  test_slot_update_counter_cap_and_wrap();
  test_slot_update_counter_adds_delta_low_byte();
  test_slot_update_invalid_arguments();
  test_pick_ranked_minimum();
  test_pick_ranked_skip_index();
  test_pick_ranked_exclusions();
  test_pick_ranked_first_wins_tie();
  test_pick_ranked_word_max_not_selectable();
  test_pick_ranked_invalid_and_empty();
  test_reselect_keeps_when_controlled_and_ball_present();
  test_reselect_phase_gate();
  test_reselect_skips_record_zero();
  test_reselect_runs_when_either_missing();
  test_reselect_none_eligible();
  test_reselect_count_zero();
  test_reselect_target_word_wrap();
  test_reselect_invalid_arguments();
  test_target_bucket_boundaries();
  puts("test_control: ok");
  return 0;
}
