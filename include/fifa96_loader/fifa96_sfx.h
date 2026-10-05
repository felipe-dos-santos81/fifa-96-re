#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_bnk.h"
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_mixer.h"
// Ghidra: play lookup FUN_000a7728 @ 0xA7728, bank arm FUN_000a780e @ 0xA780E,
// two-voice split FUN_000a6717 @ 0xA6717, EACSNDF gain FUN_000b9fdd @ 0xB9FDD,
// master volume FUN_000a6d9b @ 0xA6D9B. Spec: docs/ghidra/FU43_bnk_format.md
// §3.2; the >0x7F two-voice split read-back is FU-48 §2
// (docs/ghidra/FU48_audio_edge_cases.md).
//
// fifa96_sfx_arm is the .BNK play path: global id -> descriptor
// (fifa96_bnk_entry) -> arm parameters -> fifa96_mixer_start. Every retail
// descriptor (FU-43 §1.1/§4) has pitch 0, volume base 0x7F (0x3C/0x32 in the
// wider corpus), pan centre 0x40, zero randomization spans, no pan override
// and no two-voice flag, so the golden path resolves without randomness; the
// other branches are ported from the disassembly.
//
// Modelled and not modelled, per FU-43/FU-47's open legs:
//   * voice allocation FUN_000a62fa (descriptor +0x00 mask, +0x14 priority)
//     is ported twice: fifa96_sfx_arm keeps the caller-selected voice, while
//     fifa96_sfx_arm_alloc uses fifa96_mixer_alloc_voice for the original's
//     allocator policy (FU-47 §1; two-voice allocates independently and
//     returns (voice2<<16)|voice1, 0xA77F1).
//   * the sound-state gate [0x15FC8] in 1..5 (0xA7852, error -4) is modelled
//     as fifa96_mixer.sound_state: the check sits after the EACS tag and
//     before the first randomization draw, as in the original (0xA7840 then
//     0xA7852 then 0xA789D). Default state 1 keeps pre-FU-47 callers arming.
//   * record+0x26 gain = caller volume (record+0x24) x randomized descriptor
//     volume (record+0x25) x master (0x15FD6) / 0x3F01 (FUN_000b9fdd,
//     0xB9FEA..0xB9FF5); the mixer volume is that value, kept in 0..0x7F.
//   * pan is resolved per FU-43 +0x18/+0x1D and then converted through the
//     FU-46 §2 FUN_000a662c pan/gain split (record+0x27 -> ch+0x64/+0x68 via
//     fifa96_mixer_set_pan), exactly like FUN_000a780e's call to FUN_000a6579
//     at 0xA7991; centre pan leaves both reader gains at the combined gain.
//   * the pitch table FUN_000b86c8 head+tail is ported (FU-46 §1); the arm
//     computes the step with fifa96_mixer_step_from_pitch from the
//     record+0xC pitch, so retail pitch 0 is table[0] 0x10000/shift 16
//     (0xA78CD writes record+0xC = 0 when +0x0C/+0x10 are zero in every
//     retail entry).
//   * randomization draws come from the FUN_000cbc4c generator, ported as
//     fifa96_sfx_rng_* (FU-47 §2) and usable through
//     fifa96_sfx_rng_default; the arm still consumes `rand() >> 16`
//     (SHR EAX,0x10) and a NULL provider plus a needed draw stays
//     UNSUPPORTED, so tests keep the injectable hook.
//   * the two-voice branch is static-only in retail (no entry sets +0x1C bit
//     0) and is ported from the disassembly, including FUN_000a6717's table
//     DAT_000148e0 = {150,140,130,120,110} (image 0x1148E0) and its
//     invalid-pan branch, which reads ESI left by the caller (the id,
//     0xA772E -> 0xA67B0).

#define FIFA96_SFX_OUT_RATE 22050u    /* FU-37 §A.4: output rate */
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

// The original arm signature (FUN_000a780e): the voice is chosen by
// fifa96_mixer_alloc_voice from the descriptor's +0x00 mask and +0x14
// priority (0xA781F..0xA7825) before the EACS tag (0xA7840), the sound-state
// gate (0xA7852) and the randomization draws (0xA789D..). The two-voice
// descriptor flag allocates independently for id and id+1
// (FUN_000a7728 -> two FUN_000a780e calls, 0xA77A3/0xA77D6) and returns
// ((voice2)<<16)|voice1. On any error the port leaves no voice armed; the
// rotor and the chosen record's +0x12 have already moved, as in the original.
// Returns the armed voice (>= 0) or the packed pair, or the negated
// fifa96_err_t: NO_VOICE when the allocator matches nothing (original -0x14,
// 0xA7830), the fifa96_sfx_arm errors otherwise.
int fifa96_sfx_arm_alloc(struct fifa96_mixer *m,
                         const struct fifa96_bnk_info *bank, uint32_t id,
                         const struct fifa96_sfx_opts *opts,
                         struct fifa96_sfx_voice out[2]);

// FUN_000cbc4c @ 0xCBC4C (FU-47 §2): a six-word 32-bit generator at object-4
// 0x12E68..0x12E7F (image 0x112E68). Word order is address order: w[0] =
// [0x12E68] is the word returned each step (SHR EAX,0x10 by the arm), w[5] =
// [0x12E7C] is the word incremented every step. One step suffix-sums
// w[5]+w[4]..w[0] into w[4]..w[0] with carries (ADD/ADC chain
// 0xCBC4C..0xCBC83), increments w[5] (0xCBC88) with a carry cascade up to
// w[0] (0xCBC90..0xCBCB0), and returns w[0]; a full 192-bit wrap also bumps
// the returned word (INC EAX 0xCBCB6). Not a standard LCG form; the
// disassembly is the spec (FU-47 §2.1).
struct fifa96_sfx_rng { uint32_t w[6]; };

// Image 0x112E68 state = FUN_000cbcb8(0); the default deterministic state.
void fifa96_sfx_rng_init(struct fifa96_sfx_rng *rng);

// FUN_000cbcb8 @ 0xCBCB8: w[i] = seed + c[0]+..+c[i] with
// c = {0xF22D0E56, 0x96041893, 0x3DF3B646, 0x40DDE76D, 0x97327AE1,
// 0xD1A9FBE7}; the six constants are the seed-0 image words' differences.
void fifa96_sfx_rng_seed(struct fifa96_sfx_rng *rng, uint32_t seed);

// The game's own seeder at 0x4C698: w[i] = (seed << 25) + (int8)key[i] with
// key = "ArCaDe-CoInOp" (image 0x101D98). Called from 0x493E3 with the tick
// counter FUN_000cb2a4 ([0x12E88]); the other boot seed is 0x17CC7
// srand((b[0x66]*ticks + b[0x67]) << 16) (FU-47 §2.3).
void fifa96_sfx_rng_seed_arcade(struct fifa96_sfx_rng *rng, uint32_t seed);

// One FUN_000cbc4c step; NULL yields 0.
uint32_t fifa96_sfx_rng_next(struct fifa96_sfx_rng *rng);

// fifa96_sfx_rand_fn adapter: ctx must be struct fifa96_sfx_rng *.
uint32_t fifa96_sfx_rng_default(void *ctx);
