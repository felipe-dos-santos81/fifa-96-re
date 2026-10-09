# FU-77: the locomotion family (codes 1–7), the shared per-record mover and per-code target arms

Follow-on to FU-76: derive the locomotion action family reached from the action
table flat `0x1106E0` — the directly installed codes `1..7`, the computed
codes `0/3/4/6/8/0x19` — and the **single shared movement integrator** every
record runs each frame (`FUN_0007BF20`, twin `FUN_0008E244`), then port the
evidenced movement math plus four clean per-code target arms as
`fifa96_action_locomotion_*`.

Result in one line: **no action handler moves a record; each handler writes the
output target triple `+0x4D/+0x51/+0x55` and then the per-record mover
`FUN_0007BF20` (called from both record machines, `0x7CD48` in `FUN_0007CA54`
and `0x785C5` in `FUN_000782D0`) turns it into motion: it computes the target
delta `+0x67/+0x69` and its metric `+0x65` (`FUN_0008DC68`), the desired facing
`+0x7F` (`FUN_0008DD70`→`FUN_000CD474`), clamps the facing turn by
`(0x10 - speed) << 2` (`<<2` more with a control slot) when `+0x43 != 0`, maps
the facing through the 32-byte heading table flat `0x1104D2` into `+0x8E`,
flushes a `+0x9C` stride accumulator against threshold `2` (or
`([rec+0x6F]>>17)+2`), ramps the velocity words `+0x73/+0x75` half-way toward
the flat `0x10F680` stride-velocity row (`(desired & 0x3F0) | stride`), updates
the speed metric `+0x71`, and integrates `+0x59 += delta*+0x73`,
`+0x61 += delta*+0x75`; of codes `1..7` the locomotion-classified bodies are
`3` (`0x7E1A4`, tracked placement), `4` (`0x7E7C8`, chase/pressure) and `6`
(`0x801B4`, pursuit/duel), while `1`/`2` are phase/restart sequences, `5` is the
carrier state and `7` the FU-76 kick; the port covers the integrator core and
the code-2 restart target, code-3 hold + phase-2 boundary/offside clamp and
code-4 camera-lead arm.**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58..FU-76).
  `disassemble_function`/`disassemble_bytes` are the citation source for every
  quoted instruction; `decompile_function` was used only for structure and is
  cited as such where the listing is cut. No capture-rig, tool, ISO or
  Ghidra-project change.
* **Address mapping (FU-76, restated).** Code/function addresses equal true
  link addresses; a data immediate Ghidra renders as `A` is flat `A+0x100000`
  (e.g. `0xF680` → `0x10F680`, `0x104D2` → `0x1104D2`, `0x14E04` →
  `0x114E04`); stored code pointers/CS jump tables resolve through `+0x10000`.
* **Tooling errata this slice.** (a) `disassemble_function 0x8E244` emits a
  phantom instruction at `0x8E300` (`ADD byte ptr [ESI+6],BH`); the raw bytes
  (`read_memory 0x8E2F0`) are `…81 fa 00 02 00 00 7e 06 81 e8…`, i.e. the
  `CMP EDX,0x200` ends at `0x8E300` and there is no such instruction. (b) The
  decompiler renders `LOCK INC dword [ESI+0x1C]` (`0x7E2DC`) as a call to
  `[rec+0x1C]`; the real indirect call to `[rec+0x1C]` is at `0x7E252`
  (`CALL dword ptr [ESI+0x1c]`), a different arm. (c) `FUN_0007dbc0` and
  `FUN_0007f194` have Ghidra-cut bodies (jump-table arms); those bodies are
  read from `disassemble_bytes` and the decompiler, respectively, and are
  marked.

## 1. The family and the shared mover

### 1.1 Callers

`FUN_0007BF20` (`0x7BF20..0x7C3FE` as Ghidra defines it; the body continues
past the cut to at least `0x7C741`, see §1.4) has exactly two callers
(`get_function_xrefs 0x7BF20`):

```
0x7CD48  in FUN_0007CA54   ; outfield records 1..10, after CALL [rec+0x18]
0x785C5  in FUN_000782D0   ; record 0
```

FU-75 §1 step 6 quotes the first as the per-frame tail
`CALL [rec+0x18]` → `FUN_0006E8E8(rec)` → `FUN_0007BF20(rec)`; the second is
the same tail for record 0 (FU-74 §2). So **every record runs the
locomotion integrator once per frame after its action handler**, whatever the
code. `FUN_0008E244` (`0x8E244..0x8E50C`, 233 instructions) is the identical
core used by the AI team-positioning passes instead of the action tail
(`get_function_xrefs 0x8E244` → `0x8E5B4` in the `0x8E5A4` wrapper,
`0x8E7F3` in `FUN_0008E748`, `0x8E8F2` in `FUN_0008E810`; those passes target
the opponent team's 11 records from signed-byte tables `0x109F9`/`0x10A67`
before calling it). `FUN_0008E244` lacks the `+0x85` lob block and the contact
block of `FUN_0007BF20`; the shared A–E blocks below are instruction-for-
instruction the same (compared at `0x8E24B..0x8E507` vs `0x7BF2B..0x7C398`).

### 1.2 The integrator, blocks A–E (`FUN_0008E244`, `FUN_0007BF20`)

Input `EAX = rec`. Block A, target delta and metric:

```
0x8E24B  AX = word[rec+0x4D]; DX = word[rec+0x59]; SUB EAX,EDX
0x8E255  word[rec+0x67] = AX                    ; out.x - pos.x
0x8E259  CX = word[rec+0x61]; AX = word[rec+0x55]; SUB EAX,ECX
0x8E263  word[rec+0x69] = AX                    ; out.z - pos.z
0x8E267  EDX = dword[rec+0x67] >> 16            ; = word[+0x69] = dz
0x8E26A  EAX = dword[rec+0x65] >> 16            ; = word[+0x67] = dx
0x8E273  CALL 0x8DC68                           ; fifa96_entity_distance
0x8E278  word[rec+0x65] = AX                    ; distance left
0x7BF60  dword[rec+0x24] = 0                    ; BF20 only (contact slot)
```

