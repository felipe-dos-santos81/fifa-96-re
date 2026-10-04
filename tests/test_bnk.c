// tests/test_bnk.c — SOUND/*.BNK sound-effect bank container (FU-43) and the
// EACS bank entries it carries.
//
// Golden fixture provenance (FU-43 §4, extracted 2026-10-04 from the
// read-only game/FIFAPCCD96.iso with tools/fifa96_bind.iso_files):
//   tests/golden/sfx_game.bnk is byte-identical to the ISO's
//   /SOUND/SFX_GAME.BNK, 173016 bytes (0x2A3D8), sha256
//   fdbdf0972e595f0a. Its 128-slot table holds global ids 1..31 (31 entries:
//   30 mono f9=1 and the single stereo f9=2 id 29), descriptors packed from
//   0x200 with stride 0x48, EACS at descriptor+0x28, payloads from 0xAB8.
//   FU-43's "59 entries" is the 25-bank retail corpus; the committed golden is
//   SFX_GAME alone (31 entries, 30 parse+decode after this slice's parser fix).
// The decode pins below were computed independently (Python) from the fixture
// bytes and the committed FU-39 tables.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_bnk.h"
#include "fifa96_loader/fifa96_eacs.h"
#include "fifa96_loader/fifa96_file.h"

static void put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

// A valid 2-entry bank at b: id 5 (desc 0x200, EACS 0x228, payload 0x2A0 with
// 5 declared nibbles in 3 bytes + 1 alignment gap byte) and id 9 (desc 0x248,
// EACS 0x270, payload 0x2A4 with 4 declared nibbles in 2 bytes). Returns the
// file size 0x2A6.
static size_t build_bank(uint8_t *b) {
  memset(b, 0, 0x2A6);
  put32le(b + 4 * 5, 0x200);
  put32le(b + 4 * 9, 0x248);

  put32le(b + 0x200, 0x40);        /* descriptor id 5: voice mask */
  put32le(b + 0x204, 0x228);       /* embedded EACS offset */
  b[0x214] = 0x64;                 /* priority */
  memcpy(b + 0x228, "EACS", 4);
  put32le(b + 0x22C, 16000);
  b[0x230] = 2; b[0x231] = 1; b[0x232] = 2; b[0x233] = 0xFF;
  put32le(b + 0x234, 5);           /* declared nibbles */
  put32le(b + 0x240, 0x2A0);       /* payload file offset (bank form) */
  b[0x245] = 1;                    /* volume */

  put32le(b + 0x248, 0x800);       /* descriptor id 9: voice mask */
  put32le(b + 0x24C, 0x270);
  b[0x25C] = 0x64;
  memcpy(b + 0x270, "EACS", 4);
  put32le(b + 0x274, 16000);
  b[0x278] = 2; b[0x279] = 1; b[0x27A] = 2; b[0x27B] = 0xFF;
  put32le(b + 0x27C, 4);
  put32le(b + 0x288, 0x2A4);
  b[0x28D] = 1;

  b[0x2A0] = 0x12; b[0x2A1] = 0x34; b[0x2A2] = 0x56;  /* nibbles 1,2,3,4,5 */
  b[0x2A4] = 0x12; b[0x2A5] = 0x34;                   /* nibbles 1,2,3,4 */
  return 0x2A6;
}

// Normalize an entry into the contiguous EACS view fifa96_eacs_parse expects:
// the registrar relocates the bank's absolute +0x18 file offset to a pointer
// (FU-43 §1), and FU-43 §4.1 does the same by copying the 32-byte header,
// rewriting +0x18 to the parser's implicit 0x20 and appending the payload.
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

static void test_parse_synthetic(void) {
  uint8_t b[0x2A6];
  size_t n = build_bank(b);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_OK);
  assert(info.src == b && info.src_len == n);
  assert(info.entry_count == 2);
  assert(info.table[5] == 0x200 && info.table[9] == 0x248);
  assert(info.table[0] == 0 && info.table[127] == 0);

  struct fifa96_bnk_entry e;
  assert(fifa96_bnk_entry(&info, 5, &e) == FIFA96_OK);
  assert(e.id == 5 && e.desc_off == 0x200 && e.eacs_off == 0x228);
  assert(e.payload_off == 0x2A0 && e.payload_len == 4);  /* 3 used + 1 gap */
  assert(fifa96_bnk_entry(&info, 9, &e) == FIFA96_OK);
  assert(e.id == 9 && e.desc_off == 0x248 && e.eacs_off == 0x270);
  assert(e.payload_off == 0x2A4 && e.payload_len == 2);  /* extent to EOF */

  /* absent ids are not found; out-of-range/NULL are bounds errors */
  assert(fifa96_bnk_entry(&info, 0, &e) == FIFA96_ERR_NOT_FOUND);
  assert(fifa96_bnk_entry(&info, 6, &e) == FIFA96_ERR_NOT_FOUND);
  assert(fifa96_bnk_entry(&info, 128, &e) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_bnk_entry(NULL, 5, &e) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_bnk_entry(&info, 5, NULL) == FIFA96_ERR_TRUNCATED);
}

