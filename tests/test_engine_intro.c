/* tests/test_engine_intro.c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_intro.h"
#include "fifa96_loader/fifa96_kvgt.h"

#define W 320
#define H 240
#define FW 96
#define FH 100
#define FPIXELS (FW * FH)

static uint8_t *slurp(const char *path, size_t *len) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t *buf = malloc((size_t)n);
  if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) { free(buf); fclose(f); return NULL; }
  fclose(f);
  *len = (size_t)n;
  return buf;
}

static void put16le(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}

static void put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

/* Minimal kVGT chunk for the malformed-geometry cases. */
static size_t build_kvgt(uint8_t *dst, uint16_t w, uint16_t h, const uint8_t *rec, size_t rec_n) {
  size_t n = 0x14u + rec_n;
  memset(dst, 0, n);
  dst[0] = 'k'; dst[1] = 'V'; dst[2] = 'G'; dst[3] = 'T';
  put32le(dst + 4, (uint32_t)n);
  put16le(dst + 8, w);
  put16le(dst + 10, h);
  memcpy(dst + 0x14, rec, rec_n);
  return n;
}

int main(void) {
  size_t klen = 0;
  uint8_t *key = slurp("tests/golden/vgt/kvgt-frame-01.bin", &klen);
  assert(key && klen == 5788u);
  const uint8_t *palette = key + 0x14;

  /* independent decode of the 96x100 fixture for the expected pixels */
  uint8_t *expect = malloc(FPIXELS);
  assert(expect);
  size_t expect_n = 0;
  assert(fifa96_kvgt_decode(key, klen, expect, FPIXELS, &expect_n, NULL) == 0);
  assert(expect_n == FPIXELS);

  struct fifa96_surface *s = fifa96_surface_create(W, H);
  assert(s);
  fifa96_surface_clear(s, 0x5A);

  struct fifa96_intro intro;
  assert(fifa96_intro_start(&intro, s) == 0);
  assert(fifa96_intro_done(&intro) == 0);
  assert(fifa96_intro_feed(&intro, key, klen) == 0);

  /* one 96x100 key frame blits at (0,0); the rest stays at the sentinel */
  assert(fifa96_intro_step(&intro, s) == 0);
  for (int y = 0; y < FH; y++)
    assert(memcmp(s->indexed + (size_t)y * W, expect + (size_t)y * FW, FW) == 0);
  assert(memcmp(s->palette, palette, 768) == 0);
  assert(s->indexed[50 * W + 200] == 0x5A);   /* inside the rows, right of the frame */
  assert(s->indexed[150 * W + 10] == 0x5A);   /* below the frame */
  assert(fifa96_intro_done(&intro) == 0);

  /* the next step latches the end; the surface is left alone */
  assert(fifa96_intro_step(&intro, s) == 0);
  assert(fifa96_intro_done(&intro) == 1);
  for (int y = 0; y < FH; y++)
    assert(memcmp(s->indexed + (size_t)y * W, expect + (size_t)y * FW, FW) == 0);
  assert(s->indexed[150 * W + 10] == 0x5A);
  assert(fifa96_intro_step(&intro, s) == 0);
  assert(memcmp(s->palette, palette, 768) == 0);

  /* zero-size and oversized frames are refused before touching the surface */
  static const uint8_t empty_rec[5] = {0x6A, 0xFB, 0, 0, 0};
  uint8_t chunk[0x14 + sizeof empty_rec];
  struct fifa96_intro bad;

  assert(fifa96_intro_start(&bad, s) == 0);
  assert(fifa96_intro_feed(&bad, chunk, build_kvgt(chunk, 0, 0, empty_rec, sizeof empty_rec)) == 0);
  assert(fifa96_intro_step(&bad, s) < 0);

  assert(fifa96_intro_start(&bad, s) == 0);
  assert(fifa96_intro_feed(&bad, chunk, build_kvgt(chunk, 321, 240, empty_rec, sizeof empty_rec)) == 0);
  assert(fifa96_intro_step(&bad, s) < 0);

  /* the refused frames left the earlier blit intact */
  for (int y = 0; y < FH; y++)
    assert(memcmp(s->indexed + (size_t)y * W, expect + (size_t)y * FW, FW) == 0);
  assert(s->indexed[150 * W + 10] == 0x5A);

  /* defensive surfaces */
  assert(fifa96_intro_start(NULL, s) < 0);
  assert(fifa96_intro_start(&intro, NULL) < 0);
  assert(fifa96_intro_feed(NULL, key, klen) < 0);
  assert(fifa96_intro_step(NULL, s) < 0);
  assert(fifa96_intro_step(&intro, NULL) < 0);
  assert(fifa96_intro_done(NULL) == 1);

  fifa96_surface_destroy(s);
  free(expect);
  free(key);
  puts("test_engine_intro OK");
  return 0;
}
