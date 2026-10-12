# FU-191 — the per-record phase seam (`FUN_0008D098`'s 11-record walk)

Provenance: M2 phase-17 wave **R39**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument). Every address/byte/xref below is a fresh `disassemble_function` /
`disassemble_bytes` / `decompile_function` / `read_memory` / `get_xrefs_to` of
**this session** unless marked *cross-cited* (then the frozen FU doc is named).
Engine source read first-hand (`fifa96_match_act.c`,
`fifa96_match_handlers.c`, `fifa96_match_run.c`,
`include/fifa96_engine/fifa96_match_entities.h`, `tests/test_engine_m2.c`).
No writes: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; no git commands. The alias traps are applied
(`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`).

Scope: derive `FUN_0008D098`'s **full** per-record walk byte-level — the
`[0x157A4A]` phase selector's `SAR EAX,0x18` decomposition, the `0x110794` table
index, the `[rec+0x1C]` store, the `CALL dword[ECX+0x1C]`, the `0xB2` stride and
the `[rec+0x9A]` skip; derive the **per-record handler ABI** the `0x6Dxxx` bodies
expect; then give the **engine-conversion rec** for wiring the walk as the second
caller of `fifa96_match_dispatch_phase` (FU175-L1) without perturbing the act
seam (`fifa96_match_act.{c,h}`) or the render, with an honest **tape-neutrality
argument**.

---

## 1. `FUN_0008D098` — the two halves

Fresh `disassemble_function 0x8D098` (583 insns, body `0x8D098..0x8D820`) and
fresh `decompile_function 0x8D098` split the body into exactly two halves:

* the **record walk** (`0x8D098..0x8D167`): the prologue, `FUN_0006D9C4`
  pre-stage, `FUN_0008C418`, the per-team housekeeping zeroes, and the 11-record
  loop that stages and invokes `[rec+0x1C]`;
* the **phase switch** (`0x8D168..0x8D820`): `switch(byte[0x157A4D])` via the
  `0x8D040` jump table — the per-team *arm* body the engine already mirrors as
  `fifa96_match_run_phase_arm` (§5).

The whole function is called **once per team** by the phase writer
`FUN_000740A0` (§4). This slice is only the first half (the walk).

---

## 2. The 11-record walk — byte-level

Fresh `read_memory 0x8D0F2` (128 B) gives the raw loop bytes; fresh
`disassemble_function 0x8D098` gives the mnemonics. The loop region
(`0x8D0F2..0x8D167`):

```
0x8D0F2 LEA ESI,[EBP+0x4D]          ; ESI = &rec[0x4D]  (record 0 target block)
0x8D0F5 JMP 0x8D159                 ; enter at the guard
; --- loop body (0x8D0F7) ---
0x8D0F7 CMP byte[ECX+0x9A],0x0      ; 80 b9 9a 00 00 00 00   ; the skip byte
0x8D0FE JNZ 0x8D123                 ; occupied record -> skip the body
0x8D100 MOV EAX,ECX                 ; EAX = rec
0x8D102 MOV EBX,0xFFFFFFFF          ; EBX = -1  (the third arg)
0x8D107 CALL 0x6D920                ; e8 14 08 fe ff         ; stage [rec+0x1C]
0x8D10C MOV EDX,ESI                 ; EDX = &rec[0x4D]  (the second arg)
0x8D10E MOV EAX,ECX                 ; EAX = rec         (the first arg)
0x8D110 CALL dword[ECX+0x1C]        ; ff 51 1c               ; the phase handler
0x8D113 MOV EBX,[ESP+0xF8]          ; EBX = &rec[0x65]
0x8D11A MOV EDX,ESI                 ; EDX = &rec[0x4D]
0x8D11C MOV EAX,EDI                 ; EAX = &rec[0x59]
0x8D11E CALL 0x8DCD4                ; e8 b1 0b 00 00         ; distance triple
; --- increment (0x8D123) ---
0x8D123 MOV EAX,[ESP+0xFC]          ; the counter word (low half at 0xFC)
0x8D12A MOV EDX,[ESP+0xF8]
0x8D131 ADD EDI,0xB2                ; 0x59 pointer
0x8D137 ADD ESI,0xB2                ; 0x4D pointer
0x8D13D ADD ECX,0xB2                ; the record pointer (stride 0xB2 = 178)
0x8D143 INC EAX
0x8D144 ADD EDX,0xB2                ; the 0x65 pointer
0x8D14A MOV word[ESP+0xFC],AX
0x8D152 MOV [ESP+0xF8],EDX
; --- guard (0x8D159) ---
0x8D159 MOV EAX,[ESP+0xFA]          ; alias trap: the word at 0xFA+2 == 0xFC
0x8D160 SAR EAX,0x10                ; sign-extend that word
0x8D163 CMP EAX,0xB                 ; index < 11
0x8D166 JL 0x8D0F7                  ; -> records 0..10
```

