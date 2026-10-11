# FU-154 — live camera place/pan and the phase-2 ball on canvas (L14.8.e)

Recon wave R2 of `docs/superpowers/plans/2026-10-11-fifa96-m2-phase12-live-camera-visible-ball.md`.
Read-only Ghidra session, authoritative program `/FIFA96.EXE` (Ghidra project
path `/FIFA96.EXE`, case-sensitive; LE-image globals at `+0x100000`; scripts
disabled). Addresses are Ghidra addresses on `/FIFA96.EXE` unless quoted from a
carried doc's flat form. Every byte claim below was re-read first-hand this
session (`disassemble_bytes`, `decompile_function`, `read_memory`,
`search_instructions`, fresh `get_xrefs_to`). Engine-side numbers were measured
with scratch probes linked against the built `fifa96_engine` static libraries
(probe sources in `/tmp/opencode/r2probe*.c`, run from the repo root; outputs
quoted in §4). No repo file was modified by this slice.

Question (plan R2): what moves the camera **place/pan** from live play so the
ball's camera-derived phase-2 lead lands on canvas — closing phase-11 leg
**L14.8.e**?

Result in one line: **the phase-2 lead is a pure function of the FU-71 camera
triple (first-hand bytes `0x8BB7C..0x8BBF6`), the render eye is that triple
(staged by `FUN_00036C70`), and the live *orientation* — which the engine
stands in as `yaw = pitch = 0` — is the native view object 0's static
`+0x58 = 0xBF9C` / `+0x5C = 200`; seeding those two first-hand values makes
the ball visibly on canvas through the live kick roll (measured), while the
*camera motion* in live play only comes from the FU-71 pan integrator
(`FUN_000736AC`) whose event producer gates are the 15.a/T1 root cause.**

---

## 1. The phase-2 ball lead — exact cells and the on-canvas condition

### 1.1 First-hand bytes (`disassemble_bytes 0x8BB55`, 170 B)

```
0x8BB7C  MOV EBX, dword ptr [0x00157754]     ; 8b1d54771500
0x8BB82  MOV EAX, [0x00157754]               ; a154771500
0x8BB87  SAR EBX, 0x2
0x8BB8A  SUB EAX, EBX                       ; EAX = three_q = camZ - (camZ>>2)
0x8BB8C  MOV ECX, EAX
0x8BB8E  NEG ECX
0x8BB90  MOV ESI, dword ptr [0x0015774c]     ; camera X
0x8BB96  SAR ECX, 0x1                       ; ECX = (-three_q) >> 1   (tx base)
0x8BB9A  MOV EDI, ECX
0x8BB9C  MOV EBX, EAX                       ; EBX = three_q           (tz base)
0x8BB9E  SUB EDI, ESI                       ; EDI = tx - camX
0x8BBA0  MOV ESI, EAX
0x8BBA2  MOV EAX, [0x00157754]
0x8BBA7  MOV [0x00158834], EDX              ; target y = 0 (EDX=0 at 0x8BB98)
0x8BBAD  SUB ESI, EAX                       ; ESI = three_q - camZ
0x8BBAF  MOVSX EAX, DI
0x8BBB2  MOVSX EDX, SI
0x8BBB5  CALL 0x0008dc68                    ; the octagonal metric (16-bit args)
0x8BBBA  CWDE
0x8BBBB  CMP EAX, 0x120
0x8BBC0  JGE 0x8BBF0
0x8BBC2  TEST EDI, EDI                      ; 32-bit sign of dx
0x8BBC4  JL 0x8BBCE / 0x8BBC6 ADD ECX,0x120 / else SUB ECX,0x120
0x8BBD4  TEST ESI, ESI                      ; 32-bit sign of dz
0x8BBD6  JL 0x8BBE0 / 0x8BBD8 ADD EBX,0x120 / else SUB EBX,0x120
0x8BBE6  MOV dword ptr [0x0015884e], 0x7     ; max speed on the offset arm
0x8BBF0  MOV dword ptr [0x00158830], ECX     ; target x
0x8BBF6  MOV dword ptr [0x00158838], EBX     ; target z
```

Confirmed: the lead reads the **aligned dwords** `[0x157754]` (camera Z) and
`[0x15774C]` (camera X) — no `dword>>16` alias. The metric is called on the
**sign-extended low words** (`MOVSX EAX,DI` / `MOVSX EDX,SI`, both 16-bit),
while the ±0x120 push tests the **full 32-bit** `EDI`/`ESI`. The engine port
(`match_run_ball_machine`, `src/fifa96_engine/fifa96_match_run.c:2689`) matches
this shape (`fifa96_entity_distance` on the sign-extended 16-bit components at
`2708`), and the metric target is `0x8DC68` (the doc's §16.2 "FUN_0008DCD4" is
the shared slot port; a minor erratum candidate, §6).

