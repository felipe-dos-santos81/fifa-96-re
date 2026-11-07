/* port/tests/test_engine_sdl3.c — SDL3 backend smoke test.
 *
 * Runs only when SDL3 is present (CMake guards the target and CTest entry on
 * SDL3_FOUND). The dummy video/audio drivers keep it headless: SDL3 reads
 * SDL_VIDEO_DRIVER/SDL_AUDIO_DRIVER, SDL2's spelling SDL_VIDEODRIVER/
 * SDL_AUDIODRIVER is set too so the harness behaves under either era. The
 * dummy video driver picks the software renderer, so create/init/present
 * exercise the full backend path without a display.
 *
 * Also pins the pure scancode mapping and proves the M2 interactive match
 * start is keyboard-reachable: BACKSPACE (DECLINE) opens the front-end panel,
 * RETURN (CONFIRM) accepts it, and the bridge leaves the engine in MATCH.
 *
 * T3 (OL-T4-1): a physically held key is presented as a per-poll state sample
 * (the engine's `input_state` model), so a single KEY_DOWN with no repeats
 * keeps the held code latched and moves the controlled record across frames;
 * the KEY_UP ends the sample.
 *
 * P9: the held-state set includes LONG_BALL (10), so V / pad West holds like
 * KICK/PASS instead of pulsing once and deriving a release on the next poll.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_keys.h"
#include "fifa96_engine/fifa96_match_entities.h"
#include "fifa96_engine/fifa96_platform.h"

/* Pure scancode/button mappings from the backend source (no SDL video/audio
 * or physical pad needed). */
extern int fifa96_platform_sdl3_map_scancode(int scancode);
extern int fifa96_platform_sdl3_map_button(int button);

static void test_scancode_map(void) {
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_UP) == FIFA96_ENGINE_KEY_UP);
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_DOWN) == FIFA96_ENGINE_KEY_DOWN);
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_LEFT) == FIFA96_ENGINE_KEY_LEFT);
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_RIGHT) == FIFA96_ENGINE_KEY_RIGHT);
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_RETURN) == FIFA96_ENGINE_KEY_CONFIRM);
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_BACKSPACE) == FIFA96_ENGINE_KEY_DECLINE);
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_X) == FIFA96_ENGINE_KEY_DECLINE);
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_Z) == FIFA96_ENGINE_KEY_KICK);
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_C) == FIFA96_ENGINE_KEY_PASS);
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_V) == FIFA96_ENGINE_KEY_LONG_BALL);
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_S) == FIFA96_ENGINE_KEY_SUBST);
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_ESCAPE) == FIFA96_ENGINE_KEY_QUIT);
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_A) == 0);
  assert(fifa96_platform_sdl3_map_scancode(SDL_SCANCODE_UNKNOWN) == 0);
}

/* P8: the pad's South/East codes are shared with the front-end (CONFIRM/
 * DECLINE) and the match input maps those same codes to KICK/PASS. */
static void test_button_map(void) {
  assert(fifa96_platform_sdl3_map_button(SDL_GAMEPAD_BUTTON_DPAD_UP) ==
         FIFA96_ENGINE_KEY_UP);
  assert(fifa96_platform_sdl3_map_button(SDL_GAMEPAD_BUTTON_DPAD_DOWN) ==
         FIFA96_ENGINE_KEY_DOWN);
  assert(fifa96_platform_sdl3_map_button(SDL_GAMEPAD_BUTTON_DPAD_LEFT) ==
         FIFA96_ENGINE_KEY_LEFT);
  assert(fifa96_platform_sdl3_map_button(SDL_GAMEPAD_BUTTON_DPAD_RIGHT) ==
         FIFA96_ENGINE_KEY_RIGHT);
  assert(fifa96_platform_sdl3_map_button(SDL_GAMEPAD_BUTTON_SOUTH) ==
         FIFA96_ENGINE_KEY_CONFIRM);
  assert(fifa96_platform_sdl3_map_button(SDL_GAMEPAD_BUTTON_EAST) ==
         FIFA96_ENGINE_KEY_DECLINE);
  assert(fifa96_platform_sdl3_map_button(SDL_GAMEPAD_BUTTON_WEST) ==
         FIFA96_ENGINE_KEY_LONG_BALL);
  assert(fifa96_platform_sdl3_map_button(SDL_GAMEPAD_BUTTON_NORTH) ==
         FIFA96_ENGINE_KEY_SUBST);
  assert(fifa96_platform_sdl3_map_button(SDL_GAMEPAD_BUTTON_START) ==
         FIFA96_ENGINE_KEY_QUIT);
}

