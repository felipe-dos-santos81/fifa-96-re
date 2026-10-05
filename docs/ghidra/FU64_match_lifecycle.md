# FU-64: the match lifecycle — start path, setup/register, exit and teardown

Roadmap slices S1 (match start path) and S9 (teardown) of FU-60 §6, plus the
FU-58 §7 slice 9 open leg ("match start path"). Derives the never-closed
caller chain into the `0x493A0` setup, the initialization sequence before the
first frame, the `[0x5FFC]` exit staging, the `[0x58822]` match-over stop and
the teardown, and ports the evidenced start/stop/exit state machine.

Result in one line: **`0x493A0` has three static callers the Ghidra listing
misses — `0x180AA` in `FUN_00018014` (menu/competition path, selector 0,
reached from `FUN_00020954`/`0x2FAF0`/`0x32E10`), `0x18359` in `FUN_00018108`
(main pre-match path, selector `EBP=1`, reached from `FUN_00018680` at
`0x186C2`) and `0x18819` in `FUN_00018680` itself (boot/main flag block,
selector 0, reached from the startup via `FUN_000B26B1` at `0xB26F1`) — so the
setup is reached; the setup stores the selector to `[0x72F8]`, sets
`[0x72FC]=1`, zeroes `[0x7330]`/`[0x730C]`/`[0x7310]`, runs 21 evidenced init
calls (RNG seed, settings, clock/state zero, screen buffer, match reset,
display/player state, input/pace reset), sets `[0x5FFC]=0`, releases the pace
hold `[0x7324]=0` and registers `0x49320` in the 8-slot INT-8 table `0x5BAE4`
through `FUN_0009F64C`; the match loop leaves by setting `[0x5FFC]=4` and
`[0x7304]=3` (`0x497D3`/`0x497FD`), the period-end path in `FUN_0004B100`
additionally sets `[0x5FFC]=2` (`0x4B249`), and the teardown (`0x49541`) holds
the pace (`[0x7324]=1` at `0x49546`), cancels the callback through
`FUN_0009F684(0x39320)` at `0x4955A`, runs `FUN_0004AEA4` + `FUN_00067948`
(0x49567/0x4956C), zeroes `[0x72FC]`/`[0x1471C]`, and for `[0x7304]==3` calls
`FUN_0006D7EC(2)` then sets `[0x7304]=2`, returning `[0x7304]==2`.**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58..FU-63). All
  instructions below were read back from Ghidra
  (`disassemble_function`/`disassemble_bytes`/`read_memory`/`decompile_function`)
  unless a paragraph says otherwise; the decompiler was used for callee
  characterisation where it succeeded.
* **New method this slice (and why FU-60/FU-58 missed the callers):** the three
  `0x493A0` call sites sit in regions Ghidra never disassembled, so
  `get_xrefs_to 0x493A0` returns 0 references and `search_instructions`
  operand `493a0` returns 0 matches (both re-run this slice). The call sites
  were recovered by scanning every `E8`/`E9` rel32 in the **authoritative link
  image reconstructed from the ISO's `FIFA96.EXE`** with
  `tools.fifa96_le.parse` (the on-disk `/tmp/opencode/fifa96_le.bin` is stale
  per FU-60 Method). The reconstructed image matches Ghidra byte-for-byte at
  the sampled addresses (`read_memory 0x180A0` = `00 00 00 e8 44 b4 01 00 31
  c0 e8 f1 12 03 00 …`; the `E8` at `0x180AA` is `e8 f1 12 03 00`, target
  `0x180AF + 0x312F1 = 0x493A0`). Three sites, no others:
  `0x180AA`, `0x18359`, `0x18819` (`search` over all 1,485,392 bytes).
* **Data-reference check:** the LE fixup stream was parsed this slice (32,461
  records; shapes `(source 0x07, target 0x00)` ×6864, `(0x07, 0x10)` ×25589,
  `(0x06, 0x10)` ×8). Exactly two records target the `0x49xxx` module:
  both target `0x49320` (callback) at image sources `0x4945B` and `0x49556`
  (the two `PUSH 0x39320` immediates). **No fixup targets `0x493A0` or any
  address in `[0x493A0, 0x49460)`**, so the setup address is not in any
  fixup-populated data table either — the rel32 callers are the only entries.
* Listing defects met this slice (errata §7): `disassemble_bytes 0x493A0`
  mis-decodes two windows (`0x4946A..0x4949A`, `0x49520..0x4954F`) and
  `disassemble_bytes 0x4AEA4` starts at `0x4AEB0` (the `0x4Axxx` gap is
  undefined); those regions were hand-decoded from `read_memory` and are
  quoted byte-verified.
* FU-47 §2 (quoted) had already found `0x493A0` callers with a raw `E8` scan
  "called from `0x180AA`, `0x18359`, `0x18819`" but did not place them; this
  slice places the whole chain and the setup.

## 1. Start path: who calls `0x493A0`

Three `CALL 0x493A0` sites (all `E8 rel32`):

| site | caller | selector (EAX) | next use of the result |
|---|---|---|---|
| `0x180AA` | `FUN_00018014` (`0x18014..0x180C2`) | 0 (`XOR EAX,EAX` `0x180A8`) | `0x180AF CMP [0x5094],0 / JNZ`; if zero `CALL 0x17DF0` |
| `0x18359` | `FUN_00018108` (`0x18108..0x1839E`) | `EBP` (=1 set at `0x18169`) | `0x1835E TEST EAX,EAX / JZ 0x18170` |
| `0x18819` | `FUN_00018680` (`0x18680..0x188C7+`) | 0 (`XOR EAX,EAX` `0x18817`) | continues the `[0x4FC0]!=0` block (`0x1881E CALL 0xCE7A0`) |

### 1.1 Menu/competition path: `FUN_00018014` (`0x180AA`)

`FUN_00018014` body (`disassemble_bytes 0x18014 176`; the prologue is at
`0x18014`): `FUN_000CE7A0`, `FUN_000CBDB0(0)`, `FUN_00037A88`, `FUN_000372BC`,
then a device check (`FUN_0006D1B2`/`FUN_0006D1E5`) selecting
`FUN_0001D6F4`/`FUN_0001D8C4`/`FUN_0001D8F8` or
`FUN_0001D7CC`/`FUN_0001D8F8`/`FUN_0001D8C4`; `FUN_0001AD74`,
`FUN_0001ADF8`, `FUN_00033158(0)`, `FUN_00017F80`, `FUN_00019DC0`,
`FUN_00025E9C(0)`, `FUN_00025E9C(1)`, `FUN_000100C8`, `FUN_000100D8`,
`FUN_000334EC(1)`; then `XOR EAX,EAX; CALL 0x493A0` (`0x180A8..0x180AF`).
After the match: `if ([0x5094]==0) FUN_00017DF0();` (`0x180BA`).
`get_xrefs_to 0x18014` → three callers:

| caller site | context (byte-verified) |
|---|---|
| `0x20997` | inside the handler above `0x20868` (region: `FUN_0001D26C` confirm gate at `0x20876`, `[0x5094]=0` at `0x20885`, `CALL 0x20954` at `0x208A8`); `FUN_00020954` calls `FUN_00018014` when `FUN_00024B00()==4` (`0x20954..0x2099E` decompile) |
| `0x2FAF0` | `CALL 0x1B5B4`; `EAX=2; CALL 0x1B7B8`; `CALL 0x18014`; then `CMP EBP,[0x5094]` (`0x2FAE1..0x2FB0D`) |
| `0x32E10` | `EAX=4; CALL 0x1B7B8`; `CALL 0x18014`; `XOR EAX,EAX; TEST; JNZ` (`0x32E06..0x32E1B`) |

`FUN_0001B7B8` is the `[0x4C32A] = ([0x4C1D0]==4)` gate already characterised
in FU-60 §4.2; `[0x5094]` is the front-end confirm flag (FU-58 §3.2:
`FUN_0001D26C` confirm → `[0x5094]=1`). So this path is the menu-driven
"start a match" family; the enclosing handlers' screen identities are not
asserted (open legs).

### 1.2 Main pre-match path: `FUN_00018108` (`0x18359`)

`FUN_00018108` prologue `0x18108`; the head sets up the match screen
(`FUN_0001A0B8`, `FUN_0001A138`, `FUN_0001A21C`, `FUN_0001A04C`,
`FUN_0004AC6C`) and then waits (`0x1813B CALL 0x1A070` until nonzero), calls
`FUN_0001A280`, `FUN_0009FBC0(0)`, `FUN_00011B7C(0x1C,6)`,
`FUN_00017B40(200)`, then zeroes five bytes at `0x47E70` and enters a nested
gate chain on `FUN_0006844C`/`FUN_000180C4` (three consecutive zero returns
required, `decompile`). Inside that chain: `FUN_0001B7B8`? *(not in this
branch)*; the sequence is `FUN_000683C4`, `FUN_0001B0F0`, `FUN_0001A0F0`,
`FUN_00017B40(0x1E0)`, `FUN_0001A0F0`, `FUN_000CE7A0`, `FUN_0009FBC0(0)`,
`FUN_0001B7B8`, `FUN_0002E888`, the `FUN_00012258/48/38/28` zero pairs,
`FUN_00035A1C`, `FUN_00012010` ×2, `FUN_00018BD0`, `FUN_00018DF0` (three
`0x47E50/0x47E58/0x47E60/0x47E5C/0x47E6C/0x47E18` globals), `FUN_000122A0`,
`FUN_0001B794`, `FUN_00019F24`, `FUN_00019AD4`, `FUN_00018AF4`,
`FUN_000100C8`, `FUN_000100D8`, `FUN_000372BC`, `FUN_000334EC`,
`FUN_0001B008`, `MOV EAX,EBP; CALL 0x493A0` (`0x18357..0x1835D`,
`EBP=1` from `0x18169 MOV EBP,1`). Result: `TEST EAX,EAX; JZ 0x18170`
(re-enters the setup/loop region) else cleanup `FUN_000683C4`…`RET`
(`0x1839E`).

Its only caller is `FUN_00018680` at `0x186C2` (inside
`if ([0x4FC4]==0)`); `get_xrefs_to 0x18108` count 1.

### 1.3 Boot/main path: `FUN_00018680` (`0x18819`) and the startup

`FUN_00018680` is entered from `FUN_000B26B1` at `0xB26F1`
(`get_xrefs_to 0xB26B1` → sole caller `0x9FF9A`), i.e. from the resident
startup block (`CALL 0xB2700(0xFF)` at `0x9FF93`, then `CALL 0xB26B1` at
`0x9FF9A`; entry `0x9FD10` → `0x9FD88`, FU-4). `FUN_000B26B1` calls
`FUN_000BF52B`/`FUN_000BF534` then `FUN_00018680(…,0x82E9)` and
`FUN_000A1832`.

`FUN_00018680` body (decompile + `disassemble_bytes 0x18800 128`): it starts
with `FUN_00096C53(param_1,param_2)`, `FUN_0009FC98`, walks `param_2` with a
`'-'` test, calls `FUN_00017B78`; then `if ([0x4FC4]==0) FUN_00018108(); else
FUN_0004AC6C();`. After that, `if ([0x4FC0]!=0)` runs the full direct-start
block (the `FUN_0002E888`/`FUN_000122xx`/`FUN_00018xxx` sequence mirroring
§1.2) and `XOR EAX,EAX; CALL 0x493A0` at `0x18817..0x1881D`; then
`FUN_000CE7A0`, `FUN_0009FBC0(0)`, `FUN_00065D58`, `FUN_00011580`,
`FUN_0001770C`, `FUN_00034278`, `FUN_0004B014`, `FUN_0006D7EC`, resets the
`0x47E50`-family globals, `CALL [0x131A4]`. `FUN_00017DF0()` follows, then a
jump-table dispatch at `0x188C7` (table `0x8644`, index computed from
`[0x5090]`, `[0x5094]`, `[0x4FB8]` → values 3..7; jumptable warning in the
decompile; the target table is not decoded — open leg).

So the three entry families are: menu/competition (`FUN_00018014`), main
pre-match (`FUN_00018108`), and the boot flag block (`FUN_00018680`). The
`[0x4FC0]`/`[0x4FC4]`/`[0x5090]` flag meanings are not asserted (open legs).

## 2. The setup sequence `0x493A0`

Complete listing, decompile-backed and byte-corrected (the two mis-decoded
windows from §Method are quoted from `read_memory 0x49460` and
`read_memory 0x49510`):

