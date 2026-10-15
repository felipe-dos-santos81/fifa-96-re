# FIFA 96 M2 phase-26b — the remaining phase-7 chains + R11-L4

Date: 2026-10-14
Status: approved for planning (design approved in-chat).
Parent spec: `docs/superpowers/specs/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-design.md`
  (phase-26b is the second plan off that **whole** spec — its §5 split clause; D2 "all five clusters, one phase").
Sibling plan (closed): `docs/superpowers/plans/2026-10-14-fifa96-m2-phase26a-camera-ball-visibility.md`
  (closed as an **honest Layer-2 waiver** — the camera's live producer is dispatch-dead behind R11-L4; FU272).
Recon slices (produced by this phase's wave): `docs/ghidra/FU273…FU280`.

## 1. Context

Phase-26a closed with **no code** (FU272): the follow/pan's only sound producer — the keeper stage-5
release (`camY := 0x50`) — is **dispatch-dead behind R11-L4**, which FU272 ruled **phase-scale**.
Phase-26b therefore owns both the **four remaining chain families** the phase-26 spec enumerated and
**R11-L4 itself** — the leg that gates the camera/pan/follow **and** the keeper restarts.

What is already frozen (the *what*, not the *how*):

| family | the missing live producer (frozen) | slice |
|---|---|---|
| set-piece / restart (rows `0x10`/`0x12`/`0x13`) | the throw-in scanner + the foul adjudicator that dispatch the taker rows | FU266, FU267 |
| fouls / offside | the live contact-gate invoker + the offside receive arm (`FUN_0007A084 0x7A448`) + `field_4c2f2` | FU268 |
| keeper restart (`1D`/`1E`) | a live `goal_armed`/goal-kick producer (the keeper CPU ladder) | FU269, FU272 |
| replay / overlay / sub | a live caller for `FUN_000510DC`/`FUN_00054104` from the period/screen machine | FU270 |
| **R11-L4** | the whole keeper-AI machine (`FUN_0008D8EC→FUN_000782D0→FUN_00077EAC`, rows `1A`/`1B`, phase-6/`0x1F`, the claim bridge) | FU272 |

Phase-26b needs the **port shapes** — a fresh read-only recon wave, then one plan.

### Baseline (verified first-hand at spec time, HEAD `a1cdc8b`)

- `make check` **110/110** (sanitized, strict).
- Wired **59/80 = 73%** (`36 action + 23 phase`) — the census is closed; 26b does **not** chase it.
- M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (691 lines, immutable).
- M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (v22, 6522 lines).
- README title `73%`; `AGENTS.md:90` `59/80 → 73%`.

## 2. Goal and non-goals

**Goal.** Port the live **producers** for the four remaining phase-7 families **and** the R11-L4 keeper
CPU chain, so each chain dispatches through a live engine path (Layer 1, proven headless), and drive
≥1 **visible** acceptance (the follow-cam) if R11-L4 closes — with M1 immutable and M2 byte-identical
unless a producer genuinely changes a live path.

**Definition of done.** Each chain is either (a) **live** — a ported producer raises its gate on a live
path, proven by a headless granted-frame test (+ a mutation-RED rider), or (b) an **honest numbered leg**
with a named native owner and a proven reason. R11-L4 is **live** or a numbered leg routed onward.

### Non-goals (YAGNI)

- **No census chase.** The census is closed; a NULL row a live producer happens to wire is upside only.
- **No front-end/season/audio work** (§2 of the phase-26 spec); front-end producers (`0x389A4`) stay legs.
- **No faked producers.** A cell raised by a stand-in is a leg, not a wire (R15/R45/R49).
- **No forced re-pin.** M2 moves only on a written reason + frame diff; producers land tape-dormant.

## 3. The 8 parallel read-only recon slices

Dispatched **flat** (all 8 in parallel), read-only Ghidra MCP, after spec approval; the phase-21…26
recon-ahead precedent. First-hand bytes; fresh `get_xrefs_to` for reachability; the alias traps checked
(`dword[x]>>16` = word at `x+2`; `>>24` = byte at `x+3`; verify CALL bytes; sign/width, inverted gates,
gate-index misreads). No code, no rename/comment/label/function/script/project save.

| # | Slice | Investigates | Deliverable |
|---|---|---|---|
| 1 | **FU273** | **R11-L4** — the keeper CPU chain `FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC`; rows `1A`/`1B` body remainders; the phase-6/row-`0x1F` chain; the claim bridge (R11-L1/L2/L3) | The R11-L4 **port shape + an explicit go/no-go** + a machine inventory + an effort estimate |
| 2 | **FU274** | The set-piece/restart **live arm** — the throw-in scanner + the foul adjudicator that dispatch rows `0x10`/`0x12`/`0x13`; the taker state cells (`[0x157821]`/`[0x157A6A]`); the `goal_armed`→phase 3/4/6/7 producers | The set-piece live-arm port shapes |
| 3 | **FU275** | The **taker-arm residuals** (`FU266 L266-1..14`) — the per-phase triple `FUN_00083164`, the pick `FUN_0008DB6C`, the timer+draw `FUN_00083428`, the sinks `0x974DC(0x1E)`/`0x8F188` | The taker-arm port shapes |
| 4 | **FU276** | **Fouls/offside** — the live contact-gate invoker (the row-`0x04` duel / the pad `want 0x60` handler); the offside receive arm `FUN_0007A084 0x7A448`; `field_4c2f2`; the `FU268-L10` byte divergence (native `code`/`+0x91` vs engine `type`/`+0x8E`) | The foul/offside port shapes + the L10 reconciliation |
| 5 | **FU277** | The **keeper-restart trigger** — the live `goal_armed`/goal-kick producer for rows `1D`/`1E`; the sit-5 emit-route divergence (`FU269-L5`); the keeper ladder downstream (`FU269-L1/L2`) | The keeper-restart port shapes |
| 6 | **FU278** | **Replay/overlay/sub** — the period/screen-machine caller for `FUN_000510DC`/`FUN_00054104`/`FUN_000515C8`; the substitution-event writer (`0x8A938` case 6/8); the `ball_row` no-producer verdict | The replay/overlay port shapes |
| 7 | **FU279** | The **situation-0xB producers + the act-8 tail** — FU267's six dormant producers; the row-01 sinks (`0x974DC(0x1E)`/`0x8F188`/`0x8DCD4`); the `FU241-L1/L3` residues | The situation-producer + sink port shapes |
| 8 | **FU280** | The **live-trigger census (re-cut)** — for each chain, the live engine path that would raise it; the interactive↔headless map; the per-chain live verdict + the **visible-anchor verdict** (is the follow-cam reachable once R11-L4 lands?) | The reachability map + the per-chain verdict |

Each slice delivers: (a) the exact native producer with first-hand bytes, (b) a fresh live-path
reachability census, (c) an explicit **live-vs-leg ruling** with its payoff (Layer 1 / Layer 2), (d)
numbered open legs with native owners. Slices run in parallel read-only; the controller freeze-reviews
and promotes each to a canonical `docs/ghidra/FU2xx_*.md` in serialized commits.

## 4. Implementation task structure (Stage 2, serialized SDD)

Authored **after** all 8 drafts are frozen and promoted. One implementer at a time; dependency order.

- **T1 — determinism + reachability gate.** Recompute the census; prove M1/M2 byte-stability (≥3
  replays); build the reachability map from FU280. No re-pin, no port.
- **T2 — R11-L4** (the critical path): the keeper CPU chain — the installer machines, rows `1A`/`1B`
  remainders, phase-6/`0x1F`, the claim bridge — landed so the keeper stage-5 release raises `camY`
  **live**. If FU273 rules it beyond one phase, 26b stops honestly (§6).
- **T3..T6 — one live producer per chain**, ordered by the wave's payoff × tractability: set-piece/
  restart, fouls/offside, keeper restarts, replay/overlay. Each lands the native producer + a **headless
  granted-frame reach test** + a mutation-RED rider.
- **T(visible) — the follow-cam visible acceptance** (on the smoke, `DISPLAY=:1`) once R11-L4 + the
  camera producer are live — or an honest written waiver.
- **T(last) — the census/leg-register close-out:** recompute the census iff a live producer wires a
  NULL row (four-doc same-commit rule); append the resolved/forbidden legs (quoted errata).

**Size risk.** Five families + R11-L4 is the largest phase yet; the phase-26 spec §5 pre-authorized a
re-split. The controller may split Stage-2 into 26b-i (the chains) and 26b-ii (R11-L4) off this spec
without a new spec — the phase-23 precedent.

## 5. Acceptance and the tape/census terms

- `make check` green (sanitized, strict).
- Each live chain is proven by a **headless granted-frame test** (Layer 1) + a mutation-RED rider; each
  ruled-out chain carries a numbered leg.
- **≥1 visible acceptance:** the follow-cam on the running smoke (Layer 2), or an explicit honest waiver.
- **M1 `09b726b7…` never moves.**
- **M2:** byte-identical unless a producer genuinely changes a live path (written reason + frame diff).
  The phase prefers tape-dormant producers.
- The census is recomputed from what actually wires (it may hold at 59/80).
- An honest `make game` smoke (`DISPLAY=:1`) records the live/visible reach.

## 6. The honest-stop rule

If FU273 rules R11-L4 beyond a single phase, phase-26b **stops honestly**: the four chain families land
where their live arms are reachable **without** R11-L4; R11-L4 + the camera/pan/follow become a numbered
leg with a named native owner (FU272's T2a-L1..L6) and are routed to phase-26c — **no faked producer**
(R15/R45/R49). The phase's plan carries this branch explicitly so Stage-2 never chases an unreachable
visible anchor.

## 7. Interfaces

- **The phase-7 chains** — the taker rows `0x10`/`0x12`/`0x13`, the foul/referee contact + offside
  entries, the keeper rows `1D`/`1E`, the replay/overlay/sub rows, the camera/ball producers.
- **The gating cells** — `[0x157750]` (camera Y), `[0x1577CA]` (tracked), `[0x157821]`/`[0x157A6A]`
  (taker state), `ref_machine`/`replay.state`/`overlay.armed`, `field_4c2f2`.
- **The engine seams** — `fifa96_match_run_frame`, `fifa96_match_run_situation`, the keeper/taker/
  foul/replay machines, the camera/follow consumers.
- **The frozen slices** `docs/ghidra/FU264…FU272` + this wave's `FU273…FU280` — appended errata only.

## 8. Decisions (approved in-chat)

- **D1 — everything remaining, one phase.** The four chain families **and** R11-L4 are all in 26b;
  the Stage-2 plan may split into 26b-i/26b-ii if the frozen slices show it too large (§4).
- **D2 — 8 flat read-only slices.** The wave fans out in parallel; the plan is authored after all 8
  are frozen and promoted (the phase-26 pattern).
- **D3 — silent on Layer 2 unless it is honest.** The visible follow-cam is attempted only if R11-L4
  closes; otherwise the phase records the honest waiver (§6). No faked producers.
- **D4 — plan off the whole phase-26 spec.** No new parent spec; this doc binds 26b's scope + the
  honest-stop rule only.
- **D5 — M2 preserved.** Producers land tape-dormant; only a genuine live-path change triggers an
  honest re-pin (written reason + frame diff).
- **Tooling note.** Ghidra MCP is the recon tool; **context7 is not applicable** (library docs, not RE).
  REA MCP is available for a provider-neutral second read.
