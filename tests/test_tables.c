#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "fifa96_loader/fifa96_file.h"
#include "fifa96_loader/fifa96_tables.h"
int main(void) {
  uint8_t *fn = 0, *ln = 0; size_t nfn = 0, nln = 0;
  assert(fifa96_file_read("tests/golden/fnames.dat", &fn, &nfn) == 0);
  assert(fifa96_file_read("tests/golden/lengths.dat", &ln, &nln) == 0);
  char name[16]; uint32_t v = 0;
  assert(fifa96_tables_name_at(fn, nfn, 0, name) == 0);
  assert(memcmp(name, "FW1.QFS", 7) == 0);        // golden bytes 0000: FW1.QFS (12-byte record, 4 pad zeros)
  assert(fifa96_tables_length_at(ln, nln, 0, &v) == 0);
  assert(v == 0x26df);                            // golden bytes 0000: df 26 00 00
  printf("test_tables OK\n");
  return 0;
}
