# W26-1 / FU-264 — the open-play camera-Y producer (the visible anchor)

Draft for phase-26 recon wave W26, task 1 (FU-264). Promotes to
`docs/ghidra/FU264_camera_y_producer.md`.

Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; explicit case-sensitive
program argument; scripts disabled; standard tools only). Every address/byte
claim below is a **fresh** `disassemble_function` / `disassemble_bytes` /
`read_memory` / `get_xrefs_to` / `search_instructions` of this session; engine
source cites are read-only reads. LE-image globals are flat addresses
(the `+0x100000` rule applies only when mapping a file offset to a flat
address). Alias traps applied: `dword[x] >> 16` = the word at `x+2` (used for
the `0x1577F0` height and `rec+0x69` lane reads below). No repo file other than
this draft was modified; no git command; `/FIFA96.EXE` untouched; no rename /
comment / label / create_function / script / project save.

**Question (brief, FU168-L5 / R2-L1):** derive what raises `[0x157750]` (the
camera-Y / ball-height cell) off zero on a live open-play path, and whether the
follow-cam is smoke-visible (the Layer-2 anchor).

**Result in one line:** the cell's only **open-play** per-frame writer is the
arc `0x737B4` (`FUN_00070B94` via `FUN_000736AC`), ported as
`fifa96_camera_height` — but it runs **only when a camera event already has a
nonzero height** (`0x737A1 TEST CX,CX; JZ 0x737B9`); every other writer is a
reset, a restart/set-piece cut, or the out-of-bounds reaction. No live
open-play producer supplies the event that carries the follow's height gate —
the `event_param > 0xF0` gate is on `word[0x1577F0]` (the **event height**,
`0x71E26`, not the cell `[0x157750]`), and no live producer also drives the cell
into the row-0E mid-band `0x20 < Y < 0x70` — so **the follow-cam stays
granted-frame-only (fixture-staged event), not smoke-visible**; the ruling is
**LIVE LEG** (FU168-L5 / FU171-L4 / FU-157 T1-L3), with the keeper release the
natural producer but **dispatch-dead** (R11-L4).

---

## Step 1 — the cell `[0x157750]`: fresh writer census and the ported producer

### 1.1 The cell model (re-confirmed first-hand)

`[0x157750]` is the shared **camera-Y / ball-height proxy** (the FU-71 triple
is `0x15774C = x`, `0x157750 = y`, `0x157754 = z`; carried FU-71/FU-97/FU-157
§1). The engine maps it to `fifa96_camera.pos_y`
(`src/fifa96_loader/fifa96_camera.h`). It is the same `dword` the kick band
(`d = ball_height - height`), the BF20 record-event tail gate, the row-04
half-line admission, the row-0E pan window and the height-table gate read.

### 1.2 Fresh writer census

`get_xrefs_to 0x157750` (fresh, this session) = **146 refs** (100 + 46 paged).
Ghidra classifies **22 WRITE** refs. `search_instructions operand "157750"`
(fresh) = **84 sites** — the same count FU-171 §2.1 reports. The two counts
reconcile exactly: the operand search sees the **10 explicit `MOV [0x157750],r`
stores** FU-171 listed; `get_xrefs_to` additionally sees the **12 triple-copy
`MOVSD` stores** (a `MOVSD x3` writing `0x15774C/50/54` records a WRITE ref at
the middle copy), plus base-address computed refs. The 22 writers:

| # | site | native body | engine mapping | class |
|---|---|---|---|---|
| 1 | `0x737B4` | `FUN_000736AC` arc (`MOV [0x157750],EAX`) | `fifa96_camera_height` (camera.c:91,709) | **ported, live iff event** |
| 2 | `0x7014F` | `FUN_000700F4` place reset (Y = arg) | `fifa96_camera_init` | ported, live (kickoff/restart/setter) |
| 3 | `0x7130E` | `FUN_00070DE0` out-of-bounds reaction (tail triple) | `fifa96_camera_reposition` (camera.c:930) | ported, reaction-only |
| 4 | `0x85919` | throw-in stage-3 cut (`[0x157750] := 0x80`) | `fifa96_match_action_10` (handlers.c:1706) | ported, restart-driven |
| 5 | `0x85917` | same block, the rec-pos `MOVSD x3` middle copy | handlers.c:1706 | ported, restart-driven |
| 6 | `0x85836` | throw-in stage-2 focus := camera target | handlers.c:1668-1670 | ported, restart-driven |
| 7 | `0x8602C` | row ~0x10 stage-5 focus := camera target | handlers.c:1819-1821 | ported, restart-driven |
| 8 | `0x840CB` | 0x840xx band focus := camera target | handlers.c:1969-1971 | ported, restart-driven |
| 9 | `0x7456E` | band cut: `[rec+0x5D]+0x38` → Y, place flag 1 | — | **unported band** |
| 10-21 | `0x755A9`, `0x7568B`, `0x75A01`, `0x75A0F`, `0x75D53`, `0x75D69`, `0x75EA7`, `0x75EEA`, `0x75F67`, `0x75F86`, `0x76DB5`, `0x7751B` | 0x74xxx..0x77xxx sequence / set-piece / keeper band (jump-table code, no analyzed function; `get_function_by_address` = "No function") | keeper cut `0x75D53/0x75D69` ported (keeper.c / handlers.c:521-530), rest unported | mixed |
| 22 | `0x83E7D` | 0x83xxx set-piece cut (`MOVSD x3` from `0x158897`) | — | **unported band** |

First-hand disassembly of the load-bearing writers:

```
; the arc store (FUN_000736AC), the only OPEN-PLAY per-frame writer
0x737A1  TEST CX,CX                    ; CX = word[0x1577F0] (the event height)
0x737A4  JZ 0x737B9                    ; height == 0 -> NO Y store
0x737A6  MOV EAX,[0x1577F8]
0x737AB  SAR EAX,0x10                  ; = word[0x1577FA] (the timer)
0x737AE  CALL 0x70B94                  ; the arc (FUN_00070B94)
0x737B3  CWDE
0x737B4  MOV [0x157750],EAX            ; camY := the arc  <-- the producer
```

```
; the keeper stage-5 release cut: camY := 0x50 (mid-band!)
0x75D4A  MOV ECX,0x50
0x75D52  MOVSD ES:EDI,ESI  x3          ; [0x15774C/50/54] <- rec.pos
0x75D69  MOV [0x157750],ECX            ; camY := 0x50
```

```
; the sequence-band record-arc cut
0x74558  MOV EAX,[EDX+0x5D]
0x7455C  ADD EAX,0x38
0x7456E  MOV [0x157750],EAX            ; camY := rec+0x5D + 0x38
0x74573  CALL 0x700F4
```

```
; the throw-in stage-3 cut (the ported, restart-driven 0x80)
0x85911  MOV EAX,0x80
0x85916  MOVSD ES:EDI,ESI  x3          ; [0x15774C/50/54] <- rec.pos
0x85919  MOV [0x157750],EAX            ; camY := 0x80
0x8596B  CALL 0x700F4                  ; Y preserved
```

### 1.3 The ported producer `FUN_00070B94` + the integrator store

`disassemble_function 0x70B94` (fresh, 40 insns) — the arc:

```
0x70B9A  MOV BX,word[0x1577F4]         ; the timer limit
0x70BA3  CMP DX,BX ; JGE 0x70C01       ; timer >= limit -> 0
0x70BA8  CMP word[0x1577F0],0 ; JLE 0x70C01   ; height <= 0 -> 0
0x70BB2  MOV EBX,0x94
0x70BC3  MOV SI,word[0x1577F2]         ; F2 = the ramp peak
0x70BCC  SUB EBX,ESI                   ; EBX = 0x94 - F2
0x70BDD  ...  index the 149-entry table [0x157744] (0x10F3C4):
0x70BF3  MOV AX,word[EDX+EAX] ; 0x70BF7 SUB AX,word[EBX] ; clamp >= 0
```

`[0x157744] = 0x10F3C4` is the 149-entry 16-bit ramp table. It is **not a static
image constant**: the cell is **runtime-populated** by `FUN_0006FFC0`, which
first-handed this session loads `EDX = 0x10F3C4` (`0x6FFC6`) and stores it at
`0x6FFE9` (`MOV dword[0x157744],EDX`); fresh `get_xrefs_to 0x157744` = 16 refs
with that one WRITE (`0x6FFE9`) and the READs in `FUN_00070B94` (`0x70BEC`) and
`FUN_0008D098` (FU-148 §15.1). The
engine ports this exactly as `fifa96_camera_height` (`fifa96_camera.c:709`,
table `camera_height_arc[149]` at `:651`) and the store order as
`fifa96_camera_update_walk` (`camera.c:72`): the height store at `camera.c:91`
sits **after** the timer accumulation and **before** the pan-step check,
matching `0x73783..0x737B4`.

