// tests/test_event_queue.c — FU-63 presentation/event pump queue (docs/ghidra/FU63_event_pump.md).
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_event_queue.h"

struct stub {
  int32_t now;
  int32_t rand_value;
  int busy;
  int32_t input_code;
  int now_calls;
  int rand_calls;
  int input_calls;
  int action_calls;
  int command_calls;
  int post_calls;
  int dispatch_calls;
  int32_t last_action;
  int32_t last_command;
  int32_t last_post_id;
  int32_t last_post_param;
  int32_t last_post_code;
  int32_t last_id;
  int32_t last_value;
  int32_t last_param;
  int dispatch_result;
};

static int32_t stub_now(void *ctx) {
  struct stub *s = (struct stub *)ctx;
  s->now_calls++;
  return s->now;
}

static int32_t stub_rand(void *ctx) {
  struct stub *s = (struct stub *)ctx;
  s->rand_calls++;
  return s->rand_value;
}

static int stub_busy(void *ctx) {
  return ((struct stub *)ctx)->busy;
}

static int32_t stub_input(void *ctx) {
  struct stub *s = (struct stub *)ctx;
  s->input_calls++;
  return s->input_code;
}

static void stub_action(void *ctx, int32_t code) {
  struct stub *s = (struct stub *)ctx;
  s->action_calls++;
  s->last_action = code;
}

static void stub_command(void *ctx, int32_t threshold) {
  struct stub *s = (struct stub *)ctx;
  s->command_calls++;
  s->last_command = threshold;
}

static void stub_post(void *ctx, int32_t id, int32_t param, int32_t code) {
  struct stub *s = (struct stub *)ctx;
  s->post_calls++;
  s->last_post_id = id;
  s->last_post_param = param;
  s->last_post_code = code;
}

static int stub_dispatch(void *ctx, const struct fifa96_event_slot *slot) {
  struct stub *s = (struct stub *)ctx;
  s->dispatch_calls++;
  s->last_id = slot->id;
  s->last_value = slot->value;
  s->last_param = slot->param;
  return s->dispatch_result;
}

static struct fifa96_event_queue_backend make_backend(struct stub *s) {
  struct fifa96_event_queue_backend b;
  b.now = stub_now;
  b.rand = stub_rand;
  b.busy = stub_busy;
  b.input = stub_input;
  b.input_action = stub_action;
  b.command = stub_command;
  b.post = stub_post;
  b.dispatch = stub_dispatch;
  b.ctx = s;
  return b;
}

static void test_init(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  memset(&q, 0xFF, sizeof q);
  st.now = 1000;
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  assert(q.state == FIFA96_EVENT_QUEUE_IDLE);
  assert(q.count == 0);
  assert(q.index == 0);
  assert(q.next_time == 1000 + 0xB4);
  assert(q.last_run == 1000 + 0x708);
  assert(q.timer == 0);
  assert(q.timer_deadline == 0);
  assert(q.flush == 0);
  assert(q.ring_flag == 0);
  assert(q.one_shot == 0);
  assert(q.ring_pending == 0);
  assert(q.input_shot[0] == 0 && q.input_shot[1] == 0);
  assert(q.input_shot[2] == 0 && q.input_shot[3] == 0);
  assert(q.slot[0].id == 0 && q.slot[5].param == 0);
  assert(q.backend == &b);
  fifa96_event_queue_init(NULL, &b);
  fifa96_event_queue_init(&q, NULL);
  assert(q.next_time == 0 && q.last_run == 0 && q.backend == NULL);
}

