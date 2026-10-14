# W26-3 / FU-266 — the taker-arm residuals (FU-149 L13.1–L13.7)

Provenance: M2 phase-26 wave W26-3, 2026-10-14, **read-only** recon. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; case-sensitive program argument; confirmed
this session: `watcom:LE:32:default`, image base `0`, 2706 functions, `.image`
overlay present). Every address/byte/xref/instruction below is a fresh
`disassemble_bytes` / `disassemble_function` / `decompile_function` /
`get_xrefs_to` / `read_memory` / `get_function_by_address` of **this** session
unless marked *cross-cited* (then the frozen FU doc is named). LE-image globals
sit at `+0x100000` from the file offset; every address quoted is a `/FIFA96.EXE`
runtime (program) address. The alias traps are applied (`dword[x]>>16` = the word
at `x+2`, `>>24` = the byte at `x+3`; e.g. `[0x15781E]>>24` = byte `[0x157821]`).
**No writes**: no rename, comment, label, function, script or project save; the
ISO/HDD untouched; no git commands. Engine source read first-hand.

Scope (W26-3 brief): the producers that keep the set-piece arms `0x10..0x13`
**fixture-dormant** — the throw setup (`FUN_000832A8`/`FUN_00083164`/
`FUN_0008DB6C`), the timer+draw completion (`FUN_00085498`/`FUN_00083428`), the
arm vector/RNG cadence and the `0x974DC(0x1E)`/`0x8F188` sinks, the `0x8DE8C`
pick origins, the `[0x157821]`/`[0x157A6A]` state cells, and the penalty
(`0x84D5B` hand-off). Builds on FU-149 §7 / L13 (frozen), FU-150 § (referee
sinks), FU-151 § (picks), FU-152 § (pan), FU-168 § (reset), FU-204 §
(`FUN_00083428`). Prior sections are not rewritten; divergences land as errata.

**Live-chain rule (binding).** A taker row `0x10..0x13` is **live** only when a
**ported producer raiser** installs its code on a real engine path. Staging the
gate in a fixture, or a stand-in cell, is fixture-only / a fake.

---

## Step 1 — L13.1–L13.2: the throw setup + the timer+draw completion

### 1.1 Native producers (first-hand)

**`FUN_000832A8` `0x832A8..0x83424`** (`disassemble_function`, 108 insns) — the
throw-setup entry. Fresh `get_xrefs_to 0x832A8` = **6** callers: row 0x10 stages
1/2 (`0x857F0`/`0x85870`), **row 0x11 stages 4/5** (`0x85FF3`/`0x8604D`), and
row 0x12 stages 1/2 (`0x84092`/`0x840EC`). First-hand stream:

```
0x832B7 MOV EDI,0x158790
0x832BC MOV word [0x158784],DX          ; DX=0 (XOR EDX,EDX 0x832B5)  -> clears the staging gate
0x832C3 CALL 0x83164                     ; EAX = the record (param_1) -> sets 0x157A77 triple
0x832C8 XOR EAX,EAX ; CALL 0x585A0
0x832D4 MOVSD ES:EDI,ESI x3              ; [0x158790] = 3 dwords copied from ESI=0x10F328
0x832DA MOV EDI,0x15879C ; MOV ESI,0x157A77
0x832E4 MOV [0x1587B7],AL                ; AL = param_2 low byte ([ESP])
0x832EB MOVSD ES:EDI,ESI x3              ; [0x15879C] = the [0x157A77] triple
0x832F3 MOV EBX,[EBP+0x8A] ; SAR EBX,0x18 ; EBX = byte[rec+0x8D]  (the skip index)
0x832F9 MOV EDX,[EBP]                     ; EDX = team base
0x832FF MOV [0x1587A8],EBP                ; the record
0x83305 CALL 0x8DB6C ; 0x8330A MOV [0x1587AC],EAX   ; the picked receiver record
0x8330F MOV AH,3 ; 0x83311 MOV [EBP+0x89],0         ; clears the record timer
0x8331D MOV [0x15877E],AH ; 0x83323 MOV [0x1587B5],DH ; AH=3, DH=0 (XOR DH,DH 0x8331B)
0x8332C MOV [0x15877F],AH
0x83329 MOV EDX,[EBP+0x20]                ; the record's slot pointer
0x83332 TEST EDX,EDX ; JZ 0x83388
0x83336 ... CALL 0x78B00 ...              ; slot arm
0x8334D MOV byte [0x157AAD],1 ; 0x83358 MOV byte [0x157AB0],1
0x83366 MOV byte [0x1587B2],1 ; 0x8336C MOV word [0x1587B0],CX  (CX=1)
0x83381 CALL 0x361B0
... else 0x83388/0x8339B: [0x1587B0]=1, [0x1587B2]=0, CALL 0x36200
0x833A0 CALL 0x361A4
0x833A5 CMP [EBP+0x20],0 / 0x833AB CMP [ESP],0  -> 0x833B6/0x833BA sets [0x1587B6] = param_2!=0
0x833C4 [0x157A4A]>>24 == 4 -> [0x1587B6]=1
0x833E1 [0x1587AE]>>16 == 1 -> the target triple copy (0x15879C or [0x1587A8]+0x59) into 0x157A77
```

`0x832BC` writes the **word** gate `[0x158784]`; the engine's `sp_flag_158784`
models exactly that (`fifa96_match_handlers.c:1527`), and `0x83311` clears the
record timer `[rec+0x89]` (the engine `r->timer89 = 0`, `:1528`).

**`FUN_00083164` `0x83164..0x832A6`** (`disassemble_function`, 100 insns) — the
target-triple producer, selected by the live phase byte `[0x157A4D]`:

```
0x8316E MOV AL,[0x157A4D]
0x83173 CMP AL,4 ; JC 0x8317E          ; AL<4 -> 0x8317E
0x8317E CMP AL,3 ; JZ 0x831D5          ; phase 3 (throw-in): triple = 0x15777C (goal snap) +-0x630
0x83182 JMP 0x83211                    ; phases 0/1/2: record-relative triple, clamp fffffec0..0x140
0x83187 (phase 4): [0x157A77]=0, [0x157A7B]=0, [0x157A7F]=0x990;
        if [0x15774C]<0 -> [0x157A77]=0xFFFFF9D0; team byte==1 -> negate [0x157A7F]
0x831D5 (phase 3): copy 0x15777C triple; sign<0 -> [0x157A77]=0xFFFFF9D0 else 0x630
```

So `0x157A77` (the throw target triple) is the **same cell** the taker rows later
read as the focus/target; its per-phase source is the goal snapshot (`0x15777C`,
phase 3), the camera focus (`0x15774C`, phase 4), or the record position
(phases 0/1/2, `0x83211..0x8329E`, byte `[+0x826]` side sign).

