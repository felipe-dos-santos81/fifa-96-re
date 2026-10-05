# FU-60: the match frame body — `FUN_00049B28`, `FUN_00091DD8` and the `0x49xxx` loop

Roadmap slice #2 of FU-58 §7. Derives what actually runs per frame during a
match: the 30 Hz frame pace granted by the registered INT-8 callback
`FUN_00049320`, the drain loop `FUN_00049B28` and the presentation pump
`FUN_00091DD8`, plus the match setup/loop wrappers around them in the undefined
`0x49xxx` module. One clean self-contained piece was ported (the frame-pace
state machine); everything else is mapped as bounded sub-slices.

Result in one line: **the match frame body is a 30 Hz packet drain — the INT-8
callback `FUN_00049320` adds `0x102` per 100 Hz tick to `[0x7320]` and grants a
frame every time the accumulator reaches `0x35C` (`258/860 = 3/10`, exactly
30.0 Hz), incrementing `[0x731C]` and calling the input sampler `FUN_000456AF`;
the match setup `0x493A0` registers that callback, runs `FUN_000495B0`, which
loops `FUN_00049B28` to drain `[0x731C]` frames — each drained frame runs
`FUN_0004C394(0x200)` → `FUN_0004B100` (match update) → `FUN_00036208(0x200)`
(motion/camera) → `FUN_00091DD8` (presentation/event pump) under two gates,
with the event-code dispatcher `FUN_00045F6A`/`FUN_000451F1` selecting the frame
action — and `FUN_00091DD8` is a presentation pump, not a renderer: it starts
music, runs the FU-49 ambience machine when `[0x4C32A]==0`, and advances the
3-state, 6-slot presentation-event queue at `0x5B380` (no blit/VGA callee).**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58/FU-59). All
  instructions quoted below were read back from Ghidra this slice
  (`disassemble_function`/`disassemble_bytes`/`read_memory`); the decompiler
  was used for callee characterisation where it succeeded.
* **Address convention (FU-59 errata, quoted):** code immediates naming
  callbacks/tables/globals are object-relative values carrying LE fixups, so the
  loader adds the owning object's base at run time; this document quotes the
  encoded operands exactly as Ghidra displays them, as FU-58/FU-59 do. The
  on-disk `/tmp/opencode/fifa96_le.bin` no longer matches the Ghidra program
  (FU-59 Method), so nothing here is re-read from disk.
* **Ghidra function-boundary artifact (new, this slice):** Ghidra's auto-analysis
  merged the interleaved helpers into one function at `FUN_00091dd8`
  (`get_function_by_address 0x91DD8` → body `0x91DD5..0x924E7`, entry
  `0x91DD8`), while also defining the nested helpers separately
  (`FUN_00091E64` `0x91E64..0x91F5D`, `FUN_00091F60` `0x91F60..0x9203F`,
  `FUN_00092040` `0x92040..0x920BC`, `FUN_000920C0` `0x920C0..0x92191`,
  `FUN_00092194` `0x92194..0x92199`). The decompiler **died** on `0x91DD8`
  ("Decompiler process died"); the body below is line-decoded from
  `disassemble_function 0x91DD8` and raw-byte reads. The true body splits as
  `0x91DD8..0x91E5E` (entry/gate/scheduling) and `0x921BB..0x924E7` (tail);
  control jumps over the helper alphabet (`0x91E5E JMP 0x921BB`,
  `0x91E37 JGE 0x921ED`). `0x91DD5` is the previous function's `RET`
  (bytes `0x91DD0: 05 00 5a 59 5b c3`).
* The scheduler `FUN_00049B28` sits in the undefined `0x49xxx` gap; its call
  graph and byte stream were recovered with `disassemble_function` plus
  raw-byte hand-decode of the two mis-decoded windows inside it
  (`0x49FD8..0x49FF3` and the `0x49E60..0x49F96` chain ends are fine; the
  gaps were `0x49FD8` and `0x49FE6`), and the enclosing setup/loop region was
  swept with `disassemble_bytes 0x49300` (564 instructions,
  `0x49300..0x49B27`) and `disassemble_bytes 0x49000` (254 instructions).

## 1. The frame pace: `FUN_00049320` (encoded callback `0x39320`)

Complete listing (`disassemble_function 0x49320`; 23 instructions):

```
0x49320  PUSH EBX / PUSH ESI / PUSH EDI
0x49323  CMP dword [0x7324],0          ; pace hold flag
0x4932A  JNZ 0x4936D                   ; held -> return
0x4932C  CALL 0x3F684                  ; FUN_0003f684() = ([0x4B018] == 0x18)
0x49331  TEST EAX,EAX
0x49333  JNZ 0x4936D                   ; excluded mode -> return
0x49335  MOV EBX,[0x7320]
0x4933B  ADD EBX,0x102
0x49341  MOV [0x7320],EBX
0x49347  CMP EBX,0x35C
0x4934D  JL 0x4936D                    ; below frame size -> return
0x4934F  MOV EDI,[0x731C]
0x49355  LEA ESI,[EBX + 0xfffffca4]    ; EBX - 0x35C
0x4935B  INC EDI
0x4935C  MOV [0x7320],ESI              ; subtract one frame
0x49362  MOV [0x731C],EDI              ; grant one frame
0x49368  CALL 0x456AF                  ; per-granted-frame input sampler
0x4936D  POP EDI / POP ESI / POP EBX / RET
```

* `0x102` = 258 and `0x35C` = 860; `258·10 = 3·860`, so the callback grants
  exactly **30.0 frames per 100 ticks** (100 Hz · 258/860 = 30.0 Hz). The
  callers-visible count is `[0x731C]`; the fractional remainder lives in
  `[0x7320]`. This closes FU-59's open leg "full `0x49320` semantics (the
  `0x731C`/`0x7320`/`0x7324` block)".
* `[0x7324]` is the **pace hold** flag: set to 1 by `FUN_00049308`
  (`MOV [0x7324],1; RET`, `0x49308..0x49312`) and to 0 by `FUN_00049314`
  (`0x49314..0x4931E`); the only static callers are inside `FUN_00045D0D`
  (`CALL 0x49308` at `0x45D6A`, `CALL 0x49314` at `0x45F5C`, from
  `get_xrefs_to 0x49308`/`0x49314`). The match loop also sets it directly to 1
  at teardown (`MOV dword [0x7324],1` at `0x49546`, byte-verified
  `read_memory 0x49540`) and zeroes it before registering the callback
  (`MOV [0x7324],EDI` with `EDI=0` at `0x4945F`).
