// tests/test_outfield.c — FU-75 outfield action selection (docs/ghidra/FU75_outfield_decide.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_outfield.h"
#include "fifa96_loader/fifa96_rng.h"

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

#define ROW04_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

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

/* ===== M2 playability-legs Task 1 / OL-70: the row-04 record-visible body ==
 *
 * Fixture expectations are computed by hand from the first-hand native
 * instructions cited in the FU-142 Appendix K.3 errata (disassemble_bytes
 * 0x7E7C8..0x7F141 on /FIFA96.EXE), never from the port. */

static struct fifa96_rng row04_rng;

/* The 0x10F334/0x10F33C per-type direction bytes (first-hand: FU-139 §9 /
 * FU-142 K.1; the same tables `match_kick_dir_x/z` embeds). */
static const int8_t row04_type_x[32] = {
    0, 1, 1, 1, 0, -1, -1, -1, 1, 1, 0, -1, -1, -1, 0, 1,
    21, 3, 25, 22, 26, 21, 103, 3, 88, 91, 107, 86, 86, 80, 80, 103,
};
static const int8_t row04_type_z[32] = {
    1, 1, 0, -1, -1, -1, 0, 1, 21, 3, 25, 22, 26, 21, 103, 3,
    88, 91, 107, 86, 86, 80, 80, 103, 103, 9, 0, 120, 0, 0, 0, 0,
};

static fifa96_outfield_row04_state row04_state(void) {
  fifa96_outfield_row04_state s;
  memset(&s, 0, sizeof s);
  s.phase = 2;
  s.active = 1;
  s.code = 4;
  s.self_index = 0;
  s.team_target_index = FIFA96_OUTFIELD_ROW04_NONE;
  s.team_second_index = FIFA96_OUTFIELD_ROW04_NONE;
  s.opp_target_index = FIFA96_OUTFIELD_ROW04_NONE;
  s.opp_7c7_index = FIFA96_OUTFIELD_ROW04_NONE;
  s.lane = 0x20;
  s.bound = 0x60;
  s.side = 0;
  s.rng = &row04_rng;
  return s;
}

static void row04_no_teams(fifa96_outfield_row04_state *s) {
  s->mates = NULL;
  s->mate_count = 0;
  s->opps = NULL;
  s->opp_count = 0;
}

static void row04_expect_install(const fifa96_outfield_row04_out *out, unsigned i,
                                 uint8_t target, uint8_t code, uint8_t staged,
                                 uint8_t invoke) {
  assert(i < out->install_count);
  assert(out->installs[i].target == target);
  assert(out->installs[i].code == code);
  assert(out->installs[i].staged == staged);
  assert(out->installs[i].invoke == invoke);
}

static void test_row04_prologue(void) {
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  row04_no_teams(&s);
  s.phase = 1;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.ran == 1 && out.reset == 1);
  assert(out.target_set == 0 && out.install_count == 0 && out.team_target_set == 0);
  s.phase = 2;
  s.timer81 = 3;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.ran == 1 && out.reset == 0 && out.target_set == 0);
  assert(fifa96_outfield_row04_step(NULL, &out) == ROW04_INVALID);
  assert(fifa96_outfield_row04_step(&s, NULL) == ROW04_INVALID);
}

/* 0x7E80E..0x7E8FE: the carrier arm (actor == [0x158777]). */
static void test_row04_carrier_arm(void) {
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  static const fifa96_outfield_row04_mate mates[3] = {
      {0, 0, 0, 0, 0x20, 4, 0, 0},
      {0x10, 0, 0x10, 0, 0x10, 4, 0, 0},
      {0x20, 0, 0x20, 0, 0x30, 4, 0, 0},
  };
  row04_no_teams(&s);
  s.mates = mates;
  s.mate_count = 3;
  s.is_carrier = 1;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.team_target_set == 1 && out.team_target_index == 0);
  assert(out.install_count == 2);
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 4, 0, 1);
  row04_expect_install(&out, 1, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 3, 0, 1);
  assert(out.target_set == 0);

  /* inactive carrier: skip = 0 (the native EBX is the byte [+0x8D]) so index 0
   * is skipped and the next record wins; the second install is 0x19. */
  s.active = 0;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.team_target_index == 1);
  assert(out.install_count == 2);
  row04_expect_install(&out, 1, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 0x19, 0, 1);

  /* the nearest's byte +0x91 code gate (flat 0x110680&1): code 1 gates off and
   * no install runs (0x7E84A). */
  {
    static const fifa96_outfield_row04_mate gated[1] = {{0, 0, 0, 0, 0, 1, 0, 0}};
    s = row04_state();
    s.is_carrier = 1;
    s.mates = gated;
    s.mate_count = 1;
    assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
    assert(out.team_target_set == 1 && out.team_target_index == 0);
    assert(out.install_count == 0);
  }
}

