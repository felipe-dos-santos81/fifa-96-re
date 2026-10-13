# FU-227 — the half-time (situation-1) route: the non-extra-time period-transition kickoff

Provenance: M2 phase-21 wave **W21-5** (Stage 1 recon), 2026-10-13, read-only.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument; confirmed this session by
`get_current_program_info` — `FIFA96.EXE` / path `/FIFA96.EXE`,
`watcom:LE:32:default`, image base `0`, min `0x10000`, max
`.image::00174a2a`, **2706** functions, **15239** symbols, `.image` overlay
present). Every address/byte/instruction/xref-count below is a **fresh**
`read_memory` / `disassemble_bytes` / `disassemble_function` / `get_xrefs_to` /
`get_function_by_address` of **this** session unless marked *cross-cited* (then
the frozen FU doc is named). The alias traps are applied (`dword[x]>>16` = the
word at `x+2`, `>>24` = the byte at `x+3`); LE globals sit at `+0x100000` from
the file offset; addresses are `/FIFA96.EXE` runtime addresses. Engine/loader
source read first-hand. **No writes**: no rename, comment, label, function,
script or project save; `/FIFA96.EXE` untouched; the ISO / `game/hdd/` untouched;
no git commands.

Scope (W21-5 brief): derive the **half-time** path (period 1 → period 2) and its
live preconditions, **distinct** from the extra-time route — (1) the
situation-1 arm `0x888F2` (the controlled-side dispatch inside `FUN_00088860`,
FU-196 §3); (2) trace situation 1 → act 1 → the phase-`0x17` act-tail row
(phase-18 T4 / FU-203), and confirm the phase-`0x17` preconditions; (3) contrast
the half-time (situation 1) and extra-time (situation 8) routes — the branch and
the selecting cell; (4) verdict + numbered legs. Extends FU-196 (§2/§3/§4/§6),
FU-203 (§1/§2 + addendum), FU-208, FU-215.

**Answer in one line.** The half-time (period-transition) kickoff is the **else**
arm of `FUN_00088860`'s period-index branch: at a **phase-`0xC` period end**
(`0x88863`, gated by the caller on `[0x14C32A] == 0`, `0x7423E`) the routine tests
the period index `[0x157AC2]` at `0x888B0 CMP EAX,4` / `0x888B3 JNZ 0x888E2`;
`!= 4` → `set_piece(1, [0x157AAB]>>24, BX 0)` at **`0x888F2`**. The dispatcher
`FUN_0008A938` takes its **direct path** (`0x8AA7B`, BX==0 → `0x8AAA8`) and, with
`CX == 1`, enters the **sit-1 arm `0x8AABF`** → tail `0x8AB63` → table-2
`[1] = 0x8ABAB` → **`0x8ABCF CALL 0x888FC` — act 1, stage 0, invoke-now**. Act 1's
act-tail slot is the **phase-`0x17`** handler `0x1107F0 = 0x88DC8` (act base
`0x110794`, phase = `0x16 + act`); its stage 0 writes the **live phase 1**
(`0x88E82 CALL FUN_000740A0(1)`). Extra time (situation 8) is the **same
routine's `== 4` arm** (`0x888C8`): the discriminating instruction is the single
`0x888B0 CMP EAX,4` and the **selecting cell is `[0x157AC2]`** (the period-index
byte). The port already carries both arms verbatim
(`fifa96_match_run.c:3199..3209`) but only *invokes* the mode-0 producer on the
`== 4` path (`:2654`), so the half-time else-arm is **fixtured-only** live and the
half-time route is **not live in the engine** today — the honest producer leg for
the spine.

---

## 1. The branch and the selecting cell (fresh)

### 1.1 `FUN_00088860` — the period-end dispatcher

Fresh `disassemble_bytes 0x88860` (220 B) and `disassemble_function 0x88860`
(48 instructions) — the whole routine:

