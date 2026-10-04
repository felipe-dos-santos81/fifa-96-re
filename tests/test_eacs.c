// tests/test_eacs.c — golden parse of the real VID_BULL.TGV EACS chunks plus
// bounds coverage for the EACS header parser (FU-35).
//
// Fixture provenance (extracted 2026-10-04 from the read-only
// game/FIFAPCCD96.iso with tools/fifa96_bind.iso_files):
//   /VIDEO/VID_BULL.TGV starts at ISO offset 0xFE03800. Chunk 0 is kVGT
//   (len 0x169C); chunk 1 is the only 1SNh, at file offset 0x169C with total
//   length 0x1088. tests/golden/eacs/bank-h.eacs is its 0x1080-byte payload
//   (after the 8-byte [tag][len] chunk header): the 32-byte EACS header plus
//   its own 0x1060-byte PCM data region, and it starts with "EACS". Chunk 3
//   is the first 1SNd at 0x3DC0, total length 0x1068;
//   tests/golden/eacs/bank-d.eacs is its 0x1060-byte payload — raw PCM bytes,
//   not an EACS header (parse returns FIFA96_ERR_BAD_MAGIC; the test asserts
//   that and the 16-bit LE interleaving directly).
//   The task brief's "first 1SNh chunk is 0x465 bytes" does not match
//   VID_BULL: the 0x46x-length headers are in VID_CRED/VID_INTR/VID_DEEP/
//   VID_NGEN. The committed fixture is the verified real first 1SNh.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_eacs.h"

static void put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

// Build a 32-byte EACS header at b (the caller appends any data).
static void hdr(uint8_t *b, uint32_t rate, uint8_t f8, uint8_t f9, uint8_t f10, int8_t voice,
                uint32_t count, int32_t loop_start, uint32_t loop_len, uint32_t data_ptr, uint8_t vol) {
  memset(b, 0, 0x20);
  memcpy(b, "EACS", 4);
  put32le(b + 4, rate);
  b[8] = f8; b[9] = f9; b[10] = f10; b[11] = (uint8_t)voice;
  put32le(b + 0x0C, count);
  put32le(b + 0x10, (uint32_t)loop_start);
  put32le(b + 0x14, loop_len);
  put32le(b + 0x18, data_ptr);
  b[0x1D] = vol;
}

