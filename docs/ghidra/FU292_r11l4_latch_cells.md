# FU292 — the latch + geometry cells (the ladder's stage-select inputs)

**Phase/Provenance:** M2 phase-27 recon-ahead, Task 3 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (case-sensitive; confirmed this session by
`ghidra_get_current_program_info`: `watcom:LE:32:default`, image base 0, `.image`
overlay, 2706 functions). Read-only: no rename/comment/label/create/script/save;
`/FIFA96.EXE` untouched; ISO untouched; no git. Every native address/byte below is
a **fresh** `disassemble_bytes` / `read_memory` / `get_xrefs_to` of *this* session
(FU283 §1.4 and FU273 §1.3 are cited as *carried*). Alias traps applied
(`dword[x]>>0x18` = byte `x+3`, `dword[x]>>0x10` = word `x+2`, e.g.
`dword[0x1577f8]>>0x10` = word `0x1577fa`; `dword[0x157c4c]` is the middle dword of
the `0x157c48` vector; CALL rel32 verified by hand). All ten target addresses read
as `00…00` in the static image (fresh `read_memory`), i.e. they are
**runtime-populated** globals with no image initializer — so every value domain
below is derived **from the write sites only**.

## The two cell families

The ladder `FUN_00077EAC` (the `0x1A`/`0x1B`/`0x1C` installer, FU283 §1.4) computes
its stage index `CX ∈ {1..5}` from two families of cells:

* **The "keeper target / band" cell block `0x157C44..0x157C50`** — a scalar
  `0x157C44` plus a 3-dword vector based at `0x157C48` (`…48/4C/50`), plus the
  band-direction code `0x157C2C` and the ladder byte-latch `0x157C5C`.
* **The camera-band word block `0x1577F0..0x1577FA`** — three word pairs
  (`F0/F2/F4`, `F6/F8/FA`) that the ladder reads as *dwords shifted right 16*
  (`>>0x10` lands on `F6`/`FA`), plus the packed cell `0x157821` and the global
  gate dword `0x158740`.

## Per-cell derivation

Legend: **INPUT** = read by `FUN_00077EAC` (or by `FUN_000765C4`, which the ladder
calls at `0x781EC`) to select/emit a stage; **PRODUCT** = written by the ladder
itself and consumed downstream (row `0x1A` body), not a stage-select input.

