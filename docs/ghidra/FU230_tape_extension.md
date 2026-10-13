# FU-230 — the M2 tape extension + re-pin (phase-21 wave, slice W21-8)

Provenance: M2 phase-21 **Stage-1 recon-ahead**, 2026-10-13, **read-only**. This
slice is a **design/spec slice** — it writes no code and no test; it
pre-specifies the acceptance-tape extension and the re-pin that the Stage-2
producer plan's **T6** (`docs/superpowers/specs/2026-10-13-fifa96-m2-phase21-live-period-end-design.md`
§5) implements. Every claim below cites a **fresh first-hand read** of the repo
tree at this session (`file:line`); the one native fact is cross-cited to a
frozen FU doc. Engine source read first-hand; `tests/test_engine_m2.c` read
first-hand. **No writes**: `tests/`, `src/`, `include/`, `docs/` untouched;
`tests/golden/engine/m1-frames.txt` (`09b726b7…`, 691 lines, **immutable**) and
`tests/golden/engine/m2-frames.txt` (`441415fc…`, 792 lines, v20 strata) untouched.

Scope (W21-8 brief): (1) record the tape generator's input format + current
scripted directives; (2) design the extension that plays **half-time** and a
**regulation-end → extra-time entry**; (3) specify the re-pin (written reason,
frame-diff methodology, mutation-RED rider classes); (4) hand off to T6. This
slice extends FU-143 (the phase driver), FU-170/IM (the restart-chain re-pin
precedent), FU-207 (the OL-85 flag), FU-213 (the extra-time clock), FU-214
(phase-19 acceptance), FU-215 (the sit-8 arming writer).

---

## Answer in one line

The M2 tape is a **scripted key tape (`M2_KEYS`, `M2_KEYS_LEN == 1800`, 154
explicit entries) + 7 match-relative directive steps** driving the null backend
(`tests/test_engine_m2.c:1707`, `:1877`, `:1571`);
**the extension is append-only in the preferred design** — keep the 792-line v20
prefix byte-identical and let the resumed phase-2 clock run through **two live
period completions** (period 0 → half-time → period 1 → regulation end) — **but
the brief's "0-0 tie → extra-time" premise is contradicted by the shipped
native**: the OL-85 flag `[0x157AC0]` is raised on a **differing** regulation-end
score (`fifa96_match_run.c:2618`; FU-207 §3.2 W2 `0x8B2AC`), so a literal 0-0
tie leaves the flag **clear** and enters the **no-ET** arm (act `0xB`,
phase `0x0C`). A 0-0 tie reaches extra time **only** through the unported
equal-score override `[0x14C2EE]` (W3 `0x8B2C1`; FU213-L1). Either keep the v20
goal (score 1-0 → differ → ET, additive) or stage the override (FU213-L1).

---

## 1. The tape generator: input format + current directives (fresh)

### 1.1 The scripted input is a key tape, not a text file

`run_tape` builds a `fifa96_platform_null_config` with `pcfg.tape = M2_KEYS` and
`step_ns = 10000000` (one 100 Hz PIT tick per engine step), then steps the
engine `while (steps < M2_STEP_CAP)` with `M2_STEP_CAP = 2000`
(`tests/test_engine_m2.c:2632-2636`, `:1568`, `:2662`). `M2_KEYS` is an
**unsized designated-initializer `fifa96_platform_key` array**
(`tests/test_engine_m2.c:1707-1876`): the highest initializer index is `[1799]`
(`:1875`), so `M2_KEYS_LEN = sizeof M2_KEYS / sizeof M2_KEYS[0]` (`:1877`) is
**1800**; only **154** slots are explicitly initialized, the remainder are
zero (`{0,0}`). The length is load-bearing: `n_poll` returns `*count = 0` once
`tape_pos >= tape_len` (`src/fifa96_engine/platform_null.c:29-33`), so the
LEFT-held window through index 140 **requires `tape_len ≥ 141`** — a 105-entry
tape would cut the held-LEFT window and cannot reproduce the golden.

