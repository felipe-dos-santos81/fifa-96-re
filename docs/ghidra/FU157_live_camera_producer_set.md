# FU-157 — the live camera producer set (the minimum honest set for live camera motion)

Recon wave R5 of `docs/superpowers/plans/2026-10-11-fifa96-m2-phase13-live-camera-ball-art.md`. Read-only Ghidra session, authoritative program
**`/FIFA96.EXE`** (case-sensitive; scripts disabled; LE-image globals at
`+0x100000`; every address below is a Ghidra/flat LE address). All byte claims
are fresh `disassemble_bytes` / `decompile_function` / `read_memory` /
`search_instructions` of this session; engine-side numbers were measured with
scratch probes linked against the built (sanitized) `fifa96_engine` static
libraries, run from the repo root with `game/FIFAPCCD96.iso`
(`/tmp/opencode/r5probe{,2,3,4}.c`; outputs quoted in §3.4/§4.4). No repo file
was modified by this slice.

Question (plan R5): **what is the minimum honest producer set that makes the
camera move live end-to-end in open play?** Three components: R2-L1 (the live
view-object handler `FUN_0004E834`), R2-L4 (the `FUN_000736AC` pan integrator's
live inputs and the six unreachable `FUN_00071C94` arms), T1-L3 (the bootstrap
that raises `[0x157750]` and the place off zero/the stand-in).

Result in one line: **the camera *place* already moves live** — the walk-claim
frame fires the row-04 half-line event and the camera pans `0 → 0x90`
(first-hand probe, §3.4) — **but nothing live ever raises the event height past
`0xF0`, so the every-frame follow never wakes**; the minimum honest set is one
restored native Y producer (the **throw-in stage-3 cut's `[0x157750] = 0x80`**,
dropped by the port at `fifa96_match_handlers.c:1467`, §5) plus the already-wired
receive→tracked→follow chain; **R2-L1 writes no camera triple** (orientation
only, §2) and **no `FUN_00071C94` arm is a precondition** (§4).

---

## 1. The cell model, re-verified first-hand

The FU-71 camera triple is `0x15774C = x`, `0x157750 = y` (the shared
camera-Y/ball-height cell), `0x157754 = z` (carried; the event setter's reset
push order re-confirmed in §5). `FUN_000700F4` (`decompile_function 0x700F4`)
is the place/reset:

* it stores the three incoming stack words at `0x70014C..0x70150`
  (`MOVSD x3` from `[ESP+0x1C]`, i.e. the three pushed args x,y,z) into
  `0x15774C/50/54`;
* it zeroes the whole event state: `0x1577F0` (`0x70159`), `0x1577F2`
  (`0x70160`), `0x1577F4` (`0x70167`), `0x1577F8`, `0x1577BA/BC/BE`
  (`0x701D8..0x70204`), `0x1577C0/C2` (`0x701E5/0x701EC`), `0x1577C4/C6/C8`,
  `0x1577CA` (tracked), `0x157821` (pan counter, `0x70153`);
* it copies the triple into the four anchor triples `0x157764/157770/157788/
  157794` (`0x701AE..0x701D7`);
* the **4th argument is the pause flag**, read first-hand at
  `0x7021B MOV ECX,dword[ESP+0x28]` → `DAT_00157822 = 0; if (flag != 0)
  DAT_00157822 = 1; [0x157820] = DAT_00157822` (decompile).

The kickoff reset passes the static triple `[0x10F328/2C/30]` = `(0,0,0)` with
flag 0 (`0x88E4B..0x88E6A`, first-hand: `PUSH 0`, `[0x10F330]`, `[0x10F32C]`,
`[0x10F328]`, `CALL 0x700F4`) — the `(0,0,0)` stand-in is native state, as
carried by FU-96 §7.

`FUN_000736AC` (the pan integrator; fresh `disassemble_bytes 0x736AC..0x738D0`,
130 insns) runs this order:

```
0x736B2  EAX = (int16)word[0x1577BE] (the bearing, via dword[0x1577BC]>>16)
0x736BA  if EAX <= 0xC -> skip the velocity jitter
0x736C3  if dword[0x157750] <= 0xA0 -> skip the jitter
0x736CF..0x73723  the RNG jitter: at >= 0x19 a plain walk, else the
         (0x21 - speed) modulo gate; it +/-1-jitters word[0x1577C0] and
         word[0x1577C2] (the velocity words the integration loop adds)
0x73723  CALL 0x92864 (S4 note), save speed/vel to 0x157825/27/29
0x73756  CALL 0x72AC4; MOVSD x3: 0x157758 <- 0x15774C (the previous triple)
0x7375E  if byte[0x157820] != 0 -> 0x73CBE (the walk is PAUSED)
0x7376B  if word[0x1577BE] == 0 && word[0x1577F0] == 0 -> 0x73B70 (skip)
0x73783  timer [0x1577FA] += frame delta [0x157A64]
0x737A1  if word[0x1577F0] != 0: pos_y = FUN_00070B94(timer)  (0x737B4)
0x737B9  if timer > timer_limit -> pan step
0x737C8  else if dword[0x15775C] > 0 && dword[0x157750] <= 0 -> pan step
0x737DF  if word[0x1577BE] == 0 -> 0x73B70
0x737ED  zero 0x1577C6/C8; delta loop: acc += word[0x1577C0/C2] per frame
0x73845  step = metric(acc); pos_x += acc_x (0x7386B), pos_z += acc_z (0x73883)
0x73889  anchor 0x157788/0x157800 when timer > [0x157800]; 0x738A1 the second
         anchor 0x157794/0x157806; then the boundary/follow section (0x73B70..)
```

**Two engine gaps this exposes** (numbered legs, §7): the head RNG jitter
(`0x736B2..0x73723`) is unmodeled, and the second pan-step admission
(`0x737C8`: previous Y > 0 && current Y <= 0 — the descent-to-ground step) is
not in `fifa96_camera_update_walk` (which only tests
`timer > timer_limit`, `src/fifa96_loader/fifa96_camera.c:95`). The
`byte[0x157820]` pause read (`0x7375E`) is modelled as `cam->paused`
(`camera.c:77`), but **no live writer sets `cam->paused`** — the native cell's
live writers store the engine's `mr->flag_157820`
(`fifa96_match_handlers.c:509`), which only the record-event gate reads
(`fifa96_match_run.c:587`). A faithful port must bridge them (leg R5-L5).

---

## 2. R2-L1 — `FUN_0004E834`, the live view-object handler

### 2.1 Call sites and gates (fresh first-hand)

Fresh `get_xrefs_to 0x4E834` = 3: `0x4D940` (`FUN_0004D908`), `0x51056`
(computed, inside `FUN_000505D0`), and the handler-table DATA ref `0x108B80`.
Fresh `read_memory 0x108B80` (48 B) = `{0x4E834, 0x4E3A8, 0x4ECA8, 0x4DB3C}`
then the constants `{160, 1, 100, 120, 500, 0, 0, 24, 30}` — the four live view
bodies are handler indices 0..3 of the table `(*(&PTR_FUN_00108B80)[i])`.

* **Path A** — `FUN_000505D0` (decompile): the draw-loop view driver
  `FUN_0004D2D4` (`disassemble_bytes 0x4D3B0..`: with `FUN_00037AE4() == 0`
  and `[0x107DD8] == 0` it calls `FUN_000505D0` with `EAX=1`) ends in the
  default arm `if (iVar5 != 0) (*(&PTR_FUN_00108B80)[iVar6])(PTR_DAT_00107dc8,
  puVar7)` (`0x51056`), where `iVar6 = [view+4]` (the fresh camera-0 object
  `0x107508` has `+4 = 0`, first-hand `read_memory 0x107508`), `puVar7` is the
  behaviour record `(&PTR_DAT_00108B64)[selector]`, and `iVar5 =
  FUN_0004B8D4([view+0x3C] - [view+0x48], view)` (`0x4B8D4`, a state-index
  helper returning -1/0/1). The arm runs when the `FUN_00053D50()` value
  (`= DAT_0014E57C`, the view-mode cell) is either `>= 0x20` or an unlisted
  switch value — fresh match mode 0 is the default arm.
* **Path B** — `FUN_0004D908` (decompile): `switch(param_1[2])` case 0 calls
  `FUN_0004E834(view, (&PTR_DAT_00108B64)[view[3]], 0, 0)`. `FUN_0004D908` is
  called from `FUN_0004D2D4` at `0x4D3E5` (the `FUN_00037AE4() != 0` branch)
  and from `FUN_0004D04C` at `0x4D0D3`. The fresh camera-0 object has
  `[view+8] = 0` (`read_memory 0x107508` bytes 8..11), so case 0 fires.

So the handler runs **per draw frame in the default view mode**, gated by
`FUN_00037AE4`/`FUN_0006401C` and the behaviour state helper — the exact
predicate bodies of `FUN_0004B8D4`/`FUN_0004BA00` are the porting leg R5-L1.

### 2.2 Write set (exact store sites, `disassemble_function 0x4E834`)

