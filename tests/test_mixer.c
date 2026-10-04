// tests/test_mixer.c — synthetic and golden coverage for the FU-37 mixer and
// pacing port.
//
// Golden fixture: tests/golden/eacs/bank-h.eacs is the real first 1SNh payload
// of /VIDEO/VID_BULL.TGV (provenance in tests/test_eacs.c). Its first frame is
// L=-32/R=-38 (e0 ff da ff), recorded as the FU-36 first-frame reference; at
// EACS volume 0x7F the PCM16 reader's SAR 7 leaves negative samples unchanged,
// so the mixed first frame is exactly L=-32/R=-38.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_eacs.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_mixer.h"
#include "fifa96_loader/fifa96_pacing.h"

static void put16le(uint8_t *p, int16_t v) {
  p[0] = (uint8_t)(uint16_t)v;
  p[1] = (uint8_t)((uint16_t)v >> 8);
}

static void put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

// Build a 32-byte EACS header at b (the caller appends the data region).
static void hdr(uint8_t *b, uint8_t f8, uint8_t f9, uint8_t f10, int8_t voice,
                int32_t loop_start, uint32_t loop_len) {
  memset(b, 0, 0x20);
  memcpy(b, "EACS", 4);
  put32le(b + 0x04, 16000);
  b[8] = f8; b[9] = f9; b[10] = f10; b[11] = (uint8_t)voice;
  put32le(b + 0x10, (uint32_t)loop_start);
  put32le(b + 0x14, loop_len);
  b[0x1D] = 0x7F;
}

// Build a 20-byte FU-39 f10==2 block header: count, L/R row codes, s16 L/R
// accumulate states; the packed data follows at +0x14.
static void delta_hdr(uint8_t *p, uint32_t count, uint32_t l_code, uint32_t r_code,
                      int16_t l_acc, int16_t r_acc) {
  put32le(p, count);
  put32le(p + 4, l_code);
  put32le(p + 8, r_code);
  put32le(p + 0xc, (uint32_t)(uint16_t)l_acc);
  put32le(p + 0x10, (uint32_t)(uint16_t)r_acc);
}

static void test_pcm16_stereo_exact(void) {
  uint8_t buf[0x20 + 16];
  hdr(buf, 2, 2, 0, 3, -1, 0);
  const int16_t data[8] = {100, -100, 200, -200, -300, 300, 0, 12345};
  for (int i = 0; i < 8; i++) put16le(buf + 0x20 + 2 * i, data[i]);
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(buf, sizeof buf, &info) == 0);
  assert(info.format == FIFA96_EACS_FMT_PCM16_STEREO);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 64, (uint64_t)1 << 32) == 0);
  assert(fifa96_mixer_voice_active(&m, 3) == 1);

  int16_t out[8] = {0};
  fifa96_mixer_render(&m, out, 4);
  /* volume 64 = half gain; SAR 7 floors negative products. */
  const int16_t want[8] = {50, -50, 100, -100, -150, 150, 0, 6172};
  for (int i = 0; i < 8; i++) assert(out[i] == want[i]);
  assert(m.voices[3].pos == ((uint64_t)4 << 32));
  assert(m.voices[3].active == 1);
  for (int v = 0; v < FIFA96_MIXER_VOICES; v++)
    if (v != 3) assert(fifa96_mixer_voice_active(&m, v) == 0);
}

static void test_pcm16_stereo_fractional_step(void) {
  uint8_t buf[0x20 + 16];
  hdr(buf, 2, 2, 0, 3, -1, 0);
  const int16_t data[8] = {100, 100, 200, 200, 300, 300, 400, 400};
  for (int i = 0; i < 8; i++) put16le(buf + 0x20 + 2 * i, data[i]);
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(buf, sizeof buf, &info) == 0);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 64, (uint64_t)1 << 31) == 0);
  int16_t out[10] = {0};
  fifa96_mixer_render(&m, out, 5);
  /* step 0.5, nearest-lower, no interpolation. */
  const int16_t want[10] = {50, 50, 50, 50, 100, 100, 100, 100, 150, 150};
  for (int i = 0; i < 10; i++) assert(out[i] == want[i]);
  assert(m.voices[3].pos == ((uint64_t)5 << 31));
}

