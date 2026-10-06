# FU-134 — no-caller triage of the `FUN_000a6*` region and the `FUN_000a6a03` call site

Cycle 2, Task 5. Follows FU-133 (loader bridge) and FU-130/FU-131 (the LE
address model). Question: what is the role of `FUN_000a6a03` at its sole caller
`0x64fb2`, and is any function in the bounded `^FUN_000a6` sample a genuine
no-caller?

All results below are from the **authoritative native program `/FIFA96.EXE`**
(Ghidra program 00000002, `watcom:LE:32:default`, fixups applied → real data
addresses), except where a line explicitly names the flat cross-reference
program `/fifa96_le.bin` (offset form). The two programs' *code* is
offset-identical; only data operands differ (FU-130/FU-131).

Result in one line: **`FUN_000a6a03` is the EACS sound-library error-code→string
resolver, called once from `FUN_00064f70`@`0x64fb2` on the audio-driver-open
failure path; the entire bounded `^FUN_000a6` sample (28 native functions) is
statically reachable — the only no-caller, `FUN_000a610c` (defined in
`/fifa96_le.bin`, left as unnamed code in `/FIFA96.EXE`), is a library-internal
EACSNDF.LIB math helper with no static xref in either program.**

## 1. `FUN_000a6a03` call-site analysis

`ghidra_get_function_xrefs(program="/FIFA96.EXE", address="0xa6a03")` returns
exactly one reference:

| site | enclosing function | reference | containing body |
|---|---|---|---|
| `00064fb2` | `FUN_00064f70` | `UNCONDITIONAL_CALL` | `0x64F70..0x64FFA` |

`ghidra_get_bulk_xrefs(program="/FIFA96.EXE", addresses="0xa6a03,0x64fb2")` →
`{"0xa6a03":[{"from":"00064fb2","type":"UNCONDITIONAL_CALL"}],"0x64fb2":[]}`
(a single caller; the call site itself has no xref to it in this bulk set).

The flat program agrees on the call site:
`get_bulk_xrefs(program="/fifa96_le.bin", addresses="0xa6a03")` →
`{"from":"00064fb2","type":"UNCONDITIONAL_CALL"}` — same code offset, so the
census caller is real, not a program-specific analysis artifact.

### 1.1 `FUN_000a6a03` is an error-code → message-string resolver

`ghidra_decompile_function(program="/FIFA96.EXE", address="0xa6a03")` completed
without truncation (66 instructions) and shows an exact switch on a signed
negative error code, returning `CONCAT44(param_2, "<string>")`:

| `param_1` | string | string address |
|---|---|---|
| `0xFFFFFFEC` | `No free channel to play on` | `0x1039E9` |
| `0xFFFFFFED` | `Error: invalid sfx number` | `0x1039CF` |
| `0xFFFFFFEE` | `Error: invalid bank number` | `0x1039B4` |
| `0xFFFFFFEF` | `Error: could not resolve sfx bank` | `0x103992` |
| `0xFFFFFFF0` | `Error: invalid bend` | `0x10397E` |
| `0xFFFFFFF1` | `Error: unsupported audio type …` | `0x103952` |
| `0xFFFFFFF2` | `Error: invalid pan` | `0x10393F` |
| `0xFFFFFFF3` | `Error: invalid handle` | `0x103929` |
| `0xFFFFFFF4` | `Error: invalid volume` | `0x103913` |
| `0xFFFFFFF5` | `Error: invalid channel` | `0x1038FC` |
| `0xFFFFFFF6` | `Error: not an EACS format file` | `0x1038DD` |
| `0xFFFFFFF7` | `Error in DMA channel` | `0x1038C8` |
| `0xFFFFFFF8` | `Error initializing software mixer` | `0x1038A6` |
| `0xFFFFFFF9` | `Error initializing card` | `0x10388E` |
| `0xFFFFFFFA` | `Error allocating memory` | `0x103876` |
| `0xFFFFFFFB` | `Card not detected` | `0x103864` |
| `0xFFFFFFFC` | `Invalid driver` | `0x103855` |
| `0xFFFFFFFD` | `Error in environment variable` | `0x103837` |
| `0xFFFFFFFE` | `Environment variable not set` | `0x10381A` |
| default | `Unknown error` | `0x10380C` |

`get_function_signature` confirms a leaf: `call_count: 0`, `param_count: 2`,
`string_ref_count: 21`, 22 basic blocks (one per switch arm).

### 1.2 The containing function `FUN_00064f70` is the audio-driver init

`ghidra_disassemble_function(program="/FIFA96.EXE", address="0x64f70")`
(42 instructions) shows the caller's structure. The decisive slice:

```
00064f70  PUSH EBX / PUSH ECX / PUSH EDX
00064f73  CMP dword ptr [0x00155ce0],0x0
00064f7a  JNZ 0x00064ff2
00064f80  MOV EAX,0x1
00064f85  CALL 0x00068cfc            ; read driver/config index
00064f8a  MOV [0x00155ce4],EAX
00064f8f  TEST EAX,EAX
00064f91  JLE 0x00064ff2
00064f93  MOV EDX,0xffffffff
00064f98  CALL 0x000a6265            ; EACS driver open -> negative on failure
00064f9d  TEST EAX,EAX
00064f9f  SETGE DL                   ; DL = (ret >= 0)
00064fa2  AND EDX,0xff
00064fa8  MOV dword ptr [0x00155ce0],EDX   ; latch "started"
00064fae  TEST EAX,EAX
00064fb0  JGE 0x00064fc0
00064fb2  CALL 0x000a6a03            ; <-- resolve the negative code to a string
00064fb7  PUSH EAX                   ; string pointer
00064fb8  CALL 0x000cbbe8            ; report/teardown
00064fbd  ADD ESP,0x4
00064fc0  ...initialize 6 channel slots via FUN_0006504c(0..6)...
```

`ghidra_decompile_function` corroborates: `DAT_00155ce0` is a latched
"driver started" flag; `FUN_00068cfc(1)` returns a config/driver byte
(`FUN_00068cfc`: `*((int*)&DAT_00156400 + param_1 + 1) >> 24`); on failure
(`iVar1 < 0` from `FUN_000a6265`) the code calls
`FUN_000a6a03(iVar1, DAT_00155ce0=0)` and passes the returned string to
`FUN_000cbbe8`.

`FUN_000a6265` (decompiled) is the EACS driver-open routine: it returns `-4`
(`0xFFFFFFFC`) when `param_1 < 0 || 5 < param_1` (i.e. an invalid driver index)
— which maps exactly to `FUN_000a6a03`'s `0xFFFFFFFC` case `"Invalid driver"` —
otherwise it dispatches `(**(code **)(&DAT_00114830 + param_1 * 0x14))(param_2)`
and returns that routine's negative error code, all of which are in
`FUN_000a6a03`'s switch.

**Role at the call site: `FUN_000a6a03` converts the negative driver-open error
code into the corresponding human-readable EACS message string, which
`FUN_000cbbe8` then reports/tears down.** `FUN_00064f70` itself is the EACS
audio-init entry, invoked from game code `FUN_00017b78`@`0x17D34` and
`FUN_000679f4`@`0x67A33` (`get_function_xrefs(0x64f70)`).

### 1.3 Library identity

`search_strings(program="/FIFA96.EXE", search_term="EACS|Miles|AIL|driver|mixer")`
returns the banner at `0x1036EC`:

```
"\nEACSNDF.LIB 32 Bit Sound Library - Build: Sep 12 1995 20:17:58\r\n(c)1994-95, EAC Inc. by DM, AS, PC, IM, KH, BP, et al.\r"
```

(`read_memory 0x1036EC` returns those bytes verbatim; the banner is `DATA`
referenced from `0x1148A8`.) The `FUN_000a6*` region, the `0x1038xx` error
strings above, `FUN_000a6265`, and `FUN_000a6a03` all belong to this embedded
**EACSNDF.LIB 32-bit Sound Library**, not to game logic. `FUN_000a6a03` is
therefore a **library-internal** helper that happens to have one in-image caller
— it is **not** a no-caller function.

## 2. Bounded triage sample — `^FUN_000a6`

`ghidra_search_functions_enhanced(program="/FIFA96.EXE", name_pattern="^FUN_000a6", regex=true, limit=100)`
returns **28** functions. Cross-checked with
`get_bulk_xrefs(program="/FIFA96.EXE", addresses="<all 28>")` (counts agree).