`ES`/`ESI` = `param_1` = the **view object** (`[0x107DC8]` = `0x107508`
family); `EDI` = `param_2` = the behaviour record. The stores, with the two
target objects:

| site | write | target |
|---|---|---|
| `0x4E85F` | `[EBP+4] = EAX` (EBP = EDI+0x34) | behaviour `+0x38` (delta) |
| `0x4E869` | `[EBP] = ECX` | behaviour `+0x34` (yaw accumulator) |
| `0x4E870/0x4E87C/0x4E89B` | clamps | behaviour `+0x34` |
| `0x4E8AA` / `0x4E8B2` / `0x4E8BC` / `0x4E8C8` | `[EDI+0x1C]`, `[EDI+0x18]` + clamps | behaviour `+0x18`/`+0x1C` (roll accumulator) |
| `0x4E92D` | `[ESI+0x18] = EAX` | view **z** |
| `0x4E984` | `[ESI+0x10] = EAX` | view **x** |
| `0x4E991` / `0x4E99F` | `[ESI+0x60] = EAX`; `[ESI+0x58] = (+0x58 + +0x60) & 0xFFFF` | view **yaw** + its increment |
| `0x4EA98` | `[ESI+0x58] = EAX` | view yaw snap (class 3/4, `0x4E9B1..0x4EA96`) |
| `0x4EAB0` / `0x4EACA` | `[0x14E4D8]` / `[0x14E4D4]` | the two horizon words |
| `0x4EBD6` / `0x4EBE4` | `[ESI+0x64] = EAX`; `[ESI+0x5C] = (+0x5C + +0x64) & 0xFFFF` | view **pitch** + its increment |
| `0x4EC5A` | `[ESI+0x14] += smoothed` | view **y** |
| `0x4EC87` | `[ESI+0x4C] += smoothed` | view **angle** |
| `0x4EC90-92` | `MOVSD x3` `[ESI+0x40..0x48] <- [ESI+0x34..0x3C]` | the previous-anchor triple |

Fresh `search_instructions [0x0015774c]` (76 matches, whole program) lists
**no reference in the 0x4C000..0x51xxx view-handler band** — `FUN_0004E834`
(and its siblings `0x4E3A8/0x4ECA8/0x4DB3C`) never read or write the FU-71
triple. The view object is a separate camera *rig*: its `+0x10/+0x14/+0x18`
are its own smoothed position, `+0x4C` the angle, `+0x58/+0x5C` yaw/pitch.

### 2.3 What the view fields feed vs the reset/stand-in

* `FUN_0004C904` (`disassemble_function`, `0x4C911..0x4C92B`) stages
  `[view+0x10/14/18]` → `0x14E4E0/E4/E8`, `[view+0x58/5C]` → `0x14E4EC/F0`,
  `[view+0x4C]` → `0x14E4F4`; the draw loop `FUN_000495B0` stores the block
  pointer (`0x495E4/0x495E9`), feeds `block+0x14` (= view `+0x4C`, the ratio
  angle) to `FUN_00043E30` (`0x49600`) and block `+0x04` to `[0x154354]`
  (`FUN_00058A44 0x58A5F`). `FUN_00058A44` builds the view matrix from
  `[EDX+0xC]` = view `+0x58/+0x5C` (`0x58A4D..0x58A5A`) **but the world eye it
  subtracts is `[0x109A70]` = `0x155AA4`** (`0x58A69`, `read_memory 0x109A70`
  = `a4 5a 15 00`), the FU-71 triple staged by `FUN_00036C70`
  (`0x36C95..0x36CA1`: `[0x155AA4] <- [0x15774C]`, three MOVSD). So the view
  object supplies **orientation + ratio**, never the eye.
* vs `FUN_000700F4`: the reset re-pins the **FU-71 triple** and zeroes the
  event state/tracked/pan counter; `FUN_0004E834` touches none of those cells.
* vs the `(0,0,0)` kickoff seed: orthogonal — the kickoff pins the **place**;
  the view-object statics (`+0x58 = 0xBF9C`, `+0x5C = 200`, `+0x4C = 0x1500`,
  FU-154 §2.2) are what the engine's phase-12 T2 seed models.

