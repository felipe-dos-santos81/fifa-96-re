# FU-59: the INT-8 tick and periodic-callback machinery

Roadmap slice #1 of FU-58 §7. Derives the 100 Hz INT-8 handler, its
hardware sequence, the 8-slot callback table and the register/cancel API, and
maps the result onto the already-ported `fifa96_pacing` clock.

Result in one line: **`FUN_0009F5E4` increments the 100 Hz counter `0x12E88`
every interrupt, increments `0x12E8C` and chains the saved original INT-8
vector every 5th tick (20 Hz, no direct EOI), EOIs the PIC directly on the
other four ticks, and then calls every non-null of the 8 four-byte function
pointers in `0x5BAE4` in ascending slot order; entries are registered in the
first free slot by `FUN_0009F64C` (fatal `"addtimer - LIST FULL"` when full)
and removed by first-pointer match by `FUN_0009F684`; there are no
period/countdown fields in the original table.**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (Ghidra
  program `fifa96_le.bin`, flat link-time addresses as in FU-4/FU-37/FU-48).
  The handler sits in an analysis gap: the auto-disassembler mis-decodes
  `0x9F5F0..0x9F5F4`, so the instruction stream below is from
  `read_memory 0x9F5E4 210` and re-decoded by hand; the helper functions are
  `disassemble_bytes`; the registrars and callbacks are `decompile_function`
  plus `disassemble_bytes` at the call sites.
* **Address convention (errata to FU-37/48/58 nomenclature).** Code
  immediates that name callbacks, the callback table and the tick counters are
  *object-relative* values carrying LE fixups; the loader adds the owning
  object's load base at run time. In the flat image the resolved addresses are
  `encoded + link base` (object 1 base `0x10000`, object 4 base `0x100000`;
  FU-4). A general fixup query script written for this slice
  (`/tmp/opencode/fu59/lefix3.py`, not committed; record rule from FU-58) shows:

  | encoded in code | fixup target | resolved flat (Ghidra) |
  |--------:|--------------|-----------------------:|
  | `0x5BAE4` callback table | object 4 + `0x5BAE4` | `0x15BAE4` |
  | `0x12E88`, `0x12E8C` counters | object 4 + off | `0x112E88`, `0x112E8C` |
  | `0x12E90`/`0x12E94` saved vector | object 4 + off | `0x112E90`/`0x112E94` |
  | `0x85B03` audio callback | object 1 + `0x85B03` | `0x95B03` |
  | `0x5A0B2` device callback | object 1 + `0x5A0B2` | `0x6A0B2` |
  | `0x5D296` device callback | object 1 + `0x5D296` | `0x6D296` |
  | `0xA6A8F` music callback | object 1 + `0xA6A8F` | `0xB6A8F` |
  | `0x39320` match callback | object 1 + `0x39320` | `0x49320` |
  | `0x3644` error string | object 4 + `0x3644` | `0x103644` ("addtimer - LIST FULL\n") |
  | `0x8F5E4` ISR entry | object 1 + `0x8F5E4` | `0x9F5E4` |
  | `0x43DC` state table (FU-58 §2) | object 1 + `0x43DC` | `0x143DC` |

  FU-58's state-table address `0x43DC` and its handler targets are therefore
  object-relative/encoded values; the *targets* it computed with
  `0x10000 + off` (handlers `0x14451..0x14B28`) are already resolved, but the
  table itself is at `0x143DC`, immediately before the dispatcher `0x1442C`.
  Behaviourally none of this matters; citable addresses in this document are
  given as encoded values with the resolved flat value in parentheses.
* The on-disk `/tmp/opencode/fifa96_le.bin` (md5 `bc2a0fdd...`) no longer
  matches the Ghidra program's memory (md5 of the extracted
  `/tmp/opencode/fu58/FIFA96.EXE` is `9a461768d610121c2bb5869a7c4cfc9d`);
  every instruction/byte claim below is read back from Ghidra, not from that
  file. Program-wide call-site completeness used `search_instructions` on the
  program (191,684 instructions).

## 1. The tick handler `FUN_0009F5E4` (encoded entry; flat `0x9F5E4`)

Raw bytes `60 1e 06 0f a0 0f a8 89 e5 fc e8 e9 09 00 00 fb fc ...` decode to:

```
0x9F5E4  PUSHAD                     ; 60
0x9F5E5  PUSH DS                    ; 1e
0x9F5E6  PUSH ES                    ; 06
0x9F5E7  PUSH FS                    ; 0f a0
0x9F5E9  PUSH GS                    ; 0f a8
0x9F5EB  MOV  EBP,ESP               ; 89 e5
0x9F5ED  CLD                       ; fc
0x9F5EE  CALL 0x9F5F3+0x9E9=0x9FFDC ; e8 e9 09 00 00  (DS reload helper)
0x9F5F3  STI                       ; fb
0x9F5F4  CLD                       ; fc
0x9F5F5  MOV  EDX,[0x12E88]         ; 8b 15 88 2e 01 00
0x9F5FB  INC  EDX                   ; 42
0x9F5FC  MOV  EBX,5                 ; bb 05 00 00 00
0x9F601  MOV  [0x12E88],EDX         ; 89 15 88 2e 01 00
0x9F607  MOV  EAX,EDX               ; 89 d0
0x9F609  SAR  EDX,0x1F              ; c1 fa 1f
0x9F60C  IDIV EBX                   ; f7 fb
0x9F60E  TEST EDX,EDX               ; 85 d2   (remainder)
0x9F610  JNZ  0x9F621               ; 75 0f
0x9F612  INC  dword [0x12E8C]       ; ff 05 8c 2e 01 00
0x9F618  PUSHFD                     ; 9c
0x9F619  CALLF far [0x12E90]        ; ff 1d 90 2e 01 00
0x9F61F  JMP  0x9F629               ; eb 08
0x9F621  MOV  AL,0x20               ; b0 20
0x9F623  MOV  EDX,0x20              ; ba 20 00 00 00
0x9F628  OUT  DX,AL                 ; ee      (PIC master EOI)
0x9F629  XOR  EDX,EDX               ; 31 d2
0x9F62B  CMP  dword [EDX+0x5BAE4],0 ; 83 ba e4 ba 05 00 00
0x9F632  JZ   0x9F63C               ; 74 08
0x9F634  MOV  EAX,EDX               ; 89 d0
0x9F636  CALL near [EAX+0x5BAE4]    ; ff 90 e4 ba 05 00
0x9F63C  ADD  EDX,4                 ; 83 c2 04
0x9F63F  CMP  EDX,0x20              ; 83 fa 20
0x9F642  JNZ  0x9F62B               ; 75 e7
0x9F644  POP GS / POP FS / POP ES / POP DS / POPAD / IRETD
```

* **Counter.** `MOV EDX,[0x12E88]; INC EDX; MOV [0x12E88],EDX` at
  `0x9F5F5..0x9F601` is the only writer besides `0xCB2CB`
  (`MOV [0x12E88],EAX; RET`); there are exactly 10 xrefs to `0x12E88`
  (nine reads: `0xCB2A4/AA/B6/D1/E1/EF/FF` plus the ISR read at `0x9F5F5`;
  one write `0xCB2CB`), matching FU-58 §5.
* **Sub-counter and chain.** The incremented value is divided by 5
  (`MOV EBX,5; SAR EDX,0x1F; IDIV EBX`). When the remainder is zero
  (every 5th tick, 20 Hz) the handler increments `[0x12E8C]`, then
  `PUSHFD; CALLF far [0x12E90]` chains the saved original INT-8 vector. The
  `PUSHFD` supplies the flags slot that the chained handler's `IRET/IRETD`
  consumes, so it returns to `0x9F61F`; the chained BIOS handler performs the
  EOI. On the other four ticks the handler writes `0x20` to port `0x20`
  itself (`0x9F621..0x9F628`, `MOV AL,0x20; MOV EDX,0x20; OUT DX,AL`).
* **Dispatch.** `0x9F629..0x9F642` iterates `EDX = 0,4,...,0x1C` (eight
  4-byte slots starting at `0x5BAE4`), skips null entries, sets `EAX = EDX`
  (the slot byte offset) and does a near `CALL [EAX+0x5BAE4]`. All five
  registered callbacks ignore the incoming `EAX` (section 4), so the value is
  an addressing artifact, not an argument.
