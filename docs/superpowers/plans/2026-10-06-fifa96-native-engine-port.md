# FIFA 96 native engine port — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a native engine that boots FIFA 96 from the original CD image, plays the intro and front-end (M1), and then plays a match (M2) on Linux/macOS, reusing the 56 existing clean-room libraries unchanged.

**Architecture:** One SDL3 dependency isolated behind a frozen platform ABI (`fifa96_platform.h`); an engine core owning game state and a fixed-step loop; a headless `null` backend giving deterministic frame/audio hashes so `make check` stays dependency-free; rendering is an indexed 320×240 surface converted to Mode-X planes for presentation.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake ≥ 3.28 + CTest, SDL3 (optional target), Python 3 for tape-driven tests, existing `fifa96_*` libraries.

**Spec:** `docs/superpowers/specs/2026-10-06-fifa96-native-engine-port-design.md`

## Global Constraints

- C11, compiled with `-Wall -Wextra -Werror`; no warnings.
- `make check` must stay green (currently 80 tests) after every task.
- No new **mandatory** dependency: SDL3 is optional; the `null` backend and `fifa96_engine` library always build.
- Determinism contract: the `null` backend is the regression source of truth; the engine never reads back from the platform, so SDL cannot change hashes.
- Evidence-gated: every reverse-engineering claim cites bytes/addresses/tool output; unprovable items become named open legs; corrections land as errata, never by rewriting prior map rows.
- Authoritative Ghidra program is `/FIFA96.EXE` (native LE, fixups applied); flat offsets need `+0x100000`. `program="fifa96.exe"` case-collides with `/FIFA96.EXE` — target the loader explicitly.
- Ghidra writes (renames/comments/`fifa96.rep` commits) and all git commits are serialized by the controller; read-only Ghidra calls may run in parallel.
- `tmp*.ps` never committed; `.gitignore` already excludes `fifa96.rep/**/tmp*.ps`.
- Commit style: `feat(engine): …`, `docs(fuNNN): …`, `chore(engine): …`.
- Original assets are never committed; tests skip ISO-dependent cases when `game/FIFAPCCD96.iso` is absent.

## File Structure

| Path | Responsibility |
|---|---|
| `include/fifa96_engine/fifa96_platform.h` | Frozen platform ABI (video/audio/input/time) |
| `include/fifa96_engine/fifa96_platform_null.h` | Null backend factory + observability stats |
| `include/fifa96_engine/fifa96_engine.h` | Opaque engine API (create/boot/step/run/destroy) |
| `include/fifa96_engine/fifa96_surface.h` | Indexed 320×240 surface + palette + Mode-X plane packing |
| `include/fifa96_engine/fifa96_asset.h` | ISO mount, path→lba/size table, raw byte cache |
| `src/fifa96_engine/fifa96_engine_internal.h` | Engine state struct (controller-owned; single writer) |
| `src/fifa96_engine/fifa96_engine.c` | Engine core + mode dispatch loop |
| `src/fifa96_engine/fifa96_surface.c` | Surface/plane implementation |
| `src/fifa96_engine/fifa96_asset.c` | ISO mount + asset table + byte cache |
| `include/fifa96_engine/fifa96_cache.h`, `src/fifa96_engine/fifa96_cache.c` | Runtime byte cache |
| `src/fifa96_engine/fifa96_clock.c`, `include/fifa96_engine/fifa96_clock.h` | 100 Hz PIT model driver + tick ISR |
| `src/fifa96_engine/fifa96_intro.c` | TGV intro playback into the surface |
| `src/fifa96_engine/fifa96_frontend_run.c` | Front-end state machine + menu rendering + input |
| `src/fifa96_engine/fifa96_match_run.c` | M2 match wiring (lifecycle/frame/display/entities) |
| `src/fifa96_engine/platform_null.c` | Headless backend (hash/replay/fixed-step) |
| `src/fifa96_engine/platform_sdl3.c` | SDL3 backend |
| `src/fifa96_engine/fifa96_main.c` | `fifa96` executable entry (SDL3 target) |
| `tests/test_engine_*.c` | CTest cases per task |
| `tests/golden/engine/` | Pinned engine frame/audio hashes |

---

## M1 — Boot to front-end

### Task 1: Contract freeze — platform ABI, engine skeleton, null backend

**Files:**
- Create: `include/fifa96_engine/fifa96_platform.h`
- Create: `include/fifa96_engine/fifa96_platform_null.h`
- Create: `include/fifa96_engine/fifa96_engine.h`
- Create: `src/fifa96_engine/fifa96_engine_internal.h`
- Create: `src/fifa96_engine/fifa96_engine.c`
- Create: `src/fifa96_engine/platform_null.c`
- Create: `tests/test_engine_platform.c`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: nothing (first task).
- Produces: `fifa96_platform`, `fifa96_platform_frame`, `fifa96_platform_key` (ABI, frozen); `fifa96_platform_null_create()` / `fifa96_platform_null_stats()`; `fifa96_engine_create/boot/step/run/should_quit/destroy` (opaque).

- [ ] **Step 1: Write the failing test**

```c
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
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make build 2>&1 | tail -5`
Expected: configure/build fails on the missing `fifa96_engine/fifa96_platform.h`.

- [ ] **Step 3: Write the ABI headers**

`include/fifa96_engine/fifa96_platform.h` — complete content:

```c
#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  const uint8_t *planes[4];    /* Mode-X planes, 320x240 */
  size_t         stride;       /* plane stride in bytes (80) */
  const uint8_t  palette[768]; /* 8-bit RGB, DAC-expanded */
  uint32_t       flags;
} fifa96_platform_frame;

typedef struct {
  int32_t raw_code;            /* backend-neutral key/pad code */
  int32_t state;               /* 0 = release, 1 = press */
} fifa96_platform_key;

typedef struct fifa96_platform {
  int      (*init)(void *self, int w, int h, const char *title);
  void     (*shutdown)(void *self);
  void     (*destroy)(void *self);   /* frees backend state; generic destroy calls it */
  int      (*poll)(void *self, fifa96_platform_key *out, size_t cap, int *count);
  int      (*present)(void *self, const fifa96_platform_frame *frame);
  void     (*audio_open)(void *self, uint32_t rate, int channels);
  void     (*audio_submit)(void *self, const int16_t *pcm, size_t frames);
  uint64_t (*now_ns)(void *self);
  void     (*sleep_ns)(void *self, uint64_t ns);
  void     *self;
} fifa96_platform;

fifa96_platform *fifa96_platform_sdl3_create(void);
void fifa96_platform_destroy(fifa96_platform *p);
```

`include/fifa96_engine/fifa96_platform_null.h` — complete content:

```c
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_platform.h"

struct fifa96_platform_null_config {
  const fifa96_platform_key *tape;  /* scripted input, replayed on poll */
  size_t tape_len;
  uint64_t step_ns;                 /* virtual clock step per now_ns() call */
  int audio_ring_frames;            /* default 4096 when 0 */
};

struct fifa96_platform_null_stats {
  uint64_t presents;
  uint64_t frames;        /* frames presented in total */
  uint64_t audio_frames;  /* PCM frames submitted */
  uint64_t present_hash;  /* FNV-1a 64 over planes + palette */
  uint64_t audio_hash;    /* FNV-1a 64 over PCM (little-endian bytes) */
};

fifa96_platform *fifa96_platform_null_create(const struct fifa96_platform_null_config *cfg);
void fifa96_platform_null_stats(const fifa96_platform *p, struct fifa96_platform_null_stats *out);
```

`include/fifa96_engine/fifa96_engine.h` — complete content:

```c
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
```

`src/fifa96_engine/fifa96_engine_internal.h` — complete content:

