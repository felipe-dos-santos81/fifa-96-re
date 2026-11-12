/* port/tests/test_engine_replay_ring.c — recorded-frame replay ring Task 1:
 * the native 0x155AA4 view block and the 0x118 record pack/apply pair.
 *
 * The record map asserted here is spec §5.1 (fresh disassembly of the packer
 * `0x6408C` and the applier `0x63B80`): the byte lanes pre-increment, so the
 * appearance lane starts at record +0xC4 and the frame lane at +0xDB, and the
 * +0x116 stamp is the one asymmetric lane (pack writes the accumulator,
 * unpack writes it into the view's +0x1CE word). */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_match_replay_ring.h"

/* The host is little-endian (the record/view are native byte blocks); the
 * helpers copy bytes, never a cast through an unaligned pointer. */
static void put_u16(uint8_t *block, size_t off, uint16_t value) {
  memcpy(block + off, &value, sizeof value);
}

static void put_u32(uint8_t *block, size_t off, uint32_t value) {
  memcpy(block + off, &value, sizeof value);
}

static uint16_t get_u16(const uint8_t *block, size_t off) {
  uint16_t value;
  memcpy(&value, block + off, sizeof value);
  return value;
}

static uint32_t get_u32(const uint8_t *block, size_t off) {
  uint32_t value;
  memcpy(&value, block + off, sizeof value);
  return value;
}

/* Sign-extend a native word the way the applier's MOVSX does. */
static int32_t sign_extend16(uint16_t word) {
  return (word & 0x8000u) ? (int32_t)word - 0x10000 : (int32_t)word;
}

/* Fill the view so a pack/apply round trip can be exact: the position dwords
 * are sign-extended 16-bit values, the heading dwords are 16-bit values with a
 * zero high word, and every other byte carries a distinct fill byte. */
static void fill_view(uint8_t *view) {
  static const uint32_t focus[3] = {0x11223344u, 0xDEADBEEFu, 0x0BADF00Du};
  static const uint32_t matrix[6] = {0x01020304u, 0x80000000u, 0x7FFFFFFFu,
                                     0xFEDCBA98u, 0x13579BDFu, 0x2468ACE0u};
  size_t i;

  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SIZE; i++)
    view[i] = (uint8_t)(i * 7u + 3u);

  for (i = 0; i < 3; i++) put_u32(view, 4 * i, focus[i]);

  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++) {
    size_t slot = 0x0Cu + 0x0Cu * i;
    int16_t x = (int16_t)(-0x7000 + (int)i * 0x51F);
    int16_t y = (int16_t)(0x2000 - (int)i * 0x2E7);
    int16_t z = (int16_t)((int)i * 0x7F1 - 0x5000);
    put_u32(view, slot + 0x0, (uint32_t)(int32_t)x);
    put_u32(view, slot + 0x4, (uint32_t)(int32_t)y);
    put_u32(view, slot + 0x8, (uint32_t)(int32_t)z);
    put_u32(view, 0x120 + 4 * i, (uint32_t)(uint16_t)(0x1000u + i * 0x3FFu));
  }

  for (i = 0; i < 6; i++) put_u32(view, 0x1AE + 4 * i, matrix[i]);
  put_u16(view, 0x1CC, 0xBEEFu);
  put_u16(view, 0x1CE, 0xAAAAu);
}

/* The §5.1 pack map, lane by lane. */
static void assert_pack_map(const uint8_t *view, const uint8_t *record) {
  size_t i;

  for (i = 0; i < 0x0C; i++) assert(record[i] == view[i]);

  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++) {
    size_t slot = 0x0Cu + 0x0Cu * i;
    assert(get_u16(record, 0x0C + 2 * i) == get_u16(view, slot));          /* x */
    assert(get_u16(record, 0x3A + 2 * i) == get_u16(view, slot + 0x8));    /* z */
    assert(get_u16(record, 0x68 + 2 * i) == get_u16(view, slot + 0x4));    /* y */
    assert(get_u16(record, 0x96 + 2 * i) == get_u16(view, 0x120 + 4 * i)); /* heading */
    assert(record[0xC4 + i] == view[0x17C + i]);
    assert(record[0xDB + i] == view[0x193 + i]);
  }

  assert(memcmp(record + 0xF2, view + 0x1AA, 4) == 0);
  assert(memcmp(record + 0xF6, view + 0x1AE, 0x18) == 0);
  assert(memcmp(record + 0x10E, view + 0x1C6, 6) == 0);
  assert(memcmp(record + 0x114, view + 0x1CC, 2) == 0);
  /* The asymmetric lane: the packer writes the accumulator, not a view byte. */
  assert(record[0x116] == 0x34 && record[0x117] == 0x12);
}

static void test_ring_pack_apply_roundtrip(void) {
  uint8_t view[FIFA96_MATCH_REPLAY_VIEW_SIZE];
  uint8_t record[FIFA96_MATCH_REPLAY_RECORD_SIZE];
  uint8_t applied[FIFA96_MATCH_REPLAY_VIEW_SIZE];
  size_t i;

  fill_view(view);

  /* The filled position words must actually straddle the sign bit, or the
   * sign-extension assertion below proves nothing. */
  assert(sign_extend16(get_u16(view, 0x0C)) < 0);
  assert(sign_extend16(get_u16(view, 0x10)) > 0);
  assert(sign_extend16(get_u16(view, 0x14)) < 0);

  memset(record, 0xCC, sizeof record);
  fifa96_match_replay_pack(view, record, 0x1234u);
  assert_pack_map(view, record);

  memset(applied, 0, sizeof applied);
  fifa96_match_replay_unpack(record, applied);

  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SIZE; i++) {
    if (i == 0x1CE || i == 0x1CF) continue;
    assert(applied[i] == view[i]);
  }
  /* The applier writes the record's stamp into the view's +0x1CE word; the
   * composer's pre-apply value is not restored. */
  assert(applied[0x1CE] == 0x34 && applied[0x1CF] == 0x12);
  assert(get_u16(applied, 0x1CE) == 0x1234u);

  /* Positions: the applier sign-extends the word into the dword. */
  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++) {
    size_t slot = 0x0Cu + 0x0Cu * i;
    assert((int32_t)get_u32(applied, slot + 0x0) ==
           sign_extend16(get_u16(applied, slot + 0x0)));
    assert((int32_t)get_u32(applied, slot + 0x4) ==
           sign_extend16(get_u16(applied, slot + 0x4)));
    assert((int32_t)get_u32(applied, slot + 0x8) ==
           sign_extend16(get_u16(applied, slot + 0x8)));
    /* Headings: the applier writes the word only, leaving the high word. */
    assert(get_u32(applied, 0x120 + 4 * i) ==
           (uint32_t)get_u16(view, 0x120 + 4 * i));
    assert(get_u32(applied, 0x120 + 4 * i) <= 0xFFFFu);
  }
}

int main(void) {
  test_ring_pack_apply_roundtrip();
  puts("test_engine_replay_ring OK");
  return 0;
}
