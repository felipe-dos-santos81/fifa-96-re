# FU-153 — the `0x7CD60` second arm and the band-2 crossing (R1, phase-12 recon)

**Question (phase-12 plan R1).** What live event can produce a trajectory that crosses
band-2 (`d == 0x70` exactly) at a kick frame — closing phase-11 leg **15.a**? Prime
suspect: the `0x7CD60` second arm's missing tracked-side producer (leg 15.c / OL-68 —
the swap arm is ported but untested/live-dormant).

**Status: closed with a disproven suspect + the named next-hop producer.** The second
arm does **not** touch any height cell, the band algebra or the kick trajectory; it
writes only the team long-ball callback cells and the slot swap. The band-2 crossing is
a *camera-height-arc sampling* problem, and the next-hop producer is the **live record
height (jump/header) producer** (`FUN_0007BF20 0x7C072..0x7C10C` + the installer
constants), which makes the height-table code-4 receive (the seed) reachable. Details,
byte cites and the T1 wiring recommendation below.

All Ghidra reads in this slice are fresh, program **`/FIFA96.EXE`** (case-sensitive),
flat LE-image addresses, standard MCP tools only.

---

## 1. Method

* Fresh `get_xrefs_to 0x1577CA` (88 refs) and `0x157750` (146 refs);
  `search_instructions operand 157750` (84 sites) for the writer census.
* `decompile_function` of `0x7CD60`, `0x786A0`, `0x71C94`, `0x71DF4`, `0x70B94`,
  `0x736AC`, `0x700F4`, `0x6FFC0`, `0x73D90`, `0x7A084`, `0x7AE70`, `0x70C08`;
  `disassemble_bytes` of `0x7CD60..0x7CDD5` (39 insns, quoted §2), `0x81500..0x8154F`
  (stage-0 gate, quoted §3), `0x4B170..0x4B1BF` (the every-frame order), the direct
  `[0x157750]` writers `0x7456e`/`0x755a9`/`0x759f0`/`0x75d45`/`0x75ec0`/`0x75f60`/
  `0x76d90`/`0x774f0`/`0x83e60`/`0x840b0`/`0x85810`/`0x85900`/`0x86010` and the record
  jump arc `0x7C070..0x7C11F`.
* `read_memory 0x10F3C4` (arc table) and `0x10F4EE` (ramp table) — both re-verified
  against the ported arrays (`src/fifa96_loader/fifa96_camera.c`); the exact-hit
  derivation uses the full ported arrays (P1-phase-11 `read_memory` provenance,
  spot-checked here) plus a Python reproduction of `fifa96_camera_height`.
* Three **engine-model probes** (`/tmp/opencode/fu153/probe{,2,3}.c`, linked against the
  repo's built static libraries, run from the repo root against the ISO; no repo
  writes) to observe the live engine's arc/wait behaviour — labelled *engine-model*
  wherever cited.

---

## 2. `[0x1577CA]` — fresh writer census

Fresh `get_xrefs_to 0x1577CA`: **88 refs, exactly three writes** (reads elsewhere):

| site | body | effect |
|---|---|---|
| `0x71D27` | `FUN_00071C94` | **the only setter**: `[0x1577C4+side*4] = rec; [0x1577CA] = rec; rec+0x9B = 0; ...` after the `FUN_000700F4` reset at entry |
| `0x70258` | `FUN_000700F4` | **clear** (`DAT_001577ca = 0`; the camera reset — also clears `[0x1577EE].hi`/`[0x1577F2]/[0x1577F4]/[0x1577F8]`, zeroes the rate bytes; called at `FUN_00071C94` entry `0x71C9F` and from `FUN_0006FFC0`) |
| `0x70054` | `FUN_0006FFC0` | **clear** (with `[0x1577CE]`/`[0x1577D2]`, the camera-table install `[0x157744] = 0x10F3C4`, `[0x157748] = 0x10F4EE`, `[0x157740] = 0x110080`) |

`FUN_0006FFC0` callers (fresh xrefs): `FUN_00073D90` (`0x73D9B`) and `FUN_00088860`
(`0x88883`); `FUN_00073D90` is called from `FUN_0004B020` only (the match begin/reset
path — not the per-frame body; the every-frame body is `0x4B183..0x4B1C0`, which calls
the camera walk `0x736AC` at `0x4B193`, the optional `0x948AC` at `0x4B1A1`, then the
ball machine `0x8AF38` at `0x4B1A6`). So the clears are begin/reset-time, not per-frame.

