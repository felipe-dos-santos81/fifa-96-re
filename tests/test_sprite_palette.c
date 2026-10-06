// tests/test_sprite_palette.c — FU-91 second chunk + 6-bit palette
// (docs/ghidra/FU91_sprite_palette.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_sprite.h"

_Static_assert(offsetof(fifa96_sprite_chunk, length) > offsetof(fifa96_sprite_chunk, data),
               "length");
_Static_assert(offsetof(fifa96_sprite_chunk, type) > offsetof(fifa96_sprite_chunk, length),
               "type");

#define R_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)
#define R_TRUNCATED ((fifa96_err_t)-FIFA96_ERR_TRUNCATED)
#define R_NOT_FOUND ((fifa96_err_t)-FIFA96_ERR_NOT_FOUND)
#define R_BAD_MAGIC ((fifa96_err_t)-FIFA96_ERR_BAD_MAGIC)

#define FRAME0 0x28u
#define CHUNK0_SEC 20u
#define CHUNK0_LEN 784u
#define FRAME1 (FRAME0 + CHUNK0_SEC + CHUNK0_LEN)
#define CHUNK1_SEC 18u
#define CHUNK1_LEN 24u
#define FRAME2 (FRAME1 + CHUNK1_SEC + CHUNK1_LEN)
#define BANK_TOTAL (FRAME2 + 17u)

static uint8_t bank_bytes[BANK_TOTAL];

static void put16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static void frame_header(uint8_t *p, uint32_t second, uint16_t w, uint16_t h) {
  memset(p, 0, 16);
  p[0] = 0x7B;
  p[1] = (uint8_t)second;
  p[2] = (uint8_t)(second >> 8);
  p[3] = (uint8_t)(second >> 16);
  put16(p + 4, w);
  put16(p + 6, h);
}

static void palette6(uint8_t *p, uint32_t n) {
  for (uint32_t i = 0; i < n; i++) p[i] = (uint8_t)((i * 3u) & 0x3Fu);
}

