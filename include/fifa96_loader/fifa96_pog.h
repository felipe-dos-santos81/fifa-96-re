#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
typedef struct { uint8_t magic[2]; uint16_t w1; uint32_t w2; } fifa96_pog_hdr_t;
// Ghidra: POG header reader (FUN_11bd_* per rename map).
fifa96_err_t fifa96_pog_parse_hdr(const uint8_t *b, size_t n, fifa96_pog_hdr_t *h);
