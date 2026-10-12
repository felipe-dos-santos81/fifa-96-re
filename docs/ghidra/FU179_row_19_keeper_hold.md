# FU-179 — row 0x19 keeper-hold body: the predictor block (OL-33) and the live-install arms

Provenance: M2 phase-15 wave **R27**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive
program argument). Every xref count, table byte and instruction below is a
fresh `read_memory` / `disassemble_bytes` / `get_xrefs_to` of **this session**
unless marked *cross-cited* (then the frozen FU doc is named). Engine/loader
source read first-hand. No writes: no rename, comment, label, function, script
or project save; `/FIFA96.EXE` untouched; no git commands. The alias traps are
applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`).

Scope: derive the row-`0x19` keeper-hold body remainder — the **predictor block
`0x74832..0x74ACC`** (FU-140 OL-33) — onto the ported `fifa96_keeper_hold_*`
helpers; every cell/gate written and read; the row's first-order install arms
(the row-04 `0x19` fallbacks) and its internal fallback (the weighted camera
fallback); and the T2 wiring recommendation.

Result in one line: **the row-`0x19` handler `0x746E4` is a pure record/camera
body — it writes only `rec[+0x4d/0x51/0x55]` (target triple), `rec[+0x7b]`,
`[0x157C44]`, `[0x157C48]`, and `[rec+0x9E]`/`[0x157C5D]`, installs nothing
itself, and its only downstream seam is the `CALL [rec+0x1C]` phase method
(the R23 phase seam). Its first-order install is already live in the engine
(row-04 records `install 0x19`), so flipping `fifa96_match_action_table[0x19]`
to a ported body moves the census `22 → 23`; the predictor block ports as one
new pure helper (`fifa96_keeper_hold_predict`) taking the camera triple and the
six `0x1577xx` lead/field words as caller arguments, exactly like the already
ported `fifa96_keeper_hold_fallback`.**

---

## 1. Fresh table / dispatch re-verification

Fresh `read_memory 0x1106E0` (180 B = 45 dwords) returns the action table; the
keeper slots are unchanged from FU-140 §1:

```
0x19 0x0746E4  0x1A 0x07662C  0x1B 0x076D28  0x1C 0x077728
0x1D 0x074EB0  0x1E 0x07550C  0x1F 0x076380
```

slot `0x19` = `e4 46 07 00` at `0x1106E0 + 0x19*4 = 0x110744`. Fresh
`get_xrefs_to 0x746E4` = **1** reference, and it is `DATA @ 0x110744` — i.e. the
only way into the handler is the `FUN_0007D9A4` installer storing
`dword[rec+0x18] = 0x1106E0[code]` (FU-137 §2 / FU-140 §1). There is **no direct
caller**; a ported body is reachable only through an install of code `0x19`.
Fresh `get_xrefs_to 0x7D9A4` = **77** call sites (matches FU-174 §1).

---

## 2. The row's first-order install arms (the fallback code `0x19`)

The engine's row-04 body installs `0x19` on the non-active arm. Fresh bytes,
`0x7E850..0x7E8D3`:

* `0x7E850 MOV ECX,1; 0x7E855 MOV EDX,4; 0x7E85A XOR EBX; 0x7E85C CALL 0x7D9A4`
  — the carrier arm's *first* install (code 4).
* `0x7E866 MOV AL,[EBP+0x8D]; XOR EBX; TEST AL,AL; JZ 0x7E879; MOV EAX,3;
  JMP 0x7E87E; 0x7E879 MOV EAX,0x19; 0x7E87E MOVSX EDX,AX; MOV EAX,EBP;
  0x7E883 JMP 0x7F133` — **code 3 when the record is active, else code `0x19`**
  (the FU-174 §2 "`active != 0 ? 3 : 0x19`" arm; the engine binds it at
  `fifa96_outfield.c:427-428`).
* `0x7E8B6 JG 0x7E92F; 0x7E8BC MOV ECX,1; 0x7E8C1 MOV EDX,0x19; 0x7E8CA CALL
  0x7D9A4` — **the inactive-record arm installs `0x19`** (engine
  `fifa96_outfield.c:440`); the `JG` skips it when the picked opponent lane is
  deeper than this record's lane.

So a live install of code `0x19` exists on two already-wired row-04 arms; no new
installer producer is needed for the census move. (The code-3 sibling arm lands
on row `03`, R25's task.)

---

## 3. The row body map (`0x746E4..0x74CB7`)

Fresh `disassemble_bytes 0x746E4` (350 B, 121 insns), `0x74832` (666 B, 206
insns), `0x74ACC` (340 B, 115 insns), `0x74C20` (120 B), `0x74C96` (100 B).
Frame `SUB ESP,0x4c`; the body is a cascade of camera/keeper gates with the
target triple at `rec[+0x4d]` (`x`), `+0x51` (`y`), `+0x55` (`z`).

### 3.1 Prologue + deepest-keeper tracker (`0x746E4..0x74743`)

* `0x746ED ECX=EAX` (rec); `0x746F9 byte[rec+0x9E]=1` (ran); `0x74700 AH=1;
  0x74707 byte[0x157C5D]=AH` (the CPU-keeper latch); constants staged
  `[ESP+0x38]=0x9F0`, `[ESP+0x44]=0xAE0`, `[ESP+0x40]=0xC0`.
* `0x7470D EAX=[ECX]` (team block); `EBX=[EAX+0x7C7]` (the team's deepest-keeper
  pointer); `0x74721 DX=[ECX+0x6B]; 0x74725 CMP DX,[EBX+0x6B]; JGE` skip;
  else `0x7472B [EAX+0x7C7]=ECX` — the signed `+0x6B` compare selects the
  deepest keeper (FU-140 §2); ported as `fifa96_keeper_hold_track`.
* `0x74731 CMP byte[ECX+0x98],0; JZ 0x74748`; else copy `rec[+0x59..]` (3 MOVSD)
  → `rec[+0x4d..]` and `JMP 0x74CAE` (epilogue): the "+0x98 latch already set"
  early-out (copy pos→target and return).

### 3.2 Control-slot arm (`0x74748..0x7478B`)

* `0x74748 EBP=[ECX+0x20]` (the control-slot pointer); `0x7474D JZ 0x7478C`
  when null. When a slot exists:
  * `0x7474F EAX=[0x157A49]>>24; CMP 1; JNZ 0x7476F` — human-side phase == 1 →
    copy pos→target, return (`0x74765`).
  * else `0x7476F EAX=ECX; EBX=[EBP+0x1E]>>24; EDX=[EBP+0x1D]>>24; CALL
    0x79C20` — the slot-direction placement (see §5.7).

### 3.3 Phase gate + guard (`0x7478C..0x74831`)

* `0x7478C EAX=[0x157A4A]>>24; CMP 2; JNZ 0x74C8E` — **phase != 2 → the
  no-slot tail** (§3.7); phase == 2 → the guard.
* `0x7479D..0x747B1 EAX=[ECX]; EBP=[EAX+0x7A6]; EDX=[0x157A83]; EBP=[EBP+0x7B2]`.
* Guard rejections each jump to the predictor `0x74832`:
  `0x747B3 JNZ` (`[0x157A83] != 0`, an actor is already bound),
  `0x747C5 JGE` (`(rec[+0x69]>>16) >= 0x2D0`, the lane is too deep),
  `0x747D2 JNZ` (`[0x157750] != 0`, ball height nonzero),
  `0x747DC JNZ` (`team[+0x7C7] != rec` — this record is not the deepest keeper),
  `0x747EA JGE` (tracked opponent's `+0x6B` >= rec's `+0x6B`).
* Guard pass `0x747EC..0x74831`: `ESI=0x15774C`; copy the camera triple →
  `rec[+0x4d..]`; `EAX=[ECX+0x55]; abs; CMP 0xAE0; JLE 0x74821`; else clamp
  `rec[+0x55]` to `-0xAE0` (team side 0) or `+0xAE0` (nonzero) — this is
  `fifa96_keeper_hold_guard` + `fifa96_keeper_guard_clamp` (FU-140 §3.4/§3.5);
  then `0x74823 CALL 0x79B58(rec)` and return.

### 3.4 Predictor (`0x74832..0x74ACC`) — OL-33, see §4.

### 3.5 Carrier dive (`0x74ACC..0x74B71`)

Reached by every predictor bail (`JZ/JNZ/JGE` into `0x74ACC`) and by the
predictor fall-through.

* `0x74ACC TEST EBP,EBP; JZ 0x74B72` (no slot → weighted fallback);
  `0x74ADA CMP EBP,[0x157A83]; JNZ 0x74B72` (slot must be the bound actor).
* `0x74AE2 EAX=[[ECX+0x4]+0xB]>>24`; `EDX=[ECX+0x69]>>16`; `EAX=(carrier_type<<4)
  + 0x1E0`; `CMP EDX,EAX; JGE 0x74B72` (lane past the carrier gate → fallback).
* `0x74AFD..0x74B26` push `[0x15774C/0x157750/0x157754]/rec` and
  `CALL 0x74584`, then `CALL 0x745EC(EDI=[0x157A83], EDX=team)`; either false →
  fallback (see §5.5/§5.6).
* `0x74B28 EAX=[ECX+0x69]>>20; EDX=[EDI+0x71]>>16; EDX*=EAX; [ECX+0x4d]=
  [0x15774C]+EDX;` then the same for `+0x73` → `[ECX+0x55]`; `[ECX+0x51]=0`;
  `0x74B63 CALL 0x79B58(rec)`. This is `fifa96_keeper_hold_intercept`
  (`out.x = cam.x + (lane>>20)*carrier_vx`, `y=0`, same for z; FU-140 §3.4).

### 3.6 Weighted camera fallback (`0x74B72..0x74C8D`)

* Base (the fallback's **z**): `side = byte[[ECX]+0x826]`; `side==0` → base
  `-0x9F0`, else `+0x9F0` (`0x74B78..0x74B95`); `base += FUN_0008DC50(
  [0x157752]>>16, 4)` (`0x74B99..0x74BB0`, a truncating shift); clamp
  `±[ESP+0x44] = ±0xAE0` (`0x74BBA..0x74BCC`).
* x gate `0x74BD0..0x74BFE`: `lane=[ECX+0x69]>>16`; if `lane >= 0x780` or
  `|[0x157778]| <= 0x9F0` or `word[0x1577C2]==0` → the weighted arm; else
  `CALL 0x74694(base)` → x, clamped `±0xC0` (`0x74C10..0x74C2D`, both signs).
* weighted arm `0x74C3A..0x74C7A`: `w = (int16)word[0x15774C]`; `w >= 0` →
  `x = (w>>3)+(w>>4)+(w>>5)`; negative → negate the 16-bit word, sum shifts,
  negate the result (the native 16-bit wrap: `w = -0x8000` → `+0x1C00`).
* `0x74C7C` copy the `{x,0,z}` triple → `rec[+0x4d..]`. This is
  `fifa96_keeper_hold_fallback` (FU-140 §3.2/§3.4), fresh-confirmed byte for
  byte, including `FUN_00074694` (§5.4).

### 3.7 No-slot phase!=2 tail (`0x74C8E..0x74CB7`)

* `0x74C8E LEA EDX,[ECX+0x4d]; EBX=-1; EAX=ECX; 0x74C98 CALL dword[ECX+0x1c]` —
  **the per-record phase-handler method pointer** (the R23 act/phase seam);
  `0x74C9B EAX=[0x157A49]>>24; CMP 1; 0x74CA8 word[ECX+0x7b]=1` if human-side
  phase == 1; `0x74CAE` epilogue.

This is the row's **only downstream seam** and the hook R23 must give a live
caller. The row installs no code itself (no `CALL 0x7D9A4` anywhere in
`0x746E4..0x74CB7`).

Adjacent, **not** part of row 19: `0x74CB8` (0 xrefs — dead helper) and
`0x74CDC` (fresh `get_xrefs_to` = 1, the caller `0x75A82` inside row `1E`).

---

## 4. The predictor block `0x74832..0x74ACC` (OL-33)

Entry: `0x74832 MOV EDX,1; MOV EAX,0x90; CALL 0x71C40; CWDE; [ESP+0x3c]=EAX`.
This is a **camera-relative intercept projector**: it builds up to three
candidate placement triples (`FUN_00071B9C`) and arbitrates them into the
target. Cells read: `[0x15774C]` (cam.x), `[0x157750]` (ball/height y),
`[0x157754]` (cam.z), `[0x157778]`, `[0x1577BE]`, `[0x1577C0]`, `[0x1577C2]`
(the camera **velocity leads**), `[0x1577EE]`, `[0x1577F0]` + `[0x1577F2]`
(field half-width), `[0x1577FA]`, `[0x157821]`, `[0x157A83]`, and the record
`rec[+0x59]`/`+0x61` (pos x/z) and team `byte[[rec]+0x826]` (side). Cells
written: `[0x157C44] = -1` then the projection scalar, `[0x157C48..]` (the
shared predicted-intercept latch), and `rec[+0x4d/0x51/0x55]`.

Gate cascade (all fresh):

1. `0x7483C CALL 0x71C40(0x90,1)` → an angle from the field half-width;
   `0x74841 CWDE; [ESP+0x3c]=EAX`; `0x74846 MOV EAX,[ESP+0x3a]` — **the alias
   trap** (a 4-byte read straddling the 2-byte-low store, so it yields
   `(int16)angle` under the `SAR 0x10` at `0x7484C`); `0x7484F CALL
   0x71B9C(AX, dest=ESP)` → triple A in `[ESP]` (the "shadow" placement).
2. `0x74854..0x74875`: `AL = ([0x157754] < 0); EDX = AL&0xff; EAX = ([rec]&0xff
   side)`, `EAX ^= EDX`; **if the sign of cam.z equals the team side → bail to
   the carrier dive** (`0x74ACC`).
3. `0x7487B..0x7488D`: if `word[0x1577C0]==0 && word[0x1577C2]==0` → bail (no
   leads → the row correctly degenerates to the ported carrier/fallback).
4. `0x74893..0x748A3`: if `[0x157750] <= 0` require `byte[0x157821] != 0`, else
   bail.
5. `0x748A9..0x748B0`: if `[0x157A83] != 0` → bail (an actor is bound).
6. `0x748B6..0x748EF`: `[0x157C44] = -1`; stash `cam.x`, `([0x1577BE]>>16)`
   (lead_x), `rec[+0x59]`, `cam.z` in `EBX`, `rec[+0x61]`, `([0x1577C0]>>16)`
   (lead_z, in `ESI`), and `byte[ESP+0x48]=0` (the "fallback projection used"
   flag).
7. `0x748F8..0x74949`: require `|cam.z| < 0xB10` **and** `|[0x157778]| < 0xB10`;
   then require `sign(cam.z) == sign(lead_z)` and `lead_z != 0`; else skip to
   step 11.
8. `0x74950..0x74987`: `EDI = (0xB10 - |cam.z| - 8) / |lead_z|` (signed `IDIV`);
   require `EDI < 2*([0x1577F0]>>16)`; else skip.
9. `0x74989..0x749A2`: project `EDI` via `FUN_00071B9C(dest=ESP+0x18)` (triple
   B); `byte[ESP+0x48]=1`; if `[ESP+0x1c] > 0xC0` → bail.
10. (fall to step 11).
11. `0x749A8..0x749D5`: **the orthogonal projection scalar**
    `EBX = (lead_x*(rec.x-cam.x) + lead_z*(rec.z-cam.z)) / (lead_z² + lead_x²)`
    (the `IDIV` at `0x749D3`).
12. `0x749D7..0x749E4`: require `0 < EBX < 0x96`; else bail.
13. `0x749EA..0x749EF`: project `EBX` via `FUN_00071B9C(dest=ESP+0xc)` (triple
    C, the predicted intercept).
14. `0x749F4..0x74A55` (arbitration): if `|tripleC.z| > 0xB08` and the
    `[ESP+0x48]` flag is set → replace triple C with triple B and
    `EBX = EDI` (`0x74A12..0x74A1E`); if `tripleC.y > 0x90` and `([0x1577EE]
    >>16) > 0xC0` and `|tripleA.z| < 0xB08` → replace triple C with triple A
    and `EBX = [ESP+0x3c]` (`0x74A38..0x74A55`).
15. `0x74A56..0x74A82`: bounds gate on triple C: if `|C.x| > 0x130` go to the
    side test; else require `|C.z| < 0x990`, else go to the settle.
16. `0x74A84..0x74AA4`: side test `([0x157821]>>24) == team[+0x826]`; mismatch
    → bail.
17. `0x74AA6..0x74AC1`: `[0x157C44] = EBX`; copy triple C → `[0x157C48..]`; copy
    triple C → `rec[+0x4d..]`; epilogue.

**Degenerate case.** With all six `0x1577xx` leads/fields zero (the engine's
staged-zero camera convention, FU-172/OL-68-style), step 3 bails immediately to
`0x74ACC`, so the predictor is a no-op and the row runs the ported carrier dive
/ weighted fallback. That is what makes a staged-zero partial wiring honest.

---

## 5. Helper derivations (fresh bytes this session)

### 5.1 `FUN_00071C40` (`0x71C40..0x71C91`)
Angle limit: `BX=word[0x1577F0]`; if `BX<=0` or `word[0x1577F2]<=0` → 0; else
`EBX = BX - EAX`; `CALL FUN_000702F8(BX-EAX)` (a trig sign helper); if `DX==0`
negate; `EAX = EAX + ([0x1577F0]>>16) - word[0x1577FA]`; if `AX < 0` → 0. The
`FUN_000702F8` internals stay a leg.

### 5.2 `FUN_00071B9C` (`0x71B9C..0x71C3B`) — the angle→offset projector
`MOV [ESP],AX` (angle); `dest = EDX`; copy the camera triple `[0x15774C..]` →
`dest`; if `word[0x1577F0] > 0`: `dest.y = FUN_00070B94(angle +
word[0x1577FA])`; if `dest.y < 0` set `dest.y = -(dest.y>>1)` and stage
`lead_x = trunc(word[0x1577C0])`, `lead_z = trunc(word[0x1577C2])` via
`FUN_0008DC50(_,1)`; finally `dest.x += (int16)0x1577C0 * angle` and
`dest.z += (int16)0x1577C2 * angle` (`0x71C13..0x71C31`). Used by rows 0E/0F/19
(shared camera stand-in; FU-139/FU-142 OL-72). The sine/height body
`FUN_00070B94` and the `[ESP-2]` aliased read at `0x71C13` stay legs.

### 5.3 `FUN_00074694` (`0x74694..0x746E2`) — the projection used by the fallback
`num = |base| - |[0x157754]|`; `den = ([0x1577C0]>>16)<<4`; `quot = num/|den|`
(signed `IDIV`, `den==0` faults); `lead = ([0x1577BE]>>16)<<4`; return
`[0x15774C] + lead*quot`. **Fresh-confirmed** FU-140 §3.2.

### 5.4 `FUN_0008DC50` (`0x8DC50..0x8DC67`) — truncating shift
`if AX>=0: EAX = (int)AX >> CL else EAX = -((- (int)AX) >> CL)` — a
sign-symmetric (toward-zero) arithmetic shift; matches the `>>20`/`>>4`
semantics FU-140 pinned.

### 5.5 `FUN_00074584` (`0x74584..0x745E9`) — carrier gate A (4 args, `RET 0x10`)
Args `(cam.x, ball.y, cam.z, rec)` (pushed `0x74AFD..0x74B12`). `thr =
2*(0x10 - byte[team+0xC]) + 0x3C0`; if `|cam.x| > thr` → 0; else the side arm:
side==1 requires `0xB10-thr < cam.z`; side!=1 requires `-(0xB10-thr) < cam.z`.
Returns bool. Callers: `0x74B12` (row 19) + `0x771DA` (row 1B).

### 5.6 `FUN_000745EC` (`0x745EC..0x74690`) — carrier gate B (`rec`, `team`)
Returns 1 when this record is the deepest keeper on its side (`|rec[+0x61]| >
0x930` short-circuits, else the `[EAX+0xb2]` 11-record walk keeping the max/min
`+0x61` by side, compared against `rec[+0x61]`). Single caller `0x74B1F`
(row 19).

### 5.7 `FUN_00079C20` (`0x79C20..0x79C4E`) — slot-direction placement
`EDX = (int16)dir_x<<7`, `EBX = (int16)dir_z<<7`; `rec[+0x4d] = rec[+0x59]+EDX`;
`rec[+0x51]=0`; `rec[+0x55]=rec[+0x61]+EBX`; `CALL 0x7D3E4` (the clamp); fresh
`get_xrefs_to` = 5 (row-19 `0x7477D` + `0x7DB5F/0x7E26F/0x7EC0E/0x805F4`).

### 5.8 `FUN_00079B58` (`0x79B58`) — the receiver timer arm
`if byte[rec+0x99]==0: byte[rec+0x93]=0x10`; fresh `get_xrefs_to` = 17
(incl. row-19 `0x74823`/`0x74B63`).

---

## 6. T2 wiring recommendation (engine)

The row body is **pure record + camera math** and installs nothing; the pool
already exists (rows 1D/1E wired, FU-151) and the install producer is live
(row-04 → `0x19`, §2). Recommended, ranked:

1. **Add `fifa96_keeper_hold_predict(...)` to `src/fifa96_loader/fifa96_keeper.c`**
   — the OL-33 remainder, derived from §4. Follow the `fifa96_keeper_hold_fallback`
   precedent exactly: take the camera triple + the six lead/field words
   (`cam_lead_x = [0x1577BE]`, `cam_lead_z = [0x1577C0]`, `[0x1577C2]`,
   `field_half = [0x1577F0]`, `[0x1577FA]`, `[0x157778]`, plus `[0x1577EE]` and
   `[0x157821]`) as **caller arguments**, not globals, and `dest = &out`. Return
   the `[0x157C44]` scalar + the `[0x157C48]` triple + a `predicted` flag; when
   the leads are zero it must return `predicted = 0` (the §4 step-3 bail), so the
   caller falls through to the carrier/fallback. Add fixtures to
   `tests/test_keeper_bodies.c` (empty leads, the sign/bounds gates, the
   `|C.x|>0x130`/`|C.z|>0x990` bounds, the divider edge where `lead_z==0`).
2. **Wire `fifa96_match_action_table[0x19]`** (`fifa96_match_handlers.c:3449`):
   a `fifa96_match_action_19` that unpacks the record into `fifa96_keeper_point`
   and binds the already-ported arms in body order — prologue + `hold_track`
   (`0x746F9..0x7472B`), the `+0x98` copy-out (`0x74731`), the `[rec+0x20]`
   slot arm via `hold_intercept`-style slot math or the `0x79C20` placement, the
   phase-2 guard via `hold_guard` + `guard_clamp` (`0x7479D..0x74831`), the
   predictor via the new helper, the carrier dive via `hold_intercept`
   (`0x74ACC..0x74B71`), the weighted fallback via `hold_fallback`
   (`0x74B72..0x74C8D`). The record struct already carries
   `pos_*`/`target_*`/`lane`/`stage`/`ran`/`has_slot`; add the trailing
   `+0x98` latch and the camera-lead staging fields (write-only, like the row-1E
   `place_*`/`helper_request` sinks).
3. **The `[rec+0x1C]` phase method (`0x74C98`)** — the row's only downstream
   seam — becomes a request flag (e.g. `phase_handler` / `word[+0x7b]`), **not**
   an invented call; it is the exact hook R23's `FUN_000888FC`/`[0x158808]`
   act/phase seam must light up (FU174-L1). Keep it a disclosed unwired request
   until R23 lands.
4. **Census:** flipping the table row moves dispatch to `23 OK / 56 UNSUP /
   1 NOTF`, i.e. `floor(100 × 23/80) = 28%` (README title + ENGINE.md census
   line in the same commit; FU-174 §6 rank 5).

The predictor's live camera-velocity producers (`0x1577BE/C0/C2`, `0x157778`,
`0x1577EE`, `0x1577EE/821`) remain the engine's existing camera stand-in legs
(the OL-68/OL-72 class) — the row is honest as a staged-zero partial.

---

## 7. Numbered legs

1. **FU179-L1 — the predictor's live lead producers.** `[0x1577BE]`,
   `[0x1577C0]`, `[0x1577C2]`, `[0x157778]`, `[0x1577EE]`, `[0x1577F0]`,
   `[0x1577FA]`, `[0x157821]` have no ported engine producer; the engine stages
   zero. The predictor pure function ports now (§6.1); its producers are the
   OL-68/OL-72 camera class.
2. **FU179-L2 — the trig/height internals.** `FUN_000702F8` (the `0x71C40`
   sign helper) and `FUN_00070B94` (`0x71B9C`'s angle→height) are read at their
   call sites only; their tables/series are unread here.
3. **FU179-L3 — the `FUN_00071B9C` aliased store/read.** `MOV [ESP],AX` then
   `MOV EAX,[ESP-2]`/`[ESP+2]` reads (e.g. `0x71C13`, `0x74846`) are the
   documented `dword[x]>>16` alias class; the port must reproduce the
   `(int16)` truncation, not a clean 32-bit load.
4. **FU179-L4 — `FUN_00079C20`'s `0x7D3E4` clamp and `FUN_000745EC`'s 11-record
   walk** are not derived here (only the call bytes and the `[EAX+0xb2]` stride
   are fresh); if the slot arm is wired, re-read before porting.
5. **FU179-L5 — the `[rec+0x1C]` phase method.** The `CALL dword[ECX+0x1c]` at
   `0x74C98` is row 19's only downstream seam; it stays unported until R23's
   act/phase call site exists (FU174-L1).
6. **FU179-L6 — legs from FU-140 OL-33 not re-derived here.** The
   `0x57C44/0x57C48` latches are now fresh-confirmed as
   `[0x157C44]`/`[0x157C48]` (the intercept scalar + triple); the remaining
   OL-33 sub-legs (the `FUN_00079C20` slot-move body, the carrier-dive
   predicates' internal bands) carry as above.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`read_memory 0x1106E0` (180 B); `get_xrefs_to` `0x746E4` (1), `0x7D9A4` (77),
`0x79C20` (5), `0x79B58` (17), `0x74CDC` (1), `0x74CB8` (0), `0x74584` (2),
`0x745EC` (1); `disassemble_bytes` `0x746E4` (350 B, 121 insns),
`0x74832` (666 B, 206 insns), `0x74ACC` (340 B, 115 insns),
`0x74C20` (120 B), `0x74C96` (100 B), `0x74694` (80 B),
`0x71C40` (120 B), `0x71B9C` (160 B), `0x74584` (108 B), `0x745EC` (168 B),
`0x79B58` (200 B), `0x79C20` (200 B), `0x7E850` (132 B), `0x8DC50` (24 B).
Engine/loader source read: `src/fifa96_loader/fifa96_outfield.c` (400..459),
`src/fifa96_loader/fifa96_keeper.c` (120..199),
`include/fifa96_engine/fifa96_match_run.h` (395..479),
`src/fifa96_engine/fifa96_match_handlers.c` (3404..3457).
Docs cross-cited: FU-79 §2, FU-136 §1, FU-137 §2/§6.1, FU-138, FU-139 §9,
FU-140 §1/§2/§3.2/§3.4/§6, FU-141 §5, FU-142 Appendix K.5, FU-164, FU-174 §2/§6,
the phase-15 R27 brief. No writes beyond this slice file and the workspace
report.
