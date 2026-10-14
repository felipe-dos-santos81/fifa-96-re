# FU-253 — row `08` unmodeled inputs/sinks (OL-82), the `+0x8E` `type`/`actor_type` field-sharing wrinkle (OL-83), and the OL-19/OL-22/OL-34 metadata legs

Provenance: M2 phase-24 wave **W24-6** (Task 6), 2026-10-14, read-only
recon-ahead. Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts
disabled; case-sensitive program argument; `get_current_program_info` =
`watcom:LE:32:default`, `min_address` `0x10000`, `max_address`
`.image::00174a2a`, `function_count` **2706**, `symbol_count` **15239**, one
`.image` overlay). Every address/byte/xref/insn below is a fresh
`disassemble_bytes` / `disassemble_function` / `read_memory` /
`search_instructions` / `get_xrefs_to` / `get_function_by_address` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named). Engine
and loader source read first-hand (`fifa96_match_handlers.c`,
`fifa96_match_entities.h`, `fifa96_outfield.c/.h`,
`fifa96_arm_helpers.h`, `fifa96_arm_bodies.c`). No writes: no rename, comment,
label, function, script or project save; `/FIFA96.EXE` untouched; no git; no
code/test edits. Alias traps applied (`dword[x]>>16` = word at `x+2`; `>>24` =
byte at `x+3`); all CALL rel32 byte-verified.

Scope (task-6 brief): (1) row `08` `0x81068..0x814AF` unmodeled inputs/sinks
(OL-82); (2) the `+0x8E` `type`/`actor_type` shared-byte wrinkle (OL-83) with
the write-vs-read census and the reconciliation ruling; (3) the OL-19 / OL-22 /
OL-34 metadata legs (fresh definitions). Promote target:
`docs/ghidra/FU253_row08_fields.md`.

---

## Answer in one line