/* 0x7E895..0x7E92E: the inactive ranked-pick arm. */
static void test_row04_inactive_pick(void) {
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  static const fifa96_outfield_row04_mate mates[2] = {
      {0, 0, 0, 0, 0x20, 4, 0, 0},
      {0x30, 0, 0x30, 0, 0x40, 4, 0, 0},
  };
  static const fifa96_outfield_row04_mate opps[2] = {
      {0, 0, 0, 0, 0x10, 0, 0, 0},
      {0, 0, 0, 0, 0x15, 0, 0, 0},
  };
  s.active = 0;
  s.mates = mates;
  s.mate_count = 2;
  s.opps = opps;
  s.opp_count = 2;
  s.team_target_index = 0;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.install_count == 1);
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 0x19, 0, 1);
  assert(out.team_target_set == 1 && out.team_target_index == 1);

  /* picked lane above the actor's (0x30 > 0x20): the active target arms run */
  s = row04_state();
  s.active = 0;
  s.mates = mates;
  s.mate_count = 2;
  {
    static const fifa96_outfield_row04_mate high[2] = {
        {0, 0, 0, 0, 0x10, 0, 0, 0},
        {0, 0, 0, 0, 0x30, 0, 0, 0},
    };
    s.opps = high;
  }
  s.opp_count = 2;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.reset == 0);
  assert(out.target_set == 1);
  assert(out.install_count == 1);                      /* the shared 0x7F133 tail */
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 5, 0, 1);
}

/* 0x7E92F..0x7E983: the active actor == team+0x7B6 hand-off. */
static void test_row04_active_second_reset(void) {
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  static const fifa96_outfield_row04_mate mates[2] = {
      {0, 0, 0, 0, 0x20, 4, 0, 0},
      {0, 0, 0, 0, 0x10, 4, 0, 0},
  };
  row04_no_teams(&s);
  s.mates = mates;
  s.mate_count = 2;
  s.team_second_index = 0;
  s.team_target_index = 1;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.reset == 1);
  assert(out.target_set == 0 && out.install_count == 0);

  /* the target's lane above the actor's skips the reset (JG 0x7E984) */
  {
    static const fifa96_outfield_row04_mate high[2] = {
        {0, 0, 0, 0, 0x20, 4, 0, 0},
        {0, 0, 0, 0, 0x30, 4, 0, 0},
    };
    s = row04_state();
    row04_no_teams(&s);
    s.mates = high;
    s.mate_count = 2;
    s.team_second_index = 0;
    s.team_target_index = 1;
    assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
    assert(out.reset == 0 && out.target_set == 1 && out.install_count == 1);
    row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 5, 0, 1);
  }
}

/* 0x7E984..0x7EC16: the target arms and the 0x7D3E4 clamp. */
static void test_row04_target_arms(void) {
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  row04_no_teams(&s);
  /* no slot, w1577F0 0: the camera triple plus word[0x1577C0/C2]<<2 */
  s.has_slot = 0;
  s.camera_x = 0x100;
  s.camera_y = 5;
  s.camera_z = 0x200;
  s.lead_x = 1;
  s.lead_z = 2;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.target_set == 1);
  assert(out.target_x == 0x100 + 4 && out.target_y == 5 &&
         out.target_z == 0x200 + 8);
  assert(out.receiver_timer == 0);

  /* |camera.z| > 0x570 and lane < 0x3C0 fires 0x79B58(actor) (0x7EBD1) */
  s.camera_z = 0x600;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.receiver_timer == 1);

  /* the clamp bounds x +/-0x720 and z +/-0xB10 */
  s.camera_x = 0x1000;
  s.camera_z = -0x2000;
  s.lead_x = 0;
  s.lead_z = 0;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.target_x == 0x720 && out.target_z == -0xB10);

  /* has-slot, lane < 0x60, no user: ECX = 1; w1577F0 0x80 with
   * word[0x1577FA] < word[0x157800] copies 0x157788 and adds the two seed-0
   * jitters: draw 512 (bit 0x20 clear -> -0x10) and draw 1829 (bit set ->
   * +0x10). */
  assert(fifa96_rng_seed(&row04_rng, 0) == FIFA96_OK);
  s = row04_state();
  row04_no_teams(&s);
  s.has_slot = 1;
  s.slot_word10 = 0;
  s.lane = 0x40;
  s.vec5788_x = 0x100;
  s.vec5788_y = 7;
  s.vec5788_z = 0x200;
  s.track_577f0 = 0x80;
  s.track_577fa = 0;
  s.track_57800 = 1;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.target_set == 1);
  assert(out.target_x == 0x100 - 0x10 && out.target_y == 7 &&
         out.target_z == 0x200 + 0x10);

  /* 0x50 < w1577F0 <= 0x70 with word[0x1577FA] < word[0x157806] copies
   * 0x157794 without a jitter (0x7EB88) */
  assert(fifa96_rng_seed(&row04_rng, 0) == FIFA96_OK);
  s.track_577f0 = 0x60;
  s.track_577fa = 0;
  s.track_57806 = 1;
  s.vec5794_x = 0x200;
  s.vec5794_y = 9;
  s.vec5794_z = 0x300;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.target_x == 0x200 && out.target_y == 9 && out.target_z == 0x300);

  /* word[0x1577FA] >= word[0x157806] skips the 0x157794 copy and the camera
   * arm (0x7EB7D JGE 0x7EBF8): the input target is clamped and kept */
  s.track_577fa = 1;
  s.track_57806 = 1;
  s.target_x = 0x11;
  s.target_y = 0x22;
  s.target_z = 0x33;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.target_x == 0x11 && out.target_y == 0x22 && out.target_z == 0x33);

  /* ECX == 0 (user present and not self, lane >= 0x60): the 0x79C20
   * slot-direction target pos + byte<<7 with y = 0 */
  s = row04_state();
  row04_no_teams(&s);
  s.has_slot = 1;
  s.slot_word10 = 0x40;
  s.user_present = 1;
  s.user_is_self = 0;
  s.lane = 0x80;
  s.pos_x = 0x40;
  s.pos_y = 0x50;
  s.pos_z = 0x50;
  s.slot_dir_x = 2;
  s.slot_dir_z = -1;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.target_set == 1);
  assert(out.target_x == 0x40 + (2 << 7) && out.target_y == 0 &&
         out.target_z == 0x50 - 0x80);
}

