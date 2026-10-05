// tests/test_match_lifecycle.c — FU-64 match start/teardown lifecycle (docs/ghidra/FU64_match_lifecycle.md).
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_match_lifecycle.h"
#include "fifa96_loader/fifa96_match_pace.h"

struct track {
  struct fifa96_match_lifecycle *lc;
  struct fifa96_match_pace *pace;
  int register_calls;
  int cancel_calls;
  int teardown_calls;
  int post_calls;
  int fail_register;
  int fail_cancel;
  int fail_teardown;
  int fail_post;
};

static int register_cb(void *ctx) {
  struct track *t = ctx;
  assert(t->lc->active == 1);
  assert(t->lc->registered == 0);
  assert(t->pace->hold == 0);
  assert(t->pace->pending == 0);
  assert(t->pace->acc == 0);
  t->register_calls++;
  return t->fail_register;
}

static int cancel_cb(void *ctx) {
  struct track *t = ctx;
  assert(t->lc->active == 0);
  assert(t->pace->hold == 1);
  assert(t->cancel_calls == t->teardown_calls);
  t->cancel_calls++;
  return t->fail_cancel;
}

static int teardown_cb(void *ctx) {
  struct track *t = ctx;
  assert(t->cancel_calls == t->teardown_calls + 1);
  assert(t->lc->registered == 0);
  assert(t->lc->unload == 0);
  t->teardown_calls++;
  return t->fail_teardown;
}

static int post_cb(void *ctx) {
  struct track *t = ctx;
  assert(t->lc->target == 2);
  assert(t->teardown_calls == t->post_calls + 1);
  t->post_calls++;
  return t->fail_post;
}

static struct fifa96_match_lifecycle_backend make_backend(struct track *t) {
  struct fifa96_match_lifecycle_backend b;
  b.register_callback = register_cb;
  b.cancel_callback = cancel_cb;
  b.teardown = teardown_cb;
  b.post_exit = post_cb;
  b.ctx = t;
  return b;
}

static void test_init_zeroes(void) {
  struct fifa96_match_lifecycle lc;
  lc.selector = 7;
  lc.active = 7;
  lc.screen = 7;
  lc.target = 7;
  lc.prev = 7;
  lc.cadence = 7;
  lc.flag = 7;
  lc.unload = 7;
  lc.registered = 7;
  lc.backend = (const struct fifa96_match_lifecycle_backend *)0x1;
  fifa96_match_lifecycle_init(&lc);
  assert(lc.selector == 0);
  assert(lc.active == 0);
  assert(lc.screen == 0);
  assert(lc.target == 0);
  assert(lc.prev == 0);
  assert(lc.cadence == 0);
  assert(lc.flag == 0);
  assert(lc.unload == 0);
  assert(lc.registered == 0);
  assert(lc.backend == NULL);
}

static void test_begin_order_and_state(void) {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct track t;
  struct fifa96_match_lifecycle_backend b;
  memset(&t, 0, sizeof(t));
  t.lc = &lc;
  t.pace = &pace;
  b = make_backend(&t);
  fifa96_match_lifecycle_init(&lc);
  fifa96_match_pace_init(&pace);
  pace.acc = 0x35C;
  pace.pending = 3;
  pace.hold = 1;
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 0) == 0);
  assert(lc.selector == 0);
  assert(lc.active == 1);
  assert(lc.screen == 0);
  assert(lc.target == 0);
  assert(lc.prev == 0);
  assert(lc.cadence == 0);
  assert(lc.flag == 0);
  assert(lc.registered == 1);
  assert(lc.backend == &b);
  assert(pace.acc == 0);
  assert(pace.pending == 0);
  assert(pace.hold == 0);
  assert(t.register_calls == 1);
  assert(t.cancel_calls == 0);
  assert(t.teardown_calls == 0);
  assert(t.post_calls == 0);
}

static void test_begin_selector_nonzero(void) {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct track t;
  struct fifa96_match_lifecycle_backend b;
  memset(&t, 0, sizeof(t));
  t.lc = &lc;
  t.pace = &pace;
  b = make_backend(&t);
  fifa96_match_lifecycle_init(&lc);
  fifa96_match_pace_init(&pace);
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 1) == 0);
  assert(lc.selector == 1);
  assert(lc.active == 1);
}

