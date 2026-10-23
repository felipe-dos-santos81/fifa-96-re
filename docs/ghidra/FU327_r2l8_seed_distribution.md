# FU-327 — the measured no-slot seed distribution (phase-34 R2-L8)

**Phase/Provenance:** M2 phase-34 Stage-1 recon-ahead wave, **Task 2 of 4**
(`docs/superpowers/plans/2026-10-23-fifa96-m2-phase34-recon-ahead.md:63-84`),
serving `docs/superpowers/specs/2026-10-23-fifa96-m2-phase34-noslot-pan-design.md`
§3 (the measured distribution), §6 (the witness / NO-GO wording) and §9 item 2.
**Measured, out-of-tree, read-only.** No Ghidra read this session: the native
seed algebra is **carried** from FU316 §3.2/§4.3 (and its Review erratum) and the
sibling FU326 §4.1-§4.5 (the arm, the `word[+0x73]` domain, `T(s)`); this slice
measures the current tree's port only. The probe is a copy of
`tests/test_engine_m2.c` (`/tmp/opencode/fu327/probe.c`, sha256
`baf85616…00b4`) with a dump-gated `main` (`FU327_MODE`) and a frame dump
(`FU327_FRAME_LOG`), linked against the repo's prebuilt archives in
`build/CMakeFiles/test_engine_m2.dir/link.txt` order; the dispatch-time cells
come from one out-of-tree instrumented engine TU
(`/tmp/opencode/fu327/fifa96_match_handlers.c`, sha256 `6a254f76…2315`, the
repo TU + an env-gated logger; the diff is `handlers_instr.diff`, 192 lines).
**Build line (published):**

```
/usr/bin/cc -std=gnu11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -fno-sanitize-recover=all -I$REPO/include -I$REPO/src -c probe.c -o probe.o
/usr/bin/cc -std=gnu11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -fno-sanitize-recover=all -I$REPO/include -I$REPO/src \
  -c fifa96_match_handlers.c -o fifa96_match_handlers.o      # instrumented TU
/usr/bin/cc -fsanitize=address,undefined probe.o \
  [fifa96_match_handlers.o] -o probe_{unpatched,instr} \
  $REPO/build/libfifa96_engine.a … (link.txt order, the same 43 archives)
```

Run from the repo root (the ISO resolves), ISO present, read-only. **The
unpatched probe reproduces the committed golden byte-for-byte before any
instrumentation**: stdout sha256 `d5446144f08c22c5a4d986e1d07529068f715ff8b8e2cd86411ee493ea804ad8`
(295 087 B, `cmp` clean to `tests/golden/engine/m2-frames.txt`:
`/tmp/opencode/fu327/m2_unpatched.txt`). The
instrumented binary reproduces the same sha (`m2_instr_golden.txt`), so the hook
is observation-only. Scratch artifacts: `/tmp/opencode/fu327/{probe.c,
fifa96_match_handlers.c, handlers_instr.diff, probe_dump.diff, build.sh,
analyze.py, probe_unpatched, probe_instr, m2_unpatched.txt,
m2_instr_golden.txt, events_*.txt, frames_*.txt, events2_*.txt, frames2_*.txt,
logs/}`. **The only repo write is this draft**; no git; no ISO/build/golden
write; the census holds `60/80`.

---

## 1. The question

> **Over the current tree's measured passes — the undriven natural window
> (`M2_KEYS` verbatim, 12 000 steps) and the two driven passes (claim, offset) —
> what is the row-04 gate-pass seed distribution: per pass the gate tuple, the
> slot state, the seed arm and word, `v0`, the event cells, the place delta and
> the pan? Does **any** pass carry a no-slot `|v0| ≥ 1`; if not, which velocities
> would be needed? And does the phase-33 silent bind (granted f=1517, zero seed,
> `trk=12`, no motion) reproduce?**

