#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_lang.h"
#include "fifa96_engine/fifa96_match_bridge.h"
#include "fifa96_engine/fifa96_menu_art.h"

static int engine_char_fold(int c) {
  return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}

static int engine_ci_prefix(const char *s, const char *prefix) {
  while (*prefix) {
    if (engine_char_fold((unsigned char)*s) != engine_char_fold((unsigned char)*prefix)) return 0;
    s++;
    prefix++;
  }
  return 1;
}

static int engine_ci_suffix(const char *s, const char *suffix) {
  size_t sn = strlen(s), fn = strlen(suffix);
  if (fn > sn) return 0;
  s += sn - fn;
  while (*suffix) {
    if (engine_char_fold((unsigned char)*s) != engine_char_fold((unsigned char)*suffix)) return 0;
    s++;
    suffix++;
  }
  return 1;
}

static int engine_path_eq(const char *path, const char *want) {
  while (*path == '/') path++;
  while (*path && *want) {
    if (engine_char_fold((unsigned char)*path) != engine_char_fold((unsigned char)*want)) return 0;
    path++;
    want++;
  }
  return *path == '\0' && *want == '\0';
}

/* M1 heuristic: prefer VIDEO/VID_INTR.TGV (verified present in the ISO) as the
 * boot intro, else the first VIDEO/<name>.TGV in asset-table walk order. The
 * real intro comes from the load-order table (still underived), so the
 * explicit-name preference is the recorded approximation. Open leg: intro
 * path selection vs the load-order table. */
static const char *engine_find_intro_path(const struct fifa96_asset_table *t) {
  if (!t) return NULL;
  const char *fallback = NULL;
  for (size_t i = 0; i < t->count; i++) {
    const char *p = t->entries[i].path;
    while (*p == '/') p++;
    if (!engine_ci_prefix(p, "VIDEO/") || !engine_ci_suffix(p, ".TGV")) continue;
    if (engine_path_eq(t->entries[i].path, "VIDEO/VID_INTR.TGV")) return t->entries[i].path;
    if (!fallback) fallback = t->entries[i].path;
  }
  return fallback;
}

/* Front-end-mapped presses; any of them skips the intro (M1 uses CONFIRM).
 * QUIT is handled separately: it exits the engine. */
static int engine_key_mapped(int32_t raw_code) {
  switch (raw_code) {
    case FIFA96_ENGINE_KEY_UP:
    case FIFA96_ENGINE_KEY_DOWN:
    case FIFA96_ENGINE_KEY_LEFT:
    case FIFA96_ENGINE_KEY_RIGHT:
    case FIFA96_ENGINE_KEY_CONFIRM:
    case FIFA96_ENGINE_KEY_DECLINE:
      return 1;
    default:
      return 0;
  }
}

/* FU-343 (P4): a batch-level press test. The SDL backend re-presents a held key
 * with state 1 on every poll, so callers that need an edge track the previous
 * batch themselves (the match ESC latch does). */
static int engine_keys_down(const fifa96_platform_key *keys, int count,
                            int32_t raw_code) {
  for (int i = 0; i < count; i++)
    if (keys[i].state == 1 && keys[i].raw_code == raw_code) return 1;
  return 0;
}

/* P8: the raw held state of the two dual-role pad keys, sampled on every
 * poll. The MATCH dispatch compares it with `match_pad_down` to arm only a
 * key whose press began in live play, so a held menu/pause key cannot double
 * as a kick/pass while a fresh hold still reaches `fifa96_match_run_input`. */
static uint8_t engine_pad_down_bits(const fifa96_platform_key *keys, int count) {
  uint8_t bits = 0;
  for (int i = 0; i < count; i++) {
    if (keys[i].state != 1) continue;
    if (keys[i].raw_code == FIFA96_ENGINE_KEY_CONFIRM) bits |= 1u;
    else if (keys[i].raw_code == FIFA96_ENGINE_KEY_DECLINE) bits |= 2u;
  }
  return bits;
}

/* P15: the pause-substitution menu key level. The SDL backend re-presents a
 * held key, so the caller compares this with `match_menu_held` to get a
 * one-shot press. */