```
0x493A0 PUSH EBX/ECX/EDX/ESI/EDI/EBP
0x493A6 MOV EDX,1
0x493AB MOV [0x72F8],EAX               ; match selector argument
0x493B0 XOR EBX,EBX
0x493B2 MOV [0x72FC],EDX               ; match active = 1
0x493B8 MOV [0x7330],EBX
0x493BE MOV [0x730C],EBX
0x493C4 MOV [0x7310],EBX
0x493CA CALL 0x4C904 / 0x493CF MOV [0x7314],EAX
0x493D4 CALL 0x43600 / 0x493D9 MOV [0x7318],EAX
0x493DE CALL 0xCB2A4                   ; 100 Hz tick getter
0x493E3 CALL 0x4C698                   ; arcade-coin RNG seed (FU-47 §2)
0x493E8 MOV EAX,0x18 / CALL 0x1D940    ; settings[0x18]
0x493F2 CALL 0x92AA0
0x493F7 MOV EAX,0x19 / CALL 0x1D940    ; settings[0x19]
0x49401 CALL 0x566A8
0x49406 CALL 0x73D10                   ; zero match state blocks
0x4940B CALL 0x4B020                   ; clock reset wrapper (CALL 0x73D90; JMP 0x73EE0)
0x49410 MOV EAX,1 / CALL 0x4B02C       ; clock reset, selector 1
0x4941A MOV EAX,1 / CALL 0x4A294       ; audio refresh (can stop voices)
0x49424 CALL 0x4AD4C                   ; screen buffer 0x280x0x1E0 + audio setup
0x49429 XOR EAX,EAX / CALL 0x36BC0     ; [0x5FFC] = 0
0x49430 CALL 0x4A228                   ; match reset (FU-62 §4.6; FUN_00091BC4)
0x49435 CALL 0x47928                   ; [0x7138] = [0x713C] = 0
0x4943A CALL 0x478FC                   ; FUN_000a1668(0,0x100) once
0x4943F MOV EAX,4 / XOR EDI,EDI / CALL 0x1D940   ; settings[4]
0x4944B CALL 0x443E8                   ; display block reset
0x49450 CALL 0x45390                   ; player/input init
0x49455 CALL 0x45D0D                   ; input/event/pace reset; ends hold or resume
0x4945A PUSH 0x39320                   ; callback FUN_00049320
0x4945F MOV [0x7324],EDI               ; EDI = 0, release pace hold
0x49465 CALL 0x9F64C                   ; register in 8-slot table 0x5BAE4
0x4946A MOV EBP,[0x72F8] / ADD ESP,4 / TEST EBP,EBP / JNZ 0x4949F
0x49477 XOR EAX,EAX / CALL 0x37798
0x4947E CALL 0x4B454 / TEST EAX,EAX / JZ 0x49495
0x49487 XOR EAX,EAX / CALL 0x36BC0 / CALL 0x37DE8 / JMP 0x4949A
0x49495 CALL 0x36BD8
0x4949A CALL 0x39054
0x4949F MOV ECX,4 / XOR EBX,EBX
0x494A6 MOV EAX,ECX / CALL 0x36BC8     ; [0x5FFC] == 4 ?
0x494AD TEST EAX,EAX / JNZ 0x49541     ; exit -> teardown (§4)
0x494B5 CALL 0x495B0                   ; match main loop (FU-60 §2.2)
0x494BA MOV EAX,2 / CALL 0x36BC8       ; [0x5FFC] == 2 ?
0x494C4 TEST EAX,EAX / JZ 0x49537
0x494CC CALL 0x4B5F4 / MOV EDX,EAX     ; period [0x57AC2] (FU-62 getter)
0x494D3 CMP EAX,2 / JC 0x494E4 / CMP EAX,3 / JBE 0x494F3 / CMP EAX,4 / JZ 0x494F3 / JMP 0x494F8
0x494E4 CMP EAX,1 / JNZ 0x494F8
0x494E9 CALL 0x4AF44 / CALL 0x4AF20    ; period == 1 arm
0x494F3 CALL 0x478FC
0x494F8 MOV [0x7310],EBX               ; cadence = 0
0x494FE CALL 0x53DE0 / 0x49503 CALL 0x45D0D / 0x49508 MOV EAX,EBX / CALL 0x4B02C
0x4950F CMP EDX,4 / JL 0x49523
0x49514 MOV EAX,0x200 / CALL 0x4C394   ; Q8 frame time (FU-62 §3.1)
0x4951E CALL 0x4B100                   ; world update (FU-62 §3.2)
0x49523 CALL 0x53A48
0x49528 CALL 0x36C70
0x4952D MOV EAX,3 / CALL 0x36BC0       ; [0x5FFC] = 3
0x49537 CALL 0x54018 / JMP 0x494A6
```

Byte-verified correction of the second window (`read_memory 0x49510`,
bytes `fa 04 7c 0f b8 00 02 00 00 e8 76 2e 00 00 e8 dd 1b 00 00 e8 20 a5 00
00 e8 43 d7 fe ff b8 03 00 00 00 e8 89 d6 fe ff e8 dc aa 00 00 e9 65 ff ff
ff …`): `0x4951E CALL 0x4B100`, `0x49523 CALL 0x53A48`, `0x49528 CALL 0x36C70`,
`0x4952D MOV EAX,3`, `0x49532 CALL 0x36BC0`, `0x49537 CALL 0x54018`,
`0x4953C JMP 0x494A6`. (The original listing shifts at `0x49520`, showing
only one `FUN_00053A48` call; the bytes have exactly one.)

The `[0x5FFC]==2` arm is the **post-period resolution**: period 1 runs
`FUN_0004AF44`/`FUN_0004AF20` (extra-time file arms), periods 2..4 run
`FUN_000478FC`, cadence `[0x7310]` is zeroed, `FUN_00053DE0`/`FUN_00045D0D`/
`FUN_0004B02C(0)` reset, a final `FUN_0004C394(0x200)`+`FUN_0004B100` pair
runs for period >= 4, then `FUN_00053A48`/`FUN_00036C70`/`[0x5FFC]=3` and
`FUN_00054018`; the outer loop then re-enters `FUN_000495B0`.

### 2.1 Initialization call semantics (evidenced one-liners)

