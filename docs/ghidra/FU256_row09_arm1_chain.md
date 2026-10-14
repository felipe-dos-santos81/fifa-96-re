# FU-256 — the row-09 arm-1 install chain (→ action `0D`)

Provenance: M2 phase-25 wave **W25-1**, 2026-10-14, **read-only** recon.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument). Fresh `get_current_program_info` this session:
**2706 functions / 15239 symbols**, `watcom:LE:32:default`, LE image with one
`.image` overlay space (the overlay is *not* engine code — every address below is
default-space runtime). Every address/byte/xref/call below is a **fresh**
`disassemble_bytes` / `read_memory` / `get_xrefs_to` / `search_byte_patterns` /
`search_instructions` of this session unless marked *cross-cited* (the frozen FU
doc or the engine source is named). Alias traps applied (`dword[x]>>16` = the word
at `x+2`; `>>24` = the byte at `x+3`; so `[rec+0x3A]>>24` = byte `+0x3D` and
`[[rec+4]+0x13]>>24` = byte `[[rec+4]+0x16]`). Engine source read first-hand:
`src/fifa96_engine/fifa96_match_handlers.c` (row-09 body 4075..4182, action table
5460..5475), `include/fifa96_engine/fifa96_match_entities.h` (record layout),
`src/fifa96_loader/fifa96_action_handlers.c` (`fifa96_action_sequence_0d_step`
1410), `tests/test_engine_match_handlers.c` (row-0D leg pin). Docs cross-cited:
FU-178, FU-180, FU-204, FU-245, FU-243, FU-190, OL-80/OL-82. **No writes** beyond
this draft and the workspace report: no rename/comment/label/function/script/
project save; no git; the ISO and `game/hdd/` untouched. No subagents.

This slice independently re-derives the row-09 **arm-1** install path (`→ action
`0x0D``), the target body, the arm gates, the helpers/sinks and the reaches, and
rules whether row `0D` can honestly join the census (install **and** dispatch)
with M2 preserved (FU256; plan
`docs/superpowers/plans/2026-10-14-fifa96-m2-phase25-census-closeout-recon-ahead.md`).

---

## 1. Answer in one line

Row `0D`'s **sole** installer `0x80F61` (`ba0d000000` / `e835caffff`) is inside
row-09 **arm 1** `0x80BFB..0x80FCB`, which is reached **only** through the row-09
stage table `0x809F0` (fresh `get_xrefs_to 0x80BFB` = **2**: the slot-1 dword
`0x809F4` + the `0x80A37` computed jump — no direct `CALL`). The arm is
**enterable** on the wired row-09 stage machine (arm 0 advances `+0x92` 0→1 at
`0x80BEB`), but its body is **unported** (leg FU178-L1) and its `0x0D`-install
branch is enabled **only** by two cells with **no located producer**
(`[[rec+4]+0x16]` descriptor byte and `[rec+0x24]` pairing pointer, leg
FU204-L2). A faithful staged-zero port would emit the `0x60` event and **never
install `0D`** — so wiring the `fifa96_match_action_table[0x0D]` slot would be a
**fake count** (R15/R45). **Ruling: row `0D` installs NOT, dispatches NOT,
tape-dormant; the slot stays NULL/uncounted** (leg FU178-L1, gate producers
FU204-L2). M1/M2 stay byte-identical (no re-pin). Payoff **+0** today.

---

## 2. The install site `0x80F61` (fresh bytes)

Fresh `disassemble_bytes 0x80BFB..0x80FCB` (281 instructions; the arm) — the
install tail:

```
0x80F4B  MOV EBX,0x4        bb04000000
0x80F50  MOV EAX,0x5d       b85d000000
0x80F55  MOV EDX,EBP        89ea
0x80F57  MOV ECX,0x1        b901000000
0x80F5C  CALL 0x8F188       e827e20000     ; emit event 0x5D on EBP
0x80F61  MOV EDX,0xD        ba0d000000     ; <-- action 0x0D
0x80F66  MOV EAX,ESI        89f0
0x80F68  XOR EBX,EBX        31db           ; EBX=0 = install-only (no invoke)
0x80F6A  CALL 0x7D9A4       e835caffff     ; rel32 0xFFFFCA35 -> 0x80F6F-0x35CB = 0x7D9A4
0x80F6F  JMP 0x80F82        eb11
0x80F71  MOV EBX,0x4        bb04000000
0x80F76  MOV EAX,0x60       b860000000
0x80F7B  MOV EDX,ESI        89f2
0x80F7D  CALL 0x8F188       e806e20000     ; else emit event 0x60 on ESI
0x80F82  MOV AL,[EBP+0x92]  8a8592000000
0x80F88  MOV dword[EBP+0x89],0x0
0x80F92  INC AL ; MOV [EBP+0x92],AL          ; advance stage, clear timer89
0x80F9A  ADD ESP,0x10 ; POP×5 ; RET
```

Rel32 verified: `0x80F6A` (`e835caffff` = −0x35CB) targets `0x7D9A4`. The arm-1
sibling install `0x0C` is fresh at `0x80EEC b901000000 / 0x80EF1 ba0c000000 /
0x80EF8 31db / 0x80EFA e8a5caffff` (−0x355B → `0x7D9A4`).

**Installer ABI** (cross-cited FU-178 §2 / FU-180 §1.2, contract fresh elsewhere
in FU-204 §1): `FUN_0007D9A4(EAX=rec, DX=action, BX=stage, ECX=invoke)` writes
`rec+0x91 = action`, `rec+0x92 = stage`, `rec[+0x18] = 0x1106E0[action]` and
clears `+0x89`/`+0x9E`. Fresh `read_memory 0x1106E0` (64 B) gives the handler
table `[0]=0x7DB10 … [0xC]=0x81C90, [0xD]=0x8251C, [0xE]=0x82710, [0xF]=0x82AD0`.
So `0x80F61` installs **the row-`0D` body `0x8251C`** on the record `ESI` (not
`EBP`) — install-only.

**Sole-installer tier (fresh).**
`get_xrefs_to 0x7D9A4` = **77** UNCONDITIONAL_CALL (matches FU-174/FU-180/FU-204);
`0x80F6A` and `0x80EFA` are both in the fresh list.
`search_byte_patterns ba0d000000` = 9 default-space hits
(`0x19438, 0x49e60, 0x517b5, 0x7b0a7, 0x80f61, 0xc31a3, 0xc3252, 0xc36e1,
0xc372b`) + `.image` overlay false positives; only **`0x80F61`** is a `0x7D9A4`
site (the other eight feed `CALL 0x928F0`/`0x13600`/`0xC87D8`-family sinks —
FU-180 §1.2's ruled-out set). `search_instructions MOV EDX,0xd` = **21** (the
`d0/d5/d9/db/dc` substrings included). The register-derived code domain
(FU-243 §2.4 / FU-142f) is `{0,1,3,4,0x15,0x19,0x1b,0x1c,0x1d,0x1e,0x25,0x26,
0x28}` — **it does not contain `0xD`**. Hence **`0x80F61` is the sole installer of
action `0D`**.

---

## 3. The target body `0x8251C..0x826FE` (fresh bytes)

Fresh `read_memory 0x8250C` = `{0x82567, 0x8261F, 0x826B0, 0x826D5}` (the 4-arm CS
table) and `disassemble_bytes 0x8251C..0x826FE` (122 instructions):

