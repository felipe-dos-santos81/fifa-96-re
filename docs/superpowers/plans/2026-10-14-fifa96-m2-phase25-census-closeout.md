# FIFA 96 M2 phase-25 census close-out — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Land the one surviving census candidate — the OL-65 row-07 staging fix that makes action `22` install AND dispatch (`58/80 → 59/80 = 73%`) — plus the phase-24/25 errata sweep and the honest leg register, with M2 kept byte-identical.

**Architecture:** The phase-25 recon-ahead wave (`FU256…FU263`) proved no gating producer un-gates a row except OL-65. This plan (a) re-confirms determinism, (b) applies the 2-line OL-65 staging fix + wires the row-22 slot behind a RED-first fixture and an M2 `cmp` gate, (c) lands the accumulated errata, (d) reconciles the census only if the row wires and M2 holds, and (e) closes the leg register.

**Tech Stack:** C11/CMake/CTest, ASan/UBSan (`-fno-sanitize-recover=all`), the null backend (determinism source of truth), SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase25-census-closeout-design.md`. Frozen recon slices: `docs/ghidra/FU256…FU263`.

## Global Constraints

- **Baseline:** `make check` **110/110**; wired **58/80 = 72%** (`35 action + 23 phase`); M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (**691 lines, immutable**); M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (**v22, 6522 lines**).
- **M2 is byte-identical** unless the OL-65 fix **provably** un-dormants a tape-visible path; then stop and apply the written-reason + frame-diff re-pin rule (never silently). M1 never moves.
- **The census rule:** a row counts only when a ported path **installs AND dispatches** it. `floor(100 × wired/80)`; a count change updates README title + `docs/ENGINE.md` + `docs/PROGRESS.md` + `AGENTS.md:90` + the `tests/test_engine_m2.c` pins **in the same commit**.
- **No fakes** (R15/R45/R49). Ghidra stays read-only (`/FIFA96.EXE`; LE globals at `+0x100000`).
- Every new wire is RED-first with a mutation-RED rider.
- Commits `feat`/`fix`/`docs(fuNNN)`/`test`/`chore`; work on `main`; push per dispatch.

## File Structure

| Path | Responsibility |
|---|---|
| `src/fifa96_engine/fifa96_match_handlers.c` | The row-07 gate staging (`:983-985`) + the action table (`:5448..5548`, the `0x22` slot) |
| `tests/test_engine_match_handlers.c` | The row-22 install/dispatch fixture (RED-first) + the mutation-RED rider |
| `tests/test_engine_m2.c` | The census pins + the phase-25 provenance/riders |
| `docs/ENGINE.md`, `docs/PROGRESS.md`, `README.md`, `AGENTS.md` | The leg register + the census reconciliation |
| `docs/ghidra/FU247…FU263` | The errata targets (appended errata only — prior sections never rewritten) |

---

### Task 1: Determinism + feasibility gate

**Files:** none (verification only).

**Interfaces:**
- Produces: the frozen baseline hashes every later task's `cmp` uses.

- [ ] **Step 1: Recompute the census.**

Run: `./build/test_engine_m2 2>&1 | grep -iE "wired|58/80|UNSUP"` (and read the printed census line).
Expected: `58/80 = 72%`, dispatch `21 UNSUP / 58 OK / 1 NOTF`.

- [ ] **Step 2: Prove M1/M2 byte-stability (≥3 replays).**

Run:
```bash
for i in 1 2 3; do ./build/test_engine_m1 | sha256sum; done
for i in 1 2 3; do ./build/test_engine_m2 | sha256sum; done
```
Expected: M1 `09b726b7…` ×3; M2 `b281a9b2…` ×3. If not stable → STOP; fallback = a fixture-only proof for Task 2.

- [ ] **Step 3: Record the baseline in the ledger.** No re-pin, no port. Commit nothing (verification only).

---

### Task 2: The OL-65 staging fix + the row-22 wire (the one census candidate)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_handlers.c:983-985` (the row-07 gate staging) and `:5522` (the `0x22` action-table slot)
- Test: `tests/test_engine_match_handlers.c`

**Interfaces:**
- Consumes: the engine pool fields `opp->cam_dz6f` (the native `word[+0x6F]`, see `:2547`) and `opp->face7d` (the native `+0x7D`); the gate predicate in `fifa96_action_handlers.c` (~`:1701-1705`).
- Produces: `fifa96_match_action_22` wired at the `0x22` slot; the install+dispatch fixture.

- [ ] **Step 1: Write the failing test.**