```
0x88860 PUSH EBX / PUSH ECX / PUSH EDX
0x88863 MOV EAX,[0x00157A4A]            ; bytes a1 4a 7a 15 00
0x88868 SAR EAX,0x18                    ; EAX = byte [0x157A4D] (the live phase)
0x8886B CMP EAX,0xC
0x8886E JNZ 0x000888F7                  ; NOT a period end -> early RET
0x88874 CALL 0x0007417C                 ; (period-end sink)
0x88879 MOV AL,[0x00157AAE]
0x8887E MOV [0x00157AAF],AL             ; side store
0x88883 CALL 0x0006FFC0
0x88888 PUSH 0x0 / .. / 0x8889F CALL 0x000700F4
0x888A4 CALL 0x00073E28                 ; <- writes [0x157AAB] (the side source)
0x888A9 XOR EAX,EAX
0x888AB MOV AL,[0x00157AC2]             ; EAX = the PERIOD INDEX (zero-extended)
0x888B0 CMP EAX,0x4
0x888B3 JNZ 0x000888E2                  ; <<< THE BRANCH: index != 4 -> else arm
0x888B5 CALL 0x00092AC8                 ; extra time: rng
0x888BA XOR AH,AH / 0x888BC AND AL,0x1
0x888BE XOR EBX,EBX
0x888C0 MOVSX EDX,AX                    ; EDX = rng & 1  (side)
0x888C3 MOV EAX,0x8
0x888C8 CALL 0x0008A938                 ; <<< SITUATION 8  (extra time)
0x888CD MOV EBX,0x8 / 0x888D2 MOV EAX,0xC1 / 0x888D7 XOR EDX,EDX
0x888D9 CALL 0x0008F188                 ; FUN_0008F188(0xC1,0,8)  (sit-8 only)
0x888DE POP EDX / POP ECX / POP EBX / RET
0x888E2 MOV EAX,0x1
0x888E7 MOV EDX,[0x00157AAB]
0x888ED XOR EBX,EBX
0x888EF SAR EDX,0x18                    ; EDX = byte [0x157AAE] (controlled side)
0x888F2 CALL 0x0008A938                 ; <<< SITUATION 1  (half-time / period transition)
0x888F7 POP EDX / POP ECX / POP EBX / RET
```

So the **single discriminating instruction** between the two routes is
`0x888B0 CMP EAX,4` (with `0x888B3 JNZ 0x888E2`), and the **selecting cell is
`[0x157AC2]`** — the period-index byte read zero-extended at `0x888AB`. **`== 4`**
→ situation 8 (`0x888C8`, extra time); **`!= 4`** → situation 1 (`0x888F2`, the
period-transition kickoff, of which half-time period 1 → 2 is the canonical
case).

Two further fresh asymmetries between the arms, both in this routine:

* the sit-8 arm uses an **RNG side** (`0x888B5 CALL 0x92AC8` → `EDX = rng & 1`,
  `0x888C0`) and additionally calls **`FUN_0008F188(0xC1,0,8)`** (`0x888D9`);
  the sit-1 arm uses the **fixed controlled side** `[0x157AAB]>>24 = byte
  [0x157AAE]` (`0x888E7`/`0x888EF`) and does **not** call `0x8F188`.
* the sit-1 arm is the **fall-through** of the routine's index test (the `else`
  of the `JNZ`); the sit-8 arm sits between the test and the sit-1 arm.

`FUN_00088860` has **2 callers** (fresh `get_xrefs_to 0x88860` = **2**):
`0x4B08E` (inside `FUN_0004B02C`'s `param_1 == 0` arm) and **`0x74244`** (the
period-end family body). The `0x74244` call is gated on `[0x14C32A] == 0` — fresh
`disassemble_bytes 0x74220` (64 B):

```
0x74237 CMP byte[0x0014C32A],0x0
0x7423E JNZ 0x000742E3
0x74244 CALL 0x00088860            ; the period-end invocation (owner leg FU196-L6)
```

`0x74244` has **no Ghidra function** (fresh `get_function_by_address 0x74244` =
*"No function found"*) — the period-end family body is disassembly-only, exactly
as FU-196 §3.2 / FU196-L6 record.

### 1.2 The period-index cell `[0x157AC2]`

Fresh `get_xrefs_to 0x157AC2` = **56** refs. The **write** sites in that census
(fresh `disassemble_bytes` of each unless noted):

