// tests/test_sfx.c — FU-43 §3.2 bank arm path: global SFX id -> descriptor ->
// mixer voice.
//
// Golden fixture: tests/golden/sfx_game.bnk (provenance in tests/test_bnk.c).
// Every retail descriptor has pitch 0, volume base 0x7F (id 5 is 0x3C), pan
// centre 0x40, zero randomization spans, no pan override and no two-voice
// flag (FU-43 §1.1/§4), so the golden arm resolves without randomness; the
// expected mixer parameters are recomputed independently through
// fifa96_eacs_parse on a contiguous EACS view (the same normalization the
// registrar performs, FU-43 §1/§4.1) and fed to fifa96_mixer_start.
//
// The disassembly pins behind the edge cases (Ghidra /fifa96_le.bin):
//   * arm 0xA780E: volume 0xA78D4..0xA7922, pan 0xA7925..0xA796F,
//     pitch 0xA789D..0xA78C8, gain FUN_000b9fdd @ 0xB9FDD;
//   * volume/pan draws are `rand() >> 16` (SHR EAX,0x10);
//   * the pan-override path draws once for the bounds check and, when it
//     passes, draws again for the stored value (0xA7938 and FUN_000a79c1);
//   * split FUN_000a6717 @ 0xA6717 with table DAT_000148e0 = {150,140,130,
//     120,110} (image 0x1148E0).
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_bnk.h"
#include "fifa96_loader/fifa96_eacs.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_mixer.h"
#include "fifa96_loader/fifa96_sfx.h"

static void put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

// Contiguous EACS view (same as tests/test_bnk.c): copy the 32-byte header,
// rewrite the container's absolute +0x18 back to the parser's implicit 0x20
// and append the payload extent.
static uint8_t *eacs_view(const struct fifa96_bnk_info *info,
                          const struct fifa96_bnk_entry *e, size_t *len) {
  size_t vn = 0x20 + e->payload_len;
  uint8_t *v = malloc(vn);
  assert(v);
  memcpy(v, info->src + e->eacs_off, 0x20);
  put32le(v + 0x18, 0x20);
  memcpy(v + 0x20, info->src + e->payload_off, e->payload_len);
  *len = vn;
  return v;
}

// A 2-entry mono-delta bank: id 7 at descriptor 0x200 (EACS 0x228, absolute
// payload offset 0x2A0, 4 declared nibbles in 2 bytes + 2 gap bytes) and id 8
// at 0x248 (EACS 0x270, payload 0x2A4, 4 nibbles). Both descriptors carry the
// retail-shaped fields: pan fallback 0x40, volume base 0x7F, no spans/flags.
static size_t build_sfx_bank(uint8_t *b) {
  memset(b, 0, 0x2A6);
  put32le(b + 4 * 7, 0x200);
  put32le(b + 4 * 8, 0x248);

  put32le(b + 0x200, 0x0040);      /* id 7 voice mask */
  put32le(b + 0x204, 0x228);       /* embedded EACS */
  b[0x214] = 0x64;                 /* priority */
  b[0x218] = 0x40;                 /* pan fallback */
  b[0x219] = 0x7F;                 /* volume base */
  memcpy(b + 0x228, "EACS", 4);
  put32le(b + 0x22C, 16000);
  b[0x230] = 2; b[0x231] = 1; b[0x232] = 2; b[0x233] = 0xFF;
  put32le(b + 0x234, 4);           /* declared nibbles */
  /* loop start +0x10 = 0, loop length +0x14 = 0 */
  put32le(b + 0x240, 0x2A0);       /* absolute payload offset */
  b[0x245] = 0x7F;                 /* EACS +0x1D volume (unread by the arm) */

  put32le(b + 0x248, 0x0080);      /* id 8 voice mask */
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

  b[0x2A0] = 0x12; b[0x2A1] = 0x34;  /* nibbles 1,2,3,4 */
  b[0x2A4] = 0x56; b[0x2A5] = 0x78;
  return 0x2A6;
}

