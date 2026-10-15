Promoted canonical slice — phase-26b (FU275); read-only Ghidra session, authoritative program **`/FIFA96.EXE`**; case-sensitive. No rename/comment/label/function/script/project save.

Phase/Provenance: M2 phase-26b recon-ahead wave `2026-10-14-fifa96-m2-phase26b-recon-ahead`, Task 3 (bounded, read-only derivation), promoted to canonical by the phase-26b close-out. No production code; the derivation and its ruling are unchanged by promotion.

# W26b-3 / FU275 — the taker-arm residuals, port shapes (FU266 L266-1..14)

Provenance: M2 phase-26b wave W26b-3, 2026-10-14, **read-only** recon. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; case-sensitive program argument; confirmed
this session via `list_open_programs`: `watcom:LE:32:default`, image base `0`,
2706 functions, `.image` overlay present). Every address/byte/xref/instruction
below is a fresh `disassemble_function` / `disassemble_bytes` / `decompile_function`
/ `get_xrefs_to` / `read_memory` of **this** session unless marked *cross-cited*
(then the frozen FU doc is named). LE-image globals sit at `+0x100000` from the
file offset; every address quoted is a `/FIFA96.EXE` runtime (program) address.
The alias traps are applied (`dword[x]>>16` = the word at `x+2`, `>>24` = the byte
at `x+3`; e.g. `[0x15781E]>>24` = byte `[0x157821]`, `[0x1587AE]>>16` = word
`[0x1587B0]`). **No writes**: no rename, comment, label, function, script or
project save; the ISO/HDD untouched; no git commands. Engine source read
first-hand.

Scope (W26b-3 brief): derive the **port shapes** of FU266's open legs
`L266-1..14` — the per-phase triple `FUN_00083164`, the pick `FUN_0008DB6C`, the
timer+draw machine `FUN_00085498`/`FUN_00083428`, the presentation sinks
`0x974DC(0x1E)`/`0x8F188`, and the `[0x157821]`/`[0x157A6A]` state cells. Builds
on FU266 (`docs/ghidra/FU266_taker_arm_residuals.md`, the L266-1..14 register) and
FU-149 §7 / L13 (frozen). Prior sections are not rewritten; divergences land as
quoted **errata**.

**Live-chain rule (binding).** A taker row `0x10..0x13` is **live** only when a
**ported producer raiser** installs its code on a real engine path. Staging the
gate in a fixture, or a stand-in cell, is fixture-only / a fake. The single live
taker remains **row 0x11** (FU266 §4.3, the post-goal restart chain); the port
shapes below therefore become live precisely on the row-0x11 ladder.

---

## Goal

Give each FU266 open leg a **concrete port shape**: what a faithful engine port of
that native producer/consumer is (functions, struct fields, predicates, tables).
Read-only derivation; no code landed. New residual gaps minted as **FU275-L\\***
with a native owner.

---

## Step 1 — the per-phase triple (`FUN_00083164`) + the pick (`FUN_0008DB6C`)

### 1.1 `FUN_00083164` `0x83164..0x832A6` (fresh `disassemble_function`, 100 insns)

Selector `AL = byte [0x157A4D]` (the live phase); `EDX = param_1` (the record).

```
0x8316e MOV AL,[0x157A4D]
0x83173 CMP AL,4 ; 0x83175 JC 0x8317E       ; AL < 4   -> 0x8317E
0x83177 JBE 0x83187                          ; AL == 4  -> 0x83187 (phase 4)
0x83179 JMP 0x83211                          ; AL > 4   -> the record-relative arm
0x8317e CMP AL,3 ; 0x83180 JZ 0x831D5        ; AL == 3  -> 0x831D5 (phase 3)
0x83182 JMP 0x83211                          ; AL in {0,1,2} -> record-relative
```

Three target-triple shapes, written into `{x=[0x157A77], y=[0x157A7B],
z=[0x157A7F]}`:

- **phase 4** (`0x83187`): `[0x157A77]=0`, `[0x157A7B]=0`, `[0x157A7F]=0x990`;
  `EDI=[0x15774C]` (the camera focus x); `0x831a8` if `EDI<0` →
  `[0x157A77]=0xFFFFF9D0` (**-0x630**); `0x831b4 MOV EAX,[EDX]` (team ptr) →
  `MOV AL,[EAX+0x826]` (team side byte); `0x831c1 CMP EAX,1` → `NEG [0x157A7F]`.
  Result: `x = focus_x<0 ? -0x630 : 0`, `y = 0`, `z = side==1 ? -0x990 : +0x990`.
- **phase 3** (`0x831D5`): `MOVSD x3` copies the goal snapshot `0x15777C`
  (x/y/z) into `0x157A77`; then `0x831e2 CMP dword [0x15777C],0` → `>=0` sets
  `[0x157A77]=0x630`, else `0xFFFFF9D0` (-0x630). Result:
  `x = snap_x<0 ? -0x630 : +0x630`, `y = snap_y`, `z = snap_z`.
- **phases 0/1/2 and >4** (`0x83211`): `ESI=[EDX]` (team ptr) → `ESI=[ESI+0x7A6]`
  (the opponent block) → `LEA ESI,[ESI+0x59]` (its triple) copied to the stack;
  `dx = stack.x - [EDX+0x59]` clamped to `[-0x140,0x140]` (`0xFFFFFEC0`..`0x140`);
  `dz = stack.z - [EDX+0x61]` clamped likewise; then `[0x157A77]=[EDX+0x59]+dx`,
  `[0x157A7B]=0`, `[0x157A7F]=[EDX+0x61]+dz`. Result: the **record position
  nudged toward the opponent block triple, clamped ±0x140, y=0**.

