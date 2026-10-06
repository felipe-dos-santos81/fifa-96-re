# FU-92 — the match window record (`0x4B0A4`) and the render clip

Follow-on to FU-89 §2/§3.2 (open leg 2 "arg0 owner", and the "depth override"
reading of the `[arg0+0x18..0x24]` compare). This slice locates the arg0
producer, derives the record layout and its setter family, and shows the
render body's test is a **clip-rect edge test** — the same record is the
match's rendering window. The clean integer math is ported as
`fifa96_window`; the scene helper that was read as "depth" is renamed
`fifa96_scene_clip_edges`.

Result in one line: **`[0x7318]` = `&0x4B0A4`, a clamped pixel window
(x0,y0,x1,y1) with 16.16 copies and center fields, maintained by the match
loop; the render body culls each sorted sprite against its edges.**

## 1. The record and its owner

`FUN_00043600` (`0x43600`, 1 insn) is the whole "producer":

```
0x43600  MOV EAX,0x4B0A4 / RET        ; decompile: return &DAT_0004b0a4
```

The match update stores its return into `[0x7318]` after every refresh
(`0x493d9`, `0x495f3`, `0x49827`, `0x498a6`) and hands it to the render
drivers as arg0 (`0x49635 CALL 0x58BC0`, `0x49655 0x58D70`, `0x4966C 0x58B68`,
`0x498B5/0x498BA` — both `EAX=[0x7318]`). It is a fixed global; the "owner"
is whoever writes its fields: `FUN_00043E48`.

