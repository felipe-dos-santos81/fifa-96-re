# FIFA 96 M2 phase-26c — the R11-L4 keeper CPU chain + the FU289 cell swap

> **For agentic workers:** REQUIRED SUB-SKILL: `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans`. Steps use checkbox (`- [ ]`).

**Goal:** Land the **bounded** wins the 26c recon wave found, then port as much of the R11-L4 keeper CPU chain as a live path permits — so the keeper stage-5 release raises `camera.pos_y` and the follow/pan camera (Layer 2 visible) goes live — keeping every genuinely-unreachable surface an honest **numbered leg**.

**Architecture — what the recon wave re-ruled (FU281–FU289, frozen/promoted):**

| Surface | FU273 said | The wave's verdict |
|---|---|---|
| R11-L1 claim bridge (`FUN_0006E598`) | unported, a second gate | **FU286: bounded GO** — store core already ported (FU-159/160); fix = one missing consume + pause flag + offset table |
| row `0x1B` wide/steer + emit | unported | **FU285: GO** — emit `0x77707` reachable; wide block self-writes the actor at `0x77568` (ungated); head `0x76DBF` and wide `0x77568` are **mutually exclusive** per dispatch (`0x76FD5 CMP EBP,[0x157A83]; JZ 0x77636`) |
| row `0x1A` remainder | unported row to wire | **FU284: NO-GO** — keep NULL; the sit-5 emit is BX=1 → queue id-7 → FU-146, never phase 9 |
| pre-pass `FUN_0008D8EC` | unported | **FU281: NO-GO isolated** — it *is* the native per-record dispatch already modelled by FU-141 → wiring alone double-dispatches |
| record-0 machine `FUN_000782D0` | unported | **FU282: skeleton only** — install is deferred (EBX=0/ECX=0); ladder is one clean gated CALL (`0x78599`) |
| ladder `FUN_00077EAC` | unported installer | **FU283: CONDITIONAL** — wireable; the live *stage index* needs unported stage-select inputs (`[0x114E04]` is a **linear ramp table**, not a sine) |
| phase-6 producer | live? | **FU287: NO-GO** — stays leg T2b-L2 |
| `[0x157A83]` ordering | leg T2b-L1 | **FU288: GO** — gates are disjoint phases (2 vs `0xD`); census **stays 59/80** |
| contact-kind cell | `[0x158882]` | **FU289: it is `[0x15888C]`** — sole producer of `3` = `0x8A772`; the engine reads the **wrong cell** (`foul_kind` vs `contact_kind`) ⇒ a **bounded 2-line fix** |

**Red thread (FU273 §3.3):** row `0x1B` head → sit-5 emit → situation 5 → phase 9 → row `0x1E` → claim stage 5 → release `0x75D69` (`camY := 0x50`). Every arrow is ported **except** the installer (§T6–T8) and the claim bridge (T3).

**Tech Stack:** C11 / CMake / CTest; ASan/UBSan (`-fno-sanitize-recover=all`); the null backend as determinism source of truth; SDL3 smoke (`make game`, `DISPLAY=:1`); Ghidra MCP (read-only) for re-checks.

**Spec:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase26b-remaining-chains-r11l4-design.md` §4 (the 26b-i/26b-ii split — binding). Frozen authority: `docs/ghidra/FU273_r11l4_keeper_cpu_chain.md` + the wave `FU281`–`FU289` (already-promoted; `FU289` corrects the contact cell).

## Global Constraints

- **`make check` green** (strict `-Wall -Wextra -Werror`; ASan/UBSan; ~143 s → allow ≥600 s). Work on `main`; commit per task; push per task.
- **M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` never moves** (691 lines, immutable).
- **M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (v22):** a **live** producer on a traversed path forces a re-pin — only with a **written reason + frame diff**, per-task, after the reach test is green; else keep it byte-identical.
- **The live-chain rule (binding):** a chain is **live** only when a ported producer raiser raises its gate on a real engine path; a fixture that stages the gate is a **fake**; an unreachable surface stays a **numbered leg** with its native owner. Staging a producer's *input* (e.g. the camera pose) is legitimate.
- **Census `59/80 = 73%`.** FU288: the wave wires nothing, so it **stays 59/80** unless a task proves a live install+dispatch. Any change updates README title + `docs/ENGINE.md` + `docs/PROGRESS.md` + `AGENTS.md:90` + `tests/test_engine_m2.c` pins **in the same commit** (`floor(100 × wired/80)`).
- **Ghidra stays read-only** (`/FIFA96.EXE`; alias traps; fresh `get_xrefs_to`). No fakes. ISO-gated tests skip without the ISO.

## File Structure

