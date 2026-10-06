/* tests/test_engine_asset.c */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_platform_null.h"

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
  assert(t->count == 2u);
  assert(strcmp(t->entries[0].path, "/ART/PIX.PVI") == 0);
  assert(strcmp(t->entries[1].path, "/ART/EMPTY.DAT") == 0);

  uint32_t lba = 0, size = 0;
  assert(fifa96_asset_lookup(t, "/ART/PIX.PVI", &lba, &size) == FIFA96_OK);
  assert(lba == 20u);
  assert(size == 4u);
  uint8_t *bytes = NULL;
  size_t len = 0;
  assert(fifa96_asset_read(t, "/ART/PIX.PVI", &bytes, &len) == FIFA96_OK);
  assert(len == 4u && memcmp(bytes, "DATA", 4) == 0);
  assert(fifa96_asset_lookup(t, "/NOPE", &lba, &size) == FIFA96_ERR_NOT_FOUND);
  fifa96_asset_free(bytes);

  uint8_t *empty = NULL;
  size_t empty_len = 99;
  assert(fifa96_asset_read(t, "/ART/EMPTY.DAT", &empty, &empty_len) == FIFA96_OK);
  assert(empty_len == 0u);
  fifa96_asset_free(empty);

  /* a zero-length read result is freeable and unmount is idempotent-safe */
  fifa96_asset_free(NULL);
  fifa96_asset_unmount(t);
  fifa96_asset_unmount(NULL);
}

static void test_lookup_case_insensitive(void) {
  uint8_t image[IMG_LEN];
  size_t image_len = build_test_iso(image, sizeof image);
  struct fifa96_asset_table *t = mount_test_iso(image, image_len);
  uint32_t lba = 0, size = 0;
  assert(fifa96_asset_lookup(t, "/art/pix.pvi", &lba, &size) == FIFA96_OK);
  assert(lba == 20u && size == 4u);
  assert(fifa96_asset_lookup(t, "ART/PIX.PVI", &lba, &size) == FIFA96_OK);
  assert(lba == 20u && size == 4u);
  assert(fifa96_asset_lookup(t, "/Art/Pix.Pvi", &lba, &size) == FIFA96_OK);
  /* directories are enumerated implicitly, never stored as entries */
  assert(fifa96_asset_lookup(t, "/ART", &lba, &size) == FIFA96_ERR_NOT_FOUND);
  fifa96_asset_unmount(t);
}

static void test_errors(void) {
  uint8_t image[IMG_LEN];
  size_t image_len = build_test_iso(image, sizeof image);
  struct fifa96_asset_table *t = mount_test_iso(image, image_len);
  uint32_t lba = 0, size = 0;
  assert(fifa96_asset_lookup(NULL, "/ART/PIX.PVI", &lba, &size) == FIFA96_ERR_INVALID);
  assert(fifa96_asset_lookup(t, NULL, &lba, &size) == FIFA96_ERR_INVALID);
  assert(fifa96_asset_lookup(t, "/ART/PIX.PVI", NULL, &size) == FIFA96_ERR_INVALID);
  assert(fifa96_asset_lookup(t, "/ART/PIX.PVI", &lba, NULL) == FIFA96_ERR_INVALID);
  assert(fifa96_asset_lookup(t, "", &lba, &size) == FIFA96_ERR_NOT_FOUND);
  assert(fifa96_asset_lookup(t, "/", &lba, &size) == FIFA96_ERR_NOT_FOUND);

  uint8_t *bytes = NULL;
  size_t len = 99;
  assert(fifa96_asset_read(t, "/NOPE", &bytes, &len) == FIFA96_ERR_NOT_FOUND);
  assert(bytes == NULL && len == 0u);
  assert(fifa96_asset_read(NULL, "/ART/PIX.PVI", &bytes, &len) == FIFA96_ERR_INVALID);
  assert(fifa96_asset_read(t, "/ART/PIX.PVI", NULL, &len) == FIFA96_ERR_INVALID);
  assert(fifa96_asset_read(t, "/ART/PIX.PVI", &bytes, NULL) == FIFA96_ERR_INVALID);
  fifa96_asset_unmount(t);

  struct fifa96_asset_table *t2 = (struct fifa96_asset_table *)image;
  g_image_len = image_len;
  assert(fifa96_asset_mount_iso(NULL, image, image_len, &t2) == FIFA96_ERR_INVALID);
  assert(t2 == NULL);
  assert(fifa96_asset_mount_iso(mem_read, image, image_len, NULL) == FIFA96_ERR_INVALID);
  t2 = (struct fifa96_asset_table *)image;
  assert(fifa96_asset_mount_iso(mem_read, image, 16u * SEC, &t2) == FIFA96_ERR_TRUNCATED);
  assert(t2 == NULL);
}

