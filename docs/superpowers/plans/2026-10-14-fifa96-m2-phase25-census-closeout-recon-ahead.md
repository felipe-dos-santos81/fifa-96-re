# FIFA 96 M2 phase-25 recon-ahead — the match-action census close-out

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` to execute this plan. The 8 recon slices are **read-only** and run **in parallel**. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Freeze the eight read-only evidence slices (`FU256…FU263`) that establish, first-hand, the native **gating producer** of each of the 22 unwired match-action rows — its exact body, its installer, its reachability, and whether porting it lets the row **install AND dispatch** (a census rise) while keeping M2 byte-identical — so the phase-25 producer plan can be written without guessing.

**Architecture:** This is the phase-25 **Stage 1** (recon-ahead), exactly the phase-21/22/23/24 shape. The approved design (`docs/superpowers/specs/2026-10-14-fifa96-m2-phase25-census-closeout-design.md` §4) sequences the recon before the implementation plan; the Stage-2 producer tasks are authored off these frozen slices. Each of the 8 agents investigates one producer cluster, writes only its draft, and hands off; the controller freeze-reviews and promotes to a canonical `docs/ghidra/FU2xx_*.md` in a serialized commit.

**Tech Stack:** Ghidra MCP (read-only), REA MCP (second read), C11/CMake/CTest, ASan/UBSan, SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase25-census-closeout-design.md` (approved). Predecessor plan: `docs/superpowers/plans/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-recon-ahead.md`. Recon-ahead precedent: `docs/superpowers/plans/2026-10-14-fifa96-m2-phase23-census-ceiling-recon-ahead.md`.

## Global Constraints

- **Ghidra stays read-only.** Program argument exactly `/FIFA96.EXE` (case-sensitive). **No** rename / comment / label / function-create / script / project save. Standard MCP tools only. (context7 is **not applicable** — this is RE, not library docs.)
- **Baseline (HEAD `88a894b`, the spec commit):** `make check` **110/110**; wired **58/80 = 72%** (`35 action + 23 phase`); dispatch `21 UNSUP / 58 OK / 1 NOTF`. M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (**691 lines, immutable**); M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (**v22, 6522 lines**).
- **Evidence floor (AGENTS.md):** every address/byte claim cites a fresh disassembly output or reconciled raw-byte read. Anything not statically provable becomes a **numbered open leg** with its native owner. LE-image globals at `+0x100000`.
- **Alias traps:** `dword[addr] >> 16` = the word at `addr+2`, `>> 24` = the byte at `addr+3`; verify CALL bytes (rel32); watch sign/width, inverted gates, gate-index misreads. Any "only/sole/exactly" or caller-count claim needs a **fresh** `get_xrefs_to`. (The phase-24 wave caught two alias-trap errors; re-verify before trusting an inherited verdict.)
- **The census rule:** a row counts only when a ported path **installs AND dispatches** it (the rows-27/29/2C precedent). A NULL→fn flip with no live producer is **not** counted. Tape-dormant rows count only when a fixture proves the install+dispatch.
- **No code, no test, no shared-doc edits** by the recon agents. Each writes **only** its own draft file (+ a workspace report). The controller promotes drafts to canonical `docs/ghidra/FU2xx_*.md` and commits.
- **No fakes.** A producer that would compute a residual 0 / copy desired→current is a no-op or a fake (R15/R45/R49); it must be ruled a leg, never invented as a wire.
- **M2 preservation:** the approved stance is **M2 byte-identical**. A producer whose row is **tape-visible** keeps that row leg'd (do not move the golden). The known tape-visible risk is the `[rec+0x1C]=0x6DCC8` chain (transcribed frame 29).
- **Scope guard:** the native **gating producers** of the 22 unwired match-action rows only. No front-end *visual* work, no audio, no new subsystem.

## File Structure

| Path | Responsibility |
|---|---|
| `.superpowers/sdd/2026-10-14-fifa96-m2-phase25-census-closeout-recon-ahead/progress.md` | The SDD ledger (recovery map) |
| `.superpowers/sdd/2026-10-14-fifa96-m2-phase25-census-closeout-recon-ahead/w25-<n>-*.md` | The 8 recon drafts (git-ignored scratch) |
| `docs/ghidra/FU256_*.md … FU263_*.md` | The promoted canonical slices (controller, serialized commits) |
| Prior slices `FU137`, `FU142`, `FU175`, `FU177`, `FU178`, `FU180`, `FU185`, `FU186`, `FU191`, `FU192`, `FU201`, `FU239`, `FU243`, `FU247`, `FU248`, `FU250`, `ENGINE.md` leg register | Prior evidence — appended errata only, never rewritten |

