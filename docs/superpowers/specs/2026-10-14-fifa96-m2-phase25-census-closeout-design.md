# FIFA 96 M2 phase-25 — the match-action census close-out

Date: 2026-10-14
Status: approved for planning (design approved in-chat: theme "close the
match-action census", the **producer-porting** path chosen by the user, with
the M2 golden preserved and the ECX-aware installer re-census included).
Parent spec: `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`
Predecessor spec: `docs/superpowers/specs/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-design.md`
Predecessor plan: `docs/superpowers/plans/2026-10-14-fifa96-m2-phase24-match-fidelity-producer-closure-recon-ahead.md`
Recon slices (to be produced by this phase's wave): `docs/ghidra/FU256…FU263`.

## 1. Context

Phase-23 raised the honest action-dispatch census to **58/80 = 72%** (`35 action
+ 23 phase`). Phase-24 then tested the *producer* frontier behind the already-wired
rows and proved it **fidelity-only**: the headline producer (the formation-order
arm) is produced by a **front-end** routine with no runtime divergence (`FU248`),
the OL-13 metadata gate is **already live** (`FU249`), and OL-65 is a 2-line
staging fix (`FU250`). **No row-flip via an already-wired producer remains.**

The census is therefore *not* exhausted — it is blocked by **unported gating
producers** that no prior phase landed. Phase-25 ports them: for each of the 22
unwired rows, either (a) port the producer that installs/dispatches it and **wire
it**, or (b) prove first-hand it cannot count and freeze it as an honest leg.

### Baseline (verified first-hand at spec time, HEAD `6df8465`)

- `make check` **110/110** (sanitized, strict).
- Wired **58/80 = 72%** (`35 action + 23 phase`); dispatch `21 UNSUP / 58 OK / 1
  NOTF` (action 45 = 35 OK + 10 NULL; phase 35 = 23 OK + 11 NULL + the `0x16` INT3
  NOTF).
- M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (691 lines, immutable).
- M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (**v22, 6522 lines**).
- README title `72%`; `AGENTS.md:90` `58/80 → 72%`.

### The unwired rows (the census target)

**Action (10 NULL):** `03` (body ported, `[rec+0x1C]=0x6DCC8` target-rewrite chain
is tape-visible at transcribed frame 29), `0A` (not ported; installer `0x7CDD8`
has no xrefs), `0D` (body ported; only installer is row-09 arm 1 `0x80F61`),
`17` (body ported; only installer is the act-4 formation arm), `1A` (body ported;
only installer is the `FUN_00077EAC` CPU ladder), `22` (OL-65 gate), `27`/`29`/`2C`
(bodies ported, "no installer" verdicts via a **literal-scan method phase-23
proved incomplete**), `2B` (dead entry = the shared row-29 RET).

**Phase (12):** `01`/`11`/`12` (slots NULL; the formation-placement body `0x6E1D0`
is shared with wired rows — the R10 "do not double-wire" question + the
"never written by an engine source" question), `0B`/`0D`/`0E`/`13`/`14`/`1A`/`1B`/`1C`
(slots NULL; bodies/producers unported), `16` (INT3 stub — NOT_FOUND by design).

### The producer frontier (frozen per-leg evidence)

The engine slot comments + `docs/ENGINE.md` carry the authoritative leg evidence.
The gating-producer clusters:

| cluster | the unported producer | gates | source |
|---|---|---|---|
| **row-09 arm-1 chain** | the `0x80BFB..0x80FCB` arm-1 install path (`0x80F61 → 0x7D9A4`) + its hitched helpers/sinks (`0x8f188`/`0x8ed40`/`0x92820`/`0x7a490`/`0x8a3fc`) over the gates `[0x15877C]`/`[rec+0x3D]`/`[rec+0x24]`/`[[rec+4]+0x13]` | action `0D` (+ row-09 arms 1/2/3) | FU178-L1/FU178-L6, FU-180 §3 |
| **the keeper CPU ladder** | `FUN_0008D8EC` → `FUN_000782D0` pre-pass + `FUN_00077EAC` (`0x78227`/`0x7824F`/`0x78277`) | action `1A` | FU186-L5/L6/L7, OL-34 |
| **the `[rec+0x1C]=0x6DCC8` chain** | the OL-13 `[rec+0xC]` bind + the phase-2 hot-path target rewrite + the special arm | action `03` | FU177-L1, FU175-L1, FU192-L1, FU249 |
| **OL-65 staging** | the row-07/`22` angle words `word[+0x6D]`/`word[+0x6F]`/`word[+0x7D]` staging + the `0x816DF` invoke proof | action `22` | FU185-L6, FU250 |
| **the installer domain** | the ECX-carried bulk installer `FUN_0008CEB8`/`0x7D9A4` (and the `0x888FC` act seam) swept over the whole image | rows `27`/`29`/`2B`/`2C`/`0A` | FU-243 §4.2 (the method), OL-48, OL-14 |
| **the unwired phase bodies A** | bodies + producers for phases `01`/`0B`/`0D`/`0E`/`11`/`12` + the R10 double-wire question | the six NULL phase slots | FU192 §2, FU191 §10, OL-13 |
| **the unwired phase bodies B** | bodies + producers for phases `13`/`14`/`1A`/`1B`/`1C` | the five act-tail phase slots | FU192, FU239, OL-13 |
| **the act-4 formation arm + the phase-24 errata** | the sit-2..4 arm `0x8AB08..0x8AB62` (proven gated, FU248) + the phase-24 corrections | action `17` (likely a leg) + the errata sweep | FU201-L4/L6, FU248…FU255 |

**The honesty rail (carried from R15/R45/R49).** A producer that would compute a
residual 0 / copy desired→current is a no-op or a fake; it stays a leg. Phase-24
already proved this for the formation arm (`FU248`) — phase-25 must not re-fake it.

## 2. Goal and non-goals

**Goal.** Raise the wired census from **58/80** toward the **honest ceiling** by
**porting the gating producers** of the 22 unwired rows and wiring each row whose
producer is genuinely live; freeze every non-wireable row as an honest numbered leg.

**Definition of done.** Every unwired row is either (a) **wired + counted** — a
ported path installs *and* dispatches it (tape-dormant rows count only when a
fixture proves the install+dispatch), or (b) an **honest numbered leg** with a
named owner and a proven reason it cannot count.

### Non-goals (YAGNI)

- **No faked producers or invented installers** (R15/R45/R49).
- **No new subsystem** — this is the match-action dispatch census only; no
  front-end/season/audio work.
- **No golden re-pin** unless a producer genuinely un-dormants a **tape-visible**
  row — and even then the design prefers to keep the row leg'd (the §1 rail) rather
  than move M2. The 0x6DCC8 chain (row `03`) is the one known tape-visible risk.
- **No unrelated refactor.**

## 3. Architecture (the census-closeout model)

```
        a NULL action/phase row  (slot fn == NULL)
                   │
                   ▼
        its native gating producer  (an unported arm / ladder / chain)
                   │
                   ▼
        port the producer  ──►  wire the slot  ──►  RED-first dispatch test
                   │                                        │
                   │                              tape-dormant?  ── yes ──► fixture-proven install+dispatch ⇒ COUNTED
                   │                                        │
                   │                                    tape-visible ⇒ the row is leg'd (M2 preserved) OR an honest re-pin
                   ▼
        a producer that cannot honestly run it  ⇒  a NUMBERED LEG (owner named)
```

The phase's outcome is a set of **ported gating producers** (each with a RED-first
dispatch/frame test + a mutation-RED rider), a set of **newly-wired rows** (the
census rise), and an updated **leg register** (the honest ceiling).

