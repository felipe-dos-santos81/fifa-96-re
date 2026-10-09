// tests/test_palette.c — FU-148 §4.2 (S4): the palette translation pool
// partition (FUN_00046F80) and the per-entity kit translation
// (FUN_00048DC0 -> FUN_000CE980 -> 0x114720).
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_palette.h"

#define R_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

static void test_pool_partition_offsets(void) {
  static uint8_t base[0x3000];
  fifa96_palette_pool pool;
  assert(fifa96_palette_pool_partition(base, sizeof base, &pool) == FIFA96_OK);
  assert(pool.base == base);
  for (uint32_t i = 0; i < 23u; i++)
    assert(pool.slots23[i] == base + i * 0x100u);
  for (uint32_t i = 0; i < 7u; i++)
    assert(pool.slots7[i] == base + 0x1700u + i * 0x100u);
  for (uint32_t i = 0; i < 8u; i++)
    assert(pool.slots8[i] == base + 0x1E00u + i * 0x100u);
  assert(pool.shared == base + 0x2600u);
  assert(pool.fixed[0] == base + 0x2700u);   /* 0x14BF50 */
  assert(pool.fixed[1] == base + 0x2800u);   /* 0x14BF20 */
  assert(pool.fixed[2] == base + 0x2900u);   /* 0x14BF54 */
  assert(pool.fixed[3] == base + 0x2A00u);   /* 0x14BF2C */
  assert(pool.fixed[4] == base + 0x2B00u);   /* 0x14BF28 */
  assert(pool.fixed[5] == base + 0x2C00u);   /* 0x14BF30 */
  assert(pool.fixed[6] == base + 0x2D00u);   /* 0x14BF5C */
  assert(pool.fixed[7] == base + 0x2E00u);   /* 0x14BF58 */
  assert(pool.fixed[8] == base + 0x2F00u);   /* 0x14BF24 */
}

static void test_pool_partition_invalid(void) {
  static uint8_t base[0x3000];
  fifa96_palette_pool pool;
  assert(fifa96_palette_pool_partition(NULL, 0x3000, &pool) == R_INVALID);
  assert(fifa96_palette_pool_partition(base, 0x3000, NULL) == R_INVALID);
  assert(fifa96_palette_pool_partition(base, 0x2FFF, &pool) == R_INVALID);
}

static void test_kit_translate_entity0(void) {
  uint8_t src[256], dst[256];
  for (int i = 0; i < 256; i++) src[i] = (uint8_t)i;
  assert(fifa96_palette_translate_kit(src, dst, 0) == FIFA96_OK);
  assert(dst[0x93] == 0x93);
  assert(dst[0x94] == 0xA1);   /* table1[0] + 0xA1 */
  assert(dst[0x97] == 0xA2);   /* table1[3] + 0xA1 */
  assert(dst[0x9A] == 0xA2);   /* table1[6] + 0xA1 */
  assert(dst[0x9B] == 0xA3);   /* table2[0] + 0xA3 */
  assert(dst[0x9E] == 0xA4);   /* table2[3] + 0xA3 */
  assert(dst[0xA5] == 0xA5);   /* table2[10] + 0xA3 */
  assert(dst[0xA6] == 0xA6);
}

static void test_kit_translate_entity_b(void) {
  uint8_t src[256], dst[256];
  for (int i = 0; i < 256; i++) src[i] = (uint8_t)i;
  assert(fifa96_palette_translate_kit(src, dst, 0xB) == FIFA96_OK);
  assert(dst[0x81] == 0x81);
  assert(dst[0x82] == 0x9C);   /* table1[0] + 0x9C */
  assert(dst[0x85] == 0x9D);   /* table1[3] + 0x9C */
  assert(dst[0x88] == 0x9D);   /* table1[6] + 0x9C */
  assert(dst[0x89] == 0x9E);   /* table2[0] + 0x9E */
  assert(dst[0x8C] == 0x9F);   /* table2[3] + 0x9E */
  assert(dst[0x93] == 0xA0);   /* table2[10] + 0x9E */
  assert(dst[0x94] == 0x94);
}

static void test_kit_translate_invalid(void) {
  uint8_t src[256], dst[256];
  memset(src, 0, sizeof src);
  assert(fifa96_palette_translate_kit(NULL, dst, 0) == R_INVALID);
  assert(fifa96_palette_translate_kit(src, NULL, 0) == R_INVALID);
  assert(fifa96_palette_translate_kit(src, dst, 1) == R_INVALID);
  assert(fifa96_palette_translate_kit(src, dst, 0xC) == R_INVALID);
}

static void test_slot_copy(void) {
  uint8_t slot[256], dst[256];
  for (int i = 0; i < 256; i++) slot[i] = (uint8_t)(255 - i);
  memset(dst, 0xAA, sizeof dst);
  assert(fifa96_palette_translate_slot(dst, slot) == FIFA96_OK);
  assert(memcmp(dst, slot, 256) == 0);
  assert(fifa96_palette_translate_slot(NULL, slot) == R_INVALID);
  assert(fifa96_palette_translate_slot(dst, NULL) == R_INVALID);
}

/* FU-152 §2.10/§4.4 (P4): the FUN_00049138(1) pool request identity —
 * size 0x34E8, type/alignment 0x220, name "palettes" (0x101A20), partition
 * floor 0x3000 (fresh 0x46F80 disasm). The create/release helpers allocate the
 * native request through the engine and run the FUN_00046F80 partition. */
static void test_pool_identity(void) {
  assert(FIFA96_PALETTE_POOL_REQUEST == 0x34E8u);
  assert(FIFA96_PALETTE_POOL_TYPE == 0x220u);
  assert(FIFA96_PALETTE_POOL_FLOOR == 0x3000u);
  assert(strcmp(FIFA96_PALETTE_POOL_TAG, "palettes") == 0);
  assert(fifa96_palette_pool_identity.size == 0x34E8u);
  assert(fifa96_palette_pool_identity.type == 0x220u);
  assert(strcmp(fifa96_palette_pool_identity.tag, "palettes") == 0);
}

static void test_pool_create_release(void) {
  fifa96_palette_pool pool;
  assert(fifa96_palette_pool_create(&pool) == FIFA96_OK);
  assert(pool.base != NULL);
  assert(pool.size == FIFA96_PALETTE_POOL_REQUEST);
  assert(pool.slots23[0] == pool.base);
  assert(pool.slots23[22] == pool.base + 22u * 0x100u);
  assert(pool.slots7[6] == pool.base + 0x1700u + 6u * 0x100u);
  assert(pool.slots8[7] == pool.base + 0x1E00u + 7u * 0x100u);
  assert(pool.shared == pool.base + 0x2600u);
  assert(pool.fixed[8] == pool.base + 0x2F00u);
  fifa96_palette_pool_release(&pool);
  assert(pool.base == NULL && pool.size == 0 && pool.shared == NULL);
  assert(fifa96_palette_pool_create(NULL) == R_INVALID);
  fifa96_palette_pool_release(NULL);   /* no-op */
}

int main(void) {
  test_pool_partition_offsets();
  test_pool_partition_invalid();
  test_kit_translate_entity0();
  test_kit_translate_entity_b();
  test_kit_translate_invalid();
  test_slot_copy();
  test_pool_identity();
  test_pool_create_release();
  puts("test_palette: ok");
  return 0;
}
