// tests/test_sfx_state.c — FU-47: voice allocation FUN_000a62fa, the
// FUN_000cbc4c randomization RNG with its seeders, and the [0x15FC8]
// sound-state gate. Spec: docs/ghidra/FU47_voice_alloc_rng_gate.md.
//
// Allocation policy (FU-47 §1): phase 1 picks the first free in-mask voice in
// rotor order from m->next_voice; phase 2, only when every in-mask voice is
// in use, steals the first in-mask voice whose stored priority is <= the new
// priority (unsigned byte). The rotor advances past a successful pick and is
// left alone on failure.
//
// RNG (FU-47 §2): six 32-bit words, w[0] = [0x12E68] (returned) .. w[5] =
// [0x12E7C]; each step suffix-sums the words (ADD/ADC 0xCBC4C..0xCBC83) then
// increments w[5] with a carry cascade (0xCBC88..0xCBCB6). The pinned
// sequences below come from the image state (0x112E68) and the two seeders.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_bnk.h"
#include "fifa96_loader/fifa96_mixer.h"
#include "fifa96_loader/fifa96_sfx.h"

static void put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

// Same 2-entry mono-delta fixture as tests/test_sfx.c: id 7 descriptor 0x200
// (mask 0x00000040 -> voice 6, priority 0x64), id 8 descriptor 0x248 (mask
// 0x00000080 -> voice 7, priority 0x64). Payloads at 0x2A0/0x2A4.
static size_t build_sfx_bank(uint8_t *b) {
  memset(b, 0, 0x2A6);
  put32le(b + 4 * 7, 0x200);
  put32le(b + 4 * 8, 0x248);

  put32le(b + 0x200, 0x0040);
  put32le(b + 0x204, 0x228);
  b[0x214] = 0x64;
  b[0x218] = 0x40;
  b[0x219] = 0x7F;
  memcpy(b + 0x228, "EACS", 4);
  put32le(b + 0x22C, 16000);
  b[0x230] = 2; b[0x231] = 1; b[0x232] = 2; b[0x233] = 0xFF;
  put32le(b + 0x234, 4);
  put32le(b + 0x240, 0x2A0);

  put32le(b + 0x248, 0x0080);
  put32le(b + 0x24C, 0x270);
  b[0x25C] = 0x64;
  b[0x260] = 0x40;
  b[0x261] = 0x7F;
  memcpy(b + 0x270, "EACS", 4);
  put32le(b + 0x274, 16000);
  b[0x278] = 2; b[0x279] = 1; b[0x27A] = 2; b[0x27B] = 0xFF;
  put32le(b + 0x27C, 4);
  put32le(b + 0x288, 0x2A4);

  b[0x2A0] = 0x12; b[0x2A1] = 0x34;
  b[0x2A4] = 0x56; b[0x2A5] = 0x78;
  return 0x2A6;
}

static void arm_mark(struct fifa96_mixer *m, int voice, uint8_t prio) {
  m->voices[voice].active = 1;
  assert(fifa96_mixer_set_priority(m, voice, prio) == FIFA96_OK);
}

// ── mixer allocation policy ──────────────────────────────────────────────────

static void test_alloc_first_free_rotor(void) {
  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(m.next_voice == 0);

  assert(fifa96_mixer_alloc_voice(&m, 1u << 4, 100) == 4);
  assert(m.next_voice == 5);
  // Nothing was activated: the next alloc sees voice 4 free again.
  assert(fifa96_mixer_alloc_voice(&m, 1u << 4, 100) == 4);

  // Wrap: after voice 15 the rotor returns to 0 (0xA6329..0xA632E).
  assert(fifa96_mixer_alloc_voice(&m, 1u << 15, 100) == 15);
  assert(m.next_voice == 0);

  // Rotor order skips active voices; voice 2 is the first free in mask.
  fifa96_mixer_init(&m);
  arm_mark(&m, 0, 100);
  arm_mark(&m, 2, 100);
  assert(fifa96_mixer_alloc_voice(&m, 0x000F, 100) == 1);

  // Empty mask: no allowed voice at any priority (0xA637D).
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_alloc_voice(&m, 0, 0xFF) == -1);
  assert(fifa96_mixer_alloc_voice(NULL, 0xFFFF, 0) == -(int)FIFA96_ERR_TRUNCATED);
}

