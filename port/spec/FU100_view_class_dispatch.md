# FU-100 — camera view classes and the handler dispatch

Follow-on to FU-96 §6 leg 3 / FU-97 §8 leg 4 (the `(&0x8B80)[i]` per-camera
handlers) and FU-99 §6 leg 1 (the `+0x48`/`+0x4C` entry arrays). Also resolves
the `switch (FUN_00053D50())` truncation that stopped the FU-97 §3 decompile of
`FUN_000505D0` (the "bad instruction data" block).

Result in one line: **camera dword 1 (`+0x04`) is the view class; `FUN_000505D0`
clamps it for the four-entry handler table at object-4 `0x8B80`
(`FUN_0004e834` / `0x4E3A8` / `0x4EC9C` / `FUN_0004db38`), maps class 3 to
camera slot 5, dispatches its `0..0x1F` state through the 32-entry jump table
at `0x50550`, selects the `+0x48`/`+0x4C` entry-array variant with the same
`FUN_0004B7D0`/`FUN_0004B6FC` condition pair as the `0x83CC`/`0x84BC` side
blocks, and calls the class handler from `0x51056` only when `ECX != 0`.**

## 1. `[view+4]` — the view class

The camera records of FU-96/FU-97 carry a second discriminator at `+0x04`
(dword 1), distinct from the type at `+0x0C` (dword 3). Static values for the
ten slots (`read_memory` 0x10750C, 0x10757C, 0x1075EC, 0x10765C, 0x1076CC,
0x10773C, 0x1077AC, 0x10781C, 0x10788C, 0x1078FC and the type fields at
0x107514, 0x107584, 0x1075F4, 0x107664, 0x1076D4, 0x107744, 0x1077B4,
0x107824, 0x107894, 0x107904):

| slot | `+0x04` class | `+0x0C` type |
|------|---------------|--------------|
| 0 | 0 | 0 |
| 1 | 1 | 1 |
| 2 | 2 | 2 |
| 3 | -1 | -1 |
| 4 | -1 | -1 |
| 5 | **3** | -1 |
| 6 | -1 | -1 |
| 7 | 1 | -1 |
| 8 | -1 | -1 |
| 9 | -1 | -1 |

`FUN_000505D0` reads it twice, with two different clamps (`0x505e8..0x5060c`):

```
0x505e8 MOV EBP,[0x7DC8]      ; view
0x505ee MOV EBP,[EBP+0x4]     ; class -> handler index
0x505f1 TEST EBP,EBP
0x505f3 JGE 0x505f7
0x505f5 XOR EBP,EBP           ; clamp < 0 to 0
0x505f7 MOV EDI,[0x7DC8]
0x505fd MOV EDX,[EDI+0x4]
0x50600 CMP EDX,0x3
0x50603 JNZ 0x5060c
0x50605 MOV EDI,0x5           ; class 3 -> camera slot 5
0x5060c MOV EDI,EDX           ; else slot = class
```

and the shared tail re-selects the slot (`0x51027..0x51042`):

```
0x51027 LEA EAX,[EDI*0x8] ; SUB EAX,EDI ; SHL EAX,0x4   ; EDI*0x70
0x51039 ADD EAX,0x7508
0x5103e CMP EAX,[0x7DC8]
0x51040 JZ 0x51047
0x51042 MOV [0x7DC8],EAX
```

So the class ordinal `{0,1,2,3}` enumerates the four usable slots
`{0,1,2,5}` (3 and 4 are skipped by the remap); disabled slots carry `-1`,
the same marker as the `type < 0` disable gate of `FUN_0004D7E8` (FU-97 §3).
Slots 5 and 7 hold a plausible class (3 and 1) with `type = -1`: the class
reads as a slot property while the type is the enable/parameter field — the
runtime writer of both is an open leg (§8).

## 2. The handler table and the call site

Object-4 `0x8B80` (storage image `0x108B80`) holds four stored code-object
offsets (`read_memory 0x108B80`, 16 B → `34e80300a8e303009cec030038db0300`):