Bindings: spec §3/§6/§9 item 2; FU316 §3.2/§4.3 + Review erratum (the carried
arm `0x7F056`, `word[+0x73] << 2`, `v0 = trunc(s/0xC)`, `T(s)`); FU326 §4 (the
arm/domain, the `|w| ≤ 2` silent band, `T` map); FU323 §2/§3/§9 (the probe
method, the phase-33 baseline); the harness `tests/test_engine_m2.c`
(`run_natural_pass` :3858, `run_natural_claim_pass` :4381,
`run_natural_offset_pass` :4758); the port
`src/fifa96_loader/fifa96_outfield.c:685-720` (the seed staging) and
`src/fifa96_engine/fifa96_match_handlers.c:2644-2645` (`vel_int` = `word[+0x73]`).

---

## 2. The one-page answer

**Five row-04 gate passes were measured, and none is a moving no-slot pass.**
One is the undriven no-slot bind (entity 12, `has_slot 0`, arm `0x7F056`, seed
`(−4,0)`, `v0 (0,0)`, `T = 0`); the other four are slot-arm controls (entities 2
then 1 in each driven pass, `has_slot 1`, arm `0x7F035`, seed `(−0x40,0)`,
`v0 (−5,0)`, `T = −0x90`). The no-slot instance **exactly reproduces the phase-33
silent bind**: granted f=1517 (engine step 5065), `trk=12`, `tim 0/12`,
`ramp 12`, `vel (0,0)`, `speed 0`, place `(0,0,0)` on all 3 598 granted frames
(`camera_zero_frames` 1517 / `camera_left_zero` 2081, `segments 0`,
`changed_frames 0`, `off_zero_frames 0`).

**Answer: NO — no measured gate pass carries a no-slot `|v0| ≥ 1`.** The
no-slot seed is `s = 4·w`, `w = word[+0x73]` (signed); the port's fast path gives
`v0 = trunc(s/0xC)`, so `|v0| ≥ 1` needs `|w| ≥ 3` (`|s| ≥ 12`; FU326 §4.2/§4.4's
`|w| ≤ 2` silent band). The measured band record's `+0x73` is **`{0,−1}` on all
3 598 frames** (2 328 / 1 270), and the **formation-wide maximum, over both
axes, is 1** on every frame (all 22 records). The measured gap to a
witness is therefore two whole velocity units: `|w| = 3` is the smallest moving
no-slot seed (`T = ±12`, six ±2-unit frames — a creep), and the measured `0x90`
travel needs `|w| = 15` (the `0x10F680` locomotion maximum, `s = 0x3C`).

The **four slot-arm controls** behave exactly as the carried model: seed `−0x40`
→ `v0 = −5` → the measured decay `5,3,2,1,0` over `6,7,7,7` frame groups → 27
changed frames → `T = −144 = −0x90`, pan `0→4`; two chained passes per driven
run accumulate to `−0x120` (`stop (−288,0,0)`), the second event resetting
`pan_counter 4→0`.

Two carried readings do not reproduce and are recorded as legs (prior text
stands; this is the correction): the phase-33 band-frame tuple's `active 2` /
`dir (−1,0)` cells are **not** the current tree's dispatch state (`active 1`,
`dir (0,0)`; FU323's own `anim_patched.txt` step-5062 line has `active 1`), and
the driven passes' second segment is **a row-04 event of entity 1**
(`ecode 0x1D`, seed `−0x40`), not a "code-3 chase walk's own camera event" — the
harness's `event.code 3` is entity 2's pool cell at that frame.

**GO on the distribution register; NO-GO on a natural no-slot pan witness in
these passes** (the Stage-2 verdict is FU328's sweep + FU329's acceptance).

---

## 3. Method, and the golden gate

1. **The probe (unpatched).** `probe.c` = `tests/test_engine_m2.c` verbatim plus
   two env-gated additions: a `FU327_MODE=natural|claim|offset` branch at the top
   of `main` that runs exactly one pass (no riders, no transcript) and a
   per-granted-frame dump (`FU327_FRAME_LOG`). Unset, `main` is the repo's
   verbatim golden producer. The full output is **byte-identical** to
   `tests/golden/engine/m2-frames.txt` (`d5446144…`, 295 087 B, `cmp` clean;
   `/tmp/opencode/fu327/m2_unpatched.txt`) — the published pre-instrumentation
   gate. M1 is not re-run here: the repo engine is untouched (the probe's
   instrumented TU is an out-of-tree copy), and FU323 pinned M1 byte-identical.
