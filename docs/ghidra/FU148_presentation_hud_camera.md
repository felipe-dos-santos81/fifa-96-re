# FU-148 — presentation (HUD/camera/formation/palette)

Provenance: recon draft w4, phase-6 wave-1, frozen 2026-10-09; evidence review: PASS with corrections listed inline.

Wave 1 recon for `docs/superpowers/plans/2026-10-08-fifa96-m2-full-gameplay.md` (W4).
Read-only Ghidra session, authoritative program `/FIFA96.EXE` (Ghidra project path
`/FIFA96.EXE`, executable `/home/felipe/code/fifa96-reversed/game/hdd/FIFA96/FIFA96.EXE`,
watcom:LE:32:default, image base 0, 2706 functions). The stale flat bin
`/fifa96_le.bin` was **not** used for any quote. All addresses below are Ghidra
addresses on `/FIFA96.EXE`.

Address convention: FUs written against `/fifa96_le.bin` use flat addresses; on
`/FIFA96.EXE` data/globals are `flat + 0x100000` (FU-144 §1 note). This draft
uses the `/FIFA96.EXE` form exclusively; where a carried doc quotes the flat
form both are given.

Priority was the HUD (P0.2) — derived first-hand and completely enough to port;
camera/formation/palette follow.

---

## Scope

1. **HUD OL-T11-7** — the native match score/clock overlay draw, its state,
   resources, draw order/positions and the engine port contract.
2. **Camera legs** — FU-96 §6 legs 1/3 (camera mode/angle feed; selector +
   presets + handlers) and the FU-71 follow writer (who drives the camera
   velocity/pan during live play); port contract for the engine camera block.
3. **Formation-id producer** — the writers of `[0x14C1E4]`/`[0x14C1E5]` and the
   consumer that turns the id into a layout.
4. **Palette legs** — the base palette `0x14B200` writers (FU-144 §6 leg 1),
   the per-entity translation tables `0x14BF60 -> 0x114720`
   (`FUN_00048DC0`/`FUN_000CE980`), shade-cube placement; port contract.

An earlier lead in the plan said "FU-84 leads" for the HUD; FU-84 is the
animation selector/row-table document and contains no HUD draw. The HUD lead is
closed by direct discovery below (`FUN_000565BC` -> `FUN_00055C24`); FU-111
§5/FU-116 are the *replay* HUD button cluster, a different overlay.

---

## 1. HUD (OL-T11-7)

### 1.1 The draw path (first-hand, `/FIFA96.EXE`)

| function | window | role | callers (fresh `get_xrefs_to`) |
|---|---|---|---|
| `FUN_00055C24` | `0x55C24..0x560C6` (343 insns, `disassemble_function`) | **the match HUD draw** (bar + team names + score + clock + period) | exactly 1: `FUN_000565BC` call at `0x56677` |
| `FUN_000565BC` | `0x565BC..0x5668C` (`get_function_by_address` body_end) | the per-frame overlay presenter; owns the HUD visibility gates | 3: `FUN_000495B0` `0x49681`; `FUN_00058BC0` `0x58C9F`/`0x58D32` |
| `FUN_000495B0` | `0x495B0..0x498xx` | the main draw loop (two buffers, `uVar7`); calls `FUN_000565BC()` once per drawn buffer after `FUN_00049830` | — (indirect entry, FU-126 family) |

Call order inside `FUN_000565BC` (fresh disasm/decompile):

```
0x565C1  FUN_0004B380();                          ; mode = [0x157A4A]>>24
0x565C6  if (DAT_0014e538 == 0)                   ; [0x4E538] guard
0x565D3      if (FUN_00037ae4() == 0)             ; pad probe (FUN_00036BC8(5)/(7))
0x565DC         if (FUN_0006400C() != 0 && EDX not in {0xC,0x13,0x14}) {
0x565E8             FUN_000560C8();               ; overlay scaler A (FU-93 §2)
0x565F2/0x565FB    FUN_0005619C(0); FUN_0005619C(1);  ; overlay scalers
                }
0x5660C  if (FUN_00063FD0() != 0) ...             ; replay HUD branch (FU-111 §5)
0x56636  if (DAT_0014e688 == 0) {                 ; not paused
0x56641      if ([0x14E674] & 0x8000) { FUN_000542D4(); FUN_000550E4(); }  ; menu overlays
0x56652      if (DAT_0014e510 == 0) {
0x5665C          if (FUN_0006400C() != 0) {
0x56664              if (FUN_0004B5F4() < 4 && DAT_0014e59c == 0)
0x56677                  FUN_00055C24();       ; <<< THE HUD
                 }
             }
0x5668F      if (DAT_0014e660 != 0) FUN_00056518();
         }
```

Gates, with first-hand definitions:

| gate cell | meaning | evidence |
|---|---|---|
| `FUN_0006400C()` | `return [0x109A98] == 0` — replay state idle (the FU-108 `[0x9A98]` byte, EXE `0x109A98`) | decompile `0x6400C` |
| `FUN_0004B5F4()` | `return [0x157AC2]` — the **period byte** (FU-62/FU-142 L; EXE form of flat `0x57AC2`) | decompile `0x4B5F4` |
| `[0x14E510]` | HUD suppress; written only by `FUN_000537F8` (`0x538F2`); 2 refs total: that write + the read at `0x56652` | fresh `get_xrefs_to 0x14E510` = 2 |
| `[0x14E59C]` | second suppress; writes `0x5385C` (`FUN_000537F8`) and `0x519A9` (0x51xxx view handler); reads at `0x5666E`/`0x56698` (presenter) + `0x51996` | fresh `get_xrefs_to 0x14E59C` = 5 |
| `[0x14E688]` | pause/suspend flag; 9 refs (7 writes + 2 reads incl. `0x56636`) | fresh `get_xrefs_to 0x14E688` = 9 |
| `[0x14E538]` | first guard; 11 refs — 10 instruction (9 writes: `0x5391C` (`FUN_000537F8`), `0x543BC/F2`, `0x529E4/0x52A2C/0x52ADE`, `0x52397`, `0x512FD/0x51356`; + read `0x565C3`) + 1 DATA ref at `0x561B0` | fresh `get_xrefs_to 0x14E538` = 11 |
| `FUN_0004C2A0` | first line of the HUD body: `FUN_0001D940(6)` = table entry `[0x149290]`; 0 aborts the draw | `0x55C30 CALL 0x4C2A0`; decompile `0x4C2A0` = `FUN_0001D940(6)`; `FUN_0001D940` = `(&0x149278)[i]`; `read_memory 0x149278` = 40 zero bytes (runtime-filled BSS) |

So the HUD is drawn exactly when: the presenter runs (`FUN_000565BC`, called by
the main draw loop), not in replay (`[0x109A98]==0`), period `< 4`, not
paused/suppressed (`[0x14E538]==0`, `[0x14E688]==0`, `[0x14E510]==0`,
`[0x14E59C]==0`), and settings entry 6 non-zero (`FUN_0001D940(6)`).

### 1.2 HUD state (source cells)

| cell (EXE) | read in HUD via | meaning | first-hand writer evidence |
|---|---|---|---|
| `0x157AC5` / `0x157AC7` | `FUN_0004B5DC(&a,&b)` (`a=[0x157AC5]`, `b=[0x157AC7]`) at `0x55DAF` | per-side goal words (16-bit) | `FUN_00093944` `0x9394B INC word [EAX*2+0x157AC5]`; zeroed by `FUN_00092E2C` `0x92E7B/E82` (FU-142 L.3) |
| `0x157AB4` | `FUN_0004B594()` at `0x55C5F`; result -> `ESI` | **total elapsed seconds** (displayed clock) | writers: `FUN_00073EE0` `0x73EF3`, `FUN_0008AF38` `0x8B066`, `FUN_00092E2C` `0x92E66`, `FUN_00073D10` `0x73D48`, goal handlers `0x93CCF/0x93F60/0x940D8/0x9438F/0x94559/0x94763` (fresh `get_xrefs_to 0x157AB4` = 16) |
| `0x157AB6` | `FUN_0004B588()` at `0x55C4D` (period-half seconds; result unused by the HUD) | per-period seconds (reset on period advance) | fresh `get_xrefs_to 0x157AB6` = 38 (same family + `FUN_00088940` `0x88D2E`) |
| `0x157AC1` | — | sub-minute accumulator (`+= delta`; on `>=0x3c` minus 0x3c and both clock words `+1`) | `FUN_0008AF38` `0x8B04F/0x8B066/0x8B073` |
| `0x157AC2` | `FUN_0004B5F4()` at `0x55C3D`; displayed period = value+1 | period byte (0/1/2/3 = halves/pauses) | `FUN_0008AF38` `0x8B583/0x8B590` (`++`), reset writers as FU-62 |
| `0x108DDC` / `0x108DE0` | `[0x108DDC] < 0x10000` at `0x55CA2/0x55E24` | 16.16 window zoom scale (FU-93 §1, EXE form) | `FUN_00053240` per FU-93; recomputed in-engine |
| layout block `0x14E514..` | see §1.3 | HUD layout (FU-93 §1) | `FUN_00053240` (`0x532E0..0x5339D`), `FUN_000537F8` (`0x538F7/FC`), `FUN_000550E4` (`0x55169/6E/0x55208..0x55233`) |

Clock split (`disasm 0x55C80..0x55C98`): `EDI = ESI / 0x3C` (minutes),
`ESI = ESI % 0x3C` (seconds) with `IDIV` (signed, `SAR EDX,0x1F` sign fill), then
`sprintf("%02d:%02d", minutes, seconds)` (format at `0x101E40`).