```c
#pragma once
#include "fifa96_engine/fifa96_engine.h"

struct fifa96_engine {
  struct fifa96_engine_config cfg;
  fifa96_platform *plat;
  int booted;
  int quit;
  uint64_t frames;
  uint8_t solids[4][320 * 80];  /* temporary Task-1 surface; Task 3 replaces */
  uint8_t palette[768];
};
```

- [ ] **Step 4: Write minimal implementations**

`src/fifa96_engine/platform_null.c`:

```c
#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_platform_null.h"

struct null_state {
  struct fifa96_platform_null_config cfg;
  struct fifa96_platform_null_stats st;
  size_t tape_pos;
  uint32_t rate;
  int channels;
};

static uint64_t fnv1a(uint64_t h, const uint8_t *p, size_t n) {
  for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= 1099511628211ull; }
  return h;
}

static int n_init(void *self, int w, int h, const char *title) {
  (void)self; (void)w; (void)h; (void)title;
  return 0;
}
static void n_shutdown(void *self) { (void)self; }
static int n_poll(void *self, fifa96_platform_key *out, size_t cap, int *count) {
  struct null_state *s = self;
  size_t n = 0;
  while (s->tape_pos < s->cfg.tape_len && n < cap) {
    out[n++] = s->cfg.tape[s->tape_pos++];
  }
  *count = (int)n;
  return 0;
}
static int n_present(void *self, const fifa96_platform_frame *f) {
  struct null_state *s = self;
  s->st.presents++;
  s->st.frames += 240;
  s->st.present_hash = fnv1a(s->st.present_hash ? s->st.present_hash : 14695981039346656037ull,
                             f->planes[0], f->stride * 240);
  for (int p = 1; p < 4; p++) {
    s->st.present_hash = fnv1a(s->st.present_hash, f->planes[p], f->stride * 240);
  }
  s->st.present_hash = fnv1a(s->st.present_hash, f->palette, 768);
  return 0;
}
static void n_audio_open(void *self, uint32_t rate, int channels) {
  struct null_state *s = self; s->rate = rate; s->channels = channels;
}
static void n_audio_submit(void *self, const int16_t *pcm, size_t frames) {
  struct null_state *s = self;
  s->st.audio_frames += frames;
  s->st.audio_hash = fnv1a(s->st.audio_hash ? s->st.audio_hash : 14695981039346656037ull,
                           (const uint8_t *)pcm, frames * (size_t)s->channels * 2u);
}
static uint64_t n_now_ns(void *self) {
  struct null_state *s = self;
  return s->cfg.step_ns * (s->st.presents + 1u);
}
static void n_sleep_ns(void *self, uint64_t ns) { (void)self; (void)ns; }
static void n_destroy(void *self) { free(self); }

fifa96_platform *fifa96_platform_null_create(const struct fifa96_platform_null_config *cfg) {
  struct null_state *s = calloc(1, sizeof *s);
  if (!s) return NULL;
  if (cfg) s->cfg = *cfg;
  if (s->cfg.step_ns == 0) s->cfg.step_ns = 16666667ull;
  fifa96_platform *p = calloc(1, sizeof *p);
  if (!p) { free(s); return NULL; }
  p->init = n_init; p->shutdown = n_shutdown; p->destroy = n_destroy;
  p->poll = n_poll; p->present = n_present;
  p->audio_open = n_audio_open; p->audio_submit = n_audio_submit;
  p->now_ns = n_now_ns; p->sleep_ns = n_sleep_ns; p->self = s;
  return p;
}

void fifa96_platform_null_stats(const fifa96_platform *p, struct fifa96_platform_null_stats *out) {
  const struct null_state *s = p->self;
  *out = s->st;
}
```

`src/fifa96_engine/fifa96_engine.c`:

```c
#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_engine_internal.h"

struct fifa96_engine *fifa96_engine_create(const struct fifa96_engine_config *cfg,
                                           fifa96_platform *plat) {
  if (!cfg || !plat) return NULL;
  struct fifa96_engine *e = calloc(1, sizeof *e);
  if (!e) return NULL;
  e->cfg = *cfg;
  e->plat = plat;
  if (e->cfg.width == 0) e->cfg.width = 320;
  if (e->cfg.height == 0) e->cfg.height = 240;
  return e;
}

int fifa96_engine_boot(struct fifa96_engine *e) {
  if (!e || e->booted) return -1;
  if (e->plat->init(e->plat->self, e->cfg.width, e->cfg.height, "FIFA 96") != 0) return -1;
  e->plat->audio_open(e->plat->self, 22050u, 2);
  memset(e->palette, 0, sizeof e->palette);
  e->booted = 1;
  return 0;
}

int fifa96_engine_step(struct fifa96_engine *e) {
  if (!e || !e->booted) return -1;
  fifa96_platform_frame f;
  for (int p = 0; p < 4; p++) f.planes[p] = e->solids[p];
  f.stride = 80u;
  memcpy((void *)f.palette, e->palette, 768);
  f.flags = 0;
  if (e->plat->present(e->plat->self, &f) != 0) return -1;
  e->frames++;
  return 0;
}

int fifa96_engine_run(struct fifa96_engine *e) {
  while (e && !e->quit) {
    if (fifa96_engine_step(e) != 0) return -1;
    fifa96_platform_key keys[32];
    int n = 0;
    e->plat->poll(e->plat->self, keys, 32, &n);
  }
  return 0;
}

int fifa96_engine_should_quit(const struct fifa96_engine *e) { return e ? e->quit : 1; }
void fifa96_engine_destroy(struct fifa96_engine *e) {
  if (!e) return;
  if (e->booted) e->plat->shutdown(e->plat->self);
  free(e);
}

/* Defined once here; each backend sets its own `destroy` slot. */
void fifa96_platform_destroy(fifa96_platform *p) {
  if (!p) return;
  if (p->destroy) p->destroy(p->self);
  free(p);
}
```

- [ ] **Step 5: Register the test in CMake**

Append to `CMakeLists.txt`:

```cmake
# ── Engine (M1) ────────────────────────────────────────────────────────────────
add_library(fifa96_engine
  src/fifa96_engine/fifa96_engine.c
  src/fifa96_engine/platform_null.c)
target_include_directories(fifa96_engine PUBLIC include)
target_compile_options(fifa96_engine PRIVATE -Wall -Wextra -Werror)

add_executable(test_engine_platform tests/test_engine_platform.c)
target_link_libraries(test_engine_platform PRIVATE fifa96_engine)
add_test(NAME test_engine_platform COMMAND test_engine_platform
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
```

- [ ] **Step 6: Run the test to verify it passes**

Run: `make build && ./build/test_engine_platform`
Expected: `test_engine_platform OK`

- [ ] **Step 7: Full gate**

Run: `make check`
Expected: `100% tests passed` (81 tests).

- [ ] **Step 8: Commit**

```bash
git add include/fifa96_engine src/fifa96_engine tests/test_engine_platform.c CMakeLists.txt
git commit -m "feat(engine): freeze platform ABI, engine skeleton, null backend"
```

---

### Task 2: SDL3 backend + `make game`

**Files:**
- Create: `src/fifa96_engine/platform_sdl3.c`
- Create: `src/fifa96_engine/fifa96_main.c`
- Create: `tests/test_engine_sdl3.c`
- Modify: `CMakeLists.txt`
- Modify: `Makefile`

**Interfaces:**
- Consumes: `fifa96_platform` (Task 1), `fifa96_engine_*` (Task 1).
- Produces: `fifa96_platform_sdl3_create()`; the `fifa96` executable target; `make game`.

- [ ] **Step 1: Write the failing test**

