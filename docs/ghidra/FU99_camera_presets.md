# FU-99 — camera view presets and the setup callers

Follow-on to FU-97 (open leg 1: the `EDX` entry index). The argument of
`FUN_0004D7E8` is the **preset index** (`EAX`, saved to `EDX`; entry offset
`index*0x18`), and the three setup callers pass 0 (normal) or 7 (cinematic).
The `+0x48` array is exactly ten presets. Preset 0 and preset 7 are quoted.

Result in one line: **`FUN_0004D7E8(EAX = preset)` copies preset
`{x,y,z,yaw,pitch,angle}` from the camera-type array into every enabled
camera; `FUN_0004CAEC` applies preset 0, `FUN_0004CBA0`/`FUN_0004CC80`
preset 7 (the latter selecting camera 9).**

## 1. `FUN_0004D7E8` — the preset argument

Prologue (`0x4D7E8`):

```
0x4D7EE  MOV EDX,EAX            ; preset index
0x4D7F0  CALL 0x36B98           ; per-frame state (clobbers EAX)
0x4D7F5  MOV EAX,EDX
0x4D7F7  LEA EBP,[EAX*4]
0x4D7FE  MOV EDX,0x7508         ; camera table
0x4D803  SUB EBP,EAX
0x4D805  LEA ECX,[EDX+0x460]    ; end = 0x7968 (10 cameras)
0x4D80B  SHL EBP,3              ; EBP = index*0x18
```

so the earlier decompiler `extraout_EDX` is the caller's `EAX`, and the copy
per enabled camera (type `[cam+0xC] >= 0`) is

```
puVar4 = [[0x8B64 + type*4] + 0x48] + preset*0x18
cam+0x10 = puVar4[0] ; +0x14 = [1] ; +0x18 = [2]
cam+0x58 = puVar4[3] ; +0x5C = [4] ; +0x4C = [5]
```

## 2. The callers and their presets

* `FUN_0004CAEC` (`0x4CAF1 XOR EAX,EAX; 0x4CAF3 CALL 0x4D7E8`) — **preset 0**,
  then the home/away side block (`0x83CC`/`0x84BC`) into camera 5 and the
  180° yaw flips (`0x18000 - yaw & 0xFFFF`) of FU-97 §4.
* `FUN_0004CBA0` (`0x4CBAD` branch): when `FUN_00053D50() == 0x10`
  (`0x4CBAF MOV EAX,7; 0x4CBB4 CALL 0x4D7E8`) — **preset 7** plus
  `[[0x9A70]+8] = 1` and `FUN_0004CC98()`; the else branch repeats the
  side-block setup and calls `FUN_0004FDA4()`.
* `FUN_0004CC80` (`0x4CC80 MOV EAX,7; 0x4CC85 CALL 0x4D7E8`) — **preset 7**,
  then forces **camera 9** (`0x4CC92`-region: `[0x7DC8] = &0x78F8`,
  `(0x78F8-0x7508)/0x70 = 9`), copies the previous camera's six fields into
  it, and calls `FUN_0004F8C8(10, blockA, 0xF)` with block
  `0x872C`/`0x80B4`/`0x81A4` (side-dependent).

## 3. Array A — ten presets (object-4 `0x7E2C`, `0x18` stride)

The +0x48 pointer of type 0's descriptor (FU-97 §2) addresses exactly ten
entries (`(0x7F1C - 0x7E2C)/0x18 = 10`):

| preset | x | y | z | yaw | pitch | angle |
|--------|----|----|-----|--------|--------|--------|
| 0 | -100 | 488 | 1100 | 33900 | 3900 | 4608 |
| 1 | -2713 | 216 | 3006 | 45550 | 1037 | 3048 |
| 2 | -1020 | 260 | 1873 | 52414 | 1396 | 3464 |
| 3 | -2706 | 192 | -105 | 50129 | 972 | 3632 |
| 4 | 100 | 268 | -37 | 48909 | 2394 | 4976 |
| 5 | -562 | 264 | 3690 | 33344 | 1606 | 3104 |
| 6 | -4 | 276 | 1502 | 32800 | 1742 | 3545 |
| **7** | **-347** | **380** | **1325** | **62621** | **3852** | **4608** |
| 8 | -1388 | 3148 | 432 | 40244 | 6707 | 4608 |
| 9 | 302 | 729 | -2288 | 28207 | 1896 | 4608 |

Preset 0 is the default (yaw 186.2°, pitch 21.4°, angle 25.3° — the angle
whose `(w/2)·cot` ≈ screen width, FU-96/97); preset 7 is a yaw-rotated view
(344.3°) that camera 9 is switched to for the cinematic setup of §2; preset
8 is the high, steep view (y 3148, pitch 36.8°).

Arrays B (`0x7F1C`, first three entries read, angles constant 4608) and C
(`0x85AC`, angles 0 and 3430) belong to the other descriptor pointers
(`+0x4C`/`+0x50`); their roles stay open.

## 4. Closures / errata

* **FU-97 §8 leg 1** — closed: the entry index is a **preset selector**
  passed in `EAX` (0 for `FUN_0004CAEC`, 7 for `FUN_0004CBA0`/`FUN_0004CC80`);
  array A holds ten presets, not one entry per camera slot.
* **FU-96 §6 leg 1** — carried: `+0x4C` is written only here, from the
  preset's sixth dword; it changes only when a setup caller re-applies a
  preset.
* **FU-97 §8 leg 2/3** — descriptor field roles and the home/away blocks
  remain cited only.
* **FU-99 first issue (erratum)** — the first published table skipped entry 3
  (the read starting at `0x7E8C` is preset 4, not 3), shifting every row from
  3 on and mislabelling the high preset as 7 instead of 8. The table above
  is corrected with `read_memory 0x107E74` for preset 3.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `disassemble_bytes` 0x4D7E8 (40 B), 0x4CAEC,
0x4CBA0, 0x4CC80 (32 B each); `read_memory` 0x107E8C (144 B, presets 3..9 of
array A), 0x107E2C/0x107F1C/0x1085AC (FU-97); `decompile_function` 0x4CBA0,
0x4CC80, 0x4D7E8. Analysis-only: no port, capture-rig, ISO, or
Ghidra-project change.

## 6. Open legs

1. **Array B/C roles** (`+0x4C`/`+0x50` pointers) and which code selects
   them.
2. **`FUN_0004CC98`/`FUN_0004FDA4`/`FUN_0004F8C8(10, …, 0xF)`**: the
   camera-transition/animation players called after the preset apply.
3. **Presets 8/9**: stored (table above) but no caller was found using them.
4. **`FUN_00036B98`** (called at preset apply) and `FUN_0004B7D0`/
   `FUN_0004B818`/`FUN_0004B6FC` (side/branch selectors) are cited only.
