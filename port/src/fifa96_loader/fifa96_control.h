#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_entity_update.h"

typedef struct fifa96_control_slot {
  int32_t entity;
  uint16_t pressed;
  uint16_t released;
  uint8_t reserved_08[4];
  uint16_t held;
  uint16_t held_prev;
  uint16_t prev_mapped;
  uint16_t raw;
  uint8_t reserved_14[8];
  uint8_t player;
  uint8_t ordinal;
  uint8_t map_select;
  uint8_t anim_a;
  uint8_t anim_b;
  uint8_t anim_c;
  int8_t active;
  uint8_t counter;
} fifa96_control_slot;

typedef struct fifa96_control_candidate {
  uint16_t rank;
  uint8_t skip_98;
  uint8_t skip_9a;
} fifa96_control_candidate;

int fifa96_control_slot_init(fifa96_control_slot *slot, uint8_t player, uint8_t map_select,
                             uint8_t ordinal, int8_t active);
int fifa96_control_slot_update(fifa96_control_slot *slot, uint8_t input, uint8_t delta,
                               const uint8_t map[16], const uint8_t anim_a[16],
                               const uint8_t anim_b[16], const uint8_t anim_c[16]);
/* FU-141: the FUN_00078670 slot clear the slot merge FUN_0007876C calls after
 * binding the requester: zeroes +4/+6 (pressed/released), +8/+0xA (selection
 * scratch), +0xC (held) and the first dword of +0x14, leaving the held/raw
 * history and the player identity intact. NULL -> -FIFA96_ERR_INVALID. */
int fifa96_control_slot_merge_reset(fifa96_control_slot *slot);
int fifa96_control_pick_ranked(const fifa96_control_candidate *candidates, uint32_t count,
                               int16_t skip_index);
int fifa96_control_reselect(uint8_t phase, int controlled_present, int ball_present,
                            const fifa96_entity_candidate *candidates, uint32_t count,
                            int16_t target_x, int16_t target_y, int32_t *controlled_index,
                            int16_t *best_distance);
int fifa96_control_target_bucket(int16_t counter, int16_t t0, int16_t t1);
