# FU-306 — the record-event chaser (`FUN_00070C08`): the per-frame ramp, the call sites and the live re-arm

**Phase/Provenance:** M2 phase-29 Stage-1 recon-ahead wave (Task 1/4), 2026-10-21,
read-only. Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument). This session's `get_current_program_info` =
`FIFA96.EXE`, `watcom:LE:32:default`, image base 0, `.image` overlay,
**2708** functions (the brief/constraints quote 2706; the binary path,
language, base and overlay all match — the +2 function count is a DB-state
delta, not a code claim; noted here). Every address/byte below is a fresh
`disassemble_function` / `disassemble_bytes` / `read_memory` / `get_xrefs_to` /
`get_function_by_address` of **this** session unless marked *carried* (then the
source is named). The alias traps are applied (`dword[x]>>16` = the word at
`x+2`; `>>24` = the byte at `x+3`); every aliased read is called out inline.
No writes: no rename/comment/label/function/script/save; `/FIFA96.EXE`
untouched; the ISO untouched; no git.

Question (spec §3 item 1 / FU171-L4): the exact computation of
`FUN_00070C08`, its call sites and gates, how it raises the
`dword[0x1577EE]>>16` height, and whether it can re-arm the follow window
**live** (the spec §8.4 GO/NO-GO).

---

## 1. Result in one line

**The chaser is already ported and already on the granted-frame path**
(`fifa96_camera_record_event`, `src/fifa96_loader/fifa96_camera.c:1075`, called
from `match_run.c:1508`): it writes `word[0x1577F0]` — the high word of
`dword[0x1577EE]`, exactly the cell the follow gate tests — in a **reflect**
arm (`height := camY`) and a two-step **ramp** arm (target
`word[rec+0x5D] + 0x70`), then calls the event setter `FUN_00071C94` with a
re-derived seed triple and **`ECX = 1`** (the ramp param — first-hand: the
`0x92820` sink preserves ECX). The setter resets `pan_counter` and rebinds
`[0x1577CA]`, so a chaser fire **is** a real follow-window re-arm. But the
caller's gates admit it only for a record that is **not the tracked record**
(`0x7C83E CMP EBP,EAX; JZ 0x7C901`) and only while that record is
**within `0x10` of the camera focus and approaching** (`lane <= 0x10 &&
lane < bound`), at `pos_y <= camY <= pos_y + 0x70`. It therefore **sustains and
hands over** a tall event; it cannot bootstrap a follow and it cannot re-fire
for the record it just bound. Verdict: **GO as a one-shot live re-arm
(staged-producer Layer-1); NO-GO as the continuous per-frame re-arm** — the
latter is FU306-L1, a numbered dead-end per spec §8.4, not a blocker.

---

## 2. The body (fresh `disassemble_function 0x70C08`, 132 insns, `0x70C08..0x70DDE`)

### 2.1 Entry gates

```
0x70C08  PUSH EBX/ECX/EDX/ESI/EDI; SUB ESP,8
0x70C10  ESI = EAX                         ; rec
0x70C12  CMP byte[rec+0x9A],0
0x70C19  JNZ 0x70DD6                       ; bail (the same +0x9A skip the walk uses)
0x70C1F  EAX = dword[rec+0x8E]; SAR 0x18   ; = (int8)byte[rec+0x91] (action code)
0x70C28  CMP EAX,0x22; JNZ 0x70C3A
0x70C2D  CMP dword[0x157750],0x20
0x70C34  JL  0x70DD6                       ; code 0x22 needs camY >= 0x20
```

`EAX` is the record; there is no other argument. `ESI = rec` is kept for the
setter call (0x70DCC `MOV EAX,ESI`).

### 2.2 The seed correction (`0x8DC50`) and the RNG jitter

```
0x70C3A  AX = word[0x15774C]; SUB AX,word[0x157764]   ; camX - originX (16-bit)
0x70C47  DX = word[0x1577BA]                          ; event step x (setter triple)
0x70C4E  BX = word[rec+0x5D]                          ; rec.pos_y (word)
0x70C52  SUB EDX,EAX
0x70C54  CX = word[rec+0x5D]
0x70C58  word[ESP+2] = DX                             ; raw sx = step_x - (camX-originX)
0x70C5D  EAX = dword[ESP]; SAR 0x10                   ; = word[ESP+2] (the alias read)
0x70C60  EDX = 2; CALL 0x8DC50                        ; v1 = /4
0x70C6D  DX = word[0x157754]; DI = word[0x15776C]
0x70C7B  word[ESP+2] = AX                             ; v1
0x70C80  AX = word[0x1577BC]                          ; event step z
0x70C86  SUB EDX,EDI                                  ; camZ - originZ
0x70C88  SUB EAX,EDX
0x70C8A  word[ESP+4] = AX                             ; raw sz
0x70C8F  ADD EBX,0x70                                 ; ramp target (word)
0x70C92  EAX = dword[ESP+2]; SAR 0x10                 ; = word[ESP+4]
0x70C9E  ADD ECX,0x68                                 ; reflect threshold (word)
0x70CA1  CALL 0x8DC50                                  ; v2 = /4
0x70CAA  word[ESP+4] = AX                             ; v2
0x70CAF  TEST DX,DX; JNZ 0x70CFA                       ; if v1 != 0 skip
0x70CB4  TEST AX,AX; JNZ 0x70CFA                       ; if v2 != 0 skip
0x70CB9  CALL 0x92AC8; ... 0x70CC5 CALL 0x92AC8; ... 0x70CE1 CALL 0x92AC8
```

