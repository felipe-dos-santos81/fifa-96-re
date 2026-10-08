# FU-143: the phase drivers — writers, the 35-row table transitions and the period-end sequence

Task 10 of the M2 child plan (`docs/superpowers/plans/2026-10-07-fifa96-m2-match.md`,
G3), the follow-on to FU-83 (the phase table) and FU-62 (the clock). Derives the
writers and semantics of the phase byte `[0x157A4A]>>24` = `[0x157A4D]`, the
transition edges of the 35-row phase table (the `FUN_000740A0` setter census,
the `FUN_0008A938` situation dispatcher and the `FUN_0008B9CC` period-change
chooser), and answers the G1 carry-forward: **which phase ends periods under
the selector-0 default**.

Result in one line: **the phase byte has exactly two writers — the reset
`FUN_00073E28` (phase 0) and the setter `FUN_000740A0` (`[0x157A4E]` keeps the
previous phase, `[0x157AAF]` takes the side, phase 2 additionally clears
`[0x15781D]`, sets `[0x157AB2]` and points `[0x157A73]` at `0x15774C`); the 35
handlers at flat `0x110794` are driven by: (a) the per-record installer
`FUN_0006D920` (FU-83), (b) the `FUN_000888FC` act selector whose table base
`0x1107EC` is `phase_table[0x16]` (act `a` calls phase `0x16+a` directly), (c)
the period-end chooser `FUN_0008B9CC` (extra-time flag `[0x157AC0]==0` ->
phase `0x0C`; set -> `0x13`/`0x14` picked by `[0x157ABE]`/`[0x157ABF]`/
`[0x157AAC]>>24`, upgraded to `0x14` by the unported `FUN_00012230`/
`FUN_00012250` probes), and (d) the `FUN_0008A938` situation table (inline CS
table `0x8A904`: situations 0..0xC -> phase 0x11/3/4/8/9/5/0xD/2 or act
1/2/7/9/0xA); periods end only on a class-1 phase — `0x02` is the only class-1
phase in the live range `0x00..0x16`, and the class-2 aux counter advances in
lockstep with the seconds counter, so no class-2 phase (including the phase-0
reset default) can ever satisfy the period-end equality; after completion the
phase goes to `0x0C` (selector-0/no-extra-time default) and the phase-0xC
machine (`FUN_0008BAF0`) either sets match-over `[0x58822]=1` or hands to
phase `0x13`, whose own machine sets `[0x58822]=2`.**

## Method

* Authoritative program **`/FIFA96.EXE`** (explicit `program` argument in every
  call; `/fifa96.exe` and `/fifa96_le.bin` untouched). Ghidra **read-only**: no
  renames, comments, labels, function creation, scripts or project saves.
* Tool calls made this slice (all on `/FIFA96.EXE`): `search_instructions`
  operands `157a4a` (126 matches), `157a4d` (5), `157a4c` (6), `157a49` (26),
  `15b6a8` (25), `15b6c0` (32), `00157a4a],` (0 destination writes);
  `disassemble_function` `0x740A0`, `0x73E28`, `0x8BAF0`, `0x8A938`,
  `0x88940`, `0x88860`, `0x888FC`, `0x948AC`; `disassemble_bytes` windows
  `0x8B9CC`(292 B), `0x8B22F`(193 B), `0x8B55C`(57 B), `0x4B02C`(137 B),
  `0x4B0B5`(112 B), `0x4B1A6`(258 B), `0x8AF38`(296 B), `0x8A938`(296 B),
  `0x8AA60`(272 B), plus the §3.1 windows listed in Provenance; `read_memory`
  `0x110794` (140 B), `0x1106AD` (48 B), `0x8A8E0` (36 B), `0x8A904` (56 B),
  `0x93B98` (40 B), `0x9A8E0` (48 B); `get_xrefs_to` `0x740A0` (27),
  `0x8A938` (39);
  `get_function_by_address` `0x8A938` (body `0x8A938..0x8AF25`), `0x8A938`
  callee entry probes (`0x88C82`/`0x895AC`: no function definition).
* **Address model (restated).** Code addresses are the EXE link addresses;
  data immediates in the listing are already flat (`[0x157A4D]` is storage
  `0x157A4D`). Inline `CS:` tables quoted with their raw address resolve to
  that same address in `/FIFA96.EXE` (`JMP CS:[EAX*4+0x8a8e0]` reads the table
  at `0x8A8E0`; first-hand `read_memory`), unlike the loader image's `+0x10000`
  rule (FU-83 Method; the two images differ there and the EXE is
  authoritative).
* Every numeric claim is quoted; unresolved items are numbered open legs.
  No dynamic capture was available this slice (consistent with FU-142 §I.1).

## 1. The phase byte `[0x157A4A]>>24` / `[0x157A4D]`

### 1.1 Storage and writers

The phase is the top byte of the dword at `0x157A4A`, i.e. the byte at
`0x157A4D`. `search_instructions operand 157a4d` returns exactly five
instructions (re-run this slice):

| site | instruction | function |
|---|---|---|
| `0x73E4E` | `MOV byte [0x157A4D], AH` (AH=0) | `FUN_00073E28` reset |
| `0x740A0` | `MOV AH, byte [0x157A4D]` | `FUN_000740A0` setter (previous phase) |
| `0x740AC` | `MOV [0x157A4D], AL` | `FUN_000740A0` setter (new phase) |
| `0x8316E` | `MOV AL, [0x157A4D]` | `FUN_00083164` read |
| `0x8D178` | `MOV AL, [0x157A4D]` | `FUN_0008D098` state switch |

There is **no writer of the `0x157A4A` dword as a whole**: the operand search
for a destination `[0x00157a4a],` returns zero matches; every other `157a4a`
hit is a load. The dword's low three bytes are the adjacent `0x157A49` block
(byte `0x157A4C` is `[0x157A49]>>24`, a **goal-side flag**, written 0 by
`FUN_00073E28 0x73E42`, 1 by `0x89E9A` and 1 by `0x8AE2A`; read at `0x741D7`
and `0x765AC` and as `[0x157A49]>>24` in ~20 sites). So "`[0x157A4A]>>24`"
always means the phase byte and nothing else.

### 1.2 Setter `FUN_000740A0` (`0x740A0..0x7410D`, 25 instructions)

