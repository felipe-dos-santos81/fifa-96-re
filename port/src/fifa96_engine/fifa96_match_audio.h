#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_platform.h"
#include "fifa96_loader/fifa96_bnk.h"
#include "fifa96_loader/fifa96_mixer.h"
#include "fifa96_loader/fifa96_sfx.h"

/* Match audio (P3): the smallest audible chain from the native sound trampoline
 * FUN_000974DC(id) (FU-113 §1, FU-229 §1.1) into the SDL3 device.
 *
 * Evidence:
 *  - SFX id space and banks: FU-43 §1/§3.2. /SOUND/SFX_GAME.BNK carries ids
 *    1..31; /SOUND/CHN_*.BNK carries the looping chants 33..58.
 *  - whistle: 0x974DC(0x1E) -> 0x65CC0 -> 0xA7705(id, volume) -> 0xA7728 ->
 *    0xA780E (FU-229 §1, FU-51 §4/§5). The eight staged match writes
 *    (`match_run_ref_apply`, `match_run_ref_step_restart`, the act-0x1D wait
 *    arm and `fifa96_match_action_phase_21/22`) stage `mr->ref_whistle = 0x1E`
 *    and increment `mr->whistle_seq`; this module plays one cue per sequence
 *    change. The counter is what makes a clear + re-set inside one frame body
 *    audible (the level byte alone reads unchanged at the audio step).
 *  - crowd: the native ambience machine FUN_00091F60/FUN_000920C0 selects a CHN
 *    id per event (FU-49 §1.10/§1.11). The engine has no team identity, so the
 *    generic CHN_GEN1 id 55 is the recorded default (A6-L3); its EACS loop
 *    fields 10753 + 97673 arm a 6.1 s loop at 16 kHz (FU-43 §2, [M] probe).
 *  - output: 22050 Hz 16-bit stereo, FU-37 §A.4; 220/221 frames per 10 ms PIT
 *    tick via a fractional accumulator (100 * 22050 = 2,205,000).
 *
 * The module never opens the device: the engine calls audio_open once at boot
 * (fifa96_engine.c) and passes the platform here for audio_submit. It is a
 * no-op when both banks are absent, which keeps the headless/no-ISO tapes
 * silent; the engine additionally does not step it when cfg.headless is set.
 *
 * Open legs (A6):
 *  - A6-L1: match effect id map (1,2,4,8,0xB,0xC,0xE); no kick cue is staged
 *    by the port, so none is invented here.
 *  - A6-L2: the native goal audio is the FUN_00066724 commentary chain; the
 *    score-edge id 15 cue below is a labelled stand-in.
 *  - A6-L3: team/event CHN selection, the 20..59 s ambience timers and the
 *    0x37 volume ramp.
 *  - A6-L4: CRD music (raw 8-bit mono, no mixer format).
 *  - A6-L5: commentary (PHR_* phrase banks, ref_speech unimplemented).
 *  - A6-L6: channel/master volume model ([0x55D00..], [0x15FD6], settings
 *    0x11/0x12/0x13); P6 scales the two component base gains by the Options
 *    SFX Volume (0x11) gated by Sound Effects (0x01). Music/Speech Volume
 *    still have no consumer (P6-L2).
 *  - A6-L8: intro video companion audio (`fifa96_intro.c` discards the
 *    `1SNh`/`1SNd` chunks, FU-35 §1); this match slice does not touch the
 *    intro, so it is out of scope here.
 *  - A6-L9: the unported 0x974DC(0x1E) sinks (row-01/0x10/0x11/0x12/0x13 and
 *    the scanner) do not move ref_whistle/whistle_seq, so they stay unheard.
 */

