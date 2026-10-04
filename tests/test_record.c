// tests/test_record.c — record-dispatch routing and literal-copy coverage.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_record.h"

static void decode_golden(const char *in_path, const char *out_path,
                          size_t want_in, size_t want_out) {
  uint8_t *in = 0, *want = 0;
  size_t in_n = 0, want_n = 0;
  assert(fifa96_file_read(in_path, &in, &in_n) == 0);
  assert(fifa96_file_read(out_path, &want, &want_n) == 0);
  assert(in_n == want_in);
  assert(want_n == want_out);
  size_t cap = want_n + 16;
  uint8_t *dst = malloc(cap);
  assert(dst);
  memset(dst, 0xA5, cap);
  size_t got = 0;
  assert(fifa96_record_decode(in, in_n, dst, cap, &got) == 0);
  assert(got == want_n);
  assert(memcmp(dst, want, want_n) == 0);
  for (size_t i = want_n; i < cap; i++) assert(dst[i] == 0xA5);
  free(dst); free(in); free(want);
}

int main(void) {
  /* routed decoders: same committed goldens as the per-decoder tests. */
  decode_golden("tests/golden/vgt/record-10.in.bin",
                "tests/golden/vgt/record-10.out.bin", 17408, 10068);
  decode_golden("tests/golden/vgt/record-30.in.bin",
                "tests/golden/vgt/record-30.out.bin", 17408, 2271);
  decode_golden("tests/golden/vgt/record-46.in.bin",
                "tests/golden/vgt/record-46.out.bin", 17408, 4696);

  uint8_t dst[64];
  size_t got = 0;

  /* literal copy 0x6A and its twin 0x6E: payload at src+5, return = BE24. */
  static const uint8_t lit6a[] = {0x6A, 0xFB, 0x00, 0x00, 0x05, 'A', 'B', 'C', 'D', 'E'};
  static const uint8_t lit6e[] = {0x6E, 0xFB, 0x00, 0x00, 0x05, 'A', 'B', 'C', 'D', 'E'};
  static const uint8_t payload[] = {'A', 'B', 'C', 'D', 'E'};
  memset(dst, 0xA5, sizeof dst);
  assert(fifa96_record_decode(lit6a, sizeof lit6a, dst, sizeof dst, &got) == 0);
  assert(got == 5);
  assert(memcmp(dst, payload, 5) == 0);
  for (size_t i = 5; i < sizeof dst; i++) assert(dst[i] == 0xA5);
  assert(fifa96_record_decode(lit6e, sizeof lit6e, dst, sizeof dst, &got) == 0);
  assert(got == 5);
  assert(memcmp(dst, payload, 5) == 0);

  /* literal copy truncations and capacity error. */
  assert(fifa96_record_decode(lit6a, 4, dst, sizeof dst, &got) < 0);
  assert(fifa96_record_decode(lit6a, 9, dst, sizeof dst, &got) < 0);
  assert(fifa96_record_decode(lit6a, sizeof lit6a, dst, 4, &got) < 0);
  assert(got == 0);

  /* known selectors whose decoders are not ported yet. */
  static const uint8_t unsup[] = {0x00, 0xFB, 0x00, 0x00, 0x00};
  for (unsigned i = 0; i < 6; i++) {
    static const uint8_t sels[6] = {0x16, 0x60, 0x62, 0x66, 0x72, 0x7A};
    uint8_t rec[sizeof unsup];
    memcpy(rec, unsup, sizeof unsup);
    rec[0] = sels[i];
    assert(fifa96_record_decode(rec, sizeof rec, dst, sizeof dst, &got) ==
           -(int)FIFA96_ERR_UNSUPPORTED);
  }

  /* unknown selector and non-0xFB signature. */
  static const uint8_t unknown[] = {0x98, 0xFB, 0x00, 0x00, 0x00};
  assert(fifa96_record_decode(unknown, sizeof unknown, dst, sizeof dst, &got) ==
         -(int)FIFA96_ERR_BAD_MAGIC);
  static const uint8_t badsig[] = {0x10, 0xFA, 0x00, 0x00, 0x00};
  assert(fifa96_record_decode(badsig, sizeof badsig, dst, sizeof dst, &got) ==
         -(int)FIFA96_ERR_BAD_MAGIC);
  assert(got == 0);

  /* NULL and short input. */
  assert(fifa96_record_decode(NULL, 5, dst, sizeof dst, &got) < 0);
  assert(fifa96_record_decode(unknown, sizeof unknown, NULL, sizeof dst, &got) < 0);
  assert(fifa96_record_decode(unknown, sizeof unknown, dst, sizeof dst, NULL) < 0);
  assert(fifa96_record_decode(unknown, 0, dst, sizeof dst, &got) < 0);
  assert(fifa96_record_decode(unknown, 1, dst, sizeof dst, &got) < 0);

  printf("test_record OK\n");
  return 0;
}
