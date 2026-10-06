// tests/test_outfield.c — FU-75 outfield action selection (docs/ghidra/FU75_outfield_decide.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_outfield.h"

_Static_assert(offsetof(fifa96_outfield_rule, mask) == 0, "mask");
_Static_assert(offsetof(fifa96_outfield_rule, want) == 2, "want");
_Static_assert(offsetof(fifa96_outfield_rule, handler) == 4, "handler");
_Static_assert(offsetof(fifa96_outfield_edge, pressed) == 0, "pressed");
_Static_assert(offsetof(fifa96_outfield_edge, released) == 2, "released");
_Static_assert(offsetof(fifa96_outfield_edge, type) == 4, "type");
_Static_assert(offsetof(fifa96_outfield_edge, high_577ee_ge_50) == 5, "high");
_Static_assert(offsetof(fifa96_outfield_edge, tracked) == 6, "tracked");
_Static_assert(offsetof(fifa96_outfield_edge, user_absent_or_self) == 7, "user");
_Static_assert(offsetof(fifa96_outfield_edge, chaser) == 8, "chaser");
_Static_assert(offsetof(fifa96_outfield_forced_state, is_team_controlled) == 0, "controlled");
_Static_assert(offsetof(fifa96_outfield_forced_state, is_team_second) == 1, "second");
_Static_assert(offsetof(fifa96_outfield_forced_state, type_5) == 2, "type_5");
_Static_assert(offsetof(fifa96_outfield_forced_state, opponent_has_ball) == 3, "opponent");
_Static_assert(offsetof(fifa96_outfield_forced_state, controlled_has_ball) == 4, "own");
_Static_assert(offsetof(fifa96_outfield_chase_state, phase) == 0, "phase");
_Static_assert(offsetof(fifa96_outfield_chase_state, type_gate) == 1, "type_gate");
_Static_assert(offsetof(fifa96_outfield_chase_state, not_team_controlled) == 2, "not_ctrl");
_Static_assert(offsetof(fifa96_outfield_chase_state, not_team_second) == 3, "not_second");
_Static_assert(offsetof(fifa96_outfield_chase_state, distance) == 4, "distance");
_Static_assert(offsetof(fifa96_outfield_chase_state, camera) == 6, "camera");
_Static_assert(offsetof(fifa96_outfield_chase_state, user_present) == 8, "user_present");
_Static_assert(offsetof(fifa96_outfield_chase_state, sides_differ) == 9, "sides_differ");
_Static_assert(offsetof(fifa96_outfield_chase_state, unbound) == 10, "unbound");
_Static_assert(offsetof(fifa96_outfield_chase_state, timer) == 12, "timer");
_Static_assert(offsetof(fifa96_outfield_chase_state, third_zero) == 14, "third_zero");

static int handler_accept(uint32_t handler, void *context) {
  (void)handler;
  (void)context;
  return 1;
}

static int handler_reject(uint32_t handler, void *context) {
  (void)handler;
  (void)context;
  return 0;
}

static int handler_first_zero_then_accept(uint32_t handler, void *context) {
  uint32_t *seen = (uint32_t *)context;
  if (handler == seen[0]) return 0;
  seen[1] = handler;
  return 1;
}

