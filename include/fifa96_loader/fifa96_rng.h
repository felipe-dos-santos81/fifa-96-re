/* include/fifa96_loader/fifa96_rng.h — FU-141: the derived match RNG.
 *
 * Native evidence (read-only /FIFA96.EXE): the six-word state at flat
 * 0x110F44..0x110F58; the seed writer FUN_00092AA0 (sole call 0x493F2 in the
 * match init FUN_000493A0) sets `w[i] = (int8)"ArCaDe"[i] + (seed << 25)`
 * (`SHL ECX,0x19` at 0x92AAA, table bytes at flat 0x102D70); the step
 * FUN_00092AC8 (0x92AC8..0x92BC1) is a six-word additive generator with a
 * carry chain, returning the new w[0] word. `FUN_000CB2A4` (a getter of
 * `[0x112E88]`) is a different global and is not this generator. */
#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

/* w[0..5] are the native words 0x110F44/0x110F48/0x110F4C/0x110F50/0x110F54/
 * 0x110F58 in that order. */
struct fifa96_rng {
  uint32_t w[6];
};

/* FUN_00092AA0: seed the six words from the embedded "ArCaDe" bytes and the
 * caller's seed (`seed << 25` on the 32-bit word). NULL -> -FIFA96_ERR_INVALID. */
int fifa96_rng_seed(struct fifa96_rng *rng, uint32_t seed);

/* FUN_00092AC8: advance the state and return the new w[0] word through
 * `value`. NULL rng/value -> -FIFA96_ERR_INVALID. */
int fifa96_rng_step(struct fifa96_rng *rng, uint16_t *value);