static uint8_t engine_menu_down_bits(const fifa96_platform_key *keys, int count) {
  uint8_t bits = 0;
  for (int i = 0; i < count; i++) {
    if (keys[i].state != 1) continue;
    if (keys[i].raw_code == FIFA96_ENGINE_KEY_CONFIRM) bits |= 1u;
    else if (keys[i].raw_code == FIFA96_ENGINE_KEY_DECLINE) bits |= 2u;
    else if (keys[i].raw_code == FIFA96_ENGINE_KEY_SUBST) bits |= 4u;
    else if (keys[i].raw_code == FIFA96_ENGINE_KEY_UP) bits |= 8u;
    else if (keys[i].raw_code == FIFA96_ENGINE_KEY_DOWN) bits |= 16u;
  }
  return bits;
}

/* P6: the live audio gain is the Options SFX Volume (0x11, 0..100) gated by
 * Sound Effects (0x01): settings.value[1] ? value[0x11] : 0. The default
 * 91 scales the crowd base 0x60 to 0x57; Music/Speech Volume have no
 * consumer (P6-L2). */
int32_t fifa96_engine_audio_percent(const struct fifa96_engine *e) {
  if (e == NULL) return 0;
  int32_t percent = e->frontend.menu.settings.value[0x01] != 0
                        ? e->frontend.menu.settings.value[0x11]
                        : 0;
  if (percent < 0) percent = 0;
  if (percent > (int32_t)FIFA96_MATCH_AUDIO_PERCENT_UNITY)
    percent = (int32_t)FIFA96_MATCH_AUDIO_PERCENT_UNITY;
  return percent;
}

/* P3: start match audio on the first MATCH step with a live run. The banks are
 * borrowed from the cache (created here when boot had no intro asset), and a
 * missing ISO/bank is a soft failure: the match stays silent and playable.
 * Headless runs never reach this helper (the engine step gates them out), so
 * the null tapes keep submitting no PCM. */
static int engine_match_audio_begin(struct fifa96_engine *e) {
  if (!e || !e->match || e->audio.active) return 0;
  if (!e->assets) return -1;
  if (!e->cache) e->cache = fifa96_cache_create(e->assets);
  if (!e->cache) return -1;
  size_t sfx_len = 0, crowd_len = 0;
  const uint8_t *sfx = fifa96_cache_get(e->cache, "/SOUND/SFX_GAME.BNK", &sfx_len);
  const uint8_t *crowd = fifa96_cache_get(e->cache, "/SOUND/CHN_GEN1.BNK", &crowd_len);
  int32_t percent = fifa96_engine_audio_percent(e);
  return fifa96_match_audio_start(&e->audio, e->match, sfx, sfx_len,
                                  crowd, crowd_len, (uint32_t)percent);
}

struct fifa96_engine *fifa96_engine_create(const struct fifa96_engine_config *cfg,
                                           fifa96_platform *plat) {
  if (!cfg || !plat) return NULL;
  struct fifa96_engine *e = calloc(1, sizeof *e);
  if (!e) return NULL;
  e->cfg = *cfg;
  e->plat = plat;
  if (e->cfg.width == 0) e->cfg.width = 320;
  if (e->cfg.height == 0) e->cfg.height = 240;
  fifa96_clock_init(&e->clock);
  fifa96_match_run_init(&e->match_run);
  return e;
}

