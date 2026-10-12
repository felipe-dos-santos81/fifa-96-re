# FU-197 — the act-4 sit-2..4 formation-order arm, its `FUN_0008C974` counter/​sorter, the `0x14C360`/`0x157BD2` matrix feeders, and the act-producer census

Provenance: M2 phase-17 wave **R45**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; `get_current_program_info` = `watcom:LE:32:default`, `min_address`
`0x10000`). Every address/byte below is a fresh `read_memory` /
`disassemble_bytes` / `disassemble_function` / `decompile_function` /
`get_xrefs_to` / `search_instructions` of **this** session unless marked
*cross-cited* (then the frozen FU doc is named). The addresses quoted are the
`/FIFA96.EXE` program addresses (already the runtime ones), not file offsets. The
alias traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`).
No writes: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; no git commands.

Scope: (1) re-derive the act-4 **sit-2..4 formation-order arm**
`0x8AB08..0x8AB62` and its dispatcher routing, fresh; (2) derive its
**`FUN_0008C974`** gate producer end-to-end (the fresh full body is a
**sorter + residual counter**, not the pure compare FU-183 §4 described);
(3) audit the **`0x14C360` / `0x157BD2`** matrices' feeders (FU-183's "proven
no-op caveat") — the fresh writer census contradicts the "written only by the
placement phase handlers" attribution; (4) re-run the **act-producer census**
(`CALL 0x888FC` sites per act, fresh); (5) rank **action row 0x17**'s honest
reachability and give the T2/T4 wiring rec.

---

## 1. Fresh table anchors

`read_memory 0x1107EC` (64 B) — the act table (base = `0x110794 + 0x16*4`):

```
00000000 c88d0800 2c920800 a49f0800 20960800 ec900800 10910800
68980800 98a70800 4c8f0800 88b60800 74b80800 00b90800 ff076000 38ce0700 ff074000
```

`read_memory 0x110794` (140 B) — the phase table rows `0x17..0x22` are the act
slots, byte-identical to FU-175 §1 / FU-190 §1:

```
[0x17]=0x088DC8 [0x18]=0x08922C [0x19]=0x089FA4 [0x1A]=0x089620
[0x1B]=0x0890EC [0x1C]=0x089110 [0x1D]=0x089868 [0x1E]=0x08A798
[0x1F]=0x088F4C [0x20]=0x08B688 [0x21]=0x08B874 [0x22]=0x08B900
```

`read_memory 0x84720` (16 B) — the row-0x17 stage table:
`{0x00084780, 0x000847E5, 0x0008483F, 0x0008494C}`.

`read_memory 0x8A904` (52 B) — the `FUN_0008A938` normal table, 13 dwords:
`{0x8AB82, 0x8ABAB, 0x8ABDB, 0x8ABF3, 0x8ADC0, 0x8ADD8, 0x8AC28, 0x8ADF0,
0x8AE08, 0x8AEC6, 0x8AEDE, 0x8AEF6, 0x8AF0E}` (fresh; the FU-149 §1.6 table).

All four reads are byte-identical to the frozen docs; the act table is the
phase table addressed through `0x1107EC` (FU183-L7 overlay, re-confirmed).

---

## 2. The act-4 sit-2..4 arm `0x8AB08..0x8AB62` — fresh byte-level

`disassemble_bytes 0x8AAF0` (140 B; the arm plus the sit-1 tail and the shared
`0x8AB63` tail). The arm is entered from the `BX == 0` sit-range gate (§3):

```
0x8AB08 MOV EAX,0x1588A4              b8 a4 88 15 00   team-0 record base
0x8AB0D CALL 0x8C974                  e8 62 1e 00 00   formation-order scan/sort (team 0)
0x8AB12 MOV EAX,0x1590D9              b8 d9 90 15 00   team-1 record base (0x1588A4 + 0x835)
0x8AB17 CALL 0x8C974                  e8 58 1e 00 00   formation-order scan/sort (team 1)
0x8AB1C MOV EAX,[0x157A49]            a1 49 7a 15 00
0x8AB21 SAR EAX,0x18                  c1 f8 18         alias: byte [0x157A4C]
0x8AB24 CMP EAX,1                     83 f8 01
0x8AB27 JZ  0x8AB63                   74 3a            goal-side flag == 1 -> normal table
0x8AB29 CMP byte [0x157B8E],0         80 3d 8e 7b 15 00 00
0x8AB30 JNZ 0x8AB3B                   75 09
0x8AB32 CMP byte [0x157B8F],0         80 3d 8f 7b 15 00 00
0x8AB39 JZ  0x8AB63                   74 28            both counters clean -> normal table
0x8AB3B MOV EAX,[ESP]                 8b 04 24         side (the head's word [ESP])
0x8AB3E MOV EBX,1                     bb 01 00 00 00   invoke-now
0x8AB43 XOR EDX,EDX                   31 d2            stage 0
0x8AB45 MOV [0x158820],AX             66 a3 20 88 15 00   stored SIDE  (word)
0x8AB4B MOV EAX,4                     b8 04 00 00 00   act 4
0x8AB50 MOV word [0x15881E],CX        66 89 0d 1e 88 15 00   stored SITUATION (word)
0x8AB57 CALL 0x888FC                  e8 a0 dd ff ff   FUN_000888FC(4, 0, BX=1)
0x8AB5C ADD ESP,4 ; POP EBP/ESI/ECX ; RET
```

**Semantics (fresh, first-hand).** The arm first runs the two teams' formation
scans/sorts (`FUN_0008C974`, §4) which leave the per-team residual mismatch
counts in `[0x157B8E]`/`[0x157B8F]`. It then takes **act 4** only when the
goal-side flag `[0x157A4C] != 1` **and** (`[0x157B8E] != 0` **or**
`[0x157B8F] != 0`) — i.e. some team's line-up order could not be reconciled to
the desired formation order. On the taken path it stores the re-dispatch cells
`[0x158820] = side` (word, the head's `word[ESP]`) and `[0x15881E] = situation`
(word, `CX`) — **exactly the two cells act-4 stage 1 reads back** (`0x897AE
EDX=[0x15881E]`, `0x897B4 EAX=[0x15881C]; SAR ... 0x10` → word `[0x158820]` =
side, word `[0x15881E]` = situation, fresh §6) — and invokes
`FUN_000888FC(EAX=4, DL=0, BX=1)`. Act 4 = phase `0x1A`, handler `0x89620`
(§1). On the not-taken path (`JZ/JNZ` → `0x8AB63`) the normal table `0x8A904`
runs (sit 2 → phase 3 `0x8ABDB`; sit 3 → corner counter + phase 4 `0x8ABF3`;
sit 4 → phase 8 `0x8ADC0`).

The **sit-1 arm twin** is byte-identical in structure (§3): it too runs the two
`FUN_0008C974` sorts and the same `[0x157A4C]`/`[0x157B8E]`/`[0x157B8F]` gate,
but its taken path only stores the two cells and `JMP 0x8AB63` — it does **not**
dispatch act 4; it falls through to the normal sit-1 body `0x8ABAB` (act 1,
invoke-now). Only the sit-2..4 arm dispatches act 4.

---

## 3. The dispatcher head and routing — fresh

`disassemble_bytes 0x8A938` (120 B) + `0x8AA60` (150 B):

```
0x8A938 PUSH ECX/ESI/EBP ; SUB ESP,4
0x8A93E MOV ECX,EAX                  ; situation
0x8A940 MOV word [ESP],DX            ; side
0x8A944 TEST AX,AX / JZ 0x8AA7B      ; sit 0 -> direct
0x8A94E CMP EAX,0xB / JZ 0x8AA7B     ; sit 0xB -> direct
0x8A957 CMP byte [0x14C32A],0 / JZ 0x8AA7B
0x8A964 CMP dword [0x15B6C0],0 / JNZ 0x8AA7B
0x8A971 MOV ESI,[ESP] / TEST SI,SI / SETZ AL ; [0x15B6B8] = (side==0)
0x8A97F SUB ECX,2 ; 0x8A987 CMP CX,8 / JA 0x8AA60
0x8A996 JMP CS:[EAX*4 + 0x8A8E0]     ; table-1 queue
...
0x8AA7B TEST BX,BX
0x8AA7E JZ  0x8AAA8                  ; BX==0 -> the sit-range gate
0x8AA80 XOR EDX,EDX ; XOR EAX,EAX ; MOV EBX,1 ; CALL 0x740A0   ; phase 0, side 0
0x8AA8E MOV AL,[ESP] ; XOR EDX,EDX ; MOV [0x15882C],AL         ; stored side
0x8AA98 MOV EAX,8 ; 0x8AA9D MOV [0x15882B],CL                  ; stored situation
0x8AAA3 JMP 0x8AF1A                                            ; act 8
0x8AAA8 CMP CX,1 ; 0x8AAAC JC  0x8AB63   ; sit < 1 -> normal table
0x8AAB2 JBE 0x8AABF                       ; sit == 1 -> the sit-1 arm
0x8AAB4 CMP CX,4 ; 0x8AAB8 JBE 0x8AB08    ; sit 2..4 -> the formation-order arm
0x8AABA JMP 0x8AB63                       ; else normal table
```

The arm's entry gate `0x8AAA8..0x8AAB8` is fresh byte-for-byte the FU-183 §3
routing. The `BX != 0` fallback (`0x8AA80..0x8AAA3`) writes phase 0, the two
`[0x15882B]`/`[0x15882C]` act-8 replay bytes, then `EAX=8` → `JMP 0x8AF1A` (the
shared act invocation). The `0x8AB63` shared tail (`MOV AL,[ESP]; MOV
[0x157AAF],AL; CMP CX,0xC; JA 0x8AF1F; JMP CS:[EAX*4+0x8A904]`) is the normal
table jump.

---

## 4. `FUN_0008C974` — the formation-order sorter + residual counter (fresh)

`get_function_by_address 0x8C974` → body `0x8C974..0x8CBAD` (569 B).
`decompile_function 0x8C974` + two `disassemble_bytes` chunks give the full
body. This is **not** the pure miss-count scan FU-183 §4 described: it **sorts**
the team's records to the desired order and then stores the **residual**
mismatch count.

**4.1 Prologue + side index (`0x8C974..0x8C9A1`).**

```
0x8C974 PUSH EBX/ECX/EDX/ESI/EDI/EBP ; SUB ESP,0x28
0x8C97D MOV [ESP+0x14],EAX            ; param = the team record base
0x8C98B XOR EAX,EAX ; MOV AL,[EDX+0x826]   ; team side byte
0x8C993 CALL 0x741B4                  ; EAX = (side ^ swap) & 1
0x8C998 MOV EDI,EAX                   ; EDI = the side index
0x8C99A XOR EDX,EDX ; MOV word [ESP+0x20],DX   ; i = 0
0x8C9A1 JMP 0x8CB0C                   ; loop guard
```

`0x741B4` is the FU-149 §1.1 `(side ^ byte[0x157ABE]) & 1` display/score slot;
the side index `sVar17 = EDI` selects the `0x157BD2/0x157B90/0x157BA6` rows and
the `0x14C360/0x14C3A0/0x14C3B0` rows.

**4.2 The sort loop (`0x8C9A6..0x8CAFA`).** Guard `0x8CB0C`: read `word[ESP+0x20]`
(the alias: `dword[ESP+0x1e]>>16`), if `< 0xb` enter the body:

```
0x8C9A6 MOVSX EBX,DI                  ; side
0x8C9A9 IMUL EDX,EBX,0x7b             ; side*0x7b
0x8C9AC IMUL EBX,EBX,0x2c             ; side*0x2c
0x8C9AF LEA ECX,[EAX*4]               ; i*4
0x8C9B6 ADD EDX,ECX                   ; side*0x7b + i*4 (desired)
0x8C9B8 MOV EAX,[ECX+EBX+0x157bd2]    ; current[i] = [0x157BD2 + side*0x2c + i*4]
0x8C9BF MOV ECX,[EDX+0x14c360]        ; desired[i] = [0x14C360 + side*0x7b + i*4]
0x8C9C5 CMP EAX,ECX / JZ 0x8CB07      ; equal -> advance i
0x8C9CD.. MOV [ESP+0x18],ECX ; XOR EAX,ECX ; JMP 0x8CAFB   ; search j
0x8C9DD (search body) IMUL ESI,EBX,0x2c ; ... ; MOV EBP,[ECX+0x157bd2]
0x8C9EF CMP EBP,[ESP+0x18] / JNZ ...  ; find j with current[j] == desired[i]
```

On a hit, `0x8C9F9..0x8CAD8` swaps records `i` and `j`: it exchanges the
`0x157BD2` order dwords, the `0x157B90` bytes and the `0x157BA6` words
(`0x8CA4F/0x8CA98` write `[..+0x157bd2]`; `0x8CA5D/0x8CAA2` the `0x157B90`
bytes; `0x8CA82/0x8CAB7` the `0x157BA6` words) and re-links the entity list via
two `CALL 0x8C758` (`0x8CACC`/`0x8CAD3`), then splices the neighbours
(`0x8CADB..0x8CAEF`), sets `bVar5 = 1` (`0x8CAE3 MOV EBP,1; 0x8CAEB MOV
[ESP+0x10],EBP`), resets `i = 0` (`0x8CAF3 MOV word [ESP+0x20],DX`) and
restarts the scan (`0x8CAF8 JMP 0x8CB0C`). This is a selection sort.

**4.3 The final residual count (`0x8CB1C..0x8CB5E`).** When `i` reaches `0xb`:

```
0x8CB1C CMP dword [ESP+0x10],0 / JZ 0x8CB28
0x8CB23 CALL 0x4728C                  ; a swap happened -> the placement-commit sink
0x8CB28 XOR EAX,EAX ; XOR EDX,EDX ; JMP 0x8CB4D
0x8CB2E (count loop) MOVSX ECX,DI ; IMUL ESI,ECX,0x7b ; IMUL ECX,ECX,0x2c
0x8CB3C MOV EBX,[ECX+EBX+0x157bd2]    ; current[edx]
0x8CB43 CMP EBX,[ESI+0x14c360]        ; vs desired[edx]
0x8CB49 JZ 0x8CB4C ; 0x8CB4B INC EAX  ; count++
0x8CB4D MOVSX EBX,DX ; CMP EBX,0xb / JL 0x8CB2E   ; edx 0..10
0x8CB58 MOV ESI,[0x157A83]            ; tracked record
0x8CB5E MOV [EDX+0x157b8e],AL         ; store the residual count
```

**4.4 Tail (`0x8CB64..0x8CBAD`).** If `[0x157A83]` (the tracked record) is
non-null and `[rec+0x9A] != 0`, it clears `[0x157A83]` (`0x8CB71/0x8CB73`);
then if `[team+0x7B2]` (a record pointer) is non-null and `[rec+0x9A] != 0` it
re-resolves it via `FUN_0008DDE0(team, -1)` (`0x8CB90..0x8CB9E`).

**Derived semantics.** `FUN_0008C974(team_base)` reads the team's side byte,
computes the matrix side index, **reorders** the team's records so the order
array `0x157BD2` matches the desired formation order `0x14C360` (swapping the
`0x157BD2` order dwords, the `0x157B90` bytes, the `0x157BA6` words, re-linking
via `0x8C758`), then stores the **post-sort residual** mismatch count into
`[0x157B8E + side]`. The residual is 0 for a valid permutation and nonzero only
when a desired entry cannot be matched into the current order.

> **Erratum (quoted).** FU-183 §4 says: *"`FUN_0008C974(team_base)` scans the
> team's 11 records comparing the current order (`0x157BD2 + …`) against the
> desired line-up (`0x14C360 + …`, `0x7B` stride), counts the mismatches in
> `EAX`, and stores that count into `[0x157B8E + team_side]`"* — and FU-183 §4's
> `0x8CB43` quote is the **final count block only**. Fresh `decompile_function`
> `0x8C974` + the full body show the routine also **sorts** `0x157BD2` (with
> the `0x8C758` re-link), calls the `0x4728C` placement-commit sink on any swap,
> and maintains `[0x157A83]`/`[team+0x7B2]`. The count stored is the residual
> *after* the sort. The FU-183 §4 conclusion (the arm gate = "a line-up mismatch
> that has not yet been walked off") stands; the attribution "pure scan/count"
> is corrected.

`get_xrefs_to 0x8C974` (fresh) = **6**: `0x8AAC4`/`0x8AACE` (the sit-1 arm) and
`0x8AB0D`/`0x8AB17` (the sit-2..4 arm) in `FUN_0008A938`, plus `0x9344A`/
`0x93454` in `FUN_0009343C` (byte-identical to FU-183 §4).

---

## 5. The `0x14C360` / `0x157BD2` matrices and their feeders (fresh census)

The arm's counter is fed by the desired-order matrix `0x14C360` (with
`0x14C3A0`/`0x14C3B0` companions) and the current-order matrix `0x157BD2` (with
`0x157B90`/`0x157BA6`). Fresh `search_instructions operand 0x14C360` = **61**
sites (read + write); `operand 0x157BD2` = **30**. The **write** sites:

**5.1 `0x14C360` writers (fresh).**

| site | container | role |
|---|---|---|
| `0x117F0` | `FUN_00011620` | **formation/line-up setup** (builds the per-slot pointers into `0x1430b2`) |
| `0x11A1E` | `FUN_00011978` | formation setup |
| `0x121CD` / `0x121D7` | `FUN_00012010` | **line-up sort** (goalkeeper/position ordering; writes `0x14C360` by swap) |
| `0x41B5F` / `0x41B91` | `FUN_00041494` | match-engine line-up routine |
| `0x82233` / `0x82268` | `0x82xxx` block | line-up reorder (swaps `0x14C360`/`0x14C3A0`/`0x14C3B0`) |
| `0x896D7` | act-4 stage 0 `0x89620` | **accepts** current order when the record is occupied (`[rec+0x9a] != 0`) |
| `0x8CDD6` / `0x8CE11` | `FUN_0008CBB0` | order routine (swaps with `0x157BD2`) |
| `0x8EB28` | `FUN_0008EAB8` | order routine |
| `0x934F5` | `FUN_0009343C` | act-4 stage-0 factored twin (FU-183 §4) |

The **address-0x11xxx/0x12xxx/0x41xxx/0x82xxx** writers are **formation/line-up
setup** and match-engine order routines; `FUN_00012010`'s callers (fresh
`get_xrefs_to` = **12**) are `FUN_00018680`, `FUN_00018108`, `FUN_00024b00`,
`FUN_000302dc`, `FUN_00032520`, `FUN_00029f68` — **menu/formation-selection**
functions (`0x18xxx`/`0x24xxx`/`0x29xxx`/`0x30xxx`/`0x32xxx`), **not** the match
loop. `FUN_00011620`'s callers (fresh = **5**) are `FUN_00011b7c`,
`FUN_00024b00`.

**5.2 `0x157BD2` writers (fresh).** `0x73F80`-block `FUN_00073EE0` (match
reset), `0x84892` (row-0x17 stage-2 formation sync), the `FUN_0008C974` swap,
`FUN_0008CBB0`, `FUN_0008EAB8`, `FUN_0009343C` (each also writing its
`0x14C360` twin, above).

**5.3 The reset copy (`FUN_00073EE0`, fresh `0x73FC4..0x74018`).** For each
side (`EDX` 0..1) and record (`ESI` 0..10) the reset copies the desired
formation into the current order:

```
0x73FD0 MOV CL,[EBX+EAX+0x14c3a0]        ; desired byte
0x73FD7 MOV [EDI+EAX+0x157b90],CL        ; current byte
0x73FF0 MOV ECX,[EBP+0x14c360]           ; desired pointer
0x73FF8 MOV [EDI+0x157bd2],ECX           ; current pointer
0x74000 MOV AX,[EBX+EAX+0x14c3b0]        ; desired word
0x74009 MOV [EDX+0x157ba6],AX            ; current word
```

and zeroes `[0x157B8E]`/`[0x157B8F]` (`0x73FB8 MOV [0x157B8F],CH` with `CH=0`;
the `0x157B8E` write is at `0x73F76`, fresh `get_xrefs_to 0x157B8E` = WRITE
`0x73F76`). So after a reset `current == desired` and the residual count is 0.

**5.4 Verdict — the feeders are *not* the placement phase handlers.**

> **Erratum (quoted).** FU-181 §9 item 3 says the two matrices *"are written
> only by the unported placement phase handlers"* (phases `0x00..0x15`), and
> FU-183 §7 / FU183-L1 repeat *"those matrices are written only by the placement
> phase handlers (phases 0x00..0x15, all NULL)"*. Fresh `search_instructions
> operand 0x14C360` (61 sites) + `0x157BD2` (30 sites) show **no** writer inside
> the placement phase handlers `0x6Dxxx`/`0x6Exxx`: the `0x14C360` desired
> order is produced by **formation/line-up setup** (`FUN_00011620`,
> `FUN_00011978`, `FUN_00012010` — menu callers) and by match-engine **order
> routines** (`FUN_0008CBB0`, `FUN_0008EAB8`, `FUN_0009343C`, `FUN_00041494`,
> the `0x82xxx` swap, act-4 stage 0), with the reset `FUN_00073EE0` copying it
> into `0x157BD2`. The no-**engine**-producer conclusion holds (the engine
> models neither matrix — `match_run_ref_view` stages `duel_ok = 1`,
> `fifa96_match_run.c:5191..5206`), but the *attribution* is corrected: the
> matrices' native producers are the formation setup + the order routines, not
> the placement family.

---

## 6. The row-0x17 producer — act-4 stage 0 `0x89655..0x89756` (fresh)

The only install of row 0x17 is act-4 stage 0 (`0x89740`). Fresh
`disassemble_bytes 0x89655` (250 B) + `0x89750` (80 B) + `0x897AE` (64 B):

```
0x89655 CMP byte [0x157B8E],0 ; SETZ AL ; ... ; [ESP]=EAX   ; side select = (157B8E==0)
0x8966A MOV DL,[EAX+0x157b8e] ; DEC DL ; MOV [EAX+0x157b8e],DL ; XOR ESI,ESI
0x8967F (loop i=0..10) ... MOV EAX,[ECX+0x157bd2] / CMP EAX,[EDI+0x14c360] / JZ advance
0x896C8 CMP byte [EBP+0x9a],0 / JZ 0x896E2
0x896D1 MOV EAX,[ECX+0x157bd2] ; 0x896D7 MOV [EDI+0x14c360],EAX  ; occupied -> accept
0x896DD JMP 0x897AE                                            ; -> terminal re-dispatch
0x896E2 MOV EBX,8 ; MOV EAX,[EBP+4] ; EDX=EBP ; [0x1587f4]=EAX ; [0x1587ec]=EBP
0x896FC CALL 0x8F188  (event 0x76)
0x8970C MOV EAX,0xB ; CALL 0x4C374
0x89716 MOV EDX,[EBP] ; MOV DL,[EDX+0x826] ; ... ; MOV ECX,1
0x8972F CALL 0x740A0                       ; phase-0xB write
0x89734 XOR EBX,EBX                        ; staged
0x89736 MOV EDX,0x17 ; 0x8973B MOV EAX,[0x1587ec]
0x89740 CALL 0x7D9A4                        ; install action 0x17 (ECX=1 -> invoke-now)
0x89745 XOR EBX,EBX ; 0x89747 MOV word [0x158818],BX ; 0x8974E INC byte [0x158829]
0x89756 INC ESI ; loop until ESI==0xB
0x89763 MOVSX EAX,SI ; CMP EAX,0xB ; JZ 0x897AE ; else return
0x897AE MOV EDX,[0x15881e] ; MOV EAX,[0x15881c] ; ... SAR ..,0x10
0x897D8 MOV EBX,1 ; 0x897DD MOV [0x158808],0 ; 0x897E3 CALL 0x8A938
```

`0x89740` is `ECX=1` (invoke-now; FU-181 §3 fresh re-confirms), so the install
runs row 0x17 in the installing frame (the FU181-L3 double dispatch). The
terminal re-dispatch `0x897AE` reads the `[0x15881E]`/`[0x158820]` words the
**arm** set (§2) and clears the act slot before `CALL 0x8A938`. Fresh
`get_xrefs_to 0x8C974`, `0x888FC` confirm the call graph.

---

## 7. The act-producer census (fresh `get_xrefs_to 0x888FC` = 11)

Fresh `get_xrefs_to 0x888FC` = **11** unconditional calls (byte-for-byte the
FU-175 §1 / FU-183 §1 set). Fresh `disassemble_bytes` of each site gives the act
id in `EAX` (the Watcom contract: **AX = act id**, **DL = stage**, **BX =
invoke-now**):

| site | container | EAX (act) | EDX (stage) | EBX (invoke) | role (fresh bytes) |
|---|---|---|---|---|---|
| `0x8AB57` | the sit-2..4 arm (§2) | **4** | 0 | 1 | formation-order line-up |
| `0x88EE8` | act-1 body `0x88DC8` stage 1 | **4** | 0 | 1 | kickoff → line-up hand-off |
| `0x8AB8E` | sit-0 body `0x8AB82` | **0xA** | 0 | 1 | setup (then phase 0x11) |
| `0x8ABCF` | sit-1 body `0x8ABAB` | **1** | 0 | 1 | kickoff |
| `0x8AEBA` | sit-8 body `0x8AEAE` | **7** | 0 | 1 | match re-init |
| `0x8AED2` | sit-9 body `0x8AEC6` | **2** | 0 | 1 | free kick |
| `0x8AEEA` | sit-0xA body `0x8AEDE` | **2** | 1 | 1 | penalty |
| `0x8AF1A` | shared: sit-0/0xB BX≠0 fallback `0x8AAA3` | **8** | 0 | 1 | mid-restart hold |
| `0x8AF1A` | shared: sit-0xC tail `0x8AF0E` | **9** | 0 | 1 | restart chain (FU-165 §3.1) |
| `0x8A6FE` | `FUN_0008A43C` | **3** | 0 | 1 | foul award (card) |
| `0x8A789` | `FUN_0008A43C` | **6** | 0 | 1 | foul award (collision) |
| `0x8BAE7` | `FUN_0008B9CC` (shared) | **0xC** / **0xB** | 0 | 1 | period end (extra-time set/clear) |

Fresh confirming bytes: `0x8A784 MOV EAX,6` (act-6 foul);
`0x8ABD4 MOV EAX,0xC` / `0x8ABDB MOV EAX,0xB` (the shared `0x8BAE5 XOR
EDX,EDX` / `0x8BAE7 CALL 0x888FC`); `0x8ABC7 MOV EAX,EBX` with `EBX=1` (act-1
sit body); `0x8AB87 MOV EAX,0xA` (sit-0). The statically-invoked act set is
therefore **{1, 2, 3, 4, 6, 7, 8, 9, 0xA, 0xB, 0xC}** — **no `CALL 0x888FC`
passes `EAX=5`**, and act 0 selects the NULL slot. (Fresh; agrees with FU-175
§4 / FU-183 §1.)

**Producer-per-act summary (fresh):** act 1 → 1 site (`0x8ABCF`); act 2 → 2
(`0x8AED2`, `0x8AEEA`); act 3 → 1 (`0x8A6FE`); act 4 → **2** (`0x8AB57` the arm
+ `0x88EE8` the act-1 hand-off); act 5 → **0**; act 6 → 1 (`0x8A789`); act 7 →
1 (`0x8AEBA`); act 8 → 1 (`0x8AF1A` via `0x8AAA3`); act 9 → 1 (`0x8AF1A` via
`0x8AF0E`); act 0xA → 1 (`0x8AB8E`); act 0xB → 1 (`0x8BAE7`); act 0xC → 1
(`0x8BAE7`).

---

## 8. Row 0x17 — honest reachability ranking

The full producer chain (all links fresh this slice):

```
sit 2/3/4 direct arm  0x8AAA8..0x8AB08   [committed in the engine? NO — FU-149 L12]
  -> the two FUN_0008C974 sorts (§4)      [unported; needs the 0x14C360/0x157BD2 matrices]
    -> gate [0x157A4C]!=1 && ([0x157B8E]|[0x157B8F])  [counters unported]
      -> FUN_000888FC(4,0,1) = act 4 / phase 0x1A      [act body UNPORTED, fn==NULL]
        -> act-4 stage 0 0x89655..0x89756               [unported]
          -> form diff 0x89697 vs 0x8969D               [matrices unported]
            -> 0x8972F phase-0xB write                  [phase-0xB arm unported]
            -> 0x89740 install action 0x17 (ECX=1)      [row body PORTED, table slot NULL]
              -> CALL [rec+0x18] = row 0x17 runs        [never reached]
```

**Verdict: row 0x17 has no live engine producer.** Three independent blockers,
all fresh-confirmed this slice:

1. **The arm is unported in the engine.** `fifa96_action_phase_situations`
   (`src/fifa96_loader/fifa96_action_handlers.c:1129..1147`) maps situations
   2/3/4 with `act = FIFA96_ACTION_PHASE_NONE` and the engine's
   `fifa96_match_run_set_piece` BX==0 branch explicitly documents the arm as
   unported table-2 machinery (`fifa96_match_run.c:2823..2829`). The engine
   routes the table-2 phase write instead.
2. **The `0x14C360`/`0x157BD2` matrices have no engine producer.** The engine
   models neither array; the only stand-in is `duel_ok = 1`
   (`fifa96_match_run.c:5191..5206`). With both matrices absent/zero,
   `FUN_0008C974` would sort equal arrays and store a residual of 0, so the arm
   gate never trips — the FU-183 "proven no-op" (§7) holds, now with the feeder
   attribution corrected (§5.4).
3. **The install is invoke-now (`0x89740`, `ECX=1`), and the engine has no
   generic invoke-now port** — only the outfield-machine double dispatch
   (FU181-L3). A faithful act-4 port must call
   `fifa96_match_entities_install(rec, 0x1A, 0x17, 0)` followed by
   `fifa96_match_dispatch_action(mr, 0x17)` inline.

Consequently `fifa96_match_action_table[0x17]` stays **NULL** (`{0x17, NULL,
...}` — `src/fifa96_engine/fifa96_match_handlers.c:4592..4593`) and row 0x17 is
**not counted** (the rows-27/29/2C precedent). This matches FU-183 §10 Ruling W2
(no arm code was written; the row stays NULL and uncounted).

---

## 9. Engine wiring rec (T2/T4)

**T2 — the producer machine (the sit-2..4 arm + the matrices):**

1. **Add the formation-order state.** A per-team desired line-up
   (`mr->formation_order[2][11]`, mirroring `0x14C360 + side*0x7B + i*4`) seeded
   by the existing `match_run_formation_seed`/`fifa96_match_entities_seed_formation`
   path, and a per-team current order (`mr->formation_current[2][11]`, `0x157BD2`)
   initialized from the desired order in the match reset (the `FUN_00073EE0`
   copy, §5.3). Add the residual counters `mr->formation_mismatch[2]`
   (`0x157B8E`/`0x157B8F`).
2. **Port `FUN_0008C974`** as the sorter + residual counter (§4): the
   side index via the `side_swap` (`0x741B4`), the selection sort of the
   current order against the desired order, the residual count store, and the
   `0x4728C` placement-commit sink on any swap (the same sink row-0x17 stage 3
   uses). Without (1) this is a no-op — do not fake the matrices.
3. **The arm** in `fifa96_match_run_set_piece`'s BX==0 branch
   (`fifa96_match_run.c:2823`): when `situation ∈ {2,3,4}` and
   `mr->global_157a4c != 1` and `formation_mismatch[0] || formation_mismatch[1]`
   (after the two sorts), write `mr->store_158820 = side`,
   `mr->store_15881e = situation`, and
   `fifa96_match_act_dispatch(mr, 4, 0, 1)`. The not-taken branch keeps the
   existing table-2 route.
4. **Act-4 stage 0** (`fifa96_match_phase_table[0x1A]`): the formation diff,
   the phase-0xB write (`match_run_write_phase(0xB)` + `match_run_phase_arm(0xB)`,
   new to the arm list), and the **invoke-now double dispatch** of row 0x17
   (§8 blocker 3); stage 1's terminal `word[0x15881A] >= 0xF0` re-dispatch of
   the stored situation/side.

**T4 — the action row:** row 0x17's body (`fifa96_match_action_17`) is already
ported (FU-181 §2). Wire `fifa96_match_action_table[0x17]` to it **only after**
(T2's) arm + matrices give it a live installer; then the census honestly
`+1` (row 0x17 the first action row whose live producer is the act seam).

**Do not** wire row 0x17 standalone: without the arm + the matrices it is a
count with no live effect (FU181-L1).

---

## 10. Numbered legs

1. **FU197-L1 — the `FUN_0008C974` residual is a permutation residual.**
   The counter is stored only *after* the selection sort; a valid permutation of
   the current order against the desired order yields 0. The exact dynamic
   condition that leaves a nonzero residual (a desired pointer absent from the
   current order, or a duplicate) is not statically pinned here — it depends on
   the runtime writers of `0x14C360`/`0x157BD2` (the formation/order routines,
   §5) interacting during a match. Settle by a dynamic trace of the two arrays
   across a substitution/line-up change.
2. **FU197-L2 — `FUN_0008CBB0` / `FUN_0008EAB8` / `FUN_0009343C` semantics.**
   These three order routines write both matrices (§5.1/§5.2); their exact
   operations (swap, insert, position assignment) are cited by address only.
   `FUN_0009343C` is the FU-183 §4 "factored twin" of the act-4 stage-0 loop;
   `FUN_0008CBB0` (`0x8CBB0..0x8CE77`) and `FUN_0008EAB8` (`0x8EAB8..0x8EB6F`)
   are not decomposed.
3. **FU197-L3 — the `0x82xxx` line-up reorder block.** The writer at `0x82233`/
   `0x82268` sits in a function Ghidra does not define (no `get_function_by_address`
   hit at `0x82100`/`0x82200`); its container and role (substitution? formation
   swap?) are unresolved.
4. **FU197-L4 — the formation-setup `0x14C360` seed.** `FUN_00011620`,
   `FUN_00011978`, `FUN_00012010` build the desired line-up; whether the engine's
   `match_run_formation_seed` produces an array equivalent to the native
   `0x14C360` (pointer identity, slot order, the `0x14C3A0`/`0x14C3B0`
   companions) is unverified — the reason `duel_ok` is a stand-in.
5. **FU197-L5 — the phase-0xB arm `0x8D040[0xB]=0x8D63E`.** Carried from
   FU181-L2: the install-0-over-0..10 arm is fresh but its arg semantics are
   *cross-cited* FU-137 §5.1, not re-derived; the engine's `match_run_phase_arm`
   covers only 3/4/6/7/8/9/0xD.
6. **FU197-L6 — invoke-now has no generic engine port.** Carried from FU181-L3/
   FU183-L3: the act-4 `0x89740` `ECX=1` install runs the row in the installing
   frame; only the outfield-machine double dispatch models it.
7. **FU197-L7 — the sit-2..4 arm's stored cells.** `[0x15881E]`/`[0x158820]`
   (word) set at `0x8AB50`/`0x8AB45` are read by act-4 stage 1 (`0x897AE`/
   `0x897B4`). The engine fields `store_15881e`/`store_158820` exist; verify they
   are not clobbered between the arm and the re-dispatch when the arm lands
   (FU183-L8 carried).

---

## 11. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` (FIFA96.EXE).
* `read_memory` — `0x1107EC` (64 B), `0x110794` (140 B), `0x84720` (16 B),
  `0x8A904` (52 B).
* `disassemble_bytes` — `0x8AAF0` (140 B), `0x8A938` (120 B), `0x8C974` (200 B),
  `0x8CA3B` (160 B), `0x8CADA` (214 B), `0x89655` (250 B), `0x8AA60` (150 B),
  `0x73F80` (120 B), `0x88EB8` (56 B), `0x8AE80` (120 B), `0x8A6E0` (40 B),
  `0x8A770` (40 B), `0x8BAC0` (56 B), `0x89750` (80 B), `0x8AB80` (50 B),
  `0x897AE` (64 B), `0x8AF00` (40 B), `0x73FF6` (48 B), `0x8ABB2` (32 B),
  `0x8A720` (32 B), `0x821F0` (120 B).
* `decompile_function` — `0x8C974`, `0x11620`, `0x12010`.
* `get_function_by_address` — `0x8C974` (`0x8C974..0x8CBAD`), `0x11620`
  (`0x11620..0x11976`), `0x41494` (`0x41494..0x41C92`), `0x9343C`
  (`0x9343C..0x9359E`), `0x8CBB0` (`0x8CBB0..0x8CE77`), `0x8EAB8`
  (`0x8EAB8..0x8EB6F`).
* `get_xrefs_to` — `0x888FC` (11), `0x8C974` (6), `0x12010` (12), `0x11620` (5),
  `0x8CBB0` (4), `0x9343C` (6), `0x157B8E` (11).
* `search_instructions` — operand `0x14C360` (61), `0x157BD2` (30), `0x157B8E`
  (5).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(2580..2659, 2755..2844, 5185..5214), `src/fifa96_loader/
fifa96_action_handlers.c` (1100..1229), `src/fifa96_engine/
fifa96_match_handlers.c` (grep `fifa96_match_action_17` / `[0x17, NULL`).

Docs cross-cited: FU-137 §2/§5.1, FU-143 §2.1, FU-149 §1.1/§1.6/L12, FU-165
§3, FU-175 §1/§3/§4/§7, FU-181 §1..§9, FU-183 §1..§10, FU-190 §4. AGENTS.md
census rule. The `+0x100000` LE-image rule and the alias traps
(`[0x157A49]>>24` = `[0x157A4C]`; `dword[ESP+0x1e]>>16` = `[ESP+0x20]`) are
applied. No writes beyond this slice file and the workspace report.
