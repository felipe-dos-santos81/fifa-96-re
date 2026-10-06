#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_engine_internal.h"

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

/* First VIDEO/<name>.TGV in the asset table walk order (case-insensitive).
 * The real load-order table has not been derived yet, so this bounded scan
 * is the M1 approximation. Open leg: intro path selection. */
static const char *engine_find_intro_path(const struct fifa96_asset_table *t) {
  if (!t) return NULL;
  for (size_t i = 0; i < t->count; i++) {
    const char *p = t->entries[i].path;
    while (*p == '/') p++;
    if (engine_ci_prefix(p, "VIDEO/") && engine_ci_suffix(p, ".TGV")) return t->entries[i].path;
  }
  return NULL;
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
  /* Video cadence: 15 frames per 100 PIT ticks (FU-37); at most one intro
   * frame per engine step, so the tape stays deterministic. */
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
  fifa96_platform_frame f;
  fifa96_surface_plane(e->surface, &f);
  if (e->plat->present(e->plat->self, &f) != 0) return -1;
  e->frames++;
  return 0;
}

int fifa96_engine_run(struct fifa96_engine *e) {
  while (e && !e->quit) {
    if (fifa96_engine_step(e) != 0) return -1;
    fifa96_platform_key keys[32];
    int n = 0;
    e->plat->poll(e->plat->self, keys, 32, &n);
  }
  return 0;
}

int fifa96_engine_should_quit(const struct fifa96_engine *e) { return e ? e->quit : 1; }
void fifa96_engine_destroy(struct fifa96_engine *e) {
  if (!e) return;
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
