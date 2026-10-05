// tests/test_voice_control.c — FU-50 voice-control wrappers. Expected behavior
// is derived in docs/ghidra/FU50_voice_control.md: active 0xA6E1F, set-volume
// 0xA6BB3, set-pan 0xA6B21, start-slide 0xA6AA6, stop 0xA6CDC, gain 0xB9FDD,
// apply 0xB811B, pan split 0xA66AB.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_mixer.h"
#include "fifa96_loader/fifa96_voice.h"

static int cleanup_calls;
static void *cleanup_ctx_last;
static int cleanup_handle_last;

static void voice_cleanup(void *ctx, int handle) {
  cleanup_calls++;
  cleanup_ctx_last = ctx;
  cleanup_handle_last = handle;
}

static void registry_setup(struct fifa96_voice_registry *r) {
  fifa96_voice_registry_init(r);
  r->sound_state = 1;
}

static void mixer_setup(struct fifa96_mixer *m) {
  fifa96_mixer_init(m);
  m->voices[0].active = 1;
}

static void test_registry_init(void) {
  struct fifa96_voice_registry r;
  memset(&r, 0xAA, sizeof r);
  fifa96_voice_registry_init(&r);
  assert(r.sound_state == 0);
  assert(r.master == 0x7F);
  assert(r.cleanup == NULL && r.cleanup_ctx == NULL);
  for (int i = 0; i < FIFA96_VOICE_COUNT; i++) {
    assert(r.voice[i].state == 0 && r.voice[i].type == 0);
    assert(r.voice[i].caller == 0 && r.voice[i].volume == 0);
    assert(r.voice[i].gain == 0 && r.voice[i].pan == 0);
    assert(r.voice[i].slide_step == 0 && r.voice[i].slide_start == 0);
    assert(r.voice[i].slide_target == 0);
  }
  fifa96_voice_registry_init(NULL);
}

static void test_active(void) {
  struct fifa96_voice_registry r;
  registry_setup(&r);

  assert(fifa96_voice_active(&r, -1) == -FIFA96_ERR_VOICE_HANDLE);
  assert(fifa96_voice_active(&r, 16) == -FIFA96_ERR_VOICE_HANDLE);
  assert(fifa96_voice_active(NULL, 0) == -FIFA96_ERR_TRUNCATED);

  r.sound_state = 0;
  assert(fifa96_voice_active(&r, 0) == -FIFA96_ERR_TRUNCATED);
  r.sound_state = 6;
  assert(fifa96_voice_active(&r, 0) == -FIFA96_ERR_TRUNCATED);
  r.sound_state = 1;
  assert(fifa96_voice_active(&r, 0) == 1);
  assert(fifa96_voice_active(&r, 15) == 1);

  r.voice[0].state = 1;
  assert(fifa96_voice_active(&r, 0) == 0);
  r.voice[0].state = 2;
  assert(fifa96_voice_active(&r, 0) == 1);
  r.voice[0].state = 0xFF;
  assert(fifa96_voice_active(&r, 0) == 1);

  r.sound_state = 5;
  r.voice[15].state = 1;
  assert(fifa96_voice_active(&r, 15) == 0);
}

static void test_set_volume(void) {
  struct fifa96_voice_registry r;
  struct fifa96_mixer m;
  registry_setup(&r);
  mixer_setup(&m);
  r.voice[0].state = 1;
  r.voice[0].volume = 0x7F;
  r.voice[0].caller = 0x7F;

  assert(fifa96_voice_set_volume(&r, &m, 0, 0x40) == FIFA96_OK);
  assert(r.voice[0].pan == 0x40);
  assert(r.voice[0].gain == 0x7F);
  assert(m.voices[0].gain_l == 0x7F && m.voices[0].gain_r == 0x7F);

  assert(fifa96_voice_set_volume(&r, &m, 0, 0) == FIFA96_OK);
  assert(r.voice[0].pan == 0 && r.voice[0].gain == 0x7F);
  assert(m.voices[0].gain_l == 0x7F && m.voices[0].gain_r == 0);

  assert(fifa96_voice_set_volume(&r, &m, 0, 0xFF) == FIFA96_OK);
  assert(r.voice[0].pan == 0xFF && r.voice[0].gain == 0x7F);
  assert(m.voices[0].gain_l == 0x7F && m.voices[0].gain_r == 0);

  assert(fifa96_voice_set_volume(&r, &m, -1, 0x40) ==
         -FIFA96_ERR_VOICE_HANDLE);
  assert(fifa96_voice_set_volume(&r, &m, 16, 0x40) ==
         -FIFA96_ERR_VOICE_HANDLE);
  assert(fifa96_voice_set_volume(NULL, &m, 0, 0x40) ==
         -FIFA96_ERR_TRUNCATED);
  assert(fifa96_voice_set_volume(&r, NULL, 0, 0x40) ==
         -FIFA96_ERR_TRUNCATED);

  r.sound_state = 0;
  assert(fifa96_voice_set_volume(&r, &m, 0, 0x40) ==
         -FIFA96_ERR_TRUNCATED);
  r.sound_state = 6;
  assert(fifa96_voice_set_volume(&r, &m, 0, 0x40) ==
         -FIFA96_ERR_TRUNCATED);
  r.sound_state = 1;

  assert(fifa96_voice_set_volume(&r, &m, 0, -1) ==
         -FIFA96_ERR_VOICE_VOLUME);
  assert(fifa96_voice_set_volume(&r, &m, 0, 0x100) ==
         -FIFA96_ERR_VOICE_VOLUME);

  r.voice[0].state = 0;
  assert(fifa96_voice_set_volume(&r, &m, 0, 0x40) ==
         -FIFA96_ERR_VOICE_STATE);
  assert(fifa96_voice_set_volume(&r, &m, 0, 0x100) ==
         -FIFA96_ERR_VOICE_VOLUME);

  r.voice[0].state = 1;
  m.voices[0].active = 0;
  m.voices[0].gain_l = 0xAA;
  m.voices[0].gain_r = 0xBB;
  assert(fifa96_voice_set_volume(&r, &m, 0, 0x40) == FIFA96_OK);
  assert(r.voice[0].pan == 0x40 && r.voice[0].gain == 0x7F);
  assert(m.voices[0].gain_l == 0xAA && m.voices[0].gain_r == 0xBB);
}

