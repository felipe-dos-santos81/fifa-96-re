# FIFA 96 M2 phase-21 recon-ahead — make the period-end spine live

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` to execute this plan. The 8 recon slices are **read-only** and run **in parallel**. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Freeze the eight read-only evidence slices (`FU223…FU230`) that establish, first-hand, exactly which loader/config producers the live period-end spine needs — so the phase-21 producer plan can be written without guessing.

**Architecture:** This is the phase-21 **Stage 1** (recon-ahead). The approved design (`docs/superpowers/specs/2026-10-13-fifa96-m2-phase21-live-period-end-design.md` §4) sequences the recon **before** the implementation plan; the producer tasks (spec §5 T1–T6) are authored in a **Stage 2** plan off these frozen slices. Each of the 8 agents investigates one spine producer/gate, writes only its draft, and hands off; the controller freeze-reviews and promotes to a canonical `docs/ghidra/FU2xx_*.md` in a serialized commit.

**Tech Stack:** Ghidra MCP (read-only), C11/CMake/CTest, ASan/UBSan, SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-13-fifa96-m2-phase21-live-period-end-design.md` (approved). Predecessor plan: `docs/superpowers/plans/2026-10-13-fifa96-m2-phase20-sit8-completion.md`. Recon-ahead precedent: `docs/superpowers/plans/2026-10-08-fifa96-m2-phase7-recon-ahead.md`.

## Global Constraints

- **Ghidra stays read-only.** The program argument is exactly `/FIFA96.EXE` (case-sensitive). **No** rename / comment / label / function-create / script / project save. Standard MCP tools only (scripts are disabled).
- **Baseline (verified first-hand at spec time, HEAD `e02a547`):** `make check` **110/110** (sanitized); wired **55/80 = 68%** (`33 action + 22 phase`; dispatch `24 UNSUP / 55 OK / 1 NOTF`); M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (691 lines, immutable); M2 `441415fc423b05b2957f0a0bd51efa44f7f4651b4f8c07fcd6fb5ed6a8da9506` (792 lines, v20 strata).
- **Evidence floor (AGENTS.md):** every address/byte claim cites a fresh disassembly output or a reconciled raw-byte read. Anything not statically provable becomes a **numbered open leg** with its native owner — never asserted. LE-image globals sit at `+0x100000` from the file offset.
- **Alias traps (check before trusting a read):** `dword[addr] >> 16` is the word at `addr+2` and `>> 24` the byte at `addr+3`; verify CALL bytes (rel32) before trusting a call target; watch sign/width errors, inverted gate flags, gate-index misreads. Any "only/sole/exactly" or caller-count claim needs a **fresh** `get_xrefs_to` in the same session.
- **No code, no test, no shared-doc edits** by the recon agents. Each agent writes **only** its own draft file. The controller promotes drafts to canonical `docs/ghidra/FU2xx_*.md` and commits.
- **Scope guard:** the phase is one spine (period end → half-time / extra-time entry). The audio ring and the full extra-time match end are out of scope except where a slice must give a definitive in/out verdict.

## File Structure

| Path | Responsibility |
|---|---|
| `.superpowers/sdd/2026-10-13-fifa96-m2-phase21/progress.md` | The SDD ledger (recovery map): per-slice status + BASEs |
| `.superpowers/sdd/2026-10-13-fifa96-m2-phase21/w21-<n>-*.md` | The 8 recon drafts (git-ignored scratch) |
| `docs/ghidra/FU223_*.md … FU230_*.md` | The promoted canonical slices (controller, serialized commits) |
| `docs/ghidra/FU215_sit8_arming_writer.md` etc. | Prior slices — appended errata only, never rewritten |

---

### Task 1: W21-1 / FU223 — the period-length + config producer

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase21/w21-1-period-length-config.md`
**Promotes to:** `docs/ghidra/FU223_period_length_config.md`

**Objective:** Decide what sets a live match's period length (the FU-62 clock's
`limit`/`aux`), and whether the engine's staged default suffices or a producer must be ported.

- [ ] **Step 1: Read the engine's current clock/config model first-hand.**
  `read` `include/fifa96_engine/fifa96_clock.h`, `src/fifa96_engine/fifa96_clock.c`,
  the `fifa96_match_config` struct + its staging (`fifa96_match_run.c` begin /
  `fifa96_match_run_init`, the FU-68 default-settings handoff), and how
  `mr.clock_period_ended` is raised. Record the exact current `limit`/`aux` values.

- [ ] **Step 2: Derive the native producers.** Trace the native clock-completion
  inputs: the `0x105270`-family config block (**FU215-L5**, owner the
  input-options slice / FU-148 §12); the range words `[0x14C1D4]`/`[0x14C1D6]`
  (the bit-8/bit-9 gate, **FU208-L1/FU215-L1**); and where a live match reads the
  period/extra-time length. Use `disassemble_bytes` / `analyze_function` with
  fresh reads.

- [ ] **Step 3: Answer the decisive question.** State, first-hand, whether a live
  period length is: (a) a front-end/competition setting (FU-65-gated → engine
  stand-in), or (b) a match-init constant the engine already stages. Give the exact
  address for whichever holds.

- [ ] **Step 4: Evidence-check + legs.** Every claim cites a fresh output; each
  unprovable item is a numbered leg with its native owner. Then hand the draft to
  the controller.

---

### Task 2: W21-2 / FU224 — the roll + extra-time-flag cells and their front-end producers

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase21/w21-2-roll-flag-producers.md`
**Promotes to:** `docs/ghidra/FU224_roll_flag_producers.md`

