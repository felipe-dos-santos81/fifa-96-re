# FIFA 96 M2 phase-26b recon-ahead — the remaining phase-7 chains + R11-L4

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` to execute this plan. The 8 recon slices are **read-only** and run **in parallel**. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Freeze the eight read-only evidence slices (`FU273…FU280`) that establish, first-hand, the native **live producer** of each remaining phase-7 chain's gate cell — and, for R11-L4, an explicit **port shape + go/no-go** — so the phase-26b producer plan can be written without guessing.

**Architecture:** This is the phase-26 **Stage 1** (recon-ahead), the phase-21…26 shape, applied to the 26b half of the phase-26 spec. The approved design (`docs/superpowers/specs/2026-10-14-fifa96-m2-phase26b-remaining-chains-r11l4-design.md` §3) sequences the recon before the implementation plan; the Stage-2 producer tasks are authored off these frozen slices. Each of the 8 agents investigates one slice, writes only its draft, and hands off; the controller freeze-reviews and promotes to a canonical `docs/ghidra/FU2xx_*.md` in a serialized commit.

**Tech Stack:** Ghidra MCP (read-only), REA MCP (second read), C11/CMake/CTest, ASan/UBSan, SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase26b-remaining-chains-r11l4-design.md` (approved). Parent spec: `docs/superpowers/specs/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-design.md`. Predecessor plan: `docs/superpowers/plans/2026-10-14-fifa96-m2-phase26a-camera-ball-visibility.md` (closed as the honest waiver). Recon-ahead precedent: `docs/superpowers/plans/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-recon-ahead.md`.

## Global Constraints

- **Ghidra stays read-only.** Program argument exactly `/FIFA96.EXE` (case-sensitive). **No** rename / comment / label / function-create / script / project save. Standard MCP tools only. (context7 is **not applicable** — RE, not library docs.)
- **Baseline (HEAD `d3bdc51`, the 26b spec commit):** `make check` **110/110**; wired **59/80 = 73%** (`36 action + 23 phase`) — the census is **closed**; M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (**691 lines, immutable**); M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (**v22, 6522 lines**).
- **Evidence floor (AGENTS.md):** every address/byte claim cites a fresh disassembly output or reconciled raw-byte read. Anything not statically provable becomes a **numbered open leg** with its native owner. LE-image globals at `+0x100000`.
- **Alias traps:** `dword[addr] >> 16` = the word at `addr+2`, `>> 24` = the byte at `addr+3`; verify CALL bytes (rel32); watch sign/width, inverted gates, gate-index misreads. Any "only/sole/exactly" or caller-count claim needs a **fresh** `get_xrefs_to`. (The phase-24/25/26 waves caught alias-trap errors — re-verify before trusting an inherited verdict.)
- **The live-chain rule:** a chain is **live** only when a **ported producer raiser** raises its gate on a live engine path (the frame loop / a granted-frame scenario) — a fixture that stages the gate does **not** count; a stand-in producer is a fake. Report the exact live trigger and whether it is **headless-reachable** (Layer 1) and **smoke-reachable** (Layer 2).
- **The R11-L4 go/no-go:** FU273 must return an explicit **GO** (the keeper CPU chain is portable within a tractable task count) or **NO-GO** (phase-scale → the honest-stop branch, spec §6), with the machine inventory + the effort estimate behind the ruling.
- **No code, no test, no shared-doc edits** by the recon agents. Each writes **only** its own draft file (+ a workspace report). The controller promotes drafts to canonical `docs/ghidra/FU2xx_*.md` and commits.
- **No fakes** (R15/R45/R49). A cell with no live producer stays a numbered leg.
- **No census chase.** A live producer that wires a NULL row is honest **upside** only; the census holds at 59/80 otherwise.
- **M2 preservation:** byte-identical unless a producer genuinely changes a live path (written reason + frame diff).
- **Scope guard:** the native **live producers** of the remaining phase-7 chains + the R11-L4 keeper CPU chain only. No front-end *visual* work (a front-end-produced cell is a leg), no audio, no new subsystem.

## File Structure

| Path | Responsibility |
|---|---|
| `.superpowers/sdd/2026-10-14-fifa96-m2-phase26b-recon-ahead/progress.md` | The SDD ledger (recovery map) |
| `.superpowers/sdd/2026-10-14-fifa96-m2-phase26b-recon-ahead/w26b-<n>-*.md` | The 8 recon drafts (git-ignored scratch) |
| `docs/ghidra/FU273_*.md … FU280_*.md` | The promoted canonical slices (controller, serialized commits) |
| Prior slices `FU264…FU272`, `FU266`, `FU267`, `FU268`, `FU269`, `FU270`, `FU241`, `FU149`, `FU150`, `FU151`, `FU152`, `FU180`, `FU186`, `ENGINE.md` leg register | Prior evidence — appended errata only, never rewritten |

