// tests/test_crd.c — FU-53 CRDF music container. Expected behavior is derived
// in docs/ghidra/FU53_crd_format.md: loader FUN_000a73b2 @ 0xA73B2, producer
// FUN_00065160 @ 0x65160 / FUN_00068fd0 @ 0x68FD0, file layout §2.
//
// Golden fixture provenance: tests/golden/audio/crd-crd0.crd is byte-identical
// to /SOUND/CRD_CRD0.CRD on the read-only game/FIFAPCCD96.iso (ISO9660 LBA
// 7674, ISO byte offset 0xEFD000), 59636 bytes, sha256
// 2bb529abbec31ee39232511f9c4e00ab61ff12311ce6233decfff91081377648. It is the
// only CRDF asset on the disc.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_crd.h"
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_file.h"

#define FIXTURE "tests/golden/audio/crd-crd0.crd"

static void wr32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static uint8_t *read_fixture(size_t *len) {
  uint8_t *b = NULL;
  size_t n = 0;
  assert(fifa96_file_read(FIXTURE, &b, &n) == FIFA96_OK);
  assert(b != NULL);
  *len = n;
  return b;
}

static void test_fixture_parse(void) {
  size_t n = 0;
  uint8_t *b = read_fixture(&n);
  assert(n == 59636);
  assert(b[0] == 'C' && b[1] == 'R' && b[2] == 'D' && b[3] == 'F');
  uint8_t *before = malloc(n);
  assert(before != NULL);
  memcpy(before, b, n);

  struct fifa96_crd c;
  memset(&c, 0xAA, sizeof c);
  assert(fifa96_crd_parse(b, n, &c) == FIFA96_OK);
  assert(c.src == b && c.src_len == n);

  assert(c.header.version == FIFA96_CRD_VERSION);
  assert(c.header.coeff_pos == 0xB334);
  assert(c.header.coeff_ramp == 0x4CCC);
  assert(c.header.coeff_pos + c.header.coeff_ramp == 0x10000);
  assert(c.header.position == 0 && c.header.intensity == 0 && c.header.limit == 0);
  assert(c.header.track_count == 4);
  assert(c.header.event_count == 0x10);
  assert(c.header.limit_scale == 0x96);
  assert(c.header.period_up == 0x20000);
  assert(c.header.period_down == 0x40000);
  assert(c.header.initial_event == 2);
  assert(fifa96_crd_track_count(&c) == 4);
  assert(fifa96_crd_event_count(&c) == 0x10);
  assert(fifa96_crd_track_count(NULL) == 0);
  assert(fifa96_crd_event_count(NULL) == 0);

  static const int32_t tempo_a[FIFA96_CRD_TEMPO_MAX] = {
      0xB333, 0x8000, 0xB333, 0x20000, 0x10000, 0x10000, 0x10000, 0x10000};
  static const int32_t tempo_b[FIFA96_CRD_TEMPO_MAX] = {
      0x38000, 0x40000, 0x190000, 0xA0000, 0x3840000, 0x10000, 0xA0000, 0x50000};
  for (uint32_t i = 0; i < FIFA96_CRD_TEMPO_MAX; i++) {
    struct fifa96_crd_tempo t;
    assert(fifa96_crd_tempo(&c, i, &t) == FIFA96_OK);
    assert(t.a == tempo_a[i] && t.b == tempo_b[i]);
  }

  struct fifa96_crd_event e;
  assert(fifa96_crd_event(&c, 0, &e) == FIFA96_OK);
  assert(e.valid == 1 && e.rate == 0x12C && e.time == 0x28A && e.tempo == 0 && e.id == 0);
  assert(fifa96_crd_event(&c, 4, &e) == FIFA96_OK);
  assert(e.valid == 1 && e.rate == 0x1F4 && e.time == 0x320 && e.tempo == 7 && e.id == 0xF);
  assert(fifa96_crd_event(&c, 15, &e) == FIFA96_OK);
  assert(e.valid == 1 && e.rate == 0x96 && e.time == 0x258 && e.tempo == 0 && e.id == 0);

  struct fifa96_crd_track t;
  assert(fifa96_crd_track(&c, 0, &t) == FIFA96_OK);
  assert(t.eacs_off == 0xA4 && t.data_off == 0x38C && t.data_len == 0x40C8);
  assert(t.rate == 0x5622 && t.f8 == 1 && t.f9 == 1 && t.f10 == 0 && t.voice == -1);
  assert(t.sample_count == 0x40C8 && t.loop_start == 5 && t.loop_len == 0x40BF);
  assert(t.volume == 0);
  assert(fifa96_crd_track(&c, 1, &t) == FIFA96_OK);
  assert(t.eacs_off == 0x118 && t.data_off == 0x4454 && t.data_len == 0x2730);
  assert(t.rate == 0x3E80 && t.sample_count == 0x272E && t.loop_len == 0x21AD);
  assert(fifa96_crd_track(&c, 2, &t) == FIFA96_OK);
  assert(t.eacs_off == 0x18C && t.data_off == 0x6B84 && t.data_len == 0x4B04);
  assert(t.sample_count == 0x4B02 && t.loop_start == 0x3C && t.loop_len == 0x4A6F);
  assert(fifa96_crd_track(&c, 3, &t) == FIFA96_OK);
  assert(t.eacs_off == 0x200 && t.data_off == 0xB688 && t.data_len == 0x326C);
  assert(t.rate == 0x3E80 && t.sample_count == 0x326A);
  assert(t.loop_start == 0x10F && t.loop_len == 0x2FA2);

  assert(memcmp(b, before, n) == 0);
  free(before);
  fifa96_file_free(b);
}