| site | function | instruction | bytes | effect |
|---|---|---|---|---|
| `0x73F82` | `FUN_00073EE0` | `MOV byte[0x157AC2],DH` (DH=0; `0x73F7C XOR DH,DH`) | `88 35 c2 7a 15 00` | reset `:= 0` |
| `0x8B579`→`0x8B58A` | `FUN_0008AF38` | `MOV BH,[0x157AC2]; INC BH; MOV byte[0x157AC2],BH` | `8a 3d …; fe c7; 88 3d …` | **period index ++** (the clock rollover) |
| `0x88D56`→`0x88D6A` | the act-0xC tail | `MOV BL,[0x157AC2]; INC BL; MOV byte[0x157AC2],BL` | `8a 1d …; fe c3; 88 1d …` | **period index ++** (act-0xC tail) |
| `0x38990` | `FUN_00038630` | `MOV DH,4; MOV byte[0x157AC2],DH` (`0x38989 MOV DH,4`) | `b6 04; 88 35 …` | **front-end `:= 4`** (extra-time request; FU215-L2) |
| `0x4B0DE` | `FUN_0004B02C` | `MOV byte[0x157AC2],0x4` | (`c6 05 … 04`) | producer A `:= 4` — the **dead** bit-8 arm (FU-215 §3.1) |
| `0x4BA20`/`0x4BA31` | `FUN_0004BA00` | `MOV byte[0x157AC2],0x8` … `MOV byte[0x157AC2],DL` | `c6 05 … 08` / `88 15 …` | transient bump to 8 around `CALL 0x4BE44`, restored |

So the period index is a **counter**: reset to 0, **incremented once per period
end** (rollover `0x8B58A`, act-0xC tail `0x88D6A`), and **forced to 4** by the
front-end extra-time path (`0x38990`). `FUN_00088860`'s `[0x157AC2] == 4` test is
therefore the **extra-time selector** and `!= 4` the **regulation period
transition**. (The exact half number ↔ index mapping beyond "4 = extra time" is
not statically pinned here — see leg FU227-L1.)

---

## 2. The half-time route: situation 1 → act 1 → phase `0x17` (fresh)

### 2.1 The dispatcher `FUN_0008A938` — the two paths

Fresh `disassemble_bytes 0x8A938` (200 B), `0x8AA60` (420 B)
(`disassemble_function 0x8A938` = 48 instructions at the head). The head:

```
0x8A93E MOV ECX,EAX                     ; ECX = situation
0x8A940 MOV word[ESP],DX                ; [ESP] = side
0x8A944 TEST AX,AX / JZ 0x8AA7B         ; sit 0 -> direct
0x8A94E CMP EAX,0xB / JZ 0x8AA7B        ; sit 0xB -> direct
0x8A957 CMP byte[0x14C32A],0 / JZ 0x8AA7B   ; session gate CLOSED -> direct
0x8A964 CMP dword[0x15B6C0],0 / JNZ 0x8AA7B ; pending -> direct
0x8A97F SUB ECX,2                       ; (queue path only) CX = situation-2
0x8A982 MOV [0x15B6B8],EAX
0x8A987 CMP CX,8 / JA 0x8AA60           ; queue path: situation>0xA -> latch 0xA
0x8A996 JMP CS:[EAX*4 + 0x8A8E0]        ; QUEUE table (index = situation-2)
```

The **direct path** `0x8AA7B` (fresh `disassemble_bytes 0x8AA7B`):

```
0x8AA7B TEST BX,BX / JZ 0x8AAA8         ; BX==0 -> situation routing
0x8AA80 XOR EDX,EDX / XOR EAX,EAX / MOV EBX,1 / CALL 0x740A0  ; (BX!=0) phase 0
0x8AA8E MOV AL,[ESP] / 0x8AA93 MOV [0x15882C],AL
0x8AA98 MOV EAX,8 / 0x8AA9D MOV [0x15882B],CL / JMP 0x8AF1A   ; act-8 replay fallback
0x8AAA8 CMP CX,0x1 / JC 0x8AB63         ; CX < 1 -> tail
0x8AAB2 JBE 0x8AABF                     ; CX == 1 -> the SIT-1 ARM
0x8AAB4 CMP CX,0x4 / JBE 0x8AB08        ; CX in {2,3,4} -> act-4 formation arm
0x8AABA JMP 0x8AB63
```

The **queue table** (fresh `read_memory 0x8A8E0`, 64 B; 9 live entries, index =
situation−2): `[0..2]=0x8A99E`, `[3]=0x8A9CD`, `[4]=0x8A9E8`, `[5]=0x8A9CD`,
`[6]=0x8AA60`, `[7]=[8]=0x8AA23`. Each queue arm latches the id and `RET`s.
Because `situation 1` gives `CX = −1 = 0xFFFF > 8`, the **queue path would take
`0x8AA60`** (`[0x15B6A8] = 0xA`, `[0x15B6C0] = 1`, RET) — i.e. with the head gates
*closed*, situation 1 is only **latched** and never acts. So situation 1 reaches
the sit-1 arm **only via the direct path** — which requires (head)
`[0x14C32A] == 0` OR `[0x15B6C0] != 0`, and (direct) `BX == 0`, `CX == 1`.

