// tests/test_sfx_event.c — FU-49 SFX event registry. Expected behavior is
// derived in docs/ghidra/FU49_sfx_events_settings.md: registrar 0x652F0 /
// loop 0x6536C, play 0x65544, volume 0x65488, param 0x65510, stop 0x653D8.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_sfx_event.h"

struct rec {
  int play_calls;
  int ready_calls;
  int volume_calls;
  int param_calls;
  int stop_calls;
  int release_calls;
  int32_t last_id;
  int32_t last_handle;
  int32_t last_volume;
  int32_t last_param;
  int32_t last_stop_param;
  int32_t play_result;
  int ready_result;
  int32_t volume_result;
  int32_t param_result;
  int32_t stop_result;
  int32_t release_result;
};

static struct rec R;

static void rec_reset(void) {
  memset(&R, 0, sizeof R);
  R.ready_result = 1;
  R.play_result = 7;
}

static int32_t rec_play(void *ctx, int32_t id) {
  (void)ctx;
  R.play_calls++;
  R.last_id = id;
  return R.play_result;
}

static int rec_ready(void *ctx, int32_t handle) {
  (void)ctx;
  R.ready_calls++;
  R.last_handle = handle;
  return R.ready_result;
}

static int32_t rec_volume(void *ctx, int32_t handle, uint32_t volume) {
  (void)ctx;
  R.volume_calls++;
  R.last_handle = handle;
  R.last_volume = (int32_t)volume;
  return R.volume_result;
}

static int32_t rec_param(void *ctx, int32_t handle, int32_t param) {
  (void)ctx;
  R.param_calls++;
  R.last_handle = handle;
  R.last_param = param;
  return R.param_result;
}

static int32_t rec_stop(void *ctx, int32_t handle, uint32_t param) {
  (void)ctx;
  R.stop_calls++;
  R.last_handle = handle;
  R.last_stop_param = (int32_t)param;
  return R.stop_result;
}

static int32_t rec_release(void *ctx, int32_t handle) {
  (void)ctx;
  R.release_calls++;
  R.last_handle = handle;
  return R.release_result;
}

static const struct fifa96_sfx_event_backend BACKEND = {
    rec_play, rec_ready, rec_volume, rec_param, rec_stop, rec_release, NULL};

static void setup(struct fifa96_sfx_events *e) {
  rec_reset();
  fifa96_sfx_event_init(e, &BACKEND);
  e->enabled = 1;
  e->armed = 1;
  e->audio = 1;
}

static void test_init(void) {
  struct fifa96_sfx_events e;
  memset(&e, 0xAA, sizeof e);
  fifa96_sfx_event_init(&e, NULL);
  assert(e.count == 0 && e.enabled == 0 && e.armed == 0 && e.audio == 0);
  assert(e.backend == NULL);
  for (uint32_t i = 0; i < FIFA96_SFX_EVENT_MAX; i++) {
    assert(e.slot[i].state == FIFA96_SFX_EVENT_NONE);
    assert(e.slot[i].id == 0);
    assert(e.slot[i].handle == (i < 8 ? -1 : 0));
  }
}

static void test_register(void) {
  struct fifa96_sfx_events e;
  setup(&e);
  assert(fifa96_sfx_event_register(&e, 0, 5) == FIFA96_OK);
  assert(e.slot[0].state == FIFA96_SFX_EVENT_ALLOCATED);
  assert(e.slot[0].id == 5);
  assert(e.slot[0].handle == -1);
  assert(fifa96_sfx_event_register(&e, 0, 9) == FIFA96_OK);
  assert(e.slot[0].id == 5);
  assert(fifa96_sfx_event_register(&e, 22, 3) == FIFA96_OK);
  assert(e.slot[22].id == 3);
  assert(fifa96_sfx_event_register(&e, FIFA96_SFX_EVENT_MAX, 3) ==
         -FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_event_register(NULL, 0, 0) == -FIFA96_ERR_TRUNCATED);
  e.enabled = 0;
  assert(fifa96_sfx_event_register(&e, 1, 4) == FIFA96_OK);
  assert(e.slot[1].state == FIFA96_SFX_EVENT_ALLOCATED);
}