Block B, desired facing:

```
0x8E27C  if (word[rec+0x65] != 0) {
0x8E281     DX = word[rec+0x69]; AX = word[rec+0x67]
0x8E289     MOVSX EDX,DX ; CWDE ; CALL 0x8DD70  ; 0x8DD70: sign-extend, tail
                                                ;   into FUN_000CD474 atan
0x8E298  word[rec+0x7F] = AX                    ; desired facing
```

Block C, facing mode gated by `+0x43` (both functions, `0x8E29C..0x8E368` and
`0x7BF89..0x7C057`):

```
0x8E29C  if (byte[rec+0x43] == 0) goto block E
0x8E2A6  EAX = dword[rec+0x63] >> 16
0x8E2AC  if (EAX > 0x60) AX = word[rec+0x7F]         ; face the target
0x8E2B7  else { P = [0x57A73]; dx = P[0]-rec.x; dz = P[8]-rec.z;
                if (dx==0 && dz==0) AX = word[rec+0x7D]
                else AX = FUN_0008DD70(dx,dz) }      ; face the point
0x8E2EB  SUB AX,word[rec+0x7D]
0x8E2EF  if (AX == 0) goto block E
0x8E2F5  err = (AX & 0x3FF); if (err > 0x200) err -= 0x400
0x8E309  limit = (word)(0x10 - word[rec+0x71]) << 2
0x8E312  if ([rec+0x20] != 0) limit <<= 2            ; control slot
0x8E31F  clamp err to [-limit, +limit] (16-bit)
0x8E338  AX = (word[rec+0x7D] + err) & 0x3FF; if >0x200 -=0x400
0x8E350  word[rec+0x7D] = AX                         ; new facing
0x8E354  EAX = (AX & 0x3FF) >> 5
0x8E35C  byte[rec+0x8E] = byte[0x1104D2 + EAX]       ; heading table
```

Block E (`0x8E368..0x8E507`; BF20's `0x7C118..0x7C398` adds the `+0x93`
arm between the timer and the stride math, open leg 3):

```
0x8E368  if (word[rec+0x65] == 0 && word[rec+0x71] == 0) return
0x8E37A  byte[rec+0x9C] += (byte)[0x57A64]           ; body/stride timer
0x8E395  threshold = ([rec+0x20] != 0) ? 2 : (dword[rec+0x6F] >> 17) + 2
0x8E3AA  if (body_timer <= threshold) goto integrate
0x8E3BB  byte[rec+0x9C] = 0
0x8E3B3  DX = word[rec+0x7F]; CX = word[rec+0x7D]
0x8E3C2  diff = (DX - CX) & 0x3FF; if (diff > 0x200) diff = 0x400 - diff
0x8E3C4  AX = word[rec+0x7B]; AL &= 0xF
0x8E3EA  stride = (int16)AX - (diff >> 7); if (stride < 1) stride = 1
0x8E3F9  attr = dword[rec+0x63] >> 19; if (attr > stride) attr = stride
0x8E408  index = (word[rec+0x7F] & 0x3F0) | attr
0x8E422  tvx = word[0x10F680 + 2*index]
0x8E46F  tvz = word[0x10F680 + 2*index + 0x200]
0x8E430  if (tvx != vel_x) vel_x = |tvx-vel_x| <= 1 ? tvx : vel_x + (tvx-vel_x)/2
0x8E476  if (tvz != vel_z) vel_z = |tvz-vel_z| <= 1 ? tvz : vel_z + (tvz-vel_z)/2
0x8E4BC  if (either differed) word[rec+0x71] = CALL 0x8DC68(vel_x, vel_z)
0x8E4D1  if (word[rec+0x73] != 0) dword[rec+0x59] += (word)[0x57A64]*word[rec+0x73]
0x8E4EC  if (word[rec+0x75] != 0) dword[rec+0x61] += (word)[0x57A64]*word[rec+0x75]
```

The velocity ramp is `trunc(dv/2)` toward zero; the two `MOVSX`-guarded halves
(`0x8E44C..0x8E469` positive, `0x8E458..0x8E45F` negate/shift/negate) are
exactly C's `vel += dv/2`. The "changed" flag is the pair of pre-ramp
comparisons (`EDX=2`/`DEC EDX` at `0x8E42B..0x8E482`), not the post-ramp
values.

### 1.3 Initialisers used by the arm

`FUN_0008E244` is reached once per record by the team passes, AFTER they write
the output target from signed-byte tables (`FUN_0008E748 0x8E79A..0x8E7E8`,
`FUN_0008E810 0x8E86F..0x8E8E7`) and set `word[rec+0x7B] = 0xF`; the
`0x8E5A4` wrapper does the same single-record (`MOV word[EAX+0x7B],0xF` at
`0x8E5A7`) before `FUN_0008E244 0x8E5B4`.

### 1.4 The BF20-only blocks (not ported)

* `+0x85` lob block `0x7C057..0x7C118`: while `word[rec+0x85] != 0` it
  accumulates `word[rec+0x87] += delta`, sets `dword[rec+0x5D]` (Y) from the
  parabola table `[0x57744]` (`0x7C0D9`), or resets `+0x85/+0x87` and copies
  the position into the target when `+0x87 > 2*+0x85` (`0x7C094..0x7C0BA`).