**Live setters = the `FUN_00071C94` callers.** Its 11 call sites (FU-148 §13.4 census):
the receive arm `0x7A219` (**wired**, `fifa96_match_run_ball_receive`), row 04
`0x7EFCF`/`0x7F0D1` (**wired**, `fifa96_outfield_row04_step` → `fifa96_camera_event_set`),
row 1E stage 5 `0x75DC4` (**wired**, `fifa96_keeper_claim_step` apply), and the six
still-ledgered sites (`0x7F4E7`, `0x71B8A`, `0x70DD1`, `0x6FA62`, `0x77423`, `0x77DC6`,
`0x82A6C` — R4's scope; note `0x70DD1` is `FUN_00070C08`'s call, reached live from the
**wired** `FUN_0007BF20` tail via `match_run_record_camera_event`). The engine surface
is `camera.tracked`; the write-set itself is unchanged from FU-148 §13.2 (no new
setter appears in the fresh census).

`FUN_00071C94`'s height clamp (decompile this slice; the engine port cites the
native window 0x71CDB..0x71CF8 in `fifa96_camera_event_set`): `if (param_3 <
[0x157750]) param_3 = [0x157750]; if
(param_3 > 0x640) param_3 = 0x640;` then `[0x1577EE].hi = param_3` — i.e. the event
height is `max(caller height, current camera Y)`.

---

## 3. `FUN_0007CD60` second arm — byte-level

`disassemble_bytes 0x7CD60..0x7CDD5` (39 insns; quoted verbatim, fresh):

```
0x7CD64  MOV ECX,EAX                         ; rec
0x7CD66  MOV EBX,[0x00157a4a]; 0x7CD6C SAR EBX,0x18
0x7CD6F  CMP EBX,0x2; 0x7CD72 JNZ 0x7CDCC    ; phase byte [0x157A4D] == 2
0x7CD74  MOV EBX,[EAX]                       ; team = [rec]
0x7CD76  CMP dword[EBX+0x7CB],0x0; 0x7CD7D JNZ 0x7CDCC   ; already armed
0x7CD7F  MOV EBX,[EBX+0x7B2]                 ; EBX = [team+0x7B2] (target/receiver)
0x7CD85  CMP EAX,EBX; 0x7CD87 JNZ 0x7CD9E
0x7CD89  MOV ECX,[EBX]                       ; target's team
0x7CD8B  MOV [ECX+0x7CB],EBX                 ; ARM: [team+0x7CB] = target
0x7CD91  MOV EBX,[EBX]
0x7CD93  MOV word[EBX+0x820],0x3C            ; window
0x7CD9E  MOV ESI,[0x001577ca]                ; tracked
0x7CDA4  CMP EAX,ESI; 0x7CDA6 JNZ 0x7CDCC    ; rec == tracked
0x7CDA8  TEST EBX,EBX; 0x7CDAA JZ 0x7CDCC
0x7CDAC  CMP dword[EBX+0x20],0x0; 0x7CDB0 JNZ 0x7CDCC  ; target slot-free
0x7CDB2  MOV EDX,EBX
0x7CDB4  CALL 0x786A0                        ; CALL target verified (bytes e8e7b8ffff)
0x7CDB9  MOV EAX,[ESI]                       ; tracked's team
0x7CDBB  MOV [EAX+0x7CB],EBX                 ; ARM: [tracked_team+0x7CB] = target
0x7CDC1  MOV EAX,[ESI]
0x7CDC3  MOV word[EAX+0x820],0x3C
0x7CDCC  MOV EAX,1; pops; RET                ; returns 1 on every path
```

`FUN_000786A0` (decompiled, fresh): `p = [rec+0x20]; if (p != 0 && [target+0x20] == 0)
{ [target+0x20] = p; *p = target; [rec+0x20] = 0; word[target_slot+4/6/8/0xA/0xC/0x14/0x16]
= 0; }` — moves the slot block from the tracked record to the target and clears seven
fields of the moved block (the OL-68 "slot-block field clears").

**Second-arm preconditions in full:** phase 2; `[team+0x7CB] == 0`; `rec ==
[0x1577CA]` (the tracked record); `[team+0x7B2] != 0`; `[[team+0x7B2]+0x20] == 0`
(target slot-free). Effect: slot swap + `[team+0x7CB] = target` +
`word[team+0x820] = 0x3C`.

**What it does NOT touch:** `[0x157750]`, `[0x1577EE]/[0x1577F0]/[0x1577F2]`, the event
row band, `state->traj`, or the camera. The arm only changes *which record* carries the
armed 0x40 callback (and which record holds the slot). The engine port
(`fifa96_match_run.c:570-594`) matches this (slot swap via `fifa96_action_duel_swap`,
`timer7cb = team->target`, `timer820 = 0x3C`).

