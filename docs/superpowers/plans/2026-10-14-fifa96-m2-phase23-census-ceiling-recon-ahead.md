# FIFA 96 M2 phase-23 recon-ahead — the census ceiling (raise the honest action-dispatch frontier)

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` to execute this plan. The 8 recon slices are **read-only** and run **in parallel**. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Freeze the eight read-only evidence slices (`FU240…FU247`) that establish, first-hand, which of the 24 NULL action/phase rows can be honestly wired (a live installer+dispatch) and which are permanent ceiling legs — plus the two A-movers (phase `0x13`/`0x14`, phase `0x1E`) — so the phase-23 producer plan can be written without guessing.

**Architecture:** This is the phase-23 **Stage 1** (recon-ahead), exactly the phase-21/phase-22 shape. The approved design (`docs/superpowers/specs/2026-10-14-fifa96-m2-phase23-census-ceiling-design.md` §4) sequences the recon before the implementation plan; the producer tasks are authored in a **Stage 2** plan off these frozen slices. Each of the 8 agents investigates one frontier cluster, writes only its draft, and hands off; the controller freeze-reviews and promotes to a canonical `docs/ghidra/FU2xx_*.md` in a serialized commit.

**Tech Stack:** Ghidra MCP (read-only), C11/CMake/CTest, ASan/UBSan, SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase23-census-ceiling-design.md` (approved). Predecessor plan: `docs/superpowers/plans/2026-10-13-fifa96-m2-phase22-end-spine.md`. Recon-ahead precedent: `docs/superpowers/plans/2026-10-13-fifa96-m2-phase22-recon-ahead.md`.

## Global Constraints

- **Ghidra stays read-only.** Program argument exactly `/FIFA96.EXE` (case-sensitive). **No** rename / comment / label / function-create / script / project save. Standard MCP tools only.
- **Baseline (HEAD `7982f44`, the spec commit):** `make check` **110/110**; wired **55/80 = 68%** (`33 action + 22 phase`); dispatch `24 UNSUP / 55 OK / 1 NOTF`. M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (**691 lines, immutable**); M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (**v22, 6522 lines**).
- **Evidence floor (AGENTS.md):** every address/byte claim cites a fresh disassembly output or reconciled raw-byte read. Anything not statically provable becomes a **numbered open leg** with its native owner. LE-image globals at `+0x100000`.
- **Alias traps:** `dword[addr] >> 16` = the word at `addr+2`, `>> 24` = the byte at `addr+3`; verify CALL bytes (rel32); watch sign/width, inverted gates, gate-index misreads. Any "only/sole/exactly" or caller-count claim needs a **fresh** `get_xrefs_to`.
- **No code, no test, no shared-doc edits** by the recon agents. Each writes **only** its own draft file (+ a workspace report). The controller promotes drafts to canonical `docs/ghidra/FU2xx_*.md` and commits.
- **Census rule:** a row counts only when a **ported path installs AND dispatches** — a `NULL → fn` flip with no live producer is NOT counted (the rows-`27`/`29`/`2C` precedent). Every slice's verdict is **wire-vs-ceiling**; no faked producer.
- **Scope guard:** the census frontier only. No front-end/competition, no audio, no new subsystem. Double-wire exclusions (phase `01`/`11`/`12`) stand.

## File Structure

| Path | Responsibility |
|---|---|
| `.superpowers/sdd/2026-10-14-fifa96-m2-phase23-census-ceiling-recon-ahead/progress.md` | The SDD ledger (recovery map) |
| `.superpowers/sdd/2026-10-14-fifa96-m2-phase23-census-ceiling-recon-ahead/w23-<n>-*.md` | The 8 recon drafts (git-ignored scratch) |
| `docs/ghidra/FU240_*.md … FU247_*.md` | The promoted canonical slices (controller, serialized commits) |
| `docs/ghidra/FU222_census_audit_phase20.md`, `FU-137`, `FU-142f`, `FU192`, `FU212`, `FU177`, `FU178`, `FU186`, `FU201`, `FU195`, `FU193`, `FU189`, `FU185`, `FU146` | Prior slices — appended errata only, never rewritten |

---

### Task 1: W23-1 / FU240 — the phase-`0x13`/`0x14` scatter producer (A-mover, `+2`)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase23-census-ceiling-recon-ahead/w23-1-phase-1314-producer.md`
**Promotes to:** `docs/ghidra/FU240_phase_1314_producer.md`

