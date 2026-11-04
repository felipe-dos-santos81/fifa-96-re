#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_loader/fifa96_trace.h"

/* append frame type/seq/len/payload to a byte vector */
static void put(uint8_t *b, size_t *n, uint8_t t, uint16_t seq,
                const uint8_t *p, uint16_t plen) {
  b[(*n)++] = t; b[(*n)++] = (uint8_t)(seq & 0xff); b[(*n)++] = (uint8_t)(seq >> 8);
  b[(*n)++] = (uint8_t)(plen & 0xff); b[(*n)++] = (uint8_t)(plen >> 8);
  if (p) memcpy(b + *n, p, plen);
  *n += plen;
}

static void expect_has(const char *hay, const char *needle) {
  if (!strstr(hay, needle)) { fprintf(stderr, "missing: %s\n---\n%s\n", needle, hay); assert(0); }
}

static void w8(uint8_t *b, size_t *o, uint8_t v) { b[(*o)++] = v; }
static void w16(uint8_t *b, size_t *o, uint16_t v) {
  b[(*o)++] = (uint8_t)(v & 0xff); b[(*o)++] = (uint8_t)(v >> 8);
}
static void w32(uint8_t *b, size_t *o, uint32_t v) {
  b[(*o)++] = (uint8_t)(v & 0xff); b[(*o)++] = (uint8_t)((v >> 8) & 0xff);
  b[(*o)++] = (uint8_t)((v >> 16) & 0xff); b[(*o)++] = (uint8_t)((v >> 24) & 0xff);
}

static size_t mk_file(uint8_t *p, uint8_t ah, uint16_t bx, uint16_t cx,
                      uint16_t ds, uint16_t dx, uint16_t ax_after, uint8_t flags,
                      uint32_t hash, const uint8_t *data, uint16_t dlen) {
  size_t o = 0;
  w8(p, &o, ah); w16(p, &o, bx); w16(p, &o, cx); w16(p, &o, ds); w16(p, &o, dx);
  w16(p, &o, ax_after); w8(p, &o, flags); w32(p, &o, hash); w16(p, &o, dlen);
  if (data) memcpy(p + o, data, dlen);
  o += dlen;
  return o;
}

static size_t mk_codec(uint8_t *p, uint8_t site, uint16_t flags, uint32_t hash,
                       uint16_t dseg, uint16_t doff, const uint8_t *data, uint16_t dlen) {
  size_t o = 0;
  w8(p, &o, site);
  for (int i = 0; i < 13; i++) w16(p, &o, (uint16_t)(0x1000 + i));
  w16(p, &o, flags); w32(p, &o, hash); w16(p, &o, dseg); w16(p, &o, doff); w16(p, &o, dlen);
  if (data) memcpy(p + o, data, dlen);
  o += dlen;
  return o;
}

static void test_header_and_end(void) {
  uint8_t buf[64]; size_t n = 0; char *s = 0;
  uint8_t hdr[6] = {'F','C','A','P',1,5};
  put(buf,&n,0x01,0,hdr,6);
  put(buf,&n,0x06,0,0,0);
  assert(fifa96_trace_format(buf,n,&s) == FIFA96_OK);
  expect_has(s,"HEADER magic=FCAP version=1 patches=5");
  expect_has(s,"END");
  free(s);
}

static void test_bad_header(void) {
  uint8_t buf[8]; size_t n = 0; char *s = 0;
  buf[n++] = 0x00; buf[n++] = 0x11; buf[n++] = 0x22; buf[n++] = 0x33;
  assert(fifa96_trace_format(buf,n,&s) == FIFA96_ERR_BAD_MAGIC);
  expect_has(s,"BAD-HEADER");
  free(s);
}

static void test_seq_gap(void) {
  uint8_t buf[128]; size_t n = 0; char *s = 0;
  uint8_t hdr[6] = {'F','C','A','P',1,5};
  uint8_t f[64]; uint8_t data[4] = {1,2,3,4};
  uint16_t fl = (uint16_t)mk_file(f,0x3F,1,16,0,0,16,0x05,0,data,4);
  put(buf,&n,0x01,0,hdr,6);
  put(buf,&n,0x02,0,f,fl);
  put(buf,&n,0x02,2,f,fl);
  assert(fifa96_trace_format(buf,n,&s) == FIFA96_OK);
  expect_has(s,"SEQ-GAP type=FILE expected=1 got=2");
  free(s);
}

