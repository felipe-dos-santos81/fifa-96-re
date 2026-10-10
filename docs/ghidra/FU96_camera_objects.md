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
* **§1 `+0x4C` field row / §3 / §5 provenance / §6 leg 1 "static default 21"
  — corrected (M2 Task 11 fix round 1, first-hand `/FIFA96.EXE`)**: the camera
  record `+0x4C` is a **dword** `0x1500` (`read_memory 0x107554` = `00 15 00
  00`; sibling `0x1075C4` identical; the §5 provenance's "(all `+0x4C` = 21)"
  reads the same bytes and is corrected the same way). The "21" is the **byte
  at `+0x4D`** (`0x107555`, i.e. 0x15), and the writer
  `0x4D836 MOV [EDX+0x4C],EBX` stores the full dword from the camera-type
  entry `[5]`, so the live values are dword-scaled (FU-97 §6 carries the same
  correction for its `+0x4C = 21 each` provenance). The engine's near-depth
  ratio consumes the dword (`(0x14<<16)/(2·0x1500) = 121`).

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `read_memory` 0x107DC8 (object-4 offset
0x7508), 0x107508 (camera 0 fields), 0x107554/0x1075C4/0x107634/0x1076A4/0x107714
(all `+0x4C` dwords `0x1500`; the "21" is the `+0x4D` byte `0x15` — see §4
Errata); `decompile_function` 0x4CE34, 0x505D0, 0x4C3B0, 0x4C904;
`get_xrefs_to` 0x7DC8 (121 refs, `0x4Cxxx..0x51xxx` band); `disassemble_bytes`
0x44240 (256 B), 0x441D8 (104 B). Port write set:
`include/fifa96_loader/fifa96_projection.h`,
`src/fifa96_loader/fifa96_projection.c`, `tests/test_projection.c`, plus the
FU-95 in-place correction. Analysis-only otherwise; no capture-rig, ISO, or
Ghidra-project change.

## 6. Open legs

1. **Camera `+0x4C` writers**: the runtime update (the `0x4Cxxx..0x51xxx`
   band) and the angle's live range/units (the static field is the dword
   `0x1500`, with `+0x4D` byte `0x15`, in all five cameras — see §4 Errata).
   **Narrowed (T2, 2026-10-09)**: the FU-71 event producers now write the
   live pose/target (`fifa96_camera_event_set`/`pan_step`, FU-148 §12) and
   the row-04 gameplay caller is wired; the 0x4Cxxx..0x51xxx view-mode/angle
   writers and the `+0x4C` ratio's live clamp stay unported.
2. **`5/12` factor**: why the second ratio uses 5/6 of the first
   (`dim2/2 = 5·dim/12`).
3. **Camera selector table `(&0x8B64)[(&0x7514)[i*0x1C]]`** and the
   per-camera handlers `(&0x8B80)[i]` are cited only.
