#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_pacing.h"

#define FIFA96_TICK_SLOTS 8u

typedef void (*fifa96_tick_fn)(void *user);

struct fifa96_tick_slot {
  fifa96_tick_fn fn;
  void *user;
  uint32_t period;
  uint32_t countdown;
};

struct fifa96_tick {
  struct fifa96_tick_slot slots[FIFA96_TICK_SLOTS];
};

void fifa96_tick_init(struct fifa96_tick *t);
int fifa96_tick_active(const struct fifa96_tick *t);
int fifa96_tick_register(struct fifa96_tick *t, fifa96_tick_fn fn, void *user,
                         uint32_t period);
int fifa96_tick_cancel(struct fifa96_tick *t, fifa96_tick_fn fn);
int fifa96_tick_advance(struct fifa96_tick *t, uint32_t ticks);
int fifa96_tick_isr(struct fifa96_tick *t, struct fifa96_pacing_clock *clock);
