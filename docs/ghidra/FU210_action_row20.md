# FU-210 — action row `0x20` byte-level derivation (phase-19 wave R58)

Provenance: read-only, 2026-10-13. Authoritative program **`/FIFA96.EXE`**
(Ghidra MCP; scripts disabled; case-sensitive program argument; confirmed this
session by `get_current_program_info` — `/FIFA96.EXE`,
`watcom:LE:32:default`, image base `0`, `min_address 0x10000`). Every
address/byte/xref/disassembly below is a fresh `read_memory` /
`disassemble_bytes` / `decompile_function` / `get_xrefs_to` of **this session**
unless marked *cross-cited* (then the frozen FU doc is named). LE-image globals
sit at `+0x100000` from the file offset; the addresses here are `/FIFA96.EXE`
program (runtime) addresses. The alias traps are applied (`dword[x]>>16` = word
at `x+2`, `>>24` = byte at `x+3`; e.g. `[rec+0x8F]>>24` = the byte at
`rec+0x92`). Engine/loader source read first-hand
(`src/fifa96_engine/fifa96_match_handlers.c`, `fifa96_match_run.c`,
`fifa96_match_act.c`, `include/fifa96_engine/fifa96_match_entities.h`,
`include/fifa96_engine/fifa96_match_run.h`, `tests/test_engine_m2.c`). **No
writes**: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; ISO untouched; no git commands.

Scope (R58 brief): derive native **action row `0x20`** (body `0x84EEC` —
verified first-hand) byte-level; enumerate its stages (arm/stage chain), every
render field it writes (does it touch M2/tape-visible state?), its gates, and
exactly how the installer `0x899A2` reaches it; state what the engine must add
to wire it (handler + table row) and whether wiring it changes the tape (M2),
precise about which fields move.

---

## 1. Verdict (one line)

**Row `0x20` = the actor-record "ball-camera / restart-commit transition"
body `0x84EEC` (7-arm CS table `0x84ED0`), installed by `0x899A2` (arm 0 of the
phase-`0x1D` / act-7 handler `0x89868`, deferred) — it is render-capable (arm 0
and arm 4 write **pos_x/pos_y/pos_z + anim_id + frame**, three M2 render-read
families) but **tape-dormant**: the tape stages no extra-time period, so act 7
never fires and no record ever gets code `0x20`.**

| fact | fresh value |
|---|---|
| action-table slot | `0x1106E0 + 0x20*4 = 0x110760` = `ec 4e 08 00` = **`0x84EEC`** |
| body range | `0x84EEC..0x85213` (`0x85214` = row `0x21` prologue) |
| arm table | `0x84ED0` = `{0x84F5C,0x85056,0x8512E,0x85153,0x85197,0x851E1,0x85206}` |
| body references | `get_xrefs_to 0x84EEC` = **1** (DATA from `0x110760`) — the slot is the sole reader |
| installer | `0x899A2` = `BA 20 00 00 00` (`MOV EDX,0x20`), `0x899AB` = `E8 F4 3F FF FF` (`CALL 0x7D9A4`) |
| installer site | arm 0 of `0x89868` (phase slot `0x110808` = `[0x1D]`); `get_xrefs_to 0x89868` = **1** (DATA from `0x110808`) |
| route | act `7` (act table `0x1107EC[7]` = `0x89868`) ← situation `8` ← period transition `FUN_0004B02C` (`0x4B0D9`) |
| engine slot | `fifa96_match_action_table[0x20] = {0x20, NULL, …}` (`handlers.c:5031`) — unported |

---

## 2. The two native anchors (fresh)

* **Action-table slot.** Fresh `read_memory 0x1106E0` (180 B = 45 dwords) decodes
  slot `0x20` (byte offset `0x80`) = `ec 4e 08 00` → **`0x00084EEC`**; fresh
  `read_memory 0x110760` (4 B) = `ec 4e 08 00` = `0x84EEC` (the slot is the
  base `0x1106E0 + 0x80`). Fresh `get_xrefs_to 0x84EEC` = **1**, DATA from
  `0x110760` — the body has **no other reference** (no direct call; it is
  reached only through `CALL dword[rec+0x18]` after an install).
