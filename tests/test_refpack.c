// tests/test_refpack.c — golden and negative-path coverage for the RefPack decoder.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_refpack.h"

int main(void) {
  uint8_t *in = 0, *want = 0;
  size_t in_n = 0, want_n = 0;
  assert(fifa96_file_read("tests/golden/vgt/record-10.in.bin", &in, &in_n) == 0);
  assert(fifa96_file_read("tests/golden/vgt/record-10.out.bin", &want, &want_n) == 0);
  assert(in_n == 17408);
  assert(want_n == 10068);

  /* golden decode: capacity above out_len leaves the overrun region untouched. */
  size_t cap = want_n + 16;
  uint8_t *dst = malloc(cap);
  assert(dst);
  memset(dst, 0xA5, cap);
  size_t got = 0;
  assert(fifa96_refpack_decode(in, in_n, dst, cap, &got) == 0);
  assert(got == want_n);
  assert(memcmp(dst, want, want_n) == 0);
  for (size_t i = want_n; i < cap; i++) assert(dst[i] == 0xA5);  /* final overrun clamped */

  /* exact declared capacity is accepted. */
  uint8_t *exact = malloc(want_n);
  assert(exact);
  assert(fifa96_refpack_decode(in, in_n, exact, want_n, &got) == 0);
  assert(got == want_n);
  assert(memcmp(exact, want, want_n) == 0);

  /* destination capacity below out_len is an error and writes nothing. */
  memset(dst, 0xA5, cap);
  assert(fifa96_refpack_decode(in, in_n, dst, want_n - 1, &got) < 0);
  assert(got == 0);
  for (size_t i = 0; i < cap; i++) assert(dst[i] == 0xA5);

  /* NULL arguments. */
  assert(fifa96_refpack_decode(NULL, in_n, dst, cap, &got) < 0);
  assert(fifa96_refpack_decode(in, in_n, NULL, cap, &got) < 0);
  assert(fifa96_refpack_decode(in, in_n, dst, cap, NULL) < 0);

  /* zero-length / incomplete-header input. */
  assert(fifa96_refpack_decode(in, 0, dst, cap, &got) < 0);
  assert(fifa96_refpack_decode(in, 4, dst, cap, &got) < 0);

  /* declared-zero stream: header + immediate stop, decodes to nothing. */
  static const uint8_t empty[] = {0x10, 0xFB, 0x00, 0x00, 0x00, 0xFC};
  assert(fifa96_refpack_decode(empty, sizeof empty, dst, cap, &got) == 0);
  assert(got == 0);

  /* truncated inside a command: class A control cut before its literals. */
  static const uint8_t cut_a[] = {0x10, 0xFB, 0x00, 0x00, 0x02, 0x03, 0x00};
  assert(fifa96_refpack_decode(cut_a, sizeof cut_a, dst, cap, &got) < 0);
  /* truncated class D literal payload. */
  static const uint8_t cut_d[] = {0x10, 0xFB, 0x00, 0x00, 0x04, 0xE0, 0x11, 0x22};
  assert(fifa96_refpack_decode(cut_d, sizeof cut_d, dst, cap, &got) < 0);
  /* truncated golden prefix (cut mid-command, far before the stop). */
  assert(fifa96_refpack_decode(in, 100, dst, cap, &got) < 0);

  /* complete command but the stream never reaches a 0xFC..0xFF stop code. */
  static const uint8_t nostop[] = {0x10, 0xFB, 0x00, 0x00, 0x04, 0xE0, 0x11, 0x22, 0x33, 0x44};
  assert(fifa96_refpack_decode(nostop, sizeof nostop, dst, cap, &got) < 0);

  /* match distance reaching before the start of output is rejected. */
  static const uint8_t bad_dist[] = {0x10, 0xFB, 0x00, 0x00, 0x03, 0x00, 0x01, 0xFC};
  assert(fifa96_refpack_decode(bad_dist, sizeof bad_dist, dst, cap, &got) < 0);

  /* raw 0x11 header form: bit0 skips 3 extra header bytes (FU-22 §8, no vector). */
  static const uint8_t raw11[] = {0x11, 0xFB, 0xAA, 0xBB, 0xCC, 0x00, 0x00, 0x00, 0xFC};
  assert(fifa96_refpack_decode(raw11, sizeof raw11, dst, cap, &got) == 0);
  assert(got == 0);

  /* class C: formula-derived command (FU-22 §10 open leg, no vector coverage). */
  static const uint8_t class_c[] = {
      0x10, 0xFB, 0x00, 0x01, 0x09,        /* declared 265 */
      0xE0, 0x11, 0x22, 0x33, 0x44,        /* class D: 4 literals */
      0xC4, 0x00, 0x03, 0x00,              /* class C: dist 3, len 261 */
      0xFC};                               /* stop */
  static const uint8_t lit4[4] = {0x11, 0x22, 0x33, 0x44};
  assert(fifa96_refpack_decode(class_c, sizeof class_c, dst, cap, &got) == 0);
  assert(got == 265);
  for (size_t i = 0; i < got; i++) assert(dst[i] == lit4[i % 4]);

  free(dst); free(exact); free(in); free(want);
  printf("test_refpack OK\n");
  return 0;
}
