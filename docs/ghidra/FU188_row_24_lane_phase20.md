# FU-188 — row 0x24 (lane/anim) + the phase-0x20 / act-0xA handler

Provenance: M2 phase-16 wave **R36**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; this session `get_current_program_info` = `watcom:LE:32:default`,
`min_address` `0x10000`). Every address/byte below is a fresh `read_memory` /
`disassemble_bytes` / `disassemble_function` / `get_xrefs_to` /
`get_function_by_address` of **this** session unless marked *cross-cited* (then
the frozen FU doc is named). LE-image globals sit at `+0x100000` from the file
offset; the addresses quoted here are `/FIFA96.EXE` program (runtime) addresses,
not file offsets. The alias traps are applied (`dword[x]>>16` = the word at
`x+2`, `>>24` = the byte at `x+3`). No writes: no rename, comment, label,
function, script or project save; `/FIFA96.EXE` untouched; no git commands.

Scope: (1) action row **0x24** byte-level (`0x86510..0x866F1`); (2) the
**phase-0x20 / act-0xA handler** `0x8B688` (the timeline that installs code
0x24), re-read in one ~500 B window; (3) the install path — the two 11-record
loops at `0x8B747` (`ESI=0x1588A4`) and `0x8B782` (`ESI=0x1590D9`); (4) the
**act path that reaches phase 0x20 through T1's seam** (native `FUN_0008A938`
sit 0 → act 0xA → the `0x1107EC[0xA]` slot); (5) a **T4 wiring rec**; (6) the
remainder as numbered legs.

---

## 1. Fresh table anchors

`read_memory 0x1106E0` (180 B = 45 dwords), offset `0x24*4 = 0x90`: row
**`[0x24] = 0x00086510`** (`bytes 10 65 08 00`). The action table is contiguous
to `0x110794` (FU-174 §7 note 3 / FU-182 §1). Fresh `get_xrefs_to 0x86510` =
**1**, `0x110770` = `0x1106E0 + 0x24*4` — the row's only static dispatch is the
action-table slot (no direct call site).

`read_memory 0x110794` (140 B = 35 dwords), offset `0x20*4 = 0x80`: phase row
**`[0x20] = 0x0008B688`** (`bytes 88 b6 08 00`). Fresh `get_xrefs_to 0x8B688` =
**1**, `0x110814` = `0x110794 + 0x20*4`.

The act base is `0x1107EC = 0x110794 + 0x16*4`; `0x1107EC[0xA] = 0x110794 +
0x20*4 = 0x110814`, i.e. act 0xA and phase 0x20 are the **same slot** and the
**same handler `0x8B688`** — the FU165-L9 dual act/phase identity, re-confirmed
first-hand (one data xref, the shared `0x110814`). `0x1107EC[0xA] = 0x8B688`
(the read at FU-175 §1 table row `A` = `0x0008B688`).

Record blocks (fresh first-hand, cross-cited FU-67 §3 / FU-175 §1):
`[0x1588A4]` = team side 0, `[0x1590D9]` = team side 1, stride `0x835`; each
holds **11** records at stride **`0xB2`**. (`0x1590D9 − 0x1588A4 = 0x835`;
`11 × 0xB2 = 0x7A6`, so block 0 is `0x1588A4..0x15904A`.)

---

## 2. The phase-0x20 / act-0xA handler `0x8B688` (fresh ~500 B window)

One `disassemble_bytes 0x8B688` (500 B, `0x8B688..0x8B87B`) — the whole
handler is `0x8B688..0x8B872` (`RET`), then `0x8B873 NOP`, then row 0x21
`0x8B874`. Prologue `0x8B688: PUSH EBX/ECX/EDX/ESI/EDI/EBP; SUB ESP,0x8`.

**Head (the common timer)** `0x8B691..0x8B6A5`:
```
0x8B691 MOV AX, [0x157A64]              ; the frame delta
0x8B697 MOV DX, word [0x158818]         ; the act timer
0x8B69E ADD EDX, EAX
0x8B6A0 MOV AL, [0x158829]              ; the act stage byte
0x8B6A5 MOV word [0x158818], DX         ; store the timer
```
**Stage dispatch** `0x8B6AC..0x8B6BE`:
```
0x8B6AC CMP AL, 1
0x8B6AE JC  0x8B6C3                     ; AL == 0 -> stage 0
0x8B6B0 JBE 0x8B7CB                     ; AL == 1 -> stage 1
0x8B6B6 CMP AL, 2
0x8B6B8 JZ  0x8B7F6                     ; AL == 2 -> stage 2
0x8B6BE JMP 0x8B869                     ; else epilogue
```

