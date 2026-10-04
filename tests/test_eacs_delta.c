// tests/test_eacs_delta.c — FU-39 f10==2 adaptive-delta decoder coverage:
// extracted-table checks, hand-computed stereo/mono blocks, row/accumulator
// persistence across two blocks, error paths, and the real VID_GAME pair.
//
// Fixture provenance (extracted 2026-10-04 from the read-only
// game/FIFAPCCD96.iso with tools/fifa96_bind.iso_files):
//   /VIDEO/VID_GAME.TGV starts at ISO offset 0x12987000 (sha256
//   7680d7c3efd89278f6acae2d857b136b7177d18069b2f06516bf7f99d402cde6); the
//   committed tests/golden/vid_game.tgv is byte-identical to that extent. Its
//   `1SNh` chunk sits at file offset 0x9A14 (total length 0x464);
//   tests/golden/eacs/vid-game-h.eacs is its 0x45C-byte payload — the 32-byte
//   EACS header plus the first 20-byte block header and 1064 packed bytes
//   (sha256 df6463a976b2ce1712eb1a4d8e3ac1ff4c5b606a38a20f3ca2e5bbba53dcd75f).
//   The first `1SNd` chunk is at 0xCE6C (length 0x444);
//   tests/golden/eacs/vid-game-d0.eacs is its 0x43C-byte payload (20-byte
//   block header + 1064 packed bytes, sha256
//   bb98c7e59193a665f0009659aaf4c0becf8ea9328ce504db97372292261e02e5).
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_eacs.h"
#include "fifa96_loader/fifa96_file.h"

static void put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

// Build a 20-byte FU-39 block header: count, L/R row codes, s16 L/R accs.
static void block_hdr(uint8_t *p, uint32_t count, uint32_t l_code, uint32_t r_code,
                      int16_t l_acc, int16_t r_acc) {
  put32le(p, count);
  put32le(p + 4, l_code);
  put32le(p + 8, r_code);
  put32le(p + 0xc, (uint32_t)(uint16_t)l_acc);
  put32le(p + 0x10, (uint32_t)(uint16_t)r_acc);
}

static void test_tables(void) {
  /* FU-39 §3 table facts: DELTA is 89x16 int32 (0x1640 bytes exactly up to
   * ADAPT), ADAPT is 8 int32; every nibble bit-3 pair is anti-symmetric. */
  const int32_t *delta = fifa96_eacs_delta_table();
  const int32_t *adapt = fifa96_eacs_adapt_table();
  assert(delta && adapt);
  static const int32_t row0[16] = {0, 1, 3, 4, 7, 8, 10, 11, 0, -1, -3, -4, -7, -8, -10, -11};
  static const int32_t row88[8] = {4095, 12286, 20478, 28669, 36862, 45053, 53245, 61436};
  static const int32_t want_adapt[8] = {-64, -64, -64, -64, 128, 256, 384, 512};
  for (int n = 0; n < 16; n++) assert(delta[n] == row0[n]);
  for (int n = 0; n < 8; n++) {
    assert(delta[88 * 16 + n] == row88[n]);
    assert(adapt[n] == want_adapt[n]);
  }
  for (int r = 0; r < FIFA96_EACS_DELTA_ROWS; r++)
    for (int n = 0; n < 8; n++)
      assert(delta[r * 16 + n + 8] == -delta[r * 16 + n]);
  /* magnitudes exceed int16 in the last row, so the accumulator clamp bites */
  assert(delta[88 * 16 + 7] == 61436);
}

static void test_stereo_hand_block(void) {
  /* FU-39 §2.3/§3: one byte = one (L,R) frame, high nibble L, low R.
   * Header: count 2, L row code 1 (byte offset 64 = DELTA row 1), R row 0,
   * L acc 100, R acc -100. Data 3A E4:
   *   frame 0: L 3A>>4=3 -> DELTA[1][3]=7 -> 107, row 64+ADAPT[3]=0;
   *            R 3A&15=10 -> DELTA[0][10]=-3 -> -103, row 0+ADAPT[2]->0.
   *   frame 1: L E4>>4=14 -> DELTA[0][14]=-10 -> 97, row 0+ADAPT[6]=384;
   *            R E4&15=4 -> DELTA[0][4]=7 -> -96, row 0+ADAPT[4]=128. */
  uint8_t src[0x14 + 2];
  block_hdr(src, 2, 1, 0, 100, -100);
  src[0x14] = 0x3A;
  src[0x15] = 0xE4;
  struct fifa96_eacs_delta st;
  int16_t out[4] = {0};
  uint32_t units = 0;
  assert(fifa96_eacs_delta_block(src, sizeof src, 2, &st, out, 2, &units) == 0);
  assert(units == 2);
  const int16_t want[4] = {107, -103, 97, -96};
  for (int i = 0; i < 4; i++) assert(out[i] == want[i]);
  assert(st.l_row == 384 && st.r_row == 128 && st.l_acc == 97 && st.r_acc == -96);
}

