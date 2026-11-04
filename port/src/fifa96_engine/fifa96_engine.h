#pragma once
#include "fifa96_engine/fifa96_platform.h"

struct fifa96_engine_config {
  const char *iso_path;   /* NULL = no-assets smoke mode */
  int width;
  int height;
  int headless;
};

struct fifa96_engine;

struct fifa96_engine *fifa96_engine_create(const struct fifa96_engine_config *cfg,
                                           fifa96_platform *plat);
int  fifa96_engine_boot(struct fifa96_engine *e);
int  fifa96_engine_step(struct fifa96_engine *e);
int  fifa96_engine_run(struct fifa96_engine *e);
int  fifa96_engine_should_quit(const struct fifa96_engine *e);
void fifa96_engine_destroy(struct fifa96_engine *e);
