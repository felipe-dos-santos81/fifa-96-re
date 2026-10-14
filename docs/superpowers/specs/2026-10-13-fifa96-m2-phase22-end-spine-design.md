# FIFA 96 M2 phase-22 — the end spine (extra time to match over)

Date: 2026-10-13
Status: approved for planning (design approved in-chat: theme "the full extra-time
match end"; boundary/penalty-split chosen by the user).
Parent spec: `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`
Predecessor spec: `docs/superpowers/specs/2026-10-13-fifa96-m2-phase21-live-period-end-design.md`
Predecessor plan: `docs/superpowers/plans/2026-10-13-fifa96-m2-phase21-live-period-end.md`
Recon slices (to be produced by this phase's wave): `docs/ghidra/FU231…FU238`.
Deferred to phase-23: the **penalty shootout** (a separate subsystem; own spec→recon→plan).

## 1. Context

Phase-21 made the period-end spine **live**: the committed M2 tape **v21** drives the
FU-62 clock through two period completions — half-time (period 0 → 1) and a
differing-score regulation end → the extra-time chooser arm (`act 0xC`, phase
`0x13`) — and **stops at extra-time entry** (phase-21 §2 non-goal: no full ET match).

What is missing is the **end spine**: the extra-time halves (periods 2/3), the
end-of-match terminal, the match-over screen, and the return to the front-end. The
native machinery is frozen in **FU-228** (and FU-213/FU-64/FU-62):

- the **end word** `[0x158822]` (word, 0 = ongoing / 1 = regulation over / 2 = extra
  time over);
- the **end machine** `FUN_0008BAF0` (sole caller `0x8B664` in the per-frame clock
  `FUN_0008AF38`): writes the end word only on a gated terminal — the phase-`0xC`
  screen with the extra-time flag **clear** (`0x8BCD9` → `1`) or an **ET-half
  terminal** phase `0x13`/`0x14` (`0x8BD8A`/`0x8BDD4` → `2`, latched by
  `[0x10F360]`); flag **set** at phase-`0xC` writes phase `0x13` (the ET arm,
  `0x8BCC8`) and leaves the word `0`;
- the **frame-loop consumer** `FUN_0004B100`: the match-over screen (`[0x5FFC]=2`
  via `0x4B244`) is entered **only** when the end word is set (AX==1) **and** the
  period index is past half-time (`0x4B1C6 CMP [0x157AC2],2`); AX==2 takes the
  ET-over screen; AX==1 with the index < 2 takes the half-time block;
- the **screen timing** (`word[0x158818]`, the `0x960` camera hold, the
  `0x45001`/`[0x10F378]`/`[0x10F360]` gates).

Phase-21 merely **re-pointed** the engine's period-agnostic `mark_over` to the
native condition (past-half-time, flag-clear → exit) and deferred the end machine
(FU228 §4.2 / errata E4). Phase-22 **lands** it: the end word, the terminal
decision, the ET halves, the match-over screen, and the return.

### Baseline (verified first-hand at spec time, HEAD `7d5f4e1`)

- `make check` **110/110** (sanitized, strict).
- Wired **55/80 = 68%** (`33 action + 22 phase`); dispatch `24 UNSUP / 55 OK / 1 NOTF`.
- M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (691 lines, immutable).
- M2 `0b87fd18ad41948c8294a6cc46e40af02c8bfe5e92d708e09c3a1a2103c947c0` (**2522 lines**, v21).

## 2. Goal and non-goals

**Goal.** Turn the live match's **end** on: from the v21 tape's extra-time entry, the
match plays the extra-time halves, reaches the end-of-match terminal (end word `1`
regulation / `2` extra time), enters the match-over screen, and returns to the
front-end — proven by extending the M2 acceptance tape (a documented re-pin).

### Non-goals (YAGNI)

- **No penalty shootout.** A tied extra-time end is the **phase-23** subsystem; this
  phase records the shootout entry point as a numbered leg and does not model it.
- **No front-end/competition work** (FU-65); no audio (FU-229).
- **No dead-gate synthesis.** The dead bit-8 `[0x14C1D4]`/`[0x14C1D6]` extra-time
  gate (FU-215, FU-225-L5) is documented, never synthesized; the index-4 sit-8
  route stays a numbered leg.
- **No unrelated refactor**; no new front-end art.

## 3. The end spine (architecture and data flow)

```
FU-62 clock: sec == limit + aux  →  period++ (periods 0/1 regular, 2/3 extra time)
        │  the per-frame clock harness FUN_0008AF38
        ▼
FUN_0008BAF0 (the end machine) — the gated terminal decision
        ├─ phase 0x0C screen, extra-time flag CLEAR → end word := 1  (regulation over)
        ├─ phase 0x0C screen, flag SET              → phase := 0x13  (ET arm; word stays 0)
        └─ phase 0x13/0x14 ET-half terminal         → end word := 2  (ET over, [0x10F360] latch)
        ▼
FUN_0004B100 (the frame loop) — the consumer
        ├─ word != 0 AND period index >= 2 → mark_over  ([0x5FFC] := 2)   ← the match-over screen
        ├─ word == 2                       → the ET-over screen
        └─ word == 1 AND index < 2         → the half-time block (no over)
        ▼
the match-over screen (timing on word[0x158818] > 0x78) → EXIT → front-end
```

The engine currently compresses `OVER → POST → EXIT` into one step (FU228-L5) and
triggers it on the raw completion; phase-22 models the **end word** and the
terminal decision, and drives the screen from it.

## 4. The 8 parallel read-only recon slices

Dispatched after spec approval, before the implementation plan is written (the
phase-21 §4 / `2026-10-08-fifa96-m2-phase7-recon-ahead` precedent). Each is a
**read-only** Ghidra investigation producing one FU slice; **no code, no
rename/comment/label/function/script/project save**. First-hand bytes and
addresses; fresh `get_xrefs_to` for any reachability claim; alias traps checked.

| # | Slice | Investigates | Deliverable |
|---|---|---|---|
| 1 | **FU231** | `FUN_0008BAF0` (the end machine) whole body: the phase-`6`/`2`/`0xC`/`0x13`/`0x14` blocks, the timers (`word[0x158818]`), the gates (`0x45001`, `[0x10F378]`, `[0x10F360]`) | The exact terminal decision + the screen timing |
| 2 | **FU232** | `FUN_0004B100` (the frame loop) match-state branch: the AX==1/2 paths, the half-time block, the `mark_over` site `0x4B244`, the ET-over screen, the screen-2/3/4 transitions (FU228-L1) | The exact lifecycle mapping |
| 3 | **FU233** | The extra-time-half clock (periods 2/3 at `extra_length`) + the phase-`0x13`/`0x14` handlers and their terminal | The ET-half route + the terminal |
| 4 | **FU234** | The end-word harness return (`FUN_0008AF38` returns `[0x158822]`) + the front-end transition (`run_end`/EXIT, the leave/resume targets) | The return path |
| 5 | **FU235** | The match-over screen timing + the `0x960` camera hold + the `0x45001`/`[0x10F378]` gates + the presentation | The match-over screen fidelity |
| 6 | **FU236** | The regulation-end (word `1`) path: the phase-`0xC` flag-clear terminal + `mark_over` + the match-over screen, distinct from the ET end | The word-`1` route |
| 7 | **FU237** | The ET terminal + the winner/loser decision (phase `0x13`/`0x14` equal score → word `2`; differ → hold) and the penalty-shootout entry point | The ET-end decision + the phase-23 leg |
| 8 | **FU238** | The M2 tape extension: the exact directives/frames to play the ET halves + the terminal + the match-over + (D2) the return; the expected length; re-pin + frame-diff methodology | The pre-specified re-pin |

## 5. Implementation task structure (serialized SDD)

One implementer at a time. Dependency order: T1 → T2 → T3 → T4 → T5 → T6.

- **T1 — determinism + feasibility gate.** Extend the tape's scripted run past ET
  entry to the end terminal with the end policy still at its current state; prove
  ≥3-replay byte-stability; characterize the ET halves and the terminal. **No
  re-pin.** If not byte-stable → STOP; fallback = a fixture-only proof.
- **T2 — the end word + the terminal decision** (per FU231/FU236): add the engine
  end-word cell; port the minimal `FUN_0008BAF0` decision (flag-clear → `1`,
  flag-set → phase `0x13`; ET-half terminal → `2`).
- **T3 — the frame-loop lifecycle re-point** (per FU232): `mark_over` iff the end
  word is set **and** the period index ≥ 2; the ET-over screen path.
- **T4 — the extra-time halves live** (per FU233): the ET-half clock (periods 2/3)
  + the phase-`0x13`/`0x14` rows + the terminal.
- **T5 — the match-over screen + the return** (per FU234/FU235): the screen hold + the
  EXIT → front-end.
- **T6 — tape extension + re-pin + acceptance** (per FU238): extend the tape to play
  the ET halves + the terminal + the match-over + (D2) the return; re-pin M2 with the
  written reason + frame diff; update the docs; record the smoke.

**Census caveat (stated, not hidden).** The phase likely wires **zero new rows** (it
activates already-counted rows); if a row's live-dispatch status changes the census,
recompute `floor(100 × wired / 80)` and update the README title + `docs/ENGINE.md` +
`docs/PROGRESS.md` + `AGENTS.md` in the same commit.

## 6. Acceptance and the tape extension / re-pin

- The extended M2 tape plays the **extra-time halves** and the **end terminal**
  (regulation `word 1` / extra-time `word 2`), the **match-over screen**, and (D2)
  the **return to the front-end**.
- **M1 `09b726b7…` never moves.** The M2 re-pin carries the written reason + frame diff.
- `make check` green (sanitized); the end spine asserted **at the tape layer**.
- An honest `make game` smoke (`DISPLAY=:1`) records the live reach.

**Re-pin policy.** Additive where the evidence allows (the v21 prefix unchanged, new
frames appended — the phase-14 T3 v13→v14 and phase-21 T6 precedents). Where
additivity fails, document the diff line-by-line with the reason — never silently.
Any *unexplained* drift is a regression to investigate, never a re-pin.

## 7. Risks

1. **The ET-half length on the tape.** The ET halves at `extra_length` may extend the
   tape well past v21's 2522 lines; the caps (`M2_STEP_CAP`/`M2_TRANSCRIPT_CAP`) may
   need raising (FU238 / T1 owns the measured `N`).
2. **The match-over screen timing** (`word[0x158818]`, the `0x960` camera) may not be
   cleanly compressible; FU235 owns the fidelity verdict.
3. **The end-word decision fidelity** — the minimal port (FU231/FU236) must match the
   native's gated terminal, not the full camera/timer body.
4. **The ET terminal / shootout boundary** — a tied ET end enters the phase-23
   shootout; the tape must stop at (or disclose) that boundary (FU237).
5. **Tape additivity** — the ET halves may shift early frames if the staged period
   changes; characterized by the T1 probe and FU238.

## 8. Constraints carried in

- M1 `09b726b7…` immutable; the null backend is the determinism source of truth.
- The re-pin carries a written reason + frame diff (AGENTS.md golden rule).
- Ghidra stays **read-only**; `/FIFA96.EXE` case-sensitive; LE-image globals at `+0x100000`.
- No fakes: every unreachable native is a numbered leg with its native owner.
- ISO-gated tests skip cleanly without the ISO.
- Commits `feat` / `fix` / `docs(fuNNN)` / `test` / `chore`; work on `main`; push per dispatch.

## 9. Interfaces

- **The end word** `mr->end_word_158822` (`fifa96_match_run.c`) — new at T2.
- **The end machine decision** — the minimal `FUN_0008BAF0` port at T2.
- **The lifecycle trigger** — `mark_over` at T3 (re-pointed from phase-21's condition).
- **The extra-time halves** — the clock's periods 2/3 (`extra_length`) + the phase-`0x13`/`0x14` rows at T4.
- **The match-over screen + return** — the lifecycle at T5.
- **The tape scenario** (`tests/test_engine_m2.c`) — extended at T1/T6; `tests/golden/engine/m2-frames.txt` — re-pinned at T6.
- **The recon slices** `docs/ghidra/FU231…FU238` — appended errata only (prior sections never rewritten).

## 10. Decisions (approved in-chat)

- **D1 — match-over screen fidelity:** model the native **timing gate**
  (`word[0x158818]`); keep the `0x960` camera work a sink leg.
- **D2 — acceptance reach:** through the **return to the front-end** (the full
  lifecycle); stop short only if FU234/FU238 prove the front-end transition cannot be
  cleanly pinned.
- **D3 — ET-half clock on the tape:** 1 s periods → the tape grows past v21's 2522;
  raise the caps as needed (FU230-L4 precedent).
- **Penalty shootout:** deferred to **phase-23** (a separate subsystem); this phase
  records the shootout entry point as a numbered leg (FU237).
