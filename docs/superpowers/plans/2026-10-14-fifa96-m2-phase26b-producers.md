# FIFA 96 M2 phase-26b Stage-2 — the producer plan (the chain triggers + the ready fixes)

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Land the live **producers** (and the small ready correctness edits) that make the remaining phase-7 gameplay chains dispatch on a live engine path (Layer 1, proven headless), with the honest legs kept where the trigger is genuinely unported — off the frozen recon `docs/ghidra/FU273…FU280`.

**Architecture:** The recon wave (`FU273…FU280`) proved that for **four of the five families the bodies are already ported** — the missing pieces are **triggers** (a live `goal_armed`/sideline precondition, the row-`0x0C` contact entry) plus a few **small ready edits** (the `FU268-L10` byte, the `sp_157821` shadow). R11-L4 (the keeper CPU chain) is **NO-GO — phase-scale** (`FU273`) and is **out of this plan** (routed to phase-26b-ii / 26c). So Stage-2 is short and surgical: five producer/trigger tasks + three ready fixes + the close-out.

**Tech Stack:** C11/CMake/CTest; ASan/UBSan (`-fno-sanitize-recover=all`); the null backend as determinism source of truth; SDL3 smoke (`make game`, `DISPLAY=:1`); Ghidra MCP (read-only) for re-checks.

**Spec:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase26b-remaining-chains-r11l4-design.md` (binding) + parent `…phase26-live-gameplay-producers-design.md`. Frozen evidence: `docs/ghidra/FU273…FU280` (this plan's authority per task).

## Global Constraints

- **`make check` green** (sanitized, strict, `-Wall -Wextra -Werror`); run it before any push. Full suite ~143 s → allow ≥600 s.
- **M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` never moves** (691 lines, immutable). Verify with `./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt`.
- **M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (v22, 6522 lines) stays byte-identical** unless a producer genuinely changes a live path — then a **written reason + frame diff** and an honest re-pin. The phase prefers tape-dormant producers. Verify `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt`.
- **The live-chain rule (binding):** a chain is **live** only when a **ported producer raiser** raises its gate on a real engine path; a fixture that stages the gate does **not** count; a stand-in producer is a **fake** (R15/R45/R49). An unported/unreachable producer stays a **numbered leg** with its native owner.
- **Census:** 59/80 = 73% (`36 action + 23 phase`) at the start. A task that wires a NULL row into a live path is **honest upside**; if the count changes, update README title + `docs/ENGINE.md` + `docs/PROGRESS.md` + `AGENTS.md:90` + `tests/test_engine_m2.c` pins **in the same commit** (`floor(100 × wired/80)`).
- **Ghidra stays read-only** when re-checking (`/FIFA96.EXE`, case-sensitive; alias traps: `dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`; fresh `get_xrefs_to` for any caller count).
- **No fakes, no front-end work** (a front-end-produced cell is a leg), no audio subsystem, no new subsystem beyond the live producers. ISO-gated tests skip cleanly without the ISO.
- Commits: `feat`/`fix`/`docs(fuNNN)`/`test`/`chore`; work on `main`; push per task.

## File Structure

| Path | Responsibility |
|---|---|
| `src/fifa96_engine/fifa96_match_run.c` | the referee/offside seam (`:6597`), the goal-scan set-piece arms (`:3274-3340`), the receive arm (`:3769`), the phase arm (`:2853-2861`) |
| `src/fifa96_engine/fifa96_match_handlers.c` | the action/phase tables + the row-0x10 gate (`:1724`), the row-0x0C body (`:3329`), the keeper rows (`:5512-5522`) |
| `src/fifa96_engine/fifa96_match_input.c` *(or wherever `match_input_handler` lives)* | the pad `0x7CE38` handler (the `want 0x60` contact install) |
| `src/fifa96_loader/fifa96_outfield.c` | the row-`0x04` AI duel install (`:598-600`) |
| `src/fifa96_loader/fifa96_camera.c` | `fifa96_camera.pan_counter` (`:863`/`:893`) — the `[0x157821]` native cell |
| `src/fifa96_engine/fifa96_match_handlers.h` (`sp_157821`), `fifa96_match_run.h` (`:1487` contract) | the shadow field + the offside contract comment |
| `tests/test_engine_match_frame.c`, `tests/test_engine_m2.c` | the granted-frame reach tests + the census/golden pins |
| `docs/ENGINE.md`, `docs/PROGRESS.md`, `README.md`, `AGENTS.md` | the census line + the leg register (close-out only) |

