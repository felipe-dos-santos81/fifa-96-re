# FU-152 — presentation residual

Provenance: recon draft w7-b4, phase-7 wave-7, frozen 2026-10-09; evidence
review: PASS after correction pass. Read-only on authoritative program
**`/FIFA96.EXE`** (Ghidra MCP; never `/fifa96_le.bin`, never `/fifa96.exe`).
No writes outside this draft; no commits. Fresh xref counts for every census
claim. Extends FU-148 (frozen) §legs 1/5/6/9/10/11/13; does not repeat its HUD
core.

**Address model.** In `/FIFA96.EXE` code addresses equal the FU-doc link
addresses; data immediates render as true flat addresses (`0x109A98`,
`0x14E660`, `0x108B80`); the +0x100000 rule only applies when mapping a
flat-bin FU quote (`0x9A98` -> `0x109A98`). All `program` arguments are
`/FIFA96.EXE`. Byte quotes below are raw `read_memory`/`disassemble_bytes`.

---

## 1. Scope

Per the phase-7 plan Track B item B4 (the five residual chains beyond FU-148's
HUD core):

1. **Replay/cutscene dispatch rows** — every row of the overlay presenter
   `FUN_000565BC` and its presenters (`FUN_000560C8`, `FUN_0005619C`,
   `FUN_000542D4`, `FUN_000550E4`, `FUN_00056518`, `FUN_000564A0`,
   `FUN_00064C34`), with the `[0x109A98]` replay state machine and what draws
   when.
2. **Residual HUD overlays** — the presenter siblings of `FUN_00055C24`:
   substitution strip/number row, period/overlay screens, extra-time screen.
3. **Camera handlers** — the four `0x108B80[0..3]` bodies, the camera
   record/behavior tables, and the `FUN_000505D0` pose-feed details (FU-96
   §6 legs 1/3).
4. **FU-71 event machine bodies** — `FUN_00071C94` + `FUN_00070544` /
   `FUN_000709D0` / `FUN_00070DE0` / `FUN_00071DF4` (FU-148 §2.1(c) partial).
5. **Palette residual** — translation-pool identity
   (`FUN_00046F80`/`FUN_00049138`), slot consumer
   `FUN_00048DC0 -> FUN_000CE980 -> 0x114720`, shade-cube blender.

Not re-derived: the HUD draw (`FUN_00055C24`, FU-148 §1/§10 complete), the
replay ring/queue mechanics (FU-108/109/110/111/116), the FU-71 integrator
`FUN_000736AC` (ported), the formation-id producer (FU-148 §3, not B4 scope
per the assigned brief).

---

## 2. Evidence floor

### 2.1 Fresh censuses (this slice, `/FIFA96.EXE`)

| query | result |
|---|---|
| `get_function_xrefs 0x565BC` | **3** (`0x49681` `FUN_000495B0`, `0x58C9F`/`0x58D32` `FUN_00058BC0`) |
| `get_function_xrefs 0x560C8` | **1** (`0x565F0`) |
| `get_function_xrefs 0x5619C` | **2** (`0x565F7`, `0x56601` — sides 0/1) |
| `get_function_xrefs 0x542D4` | **1** (`0x56648`) |
| `get_function_xrefs 0x550E4` | **1** (`0x5664D`) |
| `get_function_xrefs 0x56518` | **1** (`0x56685`) |
| `get_function_xrefs 0x564A0` | **1** (`0x5662A`) |
| `get_function_xrefs 0x64C34` | **1** (`0x56631`) |
| `get_function_xrefs 0x56690` (E660 setter) | **1** (`0x56E41` `FUN_00056CF4`) |
| `get_xrefs_to 0x14E660` | **3** — WRITE `0x53901` (`FUN_000537F8` reset), READ `0x5667C` (presenter), WRITE `0x56690` (setter) |
| `search_instructions 109a98` | **29** sites, all in `0x63CBC..0x64EC3` |
| `get_xrefs_to 0x109A98` | 29 (same set; 8+ writers, 5 predicate readers) |
| `search_instructions 109ac4` | **6** (`0x64478/82` driver, `0x646E3` arm, `0x64C3D` HUD gate, `0x64E8F/9F` exit) |
| `get_function_xrefs 0x505D0` | **3** (`0x4CF51` `FUN_0004CEF4`, `0x4D009` `FUN_0004CF7C`, `0x4D3D9` `FUN_0004D2D4`) |
| `get_function_xrefs 0x71C94` | **11** (`0x71B8A` `FUN_0007131C`, `0x70DD1` `FUN_00070C08`, `0x7A219` `FUN_0007A084`, `0x6FA62` `FUN_0006E8E8`, `0x75DC4`, `0x77423`, `0x7F4E7`, `0x82A6C`, `0x77DC6`, `0x7EFCF`, `0x7F0D1`) |
| `get_xrefs_to 0x1577C0` / `0x1577C2` | **91** / **54** (matches FU-148) |
| `get_function_xrefs 0x46F80` | **1** (`0x49160`) |
| `get_function_xrefs 0x49138` | **2** (`0x4AE55` `FUN_0004AD4C` alloc, `0x4AF0A` `FUN_0004AEA4` free) |
| `get_xrefs_to 0x107290` (pool ptr) | **4**, all inside `FUN_00049138` — pool has no other consumer |
| `get_function_xrefs 0x48DC0` | **2** (`0x56478` `FUN_000563D0`, `0x5721A` `FUN_00057158`) |
| `get_function_xrefs 0xCE980` | **11** (`0x490FE`, `0x48EC1`, `0x4905B`, `0x49072`, `0x490D6`, `0x62FCD`, `0x490AC`, `0x490BF`, `0x5486E`, `0x5493B`, `0x54960`) |
| `search_instructions 14720` | **31** sites (writer `0xCE985`, reader family; §2.10) |
| `get_function_xrefs 0xA0AA0` | **1** (`0xA12E6` `FUN_000A129C`) |
| `get_function_xrefs 0xA0CB8` | **1** (`0xA0C82`, the builder `FUN_000A0AA0`) |
| `get_function_xrefs 0xA0E3C` | **2**, both from `FUN_000A10E0` |
| `get_function_xrefs 0xA10E0` | **2**, both from `FUN_000A0E3C` |
| `get_function_xrefs 0xA154C` | **3** (`0x48C1D`, `0x48CD5`, `0x48C52`) |
| `get_xrefs_to 0x10365C` / `0x103668` | **1** each (`0xA12A6` / `0xA12C2`) |
| `get_xrefs_to 0x1587E7` | **5** — WRITE `0x88824` (`FUN_000886D4`), WRITE `0x8AE5F` (`FUN_0008A938`), READ `0x89BE7`, READ_WRITE `0x89D2C`, WRITE `0x89D4F` |
| `get_xrefs_to 0x1587DA` | **6** — DATA `0x4BDC6`/`0x4BDDC` (`FUN_0004BD38`), WRITE `0x8AE85` (`FUN_0008A938`), WRITE `0x88846` (`FUN_000886D4`), DATA `0x8AC76`, DATA `0x89D61` |

### 2.2 Presenter dispatch — `FUN_000565BC` (fresh decompile, full body)

```
FUN_0004B380();                                   ; mode = [0x157A4A]>>24
if ([0x14E538] == 0) {                            ; R1 gate A
  if (FUN_00037AE4() == 0) {                      ; menu probe
    if (FUN_0006400C() != 0 && mode not in {0xC,0x13,0x14}) {
      FUN_000560C8(); FUN_0005619C(0); FUN_0005619C(1);      ; R1 substitution strip
    }}}
if (FUN_00063FD0() != 0) {                        ; [0x109A98] & 0x80
  if (FUN_00063FDC() == 0) {                      ; != 0x81
    if (FUN_00053D58() == 0) {                    ; not in replay window 0xF1..0x168
      if ([0x14E58C] == 0) FUN_00064C34();        ; R2 replay HUD
      else                 FUN_000564A0();        ; R2 blink caption
    }}}
if ([0x14E688] == 0) {                            ; not paused/suspended
  if ([0x14E674] & 0x8000) { FUN_000542D4(); FUN_000550E4(); }   ; R3 menu/cutscene overlay
  if ([0x14E510] == 0) {
    if (FUN_0006400C() != 0) {
      if (FUN_0004B5F4() < 4 && [0x14E59C] == 0)
        FUN_00055C24();                           ; R4 the match HUD (FU-148)
    }}
  if ([0x14E660] != 0) FUN_00056518();            ; R5 ball row (dormant, §2.5)
}
```

Gate predicates, fresh decompiles:

```
FUN_00063FD0: return [0x109A98] & 0x80            ; replay family active
FUN_00063FDC: return [0x109A98] == 0x81           ; armed/intro state
FUN_00063FF0: return ([0x109A98]&0x80) && FUN_00053D7C()==0   ; replay phase 0
FUN_0006400C: return [0x109A98] == 0              ; live (not replay)
FUN_0006401C: return [0x109A98] == 0x82           ; replay playing
FUN_00053D58: return ([0x14E58C]-0xF1 < 0x78) && [0x14E58C] != 0
FUN_0004B380: return [0x157A4A] >> 24             ; match mode (R1 exclusion)
FUN_00053D50: return [0x14E57C]                   ; view mode (camera switch)
FUN_00053D7C: return [0x14E58C]                   ; replay phase counter
```

