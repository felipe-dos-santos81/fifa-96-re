#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: huff_32fb_decode @ 0x9C7E0 (selectors 0x30/0x32/0x34, dispatched at 0x9E806).
// Spec: docs/ghidra/FU24_huff_opcodes.md (bit-packed canonical Huffman; run/
// escape/end rules; 0x32/0x34 byte prefix-sum transforms).
// src/src_len is the raw record starting at the [selector, 0xFB] header. The port
// is safe where the original is not: every bit read is bounds-checked against
// src_len, output writes are clamped at the declared length, and dst_cap must
// cover the declared length. Returns 0 (FIFA96_OK) on success, or the negated
// fifa96_err_t code on error (FIFA96_ERR_TRUNCATED for short/malformed input,
// a missing end marker, an output short of the declared length, or
// dst_cap < declared length); *out_len is set to the declared length on
// success, 0 on error.
int fifa96_huff_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_cap, size_t *out_len);