static void test_alloc_steal_priority(void) {
  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  arm_mark(&m, 0, 100);
  arm_mark(&m, 1, 70);
  arm_mark(&m, 2, 60);
  arm_mark(&m, 3, 50);

  // Phase 2 from rotor 0: stored 100 <= 70 fails, stored 70 <= 70 hits -> 1.
  assert(fifa96_mixer_alloc_voice(&m, 0x000F, 70) == 1);
  assert(m.next_voice == 2);

  // 49 < every stored priority: no steal.
  assert(fifa96_mixer_alloc_voice(&m, 0x000F, 49) == -1);
  // A free in-mask voice still wins phase 1 regardless of priority.
  assert(fifa96_mixer_alloc_voice(&m, 0x00F0, 0) == 4);

  // Unsigned byte compare (0xA6367 MOVZX / 0xA636B CMP): stored 0x80 is not
  // <= new 0x7F.
  fifa96_mixer_init(&m);
  arm_mark(&m, 3, 0x80);
  assert(fifa96_mixer_alloc_voice(&m, 1u << 3, 0x7F) == -1);
  assert(fifa96_mixer_alloc_voice(&m, 1u << 3, 0x80) == 3);
}

static void test_set_priority(void) {
  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_set_priority(&m, 7, 0x64) == FIFA96_OK);
  assert(m.voices[7].priority == 0x64);
  assert(fifa96_mixer_set_priority(&m, -1, 1) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_set_priority(&m, FIFA96_MIXER_VOICES, 1) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_set_priority(NULL, 0, 1) == -(int)FIFA96_ERR_TRUNCATED);
}

// ── sound-state gate [0x15FC8] ───────────────────────────────────────────────

static void test_sound_state_setter(void) {
  struct fifa96_mixer m;
  fifa96_mixer_init(&m);
  assert(m.sound_state == 1);  // port default: gate open
  assert(fifa96_mixer_set_sound_state(&m, 5) == FIFA96_OK);
  assert(m.sound_state == 5);
  assert(fifa96_mixer_set_sound_state(&m, 0) == FIFA96_OK);  // FUN_000a6505
  assert(m.sound_state == 0);
  // FUN_000a6265 validates 0..5 and stores nothing on failure.
  assert(fifa96_mixer_set_sound_state(&m, 6) == -(int)FIFA96_ERR_TRUNCATED);
  assert(m.sound_state == 0);
  assert(fifa96_mixer_set_sound_state(&m, -1) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_mixer_set_sound_state(NULL, 1) == -(int)FIFA96_ERR_TRUNCATED);
}

static void test_gate_blocks_arm(void) {
  uint8_t b[0x2A6];
  build_sfx_bank(b);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  // 0 (stopped) and out-of-range are rejected with the original's -4
  // (0xA7852..0xA7864), before the format arm.
  fifa96_mixer_init(&m);
  assert(fifa96_mixer_set_sound_state(&m, 0) == FIFA96_OK);
  assert(fifa96_sfx_arm(&m, 0, &info, 7, NULL, p) == -(int)FIFA96_ERR_TRUNCATED);
  assert(!fifa96_mixer_voice_active(&m, 0));
  m.sound_state = 6;  // bypass the setter to aim at the gate
  assert(fifa96_sfx_arm(&m, 0, &info, 7, NULL, p) == -(int)FIFA96_ERR_TRUNCATED);
  assert(!fifa96_mixer_voice_active(&m, 0));

  // Every open value 1..5 arms.
  for (int st = 1; st <= 5; st++) {
    fifa96_mixer_init(&m);
    assert(fifa96_mixer_set_sound_state(&m, st) == FIFA96_OK);
    assert(fifa96_sfx_arm(&m, 3, &info, 7, NULL, p) == 3);
  }
}

struct draw_counter { int n; };
static uint32_t count_draw(void *ctx) {
  struct draw_counter *c = ctx;
  c->n++;
  return 0;
}

static void test_gate_before_draws(void) {
  uint8_t b[0x2A6];
  build_sfx_bank(b);
  b[0x21A] = 0x20;  // descriptor +0x1A volume span: a draw is needed
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  struct draw_counter c = {0};
  struct fifa96_sfx_opts o = {-1, 0x7F, 0x7F, count_draw, &c};
  fifa96_mixer_init(&m);
  m.sound_state = 0;
  // The gate is at 0xA7852 and the first draw at 0xA78E1: no provider call.
  assert(fifa96_sfx_arm(&m, 0, &info, 7, &o, p) == -(int)FIFA96_ERR_TRUNCATED);
  assert(c.n == 0);
}

// ── RNG FUN_000cbc4c and its seeders ─────────────────────────────────────────

static const uint32_t init_state[6] = {
  0xF22D0E56u, 0x883126E9u, 0xC624DD2Fu, 0x0702C49Cu, 0x9E353F7Du, 0x6FDF3B64u
};