* contact/collision block `0x7C3B1..0x7C741`: when the record has speed it
  scans the opponent base `[[rec]+0x7A6]` (11 records, `ECX=0..0x15` at
  `0x7C6F2`), gates per phase on the `+0x69` high-word band (`0x60` in phase 2
  else `0xC0`, `0x7C3B1..0x7C3C9`), and on a hit transfers momentum
  (`candidate+0x73/0x75 += rec+0x71>>18`, `0x7C55A..0x7C5CD`), halves the
  record's own `+0x73/+0x75` (`0x7C5A8..0x7C5B8`) and can nudge positions via
  the `0x114E04` angle table fold `FUN_000795A4` (`0x7C601..0x7C6CE`, RNG
  `FUN_00092AC8`); `[rec+0x24]` receives the contact record (`0x7C702`).
  Block-level only (open leg 4).

## 2. Per-code deep dives (codes 1–7)

The common shape is: handler writes `+0x4D/+0x51/+0x55`, sets timers/stage, may
install a computed code via `FUN_0007D9A4`, and returns; `FUN_0007BF20` then
moves the record. No handler integrates position.

### 2.1 Code 1 (`0x7DBC0`, 1..0x3 stage machine) — phase/score sequence, not locomotion

`0x7DBCB` requires phase `[0x57A4A]>>24 == 1`; stage is `[rec+0x8F]>>24`:

```
0x7DBDC  if (stage >= 2) { MOVSD rec+0x59..0x61 -> rec+0x4D..0x55; }  ; 0x7DC31
0x7DBEA  else CAMRST: PUSH 0; PUSH [0xF330]; PUSH [0xF32C]; PUSH [0xF328];
         CALL 0x700F4; [0x57A83] = rec;
         target.x = (dword[rec+0x4D] < 0) ? -0x30 : 0x30; target.z = 0;
         CALL 0x7876C
0x7DC3A  dword[rec+0x89] += delta; switch (byte[rec+0x92]) CS:[EAX*4+0x6DBB0]
         ; 4 arms 0x7DC6B/.., case0 emits event 0x1E (FUN_000974DC 0x7DC92),
         ; nearest search FUN_0008DE8C 0x7DCC7 with distance gates
0x7DDF5  score diff |[0x57AC5]-[0x57AC7]| in {3,6,9} posts command ids
         0x57/0x9B..0xA0 via FUN_0008F188 (0x7DEE9..0x7DEF5)
0x7DF00  nearest; stages the ball via FUN_0007A490 code 6 (0x7DF53);
         [team+0x7B2] = nearest (0x7DF5B); FUN_0008A938 code 0xB (0x7DF90);
         CALL 0x4C380; stage++
0x7DFB2  if (byte[rec+0x44] != 0) FUN_0007DAB4
```

It writes the target (own position from stage 2, a kickoff `±0x30` spot
before) and is the phase-1 ball sequence; no seek math of its own (open leg
for the stage arms `0x6DBB0`).

### 2.2 Code 2 (`0x7DFCC`) — restart/set-piece placement

```
0x7DFE6  if (phase == 1) {
0x7DFEE     EAX = [team+0x7B2]; EAX = word[EAX+0x59]      ; controlled x
0x7DFF7     dword[rec+0x55] = 0
0x7DFFE     dword[rec+0x89] = 0
0x7E008     byte[rec+0x92] = 0
0x7E011     NEG EAX; dword[rec+0x4D] = EAX                ; target.x = -x
0x7E01B  } else if (phase == 2) {
0x7E027     [team+0x7B2] = rec
0x7E02D     if (slot == 0 && byte[team+0x828] != 0) FUN_0007876C
0x7E046     if (slot != 0) { EDX=4; ECX=1; EBX=0; FUN_0007D9A4 } ; install 4 now
0x7E069     else target = camera 0x5774C/50/54; dword[rec+0x89] += delta
0x7E0xx     stage machine: nearest (0x7E12C), metric (0x7E13C),
            FUN_00092820(2,1) (0x7E14D), FUN_0007A490(6,0) (0x7E168);
            stage 2 tail FUN_00079B1C (0x7E187), FUN_0007DAB4 (0x7E194)
0x7E192  } else FUN_0007DAB4
```

The restart target is a 180° mirror of the controlled entity on the goal line
(`-x, 0`); phase 2 becomes a camera-placed wait that hands the ball to code 4
when the record owns a slot.

### 2.3 Code 3 (`0x7E1A4`) — tracked-entity placement (**locomotion**)

```
0x7E1C4  if (rec == [[rec]+0x7BA]) FUN_00079B58, return     ; interception target
0x7E1D6  if (phase [0x57A4D] in {0x13,0x14}) {
0x7E1E0     target.z = 0; target.y = target.z; target.x = 0x780
            FUN_0008DCD4(rec, &target, &metric) (0x7E1FC);
            if (metric word < 0x30) target.x = 0xAE0 (0x7E207); FUN_00079B58; return }
0x7E230  if (slot != 0 && [0x57A4D] != 0x0C) {
            if ([0x57A49]>>24 == 1 || action_of control in {6,0x15}) {
                EAX=rec; EDX=&rec+0x4D; EBX=-1; CALL [rec+0x1C]  ; 0x7E252
            } else {
                FUN_00079C20(rec, (int8)(slot[+0x1D]>>24), (int8)(slot[+0x1E]>>24))
                     ; target = pos + dir<<7, clamped
                word[rec+0x7B] = word[rec+0x79]; goto 0x7E3A6 }
0x7E281  if ([0x57A83] != 0) cVar = [[0x57A83]+0x91]
         else if ([0x57821] == 0 && [0x577CA] != 0) cVar = [[0x577CA]+0x91]
0x7E2B0  if (cVar in {0x10,0x11,0x12}) { MOVSD rec+0x59..0x61 -> rec+0x4D..; return }
0x7E2D3  LOCK INC dword[rec+0x1C]
0x7E2E0  EAX = dword[rec+0x63]; INC EAX; word[rec+0x7B] = AX
0x7E2EF  if (word[rec+0x7B] > word[rec+0x79]) word[rec+0x7B] = word[rec+0x79]
0x7E2F8  if (phase == 2) {
0x7E309     B = team + 0x80C + ([rec+0x8D]>>24)*4
0x7E322     if (word B[0] > target.z) { if (word B[0] < pos.z) target.z = word B[0]; }
0x7E336     else if (word B[1] < target.z && word B[1] > pos.z) target.z = word B[1]
0x7E348     if ([0x4C32A] == 0) {
0x7E360        side 0: if (target.z > [opponent_controlled+0x61])
                            target.z = [opponent_controlled+0x61] - 0x60
0x7E383        side 1: if (target.z < [opponent_controlled+0x61])
                            target.z = [opponent_controlled+0x61] + 0x60 }
0x7E3A6  FUN_00079F3C(rec)                     ; camera-relative output
0x7E3AD  if ([0x57A49]>>24 == 1) word[rec+0x7B] = 1
0x7E3D7  if (phase == 1) { side 0: target.z = -0x20 when > -0x20;
                           side 1: target.z = +0x20 when < +0x20 }
0x7E418  if (phase == 7 && side != [0x57AAC]>>24): ball-list scan 0x58747,
         event 0x25 (0x7E46F)
```

