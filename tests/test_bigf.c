// tests/test_bigf.c — BIGF v2 bank container (FU-41 §1) and the EACS bank
// entries it carries (FU-41 §2).
//
// Fixture provenance (extracted 2026-10-04 from the read-only
// game/FIFAPCCD96.iso with tools/fifa96_bind.iso_files):
//   /SOUND/TEM_T019.VIV lives at ISO offset 0xF2C8800, size 36796 (0x8FBC),
//   sha256 dcb3e2d773be9e7e4643ade8fbbbb40d94ac298b839141c69b4d947a494fd832;
//   tests/golden/eacs/bank-t019.viv is byte-identical to that extent.
//   Header: size field 0x8FBC, count 6, table_end 0x90. All six records are
//   EACS bank-form entries (FU-41 §2): rate 16000, f8=2, f9=1, f10=2,
//   voice -1, +0x18 == 0x20, loop (-1, 0), volume 1, declared nibbles with
//   3-6 nibbles of trailing payload slack (FU-35 §6.1). Every record name is
//   12 chars (stride 21); the walk ends at 0x8E with two pad bytes (0B DB)
//   before table_end 0x90. Records:
//     [0] tmt01901.spc off 0x0090 size 4404 decl  8741 slack 3
//     [1] tmt01910.spc off 0x11C4 size 5284 decl 10501 slack 3
//     [2] tmt01920.spc off 0x2668 size 7456 decl 14842 slack 6
//     [3] tmt01930.spc off 0x4388 size 5828 decl 11586 slack 6
//     [4] tmt01940.spc off 0x5A4C size 5884 decl 11701 slack 3
//     [5] tmt01950.spc off 0x7148 size 7796 decl 15523 slack 5
//   The entry-0 decode pin was computed independently (Python) from the
//   fixture bytes and the committed FU-39 tables.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_bigf.h"
#include "fifa96_loader/fifa96_eacs.h"
#include "fifa96_loader/fifa96_file.h"

static void put32be(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
  p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)v;
}

// A valid 2-record bank at b: table 0x10..0x30 (records at 0x10 and 0x1E),
// data at 0x30 (4 bytes) and 0x34 (2 bytes), declared size 0x36.
static size_t build_bank(uint8_t *b) {
  memset(b, 0, 0x40);
  memcpy(b, "BIGF", 4);
  put32be(b + 4, 0x36);       /* file size */
  put32be(b + 8, 2);          /* entry count */
  put32be(b + 0x0C, 0x30);    /* directory end / first data offset */
  put32be(b + 0x10, 0x30);    /* record 0: offset */
  put32be(b + 0x14, 4);       /* record 0: size */
  memcpy(b + 0x18, "a.spc", 6);
  put32be(b + 0x1E, 0x34);    /* record 1: offset */
  put32be(b + 0x22, 2);       /* record 1: size */
  memcpy(b + 0x26, "bb", 3);
  memcpy(b + 0x30, "WXYZ", 4);
  b[0x34] = 0x12;
  b[0x35] = 0x34;
  return 0x36;
}

static void test_parse_synthetic(void) {
  uint8_t b[0x40];
  size_t n = build_bank(b);
  struct fifa96_bigf_info info;
  assert(fifa96_bigf_parse(b, n, &info) == FIFA96_OK);
  assert(info.src == b && info.src_len == n);
  assert(info.size == n && info.count == 2 && info.table_end == 0x30);

  uint32_t off = 0, size = 0;
  const char *name = NULL;
  assert(fifa96_bigf_record(&info, 0, &off, &size, &name) == FIFA96_OK);
  assert(off == 0x30 && size == 4 && strcmp(name, "a.spc") == 0);
  assert(fifa96_bigf_record(&info, 1, &off, &size, &name) == FIFA96_OK);
  assert(off == 0x34 && size == 2 && strcmp(name, "bb") == 0);

  /* every output is optional */
  assert(fifa96_bigf_record(&info, 1, NULL, NULL, &name) == FIFA96_OK);
  assert(strcmp(name, "bb") == 0);
  assert(fifa96_bigf_record(&info, 0, &off, &size, NULL) == FIFA96_OK);
  assert(off == 0x30 && size == 4);
}

