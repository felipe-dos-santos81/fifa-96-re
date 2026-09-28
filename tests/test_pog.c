#include <assert.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_pog.h"
int main(void) {
  uint8_t *b = 0; size_t n = 0;
  assert(fifa96_file_read("tests/golden/pcindex.pog", &b, &n) == 0);
  assert(n == 25857);                             // golden size
  fifa96_pog_hdr_t h;
  assert(fifa96_pog_parse_hdr(b, n, &h) == 0);
  assert(h.magic[0] == 0x10 && h.magic[1] == 0xfb); // golden bytes 0000: 10 fb
  printf("test_pog OK\n");
  return 0;
}
