# FU-194 — act-tail cohort 2: phase `0x1D`/`0x1E`/`0x20` (act 7 / 8 / 0xA)

Provenance: M2 phase-17 wave **R42**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; this session `get_current_program_info` = `watcom:LE:32:default`,
`min_address` `0x10000`, `max_address` `.image::00174a2a`). Every address/byte
below is a fresh `read_memory` / `disassemble_bytes` / `disassemble_function` /
`decompile_function` / `get_xrefs_to` / `search_byte_patterns` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named). LE-image
globals sit at `+0x100000` from the file offset; the addresses here are
`/FIFA96.EXE` program (runtime) addresses. The alias traps are applied
(`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`). No writes: no
rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; no git commands.

Scope: derive the act-tail cohort **2** — phase `0x1D` (act 7, `0x89868`),
phase `0x1E` (act 8, `0x8A798`), phase `0x20` (act 0xA, `0x8B688`) — the bodies
and their producers; re-run the fresh `CALL 0x888FC` producer census; and resolve
the brief's question: **does driving act 7 make row `0x20` producer-live**
(the row-`0x20` installer `0x899a2` lives in `0x89868` arm 0, FU189-L1). Then
state the honest T5 wiring rec per phase row. This slice unblocks task **T5**.

---

## 1. Fresh anchors — act table, phase table, the census

Fresh `read_memory 0x1107EC` (64 B), the act table:

| act | `0x1107EC[act]` | = phase | phase slot | handler |
|---|---|---|---|---|
| 0 | `0` | — | — | NULL |
| 1 | `0x00088DC8` | 0x17 | `0x1107A8` | act-1 body |
| 2 | `0x0008922C` | 0x18 | `0x1107AC` | (ported, FU-183) |
| 3 | `0x00089FA4` | 0x19 | `0x1107B0` | (ported, FU-187) |
| 4 | `0x00089620` | 0x1A | `0x1107B4` | |
| 5 | `0x000890EC` | 0x1B | `0x1107B8` | (no invoker, FU175-L2) |
| 6 | `0x00089110` | 0x1C | `0x1107BC` | |
| **7** | **`0x00089868`** | **0x1D** | **`0x110808`** | **this slice §3** |
| **8** | **`0x0008A798`** | **0x1E** | **`0x11080C`** | **this slice §4** |
| 9 | `0x00088F4C` | 0x1F | `0x110810` | (ported, FU-175 §10) |
| **0xA** | **`0x0008B688`** | **0x20** | **`0x110814`** | **this slice §5** |
| 0xB | `0x0008B874` | 0x21 | `0x110818` | |
| 0xC | `0x0008B900` | 0x22 | `0x11081C` | |

Bytes (fresh): `00000000 c88d0800 2c920800 a49f0800 20960800 ec900800 10910800
68980800 98a70800 4c8f0800 88b60800 74b80800 00b90800 …` — byte-identical to
FU-175 §1 / FU-190 §1. The base is exactly `0x110794 + 0x16*4` (act `a` →
phase `0x16+a`), so act 7/8/0xA and phase `0x1D`/`0x1E`/`0x20` are the same three
slots (**the FU165-L9 dual act/phase identity**).

Fresh `get_xrefs_to`, each handler's **sole** reference is its phase-table slot:

| handler | fresh xrefs | from | = |
|---|---|---|---|
| `0x89868` (phase 0x1D) | **1** (DATA) | `0x110808` | `0x110794 + 0x1D*4` |
| `0x8A798` (phase 0x1E) | **1** (DATA) | `0x11080C` | `0x110794 + 0x1E*4` |
| `0x8B688` (phase 0x20) | **1** (DATA) | `0x110814` | `0x110794 + 0x20*4` |

No direct call site exists for any of the three — **the act/phase seam is the
only dispatch route** (`FUN_0008D098` → `FUN_0006D920` → `[rec+0x1C]`, and the
global act slot `[0x158808]` ticked by `0x8B672`; FU-175 §2/§5).

### 1.1 Fresh `CALL 0x888FC` producer census (11 sites, byte-identical to FU-175 §1)

Fresh `get_xrefs_to 0x888FC` = **11**; every site re-disassembled this session
for its `EAX` (act) / `DL` (stage) / `BX` (invoke):

