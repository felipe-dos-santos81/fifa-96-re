# FU-330 — the native placement-limit producer `+0x79` (phase-35 R2-L9)

**Phase/Provenance:** phase-35 recon-ahead wave, Stage-1 **slice 1 of 4 — the
`+0x79` placement-limit producer census**
(`docs/superpowers/specs/2026-10-23-fifa96-m2-phase35-placement-timer-design.md`
§3/§4, §9 item 1; the plan's Task 1), 2026-10-23, read-only. Fresh Ghidra this
session: `/FIFA96.EXE`, `watcom:LE:32:default`, base 0, `.image`; standard MCP
tools only; no scripts, no saves, no writes to the project. Carried as named:
FU84 §6 (the `FUN_0007997C` / `0x10F394` material), FU328 §4.3/§5.6 and its
erratum 2 (the `+0x7B` counter chain and the "6..15 is not a proven cap"
flag), FU328-L1/L3. Scratch lives under `/tmp/opencode/fu330/` (unused beyond
this doc's working notes). **No repo file was created or changed except this
draft; no git; the ISO read-only; `build/`, `tests/golden/` and `fifa96.rep/`
untouched; census `60/80 = 75%` unchanged.**

## 1. The question

> What produces the native placement limit `word[rec+0x79]` — instruction by
> instruction, from which table with which index, with what OR, at what point
> in the frame chain — and is the "6..15" range seen by the phase-34 wave a
> **proven cap** on the register? What are the per-record values at kickoff for
> all rows and both teams, on what basis, and who reads the word?

## 2. The one-page answer

**The producer is a 32-byte per-player tier table, not a timer.** `+0x79` is
written by three sites (six stores) and read by eight load sites:

1. **The record reset** `FUN_0007997C` (`0x7997C..0x79B19`; fresh
   `get_function_by_address`, one caller `0x8C34A` in `FUN_0008C33C`,
   fresh `get_xrefs_to` = 1) at `0x79A4D`:
   `word[rec+0x79] = byte[0x10F394 + (byte[[rec+4]+0xD])] | byte[rec+0x9D]`
   (index sign-extended then `XOR BH,BH`, so `0x80..0xFF` wraps to
   `0x0FF394+b`; **no bounds check**). The same function copies
   `+0x79 → +0x7B` eleven instructions later (`0x79A86`).
2. **The lineup rebind** `FUN_0008C758` (`0x8C850`), the same table + OR
   expression, after it re-points `rec+4` at the current lineup occupant and
   recomputes `+0x9D`; it then halves `+0x79` with floor **2** when the
   roster `+0x99` flag is set (`0x8C92D..0x8C942`).
3. **The no-replacement (row-12) degradation** at `0x822DB..0x822F0`
   (handler `0x81C90`, fresh handler-table xref `0x110710` = slot 12):
   `+0x79 = max(+0x79 >> 1, 3)`.

**Is 6..15 a cap?** **No, and the wave is right — but not for the reason the
erratum implies.** For an in-range table index `0..31` the producer is
**6..15**, and **15 *is* a hard maximum** there: the table's largest byte is
`0x0F`, and the OR's only producer emits `0x00` or `0x0F`, so `t | 0x9D ≤ 15`.
The range fails as a *cap* for two independent reasons: (a) the index is
**unguarded** — `>= 0x20` walks into the `0x10F3B4` table and the `0x10F3C4`
word ramp (`0x10`…`0x40`, then `0x00/0x15/0x2B/…/0xCB`), and `>= 0x80` wraps
to `0x0FF414..0x0FF493` — so no universal upper bound is proven; (b) `6` is
not a floor of the *register*: producers 2/3 lower it to `3`, and `3 → 2` on
composition (`3>>1 = 1 → floor 2`). **The proven live domain is `2..15` only
under the unproven assumption `player_d ≤ 31`.**

**Kickoff values (basis stated, not guessed).** The index byte is per-player
roster data (`0x14C360[team][slot] → +0xD`), **not** statically present in the
image: the port stages `player_d = 0` for every record
(`fifa96_match_run.c:1443`), so a faithful producer yields **`+0x79 = 6` on all
22 records** at kickoff, with `+0x9D = 0` (the pool at `0x1588A4` is BSS: read
fresh, 64 zero bytes). The **native** per-record values are unmeasured
(FU328-L3 restated as FU330-L1).

**Timing.** The store runs in the **match-start / kickoff / period-transition
prelude**, not per frame: `FUN_0007417C` (fresh 4 callers: `FUN_00092E2C`
match start, `FUN_0004B02C` `param==1` kickoff, `FUN_00088860` phase-`0x0C`
transition, `0x8B832` state-reset region) → `FUN_0008C33C` on **both** pools
`0x1588A4`/`0x1590D9` → 11 × `FUN_0007997C`. It precedes the `+0x7B` copy and
the first state install, so the first row-03/row-09 pass of the new phase
already sees the produced value.

## 3. The producer census (fresh, this session)

| # | site | function | kind | value written | when |
|---|---|---|---|---|---|
| P1 | `0x79A4D` | `FUN_0007997C` | `+0x79` | `byte[0x10F394+idx] \| byte[+0x9D]` | record placement reset (match start / kickoff / period transition), 11 records × 2 pools |
| P1b | `0x79A86` | `FUN_0007997C` | `+0x7B` | `+0x79` (verbatim) | same call, ~11 insns later |
| P2 | `0x8C850` | `FUN_0008C758` | `+0x79` | `byte[0x10F394+idx] \| byte[+0x9D]` | lineup rebind (substitution / rebind path) |
| P2b | `0x8C939` / `0x8C942` | `FUN_0008C758` | `+0x79` | `+0x79>>1`, floored at **2**, iff `+0x99 != 0` | same call |
| P3 | `0x822E7` / `0x822F0` | handler `0x81C90` (row 12) | `+0x79` | `max(+0x79>>1, **3**)` | the no-replacement arm (`EAX == -1`) |

Reader census (fresh `search_instructions operand_pattern "0x79"`, 32 matches
over 235 070 instructions, `truncated: false`). Of the 32, **14 are the
`[reg+0x79]` record sites**: **6 stores** (P1 `0x79A4D`; P3 `0x822E7` and
`0x822F0`; P2 `0x8C850`, `0x8C939`, `0x8C942`) and **8 loads** (R1–R7 plus
`0x822DB`, the load half of P3). The other 18 are immediates
(`MOV EDX,0x79`, `CMP EAX,0x79`, the `0x79970` literal, …).

| # | site | function | expression | note |
|---|---|---|---|---|
| R1 | `0x7836A` | `FUN_000782D0` (record-0/keeper machine) | `+0x7B := +0x79` | per-frame, unconditional (`0x7836A`/`0x78374`) |
| R2 | `0x79A7C` | `FUN_0007997C` | `+0x7B := +0x79` | P1b, same call |
| R3 | `0x7DA9B` | `FUN_0007D9A4` (state installer) | `+0x7B := +0x79` | every state install; port `entities.c:94` |
| R4 | `0x7E274` | row-03 handler `0x7E1A4` (handler-table slot 3) | `+0x7B := +0x79` | the row-03 "reposition" arm (`CALL 0x79C20` first) |
| R5 | `0x7E2E7` | row-03 handler `0x7E1A4` | `min(([+0x63]>>22)+1, +0x79)` | the phase-34 counter, FU328 §4.3 — fresh bytes match exactly |
| R6 | `0x80B3D` | row-09 handler `0x80A00` (handler-table slot 9) | `+0x7B := +0x79 * 2` | then `+0x73 := mul(+0x7B, sin)`; the port's OL-82 site |
| R7 | `0x8C92D` | `FUN_0008C758` | `+0x79 >>= 1` (floor 2) | P2b |

Five reader sites sit outside the producing functions (R1, R3–R6) — `+0x79`
itself is a **stored record property**, re-derived only at the three producer
events, never recomputed per pass.

### 3.1 P1 — `FUN_0007997C`, instruction level (fresh `disassemble_bytes 0x7997C`, 416 B)

```
0x79983  e8983fffff      CALL 0x6D920(rec)                  ; EDX out (position source)
0x7998a  7410            JZ  -> 0x7999C                     ; else copy EDX→+0x59 triple
0x79991  a5 a5 a5        MOVSD ×3   [+0x59] <- [EDX]        ; then [+0x4D] <- [+0x59]
0x7999f  ...             else: [rec+0x1C](rec, rec+0x4D, -1) ; vtable position fetch
0x799af  a5 a5 a5        MOVSD ×3   [+0x4D] <- [+0x59]
0x799bf  c7452800000000  MOV dword[rec+0x28], 0
0x799c6  e8a1010000      CALL 0x79B6C(rec, rec+0x59, [0x157A73])
0x799d5  e8dafbffff      CALL 0x795B4(rec?, 0x15774C, rec+0x6B)
0x799da  66c745750000    MOV word[rec+0x75], 0           ; velocity z
0x799e0  66c785830000000000  MOV word[rec+0x83], 0
0x799e9  66c785850000000000  MOV word[rec+0x85], 0
0x799f2  66c785810000000000  MOV word[rec+0x81], 0
0x799fb  66c785870000000000  MOV word[rec+0x87], 0
0x79a04  c7858900000000000000 MOV dword[rec+0x89], 0
0x79a0e  668b556b        MOV DX, word[rec+0x6B]
0x79a12  c6859100000000  MOV byte[rec+0x91], 0
0x79a19  66895577        MOV word[rec+0x77], DX          ; +0x6B → +0x77
0x79a1d  668b5575        MOV DX, word[rec+0x75]
0x79a21  66895573        MOV word[rec+0x73], DX          ; = 0
0x79a25  66895571        MOV word[rec+0x71], DX          ; = 0
0x79a29  8b5504          MOV EDX, [rec+0x4]              ; the player descriptor
0x79a2c  c6859200000000  MOV byte[rec+0x92], 0
0x79a33  8b5a0a          MOV EBX, [EDX+0xA]
0x79a36  8b15387a1500    MOV EDX, [0x157A38]             ; = 0x10F394 (FUN_00073CD0)
0x79a3c  c1fb18          SAR EBX, 0x18                   ; byte at [[rec+4]+0xD]
0x79a3f  01da            ADD EDX, EBX                    ; + base  (no range check)
0x79a41  30ff            XOR BH, BH                      ; *** clears bits 8..15 only
0x79a43  8a1a            MOV BL, byte[EDX]               ; table byte
0x79a45  8a959d000000    MOV DL, byte[rec+0x9D]          ; the per-record flag
0x79a4b  08d3            OR BL, DL                       ; *** BL | +0x9D
0x79a4d  66895d79        MOV word[rec+0x79], BX          ; *** THE STORE
0x79a51  88bd93000000    MOV byte[rec+0x93], BH          ; = 0
0x79a57  88bd99000000    MOV byte[rec+0x99], BH          ; = 0
0x79a5f  88bd98000000    MOV byte[rec+0x98], BH          ; = 0
0x79a65  668b9594000000  MOV DX, word[rec+0x94]
0x79a6c  88bd9b000000    MOV byte[rec+0x9B], BH          ; = 0
0x79a72  c1fa04          SAR EDX, 4
0x79a75  66899596000000  MOV word[rec+0x96], DX          ; +0x96 = +0x94 >> 4
0x79a7c  668b5579        MOV DX, word[rec+0x79]
0x79a80  8aa59a000000    MOV AH, byte[rec+0x9A]
0x79a86  6689557b        MOV word[rec+0x7B], DX          ; *** the +0x7B copy
0x79a8c  7523            JNZ 0x79AB1                     ; skip install if +0x9A != 0
0x79a8e  80bd8d00000000  CMP byte[rec+0x8D], 0
0x79a97  ba19000000      EDX = (+0x8D == 0 ? 0x19 : 3)
0x79aac  e8f33e0000      CALL 0x7D9A4(rec, EDX, 0, 0)    ; install (re-copies +0x7B)
0x79ab1  3b2d30871500    CMP EBP, [0x158730]             ; the focus/controlled record
0x79ab7  755b            JNZ 0x79B14                     ; else -> epilogue
            ...           clears the [0x158730..0x158746] focus block
0x79b19  c3              RET
```

Three consequences the carried docs do not state:

* **The high byte of the store is forced to 0** (`XOR BH,BH` before the table
  load, never re-set), so `+0x79` is an 8-bit value in a word cell.
* **`+0x9D` is read, never cleared** here. P1 therefore *consumes* a flag that
  only P2 writes; a record reset does not reset the "special" attribute.
* **The index is not range-checked.** `SAR EBX,0x18` sign-extends the
  descriptor byte; `XOR BH,BH` then clears bits 8–15 **but not 16–31**, so a
  byte `0x80..0xFF` becomes `b − 0x10000` and the read lands at
  `0x0FF394 + b` (`0x0FF414..0x0FF493`), not at the table. Indices `0x20..0x7F`
  read `0x10F3B4..0x10F413` — the `+0x81` byte table, the `0x10F3C4` word ramp
  and past it. The port's `int8_t player_d` + `0..31` guard
  (`fifa96_arm_bodies.c:48`) models the *intent*, not the *hardware*.

### 3.2 P2 — `FUN_0008C758` (fresh `disassemble_bytes 0x8C758`, 0xB0 B + `0x8C800` window)

```
0x8C769  CALL 0x741B4(team)                    ; team side from [rec+0]+0x826
0x8C78A  EAX = [0x157BD2 + team*0x2C + slot*4]  ; slot = byte[rec+0x8D]
0x8C791  MOV [rec+0x4], EAX                    ; *** rec+4 := the lineup occupant
0x8C7A6  AX = word[0x14C1D4 + team*2]           ; per-team flag word
0x8C7B2  if (flags & 0x08)                     -> +0x9D = 0x0F
0x8C7CC  else if (flags & 0x20) && byte[rec+0x90] == 0  -> +0x9D = 0x0F
0x8C7EC  else if (flags & 0x80) && byte[rec+0x90] == 1  -> +0x9D = 0x0F
0x8C80F  else if (flags & 0x40) && byte[rec+0x90] == 3  -> +0x9D = 0x0F
0x8C82D  else                                  -> +0x9D = 0
0x8C834  EAX = [[rec+4]+0xA]; SAR 0x18
0x8C83A  EBX = [0x157A38]
0x8C843  AL  = byte[EBX+EAX]                   ; (note: plain signed index here —
0x8C846  BL  = byte[rec+0x9D]                  ;  no XOR BH,BH, so 0x80..0xFF is
0x8C84E  AL |= BL                              ;  a *negative* in-image index)
0x8C850  MOV word[rec+0x79], AX                ; *** P2
0x8C922  word[rec+0x96] = word[rec+0x94] >> 4
0x8C92D  if (byte[rec+0x99] != 0) {
0x8C939      word[rec+0x79] >>= 1;
0x8C942      if (word[rec+0x79] < 2) word[rec+0x79] = 2;
0x8C948      byte[rec+0x93] = 0; }
```

P2 is reached only from lineup-change paths — fresh `get_xrefs_to 0x8C758` = 7:
`0x8CACC`/`0x8CAD3` (`FUN_0008C974`, the swap/sort), `0x8CE34`/`0x8CE41`
(`FUN_0008CBB0`, the substitution re-apply), `0x8CE94` (`FUN_0008CE78`),
`0x93575` (`FUN_0009343C`, the bench→XI applier) and `0x848DF` (a lineup swap
in an undefined region). **None of the seven is in `FUN_0004B02C`'s
`param == 1` arm**, so on the first kickoff `+0x9D` is still the BSS zero.

### 3.3 P3 — the row-12 no-replacement arm (`0x822DB..0x822F0`)

```
0x822DB  668b5579   MOV DX, word[rec+0x79]
0x822DF  31c0       XOR EAX, EAX
0x822E1  66d1ea     SHR DX, 1
0x822E4  6689d0     MOV AX, DX
0x822E7  66895579   MOV word[rec+0x79], DX        ; >>= 1
0x822EB  83f803     CMP EAX, 3
0x822EE  7d06       JGE 0x822F6
0x822F0  66c745790300 MOV word[rec+0x79], 3       ; floor 3
0x822F6  bb04000000 MOV EBX, 4
0x822FB  b861000000 MOV EAX, 0x61                ; animation id 0x61
0x82302  c68593000000 MOV byte[rec+0x93], 0
0x82309  e87ace0000 CALL 0x8F188                 ; install anim 0x61
```

Reached from the branch where the bench search returned `EAX == -1`
(`0x821C8 JZ 0x822DB`, `0x82133 CMP EAX,-1`): a record leaves the pitch with no
eligible replacement. Handler identity is static: `0x81C90` is
`PTR_LAB_001106E0[12]` (fresh `read_memory 0x1106E0`; the same table also gives
`[3] = 0x7E1A4` and `[9] = 0x80A00`, which is how R4/R5 and R6 were attributed
to rows 03/09).

## 4. The `0x10F394` table (fresh `read_memory 0x10F394`, 96 B)

`FUN_00073CD0` stores `[0x157A38] = 0xF394` (fresh decompile: `DAT_00157a38 =
&DAT_0010f394`, object-relative → flat `0x10F394`; `0x73CEE` is the store).
The 32-byte table, raw (hex, fresh):

```
0x10F394  06 06 06 06 06 06 06 06  07 07 07 07 07 07 07 07
0x10F3A4  0F 0F 0F 0E 0D 0D 0C 0B  0B 0A 09 09 08 07 07 06
0x10F3B4  10 13 16 1A 1D 20 23 26  2A 2D 30 33 36 3A 3D 40   (+0x81 table)
0x10F3C4  00 00 15 00 2B 00 40 00  55 00 6A 00 7F 00 94 00   (word ramp, LE)
0x10F3D4  A8 00 BD 00 D1 00 E5 00  F9 00 0D 01 20 01 34 01
0x10F3E4  47 01 5B 01 6E 01 81 01  94 01 A6 01 B9 01 CB 01
```

Per index `0..31` (the producer's in-range map):

| idx | `0..7` | `8..15` | `16..18` | `19` | `20,21` | `22` | `23,24` | `25` | `26,27` | `28` | `29,30` | `31` |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `+0x79` | **6** | **7** | **15** | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | **6** |

* Range: **6..15**, max 15 at `16..18` (and forced to 15 by the OR), min 6 at
  `0..7` and `31`. The curve is symmetric-ish around the 15 plateau — the
  descriptor byte reads as a **tier**, and `+0x79` as the tier's
  placement/stride limit.
* **This is the same 32-byte table the port already carries**
  (`fifa96_arm_bodies.c:32-37`, `fifa96_arm_26_timer_table`) — the fresh read
  matches it byte for byte. The port's row-26 and phase-0C/code-24 consumers
  are therefore already native-faithful; only the `+0x79` producer is absent.
* **The table has six readers, four of which write other cells** (fresh
  `search_instructions operand_pattern "157a38"` = 7 matches): `0x73CEE`
  (init), `0x6DF78` (phase `0C`: `+0x7B = t>>1`), `0x79A36` (P1),
  `0x8182E` (**`+0x71 = t | +0x9D`** — the same idiom, velocity word,
  not `+0x79`), `0x8653D` (code 24: `+0x7B = t-2`), `0x86716` (row 26:
  `+0x7B = t>>1`) and `0x8C83A` (P2). Only P1/P2 OR in `+0x9D`.

## 5. The `[rec+0x9D]` OR

Fresh writer census (`search_instructions operand_pattern "0x9d"`, 31
matches): the **only** record-struct writer is `FUN_0008C758` —
`0x8C7BC`, `0x8C7DF`, `0x8C804`, `0x8C824` write **`0x0F`**, `0x8C82D` writes
**`0`**. `0x47346` writes a stack buffer (`[ESP+EBX+0x9D]`), not a record; the
other 25 matches are reads.

Therefore `+0x9D ∈ {0, 0x0F}`, and the OR is a **boolean override to 15**:

| `+0x9D` | table `t` | `+0x79` |
|---|---|---|
| `0` | 6..15 | `t` (6..15) |
| `0x0F` | 6..15 | `0x0F` (always 15) |

The override predicate (P2's prologue) is a per-team word AND a per-record
byte: `word[0x14C1D4 + team*2]` bits `0x08`, `0x20`, `0x80`, `0x40` combined
with `byte[rec+0x90] == 0/1/3`. The record byte is read as
`[rec+0x8D]>>24` (the AGENTS alias: the dword at `+0x8D` shifted 24 is the
byte at **`+0x90`**). The *meaning* of the bits and of `+0x90` is not derived
here (FU330-L3); the *mechanics* are pinned.

Two interactions the phase-35 design must respect:

* **P1 never clears `+0x9D`.** A record that was flagged "special" at a rebind
  keeps the flag through every later placement reset, so its `+0x79` is 15
  from then on.
* **The flag is per-record, not per-team**, even though the *qualifier* is a
  team word: the same team word yields 15 for every record whose `+0x90` byte
  matches.

## 6. Values, timing and the per-record kickoff table

### 6.1 The value algebra (all statically proven)

```
fresh producer (idx ≤ 31):          +0x79 ∈ 6..15
fresh producer + OR (flag 0x0F):    +0x79 = 15
after P2's +0x99 halving:           +0x79 ∈ 3..7            (floor 2 unreachable from a fresh 6..15)
after P3 (no replacement):          +0x79 ∈ 3..7, floor 3
composed (P3 then P2):              2                      (3>>1 = 1 → floor 2; 2>>1 → floor 2)
```

So the **live register domain is `2..15`**, reached as
`6..15 → {3..7} → {2..7}`. `0` and `1` are unreachable once any producer has
run — the port's `timer79 = 0` is *below* the native floor, i.e. a value the
native can never hold after a reset.

### 6.2 Timing in the frame chain

```
match start   FUN_00092E2C ─┐
kickoff       FUN_0004B02C(+1) ─┤
phase 0x0C    FUN_00088860 ─┼→ FUN_0007417C ─→ FUN_0008C33C(0x1588A4)
state reset   0x8B832 ──────┘                 FUN_0008C33C(0x1590D9)
                                                   └→ 11 × FUN_0007997C
                                                        0x79A4D store
                                                        0x79A86 → +0x7B
                                                        0x79AAC → state install (again +0x7B = +0x79)
per frame     FUN_0004B100 → FUN_0008D8EC (per team) → FUN_000782D0 (record 0)   → R1 +0x7B = +0x79
                                                     → FUN_0007CA54 (records 1..10) → row body
                                                          row 03: R4/R5, row 09: R6
```

* The store is **pre-phase, pre-frame**: `FUN_0004B02C` runs the prelude and
  then `FUN_0008C418`; the first granted frame of the new phase reads the new
  `+0x79`.
* Within the reset, the store precedes the `+0x7B` copy and the state install,
  so no window exists in which `+0x7B` holds a stale limit from the previous
  kickoff.
* `FUN_0007417C` runs on **both** pools (`0x1588A4`, `0x1590D9`, stride
  `0xB2`, 11 records each — the `0x835` team stride of FU84 §5), so all 22
  records are produced in one prelude.
* The only writer that can fire during play is P3 (row 12), and it fires on
  the no-replacement arm, not on a timer.

### 6.3 Per-record values at kickoff (all rows, both teams)

Basis is stated per column — this is a **static staging** row plus an honest
**unmeasured** row, not a measured native read:

| team | slot/record | native index source | native `+0x79` | port staging (`player_d = 0`) |
|---|---|---|---|---|
| 0 (`0x1588A4`) | 0..10 | `byte[[0x157BD2[0][slot]]+0xD]` | unmeasured (FU330-L1) | 0 → table[0] = **6** |
| 1 (`0x1590D9`) | 0..10 | `byte[[0x157BD2[1][slot]]+0xD]` | unmeasured (FU330-L1) | 0 → table[0] = **6** |

`0x157BD2` is the lineup snapshot `FUN_00073EE0` takes from `0x14C360` at
kickoff/prelude time, and it is what P2 installs into `rec+4` (`0x8C78A`).

* `+0x9D = 0` on every record at the first kickoff: the pool is BSS (fresh
  `read_memory 0x1588A4`, 64 bytes, all zero), and P2's caller set (7 sites,
  §3.2) does not intersect `FUN_0004B02C`'s `param == 1` arm. So the
  first-kickoff value is the bare table value with no OR override.
* The **keeper** (record 0) is special downstream, not in the producer: R1
  re-copies `+0x79 → +0x7B` at the head of every frame, so its counter starts
  each frame at the full limit (subject to its own row body); records 1..10
  get the row-03 `min(...)` (R5).
* The native index is roster data, not image data: `0x14C360` is filled from
  the loaded team file (`FUN_00041494` copies the `0x14ACDC`/`0x14AD1C`
  roster buffers into it), so a static read of the image cannot yield it
  (FU330-L1).

## 7. Errata to the carried docs (quoted, not rewritten)

**1. FU84 §6's `FUN_0007997C` window.** Quote: "`FUN_0007997C`
`0x799F0..0x79A4D`". **Corrected (fresh `disassemble_bytes 0x7997C`):** the
function body is **`0x7997C..0x79B19`** (`get_function_by_address`:
`body_start 0x7997c`, `body_end 0x79b19`); `0x799F0` is the 8th byte of the
9-byte `MOV word[EBP+0x85],0` at `0x799E9` and is **not an instruction
boundary**. The `+0x79` computation is `0x79A29..0x79A4D`. The §6 expression
is correct.

**2. FU84 §6's cell list.** Quote: "also zeroes
`+0x81/+0x87/+0x89/+0x91/+0x92` and `[rec+0x99]`, and copies `+0x6B→+0x77`,
`+0x75→+0x73/+0x71`". **Augmented (fresh):** it also zeroes `+0x83`, `+0x85`
(words), `+0x93`, `+0x98`, `+0x9B` (bytes), sets `+0x96 = +0x94 >> 4`, and
sets `+0x7B = +0x79` — the last is the load-bearing one for FU328-L2.

**3. FU328 erratum 2's "not a proven cap".** Quote: "6..15 is the `0x10F394`
**table entry** range; … that byte's producer is unported (OL-72). … '6..15'
is a table range, not a proven cap." **Confirmed and sharpened (§4/§5/§6):**
15 **is** a proven upper bound for in-range indices (table max 15, OR cannot
exceed 15); what is unproven is the **index domain** (unguarded `0x20..0xFF`
reads leave the table). Independently, 6 is not a floor: P2/P3 lower the
register to 3 (2 by composition). The erratum's practical conclusion (do not
port "6..15" as a constant) stands.

**4. FU328-L1's expression.** Quote: "`word[+0x79] = byte[0x10F394 +
(byte[[rec+4]+0xA]>>24)] | [rec+0x9D]`". **Refined:** the index byte is
physically `[[rec+4]+0xD]`; the high byte of the stored word is forced to 0;
and the `+0x9D` OR cannot exceed 15.

## 8. Numbered legs (FU330-L1…)

1. **FU330-L1 — the native per-record `+0x79` at kickoff is unmeasured.** The
   index is `byte[[rec+4]+0xD]`, `[rec+4] = 0x157BD2[team][slot]` (P2's
   prologue) — roster data loaded from the team file, not present in the
   image. The port stages `player_d = 0` (`fifa96_match_run.c:1443`). Measure
   by trapping `0x79A4D` (or `FUN_0007997C`'s entry) on the M2 tape and
   logging `[[rec+4]+0xD]`, `+0x9D`, `+0x79` for all 22 records; until then
   the only defensible port value is the staged 0 → 6. **Owner:** FU84 §6 +
   the roster descriptor (FU-142b Appendix C / FU188-L5).
2. **FU330-L2 — the table index has no range check.** `0x79A3C`/`0x79A41`
   give `idx = b` for `b ≤ 0x7F` (in-table only for `b ≤ 0x1F`) and
   `idx = b − 0x10000` for `b ≥ 0x80` (reading `0x0FF414..0x0FF493`). P2 uses
   a plain signed index (negative for `b ≥ 0x80`). The descriptor byte's real
   domain is unproven; the port's `0..31` guard models intent only.
   **Owner:** the roster byte's loader (unported).
3. **FU330-L3 — the `+0x9D` override's meaning is un-derived.** The mechanics
   are pinned (writer set, `0`/`0x0F`, the `0x14C1D4` bit × `+0x90` predicate),
   but which players are "special", and what `+0x90` denotes, is not.
   **Owner:** the roster/tactics flags.
4. **FU330-L4 — the `+0x9D`-writer census is operand-based.** Only direct
   `[reg+0x9D]` stores were enumerated; an address-computed store
   (`[base+index]` with `index = 0x9D`) would be missed. The nine record
   loads are consistent with the two-value model, but the completeness claim
   is census-shaped.
5. **FU330-L5 — P3's trigger semantics are inferred, not proven.** The
   mechanical condition is `EAX == -1` after the bench scan in `0x81C90` (a
   bench search comparing `+0x9` class and `+0xC` rating; `0x47` = the
   goalkeeper class exempt); the "injury / no eligible substitute" reading is
   from the surrounding code (anim `0x61`, the bench loop), not from a named
   native datum. The *value* (`max(v>>1,3)`) is exact.
6. **FU330-L6 — the `0x8B832` caller of `FUN_0007417C` is in an undefined
   region** (no Ghidra function). Its neighbourhood (`0x158818 = 0`,
   `0x158828 = 0`, `0x158808 = 0`, then `CALL 0x7417C`) reads as a match-state
   reset, but the owning routine is unnamed.
7. **FU330-L7 — `+0x79`'s downstream algebra is out of this slice.** R5's
   `min(...)` and R6's `×2` are pinned as *readers*; the stride table, the
   `attr` clamp and the seed chain are FU326/FU328/FU331 territory. In
   particular R6 (`+0x7B = +0x79*2`, then `+0x73 = mul(...)`) is the OL-82
   site the port stages as 0 (`handlers.c:5034`) — its correct value with a
   producer is `12..30`, which is a **2× change to the row-09 velocity
   staging**, not a 1× pass-through.
8. **FU330-L8 — the port has no `+0x9D` cell in the engine record.** The
   loader structs carry `byte9d` (`fifa96_action_handlers.h:1058`,
   `fifa96_outfield.h:247`), but the engine record
   (`fifa96_match_entities.h:60-180`) has `timer79`/`timer7b` and no `+0x9D`
   counterpart, so P1's OR cannot be ported faithfully without adding it
   (FU331's design decision).

## 9. GO / NO-GO

**GO — the producer is pinned and portable.** All three stores, the table (raw
bytes, per-index map, six readers), the OR (writer census, two-value model,
predicate), all seven reader rows (R1–R7), the timing (prelude, both pools,
before the first frame) and the value algebra are statically proven with fresh
reads. The port already carries the exact table
(`fifa96_arm_26_timer_table`).

**GO — the "6..15" question is settled.** 6..15 is the producer's in-range
range; **15 is a proven maximum** for `player_d ≤ 31`; **6 is not a floor**
(3 via P2/P3, 2 by composition); **no universal cap is proven** because the
index is unguarded. A port that hard-codes 6..15 as a clamp would be *safe*
against the table but would silently hide FU330-L2.

**NO-GO — any claim about the native per-record kickoff values or the natural
band record's `+0x79`.** Both need the roster descriptor (FU330-L1), which the
image does not contain. The phase-35 Stage-2 producer must therefore be
**value-staged** (port `player_d = 0` → 6) and pinned as such, with the native
distribution measured later — or the tape's actual descriptor bytes read from
the loaded team file at run time. This is FU328-L3, unchanged.

**Goldens / census.** Nothing measured here touches M1 `09b726b7…`, M2
`d5446144…` or the `60/80 = 75%` census; this slice is static only.

## 10. Known limits

* Static only: no native execution, no DOSBox-X trace. Every value is a
  read-from-bytes claim, and every data-dependent value is a leg.
* The handler-table attribution (rows 03/09/12) rests on
  `PTR_LAB_001106E0` (`0x1106E0`, fresh read) and on the handlers' own
  prologues; the *dispatch* that selects those rows at run time is carried
  (FU177/FU188), not re-derived.
* `FUN_0008C33C`'s tail (the pool header fields at `+0x7B2/+0x7B6/+0x7BF`,
  `0x8C3..`) was read only as far as needed to name the caller chain.
* The `0x0FF394 + b` out-of-range target was computed, not read byte by byte;
  its contents are not claimed.
* The P2 predicate's team word `0x14C1D4` is also written by
  `FUN_0001C9BC`/`FUN_00024B00`; those writers are not analysed here.

## 11. Provenance and self-audit

**Fresh Ghidra this session** (`/FIFA96.EXE`, `watcom:LE:32:default`, base 0,
`.image`; standard MCP tools only; no scripts/saves):
`get_function_by_address` `0x7997C` (body `0x7997C..0x79B19`), `0x7D9A4`,
`0x782D0`, `0x8C758` (`0x8C758..0x8C95B`), `0x8C33C`, `0x795B4`, `0x79B6C`,
`0x79C20`; `disassemble_bytes` `0x7997C` (416 B),
`0x78340` (0x50 B), `0x7E250`/`0x7E2C0`, `0x80B10`, `0x81820`, `0x81B00`
(0x380 B), `0x81E80` (0x2C0 B), `0x821C0` (0x160 B), `0x822B0`, `0x848A0`,
`0x8B800`, `0x8C758` (0xB0 B), `0x8C800` (0x160 B), `0x6DF60`, `0x86530`;
`decompile_function` `0x7997C`, `0x8C33C`, `0x8C758`, `0x8C974`, `0x8CBB0`,
`0x9343C`, `0x7417C`, `0x92E2C`, `0x4B02C`, `0x4B100`, `0x7D9A4`, `0x782D0`,
`0x8D8EC`, `0x88860`, `0x73EE0`, `0x73CD0`, `0x8A43C`, `0x41494`, `0x74034`;
`get_xrefs_to` `0x7997C` (1), `0x8C33C` (4), `0x8C758` (7), `0x7D9A4` (77),
`0x782D0` (1), `0x81C90` (1), `0x157BD2` (29), `0x157B90` (20),
`0x14C1D4` (21), `0x14C360` (58); `search_instructions` `"0x79"` (32
matches / 235 070 scanned), `"0x9d"` (31), `"157a38"` (7), `"0x7b"` (200, used
only for the `+0x7B` sites quoted); `read_memory` `0x10F394` (96 B),
`0x1106E0` (0x120 B), `0x1588A4` (64 B); `get_address_spaces`.

**Fresh source reads:** `src/fifa96_engine/fifa96_match_run.c:1425-1454`
(`player_d = 0`, `1443`), `src/fifa96_engine/fifa96_match_entities.c:84-99`
(`0x7DA9B` copy at `:94`), `src/fifa96_loader/fifa96_arm_bodies.c:28-57` (the
32-byte table + the row-26 read), and the `timer79`/`timer7b` grep across
`src/`/`include/` (36 + 16 matches).

**Carried (named, not re-derived):** FU84 §6 (the `FUN_0007997C` identity and
the table family; corrected in §7), FU328 §4.3 (the counter `min`, whose fresh
bytes match), FU328 §5.6 (the 3/6/15 diagnostic — a ladder, never a native
value), FU328 erratum 2 (the OR caveat; sharpened in §7), FU328-L1/L3,
FU326 §4.2 (the `|w| ≤ 15` downstream domain), FU177/FU188 (row 03/row 26
attribution).

**Self-audit.** (a) Every address, byte and value claim above cites a fresh
disassembly, a fresh decompile or a fresh raw-memory read; the only carried
claims are named as carried. (b) No native runtime value is asserted: the
per-record kickoff table states its basis (static staging) and marks the
native column unmeasured. (c) The "cap" question is answered with the exact
conditions (`idx ≤ 31`, `+0x9D ∈ {0,0x0F}`) rather than a slogan. (d) The
three producers' *values* are proven; P3's *trigger meaning* is a numbered
leg, not an assertion. (e) No repo file was written except this draft; no git;
`build/`, `tests/golden/`, the ISO and `fifa96.rep/` were never written.
(f) Census and goldens are untouched by this slice.

## Review erratum (wave review, 2026-10-23)

**1. The P2 disassembly omits `XOR AH,AH`.** Quote (§3.2): "`0x8C846  BL  = byte[rec+0x9D]` … `0x8C84E  AL |= BL` … `0x8C850  MOV word[rec+0x79], AX`". **Corrected (fresh `disassemble_bytes 0x8C820`):** the block is `0x8C846 MOV BL,[EDX+0x9D]`, **`0x8C84C 30e4 XOR AH,AH`**, `0x8C84E OR AL,BL`, `0x8C850 MOV [EDX+0x79],AX`. P2 therefore also stores an 8-bit value in the word cell; the block's "no XOR BH,BH" note is about the *index/address* only, not the stored high byte. The value algebra (§6.1) is unchanged.

**2. The index-mechanism order (§3.1).** Quote: "`SAR EBX,0x18` sign-extends the descriptor byte; `XOR BH,BH` then clears bits 8–15 but not 16–31, so a byte `0x80..0xFF` becomes `b − 0x10000` and the read lands at `0x0FF394 + b`". **Corrected (fresh bytes):** the order is `0x79A3C SAR EBX,0x18; 0x79A3F ADD EDX,EBX; 0x79A41 XOR BH,BH`. The ADD consumes the fully sign-extended EBX, so the out-of-range address `0x0FF394 + b` is produced by the sign-extended ADD alone; the `XOR BH,BH` is **after** the ADD and clears only the store's high byte (`BH`), not the index. The bullet's conclusion stands; the causal sentence does not.

**3. "row-12" is action code `0x0C`, not row `0x12`.** Quote (§2/§3.3): "The no-replacement (row-12) degradation at `0x822DB..0x822F0` (handler `0x81C90`, fresh handler-table xref `0x110710` = slot 12)". **Corrected (fresh `read_memory 0x1106E0`):** `0x1106E0[12] = 0x81C90`; the table is the **action-code** table, so [12] is code **0x0C** (decimal 12), the contact/no-replacement body — not action row 0x12 (the free-kick taker). Rename P3 "the code-0x0C no-replacement arm" (FU333's §5.2/L15 repeats the label).

**4. The consumers claim (§4).** Quote: "The port's row-26 and phase-0C/code-24 consumers are therefore already native-faithful; only the `+0x79` producer is absent." **Corrected (fresh source reads):** only the row-26 arm mirror is ported (`fifa96_arm_bodies.c:51-52`, `fifa96_arm_26_timer_table[32]`); the phase-0C `0x6DF78` (`+0x7B = t>>1`) and code-24 `0x8653D` (`+0x7B = t-2`) table readers are **unported** (the phase-0C body's own comment at `handlers.c:6479` names the `[rec+4]/0x157A38` table as FU192-L1; the code-24 row names `0x157A38` as FU188-L5). The bytes are native-faithful; the consumers are not all wired.

**5. The `+0x9D = 0` basis (§6.3).** Quote: "The `+0x9D = 0` on every record at the first kickoff: the pool is BSS (fresh `read_memory 0x1588A4`, 64 bytes, all zero)". **Corrected:** the image read proves the BSS *initial content*, not the runtime value at kickoff. The first-kickoff zero is established by the writer census: P2 (`0x8C758`) is the only record writer, and its seven callers do not intersect `FUN_0004B02C`'s `param == 1` arm. The conclusion stands; cite the caller set as the proof.
