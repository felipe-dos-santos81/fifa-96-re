/* tests/test_engine_menu_art.c — pins a deterministic menu frame hash */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_menu_art.h"
#include "fifa96_engine/fifa96_platform_null.h"
#include "fifa96_engine/fifa96_surface.h"

#define MT_SEC 2048u
#define MT_SECS 32u
#define MT_LEN (MT_SECS * MT_SEC)

static size_t mt_image_len;

static void mt_put16le(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}

static void mt_put16be(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v >> 8);
  p[1] = (uint8_t)v;
}

static void mt_put32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static void mt_put32be(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v >> 24);
  p[1] = (uint8_t)(v >> 16);
  p[2] = (uint8_t)(v >> 8);
  p[3] = (uint8_t)v;
}

static size_t mt_dir_record(uint8_t *p, const uint8_t *name, size_t namelen, uint32_t lba,
                            uint32_t size, uint8_t flags) {
  size_t len = 33 + namelen;
  if (len & 1) len++;
  memset(p, 0, len);
  p[0] = (uint8_t)len;
  mt_put32le(p + 2, lba);
  mt_put32be(p + 6, lba);
  mt_put32le(p + 10, size);
  mt_put32be(p + 14, size);
  p[25] = flags;
  mt_put16le(p + 28, 1);
  mt_put16be(p + 30, 1);
  p[32] = (uint8_t)namelen;
  memcpy(p + 33, name, namelen);
  return len;
}

static size_t mt_build_iso(uint8_t *image, size_t cap, const uint8_t *file, size_t file_len) {
  static const uint8_t self[1] = {0x00};
  static const uint8_t parent[1] = {0x01};
  assert(cap >= MT_LEN);
  memset(image, 0, MT_LEN);
  uint8_t *pvd = image + 16 * MT_SEC;
  pvd[0] = 1;
  memcpy(pvd + 1, "CD001", 5);
  pvd[6] = 1;
  mt_put32le(pvd + 80, MT_SECS);
  mt_put32be(pvd + 84, MT_SECS);
  mt_put16le(pvd + 128, MT_SEC);
  mt_put16be(pvd + 130, MT_SEC);
  mt_dir_record(pvd + 156, self, 1, 18, MT_SEC, 0x02);
  uint8_t *root = image + 18 * MT_SEC;
  size_t p = 0;
  p += mt_dir_record(root + p, self, 1, 18, MT_SEC, 0x02);
  p += mt_dir_record(root + p, parent, 1, 18, MT_SEC, 0x02);
  p += mt_dir_record(root + p, (const uint8_t *)"OPTIONS.INV;1", 13, 19, (uint32_t)file_len,
                     0x00);
  if (file_len) memcpy(image + 19 * MT_SEC, file, file_len);
  return MT_LEN;
}

static fifa96_err_t mt_read(void *ctx, uint64_t off, uint8_t *dst, size_t len) {
  const uint8_t *image = (const uint8_t *)ctx;
  if (off > (uint64_t)mt_image_len || (uint64_t)len > (uint64_t)mt_image_len - off) {
    return FIFA96_ERR_TRUNCATED;
  }
  if (len) memcpy(dst, image + off, len);
  return FIFA96_OK;
}

static size_t mt_build_shpi(uint8_t *buf, size_t cap) {
  const uint16_t w = 4, h = 4;
  uint32_t total = 16u + 8u + 16u + (uint32_t)w * h;
  assert(cap >= total);
  memset(buf, 0, total);
  memcpy(buf, "SHPI", 4);
  mt_put32le(buf + 4, total);
  mt_put32le(buf + 8, 1);
  memcpy(buf + 12, "BGND", 4);
  memcpy(buf + 16, "MAIN", 4);
  mt_put32le(buf + 20, 24);
  mt_put16le(buf + 28, w);
  mt_put16le(buf + 30, h);
  for (size_t i = 0; i < (size_t)w * h; i++) buf[40 + i] = 200;
  return total;
}

static uint64_t draw_hash(struct fifa96_surface *s, uint32_t entry_state, int row, int cursor) {
  fifa96_surface_clear(s, 0);
  struct fifa96_menu_state st = {.entry_state = entry_state, .selected_row = row,
                                 .cursor_on = cursor};
  fifa96_menu_art_draw(s, &st);
  return fifa96_surface_hash(s);
}

static void test_fallback_hash(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  assert(fifa96_menu_art_init(NULL) == 0);
  uint64_t h = draw_hash(s, 0, 0, 1);
  printf("FIFA96_MENU_FALLBACK_HASH = 0x%016llx\n", (unsigned long long)h);
  fflush(stdout);
  assert(h == FIFA96_MENU_FALLBACK_HASH);
  fifa96_surface_destroy(s);
}

