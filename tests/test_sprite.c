// tests/test_sprite.c — FU-86 sprite bank model
// (docs/ghidra/FU86_sprite_loading.md).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_sprite.h"

_Static_assert(offsetof(fifa96_sprite_bank, total_size) > offsetof(fifa96_sprite_bank, data),
               "total_size");
_Static_assert(offsetof(fifa96_sprite_bank, tag) > offsetof(fifa96_sprite_bank, count), "tag");
_Static_assert(offsetof(fifa96_sprite_entry, offset) > offsetof(fifa96_sprite_entry, name),
               "entry offset");
_Static_assert(offsetof(fifa96_sprite_frame, pixels) > offsetof(fifa96_sprite_frame, word12),
               "pixels");
_Static_assert(offsetof(fifa96_sprite_frame, pixel_len) > offsetof(fifa96_sprite_frame, pixels),
               "pixel_len");

#define R_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)
#define R_TRUNCATED ((fifa96_err_t)-FIFA96_ERR_TRUNCATED)
#define R_BAD_MAGIC ((fifa96_err_t)-FIFA96_ERR_BAD_MAGIC)

static uint8_t bank_bytes[0x434];

static void put32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static void build_bank(void) {
  memset(bank_bytes, 0xFF, sizeof bank_bytes);
  memcpy(bank_bytes, "SHPI", 4);
  put32(bank_bytes + 4, 0x434);
  put32(bank_bytes + 8, 2);
  memcpy(bank_bytes + 12, "GIMX", 4);
  memcpy(bank_bytes + 16, "p001", 4);
  put32(bank_bytes + 20, 0x20);
  memcpy(bank_bytes + 24, "p201", 4);
  put32(bank_bytes + 28, 0x41C);
  static const uint8_t frame0_header[16] = {0x7B, 0xE4, 0x03, 0x00, 0x14, 0x00, 0x31, 0x00,
                                            0x0A, 0x00, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00};
  memcpy(bank_bytes + 0x20, frame0_header, 16);
  static const uint8_t frame1_header[16] = {0x7B, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x00,
                                            0x01, 0x00, 0x02, 0x00, 0x44, 0x33, 0x22, 0x11};
  memcpy(bank_bytes + 0x41C, frame1_header, 16);
}

static fifa96_sprite_bank parse_ok(void) {
  fifa96_sprite_bank bank;
  assert(fifa96_sprite_bank_parse(bank_bytes, sizeof bank_bytes, &bank) == FIFA96_OK);
  return bank;
}

static void test_bank_parse(void) {
  fifa96_sprite_bank bank = parse_ok();
  assert(bank.data == bank_bytes);
  assert(bank.total_size == 0x434);
  assert(bank.count == 2);
  assert(memcmp(bank.tag, "GIMX", 4) == 0);
  uint8_t one[16];
  memset(one, 0, sizeof one);
  memcpy(one, "SHPI", 4);
  put32(one + 4, 16);
  put32(one + 8, 0);
  memcpy(one + 12, "GIMX", 4);
  assert(fifa96_sprite_bank_parse(one, sizeof one, &bank) == FIFA96_OK);
  assert(bank.count == 0 && bank.total_size == 16);
}

static void test_bank_parse_errors(void) {
  fifa96_sprite_bank bank;
  uint8_t mut[0x434];
  assert(fifa96_sprite_bank_parse(NULL, 0x434, &bank) == R_INVALID);
  assert(fifa96_sprite_bank_parse(bank_bytes, sizeof bank_bytes, NULL) == R_INVALID);
  assert(fifa96_sprite_bank_parse(bank_bytes, 15, &bank) == R_TRUNCATED);
  memcpy(mut, bank_bytes, sizeof mut);
  memcpy(mut, "XHPI", 4);
  assert(fifa96_sprite_bank_parse(mut, sizeof mut, &bank) == R_BAD_MAGIC);
  memcpy(mut, bank_bytes, sizeof mut);
  put32(mut + 4, 0x10000);
  assert(fifa96_sprite_bank_parse(mut, sizeof mut, &bank) == R_TRUNCATED);
  memcpy(mut, bank_bytes, sizeof mut);
  put32(mut + 4, 8);
  assert(fifa96_sprite_bank_parse(mut, sizeof mut, &bank) == R_TRUNCATED);
  memcpy(mut, bank_bytes, sizeof mut);
  put32(mut + 8, 0x20000000u);
  assert(fifa96_sprite_bank_parse(mut, sizeof mut, &bank) == R_TRUNCATED);
}

