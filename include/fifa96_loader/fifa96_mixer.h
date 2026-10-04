#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_eacs.h"
#include "fifa96_loader/fifa96_err.h"
// Ghidra: voice arm FUN_000b7fe8 @ 0xB7FE8, free FUN_000b80fa @ 0xB80FA,
// state FUN_000b817b @ 0xB817B, mixer tick FUN_000b7f62 @ 0xB7F62,
// per-format readers 0xB929D (16-bit stereo), 0xB8E8F (8-bit stereo),
// 0xB8AE1 (16-bit mono), output converter 0xB9E53, step conversion
// FUN_000b86C8 @ 0xB86C8. Spec: docs/ghidra/FU37_mixer_and_timing.md Part A.
//
// Deterministic port of the software mixer model. The original keeps 16
// channel structs (stride 0x6C) and mixes into a 512-frame 32-bit accumulator
// (0x36698) before the clamp/format converter; this port accumulates in
// 32-bit integers per frame and writes the game's retail output format
// directly: 22050 Hz, 16-bit signed stereo, interleaved (FU-37 §A.4).
// Out of scope (open legs, FU-37 §A.5/§6): the queue producer/streaming path,
// the f10==2 second-cursor nibble decoder, and the pitch-ratio table head.

#define FIFA96_MIXER_VOICES 16

struct fifa96_mixer_voice {
  int active;                   /* +0x00 state: 0 free, 1 active */
  fifa96_eacs_format_t format;  /* selected from f8/f9 (FU-35 §3.1) */
  const uint8_t *data;          /* data base (payload + info->data_off) */
  uint32_t units;               /* cursor units in data (= info->blocks) */
  uint8_t volume;               /* 0..0x7F; readers scale by vol/0x80 */
  uint64_t pos;                 /* 32.32 cursor, high 32 = unit index */
  uint64_t step;                /* 32.32 units per output frame */
  int loop;                     /* +0x01 loop flag (header loop_len != 0) */
  uint32_t loop_start;          /* +0x1C loop-start unit */
  uint32_t loop_end;            /* +0x20 loop-end unit, one past */
};

struct fifa96_mixer {
  struct fifa96_mixer_voice voices[FIFA96_MIXER_VOICES];
};

/* Zero the voice table. */
void fifa96_mixer_init(struct fifa96_mixer *m);

// Arm voice `voice` (0..15) from a parsed EACS header over `payload`
// (`payload_len` bytes, the [tag][len]-stripped 1SNh/1SNd payload). The data
// region is payload + info->data_off and holds info->blocks cursor units:
// one 4-byte frame for PCM16_STEREO, one 2-byte frame for PCM8_STEREO and one
// 2-byte sample for PCM16_MONO. `volume` is the EACS 0..0x7F scale (0x7F in
// the corpus); the packed L/R record conversion FUN_000a662c is not ported.
// `step` is the 32.32 resample step (see fifa96_mixer_step_from_rate).
// Returns FIFA96_OK or the negated fifa96_err_t:
//   TRUNCATED   NULL args, voice out of range, volume > 0x7F, or a data
//               region inconsistent with payload_len
//   UNSUPPORTED EACS format UNKNOWN, or f10 == 2 (open leg: the second-cursor
//               nibble path and 4x sample accounting are FU-37 §A.5/§6.1)
// A nonzero loop_len arms the loop when the window lies inside the buffer;
// retail videos force-write -1/0 and never loop (FU-37 §A.5).
int fifa96_mixer_start(struct fifa96_mixer *m, int voice,
                       const struct fifa96_eacs_info *info,
                       const uint8_t *payload, size_t payload_len,
                       uint8_t volume, uint64_t step);

/* Free a voice (FUN_000b80fa); inactive voices render silence. */
void fifa96_mixer_stop(struct fifa96_mixer *m, int voice);

/* 1 if voice is armed, else 0 (FUN_000b817b); out-of-range -> 0. */
int fifa96_mixer_voice_active(const struct fifa96_mixer *m, int voice);

// Mix every active voice into `frames` interleaved 16-bit stereo frames
// (out[2*i] = L, out[2*i+1] = R), like FUN_000b7f62 + converter 0xB9E53:
// 32-bit accumulate per reader sample, then clamp to +/-32767 (the converter
// clamps both bounds to 32767, FU-37 §A.4). No interpolation: the integer
// cursor part selects the nearest-lower unit (FU-37 §A.5). A looping voice
// wraps to loop_start with zero fraction when the next fetch would leave
// [loop_start, loop_end); a non-looping voice that reaches the end of its
// data is stopped and renders silence for the rest of the call.
void fifa96_mixer_render(struct fifa96_mixer *m, int16_t *out, size_t frames);

// FU-37 §A.5 tail (FUN_000b86C8 after the 0xB6B88 table lookup):
//   v = (rate * ratio) >> shift; step_int = v / out_rate;
//   step = ((step_int & 0xFF) << 32) | ((v - step_int*out_rate) * 2^32 / out_rate)
// `ratio`/`shift` are a pitch-table entry and its coarse exponent; ratio
// 0x10000 with shift 16 is the table's unity entry (table[0], FU-37 §A.5).
// The pitch+0x2000 -> window-index/CL head and the table itself are FU-37
// open leg 2 (neutral-pitch calibration), so callers pass ratio/shift or the
// step directly. Returns FIFA96_OK, or -(FIFA96_ERR_TRUNCATED) for NULL
// step, out_rate 0 or shift > 63.
int fifa96_mixer_step_from_rate(uint32_t rate, uint32_t out_rate,
                                uint32_t ratio, uint32_t shift,
                                uint64_t *step);
