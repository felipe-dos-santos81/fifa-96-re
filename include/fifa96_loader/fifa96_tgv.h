#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
typedef struct { char magic[4]; uint32_t v0; } fifa96_tgv_hdr_t;
// Ghidra: TGV framing reader, header only — PENDING — decoder-identification pass outstanding (spec §8: FUN_11bd_XXXX @ 11bd:XXXX + golden + decompile-hash). See follow-up FU-1 (repo SDD ledger, fifa96-file-loader).
fifa96_err_t fifa96_tgv_parse_hdr(const uint8_t *b, size_t n, fifa96_tgv_hdr_t *h);
