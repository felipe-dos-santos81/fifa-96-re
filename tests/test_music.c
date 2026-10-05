// tests/test_music.c — FU-52 music sequencer. Expected behavior is derived in
// docs/ghidra/FU52_music_sequencer.md: loader 0xA73B2, reset 0xA7499,
// start_event 0xA72E7, tempo_calc 0xA7040, tick 0xA7136.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_music.h"

struct rec {
  int rand_calls;
  int play_calls;
  int arm_calls;
  int32_t last_id;
  uint32_t last_track;
  int32_t rand_value;
  int32_t play_result;
  int32_t arm_result;
};

static struct rec R;
static struct fifa96_music_event EVENTS[FIFA96_MUSIC_EVENT_MAX + 1];
static struct fifa96_music_tempo TEMPO[FIFA96_MUSIC_TEMPO_MAX];

static void rec_reset(void) {
  memset(&R, 0, sizeof R);
  R.play_result = 1;
  R.arm_result = 7;
}

static int32_t rec_rand(void *ctx) {
  (void)ctx;
  R.rand_calls++;
  return R.rand_value;
}

static int32_t rec_play(void *ctx, int32_t id) {
  (void)ctx;
  R.play_calls++;
  R.last_id = id;
  return R.play_result;
}

static int32_t rec_arm(void *ctx, uint32_t track) {
  (void)ctx;
  R.arm_calls++;
  R.last_track = track;
  return R.arm_result;
}

static const struct fifa96_music_backend BACKEND = {rec_rand, rec_play, rec_arm,
                                                    NULL};

static struct fifa96_music_config base_config(void) {
  struct fifa96_music_config c;
  memset(&c, 0, sizeof c);
  c.track_count = 0;
  c.event_count = 1;
  c.events = EVENTS;
  c.tempo = TEMPO;
  c.limit_scale = 2;
  c.period_up = 2000;
  c.period_down = 2000;
  c.initial_event = 0;
  return c;
}

static void setup(struct fifa96_music *m, const struct fifa96_music_config *c) {
  rec_reset();
  fifa96_music_init(m, c, &BACKEND);
  m->enabled = 1;
}

static void events_clear(void) {
  memset(EVENTS, 0, sizeof EVENTS);
  memset(TEMPO, 0, sizeof TEMPO);
}

static void test_init(void) {
  struct fifa96_music m;
  struct fifa96_music_config c = base_config();
  c.track_count = 7;
  c.event_count = 99;
  memset(&m, 0xAA, sizeof m);
  fifa96_music_init(&m, &c, &BACKEND);
  assert(m.config.track_count == FIFA96_MUSIC_TRACK_MAX);
  assert(m.config.event_count == FIFA96_MUSIC_EVENT_MAX);
  assert(m.config.events == EVENTS && m.config.tempo == TEMPO);
  assert(m.config.initial_event == 0);
  assert(m.backend == &BACKEND);
  assert(m.enabled == 0 && m.paused == 0 && m.active == 0);
  assert(m.volume == FIFA96_MUSIC_VOLUME_MAX);
  assert(m.state == FIFA96_MUSIC_STATE_RANDOM);
  assert(m.pos == 0 && m.limit == 0 && m.period == 0 && m.current_event == 0);
  for (uint32_t i = 0; i < FIFA96_MUSIC_TRACK_MAX; i++) {
    assert(m.track[i].handle == FIFA96_MUSIC_NO_HANDLE);
    assert(m.track[i].last_volume == 0 && m.track[i].last_pan == 0);
  }
  fifa96_music_init(NULL, &c, &BACKEND);
  fifa96_music_init(&m, NULL, NULL);
  assert(m.config.events == NULL && m.backend == NULL);
}

static void test_reset_disabled(void) {
  struct fifa96_music m;
  struct fifa96_music_config c = base_config();
  c.track_count = 4;
  setup(&m, &c);
  m.enabled = 0;
  m.volume = 0x55;
  for (uint32_t i = 0; i < FIFA96_MUSIC_TRACK_MAX; i++) {
    m.track[i].last_volume = 9;
    m.track[i].last_pan = 9;
  }
  assert(fifa96_music_reset(&m, 0x80) == FIFA96_OK);
  assert(m.active == FIFA96_MUSIC_ACTIVE_DISABLED);
  assert(m.volume == 0x55);
  assert(R.arm_calls == 0);
  assert(m.track[0].last_volume == 9 && m.track[0].last_pan == 9);
  assert(fifa96_music_reset(NULL, 0) == -FIFA96_ERR_TRUNCATED);
}

