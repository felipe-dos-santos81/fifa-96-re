# FU-69: in-match display/menu arms — the `0x4E570` overlay record, the pause latch and the video/event consumers

Roadmap slice S8 of FU-60 §6 ("in-match display/menu arms"), extended to the
FU-63 anchors `0x5B33C/340/344`, `0x5B330/334/338` and `[0x4E574]`. Derives
what the per-frame display arms of `FUN_000495B0` actually run, the 7-state
overlay record they (and the drain housekeeping) dispatch on, the pause latch
path from input record code 7 to its consumers, and the relation of these
paths to the already-ported video (`fifa96_vgt_player`/`fifa96_blit`) and
event (`fifa96_event_queue`) modules. Ports the clean display-record core as
`fifa96_match_display`.

Result in one line: **the display arms (`FUN_000495B0 0x495D1..0x4968B`) are
a display pre-step (`FUN_00053D58` load counter check → `FUN_0004CD70`),
the per-frame view/input update `FUN_0004D2D4` (camera clamps, input mapping
row `[0x7DEC]` from FU-61), a camera-snapshot refresh
(`[0x7314]=FUN_0004C904`, `[0x7318]=FUN_00043600`) and a three-way render
arm selected by `[0x677C]`/`[0x6780]` and `FUN_000541D4`: arm A
`FUN_00043330` (builds the widget list from per-mode tables, keyed by
`[0x4B018]`) + `FUN_00058BC0`, arm B `FUN_000432EC` + `FUN_00058D70` (its
no-menu branch calls `FUN_000638B0` → `FUN_00068108`, the FU-57 alternate
presenter), arm C `FUN_000432EC` + `FUN_00058B68` + `FUN_00049830` +
`FUN_000565BC` + `FUN_00039180(3)`; the same loop body runs the 7-state
overlay record at `0x4E570` through `FUN_00053A48` (draw, jump table flat
`0x539BC` keyed by `[0x4E578]`) and `FUN_00053BB8` (update, table flat
`0x53B9C`), whose state-1 gate accumulates `[0x7300]>>2` into the record
timer and force-completes when the pause latch or `[0x5FFC]∈{2,4}` is set;
`[0x4E574]` is field `+4` of that record (the frame-suspend flag whose
set/clear pair `FUN_00053DC4`/`FUN_00053DE0` brackets `FUN_00064074`/
`FUN_00064080` + `FUN_00045D0D`), read by `FUN_00051AB8` at `0x49FA4` (skip
the update chain) and by the overlay/menu sub-screens; the input record code
7 latch (`[0x67EC]=[0x67ED]=0x40`, one-shot getter `FUN_00045069` with the
`[0x67F4]=5` lockout) drives the overlay force-complete, a camera-control
reset (`FUN_0004FA36`), the transition manager (`FUN_000510DC`) and the
in-match video trigger `FUN_00063734` → `FUN_00067FE4` (the FU-55 player
open) — while the mode-7 pause menu is entered by match **event code 1**
through `FUN_00038004` (`0x3828E` → `0x3EABC`, screen `0x3DE`), not by the
code-7 latch directly (quoted errata below); the `0x5B33C/340/344` triple is
the last dispatched presentation event's entity param/count/time, written in
`FUN_00092548`'s success tail and read only by the entity tracker
`FUN_00072AC4` at `0x73665` to post `0xD8`/code `0x21`.**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58..FU-68). All
  instructions below were read back this slice
  (`disassemble_function`/`disassemble_bytes`/`read_memory`/
  `decompile_function`); the decompiler was used for callee characterisation
  where it succeeded. `disassemble_bytes` was used wherever
  `decompile_function` pruned blocks or died.
* **Decompiler failures met this slice (new errata).** `decompile_function
  0x53BB8` fails outright ("Error: Decompilation failed"); `FUN_00052030`,
  `FUN_00052D68` and `FUN_00053F90` decompile with "Control flow encountered
  bad instruction data" / "Exceeded maximum restarts" (embedded fixup jump
  tables). Their bodies below are quoted from `disassemble_function`/
  `disassemble_bytes` listings, not the decompiler.
* **Address convention (FU-59/FU-66, quoted).** Code is object 1 (base
  `0x10000`), data is object 4 (base `0x100000`); instruction operands are
  stored pre-relocation. The tables cited here are embedded in the `0x53xxx`/
  `0x51xxx`/`0x37Fxx` code object: encoded `0x439BC` → flat `0x539BC`,
  `0x43B9C` → `0x53B9C`, `0x27F90` → `0x37F90`, `0x27FC8` → `0x37FC8`,
  `0x415A8` → `0x515A8`, `0x419C8` → `0x519C8`. Raw `read_memory` of the
  encoded addresses returns code/filler bytes; the flat reads return the
  tables (quoted below).
* **Exhaustive caller scans.** Several display/overlay functions live in the
  undefined `0x51xxx`/`0x53xxx` gaps and have no Ghidra xrefs
  (`get_xrefs_to 0x515C8`/`0x5154C`/`0x51CD8` = 0). Call sites were recovered
  with a full-image `E8 rel32` scan (`run_script_inline`, analysis-only, not
  committed), the FU-64/FU-68 method.

## 1. The display arms of the match loop `FUN_000495B0`

### 1.1 Listing (`0x495D1..0x4968B`, `disassemble_bytes 0x495C0`)

```
0x495D1  CALL 0x53D58                  ; load/transition counter check
0x495D6  TEST EAX,EAX / JZ 0x495DF
0x495DA  CALL 0x4CD70                  ; display pre-step (only when 53D58 != 0)
0x495DF  CALL 0x4D2D4                  ; per-frame view/input update
0x495E4  CALL 0x4C904 / 0x495E9 MOV [0x7314],EAX   ; camera snapshot ptr
0x495EE  CALL 0x43600 / 0x495F3 MOV [0x7318],EAX   ; display object ptr (&DAT_0004B0A4)
0x495F8  MOV EAX,[0x7314] / MOV EAX,[EAX+0x14] / 0x49600 CALL 0x43E30
0x49605  CALL 0x439D0
0x4960A  CALL 0x4382C                  ; [0x677C]
0x4960F  TEST EAX,EAX / JZ 0x4963C
0x49613  CALL 0x6400C                  ; [0x9A98]==0 (FU-64 §3)
0x49618  TEST EAX,EAX / JZ 0x4963C
0x4961C  CALL 0x541D4                  ; menu-record gate
0x49621  TEST EAX,EAX / JZ 0x4963C
0x49625  CALL 0x43330                  ; arm A: widget list from [0x4B018]
0x4962A  MOV EDX,[0x7314] / 0x49630 MOV EAX,[0x7318] / 0x49635 CALL 0x58BC0
0x4963A  JMP 0x49690
0x4963C  CALL 0x43608                  ; [0x6780]
0x49641  TEST EAX,EAX / JZ 0x4965C
0x49645  CALL 0x432EC
0x4964A  MOV EDX,[0x7314] / 0x49650 MOV EAX,[0x7318] / 0x49655 CALL 0x58D70
0x4965A  JMP 0x49690
0x4965C  CALL 0x432EC                  ; arm C (default)
0x4966C  CALL 0x58B68
0x4967C  CALL 0x49830                  ; transform interpolation (FU-60 §2.3)
0x49681  CALL 0x565BC
0x49686  MOV EAX,3 / 0x4968B CALL 0x39180
0x49690  CALL 0x43B48                  ; per-frame body head (FU-60 §2.2)
```

