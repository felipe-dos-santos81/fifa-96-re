/* port/tests/test_engine_audio.c — P3 match audio.
 *
 * The module is exercised against the two retail banks read through the same
 * asset table/cache the engine uses:
 *   /SOUND/SFX_GAME.BNK  ids 1..31 (whistle id 0x1E, A6 §1.1)
 *   /SOUND/CHN_GEN1.BNK  id 55, DELTA_MONO, loop 10753 + 97673 (A6 §1.2)
 *
 * Assertions:
 *  - no banks: start succeeds disabled, step submits no PCM (the headless /
 *    no-ISO contract);
 *  - retail banks: crowd id 55 arms a looping voice at gain 0x60, 100 x 10 ms
 *    ticks submit exactly 22050 stereo frames (220/221 cadence) and the PCM is
 *    non-silent;
 *  - a ref_whistle change plays one whistle cue with a nonzero gain, the same
 *    held value does not repeat, and a clear/re-set fires again;
 *  - the durable whistle_seq counter increments only at the native 0x1E
 *    writers, makes a clear+re-set inside one step audible, and fires one cue
 *    per pending write (two writes -> two cues);
 *  - a score increase fires the A6-L2 goal stand-in once;
 *  - P6: SFX Volume scales the crowd/whistle/goal base gains (100/91/50/0)
 *    and the engine's Sound Effects gate yields percent 0 and silence.
 *  - C4: arming the replay duck stops the crowd (no re-arm while ducked),
 *    scales the whistle/goal caller percent by 1/3 (a second arm does not
 *    divide again), and the release restores the full percent and re-arms the
 *    crowd; the no-bank path only moves the flag.
 *
 * ISO-gated: without data/game/FIFAPCCD96.iso the retail half prints SKIP and
 * the no-bank half still runs, matching the M1/M2 no-ISO convention.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_cache.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_match_audio.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_loader/fifa96_err.h"

#define AUDIO_ISO_PATH "data/game/FIFAPCCD96.iso"

/* Capturing sink: the module only ever calls audio_submit, so a bare platform
 * struct keeps the test independent of the null backend's hash. */
struct audio_capture {
  uint64_t frames;
  uint64_t nonzero;
  uint64_t energy;   /* sum of |sample| */
  uint32_t peak;
};

static void capture_submit(void *self, const int16_t *pcm, size_t frames) {
  struct audio_capture *c = self;
  c->frames += frames;
  for (size_t i = 0; i < frames * 2u; i++) {
    int16_t s = pcm[i];
    uint32_t mag = s < 0 ? (uint32_t)(-(int32_t)s) : (uint32_t)s;
    if (s != 0) c->nonzero++;
    c->energy += mag;
    if (mag > c->peak) c->peak = mag;
  }
}

static int iso_present(void) {
  FILE *f = fopen(AUDIO_ISO_PATH, "rb");
  if (!f) return 0;
  fclose(f);
  return 1;
}

static void test_no_assets_is_silent(void) {
  struct fifa96_match_run *mr = calloc(1, sizeof *mr);
  assert(mr != NULL);
  struct fifa96_match_audio a;
  struct audio_capture cap;
  memset(&cap, 0, sizeof cap);
  fifa96_platform plat;
  memset(&plat, 0, sizeof plat);
  plat.audio_submit = capture_submit;
  plat.self = &cap;

  assert(fifa96_match_audio_start(&a, mr, NULL, 0, NULL, 0,
                                  FIFA96_MATCH_AUDIO_PERCENT_UNITY) == 0);
  assert(a.active == 1);
  assert(a.have_sfx == 0 && a.have_crowd == 0);
  assert(a.crowd_voice == -1);
  fifa96_match_audio_replay_arm(&a);
  assert(a.replay_ducked == 1 && a.crowd_voice == -1);
  fifa96_match_audio_replay_release(&a);
  assert(a.replay_ducked == 0);
  for (int i = 0; i < 10; i++)
    assert(fifa96_match_audio_step(&a, mr, &plat, 1) == 0);
  assert(cap.frames == 0);       /* no banks -> no submit */
  assert(cap.energy == 0);
  fifa96_match_audio_stop(&a);
  assert(a.active == 0);
  free(mr);
}

/* FU-343 (P3 fix): the counter is the durable whistle edge. Init/begin zero
 * it, the native 0x1E writers increment it once each, and the referee step's
 * frame-head clear is a level clear (not an assertion), so it must not move
 * the counter. */
