#include "fifa96_loader/fifa96_viv.h"
#include "fifa96_loader/fifa96_file.h"
fifa96_err_t fifa96_viv_entry_at(const uint8_t *b, size_t n, size_t idx, uint32_t *off) {
  if (!b || !off || (idx + 1) * 4 > n) return FIFA96_ERR_TRUNCATED;
  const uint8_t *p = b + idx * 4;
  *off = fifa96_read_u32le(p);
  return FIFA96_OK;
}