### 2.3 Replay row presenters

**`FUN_000564A0` (blink caption), fresh decompile:** `[0x108FC4] +=
FUN_000492CC()` (`= [0x10732C]`, a tick); when below `0x14` it returns for
counts 10..19 and draws for 0..9; at `>= 0x14` it resets to 0 and draws. Draw
= `FUN_00019E9C(1); FUN_0001771C(0x18C); FUN_000A1720();` (colour 1,
string-table id `0x18C`, string at the current cursor via
`FUN_000986B4`+`FUN_000A06FC`). So it is a ~1 Hz blinking caption in the
0x82..0x86 replay states while `[0x14E58C] != 0` outside the 0xF1..0x168
window (ramp phases 1..0xF0 / 0x169+).

**`FUN_00064C34` (replay HUD), fresh decompile:** gated by `[0x109AC4] != 0`
(the HUD-arm flag: written `0x646E3` `FUN_00064690`, `0x64482` driver,
cleared `0x64E9F` `FUN_00064E8C`). Selects a caption by `[0x109A8C]` (0..5
replay-camera index): 1 -> `FUN_0001771C([0x109AD4])`, 2 ->
`FUN_0001771C([0x109AD8])`, else `FUN_0001771C([0x109AD0 + i*4])`; formats
with the string at `0x102110`/`0x102118`/`0x102120` (fresh bytes:
`31 25 73 20 31 00` -> "%s 1" at `0x10210F`+1, `%s 2` at `0x102118`, `%s` at
`0x102120`; preceded by `"replay loaded\n"` at `0x102100`), draws it with
`FUN_00012FE4(0x6A, ...)`; switches on `[0x109A98]`: `0x82->[0x109ACC]=0`,
`0x83->FUN_00016C08([0x155C78])`, `0x84->[0x155C8C]`, `0x85->[0x155C88]`,
`0x86->[0x155C7C]`, then `FUN_00015D40(handle, mode)` (button highlight);
progress bar `FUN_000642B0() (= [0x109A94]*100/ *[0x109AC0])` scaled by
`FUN_0003773C` / `FUN_00037778(0xBA,...)` and drawn by `FUN_0009F7F0`;
then `FUN_000196A0`, `FUN_00012940(0)`, `FUN_0003773C(7/0x2C/0xF6)`,
`FUN_0009AFD0`, `FUN_000196DC`. (FU-111 §5 described this body against the
flat bin at `0x64C35`; the EXE function starts `0x64C34`.)

**State driver `FUN_000642FC` (fresh decompile):** `[0x109A98]==0` -> return;
`0x80 -> 0x81`; the `0x81` arm: when `[0x14E58C]==0` runs `FUN_000478FC` +
`FUN_00044D7C`, then `[0x109A94]=0`, `FUN_00063CBC`, `[0x109A8C]=0`,
`FUN_0004D134(0)` (camera 0); payload (`0x82..0x87`) reads `FUN_00064AA4` (button mapper) +
`FUN_00045025` (held pad); button value 8 -> `0x85`, 4 -> `0x84`, 2 ->
`0x86`; value 1 toggles `0x82 <-> 0x83`; button `0x20` cycles
`[0x109A8C] = ([0x109A8C]+1)%6` and calls `FUN_0004D134([0x109A8C])`;
exit (`FUN_000451F1(2,..) != 0` or bit `0x80`) runs `FUN_000543D4`,
`FUN_00015594(0)`, `[0x104F60]=0`, `[0x109A98]=0`, `FUN_0004AFA0([0x109AC4])`,
`FUN_0004CEF4`, `FUN_00036C70`, `FUN_00053D9C`; state cases: `0x82` idles,
`0x83` advances (`[0x109AA8] += delta` through `FUN_00063D34` /
`FUN_00063CBC` / `FUN_0006428C`), `0x84`/`0x85` step via `FUN_00063E54`,
`0x86` waits `[0x109A9C]` frames (`[0x109AA0]` reload) then `FUN_00063CBC`.

**Replay camera selector `FUN_0004D134(index)` (fresh decompile):** index 0 ->
camera record `&0x107968 + [0x107DD0]*0x70` and copies the `[0x109A84]` pose
(+0x10..+0x18, +0x4C, +0x58, +0x5C) into it; 1/2/3/6 -> record `0x107CE8` with
pose sub-index 5/6/7/8 through `FUN_0004DDA8`; 4 -> record `0x107B98` plus
`FUN_0004DB38(0x107B98,0,0,[0x108B7C])` (=500) and again after; 5 -> record
`0x107B28` + `FUN_0004D98C`. Records (fresh bytes): `0x107B28` = `04 00 00 00
FF FF FF FF 04 ...`, `0x107B98` = `05 .. FF .. 05 .. FF`, `0x107CE8` = `08 ..
FF .. 08 .. FF`, `0x107968`+1*0x70 (with `[0x107DD0]=1`).

### 2.4 Cutscene machinery

There is no cutscene-specific presenter: during a cutscene the match loop
skips the world update (`[0x14E574]` suspend, FU-69 §2.5) but the same
presenter rows run. The cutscene-time draws are R3 (`FUN_000542D4` +
`FUN_000550E4`) and, if a substitution/refresh state is set, R1.

**`FUN_000542D4` (overlay timeout), fresh decompile:** gate
`[0x14E698]*[0x14E69C] >= 0`; `[0x14E684] += FUN_0004937C()` (`= [0x107300]>>2`);
if `[0x14E5C8]==0` compares against `[0x14E680]`, else accumulates
`[0x14E5D8]` against `[0x14E5D4]`; on timeout calls
`FUN_00053E08(0x14E674)` (and `FUN_00053E08(0x14E5C8)` if the second overlay
is active). `FUN_00053E08(ptr)` flips the sign of `[ptr+0x24]` against
`[ptr+0x28]` (timer direction toggle).

### 2.5 Row R5 `FUN_00056518` — dormant ball row

Fresh decompile/disasm: blits `FUN_0009BB20(scale, [0x14E65C], [0x14E664],
[0x14E668])` (`scale = 0x10000*zoom>>16`), then `FUN_0004AFB8(0x29)` ->
`FUN_000A1920(handle, [0x108FC8])` -> `FUN_0009BB20(scale, frame,
[0x14E66C], [0x14E670])`; a `FUN_000CB2A4()` (`= [0x112E88]`) timer advances
`[0x108FC8] = ([0x108FC8]+1)&3` every >=8 ticks. The `[0x14E65C..0x14E670]`
rect is part of the same `FUN_00053240` layout (fresh: `[0x14E664] = x1 -
0x2C*scale`, `[0x14E668] = y0 + 3*scale`, `[0x14E66C] = [0x14E664] +
0xC*scale`, `[0x14E670] = [0x14E668] + 9*scale`).

Resource 0x29 identity: name table `0x107370` entry 41 (fresh
`read_memory 0x107414`) = `58 1B 10 00` -> `0x101B58` = `"ball"` (fresh
bytes `62 61 6C 6C 00`); the retail container resolves `ball.fsh` to entry 37
(fresh: `fifa96_play sprite tests/golden/gameart0.pvi --name ball.fsh
--print-summary` -> `entry=37 name=ball.fsh frames=8 frame=0 16x16`).

**Dormancy proof:** `[0x14E660]` has exactly 3 refs. The only nonzero-capable
writer is `FUN_00056690(p) { [0x14E660] = p; }`, whose only caller is
`FUN_00056CF4` tail — disasm `0x56E1B: XOR EDX,EDX`, then the MOVSD copy loop
(no EDX writes), `0x56E39: MOV EAX,EDX`, `0x56E3B: MOV [0x109004],EDX`,
`0x56E41: CALL 0x56690` -> always 0. The other writer is the reset
`FUN_000537F8` `0x53901`. Therefore `FUN_00056518` never draws in this image.

### 2.6 Substitution strip (R1)

**`FUN_00053240` (window->layout setter, FU-93 §1) also writes the strip
block** (fresh decompile tail; stride 0x1C):

```
[0x14E53C] = y0                          [0x14E558] = y0          ; side 1 base +0x1C
[0x14E540] = h  (h = Frames.fsh frame5 [+2]>>16, *2 when wide)
[0x14E55C] = h
[0x14E544] = x0 + (4*scale>>16)          ; left-aligned x
[0x14E560] = x1 - (4*scale>>16) - w      ; right-aligned x (w = measured name width)
[0x14E548] = y0 + (12*scale>>16)         [0x14E564] = same
```

**`FUN_000560C8` (fresh decompile):** fills the 0x14E53C row and draws a
centred `"%d - %d"` (format at `0x101E4C`, fresh bytes `25 64 20 2D 20 25 64`)
whose two ints come from `FUN_0004BDF8(&local_14,&local_18)` (overwriting the
window-derived scratch); draw =
`FUN_00054640(str, [0x14E548] + 2*scale>>16)` (centre: `x = 0xA0 - w/2*scale`
narrow / `0x140 - ...` wide) -> `FUN_000544B4(str,x,y)`.

