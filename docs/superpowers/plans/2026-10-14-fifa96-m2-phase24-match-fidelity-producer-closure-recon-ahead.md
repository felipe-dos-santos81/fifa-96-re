# FIFA 96 M2 phase-24 recon-ahead — the match-fidelity producer closure

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` to execute this plan. The 8 recon slices are **read-only** and run **in parallel**. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Freeze the eight read-only evidence slices (`FU248…FU255`) that establish, first-hand, each unported native producer behind the wired match rows — its exact body, its consumers, and whether porting it yields **fidelity** or also un-dormants a still-NULL row (**census**) — so the phase-24 producer plan can be written without guessing.

**Architecture:** This is the phase-24 **Stage 1** (recon-ahead), exactly the phase-21/22/23 shape. The approved design (`docs/superpowers/specs/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-design.md` §4) sequences the recon before the implementation plan; the Stage-2 producer tasks are authored off these frozen slices. Each of the 8 agents investigates one producer cluster, writes only its draft, and hands off; the controller freeze-reviews and promotes to a canonical `docs/ghidra/FU2xx_*.md` in a serialized commit.

**Tech Stack:** Ghidra MCP (read-only), C11/CMake/CTest, ASan/UBSan, SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-design.md` (approved). Predecessor plan: `docs/superpowers/plans/2026-10-14-fifa96-m2-phase23-census-ceiling.md`. Recon-ahead precedent: `docs/superpowers/plans/2026-10-14-fifa96-m2-phase23-census-ceiling-recon-ahead.md`.

## Global Constraints

- **Ghidra stays read-only.** Program argument exactly `/FIFA96.EXE` (case-sensitive). **No** rename / comment / label / function-create / script / project save. Standard MCP tools only.
- **Baseline (HEAD `c83883f`, the spec commit):** `make check` **110/110**; wired **58/80 = 72%** (`35 action + 23 phase`); dispatch `21 UNSUP / 58 OK / 1 NOTF`. M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (**691 lines, immutable**); M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (**v22, 6522 lines**).
- **Evidence floor (AGENTS.md):** every address/byte claim cites a fresh disassembly output or reconciled raw-byte read. Anything not statically provable becomes a **numbered open leg** with its native owner. LE-image globals at `+0x100000`.
- **Alias traps:** `dword[addr] >> 16` = the word at `addr+2`, `>> 24` = the byte at `addr+3`; verify CALL bytes (rel32); watch sign/width, inverted gates, gate-index misreads. Any "only/sole/exactly" or caller-count claim needs a **fresh** `get_xrefs_to`.
- **No code, no test, no shared-doc edits** by the recon agents. Each writes **only** its own draft file (+ a workspace report). The controller promotes drafts to canonical `docs/ghidra/FU2xx_*.md` and commits.
- **No fakes.** A producer that would compute a residual 0 / copy desired→current is a no-op or a fake (R45/R49/R15); it must be ruled a leg, never invented as a wire.
- **Scope guard:** the unported native **producers** of the match rows only. No front-end *visual* work, no audio, no new subsystem.

## File Structure

| Path | Responsibility |
|---|---|
| `.superpowers/sdd/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-recon-ahead/progress.md` | The SDD ledger (recovery map) |
| `.superpowers/sdd/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-recon-ahead/w24-<n>-*.md` | The 8 recon drafts (git-ignored scratch) |
| `docs/ghidra/FU248_*.md … FU255_*.md` | The promoted canonical slices (controller, serialized commits) |
| Prior slices `FU137`, `FU142`, `FU175`, `FU185`, `FU192`, `FU197`, `FU201`, `FU241`, `FU243`, `FU247`, `ENGINE.md` leg register | Prior evidence — appended errata only, never rewritten |

---

### Task 1: W24-1 / FU248 — the desired-order matrices / line-up setup (the headline)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-recon-ahead/w24-1-formation-setup.md`
**Promotes to:** `docs/ghidra/FU248_formation_setup.md`

**Objective:** Derive the unported line-up setup `FUN_00011620`/`FUN_00012010` (the `0x14C360` desired-order producer; **5 front-end callers**) and settle the **runtime non-permutation** question (FU201-L1) that decides whether the act-4 arm can ever go live.

