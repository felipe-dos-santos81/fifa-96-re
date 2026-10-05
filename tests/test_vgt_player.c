// tests/test_vgt_player.c — player loop over the committed TGV vectors.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_kvgt.h"
#include "fifa96_loader/fifa96_vgt_player.h"

#define KVGT_N 5788u
#define FVGT_N 9972u
#define PAL_N 768u

#define TAG_KVGT 0x5447566Bu
#define TAG_FVGT 0x54475666u
#define TAG_1SNH 0x684E5331u
#define TAG_1SND 0x644E5331u
#define TAG_REWIND 0xFFFFFFFFu
#define TAG_SKIP 0xFFFFFFFDu

static uint32_t rd_u32le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void wr_u32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

static size_t build_kvgt(uint8_t *dst, uint16_t w, uint16_t h, uint16_t count,
                         const uint8_t *pal, size_t pal_n,
                         const uint8_t *rec, size_t rec_n) {
  size_t n = 0x14 + pal_n * 3 + rec_n;
  memset(dst, 0, 0x14);
  dst[0] = 'k'; dst[1] = 'V'; dst[2] = 'G'; dst[3] = 'T';
  wr_u32le(dst + 4, (uint32_t)n);
  dst[8] = (uint8_t)w; dst[9] = (uint8_t)(w >> 8);
  dst[10] = (uint8_t)h; dst[11] = (uint8_t)(h >> 8);
  dst[12] = (uint8_t)count; dst[13] = (uint8_t)(count >> 8);
  dst[14] = (uint8_t)pal_n; dst[15] = (uint8_t)(pal_n >> 8);
  if (pal_n) memcpy(dst + 0x14, pal, pal_n * 3);
  memcpy(dst + 0x14 + pal_n * 3, rec, rec_n);
  return n;
}

static void put_sentinel(uint8_t *dst, uint32_t tag) {
  wr_u32le(dst, tag);
  wr_u32le(dst + 4, 8);
}

static void put_audio(uint8_t *dst, uint32_t tag, size_t payload_n) {
  wr_u32le(dst, tag);
  wr_u32le(dst + 4, (uint32_t)(payload_n + 8));
  for (size_t i = 0; i < payload_n; i++) dst[8 + i] = (uint8_t)(0x40 + i);
}

static int g_audio_calls;
static const uint8_t *g_audio_chunk;
static size_t g_audio_len;
static uint32_t g_audio_tag;
static void *g_audio_user;

static void note_audio(void *user, const uint8_t *chunk, size_t len) {
  g_audio_calls++;
  g_audio_chunk = chunk;
  g_audio_len = len;
  g_audio_tag = rd_u32le(chunk);
  g_audio_user = user;
}