**`FUN_0004BDF8` (fresh decompile):** if `[0x1587E6]>>8 < 5` ->
`p1 = [0x1587D5]>>24`, `p2 = byte fields at 0x1587D9/0x1587D4`; else
`p1 = [0x1587E1]._3_1_`, `p2` from `0x1587E5`/`0x1587E1`. These are bytes of
the match-record pointer block `[0x1587D4]` (FU-139/FU-145: a record pointer
whose `+0x826` byte is the team side; FU-145 lists the record identity as an
open leg).

**`FUN_0005619C(side)` (fresh decompile):** 5 slots per side,
`c = FUN_0004BD38(side, i)`; states 1/2/3 call `FUN_0009B3A0()`
(= `FUN_0009B0D0`, the planar sprite blit) per slot; then the team name
`FUN_00011BEC(side, 0, [0x14E540+side*0x1C])`, measured (`FUN_000986B4`),
centred in the row
(`x = [0x14E544+side*0x1C] + ((row_dim*scale - name_width*scale) >> 1)` where
`row_dim = [0x14E638]+2 >> 16`), clamped to `[[0x108DE4]+2, [0x108DE8]-w]`,
drawn `FUN_000544B4(name, x, [0x14E53C+side*0x1C])`.

**`FUN_0004BD38(side, i)` (fresh decompile):** with `P = [0x1587D4]`,
`k = [0x1587E7] % 5`, `idx = i*2 + side`:
* `i == k` and `(*P + 0x826) == side` -> 3 (or 2 when mode `[0x157A4A]>>24 ==
  5` and `P == 0x157A9F`); 
* `i == k` and `side == [0x1587E3]>>24` -> `1 + ([0x1587DA + idx] != 0)`;
* `i < k` -> `1 + ([0x1587DA + idx] != 0)`; else 0.

Fresh writers of these cells are clear-only: `FUN_0008A938`
(`0x8AE54..0x8AE8D`: `[0x1587D4] = [0x1587CC + (arg>>16)*4]`, `[0x1587E6] = arg
byte`, zeroes `0x1587E7/0x1587D8/0x1587D9/0x1587E8` and the `0x1587DA` flag
loop) and `FUN_000886D4` (`0x88813..0x8884E`: zeroes `0x1587E7/0x1587E8/
0x1587E9/0x1587D8/0x1587D9` + the flag loop). So `k = [0x1587E7]%5 == 0` in
the image and only slot 0 can mark (state 1, or 3 for the current side).

`FUN_000544B4` (fresh decompile) is the two-pass text draw shared with the
HUD: two-pass when `(zoom==0x10000 && !wide) || zoom==0x20000`, colours 6
then 0 through `FUN_00019E9C` + `FUN_000A06FC`; otherwise a temporary scaled
surface (`FUN_0004A488` / `FUN_000CE6F0` / `FUN_000CBDB0` / `FUN_0005E1A4` /
`FUN_0009A16C`).

### 2.7 Menu/cutscene overlay presenter `FUN_000550E4`

Fresh decompile of the whole body (`0x550E4..0x55B52`): early-outs when
extra-time mode and `[0x14E674]&0xFF != 0x11`, or when replay-active
(`FUN_00063FD0`). Calls `FUN_0004937C` (tick) + `FUN_00053E5C` (timeout
probe); when the probe returns 0 and `[0x14E678]!=0`, it arms
`[0x14E674] = id | 0x8000`, resets `[0x14E684]`, and for ids
{5,9,0xB,0xC,0xD,0xE,0xF,0x12} also seeds the second overlay `[0x14E5C8]`.
Live body: `[0x14E534] = min(0, (y1 - [0x14E52C]) - 0x36)`; `[0x14E530] =
2*[0x14E534]` (confirms FU-148 errata 4); computes `local_2c/local_24/...`
from the `0x14E6xx` layout; zeroes via `FUN_00098484`; `FUN_000395CC(wide)`;
`local_30 = [0x14E674]&0xFF`; then `switch(local_30)`:

| case | draws (fresh decompile) |
|---|---|
| 0 | period announcement: `FUN_00054AE4(...)`, team names `FUN_00011BEC(0/1,0,0x14E6B4>>1)`, or string `FUN_0001771C(aiStack_74[period+1])` centred (`FUN_00054640`); `aiStack_74` copied from `0x151090` (fresh read = 24 zero bytes; runtime-filled) |
| 1,4 | player select list `FUN_0004BB2C`/`FUN_0004BEAC` via `FUN_00054D48` |
| 5,0xB,0xE | substitution/team screen: `FUN_0004B840(id)`, both team names `FUN_00011BEC(0/1,0,[0x14E6B4])`, score `FUN_0004B5DC` + `"%d"` (`0x101DF4`), sub-line strings `0x19D/0x19F` (`"%s"` `0x101DF8`), player names `FUN_000546F8` (the `0x114720` text path), string `0x16F`/`0x1D3` |
| 6 | record info `FUN_0004BE30` -> `FUN_00054D48` |
| 7,0xC | period/extra-time string `FUN_0001771C` from `aiStack_74`/`0x510A8`-copied table (fresh `0x1510A8` read = zeros) |
| 8 | `FUN_0004BB5C` list |
| 9 | string `0x15E` centred + `FUN_00054F24` panel |
| 0xA | `FUN_0004BB2C` + string `0x166` |
| 0xD | `FUN_00054AE4(...,[0x108E04],...)`, strings `0x19B`, `"%2d - %s"` (`0x101DA8`), stat panels `FUN_0004BF0C`/`FUN_0004BF7C` |
| 0xF | player name + string `0x23A` |
| 0x11 | extra-time/aggregate: `FUN_0004B45C()==0` -> string `0x214` with `"%s %d:%02d"`-family `0x101E25`; else aggregate `FUN_0004B4C4` split >>10 / &0x3FF, string `0x101E18` |
| tail | always `FUN_000563D0()` (the generic animated-object draw, FU-85 §3: descriptor table `0x108E6C`, slot `[0x108FA8]`; its only caller) then `FUN_0009849C()` (font-state restore) |

### 2.8 Camera handlers `0x108B80[0..3]`

Fresh bytes `0x108B80` (16 B): `34 E8 04 00 / A8 E3 04 00 / 9C EC 04 00 /
38 DB 04 00` -> **`0x4E834`, `0x4E3A8`, `0x4EC9C`, `0x4DB38`**. (FU-148 §2.1
quoted `0x4ECA8`/`0x4DB3C` — corrected; the decompiler names are
`FUN_0004EC9C`/`FUN_0004DB38`.)

**Camera records.** Fresh disasm of `FUN_000505D0` `0x50633..0x50678`:
`EAX = EDI*7; SHL EAX,4` -> stride `0x70`; `ESI = [0x108B64 + [[0x107514 +
EDI*0x70]]*4]` (inner = camera `+0xC`, behavior index); `[0x107508 + idx*0x70]`
record fields, fresh bytes:

```
idx0 @0x107508: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 F8 FF FF A0 00 00 00 1E 00 00 00 ...
idx1 @0x107578: 01 01 01 01    idx2 @0x1075E8: 02 02 02 02
idx3 @0x107658: {3, -1, 3, -1}          idx4 @0x1076C8 (not read)
idx5 @0x107738: {5, 3, 5, -1}
```

So `[camera+4]` = handler selector (record 5 carries `3`), `[camera+0xC]` =
behavior index; the poser maps `[camera+4]==3 -> 5` before indexing the
behavior table, clamps a negative selector to 0. `PTR_DAT_00107DC8`
(`0x107DC8`) fresh = `0x107508`.
`[camera+0x10/0x14/0x18]` = position (image init `-1920, 160, 30` for all
records), `+0x58` yaw, `+0x5C` pitch, `+0x4C` ratio/roll.

**Behavior table** `0x108B64` fresh bytes: `6C 89 10 00 / C0 89 10 00 / 14 8A
10 00 / 68 8A 10 00 / BC 8A 10 00 / 10 8B 10 00` -> `0x10896C, 0x1089C0,
0x108A14, 0x108A68, 0x108ABC, 0x108B10` (6 x 0x54 = 84 B each). Fresh
`read_memory 0x10896C` (504 B) fields per block: `+0x00` int, `+0x04` int,
`+0x08` class, `+0x0C` pad, `+0x10..+0x2C` motion constants, `+0x30`, `+0x40`,
`+0x44` state, `+0x48` pose pointer, `+0x4C` pose pointer, `+0x50`
sub-record / alt pointer. Corrected observed pointers (review; the earlier map
was one field off):

| block | +0x08 class | +0x48 | +0x4C |
|---|---|---|---|
| 0 `0x10896C` | 3 | `0x107E2C` | `0x107F1C` |
| 1 `0x1089C0` | 1 | `0x10800C` | `0x1080FC` |
| 2 `0x108A14` | 3 | `0x1081EC` | `0x1082DC` |
| 3 `0x108A68` | 3 | `0x107E2C` | `0x107F1C` |
| 4 `0x108ABC` | 3 | `0x107E2C` | `0x107F1C` |
| 5 `0x108B10` | 3 | `0x1081EC` | `0x1082DC` |