**Four moving pointers, all starting at record 0** (`EBP` = the team base =
record 0): `ECX` = the record, `ESI` = `rec+0x4D`, `EDI` = `rec+0x59`,
`[ESP+0xF8]` = `rec+0x65`; each advanced by `0xB2` per iteration. The loop count
is the *word* at `[ESP+0xFC]`, read back through the alias `[ESP+0xFA]>>16`
(`SAR EAX,0x10`), bounded `< 0xB` = **11 records**. The counter is initialised at
`0x8D0BF MOV word[ESP+0xFC],DX` immediately after `0x8D0B8 XOR EDX,EDX;
0x8D0BA CALL 0x8C418`; fresh `disassemble_function 0x8C418` (186 insns) shows
`PUSH EDX` at `0x8C41A` / `POP EDX` at `0x8C68D`/`0x8C753` — **EDX is
callee-saved**, so the start is the pre-call `0` and the walk is records `0..10`.

Two stores per non-skipped record: the target triple `[rec+0x4D/+0x51/+0x55]`
(written by the invoked `[rec+0x1C]` handler into its `param_2`) and the
distance triple `[rec+0x65/+0x67/+0x69]` (written by `FUN_0008DCD4`).

### 2.1 The record stride and the `[rec+0x9A]` skip

The stride `0xB2` (178) is `81 c7 b2 00 00 00` / `81 c6 b2 00 00 00` /
`81 c1 b2 00 00 00` / `81 c2 b2 00 00 00` — the exact native record size the
engine's `struct fifa96_match_entity` header names ("stride 0xB2",
`include/fifa96_engine/fifa96_match_entities.h:58`). 11 records occupy
`11 × 0xB2 = 0x7A6` bytes; the next team-block field is `[team+0x7AE]` (read at
`0x8D0A6`), which is why the team block ends at `+0x7AE`.

The skip byte `[rec+0x9A]` (fresh bytes `80 b9 9a 00 00 00 00` = `CMP
byte[ECX+0x9A],0`) is the **occupied-record exclusion** the engine models as
`struct fifa96_match_entity.skip_9a` (`entities.h:83`, comment "+0x9A exclusion
(occupied record)"). It is read by the render (`match_run_scene_stage`:
`re->stage.hidden = e->skip_9a ? 1 : 0`, `fifa96_match_run.c:1607`) and written
by the walk's own switch — case 1 sets `[rec+0x9A]=1` (`0x8D211`) then clears
`[rec+0x9A]=0` (`0x8D22C`), i.e. the walk's staged record is temporarily hidden
from the second `FUN_00079CCC` pick. **The walk itself only reads `+0x9A`; it
never writes it** (the `0x9A` writes are in the switch, `0x8D211`/`0x8D22C`).

### 2.2 `FUN_0008DCD4` — the distance writer

Fresh `decompile_function 0x8DCD4` (`void FUN_0008dcd4(short *p1, short *p2,
short *p3)`): `p3[1] = p2[0]-p1[0]` (dx), `p3[2] = p2[4]-p1[4]` (dz), and `p3[0]
= ` an octagonal fast-length over `|dx|,|dz|`. With the walk's arguments that is
`p1 = &rec[0x59]` (position), `p2 = &rec[0x4D]` (target), `p3 = &rec[0x65]` —
writing the word triple `+0x65` (length) `+0x67` (dx) `+0x69` (dz). This is the
per-record target-minus-position metric the phase-2 clamp/`FUN_00079CCC` pick
consume (the engine's `fifa96_action_locomotion_clamp_placement` /
`match_run_phase_pick` derived the same metric from the target triple,
`fifa96_match_run.c:2463..2477`).

