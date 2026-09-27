#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_file.h"
int main(void) {
  uint8_t *buf = 0; size_t len = 0;
  // Golden: soccer/fnames.dat starts with "FW1.QFS\0"
  assert(fifa96_file_read("tests/golden/fnames.dat", &buf, &len) == 0);
  assert(len > 16);
  assert(memcmp(buf, "FW1.QFS", 7) == 0);
  // Segment-split helper: offset 0xFF00 + 0x200 must split at 0x100
  assert(fifa96_file_split_for_segment(0xFF00, 0x200) == 0x100);
  printf("test_file OK len=%zu\n", len);
  return 0;
}