`0x8DC50` (fresh `0x8DC50..0x8DC67`) is a 16-bit-signed, truncate-toward-zero
shift: `TEST AX,AX; JL` → `CWDE; NEG; SAR EAX,CL; NEG`, else `CWDE; SAR EAX,CL`.
With `EDX = 2` each raw word is divided by 4 after the **word** store/re-read
(the `dword[ESP]>>16` and `dword[ESP+2]>>16` alias idioms).

The `[0x1577BA]/[0x1577BC]` pair the chaser reads is the **post-ramp step pair**:
the setter copies its caller triple there (`0x71CC8 MOVSD` + `0x71CC9 MOVSW`),
then `FUN_00070544` overwrites the pair with `vel * f8` (fresh
`disassemble_bytes 0x707A0..0x7080F`: `0x707B7 CX = word[0x1577F8]` = f8,
`0x707C4 AX = word[0x1577C0]` = vel_x, `0x707CA IMUL EAX,ECX`,
`0x707CD MOV [0x1577BA],AX`; the z twin at `0x707D3..0x707DC`). The engine's
`camera_event_ramp` writes the same `event_step_x/z` (`camera.c:790-792`). So
`v1/v2` are the event's residual direction after the camera's drift since the
last setter call, quartered — not the raw caller seed.

The jitter (only when **both** corrected seeds are zero): the three `0x92AC8`
draws give `t = (draw1 & 0x3F) + 0x30`; `draw2` bit 0 picks `+t/-t` for `v1`;
`draw3` bit 0 picks `+t/-t` for `v2` (`0x70CC0 XOR DH,AH` clears the high byte
before `AND DL,0x3F`). The engine port is exact here (`camera.c:1096-1108`).

### 2.3 The height: reflect vs ramp (`word[0x1577F0]`)

```
0x70CFA  MOVSX EAX,CX                       ; (int16)(pos_y + 0x68)
0x70CFD  CMP EAX,dword[0x157750]            ; camY (dword)
0x70D03  JG 0x70D43                         ; pos_y+0x68 > camY -> reflect
-- ramp arm --
0x70D05  EDX = dword[0x1577EE]; SAR 0x10    ; = word[0x1577F0] = event height
0x70D0B  EAX = MOVSX BX                     ; target = (int16)(pos_y + 0x70)
0x70D11  SUB EDX,EAX; SAR EDX,1; ADD EDX,EAX
0x70D17  word[0x1577F0] = DX                ; new = target + ((ev - target) >> 1)
0x70D1E  CMP BX,DX; JGE 0x70D9D             ; second step only when target < new
0x70D27  EDX = dword[0x1577EE]; SAR 0x10    ; re-read the stored new height
0x70D30  EBX = EDX; SUB EBX,EAX; EAX = EBX; SAR EAX,1; ADD EDX,EAX
0x70D3A  word[0x1577F0] = DX                ; new += (new - target) >> 1
0x70D41  JMP 0x70D9D
-- reflect arm --
0x70D43  CMP word[0x1577C0],0; JL -> NEG    ; e1 = |vel_x| (alias: dword[0x1577BE]>>16)
0x70D63  CMP word[0x1577C2],0; JL -> NEG    ; e2 = |vel_z| (alias: dword[0x1577C0]>>16)
0x70D81  CMP e1,e2; JLE 0x70D8C
0x70D85  NEG word[ESP+2]                     ; |vel_x| > |vel_z| -> negate v1
0x70D8C  NEG word[ESP+4]                     ; else negate v2
0x70D91  AX = word[0x157750]
0x70D97  word[0x1577F0] = AX                 ; height := camY
```

All three writes target `word[0x1577F0]` — the **high word of the dword at
`0x1577EE`** (`dword[0x1577EE] SAR 16`), not the low `event_cursor` word. This is
the cell the follow gate reads (§4).

### 2.4 The setter call (`EAX = rec`, `EDX = {dist,v1,v2}`, `EBX = height`, `ECX = 1`)

```
0x70D9D  EDX = dword[ESP+2]; SAR 0x10       ; v2 (word[ESP+4])
0x70DA1  EAX = dword[ESP];   SAR 0x10       ; v1 (word[ESP+2])
0x70DAA  CALL 0x8DC68                        ; mag = the octagonal distance
0x70DAF  EDX = 0x1C
0x70DB4  word[ESP] = AX                      ; triple word 0
0x70DB8  EAX = ESI                           ; rec
0x70DBA  MOV ECX,1                           ; <-- the FUN_00070544 ramp param
0x70DBF  CALL 0x92820                        ; sink(rec, 0x1C); PUSH/POP ECX -> preserved
0x70DC4  EDX = ESP                           ; &{mag, v1, v2}
0x70DC6  EBX = dword[0x1577EE]; SAR 0x10     ; = word[0x1577F0] (the final height)
0x70DCC  EAX = ESI; CALL 0x71C94            ; THE EVENT SETTER
0x70DD6  ADD ESP,8; POPs; RET
```