The camera triple's field order is pinned by the event setter's reset push at
`0x71CA6..0x71CBB`: `PUSH 0` (flag), `[0x157754]` (z), `[0x157750]` (y),
`[0x15774C]` (x) into `FUN_000700F4(x, y, z, flag)` — so `0x15774C = x`,
`0x157750 = y`, `0x157754 = z`, as carried.

### 1.2 What values put the lead on canvas

Engine projection (`fifa96_projection_matrix` + `…_project`,
`src/fifa96_loader/fifa96_projection.c:112/181`): canvas 320 wide, centre 160,
`x' = 160 + 320·rot.x/rot.z`, near gate `rot.z >= 5`. With
`x_rel = tx − camX`, `z_rel = tz − camZ`:

```
yaw 0:            rot.x = x_rel, rot.z = z_rel
                  on-canvas  ⇔  |x_rel / z_rel| ≤ 0.5 and z_rel ≥ 5
native yaw 0xBF9C: cos ≈ −0.0098, sin ≈ −0.99995
                  rot.x ≈ −0.0098·x_rel − z_rel,  rot.z ≈ x_rel − 0.0098·z_rel
```

For the settled lead (metric ≥ 0x120 branch): `x_rel = −3·camZ/8 − camX`,
`z_rel = −camZ/4`, so with yaw 0 the ratio `x_rel/z_rel = 1.5 + 4·camX/camZ`
and on-canvas requires **`camX/camZ ∈ [−0.5, −0.25]` with `camZ < 0`** — a
diagonal place. The engine stand-in `(0,0,0)` hits the small-metric push arm
instead: base `(0,0)` → `dx = 0 → tx = +0x120`, `dz = 0 → tz = +0x120`, so the
lead is `(0x120, 0x120)` and `x' = 160 + 320·288/288 = 480` — the L14.8.e
number.