static void test_set_pan(void) {
  struct fifa96_voice_registry r;
  struct fifa96_mixer m;
  registry_setup(&r);
  mixer_setup(&m);
  r.voice[0].state = 1;
  r.voice[0].volume = 0x7F;
  r.voice[0].caller = 0x7F;
  r.voice[0].pan = 0x40;

  assert(fifa96_voice_set_pan(&r, &m, 0, 0x40) == FIFA96_OK);
  assert(r.voice[0].caller == 0x40 && r.voice[0].gain == 0x40);
  assert(m.voices[0].gain_l == 0x40 && m.voices[0].gain_r == 0x40);

  assert(fifa96_voice_set_pan(&r, &m, 0, 0) == FIFA96_OK);
  assert(r.voice[0].caller == 0 && r.voice[0].gain == 0);
  assert(m.voices[0].gain_l == 0 && m.voices[0].gain_r == 0);

  assert(fifa96_voice_set_pan(&r, &m, 0, 0x7F) == FIFA96_OK);
  assert(r.voice[0].caller == 0x7F && r.voice[0].gain == 0x7F);

  assert(fifa96_voice_set_pan(&r, &m, -1, 0x40) ==
         -FIFA96_ERR_VOICE_HANDLE);
  assert(fifa96_voice_set_pan(&r, &m, 16, 0x40) ==
         -FIFA96_ERR_VOICE_HANDLE);
  assert(fifa96_voice_set_pan(NULL, &m, 0, 0x40) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_voice_set_pan(&r, NULL, 0, 0x40) == -FIFA96_ERR_TRUNCATED);

  r.sound_state = 0;
  assert(fifa96_voice_set_pan(&r, &m, 0, 0x40) == -FIFA96_ERR_TRUNCATED);
  r.sound_state = 1;

  assert(fifa96_voice_set_pan(&r, &m, 0, -1) == -FIFA96_ERR_VOICE_PAN);
  assert(fifa96_voice_set_pan(&r, &m, 0, 0x80) == -FIFA96_ERR_VOICE_PAN);

  r.voice[0].state = 0;
  assert(fifa96_voice_set_pan(&r, &m, 0, 0x40) ==
         -FIFA96_ERR_VOICE_STATE);
  assert(fifa96_voice_set_pan(&r, &m, 0, 0x80) == -FIFA96_ERR_VOICE_PAN);

  r.voice[0].state = 1;
  m.voices[0].active = 0;
  m.voices[0].gain_l = 0xAA;
  m.voices[0].gain_r = 0xBB;
  assert(fifa96_voice_set_pan(&r, &m, 0, 0x40) == FIFA96_OK);
  assert(r.voice[0].caller == 0x40 && r.voice[0].gain == 0x40);
  assert(m.voices[0].gain_l == 0xAA && m.voices[0].gain_r == 0xBB);
}

