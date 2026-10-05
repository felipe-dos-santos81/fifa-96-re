#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

typedef struct fifa96_history_record {
  uint8_t type;
  int32_t time;
  const void *entity;
  int32_t camera_x;
  int32_t camera_y;
  int32_t camera_z;
} fifa96_history_record;

typedef int (*fifa96_history_side_fn)(void *user, const void *entity);
typedef int (*fifa96_history_arm_fn)(void *user, const void *entity);

int fifa96_history_reset(fifa96_history_record *records, uint32_t capacity, uint8_t *cursor);
int fifa96_history_push(fifa96_history_record *records, uint32_t capacity, uint8_t *cursor,
                        uint8_t type, int32_t time, const void *entity,
                        int32_t camera_x, int32_t camera_y, int32_t camera_z);
int fifa96_history_get(const fifa96_history_record *records, uint32_t capacity, uint8_t cursor,
                       uint32_t age, fifa96_history_record *out);
int fifa96_history_find(const fifa96_history_record *records, uint32_t capacity, uint8_t cursor,
                        const uint8_t *type_flags, uint32_t type_count, uint8_t mask,
                        int side, uint16_t ordinal,
                        fifa96_history_side_fn side_of, fifa96_history_arm_fn arm_ok,
                        void *user, fifa96_history_record *out);

typedef struct fifa96_stat_slot {
  uint32_t key;
  uint16_t count;
  int32_t value;
} fifa96_stat_slot;

int fifa96_stats_reset(fifa96_stat_slot *slots, uint32_t slot_count);
int fifa96_stats_accumulate(fifa96_stat_slot *slots, uint32_t block_count,
                            uint32_t slots_per_block, uint32_t block, uint32_t key,
                            int32_t delta);
int fifa96_stats_bump(fifa96_stat_slot *slots, uint32_t block_count, uint32_t slots_per_block,
                      uint32_t block, uint32_t key, uint16_t threshold);
int fifa96_stats_get_value(const fifa96_stat_slot *slots, uint32_t block_count,
                           uint32_t slots_per_block, uint32_t block, uint32_t key,
                           int32_t *out_value);
int fifa96_stats_get_count(const fifa96_stat_slot *slots, uint32_t block_count,
                           uint32_t slots_per_block, uint32_t block, uint32_t key,
                           uint16_t *out_count);
int fifa96_stats_argmax(const fifa96_stat_slot *slots, uint32_t block_count,
                        uint32_t slots_per_block, uint32_t *out_key, int32_t *out_value);
int32_t fifa96_stats_weight(int32_t delta, int rank_above, int class_in_range);
