// tests/test_audio_edge.c — FU-48 audio edge cases. Every expected value is
// hand-computed in docs/ghidra/FU48_audio_edge_cases.md from the cited
// instruction stream of /fifa96_le.bin:
//   * pitch head 0xB86C9..0xB86E6 (out-of-table index and SHRD CL wrap),
//   * step tail 0xB86ED..0xB872B (the masked step_int #DE),
//   * two-voice split FUN_000a6717 @ 0xA6717 + unpack 0xA777A..0xA77A0 +
//     gain FUN_000b9fdd @ 0xB9FDD signed IDIV,
//   * pan/gain FUN_000a662c @ 0xA662C unsigned DIV (the >0x7F gain quirk),
//   * retail two-voice census (descriptor +0x1C bit 0, FU-43 §4).
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_bnk.h"
#include "fifa96_loader/fifa96_eacs.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_mixer.h"
#include "fifa96_loader/fifa96_pacing.h"
#include "fifa96_loader/fifa96_sfx.h"

static void put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

static void put16le(uint8_t *p, int16_t v) {
  p[0] = (uint8_t)(uint16_t)v;
  p[1] = (uint8_t)((uint16_t)v >> 8);
}

/* Contiguous 32-byte EACS header (same shape as tests/test_mixer.c). */
static void hdr(uint8_t *b, uint8_t f8, uint8_t f9, uint8_t f10, int8_t voice) {
  memset(b, 0, 0x20);
  memcpy(b, "EACS", 4);
  b[8] = f8; b[9] = f9; b[10] = f10; b[11] = (uint8_t)voice;
  b[0x1D] = 0x7F;
}

/* Two-entry two-voice bank at id slots `id0` and `id0 + 1`: the same
 * retail-shaped descriptor/EACS layout as tests/test_sfx.c build_sfx_bank,
 * with descriptor `id0`'s +0x1C bit 0 set (the FUN_000a7728 two-voice flag). */
static size_t build_pair_bank(uint8_t *b, uint32_t id0) {
  memset(b, 0, 0x2A6);
  put32le(b + 4 * id0, 0x200);
  put32le(b + 4 * (id0 + 1), 0x248);

  put32le(b + 0x200, 0x0040);      /* id0 voice mask */
  put32le(b + 0x204, 0x228);       /* embedded EACS */
  b[0x214] = 0x64;                 /* priority */
  b[0x218] = 0x40;                 /* pan fallback */
  b[0x219] = 0x7F;                 /* volume base */
  b[0x21C] = 0x01;                 /* two-voice descriptor flag */
  memcpy(b + 0x228, "EACS", 4);
  put32le(b + 0x22C, 16000);
  b[0x230] = 2; b[0x231] = 1; b[0x232] = 2; b[0x233] = 0xFF;
  put32le(b + 0x234, 4);           /* declared nibbles */
  put32le(b + 0x240, 0x2A0);       /* absolute payload offset */
  b[0x245] = 0x7F;

  put32le(b + 0x248, 0x0080);      /* id0+1 voice mask */
  put32le(b + 0x24C, 0x270);
  b[0x25C] = 0x64;
  b[0x260] = 0x40;
  b[0x261] = 0x7F;
  memcpy(b + 0x270, "EACS", 4);
  put32le(b + 0x274, 16000);
  b[0x278] = 2; b[0x279] = 1; b[0x27A] = 2; b[0x27B] = 0xFF;
  put32le(b + 0x27C, 4);
  put32le(b + 0x288, 0x2A4);
  b[0x28D] = 0x7F;

  b[0x2A0] = 0x12; b[0x2A1] = 0x34;
  b[0x2A4] = 0x56; b[0x2A5] = 0x78;
  return 0x2A6;
}

/* FU-46 §1.1 head: `MOV EBX,[EBX*4+0xA6B88]` at 0xB86E6 has no bounds check.
 * Pitch 9599 -> index 1199 (last entry); 9600 -> index 1200 reads the first
 * code dword at image 0xB7E48 (0x50EC8B55), so the port rejects. Pitch -18000
 * is CL=31; -18001 is CL=32, which the original's SHRD masks to CL&31 = 0
 * (an unintended shift), so the port rejects it too. */