static void test_resync(void) {
  uint8_t buf[64]; size_t n = 0; char *s = 0;
  uint8_t hdr[6] = {'F','C','A','P',1,5};
  put(buf,&n,0x01,0,hdr,6);
  buf[n++] = 0x00; buf[n++] = 0xFF; buf[n++] = 0x00;
  put(buf,&n,0x06,0,0,0);
  assert(fifa96_trace_format(buf,n,&s) == FIFA96_OK);
  expect_has(s,"LOST-SYNC off=0x000b");
  expect_has(s,"END");
  free(s);
}

static void test_missing_end(void) {
  uint8_t buf[64]; size_t n = 0; char *s = 0;
  uint8_t hdr[6] = {'F','C','A','P',1,5};
  uint8_t f[64]; uint8_t data[4] = {9,9,9,9};
  uint16_t fl = (uint16_t)mk_file(f,0x3F,1,16,0,0,16,0x05,0xdeadbeef,data,4);
  put(buf,&n,0x01,0,hdr,6);
  put(buf,&n,0x02,0,f,fl);
  assert(fifa96_trace_format(buf,n,&s) == FIFA96_OK);
  expect_has(s,"no-END");
  free(s);
}

static void test_patch_skip_reasons(void) {
  uint8_t buf[64]; size_t n = 0; char *s = 0;
  uint8_t hdr[6] = {'F','C','A','P',1,5};
  uint8_t sk[2];
  put(buf,&n,0x01,0,hdr,6);
  sk[0]=0; sk[1]=0; put(buf,&n,0x05,0,sk,2);
  sk[0]=1; sk[1]=1; put(buf,&n,0x05,0,sk,2);
  sk[0]=2; sk[1]=2; put(buf,&n,0x05,0,sk,2);
  assert(fifa96_trace_format(buf,n,&s) == FIFA96_OK);
  expect_has(s,"PATCH_SKIP site=5aa1 reason=bad-signature");
  expect_has(s,"PATCH_SKIP site=5aa8 reason=already-patched");
  expect_has(s,"PATCH_SKIP site=5f6e reason=site-out-of-range");
  expect_has(s,"patch: ok=0 skip=3");
  free(s);
}

static void test_patch_ok(void) {
  uint8_t buf[64]; size_t n = 0; char *s = 0;
  uint8_t hdr[6] = {'F','C','A','P',1,5};
  uint8_t ok[6];
  put(buf,&n,0x01,0,hdr,6);
  ok[0]=2; ok[1]=0x45; ok[2]=0x23; ok[3]=0x01; ok[4]=0x00; ok[5]=4;
  put(buf,&n,0x07,0,ok,6);
  assert(fifa96_trace_format(buf,n,&s) == FIFA96_OK);
  expect_has(s,"PATCH_OK site=5f6e target=0x00012345 siglen=4");
  expect_has(s,"patch: ok=1 skip=0");
  free(s);
}

static void test_codec(void) {
  uint8_t buf[128]; size_t n = 0; char *s = 0;
  uint8_t hdr[6] = {'F','C','A','P',1,5};
  uint8_t c[64]; uint8_t data[4] = {0xAA,0xBB,0xCC,0xDD};
  uint16_t cl = (uint16_t)mk_codec(c,2,0x0001,0xCAFEBABE,0x1000,0x0200,data,4);
  put(buf,&n,0x01,0,hdr,6);
  put(buf,&n,0x03,0,c,cl);
  assert(fifa96_trace_format(buf,n,&s) == FIFA96_OK);
  expect_has(s,"CODEC site=5f6e ");
  expect_has(s,"hash=cafebabe");
  expect_has(s,"head=aabbccdd");
  free(s);
}