This is the placement machine: it chooses a target (slot direction, hold
position, camera place, or the second function pointer `[rec+0x1C]`) and
clamps it to the per-record bounds `team+0x80C[+0x8D]` and the opponent
offside line before the shared mover integrates. The `[rec+0x1C]` pointer and
`FUN_00079F3C`'s body are open legs 1/5.

### 2.4 Code 4 (`0x7E7C8`, 649 instructions) — chase/pressure (**locomotion**)

Head and forced codes (`CALL` census in the function):

```
0x7E7D3  byte[rec+0x9E] = 1
0x7E7E2  if (phase != 2) FUN_0007DAB4 (0x7E7E9), return
0x7E7F3  if (word[rec+0x81] != 0) return
0x7E80E  if (rec == [0x58777]) {                      ; carrier pointer
0x7E824     nearest FUN_0008DE8C(0x57770, team, skip) -> [team+0x7B2];
0x7E85C     if (type gate flat[0x110680+type]&1 == 0) return;
            FUN_0007D9A4(code 4, invoke 1); then 3/0x19 at 0x7E8xx }
0x7E8AD  if ([rec+0x8D] == 0) { ranked pick FUN_0008DDE0;
0x7E8CA     FUN_0007D9A4(code 0x19); nearest -> [team+0x7B2]/[+0x7B6] }
```

Target arms (decompile labels quoted as block names; the disassembly of the
`0x7EB95` arm is exact):

```
0x7EA18  no-slot arm: uVar8=1 when bVar2 = (rec == [0x577CA] && [0x57750] > 0x50)
0x7EB95  if (sVar3 != 0) {
            MOVSD camera 0x5774C..54 -> rec+0x4D..55
            EAX = word[0x577C0]; SHL 2; rec+0x4D += EAX      ; camera vx lead
            EAX = word[0x577C2]; SHL 2; rec+0x55 += EAX      ; camera vz lead
            if ([rec+0x69]>>16 < 0x3C0 && |camera.z| > 0x570) FUN_00079B58 }
0x7EBxx  vectors 0x57794 / 0x57788 (+ RNG ±0x10 via FUN_00092AC8),
         0x57770 (slot held bit 0x20 at 0x7EExx)
0x7EC0E  FUN_00079C20(rec, slot dir) for the no-slot/slot dir case
0x7EC16  FUN_0007D3E4 clamp (x ±0x720, z ±0xB10)
0x7EC89  FUN_0007E600 decision (install 0x0E invoke-now on its gates)
0x7EB5C  install 0xF; 0x7ED6A install 0xB; 0x7EF33 install 7;
0x7F120/0x7F133 install 6 / 5 (type-4/5 opponent duel, FU-76 §2)
```

So code 4 writes the camera-follow target (camera + camera-velocity lead), a
wing/target vector, or the slot-direction step, then clamps and lets the mover
accelerate toward it; it also runs the tackle/duel decision (`FUN_0007E600`,
`FUN_00092AC8` RNG gates).

### 2.5 Code 5 (`0x7F194`) — carrier/possession, not locomotion

Per the decompile (body Ghidra-cut at the stage jump table, 63 attributed
instructions):

```
entry    byte[rec+0x9E] = 1
         if (phase != 2) FUN_0007DAB4, return
         if (rec != [0x58724]) zero 0x58728..0x5872D and set [0x58724] = rec
         [team+0x7B2] = rec; [team+0x7B6] = 0
         if (dword[rec+0x89] < 0x4B0) dword[rec+0x89] += delta
         if (word[rec+0x81] == 0) { target = camera 0x5774C/50/54;
             switch (byte[rec+0x92]) 0/1/2 (jump-table body open leg 6) }
```

The carrier state keeps the record as its team's controlled entity and holds
it on the camera until the stage machine releases; no seek math (FU-76 §2
classification confirmed).

### 2.6 Code 6 (`0x801B4`, 597 instructions) — pursuit/duel with ball (**locomotion**)