static void test_pitch_out_of_table(void) {
  uint32_t r = 0, s = 0;
  assert(fifa96_mixer_pitch_ratio(9599, &r, &s) == FIFA96_OK);
  assert(r == 0x1FFB4u && s == 9);
  assert(fifa96_mixer_pitch_ratio(9600, &r, &s) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_pitch_ratio(-18000, &r, &s) == FIFA96_OK);
  assert(r == 0x10000u && s == 31);
  assert(fifa96_mixer_pitch_ratio(-18001, &r, &s) == -(int)FIFA96_ERR_TRUNCATED);
  /* Extreme pitches still terminate: the head loop is stopped at CL 32. */
  assert(fifa96_mixer_pitch_ratio(0x7FFFFFFF, &r, &s) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_pitch_ratio((int32_t)0x80000000u, &r, &s) ==
         -(int)FIFA96_ERR_TRUNCATED);
  uint64_t st = 0;
  assert(fifa96_mixer_step_from_pitch(16000, 22050, 9600, &st) ==
         -(int)FIFA96_ERR_TRUNCATED);
}

/* FU-46 §1.3: `AND EAX,0xFF` at 0xB86FF masks the first DIV quotient to 8
 * bits, then `MUL 0x1000000` + `DIV [0x406A0]` at 0xB870E..0xB8722 faults
 * (#DE) whenever the unmasked quotient is >= 256: rem = v - (q&0xFF)*out =
 * 256*floor(q/256)*out + r, so (rem*2^24)>>32 = q_high*out + (r>>8) >= out.
 * The port rejects instead of fabricating the masked step (FU-46 §1.3's
 * 65535/100 -> 615683561728 truncation is now TRUNCATED).
 * Hand-computed boundaries at 22050 output, ratio 0x10000, shift 16:
 *   rate 5622750 = 255*22050:    q=255 rem 0     -> 0xFF00000000
 *   rate 5622751:                q=255 rem 1     -> 0xFF0002F800
 *   rate 5644799:                q=255 rem 22049 -> 0xFFFFFD0700
 *   rate 5644800 = 256*22050:    q=256 -> the original's second DIV faults. */
static void test_step_overflow_reject(void) {
  uint64_t st = 0;
  assert(fifa96_mixer_step_from_rate(5622750, 22050, 0x10000, 16, &st) ==
         FIFA96_OK);
  assert(st == 0xFF00000000ull);
  assert(fifa96_mixer_step_from_rate(5622751, 22050, 0x10000, 16, &st) ==
         FIFA96_OK);
  assert(st == 0xFF0002F800ull);
  assert(fifa96_mixer_step_from_rate(5644799, 22050, 0x10000, 16, &st) ==
         FIFA96_OK);
  assert(st == 0xFFFFFD0700ull);
  assert(fifa96_mixer_step_from_rate(5644800, 22050, 0x10000, 16, &st) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_step_from_rate(65535, 100, 0x10000, 16, &st) ==
         -(int)FIFA96_ERR_TRUNCATED);
  /* q < 256 is safe: 48000 -> integer part 2. */
  assert(fifa96_mixer_step_from_rate(48000, 22050, 0x10000, 16, &st) ==
         FIFA96_OK);
  assert(st == 9349588480ull);
}

/* FU-47 §1.3 / open leg 1: FUN_000a6717's invalid-pan branch leaves
 * ESI = id (0xA772E -> 0xA67B0); id < 5 scales left = table[id]*volume/100
 * (image 0x1148E0 = {150,140,130,120,110}), giving a byte > 0x7F (id 0,
 * volume 0x7F -> 150*127/100 = 190 = 0xBE). FUN_000a6717 packs it as a byte
 * (0xA67E0..0xA67E8) and the FUN_000a7728 unpack reads eax&0xFF (0xA7796); the record
 * stores it at +0x24 and FUN_000b9fdd MOVSXes it (0xB9FEE), so 0xBE becomes
 * -66. With desc vol 0x7F and master 0x7F the gain product is exactly
 * 16129*(-66) -> IDIV 0x3F01 = -66 -> AL = 0xBE (no clamp in the original).
 * FUN_000a662c then converts byte 0xBE at centre pan (0x40): q = ((uint32)
 * (127*-66))/0x7F = 0x020407CE, packed = (q<<16)|q, L = packed & 0x7F = 0x4E
 * = 78, R = packed >> 16 = 0x07CE|0x0204 = 1998. Voice B's caller byte is 0
 * (right = table[0]*0/100), so its gain and both reader gains are 0. */
