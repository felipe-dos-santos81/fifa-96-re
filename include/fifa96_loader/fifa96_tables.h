#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: table index helpers — PENDING — decoder-identification pass outstanding (spec §8: FUN_11bd_XXXX @ 11bd:XXXX + golden + decompile-hash). See follow-up FU-1 (repo SDD ledger, fifa96-file-loader).
fifa96_err_t fifa96_tables_entry_count(const uint8_t *fnames, size_t n, size_t *count);
fifa96_err_t fifa96_tables_name_at(const uint8_t *fnames, size_t n, size_t idx, char out[16]);
fifa96_err_t fifa96_tables_length_at(const uint8_t *lengths, size_t n, size_t idx, uint32_t *val);
