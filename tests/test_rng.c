/* tests/test_rng.c — FU-141: the derived match RNG.
 *
 * Native evidence (read-only /FIFA96.EXE): the six-word state block at flat
 * 0x110F44..0x110F58; the seed writer FUN_00092AA0 (called once from the match
 * init FUN_000493A0 at 0x493F2) sets each word to `(int8)"ArCaDe"[i] +
 * (seed << 25)` (SHL ECX,0x19 at 0x92AAA); the step FUN_00092AC8
 * (0x92AC8..0x92BC1) mixes all six words with the carry chain and returns the
 * new S0 word. The sequence below is pinned from a literal transcription of
 * those bytes. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "fifa96_loader/fifa96_rng.h"

static void test_seed_words(void) {
  struct fifa96_rng rng;
  assert(fifa96_rng_seed(&rng, 0) == FIFA96_OK);
  assert(rng.w[0] == 0x41 && rng.w[1] == 0x72 && rng.w[2] == 0x43);
  assert(rng.w[3] == 0x61 && rng.w[4] == 0x44 && rng.w[5] == 0x65);
  assert(fifa96_rng_seed(&rng, 1) == FIFA96_OK);
  assert(rng.w[0] == 0x2000041u && rng.w[5] == 0x2000065u);
  /* the seed shift truncates to the low 7 bits being shifted out */
  assert(fifa96_rng_seed(&rng, 0x80) == FIFA96_OK);
  assert(rng.w[0] == 0x41);
  assert(fifa96_rng_seed(NULL, 0) == -FIFA96_ERR_INVALID);
}

static void test_step_sequence_seed_zero(void) {
  static const uint16_t expected[8] = {512, 1829, 4927, 11195,
                                       22605, 41818, 6755, 52849};
  struct fifa96_rng rng;
  assert(fifa96_rng_seed(&rng, 0) == FIFA96_OK);
  for (int i = 0; i < 8; i++) {
    uint16_t value = 0;
    assert(fifa96_rng_step(&rng, &value) == FIFA96_OK);
    assert(value == expected[i]);
  }
  assert(rng.w[0] == 0xCE71 && rng.w[1] == 0xB40E && rng.w[2] == 0x3D05);
  assert(rng.w[3] == 0x1109 && rng.w[4] == 0x388 && rng.w[5] == 0x6D);
}

static void test_step_sequence_seed_one(void) {
  static const uint16_t expected[4] = {3072, 8997, 20287, 39355};
  struct fifa96_rng rng;
  assert(fifa96_rng_seed(&rng, 1) == FIFA96_OK);
  for (int i = 0; i < 4; i++) {
    uint16_t value = 0;
    assert(fifa96_rng_step(&rng, &value) == FIFA96_OK);
    assert(value == expected[i]);
  }
}

/* S5 wrapping to zero propagates the carry into S4' (and only one step here,
 * since S4' becomes 0xFFFF). */
static void test_step_carry_chain(void) {
  struct fifa96_rng rng;
  uint16_t value = 0;
  rng.w[0] = 5;
  rng.w[1] = 0xFFFF;
  rng.w[2] = 0xFFFF;
  rng.w[3] = 0xFFFF;
  rng.w[4] = 0xFFFF;
  rng.w[5] = 0xFFFF;
  assert(fifa96_rng_step(&rng, &value) == FIFA96_OK);
  assert(value == 4);
  assert(rng.w[0] == 4 && rng.w[1] == 0xFFFE && rng.w[2] == 0xFFFE);
  assert(rng.w[3] == 0xFFFE && rng.w[4] == 0xFFFF && rng.w[5] == 0);
  /* the full chain: every updated word lands on its wrap point */
  rng.w[0] = 0;
  rng.w[1] = 0;
  rng.w[2] = 0;
  rng.w[3] = 0;
  rng.w[4] = 0;
  rng.w[5] = 0xFFFF;
  assert(fifa96_rng_step(&rng, &value) == FIFA96_OK);
  assert(value == 0);
  assert(rng.w[0] == 0 && rng.w[1] == 0 && rng.w[2] == 0);
  assert(rng.w[3] == 0 && rng.w[4] == 0 && rng.w[5] == 0);
}

static void test_step_invalid(void) {
  struct fifa96_rng rng;
  uint16_t value = 0;
  assert(fifa96_rng_seed(&rng, 0) == FIFA96_OK);
  assert(fifa96_rng_step(NULL, &value) == -FIFA96_ERR_INVALID);
  assert(fifa96_rng_step(&rng, NULL) == -FIFA96_ERR_INVALID);
}

int main(void) {
  test_seed_words();
  test_step_sequence_seed_zero();
  test_step_sequence_seed_one();
  test_step_carry_chain();
  test_step_invalid();
  puts("test_rng OK");
  return 0;
}
