#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_keys.h"
#include "fifa96_engine/fifa96_match_entities.h"
#include "fifa96_engine/fifa96_match_phase_machine.h"
#include "fifa96_engine/fifa96_platform.h"
#include "fifa96_loader/fifa96_camera.h"
#include "fifa96_loader/fifa96_control.h"
#include "fifa96_loader/fifa96_input.h"
#include "fifa96_loader/fifa96_match_display.h"
#include "fifa96_loader/fifa96_match_lifecycle.h"
#include "fifa96_loader/fifa96_match_pace.h"
#include "fifa96_loader/fifa96_match_state.h"
#include "fifa96_loader/fifa96_render.h"
#include "fifa96_loader/fifa96_rng.h"
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

/* Derived period lengths in whole seconds (FU-62 §4.6: the clock compares
 * `period_seconds == [0x5881A]` for periods 0/1 and `[0x5881C]` for periods
 * 2/3, both = `[0x4C1D1]` minutes × 60/20). The default settings half-length
 * index 0 selects 2 minutes (FU-68 §4.1: table flat 0x37170 = {2,4,6,...};
 * `fifa96_settings_defaults` value[0x0E] = 0), which is what begin installs
 * for selector 0 (the menu/boot path, FU-64 §1.1). A non-zero selector takes
 * the `FUN_0004A228 -> FUN_0004B508` reset override 0x3C/0x1E (60 s / 30 s). */
#define FIFA96_MATCH_RUN_PERIOD_SECONDS_DEFAULT 120u
#define FIFA96_MATCH_RUN_EXTRA_SECONDS_DEFAULT 40u
#define FIFA96_MATCH_RUN_PERIOD_SECONDS_RESET 60u
#define FIFA96_MATCH_RUN_EXTRA_SECONDS_RESET 30u

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

/* Minimal derived match record (M2 Task 5 / FU-138 §4, extended by M2 Task 7 /
 * FU-140 §4 and M2 Task 8 / FU-141): the native record is a 0xB2-strided block
 * (FU-137 §3) whose fields the FU-76 §3.1 action-00 body and the FU-79 §7
 * keeper row 1E read and write. The engine binds these fields to the entity
 * pool (`mr->entities`, FU-141): the FU-67 update chain copies one pool record
 * into this staging record, dispatches the record's action code, then drains
 * the requests back into the pool. Offsets are the native record fields: pos
 * +0x59/+0x5D/+0x61, target +0x4D/+0x51/+0x55, timer +0x89/+0x81, active
 * +0x8D, ran +0x9E, stage +0x8F, ball flag +0x9B, control-slot pointer +0x20,
 * slot direction +0x1D/+0x1E -> +0x20/+0x21. `install` is the derived seam
 * request for the FU-137 §2 `FUN_0007D9A4` install, consumed by
 * `fifa96_match_entities_update`; `ran` is set by action 00 and cleared by the
 * same installer drain. `place_x/y/z` + `place_valid` are the row-1E
 * `FUN_000700F4` placement request (`0x15774C/50/54`), consumed by the frame
 * body as the FU-71 `fifa96_camera_init` reset; `helper_request` is the
 * `FUN_0007876C` slot-merge request, consumed by the FU-141 pool merge. FU-142b
 * adds the row-26 fields `stage92` (native +0x92, the 0/1/2 latch), `timer7b`
 * (+0x7B), `lane` (+0x69 dz word) and the `player_d`/`player_e` stand-ins for
 * the native `rec[+4]` descriptor bytes +0xD/+0xE (the roster descriptor is
 * unmodeled; the pool path stages 0). FU-142e adds `distance` (+0x65): the
 * unported FUN_0008D098 pre-switch walk (`0x8D11E`) writes the 0x8DCD4 out
 * triple for every free record before the installer arms, so the frame staging
 * recomputes the word from this dispatch's pos/target and row 2A gates on it. */