int fifa96_engine_boot(struct fifa96_engine *e) {
  if (!e || e->booted) return -1;
  if (e->cfg.iso_path) {
    if (fifa96_asset_mount_file(e->cfg.iso_path, &e->assets) != FIFA96_OK) return -1;
  } else {
    e->assets = calloc(1, sizeof *e->assets);
    if (!e->assets) return -1;
  }
  if (e->plat->init(e->plat->self, e->cfg.width, e->cfg.height, "FIFA 96") != 0) {
    fifa96_asset_unmount(e->assets);
    e->assets = NULL;
    return -1;
  }
  e->surface = fifa96_surface_create(e->cfg.width, e->cfg.height);
  if (!e->surface) {
    e->plat->shutdown(e->plat->self);
    fifa96_asset_unmount(e->assets);
    e->assets = NULL;
    return -1;
  }
  fifa96_surface_clear(e->surface, 0x00);
  const char *intro_path = engine_find_intro_path(e->assets);
  if (intro_path) {
    e->cache = fifa96_cache_create(e->assets);
    if (e->cache) {
      size_t intro_len = 0;
      const uint8_t *intro_bytes = fifa96_cache_get(e->cache, intro_path, &intro_len);
      if (intro_bytes && intro_len != 0 &&
          fifa96_intro_start(&e->intro, e->surface) == 0 &&
          fifa96_intro_feed(&e->intro, intro_bytes, intro_len) == 0) {
        e->intro_active = 1;
      } else {
        /* unreadable/invalid VIDEO asset: no intro, boot still succeeds. */
        fifa96_cache_destroy(e->cache);
        e->cache = NULL;
      }
    }
  }
  if (fifa96_frontend_run_init(&e->frontend, e->surface) != 0) {
    e->plat->shutdown(e->plat->self);
    fifa96_cache_destroy(e->cache);
    fifa96_asset_unmount(e->assets);
    e->cache = NULL;
    e->assets = NULL;
    return -1;
  }
  /* P13: derived saves live beside the emulated install (data/ is ignored);
   * headless/test boots never reach the interactive hub, so no file is
   * written unless a native-menu player asks or a result auto-saves. */
  if (e->cfg.iso_path) {
    (void)mkdir("data/game/hdd", 0777);
    fifa96_frontend_run_set_save_dir(&e->frontend, "data/game/hdd");
  }
  fifa96_menu_art_init(e->assets);
  /* P6: decode the PCCD team database into an engine-owned arena. Soft-fails
   * without the ISO (the native menu then keeps the 3-letter fallback codes and
   * zero formations); decoding is asset-only and cannot reorder boot draws. */
  (void)fifa96_team_data_init(&e->team_data, e->assets);
  e->frontend.team_data = &e->team_data;
  /* P2: the native Game Select/Team Select screen is the interactive front-end.
   * The deterministic headless M1/M2 tapes (which run with the ISO mounted)
   * keep the legacy FU-135 frames and the hash-pinned fallback, so the native
   * path is gated on the interactive (non-headless) engine here; the asset
   * decode itself still soft-fails to the legacy path. */
  if (!e->cfg.headless &&
      fifa96_lang_init(&e->frontend.lang, e->assets) == 0 &&
      fifa96_lang_ready(&e->frontend.lang) &&
      fifa96_lang_font(&e->frontend.lang) != NULL) {
    fifa96_menu_art_init_native(e->assets);
    fifa96_frontend_run_enable_native(&e->frontend, 1);
  }
  e->mode = e->intro_active ? FIFA96_ENGINE_MODE_INTRO : FIFA96_ENGINE_MODE_FRONTEND;
  e->plat->audio_open(e->plat->self, 22050u, 2);
  /* Clock baseline: in real-time mode discard everything that happened before
   * boot (platform init, asset mount, front-end init) so the first step does
   * not fire a burst of catch-up PIT ticks. Headless runs keep the legacy
   * last_ns = 0: their host clock (the null backend) is step-driven and
   * already counts the first step as one full step_ns, so baselining there
   * would shift the pinned one-tick-per-step cadence (test_engine_platform,
   * test_engine_m1, test_engine_match_frame). */
  if (!e->cfg.headless) e->last_ns = e->plat->now_ns(e->plat->self);
  e->booted = 1;
  return 0;
}