static void test_negative(void) {
  uint8_t b[0x2A6];
  size_t n = build_bank(b);
  struct fifa96_bnk_info info;

  /* NULL / short table / empty table */
  assert(fifa96_bnk_parse(NULL, n, &info) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_bnk_parse(b, n, NULL) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_bnk_parse(b, 0x1FF, &info) == FIFA96_ERR_TRUNCATED);
  uint8_t empty[0x200];
  memset(empty, 0, sizeof empty);
  assert(fifa96_bnk_parse(empty, sizeof empty, &info) == FIFA96_OK);
  assert(info.entry_count == 0);
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_OK);

  /* descriptor offset misaligned / inside the table / crossing EOF */
  put32le(b + 4 * 5, 0x201);
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32le(b + 4 * 5, 0x100);
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32le(b + 4 * 5, (uint32_t)n - 0x47);
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32le(b + 4 * 5, 0x200);

  /* embedded EACS pointer misaligned / crossing EOF */
  put32le(b + 0x204, 0x229);
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32le(b + 0x204, (uint32_t)n - 0x1F);
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32le(b + 0x204, 0x228);

  /* payload offset misaligned / inside the table / past EOF */
  put32le(b + 0x240, 0x2A1);
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32le(b + 0x240, 0x100);
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32le(b + 0x240, (uint32_t)n + 1);
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32le(b + 0x240, 0x2A0);
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_OK);

  /* outside the original's 32-bit file model */
#if SIZE_MAX > UINT32_MAX
  assert(fifa96_bnk_parse(b, (size_t)UINT32_MAX + 1u, &info) == FIFA96_ERR_UNSUPPORTED);
#endif
}

static void test_golden_table(void) {
  uint8_t *b = NULL;
  size_t n = 0;
  assert(fifa96_file_read("tests/golden/sfx_game.bnk", &b, &n) == 0);
  assert(n == 173016);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_OK);
  assert(info.entry_count == 31);

  /* Table monotonicity: ids 1..31 occupy slots 1..31 with descriptors packed
   * 0x200 + 0x48*(id-1); every other slot is empty (FU-43 §1/§4). */
  for (uint32_t id = 0; id < FIFA96_BNK_IDS; id++) {
    if (id >= 1 && id <= 31) {
      assert(info.table[id] == 0x200u + 0x48u * (id - 1));
      if (id > 1) assert(info.table[id] == info.table[id - 1] + 0x48u);
    } else {
      assert(info.table[id] == 0);
    }
  }

  uint32_t mono_units = 0, all_units = 0, stereo = 0, mono = 0;
  uint32_t first_payload = 0;
  uint32_t rate_16000 = 0, rate_16384 = 0, rate_11025 = 0;
  for (uint32_t id = 1; id <= 31; id++) {
    struct fifa96_bnk_entry e;
    assert(fifa96_bnk_entry(&info, id, &e) == FIFA96_OK);
    assert(e.id == id && e.desc_off == info.table[id]);
    assert(e.eacs_off == e.desc_off + 0x28);       /* corpus: entry+4 == +0x28 */
    assert((e.desc_off & 3u) == 0 && (e.eacs_off & 3u) == 0 && (e.payload_off & 3u) == 0);
    assert(e.payload_off >= 0xAB8u && e.payload_off <= n);
    assert((uint64_t)e.payload_off + e.payload_len <= n);
    if (first_payload == 0 || e.payload_off < first_payload) first_payload = e.payload_off;
    assert(memcmp(b + e.eacs_off, "EACS", 4) == 0);
    assert(e.eacs_off + 0x20u == e.desc_off + 0x48u);  /* EACS closes the descriptor */

    size_t vn = 0;
    uint8_t *v = eacs_view(&info, &e, &vn);
    struct fifa96_eacs_info ei;
    assert(fifa96_eacs_parse(v, vn, &ei) == 0);
    assert(ei.voice == -1);                        /* FU-43 §1.2 bank form */
    assert(ei.f8 == 2 && ei.f10 == 2);
    assert(ei.loop_start == 0 && ei.loop_len == 0);  /* SFX_GAME is one-shot */
    assert(ei.delta_units == ei.count);
    uint32_t used = ei.f9 == 1 ? (ei.count + 1u) / 2u : ei.count;
    assert(used <= e.payload_len && e.payload_len - used <= 3);  /* FU-43 §1 gap */
    if (ei.f9 == 1) {
      mono++;
      mono_units += ei.count;
    } else {
      stereo++;
    }
    all_units += ei.count;
    if (ei.rate == 16000) rate_16000++;
    if (ei.rate == 16384) rate_16384++;
    if (ei.rate == 11025) rate_11025++;
    if (id == 29) {
      /* The stereo entry FU-43 §4.1's parser rejected: declared frames at
       * 0x25234, exactly 16929 payload bytes + 3 gap to the next at 0x29458. */
      assert(ei.f9 == 2 && ei.count == 16929 && ei.format == FIFA96_EACS_FMT_DELTA_STEREO);
      assert(e.payload_off == 0x25234u && e.payload_len == 16932u);
    }
    free(v);
  }
  assert(first_payload == 0xAB8u);                 /* pool starts after the EACS */
  assert(mono == 30 && stereo == 1);
  assert(mono_units == 306620);                    /* FU-43 §4.1 */
  assert(all_units == 323549);
  assert(rate_16000 == 6 && rate_16384 == 19 && rate_11025 == 6);
  free(b);
}