**Stage 0** (`0x8B6C3`): `0x8B6C3 TEST AL,AL; 0x8B6C5 JNZ 0x8B869` (only
`AL == 0` runs). Then:
```
0x8B6CB CALL 0x73E28                    ; phase reset (FU-175/181 sink)
0x8B6D0 MOV EBX, 0x960
0x8B6D5 MOV ESI, 0x1E0
0x8B6DA MOV EBP, 0x3
0x8B6DF MOV EDX, [0x10F32C]             ; camera reset triple
0x8B6E9 XOR EAX, EAX / ECX,ECX / EDI,EDI
0x8B6EB MOV dword [0x15880C], EBX       ; = 0x960
0x8B6F1 MOV dword [0x158814], ECX       ; = 0
0x8B6F7 MOV dword [0x158830], ESI       ; = 0x1E0
0x8B6FD MOV dword [0x158838], ECX       ; = 0
0x8B703 MOV [0x15884A], AX              ; word = 0
0x8B709 MOV dword [0x158852], EBP       ; = 3
0x8B721 MOV word [0x15884C], DI         ; = 0
0x8B728 MOV word [0x158848], SI         ; = 0
0x8B737 CALL 0x700F4                    ; camera reset([0x10F328/2C/30], 0)
```
then **the two install loops** (§3), then:
```
0x8B7AA MOV CH, [0x158829]
0x8B7B2 INC CH
0x8B7B4 MOV word [0x158818], DX(0)      ; timer := 0
0x8B7BB MOV byte [0x158829], CH         ; stage 0 -> 1
0x8B7C1.. epilogue
```
So stage 0 seeds the `0x15880C/0x158814/0x158830/0x158838/0x15884A/
0x158848/0x15884C/0x158852` timeline cells, resets the camera, **installs code
0x24 invoke-now on all 22 records**, resets the timer and advances to stage 1.

**Stage 1** (`0x8B7CB`):
```
0x8B7CB MOV EAX, 0x3E8
0x8B7D0 CALL 0x974D8                    ; a 0x3E8-tick wait (sink)
0x8B7D5 MOV DH, [0x158829] ; INC DH ; MOV word [0x158818], CX(0) ; MOV [0x158829], DH
```
i.e. wait then stage 1 → 2.