| cell | size | writer set (fresh write site / raw bytes) | value domain | class |
|---|---|---|---|---|
| `0x157C44` | dword | `0x748b6 MOV dword[0x157c44],0xffffffff` (`c705447c1500ffffffff`); `0x74aaf MOV [0x157c44],EBX` — both in an **unrecovered routine** (no Ghidra function at `0x748b6`/`0x74aaf`) | `-1` sentinel, or EBX (a scalar threshold) | **INPUT** (ladder `0x7810e` reads it, compares to the stage word) |
| `0x157C48` (+`4C`,`50`) | 3×dword | `0x74aaa MOV EDI,0x157c48` then `0x74ab5 MOVSD ×3` (`a5 a5 a5`) from `[ESP+0xC]` — same unrecovered routine | 3-dword 16.16 position `(x,y,z)` | **INPUT** (ladder LEA `0x77f86` → arg to `0x795B4`; `0x781d4/…de` copies it to rec `+0x4D`) |
| `0x157C4C` | dword | 3rd dword of the `0x157C48` copy (`0x74ab6 MOVSD`, `a5`) | 16.16 Y component; ladder tests `≥ 0x100` | **INPUT** (`0x780e6 read`; `0x781c1 CMP dword[0x157c4c],0x100`) |
| `0x157C2C` | dword | ladder arms `0x78230 MOV [0x157c2c],ESI(=0x2e)` / `0x78258 ...,EDI(=0x2d)` / `0x7827e ...,EAX(=0x2c)` (`89352c7c1500` family); row-`0x1A` body `0x7674c MOV [0x157c2c],0x5c` (`89352c7c1500`) and `0x767a9 MOV [0x157c2c],EDX` (`89152c7c1500`) | small band/step code ∈ {`0x2c`,`0x2d`,`0x2e`,`0x54`,`0x5c`} | **PRODUCT** (ladder writes it; row-`0x1A` body reads it at `0x766fc`,`0x76757`,`0x767e7`,`0x76960`,`0x76a12`) |
| `0x157C5C` | byte | ladder `0x7802d MOV [0x157c5c],AL(=0)` (`a25c7c1500`) / `0x780d7 MOV byte[0x157c5c],BH(=1)` (`883d5c7c1500`); `FUN_00076130` `0x76318 MOV byte[0x157c5c],BH`; `0x763ab MOV byte[0x157c5c],DH(=0)` (`88355c7c1500`); row-`0x1A` `0x77003 MOV byte[0x157c5c],DH` | {0,1} boolean latch | **PRODUCT** (ladder writes it; only *reader* is `0x76ff4 MOV BL,[0x157c5c]` — distinguishes the camera-copy vs ball-copy path) |
| `0x1577F4` | word | `0x70167 MOV word[0x1577f4],CX(=0)` (`66890df4771500`); `0x7059a MOV [0x1577f4],AX` (`66a3f4771500`, AX = doubled scaled delta); `0x7060c MOV word[0x1577f4],DI(=0xc)` (`66893df4771500`) — init `FUN_000700f4`, recompute `FUN_00070544` | camera-band extent word (0, 0xc fallback, or computed) | **INPUT** (ladder `0x78163`/`0x78180`; also read by `FUN_000736ac`,`0x70544`,`0x77d33`) |
| `0x1577F8` | word | `0x70187 MOV word[0x1577f8],DI(=0)` (`66893df8771500`); `0x705e8 MOV [0x1577f8],AX` (`66a3f8771500`, AX = `[0x1577f4]-word[0x1577f6]`); `0x70613 MOV word[0x1577f8],DI(=0xc)` (`66893df8771500`) | band top/bottom extent word | **INPUT** (ladder `0x7815b`/`0x7817b`; 20 refs total) |
| `0x1577BC` | word | `0x707dc MOV [0x1577bc],AX` (`66a3bc771500`, `IMUL`-scaled); `0x701fd MOV word[0x1577bc],DI(=0)` (`66893dbc771500`); `0x71cc9 MOVSW` (`66a5`) | camera-band Z/extent word | **INPUT** (ladder `0x78046`; row-`0x1A` `0x76764`) |
| `0x157821` | dword (packed) | `0x70153 MOV byte[0x157821],AH(=0)` (`882521781500`) sets byte0; `0x70a65 INC byte[0x157821]` (in `FUN_000709d0`, `fe0521781500`) | byte0 = counter/flag; byte3 (`0x157824`) = side id | **INPUT** (ladder `0x78144 CMP byte[0x157821],0`; `0x7812e dword>>0x18` = byte `0x157824` compared to the record side) |
| `0x158740` | word+packed | `0x79ae7 MOV word[0x158740],DX` (`66891540871500`); `0x7a055 MOV word[0x158740],DX`; `0x7a17b MOV [0x158740],AX` (`66a340871500`); `0x7a6e8 MOV [0x158740],AX`; `0x7a9af MOV word[0x158740],CX` (`66890d40871500`); mode byte `0x158743 MOV …,DL(=2)` (`881543871500`), `0x158742 MOV …,DH(=0x20)` (`883542871500`) | word value + mode byte (`0x158743`) | **INDIRECT INPUT** (read by the ladder's predicate `FUN_000765C4` at `0x765fa MOV EAX,[0x158740]; SAR 0x18; CMP 4` → byte `0x158743`; gate returns 0 when `==4`) |

## The `[0x157C5D]` gate vs `[0x157C5C]` latch distinction (carried FU273 §1.3, restated)

* **`0x157C5D`** is the *interlock gate*: the sole setter is `0x74707`
  (`MOV byte[0x157C5D],AH`) in the row-`0x19` keeper-hold body; `FUN_000782D0`
  reads it at `0x7858E` (`CMP byte[0x157C5D],0`), calls the ladder once, then
  clears it at `0x785A2` (`MOV byte[0x157C5D],CH`). Carried (FU273 §1.3), *not*
  re-verified this pass.
* **`0x157C5C`** is the *ladder-internal latch* — a **different cell**, written by
  the ladder (`0x7802D`/`0x780D7`) and `FUN_00076130` (`0x76318`) and the row-`0x1A`
  body (`0x763AB`/`0x77003`), read only at `0x76FF4`. Fresh `get_xrefs_to 0x157C5C`
  = **6**, matching FU273 §1.3. The two must never be conflated.

## Evidence table (all fresh, `/FIFA96.EXE`, read-only)

| claim | fresh evidence |
|---|---|
| `0x157C4C` refs = 4 (reads `0x780e6`,`0x781c1`,`0x781df`; write `0x74ab6`) | `get_xrefs_to 0x157C4C` |
| `0x157C48` refs = 5 (`0x77f86` PARAM, `0x781de` READ, `0x781d4` DATA, `0x74ab5` WRITE, `0x74aaa` DATA) | `get_xrefs_to 0x157C48` |
| `0x157C44` refs = 3 (read `0x7810e`; writes `0x748b6`,`0x74aaf`) | `get_xrefs_to 0x157C44` |
| `0x157C2C` refs = 10 (ladder writes `0x78230/58/7e`; row-`0x1A` `0x7674c`/`0x767a9`; reads `0x766fc`/`0x76757`/`0x767e7`/`0x76960`/`0x76a12`) | `get_xrefs_to 0x157C2C` |
| `0x157C5C` refs = 6 (`0x7802d`,`0x780d7`,`0x76318`,`0x77003`,`0x763ab`, read `0x76ff4`) | `get_xrefs_to 0x157C5C` |
| `0x1577F4` refs = 12 (incl. ladder `0x78163`,`0x78180`) | `get_xrefs_to 0x1577F4` |
| `0x1577F8` refs = 20 (incl. ladder `0x7815b`,`0x7817b`) | `get_xrefs_to 0x1577F8` |
| `0x1577BC` refs = 21 (incl. ladder `0x78046`, row-`0x1A` `0x76764`) | `get_xrefs_to 0x1577BC` |
| `0x157821` refs = 11 (write `0x70153`; read-write `0x70a65`; ladder `0x7812e`,`0x78144`) | `get_xrefs_to 0x157821` |
| `0x158740` refs = 23 (writers `0x79ae7`,`0x7a055`,`0x7a17b`,`0x7a6e8`,`0x7a9af`; predicate read `0x765fa`) | `get_xrefs_to 0x158740` |
| ladder band/gate machine reads/writes `0x78020..0x781af` (latch `0x157c5c`=0 at `0x7802d`; `0x1577bc` gate `0x78046`; camera-copy + `0x157c5c`=1 at `0x780a5..0x780d7`; `0x157c4c` `0x780e6`/`0x781c1`; `0x157821` `0x7812e`/`0x78144`; `0x1577f8`/`0x1577f4` `0x7815b..0x78193`) | `disassemble_bytes 0x78000` |
| ladder tail dispatcher `0x781e1..0x7820f` (`TEST CX,CX`; `CALL 0x765c4`; code-4) | `disassemble_bytes 0x781AF` |
| `0x157C48`/`0x157C4C` writer = unrecovered routine: `0x74aaa LEA EDI,0x157c48`; `0x74aaf MOV [0x157c44],EBX`; `0x74ab5 MOVSD×3`; side test `0x74a8c dword[0x157821]>>0x18`; `0x74a77 CMP EAX,0x990` | `disassemble_bytes 0x74A70`; `get_function_by_address 0x74ab5` = "No function found" |
| `0x157C44` sentinel write `0x748b6 MOV dword[0x157c44],0xffffffff` | `disassemble_bytes 0x748B0` |
| `FUN_000700f4` init: `0x70153 byte[0x157821]:=0`; `0x70167 word[0x1577f4]:=0`; `0x70187 word[0x1577f8]:=0`; `0x701fd word[0x1577bc]:=0` | `disassemble_bytes 0x70140` |
| `FUN_00070544` recompute: `0x7059a word[0x1577f4]:=AX`; `0x705e8 word[0x1577f8]:=AX`; `0x7060c`/`0x70613` fallback `:=0xc` | `disassemble_bytes 0x70580` |
| `0x1577bc` scaled write `0x707dc IMUL`; `MOVSW` at `0x71cc9` | `disassemble_bytes 0x707C0`, `0x71CC0` |
| `0x157821` counter `0x70a65 INC byte[0x157821]` (in `FUN_000709d0`) | `disassemble_bytes 0x70A40` |
| `0x158740` word writes (`66891540871500`/`66a340871500`/`66890d40871500`) + mode byte `0x158743:=2` | `disassemble_bytes 0x79AE0`,`0x7A04D`,`0x7A173`,`0x7A6E0`,`0x7A9A7` |
| ladder predicate `FUN_000765C4` gate: `0x765fa MOV EAX,[0x158740]; SAR 0x18; CMP 4` | `disassemble_bytes 0x765E8`; `get_xrefs_to 0x158740` |
| all ten cells are image zeros (runtime-populated, no initializer) | `read_memory 0x1577F0`, `0x157820`, `0x157C40` |

## Numbered open legs (with owners)

* **FU292-L1 — the `0x157C44`/`0x157C48` writer routine is unrecovered.**
  `get_function_by_address 0x74ab5` = "No function found"; the enclosing routine
  (`0x748b0..0x74acc`) that sets `[0x157c44] = -1` / `EBX` and copies the 3-dword
  target to `0x157C48` (+ to `[ecx+0x4D]`) has no analysed bounds, so its caller
  set and the EBX source are unproven. Owner: FU186-L5 / FU180-L4 (the ladder's
  driver family).
* **FU292-L2 — the `0x1577F0..0x1577FA` camera-band struct semantics.** The six
  words (`F0/F2/F4` and `F6/F8/FA`) are written by `FUN_000700f4` (init) and
  `FUN_00070544` (recompute from `0x702f8`-scaled deltas); the physical meaning
  (camera near/far band vs ball-height band) is not established, only the write
  sites. Owner: FU180-L1/L3.
* **FU292-L3 — `0x157821` packed-cell field split.** Byte0 is a counter/flag
  (`INC` at `0x70a65`), byte3 (`0x157824`) is a side id compared to the record
  side; the write of byte3 is **not** among the fresh xrefs (only byte0 writes at
  `0x70153`/`0x70a65` appear), so the byte3 producer is unproven. Owner: FU186-L5.
* **FU292-L4 — `0x158740` mode byte `0x158743` domain.** Only the values `2`
  (write `0x7a060`) and the predicate comparison `4` (read `0x76602`) are pinned;
  the full mode alphabet and which producer sets `4` are open. Owner: FU180-L4.
* **FU292-L5 — `0x157C2C` full code alphabet.** The ladder arms write
  `0x2c/0x2d/0x2e` and the row-`0x1A` body writes `0x5c` (and an `EDX` value that
  includes `0x54` on the `0x7679f` path); the complete enumeration and the
  `0x767a9` EDX provenance are not proven. Owner: FU186-L3/L5.
* **FU292-L6 — `0x157C5C` latch consumers.** The only reader is `0x76ff4` in the
  row-`0x1A` body; the *behaviour* it gates (camera-copy vs ball-copy install) is
  inferred from the writer sites (`0x7802d`=0 / `0x780d7`=1), not from a
  decompiled row-`0x1A` body. Owner: FU183/FU186-L3.

**Carried (read, not rewritten):** FU283 §1.4, FU273 §1.3.