static void test_pcm8_stereo_polarity(void) {
  uint8_t buf[0x20 + 4];
  hdr(buf, 1, 2, 0, 3, -1, 0);
  /* signed bytes, byte-interleaved L/R: 0x80 = -128, 0xFF = -1. */
  buf[0x20] = 0x80; buf[0x21] = 0x7F;
  buf[0x22] = 0x01; buf[0x23] = 0xFF;
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(buf, sizeof buf, &info) == 0);
  assert(info.format == FIFA96_EACS_FMT_PCM8_STEREO);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 64, (uint64_t)1 << 32) == 0);
  int16_t out[4] = {0};
  fifa96_mixer_render(&m, out, 2);
  /* FU-37 §A.3 volume table: 2*vol*signed8(byte). */
  const int16_t want[4] = {-16384, 16256, 128, -128};
  for (int i = 0; i < 4; i++) assert(out[i] == want[i]);
}

static void test_pcm16_mono(void) {
  uint8_t buf[0x20 + 4];
  hdr(buf, 2, 1, 0, 3, -1, 0);
  put16le(buf + 0x20, 300);
  put16le(buf + 0x22, -300);
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(buf, sizeof buf, &info) == 0);
  assert(info.format == FIFA96_EACS_FMT_PCM16_MONO);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 32, (uint64_t)1 << 32) == 0);
  int16_t out[4] = {0};
  fifa96_mixer_render(&m, out, 2);
  const int16_t want[4] = {75, 75, -75, -75}; /* duplicated to both channels */
  for (int i = 0; i < 4; i++) assert(out[i] == want[i]);
}

static void test_delta_stereo_mix(void) {
  /* Hand-computed FU-39 block: L row 1/acc 100, R row 0/acc -100, data 3A E4
   * decodes to frames (107,-103), (97,-96) (tests/test_eacs_delta.c); the
   * PCM16 reader's SAR 7 at volume 64 gives (53,-52), (48,-48). */
  uint8_t buf[0x20 + 0x14 + 2];
  hdr(buf, 2, 2, 2, 3, -1, 0);
  delta_hdr(buf + 0x20, 2, 1, 0, 100, -100);
  buf[0x34] = 0x3A;
  buf[0x35] = 0xE4;
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(buf, sizeof buf, &info) == 0);
  assert(info.format == FIFA96_EACS_FMT_DELTA_STEREO && info.delta_units == 2);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 64, (uint64_t)1 << 32) == 0);
  int16_t out[4] = {0};
  fifa96_mixer_render(&m, out, 2);
  const int16_t want[4] = {53, -52, 48, -48};
  for (int i = 0; i < 4; i++) assert(out[i] == want[i]);
  assert(m.voices[3].delta == 1 && m.voices[3].delta_pos == 2);
  assert(m.voices[3].delta_state.l_row == 384 && m.voices[3].delta_state.r_row == 128);
  assert(m.voices[3].pos == ((uint64_t)2 << 32));
  assert(fifa96_mixer_voice_active(&m, 3) == 1);
  int16_t stop[2] = {1, 1};
  fifa96_mixer_render(&m, stop, 1);
  assert(stop[0] == 0 && stop[1] == 0);
  assert(fifa96_mixer_voice_active(&m, 3) == 0);
}

static void test_delta_mono_mix(void) {
  /* FU-39 §2.2 bank arm: no block header, zero state, declared +0x0C count is
   * the nibble count. Data 3A E4 -> nibbles 3, A, E from row 0:
   *   3 -> +4 = 4; A -> 4 + DELTA[0][10] = 1; E -> 1 + DELTA[0][14] = -9.
   * The mono sample is duplicated to both lanes; volume 64 => SAR 7. */
  uint8_t buf[0x20 + 2];
  hdr(buf, 2, 1, 2, 3, -1, 0);
  put32le(buf + 0x0C, 3);
  buf[0x20] = 0x3A;
  buf[0x21] = 0xE4;
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(buf, sizeof buf, &info) == 0);
  assert(info.format == FIFA96_EACS_FMT_DELTA_MONO && info.delta_units == 3);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 64, (uint64_t)1 << 32) == 0);
  int16_t out[6] = {0};
  fifa96_mixer_render(&m, out, 3);
  const int16_t want[6] = {2, 2, 0, 0, -5, -5};
  for (int i = 0; i < 6; i++) assert(out[i] == want[i]);
}

