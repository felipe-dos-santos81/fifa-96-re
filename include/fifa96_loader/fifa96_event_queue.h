#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_EVENT_QUEUE_SLOTS 6u
#define FIFA96_EVENT_QUEUE_WAIT_MIN 0xB4u
#define FIFA96_EVENT_QUEUE_WAIT_RAND 0x3Cu
#define FIFA96_EVENT_QUEUE_LAST_RUN 0x708u
#define FIFA96_EVENT_QUEUE_TIMER_TICKS 0x1Eu
#define FIFA96_EVENT_QUEUE_FLUSH_ID 0x43u

enum fifa96_event_queue_state {
  FIFA96_EVENT_QUEUE_IDLE = 0,
  FIFA96_EVENT_QUEUE_WAIT = 1,
  FIFA96_EVENT_QUEUE_ACTIVE = 2
};

struct fifa96_event_slot {
  int32_t id;
  int32_t value;
  int32_t param;
};

struct fifa96_event_queue_backend {
  int32_t (*now)(void *ctx);
  int32_t (*rand)(void *ctx);
  int (*busy)(void *ctx);
  int32_t (*input)(void *ctx);
  void (*input_action)(void *ctx, int32_t code);
  void (*command)(void *ctx, int32_t threshold);
  void (*post)(void *ctx, int32_t id, int32_t param, int32_t code);
  int (*dispatch)(void *ctx, const struct fifa96_event_slot *slot);
  void *ctx;
};

struct fifa96_event_queue {
  enum fifa96_event_queue_state state;
  uint32_t count;
  uint32_t index;
  int32_t next_time;
  int32_t last_run;
  int32_t timer_deadline;
  int timer;
  int flush;
  int ring_flag;
  int one_shot;
  int ring_pending;
  uint8_t input_shot[4];
  struct fifa96_event_slot slot[FIFA96_EVENT_QUEUE_SLOTS];
  const struct fifa96_event_queue_backend *backend;
};

void fifa96_event_queue_init(struct fifa96_event_queue *q,
                             const struct fifa96_event_queue_backend *backend);
int fifa96_event_queue_enqueue(struct fifa96_event_queue *q, int32_t id,
                               int32_t value, int32_t param);
int fifa96_event_queue_reset(struct fifa96_event_queue *q);
int fifa96_event_queue_signal(struct fifa96_event_queue *q, int32_t id,
                              int32_t param, int32_t code);
int fifa96_event_queue_schedule(struct fifa96_event_queue *q,
                                int32_t period_seconds, int32_t phase,
                                int32_t distance);
int fifa96_event_queue_tick(struct fifa96_event_queue *q, int enabled);