- [ ] **Step 1: The producer.** `FUN_00011620`/`FUN_00012010` (fresh `disassemble_bytes` + `get_xrefs_to`): the `0x14C360` desired-order write, the harness/inputs, the 5 front-end caller sites.
- [ ] **Step 2: The consumer.** The act-4 sit-2..4 arm `0x8AB08..0x8AB62` (the `FUN_0008C974` sorts, the `[0x157B8E]`/`[0x157B8F]` residual gate) and the match-init reset `FUN_00073EE0` (desired→current) — fresh bytes.
- [ ] **Step 3: The non-permutation question (FU201-L1).** Is there ANY native writer that diverges current from desired at runtime (a substitution/line-up change)? A fresh `get_xrefs_to` on the current/desired matrices + the `[0x157B8E]`/`[0x157B8F]` cells.
- [ ] **Step 4: Verdict + legs.** State the producer's port shape and the explicit **census ruling** (does action `17` + phase `1A`/`0B` go live, or stay gated?) with the payoff (census vs fidelity only). Number the legs. Hand off.

---

### Task 2: W24-2 / FU249 — OL-13 / FU192-L1: the per-record metadata + the `[rec+0x1C]` seam

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-recon-ahead/w24-2-ol13-metadata.md`
**Promotes to:** `docs/ghidra/FU249_ol13_metadata.md`

**Objective:** Derive the `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]` metadata source (FU192-L1) and the `[rec+0x1C]` seam, which gate the phase-`0x02`/`0x15` bodies and the row-03 rewrite.

- [ ] **Step 1: The metadata source.** The `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]` producers (FU192-L1; the loader-content bind is partially landed, FU-199 §7.2) — fresh bytes.
- [ ] **Step 2: The `[rec+0x1C]` seam.** The per-record dispatch seam (FU175-L1) and its live writers.
- [ ] **Step 3: The consumers.** The phase-`0x02`/`0x15` gate and the row-03 `[rec+0x1C] = 0x6DCC8` rewrite — what a ported source un-dormants.
- [ ] **Step 4: Verdict + legs.** Port shape + the **census ruling** for the phase-`0x02`/`0x15` gate and row `03`. Hand off.

---

### Task 3: W24-3 / FU250 — OL-65: the row-07/`22` angle inputs

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-recon-ahead/w24-3-ol65-angle-inputs.md`
**Promotes to:** `docs/ghidra/FU250_ol65_angle_inputs.md`

**Objective:** Derive the `word[+0x6B]`/`word[+0x6D]`/`word[+0x7D]` angle inputs whose absence keeps the row-07 `0x816DF` `opponent_invoke` dead (FU185-L6 / FU247-L1) — the gate for action `22`.

- [ ] **Step 1: The inputs.** The three angle words: their producers + whether the engine's pool stages them (FU247-L1: all pool-modelled, but staged 0).
- [ ] **Step 2: The gate.** The row-07 `0x816DF` invoke-now condition (`fifa96_entity_angle` fold `< 0x55`) — fresh bytes; confirm the brute-force claim.
- [ ] **Step 3: The row-22 body.** `0x8539C..0x85481` (ported) and what it needs from the inputs.
- [ ] **Step 4: Verdict + legs.** Port shape + the **census ruling** for action `22`. Hand off.

---

### Task 4: W24-4 / FU251 — OL-63: the row-05 stage-0 tail

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-recon-ahead/w24-4-ol63-row05-tail.md`
**Promotes to:** `docs/ghidra/FU251_ol63_row05_tail.md`

**Objective:** Derive the row-05 stage-0 target algebra `0x7F3A1..0x7F57B` + the `FUN_0007F7E0` fallback (the OL-63 residual).

- [ ] **Step 1: The tail.** `0x7F3A1..0x7F57B` (fresh disassembly): the stage-0 target algebra over the pool.
- [ ] **Step 2: The fallback.** `FUN_0007F7E0` and its conditions.
- [ ] **Step 3: The engine posture.** The ported `fifa96_match_action_05` stage-0 gate and what the tail would complete.
- [ ] **Step 4: Verdict + legs.** Port shape (fidelity; census unlikely). Hand off.

---

### Task 5: W24-5 / FU252 — OL-67/OL-72 (row `04`) + OL-68/OL-69 (rows `18`/`23`)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-recon-ahead/w24-5-row04-18-23.md`
**Promotes to:** `docs/ghidra/FU252_row04_18_23.md`

**Objective:** Derive the unmodeled record/descriptor bytes + track words for the outfield rows `04`/`18`/`23`.

- [ ] **Step 1: Row 04.** `0x7E7C8..0x7F141` — the unmodeled record/team bytes, track words, sinks (OL-67/OL-72).
- [ ] **Step 2: Rows 18/23.** `0x849B0..0x84AE1` / `0x82F84..0x83163` — the anim row byte + bind globals (OL-68), the `0x1577xx` camera/track inputs (OL-69, staged zero).
- [ ] **Step 3: The shared fields.** Which record bytes are shared across the rows (OL-83 territory) and their producers.
- [ ] **Step 4: Verdict + legs.** Port shapes (fidelity). Hand off.

