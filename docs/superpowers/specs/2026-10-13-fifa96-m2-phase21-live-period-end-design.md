# FIFA 96 M2 phase-21 — make the period-end spine live

Date: 2026-10-13
Status: approved for planning (design approved in-chat: theme, tape-reach, and
approach chosen by the user)
Parent spec: `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`
Predecessor plan: `docs/superpowers/plans/2026-10-13-fifa96-m2-phase20-sit8-completion.md`
Recon slices (to be produced by this phase's wave):
`docs/ghidra/FU223…FU230`.

## 1. Context

Phases 19 and 20 built the whole **period-end spine** as a derived, ported
model — but left it switched off. The chain exists:

- **The clock** (FU-62) raises `mr.clock_period_ended` when `sec == limit + aux`.
- **`fifa96_match_run_phase_drive`** consumes the completion: it writes phase
  `0x0C`, runs the T1/FU-213 **period-index roll** (`mr->global_157ac2`), and
  applies the FU-213 **OL-85 extra-time flag** (`mr->global_157ac0`, whose
  tie-break setters are the W2 `0x8B2AC` / W4 `0x8B3A2` / W5 `0x8B5CD` sites).
- **The derived `FUN_0008B9CC` chooser** yields the act: the extra-time arm
  (`out.act = 0x0C`, phase `0x22`) or the situation-1 arm (kickoff).
- **`fifa96_match_run_period_setup(mr, mode)`** (native `FUN_0004B02C` /
  `FUN_00088860`) dispatches situation 8 (extra-time route, RNG side `0x888C8`)
  or situation 1 (controlled side `0x888F2`).
- The **sit-8 route** runs `FUN_0008A938` → the bind (`0x8AE08..0x8AE54`,
  landed phase-20 T3) → act 7 → phase `0x1D` arm 0 (landed phase-19 T2) → row
  `0x20` (landed phase-20 T3) → the extra-time kickoff.
- The **half-time route** is situation 1 → act 1 → phase `0x17` → the kickoff
  (phase-18 T4).

The single switch is **`period_setup_live`**, which defaults to `0`
(`fifa96_match_run.c`, the phase-17 T2 R9 ruling: "wire it live only with T5's
bodies"). Those bodies have since landed — phase `0x20` (phase-16 T4/phase-18
T3), phase `0x1D` (phase-19 T2), phase `0x10` (phase-20 T2) and the sit-8 actor
bind (phase-20 T3) — so the R9 precondition is now plausibly satisfied.

What is still missing is the set of **loader/config producers** the real game
runs from the front-end and match-init, which the engine currently stages as
zero defaults. The phase-20 recon named them as legs:

- **FU215-L2** — the front-end `[0x157AC2]` producer `FUN_00038630` `0x38990`.
- **FU215-L3** — `FUN_00088860`'s live caller `0x74244`.
- **FU215-L5** — the `0x105270`-family config-block producer chain.
- **FU207-L2** — the OL-85 flag's front-end match-init producer `0x389A4`.
- **FU213-L3** — the end-of-match word `[0x158822]` / `FUN_0008BAF0`.

Phase 21 makes the spine **live** and proves it on the acceptance tape.

### Baseline (verified first-hand at spec time, HEAD `e02a547`)

- `make check` **110/110** (sanitized, `-fno-sanitize-recover=all`).
- Wired **55/80 = 68%** (`33 action + 22 phase`); dispatch
  `24 UNSUP / 55 OK / 1 NOTF` (sum 80; the phase `0x16` INT3 slot is the one
  NOTF). README title `68%`.
- M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd`
  (**691 lines**, immutable).
- M2 `441415fc423b05b2957f0a0bd51efa44f7f4651b4f8c07fcd6fb5ed6a8da9506`
  (**792 lines**, v20 strata).

## 2. Goal and non-goals

**Goal.** Turn the derived period-end spine from *tape-dormant* into *live* — a
running match completes period 1 (half-time → period 2) and, on a tied
regulation end, rolls into extra-time — and prove it by extending the committed
M2 acceptance tape to play both completions (a deliberate, documented re-pin).

### Non-goals (YAGNI)

- **No audio modelling** unless recon FU229 proves the `0x974DC(0x1E)` whistle
  touches the null-backend PCM hash. The audio ring (`0x974DC` → `0x155C8C` →
  the voice trigger) is otherwise **parked** (FU221-L1), never counted.
- **No full extra-time match end.** The tape stops at extra-time *entry*; the
  `[0x158822]` / `FUN_0008BAF0` end machine is out of scope except as far as
  FU228 proves the tape's reach needs it.
- **No dead-gate synthesis.** The dead bit-8 sit-8 gate (FU-215: no instruction
  sets bit 8 of `word[0x14C1D4]` / `word[0x14C1D6]`) is documented, never
  synthesized.
- **No new front-end art**; **no unrelated refactor**.

## 3. The live spine (architecture and data flow)

```
FU-62 clock: sec == limit + aux
        │  mr.clock_period_ended
        ▼
fifa96_match_run_phase_drive
        │  (a) writes phase 0x0C
        │  (b) period-index roll   mr->global_157ac2     ← live seed: FE 0x38990 (FU215-L2)
        │  (c) extra-time flag     mr->global_157ac0     ← OL-85; W2/W4/W5 tie-break (FU207/FU213)
        ▼
derived FUN_0008B9CC chooser   (engine mirror = phase_drive; native caller 0x74244, FU215-L3)
        ├── tied reg-end / ET-2  → extra-time arm → act 0xC (phase 0x22 timer)
        │        roll reaches 4  → period_setup(mode 0) → situation 8 (RNG side 0x888C8)
        │        → act 7 → phase 0x1D arm 0 (bind landed) → row 0x20 (landed) → ET kickoff
        └── otherwise            → situation 1 (controlled side 0x888F2)
                 → act 1 → phase 0x17 kickoff → phase 2   ← the HALF-TIME path
```

The phase activates this chain live and lands the producers that feed it.

## 4. The 8 parallel read-only recon slices

Dispatched **after spec approval**, before the implementation plan is written
(the phase-20 R63–R70 and `2026-10-08-fifa96-m2-phase7-recon-ahead` precedent).
Each is a **read-only** Ghidra investigation producing one FU slice; **no code,
no rename/comment/label/function/script/project save**. First-hand bytes and
addresses; fresh `get_xrefs_to` for any reachability claim; alias traps checked
(`dword[addr]>>16` = word at `addr+2`; `>>24` = byte at `addr+3`).

| # | Slice | Investigates | Deliverable |
|---|---|---|---|
| 1 | **FU223** | The FU-62 clock's live `limit`/`aux` source; the `fifa96_match_config` staging (FU-68 handoff); the `0x105270`-family config block (**FU215-L5**) | What sets a live period length; whether the engine's staged defaults suffice or a producer must be ported |
| 2 | **FU224** | `mr->global_157ac2` (period-index roll) + `mr->global_157ac0` (OL-85 flag): front-end match-init producers `0x38990` (**FU215-L2**) / `0x389A4` (**FU207-L2**); the W2/W4/W5 tie-break setters; exact cell identities | The live seeding + roll chain |
| 3 | **FU225** | The native `FUN_00088860` period-end chooser body + its live caller `0x74244` (**FU215-L3**); how the engine `phase_drive` mirror diverges | The chooser's exact live invocation + any engine delta to reconcile |
| 4 | **FU226** | The `FUN_0008A938` sit-8 route; the RNG side `0x888C8`; the session gate `[0x14C32A]` (**FU208-L8/FU218-L3**); `period_setup` mode-0 → situation-8 | Whether the RNG draw is null-backend determinism-safe; the session-gate live value |
| 5 | **FU227** | The situation-1 arm `0x888F2` → act 1 → phase `0x17` kickoff (the **half-time** route) vs the extra-time route | The half-time path + its live preconditions |
| 6 | **FU228** | The post-period lifecycle (OVER→POST→EXIT); the end word `[0x158822]` / `FUN_0008BAF0` (**FU213-L3**) | The lifecycle mapping the tape's reach needs |
| 7 | **FU229** | The `0x974DC(0x1E)` whistle (fires in phase `0x1D`/`0x21`/`0x22`): does it touch the null-backend **PCM hash**? | A definitive in/out-of-scope audio verdict for the re-pin |
| 8 | **FU230** | The M2 tape's scripted input: exact directives/frames for half-time + a tied reg-end → ET entry; expected new length; re-pin + frame-diff methodology | The pre-specified re-pin |

## 5. Implementation task structure (serialized SDD)

One implementer at a time; **never two implementation subagents in parallel**
(per the phase-20 constraint). Dependency order: T1 → T2 → T3 → T4 → T5 → T6.

- **T1 — determinism + feasibility gate.** Extend the tape's *scripted input* to
  drive a period completion (the clock reaches its limit) with the spine's
  producers still at their current (gated) state; prove null-backend determinism
  across ≥3 replays (byte-stable); characterize exactly what the run does now
  (the completion's frame picture and whether the transition fires). **No
  re-pin.** If not byte-stable → **STOP and report**; the fallback is a
  fixture-only proof with M2 unchanged.
- **T2 — the period/clock producer** (per FU223).
- **T3 — the roll + extra-time-flag seeding** (per FU224).
- **T4 — flip `period_setup_live` on + reconcile the chooser** (per FU225/FU226).
- **T5 — half-time + extra-time-entry landing** (per FU227/FU228): both
  completions run end-to-end; the post-period transition.
- **T6 — tape extension + re-pin + acceptance** (per FU230): extend the tape to
  play half-time + a tied regulation end → ET entry; re-pin M2 with the written
  reason + frame diff; update `docs/ENGINE.md` / `docs/PROGRESS.md` / `README.md`
  / `AGENTS.md`; record the smoke.

**Census caveat (stated, not hidden).** This phase likely wires **zero new
rows** — it activates already-counted rows — so the README percentage may not
move. If any row's live-dispatched status changes the census (e.g. a phase row
that is now written by a live source), recompute `floor(100 × wired / 80)` and
update the README title + `docs/ENGINE.md` (title + the two dispatch `N/80`
lines) + `docs/PROGRESS.md` + `AGENTS.md` **in the same commit**.

## 6. Acceptance and the tape extension / re-pin

- The extended M2 tape plays **half-time (period 1 → 2)** and a **tied
  regulation end → extra-time entry**; the golden is re-pinned with the written
  reason + frame diff.
- **M1 `09b726b7…` never moves.**
- `make check` green (sanitized); the spine asserted **at the tape layer** (the
  live dispatch order: clock completion → roll → flag → chooser → situation
  8 / situation 1 → act → phase → row → next kickoff).
- An honest `make game` smoke (`DISPLAY=:1`) records the live reach.

**Re-pin policy.** The extension is pursued **additively** where the evidence
allows (the 792-line v20 prefix unchanged, new frames appended — the phase-14 T3
v13→v14 precedent). A 0-0 regulation end is a natural tie (no goal scripted), so
the ET route is reachable without inventing score. Where additivity fails (e.g.
a shorter staged period changes the HUD clock indicator from the first frame),
the diff is documented line-by-line with the reason — never silently. Any
*unexplained* drift is a regression to investigate, never a re-pin.

## 7. Risks

1. **RNG determinism** — the sit-8 side `0x888C8` (FU226). Retired by the T1
   gate **before** any re-pin.
2. **Front-end producers unreachable** — `0x38990` / `0x389A4` may live in the
   unported, runtime-gated front-end/competition screens (FU-65). If so, the live
   seed is staged engine-side as a **disclosed stand-in** (the existing
   match-config-default pattern), never faked as a producer. Verdict owned by
   FU223/FU224.
3. **Tape additivity** — the extension may not be a clean prefix; characterized
   by the T1 probe and FU230's design.
4. **Post-period lifecycle** — the OVER→POST→EXIT mapping (FU228) may need the
   end machine; kept minimal (the tape stops at ET entry).
5. **Audio PCM** — only if the whistle touches the null-backend hash (FU229);
   otherwise parked.

## 8. Constraints carried in

- M1 `09b726b7…` immutable; the null backend is the determinism source of truth;
  the SDL3 backend stays consistent with it.
- The re-pin carries a written reason + frame diff (AGENTS.md golden rule).
- Ghidra stays **read-only**; `/FIFA96.EXE` case-sensitive; LE-image globals at
  `+0x100000`.
- No fakes: every unreachable native is a numbered leg with its native owner.
- ISO-gated tests skip cleanly without the ISO.
- Commits `feat` / `fix` / `docs(fuNNN)` / `test` / `chore`; work on `main`;
  push per dispatch.

## 9. Interfaces

- **`period_setup_live`** (`fifa96_match_run.c`) — flips `0` → `1` at T4.
- **`fifa96_match_run_period_setup(mr, mode)`** — already ported; the live caller
  path is T4/T5.
- **The roll / flag cells** `mr->global_157ac2` / `mr->global_157ac0` — seeded
  live at T3.
- **The chooser** `fifa96_action_phase_period_end` / `phase_drive` — reconciled
  at T4.
- **The tape scenario** (`tests/test_engine_m2.c`) — extended at T1/T6;
  **`tests/golden/engine/m2-frames.txt`** — re-pinned at T6.
- **The recon slices** `docs/ghidra/FU223…FU230` — appended errata only (prior
  sections never rewritten).