static void test_delta_stereo_bank_mix(void) {
  /* FU-43 §2 bank stereo: voice -1 removes the 20-byte block header, so the
   * declared +0x0C count is one packed (L,R) frame per byte at data_off,
   * decoded from zero state (unsigned producer 0xB8610). Data 12 34 56:
   *   unit 0: L nibble 1 -> +1 = 1; R nibble 2 -> +3 = 3
   *   unit 1: L nibble 3 -> +4 = 5; R nibble 4 -> +7 = 10 (R row -> 128)
   *   unit 2: L nibble 5 -> +8 = 13; R nibble 6 at row 128/4+6 -> +14 = 24
   * At volume 64 the PCM16 reader's SAR 7 gives (0,1),(2,5),(6,12). */
  uint8_t buf[0x20 + 3];
  hdr(buf, 2, 2, 2, -1, 0, 0);
  put32le(buf + 0x0C, 3);
  buf[0x20] = 0x12; buf[0x21] = 0x34; buf[0x22] = 0x56;
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(buf, sizeof buf, &info) == 0);
  assert(info.format == FIFA96_EACS_FMT_DELTA_STEREO && info.voice == -1);
  assert(info.delta_units == 3);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 64, (uint64_t)1 << 32) == 0);
  assert(m.voices[3].delta_hdr == NULL && m.voices[3].f9 == 2);
  assert(m.voices[3].delta_units == 3 && m.voices[3].units == 3);
  int16_t out[6] = {0};
  fifa96_mixer_render(&m, out, 3);
  const int16_t want[6] = {0, 1, 2, 5, 6, 12};
  for (int i = 0; i < 6; i++) assert(out[i] == want[i]);
  assert(m.voices[3].delta_pos == 3);
  assert(m.voices[3].delta_state.l_row == 256 && m.voices[3].delta_state.r_row == 512);
  assert(m.voices[3].delta_state.l_acc == 13 && m.voices[3].delta_state.r_acc == 24);
  int16_t stop[2] = {1, 1};
  fifa96_mixer_render(&m, stop, 1);
  assert(stop[0] == 0 && stop[1] == 0);
  assert(fifa96_mixer_voice_active(&m, 3) == 0);

  /* declared frames past the payload bytes are rejected. */
  struct fifa96_eacs_info bad = info;
  bad.delta_units = 4;
  assert(fifa96_mixer_start(&m, 3, &bad, buf, sizeof buf, 64, (uint64_t)1 << 32) ==
         -(int)FIFA96_ERR_TRUNCATED);
}

static void test_clamp_sum(void) {
  /* Two full-scale voices on top of each other: 32-bit accumulate, then the
   * 0xB9E53 converter clamps to +/-32767 (not -32768). */
  uint8_t hi[0x20 + 4], lo[0x20 + 4];
  hdr(hi, 2, 2, 0, 0, -1, 0);
  put16le(hi + 0x20, 32767);
  put16le(hi + 0x22, 32767);
  hdr(lo, 2, 2, 0, 1, -1, 0);
  put16le(lo + 0x20, (int16_t)-32768);
  put16le(lo + 0x22, (int16_t)-32768);
  struct fifa96_eacs_info ih, il;
  assert(fifa96_eacs_parse(hi, sizeof hi, &ih) == 0);
  assert(fifa96_eacs_parse(lo, sizeof lo, &il) == 0);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, 0, &ih, hi, sizeof hi, 127, (uint64_t)1 << 32) == 0);
  assert(fifa96_mixer_start(&m, 1, &ih, hi, sizeof hi, 127, (uint64_t)1 << 32) == 0);
  int16_t out[2] = {0};
  fifa96_mixer_render(&m, out, 1);
  /* 2 * (32767*127 >> 7) = 65022 -> +32767; 2 * (-32768*64 >> 7) = -32768
   * -> the converter's lower bound is -32767. */
  assert(out[0] == 32767 && out[1] == 32767);

  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, 0, &il, lo, sizeof lo, 64, (uint64_t)1 << 32) == 0);
  assert(fifa96_mixer_start(&m, 1, &il, lo, sizeof lo, 64, (uint64_t)1 << 32) == 0);
  fifa96_mixer_render(&m, out, 1);
  assert(out[0] == -32767 && out[1] == -32767);
}