---

## 3. The phase selector `[0x157A4A]` — the `SAR EAX,0x18` decomposition

The stager `FUN_0006D920` is the sole reader of the phase table (fresh
`get_xrefs_to 0x110794` *cross-cited* FU-182/FU-190 §1 = 2, both here). Fresh
`disassemble_function 0x6D920` (51 insns), tail `0x6D9A8..0x6D9BA`:

```
0x6D9A8 MOV EAX,[0x00157A4A]
0x6D9AD SAR EAX,0x18              ; arithmetic >>24 -> sign-extended byte 0x157A4D
0x6D9B0 SHL EAX,0x2
0x6D9B3 ADD EAX,0x110794          ; the phase-table base
0x6D9B8 MOV EAX,[EAX]             ; phase_table[phase]
0x6D9BA MOV [EDX+0x1C],EAX        ; rec+0x1C = the per-record phase handler
```

`MOV EAX,[0x157A4A]` is a **dword** load at `0x157A4A`; `SAR EAX,0x18` takes its
top (sign-extended) byte, i.e. the **byte at `0x157A4D`** — the current phase.
The writer `FUN_000740A0` (§4) confirms the layout: `MOV [0x157A4D],AL`
(`0x740AC`) stores the new phase, `MOV [0x157A4E],AH` (`0x740A6`) the previous
phase, and `MOV byte[0x157AAF],DL` (`0x740BA`) the write's side. Fresh
`read_memory 0x157A48` (16 B) is all zero (BSS; runtime-populated).

The related sub-gate `byte 0x157A4C` is `dword[0x157A49]>>24` (the alias trap the
engine acts on as `[0x157A49]>>24`; *cross-cited* FU-177 §3). The phase index is
always `0x157A4D`, distinct from `0x157A4C`.

So the stager is: **`rec+0x1C := 0x110794[byte 0x157A4D]`** — the per-record
handler pointer for the *current* phase, staged onto every visited record and
then called at `0x8D110`.

---

## 4. The walk's call site — `FUN_000740A0`, once per team

Fresh `disassemble_function 0x740A0` (25 insns) — the phase writer:

```
0x740A0 MOV AH,[0x157A4D]          ; old phase
0x740A6 MOV [0x157A4E],AH          ; prev-phase store
0x740AC MOV [0x157A4D],AL          ; AL = the new phase (write)
0x740B1 MOVSX EAX,DX               ; DX = the write's side (0/1)
0x740B4 IMUL EAX,EAX,0x835
0x740BA MOV byte[0x157AAF],DL      ; the controlled-side gate byte
0x740C0 ADD EAX,0x1588A4           ; team base = 0x1588A4 + side*0x835
0x740C5 XOR DL,0x1
0x740C8 CALL 0x8D098               ; walk team[side]
0x740CD MOVSX EAX,DX
0x740D0 IMUL EAX,EAX,0x835
0x740D6 ADD EAX,0x1588A4
0x740DB CALL 0x8D098               ; walk team[side^1]
0x740E0 MOV EAX,[0x157A4A]; SAR EAX,0x18; CMP EAX,0x2; JNZ ...  ; phase==2 tail
```

Fresh `get_xrefs_to 0x8D098` = **2**, both in `FUN_000740A0` (`0x740C8`/`0x740DB`)
— the walk's **only** callers. So `FUN_0008D098` runs once per team, immediately
after each phase write, with the *new* phase already latched in `[0x157A4D]`.

Fresh `get_xrefs_to 0x740A0` = **27** call sites (byte-for-byte the same set as
FU-175 §5 / FU-182 §1); fresh `get_xrefs_to 0x6D920` = **4** (`0x79983`
`FUN_0007997C`, `0x6DA0A` `FUN_0006D9C4`, `0x8D107` this walk, `0x8CF73`
`FUN_0008CF60`). The walk is therefore the phase writer's per-team record pass.