**Ordering (dependency):** Tasks 1–8 are independent read-only investigations and run in **parallel**. FU273 (R11-L4) is table-first because its GO/NO-GO is the largest plan-shaping input, but it is **not** a serial gate — per spec §8 D2 all 8 fan out flat and the verdict is absorbed at plan time. The controller reconciles at promotion time only if two slices reach the same producer (e.g. Tasks 2 and 3 both touch the taker-arm state cells; Tasks 5 and 1 both touch the keeper cluster).

---

### Task 1: W26b-1 / FU273 — R11-L4 (the keeper CPU chain) — port shape + go/no-go

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26b-recon-ahead/w26b-1-r11l4-keeper-cpu-chain.md`
**Promotes to:** `docs/ghidra/FU273_r11l4_keeper_cpu_chain.md`

**Objective:** Derive the **port shape** of R11-L4 — the keeper CPU chain `FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC`, the rows `1A`/`1B` body remainders, the phase-6/row-`0x1F` chain, and the claim bridge (R11-L1/L2/L3) — so the keeper stage-5 release (`camY := 0x50`) raises the camera-Y cell on a live path. Return an explicit **GO / NO-GO** with a machine inventory and an effort estimate.

- [ ] **Step 1: The installer chain.** Fresh bytes: `FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC`; the fresh `get_xrefs_to` caller counts (the FU272 census: `0x77EAC`→1 (`0x78599`), `0x782D0`→1 (`0x8DB35`), `0x8D8EC`→4 (`0x4B2AB/0x4B2B2`, `0x74429/0x74430`)); what each machine installs.
- [ ] **Step 2: The sit-5 producers.** Rows `1A` (`0x7662C`) / `1B` (`0x76D28`) — the `CALL 0x8A938 EAX=5,EBX=1` emit sites (`0x76B1C`/`0x77707`) and what dispatches the rows live; the phase-6 arm `0x8D558` (row-`0x1F`).
- [ ] **Step 3: The claim bridge.** The stage-5 release cut (`0x75D53`/`0x75D69`, `camY := 0x50`) and the claim bridge R11-L1/L2/L3 that reaches it; the phase-6/row-`0x1F` chain.
- [ ] **Step 4: The machine inventory + the GO/NO-GO.** Enumerate every machine to port (ported vs unported), estimate the Stage-2 task count, and rule **GO** (tractable) or **NO-GO** (phase-scale → spec §6 honest-stop). Hand off.

---

### Task 2: W26b-2 / FU274 — the set-piece/restart live arm

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26b-recon-ahead/w26b-2-set-piece-live-arm.md`
**Promotes to:** `docs/ghidra/FU274_set_piece_live_arm.md`

**Objective:** Derive the live engine arm that dispatches the taker rows `0x10`/`0x12`/`0x13` — the throw-in scanner + the foul adjudicator — and the `goal_armed`→phase 3/4/6/7 producers that gate them.

- [ ] **Step 1: The taker state cells.** `[0x157821]` (row-`0x10` stage-4 re-queue gate, `fifa96_match_handlers.c:1724`; the engine writes it only as resets `run.c:2087`/`2219`) + `[0x157A6A]` (`mr->referee.offside_suppress`, produced `fifa96_match_handlers.c:1555`); the fresh native writers of each.
- [ ] **Step 2: The throw-in scanner + the foul adjudicator.** The native producers that would fire rows `0x10`/`0x12`/`0x13` live (fresh bytes); the `goal_armed`→phase-3/4/6/7 route (`fifa96_match_run_goal_scan` `run.c:3274`, throw-in `:3300`, corner `:3316`; `fifa96_match_goal_arm` `run.c:3010`).
- [ ] **Step 3: Live vs fixture.** Which taker rows a live engine path can reach today (row `0x11` is live via the derived FU-170 restart wire); which need a new producer.
- [ ] **Step 4: The port shapes + the live-trigger ruling.** What makes `0x10`/`0x12`/`0x13` fire live; hand off.

---

### Task 3: W26b-3 / FU275 — the taker-arm residuals (FU266 L266-1..14)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26b-recon-ahead/w26b-3-taker-arm-residuals.md`
**Promotes to:** `docs/ghidra/FU275_taker_arm_residuals_2.md`

**Objective:** Derive the **port shapes** of FU266's open legs `L266-1..14` — the per-phase triple `FUN_00083164`, the pick `FUN_0008DB6C`, the timer+draw machine `FUN_00083428`, the presentation sinks `0x974DC(0x1E)`/`0x8F188`, and the `[0x157821]`/`[0x157A6A]` state cells.