* `FUN_000492E8` (`0x492E8..0x49304`) zeroes the whole pace block
  `[0x731C],[0x7320],[0x7328],[0x732C]`; `FUN_000492CC` returns `[0x732C]`;
  unnamed getter `0x49374..0x49379` returns `[0x731C]`.
* `FUN_000456AF` is called **from the interrupt callback on every granted
  frame** (30 Hz), not on every 100 Hz tick. It is the big key-state scanner
  (decompile: walks `DAT_00012xxx` key flags and `[0x46570]` method handlers,
  calls `FUN_0006CA17`/`FUN_0006C82C`/`FUN_0006CAEB`/`FUN_0006CBAD`); its
  output flags `0x4B1D4`/`0x4B1C4`, `0x680F`, `0x681A` are later consumed by
  `FUN_00045F6A`/`FUN_000451F1` in the drain loop.

## 2. The `0x49xxx` module: setup, callback lifecycle, match loop

### 2.1 Match setup `0x493A0` (undefined entry; no resolved xref)

`disassemble_bytes 0x49300` + hand-decode (`read_memory 0x49460`,
`0x49518`, `0x49540`). Entry `0x493A0` (byte-verified prologue
`PUSH EBX/ECX/EDX/ESI/EDI/EBP` at `0x493A0..0x493A5`; the address has **no
xref at all** — `get_xrefs_to 0x493A0` count 0 and
`search_instructions` operand `493a0` 0 matches — so it is reached by a
fixup/indirect path; open leg):

```
0x493A6  MOV EDX,1 / MOV [0x72F8],EAX   ; EAX = match selector argument
0x493B0  XOR EBX,EBX
0x493B2  MOV [0x72FC],EDX               ; = 1
0x493B8  MOV [0x7330],EBX / MOV [0x730C],EBX / MOV [0x7310],EBX
0x493CA  CALL 0x4C904 / MOV [0x7314],EAX
0x493D4  CALL 0x43600 / MOV [0x7318],EAX
0x493DE  CALL 0xCB2A4                   ; now
0x493E3  CALL 0x4C698
0x493E8  MOV EAX,0x18 / CALL 0x1D940    ; setting[0x18]
0x493F2  CALL 0x92AA0
0x493F7  MOV EAX,0x19 / CALL 0x1D940    ; setting[0x19]
0x49401  CALL 0x566A8 / CALL 0x73D10 / CALL 0x4B020
0x49410  MOV EAX,1 / CALL 0x4B02C
0x4941A  MOV EAX,1 / CALL 0x4A294
0x49424  CALL 0x4AD4C
0x49429  XOR EAX,EAX / CALL 0x36BC0     ; [0x5FFC] = 0
0x49430  CALL 0x4A228                   ; reset incl. FUN_00091BC4 ambience init
0x49435  CALL 0x47928 / CALL 0x478FC
0x4943F  MOV EAX,4 / CALL 0x1D940       ; setting[4]
0x4944B  CALL 0x443E8 / CALL 0x45390 / CALL 0x45D0D
0x4945A  PUSH 0x39320
0x4945F  MOV [0x7324],EDI               ; EDI = 0, release pace hold
0x49465  CALL 0x9F64C                   ; register INT-8 callback 0x49320
0x4946A  MOV EBP,[0x72F8] / ADD ESP,4 / TEST EBP,EBP / JNZ 0x4949F
0x49477  XOR EAX,EAX / CALL 0x37798
0x4947E  CALL 0x4B454                   ; [0x4C32A]
0x49483  TEST EAX,EAX / JZ 0x49495
0x49487  XOR EAX,EAX / CALL 0x36BC0 / CALL 0x37DE8 / JMP 0x4949A
0x49495  CALL 0x36BD8
0x4949A  CALL 0x39054
0x4949F  MOV ECX,4 / XOR EBX,EBX
0x494A6  MOV EAX,ECX / CALL 0x36BC8     ; [0x5FFC] == 4 ?
0x494AD  TEST EAX,EAX / JNZ 0x49541     ; state 4 -> leave loop
0x494B5  CALL 0x495B0                   ; match main loop (below)
0x494BA  MOV EAX,2 / CALL 0x36BC8       ; [0x5FFC] == 2 ?
0x494C4  TEST EAX,EAX / JZ 0x49537
0x494CC  CALL 0x4B5F4 / MOV EDX,EAX
0x494D3  CMP EAX,2 / JC 0x494E4 ...     ; dispatch 1 / 2..4 / other
0x4950F  CMP EDX,4 / JL 0x49523
0x49514  MOV EAX,0x200 / CALL 0x4C394 / CALL 0x4B100
0x49523  CALL 0x53A48 / CALL 0x36C70
0x4952D  MOV EAX,3 / CALL 0x36BC0       ; [0x5FFC] = 3
0x49537  CALL 0x54018 / JMP 0x494A6
0x49541  CALL 0x45D0D
0x49546  MOV dword [0x7324],1           ; hold pace on teardown
0x49550  CALL 0x45D0D
0x49555  PUSH 0x39320 / 0x4955A CALL 0x9F684 / ADD ESP,4
0x49562  CALL 0x45D0D / CALL 0x4AEA4 / CALL 0x67948
0x49571  XOR EDX,EDX / MOV ECX,[0x7304]
0x49579  MOV [0x72FC],EDX / MOV [0x1471C],EDX
0x49585  CMP ECX,3 / JNZ 0x4959A
0x4958A  MOV ESI,2 / CALL 0x6D7EC / MOV [0x7304],ESI
0x4959A  CMP [0x7304],2 / SETZ AL / ... / RET at 0x495AF
```

So `0x493A0` is the **play-match block**: initialise match state, reset
(`FUN_0004A228`), register `0x49320` in the FU-59 callback table, loop
`FUN_000495B0` while `[0x5FFC] != 4`, then hold the pace and cancel the
callback at `0x4955A`. `FUN_0004A294` (called at `0x4941A`) is the
flag/audio-refresh helper, and `FUN_0004A228` calls `FUN_00091BC4` — the
ambience initialiser already derived in FU-49 §1.10 (line 245).

### 2.2 Match main loop `FUN_000495B0` (single caller `0x494B5`)

`get_function_by_address 0x495B0` (body `0x495B0..0x49817`),
`get_xrefs_to 0x495B0` → exactly `0x494B5`. Setup: pushes, `[0x7300]=0`,
`[0x7304]=0` (raw bytes at `0x495B0`: `... XOR EDX,EDX; XOR EBX,EBX;
MOV [0x7300],EDX; MOV [0x7304],EDX; MOV ECX,1; CALL 0x53D58`). The loop
(`0x495D1..0x49817`, disassembly in the `0x49300` sweep):