**Dependency order:** T1 (gate) → T2..T7 (any order; T4–T7 are independent) → T8 (close-out).

---

### Task 1: The determinism + reachability gate

**Files:**
- Test: `tests/test_engine_m2.c` (read; the census pins)

**Interfaces:**
- Produces: the frozen baseline (census `36+23`, M1/M2 hashes) every later task compares against.

- [ ] **Step 1: Recompute the census.** Run `make check`; confirm `wired == 59` (`action_wired == 36u`, `phase_wired == 23u`) and `110/110` pass. If either moved, **stop and report** — do not continue.
- [ ] **Step 2: Prove the tape stability.** Run `./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt`, `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt` three times; confirm byte-identity and hash M1 `09b726b7…`, M2 `b281a9b2…`.
- [ ] **Step 3: Record the reachability map** (from FU280 §3): set-piece/restart + keeper + fouls are `granted-frame-only`; replay/overlay + the camera are legs. Write it into the task report.
- [ ] **Step 4: Commit.** `git commit -m "test: phase-26b T1 — the determinism + reachability gate"` (a test-only or no-op commit if nothing changed; if nothing changed, record the gate result in the report and skip the commit).

---

### Task 2: The `FU268-L10` offside-eligibility correctness fix

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c:6597`
- Modify: `include/fifa96_engine/fifa96_match_run.h:1487`
- Test: `tests/test_engine_match_frame.c` (the offside chain, `:4220-4295`)

**Interfaces:**
- Consumes: nothing.
- Produces: `fifa96_match_run_offside_reception` reading the **code** byte.

- [ ] **Step 1: Write the failing test.** Add to `tests/test_engine_match_frame.c` an assertion that a reception record whose **`code == 0x11`** is **rejected** as ineligible (native `+0x91`), distinct from a `type == 0x11` record (which can never occur — `type` is a 0–7 octant).

```c
static void test_offside_l10_reads_code_not_type(void) {
    /* native 0x79EF7/0x79F00 read code (+0x91); a record with code == 0x11 is ineligible */
    fifa96_match_entity e = {0};
    e.type = 0u;   /* the 0-7 octant — never 0x11 */
    e.code = 0x11u;
    assert(fifa96_match_offside_eligible(&e) == 0u);   /* was 1u before the fix */
}
```

- [ ] **Step 2: Run it, verify it fails.** `ctest --test-dir build -R test_engine_match_frame --output-on-failure` → FAIL (`eligible == 1u`).
- [ ] **Step 3: Apply the fix.** In `fifa96_match_run.c:6597` change the first disjunct `own->type != 0x11u` → **`own->code != 0x11u`** (the other three disjuncts already read `code`). Update the `match_run.h:1487` contract comment "type != 0x11" → "code != 0x11".
- [ ] **Step 4: Run it, verify it passes.** Same command → PASS. Then `make check` → 110/110; `cmp` M2 against the golden (the offside chain is dormant ⇒ **no re-pin**).
- [ ] **Step 5: Commit.** `git commit -m "fix(fu276): the L10 offside-eligibility byte — read code, not type"`

---

### Task 3: Unify the `sp_157821` shadow cell

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_handlers.c:1724` (the row-0x10 gate), `:832` (the field decl)
- Modify: `src/fifa96_engine/fifa96_match_run.c` (`:2087`/`:2219` seeds, `:4049` probe)
- Test: `tests/test_engine_m2.c` (or a focused unit)

**Interfaces:**
- Consumes: `fifa96_camera.pan_counter` (`fifa96_camera.c:863`/`:893`, the native `[0x157821]`).
- Produces: the row-0x10 gate reading the **same** counter the camera writes.

- [ ] **Step 1: Write the failing test.** Assert the row-0x10 re-queue gate observes the camera pan counter: seed `camera.pan_counter = 1`, run the row-0x10 stage-4 gate, assert it does **not** take the `<= 0` exit.

```c
static void test_row10_gate_reads_camera_pan_counter(void) {
    /* native: row 0x10 reads [0x157821] == fifa96_camera.pan_counter */
    fifa96_match_run mr = {0};
    mr.camera.pan_counter = 1;
    assert(fifa96_match_row10_requeue(mr) != 0);   /* was 0 — the shadow reads mr.sp_157821=0 */
}
```