---

## 5. The walk prologue — the pre-pass and team housekeeping

Before the loop (`0x8D0A4..0x8D0F5`):

* `0x8D0A4 MOV EBP,EAX` — `EBP` = `param_1` = the team base.
* `0x8D0A6 MOV EDX,[EAX+0x7AE]; 0x8D0AC MOVSX EDX,byte[EDX]; 0x8D0B1 CALL
  0x6D9C4` — `FUN_0006D9C4(team, (short)byte[[team+0x7AE]])`. Fresh
  `decompile_function 0x6D9C4`: stages `[team+0x7AE] = 0x11033A + form*0x1D`,
  then for 4 groups walks the form table byte-group and for each entry writes
  `[rec+0x90] = *group` and calls `FUN_0006D920(rec)` (a formation pre-stage of
  `[rec+0x1C]`). Then `[team+0x7DB]`/`[team+0x7DF]` from `FUN_0004C384`.
  Fresh `get_xrefs_to 0x6D9C4` = 2 (`0x8CE87` `FUN_0008CE78`, `0x8D0B1` here).
* `0x8D0B6 MOV EAX,EBP; 0x8D0B8 XOR EDX,EDX; 0x8D0BA CALL 0x8C418` —
  `FUN_0008C418(team, 0)`. Fresh `decompile_function 0x8C418`: writes the
  team block `+0x80C..+0x824`, `+0x82D` (the mode byte), `+0x82F` — the
  per-team camera/bound setup (the phase-2 clamp bounds the engine models as
  `bound`/`clamp_placement` inputs).
* `0x8D0C7 MOV [EBP+0x7CB],0`; `0x8D0D4 MOV [EBP+0x7B6],0`; `0x8D0E1 MOV
  [EBP+0x7BF],0` — the team's `timer7cb`/`second`/`chosen` cells zeroed
  (the engine's `struct fifa96_match_team.timer7cb`/`second`/`chosen`,
  `entities.h:176..193`).
* `0x8D0D1 LEA EDI,[EBP+0x59]`; `0x8D0DE LEA EAX,[EBP+0x65]`; `0x8D0EB MOV
  [ESP+0xF8],EAX`; `0x8D0F2 LEA ESI,[EBP+0x4D]` — the four loop pointers (§2).

So the walk = [pre-stage `[rec+0x1C]` from the formation] → [team camera/bounds
setup] → [zero three team cells] → [the 11-record loop].

---

## 6. The per-record handler ABI

The walk calls `[rec+0x1C]` with **EAX = rec, EDX = &rec[0x4D], EBX = -1**
(`0x8D100..0x8D110`). Fresh disassembly of four `0x6Dxxx` bodies that the phase
table (`0x110794`, fresh `read_memory` 140 B — see §7) routes to confirms the
shared contract:

| handler | phases | fresh disasm | reads | writes |
|---|---|---|---|---|
| `0x6DE34` | `0`,`0xA`,`0xB`,`0xD`,`0xE`,`0xF` | `0x6DE38 LEA ESI,[EAX+0x59]`, `0x6DE36 MOV EDI,EDX`, 3× `MOVSD` | `[rec+0x59/+0x5D/+0x61]` | `param_2[0..2] = position` (copy) |
| `0x6E1D0` | `1`,`0x12` | `0x6E1D6 MOV ECX,EDX`, `0x6E1EA MOV EDX,[EAX+0x8]`, `0x6E1DE MOV ESI,[EAX]` | `[rec]` (team), `[rec+0x8]` (staged ptr) | `param_2[..]` |
| `0x6DD9C` | `6` | `0x6DDA8 MOV EAX,[EAX+0x10]`, `0x6DDCE MOV [EDX],ECX` | `[rec]`, `[rec+0x10]` | `param_2[0]` |
| `0x6DCC8` | `2`,`8`,`9`,`0x15` | fresh `decompile` | `param_1[3]` = `[rec+0xC]`, `[rec]`, `[0x1577DE+side*4]` | `param_2[1]=0; param_2[0]=x; param_2[2]=z` |