static void test_rng_initial_sequence(void) {
  struct fifa96_sfx_rng r;
  fifa96_sfx_rng_init(&r);
  for (int i = 0; i < 6; i++) assert(r.w[i] == init_state[i]);

  static const uint32_t seq[10] = {
    0x559A51EDu, 0x274EA7F5u, 0xE827F7E0u, 0x76B65632u, 0x7F1B3FEBu,
    0xEAE8D5EEu, 0x5101186Fu, 0x6575225Au, 0x696464C0u, 0x9ADBE245u
  };
  for (int i = 0; i < 10; i++) assert(fifa96_sfx_rng_next(&r) == seq[i]);
  assert(fifa96_sfx_rng_next(NULL) == 0);
}

static void test_rng_carries(void) {
  struct fifa96_sfx_rng r;
  // All-zero state: the ADC chain returns 0 and INC w[5] advances it to 1.
  uint32_t zero[6] = {0, 0, 0, 0, 0, 0};
  memcpy(r.w, zero, sizeof zero);
  assert(fifa96_sfx_rng_next(&r) == 0);
  assert(r.w[0] == 0 && r.w[1] == 0 && r.w[2] == 0);
  assert(r.w[3] == 0 && r.w[4] == 0 && r.w[5] == 1);

  // w = (0,0,0,0,1,0xFFFFFFFF): suffix sums give w0..w4 = 1, then w5 wraps
  // and the cascade stops at the new w4 = 1.
  uint32_t wrap[6] = {0, 0, 0, 0, 1, 0xFFFFFFFFu};
  memcpy(r.w, wrap, sizeof wrap);
  assert(fifa96_sfx_rng_next(&r) == 1);
  assert(r.w[0] == 1 && r.w[1] == 1 && r.w[2] == 1);
  assert(r.w[3] == 1 && r.w[4] == 1 && r.w[5] == 0);

  // w = (0,0,0,0,0,0xFFFFFFFF): suffix sums leave every word at 0xFFFFFFFF;
  // the increment wraps all six words and the final INC EAX (0xCBCB6) turns
  // the returned 0xFFFFFFFF into 0.
  uint32_t full_wrap[6] = {0, 0, 0, 0, 0, 0xFFFFFFFFu};
  memcpy(r.w, full_wrap, sizeof full_wrap);
  assert(fifa96_sfx_rng_next(&r) == 0);
  for (int i = 0; i < 6; i++) assert(r.w[i] == 0);
}

static void test_rng_seed_functions(void) {
  struct fifa96_sfx_rng r, s;

  fifa96_sfx_rng_seed(&r, 0);
  assert(memcmp(r.w, init_state, sizeof init_state) == 0);

  fifa96_sfx_rng_seed(&r, 1);  // FUN_000cbcb8 @ 0xCBCB8 cumulative constants
  static const uint32_t seeded[6] = {
    0xF22D0E57u, 0x883126EAu, 0xC624DD30u, 0x0702C49Du, 0x9E353F7Eu, 0x6FDF3B65u
  };
  assert(memcmp(r.w, seeded, sizeof seeded) == 0);
  static const uint32_t seeded_seq[4] = {
    0x559A51F3u, 0x274EA80Au, 0xE827F818u, 0x76B656B0u
  };
  for (int i = 0; i < 4; i++) assert(fifa96_sfx_rng_next(&r) == seeded_seq[i]);

  // 0x4C698: w[i] = (seed << 25) + (int8)"ArCaDe-CoInOp"[i].
  fifa96_sfx_rng_seed_arcade(&s, 0x12345678u);
  static const uint32_t arcade[6] = {
    0xF0000041u, 0xF0000072u, 0xF0000043u, 0xF0000061u, 0xF0000044u, 0xF0000065u
  };
  assert(memcmp(s.w, arcade, sizeof arcade) == 0);
  static const uint32_t arcade_seq[4] = {
    0xA0000204u, 0xB0000733u, 0x8000135Fu, 0x20002BFCu
  };
  for (int i = 0; i < 4; i++) assert(fifa96_sfx_rng_next(&s) == arcade_seq[i]);
}

static void test_rng_default_provider(void) {
  struct fifa96_sfx_rng a, b;
  fifa96_sfx_rng_init(&a);
  fifa96_sfx_rng_init(&b);
  assert(fifa96_sfx_rng_default(&a) == fifa96_sfx_rng_next(&b));
  assert(fifa96_sfx_rng_default(&a) == 0x274EA7F5u);
  assert(fifa96_sfx_rng_default(NULL) == 0);
}

// ── sfx_arm_alloc: FUN_000a780e with the FUN_000a62fa allocator ──────────────

