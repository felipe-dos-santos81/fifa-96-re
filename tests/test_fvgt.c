// tests/test_fvgt.c — golden and negative-path coverage for the fVGT delta decoder.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_fvgt.h"

#define CANVAS_W 320u
#define CANVAS_H 240u
#define CANVAS_N (CANVAS_W * CANVAS_H)

static uint32_t rd_u32le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put_bits(uint8_t *dst, size_t bit, uint32_t v, unsigned bits) {
  for (unsigned k = 0; k < bits; k++) {
    if ((v >> k) & 1u) dst[(bit + k) >> 3] |= (uint8_t)(1u << ((bit + k) & 7u));
  }
}

/* 4x4-canvas chunk (FU-33 §2 layout): one signed-10-bit (a,b) pair, no raw
   blocks, one palette record whose 2-bit indices are zero (all 16 expanded
   pixels = palette[0]), and a single 2-bit row-stream id. 0x24 bytes total. */
static size_t build_small(uint8_t *c, uint16_t a, uint16_t b, uint32_t id) {
  memset(c, 0, 0x24);
  c[0] = 'f'; c[1] = 'V'; c[2] = 'G'; c[3] = 'T';
  c[4] = 0x24;                                     /* declared total */
  c[8] = 1;                                        /* index count */
  c[12] = 1;                                       /* palette count */
  c[14] = 2;                                       /* row bits */
  put_bits(c + 0x14, 0, (uint32_t)(a & 0x3FFu), 10);
  put_bits(c + 0x14, 10, (uint32_t)(b & 0x3FFu), 10);
  c[0x18] = 1; c[0x19] = 2; c[0x1A] = 3; c[0x1B] = 4;  /* palette */
  put_bits(c + 0x20, 0, id, 2);                    /* row stream */
  return 0x24;
}