/* 0x7EA7B..0x7EB5C: the install-0xF gate. The path returns before the clamp
 * and the timer update (0x7EB61 RET), so target_set and timer89 stay. */
static void test_row04_install_f(void) {
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  row04_no_teams(&s);
  s.has_slot = 1;
  s.slot_word10 = 0x40;
  s.user_present = 0;
  s.lane = 0x50;
  s.bound = 0x60;
  s.timer89 = 5;
  s.delta = 2;
  s.vec5788_x = 0x10;
  s.track_577f0 = 0x80;
  s.track_577fa = 5;
  s.track_57800 = 1;
  s.track_577f2 = 2;
  s.track_57802 = 0;
  s.is_team_7c7 = 1;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.install_count == 1);
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 0x0F, 0, 1);
  assert(out.target_set == 0);
  assert(out.timer89 == 5);

  /* the same gates with lane 0x120 gate the install off (0x7EAE9) */
  s.lane = 0x120;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.install_count == 0 && out.target_set == 1);
}

/* 0x7EC97..0x7ED6A: the RNG/score/team-0x7D7 install-0xB arm. */
static void test_row04_install_b(void) {
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  static const fifa96_outfield_row04_mate opps[1] = {
      {0, 0, 0, 0, 0x10, 4, 0, 0},
  };
  assert(fifa96_rng_seed(&row04_rng, 0) == FIFA96_OK);
  row04_no_teams(&s);
  s.has_slot = 0;
  s.team_828 = 0;
  s.opps = opps;
  s.opp_count = 1;
  s.opp_target_index = 0;
  s.pos_x = 0;
  s.pos_z = 0;
  s.timer89 = 5;
  s.delta = 2;
  s.score_word[0] = 0;
  s.score_word[1] = 5;   /* own + 2 < other (0x7ED1B) */
  s.desc_e = 8;          /* threshold 8 > 1829 & 0x1F = 5 (0x7ED58) */
  s.camera_x = 0;        /* the camera arm target stays in bounds */
  s.camera_z = 0;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.install_count == 1);
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 0x0B, 0, 1);
  assert(out.timer89 == 7);   /* 0x7EC1B before the install block */
  assert(out.target_set == 1);
}

/* 0x7EC89: the no-slot 0x7E600 decision installs 0x0E and returns. */
static void test_row04_defender_decision(void) {
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  row04_no_teams(&s);
  s.has_slot = 0;
  s.team_828 = 0;
  s.lane = 0x20;
  s.pos_x = 0;
  s.pos_z = 0x7B0;             /* side 0: pos_z >= 0x7B0 (0x7E6BF) */
  s.predictor_x = 0;
  s.predictor_y = 0x30;        /* in [0x20, 0x60] (0x7E662) */
  s.predictor_z = 0x7C0;
  s.code = 4;                  /* flat 0x110680[4] & 1 (0x7E620) */
  s.is_ball_track = 0;
  s.camera_x = 0;
  s.timer89 = 5;
  s.delta = 3;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.install_count == 1);
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 0x0E, 0, 1);
  assert(out.target_set == 1 && out.reset == 0);
  assert(out.timer89 == 8);
}

/* 0x7EDCE..0x7F141: the bound/ball-height tail, the angle arm, the 0x7EEC0
 * second promote/opponent clear and the install 6/5 tail. */
