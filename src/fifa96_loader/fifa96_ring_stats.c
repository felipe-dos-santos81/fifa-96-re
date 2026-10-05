#include "fifa96_loader/fifa96_ring_stats.h"
#include <stddef.h>

static fifa96_stat_slot *stats_slot_at(fifa96_stat_slot *slots, uint32_t slots_per_block,
                                       uint32_t block, uint32_t slot) {
  return &slots[(size_t)block * slots_per_block + slot];
}

static const fifa96_stat_slot *stats_const_slot_at(const fifa96_stat_slot *slots,
                                                   uint32_t slots_per_block, uint32_t block,
                                                   uint32_t slot) {
  return &slots[(size_t)block * slots_per_block + slot];
}

static int stats_find(const fifa96_stat_slot *slots, uint32_t slots_per_block, uint32_t block,
                      uint32_t key) {
  uint32_t i;
  for (i = 0; i < slots_per_block; i++) {
    if (stats_const_slot_at(slots, slots_per_block, block, i)->key == key) return (int)i;
  }
  return -1;
}

static int stats_args_ok(const fifa96_stat_slot *slots, uint32_t block_count,
                         uint32_t slots_per_block, uint32_t block) {
  if (!slots) return 0;
  if (block_count == 0 || slots_per_block == 0) return 0;
  if (block >= block_count) return 0;
  return 1;
}

int fifa96_history_reset(fifa96_history_record *records, uint32_t capacity, uint8_t *cursor) {
  uint32_t i;
  if (!records || !cursor) return -FIFA96_ERR_INVALID;
  for (i = 0; i < capacity; i++) {
    records[i].type = 0;
    records[i].time = 0;
    records[i].entity = NULL;
    records[i].camera_x = 0;
    records[i].camera_y = 0;
    records[i].camera_z = 0;
  }
  *cursor = 0;
  return FIFA96_OK;
}

int fifa96_history_push(fifa96_history_record *records, uint32_t capacity, uint8_t *cursor,
                        uint8_t type, int32_t time, const void *entity,
                        int32_t camera_x, int32_t camera_y, int32_t camera_z) {
  uint32_t next;
  if (!records || !cursor || capacity == 0) return -FIFA96_ERR_INVALID;
  next = ((uint32_t)*cursor + 1u) % capacity;
  records[next].type = type;
  records[next].time = time;
  records[next].entity = entity;
  records[next].camera_x = camera_x;
  records[next].camera_y = camera_y;
  records[next].camera_z = camera_z;
  *cursor = (uint8_t)next;
  return (int)next;
}

int fifa96_history_get(const fifa96_history_record *records, uint32_t capacity, uint8_t cursor,
                       uint32_t age, fifa96_history_record *out) {
  uint32_t index;
  if (!records || !out || capacity == 0) return -FIFA96_ERR_INVALID;
  if (age >= capacity) return -FIFA96_ERR_NOT_FOUND;
  index = ((uint32_t)cursor % capacity + capacity - age) % capacity;
  *out = records[index];
  return FIFA96_OK;
}

int fifa96_history_find(const fifa96_history_record *records, uint32_t capacity, uint8_t cursor,
                        const uint8_t *type_flags, uint32_t type_count, uint8_t mask,
                        int side, uint16_t ordinal,
                        fifa96_history_side_fn side_of, fifa96_history_arm_fn arm_ok,
                        void *user, fifa96_history_record *out) {
  uint32_t index;
  uint32_t step;
  uint16_t remaining;
  if (!records || !type_flags || !out || capacity == 0) return -FIFA96_ERR_INVALID;
  if ((side == 0 || side == 1) && !side_of) return -FIFA96_ERR_INVALID;
  remaining = ordinal;
  index = (uint32_t)cursor % capacity;
  for (step = 0; step < capacity; step++) {
    const fifa96_history_record *record = &records[index];
    uint32_t flag = (record->type < type_count) ? type_flags[record->type] : 0u;
    if ((flag & mask) != 0) {
      int accepted = 1;
      if (mask == 4 && (record->type == 0x1Cu || record->type == 0x1Du) && record->entity) {
        if (!arm_ok) return -FIFA96_ERR_INVALID;
        if (!arm_ok(user, record->entity)) accepted = 0;
      }
      if (accepted && (side == 0 || side == 1)) {
        accepted = (record->entity && side_of(user, record->entity) == side);
      }
      if (accepted) {
        remaining = (uint16_t)(remaining - 1u);
        if (remaining == 0) {
          *out = *record;
          return FIFA96_OK;
        }
      }
    }
    index = (index + capacity - 1u) % capacity;
  }
  return -FIFA96_ERR_NOT_FOUND;
}