```
0x8251C  PUSH EBX/ECX/EDX/ESI/EDI/EBP ; MOV EBP,EAX
0x82526  MOV EDX,[EBP+0x89] ; MOV AX,[0x157A64] ; ADD EDX,EAX ; MOV [EBP+0x89],EDX   ; timer89 += delta
0x82534  MOV EBX,[0x157A83] ; CMP EBP,EBX ; JNZ 0x8254C ; MOV [0x157A83],0           ; clear actor if self
0x8254C  MOV AL,[EBP+0x92] ; CMP AL,0x3 ; JA 0x826F8 ; JMP CS:[EAX*4+0x8250C]        ; stage dispatch
```

* **arm 0** `0x82567`: `byte[+0x8D]==0 → 0x826F1` (reset). Else `word[+0x83]=0x20`;
  `0x702F8((int16)[+0x81]>>16, 0x10) → +0x85`; `word[+0x87]=0`; `0x6E598` anim;
  type tables `0x10F334`/`0x10F33C[sector]`; `scale = (x!=0 && z!=0) ? 3 : 4`
  (`0x825CB..0x825E1`); `+0x73/+0x75 = type*scale`; `0x8DC68 → +0x71`; `+0x9E=1`;
  timer=0; stage++.
* **arm 1** `0x8261F`: while `word[+0x85]!=0` stay; else pos→target,
  `target += (int8)0x10F331/0x10F339[sector]<<6` (the `>>24` alias yields the
  `0x10F334/0x10F33C` bytes — FU180 §10 erratum), `0x8DCD4(pos,target,+0x65)`,
  `0x6E598` anim `0x0B`, timer=0, stage++.
* **arm 2** `0x826B0`: if `+0x71 != 0 && byte[+0x44] != 0 → 0x826F8`; else timer=0,
  stage++, fall to arm 3.
* **arm 3** `0x826D5`: `0x79C50(rec, [+0x71]>>16, [+0x73]>>16)`; `if (+0x89 > 0x14)
  → 0x826F1` reset (`FUN_0007DAB4`).

Fresh `get_xrefs_to 0x8251C` = **1** (DATA from `0x110714` = `0x1106E0 + 0xD*4`,
the action-table slot) — there is **no direct `CALL 0x8251C`**; the body is reached
only through the installed `[rec+0x18]` pointer.

The body is **already ported** as `fifa96_action_sequence_0d_step`
(`fifa96_action_handlers.c:1410`, FU-180 §9; unit-tested in
`tests/test_action_handlers.c:322`). The engine action-table slot is **NULL**
(fresh source read `fifa96_match_handlers.c:5474 {0x0D, NULL, …}`). So this is the
canonical "ported body, no ported installer" case — the rows-27/29/2C precedent.

---

## 4. Arm-1 entry and gates (fresh bytes + producer status)

### 4.1 Head gate (`0x80BFB..0x80C33`)

```
0x80BFB  CMP byte[0x15877C],0   803d7c87150000     ; sequence flag must be 0
0x80C02  JNZ 0x80C13                               ; flag != 0 -> tail (stage++, no install)
0x80C04  MOV EAX,[EBP+0x28] ; MOV AL,[EAX] ; AND 0xFF ; CMP EAX,0x11
0x80C11  JZ 0x80C35                                ; template byte [[rec+0x28]]==0x11
0x80C13  MOV [EBP+0x89],0 ; INC [EBP+0x92] ; RET    ; else advance stage, no install
```

* **`[0x15877C]`** (FU178-L6): fresh `get_xrefs_to 0x15877C` = **7** — writes
  `0x80BD3`(arm 0 → 0), `0x80DCC`(arm 1 → 1), `0x80FD1`(arm 2 → 0); reads
  `0x80BFB`/`0x80FA4` (row-09) and `0x839AB`/`0x83AE0` (`FUN_00083428`, the
  presentation consumer reading the **sibling byte** `0x15877F` via `>>24`).
  **Self-contained**: no writer outside row 09. Arm 0's own `0x80BD3` clear
  satisfies the head gate on the 0→1 transition. **Not live as a producer beyond
  row 09** — the engine arm-0 port treats it as unmodelled (OL-82).