- [ ] **Step 2: Run it, verify it fails.**
- [ ] **Step 3: Apply the fix.** Make the row-0x10 gate read `mr->camera.pan_counter` (drop the separate `sp_157821` shadow) — or, if the seam forbids it, alias the two fields and document why. Keep the native semantics (clear on camera reset, INC per pan frame while `word[0x1577F0] != 0`, cap `0x64`).
- [ ] **Step 4: Run it, verify it passes + `make check` + `cmp` M2** (the pan is event-starved ⇒ no re-pin expected; if M2 moves, written reason + diff).
- [ ] **Step 5: Commit.** `git commit -m "fix(fu274): unify the row-0x10 gate on the camera pan counter (the [0x157821] shadow)"`

---

### Task 4: The contact-gate live proof (Layer 1)

**Files:**
- Test: `tests/test_engine_match_frame.c` (a granted-frame contact scenario)
- Read: `src/fifa96_loader/fifa96_outfield.c:598-600` (the row-0x04 duel), the pad `0x7CE38` handler, `src/fifa96_engine/fifa96_match_handlers.c:3329` (the row-0x0C body)

**Interfaces:**
- Consumes: the ported chain row-`0x04` duel / pad `0x7CE38` → row-`0x0B` → row-`0x0C` → `fifa96_match_run_contact` (already unit-proven).
- Produces: a **headless granted-frame** proof that a live path raises the contact gate.

- [ ] **Step 1: Write the failing test.** Drive a granted-frame scenario where the row-`0x04` AI duel installs row `0x0B` with invoke (force the RNG/position/score terms, or press `0x60`), then assert `fifa96_match_run_contact` runs and `ref_machine` moves off `REF_NONE` (and the act-3/phase-0x19 chain dispatches).

```c
static void test_contact_gate_reachable_via_row04_duel(void) {
    fifa96_match_run mr = {0};
    match_granted_frame_setup(&mr);
    match_force_duel_rng(&mr);            /* the 1-in-64 draw + the metric/score terms */
    fifa96_match_run_frame(&mr);
    assert(mr.ref_machine != REF_NONE);   /* FAILS today — no live path raises the gate */
}
```

- [ ] **Step 2: Run it, verify it fails** (the gate is unraised).
- [ ] **Step 3: Implement the minimal live path.** If the duel terms are already reachable, this is a **test-only** task (no engine change). If a term is modelled-away, port the smallest faithful piece so the live path can raise the gate — **no stand-in** (a fixture that stages `ref_machine` is a fake). Cite the native owner (`fifa96_outfield.c:599` / the pad handler `fifa96_match_run.c:699`).
- [ ] **Step 4: Run it, verify it passes + `make check` + `cmp` M2.** A live contact on the tape would move M2 — if it does, written reason + frame diff; the preferred landing is a **granted-frame test only** (tape-dormant).
- [ ] **Step 5: Commit.** `git commit -m "test(fu276): headless reach proof — the contact gate via the row-0x04 duel"`

---

### Task 5: The offside live producer (the `0x7A448` call into the live receive arm)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c:3769` (`fifa96_match_run_ball_receive`), `:6544` (`fifa96_match_run_offside_reception`)
- Read: `docs/ghidra/FU276_fouls_offside_contact.md` §2.2/§2.4 (the native arm `FUN_0007A084 0x7A448`)
- Test: `tests/test_engine_match_frame.c`

**Interfaces:**
- Consumes: `fifa96_ref_offside_check` (ported), `fifa96_match_run_offside_reception` (callerless today).
- Produces: the receive arm calling the offside check behind `field_4c2f2`.

- [ ] **Step 1: Write the failing test.** With `field_4c2f2 != 0` and an eligible reception record, assert the live receive path calls the offside check (a live offside event is produced).

```c
static void test_offside_reachable_from_live_receive(void) {
    fifa96_match_run mr = {0};
    match_granted_frame_setup(&mr);
    mr.config.field_4c2f2 = 1u;
    match_receive_eligible(&mr);              /* a reception record, code not in {0x10,0x1D,0x1E} */
    fifa96_match_run_frame(&mr);
    assert(mr.referee.offside_checked == 1u); /* FAILS — the receive arm drops 0x7A448 */
}
```