Block 0's `+0x50` is `0x1085AC`; both `+0x48`/`+0x4C` arrays are non-NULL in
the image (the review corrected the earlier "(NULL in the image)" claim).

**Pose records** are 6 dwords `{x, y, z, yaw, pitch, angle}`. Fresh
`0x107E2C` (192 B) record 0 = `{-101, 488, 1100, 33900, 3900, 4608}`; fresh
`0x107F1C` record 0 = `{-100, 488, 1100, 33900, 3900, 4608}`; fresh
`0x10866C` (96 B, the `FUN_0004DDA8` replay pose table) record 0 =
`{3, 2800, 5000, 32828, 6400, 5376}`.

**`FUN_000505D0` pose feed (fresh decompile + disasm).** Early return when
`FUN_0006401C()` (`[0x109A98]==0x82`). `mode = FUN_00053D50()`; `puVar7 =
0x108B64[[camera+0xC]]` (or the fixed `0x1083CC`/`0x1084BC` blocks for
selector 3); pose writes per mode from `local_20`, selected at
`0x5065A..0x50678`: the block's `+0x48` array when both predicates
(`FUN_0004B7D0`/`FUN_0004B6FC`) are zero, else the `+0x4C` array (the
live-image default, matching the S4 landed port; both arrays are non-NULL):

| mode (`[0x14E57C]`) | pose source |
|---|---|
| 1, 0x12 | block `[0x4C/0x48]` record 0 (`+0x10=x,+0x14=y,+0x18=z,+0x58=yaw,+0x5C=pitch,+0x4C=angle`); mirrors by side |
| 3 | `record[FUN_000504E0(camera_x, class)]` (3 or 4) |
| 4 | `record[FUN_00050518(camera_x, class)]` (1 or 2) |
| 6, 0x10 | `record[7]` (dwords 0x2A..0x2F) |
| 7 | `FUN_000365B0`/`FUN_0003679C`/`FUN_000368EC` replay-camera writebacks |
| 8 | records 5 (dwords 0x1E..0x23) / 6 (0x24..0x29) selected by `[0x109A70+8] < 1` -> 6, `FUN_0003665C`/`FUN_000366EC` |
| 9 | `FUN_00036544` + same 5/6 records, then handler |
| 0xB | `FUN_0004F8C8` replay camera |
| 0x13/0x1C | `FUN_0004F5D0`; 0x14 `FUN_0004FF8C`; 0x15 fixed `0x108714..0x108728`; 0x1D `FUN_0004F304(block+0x300)`; 0x1F nop |
| default | `(*(&0x108B80)[handler])(camera, block)` when handler != 0 |

`FUN_000504E0(x, class)`/`FUN_00050518(x, class)` (fresh) return
`4|3`/`2|1` from `(class, sign_of_x)` — the left/right pose variant.
Selector oddity: for `[camera+4]==3` the remap yields behavior 5 whose `+0xC`
field is `-1` in the image, so `puVar7 = [0x108B60]` (the dword before the
table) while the pose comes from the fixed `0x1083CC`/`0x1084BC` blocks — a
degenerate read the engine should not reproduce (leg 8).

**Handler bodies (fresh decompiles, all `(camera, behavior_block)`):**

* **`FUN_0004E834` (handler 0)** — steady/orbit camera.
  Integrates `camera+0x34` via `FUN_0004E248(...)` clamped to
  `[-(0xB10+camera+0x3C), 0xB10-camera+0x3C]`; clamps the yaw accumulator
  `camera+0x18` into
  `[block[0x14]+0x18, +0x1C]` (the sub-record pointer at `block+0x50`);
  class `block+8 == 4` vs else chooses the yaw/pitch fold
  (`FUN_0004D698`); roll via `FUN_0004DF34(..., camera+0x4C)`, then writes
  `camera+0x58 += roll & 0xFFFF`; horizon jitter `DAT_0014E4D8 = rnd>>3 +
  0xE09`, `DAT_0014E4D4 = rnd>>3 + 0xAA1`; class-3 angle snap via
  `FUN_0004C7D0` when `camera+0x3C` crosses `±0x8A0`; pitch target from
  `[0x107DC8]+0x18 ± block[0xC]` clamped `±0x1620`; lerps `camera+0x18` and
  `camera+0x10` toward the block targets with `FUN_0004D698`, and
  `camera+0x4C` toward `block[1]` with `FUN_0004D668(8)`.
* **`FUN_0004E3A8` (handler 1)** — side-tracking camera. Same yaw accumulator
  clamp; a `±0x68` vertical bias by class (`block+8 == 2` -> `+0x68` else
  `-0x68`); yaw target ``camera+0x3C + block[0xD]`` clamped `±0xB10`; class-1
  vs else fold difference `±0x4B0` with `FUN_0004C7D0`; class-2 pitch via
  `DAT_0014E4D0 + block`, else `DAT_0014E4D0 + half`; clamps pitch to
  `±0xE40`, writes `camera+0x10/+0x18` via `FUN_0004D698` with the block's
  `[0..3]` targets; horizon globals `DAT_0014E4DC/D0 = rnd>>3 + 0xE3B/0xAD3`.
* **`FUN_0004EC9C` (handler 2)** — scaled/staged camera. Reads `block+3`
  (`<0x4000 || >0xC000` -> class 4 else 3); `FUN_000A1A60` scales a packed id
  from `block[0x14]` sub-record `+0x14` into `+0x18` (abs) and `block[4]`; the
  integrator writes `camera+0x10/+0x18`; yaw/snap fold at `0x4000/0x8000`
  boundaries; pan clamp `±0x720 / ±0xB10` against the `0x2000/0xA000` band
  test; horizon `0xE3B/0xAD3`; pitch target `[0x107DC8]+0x18 ± block[0xC]`
  clamped `±0x1620`.
* **`FUN_0004DB38` (handler 3, replay/action camera)** — first line
  `if (FUN_0006400C() == 0) param_4 += 100;` (faster when not live); tracks a
  target built from the staged triple (`camera+0x38` copy) via `FUN_0004C7D0`
  / `FUN_0004C77C` / `FUN_0004C7F8`; clamps `camera+0x10` to `±0x420` and
  `camera+0x18` to `±0x810`; yaw slew to the ball angle with divisor
  `0x1E0000`, pitch with `0x3C0000` and clamp `[0x2000, 63000]`; zeroes
  `camera+0x5A/+0x5E` (brake).

### 2.9 FU-71 event bodies

**`FUN_00071C94(player, vec, height)` (fresh decompile):** suspend gate
`[0x157A6C] != 0 -> return`; `FUN_000700F4()` reset (fresh: reads
`[0x14C2FA]/[0x14C2FE]` tables `0x1104AB`/`0x1104AC`, copies the target
triple from its stack args into `0x15774C/50/54`, zeroes
`0x1577EE+`, `0x1577F2..0x158006`, `0x1577BA/0x1577B6`, `0x1577C0/2`,
`0x157815`, `FUN_00070074(&0x15774C, flags)`, `FUN_0006D870`); writes the vec
to `0x14800` and a word to `0x1577BC`; clamps `height >= [0x157750]` and
`<= 0x640` into `[0x1577EE]` hi; for a player: `[0x158777]=0`,
`[0x1577CE + side*4] = player`, `[0x1577CA] = player`, `player+0x9B = 0`,
`[player+0x7A6]+0x7BF/+0x7CB = 0`; calls `FUN_00070544`; then sets the event
rate bytes `[0x157815]/[0x157820]/[0x157822]` from
`(*(player+0x20))+0x20/+0x21` through table `0x10E169` (or `FUN_0008DD70`
when the char is zero/side-flip).

**`FUN_00070544(arg)` (fresh decompile):** copies `0x15774C/50/54 ->
0x157764/68/6C`; when `[0x1577EE].hi < 1`: sets `[0x1577F2]=6`,
`[0x1577F4]=0xC`, `[0x1577F8].lo=0xC`, `[0x1577F8].hi=0`; else computes the
ramp via `FUN_000702F8` (`[0x1577F2]`, `[0x1577F4] = 2*F2`,
`[0x1577F8].hi = ±F2(...) + F2`) and re-rolls `[0x1577BE].hi`/`[0x1577C2]`
through `FUN_000A2AEE` (branch `[0x1577F8].lo == 0 || [0x157821] > 0`); then writes the
interpolated anchor path `0x157770/74/78`, `0x157788/8C/90`,
`0x157794/98/9C`, `0x1577FE/0x158002/0x158006`, rates
`0x157816/17/18`; if height > 0x70 and > 0x4F computes the
`FUN_000702F8(height-0x70)` tail and stores `0x1577FE.hi = height`, else copies
the previous triple; zeroes the event accumulators.