static void test_fixture_mutations(void) {
  size_t n = 0;
  uint8_t *orig = read_fixture(&n);
  uint8_t *b = malloc(n);
  assert(b != NULL);
  memcpy(b, orig, n);

  struct fifa96_crd c;
  memset(&c, 0xAA, sizeof c);
  assert(fifa96_crd_parse(NULL, n, &c) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_parse(b, n, NULL) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_parse(b, 0, &c) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_parse(b, 3, &c) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_parse(b, 4, &c) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_parse(b, FIFA96_CRD_HEADER_SIZE - 1, &c) == -FIFA96_ERR_TRUNCATED);

  memset(&c, 0xAA, sizeof c);
  b[0] = 'X';
  assert(fifa96_crd_parse(b, n, &c) == -FIFA96_ERR_BAD_MAGIC);
  assert(c.header.track_count == 0xAAAAAAAA);
  b[0] = 'C';

  wr32(b + 4, 3);
  assert(fifa96_crd_parse(b, n, &c) == -FIFA96_ERR_UNSUPPORTED);
  wr32(b + 4, 0);
  assert(fifa96_crd_parse(b, n, &c) == -FIFA96_ERR_UNSUPPORTED);
  wr32(b + 4, FIFA96_CRD_VERSION);

  wr32(b + 0x24, FIFA96_CRD_TRACK_MAX + 1);
  assert(fifa96_crd_parse(b, n, &c) == -FIFA96_ERR_TRUNCATED);
  wr32(b + 0x24, 4);

  wr32(b + 0x28, FIFA96_CRD_EVENT_MAX + 1);
  assert(fifa96_crd_parse(b, n, &c) == -FIFA96_ERR_TRUNCATED);
  wr32(b + 0x28, 0x10);

  b[0x18C] = 'X';
  assert(fifa96_crd_parse(b, n, &c) == -FIFA96_ERR_BAD_MAGIC);
  b[0x18C] = 'E';

  wr32(b + 0xBC, 0x38B);
  assert(fifa96_crd_parse(b, n, &c) == -FIFA96_ERR_TRUNCATED);
  wr32(b + 0xBC, (uint32_t)n + 1);
  assert(fifa96_crd_parse(b, n, &c) == -FIFA96_ERR_TRUNCATED);
  wr32(b + 0xBC, 0x38C);

  wr32(b + 0x20C, (uint32_t)n);
  assert(fifa96_crd_parse(b, n, &c) == -FIFA96_ERR_TRUNCATED);
  wr32(b + 0x20C, 0x326A);

  wr32(b + 0xBC, 0x4454);
  wr32(b + 0x130, 0x38C);
  assert(fifa96_crd_parse(b, n, &c) == -FIFA96_ERR_TRUNCATED);
  wr32(b + 0xBC, 0x38C);
  wr32(b + 0x130, 0x4454);

  assert(fifa96_crd_parse(b, 0xE8F2, &c) == FIFA96_OK);
  assert(fifa96_crd_parse(b, 0xE8F1, &c) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_parse(b, n + 7, &c) == FIFA96_OK);

  assert(memcmp(b, orig, n) == 0);
  free(b);
  fifa96_file_free(orig);
}

static void test_accessor_bounds(void) {
  size_t n = 0;
  uint8_t *b = read_fixture(&n);
  struct fifa96_crd c;
  assert(fifa96_crd_parse(b, n, &c) == FIFA96_OK);

  struct fifa96_crd_track t;
  struct fifa96_crd_event e;
  struct fifa96_crd_tempo m;
  for (uint32_t i = 0; i < FIFA96_CRD_TRACK_MAX; i++)
    assert(fifa96_crd_track(&c, i, &t) == FIFA96_OK);
  assert(fifa96_crd_track(&c, FIFA96_CRD_TRACK_MAX, &t) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_track(&c, 0xFFFFFFFFu, &t) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_track(NULL, 0, &t) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_track(&c, 0, NULL) == -FIFA96_ERR_TRUNCATED);

  for (uint32_t i = 0; i < FIFA96_CRD_EVENT_MAX; i++)
    assert(fifa96_crd_event(&c, i, &e) == FIFA96_OK);
  assert(fifa96_crd_event(&c, FIFA96_CRD_EVENT_MAX, &e) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_event(&c, 0xFFFFFFFFu, &e) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_event(NULL, 0, &e) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_event(&c, 0, NULL) == -FIFA96_ERR_TRUNCATED);

  for (uint32_t i = 0; i < FIFA96_CRD_TEMPO_MAX; i++)
    assert(fifa96_crd_tempo(&c, i, &m) == FIFA96_OK);
  assert(fifa96_crd_tempo(&c, FIFA96_CRD_TEMPO_MAX, &m) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_tempo(&c, 0xFFFFFFFFu, &m) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_tempo(NULL, 0, &m) == -FIFA96_ERR_TRUNCATED);
  assert(fifa96_crd_tempo(&c, 0, NULL) == -FIFA96_ERR_TRUNCATED);

  fifa96_file_free(b);
}

int main(void) {
  test_fixture_parse();
  test_fixture_mutations();
  test_accessor_bounds();
  printf("test_crd OK\n");
  return 0;
}