static void test_gain(void) {
  assert(fifa96_voice_gain(0x7F, 0x7F, 0x7F) == 0x7F);
  assert(fifa96_voice_gain(0x00, 0x7F, 0x7F) == 0x00);
  assert(fifa96_voice_gain(0x7F, 0x00, 0x7F) == 0x00);
  assert(fifa96_voice_gain(0x7F, 0x7F, 0x00) == 0x00);
  assert(fifa96_voice_gain(0x40, 0x40, 0x40) == 0x10);
  assert(fifa96_voice_gain(0x7F, 0x01, 0x01) == 0x00);

  assert(fifa96_voice_gain(0x7F, 0x80, 0x7F) == 0x80);
  assert(fifa96_voice_gain(0x80, 0x7F, 0x7F) == 0x80);
  assert(fifa96_voice_gain(0x7F, 0x7F, 0x80) == 0x80);
  assert(fifa96_voice_gain(0x7F, 0x80, 0x40) == 0xC0);
  assert(fifa96_voice_gain(0x7F, 0x80, 0x41) == 0xBF);
  assert(fifa96_voice_gain(0xFF, 0xFF, 0xFF) == 0x00);
}

static void test_gain_packed(void) {
  assert(fifa96_mixer_pan_packed(0x40, 0x7F) == 0x007F007Fu);
  assert(fifa96_mixer_pan_packed(0x00, 0x7F) == 0x0000007Fu);
  assert(fifa96_mixer_pan_packed(0x7F, 0x7F) == 0x007F0000u);
  assert(fifa96_mixer_pan_packed(0x20, 0x7F) == 0x0040007Fu);
  assert(fifa96_mixer_pan_packed(0xC0, 0x7F) == 0x007E007Fu);
  assert(fifa96_mixer_pan_packed(0x40, 0x80) == 0x07940790u);

  uint32_t l = 0, r = 0;
  fifa96_voice_gain_packed(0x40, 0x7F, &l, &r);
  assert(l == (0x7Fu << 10) && r == (0x7Fu << 10));
  fifa96_voice_gain_packed(0x00, 0x7F, &l, &r);
  assert(l == (0x7Fu << 10) && r == 0);
  fifa96_voice_gain_packed(0x7F, 0x7F, &l, &r);
  assert(l == 0 && r == (0x7Fu << 10));
  fifa96_voice_gain_packed(0x40, 0x80, &l, &r);
  assert(l == (0x0790u << 10) && r == (0x0794u << 10));
  fifa96_voice_gain_packed(0x40, 0x7F, NULL, NULL);

  for (int pan = 0; pan <= 0xFF; pan++) {
    uint32_t wl = 0, wr = 0;
    fifa96_mixer_pan_gains_wide((uint8_t)pan, 0x7F, &wl, &wr);
    assert(((wr << 16) | wl) == fifa96_mixer_pan_packed((uint8_t)pan, 0x7F));
  }
}

static void test_apply_gains(void) {
  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  m.voices[3].active = 1;
  m.voices[3].gain_l = 0x11;
  m.voices[3].gain_r = 0x22;

  assert(fifa96_voice_apply_gains(&m, 3, 0x40, 0x7F) == FIFA96_OK);
  assert(m.voices[3].gain_l == 0x7F && m.voices[3].gain_r == 0x7F);
  assert(fifa96_voice_apply_gains(&m, 3, 0x00, 0x7F) == FIFA96_OK);
  assert(m.voices[3].gain_l == 0x7F && m.voices[3].gain_r == 0);
  assert(fifa96_voice_apply_gains(&m, 3, 0x40, 0x80) == FIFA96_OK);
  assert(m.voices[3].gain_l == 0x0790 && m.voices[3].gain_r == 0x0794);

  m.voices[4].active = 0;
  m.voices[4].gain_l = 0xAA;
  m.voices[4].gain_r = 0xBB;
  assert(fifa96_voice_apply_gains(&m, 4, 0x40, 0x7F) == FIFA96_OK);
  assert(m.voices[4].gain_l == 0xAA && m.voices[4].gain_r == 0xBB);

  assert(fifa96_voice_apply_gains(NULL, 0, 0x40, 0x7F) ==
         -FIFA96_ERR_TRUNCATED);
  assert(fifa96_voice_apply_gains(&m, -1, 0x40, 0x7F) ==
         -FIFA96_ERR_TRUNCATED);
  assert(fifa96_voice_apply_gains(&m, 16, 0x40, 0x7F) ==
         -FIFA96_ERR_TRUNCATED);
}