The arm condition is therefore
`arm A := [0x677C]!=0 && [0x9A98]==0 && FUN_000541D4()!=0`, else
`arm B := [0x6780]!=0`, else arm C. This confirms FU-60 §6 S8's "three arms
selected by `FUN_0004382C`/`0x43608`/`0x541D4`" exactly; the first two are
plain getters (`FUN_0004382C` = `return [0x677C]`,
`FUN_00043608` = `return [0x6780]`, decompiles), and `FUN_000541D4` =
`return !([0x4E5C8]!=0 && [0x4E674]!=0 && [0x4E688]==0)` (decompile).

Selectors: `[0x677C]` values 1/3 and `[0x6780]` are written by the `0x43xxx`
display module (`FUN_000436E4 0x436F2/0x43701`, `FUN_0004372C
0x43744/0x437D6/0x4381B`, `0x438E5`, `FUN_000443E8 0x444DE`, `0x44DD0`,
`0x44E0F`, `FUN_00044E3C 0x44EF0`, `FUN_00044F24 0x44F46`; all
`search_instructions operand 677c/6780`). Arm A's `FUN_00043330` ends in
`FUN_00044F24` (`0x43557`), and `FUN_00044F24 0x44F46` writes `[0x677C]` —
so arm A re-stages the display selector.

### 1.2 Callee characterisation (evidenced)

| callee | evidence | behaviour (instructions only) |
|---|---|---|
| `FUN_00053D58` | decompile | `return ([0x4E58C]-0xF1) < 0x78 && [0x4E58C]!=0`; `[0x4E58C]` is the same counter `FUN_000510DC` advances to `0x169` |
| `FUN_0004CD70` | decompile | calls `FUN_0004D488()`; writes `[0x7DF0]` (decompiler register artifact for the second write) |
| `FUN_0004D2D4` | decompile | view record `[0x7DC8]` update: camera clamps `±0x720` (index 0xD) / `±0xB10` (index 0xF), `FUN_000365B0`/`FUN_00036544`, `FUN_000505D0`/`FUN_0004F8C8`/`FUN_0004D908` per view class, `FUN_00062B34`/`FUN_00062B20`, and `[0x7DEC] = FUN_0004CA70(...)` — the input mapping row of FU-61 §2.3 |
| `FUN_0004C904` → `[0x7314]` | FU-64 §2.1 (quoted) | copies 6 dwords from `[0x7DC8]+0x10/+0x14/+0x18/+0x58/+0x5C/+0x4C` to `0x4E4E0`, returns `&0x4E4E0` |
| `FUN_00043600` → `[0x7318]` | FU-64 §2.1 (quoted) | `return &DAT_0004B0A4` |
| `FUN_00043E30` | decompile | `FUN_000441D8(arg, [0x4B0D4])` |
| `FUN_000439D0` | decompile | allocates the screen buffer via `FUN_0009A45C([0x6708],[0x670C],0)`, `FUN_000CE6F0`, `FUN_000CE7BC([0x4B0A4],[0x4B0AC],[0x4B0A8],[0x4B0B0])`, sets `[0x6778]=1` |
| `FUN_00043330` | decompile | builds `PTR_DAT_0004AC38` list entries from per-mode/team tables (`0x6650`,`0x6510`,`0x6390`,`0x63B0`,`0x63D0`,`0x63F0`,`0x6430`,`0x6410`) selected by `[0x4B018]` (0..0x1C) and the player lookup; appends two constant entries `0x88DB31E3`/`0x88CC04D8`; calls `FUN_00044F24(list)` |
| `FUN_00058BC0` | decompile + callees | common `FUN_00058A44`; `FUN_00062B40`/`FUN_0006331C`/`FUN_0005FEAC`/`FUN_000610C4`/`FUN_00062620`/`FUN_00062588`/`FUN_00056FA4`/`FUN_00058AC4` draw pipeline; `FUN_00060344`/`FUN_000603A8`/`FUN_000603EC`/`FUN_00060018`; `FUN_00043568`/`FUN_000435C0`/`FUN_000435D0` display gates; `FUN_000436E4`/`FUN_0004372C`; `FUN_00039180`; `FUN_000565BC`; `FUN_0008EB70`/`FUN_0008EB78` |
| `FUN_00058D70` | decompile + callees | `FUN_00058A44`; if `[0x9094]==0 && [0x6780]!=1` → `FUN_00043610`, `FUN_000638B0`, `FUN_00043618`; else the `FUN_00062B40`/`FUN_0005FEAC`/`FUN_000610C4`/`FUN_00058AC4`/`FUN_00058538` pipeline |
| `FUN_00058B68` | decompile | `FUN_00058A44`, then the same draw pipeline without the `FUN_000638B0` branch |
| `FUN_000638B0` | FU-57 §1 (quoted) | caller of `FUN_00068108` (alternate presentation host: poll → palette translation → quad rasterizer) |
| `FUN_000565BC` | decompile | gated by `[0x4E538]`, `FUN_00037AE4` (menu), `FUN_0006400C` (`[0x9A98]==0`) and phase ∉ {0xC,0x13,0x14}: `FUN_000560C8` + `FUN_0005619C`×2; `FUN_00063FD0`/`FUN_00063FDC` and `FUN_00053D58` select `func_0x00064C34` (`[0x4E58C]==0`) vs `func_0x000564A0`; `[0x4E688]`/`[0x4E674]&0x80`/`[0x4E510]`/`[0x4E59C]`/`[0x4E660]` arms call the undefined `0x542D4`/`0x550E4`/`0x55C24`/`0x56518` |
| `FUN_00039180` | disassembly/decompile (bad-data warnings) | tests `[0x5FFC]` through `FUN_00036BC8` and switches on `[0x4B018]`; called with `EAX=3` after arm C |