static void test_register_all(void) {
  struct fifa96_sfx_events e;
  setup(&e);
  const int32_t ids[4] = {3, 0x17, -1, 4};
  assert(fifa96_sfx_event_register_all(&e, ids, 4) == FIFA96_OK);
  assert(e.count == 4 && e.armed == 1);
  assert(e.slot[0].state == FIFA96_SFX_EVENT_ALLOCATED && e.slot[0].id == 3);
  assert(e.slot[1].state == FIFA96_SFX_EVENT_NONE);
  assert(e.slot[2].state == FIFA96_SFX_EVENT_ALLOCATED && e.slot[2].id == -1);
  assert(e.slot[3].state == FIFA96_SFX_EVENT_ALLOCATED && e.slot[3].id == 4);
  assert(fifa96_sfx_event_register_all(&e, ids, FIFA96_SFX_EVENT_MAX + 1) ==
         -FIFA96_ERR_TRUNCATED);
  assert(e.count == 4);
  assert(fifa96_sfx_event_register_all(NULL, ids, 4) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_event_register_all(&e, NULL, 4) == -FIFA96_ERR_TRUNCATED);

  struct fifa96_sfx_events d;
  setup(&d);
  d.enabled = 0;
  d.count = 7;
  d.armed = 0;
  assert(fifa96_sfx_event_register_all(&d, ids, 4) == FIFA96_OK);
  assert(d.count == 7 && d.armed == 0);
  assert(d.slot[0].state == FIFA96_SFX_EVENT_NONE);
}

static void test_play_gates(void) {
  struct fifa96_sfx_events e;
  setup(&e);
  e.count = 3;
  assert(fifa96_sfx_event_register(&e, 0, 7) == FIFA96_OK);
  assert(fifa96_sfx_event_register(&e, 2, 9) == FIFA96_OK);

  e.enabled = 0;
  rec_reset();
  assert(fifa96_sfx_event_play(&e, 0) == FIFA96_OK);
  assert(R.play_calls == 0);
  e.enabled = 1;

  e.armed = 0;
  rec_reset();
  assert(fifa96_sfx_event_play(&e, 0) == FIFA96_OK);
  assert(R.play_calls == 0);
  e.armed = 1;

  e.count = 1;
  rec_reset();
  assert(fifa96_sfx_event_play(&e, 2) == FIFA96_OK);
  assert(R.play_calls == 0);
  e.count = 3;

  e.audio = 0;
  rec_reset();
  assert(fifa96_sfx_event_play(&e, 0) == FIFA96_OK);
  assert(R.play_calls == 0);
  e.audio = 1;

  rec_reset();
  assert(fifa96_sfx_event_play(&e, 1) == FIFA96_OK);
  assert(R.play_calls == 0);

  rec_reset();
  assert(fifa96_sfx_event_play(&e, 0) == FIFA96_OK);
  assert(R.play_calls == 1 && R.last_id == 7);
  assert(e.slot[0].handle == 7 && e.slot[0].state == FIFA96_SFX_EVENT_PLAYING);

  assert(fifa96_sfx_event_play(&e, FIFA96_SFX_EVENT_MAX) ==
         -FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_event_play(NULL, 0) == -FIFA96_ERR_TRUNCATED);
}

