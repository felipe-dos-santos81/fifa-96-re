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
//   +0x0B s8 voice index; the video parser requires 0..15. Bank entries
//         store -1 (unsigned: the sign selects the arm's flag set, FU-41
//         §2/§3.3); the runtime voice is supplied separately. -1 is accepted
//         as the bank-form marker, other out-of-range values are rejected.
//   +0x0C u32 declared stream length in samples; the original overwrites it
//         with this chunk's block count before use, so the on-disk value is
//         exposed here but never trusted for sizing (FU-35 §2). For f10==2
//         the authoritative per-chunk length is the 20-byte block header's
//         count (delta_units, FU-39 §4.3): declared = d0 + Σ per-chunk
//         counts over the stream.
//   +0x10 s32 loop start in blocks, +0x14 u32 loop length in blocks; the
//         original force-writes -1 / 0 in memory before use
//   +0x18 u32 data pointer/offset; video chunks store 0 (the parser writes
//         payload+0x20) while bank .spc headers store 0x20, the offset the
//         game relocates to header+0x20 before arming (FU-41 §2). The port
//         treats 0 as the implicit header-end offset 0x20 and honors a
//         nonzero offset after bounds-checking it.
//   +0x1D u8 per-voice volume (0x7F in the corpus); +0x1E/+0x1F are unread.
//
// Derived exactly as the parser: block_size = f8*f9; data_off as above;
// data_len = src_len - data_off; blocks = data_len / block_size (floor, the
// original's DIV); samples = blocks * (f10 == 2 ? 4 : 1). For f10==2 those
// remain the FU-35 nominal model; the block header's delta_units is
// authoritative. No audio mixing here — the data region is raw PCM for the
// f10!=2 layouts. Proven layouts (FU-35 §3.2, FU-39): signed 16-bit LE stereo
// interleaved per 4-byte block (f8=2,f9=2), byte-interleaved stereo (f8=1,f9=2;
// volume-table polarity is the open leg FU-35 §6.2), signed 16-bit mono in
// 2-byte bank blocks (f8=2,f9=1), and the f10==2 adaptive-delta nibble blocks
// decoded below.
typedef enum {
  FIFA96_EACS_FMT_UNKNOWN = 0,
  FIFA96_EACS_FMT_PCM16_STEREO = 1,  /* f8=2, f9=2: two signed 16-bit LE samples per block */
  FIFA96_EACS_FMT_PCM8_STEREO = 2,   /* f8=1, f9=2: one byte per channel per block */
  FIFA96_EACS_FMT_PCM16_MONO = 3,    /* f8=2, f9=1: one signed 16-bit LE sample per block */
  FIFA96_EACS_FMT_DELTA_STEREO = 4,  /* f8=2,f9=2,f10=2: byte = (L,R) delta frame */
  FIFA96_EACS_FMT_DELTA_MONO = 5     /* f8=2,f9=1,f10=2: nibble = one mono delta sample */
} fifa96_eacs_format_t;

struct fifa96_eacs_info {
  uint32_t rate;                    /* +0x04 */
  uint8_t f8, f9, f10;              /* +0x08, +0x09, +0x0A */
  int8_t voice;                     /* +0x0B: 0..15 video, -1 bank (FU-41) */
  uint32_t count;                   /* +0x0C declared sample count, as stored */
  int32_t loop_start;               /* +0x10, as stored */
  uint32_t loop_len;                /* +0x14, as stored */
  uint32_t data_ptr;                /* +0x18 raw field; 0 = implicit 0x20 */
  uint8_t volume;                   /* +0x1D */
  uint32_t data_off;                /* resolved data offset, >= 0x20 */
  uint32_t data_len;                /* src_len - data_off */
  uint32_t block_size;              /* f8*f9 */
  uint32_t blocks;                  /* data_len / block_size (FU-35 nominal) */
  uint64_t samples;                 /* blocks * (f10 == 2 ? 4 : 1), nominal */
  uint32_t delta_units;             /* f10=2: authoritative per-chunk unit count
                                       (stereo: block-header count at data_off,
                                       FU-39 §2.1; mono bank: declared +0x0C,
                                       FU-39 §2.2); else 0 */
  fifa96_eacs_format_t format;      /* proven layouts, else UNKNOWN */
};

