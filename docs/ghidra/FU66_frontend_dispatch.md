# FU-66: front-end loop and state dispatch close-out

Roadmap slice #3 of FU-58 §7 ("Front-end loop and dispatch close-out").
Derives the outer driver `FUN_0001DD40` and its boot caller chain, both
front-end loops in full (`FUN_0001E3A8`, `FUN_0001F5CC` — including the
panel tail FU-58 could not line-decode), the dispatcher `FUN_0001442C`, and
the **general LE fixup record format** of `FIFA96.EXE`, which populates the
20-cell `0x43DC` state table. Ports the loop/dispatch decision logic as
`fifa96_frontend`.

Result in one line: **the outer driver is `FUN_0001DD40` (called from
`0x188D9`, the index-1 case of the `0x8644` mode-selector table in
`FUN_00018680`, itself reached from the Watcom startup through
`FUN_000B26B1`); it alternates `FUN_0001E3A8` and `FUN_0001F5CC` until the
confirm flag `[0x5094]` is set or the front-end returns 10/11, then exits
through `FUN_0001B5B4(1)`; both loops end by dispatching **state 17**
(`0x1EB2B`, `0x1F8E9`) and exit by dispatching **state 16**
(`0x1EDFD` code 8; `0x1F9D1` panel tail — a decode FU-58 missed); the
complete `0x43DC` table is populated by 20 records whose format is now fully
decoded (all 32,461 records in the image parse with exact page-table
boundaries; only internal targets occur).**

## Method

* Static work on the open Ghidra MCP session for `/fifa96_le.bin` (program
  `/fifa96_le.bin`, flat link addresses as in FU-4/FU-58..FU-65). Every
  instruction below is read back from Ghidra
  (`disassemble_function`/`disassemble_bytes`/`read_memory`/`decompile_function`)
  unless stated otherwise. `disassemble_function` is used wherever the
  decompiler prunes blocks: `FUN_0001E3A8`'s Ghidra body ends at `0x1E60E`
  (the listing stops mid-setup) while the raw stream runs to `0x1EE3x`; the
  panel tail `0x1F9BC..0x1F9D5` mis-decodes from `0x1F9C8` in Ghidra
  (`IN AL,0x51`), so those bytes are hand-decoded from `read_memory`.