static void test_row04_tail_six_five(void) {
  static const fifa96_outfield_row04_mate opps[1] = {
      {0x10, 0x10, 0x1000, 0x1000, 0x30, 4, 0, 0},
  };
  static const fifa96_outfield_row04_mate opps_near[1] = {
      {0x10, 0x10, 0, 0, 0x10, 4, 0, 0},
  };
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  assert(fifa96_rng_seed(&row04_rng, 0) == FIFA96_OK);
  row04_no_teams(&s);
  s.has_slot = 0;
  s.team_828 = 0;
  s.opps = opps;
  s.opp_count = 1;
  s.opp_target_index = 0;
  s.lane = 0x20;               /* 0x40 - timer81 >= lane, and <= bound */
  s.timer81 = 0;
  s.bound = 0x60;
  s.ball_height = 0;
  s.word6d = 0;
  s.word6f = 0x10;             /* 0x8DD70(0, 0x10) = 0 */
  s.vel_int_x = 3;
  s.vel_int_z = -2;
  s.timer89 = 1;
  s.delta = 3;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.timer89 == 4);
  assert(out.events == 1 && out.event_code == 0x1D);
  assert(out.event_x == 12 && out.event_z == -8);
  assert(out.event_dist == 0x0F);      /* 0x8DC68(12, 8) */
  assert(out.opp_7e7_clear == 1);
  assert(out.install_count == 2);
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_OTHER, 6, 0, 0);
  row04_expect_install(&out, 1, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 5, 0, 1);

  /* self lane above the other's: install 6 on self and return (0x7F108) */
  s = row04_state();
  row04_no_teams(&s);
  s.has_slot = 0;
  s.team_828 = 0;
  s.opps = opps_near;
  s.opp_count = 1;
  s.opp_target_index = 0;
  s.lane = 0x20;
  s.bound = 0x60;
  s.ball_height = 0;
  s.word6f = 0x10;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.install_count == 1);
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 6, 0, 0);

  /* actor == team+0x7B6 promotes the actor to the team target (0x7EECE) */
  s = row04_state();
  row04_no_teams(&s);
  s.has_slot = 0;
  s.team_828 = 0;
  s.team_second_index = 0;
  s.lane = 0x20;
  s.ball_height = 0;
  s.word6f = 0x10;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.team_second_set == 1 && out.team_second_index == FIFA96_OUTFIELD_ROW04_NONE);
  assert(out.team_target_set == 1 && out.team_target_index == 0);
}

/* 0x7EEEF..0x7EF41 + 0x7E528: the team+0x7E7 corner arm. */
static void test_row04_corner(void) {
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  row04_no_teams(&s);
  s.has_slot = 0;
  s.team_7e7 = 1;
  s.side = 0;
  s.team_corner_z = 0xB10;     /* side 0: z >= 0xB10 -> code 3 (0x7E575) */
  s.lane = 0x20;
  s.bound = 0x60;
  s.ball_height = 0;
  s.word6f = 0x10;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.corner == 1 && out.corner_code == 3);
  assert(out.team7e7_inc == 1);
  assert(out.install_count == 1);
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 7, 0, 1);

  /* side 1, z == -0xB10: not > -0xB10 -> code 3 (0x7E592) */
  s = row04_state();
  row04_no_teams(&s);
  s.has_slot = 0;
  s.team_7e7 = 2;
  s.side = 1;
  s.team_corner_z = -0xB10;
  s.lane = 0x20;
  s.bound = 0x60;
  s.ball_height = 0;
  s.word6f = 0x10;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.corner == 1 && out.corner_code == 3);
  assert(out.team7e7_inc == 1);

  /* team+0x7E7 == 0 with a slot and actor == [team+0x7CB]: slot restore and
   * install 7 with the staged byte 1 (0x7EF25..0x7EF33) */
  s = row04_state();
  row04_no_teams(&s);
  s.has_slot = 1;
  s.slot_word10 = 0;           /* ECX = 1 via no user and lane < 0x60 */
  s.lane = 0x20;
  s.bound = 0x60;
  s.ball_height = 0;
  s.word6f = 0x10;
  s.is_team_7cb = 1;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.slot_restore == 1);
  assert(out.install_count == 1);
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 7, 1, 1);
}

/* 0x7EF42: the byte[[rec+0x28]] == 0x13 event-0x15 arm and the 0x1577FA
 * reload (word[0x1577F2] + (word[0x1577F2] >> 2)). */
static void test_row04_row13_event(void) {
  static const fifa96_outfield_row04_mate opps[1] = {
      {0x10, 0x10, 0x1000, 0x1000, 0x10, 0, 0, 0},   /* code 0: no 6 install */
  };
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  row04_no_teams(&s);
  s.has_slot = 0;
  s.team_828 = 0;
  s.opps = opps;
  s.opp_count = 1;
  s.opp_target_index = 0;
  s.lane = 0x20;
  s.bound = 0x60;
  s.ball_height = 0;
  s.word6f = 0x10;
  s.row_byte = 0x13;
  s.type8 = 0;                 /* 0x10F334[0] = 0, 0x10F33C[0] = 1 -> <<6 */
  s.type_off_x = row04_type_x;
  s.type_off_z = row04_type_z;
  s.track_577f2 = 4;           /* reload = 4 + 1 = 5 */
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.events == 1 && out.event_code == 0x15);
  assert(out.event_track_reload == 1);
  assert(out.event_x == 0 && out.event_z == 0x40);
  assert(out.event_dist == 0x40);      /* 0x8DC68(0, 0x40) */
  assert(out.install_count == 1);
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 5, 0, 1);
}

/* 0x7E9C2..0x7E9EC: the slot bit-0x20 + word[0x1577F0] > 0x70 direct
 * 0x157770 copy, clamped at 0x7EC13 (no arm selection). */