int fifa96_engine_step(struct fifa96_engine *e) {
  if (!e || !e->booted) return -1;
  /* Fixed-step clock first, then present: with the null backend (now_ns()
   * advances on present) every step contributes exactly one step_ns delta. */
  uint64_t now = e->plat->now_ns(e->plat->self);
  uint64_t delta = now - e->last_ns;
  /* FU-343 P5/A6-L7: bound the host catch-up. The 100 Hz tick hook runs a
   * full match frame body per tick, so replaying a long host stall (window
   * drag, debugger, starved X server, the WM's "not responding" dialog)
   * unbounded turns one engine step into thousands of frames and freezes the
   * window. Keep at most 10 ticks (100 ms) per step; the excess is dropped,
   * as a normal game drops frames. The null/headless tapes advance exactly
   * one 10 ms tick, so their transcripts are unchanged. */
  if (e->clock.tick_ns != 0) {
    uint64_t max_delta = e->clock.tick_ns * 10u;
    if (delta > max_delta) delta = max_delta;
  }
  e->step_ticks = (uint32_t)fifa96_clock_advance_ns(&e->clock, delta);
  e->last_ns = now;
  if (e->mode == FIFA96_ENGINE_MODE_INTRO) {
    /* Input-skip during the intro: QUIT exits, any other mapped press aborts
     * playback and enters the front-end immediately. The skip key is consumed
     * here and never queued for fifa96_frontend_run_input. */
    fifa96_platform_key keys[32];
    int n = 0;
    if (e->plat->poll(e->plat->self, keys, 32, &n) != 0) return -1;
    e->match_pad_down = engine_pad_down_bits(keys, n);
    for (int i = 0; i < n; i++) {
      if (keys[i].state != 1) continue;   /* press events only */
      if (keys[i].raw_code == FIFA96_ENGINE_KEY_QUIT) {
        e->quit = 1;
        e->mode = FIFA96_ENGINE_MODE_QUIT;
      } else if (engine_key_mapped(keys[i].raw_code)) {
        e->mode = FIFA96_ENGINE_MODE_FRONTEND;
      } else {
        continue;
      }
      e->intro_active = 0;
      fifa96_intro_abort(&e->intro);
      break;
    }
    /* Video cadence: 15 frames per 100 PIT ticks (FU-37); at most one intro
     * frame per engine step, so the tape stays deterministic. */
    if (e->mode == FIFA96_ENGINE_MODE_INTRO) {
      if (e->intro_active) {
        uint32_t due = fifa96_pacing_frames_due(e->clock.pit.ticks);
        if (due > e->intro_frames) {
          if (fifa96_intro_step(&e->intro, e->surface) != 0) {
            e->intro_active = 0;
          } else {
            e->intro_frames++;
            if (fifa96_intro_done(&e->intro)) e->intro_active = 0;
          }
        }
      }
      if (!e->intro_active) e->mode = FIFA96_ENGINE_MODE_FRONTEND;
    }
    /* F1: either the skip above or the natural end lands here. Disarm the
     * native latch so the held Return (or any key held across the handoff)
     * cannot be queued as a fresh press by the front-end poll. */
    if (e->mode == FIFA96_ENGINE_MODE_FRONTEND)
      fifa96_frontend_run_disarm_input(&e->frontend);
  } else if (e->mode == FIFA96_ENGINE_MODE_FRONTEND) {
    fifa96_platform_key keys[32];
    int n = 0;
    if (e->plat->poll(e->plat->self, keys, 32, &n) != 0) return -1;
    e->match_pad_down = engine_pad_down_bits(keys, n);
    fifa96_frontend_run_input(&e->frontend, keys, (size_t)n);
    int quit = 0;
    if (fifa96_frontend_run_step(&e->frontend, e->surface, &quit) != 0) return -1;
    if (quit) {
      e->quit = 1;
      e->mode = FIFA96_ENGINE_MODE_QUIT;
    } else if (e->frontend.match_start) {
      /* M2: the front-end's match-start exit classification hands off to the
       * engine-owned run; the bridge consumes the classification and starts
       * the run, which sets the engine mode MATCH. */
      if (fifa96_match_bridge_from_frontend(&e->match_run, e) != 0) return -1;
    }
  } else if (e->mode == FIFA96_ENGINE_MODE_MATCH) {
    /* Poll one input batch per step (the FRONTEND pattern) and feed it to the
     * run's FU-61/FU-70 input path. FU-343 (P4): on a live run ESC is the
     * pause/resume toggle (ACTIVE) or the FULL TIME skip (OVER/POST), never a
     * process quit; the headless/non-live path keeps ESC = quit so the M2 tape
     * and the direct-begin fixtures are unchanged. The SDL backend re-presents
     * held keys, so the toggle is edge-triggered (`match_esc_held`). The clock
     * advance above fired the run's 100 Hz tick hook once per PIT tick, driving
     * one frame-body pace tick per 10 ms; this dispatch consumes the lifecycle
     * state (exit staging). */
    fifa96_platform_key keys[32];
    int n = 0;
    if (e->plat->poll(e->plat->self, keys, 32, &n) != 0) return -1;
    const int esc_down = engine_keys_down(keys, n, FIFA96_ENGINE_KEY_QUIT);
    const int esc_edge = esc_down && !e->match_esc_held;
    e->match_esc_held = (uint8_t)(esc_down != 0);
    const uint8_t pad_down = engine_pad_down_bits(keys, n);
    const int pad_live = e->match && !e->match->paused &&
                         e->match->lc.screen == FIFA96_MATCH_SCREEN_ACTIVE;
    if (!pad_live) {
      e->match_pad_allowed = 0;   /* pause/result/menu handoff: disarm */
    } else {
      const uint8_t pad_rise =
          (uint8_t)(pad_down & (uint8_t)~e->match_pad_down);
      e->match_pad_allowed =
          (uint8_t)((e->match_pad_allowed | pad_rise) & pad_down);
    }
    e->match_pad_down = pad_down;
    const uint8_t menu_down = engine_menu_down_bits(keys, n);
    const uint8_t menu_rise = (uint8_t)(menu_down & (uint8_t)~e->match_menu_held);
    e->match_menu_held = menu_down;
    fifa96_platform_key match_keys[32];
    size_t match_count = 0;
    for (int i = 0; i < n; i++) {
      if (keys[i].raw_code == FIFA96_ENGINE_KEY_CONFIRM &&
          (e->match_pad_allowed & 1u) == 0)
        continue;
      if (keys[i].raw_code == FIFA96_ENGINE_KEY_DECLINE &&
          (e->match_pad_allowed & 2u) == 0)
        continue;
      match_keys[match_count++] = keys[i];
    }
    /* P14: the native half-time/goal kickoff gate is player-driven. Inject one
     * KICK press after a visible 3 s dwell for the actual half-time gate and
     * for a second-half goal restart (both sit at phase 1); a manual KICK/PASS
     * moves the phase first and clears this path. The half-time flag keeps a
     * goal restart from drawing the HALF TIME panel. Live-only. */
    {
      int kickoff_wait = e->match && e->match->live_start &&
                         e->match->lc.screen == FIFA96_MATCH_SCREEN_ACTIVE &&
                         (e->match->live_halftime_pending ||
                          e->match->state.phase == 1u ||
                          (e->match->state.phase == 0x0Cu &&
                           e->match->global_157ac2 < 2u));
      if (kickoff_wait) {
        if (e->match_halftime_steps < 600u) e->match_halftime_steps++;
        if (e->match_halftime_steps >= 300u && !e->match_halftime_pressed) {
          e->match_halftime_pressed = 1;
          e->match_halftime_hold = 10u;
        }
        if (e->match_halftime_hold != 0u &&
            match_count < sizeof match_keys / sizeof match_keys[0]) {
          fifa96_platform_key auto_key = {FIFA96_ENGINE_KEY_PASS, 1};
          match_keys[match_count++] = auto_key;
          e->match_halftime_hold--;
        }
      } else {
        e->match_halftime_steps = 0;
        e->match_halftime_pressed = 0;
        e->match_halftime_hold = 0;
      }
    }
    if (e->match)
      (void)fifa96_match_run_input(e->match, match_keys, match_count);
    if (e->match && e->match->live_start) {
      if (esc_edge) {
        if (e->match->sub_menu_open) {
          (void)fifa96_match_run_pause_sub(e->match, FIFA96_MATCH_RUN_SUB_BACK);
        } else if (e->match->lc.screen == FIFA96_MATCH_SCREEN_ACTIVE) {
          (void)fifa96_match_run_set_paused(e->match, e->match->paused ? 0 : 1);
        } else if (e->match->lc.screen == FIFA96_MATCH_SCREEN_OVER ||
                   e->match->lc.screen == FIFA96_MATCH_SCREEN_POST) {
          (void)fifa96_match_run_result_continue(e->match);
        }
        /* EXIT: the staged exit is consumed by run_step below. */
      }
      if (e->match->paused) {
        if (e->match->sub_menu_open) {
          if (menu_rise & 1u)
            (void)fifa96_match_run_pause_sub(e->match, FIFA96_MATCH_RUN_SUB_CONFIRM);
          else if (menu_rise & 2u)
            (void)fifa96_match_run_pause_sub(e->match, FIFA96_MATCH_RUN_SUB_BACK);
          else if (menu_rise & 8u)
            (void)fifa96_match_run_pause_sub(e->match, FIFA96_MATCH_RUN_SUB_UP);
          else if (menu_rise & 16u)
            (void)fifa96_match_run_pause_sub(e->match, FIFA96_MATCH_RUN_SUB_DOWN);
        } else if (menu_rise & 4u) {
          (void)fifa96_match_run_pause_sub(e->match, FIFA96_MATCH_RUN_SUB_OPEN);
        } else if (menu_rise & 1u) {
          (void)fifa96_match_run_set_paused(e->match, 0);
        } else if (menu_rise & 2u) {
          (void)fifa96_match_lifecycle_request_exit(&e->match->lc);
        }
      } else if ((e->match->lc.screen == FIFA96_MATCH_SCREEN_OVER ||
                  e->match->lc.screen == FIFA96_MATCH_SCREEN_POST) &&
                 (menu_rise & 1u)) {
        (void)fifa96_match_run_result_continue(e->match);
      }
    } else {
      for (int i = 0; i < n; i++) {
        if (keys[i].state == 1 && keys[i].raw_code == FIFA96_ENGINE_KEY_QUIT) {
          e->quit = 1;
          e->mode = FIFA96_ENGINE_MODE_QUIT;
          break;
        }
      }
    }
    if (e->mode == FIFA96_ENGINE_MODE_MATCH) {
      if (!e->match) {
        e->mode = FIFA96_ENGINE_MODE_FRONTEND;
        (void)fifa96_frontend_run_reset(&e->frontend, e->surface);
      } else {
        if (!e->cfg.headless && !e->audio.active)
          (void)engine_match_audio_begin(e);
        if (fifa96_match_run_step(e->match) < 0) return -1;
        /* FU-343 (P4): run_step may have ended the match (exit staging, the
         * POST dwell or the skip). Restore the front-end Game Select screen
         * before its first draw and stop the audio; the last match frame stays
         * presented for this step. */
        if (!e->match || e->mode != FIFA96_ENGINE_MODE_MATCH) {
          /* P13: preserve the completed result before frontend_run_reset
           * (the run teardown already zeroed score[]; final_score is the
           * pre-teardown copy). A pause/abandon exit leaves the flag clear. */
          if (e->frontend.mode >= FIFA96_COMPETITION_LEAGUE &&
              e->frontend.mode <= FIFA96_COMPETITION_PRACTISE &&
              e->match_run.final_score_valid)
            (void)fifa96_frontend_run_match_result(
                &e->frontend, e->match_run.final_score[0],
                e->match_run.final_score[1]);
          (void)fifa96_frontend_run_reset(&e->frontend, e->surface);
        } else {
          /* P3: the run step above has applied this frame's whistle writer, so
           * the audio cue edge is sampled after it (the clock already advanced
           * the 100 Hz pump before this dispatch). */
          if (!e->cfg.headless && e->audio.active)
            (void)fifa96_match_audio_step(&e->audio, e->match, e->plat,
                                          e->step_ticks);
          /* One presentation pass per presented frame: recompose the indexed
           * match canvas before the surface->planes conversion below. The run
           * no-ops while rendering is disabled (the Task 12-14 fixtures). */
          if (fifa96_match_run_render(e->match, e->surface) != 0)
            return -1;
        }
      }
    }
  }
  /* P3: stop on match end (fifa96_match_run_end clears e->match) or a QUIT
   * transition. The bank bytes stay in the cache until engine destroy. */
  if (e->audio.active &&
      (e->mode != FIFA96_ENGINE_MODE_MATCH || !e->match))
    fifa96_match_audio_stop(&e->audio);
  fifa96_platform_frame f;
  fifa96_surface_plane(e->surface, &f);
  if (e->plat->present(e->plat->self, &f) != 0) return -1;
  e->frames++;
  return 0;
}