2. **The dispatch sampling.** The harness alone cannot see a *zero-seed* gate
   pass (the delta-run machine only records motion), so the dispatch cells come
   from one out-of-tree copy of `fifa96_match_handlers.c` with an
   observation-only block in `fifa96_match_action_04`: it recomputes the ported
   cone predicate (near test `0x7ED9B` → bound `0x7EDCE` → `camY <= 0x50`
   `0x7EDDF` → folded angle `<= 0xAB` `0x7EE7D`) from the same staged `s` the
   ported body reads, recomputes the seed arm/word (`outfield.c:685-701`), and
   logs after the body (`FU327PASS`) and after the camera set (`FU327POST`).
   The recomputation is checked against the body's own output on every pass
   (`out.event_code == 0x1D`, `out.event_x/z` == the recomputed seed; 5/5).
   The instrumented binary reproduces the golden byte-for-byte, so the hook is
   observation-only.
3. **The frame sampling.** One line per granted frame: the camera cells (pos,
   vel, speed, follow, `event_param`, cursor, tracked, pan, timer/limit, ramp,
   rate), the band record's cells (code, slot, active, anim row, `timer81`,
   lane/word6d/word6f, bound, face, dir, pos, `+0x73/+0x75`), and the
   formation-wide `max |+0x73/+0x75|` over the 22 records with the counts of
   records at `|w| ≥ 3` (and of those on row 04).
4. **Exposure.** `FU327TOTAL` at exit counts every `fifa96_match_action_04`
   call, every cone pass, every `0x1D` event arm and every no-slot arm, so the
   sample is the whole window, not a filter: natural `1960 / 1 / 1 / 1`; claim
   `2095 / 2 / 2 / 0`; offset `2093 / 2 / 2 / 0`.
5. **Stability.** Each pass was re-run in a second process; both the event log
   and the 3 598-line frame log are byte-identical (`events2_*`/`frames2_*`).

---

## 4. The measured distribution

### 4.1 The gate tuple and the seed arm (per gate pass)

| # | pass | event f | id | dispatch `code` | `active` | `anim` | `timer81` | `pos (x,y,z)` | lane `+0x6B/+0x6D` | `bound` | `camY` | `cam (x,y,z)` | `face` | `bearing` | Δ | `has_slot` | `is_track` | arm | seed `(x,z)` | `v0 (x,z)` |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | natural | 1517 | 12 | 4 | 1 | 0 | 0 | 64,0,0 | 64,−64 | 66 | 0 | 0,0,0 | −256 | −256 | 0 | **0** | 0 | **0x7F056** | **(−4,0)** | **(0,0)** |
| 2 | claim | 1481 | 2 | 4 | 2 | 0 | 0 | 64,0,0 | 64,−64 | 66 | 0 | 0,0,0 | −256 | −256 | 0 | 1 | 0 | 0x7F035 | (−64,0) | (−5,0) |
| 3 | claim | 1553 | 1 | 4 | 1 | 0 | 0 | −80,0,0 | 64,−64 | 66 | 0 | −144,0,0 | −256 | −256 | 0 | 1 | 0 | 0x7F035 | (−64,0) | (−5,0) |
| 4 | offset | 1483 | 2 | 4 | 2 | 0 | 0 | 60,0,16 | 64,−60 | 66 | 0 | 0,0,0 | −256 | −298 | 42 | 1 | 0 | 0x7F035 | (−64,0) | (−5,0) |
| 5 | offset | 1553 | 1 | 4 | 1 | 0 | 0 | −80,0,0 | 64,−64 | 66 | 0 | −144,0,0 | −256 | −256 | 0 | 1 | 0 | 0x7F035 | (−64,0) | (−5,0) |

All five pass the cone (`cone=1`), all have `row13=0` (the row-`0x13` arm,
code `0x15`, never fires) and `is_track=0` (none is the tracked record at its
dispatch; the natural pass is the pre-bind state, `tracked_pre −1`). The height
arm (`0x7F003`) needs `camY != 0` and is likewise unexercised. `face` is
`−256` = `0x300` in the 10-bit fold; the bearing is `fifa96_entity_angle(+0x6D, +0x6F)`.
The `camY` cell is the port's `ball_height` (the native `dword[0x157750]`),
staged from `camera.pos_y`; zero at every pass. The offset #1 `lane/pos` split
(lane `64/−60` vs `pos 60,0,16`) is real: `lane_x` is the pool `+0x6B`, `pos`
the staging record's position — the same two cells the native reads.

