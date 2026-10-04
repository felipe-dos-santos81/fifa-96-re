#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: BIGF v2 "lumpy" container sniff FUN_000a2410 @ 0xA2410, directory
// walk FUN_000a275c @ 0xA275C (by index) and FUN_000a246c @ 0xA246C (by
// name), thin pointer wrapper FUN_000a263c @ 0xA263C. Spec:
// docs/ghidra/FU41_bank_loader.md §1.
//
// Retail SOUND/*.VIV are BIGF v2 (the `.BNK` sibling's stride-4 u32 offset
// table is fifa96_viv_entry_at's, a different format):
//   +0x00 char[4] "BIGF"
//   +0x04 BE32 file size (validated == src_len)
//   +0x08 BE32 entry count
//   +0x0C BE32 directory end / first data offset
//   +0x10 record[count]: [BE32 data offset][BE32 size][name NUL-terminated];
//         the walker's stride is strlen(name) + 9 (8 + the name byte count),
//         and 0-3 arbitrary pad bytes may follow the last name before +0xC.
// The original walker validates neither the header nor the records; this port
// bounds-checks the whole directory and every record's data range up front.
//
// This is a pure container layer: a record's `size` bytes at `off` are the
// entry, whose first 32 bytes are the FU-41 §2 EACS header (bank form, voice
// -1, +0x18 == 0x20) followed by the raw payload. Audio decoding stays in
// fifa96_eacs_*; no mixing happens here.
//
// Open legs (FU-41 §5, not ported): the game's by-name lookup wrapper
// FUN_000a263c, the direct-play wrappers 0xBA3B6/0xBA3DE/0xBA403 (no static
// callers in the flat image) and the playlist -> sample mapping
// (FUN_000675a0 and the stream ring) are out of scope for this slice.
struct fifa96_bigf_info {
  const uint8_t *src;  /* borrowed; must outlive fifa96_bigf_record calls */
  size_t src_len;
  uint32_t size;       /* +0x04, validated == src_len */
  uint32_t count;      /* +0x08 */
  uint32_t table_end;  /* +0x0C */
};

// Returns FIFA96_OK, FIFA96_ERR_BAD_MAGIC (src does not start with "BIGF"),
// FIFA96_ERR_TRUNCATED (NULL args, src_len < 0x10, declared size != src_len,
// table end outside [0x10, size], count too large for the directory, a record
// header or name crossing table_end, or a record's offset+size past
// src_len), or FIFA96_ERR_UNSUPPORTED for src_len > 2^32-1 (outside the
// original's 32-bit file model). *info is written only on success.
fifa96_err_t fifa96_bigf_parse(const uint8_t *src, size_t src_len,
                               struct fifa96_bigf_info *info);

// Walk to record `index` (0-based, directory order; names are variable-length
// so this is O(index), exactly like FUN_000a275c). Any of off/size/name may
// be NULL; `name` receives a borrowed pointer into info->src, NUL-terminated
// at or before table_end (parse rejects an unterminated record name, so the
// returned string is always safe to read). Returns FIFA96_OK or
// FIFA96_ERR_TRUNCATED (NULL info/src or index >= count).
fifa96_err_t fifa96_bigf_record(const struct fifa96_bigf_info *info, size_t index,
                                uint32_t *off, uint32_t *size, const char **name);
