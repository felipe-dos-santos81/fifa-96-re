#include "fifa96_loader/fifa96_tree.h"

#include <string.h>

static uint32_t tree_be24(const uint8_t *p) {
  return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | (uint32_t)p[2];
}

// FU-25 §5.2, tree_emit_symbol (0x9D9D0): emit the leaves of node `n` with the
// childA prefix walk. Depth-limited so a cyclic table errors instead of
// recursing forever (the original has no guard).
static int tree_emit(const uint8_t *flags, const uint8_t *ca, const uint8_t *cb,
                     uint8_t node, unsigned depth, uint8_t *dst,
                     size_t declared, size_t *op) {
  if (depth > 512) return -1;
  while (flags[node]) {
    if (tree_emit(flags, ca, cb, ca[node], depth + 1, dst, declared, op))
      return -1;
    node = cb[node];
  }
  if (*op < declared) dst[*op] = node;
  (*op)++;
  return 0;
}

int fifa96_tree_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  if (!src || !dst || !out_len) return -(int)FIFA96_ERR_TRUNCATED;
  *out_len = 0;
  if (src_len < 2) return -(int)FIFA96_ERR_TRUNCATED;

  // FU-25 §3: 0x47FB magic bumps the payload base from +2 to +5.
  size_t base = ((((uint32_t)src[0] << 8) | src[1]) == 0x47FBu) ? 5 : 2;
  if (src_len < base + 5) return -(int)FIFA96_ERR_TRUNCATED;
  uint32_t declared = tree_be24(src + base);
  uint8_t root = src[base + 3];
  uint8_t count = src[base + 4];
  size_t ent = base + 5;
  if (src_len < ent + (size_t)count * 3) return -(int)FIFA96_ERR_TRUNCATED;
  if (dst_cap < declared) return -(int)FIFA96_ERR_TRUNCATED;

  // FU-25 §4: flags 0 = literal, 1 = root terminator, 0xFF = internal key.
  uint8_t flags[256], ca[256], cb[256];
  memset(flags, 0, sizeof flags);
  memset(ca, 0, sizeof ca);
  memset(cb, 0, sizeof cb);
  flags[root] = 1;
  for (uint8_t i = 0; i < count; i++) {
    uint8_t key = src[ent], a = src[ent + 1], b = src[ent + 2];
    ent += 3;
    ca[key] = a;
    cb[key] = b;
    flags[key] = 0xFF;
  }

  // FU-25 §5: byte-key decode loop; only the root-plus-zero byte terminates.
  size_t p = ent;
  size_t op = 0;
  for (;;) {
    if (p >= src_len) return -(int)FIFA96_ERR_TRUNCATED;
    uint8_t sym = src[p++];
    uint8_t f = flags[sym];
    if (f == 0) {
      if (op < declared) dst[op] = sym;
      op++;
      continue;
    }
    if ((f & 0x80u) == 0) {  // positive flag: root arm (FU-25 §5.1)
      if (p >= src_len) return -(int)FIFA96_ERR_TRUNCATED;
      uint8_t nxt = src[p++];
      if (nxt == 0) break;
      if (op < declared) dst[op] = nxt;
      op++;
      continue;
    }
    // Negative flag: key expansion, two-stage (FU-25 §5.2).
    if (tree_emit(flags, ca, cb, ca[sym], 0, dst, declared, &op))
      return -(int)FIFA96_ERR_TRUNCATED;
    if (tree_emit(flags, ca, cb, cb[sym], 0, dst, declared, &op))
      return -(int)FIFA96_ERR_TRUNCATED;
  }
  if (op < declared) return -(int)FIFA96_ERR_TRUNCATED;
  *out_len = declared;
  return FIFA96_OK;
}