static void test_play_ready_and_result(void) {
  struct fifa96_sfx_events e;
  setup(&e);
  e.count = 1;
  assert(fifa96_sfx_event_register(&e, 0, 7) == FIFA96_OK);

  rec_reset();
  R.ready_result = 0;
  assert(fifa96_sfx_event_play(&e, 0) == FIFA96_OK);
  assert(R.ready_calls == 1 && R.last_handle == -1 && R.play_calls == 0);
  assert(e.slot[0].state == FIFA96_SFX_EVENT_ALLOCATED);

  rec_reset();
  R.play_result = -4;
  assert(fifa96_sfx_event_play(&e, 0) == -4);
  assert(e.slot[0].handle == -4 &&
         e.slot[0].state == FIFA96_SFX_EVENT_STOPPED);

  struct fifa96_sfx_event_backend b = BACKEND;
  b.ready = NULL;
  fifa96_sfx_event_init(&e, &b);
  e.enabled = 1;
  e.armed = 1;
  e.audio = 1;
  e.count = 1;
  assert(fifa96_sfx_event_register(&e, 0, 7) == FIFA96_OK);
  rec_reset();
  assert(fifa96_sfx_event_play(&e, 0) == FIFA96_OK);
  assert(R.ready_calls == 0 && R.play_calls == 1);

  b.play = NULL;
  fifa96_sfx_event_init(&e, &b);
  e.enabled = 1;
  e.armed = 1;
  e.audio = 1;
  e.count = 1;
  assert(fifa96_sfx_event_register(&e, 0, 7) == FIFA96_OK);
  assert(fifa96_sfx_event_play(&e, 0) == -FIFA96_ERR_UNSUPPORTED);
  assert(e.slot[0].state == FIFA96_SFX_EVENT_ALLOCATED);
}

static void test_set_volume(void) {
  struct fifa96_sfx_events e;
  setup(&e);
  e.count = 1;
  assert(fifa96_sfx_event_register(&e, 0, 7) == FIFA96_OK);

  rec_reset();
  assert(fifa96_sfx_event_set_volume(&e, 0, 0x10) == FIFA96_OK);
  assert(R.volume_calls == 0);

  e.slot[0].handle = 7;
  e.slot[0].state = FIFA96_SFX_EVENT_PLAYING;

  rec_reset();
  assert(fifa96_sfx_event_set_volume(&e, 0, 0x80) == FIFA96_OK);
  assert(R.volume_calls == 1 && R.last_handle == 7 && R.last_volume == 0x7F);
  rec_reset();
  assert(fifa96_sfx_event_set_volume(&e, 0, -5) == FIFA96_OK);
  assert(R.last_volume == 0);
  rec_reset();
  assert(fifa96_sfx_event_set_volume(&e, 0, 0x7F) == FIFA96_OK);
  assert(R.last_volume == 0x7F);
  rec_reset();
  assert(fifa96_sfx_event_set_volume(&e, 0, 0x10) == FIFA96_OK);
  assert(R.last_volume == 0x10);

  e.enabled = 0;
  rec_reset();
  assert(fifa96_sfx_event_set_volume(&e, 0, 1) == FIFA96_OK);
  assert(R.volume_calls == 0);
  e.enabled = 1;
  e.armed = 0;
  rec_reset();
  assert(fifa96_sfx_event_set_volume(&e, 0, 1) == FIFA96_OK);
  assert(R.volume_calls == 0);
  e.armed = 1;

  e.audio = 0;
  e.count = 0;
  rec_reset();
  assert(fifa96_sfx_event_set_volume(&e, 0, 1) == FIFA96_OK);
  assert(R.volume_calls == 1);
  e.audio = 1;
  e.count = 1;

  rec_reset();
  assert(fifa96_sfx_event_set_volume(&e, 5, 1) == FIFA96_OK);
  assert(R.volume_calls == 0);

  assert(fifa96_sfx_event_set_volume(&e, FIFA96_SFX_EVENT_MAX, 1) ==
         -FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_event_set_volume(NULL, 0, 1) == -FIFA96_ERR_TRUNCATED);

  struct fifa96_sfx_events z;
  setup(&z);
  z.count = 11;
  assert(fifa96_sfx_event_register(&z, 10, 3) == FIFA96_OK);
  rec_reset();
  assert(fifa96_sfx_event_set_volume(&z, 10, 1) == FIFA96_OK);
  assert(R.volume_calls == 0);

  struct fifa96_sfx_event_backend b = BACKEND;
  b.volume = NULL;
  fifa96_sfx_event_init(&e, &b);
  e.enabled = 1;
  e.armed = 1;
  e.audio = 1;
  e.count = 1;
  assert(fifa96_sfx_event_register(&e, 0, 7) == FIFA96_OK);
  e.slot[0].handle = 7;
  e.slot[0].state = FIFA96_SFX_EVENT_PLAYING;
  assert(fifa96_sfx_event_set_volume(&e, 0, 1) == -FIFA96_ERR_UNSUPPORTED);
}