`0x8DC68` (fresh `0x8DC68..0x8DCD2`) is the shared octagonal distance
(`max + min/4` or `max + 3*min/8` by the `min <= max/2` compare). It is the
exact function the engine calls `fifa96_entity_distance`
(`src/fifa96_loader/fifa96_entity_update.c:211-229`, verified shape-for-shape
this session).

**`ECX = 1` survives the sink.** `FUN_00092820` is a 27-instruction leaf
(fresh `disassemble_function 0x92820`): `0x92821 PUSH ECX` … `0x9285F POP ECX`;
no instruction writes ECX, and the one nested call on the taken path
(`0x92849 CALL 0xCB2A4`; the `0x110F1C[0x1C] = 7` byte makes the `AND AL,1`
branch fall through) is `MOV EAX,[0x112E88]; RET` — EAX only. So the setter's
`0x71D60 MOVSX EAX,CX` consumes
**ramp_param = 1**, i.e. `FUN_00070544` takes the **ADD** sign
(`0x705ae TEST DX,DX; JNZ -> CWDE; JMP` vs `CWDE; NEG`). This closes
FU171-L4's sub-question (§8, erratum 1).

---

## 3. Call sites and gates

### 3.1 The BF20 tail — `0x7C7D3..0x7C8FC` (fresh `disassemble_bytes 0x7C7B0..0x7C9BF`, 528 B)

`FUN_0007BF20` is the per-record tail called after the record's action dispatch
(`0x7CD2B CALL [rec+0x18]` → `0x7CD30 CALL 0x6E8E8` → `0x7CD48 CALL 0x7BF20`)
and from the keeper machine (`0x785C5`). Fresh `get_xrefs_to 0x7BF20` = 2:
`0x785C5` (`FUN_000782D0`) and `0x7CD48` (`FUN_0007CA54`).

The tail's camera-event gate chain, exactly:

| site | bytes | gate (skip → `0x7C901`) |
|---|---|---|
| `0x7C7D3` | `8b 45 69; c1 f8 10; 83 f8 10; 0f 8f 1f 01 00 00` | `(int16)word[rec+0x6B] > 0x10` |
| `0x7C7E2` | `66 8b 45 6b; 66 3b 45 77; 0f 8d 11 01 00 00` | `word[rec+0x6B] >= word[rec+0x77]` (lane not decreasing) |
| `0x7C7F0` | `80 3d 20 78 15 00 00; 0f 85 04 01 00 00` | `byte[0x157820] != 0` |
| `0x7C7FD` | `80 3d 22 78 15 00 00; 0f 85 f7 00 00 00` | `byte[0x157822] != 0` |
| `0x7C80A` | `a1 bc 77 15 00; c1 f8 10; 83 f8 04; 0f 8e e6 00 00 00` | `word[0x1577BE] <= 4` (camera speed/bearing) |
| `0x7C81B` | `83 3d 50 77 15 00 00; 0f 84 d9 00 00 00` | `dword[0x157750] == 0` (camY) |
| `0x7C828` | `a1 ca 77 15 00; 85 c0; 74 30` | tracked `== 0` → **pass** to `0x7C861` |
| `0x7C831` | `80 b8 8d 00 00 00 00; 0f 84 c3 00 00 00` | tracked exists: `byte[tracked+0x8D] == 0` (inactive) |
| `0x7C83E` | `39 c5; 0f 84 bb 00 00 00` | `rec == tracked` (**the hand-over lock**) |
| `0x7C846` | `8b 80 8e 00 00 00; c1 f8 18; 83 f8 08; 0f 84 a9 00 00 00; 83 f8 09; 0f 84 a0 00 00 00` | `(int8)tracked[+0x91] ∈ {8,9}` |
| `0x7C861` | `a1 50 77 15 00; 8b 55 5d; 39 d0; 0f 8c 90 00 00 00` | `(int32)dword[rec+0x5D] > dword[0x157750]` (camY below the record) |
| `0x7C871` | `8b 1d 50 77 15 00; 8d 42 70; 39 d8; 0f 8c 7f 00 00 00` | `(int32)dword[rec+0x5D] + 0x70 < dword[0x157750]` |
| `0x7C882` | `66 a1 fa 77 15 00; 66 3b 05 f2 77 15 00; 0f 9e c0; 25 ff 00 00 00; 74 61` | timer `> word[0x1577F2]` → jump to the call, skipping only the `0x8F188` ring post |
| `0x7C8FA` | `89 e8; e8 07 43 ff ff` | `EAX = rec; CALL 0x70C08` |

Two consequences, first-hand:

* the admission band is `word[rec+0x5D] <= dword[0x157750] <= word[rec+0x5D]+0x70`;
* the tail **never fires for the tracked record** (`rec == tracked → RET`), and it
  requires the *tracked* record to exist, be active and not be code 8/9 when it
  does exist. The `0x8F188` ring post is the only timer/peak-conditional part;
  the chaser call itself is unconditional after the gates.

### 3.2 The caller chain — per record, per frame

* `FUN_0008D8EC` (`0x8D8EC..0x8DB6A`, fresh `disassemble_bytes 0x8DAF0..0x8DB7F`)
  calls `FUN_000782D0(rec=EBP, EBX=1)` at `0x8DB35` for the base record, then
  walks the **10** slots at `EBP+0xB2*n` (`EDX = EBP+0xB2; INC EBX; ADD EDX,0xB2;
  CMP EBX,0xB; JL`), calling `FUN_0007CA54(EDX)` at `0x8DB4D` for each slot with
  `byte[EDX+0x9A] == 0` — 11 records total per team block. This is the native
  pre-pass the engine models with `fifa96_match_entities_update`
  (`match_run.c:4382`).
* `FUN_0008D8EC` is called by `FUN_0004B100` twice (`0x4B2AB`, `0x4B2B2` — once
  per team block) and from the `0x74429/0x74430` jump-table band; `FUN_0004B100`
  is called from `FUN_000493A0`/`FUN_00049B28`/`FUN_0004A228` (fresh
  `get_xrefs_to 0x4B100`).
* The engine's per-record callback `match_run_dispatch_entity`
  (`match_run.c:1300`) runs the mover, the jump step, the lane block and then
  `match_run_record_camera_event` at `match_run.c:1508` — one call per active
  record per granted frame, before the pool write-back (`1509-1529`).

### 3.3 The second caller — `0x7C90C..0x7C98C` (unanalysed block)

Fresh `get_xrefs_to 0x70C08` = **2**: `0x7C8FC` (`FUN_0007BF20`) and `0x7C984`
(no function: `get_function_by_address 0x7C984` = "No function found").
`0x7C90C` itself has **no xrefs**, is not in the `0x1106E0` 64-entry action
table (`read_memory 0x1106E0`, 256 B) and is not present as a 4-byte pointer
anywhere in the image (`search_byte_patterns "0c c9 07 00"` = no match). Its
gate chain (`0x7C90F..0x7C984`) is the BF20 chain **minus** the lane pair and
minus the `0x8F188`/timer arm: `[0x157820]/[0x157822]/speed>4/camY/ tracked
active/rec!=tracked/code∉{8,9}/pos_y <= camY <= pos_y+0x70` → `CALL 0x70C08`.
Reachability is unproven (leg FU306-L5); its gates cannot admit anything the
live BF20 tail does not already admit, except for records the lane pair
excludes — so it cannot be the primary route.

---

## 4. How the chaser reaches the follow gate

The follow's call site and first arm read the chaser's cell directly:

```
0x739DC  EAX = dword[0x1577EE]; SAR 0x10; CMP EAX,0x10; JLE 0x73B70   ; gate 3
0x71E26  EAX = dword[0x1577EE]; SAR 0x10; CMP EAX,0xF0; JLE 0x71EF0   ; first arm
```

Both are `word[0x1577F0]` (the `dword[0x1577EE] >> 16` alias), the same word the
chaser writes at `0x70D17`/`0x70D3A`/`0x70D97`. The chaser's setter call then:

1. `FUN_000700F4` clears `byte[0x157821]` (`0x70153 MOV byte[0x157821],AH`,
   `AH = 0` from `0x70144 XOR AH,AH`) and `dword[0x1577CA]`
   (`0x70258 MOV dword[0x1577CA],EDX`, `EDX = 0` from `0x70251 XOR EDX,EDX`) —
   **the pan-counter reset that re-opens follow gate 1**;
2. clamps the height up to `camY` (`0x71CDB CMP EAX,EDX; JGE` →
   `0x71CDF/0x71CE5`) and caps it at `0x640`;
3. rebinds `[0x1577CA] = EBP` (`0x71D27`; `0x71C97 MOV EBP,EAX` — the setter's
   own record parameter).

So a chaser fire is structurally a full re-arm: `pan_counter = 0`,
`tracked = rec`, `event_param = max(ramped, camY)`. The limiting question is
whether the output height can be `> 0xF0`, and whether it can fire again.

**Height reachability (first-hand bytes + ported-table arithmetic).** The
camera-Y cell is the arc of the *same* event: `FUN_000736AC` writes
`[0x157750] = FUN_00070B94(event_param, timer)` (native `0x737A6..0x737B4`,
*carried* FU-171 §2.3; engine `camera.c:711-717`, `camera.c:683-698`), and
`FUN_00070544` calibrates `F2 = ramp(event_param)` so
the arc's maximum tracks the event height. For the live receive arm's `traj = 0x100`
event: `ramp(0x100) = 0x94 - T_ramp[0x150] = 148 - 89 = 59`; the arc's max is
`T_arc[0x94] - T_arc[0x94 - 59] = 1600 - 1347 = 253 = 0xFD` (the `0x10F4EE`
ramp and `0x10F3C4` arc tables are the carried engine ports, FU-148 §14.5 /
`camera.c:610-669`). So a normal 0x100 event's camera Y does cross `0xF0` near
its peak.