So the display arms do **not** call the Mode-X blitter or the VGT player
directly (no `FUN_000AE7F0`/`FUN_00067FE4` edge in any of the three arm
bodies); they run the HUD/widget pipeline (`0x5Fxxx`/`0x60xxx`/`0x62xxx`
draw calls) and, in arm B's no-menu branch, the FU-57 alternate VGA
presenter `FUN_00068108` through `FUN_000638B0`.

## 2. The `0x4E570` display/overlay record

### 2.1 Layout (all offsets evidenced by the two state machines and the reset)

| offset | address | writer(s) | reader(s) | role (instructions only) |
|---|---|---|---|---|
| `+0x00` | `0x4E570` | `FUN_000537F8` zeroes | — | reset-only dword |
| `+0x04` | `0x4E574` | `FUN_00053DC4=1`, `FUN_00053DE0=0`, `FUN_00052030 0x520C8`, `FUN_000537F8 0x5381A/0x53888`, `0x51435`, `0x524D0`, `0x5296F`, `0x52BC7`, `0x52C3E`, `0x52E12`, `FUN_00053A8E 0x53AA2` | `FUN_00051AB8`, `0x5141B`, `0x51485`, `0x51535`, `0x51805`, `0x51A49`, `FUN_00052030 0x520B4/0x5206D`, `0x524C0`, `0x5295F`, `0x52BB7`, `FUN_00052030 0x52C2E`, `FUN_00052D68 0x52DBA`, `0x52E02`, `FUN_00053BB8 0x53CA2`, `FUN_00053A48 0x53A95`, `FUN_00053DC4/0x53DE0` | frame-suspend flag (`get_xrefs_to 0x4E574`, 32 refs) |
| `+0x08` | `0x4E578` | `FUN_000537F8 0x53820`, `0x51421` | `FUN_00053D9C` | overlay state 0..6 (tables below) |
| `+0x0C` | `0x4E57C` | `FUN_000537F8 0x53826/0x5387D`, `0x514A3`, `0x51819`, `0x518F3`, `0x519A3`, `0x51AD0`, `0x51B2D`, `0x51B56` | `FUN_00053D50` | overlay value/selector byte (0x10, 0x14, 0x1C/0x1D, 0x1F, phase) |
| `+0x10` | `0x4E580` | `FUN_000537F8=0`, `FUN_00053BB8 0x53BE1` (`+= [0x7300]>>2`), `0x53C17`, `0x51CC0` | `FUN_00053BB8 0x53C02` | overlay timer (`FUN_0004937C` = `[0x7300]>>2` per call) |
| `+0x14` | `0x4E584` | `FUN_000537F8=0x50`, `FUN_00053BB8 0x53C11`, `0x51CC0` | `FUN_00053BB8 0x53BE4/0x53C02` | overlay duration/deadline |
| `+0x18` | `0x4E588` | `FUN_000537F8 0x5383D/0x53882` only | `FUN_00053BB8 0x53CC8` | saved phase (or `0xC` when phase `0x10`) |
| `+0x38/+0x3C` | `0x4E5A8/A8+C` | `FUN_000537F8` zeroes | `FUN_00053BB8 0x53C1D..0x53C29`, helpers `0x51D10` | callback pointer + argument, stage 1 |
| `+0x40/+0x44` | `0x4E5B0/B4` | zeroed | `FUN_00053BB8 0x53C47..0x53C53`, helper `0x51D24` | callback pointer + argument, stage 2 |
| `+0x48/+0x4C` | `0x4E5B8/BC` | zeroed | `FUN_00053BB8 0x53C7F..0x53C8C`, helper `0x51D3C` | callback pointer + argument, stage 3 |
| `+0x50/+0x54` | `0x4E5C0/C4` | zeroed | `FUN_00053BB8 0x53D14..0x53D20`, helper `0x51D54` | callback pointer + argument, stage 4 |
| `+0x58` | `0x4E5C8` | `FUN_000537F8=0` | `FUN_000541D4`, `FUN_00053E08`, `FUN_00053BB8` | sub-screen record pointer |
| `+0x104` | `0x4E674` | `FUN_000537F8=0` | `FUN_000541D4`, `FUN_00052030`, `FUN_00052D68` | sub-screen record pointer / `0x8000` enable bit |
| `+0x1C` | `0x4E58C` | `FUN_000537F8=0` | `FUN_000510DC`, `FUN_00053D58`, `FUN_000565BC` | transition/load counter (0..0x169) |
| `+0x5C..+0x80` | `0x4E5CC..0x4E5F0` | `FUN_000537F8` | `FUN_000510DC` | transition-manager state (`[0x8E0C]/[0x8E10]` peers) |
| `+0xF0` | `0x4E660` | `FUN_000537F8=0` | `FUN_000565BC` | display flag |

The four stage-callback helpers `0x51D10`/`0x51D24`/`0x51D3C`/`0x51D54` are
undefined functions (`get_function_by_address` = none) with the shape
`if (record[+slot]!=0) { EAX = record[+slot+4]; CALL record[+slot]; }` and
return 1 when the slot is null (`0x51D35`, `0x51D4D`, `0x51D65`
`MOV EAX,1`).

### 2.2 Reset `FUN_000537F8` (`0x537F8..0x5392E`, decompile)

```
[0x4E570]=0; [0x4E574]=0; [0x4E578]=0; [0x4E57C]=0; [0x4E580]=0;
[0x4E584]=0x50;
phase = FUN_0004B380(); [0x4E588]=phase;
[0x4E58C]=[0x4E590]=[0x4E594]=[0x4E598]=[0x4E59C]=[0x4E5A0]=[0x4E5A4]=0;
copy 8 dwords 0x8C3C -> [0x4E5A8] (the four callback slots, zeroed);
if (phase == 0x10) { [0x4E588]=0xC; [0x4E574]=1; [0x4E57C]=0x10; }
[0x4E674]=0; [0x4E678]=0; [0x4E67C]=0; [0x4E680]=300; [0x4E684]=0;
[0x4E688]=0; [0x4E698]=0; [0x4E5C8]=0; [0x4E5CC]=0; [0x4E5D0]=0;
[0x4E5D4]=180000; [0x4E5D8]=0; [0x4E5DC]=0; [0x4E5EC]=0; [0x4E510]=0;
[0x4E530]=0; [0x4E534]=0; [0x4E660]=0; [0x4E69C]=-1; [0x4E5F0]=1;
[0x4E538]=1; [0x8E04]=-1;
```

