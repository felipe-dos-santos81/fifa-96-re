# FU-169 — the row-0x5E live installer (R17, phase-14 recon)

**Question (phase-14 plan R17, FU-160 rec #2).** Who installs anim row `0x5E`
in live play, and when? Derive the producer chain from the kick/receive paths
into `FUN_0006E598` (engine `fifa96_match_run_record_anim_install`) and the
frame-wrap successor, and give the exact engine wiring rec.

All Ghidra reads in this slice are fresh, program **`/FIFA96.EXE`**
(case-sensitive), flat LE-image addresses, standard MCP tools only; the GO
program reports one overlay space (`.image`, used once below only to classify
a duplicate byte match — all code cites are the `ram` space). Byte cites are
from fresh `disassemble_bytes` / `decompile_function` / `read_memory`.

**Headline results.**

* **One installer site.** The only code that installs anim row `0x5E` is the
  action-`0x0C` contact-decision body's stage-0 arm, `0x81DA4..0x81DB9`
  (`MOV EDX,0x5E; CALL 0x6E598`, bytes `ba5e000000` / `e8dac7feff`). A fresh
  `search_byte_patterns ba 5e 00 00 00` finds exactly that site in the `ram`
  space plus its pre-relocation twin in the `.image` overlay (the same
  instruction stream with the file-offset global form `[0x05888F]` — not an
  independent site), and `66 ba 5e 00` is 0 matches; the fresh 84-site
  `FUN_0006E598` census carries the single `0x5E` id (`0x81DB9`).
* **The install target is A = `[0x15888F]`** (the contact initiator stored by
  the registrar `FUN_0008A3FC`, FU-150 E2), **not** the record running the
  body (`EBP` = the challenged record B = `[0x158893]`). The arm fires when
  A is in anim row `0x59` at frame `>= 2` and the 0x795B4 band between
  `EBP.pos` and **A.pos + 2*A.vel** is `>= 0x30`; it increments A's team
  `[+0x7CF]` stumble counter, fires commentary event `0xA2` on every 7th, and
  resets EBP. Values `< 0x30` take the foul path instead.
* **Who starts it: the action-`0x0B` duel body** (`0x81908..0x81C71`). Arm 0
  installs anim **`{0x0C, 0x59}` on itself** via the exact choice the loader
  already ports as `fifa96_action_sequence_duel_event`; arm 1 registers the
  contact (kind 2) and installs **action `0x0C` on the found opponent**
  (`0x81BEB`). Both live producers of the action-`0x0B` install already exist
  engine-side: the pad tackle/press input arm (`match_input_handler` case
  `0x7CE38` ← native `0x7CE93`) and the row-04 rare arm (`fifa96_outfield.c:599`
  ← native `0x7ED6A`). The second action-`0x0C` producer (action-`0x09`
  stage 2, `0x80EFA`, invoke-now) reaches the same body but cannot install
  `0x5E` on its own (its A has no anim-`0x59` state without a prior duel).
* **When:** the challenged record B holds action `0x0C` at stage 0 and
  **retries every tick** (all failing gates return at the bare tail `0x82502`);
  the install fires on the first tick at which A's dive has reached frame
  `>= 2` and the band gate passes, then B resets the same tick. A then plays
  row `0x5E` (f0 aux `0x0E` impulsion) and the already-ported frame-wrap
  successor chains `0x5E -> 0x0F -> 0x00` (row44 `0 -> 0 -> 1`).
* **Engine:** the whole chain is **two new action bodies + two wires**. Every
  helper already exists — `fifa96_entity_intercept_band` (the `0x795B4` port),
  `fifa96_action_sequence_duel_event` (the `{0x0C,0x59}` choice),
  `fifa96_entity_angle`/`fifa96_entity_face`, `fifa96_match_entities_install`
  (the `FUN_0007D9A4` port), `fifa96_match_run_record_anim_install`, the
  referee A/B mirror, the opponent-team nearest search. Census 22 -> 24
  (`0x0B` + `0x0C`), README `floor(100*24/80) = 30%` in the same commit.
* FU-160 §4 rec #2's dependency is **inverted** for this path (recorded as a
  discrepancy, §9.3): the `0x5E` install does not need a live caller of
  `fifa96_match_run_contact`; it needs the action-`0x0C` body entry (the
  action install from the duel), and `fifa96_match_run_contact` is only the
  `< 0x30` foul branch.

---

## 1. Method