**Conclusion 1: the second arm cannot be the band-2 unblock.** Whatever it enables in
live play (the tracked-request variant of the long-ball call — a separate, still-open
leg), it is orthogonal to the `d == 0x70` crossing. The phase-12 plan's prime suspect is
**disproven** by the store set above.

---

## 4. The band-2 gate — what actually has to happen

### 4.1 The stage-0 admission (first-hand, `0x81512..0x8153F`)

```
0x81512  MOV byte[EBP+0x9E],1
0x81519  MOV EAX,[EBP+0x69]        ; lane dword
0x8151C  MOV DX,[EBP+0x5D]        ; rec pos_y
0x81520  SAR EAX,0x10             ; lane word (+0x6B)
0x81523  ADD EDX,0x70
0x81526  CMP EAX,0x40
0x81529  JG 0x81538               ; lane > 0x40 -> wait/cancel
0x8152B  MOV EBX,[0x157750]       ; ball height = the camera Y cell
0x81531  MOVSX EAX,DX
0x81534  CMP EAX,EBX
0x81536  JGE 0x8154C              ; pos_y + 0x70 >= ball -> ADVANCE
0x81538  CMP dword[EBP+0x89],0x3C
0x8153F  JG 0x81702               ; timer89 > 0x3C -> cancel (tail 07)
          (else: wait one frame; timer89 += delta at the machine top)
```

### 4.2 The band algebra (`FUN_0007AE70`, decompiled fresh)

`sVar3 = [0x157750] - word[rec+0x5D]` (d); for a grounded record (`dword[rec+0x5D] == 0`)
the banded active/idle table is used with
`sVar3 = (d < 0x20) ? 0 : (d < 0x70) ? 1 : 2;`
`puVar7 = active ? &DAT_00110196 : &DAT_0011024A; iVar5 = (band*2 + class*6 + idx)*5`
(10-byte rows). For an airborne record (`dword[rec+0x5D] != 0`) the height table
`&DAT_001102FE` is used instead and band is not consulted (`d < 0x38 -> no row`).

Class for the long-ball: `code == 0x40 -> class 2, idx 0`. Active table `0x110196`
(= ported `match_kick_rows_active`) index `2*band + 12`: **band 0 -> row 12 code 3**,
**band 1 -> row 14 code 6**, **band 2 -> row 16 code 4** (`4, 8, 240, 0, 208, 2, 112, 0,
64, 9`). Only band-2 gives the receive code 4 (class 1 behaves the same: index 10 also
code 4).

**So band-2 = `d == 0x70` exactly**: band-2 needs `d >= 0x70` (§4.2) while stage-0
advances only for `d <= 0x70` (§4.1). With a grounded kicker (`pos_y == 0`),
`[0x157750] == 0x70` at the stage-0 evaluation frame.

### 4.3 The height cell `[0x157750]` — full writer census (`search_instructions`, 84 sites)

Two classes only:

* **the arc** — `FUN_000736AC 0x737B4 MOV [0x157750],EAX` (the every-frame camera walk,
  called at `0x4B193`), which stores `FUN_00070B94(timer)` **only while
  `[0x1577EE].hi != 0`** (`FUN_00070B94` = `max(0, arc[0x94-|F2-t|] - arc[0x94-F2])`, the
  149-word table at `0x10F3C4`, F2 = `[0x1577F2]`);
* **direct camera cuts** — always the pattern `[0x15774C/50/54] <- triple` with the Y
  overwritten by a constant or `rec+0x38`: `0x7456E` (`rec[+5D]+0x38`), `0x755A9`
  (`rec[+5D]+0x38`), `0x759F5..0x75A0F` (`0x38`, triple from `rec+0x59`), `0x75D69`
  (`0x50`), `0x75EEA` (`0x10`), `0x75F86` (`0x80`), `0x76DB5` (`rec[+5D]+0x38`),
  `0x7751B` (`rec[+5D]+0x38`), `0x83E7D` (MOVSD from `0x158897`, the ball/state triple),
  `0x840CB`/`0x85836`/`0x8602C` (MOVSD from `0x157764`, the camera target triple),
  `0x85919` (`0x80`).

No producer writes `d == 0x70` directly. **The only way `d == 0x70` arises is a sampled
arc value of exactly 0x70** (or `0x70 + rec.pos_y` for an airborne kicker).

### 4.4 The exact-hit height set (first-hand tables, reproduced)

`fifa96_camera_height` samples only `t = timer`, with the timer starting at
`F6 = f2 - ramp(H - ty)` (`fifa96_camera_event_ramp`) and `+= 2` per granted frame
(the whole-frame delta 2, FU-62 §4.3; `state.frame_delta` in the engine, `[0x157A64]`
in the native). For a fresh event with `ty = 0` the grid is even. Computing
`arc(t) = table[0x94-|f2-t|] - table[0x94-f2]` over the verified tables, the event
heights H that sample **exactly 112 (0x70)** on the even grid are:

```
0x71..0x74, 0x95..0x9C, 0xC1..0xC4, 0xDD..0xE4, 0x2CD..0x2DC, 0x4D5..0x4E8   (60 values)
```

Sample witnesses (f2, hit t, crest): `H=0x71..0x74` f2=40, t=32/48, crest 116;
`H=0x95..0x9C` f2=46, t=22/70, crest 153; `H=0xE0..0xE4` f2=56, t=16/96, crest 228.
Counter-examples: **`H=0x70` crest 110** (the record-event steady state — two short of
the boundary); **`H=0x100` (the phase-11 follow height) never samples 0x70** (the
descent skips 0x6A -> 0x71 — the phase-11 "cannot hold the boundary" observation
reproduced exactly).

The **wait window makes the crossing practical**: a kick pressed while `d > 0x70`
waits (timer89 accumulates 2/frame, cancel at > 0x3C = 30 frames) and advances at the
**first sample `<= 0x70`** — exactly 0x70 when the descent samples it. For `H=0x95..0x9C`
the descent hit is t=70; pressing at t in [67,95] -> wait <= 29 frames -> band-2 at
t=70. For `H=0xE0` the descent hit is t=96; press at t in [67,95]. (Ascent hits t=22 /
t=16 also work with an exact-frame press.)

---

## 5. Which live events can supply a hit-set H?

The event height H is set by `FUN_00071C94` from its `param_3` (the receive arm's
trajectory word) — plus the row-04/record-event/keeper callers whose param_3 is 0/pos_y/
0x50 (all below the hit set). So the only high-H source is the **kick tail's receive
arm** (`FUN_0007A084` -> `FUN_00071C94(rec, vec, traj)`), i.e. the staged row's traj.
The traj quantizers:

| path | traj | hit-capable? |
|---|---|---|
| mode-0x40 code-4 (`flags == 0x40`) | `0x90 + (r&7)*(0x10 - desc15)` | **0xE0 with desc15=0, k=5** (hit); 0x100 with k=7 (follow, not a hit); desc15 != 0 shifts the set (e.g. 4 -> 0x9C hit) |
| non-0x40 code-4 | `0x70 + speed/((r&0x7F)+3)` | yes (RNG divisor; speed clamp [0xE0/0x1E0/0x2D0 .. 0x1B0/0x2D0/0x4B0] depending on row) |
| code-5 | `0x50 + min(counter23,0x3C)^2*4 / 32` (clamped [0x1E0,0x5A0]) | only odd counters 17 -> 0x74; live counters step by the frame delta 2 -> even {0x62,0x68,0x70,0x78} — **none in the set** (0x70 crest 110; 0x78 not a hit) |
| code-7 / code-13 | 0 | no |
| height-table code-4 (airborne record) | `0x70 + speed/((r&0x7F)+3)` (or the 0x40 override) | **yes** — no band-2 precondition |

**The prior-free seed is the height-table code-4 path: an airborne kicker.** The
grounded code-4/code-5 paths are either circular (need band-2 already) or blocked by
the even-counter quantization.

### 5.1 The live record-height (jump) producer — the named next hop

Fresh `search_instructions` for the record `+0x5D` stores + `disassemble_bytes
0x7C070..0x7C11F`:

* `FUN_0007BF20 0x7C072..0x7C10C` — the **jump arc**: the record's jump timer at
  `+0x85/+0x87` is advanced (`0x7C072 ADD ESI,EAX`, `0x7C07A MOV word[rec+0x87],SI`),
  then `0x7C0BF..0x7C101` computes `height = table[0x94-|a|] - table[0x94-b]` **from the
  same `[0x157744]` = `0x10F3C4` arc table** and stores it at `[rec+0x5D]`
  (`0x7C101 MOV [EBP+0x5D],EBX`), zeroing on the landing edge (`0x7C098`/`0x7C10C`).
  The native runs this inside `FUN_0007BF20`, whose tail the engine already ports
  (`match_run_record_camera_event`) — the jump-arc *block* is unported.
* Installer constants seed the height instantly: `FUN_0006E8E8` writes
  `[rec+0x5D] = 0x18` (`0x6ED33`), `0x10` (`0x6EE14`), `0` (`0x6EEB7`), `0x50`
  (`0x6FB5C`) — installer-arm heights (which action arm each belongs to is not derived
  here). `FUN_00079B6C 0x79B7A` and the BF20 clears reset it to 0.