```
0x740A0 MOV AH,[0x157A4D]     ; previous phase
0x740A6 MOV [0x157A4E],AH
0x740AC MOV [0x157A4D],AL     ; new phase = AL
0x740B1 MOVSX EAX,DX          ; side (DX)
0x740B4 IMUL EAX,EAX,0x835
0x740BA MOV [0x157AAF],DL     ; side byte
0x740C0 ADD EAX,0x1588A4      ; team block for side
0x740C5 XOR DL,0x1
0x740C8 CALL 0x8D098          ; phase machine, side
0x740CD MOVSX EAX,DX          ; other side
0x740D0 IMUL EAX,EAX,0x835
0x740D6 ADD EAX,0x1588A4
0x740DB CALL 0x8D098
0x740E0 EAX=[0x157A4A]; SAR EAX,0x18; CMP EAX,2; JNZ RET
0x740ED CALL 0x4C380          ; no-op (RET)
0x740F2 [0x15781D]=0
0x740FC EDX=0x15774C
0x74101 [0x157AB2]=1
0x74107 [0x157A73]=EDX
```

So every transition outside reset funnels here; `state`/`phase` in the engine's
`fifa96_match_phase_machine` are the same byte (FU-142 errata §7), and the
phase-2 arm is the "match live" latch.

### 1.3 Reset `FUN_00073E28` (`0x73E28..0x73EDC`, 51 instructions)

Writes `[0x157AA7]=0`, `[0x157A4C]=0`, `[0x157A4E]=0`, `[0x157A4D]=0`
(`0x73E42..0x73E4E`), `[0x157A60]`/`[0x157A5C]`/`[0x157A73]=0x15774C`/
`[0x157A9F]`/`[0x157A83]`/`[0x157A6C]`/`[0x157A6A]`/`[0x157A64]` and
`[0x157AAB]=0xFF`; then the camera-mode teardown `FUN_000702F8(0x30/0x40)`,
`[0x157A6D]`, `FUN_0007F144`, `FUN_0007A028`. This is the reset-to-kickoff
state: **phase 0 is the reset default**, not an in-play phase.

## 2. The 35-row table (flat `0x110794`, first-hand `read_memory` 140 B)

Phase `p` -> `stored = [0x110794 + p*4]`; in `/FIFA96.EXE` the stored dword is
already the runtime address (no `+0x10000`). Family from FU-83 §2/§3 (bodies
re-cited there); clock class from the flat `0x1106AD` read this slice.

| phase | handler | family | class | phase | handler | family | class |
|---|---|---|---|---|---|---|---|
| 00 | `0x6DE34` | placement | 2 | 12 | `0x6E1D0` | placement | 0 |
| 01 | `0x6E1D0` | placement | 0 | 13 | `0x6E244` | placement | 0 |
| 02 | `0x6DCC8` | placement | 1 | 14 | `0x6E244` | placement | 0 |
| 03 | `0x6DE44` | placement | 2 | 15 | `0x6DCC8` | placement | 0 |
| 04 | `0x6DE44` | placement | 2 | 16 | `0x000000` | none | 0 |
| 05 | `0x6E05C` | placement | 0 | 17 | `0x88DC8` | timeline | 1 |
| 06 | `0x6DD9C` | placement | 2 | 18 | `0x8922C` | timeline | 0 |
| 07 | `0x6DE44` | placement | 2 | 19 | `0x89FA4` | timeline | 0 |
| 08 | `0x6DD6C` | placement | 2 | 1A | `0x89620` | timeline | 1 |
| 09 | `0x6DD6C` | placement | 2 | 1B | `0x890EC` | timeline | 0 |
| 0A | `0x6DE34` | placement | 0 | 1C | `0x89110` | timeline | 1 |
| 0B | `0x6DE34` | placement | 0 | 1D | `0x89868` | timeline | 1 |
| 0C | `0x6DF4C` | placement | 0 | 1E | `0x8A798` | timeline | 1 |
| 0D | `0x6DE34` | placement | 2 | 1F | `0x88F4C` | timeline | 0 |
| 0E | `0x6DE34` | placement | 0 | 20 | `0x8B688` | timeline | 0 |
| 0F | `0x6DE34` | placement | 0 | 21 | `0x8B874` | timeline | 0 |
| 10 | `0x6E004` | placement | 0 | 22 | `0x8B900` | timeline | 0 |
| 11 | `0x6E1C8` | placement | 0 |