int main(void) {
  uint8_t *real = 0, *in = 0, *pre = 0, *want = 0;
  uint8_t *in46 = 0, *out46 = 0;
  size_t real_n = 0, in_n = 0, pre_n = 0, want_n = 0, in46_n = 0, out46_n = 0;
  assert(fifa96_file_read("tests/golden/vgt/kvgt-frame-01.bin", &real, &real_n) == 0);
  assert(fifa96_file_read("tests/golden/vgt/fvgt-01.in.bin", &in, &in_n) == 0);
  assert(fifa96_file_read("tests/golden/vgt/fvgt-01.pre.bin", &pre, &pre_n) == 0);
  assert(fifa96_file_read("tests/golden/vgt/fvgt-01.out.bin", &want, &want_n) == 0);
  assert(fifa96_file_read("tests/golden/vgt/record-46.in.bin", &in46, &in46_n) == 0);
  assert(fifa96_file_read("tests/golden/vgt/record-46.out.bin", &out46, &out46_n) == 0);
  assert(real_n == KVGT_N && in_n == 16384 && pre_n == 76800 && want_n == 76800);
  assert(out46_n == 4696);

  /* direct-decoder expectation for the committed keyframe. */
  uint8_t *expect = malloc(9600);
  assert(expect);
  size_t expect_n = 0;
  assert(fifa96_kvgt_decode(real, real_n, expect, 9600, &expect_n, NULL) == 0);
  assert(expect_n == 9600);

  /* kVGT end-to-end: canvas replaced, palette published, then end. */
  {
    uint8_t *canvas = malloc(9600 + 16);
    uint8_t *scratch = malloc(9600 + 16);
    assert(canvas && scratch);
    memset(canvas, 0xA5, 9600 + 16);
    memset(scratch, 0xA5, 9600 + 16);
    fifa96_vgt_player p;
    assert(fifa96_vgt_player_init(&p, 96, 100, canvas, 9600, scratch, 9600, NULL, NULL) == 0);
    fifa96_vgt_player_feed(&p, real, real_n);
    fifa96_vgt_frame fr;
    assert(fifa96_vgt_player_step(&p, &fr) == 1);
    assert(fr.pixels == canvas && fr.pixels_len == 9600);
    assert(fr.width == 96 && fr.height == 100);
    assert(fr.palette_changed == 1);
    assert(fr.palette != NULL && memcmp(fr.palette, real + 0x14, PAL_N) == 0);
    assert(memcmp(fr.pixels, expect, 9600) == 0);
    assert(canvas[9600] == 0xA5 && scratch[9600] == 0xA5);
    assert(fifa96_vgt_player_ended(&p) == 0);
    assert(fifa96_vgt_player_step(&p, &fr) == 0);
    assert(fifa96_vgt_player_ended(&p) == 1);
    assert(fr.pixels == NULL && fr.pixels_len == 0);
    assert(fifa96_vgt_player_step(&p, &fr) == 0);

    /* re-feeding restarts the same player. */
    fifa96_vgt_player_feed(&p, real, real_n);
    assert(fifa96_vgt_player_step(&p, &fr) == 1);
    assert(memcmp(fr.pixels, expect, 9600) == 0);
    free(canvas); free(scratch);
  }

  /* fVGT end-to-end: delta against the primed canvas, no palette change. */
  {
    uint8_t *front = malloc(pre_n);
    uint8_t *back = malloc(pre_n);
    assert(front && back);
    memcpy(front, pre, pre_n);
    memset(back, 0xA5, pre_n);
    fifa96_vgt_player p;
    assert(fifa96_vgt_player_init(&p, 320, 240, front, pre_n, back, pre_n, NULL, NULL) == 0);
    fifa96_vgt_player_feed(&p, in, FVGT_N);
    fifa96_vgt_frame fr;
    assert(fifa96_vgt_player_step(&p, &fr) == 1);
    assert(fr.pixels == back && fr.pixels_len == pre_n);
    assert(fr.width == 320 && fr.height == 240);
    assert(fr.palette_changed == 0);
    assert(memcmp(fr.pixels, want, want_n) == 0);
    assert(fifa96_vgt_player_ended(&p) == 0);
    assert(fifa96_vgt_player_step(&p, &fr) == 0);
    assert(fifa96_vgt_player_ended(&p) == 1);
    free(front); free(back);
  }

  /* geometry change: committed 96x100 keyframe, then a 320x200 tree keyframe. */
  {
    size_t c46_n = 0x14 + in46_n;
    uint8_t *c46 = malloc(c46_n);
    uint8_t *seq = malloc(real_n + c46_n);
    assert(c46 && seq);
    assert(build_kvgt(c46, 320, 200, 0, NULL, 0, in46, in46_n) == c46_n);
    memcpy(seq, real, real_n);
    memcpy(seq + real_n, c46, c46_n);
    uint8_t *canvas = malloc(320 * 200);
    uint8_t *scratch = malloc(320 * 200);
    assert(canvas && scratch);
    fifa96_vgt_player p;
    assert(fifa96_vgt_player_init(&p, 96, 100, canvas, 320 * 200, scratch, 320 * 200, NULL, NULL) == 0);
    fifa96_vgt_player_feed(&p, seq, real_n + c46_n);
    fifa96_vgt_frame fr;
    assert(fifa96_vgt_player_step(&p, &fr) == 1);
    assert(fr.width == 96 && fr.height == 100 && fr.pixels_len == 9600);
    assert(memcmp(fr.pixels, expect, 9600) == 0);
    assert(fifa96_vgt_player_step(&p, &fr) == 1);
    assert(fr.width == 320 && fr.height == 200 && fr.pixels_len == out46_n);
    assert(memcmp(fr.pixels, out46, out46_n) == 0);
    assert(fifa96_vgt_player_step(&p, &fr) == 0);
    free(c46); free(seq); free(canvas); free(scratch);
  }

  /* capacity: init rejects bad geometry; an oversized keyframe is refused
     before the canvas is written. */
  {
    uint8_t *canvas = malloc(9600);
    uint8_t *scratch = malloc(9600);
    assert(canvas && scratch);
    fifa96_vgt_player p;
    assert(fifa96_vgt_player_init(NULL, 96, 100, canvas, 9600, scratch, 9600, NULL, NULL) < 0);
    assert(fifa96_vgt_player_init(&p, 0, 100, canvas, 9600, scratch, 9600, NULL, NULL) < 0);
    assert(fifa96_vgt_player_init(&p, 96, 100, NULL, 9600, scratch, 9600, NULL, NULL) < 0);
    assert(fifa96_vgt_player_init(&p, 96, 100, canvas, 9600, NULL, 9600, NULL, NULL) < 0);
    assert(fifa96_vgt_player_init(&p, 96, 100, canvas, 9599, scratch, 9600, NULL, NULL) < 0);
    assert(fifa96_vgt_player_init(&p, 96, 100, canvas, 9600, scratch, 9599, NULL, NULL) < 0);
    assert(fifa96_vgt_player_init(&p, 96, 100, canvas, 9600, canvas, 9600, NULL, NULL) < 0);

    size_t c46_n = 0x14 + in46_n;
    uint8_t *c46 = malloc(c46_n);
    assert(c46);
    assert(build_kvgt(c46, 320, 200, 0, NULL, 0, in46, in46_n) == c46_n);
    assert(fifa96_vgt_player_init(&p, 96, 100, canvas, 9600, scratch, 9600, NULL, NULL) == 0);
    fifa96_vgt_player_feed(&p, c46, c46_n);
    memset(canvas, 0xA5, 9600);
    fifa96_vgt_frame fr;
    assert(fifa96_vgt_player_step(&p, &fr) == -(int)FIFA96_ERR_TRUNCATED);
    assert(fifa96_vgt_player_ended(&p) == 1);
    for (size_t i = 0; i < 9600; i++) assert(canvas[i] == 0xA5);

    assert(fifa96_vgt_player_step(NULL, &fr) < 0);
    assert(fifa96_vgt_player_step(&p, NULL) < 0);
    assert(fifa96_vgt_player_ended(NULL) == 1);
    fifa96_vgt_player_feed(NULL, c46, c46_n);
    free(c46); free(canvas); free(scratch);
  }

  /* sentinels: -3 ends the stream; -1 rewinds to the base frame. */
  {
    uint8_t *s1 = malloc(real_n + 8);
    assert(s1);
    memcpy(s1, real, real_n);
    put_sentinel(s1 + real_n, TAG_SKIP);
    uint8_t *canvas = malloc(9600);
    uint8_t *scratch = malloc(9600);
    assert(canvas && scratch);
    fifa96_vgt_player p;
    assert(fifa96_vgt_player_init(&p, 96, 100, canvas, 9600, scratch, 9600, NULL, NULL) == 0);
    fifa96_vgt_player_feed(&p, s1, real_n + 8);
    fifa96_vgt_frame fr;
    assert(fifa96_vgt_player_step(&p, &fr) == 1);
    assert(fifa96_vgt_player_ended(&p) == 0);
    assert(fifa96_vgt_player_step(&p, &fr) == 0);
    assert(fifa96_vgt_player_ended(&p) == 1);
    free(s1);

    uint8_t *s2 = malloc(real_n + 8);
    assert(s2);
    memcpy(s2, real, real_n);
    put_sentinel(s2 + real_n, TAG_REWIND);
    fifa96_vgt_player_feed(&p, s2, real_n + 8);
    assert(fifa96_vgt_player_step(&p, &fr) == 1);
    assert(memcmp(fr.pixels, expect, 9600) == 0);
    assert(fifa96_vgt_player_step(&p, &fr) == 1);
    assert(memcmp(fr.pixels, expect, 9600) == 0);
    assert(fifa96_vgt_player_step(&p, &fr) == 1);
    assert(fifa96_vgt_player_ended(&p) == 0);
    free(s2);

    uint8_t *s3 = malloc(8 + real_n + 8);
    assert(s3);
    put_sentinel(s3, TAG_REWIND);
    memcpy(s3 + 8, real, real_n);
    put_sentinel(s3 + 8 + real_n, TAG_SKIP);
    fifa96_vgt_player_feed(&p, s3, 8 + real_n + 8);
    g_audio_calls = 0;
    assert(fifa96_vgt_player_step(&p, &fr) == 1);
    assert(memcmp(fr.pixels, expect, 9600) == 0);
    assert(g_audio_calls == 0);
    assert(fifa96_vgt_player_step(&p, &fr) == 0);
    assert(fifa96_vgt_player_ended(&p) == 1);
    free(s3); free(canvas); free(scratch);
  }

  /* malformed chunks latch the error and end the player. */
  {
    static uint8_t bad_len[16] = {'k', 'V', 'G', 'T', 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    static const uint8_t short_buf[4] = {0, 0, 0, 0};
    uint8_t *canvas = malloc(9600);
    uint8_t *scratch = malloc(9600);
    assert(canvas && scratch);
    fifa96_vgt_player p;
    assert(fifa96_vgt_player_init(&p, 96, 100, canvas, 9600, scratch, 9600, NULL, NULL) == 0);
    fifa96_vgt_frame fr;
    fifa96_vgt_player_feed(&p, bad_len, sizeof bad_len);
    assert(fifa96_vgt_player_step(&p, &fr) == -(int)FIFA96_ERR_TRUNCATED);
    assert(fifa96_vgt_player_ended(&p) == 1);
    fifa96_vgt_player_feed(&p, short_buf, sizeof short_buf);
    assert(fifa96_vgt_player_step(&p, &fr) == -(int)FIFA96_ERR_TRUNCATED);
    fifa96_vgt_player_feed(&p, real, 0x13);
    assert(fifa96_vgt_player_step(&p, &fr) == -(int)FIFA96_ERR_TRUNCATED);
    free(canvas); free(scratch);
  }

  /* companion chunks are notified undecoded; audio-only streams still end. */
  {
    size_t audio_n = 0x20;
    size_t s1_n = real_n + 8 + audio_n + 8;
    uint8_t *s1 = malloc(s1_n);
    uint8_t *canvas = malloc(9600);
    uint8_t *scratch = malloc(9600);
    assert(s1 && canvas && scratch);
    memcpy(s1, real, real_n);
    put_audio(s1 + real_n, TAG_1SNH, audio_n);
    put_sentinel(s1 + real_n + 8 + audio_n, TAG_SKIP);
    fifa96_vgt_player p;
    assert(fifa96_vgt_player_init(&p, 96, 100, canvas, 9600, scratch, 9600, note_audio, (void *)0x1234) == 0);
    fifa96_vgt_player_feed(&p, s1, s1_n);
    fifa96_vgt_frame fr;
    g_audio_calls = 0;
    assert(fifa96_vgt_player_step(&p, &fr) == 1);
    assert(g_audio_calls == 0);
    assert(fifa96_vgt_player_step(&p, &fr) == 0);
    assert(g_audio_calls == 1);
    assert(g_audio_tag == TAG_1SNH && g_audio_len == audio_n + 8);
    assert(g_audio_chunk == s1 + real_n);
    assert(g_audio_user == (void *)0x1234);
    assert(fifa96_vgt_player_ended(&p) == 1);
    free(s1);

    uint8_t *s2 = malloc(8 + audio_n + 8 + audio_n + 8);
    assert(s2);
    put_audio(s2, TAG_1SND, audio_n);
    put_audio(s2 + 8 + audio_n, TAG_1SND, audio_n);
    put_sentinel(s2 + 2 * (8 + audio_n), TAG_SKIP);
    fifa96_vgt_player_feed(&p, s2, 2 * (8 + audio_n) + 8);
    g_audio_calls = 0;
    assert(fifa96_vgt_player_step(&p, &fr) == 0);
    assert(g_audio_calls == 2);
    assert(fifa96_vgt_player_ended(&p) == 1);
    free(s2); free(canvas); free(scratch);
  }

  free(expect); free(real); free(in); free(pre); free(want); free(in46); free(out46);
  printf("test_vgt_player OK\n");
  return 0;
}
