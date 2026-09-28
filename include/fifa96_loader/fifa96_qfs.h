#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
typedef struct { uint8_t magic[2]; uint32_t dec_len; char tag[4]; } fifa96_qfs_hdr_t;
// Ghidra: file I/O via file_open_dos@11bd:5fb8 / file_read_dos@11bd:5fe2 (INT-21h wrappers, verified). Codec funnel (load_mf_object@11bd:5dd2, mem_grow_relocate@1000:0b12, exec_loaded_image@11bd:6907, alloc_retry_loop@11bd:5d79, copy_bytes_far@11bd:6102, int21_dispatch@11bd:26d0; see docs/ghidra/loader_rename_map.md#codec-funnel) shows no QFS/PVI tag or envelope reference — funnel not yet attributable to this format. QFS/PVI framing (SHPI tag + dec_len) is NOT statically locatable in the EXE (tag absent from the image; FU-1 passes) — framing is behaviorally reconstructed from the golden CD data. See docs/ghidra/FU1_FU2_closeout.md.
// Envelope: the 10-byte uniform prefix (magic/word_a/word_b/tag@6) is parsed by fifa96_envelope_parse_hdr; dec_len (u32 @2) here straddles word_a/word_b and is kept for golden compatibility.
fifa96_err_t fifa96_qfs_parse_hdr(const uint8_t *b, size_t n, fifa96_qfs_hdr_t *h);