### 1.3 HUD layout math (from `FUN_00053240`, FU-93 §1, confirmed in the draw)

```
[0x14E514] = 0x36
[0x14E51C] = 9 (zoomed) / 0xC (full)        ; row pitch
[0x14E520] = x0+3 / x0+6                     ; left column x
[0x14E524] = x0+0x23 / x0+0x32               ; right column x
[0x14E528] = x0+1 / x0+2                     ; bar x
[0x14E52C] = y1 - (h*0xB800>>16) - 1 / y1 - h - 2   ; bar y (bottom anchor)
[0x14E534] = min(0, y1 - [0x14E52C] - 0x36)  ; zoom clamp (<=0), FUN_000550E4 0x5516E
[0x14E530] = 2*[0x14E534]
```

### 1.4 HUD draw order and positions (`FUN_00055C24`, first-hand disasm)

Registers/values established before the draws:
`EBP = period+1` (`0x55C52`, or `[0x14E5A0]+period` in modes {0xC,0x13,0x14},
`0x55C57..0x55C5D`); `x0 = [0x14E520]+[0x14E530]` -> `EDI` (`0x55E03..0x55E1B`);
`clock_x = [0x14E524]+[0x14E530]` (+6 zoomed-wide / +8 full-wide) -> `EBP`
(`0x55E15..0x55ED8`); `y2 = [0x14E52C]+[0x14E534]+local_24` (`0x55E09/0x55EFD`);
`local_24 = 2` (both modes), `local_20 = 2` (zoomed) / `3` (full), bar height
`ESI = 0x12` (zoomed) / `0x18` (full), `+6`/`+8` when wide.

```
1. bar:  FUN_0009BB20(0xB800, [0x14E658], x=[0x14E528]+[0x14E530], y=[0x14E52C]+[0x14E534])   ; zoomed
   or FUN_0009AFD0([0x14E658], x, y)                                                          ; full
   (0x55E80..0x55EEA; FUN_0009BB20 disasm 0x9BB20: rect size = word[dims+4]*0xB800>>16 x
   word[dims+6]*0xB800>>16, FUN_0009B850(x,y,w,h): first-hand 0x9BB35..0x9BB5A)
2. colour 6 (outline): FUN_00019E9C(6) then FUN_000A06FC(str, x, y); colour 0: FUN_00019E9C(0), FUN_000A06FC
   a) name0  at (x0,   y2)                  0x55F10..0x55F44  (shadow at +1,+1)
   b) name1  at (x0,   y2+[0x14E51C]-2)     0x55F47..0x55F98  (shadow at +1,+1)
   c) score0 at (clock_x - width(score0), y2)                  0x55F9B..0x55FF1
   d) score1 at (clock_x - width(score1), y2+[0x14E51C]-2)     0x55FF4..0x56056
   e) period at cell (x0 .. x0+([0x14E51C]-1), y=2*[0x14E51C]+y2_raw+local_20),
      centered by FUN_00055BA8(str,x0,y, [0x14E51C]-1, [0x14E51C]-1)   0x56057..0x5608A
   f) clock  at cell (x0+local_28 .., same y), width ESI, centered by FUN_00055BA8 0x5608A..0x560AA
3. FUN_0009849C(esp): restore the 16-dword font state   0x560AF
```

Text strings (`sprintf` = `FUN_00099E9F`, `FUN_000AFD4F` printf engine):

| string | format | source | site |
|---|---|---|---|
| team 0 name | `%s` (`0x101E3C`) then `FUN_00017748` charset filter | `FUN_00011BEC(0,0,0)` = `FUN_00010A54(&0x143018+side*0x49, ...)` | `0x55D27..0x55D4B` |
| team 1 name | `%s` | `FUN_00011BEC(1,0,0)` | `0x55D39..0x55D4B` |
| (both, when `[0x14C32A]!=0`) | `%s` | string table ids `0x1EA`/`0x1E9` via `FUN_0001771C` | `0x55CCD..0x55D25` |
| clock | `%02d:%02d` (`0x101E40`) | minutes/seconds of `[0x157AB4]` | `0x55D93..0x55DA4` |
| score0/1 | `%d` (`0x101DF4`) | `FUN_0004B5DC` out pair | `0x55DB4..0x55DDF` |
| period | `%d` (`0x101DF4`) | `EBP` = period+1 | `0x55D7D..0x55D90` |

`FUN_000A06FC(str, x, y)` disasm `0xA06FC..0xA071B`: `FUN_00098470(x,y)` (sets the
cursor `[0x1127E4]/[0x1127E8]`) then `FUN_0009FFF0(str)` (glyph blit).
`FUN_00055BA8` disasm `0x55BA8..0x55C21`: `x += (cell_w - width(str))/2`,
`y += max(0,(cell_h-12)/2)`, then the same colour-6 outline + colour-0 main pair.
`FUN_000986B4` -> `FUN_00098630`: text width = sum over the string of the
per-char advance (`DAT_001127F6` default or `PTR_DAT_001127FC[ch-first]`).

### 1.5 HUD resources

| resource | slot | evidence |
|---|---|---|
| `PALsys.fsh` | 0x32 | FU-144 §2 (`0x107370` index 50 -> `0x101BA4` "PALsys") |
| `clockfnt.fsh` | 0x35 | `0x107370` index 53 -> `0x101BC0`; bytes at `0x101BBF..`: `69 63 6c 6f 63 6b 66 6e 74 00` ("iclockfnt"), entry pointer is `0x101BC0` = "clockfnt"; `FUN_0004A6BC` appends `.fsh` for slots 39..55 (FU-144 §2) |
| `playfnt.fsh` | 0x36 | index 54 -> `0x101BCC` = "playfnt" (`0x101BCC`: `70 6c 61 79 66 6e 74 00`) |

`FUN_00055C24` selects **0x36 (playfnt)** when `[0x108DDC] < 0x10000`, else **0x35
(clockfnt)** (`0x55CB3 MOV EAX,0x36` / `0x55CBA MOV EAX,0x35`, then
`0x55CBF CALL 0x4AFB8`). Both names resolve in the retail container: first-hand
`fifa96_play sprite tests/golden/gameart0.pvi --name clockfnt.fsh` and
`--name playfnt.fsh` return "entry ... is not a decodable SHPI sprite bank"
(the by-name lookup succeeded; only the SHPI decode is refused, as expected for a
font). `tests/test_play.py::TestAutoIso::test_smoke_pass` also shows the retail
`/ART/GAMEART0.PVI` carries 60 entries (e.g. `Npost.fsh` entry 35).

Font resource parse = `FUN_000984B4(handle)` (disasm/decompile `0x984B4`), fields
read into the text renderer state:

```
[0..3]   magic dword (compared against 0x464E544D byteswapped variants: "MTNF"/"FNTI"/"P..." family)
[+4]     first char code  -> DAT_001127F4
[+5]     last char code   -> DAT_001127F5
[+6]     default glyph width (when [+0x10]>>16 == 0)   -> DAT_001127F6
[+7]     glyph height      -> DAT_001127F7
[+8],[+9],[+10]            -> DAT_001127F8/F9/FA
[+0x10]>>16              -> per-char width table (rel. offset, or 0)
[+0x14] low word (signed) -> per-char flags/heights table (rel.)
[+0x14]>>16              -> rel. offset
[+0x18] low word (signed) -> rel. offset
[+0x18]>>16              -> rel. offset
[+0x1C]                  -> glyph bitmap data base
[+0x20..]                -> per-char dword {low16 = bit offset within row, high16 = row advance}
depth (bits/pixel) = byte[+3] - '0'   (1/4/8; FUN_0009FFF0 0xA00C7..0xA00F5)
```

Glyph blit = `FUN_0009FFF0` (`0xA0?` ~large): per char, width from
`DAT_001127F6`/`PTR_DAT_001127FC`, height `DAT_001127F7`/`DAT_00112800`, data at
`DAT_00112810 + ((off&0xffff)*depth>>3) + (off>>16)*row_bytes`, pixels written
through `FUN_000CEF64(x,y,byte)` (`0xCEF64`: clip to `0x1131CC/0x1131D4`, planar
`[0x1131E4]` path or linear row-table path).
Colour selection = `FUN_00019E9C(idx)` (`0x19E9C`): loads `(&0x1050A4)[idx]` and
the byte `[0x1050E4+idx]`; `read_memory 0x1050E4..0x1050F3` =
`FF 00 00 00 00 00 00 00 FF FF FF FF 00 00 00 00` (idx 0 -> 0xFF, idx 6 -> 0x00,
idx 8..11 -> 0xFF). The HUD uses indices 6 (outline pass) and 0 (main pass).

### 1.6 HUD-derived semantics (summary)

The native match HUD is a bottom-anchored two-column block:

```
row 0:  TEAM0-name                          score0   (right-aligned at clock_x)
row 1:  TEAM1-name                          score1   (right-aligned at clock_x)
row 2:  period (centered in a [0x14E51C]-1 wide cell at x0)   clock MM:SS (centered in an ESI-wide cell)
```

drawn with the `playfnt.fsh` font in the zoomed window (`[0x108DDC] < 0x10000`)
or `clockfnt.fsh` in the full window, outline pass colour 6 then main pass colour
0, over a scaled background bar. State read: score words, total-seconds clock,
period byte. Fresh `get_xrefs_to 0x4B5DC` = 7 call sites: `0x55DAF`
(`FUN_00055C24`, this HUD), `0x54EBE`/`0x54F56`/`0x554C7` (0x54xxx overlay
draws), `0x51656`, `0x60BF7` (`FUN_00060AA8` result screen), `0x63692`
(`FUN_00063688` score predicate) — the match HUD is the `0x55DAF` draw.

