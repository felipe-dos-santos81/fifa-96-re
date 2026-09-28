#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: VIV/BNK offset-table reader (FUN_11bd_* per rename map).
fifa96_err_t fifa96_viv_entry_at(const uint8_t *b, size_t n, size_t idx, uint32_t *off);
