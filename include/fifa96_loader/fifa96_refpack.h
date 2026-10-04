#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: lz_refpack_decode @ 0xB18F8 (selector 0x10/0x11, dispatched at 0x9E7E8).
// Spec: docs/ghidra/FU22_refpack_opcodes.md (classes A/B/C/D, 0xFC-0xFF stop).
// src/src_len is the raw record starting at the [cmd, 0xFB] header. The port is
// safe where the original is not: source reads are bounds-checked, dst writes are
// clamped at the declared BE24 length (the original's final overrun is dropped),
// and dst_cap must cover the declared length. Returns 0 (FIFA96_OK) on success,
// or the negated fifa96_err_t code on error (FIFA96_ERR_TRUNCATED for short or
// malformed input, a missing stop code, or dst_cap < declared length); *out_len
// is set to the declared length on success, 0 on error.
int fifa96_refpack_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_cap, size_t *out_len);