```c
/* tests/test_engine_sdl3.c — runs only when SDL3 is present; dummy drivers */
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
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make build 2>&1 | tail -5`
Expected: configure/build fails on `fifa96_platform_sdl3_create` (not defined) once SDL3 is installed; if SDL3 is absent, CMake must skip this target — install SDL3 (`apt install libsdl3-dev` or build from source) before running this task.

- [ ] **Step 3: Implement the SDL3 backend**

`src/fifa96_engine/platform_sdl3.c` (structure; complete the audio ring):

- `struct sdl_state { SDL_Window *win; SDL_Renderer *ren; SDL_Texture *tex; SDL_AudioStream *stream; int w, h; uint8_t rgba[320*240*4]; uint64_t audio_frames; }`.
- `init`: `SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)`; `SDL_CreateWindowAndRenderer("FIFA 96", 960, 720, SDL_WINDOW_RESIZABLE, &win, &ren)`; `SDL_SetRenderLogicalPresentation(ren, 320, 240, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)` (SDL3) or manual scaling fallback for SDL2; `SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 320, 240)`.
- `present`: expand the four Mode-X planes + palette into `rgba` on the CPU (per pixel: `idx = ((plane0>>bit)&1) | ((plane1>>bit)&2) | ((plane2>>bit)&4) | ((plane3>>bit)&8)`, `bit = 7 - (x&7)`, byte `row*80 + (x>>3)`), then `SDL_UpdateTexture` + `SDL_RenderClear` + `SDL_RenderTexture` + `SDL_RenderPresent`.
- `poll`: `SDL_PumpEvents`; `SDL_PollEvent` (QUIT → set a quit flag the engine reads via a `should_quit` hook); map `SDL_SCANCODE_UP/DOWN/LEFT/RIGHT/RETURN/ESCAPE` and gamepad dpad/A into `raw_code`s `1..7`; emit press/release `fifa96_platform_key`.
- `audio_open`: `SDL_OpenAudioDevice` with `SDL_AUDIO_S16`/2ch/22050 Hz and a callback pulling from a lock-protected ring buffer the engine fills via `audio_submit` (drop-oldest on overflow).
- `now_ns` = `SDL_GetTicksNS()`; `sleep_ns` = `SDL_DelayNS(ns)`.
- `shutdown`/`destroy`: close device, destroy texture/renderer/window, `SDL_Quit`; set `p->destroy` to the state-freeing function (the generic `fifa96_platform_destroy` in `fifa96_engine.c` calls it, so the backend must not define `fifa96_platform_destroy` itself).

`src/fifa96_engine/fifa96_main.c`:

```c
#include <stdio.h>
#include "fifa96_engine/fifa96_engine.h"

int main(int argc, char **argv) {
  int headless = 0;
  const char *iso = NULL;
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1] == '-') {
      if (!strcmp(argv[i], "--headless")) headless = 1;
      else if (!strcmp(argv[i], "--no-assets")) iso = NULL;
      else if (!strcmp(argv[i], "--help")) { puts("fifa96 [ISO | --no-assets] [--headless]"); return 0; }
    } else {
      iso = argv[i];
    }
  }
  fifa96_platform *plat = fifa96_platform_sdl3_create();
  if (!plat) { fputs("SDL3 backend unavailable\n", stderr); return 1; }
  struct fifa96_engine_config cfg = {.iso_path = iso, .width = 320, .height = 240,
                                     .headless = headless};
  struct fifa96_engine *e = fifa96_engine_create(&cfg, plat);
  if (!e || fifa96_engine_boot(e) != 0) { fputs("boot failed\n", stderr); return 1; }
  int rc = fifa96_engine_run(e);
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
  return rc;
}
```

(`fifa96_main.c` includes `<string.h>` and needs `fifa96_platform_sdl3_create` declared in the ABI header — it is.)

- [ ] **Step 4: CMake + Makefile wiring**

Append to `CMakeLists.txt`:

```cmake
find_package(SDL3 QUIET)
if(SDL3_FOUND)
  message(STATUS "engine: SDL3 found -> building fifa96 + test_engine_sdl3")
  add_executable(test_engine_sdl3 tests/test_engine_sdl3.c
                 src/fifa96_engine/platform_sdl3.c)
  target_link_libraries(test_engine_sdl3 PRIVATE fifa96_engine SDL3::SDL3)
  add_test(NAME test_engine_sdl3 COMMAND test_engine_sdl3
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  add_executable(fifa96 src/fifa96_engine/fifa96_main.c
                 src/fifa96_engine/platform_sdl3.c)
  target_link_libraries(fifa96 PRIVATE fifa96_engine SDL3::SDL3)
else()
  message(STATUS "engine: SDL3 NOT found -> headless-only build (make game unavailable)")
endif()
```

Append to `Makefile` (add `game` to `.PHONY`):

```make
game: build ## Build and run the native engine (needs SDL3 + game/FIFAPCCD96.iso)
	@test -x $(BUILD)/fifa96 || { echo "fifa96 not built (SDL3 missing)"; exit 1; }
	./$(BUILD)/fifa96 $(if $(FILE),$(FILE),game/FIFAPCCD96.iso)
```

- [ ] **Step 5: Run tests and gate**

Run: `make check && ./build/test_engine_sdl3`
Expected: `100% tests passed` (82 with SDL3) and `test_engine_sdl3 OK`.

- [ ] **Step 6: Manual acceptance**

Run: `make game FILE=` (i.e. `./build/fifa96` with no ISO → no-assets smoke mode) — use `./build/fifa96 --no-assets`.
Expected: a 960×720 window opens showing black; ESC closes it.

- [ ] **Step 7: Commit**

```bash
git add src/fifa96_engine/platform_sdl3.c src/fifa96_engine/fifa96_main.c tests/test_engine_sdl3.c CMakeLists.txt Makefile
git commit -m "feat(engine): SDL3 backend, fifa96 executable, make game"
```

---

### Task 3: Render surface (indexed → Mode-X → present)

**Files:**
- Create: `include/fifa96_engine/fifa96_surface.h`
- Create: `src/fifa96_engine/fifa96_surface.c`
- Create: `tests/test_engine_surface.c`
- Modify: `src/fifa96_engine/fifa96_engine.c`, `src/fifa96_engine/fifa96_engine_internal.h`, `CMakeLists.txt`

**Interfaces:**
- Consumes: `fifa96_platform_frame` (Task 1).
- Produces: `struct fifa96_surface`, `fifa96_surface_clear`, `fifa96_surface_set_palette6`, `fifa96_surface_plane`.

- [ ] **Step 1: Write the failing test**

```c
/* tests/test_engine_surface.c */
#include <assert.h>
#include <stdio.h>
#include "fifa96_engine/fifa96_surface.h"

int main(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  assert(s != NULL);
  uint8_t pal6[768] = {0};
  pal6[0x11 * 3 + 0] = 0x3F;  /* entry 0x11 = bright red */
  fifa96_surface_set_palette6(s, pal6);
  fifa96_surface_clear(s, 0x11);
  assert(s->palette[0x11 * 3 + 0] == 0xFF);  /* 6-bit 0x3F -> 8-bit */

  fifa96_platform_frame f;
  fifa96_surface_plane(s, &f);
  /* 0x11 = 0b00010001 -> plane 0 bit set, plane 1/2 clear, plane 3 set. */
  for (size_t i = 0; i < 80u * 240u; i++) {
    assert(f.planes[0][i] == 0xFFu);
    assert(f.planes[1][i] == 0x00u);
    assert(f.planes[2][i] == 0x00u);
    assert(f.planes[3][i] == 0xFFu);
  }
  fifa96_surface_clear(s, 0x00);
  fifa96_surface_plane(s, &f);
  assert(f.planes[0][0] == 0x00u && f.planes[3][0] == 0x00u);

  fifa96_surface_destroy(s);
  puts("test_engine_surface OK");
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make build 2>&1 | tail -5`
Expected: build fails on the missing surface header.

