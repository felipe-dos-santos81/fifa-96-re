#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
typedef struct { char magic[4]; uint32_t v0; } fifa96_tgv_hdr_t;
// Ghidra: file I/O via file_open_dos@11bd:5fb8 / file_read_dos@11bd:5fe2 (INT-21h wrappers, verified). Codec funnel (load_mf_object@11bd:5dd2, mem_grow_relocate@1000:0b12, exec_loaded_image@11bd:6907, alloc_retry_loop@11bd:5d79, copy_bytes_far@11bd:6102, int21_dispatch@11bd:26d0; see docs/ghidra/loader_rename_map.md#codec-funnel) shows no TGV/kVGT tag reference — funnel not yet attributable to this format. TGV framing (kVGT tag @ off 0 + u32 w/h) is NOT statically locatable in the EXE (tag absent from the image; FU-1 passes) — header shape is behaviorally reconstructed from the golden CD data. See docs/ghidra/FU1_FU2_closeout.md.
fifa96_err_t fifa96_tgv_parse_hdr(const uint8_t *b, size_t n, fifa96_tgv_hdr_t *h);