static void push_key_down(SDL_Scancode sc) {
  SDL_Event ev;
  SDL_zero(ev);
  ev.type = SDL_EVENT_KEY_DOWN;
  ev.key.scancode = sc;
  ev.key.down = true;
  assert(SDL_PushEvent(&ev));
}

static void push_key_up(SDL_Scancode sc) {
  SDL_Event ev;
  SDL_zero(ev);
  ev.type = SDL_EVENT_KEY_UP;
  ev.key.scancode = sc;
  ev.key.down = false;
  assert(SDL_PushEvent(&ev));
}

/* Interactive reachability with the keyboard alone: BACKSPACE enters the
 * panel, RETURN drives the panel confirm, the bridge classifies the match
 * start and the engine owns a live run in MATCH mode. */
static void test_keyboard_reaches_match(void) {
  fifa96_platform *p = fifa96_platform_sdl3_create();
  assert(p != NULL);
  struct fifa96_engine_config cfg = {.iso_path = NULL, .width = 320,
                                     .height = 240, .headless = 1};
  struct fifa96_engine *e = fifa96_engine_create(&cfg, p);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);
  assert(e->mode == FIFA96_ENGINE_MODE_FRONTEND);

  push_key_down(SDL_SCANCODE_BACKSPACE);
  assert(fifa96_engine_step(e) == 0);
  assert(e->frontend.phase == FIFA96_FRONTEND_PHASE_PANEL);

  push_key_down(SDL_SCANCODE_RETURN);
  assert(fifa96_engine_step(e) == 0);
  assert(e->frontend.match_start == 0);   /* consumed by the bridge */
  assert(e->mode == FIFA96_ENGINE_MODE_MATCH);
  assert(e->match == &e->match_run);
  assert(e->match->running == 1);

  fifa96_engine_destroy(e);
  fifa96_platform_destroy(p);
}

/* T3 (OL-T4-1): one SDL KEY_DOWN (auto-repeat filtered as before) is a held
 * state sample, not a single pulse. The backend re-presents every held key on
 * every poll, matching the engine's per-poll `input_state` model (and the
 * native make/break flag array), so the controlled record keeps moving for as
 * long as the key is down. At BASE the second poll returned an empty batch,
 * `fifa96_input_update` derived the release and `input_state[0]` fell to 0. */
static void test_held_key_is_a_state_sample(void) {
  fifa96_platform *p = fifa96_platform_sdl3_create();
  assert(p != NULL);
  struct fifa96_engine_config cfg = {.iso_path = NULL, .width = 320,
                                     .height = 240, .headless = 1};
  struct fifa96_engine *e = fifa96_engine_create(&cfg, p);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);

  push_key_down(SDL_SCANCODE_BACKSPACE);
  assert(fifa96_engine_step(e) == 0);
  push_key_down(SDL_SCANCODE_RETURN);
  assert(fifa96_engine_step(e) == 0);
  assert(e->mode == FIFA96_ENGINE_MODE_MATCH);

  {
    struct fifa96_match_run *mr = &e->match_run;
    int32_t id = mr->slot.entity;
    int32_t start_z;
    /* Isolate the row-00 pad path from the live situation-0 kickoff cutscene
     * (the rationale shared with `begin_isolated` in
     * port/tests/test_engine_match_frame.c; this test enters MATCH via the engine
     * step, not `fifa96_match_run_begin`, so that helper does not apply). */
    mr->act.id = 0;
    assert(id >= 0 && id < (int32_t)FIFA96_MATCH_ENTITY_RECORDS);
    assert(fifa96_match_entities_install(&mr->entities.team[0].records[id],
                                         (uint8_t)mr->state.phase, 0, 0) == 1);
    start_z = mr->entities.team[0].records[id].pos_z;

    push_key_down(SDL_SCANCODE_RIGHT);   /* one press; no repeat events */
    for (int i = 0; i < 10; i++) {
      /* Real SDL clock: space the steps by a PIT tick so the match frame body
       * grants and runs the mover. */
      SDL_DelayNS(12000000);
      assert(fifa96_engine_step(e) == 0);
      /* The still-held menu RETURN/BACKSPACE do not become KICK/PASS (P8). */
      assert(mr->input_state[0] == 0x04);   /* held across every poll */
    }
    assert(mr->input.held == 0x04);
    {
      /* RIGHT maps to slot dir (0, -1) -> the row-00 target pos_z - 0x80. */
      int32_t mid_z = mr->entities.team[0].records[id].pos_z;
      assert(mid_z < start_z);
      for (int i = 0; i < 10; i++) {
        SDL_DelayNS(12000000);
        assert(fifa96_engine_step(e) == 0);
        assert(mr->input_state[0] == 0x04);
      }
      /* Still moving while held; a single pulse would have decayed to rest. */
      assert(mr->entities.team[0].records[id].pos_z < mid_z);
    }

    push_key_up(SDL_SCANCODE_RIGHT);
    SDL_DelayNS(12000000);
    assert(fifa96_engine_step(e) == 0);
    assert(mr->input_state[0] == 0);
    assert(mr->input.prev[0] == 0);
  }

  fifa96_engine_destroy(e);
  fifa96_platform_destroy(p);
}