**Objective:** Derive the unported `FUN_0008D098` case-`0x13/0x14` PRNG-scatter producer and the tape-inertness ruling for the phase-`0x13`/`0x14` rows (FU192-L2).

- [ ] **Step 1: The phase body.** `0x6E244` (the shared phase-`0x13`/`0x14` camera-scatter body, FU192 §2.10): the per-record walk, the two `0x92AC8` PRNG draws, the `[EAX+0x826]` side compare, the `[0x157AAC]>>24` read, and the `[ECX]`/`[ECX+8]` stores. Fresh `disassemble_bytes 0x6E244`.
- [ ] **Step 2: The producer.** `FUN_0008D098`'s case-`0x13`/`0x14` block and the `0x10F364`/`0x10F368` cells (producer chain + polarity); fresh `get_xrefs_to` on both cells. Is the producer reachable from a live engine source?
- [ ] **Step 3: The writer path.** Which native site writes phase `0x13`/`0x14` into `[0x157A4A]` (fresh `FUN_000740A0` callers), and whether the engine's `match_run_write_phase` path can reach it.
- [ ] **Step 4: The tape ruling + verdict.** Phase `0x14` is on-tape 40 frames; decide tape-inertness and state the **wire-vs-ceiling** ruling (+2 port shape) + numbered legs. Hand off.

---

### Task 2: W23-2 / FU241 — the phase-`0x1E` restart sweep de-compression (A-mover, `+1`)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase23-census-ceiling-recon-ahead/w23-2-phase-1e-decompression.md`
**Promotes to:** `docs/ghidra/FU241_phase_1e_decompression.md`

**Objective:** Re-derive, first-hand, the render-invisibility + dormancy of the faithful act-8 restart sweep so the deliberate compressed-model pin (FU212-L1 / FU194-L2) can be honestly lifted.

- [ ] **Step 1: The native sweep.** `0x8A798` (the act-8 restart sweep, FU-83): the 22-record walk, the selected-pointer store `[0x1587F0]` and the `word[0x157A68]` clear (FU-212 §6). Fresh `disassemble_bytes`.
- [ ] **Step 2: The compressed model.** The engine's in-lined stored-situation re-dispatch (`fifa96_match_run_set_piece` BX!=0) and the pin `tests/test_engine_match_frame.c::test_phase_1E_compressed_model_pin`.
- [ ] **Step 3: The dormancy.** Prove (or refute) that the golden never enters the compressed bx!=0 path and the sweep rewrites no record code; the exact pin-lift terms + which tests/pins must flip.
- [ ] **Step 4: Verdict + legs.** State the **wire-vs-ceiling** ruling (+1 honest lift) + numbered legs. Hand off.

---

### Task 3: W23-3 / FU242 — the installer mechanism + the exhaustive per-row census (anchor)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase23-census-ceiling-recon-ahead/w23-3-installer-census.md`
**Promotes to:** `docs/ghidra/FU242_installer_census.md`

**Objective:** Independently re-derive the installer mechanism (`FUN_0007D9A4` + the `[rec+0x1C]` indirect dispatch + the `FUN_000888FC` act invoker) and produce a fresh per-row installer verdict for **all 24 NULL rows** — the frontier's anchor. This slice must NOT trust FU-142f; it re-runs the census.

- [ ] **Step 1: The tables.** Fresh `read_memory` of `0x1106E0` (45 dwords), `0x110794` (35 dwords), `0x1107EC` (act table); confirm the 55/80 domain slots byte-for-byte.
- [ ] **Step 2: The installer.** `FUN_0007D9A4` (fresh `get_xrefs_to`; the code argument forms — immediate `MOV EDX/DX,imm` in all encodings vs register-derived), the `[rec+0x1C]` indirect dispatch (`fifa96_match_dispatch_phase_record`), and `FUN_000888FC` (the act invoker).
- [ ] **Step 3: The per-row verdict.** For each of the 24 NULL rows, record the exact installer chain and a fresh reachability verdict (live / unported / gated / tape-visible / absent) with its xref counts.
- [ ] **Step 4: Verdict + legs.** Publish the per-row frontier map and the ceiling model. Hand off.

---

