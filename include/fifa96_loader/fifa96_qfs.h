#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
typedef struct { uint8_t magic[2]; uint32_t dec_len; char tag[4]; } fifa96_qfs_hdr_t;
// Ghidra: QFS/PVI framing reader — PENDING — decoder-identification pass outstanding (spec §8: FUN_11bd_XXXX @ 11bd:XXXX + golden + decompile-hash). See follow-up FU-1 (repo SDD ledger, fifa96-file-loader).
fifa96_err_t fifa96_qfs_parse_hdr(const uint8_t *b, size_t n, fifa96_qfs_hdr_t *h);