static void test_row04_slot_camera_copy(void) {
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  row04_no_teams(&s);
  s.has_slot = 1;
  s.slot_word10 = 0x20;
  s.track_577f0 = 0x80;
  s.vec5770_x = 0x900;         /* clamps to 0x720 */
  s.vec5770_y = 5;
  s.vec5770_z = -0x1000;       /* clamps to -0xB10 */
  s.lane = 0x20;
  s.ball_height = 0;
  s.word6f = 0x10;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.target_set == 1);
  assert(out.target_x == 0x720 && out.target_y == 5 && out.target_z == -0xB10);
  assert(out.install_count == 1);
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 5, 0, 1);

  /* the same slot bit without the > 0x70 band falls through to the normal
   * ECX/user gates (0x7E9D3 JLE) */
  s.track_577f0 = 0x70;
  s.vec5770_x = 0x900;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.target_x != 0x900);
}

/* 0x7EC40..0x7EC80: the team+0x828 slot-merge request; 0x7ED79: the slot +6
 * backup request. */
static void test_row04_merge_and_backup(void) {
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  row04_no_teams(&s);
  s.has_slot = 0;
  s.team_828 = 1;              /* byte[team+0x828] */
  s.team_7e7 = 0;
  s.team_7bf = 0;
  s.merge_gate_1586d7 = 0;
  s.active = 1;
  s.lane = 0x20;
  s.ball_height = 0;
  s.word6f = 0x10;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.slot_merge == 1);

  /* byte[team+0x7E7] blocks the merge (0x7EC49) */
  s.team_7e7 = 1;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.slot_merge == 0);

  /* slot +6 nonzero -> FUN_00078A84 backup request (0x7ED79) */
  s = row04_state();
  row04_no_teams(&s);
  s.has_slot = 1;
  s.slot_word10 = 0;
  s.slot_word6 = 1;
  s.lane = 0x20;
  s.ball_height = 0;
  s.word6f = 0x10;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.slot_backup == 1);
  s.slot_word6 = 0;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.slot_backup == 0);
}

/* 0x7E89F..0x7E8AD: [opp+0x7C7] short-circuits the ranked pick. With the
 * pointer at index 1 (lane 0x30 > self 0x20) the body takes the main path;
 * the rank would have chosen index 2 (lane 0x10) and installed 0x19. */
static void test_row04_opp_7c7_pick(void) {
  fifa96_outfield_row04_state s = row04_state();
  fifa96_outfield_row04_out out;
  static const fifa96_outfield_row04_mate mates[1] = {
      {0, 0, 0, 0, 0x20, 4, 0, 0},
  };
  static const fifa96_outfield_row04_mate opps[3] = {
      {0, 0, 0, 0, 0x10, 0, 0, 0},
      {0, 0, 0, 0, 0x30, 0, 0, 0},
      {0, 0, 0, 0, 0x10, 0, 0, 0},
  };
  s.active = 0;
  s.mates = mates;
  s.mate_count = 1;
  s.opps = opps;
  s.opp_count = 3;
  s.opp_7c7_index = 1;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.install_count == 1);
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 5, 0, 1);

  /* the [opp+0x7C7] record at index 2 wins the 0x19 install */
  s.opp_7c7_index = 2;
  assert(fifa96_outfield_row04_step(&s, &out) == FIFA96_OK);
  assert(out.install_count == 1);
  row04_expect_install(&out, 0, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 0x19, 0, 1);
}

/* ===== M2 playability-legs Task 2 / OL-70a: the row-08 record-visible body ==
 *
 * Fixture expectations are computed by hand from the first-hand native
 * instructions cited in the FU-142 Appendix K.6 (disassemble_bytes
 * 0x81068..0x814AF on /FIFA96.EXE), never from the port. */

#define ROW08_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static struct fifa96_rng row08_rng;

/* Defaults: phase 2, stage 0, active, slotted, lane 0x20 (the face
 * entity_angle(0x20, 0) = 0x100 -> octant 2), all gates zero. */
static fifa96_outfield_row08_state row08_state(void) {
  fifa96_outfield_row08_state s;
  memset(&s, 0, sizeof s);
  s.phase = 2;
  s.stage = 0;
  s.active = 1;
  s.has_slot = 1;
  s.lane = 0x20;
  s.word6d = 0;
  s.type8 = 0;
  s.type_off_x = row04_type_x;
  s.type_off_z = row04_type_z;
  s.rng = &row08_rng;
  return s;
}

/* 0x81073..0x810C3: the prologue and the stage dispatch. */
static void test_row08_prologue(void) {
  fifa96_outfield_row08_state s = row08_state();
  fifa96_outfield_row08_out out;
  s.phase = 1;
  s.timer89 = 7;
  s.delta = 5;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.ran == 0 && out.stage92 == 0);
  assert(out.timer89 == 7);   /* the phase gate precedes the += delta */
  assert(out.target_set == 0 && out.face == 0);

  /* phase 2, stage >= 3: the prologue adds delta then returns. */
  s = row08_state();
  s.stage = 3;
  s.timer89 = 7;
  s.delta = 5;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.reset == 0 && out.stage92 == 3 && out.timer89 == 12);
  assert(out.face == 0 && out.scan == 0 && out.target_set == 0);

  assert(fifa96_outfield_row08_step(NULL, &out) == ROW08_INVALID);
  assert(fifa96_outfield_row08_step(&s, NULL) == ROW08_INVALID);
}

