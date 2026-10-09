# FU-151 — goalkeeper & AI team logic

Provenance: recon draft w7-b3, phase-7 wave-7, frozen 2026-10-09; evidence
review: PASS after correction pass. Read-only on authoritative program
**`/FIFA96.EXE`** (Ghidra MCP; never the flat bin, never `/fifa96.exe`). No
writes outside this draft; no commits. Fresh xref counts for every census
claim.

**Address model.** In `/FIFA96.EXE` code addresses equal the FU-doc link
addresses (verified: `get_function_by_address 0x782D0`/`0x8D8EC` exist); data
immediates render as true flat addresses (`0x15774C`, `0x1106E0`, `0x10F331`);
inline `CS:` jump tables hold already-relocated code addresses (fresh read:
table `0x754E4` = `{0x75611,0x7565A,0x75770,0x75795,0x75B90,0x75D17,0x75DF3,
0x75E82,0x7609D,0x760C6}` and each target disassembles cleanly at that
address). All `program` arguments are `/FIFA96.EXE`.

---

## 1. Scope

Extends FU-147 (frozen) on the two axes the phase-7 plan assigns to B3:

1. **Goalkeeper** — action/row 0x1E `0x7550C` **full 10-stage machine**
   (stage table `0x754E4`), action/row 0x1D `0x74EB0` **full 5-stage machine**
   (stage table `0x74E9C`), the keeper claim/dive/save bodies inside them, the
   **restart chain after a claim** (`0x75B36..0x75B67`, `0x7546E`), and the
   `record+0x9B` possession-flag lifecycle (fresh 9-site write census).
2. **AI team logic** — the `FUN_000795B4` producer (fresh 12-caller census,
   every call site classified), the AI-side mover fields
   (`+0x6B`/`+0x77`/`+0x6D`/`+0x6F`, `[team+0x7C7]` tracker fresh 11-site
   census), the team-level selection helpers (`FUN_0008DE8C`,
   `FUN_0008DDE0`, `FUN_0008D8EC` pre-pass, `FUN_0007997C`/`FUN_0008C33C`
   reset-lane path), and the FU-145 L3 possession-selection auxiliaries
   (`0x8DE8C/0x795B4/0x79C50/0x6E598/0x741B4/0x651F0/0x974F0/0x92040/
   0x974DC`).

Not re-derived (FU-147/FU-74/FU-75/FU-78/FU-79 own them): the action
installer `FUN_0007D9A4`, the record machines, the BF20 mover integral, the
keeper selection tail, FU-79's 7 keeper bodies and FU-75's tracker/ring.

## 2. Evidence floor

### 2.1 Fresh censuses

| query | result |
|---|---|
| `get_xrefs_to 0x754E4` (row-1E stage table) | **1** (`0x75609`, DATA) |
| `get_xrefs_to 0x795B4` | **12** (all UNCONDITIONAL_CALL, list §3.3) |
| `get_xrefs_to 0x8DE8C` | **37** |
| `get_xrefs_to 0x8A938` (situation entry) | **39** (matches FU-147) |
| `get_xrefs_to 0x740A0` | **27** |
| `search_instructions` MOV `+ 0x9b],` | **9** writes (234,866 insns scanned, not truncated) |
| `search_instructions` all-mnemonic `0x7c7]` | **11** sites (3 writers, 8 readers) |
| `search_instructions` MOV `EDX, 0x1D` | **1** match (`0x7F0BB`, *not* an install — §3.5) |
| `search_instructions` MOV `EDX, 0x1E` | match core none (`0x3F6B9` only, loader region) |

### 2.2 Row stage tables (raw, this slice)

`read_memory 0x754E4` (48 B, incl. following prologue bytes):

```
000756e4: 11560700 5a560700 70570700 95570700 905b0700
000756f4: 175d0700 f35d0700 825e0700 9d600700 c6600700
           ^^ 10 dwords, entries 0..9 -> 0x75611/0x7565A/0x75770/0x75795/
              0x75B90/0x75D17/0x75DF3/0x75E82/0x7609D/0x760C6
followed at 0x7550C by 53 51 52 56 57 55 89 e5 = PUSH EBX..EBP; MOV EBP,ESP
```

`read_memory 0x74E9C` (32 B): `{0x74F60, 0x75045, 0x7524B, 0x7529A, 0x7549B}`
(5 entries), followed at 0x74EB0 by the same 6-byte prologue. Both tables
match FU-79/FU-147; the row-1E table is fresh-confirmed here.

### 2.3 Row-1E head and common tail (`0x7550C..0x75609`)

Disassembled first-hand (`disassemble_bytes 0x7550C`, 260 B):

```
0x75514  SUB ESP,0x20
0x75517  [EBP-0xC] = rec
0x7551A  EAX = dword[rec+0x8F]>>24            ; stage (+0x92)
0x75528  if (stage < 6 && [rec+0x20]==0) CALL 0x7876C      ; slot merge
0x75547  if (stage >= 3) goto 0x755D4
0x75553  if (byte[rec+0x9B] != 0) goto 0x755D4             ; already has ball
0x7555C  EAX = dword[rec+0x8B]>>24                         ; sector (+0x8E)
0x75565  EBX = (int8)[EAX+0x10F331] << 4                   ; sprite offset x
0x75579  [0x15774C] = (int32)rec+0x59 + EBX                ; camera x
0x7557F  EDX = (int8)[EAX+0x10F339] << 4                   ; sprite offset z
0x75593  [0x157754] = (int32)rec+0x61 + EDX                ; camera z
0x7559B  [0x157750] = (int32)rec+0x5D + 0x38               ; camera y
0x755C0  byte[rec+0x9B] = 1                                ; TAKE (claim)
0x755C7  CALL 0x700F4(x,y,z,1)                             ; camera place
0x755CF  [0x157A83] = rec                                  ; controlled actor
0x755D4  word[rec+0x7B] = 3
0x755E8  EDI = dword[rec+0x89]; EAX = word[0x157A64]; [rec+0x89] = EDI+EAX
0x755F6  AL = byte[rec+0x92]; if (AL > 9) goto 0x760DF
0x75609  JMP dword CS:[EAX*4 + 0x754E4]                    ; 10-stage machine
```

So the `+0x89` action timer accrues `[0x157A64]` (frame delta) on **every**
stage, and the stage latch is `+0x92` (byte at `+0x8F`+3).

### 2.4 Row-1E stages (all quoted windows this slice)

