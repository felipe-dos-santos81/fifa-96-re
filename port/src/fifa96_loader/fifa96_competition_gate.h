#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

enum fifa96_competition_state {
  FIFA96_COMPETITION_STATE_6 = 6,
  FIFA96_COMPETITION_STATE_7 = 7,
  FIFA96_COMPETITION_STATE_8 = 8,
  FIFA96_COMPETITION_STATE_9 = 9,
  FIFA96_COMPETITION_STATE_10 = 10,
  FIFA96_COMPETITION_STATE_11 = 11
};

struct fifa96_competition_gate {
  uint32_t one_shot;
  uint32_t last_input;
  uint32_t confirm;
};

void fifa96_competition_gate_init(struct fifa96_competition_gate *gate);
int fifa96_competition_gate_initial(struct fifa96_competition_gate *gate,
                                    uint32_t *state);
int fifa96_competition_gate_after_input(struct fifa96_competition_gate *gate,
                                        int32_t input, uint32_t *state);
int fifa96_competition_gate_after_state10(struct fifa96_competition_gate *gate,
                                          uint32_t *state);