---

### Task 6: W24-6 / FU253 — OL-82/OL-83 (row `08` + the field wrinkle) + OL-19/OL-22/OL-34

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-recon-ahead/w24-6-row08-fields.md`
**Promotes to:** `docs/ghidra/FU253_row08_fields.md`

**Objective:** Derive the row-08 `0x81068..0x814AF` unmodeled inputs/sinks (OL-82), the `+0x8E` `act_type`/`actor_type` field-sharing wrinkle (OL-83), and the OL-19/OL-22/OL-34 metadata legs.

- [ ] **Step 1: Row 08.** The `+0x3D`-gated projection scan, the `0x7A490` staging, the event/sound sinks (OL-82).
- [ ] **Step 2: The field wrinkle.** The `+0x8E` `type`/`actor_type` shared byte (OL-83): which rows write vs read it, and the reconciliation.
- [ ] **Step 3: OL-19/OL-22/OL-34.** The remaining metadata legs (fresh definitions).
- [ ] **Step 4: Verdict + legs.** Port shapes (fidelity). Hand off.

---

### Task 7: W24-7 / FU254 — the anim/sink producers: OL-27/OL-52 + OL-42

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-recon-ahead/w24-7-anim-sink-producers.md`
**Promotes to:** `docs/ghidra/FU254_anim_sink_producers.md`

**Objective:** Derive the `0x92820`/`0x6E598` sinks (OL-27/OL-52) and the installer anim arm (OL-42) that produce non-zero `anim_id`s outside the wired bodies.

- [ ] **Step 1: The sinks.** `0x92820` and `0x6E598` (fresh bytes): what they consume and produce.
- [ ] **Step 2: The installer anim arm.** OL-42: the `FUN_00036C70` staging and the native arm that writes non-zero `anim_id`s outside the wired bodies.
- [ ] **Step 3: The FU-84 coupling.** The anim bank/frame tables (FU-84) and their staging.
- [ ] **Step 4: Verdict + legs.** Port shapes (fidelity). Hand off.

---

### Task 8: W24-8 / FU255 — the phase-23 residuals: FU241-L1, FU243-L4, FU241-L2

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-recon-ahead/w24-8-phase23-residuals.md`
**Promotes to:** `docs/ghidra/FU255_phase23_residuals.md`

**Objective:** Derive the residuals left by phase-23: the non-tape compression equivalence (FU241-L1), the row-25/15 scatter/id tables (FU243-L4), and FU241-L2.

- [ ] **Step 1: FU241-L1.** The retired-compression equivalence on non-tape `bx==1` paths (the T4 re-route): is the 22-frame deferral exactly equivalent, and on which callers?
- [ ] **Step 2: FU243-L4.** The row-25/15 active head block + the radius-`0xC0` `[rec+0x8A]` fold + the `[0x1587B8]`/`[0x1587BB]` id tables + the `0x741B4`/`0x651F0` sinks — port shape.
- [ ] **Step 3: FU241-L2.** `word[0x157A68]` (FU194-L4) — the clear/consumer.
- [ ] **Step 4: Verdict + legs.** Port/proof shapes. Hand off.

---

## Stage 2 (post-recon, separate plan)

After all 8 drafts are frozen and promoted, the controller writes
`docs/superpowers/plans/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure.md`
off the frozen `FU248…FU255`, carrying spec §5's T-structure: T1 the determinism +
feasibility gate (census recompute + ≥3-replay byte-stability, no re-pin); T2..Tk
one producer port per proven-tractable cluster (per-port RED/GREEN fixture tests +
mutation-RED riders); the census close-out (only if a producer wires a row); the
leg-register close-out.

## Self-Review

- **Spec coverage:** spec §4's 8 slices map 1:1 to Tasks 1–8 (FU248→Task 1 … FU255→Task 8). The headline (the formation setup + the FU201-L1 verdict) is Task 1; the phase-23 residuals are Task 8.
- **Placeholders:** none — each task names its objective, exact targets, evidence floor, deliverable and handoff. The Stage-2 T-list is deliberately parametrized (derives from the frozen slices).
- **Consistency:** slice numbers (W24-1…W24-8 ↔ FU248…FU255), draft paths and the promotion path are consistent; the frontier table matches spec §1.
- **Deferred:** the Stage-2 producer plan is the next artifact; the census is published from what the wave proves (spec §6 caveat).

## Handoff

Hand this plan to `superpowers:subagent-driven-development`: dispatch a fresh
read-only subagent per task (all 8 in parallel), then the controller freeze-reviews
and promotes each draft to a canonical `docs/ghidra/FU2xx_*.md` in a serialized
commit per slice.
