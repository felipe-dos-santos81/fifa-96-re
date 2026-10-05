// tests/test_tick.c — FU-59 INT-8 tick callback registry (docs/ghidra/FU59_tick_callbacks.md).
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_tick.h"

static int g_order[64];
static int g_n;
static void *g_users[64];
static uint32_t g_count;

static void reset(void) {
  memset(g_order, 0, sizeof g_order);
  memset(g_users, 0, sizeof g_users);
  g_n = 0;
  g_count = 0;
}

static void cb_a(void *user) { (void)user; g_order[g_n++] = 1; }
static void cb_b(void *user) { (void)user; g_order[g_n++] = 2; }
static void cb_c(void *user) { (void)user; g_order[g_n++] = 3; }

static void cb_user(void *user) {
  g_users[g_n] = user;
  g_order[g_n++] = 9;
}

static void cb_count(void *user) {
  (void)user;
  g_count++;
}

static void test_init_and_first_empty_slot(void) {
  struct fifa96_tick t;
  fifa96_tick_init(&t);
  assert(fifa96_tick_active(&t) == 0);
  assert(fifa96_tick_register(&t, cb_a, NULL, 1) == 0);
  assert(fifa96_tick_register(&t, cb_b, NULL, 1) == 1);
  assert(fifa96_tick_active(&t) == 2);
  assert(fifa96_tick_cancel(&t, cb_a) == 0);
  assert(fifa96_tick_active(&t) == 1);
  assert(fifa96_tick_register(&t, cb_c, NULL, 1) == 0);
  assert(fifa96_tick_active(&t) == 2);
}