**`FUN_0008DB6C` `0x8DB6C..0x8DC49`** (`disassemble_function`, 78 insns) — the
**receiver pick** (`0x1587AC`), an 11-record walk stride `0xb2` (records 0..10):

```
0x8DB89 CMP [EBX+0x20],0 ; JNZ skip        ; slot-empty filter (invert of the usual)
0x8DB8F MOV EAX,[ESP+0x5a] ; SAR EAX,0x10 ; EAX = param_3 (skip index)
0x8DB96 CMP ECX,EAX ; JZ skip              ; skip == this index
0x8DB9A CMP EBX,[EDI+0x7BF] ; JZ skip      ; self
0x8DBA2 TEST ECX,ECX ; 0x8DBA6 CMP byte [EDI+0x829],0 ; JZ skip  ; record 0 needs [team+0x829]
0x8DBAF CMP byte [EBX+0x9A],0 ; JNZ skip   ; +0x9A exclusion set
0x8DBB8 CMP byte [EBX+0x98],0 ; JNZ skip   ; +0x98 exclusion set
0x8DBDF CALL 0x8DC68 (dist of (param_1->[0]-rec.x, param_1->[8]-rec.z)) -> store distance
0x8DBFB if count: CALL 0xA1860 (arg-min) -> EAX = EDI + idx*0xB2   ; return the record
0x8DC1B else if param_4.lo != 0: linear scan [EAX+0x20]==0 && [EAX+0x9A]==0 -> first, else 0
```

The engine's L13.5 stand-in (`fifa96_action_kick` family) substitutes the
record-target nearest search; the native metric is `FUN_0008DC68` (`0x8DC68..
0x8DCD2`, a signed 8-way `max(|dx|,|dz|) + 3/8·min`-style octile distance,
first-hand) — identical in shape to FU-151 § (the `0x8DE8C` metric).

**`FUN_0008DC68` `0x8DC68..0x8DCD2`** (first-hand, 54 insns) — the octile
distance formatter used by both the pick and the taker row distance gates.

**`FUN_00085498` `0x85498..0x855B4`** (`disassemble_function`, 80 insns) — the
**timer+draw completion** (L13.2). The draw is the row's `% span`:

```
0x8549B CMP [EAX+0x20],0 ; JNZ 0x854BE      ; slot path
0x854A1 MOV EAX,[0x1587AC] ; CMP [EAX+0x20],0 ; JZ 0x85538   ; no-slot arm
0x854B0 CMP word [0x1587B0],0 ; JNZ 0x85538
0x854BE ... CALL 0x83428                    ; the input-driven machine (L13.2)
0x85506 ... CALL 0x361B0 / 0x79C50 ...
0x85538 CMP EDX,[ECX+0x89] ; JGE 0x85534    ; the 0x78 timer gate  (EDX=param_2)
0x85540 XOR EDX,EDX ; CALL 0x92AC8 ; 0x85547 DIV EBX   ; RNG % span  (EBX = param_3)
0x8554B MOV byte [EAX+0x7E7],DL             ; the drawn animation/delivery byte
0x85559 [EAX+0x7E4]>>24 >= 3 -> [EAX+0x7E7]=0, [0x1587B4]=7 (else 3)
```

Row 0x10 stage 2 calls it as `0x858AB MOV EBX,4; 0x858B0 MOV EDX,0x78; 0x858B7
CALL 0x85498` — i.e. **timer 0x78, span 4** — and returns 0 to keep the row
waiting. The engine's `match_sp_probe_85498(mr, 4)` models the timer gate and a
`draw % span` into `sp_delivery` (`fifa96_match_handlers.c:1539-1544`), using the
engine's 16-bit RNG (native 32-bit — cadence leg).

**`FUN_00083428` `0x83428..0x83B1D`** (`decompile_function`) — L13.2's
input-driven throw/claim machine: `FUN_0006DAB4` animation install, the
`0x1587B0/0x1587B3/0x1587B4/0x1587B5/0x1587B6` delivery state, the
`FUN_0008DCD4`/`FUN_000702F8` distance/timer decomposition across the per-team
`0x1587E8` block (`[team+0x7E8 + 0xC*idx]`), returns **0..7**. It is also the
presentation consumer at `0x839AB` (FU-204 §282/284) — not taker-exclusive.

### 1.2 Fresh reachability census

| producer | callers this slice | ported? | live? |
|---|---|---|---|
| `FUN_000832A8` | **6** (fresh): row 0x10 `0x857F0`/`0x85870`; row 0x11 `0x85FF3`/`0x8604D`; row 0x12 `0x84092`/`0x840EC` | stand-in `match_sp_setup_832a8` (clears `sp_flag_158784`+`timer89` only) | **live-reachable** via live row 0x11 (stages 4/5) |
| `FUN_00083164` | `FUN_000832A8` `0x832C3` (so reachable from every `FUN_000832A8` caller) | not ported (per-phase triple source dropped) | live-reachable via row 0x11 |
| `FUN_0008DB6C` | `FUN_000832A8` `0x83305`, plus FU-151 pick sites | stand-in via `match_sp_pick_install` | live-reachable via row 0x11 |
| `FUN_00085498` | **3** (fresh): row 0x10 `0x858B7`, **row 0x11 `0x8609F`**, row 0x12 `0x8413E`; (FU-204 row-09 `0x839AB` consumer is `FUN_00083428`) | derived `match_sp_probe_85498` | **live-reachable** via live row 0x11 (stage 5) |
| `FUN_00083428` | `FUN_00085498` `0x854DB` (reachable from the row-0x11 probe) | not ported (rows 1..6 legs) | live-reachable via row 0x11 (stage 5, transitively) |

### 1.3 Live-vs-leg ruling

**(Corrected — erratum to the single-caller claim.)** The throw setup is
reachable from **three** rows, not one: row 0x10 stages 1/2
(`0x857F0`/`0x85870`), **row 0x11 stages 4/5** (`0x85FF3`/`0x8604D`), and row 0x12
stages 1/2 (`0x84092`/`0x840EC`) — fresh `get_xrefs_to 0x832A8 = 6`, fresh
`get_xrefs_to 0x85498 = 3` (`0x858B7`/`0x8609F`/`0x8413E`). Row 0x11 is the
**live** taker (Step 4), so the Step-1 machinery is reachable from a live row.