| stage | entry | window | semantics (byte-exact) |
|---|---|---|---|
| 0 | 0x75611 | 0x75611..0x75659 | `[rec+0x44]==0` → 0x76127 (epilogue); else `0x157C36..3E := rec pos` (3 MOVSD `0x75628..`), `word[0x157C42]=0`, `[0x157AB2]=1`, `CALL 0x918CC`, `[rec+0x89]=0`, stage++ |
| 1 | 0x7565A | 0x7565A..0x7576F | `+0x89>2 → [rec+0x9E]=1`; `rec+0x4D := rec+0x59` (MOVSD); `0x15774C.. := rec pos`; camera z += ±0x20 by side; face `0x79C50(rec,-pos_x,-pos_z)`; event `0x6E598(rec,0x27,sector,0)`; `+0x89<0x3C → 0x760DF`; `0x157A77 := [0x10F328..10F330]`; slot≠0 → stage-3 body; else `RNG(0x92AC8)&1` → (face+event 0x32, timer 0, stage++) → stage 2, else stage-3 body |
| 2 | 0x75770 | 0x75770..0x75794 | `[rec+0x44]==0 → 0x760DF`; else timer 0, stage++ → stage 3 |
| 3 | 0x75795 | 0x75795..0x75B8F | §2.5 |
| 4 | 0x75B90 | 0x75B90..0x75D16 | §2.6 |
| 5 | 0x75D17 | 0x75D17..0x75DF2 | row = `byte[[rec+0x28]]`; if row==0x45 and `byte[rec+0x3D] >= 3`: camera=pos (+sector<<6, y=0x50), `0x92820(rec,5)`, `0x71C94(rec,&zero, word[0x157750])`, `[0x157A83]=rec`, **`byte[rec+0x9B]=0` (RELEASE)**; timer 0, stage++ |
| 6 | 0x75DF3 | 0x75DF3..0x75E81 | row 0x44/0x2F → `[EBP-4]=5, [0x157822]=1, [0x157820]=1`; row 0x45 → `[EBP-4]=3, [0x157822]=0, [0x157820]=0`; if `(int8)byte[rec+0x3D] < [EBP-4]` → 0x760DF; else timer 0, stage++ |
| 7 | 0x75E82 | 0x75E82..0x7609C | §2.7 |
| 8 | 0x7609D | 0x7609D..0x760C5 | `0x79B1C(rec)`; `[rec+0x44]==0 → 0x760DF`; else timer 0, stage++ |
| 9 | 0x760C6 | 0x760C6..0x760DE | `0x79B1C(rec)`; if `dword[rec+0x89] > 0x3C` `CALL 0x7DAB4(rec)`; fall to 0x760DF |
| — | 0x760DF | 0x760DF..0x76126 | common exit: if `[0x157AB2]!=0` `CALL 0x4C31C(rec+0x59)`; `0x8DCD4(rec+0x59,0x157C36,&local)`; `word[0x157C42] += local.distance`; `0x157C36.. := rec pos`; RET (epilogue 0x76127) |

**Stage 3 (`0x75795..0x75B8F`) — the claim/dive/hold body.** Sets
`byte[rec+0x92]=3`, `word[rec+0x7B]=2`, `[EBP-8]=0`; `if slot==0 CALL 0x744D4`;
then the slot-edge arm reads `word[slot+6]` (released edge):

```
0x757C4  bit 0x10 set -> [EBP-8]=1; EAX=0; CALL 0x36200; CALL 0x361A4; -> 0x758CD
0x757EF  bit 0x40 set -> CALL 0x36200; CALL 0x361A4; [EBP-8]=1;      -> 0x758CD
0x75817  bit 0x20 set -> [0x157AB2] toggled (SETZ):
            now 1 -> CALL 0x4C380; EAX=0; CALL 0x36200; CALL 0x361A4; -> 0x758C6
            now 0 -> CALL 0x79B1C(rec); 0x157A77 := [0x10F328..]; PASS args
                     to 0x4C320/0x4C380/0x361B0; XOR EAX; CALL 0x4C31C
0x758CD  if [0x157AB2]==0 -> EDX=1; EAX=[rec+0x20]; CALL 0x4C31C; -> 0x759EA
         else: rec+0x4D := rec+0x59
               if (word[0x157C42] < 0x90 && slot) rec+0x4D += (int8)(slot[+0x1D]>>24)<<4;
                                                     rec+0x55 += (int8)(slot[+0x1E]>>24)<<4
               if (byte[rec+0x9B] != 0) { 0x15774C := pos + slot_dir<<5; CALL 0x700F4 }  ; hold-follow
               else if (dword[rec+0x89] > 0x78) [EBP-8] = 1
               else if (word[0x157C42] < 0x90) { rec+0x4D := pos; rec+0x55 += ±0x10 by side }
0x759EA  if [EBP-8]!=0 -> 0x75B75 (timer 0; stage++ -> stage 4)
         else: 0x15774C := pos; [0x157750]=0x38
               if (word[rec+0x71]==0) [0x157754] += ±0x10 by side
               else 0x15774C/54 := pos + (int8)sector_tables[0x10F331/0x10F339]<<4
0x75A79  if (byte[rec+0x9B]!=0) CALL 0x74CDC                        ; guard clamp
0x75A87  if (word[0x157C42] <= 0xF0) -> 0x75B26
         else rec+0x4D := pos; if slot: 0x15774C/54 := pos + sector<<6
0x75B26  if (byte[rec+0x9B] != 0) -> 0x760DF                       ; still holding: exit, no restart
         else: CALL 0x7DAB4(rec)                                   ; RESET record
               0x75B3E EDX = byte[[rec]+0x826]                     ; team side
               0x75B46 EAX=0xB; 0x75B51 XOR EBX,EBX; 0x75B53 ECX=1
               0x75B58 CALL 0x8A938                                ; situation 0xB
               0x75B5D EDX=5; EAX=rec; EBX=0; CALL 0x7D9A4          ; install code 5
               0x75B6C..0x75B74 epilogue RET                       ; (no fall-through)
```

**Stage 4 (`0x75B90..0x75D16`) — ball-out staging.** `rec+0x4D := pos`;
slot==0 → `FUN_00074E2C(rec, 0x157C30)` (clear vector, §3.2 in FU-79);
slot≠0 and `[0x157AB2]!=0` → pin `word[slot+6]` to 0x10 when it isn't 0x20,
take `AX/CX = (int8)slot[+0x20/+0x21]` (falling back to the sector tables
`0x10F334/0x10F33C` when both zero) and `FUN_0007B878(rec, AX, CX, 0x157C30)`;
slot≠0 and `[0x157AB2]==0` → `0x8DCD4(rec+0x59, 0x157A77, 0x157C30)`. Then:

```
0x75C3B  d = word[0x157C30]; if (d < 0x5A0):
             local target = pos + (word[0x157C32], word[0x157C34])
             0x8DE8C(&local, team, skip=[rec+0x8A]>>24, 0)         ; nearest
             0x8DCD4(rec+0x59, nearest+0x59, 0x157C30)
0x75C9E  event = (d < 0x5A0) ? 0x44 : (d < 0x780) ? 0x2F : 0x45
0x75CC7  0x79C50(rec, word[0x157C32], word[0x157C34])               ; face vector
0x75CF7  0x6E598(rec, event, sector, 0); timer 0; stage++ -> stage 5
```

