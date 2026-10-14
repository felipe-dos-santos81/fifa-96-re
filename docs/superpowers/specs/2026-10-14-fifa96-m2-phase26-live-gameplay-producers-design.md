# FIFA 96 M2 phase-26 — the live-gameplay producers

Date: 2026-10-14
Status: approved for planning (design approved in-chat: theme "live gameplay
producers", **both acceptance layers** chosen, **all five cluster families** in
one phase).
Parent spec: `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`
Predecessor spec: `docs/superpowers/specs/2026-10-14-fifa96-m2-phase25-census-closeout-design.md`
Predecessor plan: `docs/superpowers/plans/2026-10-14-fifa96-m2-phase25-census-closeout.md`
Recon slices (to be produced by this phase's wave): `docs/ghidra/FU264…FU271`.

## 1. Context

The match-action census is **closed at its honest ceiling** (`59/80 = 73%`, phase-25).
What remains is not dispatch count but **reachability**: the five phase-7 gameplay
chains are **fixture-proven** — they run when a test stages their gate — but **no
live engine path raises the gate**, so they never fire in the running match. This
is the last mile between "the rows are ported" and "the game does it".

The frontier, per `docs/ENGINE.md`'s known-gaps table (`:1750-1756`) and the leg
register, is five clusters:

| cluster | the missing **live producer** | the chain it un-blocks | legs |
|---|---|---|---|
| **camera / ball visibility** | the open-play camera-Y producer (`[0x157750]` off zero) + the view-object handler + the live place/pan reach | the follow-cam; the on-screen ball framing | FU168-L5, FU171-L4, R2-L1, R2-L4, T1-L2 |
| **set piece / restart** | the taker-arm producers (the throw setup, the timer+draw completion, the arm vector/RNG, the state cells) + the situation-0xB producers | corners/throw-ins/free-kicks/penalties fire live | FU-149 L13.1–L13.7, OL-84, OL-86 |
| **fouls / offside** | the `FU-150` entry producers (sit-9/0xA) + the extra-time flag | whistles/offside/free-kicks fire live | FU-150, OL-85 (FU207-L2), FU208-L1 |
| **keeper restart** | the `FU-151` rows `1D`/`1E` live producers (the stage ladder, `0x7546E`/`0x75B58`/`0x76072`) | keeper restarts fire live | FU-151 |
| **replay / overlay** | the `FU-152` producers (legs 1/2/3/7; the rows are zero-gated) | replay/overlay/sub-strip fire live | FU-152 |

### Baseline (verified first-hand at spec time, HEAD `e619c85`)

- `make check` **110/110** (sanitized, strict).
- Wired **59/80 = 73%** (`36 action + 23 phase`) — the census is closed; phase-26
  does **not** chase it.
- M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (691 lines, immutable).
- M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (**v22, 6522 lines**).
- README title `73%`; `AGENTS.md:90` `59/80 → 73%`.

### The two-layer acceptance (approved)

- **Layer 1 — engine-reach.** Each chain's gate cell is raised by a **live** engine
  path (a ported producer fired by the frame loop / a granted-frame scenario), proven
  **headlessly** (an ISO/granted-frame test reaches the chain), even when the result
  is not visible on screen.
- **Layer 2 — visible.** At least **one** chain is visible in the running smoke;
  the **follow-cam** is the anchor (the most-referenced visible leg).

## 2. Goal and non-goals

**Goal.** Port the live **producers** that raise each phase-7 chain's gate, so the
chains dispatch through a live engine path (Layer 1, proven headless), and drive
≥1 **visible** acceptance (the follow-cam, Layer 2) — with M1 immutable and M2
byte-identical unless a producer genuinely changes a live path.

**Definition of done.** Each cluster is either (a) **live** — a ported producer
raises its gate on a live path, proven by a headless granted-frame test (and, for
the camera, a visible smoke), or (b) an **honest numbered leg** — the producer is
unported/unreachable, with a named owner and a proven reason.

### Non-goals (YAGNI)

- **No census chase.** The match-action census is closed; a NULL row that a live
  producer happens to wire is honest upside, never a target.
- **No front-end/season/audio work**; no new subsystem beyond the live producers.
- **No faked producers.** A cell raised by a stand-in rather than the native
  producer is a leg, not a wire (R15/R45/R49).
- **No forced re-pin.** M2 moves only on a written reason + frame diff; otherwise
  the producer is kept tape-dormant.

## 3. Architecture (the live-chain promotion model)

```
   a FIXTURE-PROVEN chain  (a test stages the gate)
              │
              ▼
   its gate cell has NO live producer   (the phase-7 legs)
              │
              ▼
   port the native producer  ──►  a LIVE engine path raises the cell
              │                              │
              │                     headless granted-frame proof  ── Layer 1
              │                              └── the camera: the smoke SHOWS the follow  ── Layer 2
              ▼
   no live producer exists (unported / un-dispatched)  ⇒  NUMBERED LEG (owner named)
```

The phase's outcome is a set of **ported live producers** (each with a headless
reach test + a mutation-RED rider), one **visible** follow-cam acceptance, and an
updated **leg register**.

