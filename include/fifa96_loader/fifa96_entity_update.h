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

/* FU302-L1 (phase-28 T7): `FUN_000CD514` (`0xCD514..0xCD579`, 41 instructions,
 * fresh `disassemble_function`) — the exact trig-normalized magnitude/hypot.
 * `a = [EBP+8]`, `b = [EBP+0xc]`:
 *
 *   angle = `0xCD474(A=b, B=a)`; |angle|; if > 0x100 -> 0x200-|angle|
 *   if angle <= 0x80: divisor = sine(angle + 0x100) (= cosine); mag = |a|
 *   else:             divisor = sine(angle)             (= sine);   mag = |b|
 *   return (mag << 16) / divisor
 *
 * Both branches are `hypot(a, b)`; the branch only picks the better-conditioned
 * component. Returns the full 32-bit quotient (the callers store AX).
 *
 * **Residual leg FU302-L1** (native owner `0xCD514`): the pre-existing
 * `fifa96_entity_intercept_band` below is **not** this fold — it computes
 * `fifa96_entity_angle(dx, dz)` (the transposed pair) and always divides `|dx|`,
 * so it is not an exact `0xCD514` port. Its callers
 * (`handlers.c:3198/3263/3425/5167`, `fifa96_match_entities.c:217`) are outside
 * this task's scope and keep that behavior; the row-0x1B Block E and the
 * `0x795B4` band use this exact primitive instead. */
int32_t fifa96_entity_hypot(int32_t a, int32_t b);

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
 * record. NULL `out` -> -FIFA96_ERR_INVALID.
 *
 * **FU302-L1 residual (phase-28 T7, native owner `0xCD514`):** the formula
 * above is the engine's derived behavior, **not** the native `0x795B4` body.
 * Fresh `0x795B4` computes dx/dz and calls `0xCD514(dx, dz)` (`0x795E2`),
 * which normalizes `fifa96_entity_angle(dz, dx)` and divides `|dx|/cos` or
 * `|dz|/sin` by the branch. This helper's transposed angle plus its fixed
 * `|dx|` dividend diverge on z-dominant pairs (e.g. `(0, 13)` returns 0 here
 * vs 13 natively). The row-0x1B/0x1C band uses the exact
 * `fifa96_entity_hypot`; the other callers stay on this derived sibling. */
typedef struct fifa96_entity_intercept_band_out {
  int16_t band;
  int16_t dx;
  int16_t dz;
} fifa96_entity_intercept_band_out;

int fifa96_entity_intercept_band(int32_t pos_x, int32_t pos_z,
                                 int32_t target_x, int32_t target_z,
                                 fifa96_entity_intercept_band_out *out);

/* FU-151 §Port contract item 7 / §2.9: `FUN_00079C50` (`0x79C50..0x79C98`,
 * first-hand this slice) — the record face/sector write. `dx`/`dz` are the
 * 16-bit direction words; a zero pair returns the existing `*sector` byte and
 * leaves `*face7d` alone; else `*face7d = fifa96_entity_angle(dx, dz)` (the
 * native `word[rec+0x7D]`) and `*sector = ((*face7d + 0x40) & 0x3FF) >> 7`
 * (the native byte `+0x8E`, 0..7). NULL `face7d`/`sector` ->
 * -FIFA96_ERR_INVALID. */
int fifa96_entity_face(int32_t dx, int32_t dz, int32_t *face7d, uint8_t *sector);