static void test_end_of_data_silence(void) {
  uint8_t buf[0x20 + 4];
  hdr(buf, 2, 1, 0, 3, -1, 0);
  put16le(buf + 0x20, 777);
  put16le(buf + 0x22, -777);
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(buf, sizeof buf, &info) == 0);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 127, (uint64_t)1 << 32) == 0);
  int16_t out[8] = {1, 1, 1, 1, 1, 1, 1, 1};
  fifa96_mixer_render(&m, out, 4);
  const int16_t want[8] = {770, 770, -771, -771, 0, 0, 0, 0};
  for (int i = 0; i < 8; i++) assert(out[i] == want[i]);
  assert(fifa96_mixer_voice_active(&m, 3) == 0);
}

static void test_loop_wrap(void) {
  uint8_t buf[0x20 + 8];
  hdr(buf, 2, 1, 0, 3, 1, 2); /* loop over units 1..2 (loop_end 3) */
  put16le(buf + 0x20, 1000);
  put16le(buf + 0x22, 2000);
  put16le(buf + 0x24, 3000);
  put16le(buf + 0x26, 4000);
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(buf, sizeof buf, &info) == 0);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 64, (uint64_t)1 << 32) == 0);
  assert(m.voices[3].loop == 1 && m.voices[3].loop_start == 1 && m.voices[3].loop_end == 3);
  int16_t out[12] = {0};
  fifa96_mixer_render(&m, out, 6);
  /* units 0,1,2 then wrap: 1,2, then wrap: 1. */
  const int16_t want[12] = {500, 500, 1000, 1000, 1500, 1500,
                            1000, 1000, 1500, 1500, 1000, 1000};
  for (int i = 0; i < 12; i++) assert(out[i] == want[i]);
  assert(m.voices[3].active == 1);
}

static void test_loop_window_bounds(void) {
  /* A window with loop_end past the data (or a bank -1 loop start) is not
   * armed: the static port must stay inside its buffer. */
  uint8_t over[0x20 + 8], neg[0x20 + 8];
  hdr(over, 2, 1, 0, 3, 3, 5); /* 4 units, window [3, 8) */
  hdr(neg, 2, 1, 0, 3, -1, 2); /* bank header form: start -1 */
  struct fifa96_eacs_info io, in;
  assert(fifa96_eacs_parse(over, sizeof over, &io) == 0);
  assert(fifa96_eacs_parse(neg, sizeof neg, &in) == 0);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, 3, &io, over, sizeof over, 0x7F, (uint64_t)1 << 32) == 0);
  assert(m.voices[3].loop == 0);
  fifa96_mixer_stop(&m, 3);
  assert(fifa96_mixer_start(&m, 3, &in, neg, sizeof neg, 0x7F, (uint64_t)1 << 32) == 0);
  assert(m.voices[3].loop == 0);
}

static void test_rate_step(void) {
  /* FU-37 §A.5 tail: v = (rate*ratio)>>shift; step = (v/out_rate) & 0xFF
   * in the integer word, remainder scaled by 2^32/out_rate. */
  static const struct {
    uint32_t rate, out_rate, ratio, shift;
    uint64_t want;
  } cases[] = {
      {16000, 22050, 0x10000, 16, 3116529557ull},   /* table[0] unity */
      {48000, 22050, 0x10000, 16, 9349588671ull},   /* integer part 2 */
      {16000, 22050, 0x10C1B, 16, 3263785578ull},   /* observed table[80] */
      {65535, 100, 0x10000, 16, 615683561881ull},   /* step_int 655 masked to 143 */
      {22050, 22050, 0x10000, 16, 1ull << 32},      /* unity */
      {0, 22050, 0x10000, 16, 0},                   /* zero rate -> zero step */
  };
  for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
    uint64_t st = 0xDEADBEEFull;
    assert(fifa96_mixer_step_from_rate(cases[i].rate, cases[i].out_rate,
                                       cases[i].ratio, cases[i].shift, &st) == 0);
    assert(st == cases[i].want);
  }
  uint64_t st = 0;
  assert(fifa96_mixer_step_from_rate(16000, 22050, 0x10000, 16, NULL) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_step_from_rate(16000, 0, 0x10000, 16, &st) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_step_from_rate(16000, 22050, 0x10000, 64, &st) ==
         -(int)FIFA96_ERR_TRUNCATED);
}

