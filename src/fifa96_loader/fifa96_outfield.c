#include "fifa96_loader/fifa96_outfield.h"

static const fifa96_outfield_rule pressed_0[] = {
    {0x07FF, 0x0060, 0x7CE38},
    {0x07FF, 0x0040, 0x7CF20},
    {0x07FF, 0x0080, 0x7CF20},
    {0, 0, 0},
};

static const fifa96_outfield_rule pressed_1[] = {
    {0x07FF, 0x0080, 0x7CF20},
    {0, 0, 0},
};

static const fifa96_outfield_rule pressed_2[] = {
    {0x07FF, 0x0010, 0x7D174},
    {0x07FF, 0x0040, 0x7D174},
    {0x07FF, 0x0080, 0x7CF20},
    {0, 0, 0},
};

static const fifa96_outfield_rule pressed_3[] = {
    {0x07FF, 0x0010, 0x7CFD0},
    {0x07FF, 0x0040, 0x7CD60},
    {0x07FF, 0x0080, 0x7CF20},
    {0, 0, 0},
};

static const fifa96_outfield_rule pressed_4[] = {
    {0x07FF, 0x0010, 0x7D054},
    {0x07FF, 0x0040, 0x7D08C},
    {0x07FF, 0x0080, 0x7CF20},
    {0, 0, 0},
};

static const fifa96_outfield_rule released_0[] = {
    {0x07FF, 0x0030, 0x7CEB0},
    {0x07FF, 0x0020, 0x7D1D4},
    {0x07FF, 0x0020, 0x7CEB0},
    {0x07FF, 0x0010, 0x7CF54},
    {0x07FF, 0x0040, 0x7D0C4},
    {0x07FF, 0x0040, 0x7CD60},
    {0, 0, 0},
};

static const fifa96_outfield_rule released_1[] = {
    {0x07FF, 0x0050, 0x7D010},
    {0x07FF, 0x0030, 0x7D110},
    {0x07FF, 0x0010, 0x7D110},
    {0x07FF, 0x0020, 0x7D110},
    {0x07FF, 0x0040, 0x7D110},
    {0x07FF, 0x0060, 0x7D110},
    {0, 0, 0},
};

static const fifa96_outfield_rule released_2[] = {
    {0x07FF, 0x0020, 0x7D1D4},
    {0, 0, 0},
};

static const fifa96_outfield_rule released_3[] = {
    {0x07FF, 0x0020, 0x7D1D4},
    {0, 0, 0},
};

static const fifa96_outfield_rule released_4[] = {
    {0x07FF, 0x0020, 0x7D1D4},
    {0, 0, 0},
};

static const fifa96_outfield_rule *const pressed_tables[FIFA96_OUTFIELD_CODE_COUNT] = {
    pressed_0, pressed_1, pressed_2, pressed_3, pressed_4,
};

static const fifa96_outfield_rule *const released_tables[FIFA96_OUTFIELD_CODE_COUNT] = {
    released_0, released_1, released_2, released_3, released_4,
};

const fifa96_outfield_rule *fifa96_outfield_pressed_rules(uint8_t code) {
  if (code >= FIFA96_OUTFIELD_CODE_COUNT) return NULL;
  return pressed_tables[code];
}

const fifa96_outfield_rule *fifa96_outfield_released_rules(uint8_t code) {
  if (code >= FIFA96_OUTFIELD_CODE_COUNT) return NULL;
  return released_tables[code];
}

int fifa96_outfield_rule_match(const fifa96_outfield_rule *rule, uint16_t word) {
  if (!rule) return -FIFA96_ERR_INVALID;
  return (rule->mask & word) == rule->want;
}

int fifa96_outfield_rules_run(const fifa96_outfield_rule *rules, uint16_t word,
                              fifa96_outfield_rule_fn call, void *context, uint32_t *handler) {
  if (!rules || !call || !handler) return -FIFA96_ERR_INVALID;
  for (;;) {
    if (fifa96_outfield_rule_match(rules, word)) {
      if (rules->handler == 0) return 0;
      *handler = rules->handler;
      if (call(rules->handler, context) != 0) return 1;
    }
    rules++;
  }
}

int fifa96_outfield_dispatch_code(const fifa96_outfield_edge *edge, uint8_t *code) {
  uint8_t selected;
  if (!edge || !code) return -FIFA96_ERR_INVALID;
  if (edge->pressed == 0 && edge->released == 0) return 0;
  if (edge->high_577ee_ge_50) {
    selected = 2;
  } else if (edge->type == 5) {
    selected = 1;
  } else if (edge->tracked && edge->user_absent_or_self) {
    selected = 3;
  } else if (edge->chaser) {
    selected = 4;
  } else {
    selected = 0;
  }
  *code = selected;
  return 1;
}

int fifa96_outfield_forced_action(const fifa96_outfield_forced_state *state, uint8_t current,
                                  uint8_t *next) {
  uint8_t code;
  if (!state || !next) return -FIFA96_ERR_INVALID;
  if (state->is_team_controlled) {
    if (state->opponent_has_ball) {
      code = 6;
    } else if (state->type_5) {
      return 0;
    } else {
      code = 4;
    }
  } else if (state->is_team_second) {
    if (state->opponent_has_ball) {
      code = 6;
    } else if (state->controlled_has_ball) {
      code = 3;
    } else {
      code = 4;
    }
  } else {
    code = 3;
  }
  if (code == current) return 0;
  *next = code;
  return 1;
}

