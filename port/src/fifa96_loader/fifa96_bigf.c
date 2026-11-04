#include "fifa96_loader/fifa96_bigf.h"
#include <string.h>

static uint32_t bigf_u32be(const uint8_t *p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/* Sequential walk shared by parse (validation) and the record accessor:
 * records start at 0x10, each is [BE32 off][BE32 size][name NUL-terminated],
 * and the next record starts at the name's NUL + 1 (FU-41 §1). */
static fifa96_err_t bigf_walk(const uint8_t *src, uint32_t table_end, uint32_t index,
                              uint32_t *off, uint32_t *size, const char **name) {
  if (table_end < 0x10u) return FIFA96_ERR_TRUNCATED;
  size_t p = 0x10;
  for (uint32_t i = 0; i <= index; i++) {
    if (p > (size_t)table_end - 8u) return FIFA96_ERR_TRUNCATED;
    const uint8_t *nul = (const uint8_t *)memchr(src + p + 8u, 0,
                                                 (size_t)table_end - p - 8u);
    if (!nul) return FIFA96_ERR_TRUNCATED;   /* name crosses the directory end */
    if (i == index) {
      if (off) *off = bigf_u32be(src + p);
      if (size) *size = bigf_u32be(src + p + 4u);
      if (name) *name = (const char *)(src + p + 8u);
      return FIFA96_OK;
    }
    p = (size_t)(nul - src) + 1u;
  }
  return FIFA96_ERR_TRUNCATED;   /* unreachable for index < count */
}

fifa96_err_t fifa96_bigf_parse(const uint8_t *src, size_t src_len,
                               struct fifa96_bigf_info *info) {
  if (!src || !info) return FIFA96_ERR_TRUNCATED;
  if (src_len < 0x10) return FIFA96_ERR_TRUNCATED;
  // The container's size/count/offset fields are 32-bit.
  if (src_len > (size_t)UINT32_MAX) return FIFA96_ERR_UNSUPPORTED;
  if (!(src[0] == 'B' && src[1] == 'I' && src[2] == 'G' && src[3] == 'F'))
    return FIFA96_ERR_BAD_MAGIC;
  uint32_t size = bigf_u32be(src + 4);
  if ((size_t)size != src_len) return FIFA96_ERR_TRUNCATED;
  uint32_t count = bigf_u32be(src + 8);
  uint32_t table_end = bigf_u32be(src + 0x0c);
  if (table_end < 0x10u || (size_t)table_end > src_len) return FIFA96_ERR_TRUNCATED;
  // Every record needs at least the 8-byte off/size pair plus its NUL.
  if ((uint64_t)count * 9u > (uint64_t)(table_end - 0x10u)) return FIFA96_ERR_TRUNCATED;

  uint32_t off = 0, rsize = 0;
  size_t p = 0x10;
  for (uint32_t i = 0; i < count; i++) {
    if (p > (size_t)table_end - 8u) return FIFA96_ERR_TRUNCATED;
    const uint8_t *nul = (const uint8_t *)memchr(src + p + 8u, 0,
                                                 (size_t)table_end - p - 8u);
    if (!nul) return FIFA96_ERR_TRUNCATED;
    off = bigf_u32be(src + p);
    rsize = bigf_u32be(src + p + 4u);
    if ((uint64_t)off + (uint64_t)rsize > (uint64_t)src_len) return FIFA96_ERR_TRUNCATED;
    p = (size_t)(nul - src) + 1u;
  }

  info->src = src;
  info->src_len = src_len;
  info->size = size;
  info->count = count;
  info->table_end = table_end;
  return FIFA96_OK;
}

fifa96_err_t fifa96_bigf_record(const struct fifa96_bigf_info *info, size_t index,
                                uint32_t *off, uint32_t *size, const char **name) {
  if (!info || !info->src) return FIFA96_ERR_TRUNCATED;
  // Defensive: callers normally pass the output of a successful parse.
  if (info->table_end > info->src_len) return FIFA96_ERR_TRUNCATED;
  if (index >= (size_t)info->count) return FIFA96_ERR_TRUNCATED;
  return bigf_walk(info->src, info->table_end, (uint32_t)index, off, size, name);
}