**Live vs fixture-only (Step 1 ruling).** The producer is **ported and wired
into the live frame path** (`match_run.c:4147` → `fifa96_camera_update_walk`),
but its **only** open-play trigger is `event_param != 0` — i.e. it is a
*consumer of an existing camera event*, not a producer of one. With no live
event, `camY` stays 0:
- the row-04 half-line/claim family passes **height arg 0** and is admitted only
  while `camY <= 0x50` (`0x7EDD8`), so its event height is `max(0, camY) = 0`
  and the arc is skipped (`0x737A1 JZ`); the measured live claim pan leaves
  `event_param = 0` (FU-157 §3.4 / FU171 §6);
- the throw-in cut sets `camY = 0x80` but does **not** call the event setter
  (`0x85904..0x8596B` calls only `FUN_000700F4`), so `event_param` stays 0 and
  the arc does not run;
- the fixture-only writers are the **tests** that pin `camera.pos_y` by hand
  (`test_live_long_ball_mode40_band2_follow` pins the kick-frame `camY = 0x70`;
  `test_live_airborne_long_ball_seed_then_band2_follow` stages
  `fifa96_camera_event_set(..., 0x74/0xE0, 0)`) — these are **fixture-only**,
  not a live producer.

So on the live frame path the cell is written by the arc **only**; the arc is
**starved** in open play.

---

## Step 2 — the open-play gap (FU168-L5)

### 2.1 Why no live path raises the cell

The row-0E pan arm (`FUN_0008xxxx` arm index 2, already wired) is the consumer
that a live `camY` would unlock; its gate is fresh-quoted:

```
0x82806  CMP EBP,dword[0x1577CA]        ; rec == the tracked record?
0x8280C  JZ 0x82AA0                     ; yes -> no pan
0x82812  CMP word[EBP+0x85],0           ; jump_base (the BF20 arc peak)
0x8281A  JNZ 0x82AA0                    ; arc live -> no pan
0x82820  MOV ESI,dword[0x157750]        ; camY (dword read)
0x82828  CMP ESI,0x70 ; JGE 0x82A9C      ; camY >= 0x70 -> skip
0x82831  CMP ESI,0x20 ; JLE 0x82A9C      ; camY <= 0x20 -> skip
0x8283A  MOV EAX,dword[EBP+0x69]
0x8283D  SAR EAX,0x10                    ; = word[rec+0x6B] (the lane)
0x82840  CMP EAX,0x60 ; JGE 0x82A9C      ; lane >= 0x60 -> skip
```

The pan window is strict `0x20 < camY < 0x70`. Live `camY` is 0 outside a
height event, so the window fails and the record stays at stage 2 every frame
(the `JZ return` retry). The same starvation caps every other consumer:
- the row-04 half-line event build admits only `camY <= 0x50` (`0x7EDD8
  CMP dword[0x157750],0x50; JLE 0x7EE4B`) and passes height arg 0, so its
  `event_param` is `max(0, camY) <= 0x50` and it can **never** wake the follow
  (`event_param > 0xF0`, the `word[0x1577F0]` gate at `0x71E26`);
- the follow gate chain is `pan_counter == 0` (`0x739C6`) and
  `event_param > 0x10` (`0x739DC`) at the call site, and `height > 0xF0` at
  `FUN_00071DF4` `0x71E26`; the ported gates are
  `fifa96_match_run_camera_follow` (`match_run.c:3747-3749`) and
  `fifa96_camera_rate_event` (`camera.c:528`, `if (height <= 0xF0) return 0`).
  With the arc starving, none is satisfied.