static void test_table_accessors(void) {
  const fifa96_outfield_rule *r = fifa96_outfield_pressed_rules(0);
  const fifa96_outfield_rule *q;
  assert(r != NULL);
  assert(r[0].mask == 0x07FF && r[0].want == 0x0060 && r[0].handler == 0x7CE38);
  assert(r[1].mask == 0x07FF && r[1].want == 0x0040 && r[1].handler == 0x7CF20);
  assert(r[2].mask == 0x07FF && r[2].want == 0x0080 && r[2].handler == 0x7CF20);
  assert(r[3].handler == 0);
  q = fifa96_outfield_pressed_rules(3);
  assert(q != NULL);
  assert(q[0].want == 0x0010 && q[0].handler == 0x7CFD0);
  assert(q[1].want == 0x0040 && q[1].handler == 0x7CD60);
  assert(q[2].want == 0x0080 && q[2].handler == 0x7CF20);
  assert(q[3].handler == 0);
  q = fifa96_outfield_released_rules(0);
  assert(q != NULL);
  assert(q[0].want == 0x0030 && q[0].handler == 0x7CEB0);
  assert(q[1].want == 0x0020 && q[1].handler == 0x7D1D4);
  assert(q[2].want == 0x0020 && q[2].handler == 0x7CEB0);
  assert(q[3].want == 0x0010 && q[3].handler == 0x7CF54);
  assert(q[4].want == 0x0040 && q[4].handler == 0x7D0C4);
  assert(q[5].want == 0x0040 && q[5].handler == 0x7CD60);
  assert(q[6].handler == 0);
  q = fifa96_outfield_released_rules(1);
  assert(q != NULL);
  assert(q[0].want == 0x0050 && q[0].handler == 0x7D010);
  assert(q[1].want == 0x0030 && q[1].handler == 0x7D110);
  assert(q[5].want == 0x0060 && q[5].handler == 0x7D110);
  assert(q[6].handler == 0);
  q = fifa96_outfield_released_rules(4);
  assert(q != NULL);
  assert(q[0].want == 0x0020 && q[0].handler == 0x7D1D4);
  assert(q[1].handler == 0);
  assert(fifa96_outfield_pressed_rules(FIFA96_OUTFIELD_CODE_COUNT) == NULL);
  assert(fifa96_outfield_released_rules(0xFF) == NULL);
}

static void test_rule_match(void) {
  const fifa96_outfield_rule row = {0x07FF, 0x0060, 0x7CE38};
  const fifa96_outfield_rule terminator = {0, 0, 0};
  assert(fifa96_outfield_rule_match(&row, 0x0060) == 1);
  assert(fifa96_outfield_rule_match(&row, 0x0060 | 0x800) == 1);
  assert(fifa96_outfield_rule_match(&row, 0x0040) == 0);
  assert(fifa96_outfield_rule_match(&row, 0x0061) == 0);
  assert(fifa96_outfield_rule_match(&row, 0x0620) == 0);
  assert(fifa96_outfield_rule_match(&terminator, 0x0040) == 1);
  assert(fifa96_outfield_rule_match(NULL, 0) == -FIFA96_ERR_INVALID);
}

static void test_rules_run(void) {
  static const fifa96_outfield_rule rows[] = {
      {0x07FF, 0x0040, 0x111},
      {0x07FF, 0x0080, 0x222},
      {0, 0, 0},
  };
  static const fifa96_outfield_rule dup[] = {
      {0x07FF, 0x0040, 0x111},
      {0x07FF, 0x0040, 0x222},
      {0, 0, 0},
  };
  uint32_t handler = 0xAA;
  uint32_t seen[2] = {0x111, 0};
  assert(fifa96_outfield_rules_run(rows, 0x0040, handler_reject, NULL, &handler) == 0);
  assert(handler == 0x111);
  handler = 0xAA;
  assert(fifa96_outfield_rules_run(rows, 0x0080, handler_accept, NULL, &handler) == 1);
  assert(handler == 0x222);
  handler = 0xAA;
  assert(fifa96_outfield_rules_run(rows, 0x0001, handler_accept, NULL, &handler) == 0);
  assert(handler == 0xAA);
  handler = 0xAA;
  assert(fifa96_outfield_rules_run(dup, 0x0040, handler_first_zero_then_accept, seen, &handler) == 1);
  assert(handler == 0x222);
  assert(seen[1] == 0x222);
  assert(fifa96_outfield_rules_run(NULL, 0x40, handler_accept, NULL, &handler) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_outfield_rules_run(rows, 0x40, NULL, NULL, &handler) == -FIFA96_ERR_INVALID);
  assert(fifa96_outfield_rules_run(rows, 0x40, handler_accept, NULL, NULL) ==
         -FIFA96_ERR_INVALID);
}

