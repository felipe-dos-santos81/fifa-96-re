# FU-216 — `FUN_0004B02C`: the range-word reader + sit-8 dispatch (phase-20 wave R64)

Provenance: M2 phase-20 wave **R64**, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE`,
path `/FIFA96.EXE`, `watcom:LE:32:default`, `min_address` `0x10000`,
`max_address` `.image::00174a2a`, 2706 functions, `.image` overlay present). Every
address/byte below is a fresh `read_memory` / `disassemble_bytes` /
`disassemble_function` / `get_xrefs_to` / `get_function_by_address` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named); the
engine source was read first-hand. The alias traps are applied (`dword[x]>>16` =
word at `x+2`, `>>24` = byte at `x+3`); addresses are `/FIFA96.EXE` runtime
addresses (LE globals sit at `+0x100000` from the file offset). **No writes**: no
rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; the ISO/HDD untouched; no git commands. Model: FU-208.

Scope (R64 brief): the **consumer side** of the range words — `FUN_0004B02C`
(`0x4B02C`): (1) the full body (prologue, the range-word load, the exact
`TEST`/`OR` masks, the bit-8 branch, the `FUN_0008A938` arguments, the tail and
the other arms); (2) the callers and each guard; (3) the bit-8 semantics vs the
lower bits from all fresh readers of the two words; (4) the engine's model and
the exact gap; (5) M2-tape reachability. Extends FU-208 §4, FU-196 §3.1/§3.2,
FU-194 §3.4/§7, FU-146.

**Answer in one line.** `FUN_0004B02C` is the native **match-init / period-end
setup machine** (`FUN_000493A0`'s callee): after the mode arm it does the
both-team refresh, then `MOV CX,word[0x14C1D4]; MOV AX,[0x14C1D6];
OR EAX,ECX; TEST AH,1` — **bit 8 (`0x100`) of the OR of the two per-side
input/range words** — and on set dispatches `FUN_0008A938(EAX=8, EDX=0,
BX=0)` (`0x4B0D9`) and forces the period index `[0x157AC2] := 4`; the two
words are the **side-0/side-1 input-option bitfields** built by `FUN_0001C9BC`
(bits `{0..7, 9}`), so the lower bits feed the reflect/interp/walk consumers
and **bit 8 has no located writer** (extends FU208-L1). A **third** range-word
reader + bit-8 → sit-8 site exists at `0x74312` inside an unnamed sibling body
(`0x741C4..0x744CE`) that the two-producer census (FU-208 §4) missed. The engine
models `FUN_0004B02C` as `fifa96_match_run_period_setup` but its bit-8 branch
(`fifa96_match_run.c:3175`) is dark (`input_range` bit 8 is never set); the M2
tape drives the mode-1 caller but **never with bit 8 set** (the range words are
staged zero), so the sit-8 disjunct is unexercised.

---

## 1. The full body of `FUN_0004B02C` (fresh)

`get_function_by_address 0x4B02C` = `FUN_0004b02c`, entry `0x4B02C`, body
`0x4B02C..0x4B0FE` (`undefined FUN_0004b02c(int param_1)`). Fresh
`disassemble_function 0x4B02C` (61 instructions); fresh `read_memory 0x4B02C`
(64 B) prologue `53 51 52 56` = `PUSH EBX/ECX/EDX/ESI`; fresh `read_memory
0x4B0FA` = `5e 5a 59 5b c3` = `POP ESI/EDX/ECX/EBX; RET`.

### 1.1 the shared prefix and mode switch

```
0x4B02C PUSH EBX / PUSH ECX / PUSH EDX / PUSH ESI
0x4B030 XOR DL,DL / [0x157AB3]=DL / [0x157A4F]=DL          ; clears
0x4B03E XOR EDX,EDX / MOV ESI,0x1588A4 / [0x157A5C]=EDX
0x4B04B XOR EDX,EDX / MOV ECX,0x1590D9 / word[0x157A64]=DX
0x4B059 TEST EAX,EAX / JBE 0x4B064                          ; mode <=0 -> arm A
0x4B05D CMP EAX,1 / JZ 0x4B095                              ; mode ==1 -> arm B
0x4B062 JMP 0x4B0AA                                         ; else -> tail
```

### 1.2 arm A — `mode <= 0` (the period-end arm, `0x4B064..0x4B093`)

```
0x4B064 CALL 0x7412C
0x4B069 CMP dword[0x14C2F2],0 / JZ 0x4B080
0x4B072 MOV EAX,ESI / CALL 0x8CBB0                          ; team 0 (0x1588A4)
0x4B079 MOV EAX,ECX / CALL 0x8CBB0                          ; team 1 (0x1590D9)
0x4B080 CMP dword[0x14C32C],0 / JZ 0x4B08E
0x4B089 CALL 0x78824
0x4B08E CALL 0x88860                                        ; the phase-0xC period-end
0x4B093 JMP 0x4B0AA
```

Arm A calls `FUN_00088860` (`0x88860`, the phase-`0xC` period-end — FU-208
§4.2) **unconditionally** (no session-gate guard here; §2/§4 note the sibling
*differs* — it gates this call on `[0x14C32A] == 0`).

### 1.3 arm B — `mode == 1` (the match-init arm, `0x4B095..0x4B0A5`)

```
0x4B095 CALL 0x74034
0x4B09A CALL 0x7417C
0x4B09F XOR EBX,EBX / XOR EDX,EDX / XOR EAX,EAX
0x4B0A5 CALL 0x8A938                                        ; SITUATION 0, side 0, BX 0
```

### 1.4 the common tail — the range-word gate (`0x4B0AA..0x4B0FE`)

Fresh `disassemble_bytes 0x4B0B8` (48 B); the bytes
`31c9 31c0 668b0dd4c11400 66a1d6c11400 09c8 f6c401 7415 b808000000 31db
31d2 e85af80300 c605c27a150004 31db` decode exactly:

```
0x4B0AA MOV EAX,ESI / CALL 0x8C418                          ; team 0 refresh
0x4B0B1 MOV EAX,ECX / CALL 0x8C418                          ; team 1 refresh
0x4B0B8 XOR ECX,ECX
0x4B0BA XOR EAX,EAX
0x4B0BC MOV CX, word[0x14C1D4]                              ; range word, side 0
0x4B0C3 MOV AX, word[0x14C1D6]                              ; range word, side 1
0x4B0C9 OR  EAX,ECX                                          ; OR of the two words
0x4B0CB TEST AH,0x1                                          ; <<< BIT 8 (0x100) of the OR
0x4B0CE JZ  0x4B0E5
0x4B0D0 MOV EAX,8 / XOR EBX,EBX / XOR EDX,EDX
0x4B0D9 CALL 0x8A938                                         ; <<< SITUATION 8, side 0, BX 0
0x4B0DE MOV byte[0x157AC2],4                                 ; <<< period index := 4
0x4B0E5 XOR EBX,EBX / word[0x158822]=BX / word[0x157A68]=BX
0x4B0F5 CALL 0xCB2B6
0x4B0FA POP ESI / POP EDX / POP ECX / POP EBX / RET
```

* **The exact mask.** `TEST AH,0x1` (`f6 c4 01`) tests bit **8**
  (`0x100`) of `EAX = word[0x14C1D4] | word[0x14C1D6]` — `AH = EAX[15:8]`, so
  `AH & 1` is `EAX[8]`. The branch is taken when the OR's bit 8 is **set**.
* **The `FUN_0008A938` arguments.** `EAX = 8` (the situation id; the dispatcher
  reads it at `0x8A93E MOV ECX,EAX`), `EDX = 0` (the side; stored at
  `0x8A940 MOV word[ESP],DX`), `EBX = 0` (the replay selector; the direct path
  tests it at `0x8AA7B TEST BX,BX / JZ 0x8AAA8`, so `BX=0` routes to the
  table-2 jump table `0x8A904[8] = 0x8AE08` → act 7 — FU-208 §2/§3). Fresh
  `disassemble_bytes 0x8A938` (96 B) re-confirms the head gates this session.
* **The far-call/return.** Both `CALL 0x8A938` / `CALL 0xCB2B6` are near
  `e8 rel32` calls; the function exits by the standard `POP`-ladder `RET`.
* **More than the sit-8 dispatch?** Yes — the function is a three-arm machine:
  arm A (`mode<=0`) runs `FUN_00088860`, arm B (`mode==1`) dispatches **sit 0**,
  and **every** arm falls through the common tail that (a) refreshes both teams,
  (b) applies the **bit-8 sit-8 gate**, and (c) clears `[0x158822]` /
  `[0x157A68]` and calls `FUN_000CB2B6`. The sit-0 producer (`0x4B0A5`) and the
  sit-8 producer (`0x4B0D9`) share the entry.

---

## 2. The callers and their guards (fresh)

Fresh `get_xrefs_to 0x4B02C` = **2** — both inside `FUN_000493A0`
(`0x493A0..0x495AF`), the native **match-init** (FU-196 §3.1):

| site | fresh bytes | containing fn | mode arg | guard |
|---|---|---|---|---|
| `0x49415` | `b801000000 e8121c0000` (`MOV EAX,1; CALL 0x4B02C`) | `FUN_000493A0` | **1** | none (linear in the init sequence; `0x49410 MOV EAX,1`) |
| `0x4950A` | `89d8 e81d1b0000` (`MOV EAX,EBX; CALL 0x4B02C`) | `FUN_000493A0` | **0** (`EBX`=0) | the do-while body: reached only when `FUN_00036BC8(4)==0` (`0x494A8`) **and** `FUN_00036BC8(2)!=0` (`0x494BF`); `EBX` is zeroed at `0x494A4` and never rewritten |

`CALL` targets verified by fresh `read_memory` (rel32 `0x00001C12` from
`0x4941A` → `0x4B02C`; `0x00001B1D` from `0x4950F` → `0x4B02C`). `FUN_000493A0`
itself has **3 callers**, all front-end match-start routines (fresh
`get_xrefs_to 0x493A0` = 3): `FUN_00018014` (`0x180AA`), `FUN_00018108`
(`0x18359`), `FUN_00018680` (`0x18819`). So `FUN_0004B02C` is reached **only**
from the match-init path — never from the per-frame gameplay loop.

---

## 3. The bit-8 semantics — what the two words are, and bit 8 vs the lower bits

### 3.1 what the two words are

`word[0x14C1D4]` (**side 0**) and `word[0x14C1D6]` (**side 1**) are the
**per-side input-option bitfields**. Fresh `disassemble_function 0x1C9BC` (54
insns) — the builder `FUN_0001C9BC` — zeroes both words (`0x1C9CD`/`0x1C9D4`)
then ORs config-cell bits, side-symmetrically (the two cell families are offset
by `0x14`):

* `word0` (`0x14C1D4`/`0x14C1D5`) ← cells `0x105278`, `0x10527C`, `0x105288`,
  `0x10528C`, `0x105290`: bits `{0,1,2,3,4,5,6,7,9}`
  (`0x1C9E3` b1, `0x1C9F5` b2, `0x1CA05` b0, the `0x1C99C` switch b3/b5/b6/b7,
  `0x1CA9F` b4, `0x1CA15` b9). **Bit 8 is never set.**
* `word1` (`0x14C1D6`/`0x14C1D7`) ← cells `0x10529C`, `0x1052A0`, `0x1052A4`:
  bits `{3,4,5,6,7,9}` (`0x1CA25` b9, the `0x1C9AC` switch b3/b5/b6/b7,
  `0x1CAAE` b4). **Bit 8 is never set.**

The entry gate is `FUN_0006D1B2() == 1` (`0x1C9C2 CMP EAX,1 / JNZ 0x1CAB6`).
The only other writer, `FUN_00024B00`, **zeroes** both words (`0x24B91`
`word[0x14C1D6]=DX(0)`, `0x24B98` `word[0x14C1D4]=BX(0)`; fresh
`disassemble_bytes 0x24B70`). Fresh `get_xrefs_to 0x14C1D4` = **21** and
`0x14C1D6` = **12** — the writer set is exactly these two functions.

### 3.2 the lower bits — the reflect / interp / walk consumers (fresh)

Every other reader ORs the two words and tests a **low bit**, so the words are
consumed as a *union* over sides:

| reader | fresh site | test | meaning |
|---|---|---|---|
| `FUN_0007131C` | `0x718D0`/`0x718D6`, `0x718F5 AND AL,1` | bit **0** | boundary reflect |
| `FUN_000736AC` | `0x73A07`/`0x73A0E`, `0x73A16 TEST AL,2` | bit **1** | interpolation arm |
| `FUN_000709D0` | `0x70AB1`/`0x70AB8`, `0x70AC0 TEST AL,4` | bit **2** | pan-step walk gate |
| `FUN_0004B02C` | `0x4B0BC`/`0x4B0C3`, `0x4B0CB TEST AH,1` | bit **8** | the sit-8 / period-4 request |

Bit 9 (`0x200`, the `[0x105288]`/`[0x10529C]` "high byte := 2" arms) and bits
3/4/5/6/7 have no located test in this reader set.

### 3.3 the `0x100` bit is distinct — and unproduced

The sit-8 gate tests **bit 8** (`0x100`), which the two range-word builders
**never write** — the builders set bits `{0..7, 9}` (word0) and `{3..7, 9}`
(word1). This is FU-208 §4.4 / **FU208-L1**, re-confirmed first-hand here
(§3.1). Two readings remain open (a bit-attribution off-by-one, or a third
producer outside these two builders); neither is asserted.

---

## 4. Discovery — a **third** range-word reader + bit-8 sit-8 site (`0x74312`)

FU-208 §4 states "the sit-8 sites are `0x4B0D9` and `0x888C8`". Fresh
`get_xrefs_to 0x8A938` = **39** this session and the sit-8 candidate set is
**three** sites (`0x4B0D9`, `0x888C8`, `0x74312`), plus a sit-0 site
(`0x742DE`).

### 4.1 the body `0x741C4..0x744CE` (fresh, no Ghidra function)

`get_function_by_address 0x741C4` / `0x74244` / `0x742F5` / `0x74300` all
return **none**; `search_functions FUN_00074` lists `FUN_000741b4 @ 0x741b4`
then `FUN_000744d4 @ 0x744d4`, so the range `0x741C4..0x744CE` is an **unnamed
code blob** (the "period-end body" of FU196-L6 / FU208 §4.2). Fresh
`read_memory 0x741C4` = `53 51 52 56 57 55 83 ec 04 …` = a `PUSH
EBX/ECX/EDX/ESI/EDI/EBP; SUB ESP,4` prologue; the blob ends `0x744C2..0x744CE`
with the matching `POP`-ladder `RET` (fresh `disassemble_bytes 0x74490`).

Structure (fresh `disassemble_bytes 0x74140`, `0x74230`):

* `0x74200 TEST AL,AL / JBE 0x7420D / CMP AL,1 / JZ 0x7424E / JMP 0x742E3` —
  the same **mode switch** as `FUN_0004B02C`, but on `AL`.
* arm `mode<=0` (`0x7420D..0x74249`) mirrors `FUN_0004B02C`'s arm A
  (`0x7412C` / `0x8CBB0`×2 with `EBP=0x1588A4`, `EDI=0x1590D9` / `0x78824`) but
  **adds a session-gate guard** the original lacks: `0x74237 CMP
  byte[0x14C32A],0 / JNZ 0x742E3`, i.e. it calls `FUN_00088860` (`0x74244`)
  **only when the session gate is closed**.
* arm `mode==1` (`0x7424E..0x742E2`) is a *different* setup block (the
  `0x700F4`/`0x4C31C`/`0x73EE0`/`0x73E28`/`0x886D4`/`0x7D8D0`/`0x744F4` sinks,
  the `[0x157AC2]^[0x157AC3]&1` parity, then `0x7412C`/`0x8C33C`×2/`0x78824`),
  ending in a **situation-0** dispatch `0x742DC XOR EAX,EAX / 0x742DE CALL
  0x8A938`.
* the shared tail `0x742E3..0x74312` is `FUN_0004B02C`'s tail **cloned**:
  `0x742E3 MOV EAX,EBP / CALL 0x8C418; 0x742EA MOV EAX,EDI / XOR EDX,EDX /
  CALL 0x8C418` (both-team refresh), then the range-word gate with the **bit-8**
  test:

```
0x742F3 XOR EAX,EAX
0x742F5 MOV DX, word[0x14C1D4]
0x742FC MOV AX, [0x14C1D6]
0x74302 OR  EAX,EDX
0x74304 TEST AH,0x1                       ; <<< BIT 8 (fresh bytes f6 c4 01)
0x74307 JZ  0x74317
0x74309 MOV EAX,8 / XOR EBX,EBX / XOR EDX,EDX
0x74312 CALL 0x8A938                      ; <<< SITUATION 8, side 0, BX 0
```

`0x74312` is a genuine sit-8 dispatch (fresh `read_memory 0x74300`:
`…09d0 f6c401 740e b808000000 31db 31d2 e821…`; the `CALL` rel32 targets
`0x8A938`). Unlike `FUN_0004B02C`, this arm does **not** write
`[0x157AC2]=4`; it continues `0x74317 MOV EAX,[0x157A49] / SAR 0x18 / CMP
EAX,1` → the `0x4C324` path.

### 4.2 the blob's entry is unresolved

Fresh `get_xrefs_to 0x741C4` = **0**; `search_instructions` for any `CALL`/`JMP`
with operand `741c4` = **0 matches**; `search_byte_patterns c4410700` (the raw
`0x000741C4` dword) = **no match**. The preceding function `FUN_000741B4`
(`0x741B4..0x741C3`) ends in `RET`, so there is no fall-through. The blob's
reachability is therefore **not statically resolved** — it is either reached by
a computed/indirect transfer or is an unreferenced duplicate. Recorded as
**leg FU216-L1**; it is *not* asserted to be live.

---

## 5. Engine parity and the exact gap

Engine source read first-hand.

### 5.1 what the engine already models

| native | engine | cite |
|---|---|---|
| `FUN_0004B02C` whole machine | `fifa96_match_run_period_setup(mr, mode)` | `src/fifa96_engine/fifa96_match_run.c:3145` |
| mode 1 → sit 0 (`0x4B0A5`) | `set_piece(mr,0,0,0)` | `fifa96_match_run.c:3154` |
| mode 0 → `FUN_00088860` (`0x4B08E`) | `state.phase==0xC` + `global_157ac2==4` → `set_piece(8,rng&1,0)` else `set_piece(1,side,0)` | `fifa96_match_run.c:3160..3169` |
| the bit-8 gate (`0x4B0CB`) | `(input_range[0]\|input_range[1]) & 0x0100` → `set_piece(8,0,0)` + `global_157ac2=4` | `fifa96_match_run.c:3175..3177` |
| the live mode-1 call | `begin` calls `period_setup(mr,1)` when `period_setup_live` | `fifa96_match_run.c:2412` (`period_setup_live=1` at `:2221`) |
| the mode-0 call | `phase_drive` calls `period_setup(mr,0)` when `global_157ac2==4` | `fifa96_match_run.c:2650..2653` |
| `FUN_0001C9BC` range words | `fifa96_input_range_words` (bits `{0..7,9}`) | `src/fifa96_loader/fifa96_input.c:86` |
| lower-bit consumers | `match_run_input_bits_update` bit0 / bit1; the walk gate `range&4` | `fifa96_match_run.c:466..470`, `:4025` |

### 5.2 the exact gap

1. **The bit-8 branch is dark (extends FU208-L1).** `fifa96_input_range_words`
   (`fifa96_input.c:86`) mirrors `FUN_0001C9BC` and, like it, never sets bit 8;
   `mr->input_range` is staged zero at reset (`fifa96_match_run.c:2012..2013`,
   `:2263`) and only ever rebuilt from `input_cfg`, which is zeroed at
   `:2011`. The sole non-test setter `fifa96_match_run_set_input_options`
   (`:478`) is called **only** from tests (fresh grep: tests
   `test_engine_match_frame.c:1564/1570/1574`). So the gate at `:3175` can
   never be satisfied — no engine source supplies the bit-8 producer.
2. **The third producer is unmodelled.** The engine has no analog of the
   `0x741C4` blob nor its sit-8 site `0x74312` (nor the sit-0 `0x742DE`); the
   port models only `FUN_0004B02C` + `FUN_00088860`.
3. **The `FUN_000493A0` mode-0 guard is unported.** The engine reaches
   `period_setup(mode 0)` from `phase_drive` (`:2653`), not from the
   `FUN_00036BC8(2)!=0 && FUN_00036BC8(4)==0` loop guard that decides mode 0 in
   the native (the semantics of `FUN_00036BC8`'s `2`/`4` codes are not derived).

---

## 6. M2-tape reachability — precise verdict

The M2 tape (`tests/test_engine_m2.c`) boots → front-end → **starts a match
(selector 0)** → kickoff → … So it **does** drive the native caller's analog:
`begin` runs `fifa96_match_run_period_setup(mr, 1)` (`fifa96_match_run.c:2412`,
the `FUN_000493A0`/`0x49415` mirror). Inside that call the common tail evaluates
the bit-8 gate — **but the range words are staged zero on the tape**, so
`(input_range[0] | input_range[1]) & 0x100 == 0` and the sit-8 branch is **not**
taken:

* `input_cfg` is zeroed at reset (`fifa96_match_run.c:2011`) and never set on
  the live path (`set_input_options` is test-only), so
  `fifa96_input_range_words` yields `input_range[0] == input_range[1] == 0`
  (the tape's own v8.3/v9 provenance records "the range words are zero — the
  config cells are BSS with no writer").
* The **mode-0** arm (the other caller, `0x4950A`) is never reached: it needs
  `global_157ac2 == 4` (`fifa96_match_run.c:2650`), and the tape's shortened
  period completes without the extra-time index ever reaching 4 (it exits at
  the post-goal window).

**Verdict: the tape drives a `FUN_0004B02C` caller (mode 1 at match start) but
never with bit 8 set; the sit-8 disjunct at `0x4B0D9` is unexercised, and the
mode-0 caller is never driven.** This is consistent with FU-208's M2
"tape-dormant cohort rows and byte-identical golden.

---

## 7. Numbered legs

1. **FU216-L1 — the unnamed sibling body `0x741C4..0x744CE` (a third
   range-word reader + bit-8 → sit-8 site `0x74312`).** Fresh `get_xrefs_to
   0x741C4` = 0, no `CALL`/`JMP` operand `741c4`, no raw `0x000741C4` dword;
   the preceding `FUN_000741B4` ends in `RET`. The body is a genuine
   prologue→`RET` function (mode switch on `AL`, the `0x7412C`/`0x8CBB0`/
   `0x78824`/`0x88860` mode-0 arm **with** an added `[0x14C32A]` gate, a distinct
   mode-1 setup block, and `FUN_0004B02C`'s bit-8 sit-8 tail at `0x74312`). Its
   entry/reachability is **not** statically resolved. Settle by finding the
   indirect/table entry. Do not assert it live.
2. **FU216-L2 — the bit-8 producer (extends FU208-L1, FU196-L1).** `FUN_0004B02C`
   tests bit 8 (`0x4B0CB`), but the only two located range-word writers
   (`FUN_0001C9BC`, `FUN_00024B00`) set bits `{0..7,9}` or zero. No located
   writer sets bit 8. Settle by tracing the `0x105278`-family config-cell
   producers / the extra-time menu path.
3. **FU216-L3 — the engine has no analog of the `0x741C4` blob / `0x74312`.**
   `period_setup` (`fifa96_match_run.c:3145`) covers `FUN_0004B02C` +
   `FUN_00088860` only. Owner: a future producer slice, **gated on L1**
   (reachability unknown).
4. **FU216-L4 — the `FUN_000493A0` mode-0 guard.** Mode 0 is selected by the
   `FUN_00036BC8(2)!=0 && FUN_00036BC8(4)==0` loop (`0x494A8`/`0x494BF`); the
   meaning of the `2`/`4` codes (the native period-transition signals) is not
   derived, and the engine reaches mode 0 from `phase_drive` (`:2653`) instead.
5. **FU216-L5 — an additional `[0x157AC2]` writer outside FU-208 §5.1.**
   Fresh `get_xrefs_to 0x157AC2` = 56 includes a writer **`0x88D6A`**
   (`FUN_00088940`: `0x88D62 INC BL / 0x88D6A MOV byte[0x157AC2],BL`) not listed
   in FU-208 §5.1's writer set. Erratum-adjacent; the period-index write chain
   is wider than FU-208 recorded.
6. **FU216-L6 — M2 tape: the sit-8 disjunct is unexercised.** The tape drives
   the mode-1 caller at `begin` but with `input_range` zero (§6); the mode-0
   caller is never driven. No tape change is expected from a correct bit-8
   producer *until* a range-word writer is also staged (FU216-L2).

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` (this session).
* `get_function_by_address`: `0x4B02C` (body `0x4B02C..0x4B0FE`), `0x493A0`,
  `0x741C4`/`0x74244`/`0x742F5`/`0x74300` (no function), `0x74034`, `0x741B4`,
  `0x7417C`, `0x7412C`.