### Task 4: W23-4 / FU243 — the no-installer / dead cluster

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase23-census-ceiling-recon-ahead/w23-4-no-installer-cluster.md`
**Promotes to:** `docs/ghidra/FU243_no_installer_cluster.md`

**Objective:** Independently re-derive the "no invocation / dead entry / xref-less stub" verdicts for action `27`, `29`, `2C`, `2B`, `25`, `0A`, `15` — confirm or refute FU-142f/OL-48, OL-15, OL-14, FU189-L2.

- [ ] **Step 1: The 77-site census.** Re-enumerate every `FUN_0007D9A4` call site (fresh `get_xrefs_to`) and the code argument at each; does any pass `0x27`/`0x29`/`0x2C` (immediate or register-derived)?
- [ ] **Step 2: The dead entry.** `0x87738` (the shared row-29 epilogue RET) vs a standalone `0x2B` stub — fresh bytes.
- [ ] **Step 3: The stubs + the exhaustive scan.** `0x7CDD8` (row `0A` installer, 0 xrefs) and the row-`15` stub bucket; the bounded `MOV EDX/DX,0x25` scan (row `25`).
- [ ] **Step 4: Verdict + legs.** Confirm/refute each; freeze the permanent legs. Hand off.

---

### Task 5: W23-5 / FU244 — action `1A`'s installer + the phase-`0B` writer (the `+2` cluster)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase23-census-ceiling-recon-ahead/w23-5-row-1a-ladder.md`
**Promotes to:** `docs/ghidra/FU244_row_1a_ladder.md`