static void test_bank_entry(void) {
  fifa96_sprite_bank bank = parse_ok();
  fifa96_sprite_entry e;
  assert(fifa96_sprite_bank_entry(&bank, 0, &e) == FIFA96_OK);
  assert(memcmp(e.name, "p001", 4) == 0 && e.offset == 0x20);
  assert(fifa96_sprite_bank_entry(&bank, 1, &e) == FIFA96_OK);
  assert(memcmp(e.name, "p201", 4) == 0 && e.offset == 0x41C);
  assert(fifa96_sprite_bank_entry(&bank, 2, &e) == R_INVALID);
  assert(fifa96_sprite_bank_entry(&bank, 0xFFFFFFFFu, &e) == R_INVALID);
  assert(fifa96_sprite_bank_entry(NULL, 0, &e) == R_INVALID);
  assert(fifa96_sprite_bank_entry(&bank, 0, NULL) == R_INVALID);
  uint8_t mut[0x434];
  memcpy(mut, bank_bytes, sizeof mut);
  put32(mut + 20, 0x430);
  assert(fifa96_sprite_bank_parse(mut, sizeof mut, &bank) == FIFA96_OK);
  assert(fifa96_sprite_bank_entry(&bank, 0, &e) == R_TRUNCATED);
}

static void test_frame_parse(void) {
  fifa96_sprite_bank bank = parse_ok();
  fifa96_sprite_frame f;
  assert(fifa96_sprite_frame_parse(&bank, 0x20, &f) == FIFA96_OK);
  assert(f.tag == 0x7B);
  assert(f.second_offset == 0x3E4);
  assert(f.width == 20 && f.height == 49);
  assert(f.pivot_x == 10 && f.pivot_y == 46);
  assert(f.word12 == 0);
  assert(f.pixels == bank_bytes + 0x30);
  assert(f.pixel_len == 980);
  assert(fifa96_sprite_frame_parse(&bank, 0x41C, &f) == FIFA96_OK);
  assert(f.tag == 0x7B && f.second_offset == 0);
  assert(f.width == 2 && f.height == 3);
  assert(f.pivot_x == 1 && f.pivot_y == 2);
  assert(f.word12 == 0x11223344);
  assert(f.pixels == bank_bytes + 0x42C);
  assert(f.pixel_len == 6);
}

static void test_frame_parse_errors(void) {
  fifa96_sprite_bank bank = parse_ok();
  fifa96_sprite_frame f;
  uint8_t mut[0x434];
  assert(fifa96_sprite_frame_parse(NULL, 0x20, &f) == R_INVALID);
  assert(fifa96_sprite_frame_parse(&bank, 0x20, NULL) == R_INVALID);
  assert(fifa96_sprite_frame_parse(&bank, 0x425, &f) == R_TRUNCATED);
  assert(fifa96_sprite_frame_parse(&bank, 0x434, &f) == R_TRUNCATED);
  memcpy(mut, bank_bytes, sizeof mut);
  mut[0x20 + 4] = 0xFF;
  mut[0x20 + 5] = 0xFF;
  mut[0x20 + 6] = 0xFF;
  mut[0x20 + 7] = 0xFF;
  assert(fifa96_sprite_bank_parse(mut, sizeof mut, &bank) == FIFA96_OK);
  assert(fifa96_sprite_frame_parse(&bank, 0x20, &f) == R_TRUNCATED);
  memcpy(mut, bank_bytes, sizeof mut);
  mut[0x21] = 4;
  mut[0x22] = 0;
  mut[0x23] = 0;
  assert(fifa96_sprite_bank_parse(mut, sizeof mut, &bank) == FIFA96_OK);
  assert(fifa96_sprite_frame_parse(&bank, 0x20, &f) == R_TRUNCATED);
  memcpy(mut, bank_bytes, sizeof mut);
  put32(mut + 4, 0x100);
  assert(fifa96_sprite_bank_parse(mut, sizeof mut, &bank) == FIFA96_OK);
  assert(fifa96_sprite_frame_parse(&bank, 0x20, &f) == R_TRUNCATED);
}

static void test_stride(void) {
  assert(fifa96_sprite_stride(0x18, 60) == 15);
  assert(fifa96_sprite_stride(0x19, 60) == 15);
  assert(fifa96_sprite_stride(0x25, 60) == 15);
  assert(fifa96_sprite_stride(0x2D, 40) == 10);
  assert(fifa96_sprite_stride(0x22, 30) == 10);
  assert(fifa96_sprite_stride(0x5A, 30) == 10);
  assert(fifa96_sprite_stride(0x23, 30) == 15);
  assert(fifa96_sprite_stride(0x5B, 30) == 15);
  assert(fifa96_sprite_stride(0x2E, 64) == 8);
  assert(fifa96_sprite_stride(0x27, 0) == 8);
  assert(fifa96_sprite_stride(0x2A, 123) == 8);
  assert(fifa96_sprite_stride(0x2F, 0) == 5);
  assert(fifa96_sprite_stride(0x1A, 60) == 12);
  assert(fifa96_sprite_stride(0x17, 40) == 8);
  assert(fifa96_sprite_stride(0x1, 60) == 12);
  assert(fifa96_sprite_stride(0x4, 30) == 6);
  assert(fifa96_sprite_stride(0x5B, 7) == 3);
  assert(fifa96_sprite_stride(0x2E, 7) == 0);
  assert(fifa96_sprite_stride(0x3C, 0) == 0);
}

