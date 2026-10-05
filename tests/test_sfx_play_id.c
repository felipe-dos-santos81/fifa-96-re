// tests/test_sfx_play_id.c — FU-51 play-by-id dispatch FUN_000a7728 over the
// runtime id table DAT_00061c14. Expected behavior is derived in
// docs/ghidra/FU51_audio_runtime_glue.md §4 from /fifa96_le.bin:
//   * 0xA7738..0xA7742: id outside 0..0x7F -> 0xFFFFFFED (-19);
//   * 0xA774D..0xA775D: DAT_00061c14[id] == 0 -> -19;
//   * 0xA775F..0xA776A: descriptor +0x1C bit 0 clear -> one arm (FUN_000a780e);
//   * 0xA7770..0xA7775: two-voice split FUN_000a6717(pan, volume) packs
//     local<<16 | left<<8 | right (FU-48 §2);
//   * 0xA77A3: arm id with EBX = split pan, ECX = split left;
//   * 0xA77B7..0xA77C5: second descriptor is the *next table slot*
//     ([0x61C18 + id*4] = table[id+1]); null -> -19, first voice left armed;
//   * 0xA77D6: arm id+1 with EBX = split pan, ECX = split right;
//   * 0xA77E2..0xA77E4: second arm failure -> FUN_000a6cdc(id) cleanup;
//   * 0xA77EF..0xA77F4: success returns (voice2 << 16) | voice1.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_bnk.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_mixer.h"
#include "fifa96_loader/fifa96_sfx.h"

static void put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

// One retail-shaped mono entry at table slot `id`: descriptor 0x200 (EACS
// 0x228, absolute payload offset 0x2A0, 4 declared nibbles), voice mask 0x40
// -> allocator picks voice 6 from rotor 0, priority 0x64.
static size_t build_one_bank(uint8_t *b, uint32_t id) {
  memset(b, 0, 0x2A6);
  put32le(b + 4 * id, 0x200);
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
  b[0x245] = 0x7F;
  b[0x2A0] = 0x12; b[0x2A1] = 0x34;
  return 0x2A6;
}

// Two entries at slots id0 and id0+1 (masks 0x40/0x80 -> voices 6/7 from rotor
// 0) with descriptor id0's +0x1C bit 0 set: the FUN_000a7728 two-voice flag.
// Same shape as tests/test_audio_edge.c build_pair_bank.
static size_t build_pair_bank(uint8_t *b, uint32_t id0) {
  memset(b, 0, 0x2A6);
  put32le(b + 4 * id0, 0x200);
  put32le(b + 4 * (id0 + 1), 0x248);

  put32le(b + 0x200, 0x0040);
  put32le(b + 0x204, 0x228);
  b[0x214] = 0x64;
  b[0x218] = 0x40;
  b[0x219] = 0x7F;
  b[0x21C] = 0x01;
  memcpy(b + 0x228, "EACS", 4);
  put32le(b + 0x22C, 16000);
  b[0x230] = 2; b[0x231] = 1; b[0x232] = 2; b[0x233] = 0xFF;
  put32le(b + 0x234, 4);
  put32le(b + 0x240, 0x2A0);
  b[0x245] = 0x7F;

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
  b[0x28D] = 0x7F;

  b[0x2A0] = 0x12; b[0x2A1] = 0x34;
  b[0x2A4] = 0x56; b[0x2A5] = 0x78;
  return 0x2A6;
}

static void table_clear(struct fifa96_sfx_id_table *t) {
  memset(t, 0, sizeof *t);
}

// 0xA7738..0xA775D: invalid id or an empty slot returns -19; a slot whose
// bank does not carry the id is the same -19 (the original dereferences a
// non-null descriptor, the port's bank lookup reports absence).
static void test_id_bounds_and_null_slots(void) {
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];
  struct fifa96_sfx_id_table t;
  table_clear(&t);
  fifa96_mixer_init(&m);

  assert(fifa96_sfx_play_id(NULL, &t, 0, NULL, p) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_play_id(&m, NULL, 0, NULL, p) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_sfx_play_id(&m, &t, -1, NULL, p) ==
         -(int)FIFA96_ERR_NOT_FOUND);
  assert(fifa96_sfx_play_id(&m, &t, 0, NULL, p) ==
         -(int)FIFA96_ERR_NOT_FOUND);
  assert(fifa96_sfx_play_id(&m, &t, 127, NULL, p) ==
         -(int)FIFA96_ERR_NOT_FOUND);
  assert(fifa96_sfx_play_id(&m, &t, 128, NULL, p) ==
         -(int)FIFA96_ERR_NOT_FOUND);

  uint8_t b[0x2A6];
  build_one_bank(b, 5);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  t.bank[5] = &info;
  assert(fifa96_sfx_play_id(&m, &t, 5, NULL, p) == 6);
  assert(fifa96_mixer_voice_active(&m, 6) == 1);
  // Slot 5's bank has no id 4 entry (table slot zero): -19.
  assert(fifa96_sfx_play_id(&m, &t, 4, NULL, p) ==
         -(int)FIFA96_ERR_NOT_FOUND);
}