static void test_split_cap_signed_readback(void) {
  uint8_t b[0x2A6];
  build_pair_bank(b, 0);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 0, NULL, p) == ((1 << 16) | 0));
  assert(p[0].id == 0 && p[0].voice == 0 && p[0].pan == 0x40);
  assert(p[0].volume == 0x7F && p[0].caller == 0xBE && p[0].gain == 0xBE);
  assert(p[1].id == 1 && p[1].voice == 1 && p[1].pan == 0x40);
  assert(p[1].volume == 0x7F && p[1].caller == 0 && p[1].gain == 0);
  assert(m.voices[0].volume == 0xBE);
  assert(m.voices[0].gain_l == 78 && m.voices[0].gain_r == 1998);
  assert(m.voices[1].gain_l == 0 && m.voices[1].gain_r == 0);

  /* id 4: 110*127/100 = 139 = 0x8B -> signed -117 -> gain byte 0x8B; centre
   * pan q = ((uint32)(127*-117))/0x7F = 0x0204079B, L = 0x1B = 27,
   * R = 0x079B|0x0204 = 0x079F = 1951. */
  build_pair_bank(b, 4);
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 4, NULL, p) == ((1 << 16) | 0));
  assert(p[0].caller == 0x8B && p[0].gain == 0x8B);
  assert(m.voices[0].gain_l == 27 && m.voices[0].gain_r == 1951);
}

/* FUN_000a662c 0xA6681..0xA66A4 with a gain byte > 0x7F: MOVSX makes it
 * negative, the 32-bit product is divided unsigned by 0x7F, and the packed
 * reader extraction (0xA6601 SHR 0x10, 0xA6605 AND 0x7F) keeps R = low 16 of
 * the right quotient, L = low 7 of the left quotient. Start a voice with the
 * raw gain byte 0xBE and centre pan: reader gains L=78, R=1998; the PCM16
 * reader's SAR 7 gives (1000,-1000) -> (609,-15610) and
 * (-1000,2000) -> (-610,31218) per frame. */
static void test_wide_render(void) {
  uint8_t buf[0x20 + 8];
  hdr(buf, 2, 2, 0, 3);
  put16le(buf + 0x20, 1000);
  put16le(buf + 0x22, -1000);
  put16le(buf + 0x24, -1000);
  put16le(buf + 0x26, 2000);
  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(buf, sizeof buf, &info) == FIFA96_OK);
  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_start(&m, 3, &info, buf, sizeof buf, 0xBE,
                            (uint64_t)1 << 32) == FIFA96_OK);
  assert(fifa96_mixer_set_pan(&m, 3, 0x40) == FIFA96_OK);
  int16_t out[4] = {0};
  fifa96_mixer_render(&m, out, 2);
  assert(out[0] == 609 && out[1] == -15610);
  assert(out[2] == -610 && out[3] == 31218);
}

/* FUN_000a662c 0xA6681..0xA66A4 exact quotients for a >0x7F gain byte. With
 * both factors equal (centre) and gain 0x80 (-128): q = ((uint32)(127*-128))/
 * 0x7F = (2^32-16256)/127 = 33818512 = 0x02040790; packed = (q<<16)|q, so
 * L = 0x790&0x7F = 0x10 = 16 and R = 0x0790|0x0204 = 0x0794 = 1940.
 * gain 0xBF (-65): q = ((uint32)(127*-65))/0x7F = 0x020407CF -> L=79 R=1999.
 * gain 0xFF (-1): q = ((uint32)(127*-1))/0x7F = 0x0204080F -> L=15 R=2575.
 * pan 0x00 (R factor 0): L=16, R = 0|0x0204 = 516; pan 0x7F (L factor 0):
 * L=0, R = 0x0790|0 = 1936. */
