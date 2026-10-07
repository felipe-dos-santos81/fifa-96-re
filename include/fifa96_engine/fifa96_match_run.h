#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_keys.h"
#include "fifa96_engine/fifa96_platform.h"
#include "fifa96_loader/fifa96_camera.h"
#include "fifa96_loader/fifa96_control.h"
#include "fifa96_loader/fifa96_input.h"
#include "fifa96_loader/fifa96_match_display.h"
#include "fifa96_loader/fifa96_match_lifecycle.h"
#include "fifa96_loader/fifa96_match_pace.h"
#include "fifa96_loader/fifa96_match_state.h"
#include "fifa96_loader/fifa96_render.h"
#include "fifa96_loader/fifa96_window.h"

/* Engine-level match driver (M2 foundation): owns the FU-64 lifecycle and the
 * FU-60 pace, the FU-61 input model and the controlled player's FU-70 control
 * slot, and binds them to an engine.
 *
 * Lifecycle: always call fifa96_match_run_init before first use — begin reads
 * the struct, so a non-initialized run (including one with a stale function
 * pointer in `backend`) is undefined behavior. begin is the only start and
 * end (or the exit-step implicit end) the only stop; the begin/end balance is
 * tracked in `running`.
 *
 * Ownership: the run passed to begin stays caller-owned and must remain alive
 * until fifa96_match_run_end returns or fifa96_engine_destroy runs, whichever
 * comes first. The engine ends a live run at destroy before freeing itself,
 * so a running run must outlive the engine call.
 *
 * Backend injection: `backend` doubles as the seam for headless tests. begin
 * installs the engine-backed callbacks when all four are zeroed, uses a
 * complete caller-supplied backend verbatim, and replaces an incomplete one
 * (any of the four NULL) with the engine defaults; a custom backend must
 * therefore provide all four callbacks. */
struct fifa96_engine;
struct fifa96_surface;

/* Match presentation slots: FU-85 §4 stages 23 entities (11 + 11 + ball) into
 * the render arrays; the engine keeps the same staging size. */
#define FIFA96_MATCH_RUN_RENDER_SLOTS 23

/* Engine-side staging record: the FU-85 §4 entity triple/anim/frame/hidden plus
 * the FU-84 row +8 sprite-bank (animator) index the FU-85 resolver consumes. */
struct fifa96_match_run_entity {
  fifa96_render_entity stage;
  uint8_t bank_index;
};

/* Task 15 presentation state, all caller-owned and reset by init/begin:
 *  - camera    FU-71 follow core (position/velocity/timer), advanced once per
 *              granted 30 Hz frame with the frame delta;
 *  - yaw/pitch FU-88 view record angles (obj[3]/obj[4]); the view matrix is
 *              `fifa96_projection_matrix(yaw, pitch)`;
 *  - window    FU-92 render window/clip + FU-93 window scale (HUD seam);
 *  - display   FU-90 display/suspend block (held; overlays not wired yet);
 *  - entities  the FU-85/89 scene staging and the FU-84/85 sprite assets the
 *              resolver reads (frame table, render banks, fixed 0x60/0x61
 *              banks, composite mirror table), plus the indexed blit remap;
 *  - enabled   opt-in flag: the Task 12-14 engine fixtures never stage a
 *              scene, so rendering stays off until a match sets it. */
struct fifa96_match_run_render {
  int enabled;
  struct fifa96_camera camera;
  int32_t yaw, pitch;
  int view_class;
  int input_bit2;
  struct fifa96_window window;
  fifa96_match_display display;
  int32_t window_scale_x, window_scale_y;   /* FU-93 16.16 zoom scale */
  int window_zoomed;
  uint8_t background;
  struct fifa96_match_run_entity entities[FIFA96_MATCH_RUN_RENDER_SLOTS];
  uint32_t entity_count;
  const uint8_t *frames;                    /* FU-84 frame records (5 B each) */
  const struct fifa96_render_bank *banks;
  uint32_t bank_count;
  const struct fifa96_render_bank *fixed60;
  const struct fifa96_render_bank *fixed61;
  const uint8_t *mirror;                    /* FU-85 §1.3 0x10F2E7 table */
  const uint8_t *sprite_data;               /* backing blob for frame parse */
  uint32_t sprite_data_len;
  uint8_t remap[256];                       /* indexed translation + 0xFF key */
};

struct fifa96_match_run {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct fifa96_match_state state;               /* match clock/period block */
  struct fifa96_engine *engine;                  /* engine holding this run */
  struct fifa96_match_lifecycle_backend backend; /* engine callbacks or stub */
  uint32_t ticks;                                /* 100 Hz match callback hits */
  uint32_t steps;                                /* run steps since begin */
  int running;                                   /* begin/end balance */
  struct fifa96_input input;                     /* FU-61 player-0 edge/held model */
  uint8_t input_state[FIFA96_INPUT_PLAYERS];     /* last sampled FU-61 state (slot input) */
  fifa96_control_slot slot;                      /* FU-70 slot bound to player 0 */
  struct fifa96_match_run_render render;         /* Task 15 presentation state */
};

