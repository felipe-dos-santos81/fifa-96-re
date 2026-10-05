#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

typedef struct fifa96_entity_candidate {
  int16_t x;
  int16_t y;
  uint8_t skip_98;
  uint8_t skip_9a;
} fifa96_entity_candidate;

int32_t fifa96_entity_distance(int32_t dx, int32_t dy);
int fifa96_entity_find_nearest(const fifa96_entity_candidate *candidates,
                               uint32_t count, uint32_t skip_index,
                               int16_t target_x, int16_t target_y,
                               int16_t *best_distance);