**Verdict:** R2-L1 is the live **orientation/ratio** evolution after the static
seed. Porting it (into `fifa96_camera_pose_feed`'s view-mode-0 arm) would give
live yaw/pitch/zoom framing, but it **cannot move the camera place** and is not
required for translation. The engine's `view_mode 0` no-op is faithful only
until a port lands.

---

## 3. R2-L4 — the pan integrator's live inputs

### 3.1 Which cells feed `FUN_000736AC` live

The walk runs every frame (`callers`: `FUN_0004B100 0x4B193` and `0x743EF`,
fresh `get_xrefs_to 0x736AC` = 2). Integration requires
`word[0x1577BE] (speed) != 0` (`0x737DF/0x737E7`); the timer/height section also
opens on `word[0x1577F0] (event height) != 0` (`0x73775`). The live writers:

* **the event setter chain `FUN_00071C94` → `FUN_00070544`** sets the seed
  pair → velocity `seed/F8` (fast path) and `speed = metric(vel)`; the engine's
  `fifa96_camera_event_set` is the port. Fresh `get_xrefs_to 0x71C94` = **11**
  (this session; same set as FU-156 §2).
* **the follow `FUN_00071DF4` first arm** sets the velocity words from the
  tracked record's slot dirs ×15, re-computed every frame
  (`fifa96_camera_rate_event`; requires `[0x1577EE].hi > 0xF0`).
* **the head RNG jitter** inside `FUN_000736AC` itself (§1) — only perturbs an
  existing velocity and needs `speed > 0xC && Y > 0xA0`.
* **`FUN_0007131C`'s mirror** (`0x7193C` writes `[0x15774C]`) — the
  out-of-bounds reaction.

### 3.2 The wired producers, engine-side

`grep` over the engine: `fifa96_camera_event_set` call sites =
`fifa96_match_handlers.c:524` (row-1E stage-5 release), `:2428` (row-04
half-line events), `fifa96_match_run.c:2910` (the ball-staging receive arm);
`fifa96_camera_record_event` = `match_run.c:615` (the BF20 per-record tail);
`fifa96_match_run_camera_follow` = `match_run.c:3225` (the every-frame call
site after `fifa96_camera_update_walk` at `:3212`).

**The row-04 half-line gate (first-hand, `0x7ED89..0x7ED9E`):**

```
0x7ED89  EAX = 0x40; BX = word[rec+0x81] (timer81); CX = word[rec+0x6B] (lane)
0x7ED99  EAX -= EBX
0x7ED9B  CMP AX,CX ; JGE 0x7EDCE        ; (0x40 - timer81) >= lane -> bound tail
0x7EDA0  rec == [0x1577CA] -> 0x7F138   ; the track reload arm
0x7EDB2  lane >= 0x90 -> 0x7F138
0x7EDCE  CMP CX,word[rec+0x77] ; JG 0x7F138   ; lane > bound -> return
0x7EDD8  CMP dword[0x157750],0x50 ; JLE 0x7EE4B  ; ball height <= 0x50 -> half line
0x7EE4B  ...  the angle/face gate, then 0x7EF42/0x7EFEE build the 0x71C94 event
```

So the row-04 pan events fire when the record is **within `0x40 - timer81` of
the camera** (the lane is the record↔camera metric produced per frame by
`0x7C776..0x7C7AF`, engine `fifa96_action_locomotion_track`) — i.e. when a
player runs under the camera, exactly the "fast pan" narrative of FU-154 §5.2.

### 3.3 The six unreachable `FUN_00071C94` arms — verdict

Fresh `get_xrefs_to 0x71C94` = 11 confirms FU-156's census and dispositions.
None is a **precondition** for live motion:

* `0x7F4E7` (row-05 stage-0 tail, `out.tail`, OL-63) is **adjacent**: it is the
  carrier-claim tail (`candidate tracked-side producer; FUN_00071C94 stores
  the carrier into [0x1577CA]`), the same claim frame the measured pan fires
  in (§3.4) — porting it adds a second pan event on the claim, it does not gate
  the existing one.
* `0x71B8A` (the `FUN_0007131C` angle arm) only fires **after** the camera is
  out of bounds (`|x| > 0x6C0 || |z| > 0xAB0`) — a reaction, not a starter.
* `0x6FA62`/`0x77423`/`0x77DC6`/`0x82A6C` sit behind unported
  tails/unwired rows (keeper/row-0D situations) — secondary movers.

An engine-side gap worth noting: the ball staging tail's `camera_zero`
(`0x7A91D..0x7A92D` = `0x1577BE/C0/C2 := 0`, `fifa96_ball_pairing.c:173`) has
**no consumer** (`grep camera_zero` = only the struct/loader), so the native
"zero the pan at the staging" is dropped (leg R5-L6).

### 3.4 Measured live behavior at HEAD (first-hand probes)

Deterministic, null backend, ISO fixture, no fixture pokes (probe sources
`/tmp/opencode/r5probe*.c`); frames are granted frames.

* **The claim pan (probe 3/4).** Kickoff → KICK → phase 2 → V request → walk
  UP to the carrier: at the claim frame record 9 stands at `(0,0,-2)` (lane
  `2`, bound `2`), the row-04 half-line gate passes, and the camera gets
  `vel_x = 5` (`= (slot_dir_x << 6) / 0xC = 64/12`), `event_param = 0`
  (idle arm `pk = 6`, `tl = 12`, `t = 0`), `tracked = 9`. The integrator then
  pans **`pos_x: 0 → 0x90 (144)` over 24 frames** with vel
  `5 → 3 → 2 → 1` (the pan-step decay), then stops. This is live, real motion —
  a one-shot slide, not a follow.
* **The plain kick does not produce an event** (probe 1 A): after the kick,
  `pair.code = 0/1`, `ack = 0`, the camera holds at `0x90`, `vel = 0`,
  `event_param = 0` for 120 frames.
* **The long-ball release kick stages mode `0x40` but not a receive row**
  (probe 3 `R` frame): `pair.code = 1`, `flags = 0x10`, `traj = 0xCD`,
  `ack = 0` — the receive arm (`fifa96_match_run_ball_receive`) does **not**
  run, so no camera event fires. The phase-11/12 fixtures reach the receive row
  (code 4/13) only with their pinned band boundary / pass row respectively
  (`test_live_pass_kick_runs_receive_arm`: `pair.code == 13`, `ack == 1`,
  `camera.tracked == id`).
* **The follow never wakes live** (all probes): `event_param` never exceeds 0;
  `fifa96_match_run_camera_follow`'s gates (`event_param > 0x10` at 0x739DC,
  `rate_event`'s `height > 0xF0`) are never satisfied, and no measured frame
  shows the ±15 tracked-dir signature.

So R2-L4's answer: **the integrator's live input in open play is the row-04
half-line/record-event family (already wired, measured live); the follow is
wired but starved of a tall event; no `FUN_00071C94` arm needs to land first.**

---

## 4. T1-L3 — the bootstrap

### 4.1 Fresh `FUN_000700F4` census (this session)

`search_instructions CALL 0x000700f4` = **29** (same count as FU-154 §3.1, but
this fresh sweep adds one attribution FU-154's table did not name):

* **kickoff bodies** `0x88E6A`, `0x88FA0` → static `[0x10F328/2C/30]` = (0,0,0),
  flag 0 (quoted in §1);
* **event setter** `0x71CC3` (current triple, flag 0);
* **`FUN_0006FFC0`** `0x7000F`;
* **`FUN_0006E8E8`** `0x6FAAC` — the animation machine's **aux-0x32 arm**
  (`0x6FA8F..0x6FAC2`, first-hand): `PUSH 1`; args `[EBP+0x59/0x5D/0x61]` = the
  **record position** → `FUN_000700F4(pos_x, pos_y, pos_z, flag=1)` (a camera
  cut to the record **plus the pause flag**); the jump-table decode gives
  `table[49] = 0x6FA8F` → index 49 → **aux 0x32** (`read_memory 0x6E800`,
  58 dwords; `[33] = 0x6F9EB` aux 0x22, `[34] = 0x6FAC7` — so FU-156 §3.3's
  "the arm runs 0x6F9EB..0x6FA8F (next entry [49] = 0x6FA8F)" conflates two
  arms; the aux-0x22 arm ends with `JMP 0x6FBE6` at `0x6FA8A`). The executable
  FU-84 frame records the engine already ports
  (`match_run_frame_records`, `match_run.c:173`) carry aux 0x32 on anim rows
  44/45/46 frames 0/0/2 and row 92 (anim 0x5C) frame 1 (first-hand table scan
  of the ported bytes) — so this native place producer is **keyed on the same
  unported row-selector family as the jump rows** (FU-153 §10), leg R5-L2.
* **phase machine** `0x8D301/0x8D3CA/0x8D511`, and the sequence/set-piece band
  `0x7405E…0x92EBF` (21 sites; the row-1E/1D/1B/1C and restart bodies).

Every one of the 29 either **re-pins to a known place** (kickoff/restarts/
cinematics) or **preserves** (event setter, `FUN_0006FFC0`). Nothing in the
census runs per-frame in open play, so the reset family does not "fight" a live
pan except at kickoffs/restarts — where the re-pin is correct native behavior.

### 4.2 The live `[0x157750]` producers (what can raise it)

Carried FU-153 §4.3 (two classes) + fresh checks this slice:

1. **the arc** `FUN_000736AC 0x737AE/0x737B4` → `FUN_00070B94(timer)` while
   `word[0x1577F0] != 0` — ported (`fifa96_camera_height`), live **only if an
   event with a nonzero height exists**.
2. **direct cuts.** The keeper-claim family (`fifa96_keeper.c:456/485/587/
   589/678/680/714/716/725/727`) is ported and wired through the row-1E
   release (`handlers.c:519-528`: camera := staged `cam_x/y/z`, event height
   `0x50`, tracked) — its `cam_y` values are `pos.y + 0x38`, `0x50`, `0x80`,
   `0x10` (unit-tested in `tests/test_keeper_machines.c:84/152/191/224`), i.e.
   a real Y source **if a live keeper claim reaches those stages** (leg R5-L7).
3. **the throw-in stage-3 cut** (`0x855F0` body, FU-149 §1.6 stage `0x858E4`):
   first-hand this session —

   ```
   0x85904  EDI = 0x15774C                  ; destination = the camera triple
   0x85909  CALL 0x8F188                    ; the 0x18 sink
   0x8590E  ESI = &rec[+0x59]               ; source = the RECORD position
   0x85911  MOV EAX,0x80
   0x85916  MOVSD ES:EDI,ESI   x3           ; [0x15774C/50/54] <- rec.pos
   0x85919  MOV [0x00157750],EAX            ; [0x157750] = 0x80 (the override)
   0x85935  EBX = [0x15774C]; ECX = [0x157754]
   0x8594F..0x8595F  ECX += type<<6; PUSH ECX (z)
                     EDI = [0x157750]; PUSH EDI (y = 0x80)
                     EBX += type<<6; PUSH EBX (x)
   0x8596B  CALL 0x700F4                    ; the Y is PRESERVED
   ```

   The engine's port of exactly this block (`fifa96_match_handlers.c:1467`)
   calls `fifa96_camera_init(&mr->render.camera, r->pos_x + dx, 0, r->pos_z +
   dz)` — **Y = 0**: the native `[0x157750] = 0x80` write and the Y-preserving
   re-place were dropped by the derived subset (the comment at 1456-1459
   documents the subset, not the loss). The corner arm's cut
   (`0x85E88..0x85E98`) genuinely passes Y = 0 (first-hand `PUSH 0`; faithful),
   but the throw-in cut does not.

### 4.3 The kick path (engine) — measured

The kick rail stages receive rows only in specific states: a live PASS reaches
the receive arm with `traj = 0` (code 13); the long-ball mode-0x40 release
stages code 1 in the natural chain (probe, §3.4) — the receive row (code 4)
and its tall mode-0x40 traj (`0xE0`/`0x100`, FU-153 §5) need either the exact
band-2 boundary (`d == 0x70`) or the **airborne height-table** row
(`d = [0x157750] - rec.pos_y >= 0x38`, FU-153 §5.1, no band precondition).
Both need `[0x157750]` off zero **before** the kick — the T1-L3 bootstrap.

### 4.4 What raises the place

The place (`pos_x/pos_z`) is moved by (a) the pan integration (measured live,
§3.4), (b) the direct cuts (row-1E release, restarts; the throw-in cut's X/Z
offset is ported), (c) `FUN_00079F3C` (targets, not the camera — FU-96 §7).
Nothing else in the census writes the triple in open play.

---

## 5. The ranked minimum honest producer set + T1 wiring recommendation

### Ranked set

1. **The throw-in stage-3 Y (`0x85919` = `[0x157750] := 0x80`; native
   0x85911..0x8596B).** One restored constant in a wired, live-reachable
   restart arm (`fifa96_match_action_10`, `handlers.c:1467`). After any live
   throw-in the camera Y is `0x80` — the **only** live, in-open-play
   `[0x157750]`-off-zero producer the port currently drops. It also wakes the
   BF20 record-event gate (`0x7C81B` `[0x157750] != 0`).
2. **The airborne kick route (T1-L1, R7/T3).** With Y = `0x80`, a jumping
   record's kick takes the height-table code-4 row
   (`0x1102FE` class-2 index 4; `d = 0x80 - pos_y`), the mode-0x40 draw gives
   `traj 0x90..0xE0` / `0x100`, the receive arm sets `event_param > 0xF0` +
   `tracked` (all already wired). The live jump row selector is the T1-L1 leg
   (FU-153 §10-11); the fixture already proves the rest end-to-end.
3. **The follow + integrator (already wired, fixture-proven).** Once
   `event_param > 0xF0` + `tracked`: `fifa96_match_run_camera_follow` →
   `fifa96_camera_rate_event` sets `vel = tracked slot dirs ×15` every frame →
   the integrator moves the place by `vel × delta` — the **sustained** visible
   follow.
4. **The already-live claim pan (row-04 half-line, measured).** No new
   producer; assert and capture it.
5. **R2-L1 (`FUN_0004E834`) — not required for the place.** Orientation-only;
   port later for live framing.
6. **The six `FUN_00071C94` arms — none required** (secondary movers/reactions).

### Concrete T1 wiring recommendation (order + tests + smoke)

**Step 1 (assert the live place that already works; no producer change).**
New ISO test `test_live_claim_pan_row04_half_line` in
`tests/test_engine_match_frame.c`: kickoff → KICK → phase 2 → V → walk claim;
assert at the claim `camera.tracked == id`, `vel_x == (dir_x << 6) / 0xC`
(`== 5` for dir 1), `event_param == 0`; then over the next 30 granted frames
the camera `pos_x` walks monotonically `0 → 0x90` and `vel` decays
`5→3→2→1`. This pins the measured row-04→`fifa96_camera_event_set`→
`fifa96_camera_update_walk` chain; RED if the row-04 event wiring is removed.

**Step 2 (restore the bootstrap — the T1-L3 fix).** In
`fifa96_match_action_10` stage 3 (`handlers.c:1452-1469`) restore the native
`[0x157750] = 0x80` before the place and pass `0x80` (not `0`) as the
`fifa96_camera_init` Y: the native cut is `FUN_000700F4(x+dx, 0x80, z+dz, …)`
(first-hand 0x85911..0x8596B quoted in §4.2). Test
`test_live_throw_in_camera_y_cut`: drive the throw-in situation; assert
`camera.pos_y == 0x80`, the record-event tail gate now passes
(`ball != 0`), and a subsequent live grounded kick's band `d = 0x80 - pos_y`
is `>= 0x38` (the height-table predicate). Goldens: live-only paths → expect
no M1/M2 re-pin (the tape stages no throw-in).

**Step 3 (the integration with T3/T1-L1, after the live jump selector lands).**
Extend the ISO live chain: throw-in → the taker's team jumps (aux 0x0E/0x3A arm,
already ported per-record) → KICK; assert the height-table code-4 row staged
(`pair.code == 4`), the mode-0x40 draw landed, the receive arm bound
`tracked`, `camera.event_param > 0xF0`, and
`fifa96_match_run_camera_follow(&mr) == 1` with
`camera.vel == tracked dirs ×15` — then that the **next frames'**
`camera.pos_x/z` advance by `vel × delta` while the event holds (the sustained
follow; the event height decays only at the pan steps, F4 = 2·ramp(H), so the
follow holds for ~F4/2 granted frames from a `0x100` event — the k=7 draw).

**Step 4 (smoke `p13-t1-*`, `DISPLAY=:1`).** Two captures: (a) the walk-claim
slide — the scene must shift horizontally as the camera pans `0 → 0x90`;
(b) after the throw-in + jump-kick, the camera pans continuously while the
ball stays framed (FU-154 §1.2: the native yaw keeps the lead on canvas over a
wide camera range). Record honestly if the airborne route cannot be driven
from the pad in one live pass (then the throw-in Y cut alone is the landed
step, and the follow assertion stays fixture-level).

**Explicit non-goals:** do not touch `[0x157750]`'s row-body stand-ins
(FU-75 §1.6 / leg 15.b) beyond the throw-in cut; do not port R2-L1 in T1; do
not add `FUN_00071C94` arms.

---

## 6. Errata candidates (quoted, first-hand)

1. **FU-156 §3.3 arm range.** "the arm runs `0x6F9EB..0x6FA8F` (next entry
   `[49] = 0x6FA8F`)" conflates the aux-0x22 arm (`[33]`, ends `JMP 0x6FBE6`
   at `0x6FA8A`) with the aux-0x32 arm (`[49]`, entry `0x6FA8F`,
   `PUSH 1` → `CALL 0x700F4` at `0x6FAAC`); fresh `read_memory 0x6E800` gives
   `[34] = 0x6FAC7` and `[49] = 0x6FA8F`.
