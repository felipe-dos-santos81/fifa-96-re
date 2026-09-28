// tests/test_qfs.c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_qfs.h"
int main(void) {
  uint8_t *q = 0, *p = 0; size_t nq = 0, np = 0;
  assert(fifa96_file_read("tests/golden/fw1.qfs", &q, &nq) == 0);
  assert(nq == 9951);
  fifa96_qfs_hdr_t h;
  assert(fifa96_qfs_parse_hdr(q, nq, &h) == 0);
  assert(h.magic[0] == 0x10 && h.magic[1] == 0xfb);
  assert(h.dec_len == 0xe440d400);                   // golden bytes 2-5: 00 d4 40 e4
  assert(memcmp(h.tag, "SHPI", 4) == 0);             // golden bytes 6-9: 53 48 50 49
  assert(fifa96_file_read("tests/golden/gameart0.pvi", &p, &np) == 0);
  assert(np == 154387);
  assert(memcmp(p + 6, "BIGF", 4) == 0);          // golden bytes 0006: BIGF
  printf("test_qfs OK\n");
  return 0;
}