static void test_enqueue_full(void) {
  struct stub st;
  struct fifa96_event_queue q, q2;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  for (int32_t i = 0; i < 6; i++)
    assert(fifa96_event_queue_enqueue(&q, 0x10 + i, i, 0x20 + i) == 0);
  assert(q.count == 6);
  assert(q.index == 0);
  assert(q.slot[3].id == 0x13 && q.slot[3].value == 3 && q.slot[3].param == 0x23);
  assert(fifa96_event_queue_enqueue(&q, 0x99, 0, 0) == -FIFA96_ERR_FULL);
  assert(q.count == 6);
  assert(fifa96_event_queue_enqueue(NULL, 1, 2, 3) == -FIFA96_ERR_INVALID);
  fifa96_event_queue_init(&q2, &b);
  assert(fifa96_event_queue_enqueue(&q2, -1, -2, -3) == 0);
  assert(q2.slot[0].id == -1 && q2.slot[0].value == -2 && q2.slot[0].param == -3);
}

static void test_reset(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  assert(fifa96_event_queue_enqueue(&q, 1, 2, 3) == 0);
  assert(fifa96_event_queue_enqueue(&q, 4, 5, 6) == 0);
  q.index = 1;
  assert(fifa96_event_queue_reset(&q) == 0);
  assert(q.count == 0);
  assert(q.index == 1);
  assert(fifa96_event_queue_reset(NULL) == -FIFA96_ERR_INVALID);
}

static void test_tick_gates(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  st.now_calls = 0;
  st.rand_calls = 0;
  q.flush = 1;
  assert(fifa96_event_queue_tick(&q, 0) == 0);
  assert(q.flush == 1);
  assert(st.now_calls == 0 && st.rand_calls == 0);
  assert(fifa96_event_queue_tick(NULL, 1) == -FIFA96_ERR_INVALID);
  fifa96_event_queue_init(&q, NULL);
  assert(fifa96_event_queue_tick(&q, 1) == -FIFA96_ERR_INVALID);
  b.dispatch = NULL;
  fifa96_event_queue_init(&q, &b);
  assert(fifa96_event_queue_tick(&q, 1) == -FIFA96_ERR_INVALID);
}

static void test_idle_busy_blocks(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  st.now_calls = 0;
  st.busy = 1;
  st.now = 5000;
  q.ring_flag = 1;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(q.state == FIFA96_EVENT_QUEUE_IDLE);
  assert(st.rand_calls == 0 && st.now_calls == 0);
  assert(st.command_calls == 0);
  assert(q.ring_flag == 0);
  assert(q.flush == 0 && q.one_shot == 0 && q.ring_pending == 0);
}

static void test_idle_schedules_and_waits(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  st.now = 1000;
  fifa96_event_queue_init(&q, &b);
  st.now = 5000;
  st.rand_value = 25;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(q.state == FIFA96_EVENT_QUEUE_WAIT);
  assert(q.next_time == 5000 + 25 + 0xB4);
  assert(st.rand_calls == 1);
  st.now = q.next_time;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(q.state == FIFA96_EVENT_QUEUE_WAIT);
  {
    int32_t expected = q.next_time + 1 + 0xB4;
    st.now = q.next_time + 1;
    assert(fifa96_event_queue_tick(&q, 1) == 0);
    assert(q.state == FIFA96_EVENT_QUEUE_ACTIVE);
    assert(q.next_time == expected);
  }
  assert(st.input_calls == 0);
}

static void test_active_input_chain(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  q.state = FIFA96_EVENT_QUEUE_ACTIVE;
  q.next_time = 100;
  st.now = 50;
  st.input_code = 1;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.input_calls == 0);
  st.now = 101;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.action_calls == 1 && st.last_action == 1);
  assert(q.input_shot[0] == 1 && q.input_shot[1] == 0);
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.action_calls == 1);
  assert(q.input_shot[1] == 1);
  st.input_code = 2;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.action_calls == 2 && st.last_action == 2);
  assert(q.input_shot[2] == 0);
  st.input_code = 5;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.action_calls == 3 && st.last_action == 5);
  assert(q.input_shot[3] == 0);
  st.input_code = 0;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.action_calls == 3);
}