static void test_set_param(void) {
  struct fifa96_sfx_events e;
  setup(&e);
  e.count = 1;
  assert(fifa96_sfx_event_register(&e, 0, 7) == FIFA96_OK);
  e.slot[0].handle = 7;
  e.slot[0].state = FIFA96_SFX_EVENT_PLAYING;

  rec_reset();
  assert(fifa96_sfx_event_set_param(&e, 0, 0x1F4) == FIFA96_OK);
  assert(R.param_calls == 1 && R.last_handle == 7 && R.last_param == 0x1F4);
  rec_reset();
  assert(fifa96_sfx_event_set_param(&e, 0, -5) == FIFA96_OK);
  assert(R.last_param == -5);

  e.audio = 0;
  e.count = 0;
  rec_reset();
  assert(fifa96_sfx_event_set_param(&e, 0, 1) == FIFA96_OK);
  assert(R.param_calls == 1);
  e.audio = 1;
  e.count = 1;

  e.enabled = 0;
  rec_reset();
  assert(fifa96_sfx_event_set_param(&e, 0, 1) == FIFA96_OK);
  assert(R.param_calls == 0);
  e.enabled = 1;
  e.armed = 0;
  rec_reset();
  assert(fifa96_sfx_event_set_param(&e, 0, 1) == FIFA96_OK);
  assert(R.param_calls == 0);
  e.armed = 1;

  rec_reset();
  assert(fifa96_sfx_event_set_param(&e, 5, 1) == FIFA96_OK);
  assert(R.param_calls == 0);

  e.slot[0].handle = 0;
  rec_reset();
  assert(fifa96_sfx_event_set_param(&e, 0, 1) == FIFA96_OK);
  assert(R.param_calls == 0);

  assert(fifa96_sfx_event_set_param(&e, FIFA96_SFX_EVENT_MAX, 1) ==
         -FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_event_set_param(NULL, 0, 1) == -FIFA96_ERR_TRUNCATED);

  struct fifa96_sfx_event_backend b = BACKEND;
  b.param = NULL;
  fifa96_sfx_event_init(&e, &b);
  e.enabled = 1;
  e.armed = 1;
  e.audio = 1;
  e.count = 1;
  assert(fifa96_sfx_event_register(&e, 0, 7) == FIFA96_OK);
  e.slot[0].handle = 7;
  e.slot[0].state = FIFA96_SFX_EVENT_PLAYING;
  assert(fifa96_sfx_event_set_param(&e, 0, 1) == -FIFA96_ERR_UNSUPPORTED);
}