For the period-end route the caller gate `0x7423E JNZ` already requires
`[0x14C32A] == 0`, so `0x888F2`'s dispatch always takes the direct path.

### 2.2 The sit-1 arm `0x8AABF` → tail `0x8AB63` → table-2 `[1]` (fresh)

Fresh `disassemble_bytes 0x8AABF`:

```
0x8AABF MOV EAX,0x1588A4 / CALL 0x8C974      ; team-order sort (side 0)
0x8AAC9 MOV EAX,0x1590D9 / CALL 0x8C974      ; team-order sort (side 1)
0x8AAD3 MOV EAX,[0x157A49] / SAR 0x18 / CMP EAX,1 / JZ 0x8AB63
0x8AAE4 CMP byte[0x157B8E],0 / JNZ 0x8AAF6
0x8AAED CMP byte[0x157B8F],0 / JZ 0x8AB63
0x8AAF6 MOV EAX,[ESP] / 0x8AAF9 MOV word[0x15881E],CX / 0x8AB00 MOV [0x158820],AX
0x8AB06 JMP 0x8AB63
```

Every branch lands on `0x8AB63` (the tail), so the arm's side stores
(`[0x15881E]`/`[0x158820]`, gated on the `[0x157A49]>>24 != 1` +
`[0x157B8E]`/`[0x157B8F]` conditions) are the **only** conditional part; the
arm's exit is unconditional. Tail `0x8AB63` (fresh):

```
0x8AB63 MOV AL,[ESP] / 0x8AB66 MOV [0x157AAF],AL   ; side store
0x8AB6B CMP CX,0xC / JA 0x8AF1F
0x8AB75 XOR EAX,EAX / 0x8AB77 MOV AX,CX
0x8AB7A JMP CS:[EAX*4 + 0x8A904]                    ; TABLE-2 (index = situation)
```

Fresh `read_memory 0x8A904` (52 B, 13 dwords): `[0]=0x8AB82`, **`[1]=0x8ABAB`**,
`[2]=0x8ABDB`, `[3]=0x8ABF3`, `[4]=0x8ADC0`, `[5]=0x8ADD8`, `[6]=0x8AC28`,
`[7]=0x8ADF0`, `[8]=0x8AE08`, `[9]=0x8AEC6`, `[0xA]=0x8AEDE`, `[0xB]=0x8AEF6`,
`[0xC]=0x8AF0E`. With `CX == 1` the tail takes **table-2 `[1] = 0x8ABAB`**.

### 2.3 The sit-1 table arm `0x8ABAB` → `CALL 0x888FC` (act 1) (fresh)

Fresh `disassemble_bytes 0x8ABAB`:

```
0x8ABAB MOV EBX,0x1E0
0x8ABB0 XOR CH,CH / 0x8ABB2 XOR EDX,EDX
0x8ABB4 MOV byte[0x157AB1],CH              ; = 0
0x8ABBA MOV dword[0x158830],EBX            ; = 0x1E0
0x8ABC0 MOV EBX,0x1                        ; invoke-now
0x8ABC5 XOR ECX,ECX
0x8ABC7 MOV EAX,EBX                        ; EAX = 1 (act)
0x8ABC9 MOV dword[0x158838],ECX            ; = 0
0x8ABCF CALL 0x000888FC                    ; ACT 1, stage 0 (EDX), invoke-now (EBX=1)
```

So the sit-1 route selects **act 1, stage 0, invoke-now** (`EAX=1`, `EDX=0`,
`EBX=1`). Fresh `get_xrefs_to 0x888FC` = **11** sites; `0x8ABCF` is the sole
site carrying `EAX=1` (the per-site act/stage/invoke decode is *cross-cited*
FU-203 §1, whose 11-site table matches this fresh count) — i.e. **the act-1
producer is exactly this one static call**, reached iff situation 1 is dispatched
(BX==0, direct).

### 2.4 Act 1's act-tail slot = phase `0x17`; its body writes live phase 1 (fresh)

Fresh `read_memory 0x110794` (140 B). The act base is `0x110794 + 0x16*4 =
0x1107EC`, so act `a` → slot `0x1107EC[a]` and phase `0x16 + a`:

* `0x1107EC[1] = 0x1107F0 = 0x088DC8` → **act 1 selects phase `0x17`**;
* `0x1107EC[6] = 0x110804 = 0x089110` (act 6 → phase 0x1C);
* `0x1107EC[7] = 0x110808 = 0x089868` (act 7 → phase 0x1D, the extra-time slot).