static void test_stereo_clamp_block(void) {
  /* DELTA row 88 head is (4095,12286,20478,28669,36862,45053,53245,61436,...)
   * with the sign half negated; both accumulators start near full scale, so
   * the decoder's s16 clamp [-32768,32767] must bite on both bounds. */
  uint8_t src[0x14 + 2];
  block_hdr(src, 2, 88, 88, 32000, -32000);
  src[0x14] = 0x70; /* L nibble 7: +61436 -> clamp 32767; R nibble 0: +4095 */
  src[0x15] = 0x8F; /* L nibble 8: -4095; R nibble 15: -61436 -> clamp -32768 */
  struct fifa96_eacs_delta st;
  int16_t out[4] = {0};
  assert(fifa96_eacs_delta_block(src, sizeof src, 2, &st, out, 2, NULL) == 0);
  const int16_t want[4] = {32767, -27905, 28672, -32768};
  for (int i = 0; i < 4; i++) assert(out[i] == want[i]);
  /* ADAPT[0] = -64 pulls R's row to 5568 on byte 0; L (nibble 7, +512) stays
   * at the 0x1600 cap; byte 1 swaps them (R +512 capped, L -64). */
  assert(st.l_row == 5568 && st.r_row == 0x1600);
}

static void test_mono_hand_block(void) {
  /* FU-39 §2.3: one nibble = one mono sample, MSB-first (even unit = high
   * nibble of the byte); the sample is written to both lanes.
   * Header: count 3, L row code 1 (offset 64), L acc 100. Data 3A E4 gives
   * nibbles 3, A, E:
   *   3 -> DELTA[1][3]=7 -> 107, row 64-64=0
   *   A -> DELTA[0][10]=-3 -> 104, row 0-64->0
   *   E -> DELTA[0][14]=-10 -> 94, row 0+384. */
  uint8_t src[0x14 + 2];
  block_hdr(src, 3, 1, 0, 100, 0);
  src[0x14] = 0x3A;
  src[0x15] = 0xE4;
  struct fifa96_eacs_delta st;
  int16_t out[6] = {0};
  uint32_t units = 0;
  assert(fifa96_eacs_delta_block(src, sizeof src, 1, &st, out, 3, &units) == 0);
  assert(units == 3);
  const int16_t want[6] = {107, 107, 104, 104, 94, 94};
  for (int i = 0; i < 6; i++) assert(out[i] == want[i]);
  assert(st.l_row == 384 && st.l_acc == 94);
}

static void test_two_block_persistence(void) {
  /* FU-39 §3/§6: decoder state is re-initialized from the block header at
   * every dequeue, and the stored header of the next block equals the running
   * state. Block 1: count 1, zero state, data 70 -> L nibble 7:
   * DELTA[0][7]=11, row ADAPT[7]=512. Block 2 stores that running state
   * (L code 8 = offset 512, L acc 11, R 0) and data 7F -> L DELTA[8][7]=30 ->
   * 41, R DELTA[0][15]=-11. */
  uint8_t b1[0x14 + 1], b2[0x14 + 1];
  block_hdr(b1, 1, 0, 0, 0, 0);
  b1[0x14] = 0x70;
  block_hdr(b2, 1, 8, 0, 11, 0);
  b2[0x14] = 0x7F;

  struct fifa96_eacs_delta st;
  assert(fifa96_eacs_delta_block(b1, sizeof b1, 2, &st, NULL, 0, NULL) == 0);
  assert(st.l_row == 512 && st.r_row == 0 && st.l_acc == 11 && st.r_acc == 0);

  struct fifa96_eacs_delta st_hdr;
  uint32_t count = 0;
  assert(fifa96_eacs_delta_header(b2, 0x14, &st_hdr, &count) == 0);
  assert(count == 1);
  assert(st_hdr.l_row == st.l_row && st_hdr.r_row == st.r_row &&
         st_hdr.l_acc == st.l_acc && st_hdr.r_acc == st.r_acc);

  int16_t carried[2] = {0};
  assert(fifa96_eacs_delta_unit(&st, 2, b2 + 0x14, 0, &carried[0], &carried[1]) == 0);
  assert(carried[0] == 41 && carried[1] == -11);

  int16_t reinit[2] = {0};
  assert(fifa96_eacs_delta_block(b2, sizeof b2, 2, &st_hdr, reinit, 1, NULL) == 0);
  assert(memcmp(carried, reinit, sizeof carried) == 0);
}