static void test_reset_volume_clamp(void) {
  static const int32_t in[] = {-1, 0, 1, 0x7E, 0x7F, 0x80, 0x1234};
  static const int32_t out[] = {0, 0, 1, 0x7E, 0x7F, 0x7F, 0x7F};
  struct fifa96_music_config c = base_config();
  c.event_count = 0;
  c.events = NULL;
  for (size_t i = 0; i < sizeof in / sizeof in[0]; i++) {
    struct fifa96_music m;
    setup(&m, &c);
    m.paused = 1;
    assert(fifa96_music_reset(&m, in[i]) == FIFA96_OK);
    assert(m.volume == out[i]);
    assert(m.active == FIFA96_MUSIC_ACTIVE_STARTED);
  }
}

static void test_reset_tracks(void) {
  struct fifa96_music m;
  struct fifa96_music_config c = base_config();
  c.event_count = 0;
  c.events = NULL;

  c.track_count = 4;
  setup(&m, &c);
  m.paused = 1;
  for (uint32_t i = 0; i < FIFA96_MUSIC_TRACK_MAX; i++) {
    m.track[i].handle = -5;
    m.track[i].last_volume = 11;
    m.track[i].last_pan = 12;
  }
  assert(fifa96_music_reset(&m, 0x40) == FIFA96_OK);
  assert(R.arm_calls == 4 && R.last_track == 3);
  for (uint32_t i = 0; i < FIFA96_MUSIC_TRACK_MAX; i++) {
    assert(m.track[i].handle == 7);
    assert(m.track[i].last_volume == 0 && m.track[i].last_pan == 0);
  }

  c.track_count = 1;
  setup(&m, &c);
  m.paused = 1;
  m.track[0].handle = -5;
  m.track[1].handle = -5;
  m.track[1].last_volume = 5;
  assert(fifa96_music_reset(&m, 0) == FIFA96_OK);
  assert(R.arm_calls == 1 && R.last_track == 0);
  assert(m.track[0].handle == 7 && m.track[0].last_volume == 0);
  assert(m.track[1].handle == -5);
  assert(m.track[1].last_volume == 5);

  c.track_count = 0;
  setup(&m, &c);
  m.paused = 1;
  m.track[0].handle = -5;
  assert(fifa96_music_reset(&m, 0) == FIFA96_OK);
  assert(R.arm_calls == 0 && m.track[0].handle == -5);

  c.track_count = 9;
  setup(&m, &c);
  m.paused = 1;
  assert(m.config.track_count == FIFA96_MUSIC_TRACK_MAX);
  assert(fifa96_music_reset(&m, 0) == FIFA96_OK);
  assert(R.arm_calls == 4);
}

static void test_reset_schedule(void) {
  struct fifa96_music m;
  struct fifa96_music_config c = base_config();
  events_clear();
  EVENTS[0].valid = 1;
  EVENTS[0].rate = 2;
  EVENTS[0].time = 1;
  EVENTS[0].tempo = 0;
  EVENTS[0].id = 5;
  TEMPO[0].a = 0x10000;
  TEMPO[0].b = 0x10000;
  R.rand_value = 0x00010000;
  setup(&m, &c);
  m.pos = 123;
  m.limit = 456;
  m.state = FIFA96_MUSIC_STATE_TEMPO;
  assert(fifa96_music_reset(&m, 0x40) == FIFA96_OK);
  assert(R.rand_calls == 1);
  assert(R.play_calls == 1 && R.last_id == 5);
  assert(m.active == FIFA96_MUSIC_ACTIVE_STARTED);
  assert(m.volume == 0x40 && m.paused == 0);
  assert(m.pos == 0);
  assert(m.state == FIFA96_MUSIC_STATE_EVENT);
  assert(m.current_event == 0);
  assert(m.limit == 0xFFFF);
  assert(m.period == 1310);

  setup(&m, &c);
  m.paused = 1;
  m.pos = 77;
  m.limit = 88;
  m.state = FIFA96_MUSIC_STATE_TEMPO;
  assert(fifa96_music_reset(&m, 0x10) == FIFA96_OK);
  assert(R.rand_calls == 0 && R.play_calls == 0);
  assert(m.pos == 77 && m.limit == 88);
  assert(m.state == FIFA96_MUSIC_STATE_TEMPO);
  assert(m.paused == 0 && m.volume == 0x10);
  assert(m.active == FIFA96_MUSIC_ACTIVE_STARTED);
}