Fresh `get_xrefs_to 0x88DC8` = **1** — the only reference is the DATA slot
`0x1107F0` (`get_xrefs_to 0x1107F0` type DATA). So the act-1 body is reached
**only** through the phase-`0x17`/act-1 dual identity.

Fresh `disassemble_bytes 0x88DC8` (400 B) — the act-1 body:

* head `0x88DCD MOV EAX,3 / CALL 0x4C374`; the act timer
  `word[0x158818] += word[0x157A64]` (`0x88DD7..0x88DF5`);
* **head guard** `0x88DFC CMP DX,CX` (`[0x157AC5]` vs `[0x157AC7]`) / `JZ
  0x88E39`; else `0x88E01 MOV DL,[0x157AC2] / CMP EDX,2 / JL 0x88E39`; else
  `0x88E0E MOV EAX,0x14 / CALL 0x4C374 / … clear act slot / JMP 0x88F34`
  (bail);
* stage dispatch `0x88E39 MOV AL,[0x158829]`: `AL <= 0` → **stage 0**
  `0x88E4B`; `AL == 1` → **stage 1** `0x88EBC`; else → epilogue;
* **stage 0** `0x88E4B`: `PUSH 0 / … / CALL 0x700F4 / CALL 0x73E28`;
  `0x88E74 MOV EDX,[0x157AAC] / SAR EDX,0x18` (side);
  `0x88E7A MOV EAX,0x1 / 0x88E82 CALL 0x000740A0` — **the live phase-1 write**;
  `CALL 0x73E08` (placement commit); `… / CALL 0x4C324`;
  `0x88EA0 CH=[0x158829] / INC CH / [0x158829]=CH` (stage++), `[0x158818]=0`;
* **stage 1** `0x88EBC`: `[0x157A49]>>24 == 1` → skip; else
  `[0x157B8E]|[0x157B8F] != 0` → `EBX=1 / EAX=4 / EDX=0 / CALL 0x888FC`
  (`0x88EE8`, **act-4 hand-off**, invoke-now); else `0x88EF3`: the
  `word[0x15881A] ([0x158816]>>16) >= 0x78` stage-1b release.

**So "act 1 → phase `0x17`" has two meanings, both true and distinct:** the
**act-tail slot identity** (`0x1107F0 = 0x88DC8`, phase `0x16+1 = 0x17`) and the
**live phase-register write inside the body** (stage 0 writes phase **1** via
`FUN_000740A0(1)`, the kickoff phase). The phase `0x17` is the *table row* the
act-1 body is filed under; the body's own observable phase output is **1**.

---

## 3. The half-time route's live preconditions (fresh)

The chain from a period end to the phase-`0x17` body, with the gate at each step:

| # | gate | site (fresh) | condition | selects |
|---|---|---|---|---|
| P1 | caller session gate | `0x74237 CMP byte[0x14C32A],0` / `0x7423E JNZ 0x742E3` | `[0x14C32A] == 0` | allows `0x74244 CALL 0x88860` |
| P2 | period-end phase | `0x88863 MOV EAX,[0x157A4A] / SAR 0x18 / CMP EAX,0xC / JNZ 0x888F7` | `[0x157A4A]>>24 == 0xC` (byte `0x157A4D`) | the whole body |
| P3 | **extra-time discriminator** | `0x888B0 CMP EAX,4` / `0x888B3 JNZ 0x888E2` | `[0x157AC2] != 4` | the sit-1 else arm `0x888E2` |
| P4 | dispatcher direct path | `0x8A957 JZ 0x8AA7B` (satisfied by P1) / `0x8A964 JNZ 0x8AA7B` | `[0x14C32A]==0` OR `[0x15B6C0]!=0` | `0x8AA7B` (else → queue `0x8AA60`, latch `0xA`, RET) |
| P5 | direct BX test | `0x8AA7B TEST BX,BX / JZ 0x8AAA8` (`0x888ED XOR EBX,EBX`) | `BX == 0` | situation routing (not the act-8 replay) |
| P6 | sit-1 selector | `0x8AAA8 CMP CX,1 / JBE 0x8AABF` | `CX == 1` (situation 1) | the sit-1 arm |
| P7 | act-1 head guard | `0x88DFC`..`0x88E0C` | `[0x157AC5]==[0x157AC7]` OR `[0x157AC2] < 2` | proceeds (half-time: index 1 < 2 → always) |