* Engine status: `mr->record.pos_y` has **no live producer**; the kick actor stages it
  (`match_kick_from_record` `s->pos_y`), the row resolver already supports the
  height-table arm (`fifa96_action_handlers.c:113-118`: `event->height != 0` ->
  `HEIGHT` table, `d < 0x38 -> no row`).

So with a live jump/header (`record.pos_y != 0`), a long-ball kick takes the
**height-table code-4 row** (class 2 -> index `2*2+0 = 4` of `0x1102FE` — `match_kick_rows_height[40]`:
`4, 8, 224, 1, 176, 4, 112, 0, 64, 9`), stages code 4 with the mode-0x40 traj
quantization (or the RNG divisor), the receive arm sets H, and the **arc crossing** of
§4.4 becomes reachable — the seed for 15.a.

---

## 6. Engine-model probes (supporting; `/tmp/opencode/fu153/probe*.c`)

Linked against the repo's built static libraries (`/FIFA96.EXE` ISO fixture; no repo
writes):

* **probe2** (claim at the cold camera, then `fifa96_camera_event_set(H=0xE0, ty=0)`):
  the arc produced `pos_y = 16, 32, 47, 61, 75, 88, 100, **112**, 124, 134` at t = 2..20
  — the exact-hit sample at t=16 verified in the live engine model; the stage-0 wait was
  observed (`timer89 22 -> 36` while `d > 0x70`).
* **probe3** (same, pressing at parameterized descent heights): the live **record camera
  event** (`match_run_record_camera_event` -> `FUN_00070C08`) **re-seeds the event**
  (`event_param 0xE0 -> 0xE1`, and in another run `-> 0x15`) while the pan runs — a raw
  injected seed does not survive; the re-seeded arc's timer starts mid-flight
  (`F6 = f2 - ramp(H - ty)`, ty = the current Y) and its sample grid can skip 0x70.
  (This is the *real* "tracked-side" camera producer the phase-12 plan suspected — it
  is `FUN_00070C08`, **not** the `0x7CD60` second arm.)
* probe1: with the arm ordering of the phase-11 test the live long-ball request armed
  (`timer7cb 9`) and the machine staged mode 0x40; the kick frame landed band-1 because
  the multi-frame input edge sequence consumed the boundary sample — T1 must align the
  press ~2-3 frames before the descent sample (see §7).

These probes are engine-model evidence only; every native claim above is separately
byte-cited.

---

## 7. T1 wiring recommendation (concrete)

Closing 15.a in the engine:

1. **Port the live record-height producer (the unblock).** Minimal slice: the
   `FUN_0007BF20` jump-arc block `0x7C072..0x7C10C` (the `+0x85/+0x87` jump timer + the
   `0x10F3C4`-table difference -> `record.pos_y`), plus one live starter — one of the
   `FUN_0006E8E8` install height constants (0x18 at 0x6ED33, 0x10 at 0x6EE14, 0x50 at
   0x6FB5C; which action arm each belongs to is not derived here) reached from the
   already-ported installer-arm dispatch. If the full jump timer is out of slice, stage
   the constant height on the install and number the timer as the leg.
2. **RED test** `test_live_airborne_long_ball_seed_then_band2_follow` (ISO-gated,
   `test_engine_match_frame.c`):
   * phase 2; live long-ball request (V, first arm — the phase-11 flow);
   * drive the holder into a header/jump (the new producer) and kick: assert the
     height-table arm ran (`mr.entities.ball.pair.code == 4`) and that the staged
     `camera.event_param` (= H = traj) is in the exact-hit set
     `{0x71..0x74, 0x95..0x9C, 0xC1..0xC4, 0xDD..0xE4, 0x2CD..0x2DC, 0x4D5..0x4E8}`
     — iterate the deterministic RNG draws (bounded loop) until a draw lands there
     (`0xE0` = the desc15=0 k=5 draw; the RNG-0 first draw is k=7 -> 0x100, which does
     **not** hit — the loop must advance the draws or pin the draw);
   * wait for the arc's descending window (`pos_y <= 0x80 && > 0x70`), re-arm the
     long-ball, press KICK ~2 frames before the hit sample (`t=96` for H=0xE0):
     assert `pair.flags == 0x40`, `pair.code == 4`, `pair.traj == 0x100` (the follow
     draw), `camera.tracked >= 0`, `event_param == 0x100`, and
     `fifa96_match_run_camera_follow(&mr) == 1` with the tracked dirs in the camera
     velocity — **the live follow with no pinned `pos_y`**.