## 4. The 8 parallel read-only recon slices

Dispatched after spec approval, before the implementation plan (the
phase-21/22/23/24 recon-ahead precedent). Each is a **read-only** Ghidra MCP
investigation → one FU slice; **no code, no rename/comment/label/function/script/
project save**. First-hand bytes and addresses; fresh `get_xrefs_to` for
reachability; the alias traps checked (`dword[x]>>16` = word at `x+2`; `>>24` =
byte at `x+3`; verify CALL bytes; watch sign/width, inverted gates, gate-index
misreads).

| # | Slice | Investigates | Deliverable |
|---|---|---|---|
| 1 | **FU256** | The row-09 arm-1 install chain: the `0x80BFB..0x80FCB` arm, the `0x80F61 → 0x7D9A4` install, its gates `[0x15877C]`/`[rec+0x3D]`/`[rec+0x24]`/`[[rec+4]+0x13]`, and its helpers/sinks | The row-09 arm port shape + the action-`0D` census ruling |
| 2 | **FU257** | The keeper CPU ladder `FUN_0008D8EC` → `FUN_000782D0` + `FUN_00077EAC`; the `[0x157C2C]` direction machine | The action-`1A` port shape + the census ruling |
| 3 | **FU258** | The `[rec+0x1C]=0x6DCC8` target-rewrite chain + the OL-13 bind + the phase-2 hot-path special arm; the frame-29 tape risk | The action-`03` port shape + the tape-dormancy ruling |
| 4 | **FU259** | OL-65: the row-07/`22` angle words + the `0x816DF` invoke; the reachability proof | The action-`22` port shape + the census ruling |
| 5 | **FU260** | The **ECX-carried installer re-census** (`FUN_0008CEB8`/`0x7D9A4`/`0x888FC`) over ALL unwired action rows | The re-census verdict per row (`27`/`29`/`2B`/`2C`/`0A`) |
| 6 | **FU261** | The unwired phase bodies A (`01`/`0B`/`0D`/`0E`/`11`/`12`) + the R10 double-wire question + the "never written" question | The six-port shapes + the census ruling |
| 7 | **FU262** | The unwired phase bodies B (`13`/`14`/`1A`/`1B`/`1C`) | The five-port shapes + the census ruling |
| 8 | **FU263** | The act-4 formation arm (`17`) re-verification + **the phase-24 corrections sweep** (FU247 erratum, OL-83 unify, `ENGINE.md:4037`, FU-84 §1, FU201-L1 owner) | The action-`17` verdict + the errata list |