* **display/menu arms** (`0x495D1..0x4968B`): `FUN_00053D58`, conditionally
  `FUN_0004CD70`, `FUN_0004D2D4`, `FUN_0004C904` → `[0x7314]`, `FUN_00043600`
  → `[0x7318]`, `FUN_00043E30([0x7314]+0x14)`, `FUN_000439D0`, then one of
  three arms selected by `FUN_0004382C`/`FUN_00043608`/`FUN_000541D4`:
  `FUN_00043330`+`FUN_00058BC0`, `FUN_000432EC`+`FUN_00058D70`, or
  `FUN_00058B68`+`FUN_00049830([0x7318],[0x7314])`, then `FUN_000565BC` and
  `FUN_00039180(3)`;
* **per-frame body** (`0x49690..0x497E8`):
  `FUN_00043B48`; `FUN_000478D0`; `FUN_00049280(EBX)` → `[0x7300]`;
  `FUN_0004A178` (returns 1 on mode/pause ⇒ skip body); `FUN_0006400C`
  (`[0x9A98]==0`); **`0x496D5 CALL 0x49B28`** (drain, §3);
  `FUN_00036C3C` (`[0x5FFC] ∈ {2,4}`); `FUN_0004B378` (`[0x57AC0]`);
  then `FUN_00053BB8`, `FUN_000635B8`, `FUN_00036C3C`, `FUN_00063FD0`,
  `FUN_0004A068`, `FUN_0004536C`, `FUN_0004A294(EBX)`, `FUN_0004557C`,
  `FUN_00036BC8(5)`/`(7)` and `FUN_00036BC0`;
* **loop condition** (`0x497E8`): `CMP EBX,[0x7304]; JZ 0x495D1` — repeat
  while the current selector `EBX` equals the target `[0x7304]`; when it
  changes, `FUN_0004557C` gates the exit and `FUN_00036BC0(4)` +
  `[0x7304]=3` stage the leave (`0x497F4..0x49816`).

