# FU-313 — the natural-reach acceptance harness (the camera-cell trace, the headless probe, the Layer-2 capture, the matrix)

**Phase/Provenance:** M2 phase-30 recon-ahead wave (Stage-1, **task 4/4 — `FU313: the acceptance harness`**), 2026-10-21, read-only. Deliverable of `docs/superpowers/plans/2026-10-21-fifa96-m2-phase30-recon-ahead.md:67-75` under `…phase30-natural-reach-design.md` §3.4/§5/§8.3/§8.4. **No Ghidra call was needed for this slice** — every claim below is either (a) a fresh read of the engine/test C sources (file:line), (b) a fresh measurement on this host with the already-built binaries plus a throwaway `/tmp/opencode` probe (listed in §8), or (c) a native anchor marked *carried* with the frozen doc named (FU309, FU307, FU157, FU171). No file in the repo was created or changed except this draft; no git; no ISO write; M1 `09b726b7…` and M2 `b281a9b2…` were re-verified byte-identical this session (§8.3).

---

## 1. Result and verdict

**The cheapest honest harness is three small pieces, none of which needs new machinery: (1) a ~20-line default-off camera-cell trace inside the existing granted-frame loop, (2) a ~140-line extension of the *existing natural probe* in `tests/test_engine_m2.c` — same tape, same engine path, no new target, no golden risk — that samples the camera cells per granted frame and the stage-select through the *existing* `keeper_cpu.ladder` seam, and (3) the FU309 §5.5 capture procedure with the trace promoted to primary and a whole-scene bbox added to the (relative) AE gate.**

The mechanism is measured, not projected: the throwaway probe reproduced `test_engine_m2`'s natural chain exactly (match start step 5, **phase 2 at engine step 1802**), sampled **3 598 granted frames over 101 s of match clock** with every camera cell static (`pos=(0,0,0)`, `vel=(0,0)`, `speed=0`, `event_param=0`, `tracked=-1`, `pan_counter=0`), and observed the **FU282 ladder gate firing 6 118 times** on that natural window with `fifa96_keeper_cpu_stage_select` returning `ran=0` on every call (`cam=(0,0,0)`, `lane_x=0`, `follow_speed=0`) — the FU307-L1 refusal, now durable and attributable without temporary instrumentation (§8.4).

| piece | cost | default state |
|---|---|---|
| camera-cell trace (FU309-L5) | 1 header include + 2 struct fields + init + open/close in `begin`/`end`/`teardown` + 1 line at the loop tail + 1 saved return value (~25 LOC, 4 sites, no public API) | **off** (env unset ⇒ no file, no branch taken beyond one `FILE *` test) |
| headless natural probe | ~140 LOC in `tests/test_engine_m2.c` (sampler + seam wrapper + asserts); 0 new target, 0 CMake change, 0 golden change (stderr only) | runs with the existing test (ISO-gated asserts) |
| Layer-2 capture | existing `import`/`convert`/`compare`/`ffmpeg` commands + `FIFA96_CAMERA_TRACE=<path>`; 0 code | trace off unless asked |
| matrix / invariants | the existing gates (`ctest`, `cmp` on M1/M2) | unchanged |

**Verdict: GO.** The harness is provable from the existing frame path and test seams; nothing about it stages a camera gate. Its only RED is the property itself (the camera leaving `(0,0,0)`), which is FU310/FU312's to produce — the camera assertions therefore land **with** the producer, RED-first, per the phase order (spec §4: bootstrap → event → acceptance).

---

## 2. The question, and what the harness must prove

> **What is the cheapest honest harness that proves the natural reach headlessly and attributes the visible capture?**

The spec §5 acceptance decomposes into the links the harness must carry, and nothing else:

- **L-A1** a *natural* Layer-1 witness — a headless run from the **real kickoff** with a scripted input sequence, no staged camera cells, in which the camera place/Y leaves `(0,0,0)` and the follow moves it over a **sustained window** (spec §5 Layer-1; §8.3).
- **L-A2** a **frame-path fixture** that reaches the same chain by staging *producer inputs only*, never the camera gates, with mutation-RED riders per producer (spec §5 Layer-1, second clause).
- **L-A3** the **visible** capture: `make game` shows the follow; same-clock pan/still pairs; the **trace is the primary attribution** and the AE is **relative corroboration** (spec §5 Layer-2; §8.4).
- **L-A4** the invariants: M1 immutable, M2 byte-identical (or stopped for a written diff), the census holds `60/80` unless a row is proven live, the live-chain rule (spec §5 Invariants).

The harness must therefore be: (i) deterministic and headless, (ii) runnable from the ISO-gated CTest set, (iii) unable to move the goldens, (iv) able to name *which* producer moved *which* cell in *which* frame.

---

## 3. Evidence: the frame path, the cells, the input model, the harness patterns

### 3.1 The frame path and where a per-frame sample belongs

`fifa96_match_run_frame` (`src/fifa96_engine/fifa96_match_run.c:4319-4567`) is the 30 Hz frame body. The grant is the pace delta (`:4330-4333`: `pending` before, `fifa96_match_state_tick`, `granted = pending_after − pending_before`), and the body runs once per grant in `for (uint32_t i = 0; i < granted; i++)` (`:4345-4543`). The pieces the trace must bracket, in native order:

| site | line | meaning |
|---|---|---|
| slot update | `:4360-4362` | FU-70 input slot |
| camera integrator | `:4370-4373` | `fifa96_camera_update_walk` (FUN_000736AC head) |
| every-frame follow | `:4383` | `fifa96_match_run_camera_follow` — **return value currently discarded** (`(void)`) |
| pose feed / bounds / display | `:4392`, `:4400`, `:4403` | presentation |
| entity chain + drain | `:4412-4431` | the receive arm (the event setter) runs **inside** here |
| ball machine | `:4492` | FUN_0008BAF0 |
| scene stage (opt-in) | `:4542` | render-only |
| loop close | `:4543` | **the trace line's site** |

`fifa96_match_run_step` (`:4582-4603`) never runs the body — the body runs from the 100 Hz trampoline `fifa96_match_run_tick` (`:1797-1802`) fed by the engine clock. So "per granted frame" is the loop body, not `step`; a trace hung on `step` would sample presentation frames, not match frames.

**Order matters for attribution**: the follow at `:4383` runs *before* the entity chain, so an event set in frame *N* produces the first follow frame in *N+1* (FU309 §5.2 step 4, carried). A line written at the loop tail (`:4543`) therefore shows the event and the pan counter as the frame left them, while `follow=` is that frame's follow verdict. That is the pairing the window assertion needs.

### 3.2 The camera cells

The single camera struct is `struct fifa96_camera` (`include/fifa96_loader/fifa96_camera.h:9-83`): `pos_x/y/z`, `vel_x/vel_z`, `speed`, `timer`, `timer_limit`, `event_param`, `event_cursor`, `rate_x/rate_z`, `dir_157815`, `pan_counter`, `ramp_divisor`, `tracked`, `follow_speed`, `event_peak`, `band_1577f6`. The live camera is `mr->render.camera` (`include/fifa96_engine/fifa96_match_run.h:305-307`) and is reset by `fifa96_match_run_reset_render` (`match_run.c:1891-1895`, `fifa96_camera_init(...,0,0,0)`) — the native zero kickoff (FU157 §1, carried).

