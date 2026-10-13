# FIFA 96 native engine — M2 phase 20: the sit-8 completion + the two census rows

Date: 2026-10-13
Status: approved for planning (user chose the "sit-8 / extra-time tail" cohort, then approach B — the full completion)
Parent spec: `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`
Predecessor plan: `docs/superpowers/plans/2026-10-13-fifa96-m2-phase19-period-end-spine.md` (closed; `53/80 = 66%`)
Recon: `docs/ghidra/FU215…FU222` (phase-20 wave R63–R70, committed `268516b`)

## 1. Context

Phase 19 ported the **period-end producer chain** (the period-index roll + the extra-time decision, the situation-8 selector, the phase-`0x1D`/act-7 and phase-`0x22` slots) and closed at **`53/80 = 66%`** (`32 action + 21 phase`; dispatch `26 UNSUP / 53 OK / 1 NOTF`; the `0x16` INT3 slot is the one `NOTF`). M1 `09b726b7…` unmoved; M2 `441415fc…` byte-identical.

Phase-19's close-out left a frontier of **10 named residual legs**. The phase-20 recon wave (8 parallel read-only agents, R63–R70) attacked the largest cluster — the **sit-8 / extra-time tail** — and **overturned three premises** while finding the first real census moves since phase 17:

1. **The bit-8 arming route is structurally dead** (FU-215, FU-216). Native situation `8` has two producers: the range-word test (**bit 8 = `0x100`** of `[0x14C1D4]|[0x14C1D6]`, `FUN_0004B02C` `0x4B0CB TEST AH,1`) and the period-end test (`[0x157AC2]==4`, `FUN_00088860` `0x888C8`). An exhaustive `search_instructions` over all 235 017 instructions found **no instruction anywhere writes bit 8** of either word (the only high-byte writers set bit 9). The bit-8 branch is unreachable by construction; situation 8 arms **only** through `[0x157AC2]==4`, the path phase-19 T1 already wired. A **third** reader+dispatch site (`0x74312`) exists inside an unnamed blob `0x741C4..0x744CE` with **no static reference** (FU216-L1) — unported, and out of scope here.
2. **Two census rows are attainable, not one** (FU-222). The phase-19 phase-`0x1D` landing made **two** sit-8-tail rows reachable: **phase `0x10`** — its body `fifa96_match_phase_10` is already ported (`fifa96_match_handlers.c:4806`) but the slot is `NULL` (`:5210`, under a now-stale "never written" comment at `:5211`), and its writer — the phase-`0x1D` arm-0 phase-`0x10` write (`0x898DD FUN_000740A0(0x10)`, port `match_run.c:6091`) — became live when phase-19 T3 wired row `0x1D` (**registration only**, the cheapest +1); and **action `0x20`** — its body (`0x84EEC`) and its phase-`0x1D` arm-0 installer are already ported, blocked only on the missing actor-cell bind. Honest ceiling **`55/80 = 68%`**. Every other frontier leg yields **zero**.
3. **A real port bug** (FU-219). The port dispatches act 7 with `invoke = 0` (`src/fifa96_engine/fifa96_match_run.c:2890`) where native passes `invoke = 1` (`0x8AEAE MOV EBX,1`): native runs the act-7 arm 0 **synchronously on the dispatch frame**; the port defers it one granted frame. FU208-L4 stands, confirmed at byte level.

