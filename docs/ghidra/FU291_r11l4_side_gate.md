# FU291 — the per-side table gate + side resolution (`FUN_000741B4`, `[0x14C1D4]`, `[0x157ABE]`, the `[0x157AD0]`/`[0x157ACC]` counters)

**Phase/Provenance:** M2 phase-27 recon-ahead, Task 2 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (case-sensitive; confirmed this session by
`ghidra_get_current_program_info`: `watcom:LE:32:default`, image base 0, `.image`
overlay, 2706 functions). Read-only: no rename/comment/label/create/script/save;
`/FIFA96.EXE` untouched; ISO untouched; no git. Every native address/byte below is
a **fresh** `disassemble_bytes` / `disassemble_function` / `decompile_function` /
`read_memory` / `get_xrefs_to` of *this* session unless marked *carried* (FU283
§head-gates / FU282 §1.1, named); engine-source facts cite the built tree by line.
Alias traps applied (`dword[x]>>0x18` = byte `x+3`; the `MOV AX,word[…]` word /
`AND AH,2` bit split is respected below). Prior docs are not rewritten:
FU283-L1 is **advanced**, not edited (see legs).

## Port shape

The task is the **side-resolution + per-side gate** pair shared by the keeper-CPU
ladder head (`0x77ECB..0x77EF0`, FU283 §Step 1) and the record-0 machine head
(`0x78301..0x7832E`, FU282 §1.1). Two native primitives:

1. **Side fold `FUN_000741B4` (`0x741B4`, 7 insns)** — a byte in, 0/1 out:
   `return (param_1 ^ byte[0x157ABE]) & 1`. `byte[0x157ABE]` is the **side-swap
   cell**; the fold gives the *swap-adjusted* side index `idx ∈ {0,1}`.
2. **Per-side word table `[0x14C1D4]`** — a **two-entry 16-bit** table
   (`idx0` = word at `0x14C1D4`, `idx1` = word at `0x14C1D6`), indexed
   `word[FUN_000741B4(side)*2 + 0x14C1D4]`. Its **bit 9** (bit 1 of the high
   byte) is the ladder's `AH&2` gate; other bits drive camera reflect/interp/pan.

The ladder head is therefore: `idx = (byte[[rec]+0x826] ^ [0x157ABE]) & 1;
if ((word[idx*2+0x14C1D4] >> 9) & 1) return;` — a per-side option flag that
**closes** the ladder for a side whose bit is set (return to the tail `0x782C4`
with no install). The record-0 machine head uses the *same* fold twice to bump two
**per-side statistics counters**: `[0x157AD0][idx]++` (actor side) and
`[0x157ACC][idx^1]++` (complement side).

**Engine status (the surprise):** both primitives are **already ported**, but only
the *side fold* is live; the *gate value* is a caller-supplied field with **no
glue**, and the *side-swap producer* + the *counter consumer* are unported. See
§Engine mapping and GO/NO-GO.

## The gate (`0x77ECB..0x77EF0`, fresh bytes)

```
0x77ECB  MOV EAX,[EBP]            ; rec[0]
0x77ECE  MOV AL,[EAX+0x826]       ; the side byte
0x77ED4  AND EAX,0xFF             ; side ∈ {0,1}
0x77ED9  CALL 0x000741B4          ; EAX = (side ^ [0x157ABE]) & 1
0x77EDE  MOV AX,word[EAX*2+0x14C1D4]
0x77EE6  XOR AL,AL                ; EAX = word & 0xFF00
0x77EE8  AND AH,0x2               ; keep bit 1 of the HIGH byte (word bit 9)
0x77EEB  AND EAX,0xFFFF
0x77EF0  JNZ 0x782C4              ; bit set -> gate closed (no install)
```

So the tested flag is **bit 9 of the selected word** — for `idx0` that is
`byte[0x14C1D5] & 2`, for `idx1` `byte[0x14C1D7] & 2`. The trace matches the
keeper_cpu port comment `[0x14C1D4][FUN_000741B4(side)] >> 8 & 2`.

## `FUN_000741B4` — the side fold (fresh `decompile_function` + `disassemble_function`)

```
PUSH EDX ; XOR EDX,EDX ; MOV DL,byte[0x157ABE] ; XOR EAX,EDX ; AND EAX,1 ; POP EDX ; RET
```