**Stage 7 (`0x75E82..0x7609C`) — the outlet.** Row 0x44: camera := pos
(y=0x10, sector offsets ×5<<4), `[0x158743]=0xB`, `0x92820(rec,4)`; then
`word[0x157C32] -= trunc(word[0x157C30]>>2)`, `word[0x157C34] -= trunc(...>>2)`
(0x8DC50 trunc-shift). **Erratum (P3 review):** the `>>2` operands above are
the wrong shorthand — first-hand `0x75F1F..0x75F4A` reads
`EAX=[0x157C30]>>16` (= `word[0x157C32]`) → `0x8DC50(AX, 2)` → `SUB
word[0x157C32], AX`, then the same for `word[0x157C34]`: each **component**
(dx and dz) is trunc-quartered itself, not the band. The port implements the
component form (and the `0x8DC50` truncation toward zero, not an arithmetic
shift).
Row 0x2F: camera := pos (y=0x80, sector<<6),
`[0x158743]=0xC`, `0x92820(rec,4,(word[0x157C30]>>5)+0x80)`. Else
`[0x158743]=1`, `0x92820(rec,5,word[0x157C30]>>2)`. All arms fall into:

```
0x75FF0  0x158738..3D := 0x157C30..35 (MOVSD+MOVSW, 6 bytes)
0x76012  0x79C50(rec, word[0x15873A], word[0x15873C])
0x7601E  EBX=(int16)CX; ECX=[0x158740]>>24 (=event 0x158743); PUSH 0; PUSH -1
         0x7A490(rec, 0x158738, event, ECX, stack)                 ; ball staging
0x76038  if slot: 0x8DE8C(EAX=0x157A77, team, EBX=0, ECX=0)         ; nearest teammate
                 FUN_000786A0(rec, nearest)                         ; slot hand-off
0x76058  EDX=side; EAX=0xB; EBX=0; CL=0; CALL 0x8A938               ; situation 0xB
0x76077  [0x157AB2]=0; CALL 0x4C380; timer 0; stage++ -> stage 8
```

### 2.5 Row-1D (`0x74EB0`) stage-3/4 (fresh; extends FU-79 §6)

FU-79 derived the 5-arm table and stages 0–2/4 block-level. Fresh
`disassemble_bytes 0x7529A` (608 B) gives the stage-3/4 tails byte-exact:

```
0x7529A  [0x158743]=1; 0x92820(rec, 6)
         if slot: 0x8DCD4(rec+0x59, 0x157A77, 0x158738)
         else:    FUN_00074E2C(rec, 0x158738)                     ; clear vector
0x752D5  band = word[0x158738] (via dword[0x158736]>>16); if (band < 0x5A0):
             local = pos + (word[0x15873A], word[0x15873C])
             0x8DE8C(&local, team, skip=[rec+0x8A]>>24, 0)
             0x8DCD4(rec+0x59, nearest+0x59, 0x158738)
0x75342  if (band < 0x3C0):
             angle = 0x8DD70(word[0x15873A], word[0x15873C])
             word[0x158738] = 0x3C0
             word[0x15873A] = 0x795A4(0x3C0, sine[0x114E04][angle])
             word[0x15873C] = 0x795A4(0x3C0, sine[0x114E04][angle+0x100])
0x753E2  ECX=2; EBX=(int16)(dword[0x158736]>>19) (= band>>3); PUSH 0; PUSH 0x30
         0x7A490(rec, 0x158738, EBX, ECX=2, event 0x30)           ; high trajectory
0x75405  (else arm) ECX=1; EBX=(int16)(band>>4); PUSH 0; PUSH 0x31
         0x7A490(rec, 0x158738, EBX, ECX=1, event 0x31)
         0x75421 EBX=4; EAX=0x22; EDX=rec; CALL 0x8F188            ; ring cmd 0x22
0x75433  if slot: 0x8DE8C(EAX=0x157A77, team, EBX=0, ECX=0); 0x786A0(rec, nearest)
0x75456  EAX=0xB; EDX=side; EBX=0; CALL 0x8A938                    ; situation 0xB
0x75473  [0x157AB2]=0; CALL 0x4C380; timer 0; stage++ -> stage 4
0x7549B  FUN_00079B1C(rec)
0x754A6  if (byte[rec+0x44]!=0) { [0x157AB2]=0; CALL 0x7DAB4(rec) }
0x754BC  if ([0x157AB2]!=0 && byte[rec+0x92] > 0) CALL 0x4C31C(rec+0x59)
```

Both rows therefore end a close-down with the **same restart tail shape**:
staging via `0x7A490`, optional teammate hand-off (`0x8DE8C`+`0x786A0`),
`FUN_0008A938(0xB, side, EBX=0)`, `[0x157AB2]=0`, `0x4C380`.

### 2.6 `record+0x9B` write census (fresh, MOV-scoped, 9 sites)

| site | function/context | value |
|---|---|---|
| `0x6FAA1` | `FUN_0006E8E8` event arm (`+0x9B=1` + camera place); event word-1 → CS table `0x6E800`, entry 49 = `0x6FA8F` = event `0x32` | 1 |
| `0x71D2D` | `FUN_00071C94` (ball staging entry) | 0 |
| `0x74567` | action 0x19 hold: `rec+0x4D`? no — camera := pos+sector<<4, y+0x38, `CALL 0x700F4`, `[0x157A83]=rec`, RET | 1 |
| `0x755C0` | row 1E head claim (stage<3, no ball) | 1 |
| `0x75DD1` | row 1E stage 5 (row anim 0x45): `[0x157A83]=rec` then release | 0 |
| `0x76DAE` | action 0x1B prologue (`[0x157A83]=rec`, `0x700F4`) | 1 |
| `0x7753C` | action 0x1B alternate arm (`CH=1`; also `[0x157C59]=1`) | 1 |
| `0x79A6C` | `FUN_0007997C` reset (`BH` was zeroed at `0x79A41 XOR BH,BH`) | 0 |
| `0x89903` | period/team reset pass (loops 11 records: `+0x9B=0`, install code 0x19, `0x7D9A4`) | 0 |

Semantics: **set-1 always pairs with a camera place `0x700F4` + actor bind
`[0x157A83]=rec`** (take); clear-0 in the keeper family comes only from the
stage-5 outlet, the staging entry `0x71C94`, and the two reset passes.

### 2.7 `FUN_000795B4` — fresh body and 12 callers

Fresh disasm (`0x795B4..0x795F0`): `ECX=EAX (from triple)`, `ESI=EBX (out)`,
`out.dx = (word)EDX[0] - (word)ECX[0]`, `out.dz = (word)EDX[8] - (word)ECX[8]`,
then `FUN_000CD514(out.dx, out.dz) -> word[ESI]`. So
**`FUN_000795B4(from, to, out{band,dx,dz})`**.