// Scripted FUN_000cbc4c stand-in; the arm consumes rand() >> 16.
struct rng_script { uint32_t v[8]; int n, i; };
static uint32_t rng_next(void *ctx) {
  struct rng_script *s = ctx;
  assert(s->i < s->n);
  return s->v[s->i++];
}
static uint32_t r16(uint32_t v) { return v << 16; }

static void test_parse_bank_matches_view(void) {
  uint8_t *b = NULL;
  size_t n = 0;
  assert(fifa96_file_read("tests/golden/sfx_game.bnk", &b, &n) == 0);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_OK);

  static const uint32_t ids[] = {1, 5, 10, 29, 31};
  for (size_t k = 0; k < sizeof ids / sizeof ids[0]; k++) {
    struct fifa96_bnk_entry e;
    assert(fifa96_bnk_entry(&info, ids[k], &e) == FIFA96_OK);
    struct fifa96_eacs_info bk, vw;
    assert(fifa96_eacs_parse_bank(info.src, info.src_len, e.eacs_off,
                                  e.payload_len, &bk) == 0);
    size_t vn = 0;
    uint8_t *v = eacs_view(&info, &e, &vn);
    assert(fifa96_eacs_parse(v, vn, &vw) == 0);
    free(v);
    assert(bk.rate == vw.rate && bk.f8 == vw.f8 && bk.f9 == vw.f9 && bk.f10 == vw.f10);
    assert(bk.voice == vw.voice && bk.count == vw.count);
    assert(bk.loop_start == vw.loop_start && bk.loop_len == vw.loop_len);
    assert(bk.volume == vw.volume && bk.block_size == vw.block_size);
    assert(bk.blocks == vw.blocks && bk.samples == vw.samples);
    assert(bk.delta_units == vw.delta_units && bk.format == vw.format);
    assert(bk.data_off == e.payload_off && bk.data_len == e.payload_len);
    assert(bk.data_off == e.payload_off);
  }

  struct fifa96_eacs_info eo;
  assert(fifa96_eacs_parse_bank(NULL, n, 0x228, 2, &eo) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_parse_bank(b, n, 0x228, 2, NULL) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_parse_bank(b, n, n, 2, &eo) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_parse_bank(b, n, 0x228, (uint32_t)n, &eo) == -(int)FIFA96_ERR_TRUNCATED);
  free(b);
}

static void test_golden_arm_render(void) {
  uint8_t *b = NULL;
  size_t n = 0;
  assert(fifa96_file_read("tests/golden/sfx_game.bnk", &b, &n) == 0);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_OK);

  static const uint32_t ids[] = {1, 5, 10, 29, 31};
  static const uint8_t want_vol[] = {0x7F, 0x3C, 0x7F, 0x7F, 0x7F};
  for (size_t k = 0; k < sizeof ids / sizeof ids[0]; k++) {
    struct fifa96_mixer m;
    fifa96_mixer_init(&m);
    struct fifa96_sfx_voice p[2];
    assert(fifa96_sfx_arm(&m, 4, &info, ids[k], NULL, p) == 4);
    assert(p[0].id == ids[k] && p[0].voice == 4);
    assert(p[0].volume == want_vol[k]);
    assert(p[0].pan == 0x40);
    assert(p[0].caller == 0x7F);
    assert(p[0].gain == want_vol[k]);   /* caller 0x7F x master 0x7F == unity */
    assert(p[0].pitch == 0);
    assert(p[0].loop == 0 && p[0].loop_start == 0 && p[0].loop_end == 0);
    assert(fifa96_mixer_voice_active(&m, 4) == 1);

    struct fifa96_bnk_entry e;
    assert(fifa96_bnk_entry(&info, ids[k], &e) == FIFA96_OK);
    size_t vn = 0;
    uint8_t *v = eacs_view(&info, &e, &vn);
    struct fifa96_eacs_info ei;
    assert(fifa96_eacs_parse(v, vn, &ei) == 0);
    uint64_t step = 0;
    assert(fifa96_mixer_step_from_rate(ei.rate, 22050u, 0x10000u, 16u, &step) == 0);
    struct fifa96_mixer em;
    fifa96_mixer_init(&em);
    assert(fifa96_mixer_start(&em, 4, &ei, v, vn, want_vol[k], step) == 0);

    assert(m.voices[4].volume == em.voices[4].volume);
    assert(m.voices[4].step == em.voices[4].step);
    assert(m.voices[4].units == em.voices[4].units);
    assert(m.voices[4].format == em.voices[4].format);
    assert(m.voices[4].active == 1);
    assert(m.voices[4].data == b + e.payload_off);  /* borrows the bank file */

    int16_t got[2 * 256], exp[2 * 256];
    fifa96_mixer_render(&m, got, 256);
    fifa96_mixer_render(&em, exp, 256);
    assert(memcmp(got, exp, sizeof got) == 0);
    free(v);
  }
  free(b);
}

