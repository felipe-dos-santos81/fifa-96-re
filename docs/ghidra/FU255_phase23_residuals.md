# W24-8 / FU-255 — the phase-23 residuals: FU241-L1, FU243-L4, FU241-L2

Provenance: M2 phase-24 wave W24-8, 2026-10-14, **read-only** recon. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; the 16-bit loader `/fifa96.exe` and the flat `/fifa96_le.bin` are the
wrong targets). Fresh `get_current_program_info` this session: **2706 functions /
15239 symbols**, `watcom:LE:32:default`, LE image with one `.image` overlay
space (the overlay is not engine code — all addresses below are default-space
runtime). Every xref count, table byte, call byte and instruction below is a
fresh `get_xrefs_to` / `read_memory` / `disassemble_bytes` /
`get_assembly_context` / `search_byte_patterns` of **this session** unless marked
*cross-cited* (then the frozen FU doc or the engine source is named). Alias traps
applied (`dword[x]>>16` = the word at `x+2`; `>>24` = the byte at `x+3`). Engine
source read first-hand: `src/fifa96_engine/fifa96_match_run.c`,
`fifa96_match_handlers.c`, `fifa96_match_act.c`, `fifa96_match_entities.c`; the
frozen docs `docs/ghidra/FU241_phase_1e_decompression.md`,
`docs/ghidra/FU243_no_installer_cluster.md`, `docs/ENGINE.md`,
`docs/PROGRESS.md`. **No writes**: no rename/comment/label/function/script/project
save; no git; the ISO and `game/hdd/` untouched. Draft + run report only.

This slice closes the three residuals the phase-23 wires left open (ENGINE.md
~1464 / ~1427; FU-241 §7; FU-243 §5). The phase-23 T2/T3/T4 landings already
ported row 25, row 15 and the phase-`0x1E` sweep and re-routed the `set_piece`
`BX!=0` branch; those commits are the live baseline here
(`git log`: `dffc9ec` row 25, `e73d9c0` row 15, `b915007` phase `0x1E`).

---

## 1. Verdict (one line)

**FU241-L1 — CLOSED, no port needed.** The 22-frame deferral is **exactly the
native cadence** (`FUN_0008A938` `BX!=0` → `CALL 0x888FC(8,0,BX=1)` invoke-now,
then one sweep record per granted-frame clock-tail tick, terminal on the 23rd
call); the retired compression was a **dormant-on-M2-only** in-frame
approximation and is **not** equivalent to the native on the non-tape `bx=1`
paths — the delta is a ~22-frame phase-0 dwell. The engine's
`fifa96_match_act_dispatch`/`_tick` reproduce `FUN_000888FC` / `0x8B669..0x8B672`
instruction-for-instruction, so the re-route is faithful byte-for-byte. Fresh
native `bx=1` caller census = **8 sites** (below); the engine models 5.
**FU243-L4 — SUBSTANTIALLY RESOLVED, one cell remains.** The row-25 active head
block and the row-15 arms are freshly decoded; the radius-`0xC0` fold reuses the
already-ported `fifa96_entity_sine` idiom (radius `0xC0`, not `0x180`); the
`[0x1587B8]`/`[0x1587BB]` id tables are **provably constant** (`{0x15,0x03}` at
idx 0, `0` at idx 1..4), so the row-15 arm-1 `==0x67` gate is **dead** and the
arm-2 / row-25 idx install is a **derivable anim constant**; the `0x741B4`
display-side helper is already ported (`side_swap`) and `0x651F0` is the SFX
event sink. The **one** genuinely-gating cell is `[team+0x59]` (the radius fold's
source triple) — FU255-L1. **FU241-L2 — CLOSED, inert.** `word[0x157A68]` is
**write-only**: fresh `get_xrefs_to` = **4** (three clears + one read-modify-write
accumulate) and a fresh `search_byte_patterns 687a1500` returns exactly those 4
instruction sites; there is **no reader**, so the sweep's `0x4C374` clear cannot
move any observe.