struct fifa96_match_run_record {
  int32_t pos_x;
  int32_t pos_y;       /* native +0x5D, FU-140 row 1E placement height */
  int32_t pos_z;
  int32_t target_x;
  int32_t target_y;     /* native +0x51 (row-28 target = pos copies) */
  int32_t target_z;
  int32_t place_x;     /* FU-140 row 1E: native 0x15774C */
  int32_t place_y;     /* native 0x157750 */
  int32_t place_z;     /* native 0x157754 */
  int32_t timer89;
  uint16_t timer81;
  uint16_t delta;      /* FU-62 frame delta, native [0x157A64] */
  uint8_t active;
  uint8_t has_slot;
  uint8_t has_ball;         /* native +0x9B (FU-140 row 1E) */
  uint8_t helper_request;   /* FU-140 row 1E: FUN_0007876C slot-merge request */
  uint8_t controlled;       /* FU-140 row 1E: native [0x157A83] = rec */
  uint8_t stage;            /* native +0x8F stage byte (FU-140 row 1E) */
  uint8_t stage92;          /* native +0x92 stage latch (FU-142b row 26) */
  uint16_t timer7b;         /* native +0x7B (FU-142b row 26) */
  int32_t lane;             /* native +0x69 dz word, sign-extended (FU-142b) */
  int32_t distance;         /* native +0x65 0x8DCD4 out[0] word (FU-142e row 2A) */
  int8_t player_d;          /* rec[+4][+0xD] table index (FU-142b row 26) */
  int8_t player_e;          /* rec[+4][+0xE] stage-0 gate (FU-142b row 26) */
  int8_t dir_x;
  int8_t dir_z;
  int8_t place_offset_x;    /* caller-supplied 0x10F334[type8] (FU-140) */
  int8_t place_offset_z;    /* caller-supplied 0x10F33C[type8] (FU-140) */
  uint8_t place_valid;      /* row 1E: the placement triple is live (FU-141) */
  uint8_t ran;         /* native +0x9E, set by the action-00 body */
  /* FU-139 §9 (Task 11): the current pool record identity and its +0x8B
   * actor-type byte, staged so rows 07/0F can resolve the team candidates and
   * the kick direction table. */
  int32_t entity_id;   /* team*11 + index, or NONE */
  uint8_t actor_type;  /* native +0x8B>>24 */
  uint8_t install;     /* derived install request of the last dispatch, 0 = none */
  /* FU-142d (row 28, Appendix G) staging: the native record +0x8E facing byte
   * and +0x71/+0x73 velocity pair the body writes, the derived scratch gates
   * (native +0xA0..+0xAE, carried by the pool), the team +0x830 flag, the
   * resolved [team+0x831] chosen-record position and the five process globals
   * the body reads (their native producers are unported, OL-56). */
  int32_t vel_x, vel_z;     /* native +0x71/+0x73 (arm-2 zero writes) */
  uint8_t type;             /* native +0x8E>>24 facing octant (prologue face) */
  uint8_t side;             /* team +0x826 side (arm-0 negation gate) */
  uint8_t flag830;          /* team +0x830 (arm-1 gate) */
  uint8_t chosen_ok;        /* derived: team+0x831 resolved to a pool record */
  int32_t chosen_x;         /* resolved [team+0x831]+0x59 x */
  int32_t chosen_y;         /* resolved [team+0x831]+0x5D y */
  int32_t chosen_z;         /* resolved [team+0x831]+0x61 z */
  int32_t scratch_a2;       /* native +0xA2 derived gate A */
  int32_t scratch_a6;       /* native +0xA6 derived gate B */
  int32_t scratch_aa;       /* native +0xAA approach timer A */
  int32_t scratch_ae;       /* native +0xAE approach timer B */
  uint8_t scratch_a0;       /* native +0xA0 RNG bit */
  uint8_t scratch_a1;       /* native +0xA1 RNG bit */
  int32_t global_10f358;    /* native [0x10F358] arm-2 re-roll gate */
  int32_t global_10f35c;    /* native [0x10F35C] arm-2 chase flag */
  int32_t global_10f364;    /* native [0x10F364] arm-0 set-piece x */
  int32_t global_10f368;    /* native [0x10F368] arm-0 set-piece z */
  uint8_t global_157ac2;    /* native [0x157AC2] arm-0 mode byte */
};

