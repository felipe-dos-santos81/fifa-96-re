#include "fifa96_engine/fifa96_match_audio.h"
#include <string.h>
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_loader/fifa96_err.h"

/* The native replay trampoline (FU-113 §1, FU-229 §1.1):
 *   0x974DC  EDX = EAX (id)
 *   0x974DF  CALL 0x64EC0 (replay ring)
 *   0x974E4  EAX = EDX
 *   0x974E6  CALL 0x65CC0 (voice trigger -> 0xA7705(id, volume) ->
 *            0xA7728 -> 0xA780E)
 * The port keeps only the audible half: the id table is the SFX_GAME bank
 * registration (FU-51 §4 DAT_00061c14) and the arm path is fifa96_sfx. */

static int match_audio_parse_sfx(struct fifa96_match_audio *a,
                                 const uint8_t *sfx, size_t sfx_len) {
  if (!sfx || sfx_len == 0) return 0;
  if (fifa96_bnk_parse(sfx, sfx_len, &a->sfx) != FIFA96_OK) return 0;
  a->have_sfx = 1;
  /* SFX_GAME.BNK is the 1..31 match bank (FU-43 §4). Register exactly the
   * nonzero slots; fifa96_sfx_play_id re-resolves the descriptor per call. */
  for (uint32_t id = 1; id <= 31u; id++) {
    struct fifa96_bnk_entry e;
    if (fifa96_bnk_entry(&a->sfx, id, &e) == FIFA96_OK) a->ids.bank[id] = &a->sfx;
  }
  return 1;
}

static int match_audio_parse_crowd(struct fifa96_match_audio *a,
                                   const uint8_t *crowd, size_t crowd_len) {
  if (!crowd || crowd_len == 0) return 0;
  if (fifa96_bnk_parse(crowd, crowd_len, &a->crowd) != FIFA96_OK) return 0;
  a->have_crowd = 1;
  return 1;
}

/* P6: the Options SFX Volume scales the native component base gains; 100 is
 * unity and the engine's default 91 yields crowd 0x60*91/100 = 0x57. */
static uint8_t match_audio_scale_volume(uint8_t base, uint32_t percent) {
  if (percent > FIFA96_MATCH_AUDIO_PERCENT_UNITY)
    percent = FIFA96_MATCH_AUDIO_PERCENT_UNITY;
  return (uint8_t)((uint32_t)base * percent / FIFA96_MATCH_AUDIO_PERCENT_UNITY);
}

/* Arm the CHN loop. A6-L3 records both choices: CHN_GEN1 is the generic bank
 * because the engine has no team identity (the native FUN_000652f0 selector
 * is unresolved, FU-43 §6 leg 1), and 0x60 is the plan's fixed crowd gain
 * scaled by the Options SFX volume (A6-L6's channel model stays a leg). The
 * CHN EACS +0x14 loop length makes fifa96_mixer_start arm the loop (FU-43
 * §2). */
static void match_audio_arm_crowd(struct fifa96_match_audio *a) {
  struct fifa96_sfx_opts opts;
  struct fifa96_sfx_voice out[2];
  memset(&opts, 0, sizeof opts);
  opts.pan = -1;
  opts.volume = match_audio_scale_volume(FIFA96_MATCH_AUDIO_CROWD_VOLUME,
                                         a->sfx_percent);
  opts.master = FIFA96_MATCH_AUDIO_MASTER;
  memset(out, 0, sizeof out);
  int rc = fifa96_sfx_arm_alloc(&a->mixer, &a->crowd,
                                FIFA96_MATCH_AUDIO_CROWD_ID, &opts, out);
  if (rc >= 0) a->crowd_voice = out[0].voice;
}

int fifa96_match_audio_start(struct fifa96_match_audio *a,
                             struct fifa96_match_run *mr,
                             const uint8_t *sfx, size_t sfx_len,
                             const uint8_t *crowd, size_t crowd_len,
                             uint32_t sfx_percent) {
  if (!a) return -(int)FIFA96_ERR_INVALID;
  memset(a, 0, sizeof *a);
  fifa96_mixer_init(&a->mixer);
  a->crowd_voice = -1;
  a->last_whistle_voice = -1;
  a->active = 1;
  a->sfx_percent = sfx_percent > FIFA96_MATCH_AUDIO_PERCENT_UNITY
                       ? FIFA96_MATCH_AUDIO_PERCENT_UNITY
                       : sfx_percent;
  /* Seed the observation edges from the live run so a value left over from a
   * previous match cannot fire a spurious cue at kickoff. */
  a->last_whistle = mr ? mr->ref_whistle : 0u;
  a->last_whistle_seq = mr ? mr->whistle_seq : 0u;
  a->last_score = mr ? (uint32_t)mr->score[0] + (uint32_t)mr->score[1] : 0u;
  (void)match_audio_parse_sfx(a, sfx, sfx_len);
  if (match_audio_parse_crowd(a, crowd, crowd_len)) match_audio_arm_crowd(a);
  return 0;
}

