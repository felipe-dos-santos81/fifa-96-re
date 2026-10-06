#include <stddef.h>
#include "fifa96_engine/fifa96_clock.h"

/* 0x9F799..0x9F7B0 programs PIT channel 0 with divisor 0x2E9C, so
 * 1193182/0x2E9C = 100.0 Hz -> 10 ms per ISR. */
#define FIFA96_CLOCK_TICK_NS 10000000ull

void fifa96_clock_init(struct fifa96_engine_clock *c) {
  if (!c) return;
  fifa96_pacing_clock_init(&c->pit);
  fifa96_tick_init(&c->ticks);
  c->frac_ns = 0;
  c->tick_ns = FIFA96_CLOCK_TICK_NS;
}

int fifa96_clock_advance_ns(struct fifa96_engine_clock *c, uint64_t delta_ns) {
  if (!c || c->tick_ns == 0) return 0;
  uint64_t pending = c->frac_ns + delta_ns;
  int fired = 0;
  while (pending >= c->tick_ns) {
    fifa96_tick_isr(&c->ticks, &c->pit);
    pending -= c->tick_ns;
    fired++;
  }
  c->frac_ns = pending;
  return fired;
}
