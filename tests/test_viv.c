#include <assert.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_viv.h"
int main(void) {
  uint8_t *b = 0; size_t n = 0;
  assert(fifa96_file_read("tests/golden/sfx_game.bnk", &b, &n) == 0);
  assert(n == 173016);
  uint32_t off = 0;
  assert(fifa96_viv_entry_at(b, n, 0, &off) == 0);
  assert(off == 0x00000000);                       // golden bytes 0000: 00 00 00 00
  assert(fifa96_viv_entry_at(b, n, 1, &off) == 0);
  assert(off == 0x00000200);                       // golden bytes 0008: 02 00 ...
  printf("test_viv OK\n");
  return 0;
}