Probe sweep (`r2probe4`, the engine's own projection, native lead formula):

| yaw | camera (camX,camZ) | lead (tx,tz) | x' |
|---|---|---|---|
| 0 | (0x1E0, −0x588) | (0x213, −0x426) | 206 |
| 0 | (0x420, −0xB10) | (0x426, −0x84C) | 162 |
| 49052 (0xBF9C) | (−0x720, −0xB10) | (0x426, −0x84C) | 25 |
| 49052 | (−0x720, 0) | (0, 0) | 156 |
| 49052 | (−0x720, +0x588) | (−0x10A, 0x213) | 296 |

With the native yaw, the lead is on canvas for **every** `camX ∈ [−0x720,
−0x2A0]` tested across the whole legal `camZ` span — the near-axial side view
keeps the lead framed over a wide camera range. With yaw 0 only the narrow
diagonal band works. The goal-mouth park (`camZ ≈ 0xB28`) puts the ball behind
the near plane in **both** views (`z_rel = −0x2CA` yaw 0; `x_rel = −0x42F`
native), matching the phase-11 smoke note.

---

## 2. The render camera feed — native vs engine

### 2.1 Eye = the FU-71 triple (staged), first-hand

`FUN_00036C70` (the per-frame scene composer, called at `0x49FF5`/`0x49528`):

```
0x36C76  CMP dword[0x157A6F], 0             ; 833d6f7a150000
0x36C7F  CALL 0x63fd0; TEST/JNZ            ; replay idle gate
0x36C88  MOV ESI, [0x157A6F]               ; the override triple pointer
0x36C95  MOV EDI, 0x155AA4; MOV ESI, 0x15774c
0x36C9F  MOVSD x3                          ; [0x155AA4..] = the triple
0x36CA8  |X|>0x1770 || |Z|>0x1770 -> -0xFA0/Y 0
```

`[0x9A70]` (image = `0x155AA4`, `read_memory 0x109A70`) is the “camera vector
pointer” the renderer subtracts in `FUN_00056E50` (FU-85 §4). So the live eye
is the FU-71 triple — the engine's model is faithful here.

The `[0x157A6F]` override is a **camera triple**, first-hand: `0x83471 MOV
dword[0x157A6F], 0x158767`; `0x83629..0x8363D MOV EDI,0x158767;
MOV ESI,0x15774c; MOVSD x3; MOV [0x157A6F],0` — the gate-fail arm **copies the
FU-71 triple into `0x158767`** and clears the override. All other writers
(`0x73EC7`, `0x84156`, `0x854F4`, `0x855A3`, `0x860BF`) are clears. FU-112 §3's
“ball pointer” naming for `[0x57A6F]` is an erratum candidate (§6): the
override is seeded from the camera triple, not from the ball block
(`0x158767 < 0x1587EC`).

### 2.2 Orientation = the view object `+0x58/+0x5C` (first-hand)

```
0x49892  CALL 0x4d2d4                    ; the view driver (pose feed etc.)
0x49897  CALL 0x4c904
0x4989C  MOV [0x107314], EAX             ; the draw's obj
0x4962A  MOV EDX, [0x107314]; 0x49630 MOV EAX,[0x107318]; CALL 0x58bc0
0x58BCC  CALL 0x58a44                    ; (EDX = obj)
0x58A4D  LEA EAX, [EDX + 0xc]; MOV EDX, 0x154324; CALL 0x4c4b4   ; matrix
```

`FUN_0004C904` copies `[0x7DC8]+0x10/0x14/0x18 → 0x14E4E0`, `+0x58/+0x5C →
0x14E4EC/F0`, `+0x4C → 0x14E4F4`, and returns `EAX = 0x14E4E0`. So
`obj+0xC = 0x14E4EC` = the view object's `+0x58` (yaw) and `obj+0x10` =
`+0x5C` (pitch) feed `FUN_0004C4B4` — the projection matrix. The engine's
`r->yaw`/`r->pitch` model exactly these two cells.

The fresh-match view object is camera 0: image `[0x107DC8] = 0x00107508`
(`read_memory 0x107DC8`), and its static matrix fields are
`+0x58 = 0xBF9C`, `+0x5C = 200` (`read_memory 0x107560` = `9c bf 00 00 c8 00
00 00`), `+0x4C` dword = `0x1500` (`read_memory 0x107554` = `00 15 00 00`;
the “21” is the `+0x4D` byte — FU-96 §4 errata re-verified).

Engine-side confirmation that camera 0 is the fresh-match view:
`fifa96_match_run.h:172` `MATCH_RUN_VIEW_RATIO 0x1500` equals the same
object's `+0x4C` dword. (When a pose mode *does* apply, the pose feed writes
the *preset record's* triple — e.g. mode 0x10 parks the camera at
`(-83, 320, 3631)`, `test_view_pose_feed_arms_camera` — so the preset
positions are not the live open-play eye; mode 0 writes no pose.)

### 2.3 What the engine stands in as

`fifa96_match_run_reset_render` (`fifa96_match_run.c:1185-1194`) zeroes the
render struct (`memset`) and calls `fifa96_camera_init(&r->camera, 0, 0, 0)`
— so `yaw = pitch = 0`. The pose feed (line `2875`) is a no-op for
`view_mode 0` (the default arm, `fifa96_camera.c:349-353`), so the yaw stays 0
for the whole live smoke. The native seed is `0xBF9C/200` — the engine's
stand-in is not the native default view (§5).

---

## 3. What moves the camera in live play

### 3.1 The place (position reset) call sites — fresh census

`search_instructions CALL 0x000700f4` = **29 sites**:

| family | sites | triple source |
|---|---|---|
| act-1 kickoff body | `0x88E6A`, `0x88FA0` | static `[0x10F328/2C/30]` = (0,0,0) (9 referencing sites, all reads — no static writer) |
| event setter `FUN_00071C94` | `0x71CC3` | the current triple (preserve) |
| `FUN_0006FFC0` (camera block reset) | `0x7000F` | current triple |
| phase state machine `FUN_0008D098` | `0x8D301`, `0x8D3CA`, `0x8D511` | `0x8D301` pushes `[0x15777C/780/784]` (the snapshot triple the engine models as `goal_snap_*`); `0x8D3CA`/`0x8D511` push a triple from the copied record block (`REP MOVSD` from `0x157740`) |
| sequence/phase bodies | `0x7405E`, `0x7426D`, `0x74573`, `0x74F99`, `0x750B5`, `0x755C7`, `0x75976`, `0x76DBA`, `0x77542`, `0x77D82`, `0x7DC01`, `0x83EC2`, `0x84B6B`, `0x84F84`, `0x856CB`, `0x8596B`, `0x85E98`, `0x87C49`, `0x8889F`, `0x8B737`, `0x92EBF` | set-piece/cinematic camera lines (0x74xxx–0x85xxx band; e.g. `0x83E8D MOV [0x157754],0x9F0` / `0x83EA1 … -0x9F0`) |