---

## 2. FU241-L1 — the non-tape compression equivalence

### 2.0 The native `BX!=0` path (fresh `0x8AA7B`, 80 B / 21 insns)

```
0x8AA7B TEST BX,BX ; 0x8AA7E JZ 0x8AAA8            ; BX==0 -> the table-2 route
0x8AA80 XOR EDX,EDX ; XOR EAX,EAX ; MOV EBX,0x1
0x8AA89 CALL 0x740A0                               ; FUN_000740A0(0,0) = write PHASE 0
0x8AA8E MOV AL,[ESP]                               ; AL = side (arg)
0x8AA91 XOR EDX,EDX
0x8AA93 MOV [0x15882C],AL                          ; store SIDE
0x8AA98 MOV EAX,0x8                               ; <<< ACT 8
0x8AA9D MOV byte[0x15882B],CL                      ; store SITUATION (CL)
0x8AAA3 JMP 0x8AF1A                                ; -> 0x8AF1A CALL 0x888FC
```

`0x8AF1A CALL 0x888FC` (`e8dd d9ff ff` → `0x8AF1F - 0x2623 = 0x888FC`); the
`0x8AF0E` (sit-0xC) arm falls through into the same call, so `0x8AAA3` is the
**sole** branch xref into the shared site (fresh `get_xrefs_to 0x8AF1A` = **1**,
UNCONDITIONAL_JUMP from `0x8AAA3`; `get_xrefs_to 0x8A938` = **39**, see §2.3).

### 2.1 The seam `FUN_000888FC` (fresh, 20 insns) — matches the engine exactly

```
0x888FC PUSH ECX
0x888FD MOVSX CX, byte[0x158828]      ; the installed act id
0x88905 CMP AX,CX ; 0x88908 JZ 0x8893D ; *** the latch no-op ***
0x8890A MOV [0x158828],AL             ; act id
0x88918 MOV byte[0x158829],DL         ; stage
0x88922 MOV word[0x158818],DX         ; timer = 0
0x88929 MOV [0x158808],EAX            ; handler = 0x1107EC[act]
0x8892E TEST BX,BX ; 0x88931 JZ 0x8893D ; BX==0 -> install only
0x88933 TEST EAX,EAX ; 0x88935 JZ 0x8893D ; NULL handler -> no invoke
0x88937 CALL dword[0x158808]          ; *** invoke-now ***
```

The clock tail (fresh `0x8B655`):

```
0x8B669 CMP dword[0x158808],0 ; 0x8B670 JZ 0x8B678
0x8B672 CALL dword[0x158808]          ; the per-frame act tick
```

The engine's `fifa96_match_act_dispatch` (`match_act.c:38..53`) mirrors the
latch (`:45 a->id==act`), the id/stage/timer writes and the `invoke` getest
(`:49`); `fifa96_match_act_tick` (`:55..65`) mirrors `0x8B669/0x8B672`. The
`BX!=0` re-route is `match_run.c:3145..3158`: store side/situation,
`match_run_write_phase(mr,0)`, `fifa96_match_act_dispatch(mr, 8, 0, 1)`.

`match_run_write_phase(mr, 0)` (`match_run.c:2514`) runs the per-team
`FUN_0008D098` walk for both sides because `fifa96_match_phase_table[0].fn`
(`handlers.c:5551 fifa96_match_phase_00`) is registered — matching native
`FUN_000740A0`'s unconditional two-team `CALL 0x8D098` (`0x740C8`/`0x740DB`).

### 2.2 The deferral is exact — the frame trace