Once P1–P4 hold, the sit-1 arm's exit to its act-1 call is **unconditional** (§2.2
— every arm branch lands on the tail `0x8AB63`, and `CX==1` forces table-2 `[1]`).
The **operative live precondition** is therefore **P1** (`[0x14C32A] == 0`), which
itself satisfies P4; P2 must also hold (the period must be at the phase-`0xC`
completion screen), and P3 must be false (index `!= 4`). P7 is automatically
satisfied for the half-time case (`[0x157AC2] < 2`).

Two supporting fresh facts:

* **The side source.** The sit-1 side is `[0x157AAB]>>24` = byte `[0x157AAE]`
  (`0x888E7`/`0x888EF`). Fresh `get_xrefs_to 0x157AAB` = **3**:
  `0x73E95` WRITE (`FUN_00073E28`, called at `0x888A4` just before the read),
  `0x888E7` READ (`FUN_00088860`), `0x8B84F` READ. So the half-time side is the
  cell written by the immediately-preceding `FUN_00073E28`.
* **The other situation-1 producer.** `0x8B85D` also dispatches situation 1 —
  fresh `disassemble_bytes 0x8B840`: `0x8B845 MOV [0x157AAE],AL / 0x8B84F MOV
  EDX,[0x157AAB] / 0x8B855 MOV EAX,1 / 0x8B85A SAR EDX,0x18 / 0x8B85D CALL
  0x8A938`. This is the **phase-`0x20` terminal** (the match-start / period-0→1
  kickoff, *cross-cited* FU-203 addendum), *not* the period-transition route this
  slice owns. Fresh `get_xrefs_to 0x8A938` = **39**; `0x888F2` and `0x8B85D` are
  the two located `EAX=1` dispatchers (the exhaustive per-site `EAX` census of all
  39 remains a leg — FU227-L3).

---

## 4. The two routes contrasted (fresh)

Both routes share **one routine** (`FUN_00088860`), **one predecessor** (a
phase-`0xC` period end), **one dispatcher** (`FUN_0008A938`), and **one act seam**
(`CALL 0x888FC` → act → act-tail slot). They differ in **exactly one tested cell**
and, downstream, in the act chosen:

| | **half-time** (this slice) | **extra-time** (FU-208 / FU-215) |
|---|---|---|
| discriminator | `0x888B0 CMP EAX,4` → `JNZ` (index `!= 4`) | same instruction → fall-through (index `== 4`) |
| selecting cell | **`[0x157AC2]`** (period index), byte `[0x157AC2]` | same cell |
| dispatch | `0x888F2 CALL 0x8A938` — **sit 1** | `0x888C8 CALL 0x8A938` — **sit 8** |
| side | fixed `[0x157AAB]>>24` (byte `0x157AAE`) | RNG `rng & 1` (`0x888B5`/`0x888C0`) |
| extra call | none | `FUN_0008F188(0xC1,0,8)` (`0x888D9`) |
| dispatcher path | direct `0x8AA7B` → `0x8AAA8` → sit-1 arm `0x8AABF` | direct `0x8AA7B` → `0x8AAA8` → tail `0x8AB63` |
| table-2 | `[1] = 0x8ABAB` | `[8] = 0x8AE08` |
| act seam | `0x8ABCF CALL 0x888FC(1,0,1)` | `0x8AEBA CALL 0x888FC(7,0,1)` |
| act-tail slot / live phase | phase **`0x17`** (`0x1107F0 = 0x88DC8`); body writes live phase **1** | phase **`0x1D`** (`0x110808 = 0x89868`) |

So the **cell that selects the route is `[0x157AC2]`** and the **single
discriminating instruction is `0x888B0 CMP EAX,4`**. Half-time (`!= 4`) yields
**act 1 → the phase-`0x17` kickoff row**; extra-time (`== 4`) yields **act 7 →
the phase-`0x1D` row**. There is **no other branch** between the two: the
dispatcher path, BX/queue tests and the act seam are identical up to the act
value.

---

## 5. Engine parity — the half-time arm is carried but not invoked (fresh source)

`fifa96_match_run_period_setup(mr, mode)` already reproduces **both** native arms
verbatim (`src/fifa96_engine/fifa96_match_run.c:3194..3209`, read first-hand):