Row 0x11's stage table `0x85DA0` (fresh read, 10 dwords) =
`{0x85E3C, 0x85E7D, 0x85FA2, 0x85FBD, 0x85FD8, 0x86013, 0x860CB, 0x8610E,
0x8645B, 0x86496}`: stage 4 = `0x85FD8` (the `0x974DC(0x1E)` at `0x85FE6` + the
`FUN_000832A8` call at `0x85FF3`) and stage 5 = `0x86013` (the `FUN_000832A8`
re-run `0x8604D` + the `FUN_00085498` probe `0x8609F`, timer `0x78`, span `4`).
The stage arms cascade (each tail resets `timer89`, `INC byte [rec+0x92]` and
falls into the next: `0x85FA2`→`0x85FBD`→`0x85FD8`→`0x86013`; first-hand
`0x85FA2..0x8600D`), so one row-0x11 dispatch walks stages 2→3→4→5 and **calls
the Step-1 machinery**. The tape asserts only the dispatch mask
(`test_engine_m2.c:5087`), not per-stage progress — the exact tape stage is
**L266-14**.

**Ruling (tape-stage reachability).** Rows 0x10/0x12 stay fixture-only (their
phase-3/phase-7 arms never fire live). Row 0x11 is live and its ladder enters the
Step-1 calls, so the L13.1/L13.2 machinery is **live-reachable**, not
fixture-dormant: the engine stand-ins (`match_sp_setup_832a8`,
`match_sp_probe_85498`) do run on that path; only the deeper vector/record/machine
sources stay unported (L266-1..L266-4).

### 1.4 Numbered open legs (Step 1)

- **L266-1 — `FUN_000832A8` staging block** (`0x158790` const block, `0x15879C`,
  `0x1587B0/2/5/6/7`, `0x1587AC`, `0x15877E/7F`, the `0x361A4/0x361B0/0x36200`
  presentation calls). Native owner: `FUN_000832A8`. Port has only
  `sp_flag_158784`+`timer89`.
- **L266-2 — `FUN_00083164` per-phase target triple** (`0x8316E..0x8329E`; phase
  byte `[0x157A4D]`, sources `0x15777C`/`0x15774C`/record). Native owner:
  `FUN_00083164`; consumer `0x157A77` → `0x15879C`.
- **L266-3 — `FUN_0008DB6C` receiver pick + `FUN_0008DC68` metric** (skip index
  `[rec+0x8D]`, `+0x98`/`+0x9A`/`+0x7BF`/`+0x829` filters, arg-min `0xA1860`).
  Native owner: `FUN_0008DB6C`; engine stand-in `match_sp_pick_install`.
- **L266-4 — `FUN_00085498`/`FUN_00083428` machine** (timer 0x78/span 4, the
  `0x1587B0..0x1587B6` delivery state, the per-team `0x1587E8` block, returns
  0..7). Native owner: `FUN_00083428`; the engine keeps only the derived
  timer+draw completion.

---

## Step 2 — L13.3–L13.5: the arm vector/RNG cadence, the sinks, the pick origins

### 2.1 Native producers (first-hand)

**`FUN_000974DC` `0x974DC..0x974EC`** — the `0x974DC(0x1E)` sink; body read
first-hand: `EDX=EAX; CALL 0x64EC0; EAX=EDX; CALL 0x65CC0` (the audio pair —
FU-150 §216 cross-cited). Fresh `get_xrefs_to 0x974DC` = **42** sites. The
taker/scanner set:

```
0x88B64 MOV EAX,0x1E ; 0x88B69 CALL 0x974DC   ; scanner corner/goal-kick arm
0x88BD9 MOV EAX,0x1E ; 0x88BDE CALL 0x974DC   ; scanner throw-in arm
0x857DF MOV EAX,0x1E ; 0x857E4 CALL 0x974DC   ; row 0x10 stage 1, gated [0x14C32A]==0
0x85FE1 MOV EAX,0x1E ; 0x85FE6 CALL 0x974DC   ; row 0x11 stage 4, gated [0x14C32A]==0
0x84080 MOV EAX,0x1E ; 0x84085 CALL 0x974DC   ; row 0x12 stage 1
0x84C98 MOV EAX,0x1E ; 0x84C9D CALL 0x974DC   ; row 0x13 stage 1
0x7507B / 0x7A5D1 ...                          ; other rows/scanner (FU-149 L13.4)
```

The sink argument is the **literal `0x1E`** loaded into EAX (bytes
`b8 1e 00 00 00`), then the call — no register ambiguity.

**`FUN_0008F188` `0x8F188..0x8F2C3`** — the speech/event queue signal (FU-150
§53: ring `0x15AAE0`, 130 sites). Body first-hand: head gates
`[0x14C32A]==0` (`0x8F192 JNZ exit`) and `[0x14C312]!=0` (`0x8F19F JZ exit`);
classifies `EBX ∈ {8, 0x20, 0x21, 0x40}` into `[0x15B36C/70/74]`; then pushes a
record into the 10-slot ring keyed by `[0x15AADC]` (`0x8F23C [EDX+0x15AAFC]=ESI`
the record, `0x8F259 LEA ESI,[ESI+0x59]` the triple, `0x8F27D/0x8F29D/0x8F2B3`
the fields, `0x8F2B3 INC [0x15AAD8]`, `0x8F2B9 MOV [0x15AADC],EDX`). The taker
arms call it (fresh `get_xrefs_to 0x8F188` = 130; the phase-arm subset):

```
0x8D32A MOV EAX,0x17 ; 0x8D32F EDX=[EBP+0x7B2] ; 0x8D335 CALL 0x8F188   ; phase 3 taker event 0x17
0x8D3FD MOV EBX,8 ; 0x8D402 MOV EAX,0x1A ; 0x8D407 EDX=[EBP+0x7B2] ; 0x8D40D CALL 0x8F188  ; phase 4 (RNG&3)
0x84C25 ... MOV EAX,0xA4 ; 0x8412C CALL 0x8F188                          ; row 0x12 stage 1 timer event 0xA4
0x8589A ... MOV EAX,0xA5 ; 0x858A6 CALL 0x8F188                          ; row 0x10 stage 2 timer event 0xA5
0x858FD MOV EAX,0x18 ; EDX=EBP ; 0x85909 CALL 0x8F188                    ; row 0x10 stage 3 event 0x18
0x86086 MOV EAX,0xA4 ; 0x8608D CALL 0x8F188   ; row 0x11 stage 5 timer event 0xA4
0x86391/0x863BE MOV EAX,0x19|0x1C ; 0x863CB CALL 0x8F188  ; row 0x11 stage 7 delivery event
0x83999/0x839AF (row-09 consumer 0x8F188 0x5D/0x60/0x5E — FU-204 §263-267)
```

**The phase-arm taker installs + RNG cadence** (first-hand, `disassemble_bytes`):