static void test_golden_arm_errors(void) {
  uint8_t *b = NULL;
  size_t n = 0;
  assert(fifa96_file_read("tests/golden/sfx_game.bnk", &b, &n) == 0);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_OK);
  struct fifa96_sfx_voice p[2];
  struct fifa96_mixer m;
  fifa96_mixer_init(&m);

  /* slots 0 and 32 are empty in SFX_GAME (FU-43 §4). */
  assert(fifa96_sfx_arm(&m, 0, &info, 0, NULL, p) == -(int)FIFA96_ERR_NOT_FOUND);
  assert(fifa96_sfx_arm(&m, 0, &info, 32, NULL, p) == -(int)FIFA96_ERR_NOT_FOUND);
  assert(fifa96_sfx_arm(&m, 0, &info, 128, NULL, p) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_arm(NULL, 0, &info, 1, NULL, p) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_arm(&m, 0, NULL, 1, NULL, p) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_arm(&m, -1, &info, 1, NULL, p) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_arm(&m, FIFA96_MIXER_VOICES, &info, 1, NULL, p) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(!fifa96_mixer_voice_active(&m, 0));

  struct fifa96_sfx_opts o = {-1, 0x7F, 0x7F, NULL, NULL};
  o.volume = 0x80;
  assert(fifa96_sfx_arm(&m, 0, &info, 1, &o, p) == -(int)FIFA96_ERR_TRUNCATED);
  o.volume = 0x7F;
  o.master = 0x80;
  assert(fifa96_sfx_arm(&m, 0, &info, 1, &o, p) == -(int)FIFA96_ERR_TRUNCATED);
  o.master = 0x7F;
  o.pan = 0x100;
  assert(fifa96_sfx_arm(&m, 0, &info, 1, &o, p) == -(int)FIFA96_ERR_TRUNCATED);
  o.pan = -2;
  assert(fifa96_sfx_arm(&m, 0, &info, 1, &o, p) == -(int)FIFA96_ERR_TRUNCATED);
  free(b);
}