static void test_begin_twice_is_state_error(void) {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct track t;
  struct fifa96_match_lifecycle_backend b;
  memset(&t, 0, sizeof(t));
  t.lc = &lc;
  t.pace = &pace;
  b = make_backend(&t);
  fifa96_match_lifecycle_init(&lc);
  fifa96_match_pace_init(&pace);
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 0) == 0);
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 4) == -FIFA96_ERR_STATE);
  assert(t.register_calls == 1);
  assert(lc.selector == 0);
  assert(lc.active == 1);
  assert(lc.registered == 1);
}

static void test_begin_register_failure_rolls_back(void) {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct track t;
  struct fifa96_match_lifecycle_backend b;
  memset(&t, 0, sizeof(t));
  t.lc = &lc;
  t.pace = &pace;
  t.fail_register = -FIFA96_ERR_FULL;
  b = make_backend(&t);
  fifa96_match_lifecycle_init(&lc);
  fifa96_match_pace_init(&pace);
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 0) == -FIFA96_ERR_FULL);
  assert(t.register_calls == 1);
  assert(lc.active == 0);
  assert(lc.registered == 0);
  assert(pace.hold == 1);
}

static void test_mark_over_and_resolve(void) {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct track t;
  struct fifa96_match_lifecycle_backend b;
  memset(&t, 0, sizeof(t));
  t.lc = &lc;
  t.pace = &pace;
  b = make_backend(&t);
  fifa96_match_lifecycle_init(&lc);
  fifa96_match_pace_init(&pace);
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 0) == 0);
  assert(fifa96_match_lifecycle_resolve_over(&lc) == 0);
  assert(lc.screen == 0);
  lc.cadence = 9;
  assert(fifa96_match_lifecycle_mark_over(&lc) == 0);
  assert(lc.screen == FIFA96_MATCH_SCREEN_OVER);
  assert(fifa96_match_lifecycle_resolve_over(&lc) == 1);
  assert(lc.screen == FIFA96_MATCH_SCREEN_POST);
  assert(lc.cadence == 0);
  assert(fifa96_match_lifecycle_resolve_over(&lc) == 0);
  assert(lc.screen == FIFA96_MATCH_SCREEN_POST);
}

static void test_request_exit_and_should_exit(void) {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct track t;
  struct fifa96_match_lifecycle_backend b;
  memset(&t, 0, sizeof(t));
  t.lc = &lc;
  t.pace = &pace;
  b = make_backend(&t);
  fifa96_match_lifecycle_init(&lc);
  fifa96_match_pace_init(&pace);
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 0) == 0);
  assert(fifa96_match_lifecycle_should_exit(&lc) == 0);
  assert(fifa96_match_lifecycle_request_exit(&lc) == 0);
  assert(lc.screen == FIFA96_MATCH_SCREEN_EXIT);
  assert(lc.target == 3);
  assert(fifa96_match_lifecycle_should_exit(&lc) == 1);
}

static void test_end_order_and_post_exit(void) {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct track t;
  struct fifa96_match_lifecycle_backend b;
  memset(&t, 0, sizeof(t));
  t.lc = &lc;
  t.pace = &pace;
  b = make_backend(&t);
  fifa96_match_lifecycle_init(&lc);
  fifa96_match_pace_init(&pace);
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 0) == 0);
  lc.unload = 0x3783C;
  assert(fifa96_match_lifecycle_request_exit(&lc) == 0);
  assert(fifa96_match_lifecycle_end(&lc, &pace) == 1);
  assert(t.cancel_calls == 1);
  assert(t.teardown_calls == 1);
  assert(t.post_calls == 1);
  assert(lc.active == 0);
  assert(lc.registered == 0);
  assert(lc.unload == 0);
  assert(lc.target == 2);
  assert(pace.hold == 1);
}

static void test_end_without_exit_still_tears_down(void) {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct track t;
  struct fifa96_match_lifecycle_backend b;
  memset(&t, 0, sizeof(t));
  t.lc = &lc;
  t.pace = &pace;
  b = make_backend(&t);
  fifa96_match_lifecycle_init(&lc);
  fifa96_match_pace_init(&pace);
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 0) == 0);
  assert(fifa96_match_lifecycle_end(&lc, &pace) == 0);
  assert(t.cancel_calls == 1);
  assert(t.teardown_calls == 1);
  assert(t.post_calls == 0);
  assert(lc.target == 0);
}

