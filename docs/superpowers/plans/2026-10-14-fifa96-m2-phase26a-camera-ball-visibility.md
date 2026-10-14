# FIFA 96 M2 phase-26a — camera / ball visibility (the visible anchor)

> **For agentic workers:** REQUIRED SUB-SKILL: `superpowers:subagent-driven-development`. Steps use checkbox (`- [ ]`) syntax. One implementer per task, serialized.

**Goal:** Make the camera/follow chain **live** (Layer 1) and drive the phase-26 **visible** acceptance (Layer 2) — the follow-cam or the row-0E pan — by porting the missing **live open-play `[0x157750]` (camera-Y) producer**; if the visible layer proves unreachable, record an **honest written waiver**.

**Architecture:** The arc `fifa96_camera_height` (`fifa96_camera.c:91`) and the follow (`match_run.c:3741/4147`) are **already ported**; the missing piece is a live **event that carries a nonzero height** (`FU264 §4.1`). So 26a is one producer + the pan/framing reach, not a rewrite.

**Tech Stack:** C11 / CMake / CTest, ASan/UBSan, the null backend (determinism source of truth), SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-design.md` (§5, D1, §7.1). **Frozen evidence (authority for every native address):** `docs/ghidra/FU264_camera_y_producer.md`, `FU265_place_pan_ball_framing.md`, `FU271_live_trigger_census.md`. **Parent recon plan:** `docs/superpowers/plans/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-recon-ahead.md`.

## Global Constraints

- **Baseline:** HEAD `d57b7a7`. Census **59/80 = 73%** (`36 action + 23 phase`); M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (691 lines, **immutable**); M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (**v22, 6522 lines**).
- `make check` green (strict `-Wall -Wextra -Werror`, sanitized). Tests run from the repo root.
- **The live-chain rule (AGENTS/spec):** a chain is **live** only when a **ported producer raiser** raises its gate on a real engine path — a fixture that stages the gate does **not** count; a stand-in is a **fake** (R15/R45/R49). A producer that cannot be raised live is a **numbered leg**, not a wire.
- **No fakes.** Do not invent a live gate (FU265 §4.1: "Do not invent a live gate"). A cell with no live producer stays a leg.
- **M2 preservation:** M2 moves only on a written reason + frame diff; prefer changes that leave the tape byte-identical. **M1 never moves.**
- **Census:** a NULL row a live producer wires is honest **upside**; if the wired count changes, update README title + `docs/ENGINE.md` + `docs/PROGRESS.md` + `AGENTS.md:90` + `tests/test_engine_m2.c` pins **in the same commit**. It may hold at 59/80.
- **Engine seams (verified):** the arc store `fifa96_camera.c:90-91` (inside `fifa96_camera_update_walk`, live at `match_run.c:4147`); the event setter `fifa96_camera_event_set`; the follow `fifa96_match_run_camera_follow` (`match_run.c:3747-3749`); the pan `fifa96_camera_pan_step` (`camera.c:96`); the row-1E release `handlers.c:521-530`.
- Native addresses are quoted **only** from the frozen slices; re-verify first-hand before trusting (Ghidra read-only, program exactly `/FIFA96.EXE`).

## The camera producer shape (from FU264 §4.1 / FU265 §5)

```
  the visible follow needs:  event_param > 0xF0  AND  a live jump selector  AND  camY off zero
  the row-0E pan needs:      0x20 < camY < 0x70   (the keeper release camY:=0x50 sits IN-window)
        │
        ▼  the missing piece = a LIVE open-play camY-off-zero source
  candidates (frozen):
    (a) the record-event tail  FUN_00070C08  0x70DD1   (open-play; needs rec+0x5D > 0x80  OR  camY > 0xF0)
    (b) the keeper stage-5 release  0x75D53/0x75D69  (camY := 0x50; IN the pan window)  — dispatch-dead behind rows 1A/1B (R11-L4)
    (c) the throw-in stage-3 cut  0x85919  (camY := 0x80; restart-driven; sets NO event)