static void test_synthetic_loop_propagation(void) {
  uint8_t b[0x2A6];
  build_sfx_bank(b);
  put32le(b + 0x238, 1);  /* EACS +0x10 loop start unit 1 */
  put32le(b + 0x23C, 2);  /* EACS +0x14 loop length units 2 */
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);

  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  struct fifa96_sfx_voice p[2];
  assert(fifa96_sfx_arm(&m, 3, &info, 7, NULL, p) == 3);
  assert(p[0].loop == 1 && p[0].loop_start == 1 && p[0].loop_end == 3);
  assert(m.voices[3].loop == 1 && m.voices[3].loop_start == 1 && m.voices[3].loop_end == 3);

  struct fifa96_bnk_entry e;
  assert(fifa96_bnk_entry(&info, 7, &e) == FIFA96_OK);
  size_t vn = 0;
  uint8_t *v = eacs_view(&info, &e, &vn);
  struct fifa96_eacs_info ei;
  assert(fifa96_eacs_parse(v, vn, &ei) == 0);
  uint64_t step = 0;
  assert(fifa96_mixer_step_from_rate(ei.rate, 22050u, 0x10000u, 16u, &step) == 0);
  struct fifa96_mixer em;
  fifa96_mixer_init(&em);
  assert(fifa96_mixer_start(&em, 3, &ei, v, vn, 0x7F, step) == 0);
  int16_t got[16], exp[16];
  fifa96_mixer_render(&m, got, 8);
  fifa96_mixer_render(&em, exp, 8);
  assert(memcmp(got, exp, sizeof got) == 0);
  /* count 4, window [1,3): nibbles 0,1,2, wrap to 1,2, wrap ... */
  assert(m.voices[3].loop == 1);
  free(v);
}

static void test_synthetic_pan_paths(void) {
  uint8_t b[0x2A6];
  build_sfx_bank(b);
  b[0x218] = 0x20;  /* descriptor +0x18 fallback pan 0x20 */
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  /* caller pan -1, no override: descriptor +0x18, no draw. */
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 7, NULL, p) == 0);
  assert(p[0].pan == 0x20);

  /* caller pan != -1 wins over the descriptor override and needs no draw. */
  fifa96_mixer_init(&m);
  struct fifa96_sfx_opts o = {0x35, 0x7F, 0x7F, NULL, NULL};
  b[0x21D] = 0x30;  /* descriptor +0x1D would draw if consulted */
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  assert(fifa96_sfx_arm(&m, 0, &info, 7, &o, p) == 0);
  assert(p[0].pan == 0x35);

  /* override path, first draw clamps: span 0x50, r16 0xFFFF gives
   * 0x40 + ((0xFFFF*0x50)>>15) - 0x50 = 0x40 + 159 - 80 = 0x8F > 0x7F. */
  b[0x21D] = 0x50;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct rng_script clamp = {{0, 0, 0, 0, 0, 0, 0, 0}, 8, 0};
  clamp.v[0] = r16(0xFFFF);
  o.pan = -1;
  o.rand = rng_next;
  o.rand_ctx = &clamp;
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 7, &o, p) == 0);
  assert(p[0].pan == 0x7F);
  assert(clamp.i == 1);  /* first result > 0x7F: no second draw */

  /* override path, first draw passes: the discarded first draw is followed
   * by a second, unchecked draw (0xA795D). span 0x10:
   *   r16 0     -> p1 = 0x40 + 0 - 0x10 = 0x30 (passes)
   *   r16 0xFFFF-> p2 = 0x40 + 31 - 0x10 = 0x4F (stored) */
  b[0x21D] = 0x10;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct rng_script twice = {{0, 0, 0, 0, 0, 0, 0, 0}, 8, 0};
  twice.v[0] = r16(0);
  twice.v[1] = r16(0xFFFF);
  o.rand_ctx = &twice;
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 7, &o, p) == 0);
  assert(p[0].pan == 0x4F);
  assert(twice.i == 2);

  /* no provider while a draw is needed. */
  fifa96_mixer_init(&m);
  o.rand = NULL;
  assert(fifa96_sfx_arm(&m, 0, &info, 7, &o, p) == -(int)FIFA96_ERR_UNSUPPORTED);
  assert(!fifa96_mixer_voice_active(&m, 0));
}

