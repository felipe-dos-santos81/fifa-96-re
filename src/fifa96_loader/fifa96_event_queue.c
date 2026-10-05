#include <stddef.h>
#include "fifa96_loader/fifa96_event_queue.h"

static void queue_command(struct fifa96_event_queue *q, int32_t threshold) {
  q->backend->command(q->backend->ctx, threshold);
  q->ring_pending = 0;
}

static void queue_drain(struct fifa96_event_queue *q) {
  if (q->count > q->index) {
    const struct fifa96_event_slot *slot = &q->slot[q->index];
    q->index++;
    if (q->backend->dispatch(q->backend->ctx, slot))
      q->state = FIFA96_EVENT_QUEUE_IDLE;
  } else {
    q->count = 0;
    q->index = 0;
  }
}

void fifa96_event_queue_init(struct fifa96_event_queue *q,
                             const struct fifa96_event_queue_backend *backend) {
  uint32_t i;
  if (!q) return;
  q->backend = backend;
  q->state = FIFA96_EVENT_QUEUE_IDLE;
  q->count = 0;
  q->index = 0;
  q->next_time = 0;
  q->last_run = 0;
  q->timer_deadline = 0;
  q->timer = 0;
  q->flush = 0;
  q->ring_flag = 0;
  q->one_shot = 0;
  q->ring_pending = 0;
  for (i = 0; i < 4u; i++) q->input_shot[i] = 0;
  for (i = 0; i < FIFA96_EVENT_QUEUE_SLOTS; i++) {
    q->slot[i].id = 0;
    q->slot[i].value = 0;
    q->slot[i].param = 0;
  }
  if (backend && backend->now) {
    int32_t now = backend->now(backend->ctx);
    q->next_time = now + (int32_t)FIFA96_EVENT_QUEUE_WAIT_MIN;
    q->last_run = now + (int32_t)FIFA96_EVENT_QUEUE_LAST_RUN;
  }
}

int fifa96_event_queue_enqueue(struct fifa96_event_queue *q, int32_t id,
                               int32_t value, int32_t param) {
  if (!q) return -FIFA96_ERR_INVALID;
  if (q->count >= FIFA96_EVENT_QUEUE_SLOTS) return -FIFA96_ERR_FULL;
  q->slot[q->count].id = id;
  q->slot[q->count].value = value;
  q->slot[q->count].param = param;
  q->count++;
  return 0;
}

int fifa96_event_queue_reset(struct fifa96_event_queue *q) {
  if (!q) return -FIFA96_ERR_INVALID;
  q->count = 0;
  return 0;
}

int fifa96_event_queue_signal(struct fifa96_event_queue *q, int32_t id,
                              int32_t param, int32_t code) {
  if (!q || !q->backend || !q->backend->post) return -FIFA96_ERR_INVALID;
  if (code == 8) q->ring_flag = 1;
  else if (code == 0x20 || code == 0x21) q->one_shot = 1;
  else if (code == 0x40) q->flush = 1;
  q->ring_pending = 1;
  q->backend->post(q->backend->ctx, id, param, code);
  return 0;
}

int fifa96_event_queue_schedule(struct fifa96_event_queue *q,
                                int32_t period_seconds, int32_t phase,
                                int32_t distance) {
  int32_t deadline;
  int64_t d;
  int32_t code;
  if (!q || !q->backend) return -FIFA96_ERR_INVALID;
  if (!q->backend->now || !q->backend->input) return -FIFA96_ERR_INVALID;
  deadline = q->last_run + (period_seconds / 5 + 0x2D) * 0x1C;
  if (deadline >= q->backend->now(q->backend->ctx)) return 0;
  if (phase != 2) return 0;
  d = distance;
  if (d < 0) d = -d;
  if (d >= 0x2D0) return 0;
  code = q->backend->input(q->backend->ctx);
  if (code > 0 && code <= 5) {
    int rc = fifa96_event_queue_signal(q, 0x8D, 0, 4);
    if (rc != 0) return rc;
  }
  q->last_run = q->backend->now(q->backend->ctx);
  return 0;
}

int fifa96_event_queue_tick(struct fifa96_event_queue *q, int enabled) {
  const struct fifa96_event_queue_backend *b;
  enum fifa96_event_queue_state st;
  if (!q) return -FIFA96_ERR_INVALID;
  if (!enabled) return 0;
  b = q->backend;
  if (!b || !b->now || !b->rand || !b->busy || !b->input || !b->input_action ||
      !b->command || !b->dispatch)
    return -FIFA96_ERR_INVALID;
  st = q->state;
  if (st == FIFA96_EVENT_QUEUE_IDLE) {
    if (q->flush) {
      q->count = 0;
      q->index = 0;
      fifa96_event_queue_enqueue(q, (int32_t)FIFA96_EVENT_QUEUE_FLUSH_ID, 0, 0);
      queue_command(q, 0x40);
      queue_drain(q);
      goto tail;
    }
    if (q->one_shot) {
      queue_command(q, 0x20);
      goto tail;
    }
    if (b->busy(b->ctx)) goto tail;
    if (q->timer) {
      if (!(b->now(b->ctx) > q->timer_deadline)) goto tail;
    }
    q->timer = 0;
    if (q->count != 0) {
      queue_drain(q);
      goto tail;
    }
    {
      int32_t r = b->rand(b->ctx);
      int32_t n = b->now(b->ctx);
      q->next_time = n + (r % (int32_t)FIFA96_EVENT_QUEUE_WAIT_RAND) +
                     (int32_t)FIFA96_EVENT_QUEUE_WAIT_MIN;
    }
    q->state = FIFA96_EVENT_QUEUE_WAIT;
    st = FIFA96_EVENT_QUEUE_WAIT;
  }
  if (st == FIFA96_EVENT_QUEUE_WAIT) {
    if (q->ring_flag || q->flush || q->one_shot) {
      queue_command(q, 8);
      queue_drain(q);
      goto tail;
    }
    if (b->now(b->ctx) > q->next_time) {
      int32_t n = b->now(b->ctx);
      q->next_time = n + (int32_t)FIFA96_EVENT_QUEUE_WAIT_MIN;
      q->state = FIFA96_EVENT_QUEUE_ACTIVE;
      st = FIFA96_EVENT_QUEUE_ACTIVE;
    } else {
      goto tail;
    }
  }
  if (st == FIFA96_EVENT_QUEUE_ACTIVE) {
    if (b->now(b->ctx) > q->next_time) {
      int32_t code = b->input(b->ctx);
      if (!q->input_shot[0] && code == 1) {
        b->input_action(b->ctx, 1);
        q->input_shot[0] = 1;
      } else if (!q->input_shot[1] && code == 1) {
        q->input_shot[1] = 1;
      } else if (!q->input_shot[2] && code == 2) {
        b->input_action(b->ctx, 2);
      } else if (!q->input_shot[3] && code == 5) {
        b->input_action(b->ctx, 5);
      }
    }
    if (q->ring_pending) queue_command(q, 0);
    queue_drain(q);
  }
tail:
  q->flush = 0;
  q->ring_flag = 0;
  q->one_shot = 0;
  q->ring_pending = 0;
  return 0;
}
