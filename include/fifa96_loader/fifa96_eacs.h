#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
// Ghidra: EACS header parse FUN_000a79dc @ 0xA79DC (the '1SNh' arm of the TGV
// companion-chunk router FUN_000a7d3c, FU-35 §1/§2); format flags
// FUN_000a6579 @ 0xA6579. Spec: docs/ghidra/FU35_eacs_audio.md.
//
// `src` is the chunk payload after the 8-byte [tag][len] chunk header: the
// 32-byte EACS header plus the chunk's raw PCM bytes. All fields are
// little-endian at the FU-35 §2 offsets:
//   +0x00 "EACS"
//   +0x04 u32 sample rate in Hz (16000 on 72/73 videos, 16384 once)
//   +0x08 u8 f8, +0x09 u8 f9: block size = f8*f9 bytes
//   +0x0A u8 f10: 2 => 4 samples per block (open leg, FU-35 §3.1/§6.1),
//         otherwise 1
//   +0x0B s8 voice index; the video parser requires 0..15
//   +0x0C u32 declared stream length in samples; the original overwrites it
//         with this chunk's block count before use, so the on-disk value is
//         exposed here but never trusted for sizing (FU-35 §2)
//   +0x10 s32 loop start in blocks, +0x14 u32 loop length in blocks; the
//         original force-writes -1 / 0 in memory before use
//   +0x18 u32 data pointer/offset; video chunks store 0 (the parser writes
//         payload+0x20) while bank .spc headers store 0x20 (FU-35 §2/§6.5).
//         The port treats 0 as the implicit header-end offset 0x20 and
//         honors a nonzero offset after bounds-checking it.
//   +0x1D u8 per-voice volume (0x7F in the corpus); +0x1E/+0x1F are unread.
//
// Derived exactly as the parser: block_size = f8*f9; data_off as above;
// data_len = src_len - data_off; blocks = data_len / block_size (floor, the
// original's DIV); samples = blocks * (f10 == 2 ? 4 : 1). No audio mixing
// here — the data region is raw PCM. Proven layouts (FU-35 §3.2): signed
// 16-bit LE stereo interleaved per 4-byte block (f8=2,f9=2), byte-interleaved
// stereo (f8=1,f9=2; volume-table polarity is the open leg FU-35 §6.2), and
// signed 16-bit mono in 2-byte bank blocks (f8=2,f9=1; the f10=2 sample
// accounting for banks is the open leg FU-35 §6.1).
typedef enum {
  FIFA96_EACS_FMT_UNKNOWN = 0,
  FIFA96_EACS_FMT_PCM16_STEREO = 1,  /* f8=2, f9=2: two signed 16-bit LE samples per block */
  FIFA96_EACS_FMT_PCM8_STEREO = 2,   /* f8=1, f9=2: one byte per channel per block */
  FIFA96_EACS_FMT_PCM16_MONO = 3     /* f8=2, f9=1: one signed 16-bit LE sample per block */
} fifa96_eacs_format_t;

struct fifa96_eacs_info {
  uint32_t rate;                    /* +0x04 */
  uint8_t f8, f9, f10;              /* +0x08, +0x09, +0x0A */
  int8_t voice;                     /* +0x0B, 0..15 */
  uint32_t count;                   /* +0x0C declared sample count, as stored */
  int32_t loop_start;               /* +0x10, as stored */
  uint32_t loop_len;                /* +0x14, as stored */
  uint32_t data_ptr;                /* +0x18 raw field; 0 = implicit 0x20 */
  uint8_t volume;                   /* +0x1D */
  uint32_t data_off;                /* resolved data offset, >= 0x20 */
  uint32_t data_len;                /* src_len - data_off */
  uint32_t block_size;              /* f8*f9 */
  uint32_t blocks;                  /* data_len / block_size */
  uint64_t samples;                 /* blocks * (f10 == 2 ? 4 : 1) */
  fifa96_eacs_format_t format;      /* proven layouts, else UNKNOWN */
};

// Returns 0 (FIFA96_OK) on success or the negated fifa96_err_t code:
// FIFA96_ERR_TRUNCATED for NULL args, src_len < 0x20, or a malformed header
// (voice outside 0..15, block_size 0, data offset inside the header or past
// src_len); FIFA96_ERR_BAD_MAGIC for a payload that does not start with
// "EACS"; FIFA96_ERR_UNSUPPORTED for src_len > 2^32-1 (outside the original's
// 32-bit chunk model). *info is written only on success.
int fifa96_eacs_parse(const uint8_t *src, size_t src_len, struct fifa96_eacs_info *info);