/* 0x810CC..0x81113: the stage-0 inactive arm (reset + the team pointer
 * clears when the actor is that pointer). */
static void test_row08_inactive_arm(void) {
  fifa96_outfield_row08_state s = row08_state();
  fifa96_outfield_row08_out out;
  s.active = 0;
  s.is_team_target = 1;
  s.is_team_second = 1;
  s.timer89 = 7;
  s.delta = 5;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.ran == 0 && out.stage92 == 0);
  assert(out.clear_team_target == 1 && out.clear_team_second == 1);
  assert(out.timer89 == 12);   /* the prologue write; the reset zero is the binder's */

  s.is_team_target = 0;
  s.is_team_second = 1;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.clear_team_target == 0 && out.clear_team_second == 1);

  s.is_team_target = 1;
  s.is_team_second = 0;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.clear_team_target == 1 && out.clear_team_second == 0);

  s.is_team_target = 0;
  s.is_team_second = 0;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.clear_team_target == 0 && out.clear_team_second == 0);
}

/* 0x81114..0x811DC: the stage-0 active camera+lead copy, the 0x79C50 face,
 * the 0x6E598 request, the +0x9E latch and the single stage increment. */
static void test_row08_stage0_camera_face(void) {
  fifa96_outfield_row08_state s = row08_state();
  fifa96_outfield_row08_out out;
  s.camera_x = 7;
  s.camera_y = 8;
  s.camera_z = 9;
  s.lead_x = 2;    /* +0x4D += 2 << 3 */
  s.lead_z = 3;    /* +0x55 += 3 << 3 */
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.target_set == 1);
  assert(out.target_x == 23 && out.target_y == 8 && out.target_z == 33);
  assert(out.face == 1 && out.face_angle == 0x100 && out.face_octant == 2);
  assert(out.anim == 1 && out.ran == 1);
  assert(out.stage92 == 1 && out.timer89 == 0);
  assert(out.byte_15877d_set == 1 && out.byte_15877d == 0);
  assert(out.scan == 0 && out.reset == 0 && out.receiver_timer == 0);

  /* the zero direction guard (0x79C59): both words zero leaves +0x7D/+0x8E
   * untouched (no face write). */
  s = row08_state();
  s.lane = 0;
  s.word6d = 0;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.face == 0 && out.face_angle == 0 && out.face_octant == 0);
  assert(out.stage92 == 1);
}

/* 0x8114C..0x8117A: the no-slot receiver gate (lane > 0x50 or ball height >
 * 0x38), the 0x79B58 request and the timer89 > 0x3C reset. */
static void test_row08_receiver_gate(void) {
  fifa96_outfield_row08_state s = row08_state();
  fifa96_outfield_row08_out out;
  s.has_slot = 0;
  s.lane = 0x51;              /* > 0x50 */
  s.timer89 = 0x10;
  s.delta = 0x20;             /* updated timer 0x30 <= 0x3C -> plain return */
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.receiver_timer == 1 && out.reset == 0 && out.face == 0);
  assert(out.stage92 == 0 && out.timer89 == 0x30);
  assert(out.target_set == 1);   /* the camera copy precedes the slot test */

  s.timer89 = 0x20;
  s.delta = 0x20;             /* updated timer 0x40 > 0x3C -> reset */
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.receiver_timer == 1 && out.reset == 1 && out.face == 0);

  /* lane == 0x50 and height == 0x38: the gate is skipped (JLE/JG) */
  s = row08_state();
  s.has_slot = 0;
  s.lane = 0x50;
  s.ball_height = 0x38;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.receiver_timer == 0 && out.face == 1 && out.stage92 == 1);

  /* height 0x39 crosses the signed dword gate */
  s.ball_height = 0x39;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.receiver_timer == 1 && out.face == 0 && out.stage92 == 0);
}

/* 0x811D6..0x81210: the stage-1 gate and the +0x44/+0x3D continuation. */
static void test_row08_stage1_gates(void) {
  fifa96_outfield_row08_state s = row08_state();
  fifa96_outfield_row08_out out;
  /* the stage-0 completion reaches the gate with [0x15877D] = 0: byte +0x44
   * set increments the stage a second time. */
  s.byte44 = 1;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.stage92 == 2 && out.timer89 == 0 && out.scan == 0);
  assert(out.byte_15877d_set == 1 && out.byte_15877d == 0);

  /* byte +0x3D != 1: plain return at stage 1 */
  s = row08_state();
  s.byte3d = 0;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.stage92 == 1 && out.scan == 0);
  assert(out.face == 1 && out.ran == 1);

  /* stage-1 entry: the input [0x15877D] advances the stage */
  s = row08_state();
  s.stage = 1;
  s.byte_15877d = 1;
  s.timer89 = 5;
  s.delta = 3;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.stage92 == 2 && out.timer89 == 0 && out.face == 0 && out.ran == 0);

  /* stage-1 entry: byte +0x44 advances the stage */
  s.byte_15877d = 0;
  s.byte44 = 1;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.stage92 == 2 && out.timer89 == 0);
}

