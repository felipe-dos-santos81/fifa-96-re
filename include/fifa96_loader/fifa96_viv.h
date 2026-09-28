#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: file I/O via file_open_dos@11bd:5fb8 / file_read_dos@11bd:5fe2 / file_read_far_dos@11bd:6003 (INT-21h wrappers, verified; far reader is the segment-split path). VIV/BNK stride-4 u32 offset table is NOT statically locatable in the EXE (FU-1 passes) — table shape is behaviorally reconstructed from the golden CD data. See docs/ghidra/FU1_FU2_closeout.md.
fifa96_err_t fifa96_viv_entry_at(const uint8_t *b, size_t n, size_t idx, uint32_t *off);
