// tests/test_keeper.c — FU-74 per-team dispatch + keeper action selection (docs/ghidra/FU74_keeper_dispatch.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_keeper.h"

_Static_assert(offsetof(fifa96_dispatch_record, skip_9a) == 0, "skip_9a");
_Static_assert(offsetof(fifa96_dispatch_team, records) == 0, "records");
_Static_assert(offsetof(fifa96_dispatch_team, count) == sizeof(void *), "count");
_Static_assert(offsetof(fifa96_dispatch_iter, teams) == 0, "teams");
_Static_assert(offsetof(fifa96_dispatch_iter, team_count) == sizeof(void *), "team_count");
_Static_assert(offsetof(fifa96_keeper_state, timer) == 0, "timer");
_Static_assert(offsetof(fifa96_keeper_state, phase) == 2, "phase");
_Static_assert(offsetof(fifa96_keeper_state, controlled) == 3, "controlled");
_Static_assert(offsetof(fifa96_keeper_state, own_type_5) == 4, "own_type_5");
_Static_assert(offsetof(fifa96_keeper_state, opponent_controlled) == 5, "opponent_controlled");
_Static_assert(offsetof(fifa96_keeper_state, opponent_type_5) == 6, "opponent_type_5");

static fifa96_dispatch_record rec(uint8_t skip) {
  fifa96_dispatch_record r;
  r.skip_9a = skip;
  return r;
}

static int next_of(fifa96_dispatch_iter *it, uint32_t *team, uint32_t *index, int *keeper) {
  return fifa96_dispatch_next(it, team, index, keeper);
}

static void test_dispatch_order(void) {
  static const uint32_t expect0[10] = {0, 1, 3, 4, 5, 6, 7, 8, 9, 10};
  static const uint32_t expect1[11] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  fifa96_dispatch_record t0[11];
  fifa96_dispatch_record t1[11];
  fifa96_dispatch_team teams[2];
  fifa96_dispatch_iter it;
  uint32_t team;
  uint32_t index;
  int keeper;
  uint32_t i;
  int k;
  t0[0] = rec(1);
  t0[1] = rec(0);
  t0[2] = rec(1);
  for (i = 3; i < 11; i++) t0[i] = rec(0);
  for (i = 0; i < 11; i++) t1[i] = rec(0);
  teams[0].records = t0;
  teams[0].count = 11;
  teams[1].records = t1;
  teams[1].count = 11;
  assert(fifa96_dispatch_begin(&it, teams, 2) == FIFA96_OK);
  for (k = 0; k < 10; k++) {
    assert(next_of(&it, &team, &index, &keeper) == 1);
    assert(team == 0 && index == expect0[k] && keeper == (k == 0));
  }
  for (k = 0; k < 11; k++) {
    assert(next_of(&it, &team, &index, &keeper) == 1);
    assert(team == 1 && index == expect1[k] && keeper == (k == 0));
  }
  assert(next_of(&it, &team, &index, &keeper) == 0);
  assert(next_of(&it, &team, &index, &keeper) == 0);
  assert(fifa96_dispatch_next(&it, &team, &index, NULL) == 0);
}

static void test_dispatch_single_and_empty(void) {
  fifa96_dispatch_record only[1];
  fifa96_dispatch_team teams[2];
  fifa96_dispatch_iter it;
  uint32_t team;
  uint32_t index;
  int keeper;
  only[0] = rec(1);
  teams[0].records = only;
  teams[0].count = 1;
  teams[1].records = only;
  teams[1].count = 0;
  assert(fifa96_dispatch_begin(&it, teams, 2) == FIFA96_OK);
  assert(next_of(&it, &team, &index, &keeper) == 1);
  assert(team == 0 && index == 0 && keeper == 1);
  assert(next_of(&it, &team, &index, &keeper) == 0);
  assert(fifa96_dispatch_begin(&it, teams, 2) == FIFA96_OK);
  assert(fifa96_dispatch_next(&it, &team, &index, NULL) == 1);
  assert(team == 0 && index == 0);
  assert(fifa96_dispatch_next(&it, &team, &index, NULL) == 0);
}