Port shape (L266-2): a `fifa96_sp_target_triple(mr, rec, out3)` keyed on
`mr->state.phase` with the three branches above; the field pairing is
`x=[0x157A77]`, `y=[0x157A7B]`, `z=[0x157A7F]` and the two known sources are
`goal_snap_{x,y,z}` (phase 3) and `render.camera.{pos_x}` (=`0x15774C`, phase 4);
the opponent-block triple is `[team+0x7A6]+0x59` (fresh; the port has no such
field — the double-clamp field is FU275-L1/L9).

### 1.2 `FUN_0008DB6C` `0x8DB6C..0x8DC49` (fresh `disassemble_function`, 78 insns)

The **receiver pick**. Params: `EAX = param_1` (a point triple; read as `[EAX]`=x,
`[EAX+8]`=z), `EDX = param_2` (the team's 11-record block, stride `0xB2`),
`BX = param_3` (the skip index; read via `MOV EAX,[ESP+0x5A]; SAR EAX,0x10` — the
high word, so `(int16_t)BX`), `ECX = param_4` (the fallback flag, low word at
`[ESP+0x60]`).

```
0x8db89 CMP dword [EBX+0x20],0 ; JNZ skip   ; only EMPTY-slot records are candidates
0x8db8f MOV EAX,[ESP+0x5a] ; SAR EAX,0x10   ; skip index = (int16_t)BX
0x8db96 CMP ECX,EAX ; JZ skip               ; skip == this index
0x8db9a CMP EBX,[EDI+0x7bf] ; JZ skip       ; == the team's chosen pointer
0x8dba2 TEST ECX,ECX ; JNZ ok
0x8dba6 CMP byte [EDI+0x829],0 ; JZ skip    ; record 0 needs team+0x829 (gate)
0x8dbaf CMP byte [EBX+0x9a],0 ; JNZ skip    ; +0x9A exclusion
0x8dbb8 CMP byte [EBX+0x98],0 ; JNZ skip    ; +0x98 exclusion
0x8dbdf CALL 0x8DC68                        ; metric (dx=param.x-rec.x, dz=param.z-rec.z)
0x8dbe8 NEG EAX ; MOV [ESP+ESI+0x28],EAX   ; store -distance in a parallel array
0x8dbfd JZ 0x8DC1B (count==0 -> fallback)
0x8dc08 CALL 0xA1860(count, index[]@ESP, dist[]@ESP+0x2C)   ; Shell-sort
0x8dc10 IMUL EAX,[ESP],0xB2 ; ADD EAX,EDI   ; return EDI + index[0]*0xB2
0x8dc1b fallback (param_4.lo!=0): first record with [+0x20]==0 && [+0x9A]==0, else 0
```

**`FUN_000A1860` `0xA1860..0xA18F9`** (fresh `disassemble_function`, 59 insns) is
a **Shell sort** of the parallel `(index, distance)` arrays (`gap = n/2`,
halved; swap when `dist[i] < dist[i+gap]`, `0xa18b2 CMP ECX,[EBX]; JGE`). With
the stored value = `-metric`, the descending sort puts the **nearest** candidate
at `index[0]` — so the pick is a **nearest** selection (the "arg-min" of FU266
§1.1 is precisely this sort's head). Metric owner `FUN_0008DC68`
(`0x8DC68..0x8DCD2`, fresh 54 insns, the octile `max(|dx|,|dz|)+3/8·min` form).

Caller (fresh `0x832A8` stream, §1.3): `EAX=0x15879C` (the staged triple),
`EDX=[EBP]` (team block), `EBX=[EBP+0x8A]>>24 = byte[rec+0x8D]` (skip),
`ECX=0` (fallback off).

Port shape (L266-3 / L266-6): a `fifa96_sp_pick_receiver(rec, query_triple, team,
skip_index, want_fallback, out_dist)` — the nearest record of the 11 (stride
`0xB2`) under the six filters above, with the Shell-sort `(index,distance)`
tie-order and the fallback linear scan; the `0x8DC68` metric is already the
engine's `fifa96_arm_dist_stage`/`fifa96_entity_find_nearest`. **`FUN_0008DE8C`
`0x8DE8C..0x8DEFF`** (fresh 42 insns) is the *second* pick — a **strict-minimum**
scan (`0x8dedc CMP AX,SI; JNC`) with `[+0x9A]`/`[+0x98]`/skip filters, writing
the winning distance to `*param_3` and returning the record (or NULL); the engine
`match_sp_nearest` already covers its shape, but neither pick models the
**empty-slot `[+0x20]==0` predicate**, the `[team+0x829]` record-0 gate, or the
fallback scan (FU275-L2/L9).

### 1.3 `FUN_000832A8` `0x832A8..0x83424` (fresh `disassemble_function`, 108 insns) — L266-1

The staging block. Fresh stream (abridged to the port-visible stores):

```
0x832bc MOV word [0x158784],DX        ; DX=0 (XOR EDX,EDX 0x832b5) -> clear the gate
0x832c3 CALL 0x83164                   ; EAX=record -> fills the 0x157A77 triple
0x832d4 MOVSD ES:EDI,ESI x3            ; [0x158790] = 3 dwords from 0x10F328
0x832e4 MOV [0x1587B7],AL              ; AL = param_2 low byte ([ESP])
0x832eb MOVSD ES:EDI,ESI x3            ; [0x15879C] = the [0x157A77] triple
0x832f3 MOV EBX,[EBP+0x8A] ; SAR EBX,0x18   ; skip = byte[rec+0x8D]
0x832ff MOV [0x1587A8],EBP             ; the record
0x83305 CALL 0x8DB6C ; 0x8330A MOV [0x1587AC],EAX   ; the picked receiver
0x8330f MOV AH,3 ; 0x83311 MOV dword [EBP+0x89],0   ; clear the record timer
0x8331d MOV [0x15877E],AH ; 0x83323 MOV [0x1587B5],DH ; =3 / 0
0x8332c MOV [0x15877F],AH              ; =3
0x83329 MOV EDX,[EBP+0x20]             ; the slot pointer
0x83332 TEST EDX,EDX ; JZ 0x83388
0x83336 MOV EAX,EDX ; CALL 0x78B00     ; slot arm: merge request
0x8334d [0x157AAD]=1 ; 0x83358 [0x157AB0]=1 ; 0x83366 [0x1587B2]=1
0x8336c MOV word [0x1587B0],CX (CX=1) ; 0x83381 CALL 0x361B0
0x83388 else: word [0x1587B0]=1 ; byte [0x1587B2]=0 ; CALL 0x36200
0x833a0 CALL 0x361A4
0x833a5 CMP [EBP+0x20],0 / 0x833ab CMP [ESP],0 -> 0x833b6/0x833ba [0x1587B6]
0x833c4 (slot empty) [0x157A4A]>>24 == 4 -> [0x1587B6]=1
0x833e1 [0x1587AE]>>16 == 1 -> 3-way triple copy into 0x157A77
```

Fresh `read_memory 0x10F328` = `00 00 00 00 00 00 00 00 00 00 00 00` — the
staging "vector" `[0x158790]` is **three zero dwords**. The tail three-way copy
(`0x833e1..0x8341b`): if `word[0x1587B0]==1` then `[0x157A77] := [0x1587B6] ?
0x15879C : [0x1587A8]+0x59`, else `[0x157A77] := [0x1587AC]+0x59`. So
`[0x1587B6]` selects the staged triple vs the record position, and `[0x1587AE]`
(mode word) selects "staged receiver" vs "the record itself".

Port shape (L266-1): expand `match_sp_setup_832a8` from the two-cell stand-in
(`sp_flag_158784=0`, `timer89=0`) to a `struct fifa96_sp_stage` carrying
`rec_1587A8` (record ptr), `recv_1587AC` (picked receiver ptr),
`tri_15879C[3]`, `rec_158790[3]` (zero), `tri_157A77[3]`, and the bytes
`b0_1587B0 / b2_1587B2 / b5_1587B5 / b6_1587B6 / b7_1587B7`; the slot branch
(`[+0x20]!=0` → `b2=1`, `b0=1`, `0x361B0`; else `b2=0`, `b0=1`, `0x36200`;
always `0x361A4`); the `b6` rule (`slot ? param_2!=0 : phase==4`); the
`0x78B00` merge request; and the three-way `tri_157A77` select. Owner:
`FUN_000832A8`. (The `0x361A4/0x361B0/0x36200` calls are FU266-L266-5 sinks.)

### 1.4 The event sub-tables (fresh `read_memory`)

| table | 7/8/10 dwords (fresh) |
|---|---|
| row 0x10 stages `0x855B8` | `{8566D, 857D6, 8580D, 858E4, 85CC4, 85D5A, 85D8D}` |
| row 0x10 events `0x855D4` | `{85A3F, 85AA5, 85A30, 85A3F, 85AA5, 85A30, 85AF0}` |
| row 0x11 stages `0x85DA0` | `{85E3C, 85E7D, 85FA2, 85FBD, 85FD8, 86013, 860CB, 8610E, 8645B, 86496}` |
| row 0x11 events `0x85DC8` | `{861F9, 86235, 862BD, 86272, 862AA, 862BD, 862F4}` |
| row 0x12 stages `0x83D2C` | `{83E26, 83E72, 84077, 840B2, 8416A, 841AD, 8450C, 84543}` |
| row 0x12 events `0x83D4C` | `{842AB, 8432F, 842E7, 84368, 843A0, 842E7, 843AE}` |
| row 0x13 stages `0x84AE4` | `{84BAD, 84C48, 84CBA, 84DF4, 84E26, 84E61, 84EB1}` |

Byte-for-byte with FU-149 §7.1 (cross-cited). The cascade is real: fresh
`0x85FBD` (stage 3) resets `timer89`, `INC byte[rec+0x92]` and falls into
`0x85FD8` (stage 4); fresh `0x85FD0..0x86013` shows stage 4→5 likewise.

---

## Step 2 — the timer+draw machine

### 2.1 `FUN_00085498` `0x85498..0x855B4` (fresh `disassemble_function`, 80 insns)

`ECX = param_1` (the record), `EDX = param_2` (the timer threshold), `EBX =
param_3` (the span).

```
0x8549b CMP dword [EAX+0x20],0 ; JNZ 0x854BE     ; slot path
0x854a1 MOV EAX,[0x1587AC] ; CMP [EAX+0x20],0 ; JZ 0x85538   ; no-slot arm
0x854b0 CMP word [0x1587B0],0 ; JNZ 0x85538
0x854be slot path: EAX=[0x1587AE]>>16==1 ? [ECX+0x20] : [[0x1587AC]+0x20] (the input)
0x854db CALL 0x83428                              ; the input-driven machine
0x854e0 TEST AX,AX ; JZ 0x85506
0x854e5 (AX!=0) clear [0x157AA7], CALL 0x744D4, clear [0x157A6F], CALL 0x585A0, return 1
0x85506 (AX==0) if mode==1: CALL 0x361B0(byte3[[ECX+0x20]+0x19]); CALL 0x79C50; return 0
0x85538 no-slot arm:
0x85538 CMP EDX,[ECX+0x89] ; JGE 0x85534          ; param_2 >= timer89 -> return 0
0x85540 XOR EDX,EDX ; CALL 0x92AC8 ; 0x85547 DIV EBX   ; draw = RNG % span
0x85549 MOV EAX,[ECX] ; 0x8554b MOV byte [EAX+0x7E7],DL   ; [team+0x7E7] = draw
0x85551 MOV EAX,[ECX] ; 0x85553 MOV EDX,[EAX+0x7E4] ; 0x85559 SAR EDX,0x18
                ; EDX = byte3[team+0x7E4] = byte[team+0x7E7] = the draw just written
0x8555c CMP EDX,0x3 ; 0x8555f JL 0x85572
0x85561 >=3 -> 0x85563 [EAX+0x7E7]=0 ; 0x8556a [0x1587B4]=7
0x85572 <3  -> CALL 0x6DAB4 ; clear 0x158780/0x15877F/0x15877E ; [0x1587B4]=3
0x85593 clear [0x157AA7], CALL 0x744D4, clear [0x157A6F], CALL 0x585A0, return 1
```

Fresh `0x86092..0x8609F` (row 0x11 stage 5) shows the caller: `EBX=4; EDX=0x78;
EAX=[rec]; CALL 0x85498` — **timer 0x78, span 4**; row 0x10 stage 2
(`0x858B0` region) the same; row 0x12 span 3 (cross-cited FU-149). The no-slot
latch selector reads **back the byte it just wrote**: `0x85553 MOV
EDX,[EAX+0x7E4] ; 0x85559 SAR EDX,0x18` takes byte3 of the `team+0x7E4` dword,
which is `byte[team+0x7E7]` — the `DL` stored at `0x8554b` — so the native test
is exactly `draw >= 3`, and native `draw >= 3 ? 7 : 3` **is** the port's
`(draw % span) < 3 ? 3 : 7` (no divergence; erratum 5 below).

Port shape (L266-4): `match_sp_probe_85498` keeps its `(draw % span) < 3 ? 3 : 7`
latch (correct), but must **materialize the draw** into the per-team `+0x7E7`
throw-anim byte (native `0x8554b MOV byte [EAX+0x7E7],DL`), which it never writes
(its own comment at `fifa96_match_handlers.c:1534` claims it draws into `+0x7E7`).
The slot path is a new `fifa96_sp_throw_machine(mr, rec, input_slot, span)`
returning `0..6` (§2.2). Owner: `FUN_00085498`.

### 2.2 `FUN_00083428` `0x83428..0x83B1D` (fresh `decompile_function`) — L266-4

`param_1 = rec`, `param_2 = slot` (the input/control state). Head:
`FUN_0006DAB4(*param_1, byte3[0x15877B])` (animation install); the guard
`byte[0x1587B5]!=0 || byte3[0x15877B]<0 || >2` → the silent reset arm
(`[0x157A6F]=0`, copy `0x15774C → 0x158767`, zero `0x15878C/88/86/80`) and
`goto LAB_00083664`. Else the machine: `[0x158780]` (the 1..2 sub-index)
steps, `FUN_0008DCD4` distance + `FUN_000702F8` timer over the per-team block
`*(param_1) + 0x7E8 + [0x158780]*0xC`; the `[0x1587B3]`/`[0x1587B4]` delivery
state from the input words `[param_2+4]` (0xB0/0x40 flags), `[param_2+6]`
(0x10/0x20/0x40 codes), `[param_2+0x10]` (0xF0/0xF), `[param_2+0x12]`
(8/4/2/1 bits); the receiver re-pick `FUN_0008DE8C(&0x15879C, *param_1,
[rec+0x8D], 0)`; and **returns 0/1/2/3/4/5/6** (the `[0x1587B4]` latches
1/6/3/4/5/2, else 0).

Port shape (L266-4): a `fifa96_sp_throw_machine` porting the per-branch state
transitions verbatim (input-code keyed `[param_2+6]` ∈ {0x10, 0x20, 0x40,…}),
the `0x158780`/`0x15878C` sub-index timers, the `FUN_0008DCD4`/`FUN_000702F8`
decomposition, and the return-code map; plus the per-team `0x1587E8 + 0xC*idx`
block (the `[team+0x7E4..0x7E7]` tail). Owner: `FUN_00083428` (FU-204 §282/284
notes it is also the row-09 presentation consumer — not taker-exclusive).

---

## Step 3 — the sinks + the state cells

### 3.1 `FUN_000974DC` `0x974DC..0x974EC` (fresh 7 insns) — L266-5

`PUSH EDX; MOV EDX,EAX; CALL 0x64EC0; MOV EAX,EDX; CALL 0x65CC0; POP EDX; RET`
(the audio pair). Fresh `get_xrefs_to 0x974DC` = **42**. The taker/scanner sites
(fresh bytes): `0x88B64/0x88BDE` (scanner), `0x857DF` (row 0x10 stage 1),
`0x85FE1` (row 0x11 stage 4), `0x84080` (row 0x12 stage 1), `0x84C98` (row 0x13
stage 1). The argument is the literal `0x1E` (`b8 1e 00 00 00`), no register
ambiguity.

**Erratum (quoted).** FU266 §2.1 annotates the row-0x10 (`0x857DF`) and row-0x11
(`0x85FE1`) sites as "gated `[0x14C32A]==0`". **That polarity is inverted.** Fresh
`0x857d6 CMP byte [0x14c32a],0 ; JZ 0x857E9` and `0x85FD8 CMP byte [0x14c32a],0 ;
JZ 0x85FEB` skip the sound when `[0x14c32a]==0`; the `0x974DC(0x1E)` call is
reached when **`byte[0x14c32a] != 0`**. The `0x8F188` sink is the *opposite*:
fresh `FUN_0008F188` gate `DAT_0014c32a==0 && DAT_0014c312!=0` (§3.2). FU266's
"==0" wording is superseded (kept verbatim there).

Port shape (L266-5): a presentation sink shim `fifa96_sp_sink(mr, sound_id)`
called at the exact sites with the exact gate (`[0x14c32a]!=0` for `0x974DC`;
`[0x14c32a]==0 && [0x14c312]!=0` for `0x8F188`), backed by run-state fields
`session_gate_14c32a` / `event_gate_14c312` so the drop is honest and a backend
can be dropped in. Owner: `FUN_000974DC` / `FUN_0008F188`.

### 3.2 `FUN_0008F188` (fresh `decompile_function`) — L266-5

```
if (byte[0x14C32A]==0 && dword[0x14C312]!=0) {
   classify param_3 in {8 -> [0x15B370], 0x20/0x21 -> [0x15B374], 0x40 -> [0x15B36C]};
   ring push at [0x15AAE0 + ((idx+9)%10)*8] = param_1;
   [0x15AAE4..] = FUN_000CB2A4(); [0x15AAFC..] = param_2 (the record);
   if (param_2) copy param_2+0x59 triple -> [0x15AAF0..];
   [0x15AAEC..] = param_3; idx advancement; INC [0x15AAD8];
}
```

Fresh `get_xrefs_to 0x8F188` = 130 (FU266). The taker-phase sites (fresh bytes):
`0x86086 EAX=0xA4` (row 0x11 stage 5), `0x86391 EAX=0x19` / `0x863BE EAX=0x1C`
(row 0x11 stage 7, behind the `0x86388 CALL 0x92AC8; TEST AL,1` draw), plus the
FU266 set (`0x8D335 0x17`, `0x8D40D 0x1A`, `0x8412C 0xA4`, `0x858A6 0xA5`,
`0x85909 0x18`).

### 3.3 `[0x157821]` — the row-0x10 re-queue gate (L266-8 / L266-13)

Fresh `get_xrefs_to 0x157821` = **11** (`FUN_000736AC 0x739BB` R;
`FUN_000709D0 0x70A4C` R + `0x70A65` RW; `FUN_00070544 0x7063D` R;
**`FUN_000700F4 0x70153` W**; `FUN_00077EAC 0x7812E/0x78144` R; row 0x10
`0x85CE4` R; `0x7489C`/`0x74A8C`/`0x7E295` R).

- Producer clear (fresh `0x700F4`): `0x70144 XOR AH,AH ; 0x70153 MOV byte
  [0x157821],AH` (=0) inside the camera reset.
- Producer increment (fresh `0x709D0`): `0x70A3F MOV EAX,[0x15781E] ; SAR
  EAX,0x18` (=byte `[0x157821]`); `0x70A4A CMP EAX,0x64 ; JGE 0x70A6B` (cap
  0x64); `0x70A4C CMP byte [0x157821],0 ; JNZ 0x70A5F`; `0x70A55 CALL
  0xCB2A4 ; MOV [0x1575C3],EAX` (seed once on 0→1); `0x70A5F INC byte
  [0x1575CB]`; `0x70A65 INC byte [0x157821]`.
- Row-0x10 gate (fresh `0x85CE4`): `0x85CDD CMP EAX,0x720 ; JL 0x85D19`;
  `0x85CE4 CMP byte [0x157821],0 ; 0x85CEB JLE 0x85D94` (signed byte ≤0 →
  exit); `0x85CF1`/`0x85D0A` `CALL 0x8A938` situation 2, BX=1.

Port shape (L266-8 / L266-13): **unify** the engine's `mr->sp_157821` with
`fifa96_camera.pan_counter` — the native producer is exactly the camera
pan-step `FUN_000709D0` (cap 0x64) cleared by the camera reset `FUN_000700F4`,
and the engine already ports both as `fifa96_camera.pan_counter`
(`fifa96_camera.c:863/893`, `fifa96_camera.h:28/38`). The row-0x10 gate should
read `cam->pan_counter` (not a shadow). Owner: `FUN_000700F4`/`FUN_000709D0`.

### 3.4 `[0x157A6A]` — the offside suppression timer (L266-11)

Fresh `get_xrefs_to 0x157A6A` = **6** (`FUN_00073E28 0x73E85` W;
`FUN_00079D5C 0x79D79` R; row 0x10 `0x85D19` W; row 0x11 `0x863DF` W; `0x7438D`
R; `0x743A5` RW). Fresh `FUN_00073E28`: `0x73E36 XOR EBX,EBX` … `0x73E85 MOV
word [0x157A6A],BX` (=0) — the match-reset helper. Fresh row sites:
`0x85D19 MOV word [0x157A6A],0x12C` (`66 c7 05 6a 7a 15 00 2c 01`) and
`0x863D8 MOV ECX,0x12C ; 0x863DF MOV word [0x157A6A],CX`. The per-frame
countdown is `0x7438D`/`0x743A5` (`[0x157A64]` delta).

Port shape (L266-11): the cell is already `referee.offside_suppress`; the reset
belongs at the `fifa96_referee` match-reset entry (owner `FUN_00073E28`), and the
per-frame countdown (`-= byte-delta`) belongs beside `referee.offside_suppress`
(owner `0x743A5`). Owner: `FUN_00073E28`.

### 3.5 The penalty remainder — row 0x13 `0x84D5B` (L266-10)

Fresh `disassemble_bytes 0x84D30..0x84DC5`:

```
0x84d46 MOV EAX,ECX ; AND EAX,0xFFFF ; CMP EAX,0x20 ; JNZ 0x84DBA   ; slot word == 0x20
0x84d52 CMP byte [0x14C32A],0 ; JNZ 0x84DBA                          ; session gate
0x84d5b MOVSX AX,byte [EBP+0x8D] ; INC EAX                           ; start = [rec+0x8D]+1
0x84d67 CMP EDX,0xB ; JNZ ; 0x84D6C MOV EAX,1                        ; wrap 0xB -> 1
0x84d74 IMUL EDX,EDX,0xB2                                            ; stride
0x84d7d CMP byte [EDX+EBX+0x9A],0 ; JNZ 0x84D63                      ; walk while +0x9A set
0x84d87 CALL 0x7DAB4                                                 ; reset self
0x84d8e MOV EAX,[EBP] ; ADD EDX,EAX ; MOV [EAX+0x7B2],EDX            ; team->target
0x84da0 MOV EDX,0x13 ; CALL 0x7D9A4                                  ; install 0x13
0x84dba (slot != 0x20 / gate open) CALL 0x78A84
```

Port shape (L266-10): `match_sp_penalty_handoff(mr, rec)` — start
`[rec+0x8D]+1`, wrap `0xB→1`, walk `IMUL 0xB2` while `[+0x9A]!=0`, reset self
(`0x7DAB4`), latch `[team+0x7B2]`, install 0x13 — with the walk **capped at 10
iterations** (precedent `fifa96_match_run.c:2753`, comment `:2759` "native loops
forever; engine sentinels") and the caller's fallback when none is clear. Owner:
row 0x13 `0x84B00`.

### 3.6 `FUN_00085498` slot machine reachability (fresh row-0x11 bytes)

Fresh `0x85FD0..0x860A1` and `0x85FA8..0x85FDB`: stage 3 `0x85FBD` resets
`timer89`, `INC stage92`, and falls to stage 4 `0x85FD8` (`[0x14C32A]!=0` →
`0x974DC(0x1E)`; `CALL 0x832A8`); then stage 5 `0x86013` (the `0x15774C :=
0x157764` focus copy + `0x79B6C` anchor; `[0x158784]!=0` → `CALL 0x832A8` and
exit; else the `0x4B0` timer + `0x8F188 0xA4` + `0x85498` span 4). This is the
live row-0x11 ladder FU266 §1.3 ruled on.

---

## Step 4 — the port shapes summary + the live-trigger ruling (hand-off)

### 4.1 Per-leg port shapes

| leg | native shape (fresh) | concrete port shape | owner |
|---|---|---|---|
| L266-1 | `FUN_000832A8` staging block (gate 0x158784, const 0x158790=0, triple 0x15879C, recv 0x1587AC, bytes B0/B2/B5/B6/B7, 0x361xx) | expand `match_sp_setup_832a8` → `struct fifa96_sp_stage` + slot branch + 3-way `tri_157A77` select | `0x832A8` |
| L266-2 | `FUN_00083164` | `fifa96_sp_target_triple(mr, rec, out3)` (3 branches: 0/1/2/≥5 rec-relative clamp ±0x140; 3 snap ±0x630; 4 focus ±0x630/0/±0x990) | `0x83164` |
| L266-3 | `FUN_0008DB6C` + `FUN_000A1860` + `FUN_0008DC68` | `fifa96_sp_pick_receiver(rec, query, team, skip, want_fallback, out_dist)` — nearest, six filters, Shell-sort tie + fallback scan | `0x8DB6C`/`0xA1860`/`0x8DC68` |
| L266-4 | `FUN_00085498` + `FUN_00083428` | `match_sp_probe_85498` latches `sp_delivery` (`draw>=3?7:3`, correct) but must materialize the draw into `[team+0x7E7]`; new `fifa96_sp_throw_machine` (returns 0..6) + the `0x1587E8` block | `0x85498`/`0x83428` |
| L266-5 | `0x974DC(0x1E)`, `0x8F188` ids | `fifa96_sp_sink(mr, id)` with the exact gates (`[0x14C32A]!=0` vs `==0 && [0x14C312]!=0`) + run-state gate fields | `0x974DC`/`0x8F188` |
| L266-6 | `FUN_0008DE8C` (=strict-min; origins 0x157770/0x1587AC/0x15879C) | `fifa96_sp_pick_origin(rec, target_triple, team, skip, out_dist)` — strict-min scan, writes the distance | `0x8DE8C` |
| L266-7 | 7 tables (§1.4) + `[team+0x7E8+0xC*idx]` + `0x86388 &1` + row-0x12 `0x6DBCC==3` split | expose `sp_delivery`-indexed event dispatch + the `0x1587E8` block; row-0x12 split keyed on `FUN_0006DBCC` | rows `0x855F0..` |
| L266-8 | `[0x157821]` producer (`FUN_000700F4` clear + `FUN_000709D0` inc cap 0x64) | read `cam->pan_counter` in row 0x10; drop the `sp_157821` shadow | `0x700F4`/`0x709D0` |
| L266-9 | `[0x15781D]` producers (`FUN_0007131C 0x713DB/0x71908`; clears `0x740F6`/`0x84F90`) | wire the scanner gate `phase∈{2,0x10} && goal_armed` to the real producer | `0x7131C` |
| L266-10 | penalty `0x84D5B` walk | `match_sp_penalty_handoff` with a 10-iteration cap | row 0x13 `0x84B00` |
| L266-11 | `[0x157A6A]` reset `FUN_00073E28` + countdown `0x743A5` | reset at the `fifa96_referee` entry; countdown beside `offside_suppress` | `0x73E28` |
| L266-12 | — (census) | **no port shape** — a census leg (Task 8) | `0x7131C`/`0x8AF38`/`0x88940` |
| L266-13 | `sp_157821`↔`pan_counter` unification | one cell: alias or delete the shadow (same as L266-8's shape) | `0x700F4`/`0x709D0` |
| L266-14 | — (tape-stage pin) | **no port shape** — a test-owner pin (which `0x85DA0` stage runs) | row 0x11 `0x85DE4` |

**Count: 12 of 14 legs now have a concrete port shape** (L266-12 and L266-14 are
measurement legs — a census and a tape-stage pin — not port-shape legs).

### 4.2 Live-trigger ruling (hand-off)

Unchanged from FU266 §4.1/§4.3: **row 0x11 is the single live taker** (the
post-goal restart chain → phase 4 → the `0x8D35B`/`0x8D412` arm installs
`0x11`). The Step-1/Step-2 machinery is **live-reachable** on the row-0x11 ladder
(stages 4/5, fresh §3.6), so the port shapes above become live once wired. Rows
0x10/0x12/0x13 are **LIVE-LEG** (cross-slice label ruling, W26b wave, matching
FU274 + the FU271 foul chain): their bodies/arms are ported but no live producer
raiser fires them on the tape (the throw-in scanner and the FU-150 foul
adjudicator are unported). Layer split (FU266 §4.1): **Layer 1** = the producer raiser
(scanner/isolation gate → phase arm install); **Layer 2** = the row dispatch.
A fixture that stages Layer 1 is a fake.

---

## Numbered open legs (FU275, native owners)

- **FU275-L1 — `FUN_00083164` phase-4 triple semantics.** The x term is
  `focus_x<0 ? -0x630 : 0` (asymmetric) and the z term `±0x990` keyed on
  `byte[team+0x826]`; the "camera focus guard" intent is unproven. Owner:
  `FUN_00083164 0x83187..0x831d0`.
- **FU275-L2 — `FUN_0008DB6C` fallback arm (`param_4.lo!=0`).** `FUN_000832A8`
  passes `ECX=0`, so the fallback is unused by the taker; its callers are
  unverified this slice (a fresh `get_xrefs_to 0x8DB6C` census is the probe).
  Owner: `FUN_0008DB6C 0x8DC1B`.
- **FU275-L3 — the `0x1587E8 + 0xC*idx` per-team block layout.**
  `[team+0x7E4..0x7E7]` and the sub-index (1/2) fields are used by
  `FUN_00083428`/`FUN_00085498` (`0x85559` selector) but not typed. Owner:
  `FUN_00083428`.
- **FU275-L4 — `FUN_00083428` return-value map.** The decompiled branches return
  0/1/2/3/4/5/6 (FU266 §1.1 said "0..7"); the exact input-code→return mapping is
  unenumerated. Owner: `FUN_00083428`.
- **FU275-L5 — the port never materializes the draw into `[team+0x7E7]`.**
  Native `0x8554b MOV byte [EAX+0x7E7],DL` stores the drawn remainder into the
  per-team throw-anim byte; fresh `search_instructions "0x7e7"` finds **23**
  native read/write sites, including the row-0x10 gate `0x85978 CMP byte
  [EBX+0x7E7],0` and the row-0x11 gate `0x86113 CMP byte [EDX+0x7E7],0`. The
  engine leaves `[team+0x7E7]` unmodeled (`fifa96_match_handlers.c:2597`
  `team_7e7 = 0`, OL-72). `match_sp_probe_85498` (`:1539`) latches `sp_delivery`
  correctly but does not write the byte its comment claims. The **latch selector
  itself is not a divergence** (`0x85553/0x85559` re-reads the just-written byte;
  erratum 5). Owner: `FUN_00085498 0x8554b`.
- **FU275-L6 — `[0x14C32A]` gate semantics.** Fresh: `0x974DC` fires iff
  `[0x14C32A]!=0`; `0x8F188` pushes iff `[0x14C32A]==0 && [0x14C312]!=0`; the
  penalty hand-off runs iff `[0x14C32A]==0`. What the byte models (session vs
  presentation) and its writers are unported. Owner: the `[0x14C32A]` writers.
- **FU275-L7 — `FUN_000832A8` tail 3-way select.** `word[0x1587B0]` (mode) and
  `[0x1587B6]` choose `0x15879C` / `[rec+0x59]` / `[0x1587AC]+0x59`; the mode
  word's writer beyond `FUN_000832A8` is unverified. Owner: `0x832A8
  0x833E1..0x8341B`.
- **FU275-L8 — the `0x158790` staging block.** Fresh `0x10F328` = three zeros,
  so the "vector" is zero-initialized; confirm no other writer before the rows
  read it. Owner: `FUN_000832A8 0x832D4`.
- **FU275-L9 — the pick's `[team+0x7BF]`/`[team+0x829]` cells.** The
  self-pointer and record-0 gate are read by both picks but produced unported.
  Owner: `FUN_0008DB6C`/`FUN_0008DE8C` consumers.
- **FU275-L10 — the penalty walk wrap order.** `0x84D5B` wraps `0xB→1`
  (`start..10,1..start-1`); confirm it matches `match_run_free_record`'s
  `0x897F4` order before reusing that cap. Owner: row 0x13 `0x84B00`.

---

## Legs summary (owner-tagged)

| leg | subject | port shape? | native owner |
|---|---|---|---|
| L266-1 | `FUN_000832A8` staging block | yes | `0x832A8` |
| L266-2 | `FUN_00083164` per-phase triple | yes | `0x83164` |
| L266-3 | `FUN_0008DB6C` pick + metric | yes | `0x8DB6C`/`0xA1860`/`0x8DC68` |
| L266-4 | `FUN_00085498`/`FUN_00083428` machine | yes | `0x85498`/`0x83428` |
| L266-5 | presentation sinks | yes | `0x974DC`/`0x8F188` |
| L266-6 | `FUN_0008DE8C` pick origins | yes | `0x8DE8C` |
| L266-7 | event sub-tables + `0x1587E8` | yes | rows `0x855F0..` |
| L266-8 | `[0x157821]` shadow | yes (unify) | `0x700F4`/`0x709D0` |
| L266-9 | `[0x15781D]` goal_armed producer | yes | `0x7131C` |
| L266-10 | penalty `0x84D5B` hand-off | yes | row 0x13 `0x84B00` |
| L266-11 | `[0x157A6A]` reset owner | yes | `0x73E28` |
| L266-12 | live-trigger census (`0x10`/`0x12`/`0x13`) | no (census) | `0x7131C`/`0x8AF38`/`0x88940` |
| L266-13 | `sp_157821`↔`pan_counter` unification | yes | `0x700F4`/`0x709D0` |
| L266-14 | live row-0x11 tape stage | no (tape pin) | row 0x11 `0x85DE4` |

**12/14 concrete port shapes.**

---

## Errata to FU266 (quoted, not rewritten)

1. **Inverted gate.** FU266 §2.1's "row 0x10 stage 1 / row 0x11 stage 4,
   gated `[0x14C32A]==0`" is inverted: fresh `0x857D6`/`0x85FD8` skip the
   `0x974DC(0x1E)` call when `[0x14C32A]==0`; it fires when `[0x14C32A]!=0`.
   (§3.1.)
2. **`FUN_00083428` returns 0..6**, not "0..7" (fresh decompile, §2.2;
   FU266 §1.1/§1.2 wrote 0..7).
3. **`FUN_00083164` phases 0/1/2 are not merely "record-relative"**: the native
   nudges the record toward the **opponent block triple** `[team+0x7A6]+0x59`
   with a ±0x140 clamp (§1.1); FU266 §1.1's "record-relative triple, clamp
   fffffec0..0x140, byte `[+0x826]` side sign" conflated the phase-4 side byte
   with the 0/1/2 arm.
4. **The pick is a nearest selection.** Fresh `FUN_000A1860` is a Shell sort of
   the `(index, -distance)` arrays; FU266's "arg-min" is the sort's head
   (§1.2).
5. **No delivery-latch divergence (self-correction).** The round-1 draft claimed
   the native latch selector `byte3[team+0x7E4]` differed from the port's
   `(draw % span) < 3 ? 3 : 7`; fresh `0x85551..0x8555c` shows the selector
   re-reads the byte just stored at `0x8554b` (the draw), so the two are
   identical — the framing is withdrawn. The real gap is that the port never
   writes `[team+0x7E7]` (FU275-L5).

## Risks / hand-off

- **L266-8/L266-13 is a real port divergence, not a mere leg:** the engine's
  `sp_157821` shadow and the native `[0x157821]` (the camera pan counter) are the
  same cell; unifying changes row-0x10 requeue behavior — pin with a fixture
  before the tape.
- **FU275-L5 is a live-path gap** on row 0x11: the draw into `[team+0x7E7]`
  (read by the live row-0x11 gate `0x86113`) is unmodeled; the latch selector
  itself is correct.
- **FU275-L6 (gate polarity)**: FU266/engine comments carry the inverted
  `[0x14C32A]` reading — correct before wiring the sink gates.
- Do **not** wire `sp_157821` to the camera probe blindly; and the penalty walk
  (`0x84D5B`) must be capped at 10 iterations (precedent
  `fifa96_match_run.c:2753`).
- No re-pin demanded: the live `0x11` path is already wired/tape-pinned; the
  producer work for rows 0x10/0x12/0x13 (scanner throw-in, FU-150 foul
  adjudicator) is outside this slice's owner set.