- [ ] **Step 2: Run it, verify it fails.**
- [ ] **Step 3: Implement.** Port the `0x7A448 → FUN_00079D5C` call into `fifa96_match_run_ball_receive` (behind the `field_4c2f2` gate), replacing the declared leg at `match_run.c:3768`. Wire the eligible-state gate exactly as native (`code != 0x11` + `code ∉ {0x10,0x1D,0x1E}`).
- [ ] **Step 4: Run it, verify it passes + `make check` + `cmp` M2** (offside defaults off ⇒ dormant ⇒ no re-pin; a live offside on the tape needs a written reason + diff).
- [ ] **Step 5: Commit.** `git commit -m "feat(fu276): port the 0x7A448 offside call into the live receive arm"`

---

### Task 6: The set-piece taker triggers (rows `0x10`/`0x12`/`0x13`)

**Files:**
- Read: `docs/ghidra/FU274_set_piece_live_arm.md` §3 (the trigger analysis)
- Modify (only if needed): `src/fifa96_engine/fifa96_match_run.c:3274-3340` (the goal-scan arms)
- Test: `tests/test_engine_match_frame.c`

**Interfaces:**
- Consumes: the ported scanner (`fifa96_match_run_goal_scan`) + the adjudicator (`match_run_ref_step_restart`) + the `goal_armed` armer (`fifa96_match_goal_arm`).
- Produces: a headless proof that a live path installs rows `0x10` (throw-in) and `0x12`/`0x13` (foul).

- [ ] **Step 1: Write the failing test (throw-in).** A granted-frame scenario driving `goal_armed && phase == 2 && |snap_z| <= 0xB20` (the sideline excursion) → assert row `0x10` dispatches (`M2_TAKER_ROWS_MASK & (1<<0x10)`).

```c
static void test_throw_in_taker_reachable(void) {
    fifa96_match_run mr = {0};
    match_granted_frame_setup(&mr);
    match_force_goal_armed_sideline(&mr);   /* phase 2 + |snap_z| <= 0xB20 */
    fifa96_match_run_frame(&mr);
    assert(mr.dispatched_ok & (1u << 0x10));
}
```

- [ ] **Step 2: Run it, verify it fails.**
- [ ] **Step 3: Write the failing test (foul taker).** A granted-frame contact (Task 4) → act-2 stage 4 → assert row `0x12` or `0x13` dispatches.
- [ ] **Step 4: Run it, verify it fails.**
- [ ] **Step 5: Implement only the missing trigger.** Per FU274 §4.1, **no port work** is needed for the arms — the gap is the **live precondition**. If a condition is modelled away (e.g. the sideline excursion cannot arise), port the smallest faithful producer; **do not fake** a `goal_armed`/phase writer. If the trigger proves genuinely unreachable, keep rows `0x10`/`0x12`/`0x13` as **LIVE-LEG** with the owner and record it in the report.
- [ ] **Step 6: Run, verify pass + `make check` + `cmp` M2.** A live taker install on the tape would move M2 (the preferred landing is granted-frame-only; the tape already isolates the taker rows).
- [ ] **Step 7: Commit.** `git commit -m "feat(fu274): the set-piece taker triggers — throw-in + foul live reach"`

---

### Task 7: The keeper row-`0x1D` trigger (the goal-scan sit-4)

**Files:**
- Read: `docs/ghidra/FU277_keeper_restart_trigger.md` §4.1 (the split: `0x1D` ← goal-scan sit-4; `0x1E` ← R11-L4)
- Test: `tests/test_engine_match_frame.c`

**Interfaces:**
- Consumes: `fifa96_match_run_goal_scan` (`run.c:3316`, sit-4-capable) + the goal-kick arm (`|snap_z| > 0xB20`, `goal_zone == 0`).
- Produces: a headless proof that a live path installs row `0x1D`.

- [ ] **Step 1: Write the failing test.** A granted-frame goal-kick scenario (`goal_armed && phase ∈ {2,0x10} && goal_zone == 0 && |snap_z| > 0xB20`) → sit 4 → phase 8 → assert row `0x1D` dispatches and its restart emit fires.

```c
static void test_keeper_1d_goal_kick_reachable(void) {
    fifa96_match_run mr = {0};
    match_granted_frame_setup(&mr);
    match_force_goal_kick(&mr);             /* sit 4 -> phase 8 */
    fifa96_match_run_frame(&mr);
    assert(mr.dispatched_ok & (1u << 0x1D));
}
```