**Ordering (dependency):** Tasks 1–8 are independent read-only investigations and run in parallel. The controller reconciles at promotion time only if two slices reach the same native producer (e.g. Tasks 3 and 254/FU249 both touch the OL-13 `[rec+0xC]` source; Tasks 1 and 260 both walk the installer domain).

---

### Task 1: W25-1 / FU256 — the row-09 arm-1 install chain (→ action `0D`)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase25-census-closeout-recon-ahead/w25-1-row09-arm1-chain.md`
**Promotes to:** `docs/ghidra/FU256_row09_arm1_chain.md`

**Objective:** Derive the row-09 arm-1 install path (the only installer of action `0D`) and decide whether it is honestly port-able so row `0D` installs AND dispatches (census) — with M2 preserved.

- [ ] **Step 1: The install path.** Fresh `disassemble_bytes 0x80BFB..0x80FCB`: the arm-1 `0x80F61 = ba0d000000 MOV EDX,0xD; 0x80F6A = e835caffff CALL 0x7D9A4` install; confirm the target (`fifa96_match_action_0D` = `0x8251C..0x826FE`, the 4-arm table `0x8250C`).
- [ ] **Step 2: The gates.** The arm-1 entry gates `[0x15877C]` (FU178-L6), `[rec+0x3D]` (the FU-84 frame), `[rec+0x24]` (the owner), `[[rec+4]+0x13]` — fresh bytes + producers; are they live in-engine?
- [ ] **Step 3: The helpers/sinks.** The arm-1 hitched helpers/sinks `0x8f188`/`0x8ed40`/`0x92820`/`0x7a490`/`0x8a3fc` (FU180-L8) — fresh bytes; port shapes.
- [ ] **Step 4: The reaches.** A fresh `get_xrefs_to` on the arm entry — who calls row-09 arm 1 (the row-09 stage machine)? Is the arm livable from the wired row-09 stage-0, or is it behind another unported producer?
- [ ] **Step 5: Verdict + legs.** The port shape; the explicit **census ruling** for row `0D` (installs AND dispatches? tape-dormant?); numbered legs with owners. Hand off.

---

### Task 2: W25-2 / FU257 — the keeper CPU ladder (→ action `1A`)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase25-census-closeout-recon-ahead/w25-2-keeper-cpu-ladder.md`
**Promotes to:** `docs/ghidra/FU257_keeper_cpu_ladder.md`

**Objective:** Derive the `FUN_0008D8EC` → `FUN_000782D0` pre-pass and the `FUN_00077EAC` CPU ladder that install row `1A`, and decide whether they are honestly port-able (census for action `1A`).