Decompile: `return (param_1 ^ byte[0x157ABE]) & 1;`. It is the **swap-adjusted
side index** used by the ladder head, both counter increments, and the other
indexed-table readers.

## `[0x157ABE]` — the side-swap cell: writers / readers (fresh `get_xrefs_to`)

* **Writers.** `FUN_0007417c` at `0x74189` and a **twin** at `0x742b4` (outside any
  analysed function) both compute `byte[0x157ABE] = (byte[0x157AC2] ^
  byte[0x157AC3]) & 1`, then `byte[0x157ABF] = that ^ 1` (the complement). Fresh
  bytes: `0x74189 = a2be7a1500` (`mov [0x157abe],al`), `0x742b4 = a2be7a1500`.
* **Readers** (7): `FUN_00036c70` `0x36d02`, `FUN_0004ba44` `0x4bab5`,
  `FUN_0007412c` `0x74130`, `FUN_00078824` `0x788b0`, **`FUN_000741b4` `0x741b7`**,
  `FUN_0008b9cc` `0x8ba75`, `FUN_0003705c` `0x370bf`.

## `[0x14C1D4]` — the per-side word table: writers / readers (fresh `get_xrefs_to`)

* **Writers** (3): `FUN_0001c9bc` `0x1c9d4/0x1c9e3` — the **option-pack builder**
  (also ORs the gate bits: `0x1ca15 OR byte[0x14C1D5],2` for `[0x105288]==1`,
  `0x1ca25 OR byte[0x14C1D7],2` for `[0x10529C]==1`); `FUN_00024b00` `0x24b98` —
  a **reset** that zeroes both words (fresh `0x24b8f XOR EBX,EAX` with
  `EBX=EAX` ⇒ 0, then `MOV word[0x14C1D6],DX` / `MOV word[0x14C1D4],BX`, `DX=BX=0`).
* **Direct readers** (5): `FUN_0004b02c` `0x4b0bc`, `FUN_000736ac` `0x73a07`,
  `FUN_000709d0` `0x70ab1` (the camera pan gate), `FUN_0007131c` `0x718d0`,
  `0x742f5`.
* **Indexed-word readers** (6): the ladder `FUN_00077eac` `0x77ede`, plus
  `FUN_0008c758` `0x8c7a6`, `FUN_0007a490` `0x7a6a9`, `FUN_0007b878` `0x7b8f9`,
  `FUN_0007b194` `0x7b41f`, `FUN_0007b9c4` `0x7bd19`.
* **Static content:** fresh `read_memory 0x14C1D4` (32 B) = all zero — the table
  is **RAM**, populated at match init by the two writers above.

## `[0x157AD0]` / `[0x157ACC]` — the per-side statistics counters (fresh `get_xrefs_to`)

* **Both** are written (zeroed) only by the shared match-reset `FUN_00073ee0`
  (`0x73f23 MOV word[0x157ad0]=CX=0`, `0x73f95 MOV word[0x157acc]=BX=0`), which
  zeroes a large `0x157A..` block; `FUN_00073ee0` is called by `FUN_00074034`
  `0x7407f`, `FUN_0004b020` `0x4b025`, and `0x7428e`.
* `[0x157AD0]`: writer `0x73f23`; reader `FUN_00042c88` `0x431dd`; DATA refs
  `0x78311` (the machine `INC`), `0x773d4`, `0x77bee`.
* `[0x157ACC]`: writer `0x73f95`; reader `FUN_00042c88` `0x431ac`; DATA refs
  `0x7832e` (the machine `INC`), `0x724fa` (`FUN_00072478`), `0x773b9`, `0x77bd3`.
* The sole reader **`FUN_00042c88`** indexes `word[0x157AD0+i*2]` /
  `word[0x157ACC+i*2]` (`EDI ∈ {0,2}`, loop `i ∈ {0,1}`) into a percentage form
  (`0x43108 LEA/SHL/IDIV` → `0x64 - pct`) and `sprintf`s it — it is the **match
  statistics / percentage screen**, not gameplay logic.

## Engine mapping (read-only source)