/* True when `code` appears in the poll batch with exactly `state`. */
static int key_batch_has(const fifa96_platform_key *keys, int count,
                         int32_t code, int32_t state) {
  for (int i = 0; i < count; i++)
    if (keys[i].raw_code == code && keys[i].state == state) return 1;
  return 0;
}

/* P9: the held-state sample covers the whole match action block, LONG_BALL
 * (10) included. BASE stopped the scan at `PASS + 1`, so a held V / pad West
 * was pushed only on its press poll; `fifa96_input_update` then saw an empty
 * batch and derived a release, i.e. the long ball behaved as a one-shot pulse
 * unlike KICK/PASS. The poll path needs no physical pad: SDL events are
 * pushed directly and the pure backend sample is inspected. */
static void test_held_long_ball_is_a_state_sample(void) {
  fifa96_platform *p = fifa96_platform_sdl3_create();
  assert(p != NULL);
  assert(p->init(p->self, 320, 240, "test") == 0);

  fifa96_platform_key keys[32];
  int count = 0;

  push_key_down(SDL_SCANCODE_Z);
  push_key_down(SDL_SCANCODE_C);
  push_key_down(SDL_SCANCODE_V);

  assert(p->poll(p->self, keys, 32, &count) == 0);
  assert(key_batch_has(keys, count, FIFA96_ENGINE_KEY_KICK, 1));
  assert(key_batch_has(keys, count, FIFA96_ENGINE_KEY_PASS, 1));
  assert(key_batch_has(keys, count, FIFA96_ENGINE_KEY_LONG_BALL, 1));

  /* Still held: every poll re-presents all three, exactly like KICK/PASS. */
  for (int i = 0; i < 4; i++) {
    assert(p->poll(p->self, keys, 32, &count) == 0);
    assert(count == 3);
    assert(key_batch_has(keys, count, FIFA96_ENGINE_KEY_KICK, 1));
    assert(key_batch_has(keys, count, FIFA96_ENGINE_KEY_PASS, 1));
    assert(key_batch_has(keys, count, FIFA96_ENGINE_KEY_LONG_BALL, 1));
  }

  /* The lifted long ball rides its KEY_UP release edge, then disappears
   * while KICK/PASS continue to be held. */
  push_key_up(SDL_SCANCODE_V);
  assert(p->poll(p->self, keys, 32, &count) == 0);
  assert(count == 3);
  assert(key_batch_has(keys, count, FIFA96_ENGINE_KEY_KICK, 1));
  assert(key_batch_has(keys, count, FIFA96_ENGINE_KEY_PASS, 1));
  assert(key_batch_has(keys, count, FIFA96_ENGINE_KEY_LONG_BALL, 0));

  assert(p->poll(p->self, keys, 32, &count) == 0);
  assert(count == 2);
  assert(key_batch_has(keys, count, FIFA96_ENGINE_KEY_KICK, 1));
  assert(key_batch_has(keys, count, FIFA96_ENGINE_KEY_PASS, 1));
  assert(!key_batch_has(keys, count, FIFA96_ENGINE_KEY_LONG_BALL, 0));
  assert(!key_batch_has(keys, count, FIFA96_ENGINE_KEY_LONG_BALL, 1));

  push_key_up(SDL_SCANCODE_Z);
  push_key_up(SDL_SCANCODE_C);
  p->shutdown(p->self);
  fifa96_platform_destroy(p);
}

int main(void) {
  /* SDL2 and SDL3 spellings; the backend honours SDL3's hint name too. */
  setenv("SDL_VIDEO_DRIVER", "dummy", 1);
  setenv("SDL_AUDIO_DRIVER", "dummy", 1);
  setenv("SDL_VIDEODRIVER", "dummy", 1);
  setenv("SDL_AUDIODRIVER", "dummy", 1);

  test_scancode_map();
  test_button_map();
  test_keyboard_reaches_match();
  test_held_key_is_a_state_sample();
  test_held_long_ball_is_a_state_sample();

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