Record fields (all from the setter's stores):

| Offset | Field | Source |
|--------|-------|--------|
| +0x00 | `x0` | clamped x |
| +0x04 | `y0` | clamped y |
| +0x08 | `x1` | `x0 + w` |
| +0x0C | `y1` | `y0 + h` |
| +0x10 | `w` | clamped width |
| +0x14 | `h` | clamped height |
| +0x18 | `x0 << 16` | 16.16 copy |
| +0x1C | `y0 << 16` | 16.16 copy |
| +0x20 | `x1 << 16` | 16.16 copy |
| +0x24 | `y1 << 16` | 16.16 copy |
| +0x28 | `src_x` | request x (unclamped) |
| +0x2C | `src_y` | request y (unclamped) |
| +0x30 | `src_w` | request w (unclamped) |
| +0x34 | `src_h` | request h (unclamped) |
| 0x146A8 | `(src_x + src_w/2) << 16` | center x |
| 0x146AC | `(src_y + src_h/2) << 16` | center y |

The fields continue past the 10 dwords `FUN_00043618` copies: `0x4B0E0`,
`0x4B0E4` are written by `FUN_000441D8` and `0x4B0E8` read by it (open leg).

## 2. `FUN_00043E48` — the window setter

Register-argument convention (the wrapper `FUN_00043DFC` pushes all four and
returns via `RET 0x10`, which is why a naive decompile invents 6 arguments):

```
FUN_00043E48(EAX = x, EDX = y, EBX = w, ECX = h)
```

```
w' = (w < 8) ? 8 : min(w, [0x6708])
h' = (h < 8) ? 8 : min(h, [0x670c])
x' = (x < 0) ? 0 : min(x, [0x6708] - w')
y' = (y < 0) ? 0 : min(y, [0x670c] - h')
[x0]=x' [y0]=y' [x1]=x'+w' [y1]=y'+h'      ; 0x4B0A4..
[x0<<16][y0<<16][x1<<16][y1<<16]           ; 0x4B0BC..
[w']=0x4B0B4 [h']=0x4B0B8
[0x4B0CC]=src_x [0x4B0D0]=src_y [0x4B0D4]=src_w [0x4B0D8]=src_h
[0x146A8]=(src_x + (src_w>>1))<<16 [0x146AC]=(src_y + (src_h>>1))<<16
FUN_000441D8([0x4B0E8], src_w, src_h, src_h+src_y)
FUN_00053240(src_x, src_y, src_w+src_x)
```

`[0x6708]`/`[0x670C]` are the surface dimensions, set by the mode switch
(§3.4). The originals stored at `0x4B0CC..` come from the wrapper's pushed
copies, i.e. **before** the clamps.

## 3. The window family

### 3.1 Wrappers and activators

* `FUN_00043DFC` (`0x43DFC`) — register passthrough: `PUSH` all four, `CALL
  0x43E48`. Both activators call it.
* `FUN_00043E08` (`0x43E08`) — re-apply the **current** window: loads
  `EAX=[0x6710] EDX=[0x6714] EBX=[0x6718] ECX=[0x671C]` and calls `0x43DFC`.
* `FUN_000438F0` (`EAX=w, EDX=h`) — defines window **A**: `[0x6754]=0
  [0x6758]=0 [0x675C]=w [0x6760]=h`, `MOVSD x4` into `[0x6744..0x6750]`
  (so A = rect `(0,0,w,h)`), then `MOVSD x4` into `[0x6710..0x671C]`.
* `FUN_00043920` — activates A: copies the A block `0x6744..0x6750` into the
  current `0x6710..0x671C`, sets `[0x6774]=1`, copies the A size block
  `0x6754..0x6760` into `0x6700..0x670C`, then calls `FUN_00043DFC` with the
  copied current values.
* `FUN_00043984` — activates B: same dance from the B block
  `0x6720..0x672C` / `0x6730..0x673C`, sets `[0x6774]=0`.
* `FUN_000439D0` — applies: buffer = `[0x6774] ? [0x6740] : [0x6764]`; if
  zero, `FUN_0009A45C(W,H,0)` then `FUN_000CE6F0(buffer)`,
  `FUN_000CBDB0(0)`; `FUN_000CE6F0(buffer)` (`0xCE6F0` = page setter,
  FU-57/FU-85), then `FUN_000CE7BC(x0,x1,y0,y1)` sets the **raster clip** and
  `[0x6778]=1`.

Window A is defined full-size: the live match calls `FUN_000438F0(0xA0,0x64)`
at `0x49877` — a **160×100** surface — then activates it.

### 3.2 Window B — the zoom/pan window

`FUN_00043F68` (`0x43F68`, no arguments, gated by `FUN_00037AE4()==0 &&
[0x6780]==0`) saves the current rect to `0x4B06C..0x4B078`, then steps the
window size by `EDX`'s sign:

```
W=[0x6708]=0x280:  dw=±0x50, dh=±0x3C
W=[0x6708]=0x140:  dw=±0x28, dh=±0x19
w2 = w + dw ; h2 = h + dh
x2 = (W - w2) >> 1 ; y2 = (H - h2) >> 1
CALL FUN_00043E48(x2, y2, w2, h2)
4 border fills via FUN_0009F994 when shrinking (dw<0)
B block 0x6720..0x672C = (x2, y2, w2, h2)
```

so B is the centred, bordered **camera zoom** window.

### 3.3 Restore and union

* `FUN_00044E3C` — when `[0x6770]` is set, restores the saved `0x4B06C` block
  into both current and B blocks, applies it (`0x43DFC` + `0x439D0`), clears
  `0x9F7F0(0,0,W,H)`, sets `[0x676C]=1`, calls `0xCE7A0`.
* `FUN_00043618` — copies the 10 base dwords `0x4B0A4 → 0x4B07C`, then for
  each of up to 4 points (`EBX` = start index) computes a bounding box and
  updates the saved copy **per field with a signed max**:

```
saved.x0 = max(saved.x0, bbox_min_x)
saved.x1 = max(saved.x1, bbox_max_x)
saved.y0 = max(saved.y0, bbox_min_y)
saved.y1 = max(saved.y1, bbox_max_y)
```

The top-left uses the bbox **min** under a max (a shrink), the bottom-right
the bbox max (an expand) — ported literally. `0x4B07C` is read by
`FUN_00043B48` (`0x43D3D`); the only caller is the render variant
`FUN_00058D70`.

### 3.4 Resolution switch

`FUN_000443E8(mode)` clamps mode to `0..2`, looks up
`[0x6708]=(&0x67B4)[mode]` / `[0x670C]=(&0x67BC)[mode]` (BSS at rest), resets
all window blocks to `(0,0,W,H)`, frees the old pages, loads a 0x300 DAC table
via `FUN_000CE70C(0,0x100,...)`, and re-applies. `FUN_00017B40`/`0x395CC`
follow. The `0x140`/`0x280` comparisons in §3.2 prove modes 320- and
640-wide.

## 4. Match-loop selection

`FUN_00049830` (live match update, FU-89 §1):

```
0x4983C  [0x7308]=1
0x49846  CALL 0x53D58 ; if 0 → jump to 0x49A48 (frame skipped)
0x49853  [0x7308]=0
0x4985B  CALL 0x53D84 ; ESI = [0x4E58C] - 0xF1
0x49862  if ESI < 0: ESI ^= EAX (abs)
0x49868  CMP ESI,0x78 ; JGE 0x49883
0x4986D  EDX=0x64; EAX=0xA0; CALL 0x438F0   ; A = 160×100
0x4987C  CALL 0x43920                        ; activate A
0x49881  JMP 0x49888
0x49883  CALL 0x43984                        ; else activate B
0x49888  CALL 0x439D0                        ; apply
```

So the camera value `[0x4E58C]` within ±0x78 of 0xF1 selects the 160×100
window; otherwise window B (the current zoom window) is used. `[0x4E58C]`'s
producer and `FUN_00053D58`'s gate are open legs.

## 5. Renderer use — errata for FU-89's "depth override"

The values FU-89 §3.2 called depth fields are the **16.16 clip rect**:

```
0x57D5C  EAX = jittered screen pair       ; [0x54388]+slot*8
0x57D71  ECX = [arg0+0x24]                ; bottom = y1<<16
0x57D74  CMP ECX,[EAX+4]  / JNG           ; bottom <= jitter_y → cull
0x57D79  EDX = clean screen pair          ; [0x54364]+slot*8
0x57D92  CMP [EDX+4],[arg0+0x1C] / JG     ; clean_y <= top → cull
0x57DB3  delta = clean_y - jitter_y
0x57DC6  candidate  = [arg0+0x20] + 2*delta   ; right + 2δ
0x57DDA  alternative = [arg0+0x18] - 2*delta  ; left  - 2δ
0x57DEA  draw when  left-2δ < jitter_x < right+2δ
```

Culled sprites get `jitter_y := bottom` (`[EAX+4]=bottom`) before the walk
continues. The `+0x18..+0x24` fields are exactly the layout of §1. The port
helper formerly named `fifa96_scene_depth_override` is renamed
`fifa96_scene_clip_edges` (arguments `left/top/right/bottom`); the test was
renamed and its expectations are unchanged.

## 6. Port

`fifa96_window` (`include/fifa96_loader/fifa96_window.h`,
`src/fifa96_loader/fifa96_window.c`) ports the clean math:

* `fifa96_window_box` — the 10-dword record (`x0,y0,x1,y1,w,h` + four 16.16
  copies); `fifa96_window` — the record plus `screen_w/h`, `src_*`, centers.
* `fifa96_window_init(win, screen_w, screen_h)` — mode dimensions.
* `fifa96_window_set(win, x, y, w, h)` — `FUN_00043E48` exactly (min 8, clamp
  to screen, originals, centers).
* `fifa96_window_define_full(win, w, h)` — `FUN_000438F0` (window A).
* `fifa96_window_expand(win, saved, pts, count)` — `FUN_00043618` (copy then
  per-field signed max over up to 4 points; the original's start-index quirk
  is dropped, the port takes points `0..count-1`).
* `fifa96_scene_clip_edges` — the §5 cull math (renamed).

`tests/test_window.c` covers init, all four clamps, full-define, the
asymmetric expand and the argument errors.

## 7. Errata

* **FU-89 §2** ("Depth override (`0x57D5C..0x57DFB`)") — the fields are the
  window's 16.16 clip rect; the operation is a clip/cull with edge clamping,
  not a depth override. §7's "candidate/alternative x compare semantics (edge
  placement)" is the sheared x-clip quoted in §5.