* **`[[rec+0x28]]==0x11`**: written by arm 0's anim install at `0x80BC5`
  (`0x80BB0 MOV EDX,0x11; 0x80BC5 CALL 0x6E598`); the engine arm-0 port installs
  0x11 (`handlers.c:4174`). **Satisfied by arm 0's own write.**

### 4.2 Body gates (`0x80C35..0x80F49`)

```
0x80C35  MOV EAX,[EBP+0x3A] ; SAR EAX,0x18 ; CMP EAX,0x2 ; JNZ 0x8105C   ; byte[rec+0x3D]==2 (FU-84 frame)
0x80C44  MOV EAX,[EBP] ; MOV ESI,[EAX+0x7A6] ; MOV ESI,[ESI+0x7B2]      ; team target / nearest slot
0x80C92  CMP dword[0x157750],0x20 ; JGE 0x80E13                         ; cam-z gate
0x80C9F  MOV AX,[EBP+0x6B] ; CMP AX,[EBP+0x77] ; JG 0x80E13            ; lane <= old-lane bound
0x80CAD  EAX = 3*BX + 0x40 ; CMP [rec+0x69]>>16, EAX ; JGE 0x80E13     ; BX = (int8)[[rec+4]+0xC]+(int8)[[rec+4]+0x16]
0x80E13  TEST ESI,ESI ; JZ 0x80FA4 ; CMP byte[ESI+0x8D],0 ; JZ 0x80FA4  ; target active gate
0x80E32  CALL 0x8DCD4(EBP,ESI,&tmp) / CMP dist>0x40 ; 0x80E57 CALL 0xCD474 / CMP |angle|>=0x55
0x80E81  heading(ESI)-heading(EBP) fold ; CMP EDX,0x80 ; JGE 0x80F21
  ; < 0x80  -> 0x80EAB branch: [[rec+4]+0x16] -> RNG 0x92AC8 -> 0x8A3FC -> 0x80EFA install 0x0C
  ; >= 0x80 -> 0x80F21 branch: [[rec+4]+0x16] -> 3*byte -> RNG 0x92AC8 -> AND 0x3F -> CMP
0x80F3F  JC 0x80F4B                                  ; rng&0x3F < 3*byte[[rec+4]+0x16] -> install 0x0D
0x80F41  CMP EBP,[ESI+0x24] ; JZ 0x80F4B             ; ... or rec == partner
0x80F46  CMP ESI,[EBP+0x24] ; JNZ 0x80F71            ; ... or partner == rec; else 0x60 event (no install)
```

The `0x0D`-install branch is taken **iff** `byte[rec+0x3D]==2` (else stall at the
shared tail `0x8105C`, fresh `disassemble_bytes 0x8103A..0x81066`: the tail does
**not** advance stage) **and** the geometry/RNG/pairing gates hold. The two
install-enabling cells:

* **`[[rec+4]+0x13]>>24` = byte `[[rec+4]+0x16]`** (the `[rec+4]`-descriptor byte
  read at `0x80EAE` and `0x80F24`). No located producer (leg FU204-L2,
  FU167/FU168-class descriptor binding). **Unmodelled → staged 0 → `3*0 = 0` →
  the `0x0D` RNG threshold is 0 → never fires.**
* **`[rec+0x24]`** (the reciprocal partner/pairing pointer, `0x80F41`/`0x80F46`).
  No located producer (leg FU204-L2). **Unmodelled → 0 (NONE) → EBP/ESI are valid
  pointers, never 0 → never fires.**