static void test_start_event_bounds(void) {
  struct fifa96_music m;
  struct fifa96_music_config c = base_config();
  events_clear();
  EVENTS[0].valid = 1;
  EVENTS[0].rate = 2;
  EVENTS[0].time = 0x10;
  EVENTS[0].id = 3;
  setup(&m, &c);
  m.state = FIFA96_MUSIC_STATE_TEMPO;
  m.pos = 5;
  m.paused = 1;
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(R.play_calls == 0 && m.state == FIFA96_MUSIC_STATE_TEMPO);
  m.paused = 0;

  m.state = FIFA96_MUSIC_STATE_TEMPO;
  assert(fifa96_music_start_event(&m, -1) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_music_start_event(&m, FIFA96_MUSIC_EVENT_MAX) ==
         -FIFA96_ERR_TRUNCATED);
  assert(fifa96_music_start_event(&m, 3) == FIFA96_OK);
  assert(R.play_calls == 0 && m.state == FIFA96_MUSIC_STATE_TEMPO);

  assert(fifa96_music_start_event(&m, 1) == FIFA96_OK);
  assert(R.play_calls == 0 && m.pos == 5);

  EVENTS[0].valid = 0;
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(R.play_calls == 0 && m.state == FIFA96_MUSIC_STATE_TEMPO);

  c.events = NULL;
  c.event_count = 0;
  setup(&m, &c);
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(R.play_calls == 0);
  assert(fifa96_music_start_event(NULL, 0) == -FIFA96_ERR_TRUNCATED);
}

static void test_start_event_schedule(void) {
  struct fifa96_music m;
  struct fifa96_music_config c = base_config();
  events_clear();
  EVENTS[0].valid = 1;
  EVENTS[0].rate = 2;
  EVENTS[0].time = 0x10;
  EVENTS[0].tempo = 0;
  EVENTS[0].id = 9;
  TEMPO[0].a = 0x10000;
  setup(&m, &c);
  m.state = FIFA96_MUSIC_STATE_RANDOM;
  m.pos = 0x1000;
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(m.state == FIFA96_MUSIC_STATE_EVENT);
  assert(m.current_event == 0);
  assert(m.limit == 0x21000);
  assert(m.period == 1310);
  assert(R.play_calls == 1 && R.last_id == 9 && R.rand_calls == 0);
  rec_reset();

  EVENTS[0].time = 1;
  m.state = FIFA96_MUSIC_STATE_RANDOM;
  m.pos = 0x1000;
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(m.limit == 0xFFFF);
  assert(R.play_calls == 1 && R.last_id == 9);
  rec_reset();

  EVENTS[0].id = 0;
  m.state = FIFA96_MUSIC_STATE_RANDOM;
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(R.play_calls == 0);
  rec_reset();

  EVENTS[0].id = 4;
  EVENTS[0].rate = 0;
  EVENTS[0].time = 0x10;
  m.pos = 0x1000;
  m.state = FIFA96_MUSIC_STATE_RANDOM;
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(m.state == FIFA96_MUSIC_STATE_EVENT && m.limit == 0x1000);
  assert(R.play_calls == 1 && R.last_id == 4);

  EVENTS[0].rate = -1;
  EVENTS[0].time = 1;
  m.pos = 0;
  m.state = FIFA96_MUSIC_STATE_RANDOM;
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(m.state == FIFA96_MUSIC_STATE_EVENT);
  assert(m.limit == (int32_t)0xFFFF0000);
  assert(m.period == 655);

  EVENTS[0].rate = 2;
  EVENTS[0].time = 0x10;
  EVENTS[0].tempo = 9;
  m.state = FIFA96_MUSIC_STATE_RANDOM;
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(m.period == 0x20000);
  EVENTS[0].tempo = -1;
  m.state = FIFA96_MUSIC_STATE_RANDOM;
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(m.period == 0x20000);
}