`FUN_000CD514(dx,dz)` (fresh `0xCD514..0xCD579`): `CALL 0xCD474(dx,dz)` (the
10-bit atan), mirror to `<=0x100`, then `(|dx|<<16)/sine(a)` for `a<=0x80`,
`(|dz|<<16)/sine(a-side)` for `a>0x80` — i.e. the folded-angle hypotenuse. The
engine already ports this inside `fifa96_entity_intercept_band`
(`include/fifa96_loader/fifa96_entity_update.h:59-81`, OL-41) — **FU-147 leg
10 is closed by the port + this first-hand body**.

Fresh 12 callers (each window read this slice):

| # | call site | caller | args (EAX from, EDX to, EBX out) | role |
|---|---|---|---|---|
| 1 | `0x72D87` | `FUN_00072AC4` | `0x15759F`, `0x1575AB`, `0x1575CC` | tracker target→camera band (FU-75 §3.2) |
| 2 | `0x88A6D` | `FUN_00088940` | `team+0x59` (record 0), `[0x157A9B]+0x59`, stack | goal-scan keeper→possession-record band |
| 3 | `0x799D5` | `FUN_0007997C` | `rec+0x59`, `0x15774C`, `rec+0x6B` | **reset-lane**: lane block vs camera focus |
| 4 | `0x8DA8F` | `FUN_0008D8EC` pre-pass | `[team+0x7BA]+0x59`, `+0x4D`, stack | local gate only (NOT written to `+0x6B`) |
| 5 | `0x77F90` | `FUN_00077EAC` | `rec+0x59`, `0x157C48`, stack | keeper CPU behind-goal band |
| 6 | `0x91EAD` | `FUN_00091E64` | stack triple, `0x15AC20+idx*0xC`, stack+0xC | aux table band (identity leg) |
| 7 | `0x7A149` | `FUN_0007A084` | `rec+0x59`, stack target, `0x158738` | reception → ball staging vector |
| 8 | `0x76C59` | `FUN_00076B28` | `rec+0x59`, `rec+0x4D`, `rec+0x65` | keeper steering band |
| 9 | `0x81A2B` | code 0x0B body | `rec+0x59`, nearest-opponent `+0x59`, stack | pressure check (`band > 0x20`) |
| 10 | `0x81D4F` | code 0x0C body | `rec+0x59`, stack lead target, stack+0xC | lead-target check (`band < 0x30`) |
| 11 | `0x846A5` | code 0x16 body | `rec+0x59`, `0x15880C` (ball record), stack | distance to ball |
| 12 | `0x7D6F8` | undefined body (~0x1C8 frame) | `ESI+0x59`, `[ESP+0x1C4]`, `[ESP+0x1A8]` | unclassified (leg) |

### 2.8 Team-level selection helpers

* `FUN_0008DE8C(EAX=pos short*, EDX=team, BX=skip index, ECX=&dist)` fresh
  (`0x8DE8C..0x8DEFF`): 11-record walk stride 0xB2; skips index==skip,
  `byte[rec+0x9A]`, `byte[rec+0x98]`; `d = 0x8DC68(pos.z-rec.z, pos.x-rec.x)`
  (unsigned `CMP AX,SI` keep-smallest); writes best distance to `*ECX`, returns
  best record. 37 callers — the shared AI "nearest teammate/opponent to point"
  used for support/receive/outlet targets. Engine: `fifa96_entity_find_nearest`
  (`fifa96_entity_update.h:13`).
* `FUN_0008DDE0(EAX=team, EDX=skip)` fresh (`0x8DDE0..0x8DE26`): same walk and
  skips, but ranks by **smallest signed word `+0x6B`** (lane). This is the
  team's ranked pick; `FUN_0008C33C` calls it with `EDX=-1` and stores the
  result in `[team+0x7B2]` (`0x8C37B`) and then `[team+0x7C7]` (`0x8C387`).
* `FUN_0008C33C(EAX=team)` fresh (`0x8C33C..0x8C38B`): 11-record loop calling
  `FUN_0007997C(rec, EDX=0)` (reset-lane), then `[team+0x7CB]=0`,
  `FUN_0008DDE0(team,-1)`, `[team+0x7B6]=0`, `[team+0x7B2]=pick`,
  `[team+0x7C7]=pick`. This is the **reset team-state pass** (FU-147 leg 6).
* `FUN_0008D8EC` pre-pass (`0x8D9BD..0x8DAF3`, fresh `0x8DA40` window): pick
  `0x8DE8C(0x10F37C+side*0xC, team, skip=[team+0x7B2]+0x8A>>24)` →
  `[team+0x7BA]`; reject when the pick holds a slot → `[team+0x7BA]=0`; else
  `FUN_0008D824([0x157A83], pick+0x4D)` (intercept target; engine port
  `fifa96_entity_intercept_bind`, `fifa96_entity_update.h:56`); `0x795B4(pick+0x59,
  pick+0x4D, stack)`; if `band < 0xF0 && word[pick+0x69] <= 0x1E0 &&
  |pick.z| > |cam_z|+0x90` → `[team+0x7BE]=1`; timer decay `+0x7CB` now.
  `read_memory 0x10F37C` = `{(0,0,0x990), (0,0,-0xA90)}` (side 0/1 target triples).
* `[team+0x7C7]` fresh 11-site census: writers `0x7472B` (action 0x19:
  `[team+0x7C7]=rec` when `word[rec+0x6B] < word[tracked+0x6B]` signed),
  `0x7C7CD` (BF20 tail: minimum-lane wins), `0x8C387` (reset pass); readers
  `0x74717` (0x19 head), `0x747D4` (0x19 camera guard requires
  `team[+0x7C7]==rec`), `0x7C7B6` (BF20), `0x7E89F`/`0x7EAD5` (row 04),
  `0x7F9AC` (`FUN_0007F7E0`), `0x82C46`/`0x830CE` (code 0x23 tackle window
  `[[rec]+0x7C7] != rec` and `[team+0x7C7]+0x69 >= 0x120`).

### 2.9 FU-145 L3 auxiliaries (first-hand bodies)