static void test_start_slide(void) {
  struct fifa96_voice_registry r;
  registry_setup(&r);
  r.voice[0].state = 1;
  r.voice[0].caller = 0;

  assert(fifa96_voice_start_slide(&r, 0, 1, 3) == FIFA96_OK);
  assert(r.voice[0].slide_target == 0x10000);
  assert(r.voice[0].slide_start == 0);
  assert(r.voice[0].slide_step == 21845);

  assert(fifa96_voice_start_slide(&r, 0, -1, 3) == FIFA96_OK);
  assert(r.voice[0].slide_target == -65536);
  assert(r.voice[0].slide_step == -21845);

  assert(fifa96_voice_start_slide(&r, 0, 5, 0) == FIFA96_OK);
  assert(r.voice[0].slide_step == 5 * 0x10000);
  assert(fifa96_voice_start_slide(&r, 0, 5, -7) == FIFA96_OK);
  assert(r.voice[0].slide_step == 5 * 0x10000);
  assert(fifa96_voice_start_slide(&r, 0, 5, 1) == FIFA96_OK);
  assert(r.voice[0].slide_step == 5 * 0x10000);

  assert(fifa96_voice_start_slide(&r, 0, INT32_MAX, 1) == FIFA96_OK);
  assert(r.voice[0].slide_target == -65536);
  assert(r.voice[0].slide_step == -65536);
  assert(fifa96_voice_start_slide(&r, 0, INT32_MIN, 1) == FIFA96_OK);
  assert(r.voice[0].slide_target == 0);
  assert(r.voice[0].slide_step == 0);

  r.voice[0].caller = 0x80;
  assert(fifa96_voice_start_slide(&r, 0, 0, 1) == FIFA96_OK);
  assert(r.voice[0].slide_start == -8388608);
  assert(r.voice[0].slide_step == 0x800000);
  r.voice[0].caller = 0xFF;
  assert(fifa96_voice_start_slide(&r, 0, 0x100, 1) == FIFA96_OK);
  assert(r.voice[0].slide_start == -65536);
  assert(r.voice[0].slide_step == 0x1010000);

  assert(fifa96_voice_start_slide(&r, -1, 0, 1) ==
         -FIFA96_ERR_VOICE_HANDLE);
  assert(fifa96_voice_start_slide(&r, 16, 0, 1) ==
         -FIFA96_ERR_VOICE_HANDLE);
  assert(fifa96_voice_start_slide(NULL, 0, 0, 1) == -FIFA96_ERR_TRUNCATED);

  r.sound_state = 0;
  assert(fifa96_voice_start_slide(&r, 0, 0, 1) == -FIFA96_ERR_TRUNCATED);
  r.sound_state = 6;
  assert(fifa96_voice_start_slide(&r, 0, 0, 1) == -FIFA96_ERR_TRUNCATED);
  r.sound_state = 1;

  r.voice[0].state = 0;
  assert(fifa96_voice_start_slide(&r, 0, 0, 1) ==
         -FIFA96_ERR_VOICE_SLIDE);
  r.voice[0].state = 2;
  assert(fifa96_voice_start_slide(&r, 0, 0, 1) ==
         -FIFA96_ERR_VOICE_SLIDE);
}

static void test_stop(void) {
  struct fifa96_voice_registry r;
  struct fifa96_mixer m;
  registry_setup(&r);
  mixer_setup(&m);
  r.cleanup = voice_cleanup;
  r.cleanup_ctx = &r;
  cleanup_calls = 0;

  r.voice[0].state = 1;
  r.voice[0].type = 0;
  assert(fifa96_voice_stop(&r, &m, 0) == FIFA96_OK);
  assert(m.voices[0].active == 0);
  assert(cleanup_calls == 0);
  assert(r.voice[0].state == 1);

  m.voices[1].active = 1;
  r.voice[1].state = 1;
  r.voice[1].type = 2;
  assert(fifa96_voice_stop(&r, &m, 1) == FIFA96_OK);
  assert(m.voices[1].active == 0);
  assert(cleanup_calls == 1);
  assert(cleanup_ctx_last == &r && cleanup_handle_last == 1);
  assert(r.voice[1].state == 1);

  m.voices[1].active = 1;
  r.cleanup = NULL;
  assert(fifa96_voice_stop(&r, &m, 1) == FIFA96_OK);
  assert(m.voices[1].active == 0 && cleanup_calls == 1);
  r.cleanup = voice_cleanup;

  assert(fifa96_voice_stop(&r, &m, -1) == -FIFA96_ERR_VOICE_HANDLE);
  assert(fifa96_voice_stop(&r, &m, 16) == -FIFA96_ERR_VOICE_HANDLE);
  assert(fifa96_voice_stop(NULL, &m, 0) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_voice_stop(&r, NULL, 0) == -FIFA96_ERR_TRUNCATED);

  r.sound_state = 0;
  assert(fifa96_voice_stop(&r, &m, 0) == -FIFA96_ERR_TRUNCATED);
  r.sound_state = 1;

  r.voice[2].state = 0;
  m.voices[2].active = 1;
  assert(fifa96_voice_stop(&r, &m, 2) == -FIFA96_ERR_VOICE_STATE);
  assert(m.voices[2].active == 1);
}

int main(void) {
  test_registry_init();
  test_active();
  test_set_volume();
  test_set_pan();
  test_gain();
  test_gain_packed();
  test_apply_gains();
  test_start_slide();
  test_stop();
  printf("test_voice_control OK\n");
  return 0;
}
