# FIFA 96 M2 phase-28 Stage-2 — the installer live (the staged camera unlock)

> **For agentic workers:** REQUIRED SUB-SKILL: `superpowers:subagent-driven-development`. Steps use checkbox (`- [ ]`).

**Goal:** Arm the keeper-CPU ladder live end-to-end — the static P1 permission table bound live, the interlock ordering repaired (Option B), the seam contract merged, row `0x1A` ported + wired — through the **hard Layer-1 gate** (a real-state install+dispatch), then port the FU293 deep pan + the minimal visible-motion writers so the follow/pan camera **visibly moves** in `make game`, or an honest §6 waiver for whatever remains.

**Governing spec:** `docs/superpowers/specs/2026-10-16-fifa96-m2-phase28-installer-live-design.md` §3/§5/§6.
**Frozen authority:** the phase-28 wave `docs/ghidra/FU298..FU305` (reviewed + fixed, `994f924`) + the phase-27 chain (`FU290..FU297`, `7d013e0`).

**The wave's binding findings (reviewed):**
- **P1 `[0x110680]` is a static table (GO)** — 45 byte-constants (`0x110680..0x1106AC`), 19 read-only sites, **no writer exists**: port = byte-exact `kc_permit_tbl[0x2D]` + live bind `kc_rec.permit = (e->code < 0x2D) ? tbl[e->code] & 1 : 0`, replacing the caller-supplied cell (`fifa96_match_entities.c:766`, header `:117-121`); retire FU296-L2's "producer" reading; reads bound ≤`0x2C` (the extra FU305 codes are mode-table bytes). **The natural tape arms it** (`perm[0]=3`, `perm[0x19]=3`) — the install tree can fire from the first granted frame, so **T1 is the first M2 re-pin risk, not the ladder**; the phase-27 `permit == 0` waiver test must be re-aimed (RED by design, FU298-L6).
- **Interlock = Option B (GO)** — per team: bind cell→view immediately **before** `fifa96_keeper_cpu_step`, publish view→cell immediately **after** it (in `team_update`, before the single dispatch); delete the frame-head bind + frame-tail write-back (`run.c:4249`/`:4257`; the `:4257-4259` block is the `[0x157AB2]` publish and moves into the per-step publish). Native order: gate read `0x7858E` → `CALL 0x77EAC` → clear `0x785A2` → row dispatch `0x785A8`; the row-`0x19` set (`0x74707`) then survives to the next step's gate (one-step pulse; the same-frame cross-team arm is derived — FU299-L1, leg).
- **The seam contract merge (review Top-1)** — both Seam A installs route through `fifa96_match_entities_install` (incl. stage92/timer89/ran/+0x7B), **not** a bare `e->code = install_code`, or row-`0x1A`'s first dispatch mis-routes on stale stage92.
- **`copy_pos` (GO, zero-δ)** — apply `e->target_* = e->pos_*` (`0x77F20` LEA/MOVSD×3; closes FU290-L10); the `[0x157C2C]` stamp stays a numbered leg (no engine cell).
- **Row `0x1A` (GO conditional)** — the 5-arm stage machine `{0x76693,0x766DD,0x766F5,0x76AAD,0x76AE8}` + the `[0x157C2C]` direction machine + the event ladder `{5,4,9,1,7,0}` + the sit-7/sit-5 emits (`set_piece`); L2 bridges `0x79C50`/`0x6E598` are movement/anim bridges (**required-if-touched**, not sinks); census `59→60` only on the proven live install+dispatch.
- **Deep pan (GO on shape; review Top-2)** — FU301 blocks `0x77035..0x7733F` (seed `ECX=0x60-(8-d)`; the `×3/4`@`0x7707D`; unconditional `ECX-=ECX>>2`; the code-3 reduce touches only `[0x1577C2]`) + FU302 (the folds; `EDI` sign-match `XOR EDI,EDI`@`0x76D3B`; the install tree `0x77428..`; the sit dispatch; `FUN_00076B28` steering). **The seat's height is camY-derived (`SI=dword[0x157750]`@`0x77228`), NOT jitter** (FU302-L10 superseded by the adjudication).
- **Visible motion (FU303)** — the sustained pan rides FU294-L2 `[0x157815]` → the `FUN_00071DF4` rate table → `rate_x/rate_z` → the `update_walk` pan-push (`camera.c:122-143`); FU292-L1/L2/L3 minimal (`F6 = timer_limit−ramp_divisor`; the byte3 producer `FUN_000703E8`, honest `0xFF` staging); the rest waivable; no listed cell blocks a first visible install (CX=4 → row `0x1B` wired).
- **Tape (FU305)** — phase-2 windows: 46-65, 773-845, 1803-2502, 2703-3402, 3603-4302 (2193 frames); the stage-select AX gate opens only after the m42 park (team-0 keeper); M1 `09b726b7…` immutable; M2 `b281a9b2…` re-pin only with a written reason + frame diff — **T1/T5 stop for that diff first (spec §5)**.

