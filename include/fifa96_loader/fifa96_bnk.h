#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: registrar FUN_000a75aa @ 0xA75AA, unregister FUN_000a765f @ 0xA765F,
// play lookup FUN_000a7728 @ 0xA7728, arm FUN_000a780e @ 0xA780E. Spec:
// docs/ghidra/FU43_bnk_format.md.
//
// SOUND/*.BNK is [128 x u32 LE offset table][0x48-byte descriptors][payload
// pool] (FU-43 §1). The table is indexed by global SFX id; slot 0 is a normal
// slot (all retail banks leave it zero) and a zero slot means the id is
// absent. Each nonzero slot is the file offset of a 0x48-byte descriptor whose
// +0x04 is the offset of the embedded 32-byte EACS header (entry+0x28 in
// 59/59 retail entries) and whose EACS +0x18 is the absolute file offset of the
// payload; the registrar relocates both to pointers once at registration
// (0xA7628..0xA763D). Payloads are 4-byte aligned with 0..3 alignment gap
// bytes before the next payload (FU-43 §1).
//
// This is a pure container layer: it validates the table/descriptor/payload
// bounds and exposes file offsets. It does not parse EACS (fifa96_eacs_*) and
// does not mix. Because the bank stores EACS +0x18 as an absolute file offset
// (not the parser's implicit 0x20), a caller feeding fifa96_eacs_parse must
// copy the 32-byte header, rewrite +0x18 to 0x20 and append the payload
// extent — the same relocation the game performs at registration (FU-43 §4.1).
//
// Open legs (FU-43 §6, not ported): the static SFX_GAME.BNK load site, the
// unread descriptor bytes +0x15/+0x1B/+0x1E..+0x27, the event->id table, the
// two-voice descriptor path (bit 0 of +0x1C) and the CHN loop units.
#define FIFA96_BNK_IDS 128u
#define FIFA96_BNK_DESC_SIZE 0x48u
#define FIFA96_BNK_EACS_SIZE 0x20u

struct fifa96_bnk_info {
  const uint8_t *src;                    /* borrowed; must outlive entry calls */
  size_t src_len;
  uint32_t table[FIFA96_BNK_IDS];        /* on-disk id -> descriptor offset */
  uint32_t payload_off[FIFA96_BNK_IDS];  /* resolved EACS+0x18 per id (0 absent) */
  uint32_t payload_len[FIFA96_BNK_IDS];  /* extent to the next payload/file end */
  uint32_t entry_count;                  /* nonzero table slots */
};

struct fifa96_bnk_entry {
  uint32_t id;           /* global SFX id = table slot */
  uint32_t desc_off;     /* 0x48-byte descriptor file offset */
  uint32_t eacs_off;     /* 0x20-byte embedded EACS header (descriptor+0x28) */
  uint32_t payload_off;  /* EACS +0x18 absolute payload file offset */
  uint32_t payload_len;  /* extent to the next payload/file end; includes the
                            0..3 alignment gap bytes (FU-43 §1) */
};

// Validate the 128-slot table and every present descriptor/EACS/payload and
// fill *info. Returns FIFA96_OK, FIFA96_ERR_TRUNCATED (NULL args, src_len <
// 0x200, or a nonzero slot whose descriptor is misaligned/before 0x200/crosses
// the file, whose embedded EACS pointer is misaligned/before 0x200/crossing
// the file, or whose payload offset is misaligned/before 0x200/past src_len),
// or FIFA96_ERR_UNSUPPORTED for src_len > 2^32-1 (outside the original's
// 32-bit file model). *info is written only on success.
fifa96_err_t fifa96_bnk_parse(const uint8_t *src, size_t src_len,
                              struct fifa96_bnk_info *info);

// Resolve global SFX id `id` (0..127, table slot). Returns FIFA96_OK and fills
// *out; FIFA96_ERR_NOT_FOUND for a zero table slot (id absent), or
// FIFA96_ERR_TRUNCATED for NULL info/out, id >= 128, or an info whose stored
// offsets do not describe a bounded entry.
fifa96_err_t fifa96_bnk_entry(const struct fifa96_bnk_info *info, uint32_t id,
                              struct fifa96_bnk_entry *out);
