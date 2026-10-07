/* tests/test_engine_sdl3.c — SDL3 backend smoke test.
 *
 * Runs only when SDL3 is present (CMake guards the target and CTest entry on
 * SDL3_FOUND). The dummy video/audio drivers keep it headless: SDL3 reads
 * SDL_VIDEO_DRIVER/SDL_AUDIO_DRIVER, SDL2's spelling SDL_VIDEODRIVER/
 * SDL_AUDIODRIVER is set too so the harness behaves under either era. The
 * dummy video driver picks the software renderer, so create/init/present
 * exercise the full backend path without a display.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "fifa96_engine/fifa96_platform.h"

int main(void) {
  /* SDL2 and SDL3 spellings; the backend honours SDL3's hint name too. */
  setenv("SDL_VIDEO_DRIVER", "dummy", 1);
  setenv("SDL_AUDIO_DRIVER", "dummy", 1);
  setenv("SDL_VIDEODRIVER", "dummy", 1);
  setenv("SDL_AUDIODRIVER", "dummy", 1);
  fifa96_platform *p = fifa96_platform_sdl3_create();
  assert(p != NULL);
  assert(p->init(p->self, 320, 240, "test") == 0);
  p->audio_open(p->self, 22050u, 2);
  static uint8_t planes[4][320 * 80];
  fifa96_platform_frame f = {.planes = {planes[0], planes[1], planes[2], planes[3]},
                             .stride = 80u, .flags = 0};
  assert(p->present(p->self, &f) == 0);
  assert(p->now_ns(p->self) > 0);
  p->shutdown(p->self);
  fifa96_platform_destroy(p);
  puts("test_engine_sdl3 OK");
  return 0;
}