| class | stored | + `0x10000` | image target |
|-------|--------|-------------|--------------|
| 0 | 0x3E834 | 0x4E834 | `FUN_0004e834` |
| 1 | 0x3E3A8 | 0x4E3A8 | unnamed (prologue only) |
| 2 | 0x3EC9C | 0x4EC9C | unnamed (prologue only) |
| 3 | 0x3DB38 | 0x4DB38 | `FUN_0004db38` |

The `+0x10000` resolve is the FU-88 method (stored code pointers and inline
`CS:` tables resolve through `+0x10000`); it is confirmed independently below
by the jump table at `0x40550` ↔ `0x50550` and by the two existing function
entries (`FUN_0004e834` 0x4E834..0x4EC99, `FUN_0004db38` 0x4DB38..0x4DDA4)
whose prologues sit exactly at the resolved addresses. The three other
classes' prologues are valid function heads (class 1: `PUSH ESI; PUSH EDI;
PUSH EBP; SUB ESP,0x18`; class 2: `PUSH ESI; PUSH EDI; PUSH EBP; SUB
ESP,0x28`).

The call is in the shared tail (`0x51047..0x51056`):

```
0x51047 TEST ECX,ECX
0x51049 JZ 0x5105d
0x5104b MOV ECX,dword ptr [ESP + 0x14]
0x5104f MOV EAX,[0x7dc8]
0x51054 MOV EDX,ESI
0x51056 CALL dword ptr [EBP*4 + 0x8b80]     ; FF 14 AD 80 8B 00 00
```

(`read_memory 0x51056` → `ff14ad808b000083`; the trailing `83` is the next
instruction.) The handler receives `EAX` = the current camera, `EDX = ESI`,
`ECX = [ESP+0x14]`; the guard's `ECX` is set by the case bodies (e.g.
`0x50859`/`0x50964` `MOV ECX,EBX`). The table has exactly four entries: the
lower-bound clamp only (`0x505f5`) means a class ≥ 4 would index the data
block at `0x8B90` (`0xA0`, 1, `0x64`, `0x78`, `0x1F4`) — no static producer
of such a class was found (open leg §8). The dword at `0x8B7C` = `0x1F4`
(500) is the class-3 distance constant (`+0xC8` at `0x5067e`, `+0x12C` in
state `0x1D`); `0x8B98`/`0x8B9C` = 100/120 are read by the class-3 handler.

## 3. The four handler classes

* **class 0 — `FUN_0004e834` (0x4E834..0x4EC99), chase camera.** First call
  `FUN_0004e248(param_2[0x10], param, cam[+0x3C])` produces the z step, clamped to
  `[cam[+0x3C]-0xB10, cam[+0x3C]+0xB10]`; a tracked entity from `param_2[0x14]`
  supplies the z window (`+0x18`/`+0x1C`) and x/y inputs to `FUN_0004d698`;
  yaw slews by `FUN_0004df34(cam[+0x4C])` (`+0x58`), pitch by
  `func_0x4e05c` (`+0x5C`), the `puVar6` input clamped to ±0x1620; `cam[+0x14]`
  and `cam[+0x4C]` ease toward the target; the tail copies
  `cam[+0x40/+0x44/+0x48] = cam[+0x34/+0x38/+0x3C]`.
* **class 3 — `FUN_0004db38` (0x4DB38..0x4DDA4), side camera.** The offset
  `param_1 += 100` and the `uVar6` value (100 or 120, the `0x8B98`/`0x8B9C`
  constants) are switched by `FUN_0006400c()`; position via
  `FUN_0004c7d0`/`0x4c77c`/`0x4c7f8` into `cam[+0x10]`/`cam[+0x18]` with x
  clamped ±0x420 and z clamped ±0x810; y eases toward the target by `>>3`;
  yaw/pitch slew with `FUN_00049388`-modulated steps (moduli
  `0x1E0000`/`0x3C0000`) and pitch snaps to `0x2000` for `(0x2000, 0x8000)`
  and to `0xF618` for `[0x8001, 0xF617]`; same tail copy.
* **class 1 — 0x4E3A8**: same prologue/entry shape as class 0 (first call also
  `FUN_0004e248` with `cam[+0x3C]`, `EBP = EDX+0x34`); body open.
* **class 2 — 0x4EC9C**: enters through `FUN_000a1a60` (the `sin/cos` builder
  of FU-96 §2) with `EDX = [EDX+0xC]`, `EBP = EDX+0x34`; body open.

The slot mapping makes the roles legible: class 3 is slot 5 — the side camera
whose position `FUN_0004CAEC` writes from the `0x83CC`/`0x84BC` blocks
(FU-97 §4) — and it is the branch that skips the type descriptor entirely
(`CMP EBP,0x3` / `JZ 0x5067e` at `0x5062e`).

## 4. The state switch — `FUN_00053D50` and the jump table

`FUN_00053D50` is a getter:

```
FUN_00053d50(void) { return DAT_0004e57c; }
```

storage image `0x14E57C` (data immediate + `0x100000`), static value 0.
`FUN_000505D0` stores it and bounds it (`0x506d6..0x50704`):

```
0x506d6 CALL 0x53d50
0x506df CMP EAX,0x1f
0x506e2 JA 0x51027                              ; default tail
0x506e8 SHL EAX,0x2
0x506fb MOV EDX,[ESP+0x1c]
0x50704 JMP dword ptr CS:[EDX + 0x40550]        ; 32-entry table
```

The table at code-object `0x40550` (storage image `0x50550`, 32 dwords) maps
the state to case entries (value + `0x10000`):

| state | entry | state | entry |
|-------|-------|-------|-------|
| 0x00 | 0x51027 (default) | 0x10 | 0x50E43 |
| 0x01 | 0x5070B | 0x11 | 0x51027 |
| 0x02 | 0x51027 | 0x12 | 0x5070B |
| 0x03 | 0x5083B | 0x13 | 0x50FFF |
| 0x04 | 0x50946 | 0x14 | 0x51013 |
| 0x05 | 0x51027 | 0x15 | 0x50ED9 |
| 0x06 | 0x50E43 | 0x16..0x1B | 0x51027 |
| 0x07 | 0x50A47 | 0x1C | 0x50FFF |
| 0x08 | 0x50C93 | 0x1D | 0x50FDF |
| 0x09 | 0x50B10 | 0x1E | 0x51027 |
| 0x0A | 0x51027 | 0x1F | 0x5105D |
| 0x0B | 0x50F6B | | |
| 0x0C..0x0F | 0x51027 | | |

Verified by disassembly: `0x5070B` (the camera re-select prefix
`&0x7508 + EDI*0x70` shared with the default, then a case continuation at
0x5072B), `0x5083B`/`0x50946` (`CMP EBP,0x1` — the class-1 camera gets the
plain re-select, else `[[ESP+0x20]] = 1` and `ECX = EBX`), `0x50FDF`
(`CALL 0x4F304` with `EDX = [0x8B7C]+0x12C` = 800 and EAX = camera, then the
epilogue). State 0x1F jumps straight to the epilogue `0x5105D`. The static
state 0 takes the default tail, which is where the handler call sits — the
getter's writer and the case bodies' semantics are open (§8).

`FUN_000505D0`'s callers are `FUN_0004cef4`, `FUN_0004cf7c`, `FUN_0004d2d4`
(all in the `0x4Cxxx..0x51xxx` camera band of FU-95 §1); `FUN_00053D50` is
also read by `FUN_0004cba0`, `FUN_0004ce34`, `FUN_0004fa36`.

## 5. The entry-array selection and the six descriptors

The descriptor pointer comes from the type table `[0x8B64 + type*4]` (FU-97
§2), then (`0x50645..0x50678`):

```
0x50645 MOV ESI,[ESI*4 + 0x8b64]      ; type descriptor
0x5064c LEA EAX,[ESI + 0x8]           ; [ESP+0x28] = target
0x50653 LEA EAX,[ESI + 0x44]          ; [ESP+0x20] = alt
0x5065a CALL 0x4b7d0
0x5065f TEST EAX,EAX
0x50661 JNZ 0x5066c
0x50663 CALL 0x4b6fc
0x50668 TEST EAX,EAX
0x5066a JZ 0x50675
0x5066c MOV EAX,[ESI + 0x4c]          ; [ESP+0x24] = +0x4C variant
0x50675 MOV EAX,[ESI + 0x48]          ; [ESP+0x24] = +0x48 variant
0x506cc MOV EAX,[ESP+0x20]
0x506d0 MOV dword ptr [EAX],0         ; descriptor +0x44 = 0
```

so `local_20 = (FUN_0004B7D0() == 0 && FUN_0004B6FC() == 0) ? [desc+0x48] :
[desc+0x4C]`, and `desc+0x44` is cleared. The class-3 branch makes the same
choice for the side blocks (`0x5069e..0x506ba`: both zero → `0x83CC`, else
`0x84BC`). `FUN_0004B7D0` reads a per-team byte flag
(`(&0x590CC)[team*0x835] == 0`); `FUN_0004B6FC` walks the same 0x835-stride
team tables with a match-state gate. So `+0x48`/`+0x4C` are the **two
condition-selected variants of the same entry-array slot**, not different
data roles — the clarification FU-99 §6 leg 1 asked for.

The six descriptors (image `0x10896C`/`0x1089C0`/`0x108A14`/`0x108A68`/
`0x108ABC`/`0x108B10`, 84 B each):

| type | image | [0] | [1] | [2] | [3] | `+0x48` | `+0x4C` | `+0x50` |
|------|-------|-----|-----|-----|-----|---------|---------|---------|
| 0 | 0x10896C | 0x340 | 0xFA0 | 3 | 0 | 0x7E2C | 0x7F1C | 0x85AC |
| 1 | 0x1089C0 | 0x340 | 0x1200 | 1 | 0 | 0x800C | 0x80FC | 0x862C |
| 2 | 0x108A14 | 0x340 | 0xD48 | 3 | 0xA7F8 | 0x81EC | 0x82DC | 0x85EC |
| 3 | 0x108A68 | 0x340 | 0xFA0 | 3 | 0 | 0x7E2C | 0x7F1C | 0x85AC |
| 4 | 0x108ABC | 0x340 | 0x1200 | 1 | 0 | 0x800C | 0x80FC | 0x862C |
| 5 | 0x108B10 | 0x340 | 0xD48 | 3 | 0xA21C | 0x81EC | 0x82DC | 0x85EC |

Types 0≡3 and 1≡4 are byte-identical; 2 and 5 differ only in dword 3
(0xA7F8 vs 0xA21C). The static cameras use only types 0/1/2 (slots 0/1/2), so
the duplicate descriptors have no static consumer (open leg §8). The dword
immediately before the type table, `0x8B60` = `0x85EC`, duplicates type 2/5's
`+0x50` pointer (open leg §8).

## 6. Closures / errata

* **FU-96 §6 leg 3 / FU-97 §8 leg 4 — closed.** The `(&0x8B80)[i]` handlers
  are the four-entry view-class table: `FUN_0004e834` (class 0),
  `0x4E3A8` (1), `0x4EC9C` (2), `FUN_0004db38` (3); the index is
  `[view+4]` clamped to ≥ 0 and the call is at `0x51056`.
* **FU-99 §6 leg 1 — partially closed.** `desc+0x48` and `desc+0x4C` are the
  two entry-array variants selected by `FUN_0004B7D0`/`FUN_0004B6FC` (the same
  condition that picks `0x83CC`/`0x84BC`); `desc+0x50` is not read by
  `FUN_000505D0` and stays unnamed.
* **FU-97 §3 — decompiler halt resolved.** The `switch (FUN_00053D50())`
  "bad instruction data" truncation is a 32-entry jump table at `0x50550`;
  the state getter is `FUN_00053D50` → `[0x4E57C]` (image `0x14E57C`).
* **Erratum — FU-96 §1** said "index = `[view+4]`; clamped `<= 0`, 3 → 5".
  Precise reading: the clamp is `if (< 0) → 0` (`0x505f1..0x505f5`), and the
  3 → 5 remap applies to the camera-slot index (`EDI`), while the handler
  index (`EBP`) keeps class 3 — the class-3 branch (`0x5062e`) is what makes
  slot 5 the side camera. FU-96's text is corrected by this pointer.

## 7. Provenance

Ghidra MCP on `/fifa96_le.bin`: `disassemble_function` 0x505D0 (531 insns);
`decompile_function` 0x4E834, 0x4DB38, 0x53D50, 0x4B7D0, 0x4B6FC, 0x4B818,
0x6401C; `disassemble_bytes` 0x4E3A8 (48 B), 0x4EC9C (48 B), 0x5070B (32 B),
0x5083B (32 B), 0x50946 (32 B), 0x50FDF (32 B); `get_function_callers`
0x505D0 (3), 0x53D50 (4); `read_memory` 0x108B80 (16 B), 0x108B64 (64 B),
0x108B60 (4 B), 0x10896C/0x1089C0/0x108A14/0x108A68/0x108ABC/0x108B10
(84 B each), 0x50550 (128 B), 0x40550 (128 B), 0x51056 (8 B), 0x505E8 (16 B),
0x10750C/0x10757C/0x1075EC/0x10765C/0x1076CC/0x10773C/0x1077AC/0x10781C/
0x10788C/0x1078FC (class, 4 B each), 0x107514/0x107584/0x1075F4/0x107664/
0x1076D4/0x107744/0x1077B4/0x107824/0x107894/0x107904 (type, 4 B each),
0x14E57C (4 B). Address mapping as FU-88 (data immediate `A` → storage
`A+0x100000`; stored code pointers and inline `CS:` tables → `+0x10000`).
Analysis-only: no port, capture-rig, ISO, or Ghidra-project change.

## 8. Open legs

1. **Handler bodies, classes 1 and 2** (0x4E3A8, 0x4EC9C): prologues only;
   class 1 enters through `FUN_0004e248` like class 0, class 2 through
   `FUN_000a1a60`.
2. **State semantics**: the 13 non-default case bodies and the writer of
   `[0x4E57C]` (static 0). `FUN_0006401c` (`DAT_00009a98 == 0x82`) gates the
   class-3 constant choice only.
3. **`FUN_0004B7D0`/`FUN_0004B6FC`**: the per-team tables at `0x590CC`
   (stride 0x835) and what side/condition the pair actually tests; the
   `0x835` selector sources `[0x74A4]` and `[0x57AAC]>>24`.
4. **Duplicate descriptors**: types 3/4/5 have no static camera; which runtime
   path assigns a type ≥ 3.
5. **`desc+0x50`/array C** (0x85AC/0x85EC/0x862C) consumers, and `0x8B60`
   (= 0x85EC) as the dword before the type table.
6. **Slots 5/7**: class 3/1 with `type = -1`; the runtime writers of `+0x04`
   and `+0x0C`.
7. **Class-range hazard**: the handler index is clamped only at the lower
   bound; a class ≥ 4 would call the data at `0x8B90` (`0xA0`). No static
   producer was found, so this is a note, not a claim of reachability.
8. **Cited-only helpers**: `FUN_0004e248`, `FUN_0004d698`, `FUN_0004df34`,
   `FUN_0004c7d0`/`0x4c77c`/`0x4c7f8`, `FUN_00049388`, `FUN_0004f304`,
   `FUN_000a2ad8`, `func_0x4e05c`.
