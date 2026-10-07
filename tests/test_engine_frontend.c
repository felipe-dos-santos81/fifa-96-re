/* tests/test_engine_frontend.c */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_frontend_run.h"
#include "fifa96_engine/fifa96_platform_null.h"

static void press(struct fifa96_frontend_run *fr, int32_t code, int state) {
  fifa96_platform_key k = {.raw_code = code, .state = state};
  assert(fifa96_frontend_run_input(fr, &k, 1) == 0);
}

/* The brief's wrapper contract: QUIT is an engine key and must quit without
 * touching the library; directions/confirm/decline route through it. */
static void test_wrapper_contract(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  struct fifa96_frontend_run fr;
  assert(fifa96_frontend_run_init(&fr, s) == 0);
  assert(fr.phase == FIFA96_FRONTEND_PHASE_FRONTEND);

  /* QUIT is an engine key and must quit without touching the library. */
  int quit = 0;
  press(&fr, FIFA96_ENGINE_KEY_QUIT, 1);
  assert(fifa96_frontend_run_step(&fr, s, &quit) == 0);
  assert(quit == 1);

  /* Directional/confirm keys route through the library and never quit. */
  assert(fifa96_frontend_run_init(&fr, s) == 0);
  quit = 0;
  press(&fr, FIFA96_ENGINE_KEY_DOWN, 1);
  assert(fifa96_frontend_run_step(&fr, s, &quit) == 0);
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM, 1);
  assert(fifa96_frontend_run_step(&fr, s, &quit) == 0);
  press(&fr, FIFA96_ENGINE_KEY_DECLINE, 1);
  assert(fifa96_frontend_run_step(&fr, s, &quit) == 0);
  assert(quit == 0);

  /* Every step rendered a frame. */
  assert(fr.frames > 0);
  fifa96_surface_destroy(s);
}

/* Releases are ignored, empty/NULL batches tolerated, and the queue
 * saturates at eight entries. */
static void test_input_edges(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  struct fifa96_frontend_run fr;
  assert(fifa96_frontend_run_init(&fr, s) == 0);

  fifa96_platform_key release = {.raw_code = FIFA96_ENGINE_KEY_DOWN, .state = 0};
  assert(fifa96_frontend_run_input(&fr, &release, 1) == 0);
  assert(fr.queue_len == 0);
  assert(fifa96_frontend_run_input(&fr, NULL, 0) == 0);
  assert(fr.queue_len == 0);

  for (int i = 0; i < 12; i++) press(&fr, FIFA96_ENGINE_KEY_DOWN, 1);
  assert(fr.queue_len == 8);

  int quit = 0;
  assert(fifa96_frontend_run_step(&fr, s, &quit) == 0);
  assert(quit == 0);
  assert(fr.queue_len == 0);
  assert(fr.frames == 1);

  fifa96_surface_destroy(s);
}

/* The library state machine is applied to the queue: code 8 (decline) opens
 * the panel; a gated confirm is accepted by the library and parks the driver
 * in its exit phase (the engine quit is the QUIT key or a settings exit). */
static void test_phase_derivation(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  struct fifa96_frontend_run fr;
  assert(fifa96_frontend_run_init(&fr, s) == 0);
  assert(fr.phase == FIFA96_FRONTEND_PHASE_FRONTEND);
  assert(fr.entry_state == 17);

  int quit = 0;
  press(&fr, FIFA96_ENGINE_KEY_DECLINE, 1);
  assert(fifa96_frontend_run_step(&fr, s, &quit) == 0);
  assert(fr.phase == FIFA96_FRONTEND_PHASE_PANEL);
  assert(fr.entry_state == 17);
  assert(quit == 0);

  press(&fr, FIFA96_ENGINE_KEY_CONFIRM, 1);
  assert(fifa96_frontend_run_step(&fr, s, &quit) == 0);
  assert(fr.frontend.confirm == 1);
  assert(fr.phase == FIFA96_FRONTEND_PHASE_EXIT);
  assert(quit == 0);

  fifa96_surface_destroy(s);
}

/* CONFIRM is the FU-66 fire key (action code -10): with the M1 constant
 * confirm gate the library records a real CONFIRM. The M1 wrapper then
 * releases the confirm-driven EXIT on the following step so the menu keeps
 * accepting input (the real FU-65/66 post-confirm transition is M2). */
static void test_confirm_gate(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  struct fifa96_frontend_run fr;
  assert(fifa96_frontend_run_init(&fr, s) == 0);
  assert(fr.frontend.confirm == 0);

  int quit = 0;
  press(&fr, FIFA96_ENGINE_KEY_CONFIRM, 1);
  assert(fifa96_frontend_run_step(&fr, s, &quit) == 0);
  assert(fr.frontend.confirm == 1);
  assert(quit == 0);

  /* The next event completes the return to the front-end: the menu is not
   * locked in EXIT and still accepts navigation. */
  press(&fr, FIFA96_ENGINE_KEY_DOWN, 1);
  assert(fifa96_frontend_run_step(&fr, s, &quit) == 0);
  assert(fr.phase == FIFA96_FRONTEND_PHASE_FRONTEND);
  assert(fr.frontend.confirm == 0);
  assert(quit == 0);

  fifa96_surface_destroy(s);
}

/* Engine dispatch: boot without assets starts in the front-end mode and the
 * QUIT key makes fifa96_engine_should_quit flip. */
static void test_engine_mode(void) {
  const fifa96_platform_key tape[1] = {{FIFA96_ENGINE_KEY_QUIT, 1}};
  struct fifa96_platform_null_config pcfg = {0};
  pcfg.tape = tape;
  pcfg.tape_len = 1;
  pcfg.step_ns = 10000000u;
  fifa96_platform *plat = fifa96_platform_null_create(&pcfg);
  assert(plat != NULL);

  struct fifa96_engine_config ecfg = {0};
  ecfg.iso_path = NULL;
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&ecfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);
  assert(fifa96_engine_should_quit(e) == 0);

  assert(fifa96_engine_step(e) == 0);
  assert(fifa96_engine_should_quit(e) == 1);

  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
}

int main(void) {
  test_wrapper_contract();
  test_input_edges();
  test_phase_derivation();
  test_confirm_gate();
  test_engine_mode();
  puts("test_engine_frontend OK");
  return 0;
}