**Producer status of the entry gates:**
| gate | byte | producer | live in-engine? |
|---|---|---|---|
| `[0x15877C]==0` | `0x80BFB` | row-09 arm 0 `0x80BD3` (self) | yes (arm-0 write), but unmodelled cell (OL-82) |
| `[[rec+0x28]]==0x11` | `0x80C04` | row-09 arm 0 `0x80BC5` (self) | yes (`record_anim_install(…,0x11,0)`) |
| `[rec+0x3D]==2` | `0x80C35` | FU-84 frame; `FUN_00036C70` `0x36D4F` (OL-80 **closed**, pool `frame` live) | frame cell live; the `==2` value on the row-09 record is **not shown** |
| `[[rec+4]+0x16]` | `0x80EAE`/`0x80F24` | **unlocated** (FU204-L2) | **no** |
| `[rec+0x24]` | `0x80F41`/`0x80F46` | **unlocated** (FU204-L2) | **no** |

⇒ The arm-1 **head** is enterable (both self-gates pass), but the **install**
branch is gated on two unmodelled, unproduced cells. Consistent with FU-245 §2.2
("the arm is not 'unreachable', it is **unported**, and its install inputs are
unmodelled") and FU204 §4.2.

---

## 5. The helpers/sinks (fresh bytes; port shapes)

Fresh disassembly of the five arm-1 helpers/sinks (FU180-L8 class):

* **`0x8F188`** — the event ring. Head: `CMP byte[0x14C32A],0 / JNZ 0x8F2BF` (the
  FU-155 session gate) and `CMP dword[0x14C312],0 / JZ 0x8F2BF`, then a
  `EBX`-dispatch (`EBX==0x8 → [0x15B370]=1`; `EBX==0x40 → [0x15B36C]=1`). Called
  with `EBX=4` for events `0x5D`/`0x60`/`0x5E`. **Unported**; its absence does
  **not** gate the `0x0D` install (the `0x80F5C`/`0x80F7D` calls are side-effects,
  the `0x80F61` install is unconditional once the branch is taken).
* **`0x8ED40`** (`0x80D67`/`0x80D8D`) — `EBP` lerp helper over `[rec]→[+0x7AE]`
  (`byte[rec+0x8D]` vs `[+9]`), 2-arg `EDX`, 3/4-arg `EBX`. **Unported** (FU169-L2
  class).
* **`0x92820`** (`0x80DB1`, `EDX=0x16`) — `DL` → `[0x15B650]`; if
  `byte[0x110F1C+DL]&1` copies the `0x157758` triple to `0x15B659`. **Unported**
  (OL-27 class).
* **`0x7A490`** (`0x80DC7`) — the FU-139 §3.1 staging write-back; gates on
  `byte[rec+0x99]` and `[0x157A6C]`, writes `[0x158730]=EBP` + a `MOVSD/MOVSW`
  triple + `[0x15873E]`/`[0x158743]`. **Unported** (OL-62 class).
* **`0x8A3FC`** (`0x80EE7`, the **`0x0C`** branch) — writes `[0x15888C]=AL`,
  `[0x15888F]=EDX`, `[0x158893]=EBX`, copies a `[reg+0x59]` triple to
  `0x158897`. **Unported** (FU204-L4 class).

These are all named, unported sinks/side-effects; none is a **gate** on the `0x0D`
install. Their absence makes a faithful arm-1 port side-effect-free but not
incorrect — the blocker is §4.2.

---

## 6. The reaches (fresh xrefs; is the arm livable from wired row-09?)

Fresh `get_xrefs_to 0x80BFB` = **2**: `{0x809F4 DATA (the stage-table slot 1),
0x80A37 COMPUTED_JUMP}`. Fresh `get_xrefs_from 0x80A37` = `{→0x80A3F, →0x80BFB,
→0x80FCC, →0x8103A COMPUTED_JUMP; →0x809F0 DATA}`. So:

* The arm entry has **no direct `CALL`**; the only route is the row-09 stage
  machine `0x80A37 JMP CS:[EAX*4+0x809F0]` inside row-09's head `0x80A00`.
* Row 09 is dispatched through action-table slot 9 (`0x1106E0[9]=0x80A00`); its
  installer is **live** (pad release `0x7CFB2` + the wired row-06 RNG arm
  `0x807C1`; cross-cited FU-174/FU-178 §5, engine `handlers.c:5468`).
* Fresh `disassemble_bytes 0x80BB0`: arm 0 installs anim `0x11` (`0x80BB0 MOV
  EDX,0x11; 0x80BC5 CALL 0x6E598`), clears `[0x15877C]` (`0x80BD3`), and
  **advances stage 0→1** (`0x80BEB MOV [EBP+0x92],CH`). So the next row-09
  dispatch **routes to arm 1** (`0x80A37 + 0x809F4`).

**But** the engine's row-09 handler (`fifa96_match_action_09`,
`handlers.c:4111`) **parks** at a non-zero stage:

```c
if (r->stage92 != 0u) {              /* arms 1/2/3 = FU178-L1 */
  mr->match09_leg_stage = r->stage92;
  return FIFA96_OK;
}
```

So **the engine never executes arm 1, and `0x80F61` never runs.** The arm is
"livable from the wired row-09 stage-0" in the native control-flow sense (arm 0
seeds stage 1), but the engine deliberately does **not** land it. This **refines**
the FU-190 §6.1 / action-slot comment "not shown reachable" (it is *enterable*,
not *unreachable*; FU-245 §2.2).

---

## 7. Census ruling + payoff

**Row `0D`: installs NO, dispatches NO, tape-dormant. Payoff `+0` today.**

| axis | verdict | evidence |
|---|---|---|
| sole installer | `0x80F61` inside row-09 arm 1 | fresh `get_xrefs_to 0x7D9A4`=77; `search_byte_patterns ba0d000000` (9 hits, only `0x80F61` is a `0x7D9A4` site); register-derived domain excludes `0xD` (FU-243 §2.4) |
| installer host | row-09 arm 1 `0x80BFB..0x80FCB`, unported | FU178-L1; engine parks (`match09_leg_stage`) |
| arm reachable? | enterable via `0x809F0[1]` + `0x80A37`; **engine parks at stage 1** | fresh `get_xrefs_to 0x80BFB`=2; `handlers.c:4130` |
| install-enabling cells | `[[rec+4]+0x16]` and `[rec+0x24]` — **no located producer** | FU204-L2; fresh bytes `0x80EAE/0x80F24/0x80F41/0x80F46` |
| body | ported, slot NULL | `handlers.c:5474 {0x0D, NULL, …}`; `fifa96_action_sequence_0d_step` |

**Port shape (honest ceiling).** Row `0D` becomes countable **only** when, in the
**same** commit/system:
1. row-09 arm 1 is ported (the full `0x80BFB..0x80FCB` body incl. the stage
   advance at `0x80F9A` and the `0x0C`/`0x0D` installs), **and**
2. the two install-enabling cells have live producers:
   `[[rec+4]+0x16] > 0` (descriptor byte) or a staged `[rec+0x24]` pairing
   pointer, **and**
3. the `[rec+0x3D]==2` frame condition is actually met by the row-09 record's
   live FU-84 frame (OL-80 makes the cell live; the `==2` value is not shown),
   **and**
4. the helpers/sinks (`0x8F188`/`0x8ED40`/`0x92820`/`0x7A490`/`0x8A3FC`) are at
   least modelled or honestly leg'd,
   **and** the `fifa96_match_action_table[0x0D]` slot is set non-NULL **only then**.

A port that stages the unmodelled cells to zero (the row-08/OL-82 convention)
would **not** install `0x0D` (the RNG threshold is 0 and the pairing test fails),
so setting the slot would be an **un-dispatched fake count** — forbidden by
R15/R45. **Do not wire row `0D` standalone.**

**M2 preservation.** Row `0D` never installs on the tape (the install branch is
gated on `[rec+0x3D]==2` and the unmodelled cells), and the engine never runs arm
1, so a **non-wire** keeps M1 `09b726b7…` and M2 `b281a9b2…` byte-identical (no
re-pin). This slice changes no code and is tape-visible nowhere.

