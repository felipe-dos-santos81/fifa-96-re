#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#define FIFA96_SCRIPT_TERM 0xffffu
typedef struct { uint16_t *words; size_t len; size_t cap; } fifa96_script_table_t;
// Ghidra: else-branch parse_script_text@11bd:5bdb + build_word_table@11bd:5c8b (verified; see docs/ghidra/loader_rename_map.md#script-semantics). Mechanics mirrored here: append (store at 11bd:5d44), full-cap TRUNCATED (limit idiom CMP AX,SI at 11bd:5d4d; DOS grow via mem_grow_relocate at 11bd:5d55 is allocator business — fixed-cap by design, callers grow), 0xffff terminator (store at 11bd:5d18), ;-comment skip (SUB AX,0x3c at 11bd:5be9; present only if Task-1 CONFIRMED-skip). Keyword branch semantics (C/E/M/E/R) stay out of C until proven — see map.
fifa96_err_t fifa96_script_table_append(fifa96_script_table_t *t, uint16_t v);
fifa96_err_t fifa96_script_table_terminated(const uint16_t *w, size_t n, size_t *term_at);
size_t fifa96_script_comment_len(const uint8_t *b, size_t n);