**Objective:** Pin the two live cells — the period-index roll and the OL-85
extra-time flag — and derive their front-end match-init producers.

- [ ] **Step 1: Identify the native cells.** `mr->global_157ac2` (period-index
  roll, FU-213) ≙ `[0x157AC2]`; `mr->global_157ac0` (OL-85 flag, FU-213/FU-207) ≙
  `[0x157AC0]`. Confirm both first-hand (`read_memory` / xrefs) and record the
  exact widths.

- [ ] **Step 2: Derive the producers.** `FUN_00038630` `0x38990` for `[0x157AC2]`
  (**FU215-L2**); `0x389A4` for the OL-85 flag (**FU207-L2**). Read the producer
  bodies and their surrounding front-end/competition context (FU-65: is this
  keyboard/mouse-runtime-gated?).

- [ ] **Step 3: Trace the setters.** The W2 `0x8B2AC` / W4 `0x8B3A2` / W5 `0x8B5CD`
  tie-break setters (FU-211/FU-213) and the roll semantics (when the index
  increments to 4).

- [ ] **Step 4: Verdict + legs.** State whether the front-end producers are
  reachable from the ported front-end or must be a disclosed engine-side staging
  stand-in (the match-config-default pattern). Number the legs; hand off.

---

### Task 3: W21-3 / FU225 — the `FUN_00088860` chooser and its live caller

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase21/w21-3-chooser-live-caller.md`
**Promotes to:** `docs/ghidra/FU225_period_end_chooser.md`

**Objective:** Pin the native period-end chooser's exact live invocation and any
delta between it and the engine's `phase_drive` mirror.

- [ ] **Step 1: Read the native chooser.**   `FUN_00088860` (the period-end arm;
  FU-196 §3 / FU-213). Disassemble the whole body; record its arguments, branches
  and out-values (the derived `fifa96_action_phase_period_end` mirror is
  `src/fifa96_loader/fifa96_action_handlers.c`, declared in
  `include/fifa96_loader/fifa96_action_handlers.h`).

- [ ] **Step 2: Derive the live caller `0x74244`** (**FU215-L3**; noted as having
  no Ghidra function). Establish its enclosing function, its arguments and its
  reachability (fresh `get_xrefs_to`).

- [ ] **Step 3: Diff against the engine mirror.** Enumerate where
  `fifa96_match_run_phase_drive` + `fifa96_action_phase_period_end` differ from the
  native call (args, tie-break, gate polarity). Name every divergence.

- [ ] **Step 4: Verdict + legs.** State whether flipping `period_setup_live` alone
  suffices, or the mirror must be reconciled. Hand off.

---

### Task 4: W21-4 / FU226 — the sit-8 route and RNG determinism

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase21/w21-4-sit8-rng-determinism.md`
**Promotes to:** `docs/ghidra/FU226_sit8_rng_determinism.md`

**Objective:** Prove whether the sit-8 route's RNG draw is deterministic under the
null backend (the T1 re-pin precondition) and pin the session gate's live value.

- [ ] **Step 1: Read the sit-8 arm.** `FUN_0008A938` `0x8AE08..0x8AE54` (the bind —
  landed phase-20 T3; see FU-216/FU-217) and the RNG side `0x888C8`.

- [ ] **Step 2: Read the engine RNG.** The null backend's determinism source
  (`platform_null.c`) and the engine RNG the sit-8 path would consume
  (`fifa96_entity_*` RNG helpers). Trace whether the draw is seeded deterministically
  per run.

- [ ] **Step 3: Pin the session gate.** `[0x14C32A]` (**FU208-L8/FU218-L3**): its
  live value at a fresh match begin and whether the sit-8 route is gated on it.

- [ ] **Step 4: Verdict.** State plainly whether the live sit-8 draw is
  null-backend-safe. If not, say so — the T1 gate will stop on it. Hand off.

---

### Task 5: W21-5 / FU227 — the half-time (situation-1) route

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase21/w21-5-half-time-route.md`
**Promotes to:** `docs/ghidra/FU227_half_time_route.md`

**Objective:** Derive the half-time path (period 1 → period 2) and its live
preconditions, distinct from the extra-time route.

- [ ] **Step 1: Read the situation-1 arm.** `0x888F2` (the controlled-side
  situation-1 dispatch inside `FUN_00088860`; FU-196 §3).

- [ ] **Step 2: Trace to the kickoff.** situation 1 → act 1 → phase `0x17`
  (phase-18 T4 / FU-203). Confirm the route and the phase-`0x17` preconditions.

- [ ] **Step 3: Contrast the two routes.** State the branch that distinguishes
  half-time (situation 1) from extra-time entry (situation 8) and the cell that
  selects it.

- [ ] **Step 4: Verdict + legs.** Hand off.

---

### Task 6: W21-6 / FU228 — the post-period lifecycle and the end word

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase21/w21-6-post-period-lifecycle.md`
**Promotes to:** `docs/ghidra/FU228_post_period_lifecycle.md`