* Dispatch runs on every tick, including the 20 Hz chain ticks, after the
  EOI/chain path rejoins at `0x9F629`. The handler never touches any other
  counter. `0x9FFDC` is the Watcom DS-reload helper
  (`MOV DS,CS:[0x8FFE5]; RET`, flat disp `0x9FFE5`).
* `[0x12E88]` is the 100 Hz centisecond clock already modelled by
  `fifa96_pacing_clock`; `[0x12E8C]` is the 20 Hz sub-counter read by
  `FUN_000CE580` (two reads; consumer not derived — open leg).

## 2. Callback table `0x5BAE4` (flat `0x15BAE4`)

* Eight slots of **4 bytes each** (0x20 bytes total), each a near function
  pointer; object 4 (data/BSS), so zero at image start. There is no
  period, countdown, user or priority field: the field set is exactly one
  function pointer per slot. The per-callback cadence is implemented inside
  each callback (section 4).
* Every reference to the table in the program is in the handler
  (`0x9F62B`, `0x9F636`), the register/cancel/count helpers
  (`0x9F64C`, `0x9F684`, `0x9F6B4`) and the installer's zero fill
  (`0x9F759`) — verified with `search_instructions` operand `5bae4`
  (10 matches) and `get_xrefs_to 0x5BAE4`. No other writer exists.
* The installer `FUN_0009F754` zeroes it via `0x9E907` with
  `ECX=8, EAX=0x5BAE4, EDX=0` (`0x9F754..0x9F760`), i.e.
  `memset(table, 0, 8*4)`; `0x9E907` is a Watcom dword-fill.

### Errata (FU-58 §5, quoted)

* "xrefs to `0x5BAE4` are exactly these two functions plus the handler" — the
  table is also read by a third helper, `FUN_0009F6B4` (count), and cleared by
  the installer; and there are **five** register call sites, not four (the
  match-module site at `0x49465` was in an undisassembled gap and only
  appeared after `disassemble_bytes 0x49430`; `search_instructions` then
  reports 5).
* "zeroes `[0x12e8c]`" — the installer zeroes the **callback table**; the two
  counters are zero by virtue of being BSS/image zero. No ISR or installer
  instruction writes `[0x12E8C]` except `INC` at `0x9F612`.
* The registrar immediates it quotes (`LAB_00085B03`, `0x5A0B2`, `[0x5D296]`,
  `LAB_000A6A8F`) are encoded object-relative values; see the Method table for
  flat resolutions.

## 3. Registry API

### 3.1 Register — `FUN_0009F64C`

`disassemble_bytes 0x9F64C`:

```
0x9F64C  MOV  EDX,[0x5BAE4]      ; test slot 0 first
0x9F652  XOR  EAX,EAX            ; EAX = slot byte offset
0x9F654  TEST EDX,EDX
0x9F656  JNZ  0x9F663
0x9F658  MOV  EDX,[ESP+4]        ; cdecl argument: callback pointer
0x9F65C  MOV  [EAX+0x5BAE4],EDX  ; store into first free slot
0x9F662  RET                      ; returns EAX = slot byte offset
0x9F663  ADD  EAX,4 / CMP EAX,0x20 / JGE 0x9F676
0x9F66B  CMP  dword [EAX+0x5BAE4],0 / JZ 0x9F658 / JMP 0x9F663
0x9F676  PUSH 0x3644             ; encoded -> flat 0x103644
0x9F67B  CALL 0xCBBE8            ; "addtimer - LIST FULL\n" reporter
0x9F680  ADD  ESP,4 / RET
```

Semantics: `int register(fn)` scans slots `0..7` for the first null, stores
`fn`, and returns the slot's byte offset `0,4,..,0x1C`; when the table is
full it calls `FUN_000CBBE8` with the flat string at `0x103644`
(`"addtimer - LIST FULL\n"`, read with `inspect_memory_content 0x103644`).
There is no priority and no ordering key: insertion order determines nothing,
slot index determines dispatch order, and reuse is LIFO-of-holes only because
the first hole is always taken.

### 3.2 Cancel — `FUN_0009F684`

