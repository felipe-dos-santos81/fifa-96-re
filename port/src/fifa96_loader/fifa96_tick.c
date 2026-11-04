#include <stddef.h>
#include "fifa96_loader/fifa96_tick.h"

void fifa96_tick_init(struct fifa96_tick *t) {
  if (!t) return;
  for (unsigned i = 0; i < FIFA96_TICK_SLOTS; i++) {
    t->slots[i].fn = NULL;
    t->slots[i].user = NULL;
    t->slots[i].period = 0;
    t->slots[i].countdown = 0;
  }
}

int fifa96_tick_active(const struct fifa96_tick *t) {
  if (!t) return -FIFA96_ERR_INVALID;
  int n = 0;
  for (unsigned i = 0; i < FIFA96_TICK_SLOTS; i++)
    if (t->slots[i].fn) n++;
  return n;
}

int fifa96_tick_register(struct fifa96_tick *t, fifa96_tick_fn fn, void *user,
                         uint32_t period) {
  if (!t || !fn) return -FIFA96_ERR_INVALID;
  for (unsigned i = 0; i < FIFA96_TICK_SLOTS; i++) {
    if (!t->slots[i].fn) {
      t->slots[i].fn = fn;
      t->slots[i].user = user;
      t->slots[i].period = period;
      t->slots[i].countdown = 1;
      return (int)i;
    }
  }
  return -FIFA96_ERR_FULL;
}

int fifa96_tick_cancel(struct fifa96_tick *t, fifa96_tick_fn fn) {
  if (!t || !fn) return -FIFA96_ERR_INVALID;
  for (unsigned i = 0; i < FIFA96_TICK_SLOTS; i++) {
    if (t->slots[i].fn == fn) {
      t->slots[i].fn = NULL;
      t->slots[i].user = NULL;
      t->slots[i].period = 0;
      t->slots[i].countdown = 0;
      return 0;
    }
  }
  return -FIFA96_ERR_NOT_FOUND;
}

int fifa96_tick_advance(struct fifa96_tick *t, uint32_t ticks) {
  if (!t) return -FIFA96_ERR_INVALID;
  while (ticks--) {
    for (unsigned i = 0; i < FIFA96_TICK_SLOTS; i++) {
      fifa96_tick_fn fn = t->slots[i].fn;
      if (!fn) continue;
      if (t->slots[i].countdown > 0) t->slots[i].countdown--;
      if (t->slots[i].countdown != 0) continue;
      fn(t->slots[i].user);
      if (t->slots[i].fn != fn) continue;
      if (t->slots[i].period == 0) {
        t->slots[i].fn = NULL;
        t->slots[i].user = NULL;
        t->slots[i].countdown = 0;
      } else {
        t->slots[i].countdown = t->slots[i].period;
      }
    }
  }
  return 0;
}

int fifa96_tick_isr(struct fifa96_tick *t, struct fifa96_pacing_clock *clock) {
  if (!t || !clock) return -FIFA96_ERR_INVALID;
  int fired20 = fifa96_pacing_clock_isr(clock);
  int rc = fifa96_tick_advance(t, 1);
  return rc < 0 ? rc : fired20;
}
