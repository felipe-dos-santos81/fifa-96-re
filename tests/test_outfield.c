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

/* ===== M2 Task 14 / OL-38: the FUN_0007CA54 machine subset ================ */

typedef struct row_log {
  uint32_t handlers[8];
  int count;
  int accept;
} row_log;

static int row_record(uint32_t handler, void *context) {
  row_log *log = (row_log *)context;
  if (log->count < 8) log->handlers[log->count] = handler;
  log->count++;
  return log->accept;
}

static fifa96_outfield_input_state input_state(void) {
  fifa96_outfield_input_state s;
  memset(&s, 0, sizeof s);
  s.has_slot = 1;
  s.phase = 2;
  s.type = 0;
  s.forced.is_team_controlled = 0;
  s.forced.is_team_second = 0;
  s.forced.type_5 = 0;
  s.forced.opponent_has_ball = 0;
  s.forced.controlled_has_ball = 0;
  s.chase.phase = 2;
  s.chase.type_gate = 1;
  s.chase.not_team_controlled = 1;
  s.chase.not_team_second = 1;
  s.chase.distance = 0x4F;
  s.chase.camera = 0x2F;
  s.chase.user_present = 1;
  s.chase.sides_differ = 1;
  s.chase.unbound = 1;
  s.chase.timer = 0;
  s.chase.third_zero = 1;
  return s;
}

static void test_chase_gate_type_table(void) {
  fifa96_outfield_chase_state s = input_state().chase;
  uint8_t next = 0xAA;
  assert(fifa96_outfield_chase_gate(&s, 0, 0, &next) == 1 && next == 8);
  next = 0xAA;
  assert(fifa96_outfield_chase_gate(&s, 3, 0, &next) == 1 && next == 8);
  next = 0xAA;
  assert(fifa96_outfield_chase_gate(&s, 4, 0, &next) == 1 && next == 8);
  next = 0xAA;
  assert(fifa96_outfield_chase_gate(&s, 6, 0, &next) == 1 && next == 8);
  next = 0xAA;
  assert(fifa96_outfield_chase_gate(&s, 0x19, 0, &next) == 1 && next == 8);
  /* table zero bits: types 1/2, 7..15, 16..24, 26+ */
  next = 0xAA;
  assert(fifa96_outfield_chase_gate(&s, 1, 0, &next) == 0 && next == 0xAA);
  next = 0xAA;
  assert(fifa96_outfield_chase_gate(&s, 2, 0, &next) == 0 && next == 0xAA);
  next = 0xAA;
  assert(fifa96_outfield_chase_gate(&s, 7, 0, &next) == 0 && next == 0xAA);
  next = 0xAA;
  assert(fifa96_outfield_chase_gate(&s, 8, 0, &next) == 0 && next == 0xAA);
  next = 0xAA;
  assert(fifa96_outfield_chase_gate(&s, 0x10, 0, &next) == 0 && next == 0xAA);
  next = 0xAA;
  assert(fifa96_outfield_chase_gate(&s, 0x14, 0, &next) == 0 && next == 0xAA);
  next = 0xAA;
  assert(fifa96_outfield_chase_gate(&s, 26, 0, &next) == 0 && next == 0xAA);
  /* the wrapper's table lookup overrides the state bit */
  s.type_gate = 0;
  next = 0xAA;
  assert(fifa96_outfield_chase_gate(&s, 3, 0, &next) == 1 && next == 8);
  assert(fifa96_outfield_chase_gate(NULL, 3, 0, &next) == -FIFA96_ERR_INVALID);
  assert(fifa96_outfield_chase_gate(&s, 3, 0, NULL) == -FIFA96_ERR_INVALID);
}