| symbol | window | derived |
|---|---|---|
| `FUN_00079C50` | `0x79C50..0x79C98` | `(rec, dx, dz)`; dx=dz=0 → returns existing `byte[rec+0x8E]`; else `word[rec+0x7D] = 0xCD474(dx,dz)` (10-bit facing) and `byte[rec+0x8E] = ((facing+0x40)&0x3FF)>>7` (0..7 sector), returns sector |
| `FUN_000741B4` | `0x741B4..0x741C3` | `(a ^ byte[0x157ABE]) & 1` (side-index xor) |
| `FUN_0006E598` | head `0x6E598..0x6E5F7` | gates `[rec+0x8D]!=0`, camera-z sign vs team side; event codes `{0x2C,0x2D,0x2E,0x35,0x37,0x39,0x3B,0x5C}` set `[0x157A6C]=1`; row resolution beyond the head is the animation selector (leg) |
| `FUN_000651F0` | `0x651F0..0x65214` | sound sink: `[0x155CE0] && [0x155D38] && [0x155D08]>0` → `[0x155D50]=arg`, JMP `0xA72E7` |
| `FUN_000974DC` | `0x974DC..0x974EC` | audio pair: `0x64EC0(arg)`, `0x65CC0(arg)` |
| `FUN_000974F0` | `0x974F0..0x974F4` | audio: `CWDE; arg>>4; JMP 0x63B20` |
| `FUN_00092040` | `0x92040..0x9206F` | announcer: `[0x15A990+idx*4]==1` → `word[0x15B42E+idx*2]>>16` → `0x65510` |
| `FUN_0008DC68` | `0x8DC68..0x8DCD2` | octagonal metric `max + ((min>>2 + min>>1)>>1)` (engine `fifa96_entity_distance`) |
| `FUN_000CD514` | `0xCD514..0xCD579` | folded-angle hypot (§2.7) |

### 2.10 Situation-0xB restart chain (FU-147 reading restored; errata 3 refuted)

Producer call shape is `FUN_0008A938(EAX=0xB, EDX=side, EBX=0)` (fresh
`0x75B46/0x75B58`, `0x75461/0x7546E`, `0x76063/0x76072`). Fresh head

```
0x8A938  ECX=id; [ESP]=side
0x8A94E  if (id==0xB) goto 0x8AA7B
0x8A957  if ([0x14C32A]==0) goto 0x8AA7B        ; session gate
0x8A964  if ([0x15B6C0]!=0) goto 0x8AA7B       ; pending
0x8A97F  ECX = id-2; [0x15B6B8] = (side==0)
0x8A991  JMP CS:[ECX*4 + 0x8A8E0]              ; table 1 (ids 2..0xA)
```

At `0x8AA7B`: `TEST BX,BX`; `BX!=0` → `FUN_000740A0(0, 0)` + `[0x15882C]=side`;
`BX==0` → CX dispatch (`0x8AB63`) → `JMP CS:[CX*4 + 0x8A904]` (13 entries
0..0xC; fresh read). **For id 0xB the `0x8A94E JZ 0x8AA7B` fires before the
`0x8A97F SUB ECX,2`, so `CX` is still `0xB` at the dispatch** (not `9`):
normal table[0xB] = `0x8AEF6` = `EAX=2; SAR EDX,0x10; CALL 0x740A0` →
**`FUN_000740A0(2, side)` = phase 2** (in play) — FU-147 §3.6 stands, and the
landed S1 `fifa96_action_phase_situations[0x0B] = phase 2` matches.

The `0x8AEC6` → `FUN_000888FC(2, …)` → `[0x1107EC + 2*4]` = `0x8922C` chain is
the **situation-9** row (the foul award), not the 0xB route: `0x8AEC6` is
normal-table entry 9. `FUN_000888FC` (fresh `0x888FC..0x8893B`) latches
`[0x158828]=id2`, `[0x158829]=side`, copies `[0x1107EC + id2*4]` to
`[0x158808]` and invokes it (fresh `read_memory 0x1107EC`: `{0, 0x88DC8,
0x8922C, 0x89FA4, 0x89620, 0x890EC, 0x89110, 0x89868, 0x8A798, 0x88F4C,
0x8B688, 0x8B874, 0x8B900}`); `0x8922C`'s head (`0x8922C..0x8926B`) ages
`[0x158818]` by delta and copies the ball-spawn triple `0x158897 → 0x158830`,
and it calls `FUN_000740A0(0xA, side^1)` at `0x89372` (fresh).
`FUN_000740A0` fresh (`0x740A0..0x7413x`): `[0x157A4E]=old phase`,
`[0x157A4D]=AL (phase arg)`, `[0x157AAF]=DL (side)`, runs `FUN_0008D098` over
both team blocks (`0x740C8`/`0x740DB`), and on phase==2 clears `[0x15781D]`,
sets `[0x157AB2]=1`, `[0x157A73]=0x15774C` (FU-147, confirmed).

## 3. Derived semantics

### 3.1 Keeper 0x1E state machine (row 1E / action code 0x1E)

The row-1E entry is simultaneously the action-table body for code 0x1E (FU-74
table `0x1106E0[0x1E] = 0x7550C`). Its 10 stages form three subsystems:

1. **Claim (head `0x75517..0x755CF`, stage<3 && `+0x9B==0`)**: slot-merge
   request when stage<6 & unbound; place the camera triple at the record
   position offset by the **facing-sector** sprite bytes
   (`0x10F331/0x10F339[sector]<<4`), y+0x38; set `+0x9B=1`; run camera place
   `0x700F4`; bind `[0x157A83]=rec`. Stages 0–2 are the approach/event lead-in
   (`+0x44` event acks, `+0x89` timers `0x3C`, RNG face/event 0x32).
2. **Hold/dive (stage 3)**: stage latch forced to 3, `+0x7B=2`; slot edges
   drive UI/sound calls and toggle `[0x157AB2]`; while `+0x9B!=0` the keeper
   follows its own position with the camera (slot-direction lead `<<5`, guard
   clamp `0x74CDC` when `word[0x157C42] > 0xF0`), and exits via `0x760DF` with
   **no restart** (still holding); the `0x157C42` gauge accumulates the
   distance travelled (measured at `0x760DF` against the saved `0x157C36`
   point) and gates the re-place arms (`<0x90`, `<=0xF0`). When `+0x9B==0`
   (or the hold timer `+0x89 > 0x78` fires), the no-ball exit **resets the
   record** and runs the restart tail (situation 0xB + install code 5).
3. **Outlet/throw (stages 4, 7) and release (stage 5)**: stage 4 builds the
   outlet vector (`0x7B878` slot-dir / `0x8DCD4` delta / `0x74E2C` clear),
   optionally retargets to the nearest teammate and fires event `0x44/0x2F/
   0x45` by band; stage 5 releases possession (`+0x9B=0`) on animation row
   0x45 and re-stages the ball through `0x71C94`; stage 7 places the
   throw origin and stages the ball via `0x7A490` (event `0xB/0xC/1`), then
   hands the control slot to the nearest teammate of `0x157A77`
   (`0x8DE8C`+`0x786A0`) and runs the situation-0xB tail (fresh
   `XOR CL,CL` before the call: invoke flag 0 — FU-147 leg 5's ECX doubt is
   now pinned to 0 for `0x76072`).
   Stages 6/8/9 are animation-row/frame gates and event acks
   (`[0x157820]/[0x157822]` set for rows 0x44/0x2F, cleared for 0x45).

### 3.2 Keeper 0x1D state machine (row 1D / action code 0x1D)