**The keeper release is the natural mid-band producer — and it is
dispatch-dead.** The keeper stage-5 release cut writes `camY := 0x50`
(`0x75D53/0x75D69`), which is **inside** the row-0E window (`0x20 < 0x50 <
0x70`) and satisfies the row-04 `camY <= 0x50` admission. The engine ports
exactly this as the row-1E stage-5 release (`handlers.c:521-530`:
`mr->render.camera.pos_y = s->cam_y; ... fifa96_camera_event_set(..., 0x50, 0)`),
but the row-1E claim family is **dispatch-dead in live play** — the situation-5
queue (id 7) never reaches phase 9, rows 1A/1B are the missing sit-5 producers
(carried **R11-L4**, FU-163 §7; re-stated FU-168 T2 status §1 and FU-171 T2
status legs). A dispatch that never installs row 0x1E never runs the release,
so the cut never fires live.

The two other candidate producers (carried FU-157 §5 / FU168 T2 status):
- **the throw-in stage-3 cut** (`0x85919` = `0x80`) is **restart-driven** and,
  as §1.3 shows, does **not** set an event; transplanting it onto the open-play
  kick chain re-anchors that chain on the restart camera (T2 attempt collapsed
  the chain — FU-168 T2 status §1);
- **the `0x10F3C4` arc** needs an **existing** event (`FUN_00070B94` returns 0
  when the event height/limit are 0) — the bootstrap deadlock: to raise `camY`
  you need an event, to have an event you need `camY` off zero.

### 2.2 What a live open-play event must do (the height-table `d >= 0x38` band)

The height-table (airborne) kick row is the one live route to a `0xF0`-plus
event. Its selector is the port of native **`FUN_0007AE70`**
(`0x7AE79..0x7B018`; engine `fifa96_action_kick_event_row`,
`fifa96_action_handlers.c:87`; header `:52-58`/`:77`):

```
; src/fifa96_loader/fifa96_action_handlers.c:114-118
d = (int16_t)(event->ball_height - (int16_t)event->height);
if (event->height != 0) {            /* the actor's height dword is nonzero */
    if (d < 0x38) return FIFA96_OK;
    out->table = FIFA96_ACTION_KICK_EVENT_TABLE_HEIGHT;
    out->index = 2 * class_id + idx;
}
```