In `tests/test_engine_match_handlers.c`, add a test that stages a row-07 dispatch whose opponent's `cam_dz6f`/`face7d` make `fold10(entity_angle(word[+0x6D],word[+0x6F]) − word[+0x7D]) < 0x55`, then asserts `opponent_invoke` fires and row `22` installs AND dispatches (the native `0x816D1 MOV ECX,1 / MOV EDX,0x22 / CALL 0x7D9A4`, FU259).

```c
static void test_action_07_ol65_invoke_installs_row_22(void) {
  /* stage the opponent angle cells so the 0x816CC fold passes (<0x55),
   * then dispatch row 07 and assert the row-22 install + dispatch. */
  /* (fill from the existing row-07 fixture at test_action_handlers.c:800-814) */
  assert(install_fired == 1);
  assert(dispatched_code == 0x22);
}
```

- [ ] **Step 2: Run the test to verify it fails.**

Run: `ctest --test-dir build -R test_engine_match_handlers --output-on-failure`
Expected: FAIL — `opp_angle_z == 0` / `opp_face_word == 0`, so the fold never passes (`fifa96_entity_angle(lane_z, 0)` yields `±0x100`).

- [ ] **Step 3: Apply the 2-line staging fix.**

In `src/fifa96_engine/fifa96_match_handlers.c`, replace `:984-985`:
```c
      s->opp_angle_z = 0;                /* word[+0x6F] unmodeled (OL-65) */
      s->opp_face_word = 0;              /* +0x7D facing word unmodeled (OL-65) */
```
with:
```c
      s->opp_angle_z = opp->cam_dz6f;    /* native word[+0x6F] (OL-65, FU259) */
      s->opp_face_word = opp->face7d;    /* native +0x7D facing word (OL-65, FU259) */
```
(`s->opp_angle_x = opp->lane_z;` at `:983` stays; fix its alias-trap comment per Task 3.)

- [ ] **Step 4: Wire the row-22 slot.**

In the action table, replace `:5522` `{0x22, NULL, "…"}` with `{0x22, fifa96_match_action_22, "…"}` (keep the comment, append the FU259 provenance + the OL-65 closure).

- [ ] **Step 5: Run the test to verify it passes.**

Run: `ctest --test-dir build -R test_engine_match_handlers --output-on-failure`
Expected: PASS.

- [ ] **Step 6: Add the mutation-RED rider.**

Revert `s->opp_angle_z` to `0` in a scratch copy; re-run Step 5; confirm the test FAILS (RED). Restore.

- [ ] **Step 7: The M2 gate (the decision).**

Run: `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt && echo IDENTICAL`
- **If IDENTICAL:** the row is tape-dormant → proceed to Step 8 (census rises to 59/80).
- **If MOVED:** the fix un-dormants a tape-visible path (FU259-L1). **Per the M2 rail: revert the slot AND the fix** (`git checkout`), record a ledger Ruling, keep row `22` NULL/uncounted, and skip Step 8. The phase then reports the honest ceiling `58/80`.

- [ ] **Step 8: Commit (only if M2 identical).**

```bash
git add src/fifa96_engine/fifa96_match_handlers.c tests/test_engine_match_handlers.c
git commit -m "feat(ol65): the row-07 gate staging fix wires action 22 (row 22 installs+dispatches; census 59/80)"
```

---

### Task 3: The errata sweep (doc-only)

**Files:**
- Modify: `docs/ghidra/FU247_exclusions_and_tape.md` (or the closest FU-247 file), `docs/ENGINE.md`, `src/fifa96_engine/fifa96_match_handlers.c` (comments only), any FU-84 slice.

**Interfaces:**
- Consumes: `FU250` §2.3 (the FU247 erratum correction), `FU252`/`FU253` (OL-83), `FU256`/`FU258`/`FU261`/`FU262`/`FU263` (the drift legs).

