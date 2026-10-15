# FIFA 96 M2 phase-27 — the keeper-CPU installer (the camera unlock)

**Status:** approved scope (brainstorming, 2026-10-15). Binding on the phase-27 plan + its Stage-2 execution.
**Parent:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-design.md` (the phase-26 theme: make genuinely-unreachable chains engine-reachable). Phase-26c closed the R11-L4 **bridge** + a granted-frame **reach**; phase-27 closes the **installer**.

## 1. Goal

Make the keeper-CPU install path live so the row-`0x1B`→sit-5→phase 9→row-`0x1E`→claim-stage-5→release chain fires on a **real engine frame**, raising `camera.pos_y` and putting the follow/pan camera (Layer 2) live — or honestly ruling each unported surface a numbered leg.

## 2. What phase-26c left (the binding starting state)

- The R11-L1/L2/L3 claim bridge is **ported** (`7897330`/`bd3b24a`); the row-`0x1B` wide/steer + the self-actor reach to the emit are **ported** (`68bab1b`); the record-0 machine (`9486dae`) and the ladder (`f26b075`) are **ported but wired only inside `fifa96_keeper_cpu`** — the ladder is a **LIVE-LEG** (no live stage index) and the module is not linked into the engine.
- The pre-pass is already modelled by `fifa96_match_entities_team_update` (FU281 §2 erratum, `f387465`); the `0x74442` consumer is leg FU281-L5.
- The granted-frame reach test is GREEN (Layer-1, `e96b526`); the **visible camera is NOT live** (honest §6 Layer-2 waiver).
- Census `59/80 = 73%`; M1 `09b726b7…` immutable; M2 `b281a9b2…` byte-identical.

## 3. Scope

**In:** the eight unported surfaces the phase-26c close-out routed here —
1. the ladder stage-select machine (the `CX` computation in `FUN_00077EAC`, the `0x114E04` ramp read, `0x795B4`) — FU283-L6.
2. the per-side table gate + side resolution (`[0x14C1D4]`, `FUN_000741B4`) — FU283-L1.
3. the latch + geometry cells (`0x157C4C`/`0x157C48`/`0x157C2C`/`0x157C5C`; `0x1577F4/F8`, `0x1577BC`, `0x157821`, `0x158740`) — FU283-L2/L3/L5.
4. the row-`0x1B` deep pan interpolation + steering (`0x7707D..0x77420`, FU285-L1; `FUN_00076B28`, FU285-L4).
5. the pan/camera-event setter (`FUN_00071C94`, FU285-L3) + its consumers.
6. the pre-pass consumer (`0x74442`, FU281-L5) + the actor-route producers (the `[0x157A83]` writer set, FU288).
7. the **engine installer seam** — how the record-0 machine + ladder wire into the per-frame walk (reconciled with the FU-141 dispatch) so a live frame installs rows; the `fifa96_keeper_cpu` module's real linkage.
8. the row-`0x1A` decision (the ladder installs it; does the site-5 BX=1 → queue route matter?) + the census/M2 impact.

**Out (stays a leg unless slice 8 overturns it):** row `0x1A` remains **NULL** unless a live installer + emit route is proven (FU284 NO-GO stands absent new evidence); the phase-6 producer (T2b-L2); the `contact_kind==3` live raiser (FU289-L1); the replay/overlay machine (FU278); the FU283 `0x114E04` wording sweep (`FU283_*.md` + the plan files).

## 4. Structure

**Stage-1 (recon-ahead):** an eight-slice read-only wave, one slice per §3 surface, promoted one-per-commit (the phase-26b/26c precedent).
**Stage-2:** a producer plan derived from the frozen slices — the stage-select machine first (it gates everything), then the ladder → the installer seam → the live scenario, then the census/M2 close-out.

## 5. Acceptance

- **Layer-1 (required):** a reach test in which a **real (non-staged) producer** raises the ladder install on a live frame, the row-`0x1B` emit reaches phase 9, row `0x1E` dispatches, the claim reaches stage 5, and the release sets `camera.pos_y == 0x50` — with a mutation-RED rider on each new producer. The test must fail on the phase-26c baseline.
- **Layer-2 (best-effort, honest-stop pre-authorized):** the visible follow/pan camera moves on the `make game` smoke — or an honest waiver with the Layer-1 proof as the landing (as in 26a/26c).
- **Invariants:** M1 immutable; M2 re-pinned **only** with a written reason + frame diff; the census moves **only** when a row is proven live (a `NULL→fn` flip with no live producer is NOT counted — the rows-27/29/2C precedent), and any move updates README title + `docs/ENGINE.md` + `docs/PROGRESS.md` + `AGENTS.md:90` + `tests/test_engine_m2.c` pins **in the same commit**.

## 6. Honest-stop rule

Any surface that cannot be raised live without a stand-in is a **numbered leg** with its native owner — never a fake, never a widened fixture, never an un-dispatched count. If the stage-select chain proves unreachable from ported state, the phase closes as a Layer-2 waiver with the legs named (as 26a/26c did).

## 7. Decisions (this spec)

- The target is the R11-L4 remainder (the installer), **not** the smaller legs (B/C) — it is the phase-scale unlock the visible goal waits on (user choice).
- Approach (1) recon-first, eight slices as listed — the repo's proven phase shape (user choice).
- Work continues directly on `main`; commit per task; push per task (AGENTS.md).
