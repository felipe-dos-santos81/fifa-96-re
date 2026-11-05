/* port/tests/test_engine_platform.c */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_platform_null.h"

int main(void) {
  struct fifa96_platform_null_config cfg = {0};
  cfg.step_ns = 10000000u;             /* 10 ms */
  fifa96_platform *plat = fifa96_platform_null_create(&cfg);
  assert(plat != NULL);
  assert(plat->init(plat->self, 320, 240, "test") == 0);

  struct fifa96_engine_config ecfg = {0};
  ecfg.iso_path = NULL;                /* no-assets smoke mode */
  ecfg.width = 320; ecfg.height = 240; ecfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&ecfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);
  assert(fifa96_engine_step(e) == 0);

  struct fifa96_platform_null_stats st;
  fifa96_platform_null_stats(plat, &st);
  assert(st.presents == 1u);
  assert(st.present_hash != 0u);

  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);

  /* FU-343 P5/A6-L7: a long host stall must not replay thousands of 100 Hz
   * tick bodies in one engine step. The null step_ns is one virtual second,
   * so the unclamped delta would fire 100 ticks; the engine must clamp to 10
   * (100 ms) and stay bounded. */
  struct fifa96_platform_null_config stall_cfg = {0};
  stall_cfg.step_ns = 1000000000ull;
  fifa96_platform *stall_plat = fifa96_platform_null_create(&stall_cfg);
  assert(stall_plat != NULL);
  struct fifa96_engine_config stall_ecfg = ecfg;
  stall_ecfg.headless = 0;             /* boot baselines last_ns to now_ns */
  struct fifa96_engine *stall_engine = fifa96_engine_create(&stall_ecfg, stall_plat);
  assert(stall_engine != NULL);
  assert(fifa96_engine_boot(stall_engine) == 0);
  /* The null clock advances on present: step 1 baselines (delta 0), step 2
   * carries the full virtual second. */
  assert(fifa96_engine_step(stall_engine) == 0);
  assert(fifa96_engine_step(stall_engine) == 0);
  assert(stall_engine->step_ticks == 10u);   /* one 100 ms clamp window */
  fifa96_engine_destroy(stall_engine);
  fifa96_platform_destroy(stall_plat);

  puts("test_engine_platform OK");
  return 0;
}