// Returns 0 (FIFA96_OK) on success or the negated fifa96_err_t code:
// FIFA96_ERR_TRUNCATED for NULL args, src_len < 0x20, or a malformed header
// (voice outside -1..15, block_size 0, data offset inside the header or past
// src_len, an f10=2 stereo data region too short for its 20-byte block header
// or declaring more units than the packed bytes can hold, or an f10=2 mono
// declared count exceeding two nibbles per data byte); FIFA96_ERR_BAD_MAGIC
// for a payload that does not start with "EACS"; FIFA96_ERR_UNSUPPORTED for
// src_len > 2^32-1 (outside the original's 32-bit chunk model). *info is
// written only on success.
int fifa96_eacs_parse(const uint8_t *src, size_t src_len, struct fifa96_eacs_info *info);

// FU-39 §3 adaptive-delta tables, extracted from the FIFA96.EXE flat LE link
// image (FU-4) at image 0x141668 (DELTA) and 0x142ca8 (ADAPT); provenance and
// checksums in fifa96_eacs_tables.c. DELTA is 89 rows x 16 int32 = 0x1640
// bytes exactly up to ADAPT; row r holds the deltas for nibble values 0..15,
// with nibble bit 3 the sign (DELTA[r][n+8] == -DELTA[r][n]).
#define FIFA96_EACS_DELTA_ROWS 89
#define FIFA96_EACS_DELTA_COLS 16
#define FIFA96_EACS_ADAPT_COUNT 8
const int32_t *fifa96_eacs_delta_table(void); /* 89*16 entries */
const int32_t *fifa96_eacs_adapt_table(void); /* 8 entries */

// FU-39 §2.1/§3 adaptive-delta decoder state: rows are DELTA byte offsets
// (row index << 6, clamped to [0,0x1600]); accumulators are the clamped s16
// lane values. For f9==1 the single mono lane is carried in l_row/l_acc.
struct fifa96_eacs_delta {
  uint32_t l_row, r_row;
  int32_t l_acc, r_acc;
};

// Parse the 20-byte f10==2 block header (FU-39 §2.1, data at +0x14):
//   u32[0] = decoder unit count; u32[4]/u32[8] = L/R row codes (index << 6);
//   u32[0xc]/u32[0x10] = s16 L/R accumulators.
// Row codes are clamped to 0x1600 so the static port cannot index DELTA out
// of bounds (the original clamps only after ADAPT). Returns 0 or the negated
// fifa96_err_t: FIFA96_ERR_TRUNCATED for NULL args or src_len < 0x14.
int fifa96_eacs_delta_header(const uint8_t *src, size_t src_len,
                             struct fifa96_eacs_delta *st, uint32_t *count);

// Decode one adaptive-delta unit `unit` (0-based) from a packed data region
// (`data` = block header + 0x14), updating `st` (FU-39 §2.3/§3):
//   f9==2: data[unit] -> one (L,R) frame, high nibble L, low nibble R;
//   f9==1: nibble unit of data, MSB-first (even unit = high nibble), one mono
//          sample written to both lanes.
// out_l/out_r may be NULL to advance the state only. Returns 0, or the
// negated fifa96_err_t: TRUNCATED for NULL st/data; UNSUPPORTED for f9 != 1/2.
int fifa96_eacs_delta_unit(struct fifa96_eacs_delta *st, uint8_t f9,
                           const uint8_t *data, uint32_t unit,
                           int16_t *out_l, int16_t *out_r);

// Decode one whole f10==2 block: initialize `st` from the header at src (the
// producer re-initializes per dequeue, FU-39 §3), validate the count against
// the packed bytes in src (f9==2 needs count bytes; f9==1 ceil(count/2);
// trailing slack is allowed, FU-39 §6), then decode every unit. `out` may be
// NULL to advance state only; otherwise it holds count interleaved L/R frames
// and out_frames must be >= count. `units` (optional) receives the count.
// Returns 0 or the negated fifa96_err_t (TRUNCATED/UNSUPPORTED).
int fifa96_eacs_delta_block(const uint8_t *src, size_t src_len, uint8_t f9,
                            struct fifa96_eacs_delta *st,
                            int16_t *out, size_t out_frames, uint32_t *units);