static void test_stop(void) {
  struct fifa96_sfx_events e;
  setup(&e);
  e.count = 1;
  assert(fifa96_sfx_event_register(&e, 0, 7) == FIFA96_OK);
  e.slot[0].handle = 7;
  e.slot[0].state = FIFA96_SFX_EVENT_PLAYING;

  rec_reset();
  assert(fifa96_sfx_event_stop(&e, 0, 1) == FIFA96_OK);
  assert(R.stop_calls == 1 && R.last_handle == 7 && R.last_stop_param == 1);
  assert(R.release_calls == 0);
  assert(e.slot[0].handle == 7 &&
         e.slot[0].state == FIFA96_SFX_EVENT_PLAYING);

  rec_reset();
  assert(fifa96_sfx_event_stop(&e, 0, 0x80) == FIFA96_OK);
  assert(R.last_stop_param == 0x7F && R.release_calls == 0);
  assert(e.slot[0].handle == 7);

  rec_reset();
  assert(fifa96_sfx_event_stop(&e, 0, 0) == FIFA96_OK);
  assert(R.stop_calls == 1 && R.last_stop_param == 0 && R.release_calls == 1);
  assert(e.slot[0].handle == -1 &&
         e.slot[0].state == FIFA96_SFX_EVENT_STOPPED);

  e.slot[0].handle = 7;
  e.slot[0].state = FIFA96_SFX_EVENT_PLAYING;
  rec_reset();
  assert(fifa96_sfx_event_stop(&e, 0, -9) == FIFA96_OK);
  assert(R.last_stop_param == 0 && R.release_calls == 1);
  assert(e.slot[0].handle == -1);

  e.slot[0].handle = 7;
  e.slot[0].state = FIFA96_SFX_EVENT_PLAYING;
  e.audio = 0;
  e.count = 0;
  rec_reset();
  assert(fifa96_sfx_event_stop(&e, 0, 5) == FIFA96_OK);
  assert(R.stop_calls == 1);
  e.audio = 1;
  e.count = 1;

  e.enabled = 0;
  e.slot[0].handle = 7;
  e.slot[0].state = FIFA96_SFX_EVENT_PLAYING;
  rec_reset();
  assert(fifa96_sfx_event_stop(&e, 0, 0) == FIFA96_OK);
  assert(R.stop_calls == 0 && e.slot[0].handle == 7);
  e.enabled = 1;
  e.armed = 0;
  rec_reset();
  assert(fifa96_sfx_event_stop(&e, 0, 0) == FIFA96_OK);
  assert(R.stop_calls == 0 && e.slot[0].handle == 7);
  e.armed = 1;

  rec_reset();
  assert(fifa96_sfx_event_stop(&e, 7, 0) == FIFA96_OK);
  assert(R.stop_calls == 0 && R.release_calls == 0);
  assert(e.slot[7].handle == -1 && e.slot[7].state == FIFA96_SFX_EVENT_NONE);

  struct fifa96_sfx_events z;
  setup(&z);
  z.count = 11;
  assert(fifa96_sfx_event_register(&z, 10, 3) == FIFA96_OK);
  rec_reset();
  assert(fifa96_sfx_event_stop(&z, 10, 0) == FIFA96_OK);
  assert(R.stop_calls == 0 && R.release_calls == 0);
  assert(z.slot[10].handle == -1 &&
         z.slot[10].state == FIFA96_SFX_EVENT_STOPPED);

  assert(fifa96_sfx_event_stop(&e, FIFA96_SFX_EVENT_MAX, 0) ==
         -FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_event_stop(NULL, 0, 0) == -FIFA96_ERR_TRUNCATED);

  struct fifa96_sfx_event_backend b = BACKEND;
  b.stop = NULL;
  fifa96_sfx_event_init(&e, &b);
  e.enabled = 1;
  e.armed = 1;
  e.audio = 1;
  e.count = 1;
  assert(fifa96_sfx_event_register(&e, 0, 7) == FIFA96_OK);
  e.slot[0].handle = 7;
  e.slot[0].state = FIFA96_SFX_EVENT_PLAYING;
  assert(fifa96_sfx_event_stop(&e, 0, 5) == -FIFA96_ERR_UNSUPPORTED);
  assert(e.slot[0].handle == 7);

  b.stop = rec_stop;
  b.release = NULL;
  fifa96_sfx_event_init(&e, &b);
  e.enabled = 1;
  e.armed = 1;
  e.audio = 1;
  e.count = 1;
  assert(fifa96_sfx_event_register(&e, 0, 7) == FIFA96_OK);
  e.slot[0].handle = 7;
  e.slot[0].state = FIFA96_SFX_EVENT_PLAYING;
  rec_reset();
  assert(fifa96_sfx_event_stop(&e, 0, 0) == -FIFA96_ERR_UNSUPPORTED);
  assert(e.slot[0].handle == 7);
}

int main(void) {
  test_init();
  test_register();
  test_register_all();
  test_play_gates();
  test_play_ready_and_result();
  test_set_volume();
  test_set_param();
  test_stop();
  printf("test_sfx_event OK\n");
  return 0;
}