| call | evidence |
|---|---|
| `FUN_0004C904` → `[0x7314]` | copies 6 dwords from `[0x7DC8]+0x10/+0x14/+0x18/+0x58/+0x5C/+0x4C` to `0x4E4E0` and returns `&0x4E4E0` (decompile) |
| `FUN_00043600` → `[0x7318]` | `return &DAT_0004B0A4` (decompile) |
| `FUN_000CB2A4` | tick getter `return [0x12E88]` (FU-58 §5) |
| `FUN_0004C698` | arcade-coin RNG seeder, quoted in FU-47 §2 (`0x493E3` site listed there) |
| `FUN_0001D940(i)` | `return *(dword*)(&DAT_00049278 + i*4)` — the settings block copied to `0x49210` (FU-58 §6); called with 0x18, 0x19, 4 |
| `FUN_00092AA0` / `FUN_000566A8` | seed 6 dwords at `0x10F44` / `0x8FD0` with `(int)string_byte + EAX*0x2000000` from the byte strings at `0x2D70` / `0x1E54` (decompile; EAX = preceding `FUN_0001D940` result) |
| `FUN_00073D10` | zeroes the match state blocks `0x588A4..0x58E31`, `0x57A4C..0x57AB3`, `0x587EC..0x5888A`, `0x57AB4..0x57C29`, `0x57710..0x5773E`, `0x5758C..0x5770E`, `0x57740..0x5782A` (decompile; exact lengths from the listing) |
| `FUN_0004B020`/`FUN_0004B02C` | clock reset wrappers: `0x4B020` = `CALL 0x73D90; JMP 0x73EE0` (`read_memory 0x4B020`: `e8 6b 8d 02 00 e9 b6 8e 02 00`); `0x4B02C` zeroes `[0x57AB3]`/`[0x57A4F]`/`[0x57A5C]`/`[0x57A64]` and selects a reset path on EAX (`0x4B02C..0x4B0B0`, hand-decoded) |
| `FUN_0004A294(1)` | audio refresh; when `[0x4C32A]==0` and the event conditions hold, calls `FUN_00067948` (decompile) |
| `FUN_0004AD4C` | allocates the `0x280x0x1E0` screen buffer, sets `[0x7360]/[0x7364]=6`, `FUN_00066C1C`, audio/CRD setup (decompile) |
| `FUN_00036BC0(0)` | `[0x5FFC]=0` (one-line function; FU-60) |
| `FUN_0004A228` | match reset incl. `FUN_00091BC4` ambience init (FU-49 §1.10/FU-62 §4.6) |
| `FUN_00047928` / `FUN_000478FC` | `[0x713C]=[0x7138]=0`; one-shot `FUN_000a1668(0,0x100)` |
| `FUN_000443E8` | display/UI mode reset: `FUN_0001DA58` ×2, loads `[0x6708]/[0x670C]` from tables at `0x67B4`/`0x67BC`, zeroes the `0x6700..0x673C` block, `FUN_000441D8`, `FUN_00043DFC`, frees `[0x6740]/[0x6768]`, `FUN_000cb2ff(10)`, `FUN_000439D0`, `FUN_000cb3fb` (decompile) |
| `FUN_00045390` | player/input init: `[0x6803] = FUN_0006D1B2()`, `[0x67FF] = FUN_0006D1E5()`, zeroes the `0x4B19C+0x14*i` per-player blocks, `[0x6812]=0x3C`, `FUN_000cb394`, `[0x6807]=FUN_0006D510()/2` (decompile) |
| `FUN_00045D0D` | input/event/pace reset: zeroes `0x4B1D0`/`0x4B1C8` arrays and the per-player `0x4B1A0` blocks, `FUN_000432EC`, `FUN_000492D4`, sets `[0x6816]`, and ends with `FUN_00049308` (hold) when `FUN_00037B24()` (mode `[0x4B018]==7`) is zero else `FUN_00049314` (resume); zeroes the pace block through `FUN_000492E8` at `0x45D9A` (decompile) |
| `PUSH 0x39320` / `FUN_0009F64C` | registration: finds the first null entry of the 8-slot table at `0x5BAE4` and stores the callback (decompile; FU-58 §5) |
| `[0x7324]=0` | explicit pace release before registration (FU-60 §1) |

Selector-0 tail: `FUN_00037798(0)` (`[0x4B028]=0`), `FUN_0004B454`
(`[0x4C32A]`); if nonzero → `[0x5FFC]=0` via `FUN_00036BC0` + `FUN_00037DE8`,
else `FUN_00036BD8` (sets `[0x5FFC]` from `FUN_00037DAC`); then
`FUN_00039054` (screen-state machine, `[0x5FFC]` reads/writes at
`0x3905E..0x391A5`).

### 2.2 The 30 Hz registration

`FUN_00049320` (FU-60 §1) is registered by pointer `0x39320` (object-relative
encoding; FU-59 errata), i.e. the callback runs at 100 Hz and grants 30 Hz
frames into `[0x731C]`. The setup's `PUSH 0x39320` is fixup-verified: the two
records targeting flat `0x49320` have image sources `0x4945B` (setup) and
`0x49556` (teardown), the encoded-immediate locations.

## 3. The frame call site `0x496D5` and the `FUN_0006400C` gate

* `0x496D5 CALL 0x49B28` is the drain call inside `FUN_000495B0`'s per-frame
  body (FU-60 §2.2/§3); it is the "frame body" edge FU-58 §7 slice 9 named,
  not the match entry. The body gates before it: `FUN_00043B48`,
  `FUN_000478D0`, `FUN_00049280(EBX)`, `FUN_0004A178` (returns 1 on
  mode/pause and skips), then `CALL 0x6400C` at `0x496C8`, `CALL 0x49B28` at
  `0x496D5`, `FUN_00036C3C`, `FUN_0004B378`, … (FU-60 §2.2 listing).
* `FUN_0006400C` decompiles to `return DAT_00009a98 == 0`. `[0x9A98]` is the
  async-load/busy word: `FUN_00064270` sets it to `0x81` (start),
  `FUN_00064E8C` sets it to 0 (finish) and `FUN_000642FC` writes it in the
  load-state arms; four more write sites (`FUN_00063CBC 0x63D27`,
  `FUN_00063EBC 0x63F41`, `FUN_00064DFC 0x64E1F/0x64E67`, `0x63EAE`,
  `0x63FC5`) are the 0x64xxx load manager. Both the drain frame path
  (`0x49F97`) and the display arms (`0x49613`, `0x496C8`, `0x49717`,
  `0x49779`) test the gate, so no frame work runs while a load is in flight.
  (FU-60 already listed the six call sites; this slice identifies the
  start/finish writers; the full load-state map remains open.)

