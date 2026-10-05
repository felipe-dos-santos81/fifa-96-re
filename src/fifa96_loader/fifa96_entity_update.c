#include "fifa96_loader/fifa96_entity_update.h"

static int32_t fifa96_entity_abs_word(int32_t value) {
  if ((int16_t)value < 0) value = -value;
  return value;
}

int32_t fifa96_entity_distance(int32_t dx, int32_t dy) {
  int32_t a = fifa96_entity_abs_word(dx);
  int32_t b = fifa96_entity_abs_word(dy);
  int32_t a16 = (int16_t)a;
  int32_t b16 = (int16_t)b;
  int32_t q;
  if (a16 < b16) {
    q = a16 >> 2;
    if (a16 > (b16 >> 1)) q = ((a16 >> 1) + q) >> 1;
    return q + b;
  }
  if (a16 > b16) {
    q = b16 >> 2;
    if (b16 > (a16 >> 1)) q = ((b16 >> 1) + q) >> 1;
    return q + a;
  }
  q = (a16 >> 2) + (a16 >> 1);
  return (q >> 1) + (int16_t)b;
}

int fifa96_entity_find_nearest(const fifa96_entity_candidate *candidates,
                               uint32_t count, uint32_t skip_index,
                               int16_t target_x, int16_t target_y,
                               int16_t *best_distance) {
  uint16_t best;
  int best_index = -1;
  uint32_t i;
  if (!candidates || !best_distance) return -FIFA96_ERR_INVALID;
  best = 0xFFFFu;
  for (i = 0; i < count; i++) {
    uint16_t d;
    if (i == (uint32_t)(uint16_t)skip_index) continue;
    if (candidates[i].skip_98 != 0 || candidates[i].skip_9a != 0) continue;
    d = (uint16_t)fifa96_entity_distance((int16_t)(target_x - candidates[i].x),
                                         (int16_t)(target_y - candidates[i].y));
    if (d < best) {
      best = d;
      best_index = (int)i;
    }
  }
  *best_distance = (int16_t)best;
  return best_index;
}
