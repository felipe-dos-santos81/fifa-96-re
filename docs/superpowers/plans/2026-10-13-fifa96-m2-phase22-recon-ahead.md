# FIFA 96 M2 phase-22 recon-ahead — the end spine (extra time to match over)

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` to execute this plan. The 8 recon slices are **read-only** and run **in parallel**. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Freeze the eight read-only evidence slices (`FU231…FU238`) that establish, first-hand, exactly how the native ends a match — the end machine, the frame-loop lifecycle, the extra-time halves, the terminal, the match-over screen, and the front-end return — so the phase-22 producer plan can be written without guessing.

**Architecture:** This is the phase-22 **Stage 1** (recon-ahead), exactly the phase-21 shape. The approved design (`docs/superpowers/specs/2026-10-13-fifa96-m2-phase22-end-spine-design.md` §4) sequences the recon before the implementation plan; the producer tasks (spec §5 T1–T6) are authored in a **Stage 2** plan off these frozen slices. Each of the 8 agents investigates one end-spine concern, writes only its draft, and hands off; the controller freeze-reviews and promotes to a canonical `docs/ghidra/FU2xx_*.md` in a serialized commit.

**Tech Stack:** Ghidra MCP (read-only), C11/CMake/CTest, ASan/UBSan, SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-13-fifa96-m2-phase22-end-spine-design.md` (approved). Predecessor plan: `docs/superpowers/plans/2026-10-13-fifa96-m2-phase21-live-period-end.md`. Recon-ahead precedent: `docs/superpowers/plans/2026-10-13-fifa96-m2-phase21-recon-ahead.md`.

## Global Constraints

- **Ghidra stays read-only.** Program argument exactly `/FIFA96.EXE` (case-sensitive). **No** rename / comment / label / function-create / script / project save. Standard MCP tools only.
- **Baseline (HEAD `aba4fbc`):** `make check` **110/110**; wired **55/80 = 68%**; M1 `09b726b7…` (691 lines, immutable); M2 `0b87fd18…` (**2522 lines**, v21).
- **Evidence floor (AGENTS.md):** every address/byte claim cites a fresh disassembly output or reconciled raw-byte read. Anything not statically provable becomes a **numbered open leg** with its native owner. LE-image globals at `+0x100000`.
- **Alias traps:** `dword[addr] >> 16` = the word at `addr+2`, `>> 24` = the byte at `addr+3`; verify CALL bytes (rel32); watch sign/width, inverted gates, gate-index misreads. Any "only/sole/exactly" or caller-count claim needs a **fresh** `get_xrefs_to`.
- **No code, no test, no shared-doc edits** by the recon agents. Each writes **only** its own draft file (+ a workspace report). The controller promotes drafts to canonical `docs/ghidra/FU2xx_*.md` and commits.
- **Scope guard:** one spine (extra time → match over → front-end). The penalty shootout and the front-end/competition layer are out of scope except where a slice must give a definitive in/out verdict (FU237 owns the shootout-entry leg).

## File Structure

| Path | Responsibility |
|---|---|
| `.superpowers/sdd/2026-10-13-fifa96-m2-phase22-recon-ahead/progress.md` | The SDD ledger (recovery map) |
| `.superpowers/sdd/2026-10-13-fifa96-m2-phase22-recon-ahead/w22-<n>-*.md` | The 8 recon drafts (git-ignored scratch) |
| `docs/ghidra/FU231_*.md … FU238_*.md` | The promoted canonical slices (controller, serialized commits) |
| `docs/ghidra/FU228_post_period_lifecycle.md`, `FU213_*.md`, `FU-64` | Prior slices — appended errata only, never rewritten |

---

### Task 1: W22-1 / FU231 — the `FUN_0008BAF0` end machine

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase22-recon-ahead/w22-1-end-machine.md`
**Promotes to:** `docs/ghidra/FU231_end_machine.md`

**Objective:** Map `FUN_0008BAF0`'s whole body and the exact gated terminal decision.

- [ ] **Step 1: Read the end machine first-hand.** `FUN_0008BAF0` (`0x8BAF0..0x8C065`; FU228 §2.2). Disassemble the whole body; record the phase dispatch (`{6, 2, 0x0C, 0x13, 0x14}`) and each block's timers/gates.
- [ ] **Step 2: The phase-`0xC` block.** The `word[0x158818]` timeline gates (`0x8BC57 CMP EAX,0x78`), `0x45001`, `[0x10F378]`, and the `0x8BCBF CMP byte[0x157AC0],0` extra-time-flag decision (clear → `[0x158822]=1`; set → phase `0x13`). Fresh bytes.
- [ ] **Step 3: The phase-`0x13`/`0x14` blocks.** The ET-half blocks: the `0x780` camera hold, the `[0x158816]>>16` timeline gates, the `[0x10F360]` latch, and the two `[0x158822]=2` sites (`0x8BD8A`/`0x8BDD4`) + the equal-score test.
- [ ] **Step 4: Verdict + legs.** State the exact terminal decision (the minimal port shape) and number any unprovable item. Hand off.

---

### Task 2: W22-2 / FU232 — the `FUN_0004B100` frame-loop lifecycle

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase22-recon-ahead/w22-2-frame-loop-lifecycle.md`
**Promotes to:** `docs/ghidra/FU232_frame_loop_lifecycle.md`