static void test_start_stop_errors(void) {
  uint8_t buf[0x20 + 16];
  hdr(buf, 2, 2, 0, 3, -1, 0);
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(buf, sizeof buf, &info) == 0);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(NULL, 3, &info, buf, sizeof buf, 0x7F, 1) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_start(&m, 3, NULL, buf, sizeof buf, 0x7F, 1) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_start(&m, 3, &info, NULL, sizeof buf, 0x7F, 1) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_start(&m, -1, &info, buf, sizeof buf, 0x7F, 1) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_start(&m, 16, &info, buf, sizeof buf, 0x7F, 1) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 0x80, 1) ==
         -(int)FIFA96_ERR_TRUNCATED);
  /* info says 16 data bytes; a payload that stops at the header is malformed. */
  assert(fifa96_mixer_start(&m, 3, &info, buf, 0x20, 0x7F, 1) ==
         -(int)FIFA96_ERR_TRUNCATED);

  /* f10 == 2 with an unproven f8/f9 pair has no decoder (FU-39 §1). */
  uint8_t f10buf[0x20 + 16];
  hdr(f10buf, 3, 1, 2, 3, -1, 0);
  struct fifa96_eacs_info f10info;
  assert(fifa96_eacs_parse(f10buf, sizeof f10buf, &f10info) == 0);
  assert(f10info.format == FIFA96_EACS_FMT_UNKNOWN);
  assert(fifa96_mixer_start(&m, 3, &f10info, f10buf, sizeof f10buf, 0x7F, 1) ==
         -(int)FIFA96_ERR_UNSUPPORTED);

  /* A hand-built delta info must still carry a full 20-byte block header. */
  uint8_t shortbuf[0x20 + 0x10];
  memset(shortbuf, 0, sizeof shortbuf);
  memcpy(shortbuf, "EACS", 4);
  struct fifa96_eacs_info dinfo;
  memset(&dinfo, 0, sizeof dinfo);
  dinfo.f8 = 2; dinfo.f9 = 2; dinfo.f10 = 2;
  dinfo.format = FIFA96_EACS_FMT_DELTA_STEREO;
  dinfo.data_off = 0x20; dinfo.data_len = 0x10; dinfo.block_size = 4; dinfo.blocks = 4;
  assert(fifa96_mixer_start(&m, 3, &dinfo, shortbuf, sizeof shortbuf, 0x7F, 1) ==
         -(int)FIFA96_ERR_TRUNCATED);

  /* unproven f8/f9 combination (format UNKNOWN). */
  uint8_t unkbuf[0x20 + 16];
  hdr(unkbuf, 3, 1, 0, 3, -1, 0);
  struct fifa96_eacs_info unkinfo;
  assert(fifa96_eacs_parse(unkbuf, sizeof unkbuf, &unkinfo) == 0);
  assert(unkinfo.format == FIFA96_EACS_FMT_UNKNOWN);
  assert(fifa96_mixer_start(&m, 3, &unkinfo, unkbuf, sizeof unkbuf, 0x7F, 1) ==
         -(int)FIFA96_ERR_UNSUPPORTED);

  /* valid arm, out-of-range queries, stop, then silence. */
  assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 0x7F, (uint64_t)1 << 32) == 0);
  assert(fifa96_mixer_voice_active(&m, 3) == 1);
  assert(fifa96_mixer_voice_active(&m, -1) == 0);
  assert(fifa96_mixer_voice_active(&m, 16) == 0);
  assert(fifa96_mixer_voice_active(NULL, 3) == 0);
  fifa96_mixer_stop(&m, 5); /* no-op on an inactive voice */
  assert(fifa96_mixer_voice_active(&m, 3) == 1);
  fifa96_mixer_stop(&m, 3);
  assert(fifa96_mixer_voice_active(&m, 3) == 0);
  int16_t out[4] = {1, 1, 1, 1};
  fifa96_mixer_render(&m, out, 2);
  for (int i = 0; i < 4; i++) assert(out[i] == 0);
}