---

## 8. Residual legs

1. **FU256-L1 — row-09 arm 1 body (`0x80BFB..0x80FCB`) unported.** Carries the
   sole `0x0D` installer `0x80F61` and the `0x0C` installer `0x80EFA`. Owner
   **FU178-L1** (the live sibling of this leg). The arm is enterable (arm 0 seeds
   stage 1) but the engine parks at stage 1 (`mr->match09_leg_stage`).
2. **FU256-L2 — the `0x0D`-install-enabling producers.** `[[rec+4]+0x16]` (the
   `[rec+4]` descriptor byte; FU167/FU168-class binding) and `[rec+0x24]` (the
   reciprocal pairing/owner pointer). **No producer located** in this session or
   FU-204 §4.2. Owner **FU204-L2** (carried; new).
3. **FU256-L3 — the `[rec+0x3D]==2` frame condition.** The cell is live (OL-80
   closed), but whether the row-09 record's FU-84 frame ever equals 2 (and thus
   un-stalls the arm) is unshown. Owner **OL-80 / FU-141** (the FU-84 bank
   staging), with the arm-1 host.
4. **FU256-L4 — the arm-1 helpers/sinks.** `0x8F188` (event ring, session gate
   `[0x14C32A]`/`[0x14C312]`; FU-155), `0x8ED40`, `0x92820` (OL-27), `0x7A490`
   (OL-62), `0x8A3FC` — all named, unported, **not** install gates. Owner
   **FU180-L8** (extends) / FU204-L4.
5. **FU256-L5 — the row-`0D` body sinks.** `0x702F8` (arm-0 ball parameter),
   `0x6E598` (anim selector), `0x79C50` (face); `0x8DCD4` is already ported
   (`fifa96_arm_dist_stage`). Owner **FU180-L8** (FU-180 §11 erratum).
6. **FU256-L6 — the native `[rec+0x24]` producer reachability.** Whether the
   pairing pointer is ever non-NONE on a live record (i.e. the native install ever
   fires) is unenumerated here; on the tape it is unobserved (the engine never runs
   arm 1). Owner: the pairing/possession cluster (FU-139/FU142 class).

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info` (FIFA96.EXE, watcom LE 32, image base 0, one
`.image` overlay, 2706 fns / 15239 syms); `read_memory` `0x809F0` (16 B),
`0x1106E0` (64 B), `0x8250C` (24 B); `get_xrefs_to` `0x80BFB` (2),
`0x15877C` (7), `0x8251C` (1), `0x7D9A4` (77); `get_xrefs_from` `0x80A37` (5);
`disassemble_bytes` `0x80BFB` (0x80BFB..0x80FCA, 281 insns), `0x80A00` (64 B),
`0x80BB0` (80 B), `0x80FA4` (39 B), `0x8103A` (44 B), `0x8251C` (122 insns),
`0x8F188` (80 B), `0x8ED40` (96 B), `0x92820` (96 B), `0x7A490` (96 B),
`0x8A3FC` (80 B); `search_byte_patterns` `ba0d000000` (9 default-space + overlay
false positives); `search_instructions` `MOV EDX,0xd` (21). Engine/loader source
read first-hand: `src/fifa96_engine/fifa96_match_handlers.c` (4075..4182,
5460..5475), `src/fifa96_loader/fifa96_action_handlers.c` (1410),
`include/fifa96_engine/fifa96_match_entities.h` (1..150),
`tests/test_engine_match_handlers.c` (85), `tests/test_action_handlers.c` (322).
Docs cross-cited: FU-178 (§2/§4.1/L1/L6/L7), FU-180 (§1.2/§3/§9/L8/§11), FU-204
(§4), FU-245 (§1/§2), FU-243 (§2.4), FU-190 (§6.1), FU-174, OL-80/OL-82. No
writes beyond this draft and the workspace report; no git; ISO/`game/hdd/`
untouched.