FU-79's 5 stages stand; this slice adds byte-exact stage 3/4 (§2.5). The
close-down is the keeper's **clearance**: slot (or clear-vector) → staging
`0x158738`, nearest-teammate retarget, a 0x3C0-magnitude rotated kick vector
(`0x114E04` sine + `0x795A4`) when the band is `<0x3C0`, `0x7A490` staging
(event `0x30` high / `0x31` low trajectory, ring cmd `0x22` on the low arm),
then the shared restart tail.

### 3.3 Restart chain after a claim (byte chain)

```
keeper 1D/1E no-ball exit
  -> 0x7DAB4(rec)                                   ; reset (code 0/stage 0xFF)
  -> FUN_0008A938(0xB, side, EBX=0)
       -> 0x8A94E JZ 0x8AA7B (CX still 0xB) -> 0x8AB63 -> 0x8AB7A
       -> normal table 0x8A904[0xB] = 0x8AEF6
       -> FUN_000740A0(2, side)                     ; phase := 2 (both teams
                                                    ; re-armed via 0x8D098)
  -> install code 5 onto the keeper (0x75B67) or return to stage 4 (0x1D)
```

The keeper thus joins the **shared** restart/situation queue; no keeper-only
restart mechanism exists.

### 3.4 AI target selection (non-controlled records)

* Every record's mover tail (BF20) computes lane block `{+0x6B,+0x6D,+0x6F}`
  = band/Δ vs `0x15774C` camera focus and updates `[team+0x7C7]` to the
  minimum-lane record (FU-147 §3.3; fresh readers/writers §2.8).
* Team picks: `FUN_0008DDE0` (min `+0x6B`) feeds `[team+0x7B2]` (the machine's
  "controlled/target" pointer, FU-74/75) at reset; `FUN_0008DE8C` (min metric)
  feeds support/receive/outlet targets inside actions 0x0B/0x0C/0x16/0x1D/
  0x1E/0x21; the driver pre-pass `FUN_0008D8EC` computes the `[team+0x7BA]`
  interception bind and `[team+0x7BE]` band flag from `0x10F37C`.
* `FUN_0007997C`/`FUN_0008C33C` is the reset-lane path: per-record reset
  (`0x79A6C` clears `+0x9B`; `0x79A19` seeds `+0x77 = +0x6B`; `0x799D5`
  refreshes the lane against the camera), then team pick + tracker.
* Marking/support runs are not a separate pass: they are the per-record
  targets chosen by the actions above plus `FUN_00072AC4`'s lane slots
  (`0x575F1`, FU-75 §3.3) — bounded here; see legs.

### 3.5 Errata (first-hand; changes dependent FU docs)

1. **"type" via `[rec+0x8E]>>24` is the action code at `+0x91`.**
   `0x784D3 MOV EAX,[EBP+0x8E]; SAR 0x18; MOV AL,[EAX+0x110680]` then
   `0x784F6 MOVSX AX, byte[EBP+0x91]` — the gate and the forced tail read the
   same byte the installer writes (`0x7DA63 [rec+0x91]=code`). `byte[rec+0x8E]`
   itself is the 0..7 **facing sector** written by `0x79C8E` and read by
   `0x79C5E`; it is the "type8" index into `0x10F331/0x10F334/0x10F339/
   0x10F33C` (`0x7555C..0x7557F`, `0x75A39..`). Fresh `0x110680` (48 B):
   `03 00 00 03 03 03 03 02 00... 02 02 02 02 ... 03 ... 02 .. 01`. All FU
   texts saying "type N"/"type-flag table" should read "action code N"/"code
   flag table"; the record machines' dispatch code selection
   (`[rec+0x8E]>>24==5` → carrier action 5) is consistent with this.
2. **FU-147 §3.5 driver lane refresh**: the `0x8DA8F` `FUN_000795B4` call
   writes a stack scratch used only for the pre-pass gates; the tracked
   record's `+0x6B` is refreshed by BF20's own tail, not by this call.
