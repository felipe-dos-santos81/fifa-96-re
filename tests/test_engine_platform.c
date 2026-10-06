/* tests/test_engine_platform.c */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_engine/fifa96_engine.h"
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
  puts("test_engine_platform OK");
  return 0;
}
