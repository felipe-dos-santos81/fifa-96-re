# FU-97 — camera type tables and the `+0x4C` angle source

Follow-on to FU-96 (open legs 1 and 3). This slice follows the camera
`+0x4C` angle — the input of the ratio path — to its writer and source: a
static camera-type table. The writer is `FUN_0004D7E8` alone; the value is
a per-type/per-entry tilt angle in the entry arrays behind `[0x8B64]`.

Result in one line: **`FUN_0004D7E8` copies `{x,y,z,yaw,pitch,angle}` from a
camera-type entry array into all enabled camera objects; `+0x4C` is the
entry's sixth dword (static values 4608/3048/3464 ≈ 25.3°/16.7°/19.0°), so
the ratio path is a per-camera-type constant, not a per-frame value.**

## 1. The camera arrays

`FUN_0004D7E8` (`0x4D7E8`) walks `&0x7508` in `0x70` steps until `0x7968`:
**10 camera objects** at object-4 `0x7508 + i*0x70`. The runtime state array
follows at `0x7968` (10 records of 0x70, the second loop initialises entry
`i` and zeroes `+0x34/0x38/0x10/0x14/0x18/0x1C` of each type descriptor),
and `[0x7DC8]` sits at `0x7DC8 = 0x7968 + 10*0x70` — the current-camera
pointer of FU-95/FU-96.

Camera record fields (dword indices):

| dword | byte | role |
|-------|------|------|
| [3] | +0x0C | camera type (0..5; `< 0` = disabled) |
| [4..6] | +0x10..0x18 | x, y, z |
| [0x13] | +0x4C | ratio angle |
| [0x16] | +0x58 | view-matrix yaw (FU-88) |
| [0x17] | +0x5C | view-matrix pitch (FU-88) |

`FUN_0004D498(EAX = index)` clamps the index to `0..9` and sets
`[0x7DC8] = &0x7508 + index*0x70`.

## 2. The type table `[0x8B64]`

Image `0x108B64` holds six object-4 pointers then two non-pointer dwords:

```
0x896C 0x89C0 0x8A14 0x8A68 0x8ABC 0x8B10 0x1F4 0x3E834
```

`FUN_000505D0` indexes it with the camera's type dword:
`[0x8B64 + [view+0x0C]*4]`. Type 0's descriptor (image `0x10896C`, dwords)
starts `{0x340, 0xFA0, 3, 0, 0xC8, 0, 0x578, 0x20, 8, 8, 8, 8, 0xEA6, 0, 0,
0x258, 0x50, 0, 0x7E2C, 0x7F1C, 0x85AC, 0x340, 0x1200, 1, ...}` — three
entry-array pointers at `+0x48`, `+0x4C`, `+0x50`.

Entry arrays are object-4; entries are 6 dwords, stride `0x18`:

```
{x, y, z, yaw, pitch, angle}
```

read at image addresses:

* `0x7E2C` (image `0x107E2C`): `{-100, 488, 1100, 33900, 3900, 4608}`,
  `{-2713, 216, 3006, 45550, 1037, 3048}`,
  `{-1020, 260, 1873, 52414, 1396, 3464}`, ...
* `0x7F1C`: same first entry, then `{..., 46675, 4749, 4608}`,
  `{..., 51827, 4512, 4608}` (constant angle 4608)
* `0x85AC`: `{150, 150, 1000, 3800, 0, 0}` (angle 0),
  `{1330, 1750, 450, 450, 1600, 3430}`, ...

## 3. `FUN_0004D7E8` — the only `+0x4C` writer

```
for each camera puVar2 (0x7508 .. 0x7968 step 0x70):
  if (puVar2[3] >= 0):                      ; type enabled
    puVar4 = [[0x8B64 + puVar2[3]*4] + 0x48] + EDX*0x18
    puVar2[4]  = puVar4[0]  ; +0x10 x
    puVar2[5]  = puVar4[1]  ; +0x14 y
    puVar2[6]  = puVar4[2]  ; +0x18 z
    puVar2[0x16] = puVar4[3] ; +0x58 yaw
    puVar2[0x17] = puVar4[4] ; +0x5C pitch
    puVar2[0x13] = puVar4[5] ; +0x4C angle      ← the ratio input
  puVar2[0xF] = puVar2[0x12] = puVar2[0xE] = puVar2[0xD] = puVar2[0x11] =
  puVar2[0x10] = 0
[0x7784] = [0x7BE4] = 4000
```

The `+0x4C` write at `0x4D836` is the **only** static reference to camera 0's
slot (image `0x107554`, `get_xrefs_to 0x7554`); all ten cameras carry the
static default 21 (`0x107554`, `0x1075C4`, ..., `0x107784`, `0x1077F4`,
`0x107864`, ...), overwritten by this copy at camera setup. So the ratio
angle is a **camera-type parameter**, constant between camera switches — the
`FUN_00044394` cache (`0x67AC`/`0x67B0`) rebuilds the reciprocal tables only
on change, consistent with FU-96 §1.