- phase 3 (`0x8D2A3..0x8D359`): `0x8D2A3 PUSH -1; ECX=3; EBX=0xA; EDX=0;
  CALL 0x8CEB8` (install 3 over records 0..10); `0x8D2B8 MOV EAX,[0x157AAC];
  SAR EAX,0x18; DL=[EBP+0x826]; CMP EDX,EAX; JNZ 0x8D814` (controlled gate);
  snapshot copy `0x157740→stack`; `0x8D301 CALL 0x700F4` (camera reset);
  `0x8D310 CALL 0x79CCC` (pick); `0x8D31A MOV [EBP+0x7B2],EAX`;
  `0x8D32A EAX=0x17; CALL 0x8F188`; `0x8D33C EDX=0x10; EAX=[EBP+0x7B2];
  ECX=0; EBX=0; CALL 0x7D9A4` — **install 0x10** (no RNG draw).
- phase 4 (**arm entry `0x8D35B`**; the shared `install 3` prefix + controlled
  gate, then the RNG/event/install tail `0x8D3F4..0x8D42F`):
  `0x8D3F4 CALL 0x92AC8; TEST AL,3; JZ 0x8D412` — the event is skipped when
  `(rand & 3) == 0`; `0x8D3FD EBX=8; EAX=0x1A; EDX=[EBP+0x7B2]; CALL 0x8F188`
  — event 0x1A fires on `(rand & 3) != 0`, i.e. **3-in-4**;
  `0x8D412 EDX=0x11; EAX=[EBP+0x7B2]; ECX=0; EBX=0; CALL 0x7D9A4` — **the
  `install 0x11` is unconditional** (reached from both the event and the skip
  path; FU-149 §1.5). *(Erratum: the earlier "1-in-4 corner event" wording was
  inverted — the `JZ` skips the event on `(rand&3)==0`, so the event is 3-in-4
  and the install is not gated.)*

**`FUN_00092AC8` `0x92AC8..0x92BC1`** (`disassemble_function`, 72 insns) — the
shared deterministic RNG: a 5-word (`0x110F44/48/4C/50/54`) shift-register with
the `0x110F58` counter and a carry chain (`0x92B5B..0x92BB7`). Every taker draw
(audit: phase-4 `&3`; row-0x10 probe `% span`; row-0x11 delivery `&1`) consumes
this state — the RNG cadence the port must sequence exactly.

**The kick event sub-tables** (first-hand `read_memory`, base `0x855D4` etc.):

| table | 7 dwords |
|---|---|
| row 0x10 events `0x855D4` | `0x85A3F 0x85AA5 0x85A30 0x85A3F 0x85AA5 0x85A30 0x85AF0` |
| row 0x11 events `0x85DC8` | `0x861F9 0x86235 0x862BD 0x86272 0x862AA 0x862BD 0x862F4` |
| row 0x12 events `0x83D4C` | `0x842AB 0x8432F 0x842E7 0x84368 0x843A0 0x842E7 0x843AE` |
| row 0x10 stages `0x855B8` | `0x8566D 0x857D6 0x8580D 0x858E4 0x85CC4 0x85D5A 0x85D8D` |

These match FU-149 §7.1 byte-for-byte. The vectors the native passes are the
`0x158738` staging vector, the `0x15879C` triple, the `0x1587AC` record position,
the slot words, or an RNG-built triple (L13.3); row 0x12 additionally splits on
`FUN_0006DBCC == 3` (`0x84425..0x84443`) into a `mode 0x40` strike vs the
`0x8DE8C`+`0x7F668` hand-off (FU-149 §7.3.3, cross-cited).

**`FUN_0008DE8C` `0x8DE8C..0x8DEFF`** (`disassemble_function`, 42 insns) — the
`0x8DE8C` pick: 11 records stride `0xb2`; filters `ECX==param_3(skip)`,
`[EBX+0x9A]`, `[EBX+0x98]`; `FUN_0008DC68` metric; strict-minimum
(`CMP AX,SI; JNC`); writes the winning distance to `*param_4` and returns the
record pointer (`EBP`) or NULL. Fresh `get_xrefs_to 0x8DE8C` = **37** (FU-151 §49
cross-cited). Its callers include the row 0x12 hand-off (`0x84462`) and the
keeper rows.

### 2.2 Fresh reachability census

| sink/producer | sites (this slice) | ported? | live? |
|---|---|---|---|
| `0x974DC(0x1E)` scanner arms | `0x88B69`, `0x88BDE` | dropped (engine scanner has no sound) | fixture-only |
| `0x974DC(0x1E)` row 0x10 | `0x857E4` (gate `[0x14C32A]==0`) | dropped | fixture-only |
| `0x974DC(0x1E)` row 0x11 stage 4 | **`0x85FE6`** (gate `[0x14C32A]==0`) | dropped | **live-reachable** |
| `0x974DC(0x1E)` row 0x12 stage 1 | `0x84085` | dropped | fixture-only |
| `0x974DC(0x1E)` row 0x13 | `0x84C9D` | dropped | fixture-only |
| `0x8F188` phase-arm events | `0x8D335` (0x17), `0x8D40D` (0x1A), `0x8412C` (0xA4), `0x858A6` (0xA5), `0x85909` (0x18), `0x8608D` (0xA4), `0x863CB` (0x19/0x1C) | dropped (L7) | fixture-only |
| `FUN_0008DE8C` pick | `0x8DE8C` family (37) | stand-in (record-target nearest) | fixture-only in taker rows |
| RNG `0x92AC8` | phase-4 `&3`, row-0x10 `%4`, row-0x11 `&1` | `fifa96_rng_step` (16-bit) | consumed where the arm fires |

### 2.3 Live-vs-leg ruling

The sinks are pure presentation/audio; dropping them is honest and does **not**
change the phase/action cells (the native events sit on gates the port keeps as
timers). The **arm vector/RNG cadence is the live discriminator**: the phase-4
`&3` draw (and hence the 0x1A event) runs whenever the phase-4 arm runs — and
the phase-4 arm DOES run live (row 0x11 dispatches, Step 4). The `0x8DE8C` pick
origins (`0x157770` camera-led reception triple, `0x1587AC` staged receiver) stay
stand-ins; **ruling: fixture-only for rows 0x10/0x12/0x13, and the RNG draw
consumed live only inside row 0x11's path.** The engine consumes the phase-4
`&3` draw at `fifa96_match_run.c:2884` (`fifa96_rng_step`) so the cadence is
preserved on the one arm that fires.

### 2.4 Numbered open legs (Step 2)

