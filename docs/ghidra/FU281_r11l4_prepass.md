# FU281 — R11-L4 driver pre-pass `FUN_0008D8EC`: byte-exact port shape

**Phase/Provenance:** M2 phase-26c recon-ahead, Task 1 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; case-sensitive), confirmed this
session by `get_current_program_info`: `watcom:LE:32:default`, image base 0,
`.image` overlay present, **2706** functions. Read-only session — no
rename/comment/label/function-create/script/save; `/FIFA96.EXE` and the ISO
untouched; no git command. Every native byte below is a **fresh**
`disassemble_function` / `disassemble_bytes` / `decompile_function` /
`read_memory` / `get_xrefs_to` / `search_instructions` of *this* session unless
marked *carried* (the frozen `docs/ghidra/FU273_r11l4_keeper_cpu_chain.md` §1.1/§1.2
is the source, read-only). Alias traps applied (`dword[x]>>0x10` = the word at
`x+2`, `>>0x18` = the byte at `x+3`); every CALL rel32 verified from raw bytes.

---

## 1. Port shape

`FUN_0008D8EC` (fresh `disassemble_function`: entry `0x8D8EC`, **179** insns,
body `0x8D8EC..0x8DB6A`; fresh `get_function_by_address` = `undefined
FUN_0008d8ec(int *param_1, uint param_2)`) is a **per-team-base, once-per-frame
pre-pass** over the 11-record block (`EBP := param_1` = the team's record-0 base;
record *N* = `EBP + N*0xB2`). Fresh `get_xrefs_to 0x8D8EC` = **4**:
`0x4B2AB`/`0x4B2B2` in `FUN_0004B100` (both team bases) and `0x74429`/`0x74430`
(§3, no analysed function). Shape, in order:

**(a) per-team-base frame counter (`0x8D8F7`).**
`MOV AH,byte[EAX+0x82C]` → `INC AH` → `MOV byte[EBP+0x82C],AH` → `XOR EAX,EAX;
MOV AL,byte[EBP+0x82C]; CMP EAX,0xB; JL 0x8D919; MOV byte[EBP+0x82C],0x0`.
The counter is team-level byte **`[EBP+0x82C]`** (it sits past the 11×0xB2 =
`0x7A6`-byte record array, so the "per-record" label is a misnomer — it is one
counter per *team base* per call). **Cap = 11 (`0xB`):** increment then wrap to 0
iff `>= 0xB`, i.e. the field cycles `0..10`.

