#include <stddef.h>
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_engine_internal.h"

/* Stable slot identity: the lifecycle cancels by function pointer, so every
 * run shares this one trampoline and the engine bridge keeps a single match
 * callback in the clock's tick table. */
static void fifa96_match_run_tick(void *user) {
  struct fifa96_match_run *mr = user;
  if (!mr) return;
  mr->ticks++;
}

static int fifa96_match_run_register(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr || !mr->engine) return -FIFA96_ERR_INVALID;
  int slot = fifa96_tick_register(&mr->engine->clock.ticks, fifa96_match_run_tick,
                                  mr, 1u); /* registered at 100 Hz (FU-64 §2.2) */
  return slot < 0 ? slot : 0;
}

static int fifa96_match_run_cancel(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr || !mr->engine) return -FIFA96_ERR_INVALID;
  return fifa96_tick_cancel(&mr->engine->clock.ticks, fifa96_match_run_tick);
}

static int fifa96_match_run_teardown(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr) return -FIFA96_ERR_INVALID;
  /* The foundation owns no match assets yet; clear the per-match counters.
   * Task 13 extends this to the match state block. */
  mr->ticks = 0;
  mr->steps = 0;
  return 0;
}

static int fifa96_match_run_post_exit(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr || !mr->engine) return -FIFA96_ERR_INVALID;
  mr->engine->mode = FIFA96_ENGINE_MODE_FRONTEND;
  return 0;
}

int fifa96_match_run_begin(struct fifa96_match_run *mr, struct fifa96_engine *eng,
                           uint32_t selector) {
  if (!mr || !eng) return -FIFA96_ERR_INVALID;
  if (mr->running) return -FIFA96_ERR_STATE;
  if (eng->match && eng->match != mr) return -FIFA96_ERR_STATE;
  if (!mr->backend.register_callback) {
    mr->backend.register_callback = fifa96_match_run_register;
    mr->backend.cancel_callback = fifa96_match_run_cancel;
    mr->backend.teardown = fifa96_match_run_teardown;
    mr->backend.post_exit = fifa96_match_run_post_exit;
    mr->backend.ctx = mr;
  }
  mr->engine = eng;
  mr->ticks = 0;
  mr->steps = 0;
  int rc = fifa96_match_lifecycle_begin(&mr->lc, &mr->backend, &mr->pace, selector);
  if (rc != 0) {
    mr->engine = NULL;
    return rc;
  }
  mr->running = 1;
  eng->match = mr;
  eng->mode = FIFA96_ENGINE_MODE_MATCH;
  return 0;
}

int fifa96_match_run_step(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->steps++;
  if (fifa96_match_lifecycle_should_exit(&mr->lc)) return fifa96_match_run_end(mr);
  return 0;
}

int fifa96_match_run_end(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  int rc = fifa96_match_lifecycle_end(&mr->lc, &mr->pace);
  struct fifa96_engine *eng = mr->engine;
  mr->running = 0;
  if (eng) {
    if (eng->match == mr) eng->match = NULL;
    if (eng->mode == FIFA96_ENGINE_MODE_MATCH) eng->mode = FIFA96_ENGINE_MODE_FRONTEND;
  }
  return rc;
}
