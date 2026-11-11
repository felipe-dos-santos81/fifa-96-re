# Replay Trigger Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Arm the live replay from a host key, run the native replay state machine once per granted frame with edge-triggered buttons, and close the replay trigger/state/camera/overlay slice.

**Architecture:** A run-level `fifa96_match_run_replay_begin` sets the native armed state (`0x81`) without inventing a `0x80` store. `fifa96_match_run_input` maps the replay keys to the native button word on rising edges; `fifa96_match_run_frame` consumes that edge once per granted frame through the existing `fifa96_match_run_replay_step`. The engine MATCH dispatch arms the trigger from a new `R`/gamepad-BACK key and leaves ESC to the replay exit while the replay family is active. The recorded-frame ring and the live-match world freeze stay numbered legs.

**Tech Stack:** C11, CMake/CTest, ASan/UBSan, Ghidra MCP evidence on `/FIFA96.EXE`, SDL3.

**Spec:** `docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md` §9, and `port/spec/FU-351_class3_camera.md` §21/§23.

## Global Constraints

- `make check` stays green: strict build (`-Wall -Wextra -Werror`) + full CTest (currently 124/124).
- M1/M2 stay byte-exact; any golden movement is a stop, not a re-pin.
- No completion-number movement: no new native body and no new dispatch row.
- The native arm is `FUN_00064270` → `[0x109A98] = 0x81`, gated by the allocated replay buffer `[0x109AB0]` (`FUN_00063EBC`). The engine has no recorded-frame buffer yet, so the live/ACTIVE run is the modeled gate; the buffer gate is a numbered leg.
- Do not invent a `MOV [0x109A98], 0x80` producer. `begin` stores `0x81` only.
- Host keys: `R`/gamepad BACK arms; in replay, Return = play/pause (`1`), C = camera cycle (`0x20`), Right = step forward (`8`), Left = step back (`4`), Down = slow (`2`), Esc = exit (`0x80`).
- The full `FUN_0003BB1C` match-screen UI loop, the recorded-frame ring (`FUN_00063D34/CBC/6428C`), the live-match world/clock freeze and the `0x5865C` pre-arm audio stay numbered legs.
- Work on `main`; commit per task and push.

## Review Focus

1. **Arm state** — `begin` must store `0x81`, never `0x80`; the first step must reach `0x82` and the source-4 target writer. Pinned by Task 1.
2. **Edge buttons** — the SDL backend re-presents held keys, so a held Return must not toggle pause every frame. The input path stores the rising edge only. Pinned by Task 2.
3. **ESC conflict** — while the replay family is active, ESC exits the replay instead of pausing the match. Pinned by Task 2's engine test.
4. **No tape impact** — no R key in M1/M2; the new key and frame call must not change the goldens. Pinned by Task 3.
5. **Exit restore** — `0x80` restores state 0, clears the HUD arm and returns source 0/index -1. Pinned by Task 1.
6. **Overlay gating** — the replay row appears only in the replay family, not at rest. Pinned by Task 3.

---

### Task 1: Replay begin and the state machine

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_match_run.h` (struct fields and declaration)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`fifa96_match_run_replay_begin`)
- Test: `port/tests/test_engine_match_render.c`

**Interfaces:**
- Consumes: `fifa96_match_run_replay_step`, `fifa96_match_run_replay_camera_set`, `fifa96_match_run_replay_target`, `fifa96_match_run_replay_exit` (existing).
- Produces:
  - `int fifa96_match_run_replay_begin(struct fifa96_match_run *mr);`
  - `uint8_t input_buttons;` and `uint8_t input_buttons_held;` in `struct fifa96_match_run_replay` (the replay input rising-edge word; 0 at rest).

- [ ] **Step 1: Write the failing begin/step test**

Add to `port/tests/test_engine_match_render.c` after `test_replay_target`:

```c
/* FU-351 §9.4: the host trigger arms the native replay state (0x81, never
 * 0x80); the first step runs the 0x81 arm to PLAY and the source-4 target
 * writer, and the native button word drives pause/camera/step/slow/exit. */