static void test_dispatch_order_and_reset(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  st.now = 100;
  st.dispatch_result = 1;
  assert(fifa96_event_queue_enqueue(&q, 0x11, 1, 0x21) == 0);
  assert(fifa96_event_queue_enqueue(&q, 0x22, 2, 0x22) == 0);
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.dispatch_calls == 1);
  assert(st.last_id == 0x11 && st.last_value == 1 && st.last_param == 0x21);
  assert(q.index == 1 && q.count == 2);
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.dispatch_calls == 2);
  assert(st.last_id == 0x22);
  assert(q.index == 2 && q.count == 2);
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.dispatch_calls == 2);
  assert(q.index == 0 && q.count == 0);
}

static void test_dispatch_gated_keeps_state(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  q.state = FIFA96_EVENT_QUEUE_ACTIVE;
  q.next_time = 0;
  st.now = 100;
  st.dispatch_result = 0;
  assert(fifa96_event_queue_enqueue(&q, 0x33, 3, 4) == 0);
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.dispatch_calls == 1);
  assert(q.state == FIFA96_EVENT_QUEUE_ACTIVE);
  assert(q.index == 1);
}

static void test_timer_gate(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  st.now = 100;
  assert(fifa96_event_queue_enqueue(&q, 0x44, 4, 5) == 0);
  q.timer = 1;
  q.timer_deadline = 200;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.dispatch_calls == 0);
  assert(q.timer == 1);
  st.now = 201;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.dispatch_calls == 1);
  assert(q.timer == 0);
}

static void test_flush_arm(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  st.now = 100;
  st.dispatch_result = 1;
  assert(fifa96_event_queue_enqueue(&q, 0x55, 5, 6) == 0);
  assert(fifa96_event_queue_enqueue(&q, 0x66, 6, 7) == 0);
  q.index = 1;
  q.flush = 1;
  q.one_shot = 1;
  q.ring_flag = 1;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.command_calls == 1 && st.last_command == 0x40);
  assert(st.dispatch_calls == 1);
  assert(st.last_id == 0x43 && st.last_value == 0 && st.last_param == 0);
  assert(q.count == 1 && q.index == 1);
  assert(q.flush == 0 && q.one_shot == 0 && q.ring_flag == 0);
}

static void test_one_shot_arm(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  st.now = 100;
  assert(fifa96_event_queue_enqueue(&q, 0x77, 7, 8) == 0);
  q.one_shot = 1;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.command_calls == 1 && st.last_command == 0x20);
  assert(st.dispatch_calls == 0);
  assert(q.one_shot == 0);
  assert(q.count == 1 && q.index == 0);
}

static void test_wait_flags_and_command(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  q.state = FIFA96_EVENT_QUEUE_WAIT;
  q.next_time = 0;
  q.ring_flag = 1;
  st.now = 100;
  assert(fifa96_event_queue_enqueue(&q, 0x88, 8, 9) == 0);
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.command_calls == 1 && st.last_command == 8);
  assert(st.dispatch_calls == 1);
  assert(q.ring_flag == 0);
}

static void test_active_ring_command(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  q.state = FIFA96_EVENT_QUEUE_ACTIVE;
  q.next_time = 0;
  q.ring_pending = 1;
  st.now = 100;
  st.input_code = 0;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(st.command_calls == 1 && st.last_command == 0);
  assert(q.ring_pending == 0);
}

static void test_schedule_fallthrough(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  st.now = 5000;
  fifa96_event_queue_init(&q, &b);
  st.now = 5000;
  st.rand_value = 0;
  q.ring_flag = 1;
  assert(fifa96_event_queue_tick(&q, 1) == 0);
  assert(q.state == FIFA96_EVENT_QUEUE_WAIT);
  assert(q.next_time == 5000 + 0xB4);
  assert(st.command_calls == 1 && st.last_command == 8);
  assert(q.ring_flag == 0);
}

