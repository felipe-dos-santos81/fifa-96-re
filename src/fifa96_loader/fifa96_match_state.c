#include <stddef.h>
#include "fifa96_loader/fifa96_match_state.h"

static const uint8_t fifa96_match_state_class_table[0x30] = {
  2, 0, 1, 2, 2, 0, 2, 2, 2, 2, 0, 0, 0, 2, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void fifa96_match_state_init(struct fifa96_match_state *s) {
  if (!s) return;
  s->frame_acc = 0;
  s->frame_delta = 0;
  s->tick_total = 0;
  s->period_seconds = 0;
  s->total_seconds = 0;
  s->aux_seconds = 0;
  s->aux_tick = 0;
  s->period_length = 0;
  s->extra_length = 0;
  s->second_acc = 0;
  s->period = 0;
  s->phase = 0;
  s->prev_phase = 0;
  s->aux_flag = 0;
}

uint8_t fifa96_match_state_phase_class(uint8_t phase) {
  if (phase >= 0x30u) return 0;
  return fifa96_match_state_class_table[phase];
}

int fifa96_match_state_set_phase(struct fifa96_match_state *s, uint8_t phase) {
  if (!s) return -FIFA96_ERR_INVALID;
  s->prev_phase = s->phase;
  s->phase = phase;
  return 0;
}

int fifa96_match_state_advance_period(struct fifa96_match_state *s) {
  if (!s) return -FIFA96_ERR_INVALID;
  s->period++;
  s->period_seconds = 0;
  return 0;
}

int fifa96_match_state_frame(struct fifa96_match_state *s, uint32_t step, int clock_halt, int *period_ended) {
  uint32_t whole;
  uint8_t cls;
  if (!s || !period_ended) return -FIFA96_ERR_INVALID;
  *period_ended = 0;
  s->frame_acc += step;
  whole = s->frame_acc >> 8;
  s->frame_acc &= 0xFFu;
  s->frame_delta = (uint16_t)whole;
  s->tick_total = (uint16_t)(s->tick_total + (uint16_t)whole);
  cls = fifa96_match_state_class_table[s->phase < 0x30u ? s->phase : 0x30u];
  if (cls != 1u && !(cls == 2u && !clock_halt)) return 0;
  s->second_acc = (uint8_t)(s->second_acc + (uint8_t)whole);
  while (s->second_acc >= FIFA96_MATCH_STATE_SECOND) {
    uint16_t limit = 0;
    int complete = 0;
    if (s->period < 4u) {
      limit = (s->period < 2u) ? s->period_length : s->extra_length;
    }
    if (cls == 2u && s->period < 4u) {
      if ((uint32_t)s->period_seconds + 0x14u < (uint32_t)s->aux_seconds + limit) {
        if (s->aux_tick == 0x1Eu) s->aux_seconds++;
        else s->aux_tick++;
      } else {
        s->aux_seconds++;
      }
    }
    s->second_acc = (uint8_t)(s->second_acc - FIFA96_MATCH_STATE_SECOND);
    s->period_seconds = (uint16_t)(s->period_seconds + 1u);
    s->total_seconds = (uint16_t)(s->total_seconds + 1u);
    if (s->period < 4u) {
      if ((uint32_t)s->period_seconds == (uint32_t)limit + s->aux_seconds) {
        complete = 1;
      } else if (s->period_seconds == limit && s->aux_seconds != 0) {
        s->aux_flag = 1;
      }
    }
    if (complete) {
      s->period++;
      s->period_seconds = 0;
      *period_ended = 1;
      break;
    }
  }
  return 0;
}

int fifa96_match_state_tick(struct fifa96_match_state *s, struct fifa96_match_pace *pace, int pace_blocked, int clock_halt, int *period_ended) {
  int granted = 0;
  int rc;
  if (!s || !pace || !period_ended) return -FIFA96_ERR_INVALID;
  *period_ended = 0;
  rc = fifa96_match_pace_tick(pace, pace_blocked, &granted);
  if (rc != 0) return rc;
  if (!granted) return 0;
  return fifa96_match_state_frame(s, FIFA96_MATCH_STATE_STEP, clock_halt, period_ended);
}