static void test_replay_begin_and_step(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  struct fifa96_match_run_replay *r = &mr.render.replay;
  mr.running = 1;
  mr.lc.screen = FIFA96_MATCH_SCREEN_ACTIVE;
  for (int side = 0; side < 2; side++) {
    for (int slot = 0; slot < 16; slot++) mr.roster_order[side][slot] = slot;
    mr.roster_order_ok[side] = 1;
  }
  fifa96_match_run_stats_seed(&mr);
  mr.score[0] = 2;
  mr.score[1] = 1;
  r->goal_count = 3;
  r->goal_side[0] = 0; r->goal_roster[0] = 5;
  r->goal_side[1] = 1; r->goal_roster[1] = 3;
  r->goal_side[2] = 0; r->goal_roster[2] = 7;

  assert(fifa96_match_run_replay_begin(&mr) == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_ARMED);   /* the 0x64270 arm */
  assert(r->hud_armed == 1);
  assert(r->cursor == 0 && r->camera_index == 0);
  assert(fifa96_match_run_replay_step(&mr, 0, 0) == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_PLAY);    /* first step -> 0x82 */
  assert(mr.render.camera_target.source == 4);
  assert(mr.render.camera_target.index == 7);      /* the 0,1,0 ring */

  assert(fifa96_match_run_replay_step(&mr, 1, 0) == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_PAUSE);
  assert(fifa96_match_run_replay_step(&mr, 1, 0) == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_PLAY);

  assert(fifa96_match_run_replay_step(&mr, 0x20, 0) == 0);
  assert(r->camera_index == 1);
  assert(fifa96_match_run_replay_step(&mr, 8, 0) == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_STEP_FWD);
  assert(fifa96_match_run_replay_step(&mr, 0, 0) == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_PLAY);
  assert(fifa96_match_run_replay_step(&mr, 4, 0) == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_STEP_BACK);
  assert(fifa96_match_run_replay_step(&mr, 0, 0) == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_PLAY);
  assert(fifa96_match_run_replay_step(&mr, 2, 0) == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_SLOW);

  assert(fifa96_match_run_replay_step(&mr, 0x80, 0) == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_LIVE);    /* the exit arm */
  assert(r->hud_armed == 0);
  assert(mr.render.camera_target.source == 0);
  assert(mr.render.camera_target.index == -1);
  fifa96_match_run_init(&mr);
}

static void test_replay_begin_edges(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_replay_begin(NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_replay_begin(&mr) == -FIFA96_ERR_STATE);
  mr.running = 1;
  mr.lc.screen = FIFA96_MATCH_SCREEN_OVER;
  assert(fifa96_match_run_replay_begin(&mr) == -FIFA96_ERR_STATE);
  mr.lc.screen = FIFA96_MATCH_SCREEN_ACTIVE;
  mr.paused = 1;
  assert(fifa96_match_run_replay_begin(&mr) == -FIFA96_ERR_STATE);
  mr.paused = 0;
  assert(fifa96_match_run_replay_begin(&mr) == 0);
  fifa96_match_run_init(&mr);
}
```

Register both in `main()` after `test_replay_target_consumes_stats()`.

- [ ] **Step 2: Run the test to verify it fails**

Run: `cmake --build build --target test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_render`
Expected: FAIL — `fifa96_match_run_replay_begin` is undeclared.

- [ ] **Step 3: Add the fields and the function**

In `struct fifa96_match_run_replay` after `activity_count`, add:

```c
  /* The replay input seam: the native button word's rising edge for this
   * input poll (1 play/pause, 2 slow, 4 step back, 8 step forward, 0x20
   * camera, 0x80 exit). `input_buttons_held` is the previous poll's level. */
  uint8_t input_buttons;
  uint8_t input_buttons_held;