static void test_whistle_seq_counter(void) {
  struct fifa96_match_run mr;
  memset(&mr, 0xAA, sizeof mr);
  fifa96_match_run_init(&mr);
  assert(mr.whistle_seq == 0u);
  assert(mr.ref_whistle == 0u);

  /* phase 21 stage 0 is a native 0x974DC(0x1E) writer. */
  assert(fifa96_match_action_phase_21(&mr) == FIFA96_OK);
  assert(mr.ref_whistle == FIFA96_MATCH_AUDIO_WHISTLE_ID);
  assert(mr.whistle_seq == 1u);
  /* phase 22 stage 0 is the second writer shape; stage 0 again after the
   * referee step's level clear. */
  mr.ref_whistle = 0u;                 /* the referee clear (no counter) */
  mr.act.stage = 0u;
  assert(fifa96_match_action_phase_22(&mr) == FIFA96_OK);
  assert(mr.ref_whistle == FIFA96_MATCH_AUDIO_WHISTLE_ID);
  assert(mr.whistle_seq == 2u);

  uint32_t seq = mr.whistle_seq;
  assert(fifa96_match_run_referee_step(&mr) == 0);
  assert(mr.ref_whistle == 0u);
  assert(mr.whistle_seq == seq);       /* the clear is not an assertion */
}

/* P6: the Options SFX Volume scales the base gains. 100 is unity, the default
 * 91 gives a crowd caller volume of 0x60*91/100 = 0x57, 50 gives 0x30 and 0
 * gives silence. The goal cue uses the same crowd caller volume (A6-L2
 * stand-in); id 15's own descriptor base volume is 0x32, so its mixer gain is
 * 0x32 * caller * master / 0x3F01 (100 -> 0x25, 50 -> 0x12, 0 -> 0). */
static void test_volume_scale(const uint8_t *sfx, size_t sfx_len,
                              const uint8_t *crowd, size_t crowd_len) {
  static const struct {
    uint32_t percent;
    uint8_t crowd_volume;
  } cases[] = {{100u, 0x60u}, {91u, 0x57u}, {50u, 0x30u}, {0u, 0x00u}};
  for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
    struct fifa96_match_run *mr = calloc(1, sizeof *mr);
    assert(mr != NULL);
    struct fifa96_match_audio a;
    struct audio_capture cap;
    memset(&cap, 0, sizeof cap);
    fifa96_platform plat;
    memset(&plat, 0, sizeof plat);
    plat.audio_submit = capture_submit;
    plat.self = &cap;
    assert(fifa96_match_audio_start(&a, mr, sfx, sfx_len, crowd, crowd_len,
                                    cases[i].percent) == 0);
    assert(a.active == 1 && a.have_sfx == 1 && a.have_crowd == 1);
    assert(a.sfx_percent == cases[i].percent);
    assert(a.crowd_voice >= 0 && a.crowd_voice < FIFA96_MIXER_VOICES);
    assert(fifa96_mixer_voice_active(&a.mixer, a.crowd_voice) == 1);
    assert(a.mixer.voices[a.crowd_voice].volume == cases[i].crowd_volume);

    /* The whistle caller volume is 0x7F * percent / 100; its mixer gain is
     * volume 0x7F x caller x master 0x7F / 0x3F01. */
    mr->whistle_seq = 1;
    mr->ref_whistle = FIFA96_MATCH_AUDIO_WHISTLE_ID;
    assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
    assert(a.whistle_cues == 1u);
    uint32_t caller = 0x7Fu * cases[i].percent / 100u;
    uint8_t expected_gain = (uint8_t)(0x7Fu * caller * 0x7Fu / 0x3F01u);
    assert(a.last_whistle_gain == expected_gain);

    /* Goal edge: the id-15 cue caller volume is the crowd base scaled by
     * percent; the descriptor base 0x32 then forms the mixer gain. */
    mr->score[0] = 1;
    assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
    assert(a.goal_cues == 1u);
    uint32_t goal_caller = 0x60u * cases[i].percent / 100u;
    uint8_t expected_goal_gain =
        (uint8_t)(0x32u * goal_caller * 0x7Fu / 0x3F01u);
    assert(a.last_goal_gain == expected_goal_gain);

    /* One 10 ms tick renders 2205 frames; at percent 0 the crowd loop, the
     * held whistle and the goal cue must all be silent. */
    assert(fifa96_match_audio_step(&a, mr, &plat, 10) == 0);
    assert(cap.frames == FIFA96_MATCH_AUDIO_RATE / 10u);
    if (cases[i].percent == 0) {
      assert(a.last_whistle_gain == 0u);
      assert(a.last_goal_gain == 0u);
      assert(cap.energy == 0);
    } else {
      assert(cap.energy > 0);
    }

    fifa96_match_audio_stop(&a);
    free(mr);
  }
}

