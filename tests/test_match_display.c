// tests/test_match_display.c — FU-69 in-match display/menu arms.
// Expected behavior is derived in docs/ghidra/FU69_display_menu_arms.md:
// FUN_000537F8 reset (0x4E570 record), FUN_00053DC4/0x53DE0 suspend pair,
// FUN_00053BB8 state-1 timer gate.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_match_display.h"

static int call_seq[8];
static int call_count;
static int enter_rc, leave_rc, refresh_rc;
static int enter_saw_suspend, leave_saw_suspend;

static void reset_calls(void) {
  memset(call_seq, 0, sizeof call_seq);
  call_count = 0;
  enter_rc = leave_rc = refresh_rc = 0;
  enter_saw_suspend = leave_saw_suspend = -1;
}

static int on_enter(void *user) {
  fifa96_match_display *d = user;
  call_seq[call_count++] = 1;
  enter_saw_suspend = d->suspend;
  return enter_rc;
}

static int on_leave(void *user) {
  fifa96_match_display *d = user;
  call_seq[call_count++] = 2;
  leave_saw_suspend = d->suspend;
  return leave_rc;
}

static int on_refresh(void *user) {
  (void)user;
  call_seq[call_count++] = 3;
  return refresh_rc;
}

static fifa96_match_display_backend backend(void) {
  fifa96_match_display_backend b;
  b.enter = on_enter;
  b.leave = on_leave;
  b.refresh = on_refresh;
  return b;
}

static void test_init_nulls(void) {
  assert(fifa96_match_display_init(NULL, 0) == -FIFA96_ERR_INVALID);
}

static void test_init_zeroes_and_duration(void) {
  fifa96_match_display d;
  memset(&d, 0xAA, sizeof d);
  assert(fifa96_match_display_init(&d, 2) == FIFA96_OK);
  assert(d.suspend == 0);
  assert(d.state == 0);
  assert(d.value == 0);
  assert(d.timer == 0);
  assert(d.duration == FIFA96_MATCH_DISPLAY_DURATION);
  assert(d.selector == 2);
}

static void test_init_phase_0x10_arm(void) {
  fifa96_match_display d;
  memset(&d, 0xAA, sizeof d);
  assert(fifa96_match_display_init(&d, 0x10) == FIFA96_OK);
  assert(d.suspend == 1);
  assert(d.value == 0x10);
  assert(d.selector == 0x0C);
  assert(d.duration == FIFA96_MATCH_DISPLAY_DURATION);
  assert(d.timer == 0);
  assert(d.state == 0);
}

static void test_suspend_enter_is_edge_triggered(void) {
  fifa96_match_display d;
  fifa96_match_display_backend b = backend();
  assert(fifa96_match_display_init(&d, 0) == FIFA96_OK);
  reset_calls();
  assert(fifa96_match_display_suspend_enter(&d, &b, &d) == FIFA96_OK);
  assert(d.suspend == 1);
  assert(call_count == 1);
  assert(call_seq[0] == 1);
  assert(enter_saw_suspend == 0);
  assert(fifa96_match_display_suspend_enter(&d, &b, &d) == FIFA96_OK);
  assert(d.suspend == 1);
  assert(call_count == 1);
}

static void test_suspend_enter_error_leaves_unsuspended(void) {
  fifa96_match_display d;
  fifa96_match_display_backend b = backend();
  assert(fifa96_match_display_init(&d, 0) == FIFA96_OK);
  reset_calls();
  enter_rc = -FIFA96_ERR_IO;
  assert(fifa96_match_display_suspend_enter(&d, &b, &d) == -FIFA96_ERR_IO);
  assert(d.suspend == 0);
  assert(call_count == 1);
}

static void test_suspend_leave_order_and_edge(void) {
  fifa96_match_display d;
  fifa96_match_display_backend b = backend();
  assert(fifa96_match_display_init(&d, 0) == FIFA96_OK);
  reset_calls();
  assert(fifa96_match_display_suspend_enter(&d, &b, &d) == FIFA96_OK);
  assert(fifa96_match_display_suspend_leave(&d, &b, &d) == FIFA96_OK);
  assert(d.suspend == 0);
  assert(call_count == 3);
  assert(call_seq[0] == 1);
  assert(call_seq[1] == 2);
  assert(call_seq[2] == 3);
  assert(leave_saw_suspend == 1);
  assert(fifa96_match_display_suspend_leave(&d, &b, &d) == FIFA96_OK);
  assert(call_count == 3);
}

