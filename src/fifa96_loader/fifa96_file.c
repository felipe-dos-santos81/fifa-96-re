#include "fifa96_loader/fifa96_file.h"
#include <stdio.h>
#include <stdlib.h>
size_t fifa96_file_split_for_segment(uint16_t seg_off, size_t len) {
  size_t to_boundary = (size_t)(0x10000u - (unsigned)seg_off);
  return len < to_boundary ? len : to_boundary;
}
void fifa96_file_free(uint8_t *p) { free(p); }
fifa96_err_t fifa96_file_read(const char *path, uint8_t **out, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f) return FIFA96_ERR_NOT_FOUND;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n < 0) { fclose(f); return FIFA96_ERR_IO; }
  uint8_t *b = (uint8_t *)malloc((size_t)n ? (size_t)n : 1u);
  if (!b) { fclose(f); return FIFA96_ERR_IO; }
  size_t got = fread(b, 1, (size_t)n, f);
  fclose(f);
  if (got != (size_t)n) { free(b); return FIFA96_ERR_SHORT_READ; }
  *out = b; *out_len = got;
  return FIFA96_OK;
}
fifa96_err_t fifa96_file_read_chunk(const char *path, uint64_t off, uint8_t *dst, size_t len) {
  FILE *f = fopen(path, "rb");
  if (!f) return FIFA96_ERR_NOT_FOUND;
  if (fseek(f, (long)off, SEEK_SET) != 0) { fclose(f); return FIFA96_ERR_IO; }
  size_t got = fread(dst, 1, len, f);
  fclose(f);
  return got == len ? FIFA96_OK : FIFA96_ERR_SHORT_READ;
}
