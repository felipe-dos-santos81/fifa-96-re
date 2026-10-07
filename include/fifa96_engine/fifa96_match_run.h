#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_match_lifecycle.h"
#include "fifa96_loader/fifa96_match_pace.h"

/* Engine-level match driver (M2 foundation): owns the FU-64 lifecycle and the
 * FU-60 pace and binds them to an engine. `backend` doubles as the injection
 * seam: `begin` installs the engine-backed callbacks when the field is zeroed
 * and otherwise uses the caller-supplied backend verbatim (headless tests
 * count the lifecycle callbacks through a stub). */
struct fifa96_engine;

struct fifa96_match_run {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct fifa96_engine *engine;                  /* engine holding this run */
  struct fifa96_match_lifecycle_backend backend; /* engine callbacks or stub */
  uint32_t ticks;                                /* 100 Hz match callback hits */
  uint32_t steps;                                /* run steps since begin */
  int running;                                   /* begin/end balance */
};

/* Wire the run to the engine (MATCH mode) and start the lifecycle. Returns 0,
 * a -fifa96_err_t, or the lifecycle's register failure. */
int fifa96_match_run_begin(struct fifa96_match_run *mr, struct fifa96_engine *eng,
                           uint32_t selector);

/* One engine step of the run. Drives the lifecycle: 0 while live, the end
 * result (1 = post-exit) on the exit step, or a -fifa96_err_t. */
int fifa96_match_run_step(struct fifa96_match_run *mr);

/* Tear the match down through the lifecycle and clear the engine linkage.
 * 0 = plain teardown, 1 = post-exit ran, or a -fifa96_err_t. */
int fifa96_match_run_end(struct fifa96_match_run *mr);