- [ ] **Step 1: The per-phase triple + the pick.** `FUN_00083164` / `FUN_0008DB6C` — fresh bytes; the receiver-pick origins; the event sub-tables.
- [ ] **Step 2: The timer+draw machine.** `FUN_00085498` / `FUN_00083428` — fresh bytes; the completion cadence.
- [ ] **Step 3: The sinks + the state cells.** `0x974DC(0x1E)` / `0x8F188`; the `[0x157821]` shadow (`sp_157821` divergence, L266-8); the `[0x157A6A]` reset; the penalty hand-off.
- [ ] **Step 4: The port shapes + the live-trigger ruling.** Hand off.

---

### Task 4: W26b-4 / FU276 — fouls/offside + the contact gate

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26b-recon-ahead/w26b-4-fouls-offside-contact.md`
**Promotes to:** `docs/ghidra/FU276_fouls_offside_contact.md`

**Objective:** Derive the live **contact-gate invoker** and the unported **offside** path — the row-`0x04` duel / the pad `want 0x60` handler that raises the contact gate, the offside receive arm `FUN_0007A084 0x7A448`, `field_4c2f2`, and the `FU268-L10` byte divergence.

- [ ] **Step 1: The contact gate.** `fifa96_match_run_contact` (`run.c:6489`, called from row `0x0C` `fifa96_match_handlers.c:3329` → act-3 invoke-now `run.c:6530`); what live path raises the contact precondition; the pad `want 0x60` handler.
- [ ] **Step 2: The offside entry.** `fifa96_match_run_offside_reception` (`run.c:6544`, no engine caller today — only unit tests `tests/test_engine_match_frame.c:4236`…); the unported receive arm `FUN_0007A084 0x7A448`; the offside-enable producer for `field_4c2f2`.
- [ ] **Step 3: The L10 divergence.** The offside eligibility byte: native `code` / `+0x91` vs engine `type` / `+0x8E` (`match_run.c:6597`); the reconciliation.
- [ ] **Step 4: The port shapes + the live-trigger ruling.** Hand off.

---

### Task 5: W26b-5 / FU277 — the keeper-restart trigger

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26b-recon-ahead/w26b-5-keeper-restart-trigger.md`
**Promotes to:** `docs/ghidra/FU277_keeper_restart_trigger.md`

**Objective:** Derive the live upstream trigger for the keeper restart rows `1D`/`1E` — the live `goal_armed`/goal-kick producer (the keeper CPU ladder) — and the `FU269-L5` sit-5 emit-route divergence.

- [ ] **Step 1: The upstream trigger.** Rows `1D`/`1E` install at phase 8/9 (`fifa96_match_run_phase_arm` `run.c:2859`; `FUN_0008D098` arm `0x8D607`); phases 8/9 come only from situations 4/5 (`fifa96_action_handlers.c:1137-1138`); the engine set-piece call census (`run.c:3058`…`6406`) passes no situation 4/5 — the missing live caller.
- [ ] **Step 2: The keeper ladder.** `0x7546E`/`0x75B58`/`0x76072`; the post-period hold `FUN_0008BAF0 0x8BC01` (`OL-86`); `FU269-L1/L2` downstream.
- [ ] **Step 3: The sit-5 emit-route divergence (`FU269-L5`).** Native `bx=1` vs the engine hardcoded `side=0/bx=0` (`match_run.c:2948-2950`).
- [ ] **Step 4: The port shapes + the live-trigger ruling.** Hand off.

---

### Task 6: W26b-6 / FU278 — replay / overlay / sub-strip

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26b-recon-ahead/w26b-6-replay-overlay-sub.md`
**Promotes to:** `docs/ghidra/FU278_replay_overlay_sub.md`

**Objective:** Derive the live **caller** from the period/screen machine for the replay/overlay/sub producers — `FUN_000510DC` / `FUN_00054104` / `FUN_000515C8` — and the substitution-event writer (`0x8A938` case 6/8).

- [ ] **Step 1: The replay arm entry.** `FUN_000510DC` / `FUN_00064270` / `[0x109A98]`; the engine `fifa96_match_run_replay_step` (`run.c:4624`) has **no** caller — the missing period/screen-machine caller.
- [ ] **Step 2: The overlay + the sub event.** `FUN_00054104` / `FUN_00054238` / `FUN_000550E4` / the driver `FUN_000515C8`; the sub-event writer `0x8A938` case 6/8 (`0x8AE54`); `ball_row` `[0x14E660]` (no producer).
- [ ] **Step 3: The live reach.** Which live path (if any) raises `replay.state`/`sub.active`/`overlay.armed`; the `ball_row` NULL verdict.
- [ ] **Step 4: The port shapes + the live-trigger ruling.** Hand off.

---

### Task 7: W26b-7 / FU279 — the situation-0xB producers + the act-8 tail

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26b-recon-ahead/w26b-7-situation-producers-sinks.md`
**Promotes to:** `docs/ghidra/FU279_situation_producers_sinks.md`