#define FIFA96_MATCH_AUDIO_RATE 22050u
#define FIFA96_MATCH_AUDIO_CHUNK 512u
#define FIFA96_MATCH_AUDIO_CROWD_ID 55u         /* CHN_GEN1, A6-L3 default */
#define FIFA96_MATCH_AUDIO_WHISTLE_ID 0x1Eu     /* 0x974DC(0x1E) */
#define FIFA96_MATCH_AUDIO_GOAL_ID 15u          /* labelled stand-in, A6-L2 */
#define FIFA96_MATCH_AUDIO_CROWD_VOLUME 0x60u
#define FIFA96_MATCH_AUDIO_CUE_VOLUME 0x7Fu
#define FIFA96_MATCH_AUDIO_MASTER 0x7Fu
#define FIFA96_MATCH_AUDIO_PERCENT_UNITY 100u
/* Bound on the whistle writes consumed per audio step. A frame body stages at
 * most a couple of 0x1E writes; the cap keeps a re-based/garbage counter from
 * turning one step into an unbounded cue burst. */
#define FIFA96_MATCH_AUDIO_MAX_WHISTLE_BURST 4u

struct fifa96_match_run;

struct fifa96_match_audio {
  struct fifa96_mixer mixer;
  struct fifa96_bnk_info sfx;    /* borrowed: cache bytes outlive this state */
  struct fifa96_bnk_info crowd;  /* borrowed */
  struct fifa96_sfx_id_table ids; /* SFX_GAME ids 1..31, skipping empty slots */
  int16_t pcm[2 * FIFA96_MATCH_AUDIO_CHUNK];
  uint64_t acc;              /* rate-weighted remainder (frames * 100) */
  uint8_t last_whistle;      /* previous ref_whistle observation (fallback) */
  uint32_t last_whistle_seq; /* previous whistle_seq observation */
  uint32_t last_score;       /* previous score[0] + score[1] (goal edge) */
  int crowd_voice;           /* armed CHN loop voice, -1 before/if absent */
  int have_sfx;              /* SFX_GAME parsed */
  int have_crowd;            /* CHN_GEN1 parsed */
  int active;                /* start ran; step may still be asset-disabled */
  uint32_t sfx_percent;      /* P6 SFX volume x Sound Effects, 0..100 */
  int last_whistle_voice;    /* voice of the most recent whistle, -1 if none */
  uint8_t last_whistle_gain; /* mixer gain byte of that whistle (0..0x7F) */
  uint32_t whistle_cues;
  uint32_t goal_cues;
  uint64_t frames_submitted;
};

/* Parse both banks, fill the id table, arm the CHN 55 crowd loop and seed the
 * cue edges from the run. A NULL/short/unparsable bank disables that half; the
 * call still succeeds with `active = 1` (the engine's soft-failure policy, the
 * same one used for match art staging). `sfx_percent` is the Options SFX
 * Volume (0..100, 100 = unity) gated by Sound Effects; every base gain below
 * is scaled by it, so 91 gives the default crowd gain 0x57 (not 0x60). */
int fifa96_match_audio_start(struct fifa96_match_audio *a,
                             struct fifa96_match_run *mr,
                             const uint8_t *sfx, size_t sfx_len,
                             const uint8_t *crowd, size_t crowd_len,
                             uint32_t sfx_percent);

/* One engine step. `ticks` is the PIT ticks the clock advanced (0 is legal:
 * cue edges are still consumed, no PCM is submitted). Plays one whistle cue
 * per `whistle_seq` change (usually one per staged write; two writes inside
 * one frame body fire two cues), with the `ref_whistle` level change kept as
 * the compatibility fallback for fixtures that stage the byte directly. The
 * A6-L2 goal stand-in fires on a score increase, the crowd loop is re-armed if
 * its voice was stolen, then floor(frames accumulated) frames are rendered in
 * chunked audio_submit calls. With no bank parsed it returns without touching
 * the platform. */
int fifa96_match_audio_step(struct fifa96_match_audio *a,
                            struct fifa96_match_run *mr,
                            fifa96_platform *plat, uint32_t ticks);

/* Release the voices/state. Bank bytes are borrowed and stay with the cache. */
void fifa96_match_audio_stop(struct fifa96_match_audio *a);