**Objective:** Map the post-period lifecycle the tape's reach needs, and the
end-of-match word.

- [ ] **Step 1: Read the engine lifecycle.** `fifa96_match_run` OVER→POST→EXIT
  (`resolve`), and where a period completion currently sends the run.

- [ ] **Step 2: Read the native end word.** `[0x158822]` / `FUN_0008BAF0`
  (**FU213-L3**) and its native writer/callers (fresh xrefs).

- [ ] **Step 3: Verdict.** State the minimal mapping the tape (stopping at ET
  entry) needs, and whether the end machine is required at all for this phase.
  Hand off.

---

### Task 7: W21-7 / FU229 — the whistle/audio scope verdict

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase21/w21-7-audio-verdict.md`
**Promotes to:** `docs/ghidra/FU229_whistle_audio_scope.md`

**Objective:** Give a definitive in/out-of-scope verdict on the `0x974DC(0x1E)`
whistle vs the null-backend PCM hash.

- [ ] **Step 1: Read the whistle call chain.** `0x974DC` → the `0x155C8C` ring →
  the voice trigger (`FUN_00065CC0`/`FUN_000A7705`; **FU221-L1**), as fired by the
  phase-`0x1D`/`0x21`/`0x22` bodies (all staged as `ref_whistle` in the port).

- [ ] **Step 2: Read the null backend's hash.** `src/fifa96_engine/platform_null.c`:
  what exactly it hashes (Mode-X planes + palette + PCM) and how staged requests
  reach it.

- [ ] **Step 3: Verdict.** State whether the live whistle can change the null-backend
  hash. If it cannot (the audio sink is unported), the re-pin needs no audio work —
  record that as the finding. Hand off.

---

### Task 8: W21-8 / FU230 — the tape-extension design

**Draft:** `.superpowers/sdd/2026-10-13-fifa96-m2-phase21/w21-8-tape-extension.md`
**Promotes to:** `docs/ghidra/FU230_tape_extension.md`

**Objective:** Pre-specify the M2 tape extension and the re-pin.

- [ ] **Step 1: Read the tape generator.** `tests/test_engine_m2.c`: the scenario
  staging + directive replay that produces `tests/golden/engine/m2-frames.txt`
  (792 lines). Record the input tape format and the current scripted directives.

- [ ] **Step 2: Design the extension.** Specify the exact directives/frames to play
  **half-time** and a **tied regulation end → extra-time entry** (a 0-0 reg end is a
  natural tie — no score to script). Give the expected new length range and whether
  the 792-line v20 prefix can stay byte-identical (additive) or a shorter staged
  period shifts early frames.

- [ ] **Step 3: Specify the re-pin.** The written-reason wording, the frame-diff
  methodology (`cmp` before/after, first differing line, hash-only vs `state=`),
  and the mutation-RED rider classes the T6 acceptance must add.

- [ ] **Step 4: Hand off.** This slice is the direct input to the Stage-2 producer plan's T6.

---

## Stage 2 (post-recon, separate plan)

After all 8 drafts are frozen and promoted, the controller writes
`docs/superpowers/plans/2026-10-13-fifa96-m2-phase21-live-period-end.md` off the
frozen `FU223…FU230`, carrying spec §5's T1–T6 task structure with exact code,
per-task RED/GREEN tests, mutation-RED discriminators, the T1 determinism gate and
the T6 tape re-pin.

## Self-Review

- **Spec coverage:** the spec §4's 8 slices map 1:1 to Tasks 1–8 (FU223→Task 1,
  FU224→Task 2, FU225→Task 3, FU226→Task 4, FU227→Task 5, FU228→Task 6,
  FU229→Task 7, FU230→Task 8). The spec §5 producer tasks are delegated to the
  Stage-2 plan (this plan's Stage 2 section) — recorded, not dropped.
- **Placeholders:** none — each task names its objective, exact targets, evidence
  floor, deliverable and handoff. No code steps exist (recon produces docs, per the
  repo precedent).
- **Consistency:** slice numbers (W21-1…W21-8 ↔ FU223…FU230), draft paths and the
  promotion path are consistent across tasks; the ownership mapping to FU215-L2/L3/L5,
  FU207-L2, FU213-L3, FU208-L8/FU218-L3, FU221-L1 matches the spec §1/§4.

## Handoff

Hand this plan to `superpowers:subagent-driven-development`: dispatch a fresh
read-only subagent per task (all 8 in parallel), then the controller freeze-reviews
and promotes each draft to a canonical `docs/ghidra/FU2xx_*.md` in a serialized
commit per slice.
