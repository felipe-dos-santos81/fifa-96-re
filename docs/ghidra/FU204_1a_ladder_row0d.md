# FU-204 — the `0x1A` ladder, the row-`0x0D` producer, and the row-`0x24`/situation-0 route

Provenance: M2 phase-18 wave **R52**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; `get_current_program_info` = `FIFA96.EXE`, watcom LE 32, image base 0,
one overlay `.image`, `min_address` `0x10000`, 2706 functions). Every
address/byte/xref claim below is a fresh `read_memory` / `disassemble_bytes` /
`decompile_function` / `get_xrefs_to` / `get_function_by_address` of **this
session** unless marked *cross-cited* (then the frozen FU doc is named). The
addresses quoted are the `/FIFA96.EXE` program (runtime) addresses; LE-image
globals sit at `+0x100000` from the file offset. The alias traps are applied
(`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`; here
`[rec+0x3A]>>24` = the byte at `rec+0x3D`, `[[rec+4]+0x13]>>24` = the byte at
`[rec+4]+0x16`, `[0x15877C]`'s consumer reads the sibling byte `0x15877F`). No
writes: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; no git commands.

Scope (the R52 brief): (A) the row-`0x1A` `FUN_00077EAC` CPU ladder (FU186-L5) —
the full selector body `0x77F50..0x781E1` and the **ECX selector → ladder arm**
mapping; (B) the FU193-L3 act-4 producer (the two `FUN_000888FC(4,…)` sites);
(C) the row-`0x0D` producer (row-09 arm 1 `0x80F61`, FU178-L1) and its gate
producers (`[0x15877C]` FU178-L6, `[rec+0x3D]`, `[rec+0x24]`, `[[rec+4]+0x13]`);
(D) the row-`0x24` / phase-`0x20` **situation-0** route (FU188-L1). The question
each answers: **which become live under R47 (the formation setup) / R50 (the
situation machine)?** Result in one line: the ladder and the row-`0x0D` arm stay
**honest legs** (neither R47 nor R50 touches their hosts — the unported
`FUN_0008D8EC → FUN_000782D0 → FUN_00077EAC` record-0 pre-pass and the unported
row-09 arm 1); the **act-4 producer** flips live under **R47 → T2/R49** (the
order-matrix fill un-gates `formation_order_ok`); the **row-`0x24`/phase-`0x20`
pair** flips live under **R50 → T3** (`period_setup_live` fires situation 0 at
begin).

---

## 1. Fresh table anchors

* **Action table `0x1106E0`** (fresh `read_memory`, 180 B = 45 dwords). Slots
  this slice: `[0x04] = 0x0007E7C8`, `[0x07] = 0x000814B0`,
  `[0x09] = 0x00080A00`, `[0x0D] = 0x0008251C`, `[0x0E] = 0x00082710`,
  `[0x1A] = 0x0007662C`, `[0x1B] = 0x00076D28`, `[0x1C] = 0x00077728`,
  `[0x1F] = 0x00076380`, `[0x20] = 0x00084EEC`, `[0x24] = 0x00086510`,
  `[0x25] = 0x000880CC`. So **action row `0x1A` = `0x7662C`** (the keeper
  reposition machine, FU-186 §2.1) and **row `0x0D` = `0x8251C`** (FU-180 §3).
* **Phase table `0x110794`** (fresh, 140 B). `[0x17]=0x88DC8`, `[0x1A]=0x89620`,
  `[0x1B]=0x890EC`, `[0x1C]=0x89110`, `[0x1D]=0x89868`, `[0x20]=0x8B688` — the
  act slots (act `a` → phase `0x16+a`, base `0x1107EC = 0x110794 + 0x16*4`).
  **Phase `0x1A` (act 4) = `0x89620`**, the formation-order walk (FU-193 §3 /
  FU-197 §6) — a *different* handler from action row `0x1A` (`0x7662C`).
* **Installer `0x7D9A4`** — fresh `get_xrefs_to 0x7D9A4` = **77** (matches
  FU-174 §1 / FU-186 §3 / FU-180 §1). Contract (FU-178 §2, FU-188 §3):
  `EAX = rec`, `DX = action code`, `BX = stage byte`, `ECX→EDI = invoke-now`.

---

## 2. Part A — the row-`0x1A` `FUN_00077EAC` ladder (FU186-L5)

Fresh `get_function_by_address 0x77EAC`; fresh `get_xrefs_to 0x77EAC` = **1**
(`0x78599`, inside `FUN_000782D0` — the record-0 machine). Fresh
`disassemble_bytes 0x77EAC` (560 B, `0x77EAC..0x780DB`) + `0x780DB` (498 B,
`0x780DB..0x782CC`) + `read_memory 0x77E98` (20 B). `decompile_function 0x77EAC`
confirms the control flow.

### 2.1 Head gates (`0x77EAC..0x77F51`)

```
0x77EB7 EAX=[rec+0x8E]>>24 (=byte[rec+0x91]); CMP 0x19; JNZ 0x782C4  ; code==0x19 only
0x77ECB EAX=[[rec]+0x826]; CALL 0x741B4 (side idx)
0x77EDE AX=[0x14C1D4 + idx*2]; AND AH,0x2; JNZ 0x782C4                ; per-side gate bit 0x200
0x77EF6 CMP byte[rec+0x9A],0; JNZ 0x782C4                           ; must be free
0x77F03 EAX=[rec+0x28]; AL=[EAX]; CMP 0x54; JZ 0x782C4              ; anim row 0 != 0x54
0x77F16 CMP word[rec+0x81],0; JZ 0x77F2E
0x77F20   copy pos(+0x59,+0x5D,+0x61) -> target(+0x4D,+0x51,+0x55); exit
0x77F2E EDX=[0x157A83]; CMP EBP,EDX; JZ 0x782C4                     ; rec != controlled actor
0x77F3C TEST EDX,EDX; JZ 0x77F57; same side -> exit
```

### 2.2 Selector body (`0x77F57..0x781E1`)

Fresh bytes (this session):

```
0x77F57 MOV EDI,ESP; MOV ESI,0x15774C; MOVSD x3    ; local cam triple
0x77F61 MOV ESI,[ESP]  ; cam.x  -> abs
0x77F70 MOV EDI,[ESP+8]; cam.z  -> abs
0x77F82 MOV EDX,0x157C48; LEA EAX,[EBP+0x59]; CALL 0x795B4  ; dir/rel-heading (out ESP+0xc)
0x77F95 MOVSX EDX,SI; CMP EDX,0x420; JGE 0x77FF5    ; bVar2 gate part 1
0x77FA2 MOVSX EDX,DI; CMP EDX,0x7b0; JLE 0x77FF5    ; part 2
0x77FAD..  phase=[0x157A49]>>24; side / [0x157754] sign -> EAX=1 (bVar2)
0x77FF5 TEST AX,AX; JNZ 0x7802B
  ; --- bVar2 == false ---
  0x77FFA EAX=[EBP+0x69]>>16; CMP 0x40; JGE 0x782C4
  0x78009 CALL 0x92AC8 ; 0x78012 EAX=(rng&1)+2 ; 0x7801C CALL 0x6DC88(rec, AX)
  ; FUN_0006DC88 (fresh 0x6DC88, sole xref 0x7801C) installs CODE 7 (0x6DCB3 EDX=7) EBX=0
0x7802B XOR AL,AL; MOV byte[0x157C5C],0              ; bVar2 true
  0x78032 [rec+0x69]>>16<0x20 && [0x157750]<4 && [0x1577bc]>>16<0xA
  0x7805B  MOV ECX,0x1 ; JMP 0x781DE                 ; INSIDE-BOX  -> ECX=1  (copy cam)
  0x78065  EAX = 2*byte[[rec+4]+0xE] + 0x30           ; (alias +0xB>>24)
  0x7807D  [0x157750]!=0 -> 0x780E2
  0x78086  [0x157A83]==0 -> 0x780E2
  0x7808E  side([0x157A83])==side(rec) -> 0x780E2
  0x780A5  copy cam->target; target.x += 2*[[0x157A83]+0x71]>>16;
           target.z += 2*[[0x157A83]+0x73]>>16; byte[0x157C5C]=1
  0x780D2  MOV ECX,0x4 ; JMP 0x781E1                 ; OVERTAKE   -> ECX=4
  0x780E2  bVar1 = byte[0x10E184 + (([ESP+0xa]>>0x14)<<3 + [0x157C4C]>>5)]
           AX = word[0x10E1C4 + bVar1*2]; ESI=[0x157C44]; ... side/[0x157821] gates
  0x781A1  EAX=[ESP+0xa]>>16
  0x781AD  >= 0xf0 -> ECX=0 (exit)
  0x781BA  >= 0x80 -> MOV ECX,0x4 ; JMP 0x781E1      ; LANE       -> ECX=4
  0x781C1  [0x157C4C]>=0x100 -> 0x781CD MOV ECX,0x5 ; JMP 0x781E1  ; LANE -> ECX=5
  0x781D4  ESI=0x157C48; EDI=&[EBP+0x4D]; MOV ECX,EDX(=bVar1); MOVSD x3
                                                      ; LANE       -> ECX=bVar1
0x781E1 TEST CX,CX; JZ 0x782C4                       ; ECX==0 -> out
0x781EC CALL 0x765C4 (EAX=EBP)                       ; tracked-teammate predicate
0x781F1 TEST AX,AX; JZ 0x78205
0x781F6   MOV ECX,0x1 ; MOV EDX,0x4 ; JMP 0x782BB     ; -> install CODE 4
0x78205 DEC ECX; CMP CX,0x4; JA 0x782C4
0x78210 XOR EAX,EAX; MOV AX,CX
0x78215 JMP dword CS:[EAX*4 + 0x77E98]               ; the ladder table
```

Fresh `read_memory 0x77E98` (20 B) = `6d820700 45820700 1d820700 94820700
b1820700` = **`{0x7826D, 0x78245, 0x7821D, 0x78294, 0x782B1}`**.

### 2.3 The five ladder arms (`0x7821D..0x782C4`, fresh bytes)

| index | site | arm bytes | writes | install (`EDX`) |
|---|---|---|---|---|
| `[0]` `0x7826D` | `0x78277 ba 1a 00 00 00`; `0x7827E MOV [0x157C2C],0x2C` (`b8 2c 00 00 00`) | `[0x157C2C]=0x2C` | **0x1A** (`0x78285 CALL 0x7D9A4`) |
| `[1]` `0x78245` | `0x7824F ba 1a 00 00 00`; `0x78258 MOV [0x157C2C],0x2D` (`bf 2d 00 00 00`) | `[0x157C2C]=0x2D` | **0x1A** (`0x7825E`) |
| `[2]` `0x7821D` | `0x78227 ba 1a 00 00 00`; `0x78230 MOV [0x157C2C],0x2E` (`be 2e 00 00 00`) | `[0x157C2C]=0x2E` | **0x1A** (`0x78236`) |
| `[3]` `0x78294` | `0x78299 ba 1b 00 00 00` | — | **0x1B** (`0x782A2`) |
| `[4]` `0x782B1` | `0x782B6 ba 1c 00 00 00`; falls to `0x782BB` | — | **0x1C** (`0x782BF`) |
| shared tail `0x782BB` | entered from `0x78200` (`EDX=4`) **or** fall-through from arm `[4]` | — | **4** (`0x782BF`) |

Every arm sets `ECX=1` (invoke-now) and `EBX=0`, `EAX=EBP`. Fresh
`get_xrefs_to 0x7D9A4` = **77** and the `FUN_00077eac`-sourced call sites in it
are exactly `0x78236`, `0x7825E`, `0x78285`, `0x782A2`, `0x782BF`.

### 2.4 ECX → arm mapping (the FU186-L5 answer)

`0x78205 DEC ECX; CMP CX,4; JA exit` then `JMP [0x77E98 + (ECX-1)*4]`, so
`EAX = ECX-1` indexes the table:

| selector `ECX` | set by | arm | outcome |
|---|---|---|---|
| **1** | inside-box `0x7805B` (also `LAB_0x781DE` with `ECX=1`) | `[0]` | `[0x157C2C]=0x2C`, **install 0x1A** invoke-now |
| **2** | (lane `0x781D4` with `bVar1==2`) | `[1]` | `[0x157C2C]=0x2D`, **install 0x1A** invoke-now |
| **3** | (lane `0x781D4` with `bVar1==3`) | `[2]` | `[0x157C2C]=0x2E`, **install 0x1A** invoke-now |
| **4** | overtake `0x780D2`; lane `0x781BA` | `[3]` | **install 0x1B** invoke-now |
| **5** | lane `0x781CD` | `[4]` | **install 0x1C** invoke-now |
| **bVar1** (table) | lane `0x781D4` (`ECX=EDX=bVar1`) | `[bVar1-1]` | the `0x10E184`-table byte picks the arm; only `1..5` survive `DEC/CMP/JA` |
| any `≠0`, `FUN_000765C4` nonzero | `0x781F6` | shared `0x782BB` | **install action code 4** invoke-now |

The `0x10E184` (`byte`) / `0x10E1C4` (word) / `0x10E1CE` (dword) tables and the
`0x157C44` / `0x157C48` / `0x157C4C` / `0x1577F4`/`0x1577F8` lane cells are cited
by address, not decomposed (leg FU204-L1). The bVar2-false path installs
**action code 7** via `FUN_0006DC88` (`0x6DCB3 EDX=7`, `EBX=0`, deferred).

### 2.5 Erratum (quoted) — FU-186 §1.1/§3 mis-attributes the `0x782BB` target

> **FU-186 §1.1:** "`0x781F6 ECX=1; EDX=4; JMP 0x782BB` ; JMP -> install 0x1C",
> and **FU-186 §3 table:** "`(JMP target) 0x782BB | entered from 0x78200 (the
> `0x765C4`-nonzero arm) | — | **0x1C**".

