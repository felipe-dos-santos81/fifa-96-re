# FU-328 — the natural reachability of a moving no-slot band entry (phase-34 R2-L8)

**Phase/Provenance:** phase-34 recon-ahead wave, Stage-1 **slice 3 of 4 — the
natural reachability sweep** (`docs/superpowers/specs/2026-10-23-fifa96-m2-phase34-noslot-pan-design.md`
§3/§6/§7/§9 item 3; `docs/superpowers/plans/2026-10-23-fifa96-m2-phase34-recon-ahead.md`
Task 3), 2026-10-23, read-only. Question: **which natural-play shapes produce a
no-slot row-04 gate pass while the record is moving (`|v0| ≥ 1`), and what is
the measured pan/band-entry frequency (FU316-L7)?** Method: an out-of-tree
build of the current tree (FU323 §3's method — engine TUs sanitized, loader TUs
plain, the repo's per-target flag split) whose **unpatched** `test_engine_m2`
reproduces the golden **`d5446144…`** (295 087 B) and `test_engine_m1`
**`09b726b7…`** (22 004 B) byte-for-byte, plus a measurement probe that wraps
`fifa96_camera_event_set` at link time and samples every granted frame.
Fresh Ghidra this session: `/FIFA96.EXE`, `0x7F030..0x7F0BE` (the `0x7F056`
no-slot seed arm and its `|cam|` guards) and `0x7E2BB..0x7E31F` (the row-03
placement counter). Carried as named: FU316 §3.1/§4.3 (the gate and the
`T(s)` model), FU323 §2/§4 (the measured silent bind), FU84 §6 (the `+0x79`
producer), FU177 §2/§6 (row 03), FU321 §3.4 (the closed form). Scratch,
build, probe and logs live under `/tmp/opencode/fu328/`. **No repo file was
created or changed except this draft; no git; the ISO read-only; `build/` and
`tests/golden/` untouched; census `60/80` unchanged.**

## 1. The question

> On the current tree, is there a **natural-play shape** (the verbatim
> `M2_KEYS` tape over longer windows, the second half, restarts, the
> phase-31/32 drive families, the claim/carrier contexts) whose row-04 gate
> pass is **no-slot and moving** — `has_slot 0`, `|v0| = |trunc(seed/0xC)| ≥ 1`,
> the event bound, the place travelling `T(s)` over the 5→3→2→1 decay — or is
> the honest answer a **NO-GO** with the measured distribution and the
> remaining legs? And what is the measured natural **pan count / band-entry
> rate** (FU316-L7)?

## 2. The one-page answer

**NO-GO on a natural witness — and the reason is a missing producer, not the
geometry.** Thirty-two runs were measured (every granted frame sampled, every
`fifa96_camera_event_set` captured with the arriving record's cells). The row-04 no-slot gate **does** fire on the natural
tape — exactly once, entity 12 at granted **f = 1517** (engine step 5065), the
FU323-L1 silent bind — and it fires a second time at the **moved** camera when
a kick/pass is driven (f = 1590), but **every no-slot event carries seed ±4 →
`v0 = 0` → no place motion**. Every real pan in every window is the **slot
arm** (`seed −0x40 → v0 = −5 → T = −0x90`, 27 frames; the §5.6 diagnostic
excepted): the only records that pan are the slot-bearing controlled/AI
records. The no-slot records do enter
the cone (entity 12, twice) but arrive at `|vel| = 1` and emit the zero-seed
bind.

The measured cap is exact and universal: over **all 22 records, every granted
frame, every final-probe window**, the larger of `|word[+0x73]|`/`|word[+0x75]|`
never exceeds **1** (the all-record histogram is 0/±1 only), and the
placement-counter nibble
`word[+0x7B] & 0xF` is **0 on every record-frame**. The chain is
`+0x7B & 0xF = 0` → `stride_len = max(1, 0 − err>>7) = 1` →
`attr = min(move_attr>>19, 1) ≤ 1` → `stride_table[attr] ≤ 1` (fresh table
probe: attr *k* caps |x|,|z| at *k*) → `|seed| = 4·|vel| ≤ 4` → `v0 = 0`. The
threshold the spec needs is `|vel| ≥ 3` (`seed = 0xC`). The root cause is
upstream of any input: **the port has no producer for the native placement
limit `+0x79`** — `timer79` is never written (`src/fifa96_engine/fifa96_match_entities.h:129`
is read-only; the only writers are tests), while the native reset
`FUN_0007997C` (`0x799F0..0x79A4D`, FU84 §6) sets it from the `0x10F394` byte
table to **6..15**, and the row-install reset copies it into `+0x7B`
(`entities.c:94`, `0x7DA9B`). The row-0x03 counter body — the native
maintenance — is a second gap (the dispatch row is NULL,
`fifa96_match_handlers.c:6346-6347`; its staged `move_attr` is hard 0,
`:5821`).

A **diagnostic** (never a witness, never in the sweep table) proves the chain
from the other side: staging `+0x79 = 3/6/15` on the natural tape — no slot,
seed or face cell touched — makes the same natural band record emit
`(|seed|,|v0|,T) = (0xC,1,0xC)`, `(0x18,2,0x26)`, `(0x34,4,0x84)` at f = 867,
704 and 607, with the exact 6/13/27-frame decay. The geometry, the arm, the
seed algebra and the travel are all live; **the missing `+0x79` is the whole
NO-GO**.

The frequency evidence (FU316-L7): undriven, **1 no-slot band entry per 3 598
granted frames** (0.028 %) and **0 pans**; the extra 24 660 granted frames the
96 000-step window buys are the **stalled half-time kickoff** (phase 1, clock
120 s, step 13805 onward), not play. Driven, **2 slot pans per 3 598 granted
frames** (0.056 %), still **0 no-slot pans**. The second half is unreachable
undriven without the directive path; seven KICK-release offsets and a DOWN tail
do not resume it.

## 3. The target shape (from the physics)

### 3.1 The gate set (carried, FU316 §3.1; port fresh)

The native row-04 half-line arm admits a record when

```
lane <= 0x40 - timer81          (0x7ED89..0x7ED9E)
lane <= bound                   (0x7EDCE..0x7EDD2)
camY <= 0x50                    (0x7EDD8..0x7EDDF)
folded |bearing(rec->cam) - face7d| <= 0xAB   (0x7EE4B..0x7EE7D)
rec != [0x1577CA] (tracked)     (0x7EFEE)
```

with `lane = entity_distance(cam - pos)` (the octagonal radius from the camera,
`fifa96_action_locomotion_track`, `action_handlers.c:335-349`). The port mirrors
it at `fifa96_outfield.c:610-635` and the arms at `:682-707`. A **no-slot**
record (`has_slot 0`), with the camera parked at `Y = 0` (`ball_height 0`) and
inside the outer guards `|cam_x| ≤ 0x630`, `|cam_z| ≤ 0xA20`, takes the
`0x7F056` arm.

### 3.2 The seed and the travel

Fresh bytes `0x7F08F..0x7F0A2` (this session):

```
0x7f08f  668b4573       MOV AX,word[EBP+0x73]     ; the record velocity word
0x7f093  c1e002         SHL EAX,0x2               ; *** seed_x = w << 2 ***
0x7f096  6689442402     MOV word[ESP+0x2],AX
0x7f09b  668b4575       MOV AX,word[EBP+0x75]
0x7f09f  c1e002         SHL EAX,0x2               ; *** seed_z ***
0x7f0a2  6689442404     MOV word[ESP+0x4],AX
```

(fresh, with the guards at `0x7F056..0x7F07F`: `|cam_x| ≤ 0x630` else `(0,0)`,
then `|cam_z| ≤ 0xA20` else `(0,0)`). The port stages the same words at
`fifa96_match_handlers.c:2644-2645` (`vel_int_x = (int16_t)(r->vel_x >> 16)`,
i.e. `word[+0x73]`) and applies them at `fifa96_outfield.c:692-701`.

The seed feeds `FUN_00071C94` (`0x7F0D1`); the idle arm's `F8 = 0xC`
(`fifa96_camera.c:753-762`), so on the fast path (`pan_counter == 0`)

```
v0 = trunc(seed / 0xC)                 ; seed = 4*w  ->  v0 = trunc(w/3)
v_{i+1} = trunc(3*v_i / 4)
T(s) = 2 * (6*v0 + 7*v1 + 7*v2 + ...)  ; frames 6, then 7 each
```

(carried FU316 §4.3; the tree's `m2_pan_curve`, `tests/test_engine_m2.c:2467-2481`).
**The target threshold is `|w| ≥ 3`** (seed `0xC`, `v0 = ±1`). The table by the
velocity word `w`:

| `w` | seed | `v0` | `T` | frames |
|---|---|---|---|---|
| 0 | 0 | 0 | 0 | 0 |
| 1, 2 | 4, 8 | 0 | 0 | 0 |
| **3, 4, 5** | 0xC, 0x10, 0x14 | ±1 | ±0xC | 6 |
| 6, 7, 8 | 0x18, 0x1C, 0x20 | ±2 | ±0x26 | 13 |
| 9, 10, 11 | 0x24, 0x28, 0x2C | ±3 | ±0x4E | 20 |
| 12, 13, 14 | 0x30, 0x34, 0x38 | ±4 | ±0x84 | 27 |
| 15 (the live max) | 0x3C | ±5 | ±0x90 | 27 |

The live domain is **`|w| ≤ 15`** (the `0x10F680` stride table's min −15/max +15,
FU326 §4.2), so the no-slot travel ceiling is **`T = 0x90`**, not FU316 §4.3's
`0x174`: that row's `w = 0x20` (`s = 0x80`) is outside the no-slot arm's live
domain (FU326 §8). FU316's `w = 0x10` → `s = 0x40` example is likewise just
outside the bounded domain (`w = 16`), so it is not a live no-slot seed
either.

### 3.3 The AI/slot state that produces it

A **code-4** record (it dispatches row 04), `active`, `has_slot 0`, with

1. a **placement counter** `word[+0x7B] & 0x0F ≥ 3` — the native counter is
   `min(([+0x63]>>22)+1, word[+0x79])` (fresh `0x7E2E0..0x7E2F4`: `SAR
   EAX,0x16; INC; MOV word[+0x7B],AX; CMP AX,[+0x79]; JBE; MOV word[+0x7B],DX`),
   written by the row-03 idle core and carried into row 04 on the code switch;
2. a target at least `0x18` away (`move_attr>>19 ≥ 3`);
3. a **face near the bearing to the camera** — the live focus (`[0x157A73]` →
   the camera triple) turns the record onto that bearing, so a record running
   *toward* the camera has a small facing error and keeps its stride
   (`stride_len = (counter) − (face_error>>7)`); the measured band record's
   face is exactly the bearing (`0xFF00` vs `0xFF00`) at the gate.

The natural instance is **entity 12** (team 1 record 1): the formation AI's
code-4 outfield record with no slot. In the port it reaches the gate at the
correct geometry but with `+0x7B & 0xF = 0`, i.e. with the wrong *speed*.

### 3.4 The port-side cap (every link measured or cited)

| link | port site | measured this slice |
|---|---|---|
| placement limit `+0x79` | *no producer* (`timer79` read-only; tests only) | 0 on every record-frame |
| placement counter `+0x7B` | reset `= timer79` (`entities.c:94`, `0x7DA9B`); row-03 body NULL (`handlers.c:6346`) and staged `move_attr 0` (`:5821`) | nibble 0 on every record-frame |
| `stride_len` | `action_handlers.c:257-259` | 1 |
| `attr` | `action_handlers.c:260-261` | ≤ 1 |
| stride word `+0x73` | table[`(face&0x3F0)|attr`], `action_handlers.c:262-264` | ≤ 1 (fresh table probe: attr *k* → max *k*) |
| seed | `outfield.c:697-698` | ≤ 4 |
| `v0` | `camera.c:771-772` (`seed / 0xC`) | 0 |

The native chain is the same computation with `+0x79 = 6..15` (FU84 §6) and the
row-03 counter live; the port's two missing producers are FU328-L1/L2.

## 4. Method (the out-of-tree build and the probe)

### 4.1 Build (mirrors FU323 §3's flag split)

```
CFLAGS_SAN="-std=gnu11 -Wall -Wextra -Werror -fsanitize=address,undefined \
            -fno-sanitize-recover=all -Iinclude -Isrc"
CFLAGS_PLAIN="-std=gnu11 -Wall -Wextra -Werror -Iinclude"
#-- 16 engine TUs (src/fifa96_engine/*.c minus fifa96_main.c, platform_sdl3.c)
cc $CFLAGS_SAN -c "$f" -o /tmp/opencode/fu328/obj/$(basename "$f" .c).o
#-- 63 loader TUs (src/fifa96_loader/*.c; plain: the loader carries latent UB
#--   the repo build never instruments, e.g. the negative slot-dir shift)
cc $CFLAGS_PLAIN -c "$f" -o /tmp/opencode/fu328/obj/$(basename "$f" .c).o
cc -fsanitize=address,undefined /tmp/opencode/fu328/obj/*.o -o m2_check   # + test_engine_m2.o
```

- The **unpatched** `m2_check` (unmodified `tests/test_engine_m2.c`) prints
  stdout with sha256 **`d5446144f08c22c5a4d986e1d07529068f715ff8b8e2cd86411ee493ea804ad8`**
  (295 087 B), `cmp`-identical to `tests/golden/engine/m2-frames.txt`; the
  unmodified `test_engine_m1` gives **`09b726b7…`** (22 004 B), `cmp`-identical.
  Both binaries also pass their own assert sets (rc 0).
- The measurement probe (`/tmp/opencode/fu328/sweep.c`) is linked with the same
  objects (minus the test's `main`) and `-Wl,--wrap=fifa96_camera_event_set`.
  The wrap records the arriving staging record (`entity_id`, `code`,
  `has_slot`, `vel73`, `vel75`, `dir`, `lane`, `pos`, the pre-frame pool
  `lane_z`/`cam_dz6f`/`bound`), the seed pair, the height and the ramp param,
  then calls the real setter (the engine is unmodified). Per granted frame it
  samples the 16 camera cells, the controlled/slot cells, the ball cells and
  all 22 records; it runs the same delta-run segment machine as
  `m2_claim_sample` and the same closed form as `m2_pan_curve`.
- The tape is the **verbatim `M2_KEYS` prefix** (generated from
  `tests/test_engine_m2.c:2038-2208`), zero-padded to `steps + 1` (a zero entry
  is a no-press); the family tapes are `key:from-to` ranges laid over that
  prefix at index ≥ 1820 only (the measured-safe drive boundary, FU317-L1).
- Sampling clock: `step_ns = 10 ms`, 320×240, headless, ISO present — the M2
  harness exactly. `granted` = the pace grants (3 598 in 12 000 steps).

### 4.2 The two measurement instruments, and their honesty boundary

1. **The wrap events are authoritative**: every `0x71C94` call, its seed, its
   record, the receiving camera cells. The arm classifier is mechanical:
   *arm 0* = height 0, `has_slot 0`, `seed == (int16)(vel73<<2)` (the
   `0x7F056` arm); *arm 1* = height 0, `has_slot 1`, `seed == dir<<6` (the
   `0x7F035` arm); *arm 4* = a height arm; *arm 5/6* = a mismatch class
   (defined, **never observed** in this sweep).
2. **The cone filter is recorded-only**: the per-frame census of code-4
   records satisfying `lane_x ≤ 0x40 − timer81`, `lane_x ≤ bound`, `camY ≤ 0x50`,
   folded angle ≤ 0xAB and not tracked, computed on the post-frame pool cells
   (so one frame ahead of the row-04 body's own operands). It is the
   FU316-L7 frequency instrument, **never a gate witness** (the FU317 §3.4
   warning: the algebraic filter over-approximates).

## 5. The sweep

All runs: ISO present, `M2_KEYS` verbatim prefix, 12 000 steps unless stated.
Tape indices are absolute engine steps; `f` is the 0-based granted-frame index
(the trace key); phase 2 enters at step 1802 / f 538.

### 5.1 The natural windows (verbatim, no added holds)

| shape | steps | granted | band pairs (no-slot) | no-slot row-04 events | seed / `v0` | pans | place |
|---|---|---|---|---|---|---|---|
| baseline (`M2_KEYS` verbatim) | 12 000 | 3 598 | 1 (1) | **1** | **−4 / 0** | 0 | (0,0,0) |
| longer | 24 000 | 7 198 | 1 (1) | 1 | −4 / 0 | 0 | (0,0,0) |
| longer | 48 000 | 14 398 | 1 (1) | 1 | −4 / 0 | 0 | (0,0,0) |
| longer | 96 000 | 28 798 | 1 (1) | 1 | −4 / 0 | 0 | (0,0,0) |
| longer (160 000) | 160 000 | 47 998 | 1 (1) | 1 | −4 / 0 | 0 | (0,0,0) |

The single event is the silent bind in full: **f = 1517 (step 5065)**, entity
**12**, `code 4`, `active 1` (the staging record at the event; the pool gate
tuple's `active` was 2, FU323 §4), `has_slot 0`, `dir (0,0)`, `pos (64,0)`,
`lane 64`, `lane_z −64`, `cam_dz6f 0`, `bound 66`, `timer81 0`,
`face 0xFF00`, `cam (0,0,0)`, `seed (−4,0)`, height 0, ramp 0, **arm 0**,
post-event `vel (0,0)`, `spd 0`, `pan 0`, **`T_x = T_z = 0`**, no segment.
That is the FU323-L1 measurement reproduced on a freshly built out-of-tree
binary.

### 5.2 The direction family (the phase-31/32 drives as families)

| drive (steps 1820…end) | granted | band pairs (no-slot) | events (arm) | seed / `v0` | pans | place |
|---|---|---|---|---|---|---|
| DOWN | 3 598 | 2 (0) | 2 × arm 1 | −0x40 / −5 | **2** | **(−0x120,0,0)** |
| UP | 3 598 | 1 (1) | 1 × arm 0 | −4 / 0 | 0 | (0,0,0) |
| LEFT | 3 598 | 1 (1) | 1 × arm 0 | −4 / 0 | 0 | (0,0,0) |
| RIGHT | 3 598 | 1 (1) | 1 × arm 0 | −4 / 0 | 0 | (0,0,0) |
| DOWN, 24 000 steps | 7 198 | 2 (0) | 2 × arm 1 | −0x40 / −5 | 2 (+ the half-time reset) | (−0x120,0,0) → (0,0,0) |

The DOWN pans are the phase-31 claim family: event **f = 1481** (record 2,
`has_slot 1`, `dir (−1,0)`, `lane 64`, `seed (−0x40,0)`, `vel (−5,0)`), motion
1482…1509 to **(−0x90,0,0)**; then event **f = 1553** (record 1, `has_slot 1`,
`seed (−0x40,0)`), motion 1554…1581 to **(−0x120,0,0)**. Both are `arm 1`;
**no no-slot event exists in the DOWN window** (the formation record stays at
lane 134). UP/LEFT/RIGHT are the FU317 inert family — identical to the control.

### 5.3 The offset/nudge family (the phase-32 drive)

| drive | granted | band pairs (no-slot) | events (arm) | place |
|---|---|---|---|---|
| LEFT 1820–1849 + DOWN 1850– | 3 598 | 2 (0) | 2 × arm 1 | (−0x120,0,0) |
| RIGHT 1820–1849 + DOWN 1850– | 3 598 | 2 (0) | 2 × arm 1 | (−0x120,0,0) |

The first event is **f = 1483** (record 2, `lane 64`, `cam_dz6f −16` for the
LEFT nudge and `+16` for the RIGHT nudge, `seed (−0x40,0)`), motion 1484…1511;
the second
**f = 1553** (record 1), stop 1581 at **(−0x120,0,0)**. The nudge's z-sign does
not change the x-pan (the slot dir is `(−1,0)`). Still `arm 1` only.

### 5.4 The claim/carrier contexts (kick/pass/long-ball)

| drive | granted | band pairs (no-slot) | events (arm) | seed / `v0` | pans | place |
|---|---|---|---|---|---|---|
| DOWN + KICK 5000–5004 | 3 598 | 2 (1) | 1 × arm 1, **1 × arm 0** | −0x40/−5; **−4/0** | 1 | (−0x90,0,0) |
| DOWN + PASS 5000–5004 | 3 598 | 2 (1) | 1 × arm 1, 1 × arm 0 | same | 1 | (−0x90,0,0) |
| DOWN + LONG_BALL 5000–5004 | 3 598 | 2 (1) | 1 × arm 1, 1 × arm 0 | same | 1 | (−0x90,0,0) |
| DOWN + KICK 5000 + PASS 5200 | 3 598 | 2 (1) | 1 × arm 1, 1 × arm 0 | same | 1 | (−0x90,0,0) |
| DOWN + kick/pass chain (4) | 3 598 | 2 (1) | 1 × arm 1, 1 × arm 0 | same | 1 | (−0x90,0,0) |
| DOWN 1820–4500 (release before the claim) | 3 598 | 1 (1) | 1 × arm 0 | −4 / 0 | 0 | (0,0,0) |
| UP + KICK 5000–5004 | 3 598 | 1 (1) | 1 × arm 0 | −4 / 0 | 0 | (0,0,0) |

The kick/pass family is the interesting one: the early touch **suppresses the
second slot pan** (one segment, stop −0x90) and the ball's carrier becomes the
band record (12) — which then re-enters the gate at the **moved** camera:
**f = 1590 (step 5309)**, entity 12, `has_slot 0`, `lane 63`, `lane_z −62`,
`cam_dz6f −4`, `bound 65`, `face 0xFEF6`, `pos (−82,4)`, `cam (−144,0,0)`,
`seed (−4,0)`, **arm 0**, `vel (0,0)`, `T = 0`. The no-slot arm is **live and
repeatable at a second camera place**; it is still zero-seed because `vel73 =
−1`. No height/receive arm fired in any window (`arm 4` = 0 occurrences; the
pair cells stayed `0/0`).

### 5.5 Restarts and the second half (the stall)

| shape | steps | granted | phase/clock at end | no-slot events | pans |
|---|---|---|---|---|---|
| natural 24 000 | 24 000 | 7 198 | **phase 1, clock 120** | 1 (first half) | 0 |
| natural 96 000 | 96 000 | 28 798 | phase 1, clock 120 | 1 | 0 |
| KICK 13790–13794 | 24 000 | 7 198 | phase 1, clock 120 | 1 | 0 |
| KICK 13795–13799 | 24 000 | 7 198 | phase 1, clock 120 | 1 | 0 |
| KICK 13800–13804 | 24 000 | 7 198 | phase 1, clock 120 | 1 | 0 |
| KICK 13805–13809 | 24 000 | 7 198 | phase 1, clock 120 | 1 | 0 |
| KICK 13810–13814 | 24 000 | 7 198 | phase 1, clock 120 | 1 | 0 |
| KICK 13815–13819 | 24 000 | 7 198 | phase 1, clock 120 | 1 | 0 |
| KICK 13830–13834 | 24 000 | 7 198 | phase 1, clock 120 | 1 | 0 |
| KICK 13840–13844 + DOWN 13860– | 24 000 | 7 198 | phase 1, clock 120 | 1 | 0 |

Measured transitions: phase 1 at step 9, phase 2 at step 1802, **phase 12 at
step 13802 (f 4138)**, **phase 1 at step 13805 (f 4139)**, clock 120 s; then
**no further transition** through step 96 000 (28 798 granted frames) or
160 000 (47 998). The half-time kickoff is a **stall**, not open play: the
row-01 taker waits for a release the natural tape does not supply, and neither
the seven KICK offsets (13790..13834) nor a DOWN tail resumes it (the directive
path of the M2 golden is not reproducible by input alone — the first-half
act-1 kickoff had its release at tape 1794–1799, but the same recipe does not
resume the half-time kickoff). The **second half as a natural shape is unreachable undriven**
(FU328-L4); its 24 660 post-roll granted frames contain 0 band entries and 0
events.

### 5.6 The diagnostic ladder (NOT the sweep, NOT a witness)

To separate "the geometry/arm is wrong" from "the producer is missing", the
probe was re-run with the native placement limit staged on every record at
match start (`--stage-t79`, a **record-locomotion cell**; no slot, seed or face
cell is touched). This is a causal experiment for FU328-L1, explicitly outside
the sweep table and never a natural witness (spec §7: a forced tuple becomes a
leg, not an acceptance):

| staged `+0x79` | gate f (step) | lane | `vel73` | seed | `v0` | `T` | frames | arm |
|---|---|---|---|---|---|---|---|---|
| 0 (= the port) | 1517 (5065) | 64 | −1 | −4 | 0 | 0 | 0 | 0 |
| **3** | **867 (2899)** | 64 | −3 | **−0xC** | **−1** | **−0xC** | **6** | **0** |
| **6** | **704 (2355)** | 56 | −6 | **−0x18** | **−2** | **−0x26** | **13** | **0** |
| **15** | **607 (2032)** | 40 | −13 | **−0x34** | **−4** | **−0x84** | **27** | **0** |

The measured decay signatures are the closed form's (`v0=−1` → 6 frames;
`−2` → 13; `−4` → 27, speeds `4,3,2,1` at `−0x34`). The `+0x79 = 6` row also
shows the geometry is not the issue: the same natural record, no drive, no
staging of anything but the limit (and its install-reset copy) → a real no-slot
pan. With the DOWN drive added at `+0x79 = 6`, the slot pan preempts the no-slot
one (the no-slot record never enters the cone, min lane 404) — the competition
the natural tape avoids.

## 6. The NO-GO (and the only thing that would change it)

**No nominee.** The sweep produces no drive, no gate frame and no seed that
satisfies the spec §6 tuple `(has_slot 0, |v0| ≥ 1, event, place += T(s))`. The
two no-slot gate passes that exist (f 1517, f 1590) both fail at the first
clause that matters: `v0 = 0` (the phase-32/33 NO-GO class — a "witness" with a
zero seed or a zero place delta is not a witness). The slot pans
(`v0 = −5`, `T = −0x90`) are real but `has_slot 1`; they are the phase-31/32
controls, not the target.

The failure is **not** for lack of trying shapes: the band entry happens at the
right place and angle in every shape that produces it (lane 64/63 ≤ 64,
Δ = 0, `camY = 0`); it happens **twice** under the kick/pass family; it happens
at **both** the origin and the moved camera. The missing dimension is the
record's speed, and that is upstream of the input model: **no key tape can
raise `word[+0x73]`**, because `attr` is capped by `stride_len`, which is
capped by `+0x7B & 0xF`, which is pinned to 0 by the unproduced `+0x79`. The
diagnostic ladder (§5.6) is the proof: the same input-free tape, the same
record, the same gate → a moving no-slot pan as soon as `+0x79` is supplied.

A future phase that lands the `+0x79` producer (FU328-L1) has a **predicted**
witness: the natural band record, entity 12, gate around **f ≈ 607–704** for
the native `+0x79` range 6..15 (`f = 867` at the sub-native 3), `lane ≈ 40–64`,
`|v0| = 2..5`,
`T = 0x26..0x90` (the `+0x79 = 15` diagnostic achieved `w = −13` → `0x84`), the
place moving for 13..27 granted frames. It is a prediction, not an acceptance:
the native `+0x79` for entity 12 is unmeasured (FU328-L3).

## 7. The frequency evidence (FU316-L7)

All numbers are measured on this build; "played" = the first-half granted
frames up to the half-time roll at f 4138 (4 138 in the longer windows; the
12 000-step window ends before the roll, so its 3 598 granted frames are all
first half). The phase-2 sub-span is f 538..4137 = 3 600 granted frames.

| window | granted | played | no-slot band entries | no-slot row-04 events | pans | no-slot pans | place |
|---|---|---|---|---|---|---|---|
| natural 12 000 | 3 598 | 3 598 | **1** | 1 | **0** | 0 | 0 |
| natural 24 000 | 7 198 | 4 138 + stall | 1 | 1 | 0 | 0 | 0 |
| natural 48 000 | 14 398 | 4 138 + stall | 1 | 1 | 0 | 0 | 0 |
| natural 96 000 | 28 798 | 4 138 + stall | 1 | 1 | 0 | 0 | 0 |
| natural 160 000 | 47 998 | 4 138 + stall | 1 | 1 | 0 | 0 | 0 |
| DOWN 1820– | 3 598 | 3 598 | 2 (0 no-slot) | 0 | **2** | 0 | −0x120 |
| LEFT+DOWN 1820– | 3 598 | 3 598 | 2 (0 no-slot) | 0 | 2 | 0 | −0x120 |
| DOWN+KICK 5000 | 3 598 | 3 598 | 2 (1) | 1 | 1 | 0 | −0x90 |
| DOWN+PASS 5000 | 3 598 | 3 598 | 2 (1) | 1 | 1 | 0 | −0x90 |
| DOWN+LONG_BALL 5000 | 3 598 | 3 598 | 2 (1) | 1 | 1 | 0 | −0x90 |
| UP / LEFT / RIGHT | 3 598 | 3 598 | 1 (1) | 1 | 0 | 0 | 0 |

- **The natural no-slot band-entry rate: 1 / 3 598 granted frames = 2.8 ×
  10⁻⁴ (0.028 %)** in the 12 000-step window, and **1 / 4 138 first-half
  granted frames (0.024 %)** over the longer windows. The entry is a single
  record
  (entity 12) and a single frame (the filter sees it at f 1516; the body's gate
  is f 1517).
- **The natural pan rate: 0 / 3 598 (0 %), and 0 / 28 798 over the long
  window.** The driven rate is **2 / 3 598 (0.056 %)** with the slot arm, and
  **1 / 3 598** when an early kick/pass replaces the second slot pan with the
  no-slot zero-seed gate pass.
- **The in-cone velocity distribution** (code-4, no-slot, `lane ≤ 0x40`, over
  the natural window): `|vel73| = 0` 0 record-frames, `= 1` **1** (entity 12 at
  lane 64), `≥ 2` **0**. The **all-record** histogram (every active record,
  natural window) is `[|v| 0: 44 622, 1: 27 338, ≥2: 0]` record-frames, and the
  formation-wide maximum is **1** — independently measured by FU327 §2.
- **Only 14.4 %** of the 96 000-step window's granted frames are play
  (4 138 / 28 798); the other **24 660 (85.6 %)** are the stalled half-time
  kickoff.

## 8. Numbered legs (FU328-L1…)

1. **FU328-L1 — the placement limit `+0x79` has no producer (the NO-GO's
   root).** `timer79` is never written by the engine (`fifa96_match_entities.h:129`
   is read-only; `grep` finds writers only in tests), so the row-install reset
   `timer7b = timer79` (`entities.c:94`, `0x7DA9B`; `:209`, `0x79a86`) copies
   **0**, the mover stages `stride = 0` (`match_run.c:549`), `stride_len`
   clamps to 1 (`action_handlers.c:257-259`), `attr ≤ 1`, the stride word is
   ≤ 1, the no-slot seed is ≤ 4 and `v0 = 0`. The native
   `FUN_0007997C` (`0x799F0..0x79A4D`, FU84 §6) sets
   `word[+0x79] = byte[0x10F394 + (byte[[rec+4]+0xA]>>24)] | [rec+0x9D]`, the
   table's 6..15 range (the FU84 §6 table quote: `06×8 07×8 0F 0F 0F 0E 0D …`), so
   the native cap is 6..15, not 0. **Measured blast radius:** the diagnostic
   ladder (§5.6) — with `+0x79` (and its install-reset copy `+0x7B`) staged the
   same natural record produces the full tuple, `T` up to `−0x84`. **Owner:** the entity reset/placement entry
   (`FUN_0007997C` → `fifa96_match_entities.c:94/209`) plus the `+0x9D` byte
   (OL-72). A separate, pinned, golden-moving fidelity change — never folded
   into the phase-34 acceptance.
2. **FU328-L2 — the row-0x03 placement counter is unwired and its staged
   `move_attr` is hard 0.** The counter body is present
   (`handlers.c:5813-5822`) but the dispatch row is NULL (`:6346-6347`), and
   even if wired it would compute `min((0>>22)+1, timer79)` (`:5821`) — a
   `move_attr` staging gap, not only a dispatch gap. Owner: **FU177-L1** (the
   row-03 legs). This is the *maintenance* half; L1 alone would already lift the
   cap on the install reset path, so L2 is not the blocker (measured: the port
   has `timer7b = 0` both before and after every install).
3. **FU328-L3 — the native `+0x79` value for the natural band record is
   unmeasured.** The diagnostic used 3/6/15 as a ladder, not as the native
   value; entity 12's roster/anim descriptor byte at `[[rec+4]+0xA]>>24`
   selects the table entry (6..15) and is not read here. The native natural
   moving-no-slot entry is therefore **unmeasured**, not merely unported.
   Owner: FU84 §6 + the roster descriptor; measure with L1 landed.
4. **FU328-L4 — the undriven second half is a stall, not play.** Measured:
   phase 12 at step 13802 (f 4138), phase 1 at step 13805 (f 4139), clock
   120 s, no further transition through step 160 000; seven KICK-offset attempts
   (13790..13834, plus 13840) and a DOWN tail do not resume the kickoff. The 96 000-step
   window's extra 24 660 granted frames are the stalled phase-1 kickoff. Owner:
   the row-01 kickoff release (FU-143) / the FU-84 per-frame driver; the live
   `make game` directive path is not reproducible by the null tape (the
   phase-33 Layer-2 note's live timing is a different vehicle).
5. **FU328-L5 — the drive-family coverage is finite.** Only DOWN/LEFT+DOWN put
   a record in the cone (the controlled/slot record, arm 1); UP/LEFT/RIGHT are
   inert; KICK/PASS/LONG_BALL at step 5000 suppress the second slot pan and
   produce a second no-slot gate pass at the moved camera (still zero-seed); no
   height/receive arm fires. The AI-only moving no-slot band entry is unproven
   beyond these families; the live `make game` rate is unmeasured (the null
   tape's AI is input-starved and deterministic).
6. **FU328-L6 — the cone filter is not the gate (the one-frame operand
   offset).** The census uses the post-frame pool cells, while the row-04 body
   reads the previous frame's `lane/bound/timer81`. The filter is a frequency
   instrument only; the wrap events are the gate authority. The measured
   band-pair counts are therefore ±1 frame around the body's gate frame (the
   natural pair at f 1516 corresponds to the event at f 1517).
7. **FU328-L7 — the wrap's arm classification is mechanical but partial.** Arm
   0 (no-slot seed match) and arm 1 (slot seed match) were exercised; arms
   4/5/6 (height arm, no-slot mismatch, slot mismatch) were never observed.
   A same-frame double event would be recorded (the classifier keeps the last
   event's camera cells); none occurred. The wrap is a link-time shim and the
   real setter always runs.
8. **FU328-L8 — the M2 exposure of the L1 fix.** With `+0x79` produced, every
   record's stride rises from 1 to the native 6..15 and the natural tape
   **pans undriven** (the diagnostic: place −0x26 at `+0x79 = 6`, −0x84 at 15,
   a band gate at f ≈ 700). The M2 golden will move wholesale; the Stage-2
   plan must treat L1 as a **faithful-move producer** (the first-diff rule, the
   `state=` suffix check, the stratum-set gate precedent, a written reason) —
   never as an acceptance-only change. The census `60/80` does not move (no
   dispatch row is wired by the measure).

## 9. GO / NO-GO

**NO-GO — the natural (and driven) witness.** The spec §6 tuple does not exist
on the current tree in any of the 32 measured runs: every no-slot gate pass
is a zero-seed silent bind (`v0 = 0`, place delta 0), and every real pan in
the sweep is the slot arm (the §5.6 diagnostic excluded). The spec §7 traps are exactly what the sweep found (no `lane = 0`
knife-edge witness, no zero-delta "witness"; the two no-slot passes are real
gate passes with a zero seed). The **Stage-2 consequence is the spec §5 NO-GO
branch**: P1 lands the measured distribution + this port-fidelity verdict; the
remaining legs go to the next phase; **no production change this phase**.

**GO — the diagnosis, and the fix target.** The row-04 no-slot arm, the seed
algebra, the `0xC` divisor, the 5→3→2→1 decay and the `T(s)` travel are all
live and correct (the diagnostic reproduces `T = 0xC/0x26/0x84` exactly from
the arm's own seeds). The NO-GO is one missing producer: the native placement
limit `+0x79` (`FUN_0007997C`, 6..15) with no port writer, plus the row-03
counter maintenance (FU177-L1). A future phase that lands `+0x79` gets a
**predicted** natural witness (entity 12, f ≈ 607–704 for the native 6..15
range, `|v0| = 2..5`, `T = 0x26..0x90`) — predicted, never asserted, and gated
by the golden policy.

**The spec §6 NO-GO checklist (mapped).** The no-slot instance count: **1** on
the natural tape (f 1517), **2** under the kick/pass family (f 1517 + f 1590),
**0** in the slot-only driven windows; the max `|seed|` = **4**; the max
`|v0|` = **0**; the moving-record conditions = §3.3 (counter ≥ 3, target ≥
`0x18`, face on the bearing) and the cap = §3.4; the port-fidelity verdict = the
seed staging is parity (FU326 §6) and the cap is the unproduced `+0x79`
(FU328-L1); the remaining legs = §8.

**Goldens / census.** M1 `09b726b7…` and M2 `d5446144…` are untouched (only
read); the out-of-tree probe reproduces both byte-for-byte. Census `60/80 =
75%`; no README recompute.

## 10. Known limits

- The probe is an out-of-tree **copy** of the sources; the unmodified
  `test_engine_m2`/`test_engine_m1` built from the same objects reproduce the
  goldens exactly, which is the equivalence argument for the probe build.
- The sweep is a **finite** family (32 runs, one deterministic tape); a live
  `make game` rate is unmeasured (FU328-L5).
- The cone filter is not the gate (FU328-L6); the event capture is.
- The diagnostic stages the **placement limit** (`+0x79`), not a slot/seed/face
  cell; it is a causal experiment for FU328-L1 and is excluded from the sweep
  table and from any witness claim. The spec §7 staging ban is respected in
  the sweep (no shape stages any cell); the diagnostic is the leg-shaped
  evidence the ban anticipates ("it becomes a leg and the phase lands the
  distribution").
- The native `+0x79` value for entity 12 and the native natural moving-no-slot
  entry are unmeasured (FU328-L3).
- The no-slot arm's outer guards (`|cam_x| ≤ 0x630`, `|cam_z| ≤ 0xA20`) were
  never exercised (the camera stays within ±0x120); the out-of-range `(0,0)`
  seed class is unmeasured here.
- The half-time stall is measured on the null tape; the live/directive path
  differs (FU328-L4).
- The second-half directive path, the armer/follow and the claim/carrier fusion
  are out of scope by the spec §3 (FU316-L2/L6/L8/L9, FU315-L1).

## 11. Provenance and self-audit

**Concurrent wave drafts (this wave, read after the measurements; consistent):**
FU326 §4.2 (the `|w| ≤ 15` live seed domain — which retires FU316's `s = 0x80`
row for the no-slot arm) and §6 (the seed-staging parity verdict, no
divergence); FU327 §2 (the same five gate passes, the same `|w| ≤ 1`, the same
silent bind, the same `|v0| ≥ 1` threshold). No contradiction was found; the
`+0x79` producer gap is not in either slice's scope (FU326 audits the seed
staging only, FU327 samples the measured passes), so FU328-L1 is new here.

**Fresh Ghidra this session** (`/FIFA96.EXE`, `watcom:LE:32:default`, base 0,
`.image`; standard MCP tools only; no scripts/saves):
`disassemble_bytes 0x7F030` (0x50 B: the `0x7F035` slot arm, the `0x7F056`
no-slot entry, the `|cam_x| ≤ 0x630` / `|cam_z| ≤ 0xA20` guards),
`0x7F07F` (0x40 B: the `word[+0x73]<<2` / `word[+0x75]<<2` seed stores and
`0x7F0A7`), `0x7E2BB` (0x1A B: the row-03 hold path + the idle-core entry),
`0x7E2D0` (0x50 B: `MOV EAX,[ESI+0x63]; SAR EAX,0x16; INC; MOV
word[ESI+0x7B],AX; CMP AX,DX; JBE; MOV word[ESI+0x7B],DX`). **Carried**
(named, not re-derived): FU316 §3.1 (the gate bytes `0x7ED89..0x7EFEE`), §4.3
(the `T(s)` model, the `s = 0x40 → 0x90` instance; the `s = 0x80 → 0x174`
instance is retired for the no-slot arm by FU326 §4.2/§8), FU323
§2/§4 (the measured silent bind: `trk=12` at f 1517, camera zero 1517/2081),
FU321 §3.4 (the E9 closed form), FU84 §6 (`FUN_0007997C` and the 0x10F394
table), FU177 §2/§6 (the row-03 body and the counter), FU317 §3.2/§3.5 (the
drive families and the safe append rule), the phase-33 register tail
(`docs/ENGINE.md:6733-7233`).

**Fresh source reads:** `src/fifa96_loader/fifa96_outfield.c` (610-635,
682-721), `src/fifa96_loader/fifa96_camera.c` (732-830, 855-880, 900-957,
959-991), `src/fifa96_loader/fifa96_action_handlers.c` (335-350, 240-290),
`src/fifa96_loader/fifa96_match_locomotion_tables.c` (35-55),
`src/fifa96_engine/fifa96_match_handlers.c` (2620-2800, 5813-5822, 6344-6349),
`src/fifa96_engine/fifa96_match_run.c` (500-571, 602-649, 1360-1410, 1490-1570,
4068-4110), `src/fifa96_engine/fifa96_match_entities.c` (80-100, 200-215,
735-750), `include/fifa96_engine/fifa96_match_entities.h` (60-180, 262-300),
`include/fifa96_loader/fifa96_camera.h` (7-84), `tests/test_engine_m2.c`
(1752-1782, 1896-1930, 2038-2208, 2300-2560, 3714-4013, 4219-4471, 4758-4833),
`tests/test_engine_m2.c:3935-3966` (the landed A1–A3 asserts),
`docs/superpowers/plans/2026-10-23-fifa96-m2-phase34-recon-ahead.md` (Task 3).

**Measurements (all out-of-tree, `/tmp/opencode/fu328/`):**
`m2_check` stdout sha256 `d5446144…` (295 087 B, `cmp`-identical to the
golden), `m1_check` sha256 `09b726b7…` (22 004 B, `cmp`-identical); `sweep`
runs (32 sweep runs + 5 diagnostics; logs under `/tmp/opencode/fu328/logs/`;
the stability re-runs of the control (`n12`), the claim drive (`down12`) and
the near-miss kick drive (`kick5`) are byte-identical after label
normalisation); `stride` (the attr table probe: attr *k* → max
|x|,|z| = *k*); `sweep.c` + `m2_keys.inc` (the verbatim tape). The ISO was
read only; no repo file was written except this draft; no git; `build/`,
`tests/golden/` and `fifa96.rep/` untouched.

**Self-audit.** (a) No address/byte claim without a fresh read or a named
carried doc. (b) The measured cap is stated as measured, not as a native fact
(the native mismatch is L3). (c) The diagnostic is labelled at every
appearance and is never counted as a witness. (d) The frequency numbers are
per-window, with the stall separated from play. (e) The verdict is the spec's
NO-GO branch; no number in the doc is an asserted native behaviour. (f) The
census and goldens are untouched.

## Review erratum (wave review, 2026-10-23)

**1. The band record's `active` (§5.1).** Quote: "`active 1` (the staging record at the event; the pool gate tuple's `active` was 2, FU323 §4)". **Corrected:** the pool gate tuple's `active` is **1**, not 2. FU323's own artifact `/tmp/opencode/fu323/anim_patched.txt` step 5062, team 1 record 1 (entity 12) is `5062 1 1 4 1 0 2 -256 6 1 64 0 0 0 64 -64 0 66 0 0 0 1 -1` — `active 1`, `anim 0`, `row43 2`; the `2` in FU323 §4's row is the `row43`/`+0x43` cell, not `active`. FU327's dispatch view is also `active 1`, and this doc's own `EV` line says `active=1`. The rest of §5.1 (f 1517, `has_slot 0`, `dir (0,0)`, seed −4, v0 0) stands; FU327-L2 records the correction.

**2. The native `+0x79` range (§8 L1).** Quote: "the table's 6..15 range ... so the native cap is 6..15, not 0." **Corrected:** 6..15 is the `0x10F394` **table entry** range; the native store also ORs `[rec+0x9D]` (fresh `0x79A45 MOV DL,[EBP+0x9D]`, `0x79A4B OR BL,DL`, `0x79A4D MOV [EBP+0x79],BX`), and that byte's producer is unported (OL-72). The native `+0x79` for entity 12 is unmeasured (FU328-L3); the 3/6/15 diagnostic is a ladder, not the native value. The measured blast radius and the L1 verdict stand, but "6..15" is a table range, not a proven cap.

**3. The `ramp` cell (§5.1).** Quote: "height 0, ramp 0". **Corrected:** `ramp 0` is the setter's `ramp_param` (the wrap's `rp=0`); the post-event `ramp_divisor` is **12** (`tim 0/12`), as FU327 §4.2, FU323 §2 and the `FU327POST` line report. Name the cell, so the two figures do not read as a contradiction.

**4. The direction family (§8 L5).** Quote: "Only DOWN/LEFT+DOWN put a record in the cone". **Corrected:** the **RIGHT** nudge + DOWN (`/tmp/opencode/fu328/logs/rnudge12.txt`) also puts the slot record in the cone: two arm-1 events at f 1483/1553, `place (−0x120,0,0)`, exactly as §5.3's own RIGHT row records. The family is DOWN, LEFT+DOWN, RIGHT+DOWN; the rest of L5 stands.
