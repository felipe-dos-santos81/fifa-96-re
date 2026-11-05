# Phase-53 producers — the act-9 stage-0 sinks

**Status:** proposed 2026-11-05. Spec:
`../specs/2026-11-05-fifa96-m2-phase53-act9-sinks-design.md`. Recon:
`/tmp/opencode/recon51/R7.md`.

**Baseline:** HEAD `35b156f`, `make check` **112/112**, M1 `09b726b7…`, M2
`54f44987…`, census **78/78, 58/80, 64/80, 6 blocked, action 38/45, phase
26/35**.

---

## Task 1 — the camera reset + present bind (v28)

**Files:** `port/src/fifa96_loader/fifa96_camera.c/.h`,
`port/src/fifa96_engine/fifa96_match_act.c`, `fifa96_match_run.c/.h`,
`fifa96_match_handlers.c` (header include if needed),
`port/tests/test_camera.c`, `port/tests/test_engine_match_act.c`,
`port/tests/test_engine_m2.c`, `port/tests/golden/engine/m2-frames.txt`,
`port/spec/FU-181*`, `port/ENGINE.md`, `docs/PROGRESS.md`.

1. `fifa96_camera_init`: `pan_decay = 10`, `pan_rate_hi = 16`,
   `pan_rate_lo = 24` (native `0x7011C/0x7013D/0x70146`); update the camera
   fixture that pinned zeros if any.
2. In `fifa96_match_action_phase_1F` stage 0, after
   `mr->global_5882a = 0` and before the phase write,
   `fifa96_camera_init(&mr->render.camera, 0, 0, 0)` (`0x700F4`).
3. Add `present_side` (`[0x1074A4]`) + `match_run_present_bind(mr)` (the
   `0x4C324` logic: mode `global_157ac2 >= 4 && render.sub.record[0] != 0` →
   the situation record's side `+0x826`, else `[0x1587E3]>>24`) +
   `fifa96_arm_stub_36200()`; call last in stage 0.
4. Fixtures: the stage-0 order + camera cells/tracked NONE + the two
   present-side arms.
5. M2 re-pin **v28** (measured reason + frame diff + state strata). Expected:
   first diff 422, 6101 lines, hash-only, state unchanged, pins pass.
6. Docs: FU-181 errata + the ENGINE phase-53 register start.

**Accept:** `make check` green; M1 exact; M2 v28 as measured; the existing
pins pass (if a pin moves, stop and report — A2 says they do not).

---

## Task 2 — the match reset + non-seeding placement (v29)

**Files:** `port/src/fifa96_engine/fifa96_match_entities.c/.h`,
`fifa96_match_run.c/.h`, `fifa96_match_act.c`,
`port/tests/test_engine_match_entities.c`,
`port/tests/test_engine_match_act.c`,
`port/tests/test_engine_match_frame.c`, `port/tests/test_engine_m2.c`,
`port/tests/golden/engine/m2-frames.txt`, the docs.

1. `fifa96_match_run_act9_reset(mr)`: the `0x73E28` cells (phase/prev/
   `global_157a4c`, clock `0x157A60/5C`, focus `0x157A73`, goal record
   `0x157A9F`, controlled `0x157A83`, `frame_delta = 0`, `0x157AAB = 0xFF`,
   the height anchor `0x157A6D = 0xB0/0xA0`, `0x157AB0 = 0`, `0x157A6F = 0`,
   the `0x7F144` possession block, the `0x7A028` ball-pair block +
   `0x158742 = 0x20`/`0x158743 = 2`), called between the camera reset and the
   phase write.
2. `fifa96_match_entities_restart_place(pool, cam_x, cam_y, cam_z)`: the
   non-seeding `0x73E08` (ball := `pool->ball.target_*`, x/z copied, y = 0,
   velocity/heading/metric cleared; then the `0x8CF60` loop from
   `kickoff_place`); called after the phase write. No `0x1E0` seed, no
   `0x79F3C` ring place.
3. Settle the act-tick ordering caveat (native `0x8AF38` before `0x8D8EC`);
   record a numbered leg if not aligned.
4. Fixtures: the act-9 reset cells, `restart_place`, the re-derived held-key
   and restart-chain pins.
5. M2 re-pin **v29**: measured diff recorded (expected first diff 9, 6514
   lines, the E state deltas, `gate−phase15 96→100`,
   `phase4−phase15 100→103`, `restart−phase4 303`, act timer 2→0; the
   unmeasured `0x73E28` cells may add more — measure).
6. Docs: FU-181/FU-170 errata + the ENGINE register entry.

**Accept:** `make check` green; M1 exact; M2 v29 recorded; the live held-key
suite green with the re-derived pins.

---

## Task 3 — close-out

**Files:** `port/ENGINE.md`, `docs/PROGRESS.md`, `port/spec/FU-342*` (new),
`README.md` (only if a census string changed — it should not).

- The phase-53 register with both re-pins, the ordering leg, the unmodelled
  `0x700F4` sub-effects, the final census line, and the FU-342 evidence
  record.
- Verify: M1 `09b726b7…`, M2 v29, census unchanged, `make check` green,
  tree clean, push.

---

## Global constraints

- Work on `main`, one commit per task, push `origin main`.
- No census/README number change (no dispatch row is wired).
- Every re-pin: written reason, measured first-diff/lines, state-tag set and
  counts intact, and the old/new sha.
- No fake producer: the `0x700F4` sound/place-recompute and the `0x36200`
  consumer stay named legs.
- `make check` green before each commit.