struct fifa96_match_run {
  struct fifa96_match_lifecycle lc;
  struct fifa96_match_pace pace;
  struct fifa96_match_state state;               /* match clock/period block */
  uint16_t score[2];                             /* per-side goal words (FU-72 §2.4) */
  struct fifa96_engine *engine;                  /* engine holding this run */
  struct fifa96_match_lifecycle_backend backend; /* engine callbacks or stub */
  uint32_t ticks;                                /* 100 Hz match callback hits */
  uint32_t steps;                                /* run steps since begin */
  int running;                                   /* begin/end balance */
  struct fifa96_input input;                     /* FU-61 player-0 edge/held model */
  uint8_t input_state[FIFA96_INPUT_PLAYERS];     /* last sampled FU-61 state (slot input) */
  fifa96_control_slot slot;                      /* FU-70 slot bound to player 0 */
  struct fifa96_match_run_record record;         /* FU-141 dispatch staging record */
  struct fifa96_match_entities entities;         /* FU-141 entity/ball pool */
  struct fifa96_match_phase_machine phase_machine; /* FU-142a installer-arms machine */
  /* FU-142d: the match RNG (seeded by begin, zerod by init; the native seeds at
   * match init FUN_000493A0/0x493F2 with settings[0x18] via FUN_0001D940, whose
   * value/producer is unported — the derived seed is 0, OL-56) and the five
   * row-28 process globals, staged to zero until their native producers (the
   * FUN_0008D098 entry block/row 2A/installer clear) are modelled (OL-56). */
  struct fifa96_rng rng;
  int32_t global_10f358;
  int32_t global_10f35c;
  int32_t global_10f364;
  int32_t global_10f368;
  uint8_t global_157ac2;
  /* FU-139 §11 (Task 13): the native [0x157A4F] frame toggle
   * (FUN_0004B100 0x4B11A `XOR AH,1` / 0x4B129 store; cleared by the match
   * reset FUN_0004B02C 0x4B038). Row 06's claim arm and the second-half
   * searches run only on the odd frames; the derived frame body toggles it
   * once per granted 30 Hz frame before the entity chain. */
  uint8_t pass_parity;
  /* Task 15 / M2-B observability: bit `c` is set when action code `c`
   * dispatched FIFA96_OK through fifa96_match_dispatch_action during this run
   * (the acceptance tape reads the wired-row dispatch set from it). Pure
   * bookkeeping — no handler behavior reads or depends on it. Reset by
   * init/begin like the other per-match counters. */
  uint64_t dispatched_ok;
  struct fifa96_match_run_render render;         /* Task 15 presentation state */
  void *stage_owner;                             /* Task 2 staging arena (owned) */
};

/* Zero-init a run: lifecycle, pace, match state (clock and score pair), input
 * model, control slot, the dispatch staging record (FU-141), the entity/ball
 * pool (init seeds the native record reset), the FU-142a installer-arms
 * machine (zeroed, chosen831 seeded NONE), presentation state
 * (camera/window/display/scene, rendering disabled), the staging-arena holder
 * (assigned NULL, never freed: init accepts uninitialized memory, so it cannot
 * trust the holder), backend, counters and engine linkage. Must be called
 * before the first begin on a run. A staged run must be released by end (or the
 * next begin) before re-initialization; a direct re-init leaves the arena
 * unreachable and leaks it. NULL is a no-op. */
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
 * clock (including the score pair), counters, input model, control slot and
 * presentation state for a fresh match (rendering disabled; the full-surface
 * FU-92 window is sized from the engine surface), install the derived FU-62
 * period lengths for the selector (see the FIFA96_MATCH_RUN_*_SECONDS_*
 * constants), and start the lifecycle. Returns 0,
 * -FIFA96_ERR_INVALID (NULL arguments), -FIFA96_ERR_STATE (unbooted/QUIT
 * engine, run already live, or another run live on the engine), or the
 * lifecycle's register failure. */
int fifa96_match_run_begin(struct fifa96_match_run *mr, struct fifa96_engine *eng,
                           uint32_t selector);

/* Write the FU-62 §4.6 period lengths the clock's period-end check reads
 * ([0x5881A] regular periods 0/1, [0x5881C] extra periods 2/3), in whole
 * seconds. Values are written verbatim (zero reproduces the state-init
 * end-on-first-second behavior); a live match normally keeps begin's derived
 * default. Returns 0, -FIFA96_ERR_INVALID (NULL) or -FIFA96_ERR_STATE (run
 * not live). */
int fifa96_match_run_set_period(struct fifa96_match_run *mr, uint16_t period_seconds,
                                uint16_t extra_seconds);

/* Derived score plumbing: increment one side's goal word, the FU-72 §2.4
 * `FUN_00093944` write (`INC word [side*2 + 0x57AC5]`). side 0/1; begin and
 * teardown reset the pair. The original's trigger is the eleven FUN_00093944
 * call sites in the not-yet-ported action/phase handler cluster (FU-72 §2.4
 * errata / FU-142 Appendix I.10 census), so only the derived increment is
 * exposed here — wiring an event source is an open leg for the G2/G3 handler
 * tasks. Returns 0, -FIFA96_ERR_INVALID (NULL or
 * side > 1), or -FIFA96_ERR_STATE (run not live). */
int fifa96_match_run_add_goal(struct fifa96_match_run *mr, uint32_t side);