Each slice delivers: (a) the exact native producer with first-hand bytes, (b) a
fresh consumer/reachability census, (c) an explicit **port-vs-leg ruling** with its
**payoff (census vs fidelity)** and a **tape-dormancy verdict**, (d) numbered open
legs with native owners.

## 5. Implementation task structure (Stage 2, serialized SDD)

Authored **after** all 8 drafts are frozen and promoted (the phase-21/22/23/24
shape). One implementer at a time; dependency order T1 → T2 → … → Tn.

- **T1 — determinism + feasibility gate.** Recompute the census; prove M1/M2
  byte-stability (≥3 replays); confirm each candidate producer's tape-dormancy.
  **No re-pin, no port.** If not byte-stable → STOP; fallback = a fixture-only proof.
- **T2..Tk — one producer port per proven-tractable cluster**, ordered by the
  wave's payoff × tractability ranking: each lands the native producer, wires the
  slot, and adds a RED-first dispatch/frame test + a **mutation-RED** rider.
- **T(k+1) — the installer re-census landing** (only if FU260 un-gates a row).
- **T(k+2) — the census close-out** (only if a producer wires a row): recompute
  `floor(100 × wired/80)`, update the README title + `docs/ENGINE.md` +
  `docs/PROGRESS.md` + `AGENTS.md` + the `tests/test_engine_m2.c` pins in the same
  commit.
- **T(k+3) — the corrections + leg-register close-out:** land the phase-24 errata
  (FU247 erratum, OL-83 unify, the `ENGINE.md`/FU-84 fixes) and append the
  resolved/forbidden legs (quoted errata; prior doc sections never rewritten).

The exact T-list and count are derived from the frozen slices (a slice may rule a
cluster a leg, in which case it is not a port task).

## 6. Acceptance and the tape/census terms

- `make check` green (sanitized, strict).
- Every newly-wired row is proven install+dispatch at the fixture/dispatch layer
  with a mutation-RED rider; every ruled-out row carries a numbered leg.