**`FUN_000709D0` (fresh decompile) — pan step:** for `[0x1577EE].hi != 0`
maps the height to class/parameter bands (`<0x29 -> 2h+0x14 class 1`,
`<0x65 -> (h+0x50)/2 class 2`, `<0x12D -> (h+100)/4 class 3`, else
`(100, class 3)`) and calls `FUN_00065CF8(class, param)`; advances the
`0x157821` timer (with `[0x1575C3] = FUN_000CB2A4()`); updates
`[0x1577EE].hi = FUN_000A2AEE()`; when both bearing/pitch words are nonzero
and `([0x14C1D4]|[0x14C1D6]) & 4`, random-walks them through `FUN_00092AC8` +
`FUN_000795A4` and recomputes `[0x1577BE].lo = FUN_0008DC68(bearing,pitch)`;
ends with `FUN_00070544(0)`.

**`FUN_00070DE0` (fresh decompile) — boundary/reposition:** classifies
`{0x157758,0x15775C,0x157760}` and `{0x15774C,0x157750,0x157754}` with
`FUN_00070074`; when the masks differ and bit 0x8 is clear it steps a path
with `FUN_0008DC50(direction)` from one point to the other, updating
`0x15774C/50/54`; then when the masks differ: `|[0x157750]| < 0xB22` (or
`[0x15781C]==0 && bearing > 9`) plays `FUN_000974DC` + `FUN_000651F0(6)` +
`FUN_000974F0(1000)`; mask bits `3` negate `[0x1577C0]` through
`FUN_0008DC50(-v,2)`; bit `0x10` raises/lowers the elevation
(`[0x157A62].sw` jitter or increments `[0x1577F8].hi` until `FUN_00070B94`);
bit `4` zeroes `[0x1577C2]`; ends `[0x1577BE].lo = FUN_0008DC68`. When bit 8
is set (boundary): target z `±0xB11`/`±0xB0F` by class, corner test
(`local_30 < 0x91 && |local_34| < 0xC2`) calls `FUN_0008ED40(record,3)` +
`FUN_0008F188(0x48/0x49, record, 0x40)`, target y clamp
`0xFFFFF52C`/`0xAD4`, `FUN_000974DC` + `FUN_000651F0(6)` +
`FUN_000974F0(1000)`; writes the target triple back.

**`FUN_00071DF4` (fresh decompile) — ball/event rates:** when the tracked
record `[0x1577CA]`'s sub-object type is `0x18D8` and `[0x1577EE].hi > 0xF0`:
`[0x1577C0] = subobj[0x20]*15` clamped `±15`, `[0x1577C2] =
subobj[0x21]*15` clamped, recompute `[0x1577BE].lo`; else when
`[0x157815] >= 0`, `[0x1577F8].hi <= 0x1E`, sets rates
`[0x157816]/[0x157817]` from tables `0x11042B/0x11042C` at index
`(char)(0x10E169[(subobj[0x1E]>>24)*4 + (subobj[0x1D]>>24)] +
[0x157815]*8) * 2`, then the keeper/player event
`FUN_00092998(1,2,-1)` result byte `2` with `[0x1577EE].hi >= 0xC1` and
`|[0x157770]| <= 0x23F` triggers `FUN_0008F188(0x1D or 0x1E, record, 4)`.

**`FUN_00070074(point, out)` classifier (fresh decompile):** `|z| < 0xB10 ->
8`; `< 0xB90 -> 0`; else `4`; `x < -0xD0 -> |1`; `x > 0xCF -> |2`;
z-window edge (within 0x31 of 0xB10) -> `|0x10` when `y < 0xA0 -
(distance)`. Returns `flags == 0`.

**Cell map to the ported integrator fields** (`fifa96_camera.c`):
`0x15774C/50/54` = `target_x/y/z`; `0x157788/8C/90` = `anchor_*`;
`0x157794/98/9C` = `anchor2_*`; `0x1577C0` = `vel_x`; `0x1577C2` = `vel_z`;
`0x1577C4.hi/0x1577C8` = `acc_x/acc_z`; `0x1577F8.hi` = `timer`;
`0x1577F4` = `timer_limit`; `0x1577F0` = `event_param`;
`0x1577EE.lo` = `event_cursor` (the update's `event_cursor >= 0x70/0x90` gate
at `0x73B0C` compares `[0x1577EE].lo` with `[0x14C2F6]`-selected 0x70/0x90);
`0x1577FE.hi` = `anchor_time`; `0x158006` = `anchor2_time`;
`0x1577BA.lo` = `step`; `0x157816/17` = `rate_x/rate_z`.

### 2.10 Palette residual

**`FUN_00046F80(base)` fresh disasm (54 insns, exact):**

```
0x46F84  23x: [0x14BF60 + i*4] = base + i*0x100          (i=0..22)
0x46F99  7x : [0x14BF34 + i*4] = base+0x1700 + i*0x100   (i=0..6)
0x46FAE  8x : [0x14BB00 + i*4] = base+0x1E00 + i*0x100   (i=0..7)
0x46FC1  EBX = base+0x2600
0x46FC8  [0x14BF50] = base+0x2700   [0x14BF20] = base+0x2800
0x46FDC  [0x14BF54] = base+0x2900   [0x14BF2C] = base+0x2A00
0x46FF0  [0x14BF28] = base+0x2B00   [0x14BF30] = base+0x2C00
0x47004  [0x14BF5C] = base+0x2D00   [0x14BF58] = base+0x2E00
0x47018  [0x14BF24] = base+0x2F00
0x4701F  256x: [0x14BB20 + k*4] = EBX (same base+0x2600)
0x47031  8x : [0x14BE0C + i*4] = [0x14BB00 + i*4]
0x47045  [0x14BFBC] = EBX (base+0x2600)
```

So the pool partition floor is **base+0x3000**: 23 slots at `+0x0000`, 7 at
`+0x1700`, 8 at `+0x1E00`, the shared `0x14BB20[0..255]` identity/team table at
**base+0x2600** (`0x14BFBC = base+0x2600`), and 9 fixed blocks at
`+0x2700..+0x2F00` (`0x14BF50/0x14BF20/0x14BF54/0x14BF2C/0x14BF28/0x14BF30/
0x14BF5C/0x14BF58/0x14BF24`). FU-148 §11.4/§11.5-7 agrees (the earlier
"+0x3600"/"+0x2C00" values here repeated the pre-S4 error).

**`FUN_00049138(1)` fresh disasm:** `MOV EBX,0x20; MOV EDX,0x34E8; MOV
EAX,0x101A20; CALL 0x4A448; MOV [0x107290],EAX; CALL 0x46F80` — the alloc
request is forwarded as `FUN_00098C38(size=0x34E8, type=0x220, name=0x101A20)`.
`0x4A448` fresh disasm: `CMP EBX,0x20` sets an align extra; `OR BH,0x2` ->
EBX=0x220; pushes EBX/EDI/ESI; two-branch `FUN_00098C38` (with `0x98BF8`
fallback on failure). `FUN_00098D1C` arg slots (fresh disasm): `[ESP+0x20]` =
size (0x34E8), `[ESP+0x24]` = type (0x220), `[ESP+0x1C]` = name; it rounds
the size up to the pool's alignment `[0x15B9D8 + class*0x14 + 8]` (fresh
`0x15BA00` = 20 zero bytes; runtime-filled) and tag-copies the 12-byte name
into the block header.

**Pool resource identity:** the name at `0x101A20` fresh bytes =
`70 61 6C 65 74 74 65 73 00` = **"palettes"**; `[0x107290]` has no reader
outside `FUN_00049138` (fresh 4 refs) — consumers go through the partition
pointers. The free path `FUN_00049138(0)` (callers `0x4AE55` alloc,
`0x4AF0A` teardown `FUN_0004AEA4`) releases `0x107290` via `FUN_0004AFA0`.

**Slot consumer `FUN_00048DC0(param)` fresh decompile + disasm** (extends
FU-148 §4.2): when `[0x1068E0]==1` and `param ∈ {0,0xB}` the kit path reads
`local_24 = 0x14BF60[param]`, memmoves it to the stack, translates band 1
through `0x107287` and band 2 through `0x10727C` — bands `[0x94,0x9B)`/
`[0x9B,0xA6)` + bases `0xA1/0xA3` (param 0) or `[0x82,0x89)`/`[0x89,0x94)` +
bases `0x9C/0x9E` (param 0xB) — then `PUSH ESP; CALL 0xCE980`. The **non-kit path** (any other
param, or `[0x1068E0]!=1`) is `MOV EBX,[EBP*4+0x14BF60]; PUSH EBX; CALL
0xCE980` (disasm `0x48EB9..0x48EC1`) — i.e. the pool slot itself is the
translation table. `FUN_000CE980` fresh decompile copies `0x40` dwords
(0x100 B) into `0x114720`; `FUN_000CE998` copies them out. Fresh `0x114720`
census = 31 sites; consumers include the span writer `FUN_000CEABC`
(`0xCEAEB..0xCEB42`), `FUN_000546F8` (save/restore around overlay text),
`FUN_000CB150` (push), and `FUN_000546F8`/`0xB33F4`/`0xC2B9C`/`0xC2D54`
readers.

