// tests/test_huff.c — golden and negative-path coverage for the huff decoder.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_huff.h"

int main(void) {
  uint8_t *in = 0, *want = 0;
  size_t in_n = 0, want_n = 0;
  assert(fifa96_file_read("tests/golden/vgt/record-30.in.bin", &in, &in_n) == 0);
  assert(fifa96_file_read("tests/golden/vgt/record-30.out.bin", &want, &want_n) == 0);
  assert(in_n == 17408);
  assert(want_n == 2271);
  assert(in[0] == 0x31);  /* raw selector; 8-byte header form */

  /* golden decode: capacity above out_len leaves the tail region untouched. */
  size_t cap = want_n + 16;
  uint8_t *dst = malloc(cap);
  assert(dst);
  memset(dst, 0xA5, cap);
  size_t got = 0;
  assert(fifa96_huff_decode(in, in_n, dst, cap, &got) == 0);
  assert(got == want_n);
  assert(memcmp(dst, want, want_n) == 0);
  for (size_t i = want_n; i < cap; i++) assert(dst[i] == 0xA5);

  /* exact declared capacity is accepted. */
  uint8_t *exact = malloc(want_n);
  assert(exact);
  assert(fifa96_huff_decode(in, in_n, exact, want_n, &got) == 0);
  assert(got == want_n);
  assert(memcmp(exact, want, want_n) == 0);

  /* destination capacity below out_len is an error and writes nothing. */
  memset(dst, 0xA5, cap);
  assert(fifa96_huff_decode(in, in_n, dst, want_n - 1, &got) < 0);
  assert(got == 0);
  for (size_t i = 0; i < cap; i++) assert(dst[i] == 0xA5);

  /* NULL arguments. */
  assert(fifa96_huff_decode(NULL, in_n, dst, cap, &got) < 0);
  assert(fifa96_huff_decode(in, in_n, NULL, cap, &got) < 0);
  assert(fifa96_huff_decode(in, in_n, dst, cap, NULL) < 0);

  /* zero-length / incomplete-header input. */
  assert(fifa96_huff_decode(in, 0, dst, cap, &got) < 0);
  assert(fifa96_huff_decode(in, 5, dst, cap, &got) < 0);
  assert(fifa96_huff_decode(in, 8, dst, cap, &got) < 0);

  /* truncated golden prefix (bits run out well before the end marker). */
  assert(fifa96_huff_decode(in, 100, dst, cap, &got) < 0);

  /* declared-size bomb: dst_cap can never cover 0xFFFFFF. */
  uint8_t *bomb = malloc(in_n);
  assert(bomb);
  memcpy(bomb, in, in_n);
  bomb[5] = 0xFF; bomb[6] = 0xFF; bomb[7] = 0xFF;
  assert(fifa96_huff_decode(bomb, in_n, dst, cap, &got) < 0);
  assert(got == 0);

  /* FU-24 §8 variants: raw 0x33 -> 0x32FB (prefix sum), raw 0x35 -> 0x34FB
     (double prefix sum). Header form and bitstream are unchanged by the
     selector low bits, so the expected output is derived from golden. */
  uint8_t *var = malloc(in_n);
  assert(var);
  memcpy(var, in, in_n);
  var[0] = 0x33;
  memset(dst, 0xA5, cap);
  assert(fifa96_huff_decode(var, in_n, dst, cap, &got) == 0);
  assert(got == want_n);
  uint8_t acc = 0;
  for (size_t i = 0; i < want_n; i++) {
    acc = (uint8_t)(acc + want[i]);
    assert(dst[i] == acc);
  }
  for (size_t i = want_n; i < cap; i++) assert(dst[i] == 0xA5);
  var[0] = 0x35;
  memset(dst, 0xA5, cap);
  assert(fifa96_huff_decode(var, in_n, dst, cap, &got) == 0);
  assert(got == want_n);
  uint8_t first = 0, second = 0;
  for (size_t i = 0; i < want_n; i++) {
    first = (uint8_t)(first + want[i]);
    second = (uint8_t)(second + first);
    assert(dst[i] == second);
  }
  for (size_t i = want_n; i < cap; i++) assert(dst[i] == 0xA5);

  free(dst); free(exact); free(bomb); free(var); free(in); free(want);
  printf("test_huff OK\n");
  return 0;
}
