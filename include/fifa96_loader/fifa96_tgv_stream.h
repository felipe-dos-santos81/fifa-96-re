#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: vgt_stream_poll @ 0x67BA8 walks length-prefixed chunks through
// FUN_00095cb3 (advance by the u32 length at chunk+4) and dispatches each
// chunk whose first dword is a VGT tag (0xAE4BC). Spec: FU-29 errata (chunk
// layout) and FU-30.
//
// VIDEO/*.TGV chunk layout: [u32 tag][u32 length][length-8 payload bytes].
// kVGT frame chunks (tag 0x5447566B) parse with fifa96_kvgt_decode; other
// tags (observed 0x684E5331 / 0x644E5331, lengths 0x1068-0x1088) are
// skipped by the walker.
//
// fifa96_tgv_walk_next returns 1 when a chunk is yielded (*chunk/*chunk_len/
// *is_frame set), 0 at the end of the buffer, or a negated fifa96_err_t code
// for a malformed chunk (length < 8 or extending past the buffer).
typedef struct {
  const uint8_t *data;
  size_t size;
  size_t pos;
} fifa96_tgv_walk;

int fifa96_tgv_walk_next(fifa96_tgv_walk *walk, const uint8_t **chunk, size_t *chunk_len, int *is_frame);