`FUN_000888FC(8,0,1)` sets `act.id=8`, `stage=0`, `timer=0`,
handler `0x1107EC[8] = 0x8A798` (phase `0x1E`), and **invokes the sweep in the
same frame** (`0x88937`). The sweep's stage 0 resets the cursor and falls into
stage 1, processing exactly one record. Each subsequent granted frame's clock
tail (`0x8B672`) processes the next record. 2 teams × 11 records = 22 records;
after record `(1,10)` (the 22nd call) `stage→2`; the **23rd** call runs the
stage-2 terminal: clear the act slot (`0x8A8B1/0x8A8B7`), re-dispatch
`FUN_0008A938(sit, side, BX=0)` (`0x8A8CE`, fresh `get_xrefs_to 0x8A8CE` in the
39-set, BX=0) and clear `store_15882b` (`0x8A8D3`). So the stored-situation
hand-off is deferred **~22 granted frames**. (FU-241 §2 reconfirmed.)

The engine reproduces the cadence: the re-route dispatches act 8 invoke-now
(one record in the call frame) and `fifa96_match_act_tick` (`match_run.c:4295`)
is the per-frame tick. For callers that fire *before* the tick in the same frame
body (the goal scanner at `:4244` precedes the tick at `:4295`), the native and
the engine both process **two** records in the call frame — a same-frame
double-run that is reproduced, not lost.

### 2.3 Fresh native `bx=1` caller census (39 xrefs → `get_assembly_context`)

`get_xrefs_to 0x8A938` = **39** (`FUN_0008A938` = the native
`set_piece`). Fresh 6-insn-window classification of every site's `EBX`/`EAX`:

| call site | containing fn | sit | BX | engine caller |
|---|---|---|---|---|
| `0x88c00` | `FUN_00088940` (goal scan) | 2 | **1** | `match_run.c:3301` goal_scan throw-in |
| `0x85d0a` | row-0x10 throw-in taker body | 2 | **1** | `handlers.c:1718` set-piece stage-4 re-queue |
| `0x897e3` | phase-`0x1A` tail | stored | **1** | `match_run.c:6163` `match_run_phase_1A_end` |
| `0x8a3f0` | ref cluster | 0xA | **1** | `match_run.c:5908` `match_run_ref_apply` |
| `0x8a729` | `FUN_0008a43c` (ref restart) | 9 | **1** | `match_run.c:6538` referee restart decision |
| `0x8920a` | act-2 / phase-`0x18` vicinity | 9 | **1** | ref cluster (off-tape only) — FU255-L4 |
| `0x76b1c` | row-0x1B keeper body | 5 | **1** | unported keeper row — FU255-L4 |
| `0x77707` | row-0x1B tail | 5 | **1** | unported keeper row — FU255-L4 |

The other 31 sites are **BX=0** (fresh context), including the two `FUN_0004B02C`
period-setup calls (`0x4b0a5`/`0x4b0d9`), the `FUN_00088860` arms
(`0x888c8`/`0x888f2`), the `0x8B85D` sit-1 period-end arm (`EBX=0` via the
`0x8B83E XOR EBX,EBX`), the `0x88b44`/`0x88bbd` goal-scan corner/goal-kick arms,
the `0x93xxx` ladder, and the sweep's own terminal `0x8a8ce`.

**Verdict.** The 5 named engine callers (FU-241 §5.2) are the native
`0x88c00`/`0x85d0a`/`0x897e3`/`0x8a3f0`/`0x8a729`. The retired compression
re-dispatched the stored situation **in-frame**; the native defers it to the
sweep terminal. The two are therefore **not** equivalent on the non-tape paths:
the de-compressed model runs ~22 frames in **phase 0** (the `FUN_000740A0(0,0)`
write is unconditional in both models) before the successor phase lands. Because
FU-241 §6.1 proves the sweep's per-record effect is a pointer store +
`word[0x157A68]` clear (no record/render write), the *only* delta is that dwell —
but on the goal_scan throw-in (sit 2 → phase 3), the stage-4 re-queue (sit 2 →
phase 3) and the ref cluster it is a real, observable timing change. On M2 the
golden never enters the `bx!=0` path (FU-241 §6.2: no `state=3`), so the golden
is unmoved (FU-241 T4 cmp-verified); the 3 extra native `bx=1` sites
(`0x8920a`/`0x76b1c`/`0x77707`) have no engine caller (keeper rows 1B/1C
unported; the ref cluster off-tape) — FU255-L4.