## 4. Exit and teardown

### 4.1 Leave staging inside `FUN_000495B0` (`0x49787..0x49816`)

Disassembled this slice (the region FU-60 §2.2 summarised):

```
0x49787 CMP EBX,[0x72F8] / JZ 0x497B2           ; selector != match selector?
0x4978F MOV EAX,5 / CALL 0x36BC8 / JNZ 0x497AB
0x4979D MOV EAX,7 / CALL 0x36BC8 / JZ 0x497B2  ; [0x5FFC] in {5,7}
0x497AB MOV EAX,ECX / CALL 0x36BC0              ; [0x5FFC] = ECX
0x497B2 MOV EAX,EBX / CALL 0x4A294
0x497B9 CALL 0x4557C / TEST EAX,EAX / JZ 0x497E8
0x497C2 CALL 0x6D1B2 / CMP EAX,1 / JNZ 0x497D3
0x497CC CALL 0x45D0D / JMP 0x497E8              ; device == 1 arm
0x497D3 MOV EAX,4 / MOV EDX,3 / CALL 0x36BC0    ; [0x5FFC] = 4
0x497E2 MOV [0x7304],EDX                        ; [0x7304] = 3
0x497E8 CMP EBX,[0x7304] / JZ 0x495D1           ; continue while selector == target
0x497F4 CALL 0x4557C / TEST / JZ 0x49812
0x497FD MOV EAX,4 / MOV EDI,3 / CALL 0x36BC0 / MOV [0x7304],EDI
0x49812 POP EDI/ESI/EDX/ECX/EBX / RET
```

So the loop leaves by staging `[0x5FFC]=4` (exit) and `[0x7304]=3` (leave
target), gated by `FUN_0004557C` and the `FUN_0006D1B2()!=1` device check.
`FUN_00036C3C` also maps `[0x5FFC]==6` to `4` (`0x36C3C..0x36C60`), i.e. state
6 is another exit spelling.

### 4.2 The period-end stop `[0x58822]`

* `[0x58822]` is the word FU-62 §4.1 called the match-over reason.
  `get_xrefs_to 0x58822`: writes `FUN_000886D4 0x8871B` (reset),
  `0x88D42` (reset block: zeroes `[0x57AB6]`, `[0x57ABA]`, `[0x5882D]`,
  `[0x58822]`, `[0x58818]`, `[0x58828]`, `[0x58808]`, then reads
  `[0x57AC2]`), `FUN_0008BAF0 0x8BCD9` (=1), `0x8BD8A`/`0x8BDD4` (=2),
  `FUN_0004B100 0x4B29B` (=0); reads `FUN_0004B100 0x4B106`,
  `FUN_0008AF38 0x8B678`, `FUN_0008BAF0`.
* `FUN_0008BAF0` end conditions (`disassemble_bytes 0x8BC80 360`):
  `if ([0xF378]!=0) { FUN_00037F54(); [0xF378]=0; [0x58818]=0; }` else
  `FUN_00045001` gate; `if ([0x57AC0]==0) [0x58822]=1; else
  FUN_000740A0(phase 0x13, 0)`; for phase in `{0x13,0x14}` it accumulates
  `[0x58818] += [0x57A64]`, writes `[0x58830]=0x780`, and sets
  `[0x58822]=2` when the `[0x58814]`/`[0x58816]` distance gates hold,
  followed by `[0xF360]=1`. `FUN_0008B9CC` remains the phase-transition funnel
  (FU-62 §1.2/§4.5) called by the period rollover (`0x8B574`) and this
  function.
* `FUN_0004B100` head (`disassemble_bytes 0x4B100 432`): `if
  ([0x58822]!=0) return 1;` (clock/world frozen); otherwise it splits the Q8
  accumulator (FU-62 §3.2), calls the clock machine `FUN_0008AF38`
  (`0x4B1A6`) and branches on its status: the period-end arm at `0x4B244`
  executes `MOV EAX,2; CALL 0x36BC0` → **`[0x5FFC]=2`** (the match-over screen
  transition FU-62 did not name), and the status-2 arm clears `[0x58822]=0`
  at `0x4B297..0x4B29E`.
* `[0x57AC2]++` is the period rollover `0x8B58A` (FU-62 §4.5), reached from
  the period-end case block after `FUN_0008B9CC` (`0x8B574`) and the
  `[0x57AB6]=0` write (`0x8B583`).

### 4.3 Teardown `0x49541..0x495AF`

```
0x49541 CALL 0x45D0D                   ; input/event/pace reset
0x49546 MOV dword [0x7324],1           ; hold the 30 Hz pace
0x49550 CALL 0x45D0D
0x49555 PUSH 0x39320
0x4955A CALL 0x9F684                   ; cancel callback slot (first match)
0x4955F ADD ESP,4
0x49562 CALL 0x45D0D
0x49567 CALL 0x4AEA4                   ; match teardown body (below)
0x4956C CALL 0x67948                   ; audio stream/voice cleanup
0x49571 XOR EDX,EDX / MOV ECX,[0x7304]
0x49579 MOV [0x72FC],EDX               ; active = 0
0x4957F MOV [0x1471C],EDX              ; unload pointer = 0
0x49585 CMP ECX,3 / JNZ 0x4959A
0x4958A MOV ESI,2 / CALL 0x6D7EC / MOV [0x7304],ESI   ; post-exit select
0x4959A CMP dword [0x7304],2 / SETZ AL / AND EAX,0xFF
0x495A9 POP EBP/EDI/ESI/EDX/ECX/EBX / RET
```

`FUN_0009F684` clears the first slot equal to the argument (`decompile`;
`0x4955A` is one of six callers). `FUN_00067948` frees the audio handles in
the `[0xA274]`-counted array at `0x55F70` and clears `[0x55D40]` (the
voice/music teardown; FU-60 §4.2's music start is `FUN_000652B8`, called from
the presentation pump, not here). `FUN_0006D7EC` (`in_EAX` = 2) runs the
`FUN_00065B70`/`FUN_0006C55C`/`FUN_000CB2FF(100)`/`FUN_0006D7AE` music-select
sequence when `[0x57538]!=0` (decompile). The return value is
`[0x7304]==2`.

`FUN_0004AEA4` is not a Ghidra function (the `0x4Axxx` gap after
`FUN_00049B28` is undefined); `read_memory 0x4AEA4` hand-decodes to:

| site | target | note |
|---|---|---|
| `0x4AEA4` | `0x38448` | leave: `FUN_0001A024`, `FUN_00048FF4` (match-data unload: `[0x68E0]` guard, `FUN_000479A0`/`FUN_000659F8`/`FUN_00047A70`/`FUN_0005F500`/`FUN_0005AE64`/`FUN_00047184`/`FUN_00019B44`, `[0x1471C]=0`), `[0x4AFF8]=1`, `FUN_0003749C`, `FUN_00044E3C`, `FUN_00044F24`, `FUN_000432BC`, `FUN_0001D940` (decompile) |
| `0x4AEA9` | `0x78F00` | |
| `0x4AEAE` | `0x54018` | stores `FUN_0004AFA0()` to `[0x8DF4]` (decompile) |
| `0x4AEB3` | `0x63628` | |
| `0x4AEB8` | `0x63930` | |
| `0x4AEBD` | `0x65600` | |
| `0x4AEC2` | `[0x7360]==0` → `0x4AED0`/`0x4AEDA` `0x66CC0` with `[0x7364]`/`[0x7368]` | |
| `0x4AEDF`/`0x4AEE4`/`0x4AEE9` | `0x66E28` / `0x66BD0` / `0x66AF8` | |
| `0x4AEEE` | `0x43D94` | last `CALL` before `XOR EAX,EAX` |
| `0x4AEF5` | `0x4A830` | `EAX=0` |
| `0x4AEFA` | `0x4B454` | `[0x4C32A]` getter |

