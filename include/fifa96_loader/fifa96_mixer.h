#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_eacs.h"
#include "fifa96_loader/fifa96_err.h"
// Ghidra: voice arm FUN_000b7fe8 @ 0xB7FE8, free FUN_000b80fa @ 0xB80FA,
// state FUN_000b817b @ 0xB817B, mixer tick FUN_000b7f62 @ 0xB7F62,
// per-format readers 0xB929D (16-bit stereo), 0xB8E8F (8-bit stereo),
// 0xB8AE1 (16-bit mono), output converter 0xB9E53, step conversion
// FUN_000b86C8 @ 0xB86C8 + pitch table 0xB6B88, pan/gain FUN_000a662c @
// 0xA662C. Spec: docs/ghidra/FU37_mixer_and_timing.md Part A and
// docs/ghidra/FU46_mixer_calibration.md (the FU-46 slice corrects the
// FU-37 §A.5 tail and closes its pitch-table head and packed-pan legs);
// docs/ghidra/FU48_audio_edge_cases.md closes the out-of-table pitch, the
// step_int>=256 #DE and the >0x7F gain-byte legs.
//
// Deterministic port of the software mixer model. The original keeps 16
// channel structs (stride 0x6C) and mixes into a 512-frame 32-bit accumulator
// (0x36698) before the clamp/format converter; this port accumulates in
// 32-bit integers per frame and writes the game's retail output format
// directly: 22050 Hz, 16-bit signed stereo, interleaved (FU-37 §A.4).
// f10==2 voices decode the FU-39 adaptive-delta nibble path lazily: the
// voice's row/accumulator state lives here and the packed block is walked as
// the resampler cursor advances (no staging copy; FU-39 §7 leg 1 covers the
// runtime staging buffer). Out of scope (open legs): the queue
// producer/streaming path, the signed arm's one-unit truncation (§4.3), and
// the unsigned bank loader data mapping (§2.2/§7 leg 3).

#define FIFA96_MIXER_VOICES 16

/* FUN_000b86C8's 1200-entry 1/1200-octave ratio table at image 0xB6B88. */
#define FIFA96_MIXER_PITCH_ENTRIES 1200

extern const uint32_t fifa96_mixer_pitch_table[FIFA96_MIXER_PITCH_ENTRIES];

struct fifa96_mixer_voice {
  int active;                   /* +0x00 state: 0 free, 1 active */
  uint8_t priority;             /* EACSNDF +0x13 steal priority (FU-47 §1) */
  fifa96_eacs_format_t format;  /* selected from f8/f9 (FU-35 §3.1) */
  const uint8_t *data;          /* data base (payload + info->data_off) */
  uint32_t units;               /* cursor units in data (= info->blocks) */
  uint8_t volume;               /* record+0x26 gain byte (retail 0..0x7F) */
  /* FU-48: the reader gains ch+0x64/+0x68 >> 10. A retail gain byte keeps
   * both at 0..0x7F; a >0x7F (negative s8) gain byte makes the unsigned DIV
   * emit wide quotients, so the right lane is kept to 16 bits (0..0xFFFF)
   * exactly as FUN_000a662c's packed extraction (0xA6601 SHR 0x10). */
  uint32_t gain_l, gain_r;
  uint64_t pos;                 /* 32.32 cursor, high 32 = unit index */
  uint64_t step;                /* 32.32 units per output frame */
  int loop;                     /* +0x01 loop flag (header loop_len != 0) */
  uint32_t loop_start;          /* +0x1C loop-start unit */
  uint32_t loop_end;            /* +0x20 loop-end unit, one past */
  /* FU-39 §2/§3 f10==2 adaptive-delta state (the engine's ch+0x30/+0x34/+0x38
   * second cursor). `delta` selects the lazy nibble path; the block header
   * state is re-initialized per block (header pointer kept for loop wraps). */
  int delta;                    /* 1 for a DELTA_* voice */
  uint8_t f9;                   /* delta layout: 2 stereo, 1 mono */
  const uint8_t *delta_hdr;     /* 20-byte block header, or NULL (bank forms) */
  uint32_t delta_units;         /* units in this block (header/declared count) */
  uint32_t delta_pos;           /* units decoded so far (state chain length) */
  struct fifa96_eacs_delta delta_state; /* running row/acc state */
  int16_t delta_l, delta_r;     /* last decoded frame (mono duplicates L) */
};

struct fifa96_mixer {
  struct fifa96_mixer_voice voices[FIFA96_MIXER_VOICES];
  int next_voice;               /* FU-47: DAT_000148AC allocator rotor, 0..15 */
  int sound_state;              /* FU-47: [0x15FC8], 1..5 = arm allowed, 0 = off */
};