static void test_suspend_leave_error_still_full_sequence(void) {
  fifa96_match_display d;
  fifa96_match_display_backend b = backend();
  assert(fifa96_match_display_init(&d, 0) == FIFA96_OK);
  reset_calls();
  assert(fifa96_match_display_suspend_enter(&d, &b, &d) == FIFA96_OK);
  leave_rc = -FIFA96_ERR_IO;
  refresh_rc = -FIFA96_ERR_NOT_FOUND;
  assert(fifa96_match_display_suspend_leave(&d, &b, &d) == -FIFA96_ERR_IO);
  assert(d.suspend == 0);
  assert(call_count == 3);
  assert(call_seq[1] == 2);
  assert(call_seq[2] == 3);
}

static void test_suspend_nulls(void) {
  fifa96_match_display d;
  assert(fifa96_match_display_init(&d, 0) == FIFA96_OK);
  assert(fifa96_match_display_suspend_enter(NULL, NULL, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_display_suspend_leave(NULL, NULL, NULL) == -FIFA96_ERR_INVALID);
}

static void test_suspend_without_backend_is_state_only(void) {
  fifa96_match_display d;
  assert(fifa96_match_display_init(&d, 0) == FIFA96_OK);
  assert(fifa96_match_display_suspend_enter(&d, NULL, NULL) == FIFA96_OK);
  assert(d.suspend == 1);
  assert(fifa96_match_display_suspend_leave(&d, NULL, NULL) == FIFA96_OK);
  assert(d.suspend == 0);
}

static void test_update_gated_on_state_1(void) {
  fifa96_match_display d;
  assert(fifa96_match_display_init(&d, 0) == FIFA96_OK);
  assert(fifa96_match_display_update(&d, 30, 0) == 0);
  assert(d.timer == 0);
  d.state = 7;
  assert(fifa96_match_display_update(&d, 30, 0) == 0);
  assert(d.timer == 0);
}

static void test_update_accumulates_until_deadline(void) {
  fifa96_match_display d;
  assert(fifa96_match_display_init(&d, 0) == FIFA96_OK);
  d.state = 1;
  assert(fifa96_match_display_update(&d, 30, 0) == 0);
  assert(d.timer == 30);
  assert(fifa96_match_display_update(&d, 49, 0) == 0);
  assert(d.timer == 79);
  assert(fifa96_match_display_update(&d, 1, 0) == 1);
  assert(d.timer == 0);
  assert(d.duration == 0);
}

static void test_update_forced_resets_even_below_deadline(void) {
  fifa96_match_display d;
  assert(fifa96_match_display_init(&d, 0) == FIFA96_OK);
  d.state = 1;
  assert(fifa96_match_display_update(&d, 1, 0) == 0);
  assert(fifa96_match_display_update(&d, 1, 1) == 1);
  assert(d.timer == 0);
  assert(d.duration == 0);
}

static void test_update_elapsed_passthrough_and_null(void) {
  fifa96_match_display d;
  assert(fifa96_match_display_init(&d, 0) == FIFA96_OK);
  d.state = 1;
  assert(fifa96_match_display_update(&d, -1, 0) == 0);
  assert(d.timer == -1);
  assert(fifa96_match_display_update(NULL, 1, 0) == -FIFA96_ERR_INVALID);
}

int main(void) {
  test_init_nulls();
  test_init_zeroes_and_duration();
  test_init_phase_0x10_arm();
  test_suspend_enter_is_edge_triggered();
  test_suspend_enter_error_leaves_unsuspended();
  test_suspend_leave_order_and_edge();
  test_suspend_leave_error_still_full_sequence();
  test_suspend_nulls();
  test_suspend_without_backend_is_state_only();
  test_update_gated_on_state_1();
  test_update_accumulates_until_deadline();
  test_update_forced_resets_even_below_deadline();
  test_update_elapsed_passthrough_and_null();
  printf("test_match_display OK\n");
  return 0;
}