Sole static caller (`E8` scan): `0x4A272` inside the match reset
`FUN_0004A228` — the same reset that calls `FUN_00091BC4` (FU-49 §1.10) and
that the setup `0x493A0` runs at `0x49430` (FU-60 §2.1). The phase-`0x10` arm is the
same phase FU-62 §1.4/§5 flagged for the entity pass (`FUN_00088940`); it
starts the record suspended with value `0x10`.

### 2.3 Draw dispatcher `FUN_00053A48` (`0x53A48..0x53B9B`)

```
0x53A48  prologue (EBX/ECX/EDX/ESI/EDI/EBP); EBX=0x4E570; EBP=0x4E674;
         EDI=0x8000; ESI=0
0x53A5F  EAX=[EBX+8] (state); CMP EAX,6 / JA epilogue
0x53A6B  JMP dword CS:[EAX*4 + 0x439BC]      ; flat table 0x539BC
0x53A73  state 0: CALL 0x36C0C; if !=0 { EAX=EBX; CALL 0x52030 }
0x53A8E  state 5: [EBX+8]=6; if [0x4E574]!=0 { FUN_00064080(); [0x4E574]=0;
         FUN_00045D0D(); }  ... [0x4E674] bitfield/record teardown
         (FUN_00053E2C/FUN_00053E08/FUN_00053FF4) ...
0x53B6F  EAX=0x4E570; CALL 0x52D68; if EAX==1 { [EBX+8]=0 }
0x53AAD  state 6: same [0x4E674] block, FUN_00052D68, state=0
0x53B95  epilogue (shared with the update machine)
```

Table flat `0x539BC` (`read_memory 0x539BC`, encoded values +0x10000):
`{0x53A73, 0x53B95, 0x53B95, 0x53B95, 0x53B95, 0x53A8E, 0x53AAD, 0x53AE5}`.
With the `<7` bound, states 0→`0x53A73`, 1..4→epilogue, 5→`0x53A8E`,
6→`0x53AAD`; entry 7 (`0x53AE5`, the `[0x4E5C8]` arm of the state-6 block) is
not dispatched by this machine.

`FUN_00053A48` is called from the match loop `0x49523` and the drain
`0x4A045` (`E8` scan) — i.e. once per outer-loop iteration / drained frame,
after `FUN_0004B100`/`FUN_00091DD8`.

### 2.4 Update dispatcher `FUN_00053BB8` (`0x53BB8..0x53D4F`)

```
0x53BB8  prologue; EDX=0x4E570; EDI=EDX; ESI=1; ECX=0
0x53BCC  EAX=[EDX+8]; CMP EAX,6 / JA 0x53B95
0x53BD4  JMP dword CS:[EAX*4 + 0x43B9C]      ; flat table 0x53B9C
0x53BDC  state 1: CALL 0x4937C; ADD [EDX+0x10],EAX   ; timer += [0x7300]>>2
         EBX=[EDX+0x14]                              ; duration
         CALL 0x45069                                ; pause one-shot latch
         if (EAX!=0 || FUN_00036C3C()!=0) { CALL 0x51CC0; EAX=1 }
         else EAX = ([EDX+0x14] <= [EDX+0x10])       ; SETLE
         if (!EAX) -> epilogue
         [0x4E584]=0; [0x4E580]=0                    ; zero duration then timer
         slot +0x38: if null EAX=1 else { EAX=[+0x3C]; CALL [+0x38] }
         if (!EAX) -> epilogue
         [EDX+8]=2; re-dispatch
0x53C47  state 2: slot +0x40 (null -> EAX=1); advance -> [EDX+8]=3
0x53C7D  state 3: [EDX+0x28]=0; slot +0x48; if [0x4E574]!=0 CALL 0x36C70;
         advance -> [EDX+8]=4
0x53CC8  state 4: selector [EDX+0x18]:
           ==0xC  -> CALL 0x566D0; EAX = SETZ(AL&1) + 0x1C; [EDX+0xC]=EAX;
                     period=FUN_0004B5F4(); if period<=1 [0x5FFC]=2
           ==0x13 -> [0x5FFC]=4
           else   -> [EDX+0xC]=FUN_0004B380() (phase)
         slot +0x50; advance -> [EDX+8]=5; CALL 0x4CEF4
0x53D4F  end (state 5/6 -> epilogue)
```

Table flat `0x53B9C` (`read_memory 0x53B9C`): `{0x53B95, 0x53BDC, 0x53C47,
0x53C7D, 0x53CC8, 0x53B95, 0x53B95}` — state 0 and 5/6 are no-ops in the
updater. The state writes are `1→2` (`0x53C2C`), `2→3` (`0x53C62`), `3→4`
(`0x53C9B`), `4→5` (`0x53D2F`); no handler writes state 1 (the only writer of
`[0x4E578]` besides the reset is the undefined `0x51421` in the overlay
starter `FUN_000513EC`).

`FUN_00053BB8` is called from the match loop's per-frame body at `0x49720`
(after the `FUN_00037AE4` menu gate and `FUN_0006400C` load gate) and
`0x4976B` (after `FUN_00053D7C`; both `disassemble_bytes 0x496C0` `E8` scan).
So the overlay update runs only while neither the menu nor a load is active.

### 2.5 The suspend pair `[0x4E574]`

```
FUN_00053DC4: if ([0x4E574]==0) { FUN_00064074(); [0x4E574]=1; }   ; 0x53DC5..0x53DD8
FUN_00053DE0: if ([0x4E574]!=0) { FUN_00064080(); [0x4E574]=0;
                                  FUN_00045D0D(); }                 ; 0x53DE1..0x53DF7
FUN_00064074 = [0x9A90]=0 ; FUN_00064080 = [0x9A90]=1               ; decompiles
FUN_00051AB8 = return [0x4E574]                                     ; 0x51AB8
```

Set sites (`E8` scan): `0x37F83` (`FUN_00037F54`, match-display arm),
`0x4B1D7`/`0x4B257` (`FUN_0004B100`, world update), `0x4C324`. Clear sites:
`0x389B9` (match-end `FUN_00038988` region), `0x494FE` (the match loop's
`[0x5FFC]==2` resolution arm), and the overlay transitions
`FUN_00052030 0x520C8`, `FUN_00052D68 0x52DCA`, `FUN_00053A8E 0x53AA2`.
Additional unnamed writers/readers are in the `0x51xxx`/`0x52xxx` overlay
sub-screens (table `0x4E574` xrefs, §2.1).