The display arms call `FUN_0004937C` (`[0x7300]>>2`), `FUN_00049388`
(`[0x7300]`), `FUN_00049390` (`[0x7308]`), `FUN_00049398` (`[0x72FC])` —
`get_xrefs_to` shows consumers in `0x1xxxx` (front-end), `0x36xxx` (input),
`0x4Dxxx`/`0x4Exxx`/`0x4Fxxx`, `0x51xxx`/`0x57xxx` (display/menu), and
`0x60xxx` (music/audio) — so the `0x72F8..0x7334` block is the shared match
clock/state export (open leg: full role of each getter).

### 2.3 Adjacent helpers in the gap (disassembled this slice)

| addr | body | semantics (evidenced) |
|---|---|---|
| `FUN_00049280` | `0x49280..0x492CB` | `i ∈ 0..9`: `q = now*12/5`; return `q - [0x7334+4i]`, store `q` — 10 elapsed-time slots |
| `FUN_000492CC` | `0x492CC..0x492D1` | `return [0x732C]` |
| `FUN_000492D4` | `0x492D4..0x492E5` | slot-0 elapsed via `0x49280`, store to `[0x7300]` |
| `FUN_000492E8` | `0x492E8..0x49304` | zero `[0x731C]`,`[0x7320]`,`[0x7328]`,`[0x732C]` |
| `FUN_00049308` | `0x49308..0x49312` | `[0x7324]=1` |
| `FUN_00049314` | `0x49314..0x4931E` | `[0x7324]=0` |
| getter | `0x49374..0x49379` | `return [0x731C]` (unnamed; not a Ghidra function) |
| `FUN_0004937C` | `0x4937C..0x49384` | `return [0x7300] >> 2` |
| `FUN_00049388` | `0x49388..0x4938D` | `return [0x7300]` |
| `FUN_00049390` | `0x49390..0x49395` | `return [0x7308]` |
| `FUN_00049398` | `0x49398..0x4939D` | `return [0x72FC]` |
| unnamed | `0x49AD0..0x49B24` | uncalled copy of the phase→`[0x7310]` cadence block (`get_xrefs_to 0x49AD0` count 0); the live copy is inlined in `FUN_00049B28` |
| `FUN_00049818` | `0x49818..0x4982C` | `[0x7314]=FUN_0004C904(); [0x7318]=FUN_00043600(); RET` (inline-call wrapper) |
| `FUN_00049830` | `0x49830..0x49A50` | transform interpolation over `0x545E0..0x545FC` clamped by `0x78`, then `FUN_00043DB8` |

## 3. `FUN_00049B28` — the frame drain

Clean listing `disassemble_function 0x49B28` (322 instructions,
`0x49B28..0x4A064`), with the raw-byte gap decode at `0x49FD8`
(`read_memory 0x49FD8 60`). Called from exactly one site, `0x496D5` inside
`FUN_000495B0` (`get_xrefs_to 0x49B28`).

**Entry gates** (`0x49B2B..0x49B7F`):

```
0x49B2B  CALL 0x37B24            ; FUN_00037b24() = ([0x4B018] == 7)
0x49B30  TEST EAX,EAX / JNZ 0x49B3D
0x49B34  CALL 0x3F684            ; FUN_0003f684() = ([0x4B018] == 0x18)
0x49B39  TEST EAX,EAX / JZ 0x49B81
0x49B3D  ... cleanup: FUN_00039054, FUN_000492D4, FUN_00053F90,
         [0x7330] / FUN_000478FC / FUN_00047880 / FUN_000478AC; RET
```

So modes 7 and 0x18 take the short cleanup path; all other values drain frames.

**Backlog setup and loop** (`0x49B81..0x4A064`):

```
0x49B81  MOV EAX,[0x731C] / MOV [0x732C],EAX / MOV [0x7328],EAX
0x49B90  CALL 0x46177
0x49B95  CMP [0x7328],0 / JLE 0x4A056      ; nothing pending -> subtract nothing
0x49BA2  XOR ECX,ECX
0x49BA4  CALL 0x45F6A                      ; input scan -> 0x4B1DA flags; <0 = abort
0x49BA9  TEST EAX,EAX / JGE 0x49BD2
0x49BAD  ... abort: [0x732C] -= [0x7328]; [0x731C] -= [0x732C]; RET
0x49BD2  DEC [0x7328]                      ; one frame consumed
0x49BD8  CALL 0x4B380                      ; EDX = EBX = [0x57A4A] >> 24 (phase)
0x49BE1  MOV EAX,2 / CALL 0x451F1          ; event 2 active?
0x49BEB  TEST EAX,EAX / JZ 0x49CF4
0x49BF3  CMP EDX,0xB / CMP EDX,0xF / ...   ; phase-gated event dispatch
...
0x49CF4  CALL 0x4B454                      ; [0x4C32A]
0x49CF9  MOV EDX,ECX
0x49CFB  TEST EAX,EAX / JZ 0x49D7F
0x49D03  ... event chain A: 7, 0xA, 0xB, 8, 0xF  (each FUN_000451F1)
0x49D7F  ... event chain B: 9, 4, 5, 6, 0xF, 0xB
0x49DFE  CMP EBX,0xB / 0xF -> EDX = ECX (0)
0x49E0A  ... event chain C: 7, 0xA, 0xC, 0xD, 0xE, 8, 1, 3
0x49EB9  TEST EDX,EDX / JZ 0x49F49         ; no event -> frame-housekeeping
0x49EC1  ... FUN_00037AE4 gate; FUN_000492D4, FUN_00063900, FUN_000510C0,
         FUN_00053F08, FUN_00047878/80/FC, FUN_00045268(EDX),
         FUN_00037798, FUN_00038004(EDX)
0x49F49  CALL 0x37AE4 / ... cleanup arms
0x49F97  CALL 0x6400C                      ; [0x9A98] != 0 -> skip frame
0x49F9C  TEST EAX,EAX / JZ 0x4A04A
0x49FA4  CALL 0x51AB8                      ; [0x4E574] != 0 -> skip frame
0x49FA9  TEST EAX,EAX / JNZ 0x4A045
0x49FB1  MOV EAX,0x200 / CALL 0x4C394      ; [0x57A5C] += 0x200
0x49FBB  CALL 0x4B100                      ; match update (FU-58 §5 chain)
0x49FC0  MOV EAX,0x200 / CALL 0x36208      ; 0x200-scaled motion/camera step
0x49FCA  CALL 0x91DD8                      ; presentation/event pump
0x49FCF  CALL 0x51AB8
0x49FD4  TEST EAX,EAX / JNZ 0x49FE1
0x49FD8  CALL 0x36C3C                      ; [0x5FFC] in {2,4}?
0x49FDD  TEST EAX,EAX / JZ 0x49FF5
0x49FE1  CALL 0x45D0D / CALL 0x4B454       ; state 2/4 arm
0x49FF5  CALL 0x36C70
0x49FFA  CALL 0x4B380                      ; phase
0x49FFF  CMP EAX,3 / JC 0x4A01F
0x4A004  CMP EAX,8 / JBE 0x4A024
0x4A009  CMP EAX,0xA / JC 0x4A02C
0x4A00E  CMP EAX,0xC / JBE 0x4A024
0x4A013  CMP EAX,0x13 / JC 0x4A02C
0x4A018  CMP EAX,0x14 / JBE 0x4A024
0x4A024  INC [0x7310]                      ; phases {0,1,3..8,0xA..0xC,0x13,0x14}
0x4A02C  MOV [0x7310],ECX                  ; ECX = 0 for all other phases
0x4A032  CMP [0x7310],0x5A / JGE 0x4A045
0x4A03B  MOV EAX,1 / CALL 0x6408C
0x4A045  CALL 0x53A48
0x4A04A  CMP ECX,[0x7328] / JL 0x49BA4     ; loop (ECX = 0 on entry)
0x4A056  MOV EAX,[0x732C] / SUB [0x731C],EAX
0x4A061  POP EDX / POP ECX / POP EBX / RET
```

Model:

* **Inputs/state**: pending frames `[0x731C]`, total `[0x732C]`, remaining
  `[0x7328]`, event-flag array `0x4B1DA` (0x12 bytes), phase
  `[0x57A4A] >> 24`, mode `[0x4B018]`, gates `[0x9A98]` (`FUN_0006400C`),
  `[0x4E574]` (`FUN_00051AB8`), `[0x5FFC]` (`FUN_00036C3C`), `[0x4C32A]`
  (`FUN_0004B454`).
* **Order per drained frame**: refresh input flags (`FUN_00045F6A`) → consume
  one pending (`[0x7328]--`) → read phase and pick an event code by priority
  chain (`FUN_000451F1`), gated by `[0x4C32A]`, and dispatch it
  (`FUN_00045268`/`FUN_00037798`/`FUN_00038004`) → frame path if `[0x9A98]==0`
  and `[0x4E574]==0`: `FUN_0004C394(0x200)`, `FUN_0004B100`,
  `FUN_00036208(0x200)`, `FUN_00091DD8` → phase-cadence counter `[0x7310]`
  (increment on a 13-value phase set, reset otherwise; below 0x5A calls
  `FUN_0006408C(1)`) → `FUN_00053A48` → next frame.
* **Interaction with the tick**: `FUN_00049320` (the registered callback)
  increments `[0x731C]` while `FUN_00049B28` runs; the drain reads the count
  once into `[0x732C]`/`[0x7328]` and at the end subtracts only the frames it
  observed (`[0x731C] -= [0x732C]`), so grants that arrive during the drain
  survive to the next call. Aborting (`FUN_00045F6A` < 0) subtracts only
  processed frames (`[0x732C] -= [0x7328]` = processed count).
* `FUN_00045F6A` (`0x45F6A`) decompiles as the per-player input scan: it
  clears/sets the `0x4B1DA` (0x12-byte) event array via
  `FUN_000461C6`/`FUN_00046246` over `[0x6803]` players and returns `-1` on a
  negative sub-result; `FUN_000451F1(code)` returns `0x4B1DA[code]`, with
  special cases for code 1 (`[0x6803]==2`) and code 3 (`FUN_0006D1B2`).

## 4. `FUN_00091DD8` — the match presentation/event pump

### 4.1 Identity

`FUN_00091DD8` has exactly one caller: `0x49FCA` in `FUN_00049B28`
(`get_xrefs_to 0x91DD8`). It runs **after** the match update
(`FUN_0004B100`) and the `0x200`-scaled motion step (`FUN_00036208`) and
**before** the frame housekeeping, so it is the presentation half of a
drained frame. Its callees are music/ambience/event plumbing — no VGA,
blit, VGT or record decode function appears in the listing — so it is a
presentation/**event** pump, not the renderer (the render/blit arms live in
`FUN_000495B0`, §2.2).

### 4.2 Entry/gate and scheduling (`0x91DD8..0x91E5E`)

```
0x91DD8  CALL 0x37AE4                  ; FUN_00037ae4() != 0 -> RET (0x91DD5)
0x91DE1  CALL 0x652B8                  ; music start (FU-52, caller 0x91DE1)
0x91DE6  CMP byte [0x4C32A],0 / JNZ 0x91DF4
0x91DEF  CALL 0x91F60                  ; ambience machine (FU-49 §1.10)
0x91DF4  CMP dword [0x4C312],0 / JZ 0x91DD5   ; body disabled -> RET
0x91E00  PUSH EBX/ECX/EDX/ESI/EDI/EBP
0x91E06  XOR EDX,EDX / MOV DX,[0x57AB6]
0x91E0F  MOV EBX,5 / MOV EAX,EDX / SAR EDX,0x1F / IDIV EBX   ; [0x57AB6] / 5
0x91E1B  LEA EDX,[EAX+0x2D]            ; slot = frame/5 + 0x2D
0x91E1E  MOV EAX,EDX / SHL EAX,4 / SUB EAX,EDX / MOV ESI,[0x5B35C]
0x91E2B  SHL EAX,2 / ADD ESI,EAX       ; ESI = [0x5B35C] + slot*0x1C
0x91E30  CALL 0xCB2A4                  ; now
0x91E35  CMP ESI,EAX / JGE 0x921ED     ; deadline not reached -> tail machine
0x91E3D  MOV EAX,[0x57A4A] / SAR EAX,0x18 / CMP EAX,2 / JNZ 0x921ED
0x91E4E  MOV EDX,[0x57754] / TEST / JL -> NEG
0x91E5C  JMP 0x921BB: CMP EAX,0x2D0 / JGE 0x921ED
0x921C2  CALL 0x8EF38                  ; FUN_0008ef38() -> input/phase code 0..0xB
0x921CC  CWDE / CMP EAX,5 / JG 0x921E3
0x921D2  MOV EBX,4 / MOV EAX,0x8D / XOR EDX,EDX / CALL 0x8F188  ; enqueue
0x921E3  CALL 0xCB2A4 / MOV [0x5B35C],EAX     ; stamp last-run
```

* `[0x57AB6]` is a frame/clock counter (`get_xrefs_to` 94 refs; three
  writers `FUN_00092E2C`, `FUN_00073EE0`, `FUN_0008AF38`); the pump schedules
  itself on `[0x5B35C] + ([0x57AB6]/5 + 0x2D)*0x1C` — i.e. a quantised
  deadline based on that counter (`0x1C`-stride). `[0x5B35C]` is only written
  here (`0x921E8`, `0x92548`, `FUN_00091BC4`; `get_xrefs_to 0x5B35C`).
* `[0x4C312]` has **no static writer** (`get_xrefs_to 0x4C312` → 2 reads:
  `0x8F19F`, `0x91DF4`) — open leg; it behaves as a "presentation active"
  dword. `[0x4C32A]` gates both the ambience call and the sound-event enqueue
  in `FUN_0008F188`, is set by `FUN_0001B7B8` to `([0x4C1D0] == 4)` with a
  magic companion write `[0x5B680]=0xC08310F9` / `0` (`FUN_0001B7B8`
  decompile), and selects the extra event codes in `FUN_00049B28` (§3).
* The `[0x57A4A]>>24 == 2 && |[0x57754]| < 0x2D0` branch only enqueues
  `0x8D`/param 4 when `FUN_0008EF38` returns `1..5`; `FUN_0008EF38` maps
  `[0x57AC2]` plus `[0x57AB6]`/`[0x57ABA]`/`[0x5881A]` ratios to codes
  `0..0xB` (decompile).

### 4.3 The 3-state presentation queue (`0x921ED..0x924E7`)

Dispatch on `[0x5B330]` (`0x921ED: MOV EAX,[0x5B330]`; cases 0/1/2):

* **state 0** (`0x9220B..0x9235D`): if `[0x5B36C] != 0`, reset indices
  `[0x5B334]=[0x5B338]=0`, `FUN_0008F0C4(0x43,0)`, drain the command ring with
  `FUN_0008F2C4(0x40)`, then if `[0x5B334] > [0x5B338]` process the record at
  index `[0x5B338]` with `FUN_00092548` and increment `[0x5B338]`; else reset
  both. Then `[0x5B374]` → `FUN_0008F2C4(0x20)`; `FUN_00066E70()` gate;
  `[0x5B348]` timer (deadline `[0x5B34C]`, set by `FUN_00092548` for id
  `0xDA`). At `0x92330`: `FUN_000CBC4C()` (RNG), `[0x5B364] = now +
  r % 0x3C + 0xB4`, `[0x5B330]++` — hand-decoded from raw bytes
  (`read_memory 0x9232C`: `e8 17 99 03 00 / bb 3c 00 00 00 / 31 d2 / f7 f3 /
  89 d6 / e8 ... / 01 f0 / 8b 1d 30 b3 05 00 / 05 b4 00 00 00 / 43 / a3 64 b3
  05 00 / 89 1d 30 b3 05 00`), then falls through into state 1;
* **state 1** (`0x9235E..0x923EE`): if any of `[0x5B370]`/`[0x5B36C]`/
  `[0x5B374]` is set → `FUN_0008F2C4(8)` + record drain; else wait until
  `now > [0x5B364]`, then `[0x5B364] = now + 0xB4` and `[0x5B330]++`;
* **state 2** (`0x923EF..0x924C6`): input one-shots while waiting:
  `[0x5AFAE]==0 && FUN_0008EF38()==1` → `FUN_00091528`; `[0x5AFAF]==0 &&
  ==1` → `[0x5AFAF]=1`; `[0x5AFAC]==0 && ==2` → `FUN_00091630`;
  `[0x5AFAD]==0 && ==5` → `FUN_00091774`; `[0x5AAD8]` → `FUN_0008F2C4(0)`;
  then the same record-drain block;
* **tail** (`0x924C7..0x924E7`): zero `[0x5B374]`,`[0x5B36C]`,`[0x5B370]`,
  `[0x5AAD8]` and return.

**Queue plumbing** (disassembled this slice):

* `FUN_0008F0C4` (`0x8F0C4..0x8F144`, decompiler lost it): enqueue
  `(EAX=id, EDX=param)` into the 6-slot queue: if `[0x5B334] < 6`, store
  `id` at `0x5B380 + idx*0x1C`, `EDX` at `+0x18`, `[0x5B368]` at `+0xC`, and
  `0x5AAE0 + [0x5AADC]*0x20` (command-ring entry pointer) at `+0x14`; then
  `[0x5B334]++`. Queue fields read back by `FUN_00092548` as
  `in_EAX[0]`/`[3]`/`[5]`/`[6]`.
* `FUN_0008F178` (`0x8F178..0x8F187`): `[0x5B334]=0; FUN_00066DFC()`.
* `FUN_00092548` (`0x92548..0x927D4`, decompile + `disassemble_bytes 0x92790`):
  takes the record pointer in `EAX`, applies the `0xDC`/`[0x5AC38+id*4]`
  cooldown and `0x21` team gate, dispatches by event id to
  `FUN_00066724`/`0x66840`/`0x668CC`/`0x6690C`/`0x669C0`, stores the record
  pointer to `[0x5B428]`, then at `0x9279D..0x927D3` sets
  `[record[+0x14]+8]=1`, `[0x5B330]=0`, bumps `[0x5AFB8 + id*4]` and stamps
  `[0x5AC38 + id*4]=now`; the `id == -0x617F3DC` arm additionally writes the
  `0x5B33C/0x5B340/0x5B344` triple (`0xF9E80C24`/`0xFFFE37E9`/now).

### 4.4 Relation to `FUN_00091F60` (already derived)

`FUN_00091F60` is called by `FUN_00091DD8` at `0x91DEF` **only when
`[0x4C32A]==0`**. It is the per-team ambience deadline machine; its arrays,
`FUN_00092040`/`FUN_000920C0` transitions and the `FUN_00091E64` volume ramp
are fully derived in FU-49 §1.10 (lines 240–340). This slice adds only the
call-site condition and the fact that the whole ambience machine runs inside
the drained match frame (via `FUN_00049B28` → `FUN_00091DD8`), not in the
INT-8 callback.

## 5. Structural map of the match frame

Ordered phases of one drained frame (`FUN_00049B28` iteration), with the
source of the phase:

| # | phase | entry points | notes |
|---|---|---|---|
| 0 | 100 Hz tick / pace | `FUN_00049320` (INT-8) | grants 30 Hz frames, samples input (`FUN_000456AF`) per grant |
| 1 | drain head | `FUN_00049B28 0x49B81` | snapshot `[0x731C]` into `[0x732C]`/`[0x7328]` |
| 2 | input/event selection | `FUN_00045F6A`, `FUN_0004B380`, `FUN_000451F1`, chains A/B/C | sets/reads `0x4B1DA`; phase from `[0x57A4A]>>24` |
| 3 | event dispatch | `FUN_00045268`, `FUN_00037798`, `FUN_00038004` | per-code handlers |
| 4 | match update | `FUN_0004B100` (+`FUN_0008AF38`→`FUN_00088940`, FU-58 §5) | preceded by `FUN_0004C394(0x200)` → `[0x57A5C] += 0x200` |
| 5 | motion/camera | `FUN_00036208(0x200)` | `0x5FB8`/`0x5FC0` accumulators, clamps ±0x9C0/±0xB10/±0xA0 |
| 6 | presentation pump | `FUN_00091DD8` | music `0x652B8`; ambience `0x91F60` if `[0x4C32A]==0`; 3-state/6-slot queue |
| 7 | frame housekeeping | `FUN_00036C70`, phase cadence `[0x7310]`, `FUN_0006408C(1)`, `FUN_00053A48` | 0x5A (90) limit |
| 8 | drain tail | `0x4A04A`, `0x4A056` | `[0x731C] -= [0x732C]` |
| 9 | outer loop extras | `FUN_000495B0` | display/menu arms, `FUN_00049280` deltas, `FUN_0004A178` pauses |

Key structures/globals (addresses as displayed by Ghidra; FU-59 errata apply):

| address | width | evidence | role (evidenced only) |
|---|---|---|---|
| `0x4B018` | dword | `FUN_00037B24` (`==7`), `FUN_0003F684` (`==0x18`), 41 xrefs | mode/selector gating the pace and the drain entry |
| `0x5FFC` | dword | `FUN_00036BC0` write, `FUN_00036BC8` compare | screen/match state checked as 2, 4, 5, 6, 7 |
| `0x72F8` | dword | `0x493AB`, `0x4946A`, `0x49787` | match selector argument handed to `0x493A0` |
| `0x72FC` | dword | `0x493B2`, `0x49579`, getter `0x49398` | `1` during match; zeroed at teardown |
| `0x7300` | dword | `0x492D4`, `0x4969A`, getters `0x4937C/0x49388` | elapsed 100 Hz time ×12/5 (slot-0 delta) |
| `0x7304` | dword | `0x495C6`, `0x496AC`, `0x497E8` | loop target selector / leave staging (0..4) |
| `0x730C` | dword | `0x493BE`, `0x496A1` | previous selector copy |
| `0x7310` | dword | `0x493C4`, `0x49FFF..0x4A040` | cadence counter for a 13-value phase set, limit `0x5A` |
| `0x7314`,`0x7318` | dword | `0x493CF`/`0x493D9`, `0x495F8` | two objects from `FUN_0004C904`/`FUN_00043600` |
| `0x731C` | dword | `0x4934F`, getter `0x49374` | granted 30 Hz frames pending drain |
| `0x7320` | dword | `0x49335..0x4935C` | pace accumulator (258 per 100 Hz tick) |
| `0x7324` | dword | `0x49323`, `0x49308`, `0x49314`, `0x49546` | pace hold flag |
| `0x7328` | dword | `0x49B8B`, `0x49BD2`, end/abort | frames remaining in this drain |
| `0x732C` | dword | `0x49B86`, `0x49BAD`, getter `0x492CC` | total/processed frames for the drain |
| `0x7330` | dword | `0x493B8`, `0x49C38` | flag set when `FUN_00047878` reports |
| `0x7334+4i` | 10 dwords | `0x492B7`/`0x492BE` | per-slot elapsed-time store |
| `0x57A4A` | dword | `0x4B380` (`>>24`), `0x91E3D` | match phase byte (0..0x14 observed) |
| `0x57A5C` | dword | `0x4C394` (`+= EAX`), `0x4B100` (`>>8`) | fixed-point frame accumulator |
| `0x57754` | dword | `0x91E4E` | signed distance compared against `0x2D0` |
| `0x57AB6` | word/dword | `0x91E08`, 94 xrefs | frame/clock counter (pump scheduling) |
| `0x4C312` | dword | `0x91DF4`, `0x8F19F` | presentation-active gate; no static writer |
| `0x4C32A` | byte | `0x91DE6`, `0x8F192`, `FUN_0004B454` | ambience/event-queue mode flag (`[0x4C1D0]==4`) |
| `0x4C1D0` | byte | `FUN_0001B7B8`, `FUN_00091774` | controller/device selector |
| `0x9A98` | dword | `FUN_0006400C` (`==0`) | load/busy gate over the frame path |
| `0x4E574` | dword | `FUN_00051AB8`, 27 refs | frame-suspend flag |
| `0x4B1DA` | 0x12 bytes | `FUN_00045F6A`, `FUN_000451F1` | per-event-code input flags |
| `0x5B330` | dword | init `FUN_00091BC4`, `0x921ED`, `0x923E9`, `0x92548` | presentation queue state 0/1/2 |
| `0x5B334`,`0x5B338` | dword | `0x8F0C4`/`0x8F178`/`0x91DD8` | queue count (max 6) / current index |
| `0x5B348`/`0x5B34C` | dword | `0x92548`, `0x91DD8` | timer flag / deadline (id `0xDA`) |
| `0x5B35C` | dword | `0x91E25`, `0x921E8`, `0x92548` | last-run stamp for the pump deadline |
| `0x5B364` | dword | `0x923C9`, `0x923EF` | next-time gate used by states 0/1/2 |
| `0x5B36C`,`0x5B370`,`0x5B374` | dwords | `FUN_0008F188`, `0x91DD8` | queued-event flags from enqueue params |
| `0x5B380+0x1C*i` (6) | 0x1C | `0x8F0DE..0x8F135`, `0x92548` | presentation events: `+0` id, `+0xC` value, `+0x14` command-ring ptr, `+0x18` param |
| `0x5B428` | dword | `0x92548`, `0x922E1` | current/selected record pointer |
| `0x5B438` | dword | `FUN_00091E64` | ambience ramp tick counter (FU-49) |
| `0x5A980`/`0x5A990` | 2×dword | FU-49 §1.10 | ambience timer/state per team |
| `0x5AADC`,`0x5AAD4`,`0x5AAD8` | dword | `FUN_0008F188`/`0x8F2C4`/`0x8F0C4` | 10-slot command ring cursor/count/flag |
| `0x5AAE0+0x20*i` (10) | 0x20 | `FUN_0008F188`, `FUN_0008F2C4` | command ring entries (id/time/param triplets) |
| `0x5AFAC..0x5AFAF` | bytes | `0x92400..0x9246F` | one-shot flags for input codes 3,5,1,2 |
| `0x10BB4` | dword table | `FUN_0008F2C4` | per-command callbacks (indexed by ring id) |

## 6. Bounded sub-slices

| # | slice | anchors | why bounded |
|---|---|---|---|
| S1 | match start path / setup caller | `0x493A0`, register `0x49465`, loop `0x494B5`, cancel `0x4955A` | one function, one unresolved indirect entry (FU-58 §7 slice 9) |
| S2 | frame pace (**ported**) | `0x49320`, `0x492E8`, `0x49308`, `0x49314`, getters `0x492CC`/`0x49374` | fixed 2-field arithmetic, two boolean gates |
| S3 | frame backlog drain | `FUN_00049B28 0x49B81..0x4A064` | fixed loop over `[0x7328]`; ends at RET |
| S4 | input event selection | `FUN_00045F6A`, `FUN_000451F1`, `0x4B1DA`, chains A/B/C | 0x12 codes, three explicit priority lists |
| S5 | match update step | `FUN_0004B100`, `FUN_0008AF38`, `FUN_00088940`, `FUN_00072478` | FU-58 §7 slice 5; already anchored |
| S6 | presentation event queue | `FUN_00091DD8`, `FUN_00092548`, `FUN_0008F0C4`, `FUN_0008F178` | 6 fixed slots, 3 states |
| S7 | ambience timers | `FUN_00091F60`, `FUN_00092040`, `FUN_000920C0`, `FUN_00091E64` | **already derived in FU-49 §1.10** — no re-derivation needed |
| S8 | in-match display/menu arms | `FUN_000495B0 0x495D1..0x4968B`, `[0x7304]` gate | three arms selected by `FUN_0004382C`/`0x43608`/`0x541D4`; loop-bounded |
| S9 | leave/teardown path | `0x49541..0x495AF`, `FUN_000495B0 0x497E8..0x49817` | state-4 exit, hold + cancel |

## 7. Port: `fifa96_match_pace`

`include/fifa96_loader/fifa96_match_pace.h` +
`src/fifa96_loader/fifa96_match_pace.c` (caller-owned struct, no globals, no
comments, `-fifa96_err_t` for invalid arguments). Scope is exactly S2:
`FUN_00049320` plus its hold/reset companions.

| original | port |
|---|---|
| `FUN_000492E8` zero `[0x731C]/[0x7320]/[0x7328]/[0x732C]` | `fifa96_match_pace_init` zeroes `acc`/`pending`/`hold` (the two drain counters `[0x7328]`/`[0x732C]` are not modelled) |
| `FUN_00049308` `[0x7324]=1` | `fifa96_match_pace_hold` |
| `FUN_00049314` `[0x7324]=0` | `fifa96_match_pace_resume` |
| `0x49323` hold gate + `0x4932C` `FUN_0003F684()` gate | `hold` field + `blocked` argument |
| `0x49335..0x49362` `acc += 0x102; if (acc >= 0x35C) { acc -= 0x35C; pending++; }` | `fifa96_match_pace_tick`, `FIFA96_MATCH_PACE_STEP`/`_FRAME` |
| `0x49368 CALL 0x456AF` | not modelled — the port reports the grant through `*granted` and leaves `FUN_000456AF` to the caller |
| getter `0x49374`/`FUN_000492CC` `[0x731C]` | `fifa96_match_pace_pending` |
| `FUN_00049314`’s indirect caller list | the caller supplies `blocked` (mode `[0x4B018]==0x18`) and drives `hold` |

Tests (`tests/test_match_pace.c`, suite 48 → **49**): init zeroing,
3-ticks-then-grant exact accumulator values (`0x102·4 − 0x35C`), exactly 30
granted frames in 100 ticks with zero remainder, hold blocks and preserves the
accumulator until resume, blocked ticks neither accumulate nor grant, 1000
ticks → 300 pending, NULL handling for init/hold/resume/tick/pending and for
`granted == NULL` (returns `-FIFA96_ERR_INVALID`, no state change). ASan+UBSan
build of `test_match_pace` is clean.

## 8. Errata (quoted)

* FU-58 §6: "`FUN_00049B28` … calls `FUN_00091DD8` unconditionally in its
  event path" — **the call is conditional**: `0x49F97 FUN_0006400C` must
  return nonzero (`[0x9A98]==0`), `0x49FA4 FUN_00051AB8` must return zero, and
  it is reached only after the event-code chain yields `EDX != 0` and
  `FUN_00037AE4()==0`. It is also not the loop body itself: `FUN_00049B28`
  drains `[0x731C]` 30 Hz packets and the `0x91DD8` call is the per-frame
  presentation step (`0x49FCA`).
* FU-58 §7 slice 2 "one loop + two helpers" — the loop is now resolved to the
  setup (`0x493A0`), match loop (`FUN_000495B0`) and drain (`FUN_00049B28`)
  structure above, and the two helpers to the nested `FUN_00091E64`/
  `FUN_00091F60`/`FUN_00092040`/`FUN_000920C0` family plus `FUN_00092548`.
* FU-59 §4: "`0x49465` (match module …)" and "`0x4955A` (match;
  `0x39320`)" — **confirmed and placed**: both sit in `0x493A0`, which
  registers before the `FUN_000495B0` loop (`0x4945F..0x49465`) and cancels
  after it (`0x49555..0x4955A`), with `[0x7324]=1` written between
  (`0x49541..0x49546`).
* FU-59 §8 open leg "full `0x49320` semantics (the `0x731C`/`0x7320`/`0x7324`
  block)" — **closed this slice** (§1).
* FU-58 §5 "Match-side code … uses the time getters/deadlines rather than the
  registry" — correct, and the registry edge is the whole point of the pace
  block: the callback IS the 30 Hz frame source; the drain consumes its count.
* FU-49 §1.10 call-site note: the match call sites of `FUN_00092040` at
  `FUN_00072478`/`FUN_00088940` are inside `FUN_0004B100`'s chain
  (`FUN_0004B100` → `FUN_0008AF38`), i.e. inside phase 4 of the drained frame.

## 9. Open legs

* **Setup caller unknown**: `0x493A0` has no xref and no operand match
  (`get_xrefs_to` 0, `search_instructions` `493a0` 0); the chain from the
  front-end/competition setters into it is slice S1/§FU-58 slice 9.
* `[0x4C312]` has no static writer; `[0x4E574]` writers (`0x53DC4`,
  `0x53DE0`, `0x52030`, `0x52D68`, `0x537F8`, unnamed sites) were not derived;
  `[0x9A98]`'s load-state semantics are only partially mapped (FU-58 §6 gate).
* `[0x57A4A]>>24` phase values 0..0x14 are observed only through the two
  gates here (`FUN_0004B380`, the `FUN_00049B28` cadence set and the
  `0x91DD8` `==2` test); no map of phase→situation is asserted.
* `FUN_00049B28`'s loop register contract (`ECX` used as the comparison
  operand at `0x4A04A` while `[0x7328]` is the decremented count) relies on
  Watcom callee register behaviour; the loop *bound* (`[0x7328]`) is
  unambiguous, the register contract is not line-decoded.
* `FUN_00049AD0` (`0x49AD0..0x49B24`) is an uncalled duplicate of the
  `[0x7310]` phase-cadence block (`get_xrefs_to` 0); dead or fixup-reached —
  not decided.
* `FUN_0009219C` (`0x9219C..0x921B6`: `[0x5B334]==0 && FUN_00066E70()==0`) is
  an unreferenced helper inside the `0x91DD8` region (`get_xrefs_to` 0).
* `FUN_00092548`'s inner dispatches (`FUN_00066724`/`0x66840`/`0x668CC`/
  `0x6690C`/`0x669C0`) and the command-ring callback table `0x10BB4` are not
  decomposed; `FUN_0008F0C4`'s and `FUN_0008F178`'s callers beyond `0x91DD8`
  are not enumerated.
* The `0x5B33C/0x5B340/0x5B344` triple written by `FUN_00092548`
  (`0xF9E80C24`, `0xFFFE37E9`, now) has a getter `FUN_00092194` with zero
  callers; role undetermined.
* `FUN_0004B100`/`FUN_00036208` internals beyond the cited call edges remain
  slice S5/S8 material (FU-58 §7 slices 5/7).
* Object-base classification of each quoted global (FU-59 errata) was not
  re-run per address; addresses are quoted as Ghidra displays them.

## Provenance

Ghidra MCP on `/fifa96_le.bin`:
`decompile_function` 0x49B28, 0x91F60, 0x37B24, 0x3F684, 0x37AE4, 0x45F6A,
0x451F1, 0x4B380, 0x4B454, 0x6400C, 0x51AB8, 0x4C394, 0x4B100, 0x36208,
0x45D0D, 0x36C3C, 0x4B378, 0x4A178, 0x49280, 0x4A228, 0x4A294, 0x1D940,
0x8EF38, 0x8F2C4, 0x8F0C4, 0x8F188, 0x92548, 0x91528, 0x91630, 0x91774,
0x36BC8, 0x36BC0, 0x456AF, 0x45390, 0x1B7B8 (0x91DD8 failed: "Decompiler
process died");
`disassemble_function` 0x49B28, 0x49320, 0x91DD8, 0x8F0C4, 0x8F178;
`disassemble_bytes` 0x49300 (2088 B), 0x49000 (800 B), 0x49FC0 (166 B);
`read_memory` 0x49FD8, 0x49460, 0x49518, 0x49540, 0x495B0, 0x496C8, 0x49398,
0x92194, 0x9232C, 0x91DD0;
`get_function_by_address` 0x49B28, 0x91DD8, 0x91E64, 0x91F60, 0x92040,
0x920C0, 0x92194, 0x49320, 0x495B0, 0x49830;
`get_xrefs_to` 0x49B28, 0x91DD8, 0x493A0, 0x495B0, 0x4C312, 0x4C32A, 0x5B330,
0x57AB6, 0x57A4A, 0x5B35C, 0x9A98, 0x5B334, 0x5B338, 0x5B348, 0x49308,
0x49314, 0x4B018, 0x4E574, 0x49AD0, 0x6408C, 0x9219C, 0x4937C, 0x49390,
0x492E8, 0x49398;
`search_instructions` operand `493a0`, operand `493`;
`search_byte_patterns` `a0 93 04 00`; `search_functions` `FUN_00049`;
`get_current_program_info`.

Analysis-only: no tool, capture-rig, ISO or Ghidra-project change. Port write
set: `include/fifa96_loader/fifa96_match_pace.h`,
`src/fifa96_loader/fifa96_match_pace.c`, `tests/test_match_pace.c`,
`CMakeLists.txt` (one library/test block). `make test`: 48/48 before, **49/49
after**; ASan+UBSan `test_match_pace` clean (`cc -fsanitize=address,undefined
-Iinclude tests/test_match_pace.c src/fifa96_loader/fifa96_match_pace.c`).
`game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not staged.