---

## 3. FU243-L4 — the row-25/15 active head, the radius-`0xC0` fold, the id tables, the sinks

### 3.1 Row-25 head + active block (fresh `0x880CC`, 240 B)

Prologue: `PUSH×6; MOV EBP,EAX`. Head:

```
0x880D6 MOV EDX,[EBP+0x89] ; MOV AX,[0x157A64] ; ADD EDX,EAX
0x880E9 MOV [EBP+0x89],EDX                  ; timer89 += (u16)delta
0x880E4 MOV EAX,0x9C4 ; 0x880EF CALL 0x974F0 ; sound sink 0x974F0(0x9C4)
0x880F4 MOV EAX,[EBP] ; MOV [0x157AA3],EAX  ; [0x157AA3] = [rec] (owner ptr)
0x880FC MOV EAX,0x2 ; 0x88101 CALL 0x36200  ; camera sink 0x36200(2)
0x88106 CMP byte[EBP+0x8D],0 ; JZ 0x881D2    ; *** the active gate ***
```

Active block `0x88106..0x881D2`:

```
0x88113 MOV ESI,[EBP] ; 0x88119 LEA ESI,[ESI+0x59] ; LEA EDI,[EBP+0x4D]
0x8811C.. MOVSD ×3                          ; target(+0x4D) := [owner+0x59] triple
0x8811F MOV EAX,[EBP+0x8A] ; SAR EAX,0x18   ; *** byte[rec+0x8D] (alias) ***
0x88128 SHL EAX,6 ; LEA ECX,[EAX+0x100] ; sign-fold
0x88143 MOV EAX,[EAX*4+0x114E04]            ; the sine table lookup
0x8814E MOV EDX,EAX ; MOV EAX,0xC0 ; IMUL EDX
0x8815C ADD EAX,0x8000 ; ADC EDX,0 ; SHRD EAX,EDX,0x10
0x88163 MOV EBX,[EBP+0x4D] ; ADD EBX,EAX ; MOV [EBP+0x4D],EBX   ; target_x += 0xC0*sin
0x88168.. (same fold on the z axis) ; MOV [EBP+0x55],ECX        ; target_z += 0xC0*cos
0x881BA CALL 0x8DCD4                          ; metric
0x881BF MOV EBX,[EBP+0x67]>>0x10 ; MOV EDX,[EBP+0x65]>>0x10
0x881CD CALL 0x79C50                          ; face
```

`0x114E04` is the shared sine table (fresh `read_memory`:
`00000000 92010000 24030000 b6040000`; fresh `get_xrefs_to 0x114E04` = **91**,
including `0x88143`/`0x88189`). The existing engine primitive
`fifa96_entity_sine` is exactly this fold (`match_entities.c:468`:
`(int16_t)(((int64_t)0x180 * fifa96_entity_sine(angle) + 0x8000) >> 16)`), and
`fifa96_arm_dist_stage`/`fifa96_arm_face` are the `0x8DCD4`/`0x79C50` ports.