| native | engine cell | status |
|---|---|---|
| `FUN_000741B4` = `(side ^ [0x157ABE]) & 1` | `ref_side_index` (`fifa96_loader/fifa96_referee.c:21-23`); inline `(side ^ mr->side_swap) & 1` at `fifa96_match_run.c:2907/3087/6228` | **PORTED + LIVE** |
| `[0x157ABE]` | `mr->side_swap`; `state->side_swap` (referee.h:103, "staged [0x157ABE]"); `cpu->side_xor` (keeper_cpu.h:78) | **field ported, PRODUCER unported** (only cleared: `match_run.c:2080/2214`) |
| `[0x14C1D4]` two-entry word table | `fifa96_input_range_words` (`fifa96_input.c:86`) → `mr->input_range` (`match_run.c:484/2271`), incl. the `0x200` gate bit (`input.c:97-98`) | **PORTED** |
| the `AH&2` gate value | `fifa96_keeper_cpu_record.prepass_ah2` (`keeper_cpu.h:32`), read `keeper_cpu.c:100` | **field ported, GLUE absent** (set only in `test_keeper_cpu.c:379`) |
| `[0x157AD0]` / `[0x157ACC]` | `cpu->counter_ad0[2]` / `counter_acc[2]` (`keeper_cpu.h:79-80`; `keeper_cpu.c:40-41`, `idx=(side^side_xor)&1`) | **PORTED**; machine test-only |
| reader `FUN_00042c88` (stats screen) | — | **UNPORTED** |

**Do not conflate:** the engine's `mr->corner_count` is a **different** native
cell (`0x8ABFF/0x8AC0F`, `match_run.c:2907`), not `[0x157AD0]`/`[0x157ACC]`,
even though it shares the `(side ^ side_swap) & 1` fold.

## GO/NO-GO

**GO to wire the gate's *value*; NO-GO as a behaviour slice — unchanged cluster
NO-GO.**

*The side resolution is fully ported and live.* `(side ^ side_swap) & 1` is
executed today (corner counter, referee foul/offside, etc.), and the table itself
is rebuilt live as `mr->input_range`. Therefore the gate's bit
`(input_range[(side ^ side_swap) & 1] >> 9) & 1` is **computable from current
engine state** — no unported producer blocks it.

*But the gate has no live consumer.* Its only consumer is the keeper-CPU ladder
seam (`fifa96_keeper_cpu_record.prepass_ah2`), and (a) that field is **never fed
from `input_range`** — it is set only by tests — and (b) `fifa96_keeper_cpu_step`
/ `fifa96_keeper_cpu_ladder_run` are **not called from the live engine**
(test-only module; `fifa96_match_entities.c:702` names the skeleton unported). So
wiring `prepass_ah2` is a **bounded, zero-observable-delta glue** (the gate stays
dormant until the machine itself is wired), and it does **not** raise behaviour.

*The two producers around it stay open:* `side_swap`'s native producer
(`FUN_0007417c` / the `0x742b4` twin, `(byte[0x157AC2] ^ byte[0x157AC3]) & 1`) is
unported (staged zero), and the counters' only reader (`FUN_00042c88`, the stats
screen) is unported. Neither is an external blocker, but neither is live. This
leaves FU273's phase-scale **NO-GO** for the live keeper-CPU release untouched.

**Tooling:** none. All reads fresh this session; program stayed `/FIFA96.EXE`.

## Evidence table (all fresh, `/FIFA96.EXE`, read-only)