Evidenced role: a **match-frame suspend flag** used by the overlay/cutscene
machinery (goal celebrations, period/match transitions, replays): while it is
nonzero `FUN_00049B28` skips `FUN_0004C394`/`FUN_0004B100`/`FUN_00036208`/
`FUN_00091DD8` (`0x49FA4`; FU-60 §3), `FUN_0004511D` returns 0 for every
player (`0x4512E`; FU-61 §5), `FUN_00045D0D` takes its `[0x6816]` hold path
(FU-61 §4.3), the update machine skips `FUN_00036C70` while state 3 runs
(`0x53CA2`), and the display dispatcher's state-5 arm clears the flag (and
runs `FUN_00045D0D`) itself (`0x53A8E`).

## 3. The pause latch (input record code 7) and the menu/video consumers

### 3.1 Latch write and aggregation

`FUN_00046246`'s code-7 arm (FU-61 §4.2, re-verified with
`search_instructions operand 67ed/67ec`):

```
0x46299  [0x67ED] = 0x40
0x462AE  AL = [0x67ED]
0x462BE  [0x67ED] &= 0          ; zeroed when [0x67EC] was already nonzero
0x462C5  [0x67EC] = 0x40
```

`FUN_00045F6A` then aggregates the per-player current states and rising edges
(`0x46137 [0x67ED] |= edge`, `0x46146 [0x67EC] |= held`, `0x4614E`, `0x46159`)
and latches them (`0x4615E [0x67F3] |= [0x67ED]`, plus `[0x67F2] |=
[0x67EC]`; FU-61 §4.1/§4.3), and `FUN_00046177` clears `[0x67F3]`/`[0x67F2]`
and decrements the lockout `[0x67F4]` (FU-61 §4.3). The pause getters:

| getter | disassembly | semantics |
|---|---|---|
| `FUN_00045001` | `0x4500F MOV AL,[0x67EC]; AND AL,0xF0` | held high nibble (pause bit `0x40`) |
| `FUN_00045025` | `0x45033 MOV AL,[0x67EC]` | held byte |
| `FUN_00045047` | `0x45055 MOV AL,[0x67ED]` | fresh byte |
| `FUN_00045069` | `0x45077 CMP [0x67F4],0 / JZ; 0x45086 AL=[0x67F3]&0xF0; if !=0 [0x67F4]=5` | one-shot: first call after a latch returns the high nibble and arms a 5-drain lockout; returns 0 while `[0x67F4]!=0` |
| `FUN_000450D9` | `0x450E7` same gate, returns the full `[0x67F3]` | one-shot full byte |

### 3.2 Consumers (full-image `E8` scan)

`FUN_00045069` (pause one-shot) is consumed by:

| site | enclosing | effect (evidenced) |
|---|---|---|
| `0x4FA4A` | `FUN_0004FA36` | if `[0x7DD8]==0` and the latch fires and `FUN_00053D50()` (`[0x4E57C]`) != 0x14: zero `[0x7DD8]/[0x7DE0]/[0x7DE4]/[0x7DE8]`, return 1 (view/camera control reset) |
| `0x510F4` | `FUN_000510DC` | sets `[0x8E0C] = -1` (transition-manager restart) |
| `0x6373E` | `FUN_00063734` | if `FUN_00056698()` (`[0x4E59C]`) == 0: `FUN_000492D4`, `FUN_000680F8`, `FUN_000543D4`, `FUN_00054028`, `[0x97FC]=0`; then if `[0x9804]!=0` picks a video id `[0x980C]` from tables `0x9A20`/`0x9A50` (keyed by `[0x9808]`, or `FUN_0004C2E0` codes `0x30`/`0x2F`/`0x19`/`0x31` for state 8, or `FUN_00063688` for state 7), then `FUN_00099E9F`/`FUN_00078F00`/`FUN_000543A0`/`FUN_0008F178`/`FUN_00067FE4` |
| `0x51559`, `0x5167D`, `0x516EA`, `0x5175E`, `0x517BF` | the undefined `0x51xxx` overlay sub-screens (state table flat `0x515A8`, §3.3) | pause during the overlay forces `[0x4E584]/[0x4E580]=0` (`0x51CEA`) and returns 1, or resets `[0x4E57C]=0x14/0x1F` and calls `FUN_00054238` |
| `0x51CD8` | undefined function (`0x51CD4..0x51D0D`) | if latch or `FUN_00036C3C()`: zero `[0x4E584]/[0x4E580]`, return 1 (`0x51CEA..0x51CFD`); else `duration <= timer` (`0x51D00`) |
| `0x53BE7` | `FUN_00053BB8` state 1 | force-completes the overlay transition (`0x51CC0` zeroes timer+duration) |

`FUN_00045001` (held high nibble) is consumed by `FUN_0008BAF0` (`0x8BC68`,
`0x8BCA3`, `0x8BD61`, `0x8BDB3` — the match-end/period-end machine, FU-64
§4.2), `0x8B80A`, `0x8495F`, `0x84A80` (commentary/match-event module);
`FUN_00045025` only by `FUN_00015124 0x1529A` and `FUN_000642FC 0x6438F`
(the load manager); `FUN_00045047` has no consumer. So the record-code-7
latch is read as "the player pressed the menu/advance button": it
force-completes overlays, resets camera control and arms the in-match video
trigger — it is not itself the pause-menu key edge in the static code.

### 3.3 The `0x51xxx` overlay sub-screens (context, not fully derived)

The undefined function at `0x515C8` dispatches on `[0x8E24]` (0..7) through
flat table `0x515A8` (`read_memory 0x515A8` = `{0x515FC, 0x51650, 0x516BE,
0x5172B, 0x5179F, 0x517AF, 0x51805, 0x51875}`); it has **no static caller**
(`E8` scan) — it is reached through fixup-populated callback slots or a jump
table outside the scan. Its states accumulate `[0x8E14] += [0x7300]>>2`
(`0x515D3..0x515E5`), gate on `[0x4E674]`/`[0x4E5C8]` sub-screen records via
`FUN_00053E08`, and use `FUN_00045069` for the pause one-shot. State 6
(`0x51805`) with `[0x4E574]!=0` sets `[0x4E57C]=0x1F`, zeroes
`[0x8E14]/[0x8E18]`, calls `FUN_0004FD50` and tears the sub-screen records
down; state 7 (`0x51875`) calls `FUN_0004FF0C`/`FUN_0004FF24` (input) and
`FUN_00051270`, which resets the event queue (`FUN_0008F178 0x51273`) or
runs the video path (`FUN_00063734`), then sets `[0x8E24]=0` and returns 1.
The sibling `0x519D8` machine dispatches `[0x8E30]` (0..3) through flat
`0x519C8` and walks the same records. None of this family has a static
caller; screen identities are not asserted (open legs).

