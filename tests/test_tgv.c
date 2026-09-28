#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_tgv.h"
int main(void) {
  uint8_t *b = 0; size_t n = 0;
  assert(fifa96_file_read("tests/golden/vid_game.tgv", &b, &n) == 0);
  assert(n == 8261652);
  fifa96_tgv_hdr_t h;
  assert(fifa96_tgv_parse_hdr(b, n, &h) == 0);
  assert(memcmp(h.magic, "kVGT", 4) == 0);         // golden bytes 0000: 6b 56 47 54
  assert(h.v0 == 0x9a14);                          // golden bytes 4-7: 14 9a 00 00
  printf("test_tgv OK\n");
  return 0;
}