Ordering is the evidenced part (the callees' internals are open legs).
`[0x1471C]` is set by the match-data load (`FUN_00048ED8 0x48FDB` =
`0x3783C`) and zeroed here and by `FUN_00048C8C`/`FUN_00048C6C`
(`get_xrefs_to 0x1471C`), so `unload=0` is the "match data released" marker.

### 4.4 Where control returns

`0x495AF RET` returns to the callers of §1:

* `FUN_00018014` (`0x180AF`): `if ([0x5094]==0) FUN_00017DF0();` — the
  front-end screen restore (clears the screen, restores menu sprites; big
  `decompile`).
* `FUN_00018108` (`0x1835E`): `TEST EAX,EAX; JZ 0x18170` (loop back into the
  pre-match wait/setup region) else the `FUN_000683C4`… cleanup and `RET`.
* `FUN_00018680` (`0x1881E`): continues the direct-start block
  (`FUN_000CE7A0`, `FUN_0009FBC0(0)`, `FUN_00065D58`, `FUN_00011580`,
  `FUN_0001770C`, `FUN_00034278`, `FUN_0004B014`, `FUN_0006D7EC`, the
  `0x47E50` resets, `CALL [0x131A4]`), then `FUN_00017DF0` and the
  `0x8644` dispatch.

So the AL return (`[0x7304]==2`) only steers `FUN_00018108`; the other two
callers ignore it and proceed with their own post-match sequence.

## 5. State map (fields cited this slice)

| address | width | evidence | role (evidenced only) |
|---|---|---|---|
| `0x72F8` | dword | `0x493AB` write, `0x4946A`/`0x49787` read, `FUN_0004A178` read | match selector argument; `0` from menu and boot, `1` from `FUN_00018108` |
| `0x72FC` | dword | `0x493B2`=1, `0x49579`=0, getter `0x49398` | match-active flag |
| `0x7304` | dword | `0x497E2`/`0x4980C`=3, `0x4958A..0x49594`=2, `FUN_0004A178` reads | loop target / post-exit selector |
| `0x730C` | dword | `0x493BE`=0 | previous selector copy (zeroed at start) |
| `0x7310` | dword | `0x493C4`=0, `0x494F8`=0, `FUN_00049B28` cadence | phase-cadence counter (FU-60) |
| `0x7314`,`0x7318` | dword | `0x493CF`/`0x493D9`; `0x495F8` (FU-60) | camera snapshot ptr / display object ptr |
| `0x7324` | dword | `0x4945F`=0, `0x49546`=1, `FUN_00049308/14` | pace hold (FU-60 §1) |
| `0x7330` | dword | `0x493B8`=0, `0x49C38` | flag set when `FUN_00047878` reports (FU-60 §2.3) |
| `0x5FFC` | dword | `FUN_00036BC0` write, `FUN_00036BC8` compare | match screen state: 0 active (`0x4942B`), 2 over (`0x4B249`), 3 post (`0x49532`), 4 exit (`0x497DD`, `0x49807`), 5/7 checked at leave (`0x4978F/0x4979D`), 6→4 (`FUN_00036C3C 0x36C60`) |
| `0x58822` | word | `0x8BCD9`=1, `0x8BD8A`/`0x8BDD4`=2, `0x4B29B`=0, `0x88D42`=0 | match-over reason; freezes `FUN_0004B100` (`0x4B106`) and `FUN_0008AF38` (`0x8B678`) |
| `0x57AC2` | byte | `0x8B58A` `INC`, `FUN_0004B5F4` getter | period index, read by the `[0x5FFC]==2` resolution |
| `0x1471C` | dword | `0x4957F`=0, `FUN_00048ED8 0x48FDB`=0x3783C, `FUN_00048C8C/6C` | match-data release marker |
| `0x9A98` | dword | `FUN_0006400C` (`==0`), `FUN_00064270`=0x81, `FUN_00064E8C`=0 | async-load busy word gating the frame path |
| `0x4FC0`,`0x4FC4`,`0x5090`,`0x4FB8`,`0x5094` | dwords | `FUN_00018680`/`FUN_00020954` reads+writes | boot/menu flow flags; meanings not asserted (open) |
| `0x5BAE4` | 8×dword | `FUN_0009F64C`/`FUN_0009F684`/handler `0x9F5E4` | INT-8 callback table (FU-58 §5) |
| `0x68E0` | dword | `FUN_00048FF4` guard, `FUN_00048ED8` sets 1 | match-data-loaded flag |
| `0x1471C`/`0x3783C` | dword | see above | loaded match-data size/pointer |

## 6. Port: `fifa96_match_lifecycle`

`include/fifa96_loader/fifa96_match_lifecycle.h` +
`src/fifa96_loader/fifa96_match_lifecycle.c` (caller-owned struct, no globals,
no comments, `-fifa96_err_t`; `FIFA96_ERR_STATE` added to `fifa96_err.h` for
wrong-state calls). Scope: the evidenced start/stop/exit state transitions and
the teardown ordering over caller-owned state and callbacks. The init calls
of §2.1 are not modelled (they are app-side loaders/allocators); their bound is
the `begin` ordering: bookkeeping reset → pace reset/resume → register.

| original | port |
|---|---|
| `0x493AB` `[0x72F8]=EAX` | `selector` |
| `0x493B2` `[0x72FC]=1` / `0x49579` `=0` | `active` |
| `0x493B8`/`0x493BE`/`0x493C4` zero `[0x7330]`/`[0x730C]`/`[0x7310]` | `flag`/`prev`/`cadence` zeroed by `begin` |
| `0x49429` `[0x5FFC]=0` | `screen = FIFA96_MATCH_SCREEN_ACTIVE` |
| `0x495C6` `[0x7304]=0` (loop setup) + leave staging `0x497E2`/`0x4980C`=3 | `target`, `fifa96_match_lifecycle_request_exit` (=4/3) |
| `0x45D9A` pace zero + `0x4945F` `[0x7324]=0` | `fifa96_match_pace_init` + `_resume` inside `begin` |
| `0x49465` `FUN_0009F64C(0x39320)` | `backend->register_callback` |
| `0x4B244..0x4B249` `[0x5FFC]=2` | `fifa96_match_lifecycle_mark_over` |
| `0x494CC..0x49532` period resolution (state part) + `[0x7310]=0` + `[0x5FFC]=3` | `resolve_over` (returns 1; period-file/audio side effects not ported) |
| `0x494A6` outer check `[0x5FFC]==4` | `should_exit` |
| `0x49546` `[0x7324]=1` | `_hold` inside `end` |
| `0x4955A` `FUN_0009F684(0x39320)` | `backend->cancel_callback` |
| `0x49567`/`0x4956C` `FUN_0004AEA4` + `FUN_00067948` | `backend->teardown` |
| `0x4957F` `[0x1471C]=0` | `unload = 0` |
| `0x49585..0x49594` `[0x7304]==3` → `FUN_0006D7EC(2)`, `[0x7304]=2` | `backend->post_exit` + `target=2` |
| `0x4959A..0x495A4` `SETZ AL` on `[0x7304]==2` | `end` return value |
| callback error propagation | original callbacks are void (`FUN_0009F64C`/`FUN_0009F684`); the port gives them `int` returns and propagates the first negative error after the sequence completes; `register` failure aborts `begin` and holds the pace |

Not ported (open legs): the 26 init callees, the `FUN_0004557C`/`FUN_0006D1B2`
leave gate, the period-resolution callees, `FUN_0004B100`'s period-end arm
(already in `fifa96_match_state`'s domain), the `[0x4FCx]` flags, and the
callers' post-return sequences.

Tests (`tests/test_match_lifecycle.c`, suite 52 → **53**): init zeroing; begin
stores selector/active and zeroes screen/target/prev/cadence/flag, resets and
resumes the pace and registers once; register callback sees
`active==1,registered==0,hold==0,pending==0` (ordering); double begin →
`-FIFA96_ERR_STATE` with no second register; register failure → error with
`active=0,registered=0,hold=1`; `mark_over`→2 / `resolve_over`→3 with cadence
zeroing and idempotent second call; `request_exit`→4/target=3 and
`should_exit`; end ordering (hold before cancel, cancel before teardown,
teardown sees `registered==0,unload==0`, post-exit only on target 3, return
1); end without exit (no post-exit, return 0); end without begin →
`-FIFA96_ERR_STATE` with no callbacks and no hold; three begin/end cycles stay
balanced (3 register/3 cancel/3 teardown/3 post); teardown error propagates
while cancel/teardown/post all run and the state settles; NULL handling for
every entry point (post-exit may be NULL). ASan+UBSan build of
`test_match_lifecycle` clean (`cc -fsanitize=address,undefined -Iinclude
tests/test_match_lifecycle.c src/fifa96_loader/fifa96_match_lifecycle.c
src/fifa96_loader/fifa96_match_pace.c`).

## 7. Errata (quoted)

* FU-60 §9 open leg "**Setup caller unknown**: `0x493A0` has no xref and no
  operand match (`get_xrefs_to` 0, `search_instructions` `493a0` 0)" —
  **closed/explained**: three `E8 rel32` callers exist at `0x180AA`, `0x18359`
  and `0x18819`; Ghidra misses them because the target regions are undefined
  (no instructions, so no references and no operand search hits). The full
  fixup-stream parse also shows no data-table reference. FU-47 §2 had already
  found the sites with a raw scan; this slice places them.
* FU-58 §7 slice 9 "Match start path (open) — no anchor yet: the caller chain
  into the `0x49xxx` module (call site `0x496D5`)" — **closed**: the chain is
  §1, and `0x496D5` is the drain call inside the match loop, not the entry.
* FU-60 §2.1 listing: the `0x4946A..0x4949A` window disassembles as `POP DS` /
  `ADD [EAX],AL` garbage; the true stream is `MOV EBP,[0x72F8]; ADD ESP,4;
  TEST EBP,EBP; JNZ 0x4949F; XOR EAX,EAX; CALL 0x37798; CALL 0x4B454;
  TEST EAX,EAX; JZ …` (`read_memory 0x49460`). FU-60's summary of the tail was
  right; only the operand decode was broken.
* FU-60 §2.1 listing: the `0x49520..0x4954F` window is likewise shifted; the
  true stream is `CALL 0x4B100` (`0x4951E`), `CALL 0x53A48` (`0x49523`),
  `CALL 0x36C70` (`0x49528`), `MOV EAX,3; CALL 0x36BC0` (`0x4952D`),
  `CALL 0x54018` (`0x49537`), `JMP 0x494A6` (`0x4953C`) — so there is one
  `FUN_00053A48` call, not two, and the `AND AL,0x73` bytes at `0x49548` are
  the `MOV dword [0x7324],1` immediate.
* FU-62 §4.1 table row `0x58822` "match-over reason; `FUN_0004B100` returns
  early when non-zero" and §4.6 "FUN_0008BAF0 sets `[0x58822]` to 1 or 2" —
  **extended**: exact sites `0x8BCD9` (=1, `[0x57AC0]==0`), `0x8BD8A`/`0x8BDD4`
  (=2, phase 0x13/0x14 arms, gated by `[0xF360]==0`), reset `0x88D42`;
  `[0x58822]` is cleared by `FUN_0004B100` at `0x4B29B`, and the same function
  sets `[0x5FFC]=2` at `0x4B244..0x4B249` on the period-end arm (the screen
  transition that makes `0x493A0` run the period resolution).
* FU-60 §1 "`[0x7324]` … set to 1 at teardown (`MOV dword [0x7324],1` at
  `0x49546`)" — **confirmed**; add that `FUN_00045D0D` brackets the hold and
  the `FUN_0009F684` cancel (`0x49541`, `0x49550`, `0x49562`), and that
  `FUN_00045D0D` itself zeroes the pace block via `FUN_000492E8` (`0x45D9A`).
* FU-58 §5 "`0x9F64C` registers a callback in the first empty slot" —
  **confirmed and used**: the setup's `PUSH 0x39320` is one of exactly two
  fixup-relocated copies of the callback address (the other is the cancel at
  `0x49555`).
* FU-60 Method "on-disk `/tmp/opencode/fifa96_le.bin` no longer matches the
  Ghidra program" — **confirmed again**: raw scans must run on the image
  rebuilt from `game/FIFAPCCD96.iso`; the searched bytes at `0x180AA` differ
  between the stale file and the Ghidra program.

## 8. Open legs

* The three entry handlers' screen identities and the `[0x4FC0]`/`[0x4FC4]`/
  `[0x5090]`/`[0x4FB8]` flag meanings: `FUN_000B26B1`'s `0x82E9` argument and
  `FUN_00096C53` are not decomposed; no mode/state label is asserted.
* `FUN_000180C4` (the three-call gate in `FUN_00018108`), `FUN_0001B008`,
  `FUN_000334EC`, `FUN_0006844C`, `FUN_00017F80` are characterised only by
  call position.
* `FUN_0004AEA4`'s callees (`0x78F00`, `0x63628`, `0x63930`, `0x65600`,
  `0x66CC0`, `0x66E28`, `0x66BD0`, `0x66AF8`, `0x43D94`, `0x4A830`) are not
  decomposed; only the ordering is claimed.