| function | xref_count (native) | classification | evidence |
|---|---|---|---|
| `FUN_000a6040` | 2 | library-internal | EACS region; `CALL` from `0x59685`, `0x68164` |
| `FUN_000a60a0` | 4 | library-internal | EACS region; `CALL` from `0x5aef1`, `0x5aefe`, `0x5af0b`, `0x91ed2` |
| `FUN_000a6180` | 3 | library-internal | EACS region; `CALL` from `0x60e69`, `0x60ef2`, `0x62aff` |
| `FUN_000a61b0` | 2 | library-internal | EACS region; `CALL` from `0xa625c`, `0xa6244` |
| `FUN_000a6250` | 1 | library-internal | EACS region; `CALL` from `0x63f98` |
| `FUN_000a6265` | 1 | library-internal | EACS driver-open; `CALL` from `0x64f98` (§1.2) |
| `FUN_000a62fa` | 1 | library-internal | EACS region; `CALL` from `0xa7825` |
| `FUN_000a64ac` | 1 | library-internal | EACS region; `CALL` from `0xa62ee` |
| `FUN_000a64f7` | 1 | library-internal | EACS region; `CALL` from `0xb6b7b` |
| `FUN_000a6505` | 2 | library-internal | EACS region; `CALL` from `0xa6280` **and** `DATA` ref from `0xa62ae` (vtable slot `&DAT_00114830 + idx*0x14`, §1.2) |
| `FUN_000a6579` | 3 | library-internal | EACS region; `CALL` from `0xa7993`, `0xa7d27`, `0xba4ca` |
| `FUN_000a662c` | 1 | library-internal | EACS region; `CALL` from `0xa65d7` |
| `FUN_000a66ab` | 1 | library-internal | EACS region; `CALL` from `0xb8136` |
| `FUN_000a6717` | 1 | library-internal | EACS region; `CALL` from `0xa7775` |
| `FUN_000a67f0` | 1 | library-internal | EACS region; `CALL` from `0xa6cc7` |
| `FUN_000a684a` | 4 | library-internal | EACS region; `CALL` from `0xb5a5f`, `0xb5e0e`, `0xb61c9`, `0xb66c3` |
| `FUN_000a68d9` | 2 | library-internal | EACS region; `CALL` from `0xa6979`, `0xa69a7` |
| `FUN_000a6937` | 1 | library-internal | EACS region; `CALL` from `0xa6992` |
| `FUN_000a6972` | 1 | library-internal | EACS region; `CALL` from `0x68a9e` |
| `FUN_000a6a03` | 1 | library-internal | EACS error-string resolver; `CALL` from `0x64fb2` (§1) |
| `FUN_000a6aa6` | 5 | library-internal | EACS region; `CALL` from `0x65b4c`, `0x67eda`, `0x65b94`, `0x6553b`, `0x65bc5` |
| `FUN_000a6b21` | 6 | library-internal | EACS region; `CALL` from `0xb6b4e`, `0xa6ff3`, `0x65420`, `0x65b16`, `0xa807d`, `0xa6dd0` |
| `FUN_000a6bb3` | 1 | library-internal | EACS region; `CALL` from `0x654c3` |
| `FUN_000a6c48` | 1 | library-internal | EACS region; `CALL` from `0xa700a` |
| `FUN_000a6cdc` | 15 | library-internal | EACS region; `CALL` from 15 sites (`0x655e6`, `0x65bf4`, …, `0x67f42`) |
| `FUN_000a6de2` | 1 | library-internal | EACS region; `CALL` from `0xa7f06` |
| `FUN_000a6e1f` | 2 | library-internal | EACS region; `CALL` from `0x6557f`, `0xa7f78` |
| `FUN_000a6ead` | 1 | library-internal | EACS region; `CALL` from `0xa71b3` |

**No function in the native sample has `xref_count == 0`.** The no-caller subset
is therefore **empty in `/FIFA96.EXE`**. The only `xref_count == 0` entry in the
whole sample appears on the *flat* program (§3).

The `FUN_000a6505` `DATA` reference is the only indirect entry in the sample
(it is stored in the EACS driver vtable `&DAT_00114830 + param_1*0x14` used by
`FUN_000a6265`) — but it also has a direct `CALL` from `0xa6280`, so it is
**not** an indirect-only function.

## 3. The single no-caller: `FUN_000a610c` (flat-program artifact)

`ghidra_search_functions_enhanced(program="/fifa96_le.bin", name_pattern="^FUN_000a6", regex=true, limit=100)`
returns **29** functions — one more than native:

| function | xref_count (flat) | classification | evidence |
|---|---|---|---|
| `FUN_000a610c` | **0** | library-internal | EACSNDF.LIB region; no static caller in either program; native equivalent is unnamed code |

Evidence for the discrepancy:

* Flat: `get_function_by_address(program="/fifa96_le.bin", address="0xa610c")`
  → `FUN_000a610c`, body `0xA610C..0xA617A`, signature
  `int FUN_000a610c(uint param_1)`. Its xref set is empty
  (`get_bulk_xrefs("/fifa96_le.bin", "0xa610c")` → `[]`).
* Native: `get_function_by_address(program="/FIFA96.EXE", address="0xa610c")`
  → **`{"error":"No function found for 0xa610c"}`**. Native's
  `search_functions_enhanced` does not list it.