* **FU-89 §12 open leg 2** (arg0 owner) — closed: `FUN_00043600` returns
  `&0x4B0A4`; the writer is `FUN_00043E48` via the match loop.
* **FU-89 §1/§12 open leg 5** — `FUN_00043618` is the saved-record union
  (§3.3), not a state gate; the gate tested at `0x4963C` is `FUN_00043608`
  returning `[0x6780]`.
* **FU-89 §12 open leg 5** (`FUN_00058BC0`/`0x58D70` no static callers) — the
  live match calls both (`0x49635`, `0x49655`); 0x58D70's union call site is
  the `FUN_00043618` caller of §3.3.

## 8. Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_xrefs_to` 0x7318, 0x43E48, 0x6708,
0x670C, 0x4B0A4, 0x4B0BC, 0x4B0C8, 0x4B07C, 0x6744, 0x6720;
`get_function_by_address` 0x493D9/0x495F3/0x49827 (unanalyzed — hand
disassembled); `disassemble_bytes` 0x493A0 (96 B), 0x495C0 (208 B), 0x49800
(192 B), 0x43DFC (16 B), 0x43E48 (32 B + 288 B), 0x43E08 (64 B), 0x438F0
(112 B); `decompile_function` 0x43600, 0x4C904, 0x43E48, 0x43618, 0x43DFC,
0x43E08, 0x439D0, 0x43920, 0x43984, 0x438F0, 0x441D8, 0x44E3C, 0x443E8,
0x43F68, 0x53D84, 0x43608; `get_function_callers` 0x43920/0x43984/0x438F0
(all `FUN_00049830`), 0x43618 (`FUN_00058D70`); `read_memory` 0x67B4.
Analysis-only outside the port; no capture-rig, ISO, or Ghidra-project change.
Port write set: `include/fifa96_loader/fifa96_window.h`,
`src/fifa96_loader/fifa96_window.c`, `tests/test_window.c`, `CMakeLists.txt`,
plus the `fifa96_scene_clip_edges` rename in
`include/fifa96_loader/fifa96_scene.h`, `src/fifa96_loader/fifa96_scene.c`,
`tests/test_scene.c`.

## 9. Open legs

1. **`[0x4E58C]`** (camera value, `-0xF1` centered) and `FUN_00053D58`'s gate
   semantics.
2. **`FUN_000441D8`/`FUN_00053240`** secondaries (0x4B0E0/0x4B0E4/0x4B0E8,
   the 0x6798 cache; pitch-area x/y setup) are cited only.
3. **0x67B4/0x67BC** resolution tables are BSS at rest; the runtime populator
   is not located.
4. **`FUN_00043B48`/`0x439D0`/`0x43F68` transforms** beyond the derived
   stores (scroll/scale math) are not decomposed.
5. **Window A/B roles**: A is a 160×100 surface activated from the live match
   (§4); how its smaller page is presented/scaled is not derived.