* **Reflect arm** (`pos_y + 0x68 > camY`, the common arm: the admission band is
  `camY ∈ [pos_y, pos_y+0x70]`, and the ramp arm owns only the top 8 units
  `camY ∈ [pos_y+0x68, pos_y+0x70]`): the output height **is** camY. It re-arms
  the follow iff `camY > 0xF0`, i.e. near the peak of a tall event's arc.
* **Ramp arm**: `target = pos_y + 0x70` is the fixed point;
  `nw = target + ((ev-target)>>1)` (a 3/4 step when `target < nw`). The
  admission gives `target ∈ [camY, camY+0x70]`, and `camY <= arc_max(ev) ~ ev`
  (the ramp/arc calibration above), so
  `nw <= (ev + camY + 0x70)/2 ~ ev + 0x38`. **The ramp can therefore lift an
  event whose height is already above `0xF0 - 0x38 = 0xC8` over the gate**
  (with the record at the top of the band), but a colder event (`ev <= ~0xC8`)
  can never reach `0xF0` from the chaser. The reflect arm cannot lift at all:
  it writes camY.

**Fire-again reachability.** `0x7C83E CMP EBP,EAX; JZ 0x7C901` excludes the
tracked record, and the chaser has just rebound `tracked := rec`. The same
record is therefore locked out on the next frame; only a *different* record
entering the `lane <= 0x10 && lane < bound` proximity band can fire it again.
`lane` is the `0x8DC68` distance from the camera focus to the record:
`0x7C782 AX = word[0x15774C]; SUB EAX,EDI (word[rec+0x59])` and
`0x7C792 AX = word[0x157754]; SUB EAX,EDX (word[rec+0x61])`, then
`0x7C7AA CALL 0x8DC68` → `0x7C7AF word[rec+0x6B]`; `bound` (`0x7C77E
word[rec+0x77] := old +0x6B`) makes the second gate an **approach** test.

---

## 5. Port shape

### 5.1 Already exact (no new port work; keep)

| native | engine | status |
|---|---|---|
| `FUN_00070C08` `0x70C08..0x70DDE` | `fifa96_camera_record_event`, `camera.c:1075-1174` | gates/arms exact; `0x8DC50` via `fifa96_camera_shift_step`, `0x8DC68` via `fifa96_entity_distance`; the two deltas are §5.2 items 1 and 3 |
| BF20 tail `0x7C7D3..0x7C8FC` | `match_run_record_camera_event`, `match_run.c:576-622` | all gates incl. `rec == tracked` skip and the `lane/bound` pair |
| per-record frame call | `match_run.c:1508` (inside `match_run_dispatch_entity`, after the lane block, before `e->pos_y` write-back) | wired |
| tracked rebind on apply | `match_run.c:619-620` → `fifa96_camera_set_tracked` | exact (`0x71D27`) |
| `0x92820(rec,0x1C)` sink | `out->sink = 1; sink_code = 0x1C` | request only (OL-27 family) |
| `0x71C94` reset subset | `fifa96_camera_event_set`, `camera.c:898-955` | resets `pan_counter`/`tracked`, clamps, then `camera_event_ramp` |

### 5.2 Deltas Stage-2 must land (fidelity, then the live rider)

1. **`camera.c:1161` — pass `ramp_param = 1`, not `0`.** Native `0x70DBA
   MOV ECX,1` survives the `0x92820` sink (PUSH/POP ECX, fresh) and is consumed
   by `0x71D60 MOVSX EAX,CX`. The engine's `0` takes the SUB arm of
   `FUN_00070544` (`0x705ae TEST DX,DX; JZ -> CWDE; NEG`). The wrong sign moves
   `F6/F8` and hence `timer`/the follow window: for a 0x100 event at
   `camY = 0x70`, the receive arm's `ramp_param = 0` (FU-171 §4.3) gives
   `F6 = 59 - ramp(0x90) = 14`, window `(118-14)/2 = 52` frames;
   `ramp_param = 1` would give `F6 = 59 + 45 = 104`, window `(118-104)/2 = 7`
   frames. For the chaser's own calls the height argument lies in
   `[camY, pos_y+0x70]`, so the `ramp(height - camY)` term spans
   `ramp(0..0x70) = 3..39`; in the reflect arm it is exactly `ramp(0) = 3`, so
   the live chaser window at `camY = 0xFD` is `(118 - (59+3))/2 = 28` frames
   (ported-table arithmetic; `0x100`/`0xFD` as §4).
2. **`match_run.c:584` — read the live `+0x5D`.** `in.height_5d` is
   `(int16_t)e->pos_y`, but `e->pos_y` is written at `match_run.c:1510`, *after*
   the `1508` call; the native chaser and its caller read the just-stepped
   `dword[rec+0x5D]` (the jump arc runs at `0x7C057..0x7C113`, *carried*
   `match_entities.h:124-136` / FU-153, before the lane block/camera event).
   Pass `r->pos_y` (the record being stepped) into the
   helper, or move the write-back ahead of the call.