```c
} else if (mode == 0) {                          /* FUN_00088860 */
  if (mr->state.phase == 0x0Cu) {                /* 0x88863  (P2) */
    if (mr->global_157ac2 == 4u) {               /* extra time (0x888A0) */
      uint16_t r = 0;
      (void)fifa96_rng_step(&mr->rng, &r);
      (void)fifa96_match_run_set_piece(mr, 8u, (uint8_t)(r & 1u), 0u); /* 0x888C8 */
    } else {
      (void)fifa96_match_run_set_piece(               /* 0x888F2  <-- HALF-TIME */
          mr, 1u, mr->phase_machine.side_controlled, 0u);
    }
  }
}
```

and the loader situation table carries the sit-1 → act-1 row
(`src/fifa96_loader/fifa96_action_handlers.c:1134`:
`{FIFA96_ACTION_PHASE_NONE, 1u, 0u, 0u, 0u}`), with `fifa96_action_phase_act(1) =
0x17` (`:1117`). The phase-`0x17` body is registered and **COUNTED**
(`fifa96_match_handlers.c:5235`).

**But the mode-0 producer is only *invoked* on the extra-time path.** Fresh
`src/fifa96_engine/fifa96_match_run.c:2654`:

```c
if (mr->global_157ac2 == 4u)                     /* <-- index-4 gate only */
  (void)fifa96_match_run_period_setup(mr, 0);    /* 0x74244 -> 0x88860 */
```

The comment at `:2650` states it explicitly: *"the non-extra-time sit-1 else-arm
(0x888F2) stays a fixtured branch (the live period-end screen transition is owned
by the lifecycle; not this slice)"*. So on the live tape the **else** arm
(`:3205..3208`) is **unreachable**: `period_setup(mode 0)` is never entered with
`global_157ac2 != 4`. The half-time (sit-1) period-transition route is therefore
**not live in the engine** — the native `0x74244 CALL 0x88860` runs on **every**
period end, but the port only mirrors the `== 4` case.

**Consequence for the phase-21 spine.** Playing a half-time (period 1 → 2)
completion is a **producer-wiring** task, not a body task: on the non-4
period-end completion the live driver must reach the mode-0 producer's **else**
arm (native `0x888F2`), i.e. the port must invoke `fifa96_match_run_period_setup(mr,
0)` (or `set_piece(mr, 1u, side_controlled, 0u)`) on the `!= 4` path — the same
producer the extra-time case already uses, whose sit-1 arm and act-1 body already
exist. No new phase row, no new body.

---

## 6. Census impact

**None.** The phase-`0x17` row is already wired/COUNTED (`fifa96_match_handlers.c:5235`;
ENGINE.md `N/M` census line: **55/80**, phase rows `0x02/0x10/0x17/0x18/0x19/
0x1D/0x1F/0x21/0x20/0x22`). The native half-time route adds **no** row — it uses
the existing act-1 → phase-`0x17` seam and the existing sit-1 table row. The only
change the spine needs is the **live invocation** of the already-present mode-0
else-arm; that is a producer-leg, and it does **not** by itself move
`floor(100 × 55/80) = 68` unless/until a golden is re-pinned. No engine source is
edited by this recon slice.

---

## 7. Residual legs

1. **FU227-L1 — the period-index ↔ half mapping beyond "4 = extra time."** The
   index is reset to 0 (`0x73F82`), incremented per period end (`0x8B58A`,
   `0x88D6A`), forced to 4 by the front-end (`0x38990`), and transiently set to 8
   around `0x4BE44` (`0x4BA20`/`0x4BA31`). That `[0x157AC2] == 1` is the
   **half-time** completion (period 1 → 2) is *not* statically proven here — the
   branch only proves `!= 4`; the half-number semantics need a period-end trace.
   Owner: the period/lifecycle slice (FU-213 family; FU215-L2).
2. **FU227-L2 — the half-time route is not live in the port.** The mode-0 producer
   (`FUN_00088860` mirror) is invoked only on `global_157ac2 == 4`
   (`fifa96_match_run.c:2654`), so its else arm (`:3205..3208`, native `0x888F2`)
   is fixtured-only. Wiring it live is the phase-21 spine's half-time deliverable
   (producer-only; no body, no row). Owner: the phase-21 spine (this wave) /
   FU-203 addendum.
