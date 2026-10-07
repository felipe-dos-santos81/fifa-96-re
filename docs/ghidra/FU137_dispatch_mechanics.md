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
of the 80 dispatch rows **0 are wired, 3 are unwired (2 action + the phase
zero slot), 73 are not ported, and 4 (0x27/0x29/0x2B/0x2C) are open legs** —
`fifa96_match_handlers.c` returns `-FIFA96_ERR_UNSUPPORTED` for every classified
row and `-FIFA96_ERR_NOT_FOUND` for the phase zero slot, never a silent no-op
(all error results are negated per the engine family convention).

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
program** — they are open legs (OL-15), not schedulable ports. (`0x27`'s only
match-code constant is an animation-row argument, a distinct meaning of the
same byte, so it is **not** evidence of an action install.)

## 6. Per-row classification (all 80 rows)

Rubric (refines FU-136 §1.3 by splitting the unresolved entry paths):

| class | dispatch result | definition |
|---|---|---|
| `ported` | `FIFA96_OK` | row body in C **and** wired into `fifa96_match_action_table`/`_phase_table` (`fn != NULL`). |
| `unwired` | `-FIFA96_ERR_UNSUPPORTED` | body fully covered by tested C symbols, no record/entity binding yet (open leg OL-1). Phase `0x16` is the zero/INT3 slot: dispatch-only, returns `-FIFA96_ERR_NOT_FOUND`. |
| `not ported` | `-FIFA96_ERR_UNSUPPORTED` | a needed part of the body has no C function; the row names its port group/open leg. |
| `open leg` | `-FIFA96_ERR_UNSUPPORTED` | FU-137 could not bound a static entry for the row (no install arm), so it is not schedulable as a port task yet; numbered OL. |

### 6.1 Table A — action rows `0x1106E0[code]`

| code | native | class | handler / evidence | open leg |
|---|---|---|---|---|
| 00 | 0x07DB10 | unwired | `fifa96_action_move_step`/`_move_target`; `test_action_handlers`; FU-76 §3.1/§4 | OL-1 |
| 01 | 0x07DBC0 | not ported (partial) | sequence_select/stage helpers; FU-76 §2, FU-82 §3.1 | OL-9 |
| 02 | 0x07DFCC | not ported (partial) | locomotion_restart_target; FU-77 §2.2 | OL-9 |
| 03 | 0x07E1A4 | not ported (partial) | locomotion_hold/clamp_placement; FU-77 §2.3 | OL-8 |
| 04 | 0x07E7C8 | not ported (partial) | locomotion_camera_lead; FU-77 §2.4 | OL-8 |
| 05 | 0x07F194 | not ported (partial) | possession_reset/claim/timer; FU-78 §2/§3 | OL-8 |
| 06 | 0x0801B4 | not ported | FU-77 §2.6 (597 insns), no port row | OL-8 |
| 07 | 0x0814B0 | not ported (partial) | kick_angle/kick_apply; FU-76 §3.2, FU-77 §2.7 | OL-8 |
| 08 | 0x081068 | not ported (partial) | FU-75 §1.6 chase-gate installer only | OL-8 |
| 09 | 0x080A00 | not ported (partial) | FU-81 arm table 0x809F0; stage helpers | OL-9 |
| 0A | 0x081738 | not ported | FU-76 §2; installer 0x7CDD8 has no xrefs | OL-14 |
| 0B | 0x081908 | not ported (partial) | sequence_duel_event; FU-82 §3.3 | OL-9 |
| 0C | 0x081C90 | not ported (partial) | FU-81 7-arm table 0x81C74 | OL-9 |
| 0D | 0x08251C | not ported (partial) | FU-82 §3.4 4-arm table 0x8250C | OL-9 |
| 0E | 0x082710 | not ported | FU-81 §2.1 gate/head; tackle helpers only install 0x0E | OL-9 |
| 0F | 0x082AD0 | not ported | FU-76 §2; KICK 0x7B9C4 body | OL-8 |
| 10 | 0x0855F0 | not ported (partial) | FU-81 7-arm table 0x855B8 | OL-9 |
| 11 | 0x085DE4 | not ported (partial) | FU-81 10-arm table 0x85DA0 | OL-9 |
| 12 | 0x083D68 | not ported (partial) | FU-81 tables 0x83D2C/0x83D4C | OL-9 |
| 13 | 0x084B00 | not ported (partial) | FU-81 7-arm table 0x84AE4 | OL-9 |
| 14 | 0x08784C | not ported (partial) | scatter_celebration; FU-82 §3.5 | OL-9 |
| 15 | 0x087CD0 | not ported | FU-81 §2.1 head mis-decoded; FU-82 §1 stub bucket | OL-14 |
| 16 | 0x084630 | not ported (partial) | sequence_marker/rng_event; FU-82 §3.6 | OL-9 |
| 17 | 0x084730 | not ported (partial) | FU-81 4-arm table 0x84720 | OL-9 |
| 18 | 0x0849B0 | not ported (partial) | duel_step/duel_split; FU-78 §7 | OL-11 |
| 19 | 0x0746E4 | not ported (partial) | keeper_hold_*; FU-79 §2 | OL-10 |
| 1A | 0x07662C | not ported (partial) | keeper_reposition_a_gate; FU-79 §3 | OL-10 |
| 1B | 0x076D28 | not ported (partial) | keeper_reposition_b_finish; FU-79 §4 | OL-10 |
| 1C | 0x077728 | not ported (partial) | keeper_lunge_track; FU-79 §5 | OL-10 |
| 1D | 0x074EB0 | not ported (partial) | keeper_clear_vector; FU-79 §6 | OL-10 |
| 1E | 0x07550C | unwired | `fifa96_keeper_claim_place`; `test_keeper_bodies`; FU-79 §7/§11 | OL-1 |
| 1F | 0x076380 | not ported (partial) | keeper_dive_target/arm_step; FU-79 §8 | OL-10 |
| 20 | 0x084EEC | not ported (partial) | FU-82 §3.7 7-arm table 0x84ED0 | OL-9 |
| 21 | 0x085214 | not ported (partial) | action_receive_step; FU-78 §4 | OL-11 |
| 22 | 0x08539C | not ported (partial) | sequence_press_event; FU-82 §3.8 | OL-9 |
| 23 | 0x082F84 | not ported (partial) | tackle_step/tackle_attempt; FU-78 §6 | OL-11 |
| 24 | 0x086510 | not ported (partial) | sequence_lane/anim_byte; FU-82 §3.9 | OL-9 |
| 25 | 0x0880CC | not ported (partial) | FU-82 §3.10 7-arm table 0x880B0 | OL-9 |
| 26 | 0x0866F4 | not ported | arm 0x8D74D (§5.2); body unanalyzed (FU-76 §2 METRIC only) | OL-15 |
| 27 | 0x086820 | **open leg** | no install arm (§5.3); FU-76 §2 listing cut | OL-15 |
| 28 | 0x0870E8 | not ported | arm 0x8D7CF (§5.2); body unanalyzed (FU-76 §2) | OL-15 |
| 29 | 0x0874E4 | **open leg** | no install arm and no match-code reference (§5.3) | OL-15 |
| 2A | 0x086A34 | not ported | arm 0x8D807 record scan (§5.2); body unported (FU-76 §2/§7 leg 7) | OL-15 |
| 2B | 0x087738 | **open leg** | one-byte RET 0x87738; no install arm (§5.3); dead if confirmed | OL-15 |
| 2C | 0x084598 | **open leg** | prologue-only body; no install arm (§5.3); body unanalyzed | OL-15 |

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

