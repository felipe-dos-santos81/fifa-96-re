#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
typedef struct { uint16_t magic; uint16_t word_a; uint16_t word_b; char tag[4]; size_t tail_off; size_t tail_len; } fifa96_envelope_hdr_t;
// Ghidra: file I/O via file_open_dos@11bd:5fb8 / file_read_dos@11bd:5fe2 (INT-21h wrappers, verified). The 10-byte envelope (magic 0xFB10 + word_a@2 + word_b@4 + 4cc tag@6, tail@10) is behaviorally reconstructed from the golden CD data — the decode funnel it feeds is mapped in Task 2 (see docs/ghidra/loader_rename_map.md, FU-1/FU-3). See docs/ghidra/FU1_FU2_closeout.md.
fifa96_err_t fifa96_envelope_parse_hdr(const uint8_t *b, size_t n, fifa96_envelope_hdr_t *h);