* The authoritative byte source is the flat LE image rebuilt from the ISO's
  `FIFA96.EXE` with `python3 tools/fifa96_le.py /tmp/opencode/fu58/FIFA96.EXE
  -o /tmp/opencode/fu66/fifa96_le.bin` (source md5
  `9a461768d610121c2bb5869a7c4cfc9d`, image md5
  `9f60fc0126d490505d736e1b161d408a`, 1,485,392 bytes; byte-identical to
  FU-65's rebuild). Regions were cross-checked against Ghidra `read_memory`
  at every quoted address (e.g. `0x1DD40`, `0x1ED80`, `0x1F9A0`, `0x143DC`,
  `0x18644`).
* The LE fixup stream is decoded by `/tmp/opencode/fu66/lefix.py`
  (analysis-only, not committed): header `0x290A4`, fixup page table
  `hdr+0x548 = 0x295EC` (264 dwords for 263 pages), record table
  `hdr+0x968 = 0x29A0C`. 32,461 records, **zero page-boundary mismatches**
  (the parser consumes exactly `pt[p+1]` bytes for every page). §1.
* Address-space note: object bases are obj1 code `0x10000`, obj2 stub
  `0xE0000`, obj3 data `0xF0000`, obj4 stack/data `0x100000`
  (`tools/fifa96_le.py --info`). A fixup target `obj N, offset X` resolves
  to link address `base(N) + X`. The state table's encoded displacement is
  `0x43DC`, so its flat link address (and Ghidra address) is `0x143DC`
  (obj1); the cells' targets resolve to `0x14451..0x14B28`. Other globals
  are quoted exactly as Ghidra displays the raw operand (`[0x49278]` etc.);
  no claim is made here about their containing object or runtime segment.

## 1. `0x43DC` population and the general LE fixup format

### 1.1 Record format (FU-58 open leg, closed)

The fixup record table is a byte stream grouped per page; the page table
gives the byte offset of each page's records. Every record in this image has
the same shape:

```
byte   source_flags            observed: 0x07 (32453x), 0x06 (8x)
byte   target_flags            observed: 0x00 (6864x), 0x10 (25589x)
word   source_offset LE        offset within the page (0..0xFFF)
------- target payload, one of:
  target_type = target_flags & 3
  type 0 (internal, all 32461 records):
      byte  target_object
      target_offset: word if (target_flags & 0x10)==0 else dword
```

Evidence: all 32,461 records parse under this rule; no other target type
occurs (`target_flags & 3 == 0` for every record); all source flags have
`source_flags & 0xF8 == 0` (no iterated bit, no other flag bits); target
objects are obj4 28,588 / obj1 3,861 / obj2 4 / obj3 8. The rule is validated
by (a) exact page boundaries on all 263 pages, (b) the 20 state cells landing
on the known pointer table, (c) handler targets resolving to code entry
points. The 8 `source_flags==0x06` records (sources `0x97E2F..0x97E57`,
`0x10DDE6..0x10DE16`; file `0x5B53B..0x5B556`, `0x6C2A2..0x6C2BD`; all
32-bit internal targets) use a second 4-byte source encoding whose bit
semantics are **not asserted** (open leg).

Example record (state 0), raw bytes at file `0x2AC8F`:

```
07 00 dc 03 01 51 44
^  ^  ^______ target type 0, target object 1, target offset 0x4451
|  |  source offset 0x03DC in page 4
|  target flags 0x00 -> 16-bit target offset
source flags 0x07
```

Flat source `0x14000 + 0x3DC = 0x143DC`; flat target `0x10000 + 0x4451 =
0x14451`.

### 1.2 Complete state → handler table (all 20 cells populated)

Page 4 (`0x4000..0x4FFF`) holds 99 records; exactly 20 have source offsets
`0x3DC..0x428` (80 bytes = 20 dwords). Records for page 4 are at file
`0x2AAE7..0x2ADCE` (page-table entries `pt[4]=0x10DB`, `pt[5]=0x13C2`); the
20 cell records occupy file `0x2AC0A..0x2AC96`. **No cell lacks a fixup.** Handler
cells (raw object offsets in the image; flat = `0x10000 + value`) and the
first bytes at each target:

| state | cell/encoded | flat cell | handler | first bytes | record file |
|------:|--------------|-----------|---------|-------------|-------------|
| 0 | `0x43DC` | `0x143DC` | `0x14451` | `6a ff 6a 00 68 c0 f9 ff` | `0x2AC8F` |
| 1 | `0x43E0` | `0x143E0` | `0x144A8` | `6a ff 6a 00 6a 00 6a ff` | `0x2AC88` |
| 2 | `0x43E4` | `0x143E4` | `0x14479` | `6a 01 6a 00 6a 00 6a ff` | `0x2AC81` |
| 3 | `0x43E8` | `0x143E8` | `0x144D0` | `6a 01 6a 00 68 c0 f9 ff` | `0x2AC7A` |
| 4 | `0x43EC` | `0x143EC` | `0x144FF` | `6a ff 6a 00 68 c0 f9 ff` | `0x2AC73` |
| 5 | `0x43F0` | `0x143F0` | `0x14554` | `6a ff 6a 00 6a 00 6a ff` | `0x2AC6C` |
| 6 | `0x43F4` | `0x143F4` | `0x145A9` | `6a ff 6a 00 68 c0 f9 ff` | `0x2AC65` |
| 7 | `0x43F8` | `0x143F8` | `0x14648` | `6a ff 6a 00 6a 00 6a ff` | `0x2AC5E` |
| 8 | `0x43FC` | `0x143FC` | `0x146DC` | `6a ff 6a 00 68 30 f8 ff` | `0x2AC57` |
| 9 | `0x4400` | `0x14400` | `0x1470E` | `6a ff 6a 00 6a 00 6a ff` | `0x2AC50` |
| 10 | `0x4404` | `0x14404` | `0x14742` | `6a ff 6a 00 68 c0 f9 ff` | `0x2AC49` |
| 11 | `0x4408` | `0x14408` | `0x14798` | `6a ff 6a 00 6a 00 6a ff` | `0x2AC42` |
| 12 | `0x440C` | `0x1440C` | `0x147EE` | `6a ff 6a 00 6a 00 6a ff` | `0x2AC3B` |
| 13 | `0x4410` | `0x14410` | `0x14882` | `6a ff 68 c0 f9 ff ff 6a` | `0x2AC34` |
| 14 | `0x4414` | `0x14414` | `0x1491F` | `6a ff 6a 00 6a 00 6a ff` | `0x2AC2D` |
| 15 | `0x4418` | `0x14418` | `0x149B3` | `6a ff 68 c0 f9 ff ff 6a` | `0x2AC26` |
| 16 | `0x441C` | `0x1441C` | `0x14A50` | `6a ff 6a 00 6a 00 6a ff` | `0x2AC1F` |
| 17 | `0x4420` | `0x14420` | `0x14AAA` | `6a ff 68 c0 f9 ff ff 6a` | `0x2AC18` |
| 18 | `0x4424` | `0x14424` | `0x14B01` | `6a ff 6a 00 6a 00 6a ff` | `0x2AC11` |
| 19 | `0x4428` | `0x14428` | `0x14B28` | `6a ff 6a 00 68 c0 f9 ff` | `0x2AC0A` |

This re-derives FU-58's map (same 20 handlers) from the record bytes and
adds per-cell file citations. Screen/asset identity for states outside
7..11 and 16/17 remains unasserted (open legs; FU-58 §8/FU-65 §8).

### 1.3 Dispatcher bounds `FUN_0001442C`

Byte-verified (`disassemble_function 0x1442C`):

```
0x1442C PUSH EBX/ECX/EDX/ESI/EDI/EBP
0x14432 MOV ESI,EAX
0x14434 CALL 0x18BA8
0x14439 MOV EBP,1
0x1443E MOV EDI,EAX
0x14440 CMP ESI,0x13
0x14443 JA 0x14B56            ; > 0x13 -> shared event-pump tail
0x14449 JMP dword ptr CS:[ESI*4 + 0x43DC]
```

So `state <= 0x13` resolves through the 20-cell table (flat `0x143DC`);
`state > 0x13` never touches the table and runs the pump at `0x14B56`
(`FUN_000659F8`/`FUN_00065CC0` …; FU-58 §1). The state is register-passed
(`EAX`), never stored globally (FU-58 §1 confirmed).

## 2. Outer driver `FUN_0001DD40`

Full listing, byte-verified (`disassemble_bytes 0x1DD40 160`; Ghidra now has
the function `body_start=0x1DD40 body_end=0x1DDC5`):

```
0x1DD40 PUSH EBX/ECX/EDX/ESI/EDI/EBP
0x1DD46 MOV EBX,3 / MOV EDX,0x50 / MOV EAX,1 / MOV ECX,0x68
0x1DD5A MOV ESI,0x49278 / MOV EDI,0x49210
0x1DD64 CALL 0x1B1C8          ; module/screen select, EAX=1
0x1DD69 MOVSB.REP             ; 0x68 bytes 0x49278 -> 0x49210 (settings hand-off)
0x1DD6B XOR EBP,EBP
0x1DD6D XOR EDX,EDX
loop:
0x1DD6F CMP EDX,[0x5094]      ; EDX is always 0 here (see below)
0x1DD75 JZ  0x1DD7C
0x1DD77 MOV EBP,2             ; confirm flag -> exit phase
0x1DD7C TEST EBP,EBP
0x1DD7E JBE 0x1DD87           ; EBP==0 -> front-end loop
0x1DD80 CMP EBP,1
0x1DD83 JZ  0x1DDA9           ; EBP==1 -> panel loop
0x1DD85 JMP 0x1DDB0
0x1DD87 CALL 0x1E3A8          ; FUN_0001E3A8
0x1DD8C MOV EBP,EAX
0x1DD8E CMP EAX,8
0x1DD91 JNZ 0x1DD98
0x1DD93 MOV EBP,1
0x1DD98 CMP EBP,0xA
0x1DD9B JZ  0x1DDA2
0x1DD9D CMP EBP,0xB
0x1DDA0 JNZ 0x1DDB0
0x1DDA2 MOV EBP,2
0x1DDA7 JMP 0x1DDB0
0x1DDA9 CALL 0x1F5CC          ; FUN_0001F5CC
0x1DDAE MOV EBP,EDX           ; EDX = 0 (preserved through both loops)
0x1DDB0 CMP EBP,2
0x1DDB3 JNZ 0x1DD6F           ; loop top (confirm re-check first)
0x1DDB5 MOV EAX,1
0x1DDBA CALL 0x1B5B4          ; exit selector 1
0x1DDBF POP EBP/EDI/ESI/EDX/ECX/EBX
0x1DDC5 RET
```

Semantics (all evidenced by the listing):

* Head: `FUN_0001B1C8` with `EAX=1` selects/loads the module whose 8-byte
  descriptor is at `[0x49120 + index*8]` (fixup target object 4,
  `0x149120`), falling back to the code table at object offset `0xB180`
  (flat `0x1B180`) for an unloaded index, else regenerating the VIV surface
  descriptors at `[0x47C30]`/`[0x47DC0]` through `FUN_00018C10`; then the
  0x68-byte settings block is copied `0x49278 -> 0x49210`.
* `EBP`/`EDX`: `EDX` stays 0 for the whole driver: both loops preserve it
  (`FUN_0001E3A8` returns `CONCAT44(param_2, code)`, i.e. EDX untouched;
  `FUN_0001F5CC` pushes/pops EDX — `0x1F5CE`/`0x1F9E1`), so `MOV EBP,EDX` at
  `0x1DDAE` is always `EBP=0`, the panel returns to the front-end.
* `[0x5094]` is checked at the **top of every iteration** (`0x1DD6F`): if
  set, `EBP=2` and the driver exits — including between a front-end return
  of 8 and the panel call.
* Front-end result map: `8 -> EBP=1 (panel)`, `10/11 -> EBP=2 (exit)`,
  any other value is kept in EBP (the front-end loop only returns 8/10/11,
  §4).
* Exit: `MOV EAX,1; CALL 0x1B5B4`. `FUN_0001B5B4` (byte-verified:
  `PUSH EDX; LEA EDX,[EAX*8]; MOV EAX,[EDX+0x49120]; CALL 0x18F04`) loads
  index-1 cell (`0x49120 + 1*8`) of the descriptor table at object offset
  `0x49120` (the operand has its own fixup: source `0x1B5BE`, record file
  `0x2CDB8`, target obj4 offset `0x49120` → `0x149120`) and tail-calls
  `FUN_00018F04`, which calls `FUN_000993EC` when the entry is nonzero.
  `FUN_000993EC` is not decomposed (open leg). The driver's own `POP`s then
  return if that call returns.

## 3. Boot chain and the `0x8644` mode selector (driver caller)

`get_xrefs_to` (Ghidra and raw-byte agreement):

```
startup 0x9FD10 -> 0x9FD88 (FU-4)
0x9FF9A CALL 0xB26B1            (sole caller of FUN_000B26B1)
0xB26F1 CALL 0x18680            (sole caller of FUN_00018680)
0x188D9 CALL 0x1DD40            (the driver call, fixup-confirmed)
```

`FUN_00018680` (decompile has the switch and the `Removing unreachable
block (ram,0x00018969)` warning) ends in a mode-selector loop over the table
at object offset `0x8644` (flat `0x18644`, all 11 cells fixup-populated):

| idx | handler | call site | record file |
|----:|---------|-----------|-------------|
| 0 | `0x188CF` | `CALL 0x1FC50` | `0x2C164` |
| 1 | `0x188D9` | `CALL 0x1DD40` (**outer driver**) | `0x2C15D` |
| 2 | `0x18927` | `CALL 0x206FC` | `0x2C156` |
| 3 | `0x1894A` | `MOV ESI,ECX; JMP 0x18961` (ends loop) | `0x2C14F` |
| 4 | `0x188E3` | `[0x5090]`/`[0x5094]`/`[0x4FB8]` sub-dispatch (table `0x8670`) | `0x2C148` |
| 5 | `0x1892E` | `CALL 0x28960` (competition module, FU-65) | `0x2C141` |
| 6 | `0x18935` | `CALL 0x2F9DC` | `0x2C13A` |
| 7 | `0x1893C` | `CALL 0x31AA4` | `0x2C133` |
| 8 | `0x18943` | `CALL 0x32DE0` | `0x2C12C` |
| 9 | `0x18961` | loop test / exit test | `0x2C125` |
| 10 | `0x1894E` | `CALL 0x17F80` then `0x18953 CALL 0x18108` | `0x2C11E` |

Selector at `0x1887B..0x188C8` (byte-verified; `0x188C1` `CMP EAX,0xA` /
`JA 0x18961` is the real guard — Ghidra's listing overlaps it with a phantom
`XCHG` and starts the JMP at `0x188C8`):

```
0x18865 XOR EAX,EAX ; TEST ESI,ESI ; JNZ 0x18969 (skip selector)
0x1886F MOV EDI,3 ; MOV ECX,1 ; XOR EBX,EBX ; MOV EDX,[0x5090]
         [0x5090]==1 -> EAX=5, ==2 -> EAX=6, ==3 -> EAX=7, else EAX unchanged
0x188A7 CMP EBX,[0x5094] ; JZ + ; MOV EAX,3
0x188B1 CMP EBX,[0x4FB8] ; JZ + ; MOV EAX,4
0x188BE CMP EAX,0xA ; JA 0x18961
0x188C7 JMP CS:[EAX*4+0x8644]
cases ... JMP 0x1895F
0x1895F MOV EAX,EBX ; 0x18961 TEST ESI,ESI ; 0x18963 JZ 0x1887B
exit sequence 0x18969: FUN_00017F80, FUN_000CE7A0, FUN_0009FBC0(0),
  FUN_00065D58, FUN_00011580, FUN_0001770C, FUN_00034278, FUN_0004B014,
  FUN_0006D7EC, MOV EAX,[0x4FAC]; CALL 0x18F04
```

With `EBX` zeroed once at `0x18879` and written nowhere else in the dispatch
region's own instructions, the straight-line selector emits indices
**{0,3,4,5,6,7}** only (a handler that clobbers the callee-saved EBX would
change the loop-carried `EAX=EBX` at `0x1895F`, but none is shown doing so).
Thus the driver case is cell 1 of `0x8644`, but the decoded selector has no
straight-line path to index 1 (or to 2/8/9/10); the runtime activation path
is an open leg (§9). The call-site placement (FU-65 §1.4) is unaffected:
`0x188D9` is the only `CALL 0x1DD40` in the image (`E8 62 54 00 00`).

## 4. Front-end loop `FUN_0001E3A8`

Setup (decompile): if `[0x5440]!=0` -> `FUN_0001A240()` (screen
teardown/reinit); screen/VIV init (`FUN_000CE70C`, `FUN_00019ABC`,
`FUN_00019B44`), `FUN_00013888` x4, a 0x68-byte settings-derived widget run
(`FUN_00015960` x12 writing `0x492E0/0x492E4/0x492E8`), sprite/UI setup,
and the widget-record field inits at `0x49314..0x49358`. Entry branch
(byte-verified at `0x1EB1C`):

```
0x1EB1C MOV EAX,[0x47C30] / CALL 0x14C90     ; VIV load
0x1EB26 MOV EAX,0x11 / CALL 0x1442C          ; state 17 (entry screen)
```
only when `[0x5440]==0`; otherwise `FUN_0001A2A0()` (no state dispatch).
Then `CALL [0x12AFC]` (frame callback).

Loop (do-while; top checks at `0x1EB36..0x1EB50` break to the exit tail at
`0x1ED86`; bottom at `0x1ED80` loops to `0x1EB3F`):

```
do {
  FUN_00065920(); FUN_000125C4();                  ; frame services
  if ([0x5454]==0 && [0x5458]==0 && [0x545C]==0)
      { r = FUN_00016350(); if ([0x5460]==3) { r=-1; [0x5460]=1; } }
  else { menu sub-flow calls (FUN_00015124, FUN_00013340, FUN_0001DA58,
         FUN_0001DFB8, FUN_0001ADF8); r=-1; }
  if ([0x5460]==1) three FUN_0001E274 calls set [0x5454]/[0x5458]/[0x545C];
  [0x5460]=0;
  if (0<=r<8)  { FUN_0001DD20(); FUN_0001DA58(); FUN_0001DFB8(); FUN_0001DF10(); }
  if (r==9)    { FUN_0001DDC8(); FUN_0001DFB8(); }
  if (r==-10)  { FUN_0001771C(); if (FUN_0001D26C()==1) { r=10; [0x5094]=1; } }
  if (r==-0xB) { FUN_0001DFB8(); }
  if (r!=-1)   { FUN_0001ADF8(); }
  FUN_00012728(); FUN_00018B18();
} while (r != 8);                                  ; breaks on r==10 || r==11
```

Event map (all from the decompile, `0x1EC..` region; `[0x5094]` write site
`0x1ED5A`):

| raw code | classification | action |
|---:|---|---|
| 0..7 | menu navigation | `FUN_0001DD20`/`FUN_0001DA58`/`FUN_0001DFB8`/`FUN_0001DF10` family |
| 8 | exit | leaves the loop; the state-8 tail below |
| 9 | select | `FUN_0001DDC8` + `FUN_0001DFB8` |
| -10 (`0xFFFFFFF6`) | confirm | `FUN_0001D26C` gate; on 1 -> code becomes 10 and `[0x5094]=1` |
| -11 (`0xFFFFFFF5`) | jump | `FUN_0001DFB8` |
| -1 | none | no per-event helper (still frame services) |
| 10 | settings save | exit for settings path |
| 11 | settings reload | exit for reload path |

Exit tail (hand-decoded bytes `0x1ED86..0x1EE3A`; decompile agrees except
where noted):

```
0x1ED86 CALL 0x125C4                                    ; post-loop service
r==11: REPE CMPSB 0x68 bytes [0x49210] vs [0x49278]
       ; if not equal -> CALL 0x1DC80
r==10: if [0x4921C] != [0x49284] -> CALL 0x175B8
       REP MOVSB 0x68 bytes [0x49210] -> [0x49278]        ; save working settings
r==8:  [0x5440]=0; CALL 0x14BB0([0x47C30],[0x47DC0]);
       MOV EAX,0x10; CALL 0x1442C                          ; state 16 (flat 0x1EDFD)
else (defensive, unreachable after the loop):
       CALL 0x1ADF8; CALL 0x1A280; [0x5440]=1
then:  if ([0x492CC]==0) CALL 0x15474 else CALL 0x154B0;
       CALL 0x122A0; return r in EAX (EDX = incoming)
```

`FUN_0001A240` (entry branch) decompiles to `FUN_000CE7A0`,
`FUN_000CBDB0(0)`, conditional `FUN_00018AC8`, `FUN_000CE6F0([0x5098])`;
`FUN_0001A2A0` to screen init plus a 0x300-byte expansion into `0x47E80`.
`FUN_0001DC80` decompiles to `FUN_00018BF0` -> `FUN_00069358` ->
`FUN_0003749C`; `FUN_000175B8` compares a 4-byte tag at object offset
`0x148` and loads an entry (internals not decomposed; open leg).

## 5. Panel loop `FUN_0001F5CC` (tail decoded)

Setup (decompile): `FUN_00013888([0x47CC0],0x138)`, screen init, widget
setups (`FUN_00015960` x3), `FUN_00012DE4`/`FUN_00012E78`/`FUN_0009AE70`
sprite draws, then:

```
0x1F8E4 MOV EAX,0x11 / 0x1F8E9 CALL 0x1442C     ; state 17 again
```

Loop (`ESI` starts -1 at `0x1F6A4`):

```
0x1F8EE CMP ESI,4 / JZ exit; CMP ESI,5 / JZ exit
body: FUN_00065920(); FUN_000125C4(); r = FUN_00016350(); ESI = r;
  if ((r==0 || r==1) && FUN_0001A070()!=0) -> select animation:
      FUN_00012490(0x15E), FUN_00012728, FUN_00012490(0),
      FUN_0001F440(r), FUN_0001A04C(1), FUN_00013600(0x6B), FUN_00012D7C
  if (r==-10) { FUN_0001771C(0x1C2,1);
                if (FUN_0001D26C()==1) { r=5; [0x5094]=1; } }   ; 0x1F994
  FUN_00012728(); FUN_00018B18();
0x1F9A3 CMP ESI,4 / JNZ body            ; 4 exits at the bottom, 5 at the top
```

Tail (hand-decoded from `read_memory 0x1F9A0`; Ghidra mis-decodes from
`0x1F9C8`):

```
0x1F9AC CMP dword [0x5094],0
0x1F9B3 JZ  0x1F9BC
0x1F9B5 CALL 0x1A280                          ; confirm -> cleanup only
0x1F9BA JMP 0x1F9D6
0x1F9BC MOV EDX,[0x47DC0]
0x1F9C2 MOV EAX,[0x47C30]
0x1F9C7 CALL 0x14BB0                          ; VIV reload
0x1F9CC MOV EAX,0x10
0x1F9D1 CALL 0x1442C                          ; state 16
0x1F9D6 CALL 0x122A0
0x1F9DB ADD ESP,4; POP EBP/EDI/ESI/EDX/ECX/EBX; RET
```

So the panel's non-confirm exit also dispatches **state 16**, the same
transition as the front-end code-8 tail (FU-58 §3.3 decompile showed the
call but not its `EAX=0x10` argument — errata §8).

## 6. Flag lifecycles (evidenced)

| flag | xrefs (this slice) | evidenced role |
|---|---|---|
| `[0x5094]` | 32 xrefs; driver read `0x1DD6F`; front-end write `0x1ED5A` (-10 gate), panel write `0x1F994` (-10 gate), panel read `0x1F9AC`; many other writers in menu/competition modules | "confirm accepted": suppresses the pending screen dispatch in both loops and forces the driver to exit. Full semantics outside this slice are not asserted. |
| `[0x5440]` | exactly 4 xrefs, all inside `FUN_0001E3A8`: read `0x1E3AE` (entry), read `0x1EAF7` (state-17 gate), write `0x1EDED` (=0 on code 8), write `0x1EE13` (=1 in the defensive cleanup) | front-end-local flag (no other consumer): nonzero skips the state-17 entry screen and runs `FUN_0001A2A0`; code 8 clears it. Meaning not asserted. |
| `[0x49210]`/`[0x49278]` | driver copies 0x68 bytes `0x49278 -> 0x49210`; code 10 copies back; code 11 compares | working/persisted settings pair (FU-58 §6/FU-64 §1). |
| `[0x4921C]`/`[0x49284]` | `0x1EDC4..0x1EDCF` compare | per-code-10 settings field gate (semantics open). |
| `[0x492CC]` | `0x1EE2E` tail select | selects `FUN_00015474`/`FUN_000154B0` after the front-end (open). |
| `[0x5090]`, `[0x4FB8]` | `0x1887B..0x188B9` selector reads | mode-selector inputs (FU-64 §1.3); value semantics not asserted. |
| `[0x5454]`/`[0x5458]`/`[0x545C]`/`[0x5460]` | front-end loop only | menu sub-flow gates set by `FUN_0001E274`; internals open. |

## 7. Port: `fifa96_frontend`

`include/fifa96_loader/fifa96_frontend.h` +
`src/fifa96_loader/fifa96_frontend.c` (caller-owned struct, no globals,
`-fifa96_err_t`, no comments). Scope: the evidenced decision logic of
§1.3/§2/§4/§5.

| original | port |
|---|---|
| `0x14440..0x14449` bound + table index (`state <= 0x13`, else pump) | `fifa96_frontend_dispatch_resolve` (caller-owned 20-cell `handlers` array; 0 cell -> `-FIFA96_ERR_NOT_FOUND`, `state>0x13` -> no handler) |
| `0x1DD40..0x1DDC5` driver loop | `init`, `driver`, `frontend_result`, `panel_result` |
| `0x1DD6F` confirm check at loop top | `driver` (confirm -> `phase=EXIT`) |
| `0x1DD8E..0x1DDA7` 8/10/11 map | `frontend_result` (others -> `-FIFA96_ERR_UNSUPPORTED`) |
| `0x1DDAE` `EBP=EDX` -> front-end again | `panel_result` (`phase=FRONTEND`) |
| `0x1E3A8` entry state 17 when `[0x5440]==0` | `entry_state` (returns 1 + state 17 when `in_match==0`) |
| `0x1E3A8` event classification incl. -10 gate | `event` (gate supplied by caller; accepted gate sets `confirm` and maps -10 -> 10) |
| `0x1E3A8` exit: 8 -> `[0x5440]=0`/state 16; 10/11 -> settings | `exit` (`STATE16`/`SETTINGS`/`SETTINGS_ALT`) |
| `0x1F5CC` 4/5 exit, 0/1 select gate, -10 gate | `panel_event` (`EXIT`/`SELECT`/`CONFIRM`/`NONE`) |

Not ported (app-side or unproven): `FUN_0001B1C8` module/screen select and
the settings copy, widget setup, per-event menu helpers, `FUN_0001DC80` /
`FUN_000175B8`, VIV reload / `FUN_0001A280` cleanup, state-16/17 dispatch
side effects, `FUN_0001B5B4` exit selector, and the panel/tail cleanup
branches.

Tests (`tests/test_frontend.c`, suite 54 -> **55**): init zeroing; driver
confirm gate and pre-panel re-check; front-end result 8/10/11 and
unsupported codes; panel result returns to front-end; entry state 17 vs
in-match; event classification (0..7, 8, 9, 10, 11, -1, -11, -10
accept/reject, unlisted); panel event classification (4/5, 0/1 with/without
select gate, -10 with/without gate, unlisted); exit classification; dispatch
resolve (bounds 0/19/20/0xFFFFFFFF, zero cell, NULLs); end-to-end driver
sequence. ASan+UBSan build of `test_frontend` clean
(`cc -fsanitize=address,undefined -Iinclude tests/test_frontend.c
src/fifa96_loader/fifa96_frontend.c`). `make test`: 54/54 before,
**55/55 after**.

## 8. Errata (quoted)

* FU-58 §3.3 panel tail: "`if ([0x5094] == 0) { func_0x00014bb0();
  FUN_0001442c(); } else FUN_0001a280();` (decompile)" — **corrected**: the
  bytes at `0x1F9CC..0x1F9D1` are `B8 10 00 00 00 E8 56 4A FF FF`, i.e.
  `MOV EAX,0x10; CALL 0x1442C`; the panel's non-confirm tail dispatches
  **state 16**, not an argument-less call.
* FU-58 §2/§3.1/§4 table addresses: the cells are the dispatcher's
  object-relative displacement `0x43DC..0x442B`; the flat link address is
  `0x143DC..0x1442B` (object 1 base `0x10000`). `read_memory 0x43DC` reads
  the zero filler below the first object, not the table. FU-58's record
  decode and handler list are otherwise confirmed exactly.
* FU-58 §8 "Fixup record format is only decoded for the state cells" —
  **closed**: §1 decodes every record in the image (32,461, exact page
  boundaries); the state cells are 20 of them.
* FU-58 §3.1 driver: "Copies the 0x68-byte settings block ... then sets
  EBP=0 or 2" — **confirmed and extended**: the copy is preceded by
  `FUN_0001B1C8(1)`; `MOV EBP,EDX` after the panel is always 0 because both
  loops preserve EDX; the exit call is `FUN_0001B5B4(1)` which loads the
  object descriptor `0x49128` and calls `FUN_00018F04`/`FUN_000993EC`.
* FU-64 §1.3 "a jump-table dispatch at `0x188C7` (table `0x8644` ... →
  values 3..7)" — **extended**: the table has 11 populated cells (0..10)
  and the real guard is `0x188C1 CMP EAX,0xA / JA 0x18961` (Ghidra overlaps
  it with a phantom `XCHG`); the straight-line selector emits
  `{0,3,4,5,6,7}`, the driver is cell 1 (`0x188D9`), and cells
  1/2/8/9/10 have no decoded selection path (open leg).
* FU-65 §1.4 "the `0x188D9 CALL 0x1DD40` ... closing FU-58's 'driver caller
  unknown'" — **confirmed** as a call-site placement (cell 1, record
  `0x2C15D`); the dynamic selection path is not proven (§3).

## 9. Open legs

* Screen/asset identity for states 0-6, 12-15, 18, 19 (and the selector
  ids of 16/17) remains unasserted; only handler entries and prologues are
  derived.
* `0x8644` cells 1/2/8/9/10 are populated but not emitted by the decoded
  selector; the runtime path that selects the outer driver (or proves it
  dead in this build) is unknown.
* `FUN_000993EC` (driver exit target, loaded from `0x49128`) is not
  decomposed; `FUN_0001B5B4`'s object-descriptor semantics are not asserted.
* `0x49120`/`0xB180` module-table contents (what `FUN_0001B1C8(1)` selects)
  are not decomposed beyond the fixup targets.
* `source_flags==0x06` records (8) are not semantically distinguished from
  `0x07`.
* `FUN_000175B8` (code-10 settings gate) and `FUN_0001DC80` (code-11
  settings hook) internals; `[0x4921C]`/`[0x49284]` field meanings.
* Front-end menu sub-flow gates `[0x5454]`/`[0x5458]`/`[0x545C]`/`[0x5460]`
  and `FUN_0001E274`/`FUN_0001DDC8`/`FUN_0001DF10` semantics.
* `FUN_0001A240`/`FUN_0001A2A0` internals and the `[0x5098]` object.

## Provenance

Ghidra MCP on `/fifa96_le.bin`:
`disassemble_function` 0x1442C, 0x1E3A8, 0x1F5CC; `disassemble_bytes`
0x1DD40 (160 B), 0x18860 (104 B), 0x188C0 (144 B), 0x18950 (176 B),
0x1B5B4 (16 B); `decompile_function` 0x18680, 0x1B1C8, 0x1B5B4, 0x18F04,
0x18C10, 0x1DC80, 0x175B8, 0x1A240, 0x1A2A0, 0x1FC50, 0x993EC, 0x122A0;
`read_memory` 0x1DD40, 0x188A8, 0x1EB1C, 0x1ED80, 0x1F9A0, 0x143DC,
0x18644, 0x149120; `get_xrefs_to` 0x1DD40, 0x5440, 0x5094, 0x1E3A8,
0x1F5CC, 0xB26B1, 0x18680, 0x188C7, 0x1887B, 0x188D9;
`get_function_by_address` 0x1DD40; `search_byte_patterns` `ff248544860000`
(one hit, 0x188C8).
Off-Ghidra (analysis-only, `/tmp/opencode/fu66/`): `tools/fifa96_le.py`
from `/tmp/opencode/fu58/FIFA96.EXE` (md5
`9a461768d610121c2bb5869a7c4cfc9d`, image md5 `9f60fc0126d490505d736e1b161d408a`)
and `lefix.py` (32,461 records; shapes, per-cell records and file offsets,
exact page-boundary check; handler prologue bytes).

Analysis-only: no tool, capture-rig, ISO or Ghidra-project change staged.
Port write set: `include/fifa96_loader/fifa96_frontend.h`,
`src/fifa96_loader/fifa96_frontend.c`, `tests/test_frontend.c`,
`CMakeLists.txt` (one library/test block). `make test`: 54/54 before,
**55/55 after**; ASan+UBSan `test_frontend` clean. `game/FIFAPCCD96.iso`
untouched; `fifa96.rep/**` churn not staged.
