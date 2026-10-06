#pragma once
#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_surface.h"

struct fifa96_engine {
  struct fifa96_engine_config cfg;
  fifa96_platform *plat;
  int booted;
  int quit;
  uint64_t frames;
  struct fifa96_surface *surface;
};