**Corrected (fresh `disassemble_bytes 0x781E1` this session):** the JMP target
`0x782BB` is a **shared tail**; it installs whatever `EDX` holds. From arm `[4]`
(`0x782B1 MOV EDX,0x1c`) it is 0x1C; from the tracked-teammate arm
`0x781FB = ba 04 00 00 00` (`MOV EDX,0x4`) it is **action code 4**, not 0x1C.
`decompile_function 0x77EAC` agrees (`else { sVar6 = 4; }` →
`FUN_0007d9a4(param_1,4,0,1)`). **The `FUN_00077EAC` install set is
`{0x1A ×3, 0x1B, 0x1C, 4, 7}`, not `{0x1A ×3, 0x1B, 0x1C}`.** This is the same
shared-tail/adjacency class as the FU-186 §1.1 `0x773DC` erratum; it does not
change the row-`0x1A` installer census (the three 0x1A sites are
`0x78227`/`0x7824F`/`0x78277`), but the "tracked-teammate → 0x1C" claim is
wrong.

### 2.6 Reachability

Fresh `get_xrefs_to 0x77EAC` = **1** (`0x78599` in `FUN_000782D0`); fresh
`get_xrefs_to 0x782D0` = 1 (cross-cited FU-186 §1.2 / FU-180 §4.2: `0x8DB35`
in `FUN_0008D8EC`). The engine models only `FUN_0008D8EC`'s per-team walk
(`fifa96_match_entities.c:676` records the `0x782D0` entry for record 0), **not**
the record-0 machine's `CALL 0x77EAC` keeper CPU decision. So the ladder — row
`0x1A`'s **only** installer — is unported. Row `0x1A` stays uncounted
(`fifa96_match_handlers.c:5015 {0x1A, NULL, …}`), owner **FU186-L5**.

