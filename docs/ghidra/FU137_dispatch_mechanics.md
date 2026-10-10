# FU-137: M2 dispatch mechanics and record machines

Task 4 of the M2 child plan (`docs/superpowers/plans/2026-10-07-fifa96-m2-match.md`).
This slice derives the two match dispatch surfaces and the machinery that feeds
them, so the G2 clusters (C5–C10) can port row bodies into a fixed engine seam.
It is the foundation for the `fifa96_match_handlers.c` dispatchers created in
the same task; every row the seam exposes is classified here.

Result in one line: the two tables and their sole readers re-verify exactly as
FU-136 recorded; `FUN_0007D9A4` stages a record and invokes the handler with the
record in EAX (code in EDX, no stack arguments); the two record machines
(`FUN_0007CA54` outfield, `FUN_000782D0` keeper) share one skeleton; the
`0x26`–`0x2C` installer arms are now bounded (0x26 at `0x8D74D`, 0x28 at
`0x8D7CF`, 0x2A at `0x8D807`; 0x27/0x29/0x2B/0x2C have no static install arm);
of the 80 dispatch rows **1 is ported (action `00`, wired by M2 Task 5 /
FU-138), 2 are unwired (action `1E` and the phase zero slot), 73 are not
ported, and 4 (0x27/0x29/0x2B/0x2C) are open legs** — this is the slice-time
snapshot; the current totals are in §7 (**21 ported / 58 UNSUP / 1 NOTF**, the
M2 phase-9 T1/T2 updates / FU-75 L4.6 row 02 + FU-142 OL-63 row 05) — every
still-unwired or
unported row returns `-FIFA96_ERR_UNSUPPORTED`, the phase zero slot returns
`-FIFA96_ERR_NOT_FOUND`, and never a silent no-op (all error results are
negated per the engine family convention). The Task-5 update is recorded in the
errata at the end of this doc.

## Method

* Authoritative program **`/FIFA96.EXE`** (explicit in every call; the MZ
  `fifa96.exe` was not used). Ghidra **read-only**: no renames, comments,
  labels, function creation, scripts or project saves.
* Tool calls made this slice:
  * `read_memory /FIFA96.EXE 0x1106E0` (180 B) and `0x110794` (140 B) — both
    tables re-read byte-for-byte;
  * `search_instructions operand=0x1106e0` and `operand=0x110794` — the two sole
    readers;
  * `disassemble_function 0x7D9A4` + `decompile_function 0x7D9A4` — installer
    staging;
  * `decompile_function 0x6D920`, `disassemble_bytes 0x6D9A0..0x6D9D0`,
    `0x8D098..0x8D19F`, `0x8D720..0x8D814`, `0x8D560..0x8D69F` — phase reader
    and phase-machine installer arms;
  * `decompile_function 0x7CA54`, `0x782D0`, `0x8CEB8`, `0x7DAB4`, `0x8CFAC`;
    `disassemble_bytes 0x7CCF0..0x7CD4F`, `0x78540..0x7859F` — record machines
    and their tails;
  * `get_xrefs_to 0x7D9A4` (77 call sites), `get_xrefs_to 0x8CEB8` (15 call
    sites, all inside `FUN_0008D098`), `get_xrefs_to 0x6D920` (4 callers);
  * `search_instructions mnemonic=mov` for `EDX, 0x26` … `0x2C` and the `DX,`
    forms — the constant census in §5.3;
  * `disassemble_bytes 0x87738` — the 0x2B one-byte RET.
* Flat-image offsets are not used; the native program's pointers are already
  resolved (`0x1106E0` holds runtime handler addresses), matching FU-136 §1.
* Baseline at task start: `make check` = **97/97**; after this task 98/98 with
  the new `test_engine_match_handlers` (ASan/UBSan).
* Evidence chain: FU-136 §1–§3 enumerated the 80 rows and their C/port status;
  this doc re-verifies the surfaces and adds the staging/record-machine/arm
  derivations. Where FU-136's classification is refined, the change is stated
  in the row's class cell.

## 1. Dispatch surfaces (read-only re-verification)

### 1.1 Action table flat `0x1106E0[45]`

`read_memory` returned 45 dwords, first `0x07DB10`, last `0x084598`, matching
FU-136 §1.1 dword for dword. `search_instructions operand=0x1106e0` returns
exactly one match:

```
FUN_0007D9A4 @ 0x7DA77  ADD EAX, 0x1106e0   (bytes 05 e0 06 11 00)
```

### 1.2 Phase table flat `0x110794[35]`

`read_memory` returned 35 dwords, first `0x06DE34`, last `0x08B900`, matching
FU-136 §1.2. Index `0x16` is `0x00000000` (the loader's INT3 stub).
`search_instructions operand=0x110794` returns exactly one match:

```
FUN_0006D920 @ 0x6D9B3  ADD EAX, 0x110794   (bytes 05 94 07 11 00)
```

### 1.3 Readers

* `FUN_0007D9A4` is the action installer/dispatcher (§2); its only static
  reader of `0x1106E0`. 77 call sites (`get_xrefs_to 0x7D9A4`).
* `FUN_0006D920` is the phase-select reader (§3); its callers are
  `FUN_0008D098` (0x8D107) and `FUN_0008CF60` (0x8CF73) — the two per-record
  loops — plus `FUN_0007997C`/`FUN_0006D9C4` (same family).
* The engine seam keeps one row per native slot, so the table's code/phase
  numbering is the dispatch contract.

## 2. Installer `FUN_0007D9A4` — argument staging

Register contract (Watcom): **EAX = record**, **DX = new action code**,
**BL = staged byte**, **ECX = invoke-now flag**. Disassembly walk (0x7D9A4):

```
0x7D9A9 MOV ESI,EAX            ; ESI = record
0x7D9AB MOV [ESP+0x4],DX       ; code parked on the installer's stack
0x7D9B0 MOV [ESP],BX           ; staged byte parked
0x7D9B4 MOV EDI,ECX            ; invoke flag
0x7D9B6 TEST EAX,EAX / JZ ...  ; NULL record -> no-op
0x7D9BE CMP byte [EAX+0x9A],0  ; occupied record -> no-op
0x7D9CB MOVSX AX,byte [ESI+0x91] / CMP AX,DX / JZ ... ; same code -> no-op
```

Then, in order:

* `0x7D9DC..0x7DA1F` — if `[rec+0x98]!=0` and the new code is not `0x0C` and
  `[0x157A4A]>>24` is not 2/0x0A/0x0F, call `FUN_0006E598(rec, 0x0F, 0)` (the
  FU-84 animation selector) and clear `[rec+0x98]`.
* `0x7DA26..0x7DA3B` — **code coercion**: if `[rec+0x8D]==0` and code==3, the
  staged code becomes `0x19` (`MOV word [ESP+4],0x19`).
* `0x7DA42..0x7DA5A` — `[rec+0x9F]` bit 0 set when code is 5 or 0x21, cleared
  otherwise.
* `0x7DA63` — `[rec+0x91] = code`
* `0x7DA6D..0x7DA74` — `EAX = code; SHL EAX,2; ADD EAX,0x1106E0`
* `0x7DA7C..0x7DA92` — stage the record:
  * `[rec+0x18] = table[code]` (the handler slot the record machine tails call),
  * `[rec+0x89] = 0` (dword), `[rec+0x9E] = 0` (byte),
  * `[rec+0x92] = BL`,
  * `[rec+0x7B] = [rec+0x79]` (word).
* `0x7DAA3..0x7DAAA` — if ECX (invoke) != 0:
  `MOV EAX,ESI; CALL dword ptr [ESI+0x18]`.

**What reaches the handler**: EAX = record pointer only; no stack arguments are
pushed. EDX still holds the installer's code in the normal path (it was parked
in memory, not overwritten) except when the animation gate ran, which leaves
`EDX = 0x0F`; EBX is 0 at that point. The handler is therefore a
one-argument record walker in practice — which is why the engine seam's
`fifa96_match_handler_fn` takes only the run and lets the row identity imply
the code. This is the staging the Task 4 brief asked for.

The record machines do not only install: both tails call the *current* handler
slot directly (`CALL dword ptr [EBP+0x18]` at `0x7CD2B` outfield, `0x782D0`
keeper) with EAX = record, so the same handler contract holds whether the
installer invoked it or the machine's frame did.

## 3. Phase reader `FUN_0006D920` — staging and call shape

Tail disassembly (`0x6D9A8..0x6D9BA`):

```
0x6D9A8 MOV EAX,[0x00157a4a]
0x6D9AD SAR EAX,0x18              ; current phase byte
0x6D9B0 SHL EAX,0x2
0x6D9B3 ADD EAX,0x110794          ; phase table index
0x6D9B8 MOV EAX,[EAX]
0x6D9BA MOV [EDX+0x1C],EAX        ; record+0x1C = phase handler
```

The earlier part of the function (decompile) fills `[rec+0x08]`, `[rec+0x0C]`,
`[rec+0x10]`, `[rec+0x14]` from `FUN_0004C384` results keyed by
`(short)**(char**)(*rec + 0x7AE) * 6` — the per-record animation/metadata
pointers; those are out of scope for the dispatch seam (open leg OL-13).

The phase *call* shape comes from `FUN_0008D098` (the match-phase machine, sole
caller `FUN_000740A0`, FU-136 §7 leg 1) at `0x8D0F2..0x8D11E`:

```
0x8D0F2 LEA ESI,[EBP+0x4D]                 ; EDX argument = record+0x4D
0x8D0F7 CMP byte [ECX+0x9A],0 / JNZ skip   ; occupied records skipped
0x8D100 MOV EAX,ECX                        ; EAX = record
0x8D107 CALL 0x0006d920                    ; stage [rec+0x1C]
0x8D10C MOV EDX,ESI                        ; EDX = record+0x4D
0x8D10E MOV EAX,ECX                        ; EAX = record
0x8D110 CALL dword ptr [ECX+0x1C]          ; phase handler(record, record+0x4D)
0x8D11E CALL 0x0008dcd4                    ; record+0x59, record+0x4D, record+0x65
```

The loop runs 11 records at 0xB2 stride (`0x8D131..0x8D152`); the phase
handler is a two-argument watcall (EAX record, EDX pointer to the record's
+0x4D coordinate block). Like the action handlers, the C seam models only the
run; the per-record pointers are unavailable until the entity/record model
lands (OL-1/OL-5).

## 4. Record machines

FU-136 Table C named two machines; both are now structurally derived.

### 4.1 Outfield machine `FUN_0007CA54(rec, edx)`

* Timers: word `[rec+0x81]` and byte `[rec+0x93]` are decremented by
  `[0x157A62]`/its low byte, clamped at 0.
* Input dispatch: `rec[8]` = dword `[rec+0x20]`; its words at +4/+6 are matched
  against the FU-75 input-row tables **`PTR_DAT_001109D0`** (data rows) and
  **`PTR_LAB_001109E4`** (rule rows); a row is an 8-byte record
  `{mask, value, handler, ...}`, and a matching handler is called as
  `handler(rec)` with its EAX result decoded from the returned register pair.
* Forced decision: the `LAB_0007CC82` block calls the reselect/chase gate
  `FUN_0007C990` when `[0x157A4A]>>24 == 2` and the per-type table
  `[0x110680][[rec+0x8E]>>24] & 1`, then may install action **8**
  (`0x7CD19 XOR EDX`… `0x7CD24 CALL 0x7D9A4`) under the FU-75 gate
  (`[rec+0x20]==0`, `[rec+0x81]==0`, `[rec+0x5D]==0`, side mismatch).
* Tail (`0x7CD29..0x7CD48`): `CALL [rec+0x18]` (current handler),
  `FUN_0006E8E8(rec)`, `FUN_00079B1C(rec)` if `rec == [0x157AA7]`,
  `FUN_0007BF20(rec)` (FU-77 locomotion integrator).

### 4.2 Keeper machine `FUN_000782D0(rec, edx)`

* Same timer pair `[rec+0x81]`/`[rec+0x93]`; copies `[rec+0x79]` to `[rec+0x7B]`
  at entry.
* Input dispatch against the keeper row tables **`0x1109C8`/`0x1109A8`/
  `0x110998`** (possession branch) and **`0x1109DC`/`0x1109E0`/`0x110978`/
  `0x110970`/`0x110950`** (the `[rec+0x20]` word-4 branch); matching row
  handlers are called as `handler(rec)`.
* Type gate: `[0x110680][[rec+0x8E]>>24] & 1`; when the timer is zero it
  derives the escalation code — `0x19` when the opponent type is 5, else `4`
  (`0x78544..0x78558`) — and installs only if it differs from `[rec+0x91]`,
  setting `[0x157AB2]=1` (`0x78576 CALL 0x7D9A4`).
* CPU decision: when `[0x157A4A]>>24 == 2`, `[rec+0x20]==0` and
  `[0x157C5D]!=0`, calls `FUN_00077EAC(rec)` (`0x78597`).
* Tail: `CALL [rec+0x18]`, `FUN_0006E8E8(rec)`, controlled-record copy
  (+0x59→+0x4D/…), `FUN_0007BF20(rec)`.