3. **`camera.c:1093-1094` — truncate the seed difference to int16** before
   `fifa96_camera_shift_step`: the native stores the word and `0x8DC50`
   sign-tests `AX` (`CWDE`), so a 32-bit difference outside `[-0x8000,0x7FFF]`
   wraps; the engine's 32-bit subtract does not.
4. Keep `rec == tracked → return` (`match_run.c:599`) and the `lane <= 0x10 &&
   lane < bound` pair (`match_run.c:585-586`); they are the live reachability
   boundary, not a bug.

No new record field, no census move, no tape re-pin (the tape never dispatches
this path). `make check` should stay green: `tests/test_camera.c:922-1082`
pins the chaser's arms but only `timer_limit != 0` (it does not pin `timer`), so
the `ramp_param` fix needs a new assertion (FU306-L2).

---

## 6. GO/NO-GO (spec §8.4)

**GO — reachable now (Layer-1, staged-producer).** The chaser is already
implemented and already runs on the granted-frame path. With (a) a live tall
event (the receive arm's `traj = 0x100`, FU-171 §4.3; its camera arc peaks at
`0xFD`), and (b) a *second*, non-tracked, airborne record inside
`lane <= 0x10` with `lane < bound`, `pos_y <= camY <= pos_y+0x70` and in the
reflect sub-band `pos_y > camY - 0x68`, one chaser call re-arms the follow
window: `pan_counter = 0`, `tracked := that record`,
`event_param = camY > 0xF0`. (In the ramp sub-band `pos_y <= camY - 0x68` the
re-arm works when `ev > ~0xC8` and the record is high enough; §4.) A Layer-1 test may stage the producer
cells (camera focus, the second record's `pos_y`/lane) per §8.2 and assert the
re-arm (`pan_counter == 0`, `tracked == B`, `event_param > 0xF0`), then the next
granted frame's follow (`vel = dirs x 15`) — the FU171-L1 window shape.

**NO-GO — the numbered dead-end (continuous same-record re-arming).** The gate
`0x7C83E CMP EBP,EAX; JZ 0x7C901` skips the tracked record, and the chaser's
setter call rebinds `tracked := rec`; the same record cannot fire again while it
stays tracked. Combined with the `lane <= 0x10 && lane < bound` proximity/approach
admission and the height analysis (the chaser sustains, or at best lifts, a
near-tall event with `ev > ~0xC8`; it cannot bootstrap a cold one), "the chaser
holds the follow window open per frame" is **not** a live mechanism. Per spec §8.4 this is a numbered leg
(FU306-L1), **not** a blocker for the phase: do not make the chaser the primary
sustained-follow route; keep the receive event + the FU307 Y producer as the
driver, and use the chaser as the rider/re-arm proof.

---

## 7. Evidence table (fresh this session unless marked carried)

| # | claim | first-hand read |
|---|---|---|
| E1 | program identity / DB state | `get_current_program_info`: `/FIFA96.EXE`, `watcom:LE:32:default`, base 0, `.image`, **2708** functions (brief says 2706) |
| E2 | the chaser body, 132 insns | `disassemble_function 0x70C08` (`0x70C08..0x70DDE`) |
| E3 | exactly two call sites | `get_xrefs_to 0x70C08` = 2: `0x7C8FC`, `0x7C984` |
| E4 | `0x8DC50` = signed 16-bit `/2^n`, truncating | `disassemble_bytes 0x8DC50..0x8DC67` |
| E5 | `0x8DC68` = octagonal distance (`max + min/4`, or `max + 3*min/8` when `min > max/2`) | `disassemble_bytes 0x8DC68..0x8DCD2` |
| E6 | BF20 gate chain + the `rec == tracked` skip + the call | `disassemble_bytes 0x7C7B0..0x7C9BF` (528 B) |
| E7 | `0x8DC68` lane block: cam focus − record, `+0x6B`, `bound := old +0x6B` | `disassemble_bytes 0x7C740..0x7C7D7` |
| E8 | base record + 10-slot loop (11 records per team block), two team calls | `disassemble_bytes 0x8DAF0..0x8DB7F`; `get_function_by_address 0x8D8EC`; `get_xrefs_to 0x4B100` |
| E9 | setter body (`0x71C97 MOV EBP,EAX`, `0x71D27` bind, `0x71D60 MOVSX EAX,CX`) | `disassemble_function 0x71C94` (91 insns) |
| E10 | `0x70544` copies the camera triple to `0x157764..` | `disassemble_bytes 0x70544..0x705D3` |
| E11 | `0x92820` preserves ECX (`PUSH ECX`/`POP ECX`) and its nested call `0xCB2A4` is EAX-only (`MOV EAX,[0x112E88]; RET`) | `disassemble_function 0x92820` (27 insns), `0xCB2A4` (2 insns); `read_memory 0x110F1C` (64 B) |
| E12 | setter reset clears pan counter / tracked | `disassemble_bytes 0x70140..0x70167`, `0x70240..0x7026F` |
| E13 | follow call site + first arm read `[0x1577EE]>>16` | `disassemble_bytes 0x739B0..0x739EF`, `0x71DF4..0x71E38` |
| E14 | `0x1106E0` action table does not contain `0x7C90C` | `read_memory 0x1106E0` (256 B) |
| E15 | `0x7C90C` is pointer-less | `search_byte_patterns "0c c9 07 00"` = no match; `search_instructions operand "0x7c90c"` = none |
| E16 | engine chaser + call site + deltas | `camera.c:1075-1174`, `match_run.c:576-622`, `:1508-1510`, `:4382`; `tests/test_camera.c:922-1082` |
| E17 | `0x10F4EE`/`0x10F3C4` tables + the 0x100→0xFD arithmetic | *carried* FU-148 §14.5 / `camera.c:610-669` (ported-table arithmetic) |

---

## 8. Errata candidates (quoted) and numbered legs

**Errata candidates (append-only; prior docs are not rewritten).**

1. **FU-171 §4.4, ramp param.** Quote: "whether `ECX` survives `0x92820` (the
   `FUN_00070544` ramp-param sign) is not statically decidable from this window
   — the engine passes 0 (`camera.c:1086`), which is the fall-through arm (leg
   FU171-L4)." **Corrected first-hand:** `0x92821 PUSH ECX` / `0x9285F POP ECX`
   in `FUN_00092820` make the survival decidable — `ECX = 1` at
   `0x70DD1`; the engine's `0` (`camera.c:1161` now; `1086` is stale line
   drift) is the wrong arm. FU171-L4's sub-question is **closed**.
