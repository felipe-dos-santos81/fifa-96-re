#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_pacing.h"
#include "fifa96_loader/fifa96_tick.h"

/* 100 Hz PIT ISR model driver. The original programs PIT channel 0 with
 * divisor 0x2E9C (1193182/0x2E9C = 100.0 Hz); this struct converts host
 * nanoseconds into PIT ISRs and drives the 20 Hz callback table. */
struct fifa96_engine_clock {
  struct fifa96_pacing_clock pit;
  struct fifa96_tick ticks;
  uint64_t frac_ns;   /* sub-centisecond remainder */
  uint64_t tick_ns;   /* nanoseconds per PIT tick, 1193182 Hz divisor model */
};

/* Zero the counters and tick table; sets tick_ns to the 100 Hz model. */
void fifa96_clock_init(struct fifa96_engine_clock *c);

/* Accumulate delta_ns + frac_ns, fire one fifa96_tick_isr per whole tick_ns,
 * keep the remainder in frac_ns, and return the number of PIT ticks fired.
 * NULL clock or zero tick_ns -> 0. */
int fifa96_clock_advance_ns(struct fifa96_engine_clock *c, uint64_t delta_ns);