**Shade cube (extends FU-98 §2, FU-148 §4.3).** `FUN_000A154C(param)` fresh
disasm: `SUB ESP,0x400` (the 1024-B BGRA table) -> `FUN_000A1368(0x100,
param, table)` -> `FUN_000A129C(table)`. `FUN_000A129C` fresh disasm:
alloc `0x8000` name `0x10365C` (`"inversetbl"`), alloc `0x20000` name
`0x103668` (`0x0D 69 6E 76 ...`, same tag), then
`FUN_000A0AA0(EAX=0x100, EDX=table, EBX=5, ECX=0x20000-cube, push
0x8000-scratch)` and registers the cube with `FUN_000993EC`. Fresh
`FUN_000A0AA0` decompile confirms `(count, table, bits, cube, offset)` and
`5 bits -> 32^3` cells. Fresh censuses: `FUN_000A0CB8` has exactly **1**
caller (`FUN_000A0AA0`); `FUN_000A0E3C`/`FUN_000A10E0` call only each other
(2+2, all inside the search); `FUN_000A154C` callers = `0x48C1D`/`0x48CD5`/
`0x48C52` (palette install). **No static consumer of the built cube exists**
outside the builder/search; the FU-148 §4.3 claim "its only reachable use is
the sprite blender (`FUN_000AFBFC` path)" is not reproducible statically —
fresh `FUN_000AFBFC` is the sprite-header bpp accessor (mode byte `0x79/0x7A/
0x7B/0x7D/0x7E/0x7F -> 1/4/8/0x20/0x10/0x18`), not a blender. If a runtime
consumer exists it must resolve the `"inversetbl"` allocation by name through
the resource cache (leg 11).

---

## 3. Derived semantics

### 3.1 Presenter row table (draw order, exact gates)

```
FUN_000565BC (per drawn buffer, caller FUN_000495B0 0x49681):
  R1  [0x14E538]==0 && !menu && !replay && mode∉{C,13,14}
        -> FUN_000560C8 (sub numbers "%d - %d", centred)
        -> FUN_0005619C(0), FUN_0005619C(1) (5-mark strip + team name)
  R2  replay-family && state != 0x81 && !([0x14E58C] in 0xF1..0x168)
        -> [0x14E58C]==0 ? FUN_00064C34 (replay HUD: caption/progress/buttons)
                         : FUN_000564A0 (blinking caption id 0x18C)
  R3  !paused && [0x14E674]&0x8000 -> FUN_000542D4 (timeout) + FUN_000550E4 (screen)
  R4  !paused && [0x14E510]==0 && !replay && period<4 && [0x14E59C]==0
        -> FUN_00055C24 (match HUD, FU-148)
  R5  !paused && [0x14E660]!=0 -> FUN_00056518 (ball sprite; dormant)
```

Replay state `[0x109A98]`: 0 live; 0x80 -> 0x81 armed; 0x82 playing;
0x83/0x84/0x85/0x86 paused/step modes; exit back to 0. The camera coupling is
`FUN_0004D134([0x109A8C])` (6 replay cameras) plus handler 3
(`FUN_0004DB38`) used directly by `FUN_0004D134` case 4.

### 3.2 Substitution overlay flow

```
state set ([0x14E678]/[0x14E674] id + [0x1587xx] record)
  live frame:
    R1: FUN_000560C8  -> two numbers from the [0x1587D4] block via FUN_0004BDF8
        FUN_0005619C(s) -> per-side 5 marks (FUN_0004BD38 states) + team name
    R3 (when the overlay screen is armed): FUN_000550E4 case 5/0xB/0xE
        -> team names + score + player names (FUN_000546F8 through 0x114720)
        -> tail FUN_000563D0 (animated object) + FUN_0009849C (font restore)
  timeout: FUN_000542D4 (~[0x14E680] ticks) -> FUN_00053E08 flips the timer
```

### 3.3 Camera pose feed

```
main loop FUN_000495B0 -> FUN_0004D2D4
  if [0x109A98]==0x82 -> poser OFF
  else if camera+4 < 4 && [0x107DD8]==0 && pad idle -> FUN_000505D0
     mode = [0x14E57C]
     block = 0x108B64[[camera+0xC]]
     pose = block[+0x4C|+0x48][record(sel)] / fixed blocks (mode 3) / 0x10866C
     write +0x10,+0x14,+0x18,+0x58,+0x5C,+0x4C
     default mode -> handler[clamp(camera+4)](camera, block) in 0x4E834/0x4E3A8/0x4EC9C/0x4DB38
per-frame handlers write the positional integrators and the
horizon/plane globals 0x14E4D4/0x14E4D8/0x14E4D0/0x14E4DC
```

### 3.4 FU-71 event graph

```
gameplay event (11 callers of FUN_00071C94)
  -> FUN_00071C94(player, vec, height): reset + target/tracked-player set
  -> FUN_00070544: ramp setup (0x1577F2/F4/F8, anchor path, rates)
  -> FUN_000709D0: pan step (band curve + random walk) -> FUN_00070544(0)
  -> FUN_00070DE0: out-of-bounds reposition (classifier bits, sounds, corner events)
  -> FUN_00071DF4: ball sub-object / keeper rates -> 0x1D/0x1E events
  -> FUN_00070074: boundary classifier (bits 1/2/4/8/0x10)
integrator FUN_000736AC consumes: target/anchor/anchor2, vel 0x1577C0/2,
timer 0x1577F8, timer_limit 0x1577F4, event_param 0x1577F0, rates 0x157816/17
```

### 3.5 Palette install graph (extended)

```
load FUN_0004AD4C -> FUN_00049138(1)
   EBX=0x20|0x200, size 0x34E8, name 0x101A20 "palettes"
   -> FUN_0004A448 -> FUN_00098C38/0x98BF8 -> FUN_00046F80 partition (>=0x3000)
per draw FUN_00048DC0(entity)
   -> kit path ([0x1068E0]==1, entity 0/0xB): translate bands -> stack
   -> else: table = 0x14BF60[entity]
   -> FUN_000CE980 -> 0x114720 (0x100 B)
palette install FUN_00048C8C/FUN_00048B60 -> FUN_000A154C
   -> 1024-B BGRA table -> FUN_000A129C -> "inversetbl" 0x20000 cube (5 bits)
   -> FUN_000A0AA0 build (no static consumer)
```

---

## 4. Port contract

### 4.1 `fifa96_match_run_render` (`src/fifa96_engine/fifa96_match_run.c`)

Presenter rows become named steps in the per-frame render (order preserved):

* `render.sub_strip` (R1): when `run.substitution.active` (new state seeded
  from the record block behind `[0x1587D4]`): draw the two substitution
  numbers with `fifa96_font` centred text (`FUN_00054640` math:
  `x = 0xA0|0x140 - half_scaled_width`) and the per-side 5-mark strip
  (`FUN_0004BD38` states 1..3) + team name at the `0x14E53C+side*0x1C` layout
  (which `fifa96_window_scale` must now emit: `y0`, `h` from the Frames
  frame-5 height, `x0+4*scale` left / `x1-4*scale-width` right, `y0+12*scale`).
* `render.replay` (R2): map `[0x109A98]` to
  `enum { LIVE, ARMED, PLAY, PAUSE, STEP_BACK, STEP_FWD, SLOW }`; draw the
  replay HUD (caption by camera index `[0x109A8C]`, progress
  `[0x109A94]/[0x109AC0]`, button highlight by state) or the blinking caption
  in the ramp window (`[0x14E58C]` 1..0xF0 / 0x169+, 10/20-frame blink).
* `render.overlay` (R3): a small screen-id switch (0 select; 1/4 player list;
  5/0xB/0xE substitution; 6 info; 7/0xC period; 8 list; 9/0xA/0xD/0xF
  messages; 0x11 extra-time/aggregate) driven by `overlay.id` +
  `overlay.timeout`; reuse the staged team-name/score strings; tail
  `FUN_000563D0` (animated object) stays a leg.
* `render.ball_row` (R5): omit (dormant — no producer); note in the header.
* HUD (R4) unchanged (FU-148).

### 4.2 `fifa96_camera.c` (`src/fifa96_loader/fifa96_camera.c`)

* `fifa96_camera_pose_feed(cam, view_mode, side)` implementing
  `FUN_000505D0`: pose blocks (`0x108B64` layout: `+0x08` class, `+0x48`/
  `+0x4C` pose arrays — `+0x48` when `FUN_0004B7D0`/`FUN_0004B6FC` are both
  zero, else `+0x4C` (the live-image default); `+0x50` sub-record) + record
  selection (`FUN_000504E0`/0x50518 `{3,4}`/`{1,2}` by class and x sign;
  record 7 for modes 6/0x10; records 5/6 for mode 8) writing
  `yaw/pitch/view_ratio` + position.
* Add `fifa96_camera_type(cam, type)` selecting `{handler, behavior}` from the
  `+4`/`+0xC` fields with the `3 -> 5` remap and the `-1 -> 0` clamp (replay
  records carry `-1`).
* The four handler bodies become `fifa96_camera_behavior_*` variants
  (steady `0x4E834`, sidetrack `0x4E3A8`, staged `0x4EC9C`, action/replay
  `0x4DB38`); their constants (pan limits `0xB10`, `±0x68`, `±0xE40`,
  `±0x420/±0x810`, `0x18000 - yaw` fold, horizon `0xE09/0xAA1/0xE3B/0xAD3`,
  pitch clamps `0x1324/8000`, `±0x1620`) are quoted in §2.8.
