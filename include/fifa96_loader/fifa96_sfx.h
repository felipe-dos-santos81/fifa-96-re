#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_bnk.h"
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_mixer.h"
// Ghidra: play lookup FUN_000a7728 @ 0xA7728, bank arm FUN_000a780e @ 0xA780E,
// two-voice split FUN_000a6717 @ 0xA6717, EACSNDF gain FUN_000b9fdd @ 0xB9FDD,
// master volume FUN_000a6d9b @ 0xA6D9B. Spec: docs/ghidra/FU43_bnk_format.md
// §3.2.
//
// fifa96_sfx_arm is the .BNK play path: global id -> descriptor
// (fifa96_bnk_entry) -> arm parameters -> fifa96_mixer_start. Every retail
// descriptor (FU-43 §1.1/§4) has pitch 0, volume base 0x7F (0x3C/0x32 in the
// wider corpus), pan centre 0x40, zero randomization spans, no pan override
// and no two-voice flag, so the golden path resolves without randomness; the
// other branches are ported from the disassembly.
//
// Modelled and not modelled, per FU-43's open legs:
//   * voice allocation FUN_000a62fa (descriptor +0x00 mask, +0x14 priority)
//     is not ported: the caller passes the mixer voice, and the two-voice
//     path uses voice and voice+1 (the original allocates two independent
//     records and returns (voice2<<16)|voice1, 0xA77F1).
//   * the sound-state gate [0x15FC8] in 1..5 (0xA7852, error -4) is engine
//     state and is not modelled; the port always arms. The EACS tag is
//     checked before any randomization draw, as in the original (0xA7840).
//   * record+0x26 gain = caller volume (record+0x24) x randomized descriptor
//     volume (record+0x25) x master (0x15FD6) / 0x3F01 (FUN_000b9fdd,
//     0xB9FEA..0xB9FF5); the mixer volume is that value, kept in 0..0x7F.
//   * pan is resolved per FU-43 +0x18/+0x1D and reported, but the L/R gain
//     conversion FUN_000a662c (record+0x27 -> ch+0x64/+0x68) is the existing
//     mixer open leg (fifa96_mixer.h), so both channels use the combined
//     gain.
//   * the pitch table FUN_000b86c8 (pitch -> ratio/shift) is FU-37 §A.5 open
//     leg 2; the arm reports pitch and arms at table[0] (ratio 0x10000,
//     shift 16), which is the retail pitch-0 case (0xA78CD writes record+0xC
//     = 0 when +0x0C/+0x10 are zero in every retail entry).
//   * randomization draws come from FUN_000cbc4c, a 192-bit carry generator
//     seeded by FUN_000cbcb8 (out of FU-43 scope); the arm takes a provider
//     and applies the cited formula with `rand() >> 16` (SHR EAX,0x10).
//   * the two-voice branch is static-only in retail (no entry sets +0x1C bit
//     0) and is ported from the disassembly, including FUN_000a6717's table
//     DAT_000148e0 = {150,140,130,120,110} (image 0x1148E0) and its
//     invalid-pan branch, which reads ESI left by the caller (the id,
//     0xA772E -> 0xA67B0).

#define FIFA96_SFX_OUT_RATE 22050u    /* FU-37 §A.4: output rate */
#define FIFA96_SFX_PITCH_RATIO 0x10000u /* FUN_000b86c8 table[0] */
#define FIFA96_SFX_PITCH_SHIFT 16u
#define FIFA96_SFX_VOLUME_MAX 0x7Fu

// Randomization provider standing in for FUN_000cbc4c; the arm consumes the
// high 16 bits of the returned word. May be NULL while no descriptor span is
// nonzero (every retail entry).
typedef uint32_t (*fifa96_sfx_rand_fn)(void *ctx);

struct fifa96_sfx_opts {
  int32_t pan;     /* 0..0xFF caller pan (FUN_000a6717 mirrors >0x7F), or -1 */
  uint8_t volume;  /* record+0x24 caller volume, 0..0x7F (wrappers 0x7F) */
  uint8_t master;  /* 0x15FD6 master volume, 0..0x7F (0x7F = unity) */
  fifa96_sfx_rand_fn rand;
  void *rand_ctx;
};

struct fifa96_sfx_voice {
  uint32_t id;     /* global SFX id armed in this slot */
  int32_t voice;   /* mixer voice */
  uint8_t volume;  /* record+0x25: descriptor volume after randomization */
  uint8_t pan;     /* record+0x27: resolved pan (0x40 centre) */
  uint8_t caller;  /* record+0x24: caller/split volume */
  uint8_t gain;    /* record+0x26, the mixer volume (0..0x7F) */
  int32_t pitch;   /* record+0xC: pitch base + randomization */
  int loop;        /* mixer loop armed (EACS +0x10/+0x14 propagated) */
  uint32_t loop_start;
  uint32_t loop_end;
};

// Arm `id` from a parsed bank as mixer voice `voice` (0..15), resolving the
// descriptor per FU-43 §3.2 and starting it through fifa96_mixer_start with
// the bank file itself as the payload (the port of the registrar's
// relocation: the embedded EACS +0x18 is an absolute file offset, so the
// mixer borrows bank->src, FU-43 §1/§4.1).
//
// opts semantics (NULL = the FUN_000a76ea wrapper defaults: pan -1,
// volume 0x7F, master 0x7F, no provider):
//   pan != -1 -> used as-is (0xA796C);
//   pan == -1 && desc[0x1D] == 0 -> desc[0x18] (0xA7967);
//   pan == -1 && desc[0x1D] != 0 -> 0x40 + span draw, clamped to 0x7F; when
//     the first (discarded, bounds-check) draw passes, the original draws
//     again and stores the second value unchecked as a byte
//     (0xA7931..0xA7965 + FUN_000a79c1) — both draws are modelled.
// Descriptor pitch/volume randomization follows 0xA789D..0xA7922; each draw
// is rand() >> 16.
//
// out may be NULL. On success out[0] describes the id voice; on the
// two-voice path (descriptor +0x1C bit 0) out[1] describes the id+1 voice
// (which must exist) and the return value is ((voice+1)<<16) | voice. The
// call is atomic: on an error no voice is left armed.
//
// Returns the armed voice (>= 0, or the packed pair) or the negated
// fifa96_err_t: NOT_FOUND for an absent id (BNK table slot 0; for the
// two-voice path id+1), TRUNCATED for NULL bank and bad voice/id/pan/volume,
// BAD_MAGIC/UNSUPPORTED/TRUNCATED from the bank EACS parse, UNSUPPORTED when
// a needed draw has no provider or the format has no decoder, and the
// mixer's own error otherwise.
int fifa96_sfx_arm(struct fifa96_mixer *m, int voice,
                   const struct fifa96_bnk_info *bank, uint32_t id,
                   const struct fifa96_sfx_opts *opts,
                   struct fifa96_sfx_voice out[2]);