3. **FU-147 §3.6/§4.2 situation-0xB routing — errata refuted.** This draft's
   first reading (id 0xB → `CX=9` → `0x8AEC6`/`0x8922C`, "FU-147 misread the
   call-graph") was wrong: the `0x8A94E JZ` precedes the `0x8A97F SUB ECX,2`,
   so table[0xB] = `0x8AEF6` → `FUN_000740A0(2, side)` = phase 2 and FU-147
   stands as written; the `0x8AEC6`/`0x8922C`/`0x89372` chain is the
   situation-9 (foul) path. See §2.10.
4. **FU-79 §7 "0x1E body ends `0x755D3`"**: the listing cut; the body
   continues to `0x75609` (stage dispatch) and the machine to `0x7612F`.
5. **FU-79 open leg 1** (0x1D/0x1E installs): fresh censuses show no direct
   immediate install of either code in the match core (the single
   `MOV EDX,0x1D` at `0x7F0BB` is an argument to `0x92820`, overwritten at
   `0x7F0CD` before any `0x7D9A4`); installs are computed — leg.
6. **FU-78 take-site list** gains `0x6FAA1` (`FUN_0006E8E8` event 0x32) and
   the release `0x75DD1`; `0x89903` is the period-reset pass (installs 0x19).

## 4. Port contract

Existing seams (read first-hand):

* `fifa96_match_action_1E` (`src/fifa96_engine/fifa96_match_handlers.c:294`)
  currently binds only `fifa96_keeper_claim_place` (FU-140); row table stubs
  `0x1D..0x1F` NULL at `:1585-1588`; mover `match_run_controlled_mover`
  (`fifa96_match_run.c:233`) gated to `mr->slot.entity == id` (`:391`).
* Pool fields `lane_x/lane_z/lane` (`fifa96_match_entities.h:88-89`),
  `timer89/stage92/has_ball/has_slot`, team `target/second/chosen/intercept`
  (`:131-134`), `fifa96_match_entities_team_select` (`:216`),
  `fifa96_match_entities_install` (`:225`).
* Primitives: `fifa96_entity_find_nearest` (`fifa96_entity_update.h:13`),
  `fifa96_entity_intercept_bind` (`:56`), `fifa96_entity_intercept_band`
  (`:79`), `fifa96_entity_angle/sine` (`:32-33`).

Contract:

1. **Keeper 0x1E**: extend `fifa96_match_action_1E` into the ten-stage machine
   (`fifa96_keeper_claim_step` in `fifa96_keeper.c`), caller-owned state:
   `stage92`, `timer89`, `has_ball`, `has_slot`, `sector` (+0x8E),
   `anim_row` (`byte[[rec+0x28]]`), `frame` (+0x3D), the 6-byte vector
   `{band,dx,dz}` (`0x157C30`), saved point (`0x157C36`), gauge (`0x157C42`),
   `[0x157AB2]` latch, slot-edge words; effects as caller requests: camera
   place (`0x700F4`), ball staging (`0x7A490` + event), slot merge
   (`0x7876C`), slot hand-off (`0x8DE8C`+`0x786A0`), release
   (`fifa96_ball_pair_release`), restart (`fifa96_match_run_situation`).
2. **Keeper 0x1D**: new `fifa96_keeper_closedown_step` (stages 0..4, FU-79 §6
   + §2.5 this draft) wired as `fifa96_match_action_1D`; reuses the same
   staging/restart requests.
3. **`+0x9B`**: reuse `fifa96_ball_pair_possess/release`; wire the row-1E
   claim/release (`0x755C0`/`0x75DD1`), action-0x1B takes
   (`0x76DAE`/`0x7753C`) and the event-0x32 arm (`0x6FAA1`, leg).
4. **Restart**: single shared situation-0xB route (FU-146 §7.1 reconciliation
   stands); the engine maps 0xB to `FUN_000740A0(2, side)` = **phase 2**
   (normal table `0x8A904[0xB] = 0x8AEF6`; landed S1
   `fifa96_action_phase_situations[0x0B] = phase 2`), not a parallel `_0b`
   mechanism.
5. **Lane/AI fields**: add `bound (+0x77)`, `cam_dx6d/cam_dz6f
   (+0x6D/+0x6F)` to the pool (FU-147 contract); implement the track seam
   (`fifa96_action_locomotion_track` = `FUN_000795B4` over the pool) and run
   BF20 + track for **every** dispatched record (`fifa96_match_run.c:391`).
6. **Team selection**: wire `fifa96_match_entities_team_select` (0x8DE8C) at
   the action sites listed in §2.7; add
   `fifa96_match_entities_team_pick` (`FUN_0008DDE0` min-`+0x6B`) and
   `fifa96_match_entities_reset_lane` (`FUN_0008C33C` + `FUN_0007997C`) for the
   reset pass, writing `team->target` and new `team->tracker7c7`.
7. **Face/sector**: add `fifa96_entity_face(rec, dx, dz)` = `FUN_00079C50`
   (facing word + sector byte); the pool's "actor_type" commentary should be
   corrected to sector semantics (errata 1).
8. **Interception**: reuse `fifa96_entity_intercept_bind`/`band` in the
   driver pre-pass (`FUN_0008D8EC 0x8D9BD..0x8DAF3`) with the fresh `0x10F37C`
   constants.

## 5. Numbered legs

1. Install sites of codes 0x1D/0x1E (computed; no immediates) — not located.
2. `FUN_0007F7E0` no-slot dribble/target arm (calls `0x92820(rec,0x1D)`), not
   decomposed.
3. Stage-3 slot-edge/UI calls `0x36200/0x361A4/0x361B0/0x4C320/0x4C31C/
   0x4C380` semantics (input/sound/HUD) not derived.
4. `FUN_0006E8E8` event machine: only the event-0x32 arm (`0x6FA8F`, table
   `0x6E800` entry 49) derived; the other 57 arms not.
5. `FUN_0008D098` per-team phase arm (0x788-byte body) not decomposed beyond
   FU-147's install arm.
6. `0x8922C` (sit-9 act-2) handler beyond the head (phase args at `0x895AC`/
   `0x898DD`; the `0x89903` team pass) — S3 scope.
7. `0x1107EC` handler table identities for ids 1..12 not dumped.
8. The body containing `0x7D6F8` (~0x1C8-byte frame) is not a defined
   function; its `FUN_000795B4` call context is only partially read.
9. `FUN_00091E64`/`0x15AC20` aux triple table identity not derived.
10. `FUN_0006E598` row-resolution beyond the head gate not decomposed.
11. `FUN_00074584`/`FUN_000745EC` dive predicates: constants quoted
    (`0x3C0+2*(0x10-desc[+0xC])`, `0x930`), bodies not derived.
12. `FUN_0007B878` (row-1E stage-4 slot-vector builder) not decomposed.
13. Sector consumers of the `0x79C50` return not enumerated.
14. Phase `0xA` semantics (the situation-9 act-2 handler `0x8922C` target at
    `0x89372`) and its relation to the "phase 2" language in FU-147 — the 0xB
    restart no longer depends on this (it is table[0xB] = `0x8AEF6`, phase 2);
    the leg remains for the real act-2 phase-0xA path.
15. `[0x157820]/[0x157822]` writers/consumers outside row-1E stage 6.

## 6. Risks

* **Errata cascade.** "type 5/0x19" in FU-74/75/78/79/147 is the action code
  `+0x91`; FU-142/earlier ports already model it correctly (install writes
  `+0x91`), but any new code adding a separate "type" field would double-track
  state. Engine field `actor_type` (comment "type8") should be re-read as
  sector.
* **Tape movement.** Landing either keeper machine or the per-record mover
  changes observable motion; M2 re-pin with reason + frame diff (FU-147 risk
  stands).
* **Unsigned vs signed.** Stage-3 band comparisons are unsigned
  (`CMP/JNC/JA`), stage-6 frame compare is signed 16 (`MOVSX`); the
  `FUN_0008DDE0` pick is unsigned word `+0x6B` while action-0x19/BF20 tracker
  updates use signed (`JNL`/`JGE`). Preserve casts.
* **Restart sharing.** A keeper-specific restart would fork the situation
  queue (FU-146/145 pinned); route 0xB through the shared entry.
* **Record 0 runs unconditionally** in `FUN_0008D8EC`; any per-record AI work
  must keep the keeper dispatch even when unbound (`+0x9A` skip only for
  1..10).

## 7. Provenance

Ghidra MCP on `/FIFA96.EXE`, read-only. `get_current_program_info`/
`list_open_programs`; `get_function_by_address` 0x7550C (undefined), 0x74EB0
(undefined), 0x8DE8C, 0xCD514, 0x91E64, 0x8DDE0, 0x8D098, 0x79C50, 0x741B4,
0x651F0, 0x744D4, 0x6E8E8; `disassemble_bytes` 0x7550C (260 B), 0x74EB0
(200 B), 0x75611 (528 B), 0x75795 (688 B), 0x75A44 (336 B), 0x75B90 (400 B),
0x75D17 (224 B), 0x75DF3 (256 B), 0x75E82 (544 B), 0x7609D (192 B), 0x7529A
(608 B), 0x6FA60, 0x74530, 0x76D80, 0x77500, 0x799F0, 0x898E0, 0x8DA40,
0x795B4, 0x8D824, 0xCD514, 0x8DE8C, 0x7997C, 0x8C33C, 0x72D70, 0x88A50,
0x77F70, 0x7A130, 0x76C40, 0x91E90, 0x81A10, 0x81D30, 0x84690, 0x7D6E0,
0x784C8, 0x6E598, 0x651F0, 0x974F0, 0x92040, 0x974DC, 0x8DC68, 0x74584,
0x745EC, 0x7F0A0, 0x6E8E8, 0x8A938, 0x8AA7B, 0x8AB63, 0x8AEC6, 0x888FC,
0x8922C, 0x89350, 0x8959C, 0x898CC, 0x740A0; `read_memory` 0x754E4 (48 B),
0x74E9C (32 B), 0x6E800 (236 B), 0x8A904 (52 B), 0x1107EC (64 B), 0x110680
(48 B), 0x10F37C (24 B); `search_instructions` MOV `+ 0x9b],` (9), all `0x7c7]`
(11), MOV `EDX, 0x1D` (1), MOV `EDX, 0x1E` (10 raw, none in core);
`get_xrefs_to` 0x754E4 (1), 0x795B4 (12), 0x8DE8C (37), 0x8A938 (39), 0x740A0
(27). No write outside this draft; no tool/ISO/Ghidra-project change. Engine
files read only.

*Format note:* "band" is used for the `0x795B4` out word (engine
`fifa96_entity_intercept_band`); "sector" = byte `+0x8E`; "action code" =
byte `+0x91` (the docs' "type").

---

## 8. Port landing (P3, 2026-10-09)

Frozen slice ported in phase-7 P3 (M2 phase-7 ports Task 3). First-hand
re-verification this landing (Ghidra MCP `/FIFA96.EXE`, read-only):
`disassemble_bytes` 0x7550C (260 B), 0x75611 (530 B), 0x75817 (560 B),
0x75B90 (753 B), 0x75E82 (687 B), 0x74EB0 (400 B), 0x75045 (565 B), 0x7527F
(610 B), 0x79C50, 0x8DDE0, 0x8C33C, 0x799C0/0x7997C/0x79A5D, 0x7F374, plus
the fresh 0x761C8/0x761F2 and 0x7659E row-1F gate windows. Every control-flow
claim quoted in §2.3..§2.6 and the port contract §4 was reproduced before
coding; the machines are byte-exact at the decision level.

| contract item | engine landing |
|---|---|
| 1. keeper 0x1E | `fifa96_keeper_claim_step` (`fifa96_keeper.c`), the 10-stage fall-through machine over a caller-owned `fifa96_keeper_claim`; wired as `fifa96_match_action_1E` through the run keeper cells |
| 2. keeper 0x1D | `fifa96_keeper_closedown_step`, stages 0..4; new `fifa96_match_action_1D` and the action-table row 0x1D flips to `ported` |
| 3. `+0x9B` | claim/release through the machine's `has_ball` (drained by the pool as before); the action-0x1B takes and the event-0x32 arm stay legs |
| 4. restart | the machines emit `situation_0b`; the binder calls the single shared `fifa96_match_run_situation(0x0B)` (no parallel `_0b`) |
| 5. lane/AI fields | S1 landed `bound`/`cam_dz6f` (+0x6F) / `lane_z` (+0x6D camera dx); this landing names the tracker `team.tracker7c7` (was `camera_nearest`) and stages the 0x10F37C constants |
| 6. team selection | `fifa96_match_entities_team_pick` (`FUN_0008DDE0`) + `fifa96_match_entities_reset_lane` (`FUN_0008C33C` + derived `FUN_0007997C`) writing `target`/`tracker7c7`; `team_select` was already landed |
| 7. face/sector | `fifa96_entity_face` (= `FUN_00079C50`, both outputs); the pool `actor_type` comment now reads sector (erratum 1) |
| 8. interception | the `0x10F37C` constants staged in `match_run_entity_frame`; `fifa96_entity_intercept_bind`/`_band` already landed (S1/FU-139) |

**Errata application (erratum 1, `+0x8E`/`+0x91` audit).** Audited every
engine read of the two bytes. Correct as-is: the action-code paths all read
byte **+0x91** — the installer write (`fifa96_match_entities_install`), the
pool `entity.code` / staging `record.code`, the 0x110680 gates
(`outfield_type_gate(s->code)`), and the row-07/0F kick staging (`s->type =
r->code`, FU-139 §9.7). The **+0x8E** byte is modeled as the face octant
(`entity.type`, written by `fifa96_arm_face`/the keeper machines/row arms,
staged as `record.type`/`record.actor_type`; the two names are the OL-83
duplicate of one native byte). **Two wrong-field reads found and fixed**
(both in unwired bodies, so no tape effect): `fifa96_action_carrier_arm`'s
0x7F374 tail gate read `type8` (the +0x8E octant) where the native reads
`[rec+0x8E]>>24` = +0x91 — fixed with a new `carrier.code` field; and
`fifa96_keeper_input_decide` / `fifa96_keeper_arm_step`'s row-1F gates
read `type8` where `0x761D1`/`0x7659E`/`0x761F2` read +0x91 — fixed with
`fifa96_keeper_input.code` / the `code` parameter. The carrier fix is pinned
by a discriminating octant-vs-code test (`test_action_possession.c`: octant 5
with code 4 must not fire); the keeper-1F fix is pinned symptomatically by the
renamed-field tests (`test_keeper_bodies.c`, `in.code` staging), since that
body's gates are unwired.

**Legs status after P3.** Legs 1–15 remain open as numbered; newly explicit:
- leg 1 (install sites) unchanged: computed, no immediates;
- legs 3/4/12/13/15: the unported sinks are named request bits on the out
  structs (`ui`/`slot_fill`/`guard`/`snap`/`handoff`/`ball_stage`/`ring`/
  `sink_4b0`/scenario/`vector_build`), the `0x74E2C` clear vector now runs
  through the ported `fifa96_keeper_clear_vector` with two RNG draws, and the
  stage-6 `[0x157820]/[0x157822]` writes land on the run cells (consumers
  leg 15). Dropped call sites named as legs for the ledger: `0x918CC` (row-1E
  stage-0 ring reset), `0x73E08` + `0x4C324` (row-1D stage-0 placement tail),
  and `0x4C380` (row-1E stage-7 latch clear) — all present as `ui`/scenario
  request bits only;
- leg 6 (`FUN_0007997C`): derived reset landed; the virtual `[rec+0x1C]`
  target restore and the 0x79B6C commit stay the carried leg (the derived
  reset keeps the live position); the `0x795B4` lane band uses the
  `FUN_000CD514` folded-angle hypot (`fifa96_entity_intercept_band`, OL-41),
  not the `0x8DC68` octagon;
- leg 9 (`0x10F37C`): landed (constants staged);
- leg 13: the keeper focus cells are the engine's derived stand-ins (not the
  FU-71 camera), still a leg.

**Post-landing audit note (module 0x1E stage-2/3).** The stage-2 gate is the
`+0x44` byte; the engine's `row44` is default 0 (producer unported), so a
record reaching stage 0/2/8 outside a caller-staged fixture exits at the
stage gate exactly as an animation-inactive native record. Fixtures stage it.

No write outside docs/engine sources; no ISO/Ghidra-project change.