- [ ] **Step 3: Write the surface header + implementation**

`include/fifa96_engine/fifa96_surface.h`:

```c
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_platform.h"

#define FIFA96_SURFACE_MAX_W 320
#define FIFA96_SURFACE_MAX_H 240

struct fifa96_surface {
  int width, height;
  uint8_t indexed[FIFA96_SURFACE_MAX_W * FIFA96_SURFACE_MAX_H];
  uint8_t palette[768];       /* 8-bit RGB */
  uint8_t planes[4][FIFA96_SURFACE_MAX_W * FIFA96_SURFACE_MAX_H / 4];
};

struct fifa96_surface *fifa96_surface_create(int w, int h);
void fifa96_surface_destroy(struct fifa96_surface *s);
void fifa96_surface_clear(struct fifa96_surface *s, uint8_t index);
void fifa96_surface_set_palette6(struct fifa96_surface *s, const uint8_t rgb6[768]);
void fifa96_surface_set_palette8(struct fifa96_surface *s, const uint8_t rgb8[768]);
void fifa96_surface_plane(struct fifa96_surface *s, fifa96_platform_frame *out);
uint64_t fifa96_surface_hash(const struct fifa96_surface *s); /* FNV-1a over indexed + palette */
```

Implementation notes: `set_palette6` scales `(v << 2) | (v >> 4)`; `plane` packs each row's eight pixels per byte per plane with `bit = 7 - (x & 7)`, byte index `y * (width / 8) + (x >> 3)`, and sets `out->stride = width / 4` (80), `out->palette = s->palette`, `out->flags = 0`. `hash` is FNV-1a 64 over `indexed` then `palette` (offset basis `14695981039346656037ull`, prime `1099511628211ull`), used by the menu-art and acceptance tests.

- [ ] **Step 4: Wire the engine to the surface**

Replace Task 1's temporary `solids`/`palette` in `fifa96_engine_internal.h` with `struct fifa96_surface *surface;`. In `boot`, `surface = fifa96_surface_create(w, h)` and clear to black; in `step`, `fifa96_surface_plane(e->surface, &f)` then present; in `destroy`, `fifa96_surface_destroy`.

- [ ] **Step 5: Run tests and gate**

Run: `make check && ./build/test_engine_surface`
Expected: `test_engine_surface OK`, `100% tests passed` (83 with SDL3).

- [ ] **Step 6: Commit**

```bash
git add include/fifa96_engine/fifa96_surface.h src/fifa96_engine/fifa96_surface.c \
        src/fifa96_engine/fifa96_engine.c src/fifa96_engine/fifa96_engine_internal.h \
        tests/test_engine_surface.c CMakeLists.txt
git commit -m "feat(engine): indexed surface and Mode-X plane presentation"
```

---

### Task 4: Boot + asset table (ISO mount, path enumeration)

**Files:**
- Create: `include/fifa96_engine/fifa96_asset.h`
- Create: `src/fifa96_engine/fifa96_asset.c`
- Create: `tests/test_engine_asset.c`
- Modify: `src/fifa96_engine/fifa96_engine.c`, `CMakeLists.txt`

**Interfaces:**
- Consumes: `fifa96_iso9660_*` (existing).
- Produces: `struct fifa96_asset_table`, `fifa96_asset_mount_iso`, `fifa96_asset_mount_file`, `fifa96_asset_lookup`, `fifa96_asset_read`, `fifa96_asset_unmount`.

- [ ] **Step 1: Write the failing test**

Build a minimal in-memory ISO in the test (PVD at sector 16 with `CD001`, one directory record for `ART/PIX.PVI`), following the structure used by `tests/test_iso9660.c` (reuse its helper if `static`; otherwise construct the 2048-byte sectors inline). Then:

```c
/* tests/test_engine_asset.c */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_engine/fifa96_asset.h"

/* ... build synthetic ISO into `uint8_t image[64 * 2048]` ... */

int main(void) {
  uint8_t image[64 * 2048];
  size_t image_len = build_test_iso(image, sizeof image);
  struct fifa96_asset_table *t = NULL;
  assert(fifa96_asset_mount_iso(mem_read, image, image_len, &t) == FIFA96_OK);
  uint32_t lba = 0, size = 0;
  assert(fifa96_asset_lookup(t, "/ART/PIX.PVI", &lba, &size) == FIFA96_OK);
  assert(size == 4u);
  uint8_t *bytes = NULL; size_t len = 0;
  assert(fifa96_asset_read(t, "/ART/PIX.PVI", &bytes, &len) == FIFA96_OK);
  assert(len == 4u && memcmp(bytes, "DATA", 4) == 0);
  assert(fifa96_asset_lookup(t, "/NOPE", &lba, &size) == FIFA96_ERR_NOT_FOUND);
  fifa96_asset_free(bytes);
  fifa96_asset_unmount(t);
  puts("test_engine_asset OK");
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make build 2>&1 | tail -5`
Expected: build fails on the missing asset header.

- [ ] **Step 3: Implement the asset table**

`include/fifa96_engine/fifa96_asset.h`:

```c
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_iso9660.h"

struct fifa96_asset_entry { char path[FIFA96_ISO9660_PATH_MAX]; uint32_t lba, size; };
struct fifa96_asset_table {
  struct fifa96_iso9660 iso;
  struct fifa96_asset_entry *entries;
  size_t count, cap;
};

fifa96_err_t fifa96_asset_mount_iso(fifa96_iso9660_read_fn read, void *ctx, uint64_t len,
                                    struct fifa96_asset_table **out);
fifa96_err_t fifa96_asset_mount_file(const char *path, struct fifa96_asset_table **out);
fifa96_err_t fifa96_asset_lookup(const struct fifa96_asset_table *t, const char *path,
                                 uint32_t *lba, uint32_t *size);
fifa96_err_t fifa96_asset_read(const struct fifa96_asset_table *t, const char *path,
                               uint8_t **out, size_t *len);
void fifa96_asset_free(uint8_t *p);
void fifa96_asset_unmount(struct fifa96_asset_table *t);
```