static void test_arm_alloc_single(void) {
  uint8_t b[0x2A6];
  build_sfx_bank(b);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  // id 7 mask 0x40 -> voice 6; the arm stores the descriptor priority
  // (0xA788A) and marks the record in use (0xA786E).
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm_alloc(&m, &info, 7, NULL, p) == 6);
  assert(p[0].voice == 6 && p[0].id == 7);
  assert(fifa96_mixer_voice_active(&m, 6) == 1);
  assert(m.voices[6].priority == 0x64);
  assert(m.next_voice == 7);

  // Equal priority steals the active voice (100 >= 100), no new voice.
  assert(fifa96_sfx_arm_alloc(&m, &info, 7, NULL, p) == 6);

  // Stored 100 > new 99: no allowed voice -> NO_VOICE (original -0x14).
  b[0x214] = 99;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  assert(fifa96_sfx_arm_alloc(&m, &info, 7, NULL, p) ==
         -(int)FIFA96_ERR_NO_VOICE);
  assert(fifa96_mixer_voice_active(&m, 6) == 1);  // untouched

  // A free allowed voice wins phase 1 even at low priority.
  assert(fifa96_sfx_arm_alloc(&m, &info, 8, NULL, p) == 7);
  assert(m.voices[7].priority == 0x64);
}

static void test_arm_alloc_two_voice_and_gate(void) {
  uint8_t b[0x2A6];
  build_sfx_bank(b);
  b[0x21C] = 1;  // id 7 descriptor +0x1C bit 0
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  // Default pan -1 invalid branch: voice A keeps the caller volume, voice B
  // is silent; each id allocates from its own descriptor mask.
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm_alloc(&m, &info, 7, NULL, p) == ((7 << 16) | 6));
  assert(p[0].voice == 6 && p[1].voice == 7);
  assert(fifa96_mixer_voice_active(&m, 6) && fifa96_mixer_voice_active(&m, 7));
  assert(m.voices[6].priority == 0x64 && m.voices[7].priority == 0x64);

  // Gate blocks both: nothing armed, but the first allocation already ran
  // (rotor advanced) exactly as FUN_000a780e does before 0xA7852.
  fifa96_mixer_init(&m);
  m.sound_state = 0;
  assert(fifa96_sfx_arm_alloc(&m, &info, 7, NULL, p) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(!fifa96_mixer_voice_active(&m, 6));
  assert(!fifa96_mixer_voice_active(&m, 7));
  assert(m.next_voice == 7);

  // Bad args mirror fifa96_sfx_arm.
  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm_alloc(NULL, &info, 7, NULL, p) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_arm_alloc(&m, NULL, 7, NULL, p) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_arm_alloc(&m, &info, 9, NULL, p) ==
         -(int)FIFA96_ERR_NOT_FOUND);
  assert(fifa96_sfx_arm_alloc(&m, &info, 128, NULL, p) ==
         -(int)FIFA96_ERR_TRUNCATED);
}

static void test_arm_alloc_uses_default_rng(void) {
  uint8_t b[0x2A6];
  build_sfx_bank(b);
  b[0x219] = 0x10;  // volume base 16
  b[0x21A] = 0x20;  // volume span 32 -> one draw
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  struct fifa96_sfx_rng producer, checker;
  fifa96_sfx_rng_init(&producer);
  fifa96_sfx_rng_init(&checker);
  struct fifa96_sfx_opts o = {-1, 0x7F, 0x7F, fifa96_sfx_rng_default, &producer};

  fifa96_mixer_init(&m);
  assert(fifa96_sfx_arm_alloc(&m, &info, 7, &o, p) == 6);
  // record+0x25 = base + ((r16*span)>>15) - span (0xA78E1..0xA791D).
  uint32_t r16 = fifa96_sfx_rng_next(&checker) >> 16;
  int32_t want = 0x10 + (int32_t)((r16 * 0x20u) >> 15) - 0x20;
  if (want > 0x7F) want = 0x7F;
  if (want < 0) want = 0;
  assert(p[0].volume == (uint8_t)want);
}

int main(void) {
  test_alloc_first_free_rotor();
  test_alloc_steal_priority();
  test_set_priority();
  test_sound_state_setter();
  test_gate_blocks_arm();
  test_gate_before_draws();
  test_rng_initial_sequence();
  test_rng_carries();
  test_rng_seed_functions();
  test_rng_default_provider();
  test_arm_alloc_single();
  test_arm_alloc_two_voice_and_gate();
  test_arm_alloc_uses_default_rng();
  printf("test_sfx_state OK\n");
  return 0;
}