**The arm census: `0x7F056` ×1, `0x7F035` ×4, `0x7F003` ×0, the `(0,0)` bail
×0.** `v0` is `trunc(seed/0xC)` with C truncation toward zero (the port's
`camera.c:770-772`, measured `−64 → −5`, `−4 → 0`).

### 4.2 The event cells, the place delta and the pan (per pass)

| # | pass | event f | `trk` after | cam `vel` | `speed` | `tim/lim` | `ramp` | `pan` at event | place Δ | motion frames | pan run | `stop` f | place after |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | natural | 1517 | **12** | (0,0) | 0 | 0/12 | 12 | 0 | **(0,0,0)** | **0** | 0 | — | 0,0,0 |
| 2 | claim | 1481 | 2 | (−5,0) | 5 | 0/12 | 12 | 0 | (−144,0,0) | 1482..1508 (27) | 0→4 | 1509 | −0x90 |
| 3 | claim | 1553 | 1 | (−5,0) | 5 | 0/12 | 12 | 0 (was 4) | (−144,0,0) | 1554..1580 (27) | 0→4 | 1581 | −0x120 |
| 4 | offset | 1483 | 2 | (−5,0) | 5 | 0/12 | 12 | 0 | (−144,0,0) | 1484..1510 (27) | 0→4 | 1511 | −0x90 |
| 5 | offset | 1553 | 1 | (−5,0) | 5 | 0/12 | 12 | 0 (was 4) | (−144,0,0) | 1554..1580 (27) | 0→4 | 1581 | −0x120 |

The driven passes' per-run totals: `segments 2`, `changed_frames 54`,
`off_zero_frames 2116` (claim) / `2114` (offset), `pan_max 4`,
`stop (−288,0,0)`, `Y/Z nonzero 0/0`, `armer 0`, `OOB 0`. The second event
clears the camera (the `0x700F4` subset: `tracked` NONE, `vel 0`, `speed 0`,
`pan_counter 0`, `timer/timer_limit` zeroed) and then rebinds — so the place
accumulates while the pan counter restarts.

**The measured travel arithmetic** (all from the frame log, no remembered
formula): at the event frame the velocity is set but not yet integrated — the
camera walk runs before the entity dispatch in the frame body
(`fifa96_match_run.c:4477` vs `:4535`), so the first changed frame is the next
one. Measured per segment: `v = 5` for 6 frames (Δx −10/frame), `v = 3` for 7,
`v = 2` for 7, `v = 1` for 7, then 0 — `2·(6·5 + 7·3 + 7·2 + 7·1) = 2·72 = 144
= 0x90`, exactly `T(s = −0x40)` from FU316 §4.3 / FU326 §4.3. The measured
`timer` reaches `12/12` on the sixth frame of each velocity, the `ramp`
stays 12, the `pan_counter` steps 1 per velocity change.

### 4.3 The no-slot instances (the classification)

| quantity | measured | note |
|---|---|---|
| count | **1** | natural f=1517, entity 12 |
| max `\|seed\|` | **4** | `(−4,0)`; `\|w\| = 1` |
| max `\|v0\|` | **0** | `trunc(4/12) = 0` |
| `T` | **0** | `speed 0`, `event_param 0` — the integrator freezes |
| place motion | **0 frames** | `pos (0,0,0)` on all 3 598 frames |
| event/bind | fires | `trk=12`, `tim 0/12`, `ramp 12` |

The slot-arm controls (the four driven passes), for the same table: count 4;
`|seed| = 0x40`; `|v0| = 5`; `|T| = 0x90`; 27 changed frames each; place
`−0x120` after two.

### 4.4 The measured velocity population (the condition)