`EDX` (the entry index) is the caller's argument: the three callers are
`FUN_0004CAEC`, `FUN_0004CBA0`, `FUN_0004CC80`.

## 4. `FUN_0004CAEC` — the side-swap setup

```
FUN_0004D7E8()
if (FUN_0004B7D0() == 0):  block = 0x83CC
else:                      block = 0x84BC
cam5[+0x10/0x14/0x18] = block[0..2]        ; 0x7748/0x774C/0x7750
[0x7784] = block size word
if (FUN_0004B818() != 0):                   ; side swap
    [0x75D0] = 0x18000 - [0x75D0] & 0xFFFF  ; cam1 yaw += 180°
    [0x7590] = -[0x7590]                    ; cam1 z flip
    [0x7750] = -[0x7750]                    ; cam5 z flip
    [0x7790] = 0x18000 - [0x7790] & 0xFFFF  ; cam5 yaw += 180°
FUN_0004CEF4()
```

(`0x7748 = 0x7508 + 0x240` = camera 5 `+0x10`; `0x75D0 = 0x7508 + 0xC8` =
camera 1 `+0x58` yaw; `0x7790` = camera 5 `+0x58`.) The 180° flip is
`(0x8000 - yaw) mod 0x10000` in the 16-bit angle unit — the same unit as the
ratio input.

## 5. Entry angles and the ratio

`angle = 4608` (0x1200) = **25.3°**; `3048` = 16.7°; `3464` = 19.0°;
`3430` = 18.8°; `0` = 0°. The ratio is `(dim/2)·cot(angle)` (FU-96 §2), so
the 25.3° entry gives `(320/2)·2.111 ≈ 338` — the "[0x146B0] ≈ screen
width" observation of FU-88 §3.1 (superseded by FU-94/FU-96); angle 0 fails
the `cos < 0x10000` gate and keeps the previous ratio (the `0x85AC` array's
first entry).

## 6. Errata / closures

* **FU-96 §6 leg 1 (camera `+0x4C` writers)** — closed: written only by
  `FUN_0004D7E8` from the type entry's `[5]`; no per-frame writer.
* **FU-96 §6 leg 3** — the type table and its three entry-array pointers are
  now mapped (§2), including values; the per-camera handlers
  `(&0x8B80)[i]` remain cited only.
* **FU-88 §3.1** — carried: `[0x146B0]` is yaw/type dependent (FU-94 §1,
  FU-96 §2), not a stored screen width.
* **§3/§7 "the ten cameras carry the static default 21 / `+0x4C` = 21 each"
  — corrected (M2 Task 11 fix round 1, first-hand `/FIFA96.EXE`)**: the
  camera `+0x4C` field is a **dword** `0x1500` (`read_memory 0x107554` =
  `00 15 00 00`, `0x1075C4` identical); the "21" is the **byte at `+0x4D`**
  (`0x15`, `0x107555`).
  `FUN_0004D7E8`'s `MOV [EDX+0x4C],EBX` (`0x4D836`) writes the full dword
  from the type entry `[5]`, so the runtime values (4608/3048/3464..) are
  dword-scaled.

## 7. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x4D7E8, 0x4D498,
0x4CAEC, 0x505D0, 0x4C904; `get_xrefs_to` 0x7508 (11 refs), 0x7554 (single
write `0x4D836`); `get_function_callers` 0x4D7E8 (three); `read_memory`
0x107DC8 (offset 0x7508), 0x107508 (camera 0), 0x107554/0x1075C4/0x107634/
0x1076A4/0x107714/0x107784/0x1077F4/0x107864 (`+0x4C` dwords = `0x1500`; the
"21" is the `+0x4D` byte = `0x15` — see §6), 0x108B64
(type pointers), 0x10896C (type 0 descriptor), 0x107E2C/0x107F1C/0x1085AC
(entry arrays). Analysis-only: no port, capture-rig, ISO, or Ghidra-project
change.

## 8. Open legs

1. **`EDX` entry index**: which of the three arrays/variants each caller
   (`0x4CAEC`/`0x4CBA0`/`0x4CC80`) selects, and what the arrays name
   (default/zoom/TV?).
2. **Type descriptor fields** (0..0x5C, e.g. `0x340`, `0xFA0`, `0x578`,
   `0xEA6`, `0x1200`, the `8`s): semantics not derived.
3. **Home/away blocks** `0x83CC`/`0x84BC` (positions/sizes written into
   camera 5 and `[0x7784]`).
4. **Per-camera handlers** `(&0x8B80)[i]` called at the end of
   `FUN_000505D0`.
5. **`5/12` factor** (FU-96 §6 leg 2, carried).
