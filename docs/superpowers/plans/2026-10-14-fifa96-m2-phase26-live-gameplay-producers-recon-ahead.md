# FIFA 96 M2 phase-26 recon-ahead — the live-gameplay producers

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` to execute this plan. The 8 recon slices are **read-only** and run **in parallel**. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Freeze the eight read-only evidence slices (`FU264…FU271`) that establish, first-hand, the native **live producer** of each phase-7 chain's gate cell — its exact body, its live trigger, and whether a live engine path can raise it (Layer 1 headless, Layer 2 visible for the camera) — so the phase-26 producer plan can be written without guessing.

**Architecture:** This is the phase-26 **Stage 1** (recon-ahead), exactly the phase-21…25 shape. The approved design (`docs/superpowers/specs/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-design.md` §4) sequences the recon before the implementation plan; the Stage-2 producer tasks are authored off these frozen slices. Each of the 8 agents investigates one cluster slice, writes only its draft, and hands off; the controller freeze-reviews and promotes to a canonical `docs/ghidra/FU2xx_*.md` in a serialized commit.

**Tech Stack:** Ghidra MCP (read-only), REA MCP (second read), C11/CMake/CTest, ASan/UBSan, SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-design.md` (approved). Predecessor plan: `docs/superpowers/plans/2026-10-14-fifa96-m2-phase25-census-closeout.md`. Recon-ahead precedent: `docs/superpowers/plans/2026-10-14-fifa96-m2-phase25-census-closeout-recon-ahead.md`.

## Global Constraints

- **Ghidra stays read-only.** Program argument exactly `/FIFA96.EXE` (case-sensitive). **No** rename / comment / label / function-create / script / project save. Standard MCP tools only. (context7 is **not applicable** — RE, not library docs.)
- **Baseline (HEAD `df2aaca`, the spec commit):** `make check` **110/110**; wired **59/80 = 73%** (`36 action + 23 phase`) — the census is **closed**; M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (**691 lines, immutable**); M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (**v22, 6522 lines**).
- **Evidence floor (AGENTS.md):** every address/byte claim cites a fresh disassembly output or reconciled raw-byte read. Anything not statically provable becomes a **numbered open leg** with its native owner. LE-image globals at `+0x100000`.
- **Alias traps:** `dword[addr] >> 16` = the word at `addr+2`, `>> 24` = the byte at `addr+3`; verify CALL bytes (rel32); watch sign/width, inverted gates, gate-index misreads. Any "only/sole/exactly" or caller-count claim needs a **fresh** `get_xrefs_to`. (The phase-24/25 waves caught alias-trap errors — re-verify before trusting an inherited verdict.)
- **The live-chain rule:** a chain is **live** only when a **ported producer raiser** raises its gate on a live engine path (the frame loop / a granted-frame scenario) — a fixture that stages the gate does **not** count; a stand-in producer is a fake. Report the exact live trigger and whether it is **headless-reachable** (Layer 1) and **smoke-reachable** (Layer 2).
- **No code, no test, no shared-doc edits** by the recon agents. Each writes **only** its own draft file (+ a workspace report). The controller promotes drafts to canonical `docs/ghidra/FU2xx_*.md` and commits.
- **No fakes** (R15/R45/R49). A cell with no live producer stays a numbered leg.
- **No census chase.** A live producer that wires a NULL row is honest **upside** only; the census holds at 59/80 otherwise.
- **M2 preservation:** byte-identical unless a producer genuinely changes a live path (written reason + frame diff).
- **Scope guard:** the native **live producers** of the phase-7 chains only. No front-end *visual* work (a front-end-produced cell is a leg), no audio, no new subsystem.

## File Structure

| Path | Responsibility |
|---|---|
| `.superpowers/sdd/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-recon-ahead/progress.md` | The SDD ledger (recovery map) |
| `.superpowers/sdd/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-recon-ahead/w26-<n>-*.md` | The 8 recon drafts (git-ignored scratch) |
| `docs/ghidra/FU264_*.md … FU271_*.md` | The promoted canonical slices (controller, serialized commits) |
| Prior slices `FU-148`…`FU-152`, `FU-168`, `FU-170`, `FU-171`, `FU-180`, `FU-186`, `FU-194`, `FU-204`, `FU-207`, `FU-208`, `FU-209`, `FU-241`, `ENGINE.md` leg register | Prior evidence — appended errata only, never rewritten |

**Ordering (dependency):** Tasks 1–8 are independent read-only investigations and run in parallel. The controller reconciles at promotion time only if two slices reach the same producer (e.g. Tasks 5 and 8 both touch the extra-time flag `OL-85`).

---

### Task 1: W26-1 / FU264 — the open-play camera-Y producer (the visible anchor)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-recon-ahead/w26-1-camera-y-producer.md`
**Promotes to:** `docs/ghidra/FU264_camera_y_producer.md`

