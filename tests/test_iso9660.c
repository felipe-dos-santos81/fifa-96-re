#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_iso9660.h"

#define SEC 2048u
#define IMG_SECS 24u
#define IMG_LEN (IMG_SECS * SEC)

static uint8_t img[IMG_LEN];

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

static void build_iso(void) {
  static const uint8_t self[1] = {0x00};
  static const uint8_t parent[1] = {0x01};
  memset(img, 0, sizeof img);
  uint8_t *pvd = img + 16 * SEC;
  pvd[0] = 1;
  memcpy(pvd + 1, "CD001", 5);
  pvd[6] = 1;
  put32le(pvd + 80, IMG_SECS);
  put32be(pvd + 84, IMG_SECS);
  put16le(pvd + 128, SEC);
  put16be(pvd + 130, SEC);
  dir_record(pvd + 156, self, 1, 18, SEC, 0x02);

  uint8_t *root = img + 18 * SEC;
  size_t p = 0;
  p += dir_record(root + p, self, 1, 18, SEC, 0x02);
  p += dir_record(root + p, parent, 1, 18, SEC, 0x02);
  p += dir_record(root + p, (const uint8_t *)"HELLO.TXT;1", 11, 20, 5, 0x00);
  p += dir_record(root + p, (const uint8_t *)"SUB", 3, 19, SEC, 0x02);

  uint8_t *sub = img + 19 * SEC;
  p = 0;
  p += dir_record(sub + p, self, 1, 19, SEC, 0x02);
  p += dir_record(sub + p, parent, 1, 18, SEC, 0x02);
  p += dir_record(sub + p, (const uint8_t *)"NESTED.BIN;1", 12, 21, 3, 0x00);

  memcpy(img + 20 * SEC, "HELLO", 5);
  memcpy(img + 21 * SEC, "ABC", 3);
}

struct mem_reader {
  const uint8_t *p;
  size_t n;
};

static fifa96_err_t mem_read(void *ctx, uint64_t off, uint8_t *dst, size_t len) {
  struct mem_reader *m = (struct mem_reader *)ctx;
  if (off > (uint64_t)m->n || (uint64_t)len > (uint64_t)m->n - off) return FIFA96_ERR_TRUNCATED;
  if (len) memcpy(dst, m->p + off, len);
  return FIFA96_OK;
}

static struct mem_reader reader = {img, IMG_LEN};

static fifa96_err_t open_iso(struct fifa96_iso9660 *iso) {
  return fifa96_iso9660_open(mem_read, &reader, IMG_LEN, iso);
}

static void test_open_parses_pvd(void) {
  build_iso();
  struct fifa96_iso9660 iso;
  assert(open_iso(&iso) == FIFA96_OK);
  assert(iso.image_len == IMG_LEN);
  assert(iso.root_lba == 18);
  assert(iso.root_size == SEC);
}

static void test_open_rejects_bad_input(void) {
  struct fifa96_iso9660 iso;
  struct mem_reader m;
  build_iso();
  m.p = img;
  m.n = 16 * SEC;
  assert(fifa96_iso9660_open(mem_read, &m, 16 * SEC, &iso) == FIFA96_ERR_TRUNCATED);
  m.p = img;
  m.n = sizeof img;
  assert(fifa96_iso9660_open(mem_read, &m, 15 * SEC, &iso) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_iso9660_open(mem_read, &m, sizeof img, NULL) == FIFA96_ERR_INVALID);
  assert(fifa96_iso9660_open(NULL, &m, sizeof img, &iso) == FIFA96_ERR_INVALID);

  memcpy(img + 16 * SEC + 1, "CD002", 5);
  assert(fifa96_iso9660_open(mem_read, &m, sizeof img, &iso) == FIFA96_ERR_BAD_MAGIC);
  memcpy(img + 16 * SEC + 1, "CD001", 5);

  img[16 * SEC] = 2;
  assert(fifa96_iso9660_open(mem_read, &m, sizeof img, &iso) == FIFA96_ERR_BAD_MAGIC);
  img[16 * SEC] = 1;

  put16le(img + 16 * SEC + 128, 512);
  assert(fifa96_iso9660_open(mem_read, &m, sizeof img, &iso) == FIFA96_ERR_UNSUPPORTED);
  put16le(img + 16 * SEC + 128, SEC);

  put32le(img + 16 * SEC + 156 + 2, 100);
  assert(fifa96_iso9660_open(mem_read, &m, sizeof img, &iso) == FIFA96_ERR_TRUNCATED);
}