## Global Constraints

- `make check` green (strict `-Wall -Wextra -Werror`, ASan/UBSan; ~143 s → ≥600 s timeout); work on `main`; commit per task; push per task.
- **M1 `09b726b7…` immutable.** **M2 `b281a9b2…` (v22):** re-pin only if a **live** path changes the tape — written reason + frame diff; the task that moves it stops and reports before re-pinning.
- **The live-chain rule:** a fixture that stages a gate is a fake; staging a producer's **input** is legitimate; an unraised surface is a numbered **LIVE-LEG**. Never fabricate a stage index, a permit value, or a writer.
- **The permit read is bounded ≤`0x2C`** (the 45-byte table); the native out-of-range aliasing stays FU298-L1.
- **Census `59/80`** — moves only on a proven live install+dispatch (the four-doc same-commit rule; `floor(100×wired/80)`; the FU297 dead-count discipline).
- Ghidra read-only; no fakes.

## File Structure

| Path | Responsibility |
|---|---|
| `src/fifa96_loader/fifa96_keeper_cpu.c` / `include/fifa96_loader/fifa96_keeper_cpu.h` | the permit table + the live bind helper; the existing ladder/stage machine |
| `src/fifa96_engine/fifa96_match_entities.c` | the per-step bind/publish (Option B); the merged install contract; the `copy_pos` apply |
| `src/fifa96_engine/fifa96_match_run.c` | delete the frame-head/frame-tail bind/write-back; the `[0x157AB2]` publish migration |
| `src/fifa96_engine/fifa96_match_handlers.c` | row `0x1A` (port + wire); the deep-pan blocks + the seat; the visible-motion writers |
| `tests/test_keeper_cpu.c`, `tests/test_engine_match_frame.c` | the gate/bind tests + the Layer-1 reach + mutation-RED riders |
| `docs/ENGINE.md`, `docs/PROGRESS.md`, `README.md`, `AGENTS.md:90`, `tests/test_engine_m2.c` | the census + leg register (close-out) |

---

### Task 1: The P1 permission — the static table + the live bind

**Files:** `fifa96_keeper_cpu.c/.h`; `fifa96_match_entities.c` (`:766` bind, header `:117-121`); Tests `tests/test_keeper_cpu.c`, `tests/test_engine_match_frame.c`

- [ ] **Step 1: RED** — assert the bind: codes `{0,3,4,5,6,0x19}` → 1; `{7,0x10..0x13}` → 0; `≥0x2D` → 0 (`kc_permit_tbl[0x2D]`, byte-exact from `0x110680`).
- [ ] **Step 2: Implement** the const table + the bind at the seam; re-aim the phase-27 `permit == 0` waiver test to the new expectation (FU298-L6).
- [ ] **Step 3:** PASS; `make check`.
- [ ] **Step 4: The tape check** — run the M2 tape; if frames move, **STOP and report** the diff + reason for the controller's re-pin ruling (spec §5); else record byte-identical.
- [ ] **Step 5: Commit** `feat(fu298): the install permission — the static table + the live bind`.