---

## 3. Part B — the FU193-L3 act-4 producer

Fresh `get_xrefs_to 0x888FC` = **11** (the full act-invoker set, FU-175 §1 /
FU-197 §7). The two act-4 sites (fresh `disassemble_bytes`):

```
0x8AB57 (sit-2..4 arm 0x8AB08)       0x88EE8 (act-1 body stage 1)
0x8AB08 EAX=0x1588A4; CALL 0x8C974   0x88ECB/J: phase!=[0x157A49]>>24==1 &&
0x8AB12 EAX=0x1590D9; CALL 0x8C974     ([0x157B8E]!=0 || [0x157B8F]!=0)
0x8AB1C EAX=[0x157A49]>>24; CMP 1     0x88EDC MOV EBX,1; MOV EAX,4; XOR EDX,EDX
0x8AB27 JZ 0x8AB63 (normal table)     0x88EE8 CALL 0x888FC
0x8AB29/0x8AB32 [0x157B8E]|[0x157B8F]
0x8AB3B EAX=[ESP](side); EBX=1; EDX=0
0x8AB45 MOV [0x158820],AX
0x8AB50 MOV word[0x15881E],CX
0x8AB57 CALL 0x888FC                   ; act 4, stage 0, BX=1 invoke-now
```

Both invoke `FUN_000888FC(EAX=4, DL=0, BX=1)` → act 4 = phase `0x1A` =
handler `0x89620` (phase table `[0x1A]`). Fresh `get_xrefs_to 0x89620` = **1**
(DATA from `0x1107FC` = `0x110794 + 0x1A*4`, the phase slot). The act-4/phase-0x1A
handler `0x89620` is the formation-order walk that installs **action row `0x17`**
(`0x89740`), not row `0x1A` (FU-197 §6).