| claim | fresh evidence |
|---|---|
| `FUN_000741B4` = `(param_1 ^ byte[0x157ABE]) & 1`, 7 insns | `decompile_function 0x741B4`; `disassemble_function 0x741B4` |
| gate: `side←byte[[rec]+0x826]`; `CALL 0x741B4`; `MOV AX,word[EAX*2+0x14C1D4]`; `XOR AL,AL`/`AND AH,2`/`AND EAX,0xFFFF`/`JNZ 0x782C4` | `disassemble_bytes 0x77ECB` (48 B) |
| `[0x157ABE]` writers = `FUN_0007417c` (`0x74189`) + twin (`0x742b4`), both `(byte[0x157AC2]^byte[0x157AC3])&1` (+ `[0x157ABF]` complement) | `get_xrefs_to 0x157ABE`; `disassemble_bytes 0x742A0` |
| `[0x14C1D4]` writers = `FUN_0001c9bc` (`0x1c9d4/0x1c9e3`, option-pack incl. `[0x14C1D5]|=2`/`[0x14C1D7]|=2`) + `FUN_00024b00` (`0x24b98`, zeroes both words) | `get_xrefs_to 0x14C1D4`; `disassemble_function 0x1C9BC`; `disassemble_bytes 0x24B80` |
| `[0x14C1D4]` readers: direct `FUN_0004b02c`/`FUN_000736ac`/`FUN_000709d0`/`FUN_0007131c` + indexed `FUN_00077eac`/`FUN_0008c758`/`FUN_0007a490`/`FUN_0007b878`/`FUN_0007b194`/`FUN_0007b9c4` | `get_xrefs_to 0x14C1D4` |
| table static bytes all-zero (RAM) | `read_memory 0x14C1D4` (32 B) |
| `[0x157AD0]`/`[0x157ACC]` zeroed by `FUN_00073ee0` (`0x73f23`/`0x73f95`), reset called from `0x7407f`/`0x4b025`/`0x7428e` | `get_xrefs_to 0x157AD0`; `get_xrefs_to 0x157ACC`; `get_xrefs_to 0x73EE0`; `disassemble_function 0x73EE0` |
| both counters read (as `word[+i*2]`) only by `FUN_00042c88` (`0x431dd`/`0x431ac`), the pct screen | `get_xrefs_to 0x157AD0`/`0x157ACC`; `disassemble_function 0x42C88`; `get_xrefs_to 0x42C88` |
| engine `side_swap` + `ref_side_index` port the fold; `input_range`/`input_range_words` port the table incl. `0x200` | `fifa96_referee.c:21-23`; `fifa96_match_run.c:2907/3087/6228`, `:484/2271`, `:2080/2214`; `fifa96_input.c:86-98` |
| `prepass_ah2` = caller field, read `keeper_cpu.c:100`, no engine glue | `fifa96_keeper_cpu.h:32`; `fifa96_keeper_cpu.c:100`; `test_keeper_cpu.c:379`; grep `prepass_ah2` |
| `fifa96_keeper_cpu_step`/`ladder_run` unused outside tests | grep `fifa96_keeper_cpu_step`; `fifa96_match_entities.c:702` |

**Carried (read, not rewritten):** FU283 §Step 1 (head gates) / FU282 §1.1 (record-0
head); FU283-L1 (now advanced by this draft).

## Numbered open legs (with owners)

* **FU291-L1 — `[0x157ABE]` producer unported.** Native `FUN_0007417c`
  (`0x74189`; twin `0x742b4`) sets `(byte[0x157AC2] ^ byte[0x157AC3]) & 1`; the
  engine only clears `side_swap`. Owner: FU-150 / FU291 follow-up.
* **FU291-L2 — the gate glue.** `prepass_ah2` is never computed from
  `input_range`+`side_swap`; `fifa96_keeper_cpu_record.prepass_ah2` is test-only,
  and the keeper-CPU module is not wired into the live engine. Owner: FU282/FU283
  follow-up (the machine's live driver is the carried FU282-L1/FU283-L8
  `FUN_000782D0` gate).
* **FU291-L3 — the counter consumer `FUN_00042c88` (`0x431dd`/`0x431ac`)** — the
  per-side statistics/percentage screen; the only reader of `[0x157AD0]`/
  `[0x157ACC]`; unported. Owner: stats/UI (FU-?).
* **FU291-L4 — the side-swap sources `byte[0x157AC2]`/`byte[0x157AC3]`.** Many
  writers (`FUN_0004b02c` `0x4b0de`, `FUN_0004ba00` `0x4ba20/0x4ba31`,
  `FUN_00038630` `0x38990`, the reset `FUN_00073ee0`); the engine names
  `global_157ac2` the period index (FU-213). Whether the native side-swap source
  is that same byte is **unproven**. Owner: FU-213 / FU291 follow-up.
* **FU291-L5 — the table reset `FUN_00024b00` (`0x24b98`).** Zeroes both words
  independently of the ported option-pack builder; relationship to
  `fifa96_input_range_words` unclassified. Owner: FU-148.