## 4. The 8 parallel read-only recon slices

Dispatched after spec approval, before the implementation plan (the phase-21…25
recon-ahead precedent). Each is a **read-only** Ghidra MCP investigation → one FU
slice; **no code, no rename/comment/label/function/script/project save**.
First-hand bytes; fresh `get_xrefs_to` for reachability; the alias traps checked
(`dword[x]>>16` = word at `x+2`; `>>24` = byte at `x+3`; verify CALL bytes;
watch sign/width, inverted gates, gate-index misreads).

| # | Slice | Investigates | Deliverable |
|---|---|---|---|
| 1 | **FU264** | The open-play camera-Y producer (`FU168-L5`: what raises `[0x157750]` off zero in open play; the keeper release dispatch-death R11-L4) + the view-object handler (`R2-L1`) | The camera-Y port shape + the visible-follow verdict |
| 2 | **FU265** | The live place/pan reachability (`R2-L4`: which live event fires the pan) + the row-07 `+0x44` ack (`T1-L2`) + the on-screen ball framing | The place/pan + ball-framing port shapes |
| 3 | **FU266** | The taker-arm residuals (`FU-149 L13.1–L13.7`: the throw setup, the timer+draw completion, the arm vector/RNG, the `0x974DC(0x1E)`/`0x8F188` sinks, the `0x8DE8C` picks, the `[0x157821]`/`[0x157A6A]` state cells, the penalty) | The taker-arm live port shapes |
| 4 | **FU267** | The situation-0xB producers (`OL-84`: row 02, rows `0x10..0x13`, the keeper/restart bodies, the computed act-8 call) + row-01's sinks | The situation-producer port shapes |
| 5 | **FU268** | The fouls/offside live producers (`FU-150`: the sit-9/0xA entries) + the extra-time flag `OL-85` (`FU207-L2`, the `0x389A4` front-end producer) + `FU208-L1` | The foul/offside + extra-time port shapes |
| 6 | **FU269** | The keeper restart rows `1D`/`1E` live (`FU-151`: the stage ladder, `0x7546E`/`0x75B58`/`0x76072`) + the post-period hold `OL-86` | The keeper-restart port shapes |
| 7 | **FU270** | The replay/overlay/sub-strip producers (`FU-152` legs 1/2/3/7; the zero-gated rows) | The replay/overlay port shapes |
| 8 | **FU271** | The **live-trigger census** (cross-cutting): for each gating cell, the live engine path that raises it; the interactive↔headless reachability map (which chains are smoke-reachable vs granted-frame-only) | The reachability map + the per-chain live verdict |

Each slice delivers: (a) the exact native producer with first-hand bytes, (b) a
fresh live-path/reachability census, (c) an explicit **live-vs-leg ruling** with its
**payoff (Layer 1 / Layer 2)**, (d) numbered open legs with native owners.

## 5. Implementation task structure (Stage 2, serialized SDD)

Authored **after** all 8 drafts are frozen and promoted. One implementer at a time;
dependency order T1 → T2 → … → Tn.

- **T1 — determinism + reachability gate.** Recompute the census; prove M1/M2
  byte-stability (≥3 replays); build the **reachability map** from FU271 (which
  chains a granted-frame run can drive). No re-pin, no port.
- **T2..Tk — one live producer per chain**, ordered by the wave's payoff ×
  tractability: each lands the native producer + a **headless granted-frame reach
  test** + a mutation-RED rider.
- **T(visible) — the follow-cam visible acceptance:** drive the camera chain on the
  smoke (or an honest written waiver if the chain is granted-frame-only).
