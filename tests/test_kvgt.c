// tests/test_kvgt.c — kVGT header/palette/record integration over committed vectors.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_kvgt.h"

static uint8_t *build_frame(uint16_t w, uint16_t h, uint16_t count,
                            const uint8_t *pal, size_t pal_n,
                            const uint8_t *rec, size_t rec_n) {
  size_t n = 0x14 + pal_n * 3 + rec_n;
  uint8_t *f = malloc(n);
  assert(f);
  memset(f, 0xEE, 0x14);
  f[0] = 'k'; f[1] = 'V'; f[2] = 'G'; f[3] = 'T';
  f[8] = (uint8_t)(w >> 8); f[9] = (uint8_t)w;
  f[10] = (uint8_t)(h >> 8); f[11] = (uint8_t)h;
  f[12] = (uint8_t)(count >> 8); f[13] = (uint8_t)count;
  f[14] = (uint8_t)(pal_n >> 8); f[15] = (uint8_t)pal_n;
  if (pal_n) memcpy(f + 0x14, pal, pal_n * 3);
  memcpy(f + 0x14 + pal_n * 3, rec, rec_n);
  return f;
}

int main(void) {
  uint8_t *in30 = 0, *out30 = 0, *in46 = 0, *out46 = 0;
  size_t in30_n = 0, out30_n = 0, in46_n = 0, out46_n = 0;
  assert(fifa96_file_read("tests/golden/vgt/record-30.in.bin", &in30, &in30_n) == 0);
  assert(fifa96_file_read("tests/golden/vgt/record-30.out.bin", &out30, &out30_n) == 0);
  assert(fifa96_file_read("tests/golden/vgt/record-46.in.bin", &in46, &in46_n) == 0);
  assert(fifa96_file_read("tests/golden/vgt/record-46.out.bin", &out46, &out46_n) == 0);
  assert(out30_n == 2271 && out46_n == 4696);

  static const uint8_t pal[6] = {1, 2, 3, 4, 5, 6};
  size_t cap = out30_n + 16;
  uint8_t *dst = malloc(cap);
  assert(dst);

  /* huff record inside a 96x100 frame with a 2-entry palette. */
  uint8_t *f30 = build_frame(96, 100, 7, pal, 2, in30, in30_n);
  size_t f30_n = 0x14 + 2 * 3 + in30_n;
  struct fifa96_kvgt_info info;
  memset(&info, 0xAA, sizeof info);
  memset(dst, 0xA5, cap);
  size_t got = 0;
  assert(fifa96_kvgt_decode(f30, f30_n, dst, cap, &got, &info) == 0);
  assert(got == out30_n);
  assert(memcmp(dst, out30, out30_n) == 0);
  for (size_t i = out30_n; i < cap; i++) assert(dst[i] == 0xA5);
  assert(info.width == 96 && info.height == 100);
  assert(info.count == 7 && info.palette_count == 2);
  assert(memcmp(info.palette, pal, 6) == 0);
  assert(info.palette[6] == 0);  /* untouched entries cleared */

  /* tree record inside a 320x200 frame with no palette; info may be NULL. */
  uint8_t *f46 = build_frame(320, 200, 0, NULL, 0, in46, in46_n);
  size_t f46_n = 0x14 + in46_n;
  uint8_t *dst46 = malloc(out46_n + 16);
  assert(dst46);
  memset(dst46, 0xA5, out46_n + 16);
  assert(fifa96_kvgt_decode(f46, f46_n, dst46, out46_n + 16, &got, NULL) == 0);
  assert(got == out46_n);
  assert(memcmp(dst46, out46, out46_n) == 0);
  for (size_t i = out46_n; i < out46_n + 16; i++) assert(dst46[i] == 0xA5);

  /* non-kVGT tag. */
  uint8_t *bad = build_frame(96, 100, 0, NULL, 0, in30, in30_n);
  bad[0] = 'X';
  assert(fifa96_kvgt_decode(bad, 0x14 + in30_n, dst, cap, &got, NULL) ==
         -(int)FIFA96_ERR_BAD_MAGIC);

  /* palette_count over the 256-triple limit. */
  uint8_t *bigpal = build_frame(96, 100, 0, NULL, 0, in30, in30_n);
  bigpal[14] = 0x01; bigpal[15] = 0x01;  /* 257 */
  assert(fifa96_kvgt_decode(bigpal, 0x14 + in30_n, dst, cap, &got, NULL) < 0);

  /* truncated header / palette / record. */
  assert(fifa96_kvgt_decode(f30, 0x13, dst, cap, &got, NULL) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_kvgt_decode(f30, 0x14 + 2 * 3 - 1, dst, cap, &got, NULL) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_kvgt_decode(f30, 0x14 + 2 * 3 + 4, dst, cap, &got, NULL) < 0);

  /* destination capacity below the record's declared length. */
  assert(fifa96_kvgt_decode(f30, f30_n, dst, 16, &got, NULL) < 0);

  /* NULL arguments. */
  assert(fifa96_kvgt_decode(NULL, f30_n, dst, cap, &got, NULL) < 0);
  assert(fifa96_kvgt_decode(f30, f30_n, NULL, cap, &got, NULL) < 0);
  assert(fifa96_kvgt_decode(f30, f30_n, dst, cap, NULL, NULL) < 0);

  free(f30); free(f46); free(bad); free(bigpal); free(dst); free(dst46);
  free(in30); free(out30); free(in46); free(out46);
  printf("test_kvgt OK\n");
  return 0;
}