static void test_golden_decode(void) {
  uint8_t *b = NULL;
  size_t n = 0;
  assert(fifa96_file_read("tests/golden/sfx_game.bnk", &b, &n) == 0);
  struct fifa96_bnk_info info;
  assert(fifa96_bnk_parse(b, n, &info) == FIFA96_OK);

  /* Independently computed pins: first 8 (L,R) frames, final decoder state and
   * full-stream value range per pinned id (Python, FU-39 §3 tables). */
  static const uint32_t pin_id[4] = {1, 10, 29, 31};
  static const int16_t pin_first[4][8][2] = {
      {{11, 11}, {-19, -19}, {44, 44}, {35, 35}, {43, 43}, {80, 80}, {61, 61}, {92, 92}},
      {{11, 11}, {41, 41}, {104, 104}, {240, 240}, {533, 533}, {1080, 1080}, {1602, 1602}, {1126, 1126}},
      {{-8, -8}, {-11, -11}, {2, 2}, {10, 10}, {9, 9}, {5, 5}, {-1, -1}, {-4, -4}},
      {{11, 11}, {41, 41}, {104, 104}, {240, 240}, {533, 533}, {912, 912}, {1167, 1167}, {1305, 1305}},
  };
  static const int32_t pin_state[4][4] = {
      {1408, 0, -791, 0}, {448, 0, 2, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
  };
  static const int16_t pin_range[4][2] = {
      {-23489, 27191}, {-22032, 26121}, {-32768, 31560}, {-13675, 32360},
  };

  uint64_t total = 0;
  for (uint32_t id = 1; id <= 31; id++) {
    struct fifa96_bnk_entry e;
    assert(fifa96_bnk_entry(&info, id, &e) == FIFA96_OK);
    size_t vn = 0;
    uint8_t *v = eacs_view(&info, &e, &vn);
    struct fifa96_eacs_info ei;
    assert(fifa96_eacs_parse(v, vn, &ei) == 0);
    const uint8_t *data = v + ei.data_off;

    int p = -1;
    for (int k = 0; k < 4; k++)
      if (pin_id[k] == id) p = k;

    struct fifa96_eacs_delta st;
    memset(&st, 0, sizeof st);
    int16_t mn = 0, mx = 0;
    for (uint32_t u = 0; u < ei.delta_units; u++) {
      int16_t l = 0, r = 0;
      assert(fifa96_eacs_delta_unit(&st, ei.f9, data, u, &l, &r) == 0);
      if (l < mn) mn = l;
      if (l > mx) mx = l;
      if (r < mn) mn = r;
      if (r > mx) mx = r;
      if (p >= 0 && u < 8) {
        assert(l == pin_first[p][u][0] && r == pin_first[p][u][1]);
      }
    }
    if (p >= 0) {
      assert((int32_t)st.l_row == pin_state[p][0] && (int32_t)st.r_row == pin_state[p][1]);
      assert(st.l_acc == pin_state[p][2] && st.r_acc == pin_state[p][3]);
      assert(mn == pin_range[p][0] && mx == pin_range[p][1]);
    }
    total += ei.delta_units;
    free(v);
  }
  assert(total == 323549);
  free(b);
}

int main(void) {
  test_parse_synthetic();
  test_negative();
  test_golden_table();
  test_golden_decode();
  printf("test_bnk OK\n");
  return 0;
}
