# FU298 — R11-L4: the install-permission table `[0x110680]` (P1)

**Phase/Provenance:** M2 phase-28 recon-ahead wave, Task 1/8 (bounded, read-only derivation); draft
awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; case-sensitive; confirmed this session by
`ghidra_get_current_program_info`: `watcom:LE:32:default`, image base 0, `.image` overlay,
2706 functions). Read-only: no rename/comment/label/function-create/script/save; `/FIFA96.EXE`
untouched; ISO untouched; no git. Every native address/byte below is a **fresh**
`get_xrefs_to` / `disassemble_bytes` / `disassemble_function` / `decompile_function` /
`read_memory` / `search_instructions` of *this* session unless marked *carried* (FU282/FU296,
named). Engine-source facts cite the tree by line (read fresh, read-only). No file is modified
except this draft.

**Scope.** P1 — the `[0x110680]` install-permission gate of the record-0 machine `FUN_000782D0`
(`0x784DC`), the shared gate FU282 §1.3-vs-§1.4 split, the fresh reader/writer census of
`0x110680`, the table layout / index / value domain, and the port shape. This closes FU282-L5 /
FU296-L2 (the last end-to-end blocker *besides* the interlock co-blocker, which stays out of
slice).

---

## 1. The gate (fresh bytes)

`FUN_000782D0` (`0x782D0..0x785D0`, 224 insns; `get_function_by_address 0x784DC` =
`FUN_000782d0`, fresh) — the record-0 machine, **sole caller `0x8DB35`** in `FUN_0008D8EC`
(fresh `get_xrefs_to 0x782D0` = 1 `UNCONDITIONAL_CALL`; fresh `disassemble_bytes 0x8DB10` shows
the call at the head of the `0xB2`-stride walk: `0x8DB2E MOV EAX,EBP; 0x8DB30 MOV EBX,1;
0x8DB35 CALL 0x782D0`, then records 1..10 via `0x7CA54` while `EBX < 0xB`).

The brief's bytes, re-verified fresh (`disassemble_function 0x782D0`):

```
0x784D3  MOV EAX,[EBP+0x8e]            ; 8b 85 8e 00 00 00
0x784D9  SAR EAX,0x18                  ; c1 f8 18  -> EAX = byte [EBP+0x91]
0x784DC  MOV AL,[EAX+0x110680]         ; 8a 80 80 06 11 00   (the brief's bytes)
0x784E2  AND AL,0x1                    ; 24 01
0x784E4  MOVSX EAX,AL
0x784E7  TEST EAX,EAX
0x784E9  JZ 0x7859E                    ; 0f 84 af 00 00 00  (target = 0x784EF+0xAF = 0x7859E)
0x784EF  ... the install tree 0x784EF..0x78576 (target 0 / 4 / 0x19; setter 0x7D9A4 deferred, ECX=0)
0x7857B  ... the ladder gate 0x7857B..0x78597 (mode==2 && [+0x20]==0 && [0x157C5D]!=0)
0x78599  CALL 0x77EAC                  ; the keeper-CPU ladder seam
0x7859E  [0x157C5D]=0; CALL [EBP+0x18]; ... the shared tail
```

**Shared-gate clarification (FU282 §1.3-vs-§1.4).** There is exactly **one** permit read. The
`0x784E9 JZ 0x7859E` skips **both** the §1.3 install tree and the §1.4 ladder gate to `0x7859E`,
which sits *after* the `0x78599 CALL 0x77EAC` (fresh bytes `e8 0e f9 ff ff` @`0x78599`, next insn
`0x7859E XOR CH,CH`), while the native `0x785A2` interlock clear is still reached. §1.3's
"install nothing when `[0x110680][code] & 1 == 0`" and §1.4's ladder gating are two *consumers* of
one gate — no second table read exists between them (fresh: `0x7857B..0x7858E` reads
`[0x157A4A]`, `[EBP+0x20]`, `[0x157C5D]` only). Decompiled condition (fresh,
`decompile_function 0x782D0`): `if (((&DAT_00110680)[*(int*)((int)param_1 + 0x8e) >> 0x18] & 1)
!= 0) { ... }`.

**Index (what EAX is).** `[EBP+0x8E]>>0x18` is the dword at `+0x8E` shifted down 24 bits = the
**byte at `+0x91`** — the record's current action/state code. The alias `+0x8E >> 24 = +0x91`
holds at **all 19 read sites** (fresh disassembly of every site and its predecessor; see §2).
The state byte is written by `FUN_0007D9A4` (fresh `decompile_function`): `*(byte*)(rec+0x91) =
state` after the `3 -> 0x19` remap (`if (byte[+0x8D]==0 && state==3) state = 0x19`), and the
same function dispatches `(&PTR_LAB_001106e0)[state]` into `rec[6]` (= `+0x18`, the handler
invoked by the `0x785A8 CALL [EBP+0x18]` tail — the loop closes).

