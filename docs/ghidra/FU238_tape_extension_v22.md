# FU-238 — the M2 tape extension + re-pin (phase-22 wave, slice W22-8)

Provenance: M2 phase-22 **Stage-1 recon-ahead**, 2026-10-13, **read-only**. This
slice is a **design/spec slice** — it writes no code and no test; it
pre-specifies the acceptance-tape extension (v21 → v22) and the re-pin that the
Stage-2 producer plan's **T6** implements (spec
`docs/superpowers/specs/2026-10-13-fifa96-m2-phase22-end-spine-design.md` §5).
Every claim below cites a **fresh first-hand read** of the repo tree at this
session (`file:line`); native facts are cross-cited to frozen FU docs (FU-228
§2, FU-230). Engine source read first-hand; `tests/test_engine_m2.c` read
first-hand. **No writes**: `tests/`, `src/`, `include/`, `docs/` untouched;
`tests/golden/engine/m1-frames.txt` (`09b726b7…`, 691 lines, **immutable**) and
`tests/golden/engine/m2-frames.txt` (`0b87fd18…`, **2522 lines, v21**) untouched.

Scope (W22-8 brief): (1) read the v21 tape generator (the `M2_M_*` directives,
the caps, the `M2_M_ET_ENTRY` anchor, the completion window); (2) design the
extension that plays the **ET halves** + the **end terminal** + the
**match-over screen** + (D2) the **return to the front-end**; (3) specify the
re-pin (written reason, frame-diff methodology, mutation-RED rider classes);
(4) hand off to Stage-2 T6. This slice extends **FU-230** (the v21 extension
precedent, phase-21 W21-8), FU-228 (the post-period lifecycle + end word),
FU-227 (the half-time route), FU-213/FU-225 (the period-end chooser), and the
phase-22 sibling slices FU-231…FU-237 (not yet frozen; ET-native facts below are
cited to FU-228 / the spec and marked as their owners' legs).

---

## Answer in one line

The v21 tape is a **2522-line, byte-deterministic transcript** that stops
`M2_POST_PERIOD_END_FRAMES` (20) past the **extra-time entry** (the differing
reg-end completion → the ET arm `phase 0x13` / `act 0xC` at engine step 2503)
(`tests/test_engine_m2.c:2771-2785`, `:3105-3115`); **the extension is
append-only in the preferred design** — keep the **2522-line v21 prefix
byte-identical** and let the resumed ET clock play **both extra-time halves** to
the **ET-over terminal** (end word `2`, `FU228 §2.2`), then the **match-over /
ET-over screen**, then the **return to the front-end** (the `run_end` mode flip
`fifa96_match_run.c:4241`). The tape asserts that chain at four new match-relative
directives and stops `M2_POST_RETURN_FRAMES` past the front-end return. The
cost is **one additive re-pin** (the v21 `0b87fd18…` prefix stays byte-identical;
new frames append), **two mandatory in-test edits** (the hardcoded `792`-line
additivity gate `:4561-4587` becomes `2522`; the `exit_step < 0` assert `:4349`
flips to `> 0`), and **a raised `M2_STEP_CAP`** (the two ET halves + the screen
extend past 4000; the measured `N` is T1's). The one thing the tape can **not**
exercise is the **word-`1` regulation-over route** (the tape's reg-end arms ET),
which stays FU-236's fixture.

---

## 1. The tape generator at v21 (fresh)

### 1.1 The scripted input is a sized key tape (unchanged since v20)

`run_scenario` builds a `fifa96_platform_null_config` with `pcfg.tape = M2_KEYS`
and `step_ns = 10000000` (one 100 Hz PIT tick per engine step), then steps the
engine `while (steps < M2_STEP_CAP)` (`tests/test_engine_m2.c:2798-2831`).
`M2_KEYS` is an **unsized designated-initializer `fifa96_platform_key` array**
(`:1809-1978`); `M2_KEYS_LEN = sizeof M2_KEYS / sizeof M2_KEYS[0]` (`:1979`).
The highest explicit index is 1799 (`:1975-1977`), so **`M2_KEYS_LEN = 1800`**;
`n_poll` returns `*count = 0` once `tape_pos >= tape_len`
(`src/fifa96_engine/platform_null.c:29-33`), so from engine step **1801** the
tape supplies **zero keys** — the whole ET-half / terminal / screen / return
span runs directive- and clock-driven, keyless. Entry index = engine step − 1
(`:1146-1149`).

### 1.2 The v21 caps and the completion window

| define | value | line | meaning |
|---|---|---|---|
| `M2_STEP_CAP` | `4000` | `:1647` | the hard loop bound |
| `M2_TRANSCRIPT_CAP` | `(1u << 20)` | `:1648` | the transcript buffer |
| `M2_POST_PERIOD_END_FRAMES` | `20` | `:1705` | the v21 tail past the ET-entry anchor |

The loop stops when `res->et_entry_step > 0 && steps - et_entry_step >=
M2_POST_PERIOD_END_FRAMES` (`:3112-3115`); the post-loop `assert(steps <
M2_STEP_CAP)` reports a runaway (`:3117`).

### 1.3 The seven v20 directives + the three v21 directives

`m2_directives(e, next, match_start_step, res)` runs at the top of engine step
`next`, with `m = next − match_start_step`; `match_start_step == 5` on both
modes (`:4341`). The v20 seven (bodies at `:2220-2742`):

| macro | m | step | what it forces / asserts |
|---|---|---|---|
| `M2_M_KICKOFF` | 1 | 6 | force `phase = 0x13`; the one state-1 taker + human slot bind (`:1665`, `:2222`) |
| `M2_M_HALF` | 21 | 26 | force `phase = 0x14` (`:1666`, `:2266`) |
| `M2_M_MECH` | 41 | 46 | arms 0x26/0x28/0x2A; `set_phase(2)`; **`set_period(mr,1,1)`**; stage `M2_STAGE_ROWS` (`:1667`, `:2269-2436`) |
| `M2_M_BAND` | 42 | 47 | the goal-mouth scenario staging (`:1668`, `:2436`) |
| `M2_M_LINK` | 58 | 63 | the pan/queue/claim mid-flight asserts (`:1669`, `:2526`) |
| `M2_M_SCORED` | 61 | 66 | the 1-0 writer-frame asserts (`:1670`, `:2653`) |
| `M2_M_FINAL` | 72 | 77 | the post-goal phase-0 hold + the exact `M2_WIRED_MASK` (`:1671`, `:2690`) |

The three v21 directives (`:1684-1686`, bodies `:2743-2785`):

| macro | m | step | what it asserts |
|---|---|---|---|
| `M2_M_HALF_OPEN` | 841 | 846 | post-period-0-completion frame; asserts `global_157ac2 == 1`, `global_157ac0 == 0`, `phase == 0x0C`, `act.id == 0xB`; **opens `session_gate_14c32a = 0`** (`:2743-2759`) |
| `M2_M_HALF_TIME` | 845 | 850 | the sit-1 half-time kickoff arrival; asserts `phase == 1` (kickoff), `act.id == 1`; sets `res->half_step` (`:2760-2770`) |
| `M2_M_ET_ENTRY` | 2498 | 2503 | the differing reg-end completion; asserts `global_157ac0 == 1`, `global_157ac2 == 2`, `phase == 0x13`, `act.id == 0xC`; sets `et_step`/`et_entry_step`/`et_phase` (`:2771-2785`) |

### 1.4 The measured v21 state sequence (fresh `awk` over the golden)

```
1-5     (no state; pre-match)
6-25    state=19/0-0    forced first-half kickoff (phase 0x13)
26-45   state=20/0-0    forced second-half kickoff (phase 0x14)
46-65   state=2/0-0     phase 2: mechanics + band + goal link
66-369  state=0/1-0     post-goal phase-0 hold
370-469 state=21/1-0    phase 0x15: the act-9 window
470-772 state=4/1-0     phase 4: the sit-3 corner taker window
773-845 state=2/1-0     resumed phase-2 play  -> period-0 completion (step 845)
846-849 state=12/1-0    phase 0x0C: the half-time chooser arm
850-1802 state=1/1-0    phase 1: the sit-1 half-time kickoff (953 steps)
1803-2502 state=2/1-0   resumed phase-2 play -> period-1 completion (step 2502)
2503-2522 state=19/1-0  phase 0x13: the extra-time chooser arm (ET entry; the v21 STOP + 20)
```

`main` pins the anchors (`:4416-4423`): `half_step > restart_step`,
`et_entry_step > half_step`, `et_step == et_entry_step + 1`, `et_phase == 0x13`,
`steps − et_entry_step == M2_POST_PERIOD_END_FRAMES`.

### 1.5 The additivity gate (the code T6 must move)

The v21 re-pin grew the golden 792 → 2522 lines **by appending only**, and the
in-test gate `head -792 new | cmp - old` is a hardcoded **792** at
`:4561-4587` (the prefix byte count `34820` appears twice: in the provenance
prose, `:1584-1585`, **and** in the gate's own comment, `:4566`). **For v22 this
constant must become the v21 prefix length
(2522).** The gate edits no golden byte — it is the automated reproducibility of
the `head | cmp` methodology.

### 1.6 Why no ET frame exists on v21 (the extension's starting point)

`fifa96_match_run_set_period(mr, 1, 1)` sets `period_length = extra_length = 1 s`
(`src/fifa96_engine/fifa96_match_run.c:2425-2432`). The FU-62 clock completes a
period iff `period_seconds == limit + aux_seconds`, with `limit = period_length`
for `period < 2` else `extra_length` (`src/fifa96_loader/fifa96_match_state.c:63-65`,
`:77-88`), and `aux_seconds` is grown in the class-2 windows (`:66-72`). The
phase class table (`:4-8`) makes `phase 0x13` and `0x14` **class 0** (the clock
is halted) and `phase 2` class 1 (the clock runs) — so a **1 s period at the
tape's cadence is *not* ~90 steps**: the measured play windows are 73 steps
(773-845) and 700 steps (1803-2502), because `aux_seconds` carried from the
earlier class-2 windows enters the `== limit + aux` equality. **The ET-half
completion schedule is therefore not statically derivable — T1 measures it**
(the FU230-L1 precedent).

---

## 2. The extension design

### 2.1 The four stages to play (spec D2 / §6)

| stage | native | tape assertion |
|---|---|---|
| **ET halves** (periods 2/3 at `extra_length`) | the FU-62 clock rolls `2 → 3 → 4`; the ET-first-half phase `0x13`, the ET-second-half phase `0x14` (FU-233) | the ET-second-half arrival (`state.period == 3`, the ET-half phase) |
| **end terminal** | `FUN_0008BAF0` writes `[0x158822] = 2` on the ET-half terminal (`0x8BD8A` phase `0x14`; `0x8BDD4` phase `0x13` + equal scores); `[0x10F360]` latch (FU-228 §2.2) | the **ET-over** word `2` |
| **match-over screen** | the frame loop consumes `[0x158822]`: `AX==2` → the ET-over screen; `AX==1` + index `>= 2` → `mark_over` (`0x4B244`, `[0x5FFC]=2`); index `< 2` → the half-time block (FU-228 §2.3, FU-232) | the lifecycle `screen == OVER` |
| **return to the front-end** | the lifecycle `OVER → POST → EXIT → run_end`; `run_end` sets `eng->mode = FRONTEND` (`fifa96_match_run.c:4195-4244`, esp. `:4241`) | `mode != MATCH` (the first non-live frame) |

> **Terminal route (measured leg).** The tape keeps the v20 **1-0** score
> (differing). Per FU-228 §2.2 the phase-`0x13` terminal **holds on differing
> scores** (`0x8BD95 CMP scores; JNZ tail`) and only the phase-`0x14` terminal
> writes `[0x158822] = 2` unconditionally (`0x8BD8A`). So the tape must play
> **both** ET halves and reach the **phase-`0x14`** terminal. The exact ET-half
> phases and the terminal frame are FU-231/FU-233/FU-237's to confirm; the tape
> **records** the terminal phase rather than assuming it (§8 legs).

### 2.2 The directive/frame design (preferred: append-only tail)

**Keep byte-identical:** `M2_KEYS` (all 1800 slots; 154 explicit entries), the
seven v20 directives, `M2_STAGE_ROWS`, `M2_M_*` indices, `M2_M_HALF_OPEN` /
`M2_M_HALF_TIME` / `M2_M_ET_ENTRY`, `M2_POST_RESTART_FRAMES`. **Add:**

1. **Four new match-relative directives**, placed at the **measured** `m` values
   (the T1 probe pins them; §2.4). Each mirrors the `run_phase21_riders` block
   shape / the v21 `M2_M_HALF_TIME` body:
   - **`M2_M_ET_HALF2`** — the ET **second-half** arrival (the ET-half phase
     `0x14`). Assert `state.period == 3`, `global_157ac2 == 3`,
     `state.phase == 0x14`, `act.id` at the ET-half handler; record
     `res->et_half2_step`.
   - **`M2_M_ET_TERMINAL`** — the **ET-over terminal**. Assert the end word
     `mr->end_word_158822 == 2` (the T2 cell / FU-231) and record the terminal
     phase + step (`res->et_terminal_step`, `res->et_terminal_phase`). The
     differing-score tape uses the phase-`0x14` write (`0x8BD8A`).
   - **`M2_M_ET_OVER`** — the **match-over screen**. Assert the lifecycle reached
     `screen == FIFA96_MATCH_SCREEN_OVER` (`[0x5FFC]=2`, FU-232/FU-235; the
     `AX==2` ET-over path) and record `res->et_over_step`.
   - **`M2_M_RETURN`** — the **front-end return**. Assert the run left `MATCH`
     (`e->mode != FIFA96_ENGINE_MODE_MATCH` / `res->exit_step > 0`) and record
     `res->return_step`.
2. **A return window replacing the v21 stop** at `:3105-3115`. Replace the
   `M2_POST_PERIOD_END_FRAMES`-past-ET-entry break with a break
   `M2_POST_RETURN_FRAMES` (e.g. 20, the `M2_POST_RESTART_FRAMES` convention)
   past the front-end return, plus the existing hard `M2_STEP_CAP` bound and the
   post-loop `assert(steps < M2_STEP_CAP)`. The v21 break is *removed*, not
   extended — frames 2503-2522 are still emitted identically, so the prefix is
   unaffected (§2.5).
3. **New `struct m2_result` fields**, mirroring the v21 period-end anchors
   (`:2037-2044`): `uint64_t et_half2_step, et_terminal_step, et_over_step,
   return_step;` and `uint8_t et_terminal_phase, end_word_terminal;`. `main`
   pins `et_half2_step > et_entry_step`, `et_terminal_step > et_half2_step`,
   `et_over_step >= et_terminal_step`, `return_step > et_over_step`.
4. **Raise the caps only as measured** (FU230-L4 precedent): the ET halves + the
   screen extend past `M2_STEP_CAP = 4000`; propose `M2_STEP_CAP ≈ 12000`
   (headroom) pending T1's `N`; `M2_TRANSCRIPT_CAP = 1u << 20` is likely ample
   (the v21 2522 lines ≈ 110 KiB; even 8000 lines ≈ 350 KiB).

### 2.3 The frame picture (deterministic schedule, T1-confirmed)

After the ET entry (phase `0x13` at frame 2503) the clock is **halted** (phase
`0x13` is class 0) until the ET-half play reaches a class-1 phase (`phase 2`).
The ET halves then each complete like the regulation halves: a class-1 play
window (~90 steps of pure clock, **plus** the carried `aux_seconds` — the v21
period-1 window was 700 steps) bounded by the class-0 phase-`0x13`/`0x14`
kickoff phases. The terminal write, the `[0x10F360]` latch, the
`word[0x158818] > 0x78` screen gate / the `0x960` camera hold (FU-235), and the
`OVER → POST → EXIT` resolve add a bounded tail. **Pattern:**
`2522 → [ET first half] → [ET second half] → [terminal + screen] → [return + tail]`.

### 2.4 Expected length range

`N ≈ 2522 + W_et1 + W_et2 + W_term + W_screen + W_tail`, where each `W_et` is one
ET-half window (bounded below by the ~90-step clock and above by a possible
class-0 kickoff cutscene like the 953-step half-time window, plus carried
`aux_seconds`).

- **Central estimate: N ≈ 3000–4500 lines (v22).**
- **Worst case** (both ET halves enter a half-time-length `phase-1` kickoff
  cutscene): **N ≈ 5500–6500**, which still fits `M2_STEP_CAP = 12000`.
- A hard upper bound of `M2_STEP_CAP` is preserved as an assert; if the run does
  not self-terminate inside the cap, the design has failed and T1's
  "STOP and report" fallback applies (spec §5 T1).

**The exact `N` is owned by the T1 determinism probe** (spec §5 T1). This draft
fixes the *shape*, not the integer (the FU230-L1 precedent).

### 2.5 Additivity verdict

**The 2522-line v21 prefix CAN stay byte-identical (additive) in the preferred
design**, because:

1. The extension **appends frames after** the current stop (2522); no
   pre-2522 directive/key/period change → frames 1..2522 render identically
   (the v13→v14 and v20→v21 precedent, `:1180-1188`, `:1583-1593`).
2. The null-backend `present_hash` is a **cumulative FNV chain over the
   presented frames** (`platform_null.c:23-46`): the printed hash at line `k`
   is a function of frames 1..`k` only, so appending frames leaves every prefix
   hash unchanged.
3. `M2_KEYS` is unchanged and the ET windows consume **zero keys** (tape_pos
   `>= 1800` from step 1801, `platform_null.c:29-33`), so no key side effect can
   reach back into the prefix.

**Additivity FAILS (a documented line-by-line re-pin, never silent) iff:**

- T2/T3's end-word / lifecycle re-point changes when `mark_over` fires **before
  frame 2522**. The v21 gate already excludes both the half-time completion
  (roll 1 `< 2`) and the reg-end (flag set → `end_word_decided` false)
  (`fifa96_match_run.c:4183-4190`); the T3 re-point must keep the **word-0
  half-time and reg-end** from overs (the §3.3 riders 1-3 enforce this); or
- a **new directive before m 2522** is required (e.g. a session-gate switch for
  the ET-half route) whose side effect moves a pre-2522 frame; or
- the ET-half clock (`extra_length`) is changed such that a completion lands
  **before** frame 2522 — impossible while `set_period(1,1)` at m 41 stands
  (`:2312`), but a risk if the live-length producer (FU-223) replaces it.

**Cross-slice dependency (explicit).** **FU-223 (period length) ↔ FU-238**: keep
the explicit `set_period(1,1)` staging on the tape so the completion schedule is
fixed; prove the live-length producer by fixture (FU-223's deliverable). Record
the resolution in the T6 provenance block (the FU230 §2.4 cross-dep).

### 2.6 The one route the tape cannot exercise (word `1`)

The tape's reg-end is **differing** → the OL-85 flag set → the **ET arm**
(FU-228 §2.2, `0x8BCC8`), never the word-`1` regulation-over (`0x8BCD9`). The
`word == 1` route (flag-clear reg-end past half-time → `mark_over`) is therefore
**off the v22 tape** and stays **FU-236's** rider/fixture (`run_phase22_riders`
block 1, §3.3). This is a scope statement, not a gap: the tape proves the
**word-2 ET-over** end, which is the phase theme (spec §2).

---

## 3. The re-pin specification

### 3.1 Written-reason wording (the AGENTS.md golden rule)

The re-pin carries a **v22 provenance block** at the top of `tests/test_engine_m2.c`
(the v21 block at `:1545-1618` is the template) and a matching `docs(fu238)`
note. Required content, verbatim-in-substance:

> The M2 golden is re-pinned for phase-22 (the end spine; FU-231…FU-238, T6).
> Reason: the tape now drives the extra-time halves (the FU-62 clock periods 2/3
> at `extra_length`) to the end-of-match terminal (the end word `[0x158822] = 2`,
> FU-228 §2.2), the match-over / ET-over screen, and the return to the front-end
> (spec §2 Goal / D2). The v21 acceptance stopped at extra-time entry; the phase
> theme is the full end spine. Frame diff (measured with the ISO present): lines
> 1..2522 byte-identical (`head -2522 new | cmp - old` clean; `diff` empty);
> first appended line is 2523; ... The intended live continuation, not a
> regression. Regenerated with the ISO present
> (`./build/test_engine_m2 > tests/golden/engine/m2-frames.txt`); M1 unmoved
> (`09b726b7…`); old M2 `0b87fd18ad41948c8294a6cc46e40af02c8bfe5e92d708e09c3a1a2103c947c0`
> (v21, 2522 lines) → new M2 `<sha>` (v22, `<N>` lines).

If **not** additive, the block must additionally enumerate the shifted region
line-by-line with the reason (spec §6: "the diff is documented line-by-line with
the reason — never silently").

### 3.2 Frame-diff methodology

1. **Preserve the old golden** before regenerating:
   `cp tests/golden/engine/m2-frames.txt /tmp/m2-v21.txt` (sha `0b87fd18…`).
2. Regenerate with the ISO present:
   `./build/test_engine_m2 > tests/golden/engine/m2-frames.txt` (`:29-30`).
3. **Prefix check (additivity):**
   `head -2522 tests/golden/engine/m2-frames.txt | cmp - /tmp/m2-v21.txt` — must
   be clean (34820 bytes is the v20 prefix; the v21 prefix is
   `wc -c /tmp/m2-v21.txt`).
4. **First differing line:** `cmp /tmp/m2-v21.txt tests/golden/engine/m2-frames.txt`
   reports the first differing byte; convert to a line number (the
   `report_diff` scan, `:4252-4269`).
5. **Classify the diff** into (a) **appended** lines (no old line changed), (b)
   **`state=`-changed** lines (a phase/score moved — the serious class), (c)
   **hash-only** lines (the render changed, the state did not).
   `diff <(cut -d' ' -f3- /tmp/m2-v21.txt) <(cut -d' ' -f3- new)` isolates the
   `state=` column; `diff <(cut -d' ' -f2 …)` isolates `hash=`.
6. **Hash-only vs `state=` rule** (the v5/v14/v21 precedent): a hash-only
   difference is a render change and must be explained by a render-visible
   landing; a `state=` difference means phase/score behavior moved and must be
   explained by the end spine. Any *unexplained* difference of either kind is a
   **regression to investigate, never a re-pin**.
7. **Determinism gate:** re-run the tape ≥ 3 times and `cmp` the transcripts
   byte-for-byte (spec §5 T1; `run_scenario_capture`, the `main` gate at
   `:4306-4320`). If not byte-stable (the ET-half RNG, FU-226), **STOP** — the
   fallback is a fixture-only proof with M2 unchanged.

### 3.3 Mutation-RED rider classes T6 must add

Add `run_phase22_riders()` (mirroring `run_phase21_riders`, `:4090-4250`), run in
`main` before the tape (beside `:4294`), where **every rider is mutation-RED**:

1. **Regulation-over end word (T2/FU-231 + FU-236, word `1`):** the phase-`0x0C`
   flag-clear terminal writes `[0x158822] = 1`; reverting the decision (or
   inverting the flag gate) leaves the word `0`. **This is the off-tape route
   (§2.6)** — a fixture, since the tape's reg-end is flag-set.
2. **ET-over terminal (T2/T4/FU-231/FU-233/FU-237, word `2`):** the phase-`0x14`
   terminal writes `[0x158822] = 2` (+ the `[0x10F360]` latch; the phase-`0x13`
   equal-score test). Dropping the terminal write (or the latch) leaves the word
   `0` and the run never overs — aborts `end_word_158822 == 2`.
3. **Frame-loop consumer (T3/FU-232):** `mark_over`/the ET-over screen only when
   the word is set **and** the period index `>= 2`; the index `< 2` half-time
   block must NOT over. Inverting the `CMP [0x157AC2],2` compare overs at the
   half-time completion (roll 1) — `:4405`-style anchor shift.
4. **ET halves live (T4/FU-233):** the ET-half clock (periods 2/3 at
   `extra_length`) + the phase rows. A wrong `extra_length` (or a dropped ET-half
   completion) moves `et_half2_step` / `et_terminal_step`.
5. **Front-end return (T5/FU-234):** `run_end` sets `eng->mode = FRONTEND`
   (`fifa96_match_run.c:4241`). Dropping the mode flip leaves `mode == MATCH`
   after the terminal, so `return_step` is never sampled (the run would hit the
   cap).
6. **Screen timing gate (T5/FU-235, if modelled per spec D1):** the
   `word[0x158818] > 0x78` hold. A dropped/relaxed gate changes `et_over_step`.
7. **Additivity/determinism rider:** `head -2522 new | cmp - old` clean (the v21
   prefix did not move) and 3 identical replays (byte-stable).

### 3.4 The required code-edit inventory (T6)

| edit | location | from → to |
|---|---|---|
| the additivity gate line count | `tests/test_engine_m2.c:4561-4587` | `lines < 792` / `assert(lines == 792)` → `2522` |
| the exit assertion | `:4349` | `assert(res.exit_step < 0)` → `assert(res.exit_step > 0)` |
| the v21 stop | `:3105-3115` | `M2_POST_PERIOD_END_FRAMES` break → `M2_POST_RETURN_FRAMES` past the return |
| the caps | `:1647-1648` | `M2_STEP_CAP 4000` → measured (≈12000) |
| new directives + `m2_result` fields + `run_phase22_riders` | `:1684-1686`, `:2037-2044`, `:4090+` | added |
| the provenance block + `main` anchors | `:1545-1618`, `:4416-4544` | v22 block + `et_half2`/`et_terminal`/`et_over`/`return` anchors; the `state=` strata gain the ET lines |

> The `M2_M_ET_ENTRY` directive and its anchors stay; it is no longer the stop,
> only an intermediate anchor.

---

## 4. Handoff (the direct input to Stage-2 T6)

- **T6 implements:** the appended return window + the four new directives
  (§2.2), the new `m2_result` anchors (§2.2.3), the v22 provenance block (§3.1),
  `run_phase22_riders` (§3.3), the §3.4 edit inventory, and the regenerated
  golden.
- **T1 pre-empts T6's unknowns:** it extends the *scripted input* and pins the
  ET-half frames (`et_half2_step`), the terminal (`et_terminal_step`,
  `et_terminal_phase`), the screen (`et_over_step`), the return
  (`return_step`), the exact `N`, and byte-stability. **T6 must not invent those
  frames** — it takes them from T1's measurement (the FU230-L1 precedent).
- **T2 lands the end word + terminal** (FU-231/FU-236): the `mr->end_word_158822`
  cell and the minimal `FUN_0008BAF0` decision (spec §9).
- **T3 re-points the lifecycle** (FU-232): `mark_over` iff the end word is set
  **and** the index `>= 2`; the ET-over screen path.
- **T4 lands the ET halves** (FU-233): the clock periods 2/3 + the phase-`0x13`/
  `0x14` rows.
- **T5 lands the screen + return** (FU-234/FU-235).
- **Docs updated in the same commit** (spec §5 T6): `docs/ENGINE.md`,
  `docs/PROGRESS.md`, `README.md`, `AGENTS.md` (only if the census moves — this
  phase likely wires **zero** new rows; recompute `floor(100 × wired/80)` only
  if a row's live-dispatch status changes).

---

## 5. Cross-slice dependencies (explicit)

- **FU-223 (period length) ↔ FU-238:** see §2.5 — keep the explicit
  `set_period(1,1)`; prove the live-length producer by fixture.
- **FU-231/FU-233/FU-237 (the terminal) → FU-238:** the tape asserts the
  **word-2** terminal; the exact ET-half phases and the terminal frame are the
  terminal slices' to confirm. The differing-score path uses the phase-`0x14`
  write (FU-228 §2.2).
- **FU-232 (the consumer) → FU-238:** the tape asserts the lifecycle `OVER`; the
  `AX==1` (mark_over) vs `AX==2` (ET-over) distinction is FU-232's.
- **FU-234 (the return) → FU-238:** the tape asserts `mode != MATCH`; whether the
  front-end transition is cleanly pinnable is FU-234's (spec §10 D2 leaves the
  stop-short option).
- **FU-226 (RNG determinism) → FU-238:** if the ET-half RNG perturbs the
  null-backend hash, the golden cannot be re-pinned additively → **STOP**.
- **FU-229 (whistle PCM) → FU-238:** if the end whistle touches the
  null-backend PCM hash, the appended frames' hashes change with the audio.

---

## 6. Census / status impact

**None expected.** This phase activates already-counted rows; the census line
`55/80 = 68%` (`docs/ENGINE.md:9-11`) and the README title (`68%`) move only if
a row's live-dispatch status changes (spec §5 census caveat). The M1 golden
`09b726b7…` is untouched. M2 moves from `0b87fd18…` (2522 lines, v21) to the v22
golden.

---

## 7. Residual legs (numbered; each with its owner)

1. **FU238-L1 — the exact ET-half frames and N are unmeasured.** The ET-half
   completion schedule depends on the carried `aux_seconds` and the class-0
   kickoff windows (not statically derivable, §1.6); T1 owns it. Owner:
   **spec §5 T1**.
2. **FU238-L2 — the ET-half phases and the terminal phase are not independently
   read here.** That the ET halves use phases `0x13`/`0x14` and that the
   differing-score terminal is the phase-`0x14` write (`0x8BD8A`) is
   **cross-cited from FU-228 §2.2**; the sibling slices **FU-231/FU-233/FU-237**
   must confirm first-hand. Owner: **FU-231/FU-233/FU-237** — resolve before T6.
3. **FU238-L3 — the word-`1` regulation-over route is off the tape.** The tape's
   reg-end arms ET; the `mark_over`/`word 1` route stays FU-236's fixture
   (§2.6). Owner: **T3/T6 (FU-236)**.
4. **FU238-L4 — `M2_STEP_CAP` likely needs raising** (≈12000) if the ET halves
   enter kickoff cutscenes. Owner: **T1/T6**.
5. **FU238-L5 — additivity depends on the T2/T3 lifecycle re-point not overs
   before frame 2522** and on the m 41 `set_period(1,1)` staying. Owner:
   **T2/T3 (FU-232/FU-231) + T6**.
6. **FU238-L6 — the front-end return's clean pinnability** (spec §10 D2): if
   FU-234 proves the transition cannot be cleanly pinned, the tape stops at the
   match-over screen instead. Owner: **T5 (FU-234) + T6**.
7. **FU238-L7 — the ET-half RNG (FU-226) and the end-whistle PCM (FU-229).** If
   either perturbs the null-backend hash, the golden cannot be re-pinned
   additively. Owner: **FU-226 / FU-229**.

---

## 8. Provenance (this slice)

Repo reads (first-hand, this session; no writes):

- `tests/test_engine_m2.c` — `:1146-1215` (the step cadence + the v14 re-pin
  template), `:1545-1618` (the v21 provenance block), `:1641-1720`
  (`M2_ISO_PATH`/`M2_STEP_CAP`/`M2_TRANSCRIPT_CAP`/the v21 directives /
  `M2_POST_PERIOD_END_FRAMES`), `:1809-1979` (`M2_KEYS` + `M2_KEYS_LEN`),
  `:1992-2093` (`struct m2_result`), `:2111-2118` (`struct m2_period_end`),
  `:2220-2785` (`m2_directives`, incl. `M2_M_HALF_OPEN`/`M2_M_HALF_TIME`/
  `M2_M_ET_ENTRY` `:2743-2785`), `:2798-3138` (`run_scenario` + the v21 stop
  `:3105-3115`), `:3146-3328` (`run_scenario_capture` + the probes),
  `:4090-4250` (`run_phase21_riders`), `:4252-4269` (`report_diff`),
  `:4271-4605` (`main`: the determinism gate `:4306-4320`, the anchors
  `:4416-4544`, the additivity gate `:4561-4587`).
- `tests/golden/engine/m2-frames.txt` — the 2522-line v21 state sequence (fresh
  `awk`; sha `0b87fd18…`); `tests/golden/engine/m1-frames.txt` — 691 lines,
  `09b726b7…` (immutable).
- `src/fifa96_engine/platform_null.c` — `:23-46` (`fnv1a` + the cumulative
  `present_hash`), `:29-33` (`n_poll`'s `tape_pos >= tape_len` stop).
- `src/fifa96_engine/fifa96_match_run.c` — `:2425-2432` (`set_period`),
  `:4169-4192` (the v21 `mark_over` condition), `:4195-4206` (the
  `OVER→POST→EXIT` resolve), `:4208-4228` (`run_step`), `:4231-4244` (`run_end`
  → `FRONTEND`).
- `src/fifa96_loader/fifa96_match_state.c` — `:4-8` (the phase class table:
  `0x13`/`0x14` = class 0), `:47-92` (the clock; the `limit` select `:63-65`,
  the completion `:77-88`, the class-2 aux block `:66-72`).
- `src/fifa96_engine/fifa96_engine.c` — `:211-241` (the MATCH dispatch).
- `docs/ghidra/FU230_tape_extension.md` — the v21 extension/re-pin precedent
  (§2.2 the append-only tail, §3.1 the reason wording, §3.2 the diff
  methodology, §3.3 the mutation-RED rider classes).
- `docs/ghidra/FU228_post_period_lifecycle.md` — §1 (the engine lifecycle),
  §2 (the end word `[0x158822]` + its complete writer/reader set), §2.2
  (`FUN_0008BAF0`: `0x8BCD9 = 1`; `0x8BD8A`/`0x8BDD4 = 2`; the `[0x10F360]`
  latch; the `0x8BD95` equal-score test), §2.3 (the frame-loop consumer).
- `docs/superpowers/specs/2026-10-13-fifa96-m2-phase22-end-spine-design.md` —
  §2 (goal/non-goals), §5 (T1–T6; T6 the tape re-pin), §6 (the re-pin policy),
  §7 (risks), §10 (D1–D3).
- `docs/superpowers/plans/2026-10-13-fifa96-m2-phase22-recon-ahead.md` — Task 8
  (the W22-8 objective/steps) and the handoff.

## Landing note

This slice is **design only**: it fixes the tape format at v21, the extension
shape (append-only preferred), the four new directives, the return window, the
length model, the additivity conditions, the re-pin wording/diff methodology, the
seven mutation-RED rider classes, and the required code-edit inventory — and it
records the two cross-cited ET-native items (the ET-half phases / terminal phase,
FU238-L2) as the terminal slices' to confirm before T6. No engine change, no test
change, no golden change, no census move.

---

## 9. Errata — phase-22 Stage-2 (FU-239; append-only, prior sections not rewritten)

Written 2026-10-14 at the phase-22 close-out (Task 6). Prior sections are **not**
rewritten (AGENTS.md errata discipline); the corrections are quoted cross-refs.

* **§2.3 ("The frame picture") is corrected by FU-239 §3-§5.** The text "the ET
  halves then each complete like the regulation halves: a class-1 play window …
  bounded by the class-0 phase-`0x13`/`0x14` kickoff phases" is **half right**:
  the class-1 window is real, but it is the **phase `2`** re-arm (FU-239 §3), and
  the class-0 `0x13`/`0x14` phases are the **ET-over screen dwell**
  (`word[0x158818] > 0x4B0 → word[0x158822]=2`), not a per-half kickoff boundary
  (FU-239 §4). The scheduled terminal therefore fires from the `0x13`/`0x14`
  dwell after the ET-half-2 completion (the T4b-measured engine step 6502), not
  from a `0x0C` pattern.
* **§2.3's "the `word[0x158818] > 0x78` screen gate / the `0x960` camera hold
  (FU-235)" conflates two cells (the `g2` erratum).** The **phase-`0xC`**
  terminal gate is `word[0x158818] > 0x168` (native `0x8BCAC`, 360 ticks — the
  engine `mark_over` dwell); the **ET-half** dwell threshold is
  `word[0x158818] > 0x4B0` (native `0x8BD5A`/`0x8BDAC`). FU-235 §1 already
  separates them; §2.3's single `0x78` figure is the ET-block entry gate, not
  the terminal threshold.
* **FU-232-L6 is CLOSED** (the phase-loop lifecycle consumer is self-terminating
  at the ET-over; FU-239 §4 / `t4b-recon/g3-etover-resume.md`). FU-232 §3's open
  leg — "what resumes after the ET-over consume" — is answered: the
  `FUN_0004B100` `AX==2` arm clears `word[0x158822]` (`0x4B29B`) and `RET`s
  (`0x4B2A8`), writing **no** `[0x105FFC]` screen; fresh `get_xrefs_to 0x158822`
  = 10, and the only readers are the head guard `0x4B106` and the clock tail
  `0x8B678`. The ET-over is a transient consume (**no over-screen**); the frame
  loop resumes the frame body. Landed as the same-frame consume
  (`fifa96_match_run.c:4271-4272`). The word-`1` regulation-over (which does show
  the OVER screen) is the off-tape route (§2.6), covered by
  `run_regulation_over_probe`.