### 3.4 The mode-7 pause menu entry (quoted discrepancy)

`FUN_00038004` (the event dispatcher called from the drain, FU-60 §3) ends:

```
0x38272  MOV EAX,0x19; DEC EBX; CALL 0x65CC0
0x3827D  CMP EBX,0xE / JA 0x3843A
0x38286  JMP dword CS:[EBX*4 + 0x27FC8]     ; flat table 0x37FC8, index = code-1
0x3828E  CALL 0x3EABC                       ; code 1 case
0x38293  CALL 0x45D0D
```

Table flat `0x37FC8` (`read_memory 0x37FC8`, 15 entries) maps action codes
1..0xF to `0x3828E` (code 1), `0x3843A` (2), `0x38424` (3), `0x382CF` (4:
`[0x4B018]=2`), `0x382E9` (5: `[0x4B018]=5`), `0x38303` (6), `0x38403` (7),
`0x383D2` (8), `0x3829E` (9: `[0x4B018]=1`), `0x382B8` (0xA:
`CALL 0x396E4(3,0)`), `0x383AE` (0xB), `0x38360` (0xC), `0x3837A` (0xD),
`0x38394` (0xE), `0x3831D` (0xF). The same function's earlier table flat
`0x37F90` (14 entries, `[0x4B018]` 0..0xD) is the per-mode display action
set.

`0x3EABC..0x3EB4D` (undefined thin function, two callers: `0x3828E` and
`0x38F11`) is the **pause-menu entry**:

```
0x3EABF  [0x4B018] = 7
0x3EAC9  CALL 0x1B128                      ; FUN_000CB8CF + FUN_0006864C
0x3EACE  EAX = 0x3DE / 0x3EAD3 CALL 0x3E9E0 ; screen setup id 0x3DE
0x3EAD8  EDX=0 / EAX=0x6C00 / EBX=6 / CALL 0x195D8
0x3EAE9  CALL 0x543A0
0x3EAEF  EAX=0 / EDX=1 / CALL 0x15864
0x3EAFA  CALL 0x12280
0x3EAFF  if ([0x4AFD0]!=0) EDX=0x14 else EDX=0xA
0x3EB14  PUSH 8; PUSH 0x47DE8; PUSH 0xA; EAX=0; ECX=3; CALL 0x37748
0x3EB2B  EAX=0x69 / CALL 0x12ECC
0x3EB35  CALL 0x122E0
0x3EB3A  ECX=0xF / CALL 0x45D0D
0x3EB44  [0x4AF5C] = 0xF
```

Mode 7 makes `FUN_00037B24()` true, so `FUN_00049B28` takes the cleanup path
(`0x49B2B CALL 0x37B24; TEST; JNZ cleanup`, FU-60 §3) and `FUN_00045D0D`
holds the 30 Hz pace (FU-61 §4.3); the menu screen itself (id `0x3DE`) is set
up by the front-end/screen module `FUN_0003E9E0` and is not decomposed here
(open leg). The match-loop display arms keep running under mode 7 (their
`FUN_0006400C`/`FUN_00037AE4` gates apply per arm).

**This is the quoted discrepancy with the slice's premise:** the input
record code 7 is not statically connected to `0x3EABC`; the pause menu is
dispatched by match **event code 1** (`FUN_00038004 0x3828E`), i.e. the event
array `0x4B1DA[1]` set by input record code 1 with payload `ev=1`
(FU-61 §4.2). The record-code-7 latch instead feeds §3.2. Whether one
physical key emits both encodings is an input-binding/record-layer question
(FU-61 open legs) and is not decided here.

## 4. The `0x5B33C/340/344` triple and the `0x5B330/334/338` queue (FU-63 re-verified)

The presentation queue is the six-slot `0x5B380` array with count `[0x5B334]`
and index `[0x5B338]`, fed by `FUN_0008F0C4` and drained one record per pump
tick by `FUN_00092548`; the 3-state machine runs at `[0x5B330]` (FU-63
§§3–5). Re-verified xrefs this slice:

* `[0x5B330]`: written only by `FUN_00091BC4 0x91CAB`, `FUN_00091DD8
  0x923E9` (state increment), `FUN_00092548 0x927B3` (success tail resets it
  to 0); read at `0x921ED`.
* `[0x5B334]`/`[0x5B338]`: writers/reset `FUN_0008F178 0x8F17B`, the pump
  (`0x9221F/0x92274/0x9231F/0x923B8/0x924BB` and the `0x5B338` peers),
  `FUN_0008F0C4 0x8F13C`, `FUN_00091BC4 0x91CB7` (+ `0x5B338`); readers are
  the enqueue/drain bounds.
* `[0x5B33C]`: written at `0x926B3` (id `0xDC` arm) and `0x92787` (generic
  `value!=0` arm) with `record[+0x18]`, gated by `[0x5B37C] ==
  record[+0x18]` (`0x926A5`, `0x92774..0x9277E`); initialised at `0x91CBD`;
  read only by the getter `FUN_00092194` (`MOV EAX,[0x5B33C]; RET`).
* `[0x5B340]`: incremented `0x926B9`/`0x9278D`, initialised `0x91CE2`, no
  other reader. `[0x5B344]`: written `0x92798` (`CALL now` →
  `[0x5B344]=now`), sole xref.
* Success tail `0x9279D..0x927C7`: `record[+0x14].+8=1`, `[0x5B330]=0`,
  per-id counter `[0x5AFB8+id*4]++`, per-id cooldown `[0x5AC38+id*4]=now`.

Consumer: `FUN_00072AC4 0x73665 CALL 0x92194` (sole caller of the getter),
`CMP EAX,[0x57A83] / JZ done`; when the tracked entity differs,
`0x73674..0x73680 EAX=0xD8, EBX=0x21, EDX=[0x57A83], CALL 0x8F188` — the
command-ring enqueue that sets `[0x5B374]=1` (FU-63 §4.4/§6), which the next
pump state 0 turns into command threshold `0x20` (the "new tracked entity"
presentation pass). The display/event drive is therefore: **a dispatched
presentation event records its entity param and to, and the tracker uses the
param to post exactly one `0xD8`/`0x21` notification per newly tracked
entity**.