static void test_golden_bank_h_first_frames(void) {
  uint8_t *h = NULL;
  size_t hn = 0;
  assert(fifa96_file_read("tests/golden/eacs/bank-h.eacs", &h, &hn) == 0);
  assert(hn == 0x1080);
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(h, hn, &info) == 0);
  assert(info.format == FIFA96_EACS_FMT_PCM16_STEREO && info.blocks == 1048);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, info.voice, &info, h, hn, 0x7F, (uint64_t)1 << 32) == 0);
  int16_t out[8] = {0};
  fifa96_mixer_render(&m, out, 4);
  /* FU-36 first-frame reference: raw e0 ff da ff scaled by 0x7F via SAR 7. */
  const int16_t want[8] = {-32, -38, -32, -40, -35, -42, -36, -41};
  for (int i = 0; i < 8; i++) assert(out[i] == want[i]);

  /* 300 frames of the 1048-unit header chunk: 1:1 advancement. */
  int16_t rest[600];
  fifa96_mixer_render(&m, rest, 296);
  assert(m.voices[info.voice].pos == ((uint64_t)300 << 32));
  assert(fifa96_mixer_voice_active(&m, info.voice) == 1);

  /* drain the remaining 748 units, then the voice stops at the data end. */
  int16_t tail[1496];
  fifa96_mixer_render(&m, tail, 748);
  assert(m.voices[info.voice].pos == ((uint64_t)1048 << 32));
  assert(fifa96_mixer_voice_active(&m, info.voice) == 1);
  int16_t stop[4] = {1, 1, 1, 1};
  fifa96_mixer_render(&m, stop, 2);
  for (int i = 0; i < 4; i++) assert(stop[i] == 0);
  assert(fifa96_mixer_voice_active(&m, info.voice) == 0);
  free(h);
}

static void test_golden_bank_h_loop(void) {
  uint8_t *h = NULL;
  size_t hn = 0;
  assert(fifa96_file_read("tests/golden/eacs/bank-h.eacs", &h, &hn) == 0);
  /* Video headers force loop start -1/length 0; arm a 4-unit window on the
   * real data to exercise the wrap on golden samples. */
  put32le(h + 0x10, 0);
  put32le(h + 0x14, 4);
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(h, hn, &info) == 0);
  assert(info.loop_start == 0 && info.loop_len == 4);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, info.voice, &info, h, hn, 0x7F, (uint64_t)1 << 32) == 0);
  int16_t out[12] = {0};
  fifa96_mixer_render(&m, out, 6);
  /* frames 4 and 5 are frames 0 and 1 again. */
  const int16_t want[4] = {-32, -38, -32, -40};
  for (int i = 0; i < 4; i++) {
    assert(out[8 + i] == want[i]);
    assert(out[8 + i] == out[i]);
  }
  free(h);
}

static void test_golden_delta_mix(void) {
  uint8_t *h = NULL, *d = NULL;
  size_t hn = 0, dn = 0;
  assert(fifa96_file_read("tests/golden/eacs/vid-game-h.eacs", &h, &hn) == 0);
  assert(fifa96_file_read("tests/golden/eacs/vid-game-d0.eacs", &d, &dn) == 0);
  assert(hn == 0x45C && dn == 0x43C);
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(h, hn, &info) == 0);
  assert(info.format == FIFA96_EACS_FMT_DELTA_STEREO && info.delta_units == 1064);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, info.voice, &info, h, hn, 64, (uint64_t)1 << 32) == 0);
  int16_t out[16] = {0};
  fifa96_mixer_render(&m, out, 8);
  /* Decoded (11,11),(41,41),(104,104),(240,240),(533,533),(407,1164),
   * (-167,2521),(-1400,5431) at volume 64 (x64 >> 7), hand-checked on the
   * first bytes 77 77 77 77 77 97 f7 f7. */
  const int16_t want[16] = {5, 5, 20, 20, 52, 52, 120, 120,
                            266, 266, 203, 582, -84, 1260, -700, 2715};
  for (int i = 0; i < 16; i++) assert(out[i] == want[i]);

  int16_t rest[2 * (1064 - 8)];
  fifa96_mixer_render(&m, rest, 1064 - 8);
  assert(m.voices[info.voice].pos == ((uint64_t)1064 << 32));
  /* FU-39 §6 continuity: the voice's carried row/acc state equals the stored
   * header of the next 1SNd chunk (the engine re-inits from it per dequeue). */
  struct fifa96_eacs_delta st0;
  uint32_t count = 0;
  assert(fifa96_eacs_delta_header(d, dn, &st0, &count) == 0);
  assert(count == 1064);
  assert(m.voices[info.voice].delta_state.l_row == st0.l_row);
  assert(m.voices[info.voice].delta_state.r_row == st0.r_row);
  assert(m.voices[info.voice].delta_state.l_acc == st0.l_acc);
  assert(m.voices[info.voice].delta_state.r_acc == st0.r_acc);

  int16_t stop[2] = {1, 1};
  fifa96_mixer_render(&m, stop, 1);
  assert(stop[0] == 0 && stop[1] == 0);
  assert(fifa96_mixer_voice_active(&m, info.voice) == 0);
  free(h);
  free(d);
}