static void test_find_top_and_nested(void) {
  build_iso();
  struct fifa96_iso9660 iso;
  assert(open_iso(&iso) == FIFA96_OK);
  uint32_t lba = 0, size = 0;
  assert(fifa96_iso9660_find(&iso, "/HELLO.TXT", &lba, &size) == FIFA96_OK);
  assert(lba == 20 && size == 5);
  assert(fifa96_iso9660_find(&iso, "HELLO.TXT;1", &lba, &size) == FIFA96_OK);
  assert(lba == 20 && size == 5);
  assert(fifa96_iso9660_find(&iso, "/sub/nested.bin", &lba, &size) == FIFA96_OK);
  assert(lba == 21 && size == 3);
  assert(fifa96_iso9660_find(&iso, "SUB/NESTED.BIN;1", &lba, &size) == FIFA96_OK);
  assert(lba == 21 && size == 3);
}

static void test_find_errors(void) {
  build_iso();
  struct fifa96_iso9660 iso;
  assert(open_iso(&iso) == FIFA96_OK);
  uint32_t lba = 0, size = 0;
  assert(fifa96_iso9660_find(&iso, "/NOPE.TXT", &lba, &size) == FIFA96_ERR_NOT_FOUND);
  assert(fifa96_iso9660_find(&iso, "/SUB", &lba, &size) == FIFA96_ERR_NOT_FOUND);
  assert(fifa96_iso9660_find(&iso, "/", &lba, &size) == FIFA96_ERR_NOT_FOUND);
  assert(fifa96_iso9660_find(&iso, "", &lba, &size) == FIFA96_ERR_NOT_FOUND);
  assert(fifa96_iso9660_find(&iso, "/HELLO.TXT/X", &lba, &size) == FIFA96_ERR_NOT_FOUND);
  assert(fifa96_iso9660_find(NULL, "/HELLO.TXT", &lba, &size) == FIFA96_ERR_INVALID);
  assert(fifa96_iso9660_find(&iso, NULL, &lba, &size) == FIFA96_ERR_INVALID);
  assert(fifa96_iso9660_find(&iso, "/HELLO.TXT", NULL, NULL) == FIFA96_ERR_INVALID);
}

static void test_find_rejects_bad_extent(void) {
  build_iso();
  uint8_t *root = img + 18 * SEC;
  size_t p = 2 * 34;
  put32le(root + p + 2, 100);
  put32be(root + p + 6, 100);
  struct fifa96_iso9660 iso;
  assert(open_iso(&iso) == FIFA96_OK);
  uint32_t lba = 0, size = 0;
  assert(fifa96_iso9660_find(&iso, "/HELLO.TXT", &lba, &size) == FIFA96_ERR_TRUNCATED);
}

static void test_read_extent(void) {
  build_iso();
  struct fifa96_iso9660 iso;
  assert(open_iso(&iso) == FIFA96_OK);
  uint8_t buf[8];
  memset(buf, 0, sizeof buf);
  assert(fifa96_iso9660_read_extent(&iso, 20, 5, buf, sizeof buf) == FIFA96_OK);
  assert(memcmp(buf, "HELLO", 5) == 0 && buf[5] == 0);
  assert(fifa96_iso9660_read_extent(&iso, 21, 3, buf, sizeof buf) == FIFA96_OK);
  assert(memcmp(buf, "ABC", 3) == 0);
  assert(fifa96_iso9660_read_extent(&iso, 20, 0, NULL, 0) == FIFA96_OK);
  assert(fifa96_iso9660_read_extent(&iso, 20, 5, NULL, 5) == FIFA96_ERR_INVALID);
  assert(fifa96_iso9660_read_extent(&iso, 20, 5, buf, 4) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_iso9660_read_extent(&iso, 23, 2 * SEC, buf, sizeof buf) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_iso9660_read_extent(&iso, IMG_SECS, 1, buf, sizeof buf) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_iso9660_read_extent(NULL, 20, 5, buf, sizeof buf) == FIFA96_ERR_INVALID);
}

