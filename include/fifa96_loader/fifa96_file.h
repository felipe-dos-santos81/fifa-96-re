#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: wrappers around INT 21h AH=3Dh/3Fh/3Eh (exact FUN_11bd_* cited in Task 2).
fifa96_err_t fifa96_file_read(const char *path, uint8_t **out, size_t *out_len);
fifa96_err_t fifa96_file_read_chunk(const char *path, uint64_t off, uint8_t *dst, size_t len);
size_t fifa96_file_split_for_segment(uint16_t seg_off, size_t len);
void fifa96_file_free(uint8_t *p);
uint16_t fifa96_read_u16le(const uint8_t *p);
uint32_t fifa96_read_u32le(const uint8_t *p);
