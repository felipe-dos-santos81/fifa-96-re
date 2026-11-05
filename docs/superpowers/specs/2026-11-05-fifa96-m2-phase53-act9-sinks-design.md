# M2 phase-53 — the act-9 stage-0 sinks (FU181-L5)

**Status:** proposed 2026-11-05. Supersedes no prior section; the phase-52
register stands as written.

## 1. The goal

Close FU181-L5: the native act-9 stage-0 body `0x88F81..0x88FE7` runs four
presentation/placement sinks the port still lacks, in this order:

1. `0x700F4(0,0,0)` — the camera reset (the engine has `fifa96_camera_init`
   but it is not wired here and its pan constants are zeroed rather than
   `10/16/24`);
2. `0x73E28` — the match/camera state reset (unported);
3. `0x740A0(0x15, side)` — the phase write (**landed**, phase 41);
4. `0x73E08` — the placement commit (the engine has the kickoff variant but
   it hard-seeds the act-1 ball spawn `0x1E0/0`; act 9 needs the non-seeding
   copy of the staged `0x158830/34/38` target);
5. `0x4C324` — the presentation bind (the arithmetic exists in
   `fifa96_action_duel_bind`, but the `[0x1074A4]` cell is missing).

The native falls through into stage 1 (`0x88FED`) and can walk stages 0→1→2→3
in one call; the port's stage-1/2 compression stays (FU170-L1).

The census does not move: no dispatch body or slot changes. The M2 transcript
does: the camera reset alone moves it from line 422 (hash-only, all pins
green); the placement moves it from line 9; the `0x157A64` delta cell shifts
the restart window by 3 frames. Two measured re-pins are planned (v28, v29).

## 2. Evidence base (R7, fresh; re-verified this session)

`/tmp/opencode/recon51/R7.md` — the full stage-0 listing, the callee
summaries, the engine inventory, and the measured M2 variants (A2, A3, B, D,
E, F) on a byte-identical v26 base.

Key facts:
- The `0x10F328/2C/30` triple passed to `0x700F4` is a **constant zero**
  (`read_memory` 24 B all zero; 20 xrefs, no writer).
- `0x700F4` writes the camera triples, zeroes pan/tracked/event cells, sets
  the pan constants `0x157819/1A/1B = 10/16/24` (image-default `0x1104AB`
  indices), and clears `[0x1577CA]` (tracked NONE).
- `0x73E28` clears the phase/clock/focus/goal/controlled cells, sets
  `0x157A73 = 0x15774C`, sets the camera-height anchor `0x157A6D = 0xB0/0xA0`,
  zeroes the possession (`0x158724..2C`) and ball-pair (`0x158730..46`)
  blocks (the latter with `0x158742 = 0x20`, `0x158743 = 2`), and zeroes the
  frame delta `0x157A64` **before** the stage-head add — so the stage-head
  add reads 0 on the reset frame.
- `0x73E08` commits the staged ball target (`0x8C24C`) and runs the
  `0x8CF60` loop (place → face → anim select → row43 → frame 0); the
  `0x79F3C` ring place is skipped at phase 0x15 (`0x1106C3[0x15] = 0`).
- `0x4C324` ignores all arguments; it sets `[0x1074A4]` from the situation
  record's side (`[0x1587D4]`, `+0x826`) when `[0x157AC2] >= 4`, else from
  `[0x1587E3]>>24`, and calls `0x36200` (a one-store stub).

Measured M2 (R7 §3; base v26):
| variant | first diff | lines | state | pins |
|---|---|---|---|---|
| A2 `camera_init(0,0,0)` | 422 | 6101 | unchanged | all pass |
| A3 A2 + pan 10/16/24 | 422 | 6101 | unchanged | masked (byte-identical to A2) |
| E `frame_delta=0` | 470 | 5981 | `21` 100→103, `1` 953→950, `2` 2173→2174, `12` 4→3 | restart window +3 |
| B A2 + non-seeding `0x73E08` | 9 | 6514 | unchanged | begin pins move; held-key x fails |
| F A3+B+E | 9 | 6514 | as E | `gate−phase15 96→100`, `phase4−phase15 100→103`, `restart−phase4 303`, act timer 2→0 |

## 3. Task shape