2. **FU-171 §4.4 / §7 rec 5, "per-frame chaser".** Quote: "the record-event
   tail is a **per-record, per-frame chaser** — the only native mechanism that
   re-arms the follow continuously". **Sharpened:** the call is per record per
   frame, but
   `0x7C83E CMP EBP,EAX; JZ 0x7C901` excludes the **tracked** record and the
   setter rebinds tracked to the chaser's record (`0x71D27`), so it re-arms by
   *hand-over*, once per record switch, not continuously for one record. The
   live continuous claim is FU306-L1.
3. **`fifa96_camera.h:408-409`** states the native setter is called with
   "ECX 0"; corrected as erratum 1 (the chaser's call is ECX 1; the receive
   arm's remains 0 per FU-171 §4.3).

**Numbered legs.**

* **FU306-L1 — continuous same-record re-arming (the dead-end).** The tracked
  record is excluded by `0x7C83E`, and the `lane <= 0x10 && lane < bound`
  proximity/approach gate plus the `pos_y <= camY <= pos_y+0x70` band make the
  chaser a hand-over/sustainer, not a per-frame window holder. Owner: the
  phase-29 visible-acceptance design (FU309) / the FU171-L1 window tests. Not a
  blocker (spec §8.4).
* **FU306-L2 — the chaser's `ramp_param` (1, not 0).** `camera.c:1161`; add a
  test pinning `timer`/`vel` for the chaser path (the current
  `tests/test_camera.c:922-1082` does not). Closes FU171-L4's sub-question.
* **FU306-L3 — the stale `height_5d` (one-frame lag).** `match_run.c:584` reads
  `e->pos_y` (written at `:1510` after the `:1508` call); the native reads the
  live `dword[rec+0x5D]` after the jump arc (`0x7C057..0x7C113`, *carried*).
  Owner: `match_run_dispatch_entity` ordering.
* **FU306-L4 — the 16-bit wrap of the seed correction.** The native stores the
  word and `0x8DC50` sign-tests `AX` (`CWDE`); the engine's 32-bit subtract
  (`camera.c:1093-1094`) skips the wrap. Edge case only.
* **FU306-L5 — the second caller `0x7C90C..0x7C98C`.** No xrefs, no Ghidra
  function, not in the `0x1106E0` table, no image pointer; a gate-subset variant
  (no lane pair). Reachability unproven; owner: the computed-target census of
  the `FUN_0007CA54`/`FUN_000782D0` family. It cannot widen live reach beyond
  the BF20 tail for tracked-eligible records.
* **FU306-L6 — the record-class/activity prerequisites.** The chaser is dead
  when the tracked record exists but is inactive (`tracked[+0x8D] == 0`) or is
  code 8/9 (`0x7C831/0x7C846`); the engine mirrors this (`match_run.c:598-600`)
  — carried, no action, but it gates any Layer-1 fixture (the tracked record
  must be an active, non-8/9 record).
* **FU306-L7 — the `0x92820(rec, 0x1C, ECX=1)` sink has no engine consumer.**
  OL-27 family, carried (`camera.h:413`); it does not gate the chaser.
* **FU306-L8 — camY is the same event's arc.** The chaser's "tall" admission
  (`camY > 0xF0` reflect, or a ramp target above `0xF0`) presupposes a near-tall
  `event_param` (the §4 lift bound `ev > ~0xC8`); the chaser cannot bootstrap a
  follow from a cold camera. The open-play camera-Y producer stays
  **FU307 / FU168-L5 / FU171-L4**'s residue.

---

## 9. Provenance (this session)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info` (1); `disassemble_function` `0x70C08` (132),
`0x71C94` (91), `0x92820` (27), `0xCB2A4` (2), `0x7C990` (55);
`disassemble_bytes`
`0x8DC50..0x8DC67`, `0x8DC68..0x8DCD2`, `0x7C740..0x7C7D7`,
`0x7C7B0..0x7C9BF` (528 B), `0x8DAF0..0x8DB7F`, `0x4B280..0x4B2BF`,
`0x70544..0x705D3`, `0x707A0..0x7080F`, `0x70140..0x70167`, `0x70240..0x7026F`,
`0x739B0..0x739EF`, `0x71DF4..0x71E38`, `0x7BF20` head (128 B);
`get_xrefs_to` `0x70C08` (2 fresh), `0x7BF20` (2 fresh), `0x8D8EC` (4 fresh),
`0x7CA54` (1), `0x782D0` (1), `0x4B100` (3), `0x7C990` (2), `0x7C90C` (0);
`read_memory 0x1106E0` (256 B), `0x110F1C` (64 B);
`search_byte_patterns "0c c9 07 00"` / `"0c c9 07"` (no matches);
`search_instructions operand "0x7c90c"` (none);
`get_function_by_address` `0x7BF20`, `0x782D0`, `0x8D8EC`, `0x7C990`,
`0x7C90C` (no function), `0x7C984` (no function).

Engine source read: `src/fifa96_loader/fifa96_camera.c` (600-699, 700-879,
880-999, 1000-1174), `include/fifa96_loader/fifa96_camera.h` (400-438),
`src/fifa96_loader/fifa96_entity_update.c` (185-229),
`src/fifa96_loader/fifa96_action_handlers.c` (325-360),
`src/fifa96_engine/fifa96_match_run.c` (540-622, 1300-1529, 1563-1634,
3878-3904, 4310-4337, 4360-4389), `include/fifa96_engine/fifa96_match_entities.h`
(60-150), `tests/test_camera.c` (919-1082).

Carried docs: FU-171 §3.1-§4.4/§5/§7 (FU171-L4), FU-168 §2.2/§6.2,
FU-148 §14.5 (the `0x10F4EE`/`0x10F3C4` ports), FU-147 §3.3 leg 7,
`docs/ENGINE.md` phase-28 register.

Status: recon complete; no repo file changed except this draft; no census move;
no golden touched; no git.

---

## Review erratum (wave review, 2026-10-21)

*Appended section: the frozen text above stands; every correction quotes it
first. Read-only review round — no Ghidra call was made here; the review's
fresh reads stand.*

**Verdict:** GO — the chaser is a GO as the native **per-record/per-frame
hand-over** re-arm (Layer-1, staged producer); the **same-record continuous
re-arm is NO-GO** (FU306-L1), and the NO-GO governs the "one-shot" wording.

1. **The setter call label is `0x70DD1`, not `0x70DCC`.** Quote (§2.4):
   "`0x70DCC  EAX = ESI; CALL 0x71C94            ; THE EVENT SETTER`".
   **Corrected:** `0x70DCC` is the label of the `MOV EAX,ESI` that loads the
   record argument; it sits exactly **5 bytes** (`0x70DD1 − 0x70DCC = 5`)
   before the call. The call site to cite is **`0x70DD1`**. Nothing else in
   §2.4 moves (`0x70DBA MOV ECX,1`, `0x70DBF CALL 0x92820`, `0x70DB4
   word[ESP] = AX`, and the `0x70DD6` bail label are unaffected), and the
   §5.2 item-1 port delta (`ramp_param = 1`) does not depend on the label.

2. **The "one-shot" wording, clarified — the NO-GO governs.** Quote (§1):
   "GO as a one-shot live re-arm (staged-producer Layer-1); NO-GO as the
   continuous per-frame re-arm". Read with §3.1 ("the tail **never fires for
   the tracked record**") and §4 item 3 ("rebinds `[0x1577CA] = EBP`
   (`0x71D27`)"), the two mechanisms must not be merged:

   * **same-record continuous re-arm — NO-GO.** `0x7C83E CMP EBP,EAX; JZ
     0x7C901` skips the record the setter has just rebound as tracked at
     `0x71D27`; that record cannot fire again while it stays tracked.
   * **per-record/per-frame hand-over — GO, the native shape.** The caller
     walks the records per granted frame (`0x8D8EC` → the base record plus the
     10 slots at `+0xB2*n`, `0x8DB35`/`0x8DB4D`), and every in-band record
     **other than the tracked one** may call `0x70C08`; its setter rebinds
     `tracked := rec` (`0x71D27`). The sustained mechanism is therefore a
     frame-by-frame *hand-over between records*, not a loop on one record.

   "One-shot" means **one shot per record per tracking epoch**, never "one
   shot per event". Stage-2 must not stage a same-record second fire and must
   not read §1's phrase as a single-fire mechanism.