---

## 2. Camera legs

### 2.1 FU-96 §6 legs 1/3 — verification of `:157-163` and the missing producers

FU-96 `:157-163` (leg 5) says the camera-mode/angle feed "remains carried from
legs 1/3" and the FU-71 follow writer is unported. Verified: the engine camera
stays at the kickoff triple (`FUN_000700F4` reset) and `fifa96_camera_update` is
the ported follow integrator. The missing producers are now located:

**(a) per-frame camera mode/angle feed (first-hand)**

| item | evidence |
|---|---|
| mode getter | `FUN_00053D50` = `return [0x14E57C]` (`0x53D50` decompile) |
| mode writers | `FUN_000537F8` `0x53826`/`0x5387D`; 0x51xxx view handlers `0x5144C/0x514A3/0x51819/0x518F3/0x519A3/0x51AD0/0x51B2D/0x51B56/0x51B99` (fresh `get_xrefs_to 0x14E57C` = 12) |
| selector/poser | `FUN_000505D0` (`0x505D0..0x50Fxx`), fresh `get_xrefs_to` = 3: `FUN_0004CEF4 0x4CF51`, `FUN_0004CF7C 0x4D009`, `FUN_0004D2D4 0x4D3D9` |
| per-frame driver | `FUN_0004D2D4` (`0x4D2D4..0x4D3xx`), fresh `get_xrefs_to` = 2: main loop `FUN_000495B0 0x495DF` and `0x49892` (scene assembly `FUN_00049830` region) |
| presets | `(&0x108B64)[type]` = 6 pointers, `read_memory 0x108B64` = `6C 89 10 00 / C0 89 10 00 / 14 8A 10 00 / 68 8A 10 00 / BC 8A 10 00 / 10 8B 10 00` (0x10896C, 0x1089C0, 0x108A14, 0x108A68, 0x108ABC, 0x108B10); record = 6 dwords `{x,y,z,yaw,pitch,angle}` copied to camera `+0x10/+0x14/+0x18/+0x58/+0x5C/+0x4C` (`FUN_000505D0` decompile, multiple arms) |
| handlers | `(&0x108B80)[i]` = `read_memory 0x108B80` = `34 E8 04 00 (0x4E834), A8 E3 04 00 (0x4E3A8), 9C EC 04 00 (0x4ECA8), 38 DB 04 00 (0x4DB3C)` — 4 static entries; the `[cam+4]` index is clamped `>=0` with `3 -> 5` only for the *preset* lookup (`FUN_000505D0` decompile head) |

`FUN_000505D0` arms (mode = `FUN_00053D50()`, switch on `local_2c < 0x20`): 1/0x12,
3, 4, 6/0x10, 7, 8, 9, 0xB, 0x13/0x1C, 0x14, 0x15, 0x1D; each arm writes the
camera pose (`+0x10/+0x14/+0x18/+0x58/+0x5C/+0x4C`) from a preset record
(`local_20 = (&0x108B64)[(&(camera+0xC))]`, or `local_20 + k*6` with
`FUN_000504E0`/`FUN_00050518` for arms 3/4/8) and then calls
`(*(code*)(&0x108B80)[cam_index])(camera, preset)`.

**(b) camera `+0x4C` (the ratio angle) writers** — fresh instruction search
`+ 0x4c],` restricted to the camera band:

| site | function | value |
|---|---|---|
| `0x4D836` | `FUN_0004D7E8` | camera-type init: `puVar4 = *(*(&0x108B64)[type]+0x48) + arg*0x18`; `camera+0x4C = puVar4[5]` (decompile) |
| `0x5076F, 0x508C5, 0x509CF, 0x50BC7, 0x50C0C, 0x50CED, 0x50D92, 0x50DBE, 0x50E91, 0x50F18` | `FUN_000505D0` | per-frame pose writes |
| `0x4D31E` | `FUN_0004D2D4` | replay copy: `camera+0x4C = [0x109A84+0x14]` |
| `0x4D431, 0x4D43C(c 0x200), 0x4D44D(c 0x3A00)` | `FUN_0004D2D4` | smoothed-angle clamp (`FUN_0004C8D0` output integrated, clamped to [0x200,0x3A00]) |
| `0x4CA16/0x4CA21/0x4CA33` | `FUN_0004CA08` | view pan keys: `+0x4C = EBX / 0xBB8 / 0x1800` |
| `0x366A0, 0x36736 (0x1200), 0x367A9 (0x1370), 0x368FA (0x1118)` | `FUN_0003665C/0x366EC/0x3679C/0x368EC` | replay-camera preset writebacks |
| `0x4DDFE/0x4DE3B/0x4DE91/0x4DE9B/0x4DF2C` | `FUN_0004DDA8` | other view modes |
| `0x4F91E/0x4FBA3` | `FUN_0004F8C8` | replay camera |
| `0x4E820/0x4EC87/0x4F1B4` | `FUN_0004E3A8/0x4E834/0x4EC9C` | per-camera handler `ADD [ESI+0x4C], EAX` (the handlers from `0x108B80`) |

**(c) FU-71 follow writer (who drives `vel_x`/`vel_z` in live play)** — fresh
`get_xrefs_to 0x1577C0` = 91 / `0x1577C2` = 54 sites. Writers (excluding the
integrator `FUN_000736AC` and resets `FUN_000700F4`):

| function | sites | role |
|---|---|---|
| `FUN_00070544` | `0x7065B, 0x706AA, 0x70756` | pan/event setup: writes velocity words |
| `FUN_000709D0` | `0x70B43` | pan step (called by `FUN_000736AC 0x737DA` when `timer > timer_limit`) |
| `FUN_00070DE0` | `0x71053, 0x71274` | camera cut/reposition (387 insns, FU-71 §9 leg 4) |
| `FUN_00071DF4` | `0x71E63, 0x71E84, 0x71E9A` | ball sub-object event rate → velocity ramp |
| `FUN_0007131C` | `0x7197A` | boundary reflection negation |
| `FUN_0007A490` | `0x7A92D` | ball staging: **zeros** the velocity words in one arm |
| unnamed sites | `0x76892, 0x77241/0x772A3/0x772B8/0x772F5, 0x77C92/0x77CE6/0x77CFD, 0x852C4, 0x7F59E, 0x74BF6, 0x77D0A` | further gameplay writers |

The **producer of the pan/event itself** is `FUN_00071C94` (camera cut/event
setter: `FUN_000700F4()` reset, sets `timer_min`/`event_param`, calls
`FUN_00070544`), with 11 fresh callers: `0x71B8A`, `0x70DD1` (`FUN_00070DE0`),
`0x7A219` (`FUN_0007A084`), `0x6FA62`, `0x75DC4`, `0x77423`, `0x7F4E7`,
`0x82A6C`, `0x77DC6`, `0x7EFCF`, `0x7F0D1`. All are gameplay event bodies
(ball/kick/shot/goal/keeper rows). So the FU-71 follow writer is the gameplay
event machine; the reachable subset for the port is whatever the shot/kick paths
actually invoke.

---

## 3. Formation-id producer

`[0x14C1E4]`/`[0x14C1E5]` = per-side formation index (0..4). First-hand:

| role | function | evidence |
|---|---|---|
| direct setter | `FUN_0008EA70(side, value)`: `(&0x14C1E4)[param_1] = param_2; FUN_0007412C();` | decompile `0x8EA70`; callers fresh = 2: `0x3D3F4`, `0x3D519` (containing function `FUN_0003D2DC`, the formation select screen) |
| match-init producer | `FUN_00011620(side, team, desc)` and `FUN_00011978(side, rec, desc)`: copy the team record byte `(&0x14302A)[side*0x49]` (= team block `0x143018+side*0x49`, offset +0x12) into `(&0x14C1E4)[side]` | decompile `0x11620` (tail `0x1189B MOV [EDX+0x14C1E4],AL`), `0x11978` (`0x11A97`); block written by match init `FUN_00011B7C` (`0x11BB6/0x11BC5 CALL 0x11620`, decompile) and by `0x25Bxx`/`0x25Cxx` (league/practice init) |
| front-end cycling | `FUN_0003D2DC` cases 0/1 step `[0x14AE38+team*4]` through 0..4 (`0x3D2DC` decompile: `(x+1)%5`, `(x-1)<0 -> 4`), case 2 commits via `FUN_0008EA70` | decompile `0x3D2DC` |
| consumer | `FUN_0007412C`: `FUN_0008CE78(team0, &0x14C35C, [0x14C1E4])`, `FUN_0008CE78(team1, &0x14C3D7, [0x14C1E5])` (fresh `get_xrefs_to 0x8CE78` = 2, both this function) | decompile `0x7412C` |
| installer | `FUN_0008CE78(team, desc, id)` -> `team+0x7AA = desc`; `FUN_0006D9C4(team, id)`; then 11x `FUN_0008C758` accumulate into `team+0x827` | decompile `0x8CE78` |
| layout table | `FUN_0006D9C4`: `team+0x7AE = &0x11033A + id*0x1D`; for 4 blocks of 7 bytes `{role, count, slot..}`: `record[slot]+0x90 = role`, `FUN_0006D920(record)`; then `FUN_0004C384` into `team+0x7DB/0x7DF` | decompile `0x6D9C4`; `read_memory 0x11033A` = formation 0 `00|00 01 00|01 03 01 02 03|02 05 04 05 06 07 08|03 02 09 10`, formation 1 `01|00 01 00|01 04 01 02 03 04|02 04 05 06 07 08|03 02 09 10` (4-4-2) |
| match readers | `FUN_0008E810` `0x8E81E`, `FUN_0008E910` `0x8E92C` (entity loops), `FUN_00040080`/`FUN_00041494`/`0x3FEF8` | fresh `get_xrefs_to`/`search_instructions 14c1e` = 21 sites |