static void test_start_event_overdue(void) {
  struct fifa96_music m;
  struct fifa96_music_config c = base_config();
  events_clear();
  EVENTS[0].valid = 1;
  EVENTS[0].rate = 2;
  EVENTS[0].time = 1;
  EVENTS[0].tempo = 0;
  EVENTS[0].id = 7;
  TEMPO[0].a = 0x10000;
  setup(&m, &c);
  m.state = FIFA96_MUSIC_STATE_RANDOM;
  m.pos = 0x20000;
  m.limit = 0x55;
  m.period = 0x77;
  m.current_event = 0;
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(R.play_calls == 1 && R.last_id == 7);
  assert(m.state == FIFA96_MUSIC_STATE_RANDOM);
  assert(m.pos == 0x20000 && m.limit == 0x55 && m.period == 0x77);
  assert(m.current_event == 0);
  rec_reset();

  EVENTS[0].id = 0;
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(R.play_calls == 0 && m.state == FIFA96_MUSIC_STATE_RANDOM);

  EVENTS[0].id = 7;
  EVENTS[0].time = 3;
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(m.state == FIFA96_MUSIC_STATE_EVENT);
  assert(m.limit == 0x2FFFF);

  EVENTS[0].time = 2;
  m.state = FIFA96_MUSIC_STATE_RANDOM;
  assert(fifa96_music_start_event(&m, 0) == FIFA96_OK);
  assert(m.state == FIFA96_MUSIC_STATE_RANDOM);
}

static void test_tempo_state0(void) {
  struct fifa96_music m;
  struct fifa96_music_config c = base_config();
  events_clear();
  EVENTS[0].valid = 1;
  EVENTS[0].rate = 2;
  EVENTS[0].tempo = 0;
  setup(&m, &c);
  m.state = FIFA96_MUSIC_STATE_EVENT;
  m.current_event = 0;

  TEMPO[0].a = 0;
  assert(fifa96_music_tempo_calc(&m) == FIFA96_OK);
  assert(m.period == 0x20000);

  TEMPO[0].a = 1966;
  assert(fifa96_music_tempo_calc(&m) == FIFA96_OK);
  assert(m.period == 0x10000);

  TEMPO[0].a = 1967;
  assert(fifa96_music_tempo_calc(&m) == FIFA96_OK);
  assert(m.period == 43690);

  TEMPO[0].a = 0x10000;
  assert(fifa96_music_tempo_calc(&m) == FIFA96_OK);
  assert(m.period == 1310);

  TEMPO[0].a = 2000;
  EVENTS[0].rate = -2;
  assert(fifa96_music_tempo_calc(&m) == FIFA96_OK);
  assert(m.period == 43690);

  EVENTS[0].rate = 0x8000;
  TEMPO[0].a = 0;
  assert(fifa96_music_tempo_calc(&m) == FIFA96_OK);
  assert(m.period == (int32_t)0x80000000);

  EVENTS[0].rate = 2;
  EVENTS[0].tempo = FIFA96_MUSIC_TEMPO_MAX;
  assert(fifa96_music_tempo_calc(&m) == FIFA96_OK);
  assert(m.period == 0x20000);
  EVENTS[0].tempo = 0;
  assert(R.rand_calls == 0);
}

static void test_tempo_state1(void) {
  struct fifa96_music m;
  struct fifa96_music_config c = base_config();
  events_clear();
  EVENTS[0].valid = 1;
  EVENTS[0].rate = 2;
  EVENTS[0].tempo = 0;
  TEMPO[0].b = 2000;
  c.limit_scale = 2;
  setup(&m, &c);
  m.state = FIFA96_MUSIC_STATE_TEMPO;
  m.current_event = 0;
  R.rand_value = 0x00018000;
  assert(fifa96_music_tempo_calc(&m) == FIFA96_OK);
  assert(R.rand_calls == 1);
  assert(m.limit == 0x10000);
  assert(m.period == 43690);
}

static void test_tempo_state2(void) {
  struct fifa96_music m;
  struct fifa96_music_config c = base_config();
  events_clear();
  setup(&m, &c);
  m.state = FIFA96_MUSIC_STATE_RANDOM;
  m.pos = 0x100;
  R.rand_value = 0x00028000;
  assert(fifa96_music_tempo_calc(&m) == FIFA96_OK);
  assert(R.rand_calls == 1);
  assert(m.limit == 0x10000);
  assert(m.period == 21760);

  m.pos = 0x20000;
  R.rand_value = 0x00008000;
  assert(fifa96_music_tempo_calc(&m) == FIFA96_OK);
  assert(R.rand_calls == 2);
  assert(m.limit == 0x10000);
  assert(m.period == 21845);

  m.state = (enum fifa96_music_state)9;
  m.period = 0x99;
  m.limit = 0x88;
  assert(fifa96_music_tempo_calc(&m) == FIFA96_OK);
  assert(R.rand_calls == 2 && m.period == 0x99 && m.limit == 0x88);
  assert(fifa96_music_tempo_calc(NULL) == -FIFA96_ERR_TRUNCATED);
}