- [ ] **Step 1: The FU247 OL-65 erratum correction.** Append a quoted erratum to FU247: its `§2.3` "erratum" was itself an alias-trap error — the gate reads `entity_angle(word[+0x6D], word[+0x6F]) − word[+0x7D]` (fresh `0x8169F`/`0x816A2` dword loads `>>16`), not `word[+0x6B]`; FU185-L6 stands (FU250).
- [ ] **Step 2: The OL-83 unification.** Append the ruling to the OL-83 sites: unify on `record.type` (native `+0x8E`); retire the `actor_type` alias (FU252/FU253). Correct the `ENGINE.md:4037` row-18 over-list (row `18` does **not** read `+0x8E`).
- [ ] **Step 3: The FU-84 §1 erratum.** Append: the reroll RNG is **not** discarded (the `0x6E598` id-0 reroll `rng&3 → {0,0x62,0x65,0}` via the `0x6E588` table) (FU254).
- [ ] **Step 4: The FU201-L1 owner.** Append: the only desired/current divergence candidate is `FUN_00041494` **case 5** (dynamic-only) (FU248/FU263).
- [ ] **Step 5: The doc drift.** Correct `ENGINE.md:929-930` (handler slots `:5012/:5111` → `:5500/:5603`), `ENGINE.md:944` (M2 sha `7b4b241a…` → `b281a9b2…`), the FU258-L3 frame-29 misattribution, and the row-0D comment ("sit-7's producers are unported arms, not 'no live caller'") (FU263-L3/FU258-L3/FU261).
- [ ] **Step 6: Commit.**

```bash
git add docs/ghidra docs/ENGINE.md src/fifa96_engine/fifa96_match_handlers.c
git commit -m "docs(fu263): the phase-24/25 errata sweep (FU247 OL-65, OL-83 unify, ENGINE.md drift)"
```

---

### Task 4: The census close-out (only if Task 2 wired the row)

**Files:**
- Modify: `README.md` (title), `docs/ENGINE.md` (the census line + the leg register), `docs/PROGRESS.md`, `AGENTS.md:90`, `tests/test_engine_m2.c` (the census pins + the phase-25 riders).

- [ ] **Step 1: Recompute the count.** `floor(100 × 59/80) = 73`.
- [ ] **Step 2: Update the four docs in one commit** — README title `73%`; `docs/ENGINE.md` "Action dispatch: 59/80"; `docs/PROGRESS.md` the phase-25 narrative; `AGENTS.md:90` `59/80 → 73%`.
- [ ] **Step 3: Add the phase-25 riders** to `run_census_riders`/the tape pins (the row-22 slot non-NULL, the OL-65 staging non-zero) with mutation-RED verification.
- [ ] **Step 4: Commit.**

```bash
git add README.md docs/ENGINE.md docs/PROGRESS.md AGENTS.md tests/test_engine_m2.c
git commit -m "docs: census 58/80 → 59/80 = 73% (OL-65 wires action 22)"
```

*(If Task 2's M2 gate failed, this task is skipped — the census holds at 58/80 and only the leg register is updated.)*

---

### Task 5: The leg-register close-out + acceptance

**Files:**
- Modify: `docs/ENGINE.md` (the residual-leg register), `docs/PROGRESS.md`.

- [ ] **Step 1: Append the resolved/forbidden legs** from the 8 slices: `FU256-L*` (row `0D` gated), `FU257-L*` (row `1A`), `FU258-L*` (row `03` tape-visible), `FU260-L*` (the installer domain), `FU261/262-L*` (the phase slots), `FU263-L*`.
- [ ] **Step 2: Acceptance.** Run `make check` (expect 110/110, sanitized); re-run the M1/M2 `cmp` (byte-identical).
- [ ] **Step 3: The honest smoke** (`DISPLAY=:1`, `make game`) — record the live reach, or an explicit waiver if no visible change is expected.
- [ ] **Step 4: Commit.**

```bash
git add docs/ENGINE.md docs/PROGRESS.md
git commit -m "docs(fu263): the phase-25 leg-register close-out"
```

---

## Self-Review

- **Spec coverage:** spec §1's census target → T1/T2/T4; the §4 wave → the frozen `FU256…FU263`; §6's M2 rail → T2 Steps 7–8; §5's T-structure → T1 (gate) / T2 (the port) / T3 (corrections) / T4 (census) / T5 (leg register). The design's "honest ceiling" caveat is honoured: T4 is conditional.
- **Placeholders:** the T2 test body is a scaffold pointing at the existing fixture (`test_action_handlers.c:800-814`) — the implementer fills it against the real fixture signature; every other step is exact.
- **Type consistency:** the field names (`opp->cam_dz6f`, `opp->face7d`, `s->opp_angle_z`, `s->opp_face_word`) are taken verbatim from `fifa96_match_handlers.c`; the gate address `0x816CC`/`0x816D1` is verified first-hand; the slot is `fifa96_match_action_22`.

## Handoff

Hand to `superpowers:subagent-driven-development`: T1 is a verification gate (no code); T2 is the one code task (RED-first + the M2 gate); T3–T5 are doc/register tasks. If T2's M2 gate fails, the phase is errata + leg-register only (the honest ceiling holds at 58/80).
