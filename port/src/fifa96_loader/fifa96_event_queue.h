#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_EVENT_QUEUE_SLOTS 6u
#define FIFA96_EVENT_QUEUE_WAIT_MIN 0xB4u
#define FIFA96_EVENT_QUEUE_WAIT_RAND 0x3Cu
#define FIFA96_EVENT_QUEUE_LAST_RUN 0x708u
#define FIFA96_EVENT_QUEUE_TIMER_TICKS 0x1Eu
#define FIFA96_EVENT_QUEUE_FLUSH_ID 0x43u

/* FU-142 OL-27 / M2 arms-and-wiring Task 12: the native event append sinks.
 * `FUN_000928F0` (`0x928F0..0x92993`) appends to the 25-entry ring at flat
 * 0x15B440 (stride 0x15 = code byte, 4-byte stamp from `FUN_000CB2A4`
 * = `[0x112E88]`, actor, 12-byte vector from 0x157758) unless the sink byte
 * (`0x15B650`) is exactly 0x26, and clears the sink byte; the ring index byte
 * 0x15B665 advances `(index + 1) % 25`. `FUN_00092820` (`0x92820..0x92860`)
 * stores the code byte to the sink and fills the other three fields only when
 * the 0x110F1C table byte for the (sign-extended) code has bit 0 set. The
 * table is embedded from the image (`0x110F1C..0x110F43`, the deliberate
 * 0x28-byte code table: 0 and 0x26/0x27 are the no-fill entries); a code
 * >= 0x28 is the derived boundary (the native reads the adjacent data; leg
 * OL-67). */
#define FIFA96_EVENT_RING_SLOTS 25u

struct fifa96_event_ring_entry {
  uint8_t code;      /* native ring/sink +0 */
  int32_t stamp;     /* native +1: the [0x112E88] tick via FUN_000CB2A4 */
  int32_t actor;     /* native +5: the record/actor identity (EBP) */
  int32_t vector[3]; /* native +9: the 12-byte 0x157758 triple */
};

struct fifa96_event_ring {
  struct fifa96_event_ring_entry entry[FIFA96_EVENT_RING_SLOTS];
  uint8_t index;                       /* native 0x15B665 */
  struct fifa96_event_ring_entry sink; /* native 0x15B650 */
};

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
  struct fifa96_event_ring ring;   /* FU-142 OL-27 native 0x15B440/0x15B650 */
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

/* `FUN_000928F0`: append `{code, stamp, actor, vector}` to the 25-entry ring.
 * When the sink code byte is 0x26 the append is suppressed (no index advance)
 * but the sink byte is still cleared, exactly as the native. Returns 1 when an
 * entry was appended, 0 on the suppression path, -FIFA96_ERR_INVALID on NULL
 * `q`/`vector`. */
int fifa96_event_ring_append(struct fifa96_event_queue *q, int32_t actor, uint8_t code,
                             int32_t stamp, const int32_t vector[3]);

/* `FUN_00092820`: store `code` to the sink and fill the stamp/actor/vector
 * fields when the 0x110F1C eligibility bit is set (leaving the previous sink
 * extras in place otherwise). Returns 1, or -FIFA96_ERR_INVALID on NULL
 * `q`/`vector`. */
int fifa96_event_sink_store(struct fifa96_event_queue *q, int32_t actor, uint8_t code,
                            int32_t stamp, const int32_t vector[3]);
