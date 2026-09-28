#include "fifa96_loader/fifa96_tables.h"
#include <string.h>
#define REC 12u  /* golden fnames.dat: 2604 B = 217 * 12 (8.3 name + 4 pad) — Ruling v2 */
fifa96_err_t fifa96_tables_entry_count(const uint8_t *fnames, size_t n, size_t *count) {
  if (!fnames || !count || n % REC) return FIFA96_ERR_TRUNCATED;
  *count = n / REC;
  return FIFA96_OK;
}
fifa96_err_t fifa96_tables_name_at(const uint8_t *fnames, size_t n, size_t idx, char out[16]) {
  size_t c = 0;
  if (fifa96_tables_entry_count(fnames, n, &c) != 0 || idx >= c) return FIFA96_ERR_TRUNCATED;
  memcpy(out, fnames + idx * REC, REC);
  out[15] = 0;
  return FIFA96_OK;
}
fifa96_err_t fifa96_tables_length_at(const uint8_t *lengths, size_t n, size_t idx, uint32_t *val) {
  if (!lengths || !val || (idx + 1) * 4 > n) return FIFA96_ERR_TRUNCATED;
  const uint8_t *p = lengths + idx * 4;
  *val = (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
  return FIFA96_OK;
}