static void test_golden_delta_second_chunk(void) {
  /* The raw 1SNd payload wrapped in the 32-byte EACS header the video parser
   * would have produced: the embedded block header carries the state from the
   * previous chunk, so arming at the second chunk continues the stream (the
   * port has no queue; FU-39 §7 leg 1 covers runtime staging). */
  uint8_t *d = NULL;
  size_t dn = 0;
  assert(fifa96_file_read("tests/golden/eacs/vid-game-d0.eacs", &d, &dn) == 0);
  assert(dn == 0x43C);
  uint8_t *buf = malloc(0x20 + dn);
  assert(buf);
  hdr(buf, 2, 2, 2, 0, -1, 0);
  memcpy(buf + 0x20, d, dn);
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(buf, 0x20 + dn, &info) == 0);
  assert(info.format == FIFA96_EACS_FMT_DELTA_STEREO && info.delta_units == 1064);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, info.voice, &info, buf, 0x20 + dn, 64, (uint64_t)1 << 32) == 0);
  int16_t out[8] = {0};
  fifa96_mixer_render(&m, out, 4);
  /* decoded (26350,2327),(25742,3747),(22975,6071),(19453,8882) at volume 64 */
  const int16_t want[8] = {13175, 1163, 12871, 1873, 11487, 3035, 9726, 4441};
  for (int i = 0; i < 8; i++) assert(out[i] == want[i]);
  free(buf);
  free(d);
}

static void test_pacing_frames_due(void) {
  /* FU-37 §B.3: progress = clock()*15/100 at 100 Hz (vgt_stream_poll). */
  static const struct {
    uint32_t ticks, want;
  } cases[] = {
      {0, 0},   {1, 0},   {6, 0},    {7, 1},    {13, 1},   {14, 2},
      {15, 2},  {20, 3},  {99, 14},  {100, 15}, {101, 15}, {999, 149},
      {1000, 150}, {200000000, 30000000},
  };
  for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++)
    assert(fifa96_pacing_frames_due(cases[i].ticks) == cases[i].want);
}

static void test_pacing_catch_up(void) {
  /* FU-37 §B.3 step 5: bound = frames before the increment + 2; at or past
   * the bound the next frame is decoded immediately. */
  assert(fifa96_pacing_catch_up(0, 1) == 0);
  assert(fifa96_pacing_catch_up(0, 2) == 1);
  assert(fifa96_pacing_catch_up(0, 3) == 1);
  assert(fifa96_pacing_catch_up(5, 6) == 0);
  assert(fifa96_pacing_catch_up(5, 7) == 1);
  assert(fifa96_pacing_catch_up(5, 8) == 1);
}

int main(void) {
  test_pcm16_stereo_exact();
  test_pcm16_stereo_fractional_step();
  test_pcm8_stereo_polarity();
  test_pcm16_mono();
  test_delta_stereo_mix();
  test_delta_mono_mix();
  test_delta_stereo_bank_mix();
  test_clamp_sum();
  test_end_of_data_silence();
  test_loop_wrap();
  test_loop_window_bounds();
  test_rate_step();
  test_start_stop_errors();
  test_golden_bank_h_first_frames();
  test_golden_bank_h_loop();
  test_golden_delta_mix();
  test_golden_delta_second_chunk();
  test_pacing_frames_due();
  test_pacing_catch_up();
  printf("test_mixer OK\n");
  return 0;
}
