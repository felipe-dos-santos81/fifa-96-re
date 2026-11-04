# FU-101 — view variants, timed transitions and the descriptor mutation

Follow-on to FU-99 §6 leg 2 (the camera transition/animation players), FU-100
§8 legs 2/5 (the state writer and `desc+0x50`), and FU-100 §3 (the class-3
handler's 100/120 constants). This slice maps the **(slot, variant)** view
selector behind `FUN_0004F1C8`, the timed six-field transition player
`FUN_0004F8C8`, its four producers, and the runtime mutation of the camera-type
descriptor.

Result in one line: **the 0x7508 camera views are selected by the seven
presets in `0x74D0`/`0x74EC` (`FUN_0004D04C`: (slot, variant) pairs
`{1,1,0,0,2,2,5}` × `{0,1,0,1,0,1,1}`); `FUN_0004F1C8` re-parameterizes the
current type descriptor's `[0]` from `[0x7E0C + variant*4]` and `+0x50` from
the slot's C-table + `variant*0x20`; view changes run through the timed
six-field interpolation `FUN_0004F8C8(camera, block, duration)` (origin
`0x4E4F8`, delta block `0x8BD4..`, progress `0x7DE0` += `FUN_00049388`,
`FUN_0004F830` ease curve, `[0x7DD8]` flag), fed by `FUN_0004CC98` /
`FUN_0004FD50` (cut to camera 9), `FUN_0004FDA4` (random nine-block cut) and
the drivers `FUN_0004CEF4`/`FUN_0004CF7C`; the state `[0x4E57C]` is
initialised by `FUN_000537F8` (0x10 when the mode byte `[0x57A4A]>>24 ==
0x10`) and advanced by the state-0x0B case.**

## 1. The view selector `FUN_0004D04C` and the (slot, variant) tables

`FUN_0004D04C(EAX = preset)` clamps `EAX` to 0..6 and reads two parallel
tables (`0x4D060..0x4D086`):

```
0x4d060 TEST EAX,EAX ; JL 0x4d069 ; CMP EAX,0x7 ; JL 0x4d06b ; XOR EAX,EAX
0x4d06b MOV ESI,[EAX*0x4 + 0x74d0]      ; camera slot
0x4d072 MOV EAX,[EAX*0x4 + 0x74ec]      ; variant
0x4d079 MOV [0x7dd0],ESI
0x4d07f MOV [0x7dd4],EAX
0x4d084 CMP EDX,ESI ; JNZ 0x4d08c ; CMP EBX,EAX ; JZ 0x4d0dd
```

`read_memory 0x1074D0`/`0x1074EC` (28 B each):

| preset | 0 | 1 | 2 | 3 | 4 | 5 | 6 |
|--------|---|---|---|---|---|---|---|
| slot `0x74D0` | 1 | 1 | 0 | 0 | 2 | 2 | **5** |
| variant `0x74EC` | 0 | 1 | 0 | 1 | 0 | 1 | 1 |

so the slots `{0,1,2,5}` of FU-100 §1 are exactly the four view cameras and
`[0x7DD4]` is a two-valued variant (0/1) per view. On a change
(`0x4d08c..0x4d0db`) the selector copies the entity `[0x9A70]`'s x/y/z into
the selected camera's `+0x34/+0x38/+0x3C` **and** `+0x40/+0x44/+0x48`,
calls `FUN_0004F1C8`, then steps `FUN_0004D908(camera)` **20 times**
(`0x4d0c6..0x4d0db`: `IMUL EAX,[0x7DD0],0x70 ; INC EDX ; CALL ; CMP EDX,0x14`)
to settle the view. The tail selects `&0x7968 + slot*0x70` — **not**
`&0x7508 + slot*0x70` — as `[0x7DC8]` (`0x4d0dd..0x4d0fe`) and re-runs
`FUN_0004F1C8`: the current-view pointer addresses the parallel record array
at `0x7968` (FU-97 §1) on this path, while `FUN_0004CEF4` §4 addresses the
camera objects at `0x7508`. The record-array semantics are an open leg.

Decompiler note: Ghidra renders these strides as `0x1c`; the disassembly is
`LEA EAX,[EDX*8]; SUB EAX,EDX; SHL EAX,4` = `EDX*0x70` (`0x4d092..0x4d09e`,
`0x4d0e3..0x4d0f5`, `IMUL ... 0x70` at `0x4d0c6`) — the FU-97/FU-100 `0x70`
stride stands.

## 2. `FUN_0004F1C8` — the descriptor mutation

```
0x4f1cb MOV EDX,[EAX + 0xc]              ; camera type
0x4f1ce TEST EDX,EDX ; JL ret ; CMP EDX,0x6 ; JGE ret
0x4f1d7 MOV EAX,[EDX*0x4 + 0x8b64]       ; type descriptor
0x4f1de MOV EDX,[0x7dd4]                 ; variant
0x4f1e4 MOV EDX,[EDX*0x4 + 0x7e0c]       ; variant parameter
0x4f1eb MOV [EAX],EDX                    ; descriptor [0]
0x4f1f3 MOV ECX,[0x7dd0]                 ; slot
0x4f1f9 SHL EDX,0x5                      ; variant*0x20
0x4f1fc CMP ECX,0x1 ; JNZ ; MOV EBX,0x862c
0x4f208 TEST ECX,ECX ; JNZ ; MOV EBX,0x85ac ; JMP
0x4f213 MOV EBX,0x85ec
0x4f218 ADD EBX,EDX
0x4f21a MOV [EAX + 0x50],EBX             ; descriptor +0x50
```

So the live descriptor is re-parameterized per view: `desc[0]` gets
`[0x7E0C + variant*4]` and `desc+0x50` gets the slot's C-table base
(0 → `0x85AC`, 1 → `0x862C`, else → `0x85EC` — the three FU-100 §5 bases)
plus `variant*0x20`. `read_memory 0x107E0C` (32 B):
`{0x340, 0xB4, -0x450, 0x60, -0x7F0, 0x450, 0x3C0, 0x7F0}` — variant 0
restores the static `0x340`; the deeper entries are further variants (the
2..7 values would index slot/variant pairs the selector never produces).

This closes FU-100 §8 leg 5 as far as: `desc+0x50` is a **0x20-stride record
table**, base selected by the camera slot, index by the variant; it is written
here, not read in the `FUN_000505D0` band (FU-100 §5 stands).

## 3. `FUN_0004F8C8` — the timed six-field transition

`FUN_0004F8C8(EAX = camera, EDX = start block, ECX = duration, EBX = target
block, stack = param_3)`:

* with a start block (`param_2 != 0`): copies its six fields into the camera
  `+0x10/+0x14/+0x18/+0x58/+0x5C/+0x4C` and into the origin block
  `0x4E4F8..0x4E510`; with a target block (`EBX != 0`): stores the six signed
  deltas into `0x8BD4/0x8BD8/0x8BDC/0x8BE0/0x8BE4/0x8BE8` (yaw/pitch wrapped
  through `& 0xFFFF` with `0x8000` bias) and arms the timers
  `[0x7DE4] = duration*10`, `[0x7DE0] = 0`, `[0x7DE8] = param_3*10`;
* per frame (`EAX != 0`, no blocks — called by the state-0x0B case as
  `FUN_0004F8C8(camera, 0, 0)`): if `[0x7DE8]+[0x7DE4] <= [0x7DE0]` the
  transition is done (`[0x7DD8] = 0`, return true); otherwise
  `[0x7DE0] += FUN_00049388()` (clamped to the span) and each of the six
  fields is `origin + FUN_0004f830(progress, duration)`;
* with `EAX == 0` (called as `FUN_0004F8C8(0, 0, 0)`) only the completion
  test runs.

`FUN_0004F830(progress, duration)` is the 3-phase ease curve (quadratic ramp
up, linear, quadratic ramp down, via `FUN_000a2ad8`/`FUN_000a2aee`); the
operand mapping (`in_EAX` delta, `EBX` midpoint) is not fully derived (open
leg). `FUN_00049388` is the same elapsed-timer used by the class-3 handler
(FU-100 §3).

## 4. The view-change producers and drivers

* **`FUN_0004CC98`** — cinematic cut (FU-99 §2): forces camera 9 (`0x78F8`),
  copies the previous camera's six fields, builds the start block from
  `0x872C` and the target block from `0x80B4`/`0x81A4` (`FUN_0004B7D0` /
  `FUN_0004B6FC`, the FU-100 §5 pair), flips yaw/z when `[0x9A70+8] < 0`
  (`0x18000 - yaw`, `-z`), then `FUN_0004F8C8(10, start, 0xF)`.
* **`FUN_0004FD50`** — cut to camera 9 plus field copy, then
  `FUN_0004F8C8(10, 0, 0x28)` (a 40-unit transition from the copied fields).
  It is called by the undefined `0x51xxx` block right after
  `MOV [0x4E57C],EAX` at `0x514A3` (see §5).
* **`FUN_0004FDA4`** — random cut: if the current camera's dword 0 is not 9,
  `[0x8BF4] = FUN_000566d0() % 9`; forces camera 9;
  `FUN_0004F8C8((rnd&3)+4, &0x875C + [0x8BF4]*0x18, (rnd&7)+0x14)` — nine
  six-dword blocks at `0x875C` (the first: `{138, 140, 611, 45756, 411, 2888}`)
  with a 40..70-unit duration.
* **`FUN_0004CEF4`** — driver: `FUN_00036BC8()` decides the camera-9 arm;
  else selects `&0x7508 + [0x7DD0]*0x70`, `FUN_0004F1C8()`, then
  `XOR EAX,EAX; XOR EBX,EBX; CALL 0x505d0` (flag 0, `[0x7DD8] = 0` after).
* **`FUN_0004CF7C`** — driver: saves the current six fields on the stack;
  selects `[0x7DD0]`; `XOR EAX,EAX; MOV ECX,0xa; CALL 0x505d0` (the 0xA
  survives in `ECX` — `FUN_000505D0` saves/restores it, `0x505d1`/`0x51064`);
  assembles the target block `{[0x4E470], [0x4E474], [0x4E478], [0x4E4B8],
  [0x4E4BC], [0x4E4AC]}` and calls `FUN_0004F8C8(camera, saved, 0xA)` with
  `PUSH 0x1E`, then `[0x7DD8] = 1`.
* **`FUN_0004D2D4`** — per-frame driver: copies the `[0x9A84]` entity into the
  camera fields, clamps `cam[+0x34]` (dword 0xD) to ±0x720 and `cam[+0x3C]`
  (dword 0xF) to ±0xB10, runs the selector/tick (`FUN_000505D0()` when
  `cam[1] < 4` and `[0x7DD8] == 0`, else `FUN_0004F8C8(0,0,0)` or
  `FUN_0004D908`), then `FUN_0004C8D0(camera, cam[+0x50])` into
  `cam[+0x4C]`/`cam[+0x50]` with a `[0x200, 0x3A00]` clamp.

The `FUN_000505D0` flag semantics are now pinned: the argument is **EAX →
ECX** (`0x505d9 MOV ECX,EAX`), and the fall-through guard `TEST ECX,ECX; JZ`
skips the class handler when it is 0, while case bodies that set `ECX` (e.g.
`0x50859`) or jump straight to `0x5104B` force the call. The two explicit
drivers pass 0; `FUN_0004CEF4` even zeroes both `EAX` and `EBX`.

## 5. The state `[0x4E57C]` — init and writers

`FUN_000537F8` (called by `FUN_0004A228`) zeroes the state block
`[0x4E570..0x4E584]`, sets `[0x4E584] = 0x50`, `[0x4E588] = FUN_0004B380()`,
copies a block from `0x8C3C` to `0x4E5A8`, and:

```
iVar1 = FUN_0004b380();          ; [0x57A4A] >> 24 — a mode byte
if (iVar1 == 0x10) {
    [0x4E588] = 0xc; [0x4E574] = 1; [0x4E57C] = 0x10;
}
```

so state 0x10 is the `[0x57A4A]>>24 == 0x10` mode (its case entry is the
shared state-6/0x10 `0x50E43`, which calls `FUN_0004F274` and derives a value
from `[0x9A70+8] > 0`). Further inits: `[0x4E680] = 300`,
`[0x4E5D4] = 180000`, `[0x4E69C] = -1`, `[0x4E5F0] = 1`, `[0x4E538] = 1`.

The `get_xrefs_to 0x4E57C` write sites are `0x53826`/`0x5387D` (both in
`FUN_000537F8`) and seven sites in the **undefined** `0x514A3..0x51B56`
region: `0x514A3`, `0x518F3`, `0x51819`, `0x519A3`, `0x51AD0`, `0x51B2D`,
`0x51B56`. `0x514A3` is `MOV [0x4E57C],EAX; CALL 0x4FD50`, i.e. the state
write and the camera-9 cut are one step. The region's function boundaries and
the state values it writes are an open leg.

The state-0x0B case (`0x50F6B`) is the transition driver: `FUN_0004F274`, then
`FUN_0004F8C8(camera,0,0)` (tick) and `FUN_0004F8C8(0,0,0)` (completion test).

## 6. Closures / errata

* **FU-99 §6 leg 2 — closed.** The three players are: `FUN_0004CC98`
  (cinematic cut to camera 9 with the `0x872C`/`0x80B4`/`0x81A4` blocks),
  `FUN_0004FDA4` (random cut into the nine `0x875C` blocks) and
  `FUN_0004F8C8` (the timed six-field interpolation, duration in 10-unit
  ticks, `[0x7DD8]` active flag).
* **FU-100 §8 leg 2 — partially closed.** The state source is
  `FUN_000537F8` (mode 0x10) plus the `0x51xxx` write sites; the guard is the
  `EAX` argument (`XOR EAX,EAX` in both explicit drivers) with case-body
  overrides.
* **FU-100 §8 leg 5 — partially closed.** `desc+0x50` is written by
  `FUN_0004F1C8` as a slot-based C-table record (`variant*0x20`); it is still
  not read in the `FUN_000505D0` band. `desc[0]` is likewise rewritten from
  `0x7E0C`.
* **FU-100 §3 note — confirmed.** The class-3 handler's 100/120 constants are
  the `0x8B98`/`0x8B9C` dwords; the transition durations here are 10-unit
  ticks (`[0x7DE4] = duration*10`).
* **Erratum — decompiler stride at `0x4D04C`.** Ghidra prints the camera
  walks as `*0x1c`; the disassembly is `*0x70` (`LEA [EDX*8]; SUB EDX; SHL
  4`, `IMUL ...,0x70`). No map row is affected; recorded for the next reader.

## 7. Provenance

Ghidra MCP on `/fifa96_le.bin`: `disassemble_function` 0x4D04C (67 insns),
0x4CEF4 (30), 0x4F1C8 (30), 0x4CF7C (63), 0x6401C (4);
`decompile_function` 0x4CEF4, 0x4CF7C, 0x4D2D4, 0x4F8C8, 0x4CC98, 0x4F830,
0x4FD50, 0x4B380, 0x4D04C, 0x537F8, 0x4FDA4, 0x4F1C8, 0x53D50;
`disassemble_bytes` 0x50E43 (48 B), 0x50F6B (48 B), 0x514A3 (24 B);
`get_xrefs_to` 0x4E57C (10), 0x7DD4 (5); `get_function_callers` 0x537F8 (1);
`read_memory` 0x1074D0/0x1074EC (28 B each), 0x107E0C (32 B), 0x10875C
(72 B), 0x14E470 (24 B), 0x14E57C (4 B). Address mapping as FU-88 (data
immediate `A` → storage `A+0x100000`; stored code pointers and inline `CS:`
tables → `+0x10000`). Analysis-only: no port, capture-rig, ISO, or
Ghidra-project change.

## 8. Open legs

1. **`FUN_0004F830`** ease-curve operands (`in_EAX` delta, `EBX` midpoint)
   and the exact six-field interpolation for yaw/pitch (the wrap handling in
   `0x8BE0`/`0x8BE4`).
2. **The `0x514A3..0x51B56` undefined region**: its function boundaries, the
   state values written there, and the seven write sites' conditions.
3. **The parallel record array `0x7968`**: why `FUN_0004D04C` points
   `[0x7DC8]` at `&0x7968 + slot*0x70` while `FUN_0004CEF4` uses
   `&0x7508 + slot*0x70`, and what fills the records.
4. **`[0x7DD0]`/`[0x7DD4]` producers**: `FUN_0004D04C` (preset 0..6),
   `FUN_0004D04C`'s other writer at `0x4CD3D`, and `FUN_0004D04C`'s caller
   (the view menu?).
5. **`FUN_0004D908`** (the 20× settle step) and **`FUN_0004C8D0`** (the
   `cam[+0x4C]`/`+0x50` feed) bodies; `FUN_0004F274` (called by the state
   cases) semantics.
6. **Entity/state globals**: `[0x9A70]` (position + `+8` sign), `[0x9A84]`,
   `[0x7DD8]` transition flag consumers, `cam[+0x00]` (the `== 9` marker),
   and `FUN_000566d0`'s RNG.
7. **The `0x8C3C` init copy** in `FUN_000537F8` (length comes from the
   caller) and the `0x4E470..0x4E4BC` block assembly in `FUN_0004CF7C`.
8. **The nine `0x875C` blocks and `0x872C`/`0x80B4`/`0x81A4`** as data (only
   the first `0x875C` entry was read); the `0x7E0C` variant table's entries
   2..7 are unreachable from the seven presets.