**Objective:** Derive what raises `[0x157750]` (the camera-Y / ball-height cell) off zero on a **live open-play path** (FU168-L5), and whether the follow-cam is **smoke-visible** (the Layer-2 anchor).

- [ ] **Step 1: The cell.** Fresh bytes: every writer of `[0x157750]` (`get_xrefs_to`); the ported producer `FUN_00070B94` + the integrator store (`fifa96_camera_height`); which are live vs fixture-only.
- [ ] **Step 2: The open-play gap.** FU168-L5: why no live path raises it (the keeper release is dispatch-dead R11-L4); what a live open-play event must do to raise it (the height-table `d >= 0x38` band).
- [ ] **Step 3: The view-object handler (R2-L1).** The handler that keeps the ball framed after the roll — fresh bytes; live vs unported.
- [ ] **Step 4: The port shape + the Layer-2 verdict.** The producer port shape; the **live-vs-leg ruling**; is the follow-cam smoke-visible or granted-frame-only? Hand off.

---

### Task 2: W26-2 / FU265 — the live place/pan reachability + the ball framing

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-recon-ahead/w26-2-place-pan-ball-framing.md`
**Promotes to:** `docs/ghidra/FU265_place_pan_ball_framing.md`

**Objective:** Derive the live-camera place/pan reachability (R2-L4: which live event fires the pan) + the row-07 `+0x44` animation ack (T1-L2, the one-possession second-kick blocker) + the on-screen ball framing.

- [ ] **Step 1: The pan event (R2-L4).** Which live event fires the pan/place (the `FUN_000736AC` pan step's admission); fresh `get_xrefs_to`; the band-reachable vs smoke-reachable question.
- [ ] **Step 2: The `+0x44` ack (T1-L2).** The row-07 stage-2 ack that blocks a second kick in one possession — producer + live reachability.
- [ ] **Step 3: The ball framing.** The camera-0 view seed + the ball-slot projection (`[0x157750]`/`[0x1577CA]` coupling); what keeps the ball framed live.
- [ ] **Step 4: The port shapes + the live-vs-leg ruling.** Hand off.

---

### Task 3: W26-3 / FU266 — the taker-arm residuals (FU-149 L13)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-recon-ahead/w26-3-taker-arm-residuals.md`
**Promotes to:** `docs/ghidra/FU266_taker_arm_residuals.md`

**Objective:** Derive the taker-arm producers (`FU-149 L13.1–L13.7`) that keep the set-piece arms **fixture-dormant** — the throw setup, the timer+draw completion, the arm vector/RNG, the `0x974DC(0x1E)`/`0x8F188` sinks, the `0x8DE8C` picks, the `[0x157821]`/`[0x157A6A]` state cells, the penalty.

- [ ] **Step 1: L13.1–L13.2.** The `FUN_000832A8`/`FUN_00083164`/`FUN_0008DB6C` throw setup + the timer+draw completion (fresh bytes).
- [ ] **Step 2: L13.3–L13.5.** The arm vector/RNG cadence, the `0x974DC(0x1E)`/`0x8F188` sinks, the `0x8DE8C` pick origins.
- [ ] **Step 3: L13.6–L13.7.** The `[0x157821]`/`[0x157A6A]` state producers + the penalty.
- [ ] **Step 4: The port shapes + the live-trigger ruling.** What makes `0x10..0x13` fire live; hand off.

---

### Task 4: W26-4 / FU267 — the situation-0xB producers (OL-84)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-recon-ahead/w26-4-situation-producers.md`
**Promotes to:** `docs/ghidra/FU267_situation_producers.md`

**Objective:** Derive the remaining situation-0xB producers (`OL-84`: row 02, rows `0x10..0x13`, the keeper/restart bodies, the computed act-8 call) + row-01's camera/event/ball-stage sinks.

- [ ] **Step 1: The situation machine.** The remaining situation-0xB producers (fresh bytes) + what each un-blocks.
- [ ] **Step 2: The computed act-8 call.** The `0x8A8CE`/`0x8B85D` route + its live producer.
- [ ] **Step 3: Row-01's sinks.** The camera/event/ball-stage sinks.
- [ ] **Step 4: The port shapes + the live-trigger ruling.** Hand off.

---

### Task 5: W26-5 / FU268 — the fouls/offside producers + the extra-time flag

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-recon-ahead/w26-5-fouls-offside-extratime.md`
**Promotes to:** `docs/ghidra/FU268_fouls_offside_extratime.md`

**Objective:** Derive the `FU-150` foul/referee/offside live producers (the sit-9/0xA entries) + the extra-time flag `OL-85` (`[0x157AC0]`; `FU207-L2` the `0x389A4` front-end producer) + `FU208-L1`.

- [ ] **Step 1: The `FU-150` entries.** The sit-9/0xA producers that would fire the foul/referee chain (fresh bytes); live vs unported.
- [ ] **Step 2: The extra-time flag `OL-85`.** `[0x157AC0]`'s writers; `0x389A4` (front-end) vs an engine producer.
- [ ] **Step 3: `FU208-L1`.** The bit-8 arm (no located range-word writer).
- [ ] **Step 4: The port shapes + the live-trigger ruling** (note any front-end-produced cell → leg). Hand off.

---

### Task 6: W26-6 / FU269 — the keeper restart rows + the post-period hold

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-recon-ahead/w26-6-keeper-restart-hold.md`
**Promotes to:** `docs/ghidra/FU269_keeper_restart_hold.md`