- **M1 `09b726b7…` never moves.**
- **M2 `b281a9b2…` byte-identical** (the approved stance; `cmp`-verified). A
  tape-visible producer keeps its row **leg'd** rather than move the golden, unless
  an honest re-pin (written reason + frame diff) is unavoidable.
- The census is recomputed from what actually wires (it may hold at 58/80, or rise
  toward the proven ceiling).
- An honest `make game` smoke (`DISPLAY=:1`) records the live reach (or an explicit
  waiver if no visible change is expected).

**Census caveat (stated, not hidden).** The honest ceiling is **below 80** — the
dead entry `2B`, phase `16` (INT3 by design), and FU-28's unsupported selectors can
never count. The phase publishes the ceiling it proves; a row that can only be
wired by a fake stays a leg.

## 7. Risks

1. **The heavy producers may be intractable or genuinely gated** (the CPU ladder,
   the row-09 arm, the 0x6DCC8 chain) → those rows stay honest legs; the census
   gains less than hoped. FU256…FU262 own the verdicts.
2. **The 0x6DCC8 chain is tape-visible** (frame 29) → wiring row `03` would move
   M2. The §1 rail prefers the row leg'd; FU258 owns the ruling.
3. **Installer re-census may find nothing new** → `27`/`29`/`2C`/`0A` stay
   uncounted. FU260 owns the verdict.
4. **A ported producer feeding a live row could move the tape** → T1's gate + the
   per-task tests + the M2 `cmp` own this.

## 8. Constraints carried in

- M1 `09b726b7…` immutable; the null backend is the determinism source of truth.
- Ghidra stays **read-only**; `/FIFA96.EXE` case-sensitive; LE-image globals at
  `+0x100000`; scripts disabled.
- No fakes: every unreachable native is a numbered leg with its native owner
  (R15/R45/R49 govern the formation arm and every invented installer).
- ISO-gated tests skip cleanly without the ISO.
- Commits `feat` / `fix` / `docs(fuNNN)` / `test` / `chore`; work on `main`; push
  per dispatch. Recon drafts in `.superpowers/sdd/<phase>/` (git-ignored); the
  controller promotes them to `docs/ghidra/FU256…FU263` in serialized commits.

## 9. Interfaces

- **The unwired rows** — `fifa96_match_action_table` (the 10 NULL action slots) and
  `fifa96_match_phase_table` (the 11 NULL phase slots + the `0x16` INT3 slot).
- **The gating producers** — the row-09 arm-1 chain, the keeper CPU ladder, the
  `[rec+0x1C]` chain, OL-65, the ECX-carried installer, the phase bodies.
- **The phase-24 wire seams** — `fifa96_match_run_set_piece`'s `bx!=0` re-route,
  `fifa96_match_action_25`/`_15`, `fifa96_match_action_phase_1E`.
- **The recon slices** `docs/ghidra/FU256…FU263` — appended errata only.

## 10. Decisions (approved in-chat)

- **D1 — port the gating producers.** The phase ports the producers gating the 22
  unwired rows (not a cheap NULL→fn flip); the census rises only where a producer
  genuinely installs *and* dispatches a row.
- **D2 — preserve M2 byte-identical.** Producers land install-live/tape-dormant; a
  tape-visible row is kept leg'd rather than move the golden (honest re-pin only if
  unavoidable).
- **D3 — audit the no-installer verdicts.** FU260 re-runs the ECX-aware installer
  census (the phase-23 method that refuted rows 25/15) over `27`/`29`/`2B`/`2C`/`0A`.
- **D4 — read-only recon, serialized promotion.** The 8 slices run in parallel
  read-only; the controller freeze-reviews and promotes each to a canonical
  `docs/ghidra/FU2xx_*.md`.
- **D5 — honest ceiling.** Dead rows (`2B`, phase `16`) and FU-28's unsupported
  selectors stay permanent negatives; no fake producer is ever invented.
- **Tooling note.** Ghidra MCP is the recon tool; **context7 is not applicable**
  (library docs, not RE). REA MCP is available for a provider-neutral second read.