/* Exactly one 10 ms/100 Hz pace tick of the match frame body. Feeds the FU-60
 * pace (blocked = 0, clock_halt = 0): 1 = the pace granted a 30 Hz frame, the
 * match state advanced one 0x200 step, the controlled player's FU-70 slot was
 * updated once with the FU-62 §4.3 whole frame delta
 * (`state.frame_delta`, 2 at the 0x200 step = 60 counter units/s), the
 * FU-71 camera/display blocks advanced with the same delta (FU-71's
 * FUN_000736AC runs from the frame body FUN_0004B100, not the render driver),
 * and the FU-141 entity/ball pool ran the FU-67 chain (team 0, team 1, the
 * ball pairing) with the FU-137 action dispatch bound to the pool records;
 * then, when the phase is 0x13/0x14, the FU-142a
 * `fifa96_match_phase_machine_step` ran the FUN_0008D098 installer arms once
 * (the FUN_000740A0 order);
 * 0 = no frame was due; or a -fifa96_err_t. A period end marks the lifecycle
 * over, except when an exit is already staged: the staged EXIT wins because
 * the frame body runs in the clock advance before the exit step consumes it.
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

/* Stage the match sprite assets for rendering (M2 Task 2). Reads the two
 * caller-supplied ISO bank container paths through the run engine's mounted
 * asset table (`fifa96_asset_lookup` + `fifa96_cache_get`), decodes each to
 * its SHPI sprite banks (FU-86: a raw .fsh slice or a nested
 * huff(0x31)->refpack(0x10)->tree(0x46) .qfs chain; the first stage is fed the
 * container tail because the original huff reader is unbounded, FU-86 §3
 * caveat/leg 9), then fills `render.frames` (FU-84 §4 5-byte records: duration
 * 0x50, aux 0, sprite = frame index — the row-1 walk evidence; the table spans
 * the resolver's whole 128-index signed-byte domain so no accepted frame index
 * can read past it; the real per-row tables live in the executable's object-4
 * data, FU-84 §3.1/§4, not on the ISO, open leg), `render.banks` (one record
 * per BIGF entry, in container order; the FU-84 row +8 index selects the
 * player container's slots; an entry the port cannot decode stays an unloaded
 * NULL-handle slot, FU-85 §1.2; the FU-86 §4.1 stride switch is indexed
 * container-locally because only the player container's animator indices are
 * derived) and `render.sprite_data` (one owned arena backing frames/banks), and
 * sizes the FU-92 window to the full surface `s`. `render.enabled` becomes 1
 * only after the whole stage succeeds.
 *
 * Bank identity: FU-86 names art/playart.pvi as the player animation container
 * (91 banks, its data cross-check quotes /ART/PLAYART.PVI from the retail ISO)
 * and art/gameart0.pvi as the match art container (its cross-check quotes
 * /ART/GAMEART0.PVI); which of the second container's banks serve the pitch
 * stage is not derivable from the FU docs (numbered open leg), so the caller
 * supplies both paths, as the interface requires.
 *
 * Idempotent: a re-stage builds the replacement arena first and swaps it in
 * only on success. A failure leaves the run bit-identical (the previous stage,
 * if any, stays live). The arena is owned by the run and released by the next
 * successful stage, by begin (which resets presentation) and by end.
 *
 * Returns 0, -FIFA96_ERR_INVALID (NULL mr/s/path), -FIFA96_ERR_STATE (the run
 * is not live, or is not bound to a booted engine asset table),
 * -FIFA96_ERR_NOT_FOUND (a bank path is absent from the table), or
 * -FIFA96_ERR_UNSUPPORTED (a container is present but the port cannot reach
 * sprite banks in it). */
int fifa96_match_run_stage(struct fifa96_match_run *mr, const struct fifa96_surface *s,
                           const char *player_bank, const char *pitch_bank);

/* Resolve the post-period screen chain (FU-64 §6): OVER -> POST
 * (`resolve_over`, the `[0x5FFC]=3` period resolution), POST -> EXIT (the
 * leave staging `[0x5FFC]=4`), then, when EXIT is current, run the existing
 * run_end path (pace hold + register cancel + teardown + post-exit) so the
 * engine returns to FRONTEND. A screen that is still ACTIVE is a no-op
 * (nothing has ended). Returns the run_end result (1 = post-exit ran, 0 =
 * plain teardown), 0 when there was no OVER/POST/EXIT screen to resolve,
 * -FIFA96_ERR_INVALID (NULL) or -FIFA96_ERR_STATE (run not live). */
int fifa96_match_run_resolve(struct fifa96_match_run *mr);

/* One engine step of the run: drives the lifecycle only (the frame body runs
 * on the registered 100 Hz tick hook). 0 while live; the end result
 * (1 = post-exit) when the step exits, either from a staged EXIT or from a
 * period-end OVER, which this step drives through fifa96_match_run_resolve
 * (OVER -> POST -> EXIT -> end, compressed into this step; POST screen pacing
 * is an open leg); or a -fifa96_err_t. After the step has ended the run,
 * further steps return -FIFA96_ERR_STATE. */
int fifa96_match_run_step(struct fifa96_match_run *mr);

/* Tear the match down through the lifecycle and clear the engine linkage.
 * 0 = plain teardown, 1 = post-exit ran, or a -fifa96_err_t. */
int fifa96_match_run_end(struct fifa96_match_run *mr);
