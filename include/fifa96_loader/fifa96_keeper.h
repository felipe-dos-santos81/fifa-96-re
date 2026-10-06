#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

typedef struct fifa96_dispatch_record {
  uint8_t skip_9a;
} fifa96_dispatch_record;

typedef struct fifa96_dispatch_team {
  const fifa96_dispatch_record *records;
  uint32_t count;
} fifa96_dispatch_team;

typedef struct fifa96_dispatch_iter {
  const fifa96_dispatch_team *teams;
  uint32_t team_count;
  uint32_t team_index;
  uint32_t record_index;
} fifa96_dispatch_iter;

typedef struct fifa96_keeper_state {
  uint16_t timer;
  uint8_t phase;
  uint8_t controlled;
  uint8_t own_type_5;
  uint8_t opponent_controlled;
  uint8_t opponent_type_5;
} fifa96_keeper_state;

int fifa96_dispatch_begin(fifa96_dispatch_iter *iter, const fifa96_dispatch_team *teams,
                          uint32_t team_count);
int fifa96_dispatch_next(fifa96_dispatch_iter *iter, uint32_t *team_index,
                         uint32_t *record_index, int *is_keeper);
int fifa96_keeper_select_action(const fifa96_keeper_state *state, uint8_t type_gate,
                                uint8_t current_action, uint8_t *next_action);