For **live open play** the kickoff place is `(0,0,0)` (the static reset triple);
the restart arms are goal/throw/corner placements; no site places the camera
per-frame from the ball.

### 3.2 The pan integrator and its velocity producers

`FUN_000736AC` (fresh decompile; the engine `fifa96_camera_update_walk`):

* gate `(short)[0x1577BE] != 0 || [0x1577EE].hi != 0`; the timer
  `[0x1577F8].hi += delta`; with a nonzero event height `pos_y = FUN_00070B94`;
* pan step `if ((short)[0x1577F4] < (short)[0x1577F8].hi || (…)) FUN_000709D0()`
  — before the integration;
* integration gated on the bearing: `acc_x += vel_x`, `acc_z += vel_z` per
  delta frame, then `[0x15774C] += acc_x`, `[0x157754] += acc_z` (the y is not
  integrated);
* the every-frame follow call at `0x73B5B..0x73B6B` (`FUN_00071DF4`);
* boundary `FUN_0007131C` when `|x| > 0x6C0 || |z| > 0xAB0`.

Velocity producers (carried + re-checked): `FUN_00070544` (event; fast path
`seed/F8`, idle arm F2=6/F4=0xC/F8=0xC — the *fast* pan; height arm
`F8 = F4−F6 ≈ 2·ramp(h)` — the *slow* pan), `FUN_000709D0` (pan step/decay),
`FUN_00071DF4` (follow: tracked record slot dirs ×15, height > 0xF0),
`FUN_00070DE0`/`FUN_0007131C` (reposition/boundary).

**The wired live producers** (phase-9/10/11): row-04 (`fifa96_match_action_04`
→ `fifa96_camera_event_set`, `fifa96_match_handlers.c:2427-2433`), the ball
staging receive arm (`fifa96_match_run_ball_receive`, `fifa96_match_run.c:2551`),
row-1E stage-5 release (`fifa96_match_handlers.c:524`), the follow
(`fifa96_match_run_camera_follow`, `fifa96_match_run.c:2492`).

**Why the engine camera stays static in the smoke**: no live event fires in
the smoke window (probe §4: `event_param = 0`, `vel = (0,0)`, camera
`(0,0,0)` for the whole open-play run) — the same root cause as 15.a:
row-04's `0x71C94` arms sit behind the `lane`/`bound` half-line gates
(`fifa96_outfield.c:607-717`; the m41 tape's lane stays ~1460), and the
receive-arm event pans slowly (height divisor; measured 8/12 units — §4) while
the follow's frame-path gate (`0x739DC`, `event_param > 0x10` + `follow_speed`
at `0x739CE`) refuses at the moments the smoke reaches.

### 3.3 The unported live orientation

The mode-0 default arm of `FUN_000505D0` (fresh decompile) selects the camera
object and calls `(*(&0x108B80)[iVar6])([0x7DC8], puVar7)` **without writing a
pose**. The handler table entries `0x4E834`/`0x4E3A8`/`0x4ECA8`/`0x4DB3C`
(the live view bodies, FU-152 §2.8) evolve the view object's yaw/pitch/roll —
this is the producer that keeps the view following the play after the static
seed. It is unported (FU-152 leg 9 / FU-148 leg 8) and becomes R2-L1 below.

---

## 4. Measured pathology (scratch probes against the built engine libs)

Probe #2 — the natural kickoff → KICK → phase-2 chain (`game/FIFAPCCD96.iso`),
projecting the live ball record with the render's own primitives:

```
engine stand-in (yaw 0, camera (0,0,0)):
  ball never on canvas: 0/90 frames (settled lead x' = 480)
native view seed (yaw 0xBF9C, pitch 200, camera (0,0,0)):
  kickoff/roll window: ball x' = 152,148,…,4  (ON) frames 0..15, exits left f16
  16/17 frames on canvas; the settled lead itself projects x' ≈ −167
```

Probe #3 — the live long-ball receive/follow chain (T1 choreography, no
fixture velocity pokes): after the receive event (`event_param = 0x100`) the
camera moved only `(0,0,0) → (8,131,12)` (the height-arm divisor ≈118; the
pan decay), the frame-path follow left `vel = (0,0)`, and the ball crossed
behind the camera plane in both yaws — the pan magnitude/gates are the
T1/R1 coupling, not the ball machine.