### 4.3 Shared skeleton

Both machines: decrement the two timers → run the input-row dispatch → apply a
forced/reselect gate that installs a code through `FUN_0007D9A4` → call the
current handler (`[rec+0x18]` action slot) → run the shared tails
(`FUN_0006E8E8` animation/event post, `FUN_00079B1C` for the controlled
record, `FUN_0007BF20` locomotion). The reset entry is `FUN_0007DAB4`:
`[rec+0x92]=0xFF`, `[rec+0x89]=0`, optional `FUN_00078B00(rec[8])`, then either
`FUN_0007C990(rec)` (state 2, `[rec+0x8D]!=0`) or install code 0. None of this
is ported (OL-1..OL-4); the engine seam intentionally exposes only the row
dispatch, not the machines.

## 5. Installer arms `0x26`–`0x2C`

### 5.1 The multi-record arm helper `FUN_0008CEB8`

Signature (Watcom): EAX = team/record base, DX = first record index,
BX = last index, CX = install code, stack = skip-if-current code. Body
verified by decompile:

```
for (i = first; i <= min(last,10); i++) {
  rec = base + i*0xB2;
  if ([rec+0x9A]==0 && [rec+0x91]!=skip) {
    code = (i==0 && install==3) ? 0x19 : install;   // d9a4 would coerce again
    FUN_0007D9A4(rec, code, 0, 0);
  }
}
```

`get_xrefs_to 0x8CEB8` = 15 call sites, **all inside `FUN_0008D098`** — the
phase machine is the only installer-arm caller in the program.

### 5.2 Phase-machine arms that install `0x26`–`0x2C`

Disassembly of `FUN_0008D098`'s state switch (`[0x157A4D]`, values ≤ 0x15,
jump table `0x8D040`); relevant arms:

| arm site | call | condition (FU-137 §5.2) |
|---|---|---|
| `0x8D74D` | `FUN_0008CEB8(rec,0,10,0x26,-1)` | state 0x13/0x14, `[rec+0x826] != [0x157AAC]>>24` (non-player-controlled side); `0x8D73F MOV ECX,0x26` |
| `0x8D7CF` | `FUN_0008CEB8(rec,0,10,0x28,-1)` | state 0x13/0x14, player side, `[0x157A4A]>>24 != 0x13`; `0x8D7C1 MOV ECX,0x28` |
| `0x8D807` | `FUN_0007D9A4(rec,0x2A,0,0)` | state 0x13/0x14, player side, after the 0x28 arm; scan records 1..10 for the first `[rec+0x9A]==0` (`0x8D7D4..0x8D7F6`), store it at `[team+0x831]` (`0x8D801`), code `0x2A` at `0x8D7F8` |
| `0x8D7AD` | `FUN_0008CEB8(rec,0,10,0x25,-1)` | state 0x13/0x14, player side, state==0x13 and `[0x57AC5] != [0x57AC7]` (0x25 is the neighbour arm, outside this task's range but derived for context) |

The 0x2A arm is exactly FU-136 §7 leg 1's quoted block: it resolves the
record-scan uncertainty (records **1..10**, first free slot by `[rec+0x9A]`,
result cached at `[team+0x831]`) and confirms the install site at `0x8D807`.

Other `FUN_0008D098` arms (context, not in range): code `0` at `0x8D19F`
(state 0/0xA/0x0F), code `3` at `0x8D1C1`/`0x8D2B3`/`0x8D36B`/`0x8D58B`/
`0x8D5E9`/`0x8D78B`, code `1` at `0x8D200`, code `2` at `0x8D238`, code `4` at
`0x8D291`, code `0x10` at `0x8D349`, code `0x11` at `0x8D421`, code `0x14` at
`0x8D475`, code `0x1F` at `0x8D569`, code `0x12` at `0x8D5C7`, code
`0x1D`/`0x1E` at `0x8D62C`.

**Errata (M2 arms-and-wiring Task 2 / FU142a, §5.2 first-hand re-read).** The
state 0x13/0x14 entry is `0x8D693` (switch `0x8D178 MOV AL,[0x157A4D]`, table
`0x8D040`; entries `0x8D08C`/`0x8D090`), not a bare `0x8D720` block: the
entry block `0x8D693..0x8D727` runs `0x651F0`(0xD), tests `[0x157AC2] < 4` and
writes the row-28 body globals `[0x10F364]`/`[0x10F368]` before the arms; the
derived subset starts at the side test. The switch byte **is** the phase byte
(`0x157A4D` is byte 3 of the `0x157A4A` dword read at `0x8D75F`
`SAR EAX,0x18`), so state and phase cannot differ natively. The `0x26` side
test is `zero_extend(byte [EBP+0x826]) == sign_extend(byte3 [0x157AAC])`
(`0x8D72E XOR EAX,EAX`/`0x8D733 MOV AL` vs `0x8D730 SAR EDX,0x18`): a
controlled byte `>= 0x80` sign-extends negative and can never equal the
0..255 side, so the native always takes the 0x26 arm for such a byte; the
port's ruled byte compare (`(uint8_t)side_controlled != team->side`, FU-142
Appendix B.3) differs only at `side == side_controlled >= 0x80`, unreachable
for the pool's 0/1 sides. The 3/0x25 arms compare the full 16-bit
`[0x157AC5]`/`[0x157AC7]` words (`0x8D76C MOV AX` / `0x8D772 CMP AX`); the
port stores and compares their low bytes (Appendix B.3). The 0x2A scan
exhaustion exits at `EDX=0xB` with `EAX=team+0x7A6`, stores that pointer in
`+0x831` and installs 0x2A at the aliased `+0x91 = team+0x837` (next team
block `+0x2`); the derived model flags the overflow instead (`arm2a_overflow`,
Appendix B.4, resolves FU142 OL-49). `FUN_0008D098`'s only callers are
`FUN_000740A0` `0x740C8`/`0x740DB`, which write `[0x157A4D]` from AL, call it
once per team (`0x1588A4 + side*0x835`) and then handle phase 2.

**Errata (M2 arms-and-wiring Task 8 / FU142e, §5.2 arm-tail re-read).** The
`0x8D807` arm's tail — after both the found install and the record-11 overflow
fall-through — runs `0x8D80C XOR EBX,EBX` / `0x8D80E MOV [0x10F35C],EBX`,
clearing the `[0x10F35C]` chase flag every arm execution. Task 2's port had
omitted this write; Task 8 completed it in
`fifa96_match_phase_machine_step` (first-hand
`disassemble_bytes 0x8D7CF`, 72 B; FU-142 Appendix H/H.6), because row 2A's
arm 10 sets the flag and the derived frame reads it back. The first-hand arm
bytes also confirm the scan shape (`0x8D7D4` `EDX=1`, `0x8D7D9`
`LEA EAX,[EBP+0xB2]`, `0x8D7F3 CMP ECX,0xB` / `0x8D7F6 JL`, `0x8D7F8
EDX=0x2A`).

### 5.3 Constant census for `0x27`, `0x29`, `0x2B`, `0x2C`

`search_instructions mnemonic=mov` over the whole program for `EDX,imm` (the
`DX,` search returns the same sites; both are substring-based):

| constant | match-code matches | installer arm? |
|---|---|---|
| `0x26` | 22 sites incl. handler bodies (e.g. `0x83FC0`, `0x85F34` → `FUN_0008CFAC`, the animation-row wrapper) | yes: §5.2 `0x8D74D` |
| `0x27` | `0x756D5` only, inside keeper handler `0x07550C` (action 1E): `MOV EDX,0x27` then `CALL 0x6E598` (animation selector) | **none** |
| `0x28` | `0x7644F` inside keeper handler `0x076380` (action 1F): `MOV EDX,0x28` then `CALL 0x6E598`; unrelated `FUN_0001F440`/`FUN_0002D684` sites | yes: §5.2 `0x8D7CF` |
| `0x29` | none in the match code (`0x1F53C` in `FUN_0001F440` is not an installer caller) | **none** |
| `0x2A` | `0x8D7F8` (the §5.2 arm); all others (`0x1F558`, `0x39EA7..`, `0xA9A36`) are non-installer | yes: §5.2 `0x8D807` |
| `0x2B` | none in the match code (`0x1F535` unrelated) | **none**; native body is a one-byte `RET` at `0x87738` |
| `0x2C` | `0x3035E`/`0x40A8E`/`0x40C5C` in non-match functions, none call `0x7D9A4` | **none** |

Consequence: `0x26`, `0x28`, `0x2A` have statically bounded install arms;
`0x27`/`0x29`/`0x2B`/`0x2C` have **no installer arm found anywhere in the
program** — after the FU-142f census (errata below) 27/29/2C are class
`unwired` (ported bodies, entry verdict OL-48) and 2B is a `dead entry`
(shared row-29 epilogue RET), so none of the four is a schedulable port.
(`0x27`'s only match-code constant is an animation-row argument, a distinct
meaning of the same byte, so it is **not** evidence of an action install.)

**Errata (M2 arms-and-wiring Task 9 / FU-142f).** The constant census above is
refined to a whole-call-site classification (FU-142 Appendix I): all 77
`FUN_0007D9A4` call sites were re-read first-hand; every EDX argument is a
constant or a bounded register-derived set (I.3/I.4), no control-flow path
bypasses the final write (I.5), and no stored pointer to the installer exists
(`search_byte_patterns a4 d9 07 00` = no matches), so no indirect entry can
target it (I.6). The reachable installer code domain is therefore
**{0x00..0x26} ∪ {0x28, 0x2A}** and the codes never passed are exactly
**0x27/0x29/0x2B/0x2C**. The "no installer arm found" rows are a negative
census, not merely an unsearched region: 27/29/2C stay unwired with OL-48
recorded as the negative verdict (FU-142 §6), and 2B is a dead entry.

## 6. Per-row classification (all 80 rows)

Rubric (refines FU-136 §1.3 by splitting the unresolved entry paths):