static void test_errors(void) {
  uint8_t src[0x14 + 4];
  block_hdr(src, 5, 0, 0, 0, 0);
  src[0x14] = 0x12; src[0x15] = 0x34; src[0x16] = 0x56; src[0x17] = 0x78;
  struct fifa96_eacs_delta st;
  uint32_t units = 7;
  int16_t out[16];

  /* NULL / short header */
  assert(fifa96_eacs_delta_header(NULL, 0x14, &st, &units) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_delta_header(src, 0x13, &st, &units) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_delta_header(src, 0x14, NULL, &units) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_delta_header(src, 0x14, &st, NULL) == -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_delta_block(NULL, sizeof src, 2, &st, out, 5, &units) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_delta_block(src, sizeof src, 2, NULL, out, 5, &units) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_delta_block(src, 0x13, 2, &st, out, 5, &units) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_delta_unit(NULL, 2, src + 0x14, 0, NULL, NULL) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_delta_unit(&st, 2, NULL, 0, NULL, NULL) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_delta_unit(&st, 0, src + 0x14, 0, NULL, NULL) ==
         -(int)FIFA96_ERR_UNSUPPORTED);
  assert(fifa96_eacs_delta_block(src, sizeof src, 3, &st, NULL, 0, &units) ==
         -(int)FIFA96_ERR_UNSUPPORTED);

  /* stereo count 5 needs 5 bytes, only 4 are present */
  assert(fifa96_eacs_delta_block(src, sizeof src, 2, &st, NULL, 0, &units) ==
         -(int)FIFA96_ERR_TRUNCATED);
  /* mono count 5 needs ceil(5/2)=3 bytes and fits in 4 */
  assert(fifa96_eacs_delta_block(src, sizeof src, 1, &st, NULL, 0, &units) == 0);
  assert(units == 5);
  /* mono count 9 needs 5 bytes, only 4 are present */
  put32le(src, 9);
  assert(fifa96_eacs_delta_block(src, sizeof src, 1, &st, NULL, 0, &units) ==
         -(int)FIFA96_ERR_TRUNCATED);
  /* output capacity: 5 stereo frames do not fit in 4, but NULL output is fine */
  put32le(src, 5);
  uint8_t wide[0x14 + 5];
  block_hdr(wide, 5, 0, 0, 0, 0);
  memset(wide + 0x14, 0, 5);
  assert(fifa96_eacs_delta_block(wide, sizeof wide, 2, &st, out, 4, &units) ==
         -(int)FIFA96_ERR_TRUNCATED);
  assert(fifa96_eacs_delta_block(wide, sizeof wide, 2, &st, NULL, 0, &units) == 0);
  assert(units == 5);
}