| site | in | EAX = act | DL | BX | fresh bytes |
|---|---|---|---|---|---|
| `0x8AB57` | `FUN_0008A938` (sit-3 direct arm `0x8AB08`) | **4** | 0 | 1 | `b804000000` (`0x8AB4B`) |
| `0x8AB8E` | `FUN_0008A938` (table-2 sit 0, `0x8AB82`) | **0xA** | 0 | 1 | `b80a000000` (`0x8AB87`) |
| `0x8ABCF` | `FUN_0008A938` (table-2 sit 1, `0x8ABAB`) | **1** | 0 | 1 | `89d8` (EAX=EBX=1, `0x8ABC7`) |
| `0x8A6FE` | `FUN_0008A43C` | **3** | 0 | 1 | `b803000000` (`0x8A6F9`) |
| `0x8A789` | `FUN_0008A43C` | **6** | 0 | 1 | `b806000000` (`0x8A784`; EBX=1 set `0x8A75C`) |
| `0x8AEBA` | `FUN_0008A938` (table-2 sit 8, `0x8AEAE`) | **7** | 0 | 1 | `b807000000` (`0x8AEB3`) |
| `0x8AED2` | `FUN_0008A938` (table-2 sit 9, `0x8AEC6`) | **2** | 0 | 1 | `b802000000` (`0x8AECB`) |
| `0x8AEEA` | `FUN_0008A938` (table-2 sit 0xA, `0x8AEDE`) | **2** | 1 | 1 | `b802000000`/`89da` (`0x8AEE3`/`0x8AEE8`) |
| `0x8AF1A` | `FUN_0008A938` (shared tail) | **9** *or* **8** | 0 | 1 | see §4.3 |
| `0x8BAE7` | `FUN_0008B9CC` (period chooser) | **0xC** *or* **0xB** | 0 | 1 | `b80c000000`/`b80b000000` (`0x8BAD4`/`0x8BAE0`) |
| `0x88EE8` | `FUN_00088DC8` (the act-1 body) | **4** | 0 | 1 | `b804000000` (`0x88EE1`) |

Union of statically-invoked acts = **{1,2,3,4,6,7,8,9,0xA,0xB,0xC}** — every act
except **5** (phase 0x1B) has an invocation site; act 5 has none (FU175-L2
re-confirmed: no site passes `EAX=5`). Note the **act 8** entry is not a literal
`EAX=8` site — it is a computed arrival at the shared `0x8AF1A` (§4.3).

The three handlers of this slice are produced by:
* **act 7** — `0x8AEBA` only (`FUN_0008A938` table-2 sit 8, BX=1).
* **act 8** — no literal site; the `0x8AA98 EAX=8` fallback → `JMP 0x8AF1A` (§4.3).
* **act 0xA** — `0x8AB8E` only (`FUN_0008A938` table-2 sit 0, BX=1).

---

## 2. The act/phase seam contract (fresh re-read)

The act slot writer `FUN_000888FC(AX=act, DL=stage, BX=invoke)` latches
`byte[0x158828]`, sets `[0x158808] = 0x1107EC[act]`, `[0x158818]=0`,
`[0x158829]=stage`, and invokes-now iff `BX!=0` and the handler is non-NULL
(*cross-cited* FU-175 §1). The clock tail `0x8B669 CMP dword[0x158808],0 /
0x8B672 CALL dword[0x158808]` ticks the installed handler once per granted frame
(*cross-cited* FU-175 §2). Therefore **every act handler reaches its stage-0 arm
on the frame it is dispatched (invoke-now) or on the next granted-frame tick** —
including act 7/8/0xA below.

The frame delta is `word[0x157A64]` (every handler head does
`timer[0x158818] += word[0x157A64]`).

---

## 3. Act 7 = phase `0x1D` handler `0x89868` (10-arm set-piece timeline)