* **Body prologue shape.** Fresh `disassemble_bytes 0x84EEC` (128 B): the
  standard per-record handler prologue `PUSH EBX/ECX/EDX/ESI/EDI/EBP;
  MOV EBP,EAX` (`0x84EEC` = `53 51 52 56 57 55; 89 C5`) — `EBP = rec`,
  confirming the action-table entry contract (`EAX = rec`).

---

## 3. The body `0x84EEC` — prologue, arm dispatch, and the 7 arms

### 3.1 Prologue (`0x84EEC..0x84F5B`, fresh)

```
0x84eec PUSH EBX/ECX/EDX/ESI/EDI/EBP ; MOV EBP,EAX     ; EBP = rec
0x84ef4 MOV ECX,0x9f0
0x84ef9 XOR EDX,EDX
0x84efb MOV EAX,0x14c114
0x84f00 MOV dword[0x14c114],EDX        ; clear the per-side input word 0
0x84f06 MOV dword[0x14c118],EDX        ; clear the per-side input word 1
0x84f0c MOV dword[0x14c11c],ECX        ; [0x14c11c] = 0x9f0
0x84f12 CALL 0x4c31c                   ; (FUN_0004c31c = bare RET — no effect)
0x84f17 MOV EAX,dword[EBP+0x8f] ; SAR EAX,0x18   ; AL = byte[rec+0x92] = stage92
0x84f20 CMP EAX,3 ; 0x84f23 JGE 0x84f2b
0x84f25 MOV dword[0x157a83],EBP        ; stage92 < 3 -> re-assert the ball-camera rec
0x84f2d MOV ESI,dword[EBP+0x89] ; MOV AX,word[0x157a64] ; ADD ESI,EAX
0x84f41 MOV dword[EBP+0x89],ESI        ; timer89 += word[0x157a64]
0x84f3b MOV AL,byte[EBP+0x92]          ; AL = stage92 (arm index)
0x84f47 CMP AL,6 ; JA 0x8520d          ; > 6 -> epilogue
0x84f4f AND EAX,0xff
0x84f54 JMP dword CS:[EAX*4 + 0x84ed0] ; 7-arm sub-dispatch
```

Fresh `read_memory 0x84ED0` (32 B) = `{0x84F5C,0x85056,0x8512E,0x85153,0x85197,
0x851E1,0x85206}` followed by the next function's prologue bytes `53 51 52 56`
(`PUSH EBX/ECX/EDX/ESI`). Byte-for-byte FU-82 §3.7 / FU-189 §5.1.

The prologue is `CALL 0x4C31C` = `FUN_0004c31c` (fresh decompile = `return;`) —
a **no-op**; the input-word seeding is the only prologue state besides the
timer and the `[0x157A83]` re-assert.

### 3.2 Arm 0 (`0x84F5C..0x85055`) — ball-camera place + anim reset + restart commit

Fresh `disassemble_bytes 0x84F5C` (250 B, 63 insns). This is the arm the row is
installed to run first (`stage92 = 0`):

```
0x84f5c MOV EDX,0x15774c ; XOR EAX,EAX ; MOV dword[0x157a83],EBP
0x84f69 CALL 0x73dc4                          ; ball-camera stage
0x84f6e..0x84f7d PUSH [0x157754],[0x157750],[0x15774c],0 ; CALL 0x700f4  ; camera reset
0x84f89 XOR AH,AH ; 0x84f90 MOV byte[0x15781d],AH ; CALL 0x73e08
0x84f9b LEA EDI,[EBP+0x4d]
0x84f9e/9f/a0 MOVSD x3                        ; target triple := ball triple [0x15774c..]
0x84fa1 MOV EBX,[EBP+0x55] ; 0x84fa7 SUB EBX,0xf0 ; 0x84faf MOV [EBP+0x55],EBX
0x84fad MOV EAX,EBP ; 0x84fa4 LEA EDX,[EBP+0x4d] ; 0x84fb2 MOV EBX,0x15774c
0x84fb7 CALL 0x79b6c                          ; *** place rec at the target (writes pos+x/z, anim) ***
0x84fbc MOV EAX,[EBP] ; MOV EAX,[EAX+0x7a6]   ; the ball record
0x84fca MOV dword[EAX+0x4d],0 ; 0x84fd4 MOV dword[EAX+0x55],0xb10
0x84fdb CALL 0x79b6c                          ; place the ball record at (0,?,0xb10)
0x84fe0 CMP dword[EBP+0x20],0 ; JNZ skip ; CMP byte[[rec]+0x828],0 ; JZ skip
0x84ff4 CALL 0x7876c                          ; the merge helper
0x84ff9 MOV EDX,0x26 ; MOV EAX,[EBP] ; XOR EBX,EBX ; CALL 0x8cfac   ; anim 0x26 on the team block
0x85008 MOV EAX,[EBP] ; MOV EDX,0x26 ; XOR EBX,EBX
0x85012 MOV EAX,[EAX+0x7a6] ; CALL 0x8cfac       ; anim 0x26 on the ball record
0x8501d..0x8502f PUSH 0,0 ; ECX=-1 ; EAX=[0x14c110] ; EBX=0 ; EDX=0 ; CALL 0x4c324
0x85034 MOV EAX,0x1b ; CALL 0x4c374              ; (FUN_0004c374 clears word[0x157a68])
0x8503e MOV DH,byte[EBP+0x92] ; MOV dword[EBP+0x89],0 ; INC DH ; MOV byte[EBP+0x92],DH
```