**Objective:** Decompose the `FUN_0004B100` match-state branch (FU228-L1) — the full end-word consumer.

- [ ] **Step 1: Read the branch first-hand.** `FUN_0004B100` (`0x4B100..0x4B2E7`); the head guard (`0x4B106 CMP word[0x158822],0`), the clock call (`0x4B1A6 CALL 0x8AF38`), the AX==0/1/2 dispatch.
- [ ] **Step 2: The AX==1 path.** `0x4B1C6 CMP [0x157AC2],2` → the half-time block (`0x4B1D2..0x4B21B`) vs the `mark_over` site (`0x4B244 MOV EAX,2; CALL 0x36BC0`) and the `0x4B220 JL 0x4B227 / CMP EAX,4` upper bound.
- [ ] **Step 3: The AX==2 ET-over path.** `0x4B257..0x4B2A8` and the word-clear (`0x4B297`).
- [ ] **Step 4: Verdict + legs.** The exact lifecycle mapping (which screen each word/index state enters) + numbered legs. Hand off.

---

### Task 3: W22-3 / FU233 — the extra-time-half clock + the phase-`0x13`/`0x14` rows

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase22-recon-ahead/w22-3-extra-time-halves.md`
**Promotes to:** `docs/ghidra/FU233_extra_time_halves.md`

**Objective:** Derive the ET halves (periods 2/3) and the phase-`0x13`/`0x14` handler rows.

- [ ] **Step 1: The ET-half clock.** The FU-62 clock's `period < 2 ? period_length : extra_length` (`fifa96_match_state.c`; FU-62/FU-223) — the periods 2/3 route and their completion.
- [ ] **Step 2: The phase-`0x13`/`0x14` rows.** The engine's phase rows (`fifa96_match_handlers.c`) + the native `0x110808`/`0x1107EC` slots; the ET-half handlers.
- [ ] **Step 3: The terminal.** How an ET half completes and what it writes (the FU231 terminal).
- [ ] **Step 4: Verdict + legs.** The ET-half route + its live preconditions. Hand off.

---

### Task 4: W22-4 / FU234 — the end-word harness return + the front-end transition

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase22-recon-ahead/w22-4-front-end-return.md`
**Promotes to:** `docs/ghidra/FU234_front_end_return.md`

**Objective:** Derive the clock harness's end-word return and the run's return to the front-end.

- [ ] **Step 1: The harness return.** `FUN_0008AF38`'s tail (`0x8B65B CMP byte[0x14C32A],0; JNZ; 0x8B664 CALL 0x8BAF0; … 0x8B678 MOV AX,[0x158822]; RET`) — the end-word return + the session gate.
- [ ] **Step 2: The front-end transition.** The native leave/resume targets, the `[0x5FFC]` screens (FU-64), and the engine's `run_end`/`EXIT` (`fifa96_match_run.c`); the mode switch to the front-end.
- [ ] **Step 3: Verdict + legs.** The exact return path the tape can pin. Hand off.

---

### Task 5: W22-5 / FU235 — the match-over screen timing + camera

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase22-recon-ahead/w22-5-match-over-screen.md`
**Promotes to:** `docs/ghidra/FU235_match_over_screen.md`

**Objective:** Derive the match-over screen's timing/hold and the camera work (the D1 fidelity verdict).

- [ ] **Step 1: The screen timing.** The `word[0x158818] > 0x78` (and `0xB4`/`0x168`) gates and the `0x45001`/`[0x10F378]` conditions (`FUN_0008BAF0` phase-`0xC`).
- [ ] **Step 2: The camera + presentation.** The `0x960`/`0x780` camera hold, `0x36200(3)`, the `0x8DCD4` sink; what is render-visible vs a sink.
- [ ] **Step 3: The screen machine.** `[0x5FFC]=2/3/4` (the `mark_over`/`resolve_over`/`request_exit` map) and the engine's compression (FU228-L5).
- [ ] **Step 4: Verdict + legs (D1).** Can the screen hold be modelled faithfully (or must it stay a compression)? Hand off.

---

### Task 6: W22-6 / FU236 — the regulation-end (word `1`) path

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase22-recon-ahead/w22-6-regulation-end-route.md`
**Promotes to:** `docs/ghidra/FU236_regulation_end_route.md`