Handler body `0x89868..0x89EAF` (fresh `disassemble_bytes 0x89868` 1760 B, 473
insns; `0x89EB0` is the next function's prologue). `get_function_by_address
0x89868` = "no function found" (not auto-created), as FU-189 §3.2 noted.

### 3.1 Head + arm table

```
0x89868 PUSH EBX/ECX/EDX/ESI/EDI/EBP ; SUB ESP,0x14
0x89871 EAX = [0x1587D4]            ; the actor record pointer
0x89876 [ESP+0x8] = EAX
0x8987a EAX = [EAX] ; EAX = [EAX+0x7A6]   ; actor team's ball record
0x89889 [ESP+0xc] = EAX
0x89882 DX = word[0x158818] ; 0x8988d DX += word[0x157A64]
0x89899 word[0x158818] = DX         ; timer accumulate
0x89894 AL = [0x158829]             ; stage byte
0x898a0 CMP AL,9 ; 0x898a2 JA 0x89EA6   ; > 9 -> epilogue
0x898ac JMP dword CS:[EDX*4 + 0x89840]
```

Fresh `read_memory 0x89840` (40 B) — the CS arm table, byte-for-byte FU-189
§3.2:

`{0x898B4, 0x899EE, 0x89A4E, 0x89A7A, 0x89AEA, 0x89B16, 0x89B50, 0x89B94,
0x89BB6, 0x89E6E}` (stage 0..9).

### 3.2 Arm 0 (`0x898B4..0x899ED`) — the install frame (**contains row-0x20**)

```
0x898b4 [0x15882A] = 0 ; [0x157AB1] = 0       ; clear the act-9 gate + a flag
0x898b6 EAX = [0x157A4A]>>24 ; 0x898ca CMP EAX,0x10 ; 0x898cd JZ 0x898e2
0x898cf..0x898dd  EDX = [0x157AAC]>>24 ; EAX = 0x10 ; CALL 0x740A0
                  ; if byte[0x157A4D] != 0x10 -> write PHASE 0x10
; --- two teams (EBP = 0x1588A4, stride 0x835) x 11 records (ESI += 0xB2) ---
0x898f3 MOV EDX,0x19 ; 0x898f8 EDI=1 ; 0x898fd EAX=EBP ; 0x898ff ECX=0
0x89903 MOV byte[EBP+0x9B],0 ; 0x8990a CALL 0x7D9A4      ; row 0x19 on the team base
0x8990f MOV EDX,0x26 ; 0x89916 ECX=[EBP+0x8B]>>24 ; CALL 0x6E598  ; anim 0x26
0x8992a MOV byte[ESI+0x14D],0
0x89931 AL=[ESI+0x14C] ; ESI+=0xB2
0x8993f JNZ skip ; else:
0x89941   MOV EDX,0x3 ; 0x8994c CALL 0x7D9A4             ; row 3 on inactive records
0x89960   CALL 0x6E598 (anim 0)
0x89966 MOVSX ECX,DI ; 0x8996c JL 0x8992a               ; record counter < 0xB
0x8996e [ESP+0x10]++ ; EBP += 0x835                       ; team counter < 2
; --- after both teams ---
0x8998e EAX=[ESP+8] ; 0x89992 EDX=[ESP+8] ; 0x89996 EAX=[EAX]
0x8999c MOV dword[EAX+0x7B2], EDX            ; [actor team +0x7B2] = actor record
0x89998 XOR ECX,ECX ; 0x8999a XOR EBX,EBX    ; invoke = 0 (deferred)
0x899a2 MOV EDX,0x20                          ; <<<< ROW-0x20 INSTALL
0x899a7 MOV EAX,[ESP+8]                       ; EAX = actor record
0x899ab CALL 0x7D9A4                          ; e8f43fffff
0x899b0..0x899be EAX=[ESP+0xc] ; [EAX+0x7B2] = EDX   ; [ball team+0x7B2]=ball rec
0x899c4 MOV EDX,0x1F ; 0x899c9 MOV EAX,[ESP+0xc] ; 0x899cd CALL 0x7D9A4
0x899d2 CALL 0x7A028                          ; reset the referee/event word block
0x899d7..0x899ed  stage 0 -> 1 ; timer = 0
```

Fresh bytes verified: `0x899a2 = BA 20 00 00 00`, `0x899ab = E8 F4 3F FF FF`
(the `0x899ab` CALL target re-computed: `0x899B0 + 0xFFF F3FF4 = 0x7D9A4`, agrees
with Ghidra). The sibling `0x899cd = E8 D2 3F FF FF` installs **row 0x1F** on the
ball record. `0x7A028` fresh `disassemble_function` (20 insns) = a zeroing of
`[0x158730..0x158746]` and `[0x158738..0x158744]` (the `word[0x158730]`/
`byte[0x158746]` gates that arm 2 later reads).

**The `0x7D9A4` installer contract** (fresh `get_xrefs_to 0x7D9A4` = **77**,
byte-identical to FU-174 §1/FU-182/FU-189/FU-190; contract *cross-cited* FU-188
§3): it writes `[rec+0x91]=code`, `[rec+0x18]=action_table[code]`, resets
`[rec+0x89]`/`[rec+0x9E]`, stores `[rec+0x92]=stage`, and invokes
`CALL dword[rec+0x18]` iff `ECX != 0`. At `0x899a2` `ECX=0`, so the row-0x20
install is **deferred**: the body runs only when the record's per-record action
dispatch next invokes `[rec+0x18]` (§6).

### 3.3 Arms 1..9 (the sequence) — fresh

| arm | addr | gate/wait | action |
|---|---|---|---|
| 1 | `0x899EE` | actor `word[rec+0x65] > 0x30` OR ball `word[rec+0x65] > 0x30` OR `word[0x15881A] < 0x78` -> epilogue | `CALL 0x974DC(0x1E)`; `[0x15882A]=1`; timer=0; stage→2 |
| 2 | `0x89A4E` | `[0x158730] != actor-rec` OR `byte[0x158746]==0` -> epilogue | timer=0; stage→3 |
| 3 | `0x89A7A` | `byte[0x15781D]==0` -> `0x89AB3`; `byte[0x15781E]!=0` -> `0x89AB3` | else `CALL 0x651F0(6)`; `CALL 0x974F0(0x5DC)`; timer=0; stage→4 |
| 4 | `0x89AEA` | `word[0x1577BE]!=0` OR `word[0x1577F0]!=0` -> epilogue | timer=0; stage→5 |
| 5 | `0x89B16` | `word[0x15881A] < 0x3C` -> epilogue; `>= 0x78` -> `0x89B39`; else `0x45001()` must be non-zero | `0x89B39`: timer=0; stage→6 |
| 6 | `0x89B50` | actor code `[rec+0x91] == 0x20` -> epilogue; ball `byte[rec+0x44]==0` -> epilogue | `MOV EAX,0x1A; CALL 0x4C374`; timer=0; stage→7 |
| 7 | `0x89B94` | `0x4BEC8()` != 0 -> epilogue | timer=0; stage→8 |
| 8 | `0x89BB6..0x89E6D` | the selection block (§7 leg) | emit event `0x8F188(EAX=0xBA, EBX=0x20, EDX=rec/team)`; `MOV EAX,7; CALL 0x4C374`; stage→9 |
| 9 | `0x89E6E` | `0x4BEC8()` != 0 -> epilogue | **terminal**: `[0x158828]=0` (act id), `[0x158808]=0` (slot clear), `[0x158829]=0` (stage), `[0x157AC0]=1`, `[0x157A4C]=0` |

Arm 9 is the **termination** (no situation re-dispatch, unlike act 8/0xA):
the act slot is zeroed and `[0x157AC0]` is set.

### 3.4 Act-7 producer = situation 8

Table-2 sit 8 (`0x8AE08..0x8AEC5`): if `byte[0x157A4C] != 1` it first seeds the
actor record and clears the act-7 per-act state, then the shared tail
`0x8AEAE MOV EBX,1 ; 0x8AEB3 MOV EAX,7 ; 0x8AEB8 XOR EDX,EDX ; 0x8AEBA CALL
0x888FC`. So **sit 8 → `FUN_000888FC(7, 0, BX=1)`** — invoke-now, arm 0 runs in
the same frame.

The upstream native producer of sit 8 (this session, `decompile_function
0x4B02C`): `FUN_0004B02C(param_1)` — a **period/half transition** — runs
`FUN_0008A938(0,0,0)` when `param_1 == 1` (`0x4B0A5`) and, when
`(byte[0x14C1D4] | word[0x14C1D6]) & 0x100`, runs `FUN_0008A938(8,0,0)`
(`0x4B0D9`, `MOV EAX,8`/`XOR EBX,EBX`/`XOR EDX,EDX`) and sets `[0x157AC2]=4`.
`FUN_0004B02C`'s two callers (fresh `get_xrefs_to 0x4B02C` = **2**) are both
`FUN_000493A0` (`0x49415`/`0x4950A`); `FUN_000493A0` has 3 callers
(`FUN_00018014` `0x180AA`, `FUN_00018108` `0x18359`, `FUN_00018680` `0x18819`).
This is the FU188-L1 situation-0 producer family (see §7 FU194-L6).

**The gate trap for sit 8.** `FUN_0008A938`'s head routes sit 8 (a) to the
table-1 *queue* arm (the `[0x15B6A8]` screen-id setter, no act dispatch) when the
session gate `byte[0x14C32A] != 0` **and** `dword[0x15B6C0] == 0`; (b) to the
table-2 act-7 arm only when the gate is off or a situation is pending. So
`0x4B0D9` (`FUN_0008A938(8,0,0)`) drives **act 7 only with the session gate off
or pending**; otherwise it only sets the screen id.

---

## 4. Act 8 = phase `0x1E` handler `0x8A798` (2-team × 11-record restart sweep)

Handler body `0x8A798..0x8A8DD` (fresh `disassemble_bytes 0x8A798` 360 B, 91
insns; `0x8A8DE MOV EAX,EAX` is padding; `0x8A8E0` is the next data/table).
`get_function_by_address 0x8A798` = "no function found".

### 4.1 Head + stages

```
0x8a798 PUSH EBX/ECX/EDX
0x8a79b AL = [0x158829]              ; stage
0x8a7a0 CMP AL,1 ; 0x8a7a2 JC 0x8a7b2    ; 0 -> stage 0
0x8a7a4 JBE 0x8a7d6                       ; 1 -> stage 1
0x8a7a6 CMP AL,2 ; 0x8a7a8 JZ 0x8a895     ; 2 -> stage 2
0x8a7ae POP/POP/POP ; RET                 ; else epilogue
```

**Stage 0** (`0x8A7B2`): `TEST AL,AL; JNZ epilogue`. `[0x15882E]=0`,
`[0x15882F]=0` (the team/record walk indices), timer `[0x158818]=0`, stage → 1,
then **falls through** into stage 1.

**Stage 1** (`0x8A7D6`) — the record sweep, one record per call:
```
0x8a7d6 CALL 0x4BEC8 ; TEST AX,AX ; JNZ epilogue     ; the wait gate
0x8a7e4 EAX = dword[0x15882B]>>24   ; = byte 0x15882E = TEAM index
0x8a7ec IMUL EAX,0x835 ; EDX = 0x1588A4 + EAX        ; team base
0x8a7f9 EAX = dword[0x15882C]>>24   ; = byte 0x15882F = RECORD index
0x8a801 IMUL EAX,0xB2 ; EAX += EDX                    ; record ptr
0x8a809 EDX = dword[rec+0x8E]>>24   ; = byte[rec+0x91] = the CODE byte
0x8a814 [0x1587F0] = 0
0x8a81a CMP EDX,0xC ; 0x8a81d JNZ 0x8a840
0x8a81f CMP byte[rec+0x99],0 ; JZ 0x8a840
0x8a828 CMP byte[rec+0x9A],0 ; JNZ 0x8a840
0x8a831 [0x1587F0] = rec
0x8a836 MOV EAX,0x19 ; 0x8a83b CALL 0x4C374
0x8a840 INC byte[0x15882F]                            ; record index++
0x8a846 EAX = dword[0x15882C]>>24 = byte 0x15882F
0x8a84e CMP EAX,0xB ; 0x8a851 JL epilogue             ; < 11 -> return
0x8a857 byte[0x15882E]++ ; byte[0x15882F] = 0         ; next team
0x8a86d EAX = dword[0x15882B]>>24 = byte 0x15882E
0x8a875 CMP EAX,1 ; 0x8a878 JLE epilogue              ; <= 1 -> return
0x8a87a..0x8a88b stage 1 -> 2 ; timer = 0
```

So act 8 walks **team 0 records 0..10 then team 1 records 0..10** (22 records),
one per granted frame; when a record's code `[rec+0x91] == 0xC` with
`byte[rec+0x99]!=0` and `byte[rec+0x9A]==0`, it stores the record at `[0x1587F0]`
and calls `0x4C374`. When both teams are done it stages to 2.

**Stage 2 (terminal)** (`0x8A895`):
```
0x8a895 CALL 0x4BEC8 ; TEST AX,AX ; JNZ epilogue      ; the wait gate
0x8a89f word[0x158818] = 0                            ; timer
0x8a8ab dword[0x158808] = 0                           ; act slot clear
0x8a8b1 byte[0x158829] = 0                            ; stage clear
0x8a8b7 byte[0x158828] = 0                            ; act id clear
0x8a8bd EDX = dword[0x158829]>>24 ; = byte 0x15882C = the stored SIDE
0x8a8c3 EAX = dword[0x158828]>>24 ; = byte 0x15882B = the stored SITUATION
0x8a8ce CALL 0x8A938                                 ; re-dispatch (sit, side), BX=0
0x8a8d3 byte[0x15882B] = 0xFF                         ; the stored situation cleared
```
Alias traps in action: `[0x158828]>>24` = byte `0x15882B` (the **stored
situation**), `[0x158829]>>24` = byte `0x15882C` (the **stored side**) — the same
cells the sit-0/0xB fallback writes (§4.3). So act 8 is a **restart sweep that
re-dispatches the stored situation**.

### 4.2 Act-8 walk indices vs stored situation

Three distinct cells: the walk indices live at bytes `0x15882E` (team) and
`0x15882F` (record); the stored situation/side at `0x15882B`/`0x15882C`. Stage 0
clears `0x15882E`/`0x15882F`; stage 2 clears `0x15882B` to `0xFF` after the
re-dispatch.

### 4.3 Act-8 producer = the `FUN_0008A938` BX!=0 fallback (computed, not literal)

Fresh, the dispatcher head `0x8A938..0x8A996` routes **sit 0**, **sit 0xB**, a
**closed session gate** `byte[0x14C32A]==0`, or a **pending situation**
`dword[0x15B6C0]!=0` to `0x8AA7B`:

```
0x8aa7b TEST BX,BX ; 0x8aa7e JZ 0x8AAA8          ; BX==0 -> the table-2 route
0x8aa80 XOR EDX,EDX ; 0x8aa82 XOR EAX,EAX ; 0x8aa84 MOV EBX,1
0x8aa89 CALL 0x740A0                              ; write PHASE 0 (EAX=0, side 0)
0x8aa8e AL = [ESP] (side) ; 0x8aa93 byte[0x15882C] = AL      ; stored SIDE
0x8aa98 MOV EAX,8                                 ; b808000000  <<< ACT 8
0x8aa9d byte[0x15882B] = CL (situation)           ; stored SITUATION
0x8aaa3 JMP 0x8AF1A                               ; e972040000 -> the shared CALL
```

`0x8AF1A` is the CALL shared with the table-2 sit-0xC arm (`0x8AF0E MOV EBX,1 ;
0x8AF13 MOV EAX,9`); on the fallback path `EAX=8`, `BX=1`, `DL=0` survive
(`0x8AAC7`/`0x8AEC8` not executed). So **act 8's producer is the BX!=0 fallback**
reached with sit 0/0xB or a gate-off/pending state. It is *not* a literal
`EAX=8` site.

**Verifying `0x8AF1A` bytes:** `e8ddd9ffff` → target `0x8AF1F + 0xFFF D9DDD =
0x888FC`, agrees with Ghidra (`CALL 0x000888fc`).

The engine's derived consumer of this exact fallback is
`match_run.c:2809..2821` (`fifa96_match_run_set_piece(..., bx!=0)`) — but it
**compresses the act-8 timeline into an immediate stored-situation re-dispatch**
and never dispatches act 8 (§6.2 / FU194-L2).

---

## 5. Act 0xA = phase `0x20` handler `0x8B688` (code-0x24 install timeline)

Re-verified fresh this session (`disassemble_bytes 0x8B688` 500 B, 149 insns);
byte-for-byte FU-188 §2. Handler body `0x8B688..0x8B872` (`0x8B873 NOP`,
`0x8B874` = row 0x21).

Head `0x8B691..0x8B6A5`: `word[0x158818] += word[0x157A64]` (timer), `AL =
[0x158829]`; stage dispatch `0x8B6AC CMP AL,1 / JC 0x8B6C3 (0) / JBE 0x8B7CB
(1) / CMP AL,2 / JZ 0x8B7F6 (2)`.

**Stage 0** (`0x8B6C3`): `TEST AL,AL; JNZ epilogue`; `CALL 0x73E28`; seeds
`[0x15880C]=0x960`, `[0x158814]=0`, `[0x158830]=0x1E0`, `[0x158838]=0`,
`word[0x15884A]=0`, `[0x158852]=3`, `word[0x15884C]=0`, `word[0x158848]=0`;
`CALL 0x700F4` (camera reset with `[0x10F328/2C/30]`); then **the two install
loops** (team 0 `ESI=0x1588A4`, team 1 `ESI=0x1590D9`, stride `0xB2`, 11 each):

```
0x8b742 MOV ECX,1 ; 0x8b747 MOV EDX,0x24 ; 0x8b74c MOV EAX,ESI
0x8b74e XOR EBX,EBX ; 0x8b750 CALL 0x7D9A4     ; e84f22ffff  (ECX=1 -> invoke-now)
0x8b765 MOV EAX,[ESP+1] ; SAR EAX,0x18 ; CMP EAX,0xB ; JL 0x8b742   (loop 1)
0x8b782 MOV EDX,0x24 ; 0x8b78b CALL 0x7D9A4    ; e81422ffff  (loop 2)
0x8b79e MOV EAX,[ESP-3] ; SAR EAX,0x18 ; CMP EAX,0xB ; JL 0x8b77d   (loop 2)
```

then `0x8B7AA MOV CH,[0x158829]; INC CH; word[0x158818]=0; byte[0x158829]=CH`
(stage 0 → 1). So stage 0 installs **code 0x24 invoke-now on all 22 records**
(ECX=1), i.e. the row-0x24 body runs in the act-0xA frame itself.

**Stage 1** (`0x8B7CB`): `MOV EAX,0x3E8; CALL 0x974D8`; then stage 1 → 2
(timer=0).

**Stage 2 (terminal)** (`0x8B7F6`): `EAX = [0x158816]>>16` (= `word[0x15881A]`);
`CMP EAX,0x3C; JL epilogue`; `CMP EAX,0x168; JGE 0x8B813`; else
`CALL 0x45001; TEST AL,AL; JZ epilogue`. At `0x8B813`: `word[0x158818]=0`,
`byte[0x158828]=0` (**act id**), `dword[0x158808]=0` (**slot clear**),
`byte[0x158829]=0` (stage); `CALL 0x7417C`; `CALL 0x92AC8; AND AL,1`;
`[0x158825]=AL`, `[0x157AAE]=AL`, `[0x15882C]=AL`; `EDX=[0x157AAB]>>24`;
`EAX=1; CALL 0x8A938` (**re-dispatch situation 1**, BX=0); `XOR EAX,EAX; CALL
0x36200`.

**Act-0xA producer = situation 0.** Table-2 sit 0 (`0x8AB82`): `MOV EBX,1 ;
MOV EAX,0xA ; XOR EDX,EDX ; CALL 0x888FC`; then `EDX=[ESP-2]>>16 ; EAX=0x11 ;
CALL 0x740A0` (**phase 0x11**). Upstream: `FUN_0004B02C(param_1==1)` →
`FUN_0008A938(0,0,0)` (`0x4B0A5`, §3.4).

---

## 6. The engine status + the act-7 → row-0x20 verdict

### 6.1 Does driving act 7 make row `0x20` producer-live?

**Yes — at the installer level, and transitively at the body level.**

* **Installer.** The row-0x20 installer `0x899a2` is in **arm 0** of the
  phase-0x1D handler `0x89868`, which is the handler for **act 7** (act table
  `[7]` = phase table `[0x1D]` = `0x89868`). Every act reaches its stage-0 arm
  on dispatch (invoke-now) or the next granted-frame tick (§2). The act-7
  producer is **sit 8** (`0x8AEBA`) with `BX=1` (invoke-now), so `0x89868` arm 0
  runs in the same frame. Arm 0 executes `0x899a2 MOV EDX,0x20; 0x899ab CALL
  0x7D9A4` on the **actor record** `[0x1587D4]` (with `ECX=0`, deferred).
  **So driving act 7 fires the row-0x20 installer.** This resolves FU189-L1's
  "who installs row 0x20" — the act-7 arm 0 — and confirms the
  phase-seam-conditional class: the gate is **act 7 / phase 0x1D**, not the
  row-0x20 installer.
* **Body.** The deferred install writes `[rec+0x18] = action_table[0x20] =
  0x84EEC` and `[rec+0x91]=0x20`. The per-record action dispatch is the
  **outfield record machine** `CALL dword[rec+0x18]` (native `0x7CD2B =
  ff5518`, in `FUN_0007CA54` via `FUN_0007C990`; the engine mirrors it at
  `match_run.c:1406/1410` `fifa96_match_dispatch_action(mr, e->code)`). So once
  code 0x20 is on an outfield actor record, the row-0x20 body `0x84EEC` runs via
  the ordinary per-record action machine — **not** the phase seam. (Whether the
  actor record `[0x1587D4]` is always an outfield record is FU194-L8.)

**Honest qualification.** In the **engine today** driving act 7 still does not
install row 0x20, because `fifa96_match_phase_table[0x1D].fn == NULL`
(`handlers.c:4671`): `fifa96_match_act_dispatch(mr, 7, 0, ...)` resolves phase
0x1D and the tick calls `fifa96_match_dispatch_phase(mr, 0x1D)`, which returns
`-UNSUPPORTED`, so the native arm-0 body (the installer) never runs. So the
verdict is: **row 0x20 becomes producer-live iff (a) the phase-0x1D body is
ported and (b) act 7 is driven — which needs a live situation-8 producer.** The
engine dispatches **no situation 8** (fresh grep of the engine's dispatch set:
`run_situation` 1/6/0xB and the `notify_code`; `set_piece` 2/3/6/9/0xA/0xC/0xB;
`ref_apply` sit 9/0xA) — FU189-L1 stands as the owner.

### 6.2 The three phase rows vs the engine

| phase row | handler | engine slot | producer (native) | engine producer? |
|---|---|---|---|---|
| **0x1D** (act 7) | `0x89868` | `{0x1D, NULL, …}` (`handlers.c:4671`) | sit 8 (`0x8AEBA`; `FUN_0004B02C` `0x4B0D9`) | **none** (no sit-8 dispatch) |
| **0x1E** (act 8) | `0x8A798` | `{0x1E, NULL, …}` (`handlers.c:4672`) | the `FUN_0008A938` BX!=0 fallback (`0x8AA98`) | **none dispatch act 8** — the engine *compresses* the fallback (`match_run.c:2809..2821`) |
| **0x20** (act 0xA) | `0x8B688` | `{0x20, NULL, …}` (`handlers.c:4675`; body ported FU-188 §10) | sit 0 (`0x8AB8E`; `FUN_0004B02C` `0x4B0A5`) | **none** (no sit-0 dispatch) |

Row **0x20** action slot: `0x110760` (= `0x1106E0 + 0x20*4`, fresh read
`ec4e0800` = `0x84EEC`) — engine `{0x20, NULL, …}` (`handlers.c:4611`).

### 6.3 T5 wiring rec per phase row

* **Phase 0x1D — do not count yet; port the body + a sit-8 producer as one
  slice.** Port `fifa96_match_action_phase_1D` (`0x89868`, the 10 arms of §3.2/
  §3.3) and wire act 7 via the existing seam
  (`fifa96_match_act_dispatch(mr, 7, 0, …)` + the tick). The honest count for
  the **phase row 0x1D** and **action row 0x20** is gated on a live situation-8
  producer: the engine must dispatch sit 8 (the native `FUN_0004B02C`/`0x8B0D9`
  path or an equivalent). Without it, both stay NULL/uncounted (the FU181-L1 /
  FU188-L1 precedent). **Because arm 0 is exactly the row-0x20 installer,
  landing phase 0x1D + a sit-8 producer makes row 0x20 install-live in the same
  commit** — a coupled pair, like FU-188's phase-0x20+row-0x24.
* **Phase 0x1E — leave NULL; owner = the derived act-8 compression
  (FU194-L2).** The engine already models the act-8 fallback as a *compressed*
  stored-situation re-dispatch (`match_run.c:2809..2821`, FU-149 L6) and never
  dispatches act 8. Porting `0x8A798` would *replace* that derivation with the
  full 22-record code-0xC→0x19 sweep — a behavior change, not an additive
  wiring. Do not register the slot; document FU-149 L6 as the owner. No census
  move.
* **Phase 0x20 — leave NULL; owner FU188-L1 (the sit-0 producer).** The body is
  already ported (`fifa96_match_action_phase_20`, FU-188 §10) and row 0x24 too;
  the sole missing piece is a live situation-0 producer. Same sit-0 family as
  phase 0x1D's chain (`FUN_0004B02C(param_1==1)`). No census move until it
  lands. (This matches FU-188 §10 exactly; re-verified first-hand here.)

**Census.** This slice changes nothing: all three phase slots stay NULL and no
engine producer lands. The verified count stays **[as landed by the phase-16
series]** (`docs/ENGINE.md` census line; README title is derived per AGENTS.md).
No row is asserted live.

---

## 7. Numbered legs

1. **FU194-L1 — act-7 (phase 0x1D) engine producer.** The native producer is
   sit 8 (`FUN_0008A938` table-2 `0x8AEAE`; `FUN_0004B02C` `0x4B0D9`), but no
   engine source dispatches situation 8 and the phase-0x1D body is unported, so
   row 0x20's installer never fires. Owner: FU189-L1. Required to unblock the
   phase-0x1D + row-0x20 pair.
2. **FU194-L2 — act-8 (phase 0x1E) vs the engine's compressed fallback.**
   `match_run.c:2809..2821` substitutes a stored-situation re-dispatch for the
   full act-8 timeline (FU-149 L6). Whether that compression is behaviorally
   equivalent to `0x8A798` (the 22-record code-0xC→0x19 sweep + the 0x4BEC8
   waits) is unresolved; a port would have to reconcile it. Not decomposed here.
3. **FU194-L3 — `0x4C374` semantics (erratum candidate).** Fresh
   `disassemble_function 0x4C374` = `PUSH EDX; XOR EDX,EDX; MOV word[0x157A68],DX;
   POP EDX; RET` — it **ignores EAX**. Every `MOV EAX,<code>; CALL 0x4C374`
   site (act-8 stage 1 `0x8A83B` EAX=0x19; the phase-0x1D arm 6 `0x89B79`
   EAX=0x1A; arm 8 `0x89E52` EAX=7) only clears `word[0x157A68]`. The
   "install/emit code" connotation in FU-188 §5.1 / FU-189 §3.4 does **not**
   hold byte-level; those calls are a `[0x157A68]` clear with a vestigial EAX.
   No install semantics.
4. **FU194-L4 — the `word[0x157A68]` reader.** Cleared by the act-8/act-6/act-7
   `0x4C374` calls and by `FUN_0004B02C` (`_DAT_00157a68 = 0`); no fresh reader
   traced. Its role (an event/anim counter?) is unknown.
5. **FU194-L5 — act-0xA (phase 0x20) producer.** Native sit 0
   (`0x8AB8E` / `FUN_0004B02C` `0x4B0A5`); no engine producer. Owner FU188-L1
   (unchanged; re-verified here).
6. **FU194-L6 — the sit-0/sit-8 producer trigger (`FUN_0004B02C`).** A
   period/half transition (`param_1` 0/1; the `0x100` bit of
   `byte[0x14C1D4]|word[0x14C1D6]`), called from `FUN_000493A0` (3 callers:
   `FUN_00018014`/`FUN_00018108`/`FUN_00018680`). Its trigger graph and the
   engine's derived equivalent of `FUN_0004B02C` are not decomposed here — this
   is the true owner behind FU194-L1/L5.
7. **FU194-L7 — the phase-0x1D arm-8 selection block (`0x89BB6..0x89E6D`).**
   Byte-derived structure only: the `[0x1587E3..E9]` state, the team-side /
   record-rank comparisons, the `0x741B4`/`0x897F4` helpers, the event
   `0x8F188(0xBA)`, and the `EDI >= 0` winner vs the `EDI < 0` reset-to-stage-0
   branch are cited, not semantically resolved.
8. **FU194-L8 — the row-0x20 body invoker.** The deferred install writes
   `[rec+0x18]`; the per-record dispatch is `CALL [rec+0x18]` (native `0x7CD2B`
   in `FUN_0007CA54`, engine `fifa96_match_dispatch_action`, `match_run.c:1406/
   1410`). Whether the actor record `[0x1587D4]` (set by the sit-8 arm from
   `[0x1587CC + side*4]`) is always an outfield record (`index != 0`) so the
   machine invokes it is unproven.
9. **FU194-L9 — the phase-0x1D arm-0 phase write.** `0x898CF..0x898DD` writes
   **phase 0x10** (`CALL 0x740A0`, EAX=0x10, side `[0x157AAC]>>24`) when
   `byte[0x157A4D] != 0x10`. Its interaction with the derived phase machine (the
   `FUN_000740A0` → `FUN_0008D098` per-team arm) is not traced here.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` (program `/FIFA96.EXE`).
* `read_memory`: `0x1107EC` (64 B, act table), `0x89840` (40 B, CS arm table),
  `0x110760` (8 B).
* `get_xrefs_to`: `0x888FC` (11), `0x89868` (1), `0x8A798` (1), `0x8B688` (1),
  `0x8A938` (39), `0x7D9A4` (77), `0x4B02C` (2), `0x493A0` (3).
* `disassemble_bytes`: `0x8AB60` (976 B), `0x8AB40` (40), `0x8AA80` (64),
  `0x8AAF0` (128), `0x8A720` (96), `0x8A6E0` (64), `0x8A770` (48),
  `0x88ED0` (48), `0x8BAC0` (48), `0x89868` (1760 B, the full 10-arm handler),
  `0x8A798` (360 B, the full act-8 handler), `0x8B688` (500 B), `0x4B090` (120),
  `0x4C31C` (128), `0x8A938` (400), `0x7CD10` (48).
* `disassemble_function`: `0x4C374` (5 insns), `0x7A028` (20 insns).
* `decompile_function`: `0x4B02C`.
* `search_instructions` (`CALL`, operand `+0x18]` / `+0x1c]`) — 0 matches (the
  known operand-filter artifact, FU-189 §3.4).
* `search_byte_patterns`: `ff5618`, `ff5118`, `ff561c`, `ff511c` (locating the
  per-record `[rec+0x18]`/`[rec+0x1C]` dispatch sites).

Engine/loader source read first-hand: `src/fifa96_loader/fifa96_action_handlers.c`
(1100..1161, the phase rows + the `fifa96_action_phase_situations[0x0D]` table),
`src/fifa96_engine/fifa96_match_run.c` (1380..1424, the outfield action dispatch;
2596..2660 `match_run_situation_side`; 2735..2830 the `set_piece` direct path +
the act-8 compression; 2995..3074, 3190..3259 the screen machinery),
`src/fifa96_engine/fifa96_match_handlers.c` (4118..4182, 4638..4692 the phase
table; action rows 4611/4619). Docs cross-cited: FU-137 §6, FU-149 L6, FU-175
(§1/§2/§5/§7), FU-181, FU-183, FU-187, FU-188 (§2/§3/§5/§6/§10), FU-189
(§1/§3/§4/§10), FU-190 (§1/§6.2/§9). No writes beyond this slice file and the
workspace report; no rename/label/comment/function/script/project save; no git.