/* Zero the voice table. next_voice = 0 (the BSS rotor) and sound_state = 1
 * (gate open, the FU-47 port default that preserves the pre-gate behavior). */
void fifa96_mixer_init(struct fifa96_mixer *m);

// FUN_000a62fa @ 0xA62FA (FU-47 §1): pick a voice for a descriptor voice
// `mask` (descriptor +0x00, bit v allows voice v) and `priority` (descriptor
// +0x14). Phase 1 scans from next_voice in wrap order for the first allowed
// voice with active == 0; phase 2, only when every allowed voice is active,
// steals the first allowed voice whose stored `priority` is <= the new
// priority (unsigned byte compare, 0xA636B/0xA636D). On success advances
// next_voice past the choice (0xA6328..0xA6330) and returns the voice; returns
// -1 when no allowed voice wins (0xA637D, the original's NULL). The chosen
// voice is not activated; the arm marks it (+0x16 = 1) after the EACS and
// sound-state checks, and stores the priority with fifa96_mixer_set_priority.
int fifa96_mixer_alloc_voice(struct fifa96_mixer *m, uint32_t mask,
                             uint8_t priority);

// EACSNDF +0x13 (arm 0xA7887..0xA788A): store the descriptor +0x14 priority
// so later allocations can steal this voice. Returns FIFA96_OK or
// -(FIFA96_ERR_TRUNCATED) for NULL m or a voice outside 0..15.
int fifa96_mixer_set_priority(struct fifa96_mixer *m, int voice,
                              uint8_t priority);

// FU-47 §3: write the sound-system state [0x15FC8]. FUN_000a6265 validates
// 0..5 and stores the mode; FUN_000a6505 (stop) stores 0. The arm gate
// (0xA7852..0xA7869) allows only 1..5, so state 0 stops every new arm.
// Returns FIFA96_OK, or -(FIFA96_ERR_TRUNCATED) for NULL m or state outside
// 0..5 (the original's -4; nothing is stored on failure).
int fifa96_mixer_set_sound_state(struct fifa96_mixer *m, int state);

// Arm voice `voice` (0..15) from a parsed EACS header over `payload`
// (`payload_len` bytes, the [tag][len]-stripped 1SNh/1SNd payload). For the
// PCM formats the data region is payload + info->data_off and holds
// info->blocks cursor units: one 4-byte frame for PCM16_STEREO, one 2-byte
// frame for PCM8_STEREO and one 2-byte sample for PCM16_MONO. For f10==2 the
// voice decodes the FU-39 adaptive-delta path: DELTA_STEREO with voice >= 0
// parses the 20-byte block header at data_off (signed/video producer 0xB84FE)
// and holds info->delta_units frames; DELTA_STEREO with voice -1 is the
// FU-43 §2 bank form (no block header, zero state, info->delta_units packed
// bytes at data_off, unsigned producer 0xB8610); DELTA_MONO follows the
// unsigned bank arm (no block header, zero state, declared info->delta_units
// nibbles at data_off, FU-39 §2.2). `volume` is the raw record+0x26 byte
// FUN_000b9fdd stores with `MOV [EBX+0x26],AL` (0xB9FF2), 0..0xFF: retail
// operands yield 0..0x7F, but a signed-negative gain product yields a >0x7F
// byte with no clamp (FU-48 §4). It is the gain FUN_000a662c scales for both
// channels, and fifa96_mixer_set_pan may then split it into the per-channel
// gains +0x64/+0x68. `step` is the 32.32 resample step (see
// fifa96_mixer_step_from_pitch / fifa96_mixer_step_from_rate).
// Returns FIFA96_OK or the negated fifa96_err_t:
//   TRUNCATED   NULL args, voice out of range, a PCM data region inconsistent
//               with payload_len, or an f10==2 block header/count inconsistent
//               with payload_len
//   UNSUPPORTED EACS format UNKNOWN, or an f10==2 f8/f9 pair with no decoder
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

// FUN_000b86C8 head (0xB86C9..0xB86E6): pitch (signed; EACS record+0xC) ->
// table entry + coarse exponent. E = pitch+0x2000; W walks down from 0x40D0
// by 0x4B0 with shift starting at 9 while E < W; index = E-W into the
// 1200-entry table at 0xB6B88. Pitch 0 -> table[0] 0x10000, shift 16 (unity);
// +/-1200 steps the shift by one (one octave); the window base is +8400.
// Cites: FUN-46 §1. Returns FIFA96_OK, or -(FIFA96_ERR_TRUNCATED) for NULL
// out, an index outside the table (pitch > 9599) or shift > 31 (-18001 and
// below; the original's SHRD would wrap CL mod 32, which is not modelled).
int fifa96_mixer_pitch_ratio(int32_t pitch, uint32_t *ratio, uint32_t *shift);