int fifa96_stats_reset(fifa96_stat_slot *slots, uint32_t slot_count) {
  uint32_t i;
  if (!slots) return -FIFA96_ERR_INVALID;
  for (i = 0; i < slot_count; i++) {
    slots[i].key = 0;
    slots[i].count = 0;
    slots[i].value = 0;
  }
  return FIFA96_OK;
}

int fifa96_stats_accumulate(fifa96_stat_slot *slots, uint32_t block_count,
                            uint32_t slots_per_block, uint32_t block, uint32_t key,
                            int32_t delta) {
  int index;
  fifa96_stat_slot *slot;
  if (!stats_args_ok(slots, block_count, slots_per_block, block)) return -FIFA96_ERR_INVALID;
  index = stats_find(slots, slots_per_block, block, key);
  if (index < 0) return 0;
  slot = stats_slot_at(slots, slots_per_block, block, (uint32_t)index);
  slot->value = (int32_t)((uint32_t)slot->value + (uint32_t)delta);
  return 1;
}

int fifa96_stats_bump(fifa96_stat_slot *slots, uint32_t block_count, uint32_t slots_per_block,
                      uint32_t block, uint32_t key, uint16_t threshold) {
  int index;
  fifa96_stat_slot *slot;
  if (!stats_args_ok(slots, block_count, slots_per_block, block)) return -FIFA96_ERR_INVALID;
  index = stats_find(slots, slots_per_block, block, key);
  if (index < 0) return 0;
  slot = stats_slot_at(slots, slots_per_block, block, (uint32_t)index);
  slot->count = (uint16_t)(slot->count + 1u);
  return (int16_t)slot->count == (int16_t)threshold;
}

int fifa96_stats_get_value(const fifa96_stat_slot *slots, uint32_t block_count,
                           uint32_t slots_per_block, uint32_t block, uint32_t key,
                           int32_t *out_value) {
  int index;
  if (!out_value) return -FIFA96_ERR_INVALID;
  if (!stats_args_ok(slots, block_count, slots_per_block, block)) return -FIFA96_ERR_INVALID;
  index = stats_find(slots, slots_per_block, block, key);
  if (index < 0) return -FIFA96_ERR_NOT_FOUND;
  *out_value = stats_const_slot_at(slots, slots_per_block, block, (uint32_t)index)->value;
  return FIFA96_OK;
}

int fifa96_stats_get_count(const fifa96_stat_slot *slots, uint32_t block_count,
                           uint32_t slots_per_block, uint32_t block, uint32_t key,
                           uint16_t *out_count) {
  int index;
  if (!out_count) return -FIFA96_ERR_INVALID;
  if (!stats_args_ok(slots, block_count, slots_per_block, block)) return -FIFA96_ERR_INVALID;
  index = stats_find(slots, slots_per_block, block, key);
  if (index < 0) return -FIFA96_ERR_NOT_FOUND;
  *out_count = stats_const_slot_at(slots, slots_per_block, block, (uint32_t)index)->count;
  return FIFA96_OK;
}

int fifa96_stats_argmax(const fifa96_stat_slot *slots, uint32_t block_count,
                        uint32_t slots_per_block, uint32_t *out_key, int32_t *out_value) {
  uint32_t block;
  uint32_t slot;
  int best_block = 0;
  int32_t best_value;
  if (!slots || !out_key || !out_value) return -FIFA96_ERR_INVALID;
  if (block_count == 0 || slots_per_block == 0) return -FIFA96_ERR_INVALID;
  best_value = stats_const_slot_at(slots, slots_per_block, 0, 0)->value;
  *out_key = stats_const_slot_at(slots, slots_per_block, 0, 0)->key;
  for (block = 0; block < block_count; block++) {
    for (slot = 0; slot < slots_per_block; slot++) {
      const fifa96_stat_slot *candidate =
          stats_const_slot_at(slots, slots_per_block, block, slot);
      if (candidate->value > best_value) {
        best_value = candidate->value;
        best_block = (int)block;
        *out_key = candidate->key;
      }
    }
  }
  *out_value = best_value;
  return best_block;
}

int32_t fifa96_stats_weight(int32_t delta, int rank_above, int class_in_range) {
  if (!rank_above) return (int32_t)((uint32_t)delta * 2u);
  if (!class_in_range) return delta;
  return (int32_t)((uint32_t)delta + (uint32_t)(delta >> 2));
}