```
0x801BF  byte[rec+0x9E] = 1
0x801D4  if (phase != 2) { FUN_0007DAB4 (0x801DB);
                            if (rec == team+0x7B2) clear it;
                            if (rec == team+0x7B6) clear it; return }
0x801E5  if ([rec+0x8D] == 0) { FUN_0007DAB4; clear team+0x7B2/0x7B6; return }
entry    if ([0x58724] == 0 || [0x58724+0x69]>>16 > 0x90 || [0x57750] > 0x70)
0x8025E     FUN_0007D9A4(code 4, invoke 1); return
0x802xx  target construction:
            FUN_0008DCD4/FUN_0008DD70 metric+atan (0x802BE, 0x802D6, 0x80333)
            camera base 0x5774C.. and own position (decompile local_68/local_60)
            angle -> vector folds CALL 0x795A4 with table 0x114E04
                 (0x80439, 0x80472, 0x80508, 0x80571, 0x805AC)
0x805F4  FUN_00079C20(rec, slot dir) when no slot
0x80646  FUN_00079B58 timer; 0x806F0/0x80791 RNG
0x807C1  FUN_0007D9A4(code 8 or 9 per gate) ; install 4 early
0x807C9  FUN_0007D3E4 clamp
0x80826  FUN_0006E598(type, {2,0x1C} per distance gate)
0x80850/0x808EB/0x8098C  nearest FUN_0008DE8C receiver/duel picks
0x809BE  FUN_00079CCC; 0x809E1 FUN_0006DA64
```

Code 6 is the ball pursuit: it builds a target from the camera/own position
rotated by the 10-bit angle→`0x114E04` vector fold toward the ball and turns it
into pursuit codes `8`/`9` when the RNG/attribute gates pass, else the shared
mover just consumes the written target.

### 2.7 Code 7 (`0x814B0`) — kick/pass (FU-76 §3.2)

Derived in FU-76: stage 0 output target from slot animation bytes
`table[0xF331/0xF339 + type8] << 4` or the camera triple (`0x8154C..0x815B5`);
stage 1 `FUN_0007B9C4` row application with the released word or `0x40`, and
the `0x22` invoke on a lined-up opponent (`0x816DF`); tail hands the ball actor
to code 4 (`0x81714`). Not re-derived here.

### 2.8 Computed locomotion codes and the bodies they share

| installed code | site | body (runtime) | family role |
|---|---|---|---|
| 0 | `FUN_0007DAB4` reset; keeper `0x78576` (FU-74 §3) | `0x7DB10` | FU-76 generic step (`fifa96_action_move_step`) |
| 3 | reset; `FUN_0007D9A4` 3←`+0x8D==0` coercion to 0x19; `FUN_0007C990` (FU-75 §1.5) | `0x7E1A4` | placement |
| 0x19 | reset; installer coercion; keeper reset; code 4 (0x7E8CA) | `0x746E4` | keeper hold (FU-76 §2) |
| 4 | reset; keeper `0x78576`; `FUN_0007C990`; code 7 tail `0x81714`; code 6 gate `0x8025E`; code 4 itself `0x7E85C` | `0x7E7C8` | chase |
| 6 | `FUN_0007C990` 0x7C9C5/0x7CA0C (FU-75 §1.5) | `0x801B4` | pursuit |
| 8 | chase gate `0x7CC93..0x7CD24` (FU-75 §1.6) | `0x81068` | computed chase |

Every computed code resolves to the same table slot as its direct twin
(`0x1106E0[code]`); **no two distinct codes share a handler body**. What all
codes share is the post-action mover `FUN_0007BF20`, and the handlers share the
target writers `FUN_00079C20` (codes 0/3/4/6) and `FUN_00079F3C` (code 3).

### 2.9 How FU-70/FU-75 feed the family

* FU-75 installs the locomotion codes from the record machine: the
  phase-2 forced decision `FUN_0007C990` computes `3`/`4`/`6` (carrier /
  controlled / other) and the chase gate installs `8`; the pressed/released
  row handlers install `7/8/9/0xB/0x21/0x23/0xE` (FU-75 §5). Each install
  calls `FUN_0007D9A4`, which sets `+0x91`/`+0x18` and invokes the handler on
  the spot when `ECX != 0`.
* FU-70's slot record `0x57C64` is read by the handlers through
  `rec+0x20`: direction bytes `slot+0x1D>>24`/`slot+0x1E>>24` (codes 0 and 3
  via `FUN_00079C20`), the released/held words for kick/tackle rows, and by
  the mover as the `has_slot` flag (`0x8E312`, `0x8E395`).

## 3. Port: `fifa96_action_locomotion_*`

Extends `include/fifa96_loader/fifa96_action_handlers.h` +
`src/fifa96_loader/fifa96_action_handlers.c` (caller-owned state, caller-
supplied tables, no globals, no comments, `-fifa96_err_t`; links
`fifa96_entity_update` for the metric).

| original | port |
|---|---|
| `FUN_0008E244`/`FUN_0007BF20` blocks A–E (`0x8E24B..0x8E507`, `0x7BF2B..0x7C398`) | `fifa96_action_locomotion_step(&state, heading_table, stride_table)`: delta/distance, desired facing via `fifa96_action_kick_angle`, `+0x43` facing mode, turn clamp `(0x10-speed)<<2<<slot`, heading table `0x1104D2` (32 bytes), `+0x9C` timer vs threshold `2`/`([+0x6F]>>17)+2`, stride `(+0x7B&0xF) - (mirror(diff)>>7)` min 1 min `+0x63`>>19, `0x10F680` row ramp (half-step, ±1 snap), speed metric, `delta`-scaled position integration |
| code 2 phase-1 arm `0x7DFEB..0x7E016` | `fifa96_action_locomotion_restart_target(controlled_x, &tx, &tz)`: `tx = -controlled_x`, `tz = 0` |
| code 3 hold arm `0x7E2C0..0x7E2C8` | `fifa96_action_locomotion_hold(action_code, pos, out, &held)`: copies the position when the control action is `0x10/0x11/0x12` |
| code 3 phase-2 clamp `0x7E309..0x7E399` | `fifa96_action_locomotion_clamp_placement(&target_z, pos_z, bound_lo, bound_hi, opponent_z, side, settings_latch)`: bound clamps then side-scaled opponent line ±0x60, skipped by `[0x4C32A]` |
| code 4 camera-lead arm `0x7EB9A..0x7EBC5` | `fifa96_action_locomotion_camera_lead(cam_x, cam_y, cam_z, cam_vel_x, cam_vel_z, &out)`: camera triple + `word[0x577C0]/0x577C2]<<2` |
| BF20 `+0x85` lob (`0x7C057..0x7C118`), `+0x93` stride arm (`0x7C16A..0x7C289`), contact block (`0x7C3B1..0x7C741`), `FUN_00079F3C`, `FUN_0008E008`, `0x114E04` folds, RNG/event arms, handlers | not ported (globals/RNG/cut bodies; open legs 1–9) |

