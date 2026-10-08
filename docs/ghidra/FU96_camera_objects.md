# FU-96 — the camera/view objects and the ratio formula (correction)

Follow-on to FU-95 (open legs 1, 2). This slice maps the view object behind
`[0x7DC8]`, the 5-camera table and its selector, and corrects the ratio
reading: the gated/multiplied sincos output is the **cosine**, so the ratios
are `(dim/2)·cot(angle)` in 16.16. The exact integer sequence is ported as
`fifa96_projection_view_ratio` / `fifa96_projection_view_scale`.

Result in one line: **`[0x7DC8]` points at camera `&0x7508 + i*0x70`
(object-4), `FUN_0004C904` copies six of its fields to `0x4E4E0`, and
`FUN_00049830` feeds the copy's `+0x14` (camera `+0x4C`) into the ratio
computation, which returns `(dim/2)·cot(angle)`.**

## 1. The camera objects

`[0x7DC8]` (storage image `0x107DC8`) holds the object-4 offset `0x7508`, so
the first camera object lives at object-4 `0x7508` (image `0x107508`).
`FUN_000505D0` (the camera selector, `0x505D0`) picks the active one:

```
index = [view+4]                        ; clamped <= 0, 3 -> 5
if (index != 3):
    src = (&0x8B64)[ (&0x7514)[index*0x1C] ]
    target = src + 8 ; alt = src + 0x44
    [view+0x48] = FUN_0004B7D0() || FUN_0004B6FC() ? [src+0x4C] : [src+0x48]
else:
    target = &local_38 ; alt = &local_34 ; local_30 = [0x8B7C] + 200
[alt] = 0
switch (FUN_00053D50()) { ... }
[0x7DC8] = &0x7508 + index*0x70        ; selects the camera
(*(code *)(&0x8B80)[iVar3])()          ; per-camera handler
```

