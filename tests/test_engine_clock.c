/* tests/test_engine_clock.c */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_engine/fifa96_clock.h"

static int fired;
static void on_tick(void *user) { (void)user; fired++; }

static void test_second_elapses(void) {
  struct fifa96_engine_clock c;
  fifa96_clock_init(&c);
  assert(c.tick_ns == 10000000ull);
  assert(c.pit.ticks == 0u && c.pit.ticks20 == 0u && c.frac_ns == 0u);
  /* period 5 = every 5th 100 Hz ISR = 20 Hz; register returns the slot index 0. */
  assert(fifa96_tick_register(&c.ticks, on_tick, NULL, 5u) == 0);
  /* Advance exactly one second in 10 ms steps: 100 PIT ticks, 20 callbacks. */
  for (int i = 0; i < 100; i++) {
    assert(fifa96_clock_advance_ns(&c, 10000000ull) == 1);
  }
  assert(c.pit.ticks == 100u);
  assert(c.pit.ticks20 == 20u);
  assert(fired == 20);
}

static void test_remainder_and_defensive(void) {
  struct fifa96_engine_clock c;
  fifa96_clock_init(&c);

  /* A sub-tick delta is kept as the remainder and fires nothing. */
  assert(fifa96_clock_advance_ns(&c, 9999999ull) == 0);
  assert(c.pit.ticks == 0u && c.frac_ns == 9999999ull);

  /* remainder + 1 ns == one whole 10 ms tick. */
  assert(fifa96_clock_advance_ns(&c, 1ull) == 1);
  assert(c.pit.ticks == 1u && c.frac_ns == 0ull);

  /* 25 ms fires two ticks and keeps 5 ms for the next call. */
  assert(fifa96_clock_advance_ns(&c, 25000000ull) == 2);
  assert(c.pit.ticks == 3u && c.frac_ns == 5000000ull);

  /* Defensive: NULL clock and uninitialized tick_ns fire nothing. */
  assert(fifa96_clock_advance_ns(NULL, 10000000ull) == 0);
  struct fifa96_engine_clock zeroed = {0};
  assert(fifa96_clock_advance_ns(&zeroed, 10000000ull) == 0);
  fifa96_clock_init(NULL);
}

int main(void) {
  test_second_elapses();
  test_remainder_and_defensive();
  puts("test_engine_clock OK");
  return 0;
}