1. Framing the dispatch surfaces first: the **action** table `0x1106E0` (fresh
   `read_memory`, [`0x0B`] = `0x00081908`, [`0x0C`] = `0x00081C90`) selects the
   per-record behaviour body (the "row-0x0B/0x0C body" wording of FU-82/FU-81),
   while **anim** rows (`0x10EF00`) select the animation. `FUN_0007D9A4`
   (fresh `get_xrefs_to` = the FU-142 census's **77** call sites) is the action
   installer; `FUN_0006E598` (84 call sites, FU-159 §3.3) is the anim-row
   installer. This slice's whole answer lives in the action-0x0B/0x0C pair.
2. Fresh disassembly of the full action-0x0C stage-0 window
   (`0x81C90..0x81E10`), the action-0x0B body in three windows
   (`0x81908..0x81970`, `0x819C2..0x81AD0`, `0x81AD0..0x81C75`), the
   registrar callers (`0x81BC0..0x81BF5`, `0x80E80..0x80F10`), and the two
   action-0x0B install sites (`0x7CE30..0x7CEA5`, `0x7ED30..0x7ED85`).
3. Fresh helper decompiles: `0x795B4`, `0xCD514`, `0xCE386`, `0xCD474`,
   `0x8DD70`, `0x8ED40`; fresh `read_memory` of the action table, the row
   table (rows `0x0C`/`0x0D`/`0x0E`, `0x59`..`0x5E`), and the frame tables
   `0x10E3AA` (row `0x0C`), `0x10E3E6` (row `0x5E`), `0x10E432` (row `0x0F`),
   `0x10E48A` (row `0x12`), `0x10ED3E` (row `0x59`), plus the sine table at
   `0x114E04`.
4. Fresh `get_xrefs_to 0x8A3FC` (2 callers) and `search_byte_patterns` for the
   `0x5E` immediate forms; engine-side reachability from the ported sources
   (`src/`, read-only).

---

## 2. The anim-0x5E install arm (fresh re-verification of FU-160 §1.1)

### 2.1 The body head, the stage-0 arm, the tail (`0x81C90..0x81DC5`, `0x81DCF..0x81E0E`, `0x824F0..0x82520`)

```
0x81C90  prologue; EBP = rec; SUB ESP,0x3C
0x81C99  no phase gate
0x81C9D  EDX = [EBP+0x89]; AX = [0x157A64]; EDX += EAX; [EBP+0x89] = EDX  ; timer89 += delta
0x81CAB  EBX = [EBP+0x5D]                ; pos_y
0x81CB4  if (pos_y == 0) target := pos (MOVSD x3, +0x4D <- +0x59)
0x81CC1  AL = byte[EBP+0x92]; if > 6 -> tail (0x82502)
0x81CD4  JMP dword CS:[EAX*4 + 0x81C74]  ; stage table 0x81C74 (7 arms)
--- stage 0 arm (0x81CDC) ---
0x81CDC  CMP byte[EBP+0x8D],0; 0x81CE3 JZ 0x824FB   ; index 0 -> reset (0x824FB) at once
0x81CE9  ECX = [0x15888F]; if 0 -> 0x81DCF          ; A = the contact record
0x81CF7  EDX = [ECX+0x28]; DL = byte[EDX]; EDX &= 0xFF
0x81D04  CMP EDX,0x59; 0x81D07 JNZ 0x81DCF          ; A's anim row == 0x59 ?
0x81D0D  EDX = dword[ECX+0x3A]; SAR 0x18             ; = byte[A+0x3D] (the frame, alias-safe)
0x81D13  CMP EDX,2; 0x81D16 JL 0x82502              ; frame < 2 -> retry tail
0x81D1C  stack := A.pos (3 MOVSD)
0x81D24  EDX = dword[ECX+0x71]; SAR 0x10; stack.x += 2*EDX   ; effective word = A+0x73 (vel73)
0x81D34  EAX = dword[ECX+0x73]; SAR 0x10; stack.z += 2*EAX   ; effective word = A+0x75 (vel75)
0x81D48  EAX = &EBP+0x59; EDX = ESP (the A lead triple); EBX = ESP+0xC
0x81D4F  CALL 0x795B4                     ; out[0]=band, out[1]=dx, out[2]=dz
0x81D54  EAX = word[ESP+0xC] (out[0]); CMP EAX,0x30; 0x81D5E JL 0x81DFE  ; < 0x30 -> foul path
0x81D64  EAX = [0x15888F]; EAX = [EAX] (A's team); INC dword[EAX+0x7CF]
0x81D71  re-read; IDIV EBX=7; 0x81D8D JNZ 0x81DA4    ; counter%7 != 0 -> skip event
0x81D8F  CMP dword[EBP+0x20],0; 0x81D93 JZ 0x81DA4   ; no slot -> skip event
0x81D95  EBX=4; EAX=0xA2; 0x81D9F CALL 0x8F188       ; commentary event 0xA2 (sink, FU159-L3 family)
0x81DA4  EAX = [0x15888F]; EDX = 0x5E
0x81DAE  ECX = [EAX+0x8B]; SAR 0x18      ; dead load (overwritten by the installer head)
0x81DB4  XOR EBX,EBX                      ; frame argument 0
0x81DB9  CALL 0x6E598                     ; install row 0x5E on A, bytes e8dac7feff
0x81DBE  EAX = EBP; CALL 0x7DAB4         ; reset the action-0x0C record (not A)
0x81DC5  ADD ESP,0x3C; pops; RET
--- the alternate gate (reached when A == 0 or A's row != 0x59) ---
0x81DCF  EAX = [0x15888F]; row = byte[[EAX+0x28]]
         if (row == 0x11) frame = byte[EAX+0x3D]; CMP 2
         else             frame = byte[EAX+0x3D]; CMP 3
0x81DF8  JL 0x82502                       ; below threshold -> retry tail
0x81DFE  ... the foul path head (A row re-read; `CMP EDX,0x11` at 0x81E0E)
--- tails ---
0x824F2  CMP [EBP+0x89],0x3C; JLE 0x82502 ; stage-6 arm: wait, then reset
0x824FB  EAX = EBP; CALL 0x7DAB4          ; the reset (index 0 / stage 6)
0x82502  epilogue RET                     ; no reset, no stage change -> the caller retries
```

Confirmed against FU-160 §1.1 with two corrections (§9.1, §9.2): the lead-point
loads are **dword** loads shifted 16 (the effective words are `+0x73` vel73 and
`+0x75` vel75), and the `0x81D24/0x81D34` stack triple is
`A.pos + 2*A.vel` (the 2-frame lead).

### 2.2 The band: `FUN_000795B4` / `FUN_000CD514` (fresh)

Fresh `disassemble_bytes 0x795B4..0x795E0` and `0xCD514..0xCD560`:

```
0x795B4  ECX = EAX (a); ESI = EBX (out)
0x795BB  AX = word[b] (EDX); BX = word[a] (ECX); SUB; word[out+2] = AX   ; dx = b.x - a.x
0x795C7  AX = word[b+8]; DI = word[a+8]; SUB; word[out+4] = AX           ; dz = b.z - a.z
0x795D5  EAX = dword[out+2] >> 16 (= out[2], dz); PUSH
0x795DC  EAX = dword[out]   >> 16 (= out[1], dx); PUSH; CALL 0xCD514
...      word[out] = AX (the CD514 result)
0xCD514  EBP frame; CALL 0xCD474(dx, dz)          ; the atan table 0x14072C
         |angle|; if > 0x100 -> 0x200 - angle
         if angle < 0x81: divisor = CE386(angle); num = |dx|
         else:            divisor = CE364(angle); num = |dz|
         return (num << 16) / divisor
0xCE386  a 257-entry table lookup into DAT_00114E04 with the quadrant fold
```

`read_memory 0x114E04` (fresh): `00000000 92010000 24030000 b6040000 48060000
da070000` — entries 0, 0x192, 0x324, 0x4B6, 0x648, 0x7DA (step `0x192` = the
1024-step circle's 16.16 sine). So `out[0]` is the **vector length** of
`lead - EBP.pos` in whole word units (|dx|/sin or |dz|/cos via the fold), not
an angle — FU-160 §1.1's "polar magnitude" stands; FU-82 §3.3's "angle word"
is a misnomer (discrepancy §9.2). The engine **already ports this function**:
`fifa96_entity_intercept_band(pos_x, pos_z, target_x, target_z, out)` with
`out.band` = the quotient (`fifa96_entity_update.c:145-165`, FU-151 port).

---

## 3. The producer chain

### 3.1 Action `0x0B` (the duel body, `0x81908..0x81C71`) — arm 0, the `{0x0C,0x59}` choice

Fresh head (`0x81908..0x81970`): prologue, `SUB ESP,0x1C`, `EBP = rec`;
`AL = [0x157A4A] >> 24` (phase); **phase != 2 -> reset** (`0x81924 JNZ
0x81C62`); stage `+0x92` dispatch: `< 1` -> arm 0 at `0x81951`, `== 1` -> arm 1
at `0x81B1F`, `== 2` -> arm 2 at `0x81C55`, else tail. Arm 0 requires
`byte[EBP+0x8D] != 0` (`0x81951/0x81958`) else reset.

Arm 0 (`0x819C2..0x81B19`, fresh):

```
0x819C2  stack := rec.pos; rec.pos + [0x10F334+oct]<<5 / [0x10F339+oct]<<5
         (oct = byte[+0x8E] via dword[+0x8B]>>24; the dir_x/dir_z byte tables)
0x81A06  ECX = out; EDX = [rec] -> [team+0x7A6] (the opponent block); EBX = -1
0x81A14  CALL 0x8DE8C                     ; nearest opponent to the derived point
0x81A21  CALL 0x795B4(rec.pos, found.pos, ESP+0xC)
0x81A30  EAX = out[0] (band); CMP 0x20; JLE reset; CMP 0x70; JGE reset   ; (0x20,0x70)
0x81A49  EAX = dword[EBP+0x6F]>>16 (= word[+0x71], speed71); CMP 4; JLE choice 0x0C
0x81A58  EDX = out[2] (dz); EAX = out[1] (dx); EBX = word[+0x7D] (facing)
0x81A6C  CALL 0x8DD70                     ; the atan helper (FUN_000CD474)
0x81A72  EBX = facing - atan; |..| < 0x1000 -> choice 0x59, else 0x0C
0x81ABF  MOV dword[ESP+0x14], 0x59  /  0x81AC9: MOV dword[ESP+0x14], 0x0C
0x81ADA  CALL 0x79B6C(rec, found.pos)     ; the face write
0x81AE7  EDX = word[ESP+0x14] (the choice; dword[+0x12]>>16 alias)
0x81AFB  CALL 0x6E598(rec, choice, frame 0)   ; install anim 0x0C or 0x59 on SELF
0x81B00  byte[+0x9E] = 1; timer89 = 0; stage -> 1
```

So arm 0 picks the **standing contact anim `0x0C`** when `speed71 <= 4` or the
facing is off by `>= 0x1000`, else the **dive `0x59`**; the band window
`(0x20,0x70)` means the duel only engages at medium range. The ring-post
branch (`[[rec+4]] == 0x18DE`, `0x81961..0x819B3`) installs anim `0x48`/`0x68`
and resets — a separate path, no `0x5E`.

### 3.2 Arm 1 (`0x81B1F..0x81C4F`) — the registrar + the action-0x0C install

```
0x81B1F  CMP byte[EBP+0x3D],0; 0x81B23 JZ tail     ; wait: the anim frame must be != 0
0x81B29  nearest opponent to rec.pos again (EBX = -1, [team+0x7A6], out ESP+0x18)
0x81B43  BL = byte[found+0x8D]; JZ reset           ; found must have an index != 0
0x81B53  EAX = word[ESP+0x18] (out[0] = the band); CMP 0x60; JG reset
0x81B63  if (choice == 0x59) { CMP EAX,0x30; JG reset }   ; the dive must be close
0x81B73  CMP byte[0x14C32A],0; JNZ reset           ; the session gate (FU-155)
0x81B80  INC dword[team+0x7D7]                     ; the duel counter
0x81B89  if (counter % 10 == 0 && [EBP+0x20] != 0) -> event 0xA1 (EBX=4)
0x81BB7  CALL 0x8ED40(rec, -3)                     ; the impact accumulator (leg FU169-L2)
0x81BC3  ECX = &found+0x59; EAX = 2; EBX = found; EDX = rec
0x81BCF  CALL 0x8A3FC                              ; registrar: kind 2, A = rec, B = found
0x81BD4  EDX = 0xC; AL = byte[rec+0x8E]; ECX = 0; EBX = 0
0x81BE3  byte[found+0x8E] = AL                     ; copy the facing octant
0x81BEB  CALL 0x7D9A4(EAX=found, EDX=0xC, ECX=0)   ; INSTALL ACTION 0x0C ON FOUND (no invoke)
0x81BF0  side sinks: FUN_000741B4, FUN_00092040(0xC8,0/1), FUN_000651F0(4 or 1),
         FUN_000974F0(0x258); timer89 = 0; stage -> 2
0x81C55  arm 2: FUN_00079B1C(rec) snap; if (byte[+0x44]) reset; else tail
```

Arm 1 fires **once**, on the first tick where the duel anim's frame index is
non-zero, and installs the contact decision **on the opponent**. Arm 2 then
snaps the record and resets it as soon as the anim row becomes terminal
(byte44 = row flags bit 0, FU-159 §4.1).

### 3.3 The two live producers of the action-0x0B install

Fresh `get_xrefs_to 0x7D9A4` = 77 (FU-142f census reproduced); the census
places the two `MOV EDX,0xB` sites inside the **input row** and the **row-04
body**:

1. **The pad tackle/press input arm** (`0x7CE93`), fresh bytes
   `0x7CE38..0x7CEA4`:
   ```
   0x7CE3E  phase != 2 -> skip
   0x7CE4B  type bits: byte[+0x91] (via dword[+0x8E]>>24) indexes 0x110680; & 1 -> must be set
   0x7CE63  CMP byte[ESI+0x99],0; JNZ skip       ; the receiver timer gate
   0x7CE6C  anim row [[rec+0x28]]: == 0xC / 0x59 / 0x5E -> skip   ; the block list
   0x7CE85  ECX=1; EDX=0xB; EAX=ESI; XOR EBX,EBX; 0x7CE93 CALL 0x7D9A4
   ```
   The engine already ports this arm verbatim:
   `match_input_handler` case `0x7CE38u` (`fifa96_match_run.c:693-700`, the
   same `anim_id == 0x0C/0x59/0x5E` block list) behind the input rule table
   `{0x07FF, 0x0060, 0x7CE38}` (`fifa96_outfield.c:10`), and the machine runs
   per outfield record (`match_run.c:1377`). **The engine can already install
   action 0x0B live; the row body is what is missing.**
2. **The row-04 rare arm** (`0x7ED6A`), fresh bytes: `(EAX - EDX) vs
   [team+0x7D7]` gate, `(byte[desc+0xB] | byte[+0x9D])` threshold against
   `rng & 0x1F`, then `ECX=1; EDX=0xB; EAX=EBP; 0x7ED6A CALL 0x7D9A4`. The
   engine already emits it (`fifa96_outfield.c:589-601`,
   `row04_add_install(out, SELF, 0x0B, 0, 1)`).

### 3.4 The second action-0x0C producer (action `0x09` stage 2, `0x80EFA`)

The action-`0x09` arm table is `0x809F0` = `{0x80A3F,0x80BFB,0x80FCC,0x8103A}`
(FU-81 §2.1), so `0x80EE7`/`0x80EFA` sit in **stage 2**. Fresh bytes
`0x80E80..0x80F10`: the facing-word difference `word[ESI+0x7D] -
word[EBP+0x7D]` folded through `AH & 3`/`0x400` to `(...) < 0x80`; a
kind mask `(12*desc + angle < 0xFF)` rolled against `0x92AC8`; then
```
0x80EE7  CALL 0x8A3FC(kind 0/1, A = EBP, B = ESI, point = &ESI+0x59)
0x80EF1  EDX = 0xC; ECX = 1; EAX = ESI; XOR EBX,EBX
0x80EFA  CALL 0x7D9A4               ; install action 0x0C on ESI, INVOKE-NOW
```
This reaches the same action-0x0C body but with A = the action-0x09 record;
the `[0x15888F]`-in-row-`0x59` gate can only hold there if a *prior* duel set
that anim (the FU-159 §3.3 census finds only the anim-`0x11` install in the
action-`0x09` body range, `0x80BC5`). So this producer does **not** originate a
`0x5E` install on its own; it is the alternative action-0x0C entry (leg
FU169-L1).

### 3.5 Who is A, who is B

`FUN_0008A3FC` (`0x8A3FC..0x8A43B`, FU-150 E2; fresh `get_xrefs_to` = exactly
`0x80EE7` and `0x81BCF`) stores `contact_kind [0x15888C]`, A `[0x15888F]`,
B `[0x158893]`, point `[0x158897]`. In both duel producers the **initiator is
A** and the **challenged record is B**; the action-0x0C decision body runs on
B and the anim-`0x5E` install lands on A. The `EBP == [0x158893]` gate inside
the foul path (FU-160 §1.1, fresh `0x81E0E CMP EDX,0x11` context) is the
consistency check. The engine already mirrors the cells:
`fifa96_ref_contact_register` writes `state->rec_first` / `rec_second` /
`contact_kind` / `point` (`fifa96_referee.c:25-43`).

---

## 4. When — the tick sequence

1. Tick T: the input arm (invoke-now) installs action `0x0B` on rec; the
   same-tick dispatch runs the duel body stage 0.
2. Tick T (same): arm 0 gates (phase 2, index, band `(0x20,0x70)`), picks
   `{0x0C, 0x59}`, installs that **anim row on rec at frame 0**, faces toward
   the found record, sets `ran`, `timer89 = 0`, stage -> 1.
3. Tick T+1..: arm 1 waits while `byte[rec+0x3D] == 0` (the anim has not
   advanced yet). On the first tick where the frame is non-zero: the band gate
   `<= 0x60` (`<= 0x30` for the dive), `[0x14C32A] == 0`, the counter/event,
   `0x8ED40`, the registrar, and the **action-0x0C install on the found
   record** (no invoke -> effective next tick), then stage -> 2.
4. Tick T+2..: the found record runs action `0x0C` at stage 0. The failing
   gates (A row != `0x59`, A frame < 2, the alternate-gate thresholds) return
   at the bare tail `0x82502` **without a reset or a stage change**, so the
   body retries every tick while `timer89` accumulates (the body-record index
   `0` path is the only one that resets, `0x824FB`, and it is the keeper case;
   the stage-6 arm waits `timer89 > 0x3C` then resets). The install fires on
   the first tick where **A is in anim row `0x59` at frame `>= 2` and the lead
   band `>= 0x30`**; the same tick increments A's team `[+0x7CF]`, possibly
   fires event `0xA2`, installs anim row `0x5E` (frame 0) on A, and resets
   the found record.
5. The window in which the gate can succeed is A's dive frames `2..6`
   (row `0x59` = `{last 6, flags 0, next 0x00}`, fresh at `0x10F221`); after
   the row's successor installs row `0x00` the `0x59` gate fails forever and
   the found record's body keeps retrying until its own action ends by other
   means (a leg: the retry's exit conditions beyond A's row, FU169-L4).

The engine's `fifa96_match_entities_install` gives the equivalent staging
(code, `timer89 = 0`, `stage92 = staged`, `ran = 0`), so step 4's retry needs
no new mechanism: the handler simply returns without advancing `r->stage92`.

---

## 5. The frame-wrap successor (`0x5E -> 0x0F -> 0x00`)

Fresh row table (`0x10F24E`): row `0x5E` = `5E 05 00 0F E6 E3 10 00 0C`
`{last 5, flags 0x00, next 0x0F, frame table 0x10E3E6, bank 0x0C}` — FU-159
§5.2's `0x5E {5,0,0x0F}` confirmed. Frame table fresh (`0x10E3E6`, 6 x 5 B):

| frame | dur | aux | b4 | arm |
|---|---|---|---|---|
| f0 | 0x0040 | 0x000E | 1 | the aux-0x0E impulsion (case 0xD) |
| f1 | 0x0080 | 0x002A | 1 | the carry (row set `{0x4D,0x59,0x11,0x5E}` -> `<<2`) |
| f2 | 0x0064 | 0x032B | 2 | the row-0x5E `<<2` + frame-freeze arm (inert at `jump_base == 0`) |
| f3 | 0x0080 | 0x000C | 3 | the grounded target step |
| f4 | 0x0080 | 0x000C | 4 | the grounded target step |
| f5 | 0x0140 | 0x0005 | 5 | the snap (sustained) |

The contrast with the row-`0x12` jump (fresh `0x10E48A`): row `0x12` carries
the same aux-0x0E arm at **f2** (after the `0x0A` wind-up frames `0x40/0x40`),
so the `0x12` jump waits two frames for its impulsion while row `0x5E` fires
it on the install frame — the `0x5E` "get-up carry". Row `0x0F` (fresh
`0x10E432`) is five `0x05` snap frames, and row `0x00` is the idle stub, so
the chain `0x5E -> 0x0F -> 0x00` lands the record in a terminal row
(row44 `0 -> 0 -> 1`, the FU-159 §5.2 successor) — **already ported** as
`fifa96_match_run_record_anim_successor` (`fifa96_match_run.c:1024-1070`).

---

## 6. Engine reachability (what is live today)

* **Action 0x0B producers: live.** The input tackle arm (`0x7CE38`) and the
  row-04 arm (`0x7ED6A`) are both ported and wired; an install is staged to
  `e->code = 0x0B` (invoke path re-dispatches the same tick,
  `match_run.c:1377-1392`) and the dispatch returns
  `-FIFA96_ERR_UNSUPPORTED` (`fifa96_match_action_table[0x0B] == NULL`,
  `fifa96_match_handlers.c:2960`).
* **Action 0x0C producers: half-live.** The action-`0x09` body (which would
  install it) is unwired (`:2963`); the action-`0x0B` arm-1 install cannot run
  because the body is unported. `fifa96_match_run_contact`
  (`fifa96_match_run.c:5155`, the foul-path port) still has **no live caller**
  (tests only, FU-160 §1.5 verified again).
* **Anim-0x5E install: unreachable** (the loop above); `fifa96_match_run_record_anim_install`
  exists and its test stand-in for `0x5E` was already replaced by the helper
  (FU-159 status, phase-13).
* **Successor/row44: live** for the exit chain (§5).
* The engine's `team_7d7` is staged 0 (`fifa96_match_handlers.c:2463`,
  "producer unported (OL-72)") — the duel arm 1 is that producer.

---

## 7. T1 wiring recommendation

**Rec #1 — port the action-`0x0B` duel body + the action-`0x0C` stage-0
contact body as `fifa96_match_action_0B` / `fifa96_match_action_0C`, wire both
rows (the live row-0x5E installer).** This is the direct, minimal chain: the
two action-0x0B install producers are already live (§3.3), and every helper the
two bodies need already exists in the port:

* `fifa96_action_sequence_duel_event` (`fifa96_action_handlers.c:1338`) is
  **the arm-0 choice verbatim** — band window `(0x20,0x70)`, `<= 4 -> 0x0C`,
  then the `0x1000` facing compare -> `0x59`/`0x0C`. (Its parameter names
  `delta_angle`/`aim` are FU-82-era misnomers: pass `out.band` and
  `speed71`.)
* `fifa96_entity_intercept_band` (`fifa96_entity_update.c:145`) = the
  `0x795B4` band — use for both arm-0's precheck and the action-0x0C `0x30`
  gate with the `A.pos + 2*A.vel` lead (`e->vel73`/`vel75`).
* `fifa96_entity_face` (`0x79C50`, writes `face7d` + sector) and
  `fifa96_entity_angle` (`0xCD474`) for the facing gates; the engine already
  has `face7d` staged (`fifa96_match_entities.h:144`).
* `fifa96_entity_find_nearest` over `mr->entities.team[opp]` (the `0x8DE8C`
  call with `EBX = -1`); the engine's candidate convention is already used by
  the offside arm.
* `fifa96_ref_contact_register` for A/B/kind/point; `mr->session_gate_14c32a
  == 0` for `[0x14C32A]`; `fifa96_match_run_record_anim_install` for the anim
  installs; `fifa96_match_entities_install` for the action-0x0C install.
* New pool/team pieces: the per-team `[+0x7CF]` stumble counter (+ reset in
  the team-init mirror, `0x8C2E3`), the `team+0x7D7` duel counter increment,
  the event `0xA2` and `0xA1` request bits.

Concrete shape:

* `fifa96_match_action_0B`: phase 2 else `match_row_reset`; stage dispatch;
  arm 0 (index gate, derived point `pos + dir<<5`, nearest opponent, band
  window, `fifa96_action_sequence_duel_event`, `fifa96_entity_face`, anim
  install on self frame 0, `ran`, `timer89 = 0`, stage -> 1); arm 1 (frame
  `!= 0` gate, nearest again, `[found+0x8D]`, band `<= 0x60`/`0x30`,
  `session_gate == 0`, `++team_7d7`, `%10` event `0xA1`, `0x8ED40` leg,
  registrar, copy `type`, `fifa96_match_entities_install(found, phase, 0x0C,
  0)`, sink request bits, stage -> 2); arm 2 (`match_run_record_snap` — needs
  a non-static wrapper — plus `row44 != 0 -> match_row_reset`).
* `fifa96_match_action_0C`: head (`timer89 += delta`, the `pos_y == 0` target
  copy, stage dispatch); stage 0 exactly as §2.1 (index gate -> the `0x3C`
  wait/reset; A = `mr->referee.rec_first`; A anim `0x59` & frame `>= 2`; band
  >= `0x30` -> counter/event/anim-`0x5E` install on A + `match_row_reset`(self);
  `< 0x30` -> the foul branch via the existing `fifa96_match_run_contact`
  (either call it with the registered A/B/point — a disclosed re-register —
  or split a decide-only entry); else the alternate gate (A row `0x11`
  f `>= 2` / any f `>= 3` -> foul branch; else return = retry). Stages 1..6
  (`0x81F4E..0x8251B`) stay legs.
* Wire `{0x0B, ...}`/`{0x0C, ...}`; **census 22 -> 24** (`0x0B`, `0x0C`),
  README `floor(100*24/80) = 30%`, ENGINE.md census line in the same commit.
* Tests (RED first): unit — a staged action-0x0B duel (pool rec + opponent at
  band `(0x20,0x70)`), assert the `{0x0C,0x59}` choice by speed/facing, the
  anim install, then the frame-`!= 0` arm-1 registrar + the action-0x0C
  install on the found record; then the action-0x0C stage-0 gate (A in
  `0x59` f `>= 2`, band `< 0x30` -> foul vs `>= 0x30` -> anim `0x5E` on A +
  `row44` 0 + the `0x82502` retry), the `[0x14C32A]` gate, the block list;
  ISO/smoke — a live pad tackle produces `anim_id == 0x5E` and the successor
  chain `0x5E -> 0x0F -> 0x00` (`p14-t1-*`).
* Goldens: the tape never runs the input arms or the row-04 rare arm, so
  M1/M2 byte-exact expected; verify with `cmp`, re-pin only with a diff.

**Rec #2 — bounded fallback (deepest provable step).** If the duel body
overruns the T1 slice: land `fifa96_match_action_0C`'s stage-0 installer + the
`{0x0B}` wire only, with the action-0x0C install staged from a unit fixture
(direct `fifa96_match_entities_install(..., 0x0C, 0)` on the B record) — the
`0x5E` install then runs end-to-end at the unit level with A staged by the
existing anim helper; keep the action-0x0B arm 0/1 port as the numbered next
hop (FU169-L6). Do not invent a gate: the chain is proven to the boundary
above.

**Rec #3 — do not touch rows `0x14`/`0x5C`.** Unchanged (FU159-L2/FU160 §3).

---

## 8. Numbered legs (new)

* **FU169-L1 — the action-0x09 stage-2 action-0x0C producer.** `0x80EFA`
  (invoke-now) reaches the same contact body with A = the action-0x09 record;
  reachable `0x5E` only if a prior duel left that record in anim `0x59`. The
  action-0x09 body (`0x80A00..0x81067`) stays unported; secondary for T1.
* **FU169-L2 — the action-0x0B arm-1 side effects.** `FUN_0008ED40(rec, -3)`
  (fresh decompile: the per-player impact accumulator over the `0x15A998`
  table with the `0x15AFB0` max-tracking), `FUN_00092040(0xC8, side)`,
  `FUN_000651F0(4|1)`, `FUN_000974F0(0x258)`, and the `0xA1`/`0xA2` speech
  sinks (`FUN_0008F188`). Request bits suffice for T1; port for native-exact
  presentation.
* **FU169-L3 — the ring-post branch.** `[[rec+4]] == 0x18DE` at `0x81961`
  (descriptor dword): the engine stages the roster descriptor as 0
  (`player_d`/`player_e`, OL-65/OL-72), so the branch is provably unreachable
  in the engine; native fidelity needs the descriptor family.
* **FU169-L4 — the `0xC` body's retry exit.** When the A-side gate can never
  hold (A left row `0x59` before failing the band), the found record's action
  `0x0C` stays at stage 0 with no timeout (outfield records skip the `0x3C`
  branch); the native exit is whatever later state change resets it. Bound
  with a live trace or leave as the honest retry semantics.
* **FU169-L5 — the exact frame-timing window.** The dive frame `>= 2` gate
  against the frame durations (`0x40/0xC0/0x80/0x40/0x80/0xC0/0x180`) and the
  native timer increment `[0x157A62].hi * 0x10` (engine `frame_delta`); the
  tick count for the live window is fixture-verifiable, not asserted here.
* **FU169-L6 — the action-0x0B body port (if Rec #2 is taken).** Carried as
  the explicit next hop.
* **FU169-L7 — the `fifa96_entity_intercept_band` boundary exactness.**
  FU-151's equivalence claim ("verified equal to the sine primitive for every
  `a` in range") is inherited; the `0x30`/`0x60`/`0x70` gate boundaries should
  get a unit fixture before live gating depends on them.
* Carried unchanged: FU160-L1/L2/L3/L4, FU159-L1/L2/L5/L6 (L6 now answered
  for `0x5E` by §3: the live installer is the duel -> action-0x0C chain).

---

## 9. Discrepancies / errata

**9.1 — FU-160 §1.1's lead-point words (alias render).** FU-160 prints
"`EDX=word[A+0x71]`" / "`EAX=word[A+0x73]`" with a `SAR 16`; the fresh bytes
(`0x81D24 MOV EDX,dword[ECX+0x71]`, `0x81D34 MOV EAX,dword[ECX+0x73]`) show
**dword** loads shifted 16, so the effective words are `A+0x73` (vel73) and
`A+0x75` (vel75). The doc's semantic conclusion (`A.pos + 2*A.vel`) stands;
its operand printout is the project's known `dword[addr]>>16` class.

**9.2 — FU-82 §3.3's "angle word".** The `0x795B4` output word read by the
`(0x20,0x70)` / `0x60` / `0x30` gates is the **vector length** (the
`0xCD514` quotient over the `0x114E04` sine table, fresh §§2.2), not an angle;
FU-160 §1.1's "polar magnitude" was right. The helper's parameter names in the
port (`fifa96_action_sequence_duel_event`'s `delta_angle`, `aim`) inherit the
misnomer and should be re-documented when wired.

**9.3 — FU-160 §4 rec #2's dependency (inverted for the `0x5E` path).**
Rec #2 says the row-`0x5E` install "must go after a live caller of
`fifa96_match_run_contact` is wired". Fresh: the `0x5E` install is the
action-0x0C **stage-0 arm**, whose live entry is the **action-0x0C install**
(`0x81BEB`, from the action-0x0B duel; or `0x80EFA`, action 0x09);
`fifa96_match_run_contact` is only the `< 0x30` foul branch of the same arm.
The action-0x0B body is the real prerequisite.

**9.4 — FU-81 §Code `0C`'s "ball record `[0x5888F]`".** Fresh
`0x81CE9 MOV ECX,dword[0x0015888F]` — it is the **contact record A**
(`[0x15888F]`, FU-150 E2), not a ball record, and the frame gate is
`byte[+0x3D]` (via `dword[+0x3A] >> 24`), consistent with FU-160's derived
reading.

---

## 10. Evidence index (fresh this slice)

* `disassemble_bytes`: `0x81908..0x81970`, `0x819C2..0x81AD0`,
  `0x81AD0..0x81C75`, `0x81C90..0x81CDC`, `0x81CDC..0x81E10`,
  `0x824F0..0x82520`, `0x81BC0..0x81BF5`, `0x80E80..0x80F10`,
  `0x7CE30..0x7CEA5`, `0x7ED30..0x7ED85`, `0x795B4..0x795E0`,
  `0xCD514..0xCD560`, `0x79C50..0x79C60`.
* `decompile_function`: `0x795B4`, `0xCD514`, `0xCE386`, `0xCD474`,
  `0x8DD70`, `0x8ED40`.
* `get_xrefs_to`: `0x8A3FC` (**2**), `0x7D9A4` (**77**).
* `search_byte_patterns`: `ba 5e 00 00 00` (1 `ram` + its `.image` overlay
  twin), `66 ba 5e 00` (0).
* `read_memory`: `0x1106E0` (56 B action table), `0x10EF6C` (rows
  `0x0C..0x0E`), `0x10F221` (rows `0x59..0x5E`), `0x10E3AA` (row `0x0C`
  frames), `0x10E3E6` (row `0x5E` frames), `0x10E432` (row `0x0F` frames),
  `0x10E48A` (row `0x12` frames), `0x10ED3E` (row `0x59` frames),
  `0x114E04` (24 B sine table), `.image::0xdf7f8` (the pre-relocation twin
  form `a1 8f 88 05 00`).
* Engine/port cites: `src/fifa96_engine/fifa96_match_run.c:686-770` (the
  input handler case `0x7CE38` + block list), `:1024-1070` (successor),
  `:1001-1021` (install), `:1377-1392` (the invoke re-dispatch),
  `:5155` (`fifa96_match_run_contact`, test-only), `:1208` (the aux-0x0C
  arm); `fifa96_match_handlers.c:2939-2965` (action table `0x0B`/`0x0C`
  NULL), `:2463` (`team_7d7` staged 0); `fifa96_outfield.c:10` (rule
  `{0x07FF,0x0060,0x7CE38}`), `:589-601` (row-04 `0x0B` install);
  `fifa96_action_handlers.c:1338` (`fifa96_action_sequence_duel_event`);
  `fifa96_entity_update.c:145-165` (`fifa96_entity_intercept_band`);
  `fifa96_referee.c:25-43` (`rec_first`/`rec_second`);
  `fifa96_arm_helpers.c:73` (`fifa96_arm_face`); `fifa96_match_entities.c`
  (`fifa96_match_entities_install`); headers
  `fifa96_match_entities.h:141-144` (`face7d`), `fifa96_outfield.h:243-247`,
  `fifa96_entity_update.h:60-90`.

## Status

**Derived with byte cites; engine wiring rec ready.** FU-160 rec #2 is now a
concrete two-body/two-wire T1 plan (Rec #1) with a bounded fallback (Rec #2);
the `0x5E` installer chain, timing and successor are all first-hand. No repo
write beyond this slice + the R17 report; no git; no code. FU160-L4 (the
row-0x0E stage-2 else arm) is R15/FU-167's half and independent.
