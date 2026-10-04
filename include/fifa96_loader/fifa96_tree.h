#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: tree_47fb_decode @ 0x9DA14 (selector 0x46, dispatched at 0x9E811).
// Spec: docs/ghidra/FU25_tree_opcodes.md (byte-key flat tree: literal, root
// terminator, two-stage child expansion; magic 0x47FB bumped-header form).
// src/src_len is the raw record starting at the [selector, 0xFB] header. The port
// is safe where the original is not: every source read and the entry table are
// bounds-checked, expansion recursion is depth-limited, output writes are
// clamped at the declared length, and dst_cap must cover the declared length.
// Returns 0 (FIFA96_OK) on success, or the negated fifa96_err_t code on error
// (FIFA96_ERR_TRUNCATED for short/malformed input, a missing root terminator,
// a cyclic expansion, an output short of the declared length, or
// dst_cap < declared length); *out_len is set to the declared length on
// success, 0 on error.
int fifa96_tree_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_cap, size_t *out_len);