static void match_audio_play_whistle(struct fifa96_match_audio *a) {
  struct fifa96_sfx_opts opts;
  struct fifa96_sfx_voice out[2];
  memset(&opts, 0, sizeof opts);
  opts.pan = -1;
  opts.volume = match_audio_scale_volume(FIFA96_MATCH_AUDIO_CUE_VOLUME,
                                         a->sfx_percent);
  opts.master = FIFA96_MATCH_AUDIO_MASTER;
  memset(out, 0, sizeof out);
  if (fifa96_sfx_play_id(&a->mixer, &a->ids, FIFA96_MATCH_AUDIO_WHISTLE_ID,
                         &opts, out) < 0)
    return;
  a->last_whistle_voice = out[0].voice;
  a->last_whistle_gain = out[0].gain;
  a->whistle_cues++;
}

/* A6-L2 stand-in: no static goal->sfx mapping exists (the native goal audio is
 * the commentary chain FUN_00066724, OL-89). Id 15 is the longest crowd-like
 * SFX_GAME clip (6.78 s) and is fired on a score increase. */
static void match_audio_play_goal(struct fifa96_match_audio *a) {
  struct fifa96_sfx_opts opts;
  memset(&opts, 0, sizeof opts);
  opts.pan = -1;
  opts.volume = match_audio_scale_volume(FIFA96_MATCH_AUDIO_CROWD_VOLUME,
                                         a->sfx_percent);
  opts.master = FIFA96_MATCH_AUDIO_MASTER;
  if (fifa96_sfx_play_id(&a->mixer, &a->ids, FIFA96_MATCH_AUDIO_GOAL_ID,
                         &opts, NULL) >= 0)
    a->goal_cues++;
}

int fifa96_match_audio_step(struct fifa96_match_audio *a,
                            struct fifa96_match_run *mr,
                            fifa96_platform *plat, uint32_t ticks) {
  if (!a || !a->active) return 0;
  if (!a->have_sfx && !a->have_crowd) return 0;   /* no assets: stay silent */
  if (!mr || !plat) return -(int)FIFA96_ERR_INVALID;

  /* Whistle: `whistle_seq` is the durable edge. The run increments it at every
   * native 0x974DC(0x1E) write, so a clear + re-set inside one frame body is
   * still observable (the byte alone reads unchanged). Consume the pending
   * writes, normally one; two writes between audio steps fire two cues. A
   * lowered counter means the run was reset (a fresh match), not 2^32 writes.
   * The byte comparison stays as the compatibility fallback for fixtures that
   * stage `ref_whistle` directly; a held slot does not repeat. */
  uint32_t seq = mr->whistle_seq;
  uint32_t pending = seq - a->last_whistle_seq;
  if (seq < a->last_whistle_seq) pending = 0u;   /* fresh run, not a wrap */
  if (pending > FIFA96_MATCH_AUDIO_MAX_WHISTLE_BURST)
    pending = FIFA96_MATCH_AUDIO_MAX_WHISTLE_BURST;
  for (uint32_t i = 0; i < pending; i++) match_audio_play_whistle(a);
  uint8_t whistle = mr->ref_whistle;
  if (pending == 0u && whistle != 0u && whistle != a->last_whistle)
    match_audio_play_whistle(a);
  a->last_whistle_seq = seq;
  a->last_whistle = whistle;

  /* Goal edge: score[0] + score[1] increase (A6-L2 stand-in). */
  uint32_t total = (uint32_t)mr->score[0] + (uint32_t)mr->score[1];
  if (total > a->last_score) match_audio_play_goal(a);
  a->last_score = total;

  /* The crowd loop can be stolen by the whistle/goal allocator; re-arm it. */
  if (a->have_crowd && (a->crowd_voice < 0 ||
                        !fifa96_mixer_voice_active(&a->mixer, a->crowd_voice)))
    match_audio_arm_crowd(a);

  /* 22050 frames/s over 100 PIT ticks/s: 220/221 frames per 10 ms tick. The
   * remainder is rate-weighted so the long-run rate is exact. */
  a->acc += (uint64_t)FIFA96_MATCH_AUDIO_RATE * (uint64_t)ticks;
  size_t frames = (size_t)(a->acc / 100u);
  a->acc %= 100u;
  if (frames == 0 || !plat->audio_submit) return 0;
  while (frames != 0) {
    size_t n = frames > FIFA96_MATCH_AUDIO_CHUNK ? FIFA96_MATCH_AUDIO_CHUNK : frames;
    fifa96_mixer_render(&a->mixer, a->pcm, n);
    plat->audio_submit(plat->self, a->pcm, n);
    a->frames_submitted += n;
    frames -= n;
  }
  return 0;
}

void fifa96_match_audio_stop(struct fifa96_match_audio *a) {
  if (!a) return;
  memset(a, 0, sizeof *a);
  a->crowd_voice = -1;
  a->last_whistle_voice = -1;
}