| Path | Responsibility |
|---|---|
| `src/fifa96_engine/fifa96_match_run.c` | the FU289 cell swap; the per-frame walk (the pre-pass reconciliation); the phase arms `:2834-2861`; `ref_step_restart` `:5956-6013` |
| `src/fifa96_loader/fifa96_keeper.c` | the keeper machines — the **R11-L1** bridge + R11-L2/L3 |
| `src/fifa96_engine/fifa96_match_handlers.c` | the action table (`:5512-5528`), rows `0x1B`/`0x1E`, `match_keeper_claim_apply` `:476-533` |
| `src/fifa96_loader/fifa96_keeper_cpu.c` *(new)* | the ported CPU chain (pre-pass / record-0 machine / ladder) — **only if** T6–T8 land |
| `tests/test_engine_match_frame.c`, `tests/test_keeper_cpu.c` *(new)* | the reach tests + mutation-RED riders |
| `docs/ENGINE.md`, `docs/PROGRESS.md`, `README.md`, `AGENTS.md`, `tests/test_engine_m2.c` | the census + leg register + the (expected) M2 re-pin (close-out) |

**Order:** T1 → T2 → T3 → T4 → T5 → T6 → T7 → T8 → T9 → T10 (bounded wins first, then the chain bottom-up).

---

### Task 1: The determinism + reachability gate

**Files:** Test: `tests/test_engine_m2.c` (read)

- [ ] **Step 1:** `make check` → 110/110; census `action_wired == 36u && phase_wired == 23u`; if moved STOP + report.
- [ ] **Step 2:** M1/M2 `cmp` ×3 — confirm the hashes byte-identical; record the baseline `camera.pos_y == 0`.
- [ ] **Step 3: RED reach test** — a granted-frame scenario (keeper receives row `0x1B` → stage-4 tail → sit-5 → phase 9 → row `0x1E` → claim stage 5 → release) asserting: `phase == 9`; row `0x1E` installed; `anim_row == 0x45` + the release fires; `camera.pos_y == 0x50`, `tracked != 0`. Assert it FAILS at baseline. Commit `test(fu273): phase-26c — the R11-L4 release reach test (RED)`.
- [ ] **Step 4:** Commit/push.

---

### Task 2: The FU289 contact-kind cell swap (bounded win)

**Files:** Modify `src/fifa96_engine/fifa96_match_run.c:~6002/6009`; Test `tests/test_engine_match_frame.c`

**Interfaces:** Consumes FU289 (cell = `[0x15888C]`; the `0x894AD MOV AL,[0x15888c]; CMP EAX,3` phase-`0x18` lock).

- [ ] **Step 1: Failing test** — stage `referee.contact_kind = 3` (vs `foul_kind = 3`) and drive `fifa96_match_action_phase_18`; assert the row-`0x12` (phase-7) branch is taken. Run → FAIL (the engine reads the wrong cell).
- [ ] **Step 2: Implement** the swap: `foul_kind == 3u` → `contact_kind == 3u` at the two FU289-cited sites. Keep the foul-severity draw untouched.
- [ ] **Step 3:** PASS; `make check`; M2 `cmp`. **Disclose** honestly: the live raise of `contact_kind==3` is a **leg** (FU289-L1 — only the offside arm produces 3, and that chain is callerless/settings-gated, FU276-L2/L3); the swap is latent-correctness, not a new live producer.
- [ ] **Step 4: Commit** `fix(fu289): read contact_kind (0x15888C) not foul_kind for the taker phase`.

---

### Task 3: The R11-L1 claim anim bridge (`FUN_0006E598` → `record.anim_id`)

**Files:** Modify `src/fifa96_loader/fifa96_keeper.c`; `src/fifa96_engine/fifa96_match_handlers.c:476-533`; Test `tests/test_engine_match_frame.c`

**Interfaces:** FU286 — the store core is ported (`fifa96_match_run_record_anim_install`, `match_run.c:1002-1027`); the gap is that `out->event` (`0x44`/`0x2F`/`0x45`, `keeper.c:698-700`) is written but never read.

- [ ] **Step 1: Failing test** — with row `0x1E` dispatched, assert the claim reaches stage 5 (`anim_row == 0x45`) and the release arm fires. Run → FAIL.
- [ ] **Step 2: Implement** — consume `out->event` into `record.anim_id` (the `FUN_0006E598` bridge: clamp id 0..0x6E, `row = *(0x157588) + id*9`). The table is runtime-populated; use the engine's `match_run_anim_rows` (row `0x45` at `:117`).
- [ ] **Step 3:** PASS; `make check`; M2 `cmp` (a live release moves the camera → a re-pin with a written reason + diff is expected).
- [ ] **Step 4: Commit** `feat(fu286): consume the claim anim event into record.anim_id (R11-L1 bridge)`.

---

### Task 4: R11-L2 + R11-L3 (the claim place pause + the offset tables)