static int16_t le16(const uint8_t *p) {
  return (int16_t)(uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

int main(void) {
  uint8_t *h = 0, *d = 0;
  size_t hn = 0, dn = 0;
  assert(fifa96_file_read("tests/golden/eacs/bank-h.eacs", &h, &hn) == 0);
  assert(fifa96_file_read("tests/golden/eacs/bank-d.eacs", &d, &dn) == 0);
  assert(hn == 0x1080);
  assert(dn == 0x1060);
  assert(memcmp(h, "EACS", 4) == 0);

  /* ---- golden 1SNh header: every FU-35 field and the derived layout ---- */
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(h, hn, &info) == 0);
  assert(info.rate == 16000);
  assert(info.f8 == 2 && info.f9 == 2 && info.f10 == 0);
  assert(info.voice == 7);
  assert(info.count == 44032);          /* declared whole-cue sample count, on disk */
  assert(info.loop_start == 0 && info.loop_len == 0);
  assert(info.volume == 0x7F);
  assert(info.data_ptr == 0);           /* on-disk +0x18: unset in video chunks */
  assert(info.data_off == 0x20);        /* parser form: payload+0x20 */
  assert(info.data_len == 0x1060);      /* hn - 0x20 */
  assert(info.block_size == 4);         /* f8*f9 */
  assert(info.blocks == 1048);          /* data_len / block_size */
  assert(info.samples == 1048);         /* f10 != 2 => one sample per block */
  assert(info.format == FIFA96_EACS_FMT_PCM16_STEREO);
  /* The header chunk carries PCM of its own (FU-35 §5: 1048 blocks). */
  assert(info.data_len == dn);          /* same size as each 1SNd payload */
  assert(le16(h + info.data_off) == -32);      /* first L: e0 ff */
  assert(le16(h + info.data_off + 2) == -38);  /* first R: da ff */

  /* ---- golden 1SNd payload is raw PCM, not an EACS header ---- */
  assert(memcmp(d, "EACS", 4) != 0);
  assert(fifa96_eacs_parse(d, dn, &info) == -(int)FIFA96_ERR_BAD_MAGIC);
  assert(le16(d) == 439);                       /* b7 01: signed 16-bit LE mono frame L */
  assert(le16(d + 2) == 481);                   /* e1 01: R */

  /* ---- NULL args ---- */
  assert(fifa96_eacs_parse(NULL, hn, &info) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_parse(h, hn, NULL) == -(int)FIFA96_ERR_TRUNCATED);

  /* ---- short header (32 bytes required) ---- */
  assert(fifa96_eacs_parse(h, 0, &info) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_parse(h, 0x1F, &info) == -(int)FIFA96_ERR_TRUNCATED);

  /* header with no data region is valid: zero blocks/samples. */
  assert(fifa96_eacs_parse(h, 0x20, &info) == 0);
  assert(info.data_off == 0x20 && info.data_len == 0);
  assert(info.blocks == 0 && info.samples == 0);
  assert(info.format == FIFA96_EACS_FMT_PCM16_STEREO);

  /* ---- malformed header fields (negative block size would divide by zero) ---- */
  uint8_t b[0x40];
  memcpy(b, h, sizeof b);
  b[8] = 0;                                    /* f8 = 0 => block_size 0 */
  assert(fifa96_eacs_parse(b, sizeof b, &info) == -(int)FIFA96_ERR_TRUNCATED);
  memcpy(b, h, sizeof b);
  b[9] = 0;                                    /* f9 = 0 => block_size 0 */
  assert(fifa96_eacs_parse(b, sizeof b, &info) == -(int)FIFA96_ERR_TRUNCATED);
  memcpy(b, h, sizeof b);
  b[11] = 16;                                  /* voice > 15 */
  assert(fifa96_eacs_parse(b, sizeof b, &info) == -(int)FIFA96_ERR_TRUNCATED);
  memcpy(b, h, sizeof b);
  b[11] = 0xFF;                                /* voice -1 (bank form; separate loader) */
  assert(fifa96_eacs_parse(b, sizeof b, &info) == -(int)FIFA96_ERR_TRUNCATED);

  /* ---- data pointer / length bounds ---- */
  memcpy(b, h, sizeof b);
  put32le(b + 0x18, 0x10);                     /* points into the 32-byte header */
  assert(fifa96_eacs_parse(b, sizeof b, &info) == -(int)FIFA96_ERR_TRUNCATED);
  memcpy(b, h, sizeof b);
  put32le(b + 0x18, sizeof b + 1u);            /* past src_len */
  assert(fifa96_eacs_parse(b, sizeof b, &info) == -(int)FIFA96_ERR_TRUNCATED);
  memcpy(b, h, sizeof b);
  put32le(b + 0x18, sizeof b);                 /* exactly at src_len: empty data ok */
  assert(fifa96_eacs_parse(b, sizeof b, &info) == 0);
  assert(info.data_off == sizeof b && info.data_len == 0 && info.blocks == 0);

  /* nonzero data pointer (bank .spc form, FU-35 §2/§6.5) is honored. */
  uint8_t bank[0x30];
  memset(bank, 0, sizeof bank);
  hdr(bank, 16000, 2, 1, 2, 0, 0, 0, 0, 0x28, 0x7F);
  assert(fifa96_eacs_parse(bank, sizeof bank, &info) == 0);
  assert(info.data_ptr == 0x28 && info.data_off == 0x28 && info.data_len == 8);
  assert(info.block_size == 2 && info.blocks == 4 && info.samples == 16);
  assert(info.format == FIFA96_EACS_FMT_DELTA_MONO);  /* FU-39: f10=2 nibble path */
  assert(info.delta_units == 0);                      /* block header count is 0 */

  /* declared count is not trusted: the original overwrites it with the block
   * count (FU-35 §2), so a maximal declared count still parses. */
  memcpy(b, h, sizeof b);
  put32le(b + 0x0C, 0xFFFFFFFFu);
  assert(fifa96_eacs_parse(b, sizeof b, &info) == 0);
  assert(info.count == 0xFFFFFFFFu);
  assert(info.blocks == (0x40 - 0x20) / 4 && info.samples == info.blocks);

  /* ---- format enum coverage ---- */
  hdr(b, 16000, 1, 2, 0, 0, 0, 0, 0, 0, 0x7F);   /* 8-bit byte-interleaved stereo */
  assert(fifa96_eacs_parse(b, 0x20 + 16, &info) == 0);
  assert(info.block_size == 2 && info.blocks == 8 && info.samples == 8);
  assert(info.format == FIFA96_EACS_FMT_PCM8_STEREO);

  hdr(b, 16000, 3, 1, 0, 0, 0, 0, 0, 0, 0x7F);   /* unproven combination */
  assert(fifa96_eacs_parse(b, 0x20 + 9, &info) == 0);
  assert(info.block_size == 3 && info.blocks == 3 && info.samples == 3);
  assert(info.format == FIFA96_EACS_FMT_UNKNOWN);

  /* ---- f10==2 adaptive-delta block framing (FU-39 §2.1/§4.3) ---- */
  /* stereo: the 20-byte block header's count is one byte per decoder unit */
  hdr(b, 16000, 2, 2, 2, 0, 0, 0, 0, 0, 0x7F);
  put32le(b + 0x20, 3);
  assert(fifa96_eacs_parse(b, 0x20 + 0x14 + 3, &info) == 0);
  assert(info.format == FIFA96_EACS_FMT_DELTA_STEREO);
  assert(info.delta_units == 3);
  assert(info.block_size == 4 && info.blocks == (0x14 + 3) / 4); /* FU-35 nominal */

  /* mono (bank form, FU-39 §2.2): the unsigned producer skips the block
   * header, so the declared +0x0C count is the nibble count at data_off;
   * two nibbles per byte with trailing slack allowed (FU-39 §6). */
  hdr(b, 16000, 2, 1, 2, 0, 3, 0, 0, 0, 0x7F);
  assert(fifa96_eacs_parse(b, 0x20 + 2, &info) == 0);
  assert(info.format == FIFA96_EACS_FMT_DELTA_MONO && info.delta_units == 3);
  assert(fifa96_eacs_parse(b, 0x20 + 4, &info) == 0);   /* slack is fine */
  assert(info.delta_units == 3);
  put32le(b + 0x0C, 5);                        /* 5 nibbles need 3 bytes */
  assert(fifa96_eacs_parse(b, 0x20 + 2, &info) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_parse(b, 0x20 + 3, &info) == 0);

  /* truncated block header and block-count overrun (FU-39 §2.1) */
  hdr(b, 16000, 2, 2, 2, 0, 0, 0, 0, 0, 0x7F);
  assert(fifa96_eacs_parse(b, 0x20 + 0x13, &info) == -(int)FIFA96_ERR_TRUNCATED);
  put32le(b + 0x20, 4);
  assert(fifa96_eacs_parse(b, 0x20 + 0x14 + 3, &info) == -(int)FIFA96_ERR_TRUNCATED);
  put32le(b + 0x20, 3);
  assert(fifa96_eacs_parse(b, 0x20 + 0x14 + 3, &info) == 0);
  assert(info.format == FIFA96_EACS_FMT_DELTA_STEREO && info.delta_units == 3);

  /* block count floors, like the original's DIV. */
  hdr(b, 16000, 2, 2, 0, 0, 0, 0, 0, 0, 0x7F);
  assert(fifa96_eacs_parse(b, 0x20 + 6, &info) == 0);
  assert(info.blocks == 1 && info.samples == 1);

  free(h);
  free(d);
  printf("test_eacs OK\n");
  return 0;
}