* `[0x1471C]`'s consumer `FUN_000A21FC` (read + indirect call) is not
  derived.
* The `[0x5FFC]` states 5 and 7 (checked at leave) have no writer found in
  this slice; state 1/5/6/7 producers are unmapped.
* `FUN_0006400C`'s `[0x9A98]` writer set (async load manager `0x63xxx/0x64xxx`)
  is not fully decoded; `FUN_00064270`/`FUN_00064E8C` are quoted as
  start/finish.
* Period-resolution side effects (`FUN_0004AF44`/`FUN_0004AF20` for period 1,
  `FUN_00053DE0`, `FUN_0004B02C`, `FUN_00053A48`, `FUN_00054018`) are not
  ported or semantically labelled.
* `FUN_0004557C`/`FUN_0006D1B2` leave-gate semantics (device == 1 arm) are not
  decomposed.
* `[0x72F8]` selector semantics (0 vs 1) are not asserted beyond the call
  sites' values.
* Object-base classification of each quoted global (FU-59 errata) was not
  re-run per address; addresses are quoted as Ghidra displays them.

## Provenance

Ghidra MCP on `/fifa96_le.bin`:
`decompile_function` 0x18014, 0x18108, 0x18680, 0xB26B1, 0x20954, 0x17DF0,
0x1D940, 0x4C904, 0x43600, 0x92AA0, 0x566A8, 0x73D10, 0x4A294, 0x4AD4C,
0x47928, 0x478FC, 0x443E8, 0x45390, 0x45D0D, 0x36BC0, 0x36BC8, 0x36BD8,
0x36C0C, 0x36C3C, 0x37798, 0x37DAC, 0x39054, 0x9F64C, 0x9F684, 0x4B454,
0x64270, 0x64E8C, 0x67948, 0x6D7EC, 0x4AF20, 0x54018, 0x8BAF0 (via region);
`disassemble_function` 0x1B5B4, 0x36C0C;
`disassemble_bytes` 0x493A0 (528 B), 0x48E00 (1440 B), 0x49780 (151 B),
0x4B100 (432 B), 0x8BC80 (104 B), 0x8BCC0 (296 B), 0x88D20 (64 B),
0x490F4 (45 B), 0x4B020 (80 B), 0x18040 (224 B), 0x182E0 (208 B), 0x187A0
(208 B), 0x20860 (112 B), 0x2FAE0 (48 B), 0x32E00 (48 B), 0x9FF60 (80 B);
`read_memory` 0x493A0, 0x49380, 0x49460, 0x49510, 0x4AEA4, 0x4B01C, 0x4B0A0,
0x143DC, 0x14449, 0x180A0, 0x26BEC;
`get_xrefs_to` 0x493A0, 0x4955A, 0x495B0, 0x9F684, 0x6400C, 0x5FFC, 0x9A98,
0x58822, 0x1471C, 0x18014, 0x18108, 0x18680, 0xB26B1, 0x4A228, 0x4A294,
0x4AD4C, 0x48ED8, 0x48FF4, 0x72F8, 0x20954;
`get_function_by_address` 0x493A0, 0x18014, 0x18108, 0x18680, 0x20954,
0x4AEA4, 0x2FAE1, 0x32E06, 0x9FF9A, 0x208A8;
`search_functions` FUN_00017, FUN_00018; `get_current_program_info`;
off-Ghidra (analysis-only, `/tmp/opencode/fu64/`): the LE fixup parser
(32,461 records, targets resolved) and the full `E8`/`E9` rel32 scan of the
image rebuilt from `game/FIFAPCCD96.iso`'s `FIFA96.EXE` via `tools/fifa96_le.py`
(3 hits for `0x493A0`, 1 for `0x495B0`, 2 fixup targets `0x49320`).

Analysis-only: no tool, capture-rig, ISO or Ghidra-project change staged. Port
write set: `include/fifa96_loader/fifa96_match_lifecycle.h`,
`src/fifa96_loader/fifa96_match_lifecycle.c`, `include/fifa96_loader/fifa96_err.h`
(one enum value), `tests/test_match_lifecycle.c`, `CMakeLists.txt` (one
library/test block). `make test`: 52/52 before, **53/53 after**; ASan+UBSan
`test_match_lifecycle` clean. `game/FIFAPCCD96.iso` untouched; `fifa96.rep/**`
churn not staged.