| population | measured |
|---|---|
| band record (entity 12) `+0x73`, 3 598 frames | `0` ×2 328, `−1` ×1 270 (no other value) |
| band record `+0x75` | `0` on every frame |
| formation-wide `max \|+0x73\|`/`\|+0x75\|`, every frame | **1** (0 on 1 074 frames, 1 on 2 524) |
| records at `\|w\| ≥ 3`, every frame | **0** (on any row; on row 04: 0) |
| band record `has_slot`, 3 598 frames | 0 (no slot at any frame) |
| band record `code` | 4 ×979, 5 ×2 081, 3 ×36, 36 (`0x24`) ×502 |

The `+0x73/+0x75` words are the record's 16.16 integer velocity (the mover stages
`s.vel_x = r->vel73`, `match_run.c:518`; the integrator's delta 2 gives the
measured −2 units/frame), so the natural band record walks at 1 unit/frame, not
the ≥3 a moving no-slot pan requires.

---

## 5. The baseline reproduction (the phase-33 silent bind)

Measured on the **unpatched** probe (the harness's own stderr, quoted below) and
reproduced under the instrumented binary; the natural pass was re-run in both
mode runs and the frame/event logs are byte-identical each time.

```
test_engine_m2 natural probe: ... phase 2 at step 1802, 3598 granted frames (101 s),
  camera zero 1517/3598, ...
test_engine_m2 natural probe: first camera-cell departure at step 5065 (granted f 1517,
  pos (0,0,0)); first bind f 1517 id 12 trk 12 (spd 0, tim 0/12, ramp 12, ev 0,
  vel (0,0), pos (0,0,0), band code 5); delta-run machine: segments 0,
  first_motion f -1, stop f -1, changed 0, off-zero 0, Y/Z nonzero 0/0,
  row04-after 0, armer 0, OOB 0, pan_max 0
```

Every phase-33 baseline cell reproduces: `trk=12` at granted f=1517, zero seed,
zero velocity/speed, `tim 0/12`, `ramp 12`, place never moves, `camera_zero_frames`
1517, `camera_left_zero` 2081, `segments 0 / changed 0 / off_zero 0`, `pan_max 0`,
`row04_after 0`, `armer 0`, `OOB 0`, ladder seam `6118 / ran 0`. The dispatch
view adds the seed word and the arm: `ex=−4` (`w=−1`, `0x7F056`), `v0=0`; the
row-04 body installs row 5 after the event (`band code 4` at the dispatch,
`5` after the frame) and the bind's camera cells are exactly the event's
(`FU327POST`: `trk 12`, `vel (0,0)`, `spd 0`, `tim 0/12`, `ramp 12`, `pan 0`).

### 5.1 Reconciliation with the carried table (quoted, not rewritten)

FU323 §4's row says:

> "| the gate tuple at the band frame | — | `code 4`, `active 2`, `anim 0`,
> `timer81 0`, `has_slot 0`, `dir (−1,0)`, `pos (64,0)`, `cam (0,0)`,
> `cam_dz6f 0`, `bound 66`, `lane_x 64`, `lane_z −64`, `spd71 1`, `tracked −1`,
> `controlled 1` | the gate passes ✓ (on the approach: 64 ≤ bound 66) |"

**Measured:** `code 4`, `anim 0`, `timer81 0`, `has_slot 0`, `pos (64,0,0)`,
`cam (0,0,0)`, `bound 66`, lane `64/−64`, `controlled 1`, `tracked −1` all
reproduce (the f=1517 dispatch view agrees with the step-5062 band-frame line),
and `active 1`, `dir (0,0)` are the current tree's cells. FU323's own artifact
`/tmp/opencode/fu323/anim_patched.txt` line `5062 1 1 4 1 0 2 -256 6 1 64 0 0 0
64 -64 0 66 0 0 0 1 -1` (steps, team, record, code, **active**, anim, row43,
face, type, speed71, pos x/z, target, lane x/z, cam_dz6f, bound, cam x/z,
has_slot, controlled, tracked) has `active 1`; the a12 pool `dir` is `(0,0)` on
all 3 598 frames. So the carried row's `active 2` is a transcription slip against
FU323's own dump, and `dir (−1,0)` is not in that dump and does not reproduce —
neither cell affects the no-slot seed (the arm never reads the slot dir). The
bind identity (f=1517, `trk=12`, zero seed, no motion) is exact.

FU323 §5.1's caption "its passing dispatch is no longer the row-04 code-4 gate
but the **code-3 chase walk's own camera event**" is **not supported** by the
dispatch view: the second segment's event is a row-04 dispatch of **entity 1**
(`ecode 0x1D`, seed `−0x40`, `trk 1`), and the harness's `event.code 3` is
entity 2's pool code at that frame (`m2_claim_snapshot` always reads
`M2_CLAIM_ENTITY = 2`). The frozen pins are pool-cell pins and stand; only the
attribution changes (FU327-L3).