// Single-voice golden path: the same descriptor/allocator chain as
// fifa96_sfx_arm_alloc (0xA781F..0xA7830, 0xA786E..0xA799F).
static void test_single_voice_golden(void) {
  uint8_t *b = NULL;
  size_t n = 0;
  assert(fifa96_file_read("tests/golden/sfx_game.bnk", &b, &n) == 0);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_OK);
  struct fifa96_sfx_id_table t;
  table_clear(&t);
  t.bank[1] = &info;
  t.bank[5] = &info;
  t.bank[31] = &info;
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  fifa96_mixer_init(&m);
  int v = fifa96_sfx_play_id(&m, &t, 1, NULL, p);
  assert(v >= 0 && v < FIFA96_MIXER_VOICES);
  assert(p[0].id == 1 && p[0].voice == v);
  assert(fifa96_mixer_voice_active(&m, v) == 1);
  assert(fifa96_mixer_voice_active(&m, v + 1) == 0);

  int v5 = fifa96_sfx_play_id(&m, &t, 5, NULL, p);
  assert(v5 >= 0 && v5 != v);
  assert(p[0].id == 5);
  assert(fifa96_mixer_voice_active(&m, v5) == 1);

  // Golden leaves table slot 0 zero (FU-43 §1): absent id.
  assert(fifa96_sfx_play_id(&m, &t, 0, NULL, p) ==
         -(int)FIFA96_ERR_NOT_FOUND);
  assert(fifa96_sfx_play_id(&m, &t, 32, NULL, p) ==
         -(int)FIFA96_ERR_NOT_FOUND);
  free(b);
}

// 0xA7770..0xA77F4: two-voice flag -> split, arm id (voice 6) then id+1
// (voice 7), packed (voice2 << 16) | voice1. Pan 2 / volume 0x7F split pins
// the FUN_000a6717 values already pinned by tests/test_sfx.c: caller volumes
// 114 and 50, both voices pan 2.
static void test_two_voice_packed(void) {
  uint8_t b[0x2A6];
  build_pair_bank(b, 7);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_sfx_id_table t;
  table_clear(&t);
  t.bank[7] = &info;
  t.bank[8] = &info;
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];
  struct fifa96_sfx_opts o = {2, 0x7F, 0x7F, NULL, NULL};

  fifa96_mixer_init(&m);
  assert(fifa96_sfx_play_id(&m, &t, 7, &o, p) == ((7 << 16) | 6));
  assert(p[0].id == 7 && p[0].voice == 6 && p[0].pan == 2);
  assert(p[0].caller == 114 && p[0].gain == 114);
  assert(p[1].id == 8 && p[1].voice == 7 && p[1].pan == 2);
  assert(p[1].caller == 50 && p[1].gain == 50);
  assert(fifa96_mixer_voice_active(&m, 6) == 1);
  assert(fifa96_mixer_voice_active(&m, 7) == 1);
}

// 0xA77B7: the second descriptor is table[id+1], a *different* bank slot is
// legal (DAT_00061c14 is global across registered banks).
static void test_second_descriptor_from_next_slot(void) {
  uint8_t a[0x2A6], b1[0x2A6];
  build_pair_bank(a, 7);
  put32le(a + 4 * 8, 0);  /* bank A carries only id 7 */
  build_pair_bank(b1, 8);
  put32le(b1 + 0x200, 0x0080); /* bank B id 8 -> voice 7 */
  put32le(b1 + 4 * 9, 0);      /* bank B carries only id 8 */
  struct fifa96_bnk_info ia, ib;
  assert(fifa96_bnk_parse(a, sizeof a, &ia) == FIFA96_OK);
  assert(fifa96_bnk_parse(b1, sizeof b1, &ib) == FIFA96_OK);
  struct fifa96_sfx_id_table t;
  table_clear(&t);
  t.bank[7] = &ia;
  t.bank[8] = &ib;
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  fifa96_mixer_init(&m);
  assert(fifa96_sfx_play_id(&m, &t, 7, NULL, p) == ((7 << 16) | 6));
  assert(p[0].id == 7 && p[1].id == 8);
  assert(fifa96_mixer_voice_active(&m, 6) == 1);
  assert(fifa96_mixer_voice_active(&m, 7) == 1);
}