static void test_tick(void) {
  struct fifa96_music m;
  struct fifa96_music_config c = base_config();
  events_clear();
  EVENTS[0].valid = 1;
  EVENTS[0].rate = 2;
  EVENTS[0].tempo = 0;
  TEMPO[0].a = 0x10000;
  TEMPO[0].b = 0x10000;
  setup(&m, &c);
  m.active = FIFA96_MUSIC_ACTIVE_STARTED;
  m.state = FIFA96_MUSIC_STATE_EVENT;
  m.pos = 0;
  m.limit = 100;
  m.period = 10;
  for (int i = 0; i < 9; i++) {
    assert(fifa96_music_tick(&m) == FIFA96_OK);
    assert(m.state == FIFA96_MUSIC_STATE_EVENT);
  }
  assert(m.pos == 90);
  R.rand_value = 5;
  assert(fifa96_music_tick(&m) == FIFA96_OK);
  assert(m.pos == 100 && m.state == FIFA96_MUSIC_STATE_TEMPO);
  assert(R.rand_calls == 1 && m.limit == 10);
  assert(m.period == 1310);

  setup(&m, &c);
  m.active = FIFA96_MUSIC_ACTIVE_STARTED;
  m.state = FIFA96_MUSIC_STATE_TEMPO;
  m.pos = 100;
  m.limit = 0;
  m.period = 10;
  R.rand_value = 5;
  for (int i = 0; i < 9; i++) {
    assert(fifa96_music_tick(&m) == FIFA96_OK);
    assert(m.state == FIFA96_MUSIC_STATE_TEMPO);
  }
  assert(m.pos == 10);
  assert(fifa96_music_tick(&m) == FIFA96_OK);
  assert(m.pos == 0 && m.state == FIFA96_MUSIC_STATE_RANDOM);
  assert(R.rand_calls == 1 && m.limit == 10);
  assert(m.period == 3);

  setup(&m, &c);
  m.active = FIFA96_MUSIC_ACTIVE_STARTED;
  m.state = FIFA96_MUSIC_STATE_RANDOM;
  m.pos = 0;
  m.limit = 5;
  m.period = 2;
  assert(fifa96_music_tick(&m) == FIFA96_OK);
  assert(m.pos == 2 && m.state == FIFA96_MUSIC_STATE_RANDOM);
  assert(R.rand_calls == 0);
  assert(fifa96_music_tick(&m) == FIFA96_OK);
  assert(m.pos == 4);
  assert(fifa96_music_tick(&m) == FIFA96_OK);
  assert(m.pos == 6 && m.state == FIFA96_MUSIC_STATE_RANDOM);
  assert(R.rand_calls == 1);

  setup(&m, &c);
  m.active = FIFA96_MUSIC_ACTIVE_STARTED;
  m.state = (enum fifa96_music_state)3;
  m.pos = 7;
  m.limit = 100;
  m.period = 5;
  assert(fifa96_music_tick(&m) == FIFA96_OK);
  assert(m.pos == 7 && R.rand_calls == 0);

  setup(&m, &c);
  m.active = FIFA96_MUSIC_ACTIVE_STARTED;
  m.state = FIFA96_MUSIC_STATE_EVENT;
  m.pos = 1;
  m.limit = 100;
  m.period = 5;
  m.enabled = 0;
  assert(fifa96_music_tick(&m) == FIFA96_OK);
  assert(m.pos == 1);
  m.enabled = 1;
  m.active = 0;
  assert(fifa96_music_tick(&m) == FIFA96_OK);
  assert(m.pos == 1);
  m.active = FIFA96_MUSIC_ACTIVE_STARTED;
  m.paused = 1;
  assert(fifa96_music_tick(&m) == FIFA96_OK);
  assert(m.pos == 1);
  m.paused = 0;
  assert(fifa96_music_tick(&m) == FIFA96_OK);
  assert(m.pos == 6);
  assert(fifa96_music_tick(NULL) == -FIFA96_ERR_TRUNCATED);
}

int main(void) {
  test_init();
  test_reset_disabled();
  test_reset_volume_clamp();
  test_reset_tracks();
  test_reset_schedule();
  test_start_event_bounds();
  test_start_event_schedule();
  test_start_event_overdue();
  test_tempo_state0();
  test_tempo_state1();
  test_tempo_state2();
  test_tick();
  printf("test_music OK\n");
  return 0;
}
