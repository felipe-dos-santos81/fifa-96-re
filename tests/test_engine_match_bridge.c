/* tests/test_engine_match_bridge.c — M2 C1: front-end -> match bridge.
 *
 * The bridge starts the match run from the front-end's match-start exit
 * classification and returns the engine in MATCH mode. The startable
 * classification is the port's code-8 exit state (FU-66 §4 tail at 0x1EDFD,
 * `MOV EAX,0x10; CALL 0x1442C`): the panel's accepted confirm (FU-66 §5,
 * `0x1F994` -> r=5, [0x5094]=1) drives the FU-66 §2 driver exit, and the
 * front-end exit classifier resolves that transition to
 * FIFA96_FRONTEND_EXIT_STATE16 — the same call tests/test_frontend.c's
 * test_exit_classification pins for code 8, and the classification whose
 * menu-driven start-match family enters the setup with selector 0
 * (FU-64 §1.1, `XOR EAX,EAX` at 0x180A8).
 *
 * The event sequence below is tests/test_frontend.c's: code 8 opens the panel
 * (frontend_result), the gated -10 confirm (panel_event) accepts the menu
 * selection, and the driver leaves to EXIT. A front-end (not panel) CONFIRM
 * is deliberately NOT startable: the M1 wrapping of that confirm is pinned by
 * tests/golden/engine/m1-frames.txt.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_match_bridge.h"
#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"

/* The engine's front-end wrapper and owned match run are engine-internal
 * state; this test is the bridge/wiring contract test, so it reads them. */
#include "fifa96_engine/fifa96_engine_internal.h"

struct fixture {
  fifa96_platform *plat;
  struct fifa96_engine *engine;
};

static struct fixture make_fixture(const fifa96_platform_key *tape,
                                   size_t tape_len) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.tape = tape;
  pcfg.tape_len = tape_len;
  pcfg.step_ns = 10000000ull;   /* one 100 Hz PIT tick per engine step */
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

static void press(struct fifa96_engine *e, int32_t code) {
  fifa96_platform_key k = {.raw_code = code, .state = 1};
  assert(fifa96_frontend_run_input(&e->frontend, &k, 1) == 0);
}

static void step(struct fifa96_engine *e) {
  int quit = 0;
  assert(fifa96_frontend_run_step(&e->frontend, e->surface, &quit) == 0);
  assert(quit == 0);
}

/* tests/test_frontend.c's sequence to the startable classification: decline
 * leaves the front-end loop for the panel (code 8 -> PANEL), the confirm gate
 * accepts the selection (-10 -> PANEL_CONFIRM), and the exit classification
 * is the code-8 STATE16. */
static void drive_to_match_start(struct fifa96_engine *e) {
  press(e, FIFA96_ENGINE_KEY_DECLINE);
  step(e);
  assert(e->frontend.phase == FIFA96_FRONTEND_PHASE_PANEL);

  press(e, FIFA96_ENGINE_KEY_CONFIRM);
  step(e);
  assert(e->frontend.frontend.confirm == 1);
  assert(e->frontend.match_start == 1);
  assert(e->frontend.phase == FIFA96_FRONTEND_PHASE_EXIT);
}

/* A fresh front-end is not in the startable state: the bridge refuses and
 * mutates nothing. */