Struct `fifa96_action_locomotion` mirrors the proven record fields (`_Static_
assert`ed in the test): position `+0x59/+0x61`, target `+0x4D/+0x55`, delta
`+0x67/+0x69`, distance `+0x65`, facing `+0x7D`, desired `+0x7F`, speed
`+0x71`, velocity `+0x73/+0x75`, point `[0x57A73]+0/+8`, attr `+0x63`, stride
rate `+0x6F`, heading `+0x8E`, slot `+0x20`, `+0x43`, timer `+0x9C`, stride
`+0x7B`, frame delta `[0x57A64]`.

## 4. Tables

* **Heading, flat `0x1104D2`, 32 bytes** (`read_memory`):
  `00 00 00 01 01 01 01 02 02 02 02 03 03 03 03 04 04 04 04 05 05 05 05 06 06
  06 06 07 07 07 07 00`. Index `(facing & 0x3FF) >> 5`.
* **Stride velocity, flat `0x10F680`** (immediate `0xF680` at `0x8E419` /
  `0x7C2E0` through base `0x100000`): `word[+2*index]` = target vx,
  `word[+2*index+0x200]` = target vz; `read_memory 0x10F680` (64 B) is
  zero until `+0x2C` then the `00 01 00 01 …` row pattern, i.e. data (full
  range not dumped, open leg 8).
* **`0x114E04`** angle→dword-vector table used by codes 4/6 (`CALL
  FUN_000795A4` at `0x7C601`/`0x80439` etc.) — cited, not ported (FU-76
  open leg 5).

## 5. Tests (`tests/test_action_locomotion.c`, suite 65 → 66)

* Layout `_Static_assert`s on all port-state offsets.
* `step` target delta/distance, stationary desired-facing, NULL arguments.
* Turn clamp: no-slot `(0x10-0)<<2 = 0x40`, slot `<<4 = 0x100`, `speed == 0x10`
  limit 0 and `speed == 0x20` negative-limit wrap (`-0x40`), heading mapping
  through a caller table.
* Angle wrap `0x300 → -0x100` and the mirrored stride error.
* Direct-face modes: `+0x43` with `+0x63` high `0x70` (target) vs `0x50`
  (ball point), zero point, and `+0x43 == 0` leaving facing/heading untouched.
* Stride ramp: half-step acceleration 0→0x10→0x18, ±1 exact snap, negative
  `dv/2` truncation, attr clamp, min-1 stride.
* Speed metric gate (identical target keeps the old `+0x71`).
* Body timer boundary `timer == threshold` (no flush) vs `+1` (flush), slot vs
  `([+0x6F]>>17)+2` threshold, and the distance-0/speed-0 gate.
* Position integration with `vel*delta`, delta 0/negative and both axes.
* Per-code: restart mirror, hold for `0x10/0x11/0x12`, clamp_placement bound
  and offside branches (`-0x60`/`+0x60`), camera lead.
* **Mutation check**: flipping the body-timer guard `>` to `>=` makes
  `test_step_body_timer_boundary` fail (`timer == threshold` then flushes);
  reverted after the check.
* ASan+UBSan: `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
  tests/test_action_locomotion.c src/fifa96_loader/fifa96_action_handlers.c
  src/fifa96_loader/fifa96_entity_update.c` runs clean; same for
  `test_action_handlers.c`. `make test`: 65/65 before, **66/66 after**.

## 6. Errata (quoted)

* FU-76 §2 code 3 "`CAMPLACE 0x79F3C` (camera-relative output), `TMR93`,
  `EVENT`" — **extended**: the slot arm `0x7E230`, the hold copy
  `0x7E2C0..0x7E2C8`, the `[rec+0x1C]` indirect call `0x7E252`, the
  `LOCK INC [rec+0x1C]`/`+0x7B = +0x63+1` placement counter
  `0x7E2D3..0x7E2F4` and the phase-2 bound/offside clamps
  `0x7E309..0x7E399` are derived; `[rec+0x1C]`'s body stays open.
* FU-76 §2 code 4 "installs `4/0x19/0xF/0xB/7/6/5`" — **confirmed** by the
  `CALL` census (`0x7E85C`, `0x7E8CA`, `0x7EB5C`, `0x7ED6A`, `0x7EF33`,
  `0x7F120`, `0x7F133`) and extended with the target arms (`0x7EB95` camera
  lead exact; `0x57794`/`0x57788`/`0x57770` vectors).
* FU-67 §3.2 "`+0x7F` dword high word is a timer limit" — **clarified**: the
  dword at `+0x7F` has low word `+0x7F` = desired facing (integrator
  `0x8E298`) and high word `+0x81` = the `FUN_0007CA54` countdown; the
  `>>16` read is the same field FU-74/FU-75 call the timer limit.
* FU-73 §7 "no separate per-frame integrator" applies to the ball; for records
  the per-frame integrator is `FUN_0007BF20`/`FUN_0008E244` (this slice).
* `disassemble_function 0x8E244` emits a phantom `0x8E300` (Method b); the raw
  bytes decode the `CMP EDX,0x200` tail. The decompiler also renders the
  `LOCK INC dword [ESI+0x1C]` at `0x7E2DC` as a call (Method b).

## 7. Open legs

