/* tests/test_engine_cache.c — self-contained synthetic ISO, mirrors tests/test_engine_asset.c */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_cache.h"

#define SEC 2048u
#define IMG_SECS 64u
#define IMG_LEN (IMG_SECS * SEC)

static size_t g_image_len;

static void put16le(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}

static void put16be(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v >> 8);
  p[1] = (uint8_t)v;
}

static void put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static void put32be(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v >> 24);
  p[1] = (uint8_t)(v >> 16);
  p[2] = (uint8_t)(v >> 8);
  p[3] = (uint8_t)v;
}

static size_t dir_record(uint8_t *p, const uint8_t *name, size_t namelen, uint32_t lba,
                         uint32_t size, uint8_t flags) {
  size_t len = 33 + namelen;
  if (len & 1) len++;
  memset(p, 0, len);
  p[0] = (uint8_t)len;
  put32le(p + 2, lba);
  put32be(p + 6, lba);
  put32le(p + 10, size);
  put32be(p + 14, size);
  p[25] = flags;
  put16le(p + 28, 1);
  put16be(p + 30, 1);
  p[32] = (uint8_t)namelen;
  memcpy(p + 33, name, namelen);
  return len;
}

static size_t build_test_iso(uint8_t *image, size_t cap) {
  static const uint8_t self[1] = {0x00};
  static const uint8_t parent[1] = {0x01};
  assert(cap >= IMG_LEN);
  memset(image, 0, IMG_LEN);

  uint8_t *pvd = image + 16 * SEC;
  pvd[0] = 1;
  memcpy(pvd + 1, "CD001", 5);
  pvd[6] = 1;
  put32le(pvd + 80, IMG_SECS);
  put32be(pvd + 84, IMG_SECS);
  put16le(pvd + 128, SEC);
  put16be(pvd + 130, SEC);
  dir_record(pvd + 156, self, 1, 18, SEC, 0x02);

  uint8_t *root = image + 18 * SEC;
  size_t p = 0;
  p += dir_record(root + p, self, 1, 18, SEC, 0x02);
  p += dir_record(root + p, parent, 1, 18, SEC, 0x02);
  p += dir_record(root + p, (const uint8_t *)"ART", 3, 19, SEC, 0x02);

  uint8_t *art = image + 19 * SEC;
  p = 0;
  p += dir_record(art + p, self, 1, 19, SEC, 0x02);
  p += dir_record(art + p, parent, 1, 18, SEC, 0x02);
  p += dir_record(art + p, (const uint8_t *)"PIX.PVI;1", 9, 20, 4, 0x00);
  p += dir_record(art + p, (const uint8_t *)"EMPTY.DAT;1", 11, 20, 0, 0x00);

  memcpy(image + 20 * SEC, "DATA", 4);
  return IMG_LEN;
}

static fifa96_err_t mem_read(void *ctx, uint64_t off, uint8_t *dst, size_t len) {
  const uint8_t *image = (const uint8_t *)ctx;
  if (off > (uint64_t)g_image_len || (uint64_t)len > (uint64_t)g_image_len - off)
    return FIFA96_ERR_TRUNCATED;
  if (len) memcpy(dst, image + off, len);
  return FIFA96_OK;
}

static struct fifa96_asset_table *mount_test_iso(uint8_t *image, size_t len) {
  struct fifa96_asset_table *t = NULL;
  g_image_len = len;
  assert(fifa96_asset_mount_iso(mem_read, image, len, &t) == FIFA96_OK);
  assert(t != NULL);
  return t;
}

static void test_brief_path(void) {
  uint8_t image[IMG_LEN];
  size_t image_len = build_test_iso(image, sizeof image);
  struct fifa96_asset_table *t = mount_test_iso(image, image_len);
  struct fifa96_cache *c = fifa96_cache_create(t);
  assert(c != NULL);

  size_t len1 = 0, len2 = 0;
  const uint8_t *a = fifa96_cache_get(c, "/ART/PIX.PVI", &len1);
  const uint8_t *b = fifa96_cache_get(c, "/ART/PIX.PVI", &len2);
  assert(a && b && a == b && len1 == len2 && len1 == 4u);
  assert(memcmp(a, "DATA", 4) == 0);

  fifa96_cache_destroy(c);
  fifa96_asset_unmount(t);
}

