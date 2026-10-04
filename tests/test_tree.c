// tests/test_tree.c — golden and negative-path coverage for the tree decoder.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_tree.h"

int main(void) {
  uint8_t *in = 0, *want = 0;
  size_t in_n = 0, want_n = 0;
  assert(fifa96_file_read("tests/golden/vgt/record-46.in.bin", &in, &in_n) == 0);
  assert(fifa96_file_read("tests/golden/vgt/record-46.out.bin", &want, &want_n) == 0);
  assert(in_n == 17408);
  assert(want_n == 4696);
  assert(in[0] == 0x46);
  assert(in[6] == 110);  /* entry count */

  /* golden decode: capacity above out_len leaves the tail region untouched. */
  size_t cap = want_n + 16;
  uint8_t *dst = malloc(cap);
  assert(dst);
  memset(dst, 0xA5, cap);
  size_t got = 0;
  assert(fifa96_tree_decode(in, in_n, dst, cap, &got) == 0);
  assert(got == want_n);
  assert(memcmp(dst, want, want_n) == 0);
  for (size_t i = want_n; i < cap; i++) assert(dst[i] == 0xA5);

  /* exact declared capacity is accepted. */
  uint8_t *exact = malloc(want_n);
  assert(exact);
  assert(fifa96_tree_decode(in, in_n, exact, want_n, &got) == 0);
  assert(got == want_n);
  assert(memcmp(exact, want, want_n) == 0);

  /* destination capacity below out_len is an error and writes nothing. */
  memset(dst, 0xA5, cap);
  assert(fifa96_tree_decode(in, in_n, dst, want_n - 1, &got) < 0);
  assert(got == 0);
  for (size_t i = 0; i < cap; i++) assert(dst[i] == 0xA5);

  /* NULL arguments. */
  assert(fifa96_tree_decode(NULL, in_n, dst, cap, &got) < 0);
  assert(fifa96_tree_decode(in, in_n, NULL, cap, &got) < 0);
  assert(fifa96_tree_decode(in, in_n, dst, cap, NULL) < 0);

  /* zero-length / incomplete header. */
  assert(fifa96_tree_decode(in, 0, dst, cap, &got) < 0);
  assert(fifa96_tree_decode(in, 1, dst, cap, &got) < 0);
  assert(fifa96_tree_decode(in, 6, dst, cap, &got) < 0);

  /* truncated entry table: 2 + 5 + 3*110 bytes are required. */
  assert(fifa96_tree_decode(in, 336, dst, cap, &got) < 0);
  /* entry table complete but the stream never reaches its terminator. */
  assert(fifa96_tree_decode(in, 2294, dst, cap, &got) < 0);

  /* cyclic expansion: key 0x21's childA is itself; must error, not recurse
     forever (the original has no depth guard). */
  static const uint8_t cycle[] = {
      0x46, 0xFB,              /* magic (not 0x47FB) -> payload base +2 */
      0x00, 0x00, 0x10,        /* declared 16 */
      0x10,                    /* root */
      0x01,                    /* count 1 */
      0x21, 0x21, 0x21,        /* key 0x21 -> childA 0x21, childB 0x21 */
      0x21,                    /* decode stream starts on the cyclic key */
      0x10, 0x00};             /* terminator (never reached) */
  assert(fifa96_tree_decode(cycle, sizeof cycle, dst, cap, &got) < 0);
  assert(got == 0);

  free(dst); free(exact); free(in); free(want);
  printf("test_tree OK\n");
  return 0;
}