static void test_synthetic_volume_and_pitch_span(void) {
  uint8_t b[0x2A6];
  build_sfx_bank(b);
  b[0x219] = 0x10;  /* volume base 16 */
  b[0x21A] = 0x20;  /* span 32 */
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];
  struct fifa96_sfx_opts o = {-1, 0x7F, 0x7F, rng_next, NULL};

  /* r16 0xFFFF: 16 + ((0xFFFF*32)>>15) - 32 = 16 + 63 - 32 = 47. */
  struct rng_script s = {{0, 0, 0, 0, 0, 0, 0, 0}, 8, 0};
  s.v[0] = r16(0xFFFF);
  o.rand_ctx = &s;
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 7, &o, p) == 0);
  assert(p[0].volume == 47 && p[0].gain == 47);
  assert(s.i == 1);

  /* r16 0: 16 - 32 -> clamps to 0. */
  s.i = 0;
  s.v[0] = r16(0);
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 7, &o, p) == 0);
  assert(p[0].volume == 0 && p[0].gain == 0);

  /* no span: base passes through and the provider is never called. */
  b[0x21A] = 0;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_sfx_opts no_rand = {-1, 0x7F, 0x7F, NULL, NULL};
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 7, &no_rand, p) == 0);
  assert(p[0].volume == 0x10 && p[0].gain == 0x10);

  /* pitch +0x0C/+0x10: 100 + ((0xFFFF*8)>>15) - 8 = 107; FU-46 §1 routes it
   * through the table head: table[107] = 0x11052 at shift 16. */
  put32le(b + 0x20C, 8);
  put32le(b + 0x210, 100);
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  s.i = 0;
  s.v[0] = r16(0xFFFF);
  o.rand_ctx = &s;
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 7, &o, p) == 0);
  assert(p[0].pitch == 107);
  uint64_t step = 0;
  assert(fifa96_mixer_step_from_pitch(16000, 22050u, 107, &step) == 0);
  assert(m.voices[0].step == step);
  uint32_t ratio = 0, shift = 0;
  assert(fifa96_mixer_pitch_ratio(107, &ratio, &shift) == 0);
  assert(ratio == 0x11052u && shift == 16);

  /* negative span (s8) walks the cited logical-shift path and clamps. */
  b[0x21A] = 0xF0;  /* -16 */
  b[0x219] = 0x7F;
  put32le(b + 0x20C, 0);
  put32le(b + 0x210, 0);
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  s.i = 0;
  s.v[0] = r16(0);
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 7, &o, p) == 0);
  assert(p[0].volume == 0x7F);
  assert(s.i == 1);

  /* span needs a provider. */
  b[0x21A] = 1;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 7, &no_rand, p) == -(int)FIFA96_ERR_UNSUPPORTED);
}