- **L266-5 — taker presentation sinks** (`0x974DC(0x1E)` at `0x88B69/0x88BDE/
  0x857E4/0x84C9D`; `0x8F188` ids 0x17/0x18/0x19/0x1A/0x1C/0xA4/0xA5; `0x6E598`,
  `0x92820`, `0x4C324/0x4C31C/0x4C374/0x4C380`, `0x918CC`, `0x8CFAC`, `0x585A0`,
  `0x361xx`, `0x83B80`/`0x158747` wall vector). Native owners: the sinks above;
  engine owns the ring/staging in `fifa96_match_run.c` where wired.
- **L266-6 — `FUN_0008DE8C`/`0x157770` pick origins** (param triple `0x157770`,
  the `0x1587AC` staged-receiver position; metric `0x8DC68`). Native owner:
  `FUN_0008DE8C`; engine stand-in (`match_sp_pick_install`, `match_run_phase_pick`).
- **L266-7 — kick event sub-tables + the per-team `0x1587E8` block**
  (`[team+0x7E7]`, the `0x86388` `&1` draw, the row-0x12 `0x6DBCC==3` split and
  `mode 0x40` vs the `0x8DE8C`+`0x7F668` hand-off). Native owners: the row
  bodies `0x855F0/0x85DE4/0x83D68/0x84B00`; engine collapses to one derived
  delivery per kick stage.

---

## Step 3 — L13.6–L13.7: the `[0x157821]`/`[0x157A6A]` state + the penalty

### 3.1 `[0x157821]` — the row-0x10 re-queue gate (first-hand)

Fresh `get_xrefs_to 0x157821` = **11**: `FUN_000736AC 0x739BB` READ,
`FUN_000709D0 0x70A4C` READ + `0x70A65` READ_WRITE,
`FUN_00070544 0x7063D` READ, **`FUN_000700F4 0x70153` WRITE**,
`FUN_00077EAC 0x7812E/0x78144` READ, row 0x10 **`0x85CE4` READ**,
`0x7489C`/`0x74A8C`/`0x7E295` READ.

Producer (first-hand):

```
FUN_000700F4 0x70144 XOR AH,AH
             0x70153 MOV byte [0x157821],AH     ; = 0  (ALIAS: dword[0x15781E]>>24)
FUN_000709D0 0x70A3F MOV EAX,[0x15781E] ; SAR EAX,0x18   ; = byte [0x157821]
             0x70A4A CMP EAX,0x64 ; JGE 0x70A6B        ; cap 0x64
             0x70A4C CMP byte [0x157821],0 ; JNZ 0x70A5F
             0x70A55 CALL 0xCB2A4 ; MOV [0x1575C3],EAX   ; seed once on 0->1
             0x70A5F INC byte [0x1575CB]
             0x70A65 INC byte [0x157821]
```

The row-0x10 gate (first-hand bytes `0x85CE4..0x85D0A`):

```
0x85CDD CMP EAX,0x720 ; 0x85CE2 JL 0x85D19         ; |camera focus x| < 0x720 -> resolve
0x85CE4 CMP byte [0x157821],0
0x85CEB JLE 0x85D94                                 ; signed byte <= 0 -> exit (no requeue)
0x85CF1 MOV EDX,[EBP] ; MOV DL,[EDX+0x826]
0x85CFA MOV EBX,1 ; 0x85CFF MOV EAX,2 ; 0x85D0A CALL 0x8A938  ; re-queue situation 2 BX=1
```

So `[0x157821]` is a **signed byte counter** (0 → 0x64), cleared by the
set-piece camera reset `FUN_000700F4` and incremented per frame by the camera
pan-step `FUN_000709D0`. Its producer is **fully native and already ported** as
`fifa96_camera.pan_counter` (`src/fifa96_loader/fifa96_camera.c:863` clears,
`:893` caps at 100; `fifa96_match_run.c:3729` is the doc comment — the code gate
is `:3747`, `if (cam->pan_counter != 0) return 0`).

**Key divergence (Step 4):** the engine's row 0x10 reads a **shadow**
`mr->sp_157821` (`fifa96_match_handlers.c:1724`) seeded 0 at init/begin
(`fifa96_match_run.c:2087/2219`), **not** `cam->pan_counter`. So even if a live
camera pan set the real native counter, the ported row's requeue would not see
it. Native owner of the producer: `FUN_000700F4` (clear) + `FUN_000709D0`
(increment).

### 3.2 `[0x157A6A]` — the offside suppression timer (first-hand)

Fresh `get_xrefs_to 0x157A6A` = **6**: `FUN_00073E28 0x73E85` WRITE, `FUN_00079D5C
0x79D79` READ, row 0x10 `0x85D19` WRITE, row 0x11 `0x863DF` WRITE, `0x7438D`
READ, `0x743A5` READ_WRITE.

```
FUN_00073E28 0x73E72 MOV EAX,[0x14C2F6] ... 0x73E85 MOV word [0x157A6A],BX  ; reset -> 0 (BX=0)
row 0x10     0x85D19 MOV word [0x157A6A],0x12C     ; bytes 66 c7 05 6a 7a 15 00 2c 01
row 0x11     0x863D8 MOV ECX,0x12C ; 0x863DF MOV word [0x157A6A],CX
countdown    0x7438D MOV AX,[0x157A6A]
             0x7439A CMP BX,AX ; JGE 0x743AC
             0x743A5 SUB word [0x157A6A],AX     ; -= [0x157A64] (the per-frame delta)
```

So `[0x157A6A]` is a 16-bit **countdown** decremented by the per-frame tick
`[0x157A64]` (`0x7436A MOV [0x157A64],AX` from `[0x157A5C]>>8`); rows 0x10/0x11
arm it to 0x12C on resolve. Reader: the offside check `FUN_00079D5C 0x79D79`
(FU-150 §191, cross-cited). The producer reset is `FUN_00073E28` (the match-reset
helper). Engine: `referee.offside_suppress` with the ported countdown
(`fifa96_match_run.c:6664-6670`, `fifa96_referee.c:134-136`) — **ported and
live-consistent**.

### 3.3 `goal_armed [0x15781D]` — the scanner's live gate (first-hand)

Fresh `get_xrefs_to 0x15781D` = **11**: `FUN_0007131C` READ `0x7137D` + WRITE
`0x713DB`/`0x71908`, `FUN_00072AC4 0x72AD3` READ, `FUN_000740A0 0x740F6` WRITE
(phase 2 clears), `FUN_0008AF38 0x8B635` READ (the scanner gate), row 0x20
`0x84F90` WRITE, `0x89A7A` READ, `0x93C87`/`0x9437A`/`0x9453C` WRITE (restart
handlers). The scanner gate itself (first-hand `0x8B623..0x8B643`):