int fifa96_outfield_chase_action(const fifa96_outfield_chase_state *state, uint8_t current,
                                 uint8_t *next) {
  if (!state || !next) return -FIFA96_ERR_INVALID;
  if (state->phase != 2 || state->type_gate == 0) return 0;
  if (!state->not_team_controlled || !state->not_team_second) return 0;
  if (state->distance >= 0x50 || state->camera >= 0x30) return 0;
  if (!state->user_present || !state->sides_differ) return 0;
  if (!state->unbound || state->timer != 0 || !state->third_zero) return 0;
  if (current == 8) return 0;
  *next = 8;
  return 1;
}

/* The per-type decision gate flat `0x110680[type]&1` (first-hand read: 26
 * bytes {3,0,0,3,3,3,3,2,0,...,2,2,2,2,0,0,0,0,0,3}). The native indexes
 * unbounded; types beyond 0x19 are not reachable from a real record and the
 * adjacent image bytes are not part of this table, so they return 0. */
static uint8_t outfield_type_gate(uint8_t type) {
  static const uint8_t table[26] = {
      3, 0, 0, 3, 3, 3, 3, 2, 0, 0, 0, 0, 0,
      0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 3,
  };
  if (type >= 26u) return 0;
  return (uint8_t)(table[type] & 1u);
}

int fifa96_outfield_chase_gate(const fifa96_outfield_chase_state *state, uint8_t type,
                               uint8_t current, uint8_t *next) {
  fifa96_outfield_chase_state s;
  if (!state || !next) return -FIFA96_ERR_INVALID;
  s = *state;
  s.type_gate = outfield_type_gate(type);
  return fifa96_outfield_chase_action(&s, current, next);
}

int fifa96_outfield_input_row(const fifa96_outfield_input_state *state,
                              fifa96_outfield_rule_fn call, void *context,
                              fifa96_outfield_input_out *out) {
  uint16_t word;
  uint8_t next = 0;
  if (!state || !call || !out) return -FIFA96_ERR_INVALID;
  out->direct_arm = 0;
  out->no_edge = 0;
  out->scan_ran = 0;
  out->scan_code = 0;
  out->scan_stopped = 0;
  out->no_edge_arm = 0;
  out->forced = 0;
  out->forced_code = 0;
  out->chase = 0;
  if (state->has_slot != 0) {
    if (state->flag_157ab0 != 0) {
      /* 0x7CAC4..0x7CB08: the pre-gate to the 0x7D1D4 selection handler. */
      if (state->is_1578ac == 0 && state->type == 3u &&
          (state->released & 0x20u) != 0) {
        out->direct_arm = 1;
      }
    } else if (state->pressed == 0 && state->released == 0) {
      /* 0x7CB10..0x7CB1C -> the no-edge arm 0x7CC13..0x7CC7D. */
      out->no_edge = 1;
      if ((state->slot_word10 & 0xF0u) != 0 && state->phase == 2u) {
        int16_t d = (int16_t)(state->lane >> 16);
        if (d >= 0x90) {
          /* 0x7CC41 side filter */
          if (!(state->user_present != 0 && state->user_side == state->side) &&
              (state->slot_word10 & 0xC0u) != 0)
            out->no_edge_arm = 1;
        } else if (d > 0x30) {
          out->no_edge_arm = 1;   /* 0x7CC70 */
        } else {
          if (!(state->user_present != 0 && state->user_side == state->side) &&
              (state->slot_word10 & 0xC0u) != 0)
            out->no_edge_arm = 1;
        }
      }
    } else {
      fifa96_outfield_edge edge;
      uint32_t handler = 0;
      edge.pressed = state->pressed;
      edge.released = state->released;
      edge.type = state->type;
      edge.high_577ee_ge_50 = state->high_577ee_ge_50;
      edge.tracked = state->tracked;
      edge.user_absent_or_self = state->user_absent_or_self;
      edge.chaser = state->chaser;
      if (fifa96_outfield_dispatch_code(&edge, &out->scan_code) != 1)
        return -FIFA96_ERR_INVALID;
      out->scan_ran = 1;
      word = (uint16_t)(state->pressed & 0xFF0u);
      if (word != 0) {
        if (fifa96_outfield_rules_run(fifa96_outfield_pressed_rules(out->scan_code), word,
                                      call, context, &handler) == 1)
          out->scan_stopped = 1;
      } else {
        word = (uint16_t)(state->released & 0xFF0u);
        if (word != 0) {
          if (fifa96_outfield_rules_run(fifa96_outfield_released_rules(out->scan_code), word,
                                        call, context, &handler) == 1)
            out->scan_stopped = 1;
        }
      }
    }
  }
  /* 0x7CC82..0x7CD24: the phase-2 forced decision and the code-8 chase gate,
   * both behind the per-type gate. */
  if (state->phase == 2u && outfield_type_gate(state->type) != 0) {
    if (fifa96_outfield_forced_action(&state->forced, state->current_code, &next) == 1) {
      out->forced = 1;
      out->forced_code = next;
    }
    if (fifa96_outfield_chase_gate(&state->chase, state->type, state->current_code,
                                   &next) == 1)
      out->chase = 1;
  }
  return FIFA96_OK;
}