static void test_zero_entries(void) {
  uint8_t b[0x10];
  memset(b, 0, sizeof b);
  memcpy(b, "BIGF", 4);
  put32be(b + 4, 0x10);
  put32be(b + 8, 0);
  put32be(b + 0x0C, 0x10);
  struct fifa96_bigf_info info;
  assert(fifa96_bigf_parse(b, sizeof b, &info) == FIFA96_OK);
  assert(info.count == 0 && info.table_end == 0x10);
  assert(fifa96_bigf_record(&info, 0, NULL, NULL, NULL) == FIFA96_ERR_TRUNCATED);
}

static void test_negative(void) {
  uint8_t b[0x40];
  size_t n = build_bank(b);
  struct fifa96_bigf_info info;
  uint32_t off = 0, size = 0;
  const char *name = NULL;

  /* NULL / short header / bad magic */
  assert(fifa96_bigf_parse(NULL, n, &info) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_bigf_parse(b, n, NULL) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_bigf_parse(b, 0x0F, &info) == FIFA96_ERR_TRUNCATED);
  memcpy(b, "BAGF", 4);
  assert(fifa96_bigf_parse(b, n, &info) == FIFA96_ERR_BAD_MAGIC);
  memcpy(b, "BIGF", 4);

  /* declared file size must equal the buffer length */
  put32be(b + 4, (uint32_t)n + 1);
  assert(fifa96_bigf_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32be(b + 4, (uint32_t)n);

  /* table end below the header / past the buffer */
  put32be(b + 0x0C, 0x0F);
  assert(fifa96_bigf_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32be(b + 0x0C, (uint32_t)n + 1);
  assert(fifa96_bigf_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32be(b + 0x0C, 0x30);

  /* count does not fit the directory */
  put32be(b + 8, 0xFF);
  assert(fifa96_bigf_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32be(b + 8, 2);

  /* last name has no NUL before table_end */
  put32be(b + 0x0C, 0x27);
  assert(fifa96_bigf_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32be(b + 0x0C, 0x30);

  /* record offset/size overruns the file */
  put32be(b + 0x14, (uint32_t)n);
  assert(fifa96_bigf_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32be(b + 0x14, 4);
  put32be(b + 0x10, (uint32_t)n - 1);
  assert(fifa96_bigf_parse(b, n, &info) == FIFA96_ERR_TRUNCATED);
  put32be(b + 0x10, 0x30);

  /* accessor errors: NULL info, index past count */
  assert(fifa96_bigf_record(NULL, 0, &off, &size, &name) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_bigf_parse(b, n, &info) == FIFA96_OK);
  assert(fifa96_bigf_record(&info, 2, &off, &size, &name) == FIFA96_ERR_TRUNCATED);

  /* a hand-built info (not from parse) with an out-of-range table end is
   * rejected instead of underflowing the name-bounds length */
  struct fifa96_bigf_info bad;
  memset(&bad, 0, sizeof bad);
  bad.src = b;
  bad.src_len = n;
  bad.count = 1;
  bad.table_end = 0;                                /* below the 0x10 header */
  assert(fifa96_bigf_record(&bad, 0, &off, &size, &name) == FIFA96_ERR_TRUNCATED);
  bad.table_end = (uint32_t)n + 1;                  /* past the buffer */
  assert(fifa96_bigf_record(&bad, 0, &off, &size, &name) == FIFA96_ERR_TRUNCATED);
}

static const struct golden_rec {
  const char *name;
  uint32_t off, size, decl, slack;
} k_recs[6] = {
  {"tmt01901.spc", 0x0090, 4404, 8741, 3},
  {"tmt01910.spc", 0x11C4, 5284, 10501, 3},
  {"tmt01920.spc", 0x2668, 7456, 14842, 6},
  {"tmt01930.spc", 0x4388, 5828, 11586, 6},
  {"tmt01940.spc", 0x5A4C, 5884, 11701, 3},
  {"tmt01950.spc", 0x7148, 7796, 15523, 5},
};

static void test_golden_bank(void) {
  uint8_t *b = NULL;
  size_t n = 0;
  assert(fifa96_file_read("tests/golden/eacs/bank-t019.viv", &b, &n) == 0);
  assert(n == 0x8FBC);
  struct fifa96_bigf_info info;
  assert(fifa96_bigf_parse(b, n, &info) == FIFA96_OK);
  assert(info.src == b && info.size == 0x8FBC);
  assert(info.count == 6 && info.table_end == 0x90);
  assert(info.table_end == 0x10 + 6 * 21 + 2);   /* six 12-char names + 2 pad */

  uint32_t units_total = 0;
  for (uint32_t i = 0; i < info.count; i++) {
    uint32_t off = 0, size = 0;
    const char *name = NULL;
    assert(fifa96_bigf_record(&info, i, &off, &size, &name) == FIFA96_OK);

    /* name pointer and NUL both inside the directory bounds */
    assert(name >= (const char *)b);
    assert(name < (const char *)b + info.table_end);
    assert(memchr(name, 0, info.table_end - (size_t)(name - (const char *)b)) != NULL);
    assert(strcmp(name, k_recs[i].name) == 0);
    assert(off == k_recs[i].off && size == k_recs[i].size);
    if (i > 0) assert(k_recs[i - 1].off + k_recs[i - 1].size == off); /* contiguous */
    assert((uint64_t)off + size <= n);

    struct fifa96_eacs_info e;
    assert(fifa96_eacs_parse(b + off, size, &e) == 0);
    assert(e.rate == 16000);
    assert(e.f8 == 2 && e.f9 == 1 && e.f10 == 2);
    assert(e.voice == -1);                       /* FU-41 §2 bank form */
    assert(e.count == k_recs[i].decl);
    assert(e.loop_start == -1 && e.loop_len == 0);
    assert(e.data_ptr == 0x20 && e.data_off == 0x20);   /* +0x18 relocation */
    assert(e.volume == 1);
    assert(e.block_size == 2 && e.blocks == e.data_len / 2);
    assert(e.format == FIFA96_EACS_FMT_DELTA_MONO);
    assert(e.delta_units == e.count);
    assert(e.data_len == size - 0x20);
    assert(2u * e.data_len - e.count == k_recs[i].slack);  /* trailing slack */
    assert(k_recs[i].slack <= 7);                          /* FU-35 §6.1 */
    assert((e.delta_units + 1) / 2 <= e.data_len);         /* nibble packing */
    units_total += e.delta_units;
  }
  assert(units_total == 72894);
  free(b);
}

static void test_golden_decode(void) {
  uint8_t *b = NULL;
  size_t n = 0;
  assert(fifa96_file_read("tests/golden/eacs/bank-t019.viv", &b, &n) == 0);
  struct fifa96_bigf_info info;
  assert(fifa96_bigf_parse(b, n, &info) == FIFA96_OK);
  uint32_t off = 0, size = 0;
  const char *name = NULL;
  assert(fifa96_bigf_record(&info, 0, &off, &size, &name) == FIFA96_OK);

  struct fifa96_eacs_info e;
  assert(fifa96_eacs_parse(b + off, size, &e) == 0);
  assert(e.delta_units == 8741 && e.data_len == 4372);
  const uint8_t *data = b + off + e.data_off;

  /* Unit-exact decode of the whole declared stream from zero state (FU-39
   * §2.2: the bank arm skips the block header and plays +0x0C nibbles). */
  struct fifa96_eacs_delta st;
  memset(&st, 0, sizeof st);
  static const int16_t want_first[8] = {11, 41, 104, 240, 533, 912, 963, 917};
  int16_t l = 0, r = 0;
  for (uint32_t u = 0; u < e.delta_units; u++) {
    assert(fifa96_eacs_delta_unit(&st, e.f9, data, u, &l, &r) == 0);
    assert(l == r);                              /* mono duplicated to both lanes */
    if (u < 8) assert(l == want_first[u]);
  }
  assert(l == -95 && st.l_acc == -95 && st.l_row == 0x900);
  assert((e.delta_units + 1) / 2 == 4371);       /* bytes actually consumed */
  assert(2u * e.data_len - e.delta_units == 3);  /* unused trailing nibbles */
  free(b);
}

int main(void) {
  test_parse_synthetic();
  test_zero_entries();
  test_negative();
  test_golden_bank();
  test_golden_decode();
  printf("test_bigf OK\n");
  return 0;
}