*Fresh cross-check (byte-for-byte against FU-197 §2/§7):* the sit-0 arm
`0x8AB82` is `MOV EBX,1; MOV EAX,0xA; XOR EDX,EDX; CALL 0x888FC; …; MOV EAX,0x11;
CALL 0x740A0` — act `0xA` then phase `0x11` (the situation-0 route, §5).

**Verdict:** the act-4 producer is **not** live today — the arm's gate
`formation_order_ok` is 0 (`fifa96_match_run.c:2966`; the engine models the
sit-2..4 arm at `match_run.c:2944` gated dormant). It becomes live under **R47**
(the formation/line-up setup fills `formation_order`/`formation_current` and sets
`formation_order_ok=1`), then **R49/T2** lands the arm. Owner: FU193-L3 /
FU197-L4, R49/T2.

---

## 4. Part C — the row-`0x0D` producer (row-09 arm 1) and its gates

Fresh `get_xrefs_to 0x8251C` = **1** (DATA from `0x110714` = `0x1106E0 + 0x0D*4`,
the action-table slot). The row's **only** installer is inside the row-09 body
`0x80A00` (FU-178 §4.1, FU-180 §1.2): the arm-1 stage (`0x80BFB..0x80FCB`)
installs **0x0D at `0x80F61`** (`ba 0d 00 00 00`, `EBX=0` install-only) and the
sibling **0x0C at `0x80EFA`**. Fresh `disassemble_bytes 0x80BF0` (224 B) +
`0x80CCC` (768 B) derive the whole arm.

### 4.1 The arm-1 gate chain (fresh bytes)

