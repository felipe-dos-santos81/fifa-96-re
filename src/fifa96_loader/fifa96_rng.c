/* src/fifa96_loader/fifa96_rng.c — FU-141: the derived match RNG.
 * Literal transcription of FUN_00092AA0 (seed) and FUN_00092AC8 (step); see
 * include/fifa96_loader/fifa96_rng.h and docs/ghidra/FU141_action_cluster_de.md. */
#include "fifa96_loader/fifa96_rng.h"

/* The flat 0x102D70 bytes FUN_00092AA0 reads: the first six of "ArCaDe-CoInOp". */
static const uint8_t fifa96_rng_seed_bytes[6] = {
  0x41, 0x72, 0x43, 0x61, 0x44, 0x65
};

int fifa96_rng_seed(struct fifa96_rng *rng, uint32_t seed) {
  if (!rng) return -FIFA96_ERR_INVALID;
  for (uint32_t i = 0; i < 6; i++) {
    rng->w[i] = (uint32_t)(int32_t)(int8_t)fifa96_rng_seed_bytes[i] +
                ((uint32_t)seed << 25);
  }
  return FIFA96_OK;
}

int fifa96_rng_step(struct fifa96_rng *rng, uint16_t *value) {
  uint32_t t;
  uint32_t carry;
  if (!rng || !value) return -FIFA96_ERR_INVALID;
  /* t1 = w[5] + w[4]; w[4] = low(t1); carry = high(t1) */
  t = rng->w[5] + rng->w[4];
  rng->w[4] = t & 0xFFFFu;
  carry = t >> 16;
  /* t2 = low(t1) + carry + w[3]; w[3] = low(t2); carry = high(t2) */
  t = (t & 0xFFFFu) + carry + rng->w[3];
  rng->w[3] = t & 0xFFFFu;
  carry = t >> 16;
  t = (t & 0xFFFFu) + carry + rng->w[2];
  rng->w[2] = t & 0xFFFFu;
  carry = t >> 16;
  t = (t & 0xFFFFu) + carry + rng->w[1];
  rng->w[1] = t & 0xFFFFu;
  carry = t >> 16;
  t = (t & 0xFFFFu) + carry + rng->w[0];
  rng->w[0] = t & 0xFFFFu;
  rng->w[5] = (rng->w[5] + 1u) & 0xFFFFu;
  /* The native carry chain: only when a word wrapped to zero does the next
   * lower word increment, down to w[0]. */
  if (rng->w[5] == 0) {
    rng->w[4] = (rng->w[4] + 1u) & 0xFFFFu;
    if (rng->w[4] == 0) {
      rng->w[3] = (rng->w[3] + 1u) & 0xFFFFu;
      if (rng->w[3] == 0) {
        rng->w[2] = (rng->w[2] + 1u) & 0xFFFFu;
        if (rng->w[2] == 0) {
          rng->w[1] = (rng->w[1] + 1u) & 0xFFFFu;
          if (rng->w[1] == 0) rng->w[0] = (rng->w[0] + 1u) & 0xFFFFu;
        }
      }
    }
  }
  *value = (uint16_t)rng->w[0];
  return FIFA96_OK;
}