---

## 6. The answer

**Does any measured gate pass carry a no-slot `|v0| ≥ 1`? No** — not in the
natural window, not in the claim pass, not in the offset pass.

**The measured conditions a witness would need** (all five are necessary):

1. a record **without a slot** (`has_slot 0`) at a row-04 gate pass;
2. `camY == 0` (the idle arm) and `|cam_x| ≤ 0x630`, `|cam_z| ≤ 0xA20` (the
   `0x7F056` bail — untested here, the camera is `(0,0,0)` at the only no-slot
   pass);
3. not the tracked record at its dispatch (`is_track 0`);
4. the cone: `lane ≤ 0x40 − timer81`, `lane ≤ bound`, folded `|bearing − face|
   ≤ 0xAB`;
5. **`|word[+0x73]| ≥ 3`** (and/or `|+0x75| ≥ 3`) — the seed `s = 4w`,
   `v0 = trunc(s/0xC)`, so `|w| = 0..2` is the silent band and `|w| = 3..5`
   gives the minimum moving seed `|v0| = 1`, `T = 12` (a six-frame creep);
   `|w| = 15` (the `0x10F680` maximum, FU326 §4.2) gives `|v0| = 5`,
   `T = 0x90` — the same travel the slot-arm control measured.

Measured: condition 1 holds once (entity 12, natural f=1517), conditions 2-4
hold there, and **condition 5 fails by two units**: the record's `+0x73` is
`−1`, and no record in the formation ever exceeds `|1|` on any frame of the
window.

---

## 7. Numbered legs

* **FU327-L1 — the natural tape's no-slot seed source is parked, and the whole
  formation is `|w| ≤ 1`.** Measured: entity 12's `+0x73 ∈ {0,−1}` on all 3 598
  frames; the per-frame maximum over the 22 records is 1 (per-frame histogram 0
  ×1 074, 1 ×2 524); zero records at `|w| ≥ 3` on any row, any frame. A moving
  no-slot pass needs `|w| ≥ 3` at its gate (FU326 §4.2/§4.4); the measured gap is
  two integer velocity units. Owner: **FU328** (the reachability sweep — the
  runner/shape that could put `|w| ≥ 3` on a slot-less record in the cone) and
  `FUN_0008E244`'s stride producer (FU326-L1's non-locomotion writers are not
  exercised: nothing above `|1|` appears in this window).
* **FU327-L2 — the carried band-frame tuple's `active`/`dir` cells do not
  reproduce.** FU323 §4's `active 2` / `dir (−1,0)` vs the dispatch state
  `active 1` / `dir (0,0)`; FU323's own `anim_patched.txt` step-5062 line has
  `active 1`; the a12 `dir` is `(0,0)` on every frame. The bind identity is
  exact. Owner: the Stage-2 baseline wording (FU329 §7.1) — quote the artifact
  line, never the table row.
* **FU327-L3 — the harness's pass-level `gate`/`event` records are entity-2 pool
  cells, not the firing record's.** The driven passes' second segment is a
  row-04 event of entity 1 (`ecode 0x1D`, seed `−0x40`); the `code 3` / `active
  2` / `slot 1` columns are entity 2's pool state at that frame. The frozen pins
  stand as pool-cell pins; Stage-2 must not read them as the firing record's
  code or row, and the FU323 §5.1 "code-3 walk camera event" reading is not
  supported (its erratum 1 already flagged the attribution as unestablished).
  Owner: the Stage-2 acceptance wording.
