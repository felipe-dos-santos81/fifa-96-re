// tests/test_envelope.c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_envelope.h"
int main(void) {
  uint8_t *q = 0, *p = 0, *g = 0; size_t nq = 0, np = 0, ng = 0;
  fifa96_envelope_hdr_t h;
  /* fw1.qfs: xxd -s 0 -l 24 tests/golden/fw1.qfs */
  assert(fifa96_file_read("tests/golden/fw1.qfs", &q, &nq) == 0);
  assert(nq == 9951);
  assert(fifa96_envelope_parse_hdr(q, nq, &h) == 0);
  assert(h.magic == 0xfb10);                 /* bytes 0-1: 10 fb */
  assert(h.word_a == 0xd400);                /* bytes 2-3: 00 d4 */
  assert(h.word_b == 0xe440);                /* bytes 4-5: 40 e4 */
  assert(memcmp(h.tag, "SHPI", 4) == 0);     /* bytes 6-9: 53 48 50 49 */
  assert(h.tail_off == 10);
  assert(h.tail_len == nq - 10);
  assert(memcmp(q + 0x12, "GIMX", 4) == 0);  /* xxd -s 0x12 -l 4: 47 49 4d 58 */
  fifa96_file_free(q);
  /* pcindex.pog: xxd -s 0 -l 24 tests/golden/pcindex.pog */
  assert(fifa96_file_read("tests/golden/pcindex.pog", &p, &np) == 0);
  assert(np == 25857);
  assert(fifa96_envelope_parse_hdr(p, np, &h) == 0);
  assert(h.magic == 0xfb10);
  assert(h.word_a == 0xb600);                /* bytes 2-3: 00 b6 */
  assert(h.word_b == 0xe140);                /* bytes 4-5: 40 e1 */
  assert(memcmp(h.tag, "PCNX", 4) == 0);     /* bytes 6-9: 50 43 4e 58 */
  fifa96_file_free(p);
  /* gameart0.pvi: xxd -s 0 -l 24 tests/golden/gameart0.pvi */
  assert(fifa96_file_read("tests/golden/gameart0.pvi", &g, &ng) == 0);
  assert(ng == 154387);
  assert(fifa96_envelope_parse_hdr(g, ng, &h) == 0);
  assert(h.magic == 0xfb10);
  assert(h.word_a == 0x1704);                /* bytes 2-3: 04 17 */
  assert(h.word_b == 0xe3a2);                /* bytes 4-5: a2 e3 */
  assert(memcmp(h.tag, "BIGF", 4) == 0);     /* bytes 6-9: 42 49 47 46 */
  fifa96_file_free(g);
  /* negative paths (no I/O) */
  uint8_t z[16] = {0};
  assert(fifa96_envelope_parse_hdr(0, 16, &h) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_envelope_parse_hdr(z, 16, 0) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_envelope_parse_hdr(z, 9, &h) == FIFA96_ERR_TRUNCATED);
  assert(fifa96_envelope_parse_hdr(z, 16, &h) == FIFA96_ERR_BAD_MAGIC);
  printf("test_envelope OK\n");
  return 0;
}