static void test_signal_mapping(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  fifa96_event_queue_init(&q, &b);
  assert(fifa96_event_queue_signal(&q, 5, 6, 8) == 0);
  assert(q.ring_flag == 1 && q.one_shot == 0 && q.flush == 0 && q.ring_pending == 1);
  assert(st.post_calls == 1 && st.last_post_id == 5 && st.last_post_param == 6);
  assert(st.last_post_code == 8);
  assert(fifa96_event_queue_signal(&q, 7, 8, 0x21) == 0);
  assert(q.one_shot == 1);
  assert(fifa96_event_queue_signal(&q, 9, 10, 0x20) == 0);
  assert(q.one_shot == 1);
  assert(fifa96_event_queue_signal(&q, 11, 12, 0x40) == 0);
  assert(q.flush == 1);
  assert(fifa96_event_queue_signal(&q, 13, 14, 0x10) == 0);
  assert(st.post_calls == 5);
  assert(fifa96_event_queue_signal(NULL, 1, 2, 3) == -FIFA96_ERR_INVALID);
  fifa96_event_queue_init(&q, NULL);
  assert(fifa96_event_queue_signal(&q, 1, 2, 3) == -FIFA96_ERR_INVALID);
}

static void test_schedule(void) {
  struct stub st;
  struct fifa96_event_queue q;
  struct fifa96_event_queue_backend b;
  memset(&st, 0, sizeof st);
  b = make_backend(&st);
  st.now = 1000;
  fifa96_event_queue_init(&q, &b);
  st.now = 5000;
  st.input_code = 3;
  assert(fifa96_event_queue_schedule(&q, 250, 2, -100) == 0);
  assert(st.post_calls == 0);
  st.now = 6000;
  assert(fifa96_event_queue_schedule(&q, 250, 2, -100) == 0);
  assert(st.post_calls == 1);
  assert(st.last_post_id == 0x8D && st.last_post_param == 0 && st.last_post_code == 4);
  assert(q.ring_pending == 1);
  assert(q.last_run == 6000);
  st.post_calls = 0;
  st.input_calls = 0;
  st.now = 7000;
  assert(fifa96_event_queue_schedule(&q, 250, 1, 0) == 0);
  assert(st.post_calls == 0 && st.input_calls == 0);
  assert(fifa96_event_queue_schedule(&q, 250, 2, 0x2D0) == 0);
  assert(st.post_calls == 0 && st.input_calls == 0);
  st.input_code = 0;
  st.now = 9000;
  assert(fifa96_event_queue_schedule(&q, 250, 2, 0) == 0);
  assert(st.post_calls == 0);
  assert(q.last_run == 9000);
  st.input_code = 6;
  st.now = 12000;
  assert(fifa96_event_queue_schedule(&q, 250, 2, 0x100) == 0);
  assert(st.post_calls == 0);
  assert(q.last_run == 12000);
  assert(fifa96_event_queue_schedule(NULL, 0, 0, 0) == -FIFA96_ERR_INVALID);
  fifa96_event_queue_init(&q, NULL);
  assert(fifa96_event_queue_schedule(&q, 0, 0, 0) == -FIFA96_ERR_INVALID);
  {
    struct fifa96_event_queue q2;
    struct fifa96_event_queue_backend b2 = make_backend(&st);
    b2.post = NULL;
    fifa96_event_queue_init(&q2, &b2);
    st.now = 20000;
    st.input_code = 3;
    assert(fifa96_event_queue_schedule(&q2, 250, 2, 0) == -FIFA96_ERR_INVALID);
  }
}

int main(void) {
  test_init();
  test_enqueue_full();
  test_reset();
  test_tick_gates();
  test_idle_busy_blocks();
  test_idle_schedules_and_waits();
  test_active_input_chain();
  test_dispatch_order_and_reset();
  test_dispatch_gated_keeps_state();
  test_timer_gate();
  test_flush_arm();
  test_one_shot_arm();
  test_wait_flags_and_command();
  test_active_ring_command();
  test_schedule_fallthrough();
  test_signal_mapping();
  test_schedule();
  puts("test_event_queue: ok");
  return 0;
}