3. **FU227-L3 — the exhaustive situation-1 producer census.** Fresh
   `get_xrefs_to 0x8A938` = 39; the two located `EAX=1` dispatchers are `0x888F2`
   (this slice) and `0x8B85D` (the phase-`0x20` terminal, FU-203 addendum). No
   per-site `EAX` census of all 39 was taken here (the "exactly two sit-1
   producers" claim is cross-cited, not proven). Owner: the situation-machine
   slice (FU-196).
4. **FU227-L4 — the `0x74244` period-end family body has no Ghidra function.**
   The live period-end invocation of `FUN_00088860` (`0x74244 CALL`) is
   disassembly-only (fresh `get_function_by_address 0x74244` = none); its full
   body and the `0x74237` session gate's own producer are unported.
   Owner: FU196-L6.
5. **FU227-L5 — the live value of `[0x14C32A]` at a period end.** The operative
   precondition (P1/P4) is `[0x14C32A] == 0`; whether the session gate is open at
   a half-time completion on a live run is a **runtime** fact (fresh
   `get_xrefs_to 0x14C32A` = 60 refs, many writers) not settled statically here.
   Owner: the session/lifecycle slice.
6. **FU227-L6 — `FUN_00073E28` (the `[0x157AAB]`/`[0x157AAE]` side writer).** The
   half-time side comes from `FUN_00073E28` (`0x73E95` WRITE, called at `0x888A4`);
   its body is cited only. Owner: FU196 §3.2 / the situation-side slice.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, 15239 symbols, min `0x10000`, max
  `.image::00174a2a`, `.image` overlay.
* `disassemble_bytes` — `0x88860` (220 B), `0x8A938` (200 B), `0x8AA60` (420 B,
  direct path + sit-1/act-4 arms + tail + table arms), `0x88DC8` (400 B, act 1),
  `0x8B560` (112 B, rollover), `0x73F70` (48 B, reset), `0x88D50` (48 B, act-0xC
  tail), `0x38980` (32 B, front-end), `0x74220` (64 B, caller gate), `0x4BA00`
  (64 B), `0x8B840` (32 B, the phase-0x20 sit-1 dispatcher).
* `disassemble_function` — `0x88860` (48 insns).
* `read_memory` — `0x8A8E0` (64 B, queue table), `0x8A904` (52 B, table-2),
  `0x110794` (140 B, act/phase slots).
* `get_xrefs_to` — `0x888FC` (**11**), `0x88DC8` (**1**, the DATA slot
  `0x1107F0`), `0x88860` (**2**), `0x8A938` (**39**), `0x157AC2` (**56**),
  `0x14C32A` (**60**), `0x157AAB` (**3**).
* `get_function_by_address` — `0x74244` = **none**.

Engine/loader source read first-hand:
`src/fifa96_engine/fifa96_match_run.c` (`:2620..2688` the phase-drive roll +
index-4 mode-0 gate, `:3140..3220` `fifa96_match_run_period_setup`),
`src/fifa96_loader/fifa96_action_handlers.c` (`:1114..1147`
`fifa96_action_phase_act` + `fifa96_action_phase_situations`),
`src/fifa96_engine/fifa96_match_handlers.c` (`:5235` the phase-`0x17` row).
Docs cross-cited: FU-196 (§2/§3/§4/§6), FU-203 (§1/§2 + addendum), FU-208, FU-213,
**FU-215** (§3/§4), `docs/ENGINE.md` (the `55/80` census line), README title (68%).
No writes beyond this slice file and the wave report; no
rename/label/comment/function/script/project save; `/FIFA96.EXE` and the ISO
untouched; no git commands.

---

## Landing note

The half-time (period-transition) route is **statically complete and distinct
from the extra-time route by exactly one instruction and one cell**: at a
phase-`0xC` period end `FUN_00088860` tests the period index `[0x157AC2]` at
`0x888B0 CMP EAX,4`; `!= 4` dispatches **situation 1** (`0x888F2`) on the fixed
controlled side, which the dispatcher (`FUN_0008A938`, direct path) routes through
the sit-1 arm `0x8AABF` → table-2 `[1] = 0x8ABAB` → **act 1 invoke-now**
(`0x8ABCF`), whose act-tail slot is the **phase-`0x17`** body `0x88DC8` (stage 0
writes the live phase **1**). The port already carries every byte of this route
(`period_setup(mode 0)` else-arm, `situation[1] = act 1`, the COUNTED phase-`0x17`
row) but **only invokes** the mode-0 producer on the `== 4` path
(`fifa96_match_run.c:2654`), so the half-time route is **fixtured-only** — the
honest phase-21 spine deliverable is to invoke the existing sit-1 producer live
on the non-4 period end. No body, no row, no census move; legs FU227-L1..L6
carried.