- **T(k+2) — the census/leg-register close-out:** recompute the census iff a live
  producer wires a NULL row (four-doc same-commit rule); append the resolved/forbidden
  legs (quoted errata).

**Size risk.** Five clusters is a large phase; if the frozen slices show the
Stage-2 plan exceeds a tractable task count, the controller **splits it into
phase-26a (the camera/ball visibility, including the visible anchor) and phase-26b
(the set-piece/fouls/keeper/replay chains)** — two plans off one spec, the phase-23
precedent. The spec stays whole; the plan may split.

## 6. Acceptance and the tape/census terms

- `make check` green (sanitized, strict).
- Each live chain is proven by a **headless granted-frame test** (Layer 1) + a
  mutation-RED rider; each ruled-out chain carries a numbered leg.
- **≥1 visible acceptance:** the follow-cam on the running smoke (Layer 2), or an
  explicit honest waiver if the camera chain is granted-frame-only.
- **M1 `09b726b7…` never moves.**
- **M2:** byte-identical unless a producer genuinely changes a live path (written
  reason + frame diff). The phase prefers tape-dormant producers.
- The census is recomputed from what actually wires (it may hold at 59/80).
- An honest `make game` smoke (`DISPLAY=:1`) records the live/visible reach.

## 7. Risks

1. **The visible chains may be interactively unreachable** (no scripted live goal /
   set piece in the smoke window — the earlier "no live goal segment" smokes). The
   follow-cam is the best candidate; the rest may be granted-frame-only (Layer 1).
2. **The scope is very large** (five families) → the §5 split into phase-26a/26b may
   be needed.
3. **A live producer feeding a live row could move the tape** → T1's gate + the
   per-task tests + the M2 `cmp` own this.
4. **Several producers are front-end-produced** (OL-85's `0x389A4`) → out of the
   engine's reach; those become honest legs (no front-end work, §2).

## 8. Constraints carried in

- M1 `09b726b7…` immutable; the null backend is the determinism source of truth.
- Ghidra stays **read-only**; `/FIFA96.EXE` case-sensitive; LE-image globals at
  `+0x100000`; scripts disabled.
- No fakes: every unreachable native is a numbered leg with its native owner.
- ISO-gated tests skip cleanly without the ISO.
- Commits `feat`/`fix`/`docs(fuNNN)`/`test`/`chore`; work on `main`; push per
  dispatch. Recon drafts in `.superpowers/sdd/<phase>/` (git-ignored); the controller
  promotes them to `docs/ghidra/FU264…FU271` in serialized commits.

## 9. Interfaces

- **The phase-7 chains** — the set-piece taker rows `0x10..0x13`, the foul/referee
  entries, the keeper rows `1D`/`1E`, the replay/overlay rows, the camera/ball
  producers.
- **The gating cells** — `[0x157750]` (camera Y), `[0x1577CA]` (tracked),
  `[0x157AC0]` (extra-time), `[0x157821]`/`[0x157A6A]` (taker state), the
  `ref_machine`/`replay.state`/`overlay.armed` cells.
- **The engine seams** — `fifa96_match_run_frame`, `fifa96_match_run_situation`,
  the camera/follow consumers, the taker/foul/keeper machines.
- **The recon slices** `docs/ghidra/FU264…FU271` — appended errata only.

## 10. Decisions (approved in-chat)

- **D1 — both layers.** Engine-reach (headless proof) for every live chain, plus
  ≥1 visible acceptance (the follow-cam).
- **D2 — all five clusters, one phase.** The camera, set-piece, fouls/offside,
  keeper and replay families are all in scope; the Stage-2 plan may split into
  phase-26a/26b if the frozen slices show it is too large (§5).
- **D3 — no census chase.** The census is closed at 59/80; a live producer that
  wires a NULL row is honest upside only.
- **D4 — read-only recon, serialized promotion.** The 8 slices run in parallel
  read-only; the controller freeze-reviews and promotes each to a canonical
  `docs/ghidra/FU2xx_*.md`.
- **D5 — M2 preserved.** Producers land tape-dormant; only a genuine live-path change
  triggers an honest re-pin (written reason + frame diff).
- **Tooling note.** Ghidra MCP is the recon tool; **context7 is not applicable**
  (library docs, not RE). REA MCP is available for a provider-neutral second read.
