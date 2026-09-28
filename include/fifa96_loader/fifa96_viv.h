#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: VIV/BNK offset-table reader — PENDING — decoder-identification pass outstanding (spec §8: FUN_11bd_XXXX @ 11bd:XXXX + golden + decompile-hash). See follow-up FU-1 (repo SDD ledger, fifa96-file-loader).
fifa96_err_t fifa96_viv_entry_at(const uint8_t *b, size_t n, size_t idx, uint32_t *off);