**Objective:** Derive the unported `FUN_00077EAC` CPU ladder (action `1A`'s only installer) and the phase-`0B` writer whose owning row `0x1A` this slice disambiguates — the phase's `+2` cluster candidate.

- [ ] **Step 1: The ladder.** `FUN_00077EAC` `0x78227`/`0x7824F`/`0x78277` (the action-`1A` install arms) and the `FUN_0008D8EC → FUN_000782D0` pre-pass; fresh `disassemble_bytes` + `get_xrefs_to`.
- [ ] **Step 2: The action-`1A` body + engine posture.** The ported `fifa96_match_action_1A` stage-0 slice and its unported stages 1-4 (`FU186-L1`); what the ladder's install needs to fire live.
- [ ] **Step 3: The phase-`0B` writer.** Locate the sole phase-`0B` writer (the "unwired row `0x1A` body", FU-206 §4 #12) and disambiguate action-row vs phase-row `0x1A`; fresh bytes.
- [ ] **Step 4: Verdict + legs.** State whether action `1A` (+ phase `0B`) is a **wire** or a **ceiling leg**; the port shape. Hand off.

---

### Task 6: W23-6 / FU245 — action `0D` + row-09 arm 1 + the phase-`0D`/`0E` writers

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase23-census-ceiling-recon-ahead/w23-6-row-09-arm1.md`
**Promotes to:** `docs/ghidra/FU245_row_09_arm1.md`

**Objective:** Derive the "installer inside an unported body" cluster: action `0D`'s installer (row-09 `0x80F61` inside arm 1) and the phase-`0D`/`0E` writer chains.

- [ ] **Step 1: The installer site.** `0x80F61` (`MOV EDX,0xD; CALL 0x7D9A4`) inside row-09 arm 1; fresh bytes.
- [ ] **Step 2: The arm-1 reachability.** Row-09 arm 1 (stages 1-3, FU178-L1): the gating producers (`[0x15877C]` FU178-L6, `[rec+0x3D]`, `[rec+0x24]`, `[[rec+4]+0x13]`), the helper/sink set, and whether the arm is reachable.
- [ ] **Step 3: The phase writers.** Phase `0D`'s sole argument situation 7 (`fifa96_action_handlers.c:1141`) and whether any live caller dispatches situation 7 (FU-146); phase `0E`'s writer (FU-191 §10).
- [ ] **Step 4: Verdict + legs.** Wire-vs-ceiling for action `0D` + phase `0D`/`0E`. Hand off.

---

### Task 7: W23-7 / FU246 — action `03` + action `17` + the phase act-4/5/6 producers

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase23-census-ceiling-recon-ahead/w23-7-blocked-installers.md`
**Promotes to:** `docs/ghidra/FU246_blocked_installers.md`

**Objective:** Derive the blocked-installer + no-producer cluster: action `03` (tape-visible), action `17` (permutation residual), and phase `1A`/`1B`/`1C` (act-4/5/6 producers).

- [ ] **Step 1: Action `03`.** The live phase-2 hot path's `[rec+0x1C] = 0x6DCC8` target-rewrite chain; identify the exact M2 frame-29 divergence (controller R7, FU177-L1). Can it be wired without moving the tape?
- [ ] **Step 2: Action `17`.** The act-4 stage-0 installer `0x89740` behind the sit-2..4 formation-order arm `0x8AB08..0x8AB62`; the permutation residual (`FUN_0008C974` sorts current onto desired; the reset `FUN_00073EE0` copies desired→current) → residual 0 (FU201-L1).
- [ ] **Step 3: The act-4/5/6 producers.** Phase `1A` (act-4, no producer), phase `1B` (act-5, no `EAX=5` invoker — fresh `FUN_000888FC` census), phase `1C` (act-6, offside double-run).
- [ ] **Step 4: Verdict + legs.** Wire-vs-ceiling for each; the tape-visibility ruling for action `03`. Hand off.

---

### Task 8: W23-8 / FU247 — action `22`/OL-65 + the double-wire trio + the tape terms

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase23-census-ceiling-recon-ahead/w23-8-exclusions-and-tape.md`
**Promotes to:** `docs/ghidra/FU247_exclusions_and_tape.md`

**Objective:** Derive the structurally-ungated row (action `22`/OL-65), the permanent double-wire register (phase `01`/`11`/`12`), and the tape/re-pin term inventory for the Stage-2 producer plan.

- [ ] **Step 1: Action `22` / OL-65.** The row-07 `0x816DF` invoke-now gate; the unmodeled `word[+0x6F]`/`word[+0x7D]` angle inputs and why `fifa96_entity_angle(lane_z, 0)` can never yield the `< 0x55` diff (FU185-L6).
- [ ] **Step 2: The double-wire trio.** Phase `01`/`11`/`12` (`0x6E1D0`/`0x6E1C8`): confirm the seeding-path realization and the R10 exclusion (FU192-L6/L7).
- [ ] **Step 3: The tape terms.** Inventory which NULL rows are on-tape (`03`, phase `0x14`'s 40 frames), which are tape-dormant, and pre-specify the re-pin policy (additive / written reason + frame diff) + the mutation-RED rider classes a Stage-2 acceptance must add.
- [ ] **Step 4: Hand off.** Direct input to the Stage-2 producer plan's tape/census close-out task.

---

## Stage 2 (post-recon, separate plan)

After all 8 drafts are frozen and promoted, the controller writes
`docs/superpowers/plans/2026-10-14-fifa96-m2-phase23-census-ceiling.md` off the
frozen `FU240…FU247`, carrying the spec §5 T-structure: T1 the determinism +
feasibility gate (census recompute + ≥3-replay byte-stability, no re-pin); T2..Tk
one producer/registration per proven-attainable row (with per-row RED/GREEN
dispatch tests + mutation-RED riders); the tape-touch task; the census close-out
(README/ENGINE/PROGRESS/AGENTS + the `test_engine_m2.c:3666`/`:4041` pins).

## Self-Review

- **Spec coverage:** spec §4's 8 slices map 1:1 to Tasks 1–8 (FU240→Task 1 … FU247→Task 8). The spec §2 A-movers (phase `0x13`/`0x14`, phase `0x1E`) are Tasks 1–2; the B-front (24 NULL rows) is Tasks 3–8; the double-wire trio is Task 8.
- **Placeholders:** none — each task names its objective, exact targets, evidence floor, deliverable and handoff. The Stage-2 T-list is deliberately parametrized (it derives from the frozen slices), matching the phase-21/22 precedent.
- **Consistency:** slice numbers (W23-1…W23-8 ↔ FU240…FU247), draft paths and the promotion path are consistent; the ownership mapping matches spec §1/§4.
- **Deferred:** the Stage-2 producer plan is the next artifact; the ceiling is published from what the wave proves (spec §6 caveat).

## Handoff

Hand this plan to `superpowers:subagent-driven-development`: dispatch a fresh
read-only subagent per task (all 8 in parallel), then the controller freeze-reviews
and promotes each draft to a canonical `docs/ghidra/FU2xx_*.md` in a serialized
commit per slice.
