#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

/* FU-148 §4.2 (S4 presentation completion): the palette translation pool
 * partitioner FUN_00046F80 and the per-entity kit translation
 * FUN_00048DC0 -> FUN_000CE980 -> 0x114720.
 *
 * The native match-data load FUN_00049138(1) allocates one buffer (the
 * "palettes" tag at 0x101A20; request 0x34E8 bytes per w7-b4 §2.10, alignment
 * class 0x220) and partitions it with FUN_00046F80 (first-hand 0x46F80
 * disasm): 23 x 0x100 sprite slots at 0x14BF60, 7 x 0x100 at 0x14BF34,
 * 8 x 0x100 at 0x14BB00, one shared 0x100 table at base+0x2600 (0x14BB20[0..255]
 * and 0x14BFBC alias it) and 9 fixed 0x100 blocks at base+0x2700..base+0x2F00.
 * Floor = 0x3000 (w7-b4 §2.10's "0x3600" is an erratum; FU-148 §4.2's
 * fixed-slot range 0x2800..0x3000 is off by one slot).
 *
 * The pool contents' producer is still leg 11 (which loaded file fills
 * [0x107290]); the engine therefore lands the seam without wiring it into the
 * render path. */
typedef struct fifa96_palette_pool {
  uint8_t *base;
  uint32_t size;
  const uint8_t *slots23[23];   /* 0x14BF60[i] = base + i*0x100 */
  const uint8_t *slots7[7];     /* 0x14BF34[i] = base + 0x1700 + i*0x100 */
  const uint8_t *slots8[8];     /* 0x14BB00[i] = base + 0x1E00 + i*0x100 */
  const uint8_t *shared;        /* 0x14BB20 / 0x14BFBC = base + 0x2600 */
  const uint8_t *fixed[9];      /* 0x14BF50,20,54,2C,28,30,5C,58,24 ascending */
} fifa96_palette_pool;

/* FUN_00046F80 partition arithmetic over a caller-owned buffer (the native
 * writer is blind; the port requires size >= 0x3000). Returns FIFA96_OK,
 * -FIFA96_ERR_INVALID (NULL base/out or size < 0x3000). */
fifa96_err_t fifa96_palette_pool_partition(uint8_t *base, uint32_t size,
                                           fifa96_palette_pool *out);

/* The FUN_00048DC0 kit path for entity 0 (bands [0x94,0x9B) -> table 0x107287
 * + base 0xA1 and [0x9B,0xA6) -> table 0x10727C + base 0xA3) and entity 0xB
 * ([0x82,0x89) -> table 0x107287 + base 0x9C and [0x89,0x94) -> table 0x10727C
 * + base 0x9E). Copies src to dst first (the native memmove), then rewrites
 * the band members. Returns FIFA96_OK, -FIFA96_ERR_INVALID (NULL or an entity
 * without a kit band; the other entities take the verbatim slot copy). */
fifa96_err_t fifa96_palette_translate_kit(const uint8_t *src, uint8_t *dst,
                                          uint32_t entity);

/* FUN_000CE980: copy the 0x100-byte slot verbatim into the 0x114720 consumer
 * table (the engine's render.remap). */
fifa96_err_t fifa96_palette_translate_slot(uint8_t *dst, const uint8_t *slot);
