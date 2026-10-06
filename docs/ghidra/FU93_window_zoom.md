# FU-93 — window → zoom scale (`0x8DDC`/`0x8DE0`) and the replay counter

Follow-on to FU-92 (open legs 1 and 2: the `FUN_000441D8`/`FUN_00053240`
secondaries and the `[0x4E58C]` camera value). This slice derives the window
feedback: the setter converts the window rect into a 16.16 scale consumed by
the overlay/HUD coordinate scalers, and the `0x4E58C` value is a replay
sequence counter driven by a small state machine. The clean ratio math is
ported as `fifa96_window_scale`.

Result in one line: **`FUN_00043E48` ends by calling `FUN_00053240`, which
stores `window_width * 0x10000 / 320` and `window_height * 0x10000 / 200` in
`[0x8DDC]`/`[0x8DE0]`; the overlay code multiplies screen coordinates by
these factors.**

## 1. `FUN_00053240` — the window-to-zoom setter

Register arguments (from the `FUN_00043E48` call site, `0x43F53..0x43F5D`):

```
FUN_00053240(EAX = x0, EDX = y0, EBX = x1, ECX = y1)
```

`FUN_00043E48` calls it with the **original** (pre-clamp) values:
`EAX = src_x`, `EDX = src_y`, `EBX = src_w + src_x`, `ECX = src_h + src_y`
(`0x43F53 MOV EBX,[ESP+0x18]; 0x43F57 MOV EDX,ESI; 0x43F59 MOV EAX,EDI;
0x43F5B ADD EBX,EDI; 0x43F5D CALL 0x53240`; `ECX` was computed `src_h+src_y`
at `0x43F4C`).

The two ratios are computed with 32×32→64 division (the decompiler's
`CONCAT44`/`%` dance is one 64-bit divide):

```
[0x8DDC] = ((x1 - x0) << 16) * 0x10000 / 0x1400000     ; width  * 65536 / 320
[0x8DE0] = ((y1 - y0) << 16) * 0x10000 / 0xC80000      ; height * 65536 / 200
```

`0x1400000 = 320 << 16`, `0xC80000 = 200 << 16`: the reference screen. A
320-wide window gives `0x10000`, a 160-wide one `0x8000` — the factor is the
**window size as a 16.16 fraction of the screen**.

The same call snapshots the window into nearby globals and fills a HUD
layout block:

```
[0x8DE4] = x0   [0x8DE8] = x1   [0x8DEC] = y0   [0x8DF0] = y1
if [0x8DDC] < 0x10000:            ; window narrower than the screen
    [0x4E51C] = 9    [0x4E528] = x0 + 1    [0x4E520] = x0 + 3
    [0x4E524] = x0 + 0x23
    [0x4E52C] = y1 - (sprite_h * 0xB800 >> 16) - 1
else:
    [0x4E51C] = 0xC  [0x4E528] = x0 + 2    [0x4E520] = x0 + 6
    [0x4E524] = x0 + 0x32
    [0x4E52C] = y1 - sprite_h - 2
[0x4E514] = 0x36
```

then a series of `scale * {0x43, 5, 3, 7, 0x28, -0x12, 4}` 16.16 products
into `0x4E690..0x4E6CC` (HUD spacing), and ends calling `FUN_00044BE0`
(no-return path per the decompiler — open leg).

## 2. Consumers: the 16.16 coordinate scalers

`[0x8DDC]`/`[0x8DE0]` are read as **coordinate multipliers** by the overlay
family (`FUN_00054AE4`, `0x54F24`, `0x560C8`, `0x5619C`, `0x544B4`). The
pattern (`0x560D4`, `0x54B00`):

```
MOV EAX,screen_axis_value        ; e.g. 0x140, 0x21, a sprite field
MOV EDX,[0x8DDC]
IMUL EDX                         ; 32x32 -> 64
ADD EAX,0x8000 / ADC EDX,0
SHRD EAX,EDX,0x10                ; rounded 16.16 product
ADD EAX,[0x8DE4]                 ; + window origin x
```

so overlay coordinates are mapped from the reference 320×200 space into the
current window (scale ≤ `0x10000`, offset `[0x8DE4]`/`[0x8DEC]`).

## 3. `FUN_000441D8` — scaled bitmap dimensions

```
FUN_000441D8(EAX = packed id, EDX = w?, EBX = h?, ...)   ; args messy, cite only
```

caches `(packed id, w', h')` against `[0x6798]/[0x679C]/[0x67A0]` and, on a
miss, calls `FUN_000A1A60(packed id, &w', &h')`, storing the scale block in
`[0x4B0E0]`/`[0x4B0E4]`. `FUN_000A1A60` splits the packed value: index
`id >> 6` and scale `id & 0x3F`, fetches base dimensions via
`FUN_000CE3B0(index, &w, &h)` and returns

```
scale = ((id & 0x3F) * 0x6487E) >> 9
w' = w + ((h >> 2) * scale >> 0x15)
h' = h - ((w >> 2) * scale >> 0x15)
```

— a small aspect-preserving resize. `0x4B0E8` (the packed id) and the
`FUN_000CE3B0` resource table are open legs.