```
0x80BFB CMP byte[0x15877C],0 ; JNZ 0x80C13     ; install path needs [0x15877C]==0
0x80C04 EAX=[EBP+0x28]; AL=[EAX]; CMP AL,0x11
0x80C11 JZ 0x80C35                            ; template byte [[rec+0x28]]==0x11
0x80C13 (stage++ -> return)                   ; else no install
0x80C35 EAX=[EBP+0x3A]; SAR 0x18              ; alias: byte [rec+0x3D]
0x80C3B CMP EAX,0x2 ; JNZ 0x8105C             ; [rec+0x3D]==2 required
0x80C44.. ESI = [[team]+0x7A6][+0x7B2]        ; the ball-slot record
0x80C92 CMP [0x157750],0x20 ; JGE 0x80E13
0x80C9F AX=[EBP+0x6b]; CMP AX,[EBP+0x77]; JG 0x80E13
0x80CAD EAX=3*BX + 0x40; CMP [rec+0x69]>>16,EAX; JGE 0x80E13
   ; BX = (int8)[[rec+4]+0xC] + (int8)[[rec+4]+0x16]  (0x80C61..0x80C90)
0x80E13 TEST ESI,ESI; JZ 0x80FA4 ; CMP byte[ESI+0x8D],0; JZ 0x80FA4
0x80E32 CALL 0x8DCD4(EBP,ESI,ESP+8)            ; dist stage EBP<->ESI
0x80E3E CMP [ESP+6]>>16,0x40; JG 0x8105C      ; distance <= 0x40
0x80E57 CALL 0xCD474 ; angle fold ; CMP |..|,0x55; JGE 0x8105C
0x80E81 heading(ESI)-heading(EBP) fold; CMP EDX,0x80; JGE 0x80F21
 ; --- <0x80 branch (0x80EAB): install 0x0C ---
 0x80EAB EDX=byte[[rec+4]+0x16] (alias [rec+4]+0x13>>24); EAX=3*EDX
 0x80EC3 EAX = (EAX<<2)+angle ; SETL ; RNG 0x92AC8 ; CALL 0x8A3FC
 0x80EFA CALL 0x7D9A4   ; *** install 0x0C on ESI (ECX=1 invoke-now) ***
 ; --- >=0x80 branch (0x80F21): install 0x0D ---
 0x80F21 EDX=byte[[rec+4]+0x16]; EAX=3*EDX; RNG 0x92AC8; EAX &= 0x3F; CMP EAX,EDX
 0x80F3F JC 0x80F4B                             ; rng&0x3f < 3*byte -> install
 0x80F41 CMP EBP,[ESI+0x24]; JZ 0x80F4B         ; already paired (rec -> partner)
 0x80F46 CMP ESI,[EBP+0x24]; JNZ 0x80F71        ; or partner -> rec; else no install
 0x80F4B EBX=4; EAX=0x5D; EDX=EBP; ECX=1; CALL 0x8F188   ; event 0x5D
 0x80F61 MOV EDX,0xD; MOV EAX,ESI; XOR EBX,EBX; CALL 0x7D9A4
                                                ; *** install 0x0D on ESI ***
 0x80F71 EBX=4; EAX=0x60; EDX=ESI; CALL 0x8F188 ; event 0x60 (no install)
0x80FA4 CMP byte[0x15877C],0; JZ 0x8105C ; EAX=0x5E; CALL 0x8F188 ; arm tail
```

So the install path is gated by **`[0x15877C]==0`** AND **`[[rec+0x28]]==0x11`**
AND **`[rec+0x3D]==2`** (the alias `[rec+0x3A]>>24`), then the dual-record
geometry (`0x8DCD4` distance `≤0x40`, angle `<0x55`, heading diff `≥0x80`), then
either the RNG (`rng&0x3F < 3*byte[[rec+4]+0x16]`) or the pairing test
(`[rec+0x24]`), with two `0x8F188` event emits. Installs are install-only
(`EBX=0`), and target the **partner record `ESI`**, not the dispatcher `EBP`.

### 4.2 The gate producers