## 5. Relation to the ported video and event modules

| ported module | original chain | evidence |
|---|---|---|
| `fifa96_event_queue` | the `0x5B380` six-slot queue, `[0x5B330]` 3-state machine, `FUN_0008F0C4`/`0x8F178`/`0x8F188`/`0x8F2C4` (FU-63) | §4; FU-63 §8 |
| `fifa96_input` | device/mapping/latch core incl. `0x67EC`/`0x67ED` aggregation and `0x45155`/`0x4511D` getters (FU-61) | §3; FU-61 §§2–5 |
| `fifa96_vgt_player` | FU-55 TGV/VGT stream walk over stream `FUN_00067FE4` / host `FUN_00068194` | `FUN_00067FE4` callees `FUN_000679F4`, `FUN_00098C38`, `FUN_000993EC`, `FUN_000A1764`; FU-55 §1/§7 |
| `fifa96_blit` | Mode-X blitter `FUN_000AE7F0`, sole caller `FUN_00068194` (FU-56 §1) | FU-55/FU-56 |
| display arms (§1) | HUD/widget pipeline `0x5Fxxx`/`0x60xxx`/`0x62xxx` + arm B `FUN_000638B0` → `FUN_00068108` (FU-57 alternate presenter) | §1.2 |

The only static edge from the match-menu/display code into the ported video
chain found this slice is the pause-latch consumer `FUN_00063734`, which
calls `FUN_00067FE4` (open the VGT stream, §3.2); the movie host/blitter loop
then runs `FUN_00068194` → `FUN_000AE7F0` (FU-57 §1/§2). The display arms
themselves never call `FUN_000AE7F0` or `FUN_00067FE4`.

## 6. Port: `fifa96_match_display`

`include/fifa96_loader/fifa96_match_display.h` +
`src/fifa96_loader/fifa96_match_display.c` (caller-owned struct, no globals,
no comments, `-fifa96_err_t` for invalid arguments). Scope: the evidenced
record fields and the state-1 gate — `FUN_000537F8`'s write set, the
`FUN_00053DC4`/`FUN_00053DE0` suspend pair, and the `FUN_00053BB8` state-1
timer block.

| original | port |
|---|---|
| `FUN_000537F8 0x537F8..0x53888` zero/duration write set | `fifa96_match_display_init` (`duration = FIFA96_MATCH_DISPLAY_DURATION = 0x50`, `selector = phase`) |
| `0x5387D..0x53888` phase-`0x10` arm (`selector=0xC`, `suspend=1`, `value=0x10`) | `init` arm |
| `FUN_00053DC4` (`if !suspend { FUN_00064074(); suspend=1 }`) | `fifa96_match_display_suspend_enter` (enter callback before the flag; edge-triggered; callback error aborts) |
| `FUN_00053DE0` (`if suspend { FUN_00064080(); suspend=0; FUN_00045D0D() }`) | `fifa96_match_display_suspend_leave` (leave, clear, refresh; first negative error propagated after the sequence) |
| `0x53BDC..0x53C0D` state-1 gate (`state==1`; `timer += FUN_0004937C()`; forced = pause/`[0x5FFC]∈{2,4}`; `duration <= timer`; zero timer+duration) | `fifa96_match_display_update(d, elapsed, forced)` (returns 1 on advance, 0 otherwise; state != 1 is a no-op) |
| stage callbacks `+0x38/+0x40/+0x48/+0x50`, `FUN_00053CC8` selector arm, `FUN_00053A48` draw table, `0x51xxx`/`0x52xxx` sub-screens, `[0x4E58C]`/`[0x8E24]` counters | not modelled (open legs) |

Tests (`tests/test_match_display.c`, suite 57 → **58**): init NULL; init
zeroing + duration 0x50 + selector=phase; the phase-`0x10` arm; suspend
enter edge (callback sees `suspend==0`, second call no callback) and callback
error abort; suspend leave order (leave sees `suspend==1`, then clear, then
refresh; second leave no-op) with first-error propagation and full sequence;
NULL handling; backend-less state-only operation; update state gate (state 0
and 7 no-ops); timer accumulation to the strict `duration <= timer`
boundary (30+49 then +1 advances and zeroes both); forced advance below the
deadline; elapsed pass-through and NULL error. ASan+UBSan build of
`test_match_display` clean (`cc -fsanitize=address,undefined -Iinclude
tests/test_match_display.c src/fifa96_loader/fifa96_match_display.c`).

## 7. Errata (quoted)

* FU-60 §6 S8 "three arms selected by
  `FUN_0004382C`/`0x43608`/`0x541D4`; `FUN_00043330`+`FUN_00058BC0`,
  `FUN_000432EC`+`FUN_00058D70`, or `FUN_00058B68`+`FUN_00049830` …" —
  **confirmed by the listing** (§1.1); `FUN_0004382C`/`FUN_00043608` are
  getters of `[0x677C]`/`[0x6780]` and `FUN_000541D4` gates on the
  `[0x4E674]`/`[0x4E5C8]`/`[0x4E688]` sub-screen records.
* FU-60 §5 table row "`0x4E574` … frame-suspend flag" — **refined**: it is
  field `+4` of the `0x4E570` display/overlay record; the exact set/clear
  pair is `FUN_00053DC4`/`FUN_00053DE0` after `FUN_00064074`/`FUN_00064080`
  (`[0x9A90]=0/1`) with `FUN_00045D0D` on clear; the complete xref set is
  §2.1.
* FU-63 §7 "`FUN_00052030 0x520C8` | 0 on menu transitions |
  `FUN_00064080()` + `FUN_00045D0D()`" — **confirmed**, with the second
  clear site `FUN_00052D68 0x52DCA` and the full 32-reference table (§2.1).
* FU-63 §5.4/FU-60 §9 `0x5B33C/340/344` — **re-verified**: the disassembly
  writes `[0x5B33C]=record[+0x18]`, `[0x5B340]++`,
  `[0x5B344]=now` exactly as quoted (errata 1 of FU-60 remains correct); the
  gate is `[0x5B37C]==record[+0x18]`; only `[0x5B33C]` has a reader
  (`FUN_00092194` → `FUN_00072AC4 0x73665`).
