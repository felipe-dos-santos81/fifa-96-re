# FU-186 — the keeper reposition/lunge chain → rows 0x1A / 0x1C (+ row-0x1B legs)

Provenance: M2 phase-16 wave **R34**, 2026-10-12, read-only. Authoritative program
**`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program argument;
`get_current_program_info` = `FIFA96.EXE`, watcom LE 32, image base 0, one overlay
`.image`; plain-hex reads resolve to the default physical space, the LE flat
convention of FU-180/FU-182). Every address/byte below is a fresh `read_memory` /
`disassemble_bytes` / `get_xrefs_to` / `get_function_by_address` /
`search_instructions` of this session unless marked *cross-cited* (the frozen doc
is named). Engine/loader source read first-hand. No writes: no rename, comment,
label, function, script or project save; `/FIFA96.EXE` untouched; no git commands.
Alias traps applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`; here
`[rec+0x8E]>>24` = the type byte at `+0x91`, `[rec+0x8B]>>24` = the sector at
`+0x8E`, `[rec+0x89]` is a dword timer, `[0x157A4A]>>24` = phase).

Scope (bounded, per the R34 brief): the keeper **reposition/lunge chain**
(`FUN_00077EAC`, `FUN_000782D0`, `FUN_0008D8EC`, `FUN_00076130`, `FUN_00070C08`),
the rows **0x1A**/**0x1C** bodies whose installers that chain exposes, and the
still-open row-0x1B sine/steering legs (`0x14E04`/`0x76B28`, FU180-L1/L2/L3). The
question: for each install to *execute honestly* — a native installer invocation
the engine already reaches — versus the deep chain remainder that becomes a
numbered leg.

Result in one line: the true 0x1A installers are **only** the three `FUN_00077EAC`
arms (`0x78227`/`0x7824F`/`0x78277`); the true 0x1C installers are `FUN_00077EAC`
`0x782B6` **and** `FUN_00076130` `0x76291`/`0x7634B`; **row 0x1C already has a live
engine installer** (row-0x1F's ported tail drains `fifa96_keeper_input_decide`'s
install `0x1C`), so wiring `fifa96_match_action_1C` moves the census; **row 0x1A
does not** (its only installer is the unported CPU ladder), so its body ports now
but stays uncounted until the ladder lands. Four FU-182 §5 “installer” sites are
**false positives** (argument immediates to other calls, plus a constant getter).

---

## 1. The chain — fresh functions, xrefs and the key arms

### 1.1 `FUN_00077EAC` — the keeper CPU decision (`0x77EAC..0x782CD`)

Fresh `get_function_by_address 0x77EAC`; fresh `get_xrefs_to 0x77EAC` = **1**
(`0x78599`, in `FUN_000782D0`). Fresh `disassemble_bytes 0x77EAC` (160 B) — the
gate head:

```
0x77EB5  MOV EBP,EAX                          ; EBP = rec
0x77EB7  EAX = [rec+0x8E] >> 24              ; type8 = byte[rec+0x91]
0x77EC2  CMP EAX, 0x19; JNZ 0x782C4          ; only type 0x19 proceeds
0x77ECB  AL = team[EBP[0]+0x826]; CALL 0x741B4(side)
0x77EDE  AX = word[0x14C1D4 + side*2]; AND AH, 0x2    ; per-side gate word
0x77EF0  JNZ 0x782C4
0x77EF6  CMP byte[rec+0x9A], 0; JNZ 0x782C4  ; must be inactive
0x77F03..0x77F10  byte[[rec+0x28]] == 0x54 -> 0x782C4
0x77F16  word[rec+0x81] == 0 ? (build target) : copy pos -> target, tail
```

The body then builds a **lateral direction selector `ECX`** and falls into the
ladder tail `0x781E1..0x78205`. Fresh `disassemble_bytes 0x77F50`/`0x7802B`/
`0x7816A` (three windows) give the selector sites:

```
0x7805B  ECX=1; copy cam(0x15774C) -> target; JMP 0x781DE (MOVSD)   ; inside-box arm
0x780D2  ECX=4; byte[0x157C5C]=1; JMP 0x781E1                      ; overtake arm
0x780DC  ECX=4 (0x781BA); 0x781CD ECX=5; 0x781DC ECX=EDX           ; lane arms
0x781E1  TEST CX,CX; JZ 0x782C4
0x781EC  CALL 0x765C4 (EAX=EBP)              ; FUN_000765C4 tracked-teammate predicate
0x781F1  TEST AX,AX; JZ 0x78205
0x781F6  ECX=1; EDX=4; JMP 0x782BB           ; JMP -> install 0x1C
0x78205  DEC ECX; CMP CX,4; JA 0x782C4
0x78215  JMP dword CS:[EAX*4 + 0x77E98]      ; the ladder table
```

Fresh `read_memory 0x77E98` (20 B) = the ladder table
`{0x0007826D, 0x00078245, 0x0007821D, 0x00078294, 0x000782B1}`. Fresh
`disassemble_bytes 0x78200` (256 B) — the five arms, each `ECX=1` (invoke-now),
`EBX=0` (not staged), `EAX=EBP`, `CALL 0x7D9A4`:

| arm (index) | site | writes | install `EDX` |
|---|---|---|---|
| `[0]` `0x7826D` | `0x78277` `ba 1a 00 00 00`; `MOV [0x157C2C]=0x2C` (`b8 2c 00 00 00`) | `[0x157C2C]=0x2C` | **0x1A** |
| `[1]` `0x78245` | `0x7824F` `ba 1a 00 00 00`; `MOV [0x157C2C]=0x2D` (`bf 2d 00 00 00`) | `[0x157C2C]=0x2D` | **0x1A** |
| `[2]` `0x7821D` | `0x78227` `ba 1a 00 00 00`; `MOV [0x157C2C]=0x2E` (`be 2e 00 00 00`) | `[0x157C2C]=0x2E` | **0x1A** |
| `[3]` `0x78294` | `0x78299` `ba 1b 00 00 00` | — | **0x1B** |
| `[4]` `0x782B1` | `0x782B6` `ba 1c 00 00 00` | — | **0x1C** |
| (JMP target) `0x782BB` | entered from `0x78200` (the `0x765C4`-nonzero arm) | — | **0x1C** |

Every arm's call bytes are `CALL 0x7D9A4` (`e8`-relative to `0x78236`/`0x7825E`/
`0x78285`/`0x782A2`/`0x782BF` — all in the fresh §3 `get_xrefs_to 0x7D9A4` = **77**
list). So the CPU ladder's **install set is {0x1A ×3, 0x1B, 0x1C}**, all
invoke-now, and it stamps `[0x157C2C] ∈ {0x2C,0x2D,0x2E}` before the 0x1A arms —
the same global the row-0x1A stage bodies read back (§4). The fresh `MOV EDX,0x1a`
search (§3) confirms `0x78227/0x7824F/0x78277` are these three sites (plus the
false positives).

> **Erratum (FU-182 §5).** FU-182 listed the 0x1A installer group as
> “`0x773DC`, `FUN_00077eac` `0x78227/0x7824F/0x78277`, `FUN_0009e420` `0x9E665`”
> and the 0x1C group as “`FUN_00070c08` `0x70DAF`, `FUN_00076130` `0x76291`,
> `FUN_00077eac` `0x782B6`, `0x80814`”. **Corrected this session (fresh
> `disassemble_bytes`):** the only `CALL 0x7D9A4` sites in those groups are
> `0x78227`/`0x7824F`/`0x78277` (0x1A), `0x782B6` (0x1C) and `0x76291` (0x1C, the
> call at `0x762A4`). `0x773DC`/`0x70DAF` are `MOV EDX,0x1a/0x1c` **arguments** to
> `CALL 0x92820` (the deferred motion/camera mailbox, §3.2); `0x80814` is an
> argument to `CALL 0x6E598` (animation selector); `0x9E665` is inside the
> constant-return getter `FUN_0009E420` (not an installer). This is the same
> alias/adjacency trap FU-180 §1.2 flagged for `0x7B0A7` (a `0x0D` immediate
> followed by `CALL 0x928F0`, not `0x7D9A4`).

### 1.2 `FUN_000782D0` — the record-0 machine (`0x782D0..0x785D0`)

Fresh `get_function_by_address 0x782D0`; fresh `get_xrefs_to 0x782D0` = **1**
(`0x8DB35`, in `FUN_0008D8EC`). Fresh `disassemble_bytes 0x782D0` (96 B): gates
`byte[0x157823]!=0`, `rec==[0x157A83]`, `phase!=5`; then increments the per-side
words via `FUN_000741B4` (FU-180 §4.2). The tail (fresh `0x78540`, 160 B) is the
**action install + decision dispatch**:

```
0x78544  EDX = [0x157A4A]>>24 (phase); CMP 0x5
0x7854C  EAX=4 (phase!=5) / 0x78553 EAX=0x19 (phase==5)
0x78558  DX = (int8)[rec+0x91]; if AX != DX:
0x7856C    byte[0x157AB2]=1; CALL 0x7D9A4(EAX, rec, ECX=0,EBX=0)   ; install 4 or 0x19
0x7857B  phase==2 && [rec+0x20]==0 && byte[0x157C5D]!=0
0x78599    CALL 0x77EAC (EAX=EBP)                                 ; -> the CPU ladder
0x785A8  CALL [rec+0x18]                                          ; staged action handler
0x785AD  CALL 0x6E8E8; (EBP==[0x157AA7]) copy pos->target
0x785C5  CALL 0x7BF20
```

The **record-0 machine** therefore: installs the record's own action (`4`/`0x19`),
and on `phase==2 & no slot & [0x157C5D]!=0` calls the CPU decision `FUN_00077EAC`.
Fresh `disassemble_bytes 0x78420` (96 B) shows the **input-row table dispatch**: at
`0x7842F`/`0x78436` it selects `0x110978`/`0x110970` by a `[0x157750]` compare, walks
8-byte entries (`word mask`, `word value`, `dword handler`), and at `0x7845D`
issues `CALL dword [EDX+4]` — the **COMPUTED_CALL** that is `FUN_00076130`
(fresh `get_xrefs_to 0x76130` lists `0x7845D` as COMPUTED_CALL; the two other
`0x110954`/`0x11097C` xrefs are the DATA tables).

### 1.3 `FUN_0008D8EC` — the driver pre-pass (`0x8D8EC..0x8DB6A`)

Fresh `get_function_by_address 0x8D8EC`; fresh `get_xrefs_to 0x8D8EC` = **4**:
`0x4B2AB`/`0x4B2B2` (in `FUN_0004B100`) and `0x74429`/`0x74430` (an unnamed
`0x743xx` routine — no function defined there this session). Fresh
`disassemble_bytes 0x8DB00` (96 B): it decrements a per-record timer, then
`0x8DB35 CALL 0x782D0 (EAX=EBP)`, then loops `0x8DB42` over `0xB2`-strided records
(`CMP EAX,0xB`) calling `0x7CA54` — the per-record walk FU-180 §4.2 named. **So the
chain `FUN_0008D8EC → FUN_000782D0 → FUN_00077EAC` is reachable from native code
via four call sites**, but the engine has not ported any of the three (FU180-L4/L5;
FU-151 §8 contract item 8).

### 1.4 `FUN_00076130` — the keeper input handler (`0x76130..0x7636C`)

Fresh `get_function_by_address 0x76130`; fresh `get_xrefs_to 0x76130` = **4**:
`0x76599` (UNCONDITIONAL_CALL, row-0x1F **stage-3 tail**), `0x7845D`
(COMPUTED_CALL, record-0 machine, §1.2), and `0x110954`/`0x11097C` (DATA, the
input-row tables). Fresh `disassemble_bytes 0x76130` (200 B) + `0x76250` (80 B) +
`0x7629D` (208 B) confirms the ported decision (`fifa96_keeper_input_decide`,
FU-140 §3.1) and pins its **install sites**:

```
0x76251  ECX=1; EDX=0x19; copy cam(0x15774C) -> target; XOR EBX,EBX; JMP 0x7634B
0x76286  slot dirs both zero (DX==0 && BX==0):
0x76291    EDX=0x1C; copy pos(EBP+0x59) -> target; MOV [EBP+0x51]=0xA0
0x762A4    CALL 0x7D9A4                                        ; install 0x1C invoke-now
0x762BC  slot dir nonzero: EDX = [EBP+0x20]; DX=(int8)[+0x20], BX=(int8)[+0x21]
0x7631E    target = pos + dir*0x70; z clamp +-0xAF0; clear byte[0x157C5C]
0x76327    CALL 0x8DCD4(pos,target,rec+0x65)
0x76332    dist = [rec+0x63]>>16; SETGE; EAX = 0x1B + (dist>=0x70)
0x7634B    CALL 0x7D9A4                                        ; install 0x1B / 0x1C invoke-now
```

So `FUN_00076130` exposes installs **4 / 0x19 / 0x1C (`0x762A4`) / 0x1B+1
(`0x7634B`)**, all invoke-now (`ECX=1`). The `0x76291`/`0x762A4` site is the exact
0x1C install; `0x7634B` yields 0x1C when `dist>=0x70`.

### 1.5 `FUN_00070C08` — **not** an installer (`0x70C08..`)

Fresh `disassemble_bytes 0x70C08` (96 B) + `0x70D80` (96 B): a ball/incoming helper
gated on `[rec+0x9A]==0` and `type8==0x22 && [0x157750]>=0x20`; it computes a target
from the `0x15774C`/`0x157764`/`0x1577BA` words, calls `0x8DC68`, then sets
`EDX=0x1C` at `0x70DAF` **and calls `0x92820`** (`0x70DBF`), then `0x71C94`. The
`MOV EDX,0x1C` is an **argument** to `0x92820` — not a `CALL 0x7D9A4`. Fresh
`get_xrefs_to 0x70C08` = 2 (`0x7C8FC` in `FUN_0007BF20`, `0x7C984`).

### 1.6 `FUN_00092820` — the deferred motion mailbox (why `0x92820` is not 0x7D9A4)

Fresh `get_function_by_address 0x92820` (body `0x92820..0x92861`) + fresh
`disassemble_bytes 0x92820`: `EAX=DL; AL=byte[EAX+0x110F1C] & 1`; `MOV
[0x15B650]=DL`; if the gate bit set, `CALL 0xCB2A4`, copy the `0x157758` triple to
`0x15B659..`, and `MOV [0x15B655]=EBP`. Fresh `get_xrefs_to 0x15B650` = 10: the
**readers** are `FUN_000700F4` (camera place), `FUN_00070544`, `FUN_000736AC`,
`FUN_00071DF4`; the **writers** are `FUN_000927E0`/`0x92820`/`0x92864`/`0x928F0`.
So `0x92820(rec, code)` is a **deferred motion/camera request**, not the action
installer. `0x70DAF` (0x1C) and `0x773DC` (0x1A) feed *this* mailbox.

---

## 2. The rows 0x1A / 0x1C bodies (fresh)

### 2.1 Row 0x1A `0x7662C` — 5-arm stage machine

Fresh `read_memory 0x76618` (20 B) = the arm table
`{0x00076693, 0x000766DD, 0x000766F5, 0x00076AAD, 0x00076AE8}` (stages 0..4).
Fresh `disassemble_bytes 0x7662C` (240 B):

```
0x76634  phase = [0x157A4A]>>24; CMP 0xD; JNZ 0x76662
0x76641    team[rec[0]+0x826] == [0x157AAC]>>24 -> MOV [0x157A83]=EBP
0x76662  [rec+0x89] += word[0x157A64]                          ; dword timer += delta
0x7667E  stage = byte[rec+0x92]; CMP 4; JA 0x76B21; JMP [0x76618+stage*4]
0x76693  (stage 0) CALL 0x765C4(EAX=EBP)                        ; FUN_000765C4
0x7669D    nonzero -> MOV EDX=4; CALL 0x7DAB4; CALL 0x7D9A4   ; install 4 invoke-now
0x766BE    zero -> byte[rec+0x9E]=1; [rec+0x89]=0; +0x92 += 2
0x766F7    CALL 0x79B1C
0x766FC    CMP [0x157C2C],0x2D  (then 0x76757: CMP 0x2E, 0x767E7, ... 0x5C at 0x7674C)
           the [0x157C2C] direction machine + common movement/event block
```

Fresh `disassemble_bytes 0x76700` (128 B) confirms the direction machine keys on
`[0x157C2C] ∈ {0x2D,0x2E,0x5C}` and other latches (`[0x1577EE]`, `[0x1577BC]`,
`[0x1577F8]`, `[rec+0x61]`), and writes `[0x157C2C]=0x5C; EAX=6; CALL 0x651F0`
(`0x7674C`). The fresh `get_xrefs_to 0x765C4` = 2 (`0x781EC` in `FUN_00077EAC`,
`0x76695` in row 0x1A) confirms the shared tracked-teammate predicate. **Deep**
(OL-34): the `FUN_000765C4` body, the full `[0x157C2C]` machine
(`0x766FC..0x767E7`), the common movement/event block (`0x767E7..0x76AA7`) and
stages 3/4 (`0x76AAD`/`0x76AE8`).

### 2.2 Row 0x1C `0x77728` — inline 0/1/2 dispatch

Fresh `disassemble_bytes 0x77728` (256 B):

```
0x77735  [rec+0x89] += word[0x157A64]
0x7774F  stage = byte[rec+0x92]: <1 -> 0x77762, ==1 -> 0x7778E, ==2 -> 0x779D1, else 0x779DE
0x7776A  (stage 0) byte[rec+0x9E]=1; [rec+0x89]=0; +0x92++; [0x157C56]=0
0x7778E  (stage 1) CALL 0x8DCD4(pos,target,rec+0x65)
0x7779C    FUN_00079C50(rec, -[0x1577C2], -[0x1577C0]); CALL 0xCD474 -> [rec+0x7F]
0x777D3    FUN_000CD474([rec+0x63]>>16, [rec+0x51]) -> angle; |angle| < 0x55 ->
0x777F6      pos += trunc_shift(delta,1) (0x8DC50); FUN_0008DC50 x2; event 0x3C; steer 0x80/0x40
0x7783C    else the rotated arm 0x77856 (see below)
0x779D1  (stage 2) byte[rec+0x44] -> FUN_0007DAB4; [rec+0x28] row byte 0x3C vs [rec+0x3A]>>24 ==2/3
0x779DE  (tail) word[rec+0x81]!=0 -> 0x77E8B; ... CALL 0x76B28 at 0x779B4
```

Fresh `disassemble_bytes 0x77856` (80 B) — the **rotated arm** uses the
`0x114E04` sine table: `word[rec+0x65] += 0x40`, `AH = ([rec+0x7D]>>16)` sign-fold,
`EAX = dword[EAX*4 + 0x114E04]`, then `CALL 0x795A4` -> `word[rec+0x67]` (and the
`+0x100` second tap). Fresh `get_xrefs_to 0x76B28` = 2 (`0x76FA2` row-0x1B stage 1;
`0x779B4` row-0x1C tail), confirming the FU-180 §4.4 steering caller set.
**Deep** (OL-35): the rotated arm, the `0x795A4` fold and the tail steering.

---

## 3. The installer census — true sites vs FU-182 §5’s list

Fresh `search_instructions "EDX, 0x1a"` = 11 and `"EDX, 0x1c"` = 11. Classifying
each by whether a `CALL 0x7D9A4` follows in the same basic block (fresh
`get_xrefs_to 0x7D9A4` = 77):

| site | immediate | following call | verdict |
|---|---|---|---|
| `0x78227`/`0x7824F`/`0x78277` | `EDX,0x1a` | `CALL 0x7D9A4` (`0x78236`/`5E`/`85`) | **0x1A installer** (`FUN_00077EAC` arm 0x2C/2D/2E) |
| `0x782B6` | `EDX,0x1c` | `CALL 0x7D9A4` (`0x782BF`) | **0x1C installer** (`FUN_00077EAC` arm) |
| `0x773DC` | `EDX,0x1a` | `CALL 0x92820` (`0x773E3`) | **not** — mailbox arg |
| `0x29563`, `0x2FDDD` | `EDX,0x1a` | non-match-core region | not 0x1A installs |
| `0x70DAF` | `EDX,0x1c` | `CALL 0x92820` (`0x70DBF`) | **not** — mailbox arg (`FUN_00070C08`) |
| `0x80814` | `EDX,0x1c` | `CALL 0x6E598` (`0x80826`) | **not** — anim-selector arg |
| `0x76291` | `EDX,0x1c` | `CALL 0x7D9A4` (`0x762A4`) | **0x1C installer** (`FUN_00076130`) |
| `0x46FA7`, `0x929E8`, `0xB1953`, `0xB1979` | `EDX,0x1c` | `CMP`/`AND`, not installs | not installs |
| `0x9E665` | `EDX,0x1a` | `MOV EAX,EDX; RET` (`FUN_0009E420`) | **not** — constant getter |

**True installer set.**
* **0x1A** — three sites, all `FUN_00077EAC`: `0x78227`→`0x78236`, `0x7824F`→
  `0x7825E`, `0x78277`→`0x78285`. (No other native 0x1A installer exists.)
* **0x1C** — `FUN_00077EAC` `0x782B6`→`0x782BF` (and `0x782BB` via the `0x78200`
  JMP); `FUN_00076130` `0x76291`→`0x762A4` (slot-dir both-zero arm) and
  `0x7634B` (the computed `0x1B+1`, = 0x1C when `dist>=0x70`). All invoke-now.

---

## 4. Live reach — what each install needs to *execute honestly*

| install | native installer(s) | live native reach | live **engine** reach today | body cost |
|---|---|---|---|---|
| **0x1C** | `0x76291`→`0x762A4` and `0x7634B` (`FUN_00076130`); `0x782B6` (`FUN_00077EAC`) | `FUN_00076130`’s `0x762A4` runs from the **row-0x1F stage-3 tail** (`0x76599`, unconditional) and the record-0 machine table (`0x7845D`, computed) | **YES** — the wired `fifa96_match_action_1F` drains `fifa96_keeper_input_decide` and calls `fifa96_match_entities_install(..., dec.install, ...)` + `fifa96_match_dispatch_action(mr, dec.install)` (`fifa96_match_handlers.c:3629..3645`); `dec.install` is 0x1C at the both-slot-dirs-zero arm | **medium** — inline 0/1/2 body; `fifa96_keeper_lunge_track` ported |
| **0x1A** | `0x78227`/`0x7824F`/`0x78277` (`FUN_00077EAC`) | the ladder runs on the live `FUN_0008D8EC → FUN_000782D0` walk (`0x8DB35`), gated `phase==2 & no slot & [0x157C5D]!=0` | **NO** — `FUN_0008D8EC`/`FUN_000782D0`/`FUN_00077EAC` are unported; the ported `fifa96_keeper_input_decide` never emits 0x1A | **high** — 5-arm machine + `FUN_000765C4` predicate + `[0x157C2C]` direction machine + the common movement/event block |

**Honest conclusion.**
* **0x1C is a first-order census mover.** Its native installer (`0x76291`→
  `0x762A4`) is reached by the *already wired* row-0x1F handler through the ported
  `fifa96_keeper_input_decide` drain, which calls the pool install with
  `dec.install = 0x1C` and then `fifa96_match_dispatch_action(mr, 0x1C)`. Wiring
  `fifa96_match_action_1C` makes that dispatch reach a real body — the census
  criterion (an installer invocation the engine already reaches) is satisfied
  **today**. (The `FUN_00077EAC` `0x782B6` path stays FU180-L5.)
* **0x1A is a ported-body-without-installer** (the rows 0x17/27/29/2C precedent).
  Its only native installers are the three `FUN_00077EAC` ladder arms, on the
  unported `FUN_0008D8EC → FUN_000782D0 → FUN_00077EAC` CPU decision; the engine
  reaches no 0x1A install. Wiring the body is honest but **does not move the
  census** until the ladder lands.

### 4.1 Ranked T3 wiring recommendation (both bodies)

1. **Row 0x1C — wire now (counted).** Port the inline 0/1/2 body
   (`0x77728..0x77E8B`) as `fifa96_match_action_1C` over the pool record, binding
   the ported `fifa96_keeper_lunge_track` (stage 1 on-target arm) and the
   `0x8DCD4`/`0x79C50`/`0xCD474` metric/face calls; disclose the rotated arm
   (`0x77856`, `0x114E04` sine + `0x795A4`) and the tail steering (`0x779B4`
   `FUN_00076B28`) as legs. Set `action_table[0x1C]`. The row-0x1F tail’s live
   `install(0x1C, invoke-now)` + `dispatch(0x1C)` makes it execute end-to-end.
   **Moves 24→25 (`floor(100×25/80)` = 31%).**
2. **Row 0x1A — wire the bounded body, co-scoped (uncounted).** Port the head
   (`phase-0xD` gate, `[0x157A83]` bind, `+0x89 += delta`, stage dispatch), stage 0
   (`FUN_000765C4` → `FUN_0007DAB4` + install 4 invoke-now; else `[+0x9E]=1`,
   `+0x92 += 2`, `+0x89=0`, `FUN_00079B1C`, the `[0x157C2C]` machine), and stages
   1/2/3/4 with all roads disclosed. Its installer is the `FUN_00077EAC` ladder, so
   keep `action_table[0x1A]` NULL (or set it with an explicit “ported body, entry
   unresolved” note as rows 0x17/27/29/2C do) until the ladder lands. **Counts only
   when FU180-L4/L5 is ported** (then both → `26/80` = 32%).
3. **The chain (deep remainder → legs).** `FUN_0008D8EC` (driver pre-pass),
   `FUN_000782D0` (record-0 machine + input-row table), `FUN_00077EAC` (CPU ladder
   incl. the `[0x14C1D4+side*2]` gate), `FUN_000765C4` (tracked-teammate
   predicate), `FUN_00076130`’s install *execution*, the `0x114E04` sine table and
   the `FUN_00076B28` steering all stay numbered legs below.

---

## 5. Row-0x1B sine/steering legs (re-verified this session)

Fresh `disassemble_bytes 0x76E3B` (200 B) re-confirms FU-180 §2 and FU140 OL-35:

* **sine rotation (FU180-L2).** Row-0x1B stage 1 at `0x76E95..0x76EDA`:
  `[rec+0x65] += 0x40`; `AH = ([rec+0x7D]>>16)` sign-fold (`SHL AH,7; SBB; ADD AH,AH;
  SBB ECX; XOR EAX,ECX; AND 0xff; SUB EAX,ECX`); `EAX = dword[EAX*4 + 0x114E04]`;
  then `CALL 0x795A4` → `word[rec+0x67]`, plus the `+0x100` second tap. Fresh
  `search_instructions "0x114e04"` = 20+ sites (the table is loaded as an immediate
  into a register, hence fresh `get_xrefs_to 0x14E04` = **0**); the flat base is
  `0x14E04` (`0x114E04` in the resolved form).
* **steering (FU180-L3).** `FUN_00076B28` body `0x76B28..0x76D11`; fresh
  `get_xrefs_to 0x76B28` = 2 (`0x76FA2` row-0x1B stage 1, `0x779B4` row-0x1C tail);
  it calls `FUN_000795B4` at `0x76C59`. Row-0x1B stage 1 reaches it at `0x76FA2`.
* **stage-2 wide body (FU180-L1).** The `0x76FD5..0x77636` rush body incl. the
  `0x71C94` call at `0x77423` stays FU180-L1 (unchanged this session; the fresh
  `get_xrefs_to 0x71C94` in FU-180 was 11).

The row-0x1B body is already wired (FU-180 §8); this slice only re-pins its two
unported arms. No change to the FU-180 landing note.

---

## 6. Numbered legs

1. **FU186-L1 — row-0x1A direction machine + movement block.** The
   `[0x157C2C] ∈ {0x2C,0x2D,0x2E,0x5C}` direction machine (`0x766FC..0x767E7`) and
   the common movement/event block (`0x767E7..0x76AA7`), stages 3/4
   (`0x76AAD`/`0x76AE8`). Carries FU-140 OL-34. (`FUN_000765C4` is L4.)
2. **FU186-L2 — row-0x1C rotated arm + tail steering.** The `0x77856` rotated arm
   (`0x114E04` sine + `0x795A4`), the `0x779D1` stage-2 body and the
   `0x779DE..0x77E8B` tail incl. the `0x779B4` `FUN_00076B28` call. Carries FU-140
   OL-35.
3. **FU186-L3 — the `0x14E04` sine table.** Address, entry size (dword), length and
   value range not decomposed here; only that it is loaded as `dword[EAX*4+0x114E04]`
   by both the row-0x1B stage-1 arm (`0x76EA7`/`0x76EE3`) and the row-0x1C rotated
   arm (`0x77879`). Carries FU180-L2.
4. **FU186-L4 — `FUN_000765C4` tracked-teammate predicate** (`0x765C4..0x76615`).
   Shared by `FUN_00077EAC` (`0x781EC`) and row-0x1A stage 0 (`0x76695`); body not
   decomposed. Carries FU-140 OL-34.
5. **FU186-L5 — the CPU decision `FUN_00077EAC`** (`0x77EAC..0x782CD`): the full
   selector body (`0x77F50..0x781E1`: the `0x1577BC`/`0x15774C`/`0x157750` gates,
   `FUN_000795B4` at `0x77F90`, the `0x10E184`/`0x10E1C4`/`0x10E1CE` tables at
   `0x780E2..0x7819D`) and the `ECX` selector → ladder mapping (which arm each
   `ECX = 1..5` reaches). This is the **0x1A installer**; the row cannot count
   without it. Carries FU180-L5 and FU-140 OL-36.
6. **FU186-L6 — `FUN_000782D0` record-0 machine** (`0x782D0..0x785D0`): the
   prologue gates, the `0x78544` action install (`4`/`0x19`), the `0x7857B`
   phase-2/no-slot/`[0x157C5D]` `FUN_00077EAC` gate, `CALL [rec+0x18]`, `0x6E8E8`,
   `0x7BF20`, and the input-row table dispatch (`0x110970`/`0x110978`,
   `CALL [EDX+4]` at `0x7845D` = `FUN_00076130`). Carries FU180-L4.
7. **FU186-L7 — `FUN_0008D8EC` driver pre-pass** (`0x8D8EC..0x8DB6A`): the callers
   (`FUN_0004B100` `0x4B2AB`/`0x4B2B2`; the unnamed `0x743xx` routine
   `0x74429`/`0x74430`), the `0x782D0` call (`0x8DB35`) and the `0xB2`-stride
   `0x7CA54` loop. Whether the engine’s per-frame pre-pass reaches it is FU-151 §8
   contract item 8 / leg 8.
8. **FU186-L8 — the `0x92820` motion mailbox** (`0x92820..0x92861`,
   `0x15B650..`): the deferred-code writers (`FUN_000927E0`/`0x92820`/`0x92864`/
   `0x928F0`) and readers (`FUN_000700F4`/`0x70544`/`0x736AC`/`0x71DF4`), and the
   `0x70DAF`/`0x773DC` request sites. Ruled **out** of the action-installer set
   here; body not decomposed.
9. **FU186-L9 — `FUN_0009E420`** (the constant-code getter): `0x9E665` returns
   `EDX/EAX = 0x1A`; the table spans `0x9E420..0x9E715` (callers: `FUN_0009E890`
   `0x9E89B`). Ruled out as an installer; consumer not traced.

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info`; `get_function_by_address` `0x77EAC`? (via search),
`0x92820` (`0x92820..0x92861`), `0x9E420` (`0x9E420..0x9E715`), `0x76B28`
(`0x76B28..0x76D11`), `0x765C4` (`0x765C4..0x76615`), `0x76130`, `0x8D8EC`
(`0x8D8EC..0x8DB6A`), `0x7BF20` (`0x7BF20..0x7C90A`, from the `0x70C08` xref);
`get_xrefs_to` `0x77EAC` (1), `0x782D0` (1), `0x8D8EC` (4), `0x70C08` (2),
`0x76130` (4), `0x7D9A4` (77), `0x15B650` (10), `0x14E04` (0), `0x76B28` (2),
`0x765C4` (2), `0x9E420` (1); `search_instructions` `EDX, 0x1a` (11), `EDX, 0x1c`
(11), `0x14e04` (0), `0x114e04` (>20, truncated); `read_memory` `0x77E98` (24 B),
`0x76618` (20 B); `disassemble_bytes` `0x78200` (256 B), `0x9E640` (80 B),
`0x70D80` (96 B), `0x76250` (80 B), `0x807F0` (64 B), `0x773C0` (64 B), `0x77EAC`
(160 B), `0x77F50` (224 B), `0x7802B` (320 B), `0x7816A` (160 B), `0x78540`
(160 B), `0x8DB00` (96 B), `0x78420` (96 B), `0x76130` (200 B), `0x7629D`
(208 B), `0x70C08` (96 B), `0x92820` (80 B), `0x743F0` (96 B), `0x7662C`
(240 B), `0x77728` (256 B), `0x76E3B` (200 B), `0x77856` (80 B), `0x779D1`
(80 B), `0x76700` (128 B). Engine/loader source read: `fifa96_match_handlers.c`
(3650..3775, 4078..4088, 3615..3648), `fifa96_keeper.c` (140..289, 380..438),
`fifa96_action_handlers.c`. Docs cross-cited: FU-79, FU-82, FU-136, FU-137,
FU-140, FU-151, FU-174, FU-176, FU-179, FU-180, FU-182. No writes beyond this
slice file and the workspace report.
