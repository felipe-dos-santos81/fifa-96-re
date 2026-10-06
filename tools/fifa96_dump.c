// tools/fifa96_dump.c — CLI driver: detect and print FIFA96 CD container headers.
#include <stdint.h>
#include <stdio.h>
#include "fifa96_detect.h"
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_file.h"

static void dump_hex(const uint8_t *b, size_t n) {
  size_t i;
  for (i = 0; i < n && i < 16; i++) {
    printf("%02x%s", b[i], (i % 4 == 3) ? " " : "");
  }
  printf("\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s FILE\n", argv[0]);
    return 2;
  }
  uint8_t *b = 0;
  size_t n = 0;
  fifa96_err_t e = fifa96_file_read(argv[1], &b, &n);
  if (e != FIFA96_OK) {
    fprintf(stderr, "read failed (%d)\n", e);
    return 1;
  }
  fifa96_detect_kind_t kind = fifa96_detect_kind(b, n);
  printf("%s: %zu bytes\n", argv[1], n);
  printf("  kind: %s\n", fifa96_detect_kind_name(kind));
  fifa96_detect_summary(b, n, "  ");
  if (kind == FIFA96_DETECT_UNKNOWN) dump_hex(b, n);
  fifa96_file_free(b);
  return 0;
}