1. **`[rec+0x1C]`** second function pointer: called with `EAX=rec`,
   `EDX=&rec+0x4D`, `EBX=-1` at `0x7E252`; body/identity not derived.
2. **BF20 `+0x85` lob block** (`0x7C057..0x7C118`, parabola table `0x57744`)
   and the writes to `dword[rec+0x5D]`.
3. **BF20 `+0x93` stride arm** (`0x7C16A..0x7C289`): animation timers
   `+0x94/+0x96`, the `[0x4C2F6]`-gated write to `0x57BA6 + side*0x2A +
   type*2`, and `EBX += 3 - (+0x96>>7)` are cited, not ported.
4. **BF20 contact/collision block** (`0x7C3B1..0x7C741`): RNG, momentum
   transfer, `0x114E04` position kicks, `[rec+0x24]`.
5. **`FUN_00079F3C`** (camera-relative output) and **`FUN_0008E008`**
   (per-record animation state machine on `rec+0x28`) bodies.
6. **Code 5 stage machine** (`+0x92` cases 0/1/2) — jump table truncates the
   decompiler; stages/effects not decomposed.
7. **Code 4/6 event and RNG arms**: `FUN_0007E600`, `FUN_00092AC8` gates,
   `FUN_0006E598` code sets, `FUN_00079CCC`/`FUN_0006DA64` calls are cited by
   address only.
8. **`0x10F680` full table** and the stride-index semantics
   (`(desired & 0x3F0) | stride`) — data read at the head only.
9. **Heading table `0x1104D2`** mapping meaning; and **who sets `+0x43`**
   (the direct-face flag) outside code 6 (`*(byte*)(rec+0x9d)` in the
   decompile is the only other observed `+0x9x` animation byte).
10. **Code 1 stage arms** `CS:0x6DBB0` case bodies beyond the event-0x1E
    citation.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x8E244, 0x7BF20, 0x8E008, 0x8DD70, 0x7DFCC, 0x7E1A4,
0x7F194, 0x7E7C8 (decompile), 0x801B4 (decompile), 0x7DBC0 (decompile
timeout, bytes); `disassemble_bytes` 0x7C3E0..0x7C760 (BF20 tail),
0x7DBC0..0x7DFC8 (code 1), 0x7DFCC..0x7E060 (code 2 head), 0x7E230..0x7E2A4
and 0x7E2A0..0x7E470 (code 3), 0x7EB90..0x7EBC8 (code 4 camera lead),
0x8E500..0x8E5C0 (E244 tail + wrapper), 0x8E2F0 (phantom check);
`read_memory` 0x1104D2 (32 B), 0x10F680 (64 B);
`search_instructions` `CALL` in FUN_0007dbc0/dfcc/e1a4/e7c8/f194/801b4,
operand `+ 0x7d]`; `get_function_xrefs` 0x7BF20, 0x8E244, 0x8E008, 0x79C50,
0x79F3C; `get_function_by_address` 0x8E244, 0x8E008, 0x7BF20, 0x8D824,
0x7C400, 0x7C741.