**Stage 2 (terminal)** (`0x8B7F6`):
```
0x8B7F6 MOV EAX, [0x158816]
0x8B7FB SAR EAX, 0x10                   ; alias: word [0x15881A]
0x8B7FE CMP EAX, 0x3C
0x8B801 JL  0x8B869                     ; wait < 0x3C
0x8B803 CMP EAX, 0x168
0x8B808 JGE 0x8B813
0x8B80A CALL 0x45001 ; TEST AL,AL ; JZ 0x8B869   ; the shape gate
0x8B813 XOR EBX,EBX / AH,AH / EDX,EDX
0x8B819 MOV word [0x158818], BX(0)      ; timer := 0
0x8B820 MOV byte [0x158828], AH(0)      ; act id   := 0
0x8B826 MOV dword [0x158808], EDX(0)    ; act handler := 0 (the slot clears)
0x8B82C MOV byte [0x158829], AH(0)      ; stage := 0
0x8B832 CALL 0x7417C
0x8B837 CALL 0x92AC8 ; AND AL,1
0x8B840 MOV [0x158825], AL
0x8B845 MOV [0x157AAE], AL
0x8B84A MOV [0x15882C], AL
0x8B84F MOV EDX, [0x157AAB] ; SAR EDX,0x18   ; the stored side
0x8B855 MOV EAX, 1
0x8B85D CALL 0x8A938                     ; re-dispatch situation 1, BX=0
0x8B862 XOR EAX, EAX
0x8B864 CALL 0x36200
```
So the terminal waits `word[0x15881A] >= 0x3C` (alias trap), applies the
`0x45001` gate while `< 0x168`, then **clears the act slot** (`[0x158808/
0x158828/0x158818/0x158829] = 0`), stores the `0x92AC8`-derived bit into
`0x158825/0x157AAE/0x15882C`, and re-dispatches **situation 1** with `BX=0`
(the same shape as act 8's terminal, FU-181 §4).

---

## 3. The code-0x24 install path (both loops)

Interleaved between the stage-0 init and the stage-0 tail:

**Loop 1** (team side 0):
```
0x8B730 MOV ESI, 0x1588A4              ; team-0 record base
0x8B735 XOR BL, BL
0x8B73C MOV byte [ESP+4], BL           ; counter := 0
0x8B740 JMP 0x8B765
0x8B742 MOV ECX, 1                     ; ECX = 1  -> invoke-now
0x8B747 MOV EDX, 0x24                  ; DX  = code 0x24
0x8B74C MOV EAX, ESI                   ; EAX = record
0x8B74E XOR EBX, EBX                   ; BL  = 0  -> staged byte
0x8B750 CALL 0x7D9A4                   ; the installer
0x8B755 MOV CL, [ESP+4] ; INC CL ; ADD ESI, 0xB2 ; MOV [ESP+4], CL
0x8B765 MOV EAX, [ESP+1] ; SAR EAX,0x18 ; CMP EAX,0xB ; JL 0x8B742   ; counter < 11, alias -> byte [ESP+4]
```
**Loop 2** (team side 1):
```
0x8B771 XOR BH, BH
0x8B773 MOV ESI, 0x1590D9              ; team-1 record base
0x8B778 MOV byte [ESP], BH             ; counter := 0
0x8B77B JMP 0x8B79E
0x8B77D MOV ECX, 1
0x8B782 MOV EDX, 0x24
0x8B787 MOV EAX, ESI
0x8B789 XOR EBX, EBX
0x8B78B CALL 0x7D9A4
0x8B790 MOV AL, [ESP] ; INC AL ; ADD ESI, 0xB2 ; MOV [ESP], AL
0x8B79E MOV EAX, [ESP-3] ; SAR EAX,0x18 ; CMP EAX,0xB ; JL 0x8B77D   ; alias -> byte [ESP]
```
Both loops install `EAX = record`, `EDX = 0x24`, `ECX = 1`, `EBX = 0` over 11
records at stride `0xB2` — **22 records total** (both teams).

**Installer contract (fresh `disassemble_function 0x7D9A4`, 73 insns).**
`0x7D9A9 MOV ESI,EAX; 0x7D9AB MOV word[ESP+4],DX; 0x7D9B0 MOV word[ESP],BX;
0x7D9B4 MOV EDI,ECX`. Guards: `0x7D9BE CMP byte[ESI+0x9A],0; JNZ 0x7DAAD`
(skip an occupied record), `0x7D9D3 CMP AX,DX; JZ 0x7DAAD` (no-op if the record's
code `[ESI+0x91]` already equals the new code). It writes `[ESI+0x91]=code`
(`0x7DA67`), `[ESI+0x18] = action_table[code]` (`0x7DA74 SHL EAX,2; ADD
EAX,0x1106E0; MOV EAX,[EAX]; MOV [ESI+0x18],EAX`), resets `[ESI+0x89]=0`/
`[ESI+0x9E]=0`, stores the stage byte `[ESI+0x92]=BL`, and copies
`[ESI+0x7B]=word[ESI+0x79]`. Then **`0x7DAA3 TEST DI,DI; 0x7DAA6 JZ 0x7DAAD;
0x7DAA8 MOV EAX,ESI; 0x7DAAAA CALL dword[ESI+0x18]`** — `ECX(→EDI) != 0` is the
**invoke-now**: the row-0x24 body runs immediately on each record it installs.
(The FU-137 §2 / FU-181 §3 "ECX = invoke" contract is confirmed byte-level
here.)

Net effect: entering act 0xA (phase 0x20) fan-outs the row-0x24 machine onto
every installable record of both teams, in the act frame itself.

---

## 4. Action row 0x24 body (`0x86510..0x866F1`) — fresh byte-level

`0x86510..0x866F1` (482 B) is a self-contained 3-stage machine; `0x866F2
MOV EAX,EAX` is padding and row 0x26 begins at `0x866F4` (table §1), so the
boundary is exact.

**Prologue** `0x86510`: `PUSH EBX/ECX/EDX/ESI/EDI; MOV ECX,EAX (record);
XOR EAX,EAX`.

**Head** `0x86519..0x8654E`:
```
0x86519 MOV EDX, [ECX+0x89]             ; timer89
0x8651F MOV AX,  [0x157A64]             ; frame delta
0x86525 ADD EDX, EAX
0x86527 MOV AH,  [ECX+0x8D]             ; active
0x8652D MOV [ECX+0x89], EDX             ; store timer89
0x86533 TEST AH,AH ; JZ 0x86552         ; inactive skips the lane write
0x86537 MOV EAX,[ECX+4] ; MOV EAX,[EAX+0xA] ; MOV EDX,[0x157A38]
0x86543 SAR EAX,0x18                    ; byte [[rec+4]+0xD]
0x86546 MOV AL, [EDX+EAX]               ; table_0x157A38[that byte]
0x8654B SUB EAX, 2
0x8654E MOV word [ECX+0x7B], AX         ; [rec+0x7B] = table - 2
```
So the head accumulates `+0x89` and (when active) refreshes the `+0x7B` word
from the `0x157A38` table.

**Stage dispatch** `0x86552`: `AL = [ECX+0x92]`; `CMP AL,1; JC 0x8656F` (0);
`JBE 0x8665C` (1); `CMP AL,2; JZ 0x866DD` (2); else `0x866EC` epilogue.

**Stage 0** `0x8656F..0x8665B`:
```
0x8656F TEST AL,AL ; JNZ 0x866EC
0x86577 MOV EDX,[ECX+4]                 ; second pointer
0x8657A MOV dword [ECX+0x59], 0x9F0     ; pos_z := 0x9F0
0x86581 MOV EDX,[EDX+0xB] ; SAR EDX,0x18 ; byte [[rec+4]+0xE]
0x86587 LEA EAX,[EDX*4] ; SUB EAX,EDX ; SHL EAX,5 ; SAR EAX,4   ; = 6*that byte
0x86590 MOV EDX,[ECX]                   ; the team pointer
0x86595 MOV BH, byte [EDX+0x826]        ; team side
0x865A0 JNZ / 0x865A2 CWDE;NEG EAX      ; sign by the side byte
0x865A8 MOV word  [ECX+0x71], 0
0x865AE MOV dword [ECX+0x61], EAX       ; pos_x := facing term
0x865B1 MOV AX, [ECX+0x71]
0x865BA MOV word  [ECX+0x75], AX        ; vel75 := 0
0x865C0 MOV word  [ECX+0x73], AX        ; vel73 := 0
0x865C6 CALL 0x79C50                    ; face/aim helper (FU-160 §2 0x79C50)
0x865CB MOV EDX,[ECX+0x8A] ; SAR EDX,0x19
0x865E7 IMUL EDX,EAX                    ; the timer threshold
0x86601 MOV ESI,[ECX+0x89] ; CMP EDX,ESI ; JG 0x866EC   ; wait
0x86611 MOV EAX,[ECX+0x61] ; MOV [ECX+0x55],EAX
0x86617 MOV EAX,[ECX+4] ; MOV dword [ECX+0x4D], 0x7E0   ; target_x := 0x7E0
0x86639 MOV dword [ECX+0x89], 0
0x8664B MOV [ECX+0x4D], EDI             ; target_x += 6*byte
0x8664E INC AL ; MOV [ECX+0x92], AL     ; stage 0 -> 1
```
Stage 0 seeds `pos_z = 0x9F0`, a `±6·[[rec+4]+0xE]` facing written to `pos_x`
(`+0x61`), zeroes `+0x71/+0x73/+0x75`, calls the `0x79C50` face helper, then
waits until `timer89` crosses the `0x79C50`-derived threshold, commits
`target_x = 0x7E0 + 6·byte` (`+0x4D`), resets the timer and stages to 1.

**Stage 1** `0x8665C..0x866DC`:
```
0x8665C CALL 0x8EEC0 ; TEST AX,AX ; JNZ 0x86687
0x86666 CMP byte [0x157AC2],0 ; JNZ 0x86687
0x8666F CMP byte [0x14C32A],0 ; JNZ 0x86687
0x86678 MOV EAX,1 ; CALL 0x67800 ; CALL 0x8EEB4
0x86687 MOV EAX,[ECX+0x61] ; MOV [ECX+0x55],EAX
0x8668D MOV EAX,[ECX+4] ; MOV dword [ECX+0x4D], 0x7E0
0x866B4 SAR EAX,0x10                    ; EAX = [ECX+0x63]>>16 -> alias word [ECX+0x65]
0x866BA CMP EAX, 0x20
0x866BD JGE 0x866EC                     ; still far -> return
0x866C5 MOV dword [ECX+0x89], 0
0x866CF INC BL ; MOV [ECX+0x92], BL     ; stage 1 -> 2
```
Stage 1 runs the `0x8EEC0`/`0x67800`/`0x8EEB4` ball gates (skipped when
`[0x157AC2]`/`[0x14C32A]` are set), re-commits `target_x = 0x7E0 + 6·byte`,
waits while `word[rec+0x65] >= 0x20`, then stages to 2.

**Stage 2** `0x866DD..0x866F1`:
```
0x866DD LEA EDX, [ECX+0x4D]
0x866E0 MOV EBX, 0xFFFFFFFF
0x866E5 MOV EAX, ECX
0x866E7 CALL 0x6E1D0                    ; (rec, rec+0x4D, -1); same sink as phase 0x11
0x866EC.. epilogue: POP EDI/ESI/EDX/ECX/EBX ; RET
```
Stage 2 calls `0x6E1D0(rec, &rec+0x4D, -1)` and returns (no further stage).

So row 0x24 is a **two-target lane/anim machine**: assign the `0x7B` lane word,
aim `pos_x`/`target_x` through the `0x79C50` face, hold at `0x7E0 + 6·z`, then
release through `0x6E1D0`. The engine labels it
`sequence_lane/anim_byte` (`fifa96_match_handlers.c:4094`).

---

## 5. The act path to phase 0x20 through T1's seam

**Native.** The situation dispatcher `FUN_0008A938` (AX = situation, DL = side,
BX = invoke) routes situation 0 to the direct path (`0x8A947`) and, at the
table-2 arm `0x8AB82` (fresh `disassemble_bytes 0x8AB70`):
```
0x8AB82 MOV EBX, 1                      ; invoke-now
0x8AB87 MOV EAX, 0xA                    ; act 0xA
0x8AB8C XOR EDX, EDX                    ; stage 0
0x8AB8E CALL 0x888FC                    ; dispatch act 0xA
0x8AB93 MOV EDX, [ESP-2] ; SAR EDX,0x10 ; side
0x8AB97 MOV EAX, 0x11
0x8AB9C CALL 0x740A0                    ; write phase 0x11
```
`FUN_000888FC` then latches `byte[0x158828]=0xA`, `dword[0x158808]=
0x1107EC[0xA]=0x8B688` (`0x88929`), `word[0x158818]=0` (`0x88920`),
`byte[0x158829]=0` (`0x88918`), and — because `BX=1` and the handler is
non-NULL — invokes it **in the same frame** via `CALL dword[0x158808]`
(`0x7DAAA`/the act-machine invoke). The clock-tail tick
(`0x8B669 CMP [0x158808],0; 0x8B672 CALL [0x158808]`) re-runs the handler once
per granted frame until it clears the slot at its stage-2 terminal (§2).

**Native situation-0 producer.** `FUN_0004b02c` calls `FUN_0008A938` with
situation 0 at `0x4B0A3 XOR EAX,EAX; 0x4B0A5 CALL 0x8A938` (fresh
`disassemble_bytes 0x4B090`; the same function issues EAX=8 at `0x4B0D0`).
`FUN_0004b02c` has two callers, both `FUN_000493a0` (fresh `get_xrefs_to
0x4B02C` = 2). *(The `FUN_000493a0`/`FUN_0004b02c` body is not decomposed here
— an open leg, §8 FU188-L2.)*

**Engine parity (T1 landed).** `match_run_situation_side` is the derived
`FUN_0008A938`: fresh source, situation 0 → `fifa96_action_phase_situations[0]
= {phase 0x11, act 0x0A, stage 0}` (`fifa96_action_handlers.c:1133`), so it
writes phase 0x11 + arms, then unconditionally
`fifa96_match_act_dispatch(mr, out.act, out.stage, 0)` at
`fifa96_match_run.c:2641` (invoke deferred to the tick). The clock-tail tick
`fifa96_match_act_tick(mr)` (`match_run.c:3745`) resolves the phase via
`fifa96_action_phase_act(0xA) = 0x20` and calls
`fifa96_match_dispatch_phase(mr, 0x20)`. **phase_table[0x20].fn is NULL**, so
the tick no-ops today — the seam is wired end-to-end *except* the phase-0x20
body.

So the chain is: **situation 0 → `FUN_000888FC(0xA,0,1)` → `0x1107EC[0xA]
= phase_table[0x20] = 0x8B688` → code-0x24 install on 22 records → row 0x24**.

---

## 6. Engine status + T4 wiring rec

Engine tables (fresh source): action row **0x24 = NULL**
(`fifa96_match_handlers.c:4094`: `"FU-137 §6: FU-136 row 24: not ported
(partial); sequence_lane/anim_byte; OL-9"`); phase row **0x20 = NULL**
(`:4147`: `"FU-137 §6: FU-136 phase 20: not ported; FU-83 timeline, installs
0x24; OL-13"`). The phase resolver
`fifa96_match_dispatch_phase` reaches row 0x20 through the T1 seam but returns
`-UNSUPPORTED` (NULL `fn`).

**T4 wiring rec (coupled pair).**

1. **Phase 0x20 body** — add `fifa96_match_action_phase_20(struct
   fifa96_match_run *mr)` (native `0x8B688`) and register it at
   `fifa96_match_phase_table[0x20].fn` (`handlers.c:4147`):
   * head (`0x8B691`): `mr->act.timer += (int16_t)mr->state.frame_delta`;
   * stage 0 (`0x8B6C3`): seed the `0x15880C/0x158814/0x158830/0x158838/
     0x15884A/0x158848/0x15884C/0x158852` timeline cells (the `0x700F4`
     camera reset is a sink leg); **install code 0x24 on both teams' 11
     records** — the native invoke-now has no generic engine port (FU181-L3),
     so use the outfield double-dispatch shape: for each resolvable record set
     the code and run `fifa96_match_dispatch_action(mr, 0x24)` in the same
     frame (or an explicit `fifa96_match_entities_install` + dispatch pair);
     reset `mr->act.timer`, `stage -> 1`;
   * stage 1 (`0x8B7CB`): the `0x974D8(0x3E8)` wait, then `stage -> 2`;
   * stage 2 (`0x8B7F6`): wait `word[0x15881A] >= 0x3C` (alias trap; producer
     FU165-L2/FU175-L5), the `0x45001` gate, then **clear the act slot**
     (`mr->act.id/stage/timer = 0`), store the `0x92AC8` bit, and re-dispatch
     **situation 1** with `BX=0` (`fifa96_match_run_situation(mr, 1)`).
2. **Row 0x24 body** — add `fifa96_match_action_24` (native
   `0x86510..0x866F1`) and register it at `fifa96_match_action_table[0x24].fn`
   (`handlers.c:4094`): the head `+0x89` timer + `0x157A38`-table `+0x7B`
   refresh; stage 0 (`0x8656F`) the `0x79C50`-face aim + threshold wait +
   `target_x = 0x7E0 + 6·byte`; stage 1 (`0x8665C`) the
   `0x8EEC0`/`0x67800`/`0x8EEB4` gates + `word[+0x65] < 0x20` wait; stage 2
   (`0x866DD`) the `0x6E1D0(rec, rec+0x4D, -1)` release.

**Census honesty caveat (must not be fudged).** The pair has a **single
producer**: situation 0. No engine source calls
`fifa96_match_run_set_piece(mr, 0, …)` / `fifa96_match_run_situation(mr, 0)`
today (fresh `grep`: the direct-set situations are 3/5/6/7/9/0xC/0xB;
`set_piece` maps 0 only via the native direct path), and no test dispatches
situation 0. Under the FU-174 §2 criterion (a *live* engine installer) and the
rows-27/29/2C precedent, the coupled pair **adds 0 to the census until a live
situation-0 producer lands**; registering the bodies without that producer is
the same "ported body, no installer" honesty class and the table slots should
stay NULL (the FU181-L1 pattern). If T4 also lands the `FUN_0004b02c`-
equivalent situation-0 producer (or proves situation 0 reachable through the
act-8 stored-situation re-dispatch, `match_run.c:2803`), then **+1 phase
(0x20) +1 action (0x24) → `25/80 → 27/80 = 33%`** and the README title moves in
the same commit (AGENTS.md census rule). Recommended headline: **wire the
phase-0x20 body + row 0x24 as one coupled slice, gated on the situation-0
producer; do not count them before it.**

---

## 7. Errata / notes

1. **FU-182 §5 row "0x24" installer.** The installer addresses
   `0x8B747`/`0x8B782` are the loop **bodies**, and `0x8B750`/`0x8B78B` are the
   `CALL 0x7D9A4` bytes (`e84f22ffff`/`e81422ffff`, targets verified). The loop
   guards are `0x8B765`/`0x8B79E` (this slice).
2. **Phase-0x20 body ≠ FU-83 alone.** FU-182 §1 called row 0x20's body
   "FU-83 timeline, installs 0x24"; the byte-level derivation here is FU-188's
   (dates earlier). No contradiction.
3. **`0x110814` single xref.** The phase-0x20 handler has exactly one data xref
   (`0x110814`), the shared act/phase slot — no second dispatch surface.

---

## 8. Numbered legs

1. **FU188-L1 — the situation-0 producer (`FUN_0004b02c`/`FUN_000493a0`).** The
   native situation-0 dispatch is proven (`0x4B0A5`), but the
   `FUN_000493a0`→`FUN_0004b02c` body (its trigger and whether the engine's
   tape ever reaches it) is not decomposed here. This gates the T4 census
   (§6).
2. **FU188-L2 — the phase-0x20 timeline cells.** `0x15880C`, `0x158814`,
   `0x158830`, `0x158838`, `0x15884A`, `0x158848`, `0x15884C`, `0x158852` are
   written by stage 0 but their readers (and the `0x700F4` camera reset, the
   stage-1 `0x974D8` wait, the stage-2 `0x7417C`/`0x92AC8`/`0x36200` sinks) are
   cited by address only; the `word[0x15881A]` gate producer is FU-165-L2 /
   FU-175-L5.
3. **FU188-L3 — the row-0x24 body sinks.** `0x79C50` (face), `0x8EEC0`,
   `0x67800`, `0x8EEB4`, `0x6E1D0`, and the `0x157A38`/`0x157AC2`/`0x14C32A`
   cells are byte-derived only; the `0x79C50` threshold arithmetic
   (`dword[+0x8A] >> 0x19`) is not semantically resolved.
4. **FU188-L4 — the invoke-now port.** The code-0x24 install is `ECX=1`
   (invoke-now, verified at `0x7DAA3`); the engine has no generic invoke-now
   (FU181-L3), so the phase-0x20 body must replicate the double dispatch or the
   row-0x24 stage/timer starts a frame off (cadence leg).
5. **FU188-L5 — the `0x7B` lane word.** Row 0x24's head writes
   `[rec+0x7B] = table_0x157A38[byte[[rec+4]+0xD]] − 2` (active only); the
   `0x157A38` table identity and the `[rec+4]` second-pointer binding are
   cited, not decomposed.

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info`; `read_memory` `0x1106E0` (180 B), `0x110794`
(140 B); `disassemble_bytes` `0x8B688` (500 B), `0x86510` (320 B), `0x8664E`
(240 B), `0x8AB70` (80 B), `0x888B0` (80 B), `0x88B30` (80 B), `0x4B090`
(80 B); `disassemble_function` `0x7D9A4` (73 insns); `get_xrefs_to` `0x86510`
(1), `0x8B688` (1), `0x8A938` (39), `0x4B02C` (2); `get_function_by_address`
`0x4B02C`; `search_instructions` operand `0x24` (200, truncated). Engine/loader
source read first-hand: `src/fifa96_engine/fifa96_match_act.{c,h}` (whole),
`fifa96_match_handlers.c` (3390..3570, 4090..4176),
`include/fifa96_engine/fifa96_match_entities.h` (4..204),
`src/fifa96_engine/fifa96_match_run.c` (2596..2645, 2740..2814),
`src/fifa96_loader/fifa96_action_handlers.c` (1100..1161). Docs cross-cited:
FU-67 §3, FU-137 §2/§6, FU-165-L2, FU-170 §7, FU-174 §2, FU-175 §1/§2/§10,
FU-181 §3/§4, FU-182 §1/§5. No writes beyond this slice file and the workspace
report.