**Objective:** Derive the remaining situation-0xB producers (`FU267`: the six dormant rows) + the row-01 sinks (`0x974DC(0x1E)` / `0x8F188` / `0x8DCD4`) + the `FU241-L1/L3` residues.

- [ ] **Step 1: The six dormant producers.** The `0x8A938(0xB, side, BX=0)` tails for rows `0x10`/`0x12`/`0x13` + keeper `1D`/`1E` (fresh bytes); what live arm each needs.
- [ ] **Step 2: Row-01's sinks.** The sound `0x974DC(0x1E)` + ring `0x8F188` (the OL-67 event-ring seam) + the metric/ball staging `0x8DCD4`→`0x158738` / `0x7A490` (the FU-139 staging block).
- [ ] **Step 3: The act-8/act-0xA tail.** The `FU241-L1` (non-tape equivalence) + `FU241-L3` (act-slot residency) residues.
- [ ] **Step 4: The port shapes + the live-trigger ruling.** Hand off.

---

### Task 8: W26b-8 / FU280 — the live-trigger census (re-cut)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26b-recon-ahead/w26b-8-live-trigger-census.md`
**Promotes to:** `docs/ghidra/FU280_live_trigger_census_2.md`

**Objective:** The cross-cutting reachability map for the 26b half: for each remaining phase-7 gating cell, the live engine path that raises it, whether it is **interactively reachable** (smoke) vs **granted-frame-only** (headless Layer 1) — and, given FU273's verdict, the **visible-anchor verdict** (is the follow-cam reachable once R11-L4 lands?).

- [ ] **Step 1: The cell census.** Enumerate the remaining gating cells (`[0x157750]`, `[0x1577CA]`, `[0x157821]`/`[0x157A6A]`, `ref_machine`, `replay.state`, `overlay.armed`, `field_4c2f2`, the keeper/taker cells) + their writers; carry forward FU271's map as prior evidence.
- [ ] **Step 2: The live-path map.** For each: which ported path could raise it; fixture-only vs live; ported vs unported.
- [ ] **Step 3: The reachability split + the visible-anchor verdict.** Which chains are smoke-reachable (Layer 2 candidates) vs granted-frame-only (Layer 1); is the follow-cam reachable once R11-L4 + the camera producer are live (dependent on FU273).
- [ ] **Step 4: The verdict + the Stage-2 sizing input.** Which chains are live-portable; hand off.

---

## Stage 2 (post-recon, separate plan)

After all 8 drafts are frozen and promoted, the controller writes the phase-26b
producer plan off the frozen `FU273…FU280`, carrying spec §4's T-structure: **T1**
the determinism + reachability gate (the FU280 map); **T2 = R11-L4** (the critical
path — the keeper CPU chain; if FU273 rules NO-GO, spec §6's honest-stop branch
applies); **T3..T6** one live producer per chain (each a headless granted-frame
reach test + a mutation-RED rider); **T(visible)** the follow-cam smoke acceptance
or an honest waiver; **T(last)** the census/leg-register close-out. **Per spec §4,
if the frozen slices show the plan is too large, Stage-2 splits into 26b-i (the
chains) and 26b-ii (R11-L4) off this spec.**

## Self-Review

- **Spec coverage:** spec §3's 8 slices map 1:1 to Tasks 1–8 (FU273→Task 1 … FU280→Task 8). Spec §1's five families are covered: R11-L4 (T1), set-piece/restart (T2/T3), fouls/offside (T4), keeper restart (T5), replay/overlay (T6), plus the situation/sink residues (T7) and the cross-cutting reachability map (T8). Spec §6's honest-stop rule is stated in Global Constraints and in Task 1 Step 4 + Stage 2.
- **Placeholders:** none — each task names its objective, exact addresses/targets, the evidence floor, the deliverable and the handoff. The Stage-2 T-list is deliberately parametrized (derives from the frozen slices; spec §4 allows the 26b-i/26b-ii split).
- **Consistency:** slice numbers (W26b-1…W26b-8 ↔ FU273…FU280), draft paths and the promotion path are consistent; the family table matches spec §1; the live-chain rule (a ported producer raiser, not a fixture) is stated in Global Constraints and repeated per task; the R11-L4 GO/NO-GO wording matches spec §3/§4/§6.
- **Deferred:** the Stage-2 producer plan (or the 26b-i/26b-ii split) is the next artifact; the reachability map (FU280) + the R11-L4 verdict (FU273) are the sizing inputs (spec §4's split decision).

## Handoff

Hand this plan to `superpowers:subagent-driven-development`: dispatch a fresh
read-only subagent per task (all 8 in parallel), then the controller freeze-reviews
and promotes each draft to a canonical `docs/ghidra/FU2xx_*.md` in a serialized
commit per slice.
