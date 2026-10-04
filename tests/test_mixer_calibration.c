// tests/test_mixer_calibration.c — FU-46 mixer calibration: FUN_000b86C8
// pitch -> 32.32 step (head + table 0xB6B88 + tail) and FUN_000a662c
// pan/gain -> per-channel L/R gains.
//
// Every expected value is derived from the image bytes (table[0] = 0x10000
// unity, table[80] = 0x10C1B, table[1199] = 0x1FFB4) and the cited
// instruction stream; the throwaway extraction/hand-computation lives in the
// FU-46 report. Spec: docs/ghidra/FU46_mixer_calibration.md.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_eacs.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_mixer.h"

static void put16le(uint8_t *p, int16_t v) {
  p[0] = (uint8_t)(uint16_t)v;
  p[1] = (uint8_t)((uint16_t)v >> 8);
}

// 32-byte EACS header (same shape as tests/test_mixer.c).
static void hdr(uint8_t *b, uint8_t f8, uint8_t f9, uint8_t f10, int8_t voice) {
  memset(b, 0, 0x20);
  memcpy(b, "EACS", 4);
  b[8] = f8; b[9] = f9; b[10] = f10; b[11] = (uint8_t)voice;
  b[0x1D] = 0x7F;
}

static void test_pitch_ratio_table(void) {
  /* FUN_000b86C8 head 0xB86C9..0xB86E6: E = pitch+0x2000; W walks down from
   * 0x40D0 by 0x4B0 with CL from 9; index = E-W into the 1200-entry table.
   * Unity pitch 0 -> table[0] 0x10000 shift 16; +/-1200 cents step the shift
   * by one (octaves); the window base is 8400. */
  static const struct {
    int32_t pitch;
    uint32_t ratio, shift;
  } cases[] = {
      {0, 0x10000u, 16},      {1, 0x10025u, 16},      {80, 0x10C1Bu, 16},
      {1199, 0x1FFB4u, 16},   {1200, 0x10000u, 15},   {-1, 0x1FFB4u, 17},
      {-1200, 0x10000u, 17},  {8399, 0x1FFB4u, 10},   {8400, 0x10000u, 9},
      {-18000, 0x10000u, 31},
  };
  for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
    uint32_t ratio = 0xDEADBEEFu, shift = 0xDEADBEEFu;
    assert(fifa96_mixer_pitch_ratio(cases[i].pitch, &ratio, &shift) == 0);
    assert(ratio == cases[i].ratio && shift == cases[i].shift);
  }
  /* 9600 goes one entry past the table; -18001 needs CL 32 (the original's
   * SHRD would wrap CL mod 32, the port rejects). */
  uint32_t r = 0, s = 0;
  assert(fifa96_mixer_pitch_ratio(9600, &r, &s) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_pitch_ratio(-18001, &r, &s) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_pitch_ratio(0, NULL, &s) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_pitch_ratio(0, &r, NULL) == -(int)FIFA96_ERR_TRUNCATED);
}

static void test_step_from_pitch(void) {
  /* Head + the FUN_000b86C8 tail: v = (rate*ratio)>>CL; step_int =
   * (v/out)&0xFF; frac = ((rem<<24)/out)<<8 (the 8.24 fraction scaled into
   * the 32.32 low word); step = step_int<<32 | frac. */
  static const struct {
    uint32_t rate, out_rate;
    int32_t pitch;
    uint64_t want;
  } cases[] = {
      {16000, 22050, 0, 3116529408ull},      /* unity: table[0]/shift 16 */
      {16000, 22050, 80, 3263785472ull},     /* table[80] = 0x10C1B */
      {16000, 22050, -1, 3114581504ull},     /* table[1199], shift 17 */
      {16000, 22050, 1200, 6233059072ull},   /* one octave up, shift 15 */
      {16000, 22050, -1200, 1558264576ull},  /* one octave down, shift 17 */
      {16000, 22050, 8400, 398915783168ull}, /* window base 7 octaves up */
      {22050, 22050, 0, 1ull << 32},         /* rate == out_rate unity */
      {0, 22050, 0, 0},                      /* zero rate -> zero step */
  };
  for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
    uint64_t st = 0xDEADBEEFull;
    assert(fifa96_mixer_step_from_pitch(cases[i].rate, cases[i].out_rate,
                                        cases[i].pitch, &st) == 0);
    assert(st == cases[i].want);
  }
  uint64_t st = 0;
  assert(fifa96_mixer_step_from_pitch(16000, 0, 0, &st) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_step_from_pitch(16000, 22050, 9600, &st) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_step_from_pitch(16000, 22050, 0, NULL) ==
         -(int)FIFA96_ERR_TRUNCATED);
}

static void test_pitch_table_checksum(void) {
  /* Pitch 0..1199 selects table[pitch] directly at shift 16; FNV-1a/32 over
   * the 1200 entry bytes guards the embedded extraction (image 0xB6B88)
   * against transcription drift. */
  uint32_t h = 2166136261u;
  for (int32_t p = 0; p < 1200; p++) {
    uint32_t ratio = 0, shift = 0;
    assert(fifa96_mixer_pitch_ratio(p, &ratio, &shift) == 0);
    assert(shift == 16);
    for (int b = 0; b < 4; b++) {
      h ^= (ratio >> (8 * b)) & 0xFFu;
      h *= 16777619u;
    }
  }
  assert(h == 0xDAFFEBCFu);
}

