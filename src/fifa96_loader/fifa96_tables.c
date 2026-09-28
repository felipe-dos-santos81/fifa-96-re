#include "fifa96_loader/fifa96_tables.h"
#include "fifa96_loader/fifa96_file.h"
#include <string.h>
#define REC 12u  /* golden fnames.dat: 2604 B = 217 * 12 (fixed-width 12-byte name field) — Ruling v2 */
fifa96_err_t fifa96_tables_entry_count(const uint8_t *fnames, size_t n, size_t *count) {
  if (!fnames || !count || n % REC) return FIFA96_ERR_TRUNCATED;
  *count = n / REC;
  return FIFA96_OK;
}
fifa96_err_t fifa96_tables_name_at(const uint8_t *fnames, size_t n, size_t idx, char out[16]) {
  size_t c = 0;
  if (fifa96_tables_entry_count(fnames, n, &c) != 0 || idx >= c) return FIFA96_ERR_TRUNCATED;
  memcpy(out, fnames + idx * REC, REC);
  memset(out + REC, 0, sizeof(out[0]) * (16 - REC));
  out[15] = 0;
  return FIFA96_OK;
}
fifa96_err_t fifa96_tables_length_at(const uint8_t *lengths, size_t n, size_t idx, uint32_t *val) {
  if (!lengths || !val || (idx + 1) * 4 > n) return FIFA96_ERR_TRUNCATED;
  const uint8_t *p = lengths + idx * 4;
  *val = fifa96_read_u32le(p);
  return FIFA96_OK;
}