2. **`FUN_000736AC` pan-step admission.** The engine tests only
   `timer > timer_limit` (`fifa96_camera.c:95`); the native also pan-steps when
   `timer <= limit && dword[0x15775C] > 0 && dword[0x157750] <= 0`
   (`0x737C8..0x737D8`, the previous-Y descent step) — FU-152 §2.9's summary
   omits this second term.
3. **The pause cell.** Native `[0x157820]` (`FUN_000700F4`'s flag argument,
   `0x7021B`; checked by `FUN_000736AC 0x7375E`) is modelled engine-side as
   `mr->flag_157820` (row-1E `flag_write`, `handlers.c:509`) while the walk
   reads `cam->paused` (`camera.c:77`), which nothing live writes. The native
   pause therefore does not stop the engine pan.

## 7. Numbered legs

* **R5-L1 — the default-arm run predicate of `FUN_0004E834`.** The exact
  semantics of `FUN_0004B8D4`/`FUN_0004BA00` (`0x4B8D4`, `DAT_001074A8`/
  `DAT_00157AC2`) and `FUN_00037AE4`/`FUN_0006401C` — the porting gate for
  R2-L1 (the write set is derived; the run condition is not).
* **R5-L2 — the aux-0x32 live selector.** Which live animation selections
  reach anim `0x2C/0x2D/0x2E/0x5C` (the frame records carry aux 0x32); the
  arm's place+pause (`FUN_000700F4(pos, flag=1)`) cannot be woken engine-side
  until a selector reaches them (same family as FU-153 T1-L1).