**The ABI (watcom register):**

* **`param_1` (EAX)** = the record pointer `rec`. The handler's `[rec]` is the
  **team back-pointer** (`0x6E1DE MOV ESI,[EAX]` then `[ESI+0x826]` = the side;
  `0x6DCC8`'s `*param_1`); the engine models this as
  `struct fifa96_match_entity.team` (index) with `mr->entities.team[rec->team]`
  giving the team.
* **`param_2` (EDX)** = `&rec[0x4D]` — the **target triple**
  `[rec+0x4D]/[rec+0x51]/[rec+0x55]` (the engine's `target_x`/`target_y`/
  `target_z`, `entities.h:125`). The handler's output.
* **`param_3` (EBX)** = `-1` from the walk. `0x6DCC8` treats `param_3 < 0` as
  "use the per-side default direction index
  `[0x1577DE + side*4] >> 0x10`"; a non-negative `param_3` selects an explicit
  pair index (`0x6DCC8`: `pcVar1 = [rec+0xC] + iVar5*2`).
* **Staged inputs**: `[rec+0x8]`, `[rec+0xC]`, `[rec+0x10]`, `[rec+0x14]` are
  written by `FUN_0006D920` *before* the call (`0x6D95E`/`0x6D970`/`0x6D996`/
  `0x6D9A5`; `[rec+0x8]` = a `FUN_0004C384(anim*6) + [rec+0x8A]>>24*4` pointer,
  `[rec+0xC] = FUN_0004C384(anim*6+1) + [rec+0x8D]>>24*0x256`, etc.). These are
  the per-record metadata pointers the placement bodies read (OL-13).

Row 03 (`0x7E1A4`) is a *second* live caller shape of the same ABI: fresh
`0x7E252`/`0x7E2DD` = `CALL dword[ESI+0x1C]` with `EAX=rec`, `EDX=&rec[0x4D]`,
`EBX=-1` (*cross-cited* FU-177 §3, `ff 51 1c`). So the walk and row 03 call the
same `[rec+0x1C]` slot with the same three-register contract.

---

## 7. The phase table (fresh) and the switch arms

Fresh `read_memory 0x110794` (140 B = 35 dwords), byte-identical to FU-190 §1:

```
[0]=0006DE34 [1]=0006E1D0 [2]=0006DCC8 [3]=0006DE44 [4]=0006DE44
[5]=0006E05C [6]=0006DD9C [7]=0006DE44 [8]=0006DD6C [9]=0006DD6C
[0A]=0006DE34 [0B]=0006DE34 [0C]=0006DF4C [0D]=0006DE34 [0E]=0006DE34
[0F]=0006DE34 [10]=0006E004 [11]=0006E1C8 [12]=0006E1D0 [13]=0006E244
[14]=0006E244 [15]=0006DCC8 [16]=00000000 (the INT3 slot)
[17]=00088DC8 [18]=0008922C [19]=00089FA4 [1A]=00089620 [1B]=000890EC
[1C]=00089110 [1D]=00089868 [1E]=0008A798 [1F]=00088F4C [20]=0008B688
[21]=0008B874 [22]=0008B900
```

The **switch** half (`0x8D178..0x8D820`) indexes the *same* phase byte
`[0x157A4D]` through the `0x8D040` jump table. Fresh `read_memory 0x8D040` (48 B)
+ `0x8D070` (40 B) give the 22 entries (phases `0x00..0x15`; `0x8D040 + 4*p`):

| phase | arm | phase | arm |
|---|---|---|---|
| `0x00` | `0x8D192` | `0x0B` | `0x8D63E` |
| `0x01` | `0x8D1B1` | `0x0C` | `0x8D77B` |
| `0x02` | `0x8D255` | `0x0D` | `0x8D65D` |
| `0x03` | `0x8D2A3` | `0x0E` | `0x8D63E` |
| `0x04` | `0x8D35B` | `0x0F` | `0x8D192` |
| `0x05` | `0x8D433` | `0x10` | `0x8D77B` |
| `0x06` | `0x8D4A2` | `0x11` | `0x8D814` (default) |
| `0x07` | `0x8D57B` | `0x12` | `0x8D814` (default) |
| `0x08` | `0x8D5D9` | `0x13` | `0x8D693` |
| `0x09` | `0x8D5D9` | `0x14` | `0x8D693` |
| `0x0A` | `0x8D192` | `0x15` | `0x8D77B` |

This is the arm body the engine already partially mirrors as
`fifa96_match_run_phase_arm`, gated to phases `{3,4,5,6,7,8,9,0x0D}`
(`fifa96_match_run.c:2482..2484`) — a *subset* of the 22 arms.

---

## 8. The engine seam today

First-hand source:

* `fifa96_match_dispatch_phase(mr, phase)` (`fifa96_match_handlers.c:4700..4705`)
  resolves `fifa96_match_phase_table[phase]` and calls `row->fn(mr)`; it returns
  `-FIFA96_ERR_UNSUPPORTED` for every NULL row. The phase table has **1**
  non-NULL row today (`0x1F`, `fifa96_match_action_phase_1F`), plus `0x18`/`0x19`
  (FU-183/FU-187) — *cross-cited* FU-190 §1. Its only caller is the act seam
  (`fifa96_match_act.c`: `fifa96_match_act_dispatch`, `fifa96_match_act_tick`);
  `grep dispatch_phase src/` shows no per-record caller.
* `match_run_write_phase(mr, phase)` (`fifa96_match_run.c:2372`) is the
  `FUN_000740A0` mirror (writes `mr->state.phase`/`mr->phase_machine`, clears the
  phase-2 arm). It does **not** run a per-record walk.
* `fifa96_match_run_phase_arm(mr, phase)` (`fifa96_match_run.c:2480`) is the
  derived **switch** half (phases `3..9,0xD`), wired at the two write sites
  (`0x2630`, `0x5231`).
* `match_run_scene_stage` (`fifa96_match_run.c:1595`) stages the render from the
  entity **positions** `+0x59/+0x5D/+0x61`, `anim_id` `+0x28`, `frame` `+0x3D`,
  `hidden` `+0x9A` — **not** the target triple `+0x4D` or the distance triple
  `+0x65`.

**What is missing:** the walk's per-record prologue loop
(`0x8D0F2..0x8D166`) — the second invocation surface into the phase table
(FU175-L1). The engine has the *switch* (the arm) but not the *walk*.

---

## 9. T1 engine-conversion rec

**Add the walk as a per-record caller of `fifa96_match_dispatch_phase`**, folded
into `match_run_write_phase` (the `FUN_000740A0` mirror), in
`fifa96_match_run.c` only. Nothing in `fifa96_match_act.{c,h}` changes.

**Walk site.** `match_run_write_phase(mr, phase)`, after the phase-state write,
for both teams (`t` and `t^1`, mirroring `0x740C8`/`0x740DB`), call a new static
`match_run_record_phase_walk(mr, t)`.

**Record loop.** Iterate `mr->entities.team[t].records[0..10]` (the native
`ECX`/`0xB2` walk; one array element = one `0xB2` stride). Skip records whose
`skip_9a != 0` (the `0x8D0FE JNZ`).

**Per-record call.** For each non-skipped record: resolve
`fifa96_match_phase_table[phase].fn`. If `NULL`, **skip** (the unported placement
body — §10). If non-NULL, dispatch with the record threaded (the ABI mismatch:
the engine's `fn(mr)` carries no record — FU175-L4). Two honest options:

1. stage a cursor `mr->phase_walk_rec = &records[i]` before
   `fifa96_match_dispatch_phase(mr, phase)` and have the ported placement rows
   read it (keeps the `fn(mr)` ABI, touches only `fifa96_match_run.h`); or
2. add a record-threaded sibling
   `fifa96_match_dispatch_phase_record(mr, phase, rec, &rec->target_x)` in
   `fifa96_match_handlers.c`.

Option 1 is the smaller seam and matches the FU-175 §6 "absorb into the walk"
shape.

**Selector source.** The phase is the argument `match_run_write_phase` was
called with (the native `AL` written to `[0x157A4D]` at `0x740AC`), **not**
`mr->state.phase` re-read — the native stager reads the just-written byte. The
walk's *side* is the write's side (`mr->phase_machine.side_controlled` on the
sites that set it; a site that does not set it is a leg below).

**Do not** touch `fifa96_match_act.{c,h}` (the act seam keeps its own global
`fifa96_match_dispatch_phase` caller with phase `0x16+act ≥ 0x17` — disjoint
from the walk's game-phase domain `0x00..0x15`), and **do not** touch the render
(§10).

---

## 10. Tape-neutrality argument (honest)

**What the walk writes**, per non-skipped record: (a) the staged metadata
pointers `[rec+0x8]/+0xC/+0x10/+0x14` and `[rec+0x1C]` (via `FUN_0006D920`);
(b) the target triple `[rec+0x4D/+0x51/+0x55]` (via the invoked `[rec+0x1C]`
handler); (c) the distance triple `[rec+0x65/+0x67/+0x69]` (via
`FUN_0008DCD4`). Plus the team-level `[team+0x7AE]`, `[team+0x7CB/+0x7B6/+0x7BF]`
and the `FUN_0008C418` block.

**Why it is render-invisible on the M2 tape's state:**

1. **The walk's phase domain is the placement family `0x00..0x15`.** The phases
   reaching `match_run_write_phase` are the situation/period/kickoff phases
   (`fifa96_action_phase_situation` → `0x0C`/`0x14`/`row->phase` ≤ `0x15`,
   `fifa96_action_handlers.c:1155`/`0x1175`; the period-end chooser → `0x0C`;
   kickoff `1`; the ref sequence `phase_write` set) — fresh `grep` of the 9
   `match_run_write_phase` call sites (`fifa96_match_run.c:2392,2447,2628,2650,
   2817,3062,5229,5302,5335`). The act-indexed phases `0x17..0x22` are dispatched
   **only** by `fifa96_match_act_dispatch`/`fifa96_match_dispatch_phase`
   (`fifa96_action_phase_act` = `0x16 + act`, `action_handlers.c:1117`), never by
   `match_run_write_phase`. So the walk can never double-run the three live rows
   (`0x18`/`0x19`/`0x1F`).
2. **Every placement-family row is `fn == NULL`.** Fresh source
   (`fifa96_match_handlers.c:4638..4679`): all of `0x00..0x15` are NULL. So each
   per-record `fifa96_match_dispatch_phase(mr, phase)` returns
   `-FIFA96_ERR_UNSUPPORTED` and **writes nothing** — (b) is never written by the
   walk.
3. **The metadata cells (a) and the distance triple (c) are not read by the
   render.** `match_run_scene_stage` reads only positions `+0x59`, `anim_id`
   `+0x28`, `frame` `+0x3D`, `hidden` `+0x9A` (`fifa96_match_run.c:1602..1607`).
   `[rec+0x8/+0xC/+0x10/+0x14]` and `[rec+0x65]` are absent from the render
   path. (`+0x69` overlaps the engine's `lane` field, `entities.h:134`; the
   gated walk never reaches `FUN_0008DCD4` today because the whole per-record
   body is skipped on a NULL row — see the recommended gate.)

Therefore the recommended conversion is a **structural no-op** on the M2 tape:
the walk is added, but with the per-record body gated on the phase row resolving
(`fn != NULL`), it writes nothing while no placement body is ported. Verify with
`./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt` (M2
`7b4b241a…` v16) and the M1 golden (`09b726b7…`) — expect **no re-pin**.

The gate is honest, not a fake: the native `FUN_0008DCD4` does run per record,
but its outputs (`+0x65/+0x67/+0x69`) are the *unported placement metric* (OL-13)
and are render-invisible; deferring the whole per-record placement (handler +
distance) until a body lands is the same "unported body" leg the phase table
already declares, not a dropped side effect on a live path.

**First honest mover once the walk lands:** the placement row whose body is
ported first (candidate `0x02` → `0x6DCC8`, the FU177-L1/FU190 §6.2 row) — the
walk then runs it per record exactly where the native does.

---

## 11. Numbered legs

1. **FU191-L1 — the `FUN_0006D9C4` pre-pass.** The walk calls it at `0x8D0B1`;
   fresh decompile gives `[team+0x7AE] = 0x11033A + form*0x1D` and the grouped
   `[rec+0x90]` byte write + per-entry `FUN_0006D920`, but the form-table group
   semantics (group count field, the `[rec+0x90]` meaning) are unported.
2. **FU191-L2 — the `FUN_0008DCD4` distance output.** The walk writes the word
   triple `[rec+0x65/+0x67/+0x69]` per non-skipped record; `+0x69` overlaps the
   engine's `lane` field. Whether a faithful walk must reproduce it, and whether
   that would perturb the movement chain (not the render), is unproven — defer
   with the first placement body.
3. **FU191-L3 — the record-threaded dispatch ABI.** The engine's
   `fifa96_match_handler_fn` is `int (*)(mr)`; the native handler is
   `void f(rec, &rec[0x4D], -1)`. The cursor-vs-sibling choice (§9) is a seam
   change, not pure wiring (FU175-L4).
4. **FU191-L4 — the walk's *side* on side-less writes.** `FUN_000740A0` always
   takes a side (`DX`); `match_run_write_phase` does not. Which side the derived
   walk uses when the write site did not set `side_controlled` is a modelling
   choice — pin before wiring.
5. **FU191-L5 — the switch arms.** The walk's switch (§7) runs for all
   `0x00..0x15`; the engine's `fifa96_match_run_phase_arm` ports only
   `{3..9,0xD}`. The unported arms (`0,1,2,0xA,0xB,0xC,0xE,0xF,0x10..0x15`) stay
   OL-13. Landing the walk does not change this.
6. **FU191-L6 — the phase sub-gates.** `byte 0x157A4C` (`[0x157A49]>>24`) and the
   prev-phase `0x157A4E` are written by `FUN_000740A0` but not modelled by the
   engine (`mr->state.phase` is the only derived phase cell). The walk's
   `case 0x13/0x14` reads `[0x157AC3+2]` (`DAT_00157ac3._2_2_`) and the `0x14`
   `[0x157AC2]` extra-time counter; those remain unported.

---

## 12. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `list_open_programs`, `get_current_program_info` (program `/FIFA96.EXE`,
  watcom LE, image base 0).
* `disassemble_function` — `0x8D098` (583 insns, `0x8D098..0x8D820`), `0x6D920`
  (51), `0x740A0` (25), `0x8C418` (186).
* `decompile_function` — `0x8D098` (`FUN_0008D098`), `0x6D920`, `0x6D9C4`,
  `0x6DCC8`, `0x8DCD4`, `0x8C418`.
* `get_function_by_address 0x8D098` (body `0x8D098..0x8D820`).
* `get_xrefs_to` — `0x8D098` (2), `0x740A0` (27), `0x6D920` (4), `0x6D9C4` (2).
* `read_memory` — `0x157A48` (16 B, all zero), `0x8D0F2` (128 B, the raw walk),
  `0x8D040` (48 B) + `0x8D070` (40 B) (the 22-entry `0x8D040` switch table),
  `0x110794` (140 B, the phase table).
* `disassemble_bytes` — `0x6DE34` (80 B), `0x6E1D0` (56 B), `0x6DD9C` (56 B).

Engine source read first-hand: `src/fifa96_engine/fifa96_match_act.c` (whole),
`src/fifa96_engine/fifa96_match_handlers.c` (4638..4705),
`src/fifa96_engine/fifa96_match_run.c` (1595..1664, 2350..2607, 2609..2665,
3735..3770, 5200..5250), `include/fifa96_engine/fifa96_match_entities.h`
(55..194), `tests/test_engine_m2.c` (head), `src/fifa96_loader/
fifa96_action_handlers.c` (1117, 1155..1200). Docs cross-cited: FU-96, FU-136,
FU-137, FU-140, FU-143, FU-148, FU-149, FU-150, FU-175, FU-177, FU-181, FU-182,
FU-183, FU-187, FU-190; AGENTS.md census rule. No writes beyond this slice file
and the workspace report.