static void test_end_without_begin_is_state_error(void) {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct track t;
  memset(&t, 0, sizeof(t));
  t.lc = &lc;
  t.pace = &pace;
  fifa96_match_lifecycle_init(&lc);
  fifa96_match_pace_init(&pace);
  assert(fifa96_match_lifecycle_end(&lc, &pace) == -FIFA96_ERR_STATE);
  assert(t.cancel_calls == 0);
  assert(t.teardown_calls == 0);
  assert(pace.hold == 0);
}

static void test_begin_end_balance_across_reuse(void) {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct track t;
  struct fifa96_match_lifecycle_backend b;
  memset(&t, 0, sizeof(t));
  t.lc = &lc;
  t.pace = &pace;
  b = make_backend(&t);
  fifa96_match_lifecycle_init(&lc);
  fifa96_match_pace_init(&pace);
  for (int i = 0; i < 3; i++) {
    assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, (uint32_t)i) == 0);
    assert(fifa96_match_lifecycle_request_exit(&lc) == 0);
    assert(fifa96_match_lifecycle_end(&lc, &pace) == 1);
  }
  assert(t.register_calls == 3);
  assert(t.cancel_calls == 3);
  assert(t.teardown_calls == 3);
  assert(t.post_calls == 3);
  assert(lc.active == 0);
  assert(lc.registered == 0);
}

static void test_end_propagates_callback_error(void) {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct track t;
  struct fifa96_match_lifecycle_backend b;
  memset(&t, 0, sizeof(t));
  t.lc = &lc;
  t.pace = &pace;
  t.fail_teardown = -FIFA96_ERR_IO;
  b = make_backend(&t);
  fifa96_match_lifecycle_init(&lc);
  fifa96_match_pace_init(&pace);
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 0) == 0);
  assert(fifa96_match_lifecycle_request_exit(&lc) == 0);
  assert(fifa96_match_lifecycle_end(&lc, &pace) == -FIFA96_ERR_IO);
  assert(t.cancel_calls == 1);
  assert(t.teardown_calls == 1);
  assert(t.post_calls == 1);
  assert(lc.target == 2);
  assert(lc.registered == 0);
}

static void test_null_arguments(void) {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct track t;
  struct fifa96_match_lifecycle_backend b;
  memset(&t, 0, sizeof(t));
  t.lc = &lc;
  t.pace = &pace;
  b = make_backend(&t);
  fifa96_match_lifecycle_init(NULL);
  fifa96_match_pace_init(&pace);
  fifa96_match_lifecycle_init(&lc);
  assert(fifa96_match_lifecycle_begin(NULL, &b, &pace, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_lifecycle_begin(&lc, NULL, &pace, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_lifecycle_begin(&lc, &b, NULL, 0) == -FIFA96_ERR_INVALID);
  b.register_callback = NULL;
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 0) == -FIFA96_ERR_INVALID);
  b = make_backend(&t);
  b.cancel_callback = NULL;
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 0) == -FIFA96_ERR_INVALID);
  b = make_backend(&t);
  b.teardown = NULL;
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 0) == -FIFA96_ERR_INVALID);
  b = make_backend(&t);
  b.post_exit = NULL;
  assert(fifa96_match_lifecycle_begin(&lc, &b, &pace, 0) == 0);
  assert(fifa96_match_lifecycle_mark_over(NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_lifecycle_resolve_over(NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_lifecycle_request_exit(NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_lifecycle_should_exit(NULL) == 0);
  assert(fifa96_match_lifecycle_end(NULL, &pace) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_lifecycle_end(&lc, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_lifecycle_end(&lc, &pace) == 0);
  assert(t.post_calls == 0);
}

int main(void) {
  test_init_zeroes();
  test_begin_order_and_state();
  test_begin_selector_nonzero();
  test_begin_twice_is_state_error();
  test_begin_register_failure_rolls_back();
  test_mark_over_and_resolve();
  test_request_exit_and_should_exit();
  test_end_order_and_post_exit();
  test_end_without_exit_still_tears_down();
  test_end_without_begin_is_state_error();
  test_begin_end_balance_across_reuse();
  test_end_propagates_callback_error();
  test_null_arguments();
  puts("test_match_lifecycle: ok");
  return 0;
}
