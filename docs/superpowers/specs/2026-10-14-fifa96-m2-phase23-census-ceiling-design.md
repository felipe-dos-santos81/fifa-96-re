# FIFA 96 M2 phase-23 — the census ceiling (raise the honest action-dispatch frontier)

Date: 2026-10-14
Status: approved for planning (design approved in-chat: theme "raise the honest
census ceiling", both fronts chosen by the user).
Parent spec: `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`
Predecessor spec: `docs/superpowers/specs/2026-10-13-fifa96-m2-phase22-end-spine-design.md`
Predecessor plan: `docs/superpowers/plans/2026-10-13-fifa96-m2-phase22-end-spine.md`
Recon slices (to be produced by this phase's wave): `docs/ghidra/FU240…FU247`.

## 1. Context

Phase-22 closed the end spine (extra time → match over → front-end return) and
re-pinned the M2 tape `v21 → v22` (6522 lines), wiring **no** new dispatch row.
The action-dispatch census has stood at **55/80 = 68%** (`33 action + 22 phase`;
dispatch `24 UNSUP / 55 OK / 1 NOTF`, the `0x16` INT3 slot the NOTF) since
phase-20.

The census frontier was audited last in **FU-222** (the phase-20 wave R70), which
established the **honest ceiling** rule and the phase-20 result:

- A row counts only when a **ported path installs AND dispatches** — a `NULL → fn`
  flip with no live producer is **not** counted (the rows-`27`/`29`/`2C`
  precedent; AGENTS.md §Completion percentage).
- The phase-20 honest ceiling was **55/80 = 68%**, reached by registering phase
  `0x10` (FU214-L4) and porting the actor-cell producer for action `0x20`
  (FU210-L3). Both landed in phase-20; the current count *is* that ceiling.
- The wider bound was **58/80 = 72%** via phase `0x13`/`0x14` (FU192-L2, `+2`)
  and **56/80 = 70%** via phase `0x1E` (FU212-L1, `+1`, only by lifting a
  deliberate pin).

So the frontier is **not** an "unported bodies" problem — almost every NULL row
already has a **ported body**. It is a **producer/installer reachability**
problem: each NULL row is blocked by a specific unported, gated, tape-visible, or
provably-absent installer. Phase-23 raises the ceiling honestly by (A) landing the
attainable movers and (B) resolving the installer frontier row-by-row — wire or
freeze with a fresh census.

### Baseline (verified first-hand at spec time, HEAD `4f7ca385…`)

- `make check` **110/110** (sanitized, strict).
- Wired **55/80 = 68%** (`33 action + 22 phase`); dispatch `24 UNSUP / 55 OK / 1 NOTF`.
  The test pins are `tests/test_engine_m2.c:3666` and `:4041`
  (`action_wired == 33u && phase_wired == 22u`).
- M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (691 lines, immutable).
- M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (**6522 lines**, v22).
- README title `68%`; `AGENTS.md:90` `55/80 → 68%`.

### The 24 NULL rows and their named blockers (frozen per-row evidence)

The engine slot comments (`src/fifa96_engine/fifa96_match_handlers.c:5097..5259`)
carry the authoritative per-row evidence. Enumerated at the 55/80 baseline:

**Action NULL (12):** `03 0A 0D 15 17 1A 22 25 27 29 2B 2C`.

| row | blocker | owner leg |
|---|---|---|
| `03` | ported body; the live phase-2 `[rec+0x1C] = 0x6DCC8` target-rewrite chain is **tape-visible** (wiring moved M2 at frame 29) | FU177-L1 |
| `0A` | not ported; installer `0x7CDD8` has **no xrefs**; head mis-decoded stub | OL-14 / FU-137 §6 |
| `0D` | ported body; only installer is row-09 `0x80F61`, inside the **unported row-09 arm 1** | FU178-L1 |
| `15` | not ported; head mis-decoded stub | OL-14 / FU-137 §6 |
| `17` | ported body; only installer is act-4 stage 0 `0x89740`, behind the sit-2..4 formation-order arm that is a **permutation residual = 0** | FU201-L1 / FU181-L1 |
| `1A` | ported body; only installer is the **unported `FUN_00077EAC` CPU ladder** | FU186-L5 |
| `22` | ported body; only installer is row-07 `0x816DF` invoke-now, **structurally ungated** (OL-65 angle inputs staged 0) | FU185-L6 / OL-65 |
| `25` | **no installer exists** (exhaustive literal scan); body not ported | FU189-L2 |
| `27` | ported body; the FU-142f census of all 77 `0x7D9A4` sites finds **no `0x27` invocation** | FU-142f / OL-48 |
| `29` | ported body; **no `0x29` invocation** | FU-142f / OL-48 |
| `2B` | **dead entry** (native `0x87738` is the shared row-29 epilogue RET) | FU-142f / OL-15 |
| `2C` | ported body; **no `0x2C` invocation** | FU-142f / OL-48 |

**Phase NULL (12):** `01 0B 0D 0E 11 12 13 14 1A 1B 1C 1E`.

| row | blocker | owner leg |
|---|---|---|
| `01` | native `0x6E1D0` already realized by the seeding path — **double-wire excluded** | FU192-L7 |
| `0B` | shared reset; sole writer sits inside the **unwired row `0x1A` body** (the FU244 slice disambiguates action-row vs phase-row `0x1A`) | FU-206 §4 #12 |
| `0D` | shared reset; sole argument is situation 7, and **no live caller dispatches situation 7** | FU-146 |
| `0E` | shared reset; **no live writer** | FU-191 §10 |
| `11` | shim identical to rows 01/12 — **double-wire excluded** | FU192-L6/L7 |
| `12` | native `0x6E1D0` + **never written** — excluded | FU192-L6/L7 |
| `13` | ported body `0x6E244`; consumes PRNG `0x92AC8` and reads `0x10F364`/`0x10F368`, whose producer is the **unported `FUN_0008D098` case-`0x13/0x14`** scatter | FU192-L2 — **A-mover (+1)** |
| `14` | same body as `13`; **on-tape 40 frames** (tape-visibility risk) | FU192-L2 — **A-mover (+1)** |
| `1A` | ported body (act-4 formation-order walk); **no act-4 producer** (permutation residual = 0) | FU201-L1 / FU193-L3 |
| `1B` | ported body (act-5 pure reset); a fresh 11-xref `FUN_000888FC` census finds **no `EAX=5` invoker** | FU195-L1 |
| `1C` | ported body (act-6 pre-restart timeline); **no act-6 producer** (the offside machine would double-run) | FU193-L4 |
| `1E` | ported body (act-8 restart sweep); deliberately pinned to the **compressed model** — de-compression is FU212-L1 | FU212-L1 — **A-mover (+1, pin-lift)** |

## 2. Goal and non-goals

**Goal.** Raise the honest action-dispatch census above **55/80 = 68%**, and
convert the unattainable frontier from assertion into proof. Two fronts:

- **Front A — the attainable movers.** Land phase `0x13`/`0x14` (`+2`, via the
  `FUN_0008D098` case-`0x13/0x14` scatter producer, FU192-L2) and phase `0x1E`
  (`+1`, via an honest lift of the compressed-act-8 pin, FU212-L1), subject to
  the A-mover tape rulings.
- **Front B — the installer frontier.** Resolve every unresolved NULL row's
  installer chain, first-hand: each either becomes wireable (a newly-located /
  newly-ported installer) or is **frozen as a permanent ceiling leg** with a fresh
  census. This front proves the ceiling whether or not it adds rows.

The phase's outcome is a **raised, proven ceiling** — a floor of **58/80 = 72%**
if both A-movers land, plus whatever the B-front resolution yields — plus the
**8 frozen recon slices** `FU240…FU247` that the Stage-2 producer plan consumes.

### Non-goals (YAGNI)

- **No front-end/competition work** (FU-65); no audio (FU-229); no new subsystem.
- **No faked producers.** No `NULL → fn` flip without a live installer+dispatch
  (AGENTS.md census rule; the rows-`27`/`29`/`2C` precedent). A row that cannot
  be honestly wired is *frozen*, never counted.
- **No double-wire.** The phase `01`/`11`/`12` exclusions stand (R10).
- **No unrelated refactor**; no golden re-pin unless a tape-visible mover forces
  one (then: written reason + frame diff).

## 3. The honest-ceiling model (architecture and data flow)

```
        the native tables (fresh)
        0x1106E0[45] (actions)   0x110794[35] (phases)
                 │                       │
                 ▼                       ▼
        the installer mechanism  FUN_0007D9A4(code, arg0, arg1, arg2)
        +  the [rec+0x1C] indirect dispatch / FUN_000888FC act invoker
                 │
                 ▼
        per NULL row: is there a reachable install+dispatch path?
                 ├─ YES (ported)  → the row is an A/B mover → wire
                 ├─ YES (unported) → port the installer body → wire
                 └─ NO             → freeze: a numbered ceiling leg
                 ▼
        the census numerator grows only on wire; the ceiling is published
```

**A-movers.** phase `0x13`/`0x14` need the `FUN_0008D098` case-`0x13/0x14`
PRNG-scatter producer (`0x10F364`/`0x10F368`); the body `0x6E244` is ported.
phase `0x1E` needs the compressed-act-8 pin lifted, which is honest **iff** the
faithful 22-record sweep is proven render-invisible and dormant (FU-212 §6
argues it is; FU241 must re-derive it first-hand).

**B-front.** Each installer chain is one of: unported body (`FUN_00077EAC` ladder
for action `1A`; row-09 arm 1 for action `0D`), gated-ungated (`0x816DF` for
action `22`; the permutation residual for action `17`), tape-visible (action
`03`), or provably absent (action `25`; the FU-142f no-invocation rows
`27`/`29`/`2C`; the dead `2B`). The phase re-derives each fresh and rules.

## 4. The 8 parallel read-only recon slices

Dispatched after spec approval, before the implementation plan is written (the
phase-21/22 recon-ahead precedent). Each is a **read-only** Ghidra MCP
investigation producing one FU slice; **no code, no rename/comment/label/function/
script/project save**. First-hand bytes and addresses; fresh `get_xrefs_to` for
any reachability claim; the alias traps checked (`dword[x]>>16` = word at `x+2`;
`>>24` = byte at `x+3`; verify CALL bytes; watch sign/width, inverted gates).

| # | Slice | Front | Investigates | Deliverable |
|---|---|---|---|---|
| 1 | **FU240** | A | The phase-`0x13`/`0x14` scatter producer: `FUN_0008D098` case-`0x13/0x14`, the `0x10F364`/`0x10F368` producer chain, the `0x92AC8` PRNG consumption in `0x6E244`, and the tape-inertness (phase-`0x14` is on-tape 40 frames) | The port shape for **+2** + the tape ruling |
| 2 | **FU241** | A | The phase-`0x1E` restart sweep (`0x8A798`): the render-invisibility + dormancy argument and the exact terms of the compressed-model pin (`test_phase_1E_compressed_model_pin`) | The honest **+1** pin-lift terms |
| 3 | **FU242** | B (anchor) | The installer mechanism: `FUN_0007D9A4`'s code-argument forms (immediate vs register-derived), the `[rec+0x1C]` indirect dispatch, the `FUN_000888FC` act invoker, and a fresh per-row installer verdict for **all 24 NULL rows** | The definitive frontier map + the fresh census machinery |
| 4 | **FU243** | B | The no-installer / dead cluster: action `27`/`29`/`2C` (FU-142f no-invocation), `2B` (dead entry), `25` (exhaustive scan), `0A`/`15` (xref-less installer stubs) — independent re-derivation | Confirm/refute the census; freeze or expose |
| 5 | **FU244** | B | Action `1A`'s installer (the `FUN_00077EAC` CPU ladder `0x78227`/`0x7824F`/`0x78277` + the `FUN_0008D8EC → FUN_000782D0` pre-pass) **and** the coupled phase-`0B` writer (owning row `0x1A` disambiguated here) | The **+2 cluster** verdict |
| 6 | **FU245** | B | Action `0D`'s installer (row-09 `0x80F61` inside the unported row-09 arm 1) and the row-09 arm-1 reachability; plus the phase-`0D`/`0E` writer chains | The "installer inside an unported body" verdict |
| 7 | **FU246** | B | Action `03` (the tape-visible `[rec+0x1C] = 0x6DCC8` rewrite chain) + action `17` (the sit-2..4 permutation residual) + the phase act-4/5/6 producers `1A`/`1B`/`1C` | The blocked-installer + no-producer verdicts |
| 8 | **FU247** | B′ | Action `22`/OL-65 (the ungated row-07 `0x816DF` invoke inputs) + the phase double-wire trio `01`/`11`/`12` + the tape/re-pin term inventory | The permanent-exclusion register |

Each slice's deliverable carries: (a) the exact body/mechanism with first-hand
bytes, (b) a fresh reachability verdict, (c) an explicit **wire-vs-ceiling
ruling**, (d) numbered open legs with their native owners.

## 5. Implementation task structure (Stage 2, serialized SDD)

Authored **after** all 8 drafts are frozen and promoted (the phase-21/22 shape).
One implementer at a time; dependency order T1 → T2 → … → Tn.

- **T1 — determinism + feasibility gate.** Recompute the census against the
  native tables; re-derive the honest ceiling from `FU240…FU247`; prove the
  M1/M2 replay byte-stability (≥3 replays). **No re-pin, no wire.** If not
  byte-stable → STOP; fallback = a fixture-only proof.
- **T2..Tk — one producer/registration per proven-attainable row**, ordered by
  the wave's tractability ranking: each lands the installer/producer with a
  per-row RED/GREEN dispatch test and a **mutation-RED** rider (disable the
  guarded code → the assert aborts). Includes the A-movers (phase `0x13`/`0x14`,
  phase `0x1E`) and whichever B-rows the wave proves attainably wireable.
- **T(k+1) — the tape touch.** If a mover is tape-visible (action `03`, or phase
  `0x14`'s 40 on-tape frames), apply the re-pin policy (additive where possible;
  written reason + frame diff otherwise). Otherwise the tape stays byte-exact
  and the movers are asserted off-tape on a fixture.
- **T(k+2) — the census close-out.** Recompute `floor(100 × wired/80)`; update
  the README title + `docs/ENGINE.md` + `docs/PROGRESS.md` + `AGENTS.md` +
  the `tests/test_engine_m2.c` census pins (`:3666`/`:4041`) **in the same
  commit**; record the honest smoke.

The exact T-list and count are **not fixed here** — they are derived from the
frozen slices (a slice may rule a row permanent, in which case it is a leg, not a
task).

## 6. Acceptance and the census / tape terms

- `make check` green (sanitized, strict).
- Every newly-wired row is asserted at the **dispatch layer** with a
  mutation-RED rider; every frozen row carries a numbered ceiling leg with a
  fresh census.
- The census recompute `floor(100 × wired / 80)` is derived, never hand-set; the
  README title, `docs/ENGINE.md`, `docs/PROGRESS.md`, `AGENTS.md` and the test
  pins are consistent in the same commit.
- **M1 `09b726b7…` never moves.**
- **Tape:** byte-exact unless a tape-visible mover forces a re-pin; then written
  reason + frame diff. Any *unexplained* drift is a regression to investigate,
  never a re-pin.
- An honest `make game` smoke (`DISPLAY=:1`) records the live reach (expected: no
  visible change — the movers are tape-dormant or render-invisible).

**Census caveat (stated, not hidden).** The phase's guaranteed gain is the
A-front (**+3 → 58/80 = 72%**); the B-front may add rows or may resolve entirely
to ceiling legs. The census is recomputed from what actually wires.

## 7. Risks

1. **The B-front resolves to *fewer* wires than hoped.** Then the phase's value is
   the *proven* ceiling, not the count. Stated, not hidden.
2. **Action `03` and phase `0x14` are tape-visible.** Wiring them risks an M2
   re-pin; the T1 gate + FU243/FU240 own the verdict.
3. **The installer mechanism may have an unmodeled dispatch** (e.g. a
   register-derived code the static census missed). FU242 owns this risk — it is
   the cross-cutting anchor and must re-derive the mechanism, not trust FU-142f.
4. **The `+2` cluster (action `1A` + phase `0B`)** depends on the unported
   `FUN_00077EAC` ladder; FU244 owns whether it is honestly portable.
5. **Only the A-movers are guaranteed.** Everything else is upside.

## 8. Constraints carried in

- M1 `09b726b7…` immutable; the null backend is the determinism source of truth.
- Ghidra stays **read-only**; `/FIFA96.EXE` case-sensitive; LE-image globals at
  `+0x100000`; scripts disabled.
- No fakes: every unreachable native is a numbered leg with its native owner.
- ISO-gated tests skip cleanly without the ISO.
- Commits `feat` / `fix` / `docs(fuNNN)` / `test` / `chore`; work on `main`; push
  per dispatch. The recon drafts live in `.superpowers/sdd/<phase>/` (git-ignored);
  the controller promotes them to `docs/ghidra/FU240…FU247` in serialized commits.

## 9. Interfaces

- **The native tables** `0x1106E0[45]` / `0x110794[35]` — re-read fresh (no change
  expected; FU242/FU-222 audit).
- **The installer mechanism** `FUN_0007D9A4` + the `[rec+0x1C]` indirect dispatch
  + `FUN_000888FC` — the frontier's hinge (FU242).
- **The A-movers** — the `FUN_0008D098` case-`0x13/0x14` producer (FU240) and the
  phase-`0x1E` de-compression (FU241).
- **The census pins** `tests/test_engine_m2.c:3666`/`:4041` — updated in the
  close-out commit.
- **The recon slices** `docs/ghidra/FU240…FU247` — appended errata only (prior
  sections never rewritten).

## 10. Decisions (approved in-chat)

- **D1 — both fronts.** The phase raises the ceiling via the attainable movers
  **and** the installer-frontier resolution (not either alone).
- **D2 — wire-vs-freeze per row.** Every NULL row ends the wave as either a wire
  or a numbered permanent ceiling leg; no row is left ambiguous.
- **D3 — read-only recon, serialized promotion.** The 8 slices run in parallel
  read-only; the controller freeze-reviews and promotes each to a canonical
  `docs/ghidra/FU2xx_*.md`.
- **D4 — no faked producers.** The AGENTS.md census rule binds; the rows-27/29/2C
  precedent governs.
- **Tooling note.** Ghidra MCP is the recon tool; **context7 is not applicable**
  (library docs, not RE). REA MCP is available for a provider-neutral second read
  if a slice needs one.
