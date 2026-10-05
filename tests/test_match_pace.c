// tests/test_match_pace.c — FU-60 30 Hz match frame pace (docs/ghidra/FU60_match_frame_body.md).
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_match_pace.h"

static void test_init_zeroes(void) {
  struct fifa96_match_pace p;
  p.acc = 1;
  p.pending = 1;
  p.hold = 1;
  fifa96_match_pace_init(&p);
  assert(p.acc == 0);
  assert(p.pending == 0);
  assert(p.hold == 0);
  assert(fifa96_match_pace_pending(&p) == 0);
}

static void test_three_ticks_then_grant(void) {
  struct fifa96_match_pace p;
  int granted;
  fifa96_match_pace_init(&p);
  for (int i = 0; i < 3; i++) {
    granted = -1;
    assert(fifa96_match_pace_tick(&p, 0, &granted) == 0);
    assert(granted == 0);
  }
  assert(p.acc == 0x102u * 3u);
  granted = -1;
  assert(fifa96_match_pace_tick(&p, 0, &granted) == 0);
  assert(granted == 1);
  assert(p.acc == 0x102u * 4u - 0x35Cu);
  assert(fifa96_match_pace_pending(&p) == 1);
}

static void test_exactly_thirty_hz(void) {
  struct fifa96_match_pace p;
  int frames = 0;
  fifa96_match_pace_init(&p);
  for (int i = 0; i < 100; i++) {
    int granted = 0;
    assert(fifa96_match_pace_tick(&p, 0, &granted) == 0);
    frames += granted;
  }
  assert(frames == 30);
  assert(p.acc == 0);
  assert(fifa96_match_pace_pending(&p) == 30);
}

static void test_hold_blocks_accumulator_and_resume_restores(void) {
  struct fifa96_match_pace p;
  fifa96_match_pace_init(&p);
  assert(fifa96_match_pace_hold(&p) == 0);
  assert(p.hold == 1);
  assert(fifa96_match_pace_hold(&p) == 0);
  assert(p.hold == 1);
  for (int i = 0; i < 100; i++) {
    int granted = 1;
    assert(fifa96_match_pace_tick(&p, 0, &granted) == 0);
    assert(granted == 0);
  }
  assert(p.acc == 0);
  assert(fifa96_match_pace_pending(&p) == 0);
  assert(fifa96_match_pace_resume(&p) == 0);
  assert(p.hold == 0);
  for (int i = 0; i < 4; i++) {
    int granted = 0;
    assert(fifa96_match_pace_tick(&p, 0, &granted) == 0);
    if (i < 3) assert(granted == 0);
    else assert(granted == 1);
  }
  assert(fifa96_match_pace_pending(&p) == 1);
}

static void test_blocked_preserves_accumulator(void) {
  struct fifa96_match_pace p;
  fifa96_match_pace_init(&p);
  for (int i = 0; i < 3; i++) {
    int granted = 1;
    assert(fifa96_match_pace_tick(&p, 0, &granted) == 0);
    assert(granted == 0);
  }
  assert(p.acc == 0x102u * 3u);
  for (int i = 0; i < 5; i++) {
    int granted = 1;
    assert(fifa96_match_pace_tick(&p, 1, &granted) == 0);
    assert(granted == 0);
  }
  assert(p.acc == 0x102u * 3u);
  int granted = 0;
  assert(fifa96_match_pace_tick(&p, 0, &granted) == 0);
  assert(granted == 1);
  assert(p.acc == 0x102u * 4u - 0x35Cu);
}

static void test_pending_accumulates_until_reset(void) {
  struct fifa96_match_pace p;
  fifa96_match_pace_init(&p);
  for (int i = 0; i < 1000; i++) {
    int granted = 0;
    assert(fifa96_match_pace_tick(&p, 0, &granted) == 0);
  }
  assert(fifa96_match_pace_pending(&p) == 300);
  fifa96_match_pace_init(&p);
  assert(fifa96_match_pace_pending(&p) == 0);
  assert(p.acc == 0);
}

static void test_null_arguments(void) {
  struct fifa96_match_pace p;
  int granted = 0;
  fifa96_match_pace_init(NULL);
  assert(fifa96_match_pace_hold(NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_pace_resume(NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_pace_tick(NULL, 0, &granted) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_pace_pending(NULL) == 0);
  fifa96_match_pace_init(&p);
  assert(fifa96_match_pace_tick(&p, 0, NULL) == -FIFA96_ERR_INVALID);
  assert(p.acc == 0);
}

int main(void) {
  test_init_zeroes();
  test_three_ticks_then_grant();
  test_exactly_thirty_hz();
  test_hold_blocks_accumulator_and_resume_restores();
  test_blocked_preserves_accumulator();
  test_pending_accumulates_until_reset();
  test_null_arguments();
  puts("test_match_pace: ok");
  return 0;
}