3. **Do not touch `src/fifa96_ball_pairing.c` or the second arm.** Its port is
   verified correct; its live-dormancy is not 15.a. If T1 wants the tracked-request
   variant tested, add a separate test (`id == camera.tracked && team->target
   slotless`) and keep the 15.c leg (tracked-side request semantics) open.
4. **Docs:** update `docs/ENGINE.md` leg 15.a to name the jump producer as the seed and
   the exact-hit sampling condition; keep 15.b; reword 15.c (OL-68) as
   independent of the height.

---

## 8. Numbered legs (new)

* **FU153-L1 — the live record-height (jump) producer.** `FUN_0007BF20
  0x7C072..0x7C10C` (jump timer `+0x85/+0x87`, the `0x10F3C4` arc difference ->
  `+0x5D`), the `FUN_0006E8E8` install constants `0x6ED33/0x6EE14/0x6EEB7/0x6FB5C`
  (0x18/0x10/0/0x50), and the jump-timer starters are unported; the engine
  `record.pos_y` has no live producer. **The named next-hop for 15.a.**
* **FU153-L2 — `slot[+0x23]` (the pad hold counter).** FU-70 `+0x23`
  (`0x78A02 MOV AL,[ESP]; 0x78A05 ADD byte[EBX+0x23],AL`); the engine's
  `match_kick_run` memsets the slot and never fills `counter23` (always 0 -> code-5
  traj 0x5F). With the live counter the code-5 trajs quantize 0x5F..0x7D, but the
  delta-2 even grid yields {0x62,0x68,0x70,0x78} — none in the exact-hit set; the
  counter's native increment value/parity at the map site (the `[ESP]` argument) is
  not statically resolved here. Quantizer leg, not the primary unblock.
* **FU153-L3 — `desc_15` (roster descriptor `rec[+4][+0x15]`).** Engine stand-in 0
  (OL-72); it shifts the mode-0x40 code-4 draw set (0x90 + k*(0x10-desc15)). With 0 the
  set contains the hit 0xE0 (k=5) and the follow 0x100 (k=7); follow-eligible heights
  (>0xF0) exist only for desc15 <= 2 with k=7. The live descriptor source (the runtime
  `rec[+4]` block) remains unmodeled.
* **FU153-L4 — the record camera event re-seed.** `FUN_00070C08` (port
  `fifa96_camera_record_event`) re-seeds the event mid-flight (probe3: 0xE0 -> 0xE1 /
  0x15); its `event_param` arms (reflect -> the current camera Y; ramp -> toward
  `rec.pos_y + 0x70` with the double-step `new += (new-target)>>1`) can leave the
  camera Y at the crest 110 for a grounded record — two short of the band. Any T1 test
  must account for this live setter; the interaction with a jump/header-driven event
  chain is untested.

---

## 9. Evidence index (fresh this slice)

* `get_xrefs_to 0x1577CA` (88 refs; 3 writes: 0x71D27, 0x70258, 0x70054);
  `get_xrefs_to 0x157750` (146 refs); `search_instructions 157750` (84 sites).
* `disassemble_bytes 0x7CD60..0x7CDD5` (39 insns, quoted §3) — `CALL 0x786A0` target
  verified from bytes `e8e7b8ffff`.
* `decompile_function 0x786A0`, `0x71C94`, `0x71DF4`, `0x70B94`, `0x736AC`, `0x700F4`,
  `0x6FFC0`, `0x73D90`, `0x7A084`, `0x7AE70`, `0x70C08`.
* `disassemble_bytes 0x81500..0x8154F` (stage-0), `0x4B170..0x4B1BF` (frame order),
  `0x7C070..0x7C11F` (jump arc).
* `read_memory 0x10F3C4` (64 B: `0000 1500 2b00 4000 ...` = arc table head) and
  `0x10F4EE` (64 B: ramp table head) — match the ported arrays.
* Engine model: `tests/test_engine_match_frame.c:1777` (`test_live_long_ball_mode40_band2_follow`
  staged `pos_y = 0x70`), `src/fifa96_loader/fifa96_camera.c:638-709` (`fifa96_camera_ramp`/`fifa96_camera_height`),
  `:993-1068` (`fifa96_camera_record_event`), `src/fifa96_engine/fifa96_match_run.c:570-594`
  (the second arm port), `:2500-2510` (the follow gates), `src/fifa96_engine/fifa96_match_handlers.c:1066-1090`
  (slot staging; `counter23` never filled), `src/fifa96_loader/fifa96_action_handlers.c:113-130`
  (the band), `src/fifa96_loader/fifa96_ball_pairing.c:780-788` (the mode-0x40 code-4 traj).
* Probes `/tmp/opencode/fu153/probe{,2,3}.c` (sources kept for the controller).

