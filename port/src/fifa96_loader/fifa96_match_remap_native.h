#pragma once
#include <stdint.h>

/* Native FUN_00047298 (flat LE image address 0x47298): the 23-block kit/skin
 * remap descriptor builder.
 *
 * The native builder keeps five 0x8E-byte templates (copies of the static
 * 0x106962 base), then composes 23 256-byte output descriptors. Each output
 * remaps one render slot: pool[0..10] = home (0 = keeper), pool[11..21] = away
 * (11 = keeper), pool[22] = the extra (ball) descriptor.
 *
 * Class pairing (the native's two roster-derived classes, computed by the
 * engine from the PCCD roster codes -- not consumed as raw digits here):
 *   - class_a[side][pitch] is the native FUN_0004C00C class (0..3); it selects
 *     the skin ramp base (0x1070E4) and the a==0 clear.
 *   - class_b[side][pitch] is the native FUN_0004C098 class (0..3); it selects
 *     the hair ramp (0x1070D4).
 *   - side 0 = home (pool slots 0..10), side 1 = away (pool slots 11..21).
 *     The extra descriptor (slot 22) uses the hardcoded class_a = 2,
 *     class_b = 2 (template class 4, image row 13).
 *
 * The builder is pure data: no error returns, and it never fails. */
#define FIFA96_MATCH_REMAP_BLOCKS 23u
#define FIFA96_MATCH_REMAP_BYTES 256u

void fifa96_match_remap_build(uint8_t pool[FIFA96_MATCH_REMAP_BLOCKS]
                                             [FIFA96_MATCH_REMAP_BYTES],
                              const uint8_t class_a[2][11],
                              const uint8_t class_b[2][11], uint8_t home_blank,
                              uint8_t away_blank);
