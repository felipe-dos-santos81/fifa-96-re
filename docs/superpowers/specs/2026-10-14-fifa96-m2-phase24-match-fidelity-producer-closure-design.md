# FIFA 96 M2 phase-24 — the match-fidelity producer closure

Date: 2026-10-14
Status: approved for planning (design approved in-chat: theme "match-fidelity
leg closure", the producer-closure wave chosen by the user).
Parent spec: `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`
Predecessor spec: `docs/superpowers/specs/2026-10-14-fifa96-m2-phase23-census-ceiling-design.md`
Predecessor plan: `docs/superpowers/plans/2026-10-14-fifa96-m2-phase23-census-ceiling.md`
Recon slices (to be produced by this phase's wave): `docs/ghidra/FU248…FU255`.

## 1. Context

Phase-23 raised the honest action-dispatch census to **58/80 = 72%** (`35 action
+ 23 phase`) by wiring the three proven-attainable rows (action `25`/`15`, phase
`0x1E`) and freezing the rest as **ceiling legs** (`FU240…FU247`). The census is
now at its honest ceiling: the remaining 22 NULL rows are all blocked by a
missing, unported, or deliberately-excluded producer. **Further census work by
row-flipping is exhausted.**

What remains is the *cause* of the ceiling: rows that are **wired but partial** —
ported bodies whose native **producers/sinks are unmodeled**. The engine's slot
comments carry these as `OL-*` and `FU24x-L*` legs. Phase-24 closes them: port the
missing native producers so the wired rows run **whole** (fidelity), and wherever
a producer genuinely un-dormants a row still NULL, the census rises too.

### Baseline (verified first-hand at spec time, HEAD `479efa8`)

- `make check` **110/110** (sanitized, strict).
- Wired **58/80 = 72%** (`35 action + 23 phase`); dispatch `21 UNSUP / 58 OK / 1 NOTF` (action 45 = 35 OK + 10 NULL; phase 35 = 23 OK + 11 NULL + the `0x16` INT3 NOTF).
- M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (691 lines, immutable).
- M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (**v22, 6522 lines**).
- README title `72%`; `AGENTS.md:90` `58/80 → 72%`.

### The producer frontier (frozen per-leg evidence)

The engine slot comments + `docs/ENGINE.md` carry the authoritative leg evidence.
The high-value clusters:

| cluster | the unported producer | what it un-dormants | source |
|---|---|---|---|
| **the desired-order matrices** | the line-up setup `FUN_00011620`/`FUN_00012010` (the `0x14C360` producer; **5 front-end callers**) | the act-4 formation-order arm `0x8AB08..0x8AB62` → action `17` + phase `1A`/`0B`; the OL-13 metadata source; the row-03 rewrite | FU201-L4 / FU197-L4 / FU192-L1 |
| **OL-13 metadata** | the `[rec+0xC]`·`[rec+0x10]`·`[rec+0x14]` source + the `[rec+0x1C]` seam | the phase-`0x02`/`0x15` gate; the row-03 rewrite | FU192-L1 / FU175-L1 |
| **OL-65** | the row-07/`22` angle inputs `word[+0x6B]`/`[+0x6D]`/`[+0x7D]` | action `22` (the `0x816DF` invoke gate) | FU185-L6 / FU247-L1 |
| **OL-63** | the row-05 stage-0 tail `0x7F3A1..0x7F57B` + the `FUN_0007F7E0` fallback | row-05 carrier completeness | FU-142 OL-63 |
| **OL-67/OL-72, OL-68/OL-69** | the row-04 (`0x7E7C8..0x7F141`) + rows-18/23 record/descriptor bytes + track words | rows `04`/`18`/`23` completeness | FU-137 |
| **OL-82/OL-83, OL-19/OL-22/OL-34** | the row-08 `+0x3D` projection scan + the `+0x8E`/`actor_type` field wrinkle | row-08 + the field-sharing reconciliation | FU-142 K.6 / FU-186 |
| **the anim/sink producers** | OL-27/OL-52 (`0x92820`/`0x6E598`) + OL-42 (the installer anim arm) | the per-row anim/sink completeness | FU-141 |
| **the phase-23 residuals** | FU241-L1 (non-tape compression equivalence), FU243-L4 (the row-25/15 scatter/id tables), FU241-L2 | the phase-23 wires' completeness | FU-241 §7 / FU-243 §5 |

**The headline caveat (FU-201 §8 / FU201-L1).** The desired-order matrices'
payoff may be a **runtime non-permutation** of current vs desired that *no native
routine in the write census performs* — in which case un-gating the act-4 arm on
the fill alone is a no-op or a fabricated residual (R45/R49/R15). FU248 must
settle this first-hand: port the producer's *observable* effect or rule the
census gain out and keep the arm gated.

## 2. Goal and non-goals

**Goal.** Close the unported native **producers** behind the already-wired match
rows, converting "ported body + unmodeled input/sink" legs into complete, live
behavior — proven at the fixture/dispatch layer, with the census recomputed if a
producer genuinely un-dormants a stray row.

### Non-goals (YAGNI)

- **No front-end visual work.** The formation setup is front-end-*produced*, but
  this phase ports the **producer**, not the menu art/screens (FU135 / the
  front-end layer is a separate subsystem).
- **No audio** (FU-229); no new subsystem.
- **No faked producers.** A producer that would compute a residual 0 / copy
  desired→current is a no-op or a fake (R45/R49/R15); it stays a leg.
- **No golden re-pin** unless a producer genuinely un-dormants a **tape-visible**
  row — then: written reason + frame diff. Most of this frontier is tape-dormant.
- **No unrelated refactor.**

## 3. Architecture (the producer-closure model)

```
        a WIRED row (non-NULL fn) whose body is PARTIAL
                 │
                 ▼
        its unmodeled native input/sink  (an OL-* / FU24x-L* leg)
                 │
                 ▼
        port the native producer  ──►  the row runs whole  (fidelity)
                 │
                 └──►  does it ALSO un-dormant a still-NULL row's installer?
                              ├─ YES → the census rises (+1 / +2)
                              └─ NO  → fidelity only (the census holds)
```

The phase's outcome is a set of **ported producers** (each with a fixture test +
a mutation-RED rider) and an updated **leg register**; the census rises only where
a producer is the genuine gate of a NULL row's installer.

## 4. The 8 parallel read-only recon slices

Dispatched after spec approval, before the implementation plan (the phase-21/22/23
recon-ahead precedent). Each is a **read-only** Ghidra MCP investigation → one FU
slice; **no code, no rename/comment/label/function/script/project save**.
First-hand bytes and addresses; fresh `get_xrefs_to` for reachability; the alias
traps checked (`dword[x]>>16` = word at `x+2`; `>>24` = byte at `x+3`; verify CALL
bytes; watch sign/width, inverted gates, gate-index misreads).

| # | Slice | Investigates | Deliverable |
|---|---|---|---|
| 1 | **FU248** | The desired-order matrices / line-up setup `FUN_00011620`/`FUN_00012010` (the `0x14C360` producer; the 5 front-end callers; the match-begin consumer); the runtime non-permutation question (FU201-L1) | The producer's port shape + the census ruling (`17`/`1A`/`0B` in or out) |
| 2 | **FU249** | OL-13 / FU192-L1: the `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]` metadata producer + the `[rec+0x1C]` seam | The port shape for the phase-`0x02`/`0x15` gate + the row-03 rewrite |
| 3 | **FU250** | OL-65: the row-07/`22` angle inputs `word[+0x6B]`/`[+0x6D]`/`[+0x7D]` and the `0x816DF` invoke gate | The row-22 port shape + the census ruling |
| 4 | **FU251** | OL-63: the row-05 stage-0 tail `0x7F3A1..0x7F57B` + the `FUN_0007F7E0` fallback | The row-05 completeness port shape |
| 5 | **FU252** | OL-67/OL-72 (row `04`) + OL-68/OL-69 (rows `18`/`23`): the record/descriptor bytes + track words | The rows-`04`/`18`/`23` completeness port shapes |
| 6 | **FU253** | OL-82/OL-83 (row `08` + the `+0x8E`/`actor_type` field wrinkle) + OL-19/OL-22/OL-34 | The row-08 + field-reconciliation port shapes |
| 7 | **FU254** | OL-27/OL-52 (`0x92820`/`0x6E598`) + OL-42 (the installer anim arm) | The anim/sink producer port shapes |
| 8 | **FU255** | FU241-L1 (non-tape compression equivalence) + FU243-L4 (the row-25/15 scatter/id tables) + FU241-L2 | The phase-23 residuals' port/proof shapes |

Each slice delivers: (a) the exact native producer with first-hand bytes, (b) a
fresh consumer/reachability census, (c) an explicit **port-vs-leg ruling** with its
**payoff (census vs fidelity)**, (d) numbered open legs with native owners.

## 5. Implementation task structure (Stage 2, serialized SDD)

Authored **after** all 8 drafts are frozen and promoted (the phase-21/22/23
shape). One implementer at a time; dependency order T1 → T2 → … → Tn.

- **T1 — determinism + feasibility gate.** Recompute the census; prove M1/M2
  byte-stability (≥3 replays); confirm the frontier's tape-dormancy. **No
  re-pin, no port.** If not byte-stable → STOP; fallback = a fixture-only proof.
- **T2..Tk — one producer port per proven-tractable cluster**, ordered by the
  wave's payoff × tractability ranking: each lands the native producer with a
  RED/GREEN fixture test + a **mutation-RED** rider.
- **T(k+1) — the census close-out** (only if a producer wires a row): recompute
  `floor(100 × wired/80)`, update the README title + `docs/ENGINE.md` +
  `docs/PROGRESS.md` + `AGENTS.md` + the `tests/test_engine_m2.c` pins in the
  same commit.
- **T(k+2) — the leg-register close-out:** append the resolved/forbidden legs
  (quoted errata; prior doc sections never rewritten).

The exact T-list and count are derived from the frozen slices (a slice may rule a
cluster a leg, in which case it is not a task).

## 6. Acceptance and the tape/census terms

- `make check` green (sanitized, strict).
- Every ported producer is asserted at the fixture/dispatch layer with a
  mutation-RED rider; every ruled-out cluster carries a numbered leg.
- **M1 `09b726b7…` never moves.**
- **Tape:** M2 byte-identical unless a producer un-dormants a **tape-visible**
  row (then written reason + frame diff). Any *unexplained* drift is a regression.
- The census is recomputed from what actually wires (it may hold at 58/80).
- An honest `make game` smoke (`DISPLAY=:1`) records the live reach (or an
  explicit waiver if no visible change is expected).

**Census caveat (stated, not hidden).** The guaranteed outcome is **fidelity**;
the census upside (the formation setup, OL-13, OL-65) is **uncertain** — FU-201
§8 warns the formation arm may have no honest trigger. The phase publishes the
ceiling it proves.

## 7. Risks

1. **The formation-setup payoff may be nil** (FU201-L1: the residual is a runtime
   non-permutation with no native writer) → fidelity only, no census gain. FU248
   owns the verdict.
2. **Most legs are tape-dormant** → fixture-only evidence, no tape proof.
3. **The frontier is diffuse** (many independent leg clusters) → the wave ranks by
   tractability so the Stage-2 plan does not over-scope.
4. **Producer ports risk touching wired behavior** — a ported producer feeding a
   live row could move the tape; T1's gate + the per-task tests own this.

## 8. Constraints carried in

- M1 `09b726b7…` immutable; the null backend is the determinism source of truth.
- Ghidra stays **read-only**; `/FIFA96.EXE` case-sensitive; LE-image globals at
  `+0x100000`; scripts disabled.
- No fakes: every unreachable native is a numbered leg with its native owner
  (R45/R49/R15 govern the formation arm).
- ISO-gated tests skip cleanly without the ISO.
- Commits `feat` / `fix` / `docs(fuNNN)` / `test` / `chore`; work on `main`; push
  per dispatch. Recon drafts in `.superpowers/sdd/<phase>/` (git-ignored); the
  controller promotes them to `docs/ghidra/FU248…FU255` in serialized commits.

## 9. Interfaces

- **The producer frontier** — the `OL-*` / `FU24x-L*` legs (the engine slot
  comments + `docs/ENGINE.md`).
- **The desired-order matrices** `formation_order`/`formation_current`/
  `formation_mismatch` + `fifa96_match_run_formation_sort` (the act-4 consumer).
- **The phase-23 wire seams** — `fifa96_match_action_25`/`_15`,
  `fifa96_match_action_phase_1E`, the `bx!=0` re-route.
- **The recon slices** `docs/ghidra/FU248…FU255` — appended errata only.

## 10. Decisions (approved in-chat)

- **D1 — producer closure, both payoffs.** The phase ports producers for
  **fidelity**, and treats any **census** gain as honest upside (never a reason
  to fake a producer).
- **D2 — read-only recon, serialized promotion.** The 8 slices run in parallel
  read-only; the controller freeze-reviews and promotes each to a canonical
  `docs/ghidra/FU2xx_*.md`.
- **D3 — the formation arm's honesty gate.** FU248 must settle the non-permutation
  question first-hand before any un-gating; a no-op/fake is forbidden.
- **D4 — no front-end visual work.** The formation setup is ported as a producer,
  not as a rendered menu.
- **Tooling note.** Ghidra MCP is the recon tool; **context7 is not applicable**
  (library docs, not RE). REA MCP is available for a provider-neutral second read.