**(b) sector-template select (`0x8D948..0x8D9B7`).** Reads `AX := word[0x1577FA]`
(compare cell = `0x1577F8`'s high word), then:
* `AX < word[0x157800]` (`0x8D94E JGE`) → copy 12 B from **`0x157788`** (three
  `MOVSD`) into the `ESP` query triple;
* else `AX < word[0x157806]` (`0x8D963 JGE`) → copy 12 B from **`0x157794`**;
* else copy 12 B from **`0x157770`**, then add a grid offset to the copy:
  `dest[0] += (dword[0x1577BE]>>16)<<5` and `dest[2] += (dword[0x1577C0]>>16)<<5`
  (`0x8D987 SAR`/`0x8D98D SHL 0x5`; scale ×32).

**(c) nearest-record search #1 (`0x8D9B2 CALL 0x8DE8C`)** — query = the copied
sector anchor (`EAX=ESP`), records = `EDX=EBP`, exclude-index `ECX=0`, out `EBX=0`;
result stored to **`[EBP+0x7B2]`** (`0x8D9B7`). Gate for this whole block
(`0x8D929 CMP EAX,2 JNZ` + `0x8D932`/`0x8D93B`): `phase == 2 AND
([EBP+0x7B2] == 0 OR [0x157A83] == 0)`.

**(d) per-side table read (`0x8DA4D ADD EAX,0x10F37C`).** `EAX := side*0xC` where
`side = byte[EBP+0x826]` (`0x8DA36 LEA EAX,[EDX*4]; SUB EAX,EDX; SHL EAX,0x2`;
`0x8DA1E MOV DL,byte[EBP+0x826]`), so the **only** table read in this function is
`0x10F37C + side*0xC` (stride `0xC`, two entries). Fresh `search_instructions
operand_pattern=0x10f37c` = **1** hit program-wide (`0x8DA4D`). **`0x10F334` is
NOT read in this function** (see leg FU281-L1).

**(e) nearest-record search #2 (`0x8DA52 CALL 0x8DE8C`)** — query = the
`0x10F37C` entry, records = `EBP`, exclude value = `byte[[EBP+0x7B2]+0x8D]`
(`0x8DA45 SAR EBX,0x18`), out = `ECX=[ESP+0x14]`; result stored to **`[EBP+0x7BA]`**
(`0x8DA57`). Then `FUN_0008D824(actor, [EBP+0x7BA], [EBP+0x7BA]+0x4D)` when
`[[EBP+0x7BA]+0x20] == 0`, else `[EBP+0x7BA] := 0`; `FUN_000795B4([+0x59],
[+0x4D], out)`; and the visibility flag **`[EBP+0x7BE] := 1`** (`0x8DAE0`) when the
out metric `< 0xF0` and `[[+0x69]>>16 <= 0x1E0` and `|[[+0x61]]| > |[0x157754]| + 0x90`.
Gate (`0x8D9CC..0x8DA26`): `phase == 2 AND actor != 0 AND byte[EBP+0x826] ==
byte[[actor]+0x826] AND |word[0x157754]| > 0x480 AND sign(word[0x157754]) ==
byte[EBP+0x826]`.

**(f) countdown block (`0x8DAF3..0x8DB2E`).** If `[EBP+0x7CB] != 0`: while
`word[0x157A64] < [EBP+0x81E]>>16`, `[EBP+0x820] -= word[0x157A64]`, else
`[EBP+0x820] := 0` and `[EBP+0x7CB] := 0`.

**(g) record-0 dispatch + the 0xB2-stride loop.**
```
0x8DB2E  MOV EAX,EBP
0x8DB30  MOV EBX,0x1
0x8DB35  CALL 0x000782d0            ; e8 96 a7 fe ff -> (-0x1586A) 0x782D0
0x8DB3A  LEA EDX,[EBP+0xb2]         ; record 1
0x8DB42  CMP byte[EDX+0x9a],0 ; JNZ 0x8DB52   ; +0x9A skip
0x8DB4B  MOV EAX,EDX ; CALL 0x0007ca54        ; e8 02 ef fe ff (-0x110FE) 0x7CA54
0x8DB52  INC EBX ; ADD EDX,0xb2
0x8DB59  MOVSX EAX,BX ; CMP EAX,0xb ; JL 0x8DB42   ; EBX in {1..10}
```
Record 0 is driven by `FUN_000782D0` (the record-0 machine, *carried* FU273 §1.3);
records 1..10 (stride `0xB2`) by `FUN_0007CA54`, skipping any record with
`byte[rec+0x9A] != 0`. Fresh `decompile_function 0x7CA54`: it decays the record
timers `[+0x81]`/`[+0x93]` by `word[0x157A62]>>8`, runs the `[+0x20]`-gated
steer/action block, then `CALL dword[rec+0x18]` (the record's own action
dispatch) + `FUN_0006E8E8` + `FUN_0007BF20` — i.e. the native per-record update.
`FUN_0008DE8C` (fresh `decompile_function`) scans the 11 records, skipping
`index == param_3` and any record with `byte[+0x9A] != 0 || byte[+0x98] != 0`,
minimizing `FUN_0008DC68(query.x − rec[0x59], query.z − rec[0x61])`; it returns
the nearest record pointer and writes the min metric to `*param_4`.

---

## 2. Evidence table (claim | fresh evidence)

| claim | fresh evidence |
|---|---|
| program `/FIFA96.EXE`, `watcom:LE:32:default`, 2706 fns | `get_current_program_info` |
| `FUN_0008D8EC` = `0x8D8EC..0x8DB6A`, 179 insns | `disassemble_function 0x8D8EC`; `get_function_by_address 0x8D8EC` |
| callers = 4 (`0x4B2AB`/`0x4B2B2` `FUN_0004B100`; `0x74429`/`0x74430`) | `get_xrefs_to 0x8D8EC` |
| counter `MOV AH,[EAX+0x82C]; INC; CMP EAX,0xB; wrap→0` | `disassemble_function 0x8D8EC` (`0x8D8F7..0x8D912`) |
| template select cells `word[0x1577FA]` vs `word[0x157800]`/`word[0x157806]`; templates `0x157788`/`0x157794`/`0x157770` | `disassemble_function` (`0x8D948..0x8D9A6`); `read_memory 0x8D948` = `66 a1 fa 77 15 00` |
| search #1 `0x8D9B2 CALL 0x8DE8C` → `[EBP+0x7B2]` | `read_memory 0x8D9B2` = `e8 d5 04 00 00` → `0x8DE8C` |
| search #2 `0x8DA52 CALL 0x8DE8C` → `[EBP+0x7BA]` | `read_memory 0x8DA52` = `e8 35 04 00 00` → `0x8DE8C` |
| `FUN_0008DE8C` = nearest eligible record by `FUN_0008DC68`, skipping `+0x9A`/`+0x98`/exclude | `decompile_function 0x8DE8C`; `decompile_function 0x8DC68` |
| `0x10F37C + side*0xC` is the sole table read in the fn | `read_memory 0x8DA30`; `search_instructions 0x10f37c` = 1 (`0x8DA4D`) |
| `0x10F334` not referenced by the fn | `search_instructions 0x10f334` (20 hits, all outside `FUN_0008D8EC`) |
| `FUN_0008D824` called at `0x8DA7A` | `read_memory 0x8DA7A` = `e8 a5 fd ff ff` → `0x8D824` |
| record-0 call `0x8DB35 CALL 0x782D0`, bytes `e8 96 a7 fe ff` | `read_memory 0x8DB2E` (16 B) |
| loop `0x8DB3A..0x8DB59`, `+0x9A` skip, `CALL 0x7CA54` | `disassemble_function` (`0x8DB3A..0x8DB5F`); `read_memory 0x8DB4D` = `e8 02 ef fe ff` → `0x7CA54` |
| `FUN_0007CA54` = per-record timer decay + `CALL dword[rec+0x18]` | `decompile_function 0x7CA54` |
| `FUN_0004B100` calls the pre-pass for both team bases after the clock/gate | `decompile_function 0x4B100` |
| `get_xrefs_to 0x4B100` = 3 (`0x4951E`/`0x49FBB`/`0x4A25A`) | `get_xrefs_to 0x4B100` |
| second driver `0x74427 MOV EAX,EBP; CALL 0x8D8EC; MOV EAX,EDI; CALL 0x8D8EC` | `disassemble_bytes 0x74400` |
| `0x743CF` reads session gate `[0x14C32A]`; `0x7440B CALL 0x8AF38` | `disassemble_bytes 0x743C0` |
| Ghidra has **no** function at `0x74429` | `get_function_by_address 0x74429` = "No function found" |
| `0x74442` reads `[EBP+0x7B2]` (the pre-pass output) | `disassemble_bytes 0x74400` |

**Engine seam (read-only, first-hand):** `src/fifa96_engine/fifa96_match_run.c`
`fifa96_match_run_frame` (`:4124`) is the per-frame body — the derived equivalent
of `FUN_0004B100` (`:4110-4266`); its per-record action dispatch is the FU-141
entity chain (`match_run_entity_frame`/`fifa96_match_entities_update`, `:4212-4217`).
There is **no** engine equivalent of `FUN_0008D8EC` (grep of the tree for the
pre-pass shape = none); the only modelled fragment is
`fifa96_match_entities.c:698` (record-0 `timer7b = timer79`, *carried* FU273 §1.3).

---

## 3. ENGINE integration seams

* **Per-frame walk** — `fifa96_match_run_frame` (`match_run.c:4124`, region
  `:4122-4266`) is the derived `FUN_0004B100`; fresh `get_xrefs_to 0x4B100` = 3.
  The native pre-pass runs **inside** `FUN_0004B100` at `0x4B2AB`/`0x4B2B2`, once
  per team base, *after* the clock/goal block. The engine has no such call.
* **Second driver** — `0x74427` (fresh `disassemble_bytes 0x74400`):
  `MOV EAX,EBP; CALL 0x8D8EC; MOV EAX,EDI; CALL 0x8D8EC; MOV
  EAX,[0x157A4A]; SAR EAX,0x18; CMP EAX,2; JNZ 0x7445B; MOV EAX,[EBP+0x7B2]`
  — i.e. it drives the pre-pass for two bases and **consumes `[EBP+0x7B2]`**. It
  follows the camera track `CALL 0x736AC` (`0x743EF`), reads the session gate
  `[0x14C32A]` (`0x743CF`), and the clock `CALL 0x8AF38` (`0x7440B`). Ghidra has
  **no function** at `0x74429` (fresh `get_function_by_address` = none); the
  enclosing routine is unrecovered (leg FU281-L5). This is the render/camera
  driver — the *consumer* of the pre-pass's pick cell.
* **Double-dispatch caution.** `FUN_000782D0` (record 0) and `FUN_0007CA54`
  (records 1..10) both end in the record's own action dispatch
  (`CALL dword[rec+0x18]`). The engine's FU-141 chain already dispatches record
  actions, so a naive "run the pre-pass too" wiring dispatches record 0 twice.

**M2 re-pin answer.** The M2 tape's per-frame value is `hash=<016x>` — one
chained FNV-1a over the presented indexed canvas + palette
(`platform_null.c n_present`/`present_hash`, confirmed in `test_engine_m2.c:223-230`).
Today the pre-pass is **unported**, so the M2 baseline does **not** observe it and
this derivation is M2-neutral. **If ported and wired**, the pre-pass becomes
M2-observable on two counts: (i) it writes the team pick cells `[+0x7B2]`/
`[+0x7BA]`/`[+0x7BE]` that the camera/render driver consumes (`0x74442` reads
`[EBP+0x7B2]`; a faithful wiring feeds the camera the engine draws), and (ii) it
re-dispatches record actions already modelled by FU-141. Either can move the
drawn canvas and therefore the hash chain → **M2 re-pin required** (written reason
+ frame diff, the AGENTS rail) once the port lands. The pre-pass is therefore a
**faithful-port risk** for M2, not a neutral refactor.

---

## 4. Numbered open legs (with native owners)

* **FU281-L1 — `0x10F334` vs `0x10F37C` conflation.** The task/FU273 §1.2 read
  "the `0x10F37C`/`0x10F334` per-side tables"; fresh evidence shows
  `FUN_0008D8EC` reads **only** `0x10F37C + side*0xC` (single program-wide hit
  `0x8DA4D`), and `0x10F334` is read elsewhere (owner `FUN_0006E8E8`, the R11-L1
  claim bridge, plus `0x75BEF`/`0x7BBC2`/`0x7F00C`…). Native owner:
  `FUN_0008D8EC` (`0x8DA4D`) + `FUN_0006E8E8`. Next hop: pin the two adjacent
  tables' separate roles/sizes.
* **FU281-L2 — the NULL distance-out at search #1.** The first `CALL 0x8DE8C`
  (`0x8D9B2`) passes both `ECX` and `EBX` zero; fresh `decompile_function
  0x8DE8C` writes `*param_4` unconditionally, so one of the two zero args is a
  NULL out-pointer. Native owner: `FUN_0008DE8C` (`0x8DE8C`) — decide whether the
  write is guarded (Ghidra may have dropped the null check) or genuinely aliases
  linear 0.
* **FU281-L3 — the `FUN_0008DC68` metric.** The nearest-record score is `FUN_0008DC68`
  (`0x8DC68`); fresh `decompile_function` shows an abs-then-weighted-min/max fast
  planar-distance approximation whose exact algebra reads ambiguously under the
  short/int casts. Native owner: `FUN_0008DC68`. Next hop: pin the metric form
  (octagonal / alpha-max-beta-min) from raw bytes.
* **FU281-L4 — the `0x1577FA` / `0x1577F8` "ball X" label.** The selector cell is
  `word[0x1577FA]`; FU273 §1.2 labels it "ball X" (*carried*). Fresh xrefs show
  `0x1577F8` written by `FUN_000700F4` (`0x70187`), `FUN_00070544`
  (`0x705E8`/`0x70613`), `FUN_000736AC` (`0x7389E`) — the camera/ball-path helpers.
  Native owner: those writers. Next hop: name the cell (ball x or camera x).
* **FU281-L5 — the second driver's enclosing routine.** `0x74427`/`0x74429`
  belong to no analysed function (fresh `get_function_by_address 0x74429` =
  none). Native owner: the unrecovered routine around `0x74427` (carries the
  `0x743CF` gate and `0x7440B` clock). Next hop: recover the routine head.
* **FU281-L6 — the template grid scale and its sources.** The `0x157770` arm adds
  `(dword[0x1577BE]>>16)<<5` / `(dword[0x1577C0]>>16)<<5` (scale ×32). Fresh
  evidence shows the arithmetic; the producer of `0x1577BE`/`0x1577C0` is not
  pinned here. Native owner: the `0x157770`-writing helpers (e.g. `FUN_00070544`).
* **FU281-L7 — the wiring seam (does the pre-pass replace or augment the FU-141
  chain?).** The pre-pass *is* the native per-record action dispatch (`FUN_000782D0`
  + `FUN_0007CA54` → `CALL dword[rec+0x18]`), which the engine already models via
  FU-141. Whether the port routes the pre-pass through the existing dispatch
  (no double-run) or adds it is the pivot for both correctness and the M2 re-pin.
  Native owner: `FUN_000782D0` (`0x782D0`) + `FUN_0007CA54` (`0x7CA54`).

---

## 5. Verdict

The **port shape is fully derivable** (this document). The pre-pass is **not**
a self-contained bounded implementation slice: it only means anything once
`FUN_000782D0` (record-0 machine) and `FUN_0007CA54` (per-record update) are
port-owned, and both are the native per-record dispatch the engine already
models (FU-141) — so wiring `FUN_0008D8EC` alone either no-ops or double-dispatches,
and once wired faithfully it perturbs the M2 golden. This corroborates FU273 §4.3:
the driver pre-pass is a **seam of the keeper-CPU cluster (FU273 unported surface
#1: FU186-L7 / FU180-L4)**, not a standalone task. Derivation **GO**; isolated
implementation **NO-GO** — fold into the keeper-CPU multi-task phase.
