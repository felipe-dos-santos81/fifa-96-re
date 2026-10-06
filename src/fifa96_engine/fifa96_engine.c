#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_engine_internal.h"

struct fifa96_engine *fifa96_engine_create(const struct fifa96_engine_config *cfg,
                                           fifa96_platform *plat) {
  if (!cfg || !plat) return NULL;
  struct fifa96_engine *e = calloc(1, sizeof *e);
  if (!e) return NULL;
  e->cfg = *cfg;
  e->plat = plat;
  if (e->cfg.width == 0) e->cfg.width = 320;
  if (e->cfg.height == 0) e->cfg.height = 240;
  return e;
}

int fifa96_engine_boot(struct fifa96_engine *e) {
  if (!e || e->booted) return -1;
  if (e->plat->init(e->plat->self, e->cfg.width, e->cfg.height, "FIFA 96") != 0) return -1;
  e->plat->audio_open(e->plat->self, 22050u, 2);
  memset(e->palette, 0, sizeof e->palette);
  e->booted = 1;
  return 0;
}

int fifa96_engine_step(struct fifa96_engine *e) {
  if (!e || !e->booted) return -1;
  fifa96_platform_frame f;
  for (int p = 0; p < 4; p++) f.planes[p] = e->solids[p];
  f.stride = 80u;
  memcpy((void *)f.palette, e->palette, 768);
  f.flags = 0;
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
  free(e);
}

/* Defined once here; each backend sets its own `destroy` slot. */
void fifa96_platform_destroy(fifa96_platform *p) {
  if (!p) return;
  if (p->destroy) p->destroy(p->self);
  free(p);
}
