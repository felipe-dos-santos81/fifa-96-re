#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_OUTFIELD_CODE_COUNT 5

typedef struct fifa96_outfield_rule {
  uint16_t mask;
  uint16_t want;
  uint32_t handler;
} fifa96_outfield_rule;

typedef int (*fifa96_outfield_rule_fn)(uint32_t handler, void *context);

const fifa96_outfield_rule *fifa96_outfield_pressed_rules(uint8_t code);
const fifa96_outfield_rule *fifa96_outfield_released_rules(uint8_t code);
int fifa96_outfield_rule_match(const fifa96_outfield_rule *rule, uint16_t word);
int fifa96_outfield_rules_run(const fifa96_outfield_rule *rules, uint16_t word,
                              fifa96_outfield_rule_fn call, void *context,
                              uint32_t *handler);

typedef struct fifa96_outfield_edge {
  uint16_t pressed;
  uint16_t released;
  uint8_t type;
  uint8_t high_577ee_ge_50;
  uint8_t tracked;
  uint8_t user_absent_or_self;
  uint8_t chaser;
} fifa96_outfield_edge;

int fifa96_outfield_dispatch_code(const fifa96_outfield_edge *edge, uint8_t *code);

typedef struct fifa96_outfield_forced_state {
  uint8_t is_team_controlled;
  uint8_t is_team_second;
  uint8_t type_5;
  uint8_t opponent_has_ball;
  uint8_t controlled_has_ball;
} fifa96_outfield_forced_state;

int fifa96_outfield_forced_action(const fifa96_outfield_forced_state *state, uint8_t current,
                                  uint8_t *next);

typedef struct fifa96_outfield_chase_state {
  uint8_t phase;
  uint8_t type_gate;
  uint8_t not_team_controlled;
  uint8_t not_team_second;
  uint16_t distance;
  uint16_t camera;
  uint8_t user_present;
  uint8_t sides_differ;
  uint8_t unbound;
  uint16_t timer;
  uint8_t third_zero;
} fifa96_outfield_chase_state;

int fifa96_outfield_chase_action(const fifa96_outfield_chase_state *state, uint8_t current,
                                 uint8_t *next);