static void test_pan_gain_quirk(void) {
  static const struct {
    uint8_t pan, gain;
    uint32_t left, right;
  } cases[] = {
      {0x40, 0x80, 16, 1940}, {0x40, 0xBF, 79, 1999}, {0x40, 0xFF, 15, 2575},
      {0x00, 0x80, 16, 516},  {0x7F, 0x80, 0, 1936},   {0x20, 0xFF, 15, 2575},
  };
  for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
    uint32_t l = 0xDEADBEEFu, r = 0xDEADBEEFu;
    fifa96_mixer_pan_gains_wide(cases[i].pan, cases[i].gain, &l, &r);
    assert(l == cases[i].left && r == cases[i].right);
  }
  /* Retail 0..0x7F gains keep the narrow helper's 0..0x7F result. */
  uint8_t l = 0, r = 0;
  fifa96_mixer_pan_gains(0x20, 127, &l, &r);
  assert(l == 127 && r == 64);
  fifa96_mixer_pan_gains(0x50, 64, &l, &r);
  assert(l == 47 && r == 64);
  fifa96_mixer_pan_gains(0x00, 0, &l, &r);
  assert(l == 0 && r == 0);
}

/* ISR 0x9F5E4..0x9F64B: [0x12E88]++ per PIT tick (100 Hz) and every 5th
 * tick the 20 Hz divider [0x12E8C]++ plus the saved-handler call. The
 * carry-free tick counter wraps 32-bit; 0xFFFFFFFF + 1 = 0 and 0 % 5 == 0
 * (IDIV 5 remainder zero at 0x9F60E) fires the callback on the wrap. */
static void test_pacing_clock_isr(void) {
  struct fifa96_pacing_clock c;
  fifa96_pacing_clock_init(&c);
  assert(c.ticks == 0 && c.ticks20 == 0);
  for (int i = 1; i <= 4; i++) assert(fifa96_pacing_clock_isr(&c) == 0);
  assert(c.ticks == 4 && c.ticks20 == 0);
  assert(fifa96_pacing_clock_isr(&c) == 1);
  assert(c.ticks == 5 && c.ticks20 == 1);
  for (int i = 6; i <= 9; i++) assert(fifa96_pacing_clock_isr(&c) == 0);
  assert(fifa96_pacing_clock_isr(&c) == 1);
  assert(c.ticks == 10 && c.ticks20 == 2);

  c.ticks = 0xFFFFFFFFu; c.ticks20 = 0xFFFFFFFFu;
  assert(fifa96_pacing_clock_isr(&c) == 1);
  assert(c.ticks == 0 && c.ticks20 == 0);
  assert(fifa96_pacing_clock_isr(NULL) == 0);

  /* FUN_000cb2aa: now - then wraps. */
  assert(fifa96_pacing_clock_elapsed(10, 25) == 15);
  assert(fifa96_pacing_clock_elapsed(0xFFFFFFFEu, 1) == 3);
  /* 0xCB2EF: signed now-deadline >= 0. */
  assert(fifa96_pacing_deadline_reached(10, 9) == 0);
  assert(fifa96_pacing_deadline_reached(10, 10) == 1);
  assert(fifa96_pacing_deadline_reached(10, 11) == 1);
  /* 0 - 0x80000000 is negative as int32: the deadline is not reached. */
  assert(fifa96_pacing_deadline_reached(0, 0x80000000u) == 0);
}

/* FU-43 §4 / FU-47 §6 leg 2: every retail descriptor leaves +0x1C bit 0
 * clear, so the two-voice path is static-only. */
static void test_retail_two_voice_census(void) {
  uint8_t *b = NULL;
  size_t n = 0;
  assert(fifa96_file_read("tests/golden/sfx_game.bnk", &b, &n) == 0);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_OK);
  uint32_t present = 0;
  for (uint32_t id = 0; id < FIFA96_BNK_IDS; id++) {
    struct fifa96_bnk_entry e;
    if (fifa96_bnk_entry(&info, id, &e) != FIFA96_OK) continue;
    present++;
    assert((b[e.desc_off + 0x1C] & 1u) == 0);
  }
  assert(present > 0);
  free(b);
}

int main(void) {
  test_pitch_out_of_table();
  test_step_overflow_reject();
  test_split_cap_signed_readback();
  test_wide_render();
  test_pan_gain_quirk();
  test_pacing_clock_isr();
  test_retail_two_voice_census();
  printf("test_audio_edge OK\n");
  return 0;
}
