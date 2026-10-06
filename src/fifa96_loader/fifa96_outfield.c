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
