#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_keys.h"
#include "fifa96_engine/fifa96_platform.h"
#include "fifa96_loader/fifa96_control.h"
#include "fifa96_loader/fifa96_input.h"
#include "fifa96_loader/fifa96_match_lifecycle.h"
#include "fifa96_loader/fifa96_match_pace.h"
#include "fifa96_loader/fifa96_match_state.h"

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
};

/* Zero-init a run: lifecycle, pace, match state, input model, control slot,
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
 * clock, counters, input model and control slot for a fresh match, and start
 * the lifecycle. Returns 0, -FIFA96_ERR_INVALID (NULL arguments),
 * -FIFA96_ERR_STATE (unbooted/QUIT engine, run already live, or another run
 * live on the engine), or the lifecycle's register failure. */
int fifa96_match_run_begin(struct fifa96_match_run *mr, struct fifa96_engine *eng,
                           uint32_t selector);

/* Exactly one 10 ms/100 Hz pace tick of the match frame body. Feeds the FU-60
 * pace (blocked = 0, clock_halt = 0): 1 = the pace granted a 30 Hz frame, the
 * match state advanced one 0x200 step and the controlled player's FU-70 slot
 * was updated once with the FU-62 §4.3 whole frame delta
 * (`state.frame_delta`, 2 at the 0x200 step = 60 counter units/s); 0 = no
 * frame was due; or a -fifa96_err_t. A period end marks the lifecycle over.
 *
 * Sole driver: the registered 100 Hz tick trampoline (fifa96_match_run_tick),
 * so one call happens per PIT tick. fifa96_match_run_step must NOT call it. */
int fifa96_match_run_frame(struct fifa96_match_run *mr);

/* One engine step of the run: drives the lifecycle only (the frame body runs
 * on the registered 100 Hz tick hook). 0 while live, the end result
 * (1 = post-exit) on the exit step, or a -fifa96_err_t. */
int fifa96_match_run_step(struct fifa96_match_run *mr);

/* Tear the match down through the lifecycle and clear the engine linkage.
 * 0 = plain teardown, 1 = post-exit ran, or a -fifa96_err_t. */
int fifa96_match_run_end(struct fifa96_match_run *mr);