- [ ] **Step 1: The pre-pass.** `FUN_0008D8EC` and `FUN_000782D0` fresh bytes — the per-frame keeper pre-pass; its caller (the frame body?) and inputs.
- [ ] **Step 2: The ladder.** `FUN_00077EAC` (`0x78227`/`0x7824F`/`0x78277` install sites) — the decision ladder and its `1A` install; the `[0x157C2C]` direction machine.
- [ ] **Step 3: The gates.** What gates the ladder (team/record state, the keeper's `+0x89`/`+0x92` cells)? Fresh bytes + producers.
- [ ] **Step 4: The reaches.** A fresh `get_xrefs_to` on `FUN_0008D8EC`/`FUN_00077EAC` — is the ladder reached live from the ported keeper chain (row `0x1F`/`0x1C`), or behind an unported machine?
- [ ] **Step 5: Verdict + legs.** The port shape; the **census ruling** for row `1A`; numbered legs. Hand off.

---

### Task 3: W25-3 / FU258 — the `[rec+0x1C]=0x6DCC8` target-rewrite chain (→ action `03`)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase25-census-closeout-recon-ahead/w25-3-rec1c-dcc8-chain.md`
**Promotes to:** `docs/ghidra/FU258_rec1c_dcc8_chain.md`

**Objective:** Derive the `[rec+0x1C]=0x6DCC8` target-rewrite chain + the OL-13 `[rec+0xC]` bind that gate action `03`, and settle the **tape-visibility** question (frame 29) — since the approved stance preserves M2.

- [ ] **Step 1: The rewrite target.** The phase-2 hot-path `[rec+0x1C]` = `0x6DCC8` target rewrite and the phase-0x13/0x14 special arm (FU177-L1/L6) — fresh bytes.
- [ ] **Step 2: The OL-13 bind.** The `[rec+0xC]` metadata source (FU192-L1; FU249 derived `FUN_0006D920`) — confirm the live bind surface and whether the row-03 body's inputs are live.
- [ ] **Step 3: The tape-visibility analysis.** Why wiring row `03` moves M2 at transcribed frame 29 (the forced code-3 record runs its row-03 body in the phase-2 window); is the rewrite avoidable (a gated/special-arm form) or unavoidable?
- [ ] **Step 4: Verdict + legs.** The port shape; the explicit **ruling**: does row `03` wire with M2 preserved, or stay leg'd (M2 preserved) as spec §1 prefers? Numbered legs. Hand off.

---

### Task 4: W25-4 / FU259 — OL-65: the staging fix + the row-07 `0x816DF` invoke (→ action `22`)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase25-census-closeout-recon-ahead/w25-4-ol65-invoke.md`
**Promotes to:** `docs/ghidra/FU259_ol65_invoke.md`

**Objective:** Turn FU250's finding into a port plan — the 3 angle words are live pool fields (`word[+0x6D]`/`word[+0x6F]`/`word[+0x7D]`); the fix is a 2-line staging; prove whether the `0x816DF` invoke then fires (census for action `22`).

- [ ] **Step 1: The staging fix.** First-hand bytes at the engine's row-07 gate staging (`handlers.c:984-985`): what the engine currently hardcodes (`opp_angle_z = 0`/`opp_face_word = 0`) vs the native reads; the exact 2-line fix.
- [ ] **Step 2: The invoke condition.** The `0x816DF` fold (`entity_angle(word[+0x6D], word[+0x6F]) − word[+0x7D]` folded `< 0x55`, `0x816B4 AND AH,0x3`) — fresh bytes; confirm the threshold.
- [ ] **Step 3: The reachability.** With the fix, is the invoke reachable from a live/fixture row-07 dispatch? What must be staged (the two records' angle cells) for the fold to pass?
- [ ] **Step 4: Verdict + legs.** The port shape (the staging fix + the fixture proof); the **census ruling** for row `22` (does it install AND dispatch in-engine/fixture?); numbered legs. Hand off.

---

### Task 5: W25-5 / FU260 — the ECX-carried installer re-census (→ `27`/`29`/`2B`/`2C`/`0A`)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase25-census-closeout-recon-ahead/w25-5-installer-recensus.md`
**Promotes to:** `docs/ghidra/FU260_installer_recensus.md`

**Objective:** Re-run the installer census with the **ECX-carried** method phase-23 pioneered (which refuted the "no installer" verdicts for rows `25`/`15`) over ALL unwired action rows, and settle the `27`/`29`/`2B`/`2C`/`0A` verdicts.

- [ ] **Step 1: The method.** Re-state the method first-hand: the bulk installer `FUN_0008CEB8(rec,i0,i1,code,skip)` carries `code` in `ECX` (`B9`), invisible to the `MOV EDX/DX,code` literal scan; the `0x7D9A4` per-record installer; the `0x888FC` act seam.
- [ ] **Step 2: The parametric sweep.** For each unwired action row `27`/`29`/`2B`/`2C`/`0A`: does any `FUN_0008CEB8`/`0x7D9A4` call site carry the code (immediate or register-derived)? Fresh `get_xrefs_to` per candidate site.
- [ ] **Step 3: The `0A` case.** `0A` is unported with installer `0x7CDD8` (no xrefs, OL-14); re-verify the no-xrefs claim fresh, and whether the ECX method finds a carrier.
- [ ] **Step 4: Verdict + legs.** Per-row verdicts (`27`/`29`/`2C` may un-gate; `2B` dead entry = the shared row-29 RET; `0A` needs porting); the **census ruling** per row; numbered legs. Hand off.

---

### Task 6: W25-6 / FU261 — the unwired phase bodies A (`01`/`0B`/`0D`/`0E`/`11`/`12`)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase25-census-closeout-recon-ahead/w25-6-phase-bodies-a.md`
**Promotes to:** `docs/ghidra/FU261_phase_bodies_a.md`

**Objective:** Derive the native bodies + producer-writers for the six NULL phase slots `01`/`0B`/`0D`/`0E`/`11`/`12`, settling the R10 "do not double-wire" question (`01`/`11`/`12` share the formation-placement body) and the "never written by an engine source" question (FU191 §10).

- [ ] **Step 1: `01`/`11`/`12`.** Confirm the shared body `0x6E1D0` (= wired phase `0x03`/`0x04`/`0x07`) and whether wiring the slots is correct or a double-wire; is each phase number written by a live engine source?
- [ ] **Step 2: `0B`/`0D`/`0E`.** The native bodies (`0x110794[0x0B/0x0D/0x0E]`) — fresh bytes; what they do; their writers.
- [ ] **Step 3: The writers.** For each: a fresh `get_xrefs_to` on the phase-write sites — is there a live engine path that writes the phase number (so the walk dispatches the slot)?
- [ ] **Step 4: Verdict + legs.** The port shapes; the **census ruling** per phase; numbered legs. Hand off.

---

### Task 7: W25-7 / FU262 — the unwired phase bodies B (`13`/`14`/`1A`/`1B`/`1C`)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase25-census-closeout-recon-ahead/w25-7-phase-bodies-b.md`
**Promotes to:** `docs/ghidra/FU262_phase_bodies_b.md`

**Objective:** Derive the native bodies + producer-writers for the five act-tail phase slots `13`/`14`/`1A`/`1B`/`1C`, incl. the FU-239 collapsed-arm question for `13`/`14`.

- [ ] **Step 1: `13`/`14`.** The native bodies + the engine's collapsed `0x13`/`0x14` arm (FU239-L5); the missing score-equality guard `0x8BD95`.
- [ ] **Step 2: `1A`/`1B`/`1C`.** The native bodies (`0x110794[0x1A/0x1B/0x1C]`) — fresh bytes; the act-8/act-tail producers.
- [ ] **Step 3: The writers.** For each: a fresh `get_xrefs_to` on the phase-write sites — is there a live engine path that writes the phase number?
- [ ] **Step 4: Verdict + legs.** The port shapes; the **census ruling** per phase; numbered legs. Hand off.

---

### Task 8: W25-8 / FU263 — the act-4 formation arm (`17`) + the phase-24 errata sweep

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase25-census-closeout-recon-ahead/w25-8-formation-arm-errata.md`
**Promotes to:** `docs/ghidra/FU263_formation_arm_errata.md`

**Objective:** Re-verify the act-4 formation arm verdict (`17` — phase-24 proved it gated) against the frozen `FU248`, and assemble the **phase-24 corrections sweep** the wave surfaced, as a port/errata list for Stage 2.

- [ ] **Step 1: The formation arm.** Re-verify `FU248`'s gated verdict (`FUN_00011620` = front-end producer; the reset copies desired→current; the only divergence candidate `FUN_00041494` case 5 is dynamic-only) — confirm row `17` stays a leg.
- [ ] **Step 2: The errata list.** Assemble: the `FU247` OL-65 erratum correction (FU250 proved FU247's erratum wrong — `word[+0x6F]`, not `word[+0x6B]`), the `OL-83` unification (unify on `record.type`, retire `actor_type` — FU252/FU253), the `ENGINE.md:4037` row-18 over-list, the `FU-84 §1` errata (the reroll RNG is not discarded), the `FU201-L1` owner (`FUN_00041494` case 5).
- [ ] **Step 3: The port shape.** For each errata: the exact doc/code edit (a `docs(fuNNN)` errata commit, or a code comment fix).
- [ ] **Step 4: Verdict + legs.** The consolidated errata/port list; numbered legs. Hand off.

---

## Stage 2 (post-recon, separate plan)

After all 8 drafts are frozen and promoted, the controller writes
`docs/superpowers/plans/2026-10-14-fifa96-m2-phase25-census-closeout.md`
off the frozen `FU256…FU263`, carrying spec §5's T-structure: T1 the determinism +
feasibility gate (census recompute + ≥3-replay byte-stability, no re-pin); T2..Tk
one gating-producer port per proven-tractable cluster (per-port RED-first
dispatch/frame tests + mutation-RED riders); the installer re-census landing; the
census close-out (only if a producer wires a row; the four-doc same-commit rule);
the corrections + leg-register close-out.

## Self-Review

- **Spec coverage:** spec §4's 8 slices map 1:1 to Tasks 1–8 (FU256→Task 1 … FU263→Task 8). The producer clusters of spec §1's frontier table are covered: row-09 arm (T1), CPU ladder (T2), `[rec+0x1C]` chain (T3), OL-65 (T4), installer re-census (T5), phase bodies A/B (T6/T7), formation arm + errata (T8).
- **Placeholders:** none — each task names its objective, exact addresses/targets, the evidence floor, the deliverable and the handoff. The Stage-2 T-list is deliberately parametrized (derives from the frozen slices).
- **Consistency:** slice numbers (W25-1…W25-8 ↔ FU256…FU263), draft paths and the promotion path are consistent; the frontier table matches spec §1; the census rule (install AND dispatch) is stated in Global Constraints and repeated per task.
- **Deferred:** the Stage-2 producer plan is the next artifact; the census is published from what the wave proves (spec §6 caveat — the honest ceiling is below 80).

## Handoff

Hand this plan to `superpowers:subagent-driven-development`: dispatch a fresh
read-only subagent per task (all 8 in parallel), then the controller freeze-reviews
and promotes each draft to a canonical `docs/ghidra/FU2xx_*.md` in a serialized
commit per slice.