* FU-61 §4.2 "code == 7 → … (pause latch)" — **refined**: the record-code-7
  arm is the high-nibble (`0x40`) latch source; the mode-7 pause menu is
  entered by **match event code 1** via `FUN_00038004 0x3828E` → `0x3EABC`
  (screen `0x3DE`, `[0x4B018]=7`), and no static edge from the code-7 latch
  to `0x3EABC` exists (§3.4). The code-7 latch drives the overlay
  force-complete, camera reset, transition manager and in-match video
  trigger (§3.2).
* FU-64 §4.1 `FUN_00036C3C` maps `[0x5FFC]==6` to 4 — re-used here: the
  overlay state-1 and `0x51xxx` gates treat `[0x5FFC]∈{2,4}` as
  "menu/transition" and force-complete.
* Listing defects (new): `decompile_function 0x53BB8` fails;
  `FUN_00052030`/`FUN_00052D68`/`FUN_00053F90` decompile hit "bad
  instruction data"; the embedded tables at encoded `0x439BC`/`0x43B9C`/
  `0x27F90`/`0x27FC8`/`0x415A8`/`0x419C8` read as code/filler at the encoded
  address and must be read at `+0x10000` (flat `0x539BC`/`0x53B9C`/
  `0x37F90`/`0x37FC8`/`0x515A8`/`0x519C8`).

## 8. Open legs

* **`0x51xxx`/`0x52xxx` overlay sub-screens**: no static callers (full-image
  `E8` scan); reached via fixup-populated callback/jump-table entries not
  decoded here. Screen identities, the `[0x8E24]`/`[0x8E30]`/`[0x8E1C]`
  state meanings and `FUN_00053E08`'s sub-screen record layout are not
  asserted.
* **Pause menu (mode 7 / screen `0x3DE`)**: `FUN_0003E9E0`'s setup and the
  exit path (mode reset, `[0x4AF5C]`, `FUN_00037748(msg,10,ptr 0x47DE8,8)`)
  are not decomposed; whether the code-7 latch and event code 1 are the same
  physical key is an input-binding question (FU-61 open legs).
* **`FUN_00052030`/`FUN_00052D68` internals**: the `[0x4E588]`-keyed
  sub-screen updates, `FUN_00053E2C`/`FUN_00053E08`/`FUN_00053FF4` and the
  `[0x4E674]`/`[0x4E5C8]` record fields are only cited by site.
* **Video selection**: `[0x9808]`/`[0x980C]`, tables `0x9A20`/`0x9A50`,
  `FUN_00063688`/`FUN_00063628` and `FUN_0004C2E0` codes are quoted from the
  `FUN_00063734` listing only; no video identity is asserted.
* **`FUN_0004382C`/`FUN_00043608` display modes**: `[0x677C]` values 1/3 and
  `[0x6780]` are listed by writer only; arm semantics not labelled.
* **`[0x4E58C]`** transition counter: only `FUN_00053D58`'s window
  (`0xF1..0x168`) and `FUN_000510DC`'s `0..0x169` arithmetic are derived.
* **Callbacks**: the four `0x4E570` callback slots are zeroed at reset and
  never written in the analysed code; their runtime populators are not
  located.
* Object-base classification of each quoted global (FU-59 errata) was applied
  only to the tables read as bytes; other addresses are quoted as Ghidra
  displays them.

## Provenance

Ghidra MCP on `/fifa96_le.bin`:
`disassemble_bytes` `0x495C0` (256 B), `0x496C0` (208 B), `0x44F80` (560 B),
`0x4634C`/`0x46246` (via FU-61), `0x53A48` (312 B), `0x53BB8` (416 B),
`0x53DA0` (112 B), `0x51B40` (544 B), `0x51800` (608 B), `0x513A0` (128 B),
`0x3EABC` (164 B), `0x37F00` (1024 B), `0x926A0` (320 B), `0x73650` (96 B),
`0x415A8` (32 B), `0x515A8`, `0x519C8`;
`decompile_function` `0x53D58`, `0x4CD70`, `0x4D2D4`, `0x4382C`, `0x43608`,
`0x541D4`, `0x43E30`, `0x439D0`, `0x58BC0`, `0x58D70`, `0x58B68`, `0x565BC`,
`0x39180`, `0x43330`, `0x52D68`, `0x52030`, `0x537F8`, `0x51AB8`, `0x53F08`,
`0x53F90`, `0x54018`, `0x53A48`, `0x53D50`, `0x36C0C`, `0x1B128`, `0x64074`,
`0x64080`, `0x56698`, `0x37B24`, `0x36C3C`, `0x51270`, `0x512B0`, `0x4A178`,
`0x4FA36`, `0x510DC`, `0x63734`;
`read_memory` `0x539BC` (32 B), `0x53B9C` (32 B), `0x515A8` (32 B), `0x519C8`
(16 B), `0x37F90` (56 B), `0x37FC8` (64 B), `0x415A8`;
`search_instructions` operand `67ec`, `67ed`, `67f3`, `4b018`, `677c`,
`6780`, `4e578`, `4e57c`, `4e588`;
`get_xrefs_to` `0x45001`, `0x45069`, `0x450D9`, `0x44FB4`, `0x44FA8`,
`0x4E574` (32), `0x5B330`, `0x5B334`, `0x5B338`, `0x5B33C`, `0x5B340`,
`0x5B344`, `0x45155`;
`get_function_callees` `0x43E30`, `0x58BC0`, `0x58D70`, `0x58B68`, `0x62B40`,
`0x565BC`, `0x67FE4`; `get_function_by_address` `0x3EABF` (none), `0x5154C`
(none), `0x51CD8` (none), `0x53DC4`, `0x53DE0`, `0x3E9E0`, `0x1B128`,
`0x512B0`, `0x53A48`, `0x52D68`, `0x52030`, `0x537F8`, `0x510DC`, `0x63734`;
`run_script_inline` (analysis-only, not committed): full-image `E8 rel32`
scans for the `0x51xxx`/`0x52xxx`/`0x53xxx` call site sets and for `0x3EABC`
(the two callers `0x3828E`/`0x38F11`).

Analysis-only: no tool, capture-rig, ISO or Ghidra-project change staged.
Port write set: `include/fifa96_loader/fifa96_match_display.h`,
`src/fifa96_loader/fifa96_match_display.c`, `tests/test_match_display.c`,
`CMakeLists.txt` (one library/test block). `make test`: 57/57 before,
**58/58 after**; ASan+UBSan `test_match_display` clean. `game/FIFAPCCD96.iso`
untouched; `fifa96.rep/**` churn not staged.