with `event->ball_height = word[0x157750]` (the live camera Y;
`handlers.c:934 s.ball_height = mr->render.camera.pos_y`) and
`event->height = dword actor[+0x5D]` (the actor's own vertical). The native `d`
fold is fresh-quoted at `FUN_0007AE70`:

```
0x7AEFD  MOV DX,word[0x157750]          ; ball_height
0x7AF04  MOV CX,word[ESI+0x5D]          ; height (low word)
0x7AF08  MOV EBX,dword[ESI+0x5D]        ; the height dword (the zero test)
0x7AF0B  SUB EDX,ECX                    ; d = camY - actor[+0x5D]   <-- the fold
0x7AF0D  TEST EBX,EBX ; JZ 0x7AF3F      ; height == 0 -> carry/active/idle
0x7AF11  MOVSX EDX,DX
0x7AF14  CMP EDX,0x38 ; JGE 0x7AF20     ; d >= 0x38 -> the HEIGHT table
0x7AF19  XOR EAX,EAX ; JMP 0x7B18E      ; d < 0x38 -> the NULL return
0x7AF35  MOV EBX,0x1102FE               ; = FIFA96_ACTION_KICK_EVENT_TABLE_HEIGHT
```

so the **height-table band is
`d = word[0x157750] - word[actor+0x5D] >= 0x38`**:
1. the kicker's height dword `actor[+0x5D]` must be **nonzero** (`0x7AF0F`), and
2. `camY` must exceed the actor's vertical by `>= 0x38` — i.e. an
   **open-play Y source already above the actor** is required *before* the kick.

**Erratum (fix round 1).** A first-draft version of this section cited two
`CMP dword[0x157750],0x38` sites (`0x8061B`, `0x81157`) as "the native height
gate". Those sites are a **different** camY-vs-`0x38` gate in the `0x80xxx`
kick-target band; they read the cell **directly** and do **no** actor-vertical
subtraction. The engine's `d = ball_height - height` maps to `FUN_0007AE70`
(`0x7AF0B`/`0x7AF14`), fresh-quoted above — the fold is **native and
first-hand**, not carried.

The live jump row selector that makes a record airborne is itself the R17/phase
leg (`fifa96_match_run_record_anim_install(rec, 0x5E, 0)` is what the
airborne ISO test stages; FU-171 §6 item 3). So the open-play event needs
**both** a live jump selector **and** a live `camY` off zero — the same
bootstrap the follow needs. Nothing live supplies either from a cold start.

---

## Step 3 — the view-object handler (R2-L1)

### 3.1 Fresh first-hand (native)

The R2-L1 handler is `FUN_0004E834` (the live default view-mode handler).
`get_xrefs_to 0x4E834` (fresh) = **3**: `0x4D940` (`FUN_0004D908`,
`UNCONDITIONAL_CALL`), `0x51056` (`COMPUTED_CALL` inside `FUN_000505D0`) and the
handler-table DATA ref `0x108B80`. `disassemble_function 0x4E834` (fresh, 364
insns) gives the full write set — the **view object** `ESI = [0x107DC8]`
(= the `0x107508` camera-0 object):

| site | write | field |
|---|---|---|
| `0x4E85F`/`0x4E869` + clamps | `[EBP]`,`[EBP+4]` (EBP = EDI+0x34) | behaviour yaw accumulator + delta |
| `0x4E8AA`/`0x4E8B2`/`0x4E8BC`/`0x4E8C8` | `[EDI+0x1C]`,`[EDI+0x18]` | behaviour roll accumulator |
| `0x4E92D` | `MOV [ESI+0x18],EAX` | view **z** |
| `0x4E984` | `MOV [ESI+0x10],EAX` | view **x** |
| `0x4E991`/`0x4E99F` | `MOV [ESI+0x60],EAX`; `[ESI+0x58] = (+0x58 + +0x60) & 0xFFFF` | view **yaw** + increment |
| `0x4EA98` | `MOV [ESI+0x58],EAX` | view yaw snap (class 3/4) |
| `0x4EAB0`/`0x4EACA` | `MOV [0x14E4D8]` / `MOV [0x14E4D4]` | the two horizon words |
| `0x4EBD6`/`0x4EBE4` | `MOV [ESI+0x64],EAX`; `[ESI+0x5C] = (+0x5C + +0x64) & 0xFFFF` | view **pitch** + increment |
| `0x4EC5A` | `ADD [ESI+0x14],EAX` | view **y** (smoothed) |
| `0x4EC87` | `ADD [ESI+0x4C],EAX` | view **angle** (smoothed) |
| `0x4EC90-92` | `MOVSD x3: [ESI+0x40..0x48] <- [ESI+0x34..0x3C]` | the previous-anchor triple |

The handler **never reads or writes the FU-71 triple** `0x15774C/50/54` — the
fresh 364-instruction body contains no reference to that address. It is a
separate camera **rig**: its `+0x10/+0x14/+0x18` are its own smoothed position,
`+0x4C` the angle, `+0x58/+0x5C` yaw/pitch. The world eye it feeds is the FU-71
triple staged by `FUN_00036C70` (`[0x155AA4] <- [0x15774C]`), so **R2-L1
supplies orientation + ratio only, never the eye/place** (carried FU-157 §2.3).

### 3.2 Live vs unported (Step 3 ruling)

**Partially ported, unported live.** The engine has
`fifa96_camera_behavior_steady` (`fifa96_camera.c:403-416`) — the
yaw/pitch clamp + horizon-jitter subset of `0x4E834` — reached only through
`camera_pose_handler_arm` (`camera.c:565`) case 0. But the fresh-match default
is **view_mode 0**, which takes `fifa96_camera_pose_feed`'s `default` arm →
`camera_pose_handler_arm`, and the call site itself documents the outcome:

```
; src/fifa96_engine/fifa96_match_run.c:4166-4168
* native 0x15774C event target per the FU-71 port mapping). view_mode 0
* (the fresh-match default) is the unported handler arm: a no-op, so the
* static tape is unaffected.
```

`get_xrefs_to 0x4E834` = 3 confirms only the two native call sites plus the
table; there is no engine caller of the full handler. The integrators the
comment at `camera.c:407-410` names — the class-4/else fold, the roll
`FUN_0004DF34` (`0x4E989`), the class-3 snap `FUN_0004C7D0`
(`0x4E9D9/0x4EA4F/0x4EA7C`), and the `+0x18`/`+0x10` lerps — stay **leg 9**.
So R2-L1 is **unported live** (the clamps are a partial port; the framing
integrators are not run in match play). Its role is the **framing** (yaw/pitch/
zoom) that would keep the ball on canvas after a roll — it **cannot move the
camera place**, so it is **not** a camera-Y producer and not required for
translation.

---

## Step 4 — the port shape + the Layer-2 verdict

### 4.1 The port shape

The producer's port shape is **not** a new function: it is the already-ported
arc (`fifa96_camera_height`) driven by a live **event**. The minimum honest
producer is therefore one restored native **Y source** feeding
`fifa96_camera_event_set` with a height above the consumer gate, plus the
already-wired receive→tracked→follow chain (FU-157 §5 ranked set):

1. a live `camY`-off-zero source: the keeper stage-5 release cut
   (`camY := 0x50`, dispatch-dead → needs rows 1A/1B, R11-L4) **or** the
   throw-in stage-3 cut (`camY := 0x80`, restart-driven, already ported at
   `handlers.c:1706`);
2. with `camY` off zero, the airborne height-table row
   (`d = word[0x157750] - word[actor+0x5D] >= 0x38`, §2.2) and a live jump
   selector;
3. the mode-0x40 receive event (`traj 0x100`) →
   `fifa96_camera_rate_event` (`vel = tracked dirs × 15`) →
   `fifa96_match_run_camera_follow` (both wired, fixture-proven).

The arc store itself (`camera.c:91`) and the follow (`match_run.c:3741`) need
**no change**; the missing piece is step 1's live trigger.

### 4.2 Live-vs-leg ruling

**LIVE LEG — FU168-L5 stands.** No live open-play path raises `[0x157750]`:
- the only per-frame open-play writer (the arc) is starved of an event
  (`0x737A1 JZ`);
- the row-04 event is structurally incapable of a tall event (height arg 0 and a
  `camY <= 0x50` admission);
- the natural mid-band producer (the keeper release, `camY := 0x50`) is
  **dispatch-dead** (R11-L4);
- the throw-in cut is restart-driven and does not set an event;
- the height-table route needs `camY` off zero *before* the kick (bootstrap
  deadlock).

The producer port (`fifa96_camera_height`) is **not a fake** — it is a faithful
port of `FUN_00070B94` and it runs live whenever an event exists — but the
**live trigger is the open leg**, so any claim that the follow-cam is live in
open play without a fixture-staged event is a **fixture-only stand-in**.

### 4.3 Layer-2 verdict

**The follow-cam is granted-frame-only, not smoke-visible. This verdict is
CARRIED** from the prior smoke records (FU-148 §15.3 `p11-t1-*`; FU-157 §9
`p13-t1-*`; FU-171 smoke note) — **no fresh SDL smoke was run in this recon
slice** (no ISO-gated gameplay run). Those records all report honest-no-follow,
and the follow only runs in the ISO chains from a
**pinned** `camera.pos_y` / a staged `camera_event_set`
(`test_live_long_ball_mode40_band2_follow`,
`test_live_airborne_long_ball_seed_then_band2_follow`,
`test_live_claim_pan_row04_half_line`). The static stand-in camera (the kickoff
`(0,0,0)` seed, FU-157 §1) is what the window shows. **Layer-2 anchor = NOT
reached; the follow-cam is granted-frame-only (carried, not re-measured).**

---

## Numbered open legs (FU264-L*)

* **FU264-L1 — the live open-play `[0x157750]` producer (FU168-L5 / FU171-L4).**
  No wired live open-play writer raises the cell into the row-0E window
  (`0x20 < Y < 0x70`), and none supplies the event height the follow's
  `event_param > 0xF0` gate needs (that gate is on `word[0x1577F0]`,
  `0x71E26`). Native owners: the
  keeper stage-5 release cut (`0x75D53/0x75D69`, `camY := 0x50`) — **dead**
  behind rows 1A/1B (**R11-L4**, FU-163 §7); the throw-in stage-3 cut (`0x85919`)
  — restart-driven and not an event setter. Next hop: port the row-1A/1B sit-5
  producers, or drive a throw-in in one live pass (FU-157 §5 steps 2-3).
* **FU264-L2 — the natural event bootstrap.** The arc needs `event_param != 0`
  and the height-table row needs `camY` off zero; neither is produced live from
  a cold start. Native owners: `FUN_00070B94` (`0x70BA8` `height <= 0 -> 0`),
  the height-table selector `d >= 0x38` at `FUN_0007AE70` (`0x7AF0B`/`0x7AF14`;
  engine `fifa96_action_handlers.c:114-118`). Next hop: a live jump selector
  (R17 / FU-153-L1) plus any live Y source.
* **FU264-L3 — the unported band Y writers.** Twelve writers in the
  0x74xxx..0x77xxx sequence/set-piece/keeper band and `0x83E7D` are jump-table
  code with no analyzed function and no engine symbol (fresh
  `get_function_by_address` = "No function" for `0x74550`, `0x7568B`, `0x75D53`,
  `0x75F67`); only the keeper cut is ported. Native owner: the band itself.
  Next hop: attribute each cut to its row (they are the closed-cam / restart
  cuts).
* **FU264-L4 — R2-L1's framing integrators (leg 9).** `FUN_0004E834`'s roll
  (`0x4E989` `FUN_0004DF34`), snap (`0x4E9D9`/`0x4EA4F`/`0x4EA7C`
  `FUN_0004C7D0`), the `+0x18`/`+0x10` lerps and the class 3/4 yaw snap are
  unported (only `fifa96_camera_behavior_steady`'s clamps + horizon jitter are
  ported, unreachable from view_mode 0 live). Native owner: `FUN_0004E834`.
  Framing only — does not move the place.
* **FU264-L5 — the `FUN_00070DE0` reaction's jitter term.** The ported
  `fifa96_camera_reposition` drops the arc-elevation branch of the `uVar5 & 0x10`
  mask (its own comment cites OL-T11-79); the `FUN_00070B94` elevation loop in
  the reaction tail is carried. Native owner: `FUN_00070DE0 0x7130E` region.

---

## Evidence table (this session)

| claim | evidence (fresh) |
|---|---|
| `[0x157750]` writer census = 146 refs / 22 writes | `get_xrefs_to 0x157750` (offset 0 + 100) |
| operand census = 84 sites; 10 explicit stores | `search_instructions operand "157750"` |
| the arc = the open-play per-frame writer, event-gated | `disassemble_bytes 0x737A1..0x737C8` |
| `FUN_00070B94` = the 16-bit ramp arc | `disassemble_function 0x70B94` (40 insns) |
| row-0E pan window `0x20 < camY < 0x70` | `disassemble_bytes 0x82806..0x82845` |
| row-04 half-line admission `camY <= 0x50` | `disassemble_bytes 0x7EDD0..0x7EDE7` |
| height-table `d` fold `word[0x157750]-word[actor+0x5D] >= 0x38` | `disassemble_function 0x7AE70` (273 insns; `0x7AEFD/0x7AF0B/0x7AF14`) |
| the `0x8061B`/`0x81157` camY-vs-`0x38` sites are a **different** gate | `disassemble_bytes 0x805D0/0x81118` |
| `[0x157744]` runtime-populated by `FUN_0006FFC0` | `disassemble_bytes 0x6FFC0` (`0x6FFC6`/`0x6FFE9`); `get_xrefs_to 0x157744` |
| the `0xF0` follow gate is on `word[0x1577F0]` | `disassemble_bytes 0x71E20` (`0x71E26`) |
| keeper release cut `camY := 0x50` | `disassemble_bytes 0x75D40..0x75D7F` |
| sequence/throw-in cuts | `disassemble_bytes 0x74550/0x85820/0x86010/0x83E60/0x840B0` |
| `FUN_00070DE0` = the arc's out-of-bounds reaction | `decompile_function 0x70DE0` |
| R2-L1 = `FUN_0004E834`, 3 xrefs, view-object only | `disassemble_function 0x4E834` (364 insns); `get_xrefs_to 0x4E834` |
| the camera-0 view object | `read_memory 0x107508` (32 B) |

Engine source read: `fifa96_camera.c` (60-160, 273-472, 505-534, 562-661,
925-994), `fifa96_action_handlers.c` (95-134), `fifa96_match_run.c`
(3120-3200, 3741-3860, 4140-4184), `fifa96_match_handlers.c` (505-534,
1445-1456, 1603-1724, 1812-1841, 1962-1989).
Carried docs: FU-148 §15, FU-152 §2.8/2.9, FU-153 §4-5, FU-154, FU-156 §3,
FU-157 §1-5/§7/§9, FU-163 §7 (R11-L4), FU-168 (incl. the T2 status),
FU-171 (incl. the T2/T3 status), `docs/ENGINE.md` (1749-1756).
