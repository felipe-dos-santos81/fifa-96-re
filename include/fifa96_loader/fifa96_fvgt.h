#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: vgt_decode_f @ 0xADEFC, the 'fVGT' delta-frame arm of
// vgt_dispatch (0xAE4BC). Spec: docs/ghidra/FU33_fvgt_opcodes.md.
//
// Chunk layout (all fields little-endian):
//   +0  4   'fVGT' tag (LE32 0x54475666). The stream walker also uses the
//          declared total at +4 as its stride; vgt_decode_f never reads it.
//   +8  2   index count (width): row-stream ids < width are delta references
//   +10 2   raw 16-byte block count
//   +12 2   8-byte palette record count
//   +14 2   row-stream bits per entry
//   +16 4   unused in vgt_decode_f
//   +20 ..  index table: width pairs of signed 10-bit fields, LSB-first,
//           ((width*20+31)&~31)>>3 bytes
//   ...     raw 16-byte blocks, then palette records (count*8 bytes; first 4
//           bytes are a 4-colour palette, next 4 the 16 two-bit indices,
//           MSB pair first)
//   ...     row stream: canvas_w*canvas_h/16 block ids of `bits`, LSB-first,
//           ((bits*count+31)&~31)>>3 bytes
//
// `canvas_w`/`canvas_h` stand in for the keyframe geometry vgt_decode_f reads
// from ctx[0]/ctx[1] (pitch/height; FU-33 §4, §10 leg 1): the row-table term
// rowtab[height+b] reduces to b*canvas_w. `pre` is the previous canvas (the
// delta source; canvas_w*canvas_h bytes) and `dst` receives the post canvas;
// they must not alias.
//
// Returns 0 (FIFA96_OK) on success, or the negated fifa96_err_t code:
// FIFA96_ERR_BAD_MAGIC for a non-fVGT tag, FIFA96_ERR_TRUNCATED for NULL
// args, a short/malformed chunk (including any delta/block reference outside
// its surface, where the original reads unchecked) or dst_cap <
// canvas_w*canvas_h, FIFA96_ERR_UNSUPPORTED for canvases the original only
// partly rewrites (a dimension not a multiple of 4), > 2^31-1 bytes, or zero
// row bits (0xAE1BC skips the unpack, leaving the previous keyframe's ids).
// *out_len is canvas_w*canvas_h on success, 0 on error; the destination is
// never written on error.
int fifa96_fvgt_decode(const uint8_t *chunk, size_t chunk_len,
                       uint32_t canvas_w, uint32_t canvas_h,
                       const uint8_t *pre, uint8_t *dst, size_t dst_cap,
                       size_t *out_len);