Two further findings are corrections, not code: **FU211-L2 is dissolved** (FU-221 — the "`[0x15881A]` cell" was a re-propagation of the FU-165 alias error: by the repo alias rule `dword[0x158816]>>16` is `word[0x158818]`, the act timer, not `word[0x15881A]`; the port's `mr->act.timer >= 0x3C` is already faithful), and the phase-`0x10` evidence strings are **stale** (FU-222 — comments at `fifa96_match_handlers.c:5211` and pins at `tests/test_engine_m2.c:3045/3310/3366` still say "never written … unwired row 0x1D body").

## 2. Goal

**Land the two attainable dispatch rows** (`phase 0x10` → 54, `action 0x20` → 55 = 68%), **fix the act-7 invoke-now cadence**, and **register the sit-8 tail's honest legs** — without ever synthesizing the dead bit-8 gate. The sit-8 chain becomes genuinely live (bind → act 7 → phase `0x1D` arm 0 → row `0x20`) rather than theory.

### Non-goals

- **The dead bit-8 gate.** No producer is synthesized for bit 8 (native never sets it; FU-215). The finding is documented, and the port's existing `fifa96_input_range_words` (correctly omitting `0x100`) is left alone.
- **The audio subsystem.** The phase-`0x22` audio ring-push (`0x974DC` → the 20-slot ring at `0x155C8C`) has no ported ring/voice/mixer; it stays an honest leg (FU221-L1), never faked.
- **Unreachable natives.** The `0x74312` third producer (FU216-L1), the arm-8 no-winner rebind's second writer (FU217-L2), and the `0x7A490` record place's unported integral are documented as numbered legs; only the faithful, reachable subset is ported.
- Bit-exact AI fidelity; the arm-4 record `pos_x/pos_z`+anim write-back is a leg unless the recon proves the port can perform it faithfully (FU-220: it currently stages into a stack-local copy only).

## 3. Design

### 3.1 The target (census arithmetic)

- Start: **`53/80`** (`32 action + 21 phase`).
- **T2** registers phase slot `0x10` (producer live) → **`54/80`**.
- **T3** binds `[0x1587D4]` + flips `{0x20, NULL}` (producer live) → **`55/80 = 68%`**.
- No other task moves a row. The number is **whatever the tasks prove producer-live** — never hand-set; each task recomputes `floor(100 × wired/80)` when its count moves, keeping README title + `docs/ENGINE.md` (title + the dispatch sum-to-80 line) + `docs/PROGRESS.md` + `AGENTS.md:90` consistent **in the same commit**.

### 3.2 The sit-8 completion (T1, T3, T4, T5)

The producer chain, top-down, matching native byte-for-byte:

- **The bind** (FU-217): native `0x8AE54 MOV [0x1587D4],EAX`, `EAX = [0x1587CC + side*4]`, filled by `FUN_000897F4(side, start)` — a wrap-order scan of records `1..10` (index 0 never scanned) selecting the first **non-occupied** record (`byte[rec+0x9A] == 0`). Guard: `byte[0x157A4C] != 1`. The port adds a `match_run_free_record(mr, team, start)` helper (mirroring `FUN_000897F4`, placed near the existing `match_run_phase_pick`) and a `if (situation == 8u && mr->global_157a4c != 1u) { global_157a4c = 1; actor_1587d4 = free_record(side, 0xA); }` block on the sit-8 direct path (before the terminal at `match_run.c:3133`), then flips `fifa96_match_handlers.c:5149` `{0x20, NULL}` → `{0x20, fifa96_match_action_20}`.
- **The cadence** (FU-219, T1): the ported `switch` advances one arm per granted frame; native runs arm 0 synchronously at dispatch (the `0x8892E TEST BX,BX` / `0x88937 CALL dword[0x158808]` tail, `BX = 1`). The fix is the single expression at `match_run.c:2890` — act 7 must pass `invoke = 1`. This is a correctness fix, not a row: **any tape movement from it is a regression to explain, not a re-pin** (the sit-8 route is dormant on the tape).
- **Row `0x20` body parity** (FU-220, T4): the ball-record place (arm 0: `[ball+0x4D]=0`, `[ball+0x55]=0xB10`, `0x79B6C` → `ball+0x59/0x5D/0x61/0x28/0x3D`) is ported to the extent the port's ball model supports it; the unportable subset (the `0x8CFAC` 22-record anim walks, the `0x73E08` block commit, the `0x73DC4`/`0x700F4` camera sinks, the arm-4 `0x7A490` record write) stays a leg. Tests assert **only** the genuine writes the port performs — never a write it does not.
- **The errata + rebind** (FU-216/FU-221, T5): the arm-8 no-winner rebind (`0x89DCF`) and the `0x74312` third producer are documented; FU211-L2's dissolution and FU-216's "three sit-8 sites, not two" land as **appended errata** in the frozen FU docs (prior sections never rewritten).

### 3.3 Interfaces

- `fifa96_match_free_record(mr, team, start)` → the record index (or a sentinel), mirroring `FUN_000897F4`'s wrap-order non-occupied scan; unit-testable in isolation.
- The `actor_1587d4` cell + the per-side `[0x1587CC]/[0x1587D0]` source cells modelled on the run state (FU217-L3/L5) — the minimum the bind + row-`0x20` body need.
- `fifa96_match_action_20` (`fifa96_match_handlers.c`) gains the ball-record place; its signature is unchanged.

### 3.4 Testing

- **Per task:** RED test → implement → GREEN → `make check` + tape `cmp`; each gate **mutation-RED** (remove/mutate the new producer or gate → the test aborts). The bind's fixture drives a real period completion (`period == 4` / `clock_period_ended` through `phase_drive`, mirroring the phase-19 loop-backed test) and asserts `actor_1587d4` = the first non-occupied record `>= 1` + the code-`0x20` install after `act_tick`.
- **Tape posture:** all phase-20 landings are **render-invisible and/or dormant on the scripted tape** (the tape stages no period-end, reaches neither sit-8 producer). Expect **M2 `441415fc…` byte-identical** — a **strata-only v20 label** (the phase-15/19 precedent), **no re-pin**. M1 `09b726b7…` never moves. Any *unexplained* presented-frame move is a regression to investigate.

## 4. Workstreams (tasks)

Serialized SDD; each task = fresh implementer → spec+quality review → bounded fix loop → controller verify (`make check` + tape `cmp` + clean tree).

| # | Task | Row |
|---|---|---|
| **T1** | The act-7 invoke-now cadence fix (`match_run.c:2890` → `invoke = 1` for act 7) + the stale phase-`0x10` evidence-string audit (FU-222: `handlers.c:5211`, `test_engine_m2.c:3045/3310/3366`) | — |
| **T2** | Register phase slot `0x10` (FU-222/FU214-L4; body `fifa96_match_phase_10` ported, slot `:5210` → flip, stale comment `:5211` fixed; writer live via row `0x1D`) | 53 → **54** |
| **T3** | The `[0x1587D4]` bind (`fifa96_match_free_record` mirroring `FUN_000897F4`) + flip `{0x20, NULL}` (FU-217/FU210-L3) | 54 → **55 (68%)** |
| **T4** | Row-`0x20` body parity — the ball-record place (FU-220/FU210-L2); genuine writes only | — |
| **T5** | The arm-8 rebind + the `0x74312` third-producer + the FU211-L2/FU-216 errata (docs) | — |
| **T6** | Acceptance: the tape **v20 strata label**, the four riders, honest no-re-pin | — |

Dependencies: **T1 → T2 → T3 → T4 → T5 → T6**. T1 first (the cadence is a correctness fix upstream of the completion); T2 before T3 (each census bump lands in its own consistent commit); T4 depends on T3 (the bind makes row `0x20` dispatch); T5/T6 close.

## 5. Method

Subagent-driven development, exactly as phases 17–19: one plan + this spec; `.superpowers/sdd/` workspace with a `progress.md` ledger; `scripts/task-brief` / `scripts/review-package`; per-task implementer + task reviewer + fix loop (max 5 rounds); a final whole-branch review → **one** fix wave → **one** scoped re-review; then workspace deletion. Ghidra stays **read-only** (`/FIFA96.EXE`, case-sensitive; fresh xrefs for census claims; the alias traps); new FU numbers continue from **FU223**. Commits `feat/fix/docs/test`; implementers commit + push per dispatch; the controller verifies.

## 6. Acceptance criteria

- **Census:** `55/80 = 68%` (`34 action + 21 phase`; dispatch `24 UNSUP / 55 OK / 1 NOTF`), recomputed + consistent across README title, `docs/ENGINE.md`, `docs/PROGRESS.md`, `AGENTS.md:90`.
- **Determinism:** M1 `09b726b7…` unmoved; M2 `441415fc…` byte-identical (strata-only v20 label); `make check` **110/110+** (sanitized).
- **Fidelity:** the bind + the cadence fix reproduce native byte-for-byte on their covered paths; the dead bit-8 gate is documented, not synthesized; every unreachable native is a numbered leg.
- **Honesty:** no row counted without a live dispatch; no fake write asserted.

## 7. Risks

- **The bind's record-scan exactness** (FU217-L1) — the wrap-order + the `+0x9A` occupancy writer are only partly derived; mitigate by unit-testing `fifa96_match_free_record` against the fixture's record table and leg'ing the ambiguous corners.
- **The cadence fix could move a frame** if act 7 ever fires on the tape — it does not (dormant), but the tape `cmp` is the guard; any move is investigated, never re-pinned.
- **Census churn** — the two +1s must each land in their own commit with the count consistent; a registration-only count is a violation (the AGENTS.md rule).
- **Scope creep into the dead bit-8 / audio subsystems** — both are explicit non-goals; keep them legs.

## 8. Open legs (the honest register)

FU215-L1…L5 (the dead gate + the front-end producer chain), FU216-L1/L3 (the `0x74312` blob + the engine blob gap), FU217-L1/L2/L3/L5 (the scan exactness, the arm-8 rebind, the per-side cells), FU218-L3 (the mode-4 Practise queue session), FU219-L1…L6 (the invoke-now superset, the seam latch, the arm-0 record writes, the frame-boundary gate, the arm-9 `[0x157AC0]` write-back), FU220-L1…L6 (the ball place, the anim walks, the arm-4 integral, the block commit, the camera sinks), FU221-L1…L4 (the audio subsystem — parked). Each keeps its native owner; none is faked.