* **FU327-L4 — the moving no-slot travel ladder is arithmetic-only.** No
  measured instance exists at any `|w| ≥ 3`; the `T = 12/38/78/132/144` rows are
  FU326 §4.3/§4.4's closed form (recomputed here), corroborated only by the
  slot-arm `s = −0x40 → T = −0x90` control and the measured 6/7/7/7 cadence.
  Owner: FU328's drive (if any) or the next phase.
* **FU327-L5 — the no-slot camera-bounds bail (`0x7F081`, seed `(0,0)`) is
  unexercised.** The only no-slot pass is at `cam (0,0,0)`, far inside
  `|x| ≤ 0x630` / `|z| ≤ 0xA20`; its behaviour after a pan (a second no-slot pass
  with the camera off-origin) is unmeasured. Owner: FU326 §5 item 5 / FU328.
* **FU327-L6 — the height arm (`0x7F003`) and the row-`0x13` code-`0x15` arm are
  unexercised.** `camY = 0` and `row13 = 0` at all five passes; their seed words
  are not measured on this tape. Owner: none in phase 34 (outside the no-slot
  arm); recorded so no later slice reads a coverage it does not have.
* **FU327-L7 — the probe's cone predicate is a replica, not the port's bytes.**
  The evidence is the 5/5 agreement (`out.event_code == 0x1D`; `out.event_x/z`
  equals the recomputed seed) and the per-pass cone/event counts. The native
  branch order is FU326 §6 P1-P7's fresh parity, not re-derived here.

---

## 8. GO / NO-GO

**GO — the measured-distribution register.** The three passes are sampled
exhaustively (`cone_passes` equals `events_1d` equals the fired events: 1/2/2;
every row-04 dispatch counted), the numbers are stable over two processes, the
phase-33 silent bind reproduces cell for cell, and the driven controls pin the
`T(s)` arithmetic end to end (`5,3,2,1,0`, 6/7/7/7, 27 frames, `0x90`).

**NO-GO — a natural no-slot pan witness on the current tree's measured
passes.** No pass carries a no-slot `|v0| ≥ 1`; the tape's whole formation stays
at `|w| ≤ 1`, two units below the smallest moving seed. The distribution is the
evidence, not a defect: the port's arm and the harness behave exactly as FU326's
algebra predicts. The witness question passes to **FU328** (does any natural-play
shape give `|w| ≥ 3` on a slot-less record at the cone?) and the acceptance
design to **FU329** (Branch B lands this register verbatim; Branch A needs a
FU328 drive).

---

## 9. Known limits