static void test_fresh_frontend_not_startable(void) {
  struct fixture f = make_fixture(NULL, 0);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  struct fifa96_frontend_run before = f.engine->frontend;

  assert(f.engine->frontend.match_start == 0);
  assert(fifa96_match_bridge_from_frontend(&mr, f.engine) == -FIFA96_ERR_STATE);
  assert(mr.running == 0);
  assert(mr.engine == NULL);
  assert(f.engine->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(f.engine->match == NULL);
  assert(memcmp(&before, &f.engine->frontend, sizeof before) == 0);

  assert(fifa96_match_bridge_from_frontend(NULL, f.engine) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_bridge_from_frontend(&mr, NULL) == -FIFA96_ERR_INVALID);

  drop_fixture(f);
}

/* The front-end (non-panel) accepted confirm is deliberately not startable:
 * its M1 unwind is pinned by tests/golden/engine/m1-frames.txt, so the
 * FU-66 front-end confirm route stays an open leg (see the wrapper). */
static void test_frontend_confirm_is_not_startable(void) {
  struct fixture f = make_fixture(NULL, 0);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);

  press(f.engine, FIFA96_ENGINE_KEY_CONFIRM);
  step(f.engine);
  assert(f.engine->frontend.frontend.confirm == 1);
  assert(f.engine->frontend.phase == FIFA96_FRONTEND_PHASE_EXIT);
  assert(f.engine->frontend.match_start == 0);
  assert(fifa96_match_bridge_from_frontend(&mr, f.engine) == -FIFA96_ERR_STATE);
  assert(mr.running == 0);

  drop_fixture(f);
}

/* The startable classification begins the run, sets MATCH, refuses a second
 * begin, and the end returns the engine to FRONTEND; the sequence can be
 * replayed to begin again. */
static void test_bridge_starts_and_rebegins(void) {
  struct fixture f = make_fixture(NULL, 0);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);

  drive_to_match_start(f.engine);
  assert(fifa96_match_bridge_from_frontend(&mr, f.engine) == 0);
  assert(mr.running == 1);
  assert(mr.engine == f.engine);
  assert(mr.lc.selector == 0);   /* FU-64 §1.1 menu-path selector */
  assert(f.engine->mode == FIFA96_ENGINE_MODE_MATCH);
  assert(f.engine->match == &mr);
  assert(f.engine->frontend.match_start == 0);   /* classification consumed */

  assert(fifa96_match_bridge_from_frontend(&mr, f.engine) == -FIFA96_ERR_STATE);
  assert(mr.running == 1);

  assert(fifa96_match_run_end(&mr) == 0);
  assert(mr.running == 0);
  assert(f.engine->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(f.engine->match == NULL);

  /* Re-begin: the same classification sequence works again. */
  drive_to_match_start(f.engine);
  assert(fifa96_match_bridge_from_frontend(&mr, f.engine) == 0);
  assert(mr.running == 1);
  assert(f.engine->mode == FIFA96_ENGINE_MODE_MATCH);
  assert(fifa96_match_run_end(&mr) == 0);

  drop_fixture(f);
}

/* Engine wiring: the step that accepts the panel confirm calls the bridge on
 * the engine-owned run and switches the engine to MATCH. */
static void test_engine_step_wires_bridge(void) {
  const fifa96_platform_key tape[] = {
      {FIFA96_ENGINE_KEY_DECLINE, 1}, {FIFA96_ENGINE_KEY_DECLINE, 0},
      {FIFA96_ENGINE_KEY_CONFIRM, 1}, {FIFA96_ENGINE_KEY_CONFIRM, 0},
  };
  struct fixture f = make_fixture(tape, sizeof tape / sizeof tape[0]);

  assert(f.engine->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(f.engine->match == NULL);

  assert(fifa96_engine_step(f.engine) == 0);   /* DECLINE -> panel */
  assert(f.engine->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(f.engine->frontend.phase == FIFA96_FRONTEND_PHASE_PANEL);
  assert(fifa96_engine_step(f.engine) == 0);   /* release */
  assert(fifa96_engine_step(f.engine) == 0);   /* CONFIRM -> bridge */
  assert(f.engine->mode == FIFA96_ENGINE_MODE_MATCH);
  assert(f.engine->match == &f.engine->match_run);
  assert(f.engine->match_run.running == 1);
  assert(f.engine->frontend.match_start == 0);
  assert(fifa96_engine_step(f.engine) == 0);   /* release, match input */

  drop_fixture(f);
}

int main(void) {
  test_fresh_frontend_not_startable();
  test_frontend_confirm_is_not_startable();
  test_bridge_starts_and_rebegins();
  test_engine_step_wires_bridge();
  puts("test_engine_match_bridge OK");
  return 0;
}
