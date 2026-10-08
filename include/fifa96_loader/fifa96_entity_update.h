#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

typedef struct fifa96_entity_candidate {
  int16_t x;
  int16_t y;
  uint8_t skip_98;
  uint8_t skip_9a;
} fifa96_entity_candidate;

int32_t fifa96_entity_distance(int32_t dx, int32_t dy);
int fifa96_entity_find_nearest(const fifa96_entity_candidate *candidates,
                               uint32_t count, uint32_t skip_index,
                               int16_t target_x, int16_t target_y,
                               int16_t *best_distance);

/* Moved-down integer math primitives (M2 Task 14 / OL-41), first-hand on
 * /FIFA96.EXE:
 *
 * `fifa96_entity_angle(x, z, &angle)` is `FUN_000CD474` (`0xCD474..0xCD4C3`,
 * 26 instructions, the 257-byte atan table flat `0x14072C`): the sign-magnitude
 * ratio fold with the CS jump table; negative results are produced by the
 * quadrant cases exactly as `fifa96_action_kick_angle` previously ported it
 * (that function now forwards here so the entity library is a leaf again).
 *
 * `fifa96_entity_sine(angle)` is the `0x114E04` 257-entry sine table fold with
 * the byte-shift quadrant idiom (`idx = (angle & 0xFF) ^ -bit8`, `idx += bit8`,
 * bit9 negates), the exact primitive `fifa96_ball_fold` (ball pairing) and the
 * `0x795B4` band divide both consume. NULL `angle` ->
 * -FIFA96_ERR_INVALID; `fifa96_entity_sine` takes any angle. */
int fifa96_entity_angle(int32_t x, int32_t z, int32_t *angle);
int32_t fifa96_entity_sine(int32_t angle);

/* `FUN_0008D824` (`0x8D824..0x8D8EB`, 81 instructions, OL-41): the
 * interception bind. EAX = the controlled actor record (`0x8DA75 MOV
 * EAX,[0x157A83]`), EDX = the nearest record, EBX = the nearest record's
 * +0x4D output triple. Writes the triple:
 *   out.y = 0;
 *   out.z = 0xB10 - ((0xB10 - |actor.z|) / 2 + 0x120), negated when the
 *           actor's team side ([[actor]+0x826]) is non-zero
 *           (`0x8D82A MOV EDI,[EAX+0x61]` — the *actor's* z, not the
 *           nearest's);
 *   out.x = |actor.x| >= 0x180 ? (actor.x + 0x180) / 3 + 0xC0
 *                             : (actor.x > 0 ? actor.x - 0x240 : 0x240 - actor.x)
 * (the actor.x branch is signed IDIV truncation; the +0x180/-0x180 branch pair
 * at `0x8D8AC` selects on |nearest.x|, which is never negative, so the live
 * arm is the +0x180 one — the other is dead compiler output; `nearest_x`
 * exists only for that dead read). NULL `out` -> -FIFA96_ERR_INVALID. */
typedef struct fifa96_entity_intercept_target {
  int32_t x;
  int32_t y;
  int32_t z;
} fifa96_entity_intercept_target;

int fifa96_entity_intercept_bind(int32_t actor_x, int32_t actor_z, uint8_t actor_side,
                                 int32_t nearest_x, fifa96_entity_intercept_target *out);

/* `FUN_000795B4` (`0x795B4..0x795F0`, 28 instructions, OL-41): the
 * interception distance band. EAX = the position triple (`from`), EDX = the
 * target triple, EBX = the output triple. First-hand it writes
 *   out.dx = (int16)(target.word0 - from.word0);
 *   out.dz = (int16)(target.word8 - from.word8);
 *   out.band = (int16)((|dx| << 16) / sine_or_cosine(angle(dx, dz)))
 * where the divisor is `fifa96_entity_sine(a)` for `a > 0x80` and
 * `fifa96_entity_sine(a + 0x100)` otherwise, with `a` the
 * `fifa96_entity_angle` result normalized to `0..0x100` by mirroring over
 * `0x100` (the `0xCE364`/`0xCE386` paths, verified equal to the sine primitive
 * for every `a` in range). The output triple is a caller scratch: the
 * interception caller passes `LEA EBX,[ESP+0xC]` (`0x8DA88`) and the record's
 * +0x4D triple is only the *input* (`EDX`) — nothing is stored back to the
 * record. NULL `out` -> -FIFA96_ERR_INVALID. */
typedef struct fifa96_entity_intercept_band_out {
  int16_t band;
  int16_t dx;
  int16_t dz;
} fifa96_entity_intercept_band_out;

int fifa96_entity_intercept_band(int32_t pos_x, int32_t pos_z,
                                 int32_t target_x, int32_t target_z,
                                 fifa96_entity_intercept_band_out *out);