static void test_heartbeat(void) {
  uint8_t buf[64]; size_t n = 0; char *s = 0;
  uint8_t hdr[6] = {'F','C','A','P',1,5};
  uint8_t hb[4] = {0x00,0x04,0x00,0x00};
  put(buf,&n,0x01,0,hdr,6);
  put(buf,&n,0x04,0,hb,4);
  assert(fifa96_trace_format(buf,n,&s) == FIFA96_OK);
  expect_has(s,"HEARTBEAT files=1024");
  free(s);
}

static void test_raw_mode(void) {
  uint8_t buf[64]; size_t n = 0; char *s = 0;
  uint8_t hdr[6] = {'F','C','A','P',1,5};
  put(buf,&n,0x01,0,hdr,6);
  assert(fifa96_trace_raw(buf,n,&s) == FIFA96_OK);
  expect_has(s,"RAW type=01");
  free(s);
}

static void test_file_name_resolves(void) {
  uint8_t buf[256]; size_t n = 0; char *s = 0;
  uint8_t hdr[6] = {'F','C','A','P',1,5};
  uint8_t f[64]; uint16_t fl;
  const uint8_t nm[7] = {'F','W','1','.','Q','F','S'};
  const uint8_t rd[4] = {0xDE,0xAD,0xBE,0xEF};
  put(buf,&n,0x01,0,hdr,6);
  fl = (uint16_t)mk_file(f,0x3D,0,0,0,0,3,0x02,0,nm,7);
  put(buf,&n,0x02,0,f,fl);
  fl = (uint16_t)mk_file(f,0x3F,3,1024,0,0,1024,0x05,0x8a3f21b0,rd,4);
  put(buf,&n,0x02,1,f,fl);
  put(buf,&n,0x06,0,0,0);
  assert(fifa96_trace_format(buf,n,&s) == FIFA96_OK);
  expect_has(s,"FILE ah=3D");
  expect_has(s,"FILE ah=3F h=0x0003 x=1024 got=1024 name=FW1.QFS hash=8a3f21b0");
  expect_has(s,"FW1.QFS opens=1 reads=1 bytes=1024");
  free(s);
}

static void test_failed_ops_uncounted(void) {
  uint8_t buf[256]; size_t n = 0; char *s = 0;
  uint8_t hdr[6] = {'F','C','A','P',1,5};
  uint8_t f[64]; uint16_t fl;
  const uint8_t nm[7] = {'F','W','1','.','Q','F','S'};
  put(buf,&n,0x01,0,hdr,6);
  /* success: open binds handle 3 -> name, counted as one open */
  fl = (uint16_t)mk_file(f,0x3D,0,0,0,0,3,0x02,0,nm,7);
  put(buf,&n,0x02,0,f,fl);
  /* failure: open CF=1 (flags=0, AX=0x0002) -> emitted but not counted */
  fl = (uint16_t)mk_file(f,0x3D,0,0,0,0,2,0,0,0,0);
  put(buf,&n,0x02,1,f,fl);
  /* failure: read CF=1 (flags=0, AX=0x0005) -> emitted but not counted */
  fl = (uint16_t)mk_file(f,0x3F,3,1024,0,0,5,0,0,0,0);
  put(buf,&n,0x02,2,f,fl);
  put(buf,&n,0x06,0,0,0);
  assert(fifa96_trace_format(buf,n,&s) == FIFA96_OK);
  expect_has(s,"FILE ah=3D h=0x0000 x=0 got=2");
  expect_has(s,"FILE ah=3F h=0x0003 x=1024 got=5 name=FW1.QFS");
  expect_has(s,"FW1.QFS opens=1 reads=0 bytes=0");
  expect_has(s,"SUMMARY opens=1 reads=0");
  free(s);
}

int main(void) {
  test_header_and_end();
  test_bad_header();
  test_seq_gap();
  test_resync();
  test_missing_end();
  test_patch_skip_reasons();
  test_patch_ok();
  test_codec();
  test_heartbeat();
  test_raw_mode();
  test_file_name_resolves();
  test_failed_ops_uncounted();
  printf("test_trace OK\n");
  return 0;
}