## 2. The reader census (fresh xrefs — no writer exists)

`get_xrefs_to 0x110680` = **19** references, all `DATA`; every `from_address` is inside code.
Program-wide `search_instructions` operand `110680` over **234,988 instructions** returns the
**same 19 sites and nothing else** — every one is a byte **load** (`MOV AL/DL, [reg+0x110680]`).
There is no store, no `LEA`/`MOV reg,imm`, and no other literal form addressing the table
(companion scan operand `1106` adds only the mode-table readers / the `ADD EAX,0x1106E0` /
unrelated `JNZ 0xc1106`). **No code writes the table anywhere in the LE image.**

| # | site | containing function | mask | index source (fresh) |
|---|------|--------------------|------|----------------------|
| 1 | `0x761D6` | `FUN_00076130` | `&1` | `[rec+0x8E]>>0x18` — **with the `state != 0x1F` special case** (below) |
| 2 | `0x784DC` | `FUN_000782D0` | `&1` | `[rec+0x8E]>>0x18` (the P1 gate) |
| 3 | `0x7CC9C` | `FUN_0007CA54` | `&1` | `[rec+0x8E]>>0x18`, `mode==2` |
| 4 | `0x7CDF4` | (0x7Cxxx family body) | `&1` | `[rec+0x8E]>>0x18`, `mode==2` |
| 5 | `0x7CE54` | (0x7Cxxx family body) | `&1` | `[rec+0x8E]>>0x18`, `mode==2` |
| 6 | `0x7CECC` | (0x7Cxxx family body) | `&1` | `[rec+0x8E]>>0x18`, `mode==2` |
| 7 | `0x7CF38` | (0x7Cxxx family body) | `&1` | `[rec+0x8E]>>0x18`, `mode==2` |
| 8 | `0x7CF70` | (0x7Cxxx family body) | `&1` | `[rec+0x8E]>>0x18`, `mode==2` |
| 9 | `0x7CFE8` | `FUN_0007CFD0` | **`&3`** | `[rec+0x8E]>>0x18`, `mode==2` |
| 10 | `0x7D02A` | (0x7Cxxx family body) | `&1` | `[rec+0x8E]>>0x18`, `mode==2` |
| 11 | `0x7D06C` | `FUN_0007D054` | **`&3`** | `[rec+0x8E]>>0x18`, `mode==2` |
| 12 | `0x7D0A4` | (0x7Cxxx family body) | **`&3`** | `[rec+0x8E]>>0x18`, `mode==2` |
| 13 | `0x7D0DD` | (0x7Cxxx family body) | `&1` | `[rec+0x8E]>>0x18`, `mode==2` |
| 14 | `0x7D12C` | `FUN_0007D110` | `&1` | `[rec+0x8E]>>0x18`, `mode==2` |
| 15 | `0x7D190` | `FUN_0007D174` | `&1` | `[rec+0x8E]>>0x18`, `mode==2` |
| 16 | `0x7E620` | `FUN_0007E600` | `&1` | `[rec+0x8E]>>0x18` |
| 17 | `0x7E83C` | (0x7Exxx family body) | `&1` | `[rec+0x8E]>>0x18` (record just found by `0x8DE8C`) |
| 18 | `0x89790` | (0x89xxx body) | `&1` | `[[0x1587EC]+0x8E]>>0x18` (the `0x1587EC` holder's record) |
| 19 | `0x8D274` | `FUN_0008D098` | `&1` | the record stored to `[rec+0x7B2]` by `0x8DE8C` |

**The `0x1F` special case is local to `FUN_00076130` (fresh `decompile_function`).** Its gate is
`if ((state != 0x1f) && ((&DAT_00110680)[state] & 1) == 0) return 0;` — i.e. **state `0x1F`
(31) is permitted unconditionally**, bypassing the table (whose byte 31 is `0`). No other site
shows a pre-compare (all 19 predecessor windows are the plain `SAR/MOV AL/AND/TEST/JZ`).
**The ladder gate `0x784DC` does NOT carry this special case** — a port must keep them distinct.

**Semantics (fresh, byte-level).** Bit 0 is the re-selection/redirect permission: at `0x784DC`
it gates the record-0 install tree + ladder; at `0x7CC9C` (`FUN_0007CA54`, the records 1..10
sibling machine) it gates `FUN_0007C990` + the `FUN_0007D9A4(rec, 8, 0, 0)` state-8 re-install;
the `&1` sites are all "may this state re-select/engage" predicates under `mode==2`. Bits 1
(the `&3` sites) gate writes of the `[*rec+0x82A]` flag and the `[+0x7BF]` pointer (fresh
`0x7CFE8`/`0x7D06C` bodies) — a second, narrower flag; its consumers are off-slice (FU298-L3).

## 3. The table itself (fresh `read_memory`)

Fresh `read_memory 0x110680` (256 B), `0x1106E0` (256 B), `0x1107E0` (256 B), `0x110620` (96 B):

**State-indexed bytes `0x110680..0x1106AC` (45 = `0x2D` entries, indices 0..44):**
`3` at `{0, 3, 4, 5, 6, 25}`; `2` at `{7, 16, 17, 18, 19}`; `0` everywhere else. So the
**value domain is `{0, 2, 3}`** and `bit0` (all 16 `&1` sites) is set **iff the value is 3** —
the permitted codes are **`{0, 3, 4, 5, 6, 0x19}`**. Bit 1 alone (value 2) marks
`{7, 16, 17, 18, 19}`.

**The table is bounded by its physical neighbours (fresh, all reads):**

- `0x1106AD = 0x110680 + 0x2D` — a **mode-indexed** byte table (index =
  `[0x157A4A]>>24`, the mode byte): fresh `disassemble_bytes 0x8AF40` shows
  `MOV EAX,[0x00157a4a]; SAR EAX,0x18; MOVZX DI/MOV AL, [EAX+0x1106AD]` in
  `FUN_0008AF38` (and its `decompile_function` form `(&DAT_001106ad)[mode]`).
- `0x1106C3 = 0x110680 + 0x43` — a second **mode-indexed** byte table: fresh
  `disassemble_bytes 0x79F58` shows `MOV EAX,[0x00157a4a]; SAR EAX,0x18;
  CMP byte [EAX+0x1106C3],0` in `FUN_00079F3C` (`(&DAT_001106c3)[mode]`).
- `0x1106E0 = 0x110680 + 0x60` — the **state → handler dispatch table**: 80 dwords
  (`0x1106E0..0x11081C`, fresh reads; entry 67 (`0x1107EC`) is `0`), consumed *solely* by
  `FUN_0007D9A4` (fresh `get_xrefs_to 0x1106E0` = 2, both in `FUN_0007D9A4`; fresh
  `search_instructions` finds `ADD EAX,0x1106E0` @`0x7DA77`).
- `0x110820` — the `{mask-word, id-word, fn-dword}` 8-byte record tables (fresh read;
  fresh `get_xrefs_to 0x110820`: one code read in `FUN_0007CA54` @`0x7CB9D` and the
  `0x1109D0` pointer array). Off-slice.

So the state-indexed table **cannot be longer than `0x2D` bytes** without aliasing the mode
tables; its working set is `≤` the highest state value that ever reaches a read. Fresh sampled
state immediates at the 77 `FUN_0007D9A4` call sites (`get_xrefs_to 0x7D9A4` = 77) all land
`≤ 0x24` (`0x24`@`0x8B750`, `0x20`@`0x899AB`, `0x18`@`0x8A32F`, `0x16`@`0x8A0A0`, plus the
machines' own `{0,3,4,5,7,0xA,0xC,0xF,0x19,0x1A,0x1B,0x1C,0x1F,0x21}`); the exhaustively decoded
call-site domain stays a leg (FU298-L1).

**Who writes entries / when do they go 1?** Nobody, and they never change: there is **no code
producer** (the scan above), no data-stored pointer into the table (every xref is an
instruction), and the bytes read are the LE image's initialized data
(`+0x100000` file-offset mapping: file offset `0x10680`). The entries *are* 1 (bit 0) for
`{0,3,4,5,6,0x19}` from the moment the image loads — "arming" is not a runtime event but a
function of the record's current code.

## 4. Would the natural M2 tape arm it? — YES

- The engine's stepped record is `records[0]` per team (fresh read,
  `src/fifa96_match_entities.c:732-780`), and `kc_rec.code = e->code` (`:747`) is the live
  state. Fresh init path sets `e->code = 0` (`:202`); **`perm[0] = 3` → bit0 = 1** — the gate
  opens on the very first stepped frame.
- The M2 tape drives record 0 through the keeper-hold code `0x19` (the tape's own documented
  staging: `tests/test_engine_m2.c:355` "the inactive live kickoff record (0x19 code, row 19
  …)", `:1239` "the row-04 carrier install `0x7E866`/`0x7E8C1` stages code 0x19"); **`perm[0x19]
  = 3`** — the gate opens there too.
- The gate record set that keeps it **closed** is `{1, 2, 7..15, 16..24, 26..44}`
  (bit0 = 0), e.g. the bit1-only `{7,16,17,18,19}`. A frame where record 0 carries one of
  those codes is not excluded (FU298-L5).
- **Tape consequence (Stage-2, named):** once P1 is live, the same gate ALSO opens the install
  tree; with the seam's unset `ball_owner = 0` (fresh `memset` + no bind,
  `src/fifa96_match_entities.c:745-766`) the tree computes target `0x19` and the `0x78560`
  same-code guard decides whether `e->code` moves. The phase-27 waiver test pins `permit == 0`
  and says so itself ("A future `permit` producer (or a gate flip) makes this RED",
  `tests/test_engine_match_frame.c:3572-3573`, fresh) — that test must be re-aimed, and the M2
  byte-exactness re-checked, in the Stage-2 change (FU298-L6), not assumed.

## 5. Port shape (cell + producer) and GO/NO-GO

**Cell.** A byte-exact **`const` table** in the keeper-CPU module — the fresh 45 state bytes
`0x110680..0x1106AC` (values `3` at `{0,3,4,5,6,25}`, `2` at `{7,16,17,18,19}`, else `0`) —
plus the live read at the seam bind, replacing the caller-supplied stand-in:

```c
static const uint8_t kc_permit_tbl[0x2D] = { /* fresh read_memory 0x110680, 45 B */ };
...
kc_rec.permit = (e->code < 0x2D) ? (kc_permit_tbl[e->code] & 1u) : 0u;   /* the 0x784DC gate */
```

The bound check is port-side hardening (the FU290 clamp precedent): native has no visible
guard; an out-of-range state would alias the mode tables (FU298-L1). The 0x1F special case
stays with `FUN_00076130` only (§2) and must **not** leak into `0x784DC`.

**Producer.** None to port. This is the decisive correction to FU296-L2's "a different unported
producer" framing: there is no code writer to reach or to trace — the producer is the image's
static initializer, i.e. the same class as FU290's `kc_stage_tbl` and FU291's
`fifa96_input_range_words` (ported as data, zero-δ). The engine's `fifa96_keeper_cpu.permit`
cell (`include/fifa96_loader/fifa96_keeper_cpu.h:117-121`) and the seam bind
(`fifa96_match_entities.c:766`) retire to a computed value (optionally kept as a test-only
override).

**GO/NO-GO: GO.** The permission can go live **honestly**: the bits are static image data, the
reader is fully determined (`perm[code] & 1`), the tape demonstrably arms it (§4), and the
errata below retires the "dead producer" reading. The remaining work is a Stage-2 change with a
known RED-first test, not an unprovable frontier. (The ladder itself still needs the interlock
co-blocker fix — FU296-L3 / FU257-L6 — but that is a separate, already-named leg.)

## 6. Errata (quoted; prior docs not rewritten)

- **FU282-L5** ("the table contents are not enumerated here") — **advanced**: the state table is
  enumerated (§3: 45 bytes, `{0,2,3}`, bit0-set `{0,3,4,5,6,0x19}`); the leg narrows to the
  exhaustively-decoded state domain (FU298-L1).
- **FU296-L2** ("a *different* unported producer, zero live") — **partially superseded**: there
  is no producer code at all (§2/§5); the honest port is a data table + live bind, so the
  phrase "producer" describes a static initializer, not an unreachable writer.
- **T7 waiver expectation** (`tests/test_engine_match_frame.c:3555-3581`) — confirmed as written:
  a live P1 flips that test RED by design; the flip is the Stage-2 acceptance signal, not a
  regression (FU298-L6).

## 7. Numbered open legs

- **FU298-L1 (owner: the `FUN_0007D9A4` call-site census).** The exhaustive state-value domain
  written to `+0x91` (77 call sites; fresh samples all `≤ 0x24`; `FUN_00076130`'s `0x1F` is a
  *read* special, not a written state). Any state `≥ 0x2D` would read the mode tables at
  `+0x2D`/`+0x43` (fresh bases); no native bound check exists at the read sites.
- **FU298-L2 (owner: the 16-bit loader spine / `docs/ghidra/loader_rename_map.md`).** The
  no-writer proof covers the whole LE image instruction surface; a pre-entry patch of the image
  data by the 16-bit loader stub is neither excluded nor evidenced.
- **FU298-L3 (owner: the `&3` sites' consumers).** Bit 1 semantics beyond "gates writes of
  `[*rec+0x82A]` / `[+0x7BF]`" (fresh `0x7CFE8`, `0x7D06C`); the consumers of those cells are
  off-slice.
- **FU298-L4 (owner: `FUN_0008AF38` / `FUN_00079F3C`).** The mode-indexed tables at `0x1106AD` /
  `0x1106C3` — decoded here only as the physical bound of the state table; their own meanings
  are out of slice.
- **FU298-L5 (owner: the M2 golden harness).** The frame-by-frame record-0 code trajectory on
  M2: the arm claim rests on fresh init code `0` + the tape's documented `0x19` staging; frames
  carrying a bit0=0 code (`{1,2,7..24,26..44}`) are not excluded.
- **FU298-L6 (owner: Stage-2 / the FU296-T4 harness).** The tape consequence of arming P1 live:
  the install tree can emit a deferred install (target `0x19` with the unset `ball_owner = 0`)
  — re-aim `test_keeper_installer_live_waiver` (RED by design) and re-verify M2 byte-exactness
  (or re-pin with a written reason) in the same change.

## 8. Evidence ledger (all fresh, this session)

| # | operation | result |
|---|-----------|--------|
| E1 | `get_current_program_info` | `/FIFA96.EXE`, `watcom:LE:32:default`, base 0, `.image`, 2706 fns |
| E2 | `get_function_by_address 0x784DC` | `FUN_000782d0` (`0x782D0..0x785D0`) |
| E3 | `disassemble_function 0x782D0` (224) | the gate bytes `0x784D3..0x784E9`; `CALL 0x77EAC` @`0x78599`; `JZ 0x7859E`; tail |
| E4 | `decompile_function 0x782D0` | `((&DAT_00110680)[+0x8E>>24] & 1) != 0` guards tree + ladder; target tree 0/4/0x19 |
| E5 | `get_xrefs_to 0x110680` | 19 `DATA` refs, all instruction sites (§2 table) |
| E6 | `search_instructions "110680"` | 19 matches, all `MOV` loads (234,988 insns scanned) |
| E7 | `search_instructions "1106"` | + mode tables (`0x11064E`,`0x1106AD`,`0x1106C3`), `ADD EAX,0x1106E0` @`0x7DA77`; no writes |
| E8 | `read_memory 0x110680/0x110620/0x1106E0/0x1107E0` | the 45 state bytes; mode tables; 80-dword dispatch (NULL @idx 67); 8-byte records @`0x110820` |
| E9 | `disassemble_bytes` at all 19 sites + predecessors | index `[rec+0x8E]>>0x18`, masks 16×`&1` / 3×`&3`, `mode==2` guards |
| E10 | `decompile_function 0x76130` | `(state != 0x1f) && (perm&1)==0 -> 0`; states 0x19/0x1C/0x1B/4/5 installs |
| E11 | `decompile_function 0x7D9A4` | writes `+0x91`; `3->0x19` remap; `PTR_LAB_001106e0[state]` -> `rec[6]` (`+0x18`) |
| E12 | `get_xrefs_to 0x7D9A4` = 77; `get_xrefs_to 0x1106E0` = 2; `get_xrefs_to 0x782D0` = 1; `get_xrefs_to 0x110820` = 2 | setter census; dispatch consumers; sole caller `0x8DB35`; record-table refs |
| E13 | `disassemble_bytes 0x8DB10` (80 B) | record-0 call head + `0xB2` walk to `0x7CA54` (records 1..10) |
| E14 | `decompile_function 0x7CA54` | sibling gate `0x7CC9C` + `FUN_0007D9A4(rec,8,0,0)` |
| E15 | `decompile_function 0x79F3C` + `disassemble_bytes 0x79F58`; `disassemble_bytes 0x8AF40` | mode tables `0x1106C3` / `0x1106AD` (index `[0x157A4A]>>24`) |
| E16 | `disassemble_bytes` at `0x8B750`, `0x899AB`, `0x8A0A0`, `0x8A32F`, `0x844E7`, `0x8753C` | setter states `0x24/0x20/0x16/0x18/4/3` (max `0x24`) |
| E17 | engine sources (read fresh) | `fifa96_keeper_cpu.c:50`, `.h:25/117-121`, `fifa96_match_entities.c:202/732-783` (`permit` bind `:766`), `fifa96_match_run.c:4249-4259`, `test_engine_match_frame.c:3555-3624`, `test_engine_m2.c:355/1239` |

*Carried and named:* FU282 §1.3/§1.4 (gate shape, deferred install ECX=0), FU282-L3/L5,
FU296-L2/L3, FU290's data-port precedent (`kc_stage_tbl`), FU291's `input_range` data port, the
ENGINE.md phase-27 register (`:1765-1914`), the census line `59/80`.