/* 0x81216..0x81472: the projection scan. The 0x114E04 fold stores the low
 * word of the table value and the 0x795A4 callers sign-extend it, so word7d
 * 0x40 gives sine1 = +25079 (0x61F7) and sine2 = +60547 (0xEC83) -> int16
 * -4989; the folds are +6/-1 at offset 0x10. With camera x 0x20, pos 0 and
 * q 0x20 the first offset distance is 0x20 (miss) and the second 0x1A
 * (0x20 > 0x1A -> hit, metric 0x1A). */
static void test_row08_scan_rec_arm(void) {
  fifa96_outfield_row08_state s = row08_state();
  fifa96_outfield_row08_out out;
  assert(fifa96_rng_seed(&row08_rng, 0) == FIFA96_OK);
  s.lane = 0;          /* the 0x79C59 zero guard: no face write */
  s.word6d = 0;
  s.word7d = 0x40;
  s.byte3d = 1;
  s.camera_x = 0x20;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.scan == 1 && out.scan_hit == 1 && out.scan_dist == 0x1A);
  assert(out.face == 0);
  assert(out.user_row_4a == 0);
  assert(out.event_code == 0x16 && out.events == 1);
  assert(out.ball_stage == 1 && out.ball_stage_dist == 0x1A);
  assert(out.ball_stage_x == 0 && out.ball_stage_z == 0xA0);   /* type 0 */
  assert(out.byte_15877d_set == 1 && out.byte_15877d == 1);
  assert(out.stage92 == 2 && out.timer89 == 0 && out.ran == 1);
  assert(out.event_sound == 0x190);   /* seed-0 first draw 512 & 0x7F = 0 */

  /* a scan with no hit keeps the stage-0 completion writes and returns with
   * the stage at 1 (timer89 0 from 0x811B6). */
  s = row08_state();
  s.lane = 0;
  s.word6d = 0;
  s.word7d = 0x40;
  s.byte3d = 1;
  s.camera_x = 0x100;   /* every distance >= 0xE6 > q = 0x20 */
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.scan == 1 && out.scan_hit == 0 && out.scan_dist == 0);
  assert(out.stage92 == 1 && out.timer89 == 0 && out.ran == 1);
  assert(out.ball_stage == 0 && out.event_code == 0);

  /* stage-1 entry: no face/anim/ran writes, the record arm still fires; the
   * miss keeps the prologue's timer89. */
  s = row08_state();
  s.stage = 1;
  s.byte3d = 1;
  s.word7d = 0x40;
  s.camera_x = 0x20;
  s.timer89 = 5;
  s.delta = 3;
  assert(fifa96_rng_seed(&row08_rng, 0) == FIFA96_OK);
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.scan_hit == 1 && out.face == 0 && out.ran == 0);
  assert(out.stage92 == 2 && out.timer89 == 0);
  assert(out.byte_15877d_set == 1 && out.byte_15877d == 1);

  s = row08_state();
  s.stage = 1;
  s.byte3d = 1;
  s.word7d = 0x40;
  s.camera_x = 0x100;
  s.timer89 = 5;
  s.delta = 3;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.scan_hit == 0 && out.stage92 == 1 && out.timer89 == 8);

  /* a hit on a NULL rng is invalid (the 0x92AC8 draw cannot run) */
  s = row08_state();
  s.lane = 0;
  s.word6d = 0;
  s.word7d = 0x40;
  s.byte3d = 1;
  s.camera_x = 0x20;
  s.rng = NULL;
  assert(fifa96_outfield_row08_step(&s, &out) == ROW08_INVALID);
}

/* 0x813A2..0x81423: the [0x157A83] user arm (user present, its row byte 0x4A)
 * skips the 0x92820/0x7A490 staging and the [0x15877D] write. */
static void test_row08_scan_user_arm(void) {
  fifa96_outfield_row08_state s = row08_state();
  fifa96_outfield_row08_out out;
  assert(fifa96_rng_seed(&row08_rng, 0) == FIFA96_OK);
  s.lane = 0;
  s.word6d = 0;
  s.word7d = 0x40;
  s.byte3d = 1;
  s.camera_x = 0x20;
  s.user_present = 1;
  s.user_is_opp_target = 1;
  s.user_row_byte = 0x4A;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.scan_hit == 1 && out.user_row_4a == 1);
  assert(out.event_code == 0 && out.ball_stage == 0);
  assert(out.byte_15877d_set == 1 && out.byte_15877d == 0);   /* 0x811CA only */
  assert(out.events == 1 && out.event_sound == 0x190);
  assert(out.stage92 == 2 && out.timer89 == 0);

  /* the user arm also needs the row byte 0x4A: 0x49 takes the record arm. */
  s = row08_state();
  s.lane = 0;
  s.word6d = 0;
  s.word7d = 0x40;
  s.byte3d = 1;
  s.camera_x = 0x20;
  s.user_present = 1;
  s.user_is_opp_target = 1;
  s.user_row_byte = 0x49;
  assert(fifa96_rng_seed(&row08_rng, 0) == FIFA96_OK);
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.user_row_4a == 0 && out.event_code == 0x16 && out.ball_stage == 1);
  assert(out.byte_15877d_set == 1 && out.byte_15877d == 1);
}