```
0x8B623 MOV EAX,[0x157A4A] ; SAR EAX,0x18       ; the live phase byte
0x8B62B CMP EAX,2 ; JZ 0x8B635
0x8B630 CMP EAX,0x10 ; JNZ 0x8B645               ; phase must be 2 or 0x10
0x8B635 CMP byte [0x15781D],0 ; JZ 0x8B65B      ; goal_armed must be set
0x8B63E CALL 0x88940                            ; the restart scanner
```

The block has a **third sibling branch** beyond the two scanner entries: the
`phase == 0xC` arm `0x8B645..0x8B64E` (`0x8B645 CMP EAX,0xC; JNZ 0x8B65B;
0x8B64A XOR EBX,EBX; 0x8B64C XOR CH,CH; 0x8B64E MOV word [0x157ABA],BX`) clears
the period-0xC stall word `[0x157ABA]` and does **not** call the scanner — so the
only scanner entries remain `phase ∈ {2, 0x10}` (first-hand bytes).

### 3.4 The penalty remainder — row 0x13 `0x84D5B` (first-hand)

Row 0x13 `0x84B00..0x84EEB`: phase gate `{2,6}` (`0x84B13 CMP 2 / 0x84B18 CMP
6`); head spot aim (`[EBP+0x8F]>>24 < 5` -> `0x84B31 [0x157A83]=EBP`,
`0x84B4F CALL 0x73DC4` the spot triple, `0x84B6B CALL 0x700F4`); then
`0x84B73 MOV EAX,[EBP]→[EAX+0x7A6]` (the opponent team block, stored `[ESP]`);
stage jump `0x84BA5 JMP CS:[EAX*4+0x84AE4]`. The `0x84D5B` free-record hand-off:

```
0x84D46 MOV EAX,ECX ; AND EAX,0xFFFF ; CMP EAX,0x20 ; JNZ 0x84DBA   ; slot word == 0x20
0x84D52 CMP byte [0x14C32A],0 ; JNZ 0x84DBA                          ; session gate closed
0x84D5B MOVSX AX,byte [EBP+0x8D] ; INC EAX                          ; start = [rec+0x8D]+1
0x84D67 CMP EDX,0xB ; JNZ ; 0x84D6C MOV EAX,1                        ; wrap 0xB -> 1
0x84D74 IMUL EDX,EDX,0xB2                                            ; record stride
0x84D7D CMP byte [EDX+EBX+0x9A],0 ; JNZ 0x84D63                     ; walk while +0x9A set
0x84D87 CALL 0x7DAB4                                                 ; reset self
0x84D8E MOV EAX,[EBP] ; ADD EDX,EAX ; MOV [EAX+0x7B2],EDX           ; team->target = found record
0x84D9E EDX=0x13 ; EAX=[EAX+0x7B2] ; CALL 0x7D9A4                  ; install 0x13 on it
```

The walk is **unbounded** (no `+0x9A==0` guard terminator beyond the `0xB→1`
wrap; if all 1..10 have `+0x9A` set it loops forever). The input words
`0x14C114/0x14C118/0x14C11C` are written at `0x84CCC..0x84D0C` (per-side);
`0x84D19 CMP word [EAX+0x71],0; JG exit` is the opponent-record slot gate.
**Hang remediation (erratum).** "Records 1..10" is **not** a bound: the native
walk already visits only records 1..10 (`start = [rec+0x8D]+1`, wrap `0xB→1`), so
the all-`+0x9A` case still spins forever. The correct remediation is an
**iteration cap**, whose precedent is already in the tree:
`fifa96_match_run.c:2753` — `for (int n = 0; n < 10; n++)` in
`match_run_free_record`, whose closing comment `:2759` reads *"native loops
forever; engine sentinels"*. A faithful port of `0x84D5B` must cap the walk at 10
iterations and return the caller's fallback when no `+0x9A`-clear record is
found, exactly as that sibling selector does.

### 3.5 Fresh reachability census & ruling

| cell | writer (fresh) | ported? | live? |
|---|---|---|---|
| `[0x157821]` | `FUN_000700F4` (0), `FUN_000709D0` (INC cap 0x64) | native ported as `fifa96_camera.pan_counter`; **row reads a shadow `sp_157821`** | producer live (pan); row gate dormant |
| `[0x157A6A]` | `FUN_00073E28` (0), rows 0x10/0x11 (0x12C) + countdown `0x743A5` | `referee.offside_suppress` (ported) | live (rows arm it when they resolve) |
| `[0x15781D]` goal_armed | `FUN_0007131C`, cleared by `FUN_000740A0`/row 0x20 | unported (engine uses `goal_armed` mirror seeded 0) | the scanner gate; live only after a goal event |
| penalty `0x84D5B` | row 0x13 body (install 0x13) | not ported (row 0x13 stage 5 hand-off dropped) | fixture-only (phase 6 not reached) |

**Ruling.** `[0x157A6A]` is **live** (rows arm it; engine ported). `[0x157821]`'s
**producer is live** (camera pan) but the **row-0x10 gate is dormant** because the
engine reads a separate `sp_157821` — a real port divergence, not merely an
unported leg. The penalty `0x84D5B` walk is a native **hang risk** (unbounded),
fixture-only.

### 3.6 Numbered open legs (Step 3)

- **L266-8 — `[0x157821]` shadow cell** (row 0x10 `0x85CE4` reads the run's
  `sp_157821`, the native producer is `fifa96_camera.pan_counter` at the same
  address). Native owner: `FUN_000700F4`/`FUN_000709D0`; port owns
  `fifa96_camera.c`. **Evidence that the port and native would disagree.**
- **L266-9 — `[0x15781D]` goal_armed producer** (`FUN_0007131C 0x713DB/0x71908`;
  clear `FUN_000740A0 0x740F6`, row 0x20 `0x84F90`, restart handlers
  `0x93C87/0x9437A/0x9453C`). Native owner: `FUN_0007131C`; engine `goal_armed`.
- **L266-10 — penalty `0x84D5B` hand-off** (unbounded walk, `+0x9A` filter,
  `[team+0x7B2]` install, `0x14C114/18/1C` input words, `word[+0x71]` opponent
  gate). Native owner: row 0x13 `0x84B00`; port drop + a **10-iteration cap**
  (precedent `fifa96_match_run.c:2753`), not "records 1..10".
- **L266-11 — `[0x157A6A]` reset owner** (`FUN_00073E28` is the match-reset
  helper; verify it is the sole reset in the live loop). Native owner:
  `FUN_00073E28`.

---

## Step 4 — the port shapes + the live-trigger ruling (hand-off)

