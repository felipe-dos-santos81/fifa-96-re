// tests/test_ring_stats.c — FU-72 history ring + stat block port (docs/ghidra/FU72_ring_stats.md).
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_ring_stats.h"

typedef struct test_entity {
  int side;
  int arm_ok;
} test_entity;

static int test_side_of(void *user, const void *entity) {
  (void)user;
  return ((const test_entity *)entity)->side;
}

static int test_arm_ok(void *user, const void *entity) {
  (void)user;
  return ((const test_entity *)entity)->arm_ok;
}

static void test_history_reset(void) {
  fifa96_history_record r[3];
  uint8_t cursor = 9;
  r[0].type = 1;
  r[1].type = 2;
  r[2].type = 3;
  assert(fifa96_history_reset(r, 3, &cursor) == FIFA96_OK);
  assert(cursor == 0);
  assert(r[0].type == 0 && r[1].type == 0 && r[2].type == 0);
  assert(r[0].time == 0 && r[0].entity == NULL && r[0].camera_x == 0);
  assert(fifa96_history_reset(NULL, 3, &cursor) == -FIFA96_ERR_INVALID);
  assert(fifa96_history_reset(r, 3, NULL) == -FIFA96_ERR_INVALID);
}

static void test_history_push_advances(void) {
  fifa96_history_record r[4];
  uint8_t cursor = 0;
  assert(fifa96_history_reset(r, 4, &cursor) == FIFA96_OK);
  assert(fifa96_history_push(r, 4, &cursor, 7, 100, (const void *)0x1234, 11, 12, 13) == 1);
  assert(cursor == 1);
  assert(r[1].type == 7 && r[1].time == 100);
  assert(r[1].entity == (const void *)0x1234);
  assert(r[1].camera_x == 11 && r[1].camera_y == 12 && r[1].camera_z == 13);
  assert(fifa96_history_push(r, 4, &cursor, 8, 200, NULL, 0, 0, 0) == 2);
  assert(cursor == 2);
  assert(fifa96_history_push(r, 4, &cursor, 9, 300, NULL, 0, 0, 0) == 3);
  assert(fifa96_history_push(r, 4, &cursor, 10, 400, NULL, 0, 0, 0) == 0);
  assert(cursor == 0);
  assert(r[0].type == 10 && r[0].time == 400);
  assert(fifa96_history_push(NULL, 4, &cursor, 1, 0, NULL, 0, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_history_push(r, 4, NULL, 1, 0, NULL, 0, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_history_push(r, 0, &cursor, 1, 0, NULL, 0, 0, 0) == -FIFA96_ERR_INVALID);
}

static void test_history_push_wraps_overwrites(void) {
  fifa96_history_record r[3];
  uint8_t cursor = 0;
  int i;
  assert(fifa96_history_reset(r, 3, &cursor) == FIFA96_OK);
  for (i = 0; i < 5; i++) {
    assert(fifa96_history_push(r, 3, &cursor, (uint8_t)(i + 1), i, NULL, 0, 0, 0) ==
           (int)((i + 1) % 3));
  }
  assert(cursor == 2);
  assert(r[0].type == 3 && r[0].time == 2);
  assert(r[1].type == 4 && r[1].time == 3);
  assert(r[2].type == 5 && r[2].time == 4);
}

static void test_history_push_capacity_one(void) {
  fifa96_history_record r[1];
  uint8_t cursor = 0;
  assert(fifa96_history_reset(r, 1, &cursor) == FIFA96_OK);
  assert(fifa96_history_push(r, 1, &cursor, 4, 10, NULL, 0, 0, 0) == 0);
  assert(fifa96_history_push(r, 1, &cursor, 5, 20, NULL, 0, 0, 0) == 0);
  assert(cursor == 0);
  assert(r[0].type == 5 && r[0].time == 20);
}

static void test_history_get_newest_first(void) {
  fifa96_history_record r[3];
  fifa96_history_record out;
  uint8_t cursor = 0;
  assert(fifa96_history_reset(r, 3, &cursor) == FIFA96_OK);
  fifa96_history_push(r, 3, &cursor, 1, 10, NULL, 0, 0, 0);
  fifa96_history_push(r, 3, &cursor, 2, 20, NULL, 0, 0, 0);
  fifa96_history_push(r, 3, &cursor, 3, 30, NULL, 0, 0, 0);
  assert(fifa96_history_get(r, 3, cursor, 0, &out) == FIFA96_OK);
  assert(out.type == 3 && out.time == 30);
  assert(fifa96_history_get(r, 3, cursor, 1, &out) == FIFA96_OK);
  assert(out.type == 2 && out.time == 20);
  assert(fifa96_history_get(r, 3, cursor, 2, &out) == FIFA96_OK);
  assert(out.type == 1 && out.time == 10);
  assert(fifa96_history_get(r, 3, cursor, 3, &out) == -FIFA96_ERR_NOT_FOUND);
  assert(fifa96_history_get(NULL, 3, cursor, 0, &out) == -FIFA96_ERR_INVALID);
  assert(fifa96_history_get(r, 3, cursor, 0, NULL) == -FIFA96_ERR_INVALID);
}

static void test_history_find_mask_and_order(void) {
  fifa96_history_record r[4];
  fifa96_history_record out;
  uint8_t flags[0x28];
  uint8_t cursor = 3;
  uint32_t i;
  for (i = 0; i < 0x28; i++) flags[i] = 0;
  flags[7] = 7;
  flags[0x19] = 3;
  assert(fifa96_history_reset(r, 4, &cursor) == FIFA96_OK);
  r[0].type = 1;
  r[1].type = 7;
  r[2].type = 0x19;
  r[3].type = 7;
  cursor = 3;
  assert(fifa96_history_find(r, 4, cursor, flags, 0x28, 4, 2, 1, NULL, NULL, NULL, &out) ==
         FIFA96_OK);
  assert(out.type == 7);
  assert(fifa96_history_find(r, 4, cursor, flags, 0x28, 4, 2, 2, NULL, NULL, NULL, &out) ==
         FIFA96_OK);
  assert(out.type == 7 && out.time == 0);
  assert(fifa96_history_find(r, 4, cursor, flags, 0x28, 8, 2, 1, NULL, NULL, NULL, &out) ==
         -FIFA96_ERR_NOT_FOUND);
  assert(fifa96_history_find(r, 4, cursor, flags, 0x28, 2, 2, 1, NULL, NULL, NULL, &out) ==
         FIFA96_OK);
  assert(out.type == 7);
  assert(fifa96_history_find(r, 4, cursor, flags, 0x28, 0, 2, 1, NULL, NULL, NULL, &out) ==
         -FIFA96_ERR_NOT_FOUND);
}

static void test_history_find_type_out_of_range(void) {
  fifa96_history_record r[2];
  fifa96_history_record out;
  uint8_t flags[2] = {0, 7};
  uint8_t cursor = 0;
  assert(fifa96_history_reset(r, 2, &cursor) == FIFA96_OK);
  r[0].type = 9;
  cursor = 0;
  assert(fifa96_history_find(r, 2, cursor, flags, 2, 4, 2, 1, NULL, NULL, NULL, &out) ==
         -FIFA96_ERR_NOT_FOUND);
}

static void test_history_find_side_filter(void) {
  fifa96_history_record r[3];
  fifa96_history_record out;
  uint8_t flags[0x28];
  uint8_t cursor = 0;
  test_entity side0 = {0, 1};
  test_entity side1 = {1, 1};
  uint32_t i;
  for (i = 0; i < 0x28; i++) flags[i] = 0;
  flags[7] = 7;
  assert(fifa96_history_reset(r, 3, &cursor) == FIFA96_OK);
  r[0].type = 7;
  r[0].entity = &side1;
  r[1].type = 7;
  r[1].entity = &side0;
  r[2].type = 7;
  r[2].entity = NULL;
  cursor = 2;
  assert(fifa96_history_find(r, 3, cursor, flags, 0x28, 4, 0, 1, test_side_of, NULL, NULL,
                             &out) == FIFA96_OK);
  assert(out.entity == &side0);
  assert(fifa96_history_find(r, 3, cursor, flags, 0x28, 4, 1, 1, test_side_of, NULL, NULL,
                             &out) == FIFA96_OK);
  assert(out.entity == &side1);
  assert(fifa96_history_find(r, 3, cursor, flags, 0x28, 4, 2, 1, test_side_of, NULL, NULL,
                             &out) == FIFA96_OK);
  assert(out.entity == NULL);
  assert(fifa96_history_find(r, 3, cursor, flags, 0x28, 4, 0, 1, NULL, NULL, NULL, &out) ==
         -FIFA96_ERR_INVALID);
}

static void test_history_find_special_arm(void) {
  fifa96_history_record r[4];
  fifa96_history_record out;
  uint8_t flags[0x28];
  uint8_t cursor = 0;
  test_entity reject = {0, 0};
  test_entity accept = {0, 1};
  uint32_t i;
  for (i = 0; i < 0x28; i++) flags[i] = 0;
  flags[0x1C] = 7;
  flags[7] = 7;
  assert(fifa96_history_reset(r, 4, &cursor) == FIFA96_OK);
  r[0].type = 7;
  r[1].type = 0x1C;
  r[1].entity = &reject;
  r[2].type = 0x1C;
  r[2].entity = &accept;
  r[3].type = 0x1C;
  r[3].entity = NULL;
  cursor = 3;
  assert(fifa96_history_find(r, 4, cursor, flags, 0x28, 4, 2, 1, NULL, test_arm_ok, NULL,
                             &out) == FIFA96_OK);
  assert(out.entity == NULL);
  r[3].type = 0;
  assert(fifa96_history_find(r, 4, cursor, flags, 0x28, 4, 2, 1, NULL, test_arm_ok, NULL,
                             &out) == FIFA96_OK);
  assert(out.entity == &accept);
  assert(fifa96_history_find(r, 4, cursor, flags, 0x28, 4, 2, 1, NULL, NULL, NULL, &out) ==
         -FIFA96_ERR_INVALID);
}

static void test_history_find_invalid_and_ordinal_zero(void) {
  fifa96_history_record r[2];
  fifa96_history_record out;
  uint8_t flags[1] = {7};
  uint8_t cursor = 0;
  assert(fifa96_history_reset(r, 2, &cursor) == FIFA96_OK);
  r[0].type = 0;
  cursor = 0;
  assert(fifa96_history_find(NULL, 2, cursor, flags, 1, 1, 2, 1, NULL, NULL, NULL, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_history_find(r, 2, cursor, NULL, 1, 1, 2, 1, NULL, NULL, NULL, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_history_find(r, 2, cursor, flags, 1, 1, 2, 1, NULL, NULL, NULL, NULL) ==
         -FIFA96_ERR_INVALID);
  r[0].type = 1;
  assert(fifa96_history_find(r, 2, cursor, flags, 1, 1, 2, 0, NULL, NULL, NULL, &out) ==
         -FIFA96_ERR_NOT_FOUND);
}

static void test_stats_reset(void) {
  fifa96_stat_slot s[4];
  s[0].key = 9;
  s[0].count = 2;
  s[0].value = 3;
  assert(fifa96_stats_reset(s, 4) == FIFA96_OK);
  assert(s[0].key == 0 && s[0].count == 0 && s[0].value == 0);
  assert(fifa96_stats_reset(NULL, 4) == -FIFA96_ERR_INVALID);
}

static void test_stats_accumulate(void) {
  fifa96_stat_slot s[4] = {{0}};
  int32_t value = 0;
  s[0].key = 0x11;
  s[1].key = 0x22;
  s[2].key = 0x33;
  s[3].key = 0x44;
  assert(fifa96_stats_accumulate(s, 2, 2, 0, 0x22, 50) == 1);
  assert(fifa96_stats_get_value(s, 2, 2, 0, 0x22, &value) == FIFA96_OK);
  assert(value == 50);
  assert(fifa96_stats_accumulate(s, 2, 2, 0, 0x22, -75) == 1);
  assert(fifa96_stats_get_value(s, 2, 2, 0, 0x22, &value) == FIFA96_OK);
  assert(value == -25);
  assert(fifa96_stats_accumulate(s, 2, 2, 1, 0x22, 5) == 0);
  assert(fifa96_stats_get_value(s, 2, 2, 1, 0x22, &value) == -FIFA96_ERR_NOT_FOUND);
  assert(fifa96_stats_get_value(s, 2, 2, 1, 0x33, &value) == FIFA96_OK);
  assert(value == 0);
  assert(fifa96_stats_accumulate(s, 2, 2, 0, 0x99, 5) == 0);
  assert(fifa96_stats_accumulate(NULL, 2, 2, 0, 1, 1) == -FIFA96_ERR_INVALID);
  assert(fifa96_stats_accumulate(s, 2, 2, 2, 1, 1) == -FIFA96_ERR_INVALID);
  assert(fifa96_stats_accumulate(s, 2, 0, 0, 1, 1) == -FIFA96_ERR_INVALID);
}

static void test_stats_accumulate_wraps(void) {
  fifa96_stat_slot s[1] = {{0}};
  int32_t value = 0;
  s[0].key = 1;
  s[0].value = 0x7FFFFFFF;
  assert(fifa96_stats_accumulate(s, 1, 1, 0, 1, 1) == 1);
  assert(fifa96_stats_get_value(s, 1, 1, 0, 1, &value) == FIFA96_OK);
  assert(value == (int32_t)0x80000000);
}

static void test_stats_bump_threshold(void) {
  fifa96_stat_slot s[2] = {{0}};
  uint16_t count = 0;
  s[0].key = 5;
  assert(fifa96_stats_bump(s, 1, 2, 0, 5, 3) == 0);
  assert(fifa96_stats_bump(s, 1, 2, 0, 5, 3) == 0);
  assert(fifa96_stats_bump(s, 1, 2, 0, 5, 3) == 1);
  assert(fifa96_stats_bump(s, 1, 2, 0, 5, 3) == 0);
  assert(fifa96_stats_get_count(s, 1, 2, 0, 5, &count) == FIFA96_OK);
  assert(count == 4);
  assert(fifa96_stats_bump(s, 1, 2, 0, 9, 3) == 0);
  assert(fifa96_stats_bump(NULL, 1, 2, 0, 5, 3) == -FIFA96_ERR_INVALID);
  assert(fifa96_stats_bump(s, 1, 0, 0, 5, 3) == -FIFA96_ERR_INVALID);
  assert(fifa96_stats_bump(s, 1, 2, 1, 5, 3) == -FIFA96_ERR_INVALID);
}

static void test_stats_bump_wraps(void) {
  fifa96_stat_slot s[1] = {{0}};
  uint16_t count = 0;
  s[0].key = 5;
  s[0].count = 0xFFFF;
  assert(fifa96_stats_bump(s, 1, 1, 0, 5, 0) == 1);
  assert(fifa96_stats_get_count(s, 1, 1, 0, 5, &count) == FIFA96_OK);
  assert(count == 0);
}

static void test_stats_accessor_errors(void) {
  fifa96_stat_slot s[1] = {{0}};
  int32_t value = 0;
  uint16_t count = 0;
  s[0].key = 1;
  assert(fifa96_stats_get_value(s, 1, 1, 0, 2, &value) == -FIFA96_ERR_NOT_FOUND);
  assert(fifa96_stats_get_count(s, 1, 1, 0, 2, &count) == -FIFA96_ERR_NOT_FOUND);
  assert(fifa96_stats_get_value(NULL, 1, 1, 0, 1, &value) == -FIFA96_ERR_INVALID);
  assert(fifa96_stats_get_value(s, 1, 1, 0, 1, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_stats_get_value(s, 1, 1, 1, 1, &value) == -FIFA96_ERR_INVALID);
}

static void test_stats_argmax(void) {
  fifa96_stat_slot s[6] = {{0}};
  uint32_t key = 0;
  int32_t value = 0;
  int block;
  s[0].key = 1;
  s[0].value = 10;
  s[1].key = 2;
  s[1].value = 30;
  s[3].key = 3;
  s[3].value = 20;
  block = fifa96_stats_argmax(s, 2, 3, &key, &value);
  assert(block == 0);
  assert(key == 2 && value == 30);
  s[3].value = 30;
  block = fifa96_stats_argmax(s, 2, 3, &key, &value);
  assert(block == 0);
  assert(key == 2 && value == 30);
  s[3].value = 31;
  block = fifa96_stats_argmax(s, 2, 3, &key, &value);
  assert(block == 1);
  assert(key == 3 && value == 31);
  assert(fifa96_stats_argmax(NULL, 2, 3, &key, &value) == -FIFA96_ERR_INVALID);
  assert(fifa96_stats_argmax(s, 0, 3, &key, &value) == -FIFA96_ERR_INVALID);
  assert(fifa96_stats_argmax(s, 2, 0, &key, &value) == -FIFA96_ERR_INVALID);
  assert(fifa96_stats_argmax(s, 2, 3, NULL, &value) == -FIFA96_ERR_INVALID);
  assert(fifa96_stats_argmax(s, 2, 3, &key, NULL) == -FIFA96_ERR_INVALID);
}

static void test_stats_weight(void) {
  assert(fifa96_stats_weight(100, 0, 0) == 200);
  assert(fifa96_stats_weight(100, 1, 0) == 100);
  assert(fifa96_stats_weight(100, 1, 1) == 125);
  assert(fifa96_stats_weight(-100, 0, 0) == -200);
  assert(fifa96_stats_weight(-100, 1, 0) == -100);
  assert(fifa96_stats_weight(-100, 1, 1) == -125);
  assert(fifa96_stats_weight(-3, 1, 1) == -4);
  assert(fifa96_stats_weight(0, 1, 1) == 0);
}

int main(void) {
  test_history_reset();
  test_history_push_advances();
  test_history_push_wraps_overwrites();
  test_history_push_capacity_one();
  test_history_get_newest_first();
  test_history_find_mask_and_order();
  test_history_find_type_out_of_range();
  test_history_find_side_filter();
  test_history_find_special_arm();
  test_history_find_invalid_and_ordinal_zero();
  test_stats_reset();
  test_stats_accumulate();
  test_stats_accumulate_wraps();
  test_stats_bump_threshold();
  test_stats_bump_wraps();
  test_stats_accessor_errors();
  test_stats_argmax();
  test_stats_weight();
  puts("test_ring_stats: ok");
  return 0;
}