int main(void) {
  uint8_t *in = 0, *pre = 0, *want = 0;
  size_t in_n = 0, pre_n = 0, want_n = 0;
  assert(fifa96_file_read("tests/golden/vgt/fvgt-01.in.bin", &in, &in_n) == 0);
  assert(fifa96_file_read("tests/golden/vgt/fvgt-01.pre.bin", &pre, &pre_n) == 0);
  assert(fifa96_file_read("tests/golden/vgt/fvgt-01.out.bin", &want, &want_n) == 0);
  assert(in_n == 0x4000 && pre_n == CANVAS_N && want_n == CANVAS_N);
  assert(rd_u32le(in + 4) == 9972);

  size_t cap = CANVAS_N + 16;
  uint8_t *dst = malloc(cap);
  assert(dst);
  size_t got = 0;

  /* golden: both the 0x4000 slice length and the declared 9972 decode. */
  memset(dst, 0xA5, cap);
  assert(fifa96_fvgt_decode(in, in_n, CANVAS_W, CANVAS_H, pre, dst, cap, &got) == 0);
  assert(got == CANVAS_N);
  assert(memcmp(dst, want, CANVAS_N) == 0);
  for (size_t i = CANVAS_N; i < cap; i++) assert(dst[i] == 0xA5);
  memset(dst, 0xA5, cap);
  assert(fifa96_fvgt_decode(in, 9972, CANVAS_W, CANVAS_H, pre, dst, cap, &got) == 0);
  assert(got == CANVAS_N);
  assert(memcmp(dst, want, CANVAS_N) == 0);
  for (size_t i = CANVAS_N; i < cap; i++) assert(dst[i] == 0xA5);

  /* exact capacity is accepted. */
  uint8_t *exact = malloc(CANVAS_N);
  assert(exact);
  assert(fifa96_fvgt_decode(in, in_n, CANVAS_W, CANVAS_H, pre, exact, CANVAS_N, &got) == 0);
  assert(got == CANVAS_N);
  assert(memcmp(exact, want, CANVAS_N) == 0);

  /* dst_cap below the canvas: error, destination untouched. */
  memset(dst, 0xA5, cap);
  assert(fifa96_fvgt_decode(in, in_n, CANVAS_W, CANVAS_H, pre, dst, CANVAS_N - 1, &got) < 0);
  assert(got == 0);
  for (size_t i = 0; i < cap; i++) assert(dst[i] == 0xA5);

  /* truncated chunks: short header, header only, one byte short of the
     structural size (cuts the row stream), and a mid-payload cut. */
  assert(fifa96_fvgt_decode(in, 0x13, CANVAS_W, CANVAS_H, pre, dst, cap, &got) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_fvgt_decode(in, 0x14, CANVAS_W, CANVAS_H, pre, dst, cap, &got) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_fvgt_decode(in, 9971, CANVAS_W, CANVAS_H, pre, dst, cap, &got) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_fvgt_decode(in, 100, CANVAS_W, CANVAS_H, pre, dst, cap, &got) < 0);

  /* wrong tag. */
  uint8_t *bad = malloc(in_n);
  assert(bad);
  memcpy(bad, in, in_n);
  bad[0] = 'x';
  assert(fifa96_fvgt_decode(bad, in_n, CANVAS_W, CANVAS_H, pre, dst, cap, &got) ==
         -(int)FIFA96_ERR_BAD_MAGIC);

  /* NULL arguments. */
  assert(fifa96_fvgt_decode(NULL, in_n, CANVAS_W, CANVAS_H, pre, dst, cap, &got) < 0);
  assert(fifa96_fvgt_decode(in, in_n, CANVAS_W, CANVAS_H, NULL, dst, cap, &got) < 0);
  assert(fifa96_fvgt_decode(in, in_n, CANVAS_W, CANVAS_H, pre, NULL, cap, &got) < 0);
  assert(fifa96_fvgt_decode(in, in_n, CANVAS_W, CANVAS_H, pre, dst, cap, NULL) < 0);

  /* zero canvas dimensions. */
  assert(fifa96_fvgt_decode(in, in_n, 0, CANVAS_H, pre, dst, cap, &got) < 0);
  assert(fifa96_fvgt_decode(in, in_n, CANVAS_W, 0, pre, dst, cap, &got) < 0);

  /* synthetic 4x4 chunks: delta and block references decode; malformed
     references are rejected before any destination write. */
  uint8_t small[0x40];
  uint8_t canvas[16];
  uint8_t small_pre[16];
  for (size_t i = 0; i < sizeof small_pre; i++) small_pre[i] = (uint8_t)i;

  size_t small_n = build_small(small, 0, 0, 0);   /* delta id 0: copy pre */
  memset(canvas, 0xA5, sizeof canvas);
  assert(fifa96_fvgt_decode(small, small_n, 4, 4, small_pre, canvas, sizeof canvas, &got) == 0);
  assert(got == 16);
  assert(memcmp(canvas, small_pre, sizeof canvas) == 0);

  small_n = build_small(small, 0, 0, 1);          /* block id 1: palette[0] */
  memset(canvas, 0xA5, sizeof canvas);
  assert(fifa96_fvgt_decode(small, small_n, 4, 4, small_pre, canvas, sizeof canvas, &got) == 0);
  assert(got == 16);
  for (size_t i = 0; i < sizeof canvas; i++) assert(canvas[i] == 1);

  small_n = build_small(small, 0, 0, 3);          /* block id 3 > raw+pal */
  memset(canvas, 0xA5, sizeof canvas);
  assert(fifa96_fvgt_decode(small, small_n, 4, 4, small_pre, canvas, sizeof canvas, &got) < 0);
  assert(got == 0);
  for (size_t i = 0; i < sizeof canvas; i++) assert(canvas[i] == 0xA5);

  small_n = build_small(small, 511, 0, 0);        /* delta source past pre */
  memset(canvas, 0xA5, sizeof canvas);
  assert(fifa96_fvgt_decode(small, small_n, 4, 4, small_pre, canvas, sizeof canvas, &got) < 0);
  assert(got == 0);
  for (size_t i = 0; i < sizeof canvas; i++) assert(canvas[i] == 0xA5);

  /* canvas dimensions the original only partly rewrites are refused. */
  memset(dst, 0xA5, cap);
  assert(fifa96_fvgt_decode(in, in_n, 321, CANVAS_H, pre, dst, cap, &got) < 0);
  assert(got == 0);

  free(bad); free(exact); free(dst); free(in); free(pre); free(want);
  printf("test_fvgt OK\n");
  return 0;
}