static void test_capacity_and_errors(void) {
  struct fifa96_tick t;
  fifa96_tick_init(&t);
  for (int i = 0; i < (int)FIFA96_TICK_SLOTS; i++)
    assert(fifa96_tick_register(&t, cb_a, NULL, 1) == i);
  assert(fifa96_tick_register(&t, cb_b, NULL, 1) == -FIFA96_ERR_FULL);
  assert(fifa96_tick_active(&t) == (int)FIFA96_TICK_SLOTS);
  assert(fifa96_tick_register(&t, NULL, NULL, 1) == -FIFA96_ERR_INVALID);
  assert(fifa96_tick_register(NULL, cb_a, NULL, 1) == -FIFA96_ERR_INVALID);
  assert(fifa96_tick_cancel(&t, cb_b) == -FIFA96_ERR_NOT_FOUND);
  assert(fifa96_tick_cancel(&t, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_tick_cancel(NULL, cb_a) == -FIFA96_ERR_INVALID);
  assert(fifa96_tick_cancel(&t, cb_a) == 0);
  assert(fifa96_tick_active(&t) == (int)FIFA96_TICK_SLOTS - 1);
}

static void test_null_queries(void) {
  assert(fifa96_tick_active(NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_tick_advance(NULL, 1) == -FIFA96_ERR_INVALID);
  struct fifa96_tick t;
  fifa96_tick_init(&t);
  assert(fifa96_tick_advance(&t, 0) == 0);
  assert(g_n == 0);
}

static void test_dispatch_order_is_slot_order(void) {
  struct fifa96_tick t;
  fifa96_tick_init(&t);
  reset();
  assert(fifa96_tick_register(&t, cb_a, NULL, 1) == 0);
  assert(fifa96_tick_register(&t, cb_b, NULL, 1) == 1);
  assert(fifa96_tick_cancel(&t, cb_a) == 0);
  assert(fifa96_tick_register(&t, cb_c, NULL, 1) == 0);
  assert(fifa96_tick_advance(&t, 1) == 0);
  assert(g_n == 2);
  assert(g_order[0] == 3);
  assert(g_order[1] == 2);
}

static void test_user_pointer(void) {
  struct fifa96_tick t;
  int marker = 7;
  fifa96_tick_init(&t);
  reset();
  assert(fifa96_tick_register(&t, cb_user, &marker, 1) == 0);
  assert(fifa96_tick_advance(&t, 1) == 0);
  assert(g_n == 1);
  assert(g_users[0] == &marker);
}

static void test_period_countdown_arithmetic(void) {
  struct fifa96_tick t;
  fifa96_tick_init(&t);
  reset();
  assert(fifa96_tick_register(&t, cb_a, NULL, 3) == 0);
  assert(fifa96_tick_advance(&t, 1) == 0);
  assert(g_n == 1);
  assert(t.slots[0].countdown == 3);
  assert(fifa96_tick_advance(&t, 2) == 0);
  assert(g_n == 1);
  assert(fifa96_tick_advance(&t, 1) == 0);
  assert(g_n == 2);
  assert(fifa96_tick_advance(&t, 6) == 0);
  assert(g_n == 4);
}

static void test_period_one_fires_every_tick(void) {
  struct fifa96_tick t;
  fifa96_tick_init(&t);
  reset();
  assert(fifa96_tick_register(&t, cb_count, NULL, 1) == 0);
  assert(fifa96_tick_advance(&t, 100) == 0);
  assert(g_count == 100);
}

static void test_large_period_boundary(void) {
  struct fifa96_tick t;
  fifa96_tick_init(&t);
  reset();
  assert(fifa96_tick_register(&t, cb_a, NULL, 0x10000u) == 0);
  assert(fifa96_tick_advance(&t, 1) == 0);
  assert(g_n == 1);
  assert(t.slots[0].countdown == 0x10000u);
  assert(fifa96_tick_advance(&t, 0xFFFFu) == 0);
  assert(g_n == 1);
  assert(t.slots[0].countdown == 1);
  assert(fifa96_tick_advance(&t, 1) == 0);
  assert(g_n == 2);
  assert(t.slots[0].countdown == 0x10000u);
}

static void test_one_shot(void) {
  struct fifa96_tick t;
  fifa96_tick_init(&t);
  reset();
  assert(fifa96_tick_register(&t, cb_count, NULL, 0) == 0);
  assert(fifa96_tick_advance(&t, 3) == 0);
  assert(g_count == 1);
  assert(fifa96_tick_active(&t) == 0);
  assert(t.slots[0].fn == NULL);
  assert(fifa96_tick_advance(&t, 1) == 0);
  assert(g_count == 1);
}

static struct fifa96_tick *g_t;

static void cb_self_cancel(void *user) {
  (void)user;
  g_count++;
  fifa96_tick_cancel(g_t, cb_self_cancel);
}

static void cb_cancel_other(void *user) {
  (void)user;
  g_count++;
  fifa96_tick_cancel(g_t, cb_count);
}

static void cb_late_register(void *user) {
  (void)user;
  g_count++;
  fifa96_tick_register(g_t, cb_count, NULL, 1);
}

static void test_self_cancel_during_dispatch(void) {
  struct fifa96_tick t;
  fifa96_tick_init(&t);
  reset();
  g_t = &t;
  assert(fifa96_tick_register(&t, cb_self_cancel, NULL, 1) == 0);
  assert(fifa96_tick_advance(&t, 1) == 0);
  assert(g_count == 1);
  assert(fifa96_tick_active(&t) == 0);
  assert(fifa96_tick_advance(&t, 1) == 0);
  assert(g_count == 1);
}

static void test_cancel_later_slot_suppresses_same_tick(void) {
  struct fifa96_tick t;
  fifa96_tick_init(&t);
  reset();
  g_t = &t;
  assert(fifa96_tick_register(&t, cb_cancel_other, NULL, 1) == 0);
  assert(fifa96_tick_register(&t, cb_count, NULL, 1) == 1);
  assert(fifa96_tick_advance(&t, 1) == 0);
  assert(g_count == 1);
  assert(fifa96_tick_active(&t) == 1);
}

static void test_register_later_slot_runs_same_tick(void) {
  struct fifa96_tick t;
  fifa96_tick_init(&t);
  reset();
  g_t = &t;
  assert(fifa96_tick_register(&t, cb_late_register, NULL, 1) == 0);
  assert(fifa96_tick_advance(&t, 1) == 0);
  assert(g_count == 2);
  assert(fifa96_tick_active(&t) == 2);
}

static void test_isr_integration_with_pacing(void) {
  struct fifa96_tick t;
  struct fifa96_pacing_clock c;
  fifa96_tick_init(&t);
  fifa96_pacing_clock_init(&c);
  reset();
  assert(fifa96_tick_register(&t, cb_count, NULL, 1) == 0);
  for (int i = 0; i < 4; i++) assert(fifa96_tick_isr(&t, &c) == 0);
  assert(fifa96_tick_isr(&t, &c) == 1);
  assert(c.ticks == 5);
  assert(c.ticks20 == 1);
  assert(g_count == 5);
  assert(fifa96_tick_isr(&t, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_tick_isr(NULL, &c) == -FIFA96_ERR_INVALID);
  assert(c.ticks == 5);
}

int main(void) {
  test_init_and_first_empty_slot();
  test_capacity_and_errors();
  test_null_queries();
  test_dispatch_order_is_slot_order();
  test_user_pointer();
  test_period_countdown_arithmetic();
  test_period_one_fires_every_tick();
  test_large_period_boundary();
  test_one_shot();
  test_self_cancel_during_dispatch();
  test_cancel_later_slot_suppresses_same_tick();
  test_register_later_slot_runs_same_tick();
  test_isr_integration_with_pacing();
  puts("test_tick: ok");
  return 0;
}