int fifa96_engine_run(struct fifa96_engine *e) {
  if (!e) return 0;
  /* Sleep to the engine's fixed-step deadline so a real backend does not
   * busy-spin: each iteration adds one clock tick (10 ms) to the absolute
   * deadline and yields the remaining time via the backend. A no-op
   * sleep_ns (null backend) keeps headless runs deterministic. */
  uint64_t tick_ns = e->clock.tick_ns;
  uint64_t deadline = e->plat->now_ns(e->plat->self);
  while (!e->quit) {
    if (fifa96_engine_step(e) != 0) return -1;
    if (tick_ns == 0) continue;
    deadline += tick_ns;
    uint64_t now = e->plat->now_ns(e->plat->self);
    if (deadline > now) e->plat->sleep_ns(e->plat->self, deadline - now);
  }
  return 0;
}

int fifa96_engine_should_quit(const struct fifa96_engine *e) {
  if (!e) return 1;
  return e->quit != 0 || e->mode == FIFA96_ENGINE_MODE_QUIT;
}
void fifa96_engine_destroy(struct fifa96_engine *e) {
  if (!e) return;
  /* Ownership contract in fifa96_match_run.h: a run alive at destroy must
   * outlive this call; end it before the engine state is freed. Stop the
   * audio first: its bank descriptors borrow the cache bytes. */
  fifa96_match_audio_stop(&e->audio);
  if (e->match) fifa96_match_run_end(e->match);
  fifa96_team_data_shutdown(&e->team_data);
  if (e->booted) e->plat->shutdown(e->plat->self);
  fifa96_lang_shutdown(&e->frontend.lang);
  fifa96_surface_destroy(e->surface);
  fifa96_cache_destroy(e->cache);
  fifa96_asset_unmount(e->assets);
  free(e);
}

/* Defined once here; each backend sets its own `destroy` slot. */
void fifa96_platform_destroy(fifa96_platform *p) {
  if (!p) return;
  if (p->destroy) p->destroy(p->self);
  free(p);
}