* **Sampling.** One sample per granted frame, after the frame body; the
  dispatch-time cells are the instrumented handler's view of the same staged
  `s` (before that frame's installs/mover). The harness's pass-level
  `gate`/`event` snapshots are entity-2 pool cells (`M2_CLAIM_ENTITY = 2`), so
  they are quoted as pool cells, never as the firing record's cells (L3).
* **Scope of the arm sample.** Only the row-04 event arm is instrumented. On
  these three passes no other camera event fired (the camera cells show exactly
  the sampled events; the natural window's sole departure is the bind), but the
  statement is about these passes, not a census of all `0x71C94` sites.
* **ISO-only.** The ISO was present (`game/FIFAPCCD96.iso`, read-only); the
  ISO-gated passes and the formation seeding need it. The no-ISO natural shape
  is unmeasured (FU322-L10).
* **Window.** The natural window is `M2_KEYS`'s 12 000-step span (3 598 granted
  frames, 101 s of match clock) — the phase-30/31 measurement window, not a full
  match; the driven passes share the window and append their holds from tape
  index 1820.
* **The instrumentation.** One out-of-tree engine-TU copy (the logger) + the
  harness copy; the instrumented binary reproduces the golden byte-for-byte, but
  the *predicate* is a replica (L7).
* **The velocity population table** reads the pool dwords (`e->vel_x/vel_z`),
  the same words the dispatch stages (`r->vel_x = e->vel_x`); a record that is
  not dispatched on a frame carries its last written value, which is what the
  seed staging would read on its next dispatch.
* **No native re-derivation here.** The arm, the word packing, `v0` and `T(s)`
  are FU316/FU326's; this slice measures the port. The `v0` truncation and the
  `T` identity are confirmed only at the measured seeds (`−4`, `−0x40`).

---

## 10. Provenance and self-audit

| claim | evidence |
|---|---|
| the unpatched probe = the committed golden | `/tmp/opencode/fu327/probe_unpatched > m2_unpatched.txt`; sha256 `d5446144…`, 295 087 B, `cmp` clean against `tests/golden/engine/m2-frames.txt` |
| the instrumented probe is observation-only | `probe_instr` (no dump env) → `m2_instr_golden.txt`, the same sha `d5446144…` |
| the probe is a harness copy | `probe_dump.diff` (152 lines, 123 added): the `FU327_MODE` branch + the frame dump + the two dump calls; no other change |
| the handler hook is one block | `handlers_instr.diff` (192 lines, 162 added): the loggers + the `fifa96_match_action_04` pass/post block |
| 5 gate passes, 1 no-slot | `events_natural.txt` (PASS/POST/TOTAL), `events_claim.txt`, `events_offset.txt`; `FU327TOTAL … cone_passes=… noslot_arm=…` |
| the gate tuple / arm / seed / v0 per pass | `FU327PASS` lines (§4.1); `out.event_x/z` equals the recomputed seed on all five |
| the event cells / place / pan per pass | `FU327POST` lines + `frames_*.txt` (3 598 lines each); the motion windows and the 6/7/7/7 decay are the frame log's own |
| the baseline reproduction | `logs/unpatched.err` natural lines (quoted §5); `frames_natural.txt` f=1517..1522; `segments 0 / changed 0 / off-zero 0` |
| the no-slot word population | `frames_natural.txt` a12 `v73` histogram (2 328/1 270) and `maxv73/nv3/nv3row4` columns (max 1, zero at ≥3, zero on row 04) |
| the carried tuple reconciliation | FU323 §4 quote (§5.1); `/tmp/opencode/fu323/anim_patched.txt` line 5062; `frames_natural.txt` a12 `act`/`dir` columns |
| the second-segment attribution | `events_claim.txt`/`events_offset.txt` f=1553 (id 1, `ecode 0x1D`, seed −64); `m2_claim_snapshot` (`tests/test_engine_m2.c:4219-4257`) reads entity 2 |
| the source cells | `fifa96_outfield.c:685-701` (the arms), `fifa96_match_handlers.c:2644-2645` (`vel_int`), `:2707-2710` (`camY`/camera), `fifa96_camera.c:770-772` (`v0` divide), `:779` (speed), `fifa96_match_run.c:518` (mover `vel73`) |
| stability | `events2_*`/`frames2_*` byte-identical to `events_*`/`frames_*` (both passes, both runs) |

**Self-audit.** The only repo file written is this draft; no git, no ISO write,
no `build/` write, no golden touch (`tests/golden/engine/{m1,m2}-frames.txt`
unchanged: `09b726b7…` / `d5446144…`). The probe binaries, logs and dumps live
under `/tmp/opencode/fu327/`. Every number in §4-§6 is read from those artifacts;
the carried native facts are cited to FU316/FU326 and not re-derived. The two
carried readings that do not reproduce are reported as corrections with the
artifact line that decides them (L2/L3), not explained away.

Status: recon complete; **this draft is the only file written**; census
`60/80 = 75%`; M1/M2 unmoved; no git.

## Review erratum (wave review, 2026-10-23)

**The band record's walk rate (§4.4).** Quote: "the natural band record walks at 1 unit/frame, not the ≥3 a moving no-slot pan requires." **Corrected:** the record's velocity **word** is `+0x73 = −1` on the measured frames, and the integrator applies `delta = 2`, so the **place** steps **2 units/frame** (the frame log's a12 `pos_x` 66 → 64 → 62 → 60 at f 1515..1518; the mover's `s.vel_x = r->vel73`, `match_run.c:518`). The threshold a moving no-slot pan needs is `|word[+0x73]| ≥ 3` (place step ≥ 6), not "3 place units/frame". The measured conclusion is unchanged: `|w| ≤ 1` on all 3 598 frames, seed `|4|`, `v0 = 0`, no moving no-slot pass.
