#include <stddef.h>
#include "fifa96_loader/fifa96_competition_gate.h"

void fifa96_competition_gate_init(struct fifa96_competition_gate *gate) {
  if (gate == NULL) {
    return;
  }
  gate->one_shot = 0;
  gate->last_input = 0;
  gate->confirm = 0;
}

int fifa96_competition_gate_initial(struct fifa96_competition_gate *gate,
                                    uint32_t *state) {
  if (gate == NULL || state == NULL) {
    return -FIFA96_ERR_INVALID;
  }
  if (gate->one_shot != 0) {
    gate->one_shot = 0;
    *state = FIFA96_COMPETITION_STATE_6;
  } else if (gate->last_input == 1) {
    *state = FIFA96_COMPETITION_STATE_8;
  } else {
    *state = FIFA96_COMPETITION_STATE_9;
  }
  return 1;
}

int fifa96_competition_gate_after_input(struct fifa96_competition_gate *gate,
                                        int32_t input, uint32_t *state) {
  if (gate == NULL || state == NULL) {
    return -FIFA96_ERR_INVALID;
  }
  gate->last_input = (uint32_t)input;
  if (gate->confirm != 0) {
    return 0;
  }
  if (input == 2) {
    *state = FIFA96_COMPETITION_STATE_7;
  } else if (input == 0) {
    *state = FIFA96_COMPETITION_STATE_8;
  } else {
    *state = FIFA96_COMPETITION_STATE_9;
  }
  return 1;
}

int fifa96_competition_gate_after_state10(struct fifa96_competition_gate *gate,
                                          uint32_t *state) {
  if (gate == NULL || state == NULL) {
    return -FIFA96_ERR_INVALID;
  }
  if (gate->confirm != 0) {
    return 0;
  }
  *state = FIFA96_COMPETITION_STATE_11;
  return 1;
}