4. **Settings index 4** option semantics (FU-95 §6 leg 3, carried).
5. **`FUN_00079F3C` camera place** — **closed (M2 interactive Task 2 / T2);
   derivation in §7.** The function moves record *targets*, not the camera (the
   caller's `EBX = 0x15774C` is dead; the body loads the camera triple itself),
   so the engine camera staying at the `[0x10F328/2C/30]` reset triple at
   kickoff (`FUN_000700F4((0,0,0))`; first-hand 0x88E4B..0x88E6A) is correct.
   The place preserves the target's camera direction, so it cannot move the
   controlled side out from behind the camera; the one-sided kickoff draw is
   the **engine's stand-in view** (yaw/pitch 0), and native kickoff framing is
   carried on legs 1/3 (the camera-mode/angle feed `[0x14E57C]`/`FUN_000505D0`
   presets `0x108B64` handlers `0x108B80`) plus the FU-71 follow writer
   `FUN_00071C94`. T2 corrects the earlier "both sides draw at kickoff"
   reading — the place re-frames the non-controlled side's in-ring records
   around the camera, it does not create visibility behind it.

## 7. Leg 5 derivation — `FUN_00079F3C` (M2 interactive Task 2 / T2)

First-hand `/FIFA96.EXE` (read-only): `get_function_by_address 0x79F3C` ->
body `0x79F3C..0x7A027` (74 instructions); `disassemble_function 0x79F3C`;
`read_memory 0x1106C3` (29 B), `0x15774C` (16 B), `0x10F328` (16 B);
`read_memory 0x79F5F`/`0x79FA7` (raw displacement/encoding checks);
`disassemble_function 0x8CF60` (the calling loop), `0x8DCD4`, `0x8DD70`,
`0x795A4`, `0xCD474`; `disassemble_bytes 0x88DC8` (the act-1 stage-0 camera
reset); `get_xrefs_to 0x8CF60`/`0x73E08`.

### 7.1 Call site and record

`FUN_0008CF60` (`0x8CF60..0x8CFAB`) loops the 11 records of one team:
`0x8CF6C MOV EAX,ECX; MOV EBX,-1; CALL 0x6D920` (resolver), `0x8CF7C CALL
[ECX+0x1C]` (the phase handler, `EDX=&rec+0x4D`), `0x8CF81 MOV EBX,0x15774C;
0x8CF86 CALL 0x79F3C` (this function), `0x8CF90 CALL 0x79B6C` (the commit).
The `EBX` value is dead: `0x79F43 MOV ESI,EAX` takes the record (EAX) and
`0x79F76 MOV EAX,0x15774C` loads the *address* of the FU-71 camera triple
(`0x15774C` x / `0x157750` y / `0x157754` z dwords).

### 7.2 Body

```
0x79F45 MOV EAX,[EAX]            ; [rec] = the team pointer
0x79F49 MOV DL,[EAX+0x826]       ; team side byte
0x79F4F MOV EAX,[0x157AAC]; SAR EAX,0x18   ; controlled side
0x79F57 CMP EDX,EAX; JZ 0x7A020  ; gate 1: skip the controlled team
0x79F5F MOV EAX,[0x157A4A]; SAR EAX,0x18   ; live phase
0x79F67 CMP byte [EAX+0x1106C3],0; JZ 0x7A020   ; gate 2
0x79F74 MOV EBX,ESP; MOV EAX,0x15774C; LEA EDX,[ESI+0x4D]; CALL 0x8DCD4
0x79F83 MOV EAX,[ESP-2]; SAR EAX,0x10      ; the distance word
0x79F8A CMP EAX,0x180; JG 0x7A020          ; out-of-ring records keep the target
0x79FA2 CALL 0x8DD70                       ; angle = FUN_000CD474(dx,dz)
0x79FA7..0x79FD5  the 0x114E04 sine fold + FUN_000795A4(0x180, sin)
0x79FD8 MOV EAX,[0x15774C]; ADD EAX,EDX; MOV [ESI+0x4D],EAX   ; target.x
0x79FE2..0x7A013  the same fold for angle+0x100 (cosine)
0x7A016 MOV EAX,[0x157754]; ADD EAX,EDX; MOV [ESI+0x55],EAX   ; target.z
```

* `FUN_0008DCD4` (`0x8DCD4`) is the shared metric port (`fifa96_arm_dist_stage`):
  word `dx = target.x - camera.x`, `dz = target.z - camera.z`, output
  `out[0]` = octagonal length `max + f(min)`, `out[1] = dx`, `out[2] = dz`
  (the `0x79F95`/`0x79F99` dword loads plus `SAR 0x10` recover the words the
  trap table warns about).
* `FUN_0008DD70(dx,dz)` is the `FUN_000CD474` atan table port
  (`fifa96_entity_angle`), 0x400-per-turn.
* The `0x79FA7..0x79FD5` fold (`SHL AH,7` / `ADD AH,AH` capture bits 9/8 then
  complement-and-negate) is exactly `fifa96_entity_sine` (the `0x114E04`
  257-entry table; the `angle + 0x100` second call is the cosine); the table's
  first-hand values (`0,402,804,...`) match the existing port.
* `FUN_000795A4` is `(a*b + 0x8000) >> 16`; the result's low word only is
  stored (`MOVSX` of AX, `0x79FD5`/`0x7A013`).

**Derived semantics:** each non-controlled-team record whose octagonal camera
distance is `<= 0x180` has its target moved to `camera + 0x180·(sin,cos)(angle
of target-camera)`, i.e. snapped onto the `0x180` ring around the camera along
its existing direction; farther records are untouched, the controlled team is
never touched. The second gate is the first-hand phase table
`0x1106C3 + phase` = `00 01 00 00 01 00 01 01 01` then zeros and the
`0x1A..0x1C` tail `03 01 78` (active at phases 1/4/6/7/8/0x1A..0x1C; kickoff
phase 1 passes).

### 7.3 Engine landing (T2)

`fifa96_match_entities_camera_place(pool, controlled_side, phase, cam_x,
cam_z)` is the pool pass; begin calls it after the formation seed and before
`fifa96_match_entities_kickoff_place` (the native order handler -> place ->
commit; the commit leaves targets untouched, so the pass ordering is
equivalent). The place is skipped when the formation resource is absent (the
engine's zero-target degradation; a zero target would otherwise fabricate a
`(272,272)` ring position that no native state reaches, because the native
phase handler always precedes the place). Fixtures:
`tests/test_engine_match_entities.c` (anchors, both gates, camera offset, the
octagonal-metric discriminator) and
`tests/test_engine_match_render.c::test_camera_place_moves_near_record_into_frame`
(an in-ring record below the `0x78` near gate moves onto the ring and draws).
The M2 tape re-pin (frames 49..165, hash-only) and the framing correction are
recorded in the test provenance and ENGINE.md.