```
0x9F684  PUSH ESI
0x9F685  MOV  EDX,[ESP+8]        ; cdecl argument: callback pointer
0x9F689  MOV  EBX,[0x5BAE4]      ; slot 0
0x9F68F  XOR  EAX,EAX
0x9F691  CMP  EDX,EBX / JNZ scan
0x9F695  XOR  ESI,ESI / MOV [EAX+0x5BAE4],ESI / POP ESI / RET
0x9F69F  ADD EAX,4 / CMP EAX,0x20 / JGE done
0x9F6A7  CMP EDX,[EAX+0x5BAE4] / JZ 0x9F695 / JMP 0x9F69F
0x9F6B1  POP ESI / RET          ; not found: silent, no error
```

Semantics: `void cancel(fn)` clears the **first** slot whose pointer equals
`fn`; the table is not compacted and a missing pointer is silently ignored.
Cancel-by-pointer means a callback registered twice occupies two slots and one
cancel removes only the lower one.

### 3.3 Count — `FUN_0009F6B4`

`0x9F6B4..0x9F6CC`: walks the eight slots, counts non-null entries, returns
the count in `EAX`. `search_instructions CALL 9f6b4` finds **no callers**
(unreferenced helper; open leg). It is the natural model for a port-side
"active callback count".

### 3.4 Call sites (program-wide search)

Register (`CALL 0x9F64C`, 5):
`0x49465` (match module; pushes `0x39320`/flat `0x49320`),
`0x6A42C` (`FUN_0006A2AB`; `0x5A0B2`/flat `0x6A0B2`),
`0x6D769` (`FUN_0006D742`; `0x5D296`/flat `0x6D296`),
`0x94B27` (`FUN_00094A48`; `0x85B03`/flat `0x95B03`),
`0xA6293` (`FUN_000A6265`; `0xA6A8F`/flat `0xB6A8F`).

Cancel (`CALL 0x9F684`, 6):
`0x4955A` (match; `0x39320`),
`0x6A5A8` (`FUN_0006A56A`; `0x5A0B2`),
`0x6D79F` (`FUN_0006D778`; `0x5D296`),
`0x94DE0`/`0x94E1F` (`FUN_00094DC4`/`FUN_00094E03`; `0x85B03`),
`0xA6517` (`FUN_000A6505`; `0xA6A8F`).

### 3.5 Install / post-install / uninstall

`FUN_0009F754` (encoded entry; flat `0x9F754`), `disassemble_bytes`:

1. `memset(table, 0, 32)` (`0x9F754..0x9F760`, via `0x9E907`).
2. Idempotence guard **after** the zeroing: if `[0x12E90]!=0` or
   `[0x12E94]!=0`, return (`0x9F765..0x9F776`). Calling the installer twice
   therefore wipes one registration; both observed callers call it once.
3. Mask IRQ0/IRQ1 at the PIC (`IN AL,0x21; OR AL,3; OUT 0x21`).
4. Save the current INT-8 vector: `FUN_000AE9A0` is the DOS/DPMI
   `INT 21h AH=35h` get-vector wrapper; result offset -> `[0x12E90]`,
   segment -> `[0x12E94]` (`0x9F778..0x9F794`).
5. Program PIT channel 0 with divider bytes `0x9C` then `0x2E`
   (`OUT 0x40`), i.e. divisor `0x2E9C` = 11932 -> 1193182/11932 = 100.0 Hz
   (`0x9F799..0x9F7AA`).
6. Install the new vector 8 as `CS:0x8F5E4` (stack offset `0xF5E4` within the
   code selector; flat `0x9F5E4`) through `FUN_000AE9D4` (the
   `INT 21h AH=25h` set-vector wrapper) (`0x9F7AB..0x9F7B0`).
7. Register the uninstall function `0x9F6F0` (flat) through `FUN_000CBDA0`
   (`PUSH 0x8F6F0` fixup -> `0x9F6F0`) (`0x9F7B5..0x9F7BF`). `FUN_000CBDA0`
   forwards to `FUN_000B25DF`, which stores the pointer in a 0x20-entry
   function-pointer table at `0x6182C` (returning `-1` when full) before
   setting `DAT_00014688 = 0xA25A8` — an exit/callback hook registrar.
8. Unmask IRQ0/IRQ1 (`IN AL,0x21; AND AL,0xFC; OUT 0x21`).

Callers: `0x17CD1` and `0xB24CC`, both `PUSH 0x64` (100) then `CALL`; the
function never reads `[ESP+4]` in its listing, so the argument is unused.
Both call `FUN_0009F7DD` (flat `0x9F7DD`: `AND byte [0x417],0xDF;
CALL 0xCB78F; RET`) immediately after, and `FUN_0009F754` is in turn only
reachable from those two sites.

