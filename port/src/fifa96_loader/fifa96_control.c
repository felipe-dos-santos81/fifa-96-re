#include "fifa96_loader/fifa96_control.h"
#include "fifa96_loader/fifa96_entity_update.h"

int fifa96_control_slot_init(fifa96_control_slot *slot, uint8_t player, uint8_t map_select,
                             uint8_t ordinal, int8_t active) {
  if (!slot) return -FIFA96_ERR_INVALID;
  slot->entity = -1;
  slot->player = player;
  slot->map_select = map_select;
  slot->ordinal = ordinal;
  slot->active = active;
  return FIFA96_OK;
}

int fifa96_control_slot_update(fifa96_control_slot *slot, uint8_t input, uint8_t delta,
                               const uint8_t map[16], const uint8_t anim_a[16],
                               const uint8_t anim_b[16], const uint8_t anim_c[16]) {
  uint16_t mapped;
  uint16_t prev;
  uint16_t rising;
  uint16_t falling;
  uint8_t direction;
  uint8_t index;
  if (!slot || !anim_a || !anim_b || !anim_c) return -FIFA96_ERR_INVALID;
  if (slot->map_select == 0 && !map) return -FIFA96_ERR_INVALID;
  slot->raw = input;
  if (slot->map_select == 0) {
    mapped = (uint16_t)((input & 0xF0u) | map[input & 0x0Fu]);
  } else {
    mapped = input;
  }
  prev = slot->prev_mapped;
  rising = (uint16_t)((mapped ^ prev) & mapped);
  falling = (uint16_t)((mapped ^ prev) & prev);
  slot->pressed = rising;
  slot->released = 0;
  slot->prev_mapped = mapped;
  if (falling != 0) {
    if (slot->held != 0) {
      slot->held = (uint16_t)(slot->held & ~falling);
      if (slot->held == 0) slot->released = slot->held_prev;
    } else if (mapped != 0) {
      slot->held = mapped;
      slot->held_prev = (uint16_t)(prev & 0xFF0u);
    } else {
      slot->released = (uint16_t)(prev & 0xFF0u);
    }
  }
  if (mapped != 0) {
    if (slot->counter < 0xFAu) slot->counter = (uint8_t)(slot->counter + delta);
  } else if (slot->released == 0) {
    slot->counter = 0;
  }
  direction = (uint8_t)(mapped & 0x0Fu);
  index = anim_a[direction];
  slot->anim_b = anim_b[index];
  slot->anim_c = anim_c[index];
  slot->anim_a = index;
  return FIFA96_OK;
}

int fifa96_control_slot_merge_reset(fifa96_control_slot *slot) {
  if (!slot) return -FIFA96_ERR_INVALID;
  slot->pressed = 0;      /* FUN_00078670 0x78674 */
  slot->released = 0;     /* 0x7867A */
  slot->reserved_08[0] = 0;
  slot->reserved_08[1] = 0; /* word +8, 0x78680 */
  slot->reserved_08[2] = 0;
  slot->reserved_08[3] = 0; /* word +0xA, 0x78686 */
  slot->held = 0;         /* 0x78698 word +0xC */
  slot->reserved_14[0] = 0;
  slot->reserved_14[1] = 0; /* word +0x14, 0x7868C */
  slot->reserved_14[2] = 0;
  slot->reserved_14[3] = 0; /* word +0x16, 0x78692 */
  return FIFA96_OK;
}

int fifa96_control_pick_ranked(const fifa96_control_candidate *candidates, uint32_t count,
                               int16_t skip_index) {
  uint16_t best = 0xFFFFu;
  int best_index = -1;
  uint32_t i;
  if (!candidates) return -FIFA96_ERR_INVALID;
  for (i = 0; i < count; i++) {
    uint16_t rank;
    if ((int32_t)i == skip_index) continue;
    if (candidates[i].skip_98 != 0 || candidates[i].skip_9a != 0) continue;
    rank = candidates[i].rank;
    if (rank < best) {
      best = rank;
      best_index = (int)i;
    }
  }
  return best_index;
}

int fifa96_control_reselect(uint8_t phase, int controlled_present, int ball_present,
                            const fifa96_entity_candidate *candidates, uint32_t count,
                            int16_t target_x, int16_t target_y, int32_t *controlled_index,
                            int16_t *best_distance) {
  if (!controlled_index || !best_distance || !candidates) return -FIFA96_ERR_INVALID;
  if (phase != 2) return 0;
  if (controlled_present && ball_present) return 0;
  *controlled_index = fifa96_entity_find_nearest(candidates, count, 0, target_x, target_y,
                                                 best_distance);
  return 1;
}

int fifa96_control_target_bucket(int16_t counter, int16_t t0, int16_t t1) {
  if (counter < t0) return 0;
  if (counter < t1) return 1;
  return 2;
}