| indices | key | role (fresh) |
|---|---|---|
| 0..5 | CONFIRM/DECLINE edges | boot → skip intro → front-end → match start (frame 5) |
| 6..11 | RIGHT held | move (the pad locomotion path) |
| 12..14 | KICK edge | kick |
| 15..44 | RIGHT held | held direction across the forced 0x13/0x14/mechanics windows |
| 45..139 | LEFT held | held direction through the band/goal window |
| 140 | LEFT released | (`:1850`) |
| 206..212 | KICK edge | natural-probe release (no directives) |
| 1794..1799 | KICK edge | phase-18 T4 second release (past the tape's stop; `:1870`) |

(The 154 explicit entries = 6 + 6 + 3 + 30 + 95 + 1 + 7 + 6; the array's
`sized-1800` tail is zero-initialized and safe because `n_poll` stops consuming at
`tape_len`.)

`fifa96_platform_key` is `{keycode, pressed}`; the only keycodes used are
CONFIRM/DECLINE/RIGHT/LEFT/KICK (`:1708-1875`). `step_ns = 10 ms` means the
engine polls once per step, so **entry index = engine step − 1**
(`:1146-1148`); the accepted run self-terminates at step 792, so the explicit
entries at 206..212 (index ≥ 206) and 1794..1799 are never reached on the
accepted transcript and exist only for the natural probe
(`run_natural_probe` replays `M2_KEYS` with the directives disabled).

### 1.2 The directives (match-relative steps)

`m2_directives(e, next, match_start_step, res)` runs at the top of engine step
`next`, with `m = next − match_start_step` (`:2098-2101`). `match_start_step`
is 5 on both ISO and no-assets modes (`main` asserts `res.match_start_step == 5`,
`:3852`). The seven scripted directives (`:1571-1592`, bodies at `:2102-2623`):

| macro | m | step | what it forces / asserts (fresh) |
|---|---|---|---|
| `M2_M_KICKOFF` | 1 | 6 | force `state.phase = 0x13` over the derived kickoff entry; stage `session_gate_14c32a = 1`; `phase_machine.state = 0x13` (`:2102-2145`). Asserts the one state-1 taker + the human slot bind (`:2117-2129`). |
| `M2_M_HALF` | 21 | 26 | force `state.phase = 0x14`; `phase_machine.state = 0x14` (`:2146-2148`). |
| `M2_M_MECH` | 41 | 46 | assert the FU-142a arm codes 0x26/0x28/0x2A (`:2153-2156`); assert the pre-staging dormancy; **`set_phase(2)` (class 1: the clock runs)** and **`set_period(mr, 1, 1)`** (`:2311-2312`); stage `M2_STAGE_ROWS` (`:2313-2315`). |
| `M2_M_BAND` | 42 | 47 | the goal-mouth scenario staging: claim isolation, act-1 flag, camera park, band record placement (`:2316-2405`). |
| `M2_M_LINK` | 58 | 63 | the pan/queue/claim mid-flight asserts (`:2406-2532`). |
| `M2_M_SCORED` | 61 | 66 | the 1-0 writer frame asserts (`:2533-2569`). |
| `M2_M_FINAL` | 72 | 77 | the post-goal phase-0 hold + the final exact `M2_WIRED_MASK` (`:2570-2623`). |

`M2_STAGE_ROWS = {0x04,0x06,0x07,0x08,0x0F,0x18,0x21,0x23,0x1E,0x00}` (`:1887-1888`).
The staged period is **1 s / 1 s** (`set_period(mr,1,1)`, `:2312`).

### 1.3 The transcript + the stop condition

Every step appends `frame=<presents> hash=<%016x>` (`:2684-2685`); while
`e->mode == MATCH` it additionally appends `state=<phase>/<home>-<away>`
(`:2669-2687`; format `state=%u/%u-%u`, `:2034-2036`). The run stops when
`restart_step` is found (phase 2, `screen_step == 4`, `corner_count[0] == 1`) and
`steps − restart_step >= M2_POST_RESTART_FRAMES` (20) (`:2926-2937`,
`:1603`). Present count == step count (`:2682`), so **transcript lines == engine
steps == 792** in the committed golden. The observed v20 state sequence
(fresh `awk` over the golden) is:

```
1-5    (no state; pre-match)
6-25   state=19/0-0   (0x13 forced first-half kickoff)
26-45  state=20/0-0   (0x14 forced second-half kickoff)
46-65  state=2/0-0    (phase 2: mechanics + band + goal link)
66-369 state=0/1-0    (post-goal phase-0 hold; the restart chain starts)
370-469 state=21/1-0  (0x15: the act-9 window; gate_step−phase15_step == 96)
470-772 state=4/1-0   (phase 4: the sit-3 taker window; 303 steps)
773-792 state=2/1-0   (restart completion + M2_POST_RESTART_FRAMES)
```

`main` pins `goal_step == match_start_step + 60` (`:3893`),
`gate_step − phase15_step == 96` (`:3916`), `phase4_step − phase15_step == 100`
(`:3921`), `restart_step − phase4_step == 303` (`:3924`),
`steps − restart_step == M2_POST_RESTART_FRAMES` (`:3927`).

### 1.4 Why no period ever completes on v20 (the extension's starting point)

The clock (`src/fifa96_loader/fifa96_match_state.c:47-92`) completes a period
iff `period_seconds == limit + aux_seconds` where `limit = period_length` for
`period < 2` else `extra_length` (`:63-65`, `:78`), then `period++`
(`:84-88`). `set_period(1,1)` (`fifa96_match_run.c:2425-2432`) makes the limit
1 s. **But** the match is only in a class-1 phase (the clock's clean runner) for
the 20-step phase-2 window (46-65); the phase-0 and phase-4 windows are
**class 2** (`fifa96_match_state.c:4-8`, `fifa96_match_action_handlers.c:1072`,
`:1076`), where the `cls == 2` aux block (`:66-72`) grows `aux_seconds` faster
than `period_seconds`, so the `== limit + aux` equality does not surface. Hence
the v20 golden carries no `state=12/…` (phase `0x0C`) and no per-step completion
— matching FU-215 §4.3 / `docs/ENGINE.md:1204` ("no period-end completion reaches
the roll's `global_157ac2 == 4` sit-8 gate"). The clock is nonetheless *running*
in the class-2 windows in the sense that `period_seconds`/`total_seconds` accrue.

---

## 2. The extension design

### 2.1 The two completions to play

| completion | `completed = period − 1` | route | expected chooser (fresh `fifa96_action_handlers.c:1163-1210`) |
|---|---|---|---|
| period 0 → 1 | 0 | **half-time** | `extra_time == 0` → `act 0xB`, `phase 0x0C` (`:1170-1178`) → the sit-1 kickoff (`period_setup` `fifa96_match_run.c:3205-3208`) → act 1 → phase `0x17` (FU-227, the half-time route) |
| period 1 → 2 | 1 | **regulation end → ET** | flag set iff **scores differ** (`fifa96_match_run.c:2618`) → `act 0xC`, `phase 0x13` (`:1180-1208`) → roll `= 2`; the index-4 sit-8 gate is reached only after the roll climbs to 4 (`:2654`) |

`fifa96_match_run_phase_drive` (`:2584-2688`) consumes the completion:
writes `out.phase`, dispatches `out.act`, rolls `global_157ac2 = state.period`
(`:2643`), and — **only on `global_157ac2 == 4`** — calls
`period_setup(mr, 0)` (`:2654-2657`) → situation 8 (RNG side) when
`state.phase == 0x0C && global_157ac2 == 4`, else situation 1 on the controlled
side (`:3199-3208`).

### 2.2 Directives/frames (preferred: append-only tail)

**Design.** Keep **every** v20 element byte-identical — `M2_KEYS` (all
`M2_KEYS_LEN` = 1800 slots; 154 explicit entries), the seven existing
directives, `M2_STAGE_ROWS`, `M2_M_*` indices, `M2_POST_RESTART_FRAMES`. Add:

1. **A completion window** replacing the single stop at
   `tests/test_engine_m2.c:2936-2937`: after the restart completes, keep running
   until the ET-entry anchor is seen (or a bound), instead of breaking at
   `restart_step + 20`. Suggested constants:
   `M2_POST_PERIOD_END_FRAMES` (the tail past the ET entry; e.g. the 20-post-goal
   convention) and a hard bound so the loop cannot run away;
   **raise `M2_STEP_CAP` (`:1568`) and `M2_TRANSCRIPT_CAP` (`:1569`) only if the
   measured length demands it** (T1 owns the number).
2. **Two new match-relative directives**, `M2_M_HALF_TIME` (the first
   post-restart completion) and `M2_M_ET_ENTRY` (the reg-end roll to the sit-8
   gate), placed at the **measured** `m` values (the T1 probe pins them; see
   §2.4). Each mirrors the `run_phase19_riders` block shape (`:3597-3660`):
   - `M2_M_HALF_TIME`: assert `global_157ac0 == 0u` (a first-half completion is
     not a tie-break site), `state.phase == 0x0Cu`, `act.id == 0x0Bu`,
     `global_157ac2 == 1u` (the roll), then the sit-1/act-1 arrival
     (`state.phase == 0x17u`).
   - `M2_M_ET_ENTRY`: assert `global_157ac0 == 1u` (the live flag),
     `global_157ac2 == 4u` (the roll to the ET gate), the sit-8 outcome
     `act.id == 7u`, and the phase-`0x1D` arm-0 phase write
     (`state.phase == 0x10u`; FU-215 §3.2 / FU-219). No engine change is implied
     (the riders already prove these pure-state paths).
3. **New `struct m2_result` fields** mirroring the existing step anchors
   (`:1890-1934`): `half_step`, `et_step`, `et_entry_step`, `et_phase`, and
   `steps`; `main` pins `half_step > restart_step` and
   `et_entry_step > et_step > half_step` (the phase-14 step-anchor idiom,
   `:3899-3901`).

**Frame picture (deterministic schedule, T1-confirmed).** After the restart at
frame 772 the run resumes at `state=2/1-0` with `period_length = 1`. A class-1
clock second is `FIFA96_MATCH_STATE_SECOND / (STEP>>8) = 0x3C / 2 = 30` granted
frames = **~90 engine steps** at the tape's 3-step grant cadence
(`fifa96_match_state.h:6-7`; grant cadence `tests/test_engine_m2.c:1728`). The
two completions therefore land on the order of `~90` steps apart once the clock
resumes, plus each period-end transition (the half-time/kickoff and the
sit-8→act-7→phase-`0x1D`→row-`0x20` ET entry).

### 2.3 Expected length range

`N ≈ 792 + W_half + W_et + W_tail`, where `W_half`/`W_et` are each one clock
completion (~90 steps) plus its period-end transition, and `W_tail` is the
post-ET-entry tail.

- **If the period-end transitions run on the derived/forced fast chain (the
  phase-2 kickoff path the tape already exercises):** estimate **N ≈ 1050–1600
  lines (v21)**.
- **If a transition enters the native act-1 kickoff cutscene** (the phase-`0x20`
  stage-1 wait is `0x3E8` act-timer units ≈ ~500 granted frames ≈ ~1500 steps;
  FU-203, cross-cited `tests/test_engine_m2.c:1862-1868`), **N can exceed 2000**,
  which would require raising `M2_STEP_CAP` (currently 2000, `:1568`) and
  `M2_TRANSCRIPT_CAP` (`:1569`).
- A hard **upper bound of `M2_STEP_CAP`** must be preserved as an assert
  (`:2939`); if the run does not self-terminate inside the cap, the design has
  failed and T1's "STOP and report" fallback applies (spec §5 T1).

**The exact `N` is owned by the T1 determinism probe** (spec §5 T1: "characterize
exactly what the run does now"). This draft fixes the *shape*, not the integer.

### 2.4 Additivity verdict

**The 792-line v20 prefix CAN stay byte-identical (additive) in the preferred
design**, because:

1. The extension appends frames **after** the current stop (frame 792); no
   pre-792 directive/key changes → frames 1..792 render identically. This is the
   **v13 → v14 precedent**: *"the transcript grows 85 -> 792 lines; the first 85
   lines are byte-identical"* (`tests/test_engine_m2.c:1180-1183`).
2. The null-backend `present_hash` is a per-step chain over the presented frames
   (v5 provenance, `:220-222`), so an identical prefix yields identical prefix
   hashes.
3. The HUD clock is `total_seconds` and the period label is `state.period + 1`
   (`fifa96_match_run.c:4367-4372`) — the clock text is **period-length
   independent**, so even a *changed* period length would not move the early HUD
   pixels; only the *period number* changes, and only at a completion.

**Additivity FAILS (a documented line-by-line re-pin, never silent) iff:**

- the plan changes the m 41 `set_period(1,1)` (`:2312`) to a shorter staged/live
  period (FU-223) such that the **first** completion lands at or before frame
  792 — then `state.period` and the HUD period label change inside the old
  prefix; or
- the plan drops the goal scenario (§3, Scenario Z) so frames from m 42 shift; or
- the plan must add an m-directive whose **side effects** (e.g. a
  `phase_machine`/`session_gate` write) alter the pre-792 head.

**Cross-slice dependency (explicit).** **FU-223 (the period-length producer) ↔
FU-230**: if T2 makes the live period length differ from the staged 1 s and the
tape stops forcing `set_period(1,1)`, the completion schedule moves and the
prefix may shift. **Mitigation:** keep the explicit `set_period(1,1)` staging on
the tape (so the tape's clock schedule is fixed) and prove the live-length
producer in a **unit fixture** (FU-223's deliverable). Record the chosen
resolution in the T6 provenance block.

### 2.5 Alternative: the spec-literal 0-0 tie (Scenario Z, non-additive)

If T6/T5 insists on a literal **0-0 tied regulation end**:

- **Omit the goal** — the m 42 band scenario / the m 58–m 72 goal chain stage the
  1-0 (`:2316-2623`). Skipping them keeps the score 0-0, but the directives from
  m 42 differ, so **frames from ~47 shift** (not additive).
- **A 0-0 reg-end does NOT reach extra time natively.** The tie-break
  (`fifa96_match_run.c:2618`) raises `global_157ac0` only on a **differing**
  score (W2 `0x8B2AC`; FU-207 §3.2); a tie keeps it clear → `act 0xB`,
  `phase 0x0C` (the no-ET arm; `run_phase19_riders` block 2 pins exactly this,
  `:3621-3636`). The only tie→ET path is the equal-score override
  `[0x14C2EE]` (W3 `0x8B2C1`), which is **unported** (FU213-L1). So Scenario Z
  reaches ET **only** by staging `[0x14C2EE] = 1` as a **disclosed stand-in**
  (the spec §7 risk-2 pattern) — a T3/T4 decision, not a tape fabrication.

**Recommendation.** Prefer the additive tail **with the existing 1-0 goal**
(the reg-end score *differs*, which is the native W2 ET arm). Scenario Z is a
larger, non-additive re-pin whose ET entry depends on an unported cell.

---

## 3. The re-pin specification

### 3.1 Written-reason wording (the AGENTS.md golden rule)

The re-pin must carry a **v21 provenance block** at the top of
`tests/test_engine_m2.c` (the v14 block at `:1154-1213` is the template) and a
matching `docs(fu230)` note. Required content, verbatim-in-substance:

> The M2 golden is re-pinned for phase-21 (the live period-end spine; FU-223…
> FU-230, T6). Reason: the tape now drives the live FU-62 clock through **two
> period completions** — period 0 → 1 (half-time) and period 1 → 2 (regulation
> end) — and the reg-end completion installs the OL-85 extra-time arm
> (`[0x157AC0]` W2 `0x8B2AC`, raised because the reg-end score differs). The
> acceptance stops at **extra-time entry** (spec §2 non-goal: no full ET match).
> Frame diff (measured with the ISO present): lines 1..792 byte-identical
> (`head -792 new | cmp - old` clean; `diff` empty); first appended line is 793;
> ... The intended live continuation, not a regression. Regenerated with the ISO
> present (`./build/test_engine_m2 > tests/golden/engine/m2-frames.txt`); M1
> unmoved (`09b726b7…`); old M2 `441415fc…` (v20, 792 lines) → new M2 `<sha>`
> (v21, `<N>` lines).

If **not** additive, the block must additionally enumerate the shifted region
line-by-line with the reason (spec §6: "the diff is documented line-by-line with
the reason — never silently").

### 3.2 Frame-diff methodology

1. **Preserve the old golden** before regenerating
   (e.g. `cp tests/golden/engine/m2-frames.txt /tmp/m2-v20.txt`).
2. Regenerate with the ISO present:
   `./build/test_engine_m2 > tests/golden/engine/m2-frames.txt`
   (`tests/test_engine_m2.c:29-30`).
3. **Prefix check (additivity):**
   `head -792 tests/golden/engine/m2-frames.txt | cmp - /tmp/m2-v20.txt` — must
   be clean for the additive design.
4. **First differing line:** `cmp /tmp/m2-v20.txt tests/golden/engine/m2-frames.txt`
   reports the first differing byte; convert to a line number with the
   `main`-style scan (`tests/test_engine_m2.c:3800-3807`).
5. **Classify the diff** into (a) **appended** lines (no old line changed), (b)
   **`state=`-changed** lines (a phase/score moved — the serious class), (c)
   **hash-only** lines (the render changed, the state did not — the v4.1/v4.2
   precedent, `:160-168`, `:186-195`). Report the counts:
   `diff <(cut -d' ' -f3- /tmp/m2-v20.txt) <(cut -d' ' -f3- new)` isolates
   `state=`; `diff <(cut -d' ' -f2 …)` isolates `hash=`.
6. **Hash-only vs `state=` rule** (the v5/v14 precedent): a hash-only difference
   is a render change and must be explained by a render-visible landing; a
   `state=` difference means phase/score behavior moved and must be explained by
   the period-end spine. Any *unexplained* difference of either kind is a
   **regression to investigate, never a re-pin** (v19/v20 wording).
7. **Determinism gate:** re-run the tape ≥ 3 times and `cmp` the transcripts
   byte-for-byte (spec §5 T1; the null backend is the determinism source of
   truth). If not byte-stable (the RNG sit-8 side, FU-226), **STOP** — the
   fallback is a fixture-only proof with M2 unchanged (spec §5 T1).

### 3.3 Mutation-RED rider classes T6 must add

Add a `run_phase21_riders()` (mirroring `run_phase19_riders`, `:3597-3660`),
run in `main` before the tape (`:3826-3832`), where **every rider is
mutation-RED** (each deliberately-broken mutation aborts at its own assert):

1. **Half-time transition (T5/FU-227):** the period-0 completion → the sit-1 arm
   → act 1 → phase `0x17`. Dropping the sit-1 dispatch (the `period_setup(mode
   0)` else-arm, `fifa96_match_run.c:3205-3208`) or reverting the chooser to the
   ET arm aborts `state.phase == 0x17`.
2. **Reg-end → ET arm (T3/FU-213, T4/FU-225):** a **differing** reg-end completion
   raises the flag and the chooser yields act `0xC`/phase `0x13`. Reverting the
   chooser's live flag to `0u` (or inverting the tie-break) aborts
   `global_157ac0 == 1` / `state.phase == 0x13` (extends `run_phase19_riders`
   block 1, `:3598-3619`).
3. **Tie does NOT arm ET (the polarity discriminator):** a **tied** reg-end
   leaves the flag clear → act `0xB`/phase `0x0C`. Inverting `differ` aborts
   `global_157ac0 == 0` (extends block 2, `:3621-3651`). **This is the rider that
   makes the brief's "0-0 tie → ET" premise executable as a test** — it fails if
   the premise is implemented as stated.
4. **Period-index roll (T3/FU-224 + T2/FU-208):** the roll `global_157ac2 =
   state.period` and the `== 4` sit-8 gate. Dropping the roll or gating off
   index 4 aborts `act.id == 7` (extends block 3, `:3653-3684`).
5. **ET-entry slot dereference (T2/T3/FU-209/FU-210/FU-217):** the phase-`0x1D`
   arm-0 phase write `0x10` + the sit-8 actor bind → the coupled row-`0x20`
   install. A NULLed slot or a dropped arm-0 write aborts `state.phase == 0x10`
   (extends block 4, `:3685-3716`).
6. **Clock-completion schedule (T1/FU-230):** the completion fires exactly on
   the `sec == limit + aux` equality (`fifa96_match_state.c:78`); a wrong
   `period_length`/`extra_length` (or a dropped `set_period`) aborts the
   `half_step`/`et_step` anchors.
7. **Additivity/determinism rider:** `head -792 new | cmp - old` clean (the
   prefix did not move) and 3 identical replays (byte-stable).

---

## 4. Handoff (the direct input to Stage-2 T6)

- **T6 implements:** the appended completion window + the two new directives
  (§2.2), the new `m2_result` anchors (§2.2.3), the v21 provenance block (§3.1),
  `run_phase21_riders` (§3.3), and the regenerated golden.
- **T1 pre-empts T6's unknowns:** it extends the *scripted input* and pins the
  completion frames (`half_step`/`et_step`), the exact `N`, the transition
  lengths, and byte-stability (spec §5 T1). **T6 must not invent those frames**
  — it takes them from T1's measurement.
- **T3/T4 decide the tie path:** whether the reg-end is a *differing* score (the
  additive v20 goal) or the *equal + `[0x14C2EE]` override* stand-in (§2.5).
- **T5 lands the two transitions** (half-time per FU-227, ET entry per
  FU-228).
- **Docs updated in the same commit** (spec §5 T6): `docs/ENGINE.md`,
  `docs/PROGRESS.md`, `README.md`, `AGENTS.md` (only if the census moves —
  this phase likely wires **zero** new rows; recompute
  `floor(100 × wired/80)` only if the live-dispatch status changes a row).

---

## 5. Cross-slice dependencies (explicit)

- **FU-223 (period length) ↔ FU-230 (tape):** see §2.4. If the tape's `set_period
  (1,1)` is removed/changed to a live length, the completion schedule and thus
  the prefix additivity change. Keep the explicit staging; prove the live-length
  producer by fixture.
- **FU-228 (post-period lifecycle) → FU-230:** the tape **stops at ET entry**
  (spec §2 non-goal); if the lifecycle's OVER→POST→EXIT mapping exits the run
  before the ET entry, the transcript would stop presenting match frames. T1
  must characterize this.
- **FU-226 (RNG determinism) → FU-230:** the sit-8 side `0x888C8` draws the RNG
  (`fifa96_match_run.c:3200-3203`); if the draw is not null-backend
  deterministic, the golden is not byte-stable → **STOP** (spec §5 T1).
- **FU-229 (whistle PCM) → FU-230:** if the `0x974DC(0x1E)` whistle touches the
  null-backend PCM hash, the appended frames' hashes change with the audio; FU-229
  owns the verdict.

---

## 6. Census / status impact

**None expected.** This phase activates already-counted rows; the census line
`55/80 = 68%` (`docs/ENGINE.md:9-11`) and the README title (`68%`) move only if
a row's live-dispatch status changes (spec §5 census caveat). The M1 golden
`09b726b7…` is untouched. M2 moves from `441415fc…` (792 lines, v20) to the v21
golden.

---

## 7. Residual legs (numbered; each with its owner)

1. **FU230-L1 — the exact completion frames and N are unmeasured.** The
   deterministic schedule (`period_seconds`, the class-2 aux growth in the
   phase-0/4 windows) is not resolved by static read; the T1 probe owns it.
   Owner: **spec §5 T1**.
2. **FU230-L2 — the tie→ET premise is contradicted by the native — and the
   reading is INHERITED.** The brief's "0-0 tie → extra-time entry" is
   inconsistent with the OL-85 writer W2 (`0x8B2AC`, *differ* only). **This
   reading is inherited / cross-cited from a single source, FU-207 §3.2** — the
   writer path `0x8B2A7 CMP AX,DX` / `0x8B2AA JZ` / `0x8B2AC MOV
   byte[0x157AC0],1` is re-read first-hand in FU-224 §3.1, but the
   `[0x157AC5]`/`[0x157AC7]` score-cell **identity and polarity** are still
   FU-207's, not independently corroborated here. **Stage-2 T1/T3 must confirm the
   `[0x157AC5]`/`[0x157AC7]` identity and polarity first-hand.** A literal tied /
   0-0 regulation end therefore reaches extra time **only** through the unported
   equal-score override `[0x14C2EE]` (W3 `0x8B2C1`; FU213-L1) staged as a
   disclosed stand-in, or the tape must keep a differing reg-end. **Recorded as an
   explicit spec-conflict erratum** (the spec's literal "tied / 0-0 → extra-time"
   is unreachable without the unported override), to be carried to Stage 2 — the
   spec is **not** edited here. Owner: **T3/T4 (FU-224/FU-225)** — must resolve
   before T6.
3. **FU230-L3 — additivity depends on the m 41 `set_period(1,1)` staying.**
   (Cross-slice FU223.) Owner: **T2 (FU-223) + T6**.
4. **FU230-L4 — `M2_STEP_CAP`/`M2_TRANSCRIPT_CAP` may need raising** if a
   transition enters the native kickoff cutscene. Owner: **T1/T6**.
5. **FU230-L5 — the exit-lifecycle interaction (FU-228).** Whether the live
   period-end leaves MATCH before the ET entry is unmeasured. Owner: **T5
   (FU-228) + T1**.
6. **FU230-L6 — the sit-8 RNG determinism (FU-226) and the whistle PCM
   (FU-229).** If either perturbs the null-backend hash, the golden cannot be
   re-pinned additively. Owner: **FU-226 / FU-229**.

---

## 8. Provenance (this slice)

Repo reads (first-hand, this session; no writes):

- `tests/test_engine_m2.c` — `:1-27` (the tape's purpose), `:1146-1203` (the
  step cadence + the v14 additive re-pin precedent), `:1566-1569`
  (`M2_ISO_PATH`/`M2_GOLDEN_PATH`/`M2_STEP_CAP`/`M2_TRANSCRIPT_CAP`),
  `:1571-1592` (the directive index macros), `:1603`
  (`M2_POST_RESTART_FRAMES`), `:1620-1673` (`M2_WIRED_MASK`/taker mask),
  `:1707-1877` (`M2_KEYS` + `M2_KEYS_LEN`), `:1879-1888` (`M2_STAGE_ROWS`),
  `:1890-1934` (`struct m2_result`), `:2027-2036` (`append`/`append_state`),
  `:2098-2624` (`m2_directives`), `:2626-2949` (`run_tape` + the stop),
  `:3597-3716` (`run_phase19_riders`), `:3800-3807` (the diff-line reporter),
  `:3810-3927` (`main`'s anchors).
- `tests/golden/engine/m2-frames.txt` — the 792-line v20 state sequence (fresh
  `awk`; sha `441415fc…`).
- `src/fifa96_engine/fifa96_match_run.c` — `:2366-2372` (begin's
  `set_period`), `:2425-2432` (`set_period`), `:2548-2688` (`phase_drive`; the
  tie-break `:2618`, the roll `:2643`, the index-4 gate `:2654`),
  `:3147-3220` (`set_piece` sit 2/3/4 + the sit-8 bind + `period_setup`),
  `:4338-4372` (the HUD clock/period text).
- `src/fifa96_loader/fifa96_match_state.c` — `:4-8` (the class table), `:47-92`
  (the clock; the completion `:78`, `period++` `:84-88`).
- `include/fifa96_loader/fifa96_match_state.h` — `:6-7`
  (`FIFA96_MATCH_STATE_STEP 0x200` / `FIFA96_MATCH_STATE_SECOND 0x3C`).
- `src/fifa96_loader/fifa96_action_handlers.c` — `:1071-1107` (the phase-row
  clock classes), `:1163-1210` (the `FUN_0008B9CC` chooser).
- `docs/ENGINE.md` — `:9-11` (the current status/census), `:1198-1213`
  (the phase-19 acceptance + "a differing-score regulation-end completion
  installs the extra-time arm"), `:1495-1542` (the phase-20 v20 strata).
- `docs/superpowers/specs/2026-10-13-fifa96-m2-phase21-live-period-end-design.md`
  — §1 (`period_setup_live`), §3 (the live-spine diagram), §5 (T1/T6), §6 (the
  re-pin policy).
- Frozen FU docs (cross-cited): **FU-207** §1/§2/§3.2 (the OL-85 flag; W2
  `0x8B2AC` differ, W3 `0x8B2C1` equal+override), **FU-213** (the extra-time
  clock), **FU-214** §8 (the phase-19 acceptance), **FU-215** §3.2/§4.3 (the
  sit-8 producer B; the M2 tape arms no sit-8).

## Landing note

This slice is **design only**: it fixes the tape format, the extension shape
(append-only preferred), the length model, the additivity conditions, the re-pin
wording/diff methodology, and the seven mutation-RED rider classes — and it
records the one contradiction the brief carries (the **tie→ET polarity**) as
**FU230-L2**, to be resolved by T3/T4 before T6 re-pins. No engine change, no
test change, no golden change, no census move.