static fifa96_outfield_edge edge(uint16_t pressed, uint16_t released, uint8_t type,
                                 uint8_t high, uint8_t tracked, uint8_t user, uint8_t chaser) {
  fifa96_outfield_edge e;
  e.pressed = pressed;
  e.released = released;
  e.type = type;
  e.high_577ee_ge_50 = high;
  e.tracked = tracked;
  e.user_absent_or_self = user;
  e.chaser = chaser;
  return e;
}

static void test_dispatch_code(void) {
  uint8_t code = 0xAA;
  assert(fifa96_outfield_dispatch_code(&(fifa96_outfield_edge){0}, &code) == 0);
  assert(code == 0xAA);
  assert(fifa96_outfield_dispatch_code(&(fifa96_outfield_edge){.pressed = 0x40}, &code) == 1);
  assert(code == 0);
  assert(fifa96_outfield_dispatch_code(&(fifa96_outfield_edge){.pressed = 0x0001}, &code) == 1);
  assert(code == 0);
  code = 0xAA;
  assert(fifa96_outfield_dispatch_code(&(fifa96_outfield_edge){.released = 0x1}, &code) == 1);
  assert(code == 0);
  fifa96_outfield_edge e;
  e = edge(0x40, 0, 5, 0, 0, 0, 0);
  assert(fifa96_outfield_dispatch_code(&e, &code) == 1);
  assert(code == 1);
  e = edge(0x40, 0, 5, 1, 0, 0, 0);
  assert(fifa96_outfield_dispatch_code(&e, &code) == 1);
  assert(code == 2);
  e = edge(0x40, 0, 7, 0, 1, 1, 0);
  assert(fifa96_outfield_dispatch_code(&e, &code) == 1);
  assert(code == 3);
  e = edge(0x40, 0, 7, 0, 1, 0, 1);
  assert(fifa96_outfield_dispatch_code(&e, &code) == 1);
  assert(code == 4);
  e = edge(0x40, 0, 7, 0, 0, 0, 1);
  assert(fifa96_outfield_dispatch_code(&e, &code) == 1);
  assert(code == 4);
  e = edge(0x40, 0, 7, 0, 1, 0, 0);
  assert(fifa96_outfield_dispatch_code(&e, &code) == 1);
  assert(code == 0);
  assert(fifa96_outfield_dispatch_code(NULL, &code) == -FIFA96_ERR_INVALID);
  assert(fifa96_outfield_dispatch_code(&(fifa96_outfield_edge){.pressed = 0x40}, NULL) ==
         -FIFA96_ERR_INVALID);
}

static fifa96_outfield_forced_state forced(uint8_t controlled, uint8_t second, uint8_t type_5,
                                           uint8_t opponent_ball, uint8_t controlled_ball) {
  fifa96_outfield_forced_state s;
  s.is_team_controlled = controlled;
  s.is_team_second = second;
  s.type_5 = type_5;
  s.opponent_has_ball = opponent_ball;
  s.controlled_has_ball = controlled_ball;
  return s;
}