/* 0x81233..0x81279: the descriptor delta and the q clamp. The self
 * descriptor bytes +0xC/+0x16 give 6*delta + 0x20; desc_c = 0x10,
 * desc_16 = 0 -> delta 0x10 -> q = 6*0x10 + 0x20 = 0x80 -> clamped 0x40. */
static void test_row08_scan_radius(void) {
  fifa96_outfield_row08_state s = row08_state();
  fifa96_outfield_row08_out out;
  s.lane = 0;
  s.word6d = 0;
  s.word7d = 0x40;
  s.byte3d = 1;
  s.desc_c = 0x10;
  s.desc_16 = 0;
  s.camera_x = 0x30;   /* offset 0 distance 0x30 < 0x40 -> hit at offset 0 */
  assert(fifa96_rng_seed(&row08_rng, 0) == FIFA96_OK);
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.scan_hit == 1 && out.scan_dist == 0x30);

  /* the +0x15872F process byte shifts the radius: 6*0 + 1*8 + 0x20 = 0x28
   * with camera x 0x30 makes offset 0 (0x30) and offset 0x10 (0x2A) miss
   * (0x28 <= d) and offset 0x20 (0x24) hit. */
  s = row08_state();
  s.lane = 0;
  s.word6d = 0;
  s.word7d = 0x40;
  s.byte3d = 1;
  s.byte_15872f = 1;
  s.camera_x = 0x30;
  assert(fifa96_rng_seed(&row08_rng, 0) == FIFA96_OK);
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.scan_hit == 1 && out.scan_dist == 0x24);

  /* the user/opp-descriptor delta subtracts; with self 0 and opp 0x10 the
   * q word goes negative and clamps to 8, so every loop distance misses. */
  s = row08_state();
  s.lane = 0;
  s.word6d = 0;
  s.word7d = 0x40;
  s.byte3d = 1;
  s.user_present = 1;
  s.user_is_opp_target = 1;
  s.desc_opp_c = 0x10;
  s.camera_x = 0x30;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.scan == 1 && out.scan_hit == 0);   /* q = 8; all d >= 0x1A -> miss */
}

/* 0x8147C..0x814AF: the stage-2 snap and the +0x44 / [0x15872F] tail. */
static void test_row08_stage2(void) {
  fifa96_outfield_row08_state s = row08_state();
  fifa96_outfield_row08_out out;
  s.stage = 2;
  s.byte_15877d = 1;
  s.pos_x = 1;
  s.pos_y = 2;
  s.pos_z = 3;
  s.timer89 = 4;
  s.delta = 2;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.snap == 1 && out.target_set == 1);
  assert(out.target_x == 1 && out.target_y == 2 && out.target_z == 3);
  assert(out.byte_15877d_set == 1 && out.byte_15877d == 0);
  assert(out.reset == 0 && out.timer89 == 6 && out.stage92 == 2);

  /* byte +0x44 set: reset and the [0x15872F] increment when the actor is not
   * [0x1577CA]. */
  s.byte44 = 1;
  s.byte_15872f = 5;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.byte_15872f_set == 1 && out.byte_15872f == 6);

  /* the [0x1577CA] actor skips the increment but still resets. */
  s.is_ball_track = 1;
  s.byte_15872f = 5;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.reset == 1 && out.byte_15872f_set == 0 && out.byte_15872f == 5);

  /* byte +0x44 clear: no reset/increment */
  s.byte44 = 0;
  s.is_ball_track = 0;
  assert(fifa96_outfield_row08_step(&s, &out) == FIFA96_OK);
  assert(out.reset == 0 && out.byte_15872f_set == 0);
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
  test_row04_prologue();
  test_row04_carrier_arm();
  test_row04_inactive_pick();
  test_row04_active_second_reset();
  test_row04_target_arms();
  test_row04_install_f();
  test_row04_install_b();
  test_row04_defender_decision();
  test_row04_tail_six_five();
  test_row04_corner();
  test_row04_row13_event();
  test_row04_slot_camera_copy();
  test_row04_merge_and_backup();
  test_row04_opp_7c7_pick();
  test_row08_prologue();
  test_row08_inactive_arm();
  test_row08_stage0_camera_face();
  test_row08_receiver_gate();
  test_row08_stage1_gates();
  test_row08_scan_rec_arm();
  test_row08_scan_user_arm();
  test_row08_scan_radius();
  test_row08_stage2();
  puts("test_outfield: all assertions passed");
  return 0;
}