### 4.1 What makes `0x10..0x13` fire live

**Layer split (explicit).**
* **Layer 1 — the producer raiser:** the scanner gate `phase ∈ {2,0x10} &&
  goal_armed [0x15781D]` → `FUN_00088940` → `FUN_0008A938` → `FUN_000740A0(phase,
  side)` → the phase arm `FUN_0008D098` that *installs* a taker code.
* **Layer 2 — the row dispatch:** the installed code `0x10..0x13` → the action
  table `0x1106E0` → the row body `0x855F0/0x85DE4/0x83D68/0x84B00` → resolve.

A row is live **only** if Layer 1 raises its code on a real path; a fixture that
stages the phase or the arm gate is Layer-2-only and therefore a fake.

The chain (all stages first-hand above):

1. **Scanner gate** `FUN_0008AF38 0x8B623..0x8B643`: live `phase ∈ {2, 0x10}`
   (from `[0x157A4A]>>24`) **AND** `goal_armed [0x15781D] != 0` → `CALL 0x88940`.
   `goal_armed` is set by the camera/goal event `FUN_0007131C` and cleared by the
   phase-2 write `FUN_000740A0 0x740F6` and the restart commit `0x84F90`.
2. **Scanner `FUN_00088940`** arms a situation + side: throw-in sit 2
   (`0x88BD9`, `|snap z|<=0xB20`), corner/goal-kick sit `3+cond` (`0x88B64`),
   with the `0x974DC(0x1E)` sound.
3. **Dispatcher `FUN_0008A938`** routes to `FUN_000740A0(phase, side)`.
4. **Phase arm `FUN_0008D098`** installs the taker code on the controlled team's
   chosen record (`0x7D9A4`): phase 3 → `0x10` (`0x8D33C`, no draw), phase 4 →
   `0x11` (`0x8D412`, unconditional; the `&3` draw at `0x8D3F4` only gates the
   3-in-4 event 0x1A), phase 7 → `0x12`, phase 6 → `0x13`.
5. The action table (`0x1106E0[0x10..0x13]` = `0x855F0/0x85DE4/0x83D68/0x84B00`)
   dispatches the row; the row resolves to phase 2 via `FUN_0008A938(0xB,...)`.

