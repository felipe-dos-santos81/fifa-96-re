#pragma once
#include "fifa96_engine/fifa96_engine.h"

struct fifa96_engine {
  struct fifa96_engine_config cfg;
  fifa96_platform *plat;
  int booted;
  int quit;
  uint64_t frames;
  uint8_t solids[4][320 * 80];  /* temporary Task-1 surface; Task 3 replaces */
  uint8_t palette[768];
};