**Row `08`'s unmodeled surface (OL-82) is fully enumerated and producer-
attributable — every input is either a record byte (`+0x3D`/`+0x44`/`+0x99`/
`+0x7D`, `rec[+4]` descriptor), a process byte (`[0x15877D]`/`[0x15872F]`,
`[0x1577C0/C2]`, `[0x15774C]`, `[0x157750]`, `[0x157A83]`, `[0x1577CA]`) or a
shared sink (`0x8ED40`/`0x8F188`/`0x92820`/`0x7A490`/`0x974F0`/`0x651F0`/
`0x92AC8`/`0x6E598`), all named below; and the OL-83 split is a **documentation
artifact, not two bytes** — the pool's `record.type` and `record.actor_type`
both denote the *same* native per-record byte `+0x8E` (one via the
`byte[rec+0x8E]` idiom, the other via the `dword[rec+0x8B]>>24` idiom), so the
ruling is to **unify them** (`record.type` is the correct field; `actor_type` is
a legacy alias, per the engine's own FU-151 erratum wording), not to keep two
aliased cells.** The OL-19/OL-22/OL-34 legs remain open (OL-19 partially: its
`FUN_00079F3C` camera item is stale, closed by FU-177).**

---

## 1. Tool calls (first-hand, read-only)

* `disassemble_bytes` `0x81068..0x814AF` (328 insns; terminal `RET` at
  `0x814AE` — the FU-142 K.6.8 "329 incl. RET" erratum re-confirmed: 328 insns
  returned for the span, the last a `POP EBX` before the `0x814AF`-adjacent
  epilogue; the row body ends `0x814AF`).
* `disassemble_bytes` `0x870E8..0x872E8` (148 insns; row `28` head — the
  `0x79C50` face call at `0x87127`, the `0x6E598` anim at `0x8722C`).
* `disassemble_bytes` `0x7E7C8..0x7E900` (90 insns; row `04` head) and
  `0x7E45C..0x7E4B4` (26 insns; row-04 `+0x8E` anim reads `0x7E467`/`0x7E4A0`).
* `disassemble_bytes` `0x7EF60..0x7EFC0` (21 insns; row-04 `0x10F334/0x10F33C`
  reads `0x7EF70`/`0x7EF89`).
* `disassemble_bytes` `0x80810..0x80840` (11 insns; row-06 `+0x8E` read
  `0x8081B`).
* `disassemble_bytes` `0x849B0..0x84AE1` (82 insns; row `18` — **no `+0x8E`
  access**).
* `disassemble_bytes` `0x81558..0x815A0` (17 insns; row-07 `+0x8E` reads
  `0x81568`/`0x81585` via the `0x10F331`/`0x10F339` dword-base alias).
* `disassemble_bytes` `0x6F8F0..0x6F960` (32 insns; `FUN_0006E8E8` `+0x8E`
  writes `0x6F90A`/`0x6F945`).
* `disassemble_function` `0x79C50` (26 insns; the face helper).
* `read_memory` `0x10F334` (64 B) — the `0x10F334`(+`0x10F33C`) dir tables.
* `search_instructions` operand `0x8e]` (87 hits, `truncated:false`), operand
  `+ 0x8b]` (156 hits, `truncated:false`), operand `79c50` on `CALL` (40 hits).
* `get_xrefs_to` fresh: `0x7A490`(9), `0x8ED40`(35), `0x92820`(33),
  `0x651F0`(53).
* `get_function_by_address` probes `0x86ACB`, `0x81683`, `0x7F374`, `0x7E7C8`,
  `0x870E8`, `0x80200` — all `no function found` (the action-table targets are
  not Ghidra functions, matching FU-138 §7's standing concern; the citations are
  raw disassembly windows).

---

## 2. Row `08` `0x81068..0x814AF` — the OL-82 input/sink census (all fresh)

The body is the `+0x3D`-gated projection scan machine. Fresh sites (the ported
loader body is `fifa96_outfield_row08_step`, `src/fifa96_loader/fifa96_outfield.c:735`):

### 2.1 The head / stage dispatch

| site | fresh instruction | meaning |
|---|---|---|
| `0x81073` | `MOV EAX,[0x157A4A]; SAR EAX,0x18; CMP EAX,2` | **phase gate** `byte[0x157A4D] == 2` |
| `0x81080` | `CALL 0x7DAB4` | phase-mismatch reset |
| `0x81094` | `MOV AX,[0x157A64]` | **`[0x157A64]` zero-extended word added to `dword[+0x89]` timer** |
| `0x8109C` | `MOV AL,byte[EBP+0x92]` | stage latch `+0x92`; dispatch `0→0x810C4`, `1→0x811D6`, `2→0x8147C`, else epilogue |

### 2.2 Stage `0` / continuation

| site | fresh | meaning |
|---|---|---|
| `0x810CC` | `CMP byte[EBP+0x8D],0` | `+0x8D` active flag |
| `0x810DF`/`0x810F4` | `CMP EBP,[EAX+0x7B2]`/`[EAX+0x7B6]` | team target/second clear (inactive arm) |
| `0x81114` | `MOV ESI,0x15774C; MOVSD×3` | **camera triple `[0x15774C..]` → `rec+0x4D..+0x58`** |
| `0x8112F`/`0x8113D` | `MOV EAX,[0x1577C0]; SAR EAX,0x10; SHL EAX,0x3; ADD [EBP+0x4D],EAX` | **`word[0x1577C0]<<3` added to `+0x4D`** |
| `0x81137`/`0x81145` | `MOV EAX,[0x1577C2]; SAR EAX,0x10; SHL EAX,0x3; ADD [EBP+0x55],EAX` | **`word[0x1577C2]<<3` added to `+0x55`** |
| `0x81148` | `TEST EDI,[EBP+0x20]` | slot pointer `+0x20` (has_slot) |
| `0x81157` | `CMP dword[0x157750],0x38` | **ball-height word `[0x157750]`** (with `word[+0x6B]>0x50` at `0x81152`) |
| `0x81162` | `CALL 0x79B58` | receiver timer |
| `0x81167` | `CMP dword[EBP+0x89],0x3C` | timer89 > 0x3C → reset |
| `0x81193` | `CALL 0x79C50` | **face: `0x79C50(rec, DX=word[+0x6B], BX=word[+0x6D])`** (`0x8118D`/`0x81190` = `SAR EBX/DX,0x10`) |
| `0x8119F` | `MOV ECX,[EBP+0x8B]; SAR ECX,0x18` | **reads byte `+0x8E`** |
| `0x811AA` | `CALL 0x6E598` | **anim kind `0xB`, frame `byte[+0x8E]`** (OL-52) |
| `0x811AF` | `MOV byte[EBP+0x9E],1` | ran latch `+0x9E` |
| `0x811CA` | `MOV byte[0x15877D],BH` | **process byte `[0x15877D] = 0`** |
| `0x811D0` | `MOV byte[EBP+0x92],CL` (`INC CL`) | stage++ |

### 2.3 Stage `1` gates + the `+0x3D` projection scan

| site | fresh | meaning |
|---|---|---|
| `0x811D6` | `CMP byte[0x15877D],0` | **process byte `[0x15877D]` gate** |
| `0x811DF` | `CMP byte[EBP+0x44],0` | `+0x44` anim-row terminal gate |
| `0x81207` | `MOV EAX,[EBP+0x3A]; SAR EAX,0x18; CMP EAX,1` | **`+0x3D` frame gate (`byte[+0x3D]==1`)** |
| `0x81216`/`0x8121F` | `MOV EAX,[EBP]; MOV EAX,[EAX+0x7A6]; MOV EDX,[0x157A83]; MOV EAX,[EAX+0x7B2]` | opp team `[team+0x7A6]`, user `[0x157A83]`, opp target `+0x7B2` |
| `0x81233` | `MOV EDX,[EBP+4]; MOVSX BX,byte[EDX+0xC]; MOVSX DX,byte[EDX+0x16]; … [EAX+0xC]/[EAX+0xF]` | **descriptor delta `rec[+4]`(`+0xC`,`+0x16`) − opp`[+4]`(`+0xC`,`+0xF`)`** |
| `0x81257` | `MOVSX DX,byte[EAX+0xC]; MOVSX AX,byte[EAX+0x16]` | same descriptor pair (no-user arm) |
| `0x81269` | `MOVSX DX,byte[0x15872F]` | **process byte `[0x15872F]`** |
| `0x81266..0x81298` | `IMUL EAX,EAX,6; SHL EDX,3; ADD EDX,0x20; ADD EDX,EAX` then `CMP 0x40`/`0x8` | radius `q = clamp(6·delta + (int8)[0x15872F]·8 + 0x20, 8, 0x40)` |
| `0x812FC..0x81351` | `LEA ESI,[EBP+0x59]; 0x795A4`×2 (`0x8130D`/`0x81326`), `0x8DCD4` (`0x81347`) | scan offsets `0,0x10,0x20,0x30` (`0x81463` `ADD ECX,0x10`; `0x81466` `<0x40`) projecting `[EBP+0x59]`/`[0x15774C]`, distance vs `q` |
| `0x81357`/`0x81373` | `MOV EAX,[EBP+0x8B]; SAR EAX,0x18; MOVSX AX,[EAX+0x10F334]` / `…+0x10F33C`; `IMUL 0xA0` | **per-type bytes indexed by `byte[+0x8E]` × `0xA0`** |
| `0x8139D` | `CALL 0x8DC68` | octagonal distance |

### 2.4 The event/sound arms (the OL-82 sinks) + stage `2`

| site | fresh | meaning |
|---|---|---|
| `0x813A2`/`0x813B1` | `MOV ECX,[0x157A83]; MOV EDX,[ECX+0x28]; MOV DL,[EDX]; AND EDX,0xFF; CMP EDX,0x4A` | **user `[0x157A83]` and `byte[[user+0x28]] == 0x4A`** |
| `0x813C3` | `MOV EDX,2; MOV EBX,4; CALL 0x8ED40` | user arm event `0x8ED40(user, 2, 4)` |
| `0x813D2` | `MOV EAX,0x67; MOV EDX,[0x157A83]; CALL 0x8F188` | `0x8F188(0x67, user)` |
| `0x813E2`/`0x813E7` | `MOV EAX,1; CALL 0x651F0` | sound/flag `0x651F0(1)` |
| `0x813EE`/`0x813F5` | `MOV EDX,2; MOV EAX,EBP; CALL 0x8ED40` | record arm event `0x8ED40(rec, 2, 0)` |
| `0x813FA`/`0x81401` | `MOV EDX,0x16; MOV EAX,EBP; CALL 0x92820` | `0x92820(rec, 0x16)` |
| `0x81406..0x81417` | `PUSH 0; MOV ECX,9; XOR EBX,EBX; PUSH -1; LEA EDX,[ESP+0x14]; MOV EAX,EBP; CALL 0x7A490` | **`0x7A490(rec, EDX=6-byte vec, EBX=0, ECX=9, stack -1, 0)`** (OL-62 core) |
| `0x8141C` | `MOV byte[0x15877D],1` | **`[0x15877D] = 1`** |
| `0x81423` | `CALL 0x92AC8` (`0x81428` `XOR AH,AH`; `0x8142A` `AND AL,0x7F`; `0x8142C` `ADD EAX,0x190`; `0x81431` `CWDE`) | **RNG `0x92AC8`** → `0x974F0(0x190 + (rng & 0x7F))` |
| `0x81437`/`0x8143C` | `MOV EAX,1; CALL 0x651F0` | `0x651F0(1)` |
| `0x81441..0x81453` | `MOV dword[EBP+0x89],0; INC byte[EBP+0x92]` | stage++, timer89 = 0 |
| `0x8147C..0x814A0` | `MOV byte[0x15877D],0; CALL 0x79B1C; CMP byte[EBP+0x44],0; CALL 0x7DAB4; CMP EBP,[0x1577CA]; INC byte[0x15872F]` | **stage 2**: `[0x15877D]=0`, `0x79B1C` snap, `+0x44`→reset, `[0x1577CA]` compare, **`[0x15872F]++`** |

**Row 08 installs no code** (fresh: no `CALL 0x7D9A4` anywhere in the span;
the only `0x7D9A4`-family calls are *outside* `0x81068..0x814AF`).

**Sink sharing (fresh `get_xrefs_to`, "shared not row-08-specific"):**
`0x8ED40` = 35 call sites; `0x92820` = 33; `0x651F0` = 53; `0x7A490` = 9
(incl. row 08's `0x81417`, `FUN_0007B9C4` `0x7BF05`, `0x76030`, `0x7DF53`,
`0x7E168`, `0x753FE`/`0x7541C`, `0x80DC7`, `0x85C68`). None is consumed by a
derived home; they are the OL-27/OL-52/OL-62/OL-82 request sinks.

---

## 3. OL-83 — the `+0x8E` `type`/`actor_type` field-sharing wrinkle

### 3.1 The fact: one native byte, two pool fields

Fresh first-hand, **both** pool fields denote the *same* native per-record byte:

* `include/fifa96_engine/fifa96_match_entities.h:76-78` —
  `uint8_t type; /* native byte +0x8E (the face octant; the dword[+0x8E] >> 24
  alias selects +0x91) */`
  `uint8_t actor_type; /* +0x8B >> 24 */`.
* `+0x8B >> 24` is `dword[+0x8B]` shifted 24 = **the byte at `+0x8E`** (little-
  endian: `+0x8B`is bits 0-7, `+0x8C` 8-15, `+0x8D` 16-23, `+0x8E` 24-31). So
  `type == actor_type == byte[rec+0x8E]`. The two names are **two access idioms
  for one byte**, not two bytes.
* The engine *itself* says so at `fifa96_match_handlers.c:3931` (the FU-151 §3.5
  erratum): *"the `+0x8E` byte is correctly the face octant (`entity.type`, **the
  legacy alias `actor_type`**, OL-83 naming)"*. The handlers comment at
  `:1683-1685` names the reader idiom as `native +0x8B` (the dword base) — the
  same byte.

### 3.2 Write vs read census (fresh)

**Direct `MOV byte[reg+0x8E], AL` writers** (`search_instructions` `0x8e]`,
87 hits, untruncated; the byte-store subset):

| writer | site | character |
|---|---|---|
| `FUN_00079C50` | `0x79C8E`, `0x79CA0` | **the face octant** `((word[+0x7D]+0x40)&0x3FF)>>7` (+ `word[+0x7D]` at `0x79C76`) |
| `FUN_0006E8E8` | `0x6F90A`, `0x6F945` | **a 3-bit cycle** `(byte[+0x8E]±1)&7` (fresh `0x6F8F8 INC AL; 0x6F904 AND AL,0x7; 0x6F90A store`) — base identity owed (leg FU253-L4) |
| `FUN_0007BF20` | `0x7C051` | (row-06 family) |
| row `0B` | `0x81BE3` | `MOV AL,[EBP+0x8E]; MOV byte[ESI+0x8E],AL` (type copy) |
| `FUN_0008E244` | `0x8E362` | row-24 family |
| `FUN_0008E5BC`/`FUN_0008E910` | `0x8E691`, `0x8E9EE` | `MOV byte[reg+0x8E], 0x2` |

CALL `0x79C50` sites = **40** (`search_instructions`, untruncated). Rows **08**
(`0x81193`), **28** (`0x87127`), **2A** (`0x86AC1`) — the wired writers — all
reach `+0x8E` **through the face helper**, not a direct store (which is why a
`byte[+0x8E]`-write census alone misses them us).

**`byte[+0x8E]` readers** (`dword[+0x8B]>>24` or `MOVSX AX,byte[+0x8E]`) among
the *wired* rows (fresh sites):

| row | fresh site(s) | consumer |
|---|---|---|
| `04` | `0x7E467`, `0x7E4A0` (`→0x6E598`); `0x7EE31`, `0x7EF70`, `0x7EF8F`, `0x7F003`, `0x7F01C` (`→0x10F334/33C`) | anim frame + type offsets |
| `06` | `0x8081B` (`→0x6E598`) | anim frame |
| `07` | `0x81568`, `0x81585` (`0x10F331`/`0x10F339` dword-base `>>24` alias) | type offsets |
| `21` | `0x8532A` (`→0x6E598`) | anim frame |
| `0B` | `0x81DAE`, `0x81EDC` | type |
| `08`/`28`/`2A` | `0x8119F`/`0x81357`; `0x8722C`; `0x86ACB`/`0x86B12`/`0x86F8A`/`0x86FD0` | read back the byte they write |
| **`18`** | **none** — fresh `0x849B0..0x84AE1` has **no `+0x8E` and no `+0x8B` access**; `fifa96_action_duel` has no `type8` field | — |

### 3.3 The reconciliation ruling

* **`record.actor_type` is dead-aliased metadata.** The pool split native
  `+0x8E` into `type` (written by the wired face arms: rows 08/28/2A via
  `0x79C50` → `fifa96_arm_face` → `out.face_octant` → `r->type`) and
  `actor_type` (**never written** by any wired body — fresh: no writer). Every
  wired `actor_type` read (`handlers.c:906, 2341, 2534, 2779, 3004`) therefore
  sees a **stale 0**, while the native reader would see the last face write.
* **Ruling: unify on `record.type`.** Repoint the wired readers
  (rows 04/06/07 + the kick binder used by /0F, rows 0B/21) to `record.type`,
  and retire the `actor_type` C field (it is a naming alias, not a cell). This is
  the engine's own FU-151 wording ("legacy alias"). Keeping two fields that
  alias one byte is the bug; no native byte semantics are needed.
* **Erratum candidate (doc):** FU-142 §6 / ENGINE.md `:4037-4038` list the
  `actor_type` readers as **rows `04`/`06`/`07`/`18`**. Fresh this slice, **row
  `18` does not read `+0x8E`** (`0x849B0..0x84AE1` touches only `+0x28` anim
  pointer, `+0x65`, `+0x63`, `+0x89`, `+0x92`, `+0x9A`, `+0x7B`). The actual
  wired `actor_type` readers are `04`/`06`/`07` (+ the shared kick binder and
  rows `0B`/`21`). Recommend the controller correct the reader list on
  promotion.
* **Widening (cross-ref FU-252 W24-5 §5.1/L4):** W24-5 owed a full `+0x8E`
  writer census. This slice supplies it (§3.2): 9 direct byte stores across
  `FUN_0006E8E8`(2), `FUN_00079C50`(2), `FUN_0007BF20`(1), row `0B`(1),
  `FUN_0008E244`(1), `FUN_0008E5BC`(1), `FUN_0008E910`(1), plus the 40 `0x79C50`
  call sites — i.e. **≥9 writer functions**, so the reconciliation must repoint
  readers to a byte with many producers (unification is the only honest option).

---

## 4. The OL-19 / OL-22 / OL-34 metadata legs (fresh definitions)

These are *not* in `docs/ENGINE.md` (they live in their FU reports); recorded
here as the task asks, with current status.

* **OL-19 — FU-138 row `03` indirect pieces.** Definition (FU-138 §6, lines
  175-178): the `[rec+0x1C]` phase-handler call (`0x7E252`), the camera
  placement `FUN_00079F3C` (`0x7E3A8`) and the phase-7 ball scan (`0x7E418`)
  are unported (FU-77 §7 legs 1/5). **Status: partially closed by FU-177**
  (phase-15 wave R25): FU-177 §4 finds `FUN_00079F3C` **already ported**
  (`fifa96_match_entities_camera_place`, FU-96 leg 5) — the OL-19 camera item is
  stale; the residual OL-19 pieces are the `[rec+0x1C]` call (two sites) and the
  `FUN_00079C20` slot dir step (`0x7E25F..0x7E27C`, FU-177 §5/§6) plus the
  phase-7 ball scan (`0x7E418..0x7E4BB`). Row `03` stays unwired.
* **OL-22 — FU-138 rows `09`/`0A`/`0B`/`0C`/`0D`/`0E` stage/sequence arms.**
  Definition (FU-138 §6): the `CS:` arm bodies, the `0x14E04` angle→vector fold,
  `FUN_00079C50`/`FUN_000702F8`/`FUN_0006E598` and the nearest searches are
  unported (FU-81 §2.1, FU-82 §3.3/3.4). Engine: `fifa96_match_action_09..0E`
  are absent from the dispatch table → these rows dispatch `-FIFA96_ERR_UNSUPPORTED`
  (ENGINE.md `:4014-4021`, the 24 unwired action rows). **Status: open.**
* **OL-34 — FU-140 rows `1A`/`1B` stage bodies.** Definition (FU-140 §6, line
  263): `FUN_000765C4` tracked-teammate predicate, the `[0x157C2C]` direction
  machine (`0x766FC..0x767E7`), the common movement/event block
  (`0x76816..0x76AA7`), the 1B stage-2 rush test (`0x76FC7..0x77636`, per-side
  `[0x4C2F6]` thresholds) and the stage 3/4 finish side-effects are unported
  (FU-79 §3/§4, §14 leg 4). The row-`1A`/`1B` gates (`reposition_a_gate`/
  `reposition_b_finish`) are ported but the bodies are not wired. **Status:
  open.**

All three are **doc-only metadata legs** (no engine consumer reachable until
their rows are wired); re-deriving them instruction-level is out of this slice's
scope (the task asks for the fresh definitions, which is what §4 records).

---

## 5. Verdict

Row `08` is **fully mapped**: its unmodeled surface (OL-82) is a closed,
producer-attributable set (§2) and the one live gate (`+0x3D`) is already fed by
the OL-80 `record.frame` staging. The OL-83 wrinkle is a **naming/split bug**,
not a missing producer: `type` and `actor_type` are the same native `+0x8E`
byte; the correction is to unify on `record.type`. The OL-19/OL-22/OL-34 legs are
carried metadata; only OL-19 has partial progress (FU-177). **Census impact:
none** — row 08 is already wired, no action row changes state, so the
README/ENGINE "wired/total" count is unchanged.

---

## 6. Numbered legs (owners + next evidence)

* **FU253-L1 (OL-82 producer closure, owner: row-08/loader port).** Bind the
  row-08 zero-staged inputs to their named producers: `[0x15877D]`/`[0x15872F]`
  (need a derived home), `[0x1577C0/C2]` (the FU-252 `fifa96_camera_track`
  family), `[0x15774C]` (the shared focus triple), `[0x157750]` (ball height),
  `[0x157A83]`/`[0x1577CA]` (pool `controlled` stand-ins already), the `rec[+4]`
  descriptor bytes `+0xC`/`+0x16` (+ opp `+0xC`/`+0xF`). Evidence: §2.2-§2.3.
* **FU253-L2 (OL-82 sink closure, owners: FU-254 for `0x6E598`; FU-142 §L/§K
  for the events).** Consume `out.anim` (`0x6E598`, OL-52), `out.ball_stage`
  (`0x7A490`, OL-62), and `out.events`/`event_code`/`event_sound`
  (`0x8ED40`/`0x8F188`/`0x92820`/`0x974F0`/`0x651F0`, OL-82/OL-27). Evidence:
  §2.4, `get_xrefs_to` counts.
* **FU253-L3 (OL-83 ruling — MUST be applied before any reader is re-wired).**
  Unify `actor_type` → `record.type`; delete the alias or write both on every
  writer; correct the FU-142 §6 / ENGINE.md reader list (drop row `18`). Evidence:
  §3.
* **FU253-L4 (`+0x8E` writer census completeness).** `FUN_0006E8E8`'s
  `+0x8E` write (§3.2) is a 3-bit *cycle*, not the face octant — decide whether
  its base is the match record or an anim descriptor (owner: FU-254/W24-7
  anim-sink slice). Evidence: fresh `0x6F8F8..0x6F945`.
* **FU253-L5 (OL-19 remainder).** Re-derive the `[rec+0x1C]` call sites,
  `FUN_00079C20` slot step and the phase-7 ball scan (FU-177 §5/§6 already
  sketches them; owner: row-03 slice). Evidence: FU-177.
* **FU253-L6 (OL-22 / OL-34).** Rows `09..0E` (OL-22) and keeper `1A`/`1B`
  (OL-34) remain unported; carry as-is (owners FU-138 §6 / FU-140 §6).

---

## 7. Port shapes (fidelity)

* **Row `08`.** The ported `fifa96_outfield_row08_step` and the engine binder
  `fifa96_match_action_08` are **shape-correct**; the only fidelity work is
  FU253-L1/L2 (bind the named producers/consumers). No new step semantics.
  Keep `record.type` as the `+0x8E` cell (§3).
* **OL-83.** One C field. Repoint `s.type8 = r->actor_type` (rows 04/06/07, the
  kick binder, rows 0B/21) to `r->type`; delete `actor_type` (or make it a
  `#define` alias). Do not add a second writer. This makes the row-08/28/2A face
  write visible to the wired readers exactly as the native shared byte is.
* **OL-19/OL-22/OL-34.** Metadata-only; no port shape until the rows wire
  (unchanged from their FU reports).

---

## 8. Hand-off

Draft only. No engine/doc/code/test edits made; `/FIFA96.EXE` untouched. Promote
to `docs/ghidra/FU253_row08_fields.md`; the OL-83 ruling (§3.3) and the reader-list
erratum (row `18`) are the controller's call at reconciliation, jointly with
W24-5's FU252-L4 (which this slice completes with the §3.2 writer census).
Concerns: (1) `FUN_0006E8E8`'s `+0x8E` write semantics/base (FU253-L4);
(2) the OL-19/OL-22/OL-34 legs are inherited definitions, not re-derived this
slice (§4). No census change.