Fresh bytes confirmed: `0x899a2`-independent; the arm ends `0x8503e..0x85050`
(stage 0 → 1, timer = 0).

### 3.3 Arm 1 (`0x85056..0x8512D`) — the event/ring pick

Fresh `disassemble_bytes 0x85056` (216 B). Gate: `byte[0x15882A] != 0` **and**
`CALL 0x4BEC8() == 0` **and** `CALL 0x7A028()` **and** `timer89 >= 0x3C` (else
epilogue `0x8520D`). Picks a ring/event id from `[0x1587E6]>>24` vs the team
side `[team+0x826]` plus `CALL 0x92AC8()` and `mov eax,{0x26,0x28,0x25,0x27,
0x54}` (`0x850AC/0x850BE/0x850E3/0x850F5/0x8510A`), then
`CALL 0x8F188(EAX=id, EBX=0x20, EDX=rec|ball)` (`0x85111`) — the FU-63 event
sink; advance (`0x85116..0x85128`). No render field written.

### 3.4 Arm 2 (`0x8512E..0x85152`) — wait

Fresh: `CMP dword[EBP+0x89],0x3C; JL 0x8520D` → timer = 0, stage → 1. No render
field.

### 3.5 Arm 3 (`0x85153..0x85196`) — the control-slot predicate

Fresh: `EDX = [EBP+0x20]` (the control slot). If `EDX != 0` and
`word[EDX+6] == 0` → wait while `timer89 <= 0x78`; else if `EDX != 0` →
`CALL 0x78A84(EDX)` (the slot copy; render-invisible). Advance on the predicate.

### 3.6 Arm 4 (`0x85197..0x851E0`) — the kick/commit

Fresh `disassemble_bytes 0x85197` (80 B): `target := ball triple`;
`EAX = dword[EBP+0x69]>>16` (the height word); `CMP EAX,0x40; JG epilogue`;
if a slot, `CALL 0x78AA4(slot)`; then `EDX=0x40; EAX=EBP; EBX=0;
CALL 0x7B9C4` (`0x851C4`) — the **kick-apply** path (which calls
`FUN_0007a490`); timer = 0, stage → +1. `FUN_0007b9c4`→`FUN_0007a490` writes
**pos_x/pos_z** (`param_1+0x59/+0x61`) and the anim (fresh decompile, §4).

### 3.7 Arm 5 (`0x851E1..0x85205`) — snap

Fresh: `CALL 0x79B1C(EBP)`. Fresh decompile of `0x79B1C`: copies
`+0x4D/+0x51/+0x55 := +0x59/+0x5D/+0x61` (target := position), clears
`+0x69/+0x71/+0x9c/+0x65/+0x67/+0x73/+0x75` — **target/velocity only, no render
field**. Then `CMP byte[EBP+0x44],0; JZ epilogue`; else timer = 0, stage → +1.

### 3.8 Arm 6 (`0x85206..0x85213`) — reset

Fresh: `CALL 0x7DAB4(EBP)`; tail `0x8520D POP/…/RET`. Fresh decompile of
`0x7DAB4`: `byte[rec+0x92] = 0xFF`, `dword[rec+0x89] = 0`, then
`FUN_0007D9A4(rec, 0, 0, 0)` (install code 0) — the row reset.