```

In `fifa96_match_run.c`, before `fifa96_match_run_replay_target`:

```c
int fifa96_match_run_replay_begin(struct fifa96_match_run *mr) {
  struct fifa96_match_run_replay *r;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running || mr->paused ||
      mr->lc.screen != FIFA96_MATCH_SCREEN_ACTIVE)
    return -FIFA96_ERR_STATE;
  /* FUN_00064270 stores 0x81 when the replay buffer [0x109AB0] is allocated
   * (FUN_00063EBC). The engine's recorded-frame buffer is a later leg, so the
   * live ACTIVE run is the modeled gate; no 0x80 store exists in the native. */
  r = &mr->render.replay;
  r->state = FIFA96_MATCH_REPLAY_ARMED;
  r->hud_armed = 1;
  r->phase_counter = 0;
  r->cursor = 0;
  r->camera_index = 0;
  (void)fifa96_match_run_replay_camera_set(mr, 0);
  return FIFA96_OK;
}
```

Declare it in the header above `fifa96_match_run_replay_step`:

```c
/* FUN_00064270: arm the replay (state 0x81) for a live, unpaused, ACTIVE
 * run. The native buffer gate [0x109AB0] is modeled by the live-run gate
 * (the recorded-frame ring is a later leg). Returns 0, -FIFA96_ERR_INVALID
 * on NULL, -FIFA96_ERR_STATE when the run is not running/ACTIVE or paused. */
int fifa96_match_run_replay_begin(struct fifa96_match_run *mr);
```

- [ ] **Step 4: Run the test**

Run: `cmake --build build --target test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_render`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_match_run.h port/src/fifa96_engine/fifa96_match_run.c port/tests/test_engine_match_render.c
git commit -m "feat(engine): add the replay arm and state machine"
```

---

### Task 2: Host keys, edge buttons and the per-frame step

**Files:**
- Modify: `port/src/fifa96_engine/fifa96_keys.h` (new key code)
- Modify: `port/src/fifa96_engine/platform_sdl3.c` (SDL3 mapping)
- Modify: `port/src/fifa96_engine/fifa96_match_run.c` (`fifa96_match_run_input`, `fifa96_match_run_frame`)
- Modify: `port/src/fifa96_engine/fifa96_engine_internal.h` (the trigger edge latch)
- Modify: `port/src/fifa96_engine/fifa96_engine.c` (MATCH trigger + ESC guard)
- Test: `port/tests/test_engine_match_render.c`, `port/tests/test_engine_match_bridge.c`

**Interfaces:**
- Consumes: Task 1's `fifa96_match_run_replay_begin` and `input_buttons`.
- Produces: `#define FIFA96_ENGINE_KEY_REPLAY 14`; the engine trigger and the edge mapping.

- [ ] **Step 1: Write the failing input/frame test**

Add to `port/tests/test_engine_match_render.c`:

```c
/* The replay input path maps the host keys to the native button word on
 * rising edges and the frame body consumes the edge exactly once. */
static void test_replay_input_edges(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.running = 1;
  mr.lc.screen = FIFA96_MATCH_SCREEN_ACTIVE;
  assert(fifa96_match_run_replay_begin(&mr) == 0);
  fifa96_platform_key keys[3] = {
      {FIFA96_ENGINE_KEY_CONFIRM, 1},
      {FIFA96_ENGINE_KEY_RIGHT, 1},
      {FIFA96_ENGINE_KEY_QUIT, 1},
  };
  assert(fifa96_match_run_input(&mr, keys, 3) == 0);
  assert(mr.render.replay.input_buttons == (1u | 8u | 0x80u));
  /* A held poll produces no new edge. */
  assert(fifa96_match_run_input(&mr, keys, 3) == 0);
  assert(mr.render.replay.input_buttons == 0);
  assert(mr.render.replay.input_buttons_held == (1u | 8u | 0x80u));
  /* Live state clears the seam. */
  mr.render.replay.state = FIFA96_MATCH_REPLAY_LIVE;
  assert(fifa96_match_run_input(&mr, keys, 3) == 0);
  assert(mr.render.replay.input_buttons == 0);
  assert(mr.render.replay.input_buttons_held == 0);
  fifa96_match_run_init(&mr);
}
```

Add the engine trigger test to `port/tests/test_engine_match_bridge.c`:

```c
/* The MATCH dispatch arms the replay from R; a second R while armed does
 * not re-arm, and the trigger is ignored outside ACTIVE play. */
static void test_engine_replay_key_arms(void) {
  const fifa96_platform_key tape[] = {
      {FIFA96_ENGINE_KEY_DECLINE, 1}, {FIFA96_ENGINE_KEY_DECLINE, 0},
      {FIFA96_ENGINE_KEY_CONFIRM, 1}, {FIFA96_ENGINE_KEY_CONFIRM, 0},
      {FIFA96_ENGINE_KEY_REPLAY, 1},  {FIFA96_ENGINE_KEY_REPLAY, 0},
  };
  struct fixture f = make_fixture(tape, sizeof tape / sizeof tape[0], NULL);
  assert(fifa96_engine_step(f.engine) == 0);   /* DECLINE -> panel */
  assert(fifa96_engine_step(f.engine) == 0);   /* release */
  assert(fifa96_engine_step(f.engine) == 0);   /* CONFIRM -> bridge */
  assert(f.engine->mode == FIFA96_ENGINE_MODE_MATCH);
  assert(f.engine->match_run.render.replay.state == FIFA96_MATCH_REPLAY_LIVE);
  assert(fifa96_engine_step(f.engine) == 0);   /* R -> the 0x81 arm */
  assert(f.engine->match_run.render.replay.state == FIFA96_MATCH_REPLAY_ARMED);
  assert(fifa96_engine_step(f.engine) == 0);   /* release: stays in the family */
  assert(fifa96_match_run_replay_family(&f.engine->match_run.render.replay));
  drop_fixture(f);
}
```

Register both in their `main()` functions.

- [ ] **Step 2: Run the tests to verify they fail**

Run: `cmake --build build --target test_engine_match_render test_engine_match_bridge -j"$(nproc)" && ./build/test_engine_match_render && ./build/test_engine_match_bridge`
Expected: FAIL — `FIFA96_ENGINE_KEY_REPLAY` and the edge mapping are undefined.

- [ ] **Step 3: Add the key, the mapping and the edge**

In `fifa96_keys.h` after `FIFA96_ENGINE_KEY_TACKLE`:

```c
/* FU-351 §9: the instant-replay trigger (SDL3 R, gamepad BACK). It never
 * enters the FU-61 action word; the engine MATCH dispatch arms the replay. */
#define FIFA96_ENGINE_KEY_REPLAY 14
```

In `platform_sdl3.c`, add `SDL_SCANCODE_R` and `SDL_GAMEPAD_BUTTON_BACK` cases; change `#define SDL3_HELD_CODES (FIFA96_ENGINE_KEY_TACKLE + 1)` to `(FIFA96_ENGINE_KEY_REPLAY + 1)`.

In `fifa96_match_run_input`, before the normal key fold:

```c
  if (fifa96_match_run_replay_family(&mr->render.replay)) {
    uint8_t held = 0;
    if (keys) {
      for (size_t i = 0; i < count; i++) {
        if (keys[i].state != 1) continue;
        switch (keys[i].raw_code) {
          case FIFA96_ENGINE_KEY_CONFIRM: held |= 1u; break;
          case FIFA96_ENGINE_KEY_DOWN:    held |= 2u; break;
          case FIFA96_ENGINE_KEY_LEFT:    held |= 4u; break;
          case FIFA96_ENGINE_KEY_RIGHT:   held |= 8u; break;
          case FIFA96_ENGINE_KEY_PASS:    held |= 0x20u; break;
          case FIFA96_ENGINE_KEY_QUIT:    held |= 0x80u; break;
          default: break;
        }
      }
    }
    mr->render.replay.input_buttons =
        (uint8_t)(held & (uint8_t)~mr->render.replay.input_buttons_held);
    mr->render.replay.input_buttons_held = held;
    return 0;
  }
  mr->render.replay.input_buttons = 0;
  mr->render.replay.input_buttons_held = 0;
```

In `fifa96_match_run_frame`, at the top of the granted-frame loop:

```c
    if (fifa96_match_run_replay_family(&mr->render.replay))
      (void)fifa96_match_run_replay_step(mr, mr->render.replay.input_buttons,
                                         (int32_t)mr->state.frame_delta);
    mr->render.replay.input_buttons = 0;
```