**Files:** Modify `src/fifa96_loader/fifa96_keeper.c`; Test `tests/test_engine_match_frame.c`

**Interfaces:** FU286 — R11-L2 = two `PUSH 1` (`0x7559E`/`0x7595F`, bytes `6a01`); R11-L3 = the adjacent tables `0x10F334 = {0,1,1,1,0,-1,-1,-1}` / `0x10F33C = {1,1,0,-1,-1,-1,0,1}`.

- [ ] **Step 1: Failing test** — assert the place pause flag + non-zero `place_offset_x/z`.
- [ ] **Step 2: Implement** the pause + read the sector offsets.
- [ ] **Step 3:** PASS; `make check`; M2 `cmp`.
- [ ] **Step 4: Commit** `feat(fu286): R11-L2 place pause + R11-L3 offset tables`.

---

### Task 5: The row-`0x1B` wide/steer + the reachability (the load-bearing win)

**Files:** Modify `src/fifa96_engine/fifa96_match_handlers.c` (`fifa96_match_action_1B` `:3688`); Test `tests/test_engine_match_handlers.c`

**Interfaces:** FU285 — the stage-2 wide `0x76FC7..0x77636` (band `[0x34,0x3A]`, pan cells `0x1577BE/C0/C2`, `0x77423 CALL 0x71C94`), the `0x774DD` re-place with the **ungated self-actor** `0x77568`, and the `FUN_00076B28` steering (refs 2).

- [ ] **Step 1: Failing test** — drive the row via a granted frame; assert the wide/steer stages + the `0x71C94` pan event set + that the `0x77568` self-actor path reaches the stage-4 emit `0x77707`.
- [ ] **Step 2: Implement** faithfully. Preserve the mutual-exclusion with the head write (`0x76FD5 CMP EBP,[0x157A83]; JZ 0x77636`).
- [ ] **Step 3:** PASS; `make check`; M2 `cmp`.
- [ ] **Step 4: Commit** `feat(fu285): port the row-0x1B wide/steer + the self-actor reach to the emit`.

---

### Task 6: The pre-pass reconciliation (`FUN_0008D8EC` vs FU-141)

**Files:** Read/modify `src/fifa96_engine/fifa96_match_run.c` (the per-frame walk); `src/fifa96_loader/fifa96_keeper_cpu.c`

**Interfaces:** FU281 — the pre-pass **is** the native per-record dispatch (`FUN_000782D0` + `FUN_0007CA54` → `CALL dword[rec+0x18]`) already modelled by FU-141. Decide **replace vs augment**; wiring alone double-dispatches.

- [ ] **Step 1: Failing test** — assert the pre-pass writes `[EBP+0x7B2]/[+0x7BA]/[+0x7BE]` (consumed at `0x74442`) exactly once per team per frame, with no double-dispatch of record actions.
- [ ] **Step 2: Implement** the reconciliation (`[EBP+0x82C]` counter cap `0xB`; sector template `0x157770/788/794`; `CALL 0x8DE8C` → `+0x7B2`/`+0x7BA`; the `0xB2`-stride loop).
- [ ] **Step 3:** PASS; `make check`; M2 `cmp` (the pre-pass writes tape-visible cells → a re-pin with a written reason if it does).
- [ ] **Step 4: Commit** `feat(fu281): reconcile the keeper-CPU pre-pass with the FU-141 dispatch`.

---

### Task 7: The `FUN_000782D0` record-0 machine (skeleton)

**Files:** `src/fifa96_loader/fifa96_keeper_cpu.c`; `src/fifa96_engine/fifa96_match_entities.c:698`; Test `tests/test_keeper_cpu.c`

**Interfaces:** FU282 — the head per-actor counter (gated `[0x157823]!=0 && rec==[0x157A83] && phase!=5`), the **deferred** install (`0x19`/`0x4`, EBX=0/ECX=0, `[0x157AB2]:=1` on mismatch), and the ladder gate `phase==2 && [rec+0x20]==0 && [0x157C5D]!=0` → `0x78599`.

- [ ] **Step 1: Failing test** — assert the head counter + the deferred install + the ladder-gate shape (stub the ladder).
- [ ] **Step 2: Implement** the skeleton; model the `[0x157C5D]` (gate) vs `[0x157C5C]` (latch) cells.
- [ ] **Step 3:** PASS; `make check`; M2 `cmp`.
- [ ] **Step 4: Commit** `feat(fu282): port the FUN_000782D0 record-0 machine skeleton`.

---

### Task 8: The `FUN_00077EAC` ladder (+ the shared predicates) — or legs

**Files:** `src/fifa96_loader/fifa96_keeper_cpu.c`; Test `tests/test_keeper_cpu.c`

