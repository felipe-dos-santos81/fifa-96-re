/* tests/test_engine_match_lifecycle.c — Task 12: match lifecycle wiring. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"

struct stub {
  struct fifa96_match_run *mr;
  int register_calls;
  int cancel_calls;
  int teardown_calls;
  int post_calls;
};

static int stub_register(void *ctx) {
  struct stub *s = ctx;
  assert(s->mr->lc.active == 1);
  assert(s->mr->lc.registered == 0);
  assert(s->mr->pace.hold == 0);
  s->register_calls++;
  return 0;
}

static int stub_cancel(void *ctx) {
  struct stub *s = ctx;
  assert(s->mr->lc.active == 0);
  assert(s->mr->pace.hold == 1);
  s->cancel_calls++;
  return 0;
}

static int stub_teardown(void *ctx) {
  struct stub *s = ctx;
  assert(s->mr->lc.registered == 0);
  assert(s->mr->lc.unload == 0);
  s->teardown_calls++;
  return 0;
}

static int stub_post_exit(void *ctx) {
  struct stub *s = ctx;
  assert(s->mr->lc.target == FIFA96_MATCH_LIFECYCLE_RESUME);
  s->post_calls++;
  return 0;
}

struct fixture {
  fifa96_platform *plat;
  struct fifa96_engine *engine;
};

static struct fixture make_fixture(void) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.step_ns = 10000000ull;   /* exactly one 100 Hz PIT tick per engine step */
  struct fixture f;
  f.plat = fifa96_platform_null_create(&pcfg);
  assert(f.plat != NULL);
  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  f.engine = fifa96_engine_create(&ecfg, f.plat);
  assert(f.engine != NULL);
  assert(fifa96_engine_boot(f.engine) == 0);
  return f;
}

static void drop_fixture(struct fixture f) {
  fifa96_engine_destroy(f.engine);
  fifa96_platform_destroy(f.plat);
}

/* A caller-supplied backend is used verbatim, so the test can count the
 * lifecycle callbacks while still beginning through the engine run path. */
static void test_stub_backend_lifecycle(void) {
  struct fixture f = make_fixture();
  struct fifa96_match_run mr;
  struct stub s;
  memset(&mr, 0, sizeof mr);
  memset(&s, 0, sizeof s);
  s.mr = &mr;
  mr.backend.register_callback = stub_register;
  mr.backend.cancel_callback = stub_cancel;
  mr.backend.teardown = stub_teardown;
  mr.backend.post_exit = stub_post_exit;
  mr.backend.ctx = &s;

  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.running == 1);
  assert(mr.lc.selector == 0);
  assert(mr.lc.active == 1);
  assert(mr.lc.registered == 1);
  assert(s.register_calls == 1);
  assert(s.cancel_calls == 0);
  assert(s.teardown_calls == 0);
  assert(s.post_calls == 0);

  /* Steps while the match is live do not tear anything down. */
  assert(fifa96_match_run_step(&mr) == 0);
  assert(mr.steps == 1);
  assert(s.teardown_calls == 0);

  /* Exit staging (FU-64 leave): the next step performs the teardown. */
  assert(fifa96_match_lifecycle_request_exit(&mr.lc) == 0);
  assert(fifa96_match_lifecycle_should_exit(&mr.lc) == 1);
  assert(fifa96_match_run_step(&mr) == 1);
  assert(s.register_calls == 1);
  assert(s.cancel_calls == 1);
  assert(s.teardown_calls == 1);
  assert(s.post_calls == 1);
  assert(mr.running == 0);
  assert(mr.lc.active == 0);
  assert(mr.lc.registered == 0);
  assert(mr.pace.hold == 1);
  assert(mr.lc.target == FIFA96_MATCH_LIFECYCLE_RESUME);

  /* End without a live match is a state error. */
  assert(fifa96_match_run_end(&mr) == -FIFA96_ERR_STATE);

  /* The run is reusable: re-begin after end returns OK. */
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.running == 1);
  assert(s.register_calls == 2);

  /* Explicit end without the exit staging: teardown runs, no post-exit. */
  assert(fifa96_match_run_end(&mr) == 0);
  assert(s.cancel_calls == 2);
  assert(s.teardown_calls == 2);
  assert(s.post_calls == 1);
  assert(mr.running == 0);

  drop_fixture(f);
}

/* Zeroed backend means the engine installs its own callbacks: the 100 Hz
 * match tick lands on the engine clock, MATCH-mode dispatch steps the run,
 * and teardown cancels the slot and returns to the front-end. */
static void test_engine_backend_path(void) {
  struct fixture f = make_fixture();
  struct fifa96_match_run mr;
  memset(&mr, 0, sizeof mr);

  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.running == 1);
  assert(mr.ticks == 0);
  assert(mr.steps == 0);

  /* One engine step = one 10 ms PIT tick: the registered slot fires and the
   * MATCH-mode dispatch advances the run once. */
  assert(fifa96_engine_step(f.engine) == 0);
  assert(mr.ticks == 1);
  assert(mr.steps == 1);
  assert(fifa96_engine_step(f.engine) == 0);
  assert(mr.ticks == 2);
  assert(mr.steps == 2);

  /* The exit step ends through the engine backend: cancel, teardown (match
   * state cleared) and post-exit. */
  assert(fifa96_match_lifecycle_request_exit(&mr.lc) == 0);
  assert(fifa96_engine_step(f.engine) == 0);
  assert(mr.running == 0);
  assert(mr.lc.active == 0);
  assert(mr.lc.registered == 0);
  assert(mr.lc.target == FIFA96_MATCH_LIFECYCLE_RESUME);
  assert(mr.ticks == 0);
  assert(mr.steps == 0);

  /* FRONTEND again and the tick slot is cancelled: later steps do not touch
   * the run. */
  assert(fifa96_engine_step(f.engine) == 0);
  assert(mr.ticks == 0);
  assert(mr.steps == 0);

  /* Re-begin with the engine backend after end. */
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(fifa96_engine_step(f.engine) == 0);
  assert(mr.ticks == 1);
  assert(mr.steps == 1);
  assert(fifa96_match_run_end(&mr) == 0);
  assert(mr.running == 0);

  drop_fixture(f);
}

static void test_begin_state_and_null_guards(void) {
  struct fixture f = make_fixture();
  struct fifa96_match_run mr = {0};
  struct fifa96_match_run other = {0};

  assert(fifa96_match_run_begin(NULL, f.engine, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_begin(&mr, NULL, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_step(NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_end(NULL) == -FIFA96_ERR_INVALID);

  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  /* One live match per engine, and no double begin on a live run. */
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == -FIFA96_ERR_STATE);
  assert(fifa96_match_run_begin(&other, f.engine, 0) == -FIFA96_ERR_STATE);
  assert(mr.running == 1);
  assert(other.running == 0);
  assert(fifa96_match_run_end(&mr) == 0);

  drop_fixture(f);
}

int main(void) {
  test_stub_backend_lifecycle();
  test_engine_backend_path();
  test_begin_state_and_null_guards();
  puts("test_engine_match_lifecycle OK");
  return 0;
}
