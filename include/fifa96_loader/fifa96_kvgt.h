#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: vgt_decode_k @ 0xAE218 (kVGT arm of vgt_dispatch 0xAE4BC, tag
// 0x5447566B = 'kVGT'). Spec: the disassembled field reads at 0xAE22B..0xAE2A4
// (each `MOV EAX,[p+6]; SHR EAX,16` = LE16 at frame+8/10/12/14) and the
// record call at 0xAE45A..0xAE474.
//
// Frame chunk layout (as stored in VIDEO/*.TGV and passed to vgt_decode_k):
//   +0  'kVGT' tag dword (LE 0x5447566B)
//   +4  u32 chunk length (total bytes; the stream walker's stride)
//   +8  LE16 width
//   +10 LE16 height
//   +12 LE16 count      (row/block count; ctx[2])
//   +14 LE16 palette_count (RGB triples at +0x14, ctx[3])
//   +0x14 palette_count RGB triples
//   ... record = frame + 0x14 + palette_count*3, decoded by
//   fifa96_record_decode.
//
// vgt_stream_poll (0x67BA8) only walks these length-prefixed chunks (via
// FUN_00095cb3) and dispatches each; the file bytes are the in-memory bytes.
// FU-29 errata corrected the earlier BE16 reading (see FU29 doc).
//
// Returns 0 (FIFA96_OK) on success, or the negated fifa96_err_t code on error
// (FIFA96_ERR_BAD_MAGIC for a non-kVGT tag, FIFA96_ERR_TRUNCATED for short
// input, dst_cap < decoded length, palette_count > 256, or any record-decode
// error code passed through). `info` may be NULL; when non-NULL it is filled
// with the parsed header and palette on success. *out_len is the decoded
// record length (the original returns 1/0 as a flag, not a length).
struct fifa96_kvgt_info {
  uint32_t width;
  uint32_t height;
  uint32_t count;
  uint32_t palette_count;
  uint8_t palette[256 * 3];
};

int fifa96_kvgt_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_cap, size_t *out_len,
                       struct fifa96_kvgt_info *info);