static void test_mount_file(void) {
  uint8_t image[IMG_LEN];
  size_t image_len = build_test_iso(image, sizeof image);
  char path[] = "/tmp/fifa96_asset_test_XXXXXX";
  int fd = mkstemp(path);
  assert(fd >= 0);
  FILE *f = fdopen(fd, "wb");
  assert(f != NULL);
  assert(fwrite(image, 1, image_len, f) == image_len);
  assert(fclose(f) == 0);

  struct fifa96_asset_table *t = NULL;
  assert(fifa96_asset_mount_file(path, &t) == FIFA96_OK);
  uint32_t lba = 0, size = 0;
  assert(fifa96_asset_lookup(t, "/art/pix.pvi", &lba, &size) == FIFA96_OK);
  assert(lba == 20u && size == 4u);
  uint8_t *bytes = NULL;
  size_t len = 0;
  assert(fifa96_asset_read(t, "/ART/PIX.PVI", &bytes, &len) == FIFA96_OK);
  assert(len == 4u && memcmp(bytes, "DATA", 4) == 0);
  fifa96_asset_free(bytes);
  fifa96_asset_unmount(t);
  assert(remove(path) == 0);

  assert(fifa96_asset_mount_file("/tmp/fifa96_asset_missing.iso", &t) == FIFA96_ERR_NOT_FOUND);
  assert(t == NULL);
  t = (struct fifa96_asset_table *)image;
  assert(fifa96_asset_mount_file(NULL, &t) == FIFA96_ERR_INVALID);
  assert(t == NULL);
  assert(fifa96_asset_mount_file(path, NULL) == FIFA96_ERR_INVALID);
}

static void test_engine_boot_with_iso(void) {
  uint8_t image[IMG_LEN];
  size_t image_len = build_test_iso(image, sizeof image);
  char path[] = "/tmp/fifa96_engine_asset_test_XXXXXX";
  int fd = mkstemp(path);
  assert(fd >= 0);
  FILE *f = fdopen(fd, "wb");
  assert(f != NULL);
  assert(fwrite(image, 1, image_len, f) == image_len);
  assert(fclose(f) == 0);

  fifa96_platform *plat = fifa96_platform_null_create(NULL);
  assert(plat != NULL);
  struct fifa96_engine_config cfg = {0};
  cfg.iso_path = path;
  cfg.width = 320;
  cfg.height = 240;
  cfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&cfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);
  assert(fifa96_engine_step(e) == 0);
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
  assert(remove(path) == 0);
}

static void test_engine_boot_missing_iso(void) {
  fifa96_platform *plat = fifa96_platform_null_create(NULL);
  assert(plat != NULL);
  struct fifa96_engine_config cfg = {0};
  cfg.iso_path = "/tmp/fifa96_engine_missing.iso";
  cfg.width = 320;
  cfg.height = 240;
  cfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&cfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == -1);
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
}

int main(void) {
  test_brief_path();
  test_lookup_case_insensitive();
  test_errors();
  test_mount_file();
  test_engine_boot_with_iso();
  test_engine_boot_missing_iso();
  puts("test_engine_asset OK");
  return 0;
}