So a taker row fires **only** when (a) the scanner runs (`phase 2 && goal_armed`)
and (b) the phase arm's controlled-side gate `byte[team+0x826] ==
byte3[0x157AAC]` matches. A fixture that stages the phase or the gate is a fake.

### 4.2 Engine port shapes (first-hand source)

- **`fifa96_match_action_10..13`** (`src/fifa96_engine/fifa96_match_handlers.c`
  `:1609/1756/1897/2038`; table `:5486..5492`). Bodies ported per FU-149 §7.2;
  row 0x10's requeue reads the shadow `sp_157821` (`:1724`); rows resolve through
  the shared `match_sp_resolve` (`:1553`, writes `referee.offside_suppress`).
- **`fifa96_match_run_phase_arm`** (`fifa96_match_run.c:2762`): phases 3/4/6/7/8/9/
  0xD; phase 3 installs `0x10` (`:2872`), phase 4 consumes the `&3` RNG draw
  (`:2884`) + installs `0x11` (`:2885`), phase 7 installs `0x12` (`:2833`),
  phase 6 installs `0x13`/`0x1F` (`:2846/:2849`).
- **`fifa96_match_run_set_piece`** (`:3111`): the `FUN_0008A938` head, the queue
  ids, the BX fallback, the corner-count `side^side_swap` write.
- **Scanner** `fifa96_match_run_goal_scan` (`:3728` region + FU-149 §6.4): the
  `|snap z|<=0xB20` throw-in arm and the `3+cond` corner/goal-kick arm.
- **State**: `sp_flag_158784`/`sp_delivery`/`sp_157821`
  (`include/.../fifa96_match_run.h:826-832`, seeded 0 at
  `fifa96_match_run.c:2085-2087/2217-2219`); `referee.offside_suppress`
  (`fifa96_referee.h:104`); `corner_count`/`side_swap`/`sit_side_pending`
  (`:798-802`); `fifa96_camera.pan_counter` (`fifa96_camera.h:28`).

### 4.3 The explicit live-vs-leg ruling

| row | install owner | tape status (fresh test source) | ruling |
|---|---|---|---|
| **0x11 corner taker** | phase-4 arm `0x8D35B`/`0x8D412` | **live on the tape**: `tests/test_engine_m2.c:5087` `assert(res.mask_final == (M2_WIRED_MASK \| M2_RESTART_ROWS_MASK))`, `M2_RESTART_ROWS_MASK = 1<<0x11` (`:1902`); the post-goal restart chain (v14, FU-170) reaches sit-3 → phase 4 → install 0x11 | **LIVE** |
| **0x10 throw-in taker** | phase-3 arm `0x8D2A3`/`0x8D33C` | absent: `assert((mr->dispatched_ok & M2_TAKER_ROWS_MASK) == 0u)` at m 41 (`:2480`) and m 62 (`:2859`) | **fixture-only** |
| **0x12 free-kick taker** | phase-7 arm `0x8D57B` | same absence assertion; FU-150 foul adjudicator is P2/unported | **fixture-only** |
| **0x13 penalty taker** | phase-6 arm `0x8D4A2` | same absence assertion; penalty chain not reached on tape | **fixture-only** |

The single live trigger is therefore the **post-goal restart chain** (act-9 phase
0x15 → SETUP2 sit-3 → phase 4 → corner arm → `0x11`, M2 v14+, FU-170). Rows
0x10/0x12/0x13 need the throw-in scanner and the foul/penalty adjudicator, neither
of which fires on the tape: the scanner's `phase 2 && goal_armed` gate is closed
by the post-goal hold, and the foul producer is FU-150/P2 (unported). **The
`0x974DC`/`0x8F188` sinks and the `[0x157821]` shadow never gate a live taker
row**; the only live gate the port must keep faithful is the scanner
`phase∈{2,0x10} && goal_armed` + the controlled-side arm gate.

### 4.4 Hand-off

- **Do not** wire `sp_157821` to the camera probe blindly: the native cell is the
  shared `0x157821` (camera pan counter); unifying it (L266-8) changes row 0x10's
  requeue threshold behavior and must be fixture-pinned before the tape.
- The live `0x11` path is already wired and tape-pinned; no re-pin demanded by
  this slice.
- The producer work for rows 0x10/0x12/0x13 is the scanner (throw-in) and the
  foul adjudicator (FU-150/P2) — outside this slice's owner set.

### 4.5 Numbered open legs (Step 4)

- **L266-12 — the live-trigger census for `0x10`/`0x12`/`0x13`** (what live path,
  beyond the post-goal restart, can raise the scanner with a throw-in/foul
  situation, and at what `[0x15781D]`/phase). Native owners: `FUN_0007131C`
  (goal_armed), `FUN_0008AF38` (scanner gate), `FUN_00088940` (arms). Cross-task:
  Task 8 (W26-8 live-trigger census).
- **L266-13 — the engine `sp_157821`↔`pan_counter` unification** (does the port
  intend one cell? the native does). Native owner: `FUN_000700F4`/`FUN_000709D0`.
- **L266-14 — the live row-0x11 tape stage** (the tape asserts the dispatch mask
  only, `test_engine_m2.c:5087`; which stage of `0x85DA0` the ladder enters —
  and so which of stage 4's `FUN_000832A8` / stage 5's `FUN_00085498` actually
  runs on the tape — is unpinned). Native owner: row 0x11 `0x85DE4`; test owner:
  `tests/test_engine_m2.c`.

---

## Legs summary (owner-tagged)

| leg | subject | native owner |
|---|---|---|
| L266-1 | `FUN_000832A8` staging block | `0x832A8` |
| L266-2 | `FUN_00083164` per-phase target triple | `0x83164` |
| L266-3 | `FUN_0008DB6C` receiver pick + `0x8DC68` metric | `0x8DB6C`/`0x8DC68` |
| L266-4 | `FUN_00085498`/`FUN_00083428` machine | `0x85498`/`0x83428` |
| L266-5 | taker presentation sinks (`0x974DC`, `0x8F188`, …) | scanner/rows |
| L266-6 | `FUN_0008DE8C`/`0x157770` pick origins | `0x8DE8C` |
| L266-7 | kick event sub-tables + `0x1587E8` block | rows `0x855F0..` |
| L266-8 | `[0x157821]` shadow cell | `0x700F4`/`0x709D0` |
| L266-9 | `[0x15781D]` goal_armed producer | `0x7131C` |
| L266-10 | penalty `0x84D5B` hand-off | row 0x13 `0x84B00` |
| L266-11 | `[0x157A6A]` reset owner | `0x73E28` |
| L266-12 | live-trigger census for rows 0x10/0x12/0x13 | `0x7131C`/`0x8AF38`/`0x88940` |
| L266-13 | `sp_157821`↔`pan_counter` unification | `0x700F4`/`0x709D0` |
| L266-14 | live row-0x11 tape stage (which of `0x85DA0` runs) | row 0x11 `0x85DE4` |

## Risks

- **Shadow-cell divergence (L266-8).** The engine's `sp_157821` and the native
  `[0x157821]` (the camera pan counter) are the same address; wiring the row gate
  to the real producer changes row-0x10 behavior — pin with a fixture.
- **Penalty walk hang (L266-10).** The native `0x84D5B` loop is unbounded (it
  already visits only records 1..10, so "harden to 1..10" does not help); a
  faithful port could hang on an all-`+0x9A` team. Cap the walk at 10 iterations
  (precedent `fifa96_match_run.c:2753`).
- **Drop-the-sinks temptation.** The `0x974DC(0x1E)`/`0x8F188` sinks sit on
  timers, not on the phase/action cells; dropping them is honest only while the
  gates are kept (the row-0x10 stage-1 and row-0x11 stage-4 sinks are gated on
  the session gate `[0x14C32A]`, which the engine must still model).
- **RNG cadence.** The three taker draws (`&3`, `%4`, `&1`) consume the shared
  `0x92AC8` state; dropping any draw shifts all downstream draws.

---

## Fix report (round 1/5)

Read-only; all re-verified first-hand this round. No code/tests/commits, no Ghidra
writes.

1. **Critical — false single-caller claim (§1.1/§1.2/§1.3).** Fixed. Fresh
   `get_xrefs_to 0x832A8` = **6** (`0x84092`/`0x840EC` row 0x12; `0x857F0`/
   `0x85870` row 0x10; `0x85FF3`/`0x8604D` row 0x11); fresh
   `get_xrefs_to 0x85498` = **3** (`0x8413E` row 0x12, `0x858B7` row 0x10,
   `0x8609F` row 0x11). The §1.2 census rows now list all callers; §1.3 is
   rewritten to rule on **tape-stage reachability**: row 0x11's stage table
   `0x85DA0` (fresh read) has stage 4 = `0x85FD8` (`0x974DC(0x1E)` + `0x832A8`)
   and stage 5 = `0x86013` (`0x832A8` re-run + `0x85498` span 4), and the arms
   cascade, so the Step-1 machinery is **live-reachable** via row 0x11; the exact
   tape stage is a new leg **L266-14** (the tape asserts the dispatch mask only).
2. **Important — hang remediation (L266-10/§3.4/Risks).** Fixed. The native walk
   already visits only records 1..10, so "harden to 1..10" cannot bound it; the
   fix is an **iteration cap** citing the in-tree precedent
   `fifa96_match_run.c:2753` (`for (n = 0; n < 10; n++)`, comment `:2759`
   "native loops forever; engine sentinels").
3. **Minor — phase-4 arm label.** Fixed: arm entry `0x8D35B`; `0x8D3F4..0x8D42F`
   is the shared install-3 prefix's RNG/event/install tail.
4. **Minor — `&3` event mislabel.** Fixed: `0x8D3F4 CALL 0x92AC8; TEST AL,3;
   JZ 0x8D412` skips the event on `(rand&3)==0`, so event 0x1A is **3-in-4** and
   the `install 0x11` at `0x8D412` is **unconditional** (both paths converge).
   Corrected in §2.1, §2.2, §4.1.
5. **Minor — citation nits.** Fixed: `fifa96_match_run.c:3729` is the doc
   comment, code gate is `:3747`; `fifa96_camera.c` path is
   `src/fifa96_loader/fifa96_camera.c`; the Step-1 report line "`DIV EDX(spans)`"
   corrected to native `DIV EBX` (span in EBX) in `task-3-report.md`.
6. **Minor — scanner sibling branch.** Added §3.3: the `phase == 0xC` branch
   `0x8B645..0x8B64E` clears `[0x157ABA]` and does **not** call the scanner; the
   only scanner entries are `phase ∈ {2, 0x10}`.
7. **Minor — Layer split.** Added §4.1: **Layer 1** = the producer raiser
   (scanner gate → `FUN_0008A938` → phase arm install); **Layer 2** = the row
   dispatch (installed code → action table → row body). A row is live only if
   Layer 1 raises its code.

New legs this round: **L266-14** (live row-0x11 tape stage). Legs L266-1..L266-13
unchanged except L266-10's remediation wording.