* Event producers map onto the existing `fifa96_camera` fields per §2.9:
  expose `fifa96_camera_event_set(player-like, vec{x,y,z}, height)`
  (`FUN_00071C94`+`FUN_00070544`), `fifa96_camera_pan_step`
  (`FUN_000709D0`), `fifa96_camera_reposition` (`FUN_00070DE0`),
  `fifa96_camera_rate_event` (`FUN_00071DF4`).
* `fifa96_camera_reflect` (already ported) is the `FUN_000736AC` boundary arm;
  `FUN_00070DE0` is the cut/reposition writer the FU-96 leg-3 note asked for.

### 4.3 `fifa96_font.c` (`src/fifa96_loader/fifa96_font.c`)

* The residual overlays reuse the HUD text API: add
  `fifa96_font_draw_centered(text, y, cell/centre)` implementing
  `FUN_00054640` (centre `0xA0`/`0x140` minus half-scaled width) and keep the
  two-pass colour 6/0 logic of `FUN_000544B4`.
* Overlay strings to stage as data (ids from the engine's own tables):
  period ids `0x51090`/`0x510A8` (all zero at rest — supply engine data), the
  extra-time ids `0x214/0x101E25`, `0x101E18`, sub ids `0x19D/0x19F`
  (`0x101DF8`), `0x15E`, `0x166`, `0x19B`, `0x101DA8`, `0x23A`.

### 4.4 Palette (`fifa96_match_run_stage` / loader seams)

* Pool: allocate the native layout — **38 x 0x100 sprite/kit slots
  (23+7+8 at +0x0000/+0x1700/+0x1E00), one shared 0x100 table at +0x2600,
  9 fixed 0x100 blocks at +0x2700..+0x2F00**, total floor `0x3000` (the
  `0x34E8` size request covers it), tag "palettes" (leg 12).
* Translation: the non-kit path installs the pool slot directly
  (`0x14BF60[entity]`); the engine's identity stand-in remains valid until the
  pool slots are staged. The kit path bands (`0x94/0x9B -> 0xA1/0xA3`,
  `0x82/0x89 -> 0x9C/0x9E`) are the table to port with the pool.
* Shade cube: keep out of the match contract until a consumer is found
  (leg 11); the build is `FUN_000A154C`/`FUN_000A129C` with a 5-bit
  32^3 `"inversetbl"` cube.

---

## 5. Numbered legs

1. **Replay HUD strings** — `0x102110/0x102118/0x102120` are `"%s 1"`/
   `"%s 2"`/`"%s"` in the image and the caption ids `[0x109AD0..0x109AD8]`
   are runtime-filled; the visible captions/format are not derivable
   statically.
2. **`FUN_000564A0` blinking caption id `0x18C`** — the string survives in
   the language resource, not the EXE; blink window 0..9/10..19 derived, the
   text is not.
3. **Replay HUD button semantics** — `[0x155C78/7C/88/8C]` descriptor
   handles (FU-116 §6 leg 3 carried); which button each state highlights is
   mapped (0x82->8C, 0x83->88, 0x84->7C, 0x85->78) but the glyphs are
   runtime.
4. **Substitution record fields** — `FUN_0004BDF8` selects bytes of the
   `[0x1587D4]` block by `[0x1587E6]>>8 < 5`; the field meanings (which
   numbers are shown) are not derived (record identity is FU-145 L4).
5. **`FUN_0004BD38` strip states** — fresh censuses show the `[0x1587E7]`
   cursor and the `[0x1587DA + i*2 + side]` flags are **only zeroed** in the
   image (`FUN_0008A938 0x8AE5F/0x8AE85` — which also sets
   `[0x1587D4] = [0x1587CC + i*4]` and `[0x1587E6]` — and `FUN_000886D4`
   `0x88824/0x88846`); `k` is therefore 0 and only slot 0 can mark. Whether a
   computed-pointer setter exists (as with R5) and `FUN_0009B0D0`'s blit
   inputs (registers) are the open part.
6. **`FUN_00056518` dormancy** — proven for the static writers
   (`FUN_00056690` always called with 0); if a runtime/computed writer exists
   the row resurrects (ball.fsh entry 37, 8 frames).
7. **`FUN_000550E4` case arms** — the player/team selection helpers
   `FUN_0004BB2C`/`FUN_0004BB5C`/`FUN_0004BEAC`/`FUN_0004B840`/`FUN_0004B860`
   and the `0x151090`/`0x1510A8` id tables are cited, not decomposed.
8. **Camera pose-array selection** — `[block+0x48]` vs `[block+0x4C]` is
   chosen by `FUN_0004B7D0`/`FUN_0004B6FC` (team/AI predicates): `+0x48` when
   both are zero, else `+0x4C` (the live-image value); both arrays are
   non-NULL. The predicates' exact semantics remain a leg.
9. **Camera handler math** — `FUN_0004E248`, `FUN_0004D698`, `FUN_0004DF34`,
   `FUN_0004C7D0/77C/7F8`, `FUN_0004D668`, `FUN_0004A2AEE` are cited; the
   handler bodies are quoted at the call/constant level only.
10. **Replay camera set** — `FUN_0004D134` cases 1..6 records
    (`0x107968+[0x107DD0]*0x70`, `0x107CE8`, `0x107B98`, `0x107B28`) and
    `FUN_0004D98C`/`FUN_0004DDA8` sub-modes 5..8: the record list is quoted,
    the per-mode pose semantics of `FUN_0004D98C` are not derived.
11. **Shade cube consumer** — no static consumer; the `"inversetbl"`
    allocation (0x20000) is reachable only through the resource cache by
    name (or a computed pointer). The `FUN_000A0CB8` search internals stay
    FU-98 leg 1.
12. **Palette pool exact size/alignment** — the partition floor `0x3000`
    fits inside the `0x34E8` request; the pool-class alignment
    `[0x15B9D8 + class*0x14 + 8]` is BSS/runtime (`0x15BA00` fresh = zeros),
    so the exact rounded size and pool base semantics (`*block` return) are
    not pinned.
13. **`FUN_0004BD38`'s `FUN_0009B0D0`/`FUN_0009B3A0` blit** per-slot
    glyph/colour state.
14. **`FUN_00011BEC` third argument** — the strip passes
    `[0x14E540+side*0x1C]` (the row height) where the HUD passes 0; whether it
    is a width/abbreviation control is not derived.

## 6. Risks

* **Runtime-filled strings** (`0x109AD0`, `0x18C`, `0x151090/0x1510A8`,
  button descriptors): the replay/menu overlays are portable in structure,
  but their captions must come from staged language data; pixel re-pin will
  move the M2 tape.
* **Dormancy claims**: R5 is proven dormant from static writers only; any
  dynamic write (computed pointer into `0x14E6xx`) would invalidate it.
* **Camera**: the pose feed + four handlers are now portable at the semantics
  level; the per-frame *motion* during live play still depends on the 11
  `FUN_00071C94` gameplay callers (FU-148 leg 9), which remain unported.
* **Palette**: pool sizes/alignment are the last gate to a faithful
  translation-table build; the shade cube is un-consumed statically.
* **Tape churn**: replay HUD/substitution screens + camera feed move the M2
  golden; re-pin with a written reason + frame diff.

## 7. Provenance (Ghidra MCP, read-only, `/FIFA96.EXE`)

`decompile_function` 0x565BC, 0x56518, 0x563D0, 0x63FD0, 0x63FDC, 0x63FF0,
0x6400C/0x6401C (via search), 0x53D58, 0x53D50, 0x564A0, 0x64C34, 0x560C8,
0x5619C, 0x542D4, 0x53E08, 0x550E4, 0x56690, 0x56CF4 (disasm tail), 0x4BDF8,
0x4BD38, 0x4BE30, 0x54640, 0x544B4, 0x53240, 0x492CC, 0x4937C, 0xCB2A4,
0x505D0, 0x504E0, 0x50518, 0x4E834, 0x4E3A8, 0x4EC9C, 0x4DB38, 0x4D134,
0x4CEF4, 0x4DDA8, 0x71C94, 0x700F4, 0x70544, 0x709D0, 0x70DE0, 0x71DF4,
0x70074, 0x736AC, 0x46F80, 0x49138, 0x4A448, 0x4AD4C, 0x4A448 (disasm),
0x98C38, 0x98BF8, 0x98D1C, 0x5B5DC, 0x59B54, 0x48DC0, 0xCE980, 0xCE998,
0x546F8, 0x471EC, 0xA0AA0, 0xA129C, 0xA154C, 0xA0E3C, 0xA10E0, 0xAFBFC,
0x993EC;
`disassemble_function` 0x56518, 0x46F80, 0x49138, 0x4A448, 0x98C38,
0x98D1C, 0xA129C, 0xA154C;
`disassemble_bytes` 0x50680 (80 B), 0x505E0 (160 B), 0x56DE0 (96 B),
0x56E20 (40 B), 0x4A228 (32 B), 0x48EB0 (64 B), 0x8AE40 (80 B),
0x88810 (64 B);
`read_memory` 0x107370 (180 B), 0x107414 (16 B), 0x107438 (72 B),
0x101B58 (8 B), 0x101B34 (16 B), 0x102100 (48 B), 0x101A20 (32 B),
0x101E30 (64 B), 0x108B64 (32 B), 0x108B80 (16 B), 0x10896C (88 B + 504 B),
0x1089B0 (16 B), 0x107E2C (192 B), 0x107F1C (96 B), 0x10866C (96 B),
0x107508 (672 B), 0x107578/0x1075E8/0x107658 (16 B each), 0x107DC8 (8 B),
0x107500 (48 B), 0x107B28/0x107B98/0x107CE8 (16 B each), 0x107968 (16 B),
0x107DD0 (8 B), 0x151090 (48 B), 0x15BA00 (20 B), 0x103650 (32 B),
0x14E4D8-adjacent values via decompiles;
fresh `get_function_xrefs` 0x565BC (3), 0x560C8 (1), 0x5619C (2), 0x542D4
(1), 0x550E4 (1), 0x56518 (1), 0x564A0 (1), 0x64C34 (1), 0x56690 (1),
0x505D0 (3), 0x71C94 (11), 0x46F80 (1), 0x49138 (2), 0x48DC0 (2),
0xCE980 (11), 0xA0AA0 (1), 0xA0CB8 (1), 0xA0E3C (2), 0xA10E0 (2),
0xA154C (3), 0x56CF4 (1);
fresh `get_xrefs_to` 0x14E660 (3), 0x109A98 (29), 0x107290 (4),
0x1577C0 (91), 0x1577C2 (54), 0x14E540 (2), 0x14E53C (2), 0x10365C (1),
0x103668 (1);
`search_instructions` operands `109a98` (29), `109ac4` (6), `14720` (31),
`5bb3` (31), `5bb5` (15), `1587` (200+, truncated), `56690` (1);
retail: `build/fifa96_play sprite tests/golden/gameart0.pvi --name ball.fsh
--print-summary` -> entry 37, 8 frames, 16x16 (build/play artifact removed).
No Ghidra writes, no project saves, no repo edits; this draft is the only
file written.

---

## 8. P4 landing (phase-7, 2026-10-09)

All items first-hand spot-checked on `/FIFA96.EXE` before porting; errata
below correct this slice where the port found the prose inexact.

* **R1 substitution strip.** `fifa96_window_strip_layout` implements the
  FUN_00053240 tail (y0; row height = Frames frame-5 height doubled under the
  settings-4 "wide" flag; `x0 + ((4*scale_x+0x8000)>>16)` and
  `x1 − ((4*scale_x+0x8000)>>16) − ((name_width*scale_x+0x8000)>>16)`;
  `y0 + ((12*scale_y+0x8000)>>16)`). **Erratum:** "wide" is settings slot 4
  (fresh `FUN_00044BE0` = `FUN_0001D940(4)`), used by FUN_00053240 and the
  FUN_0005619C name halving. `fifa96_font_blit_outlined` (colour 6 then 0) and
  `fifa96_font_draw_centered` implement FUN_000544B4/FUN_00054640.
  **Erratum:** FUN_00054640's wide path quarters the measure (`0x140 −
  ((w>>2)*scale + 0x8000)>>16`), not "half-scaled"; the narrow path halves.
  `fifa96_match_run_sub_mark` = FUN_0004BD38 (all branches incl. the mode-5
  special returning 2). **Erratum:** FUN_0004BDF8's else arm reads the
  `+0x10/+0x11` bytes of the 0x1587D4 block (the decompiler's
  `[0x1587E1]._3_1_`/`0x1587E5` pair; condition is the `[0x1587E7] < 5`
  byte). The R1 draw centres the `"%d - %d"` pair at `y_text + 2*scale`,
  blits the staged mark glyph for FUN_0004BD38 states 1..3, and draws the
  names in the row column with the native clamp.
* **R2 replay row.** Predicates as helpers; `fifa96_match_run_replay_row`
  implements the FUN_000565BC gates; `fifa96_match_run_replay_step` the
  reachable FUN_000642FC subset (the 0x80→0x81 promotion and the 0x81 arm run
  in the same call, button 1 toggle + cursor wrap, 0x20 camera cycle mod 6,
  8/4/2 step modes with the single-frame reset, the bit-0x80 exit),
  `fifa96_match_run_replay_blink_step` = FUN_000564A0 (draw when the counter
  is <= 9 or reset from >= 0x14), `fifa96_match_run_replay_progress` =
  FUN_000642B0 (no clamp), and `fifa96_camera_replay_select` = FUN_0004D134
  (0 → 0x107968+[0x107DD0]*0x70, 1/2/3/6 → 0x107CE8 sub 5..8, 4 → 0x107B98,
  5 → 0x107B28). **Erratum:** FUN_00053D58's `[0x14E58C]−0xF1 < 0x78` is a
  signed dword compare, so phases 1..0xF0 also take the ramp branch — the
  engine matches the signed compare.
* **R3 overlay.** `fifa96_match_run_overlay_row` = the FUN_000550E4 case
  table; `fifa96_match_run_overlay_arm` = the `id|0x8000` arm + timer reset +
  the `{5,9,0xB,0xC,0xD,0xE,0xF,0x12}` second-overlay seed;
  `fifa96_match_run_overlay_timeout_step` = FUN_000542D4 + FUN_00053E08
  direction flips (first call `dir = -rate`, then the `dir*rate > 0` negate);
  the visible gate covers `[0x14E688]`, the replay family and the extra-time
  id exclusion. Draw renders staged lines for draw-path ids; the per-case
  helpers stay leg 7.
* **R5 dormancy pinned.** Disasm re-verified `0x56E1B..0x56E41` (XOR EDX,EDX
  then the MOVSD loop, no EDX writes, `CALL 0x56690`); `render.ball_row` stays
  NULL and `fifa96_match_run_ball_row_reachable` reports the staged producer.
* **Camera handlers.** `fifa96_camera_type` = the 0x107508 record `{+4,+0xC}`
  cells (handler clamp >= 0; the +4==3 → behavior 5 remap; the image's −1
  behavior cells are the degenerate `[0x108B60]` read, engine-clamped to 0 —
  leg 8). The six-block mode table is pinned (`fifa96_camera_behavior_blocks`,
  fresh 0x10896C read: classes {3,1,3,3,1,3}, +0x30 {0xEA6,0x578,0x1130,…},
  every byte +3 and every +0x34 zero — so FUN_0004EC9C always takes the
  class-4 arm and handler 1's yaw target is the bare track z). The four bodies
  land at the quoted constant/clamp level
  (`fifa96_camera_behavior_steady/sidetrack/staged/action`: yaw bounds,
  ±0x1620/±0xE40 pitch clamps, ±0x68 class bias, ±0x720/±0xB10 pan, the
  class/band gates, horizon `rnd>>3 + 0xE09/0xAA1/0xE3B/0xAD3`, the action
  `!live -> +100` speed bump, ±0x420/±0x810 pos clamps, pitch [0x2000,63000],
  brake zeroing) and are wired into the FUN_000505D0 default arm when the
  caller stages the record subset + behavior block; a NULL state keeps the
  unported result. The FUN_0004E248/D698/DF34/C7D0/D668 integrators stay
  leg 9.
* **FU-71 residual.** `fifa96_camera_classify` (FUN_00070074 full bits),
  `fifa96_camera_pan_band` (FUN_000709D0's ladder) and
  `fifa96_camera_rate_event` (FUN_00071DF4 first arm, `h > 0xF0`). The
  FUN_00070DE0 reposition body, the FUN_00071DF4 table arm and the atan walk
  stay legs; first-hand FUN_000CD474 is an octant-dispatch atan2 (sign/swap
  bits select one of 16 indirect jump targets over the ratio table
  `0x14072C + index`), not a flat table.
* **Palette pool identity.** `fifa96_palette_pool_identity` pins
  size 0x34E8 / type 0x220 / tag "palettes" (fresh 0x49138 disasm + 0x101A20
  bytes) and `fifa96_palette_pool_create/release` allocate/partition the
  native request. The content producer stays leg 11/OL-T11-80; the shade cube
  stays out of the contract (no static consumer).

**Goldens:** M1 `09b726b7…` and M2 `2e709151…` byte-identical, no re-pin (all
new rows are zero-gated at rest). `make check` 108/108.

### Leg status after P4

1. Replay HUD strings — open (staged). 2. Blink id 0x18C — open (window
derived). 3. Button glyphs — open. 4. Sub record field meanings — open.
5. Strip states — landed; the FUN_0009B0D0 glyph/colour state stays open
(mark glyph is staged). 6. R5 dormancy — pinned. 7. Overlay case helpers —
open (row table landed, layouts staged). 8. Pose-array selection — default
arm + type mapping landed; the +0x48 alternate and selector-3 preamble arrays
open. 9. Handler math — open. 10. Replay camera set — index mapping landed;
FUN_0004D98C/DDA8 per-mode pose open. 11. Shade cube consumer — open.
12. Pool exact size/alignment — request pinned; the runtime-rounded size/pool
base semantics open. 13. Mark blit inputs — staged. 14. FUN_00011BEC third
arg — open.
