#include "fifa96_loader/fifa96_keeper.h"

int fifa96_dispatch_begin(fifa96_dispatch_iter *iter, const fifa96_dispatch_team *teams,
                          uint32_t team_count) {
  uint32_t i;
  if (!iter || !teams || team_count == 0) return -FIFA96_ERR_INVALID;
  for (i = 0; i < team_count; i++) {
    if (teams[i].count != 0 && !teams[i].records) return -FIFA96_ERR_INVALID;
  }
  iter->teams = teams;
  iter->team_count = team_count;
  iter->team_index = 0;
  iter->record_index = 0;
  return FIFA96_OK;
}

int fifa96_dispatch_next(fifa96_dispatch_iter *iter, uint32_t *team_index,
                         uint32_t *record_index, int *is_keeper) {
  if (!iter || !iter->teams || !team_index || !record_index) return -FIFA96_ERR_INVALID;
  while (iter->team_index < iter->team_count) {
    const fifa96_dispatch_team *team = &iter->teams[iter->team_index];
    uint32_t index;
    if (iter->record_index >= team->count) {
      iter->team_index++;
      iter->record_index = 0;
      continue;
    }
    index = iter->record_index;
    iter->record_index++;
    if (index == 0) {
      *team_index = iter->team_index;
      *record_index = 0;
      if (is_keeper) *is_keeper = 1;
      return 1;
    }
    if (team->records[index].skip_9a == 0) {
      *team_index = iter->team_index;
      *record_index = index;
      if (is_keeper) *is_keeper = 0;
      return 1;
    }
  }
  return 0;
}

int fifa96_keeper_select_action(const fifa96_keeper_state *state, uint8_t type_gate,
                                uint8_t current_action, uint8_t *next_action) {
  uint8_t code;
  if (!state || !next_action) return -FIFA96_ERR_INVALID;
  if (type_gate == 0) return 0;
  if (state->timer != 0) {
    code = 0;
  } else if (state->phase != 2) {
    code = 0x19;
  } else if (state->controlled == 0) {
    code = 0x19;
  } else if (state->opponent_controlled != 0 && state->opponent_type_5 != 0) {
    code = 0x19;
  } else if (state->own_type_5 != 0) {
    code = current_action;
  } else {
    code = 4;
  }
  if (code == current_action) return 0;
  *next_action = code;
  return 1;
}