static void test_synthetic_two_voice(void) {
  uint8_t b[0x2A6];
  build_sfx_bank(b);
  b[0x21C] = 1;  /* descriptor id 7 +0x1C bit 0 */
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  /* explicit pan 2: esi 2 -> volA=(2+5)*127/10=88, volB=39; table[2]=130 ->
   * volA=114, volB=50; both voices pan 2. */
  struct fifa96_sfx_opts o = {2, 0x7F, 0x7F, NULL, NULL};
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 1, &info, 7, &o, p) == ((2 << 16) | 1));
  assert(p[0].id == 7 && p[0].voice == 1 && p[0].pan == 2);
  assert(p[0].volume == 0x7F && p[0].caller == 114 && p[0].gain == 114);
  assert(p[1].id == 8 && p[1].voice == 2 && p[1].pan == 2);
  assert(p[1].volume == 0x7F && p[1].caller == 50 && p[1].gain == 50);
  assert(fifa96_mixer_voice_active(&m, 1) == 1);
  assert(fifa96_mixer_voice_active(&m, 2) == 1);
  assert(m.voices[1].volume == 114 && m.voices[2].volume == 50);
  /* FU-46 §2: pan 2 -> L factor 0x7F, R factor 4; gain 114 -> L 114 R 3,
   * gain 50 -> L 50 R 1. */
  assert(m.voices[1].gain_l == 114 && m.voices[1].gain_r == 3);
  assert(m.voices[2].gain_l == 50 && m.voices[2].gain_r == 1);

  /* mirrored pan 0x80: local 0x7F, esi 0; volB=(0+5)*127/10=63, volA=64;
   * table[0]=150 -> volA=96, volB=94. */
  struct fifa96_sfx_opts mirror = {0x80, 0x7F, 0x7F, NULL, NULL};
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 1, &info, 7, &mirror, p) == ((2 << 16) | 1));
  assert(p[0].pan == 0x7F && p[0].caller == 96 && p[0].gain == 96);
  assert(p[1].pan == 0x7F && p[1].caller == 94 && p[1].gain == 94);
  /* mirrored 0x80 -> p 0x7F: hard right, both channels at the gain. */
  assert(m.voices[1].gain_l == 0 && m.voices[1].gain_r == 96);
  assert(m.voices[2].gain_l == 0 && m.voices[2].gain_r == 94);

  /* default pan -1: FUN_000a6717's invalid branch takes ESI = id (0xA772E ->
   * 0xA67B0); id 7 >= 5 skips the table, so voice A keeps the caller volume
   * and voice B is silent, both at centre. */
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 3, &info, 7, NULL, p) == ((4 << 16) | 3));
  assert(p[0].pan == 0x40 && p[0].caller == 0x7F && p[0].gain == 0x7F);
  assert(p[1].pan == 0x40 && p[1].caller == 0 && p[1].gain == 0);
  assert(fifa96_mixer_voice_active(&m, 3) == 1);
  assert(fifa96_mixer_voice_active(&m, 4) == 1);
  /* centre: both reader gains equal the combined gain. */
  assert(m.voices[3].gain_l == 0x7F && m.voices[3].gain_r == 0x7F);
  assert(m.voices[4].gain_l == 0 && m.voices[4].gain_r == 0);

  /* id+1 absent: NOT_FOUND and nothing armed (port atomicity). */
  put32le(b + 4 * 8, 0);
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 1, &info, 7, &o, p) == -(int)FIFA96_ERR_NOT_FOUND);
  assert(!fifa96_mixer_voice_active(&m, 1));
  assert(!fifa96_mixer_voice_active(&m, 2));
}

static void test_synthetic_errors(void) {
  uint8_t b[0x2A6];
  build_sfx_bank(b);
  struct fifa96_bnk_info info;
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];
  fifa96_mixer_init(&m);

  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  assert(fifa96_sfx_arm(&m, 0, &info, 8, NULL, p) == 0);  /* id 8 present, mono */
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 9, NULL, p) == -(int)FIFA96_ERR_NOT_FOUND);

  /* embedded EACS magic (arm check 0xA7840 -> -10); the tag is checked before
   * the record fill, so the span must not consume a draw. */
  b[0x228] = 'X';
  b[0x21A] = 1;
  struct rng_script none = {{0, 0, 0, 0, 0, 0, 0, 0}, 0, 0};
  struct fifa96_sfx_opts magic_opts = {-1, 0x7F, 0x7F, rng_next, &none};
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 7, &magic_opts, p) == -(int)FIFA96_ERR_BAD_MAGIC);
  assert(none.i == 0);
  assert(!fifa96_mixer_voice_active(&m, 0));
  b[0x228] = 'E';
  b[0x21A] = 0;

  /* unproven format flag combination reaches the mixer as UNSUPPORTED. */
  b[0x230] = 3;  /* f8 = 3 with f10 = 2 */
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm(&m, 0, &info, 7, NULL, p) == -(int)FIFA96_ERR_UNSUPPORTED);
}

int main(void) {
  test_parse_bank_matches_view();
  test_golden_arm_render();
  test_golden_arm_errors();
  test_synthetic_loop_propagation();
  test_synthetic_pan_paths();
  test_synthetic_volume_and_pitch_span();
  test_synthetic_two_voice();
  test_synthetic_errors();
  printf("test_sfx OK\n");
  return 0;
}