static void test_pan_gains(void) {
  /* FUN_000a662c 0xA663A..0xA66A4: pan > 0x7F mirrors (0xFF-pan); p < 0x40
   * keeps L 0x7F and raises R 2p; p == 0x40 is both 0x7F; p > 0x40 drops L
   * by (0x7F-p)*0x7E/0x3E and keeps R 0x7F. Each factor is scaled by the
   * signed record gain (+0x26) with /0x7F. */
  static const struct {
    uint8_t pan, gain, left, right;
  } cases[] = {
      {0x00, 127, 127, 0},   {0x20, 127, 127, 64},  {0x3F, 127, 127, 126},
      {0x40, 127, 127, 127}, {0x41, 127, 126, 127}, {0x50, 127, 95, 127},
      {0x60, 127, 63, 127},  {0x7F, 127, 0, 127},   {0x80, 127, 0, 127},
      {0xC0, 127, 127, 126}, {0xFF, 127, 127, 0},   {0x40, 64, 64, 64},
      {0x50, 64, 47, 64},    {0x20, 64, 64, 32},
  };
  for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
    uint8_t l = 0xAA, r = 0xBB;
    fifa96_mixer_pan_gains(cases[i].pan, cases[i].gain, &l, &r);
    assert(l == cases[i].left && r == cases[i].right);
  }
}

static void test_pan_render(void) {
  /* PCM16 stereo units (1000,2000) and (-1000,-2000), gain 0x7F; the reader
   * applies L to the left word and R to the right word with SAR 7. */
  uint8_t buf[0x20 + 8];
  hdr(buf, 2, 2, 0, 3);
  put16le(buf + 0x20, 1000);
  put16le(buf + 0x22, 2000);
  put16le(buf + 0x24, -1000);
  put16le(buf + 0x26, -2000);
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(buf, sizeof buf, &info) == 0);

  static const struct {
    uint8_t pan;
    int16_t want[4];
  } cases[] = {
      {0x00, {992, 0, -993, 0}},          /* hard left  L 127 R 0 */
      {0x20, {992, 1000, -993, -1000}},   /* L 127 R 64 */
      {0x50, {742, 1984, -743, -1985}},   /* L 95 R 127 */
      {0x7F, {0, 1984, 0, -1985}},        /* hard right L 0 R 127 */
      {0xC0, {992, 1968, -993, -1969}},   /* 0x80.. mirror to p=0x3F */
      {0xFF, {992, 0, -993, 0}},          /* mirror to hard left */
  };
  for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
    struct fifa96_mixer m;
    fifa96_mixer_init(&m);
    assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 0x7F,
                              (uint64_t)1 << 32) == 0);
    assert(fifa96_mixer_set_pan(&m, 3, cases[i].pan) == 0);
    int16_t out[4] = {0};
    fifa96_mixer_render(&m, out, 2);
    for (int k = 0; k < 4; k++) assert(out[k] == cases[i].want[k]);
  }

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_set_pan(NULL, 3, 0x40) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_set_pan(&m, 3, 0x40) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 0x7F,
                            (uint64_t)1 << 32) == 0);
  assert(fifa96_mixer_set_pan(&m, -1, 0x40) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_set_pan(&m, 16, 0x40) == -(int)FIFA96_ERR_TRUNCATED);
}

static void test_calibrated_golden_first_frames(void) {
  /* The real bank-h chunk at the calibrated 16000 -> 22050 pitch-0 step
   * (3116529408, not the old tail's 3116529557): cursor units advance
   * 0,0,1,2,2,3,4,5, selecting (-32,-38) (-32,-38) (-32,-40) (-35,-42)
   * (-35,-42) (-36,-41) (-31,-42) (-32,-44) at volume 0x7F. */
  uint8_t *h = NULL;
  size_t hn = 0;
  assert(fifa96_file_read("tests/golden/eacs/bank-h.eacs", &h, &hn) == 0);
  assert(hn == 0x1080);
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(h, hn, &info) == 0);
  assert(info.rate == 16000);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  uint64_t step = 0;
  assert(fifa96_mixer_step_from_pitch(info.rate, 22050, 0, &step) == 0);
  assert(step == 3116529408ull);
  assert(fifa96_mixer_start(&m, info.voice, &info, h, hn, 0x7F, step) == 0);
  int16_t out[16] = {0};
  fifa96_mixer_render(&m, out, 8);
  const int16_t want[16] = {-32, -38, -32, -38, -32, -40, -35, -42,
                            -35, -42, -36, -41, -31, -42, -32, -44};
  for (int i = 0; i < 16; i++) assert(out[i] == want[i]);
  assert(m.voices[info.voice].pos == 8 * step);
  free(h);
}

int main(void) {
  test_pitch_ratio_table();
  test_pitch_table_checksum();
  test_step_from_pitch();
  test_pan_gains();
  test_pan_render();
  test_calibrated_golden_first_frames();
  printf("test_mixer_calibration OK\n");
  return 0;
}
