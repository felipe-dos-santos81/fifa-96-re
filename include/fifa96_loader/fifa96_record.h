#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: decode_record_dispatch @ 0x9E718 (selector tree 0x9E757..0x9E840).
// Spec: docs/ghidra/FU19_vgt_decode_kernels.md §1 and the FU-21 §2 selector
// errata. src/src_len is the raw record starting at the [selector, 0xFB]
// header; the selector is src[0] & 0xFE (0x9E759 AND AL,0xFE).
//
// Routed here: 0x10 refpack, 0x30/0x32/0x34 huff, 0x46 tree, and the
// 0x6A/0x6E literal copy (BE24 length at src+2, payload at src+5, return =
// length). Selectors 0x16 (lz_16fb), 0x60/0x62/0x66/0x72 (delta_prefix) and
// 0x7A (rle_row) are known but not ported: they return
// -(int)FIFA96_ERR_UNSUPPORTED. An unknown selector or a non-0xFB signature
// byte returns -(int)FIFA96_ERR_BAD_MAGIC (the original's strict=1 error
// path at 0x9E840 / silent 0 return at 0x9E74C respectively).
//
// Returns 0 (FIFA96_OK) on success, or the negated fifa96_err_t code on
// error; *out_len is set on success, 0 on error. The routed decoders apply
// their own bounds checks and dst_cap >= declared-length requirement.
int fifa96_record_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_cap, size_t *out_len);
