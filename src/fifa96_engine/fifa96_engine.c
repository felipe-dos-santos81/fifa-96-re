#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_engine_internal.h"
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
  fifa96_menu_art_init(e->assets);
  e->mode = e->intro_active ? FIFA96_ENGINE_MODE_INTRO : FIFA96_ENGINE_MODE_FRONTEND;
  e->plat->audio_open(e->plat->self, 22050u, 2);
  e->booted = 1;
  return 0;
}

int fifa96_engine_step(struct fifa96_engine *e) {
  if (!e || !e->booted) return -1;
  /* Fixed-step clock first, then present: with the null backend (now_ns()
   * advances on present) every step contributes exactly one step_ns delta. */
  uint64_t now = e->plat->now_ns(e->plat->self);
  e->step_ticks = (uint32_t)fifa96_clock_advance_ns(&e->clock, now - e->last_ns);
  e->last_ns = now;
  if (e->mode == FIFA96_ENGINE_MODE_INTRO) {
    /* Input-skip during the intro: QUIT exits, any other mapped press aborts
     * playback and enters the front-end immediately. The skip key is consumed
     * here and never queued for fifa96_frontend_run_input. */
    fifa96_platform_key keys[32];
    int n = 0;
    if (e->plat->poll(e->plat->self, keys, 32, &n) != 0) return -1;
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
  } else if (e->mode == FIFA96_ENGINE_MODE_FRONTEND) {
    fifa96_platform_key keys[32];
    int n = 0;
    if (e->plat->poll(e->plat->self, keys, 32, &n) != 0) return -1;
    fifa96_frontend_run_input(&e->frontend, keys, (size_t)n);
    int quit = 0;
    if (fifa96_frontend_run_step(&e->frontend, e->surface, &quit) != 0) return -1;
    if (quit) {
      e->quit = 1;
      e->mode = FIFA96_ENGINE_MODE_QUIT;
    }
  } else if (e->mode == FIFA96_ENGINE_MODE_MATCH) {
    /* Poll one input batch per step (the FRONTEND pattern) and feed it to the
     * run's FU-61/FU-70 input path; QUIT leaves the engine and is never a
     * match key. The clock advance above fired the run's 100 Hz tick hook once
     * per PIT tick, driving one frame-body pace tick per 10 ms; this dispatch
     * consumes the lifecycle state (exit staging). */
    fifa96_platform_key keys[32];
    int n = 0;
    if (e->plat->poll(e->plat->self, keys, 32, &n) != 0) return -1;
    if (e->match) (void)fifa96_match_run_input(e->match, keys, (size_t)n);
    for (int i = 0; i < n; i++) {
      if (keys[i].state == 1 && keys[i].raw_code == FIFA96_ENGINE_KEY_QUIT) {
        e->quit = 1;
        e->mode = FIFA96_ENGINE_MODE_QUIT;
        break;
      }
    }
    if (e->mode == FIFA96_ENGINE_MODE_MATCH) {
      if (!e->match) {
        e->mode = FIFA96_ENGINE_MODE_FRONTEND;
      } else {
        if (fifa96_match_run_step(e->match) < 0) return -1;
        /* One presentation pass per presented frame: recompose the indexed
         * match canvas before the surface->planes conversion below. The run
         * no-ops while rendering is disabled (the Task 12-14 fixtures). */
        if (e->mode == FIFA96_ENGINE_MODE_MATCH &&
            fifa96_match_run_render(e->match, e->surface) != 0)
          return -1;
      }
    }
  }
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
   * outlive this call; end it before the engine state is freed. */
  if (e->match) fifa96_match_run_end(e->match);
  if (e->booted) e->plat->shutdown(e->plat->self);
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