## 4. `[0x4E58C]` — the replay sequence counter

`FUN_00053D58` (gate used by the marker phase `FUN_00057510` and by the
match loop):

```
return ([0x4E58C] - 0xF1 < 0x78) && [0x4E58C] != 0;     ; 0x7A..0x168
```

`FUN_00053D84` returns the raw difference (`[0x4E58C] - 0xF1`) and the live
loop takes its absolute value (`0x49862 XOR ESI,EAX`) for the window A/B
choice — the same band.

`FUN_000510DC` is the driver — a sequence/replay state machine:

* state `[0x8E0C]`, accumulator `[0x8E10]` (counts up to `0xF0`);
* `[0x8E0C]==1` → `FUN_000642FC()` (replay stop path);
* `[0x8E0C]>1 && [0x8E10]<0xF0` → advances `[0x8E10]` by the frame delta;
* the reset branch zeroes the state and, through
  `FUN_00064E74`/`FUN_00064DFC`, steps `[0x4E58C]`:
  `if [0x4E58C] > 0xF0 → FUN_00064DFC(...)`, `if [0x4E58C] < 0x168 → +=delta
  else = 0x169`; when `[0x4E58C] <= delta` → `FUN_00064E8C`, `[0x4E58C]=0`;
* `in_EAX != 0` start path: `[0x4E58C]=1`, `FUN_00064270`, `FUN_00037114`,
  state=1.

`FUN_000537F8` resets a 0x4E5xx block (including `[0x4E58C]`, `[0x4E588]` via
`FUN_0004B380`, `[0x4E574]/[0x4E584]`, the `0x4E5A8..` copy from `0x8C3C`)
and `[0x8E04]=-1`. Together with FU-92 §4 (160×100 window active in the band)
this reads as a **replay/sequence playback timer**: the window is the
sequence camera view, and the HUD scalers of §2 place the overlays.

## 5. Errata

* **FU-85 §9 leg 3** — `[0x8DDC]` is not the entity-projection divide: its
  located consumers are the overlay/HUD coordinate scalers (`0x54AE4`,
  `0x54F24`, `0x560C8`, `0x5619C`, `0x544B4`); the entity projection chain
  (FU-88) does not read it.
* **FU-92 §9 leg 1 / §3.1** — `[0x4E58C]` is a replay/sequence counter
  (cap `0x169`, gate band `0x7A..0x168`) driven by `FUN_000510DC`, not a
  continuous camera zoom; window A is the sequence view.

## 6. Port

`fifa96_window_scale` (`include/fifa96_loader/fifa96_window.h`) ports the
ratio math of §1: `scale_x = window_w * 0x10000 / 320`,
`scale_y = window_h * 0x10000 / 200` (16.16, 64-bit intermediate), plus
`fifa96_window_zoomed(win)` returning whether `scale_x < 0x10000` (the §1
layout branch). `tests/test_window.c` covers full-size (identity), the
160×100 sequence window (`0x8000`), a non-multiple size and the errors.

## 7. Provenance

Ghidra MCP on `/fifa96_le.bin`: `decompile_function` 0x53D58, 0x53D84,
0x510C0, 0x510DC, 0x537F8, 0x441D8, 0xA1A60, 0x53240, 0xCBDB0, 0x9A45C,
0x9A780, 0x9A9E4, 0x9A520, 0xCE7A0, 0xCE6F0, 0xD0998, 0xD0A87, 0x43B48,
0x43D94, 0x436E4, 0x438F0, 0x43920, 0x43984, 0x439D0, 0x43F68, 0x443E8,
0x44E3C; `get_xrefs_to` 0x4E58C, 0x6740, 0x6764, 0x8DDC, 0x8DE0;
`disassemble_bytes` 0x560D0 (64 B), 0x54AF0 (64 B), 0x43E48 (288 B).
Analysis-only outside the port; no capture-rig, ISO, or Ghidra-project
change. Port write set: `include/fifa96_loader/fifa96_window.h`,
`src/fifa96_loader/fifa96_window.c`, `tests/test_window.c`.

## 8. Open legs

1. **Magnification of the window surface**: `FUN_00043B48` blits the window
   drawable 1:1 at (0,0) into the selected page; whether the 160×100 page is
   stretched to the 320×200 CRT by a CRTC/double-scan mode or by another
   blit is not evidenced (candidates: `FUN_000CBDB0` is only a clear on the
   current clip; `FUN_0009A780` does no scaling).
2. **`FUN_00044BE0`** (called from `FUN_00053240`): decompiles as no-return;
   its role (probably a jmp-based transition) is not derived.
3. **`0x4B0E8`** (the packed bitmap id) producer and the `FUN_000CE3B0`
   resource table behind `FUN_000A1A60`.
4. **HUD layout block `0x4E514..0x4E6CC`**: field meanings beyond the quoted
   constants and the `scale * {0x43, 5, 3, 7, 0x28, -0x12, 4}` products.
5. **`0x64270`/`0x642FC`/`0x64E74`/`0x64DFC`/`0x64E8C`** (replay helpers
   around the `0x64xxx` band) are cited only.