* **R5-L3 — the throw-in cut's full native shape.** The `0x85911..0x8596B`
  block's dropped neighbours (the `0x4C380` no-op, the animation-frame gate,
  the `0x8DCD4` clamp/folds per FU-149 L13) — the Y=0x80 restore is bounded;
  the rest stays carried.
* **R5-L4 — the `FUN_000736AC` head jitter** (`0x736B2..0x73723`, the RNG
  ±1 walk of `word[0x1577C0/C2]`) and the `0x72AC4` save block — unmodeled.
* **R5-L5 — the pause bridge.** Wire `mr->flag_157820` into the walk's pause
  (or model the cell exactly); also `0x157822`'s writer.
* **R5-L6 — the `out.camera_zero` consumer** (`0x1577BE/C0/C2 := 0` at the
  staging tail / carrier stage 1; `fifa96_ball_pairing.c:173`,
  `fifa96_action_handlers.c:522`) — currently dropped.
* **R5-L7 — the live keeper-claim reachability** of the row-1E stage-5
  release (`handlers.c:519`; the unit path is proven,
  `tests/test_keeper_machines.c:224`) — the second Y producer candidate.

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_xrefs_to` `0x4E834` (3), `0x736AC` (2), `0x71C94` (11 fresh), `0x4D908`
(2), `0x71C94`-family as carried; `get_function_by_address` `0x85919` (no
function); `decompile_function` `0x4E834`, `0x505D0`, `0x4B8D4`, `0x53D50`,
`0x4D908`, `0x700F4`, `0x4C904` (as `disassemble_function`); `disassemble_bytes`
`0x4E834` (364 insns), `0x4C904`, `0x495B0`, `0x58A44`, `0x56E50`, `0x36C70`,
`0x736AC..0x738D0` (130 insns), `0x7ED80..0x7EE50`, `0x6E800`-adjacent
(`0x6FA50..0x6FAC7`), `0x700F4..0x7022F`, `0x88E40..0x88E80`,
`0x4D3B0..0x4D3F8`, `0x85E88..0x85EA5`, `0x85910..0x85980`; `read_memory`
`0x108B80`, `0x107508`, `0x109A70`, `0x6E800` (232 B); `search_instructions`
`[0x0015774c]` (76), `CALL 0x000700f4` (29). Engine source read:
`fifa96_match_run.c` (540-620, 1080-1300, 1530-1560, 2130-2210, 2820-2960),
`fifa96_match_handlers.c` (430-530, 1374-1560, 2230-2490),
`fifa96_camera.c/h`, `fifa96_ball_pairing.c` (255-330, 590-835),
`fifa96_keeper.c` (450-730), `fifa96_action_handlers.c` (300-400),
`fifa96_arm_helpers.c` (96-117), `tests/test_engine_match_frame.c`
(1854-2298), `tests/test_keeper_machines.c` (45-224).

Probes (scratch only, not committed):
`/tmp/opencode/r5probe.c` (kick/pass/long-ball camera traces),
`r5probe2.c` (prologue + correct-order request), `r5probe3.c` (frame markers),
`r5probe4.c` (the claim-frame lane/ball state) — linked against
`build/libfifa96_*.a` (sanitized build), run from the repo root with
`game/FIFAPCCD96.iso`. Key quoted outputs: claim pan `0 → 0x90` (144) with
`vel 5→3→2→1`; `event_param` never > 0 in the natural live chain; the
long-ball release staged `pair.code == 1`, `flags == 0x10`, `ack == 0`.

Carried docs: FU-96 §4/§7, FU-148 §12/§13/§15, FU-152 §2.8/2.9, FU-153
§4-5/§10-11, FU-154 §2-5/§9, FU-156 §2-3/§5, FU-149 §1.6 stage table,
FU-70 §slot `+0x23`.