The engine currently hard-0s the formation id; the native default is also 0
(EXE image is BSS at `0x14C1E4`), and the id comes from the team record's +0x12
byte at init or the front-end formation screen.

---

## 4. Palette legs

### 4.1 Base palette `0x14B200` (FU-144 §6 leg 1) — closed

Fresh instruction search `14b200` = 10 sites (only these exist):

| site | function | action |
|---|---|---|
| `0x4793C` | `FUN_0004793C` | getter: `EAX = 0x14B200` |
| `0x4795B` | `FUN_00047944` | push arg |
| `0x479A8` | `FUN_000479A0` | reads `[EDX+0x14B200]` to build the 8-bit copy (`v<<2`, FU-144 §1) |
| `0x48C96/0x48CC2/0x48CF1/0x48CFB/0x48D05` | `FUN_00048C8C` | **sole writer**: `memmove(src -> 0x14B200, 0x300)` (copy helper `FUN_000CD390` arg1->arg2), then `FUN_000479A0`, `FUN_00047A70` (near-black key table `0x14BF28`) and the codec hooks |
| `0x48F1B/0x48F2F` | `FUN_00048ED8` | match build: `EAX=0x14B200` base for `FUN_00048B60`; snapshot to stack; `memmove(local -> 0x14B500, 0x300)` |
| `0x48Dxx` | `FUN_00048B60` | build-on-args (FU-144 §1) |

`FUN_00048C8C` fresh callers = 3: `FUN_00048D38` (`0x48D76`), `FUN_00048FF4`
(`0x4901E`), `FUN_00048ED8` (`0x48FD0`). Therefore:

* **base installer** `FUN_00048D38` (`0x48D38` decompile):
  `FUN_0004AFB8(0x32)` -> `FUN_000A1920(frame)` -> `FUN_00047814` -> preprocessing
  `FUN_000481EC`/`FUN_00048AB0`/`FUN_00047298(1)`/`FUN_000471EC` ->
  `FUN_00048C8C(chunk)` (installs PALsys frame as `0x14B200`) ->
  `FUN_00048B60(chunk)` (rebuild with the FU-98 kit remap/appends).
  The frame index is passed in EDX; its caller `FUN_0003726C`
  (`0x3726C..0x372BB`, first-hand) sets `[0x14C2FA] = settings[0xC]`
  (`FUN_0001D940(0xC)`; when that is 4, a random 0..3 via `FUN_00092AC8()&3`
  or `FUN_000A16EC()%4`) and calls `FUN_00048D38` at `0x372B4`. The second
  caller of `FUN_00048D38` is `0x4AE76` (match-load side).
* **restore** `FUN_00048FF4`: if `[0x1068E0]!=0`, `FUN_00048C8C(&0x14B500, ...)`
  — reinstalls the saved pre-match palette; then `[0x1068E0]=0` disables the
  kit-translation branch. Callers `0x384B0`, `0x3BF13` (front-end).