In `fifa96_engine_internal.h`, next to `match_esc_held`:

```c
  uint8_t match_replay_held;  /* R/BACK level for the replay trigger edge */
```

In `fifa96_engine.c`'s MATCH dispatch, after the poll and before the input call:

```c
    const int replay_down = engine_keys_down(keys, n, FIFA96_ENGINE_KEY_REPLAY);
    const int replay_edge = replay_down && !e->match_replay_held;
    e->match_replay_held = (uint8_t)(replay_down != 0);
    if (e->match && replay_edge &&
        e->match->lc.screen == FIFA96_MATCH_SCREEN_ACTIVE && !e->match->paused &&
        !fifa96_match_run_replay_family(&e->match->render.replay))
      (void)fifa96_match_run_replay_begin(e->match);
```

Guard the live ESC pause block with the replay family so ESC exits the replay
instead of pausing the match:

```c
      if (esc_edge &&
          !fifa96_match_run_replay_family(&e->match->render.replay)) {
```

- [ ] **Step 4: Run the focused tests**

Run: `cmake --build build --target test_engine_match_render test_engine_match_bridge test_engine_sdl3 -j"$(nproc)" && ./build/test_engine_match_render && ./build/test_engine_match_bridge && ./build/test_engine_sdl3`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add port/src/fifa96_engine/fifa96_keys.h port/src/fifa96_engine/platform_sdl3.c port/src/fifa96_engine/fifa96_match_run.c port/src/fifa96_engine/fifa96_engine_internal.h port/src/fifa96_engine/fifa96_engine.c port/tests/test_engine_match_render.c port/tests/test_engine_match_bridge.c
git commit -m "feat(engine): wire the replay trigger and pad edges"
```

---

### Task 3: Overlay, docs and the gate

**Files:**
- Test: `port/tests/test_engine_match_render.c` (the replay row after the trigger)
- Modify: `port/spec/FU-351_class3_camera.md` (a §9 closure erratum)
- Modify: `port/ENGINE.md`, `docs/PROGRESS.md`
- Modify: `docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md` (§9 closure)

**Interfaces:**
- Consumes: Tasks 1–2.
- Produces: the recorded closure and the remaining replay legs.

- [ ] **Step 1: Add the overlay-after-trigger assertion**

Extend `test_replay_begin_and_step` (Task 1) after the first step:

```c
  /* The armed HUD row appears at PLAY; the armed state itself has no row. */
  assert(fifa96_match_run_replay_row(&mr) == FIFA96_MATCH_REPLAY_ROW_HUD);
```

Add the matching pre-step assertion:

```c
  assert(fifa96_match_run_replay_row(&mr) == FIFA96_MATCH_REPLAY_ROW_NONE);
```

Run: `cmake --build build --target test_engine_match_render -j"$(nproc)" && ./build/test_engine_match_render`
Expected: PASS.

- [ ] **Step 2: Update the records**

- FU-351: append a §23.2 erratum recording the `0x64270` arm, the `0x109AB0` buffer-gate model, the host key/button map, the per-frame step, the ESC-exit routing, and the legs (the native `FUN_0004B100` mode-7/`FUN_000510DC` request path, the `FUN_0003BB1C` UI loop, the recorded-frame ring, the world/clock freeze, the `0x5865C` pre-arm audio).
- ENGINE and PROGRESS: phase 116 with the trigger, the state machine, the button map, the tests and the gate.
- Design §9: append a §9.5 closure record with the same scope and legs.
- Do not claim a completion-number change.

- [ ] **Step 3: Run the full gate**

Run: `make check` → 124/124.
Run the M1/M2 `cmp` → exact.
Run the windowed smoke: `timeout 20s env DISPLAY=:1 make game` → exit 124 with a clean log.

- [ ] **Step 4: Commit and push**

```bash
git add port/tests/test_engine_match_render.c port/spec/FU-351_class3_camera.md port/ENGINE.md docs/PROGRESS.md docs/superpowers/specs/2026-11-10-five-open-items-closure-design.md
git commit -m "docs(fu351): record the replay trigger"
git push origin main
```