Class table flat `0x1106AD` (first-hand 48 B):
`2,0,1,2,2,0,2,2,2,2,0,0,0,2,0,0 / 0,0,0,0,0,0,0,1,0,0,1,0,1,1,1,0 / 0..`
(48 entries; the tail past `0x1F` is zero). Class 1 = `{02,17,1A,1C,1D,1E}`;
class 2 = `{00,03,04,06,07,08,09,0D}`; the rest 0. This re-verifies FU-62 §1.3
on `/FIFA96.EXE` (the port's `fifa96_match_state_class_table` matches).

### 2.1 Act selector `FUN_000888FC` (`0x888FC..0x8893E`, 20 instructions)

```
0x888FD MOVSX CX,byte [0x158828]     ; current act id
0x88905 CMP AX,CX; JZ RET
0x8890A [0x158828]=AL                ; new act id
0x8890F CWDE; SHL EAX,2; ADD EAX,0x1107EC
0x88918 [0x158829]=DL                ; stage
0x88920 EAX=[EAX]                    ; handler = phase_table[0x16+id]
0x88922 [0x158818]=0
0x88929 [0x158808]=EAX
0x8892E TEST BX,BX; JZ RET
0x88933 TEST EAX,EAX; JZ RET
0x88937 CALL dword [0x158808]        ; run the act handler directly
```

`0x1107EC` is `0x110794 + 0x16*4` = the phase-table slot of phase **0x16** (the
zero slot), so act `a` selects phase `0x16+a` (0..0xC -> 0x16..0x22) and
invokes that phase handler directly, decoupled from the phase byte. Act ids
observed in calls: 1 (`0x8ABAB`), 2 (`0x8AEC6`/`0x8AEDE`), 7 (`0x8AEAE`),
8 (`0x8AAA3`), 9 (`0x8AF0E`), 0xA (`0x8AB82`), 0xB (`0x8BADB`), 0xC
(`0x8BACF`).

## 3. Transition edges

### 3.1 `FUN_000740A0` call-site census (`get_xrefs_to 0x740A0` = 27)

27 unconditional call sites: 9 in `FUN_0008A938`'s table-2 arms, 10 in
table-referenced phase/act bodies (`0x87C71`, `0x88C82`, `0x88E82`, `0x88FB8`,
`0x8915B`, `0x89372`, `0x895AC`, `0x8972F`, `0x898DD`, `0x8A091`), 6 in the
goal-screen bodies (`0x93xxx`) and 2 in named functions (`0x8BAC1`
`FUN_0008B9CC`, `0x8BCCF` `FUN_0008BAF0`). The constant-argument sites:

| site | phase | side source | context (first-hand window) |
|---|---|---|---|
| `0x87C71` | 1 | `[0x157AAC]>>24` | arm before `FUN_00073E08` install + `0x8CFAC(rec,0x26)`; sets side `[0x157AAF] = team[+0x826]^1` (`0x87C50` window) |
| `0x88C82` | 0x0C | controlled | pre-17 act body: `0xF0` sound, camera copy `0x14C110->0x14C114`, phase 0x0C, `[0x158829]++` (`0x88C60` window) |
| `0x88E82` | 1 | controlled | phase-17 period-end arm: camera reset, `FUN_00073E28`, phase 1, `FUN_00073E08` (`0x88E60`) |
| `0x88FB8` | 0x15 | controlled | phase-1F body: camera `FUN_000700F4`, `FUN_00073E28`, phase 0x15, `FUN_00073E08` (`0x88F90` window) |
| `0x8915B` | 0x0A | 0 | phase-1C stage 0: `FUN_0004C374` (`[0x57A68]=0`), phase 0x0A, sounds `0x1E`/`0x190` (`0x89138`) |
| `0x89372` | 0x0A | ball team `^1` | phase-18: `FUN_000651F0(4)`, phase 0x0A, `[0x58818]=0`, `[0x158829]++` (`0x89350`) |
| `0x895AC` | caller stack word | ball team `^1` | phase-18 arm: `EAX = [ESP-2] SAR 16` (variable), `[0x58818]=0`, stage++ — **OL-72** |
| `0x8972F` | 0x0B | record team (zero-ext) | phase-1A: phase 0x0B, `FUN_0007D9A4(rec,0x17)`, `[0x58818]=0`, stage++ (`0x89710`) |
| `0x898DD` | 0x10 | controlled | phase-1D: if `[0x157A4A]>>24 != 0x10` -> phase 0x10 (`0x898C0`) |
| `0x8A091` | 0x0F | ball team `^1` | phase-19: phase 0x0F, install 0x16, `[0x57A68]=0`, stage++ (`0x8A070`) |
| `0x8AA89`/`0x8AB9F`/`0x8ABE7`/`0x8AC1C`/`0x8ADB4`/`0x8ADCC`/`0x8ADE4`/`0x8ADFC`/`0x8AF02` | 0 / 0x11 / 3 / 4 / 5 / 8 / 9 / 0x0D / 2 | situation/side args | `FUN_0008A938` table-2 arms (§3.2) |
| `0x8BAC1` | 0x0C/0x13/0x14 | derived | `FUN_0008B9CC` period chooser (§3.3) |
| `0x8BCCF` | 0x13 | 0 | `FUN_0008BAF0` phase-0C extra-time branch (§3.4) |
| `0x93DB2`/`0x94040`/`0x941FF`/`0x944A3`/`0x94681`/`0x94877` | 0 | 0 | goal-screen bodies with the `phase==2` head (`0x93D03 CMP EAX,2`), score via `FUN_00093944`, then phase 0 (`0x93D00`/`0x94020`/`0x941E0`/`0x94480`/`0x94660`/`0x94860` windows) |

The `0x8A938` head additionally calls `0x740A0` inside `FUN_0008A938`'s
table-2 arms (the nine sites above); the two named callers
(`FUN_0004B02C` 0x4B0A5/0x4B0D9 and `FUN_00088860` 0x888C8/0x888F2) call
`FUN_0008A938`, not the setter.

### 3.2 Situation dispatcher `FUN_0008A938` (`0x8A938..0x8AF25`)

Head (`0x8A938..0x8A99E`, first-hand):

```
ECX=EAX (situation); [ESP]=DX (side)
if (situation == 0) goto 0x8AA7B
if (situation == 0xB) goto 0x8AA7B
if ([0x14C32A] == 0) goto 0x8AA7B       ; pause/skip gate
if ([0x15B6C0] != 0) goto 0x8AA7B      ; a situation is pending
[0x15B6B8] = (side == 0)
CX = situation - 2
if (CX > 8) -> 0x8AA60                 ; default: [0x15B6A8]=0xA, [0x15B6C0]=1
JMP CS:[CX*4 + 0x8A8E0]                ; table 1 (0x8A8E0)
```

**Table 1** (`read_memory 0x8A8E0`, 36 B -> 9 dwords):
`0x8A99E x3, 0x8A9CD, 0x8A9E8, 0x8A9CD, 0x8AA60, 0x8AA23, 0x8AA23`. Its arms
*queue* `[0x15B6A8]` and set `[0x15B6C0]=1`:

| situation | CX | target | queued `[0x15B6A8]` | first-hand window |
|---|---|---|---|---|
| 2 | 0 | `0x8A99E` | 9 if side==0 else 0 | `0x8A9A5`/`0x8A9B6` |
| 3 | 1 | `0x8A99E` | 9/0 | `0x8A99E` |
| 4 | 2 | `0x8A99E` | 9/0 | `0x8A99E` |
| 5 | 3 | `0x8A9CD` | 7 | `0x8A9CD` |
| 6 | 4 | `0x8A9E8` | 5 if side==0 else 6 | `0x8A9ED`/`0x8AA08` |
| 7 | 5 | `0x8A9CD` | 7 | `0x8A9CD` |
| 8 | 6 | `0x8AA60` | 0xA | `0x8AA60` |
| 9 | 7 | `0x8AA23` | 1 if arg==1 else 2 | `0x8AA2F`/`0x8AA45` |
| 0xA | 8 | `0x8AA23` | 1/2 | `0x8AA23` |
| other | >8 | `0x8AA60` | 0xA | `0x8AA60` |

With situations 0/0xB (or the gates tripped) the dispatcher reaches `0x8AA7B`:
`if (BX != 0)`: `EDX=0; EAX=0; EBX=1; CALL 0x740A0` -> **phase 0, side 0**
(`0x8AA89`), then `[0x15882C]=side`, `EAX=8`, `[0x15882B]=CL` and
`JMP 0x8AF1A` -> `CALL 0x888FC(act 8, stage 0)` (`0x8AA8E..0x8AAA3`, raw
`read_memory 0x8AA98`); if `BX == 0` -> `0x8AAA8`,
whose range dispatch (`CX = situation`, ECX was *not* decremented on this
path) selects the pending path, ending at the **table 2** jump
`JMP CS:[CX*4 + 0x8A904]` (`0x8AB7A`). Both the `BX!=0` phase-0 fallback and
the table-2 outcomes are derived; the port models table 2 only (the fallback
is the caller-visible `FUN_000740A0(0,0)` edge, OL-73).

**Table 2** (`read_memory 0x8A904`, 56 B -> 13 dwords), situation ->
phase/act:

| situation | target | derived outcome | first-hand window |
|---|---|---|---|
| 0 | `0x8AB82` | phase 0x11 + act 0xA | `0x8AB82` |
| 1 | `0x8ABAB` | act 1 (camera `[0x158830]=0x1E0`, `[0x158838]=0`) | `0x8ABAB` |
| 2 | `0x8ABDB` | phase 3 | `0x8ABDB` |
| 3 | `0x8ABF3` | phase 4 + `[0x157AD4+side*2]++` | `0x8ABF3` |
| 4 | `0x8ADC0` | phase 8 | `0x8ADC0` |
| 5 | `0x8ADD8` | phase 9 | `0x8ADD8` |
| 6 | `0x8AC28` | phase 5 unless `[0x157AC2]` is 2/3; score/stat tables (unported) | `0x8AC28..0x8ADB4` |
| 7 | `0x8ADF0` | phase 0x0D | `0x8ADF0` |
| 8 | `0x8AE08` | act 7; `0x897F4` resets + score zero (unported) | `0x8AE08`/`0x8AEAE` |
| 9 | `0x8AEC6` | act 2 | `0x8AEC6` |
| 0xA | `0x8AEDE` | act 2, stage 1 | `0x8AEDE` |
| 0xB | `0x8AEF6` | **phase 2** (in play) | `0x8AEF6` |
| 0xC | `0x8AF0E` | act 9 | `0x8AF0E` |

The high-level callers show how the code is produced:
`FUN_00088940` (phase 2/0x10, `[0x5781D]!=0`) scans the ball/goal state and
calls `FUN_0008A938(6, ball side)` (`0x88B44`), `(3|4, side)` (`0x88BBD`) and
`(2, side, BX=1)` (`0x88C00`); `FUN_00088860` (phase 0x0C, called from
`FUN_0004B02C(0)`) calls `FUN_0008A938(8, RNG&1)` for period 4 else
`(1, [0x157AAB]>>24)` (`0x888C8`/`0x888F2`); `FUN_0004B02C`'s
`[0x14C1D4]|[0x14C1D6]` bit-1 arm calls `(8,0,0)` and sets `[0x157AC2]=4`
(`0x4B0D0..0x4B0DE`); the boolean-camera arm calls `(2, ball side ^1, BX=1)`
(`0x88C00`).

### 3.3 Period-change chooser `FUN_0008B9CC` (`0x8B9CC..0x8BAEF`)

```
0x8B9CF if ([0x157AC0] == 0) goto 0x8BAA7
  ; extra-time flag set
EBX=0x13; AL=[0x157AC2]; EDX=3
if (period < 4) {
  if (score([0x157AC5]) == score([0x157AC7])) goto 0x8BA0B
  EDX = (score_own <= score_other)          ; SETBE
  goto 0x8BA26
}
0x8BA0B if (period >= 4) EDX = ([0x1587D8] <= [0x1587D9])   ; signed SETLE
0x8BA2C if (EDX == 0 || EDX == 1) {
  if (FUN_00012230(EDX) != 0) EBX=0x14
} else { /* EDX==3, equal scores, period<4 */
  if (FUN_00012250(0) != 0) { EBX=0x14; EDX=0; }
  else if (FUN_00012250(1) != 0) { EBX=0x14; EDX=1; }
}
side = EDX==0 ? [0x157ABE] : EDX==1 ? [0x157ABF] : [0x157AAC]>>24
CALL 0x740A0(phase=EBX, side)               ; 0x8BAC1
if (extra != 0) act = 0xC else act = 0xB
CALL 0x888FC(act, 0)                         ; 0x8BAE7
; extra == 0 path (0x8BAA7):
act = 0xB
if (period < 4) { CALL 0x740A0(0x0C, [0x157AAC]>>24); }
; period >= 4: no phase write, act 0xB only
```

`FUN_0008B9CC` is called from the clock rollover (`0x8B574 CALL 0x8B9CC`,
first-hand window `0x8B55C..0x8B594`, immediately followed by `[0x157AB6]=0`
and `[0x157AC2]++`) and
from `FUN_0008BAF0` (`0x8BAC1` on the extra-time branch; `0x8BCCF` is
`FUN_0008BAF0`'s own direct `0x740A0(0x13,0)` call).

### 3.4 Post-period machine `FUN_0008BAF0` (phase 0x0C / 0x13 / 0x14)

First-hand windows: `0x8BAF0..0x8BE65` (379 instructions). The phase-keyed
branches: phase 6 sets a fixed camera lead (`0x8BB13`); phase 2 builds the
ball-follow lead unless period > 3 (`0x8BB55`); **phase 0x0C** (`0x8BC01`):
`[0x58818] += [0x57A64]` when `[0x58808]==0`, camera `0x158830=0x960`,
`[0x58834]=[0x58838]=0`, `FUN_00036200(3)`, `FUN_0008DCD4`, then the timer
gates `[0x58816]>>16 > 0x78` and `> 0xB4` with `FUN_00045001` (`0x8BC57`/
`0x8BC71`): on pass, `[0x157AC0]!=0` -> `CALL 0x740A0(0x13, 0)` (`0x8BCCF`),
else `[0x58822]=1` (match over) (`0x8BCD9`); **phases 0x13/0x14**
(`0x8BCE7`): `[0x58818] += delta`, lead `0x780`, and at `[0x58816]>>16 >
0x4B0` (0x13 also requires scores equal; 0x14 uses `0x258` and, on 0x13 the
0x12C fold, both gated by `FUN_00045001`) -> `[0x58822]=2` with the
`[0x10F360]` first-entry latch (`0x8BD81..0x8BDDD`). Then the common
`FUN_0008DCD4`/`FUN_0008DD70` tail.

### 3.5 The clock's period end (first-hand `0x8AF38` window)

`0x8AF41..0x8AF80`: `class = [0x1106AD+phase]`; run iff `class==1`, or
`class==2 && [0x14C302]==0`; else the tail at `0x8B590`. `0x8AFAC`:
`[0x57AC1] += delta`. `0x8AFD0..0x8AFF2`: `limit = period<=1 ? [0x5881A] :
(period<=3 ? [0x5881C] : stale)`. `0x8AFF2..0x8B04A`: the class-2 aux update —
`sec + 0x14` vs `aux + limit` selects one of two arms; see §4.1 for the exact
byte-level arithmetic and the true invariant.

## 4. G1 carry-forward: which phase ends periods under the selector-0 default

### 4.1 The class-2 aux arithmetic and the invariant (first-hand)

`FUN_0008AF38`'s per-second rollover (`0x8AFF2..0x8B04A`) updates `[0x57ABA]`
(aux) as follows, with `sec = [0x57AB6]`, `limit` as above and
`tick = [0x57ABC]`:

```
0x8AFF2  class != 2 -> tick = 0 (0x8B042), goto seconds
0x8AFFC  EAX = aux + limit            ; EDX = aux, EAX = limit (MOVSX)
0x8B008  EAX = sec + 0x14
0x8B013  CMP EAX, EDX                 ; (sec + 0x14) vs (aux + limit)
0x8B015  JL  0x8B020                  ; path B: sec + 0x14 <  aux + limit
0x8B017  INC word [0x57ABA]           ; path A: sec + 0x14 >= aux + limit -> aux++
0x8B020  MOV AX,[0x57ABC]             ; path B:
0x8B028  CMP EAX, 0x1E
0x8B02B  JNZ 0x8B036                  ; tick != 0x1E -> tick++ only (0x8B038/0x8B039)
0x8B02D  INC word [0x57ABA]           ; tick == 0x1E -> aux++
0x8B04A  [0x57AC1] -= 0x3C; [0x57AB4]++; [0x57AB6]++    ; the second rollover
```

The invariant: with `d = sec - aux`, path A advances both `sec` and `aux`
(each +1) so `d` is unchanged; path B with `tick != 0x1E` advances only `sec`
so `d` increases by 1; path B with `tick == 0x1E` advances both so `d` is
unchanged. Hence `d` is **nondecreasing**, and path B fires only while
`d < limit - 0x14` (the strict `JL`); once `d == limit - 0x14` the comparison
falls through to path A, which preserves `d`. Starting from the reset
`sec == aux == 0` (`FUN_00073EE0 0x73EFA..0x73F0E` writes sec/acc/aux/tick
zero) the bound `d <= limit - 0x14` holds for every class-2 phase, so the
completion equality `sec == limit + aux` (`d == limit`) is **unreachable for
every class-2 phase, assuming `limit > 0`**. The weaker flag condition
`sec == limit && aux != 0` (period 1, sets `[0x5882D]`, `0x8B241..0x8B26F`)
can be reached (e.g. at `d == limit - 0x14`, `sec == limit` gives
`aux == 0x14`), but it does not complete the period. aux has three zeroers:
`FUN_00073EE0 0x73F07` (match reset), the `0x88D2E..0x88D42` reset block
(also zeroes `[0x57AB6]`, `[0x5882D]`, `[0x58822]`), and `FUN_0008AF38`'s own
`0x8B645..0x8B64E` tail (zeroes aux when the phase is 0x0C).

Class-1 phases never touch aux (the whole `0x8AFF2..0x8B04A` block is behind
the `class==2` test), so their completion is `sec == limit`. The only class-1
phase in the live range `0x00..0x16` is **phase 2** (`0x1106AD` read, §2);
phases 0x17/0x1A/0x1C/0x1D/0x1E are class 1 but are timeline acts already
inside the post-period sequence.

So: **a period ends while the phase byte is 2 (in play)**. The engine tests'
forced `state.phase = 2` is the native in-play phase; the reset/selector-0
default phase 0 is the kickoff/placement phase, not a period driver.

### 4.2 The post-period sequence (selector-0 default)

On completion the rollover calls `FUN_0008B9CC` (`0x8B574`). With the
extra-time flag clear — the selector-0/no-extra-time default — and period<4 it
writes **phase 0x0C** on `[0x157AAC]>>24` and act 0xB (`0x8BAA7..0x8BABE`,
`0x8BADB..0x8BAE7`); period>=4 writes no phase (act 0xB only). `FUN_0004B100`'s
period-end dispatch then sets the period screen `[0x5FFC]=2` for periods 2/3
always and for period 4 when `[0x157AC0]==0` (`0x4B220..0x4B249`), while the
match loop's `FUN_0004B02C(0)` -> `FUN_00088860` (phase 0x0C) resets to phase 0
(`FUN_00073E28`) and calls `FUN_0008A938(1)` (or `(8, RNG&1)` for period 4).
Phase 0x0C itself is class 0 (clock stopped) and its `FUN_0008BAF0` machine
settles the match: `[0x58822]=1` (over) when `[0x157AC0]==0`, or phase 0x13
when the extra-time flag is set; phases 0x13/0x14 then set `[0x58822]=2`.
Extra time enters through `FUN_0008B9CC`'s flag branch (0x13/0x14), driven by
`[0x157AC0]`, which the period-1 case sets when the scores differ at
`0x8B2AC` (and via `[0x14C2EE]`; equal scores fall to `[0x10F378]`).

**Selector-0 answer:** periods end at **phase 2**; the period-end phase
sequence under the selector-0 (no-extra-time) default is **2 -> 0x0C -> 0**
(the 0xC/0 write followed by the kickoff situation), with the match-over word
`[0x58822]` set by the phase-0xC machine; when `[0x157AC0]` is set the
sequence is **0xC -> 0x13 -> 0x14** (extra-time halves, probed 0x13/0x14).

### 4.3 Integration note (acceptance tape, no change made)

`tests/test_engine_m2.c` forces `0x13`/`0x14` at kickoff and phase 2 for the
mechanics window as declared carry C3-OL2. The derived native meaning of
0x13/0x14 is the **extra-time half pair** (post-period, `[0x157AC0]` set), not
the kick-off phase; the tape's forcing is a stand-in for the unported
kickoff/act machinery (`FUN_0008A938` act 1/2 handlers, `FUN_00088860`) and
is **not changed here** (the tape is closed; recorded as a follow-up note,
not a driver that alters it). The engine's `fifa96_match_run` default phase 0
is the native reset phase; wiring the derived drivers into the engine
(`FUN_0008A938` act bodies, `FUN_0008B9CC`, `FUN_0008BAF0`) is follow-up
scope, not Task 10.

## 5. Port: the phase-row drivers

Added to `include/fifa96_loader/fifa96_action_handlers.h` +
`src/fifa96_loader/fifa96_action_handlers.c` (FU-83's module; no new target):

| original | port |
|---|---|
| flat `0x110794` 35 dwords | `fifa96_action_phase_row(phase)` -> `fifa96_action_phase_row_desc` (handler/family/clock_class; `phase >= 0x23` -> NULL hardening, FU-83 §1.1) |
| flat `0x1106AD` bytes | `clock_class` field of the same row |
| `FUN_000888FC` table `0x1107EC` = phase_table[0x16] | `fifa96_action_phase_act(act,&phase)`: `phase = 0x16+act`, `act > 0x0C` -> error |
| `FUN_0008A938` table 2 (`0x8A904`, 13 rows) | `fifa96_action_phase_situation(situation,&out)`: `phase`/`act`/`stage`/`flags`; situations 6/8 carry `OPEN_LEG`, situation 6 carries `EXTRA_HOLD` (`[0x157AC2]==2/3`) |
| `FUN_0008B9CC` (`0x8B9CC..0x8BAEF`) | `fifa96_action_phase_period_end(...)`: the derived phase/side/act choice; `probe[4]` are the `0x12230`/`0x12250` results (documented); `probe==NULL` -> error |
| `FUN_00073E28`/`FUN_000740A0` setter | not re-ported here (`fifa96_match_state_set_phase` already stores phase/prev; the `[0x15781D]`/`[0x157AB2]`/`[0x157A73]` phase-2 arm and the two-team `FUN_0008D098` loop stay the engine machine's domain, FU-142) |
| `0x8B9CC`'s post-call `FUN_000888FC(act,0)` body, `FUN_00036200(3)` camera call, `FUN_0008BAF0` end machine, `0x93xxx` goal bodies, `FUN_0008A938` table 1/act bodies | not ported (open legs 2..7) |

## 6. Tests (`tests/test_phase_drivers.c`, suite row unchanged count)

* `test_phase_rows`: all 35 handlers and clock classes pinned against the
  first-hand table; family boundary (0x15 placement / 0x16 none / 0x17
  timeline / 0x22 timeline); `0x23` -> NULL.
* `test_phase_act`: act 0/1/0xA/0xC -> phases 0x16/0x17/0x20/0x22; act 0xD ->
  error with the output untouched; NULL.
* `test_phase_situations`: every situation 0..0xC outcome (phase/act/stage),
  the 6 `EXTRA_HOLD|OPEN_LEG` and 8 `OPEN_LEG` flags, 0xD -> error, NULL.
* `test_phase_period_end`: selector-0 default (extra=0) 0xC/act 0xB for
  periods 0..3 on the controlled side; period>=4 no phase/act 0xB; extra-time
  own>other/own<other/equal arms with the 0x12230/0x12250 probes and the
  side_abe/side_abf/controlled picks; the d8/d9 signed compare for period>=4;
  NULL probe/out.
* RED/GREEN: the test file was extended first and failed to build against the
  missing declarations/definitions; after the port it builds and passes
  (observed in the task report). Full `make check` re-run in the report.

## 7. Errata (quoted)

* **FU-83 §7 open leg 1 ("`[0x57A4A]` writers") — closed**: exactly two
  writers (`FUN_00073E28` reset to 0, `FUN_000740A0` setter; the dword at
  `0x157A4A` is never written whole), and the transition graph is now
  enumerated (27 `FUN_000740A0` call sites, the `FUN_0008A938` situation
  table, `FUN_0008B9CC`, `FUN_0008BAF0`).
* **FU-83 §7 open leg 7 ("`FUN_0008D098`'s `[0x57A4D]` jump table")** — the
  state switch is `[0x157A4D]` (same byte as the phase); the 22-entry table
  `0x8D040` and its state arms are FU-137/FU-142's scope; this slice re-cites
  the byte identity only.
* **FU-62 §8 open leg "phase-transition graph" — partly closed**: the
  situation -> phase table (`FUN_0008A938`, including the phase-2 entry),
  the period-end chooser (`FUN_0008B9CC`) and the `0x740A0` census are
  derived; the variable-phase site `0x895AC`, the `FUN_0008A938` table-1
  consumers and the `0x93xxx` screen consumers stay open (below).
* **FU-62 §4.1 "`0x57A4A` dword"** — the low bytes `0x157A4A..0x157A4C` are
  the adjacent `0x157A49` block; `0x157A4C` is the goal-side byte
  (`[0x157A49]>>24`), not phase state (writers `0x73E42`/`0x89E9A`/`0x8AE2A`).
* **FU-142 §7 "state vs phase"** — re-verified on `/FIFA96.EXE`: the state
  switch at `0x8D178` reads `[0x157A4D]`, i.e. the same byte as the phase.

## 8. Open legs

1. **OL-72 — variable-phase site `0x895AC`.** The phase-18 act body passes a
   caller stack word (`[ESP-2] SAR 16`) as the new phase; its producer is not
   located (the phase-18 handler is table-referenced code with no function
   definition). The port leaves it out of the derived situation table.
2. **OL-73 — `FUN_0008A938` table 1 (`0x8A8E0`) consumers and the `BX!=0`
   fallback.** The queued `[0x15B6A8]` values and their consumer (the `0x93xxx`
   bodies read them and score, `FUN_000948AC` queues 3/4/7/8/9) are only
   partly decomposed, and the `0x8AA7B` `BX!=0` fallback (phase 0 + act 8) is
   derived but not part of the table-2 port; the phase-table-2 path is
   ported, the queue's full lifecycle and act-8 body are not.
3. **OL-74 — `FUN_00012230`/`FUN_00012250` semantics.** The 0x13/0x14 upgrade
   probes are not decomposed; the port takes their results as inputs.
4. **OL-75 — `[0x1587D8]`/`[0x1587D9]` semantics (period>=4 pick).** Written
   by the situation-6/8 arms (`0x8AC7D`, `0x8AE59..0x8AE6B` clear) and read by
   `FUN_0008B9CC`; the port takes them as inputs.
5. **OL-76 — `FUN_0008BAF0`'s unported tail inputs.** `FUN_00045001`,
   `FUN_0008DD70`, the `[0x10F360]`/`[0x10F378]`/`[0x14C2EE]`/`[0x14C1D0]`
   flags and the phase-2/6 lead-vector algebra are cited, not ported.
6. **OL-77 — `0x93xxx` goal-screen bodies.** The `phase==2` + `[0x15B6A8]`
   pending + jump table `0x93B98` + `FUN_00093944(side)` + phase-0 sequence is
   bounded (windows in §3.1); the surrounding screen/stat code is not.
7. **OL-78 — `FUN_0008A938` act bodies and table-2 arms 1/6/8 side effects.**
   The camera `0x1E0` arm, the score/stat table writes (`0x157AEE..0x157B1A`)
   and the `0x897F4` reset loop are cited but not ported (flags
   `OPEN_LEG`/`EXTRA_HOLD` on the port rows).
8. **OL-79 — kickoff entry into phase 2 under selector 0.** The path from the
   setup's `FUN_0004B02C(1)`/`FUN_0008A938(0)` no-op to the first phase-2
   situation is not statically closed (the runtime situation producers are the
   `0x88xxx`/`0x93xxx`/unanalyzed callers); the derived statement is the
   phase-2 period-end answer, not a proof of the opening kickoff's first
   transition.

## 9. Integration errata (M2 playability Task 3: run-loop wiring)

Plan task `docs/superpowers/plans/2026-10-07-fifa96-m2-playability-legs.md`
Task 3 wires this slice's derived drivers into the engine run loop. Added
`fifa96_match_run_phase_drive`
(`include/fifa96_engine/fifa96_match_run.h`,
`src/fifa96_engine/fifa96_match_run.c`), called once per granted 30 Hz frame at
the end of the frame body: after the FU-141 entity chain and the FU-142a
0x13/0x14 hook, before the FU-85 scene staging. That is the native order —
`FUN_0004B100` runs the entity block (`0x4B15E`/`0x4B181`) before the clock
`FUN_0008AF38` (`0x4B1A6`), whose rollover calls `FUN_0008B9CC` (`0x8B574`);
`FUN_00036C70` stages the render slots after `FUN_0004B100` returns (`0x49523`,
FU-60 §3.1).

**Call shape** (derived entry points, FU-143 §5):

* `fifa96_action_phase_row(mr->state.phase)` supplies the clock class for the
  class-1/class-2 gate; class 0 returns without a write, and a phase past the
  table (`row == NULL`) is the FU-83 hardening path.
* `fifa96_action_phase_period_end` runs the derived `FUN_0008B9CC` chooser for
  the completed period `[0x157AC2] - 1`: the FU-62 library increments the
  period at completion, while the native chooser call `0x8B574` precedes
  `INC [0x157AC2]` at `0x8B58A`. With the selector-0/no-extra-time default
  (extra_time clear, period < 4) it returns phase 0x0C, the controlled side and
  act 0xB; the driver writes the phase through `fifa96_match_state_set_phase`
  and mirrors `phase_machine.state`/`phase` (the native `[0x157A4D]` switch
  byte is the phase dword's high byte, §1.1).
* `fifa96_action_phase_act` and `fifa96_action_phase_situation` are **not
  invoked**: `out.act` is already derived by the chooser (0xB), and invoking
  the act selector would only re-derive `0x16 + act` while the selector's
  handler invocation and the `FUN_0008A938` act/situation bodies stay unported
  (OL-73/OL-78).

**Completion staging.** The plan fixes the driver signature to `(mr)` alone,
and the FU-62 library owns the derived `sec == limit + aux` test
(`src/fifa96_loader/fifa96_match_state.c` completion block; first-hand case
body `0x8B1DD`: `0x8B1E3 MOV BX,[0x5881A]` / `0x8B1EA CMP AX,BX` / `0x8B21D CMP
EAX,EDX` with `EDX = aux + limit` / `0x8B225 ECX=1`). The frame body stages the
library's `period_ended` output in
`struct fifa96_match_run.clock_period_ended`; the driver consumes the staging
one-shot first (the `ended` latch and the `clock_period_ended = 0` clear precede
the row lookup) and only then applies the class gate. Class-2 phases pass the
gate (the native `[0x14C302]` halt is clear in the engine:
`fifa96_match_state_tick` is called with `clock_halt = 0`), but the §4.1 aux
invariant still prevents a class-2 completion, matching the native.

**First-hand re-verification this slice** (all `/FIFA96.EXE`, read-only,
explicit program argument; the derived entry points diff clean against the
bytes):

* class gate `0x8AF41 MOV EAX,[0x157A4A]` / `0x8AF46 SAR EAX,0x18` /
  `0x8AF4B MOVZX DI,byte[EAX+0x1106AD]` / `0x8AF53 MOV AL,[EAX+0x1106AD]` /
  `0x8AF5E CMP EAX,2` / `0x8AF63 CMP dword[0x14C302],0` / `0x8AF75 MOVSX
  EDX,DI` / `0x8AF78 CMP EDX,1` / `0x8AF80 JZ 0x8B590` — class 1 runs, class 2
  runs while `[0x14C302]==0`, class 0 stops (`0x8B590` tail);
* rollover `0x8B574 CALL 0x8B9CC` / `0x8B579 MOV BH,[0x157AC2]` / `0x8B581 INC
  BH` / `0x8B583 MOV word[0x57AB6],0` / `0x8B58A MOV [0x157AC2],BH`;
* chooser selector-0 arm `0x8BAA7 XOR EAX,EAX` / `0x8BAA9 MOV AL,[0x157AC2]` /
  `0x8BAAE CMP EAX,4` / `0x8BAB1 JGE 0x8BAC6` / `0x8BAB3 MOV EDX,[0x157AAC]` /
  `0x8BAB9 MOV EAX,0xC` / `0x8BABE SAR EDX,0x18` / `0x8BAC1 CALL 0x740A0`, and
  the act pick `0x8BADB MOV EBX,1` / `0x8BAE0 MOV EAX,0xB` / `0x8BAE5 XOR
  EDX,EDX` / `0x8BAE7 CALL 0x888FC`;
* setter `FUN_000740A0` (25 instructions, §1.2 re-read) and act selector entry
  `0x888FC` (`0x888FD MOVSX CX,byte[0x158828]` / `0x88913 ADD EAX,0x1107EC`);
* period-0 completion case `0x8B1DD..0x8B22A` (the `sec == limit` /
  `sec == limit + aux` split and the `[0x5882D]` flag arm);
* situation dispatcher head `0x8A938` (`0x8A947 JZ 0x8AA7B` situation 0 /
  `0x8A94E CMP EAX,0xB` / `0x8A957 CMP byte[0x14C32A],0` / `0x8A964 CMP
  dword[0x15B6C0],0`).

**Observed behavior** (engine tests under ASan/UBSan; the CTest count is
unchanged, the fixtures extend `test_engine_match_frame`):

* a live class-1 phase 2 on a 1 s period reaches the derived period end without
  the fixture writing the post-period phase: phase 2 -> 0x0C (`prev_phase` 2,
  `phase_machine.state/phase` 0x0C) on the completing frame, lifecycle OVER,
  and the begun run's `run_end` teardown resets the phase to 0
  (`fifa96_match_state_init`, the `FUN_00073EE0`/`FUN_00073E28` reset surface);
* the M2-B forced-phase transcript is **byte-identical** with the driver wired
  (frames 1..165, `tests/golden/engine/m2-frames.txt` unchanged): the tape
  samples `state=` before each step, while the 0x0C write happens inside the
  exit step, so no transcript line changes and the golden is not re-pinned;
* the tape keeps its declared 0x13/0x14/2 forcing; no frame is re-pinned to
  hide a mismatch. Byte-identity is a replay-level check, not a proof that the
  driver reproduces forced behavior frame-for-frame: the completion
  consumption and the 2 -> 0x0C write are pinned by the
  `test_engine_match_frame` fixtures and by the v2 tape's post-exit mirror
  read; the forcing cannot be dropped because the live-phase entry stays
  unported (OL-84).

**New open legs** (extend §8):

1. **OL-84 — live-phase entry (kickoff) under selector 0.** The begin default
   is phase 0 (class 2); the engine has no derived path into phase 2 because
   the `FUN_0008A938` situation producers/act bodies are unported (§3.2,
   OL-79 consequence). The engine fixture enters at the native in-play phase 2
   and the tape keeps its forced phase-2 window; the driver's period-end write
   itself is derived.
2. **OL-85 — extra-time flag `[0x157AC0]` producer.** The driver passes
   `extra_time = 0` (the selector-0/no-extra-time default, §4.2); the period-1
   completion side effect that sets the flag (`0x8B2AC` window) and the
   `0x12230`/`0x12250` upgrade probes (OL-74) are unported, so the derived
   0x0C -> 0x13 -> 0x14 branch is not reachable from the engine.
3. **OL-86 — post-period 0x0C hold/reset timing.** `FUN_0008BAF0`'s selector-0
   arm sets `[0x58822]=1` behind the `[0x58816]` timer gates (`0x8BC57`/
   `0x8BC71`, §3.4), and `FUN_0004B02C(0)` -> `FUN_00088860` resets to phase 0
   and kicks off via `FUN_0008A938(1)` (§4.2). The engine lifecycle instead
   marks the run over on the completion frame and exits on the next step
   (`fifa96_match_run_step`'s documented one-step compression), so the 0x0C
   phase is not held for its native duration; OL-76's unported tail inputs are
   the producer gap.

## Provenance

Ghidra MCP on `/FIFA96.EXE`: `get_current_program_info`;
`search_instructions` operands `157a4a`, `157a4d`, `157a4c`, `157a49`,
`15b6a8`, `15b6c0`, `00157a4a],`, `157aba` (fix round 1); `read_memory`
`0x110794` (140 B),
`0x1106AD` (48 B), `0x8A8E0` (36 B), `0x8A904` (56 B), `0x93B98` (40 B),
`0x9A8E0` (48 B), `0x8AA98` (12 B, fix round 1); `get_xrefs_to` `0x740A0`
(27), `0x8A938` (39);
`get_function_by_address` `0x740A0`-callers `0x8A938`, `0x88C82` (none),
`0x895AC` (none); `disassemble_function` `0x740A0`, `0x73E28`, `0x8BAF0`,
`0x8A938`, `0x88940`, `0x88860`, `0x888FC`, `0x948AC`; `disassemble_bytes`
`0x8B9CC`(292 B), `0x8B22F`(193 B), `0x8B55C`(57 B), `0x4B02C`(137 B),
`0x4B0B5`(112 B), `0x4B1A6`(258 B), `0x8AF38`(296 B), `0x8AA60`(272 B),
`0x8ABAB`(49 B),
`0x8AC28`(360 B), `0x8AE08`(166 B), `0x8AEE0`(64 B), `0x8AE90`(85 B),
`0x87C50`(64 B), `0x88C60`(48 B), `0x88E60`(53 B), `0x88F90`(53 B),
`0x89138`(56 B), `0x89350`(64 B), `0x89590`(60 B), `0x89710`(64 B),
`0x898C0`(64 B), `0x8A070`(64 B), `0x93D00`(183 B), `0x93D90`(64 B),
`0x94020`(64 B), `0x941E0`(64 B), `0x94480`(64 B), `0x94660`(64 B),
`0x94860`(64 B), `0x8AAA0`(272 B), `0x8A938`(296 B), `0x73EE0`(26 B),
`0x73EF8`(29 B), `0x8B640`(21 B), `0x88D28`(29 B) (all fix round 1).

Analysis-only: no tool, capture-rig, ISO or Ghidra-project change. Port write
set: `include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c`, `tests/test_phase_drivers.c`.
`game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not staged.