static void test_fallback_deterministic(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  assert(fifa96_menu_art_init(NULL) == 0);
  uint64_t a = draw_hash(s, 0, 0, 1);
  uint64_t b = draw_hash(s, 0, 0, 1);
  assert(a == b);
  fifa96_surface_destroy(s);
}

static void test_state_changes_frame(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  assert(fifa96_menu_art_init(NULL) == 0);
  uint64_t cursor_on = draw_hash(s, 0, 0, 1);
  uint64_t cursor_off = draw_hash(s, 0, 0, 0);
  uint64_t row1 = draw_hash(s, 0, 1, 1);
  uint64_t row0 = draw_hash(s, 0, 0, 1);
  assert(cursor_on != cursor_off);
  assert(row1 != row0);
  fifa96_surface_destroy(s);
}

static void test_draw_differs_from_clear(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  fifa96_surface_clear(s, 0);
  uint64_t cleared = fifa96_surface_hash(s);
  assert(fifa96_menu_art_init(NULL) == 0);
  struct fifa96_menu_state st = {.entry_state = 17, .selected_row = 3, .cursor_on = 1};
  fifa96_menu_art_draw(s, &st);
  assert(fifa96_surface_hash(s) != cleared);
  fifa96_surface_destroy(s);
}

static void test_empty_asset_table_falls_back(void) {
  struct fifa96_asset_table t;
  memset(&t, 0, sizeof t);
  assert(fifa96_menu_art_init(&t) == 0);
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  uint64_t h = draw_hash(s, 0, 0, 1);
  assert(h == FIFA96_MENU_FALLBACK_HASH);
  fifa96_surface_destroy(s);
}

static void test_null_args(void) {
  assert(fifa96_menu_art_init(NULL) == 0);
  struct fifa96_menu_state st = {0, 0, 0};
  fifa96_menu_art_draw(NULL, &st);
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  fifa96_menu_art_draw(s, NULL);
  fifa96_surface_destroy(s);
}

static void test_zero_length_options_inv(void) {
  uint8_t image[MT_LEN];
  mt_build_iso(image, sizeof image, NULL, 0);
  mt_image_len = sizeof image;
  struct fifa96_asset_table *t = NULL;
  assert(fifa96_asset_mount_iso(mt_read, image, sizeof image, &t) == FIFA96_OK);
  assert(t != NULL && t->count == 1u);
  assert(fifa96_menu_art_init(t) == 0);
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  assert(draw_hash(s, 0, 0, 1) == FIFA96_MENU_FALLBACK_HASH);
  fifa96_surface_destroy(s);
  fifa96_asset_unmount(t);
}

static void test_engine_boot_uses_asset_table(void) {
  uint8_t shpi[64];
  size_t shpi_len = mt_build_shpi(shpi, sizeof shpi);
  uint8_t image[MT_LEN];
  mt_build_iso(image, sizeof image, shpi, shpi_len);
  char path[] = "/tmp/fifa96_menu_art_XXXXXX";
  int fd = mkstemp(path);
  assert(fd >= 0);
  FILE *f = fdopen(fd, "wb");
  assert(f != NULL);
  assert(fwrite(image, 1, sizeof image, f) == sizeof image);
  assert(fclose(f) == 0);

  struct fifa96_platform_null_config pcfg = {0};
  fifa96_platform *plain = fifa96_platform_null_create(&pcfg);
  assert(plain != NULL);
  struct fifa96_engine_config ecfg = {0};
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  struct fifa96_engine *pe = fifa96_engine_create(&ecfg, plain);
  assert(pe != NULL);
  assert(fifa96_engine_boot(pe) == 0);
  assert(fifa96_engine_step(pe) == 0);
  struct fifa96_platform_null_stats ps;
  fifa96_platform_null_stats(plain, &ps);

  fifa96_platform *asset = fifa96_platform_null_create(&pcfg);
  assert(asset != NULL);
  ecfg.iso_path = path;
  struct fifa96_engine *ae = fifa96_engine_create(&ecfg, asset);
  assert(ae != NULL);
  assert(fifa96_engine_boot(ae) == 0);
  assert(fifa96_engine_step(ae) == 0);
  struct fifa96_platform_null_stats as;
  fifa96_platform_null_stats(asset, &as);
  assert(as.present_hash != ps.present_hash);

  fifa96_engine_destroy(ae);
  fifa96_platform_destroy(asset);
  fifa96_engine_destroy(pe);
  fifa96_platform_destroy(plain);
  assert(remove(path) == 0);
}

int main(void) {
  test_fallback_hash();
  test_fallback_deterministic();
  test_state_changes_frame();
  test_draw_differs_from_clear();
  test_empty_asset_table_falls_back();
  test_null_args();
  test_zero_length_options_inv();
  test_engine_boot_uses_asset_table();
  puts("test_engine_menu_art OK");
  return 0;
}