* The bytes are present in native and start a valid prologue+body:
  `disassemble_bytes(program="/FIFA96.EXE", start="0xa60f0", length=160)` shows
  `FUN_000a60a0` ending `POP ESI`/`RET` at `0xA610A`, a 1-byte `NOP` at
  `0xA610B`, then a coherent routine `0xA610C` (`PUSH ESI` … `POP ESI`/`RET` at
  `0xA617A`), then two padding bytes `00 00`, then `FUN_000a6180` @`0xA6180`.
  The flat `FUN_000a610c` decompile is the same fixed-point/phase-interpolation
  math kernel as `FUN_000a60a0` (table `DAT_00114e04`, `IMUL … 0x6487E`,
  `SAR … 0xA15`), i.e. a library sibling helper.
* Native has **no** static reference to it either:
  `get_bulk_xrefs("/FIFA96.EXE","0xa610a,0xa610c,0xa617a")` → all empty, and
  `search_instructions(program="/FIFA96.EXE", operand_pattern="a610c")` → 0 hits
  across 234,609 instructions. Flat's empty xref set matches.

**Verdict:** `FUN_000a610c` is a genuine EACSNDF.LIB internal math helper with
**no static caller in either program**. The flat program is the only one that
defines it as a function; native `/FIFA96.EXE` leaves the identical body at
`0xA610C..0xA617A` as unnamed code. Classification: **library-internal** (not
game/RE code, and not runtime-only). The absence of any static xref means it is
either reached indirectly (a computed/table call not present as a direct
reference) or is dead in this build — this is left as an open leg (§4).

## 4. Open legs

1. **`FUN_000a610c` reachability.** It has no static `CALL`/`JMP`/`DATA`
   reference in either `/FIFA96.EXE` or `/fifa96_le.bin`, yet the body is
   coherent. Whether it is (a) called through an EACS interpolation/vtable
   pointer not materialised as a Ghidra reference, or (b) dead code in this
   build, is unresolved. A full scan for its address in the `0x114xxx` driver
   vtables / `0x114e04` table area did not find a match.
2. **Native analysis gap.** `/FIFA96.EXE` does not define a function at
   `0xA610C` although `/fifa96_le.bin` does (identical bytes). That is a native
   auto-analysis gap, not a data-model issue; a `create_function` at `0xA610C`
   would restore parity but was **not** performed here (out of scope, and no
   save is required to change it).
3. **Triage is bounded.** Only the `^FUN_000a6` sample was classified. A
   program-wide no-caller census (2,706 native functions) was not run; other
   regions may hold genuine no-callers.
4. **`FUN_000cbbe8` internals.** It is identified only as the consumer of the
   resolved error string (report/teardown); its exact output path
   (`PTR_FUN_001131a4`, `DAT_00112e64`) was not decompiled.
5. **`FUN_000a6265` driver table.** The vtable `&DAT_00114830 + idx*0x14`
   (6 entries, idx 0..5) was inferred from `FUN_000a6265`; individual driver
   routines were not enumerated.

## 5. Provenance

* **`/FIFA96.EXE` (native, authoritative)** — `get_function_xrefs` `0xa6a03`,
  `0x64f70`; `get_bulk_xrefs` `0xa6a03`, `0x64fb2`, `0x1036ec`, the 28-address
  `^FUN_000a6` set, `0xa610a/0xa610c/0xa617a`; `get_function_by_address`
  `0x64fb2`→`FUN_00064f70`, `0xa610c` (no function), `0xa60a0`, `0xa6180`,
  `0xa6040`, `0xa6250`, `0x679f4`, `0x17b78`; `get_function_signature`/`callees`
  `0xa6a03`; `decompile_function` `0x64f70`, `0xa6a03` (66 instr, no
  truncation), `0xa6265`, `0x68cfc`, `0xcbbe8`, `0xa60a0`; `disassemble_function`
  `0x64f70` (42 instr); `disassemble_bytes` `0xa60f0` (160 B); `read_memory`
  `0x1036ec`; `search_strings` `EACS|Miles|AIL|driver|mixer`;
  `search_instructions` `a610c` (0 hits, 234,609 scanned).
* **`/fifa96_le.bin` (flat cross-reference)** — `search_functions_enhanced`
  `^FUN_000a6` (29, incl. `FUN_000a610c` xref 0); `get_function_by_address`
  `0xa610c`, `0xa60a0`; `get_bulk_xrefs` `0xa610c`, `0xa6a03`, `0xa64f7`,
  `0xa62fa`; `decompile_function` `0xa610c`.
* **Program-target note.** `/fifa96.exe` and `/FIFA96.EXE` collide
  case-insensitively in the MCP `program=` argument (FU-133 §notes); every
  native result above passed `program="/FIFA96.EXE"` explicitly.
* Address convention and library identity per FU-129/FU-130/FU-131/FU-133.