---

## 10. T1 landing (phase-12) — the producer port, the chain reality, new legs

**Landed (first-hand re-verified this task; `/FIFA96.EXE`, fresh reads).** The
T1 implementation ports exactly the reachable subset of §5.1/§7:

* **the `FUN_0007BF20` jump arc `0x7C057..0x7C113`** into the engine's
  per-record mover path (`match_run_record_jump_step` over the new pool/staging
  `jump_base` (+0x85)/`jump_timer` (+0x87) fields; the `0x7C057/0x7C05F` gate,
  the `0x7C065..0x7C07A` advance, the `0x7C094/0x7C096` landing edge with the
  `0x7C098..0x7C0BA` zeroing + position->target MOVSD, the
  `0x7C0BF..0x7C101` arc). The shared `[0x157744] = 0x10F3C4` sampler is
  exposed as `fifa96_camera_arc_height(peak, timer)` (same table as
  FUN_00070B94, first-hand: `0x70B94` body + the `0x7C0D9 MOV EAX,[0x157744]`).
* **the FUN_0006E8E8 animation-aux-keyed jump arms** (`match_run_record_jump_arm`):
  aux 0x0E -> case 0xD `pos_y 0x10` + `+0x85 = +0x87 = ramp(word[+0x81])`
  (`0x6EDFD/0x6EE04/0x6EE14`), aux 0x3A -> case 0x39 `pos_y 0x18` + the same
  pair (`0x6ED1F/0x6ED26/0x6ED33`), aux 0x0F -> case 0xE the zero-height
  relaunch (`0x6EE90..0x6EEBE`). The arm's velocity/sine/move writes and the
  case-0xE `pos_y != 0` frame-3 resolve stay leg **T1-L1** (the live row
  selector that reaches the jump-aux rows 0x5E/0x12/0x14/0x5C is the same leg:
  every ported `fifa96_arm_anim_select` id set {0x46,0x5D,0x60,0x61,0x15,0x19,
  0x02,0x03,0x58,0x5B,0x6B,0x26,0x0F,0x53,0x69,0x5F} misses them — the rows
  are entered only by direct FUN_0006E598 calls from unported action bodies,
  e.g. `0x81DA4` inside the row-0x0C body, first-hand this task).
* **the executable FU-84 frame records `0x10E200..0x10EF00`** (0xD00 bytes,
  reconciled byte-for-byte against `read_memory 0x10E200`/`0x10E400`) as the
  aux source; the duration/sprite columns stay on the render's synthetic leg
  so the tape does not move.

**Independently re-verified (the §4.4 spot-checks).** The exact-hit height set
`{0x71..0x74, 0x95..0x9C, 0xC1..0xC4, 0xDD..0xE4, 0x2CD..0x2DC, 0x4D5..0x4E8}`
(60 values; H=0xE0 samples 0x70 at t=16/96; H=0x70 crest 110 and H=0x100 never
sample it) reproduced from the reconciled `0x10F3C4`/`0x10F4EE` tables; the
stage-0 JGE / band algebra and the `FUN_0006E8E8` install constants
(`0x6ED33`/`0x6EE14`/`0x6FB5C`) re-read.

**Push-back (with evidence) on the §7 recommendation.** (1) The recommended
"installer-arm" reading is wrong: the heights live in the **FUN_0006E8E8**
animation-aux arms, not in the installer `FUN_0007D9A4` (decompile: it writes
`+0x91/+0x92/+0x89/+0x9E/+0x7B` and the `+0x18` action pointer only — no
`+0x5D`/`+0x85`). (2) The chain cannot run fully live: the airborne
height-table row needs `d = camera.pos_y - rec.pos_y >= 0x38`
(`fifa96_action_handlers.c` height arm), while **no ported producer raises
`0x157750` off zero** (the §4.3 writer census; `FUN_00070C08` re-seeds only
with `ball != 0`, `0x7C81B`) — the seed kick is therefore reachable only over
a bootstrapped event. (3) The two-kick sequence of §7 collides with the
row-07 machine's stage-2 `+0x44` gate, whose animation/event producer is
unported (the engine `row44` stays 0), so a second kick by the same record in
one possession cannot stage — the T1 test splits the chain (fresh chain per
half).

**New legs (engine).**

* **T1-L1** the FUN_0006E8E8 arm's velocity/sine/move writes + the live jump
  row selector (above).
* **T1-L2** the row-07 stage-2 `+0x44` animation/event ack (blocks the second
  kick per possession; the engine `row44` producer is FU-151/FU-84's unported
  fan-out).