---

## 4. Render fields the body writes (M2 render-read map)

The M2 scene stage reads exactly **four** record fields — `pos_x/pos_y/pos_z`
(`+0x59/+0x5D/+0x61`), `anim_id` (`byte[[rec+0x28]]`), `frame` (`byte[rec+0x3D]`)
and `hidden` (`+0x9A`) — `fifa96_match_run.c:1602..1607` (FU-206 §5 ground
truth, re-read first-hand). Fresh helper decompiles map the body's writes:

| helper | fresh decompile | render fields written |
|---|---|---|
| `0x79B6C(rec, &tgt, ball)` | `[rec+0x59]=tgt.x`, `[rec+0x5D]=…` then `=0`, `[rec+0x61]=tgt.z`; `target:=pos`; clears `+0x69/+0x71/+0x65/+0x67/+0x73/+0x75/+0x9c`; `0x79C50` face; `CALL 0x6E598(rec, {0x26 or 0}, 0)` | **pos_x / pos_y / pos_z**, **anim_id** (+ **frame**, see `0x6E598`) |
| `0x8CFAC(rec, p2, p3)` | `0x6E598(rec, p2, 0)` then `0x6E598(rec + 0xB2*k, p3, 0)` for `k=1..10` | **anim_id** (+ **frame**) on 11 records |
| `0x6E598(rec, anim, frame)` → `0x6E490(rec, rec+0x28, frame)` | `0x6E490`: `byte[rec+0x3D] = frame`; `[rec+0x28] = row_ptr`; writes `+0x2C/+0x34/+0x3E/+0x36` | **frame** (`+0x3D`), **anim_id** (`[[rec+0x28]]`) |
| `0x7B9C4` → `0x7A490` | `FUN_0007a490`: `dword[param_1+0x59] += dword[param_1+0x6b]>>17` and `dword[param_1+0x61] += dword[param_1+0x6d]>>17` when `word[param_1+0x69]>>16 < 0x60`; `CALL 0x6E598(param_1, …)` | **pos_x / pos_z**, **anim_id / frame** |
| `0x79B1C` | `target := position`; clears `+0x69/+0x71/+0x9c/+0x65/+0x67/+0x73/+0x75` | none (target/velocity only) |
| `0x78A84` / `0x78AA4` | copy fields of the control-slot object | none |
| `0x7DAB4` | reset code/stage | none directly |