Implementation notes: mount walks the ISO with `fifa96_iso9660_walk`, copying each visited path into a growable `entries` array (case-insensitive compare in `lookup`, matching the game's DOS paths); `read` looks up, allocates `size`, and calls `fifa96_iso9660_read_extent`. `mount_file` opens with stdio and supplies a `fseek`/`fread` reader, keeping the `FILE*` in a wrapper struct whose pointer is `ctx`; `unmount` closes it. Bounds-check every allocation. Engine boot: when `cfg.iso_path != NULL`, mount the table and store it; when NULL, boot with an empty table (smoke mode).

- [ ] **Step 4: Run tests and gate**

Run: `make check && ./build/test_engine_asset`
Expected: `test_engine_asset OK`, `100% tests passed` (84 with SDL3).

- [ ] **Step 5: Commit**

```bash
git add include/fifa96_engine/fifa96_asset.h src/fifa96_engine/fifa96_asset.c \
        src/fifa96_engine/fifa96_engine.c tests/test_engine_asset.c CMakeLists.txt
git commit -m "feat(engine): ISO mount and asset table"
```

---

### Task 5: Runtime byte cache

**Files:**
- Create: `src/fifa96_engine/fifa96_cache.c`
- Create: `include/fifa96_engine/fifa96_cache.h`
- Create: `tests/test_engine_cache.c`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `fifa96_asset_*` (Task 4).
- Produces: `struct fifa96_cache`, `fifa96_cache_get`, `fifa96_cache_destroy`.

- [ ] **Step 1: Write the failing test**

```c
/* tests/test_engine_cache.c — reuses the synthetic ISO from Task 4's test */
#include <assert.h>
#include <stdio.h>
#include "fifa96_engine/fifa96_cache.h"

int main(void) {
  uint8_t image[64 * 2048];
  size_t image_len = build_test_iso(image, sizeof image);  /* shared helper, copy it */
  struct fifa96_asset_table *t = NULL;
  assert(fifa96_asset_mount_iso(mem_read, image, image_len, &t) == FIFA96_OK);
  struct fifa96_cache *c = fifa96_cache_create(t);
  size_t len1 = 0, len2 = 0;
  const uint8_t *a = fifa96_cache_get(c, "/ART/PIX.PVI", &len1);
  const uint8_t *b = fifa96_cache_get(c, "/ART/PIX.PVI", &len2);
  assert(a && b && a == b && len1 == len2 && len1 == 4u);
  fifa96_cache_destroy(c);
  fifa96_asset_unmount(t);
  puts("test_engine_cache OK");
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make build 2>&1 | tail -5`
Expected: build fails on the missing cache header.

- [ ] **Step 3: Implement the cache**

`include/fifa96_engine/fifa96_cache.h` declares the struct and API; `fifa96_cache.c` holds a linked list of `{ char path[]; uint8_t *bytes; size_t len; }`. `get` returns the cached pointer or calls `fifa96_asset_read`, stores, and returns it; unknown path → `NULL` and `*len = 0`. `destroy` frees all entries and the table's bytes but not the asset table.

- [ ] **Step 4: Run tests and gate**

Run: `make check && ./build/test_engine_cache`
Expected: `test_engine_cache OK`, `100% tests passed` (85 with SDL3).

- [ ] **Step 5: Commit**

```bash
git add src/fifa96_engine/fifa96_cache.c src/fifa96_engine/fifa96_cache.h \
        tests/test_engine_cache.c CMakeLists.txt
git commit -m "feat(engine): runtime asset byte cache"
```

---

### Task 6: Clock and fixed-step tick loop

**Files:**
- Create: `src/fifa96_engine/fifa96_clock.c`
- Create: `include/fifa96_engine/fifa96_clock.h`
- Create: `tests/test_engine_clock.c`
- Modify: `src/fifa96_engine/fifa96_engine.c`, `CMakeLists.txt`

**Interfaces:**
- Consumes: `fifa96_pacing_clock`, `fifa96_tick` (existing).
- Produces: `struct fifa96_engine_clock`, `fifa96_clock_init`, `fifa96_clock_advance_ns`.

- [ ] **Step 1: Write the failing test**

```c
/* tests/test_engine_clock.c */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_engine/fifa96_clock.h"

static int fired;
static void on_tick(void *user) { (void)user; fired++; }

int main(void) {
  struct fifa96_engine_clock c;
  fifa96_clock_init(&c);
  /* period 5 = every 5th 100 Hz ISR = 20 Hz; register returns the slot index 0. */
  assert(fifa96_tick_register(&c.ticks, on_tick, NULL, 5u) == 0);
  /* Advance exactly one second in 10 ms steps: 100 PIT ticks, 20 callbacks. */
  for (int i = 0; i < 100; i++) fifa96_clock_advance_ns(&c, 10000000ull);
  assert(c.pit.ticks == 100u);
  assert(c.pit.ticks20 == 20u);
  assert(fired == 20);
  puts("test_engine_clock OK");
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make build 2>&1 | tail -5`
Expected: build fails on the missing clock header.

- [ ] **Step 3: Implement the clock**

```c
/* src/fifa96_engine/fifa96_clock.h */
#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_pacing.h"
#include "fifa96_loader/fifa96_tick.h"

struct fifa96_engine_clock {
  struct fifa96_pacing_clock pit;
  struct fifa96_tick ticks;
  uint64_t frac_ns;   /* sub-centisecond remainder */
  uint64_t tick_ns;   /* nanoseconds per PIT tick, 1193182 Hz divisor model */
};

void fifa96_clock_init(struct fifa96_engine_clock *c);
int  fifa96_clock_advance_ns(struct fifa96_engine_clock *c, uint64_t delta_ns);
```

`fifa96_clock.c`: `tick_ns = 10000000ull` is the 100 Hz model (the original PIT divisor `0x2E9C` gives 100.0 Hz); `advance_ns` accumulates `delta_ns + frac_ns`, fires `fifa96_tick_isr(&c->ticks, &c->pit)` once per whole 10 ms, keeps the remainder, and returns the number of PIT ticks fired. Engine `step` calls `advance_ns(plat->now_ns()` delta`)` at the top of each iteration and uses the returned tick count to drive M1/M2 logic.

- [ ] **Step 4: Run tests and gate**

Run: `make check && ./build/test_engine_clock`
Expected: `test_engine_clock OK`, `100% tests passed` (86 with SDL3).

- [ ] **Step 5: Commit**

```bash
git add src/fifa96_engine/fifa96_clock.c src/fifa96_engine/fifa96_clock.h \
        src/fifa96_engine/fifa96_engine.c tests/test_engine_clock.c CMakeLists.txt
git commit -m "feat(engine): 100 Hz pacing clock and tick loop driver"
```

---

### Task 7: Intro video playback (TGV → surface)

**Files:**
- Create: `src/fifa96_engine/fifa96_intro.c`
- Create: `include/fifa96_engine/fifa96_intro.h`
- Create: `tests/test_engine_intro.c`
- Modify: `src/fifa96_engine/fifa96_engine.c`, `CMakeLists.txt`

**Interfaces:**
- Consumes: `fifa96_vgt_player` (existing), `fifa96_surface` (Task 3), `fifa96_cache` (Task 5).
- Produces: `struct fifa96_intro`, `fifa96_intro_start`, `fifa96_intro_feed`, `fifa96_intro_step`, `fifa96_intro_done`.

- [ ] **Step 1: Write the failing test**

Use the committed vectors already exercised by `tests/test_vgt_player.c` (`tests/golden/vgt/kvgt-frame-01.bin`, `fvgt-01.*`); build the same 2-chunk stream that `tests/test_play.py` builds.

```c
/* tests/test_engine_intro.c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_intro.h"

static uint8_t *slurp(const char *path, size_t *len) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t *buf = malloc((size_t)n);
  if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) { free(buf); fclose(f); return NULL; }
  fclose(f);
  *len = (size_t)n;
  return buf;
}

int main(void) {
  size_t klen = 0;
  uint8_t *kvgt = slurp("tests/golden/vgt/kvgt-frame-01.bin", &klen);
  assert(kvgt && klen > 0);

  size_t stream_len = 8u + klen;
  uint8_t *stream = malloc(stream_len);
  uint32_t tag = 0x5447566Bu;   /* 'kVGT' as the little-endian chunk tag */
  uint32_t clen = (uint32_t)stream_len;
  memcpy(stream, &tag, 4);
  memcpy(stream + 4, &clen, 4);
  memcpy(stream + 8, kvgt, klen);

  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  struct fifa96_intro intro;
  assert(fifa96_intro_start(&intro, s) == 0);
  assert(fifa96_intro_feed(&intro, stream, stream_len) == 0);
  int steps = 0;
  while (!fifa96_intro_done(&intro) && steps < 10) {
    assert(fifa96_intro_step(&intro, s) == 0);
    steps++;
  }
  assert(steps == 1);          /* one key frame, then the stream ends */
  assert(fifa96_intro_done(&intro));
  fifa96_surface_destroy(s);
  free(stream); free(kvgt);
  puts("test_engine_intro OK");
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make build 2>&1 | tail -5`
Expected: build fails on the missing intro header.

- [ ] **Step 3: Implement intro playback**

`src/fifa96_engine/fifa96_intro.h`:

```c
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_surface.h"
#include "fifa96_loader/fifa96_vgt_player.h"

struct fifa96_intro {
  fifa96_vgt_player player;
  uint8_t canvas[320 * 240];
  uint8_t scratch[320 * 240];
  int started;
};

int fifa96_intro_start(struct fifa96_intro *in, struct fifa96_surface *s);
int fifa96_intro_feed(struct fifa96_intro *in, const uint8_t *stream, size_t len);
int fifa96_intro_step(struct fifa96_intro *in, struct fifa96_surface *s);
int fifa96_intro_done(const struct fifa96_intro *in);
```

`fifa96_intro.c`: `start` calls `fifa96_vgt_player_init(&in->player, 320, 240, in->canvas, sizeof in->canvas, in->scratch, sizeof in->scratch, on_audio, in)`; `feed` calls `fifa96_vgt_player_feed`; `step` calls `fifa96_vgt_player_step`, and on a frame copies `frame->pixels` over `s->indexed` (row-major 320×240), calls `fifa96_surface_set_palette8(s, frame->palette)` when `palette_changed`, and returns 0; `done` returns `fifa96_vgt_player_ended`. `on_audio` forwards companion `1SNh/1SNd` chunks to the engine mixer when M2 wires it; for M1 it is a no-op (record as an open leg: audio during video, FU-37 §B).

- [ ] **Step 4: Wire into engine boot/intro**

Engine `boot` loads `VIDEO/<name>.TGV` via the cache when an ISO is mounted (choose the path from the load-order table; until that is derived, `fifa96_intro_start` on the first `VIDEO/*.TGV` found by a bounded table scan, recorded as an open leg), feeds it, and `step` advances one intro frame per video tick (15 fps via `fifa96_pacing_frames_due`), presenting each.

- [ ] **Step 5: Run tests and gate**

Run: `make check && ./build/test_engine_intro`
Expected: `test_engine_intro OK`, `100% tests passed` (87 with SDL3).

- [ ] **Step 6: Commit**

```bash
git add src/fifa96_engine/fifa96_intro.c src/fifa96_engine/fifa96_intro.h \
        src/fifa96_engine/fifa96_engine.c tests/test_engine_intro.c CMakeLists.txt
git commit -m "feat(engine): TGV intro playback into the surface"
```

---

### Task 8: Front-end control flow and input

**Files:**
- Create: `src/fifa96_engine/fifa96_frontend_run.c`
- Create: `include/fifa96_engine/fifa96_frontend_run.h`
- Create: `tests/test_engine_frontend.c`
- Modify: `src/fifa96_engine/fifa96_engine.c`, `CMakeLists.txt`

**Interfaces:**
- Consumes: `fifa96_frontend` (existing), `fifa96_platform_key` (Task 1), `fifa96_surface` (Task 3).
- Produces: `struct fifa96_frontend_run`, `fifa96_frontend_run_init`, `fifa96_frontend_run_input`, `fifa96_frontend_run_step`.

- [ ] **Step 1: Write the failing test**

```c
/* tests/test_engine_frontend.c */
#include <assert.h>
#include <stdio.h>
#include "fifa96_engine/fifa96_frontend_run.h"

static void press(struct fifa96_frontend_run *fr, int32_t code, int state) {
  fifa96_platform_key k = {.raw_code = code, .state = state};
  assert(fifa96_frontend_run_input(fr, &k, 1) == 0);
}

int main(void) {
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
  puts("test_engine_frontend OK");
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make build 2>&1 | tail -5`
Expected: build fails on the missing front-end-run header.

- [ ] **Step 3: Implement input mapping and the state machine**

`fifa96_frontend_run.h`:

```c
#pragma once
#include <stdint.h>
#include "fifa96_engine/fifa96_surface.h"
#include "fifa96_loader/fifa96_frontend.h"

#define FIFA96_ENGINE_KEY_UP      1
#define FIFA96_ENGINE_KEY_DOWN    2
#define FIFA96_ENGINE_KEY_LEFT    3
#define FIFA96_ENGINE_KEY_RIGHT   4
#define FIFA96_ENGINE_KEY_CONFIRM 5
#define FIFA96_ENGINE_KEY_DECLINE 6
#define FIFA96_ENGINE_KEY_QUIT    7

struct fifa96_frontend_run {
  struct fifa96_frontend frontend;  /* engine/library state machine */
  struct fifa96_surface *surface;
  uint32_t phase;                   /* mirrors fifa96_frontend.phase */
  uint32_t entry_state;             /* FU-65 state table index */
  int      queue[8];                /* pending mapped key codes */
  int      queue_len;
  uint64_t frames;                  /* frames rendered since init */
};

int fifa96_frontend_run_init(struct fifa96_frontend_run *fr, struct fifa96_surface *s);
int fifa96_frontend_run_input(struct fifa96_frontend_run *fr, const fifa96_platform_key *keys,
                              size_t count);
int fifa96_frontend_run_step(struct fifa96_frontend_run *fr, struct fifa96_surface *s, int *quit);
```

Implementation notes: `init` calls `fifa96_frontend_init` and seeds `entry_state` from `fifa96_frontend_entry_state`. `input` maps each `raw_code` to the game key code (the FU-66 mapping table; `FIFA96_ENGINE_KEY_QUIT` is handled by setting the engine quit flag, not queued), appending mapped codes to `queue` and dropping when full. `step` calls `fifa96_frontend_driver`, drains `queue` through `fifa96_frontend_event` with the confirm gate and applies `fifa96_frontend_frontend_result`/`fifa96_frontend_panel_event`/`fifa96_frontend_exit`, derives `phase`/`entry_state`, calls the menu renderer (Task 9 owns the art; Task 8 uses a solid clear as a stub that Task 9 replaces), increments `frames`, and sets `*quit` when the exit state is reached. Engine mode dispatch calls `fifa96_frontend_run_step` when in the front-end mode and treats `*quit` as engine exit.

- [ ] **Step 4: Run tests and gate**

Run: `make check && ./build/test_engine_frontend`
Expected: `test_engine_frontend OK`, `100% tests passed` (88 with SDL3).

- [ ] **Step 5: Commit**

```bash
git add src/fifa96_engine/fifa96_frontend_run.c src/fifa96_engine/fifa96_frontend_run.h \
        src/fifa96_engine/fifa96_engine.c tests/test_engine_frontend.c CMakeLists.txt
git commit -m "feat(engine): front-end control flow and input mapping"
```

---

### Task 9: Front-end screen rendering (visual reconstruction)

**Files:**
- Create: `src/fifa96_engine/fifa96_menu_art.c`
- Create: `include/fifa96_engine/fifa96_menu_art.h`
- Create: `tests/test_engine_menu_art.c`
- Modify: `src/fifa96_engine/fifa96_frontend_run.c`, `CMakeLists.txt`
- Read: `docs/ghidra/FU65_competition_screens.md`, `FU66_frontend_dispatch.md`, `FU56_modex_blit.md`, `FU98_kit_remap_shade_cube.md`

**Interfaces:**
- Consumes: `fifa96_sprite`, `fifa96_bigf`, `fifa96_blit`, `fifa96_render` (existing).
- Produces: `fifa96_menu_art_draw(struct fifa96_surface *, const struct fifa96_menu_state *)`.

- [ ] **Step 1: Ghidra evidence pass (bounded)**

Derive from the native program: for the front-end entry states (`fifa96_frontend_dispatch_resolve` table, FU-66), the background/sprite asset path and the menu row placement. Produce `docs/ghidra/FU135_frontend_menu_render.md` (new number) with: the state table, the asset paths, the row geometry, and the cursor blit calls, every claim cited to addresses/tool output. If geometry cannot be statically proven, record it as an open leg and use the DOSBox-X screenshot oracle (non-gating) for placement.

- [ ] **Step 2: Write the failing test**

```c
/* tests/test_engine_menu_art.c — pins a deterministic menu frame hash */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "fifa96_engine/fifa96_menu_art.h"

int main(void) {
  struct fifa96_surface *s = fifa96_surface_create(320, 240);
  struct fifa96_menu_state st = {.entry_state = 0, .selected_row = 0, .cursor_on = 1};
  fifa96_menu_art_init(NULL);            /* no assets -> procedural fallback */
  fifa96_menu_art_draw(s, &st);
  uint64_t h = fifa96_surface_hash(s);
  assert(h == FIFA96_MENU_FALLBACK_HASH);  /* pin after first verified run */
  fifa96_surface_destroy(s);
  puts("test_engine_menu_art OK");
  return 0;
}
```

- [ ] **Step 3: Implement the renderer**

`fifa96_menu_art_draw` composes the background (asset sprite if mounted, else the procedural fallback: palette-indexed solid background + menu bar rectangles), draws the menu rows at the derived geometry, and blits the cursor sprite at `selected_row`. `fifa96_surface_hash` is the same FNV-1a over `indexed` + `palette` used by the null backend. Pin `FIFA96_MENU_FALLBACK_HASH` from the first verified run (`./build/test_engine_menu_art` then copy the printed hash).

- [ ] **Step 4: Wire into the front-end run**

`fifa96_frontend_run_step` builds `struct fifa96_menu_state` from the library state and calls `fifa96_menu_art_draw` instead of the Task-8 stub.

- [ ] **Step 5: Run tests and gate**

Run: `make check && ./build/test_engine_menu_art`
Expected: `test_engine_menu_art OK`, `100% tests passed` (89 with SDL3).

- [ ] **Step 6: Commit**

```bash
git add docs/ghidra/FU135_frontend_menu_render.md \
        src/fifa96_engine/fifa96_menu_art.c src/fifa96_engine/fifa96_menu_art.h \
        src/fifa96_engine/fifa96_frontend_run.c tests/test_engine_menu_art.c CMakeLists.txt
git commit -m "feat(engine): front-end menu rendering with pinned fallback"
```

---

### Task 10: M1 acceptance harness

**Files:**
- Create: `tests/test_engine_m1.c`
- Create: `tests/golden/engine/m1-frames.txt` (hash tape, generated once and reviewed)
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: everything from Tasks 1–9.
- Produces: a headless boot→intro→front-end→navigate→quit tape with pinned hashes.

- [ ] **Step 1: Write the acceptance test**

The test drives the engine with the null backend and a scripted tape: skip intro (CONFIRM), navigate the menu (DOWN×2, CONFIRM, DECLINE), quit (QUIT key). It writes one line per presented frame to stdout as `frame=<n> hash=<hex>` and compares the full transcript against `tests/golden/engine/m1-frames.txt` when `game/FIFAPCCD96.iso` is present; without the ISO it runs the no-assets smoke mode and asserts the same number of presents and a stable hash (self-consistency), then skips the golden comparison.

```c
/* tests/test_engine_m1.c (core loop) */
fifa96_engine *e = /* create with ISO if present, else NULL assets */;
fifa96_engine_boot(e);
while (!fifa96_engine_should_quit(e)) fifa96_engine_step(e);
/* then compare the collected transcript or self-hash */
```

- [ ] **Step 2: Generate and pin the tape**

Run: `./build/test_engine_m1 > tests/golden/engine/m1-frames.txt` (with the ISO present), review the transcript for sanity (frame count, no all-zero hashes), and commit it.

- [ ] **Step 3: Gate + commit**

Run: `make check`
Expected: `100% tests passed` (90 with SDL3).

```bash
git add tests/test_engine_m1.c tests/golden/engine/m1-frames.txt CMakeLists.txt
git commit -m "test(engine): M1 headless acceptance tape with pinned frames"
```

---

## M2 — Playable match

### Task 11: M2 scope probe (action-handler/entity surface)

**Files:**
- Create: `docs/ghidra/FU136_action_handler_port_scope.md`
- Modify: `.superpowers/sdd/<workspace>/progress.md` (controller-ledger ruling)

**Interfaces:**
- Consumes: FU-76/FU-77 dispatch tables, `fifa96_action_handlers`, `fifa96_ball_pairing`, `fifa96_keeper`, `fifa96_outfield`, `fifa96_entity_update`, `fifa96_control`.
- Produces: a task count for M2's entity/action port and the decision **continue** (≤4 tasks) or **split** (>4 tasks, per spec §12).

- [ ] **Step 1: Enumerate the gap.** For each action-handler dispatch row in the native program (cited addresses), classify: already ported (function exists + tested), ported but unwired (function exists, no caller), not ported (needs an RE slice + C function).
- [ ] **Step 2: Write the doc** with one row per handler: `handler | address | classification | evidence | estimated task`.
- [ ] **Step 3: Decide.** Record `M2_SCOPE: continue` (≤4 tasks) or `M2_SCOPE: split` (>4) in the ledger and the doc.
- [ ] **Step 4: Commit.**

```bash
git add docs/ghidra/FU136_action_handler_port_scope.md
git commit -m "docs(fu136): M2 action-handler port scope probe"
```

If the decision is **split**: stop here, write the M2 child plan (its own brainstorm → spec → plan per spec §12), and hand the M1+foundation work over.

---

### Task 12: Match lifecycle wiring

**Files:**
- Create: `src/fifa96_engine/fifa96_match_run.c`
- Create: `include/fifa96_engine/fifa96_match_run.h`
- Create: `tests/test_engine_match_lifecycle.c`
- Modify: `src/fifa96_engine/fifa96_engine.c`, `CMakeLists.txt`

**Interfaces:**
- Consumes: `fifa96_match_lifecycle` (existing), `fifa96_match_pace` (existing), `fifa96_tick` (Task 6).
- Produces: `struct fifa96_match_run`, `fifa96_match_run_begin`, `fifa96_match_run_step`, `fifa96_match_run_end`.

- [ ] **Step 1: Failing test** — begin with selector 0 via a stub backend (counts register/cancel/teardown/post_exit), step until `should_exit`, assert `register_callback` and `teardown` were called once and `fifa96_match_lifecycle_begin` returned OK; re-begin after end returns OK.
- [ ] **Step 2: Run to fail** — missing header.
- [ ] **Step 3: Implement** — the struct holds `fifa96_match_lifecycle`, `fifa96_match_pace`, and the engine backend callbacks (`register_callback` registers a `fifa96_tick` slot; `cancel_callback` cancels it; `teardown` frees match assets; `post_exit` returns to the front-end mode). `begin` wires the backend and calls `fifa96_match_lifecycle_begin`; `step` drives the tick table and lifecycle; `end` calls `fifa96_match_lifecycle_end`.
- [ ] **Step 4: Run tests and gate.**
- [ ] **Step 5: Commit** — `feat(engine): match lifecycle wiring`.

---

### Task 13: Match frame body, pacing and clock state

**Files:**
- Create: `tests/test_engine_match_frame.c`
- Modify: `src/fifa96_engine/fifa96_match_run.c`

**Interfaces:**
- Consumes: `fifa96_match_state`, `fifa96_match_pace` (existing).
- Produces: `fifa96_match_run_frame(struct fifa96_match_run *mr)` at the derived 30 Hz cadence.

- [ ] **Step 1: Failing test** — run 300 ticks (10 s at 30 Hz) and assert `match_state.total_seconds == 10`, `period`/`phase` unchanged, and that `frame_acc` never exceeds one step (no drift): feed the clock 1/30 s exactly.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement** — `mr->state = fifa96_match_state_init()`; each granted pace tick calls `fifa96_match_state_frame(&mr->state, FIFA96_MATCH_STATE_STEP, /*clock_halt=*/0, &period_ended)` and `fifa96_match_state_tick` with the pace/blocked flags; a `period_ended` sets `fifa96_match_lifecycle_mark_over`.
- [ ] **Step 4: Gate + commit** — `feat(engine): match frame body and 30 Hz pacing`.

---

### Task 14: Match input and control slots

**Files:**
- Create: `tests/test_engine_match_input.c`
- Modify: `src/fifa96_engine/fifa96_match_run.c`

**Interfaces:**
- Consumes: `fifa96_input` (existing), `fifa96_control_slot` (existing), `fifa96_platform_key` (Task 1).
- Produces: `fifa96_match_run_input(struct fifa96_match_run *mr, const fifa96_platform_key *keys, size_t n)`.

- [ ] Step 1: Failing test — press directions + CONFIRM + KICK (define `FIFA96_ENGINE_KEY_KICK = 8`, `FIFA96_ENGINE_KEY_PASS = 9`) and assert `fifa96_input_update` edges and the slot's `pressed`/`held` bits match the FU-61 codes.
- [ ] Step 2: Run to fail.
- [ ] Step 3: Implement — map raw codes through the derived FU-61 rows into `fifa96_input_map`, maintain `struct fifa96_input`, and update the player's `fifa96_control_slot` per tick with the FU-70 anim/map tables.
- [ ] Step 4: Gate + commit — `feat(engine): match input mapping and control slots`.

---

### Task 15: Match presentation (camera, scene, sprites, display)

**Files:**
- Create: `tests/test_engine_match_render.c`
- Modify: `src/fifa96_engine/fifa96_match_run.c`
- Read: `docs/ghidra/FU71_camera_track.md`, `FU84_animation.md`, `FU85_match_renderer.md`, `FU88_projection.md`, `FU89_scene_assembly.md`, `FU92_match_window.md`, `FU93_window_zoom.md`, `FU98_kit_remap_shade_cube.md`

**Interfaces:**
- Consumes: `fifa96_camera`, `fifa96_projection`, `fifa96_scene`, `fifa96_render`, `fifa96_sprite`, `fifa96_blit`, `fifa96_window`, `fifa96_match_display`.
- Produces: `fifa96_match_run_render(struct fifa96_match_run *mr, struct fifa96_surface *s)`.

- [ ] Step 1: Failing test — build a deterministic camera + one entity at a fixed position, render one frame, and pin `fifa96_surface_hash` after the first verified run (with a commented derivation of the expected sprite rectangle from the FU-88 projection math).
- [ ] Step 2: Run to fail.
- [ ] Step 3: Implement — per the FU chain: `fifa96_camera_update` → `fifa96_projection_matrix/project` for each lit entity → `fifa96_scene_sort` → `fifa96_render_resolve`/`fifa96_render_place`/`fifa96_render_cover_rect` → `fifa96_blit_modex` into the surface planes (or into `indexed` via a small indexed blit if the modex path cannot target it — pick one and keep the test's expected geometry cited in the doc), then the match window/zoom and display overlays.
- [ ] Step 4: Gate + commit — `feat(engine): match rendering chain`.

---

### Task 16: Entities, ball, action handlers, score/half/end

**Files:**
- Create: `tests/test_engine_match_play.c`
- Modify: `src/fifa96_engine/fifa96_match_run.c`
- Read: `docs/ghidra/FU60_match_frame_body.md`, `FU67_entity_update.md`, `FU73_ball_pairing.md`, `FU74_keeper_dispatch.md`, `FU75_outfield_decide.md`, `FU76_action_handlers.md`, `FU77_locomotion.md`, `FU78_possession_tackle.md`, `FU120_snapshot_sides_ball.md`

**Interfaces:**
- Consumes: all match libraries; output of Task 11 governs scope.
- Produces: a full kickoff → play → score → half/end path.

- [ ] Step 1: Failing test — scripted tape plays a kickoff, moves the controlled player toward the ball, kicks, and asserts the ball's position/velocity changes per the FU-73 tables; then force the period counter to the half boundary and assert `mark_over`/`resolve_over` and lifecycle exit.
- [ ] Step 2: Run to fail.
- [ ] Step 3: Implement — wire the entity pool + update chain (FU-67), ball pairing/kick (FU-73), keeper/outfield dispatch (FU-74/75), possession/tackle (FU-78), and the action-handler bodies scoped by Task 11 (FU-76/77). Score events increment the match state; `period_ended` drives half/end via the lifecycle.
- [ ] Step 4: Gate + commit — `feat(engine): match entities, ball, action handlers, score flow`.

---

### Task 17: M2 acceptance harness

**Files:**
- Create: `tests/test_engine_m2.c`
- Create: `tests/golden/engine/m2-frames.txt`
- Modify: `CMakeLists.txt`

- [ ] Step 1: Write the tape test — boot → skip intro → menu → start match (CONFIRM×n) → kickoff → move → kick → score → run to half → quit; record `frame=<n> hash=<hex>` + `state=<phase>` lines.
- [ ] Step 2: Pin the transcript with the ISO present; review for sanity (score increments, period advances).
- [ ] Step 3: Gate + commit — `test(engine): M2 headless acceptance tape`.

---

### Task 18: M2 gate — docs, review, merge

Controller task (not a subagent implementer): update `README.md` (new engine targets, `make game`, test counts), append the engine sections to `docs/HOST_RUNNER.md` or a new `docs/ENGINE.md`, run the final whole-branch review on the M1+M2 range, adjudicate findings, and finish the branch (git worktree/branch flow per `superpowers:finishing-a-development-branch`). No code steps; the reviewer prompts and ledger live in the SDD workspace.

```bash
git add README.md docs/
git commit -m "docs(engine): native engine targets, make game, verification"
```

---

## Self-Review Notes

- **Spec coverage:** §4 architecture → Tasks 1–3; §5 ABI → Task 1; §6 boot/cache/loop → Tasks 4–6; §7 M1 → Tasks 7–10; §8 M2 → Tasks 12–17; §9 verification → every task's gate plus Tasks 10/17; §10 orchestration → the plan's task order and the controller's serialization rules; §11 risks → R2 handled by Task 11, R1 by Task 9's oracle note, R4 by the null backend, R6 by the optional SDL3 target; §12 → Task 11 is the decomposition gate.
- **Known gap (by design):** Task 9's screen geometry may be runtime-gated; the task requires the derived evidence first and a DOSBox-X oracle, with the procedural fallback pinned so the task is testable either way.
- **Type consistency:** `fifa96_platform_frame`/`fifa96_platform_key`, `fifa96_engine_*`, `fifa96_surface_*`, `fifa96_asset_*`, `fifa96_cache_*`, `fifa96_clock_*`, `fifa96_intro_*`, `fifa96_frontend_run_*`, `fifa96_menu_art_*`, `fifa96_match_run_*` are used with the same names and signatures in every task that touches them.
- **M2 test density:** Tasks 12–17 state concrete assertions rather than full test bodies; implementers must mirror the semantics pinned by the existing `tests/test_match_lifecycle.c`, `test_match_state.c`, `test_match_pace.c`, `test_camera.c`, `test_projection.c`, `test_scene.c`, `test_render.c`, `test_ball_pairing.c`, `test_keeper.c`, `test_outfield.c`, `test_control.c` cases. If a library's tested contract contradicts a described assertion, the library's test wins and the task report must say so.
- **Test-count note:** the counts in each task's gate are the expected running totals; adjust the CMake registration and this plan if a task adds more than one test binary.