Analysis-only outside the port: no tool, capture-rig, ISO or Ghidra-project
change. Port write set: `include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c`,
`tests/test_action_locomotion.c`, `CMakeLists.txt` (one test block + the
`fifa96_entity_update` link). `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.

## Errata (M2 interactive Task 1 — pad-driven locomotion / G1)

First-hand `/FIFA96.EXE`, read-only. This slice re-read the row-00 slot arm,
the setup bind and the mover tables, and the engine now binds the FU-70 human
slot to the kickoff taker and runs the shared mover for that record.

* **Row 00's `FUN_00079C20` args are slot bytes `+0x20`/`+0x21`, not
  `+0x1D`/`+0x1E`.** The body reads `MOV EDX,[ECX+0x1D]` / `MOV EBX,[ECX+0x1E]`
  with `ECX = [rec+0x20]` (`0x7DB51..0x7DB56`) and shifts both `SAR ...,0x18`
  (`0x7DB59..0x7DB5C`): the dword at `+0x1D` has `+0x20` as its top byte and
  the dword at `+0x1E` has `+0x21`, so the arguments are the T2/T3 direction
  bytes `FUN_00078950` writes (`0x78A39`/`0x78A46`). FU-70 §1.2's `+0x1D`
  (bind ordinal) / `+0x1E` (map select) labels are not the row's inputs; the
  `>>24` folds them onto `+0x20`/`+0x21`. FU-139 §10.8's `(int8)slot+0x20` /
  `+0x21` reading is the correct one.
* **The mover's `move_attr` high word is the block-A distance `+0x65`.** The
  attr extract is `dword[rec+0x63] >> 19` (`0x8E3F9`): the low word `+0x63`
  is shifted out (`word >> 19 == 0`), so the result is `word[rec+0x65] >> 3` —
  the distance block A recomputed at `0x8E267..0x8E278` immediately before the
  table read. The engine composes `move_attr = (uint16_t)distance << 16` from
  the post-action target, so the stride index sees the fresh distance.
* **Row 01 stage 1's slot arm is `word[slot+6] & 0x70` over the live FU-70
  release word.** First-hand `0x7DCE2..0x7DCF8`: with `[rec+0x20] != 0` the
  body tests `AX = word[slot+6]; AL &= 0x70` and readies on non-zero; the
  no-slot arm is the `+0x89 > 0x78` timer. The engine now passes the FU-70
  release word (`fifa96_control_slot.released`) and the natural kickoff waits
  for a button press/release exactly as the native. The pre-T1 "staged-zero
  slot cannot fire" stand-in (FU-143 §11.1/OL-84f) is superseded by this
  erratum.
* **Tables pinned first-hand.** `read_memory 0x1104D2` (32 B) = the heading
  row; `read_memory 0x10F680` (0xA00 B) = the full 0x500-word stride table
  (the FU-77 §4 head read was 64 B). Both are embedded in the engine
  (`src/fifa96_engine/fifa96_match_locomotion_tables.c`); the engine re-read
  is byte-identical for the 64-byte head.
* **`FUN_00078824`/`FUN_000785E0` bind semantics re-read** (`0x78824..0x7891C`,
  `0x785E0..0x7866D`): the setup clears every record `+0x20` and both teams'
  `+0x828`, then binds up to four `0x57C64` slots from the `0x4C1E0` mode rows
  (mode 0 -> team `[0x57ABE]`, mode 2 -> `[0x57ABF]`, else unbound) with the
  `0x4C1DC` map select; `FUN_000785E0` picks the free record with
  `FUN_0008DB6C(0x5774C, team, -1, 1)`, writes the record slot pointer and
  increments `team+0x828`. The engine models the one-slot subset: the four
  mode rows are unported (derived default mode 0 = the controlled side), the
  pick substitutes the shared nearest search for `FUN_0008DB6C`'s
  `FUN_000A1860` sort/tie order (leg), and the result seeds the state-1 arm's
  `FUN_0007876C` merge.
* **Engine landing (M2 interactive Task 1).**
  `fifa96_match_entities_bind_slot` (entities pool) and `match_run_slot_bind`
  (begin, after the formation seed and before the phase-1 entry) implement the
  bind; begin then calls the FU-141 drain so the arm's merge result points the
  FU-70 slot at the taker. `match_run_controlled_mover` (run.c) stages the
  mover state from the pool record (`pos`/`target`/`delta`, `face7d` +0x7D,
  `speed71` +0x71, `vel73`/`vel75` +0x73/+0x75, `body_timer9c` +0x9C,
  `timer7b`+0x7B stride, `has_slot`, `type` as `+0x8E`) and calls
  `fifa96_action_locomotion_step` with the embedded tables. Unported inputs
  staged zero: `+0x6F` stride rate, `+0x43` direct-face and the `0x57A73`
  point. AI-side integration (the native calls the mover for every record) is
  a numbered leg.
* **Tests.** `tests/test_engine_match_frame.c::test_pad_drives_controlled_locomotion`
  (begun run: bind + merge, staged reset code 0, held UP, row-00 target →
  velocity/position; a no-input control run stays still);
  `tests/test_engine_match_entities.c::test_bind_slot`;
  `tests/test_engine_match_handlers.c::test_action_01_runs_kickoff_body`'s slot
  sub-case; `tests/test_engine_match_input.c` begun-run bind assertions. Full
  suite 104/104 (ASan/UBSan; ISO present); M1 golden untouched; M2 re-pinned
  (first differing line frame 59, 107 hash lines, `state=` suffixes unchanged)
  with the written reason in `tests/test_engine_m2.c`'s v4.1 provenance.

### Fix round (T1 review): velocity word/dword alias sync

The FU-77 mover writes the native **words** `+0x71` (speed), `+0x73` and
`+0x75` (velocity); the engine's pool also carried the pre-T1 dword views
`vel_x` (+0x71) / `vel_z` (+0x73), and the generic write-back copied both, so
the dwords stopped mirroring the bytes they overlap after a mover step. Live
consumers read the words through the dwords: ball pairing
`(int16_t)vel_x` = word +0x71 and `(int16_t)vel_z` = word +0x73
(`fifa96_match_entities.c` `_ball_pair`), row 04 `r->vel_x >> 16` = word +0x73
and `r->vel_z >> 16` = word +0x75 (`fifa96_match_handlers.c`
`fifa96_match_action_04`), the row-2A carrier speed `(int16_t)vel_x` = word
+0x71. Fixes:

* `match_run_controlled_mover` recomposes both dwords from the three words on
  write-back: `vel_x = speed71 | vel73<<16`, `vel_z = vel73 | vel75<<16`.
* After the action dispatch (before the mover) the dwords are decomposed back
  onto the words (`speed71 = (int16_t)vel_x`, `vel73 = vel_x>>16`,
  `vel75 = vel_z>>16`): the arm handlers (rows 26/28/2A) write only the dword
  views and only ever zero them, so an arm zero now clears the words too and a
  pass-through is identity under the staging invariant.
* `fifa96_match_entities_place` zeroes the three word fields (native
  `0x79B8A..0x79BB1` clears words, not dwords) plus the recomposed dwords, so
  no stale word survives a `FUN_00079B6C` commit.
* Tests: `test_pad_drives_controlled_locomotion` asserts the dual views stay in
  lockstep after movement; `test_place_commits_target` stages the words and
  both dwords and asserts all five are zero after the commit.

M2 golden unmoved by this fix (transcript byte-identical, `cmp` clean): the
composed dwords feed ball pairing and the row-04/2A word readers, whose
outputs are unported sinks or gate-neutral on the tape's forced window. M1
byte-identical.

### Fix round (T1 review): the second activation — row 01 stage-2 merge

The stage-2 conditional merge (`0x7DF61..0x7DF75`) was previously unreachable
because the pool staged `+0x828` (slot-pool ordinal) at zero. The setup bind
now increments it, so the arm fires when the nearest record holds no slot; the
recorded `FUN_0007876C` request is consumed by the frame drain
(`match_run_entity_drain`) and rebinds the FU-70 slot onto the nearest. Both
row-01 comments in `fifa96_match_handlers.c` are refreshed accordingly.

### Bind leg 7 (T1 review, carried)

`FUN_0008DB6C`'s no-candidate fallback with `ECX != 0` (`0x8DC1B..0x8DC41`):
when the distance scan finds no eligible record, the native falls back to the
first record with `[rec+0x20] == 0 && [rec+0x9A] == 0` (ignoring `+0x98`). The
derived `fifa96_match_entities_bind_slot` returns NONE instead (soft no-bind);
recorded as leg 7 of the bind.