```

**File structure (touched):** `src/fifa96_engine/fifa96_match_run.c` (the frame body / the producer seam), `src/fifa96_engine/fifa96_match_handlers.c` / `fifa96_match_action_*.c` (the row bodies a producer feeds), `src/fifa96_loader/fifa96_camera.c` (only if the arc/pan needs a faithfulness fix), `tests/test_engine_match_frame.c` + `tests/test_camera.c` (the reach tests), `docs/ENGINE.md` / `docs/PROGRESS.md` / `README.md` / `tests/test_engine_m2.c` (only on a census change).

---

### Task 1: The determinism + reachability gate

**Files:** none changed. `docs/ENGINE.md` read.

- [ ] Recompute the census (`36 action + 23 phase`); confirm `59/80`.
- [ ] Prove M1/M2 byte-stability: `./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt` and the M2 equivalent (≥3 replays); sha-check both goldens.
- [ ] Build the 26a reachability map from FU264/FU271: the arc is live-but-starved; the only open-play writer candidates are (a)/(b)/(c) above.
- [ ] Report: the gate is green, and the exact producer route Task 2 must port.

### Task 2: The live open-play camera-Y producer (the visible anchor's gate)

**Files:** `src/fifa96_engine/fifa96_match_run.c` (+ the row body a producer feeds, per the chosen route), `tests/test_engine_match_frame.c`.

- [ ] **RED first:** add a test that drives a **live** open-play scenario and asserts `fifa96_camera.pos_y != 0` **and** the follow/pan consumer fires (e.g. `camera.tracked != 0` / the row-0E window admits). Run it — it **must fail** on the baseline (the cell stays 0). Record the failure.
- [ ] Port the **shortest honest** live route among (a)/(b)/(c) — prefer **(a) `FUN_00070C08 0x70DD1`** (open-play, no R11-L4 dependency) if it is reachable from a live actor; else port **(b)** behind the R11-L4 sit-5 producers (rows 1A/1B — a separate, larger leg; if chosen, split into its own task). Cite FU264/FU265 bytes; do **not** invent a gate.
- [ ] GREEN: the test passes; the cell goes off zero on a live path (Layer 1 proven).
- [ ] Run the covering tests (`test_engine_match_frame`, `test_camera`, `test_engine_m2`); confirm M2 byte-identical (the tape must not move) — if it does, the producer is tape-visible and needs a written reason + frame diff.

### Task 3: The live pan + ball framing (FU265)

**Files:** `src/fifa96_engine/fifa96_match_run.c` / `fifa96_camera.c`, `tests/test_camera.c`.

- [ ] With Task 2's Y source live, prove the **row-0E pan window** (`0x20 < camY < 0x70`, FU264 §2.1) admits and the pan fires (the pan step + counter are already ported, `camera.c:96`/`883`).
- [ ] Keep the camera-0 seed + the camera-derived lead (FU265 §3); do **not** port R2-L1's framing integrators (FU264-L4 — framing only, not required for translation).
- [ ] RED-first where a gate is newly reachable; GREEN + the covering tests.

### Task 4: The visible acceptance (Layer 2) — or the honest waiver

- [ ] `make game` smoke (`DISPLAY=:1`): capture the frame sequence; determine whether the follow and/or the pan is **visible** on a live kick.
- [ ] If visible: record the acceptance (the captured frames + the driver) in `docs/PROGRESS.md`.
- [ ] If **not** visible after Task 2's port: write the **honest waiver** in `docs/PROGRESS.md` (per spec §7.1) — exactly which route was ported, why it does not surface in the smoke window, and the residual leg (e.g. the follow's `>0xF0` bootstrap if only the pan came live).

### Task 5: The census / leg-register close-out

**Files:** `docs/ENGINE.md`, `docs/PROGRESS.md`, `README.md`, `AGENTS.md:90`, `tests/test_engine_m2.c` — **only if the wired count changed.**

- [ ] Recompute `floor(100 × wired/80)`; if it moved, reconcile the five docs + the test pins **in one commit**.
- [ ] Append the resolved/forbidden camera legs (FU264-L1..L5) as quoted errata; update the phase-7 known-gaps rows.
- [ ] `make check` green; M1/M2 as declared.

## Phase-26b (follow-on, separate plan)

`phase-26b — set-piece / fouls / keeper / replay` (FU266…FU270): one live producer per chain (set-piece triggers, the row-04 `0x0B` duel LIVE-LEG, the keeper installer, the front-end replay/overlay **honest legs**), per FU271 §4.2. Authored after 26a.

## Self-Review

- **Spec coverage:** 26a = the camera cluster (spec §1 camera row; FU264/FU265) with the Layer-2 anchor (spec D1/§6) and the §7.1 waiver path. 26b = the rest (FU266…FU270) — deferred as a separate plan (spec §5 permits the split).
- **Placeholders:** the producer route is deliberately enumerated (a)/(b)/(c) because Task 2 must *prove* the route live (RED-first) — a plan that fixed the route before the reachability gate would guess. Task 2's deferral to Task 1's map is the plan's own sequence.
- **Consistency:** baseline `d57b7a7`; the arc/follow "already ported" claim matches FU264; the "no invented gate" rail is repeated (FU265 §4.1); the M2/census rails match AGENTS.
- **Risk:** the visible layer may be unreachable (Tasks 2→4 may end at the waiver) — flagged, not hidden.