static void test_input_row_no_edge_arm(void) {
  fifa96_outfield_input_state s = input_state();
  fifa96_outfield_input_out out;
  row_log log = {{0}, 0, 0};
  s.pressed = 0;
  s.released = 0;
  s.slot_word10 = 0x10;
  s.lane = 0x40 << 16;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.no_edge == 1 && out.no_edge_arm == 1 && out.scan_ran == 0);
  s.slot_word10 = 0x0F;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.no_edge == 1 && out.no_edge_arm == 0);
  s.slot_word10 = 0x10;
  s.phase = 1;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.no_edge_arm == 0);
  s.phase = 2;
  /* lane <= 0x30 takes the side filter: same user side refuses */
  s.lane = 0x20 << 16;
  s.user_present = 1;
  s.user_side = 0;
  s.side = 0;
  s.slot_word10 = 0xC0;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.no_edge_arm == 0);
  s.user_side = 1;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.no_edge_arm == 1);
  s.user_present = 0;
  s.slot_word10 = 0x80;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.no_edge_arm == 1);
  s.slot_word10 = 0x20;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.no_edge_arm == 0);
  s.slot_word10 = 0xC0;
  s.lane = 0x90 << 16;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.no_edge_arm == 1);
}

static void test_input_row_scan(void) {
  fifa96_outfield_input_state s = input_state();
  fifa96_outfield_input_out out;
  row_log log = {{0}, 0, 0};
  s.type = 7;
  s.pressed = 0x40;
  s.released = 0x40;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.scan_ran == 1 && out.scan_code == 0);
  assert(out.scan_stopped == 0);
  /* only the pressed table runs (the released table must not) */
  assert(log.count == 1 && log.handlers[0] == 0x7CF20);
  log.count = 0;
  log.accept = 1;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.scan_stopped == 1 && log.count == 1);
  /* pressed word zero (raw 0x0001) falls to the released table */
  log.count = 0;
  log.accept = 0;
  s.pressed = 0x0001;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.scan_code == 0 && out.scan_ran == 1);
  assert(log.count == 2 && log.handlers[0] == 0x7D0C4 && log.handlers[1] == 0x7CD60);
  /* the pre-gate direct arm: [0x157AB0] set, type 3, released bit 0x20 */
  s = input_state();
  s.pressed = 0;
  s.released = 0x20;
  s.flag_157ab0 = 1;
  s.type = 3;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.direct_arm == 1 && out.scan_ran == 0 && out.no_edge == 0);
  s.is_1578ac = 1;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.direct_arm == 0);
  s.is_1578ac = 0;
  s.type = 4;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.direct_arm == 0);
  s.type = 3;
  s.released = 0x40;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.direct_arm == 0);
}

static void test_input_row_tail(void) {
  fifa96_outfield_input_state s = input_state();
  fifa96_outfield_input_out out;
  row_log log = {{0}, 0, 0};
  s.has_slot = 0;
  s.type = 0;
  s.forced.is_team_controlled = 1;
  s.forced.opponent_has_ball = 1;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.no_edge == 0 && out.scan_ran == 0);
  assert(out.forced == 1 && out.forced_code == 6);
  assert(out.chase == 1);
  s.type = 7;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.forced == 0 && out.chase == 0);
  s.type = 0;
  s.phase = 1;
  assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
  assert(out.forced == 0 && out.chase == 0);
  s.phase = 2;
  assert(fifa96_outfield_input_row(NULL, row_record, &log, &out) == -FIFA96_ERR_INVALID);
  assert(fifa96_outfield_input_row(&s, NULL, &log, &out) == -FIFA96_ERR_INVALID);
  assert(fifa96_outfield_input_row(&s, row_record, &log, NULL) == -FIFA96_ERR_INVALID);
}

static void test_input_row_type_gate_matches_chase(void) {
  for (uint8_t type = 0; type < 27; type++) {
    fifa96_outfield_input_state s = input_state();
    fifa96_outfield_input_out out;
    row_log log = {{0}, 0, 0};
    uint8_t next = 0xAA;
    fifa96_outfield_chase_state c;
    s.has_slot = 0;
    s.type = type;
    assert(fifa96_outfield_input_row(&s, row_record, &log, &out) == FIFA96_OK);
    c = s.chase;
    assert(fifa96_outfield_chase_gate(&c, type, 0, &next) == (out.chase != 0 ? 1 : 0));
  }
}

int main(void) {
  test_table_accessors();
  test_rule_match();
  test_rules_run();
  test_dispatch_code();
  test_forced_action();
  test_chase_action();
  test_chase_gate_type_table();
  test_input_row_no_edge_arm();
  test_input_row_scan();
  test_input_row_tail();
  test_input_row_type_gate_matches_chase();
  puts("test_outfield: all assertions passed");
  return 0;
}