**Objective:** Derive the `FU-151` keeper restart rows `1D`/`1E` live producers (the stage ladder, `0x7546E`/`0x75B58`/`0x76072`) + the post-period hold `OL-86`.

- [ ] **Step 1: Rows `1D`/`1E`.** The live producers for the keeper restart rows (fresh bytes); the m 42 isolation issue.
- [ ] **Step 2: The stage ladder.** `0x7546E`/`0x75B58`/`0x76072` — what they need to run live.
- [ ] **Step 3: `OL-86`.** The post-period hold/reset timing.
- [ ] **Step 4: The port shapes + the live-trigger ruling.** Hand off.

---

### Task 7: W26-7 / FU270 — the replay/overlay/sub-strip producers (FU-152)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-recon-ahead/w26-7-replay-overlay.md`
**Promotes to:** `docs/ghidra/FU270_replay_overlay.md`

**Objective:** Derive the `FU-152` replay/overlay/sub-strip live producers (legs 1/2/3/7; the rows are zero-gated).

- [ ] **Step 1: The gating cells.** `replay.state`/`sub.active`/`overlay.armed` — the native writers (fresh bytes).
- [ ] **Step 2: Legs 1/2/3/7.** The producer shapes per leg.
- [ ] **Step 3: The live reach.** Which live path (if any) raises them; the `ball_row` NULL case.
- [ ] **Step 4: The port shapes + the live-trigger ruling.** Hand off.

---

### Task 8: W26-8 / FU271 — the live-trigger census (cross-cutting)

**Draft:** `.superpowers/sdd/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-recon-ahead/w26-8-live-trigger-census.md`
**Promotes to:** `docs/ghidra/FU271_live_trigger_census.md`

**Objective:** The cross-cutting reachability map: for each phase-7 gating cell, the live engine path that raises it, and whether it is **interactively reachable** (smoke) vs **granted-frame-only** (headless Layer 1) — the map that sizes the Stage-2 plan and anchors the visible acceptance.

- [ ] **Step 1: The cell census.** Enumerate every phase-7 gating cell (`[0x157750]`, `[0x1577CA]`, `[0x157AC0]`, `[0x157821]`/`[0x157A6A]`, `ref_machine`, `replay.state`, `overlay.armed`, the taker/foul/keeper cells) + its writers.
- [ ] **Step 2: The live-path map.** For each: which ported path could raise it; fixture-only vs live; ported vs unported.
- [ ] **Step 3: The reachability split.** Which chains are smoke-reachable (Layer 2 candidates) vs granted-frame-only (Layer 1).
- [ ] **Step 4: The verdict + the Stage-2 sizing input.** Which chains are live-portable; hand off.

---

## Stage 2 (post-recon, separate plan)

After all 8 drafts are frozen and promoted, the controller writes the phase-26
producer plan(s) off the frozen `FU264…FU271`, carrying spec §5's T-structure: T1
the determinism + reachability gate (the FU271 map); T2..Tk one live producer per
chain (each a headless granted-frame reach test + a mutation-RED rider); the
follow-cam visible acceptance; the census/leg-register close-out. **Per spec §5, if
the frozen slices show the plan is too large, it splits into phase-26a (the camera
cluster, incl. the visible anchor) and phase-26b (set-piece/fouls/keeper/replay).**

## Self-Review

- **Spec coverage:** spec §4's 8 slices map 1:1 to Tasks 1–8 (FU264→Task 1 … FU271→Task 8). The five clusters of spec §1's table are covered: camera (T1/T2), set-piece (T3/T4), fouls/offside (T5), keeper (T6), replay (T7), plus the cross-cutting reachability map (T8).
- **Placeholders:** none — each task names its objective, exact addresses/targets, the evidence floor, the deliverable and the handoff. The Stage-2 T-list is deliberately parametrized (derives from the frozen slices; spec §5 allows the split).
- **Consistency:** slice numbers (W26-1…W26-8 ↔ FU264…FU271), draft paths and the promotion path are consistent; the cluster table matches spec §1; the live-chain rule (a ported producer raiser, not a fixture) is stated in Global Constraints and repeated per task.
- **Deferred:** the Stage-2 producer plan(s) are the next artifact; the reachability map (FU271) is the sizing input (spec §5's split decision).

## Handoff

Hand this plan to `superpowers:subagent-driven-development`: dispatch a fresh
read-only subagent per task (all 8 in parallel), then the controller freeze-reviews
and promotes each draft to a canonical `docs/ghidra/FU2xx_*.md` in a serialized
commit per slice.
