#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_platform_null.h"

struct null_state {
  struct fifa96_platform_null_config cfg;
  struct fifa96_platform_null_stats st;
  size_t tape_pos;
  uint32_t rate;
  int channels;
};

static uint64_t fnv1a(uint64_t h, const uint8_t *p, size_t n) {
  for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= 1099511628211ull; }
  return h;
}

static int n_init(void *self, int w, int h, const char *title) {
  (void)self; (void)w; (void)h; (void)title;
  return 0;
}
static void n_shutdown(void *self) { (void)self; }
static int n_poll(void *self, fifa96_platform_key *out, size_t cap, int *count) {
  struct null_state *s = self;
  size_t n = 0;
  while (s->tape_pos < s->cfg.tape_len && n < cap) {
    out[n++] = s->cfg.tape[s->tape_pos++];
  }
  *count = (int)n;
  return 0;
}
static int n_present(void *self, const fifa96_platform_frame *f) {
  struct null_state *s = self;
  s->st.presents++;
  s->st.frames += 240;
  s->st.present_hash = fnv1a(s->st.present_hash ? s->st.present_hash : 14695981039346656037ull,
                             f->planes[0], f->stride * 240);
  for (int p = 1; p < 4; p++) {
    s->st.present_hash = fnv1a(s->st.present_hash, f->planes[p], f->stride * 240);
  }
  s->st.present_hash = fnv1a(s->st.present_hash, f->palette, 768);
  return 0;
}
static void n_audio_open(void *self, uint32_t rate, int channels) {
  struct null_state *s = self; s->rate = rate; s->channels = channels;
}
static void n_audio_submit(void *self, const int16_t *pcm, size_t frames) {
  struct null_state *s = self;
  s->st.audio_frames += frames;
  s->st.audio_hash = fnv1a(s->st.audio_hash ? s->st.audio_hash : 14695981039346656037ull,
                           (const uint8_t *)pcm, frames * (size_t)s->channels * 2u);
}
static uint64_t n_now_ns(void *self) {
  struct null_state *s = self;
  return s->cfg.step_ns * (s->st.presents + 1u);
}
static void n_sleep_ns(void *self, uint64_t ns) { (void)self; (void)ns; }
static void n_destroy(void *self) { free(self); }

fifa96_platform *fifa96_platform_null_create(const struct fifa96_platform_null_config *cfg) {
  struct null_state *s = calloc(1, sizeof *s);
  if (!s) return NULL;
  if (cfg) s->cfg = *cfg;
  if (s->cfg.step_ns == 0) s->cfg.step_ns = 16666667ull;
  fifa96_platform *p = calloc(1, sizeof *p);
  if (!p) { free(s); return NULL; }
  p->init = n_init; p->shutdown = n_shutdown; p->destroy = n_destroy;
  p->poll = n_poll; p->present = n_present;
  p->audio_open = n_audio_open; p->audio_submit = n_audio_submit;
  p->now_ns = n_now_ns; p->sleep_ns = n_sleep_ns; p->self = s;
  return p;
}

void fifa96_platform_null_stats(const fifa96_platform *p, struct fifa96_platform_null_stats *out) {
  const struct null_state *s = p->self;
  *out = s->st;
}