| surface | rows | ported | unwired | not ported | open leg |
|---|---|---|---|---|---|
| action `0x1106E0` | 45 | 0 | 2 (`00`, `1E`) | 39 | 4 (`27`, `29`, `2B`, `2C`) |
| phase `0x110794` | 35 | 0 | 1 (`16`, zero slot -> `-NOT_FOUND`) | 34 | 0 |
| **dispatch total** | **80** | **0** | **3** | **73** | **4** |

Dispatch results at this commit: **79 × `-FIFA96_ERR_UNSUPPORTED`** (the 73 not
ported rows + the 2 unwired action rows + the 4 open legs) and **1 ×
`-FIFA96_ERR_NOT_FOUND`** (phase `0x16`); out-of-range -> `-NOT_FOUND`; NULL
`mr` -> `-INVALID`. All error results are negated, matching the engine family
convention (`fifa96_match_run_*`).

## 8. Open legs

1. **OL-1 — entity/record model and machine binding.** The 0xB2-stride record,
   its `[rec+0x18]` handler slot, and the record fields the machines stage
   (`+0x81`, `+0x89`, `+0x91`, `+0x92`, `+0x9A`, `+0x9E`, `+0x9F`) have no
   engine surface, so action 00/1E's tested bodies cannot bind to a match run.
   Blocks wiring those two rows.
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
10. **OL-10 — keeper bodies** (19–1D, 1F) + `FUN_00076130` input handler.
11. **OL-11 — duel/tackle/receive bodies** (18, 21, 23) + NSEARCH/SWAP.
12. **OL-12 — entity frame chain** `FUN_0004B100`, control slots, camera/track.
13. **OL-13 — phase bodies** (34 rows) + phase entry wrappers.
14. **OL-14 — residual/unclassified** (0A, 15) + dead-code classification.
15. **OL-15 — installer arms `0x26`–`0x2C`.** Arms resolved for 26/28/2A
    (§5.2); bodies 0x0866F4/0x0870E8 unanalyzed, 0x086A34 unported. No static
    install arm found for 27/29/2B/2C anywhere in the program; 0x2B's body is a
    one-byte RET and 0x27's only constant is an animation-row argument. These
    four rows stay unscheduled until a reachability/dynamic pass (or the FU-142
    probe for plan C9) resolves their entry.

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