Probe #1 — the same live chain with the stand-in: the camera never leaves
`(0,0,0)`, `vel (0,0)`, `event_param 0`; the ball chases the fixed
`(0x120,0x120)` lead (x' 15328→382) — never on canvas.

So: the ball-visibility blocker has **two independent terms** — (a) the
orientation stand-in (`yaw = pitch = 0`), whose first-hand native seed alone
opens a measured on-canvas window, and (b) the live pan/place (no event fires;
the camera is static, so the ball settles at the lead and slides off-screen).
(a) is bounded and landable now; (b) is the T1/R1 event-reachability coupling.

---

## 5. T2 wiring recommendation

### Step 1 (land this first — bounded, no fake)

**Seed the fresh-match render view from the native camera object 0's static
matrix fields** in `fifa96_match_run_reset_render`
(`src/fifa96_engine/fifa96_match_run.c:1185`, next to the existing
`r->view_ratio = MATCH_RUN_VIEW_RATIO` whose value `0x1500` is the same
object's `+0x4C`):

```c
/* The native fresh-match view object is camera 0 (image [0x107DC8] =
 * 0x107508); its static matrix fields are +0x58 yaw = 0xBF9C, +0x5C
 * pitch = 200 (read_memory 0x107560) — the live pose feed's mode-0
 * default arm writes no pose, so these are the live seed. */
#define MATCH_RUN_VIEW_YAW   0xBF9C
#define MATCH_RUN_VIEW_PITCH 200
...
r->yaw = MATCH_RUN_VIEW_YAW;
r->pitch = MATCH_RUN_VIEW_PITCH;
```

This is first-hand native state, not a camera fake: the projection matrix
already consumes `r->yaw/r->pitch` (`fifa96_match_run.c:3636`), and the pose
feed may overwrite them whenever a real mode applies.

Tests:
* `tests/test_engine_match_render.c:139` (`test_init_resets_render_state`)
  updates the fresh pins `yaw == 0xBF9C`, `pitch == 200`. The
  `render_fixture` family already re-stages `yaw = pitch = 0` (`setup_render`,
  line 89); the **`scene_fixture` family does not** (`scene_fixture_init`,
  line 379 — it inherits the reset and its camera-relative pixel assertions,
  e.g. `test_ball_machine_moves_ball_slot_pixels` line 581, will shift):
  re-stage `yaw = pitch = 0` there (preserving the fixtures' yaw-0 pins), or
  update the affected pin values.
* Extend `test_live_ball_record_produced_in_open_play`
  (`tests/test_engine_match_frame.c:1611`) with the render on: project the
  staged slot 22 with `fifa96_projection_*` and assert **on canvas** for the
  first frames after the phase-2 flip (RED with yaw 0: x′ = 480; GREEN with
  the seed: x′ ≈ 152…). A projection-level assert is exact and avoids a
  pixel-position regression.
* The v12 acceptance (T4) must re-pin the M2 negatives: `tests/test_engine_m2.c:1811-1812`
  asserts `ball_link_off_canvas == 0` in the m58 window; with the seed the
  clean depth gate no longer rejects the ball (it is laterally off, but the
  flag only tests the near gate), so the v12 strata should assert drawable
  *and* screen-bounds, and the ISO-mode golden hash re-pins (the render is
  enabled on the tape, `test_engine_m2.c:1966`) — an intended-upgrade re-pin
  with reason + frame diff per the plan.
* Smoke `p12-t2-*`: the phase-11 smoke's KICK-at-kickoff chain reaches phase 2;
  capture within the roll window (≈16 granted frames ≈ 0.5 s after the flip)
  and the ball is on screen. Without Step 2 the ball exits left once the chase
  settles — record that honestly.

### Step 2 (the camera actually moving — coupled to T1/R1)

The live pan needs an event that the *frame path* produces: the fast pans are
the **row-04 idle-height events** (divisor 0xC; seeds = the type table/slot
dirs, `fifa96_outfield.c:667-704`); their reachability is the `lane`/`bound`
half-line gate (R1/R4 carry this). The follow arm's rate path
(`0x739CE..0x73B6B` gates) is T1's. Until one of those lands, the camera
place/pan stays static in the smoke and only Step 1's orientation holds the
ball.

### Named next-hop producer (leg)

**R2-L1 — the live view-object evolution.** `FUN_0004E834` (handler 0, the
mode-0 default arm's `(&0x108B80)[0]`) plus the behavior blocks `0x10896C/…`
is what evolves the view yaw/pitch/roll per frame and keeps the play framed
after the static seed. Porting it (or the equivalent yaw tracking) is the
honest closure of “the camera follows the play”; it also decides whether the
settled lead returns on canvas without a pan.

---

## 6. Errata candidates (quoted, first-hand)

1. **FU-148 §16.2 metric call target.** The phase-2 metric call at `0x8BBB5`
   is `CALL 0x0008dc68` (`e8ae200000`), the distance primitive the engine
   ports as `fifa96_entity_distance`; the doc names `FUN_0008DCD4` (the
   packed-out port) for this call site.
2. **FU-112 §3 “ball pointer `[0x57A6F]`”.** First-hand, the composer copies
   `[0x157A6F]` (else `0x15774C`) into the staged triple `0x155AA4`, and the
   only non-clear writer seeds the override from `0x15774C` into `0x158767`
   (`0x83471`/`0x83629`); the renderer consumes `[0x9A70]` (= `0x155AA4`) as
   the camera-relative basis. The override is a camera triple, not the ball
   block (`0x158767 < 0x1587EC`). FU-120 §51 “ball pointer” carries the same
   naming.
3. **FU-96 §4 errata re-verified** (not an erratum, a re-check): `0x107554`
   reads `00 15 00 00` → dword `0x1500`, byte `+0x4D = 0x15`; both sibling
   objects have it.

## 7. Numbered legs

* **R2-L1 the live view-object handler** (`FUN_0004E834` + behavior blocks):
  the per-frame yaw/pitch/roll evolution after the static seed; the next-hop
  producer for a *continuously* framed ball. (Same ground as FU-152 leg 9.)
* **R2-L2 the `[0x157A6F] → 0x158767` animation camera override** (the
  sequence-body camera copy, `FUN_00083428`'s gate `byte[0x15877E] >= 0 &&
  dword[0x15877B]>>24 < 3`): what maintains `0x158767` when the override is
  set, and which animation mode uses it. Not live open play.
* **R2-L3 the `[0x10F328/2C/30]` reset triple writers.** All 9 referencing
  sites are reads; the static cells are `(0,0,0)`. A computed-pointer/memcpy
  writer cannot be excluded statically (leg; ties into 15.a).
* **R2-L4 the live pan reachability** (row-04 `0x71C94` arms' `lane`/`bound`
  gates + the follow's frame-path gates): measured inactive in the smoke
  (probes #1/#3). R1/R4 territory; T2's Step 2 consumes their result.
* **R2-L5 the settled-lead screen position** with the native seed is x′ ≈ −167
  (off-left) at the stand-in camera; whether the live handler/pan returns it
  on canvas is part of R2-L1.

## 8. Provenance

Ghidra MCP (read-only `/FIFA96.EXE`, scripts disabled): `disassemble_bytes`
0x8BB55 (170 B), 0x36C70 (80 B), 0x83450/0x83620/0x84130/0x854D0/0x85590/
0x860B0, 0x4C904, 0x58A44, 0x58BA0, 0x49600, 0x493C0, 0x49880, 0x4D2D4,
0x71C94, 0x88E30, 0x88F40, 0x8D2C0/0x8D3A0/0x8D4E0, 0x53810, 0x51420,
0x51980, 0x36544, 0x365B0, 0x4C324; `decompile_function` 0x736AC, 0x505D0;
`read_memory` 0x10F328, 0x107508 (112 B), 0x107554, 0x107560, 0x1075C4,
0x107DC8, 0x109A70, 0x158740, 0x108E1C, 0x157A60, 0x105FA0;
`search_instructions` `[0x00157754]` (110), `[0x0015774c]` (76),
`[0x00107dc8]` (100), `CALL 0x000700f4` (29), `[0x0010f328]`/`[0x0010f32c]`
(9 each); fresh `get_xrefs_to` 0x157A6F (9), 0x14E57C (12), 0x107314 (8),
0x58A44 (3), 0x58BC0 (1), 0x108E1C (4).

Engine probes: `/tmp/opencode/r2probe.c` (live chain + sweep),
`r2probe2.c` (yaw 0 vs native seed), `r2probe3.c` (long-ball receive/follow),
`r2probe4.c` (lead on-canvas sweep) — linked against `build/lib*.a`, run from
the repo root with `game/FIFAPCCD96.iso`. Scratch only; not committed.

Carried docs: FU-96 §4/§7, FU-112 §1/§3, FU-148 §12/§13/§15/§16, FU-152 §2.8/
§2.9, FU-85 §4, plan R2/§Global Constraints.