* **T1-L3** the live camera bootstrap: the producer that raises `[0x157750]`
  off zero in open play (the direct-cut rows 0x7456E/0x755A9/0x76DB5/0x7751B
  `rec[+0x5D]+0x38`, 0x75F86/0x85919 `0x80`, and the record-event path) — the
  T1 test discloses the `fifa96_camera_event_set` bootstrap stand-in for it.

**Chain evidence (engine tests, RED-verified).**
`test_record_jump_arc_starter` (the starter + arc trace 16/15/13/12/9/6/2 and
the t=31 landing; the arm's `ramp(0x10) = 15` first-hand) and the ISO
`test_live_airborne_long_ball_seed_then_band2_follow` (A: the live claim ->
aux-row jump -> the height-table code-4 receive with the mode-0x40 k=5 `0xE0`
draw and the even grid, `d < 0x70` as the height-table discriminator; B: the
exact-`0x70`-sample band-2 kick with the k=7 `0x100` draw and
`fifa96_match_run_camera_follow == 1`). Smoke `p12-t1-*`: live motion, no
visible follow (honest). Goldens unmoved.

---

## 11. Errata (T1 fix round 1, 2026-10-11) — the arm's ramp argument is `+0x83`, not `+0x81`

**Quoted from §10 (left in place, per the no-rewrite rule):**

> * **the FUN_0006E8E8 animation-aux-keyed jump arms** (`match_run_record_jump_arm`):
>   aux 0x0E -> case 0xD `pos_y 0x10` + `+0x85 = +0x87 = ramp(word[+0x81])`
>   (`0x6EDFD/0x6EE04/0x6EE14`), aux 0x3A -> case 0x39 `pos_y 0x18` + the same
>   pair (`0x6ED1F/0x6ED26/0x6ED33`), aux 0x0F -> case 0xE the zero-height
>   relaunch (`0x6EE90..0x6EEBE`).

**The `ramp(word[+0x81])` reading is wrong — the fifth `dword>>16` alias
strike in this project.** The arms' ramp argument is the word at **`+0x83`**,
read through the trap `dword[+0x81] >> 16` (= the word at `+0x81+2`), and the
value is the arm's own just-stored marker:

```
aux 0x3A (case 0x39), first-hand disassemble_bytes:
  0x6ED08  MOV word[EBP+0x83], 0x18          ; the marker
  0x6ED11  MOV EAX, dword[EBP+0x81]          ; low word +0x81, high word +0x83
  0x6ED17  SAR EAX, 0x10                     ; = word[+0x83] = 0x18
  0x6ED1A  CALL 0x702F8                      ; ramp(0x18)
  0x6ED1F  MOV word[EBP+0x85], AX
  0x6ED26  MOV word[EBP+0x87], AX
  0x6ED33  MOV dword[EBP+0x5d], 0x18

aux 0x0E (case 0xD):
  0x6EDE6  MOV word[EBP+0x83], 0x10
  0x6EDEF  MOV EAX, dword[EBP+0x81]; 0x6EDF5 SAR EAX,0x10; 0x6EDF8 CALL 0x702F8
  0x6EDFD/0x6EE04  store +0x85/+0x87;  0x6EE14  MOV dword[EBP+0x5d], 0x10

aux 0x25 (case 0x24, unported):
  0x6FB2F  MOV word[EBP+0x83], 0x50
  0x6FB38/0x6FB3E/0x6FB41  the same dword-shift -> ramp(0x50)
```

The decompile agrees literally: case 0xd stores `+0x83 = 0x10` and the next
line calls `FUN_000702f8((dword[+0x81]) >> 16)`; case 0x39 the same with
`0x18`. `word[+0x81]` is a **different cell** — the decayed countdown
(`0x7CA5C CMP word[EAX+0x81],0` / `0x7CA78 MOV word[EBP+0x81],0`, the
`entity_timer_decay` twin).

**Corrections to §10's consequences.** The live jump bases for the verified
rows are `ramp(0x10) = 15` (crest 16) and `ramp(0x18) = 18`; there is **no
`timer81` dependency** for the jump base, so T1-L1's "`timer81` producer"
framing (and the T1 report's "tiny 1-frame blip" concern) are withdrawn —
the misread made the port compute `ramp(timer81→0) = 3` (`pos_y = 1`), which
the T1 unit test masked by staging `timer81 = 0x10`. Fixed in the T1 fix
round 1 commit (see the T1 report §"Fix round 1"); the unit test now sets
`timer81 = 0` and still pins `jump_base 15` / the crest-16 trace, so the trap
is regression-detected. The case-0x0F arm's `FUN_00079B1C` snap (`0x6EE90/0x6EE92`, the
target := position call before the `+0x85` check) is added to the T1-L1 leg.