- [ ] **Step 2: Run it, verify it fails.**
- [ ] **Step 3: Implement only the missing trigger** (no row-`0x1E` work — that is R11-L4, out of scope). **Do not** port the keeper ladder or fake a `goal_armed` producer. If unreachable, keep row `0x1D` as **LIVE-LEG** (`FU277-L1`) and record it.
- [ ] **Step 4: Run, verify pass + `make check` + `cmp` M2.**
- [ ] **Step 5: Commit.** `git commit -m "test(fu277): the keeper row-0x1D reach — the goal-scan sit-4 trigger"`

---

### Task 8: The replay/overlay ruling + the close-out

**Files:**
- Modify: `docs/ENGINE.md` (the known-gaps rows + the census line), `docs/PROGRESS.md`, `README.md` (title), `AGENTS.md:90`, `tests/test_engine_m2.c` (pins — only if the census moved)
- Test: `tests/test_engine_m2.c`

**Interfaces:**
- Consumes: every task's verdict.

- [ ] **Step 1: Record the replay/overlay leg.** Per `FU278` §3.3, `replay.state`/`sub.active`/`overlay.armed` are **LEG** (the whole period/screen-machine caller chain is unported) and `ball_row` is **pinned dormant** — append the honest leg (quoted errata; owner `FUN_0004B100`/`FUN_00052030`). **No port** this phase.
- [ ] **Step 2: Sync the doc errata the wave filed** (as quoted errata): `docs/ENGINE.md:1754` ("the FU-150 entries have no live producer" → "**producer-wired, unraised**", `FU276`); `docs/ENGINE.md:1489`/`docs/PROGRESS.md:1905` `FU241-L1` (port-closed; a test-coverage residual, `FU279`).
- [ ] **Step 3: Recompute the census.** Recompute `floor(100 × wired/80)`. If a task wired a NULL row into a live path, update **README title + `AGENTS.md:90` + `docs/ENGINE.md` + `docs/PROGRESS.md` + `tests/test_engine_m2.c` pins in the same commit**. If it held at 59/80, no title change.
- [ ] **Step 4: Update the leg register** — append the resolved/forbidden legs as quoted errata (prior sections never rewritten).
- [ ] **Step 5: `make check` + `cmp` M1/M2.** Green; M1 byte-identical; M2 byte-identical or a written re-pin.
- [ ] **Step 6: Commit + push.** `git commit -m "docs(fu280): phase-26b close-out — the leg register + the census"` then `git push origin main`.

---

## Out of scope (routed onward)

- **R11-L4 (phase-26b-ii / 26c):** the keeper CPU chain (`FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC`), rows `0x1A`/`0x1B`, the phase-6/`0x1F` chain, the claim bridge — **NO-GO phase-scale** (`FU273`, ~9–11 tasks). It gates the camera/pan/follow and the keeper row `0x1E`.
- **The camera producer** (FU264-L1/L2) — rides R11-L4.

## Self-Review

- **Spec coverage:** spec §4's T-structure maps to T1 (the gate), T2..T7 (one live producer/fix per chain), T8 (the close-out). Spec §6's honest-stop rule is carried in Global Constraints + T6/T7 Step 3. §2's non-goals (no census chase, no fakes, no front-end/audio) are in Global Constraints.
- **Recon coverage:** FU274→T3/T6; FU275→T3 (the shadow); FU276→T2/T4/T5; FU277→T7; FU278→T8; FU279→T8 (the sink legs); FU273→out-of-scope; FU280→T1/T8.
- **Placeholders:** none — each task names its files, the exact edit (`match_run.c:6597`, the `sp_157821` field), the test and the command.
- **Type consistency:** `fifa96_match_offside_eligible`, `fifa96_match_row10_requeue` are the helper names the tests use; the field names (`own->code`, `mr.camera.pan_counter`, `mr.config.field_4c2f2`) match the frozen slices' citations.
- **Honest risks:** T4/T6/T7 may land as **granted-frame tests** (Layer 1) with the tape unmoved; if a trigger proves unreachable, the row stays a **LIVE-LEG** — never a fake.

## Execution Handoff

**1. Subagent-Driven (recommended)** — fresh subagent per task + two-stage review (use `superpowers:subagent-driven-development`).
**2. Inline Execution** — batch with checkpoints (use `superpowers:executing-plans`).