**Per-arm render writes:** arm 0 → **pos_x/pos_y/pos_z + anim_id + frame** (the
actor record via `0x79B6C`; the team's records + ball via `0x8CFAC`); arm 4 →
**pos_x/pos_z + anim_id + frame** (via `0x7B9C4`→`0x7A490`); arms 1/2/3/5/6 → no
render field. `hidden` (`+0x9A`) is **not** written by the body.

**Verdict:** row `0x20` is **render-capable** — it touches all three M2
render-read families (`pos`, `anim_id`, `frame`) on the actor record and on the
team records the `0x8CFAC` walk reaches (and the ball record). It does **not**
write `hidden`.

---

## 5. Gates

| scope | gate (fresh) | effect |
|---|---|---|
| prologue | `byte[rec+0x92] (stage92) < 3` (`0x84F17..0x84F25`) | re-assert `[0x157A83] = rec` |
| arm dispatch | `AL (stage92) <= 6` (`0x84F47`) | else epilogue — stage > 6 no-op |
| arm 1 | `byte[0x15882A] != 0` **and** `0x4BEC8() == 0` **and** `0x7A028() != 0` **and** `timer89 >= 0x3C` (`0x85056..0x8507D`) | else epilogue (the arm does not advance) |
| arm 2 | `timer89 >= 0x3C` (`0x8512E`) | else wait |
| arm 3 | slot `[rec+0x20]`; `word[slot+6] == 0` → wait `timer89 > 0x78` (`0x85158..0x85173`) | slot-dependent advance |
| arm 4 | `dword[rec+0x69]>>16 <= 0x40` (the height word, `0x851A2..0x851AB`) | else epilogue |
| arm 5 | `byte[rec+0x44] != 0` (the ack byte, `0x851E8`) | else wait |
| arm 6 | none | reset |

The `0x7D9A4` install itself is gated by the installer contract (fresh
`decompile_function 0x7D9A4`): applies only when
`rec != NULL && byte[rec+0x9A] == 0 && byte[rec+0x91] != code`.

---

## 6. How the installer `0x899A2` reaches row `0x20`

### 6.1 The install bytes + the frame (fresh)

Fresh `read_memory 0x8999C` (60 B) — the arm-0 tail of the phase-`0x1D` handler
`0x89868`:

```
0x8999c 89 90 b2 07 00 00   MOV [EAX+0x7B2], EDX      ; [actor-team+0x7B2] = actor rec
0x899a2 ba 20 00 00 00      MOV EDX, 0x20            ; <<< ROW-0x20 CODE
0x899a7 8b 44 24 08         MOV EAX, [ESP+0x8]       ; EAX = [0x1587D4] (actor rec)
0x899ab e8 f4 3f ff ff      CALL 0x7D9A4            ; install row 0x20 (deferred)
```

Byte-exact to FU-189 §3.3 / FU-194 §3.2 / FU-205 §2.1. The install frame is
**arm 0** of `0x89868`: fresh `disassemble_bytes 0x89868` (120 B) confirms
`0x89871 MOV EAX,[0x1587D4]` → `[ESP+8]` = the **actor record**, and
`[ESP+0xC] = [[actor]+0x7A6]` = the ball record. `0x89998 XOR ECX,ECX` (before
`0x899A2`) forces the `0x7D9A4` invoke flag to 0 → the install is **deferred**
(no invoke-now): the body runs only when the record's per-record action machine
next calls `[rec+0x18]`.

### 6.2 The install contract (fresh `decompile_function 0x7D9A4`)

`FUN_0007D9A4(rec, code, stage, invoke)`: guard `rec && byte[rec+0x9A]==0 &&
byte[rec+0x91] != code`; writes `byte[rec+0x91] = code`, `[rec+0x18] (param_1[6])
= (&PTR_LAB_001106e0)[code]` (= `0x84EEC` for code `0x20`), `[rec+0x89]=0`,
`byte[rec+0x9E]=0`, `byte[rec+0x92]=stage`, `word[rec+0x7B]=word[rec+0x79]`;
calls `[rec+0x18]` iff `invoke != 0`. Fresh `get_xrefs_to 0x7D9A4` = **77**;
`0x899AB` (row `0x20`) and `0x899CD` (the sibling ball row `0x1F`) are both in
the set.

### 6.3 The sole route to `0x89868` (fresh)

Fresh `get_xrefs_to 0x89868` = **1**, DATA from `0x110808`. Fresh
`read_memory 0x110808` = `68 98 08 00` = `0x89868` = `0x110794 + 0x1D*4`
(phase-table slot `0x1D`). There is **no direct call** — the only dispatch is
the phase seam. Fresh `read_memory 0x1107EC` (act table) confirms act `7`
(index 7) = `68 98 08 00` = `0x89868`, i.e. **act `a` → phase `0x16+a`** (the
act-7 / phase-`0x1D` dual identity).

### 6.4 The producer chain (cross-cited + fresh anchors)

From FU-194 §3.4/§1.1 (frozen):

* **sit 8 → act 7.** The sit-8 arm of `FUN_0008A938` (`0x8AEBA`) does
  `MOV EBX,1; MOV EAX,7; XOR EDX,EDX; CALL 0x888FC` — invoke-now (`BX=1`), so
  `0x89868` arm 0 runs the same frame and executes `0x899A2`.
* **The sit-8 producer** is the period/half transition `FUN_0004B02C`
  (`0x4B0D9`, `MOV EAX,8`), reached when
  `(byte[0x14C1D4] | word[0x14C1D6]) & 0x100` — the **extra-time branch** —
  which also sets `[0x157AC2] = 4`. No engine source dispatches situation 8
  (the dispatched set is 1/2/3/6/9/0xB/0xC).
* **The session-gate queue trap.** `FUN_0008A938` routes sit 8 to the table-1
  *queue* arm (screen-id setter, no act dispatch) when the session gate
  `byte[0x14C32A] != 0` and `dword[0x15B6C0] == 0`; only with the gate off or a
  pending situation does it reach the act-7 arm (FU-194 §3.4 / FU205 §4 cell 7).

**So:** `0x899A2` is reached iff act 7 fires, which needs the situated period
transition (extra-time flag / `input_range & 0x100`) with the session gate clear
or pending, and the actor-record `[0x1587D4]` producer.

---

## 7. Engine wiring — what must be added, and the M2 verdict

### 7.1 Current engine state (fresh source)

* **Action slot.** `fifa96_match_action_table[0x20] = {0x20, NULL, "<FU-189 …>"}`
  (`fifa96_match_handlers.c:5031`) — the body is **unported**; the row is NULL
  and uncounted; `fifa96_match_dispatch_action(mr, 0x20)` returns `UNSUP`
  (`tests/test_engine_match_handlers.c:964`, `test_action_20_phase_1d_gated_leg`).
* **Phase slot.** `fifa96_match_phase_table[0x1D] = {0x1D, NULL, …}`
  (`handlers.c:5118`). The body `fifa96_match_action_phase_1D`
  (`match_run.c:5998`) is ported but its **arm 0 explicitly legs the `0x899A2`
  install** (`match_run.c:6017`) — the actor `[0x1587D4]` / ball-row producers
  have no engine cell, and a fake install is forbidden.
* **The act seam refuses a NULL phase row.** `fifa96_match_act_dispatch`
  (`fifa96_match_act.c:51`) and `fifa96_match_act_tick` (`:63`) both return
  `OK` before dispatch when `phase_table[phase].fn == NULL`.

### 7.2 What the engine must add

1. **The row body** `fifa96_match_action_20` (the 7 arms of §3; table `0x84ED0`;
   render writes pos/anim/frame via the `0x79B6C`/`0x8CFAC`/`0x7A490` helpers).
2. **The action-table row** — flip `fifa96_match_action_table[0x20].fn` from
   `NULL` to `fifa96_match_action_20`.
3. **The coupled installer** — implement the phase-`0x1D` arm-0 `0x899A2`
   deferred install on the actor record (currently a leg at `match_run.c:6017`),
   plus the **actor-record `[0x1587D4]` cell** (the producer is unported,
   FU194-L8).
4. **The live sit-8 producer** — the engine's `fifa96_match_run_period_setup` +
   `period_setup_live` (gated off, R9 / FU196-L8), the extra-time flag (OL-85),
   the `session_gate_14c32a` queue gate, and the `fifa96_match_act_dispatch(7,
   0, …)` + clock-tail tick.

Steps 1–2 alone install nothing (no producer); steps 1–4 are one coupled unit
(FU-194 §6.1): phase `0x1D` and row `0x20` stand or fall together.

### 7.3 Census consequence (per the honest-counting rule)

A mechanical `NULL → fn` flip of row `0x20` would move the census
`51/80 = 63% → 52/80 = 65%` (action `32 → 33`). **This recon slice asserts no
number.** Per controller Ruling R8 / the rows-`27`/`29`/`2C` precedent (a ported
body with no reachable installer stays NULL/uncounted), row `0x20` should stay
`NULL` until the coupled installer + sit-8 producer land. No engine change is
made by this slice; the verified census stays **`51/80 = 63%`**.

### 7.4 Does wiring it change the tape (M2)? — precise

* **As a dormant row (steps 1–2 only): NO.** The tape stages no extra-time
  period / situation 8 (`tests/test_engine_m2.c`: the tape drives begin →
  kickoff → move → kick → the forced 0x13/0x14 window → the band scenario → the
  goal → the post-goal hold; `ref_machine == REF_NONE`, no extra-time). The
  per-record action machine invokes `[rec+0x18]` only for records whose code was
  installed by a live producer; no record ever carries code `0x20`, so
  `fifa96_match_action_20` is dispatched **0 times** and M2 is byte-identical
  (no re-pin).
* **If the coupled route also lands and the installer fires: YES — tape-visible.**
  The fields that move are exactly the three M2 render-read families:
  **`pos_x/pos_y/pos_z`** (arm 0 `0x79B6C`; arm 4 `0x7A490`) and
  **`anim_id` + `frame`** (arm 0 `0x79B6C`/`0x8CFAC`; arm 4 `0x7A490`, all via
  `0x6E598 → 0x6E490`). The render-invisible writes are the target triple
  (`+0x4D/+0x51/+0x55`), the velocity words, the control-slot object and the
  `[0x14C11x]`/`[0x157A68]` process cells. `hidden` (`+0x9A`) does not move. Any
  such landing must `cmp`-pin the M2 tape first (AGENTS.md golden rule).

---

## 8. Numbered open legs

1. **FU210-L1 — the row-0x20 helper port spec (OL-9).** `0x4C31C` (no-op),
   `0x79B6C`, `0x8CFAC`, `0x6E598`/`0x6E490`, `0x79C50` face, `0x73DC4`/
   `0x700F4`/`0x73E08` (ball camera), `0x78A84`/`0x78AA4` (slot), `0x7B9C4`/
   `0x7A490` (kick), `0x79B1C` (snap), `0x7DAB4` (reset), `0x7876C` (merge),
   `0x8F188` (event), `0x92AC8` (RNG), `0x4C324`/`0x4C374` are cited by address
   and decompiled first-order above; the full byte-for-byte port is the
   OL-9 open class (FU-81/FU-82). The `[0x14C11x]`/`[0x1577xx]`/`[0x157Axx]`
   cells are not fully resolved.
2. **FU210-L2 — the install `move`/place semantics of `0x79B6C`.** Fresh
   decompile shows `[rec+0x59/+0x5D/+0x61] := *param_2` (then `+0x5D := 0`) with
   `param_2 = &rec+0x4D` already holding the ball triple, i.e. the actor record
   is teleported to `(ball.x, 0, ball.z-0xF0)`. Which native records the two
   `0x8CFAC` calls reach (team base vs ball record; the 11-stride walk) is
   derived but not field-verified. (FU-82 §2.1 context.)
3. **FU210-L3 — the actor-record `[0x1587D4]` producer.** The installer target;
   no engine cell exists (FU194-L8). Whether it is always an outfield record
   (`index != 0`, so the per-record action machine invokes it) is unproven.
4. **FU210-L4 — the sit-8 producer gates.** `FUN_0004B02C` (`0x4B095`/`0x4B0D9`),
   the extra-time flag `[0x157AC2]`/`[0x157AC0]` (OL-85, FU195-L2), the
   `input_range & 0x100` branch and the `session_gate_14c32a` queue trap
   (FU205-L3). Not decomposed here; owner the phase-`0x1D`/row-`0x20` coupled
   unit (FU194-L1/L8, FU205-L2).
5. **FU210-L5 — the count rule.** Row `0x20` stays `NULL`/uncounted until the
   coupled installer + sit-8 producer land (R8 / the rows-`27`/`29`/`2C`
   precedent). This slice does not assert a census number; the current
   `51/80 = 63%` is unchanged.

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, image base
  `0`, `min_address 0x10000`.
* `read_memory` — `0x1106E0` (180 B, action table), `0x110760` (4 B, slot `0x20`),
  `0x84ED0` (32 B, the 7-arm table), `0x110808` (8 B, phase slots `0x1D`/`0x1E`),
  `0x1107EC` (56 B, act table), `0x8999C` (60 B, the row-`0x20` install + the
  `0x899CD` sibling).
* `disassemble_bytes` — `0x84EEC` (128 B, prologue), `0x84F5C` (250 B, arm 0),
  `0x85056` (216 B, arm 1), `0x8512E` (240 B, arms 2/3/4), `0x85197` (80 B, arm
  4/5), `0x89868` (120 B, the phase-`0x1D` head + arm-0 prologue).
* `decompile_function` — `0x7D9A4` (installer contract), `0x8CFAC`, `0x79B6C`,
  `0x7B9C4`, `0x79B1C`, `0x6E598`, `0x6E490`, `0x79C50`, `0x7DAB4`, `0x4C324`,
  `0x4C31C`, `0x7A490`, `0x78A84`, `0x78AA4`, `0x7876C`.
* `get_xrefs_to` — `0x84EEC` (**1**, DATA `0x110760`), `0x89868` (**1**, DATA
  `0x110808`), `0x7D9A4` (**77**).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(the action table `:4967..5096`, the row-`0x20` note `:5031`, the phase table
`:5098..`, the phase-`0x1D` note `:5118`, `dispatch_action`/`dispatch_row`
`:5130..5145`, the row-`0x24` port `:4493..`); `src/fifa96_engine/fifa96_match_run.c`
(`match_run_scene_stage` `:1595..1607`, the per-record action dispatch
`:1380..1415`, the phase-`0x1D` port `:5998..`); `src/fifa96_engine/fifa96_match_act.c`
(`:38..65`, the act seam + tick); `include/fifa96_engine/fifa96_match_entities.h`
(`:40..135`, the record field map; `fifa96_match_entities_install` `:400`);
`include/fifa96_engine/fifa96_match_run.h` (`:1520..1571`, the phase-`0x1D`/`1E`
decls); `tests/test_engine_match_handlers.c` (`:955..977`, the row-`0x20` leg
test); `tests/test_engine_m2.c` (`:2990..3002`, the phase-slot assertions).

Docs cross-cited: FU-82 (§2.1/§3.7/§4), FU-137 §6, FU-189 (§1/§3/§5/§6/§10),
FU-190 §6.2, FU-194 (§1/§3/§6.1/§7), FU-196 (§3/§4), FU-198 §5, FU-202 §6,
FU-205 (§2/§3/§4/§6), FU-206 (§1/§4/§5), ENGINE.md census line, AGENTS.md.

No writes beyond this slice file; no rename/label/comment/function/script/project
save; no git.

---

## Landing (M2 phase-19 T3, 2026-10-13)

T3 ports the row-`0x20` body and its coupled phase-`0x1D` arm-0 installer, but
row `0x20` **stays a leg (uncounted)** — FU210-L5 holds.

* **The body `0x84EEC` is PORTED** as `fifa96_match_action_20`
  (`src/fifa96_engine/fifa96_match_handlers.c`): the prologue
  (`[0x157A83]` re-assert when `+0x92 < 3`, `+0x89 += delta`, the 7-arm
  dispatch) and the arms — 0 (the `0x73E08` commit + the `0x79B6C` place
  `pos_x/y/z` + the `0x8CFAC` anim `0x26`), 1/2 (the wait/advance), 3 (the
  control-slot predicate), 4 (the `0x7B9C4` mode-`0x40` kick `pos_x/pos_z` +
  anim via `0x7A490`), 5 (the `0x79B1C` snap + `+0x44` ack), 6 (the `0x7DAB4`
  reset). The §3 render writes are pinned (arm 0 pos+anim; arm 4 pos+anim;
  `hidden` `+0x9A` unwritten). The §8 legs remain legs: **FU210-L1** (the
  `0x73DC4`/`0x700F4` ball-camera stage, the `0x4BEC8`/`0x7A028` gates, the
  `0x8F188` event, the `0x4C324`/`0x4C374` sinks, the `[0x14C11x]` input words),
  **FU210-L2** (the ball-record place at `(0, 0xB10)`; the 11-stride `0x8CFAC`
  walk).
* **The coupled arm-0 installer `0x899A2` is PORTED**
  (`fifa96_match_run.c`, `fifa96_match_action_phase_1D` arm 0):
  `fifa96_match_entities_install(actor, phase, 0x20, 0)` (the `FUN_0007D9A4`
  equivalent), deferred/stage-0, gated on the new `mr->actor_1587d4` cell. The
  0x899CD ball row-`0x1F` install and the `0x7A028` reset stay FU194-L7/L8 legs.
* **The row stays NULL/uncounted.** The installer target `[0x1587D4]` has no
  live producer (`0x8AE4B` `[0x1587CC + side*4] -> [0x1587D4]`; the referenced
  record's identity is the FU-145 / **FU210-L3** open leg), so `mr->actor_1587d4`
  stays `NONE` and no record carries code `0x20` — the row cannot dispatch. Per
  the binding census rule and the rows-27/29/2C precedent the slot stays `NULL`
  (the body is exported for direct unit testing; the installer is fixtured).
  **FU210-L5 is unchanged** (row `0x20` uncounted until the actor-cell producer
  + the sit-8 route land). FU189-L1 (the installer hunt) is RESOLVED.
* **M2 verdict:** byte-identical (`441415fc…`; the tape reaches no sit-8 and the
  cell defaults `NONE`); M1 `09b726b7…` unmoved. Census unchanged `52/80 = 65%`.
  No prior section rewritten.