### Task 2: The interlock ordering — Option B

**Files:** `fifa96_match_entities.c` (the per-step bind/publish), `fifa96_match_run.c` (delete `:4249`/`:4257`, migrate the `[0x157AB2]` publish); Test `tests/test_engine_match_frame.c`

- [ ] **Step 1: RED riders** — (a) the cell reads 1 after a team-1 row-`0x19` frame; (b) the same-frame cross-team `0x1B` install (team-1's gate sees team-0's same-frame set); (c) the phase-27 T4 reach keeps its staged P4 fixture green.
- [ ] **Step 2: Implement** Option B (bind→step→publish per team); zero-δ while `permit == 0`.
- [ ] **Step 3:** PASS; `make check`; M2 `cmp` (expect byte-identical — permit bind is T1's).
- [ ] **Step 4: Commit** `fix(fu299): the interlock ordering — the per-step bind/publish (Option B)`.

### Task 3: The merged install contract + the copy_pos apply

**Files:** `fifa96_match_entities.c`/`fifa96_match_run.c`/`fifa96_match_handlers.c`; Test `tests/test_engine_match_frame.c`

- [ ] **Step 1: RED** — an install through the seam carries the full record state (stage92/timer89/ran/+0x7B) via `fifa96_match_entities_install`; and `copy_pos` applies `target_* = pos_*`.
- [ ] **Step 2: Implement** the merged contract + the copy apply; keep the `[0x157C2C]` stamp a numbered leg (no engine cell).
- [ ] **Step 3:** PASS; `make check`; M2 `cmp` (zero-δ while unraised).
- [ ] **Step 4: Commit** `feat(fu304): the seam contract — the merged install + the copy_pos apply`.

### Task 4: Row `0x1A` — the port + the wire

**Files:** `fifa96_match_handlers.c`; Test `tests/test_engine_match_frame.c`

- [ ] **Step 1: RED** — the 5-arm stage machine + the `[0x157C2C]` direction machine + the event ladder + the sit-7/sit-5 emits, dispatched via `{0x1A, fifa96_match_action_1A}`.
- [ ] **Step 2: Implement** the port; wire the slot; the L2 bridges (`0x79C50` → the ported `fifa96_entity_face`; `0x6E598` anim) required-if-touched — anything genuinely unported stays a numbered leg (no fakes).
- [ ] **Step 3:** PASS (row-level); `make check`; M2 `cmp` (unraised — the reach gate is T5).
- [ ] **Step 4: Commit** `feat(fu300): row 0x1A — the body + the wire (the census flip is T5's)`.

### Task 5: The Layer-1 reach proof (the hard gate) + the census

**Files:** Test `tests/test_engine_match_frame.c`; the four docs + `tests/test_engine_m2.c` pins **only if green**

- [ ] **Step 1: RED on the phase-27 baseline** — real state arms the ladder (phase 2 + the table permit + the Option-B interlock), a non-zero `CX` installs (the merged contract), the installed arms dispatch (row `0x1B`; row `0x1A`) and the camera cells move (`camera.pos_y`/pan).
- [ ] **Step 2: GREEN** with mutation-RED riders on each new producer (the permit bind, the interlock publish, the copy_pos apply, the row-`0x1A` route).
- [ ] **Step 3: Census** — if GREEN: `59/80 → 60/80 = 75%` (README title + `docs/ENGINE.md` + `docs/PROGRESS.md` + `AGENTS.md:90` + `tests/test_engine_m2.c` pins in ONE commit); if the route honestly fails: the slot stays `NULL`, the legs name the gap (the FU297 discipline).
- [ ] **Step 4: Commit** `test(fu298): the live installer reach — GREEN (Layer 1)` (+ the census in the same commit).

### Task 6: The deep pan I — the seed and the A–F blocks

**Files:** `fifa96_match_handlers.c` (the `match1b_wide_leg` region); Test `tests/test_engine_match_frame.c`

- [ ] **Step 1: RED** — staged-input block tests: the seed `ECX=0x60-(8-d)`; the `×3/4`; the `[0x14C2F6]` switch; block-C jitter; block-E latch path.
- [ ] **Step 2: Implement** `0x77035..0x7733F` per FU301 (the three corrections included).
- [ ] **Step 3:** PASS; `make check`; M2 `cmp`.
- [ ] **Step 4: Commit** `feat(fu301): the deep-pan interpolation I — the seed and the A–F blocks`.

### Task 7: The deep pan II — the folds, the seat, the steering

**Files:** `fifa96_match_handlers.c`; Test `tests/test_engine_match_frame.c`

- [ ] **Step 1: RED** — the folds (incl. the `0xCD514` hypot and the `0x74584` band predicate); the `EDI` sign-match; the install-tree sinks; the seat call with the **camY-derived height** (`dword[0x157750]`, never jitter — the adjudication); the `FUN_00076B28` steering writes.
- [ ] **Step 2: Implement** FU302 (the adjudicated seat arg).
- [ ] **Step 3:** PASS; `make check`; M2 `cmp`.
- [ ] **Step 4: Commit** `feat(fu302): the deep-pan interpolation II — the folds, the seat, the steering`.

### Task 8: The visible-motion writers

**Files:** `fifa96_match_handlers.c`/`fifa96_match_run.c`; Test `tests/test_engine_match_frame.c`

- [ ] **Step 1: RED** — the rate chain: `[0x157815]` → the `FUN_00071DF4` table → `rate_x/rate_z` → the `update_walk` pan-push produces sustained motion on the frame path.
- [ ] **Step 2: Implement** the dir byte + the rate-table arm + the minimal FU292 writers (`F6` derivable; byte3 `FUN_000703E8` with honest `0xFF` staging); waivable cells stay legs.
- [ ] **Step 3:** PASS; `make check`; M2 `cmp`.
- [ ] **Step 4: Commit** `feat(fu303): the visible-motion cells — the dir byte, the rate table, the minimal writers`.

### Task 9: The Layer-2 acceptance + the close-out

**Files:** `docs/screens/p28-*.png`; `docs/ENGINE.md`, `docs/PROGRESS.md`, `README.md`, `AGENTS.md:90`, `tests/test_engine_m2.c`

- [ ] **Step 1: `make game`** (`DISPLAY=:1`) — the walkthrough → the pan pair vs the still pair (`docs/screens/p28-<tag>-pan-a/b.png`); AE on the `y < 591` crop (pan pair > 0; still pair = 0; precedent ~2.9e4–4.6e4); attribution = the same-commit Layer-1 cell assertions (AE alone cannot separate camera from entity motion).
- [ ] **Step 2: The honest verdict** — visible, or the §6 waiver with the legs named.
- [ ] **Step 3: The leg register** — append as quoted errata: FU298 (P1 static; the re-armed waiver), FU299 (Option B; the cross-team leg), FU300 (row `0x1A`), FU301/302 (the deep pan + the seat adjudication), FU303 (the needed/waivable split), FU304 (the contract), FU305 (the acceptance + the tape); `docs/PROGRESS.md` close-out.
- [ ] **Step 4:** `make check`; M1 byte-identical; commit `docs(phase28): the close-out — the census + the leg register`.

---

## Self-Review

- **Spec coverage:** §3's 8 surfaces map to T1 (FU298), T2 (FU299), T4 (FU300), T6/T7 (FU301/FU302), T8 (FU303), T3 (FU304), T9 (FU305); §5 Layer-1 = T5, Layer-2 = T6–T9; §6 honest-stop is the Global Constraints + T4/T5/T9.
- **The honest risk:** the row-`0x1A` L2 bridges and the T1 tape move. If the tape moves at T1, the task stops for the diff + reason (spec §5); if the row route fails honestly, the census holds `59/80` with the legs named.
- **Placeholders:** none.

## Execution Handoff

**1. Subagent-Driven (recommended).** **2. Inline.**