So the native base is **`PALsys.fsh` frame `[0x14C2FA]`** (settings slot 12,
default 0; 0..3 when 4) installed by `FUN_00048D38`, and the match load
(`FUN_00048ED8`) appends/remaps on top and snapshots to `0x14B500`. The
engine's base:=chunk substitution is the correct special case for settings 0
when the preprocessing passes are identity (leg: the four preprocessing
functions' full semantics).

### 4.2 Per-entity translation tables `0x14BF60 -> 0x114720` — producers found

| item | evidence |
|---|---|
| pool partitioner | `FUN_00046F80(base)` (`0x46F80` decompile): `0x14BF60[i] = base + i*0x100` for 23 slots (0x5C/4), `0x14BF34[i]` 7 slots, `0x14BB00[i]` 8 slots, fixed slots `0x14BF50/0x14BF20/0x14BF54/0x14BF2C/0x14BF28/0x14BF30/0x14BF5C/0x14BF58/0x14BF24` at base+0x2800..base+0x3000, `0x14BB20[0..255] = base+0x2600`, `0x14BFBC = base+0x2600` |
| sole caller | `FUN_00049138` (`0x49138` decompile, fresh xref = 1): when EAX!=0 allocates `DAT_00107290 = FUN_0004A448()` then `FUN_00046F80([0x7290])`; when EAX==0 frees `[0x7100]/[0x7104]/[0x7290]` (the FU-111 §2 orphan teardown tail calls it with EAX=0 at `0x4AF0A`) |
| slot consumer | `FUN_00048DC0(param)` (`0x48DC0` decompile): when `[0x1068E0]==1` and param in {0, 0xB}, uses `(&0x14BF60)[param]` (teams A/B kit ranges): builds a 256-byte translation via tables `0x107287`/`0x10727C` with base bands 0x94/0x9B (param 0) or 0x82/0x89 (param 0xB) and local bases 0xA1/0xA3 (param 0) or 0x9C/0x9E (param 0xB) |
| table install | `FUN_000CE980(src)` (`0xCE980`): copies 0x40 dwords into `0x114720` (the FU-57 §7 indexed translation table). Fresh callers = 11 (`FUN_000490F4 0x490FE`, `FUN_00048DC0 0x48EC1`, `0x4905B/0x49072/0x490AC/0x490BF/0x490D6`, `0x62FCD`, `0x5486E/0x5493B/0x54960`) — identity/copy variants in the 0x490xx sprite-state helpers |
| per-draw callers | `FUN_00048DC0` fresh = 2: `0x56478` (draw path `0x563xx`, FU-84 §7) and `0x5721A` (`FUN_00057158` per-entity draw) |
| identity reset | `FUN_000471EC`: fills `0x14BF24` identity 0..255 + team bytes |

So the per-entity translation install is: pool partition
(`FUN_0004A448` buffer -> `FUN_00046F80`) at match-data load
(`FUN_00049138(1)`), per-draw rebuild `FUN_00048DC0(entity 0/0xB)` ->
`FUN_000CE980` -> `0x114720`, and identity copies via the 0x490xx helpers.
The resource identity of the pool buffer (`FUN_0004A448` -> `FUN_00098C38`/
`FUN_00098BF8` allocators; pool size needed >= base+0x3100) remains a leg.

**FU-152 pointer (2026-10-09):** FU-152 confirms the partition as corrected in §11.4/§11.5-7 — 23+7+8 slots at `+0x0000`/`+0x1700`/`+0x1E00`, shared `+0x2600`, 9 fixed `+0x2700..+0x2F00`, floor `0x3000`; the w7-b4 draft's contradicting values (`+0x2400`/`+0x2C00`, floor `0x3600`) were corrected in FU-152 §2.10/§4.4.

### 4.3 Shade cube

FU-98 §2 (`FUN_000A0AA0` RGB->index quantisation cube, `2^B` per channel; built
from the 1024-byte BGRA table via `FUN_000A154C` -> `FUN_000A129C`) is unchanged;
its only reachable use is the sprite blender (`FUN_000AFBFC` path), not the
match draw. No new first-hand work needed for the HUD (the HUD text uses the
indexed colours above, not the cube).

---

## 5. Derived semantics

### 5.1 HUD draw flow

```
main draw loop FUN_000495B0 (per drawn buffer)
  -> FUN_00049830 (scene)
  -> FUN_000565BC (overlay presenter)
       gates: [0x14E538]==0, not replay [0x109A98]==0, [0x14E688]==0
       HUD branch: [0x14E510]==0 && FUN_0006400C()!=0 && FUN_0004B5F4()<4 && [0x14E59C]==0
  -> FUN_00055C24
       if FUN_0001D940(6)==0 -> return
       period = [0x157AC2]; displayed = period+1 (or [0x14E5A0]+period in modes 0xC/0x13/0x14)
       clock  = [0x157AB4] -> minutes/seconds
       score  = FUN_0004B5DC -> {[0x157AC5], [0x157AC7]}
       names  = FUN_00011BEC(0/1) upper-cased by FUN_00017748
       font   = slot 0x36 "playfnt.fsh" (zoomed) | 0x35 "clockfnt.fsh" (full)
       layout = [0x14E514..0x14E534] (FU-93) + [0x14E658] dims
       draw: bar -> name0 -> name1 -> score0 -> score1 -> period -> clock
```

### 5.2 Camera mode/angle feed

```
main loop FUN_000495B0 -> FUN_0004D2D4 (per-frame view driver)
    if replay-ish: copy [0x109A84] pose (incl. +0x4C) into the camera
    else if camera +4 < 4 && [0x107DD8]==0 && pad idle: FUN_000505D0()
        -> mode = FUN_00053D50() = [0x14E57C]
        -> pose = (&0x108B64)[camera +0xC] (+ sub-index for modes 3/4/8)
        -> write +0x10/+0x14/+0x18/+0x58/+0x5C/+0x4C; call (&0x108B80)[cam+4]
    camera +0x4C also init-written by FUN_0004D7E8 (type entry[5]) and
    clamped to [0x200,0x3A00] after the FUN_0004C8D0 smoothing.
Follow integrator FUN_000736AC (ported as fifa96_camera_update) consumes
    vel_x/vel_z/event_param/timer; producers are the event machine
    FUN_00071C94 (+ FUN_00070544/0x709D0/0x70DE0/0x71DF4) called from 11
    gameplay sites; the ball staging FUN_0007A490 zeroes velocity in its arm.
    T2 landed the producer chain: event_set is complete (bail gate, ramp sign
    param, the corrected timer cell, the fast/slow paths, the > 0x19 atan
    walk), pan_step/reposition/rate_table are derived ports, the update calls
    pan_step at the native site, and row 04's event outputs are wired live
    (§12).
```

### 5.3 Palette install graph

```
boot/front-end/settings: FUN_0003726C -> [0x14C2FA]=settings[0xC] (or rnd 0..3)
    -> FUN_00048D38 -> PALsys.fsh frame [0x14C2FA] -> preprocess -> FUN_00048C8C -> 0x14B200
       -> FUN_00048B60 (FU-98 remap + appends) -> FUN_000A154C (register + shade cube)
match load: FUN_00048ED8 -> build from 0x14B200 -> snapshot 0x14B500 -> FUN_00048C8C -> 0x14B200
             -> [0x1068E0]=1 enables FUN_00048DC0 kit translation
restore: FUN_00048FF4 -> FUN_00048C8C(&0x14B500) -> [0x1068E0]=0
translation: FUN_00049138(1) -> FUN_0004A448 pool -> FUN_00046F80 (23x0x100 @0x14BF60)
             per draw: FUN_00048DC0(0/0xB) -> FUN_000CE980 -> 0x114720
```

---

## 6. Port contract

### 6.1 HUD (P0.2) — `fifa96_match_run_render` / new `fifa96_match_hud`

Names to use in the engine; everything below is first-hand evidenced above.

State (already in the run):
* `mr->score[0]` / `mr->score[1]` <- `[0x157AC5]`/`[0x157AC7]` (existing).
* `mr->state.total_seconds` <- `[0x157AB4]` (existing field; displayed clock).
* `mr->state.period_seconds` <- `[0x157AB6]` (not displayed by the HUD).
* `mr->state.period` <- `[0x157AC2]`; displayed period = `period + 1`.
* Gate: draw only when `mr->render.enabled` and `!mr->render.display.suspend`
  (the `[0x14E688]==0` analog) and period < 4 and not replay. The native
  settings gate `FUN_0001D940(6)` and `[0x14E510]`/`[0x14E538]`/`[0x14E59C]`
  have no engine producer — treat as "asset readiness" (leg 6).
* Layout: recompute from FU-93's `fifa96_window_scale` inputs
  (`render.window_scale_x`, `render.window_zoomed`, window rect
  `x0,y0,x1,y1`): `row_pitch = 9|0xC`, `x_left = x0+3|x0+6`,
  `clock_x = x0+0x23|x0+0x32`, `bar_x = x0+1|x0+2`,
  `bar_y = y1 - bar_h - 1 (zoomed) | y1 - bar_h - 2 (full)`, `y2 = bar_y + 2`.
* Font: stage `clockfnt.fsh` (slot 0x35) and `playfnt.fsh` (slot 0x36) from the
  pitch container by BIGF name (the staging path already name-matches
  `PALsys.fsh`); decode the `FUN_000984B4` header layout and blit glyphs with
  `FUN_0009FFF0`'s algorithm (depth byte, per-char width/height/offset tables)
  into `s->indexed`; outline pass colour index 6 then main pass colour 0
  (`(&0x1050A4)[idx]`; statically zero, the engine may define the two palette
  indices explicitly — leg 7).
* Strings: `%d` for scores/period, `%02d:%02d` for the clock; team names via
  whatever team-name strings the run stages (native `FUN_00011BEC(side)`; the
  engine has no team block — leg 8).
* Draw order: bar -> name0 -> name1 -> score0 -> score1 -> period -> clock,
  with the derived positions (the HUD is bottom-anchored; the zoomed branch is
  the windowed one).
* Acceptance: HUD pixels are a render change -> M2 tape re-pin with frame diff
  (expected by the plan, P0.2).

### 6.2 Camera

* Feed `mr->render.yaw/pitch` and `mr->render.view_ratio` from the
  `FUN_000505D0` pose model: preset table `(&0x108B64)[type]` (6 dwords
  `{x,y,z,yaw,pitch,angle}`) selected by `[0x14E57C]` (engine: a `view_mode`
  field) and sub-index arms 3/4/8; write `+0x10..+0x18`, `+0x58` (yaw), `+0x5C`
  (pitch), `+0x4C` (ratio angle -> `view_ratio`).
* Init: `mr->render.view_ratio` may be seeded from `FUN_0004D7E8`'s type entry
  `[5]` instead of the current static default (closes the FU-96 leg-5 note).
* Follow: `fifa96_camera_update` stays the integrator; to move the camera during
  play, the engine needs an event/pan producer — the unported subset of the
  `FUN_00071C94` callers documented in §2.1(c). Reachable first step: a
  kickoff/restart pan using `FUN_00070544` semantics is not derivable from this
  slice alone (leg 9).
* `fifa96_match_render.c` does not exist in the tree; the render seam is
  `fifa96_match_run_render` in `src/fifa96_engine/fifa96_match_run.c`.

### 6.3 Formation id

* Add `uint8_t formation[2]` to the run (or to the team staging): seed from the
  team record byte (+0x12 of the native `0x143018+side*0x49` block) exactly as
  `FUN_00011620` does; default 0 (native image value).
* Consumer contract: `formation_layout = &0x11033A + id*0x1D` (4 blocks of 7
  bytes `{role, count, slot..}`; role written to record byte +0x90). The engine
  can index the same table (it is in the EXE image, not the ISO).
* The front-end cycle (`FUN_0003D2DC`, 5 values 0..4) is out of scope for the
  headless port.

### 6.4 Palette

* Base: the engine's `fifa96_match_run_stage` can set `base6` from `PALsys.fsh`
  frame `[0x14C2FA]` (settings slot 12; default 0) instead of the current
  base:=chunk, closing FU-144 §6 leg 1 at the contract level. The preprocessing
  passes (`FUN_000481EC`/`FUN_00048AB0`/`FUN_00047298`/`FUN_000471EC`) are leg 10.
* Translation: expose `render.remap` writes through a `fifa96_match_translation`
  seam driven by `FUN_00048DC0(0/0xB)` (kit ranges 0x94..0xA6 / 0x82..0x94,
  base bands 0xA1/0xA3 and 0x9C/0x9E) when the pool tables are available; the
  0x490xx identity helpers keep the current identity stand-in. Pool resource
  identity is leg 11.
* `0x14B500` snapshot/restore maps onto the engine's staged palette lifetime.

---

## 7. Numbered legs

1. **HUD `FUN_0001D940(6)` gate** (`[0x149290]`): no static writer found
   (`get_xrefs_to` = 0); the settings table `0x149278` is zero in the image.
   Engine maps it to asset-readiness.
2. **`[0x14E658]` bar dims pointer**: only 2 references (both reads in
   `FUN_00055C24`); no static writer — the writer is indexed (in the
   `FUN_00053240` layout block). Engine supplies its own bar dims.
3. **Bar fill colour**: `FUN_0009B850` fills spans via `FUN_000B1730`/
   `FUN_000CE9D8`/`FUN_000CEB9A`; the colour-index source is not derived.
4. **Font glyph colour plumbing**: `FUN_00019E9C(idx)` sets
   `(&0x1050A4)[idx]` (BSS) and `[0x1050E4+idx]`; the exact byte pushed into
   `FUN_000CEF64` from `FUN_0009FFF0` is hidden in the register/stack argument
   setup. The two-pass indices (6 outline, 0 main) are certain; the palette
   bytes they resolve to are runtime-filled.
5. **HUD when `[0x14C32A]!=0`** (`0x55CCD..0x55D25`): uses string-table ids
   0x1EA/0x1E9 as team-name substitutes; the mode is not derived (extra-time /
   penalties display).
6. **`FUN_00056518`, `FUN_000563D0`, `FUN_000560C8`, `FUN_0005619C`,
   `FUN_000564A0`, `FUN_00064C34`**: the remaining overlay draws in the
   presenter; only the match HUD is derived.
7. **Team-name stage**: the engine has no native team block
   (`0x143018+side*0x49`); the HUD name source needs a staging decision.
8. **Camera `[0x14E57C]` writer 0x51xxx**: which view handler sets which mode
   value (and when) is cited only.
9. **FU-71 event producers**: the 11 `FUN_00071C94` callers and
   `FUN_00070544`/`FUN_000709D0`/`FUN_00070DE0` internals are unported
   gameplay bodies; no reachable kickoff/restart pan was proven from this
   slice.
10. **Base palette preprocessing** `FUN_000481EC` (large sprite/palette
    translation setup), `FUN_00048AB0`, `FUN_00047298` (kit/team colour
    translation), `FUN_000471EC` (identity resets) — cited, not decomposed.
11. **Palette pool resource identity**: `FUN_0004A448` -> `FUN_00098C38`/
    `FUN_00098BF8` allocators; which loaded file fills `[0x7290]` and its exact
    size (>= base+0x3100) is not derived.
12. **`FUN_0004A6BC` font slot names**: `clockfnt`/`playfnt` strings verified in
    the parameter table and present in `gameart0.pvi`; the `.fsh` suffix comes
    from the FU-144 §2 format loop for slots 39..55 (not re-derived here).
13. **`FUN_0004C8D0` smoothing/clamp object**: the `+0x4C` clamp at
    `0x4D431..0x4D44D` writes through a register artifact
    (`extraout_ECX`); the owning object (camera vs `0x14E4E0` copy) is not
    proven.
14. **HUD `[0x14E5A0]`** (mode-dependent period offset, `0x55C57`): runtime
    writer not located.

---

## 8. Risks

* **HUD text fidelity**: the port depends on the FSH font decode. The header
  parser is derived, but the glyph tables' exact per-char height/flag semantics
  (`DAT_00112800`/`DAT_00112804`/`DAT_00112808`) are only partly traced; a
  minimal implementer may prefer a derived bitmap font and re-pin the tape
  (risk to the P0.2 acceptance only at the pixel level).
* **Colour indices**: the HUD's outline/main palette bytes are runtime-filled;
  the engine must pick two explicit indices (leg 4) — a documented divergence.
* **Camera**: the mode/preset feed is portable now; the *motion* during play
  (FU-71 event machine) is the deep part and may remain a leg (matches the
  plan's honesty clause).
* **Palette**: base frame/branch is portable; the preprocessing passes are the
  gate to bit-exactness.
* **Tape churn**: HUD pixels + camera feed will move the M2 golden; re-pin with
  a written reason and frame diff (expected by the plan).

## 9. Provenance (Ghidra MCP, read-only, `/FIFA96.EXE`)

`disassemble_function` 0x55C24 (343 insns), 0x55BA8, 0x9BB20, 0x9B850 (window),
0x736AC (FU-71 re-check window), 0x3726C, 0x372BC, 0x48D38 (site window
0x372A0/0x4AE60); `disassemble_bytes` 0xA06FC (48 B), 0x372A0 (40 B), 0x4AE60
(48 B); `decompile_function` 0x4C2A0, 0x4B5F4/0x4B594/0x4B588/0x4B454/0x4B408,
0x4B5DC, 0x55C24 (incl. 0x58484/0x5849C/0x584B4/0x58630/0x9FFF0/0x9BB20/0x9AFD0
helpers), 0x565BC, 0x495B0, 0x6400C, 0x63FD0, 0x1D940, 0x37AE4, 0x8AF38, 0x505D0,
0x4D2D4, 0x4D7E8, 0x53D50, 0x71C94, 0x7A490, 0x3CD30, 0x7412C, 0x8EA70, 0x11620,
0x11978, 0x8CE78, 0x6D9C4, 0x11B7C, 0x3D2DC, 0x48C8C (via sites), 0x48D38,
0x48FF4, 0x46F80, 0x49138, 0x48DC0, 0xCE980, 0x471EC, 0x481EC, 0x48AB0,
0x47298, 0x4A448; `read_memory` 0x101DE0 (128 B), 0x101BB0 (96 B),
0x107370 (256 B), 0x107490 (32 B), 0x108B64 (64 B), 0x108B80 (read via 0x108B64
window), 0x1050A4 (80 B), 0x149278
(40 B), 0x14E650 (32 B), 0x11033A (64 B); fresh `get_xrefs_to` 0x55C24 (1),
0x565BC (3), 0x14E510 (2), 0x14E59C (5), 0x14E688 (9), 0x14E538 (10),
0x157AC5 (95), 0x157AB4 (16), 0x157AB6 (38), 0x14C1E4 (10), 0x14C1E5 (2),
0x14E57C (12), 0x505D0 (3), 0x4D2D4 (2), 0x14BF60 (4), 0x48DC0 (2), 0xCE980 (11),
0x48C8C (3), 0x48D38 (2), 0x46F80 (1), 0x49138 (2), 0x8CE78 (2), 0x8EA70 (2),
0x1577C0 (91), 0x1577C2 (54); `get_bulk_xrefs`/`search_instructions` operand
`14c1e` (21 hits), `14b200` (10 hits), `14e658` (2 hits), `+ 0x4c],` (82 hits,
camera-band subset tabled); `get_function_by_address` 0x4D836, 0x3D3F4,
0x11BB6, 0x372B4.
Retail data: `/ART/GAMEART0.PVI` extracted from `game/FIFAPCCD96.iso` (xorriso,
154387 B, sha not recomputed here) and the by-name lookups via
`build/fifa96_play sprite` for `clockfnt.fsh`/`playfnt.fsh`; native names read at
0x101BC0/0x101BCC.
No Ghidra writes, no project saves, no repo edits; this draft is the only file
written.

---

## 10. Port errata — OL-T11-7 landed (M2 full-gameplay P0.2, 2026-10-09)

The HUD port (`fifa96_match_run_stage` + `fifa96_match_run_render` +
`src/fifa96_loader/fifa96_font.c`; tests `test_font`,
`test_engine_match_render::test_hud_*`, `test_engine_match_staging`
ISO cases) re-verified §1 first-hand. Corrections and closures:

1. **§7 leg 2 CLOSED — the `[0x14E658]`/`[0x14E634]` writer is static.**
   `FUN_00053930` (single caller `FUN_0004AD4C`) runs
   `FUN_0004AFB8(0x2F)`, takes the bank's frame count and stores each frame
   pointer at `(&0x14E624)[i]` (`0x53994/0x539AA` are frame 9/10 stores; the
   rest are the loop). Resource slot 0x2F resolves through the `0x107370`
   name table to `0x101B8C` = "Frames" and the slots-39..55 `%s.%s` loop with
   the constant `fsh` at `0x101C90`, i.e. **Frames.fsh** (GAMEART0 BIGF entry
   43, 15 frames). The HUD's pointers are therefore Frames.fsh frame
   descriptors: `[0x14E634]` = frame 4 (layout height 41), `[0x14E658]` =
   frame 13 (the drawn bar, 60x41; pixel (0,0) = 0x45), `[0x14E638]` =
   frame 5 (overlay scaler A), `[0x14E648]/[0x14E64C]` = frames 9/10 (then
   conditionally zeroed by the `FUN_00011BD4` side results). The engine
   stages the bank by name and draws frame 13 with frame 4's height.
2. **§1.4 register names**: `ESI` (0x12/0x18, +6/+8 when wide) is the
   **clock cell width** (`0x55E58/0x55EBA` set it just before the wide check;
   `0x5609C MOV ECX,ESI` feeds FUN_00055BA8), not the bar height
   (`local_28` = `[ESP+0x90]` = 0xE/0x11 is the clock cell x offset from x0,
   `0x5608A/0x560A1`; `local_20` = 2/3 is the cells' y offset). The bar's
   dimensions come only from the frame-13 descriptor.
3. **§1.4 cell y**: the period/clock row is
   `[0x14E52C]+[0x14E534] + 2*[0x14E51C] + local_20` (`0x56068..0x56085`),
   i.e. `bar_y + 2*row_pitch + 2|3`; `y2 = bar_y + local_24(2)` feeds only
   the name/score rows. `local_24 = 2` both modes.
4. **`[0x14E534]/[0x14E530]` are menu-overlay outputs**: `FUN_000550E4`
   zeroes them on entry (`0x55169/0x5516E`) and recomputes
   `[0x14E534] = min(0, y1 - [0x14E52C] - 0x36)`, `[0x14E530] = 2*` at
   `0x55205..0x55233`; live play without the menu leaves 0 (also the reset
   `FUN_000537F8`). The engine keeps the live-play zero offset.
5. **§1.5 font layout confirmed and completed**: the `[+0x1C]` block has its
   own 16-byte header — mode byte `0x79/0x7A/0x7B` (FUN_000AFBFC → 1/4/8
   bpp; the retail block says 0x7A), u16 pixel width at +4, u16 row count at
   +6 — and the glyph base is block+0x10. The retail fonts are 4bpp:
   clockfnt (BIGF entry 53, slot 0x35) 682x13, playfnt (entry 54, slot 0x36)
   368x7. Per char, the +0x20 dword is {low16 bit column, high16 row} into
   the shared bitmap; the row stride is `(bitmap_width*depth+7)>>3`; glyph
   pixels are read at column `bit+gx` (high nibble first) and a zero nibble
   is transparent. The decode is pixel-verified against the retail '0'
   (test_font's 32-pixel raster) — §8's "only partly traced" risk is closed
   at this level.
6. The native font selection is `FUN_0004AFB8(0x36)` when `[0x108DDC] <
   0x10000` else `0x35` (playfnt/clockfnt), matching §1.5 — the engine
   stages both and selects on `render.window_zoomed`.

### Remaining numbered legs (OL-T11-7x)

* **OL-T11-71 (glyph colour ramp / FU-148 leg 4)**: the native 4bpp plot
  path maps each glyph nibble through the runtime-filled `0x15BBAC` shade
  table (`FUN_00019E9C` → `FUN_000AFCB0`/`FUN_000B2A90`); the engine draws
  the two documented flat indices (6 outline, 0 main) for every non-zero
  nibble, so anti-aliased edge pixels are binary. Documented divergence.
* **OL-T11-72 (team-name stage / FU-148 leg 7)**: the native name source
  (`FUN_00011BEC(side,0,0)` → `FUN_00017748` filter over the unported team
  block `0x143018+side*0x49`) is not wired; the run stages empty names and
  exposes `render.hud_name[2]` so a future producer can fill them (the name
  pass, its placement and the wide-name shift are ported and tested).
* **OL-T11-73 (extra-time period/clock / FU-148 legs 5/14)**: the native
  branch `FUN_0004B454() != 0` re-labels the names with string-table ids
  0x1EA/0x1E9 and adjusts the clock via `FUN_0004B4CC`; the mode-dependent
  period offset `[0x14E5A0]` writer is still unlocated. The engine has no
  extra-time mode and displays `period+1` with the unadjusted clock.
* **OL-T11-74 (settings/suppress gate / FU-148 leg 1 + §6.1)**: the
  `FUN_0001D940(6)` settings cell has no static writer and the
  `[0x14E510]`/`[0x14E538]`/`[0x14E59C]` suppress writers stay unported; the
  engine maps the gate to "HUD assets staged" (bar + the window branch's
  font) and the existing `render.display.suspend` flag.
* **OL-T11-75 (bar blit exactness / FU-148 leg 3)**: the native zoomed bar
  scaler is `FUN_0009B850`'s 16.16 span stepper (including its per-pixel
  colour path); the engine draws the frame-13 indices 1:1 in the full
  window and nearest-neighbour at the derived `size*0xB800>>16` size in the
  zoomed one, with source pixel 0 transparent. FU-148 leg 3's "fill colour
  source" remains the open part.

---

## 11. Port landing (S4 presentation completion, 2026-10-09)

Phase-6 wave-2 S4 (`docs/superpowers/plans/2026-10-08-fifa96-m2-phase7-recon-ahead.md`)
landed §2–§4 as far as reachable, with the phase-7 w7-B4 draft as a
verify-only lead. First-hand `/FIFA96.EXE` re-derivation this slice; the
authoritative corrections to both docs are in the errata below.

### 11.1 Camera pose feed (FU-96 legs 1/3, §2.1(a)/§6.2)

`src/fifa96_loader/fifa96_camera.c`:

| landed | native | notes |
|---|---|---|
| `fifa96_camera_pose_blocks[6]` | behavior table `0x108B64`, 6 x 0x54 | class `+8`, pose array `+0x4C` (image default), 8 records x 6 dwords `{x,y,z,yaw,pitch,angle}` |
| `fifa96_camera_pose_apply` | FUN_000505D0 shared arm | writes camera `+0x10/+0x14/+0x18/+0x58/+0x5C/+0x4C` |
| `fifa96_camera_pose_feed` arms 1/0x12 | record 0 + the selector 1/3 and class 2/4 mirrors (`FUN_0004B818`) | full arm |
| arms 3/4 | record `{3|4}`/`{1|2}` via FUN_000504E0/FUN_00050518 | record selection + pose writes; the mode-3 replay copy/offset/z clamp and the mode-4 sub mirror are legs |
| arm 6/0x10 | record 7 + the `sub < 0` mirror | full arm |
| arm 8 | record 6 when `sub < 1`, else 5 (default selector) | selector 1/3 branch + mirror tails are legs |
| arm 0x15 | fixed record `0x108714` + yaw fold + z negate | the `camera+0x3c` / `[0x108C38]` clamp is a leg |
| default | handler call `(&0x108B80)[selector]` | legs (w7-B4 §2.8 leg 9) |

Engine wire: `render.camera_pose` staging, applied once per granted frame
after `fifa96_camera_update` and before the armer. `view_mode` is the native
`[0x14E57C]`; the 0x51xxx view-handler writers stay unported (FU-148 leg 8),
so a fresh match applies no pose (default 0 = the unported handler arm) and
the tape is unchanged. `fifa96_camera_pose_feed`/`apply` are pure
loader-level functions (block 0..5 range-checked; the native indexes blindly).

Pose tables first-hand `read_memory` (all `/FIFA96.EXE`): `0x10896C` (504 B
behavior blocks), `0x107F1C`, `0x1080FC`, `0x1082DC` (the three `+0x4C`
arrays), `0x108714` (mode-0x15 record), block `+8`/`+0x4C` at
`0x108A70/0x108AB0/0x108AC4/0x108B04/0x108B18/0x108B58`.

### 11.2 FU-71 event bodies (§2.1(c)/§5.2)

`fifa96_camera_event_set` (the derived FUN_00071C94 + FUN_00070544 subset):
preserves the target triple (= the engine position; the native 71C94 call
hands FUN_000700F4 the current target), resets the event state, copies the
seed step pair (the native 6-byte vector at `0x1577B8`:
`{bearing, step_x, step_z}`), clamps the height to `[target y, 0x640]`
(zeroed again when the clamped value is < 1, the native
`FUN_00070544` `0x705f1`/`0x705fe` cell clear), runs
the `FUN_000702F8` ramp over the pinned 400-byte `0x10F4EE` table
(`fifa96_camera_ramp`), computes the signed fast-path velocity
`seed / timer` (IDIV semantics), the bearing magnitude via
`fifa96_entity_distance` (= FUN_0008DC68; the engine's `speed` = native
`0x1577BE`), the step products and the anchor A/B sets (`0x157770/88/94`,
header A y forced 0 as the native zeroes `0x157774`).

The chain closes the S2 L1 hand-off at the producer level: the rate words the
FU-71 integrator consumes are now produced by the ported event machine (test
`test_camera_pan_event_chain`: seed 2000/height 0x30 -> `vel_z = 40`,
`timer = 50` -> one granted frame lands `z = 0xB50` -> armer -> zone 1 ->
queued situation 5 -> S3 consumer scores). The *natural* invoker stays
absent: the 11 `FUN_00071C94` callers are the unported gameplay-row bodies
(`0x70DD1`, `0x7A219`, `0x6FA62`, `0x75DC4`, `0x77423`, `0x7F4E7`, `0x82A6C`,
`0x77DC6`, `0x7EFCF`, `0x7F0D1`; `0x71B8A` is the armer's own angle arm,
which requires pre-existing event state: its `[0x1577EE].hi == 0 &&
[0x1577BE] == 0` early return gates a cold start). Legs: FUN_000709D0
(pan step), FUN_00070DE0 (boundary/reposition), FUN_00071DF4 (ball sub-object
rates), the `> 0x19` atan walk (FUN_000CD474), FUN_000703E8's corner cells,
the height `> 0x70` `[0x157A6D]` branch, the `0x15780C/0E` smoothing words,
the tracked-player/event-rate tail ([player+0x20], table 0x10E169) and the
sound sinks.

### 11.3 Formation id (§3)

Run field `formation[2]` (`[0x14C1E4]/[0x14C1E5]`, BSS 0), writer
`fifa96_match_run_set_formation` (FUN_0008EA70: `(&0x14C1E4)[side] = id` +
the FUN_0007412C consumer), layout `fifa96_match_formation_layout` over the
pinned 0x11033A rows and placement names
`fifa96_match_formation_fmt_name` (the 0x14BFC0 `6*id` slot built by
FUN_0004A6BC over the 0x107370 loader table: id 0 `352ko.fmt`, 1 `442ko.fmt`,
2 `swko.fmt`, 3 `424ko.fmt`, 4 `433ko.fmt`; first-hand strings 0x101A30,
0x101A64, 0x101A8C, 0x101AB4, 0x101ADC). `match_run_formation_seed` now
reads the run's derived id instead of the hard-coded 0. Legs: the FUN_00011620
team-record (+0x12) producer and the FUN_0007412C layout install (no engine
team+0x7AE/record+0x90 fields).

### 11.4 Palette residual (§4.2)

`src/fifa96_loader/fifa96_palette.c`: `fifa96_palette_pool_partition`
(FUN_00046F80: 23+7+8 slots at `+0x0000`/`+0x1700`/`+0x1E00`, shared
`+0x2600`, 9 fixed `+0x2700..+0x2F00`; floor 0x3000),
`fifa96_palette_translate_kit` (FUN_00048DC0 kit bands, tables 0x107287
`{0,0,0,1,1,1,1}` / 0x10727C `{0,0,0,1,1,1,1,2,2,2,2}`) and
`fifa96_palette_translate_slot` (FUN_000CE980 0x100-byte copy), plus the
engine seam `fifa96_match_run_translation_install(entity)` writing
`render.remap` (the 0x114720 analog) through a caller-staged
`render.palette_pool`. The install is not wired into the render path: the
pool content producer is still leg 11, so the identity remap stays the
stand-in. Shade cube: w7-B4 §2.10's "no static consumer" re-verified (the
`"inversetbl"` 0x20000 allocation has no static reader), so it stays a leg
and is not part of the match contract.

### 11.5 Errata (first-hand; corrects FU-148 and/or w7-B4)

1. **Pose-array selection.** The `+0x48` and `+0x4C` block fields are both
   non-NULL pointer arrays (`0x107E2C`/`0x107F1C` etc.). The head of
   `FUN_000505D0` uses `+0x48` only when `FUN_0004B7D0() == 0 &&
   FUN_0004B6FC() == 0`; `FUN_0004B7D0` returns `[0x1590CC + side*0x835] == 0`,
   which is 1 for the image's zero flags, so **the image default is `+0x4C`**.
   FU-148 §2.1(a) ("+0x48 array ... NULL in the image") and w7-B4 §2.8
   ("`+0x4C` when the predicates are nonzero") are both wrong; the engine
   pins the three distinct `+0x4C` arrays.
2. **Pose-table shape.** `0x108B64[type]` is not a 6-dword preset record: it
   is the 0x54-byte behavior block whose `+0x4C` is the pose array pointer.
   FU-148 §6.2's "preset table (6 dwords) selected by [0x14E57C]" is a
   shorthand; the record selection is per-arm (1/0x12 record 0, 3/4 the
   variant records, 6/0x10 record 7, 8 records 5/6, 0x15 fixed).
3. **Handler table.** `0x108B80` = `{0x4E834, 0x4E3A8, 0x4EC9C, 0x4DB38}`
   (w7-B4 already corrected FU-148 §2.1(a)'s `0x4ECA8`/`0x4DB3C`).
4. **`FUN_000504E0`/`FUN_00050518` predicate.** The "left" record (3/1) is
   selected iff `(class == 2 && x >= 1) || (class != 2 && x < 0)`; the
   decompiler's conjunction collapses to that (not merely the sign of x).
5. **Event height cell.** The height is the word at `0x1577F0` (the high
   word of the dword `0x1577EE`, which also holds the cursor in its low
   word) — w7-B4 §2.9's "`[0x1577EE].hi` = height" and the FU-71
   `event_param` mapping agree once the word split is read.
6. **FUN_00070544 param.** The ramp's EDX/EAX passthrough is the caller's
   register in `FUN_00071C94` (a decompiler artifact; `FUN_000709D0` passes
   0). The port fixes param 0 (the NEG F6 path: `F6 = F2 - ramp(h - ty)`).
7. **Pool floor.** `FUN_00046F80` writes up to `base+0x2FFF` (shared table
   `+0x2600`, 9 fixed blocks `+0x2700..+0x2F00`), floor **0x3000**; FU-148
   §4.2's "base+0x2800..base+0x3000" is off by one slot and w7-B4 §2.10's
   "ends at base+0x3600" is wrong.
8. **Pose-feed ordering.** The native driver FUN_0004D2D4 runs from the draw
   loop (FUN_000495B0), after the frame-body armer; the engine applies the
   staged pose at the frame-body camera site so a fed pose is observable to
   the same frame's armer (the engine `pos` doubles as the native 0x15774C
   event target under the accepted FU-71 mapping). The `[0x107DD8]`, pad-idle
   (FUN_00037AE4) and replay gates stay legs.

### 11.6 Numbered legs (S4 additions)

* **OL-T11-76 (camera handlers / FU-148 leg 8 + w7-B4 leg 9)**: the four
  `0x108B80` handler bodies (`0x4E834`, `0x4E3A8`, `0x4EC9C`, `0x4DB38`) and
  the default-arm invocation; the horizon/plane globals
  `0x14E4D0/D4/D8/DC`.
* **OL-T11-77 (view-mode writer / FU-148 leg 8)**: the 0x51xxx view handlers
  that set `[0x14E57C]`; the engine stages `render.camera_pose.view_mode`.
* **OL-T11-78 (+0x48 alternate pose arrays)**: the `[0x1590CC]` team-flag /
  `FUN_0004B6FC` predicate and the second pose arrays; the selector-3 preamble
  pose arrays `0x1083CC`/`0x1084BC` (reached by arms 3/4/6/8 when
  `[camera+4] == 3`; the degenerate behavior-index read at `[0x108B60]`).
* **OL-T11-79 (event-machine bodies)**: **narrowed (T2, §11.7)** — the
  FUN_00071C94/FUN_00070544 event setter is complete (bail gate, ramp sign
  param, corrected timer cell, slow path, > 0x19 atan walk, tails),
  FUN_000709D0/FUN_00070DE0/FUN_00071DF4 have derived ports and the row-04
  gameplay caller is wired live. Carried: the `> 0x70` anchor branch, the
  FUN_00065CF8/FUN_000974DC/FUN_000651F0/FUN_000974F0 sound sinks,
  FUN_000703E8, the 0x15780C/0E smoothing words, the tracked-player tail
  (0x1577CA/CE + table 0x10E169) and FUN_00071DF4's 0x11042B/0x11042C table
  lookup; the other 10 row callers (0x70DD1, 0x7A219, 0x6FA62, 0x75DC4,
  0x77423, 0x7F4E7, 0x82A6C, 0x77DC6, 0x7EFCF, 0x7F0D1) stay row legs.
* **OL-T11-80 (palette pool identity / leg 11 carried)**: the loaded file
  that fills `[0x107290]` and the pool's rounded size (request 0x34E8,
  partition floor 0x3000).
* **OL-T11-81 (formation team-record producer)**: FUN_00011620's team block
  `0x143018+side*0x49` +0x12 source and the FUN_0007412C layout install.
* **OL-T11-82 (shade cube / leg 11)**: no static consumer of the
  `"inversetbl"` cube (re-verified); stays out of the match contract.

---

## 12. Port landing (T2 camera live feed / pan origin, 2026-10-09)

Phase-8 T2 (`docs/superpowers/plans/2026-10-09-fifa96-m2-phase8-live-loop.md`),
first-hand `/FIFA96.EXE` re-derivation of the §2.1(c)/§5.2 producer chain.

### 12.1 `fifa96_camera_event_set` completion (FUN_00071C94 + FUN_00070544)

| landed | native | notes |
|---|---|---|
| `[0x157A6C] != 0` bail gate | `0x71c99` | `fifa96_camera.event_suspended`; returns 1 with no reset |
| ramp sign param | FUN_00070544 EAX arg | `ramp_param != 0` -> `F6 = F2 + ramp(h - ty)` (row-04 arm A's ECX=1), else the NEG path |
| timer cell | `[0x1577FA]` = **F6** | S4 stored `timer = F8`; the first-hand disasm stores F8 at `[0x1577F8].lo` (the fast-path divisor) and F6 at `[0x1577FA]` (the timer). Corrected; `ramp_divisor`/`event_step_x/z` now carry the divisor and the stored 0x1577BA/BC step pair |
| fast/slow path | `0x70630..0x706d9` | fast (`F8 != 0 && [0x157821] == 0`) = `seed / F8`; slow = each nonzero velocity `* k / 0x20` (k = `[0x15781A]` height arm / `[0x15781B]` idle arm; FUN_000700F4 table 0x1104AB image defaults 10/16/8 staged on the reset) |
| > 0x19 atan walk | `0x70704..0x70798` | `fifa96_entity_angle` + `fifa96_entity_sine` folds + `FUN_000795A4` products cap the velocity magnitude to 0x19 in the current direction |
| tails | `0x70996..0x709c1` | event cursor/acc cleared; rate bytes 0x157816/17 zeroed (rate-819/81A/81B staged by the reset) |

### 12.2 Pan producers (FUN_000709D0 / FUN_00070DE0 / FUN_00071DF4)

* `fifa96_camera_pan_step` = FUN_000709D0: the height band (returned for the
  FUN_00065CF8 sink), the `[0x157821]` counter (cap 100), the
  `event_param * [0x157819] / 0x20` decay, the `([0x14C1D4]|[0x14C1D6]) & 4`
  random walk (atan direction + rng low-byte jitter; an absent rng is the
  staged default), then FUN_00070544(0) over the stored step pair.
  `fifa96_camera_update` now calls it in-line when `timer > timer_limit`
  (`0x737b9..0x737da`, signed word compare; the native call site).
* `fifa96_camera_reposition` = FUN_00070DE0 derived core: the
  FUN_00070074 flag compare (AND != 0 -> no-op), the FUN_0008DC50(delta, 1)
  walk from the previous triple toward the current one while the previous
  class holds, the mask bit effects (3 -> vel_x negate-quarter, 4 -> vel_z = 0,
  0x10 -> y jitter) and the bearing recompute. The bit-0x8 boundary arm
  (target z ±0xB11/±0xB0F, 0x8ED40/0x8F188 corner events) and the sound sinks
  are legs.
* `fifa96_camera_rate_table` = FUN_00071DF4 table/keeper arm (derived): the
  signed rate-byte and timer <= 0x1E gates, the caller-resolved rate write,
  and the keeper 0x1D/0x1E gate (event byte 2, height >= 0xC1, |anchor_x| <=
  0x23F, rate_z non-zero with the pos_z/vel_z sign cases). The 0x10E169 +
  0x11042B/0x11042C lookup is a leg (the caller stages the pair).

### 12.3 Live wire (first reachable row caller)

Row 04's ported body (`fifa96_outfield_row04_step`) already produced the
native 0x7EFCF/0x7F0D1 event outputs; `fifa96_match_action_04` now consumes
them: `out.events` -> `fifa96_camera_event_set(camera, event_x, event_z, 0,
event_track_reload)` (+ the arm-A `[0x1577FA] = h + h/4` tail). This is the
first *reachable gameplay-row* pan origin (the other ten FUN_00071C94 callers
are body legs). The armer chain then fires from the real row:
`test_engine_match_frame::test_row04_live_pan_arms_camera` installs a live
action-04 record, lets the frame dispatch produce the event (no fixture poke),
parks the camera at z 0xB00 and observes armed -> zone 1 -> queued situation 5
-> S3 score 1-0.

### 12.4 Errata (T2, first-hand)

1. **Timer cell (S4 §11.2).** FUN_00070544 stores the fast-path divisor at
   `[0x1577F8].lo` (=F8 = F4-F6) and the timer at `[0x1577FA]` (=F6); S4's
   "timer = F8" read the wrong word. The FU-152 §2.9 cell map's
   "`0x1577F8.hi` = timer" was right; the code was not.
2. **Slow path constants.** `[0x15781A]`/`[0x15781B]` are the slow-path
   velocity scale bytes (image 16/8), not event-rate words; they come from
   FUN_000700F4's 0x1104AB table. `[0x157819]` = 10 is the height decay rate.
3. **Pan-step call site.** FUN_000736AC calls FUN_000709D0 when
   `(short)[0x1577FA] > (short)[0x1577F4]` (`0x737b9..0x737da`, JG) before the
   same frame's integration; the engine port follows that order.
4. **`FUN_00070544` C6/F6 base.** `F6 = [0x1577F2] ± ramp([0x1577F0] -
   [0x157768])` — the decompiled `EDX = [0x1577F0] >> 16` is the high word
   = F2 (the dword alias at 0x1577F0/0x1577F2), not event_param. S4's
   `F6 = F2 - ramp(h - ty)` was already correct.
