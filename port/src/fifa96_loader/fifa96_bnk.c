#include "fifa96_loader/fifa96_bnk.h"

static uint32_t bnk_u32le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* Descriptor + EACS bounds shared by parse and the accessor. Returns the
 * embedded EACS offset through *eacs_off when non-NULL. */
static fifa96_err_t bnk_check_desc(const uint8_t *src, size_t src_len,
                                   uint32_t desc_off, uint32_t *eacs_off) {
  if ((desc_off & 3u) != 0 || desc_off < 0x200u ||
      (uint64_t)desc_off + FIFA96_BNK_DESC_SIZE > (uint64_t)src_len)
    return FIFA96_ERR_TRUNCATED;
  uint32_t eacs = bnk_u32le(src + desc_off + 4u);
  if ((eacs & 3u) != 0 || eacs < 0x200u ||
      (uint64_t)eacs + FIFA96_BNK_EACS_SIZE > (uint64_t)src_len)
    return FIFA96_ERR_TRUNCATED;
  if (eacs_off) *eacs_off = eacs;
  return FIFA96_OK;
}

fifa96_err_t fifa96_bnk_parse(const uint8_t *src, size_t src_len,
                              struct fifa96_bnk_info *info) {
  if (!src || !info) return FIFA96_ERR_TRUNCATED;
  if (src_len < 0x200u) return FIFA96_ERR_TRUNCATED;
  // The table/descriptor/payload offsets are 32-bit file offsets.
  if (src_len > (size_t)UINT32_MAX) return FIFA96_ERR_UNSUPPORTED;

  struct fifa96_bnk_info out;
  out.src = src;
  out.src_len = src_len;
  out.entry_count = 0;
  for (uint32_t id = 0; id < FIFA96_BNK_IDS; id++) {
    out.table[id] = bnk_u32le(src + 4u * id);
    out.payload_off[id] = 0;
    out.payload_len[id] = 0;
  }

  // FU-43 §1: a nonzero slot is a descriptor offset; descriptor+0x04 points at
  // the embedded EACS whose +0x18 is the absolute payload file offset. The
  // original registrar relocates without checking; this port bounds-checks.
  for (uint32_t id = 0; id < FIFA96_BNK_IDS; id++) {
    uint32_t desc = out.table[id];
    if (desc == 0) continue;
    uint32_t eacs = 0;
    fifa96_err_t rc = bnk_check_desc(src, src_len, desc, &eacs);
    if (rc != FIFA96_OK) return rc;
    uint32_t pay = bnk_u32le(src + eacs + 0x18u);
    if ((pay & 3u) != 0 || pay < 0x200u || (size_t)pay > src_len)
      return FIFA96_ERR_TRUNCATED;
    out.payload_off[id] = pay;
    out.entry_count++;
  }

  // Payload extent = distance to the next payload offset in any slot, or to
  // the file end for the last one; the 0..3 alignment gap bytes belong to the
  // preceding payload (FU-43 §1).
  for (uint32_t id = 0; id < FIFA96_BNK_IDS; id++) {
    if (out.table[id] == 0) continue;
    uint32_t next = (uint32_t)src_len;
    for (uint32_t j = 0; j < FIFA96_BNK_IDS; j++) {
      if (out.table[j] == 0) continue;
      uint32_t po = out.payload_off[j];
      if (po > out.payload_off[id] && po < next) next = po;
    }
    out.payload_len[id] = next - out.payload_off[id];
  }

  *info = out;
  return FIFA96_OK;
}

fifa96_err_t fifa96_bnk_entry(const struct fifa96_bnk_info *info, uint32_t id,
                              struct fifa96_bnk_entry *out) {
  if (!info || !out || !info->src) return FIFA96_ERR_TRUNCATED;
  if (id >= FIFA96_BNK_IDS) return FIFA96_ERR_TRUNCATED;
  if (info->table[id] == 0) return FIFA96_ERR_NOT_FOUND;

  // Defensive re-validation so a hand-built info cannot walk out of bounds.
  uint32_t eacs = 0;
  fifa96_err_t rc = bnk_check_desc(info->src, info->src_len, info->table[id], &eacs);
  if (rc != FIFA96_OK) return rc;
  uint32_t pay = info->payload_off[id];
  if ((pay & 3u) != 0 || pay < 0x200u || (size_t)pay > info->src_len ||
      (uint64_t)pay + info->payload_len[id] > (uint64_t)info->src_len)
    return FIFA96_ERR_TRUNCATED;

  out->id = id;
  out->desc_off = info->table[id];
  out->eacs_off = eacs;
  out->payload_off = pay;
  out->payload_len = info->payload_len[id];
  return FIFA96_OK;
}
