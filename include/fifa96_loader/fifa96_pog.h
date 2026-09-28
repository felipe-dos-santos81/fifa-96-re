#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
typedef struct { uint8_t magic[2]; uint16_t w1; uint32_t w2; } fifa96_pog_hdr_t;
// Ghidra: file I/O via file_open_dos@11bd:5fb8 / file_read_dos@11bd:5fe2 (INT-21h wrappers, verified). POG format parser (PCNX tag @ off 4) is NOT statically locatable in the EXE (tag absent from the image; FU-1 passes) — header shape is behaviorally reconstructed from the golden CD data. See docs/ghidra/FU1_FU2_closeout.md.
// Envelope: the 10-byte uniform prefix (magic/word_a/word_b/tag@6) is parsed by fifa96_envelope_parse_hdr; w2 (u32 @4) here overlaps tag@6 and is kept for golden compatibility.
fifa96_err_t fifa96_pog_parse_hdr(const uint8_t *b, size_t n, fifa96_pog_hdr_t *h);