// FUN_000b86C8 head+tail: the 32.32 resample step FUN_000b7fe8 stores in
// ch+0x24/+0x28, from the EACS sample `rate`, the output rate [0x406A0] and
// the signed pitch argument (FUN_000a6579 builds it from record+0xC/+0x10/
// +0x11). Retail videos and every retail bank descriptor are pitch 0, i.e.
// table[0]/shift 16. Returns FIFA96_OK or the pitch_ratio /
// step_from_rate errors.
int fifa96_mixer_step_from_pitch(uint32_t rate, uint32_t out_rate,
                                 int32_t pitch, uint64_t *step);

// FU-46 §1 tail (FUN_000b86C8 after the 0xB6B88 table lookup):
//   v = (rate * ratio) >> shift;              (MUL 0xB86ED, SHRD 0xB86EF)
//   step_int = (v / out_rate) & 0xFF;         (DIV 0xB86F9, AND 0xB86FF)
//   frac = (((v - step_int*out_rate) << 24) / out_rate) << 8;  (0xB870E..B872B)
//   step = (step_int << 32) | frac;           (+0x24/+0x28)
// The fraction is the 8.24 step (+0x2C) scaled into the 32.32 low word, so
// its low 8 bits are always zero. The original's second DIV (`MUL 0x1000000`
// 0xB871B, `DIV [0x406A0]` 0xB8722) faults (#DE) whenever the unmasked first
// quotient v/out_rate is >= 256: the `AND EAX,0xFF` at 0xB86FF makes rem =
// 256*floor(q/256)*out + r, so the DIV's high word is >= out_rate (FU-48 §3).
// The port rejects that input instead of fabricating the masked step; q < 256
// cannot fault and is computed with 64-bit intermediates (FU-46 §1.3).
// `ratio`/`shift` are a pitch-table entry and its coarse exponent; ratio
// 0x10000 with shift 16 is the table's unity entry. Returns FIFA96_OK, or
// -(FIFA96_ERR_TRUNCATED) for NULL step, out_rate 0, shift > 31 (the
// original's SHRD reads CL&31) or q >= 256 (the original's #DE).
int fifa96_mixer_step_from_rate(uint32_t rate, uint32_t out_rate,
                                uint32_t ratio, uint32_t shift,
                                uint64_t *step);

// FUN_000a662c @ 0xA662C: an EACSNDF record's pan byte (+0x27) and signed
// gain byte (+0x26, FUN_000b9fdd) -> the packed (R<<16)|L the arm extracts
// for ch+0x64/+0x68 (FUN_000a6579 0xA6601..0xA6608). Pan > 0x7F mirrors
// (0xFF-pan). For p in 0..0x7F: p<0x40 keeps L 0x7F and raises R to 2p;
// p==0x40 is 0x7F/0x7F (centre); p>0x40 drops L by (0x7F-p)*0x7E/0x3E and
// keeps R 0x7F. Each factor is scaled by the MOVSX gain with a 32-bit IMUL and
// an unsigned DIV 0x7F (0xA6681..0xA66A4; a >0x7F gain byte is negative, so
// the quotient is the unsigned reading of the negative product). The packed
// value keeps L = quotient & 0x7F and R = (right_q & 0xFFFF) | (left_q >> 16)
// (0xA66A1 SHL/OR with the 0xA6601 SHR/0xA6605 AND extraction). The wide
// helper returns those exact reader gains: L 0..0x7F, R 0..0xFFFF. For a
// retail 0..0x7F gain both are <= 0x7F and identical to the narrow helper.
void fifa96_mixer_pan_gains_wide(uint8_t pan, uint8_t gain,
                                 uint32_t *left, uint32_t *right);

// Low-byte compatibility form of fifa96_mixer_pan_gains_wide: unchanged for
// every retail 0..0x7F gain; a wide R > 0xFF is truncated (the exact value is
// in the wide helper and in the voice's gain fields).
void fifa96_mixer_pan_gains(uint8_t pan, uint8_t gain,
                            uint8_t *left, uint8_t *right);

// Apply fifa96_mixer_pan_gains to an armed voice using the combined gain
// passed to fifa96_mixer_start, like FUN_000a6579's arm-time conversion.
// Returns FIFA96_OK, or -(FIFA96_ERR_TRUNCATED) for NULL m, a bad voice or
// an inactive voice.
int fifa96_mixer_set_pan(struct fifa96_mixer *m, int voice, uint8_t pan);