static void build_bank(void) {
  memset(bank_bytes, 0xFF, sizeof bank_bytes);
  memcpy(bank_bytes, "SHPI", 4);
  put32(bank_bytes + 4, BANK_TOTAL);
  put32(bank_bytes + 8, 3);
  memcpy(bank_bytes + 12, "GIMX", 4);
  memcpy(bank_bytes + 16, "p000", 4);
  put32(bank_bytes + 20, FRAME0);
  memcpy(bank_bytes + 24, "p001", 4);
  put32(bank_bytes + 28, FRAME1);
  memcpy(bank_bytes + 32, "p002", 4);
  put32(bank_bytes + 36, FRAME2);

  frame_header(bank_bytes + FRAME0, CHUNK0_SEC, 2, 2);
  memset(bank_bytes + FRAME0 + 16, 0x11, 4);
  uint8_t *c0 = bank_bytes + FRAME0 + CHUNK0_SEC;
  c0[0] = 0x22;
  put16(c0 + 4, 256);
  put16(c0 + 6, 1);
  put16(c0 + 8, 256);
  put16(c0 + 10, 0);
  put32(c0 + 12, 0);
  palette6(c0 + 16, 768);

  frame_header(bank_bytes + FRAME1, CHUNK1_SEC, 1, 1);
  uint8_t *c1 = bank_bytes + FRAME1 + CHUNK1_SEC;
  static const uint8_t walk_chunk[CHUNK1_LEN] = {
      0x7C, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x7B, 0x00, 0x00, 0x00,
      0xB4, 0xFF, 0xFF, 0xFF, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  memcpy(c1, walk_chunk, CHUNK1_LEN);

  frame_header(bank_bytes + FRAME2, 0, 1, 1);
}

static fifa96_sprite_bank parse_ok(const uint8_t *data, size_t len) {
  fifa96_sprite_bank bank;
  assert(fifa96_sprite_bank_parse(data, len, &bank) == FIFA96_OK);
  return bank;
}

static void test_chunk_parse(void) {
  fifa96_sprite_bank bank = parse_ok(bank_bytes, sizeof bank_bytes);
  fifa96_sprite_chunk c;
  assert(fifa96_sprite_chunk_parse(&bank, FRAME0, &c) == FIFA96_OK);
  assert(c.data == bank_bytes + FRAME0 + CHUNK0_SEC);
  assert(c.length == BANK_TOTAL - (FRAME0 + CHUNK0_SEC));
  assert(c.type == 0x22);
  assert(fifa96_sprite_chunk_parse(&bank, FRAME1, &c) == FIFA96_OK);
  assert(c.data == bank_bytes + FRAME1 + CHUNK1_SEC);
  assert(c.length == BANK_TOTAL - (FRAME1 + CHUNK1_SEC));
  assert(c.type == 0x7C);
  assert(fifa96_sprite_chunk_parse(&bank, FRAME2, &c) == R_NOT_FOUND);
}

static void test_chunk_parse_errors(void) {
  fifa96_sprite_bank bank = parse_ok(bank_bytes, sizeof bank_bytes);
  fifa96_sprite_chunk c;
  assert(fifa96_sprite_chunk_parse(NULL, FRAME0, &c) == R_INVALID);
  assert(fifa96_sprite_chunk_parse(&bank, FRAME0, NULL) == R_INVALID);
  assert(fifa96_sprite_chunk_parse(&bank, 0xFFFFFFFFu, &c) == R_TRUNCATED);

  uint8_t mut[BANK_TOTAL];
  memcpy(mut, bank_bytes, sizeof mut);
  put32(mut + 8, 1);
  put32(mut + 20, 0x18);
  memset(mut + 0x18, 0, 16);
  frame_header(mut + 0x18, 0x28, 1, 1);
  put32(mut + 4, 0x30);
  bank = parse_ok(mut, 0x30);
  fifa96_sprite_frame f;
  assert(fifa96_sprite_frame_parse(&bank, 0x18, &f) == FIFA96_OK);
  assert(f.second_offset == 0x28);
  assert(fifa96_sprite_chunk_parse(&bank, 0x18, &c) == R_TRUNCATED);
}

static void test_chunk_palette(void) {
  fifa96_sprite_bank bank = parse_ok(bank_bytes, sizeof bank_bytes);
  fifa96_sprite_chunk c;
  const uint8_t *rgb6 = NULL;
  uint16_t n = 0;
  assert(fifa96_sprite_chunk_parse(&bank, FRAME0, &c) == FIFA96_OK);
  assert(fifa96_sprite_chunk_palette(&c, &rgb6, &n) == FIFA96_OK);
  assert(n == 256);
  assert(rgb6 == c.data + 16);
  assert(rgb6[0] == 0 && rgb6[1] == 3 && rgb6[2] == 6);
  assert(rgb6[255 * 3] == (uint8_t)((765u * 3u) & 0x3Fu));
  assert(fifa96_sprite_chunk_parse(&bank, FRAME1, &c) == FIFA96_OK);
  assert(fifa96_sprite_chunk_palette(&c, &rgb6, &n) == R_BAD_MAGIC);

  assert(fifa96_sprite_chunk_palette(NULL, &rgb6, &n) == R_INVALID);
  assert(fifa96_sprite_chunk_palette(&c, NULL, &n) == R_INVALID);
  assert(fifa96_sprite_chunk_palette(&c, &rgb6, NULL) == R_INVALID);

  uint8_t mut[BANK_TOTAL];
  memcpy(mut, bank_bytes, sizeof mut);
  put32(mut + 4, 0x100);
  bank = parse_ok(mut, 0x100);
  assert(fifa96_sprite_chunk_parse(&bank, FRAME0, &c) == FIFA96_OK);
  assert(fifa96_sprite_chunk_palette(&c, &rgb6, &n) == R_TRUNCATED);

  memcpy(mut, bank_bytes, sizeof mut);
  put16(mut + FRAME0 + CHUNK0_SEC + 4, 0xFFFF);
  bank = parse_ok(mut, sizeof mut);
  assert(fifa96_sprite_chunk_parse(&bank, FRAME0, &c) == FIFA96_OK);
  assert(fifa96_sprite_chunk_palette(&c, &rgb6, &n) == R_TRUNCATED);
}

static void test_palette_to_rgb(void) {
  static const uint8_t six[8] = {0x00, 0x3F, 0x40, 0xFF, 0x01, 0x20, 0x3E, 0x3F};
  uint8_t rgb[24];
  assert(fifa96_sprite_palette_to_rgb(six, 8, rgb) == FIFA96_OK);
  assert(rgb[0] == 0x00 && rgb[1] == 0xFF && rgb[2] == 0x03 && rgb[3] == 0x08);
  assert(rgb[4] == (uint8_t)((1u * 255u) / 63u));
  assert(rgb[5] == (uint8_t)((0x20u * 255u) / 63u));
  assert(rgb[6] == (uint8_t)((0x3Eu * 255u) / 63u));
  assert(rgb[7] == 0xFF);
  uint8_t all[768];
  uint8_t out[768];
  for (uint32_t i = 0; i < 256; i++) {
    all[i * 3] = (uint8_t)i;
    all[i * 3 + 1] = (uint8_t)i;
    all[i * 3 + 2] = (uint8_t)i;
  }
  assert(fifa96_sprite_palette_to_rgb(all, 256, out) == FIFA96_OK);
  assert(out[0] == 0 && out[3 * 0x3F] == 0xFF && out[3 * 0x40] == 0x03 && out[3 * 0xFF] == 0x08);
  assert(fifa96_sprite_palette_to_rgb(NULL, 1, rgb) == R_INVALID);
  assert(fifa96_sprite_palette_to_rgb(six, 1, NULL) == R_INVALID);
  assert(fifa96_sprite_palette_to_rgb(six, 0, NULL) == FIFA96_OK);
}

int main(void) {
  build_bank();
  test_chunk_parse();
  test_chunk_parse_errors();
  test_chunk_palette();
  test_palette_to_rgb();
  puts("test_sprite_palette: all assertions passed");
  return 0;
}