/* P6 (T3 review F4): the engine's Sound Effects gate. `engine_match_audio_begin`
 * passes fifa96_engine_audio_percent() to fifa96_match_audio_start; off must be
 * percent 0, and a percent-0 start must leave the crowd/whistle/goal silent. */
static void test_sound_effects_gate(const uint8_t *sfx, size_t sfx_len,
                                    const uint8_t *crowd, size_t crowd_len) {
  struct fifa96_engine *e = calloc(1, sizeof *e);
  assert(e != NULL);
  e->frontend.menu.settings.value[0x01] = 1;   /* Sound Effects On */
  e->frontend.menu.settings.value[0x11] = 91;  /* default SFX Volume */
  assert(fifa96_engine_audio_percent(e) == 91);
  e->frontend.menu.settings.value[0x01] = 0;
  assert(fifa96_engine_audio_percent(e) == 0);
  e->frontend.menu.settings.value[0x01] = 1;
  e->frontend.menu.settings.value[0x11] = -5;
  assert(fifa96_engine_audio_percent(e) == 0);   /* low clamp */
  e->frontend.menu.settings.value[0x11] = 150;
  assert(fifa96_engine_audio_percent(e) == 100); /* high clamp */
  e->frontend.menu.settings.value[0x11] = 50;
  assert(fifa96_engine_audio_percent(e) == 50);
  assert(fifa96_engine_audio_percent(NULL) == 0);
  e->frontend.menu.settings.value[0x01] = 0;     /* Off -> 0 regardless */
  e->frontend.menu.settings.value[0x11] = 91;
  uint32_t percent = (uint32_t)fifa96_engine_audio_percent(e);
  assert(percent == 0u);

  struct fifa96_match_run *mr = calloc(1, sizeof *mr);
  assert(mr != NULL);
  struct fifa96_match_audio a;
  struct audio_capture cap;
  memset(&cap, 0, sizeof cap);
  fifa96_platform plat;
  memset(&plat, 0, sizeof plat);
  plat.audio_submit = capture_submit;
  plat.self = &cap;
  assert(fifa96_match_audio_start(&a, mr, sfx, sfx_len, crowd, crowd_len,
                                  percent) == 0);
  assert(a.sfx_percent == 0u);
  assert(a.crowd_voice >= 0);
  assert(a.mixer.voices[a.crowd_voice].volume == 0u);
  mr->whistle_seq = 1;
  mr->ref_whistle = FIFA96_MATCH_AUDIO_WHISTLE_ID;
  mr->score[1] = 1;
  assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
  assert(a.whistle_cues == 1u && a.last_whistle_gain == 0u);
  assert(a.goal_cues == 1u && a.last_goal_gain == 0u);
  assert(fifa96_match_audio_step(&a, mr, &plat, 10) == 0);
  assert(cap.frames == FIFA96_MATCH_AUDIO_RATE / 10u);
  assert(cap.energy == 0);

  fifa96_match_audio_stop(&a);
  free(mr);
  free(e);
}