### Task 1 — the camera reset + present bind (re-pin v28)

- `fifa96_camera_init`: set `pan_decay = 10`, `pan_rate_hi = 16`,
  `pan_rate_lo = 24` (the native `0x7011C/0x7013D/0x70146` stores; the
  `0x1104AB` table's image-default indices). Adjust any unit fixture that
  pinned the old zeros.
- Wire `fifa96_camera_init(&mr->render.camera, 0, 0, 0)` in
  `fifa96_match_action_phase_1F` stage 0 after the `[0x15882A]` clear and
  before the phase write.
- Add `present_side` (`[0x1074A4]`) to the run/render state and
  `match_run_present_bind(mr)` reusing the `fifa96_action_duel_bind` inputs
  (mode `global_157ac2`, the `0x1587D4` situation pointer, its `+0x826`
  side, fallback `[0x1587E3]>>24`) + `fifa96_arm_stub_36200()`; call last in
  stage 0.
- Fixtures: the stage-0 order (gate clear → camera zero + tracked NONE →
  phase 0x15 → present bind), the pan constants, the two present-side arms.
- M2 re-pin **v28**: expected first diff 422, 6101 lines, hash-only, state
  tags unchanged; all existing pins pass. If the measured diff is not this,
  stop and report.

### Task 2 — the match reset + non-seeding placement (re-pin v29)

- `fifa96_match_run_act9_reset(mr)` porting `0x73E28` cell-for-cell (phase/
  prev phase/`global_157a4c`, clock cells, focus pointer, goal record,
  controlled, `frame_delta = 0`, camera-height anchor, the `0x7F144`
  possession block, the `0x7A028` ball-pair block + `0x158742 = 0x20`,
  `0x158743 = 2`), called between the camera reset and the phase write.
- `fifa96_match_entities_restart_place(pool, cam_x, cam_y, cam_z)`: the
  non-seeding `0x73E08` — ball := `pool->ball.target_*` (x/z copied, y = 0,
  velocity/heading/metric cleared) then the `0x8CF60` loop from
  `kickoff_place` (no `0x1E0` seed); call after the phase write. Do not run
  the `0x79F3C` ring place (`0x1106C3[0x15] == 0`).
- Settle the act-tick ordering caveat (native `0x8AF38` before `0x8D8EC`);
  if the engine order cannot be aligned cheaply, record it as a numbered leg
  and re-derive the begin pins from the measured transcript.
- Fixtures: the act-9 reset cells, `restart_place` (ball from the target,
  per-record pos/face/anim/frame, the ring-place skip), the re-derived
  held-key and restart-chain pins.
- M2 re-pin **v29**: measured diff recorded; expected first diff 9, 6514
  lines, the E state deltas, `gate−phase15 96→100`,
  `phase4−phase15 100→103`, `restart−phase4 303`, act timer 2→0, plus the
  unmeasured `0x73E28` cell effects (measure, do not assume F).

### Task 3 — close-out

- FU-181/FU-170 errata (L5 closed; the unmodelled `0x700F4` sound/place
  recompute legs; the act-tick ordering leg if it survives), a phase-53
  ENGINE register, PROGRESS, the FU-342 spec record (the act-9 sinks).
- Final numbers: census unchanged (78/78, 58/80, 64/80, 6 blocked, action
  38/45, phase 26/35); M1 exact; M2 v29; `make check` green.

## 4. Invariants

- M1 `09b726b7…` immutable. Each re-pin carries a written reason + the
  measured frame diff + the state-stratum check.
- The `0x73E28` / `0x73E08` / `present_side` cells cite their native
  addresses; unmodelled sub-effects become named legs.
- Census strings/README unchanged by this phase; `make check` green at every
  commit; work on `main`, one commit per task, push.

## 5. Out of scope

- Act-9 stages 1/2 (the compression stays, FU170-L1) and the `0x79F3C` ring
  place at phase 0x15 (the native gate is 0).
- The `0x92820(0,0x24)` sound and the `0x70074/0x6D870` place recompute
  inside `0x700F4` (legs; no consumers on the tape).
- The `0x36200` consumer (`0x36208`, unported).
- The player-visible register and the `PCCD.POG`/`PCDB` data slice.