**Interfaces:** FU283 — the head gates, the five arms (`0x1A`×3 / `0x1B` / `0x1C`, `ECX=1` invoke-now), the `FUN_000765C4 != 0` code-`4` fall-through (`0x781FB MOV EDX,0x4` → tail `0x782BB`); helpers `FUN_000765C4`/`FUN_00076B28`/`0x795B4`/the `0x114E04` **ramp table**.

- [ ] **Step 1: Failing test** — assert each arm installs the right row + `[0x157C2C]`, and the code-`4` fall-through.
- [ ] **Step 2: Implement** the ladder + the ported helpers. For the **stage-select** inputs that stay unported (`[0x14C1D4]`, the stage tables, the latches), **keep them numbered legs** — never a fabricated stage.
- [ ] **Step 3:** PASS; `make check`; M2 `cmp`. If a live stage index cannot be raised, the ladder is a **wired-but-unraised LIVE-LEG** row set (rows-27/29/2C precedent) — do not fake it.
- [ ] **Step 4: Commit** `feat(fu283): port the FUN_00077EAC keeper CPU ladder + the shared predicates`.

---

### Task 9: The RED-first reach test green + the visible camera acceptance

**Files:** Test `tests/test_engine_match_frame.c`; `tools/fifa96_play.c`

- [ ] **Step 1:** The T1 reach test now PASSES end-to-end: sit-5 → phase 9 → row `0x1E` → stage 5 → the release → `camera.pos_y == 0x50`, `tracked != 0`, and the row-0E window admits (`0x20 < pos_y < 0x70`) next granted frame.
- [ ] **Step 2: Mutation-RED rider** — stubbing the CPU step / the sit-5 emit / the `FUN_0006E598` bridge must break it. Record each.
- [ ] **Step 3: Layer-2 acceptance** — `make game` (`DISPLAY=:1`); confirm the follow/pan camera moves on the smoke — or, if the smoke window has no keeper restart, record an honest waiver with the granted-frame proof as the Layer-1 landing (spec §6 honest-stop).
- [ ] **Step 4: Commit** `test(fu273): the R11-L4 release reach — green (Layer 1) + the camera acceptance`.

---

### Task 10: The close-out (the census + the M2 re-pin + the leg register)

**Files:** `docs/ENGINE.md`, `docs/PROGRESS.md`, `README.md`, `AGENTS.md:90`, `tests/test_engine_m2.c`; `tests/golden/engine/m2-frames.txt` (only if re-pinning)

- [ ] **Step 1: Census** — if T3–T8 landed a live install+dispatch, recompute + update all four docs + pins in one commit; else hold `59/80` (FU288).
- [ ] **Step 2: M2** — re-pin (v23) with a written reason + diff if a live path changed; else byte-identical.
- [ ] **Step 3: Leg register** — append as **quoted errata**: FU289 (`[0x158882]` ≠ contact_kind; the cell is `[0x15888C]`; sole producer `0x8A772`); FU284 (row `0x1A` stays NULL); FU287 (phase-6 producer = leg T2b-L2); FU281 (double-dispatch); FU283 (`0x114E04` is a ramp); FU288 (ordering resolved; the `:5512→:5517` drift); the FU285/FU288 mutual-exclusion resolution; the FU282 §1.3-vs-§1.4 clarification.
- [ ] **Step 4: `docs/PROGRESS.md`** close-out entry.
- [ ] **Step 5:** `make check` green; M1 byte-identical. Commit + push `docs(fu273): phase-26c close-out — the census + the leg register`.

---

## Out of scope (routed onward as named legs)

- **Row `0x1A` wiring** — FU284 NO-GO (keep NULL; the emit is BX=1 → queue id-7 → FU-146).
- **The live phase-6 producer** — leg T2b-L2 (FU287).
- **The replay/overlay period/screen-machine port** — FU278.
- **The live `contact_kind==3` raiser** — leg FU289-L1 (the offside chain is callerless/settings-gated).

## Self-Review

- **Spec coverage:** the phase-26b §4 split is honored (this is 26b-ii); FU273 §4.2's tasks map to T3–T9, minus the FU284 NO-GO (dropped) and with the FU289 bounded win folded in as T2 (the user's option 3).
- **Placeholders:** none — each task names files, native addresses, the test, the command.
- **Type consistency:** `fifa96_keeper_cpu_*` are the helper names the keeper-CPU tests use; the cells match the FU281–FU288 citations.
- **Honest risks:** R11-L4 is **still phase-scale** (the installer + ladder remain unported in part); T5/T3 are the load-bearing wins; the census stays `59/80` unless a task proves a live install+dispatch. Any unreachable surface stays a numbered leg — never a fake — and the camera stays granted-frame-only if the smoke window cannot show it (the §6 honest-stop, as in 26a).

## Execution Handoff

**1. Subagent-Driven (recommended)** — fresh subagent per task + two-stage review. **2. Inline** — batch with checkpoints.