> **Alias finding (new).** `[rec+0x8A]>>24` = **`byte[rec+0x8D]`**. The active
> gate at `0x88106` tests the *same byte* (`CMP byte[EBP+0x8D],0`). So the
> "radius-`0xC0` `[rec+0x8A]` fold" angle source is `byte[rec+0x8D]` — an
> active/angle byte, **not** a 24-bit field at `+0x8A` (corrects the FU192
> `[rec+0x8A]` "24-bit angle" wording; still consistent with FU192's "not a
> distinct field"). The engine models neither `+0x8A` nor `+0x8D`.

### 3.2 The id tables `[0x1587B8..0x1587BC]` — provably constant

Fresh `read_memory 0x1587B8` (24 B) = **all zero**; fresh
`get_xrefs_to 0x1587B8` = **7**: writers `0x7d95d` (`FUN_0007D8D0`) and
`0x87784` (`FUN_0008776C`), readers-in-`0x8776C` `0x8778b/0x8779e/0x877bf`, and
the two indexed DATA sites `0x87db6` (row 15) / `0x8843b` (row 25).

`FUN_0008776C` (fresh, 24 insns) is the id-init:

```
0x87770 MOV dword[0x158720],0        ; [0x158720] = celebration idx = 0
0x87776 CALL 0x92AC8 ; 0x8777B AND EAX,1
0x8777E MOV AL, byte[EAX + 0x10F344]
0x87784 MOV [0x1587B8],AL            ; seed id0 from 0x10F344[rng&1]
0x87790 CMP EAX,0x67 ; 0x87793 JNZ 0x877BA
0x8779E.. MOV [0x1587B9..0x1587BC] = AL   ; the 0x67 copy (never reached)
```

`0x10F344` is **read-only** (fresh `get_xrefs_to 0x10F344` = **1**, the
`0x8777e` read); fresh `read_memory 0x10F344` (8 B) =
`15 03 19 16 1a 15 67 03` → `[0]=0x15`, `[1]=0x03`. So `rng&1 ∈ {0,1}` selects
`[0x1587B8] ∈ {0x15,0x03}` — **never `0x67`** — so the `0x87793 JNZ` always
takes, the `+0x67` copy at `0x877A3..0x877B2` is **unreachable**, and the only
other writer, `FUN_0007D8D0` (`0x7D95D`: `XOR BH,BH; loop EDX=0..4;
MOV byte[EDX+0x1587B8],BH`), writes **0**. Hence:

* `byte[0x1587B8+idx]` = `{0x15,0x03}` at `idx==0`, **`0`** at `idx==1..4`;
* `byte[0x1587BB]` = **`0`** always.

`FUN_0008776C` is called from row 14 (`0x87a83`) and row 25 (`0x883e2`); fresh
`get_xrefs_to 0x8776C` = **2**.

**Row-15 arm 1** (`0x87DA8`, fresh): `0x87DB0 JZ 0x87DC6` (idx==3), else
`0x87DB6 MOV AL,[EAX+0x1587B8]; CMP AL,0x67; JNZ 0x87E09` — the `==0x67` gate is
**dead** for every reachable `idx` (idx 0 → 0x15/0x03; idx ≥1 → 0).
**Row-15 arm 2** (`0x87F4A`, fresh): `0x87FB3 MOV AH,[0x1587BB]` (always 0);
`0x87FBD.. CALL 0x6E598` installs anim `[0x1587BB] = 0` when the current anim
differs (with `ECX = [rec+0x8B]>>24`).
**Row-25 arm 0** (`0x88430`, fresh): `0x8843B MOV BL,[EDX+0x1587B8]` then
`0x8845A CALL 0x6E598` installs `[0x1587B8+idx]` ∈ `{0x15,0x03}` (idx 0) / `0`
(idx ≥1).

**Verdict.** The "unported id tables" of FU-243 §5/FU184-L1 are **derivable
constants**, not unknowns: the row-15 arm-1 gate is dead, and the arm-2/row-25
installs are computed from `0x10F344[rng&1]` (idx 0) and 0. The engine's row-25
arm 0 currently mirrors only the `[0x158720]=0` reset (`handlers.c:5187`).

### 3.3 Row-15 prologue sinks + the arm tables (fresh)

```
0x87D2C MOV EAX,[EBP] ; MOV AL,[EAX+0x826] ; AND EAX,0xff
0x87D3A CALL 0x741b4                         ; display side = (side ^ [0x157ABE]) & 1
0x87D3F TEST EAX,EAX ; JZ -> EAX=2 else EAX=8
0x87D4F CALL 0x651f0                         ; 0x651F0(display-side ? 8 : 2)
```

Fresh `disassemble_function 0x741B4` = `(EAX ^ byte[0x157ABE]) & 1` — the
display-side helper, **already ported** as `mr->side_swap`
(`match_run.c:3085`; producer unported, OL-72/OL-75, so identity in-engine).
Fresh `disassemble_function 0x651F0` = the gated SFX event sink
(`[0x55CE0]`/`[0x55D38]`/`[0x55D08]` gates → `MOV [0x55D50],EAX; JMP 0xA72E7`;
FU-113). The 4-arm row-15 table (fresh `read_memory 0x87CC0`) =
`{0x87D6F, 0x87DA8, 0x87F4A, 0x87FF5}`; the 7-arm row-25 table (fresh
`read_memory 0x880B0`) = `{0x881ED, 0x8847E, 0x88568, 0x8859B, 0x885FE, 0x88645,
0x88699}` — both match the engine. Second id writer `FUN_0007D8D0` (fresh,
53 insns) is the match-init reset (`[0x158720]=0` + the 5-byte `0x1587B8` clear
+ the `0x1586CC..0x1586D4` scatter buffer).

---

## 4. FU241-L2 — `word[0x157A68]`, the clear and the consumer

Fresh `get_xrefs_to 0x157A68` = **4**:

| site | kind | function | instruction |
|---|---|---|---|
| `0x8AF8C` | **READ_WRITE** | `FUN_0008AF38` (clock tail) | `ADD word[0x157A68],AX` (AX = `[0x157A64]` frame delta) |
| `0x4C377` | WRITE | `FUN_0004C374` | `MOV word[0x157A68],DX` (DX=0) — the sweep's `0x8A836` call |
| `0x4B0EE` | WRITE | `FUN_0004B02C` | `MOV word[0x157A68],BX` (BX=0) — period setup |
| `0x74349` | WRITE | (period-end) | `MOV word[0x157A68],CX` (CX=0) |

Fresh `disassemble_function 0x4C374` = `PUSH EDX; XOR EDX,EDX;
MOV word[0x157A68],DX; POP EDX; NOP` — **ignores EAX** (reconfirms FU-212-E1 /
FU194-L3: the `0x8A836 MOV EAX,0x19; CALL 0x4C374` does not install code `0x19`).
The clock tail (`0x8AF86 MOV AX,[0x157A64]; 0x8AF8C ADD word[0x157A68],AX`,
gated `0x8AF75 MOVSX EDX,DI; CMP EDX,1; JZ` / `TEST AX,AX; JZ`) **accumulates**
the frame delta into it.

**Consumer: none.** A fresh `search_byte_patterns 687a1500` returns exactly
`{0x4b0f1, 0x4c37a, 0x7434c, 0x8af8f}` — the four instruction operands of the
four xrefs. There is **no reader**: the only instruction that touches it and
reads is the accumulate itself. The adjacent cell `word[0x157A66]` is accessed
word-sized only (fresh `get_xrefs_to 0x157A66` = 7; e.g. `0x4B143 MOV
CX,word[0x157A66]`, `0x7437A MOV DX,word[0x157A66]`, `0x4B55A MOV
AX,word[0x157A66]`) — no dword access aliases `0x157A68`. `word[0x157A66]` is a
separate accumulator (`0x4B157 MOV [0x157A66],CX` `= old + (X>>8)`).

**Verdict.** `word[0x157A68]` is a **written-only word accumulator** (clock-tail
frame-delta accrual) reset at period setup (`0x4B0EE`), period-end (`0x74349`)
and the act-8 sweep (`0x4C374`). With no reader its value is inert; the sweep's
clear is therefore provably observe-free. (It is *not* the FU-62 clock, which is
the `[0x157A64]`/`[0x157A66]`/`[0x157A5C]` set and is already ported.)

---

## 5. Port / proof shapes

* **FU241-L1 — no port.** T4 (`b915007`) already landed the correct model. Proof
  shape: a non-tape `set_piece(sit,1,1)` fixture (sit 2 the cheapest) asserting
  (a) `state.phase == 0` and `act.id == 8` immediately; (b) the successor phase
  (`3` for sit 2) is reached **only after** the 22-record sweep reaches its
  stage-2 terminal (`fifa96_match_act_tick` × 22), never in-frame; (c) a
  call-frame double-run for a pre-tick caller (drive `match_run_goal_scan` with
  the throw-in gate open and assert two records consumed in the dispatch frame).
  Plus the standing M2/M1 `cmp` byte-stability re-check.
* **FU243-L4 — partial port.** (1) The radius-`0xC0` fold is
  `target_x += round16(0xC0 * fifa96_entity_sine(angle))`,
  `target_z += round16(0xC0 * fifa96_entity_sine(angle + 0x100))` after
  `target := [owner+0x59]`, with `angle = byte[rec+0x8D]` (the `[rec+0x8A]>>24`
  alias) — reusing the ported `fifa96_entity_sine`/`fifa96_arm_dist_stage`/
  `fifa96_arm_face` (radius `0xC0`, not the existing `0x180`). Gated on the
  owner/team base triple `[owner+0x59]` (FU255-L1). (2) The id-table installs are
  **derivable**: add `id[0] = byte[0x10F344][rng_draw & 1]` (one `0x92AC8` draw,
  the `FUN_0008776C` seed) and `id[1..4] = 0`, then the row-25 arm-0 and row-15
  arm-2 installs are `byte[0x1587B8+idx]`; the row-15 arm-1 `==0x67` gate is
  dead (assert it never fires). (3) The `0x741B4` display side is already
  `side_swap`; `0x651F0(8|2)` stages as an SFX event cell. Proof: direct unit
  tests on `fifa96_match_action_15`/`_25` over a constructed record/team, then a
  byte-stable M2/M1 `cmp` (rows are tape-dormant).
* **FU241-L2 — no port.** The clear is inert (no reader). Proof shape: a
  fixture that runs the sweep's select arm and asserts no presented frame/record
  moves (the M2/M1 `cmp` already covers it); the cell itself can stay an
  unmodelled no-op leg without effect.

---

## 6. Numbered legs

1. **FU255-L1 — the radius-fold source `[owner+0x59]`.** The row-25 active head
   (and the row-15 arm folds) copy the owner/team base triple `[rec]+0x59`; the
   engine's `fifa96_match_team` (`match_entities.h:195`) carries no `+0x59`
   position (its fields start at side `+0x826`). Whether `[rec]` resolves to the
   team block (whose `+0x59` would be a team-base/record-0 position) or another
   owner pointer is not decomposed here. Owner: the FU-243 row-25 landing /
   FU184-L1 sibling. This is the one genuinely-gating FU243-L4 cell.
2. **FU255-L2 — the id-init RNG consumption.** `FUN_0008776C` spends one
   `0x92AC8` draw (`0x87776`) to seed `[0x1587B8]`. The engine's row-25 arm 0
   currently does not consume it; adding it (for the derivable id value) shifts
   the RNG stream for any *downstream* consumer of the same stream. The id-table
   *decisions* are provably value-constant, so the only risk is stream parity.
   Owner: the row-14/25 id-init landing.
3. **FU255-L3 — `word[0x157A68]` reader confirmation.** No reader is statically
   present (4 xrefs + byte scan). Whether it is a dead accumulator or read
   through a runtime-patched pointer (e.g. a debug overlay) is unprovable; the
   port may safely drop it. Owner: this slice / FU194-L4 close-out.
4. **FU255-L4 — the 3 extra native `bx=1` sites.** `0x8920a` (sit 9, the
   act-2/phase-`0x18` vicinity), `0x76b1c` and `0x77707` (sit 5, the row-0x1B
   keeper body/tail) pass `EBX=1` to `FUN_0008A938` but have no named engine
   caller (keeper rows 1B/1C unported; the ref cluster off-tape). Their engine
   mapping/port is a separate producer question. Owner: the keeper-row cluster /
   the FU-183 ref cluster.
5. **FU255-L5 — the `[rec+0x8A]`/`[rec+0x8D]` cell identity.** The active gate
   and the fold angle are the **same** byte (`rec+0x8D`); the surrounding
   `rec+0x8A..0x8C` bytes are untouched by the row-15/25/24 paths decoded here.
   Whether `+0x8D` is a dedicated active-angle byte or the top of a packed field
   is not decomposed. Reference: FU192-L1, FU188 §4. Owner: the FU-243 row-25
   landing.

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled;
overlay `.image` present; `get_current_program_info` confirmed this session):