`0x9F6F0` (uninstall): guard saved vector non-null; mask IRQ0/1; restore
vector 8 from `CX=[0x12E94]`, `EBX=[0x12E90]` via `FUN_000AE9D4`; reset the
PIT (`MOV AL,0; OUT 0x40` twice); clear `[0x12E90]`/`[0x12E94]`; unmask.
`search_instructions CALL 9f6f0` finds no direct caller — it is only handed
to `0xCBDA0` as the exit hook.

## 4. The registered callbacks

All five callbacks start with a Watcom `PUSH EBX/ECX/EDX/ESI/EDI/EBP;
MOV EBP,ESP` prologue and never read the incoming `EAX`; four of them
self-gate on a re-entry/once flag.

| encoded / flat | registrar (gate) | cancel site | behaviour (evidence) |
|---|---|---|---|
| `0x85B03` / `0x95B03` | `FUN_00094A48` (none) | `0x94DE0`, `0x94E1F` | `MOV EAX,[0x5B7E4]; SUB dword [EAX+0xEB],0xA; INC dword [0x11174]` (`0x95B11..0x95B1D`). Audio-stream service: decrements a stream field by 10 and bumps the object-4 counter at `0x11174`/`0x111174` every 100 Hz tick. `FUN_00094A48` is the stream constructor (callers `FUN_0006572C`, `FUN_00094B3E`); `[0x5B7E4]` is the object it builds. |
| `0x5A0B2` / `0x6A0B2` | `FUN_0006A2AB` (`[0xDE40]!=0`) | `0x6A5A8` (same gate) | Gates on `[0xDE80]==2 && [0xDE24]==0`, then `[0xDE24]++`, writes `0x63` into the byte buffer at `EAX+0xDEA0`, calls `FUN_00069F9A/0x6A999/0x69DE8` when `[0xDE3C]!=0`, decrements `[0xDE84]` (`0x6A0C0..0x6A125`). Device/CD command pump. |
| `0x5D296` / `0x6D296` | `FUN_0006D742` (`[0x5756C]==0` once) | `0x6D79F` | Gates on `[0xE11C]==0`, sets it 1, conditionally calls `FUN_0006A7B0` (only when `[0x5754C]!=0` and the call succeeds), then calls `FUN_0006C42A`, clears the flag (`0x6D2A4..0x6D2E2`). Device poll path. |
| `0xA6A8F` / `0xB6A8F` | `FUN_000A6265` (`[0x148B0]==0` once) | `0xA6517` | Outer gate `[0x15FC4]==0 && [0x15FCE]==0`; sets `[0x15FC4]=1`, calls `0xB6AB3`, clears it. `0xB6AB3` services `[0x15FCC]`/`[0x15FCD]` (`CALL 0xA7EDF`/`A7136`), handles `[0x149CC]==1` (`CALL 0xB80FA([0x149D0])`), then walks a table at encoded `0x61994` (flat `0x71994`): byte `+0x16` type, signed dword `+0x18` increment, dword `+0x1C` accumulator, dword `+0x20` limit, adding the increment to the accumulator each tick (`0xB6B00..0xB6B2E`). Music/MIDI mode service; `FUN_000A6265(mode 0..5)` selects an entry of the `0x14830` table (stride `0x14`) and stores `[0x15FC8]`; `FUN_000A6505` cancels, clears `[0x15FC8]`, calls `FUN_000BA00E(0..0xF)` and `FUN_000A765F`. |
| `0x39320` / `0x49320` | match module, `0x49465` (none) | `0x4955A` | `CMP [0x7324],0` gate; `CALL 0x3F684` gate; `[0x7320]+=0x102`; when `>=0x35C`, `[0x731C]++` and `CALL FUN_000456AF`, subtract `0x35C` (`0x49320..0x49370`). Match-side progress tick. |

