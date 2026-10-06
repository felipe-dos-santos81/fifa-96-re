#pragma once
#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_cache.h"
#include "fifa96_engine/fifa96_clock.h"
#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_intro.h"
#include "fifa96_engine/fifa96_surface.h"

struct fifa96_engine {
  struct fifa96_engine_config cfg;
  fifa96_platform *plat;
  int booted;
  int quit;
  uint64_t frames;
  struct fifa96_surface *surface;
  struct fifa96_asset_table *assets;
  struct fifa96_cache *cache;   /* asset bytes backing a live intro */
  struct fifa96_intro intro;
  int intro_active;             /* intro stream loaded and still stepping */
  uint32_t intro_frames;        /* video frames advanced at 15 fps */
  struct fifa96_engine_clock clock;
  uint64_t last_ns;      /* now_ns() at the previous step; 0 before the first */
  uint32_t step_ticks;   /* PIT ticks fired by the most recent step */
};