// 0xA77C4: table[id+1] == 0 returns -19 with no cleanup, so the first voice
// stays armed (the original's POPFD; JMP 0x7744). This is the one path where
// the port is intentionally *not* atomic, matching FUN_000a7728.
static void test_second_voice_absent_leaves_first(void) {
  uint8_t b[0x2A6];
  build_pair_bank(b, 7);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_sfx_id_table t;
  table_clear(&t);
  t.bank[7] = &info;  /* slot 8 stays NULL */
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  fifa96_mixer_init(&m);
  assert(fifa96_sfx_play_id(&m, &t, 7, NULL, p) ==
         -(int)FIFA96_ERR_NOT_FOUND);
  assert(fifa96_mixer_voice_active(&m, 6) == 1);
  assert(fifa96_mixer_voice_active(&m, 7) == 0);
}

// 0xA77E2..0xA77E4: second arm failure runs the FUN_000a6cdc(id) cleanup, so
// the first voice is stopped (the port's fifa96_mixer_stop analogue).
static void test_second_arm_failure_stops_first(void) {
  uint8_t b[0x2A6];
  build_pair_bank(b, 7);
  b[0x270] = 'X';  /* id 8 embedded EACS magic -> FUN_000a780e returns -10 */
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_sfx_id_table t;
  table_clear(&t);
  t.bank[7] = &info;
  t.bank[8] = &info;
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  fifa96_mixer_init(&m);
  assert(fifa96_sfx_play_id(&m, &t, 7, NULL, p) ==
         -(int)FIFA96_ERR_BAD_MAGIC);
  assert(fifa96_mixer_voice_active(&m, 6) == 0);
  assert(fifa96_mixer_voice_active(&m, 7) == 0);
}

// 0xA775F: flag bit 0 clear -> single arm only, even with slot id+1 present.
static void test_flag_clear_single_only(void) {
  uint8_t b[0x2A6];
  build_pair_bank(b, 7);
  b[0x21C] = 0;
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, sizeof b, &info) == FIFA96_OK);
  struct fifa96_sfx_id_table t;
  table_clear(&t);
  t.bank[7] = &info;
  t.bank[8] = &info;
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  fifa96_mixer_init(&m);
  assert(fifa96_sfx_play_id(&m, &t, 7, NULL, p) == 6);
  assert(fifa96_mixer_voice_active(&m, 6) == 1);
  assert(fifa96_mixer_voice_active(&m, 7) == 0);
}

// Id boundaries 0 and 0x7F with present slots; 0x80 is invalid.
static void test_id_boundaries_present(void) {
  uint8_t b0[0x2A6], b127[0x2A6];
  build_pair_bank(b0, 0);
  b0[0x21C] = 0;  /* single-voice id 0 */
  build_pair_bank(b127, 126);  /* ids 126/127, id 127 flag clear */
  struct fifa96_bnk_info i0, i127;
  assert(fifa96_bnk_parse(b0, sizeof b0, &i0) == FIFA96_OK);
  assert(fifa96_bnk_parse(b127, sizeof b127, &i127) == FIFA96_OK);
  struct fifa96_sfx_id_table t;
  table_clear(&t);
  t.bank[0] = &i0;
  t.bank[127] = &i127;
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  fifa96_mixer_init(&m);
  assert(fifa96_sfx_play_id(&m, &t, 0, NULL, p) == 6);
  assert(fifa96_sfx_play_id(&m, &t, 127, NULL, p) == 7);
  assert(fifa96_sfx_play_id(&m, &t, 128, NULL, p) ==
         -(int)FIFA96_ERR_NOT_FOUND);
}

// The arm's sound-state gate (FU-47 §3, 0xA7852) and opts guards still apply
// through the dispatch.
static void test_gate_and_opts(void) {
  uint8_t *b = NULL;
  size_t n = 0;
  assert(fifa96_file_read("tests/golden/sfx_game.bnk", &b, &n) == 0);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_OK);
  struct fifa96_sfx_id_table t;
  table_clear(&t);
  t.bank[1] = &info;
  struct fifa96_mixer m;
  struct fifa96_sfx_voice p[2];

  fifa96_mixer_init(&m);
  m.sound_state = 0;
  assert(fifa96_sfx_play_id(&m, &t, 1, NULL, p) ==
         -(int)FIFA96_ERR_TRUNCATED);
  m.sound_state = 6;
  assert(fifa96_sfx_play_id(&m, &t, 1, NULL, p) ==
         -(int)FIFA96_ERR_TRUNCATED);
  m.sound_state = 1;
  struct fifa96_sfx_opts bad = {-1, 0x80, 0x7F, NULL, NULL};
  assert(fifa96_sfx_play_id(&m, &t, 1, &bad, p) ==
         -(int)FIFA96_ERR_TRUNCATED);
  free(b);
}

int main(void) {
  test_id_bounds_and_null_slots();
  test_single_voice_golden();
  test_two_voice_packed();
  test_second_descriptor_from_next_slot();
  test_second_voice_absent_leaves_first();
  test_second_arm_failure_stops_first();
  test_flag_clear_single_only();
  test_id_boundaries_present();
  test_gate_and_opts();
  printf("test_sfx_play_id OK\n");
  return 0;
}