struct walk_log {
  char path[8][64];
  uint32_t lba[8];
  uint32_t size[8];
  size_t count;
};

static fifa96_err_t log_visit(void *ctx, const char *path, uint32_t lba, uint32_t size) {
  struct walk_log *w = (struct walk_log *)ctx;
  if (w->count < 8) {
    snprintf(w->path[w->count], sizeof w->path[0], "%s", path);
    w->lba[w->count] = lba;
    w->size[w->count] = size;
  }
  w->count++;
  return FIFA96_OK;
}

static fifa96_err_t stop_visit(void *ctx, const char *path, uint32_t lba, uint32_t size) {
  (void)path;
  (void)lba;
  (void)size;
  (*(size_t *)ctx)++;
  return FIFA96_ERR_FULL;
}

static void test_walk_files(void) {
  build_iso();
  struct fifa96_iso9660 iso;
  assert(open_iso(&iso) == FIFA96_OK);
  struct walk_log w;
  memset(&w, 0, sizeof w);
  assert(fifa96_iso9660_walk(&iso, log_visit, &w) == FIFA96_OK);
  assert(w.count == 2);
  assert(strcmp(w.path[0], "/HELLO.TXT") == 0 && w.lba[0] == 20 && w.size[0] == 5);
  assert(strcmp(w.path[1], "/SUB/NESTED.BIN") == 0 && w.lba[1] == 21 && w.size[1] == 3);
  assert(fifa96_iso9660_walk(NULL, log_visit, &w) == FIFA96_ERR_INVALID);
  assert(fifa96_iso9660_walk(&iso, NULL, &w) == FIFA96_ERR_INVALID);
}

static void test_walk_stops_on_visitor_error(void) {
  build_iso();
  struct fifa96_iso9660 iso;
  assert(open_iso(&iso) == FIFA96_OK);
  size_t seen = 0;
  assert(fifa96_iso9660_walk(&iso, stop_visit, &seen) == FIFA96_ERR_FULL);
  assert(seen == 1);
}

static void test_walk_rejects_malformed_directory(void) {
  build_iso();
  img[18 * SEC] = 10;
  struct fifa96_iso9660 iso;
  assert(open_iso(&iso) == FIFA96_OK);
  struct walk_log w;
  memset(&w, 0, sizeof w);
  assert(fifa96_iso9660_walk(&iso, log_visit, &w) == FIFA96_ERR_TRUNCATED);

  build_iso();
  uint8_t *root = img + 18 * SEC;
  size_t p = 2 * 34;
  put32le(root + p + 2, 100);
  put32be(root + p + 6, 100);
  assert(open_iso(&iso) == FIFA96_OK);
  assert(fifa96_iso9660_walk(&iso, log_visit, &w) == FIFA96_ERR_TRUNCATED);
}

static void test_walk_rejects_bad_name(void) {
  build_iso();
  uint8_t *root = img + 18 * SEC;
  size_t p = 2 * 34;
  root[p + 32] = 200;
  struct fifa96_iso9660 iso;
  assert(open_iso(&iso) == FIFA96_OK);
  struct walk_log w;
  memset(&w, 0, sizeof w);
  assert(fifa96_iso9660_walk(&iso, log_visit, &w) == FIFA96_ERR_TRUNCATED);
}

int main(void) {
  test_open_parses_pvd();
  test_open_rejects_bad_input();
  test_find_top_and_nested();
  test_find_errors();
  test_find_rejects_bad_extent();
  test_read_extent();
  test_walk_files();
  test_walk_stops_on_visitor_error();
  test_walk_rejects_malformed_directory();
  test_walk_rejects_bad_name();
  printf("test_iso9660 OK\n");
  return 0;
}