* **`[0x15877C]` (FU178-L6).** Fresh `get_xrefs_to 0x15877C` = **7**: writes
  `0x80BD3` (row-09 arm 0, clear), `0x80DCC` (arm 1, set), `0x80FD1` (arm 2,
  clear); reads `0x80BFB`/`0x80FA4` (arm 1) and `0x839AB`/`0x83AE0`
  (`FUN_00083428`, the presentation consumer). **The flag is
  self-contained:** its producers are row-09's own arm 0 / arm 2; nothing
  outside row 09 writes it. (Note: the `FUN_00083428` consumer at `0x839AB` does
  `MOV EDX,[0x15877C]; SAR EDX,0x18` — it reads the **sibling byte `0x15877F`**
  of the same dword, an alias the arm bytes never touch; FU178-L6's "shared
  flag" semantics stay a leg.) **Not produced by R47/R50.**
* **`[rec+0x3D]` (byte via `[rec+0x3A]>>24`).** The FU-84 animation-frame index
  (entity header: "frame `+0x3D` … staged by `FUN_00036C70` at
  `0x36D44`/`0x36D4F`; OL-80"). Producer = the unported animation-frame
  machinery, not R47/R50. (The engine models `frame` in the pool entity but
  stages it from the FU-84 selector, not `FUN_00036C70`.)
* **`[rec+0x24]`.** A partner/pairing record pointer (fresh reads at
  `0x80F41`/`0x80F46`; not named in `fifa96_match_entities.h`). Its native
  producer was not located this slice (**leg FU204-L2**).
* **`[[rec+4]+0x13]` → byte `[[rec+4]+0x16]`** (the alias FU178-L7 flags). A
  `[rec+4]`-descriptor byte (the `FUN_00011620`-family descriptor, FU167/FU168
  "descriptor byte" class). The `[rec+4]` second-pointer binding and this byte's
  producer are not derived (**leg FU204-L2**).

### 4.3 Reachability

Row-09's `fn` is the **stage-0-only** landing (`fifa96_match_action_09`,
`handlers.c:4131` parks at `mr->match09_leg_stage` for stages 1..3); arm 1 is
FU178-L1 (unported). Row `0x0D`'s slot stays NULL
(`handlers.c:4990 {0x0D, NULL, …}`); the body was ported separately as
`fifa96_action_sequence_0d_step` (FU-180 §9). **Neither R47 nor R50 un-gates
this** — the host is the row-09 body, not the formation setup or the situation
machine. Owner: **FU178-L1**.

---

## 5. Part D — the row-`0x24`/phase-`0x20` situation-0 route (FU188-L1)

Fresh `disassemble_bytes 0x4B090` (80 B) confirms the native situation-0
producer inside `FUN_0004b02c`:

```
0x4B095 CALL 0x74034 ; CALL 0x7417C
0x4B09F XOR EBX,EBX; XOR EDX,EDX; XOR EAX,EAX
0x4B0A5 CALL 0x8A938                 ; *** situation 0, side 0, BX=0 (direct) ***
0x4B0AA.. CALL 0x8C418 x2            ; both team refresh
0x4B0BC MOV CX,[0x14C1D4]; MOV AX,[0x14C1D6]; OR EAX,ECX; TEST AH,1; JZ 0x4B0E5
0x4B0D0 MOV EAX,8; XOR EBX,EBX; XOR EDX,EDX; CALL 0x8A938   ; situation 8
0x4B0DE MOV byte[0x157AC2],4
```

Fresh `get_xrefs_to 0x4B02C` = **2** (`0x49415`, `0x4950A`, both
`FUN_000493a0`); fresh `get_xrefs_to 0x493A0` = **3** front-end callers
(`0x180AA` `FUN_00018014`, `0x18359` `FUN_00018108`, `0x18819` `FUN_00018680`).
The situation-0 dispatch flows: `FUN_0008A938(0,0,0)` → dispatcher direct path
(`0x8A944 TEST AX,AX; JZ 0x8AA7B`) → table-2 sit-0 arm `0x8AB82` (fresh):
`MOV EBX,1; MOV EAX,0xA; XOR EDX,EDX; CALL 0x888FC` → **act 0xA** = phase
`0x20` = `0x8B688` (fresh `get_xrefs_to 0x8B688` = 1, DATA from `0x110814` =
`0x110794 + 0x20*4`), then `MOV EAX,0x11; CALL 0x740A0` (phase 0x11).

The phase-0x20 body installs **action row 0x24** on all 22 records (FU-188 §3).
The engine already carries the whole seam: `fifa96_match_run_period_setup(mr,1)`
is the `FUN_0004b02c` mirror (`match_run.c:2989`), the phase-0x20 body is
`fifa96_match_action_phase_20` (`match_run.c:5617`), the row-0x24 body is
`fifa96_match_action_24` — but **all three are gated off today**: `begin` calls
the producer only `if (mr->period_setup_live)` (`match_run.c:2244`, default 0),
and the phase/action slots stay NULL (`handlers.c:5035`/`:5120`).

**Verdict:** the pair becomes **producer-live under R50** (the situation machine
wires `period_setup_live`, firing situation 0 at `begin`) **+ T3** (registers the
phase-0x20 body and sets `fifa96_match_action_table[0x24]`). Not live today.
Owner: **FU188-L1** / FU196-L8, R50/T3.

---

## 6. Part E — what becomes live under R47 / R50

| item | host / installer | live today | R47 (formation setup) | R50 (situation machine) | owner |
|---|---|---|---|---|---|
| **act-4 producer** (`0x8AB57` sit-2..4 arm; `0x88EE8` act-1 hand-off) → phase `0x1A` → **row `0x17`** | `FUN_0008C974` ×2 + the `[0x157B8E]/[0x157B8F]` gate | **no** (`formation_order_ok`=0) | **YES** — the matrix fill un-gates the arm | no | FU193-L3 / FU197-L4 → R49/T2 |
| **row-`0x24` + phase-`0x20`** | situation 0 (`0x4B0A5`) → act `0xA` (`0x8B688`) | **no** (`period_setup_live`=0) | no | **YES** — `period_setup_live` fires situation 0 at begin | FU188-L1 / FU196-L8 → R50/T3 |
| **row-`0x1A` ladder** (`0x78227`/`0x7824F`/`0x78277`, +0x1B/0x1C/4/7) | `FUN_0008D8EC → FUN_000782D0 → FUN_00077EAC` | **no** | no | no | FU186-L5 |
| **row-`0x0D`** (`0x80F61`) + 0x0C | row-09 arm 1 `0x80BFB..0x80FCB` | **no** | no | no | FU178-L1 |

Neither R47 nor R50 touches the ladder's host (the unported record-0 keeper CPU
pre-pass) or the row-09 arm-1 host. The rows they **do** move are different from
the ones this slice's ladder/row-0x0D derive:

* **R47 → T2/R49**: `formation_order_ok` 1 → the sit-2..4 act-4 arm runs →
  phase `0x1A` → **row `0x17`** installs (`0x89740`). (Row `0x1A` is **not** in
  this chain — the act-4/phase-0x1A handler `0x89620` is a namesake, not the
  action row.)
* **R50 → T3**: situation 0 → act `0xA` → **phase `0x20` + row `0x24`**;
  situation 8 → act 7 → phase `0x1D` + row `0x20` (FU-189 §3/FU-194).

---

## 7. T2 / T3 / T5 wiring recommendations

* **T2 (rows `0x17`/`0x1A`).** Wire **row `0x17`** through the act-4 arm
  (FU-197 §9 T4) — that is R47's un-gate, **not** this slice's. Leave
  `fifa96_match_action_table[0x1A]` **NULL**: row `0x1A`'s only installer is the
  `FUN_00077EAC` ladder on the unported `FUN_0008D8EC → FUN_000782D0` pre-pass
  (FU186-L5). Do **not** wire row `0x1A` standalone (the rows-27/29/2C
  precedent). The ladder derivation here (the ECX→arm map + the code-4/code-7
  erratum) is the spec for the eventual port, but the pre-pass is a separate,
  large slice.
* **T3 (situation machine).** This is where **row `0x24` + phase `0x20`** (and
  row `0x20` + phase `0x1D`, FU-196 §5/FU-189 §10) become live: flip
  `period_setup_live` on the live `begin` path and register
  `fifa96_match_action_phase_20` + `fifa96_match_action_table[0x24]` in the same
  commit as `fifa96_match_run_period_setup` (the R2 rule: no count before the
  full route lands; the FU-196 §4 tape caveat). The situation-0 producer here is
  `FUN_0004b02c(1)` `0x4B0A5`; the engine's `begin` is its analog.
* **T5 (remaining bound).** Row **`0x0D`** (FU178-L1) and row **`0x1A`**
  (FU186-L5) stay **honest numbered legs** — no fake producer. If row-09 arm 1
  lands later, `fifa96_match_action_0D`'s slot (`handlers.c:4990`) becomes
  non-NULL and the row counts; the row body already exists as
  `fifa96_action_sequence_0d_step`. The row-`0x24`/phase-`0x20` pair is T3's, not
  T5's.

---

## 8. Numbered legs

1. **FU204-L1 — the ladder's selector tables/cells.** The `0x10E184` (byte) /
   `0x10E1C4` (word) / `0x10E1CE` (dword) tables and the `0x157C44` / `0x157C48`
   / `0x157C4C` / `0x1577F4`/`0x1577F8` lane cells (the `0x780E2..0x781A1` gate
   and the `bVar1` lane selector) are cited by address only; the `bVar1` values
   that reach each ladder arm are dynamic. Carries FU186-L5's table half.
2. **FU204-L2 — the row-`0x0D` partner/descriptor producers.** `[rec+0x24]` (the
   pairing record pointer at `0x80F41`/`0x80F46`) and the `[rec+4]` descriptor
   byte `[[rec+4]+0x16]` (the RNG threshold) have producers not located this
   slice; the `[rec+4]` binding itself is a FU167/FU168-class leg.