* `disassemble_bytes` — `0x8AA7B` (80 B), `0x8AF00` (48 B), `0x8B650` (48 B),
  `0x8B810` (96 B), `0x891D0` (70 B), `0x8AF70` (64 B), `0x74330` (64 B),
  `0x4B0E5` (40 B), `0x4B138` (48 B), `0x4B548` (48 B), `0x74370` (48 B),
  `0x880CC` (240 B), `0x881BA` (48 B), `0x87DA8` (224 B), `0x87F4A` (144 B),
  `0x87D1C` (80 B), `0x88430` (70 B), `0x87FF5` (40 B), `0x8776C` (80 B).
* `disassemble_function` — `0x888FC` (20), `0x740A0` (25), `0x4C374` (5),
  `0x89EB0` (59), `0x741B4` (7), `0x651F0` (8), `0x7D8D0` (53).
* `get_assembly_context` — the 39 `FUN_0008A938` xrefs (6-insn windows).
* `read_memory` — `0x1587B8` (24), `0x10F344` (8), `0x114E04` (16), `0x87CC0`
  (20), `0x880B0` (32).
* `get_xrefs_to` — `0x8A938` (39), `0x8AF1A` (1), `0x157A68` (4), `0x157A66`
  (7), `0x1587B8` (7), `0x8776C` (2), `0x10F344` (1), `0x114E04` (91).