static void test_case_insensitive_hit(void) {
  uint8_t image[IMG_LEN];
  size_t image_len = build_test_iso(image, sizeof image);
  struct fifa96_asset_table *t = mount_test_iso(image, image_len);
  struct fifa96_cache *c = fifa96_cache_create(t);
  assert(c != NULL);

  size_t len = 99;
  const uint8_t *a = fifa96_cache_get(c, "/ART/PIX.PVI", &len);
  assert(a != NULL && len == 4u);
  const uint8_t *b = fifa96_cache_get(c, "art/pix.pvi", &len);
  assert(b == a && len == 4u);
  const uint8_t *d = fifa96_cache_get(c, "ART/PIX.PVI", &len);
  assert(d == a && len == 4u);

  fifa96_cache_destroy(c);
  fifa96_asset_unmount(t);
}

static void test_miss(void) {
  uint8_t image[IMG_LEN];
  size_t image_len = build_test_iso(image, sizeof image);
  struct fifa96_asset_table *t = mount_test_iso(image, image_len);
  struct fifa96_cache *c = fifa96_cache_create(t);
  assert(c != NULL);

  size_t len = 99;
  assert(fifa96_cache_get(c, "/NOPE", &len) == NULL);
  assert(len == 0u);
  assert(fifa96_cache_get(c, "", &len) == NULL);
  assert(len == 0u);
  assert(fifa96_cache_get(c, "/ART", &len) == NULL);
  assert(len == 0u);

  size_t len1 = 0;
  assert(fifa96_cache_get(c, "/ART/PIX.PVI", &len1) != NULL && len1 == 4u);

  fifa96_cache_destroy(c);
  fifa96_asset_unmount(t);
}

static void test_zero_length(void) {
  uint8_t image[IMG_LEN];
  size_t image_len = build_test_iso(image, sizeof image);
  struct fifa96_asset_table *t = mount_test_iso(image, image_len);
  struct fifa96_cache *c = fifa96_cache_create(t);
  assert(c != NULL);

  size_t len = 99;
  const uint8_t *p = fifa96_cache_get(c, "/ART/EMPTY.DAT", &len);
  assert(p != NULL && len == 0u);
  size_t len2 = 99;
  assert(fifa96_cache_get(c, "/ART/EMPTY.DAT", &len2) == p && len2 == 0u);

  fifa96_cache_destroy(c);
  fifa96_asset_unmount(t);
}

static void test_invalid_and_ownership(void) {
  uint8_t image[IMG_LEN];
  size_t image_len = build_test_iso(image, sizeof image);
  struct fifa96_asset_table *t = mount_test_iso(image, image_len);

  assert(fifa96_cache_create(NULL) == NULL);

  struct fifa96_cache *c = fifa96_cache_create(t);
  assert(c != NULL);
  size_t len = 77;
  assert(fifa96_cache_get(NULL, "/ART/PIX.PVI", &len) == NULL);
  assert(len == 0u);
  len = 77;
  assert(fifa96_cache_get(c, NULL, &len) == NULL);
  assert(len == 0u);
  assert(fifa96_cache_get(c, "/ART/PIX.PVI", NULL) == NULL);

  /* destroy frees the cache but not the asset table */
  fifa96_cache_destroy(c);
  fifa96_cache_destroy(NULL);

  uint32_t lba = 0, size = 0;
  assert(fifa96_asset_lookup(t, "/ART/PIX.PVI", &lba, &size) == FIFA96_OK);
  assert(lba == 20u && size == 4u);
  fifa96_asset_unmount(t);
}

int main(void) {
  test_brief_path();
  test_case_insensitive_hit();
  test_miss();
  test_zero_length();
  test_invalid_and_ownership();
  puts("test_engine_cache OK");
  return 0;
}