static void test_golden_pair(void) {
  uint8_t *h = NULL, *d = NULL;
  size_t hn = 0, dn = 0;
  assert(fifa96_file_read("tests/golden/eacs/vid-game-h.eacs", &h, &hn) == 0);
  assert(fifa96_file_read("tests/golden/eacs/vid-game-d0.eacs", &d, &dn) == 0);
  assert(hn == 0x45C && dn == 0x43C);
  assert(memcmp(h, "EACS", 4) == 0);

  struct fifa96_eacs_info info;
  assert(fifa96_eacs_parse(h, hn, &info) == 0);
  assert(info.rate == 16000 && info.f8 == 2 && info.f9 == 2 && info.f10 == 2);
  assert(info.voice == 0 && info.volume == 0x7F);
  assert(info.count == 620544);            /* declared whole-cue length, FU-39 §6 */
  assert(info.data_off == 0x20 && info.data_len == 0x43C);
  assert(info.block_size == 4 && info.blocks == 271);   /* FU-35 nominal model */
  assert(info.delta_units == 1064);                     /* authoritative count */
  assert(info.format == FIFA96_EACS_FMT_DELTA_STEREO);

  /* The arm zeroes the stored state (FU-39 §2.2); the embedded block header
   * declares d0=1064 decoder units. */
  const uint8_t *hblk = h + info.data_off;
  assert((uint32_t)(hblk[0] | (hblk[1] << 8) | (hblk[2] << 16) | (hblk[3] << 24)) == 1064);
  for (int i = 4; i < 0x14; i++) assert(hblk[i] == 0);

  struct fifa96_eacs_delta st, st0;
  int16_t out[2 * 1064];
  uint32_t units = 0;
  assert(fifa96_eacs_delta_block(hblk, hn - info.data_off, 2, &st, out, 1064, &units) == 0);
  assert(units == 1064);
  /* Hand-checkable opening: byte 0x77 -> DELTA[0][7]=11 both lanes;
   * 0x97 -> L DELTA[40][9]=-126 => 533-126=407. */
  const int16_t want_h[16] = {11, 11, 41, 41, 104, 104, 240, 240,
                              533, 533, 407, 1164, -167, 2521, -1400, 5431};
  for (int i = 0; i < 16; i++) assert(out[i] == want_h[i]);
  assert(out[2 * 1064 - 2] == 24342 && out[2 * 1064 - 1] == 2015);
  assert(st.l_row == 4416 && st.r_row == 3904 && st.l_acc == 24342 && st.r_acc == 2015);

  /* Continuity (FU-39 §6): the next chunk's stored header equals the running
   * state after the header chunk's d0 units. */
  uint32_t count = 0;
  assert(fifa96_eacs_delta_header(d, dn, &st0, &count) == 0);
  assert(count == 1064);
  assert(st0.l_row == st.l_row && st0.r_row == st.r_row &&
         st0.l_acc == st.l_acc && st0.r_acc == st.r_acc);

  /* The first 1SNd block decodes identically whether carried or re-inited. */
  int16_t out_carry[2 * 1064], out_reinit[2 * 1064];
  assert(fifa96_eacs_delta_block(d, dn, 2, &st, out_carry, 1064, &units) == 0);
  assert(fifa96_eacs_delta_block(d, dn, 2, &st0, out_reinit, 1064, &units) == 0);
  assert(memcmp(out_carry, out_reinit, sizeof out_carry) == 0);
  const int16_t want_d[16] = {26350, 2327, 25742, 3747, 22975, 6071, 19453, 8882,
                              16251, 10772, 12509, 11802, 8987, 12738, 6700, 10182};
  for (int i = 0; i < 16; i++) assert(out_carry[i] == want_d[i]);
  assert(st.l_row == 4352 && st.r_row == 3840 && st.l_acc == 20914 && st.r_acc == 10845);

  /* range sanity: the header block stays inside [-24108,24342]; the second
   * block's L lane hits the decoder's -32768 floor (FU-39 §6). */
  int16_t mn = 0, mx = 0, mn2 = 0, mx2 = 0;
  for (int i = 0; i < 2 * 1064; i++) {
    if (out[i] < mn) mn = out[i];
    if (out[i] > mx) mx = out[i];
    if (out_carry[i] < mn2) mn2 = out_carry[i];
    if (out_carry[i] > mx2) mx2 = out_carry[i];
  }
  assert(mn == -24108 && mx == 24342);
  assert(mn2 == -32768 && mx2 == 26350);
  free(h);
  free(d);
}

static void test_golden_accounting(void) {
  /* FU-39 §4.3: the authoritative stream length is the sum of the per-chunk
   * block-header counts; declared == d0 + sum over the 1SNd chunks. */
  uint8_t *tgv = NULL;
  size_t n = 0;
  assert(fifa96_file_read("tests/golden/vid_game.tgv", &tgv, &n) == 0);
  assert(n == 0x7E1014);

  uint32_t declared = 0, d0 = 0, sum = 0, nd = 0;
  int have_h = 0;
  size_t pos = 0;
  while (pos + 8 <= n) {
    uint32_t tag = (uint32_t)tgv[pos] | ((uint32_t)tgv[pos + 1] << 8) |
                   ((uint32_t)tgv[pos + 2] << 16) | ((uint32_t)tgv[pos + 3] << 24);
    uint32_t len = (uint32_t)tgv[pos + 4] | ((uint32_t)tgv[pos + 5] << 8) |
                   ((uint32_t)tgv[pos + 6] << 16) | ((uint32_t)tgv[pos + 7] << 24);
    assert(len >= 8 && pos + len <= n);
    const uint8_t *pay = tgv + pos + 8;
    size_t plen = len - 8;
    if (tag == 0x684E5331u) { /* 1SNh */
      struct fifa96_eacs_info info;
      assert(fifa96_eacs_parse(pay, plen, &info) == 0);
      assert(info.format == FIFA96_EACS_FMT_DELTA_STEREO);
      declared = info.count;
      d0 = info.delta_units;
      have_h = 1;
    } else if (tag == 0x644E5331u) { /* 1SNd */
      uint32_t count = (uint32_t)pay[0] | ((uint32_t)pay[1] << 8) |
                       ((uint32_t)pay[2] << 16) | ((uint32_t)pay[3] << 24);
      assert(count == plen - 0x14);
      sum += count;
      nd++;
    }
    pos += len;
  }
  assert(have_h && nd == 582);
  assert(d0 == 1064);
  assert(declared == d0 + sum);
  assert(declared == 620544);
  free(tgv);
}

int main(void) {
  test_tables();
  test_stereo_hand_block();
  test_stereo_clamp_block();
  test_mono_hand_block();
  test_two_block_persistence();
  test_errors();
  test_golden_pair();
  test_golden_accounting();
  printf("test_eacs_delta OK\n");
  return 0;
}