static void test_columns(void) {
  int32_t cols[16];
  assert(fifa96_sprite_columns(cols, 1, 0, 0x10000, 0, 0, 20) == FIFA96_OK);
  assert(cols[0] == 0 && cols[1] == 1);
  assert(fifa96_sprite_columns(cols, 2, 0, 0x10000, 0, 0, 20) == FIFA96_OK);
  assert(cols[0] == 0 && cols[1] == 1);
  assert(fifa96_sprite_columns(cols, 3, 0, 0x10000, 0, 0, 20) == FIFA96_OK);
  assert(cols[0] == 0 && cols[1] == 1 && cols[2] == 2 && cols[3] == 3);
  assert(fifa96_sprite_columns(cols, 4, 0, 0x18000, 0, 0, 16) == FIFA96_OK);
  assert(cols[0] == 0 && cols[1] == 1 && cols[2] == 3 && cols[3] == 4);
  assert(fifa96_sprite_columns(cols, 4, 0x18000, -0x10000, 0x30000, 0x10000, 20) == FIFA96_OK);
  assert(cols[0] == 61 && cols[1] == 80 && cols[2] == 99 && cols[3] == 118);
  assert(fifa96_sprite_columns(cols, 1, -0x8000, 0, 0, 0, 20) == FIFA96_OK);
  assert(cols[0] == -1 && cols[1] == -1);
  assert(fifa96_sprite_columns(cols, 1, 0, 0, 0, 0, 0) == FIFA96_OK);
  assert(cols[0] == 0 && cols[1] == 0);
  assert(fifa96_sprite_columns(NULL, 0, 0, 0, 0, 0, 20) == FIFA96_OK);
  assert(fifa96_sprite_columns(NULL, 1, 0, 0, 0, 0, 20) == R_INVALID);
}

static void test_span(void) {
  uint8_t dst[4] = {0xAA, 0xAA, 0xAA, 0xAA};
  const uint8_t src[4] = {1, 2, 3, 4};
  int32_t cols[4] = {3, 2, 1, 0};
  uint8_t remap[256];
  for (int i = 0; i < 256; i++) remap[i] = (uint8_t)i;
  assert(fifa96_sprite_span(dst, src, sizeof src, cols, 4, remap) == FIFA96_OK);
  assert(dst[0] == 4 && dst[1] == 3 && dst[2] == 2 && dst[3] == 1);
  dst[1] = 0xAA;
  remap[3] = 0xFF;
  assert(fifa96_sprite_span(dst, src, sizeof src, cols, 4, remap) == FIFA96_OK);
  assert(dst[0] == 4 && dst[1] == 0xAA && dst[2] == 2 && dst[3] == 1);
  for (int i = 0; i < 255; i++) remap[i] = (uint8_t)(i + 1);
  remap[255] = 0xFF;
  dst[0] = 0xAA;
  dst[1] = 0xAA;
  const uint8_t key[2] = {0, 255};
  const int32_t kcols[2] = {0, 1};
  assert(fifa96_sprite_span(dst, key, sizeof key, kcols, 2, remap) == FIFA96_OK);
  assert(dst[0] == 1 && dst[1] == 0xAA);
}

static void test_span_errors(void) {
  uint8_t dst[4] = {0xAA, 0xAA, 0xAA, 0xAA};
  const uint8_t src[4] = {1, 2, 3, 4};
  int32_t cols[4] = {3, 2, 1, 0};
  uint8_t remap[256];
  for (int i = 0; i < 256; i++) remap[i] = (uint8_t)i;
  assert(fifa96_sprite_span(NULL, src, 4, cols, 4, remap) == R_INVALID);
  assert(fifa96_sprite_span(dst, NULL, 4, cols, 4, remap) == R_INVALID);
  assert(fifa96_sprite_span(dst, src, 4, NULL, 4, remap) == R_INVALID);
  assert(fifa96_sprite_span(dst, src, 4, cols, 4, NULL) == R_INVALID);
  assert(fifa96_sprite_span(NULL, NULL, 0, NULL, 0, NULL) == FIFA96_OK);
  assert(fifa96_sprite_span(dst, src, 4, cols, 0, remap) == FIFA96_OK);
  cols[2] = 4;
  assert(fifa96_sprite_span(dst, src, 4, cols, 4, remap) == R_TRUNCATED);
  cols[2] = -1;
  assert(fifa96_sprite_span(dst, src, 4, cols, 4, remap) == R_TRUNCATED);
  cols[2] = 0;
  assert(fifa96_sprite_span(dst, src, 0, cols, 1, remap) == R_TRUNCATED);
}

int main(void) {
  build_bank();
  test_bank_parse();
  test_bank_parse_errors();
  test_bank_entry();
  test_frame_parse();
  test_frame_parse_errors();
  test_stride();
  test_columns();
  test_span();
  test_span_errors();
  puts("test_sprite: all assertions passed");
  return 0;
}