* `disassemble_function`: `0x4B02C`, `0x493A0`, `0x1C9BC`.
* `decompile_function`: `0x4B02C`, `0x493A0`.
* `disassemble_bytes` (start, length B): `0x4B0B8` (48), `0x739F0` (48),
  `0x70AA5` (40), `0x718C5` (40), `0x718E4` (40), `0x742E8` (40), `0x7430F`
  (48), `0x74230` (240), `0x74140` (288), `0x7409C` (144), `0x74340` (192),
  `0x74490` (80), `0x8A938` (96), `0x24B70` (48), `0x88D60` (20).
* `read_memory`: `0x4B02C` (64), `0x4B0B8` (48), `0x4B0FA` (5), `0x49410`
  (10), `0x49508` (7), `0x741C4` (16), `0x74300` (20).
* `get_xrefs_to`: `0x4B02C` (2), `0x493A0` (3), `0x8A938` (39), `0x14C1D4`
  (21), `0x14C1D6` (12), `0x157AC2` (56), `0x741C4` (0).
* `search_functions`: `FUN_00074` (13; no `0x741C4`). `search_instructions`:
  `CALL`/`JMP` operand `741c4` (0). `search_byte_patterns`: `c4410700` (none).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(`:455..487` the input-bits helper + `set_input_options`; `:2005..2044` the
reset incl. `input_cfg`/`input_range` zeroing `:2011..2013`; `:2255..2264` the
`input_range_words` build; `:2405..2412` the begin mode-1 call; `:2580..2684`
`phase_drive` incl. the `global_157ac2` roll `:2639` and the mode-0 call
`:2650..2653`; `:3136..3181` `fifa96_match_run_period_setup`; `:4019..4030` the
walk gate); `src/fifa96_loader/fifa96_input.c` (`:86`
`fifa96_input_range_words`); `include/fifa96_engine/fifa96_match_run.h`
(`:979` the setter); `tests/test_engine_m2.c` (the tape + the v8.3/v9
"range words zero" provenance); `tests/test_engine_match_frame.c`
(`:1564..1574` the test-only setter callers). Docs cross-cited: FU-146, FU-148
§12.2/§13.1, FU-149, FU-175 §1/§4, FU-194 §3.4/§7, FU-196 §1–§6, FU-202
§1/§3/§4/§5/§7, FU-208 §4/§5/§6/§7, FU-209 §6. No writes beyond this slice
file; no rename/label/comment/function/script/project save; no git commands.

---

## Errata

**Erratum — FU-208 §4 (the two-producer count).** FU-208 §4 reads "Situation 8
has exactly **two** native producers … the sit-8 sites are `0x4B0D9` and
`0x888C8`". Fresh `get_xrefs_to 0x8A938` = 39 this session shows a **third**
sit-8 dispatch site at **`0x74312`** (inside the unnamed body
`0x741C4..0x744CE`, §4) — a `FUN_0004B02C`-tail clone gated on the same bit 8.
The `0x888C8`/`0x4B0D9` claims themselves stand; only the "exactly two" count is
incomplete. (Leg FU216-L1.)

**Erratum — FU-208 §5.1 (the `[0x157AC2]` writer set).** FU-208 §5.1 names the
writers as the reset `0x73F82`, `0x4B0DE`, the rollover `0x8B58A`, the front-end
`0x38990` and `FUN_0004BA00`. Fresh `get_xrefs_to 0x157AC2` = 56 adds a writer
**`0x88D6A`** (`FUN_00088940`'s `INC BL; MOV byte[0x157AC2],BL`). The sit-8
producers read the cell as before; the writer set is wider. (Leg FU216-L5.)