**Objective:** Derive the regulation-end route (end word `1` on the flag-clear phase-`0xC` terminal) and how it differs from the ET end.

- [ ] **Step 1: The phase-`0xC` flag-clear terminal.** `0x8BCBF CMP byte[0x157AC0],0 ; JZ 0x8BCD9 ; MOV word[0x158822],1` (FU228 §2.2) + the `word[0x158818]` preconditions.
- [ ] **Step 2: The consumer.** The AX==1 + index≥2 → `mark_over` (Task 2) and the match-over screen.
- [ ] **Step 3: Contrast the routes.** State the branch that distinguishes the regulation over (word `1`) from the ET end (word `2`) and the cell that selects it.
- [ ] **Step 4: Verdict + legs.** Hand off.

---

### Task 7: W22-7 / FU237 — the extra-time terminal + the shootout entry

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase22-recon-ahead/w22-7-extra-time-terminal.md`
**Promotes to:** `docs/ghidra/FU237_extra_time_terminal.md`

**Objective:** Derive the ET terminal's winner/loser decision and pin the (phase-23) penalty-shootout entry point.

- [ ] **Step 1: The ET terminal.** The phase-`0x13`/`0x14` equal-score test (`0x8BD95 MOV AX,[0x157AC5]; CMP AX,[0x157AC7]; JNZ hold`) → `[0x158822]=2`; the `[0x10F360]` latch.
- [ ] **Step 2: The tie→shootout boundary.** What the native does on a tied ET end (the phase-23 entry): a definitive in/out verdict + the entry point.
- [ ] **Step 3: Verdict + legs.** State the ET-end decision and leg the shootout to phase-23. Hand off.

---

### Task 8: W22-8 / FU238 — the tape-extension design

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase22-recon-ahead/w22-8-tape-extension.md`
**Promotes to:** `docs/ghidra/FU238_tape_extension_v22.md`

**Objective:** Pre-specify the M2 tape extension (v21 → v22) and the re-pin.

- [ ] **Step 1: Read the tape generator.** `tests/test_engine_m2.c`: the scenario staging + directive replay (the v21 shape); the current `M2_M_*` directives and caps; the `M2_M_ET_ENTRY` anchor.
- [ ] **Step 2: Design the extension.** The exact directives/frames to play the **ET halves** + the **end terminal** + the **match-over screen** + (D2) the **return**; the expected new length range; whether the v21 2522-line prefix can stay byte-identical (additive).
- [ ] **Step 3: Specify the re-pin.** The written-reason wording, the frame-diff methodology (`cmp`, first differing line, hash-only vs `state=`), and the mutation-RED rider classes the T6 acceptance must add.
- [ ] **Step 4: Hand off.** Direct input to the Stage-2 producer plan's T6.

---

## Stage 2 (post-recon, separate plan)

After all 8 drafts are frozen and promoted, the controller writes
`docs/superpowers/plans/2026-10-13-fifa96-m2-phase22-end-spine.md` off the frozen
`FU231…FU238`, carrying spec §5's T1–T6 structure with exact code, per-task RED/GREEN
tests, mutation-RED discriminators, the T1 determinism gate, and the T6 tape re-pin.

## Self-Review

- **Spec coverage:** spec §4's 8 slices map 1:1 to Tasks 1–8 (FU231→Task 1 … FU238→Task 8).
- **Placeholders:** none — each task names its objective, exact targets, evidence floor, deliverable and handoff.
- **Consistency:** slice numbers (W22-1…W22-8 ↔ FU231…FU238), draft paths and the promotion path are consistent; the ownership mapping matches spec §1/§4.
- **Deferred:** the penalty shootout (spec §10) is recorded as a phase-23 leg (FU237), not dropped.

## Handoff

Hand this plan to `superpowers:subagent-driven-development`: dispatch a fresh
read-only subagent per task (all 8 in parallel), then the controller freeze-reviews
and promotes each draft to a canonical `docs/ghidra/FU2xx_*.md` in a serialized
commit per slice.