static void test_forced_action(void) {
  uint8_t next = 0xAA;
  fifa96_outfield_forced_state s;
  s = forced(1, 0, 0, 1, 0);
  assert(fifa96_outfield_forced_action(&s, 0, &next) == 1);
  assert(next == 6);
  next = 0xAA;
  s = forced(1, 0, 1, 0, 0);
  assert(fifa96_outfield_forced_action(&s, 5, &next) == 0);
  assert(next == 0xAA);
  assert(fifa96_outfield_forced_action(&s, 4, &next) == 0);
  assert(next == 0xAA);
  s = forced(1, 0, 0, 0, 0);
  assert(fifa96_outfield_forced_action(&s, 0, &next) == 1);
  assert(next == 4);
  s = forced(0, 1, 0, 1, 0);
  assert(fifa96_outfield_forced_action(&s, 0, &next) == 1);
  assert(next == 6);
  s = forced(0, 1, 0, 0, 1);
  assert(fifa96_outfield_forced_action(&s, 0, &next) == 1);
  assert(next == 3);
  s = forced(0, 1, 0, 0, 0);
  assert(fifa96_outfield_forced_action(&s, 0, &next) == 1);
  assert(next == 4);
  s = forced(0, 0, 0, 0, 0);
  assert(fifa96_outfield_forced_action(&s, 0, &next) == 1);
  assert(next == 3);
  assert(fifa96_outfield_forced_action(&s, 3, &next) == 0);
  assert(next == 3);
  assert(fifa96_outfield_forced_action(&s, 4, &next) == 1);
  assert(next == 3);
  assert(fifa96_outfield_forced_action(NULL, 0, &next) == -FIFA96_ERR_INVALID);
  assert(fifa96_outfield_forced_action(&s, 0, NULL) == -FIFA96_ERR_INVALID);
}

static fifa96_outfield_chase_state chase(uint8_t phase, uint8_t gate, uint16_t distance,
                                         uint16_t camera) {
  fifa96_outfield_chase_state s;
  s.phase = phase;
  s.type_gate = gate;
  s.not_team_controlled = 1;
  s.not_team_second = 1;
  s.distance = distance;
  s.camera = camera;
  s.user_present = 1;
  s.sides_differ = 1;
  s.unbound = 1;
  s.timer = 0;
  s.third_zero = 1;
  return s;
}

static void test_chase_action(void) {
  uint8_t next = 0xAA;
  fifa96_outfield_chase_state s = chase(2, 1, 0x4F, 0x2F);
  assert(fifa96_outfield_chase_action(&s, 0, &next) == 1);
  assert(next == 8);
  assert(fifa96_outfield_chase_action(&s, 8, &next) == 0);
  s.phase = 1;
  assert(fifa96_outfield_chase_action(&s, 0, &next) == 0);
  s = chase(2, 0, 0x4F, 0x2F);
  assert(fifa96_outfield_chase_action(&s, 0, &next) == 0);
  s = chase(2, 1, 0x50, 0x2F);
  assert(fifa96_outfield_chase_action(&s, 0, &next) == 0);
  s = chase(2, 1, 0x4F, 0x30);
  assert(fifa96_outfield_chase_action(&s, 0, &next) == 0);
  s = chase(2, 1, 0x4F, 0x2F);
  s.not_team_controlled = 0;
  assert(fifa96_outfield_chase_action(&s, 0, &next) == 0);
  s = chase(2, 1, 0x4F, 0x2F);
  s.not_team_second = 0;
  assert(fifa96_outfield_chase_action(&s, 0, &next) == 0);
  s = chase(2, 1, 0x4F, 0x2F);
  s.user_present = 0;
  assert(fifa96_outfield_chase_action(&s, 0, &next) == 0);
  s = chase(2, 1, 0x4F, 0x2F);
  s.sides_differ = 0;
  assert(fifa96_outfield_chase_action(&s, 0, &next) == 0);
  s = chase(2, 1, 0x4F, 0x2F);
  s.unbound = 0;
  assert(fifa96_outfield_chase_action(&s, 0, &next) == 0);
  s = chase(2, 1, 0x4F, 0x2F);
  s.timer = 1;
  assert(fifa96_outfield_chase_action(&s, 0, &next) == 0);
  s = chase(2, 1, 0x4F, 0x2F);
  s.third_zero = 0;
  assert(fifa96_outfield_chase_action(&s, 0, &next) == 0);
  assert(fifa96_outfield_chase_action(NULL, 0, &next) == -FIFA96_ERR_INVALID);
  assert(fifa96_outfield_chase_action(&s, 0, NULL) == -FIFA96_ERR_INVALID);
}

int main(void) {
  test_table_accessors();
  test_rule_match();
  test_rules_run();
  test_dispatch_code();
  test_forced_action();
  test_chase_action();
  puts("test_outfield: all assertions passed");
  return 0;
}