/* Zero-init a run: lifecycle, pace, match state, input model, control slot,
 * presentation state (camera/window/display/scene, rendering disabled),
 * backend, counters and engine linkage. Must be called before the first begin
 * on a run. NULL is a no-op. */
void fifa96_match_run_init(struct fifa96_match_run *mr);

/* One match input poll: fold the engine key presses in `keys` (codes 1..9;
 * state == 1 only) into the FU-61 keyboard-handler code byte (directions
 * 0x1/0x2/0x4/0x8 = up/down/right/left, buttons 0x10 kick / 0x20 pass), pass
 * it through the FU-61 identity mapping row into the run's fifa96_input
 * edge/held model, and latch the mapped per-player state for the frame body.
 * A key absent from a later call reads as released through the input model's
 * previous-state array. The FU-70 control slot is NOT touched here: its update
 * runs once per granted frame inside fifa96_match_run_frame (FU-70 §1.1).
 * Returns 0, or -FIFA96_ERR_INVALID when `mr` is NULL; NULL `keys` and
 * `count == 0` are tolerated as a no-input poll. */
int fifa96_match_run_input(struct fifa96_match_run *mr, const fifa96_platform_key *keys,
                           size_t count);

/* Wire the run to a booted, non-quitting engine (MATCH mode), reset the match
 * clock, counters, input model, control slot and presentation state for a
 * fresh match (rendering disabled; the full-surface FU-92 window is sized from
 * the engine surface), and start the lifecycle. Returns 0,
 * -FIFA96_ERR_INVALID (NULL arguments), -FIFA96_ERR_STATE (unbooted/QUIT
 * engine, run already live, or another run live on the engine), or the
 * lifecycle's register failure. */
int fifa96_match_run_begin(struct fifa96_match_run *mr, struct fifa96_engine *eng,
                           uint32_t selector);

/* Exactly one 10 ms/100 Hz pace tick of the match frame body. Feeds the FU-60
 * pace (blocked = 0, clock_halt = 0): 1 = the pace granted a 30 Hz frame, the
 * match state advanced one 0x200 step, the controlled player's FU-70 slot was
 * updated once with the FU-62 §4.3 whole frame delta
 * (`state.frame_delta`, 2 at the 0x200 step = 60 counter units/s), and the
 * FU-71 camera/display blocks advanced with the same delta (FU-71's
 * FUN_000736AC runs from the frame body FUN_0004B100, not the render driver);
 * 0 = no frame was due; or a -fifa96_err_t. A period end marks the lifecycle
 * over.
 *
 * Sole driver: the registered 100 Hz tick trampoline (fifa96_match_run_tick),
 * so one call happens per PIT tick. fifa96_match_run_step must NOT call it. */
int fifa96_match_run_frame(struct fifa96_match_run *mr);

/* One match presentation pass into the engine's indexed surface (Task 15):
 * clears the canvas to `render.background`, then recomposes the scene per the
 * FU-85/88/89 chain — FU-88 view matrix from `yaw`/`pitch`, reciprocal divide
 * (surface width/height), per-entity FU-85 §4 staging plus the FU-89 §7
 * jitter, FU-89 depth key seeding/sort and window clip, FU-85 resolver over
 * the caller-supplied FU-84 frame table/banks, the second overlay-frame pass
 * for the composite classes (FU-85 §2 second `FUN_00057080`), signed-scale
 * `fifa96_render_place` pivot placement and `fifa96_render_cover_rect`
 * clipping, and an indexed span blit through `render.remap` (0xFF
 * transparent) into `s->indexed`; the clip is clamped to `s` and presentation
 * stays `fifa96_surface_plane`'s job. Window scale (FU-93) is recomputed into
 * `window_scale_x`/`window_scale_y`/`window_zoomed` for the future HUD seam.
 *
 * Pure recomposition: the camera/display advance once per granted 30 Hz frame
 * in fifa96_match_run_frame, so calling this once per presented engine frame
 * (the MATCH dispatch does) is idempotent and hash-stable.
 *
 * Returns 0 (including the `enabled == 0` no-op), -FIFA96_ERR_INVALID (NULL),
 * or -FIFA96_ERR_STATE when enabled without the sprite assets (frames/banks/
 * sprite_data); no partial canvas is written on validation failure. */
int fifa96_match_run_render(struct fifa96_match_run *mr, struct fifa96_surface *s);

/* One engine step of the run: drives the lifecycle only (the frame body runs
 * on the registered 100 Hz tick hook). 0 while live, the end result
 * (1 = post-exit) on the exit step, or a -fifa96_err_t. */
int fifa96_match_run_step(struct fifa96_match_run *mr);

/* Tear the match down through the lifecycle and clear the engine linkage.
 * 0 = plain teardown, 1 = post-exit ran, or a -fifa96_err_t. */
int fifa96_match_run_end(struct fifa96_match_run *mr);
