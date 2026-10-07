/* src/fifa96_engine/fifa96_main.c — the `fifa96` executable.
 *
 * Usage: fifa96 [ISO | --no-assets] [--headless]
 * With no ISO argument the engine boots in no-assets smoke mode (black
 * 320x240 front-end canvas), which is what `make game FILE=` uses to check
 * the SDL3 path without the disc image.
 */
#include <stdio.h>
#include <string.h>
#include "fifa96_engine/fifa96_engine.h"

int main(int argc, char **argv) {
  int headless = 0;
  const char *iso = NULL;
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1] == '-') {
      if (!strcmp(argv[i], "--headless")) headless = 1;
      else if (!strcmp(argv[i], "--no-assets")) iso = NULL;
      else if (!strcmp(argv[i], "--help")) {
        puts("fifa96 [ISO | --no-assets] [--headless]");
        return 0;
      } else {
        fprintf(stderr, "fifa96: unknown option %s\n", argv[i]);
        return 1;
      }
    } else {
      iso = argv[i];
    }
  }
  fifa96_platform *plat = fifa96_platform_sdl3_create();
  if (!plat) {
    fputs("SDL3 backend unavailable\n", stderr);
    return 1;
  }
  struct fifa96_engine_config cfg = {.iso_path = iso, .width = 320, .height = 240,
                                     .headless = headless};
  struct fifa96_engine *e = fifa96_engine_create(&cfg, plat);
  if (!e || fifa96_engine_boot(e) != 0) {
    fputs("fifa96: boot failed\n", stderr);
    fifa96_engine_destroy(e);
    fifa96_platform_destroy(plat);
    return 1;
  }
  int rc = fifa96_engine_run(e);
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
  return rc;
}
