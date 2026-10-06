/* tests/test_engine_intro.c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_intro.h"

#define W 320
#define H 240
#define PIXELS (W * H)

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

/* The 320x240 keyframe test_play.py builds: the committed 96x100 keyframe
 * supplies the palette; the record is a 0x6A/0xFB literal copy (BE24 length)
 * of the 320x240 pre-frame. The length field covers the whole chunk. */
static void build_kvgt_320x240(uint8_t *dst, const uint8_t *palette, const uint8_t *pixels) {
  size_t n = 0x14u + 768u + 5u + PIXELS;
  memset(dst, 0, 0x14);
  dst[0] = 'k'; dst[1] = 'V'; dst[2] = 'G'; dst[3] = 'T';
  put32le(dst + 4, (uint32_t)n);
  put16le(dst + 8, W);
  put16le(dst + 10, H);
  put16le(dst + 12, 0);
  put16le(dst + 14, 256);
  memcpy(dst + 0x14, palette, 768);
  uint8_t *rec = dst + 0x14 + 768;
  rec[0] = 0x6A; rec[1] = 0xFB;
  rec[2] = (uint8_t)(PIXELS >> 16);
  rec[3] = (uint8_t)(PIXELS >> 8);
  rec[4] = (uint8_t)PIXELS;
  memcpy(rec + 5, pixels, PIXELS);
}

int main(void) {
  size_t klen = 0;
  uint8_t *key = slurp("tests/golden/vgt/kvgt-frame-01.bin", &klen);
  assert(key && klen == 5788u);
  size_t pre_len = 0;
  uint8_t *pre = slurp("tests/golden/vgt/fvgt-01.pre.bin", &pre_len);
  assert(pre && pre_len == PIXELS);
  const uint8_t *palette = key + 0x14;

  size_t chunk_len = 0x14u + 768u + 5u + PIXELS;
  uint8_t *chunk = malloc(chunk_len);
  assert(chunk);
  build_kvgt_320x240(chunk, palette, pre);

  struct fifa96_surface *s = fifa96_surface_create(W, H);
  assert(s);

  struct fifa96_intro intro;
  assert(fifa96_intro_start(&intro, s) == 0);
  assert(fifa96_intro_done(&intro) == 0);
  assert(fifa96_intro_feed(&intro, chunk, chunk_len) == 0);

  /* one key frame: pixels and palette land on the surface, not yet ended */
  assert(fifa96_intro_step(&intro, s) == 0);
  assert(memcmp(s->indexed, pre, PIXELS) == 0);
  assert(memcmp(s->palette, palette, 768) == 0);
  assert(fifa96_intro_done(&intro) == 0);

  /* the next step latches the end; the surface is left alone */
  assert(fifa96_intro_step(&intro, s) == 0);
  assert(fifa96_intro_done(&intro) == 1);
  assert(memcmp(s->indexed, pre, PIXELS) == 0);
  assert(fifa96_intro_step(&intro, s) == 0);
  assert(memcmp(s->indexed, pre, PIXELS) == 0);

  /* a sub-320x240 frame is refused before it can touch the surface */
  struct fifa96_intro small;
  assert(fifa96_intro_start(&small, s) == 0);
  assert(fifa96_intro_feed(&small, key, klen) == 0);
  assert(fifa96_intro_step(&small, s) < 0);
  assert(memcmp(s->indexed, pre, PIXELS) == 0);
  assert(memcmp(s->palette, palette, 768) == 0);

  /* defensive surfaces */
  assert(fifa96_intro_start(NULL, s) < 0);
  assert(fifa96_intro_start(&intro, NULL) < 0);
  assert(fifa96_intro_feed(NULL, chunk, chunk_len) < 0);
  assert(fifa96_intro_step(NULL, s) < 0);
  assert(fifa96_intro_step(&intro, NULL) < 0);
  assert(fifa96_intro_done(NULL) == 1);

  fifa96_surface_destroy(s);
  free(chunk);
  free(pre);
  free(key);
  puts("test_engine_intro OK");
  return 0;
}