| class | dispatch result | definition |
|---|---|---|
| `ported` | `FIFA96_OK` | row body in C **and** wired into `fifa96_match_action_table`/`_phase_table` (`fn != NULL`). |
| `unwired` | `-FIFA96_ERR_UNSUPPORTED` | body fully covered by tested C symbols, no record/entity binding yet (open leg OL-1). Phase `0x16` is the zero/INT3 slot: dispatch-only, returns `-FIFA96_ERR_NOT_FOUND`. |
| `not ported` | `-FIFA96_ERR_UNSUPPORTED` | a needed part of the body has no C function; the row names its port group/open leg. |
| `open leg` | `-FIFA96_ERR_UNSUPPORTED` | FU-137 could not bound a static entry for the row (no install arm), so it is not schedulable as a port task yet; numbered OL. |
| `dead entry` | `-FIFA96_ERR_UNSUPPORTED` | the native entry has no distinct body (aliases another row's epilogue) and/or the exhaustive installer census never passes its code; not schedulable, no actionable leg. |

### 6.1 Table A — action rows `0x1106E0[code]`

| code | native | class | handler / evidence | open leg |
|---|---|---|---|---|
| 00 | 0x07DB10 | ported (M2 Task 5 / FU-138 §4) | `fifa96_match_action_00` binds `fifa96_action_move_step`/`_move_target` to `mr->record` (FU-76 §3.1/§4; `test_engine_match_handlers::test_action_00_runs_move_step`) | OL-1 closed for row 00; record pool OL-16 |
| 01 | 0x07DBC0 | ported (M2 playable-match Task 2 / FU-143 §11) | `fifa96_match_action_01` binds the native phase-1 stage walk (`0x7DBC0..0x7DFC8`: the `phase == 1` gate, the `[+0x8F]>>24` = byte `+0x92` marker branch, the ±0x30 kickoff target, the stage 0/1/2 walk and the `FUN_0008A938` situation 0xB call that writes phase 2) to `mr->record`/the pool; `test_engine_match_handlers::test_action_01_runs_kickoff_body` | row-01 event/camera/ball-stage sinks OL-84a/OL-84b; rows 02/0x10..0x13 producers OL-84d |
| 02 | 0x07DFCC | ported (M2 phase-9 T1 / FU-75 L4.6) | `fifa96_match_action_02` binds the first-hand `0x7DFCC..0x7E1A2` body (phase-1 `locomotion_restart_target` mirror; phase-2 `[team+0x7B2] = rec`, the `0x7E041` merge request, the install-4 invoke-now, the no-slot camera target + `+0x89 += delta` and the `restart_wait` stage 0) to `mr->record`/the pool; the stage-1/2 arms stay OL-18; `test_engine_match_handlers::test_action_02_restart_and_phase2_arms` | FU-138 OL-18 (stage 1/2); OL-9 closed for row 02 |
| 03 | 0x07E1A4 | not ported (partial) | locomotion_hold/clamp_placement; FU-77 §2.3; FU-138 counter/phase1_clamp | OL-8; FU-138 OL-19 |
| 04 | 0x07E7C8 | ported (M2 playability-legs Task 1 / FU-142 Appendix K.5) | `fifa96_match_action_04` binds `fifa96_outfield_row04_step` (`0x7E7C8..0x7F141`: carrier/ranked-pick arms, the target arms + `0x7D3E4` clamp, the no-slot `0x7E600` decision, the score/RNG install `0xB`, the bound/angle tail and the installs 4/0x19/0xF/0xE/0xB/7/6/5) to `mr->record`/the pool; the `0x7CA54` input-row machine stays a separate unwired seam; `test_engine_match_handlers::test_action_04_and_08_wired` | unmodeled record/team bytes, track words, predictor and sinks OL-65/OL-67/OL-72 (K.5.8); the `+0x8E` `record.type`/`actor_type` field split OL-83 |
| 05 | 0x07F194 | ported (M2 phase-9 T2 / FU-142 OL-63) | `fifa96_match_action_05` binds `fifa96_action_carrier_arm` (`0x7F194..0x7F665` stages 0-3) over the pool: the `0x7F1FF` `[0x158724]` claim + the `0x158728..0x15872F` block (`ball.pos_*`), the `[team+0x7B2]=rec`/`+0x7B6=0` bind, the capped `+0x89`, the camera triple `+0x4D..+0x55`, the stage-0 control/dir gates (`[0x157A83]`, `0x15872A/B`), the `0x7876C` merge request, the `0x79B1C` snap, the `0x79C50` face and the stage-3 hand-off; also `fifa96_ball_pair_stage_tail` (`0x7A8D1..0x7AA2F`); `test_engine_match_handlers::test_action_05_wired_carrier` / `_claim_dirs_and_stages` | stage-0 target algebra `0x7F3A1..0x7F57B` + `FUN_0007F7E0` fallback OL-63 (named requests); sinks 0x92820/0x6E598 OL-27/OL-52 |
| 06 | 0x0801B4 | ported (M2 arms-and-wiring Task 13 / FU-139 §11) | `fifa96_match_action_06` binds `fifa96_action_pursuit_step` (`0x801B4..0x809EF`: the `0x8DCD4`/`0x8DD70` camera metric, the `0x114E04` folds, the carrier-gate install 4 and the RNG installs 8/9, the `0x79C20` slot target, the `0x8DE8C`/`0x79CCC` mate selections and the `0x6DA64` swap request) to `mr->record`/the pool; the plan span end `0x81067` is the row-09 body `0x80A00` (slot `0x1106E0[9]`, unwired OL-9); `test_engine_match_handlers::test_action_06_runs_body` | OL-69 (record bytes, lead, callback position, swap); OL-52 remark |
| 07 | 0x0814B0 | ported (M2 arms-and-wiring Task 11 / FU-139 §9) | `fifa96_match_action_07` binds `fifa96_action_kick_machine` (`0x814B0..0x81737`) and `fifa96_ball_kick_target` (`FUN_0007B9C4`, `0x7B9C4..0x7BF16` incl. the `FUN_0007B878` dir arm and both mode arms) to `mr->record`/the pool ball block; defender 0x0E / opponent 0x22 / ball 4 requests; `test_engine_match_handlers::test_action_07_runs_body` | OL-65 (record/presentation auxiliaries) / OL-66 (external block inputs) / OL-62 remark |
| 08 | 0x081068 | ported (M2 playability-legs Task 2 / FU-142 Appendix K.6) | `fifa96_match_action_08` binds `fifa96_outfield_row08_step` (`0x81068..0x814AF`, 329 linearly decoded insns incl. the RET; FU-77 §2.7/FU-141 §7's `..0x81188` head is a prefix, first-hand): the stage 0/1/2 machine (phase gate + timer add, the stage-0 camera/lead copy, the `0x79C50` face, the `0x79B58` receiver gate, the +0x3D-gated `0x795A4`/`0x8DCD4`/`0x8DC68` projection scan, the user/record event arms, the `0x79B1C` stage-2 snap) over `mr->record`/the pool; row 08 installs no code; `test_engine_match_handlers::test_action_04_and_08_wired` | unmodeled record/process bytes, the 0x7A490 staging and the event/sound sinks OL-52/OL-62/OL-82 (K.6.7); the `+0x8E` `record.type`/`actor_type` field split OL-83 |
| 09 | 0x080A00 | not ported (partial) | FU-81 arm table 0x809F0; stage helpers | OL-9 |
| 0A | 0x081738 | not ported | FU-76 §2; installer 0x7CDD8 has no xrefs | OL-14 |
| 0B | 0x081908 | not ported (partial) | sequence_duel_event; FU-82 §3.3 | OL-9 |
| 0C | 0x081C90 | not ported (partial) | FU-81 7-arm table 0x81C74 | OL-9 |
| 0D | 0x08251C | not ported (partial) | FU-82 §3.4 4-arm table 0x8250C; FU-138 velocity_scale | OL-9; FU-138 OL-22 |
| 0E | 0x082710 | not ported | FU-81 §2.1 gate/head; tackle helpers only install 0x0E | OL-9 |
| 0F | 0x082AD0 | ported (M2 arms-and-wiring Task 11 / FU-139 §9) | `fifa96_match_action_0F` binds `fifa96_action_kick_machine` (`0x82AD0..0x82DCF`: `0x79B1C` snap, `0x79B6C` re-anchor/face, the `0x7876C` merge request, the predictor distance and the corner kick) and `fifa96_ball_kick_target`; `test_engine_match_handlers::test_action_0F_runs_body` | OL-65 (`+0x77` bound, `+0x85/87`) / OL-66 (predictor/corner tables) / OL-62 remark |
| 10 | 0x0855F0 | ported (M2 phase-8 T1 / FU-149 §7) | `fifa96_match_action_10` binds the 7-stage throw-in taker (`0x855F0..0x85DE3`, table 0x855B8): snapshot placement, the `FUN_00085498` probe, the pick/install-4 relay, the resolution + situation-2 re-queue; staging bodies/event sinks are FU-149 §7.3 legs | L13.1/L13.3/L13.4/L13.5 |
| 11 | 0x085DE4 | ported (M2 phase-8 T1 / FU-149 §7) | `fifa96_match_action_11` binds the 10-stage corner taker (`0x85DE4..0x864FF`, table 0x85DA0): `0x3C` wait, `FUN_0007D360` probe placement, the probe/kick/resolution and the target clamp; kick vectors/sinks are FU-149 §7.3 legs | L13.1..L13.5 |
| 12 | 0x083D68 | ported (M2 phase-8 T1 / FU-149 §7) | `fifa96_match_action_12` binds the 8-stage free-kick taker (`0x83D68..0x84597`, tables 0x83D2C/0x83D4C): incident placement + wall-point fold, the span-3 probe, the kick + code-3-gated relay; the wall block/kick arms are FU-149 §7.3 legs | L13.1/L13.3/L13.4/L13.6 |
| 13 | 0x084B00 | ported (M2 phase-8 T1 / FU-149 §7) | `fifa96_match_action_13` binds the 7-stage penalty taker (`0x84B00..0x84EEB`, table 0x84AE4): spot focus + commit, the distance/opponent/slot gates, the mode-0x40 strike and the ack-gated resolution; the free-record hand-off/input words are FU-149 §7.3 legs | L13.4/L13.7 |
| 14 | 0x08784C | not ported (partial) | scatter_celebration; FU-82 §3.5 | OL-9 |
| 15 | 0x087CD0 | not ported | FU-81 §2.1 head mis-decoded; FU-82 §1 stub bucket | OL-14 |
| 16 | 0x084630 | not ported (partial) | sequence_marker/rng_event; FU-82 §3.6 | OL-9 |
| 17 | 0x084730 | not ported (partial) | FU-81 4-arm table 0x84720 | OL-9 |
| 18 | 0x0849B0 | ported (M2 arms-and-wiring Task 12 / FU-139 §10) | `fifa96_match_action_18` binds `fifa96_action_duel_step` (`0x849B0..0x84AE1`) plus `fifa96_action_duel_bind` (0x4C324), `fifa96_action_duel_search` (0x8DB6C + the 0xA1860 shell sort) and `fifa96_action_duel_swap` (0x786A0) to `mr->record`/the pool; install arm 0x8A32F; `test_engine_match_handlers::test_action_18_runs_body` | OL-68 (anim row byte, bind globals, slot-word clears); OL-52 remark |
| 19 | 0x0746E4 | not ported (partial) | keeper_hold_* + FU-140 §3 fallback; FU-79 §2 | FU-140 OL-33 |
| 1A | 0x07662C | not ported (partial) | keeper_reposition_a_gate; FU-79 §3; FU-140 §2 | FU-140 OL-34 |
| 1B | 0x076D28 | not ported (partial) | keeper_reposition_b_finish; FU-79 §4; FU-140 §2 | FU-140 OL-34 |
| 1C | 0x077728 | not ported (partial) | keeper_lunge_track; FU-79 §5; FU-140 §2 | FU-140 OL-35 |
| 1D | 0x074EB0 | not ported (partial) | keeper_clear_vector; FU-79 §6; FU-140 §2 | FU-140 OL-35 |
| 1E | 0x07550C | ported (M2 Task 7 / FU-140 §4) | `fifa96_match_action_1E` binds `fifa96_keeper_claim_place` to `mr->record` (FU-79 §7; `test_engine_match_handlers::test_action_1E_runs_claim_place`); M2 phase-9 T3 wires the stage-5 `0x75DC4` FUN_00071C94 event (camera cut to the release triple + tracked-record bind; `test_action_1E_machine_release_chain`) | OL-37 closed for the record-visible body; slot-merge/record pool OL-37 (carries OL-16); the stage-5/7 FUN_00092820 sinks stay legs |
| 1F | 0x076380 | not ported (partial) | keeper_dive_target/arm_step + FU-140 §3 input_decide/arm_camera; FU-79 §8 | FU-140 OL-36 |
| 20 | 0x084EEC | not ported (partial) | FU-82 §3.7 7-arm table 0x84ED0 | OL-9 |
| 21 | 0x085214 | ported (M2 arms-and-wiring Task 12 / FU-139 §10) | `fifa96_match_action_21` binds `fifa96_action_receive_step` (`0x85214..0x8539B`: phase/tracked, camera copy, the 0x8DE8C nearest + 0x4A arm, the 0x7DAB4 reset and the team-target ball 4/receiver) to `mr->record`/the pool; install arms 0x7D046 and row-05 0x7F791/0x7F7B6; `test_engine_match_handlers::test_action_21_runs_body` | OL-68 (+0x44, camera velocity, 0x79B58 gate); OL-52 remark |
| 22 | 0x08539C | not ported (partial) | sequence_press_event; FU-82 §3.8 | OL-9 |
| 23 | 0x082F84 | ported (M2 arms-and-wiring Task 12 / FU-139 §10) | `fifa96_match_action_23` binds `fifa96_action_tackle_step`/`_attempt` (`0x82F84..0x83163` + `0x82DD0`: stage gates, target arm, installs 0x0E/0x0F, 0x79B58 receiver) to `mr->record`/the pool; install arm 0x7D1B9; `test_engine_match_handlers::test_action_23_runs_body`; the pre-Task-12 `0x82E8E` camera gate is corrected in the FU-139 §10 errata | OL-68 (0x1577xx inputs, +0x85/+0x99/+0x5D, +0x7C7, slot +0x10); OL-65 remark |
| 24 | 0x086510 | not ported (partial) | sequence_lane/anim_byte; FU-82 §3.9 | OL-9 |
| 25 | 0x0880CC | not ported (partial) | FU-82 §3.10 7-arm table 0x880B0 | OL-9 |
| 26 | 0x0866F4 | ported (M2 arms-and-wiring Task 3 / FU-142 Appendix C) | `fifa96_match_action_26` binds `fifa96_arm_26_step` (`0x866F4..0x8681C` + `0x8DCD4`) to `mr->record`; arm 0x8D74D (§5.2); `test_engine_match_handlers::test_action_26_runs_body` | OL-50 (descriptor bytes) / OL-51 (`0x36200` gate) carry the row's remainder |
| 27 | 0x086820 | unwired (M2 arms-and-wiring Task 4 / FU-142 Appendix D; entry verdict FU-142f App. I) | body `0x86820..0x86A02` (136 insns) ported as `fifa96_arm_27_step` with the `0x79C50`/`0x6E598` helpers (`fifa96_arm_face`/`fifa96_arm_anim_select`); **no entry** — the only reference to `0x86820` is the action-table slot `0x11077C`, the `MOV EDX,0x27` sites are animation args, `MOV ECX,0x27` has no site, and the FU-142f census of all 77 `FUN_0007D9A4` call sites (register-derived args resolved) finds no 0x27 invocation and no stored installer pointer (FU-142 D.1/I) | OL-48 (entry; FU-142f negative census); FU-142 OL-52/OL-53 remainder |
| 28 | 0x0870E8 | ported (M2 arms-and-wiring Task 7 / FU-142 Appendix G) | `fifa96_match_action_28` binds `fifa96_arm_28_step` (`0x870E8..0x874E3`, 4-arm table `0x870D8` + internal `0x87014` gate helper; the `0x114E04` fold reuses `fifa96_projection_sincos`) to `mr->record`; arm 0x8D7CF (§5.2); entry resolved (the arm installs code 0x28; the only body ref is the action-table slot 0x110780); `test_engine_match_handlers::test_action_28_runs_body` | OL-56 (global inputs/RNG seed) / OL-57 (`[0x157AA3]`, row-byte stand-in) / OL-58 (chosen-record resolution) carry the row's remainder |
| 29 | 0x0874E4 | unwired (M2 arms-and-wiring Task 6 / FU-142 Appendix F; entry verdict FU-142f App. I) | body `0x874E4..0x87738` (187 insns) ported as `fifa96_arm_29_step` (the phase-5 stage machine: `0x8DE8C` nearest, `0x6E1D0` phase cell, `0x92AC8` RNG, `0x6E598` selector, `fifa96_arm_reset`, self-install code 3); **no entry** — the only reference to `0x874E4` is the action-table slot `0x110784`, the sole `MOV EDX,0x29` site is `0x1F53C` in the non-match `FUN_0001F440`, the phase-5 handler `0x6E05C..0x6E1B2` contains no installer call, and the FU-142f census of all 77 `FUN_0007D9A4` call sites finds no 0x29 invocation (FU-142 F.3/I) | OL-48 (entry; FU-142f negative census); FU-142 OL-55 remainder |
| 2A | 0x086A34 | ported (M2 arms-and-wiring Task 8 / FU-142 Appendix H) | `fifa96_match_action_2A` binds `fifa96_arm_2a_step` (`0x86A34..0x87010`, 12-arm table `0x86A04`; the `0x513EC` camera stop is the `fifa96_arm_camera_stop` derived no-op) to `mr->record`; arm 0x8D807 (§5.2); entry resolved (the arm installs code 0x2A); the frame staging computes the `+0x65` distance word; `test_engine_match_handlers::test_action_2A_runs_body` | OL-59 (`[0x157AA3]` store) / OL-60 (`+0x65` dispatch-time recompute) / OL-61 (camera-stop block) carry the row's remainder; `0x36200` value OL-51 |
| 2B | 0x087738 | **dead entry** (FU-142f) | native entry `0x87738` is the final `RET` of the row-29 body (the shared epilogue, FU-142 §1.1), not a standalone stub; the FU-142f census passes no 0x2B; the helpers at `0x8773C`/`0x8776C` belong to no action row | OL-15 (closed by the FU-142f verdict) |
| 2C | 0x084598 | unwired (M2 arms-and-wiring Task 5 / FU-142 Appendix E; entry verdict FU-142f App. I) | body `0x84598..0x8462D` (48 insns) ported as `fifa96_arm_2c_step` with the shared `FUN_0007DAB4` reset subset (`fifa96_arm_reset`); **no entry** — the only reference to `0x84598` is the action-table slot `0x110790`, the three `MOV EDX,0x2C` sites are non-match constants (0x3035E/0x40A8E/0x40C5C, none calls `0x7D9A4`), `MOV ECX,0x2C` has no site, and the FU-142f census of all 77 `FUN_0007D9A4` call sites finds no 0x2C invocation (FU-142 E.4/I) | OL-48 (entry; FU-142f negative census); FU-142 OL-54 remainder |

### 6.2 Table B — phase rows `0x110794[phase]`

All 34 non-zero rows are FU-136 §3 `not ported` and share port group T16-G;
the FU-83 §3.1/§3.2 body column is reproduced for the later cluster, with the
tested helper named where FU-136 credited one. The dispatch layer itself
(`fifa96_action_phase_install/drive/select`) is tested (`test_phase_drivers`,
`test_stage_family`) — partial coverage only; no phase body is ported.

| phase | native | class | body / helper evidence | open leg |
|---|---|---|---|---|
| 00 | 0x06DE34 | not ported | FU-83 §2/§3.1 held-position copy | OL-13 |
| 01 | 0x06E1D0 | not ported (partial) | phase_cell; resolver ptr/+2 variant unported | OL-13 |
| 02 | 0x06DCC8 | not ported (partial) | phase_cell; ptr-2 and 0x577DE lookup unported | OL-13 |
| 03 | 0x06DE44 | not ported (partial) | phase_slot; selection/ball arm unported | OL-13 |
| 04 | 0x06DE44 | not ported (partial) | same body as 03 | OL-13 |
| 05 | 0x06E05C | not ported | FU-83 distance line | OL-13 |
| 06 | 0x06DD9C | not ported (partial) | phase_ball_entry/ball_line | OL-13 |
| 07 | 0x06DE44 | not ported (partial) | same body as 03 | OL-13 |
| 08 | 0x06DD6C | not ported (partial) | wrapper -> 0x6DCC8; phase_cell | OL-13 |
| 09 | 0x06DD6C | not ported (partial) | same as 08 | OL-13 |
| 0A | 0x06DE34 | not ported | same body as 00 | OL-13 |
| 0B | 0x06DE34 | not ported | same body as 00 | OL-13 |
| 0C | 0x06DF4C | not ported (partial) | phase_line_timer/restart_line; anim-byte side effect unported | OL-13 |
| 0D | 0x06DE34 | not ported | same body as 00 | OL-13 |
| 0E | 0x06DE34 | not ported | same body as 00 | OL-13 |
| 0F | 0x06DE34 | not ported | same body as 00 | OL-13 |
| 10 | 0x06E004 | not ported | FU-83 variant tables 0x105E7/0x105E8 | OL-13 |
| 11 | 0x06E1C8 | not ported (partial) | falls into 0x6E1D0; phase_cell | OL-13 |
| 12 | 0x06E1D0 | not ported (partial) | same as 01 | OL-13 |
| 13 | 0x06E244 | not ported | FU-83 camera-bound scatter | OL-13 |
| 14 | 0x06E244 | not ported | same as 13 | OL-13 |
| 15 | 0x06DCC8 | not ported (partial) | same as 02 | OL-13 |
| 16 | **0x00000000** | unwired (zero slot) | native zero entry, loader INT3 stub, no body by design; dispatch -> `-FIFA96_ERR_NOT_FOUND` | — |
| 17 | 0x088DC8 | not ported | FU-83 §3.2 timeline | OL-13 |
| 18 | 0x08922C | not ported | FU-83 §3.2 timeline | OL-13 |
| 19 | 0x089FA4 | not ported | FU-83 §3.2 timeline; installs action 0x16 | OL-13 |
| 1A | 0x089620 | not ported | FU-83 §3.2 timeline | OL-13 |
| 1B | 0x0890EC | not ported | FU-83 §3.2 pure reset | OL-13 |
| 1C | 0x089110 | not ported | FU-83 §3.2 timeline | OL-13 |
| 1D | 0x089868 | not ported | FU-83 §3.2 timeline; installs 0x19/3 | OL-13 |
| 1E | 0x08A798 | not ported | FU-83 §3.2 timeline | OL-13 |
| 1F | 0x088F4C | not ported | FU-83 §3.2 timeline | OL-13 |
| 20 | 0x08B688 | not ported | FU-83 §3.2 timeline; installs 0x24 | OL-13 |
| 21 | 0x08B874 | not ported | FU-83 §3.2 timeline | OL-13 |
| 22 | 0x08B900 | not ported | FU-83 §3.2 timeline | OL-13 |

## 7. Totals

| surface | rows | ported | unwired | not ported | open leg | dead |
|---|---|---|---|---|---|---|
| action `0x1106E0` | 45 | 21 (`00` FU-138; `01` FU-143 §11 / M2 playable-match Task 2; `02` FU-75 L4.6 / M2 phase-9 T1; `05` FU-142 OL-63 / M2 phase-9 T2; `1E` FU-140; `1D` FU-151 P3; `26` FU-142b; `28` FU-142d; `2A` FU-142e; `07`/`0F` FU-139 §9 Task 11; `18`/`21`/`23` FU-139 §10 Task 12; `06` FU-139 §11 Task 13; `04` FU-142 K.5 Task 1; `08` FU-142 K.6 Task 2; `10`/`11`/`12`/`13` FU-149 §7 / M2 phase-8 T1) | 3 (`27` FU-142b, `2C` FU-142b, `29` FU-142c bodies ported, entries OL-48 negative per FU-142f) | 20 | 0 | 1 (`2B` FU-142f) |
| phase `0x110794` | 35 | 0 | 1 (`16`, zero slot -> `-NOT_FOUND`) | 34 | 0 | 0 |
| **dispatch total** | **80** | **21** | **4** | **54** | **0** | **1** |

Dispatch results at this commit (M2 phase-9 T2 update / FU-142 OL-63): **58 ×
`-FIFA96_ERR_UNSUPPORTED`** (the 54 not ported rows + the unwired actions
`27`/`29`/`2C` + the dead entry `2B`), **1 ×
`-FIFA96_ERR_NOT_FOUND`** (phase `0x16`) and **21 × `FIFA96_OK`** (actions
`00`, `01`, `02`, `05`, `1E`, `1D`, `26`, `28`, `2A`, `07`, `0F`, `18`, `21`,
`23`, `06`,
`04`, `08`, `10`, `11`, `12`, `13`);
out-of-range ->
`-NOT_FOUND`; NULL `mr` -> `-INVALID`. All error results are negated, matching
the engine family convention (`fifa96_match_run_*`). (Historical: the Task-1
state was 12 ported / 29 action-not-ported / 63 not ported / 67 UNSUP / 12 OK
at FU-142 K.5.)

**M2 phase-8 T3 note (2026-10-09; no row count change).** The `FUN_0007CA54`
input-row dispatch (FU-75 §1.2/§1.3) is now wired over the pool for the
slot-bearing outfield record (`match_run_outfield_input` in
`fifa96_match_run.c`), executing the reachable handler subset first-hand this
task. The pad kick derivation: carrier (`byte[rec+0x91] == 5`, `0x7CB36`)
selects dispatch code 1 and the released `0x7D110` row installs action `07`
(invoke-now) — the ported kick machine — so the FU-137 §6.1 `07` cell gains a
live pad seam. Row `19` stays `not ported (partial)`: the live post-kickoff
slot record's code in the ISO probe is `02`, and the keeper machine's own
input tables (`FUN_000782D0`, FU-74 §2) remain unported. Details and the
numbered legs are in FU-75 §10 and the ENGINE.md phase-8 T3 entry.

**M2 phase-9 T1 note (2026-10-10; row `02` flips to ported).** Row `02`
(`0x7DFCC`, FU-138's partial) is now `ported`: `fifa96_match_action_02` binds
the first-hand `0x7DFCC..0x7E1A2` body to `mr->record`/the pool — the phase-1
`fifa96_action_locomotion_restart_target` mirror, the phase-2
`[team+0x7B2] = rec`, the `0x7E041` `FUN_0007876C` merge request, the install-4
invoke-now (`0x7E046..0x7E068`) and the no-slot camera-target/`+0x89 += delta`
arm with the `fifa96_action_locomotion_restart_wait` stage-0 gate. The stage-1
arm (`0x7E0F6..0x7E184`) and the stage-2 snap stay FU-138 OL-18. §7 totals
move 19 -> 20 ported / 60 -> 59 UNSUP. The live consequence: the ISO kickoff
probe's slot record (team 0 record 9, code `02`) now installs `4` on its first
live phase-2 frame and the shared mover moves it (FU-75 §10 T1 landing;
`test_engine_match_frame::test_held_key_moves_live_controlled_record`).
Tests: `test_engine_match_handlers::test_action_02_restart_and_phase2_arms`
and the `action_expect[0x02] = FIFA96_OK` flip.

**M2 phase-9 T2 note (2026-10-10; row `05` flips to ported).** Row `05`
(`0x7F194`, FU-139 §8's partial) is now `ported` over the pool:
`fifa96_match_action_05` binds the ported `fifa96_action_carrier_arm`
stages 0-3 and applies the bounded record-visible effects — the `0x7F1FF`
`[0x158724]` possession claim (the pool `ball.carrier`) together with the
derived `0x158728..0x15872F` block (`ball.pos_*`), the `[team+0x7B2] = rec` /
`+0x7B6 = 0` bind, the capped/additive `+0x89`, the camera triple copy to
`+0x4D/+0x51/+0x55`, the stage-0 `lane > 0x40` clear-control /
`[0x157A83] = rec` set-control, the `0x15872A/B` dir bytes, the `0x7876C`
merge request (`helper_request`), the `0x79B1C` snap, the `0x79C50` face
(+0x8E) and the stage-3 team-target hand-off (install `4` on `[0x158730]` +
the `0x79B58` receiver timer on `[0x158734]`). The stage-0 target algebra
`0x7F3A1..0x7F57B` and the `FUN_0007F7E0` fallback remain named OL-63
requests (`out.tail`/`out.fallback`) with no derived consumer. Live effect:
the ISO movement path (`test_engine_match_frame::test_live_carrier_claim_and_kick`)
runs held-UP movement -> row 04's half-line coda install `5` -> row 05 claim
(`ball.carrier`/`[0x157A83]` = the slot record, possession dirs live) -> the
pad KICK release -> row `07` -> the staged ball pair; the row-06 pursuit
staging now reads the live `0x15872A/0x15872F` producers (the OL-69 half
closed). The possession-block release countdown `[0x15872D]` gains its
`0x4B163` per-frame decay (`fifa96_match_entities_update`). §7 totals move
20 -> 21 ported / 59 -> 58 UNSUP. Tests:
`test_engine_match_handlers::test_action_05_wired_carrier` /
`test_action_05_claim_dirs_and_stages`,
`test_engine_match_entities::test_possession_release_decay` and the
`action_expect[0x05] = FIFA96_OK` flip.

FU-139 §8 (Task 10) advances action row `05`'s bounded parts (the carrier
machine stages 0-3 and the staging tail are ported and tested) but adds no
dispatch: the stage-0 target algebra and the `FUN_0007F7E0` fallback stay
unported (OL-63) and the row remains `not ported (partial)` with `fn == NULL`.

FU-139 §9 (Task 11) closes OL-28 and OL-31: `fifa96_ball_kick_target`
(`FUN_0007B9C4` full bounded path incl. `FUN_0007B878` and both mode arms) and
`fifa96_action_kick_machine` (rows 07/0F) are ported and tested, rows `07` and
`0F` flip to `ported` with `fn != NULL`, and the counts above move 5 -> 7 OK /
74 -> 72 UNSUP. The request surfaces (defender 0x0E, opponent 0x22, ball 4,
reset/merge) drain through the FU-141 pool installer; the unmodeled record
bytes and external tables are the OL-65/OL-66 legs.

FU-139 §11 (Task 13) closes OL-30: `fifa96_action_pursuit_step`
(`0x801B4..0x809EF`, the carrier gate + target construction + `0x114E04`
folds + RNG installs 8/9 + the `0x8DE8C`/`0x79CCC` selections) and
`fifa96_match_action_06` are ported and tested, row `06` flips to `ported`
with `fn != NULL`, and the counts move 10 -> 11 OK / 69 -> 68 UNSUP. The plan
span `0x801B4..0x81067` includes the row-09 handler `0x80A00` (slot
`0x1106E0[9]`), which stays `not ported (partial)` (OL-9); the row-06
remainder is OL-69.

M2 arms-and-wiring Task 14 (FU-142 Appendix K) closes OL-41 (the interception
bind `FUN_0008D824` and band `FUN_000795B4` are ported as
`fifa96_entity_intercept_bind`/`_band`, feeding `team+0x7BE`) and ports the
OL-38 `FUN_0007CA54` machine subset (`fifa96_outfield_input_row`,
`fifa96_outfield_chase_gate`, the flat `0x110680` type gate). No action row
flips: rows `04`/`08`'s full bodies (574 + 213 defined-code insns) are split
to OL-70/OL-70a per the plan's row-window rule, so the counts stay 11 OK / 68 UNSUP / 1 NOTF
and the two §6.1 evidence cells name OL-70. FU-75 §1.3's pressed->released
fall-through is corrected by the first-hand bytes (the pressed terminator
jumps to the tail; the released table runs only when the pressed word is
zero) — see FU-142 K.3.

M2 playability-legs Task 1 (FU-142 Appendix K.5) closes OL-70: the row-04
record-visible body `0x7E7C8..0x7F141` is ported as
`fifa96_outfield_row04_step` and wired as `fifa96_match_action_04` over the
FU-141 pool, so row `04` flips to `ported` and the counts move 11 -> 12 OK /
68 -> 67 UNSUP. The `0x7CA54` input-row machine remains its own (unwired)
seam; the unmodeled record/team bytes, the `0x1577F0..0x157806` track words,
the `0x71B9C` predictor and the event/audio sinks are OL-72 (K.5.8). A
first-hand erratum for K.3/FU-77: the `0x110680` gates are indexed by the
action-code byte `+0x91` (the installer's target), not `+0x8E`; the pool
wrapper's naming is unaffected.

M2 playability-legs Task 2 (FU-142 Appendix K.6) closes OL-70a: the row-08
record-visible body `0x81068..0x814AF` is ported as
`fifa96_outfield_row08_step` and wired as `fifa96_match_action_08` over the
FU-141 pool (row 08 installs no code, so there is no install arm), so row `08`
flips to `ported` and the counts move 12 -> 13 OK / 67 -> 66 UNSUP. The
first-hand linear sweep corrects the span's defined-code count to 329
instructions including the RET; the unmodeled record/process bytes, the
`0x7A490` staging call and the event/audio sinks are OL-82 (K.6.7), and the
+0x3D frame gate keeps the projection scan inert live until the OL-80
producer lands.

## 8. Open legs

1. **OL-1 — entity/record model and machine binding.** The 0xB2-stride record,
   its `[rec+0x18]` handler slot, and the record fields the machines stage
   (`+0x81`, `+0x89`, `+0x91`, `+0x92`, `+0x9A`, `+0x9E`, `+0x9F`) have no
   engine surface. Task 5 (FU-138 §4) added the minimal one-record action
   surface and wired action 00 through it; Task 7 (FU-140 §4) bound the keeper
   claim row 1E to the same record (its own fields `+0x8F`/`+0x9B`/`+0x5D` and
   derived request flags). Every other row still waits for the full record pool
   (FU-138 OL-16, FU-140 OL-37).
2. **OL-2 — installer tails/reset chain.** `FUN_0007DAB4` (reset/chooser),
   `FUN_0006E8E8`, `FUN_00079B1C`, `FUN_00079F3C`, `FUN_0007BF20` are not
   ported; handler side effects through them are absent.
3. **OL-3 — outfield record machine** `FUN_0007CA54` + input-row dispatcher
   tables `0x1109D0`/`0x1109E4` + the 14 row handler bodies.
4. **OL-4 — keeper record machine** `FUN_000782D0` + decision `FUN_00077EAC` +
   keeper input tables (`0x110950`..`0x1109E0`).
5. **OL-5 — phase entry/clock machine.** `FUN_000740A0`/`FUN_0008D098` switch,
   `[0x157A4A]` semantics and the per-record 11-slot loop (also the G1
   selector-0/phase-0 caveat).
6. **OL-6 — ball staging/resolver.** `FUN_0007A490`, `FUN_0007AE70` and the
   full kick target selection `0x7BA1E..0x7BBE4`.
7. **OL-7 — RNG.** `FUN_00092AC8`/`FUN_000CB2A4` (phase arms 4, 0x13/0x14 and
   most scatter/duel bodies).
8. **OL-8 — action bodies group B** (03, 04, 05, 06, 07, 08, 0F).
9. **OL-9 — action bodies group C** (01, 02, 09, 0B–17, 20, 22, 24, 25).
10. **OL-10 — keeper bodies** (19–1D, 1F) + `FUN_00076130` input handler;
    FU-140 refines these into OL-33..OL-36 (row 1E's arms are OL-37) after
    porting the pure parts.
11. **OL-11 — duel/tackle/receive bodies** (18, 21, 23) + NSEARCH/SWAP.
12. **OL-12 — entity frame chain** `FUN_0004B100`, control slots, camera/track.
13. **OL-13 — phase bodies** (34 rows) + phase entry wrappers.
14. **OL-14 — residual/unclassified** (0A, 15) + dead-code classification.
15. **OL-15 — installer arms `0x26`–`0x2C`.** Arms resolved for 26/28/2A
    (§5.2); bodies 0x0866F4/0x0870E8 unanalyzed, 0x086A34 unported. No static
    install arm found for 27/29/2B/2C anywhere in the program; 0x2B's body is a
    one-byte RET and 0x27's only constant is an animation-row argument. These
    four rows stay unscheduled until a reachability/dynamic pass (or the FU-142
    probe for plan C9) resolves their entry. **Status (FU-142b Tasks 3-5,
    FU-142c Task 6, FU-142d Task 7, FU-142e Task 8): rows 26, 28 and 2A are
    ported/wired; rows 27, 2C and 29 have ported bodies but their entries stay
    unresolved (OL-48/FU-142f); 2B remains unscheduled (the shared row-29
    epilogue RET entry, FU-142 §1.1). The 0x2C body is not "prologue-only": it
    is a 48-instruction stage machine (FU-142 §7 / Appendix E); the 0x29 body
    is a 187-instruction phase-5 stage machine (FU-142 Appendix F); the 0x28
    body is a 294-instruction 4-arm machine with an internal gate helper
    (FU-142 Appendix G); the 0x2A body is a 409-instruction 12-arm machine
    plus the 0x513EC camera-stop call (FU-142 Appendix H). Task 9 (FU-142f)
    status: the FU-142 Appendix I census classifies all 77 `FUN_0007D9A4` call
    sites (register-derived arguments included, bypass/indirect checks done);
    the installer code domain is {0x00..0x26} ∪ {0x28,0x2A}, so 27/29/2C are
    never passed and stay unwired with the OL-48 negative verdict, and 2B is a
    dead entry (class `dead entry`, this leg's actionable part closed).**

## 9. Concerns

* The 77 `FUN_0007D9A4` call sites make the installer the most-called match
  helper; clusters C5–C10 should wire rows through this dispatch rather than
  new call sites, otherwise the single-row seam and the machines will diverge.
* `0x26`/`0x27`/`0x28` are overloaded as animation-row ids in handler bodies
  (`FUN_0008CFAC`/`FUN_0006E598` calls). A later slice must not mistake those
  calls for action installs; §5.3 records the distinction.
* The `0x2A` arm writes `[team+0x831]` (the chosen record) outside the record
  it installs into; the engine seam has no team/record surface yet, so the
  field is documented but unused (OL-1/OL-15).
* Phase dispatch is per-record (11 × 0xB2), not global; the engine seam's
  `fifa96_match_dispatch_phase(mr, phase)` is therefore a row resolver only —
  Task 10 must add the record walk and keep the zero-slot `NOT_FOUND`.

## Errata (M2 Task 5 / FU-138)

* §6.1 action row `00` moves from `unwired`/OL-1 to `ported`: Task 5 added
  `fifa96_match_action_00` (bound to the minimal `mr->record` surface) and
  updated the §7 totals and dispatch-result paragraph in place. The row's RE
  evidence is unchanged (FU-76 §3.1/§4); the class change is a wiring status
  update the §6 rubric anticipates ("body in C **and** wired"). The record pool
  is FU-138 OL-16 = the carried remainder of this doc's OL-1.
* Action rows `01`/`02`/`03`/`0D` keep their `not ported (partial)` class; FU-138
  §3 adds their derived pure parts (`sequence_marker_target`, `stage_wait`,
  `locomotion_restart_wait`, `locomotion_placement_counter`,
  `locomotion_phase1_clamp`, `sequence_velocity_scale`) as additional tested
  helper coverage. Their wiring stays open (01 → FU-138 OL-17, 02 → OL-18,
  03 → OL-19, 0D → OL-22).

## Errata (M2 Task 6 / FU-139)

* §6.1 action rows `05`/`06`/`07`/`0F`/`18`/`21`/`23` keep their class
  (`not ported` / `not ported (partial)`); their evidence cells gain the FU-139
  cross-refs and their leg cells the new actionable legs `OL-29`..`OL-32`
  (carrying OL-1/OL-16). FU-139 §3.7 adds seven tested pure helpers
  (ball clear/stage, reception target, kick range band, event-row resolver,
  append selector, row-07 stage-0 target); FU-139 §4 records why no row is
  wired (unported arms + absent entity/ball pool). §7 totals and the
  dispatch-result paragraph are unchanged: `1 × FIFA96_OK` (action `00`),
  `78 × -FIFA96_ERR_UNSUPPORTED`, `1 × -FIFA96_ERR_NOT_FOUND` (phase `0x16`).
* The `fifa96_match_handlers.c` evidence strings for those rows are updated in
  place (compact form of the new §6.1 cells); `test_engine_match_handlers.c`
  keeps their dispatch expectations `UNSUPPORTED` and documents the FU-139
  reason.

## Errata (M2 Task 7 / FU-140)

* §6.1 action row `1E` moves from `unwired`/OL-1 to `ported`: Task 7 added
  `fifa96_match_action_1E` (bound to the extended minimal `mr->record` surface,
  FU-140 §4) and updated the §7 totals and dispatch-result paragraph in place.
  The row's RE evidence is refined by FU-140 §2/§3 (the full 59-instruction
  body re-read; `stage`/`has_ball`/`pos_y` fields and the `FUN_0007876C` /
  `FUN_000700F4` request stand-ins). The record pool is FU-140 OL-37 = the
  carried remainder of OL-1/OL-16.
* Action rows `19`/`1A`/`1B`/`1C`/`1D`/`1F` keep their `not ported (partial)`
  class; FU-140 §3 adds three tested pure helpers (`keeper_input_decide` for
  `FUN_00076130`, `keeper_hold_fallback` for the row-19 weighted camera
  fallback, `keeper_arm_camera` for the row-1F prologue) and re-numbers the
  actionable legs OL-33..OL-37 (carrying OL-16). FU-140 corrects four FU-79
  claims (FU-140 Errata).
* The `fifa96_match_handlers.c` evidence strings for the keeper rows are
  updated in place (compact form of the new §6.1 cells); the engine links
  `fifa96_keeper` for the wired row.

## Provenance

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit): `read_memory
0x1106E0` (180 B), `read_memory 0x110794` (140 B), `search_instructions
operand=0x1106e0` (1 match `FUN_0007D9A4 @ 0x7DA77`), `search_instructions
operand=0x110794` (1 match `FUN_0006D920 @ 0x6D9B3`), `disassemble_function
0x7D9A4`, `decompile_function 0x7D9A4/0x6D920/0x7CA54/0x782D0/0x8CEB8/0x7DAB4/
0x8CFAC`, `disassemble_bytes 0x6D9A0..0x6D9D0`, `0x8D098..0x8D19F`,
`0x8D560..0x8D69F`, `0x8D720..0x8D814`, `0x7CCF0..0x7CD4F`,
`0x78540..0x7859F`, `0x87738..0x8774F`, `get_xrefs_to 0x7D9A4` (77),
`get_xrefs_to 0x8CEB8` (15), `get_xrefs_to 0x6D920` (4),
`search_instructions mnemonic=mov operand=EDX,0x26..0x2C` (+`DX,` forms).
No writes: no rename/comment/label/function/script/project save. `/fifa96.exe`
(the MZ loader) and `/fifa96_le.bin` untouched.

Repo: `make check` 97/97 at task start, 98/98 with the new
`test_engine_match_handlers` (ASan/UBSan); M1 golden and pinned render hashes
unchanged. The `fifa96_match_handlers.c` evidence strings are the compact form
of §6.1/§6.2 and must be kept in sync with this doc.

Write set: this doc; `include/fifa96_engine/fifa96_match_handlers.h`;
`src/fifa96_engine/fifa96_match_handlers.c`; `tests/test_engine_match_handlers.c`;
`CMakeLists.txt` (engine source + test registration). No asset/ISO/Ghidra
change.

## Errata (M2 Task 8 / FU-141)

* **§4.1 record timer source** — "word `[rec+0x81]` and byte `[rec+0x93]` are
  decremented by `[0x157A62]`/its low byte" is refined: `FUN_0007CA54`
  (`0x7CA6E`/`0x7CA9A`) loads the word `[0x157A64]` (the frame delta) and
  compares it against `[rec+0x7F]>>16` (word timer) or the raw byte (byte
  timer) with a signed-word/zero-extended-word compare. Ported exactly in
  `fifa96_match_entities` (FU-141 §2.3).
* **OL-1 record surface** — the C8/FU-141 pool closes the record-surface half:
  the 0xB2 record pool, the team blocks and the request drains for the two
  wired rows exist (`fifa96_match_entities`). The machine/arm halves
  (FUN_0007CA54/FUN_000782D0, installer invoke, animation arms) remain
  FU-141 OL-42..OL-44.
* **§6.1 action rows `00`/`1E`** — stay `ported`; their evidence strings now
  cite FU-141 and the pool binding. No other row changes class (FU-141 §6).

## Errata (M2 arms-and-wiring Task 3 / FU-142b)

* §6.1 action row `26` moves from `not ported`/OL-15 to `ported`: Task 3 added
  `fifa96_match_action_26`, binding the ported `fifa96_arm_26_step`
  (`0x866F4..0x8681C`, 90 instructions) through the shared `0x8DCD4` helper
  (`fifa96_arm_dist_stage`) to `mr->record`. The install arm 0x8D74D (§5.2)
  was already derived by FU-142a, and the FU-141 pool now stages/repacks the
  row's `stage92`/`timer7b`/`lane` fields; the row is the first cluster-G
  wiring because its arm + body + pool binding are all bounded. The row's
  remaining RE surface is the unmodeled `rec[+0x4]` descriptor bytes
  (`P[+0xD]`/`P[+0xE]`, FU-142 OL-50) and the `0x36200` gate global
  (FU-142 OL-51).
* §7 totals and the dispatch-result paragraph are updated in place: action
  ported 2 -> 3, not ported 39 -> 38; dispatch 77 -> 76 ×
  `-FIFA96_ERR_UNSUPPORTED`, 2 -> 3 × `FIFA96_OK`, `-NOT_FOUND` unchanged
  (phase `0x16`).
* The `fifa96_match_run_record` staging surface gains the FU-142b fields
  (`stage92`, `timer7b`, `lane`, `player_d`, `player_e`); the two descriptor
  bytes are the derived 0 default until FU-142 OL-50 closes. Rows
  27/28/29/2A/2C keep their classes (28/2A their FU-142d/e body tasks; 27/29/2C
  the OL-48 entry verdict).
* §5.2 is unchanged by this task; the `0x26` zero-extended side-test handling
  stands as recorded in the Task 2 errata.

## Errata (M2 arms-and-wiring Task 4 / FU-142b)

* §6.1 action row `27` moves from `open leg`/OL-15 to `unwired`: Task 4 ported
  the body `0x86820..0x86A02` (136 instructions) as `fifa96_arm_27_step`,
  plus the shared `0x79C50` face helper (`fifa96_arm_face`, the FU-76 §5
  `fifa96_action_kick_angle` angle primitive) and the `0x6E598` id-resolution
  subset (`fifa96_arm_anim_select`) — FU-142 Appendix D. The entry stays
  unresolved: first-hand, the only reference to `0x86820` in `/FIFA96.EXE` is
  the action-table slot `0x11077C`; the two `MOV EDX,0x27` sites are
  animation-row arguments (`0x756D5` -> `CALL 0x6E598`; `0x1F52E` -> the
  `FUN_0001F440` jump-table tail `0x1F55D CALL 0x13600`), `MOV ECX,0x27` has
  no real site and `0x8CEB8`'s 15 arms stage no code 0x27 (FU-142 D.1). A
  register-derived installer argument cannot be excluded by a constant census,
  so FU-142f (runtime/reachability) remains the precondition.
  `fifa96_match_action_table[0x27].fn` stays **NULL**, its evidence names
  `FU-142b ... entry unresolved (FU-142f/OL-48)`, and the dispatch result is
  unchanged (`-FIFA96_ERR_UNSUPPORTED`). The row's remainder surfaces are
  FU-142 OL-52 (unmodeled selector record writes, `[0x57A6C]` side byte and
  RNG reroll) and OL-53 (pair-walk globals and the bounded 24-pair read).
* §7 totals update in place: action unwired 0 -> 1 (`27`), open leg 4 -> 3
  (`29`, `2B`, `2C`); dispatch total unwired 1 -> 2, open leg 4 -> 3, ported 3,
  not ported 72. The dispatch-result paragraph is updated in place:
  `-UNSUPPORTED` stays 76, `FIFA96_OK` stays 3, `-NOT_FOUND` unchanged (phase
  `0x16`).
* `tests/test_engine_match_handlers.c` gains `test_action_27_unwired_entry`
  (row fn NULL, evidence names FU-142b/OL-48/UNSUPPORTED, dispatch UNSUP);
  `action_expect[0x27]` stays `UNSUP`.
* The `fifa96_arm_record` view (loader-side) gains `anim_cycle`, `anim_cursor`,
  `anim_sel`, `flag44` and `anim_overflow`; `fifa96_match_run` staging is
  unchanged because the row is unwired. Rows 28/29/2A/2C keep their classes
  (28/2A their FU-142d/e body tasks; 29/2C the OL-48 entry verdict).

## Errata (M2 arms-and-wiring Task 5 / FU-142b)

* §6.1 action row `2C` moves from `open leg`/OL-15 to `unwired`: Task 5 ported
  the body `0x84598..0x8462D` (48 instructions) as `fifa96_arm_2c_step`, plus
  the shared `FUN_0007DAB4` derived reset subset (`fifa96_arm_reset`,
  `stage92 = 0xFF`, `timer89 = 0`, `code = 0`) — FU-142 Appendix E.
  "Prologue-only body" (§6.1/§5.3's wording) is corrected per FU-142 §7: the
  body is a real stage-latch machine (`+0x92` 0/1/2, calls the constant-id
  `0x6E598` selector (id 0x5D) and `0x7DAB4`). The entry stays unresolved:
  first-hand, the only reference to `0x84598` in `/FIFA96.EXE` is the
  action-table slot `0x110790`; the three `MOV EDX,0x2C` sites are non-match
  constants (`0x3035E` in `FUN_000302DC`, `0x40A8E`/`0x40C5C` in
  `FUN_00040A5C`; none calls `0x7D9A4`), `MOV ECX,0x2C` has no site and
  `0x8CEB8`'s 15 arms stage no code 0x2C (FU-142 E.4). A register-derived
  installer argument cannot be excluded by a constant census, so FU-142f
  (runtime/reachability) remains the precondition.
  `fifa96_match_action_table[0x2C].fn` stays **NULL**, its evidence names
  `FU-142b ... entry unresolved (FU-142f/OL-48)`, and the dispatch result is
  unchanged (`-FIFA96_ERR_UNSUPPORTED`). The row's remainder surface is
  FU-142 OL-54 (the reset's `[rec+0x20]` slot callback, the phase-2
  forced-decision arm and the native installer's accepted-install tail).
* §7 totals update in place: action unwired 1 -> 2 (`27`, `2C`), open leg
  3 -> 2 (`29`, `2B`); dispatch total unwired 2 -> 3, open leg 3 -> 2, ported
  3, not ported 72. The dispatch-result paragraph is updated in place:
  `-UNSUPPORTED` stays 76, `FIFA96_OK` stays 3, `-NOT_FOUND` unchanged (phase
  `0x16`).
* §5.3 row `0x2C` — the constant census re-verifies first-hand this slice (3
  `MOV EDX,0x2C` sites, none an installer, plus 1 body-pointer hit at the
  action-table slot); the "body unanalyzed" cell is superseded by the FU-142
  §1/Appendix E window (48 instructions, `0x84598..0x8462D`).
* §8 OL-15 gains the FU-142b Tasks 3-5 status (row 26 wired; rows 27/2C bodies
  ported with entries OL-48; 29/2B unscheduled) and the 0x2C
  "prologue-only" correction.
* `tests/test_engine_match_handlers.c` gains `test_action_2C_unwired_entry`
  (row fn NULL, evidence names FU-142b/OL-48/UNSUPPORTED, dispatch UNSUP);
  `action_expect[0x2C]` stays `UNSUP`.
* The `fifa96_arm_record` view is unchanged; the loader-side helper surface
  gains `fifa96_arm_reset` and the body surface `fifa96_arm_2c_step`.
  `fifa96_match_run` staging is unchanged because the row is unwired. Rows
  28/29/2A keep their classes (28/2A their FU-142d/e body tasks; 29 the
  OL-48 entry verdict).

## Errata (M2 arms-and-wiring Task 6 / FU-142c)

* §6.1 action row `29` moves from `open leg`/OL-15 to `unwired`: Task 6 ported
  the body `0x874E4..0x87738` (187 instructions) as `fifa96_arm_29_step` —
  FU-142 Appendix F. The body is the phase-5 stage machine: the
  `[0x157A4A]>>24 == 5` gate; otherwise sync + `fifa96_arm_reset` + the
  code-3 install request (`0x8753C`, gated by `+0x9A == 0`). In phase 5:
  the `0x8DE8C` nearest search over `[rec+0]`'s 11 records
  (`fifa96_entity_find_nearest`), the `[0x10F36C]` chase park, one `0x92AC8`
  RNG draw (`& 1` -> id 0x5D/0x46) through `fifa96_arm_anim_select`, the
  `P[+0xE]` wait gate, the `0x6E1D0` phase cell (`fifa96_action_phase_cell`),
  the `0x8DCD4` distance gate (`fifa96_arm_dist_stage`) and the stage-2 target
  sync. The entry stays unresolved: first-hand, the only reference to
  `0x874E4` in `/FIFA96.EXE` is the action-table slot `0x110784`
  (`get_xrefs_to` 1; `search_byte_patterns e4 74 08 00` 1 hit); the sole
  `MOV EDX,0x29` site is `0x1F53C` inside the non-match `FUN_0001F440`
  (jump-table tail `0x1F55D MOV EAX,0x6B; CALL 0x13600`, no installer),
  `MOV ECX,0x29` has no site, the phase-5 handler `0x6E05C..0x6E1B2` — the
  body's own phase gate's handler — contains no `FUN_0007D9A4` call, and
  `0x8CEB8`'s 15 arms stage no code 0x29 (FU-142 F.3). A register-derived
  installer argument cannot be excluded by a constant census, so FU-142f
  (runtime/reachability) remains the precondition.
  `fifa96_match_action_table[0x29].fn` stays **NULL**, its evidence names
  `FU-142c ... entry unresolved (FU-142f/OL-48)`, and the dispatch result is
  unchanged (`-FIFA96_ERR_UNSUPPORTED`). The row's remainder surface is
  FU-142 OL-55 (the `[0x157AA3]` store, the `0x36200` value 2 shared with
  OL-51, the `[rec+8]`/`[rec+0]` pointer sources and the multi-record
  `[0x10F36C]` identity).
* §7 totals update in place: action unwired 2 -> 3 (`27`, `2C`, `29`), open
  leg 2 -> 1 (`2B`); dispatch total unwired 3 -> 4, open leg 2 -> 1, ported 3,
  not ported 72. The dispatch-result paragraph is updated in place:
  `-UNSUPPORTED` stays 76, `FIFA96_OK` stays 3, `-NOT_FOUND` unchanged (phase
  `0x16`).
* §5.3 row `0x29` — the constant census re-verifies first-hand this slice
  (`MOV EDX,0x29` 1 real site, `MOV ECX,0x29` 0 sites, both bodies/pointers
  swept), and the phase-5-handler probe is new: the body's own `0x6E05C`
  gate handler installs nothing.
* §8 OL-15 gains the FU-142c Task 6 status (rows 27/29/2C bodies ported with
  entries OL-48; 2B unscheduled) and the 0x29 body window.
* `tests/test_engine_match_handlers.c` gains `test_action_29_unwired_entry`
  (row fn NULL, evidence names FU-142c/OL-48/UNSUPPORTED, dispatch UNSUP);
  `action_expect[0x29]` stays `UNSUP`.
* The `fifa96_arm_record` view gains the row-29 fields (`phase`, `skip_9a`,
  `team_index`, `ball_skip`, `chase`, `side_controlled`, `install`,
  `cell[2][2]`, `ball_pos`, `team_candidates`) and the body surface gains
  `fifa96_arm_29_step`. `fifa96_match_run` staging is unchanged because the
  row is unwired. Rows 28/2A keep their classes (their FU-142d/e body tasks);
  OL-54's pool-drain decision is recorded in FU-142 §6.

## Errata (M2 arms-and-wiring Task 7 / FU-142d)

* §6.1 action row `28` moves from `not ported`/OL-15 to `ported`: Task 7 ported
  the body `0x870E8..0x874E3` (294 instructions, 4-arm jump table `0x870D8`)
  and its internal `0x87014` gate helper as `fifa96_arm_28_step` — FU-142
  Appendix G. The body is the two-stage approach machine: the prologue
  `0x8DCD4` out triple + `0x79C50` face; arm 0 builds the set-piece target
  `([0x10F364], [0x10F368])` (side-0 z negate below mode 4) and adds the
  `0x114E04` fold (`(int8)active` -> `144*cos/sin`, ported through the existing
  `fifa96_projection_sincos`); arm 1 waits on `+0xA2` and either syncs
  `target = pos` (team `flag830` clear) or runs the six-draw `0x87014` setup
  and latches; arm 2 runs the `+0xAA`/`+0xAE` approach, the `0x7D8B0`/`0x7D8C0`
  animation-id tables and the `[team+0x831]` chosen-record copy. The **entry is
  resolved** (unlike 27/29/2C): `get_xrefs_to 0x870E8` returns exactly the
  action-table slot `0x110780` and the FU-142a arm `0x8D7CF` statically
  installs code 0x28 (`FUN_0008CEB8` -> `FUN_0007D9A4`; FU-142 G.2).
  `fifa96_match_action_table[0x28].fn` is set, its evidence names
  `FU-142d`, and the dispatch result is `FIFA96_OK`
  (`test_engine_match_handlers::test_action_28_runs_body`). The row's remainder
  surfaces are FU-142 OL-56 (the five process-global inputs
  `0x10F358/35C/364/368`/`0x157AC2` and the derived RNG seed 0; first-hand
  writers are `FUN_0008D098`'s entry block/row 2A/installer clear/init),
  OL-57 (the `[0x157AA3]` store and the `[rec+0x28]` row-byte stand-in) and
  OL-58 (the chosen-record pointer -> pool-id resolution and the missing-chosen
  bounded model).
* §7 totals update in place: action ported 3 -> 4 (`28`), not ported 38 -> 37;
  dispatch total ported 3 -> 4, not ported 72 -> 71. The dispatch-result
  paragraph is updated in place: `-UNSUPPORTED` 76 -> 75, `FIFA96_OK` 3 -> 4,
  `-NOT_FOUND` unchanged (phase `0x16`).
* §5.2 arm `0x28` — re-verified first-hand this slice: `PUSH -1; MOV ECX,0x28;
  MOV EBX,0xA; MOV EAX,EBP; XOR EDX,EDX; CALL FUN_0008CEB8` at
  `0x8D7BF..0x8D7CF`, falling into the 0x2A scan; the arm's code is now backed
  by a ported, wired body (FU-142 Appendix G).
* §8 OL-15 gains the FU-142d Task 7 status (row 28 ported/wired; 2A still its
  FU-142e body task) and the 0x28 body window.
* `tests/test_engine_match_handlers.c` gains `test_action_28_runs_body` and
  flips `action_expect[0x28]` to `FIFA96_OK`; `tests/test_arm_bodies.c` gains
  `test_arm_28_*` (16 cases); `tests/test_engine_match_frame.c` gains
  `test_action_28_repack_round_trips_fields` (pool staging/repack round-trip:
  scratch persistence, `type`, the resolved chosen triple and the latches).
* The `fifa96_arm_record` view gains the row-28 scratch/global/chosen fields;
  `fifa96_match_run_record` gains `target_y`, `vel_x/vel_z`, `type`, `side`,
  `flag830`, the chosen triple, the six scratch cells and the five globals;
  `struct fifa96_match_run` gains `struct fifa96_rng rng` (seeded 0 by begin)
  and the five globals; `struct fifa96_match_entity` gains the six scratch
  cells. The engine's first wired RNG-drawing row is row 28 (OL-56 carries the
  native seed `settings[0x18]`). Rows 27/29/2C/2A keep their classes.

## Errata (M2 arms-and-wiring Task 9 / FU-142f)

* **Entry closure for 27/29/2C (FU-142 Appendix I).** All 77 `FUN_0007D9A4`
  call sites are classified first-hand: 66 are immediate-code calls, six load
  EDX from a constant-bounded register/stack word (I.4 #1/#2/#3/#5/#7/#8),
  four have control-flow entries bypassing the linear write (I.4
  #6/#9/#10/#11, each bypass value one of the constants already unioned), and
  one (`0x766b4`) is a callee-preserved EDX=4 (66 + 6 + 4 + 1 = 77). The
  union is
  {0x00..0x26} ∪ {0x28,0x2A}; 0x27/0x29/0x2C are never passed. No stored
  installer pointer exists (`search_byte_patterns a4 d9 07 00` = no matches),
  the action table's sole reader is the installer itself (`0x7DA77`), and each
  body pointer occurs only in its table slot — no indirect entry exists.
  Rows 27/29/2C stay unwired with the **OL-48 negative verdict** (not wireable
  without claiming an entry the program does not have); their evidence strings
  and the `test_engine_match_handlers` per-row pins now name FU-142f.
* **§6.1 row 2B -> `dead entry`.** `0x87738` is the row-29 shared epilogue RET
  (FU-142 §1.1), not a standalone stub, and the census passes no 0x2B. OL-15's
  actionable part closes; §7 gains the `dead` column (open leg 1 -> 0, dead 1)
  and the dispatch-result paragraph now names 2B as a dead entry (arithmetic
  unchanged: 74 UNSUP / 5 OK / 1 NOTF).
* **§6 classifier** gains the `dead entry` class (no distinct body and/or the
  code is never installed; not schedulable, no actionable leg).
* `tests/test_engine_match_handlers.c` gains `test_dead_2b_evidence` and pins
  `FU-142f` in the 27/29/2C entry tests; the `fifa96_match_handlers.c`
  evidence strings cite `FU-142f App. I`.

## Errata (M2 arms-and-wiring Task 10 / FU-139 §8)

* **§6.1 action row `05` stays `not ported (partial)` but advances.** Task 10
  ports the bounded row-05 carrier machine
  (`fifa96_action_carrier_arm`, `0x7F194..0x7F665`: phase/claim/timer head, the
  stage-0 gates with the existing dribble-dir helper, stages 1-3 with the
  `0x92820`/`0x6E598`/`0x79B1C`/`0x79C50`/`0x7D9A4` surfaces) and the
  `FUN_0007A490` code-keyed staging tail
  (`fifa96_ball_pair_stage_tail`, `0x7A8D1..0x7AA2F`), both tested (FU-139 §8).
  The stage-0 target algebra (`0x7F3A1..0x7F57B`, incl. the
  `0x71C94`/`0x79CCC`/`0x6DA64` call bodies and the record target writes) and
  the `FUN_0007F7E0` fallback remain unported, so the wiring gate keeps
  `fn == NULL` with the refined leg **OL-63**; the §7 counts and dispatch
  results are unchanged by this task (74 UNSUP / 5 OK / 1 NOTF). The native
  `0x7F19F` `byte[+0x9E] = 1` latch (the engine `ran` field) is tracked through
  `fifa96_action_carrier_out.ran_set` so a future wiring cannot drop it, and
  `tail` mirrors the `0x7F374..0x7F380` `type8 == 5` gate on the fallback path
  (fix round 1).
* **§8 OL-15 remainder / row-05 evidence** — the `fifa96_match_handlers.c` row
  `05` evidence names FU-139 §8 and OL-63; `tests/test_engine_match_handlers.c`
  gains `test_action_05_unwired_carrier` (fn NULL, evidence names
  FU-139/OL-63/UNSUPPORTED, `action_expect[0x05]` stays `UNSUP`).
* **FU-139 §3.3 staging-block vector erratum** — the block at `0x158738` holds
  the `FUN_0008DCD4` out triple `{distance, dx, dz}`, not `{x, height, z}`
  (first-hand: `0x7F666 0x7F6AF..0x7F6BB` writes it, and the staging tail reads
  dword `0x158738` >> 16 as the x addend and dword `0x15873A` >> 16 as the z
  addend at `0x7AA75..0x7AA99`). `fifa96_ball_pair_state.vector` is unchanged
  (no repurposing); FU-139 §8 records the semantic correction.
* New numbered legs OL-62..OL-64 are registered in FU-142 §6 with their
  FU-139 §8 detail (staging-tail residual, row-05 residual + non-wiring
  verdict, and the stage-0 -> stage-1 re-install edge).

## Errata (M2 arms-and-wiring Task 11 / FU-139 §9)

* **§6.1 action rows `07` and `0F` flip to `ported`.** `FUN_0007B9C4`
  (`0x7B9C4..0x7BF16`) is ported in full as `fifa96_ball_kick_target`
  (prologue/vector, slot latch + L1/L2, the `FUN_0007B878` direction arm, the
  negative-mode band, the `FUN_0007B194`/`FUN_0007B57C` mode arms, the
  `FUN_0007AE70` row resolver on the EXE row tables, the lower-first clamp with
  the `0x114E04` angle fold, the code-4 RNG/divisor arm and the `0x460` cap,
  then the `FUN_0007A490` stage + staging tail). The row machines
  (`0x814B0..0x81737`, `0x82AD0..0x82DCF`) are `fifa96_action_kick_machine`
  with `kick`/`corner_kick` requests run through `fifa96_ball_kick_target` by
  the engine handlers. The pre-Task-11 prose that called row `07`
  `not ported (partial)`/OL-31 and row `0F` `KICK 0x7B9C4 body; OL-31` is
  superseded (FU-139 §9, FU-142 OL-28/OL-31 status).
* **FU-76 §3.3 `0x58742 & 0x20` arm condition corrected.** The mode arms run
  from the fresh `0x158742` byte at `0x7BC1A..0x7BC80`: inactive + `& 0x20` ->
  `FUN_0007B57C(actor, vector, 0)`; active + byte `== 0x40` ->
  `FUN_0007B194(actor, vector, traj)`; active + (`& 0x20` or L1) ->
  `FUN_0007B57C(actor, vector, L1)` (L1 = active && phase 2 && slot word
  `0x20` bit && `slot[+0x23] < 7`). FU-76 §3.3's `([0x58742] & 0x20) &&
  [rec+0x8D] != 0` conflates the two `B57C` arms and omits the active
  `== 0x40` precedence.
* **FU-76 §3.3 code-4 divisor correction.** The divisor register is loaded
  from `row[+8]` and overwritten by the code-4 arm (mode 0x40:
  `traj = 0x90 + (rng & 7) * (0x10 - desc15)`, SI zeroed; else
  `(rng & 0x7F) + 3`); the divisor line computes
  `traj += (int16)word[0x158738] / (int16)divisor` (the addend is the traj
  word `0x15873E`, per `0x7BEB6`). The `0x460` cap compares and clamps
  `word[0x15873E]` (FU-139 §9.2, first-hand).
* **FU-139 §3.4 `fifa96_action_kick_range_band` boundary wording.** The band
  is applied only on the no-slot / direction-arm paths (`0x7BBE4`); the wing
  arm (`0x7BB46`) jumps to `0x7BC1A` and keeps the mode byte (FU-139 §9.1).
* **§7 totals update in place:** action ported 5 -> 7 (`07`, `0F`), not ported
  36 -> 34; dispatch 74 -> 72 `-UNSUPPORTED`, 5 -> 7 `FIFA96_OK`;
  `tests/test_engine_match_handlers.c` flips `action_expect[0x07]`/`[0x0F]` to
  `FIFA96_OK` and adds `test_action_07_runs_body`/`test_action_0F_runs_body`.
* New numbered legs OL-65/OL-66 are registered in FU-142 §6 (row 07/0F
  unmodeled auxiliaries; kick-path external tables/inputs).

## Errata (M2 arms-and-wiring Task 12 / FU-139 §10)

* **§6.1 action rows `18`/`21`/`23` move from `not ported (partial)` to
  `ported`.** Task 12 ports the row-18/21/23 resolution arms (FU-139 §10):
  `fifa96_action_duel_search` (NSEARCH `0x8DB6C` + the `0xA1860` shell sort),
  `fifa96_action_duel_swap` (`0x786A0`), `fifa96_action_duel_bind`
  (`0x4C324`), the extended `fifa96_action_duel_step`/`_receive_step`/
  `_tackle_step`/`_tackle_attempt` and the engine handlers
  `fifa96_match_action_18/_21/_23`. Install arms: `0x8A32F` (18), `0x7D046`
  (21; the row-05 `0x7F791`/`0x7F7B6` sites too), `0x7D1B9` (23). The wiring
  gate (arm + full record-visible body + pool binding) passes for all three.
* **§7 totals update in place:** action ported 7 -> 10 (`18`, `21`, `23`),
  not ported 34 -> 31; dispatch 72 -> 69 `-UNSUPPORTED`, 7 -> 10 `FIFA96_OK`;
  `tests/test_engine_match_handlers.c` flips
  `action_expect[0x18]`/`[0x21]`/`[0x23]` to `FIFA96_OK` and adds
  `test_action_18_runs_body`/`test_action_21_runs_body`/
  `test_action_23_runs_body`.
* **FU-78 §5/§6 tackle-attempt camera gate erratum.** The second camera-x gate
  in `FUN_00082DD0` (`0x82E85..0x82E96`) returns only when `camera_x > pos_x`
  (`MOV EAX,[0x15774C]; CMP EAX,[ESI+0x59]; JLE continue`), not when
  `camera_x < pos_x`; the pre-Task-12 `fifa96_action_tackle_attempt` inverted
  it. Fixed and pinned by a discriminating fixture (FU-139 §10.5 erratum).
* **OL-27/OL-32 close** (FU-139 §10): the event ring/sink are ported
  (`fifa96_event_ring_append`/`_sink_store`), the duel/receive/tackle
  resolution arms are ported and tested. New numbered legs OL-67 (event-ring
  binding) and OL-68 (rows 18/21/23 unmodeled record/presentation inputs) are
  registered in FU-142 §6.

## Errata (M2 arms-and-wiring Task 13 / FU-139 §11)

* **§6.1 action row `06` moves from `not ported` to `ported`.** Task 13 ports
  the row-06 pursuit body (`0x801B4..0x809EF`, ~597 instructions) as
  `fifa96_action_pursuit_step` (FU-139 §11) and wires
  `fifa96_match_action_06` over the FU-141 pool. The loader consumes Task 3's
  material through the FU-141 entity view and the Task-11 fold: the
  `0x114E04` quadrant idiom (`0x80439`/`0x80472`/`0x80508`/`0x80571`/
  `0x805AC`) is shared through the new `fifa96_ball_fold` surface
  (`fifa96_ball_pairing.h`; the former static `kick_fold`).
* **§7 totals update in place:** action ported 10 -> 11 (`06`), not ported
  31 -> 30; dispatch 69 -> 68 `-UNSUPPORTED`, 10 -> 11 `FIFA96_OK`;
  `tests/test_engine_match_handlers.c` flips `action_expect[0x06]` to
  `FIFA96_OK` and adds `test_action_06_runs_body`.
* **OL-30 span erratum.** FU-142 §3's "row 06 `0x801B4..0x81067` (597 insns)"
  mixes two functions: the row-06 body RETs at `0x809EF`; `0x80A00..0x81065`
  is the row-09 handler (action table `0x1106E0[9]`, jump table `0x809F0`),
  which stays unwired (OL-9). The port bounds `0x801B4..0x809EF`.
* **OL-30 closes; OL-69 opens.** The new numbered leg covers row 06's
  unmodeled record/presentation inputs (record `+0x90/+0x99/+0x9D`, the
  descriptor bytes, `[[rec+0x28]]`, the `0x1577C0/0x1577C2` lead words, the
  row-05 `0x15872A/0x15872F` bytes, the `[0x157ABE]` mirror, the callback
  position stand-in and the opaque `0x6DA64` swap) and the `[0x157A4F]`
  parity producer (the run's `pass_parity`, first-hand `FUN_0004B100`
  `0x4B11A`/`0x4B129`); registered in FU-142 §6.

## Errata (M2 child Task 10 / FU-143)

* **§6.2 Table B / §7 phase-row pointer.** The Table B classification and the
  §7 phase totals above are the pre-Task-10 state. FU-143
  (`docs/ghidra/FU143_phase_rows.md`) derives the 35 FU-83 phase rows (handler
  table `0x110794`, class table `0x1106AD`) and the transitions
  `FUN_000740A0`/`FUN_000888FC`/`FUN_0008A938`/`FUN_0008B9CC`, porting the row
  metadata and drivers (`fifa96_action_phase_row/_act/_situation/_period_end`)
  at the loader level with open legs OL-72…OL-79; the engine phase dispatch and
  run-loop wiring are still pending, so the engine dispatch counts stay as
  recorded and Table B's "not ported / OL-13" cells are superseded by FU-143
  for the loader-level derivations.

## Errata (M2 playable-match Task 2 / FU-143 §11)

* **§6.1 action row `01` is `ported`** (row `0x7DBC0`;
  `fifa96_match_action_01`, FU-143 §11 / engine commit `e89e985`); §7 above was
  stale at the previous 13-row snapshot and is now updated in place: action
  ported 13 -> 14 (`01`), not ported 28 -> 27; dispatch 66 -> 65
  `-UNSUP` (61 not ported + the unwired `27`/`29`/`2C` + the dead `2B`),
  13 -> 14 `FIFA96_OK` (the list now includes `01`). Cross-checked against the
  engine handler table (`src/fifa96_engine/fifa96_match_handlers.c`:
  `{0x01, fifa96_match_action_01, ...}`) and `docs/ENGINE.md` (14/80 wired,
  65 UNSUP / 14 OK / 1 NOTF). The §8 OL-9 group-C list (`01`, `02`, ...) is
  superseded for `01`; the other group-C rows stay open.

## Errata (M2 phase-7 P3 / FU-151)

* **§6.1 action row `1D` is `ported`, and row `1E` is the full machine.**
  Row `0x1D` (`fifa96_match_action_1D`, FU-151 §Port contract item 2 /
  `fifa96_keeper_closedown_step`, stages 0..4) and row `0x1E`
  (`fifa96_match_action_1E` extended from the FU-140 linear claim to the
  full ten-stage `fifa96_keeper_claim_step`, FU-151 item 1). Table A's
  `1D` cell ("not ported (partial); keeper_clear_vector; FU-79 §6; FU-140
  §2") is superseded; the FU-140 OL-35 legs for the stage bodies are carried
  by FU-151 §5 (legs 3/4/12/13/15). The §7 totals were not updated by the
  engine landing (this erratum was appended alone); the final-review fix wave
  updated them in place: action ported 14 -> 15 (`1D`), not ported 27 -> 26;
  dispatch 64 `-UNSUP` (26 not ported action rows + 34 phase rows + the
  unwired `27`/`29`/`2C` + the dead `2B`), 15 `FIFA96_OK`, 1 `NOTF`; `docs/ENGINE.md` / `README.md` show
  `15/80` and `64 UNSUP / 15 OK / 1 NOTF`. Cross-checked against
  `src/fifa96_engine/fifa96_match_handlers.c` (`{0x1D, fifa96_match_action_1D,
  ...}`).
* **`+0x8E`/`+0x91` audit (FU-151 §3.5 erratum 1).** The dispatch code
  selected by the record machines is byte `+0x91` (the installer's write);
  byte `+0x8E` is the face octant. No wrong-field read in the dispatch
  landing; two unwired loader bodies were fixed by FU-151 P3
  (`fifa96_action_carrier_arm` 0x7F374 and the keeper-1F input gates).

## Errata (M2 phase-8 T1 / FU-149 §7)

* **§6.1 action rows `10`/`11`/`12`/`13` are `ported`.** The four set-piece
  taker machines landed as `fifa96_match_action_10..13` (FU-149 §7; first-hand
  windows/tables in §7.1) with the stage machines, the resolutions (situation
  0xB -> phase 2; row 0x10's situation-2 re-queue) and the FK/penalty
  tail distinction (rows 0x10/0x11 write `[0x157A6A] = 0x12C`; rows 0x12/0x13
  never touch the cell). Table A's four "not ported (partial)" cells are
  superseded; the staging/kick-vector/sink residue is FU-149 §7.3 legs
  L13.1..L13.7. §7 totals refreshed in place: action ported 15 -> 19, action
  not ported 26 -> 22, not ported 60 -> 56; dispatch 60 `-UNSUP` (22 not ported
  action rows + 34 phase rows + the unwired `27`/`29`/`2C` + the dead `2B`), 19
  `FIFA96_OK`, 1 `NOTF`; `docs/ENGINE.md` / `README.md` show `19/80` and
  `60 UNSUP / 19 OK / 1 NOTF`. Cross-checked against
  `src/fifa96_engine/fifa96_match_handlers.c`
  (`{0x10, fifa96_match_action_10, ...}` .. `{0x13, fifa96_match_action_13,
  ...}`) and the review fix (row-0x12 timer/relay-pick).

## Errata (M2 phase-9 T1 / FU-75 L4.6)

* **§6.1 action row `02` is `ported`.** M2 phase-9 T1 ported the `0x7DFCC`
  body as `fifa96_match_action_02` (first-hand `disassemble_bytes
  0x7DFCC..0x7E1A2` on /FIFA96.EXE; the FU-138 §2 row-02 summary is confirmed
  and extended): phase 1 mirrors `-dword[[team+0x7B2]+0x59]` into `+0x4D` with
  `+0x55 = 0`, `+0x89 = 0`, `+0x92 = 0` and returns; phase 2 writes
  `[team+0x7B2] = rec`, requests the `FUN_0007876C` merge when it has no slot
  and `byte[team+0x828] != 0`, installs `4` invoke-now when it has a slot
  (direct return), and for the no-slot arm writes the `0x15774C` camera triple
  target, `+0x89 += delta` and the stage-0 `lane > 0x40 ? 0x78 : 0xA` wait
  (the `lane > 0x40` ready path runs `FUN_0007DAB4` first); any other phase
  resets. The stage-1 (`0x7E0F6..0x7E184`: stack position copy, `+/-0x1E0`
  z offset, `0x8DE8C` nearest, `0x8DCD4` metric, `0x92820` event, `0x7A490`
  staging) and stage-2 (`0x7E185` `0x79B1C` snap + `+0x44` reset) arms remain
  FU-138 OL-18 (narrowed: the phase-1/phase-2 slot/no-slot cores plus the
  stage-0 wait are wired). §7 totals refreshed in place: action ported
  19 -> 20, action not ported 22 -> 21, not ported 56 -> 55; dispatch 59
  `-UNSUP`, 20 `FIFA96_OK`, 1 `NOTF`. Cross-checked against
  `src/fifa96_engine/fifa96_match_handlers.c` (`{0x02,
  fifa96_match_action_02, ...}`).

## Errata (M2 phase-9 T2 / FU-142 OL-63)

* **§6.1 action row `05` is `ported`.** M2 phase-9 T2 wired the already-ported
  `fifa96_action_carrier_arm` (`0x7F194..0x7F665`, FU-139 §8) as
  `fifa96_match_action_05` over `mr->record` and the FU-141 pool. The
  record-visible landing: the `0x7F1FF` `[0x158724]` claim (pool
  `ball.carrier`) + the `0x158728..0x15872F` block reset (`ball.pos_*`), the
  team-target bind, the capped `+0x89`, the camera triple copy, the stage-0
  control/dir gates (`0x15872A/B`), the `0x7876C` merge request, the `0x79B1C`
  snap, the `0x79C50` face and the stage-3 hand-off. The Task-10
  "row remains `not ported (partial)` with `fn == NULL`" statements in §7 are
  superseded by the T2 note; the residual is exactly the stage-0 target
  algebra `0x7F3A1..0x7F57B` and the `FUN_0007F7E0` fallback (OL-63
  narrowed). The possession release countdown gains its `FUN_0004B100`
  `0x4B163..0x4B17B` decay in `fifa96_match_entities_update` and the row-06
  pursuit staging reads the live `0x15872A`/`0x15872F` bytes (the OL-69
  producer half closed). §7 totals refreshed in place: action ported 20 -> 21,
  action not ported 21 -> 20, not ported 55 -> 54; dispatch 58 `-UNSUP`,
  21 `FIFA96_OK`, 1 `NOTF`. Goldens: M1/M2 byte-identical (`cmp` clean; the
  tape never stages code `5`, `M2_WIRED_MASK` unchanged). Cross-checked
  against `src/fifa96_engine/fifa96_match_handlers.c` (`{0x05,
  fifa96_match_action_05, ...}`).