3. **FU204-L3 — `FUN_0006DC88` (the ladder's code-7 path).** The bVar2-false arm
   installs action code 7 (`0x6DCB3`) and stages `[0x158743]` via `FUN_0006DBCC`
   (the `(rng&1)+2` argument); the helper body and the code-7 row are cited only.
   Carries FU186-L5's corner path.
4. **FU204-L4 — the `0x8F188` event ring `0x5D`/`0x60`/`0x5E`.** The row-09
   arm-1 events (FU178 §4) and `0x8A3FC` are cited by address; semantics not
   decomposed here (the FU178/FU180 sink class).
5. **FU204-L5 — the `0x15877C` sibling byte `0x15877F`.** The `FUN_00083428`
   consumer reads `dword[0x15877C]>>24` = byte `0x15877F`, a different byte from
   the row-09 sequence flag at `0x15877C`; the shared-flag semantics (FU178-L6)
   are not resolved.
6. **FU204-L6 — the record-0 keeper CPU pre-pass.** `FUN_0008D8EC` (driver
   pre-pass) + `FUN_000782D0` (record-0 machine, `0x782D0..0x785D0`) + the
   `CALL 0x77EAC` at `0x78599` are unported; the engine models the walk the
   FU-186 §1.2/§1.3 bodies describe only partially. This is the gate on wiring
   any ladder install. Carries FU186-L6/L7 (FU180-L4/L5).

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` (`FIFA96.EXE`, watcom LE 32, `min_address` `0x10000`).
* `read_memory` — `0x1106E0` (180 B, the action table), `0x110794` (140 B, the
  phase table), `0x77E98` (20 B, the ladder table).
* `get_xrefs_to` — `0x77EAC` (1), `0x7D9A4` (77), `0x89620` (1 DATA from
  `0x1107FC`), `0x8B688` (1 DATA from `0x110814`), `0x8251C` (1 DATA from
  `0x110714`), `0x15877C` (7), `0x4B02C` (2), `0x493A0` (3), `0x888FC` (11),
  `0x6DC88` (1).
* `disassemble_bytes` — `0x77EAC` (560 B), `0x780DB` (498 B), `0x80BF0` (224 B,
  the row-09 arm-1 head), `0x80CCC` (768 B, the row-09 arm-1 body incl.
  `0x80F61`/`0x80EFA`), `0x4B090` (80 B), `0x8AB00` (112 B, the sit-2..4 arm),
  `0x8AB70` (64 B, the sit-0 arm), `0x88EC0` (80 B, the act-1 hand-off),
  `0x83990` (96 B, the `FUN_00083428` `[0x15877C]` consumer), `0x6DC88` (64 B).
* `decompile_function` — `0x77EAC`.
* `get_function_by_address` — `0x83428` (`0x83428..0x83B1D`).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(3810..3920, the row-1A body; 4990/5015/5035/5082/5108/5114/5120, the
action/phase slots), `fifa96_match_run.c` (2855..2966, the act-4 arm +
`formation_order_ok`; 2989.. the `FUN_0004b02c` mirror; 5617.. the phase-0x20
body; 2068/2193/2241.. the `period_setup_live` gate),
`fifa96_match_entities.c` (640..700, the `0x782D0` record-0 entry),
`include/fifa96_engine/fifa96_match_entities.h` (the record layout),
`include/fifa96_engine/fifa96_match_run.h` (787..810, the formation/period
cells).

Docs cross-cited: FU-175 (§1/§4/§7), FU-178 (§4.1/§7/L1/L6/L7), FU-180
(§1.2/§3/§9), FU-186 (§1.1/§1.2/§1.3/§2.1/§3/§4.1/L5/L6/L7), FU-188
(§2/§3/§4/§6/§10), FU-189 (§3/§10), FU-193 (§3/§5/§6), FU-196 (§3/§4/§5), FU-197
(§2/§4/§6/§7/§9), FU-190 (§9). No writes beyond this slice file and the
workspace report.

---

## 10. Erratum (phase-18 T2 / R49, 2026-10-12) — the act-4 producer does **not** flip live under R47

**Corrected (quoted).** This slice's Part B verdict (§3) says the act-4 producer
"becomes live under **R47** ... then **R49/T2** lands the arm"; the §6 table
marks the act-4 producer row "R47 (formation setup) = **YES — the matrix fill
un-gates the arm**"; and the §7 T2 recommendation says "Wire **row `0x17`**
through the act-4 arm ... that is R47's un-gate". **All three are superseded.**

R49/FU-201 §4/§6 (fresh, read-only) proves the arm gate
`[0x157A4C] != 1 && ([0x157B8E] | [0x157B8F]) != 0` is a **permutation
residual**: `FUN_0008C974` selection-sorts the current order `0x157BD2` onto the
desired `0x14C360` and counts the positions it cannot reconcile, and the
match-init reset `FUN_00073EE0` copies desired→current. Therefore a faithful
desired-order fill leaves `current == desired` and the residual **0** — the fill
is *necessary but not sufficient* to trip the arm. Un-gating
`formation_order_ok` on the fill alone is a **no-op** (current copied) or, with
current left zeroed, a **fabricated residual / fake producer** (R45). The genuine
trigger is a runtime **non-permutation** of current vs desired (the
substitution/line-up-divergence transient, FU201-L1), which no native routine in
the fresh write census performs and no engine source emits.

**Consequence under R47/T2:** row `0x17` and phase `0x1A` stay **NULL/uncounted
(`+0`)** — honest numbered legs (FU201-L1/L4/L6, FU181-L1); no fake producer is
installed. The rest of this slice's T2/T3/T5 recs **stand**: row `0x1A`'s only
installer is the unported `FUN_00077EAC` ladder (§2.6, FU186-L5); row `0x0D`'s
only installer is row-09 arm 1 (§4.3, FU178-L1); the row-`0x24`/phase-`0x20`
pair flips live only under **R50 → T3** (§5), not here.