static void test_retail_banks(void) {
  struct fifa96_asset_table *assets = NULL;
  assert(fifa96_asset_mount_file(AUDIO_ISO_PATH, &assets) == FIFA96_OK);
  struct fifa96_cache *cache = fifa96_cache_create(assets);
  assert(cache != NULL);
  size_t sfx_len = 0, crowd_len = 0;
  const uint8_t *sfx = fifa96_cache_get(cache, "/SOUND/SFX_GAME.BNK", &sfx_len);
  const uint8_t *crowd = fifa96_cache_get(cache, "/SOUND/CHN_GEN1.BNK", &crowd_len);
  assert(sfx != NULL && sfx_len > 0x200u);
  assert(crowd != NULL && crowd_len > 0x200u);

  struct fifa96_match_run *mr = calloc(1, sizeof *mr);
  assert(mr != NULL);
  struct fifa96_match_audio a;
  struct audio_capture cap;
  memset(&cap, 0, sizeof cap);
  fifa96_platform plat;
  memset(&plat, 0, sizeof plat);
  plat.audio_submit = capture_submit;
  plat.self = &cap;

  assert(fifa96_match_audio_start(&a, mr, sfx, sfx_len, crowd, crowd_len,
                                  FIFA96_MATCH_AUDIO_PERCENT_UNITY) == 0);
  assert(a.active == 1 && a.have_sfx == 1 && a.have_crowd == 1);
  assert(a.ids.bank[FIFA96_MATCH_AUDIO_WHISTLE_ID] == &a.sfx);
  assert(a.ids.bank[FIFA96_MATCH_AUDIO_GOAL_ID] == &a.sfx);
  assert(a.last_whistle_seq == mr->whistle_seq);   /* seeded, no kickoff cue */
  assert(a.crowd_voice >= 0 && a.crowd_voice < FIFA96_MIXER_VOICES);
  assert(fifa96_mixer_voice_active(&a.mixer, a.crowd_voice) == 1);
  assert(a.mixer.voices[a.crowd_voice].loop == 1);
  /* volume 0x7F (descriptor) x 0x60 caller x 0x7F master / 0x3F01 = 0x60. */
  assert(a.mixer.voices[a.crowd_voice].volume == 0x60u);

  /* 100 PIT ticks = 1 s -> exactly 22050 frames, alternating 220/221. */
  for (int i = 0; i < 100; i++)
    assert(fifa96_match_audio_step(&a, mr, &plat, 1) == 0);
  assert(cap.frames == FIFA96_MATCH_AUDIO_RATE);
  assert(a.frames_submitted == FIFA96_MATCH_AUDIO_RATE);
  assert(cap.nonzero > cap.frames);       /* both channels carry samples */
  assert(cap.peak > 0x100u);
  assert(cap.energy / (cap.frames * 2u) > 100u);  /* mean |sample| */
  assert(fifa96_mixer_voice_active(&a.mixer, a.crowd_voice) == 1);

  /* C4: the pre-arm duck is persistent and idempotent. */
  int voice = a.crowd_voice;
  fifa96_match_audio_replay_arm(&a);
  assert(a.replay_ducked == 1);
  assert(!fifa96_mixer_voice_active(&a.mixer, voice));
  assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
  assert(a.crowd_voice == -1);                 /* no re-arm while ducked */
  fifa96_match_audio_replay_arm(&a);           /* idempotent */
  assert(a.replay_ducked == 1 && a.crowd_voice == -1);
  fifa96_match_audio_replay_release(&a);
  assert(a.replay_ducked == 0);
  assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
  assert(a.crowd_voice >= 0);
  assert(fifa96_mixer_voice_active(&a.mixer, a.crowd_voice));

  /* Whistle: one cue per ref_whistle change, none while the slot is held. */
  uint32_t cues = a.whistle_cues;
  mr->ref_whistle = FIFA96_MATCH_AUDIO_WHISTLE_ID;
  assert(fifa96_match_audio_step(&a, mr, &plat, 1) == 0);
  assert(a.whistle_cues == cues + 1u);
  assert(a.last_whistle_voice >= 0 && a.last_whistle_voice < FIFA96_MIXER_VOICES);
  assert(a.last_whistle_gain != 0u);
  assert(fifa96_mixer_voice_active(&a.mixer, a.last_whistle_voice) == 1);
  assert(fifa96_match_audio_step(&a, mr, &plat, 1) == 0);
  assert(a.whistle_cues == cues + 1u);    /* held slot does not repeat */

  /* The stage clears the slot before the frame body; a later set is a new cue
   * even without an intervening audio step (the observation is the edge). */
  mr->ref_whistle = 0;
  assert(fifa96_match_audio_step(&a, mr, &plat, 1) == 0);
  mr->ref_whistle = FIFA96_MATCH_AUDIO_WHISTLE_ID;
  assert(fifa96_match_audio_step(&a, mr, &plat, 1) == 0);
  assert(a.whistle_cues == cues + 2u);

  /* FU-343 (P3 fix): the durable counter. A clear + re-set inside one frame
   * body leaves ref_whistle at 0x1E, so the level edge alone misses it; the
   * run's whistle_seq write makes it observable. */
  uint32_t seq_cues = a.whistle_cues;
  uint32_t seq_before = mr->whistle_seq;
  mr->ref_whistle = 0;                 /* the referee frame-head clear */
  mr->whistle_seq++;                   /* one native 0x974DC(0x1E) write */
  mr->ref_whistle = FIFA96_MATCH_AUDIO_WHISTLE_ID;
  assert(fifa96_match_audio_step(&a, mr, &plat, 1) == 0);
  assert(a.whistle_cues == seq_cues + 1u);
  assert(a.last_whistle_seq == seq_before + 1u);
  /* A held level/sequence does not repeat. */
  assert(fifa96_match_audio_step(&a, mr, &plat, 1) == 0);
  assert(a.whistle_cues == seq_cues + 1u);
  /* Two staged writes between audio steps fire two cues. */
  mr->whistle_seq += 2u;
  assert(fifa96_match_audio_step(&a, mr, &plat, 1) == 0);
  assert(a.whistle_cues == seq_cues + 3u);
  assert(a.last_whistle_seq == seq_before + 3u);
  assert(a.last_whistle_voice >= 0 && a.last_whistle_voice < FIFA96_MIXER_VOICES);
  assert(a.last_whistle_gain != 0u);
  assert(fifa96_mixer_voice_active(&a.mixer, a.last_whistle_voice) == 1);

  /* Goal stand-in (A6-L2): score increase fires id 15 once. */
  assert(a.goal_cues == 0);
  mr->score[0] = 1;
  assert(fifa96_match_audio_step(&a, mr, &plat, 1) == 0);
  assert(a.goal_cues == 1u);
  assert(fifa96_match_audio_step(&a, mr, &plat, 1) == 0);
  assert(a.goal_cues == 1u);

  /* C4: the ducked gain is the native 1/3 percent, and a second arm does not
   * divide it again. The caller/descriptor/master values mirror
   * `test_volume_scale` (whistle caller 0x7F, descriptor 0x7F; goal caller
   * 0x60, descriptor 0x32; master 0x7F). */
  uint32_t duck_percent = a.sfx_percent / 3u;
  uint8_t duck_whistle_gain = (uint8_t)(0x7Fu * (0x7Fu * duck_percent / 100u) *
                                        0x7Fu / 0x3F01u);
  uint8_t duck_goal_gain = (uint8_t)(0x32u * (0x60u * duck_percent / 100u) *
                                     0x7Fu / 0x3F01u);
  fifa96_match_audio_replay_arm(&a);
  mr->whistle_seq++;
  assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
  assert(a.last_whistle_gain == duck_whistle_gain);
  mr->score[0] = 2;
  assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
  assert(a.last_goal_gain == duck_goal_gain);
  fifa96_match_audio_replay_arm(&a);           /* not /9 on a second arm */
  mr->whistle_seq++;
  assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
  assert(a.last_whistle_gain == duck_whistle_gain);
  mr->score[0] = 3;
  assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
  assert(a.last_goal_gain == duck_goal_gain);
  fifa96_match_audio_replay_release(&a);
  assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
  assert(a.crowd_voice >= 0);
  assert(fifa96_mixer_voice_active(&a.mixer, a.crowd_voice));
  uint8_t full_whistle_gain = (uint8_t)(0x7Fu * (0x7Fu * a.sfx_percent / 100u) *
                                        0x7Fu / 0x3F01u);
  uint8_t full_goal_gain = (uint8_t)(0x32u * (0x60u * a.sfx_percent / 100u) *
                                     0x7Fu / 0x3F01u);
  mr->whistle_seq++;
  assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
  assert(a.last_whistle_gain == full_whistle_gain);
  mr->score[0] = 4;
  assert(fifa96_match_audio_step(&a, mr, &plat, 0) == 0);
  assert(a.last_goal_gain == full_goal_gain);

  fifa96_match_audio_stop(&a);
  test_volume_scale(sfx, sfx_len, crowd, crowd_len);
  test_sound_effects_gate(sfx, sfx_len, crowd, crowd_len);
  fifa96_cache_destroy(cache);
  fifa96_asset_unmount(assets);
  free(mr);
}

int main(void) {
  test_no_assets_is_silent();
  test_whistle_seq_counter();
  if (iso_present()) {
    test_retail_banks();
  } else {
    fprintf(stderr, "SKIP ISO match-audio cases (no %s)\n", AUDIO_ISO_PATH);
  }
  puts("test_engine_audio OK");
  return 0;
}