The original has no per-slot countdown: every registered callback is expected
to run at 100 Hz and implements its own cadence internally (the clearest
example is `0xB6A8F`'s `+0x18/+0x1C/+0x20` accumulator table). This is the
direct answer to "period/countdown/user?" — none of those fields exist.

## 5. Relation to `fifa96_pacing` (already ported)

* `fifa96_pacing_clock` (FU-48 §6) *is* the counter half of this handler:
  `ticks` = `[0x12E88]`, `ticks20` = `[0x12E8C]`, and
  `fifa96_pacing_clock_isr()` reproduces the increment plus the
  "divisible by 5" rule. No mismatch: the ISR listing here confirms
  `MOV EDX,[0x12E88]; INC EDX; MOV EBX,5; MOV [0x12E88],EDX;
  MOV EAX,EDX; SAR EDX,0x1F; IDIV EBX; TEST EDX,EDX` exactly.
* What the pacing port does **not** model is the callback registry, the EOI,
  and the every-5th-tick chain to the saved vector. Those split cleanly:
  hardware/vector concerns are out of scope for a hardware-free port; the
  registry is `fifa96_tick`. The port therefore *reuses* pacing:
  `fifa96_tick_isr(t, clock)` calls `fifa96_pacing_clock_isr(clock)` and then
  dispatches one tick, returning the 20 Hz flag.
* The same tick family feeds the already-ported wait/deadline model: the
  getters `0xCB2AA` (`now-then`, = `fifa96_pacing_clock_elapsed`), `0xCB2D1`
  (deadline = arg + now), `0xCB2E1` (signed wait loop), `0xCB2EF`
  (now >= deadline, = `fifa96_pacing_deadline_reached`) and the writer
  `0xCB2CB` (`[0x12E88]=EAX`). `0xCB2A4` is the `return [0x12E88]` getter;
  `FUN_000CB2B6` copies the counter to `[0x127D4]` and returns a constant-
  offset variant (not fully decoded); `0xCB2FF` reads the counter twice.
* Match-side code that FU-58 ties to this layer (`FUN_00049B28`,
  `FUN_00091DD8`, `FUN_00091F60`) uses the time getters/deadlines rather than
  the registry; the only match-module registry edge found is
  `0x49465`/`0x4955A` around `0x49320` (section 4).

## 6. Port: `fifa96_pacing`-backed tick scheduler

`include/fifa96_loader/fifa96_tick.h` + `src/fifa96_loader/fifa96_tick.c`
(caller-owned, no globals, `-fifa96_err_t`), plus `FIFA96_ERR_FULL` /
`FIFA96_ERR_INVALID` in `fifa96_err.h`. `fifa96_pacing.c` was split out of
the `fifa96_mixer` library so `fifa96_tick` can link it directly
(`CMakeLists.txt`); `test_mixer`/`test_audio_edge` now link `fifa96_pacing`
explicitly.

| original | port |
|---|---|
| 8 slots × 4-byte fn, first-empty insert, asc. dispatch | `FIFA96_TICK_SLOTS` (8), `fifa96_tick_register` returns slot index, `fifa96_tick_advance`/`_isr` dispatch slots 0..7 |
| every callback every tick | `period == 1` gives exactly that; `period` default is explicit (original has none) |
| count `0x9F6B4` | `fifa96_tick_active` |
| first-pointer cancel, no compaction, silent | `fifa96_tick_cancel`; returns `-FIFA96_ERR_NOT_FOUND` when absent (original is silent) |
| register full -> `"addtimer - LIST FULL"` fatal | `-FIFA96_ERR_FULL` |
| fresh per-slot read each iteration | dispatch re-reads `slots[i].fn` each iteration, so register/cancel from a callback affects the rest of the same tick (mirrors the ISR's `CMP`/`CALL` on the table) |
| `PUSHFD; CALLF [0x12E90]` 20 Hz chain; EOI | not modelled (hardware); `fifa96_tick_isr` returns the pacing 20 Hz flag |
| `EAX` = slot byte offset at call time | callback receives `void *user` (all original callbacks ignore `EAX`) |

Port extension for the task's "period/countdown": `period == 0` is a one-shot
(fires on the next tick, then unregisters); `period >= 1` fires on the next
tick and then every `period` ticks (`countdown` starts at 1; after a firing it
resets to `period`). This is a deliberate generalization: the original table
carries no periods, so `period == 1` is the faithful model and
`fifa96_pacing_clock_isr` + `fifa96_tick_advance(t, 1)` reproduce the ISR.

## 7. Tests

`tests/test_tick.c` (suite 47 -> 48). Pins: init/active, first-empty insert,
no compaction, 8-slot capacity and `-FIFA96_ERR_FULL`, NULL handling, slot-
order dispatch after cancel+reuse, user pointer delivery, period/countdown
arithmetic (period 1 every tick, period 3 fires at ticks 1/4/7/10, large
period boundary `0x10000`), one-shot auto-unregister, self-cancel during
dispatch, cross-cancel suppressing a later slot in the same tick, late
registration running in the same tick, and `fifa96_tick_isr` integration with
`fifa96_pacing_clock` (5 ISRs -> `ticks=5`, `ticks20=1`, 5 callback runs,
20 Hz flag on the fifth, NULL-safe). ASan/UBSan build of `test_tick` is clean.

## 8. Open legs

* `FUN_0009F6B4` (count) has no callers; `0xCE580`, which reads the 20 Hz
  counter `[0x12E8C]` twice, is not identified.
* The match-module register/cancel sites `0x49465`/`0x4955A` live in an
  undefined function inside the `0x49xxx` analysis gap; its boundary and the
  full `0x49320` semantics (the `0x731C`/`0x7320`/`0x7324` block) are open.
* Callback bodies are only derived down to their gates and inner calls
  (`0x6A0B2`, `0x6D296`, `0xB6A8F`, `0x49320`); the called helpers
  (`0x69F9A`, `0x6A999`, `0x69DE8`, `0x6A7B0`, `0x6C42A`, `0xA7EDF`,
  `0xA7136`, `0xB80FA`, `0x456AF`, ...) are not decomposed here.
* `FUN_000CB2B6`'s constant-offset clock variant is decompiled but not
  cleanly line-decoded.
* The installer's idempotence guard sits after the table zeroing, so the
  function is not safe to call twice; this is likely dead-code defensive
  coding but is preserved as an observation, not a port claim.
* The on-disk `/tmp/opencode/fifa96_le.bin` no longer matches the Ghidra
  program (method section); regenerating it would not reproduce the flat
  layout used by the FU-1..58 docs because the tool now writes a different
  image.

## Provenance

Ghidra MCP on `fifa96_le.bin` (program `fifa96_le.bin`):
`read_memory 0x9F5E4` (210 bytes, handler decode), `read_memory 0x12E88`,
`inspect_memory_content 0x3644`, `0x103644`, `0x8F6F0`, `0x9F6F0`;
`disassemble_bytes` 0x9F5E4, 0x9F64C, 0x9F684, 0x9F6B4, 0x9F6F0, 0x9F754,
0x9F7DD, 0x9FFDC, 0xAE9A0, 0xAE9D4, 0x6D742, 0x5D296, 0x5A0B2, 0x49430,
0x494E0, 0x49320, 0x496B0, 0x95B03, 0x6A0B2, 0x6D296, 0xB6A8F, 0x17C90,
0xB2490, 0x1442C; `decompile_function` 0x94A48, 0x94B3E, 0x6572C, 0x94DC4,
0x94E03, 0x6A2AB, 0x6A56A, 0x6D742, 0x6D778, 0xA6265, 0xA6505, 0x9E907,
0xCBBE8, 0xCBDA0, 0xCB2A4, 0xCB2AA, 0xCB2B6, 0xCB2D1, 0xCB2E1, 0xCB2EF,
0xCB2FF, 0xCE580; `get_xrefs_to` 0x9F64C, 0x9F684, 0x9F6B4, 0x5BAE4,
0x12E88, 0x12E8C, 0x9F754, 0x94A48, 0x6A2AB, 0x6D742, 0xA6265, 0x4955A;
`search_instructions` CALL 0x9F64C/0x9F684/0x9F6B4/0x9F754/0x9F6F0 and
operand `5bae4`; `search_byte_patterns` `68 20 93 03 00`.
Fixup decoding: `/tmp/opencode/fu59/lefix3.py`, `scan_table.py` (not
committed; built on FU-58's rule, `/tmp/opencode/fu58/FIFA96.EXE`, md5
`9a461768d610121c2bb5869a7c4cfc9d`).

`make test`: 47/47 before, 48/48 after (new `test_tick`); ASan+UBSan
`test_tick` clean. No tool, capture-rig or ISO change; `game/FIFAPCCD96.iso`
untouched. `fifa96.rep/**` churn was not staged.