static void test_dispatch_errors(void) {
  fifa96_dispatch_record r = rec(0);
  fifa96_dispatch_team teams[1];
  fifa96_dispatch_iter it;
  uint32_t team;
  uint32_t index;
  teams[0].records = &r;
  teams[0].count = 1;
  assert(fifa96_dispatch_begin(NULL, teams, 1) == -FIFA96_ERR_INVALID);
  assert(fifa96_dispatch_begin(&it, NULL, 1) == -FIFA96_ERR_INVALID);
  assert(fifa96_dispatch_begin(&it, teams, 0) == -FIFA96_ERR_INVALID);
  teams[0].records = NULL;
  teams[0].count = 1;
  assert(fifa96_dispatch_begin(&it, teams, 1) == -FIFA96_ERR_INVALID);
  teams[0].count = 0;
  assert(fifa96_dispatch_begin(&it, teams, 1) == FIFA96_OK);
  assert(fifa96_dispatch_next(NULL, &team, &index, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_dispatch_next(&it, NULL, &index, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_dispatch_next(&it, &team, NULL, NULL) == -FIFA96_ERR_INVALID);
}

static fifa96_keeper_state state(uint16_t timer, uint8_t phase, uint8_t controlled,
                                 uint8_t own_type_5, uint8_t opponent_controlled,
                                 uint8_t opponent_type_5) {
  fifa96_keeper_state s;
  s.timer = timer;
  s.phase = phase;
  s.controlled = controlled;
  s.own_type_5 = own_type_5;
  s.opponent_controlled = opponent_controlled;
  s.opponent_type_5 = opponent_type_5;
  return s;
}

static void test_keeper_select_gates(void) {
  fifa96_keeper_state s = state(0, 2, 1, 0, 0, 0);
  uint8_t next = 0xAA;
  assert(fifa96_keeper_select_action(&s, 0, 0x19, &next) == 0);
  assert(next == 0xAA);
  assert(fifa96_keeper_select_action(&s, 1, 0x19, &next) == 1);
  assert(next == 4);
}

static void test_keeper_select_timer(void) {
  fifa96_keeper_state s = state(7, 2, 1, 0, 0, 0);
  uint8_t next = 0xAA;
  assert(fifa96_keeper_select_action(&s, 1, 0x19, &next) == 1);
  assert(next == 0);
  assert(fifa96_keeper_select_action(&s, 1, 0, &next) == 0);
  assert(next == 0);
}

static void test_keeper_select_phase_and_control(void) {
  fifa96_keeper_state s = state(0, 1, 1, 0, 0, 0);
  uint8_t next = 0xAA;
  assert(fifa96_keeper_select_action(&s, 1, 0, &next) == 1);
  assert(next == 0x19);
  s = state(0, 0x10, 1, 0, 0, 0);
  assert(fifa96_keeper_select_action(&s, 1, 4, &next) == 1);
  assert(next == 0x19);
  s = state(0, 2, 0, 0, 0, 0);
  assert(fifa96_keeper_select_action(&s, 1, 4, &next) == 1);
  assert(next == 0x19);
  s = state(0, 2, 0, 0, 1, 1);
  assert(fifa96_keeper_select_action(&s, 1, 4, &next) == 1);
  assert(next == 0x19);
}

static void test_keeper_select_opponent_and_own_type(void) {
  fifa96_keeper_state s = state(0, 2, 1, 0, 1, 1);
  uint8_t next = 0xAA;
  assert(fifa96_keeper_select_action(&s, 1, 4, &next) == 1);
  assert(next == 0x19);
  s = state(0, 2, 1, 0, 1, 0);
  assert(fifa96_keeper_select_action(&s, 1, 0x19, &next) == 1);
  assert(next == 4);
  s = state(0, 2, 1, 1, 0, 0);
  next = 0xAA;
  assert(fifa96_keeper_select_action(&s, 1, 0x1A, &next) == 0);
  assert(next == 0xAA);
  s = state(0, 2, 1, 1, 1, 1);
  assert(fifa96_keeper_select_action(&s, 1, 0x1A, &next) == 1);
  assert(next == 0x19);
}

static void test_keeper_select_errors(void) {
  fifa96_keeper_state s = state(0, 2, 1, 0, 0, 0);
  uint8_t next = 0;
  assert(fifa96_keeper_select_action(NULL, 1, 0, &next) == -FIFA96_ERR_INVALID);
  assert(fifa96_keeper_select_action(&s, 1, 0, NULL) == -FIFA96_ERR_INVALID);
}

/* FU-140 re-verification of the keeper machine forced-action tail
 * (0x784EF..0x78576): the five rows are timer -> 0, phase/not-controlled ->
 * 0x19, opponent type-5 -> 0x19, own type-5 -> keep current, else 4; the
 * selection only reports a change when the row differs from the current
 * action, and the type gate suppresses the whole tail. */
static void test_keeper_forced_rows(void) {
  fifa96_keeper_state s;
  uint8_t next = 0xAA;
  s = state(1, 2, 1, 0, 0, 0);
  assert(fifa96_keeper_select_action(&s, 1, 0x1A, &next) == 1 && next == 0);
  s = state(0, 1, 1, 0, 0, 0);
  assert(fifa96_keeper_select_action(&s, 1, 0x1A, &next) == 1 && next == 0x19);
  s = state(0, 2, 0, 0, 0, 0);
  assert(fifa96_keeper_select_action(&s, 1, 0x1A, &next) == 1 && next == 0x19);
  s = state(0, 2, 1, 0, 1, 1);
  assert(fifa96_keeper_select_action(&s, 1, 0x1A, &next) == 1 && next == 0x19);
  s = state(0, 2, 1, 1, 0, 0);
  next = 0xAA;
  assert(fifa96_keeper_select_action(&s, 1, 0x1A, &next) == 0 && next == 0xAA);
  next = 0xAA;
  assert(fifa96_keeper_select_action(&s, 1, 0x19, &next) == 0 && next == 0xAA);
  s = state(0, 2, 1, 0, 0, 0);
  assert(fifa96_keeper_select_action(&s, 1, 0x1A, &next) == 1 && next == 4);
  next = 0xAA;
  assert(fifa96_keeper_select_action(&s, 1, 4, &next) == 0 && next == 0xAA);
  s = state(0, 2, 1, 0, 0, 0);
  next = 0xAA;
  assert(fifa96_keeper_select_action(&s, 0, 0x1A, &next) == 0 && next == 0xAA);
}

int main(void) {
  test_dispatch_order();
  test_dispatch_single_and_empty();
  test_dispatch_errors();
  test_keeper_select_gates();
  test_keeper_select_timer();
  test_keeper_select_phase_and_control();
  test_keeper_select_opponent_and_own_type();
  test_keeper_select_errors();
  test_keeper_forced_rows();
  puts("test_keeper: all assertions passed");
  return 0;
}
