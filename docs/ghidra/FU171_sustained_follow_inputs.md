# FU-171 — sustained-follow event inputs (the follow rate, the row-04 half-line, the `0x157750` events)

Recon wave R19 of `docs/superpowers/plans/2026-10-12-fifa96-m2-phase14-live-continuity.md`.
Read-only Ghidra session, authoritative program **`/FIFA96.EXE`** (case-sensitive
program argument; Ghidra project path `/FIFA96.EXE`; scripts disabled; standard
MCP tools only). Every address/byte claim below is a fresh `disassemble_bytes` /
`search_instructions` / `get_xrefs_to` of this session; engine-side facts cite
the built sources by line. Alias traps applied throughout — this slice leans on
`dword[x]>>16` = word at `x+2` at `0x71E7C`/`0x71EA8` (the follow's ±15
velocity clamps), `0x7EDAC` (the row-04 lane), `0x75DB8` (the sequence-band
camY argument), `0x7A20C` (`[0x15873C]>>16` = word `0x15873E`, the receive
arm's traj height), `0x7C80A` (`[0x1577BC]>>16`, the BF20 speed) and `0x73A95`
(`[0x1577EC]>>16`, the follow's approach word). Ported-table arithmetic is
labelled as such. No repo file was modified by this slice.

Question (plan R19): what drives the follow *rate* live? Derive
`fifa96_camera_rate_event`'s live inputs, the row-04 half-line reachability
(FU-154 §9/§5.1) and the `0x157750` place/target events in open play — the
coupling that turns the phase-13 asserted pan (`0 → 0x90` in the ISO test)
into sustained on-screen following. This is the R2-L4/T1-L3 residue.

Result in one line: **the follow rate is the tracked record's slot dirs ×15,
re-set every frame by `FUN_00071DF4`'s first arm while `[0x1577F0] > 0xF0` and
the pan counter `[0x157821] == 0`; the event height is `max(height_arg, camY)`
(setter clamp `0x71CDB`), and the fresh `[0x1577F0]` writer census shows only
the setter, the per-record `FUN_00070C08` ramp/reflect and the decays ever
write it — so the row-04 half-line events (height arg 0, admitted only while
`camY <= 0x50`) can never wake the follow; the only live wake routes are the
receive arm's staged trajectory (`0x100`, the k=7 mode-0x40 draw) and the
record-event tail ramp/reflect — both requiring an airborne record
(`actor->pos_y != 0` and `[0x157750] − pos_y >= 0x38`).**

---

## 1. Scope and method

The slice bounds three first-hand derivations plus the coupling:

1. `fifa96_camera_rate_event`'s live inputs — native `FUN_00071DF4`'s first
   arm, its two gates and its write set, and the every-frame call site's gates
   inside `FUN_000736AC` (`0x739C6..0x73B6B`).
2. The event-height calculus — a fresh `[0x1577F0]` / `0x1577F2` writer census
   and the eleven `FUN_00071C94` callers' height arguments (the decisive
   "what can wake the follow" census).
3. Row-04 half-line reachability — the full `0x7ED89..0x7F0D6` gate chain and
   both event-build arms, against the engine's ported `fifa96_outfield_row04_step`
   staging.
4. The `0x157750` place/target events in open play — the fresh 84-site
   read/write census, the per-frame arc writer, and the setter's clamp that
   makes camY the event-height floor.

Beyond the plan's starting windows this slice also re-read the engine's live
ISO chains (`test_live_claim_pan_row04_half_line`,
`test_live_long_ball_mode40_band2_follow`,
`test_live_airborne_long_ball_seed_then_band2_follow`) to separate what is
already proven live from what is staged.

## 2. The cell model, re-verified

### 2.1 `[0x157750]` is the camera-Y cell and the shared ball-height proxy

`FUN_00071C94`'s place push (`0x71CA6..0x71CBB`) re-confirms the FU-71 triple
order: `PUSH 0` (flag), `[0x157754]` (z), `[0x157750]` (y), `[0x15774C]` (x)
into `FUN_000700F4`. The fresh `search_instructions [0x00157750]` = **84
sites**: **10 writers** and 74 reads/tests. The writers are

| site | body | first-hand shape |
|---|---|---|
| `0x737B4` | `FUN_000736AC` (the walk) | `MOV [0x157750], EAX` — the per-frame arc (see §2.3) |
| `0x7456E`, `0x755A9`, `0x75A0F`, `0x75D69`, `0x75EEA`, `0x75F86`, `0x76DB5`, `0x7751B` | the `0x74xxx..0x77xxx` sequence/set-piece band (attribution carried from FU-153 §4.3; the region is jump-table code, no analyzed function) | direct dword cuts |
| `0x85919` | the throw-in stage-3 cut | `MOV [0x00157750], EAX` (EAX = 0x80), the phase-13 T1 restore |

Everything else is a test/read: `0x736C3` (the head-jitter gate `> 0xA0`),
`0x737D1` (the descent pan-step admission — `JG 0x737DF` skips the pan step
when camY > 0), `0x7EDD8` (the row-04 half-line admission `<= 0x50`),
`0x7C81B` (the BF20 record-event tail gate `!= 0`), `0x7C861`/`0x7C871` (the
record's height band), `0x7F4D0` (the row-05 tail's `> 0x10` read), the
row-0E arm (`0x82820`, R15's target),
`0x8061B`/`0x81157` (`>= 0x38` — the height-table reads), etc.

The census confirms FU-157 §4.4: **nothing else moves or reads the place in
open play**; per-frame, the cell is written only by the arc (`0x737B4`), and
the arc is skipped when the event height is zero (`0x737A1 TEST CX,CX; JZ
0x737B9`).

### 2.2 The setter `FUN_00071C94` — first-hand body

```
0x71C99  CMP byte[0x157A6C],0; JNZ 0x71DEF   ; the suspend gate (fresh xrefs:
                                              ; writers 0x6E5F7/0x6E602
                                              ; FUN_0006E598, 0x73E7B
                                              ; FUN_00073E28, 0x7741A)
0x71CA6..0x71CBB  PUSH 0; PUSH [0x157754]; PUSH [0x157750]; PUSH [0x15774C]
0x71CBC  MOV ESI,EDX        ; ESI = the caller's 3-word seed vector
0x71CBE  MOV EDI,0x1577B8
0x71CC3  CALL 0x000700F4    ; the place/reset (flag 0 -> pause flags clear)
0x71CC8  MOVSD ES:EDI,ESI   ; 6-byte seed copy -> 0x1577B8..0x1577BD
0x71CC9  MOVSW ES:EDI,ESI
0x71CCB  MOVSX EAX,BX       ; the height argument (signed)
0x71CCE  MOV EDX,[0x157750]
0x71CD4  MOV word[0x1577F0],BX          ; event height := arg
0x71CDB  CMP EAX,EDX; JGE 0x71CEB       ; CLAMP UP: if (height < camY)
0x71CDF  MOV AX,word[0x157750]          ;   height := camY
0x71CE5  MOV [0x1577F0],AX
0x71CEB  MOV EAX,[0x1577EE]; SAR 16; CMP EAX,0x640; JLE 0x71D03
0x71CFA  MOV word[0x1577F0],0x640       ; clamp 0x640
0x71D03  TEST EBP,EBP; JZ 0x71D60       ; the tracked arg
0x71D27  MOV [0x1577CA],EBP             ; tracked := the record argument
0x71D60  MOVSX EAX,CX                   ; the 4th arg = the FUN_00070544 ramp param
0x71D65  CALL 0x00070544
0x71D6A  MOV byte[0x157815],DL          ; DL=0 (0x71D63 XOR DL,DL)
```

Three consequences, first-hand:

* **the event height is `max(height_arg, camY)`** (clamped to `0x640`) — so
  *any* setter call on a camera whose Y is nonzero silently carries camY as
  the event height floor;
* the `0x71C99` bail returns without the reset and without the tracked store
  (the engine contract, `fifa96_match_run.h:951`);
* `0x71D27` is one of only **three** writers of `[0x1577CA]` (fresh
  `get_xrefs_to 0x1577CA` = 88 refs; writes: `0x71D27` the store, `0x70258`
  `FUN_000700F4` clear, `0x70054` `FUN_0006FFC0` clear) — the reset inside
  every setter *clears* tracked, then the store rebinds it when the argument
  is nonzero.

### 2.3 The height producer `FUN_000736AC` — first-hand re-read

```
0x7376B  CMP word[0x1577BE],0; JNZ 0x73783
0x73775  CMP word[0x1577F0],0; JZ 0x73B70    ; no speed and no event -> skip
0x73783  BX = word[0x157A64] (delta); DX = word[0x1577FA] (timer)
0x73791  ADD EDX,EBX; MOV [0x1577FA],DX      ; timer += delta
0x737A1  TEST CX,CX (CX = word[0x1577F0]); JZ 0x737B9
0x737A6  EAX = word[0x1577FA]; CALL 0x00070B94; CWDE
0x737B4  MOV [0x157750],EAX                   ; camY := the arc (absolute store)
0x737B9  AX = timer; CMP AX,word[0x1577F4]; JG 0x737DA   ; pan step when timer > limit
0x737C8  CMP dword[0x15775C],0; JLE 0x737DF   ; dword[0x15775C] = the PREVIOUS Y
0x737D1  CMP dword[0x157750],0; JG 0x737DF
0x737DA  CALL 0x000709D0                      ; the descent pan step
```

`0x737C8`'s cell is the previous-triple Y (`0x73756 MOVSD x3: 0x157758 <-
0x15774C` copies the previous x/y/z to `0x157758/5C/60`), so the second
pan-step admission is `prevY > 0 && camY <= 0 && timer <= limit` — FU-157 §1's
"descent step", confirmed.

## 3. `fifa96_camera_rate_event`'s live inputs

### 3.1 The first arm of `FUN_00071DF4` — first-hand write set and gates

```
0x71DF8  EDX = [0x1577CA]; TEST; JZ 0x7200B          ; tracked != 0
0x71E0B  EAX = EDX; EDX = [EDX+4]; CMP dword[EDX],0x18D8; JNZ 0x71EF0
0x71E1C  CMP dword[EAX+0x20],0; JZ 0x71EF0           ; the slot must exist
0x71E26  EAX = [0x1577EE]; SAR 16 (= word[0x1577F0]); CMP EAX,0xF0; JLE 0x71EF0
0x71E39  CMP byte[0x15B650],0; JNZ 0x71E4E
0x71E42..0x71E49  FUN_00092820(0, 0x26)              ; the follow sound latch
0x71E4E..0x71E71  [0x1577C0] = (int16)slot[+0x20] * 0xF   ; vel_x
                  [0x1577C2] = (int16)slot[+0x21] * 0xF   ; vel_z
0x71E77..0x71EC6  the ±15 clamps (alias-checked: the compares read
                  word[0x1577C0] at 0x71E7C and word[0x1577C2] at 0x71EA8)
0x71ECF..0x71EE5  speed [0x1577BE] = distance(vel_x, vel_z)
```

So `fifa96_camera_rate_event`'s live inputs are exactly:

1. **`[0x1577CA]` (tracked)** — set by the event setter (`0x71D27`) for any
   nonzero record argument, cleared by the two resets;
2. **`[[tracked]+4][0] == 0x18D8`** and **`[[tracked]+0x20] != 0`** (the
   record's descriptor class and slot pointer);
3. **`word[0x1577F0]` (event height) `> 0xF0`**;
4. **the slot dir bytes `slot[+0x20]`/`slot[+0x21]`** (the record's facing
   octant pair — engine `e->dir_x`/`e->dir_z`);
5. the output: `vel = clamp(dirs × 15, ±15)`, `speed = |vel|`, plus the
   0x92820(0,0x26) sink request.

The engine port (`fifa96_loader/fifa96_camera.c:525-533`) is exact, including
the `height <= 0xF0` return 0 and the ±15 clamp.

### 3.2 The every-frame call site (`0x739C6..0x73B6B`) — first-hand

```
0x739BB  DL = byte[0x157821] (the pan counter)
0x739C6  TEST DL,DL; JNZ 0x73B70        ; GATE 1 pan_counter == 0
0x739CE  CMP word[0x1577BE],0; JZ 0x73B70   ; GATE 2 speed != 0
0x739DC  EAX = [0x1577EE]; SAR 16; CMP EAX,0x10; JLE 0x73B70   ; GATE 3 height > 0x10
0x739ED  CMP byte[0x157816],0; JNZ 0x73A03
0x739F6  CMP byte[0x157817],0; JZ 0x73B5B    ; GATE 4 both rate bytes zero -> the follow
0x73A03  (rate bytes nonzero -> the interpolation arm; see §3.3)
0x73B5B  EBP = [0x1577CA]; TEST; JZ 0x73B70
0x73B65  CMP dword[EBP+0x20],0; JZ 0x73B70
0x73B6B  CALL 0x00071DF4
```

Two rate paths are **mutually exclusive**: the rate bytes `[0x157816]/[0x157817]`
zero take the follow (`0x73B5B`); nonzero take the interpolation arm
(`0x73A03..0x73B59`, the engine's `camera.c:122-143`), whose gates re-open the
follow only when the tracked/class/height trio holds (the `0x73A42..0x73A7B`
timer/peak jiggle then `JMP`-falls into `0x73B5B`). The engine's
`fifa96_match_run_camera_follow` (`match_run.c:3128-3147`) models gates 1–4
and the tracked/slot pair exactly.

### 3.3 The "sustained" semantics — the pan counter is the window

`FUN_000709D0` (fresh `disassemble_bytes 0x70A30`): `0x70A3F` reads the pan
counter via `dword[0x15781E]>>24` = byte `[0x157821]`; `CMP EAX,0x64; JGE`
skips; otherwise `0x70A65 INC byte[0x157821]` — **every pan step increments
the counter** (capped at 100). `FUN_000700F4` clears it (`0x70153`), and every
setter call routes through `FUN_000700F4` (`0x71CC3`). Therefore:

* the follow runs **from the event set until the first pan step of that
  event**, then gate 1 locks it out until the next setter call;
* the pan step itself runs when `timer > timer_limit` (`0x737B9`; the second
  admission at `0x737C8`), and it *decays* the event height
  (`0x70A6B..0x70A90`: `event_param = event_param × pan_decay / 0x20`,
  `pan_decay = [0x157819] = 10` image default) and re-runs `FUN_00070544(0)`
  over the event's stored step pair.

Ported-table arithmetic (labelled): for a fresh H = 0x100 event at
`camY = 0x80`, `ramp(0x100) = 0x94 − T_ramp[(0x640−0x100)>>2 = 0x150] =
148 − 89 = 59` (`T_ramp[0x150] = 89`), so `F2 = 59`, `F4 = timer_limit = 118`,
`F6 = timer = 59 − ramp(0x80) = 59 − (148 − T_ramp[0x170 = 106]) = 17`; at
+2/frame the first pan step lands ≈ frame 51, i.e. the follow window of one
k=7 receive event is ≈ 51 granted frames.

## 4. What can raise the event height (`[0x1577F0]`)

### 4.1 Fresh writer census (31 xrefs)

`get_xrefs_to 0x1577F0` = **31 refs; 10 writes**:

| writer | body | shape |
|---|---|---|
| `0x71CD4`, `0x71CE5`, `0x71CFA` | `FUN_00071C94` | the arg, the camY clamp-up, the 0x640 clamp |
| `0x70575`, `0x705FE` | `FUN_00070544` | the height-arm 0x640 clamp / the idle-arm zero |
| `0x70A90` | `FUN_000709D0` | the pan-step decay |
| `0x70D17`, `0x70D3A` | `FUN_00070C08` | the two-step ramp (`target + (ev−target)>>1`, then the re-read step) |
| `0x70D97` | `FUN_00070C08` | the reflect arm (`event_param = ball_height` = camY) |
| `0x70159` | `FUN_000700F4` | the reset zero |

`[0x1577F2]` (the arc peak F2, fresh 14 refs): writes only at `0x7058B`/
`0x70605` (`FUN_00070544`: `ramp(height)` / the idle `6`) and `0x70160` (the
reset zero). **There is no other producer of the event height** — the follow
can only be woken by (a) a setter caller passing `height_arg > 0xF0`, (b) a
setter caller passing `height_arg > camY` with `camY > 0xF0`, or (c) the
`FUN_00070C08` ramp/reflect.

### 4.2 The eleven `FUN_00071C94` callers' height arguments (fresh xrefs = 11)

| caller | row/situation | height arg (first-hand) |
|---|---|---|
| `0x7EFCF` | row-04 row-0x13 arm | `XOR EBX,EBX` (`0x7EFC4`) → **0**; `ECX = 1` |
| `0x7F0D1` | row-04 general half-line arm | `XOR EBX,EBX` (`0x7F0C6`) → **0**; `ECX = 0` |
| `0x7A219` | `FUN_0007A084` receive arm | `EBX = word[0x15873E]` = the staged traj |
| `0x70DD1` | `FUN_00070C08` tail | re-enters with the current (`FUN_00070C08`-managed) height |
| `0x7F4E7` | row-05 stage-0 tail (the carrier claim) | **0** (`0x7F4DA`), `ECX = (camY > 0x10)` (`0x7F4D0 SETG`/`0x7F4E2 MOVSX`) |
| `0x6FA62` | `FUN_0006E8E8` aux arm | **0** (`0x6FA57`), `ECX = (camY > 0x10)` |
| `0x75DC4` | the sequence band | `EBX = dword[0x15774E]>>16` = **camY** (`0x75DB8/0x75DC1`) |
| `0x82A6C` | the `0x82xxx` arm (row-0D family, FU-157 §3.3) | **camY + (rng & 0x3F)** (`0x82A4F CALL 0x92AC8`; `0x82A61 ADD EAX,EDX`; `0x82A67 MOVSX EBX,AX`) |
| `0x77423`, `0x77DC6` | the `0x77xxx` set-piece rows | `MOVSX EBX,SI` — a register set outside the inspected windows (leg FU171-L5) |
| `0x71B8A` | `FUN_0007131C` (post-out-of-bounds) | `MOVSX EBX,SI`, record arg = `[0x1577CA]` (the tracked record); SI source outside the window (leg FU171-L5) |

**Verdict.** Only the receive arm passes a trajectory word that can exceed
`0xF0`; every other caller passes 0, camY, camY + (rng & 0x3F) or a
register-staged value, all of which the setter clamps to `max(arg, camY)`.
The receive arm's traj is the staged `0x15873E` word; its writers (fresh
`search_instructions 15873e` = 17 sites) are the stage core `FUN_0007A490`
(`0x7A4E3`, `0x7A638` the 0x460 clamp, `0x7A9A8`) plus the engine-mirrored
`FUN_0007A084`/`FUN_0007B9C4` snapshot/clear block — i.e. the engine's
`ball.pair.traj`.

### 4.3 The receive arm's event set (`0x7A207..0x7A219`) — first-hand

```
0x7A207  EDX = 0x158738                  ; the staged vector
0x7A20C  EBX = [0x15873C]; SAR EBX,16    ; = word[0x15873E] = traj
0x7A212  EAX = EBP                       ; the actor
0x7A214  XOR ECX,ECX
0x7A219  CALL 0x00071C94
```

The engine's `fifa96_match_run_ball_receive` (`match_run.c:3187-3191`) passes
`traj` as the height and binds `tracked = actor_id` on an applied set — the
port shape is exact, and `test_ball_receive_arm_high_ball_chain`
(`tests/test_engine_match_frame.c:4802-4813`) proves `event_param == 0x100`
and `tracked == 9` from `traj = 0x100`.

### 4.4 The record-event tail ramp (`FUN_00070C08` → `0x70DD1`)

The BF20 tail (first-hand `0x7C7C0..0x7C8FC`) calls `0x70C08` per record when
`word[+0x6B] lane <= 0x10 && lane < bound && ![0x157820] && ![0x157822] &&
speed > 4 && dword[0x157750] != 0 && dword[0x157750] ∈ [rec+0x5D, rec+0x5D +
0x70]` plus the tracked/class gates. `FUN_00070C08` then:

* **reflect arm** (`rec+0x5D + 0x68 > camY`, `0x70D91 MOV AX,[0x157750]`):
  negates one RNG seed by the |vel_x| vs |vel_z| compare and sets
  `event_param = camY` (`0x70D97`);
* **ramp arm** (else): `target = rec+0x5D + 0x70`, `event_param =
  target + (ev−target)>>1` then a second `+= (new−target)>>1` (`0x70D17`,
  `0x70D3A`), i.e. an asymptotic climb toward `+0x5D + 0x70`;
* then re-runs the setter with the current height (`0x70DD1`; `EAX = ESI` = the
  record → `0x71D27` rebinds tracked, and `EBX = word[0x1577F0]` = the current
  height), which resets the pan counter and re-binds tracked every time it
  fires. The tail stages `ECX = 1` at `0x70DBA` *before* the `0x92820` sink
  call and the setter call at `0x70DD1` does not reload it; whether `ECX`
  survives `0x92820` (the `FUN_00070544` ramp-param sign) is not statically
  decidable from this window — the engine passes 0 (`camera.c:1086`), which is
  the fall-through arm (leg FU171-L4).

So the record-event tail is a **per-record, per-frame chaser** — the only
native mechanism that re-arms the follow continuously — but its ramp crosses
`0xF0` only when `rec+0x5D > 0x80` (a genuinely high jumping record), and its
reflect arm only when `camY > 0xF0`. The engine port
(`match_run.c:576-619`) matches the gates first-hand; `ball == 0` (its
`cam->pos_y` staging, line 583) is the same hard stopper the current live
chain hits.

## 5. Row-04 half-line reachability (the R2-L4 residue)

### 5.1 The gate chain (`0x7ED79..0x7F0D6`, first-hand; engine port
`fifa96_outfield.c:607-718`)

```
0x7ED89  EAX = 0x40; BX = word[rec+0x81] (timer81); CX = word[rec+0x6B] (lane)
0x7ED99  EAX -= EBX; CMP AX,CX; JGE 0x7EDCE   ; near arm when 0x40 - timer81 >= lane
   far arm: rec == [0x1577CA] -> 0x7F138; lane >= 0x90 (alias 0x7EDAC) -> 0x7F138;
            else the 0x79B58 call and return
0x7EDCE  CMP CX,word[rec+0x77] (bound); JG 0x7F138   ; lane > bound -> return
0x7EDD8  CMP dword[0x157750],0x50; JLE 0x7EE4B      ; camY <= 0x50 -> the half-line arm
   camY > 0x50 arm (0x7EDE1..0x7EE4A): active && rec+0x5D == 0 && the
   POS-vs-[0x157770] metric < 0x30 && row != 0x13 -> install row 0x13
0x7EE4B  (half-line) the angle fold modulo 0x400, fold > 0x200 -> 0x400 - fold,
         reject |fold| > 0xAB; then the team-side sound arm; then the
         team_7E7/corner arm; then the slot arm; then the event build:
0x7EF0E  no slot OR not the 7cb tracker -> the event build
0x7EF42  row == 0x13 -> seeds = type-table <<6, height EBX = 0, ECX = 1
0x7EFEE  else if rec != [0x1577CA]:
0x7EFFA    camY != 0 -> seeds = type-table <<5
0x7F035    camY == 0 -> slot present: seeds = slot dirs <<6; no slot:
           |camX| > 0x630 or |camZ| > 0xA20 -> zeros, else rec vel words <<2
0x7F0D1  CALL 0x00071C94   (height EBX = 0, ECX = 0)
```

Reachability, exact:

* the half-line pan needs a **non-tracked** record (`0x7EDA0` in the far arm,
  `0x7EFEE` before the event build) with `lane <= 0x40 − timer81`,
  `lane <= bound`, `camY <= 0x50`, and the face fold `<= 0xAB`;
* the event build's seeds switch on camY: `camY == 0` takes the slot dirs
  (`<<6`, the measured claim pan `vel_x = 64/0xC = 5`), `camY ∈ [1,0x50]`
  takes the type-table kick direction (`<<5`);
* **the height argument is always 0**, and the admission caps camY at 0x50,
  so the resulting `event_param = max(0, camY) <= 0x50` — **the row-04
  half-line family can never satisfy the follow's `> 0xF0` gate**. Its role is
  the one-shot pan + the tracked rebind, exactly the phase-13 asserted
  behavior.

### 5.2 Engine staging points that gate the same native cells

`fifa96_match_row04_step`'s snapshot (`fifa96_match_handlers.c:2396-2505`)
stages three cells the native reads from elsewhere:

* `s.ball_height = mr->entities.ball.y` (line 2469, "FU-75 §1.6 stand-in,
  leg 15.b") — but the native cells at `0x7EDD8`, `0x7EFFA`, `0x7F003` are
  `dword[0x157750]`, the **camera-Y** cell the same slice's BF20 tail
  (`match_run.c:583`) and the kick ctx (`match_run.c:932`) already stage from
  `camera.pos_y`. Two engine cells feed one native cell across paths;
* `s.is_ball_track = id == controlled` (line 2448) and
  `s.ball_track_side` (line 2494) — the native cells at `0x7EDA0`, `0x7EE83`
  and `0x7EFEE` are `[0x1577CA]`, which the engine now maintains live at
  `mr->render.camera.tracked`;
* `s.team_7e7 = 0` / `s.is_team_7cb = 0` (lines 2460/2455) keep the
  corner/install pre-emptions from firing; `s.face_word7d = 0` (line 2432)
  zeroes the angle-fold target.

## 6. The coupling: asserted pan → sustained following

Live already (phase-13 T1, `test_live_claim_pan_row04_half_line`,
`tests/test_engine_match_frame.c:2041-2110`): the claim frame fires the
row-04 half-line event (camY == 0 → slot-dir seeds), binds `tracked == 9`,
and the walk pans `0 → 0x90` with the decay `5→3→2→1`, then stops with
`speed == 0`. That is the one-shot slide.

The sustained contract, from §3–§4:

1. the follow needs `event_param > 0xF0` (arm gate) **and**
   `pan_counter == 0` (call-site gate 1) **and** a tracked record with a
   slot, with `speed != 0`;
2. the only live producer of `> 0xF0` in open play is the receive arm's
   traj `0x100` (the k=7 mode-0x40 code-4 draw,
   `src/fifa96_loader/fifa96_ball_pairing.c:783`: `0x90 + (r&7)*(0x10 −
   desc15)`) or the record-event `FUN_00070C08` ramp/reflect;
3. the receive route needs the airborne height-table row, whose selector is
   `event->height = actor->pos_y != 0` **and** `d = [0x157750] − pos_y >=
   0x38` (`fifa96_action_handlers.c:113-117`) — the jump selector *and* a
   camera Y off zero; the row-04 pan events cannot substitute (height 0);
4. the follow window closes at the first pan step; only a new setter call
   (a new receive event, or the per-frame record-event chaser) reopens it.

The live ISO chains already prove the whole chain with two disclosed
stand-ins: `test_live_long_ball_mode40_band2_follow:1940` pins the camera Y
by hand, and `test_live_airborne_long_ball_seed_then_band2_follow:2254/2267`
stages `fifa96_camera_event_set(..., 0x74/0xE0, 0)` (the T1-L3 bootstrap) and
the jump via `fifa96_match_run_record_anim_install(rec, 0x5E, 0)` (the live
row selector stays FU-153-L1/R17). Both end at
`fifa96_match_run_camera_follow(&mr) == 1` with `vel = dirs × 15`;
`match_run.c:3492-3505` runs both from the granted frame, so the frame path is
wired.

## 7. T2 coupling recommendation (ranked)

1. **Unify the row-04 camY staging** — stage `s.ball_height` in
   `fifa96_match_row04_step` from `mr->render.camera.pos_y` (the same native
   `[0x157750]` the BF20 tail at `match_run.c:583` and the kick ctx at
   `match_run.c:932` already use), closing the FU-75 §1.6 `ball.y` stand-in
   for this path. Effect once a live Y exists: the half-line admission
   `camY <= 0x50` flips above 0x50 to the row-0x13 install arm and the event
   seeds switch to the type-table `<<5` — native behavior. Cheap, no new
   state.
2. **Feed the live tracked record into the row-04 snapshot** — `s.is_ball_track`
   and `s.ball_track_side` from `mr->render.camera.tracked` (falling back to
   the controlled stand-in when `TRACKED_NONE`), because `0x7EDA0`/`0x7EE83`/
   `0x7EFEE` all read `[0x1577CA]`. Effect: after a claim the followed record
   stops re-firing its own pan and another record can claim it — the native
   hop behavior.
3. **The actual sustained follow = R17's row-0x5E install + one open-play
   `[0x157750]` producer** — R19's evidence fixes the requirement set:
   a jumping kicker (`pos_y != 0`) with `camY − pos_y >= 0x38` → the
   height-table code-4 row → the mode-0x40 k=7 `0x100` traj → the receive arm
   sets `event_param 0x100` + tracked → the follow walks the camera on the
   tracked dirs × 15 each frame until the first pan step. The throw-in Y cut
   (phase-13) covers only restarts; the second candidate Y producer is the
   row-1E keeper-claim release (FU-157 §4.2, R5-L7) or driving a throw-in in
   the T2 smoke script.
4. **Rider: pin the follow window and the pan-counter lockout** in the live
   ISO chain — after the receive event, assert the frame-path follow sets
   `vel = dirs × 15` on the next granted frame and that `pos_x/z` advance by
   `vel × delta` while `pan_counter == 0`, then that the first pan step
   (`timer > timer_limit`) increments `pan_counter` and
   `fifa96_match_run_camera_follow` returns 0. Today the tests call the
   follow directly; the sustained *window* is unpinned.
5. **Frontier (leg): the record-event chaser** — `FUN_00070C08`'s per-frame
   re-seed is the native's continuous-follow mechanism; with a live jump
   (`rec+0x5D > 0x80`) and a live camY it fires per record per frame
   (`lane <= 0x10`). Land after 1–3; it is what makes following continuous
   rather than per-event.

**Smoke note (honest).** The phase-13 `p13-t1-*` smoke saw no whole-scene
translation; rec 1–2 change what the row-04 arm *reads*, not whether a live
height event exists. The smallest visible sustained pan in one live pass is
item 3's throw-in restart → kick, or item 4's assertion as the primary
evidence.

## 8. Discrepancies and errata candidates (quoted, first-hand)

1. **The same native cell, two engine cells.** Row-04 stages `ball.y` for
   `dword[0x157750]` (`handlers.c:2469`) while the BF20 tail
   (`match_run.c:583`) and the kick ctx (`match_run.c:932`) stage
   `camera.pos_y`. Native `[0x157750]` is the camera triple's Y (§2.1); the
   row-04 stand-in predates the live camera-Y producer (FU-75 §1.6). Fix per
   §7 rec 1.
2. **`is_ball_track` stand-in.** Documented at `handlers.c:2448`; the native
   cell is the same `[0x1577CA]` the camera tracked now models live. Not an
   erratum — the stand-in is honest — but it is the coupling seam §7 rec 2
   closes.
3. **FU-157 §5 step 3's phrasing.** "the taker's team jumps … → KICK" is
   precise per this slice: the height-table row's selector is the kicker's
   own `pos_y != 0` (`fifa96_action_handlers.c:114`), and the throw-in Y
   (`0x80`) only widens `d = [0x157750] − pos_y` past `0x38`. Both are
   required; the Y cut alone does not admit a grounded kick into the
   height-table row.
4. **FU-154 §9 / R2-L4's "follow is starved of a tall event"** is confirmed
   and sharpened: the row-04 arms are *structurally* incapable of a tall
   event (height arg 0 and a `camY <= 0x50` admission); the starved
   producers are the receive arm and the record-event tail only.
5. **`0x73A80`'s approach counter.** The follow's fail path increments
   `word[0x1577EE]` (the low word next to the `0x1577F0` height) by
   `word[0x1577C4]` toward the `[0x14C2F6]`-selected `0x70`/`0x90` threshold,
   then takes the interpolation arm; the engine models this as
   `event_cursor += step` (`camera.c:122-143`). Cell-pair confirmed
   (`0x1577EE` low word is *not* the height); the `word[0x1577C4]` = `step`
   equivalence stays carried from FU-152 §2.9.

## 9. Numbered legs

* **FU171-L1 — the frame-path sustained window / lockout assertion.** No live
   test pins the follow's frame-path window: `pan_counter == 0` opens it, the
   first pan step closes it, and a new event re-opens it. Next hop: extend
   `test_live_claim_pan_row04_half_line` or the band-2 follow chain with
   granted-frame assertions (`vel` per frame, `pan_counter`, the lockout).
* **FU171-L2 — the row-04 `[0x1577CA]` stand-in** (`is_ball_track`/
   `ball_track_side` from `controlled`): re-stage from `camera.tracked`
   (rec 2). Also the `face_word7d`/`vel_int` staging the same snapshot
   carries.
* **FU171-L3 — the row-04 `[0x157750]` stand-in** (`ball.y`): re-stage from
   `camera.pos_y` (rec 1), with the half-line branch assertions
   (`camY ∈ [1,0x50]` seeds, `camY > 0x50` row-0x13 install).
* **FU171-L4 — the record-event chaser live** (`FUN_00070C08` per-frame ramp;
   needs `rec+0x5D > 0x80` or `camY > 0xF0`). The continuous-follow frontier.
   Sub-question: the setter's ramp param at `0x70DD1` (the `0x70DBA ECX = 1`
   staging vs the engine's 0 — the `FUN_00092820` clobber question).
* **FU171-L5 — the three register-staged setter callers** (`0x77423`,
   `0x77DC6`, `0x71B8A` pass `MOVSX EBX,SI`): the SI source per caller
   (the `0x77xxx` set-piece rows and the `FUN_0007131C` reposition) — their
   rows are unported (FU-156/FU-157 dispositions).
* **FU171-L6 — the row-05 claim arm's setter** (`0x7F4E7`, height 0,
   `ECX = (camY > 0x10)`): the candidate second claim-frame pan/tracked
   rebind (FU-156 OL-63's row-05 stage-0 tail); its `ECX` semantics (the
   `FUN_00070544` fast-path param) and whether it and the row-04 arm double-
   fire at the claim.

## 10. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_xrefs_to` `0x1577CA` (88 fresh; writes 0x71D27/0x70258/0x70054),
`0x1577F0` (31 fresh), `0x1577F2` (14 fresh), `0x71C94` (11 fresh),
`0x157A6C` (6 fresh; writers 0x6E5F7/0x6E602/0x73E7B/0x7741A), `0x736AC`
(2 fresh: 0x4B193/0x743EF); `search_instructions` `[0x00157750]` (84),
`15873e` (17); `disassemble_bytes` `0x71C94..0x71D70` (54 insns),
`0x71DF4..0x71F30` (81), `0x736AC 0x73760..0x737E0` (26), `0x739B0..0x73B80`
(107), `0x70A30..0x70AA0` (28), `0x7A1E0..0x7A240` (28), `0x7C7C0..0x7C910`
(89), `0x7ED60..0x7F100` (263), `0x7F4B0..0x7F500` (22), `0x82A30..0x82A80`
(22), `0x6FA30..0x6FA80` (23), `0x75D90..0x75DD0` (16), `0x773C0..0x77428`
(25), `0x77D60..0x77DC6` (29), `0x71B20..0x71B8A` (33);
`get_function_by_address` `0x7456E`/`0x755A9`/`0x76DB5`/`0x7751B` (no
function — jump-table band).

Engine source read: `src/fifa96_loader/fifa96_camera.c` (whole),
`src/fifa96_loader/fifa96_outfield.c` (600-718),
`src/fifa96_loader/fifa96_ball_pairing.c` (262-330, 755-824),
`src/fifa96_loader/fifa96_action_handlers.c` (60-133),
`src/fifa96_engine/fifa96_match_run.c` (556-619, 3100-3260, 3441-3570),
`src/fifa96_engine/fifa96_match_handlers.c` (815-875, 1160-1192, 2396-2560),
`src/fifa96_engine/fifa96_match_entities.c` (402-464),
`tests/test_engine_match_frame.c` (1882-1970, 2041-2110, 2199-2378, 4762-4822),
`docs/ghidra/FU153`, `FU154`, `FU157`, `docs/ENGINE.md` (426).

Carried docs: FU-148 §2.1(c)/§6.2/§13.5/§14.5, FU-152 §2.8/§2.9, FU-153
§4-5/§10-11, FU-154 §1-5/§7/§9, FU-156 §3, FU-157 §1-5/§7, FU-75 §1.6,
FU-96 §7.

Status: recon complete; **no repo file changed except this slice**; no
census change (22/80 rows; README title unchanged); no golden touched.

---

## T2 status (2026-10-12, phase-14 T2 — append-only)

The phase-14 T2 slice landed the live sustained follow from this slice's recs.
Fresh Ghidra reads this session (program `/FIFA96.EXE`, scripts disabled)
re-verified §1/§3 first-hand before any code change:

1. **The first arm** (`disassemble_bytes 0x71DF4..0x71F30`, 81 insns;
   fresh): `0x71DF8 EDX=[0x1577CA]` / `0x71E03 JZ` bail; `0x71E0B EAX=EDX;
   0x71E0D EDX=[EDX+4]; 0x71E10 CMP dword[EDX],0x18D8; 0x71E16 JNZ 0x71EF0`;
   `0x71E1C CMP dword[EAX+0x20],0; JZ`; `0x71E26 EAX=[0x1577EE]; SAR 16;
   CMP EAX,0xF0; JLE`; `0x71E4E [0x1577C0] = sext(byte[slot+0x20]) * 0xF`,
   `0x71E69 [0x1577C2] = sext(byte[slot+0x21]) * 0xF`; the clamps read the
   **alias words** `0x71E77 dword[0x1577BE]>>16` = `word[0x1577C0]` and
   `0x71EA3 dword[0x1577C0]>>16` = `word[0x1577C2]` (matching §3.1's alias
   note); `0x71EE5 word[0x1577BE] = distance(vel)` (CALL `0x8DC68`). The
   engine port (`fifa96_camera.c:525-533` = `clamp(dirs*15, +-15)` + speed;
   `match_run.c:3128-3147` = the call-site gates) is byte-exact.
2. **The call-site gates** (`disassemble_bytes 0x739C0..0x73B6F`, 97 insns;
   fresh): `0x739BB DL=byte[0x157821]`; `0x739C6 TEST DL,DL` (gate 1);
   `0x739CE CMP word[0x1577BE],0` (gate 2); `0x739DC EAX=[0x1577EE]>>16 > 0x10`
   (gate 3); `0x739ED/0x739F6` the two rate bytes (`0x73B5B` follow when both
   zero, else the `0x73A03` interpolation arm); `0x73B5B..0x73B69` the
   tracked/has-slot pair; `0x73B6B CALL 0x71DF4`. The engine's `follow_speed`
   staging is confirmed by the walk tail (`0x73CA7 EDX=[0x1577C0];
   0x73CAD EAX=[0x1577BE]; SAR 16` -> `0x73CB8 word[0x1577BE] = AX` = the
   after-gate recompute) — the gate reads the pre-tail value the engine
   captures as `follow_speed` (`camera.c:83/147`).
3. **The window/lockout mechanism** (`disassemble_bytes 0x70A30..0x70A9F`):
   `0x70A3F EAX=[0x15781E]>>24 = byte[0x157821]; 0x70A47 CMP 0x64; JGE`
   (cap) and `0x70A65 INC byte[0x157821]`; the setter's clear is
   `0x70153 MOV byte[0x157821],AH` with `AH=0` (`0x70144 XOR AH,AH`) inside
   `FUN_000700F4`. Both already ported (`camera.c:892-893`, `camera.c:863`).
4. **The row-04 half-line natives** (recs 1-2 consumers; fresh):
   `0x7EDD8 CMP dword[0x157750],0x50; JLE` (the admission is the **camera
   Y** dword, not the ball record); `0x7EE83 ECX=[0x1577CA]; TEST; JZ;
   CMP EBP,ECX; JZ;` then `0x7EE95 EDX=[ECX]` (the tracked record's owner)
   and `0x7EE9D CMP AL,[EDX+0x826]` (the tracked owner's side byte) — the
   `ball_track_side` source is the tracked record, not the controlled actor.

**What landed.**

* **(c) rec 1 + rec 2** (`fifa96_match_handlers.c`, row-04 staging):
  `s.ball_height = (int16_t)mr->render.camera.pos_y` (the FU-75 §1.6
  `ball.y` stand-in retired; **FU171-L3 closed**) and
  `s.is_ball_track`/`s.ball_track_side` from `camera.tracked` with the
  controlled fallback only on `TRACKED_NONE` (**FU171-L2 closed**).
  Discriminators (RED first, mutation-RED re-verified):
  `test_row04_live_pan_arms_camera` now stages the camera cell with `ball.y`
  zero (fails pre-re-stage), the new
  `test_row04_tracked_gate_suppresses_half_line` (the far-arm/event-build
  tracked gate; the near-arm coda is *not* tracked-gated and still installs
  5 — pinned) and the new handler
  `test_action_04_ball_track_side_from_tracked` (the RNG draw witness for
  the side compare). Live ripple: the long-ball claim chain now claims at
  `camY == 0` (the native admission) — the pre-re-stage chain ran the claim
  at `camY >= 0x70` only because the stand-in read `ball.y == 0` — and
  `test_view_pose_feed_arms_camera`'s corner/id expectation moved to the
  native claimant's side (sit 3/id 9).
* **(b) FU171-L1 closed** (`tests/test_engine_match_frame.c`,
  `test_live_long_ball_mode40_band2_follow` extended): from the receive
  event the granted-frame path runs the sustained window — per frame the
  follow re-seeds `vel = dirs x 15` and `pos_x` advances by `vel*delta`
  while `pan_counter == 0`; the window is **52 granted frames** for the k=7
  `0x100` event at the pinned `0x70` camera Y (F2 = 59, F4 = 118,
  F6 = 59 - ramp(0x90) = 14 -> (118 - 14)/2, matching §3.3's ~51 at
  camY 0x80), the first pan step increments `pan_counter` and
  `fifa96_match_run_camera_follow` returns 0 across the lockout, and a new
  setter call (+ the caller's `0x71D27` bind) re-opens it. Mutation-RED:
  disabling `0x70A65` breaks the window (the decay frame's velocity assert).
* **Goldens/census**: M1 `09b726b7…` / M2 `5f7e7692…` byte-identical (no
  re-pin — the tape does not run the row-04 claim); `make check` 109/109;
  census 24/80 unchanged.

**Legs after T2.**

* **FU168-L5 / FU171-L4 stand (the camera-Y producer).** The sustained route
  is now proven from the event live; the remaining gap is a *naturally
  arising* event in open play. Verdict: not cheap — the row-1E keeper-claim
  release is dispatch-dead (R11-L4: rows 1A/1B unported, the stage-5 anim
  bridge R11-L1), the throw-in stage-3 cut is restart-driven and transplanting
  it re-anchors the whole chain on the restart camera (the kick chain's
  context collapses; the cut itself stays handler-proven by
  `test_live_throw_in_camera_y_cut`), and the camera-height arc needs an
  existing event (`event_param != 0`) before it can raise `[0x157750]`. The
  kick frame's camera Y therefore stays the pinned stand-in in the chain
  (disclosed at the (4d) comment).
* **FU171-L5/L6 unchanged** (the three register-staged setter callers; the
  row-05 claim setter).
* **FU171-L7 (new) — the remaining `[0x157750]` stand-in sites.** The
  same-cell re-stage landed for row-04 only (the brief's scope). The other
  engine sites still stage `mr->entities.ball.y` for the native camera-Y
  cell: row-05 (`handlers.c` `c.ball_height`), row-06, row-08 and the record
  machine's chase gate (`match_run.c` `s.chase.camera`). Each must be
  re-staged from `camera.pos_y` with its own live-chain assertions (row-05's
  `0x7F4B0`/`0x7F4D0` reads, row-08's `0x81157`, the chase gate's
  `0x7CCD8 < 0x30`) and the tape checked for movement.

## T3 status (2026-10-12, phase-14 T3 — append-only)

Two review minors folded this round; fresh Ghidra reads (program
`/FIFA96.EXE`, scripts disabled), no repo file changed except this append:

1. **Erratum: the T2 status's call-site window.** The T2 status item 2
   quotes `disassemble_bytes 0x739C0..0x73B6F` (97 insns) while its own
   first cite is `0x739BB DL=byte[0x157821]` — a site **outside** that
   window. The covering window is the method record's `0x739B0..0x73B80`
   (107 insns); fresh `disassemble_bytes 0x739B0..0x739C7` this round
   confirms the boundary: `0x739B0 AND EAX,0x1` / `0x739B3 AND EDX,0x3` /
   `0x739B6 SHL EAX,0x2` / `0x739B9 ADD EAX,EDX` / `0x739BB MOV
   DL,[0x157821]` / `0x739C1 MOV [0x157814],AL` / `0x739C6 TEST DL,DL`
   (gate 1). The §3.2 engine mapping is unaffected (the four gates are
   implemented); only the quoted window was mis-stated.
2. **The pan step's second admission `0x737C8` cited.** Fresh
   `disassemble_bytes 0x737A1..0x737D2`: `0x737A1 TEST CX,CX` /
   `0x737A4 JZ 0x737B9` (the arc-Y store arm — the `word[0x1577F8]`
   `0x70B94`-family producer follows), `0x737B9 MOV AX,[0x1577FA]` /
   `0x737BF CMP AX,[0x1577F4]` / `0x737C6 JG 0x737DA` (timer > limit ->
   the pan step), and the second admission `0x737C8 CMP dword[0x15775C],0`
   (`83 3d 5c 77 15 00 00`) / `0x737CF JLE 0x737DF` + `0x737D1 CMP
   dword[0x157750],0` / `JG` — the descent form `prevY > 0 && camY <= 0 &&
   timer <= limit`, exactly as §2.3 derived. The engine carries only the
   `timer > limit` arm (`fifa96_camera_update_walk`); the descent arm stays
   a numbered leg of the live producer set (**FU171-L4/L7** family — the
   camera Y producer gap the phase-14 T3 status in ENGINE.md records; the
   FU-170 restart wire now supplies the goal path, the open-play Y producer
   stays open).
