#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_match_pace.h"

#define FIFA96_MATCH_STATE_STEP 0x200u
#define FIFA96_MATCH_STATE_SECOND 0x3Cu

struct fifa96_match_state {
  uint32_t frame_acc;
  uint16_t frame_delta;
  uint16_t tick_total;
  uint16_t period_seconds;
  uint16_t total_seconds;
  uint16_t aux_seconds;
  uint16_t aux_tick;
  uint16_t period_length;
  uint16_t extra_length;
  uint8_t second_acc;
  uint8_t period;
  uint8_t phase;
  uint8_t prev_phase;
  uint8_t aux_flag;
};

void fifa96_match_state_init(struct fifa96_match_state *s);
uint8_t fifa96_match_state_phase_class(uint8_t phase);
int fifa96_match_state_set_phase(struct fifa96_match_state *s, uint8_t phase);
int fifa96_match_state_advance_period(struct fifa96_match_state *s);
int fifa96_match_state_frame(struct fifa96_match_state *s, uint32_t step, int clock_halt, int *period_ended);
int fifa96_match_state_tick(struct fifa96_match_state *s, struct fifa96_match_pace *pace, int pace_blocked, int clock_halt, int *period_ended);