`FUN_0004CE34`/`FUN_0004CEF4`/`FUN_0004D04C`/`FUN_0004CF7C`/`FUN_0004D488`
are the other `[0x7DC8]` readers/writers in the `0x4Cxxx..0x51xxx` camera
band (FU-95 §1's driver chain).

Static camera fields (object 0, image `0x107508`; all five cameras have the
same `+0x4C`):

| offset | static | role |
|--------|--------|------|
| +0x10 | -0x780 | x |
| +0x14 | 0xA0 | y |
| +0x18 | 0x1E | z |
| +0x4C | 21 | angle fed to the ratio path |
| +0x58 | 0xBF9C | view-matrix yaw (FU-88 §1.1) |
| +0x5C | 200 | view-matrix pitch |

`FUN_0004C904` copies the fields into the live record `0x4E4E0`:
`+0x00 ← cam+0x10`, `+0x04 ← +0x14`, `+0x08 ← +0x18`, `+0x0C ← +0x58`,
`+0x10 ← +0x5C`, `+0x14 ← +0x4C`. `FUN_00049830` then reads `[0x4E4E0+0x14]`
= `cam+0x4C` (`0x498AB MOV EAX,[EBX+0x14]`) and calls `FUN_00043E30`. So the
ratio angle is the camera field `+0x4C`, **distinct** from the matrix
yaw/pitch at `+0x58`/`+0x5C`.

## 2. The ratio formula — correction to FU-95

`FUN_0004C3B0` (`0x4C3B0`) builds the X-rotation matrix from
`FUN_000A1A60(angle, &A, &B)`:

```
[0]=0x10000 [4]=B [5]=A [7]=-A [8]=B        ; Rx = [[1,0,0],[0,B,A],[0,-A,B]]
```

so **`A` (param_2) = sine** and **`B` (param_3) = cosine** — the ported
`fifa96_projection_sincos(angle, &sin, &cos)` naming holds.

In `FUN_000441D8` the sincos outputs land at `[ESP]` (param_3) and `[ESP+4]`
(param_2) after the call (`0x44216..0x44224`; `param_3` is pushed last, so it
is at the frame base). The body then:

```
gate:   0 < [ESP] < 0x10000                 ; the cosine output
t:      EAX = dim>>1 ; IMUL EDX=[ESP]       ; (dim/2) * cos
        +0x8000 ; SHRD 16                   ; t = round((dim/2)*cos) in integer units
divide: EBX = [ESP+4] = sin
        q = t / sin ; rem = t % sin
        frac = (rem<<16) / sin              ; 0..0xFFFF
        EAX = ((q<<16) + frac) << 16         ; 32-bit wrap; low 16 bits zero
        [0x4B0E0] = EAX
```

so the stored value is the 16.16 representation of `(dim/2) · cos/sin` =
**`(dim/2)·cot(angle)`**, for `angle` in the open first quadrant. The second
ratio uses `dim2 = floor(floor(10·dim/12)/2)` = `floor(5·dim/12)`
(`0x44299..0x442C4`) and the settings-4 gate as in FU-95 §2.

Hand-checked anchors (also pinned in `tests/test_projection.c`):
`angle = 0x2000` (45°), `dim = 320` → `ratio = 159<<16`;
`view_scale(..., dim=320)`: `ratio1 = 159<<16` (`dim2 = 133`) and
`ratio2 = 66<<16` (since the helper halves `dim2` again).
`dim = 640` → `319<<16`.

Geometry note: `(w/2)·cot(tilt)` equals the screen width `w` near a **~26.6°**
tilt; the static `+0x4C = 21` is overwritten by the camera update, and the
gate only admits `(0°, 90°)`, so the live value is a mid-quadrant camera
angle. `[0x146B0] = ratio1>>16` therefore reads as "screen width" only at
that tilt — FU-88 §3.1's naming, corrected in FU-94/FU-95.

## 3. Port

`fifa96_projection_view_ratio(angle, dim, &ratio, &computed)` — the exact
integer sequence above (`computed = 0` when the gate fails, the caller keeps
the previous ratio); `fifa96_projection_view_scale(angle, dim, skip_second,
&ratio1, &ratio2, &computed)` — `FUN_000441D8`'s two ratios with the
settings-4 fallback (`skip_second` → `ratio2 = ratio1`).

`tests/test_projection.c::test_view_ratio` pins the 45° anchors, the
640-width ratio, the gate (`0x0000`, `0x4000` → `computed = 0`), the
`skip_second` fallback and the argument errors.

## 4. Errata

* **FU-95 §2/§4** — the sincos output used as multiplier and gate is the
  **cosine** (param_3), not the sine; the ratios are
  `(dim/2)·cos/sin` = `(dim/2)·cot(angle)`. FU-95's text is corrected in
  place with a pointer to this section.
* **FU-95 §6 leg 2** — the producer question is narrowed: the angle is the
  camera object's `+0x4C`, copied to `0x4E4E0+0x14` by `FUN_0004C904` and
  read by `FUN_00049830`; the writers of `+0x4C` live in the
  `0x4Cxxx..0x51xxx` camera band (open leg).
* **§1 `+0x4C` field row / §3 / §6 leg 1 "static default 21" — corrected
  (M2 Task 11 fix round 1, first-hand `/FIFA96.EXE`)**: the camera record
  `+0x4C` is a **dword** `0x1500` (`read_memory 0x107554` = `00 15 00 00`;
  sibling `0x1075C4` identical). The "21" is the **byte at `+0x4D`**
  (`0x107555`), and the writer `0x4D836 MOV [EDX+0x4C],EBX` stores the full
  dword from the camera-type entry `[5]`, so the live values are
  dword-scaled. The engine's near-depth ratio consumes the dword
  (`(0x14<<16)/(2·0x1500) = 121`).

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `read_memory` 0x107DC8 (object-4 offset
0x7508), 0x107508 (camera 0 fields), 0x107554/0x1075C4/0x107634/0x1076A4/0x107714
(all `+0x4C` = 21); `decompile_function` 0x4CE34, 0x505D0, 0x4C3B0, 0x4C904;
`get_xrefs_to` 0x7DC8 (121 refs, `0x4Cxxx..0x51xxx` band); `disassemble_bytes`
0x44240 (256 B), 0x441D8 (104 B). Port write set:
`include/fifa96_loader/fifa96_projection.h`,
`src/fifa96_loader/fifa96_projection.c`, `tests/test_projection.c`, plus the
FU-95 in-place correction. Analysis-only otherwise; no capture-rig, ISO, or
Ghidra-project change.

## 6. Open legs

1. **Camera `+0x4C` writers**: the runtime update (the `0x4Cxxx..0x51xxx`
   band) and the angle's live range/units (static default 21 in all five
   cameras).
2. **`5/12` factor**: why the second ratio uses 5/6 of the first
   (`dim2/2 = 5·dim/12`).
3. **Camera selector table `(&0x8B64)[(&0x7514)[i*0x1C]]`** and the
   per-camera handlers `(&0x8B80)[i]` are cited only.
4. **Settings index 4** option semantics (FU-95 §6 leg 3, carried).