* `get_function_by_address` — `0x88940`, `0x8A43C` (and the unanalysed spans
  0x85D0A/0x8920A/0x76B1C/0x77707/0x897E3/0x8A3F0 → "No function found").
* `search_byte_patterns` — `687a1500` (4 hits).
* `get_current_program_info`.

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(`:2514..2534` `match_run_write_phase`; `:3112..3201` `set_piece` incl. the
`BX!=0` re-route `:3145..3158`; `:2897..2947` `match_run_situation_side`;
`:3280..3320` goal scan; `:4244`/`:4295` the frame order; `:5891/:5908`
ref apply; `:6158..6164` phase_1A_end; `:6345..6412` `fifa96_match_action_phase_1E`;
`:6535..6540` referee restart); `src/fifa96_engine/fifa96_match_act.c`
(`:38..65` the dispatch/tick seam); `src/fifa96_engine/fifa96_match_handlers.c`
(`:5097..5282` row 25; `:5284..5446` row 15; `:5549..5621` the phase table);
`src/fifa96_engine/fifa96_match_entities.c` (`:439..486` the `0x180` sine fold);
`include/fifa96_engine/fifa96_match_entities.h` (`:195` team struct, `:562` the
sine idiom); `include/fifa96_engine/fifa96_match_handlers.h` (`:262/:281`
FU243-L4). Docs cross-cited: FU-241 (§2/§5.2/§6.1/§6.2/§6.3/§7), FU-243
(§4.2/§4.3/§5/§6/§8), FU-212 (§6/E1), FU-194 (L2/L3/L4), FU-184 (L1/L3/L4/L8),
FU-192 (L1), FU-113. **No writes** beyond this draft + the run report; no
rename/label/comment/function/script/project save; no git; ISO/`game/hdd/`
untouched.