Two fields are easy to conflate and the trace must carry both: `speed` (native word `0x1577BE`, follow gate 2 at `match_run.c:3932`) and `follow_speed` (the pre-tail snapshot of the same word the gate actually reads, FU308's seam; `camera.h:60-73`).

### 3.3 The input model and the "scripted input" precedent

- Engine keys: `include/fifa96_engine/fifa96_keys.h:7-23` (`UP/DOWN/LEFT/RIGHT`, `CONFIRM=RETURN`, `DECLINE=BACKSPACE`, `KICK`, `PASS`, `LONG_BALL`, `QUIT`).
- `fifa96_match_run_input` (`match_run.c:436-455`) maps a key batch and **consumes press events only** (`state != 1` skipped, `:445`); it latches the sample; the *frame body* consumes it. `match_run_map_key` (`:423-434`) is the raw→FU-61 code map (directions OR `0x1/0x2/0x8/0x4`; KICK `0x10`, PASS `0x20`, LONG_BALL `0x40`).
- The headless input source is the **null platform's scripted tape** (`src/fifa96_engine/platform_null.c:25-34`): one entry per poll, so a press/release pair spans two engine steps. The real walkthrough is already encoded: `M2_KEYS` (`tests/test_engine_m2.c:1932-2102`: CONFIRM/CONFIRM → DECLINE/DECLINE → CONFIRM/CONFIRM → the kickoff and the direction holds).
- The match branch of the engine polls once per step and feeds the run (`src/fifa96_engine/fifa96_engine.c:211-240`), so `fifa96_engine_step` × N with a tape is bit-identical to `make game` minus the SDL3 present. `--headless` exists on the real executable too (`src/fifa96_engine/fifa96_main.c:12-47`), and `make game` is `./build/fifa96 game/FIFAPCCD96.iso` (`Makefile:39-41`).

### 3.4 The harness patterns that already exist (no new framework)

| pattern | site | what it gives |
|---|---|---|
| the **natural probe** (the exact shape to extend) | `tests/test_engine_m2.c:3356-3436` | boot → front-end → bridge → kickoff, **no directives**, the M2 tape, null backend, `step_ns = 10 ms`; asserts the situation-0 kickoff (row `0x24`), the act-1 route, and live phase 2 (`:3417-3433`) |
| its call site + stderr report | `:4863` and the `natural probe: … phase 2 at step %d` line | the probe is already run in both ISO modes and writes only to stderr |
| the **granted-frame** helper | `tests/test_engine_match_frame.c:1144` (`one_granted_frame`) | ticks the pace until exactly one grant |
| the **follow-window** loop | `tests/test_engine_match_frame.c:2486-2510` | `while (pan_counter == 0 && window < 120)`, `vel == dirs*15`, `pos_x == prev + vel*delta`, then the derived window == 52 |
| the **ladder seam** precedent | `tests/test_engine_match_frame.c:7187-7243` (`band_ladder_capture`, installed at `:7231-7232`) | the probe can wrap the ladder and read the stage-select inputs of the real frame path |
| the ISO-gated skip | `tests/test_engine_m2.c` `file_exists(M2_ISO_PATH)`; `CMakeLists.txt:621-632` | the repo convention; `test_engine_m2_noiso` covers the no-ISO path from the build dir |
| the M2 golden | `tests/golden/engine/m2-frames.txt`, sha `b281a9b2…`; M1 `09b726b7…` | **stdout** only — the probe's stderr asserts cannot move it |

### 3.5 The stage-select sample point (and why it needs no production change)

`fifa96_keeper_cpu` carries a **function-pointer seam** — `fifa96_keeper_cpu_ladder_fn ladder; void *ladder_ctx;` (`include/fifa96_loader/fifa96_keeper_cpu.h:139-152`) — initialised to the ported body (`src/fifa96_loader/fifa96_keeper_cpu.c:27-41`) and invoked by the entity walk only when the FU282 gate holds (`src/fifa96_engine/fifa96_match_entities.c:742-826`; the gate is `phase == 2 && queued20 == 0 && interlock != 0`, `fifa96_keeper_cpu.c:81-104`). A test-side wrapper can therefore (a) count the gate firings, (b) delegate to `fifa96_keeper_cpu_ladder_run` unchanged, and (c) apply the public `fifa96_keeper_cpu_stage_select` (`:141-234`) to the *same live record view* to sample `ran/stage/latch/copy`. It **observes**; it stages nothing. The measured run confirms it is behaviour-neutral (§8.4: phase 2 still at step 1802 with the seam installed).

The AX prepass that decides `ran` is `fifa96_keeper_cpu.c:158-175`: `si = |cam_x|`, `di = |cam_z|` (both truncated to int16), `if (si < 0x420 && di > 0x7B0)` plus the sign/side arm. With `cam=(0,0,0)` the `di > 0x7B0` term fails — the mechanical reason the sample is `ran=0` today.

---

## 4. The trace design (FU309-L5) — exact shape

### 4.1 Name, value, lifetime

- **Env var:** `FIFA96_CAMERA_TRACE` — the only new env var in the tree (measured: no `getenv` exists anywhere in `src/`, `include/`, `tools/`, `tests/` this session).
- **Value:** a filesystem path. Unset or empty ⇒ **off**. Opened `"w"` (one file per run) with `fopen` at the **end of `fifa96_match_run_begin`**, immediately before its `return 0` (`src/fifa96_engine/fifa96_match_run.c:2593`) — i.e. after `fifa96_match_lifecycle_begin` has registered the run's 100 Hz callback (`:2532`) and `mr->running` is set (`:2537`) — so a failed begin creates no file. Closed in `fifa96_match_run_end` (`:4605`) and in `fifa96_match_run_teardown` (`:1820`), idempotently.
- **Open failure:** one `stderr` line (`fifa96: camera trace: cannot open <path>`) and off. Never stdout, never the goldens.
- **Per-line flush:** `fflush` after each line, so a live `make game` capture can be read while the window is open and a crash keeps the tail.

### 4.2 The two struct fields (the only state)

```c
/* in struct fifa96_match_run, beside ticks/steps (fifa96_match_run.h:604-605) */
FILE *cam_trace;            /* FU313/FU309-L5: the FIFA96_CAMERA_TRACE sink (NULL = off) */
uint32_t cam_trace_frame;   /* FU313: the run-relative granted-frame index */
```

`fifa96_match_run_init` must zero both explicitly (the init is field-wise, not a `memset`: `match_run.c:2069-2100`), and `begin` must reset `cam_trace_frame = 0`. The header gains `#include <stdio.h>` (it currently includes only `<stddef.h>`/`<stdint.h>`, `fifa96_match_run.h:1-3`).

### 4.3 The line (one per granted frame, at the loop tail `match_run.c:4542-4543`)

Header, written at open:

```
# fifa96-camera-trace v1 frame clock phase pos vel spd fspd ev cur trk pan tim ramp rate follow tick
```

Body:

```
f=412 clock=00:37 ph=2 pos=0,0,0 vel=0,0 spd=0 fspd=0 ev=0x0 cur=0x0 trk=-1 pan=0 tim=0/0 ramp=0 rate=0,0 follow=0 tick=1121
```

| field | source | why it is in the line |
|---|---|---|
| `f` | `cam_trace_frame++` | the run-relative granted-frame index — the alignment key with the fixture's window counts |
| `clock` | `state.total_seconds` as `MM:SS` | **the same string the HUD draws** (`match_run.c:4765-4767`), so a same-clock capture pair maps to a trace interval |
| `ph` | `state.phase` | 1 = kickoff, 2 = open play; the ladder gate needs 2 |
| `pos` | `camera.pos_x,pos_y,pos_z` | the place; the left-zero witness |
| `vel` | `camera.vel_x,vel_z` | the follow rate (`dirs × 15`) |
| `spd` | `camera.speed` | word `0x1577BE`, follow gate 2 |
| `fspd` | `camera.follow_speed` | the pre-tail word gate 2 actually reads (FU308) |
| `ev` | `camera.event_param` (hex) | word `0x1577F0`: the `>0x10` call gate and the `>0xF0` arm gate |
| `cur` | `camera.event_cursor` (hex) | word `0x1577EE` low |
| `trk` | `camera.tracked` | `[0x1577CA]`; `-1` = NONE |
| `pan` | `camera.pan_counter` | `[0x157821]`; the lockout |
| `tim` | `camera.timer/camera.timer_limit` | words `0x1577FA/0x1577F4`; the window |
| `ramp` | `camera.ramp_divisor` | F8 = `[0x1577F8].lo`; the window derives as `win = ramp/2` (52 at camY `0x70`, 54 at `0x50`, FU309 §3.1) — the trace self-checks the window without a debugger |
| `rate` | `camera.rate_x,rate_z` | `[0x157816/17]`; follow gate 4 |
| `follow` | the saved return of `fifa96_match_run_camera_follow` (`match_run.c:4383`) | 1 = an arm ran, 0 = a gate refused — the per-frame follow grant |
| `tick` | `mr->ticks` (`fifa96_match_run.h:604`) | the 100 Hz counter, for sub-second alignment of a capture pair |

The field set is exactly the phase-29 gdb table's cells (`docs/ENGINE.md:2264-2286`) plus the four discriminators (`fspd`, `ramp`, `rate`, `follow`) and the two alignment keys. The phase-29 sample becomes reproducible from a file instead of a debugger session — which is what FU309-L5 asked for.

### 4.4 Why this cannot move M1/M2

- Env unset in CTest ⇒ `cam_trace == NULL`; the only per-frame cost is one pointer test.
- Even with the env **set**, output goes to the named file, never stdout — so `./build/test_engine_m2 > x` stays byte-identical. Measured today: M2 stdout `cmp`-identical to `tests/golden/engine/m2-frames.txt` (`b281a9b2…`), M1 `09b726b7…` untouched (§8.3).
- The one production edit inside the loop is replacing `(void)fifa96_match_run_camera_follow(mr);` with `int follow_ran = fifa96_match_run_camera_follow(mr);` — no behaviour change (the return is already 0/1 and discarded).

### 4.5 Rejected alternatives

1. **A `gdb` sample per acceptance** (the phase-28/29 precedent): not durable, not scriptable in CI, and the phase-29 table already lost its frame/clock columns (`n.r.`, `docs/ENGINE.md:2281-2287`).
2. **A file-static sink in `match_run.c`**: leaks across runs and tests, cannot be closed at `end`, and makes the probe's self-check order-dependent.
3. **A public `fifa96_match_run_camera_trace_open()` API**: not needed — the probe can `setenv` before `begin` (`-std=gnu11`, POSIX `setenv`), and `make game` needs only the env. (If Stage-2 finds `setenv` awkward, the opener is a 6-line addition; the env read stays the `make game` path either way.)
4. **A debug-build-only `#ifdef`**: the acceptance must run on the shipped `make game` binary, so the trace is runtime-gated, not compile-gated.

---

## 5. The headless natural-play probe — exact shape

### 5.1 Where it lives

**Extend `run_natural_probe` in `tests/test_engine_m2.c:3356-3436`.** No new test binary, no new CMake target, and **no change to the golden-producing tape**: the function already boots the engine with the ISO, walks the real front-end → bridge → kickoff, steps with `step_ns = 10 ms`, and reaches live phase 2 with **no match directives** (asserts at `:3417-3433`). The probe's own tape is a copy (`M2_KEYS` + the route tail, or a sibling array), so `M2_KEYS`/`run_scenario` are untouched. Its output goes to stderr (`:4863`), so the golden cannot move. A separate `test_engine_natural_reach.c` was considered and rejected on cost (a second copy of the walkthrough tape + a target + a CTest entry for zero extra coverage).

### 5.2 The drive

1. **The real kickoff:** unchanged — engine boot with `game/FIFAPCCD96.iso`, the `M2_KEYS` tape (`:1932-2102`), no directives. Measured: match start at engine step 5, phase 1.
2. **The measured tape must be replayed verbatim, and appended to only after phase 2.** This session measured that a synthetic held-direction tail appended at the tape's end (RIGHT entries from index 1800 on) **suppresses the natural phase-2 entry entirely** — phase stuck at 1 for 12 000 steps, vs **phase 2 at step 1802** with the exact tape (§8.4). The probe therefore replays `M2_KEYS` intact and appends the FU310/FU312 route script **in a probe-local tape array** (`M2_KEYS` + the route tail, or a sibling `M2_PROBE_KEYS`) — never by editing `M2_KEYS` itself, because the golden producer (`run_scenario`) shares that array and its byte-exact transcript would move.
3. **No camera staging:** the probe never calls a `fifa96_camera_*` setter and asserts `camera.pos == (0,0,0)` at match start (the native kickoff reset). The only write it makes to run state is the observation seam.
4. **The stage-select observation seam:** at match start (after the bridge; `begin` re-inits `keeper_cpu` at `match_run.c:2321`), install

```c
struct m2_stage_capture cap;              /* calls, ran, cx1, cx4, stage_hist, last rec view */
mr.keeper_cpu.ladder     = m2_stage_capture;   /* wraps fifa96_keeper_cpu_ladder_run */
mr.keeper_cpu.ladder_ctx = &cap;
```

where the wrapper delegates to the real body and additionally calls `fifa96_keeper_cpu_stage_select(rec, &sel)` on the same `rec`. Measured neutral: with the seam installed the phase-2 step is unchanged (1802).

### 5.3 What it asserts

**Structural (GREEN today; can land before the producer):**

| # | assertion | basis |
|---|---|---|
| P1 | `match_start_step == 5`, `phase == 1` at start | `tests/test_engine_m2.c:3390`, `:3417`, measured |
| P2 | the natural phase-2 entry exists (`> 0`, after the act-1 route) — today 1802; the tape's kick step stays inside the existing `M2_NATURAL_PHASE2_CAP` bound (`:3422`); record the measured step, do not hard-code 1802 | `:3425-3433`, measured |
| P3 | `camera.pos == (0,0,0)` at match start (no staging) | `reset_render` `match_run.c:1894` |
| P4 | the ladder gate fires (`cap.calls > 0`) in the natural window, and every sampled `stage_select` is recorded with its input view | measured 6 118 calls |
| P5 | determinism: the probe replayed twice yields identical samples (the M2 3-replay gate at `:4842-4860` already pins the transcript; the probe's samples are the witness) | measured |

**The acceptance assertions (RED today; land with FU310/FU312):**

| # | assertion | basis |
|---|---|---|
| P6 | **the camera leaves zero**: at least one granted frame with `pos != (0,0,0)` (the first non-zero place recorded with its step/frame/clock) | spec §5 Layer-1; today 0 of 3 598 frames |
| P7 | **the follow window is sustained**: a contiguous run with `follow == 1 && pan == 0`; every frame `vel == dirs*15` and `pos_x == prev + vel*frame_delta`; the run length equals `ramp_divisor/2` at the bind (52 @ camY `0x70`, 54 @ `0x50`), mirroring `tests/test_engine_match_frame.c:2486-2510` | FU309 §3.1; the window is derived, never a literal |
| P8 | the lockout: after the window `pan_counter == 1` and `follow == 0` until a new setter re-opens | `:2511-2519` |
| P9 | **stage-select samples**: the sample carries the camera triple, `lane_x` and `follow_speed` at every ladder call; the assertion is `cx4 >= 1` **iff** FU311 selects the keeper route, otherwise the count is *recorded* and the keeper route stays FU307-L1 (spec §8.2 — the keeper route is optional) | FU307-L1 / FU311 |
| P10 | **trace self-consistency** (only when the probe sets `FIFA96_CAMERA_TRACE`): the body-line count (excluding the header) equals `pace.pending` at the end, and each sampled frame's `pos/ev/trk/pan/follow` equals the probe's in-memory sample at the same `f` | this design |

**ISO gating:** P6–P9 are ISO-dependent (the formation seed places the records the follow tracks) and go behind the existing `with_iso` gate; P1–P5 hold in both modes, matching the current probe.

**RED-first staging (the phase order):** the structural block (P1–P5) is GREEN on today's tree and lands first; P6–P10 land in the acceptance task together with the bootstrap/event producer, so `make check` is never left red between commits.

---

## 6. The Layer-2 capture procedure (the trace primary, the AE relative)

Host: ISO present, SDL3 build, `DISPLAY=:1` (all four capture tools verified present this session: `import`, `xdotool`, `convert`, `compare`, plus `ffmpeg`).

### 6.1 Run

```
export DISPLAY=:1
FIFA96_CAMERA_TRACE=/tmp/opencode/p30-cam-trace.log ./build/fifa96 game/FIFAPCCD96.iso
# equivalently: FIFA96_CAMERA_TRACE=… make game
```

The trace is the **only** artefact that attributes motion to the camera. Keep it out of the repo (`/tmp/opencode/…`) unless a capture is being committed; the acceptance record quotes the window's lines.

### 6.2 Walkthrough (unchanged from the phase-8/29 precedent, plus the FU310/FU312 route)

intro RETURN → front-end BACKSPACE → RETURN → the kickoff KICK release (clock starts) → the FU310 bootstrap script (the restart/kick sequence its verdict names) → the FU312 open-play script. The FU309 §5.5 route script (`docs/ghidra/FU309_r2l4_event_source.md:184-185`) is carried as the fallback.

### 6.3 The trace is primary

After the run, attribute in this order:

1. `grep -c 'follow=1' <trace>` — must be non-zero; `grep -n 'follow=1'` gives the window's first/last `f`.
2. `grep -nv 'pos=0,0,0' <trace> | head` — the first line whose place is not the zero triple; its `clock=` is the window's clock second.
3. The contiguous `f` range of the `follow=1` lines — its length must equal `ramp/2` from the same line (52 or 54), and `pan=0` must hold throughout.
4. The `ph=` of the window — 2 (or the set-piece phase the bootstrap uses, recorded).
5. Only then look at the pixels.

A capture pair that does not fall inside a `follow=1` run of the trace is **unattributed** and cannot support a Layer-2 claim.

### 6.4 Same-clock pairs

```
WID=$(xdotool search --name '^FIFA 96$' | head -1)
import -window "$WID" /tmp/opencode/p30-pan-a.png      # inside one HUD second
import -window "$WID" /tmp/opencode/p30-pan-b.png      # same MM:SS
convert … -crop 960x591+0+0 +repage    # the scene band; the HUD/clock sits below y = 591
compare -metric AE /tmp/opencode/p30-pan-a.png /tmp/opencode/p30-pan-b.png null: 2>&1
```

Same for the **still control** with no key held, in the same second, and — binding — **re-measured in the same session and the same build** (FU309-L6; the p28 datum 1 582/2 052 and the p29 datum 3 421/2 988 invert each other, so there is no absolute floor).

### 6.5 The AE gate (relative) and the bbox criterion (new)

- **Gate 1 (relative, binding):** `AE_pan > AE_still`. Both numbers recorded with the capture pair's clock second and the trace window.
- **Gate 2 (new, necessary-not-sufficient):** the `compare -compose src` diff bbox must **span the scene band**, because a camera translation moves the whole scene. The phase-29 pan pair's bbox was `476x56+230+296` — the sprite band — which is exactly why its AE delta could not attest a pan (`docs/ENGINE.md:2253-2260`). Proposed threshold, to be re-measured at the acceptance: bbox width `>= 0.9 × 960 = 864` **and** height `>= 0.75 × 591 = 443`. A band-limited diff fails Gate 2 and the pair is labelled "entity motion", never a pan.
- **Guidance, not a floor:** a genuine whole-scene shift sat near `2.9e4–4.6e4` in the phase-9 landings (≈14–22× the still control); a pan AE in the low thousands is a fail signal to investigate (FU309 §5.5 step 5, carried).

### 6.6 Attribution rules (binding)

1. The trace attributes; the AE corroborates (spec §8.4). No pixel-only claim.
2. Trace and capture must be the **same run**.
3. Same-clock pairs only; the pair must lie inside a `follow=1` run.
4. The still control is re-measured in-session; no absolute AE floor.
5. A band-limited bbox is entity motion, not a pan.
6. No golden re-pin for a capture; a live path that moves M1/M2 stops the task for the written diff first (spec §5 Invariants).
7. If the trace shows no window, Layer-2 is a numbered waiver with the trace as the record (spec §6) — never a widened fixture.

---

## 7. The acceptance matrix (link → test/measurement)

| link (spec §5) | artefact | today |
|---|---|---|
| L-A1 natural witness | natural probe P1–P3, P6–P8 (ISO-gated), `test_engine_m2` | P1–P3 GREEN; **P6 RED by design** (0/3 598 frames leave zero) |
| L-A2 frame-path fixture (producer inputs only) | the FU309 Part-A fixture (`tests/test_engine_match_frame.c:3660-3718`) extended per the chosen producer; the pan arm labelled corroboration (FU309 errata 6) | GREEN on the staged chain; the new producer's rider lands with it |
| mutation-RED riders per producer | the seven one-line reversions re-applied manually in a scratch worktree; the task report records each RED; automation **out of scope** (spec §3) | manual (FU309-L9, open) |
| the follow window (sustained) | probe P7/P8 + the trace's `follow=1` run length == `ramp/2` | RED by design |
| stage-select samples | probe P4/P9 (the existing seam) | `calls 6 118 / ran 0 / cx4 0` measured |
| L-A3 visible follow | `make game` + trace primary (§6.3) + the same-clock pan/still pairs (§6.4) + the relative AE + bbox (§6.5) | WAIVED in phase-29 (no window); procedure now frozen |
| trace is default-off | M1 `cmp` (`09b726b7…`) and M2 `cmp` (`b281a9b2…`) with the env unset (verified this session, §8.3); at landing also re-run M2 with the env **set** — the stdout must stay identical (the sink is a file) | verified unset; the env-set arm is the landing check |
| M1 immutable | `./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt` | sha verified `09b726b7…` |
| M2 byte-identical | `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt` | verified identical (295 087 B) |
| determinism | the M2 3-replay gate (`tests/test_engine_m2.c:4842-4860`) + the probe replayed twice (P5) | GREEN |
| census `60/80` unless a row is proven live | `docs/ENGINE.md` headline + the four-doc same-commit rule | holds (no row wired here) |
| live-chain rule (no staged gates) | the probe stages no camera cell; P3 asserts the zero start; the seam only observes | holds by construction |

---

## 8. Measurements this session (read-only; no repo change)

### 8.1 The throwaway probe

`/tmp/opencode/fu313_probe*.c` — the M2 `M2_KEYS` array extracted verbatim from `tests/test_engine_m2.c:1932-2102`, linked against the existing `build/*.a` (the `build/CMakeFiles/test_engine_m2.dir/link.txt` order), null backend, `step_ns = 10 ms`, `headless = 1`, `iso_path = game/FIFAPCCD96.iso`; per live step it samples the camera cells and (in the `_sel` variant) wraps `mr.keeper_cpu.ladder` to count the gate and call `fifa96_keeper_cpu_stage_select` on the same record view. Built with `/usr/bin/cc -g -std=gnu11 -fsanitize=address,undefined`; run from the repo root.

### 8.2 The numbers

| measurement | value |
|---|---|
| match start (engine step) | **5**, `phase == 1` |
| natural phase 2 (engine step) | **1802** — reproduces `test_engine_m2`'s own stderr line (`… phase 2 at step 1802 …`, §8.3) |
| live window / granted frames | 11 996 live engine steps (match start at step 5, run to step 12 000) = **3 598 granted frames**, match clock 0 → **101 s** |
| camera cells over the window | `pos=(0,0,0)`, `vel=(0,0)`, `speed=0`, `follow_speed=0`, `event_param=0`, `event_cursor=0`, `tracked=-1`, `pan_counter=0`, `timer=0/0`, `rate=(0,0)` at **every** sample — `left_zero=0` |
| ladder seam calls (the FU282 gate) | **6 118** |
| `fifa96_keeper_cpu_stage_select` on the same record view | **6 118 calls, `ran=0` every time**, `cx1=0`, `cx4=0`, `cam=(0,0,0)`, `lane_x=0`, `follow_speed=0` |
| seam neutrality | with the seam installed, phase 2 still at step **1802** |
| the synthetic-tail control | a held RIGHT appended from tape index 1800 ⇒ `phase == 1` for 12 000 steps, `first_phase2 = -1` (three runs: the verbatim tape + tail with and without the seam, and the hand-rolled prefix) |

### 8.3 The goldens and the gate (fresh)

```
sha256sum tests/golden/engine/m1-frames.txt  -> 09b726b7…a2cd   (691 lines)
sha256sum tests/golden/engine/m2-frames.txt  -> b281a9b2…8f4a   (6522 lines)
./build/test_engine_m2 > /tmp/opencode/m2_out.txt 2> …          -> rc=0, "test_engine_m2 OK"
cmp /tmp/opencode/m2_out.txt tests/golden/engine/m2-frames.txt  -> identical
stderr: "natural probe: situation-0 kickoff at step 12 (match start 5), … act-1 route at step 1679, phase 2 at step 1802, score 0-0"
stderr: "determinism gate: 3 replays byte-stable (295087 bytes each); the golden producer matches them"
```

### 8.4 What the measurement corrects or pins

1. **The ladder gate is live on the natural tape; the AX prepass is the refusal.** `docs/ENGINE.md:2336-2339` (FU307-L1) reads "measured `stage_select ran=0 …` over the 165-frame transcript: the ladder does not run at all". The first clause is confirmed; the second is **window-specific**: on the natural post-kickoff window the ladder seam is entered **6 118 times**, and the *stage-select* refuses every call. The durable claim for FU311 is "the AX prepass refuses every ladder call in the natural window", not "the ladder never runs" — and a harness that sampled only `ran` would conflate the two. The probe records both counters for exactly that reason.
2. **The input script is timing-coupled.** A held-direction tail after the tape's last entry suppressed the natural phase-2 entry (12 000 steps, phase 1) where the exact tape reaches it at step 1802. The cause is not proven (candidate: the row-01 kickoff release gate reading the slot edge) — a numbered leg below, and a hard constraint on the probe's script shape.
3. **The stage-select seam is behaviour-neutral.** Phase 2 at 1802 with and without it, on the same tape.
4. **The baseline is RED exactly where the phase expects.** No camera cell leaves `(0,0,0)` over 101 s of natural open play — the witness P6 is a real gate, not a tautology.

---

## 9. Numbered legs

1. **FU309-L5 — the env-gated camera-cell trace.** **Settled as a design** (§4: `FIFA96_CAMERA_TRACE`, the 16 fields, the loop-tail site, default-off, no API). **Not closed** until the code lands; owner: the phase-30 acceptance task (`fifa96_match_run_frame` / `fifa96_match_run_begin` / `_end`). The probe's P10 is the trace's own regression check.
2. **FU309-L6 — the still-control AE.** **Closed as a procedure** (§6.4-6.5): relative gate, same-session re-measure, no absolute floor, **plus** the new whole-scene bbox criterion. The AE remains corroboration only; owner: the acceptance procedure.
3. **FU309-L9 — the mutation-rider automation.** **Stays open, deferred by spec §3** ("Out: … the FU309-L9 mutation-harness automation"). Stage-2 re-applies the seven one-line reversions manually per producer task and records each RED; the deferred durable form is a small script that applies each reversion into a temporary worktree and asserts the fixture fails (owner: `tests/test_engine_match_frame.c` + the acceptance procedure).
4. **FU313-L1 — the probe's camera assertions need the producer.** P6-P8 are RED until FU310 (bootstrap) and FU312 (event) land; they are written in the acceptance task, RED-first, so `make check` is never red between commits. Owner: the phase-30 acceptance task.
5. **FU313-L2 — the input-timing coupling.** A held-direction tail appended after `M2_KEYS` suppresses the natural phase-2 entry (measured twice; §8.4.2). The cause is unproven — the candidate is the row-01 kickoff release gate consuming the slot edge. Owner: `fifa96_match_run_input` + the row-01 body (`match_run.c:436-455`, the kickoff release path). Until settled, the probe replays the measured tape and appends only after phase 2.
6. **FU313-L3 — the ladder-vs-stage-select attribution split.** A `ran`-only observable cannot distinguish "the ladder gate never fired" from "the AX prepass refused"; the probe and any future trace must carry both counters. Owner: the acceptance procedure (+ FU311's register text, which should quote the measurement in §8.4.1).
7. **FU313-L4 — the bbox threshold is proposed, not measured.** `864x443` (0.9×960 × 0.75×591) is derived from "a camera translation moves the whole scene", not from a live follow capture (none exists yet). The acceptance measures the first real pair and records the observed bbox; the threshold is re-pinned with that datum, never silently. Owner: the Layer-2 acceptance task.
8. **FU313-L5 — no visible-camera capture in the repo for a pan.** `docs/screens/*` has no whole-scene-shift pair; the phase-9 AE data are entity-motion deltas. The first live follow capture therefore has no pixel precedent to compare against — only the AE magnitude guidance (§6.5). Owner: the Layer-2 acceptance task.

---

## 10. GO/NO-GO

**GO — the harness.** The cheapest honest shape is frozen:

- **file/API:** `fifa96_match_run.h` (+`<stdio.h>`, `FILE *cam_trace`, `uint32_t cam_trace_frame`), `fifa96_match_run.c` (init zero, `begin` open from `FIFA96_CAMERA_TRACE` + reset the index, `end`/`teardown` close, one `fprintf` at `:4543`, the saved follow return at `:4383`); `tests/test_engine_m2.c` (the sampler + the `keeper_cpu.ladder` wrapper + P1-P10 in `run_natural_probe`).
- **no new target, no new framework, no new test binary, no golden change**; the trace is inert unless the env names a path; the probe's asserts are ISO-gated exactly like the existing drawing asserts.
- **the four links** are carried by the matrix (§7), and the measured baseline is RED precisely at the property the phase exists to create.

**NO-GO only on** treating the AE as attribution (forbidden, §6.6 rule 1) or landing P6-P8 before their producer (would leave `make check` red across commits). Neither is part of the harness as specified.

---

## 11. Provenance

**Read-only, this session.** No Ghidra call was required: the native anchors are *carried* — FU309 (`docs/ghidra/FU309_r2l4_event_source.md:9, 95, 155-190, 244-253`; the arm, the window formula, §5.4/§5.5, L5/L6), FU307-L1 (`docs/ENGINE.md:2334-2344`), FU157 §1 (the zero kickoff reset, carried), FU171-L1 (the 52-frame pin, carried), FU168 (the pan band, carried), the phase-29 register (`docs/ENGINE.md:2234-2298`, `:2408-2424`).

**Engine/test sources read:** `src/fifa96_engine/fifa96_match_run.c` (400-519, 1795-1834, 1860-1966, 2069-2100, 2248-2347, 3920-4039, 4319-4567, 4582-4618, 4752-4792); `src/fifa96_engine/fifa96_engine.c` (145-274); `src/fifa96_engine/fifa96_main.c` (1-48); `src/fifa96_engine/platform_null.c` (1-79); `src/fifa96_engine/fifa96_match_entities.c` (735-856); `src/fifa96_loader/fifa96_keeper_cpu.c` (27-41, 43-108, 141-234); `src/fifa96_loader/fifa96_camera.h` (1-140); `include/fifa96_engine/fifa96_match_run.h` (1-30, 296-320, 596-625); `include/fifa96_engine/fifa96_keys.h`; `include/fifa96_engine/fifa96_platform_null.h`; `include/fifa96_loader/fifa96_keeper_cpu.h` (38-184); `tests/test_engine_m2.c` (1-140, 1704-1730, 1737-1843, 1932-2115, 3330-3460, 4840-4870); `tests/test_engine_match_frame.c` (1-201, 837, 1144-1151, 2285-2352, 2470-2525, 3650-3729, 7180-7243); `tests/test_engine_match_input.c` (1-140); `CMakeLists.txt` (621-632); `Makefile` (32-41).

**Measurements:** `sha256sum` + `wc -l` on `tests/golden/engine/m1-frames.txt` / `m2-frames.txt`; `./build/test_engine_m2` (rc=0, stderr natural-probe line, stdout `cmp`-identical to the golden, 295 087 B); `/tmp/opencode/fu313_probe*.c` (five builds: the hand-rolled prefix; the verbatim tape + a held tail; the same without the seam; the exact tape; the exact tape + the stage-select seam), run from the repo root with `ASAN_OPTIONS=detect_leaks=0`; `command -v import xdotool convert compare ffmpeg`; `ls game/` (the ISO present).

**No writes:** no repo file created or modified except this draft; no Ghidra rename/comment/script/save; no git; `/FIFA96.EXE` and the ISO untouched. The throwaway probes and outputs live under `/tmp/opencode/` only.

---

## Review erratum (wave review, 2026-10-21)

The frozen text above stands; only the following claims change.

1. **The trace-open line.** Quote: "Opened `\"w\"` (one file per run) with
   `fopen` at the **end of `fifa96_match_run_begin`**, immediately before its
   `return 0` (`src/fifa96_engine/fifa96_match_run.c:2593`)". Fresh:
   `fifa96_match_run_begin` is `2248..2596`; the `return 0` is at **2596**
   (2593 is the phase restore). The site is still "immediately before return
   0". *Stage-2 consequence:* anchor the patch hunk at 2596.

2. **The trace's "only artefact" is post-landing.** Quote: "The trace is the
   **only** artefact that attributes motion to the camera." Until the trace
   code lands, the primary attribution is the probe's in-memory samples
   (P1–P10) or the read-only gdb sample; the trace becomes primary only when
   FU309-L5 closes. *Stage-2 consequence:* land the probe first (P1–P5 GREEN),
   then the trace, then the Layer-2 procedure.
