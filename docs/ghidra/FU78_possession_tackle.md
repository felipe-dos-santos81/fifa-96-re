# FU-78: the possession block and the tackle/duel family

Follow-on to FU-76/FU-77: fully derive the possession-family handlers (code 5
`0x7F194`, code 0x21 `0x85214`) and the tackle/duel-family handlers (code 0x23
`0x82F84`, code 0x18 `0x849B0`) reached from the action table flat `0x1106E0`,
tie them to the ported ball model (`fifa96_ball_pairing`, FU-73) and to the
per-frame duel snap `FUN_0007D430`, and port the clean gates as
`fifa96_action_possession_*` / `fifa96_action_tackle_*` / `fifa96_action_duel_*`.

Result in one line: **possession has three separate tracks — the per-record
flag `+0x9B` (set at `0x74567`/`0x755C0`/`0x76DAE`, cleared at
`0x71D2D`/`0x79A6C`/`0x89903`), the runtime possession block
`0x58724..0x5872F` (`[0x58724]` = carrier record, `0x5872A/B` = dribble dir
bytes, `0x5872C..0x5872E` = counters, `0x5872D` = a per-frame countdown set to
`0x14` by reception `FUN_0007A084 0x7A412` and decremented by `FUN_0004B100
0x4B163..0x4B17B`), and the ball staging block `0x58730..0x58746` (FU-73);
code 5 is the carrier machine (phase gate → claim → `+0x89 += delta` capped
`0x4B0` → camera-held target → 4-stage dir/hand-off table `0x7F184`) and code
0x21 is the controlled receive/claim machine (phase 2 + `rec == [0x57A83]`,
stage 0 active/offset/timer gates, nearest-opponent contact `FUN_0008DE8C` +
`FUN_0008DCD4` + `FUN_000CD474`, stage 1 event/offset gate, reset path hands
the ball actor to code 4 and pings `[0x58734]`); the tackle family is the
attempt gate `FUN_00082DD0` (tracked entity, camera timer `0x14`, close word
`0x180`, predicted target height `0x20..0x60` from `FUN_00071B9C(0x12)`,
metric `0xF0`, pitch/half bounds, facing cone `0x100`) which installs code
`0x0E` invoke-now, wrapped by code 0x23 (lob/timer `0x78`, slot button `0x40`,
flags `+0x99`/`+0x5D`, camera words `0x577F2/FA/F8/FE/100`, own-record
`team+0x7C7`, opponent `+0x6B`/`+0x77` `< 0x120`, vector `0x57788/0x57790`
metric `< 0x60`) which installs code `0x0F` invoke-now; and the duel family is
code 0x18 (stride `+0x7B = 2`, animation bytes `0x55`/`0x6A` abort, target
`(0x900,0)` metric into `+0x65/+0x67/+0x69`, stage-2 window `0x78..0x12C`
with the input byte `0x67EC & 0xF0` and distance `< 0x20`) whose resolution
transfers the control slot to the nearest eligible teammate of
`[0x5888F]`'s team around the `0x58897` vector via `FUN_0008DB6C` +
`FUN_000786A0`, plus the per-frame snap `FUN_0007D430` (already ported
`fifa96_ball_pair_decide`) and the carrier/chaser split at code 4's
`0x7F0D6..0x7F133` (own → code 5 invoked, type-4/5 opponent → code 6 when its
`+0x6B >= own`).**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58..FU-77).
  `disassemble_function`/`disassemble_bytes`/`read_memory` are the citation
  source for every quoted instruction; `decompile_function` was not used for
  any quote.
* **Address mapping (FU-76, restated).** Code/function addresses equal true
  link addresses; a data immediate Ghidra renders as `A` is flat `A+0x100000`
  (e.g. `0xF334` → `0x10F334`, `0xF33C` → `0x10F33C`, `0x10680` →
  `0x110680`); stored code pointers/CS jump tables resolve through `+0x10000`
  (e.g. code 5's stage table operand `0x6F184` → `0x7F184`; code 0x23's install
  target `0x1106E0[0x23]` = `0x82F84`). The `0x577xx`/`0x587xx` operands in the
  listings are the raw stored object-4 addresses (the loader relocates them);
  the calls below cite the stored addresses exactly as the listing renders
  them.
* **Tooling errata this slice.**
  * `disassemble_function 0x7F194` dropped the code-5 stage-0 arm
    `0x7F274..0x7F385` and skipped `0x7F386..0x7F3A0`; recovered with
    `disassemble_bytes` plus a manual decode of the bytes at `0x7F386`
    (`8a 44 24 24 a2 2a 87 05 00 …`). The dropped arm is quoted in §3.
  * `disassemble_function 0x82F84` and the row handler at `0x7D174` desync in
    their `MOV AL,[EAX+0x10680]` sequences (`0x82FBF`/`0x7D192`): the raw bytes
    (`read_memory 0x7D190`) are `8a 80 80 06 01 00 24 01` = `MOV AL,[EAX+
    0x10680]; AND AL,1`, not the `ADD byte ptr [ESI],1; ADD byte ptr [ECX+EAX],
    AH` pair the listing shows.
  * `FUN_00089EB0` is desynced from `0x8A2D0` through the code-0x18 install at
    `0x8A32E` (the listing shows `RETF 0xC829` at `0x8A2E0`); the true
    instruction stream was decoded from `read_memory 0x8A2D0` (bytes quoted in
    §7). Ghidra defines the function through `0x8A3FB`, so the installer is
    inside `FUN_00089EB0`; no static `MOV EDX,0x18` exists elsewhere in the
    image (`search_byte_patterns ba18000000`: only `0x29EB3`, `0x2DC14`,
    `0x31516`, `0x32BD8`, `0x3998F`, `0x71440` (camera), `0x8A326`, `0x9E65A`).
  * `FUN_0004C324` (called from code 0x18's resolution) has no defined
    function; its bytes were read raw (`0x4C324: e8 9b 7a 00 00` = a call into
    the `0x4C7C4` region) and its visible logic is quoted at block level only
    (open leg 9).
* Every numeric claim is quoted from the listings; unproven items are open
  legs (no guessed labels).

## 1. Family map

Code/address from the FU-76 table; installers from the 69 `CALL 0x7D9A4` sites
and the install census.

| code | runtime | family role (evidenced) | installer(s) cited |
|---|---|---|---|
| 05 | `0x7F194` | carrier/possession machine: block claim, `+0x89` cap, camera target, 4-stage table `0x7F184` | `0x75B67` (`FUN_0007550C`, code 0x1E), `0x7F133` (code 4 split, §8) |
| 21 | `0x85214` | controlled receive/claim: phase 2 + user gate, contact then hand-off to code 4 | `0x7D046` pressed row (phase 2, `[type8+0x110680]&1`, invoke-now) |
| 23 | `0x82F84` | tackle/lunge: stage 0→1, attempt `FUN_00082DD0` installs 0x0E, window installs 0x0F | `0x7D174` row (phase 2, type-flag bit 0, `+0x99==0`, `+0x5D==0`) |
| 18 | `0x849B0` | downed/claim slot hand-off: stride 2, `(0x900,0)` metric, timed resolution via NSEARCH+SWAP | `0x8A32E` in `FUN_00089EB0` (§7) |

Installers quoted:

```
0x7D010  (pressed handler)  if phase==2 && ([[rec+0x8E]>>24 byte table 0x110680] & 1)
0x7D03A     { ECX=1; EDX=0x21; EBX=0; CALL 0x7D9A4 }        ; invoke 0x21 now
0x7D174  (row handler)      if phase==2 && bit0 && [+0x99]==0 && [+0x5D]==0
0x7D1AE     { EDX=0x23; EAX=ESI; ECX=0; EBX=0; CALL 0x7D9A4 }
0x7F0F0  (code 4 split)     if (opp type8 in {4,5}) {
0x7F0FE     AX=[EBP+0x6B]; CMP AX,[EDX+0x6B]; JLE 0x7F113
0x7F108        EDX=6; EAX=EBP; ECX=0; JMP 0x7F131
0x7F113     EDX=6; EAX=[ESP+0xC]; ECX=0; EBX=0; CALL 0x7D9A4   ; 6 on opponent
0x7F131     XOR EBX,EBX; (0x7F133) EDX=5; EAX=EBP; ECX=1; CALL 0x7D9A4 }
0x75B50  (keeper region)    … CALL 0x8A938; EDX=5; EAX=[EBP-0xC]; EBX=0; CALL 0x7D9A4
```

## 2. The possession block `0x58724..0x5872F`

`FUN_0007F144` (14 instructions, `0x7F144..0x7F180`) is the block clear:

```
0x7F145  EDX=0; AH=0
0x7F149  dword [0x58724]=EDX      ; carrier pointer
0x7F14F  byte [0x58728]=AH; [0x58729]=AH; [0x5872B]=AH; [0x5872A]=AH
0x7F167  byte [0x5872C]=AH; [0x5872D]=AH; [0x5872E]=AH; [0x5872F]=AH
```

Its only caller is `FUN_00073E28 0x73ECD`, the match-reset routine that also
zeroes `[0x57A83]` (`0x73E6C`) and then calls `FUN_0007A028` (ball block clear,
FU-73 §1) at `0x73ED2` — i.e. possession and ball staging are reset together at
kickoff.

Field map (all uses read/written by this slice's quoted functions):

| address | width | writer(s) | reader(s) | evidenced use |
|---|---|---|---|---|
| `0x58724` | dword | code 5 claim `0x7F1FF`, `FUN_0007F144 0x7F149`, `FUN_00081908 0x819B7` | `FUN_0007A084 0x7A40A`, code 5 `0x7F1BF`, code 6 `0x8022D` | carrier record pointer |
| `0x58728` | byte | clear/claim | — | cleared only |
| `0x58729` | byte | clear/claim; `FUN_0007F7E0` rotates (`INC`/`AND 3` at `0x7FEDB..0x7FEE0`) | `FUN_0007F7E0 0x7FEA8`, `0x7FEF5` | rotation index 0..3 |
| `0x5872A` | byte | clear/claim; code 5 `0x7F38A` | `FUN_0007F7E0`? (reads `0x5872C`) | dribble dir x |
| `0x5872B` | byte | clear/claim; code 5 `0x7F397` | — | dribble dir z |
| `0x5872C` | byte | clear/claim; `FUN_0007F7E0 0x7FFE5` | code 5 `0x7FEBE`, code 6 `0x806D0` | dir counter |
| `0x5872D` | byte | clear/claim; `FUN_0007A084 0x7A412` = `0x14`; code 5 `0x7F51D` | `FUN_0004B100 0x4B163`, code 5 `0x7F2A8` | release countdown |
| `0x5872E` | byte | clear/claim; `FUN_0007F7E0 0x7FDD3/0x7FE39` | — | counter |
| `0x5872F` | byte | clear/claim; `FUN_00081056 0x81056`, `0x814A0` | `FUN_0007F7E0 0x7FCFB` | counter |

The countdown is the only field with a frame integrator outside the handlers:

```
0x4B163  CL=[0x5872D]; EDX=0x590D9
0x4B16E  TEST CL,CL; JLE 0x4B181
0x4B172  AL=[0x57A64]                 ; frame delta
0x4B177  CH=CL; CH-=AL; [0x5872D]=CH  ; unsigned byte decay
```

`FUN_0007A084` (FU-73 §3.3) sets it back to `0x14` when the receiving action's
actor is the carrier:

```
0x7A40A  CMP EBP,[0x58724]
0x7A412  MOV byte [0x5872D],0x14
```

So the block tracks "who is on the ball at the event level" and a short release
delay; the per-record `+0x9B` flag is a *separate* signal set only by the take
sites (§8).

## 3. Code 5 — carrier/possession (`0x7F194`)

Stage table: `JMP dword CS:[EAX*4+0x6F184]` at `0x7F26C` reads flat `0x7F184`
= `{0x6F274, 0x6F57C, 0x6F5E9, 0x6F627}` → `0x7F274/0x7F57C/0x7F5E9/0x7F627`
(stages 0..3; `CMP AL,3; JA` at `0x7F25F` bounds it).

### 3.1 Entry, phase gate, claim, timer (`0x7F194..0x7F248`)

```
0x7F19F  byte [rec+0x9E]=1
0x7F1A6  if (phase != 2) { FUN_0007DAB4(rec); return }     ; 0x7F1B3..0x7F1BA
0x7F1BF  if ([0x58724] != rec) {
0x7F1C7     clear the whole block (carrier=0, bytes 0x58728..0x5872F=0)
0x7F1FF     [0x58724]=rec
         }
0x7F20B  [[rec]+0x7B2]=rec; [[rec]+0x7B6]=0               ; team controlled/second
0x7F221  if (rec[+0x89] < 0x4B0) rec[+0x89] += (word)[0x57A64]
0x7F240  if (word [rec+0x81] != 0) return                 ; event/timer latch
```

The claim arm is the exact semantics ported as `fifa96_action_possession_claim`
(`+0x9E=1` and the team pointers are handler side effects and stay out of the
port). The timer cap is `fifa96_action_possession_timer`: `+0x89` is compared
as a dword (`0x7F227 CMP ECX,0x4B0; JGE`), added with the zero-extended delta
(`0x7F22F XOR EAX; MOV AX,[0x57A64]`).

### 3.2 Stage 0 — dribble direction selection (`0x7F274..0x7F386`)

```
0x7F274  if (dword [rec+0x69]>>16 > 0x40) { [0x57A83]=0; return }   ; word [+0x6B]
0x7F291  [0x57A83]=rec
0x7F299  if (word [rec+0x6B] > word [rec+0x77]) return
0x7F2A8  if (byte [0x5872D] > 0) return                  ; release countdown active
0x7F2B5  if (dword [rec+0x5D] != 0) return               ; airborne Y
0x7F2BF  type8 = [rec+0x8B]>>24
0x7F2C8  EBX = dword [0x57750]
0x7F2CE  DX = (int8)table[type8 + 0xF334]                ; dir x
0x7F2D6  AX = (int8)table[type8 + 0xF33C]                ; dir z
0x7F2E6  if (EBX > 0x38) { [ESP+0x24]=DX; [ESP+0x14]=AX; [ESP+0x18]=0x60 }
0x7F304  else if (slot != 0) { DX=slot[+0x20]; AX=slot[+0x21]; [ESP+0x18]=0x30 }
0x7F32D  else { team gates ([team+0x828]/0x829, +0x7BF, +0x8D);
0x7F36F          FUN_0007F7E0(rec, &x, &z, &speed) }     ; unported fallback
0x7F386  [0x5872A] = (byte)[ESP+0x24]
0x7F397  [0x5872B] = (byte)[ESP+0x14]
```

The two tables are flat `0x10F334`/`0x10F33C`; the `0x10F334` bytes are
`00 01 01 01 00 FF FF FF 01 01 00 FF FF FF 00 01`, the `0x10F33C` head is
`01 01 00 FF FF FF 00 01`. This is the piece ported as
`fifa96_action_possession_dribble_dir` (type arm `speed 0x60`, slot arm
`speed 0x30`, `resolved=0` for the `FUN_0007F7E0` fallback).

### 3.3 Stage 0 common tail (`0x7F3A1..0x7F57B`, block level)

* local target copy from camera `0x5774C/50/54` or own position, depending on
  `dword [rec+0x69]>>16 > 0x20` (`0x7F3A9..0x7F3CC`);
* camera-relative target `out.x = [0x5774C] + dword[rec+0x6B]>>16`,
  `out.z = [0x57754] + dword[rec+0x6D]>>16` (`0x7F42A..0x7F447`);
* gates `[rec+0x6B] > 0x28` (`0x7F44A`) and `[rec+0x6B] > [rec+0x77]`
  (`0x7F459`), then a speed-scaled (`<<3`, `*0xB5>>8`) accumulator and
  `FUN_0008DC68` metric (`0x7F48E..0x7F4CB`);
* `FUN_00092820(rec, 0x26)` (`0x7F4C9`), `FUN_00071C94(rec, &vec, …)`
  (`0x7F4E7`), `[0x5872D] = min(0x3C, …)` (`0x7F50D..0x7F51D`), the
  `[0x57A4F]` gate (`0x7F522`), `FUN_00079CCC` with `+0x9A=1/0`
  (`0x7F53B..0x7F547`) and `FUN_0006DA64` (`0x7F56D`).

Not ported (globals/camera/RNG, open leg 2).

### 3.4 Stage 1 (`0x7F57C..0x7F5E8`) and stage 2/3 (`0x7F5E9..0x7F665`)

```
0x7F57C  FUN_00092820(rec, 0x26)
0x7F58A  0x577BE=0x577C0=0x577C2=0     ; zero camera velocity
0x7F5A5  code = (rec[+0x8D] != 0) ? 6 : 0x30
0x7F5B5  FUN_0006E598(rec, code, type8, 0)
0x7F5C7  rec[+0x89]=0; rec[+0x92]++    ; stage 2
```

Stage 2 (`0x7F5E9`): if no slot return; `FUN_00079B1C(rec)` (snap target to
position, zero delta/speed/body timer/velocity, quoted in §4), then
`FUN_00079C50(rec, slot[+0x1D]>>24, slot[+0x1E]>>24)` (`0x7F607`); if
`word [slot+6] == 0` return, else `rec[+0x92] = 0` (`0x7F616`, loop to stage 0).

Stage 3 (`0x7F627..0x7F665`): if `rec[+0x44] == 0` return; `rec[+0x92] = 0`;
if `rec == [[rec]+0x7B2]` then `FUN_0007D9A4([0x58730], code 4, 0, 0)` +
`FUN_00079B58([0x58734])` — the ball actor is handed to code 4 and the receiver
timer is armed.

## 4. Code 0x21 — controlled receive/claim (`0x85214`)

```
0x8521F  if (phase != 2 || rec != [0x57A83]) { FUN_0007DAB4(rec); return }
0x85242  copy camera 0x5774C/50/54 -> rec+0x4D/0x51/0x55
0x8524D  switch (rec[+0x92]) {
0x85253    case 0: goto 0x85269
0x85259    case 1: goto 0x85352
0x8525F    default: return }
```

Stage 0 (`0x85269`):

```
0x85269  AH = rec[+0x8D]; rec[+0x9E]=1
0x85276  if (AH == 0) { FUN_0007DAB4(rec); return }        ; inactive -> reset, no hand-off
0x8528B  if (dword [rec+0x69]>>16 > 0x40) {                 ; word [+0x6B]
0x85296     if (dword [rec+0x89] > 0x3C) goto 0x85363       ; far + timed -> reset path
0x852A3     return }                                        ; far, not timed -> wait
0x852AD  zero 0x577BE/0x577C0/0x577C2
0x852CB  nearest = FUN_0008DE8C(rec+0x59, [rec]team+0x7A6, -1, &out)
0x852E7  FUN_0008DCD4(rec+0x59, nearest+0x59, &offset)
0x852FC  angle = FUN_000CD474(dx, dz) - rec[+0x7D] (mod 0x400)
0x85335  FUN_0006E598(rec, 0x4A, type8, 0)
0x85340  rec[+0x89]=0; rec[+0x92]++                         ; stage 1
```

Stage 1 (`0x85352`):

```
0x85352  if (rec[+0x44] != 0) goto 0x85363                  ; event ack -> reset path
0x85358  if (dword [rec+0x69]>>16 <= 0x40) return           ; contact kept -> wait
0x85363  FUN_0007DAB4(rec)
0x8536A  if (rec == [[rec]+0x7B2]) {
0x85375     FUN_0007D9A4([0x58730], code 4, 0, 0)
0x85388     FUN_00079B58([0x58734]) }
```

So the machine holds the controlled record on the camera-target until it either
loses activeness (reset, no hand-off), drifts > `0x40` for > `0x3C` frames
(reset + hand-off), sets the event flag `+0x44`, or grows past `0x40` in stage 1
(reset + hand-off). The claim arm's angle/event calls are not decomposed
(open leg 3). Ported as `fifa96_action_receive_step`.

## 5. Tackle attempt `FUN_00082DD0` (`0x82DD0..0x82F82`)

Entry: `EAX = rec`. Gates in order (all constants quoted):

```
0x82DDA  if (phase != 2 || rec != [0x577CA]) return 0
0x82DF6  if (dword [0x577F8]>>16 < 0x14) return 0
0x82E0E  if (dword [rec+0x69]>>16 > 0x180) return 0        ; word [+0x6B]
0x82E26  target = FUN_00071B9C(0x12, &vec)
0x82E32  if (vec.y > 0x60 || vec.y < 0x20) return 0
0x82E4B  FUN_0008DCD4(rec+0x59, &vec, &off)
0x82E59  if (off.distance > 0xF0) return 0
0x82E72  if (rec[+0x59] < -0x210 && rec[+0x59] > [0x5774C]) return 0
0x82E85  if (rec[+0x59] >  0x210 && [0x5774C] > rec[+0x59]) return 0   ; camera_x > pos_x
0x82EA3  if (side == 0 && rec[+0x61] < 0x690) return 0
0x82EB7  if (side == 1 && rec[+0x61] > -0x690) return 0
0x82EDD  angle = FUN_0008DD70(off.dx, off.dz)
0x82EF0  if (side == 0) { if (angle < -0x100 || angle > 0x100) return 0 }
0x82F18  else          { if (angle > -0x100 && angle < 0x100) return 0 }
0x82F32  diff = (angle - rec[+0x7D]) & 0x3FF; fold to <=0x200; if > 0x100 return 0
0x82F62  ECX=1; EDX=0x0E; EAX=rec; EBX=0; CALL 0x7D9A4     ; install 0x0E now
0x82F75  return 1
```

`FUN_0008DD70` is the sign-extending wrapper into the 10-bit atan
`FUN_000CD474` (FU-76 §3.5), so `angle` is exactly
`fifa96_action_kick_angle`'s result. `FUN_00071B9C` (`0x71B9C..0x71C3C`) builds
the predicted target: it copies camera `0x5774C..54`, and if
`word [0x577F0] > 0` sets `out.y = FUN_00070B94(word [0x577FA] + index)`
(`0x71BC3..0x71BD2`), then advances `out.x/out.z` by the camera velocity words
`0x577C0`/`0x577C2` scaled by the index (`0x71BD5..0x71C31`, the negative case
halves first). `FUN_0007E600` (`0x7E600..0x7E7C4`) is the same shape for code
4's decision with different constants: type-flag gate `0x110680`, index `4`,
x bounds `±0x1E0`, z bounds `0x7B0`/`-0x850`, installing 0x0E on success.

Ported as `fifa96_action_tackle_attempt`.

## 6. Code 0x23 — tackle/lunge (`0x82F84`)

```
0x82F8C  if (phase != 2) { FUN_0007DAB4; return }
0x82F9F  rec[+0x89] += (word)[0x57A64]
0x82FAD  switch (rec[+0x92]) {
0x82FBB    case 0 (JBE): {
0x82FC6       rec[+0x9E]=1
0x82FD3       if (rec[+0x8D] == 0) { FUN_0007DAB4; return }
0x82FDB       rec[+0x89]=0; rec[+0x92]=1 }        ; fall into stage 1
0x82FBF    case 1: goto 0x82FF3
0x82FC1    default: goto 0x8314C }
```

Stage 1 head (`0x82FF3..0x83055`, target arm, not ported):

```
0x82FF3  if (dword [0x577EE]>>16 > 0x70 && word [0x577FA] < word [0x57800]) {
0x8300F     if (slot != 0 && (word [slot+0x10] & 0x40)) {
0x83021        out = vector 0x57794; out.z -=/+= 0xC0 by [team+0x826] }
0x8304A     else out = vector 0x57788
0x83055     FUN_00079B58(rec) }
```

Stage 1 gates (`0x8305C..0x8314A`):

```
0x8305C  if (word [rec+0x85] != 0 || dword [rec+0x89] > 0x78) { FUN_0007DAB4; goto tail }
0x8307B  if (FUN_00082DD0(rec) != 0) goto tail              ; attempt installed 0x0E
0x8308B  if (slot && (word [slot+0x10] & 0x40)) goto tail
0x830A1  if (rec[+0x99] != 0) goto tail
0x830AE  if (dword [rec+0x5D] != 0) goto tail
0x830B8  if (word [0x577FA] <= word [0x577F2]) goto tail
0x830CB  if ([[rec]+0x7C7] != rec) goto tail                ; must own the "opponent" pointer
0x830DC  EDX=[team+0x7C7]; if (dword [EDX+0x69]>>16 >= 0x120) goto tail
0x830E9  if (word [EDX+0x6B] > word [EDX+0x77]) goto tail
0x830F3  if ((word [0x577F8]>>16 + word [0x57800]>>16) < word [0x577FE]>>16) goto tail
0x83112  if (FUN_0008DC68(word [0x57788]-rec[+0x59], word [0x57790]-rec[+0x61]) >= 0x60) goto tail
0x83137  ECX=1; EDX=0x0F; EAX=rec; EBX=0; CALL 0x7D9A4     ; install 0x0F now
0x8314A  goto end
0x8314C  tail: if (word [rec+0x6B] > word [rec+0x77]) FUN_0007DAB4
```

Ported as `fifa96_action_tackle_step` (the target arm and the `FUN_00079B58`
side effect — `if ([rec+0x99]==0) [rec+0x93]=0x10`, quoted below — stay out):

```
0x79B58  if (byte [EAX+0x99] == 0) byte [EAX+0x93] = 0x10
```

## 7. Code 0x18 — downed/claim slot hand-off (`0x849B0`)

```
0x849B7  word [rec+0x7B] = 2
0x849BD  rec[+0x89] += (word)[0x57A64]
0x849D9  switch (rec[+0x92]) {
0x849EC    case 0: {
0x849F4       anim = [[rec+0x28]][0]
0x849FE       if (anim == 0x55 || anim == 0x6A) return     ; animation abort
0x84A10       rec[+0x89]=0; rec[+0x92]=1 }                 ; fall into stage 1
0x849DD    case 1: goto 0x84A28
0x849E1    case 2: goto 0x84A6D
0x849E7    default: return }
0x84A28  stage 1: rec[+0x4D]=0x900; rec[+0x51]=0; rec[+0x55]=0
0x84A32  rec[+0x89]=0
0x84A50  FUN_0008DCD4(rec+0x59, rec+0x4D, rec+0x65)        ; distance/dx/dz -> +0x65/67/69
0x84A55  rec[+0x89]=0; rec[+0x92]=2                        ; fall into stage 2
0x84A6D  stage 2: if (dword [rec+0x89] < 0x78) return
0x84A78  if (dword [rec+0x89] > 0x12C) goto 0x84A94
0x84A80  AL = FUN_00045001([0x67EC] & 0xF0)
0x84A87  if (AL != 0) goto 0x84A94
0x84A89  if (dword [rec+0x63]>>16 (= word [+0x65] distance) >= 0x20) return
0x84A94  resolution:
0x84AA3     CALL 0x4C324(0x5774C, 0, 0, 0)                 ; side select (raw bytes, open leg)
0x84AA8     EDI = rec[+0x20]; rec[+0x9A]=1
0x84AB2     if (slot != 0) {
0x84AC7        found = FUN_0008DB6C(0x58897, [rec], -1, 1)
0x84AC5        FUN_000786A0(rec, found) }                  ; transfer control slot
0x84AD5  FUN_0007DAB4(rec)
```

The resolution therefore runs when `timer >= 0x78` and either the timer has
passed `0x12C`, the input byte `[0x67EC]` has a nonzero high nibble, or the
stage-1 distance is `< 0x20`; then the record's control slot is moved to the
nearest eligible teammate of its own team around the `0x58897` vector:

```
0x8DB6C  FUN_0008DB6C(EAX=pos ptr, EDX=team, BX=skip, ECX=fallback):
0x8DB89     for (i=0;i<0xB;i++, rec+=0xB2) skip if slot/0x98/0x9A/0x7BF/0x829
0x8DBDF     d = FUN_0008DC68(pos - rec[+0x59]); keep the largest -d (nearest)
0x8DBFB     if (none && fallback) return first rec with no +0x20 and +0x9A==0
0x8DC10     return team + 0xB2*index
0x786A0  FUN_000786A0(EAX=from, EDX=to):
0x786A2     if (from[+0x20] == 0 || to[+0x20] != 0) return
0x786B0     to[+0x20] = from[+0x20]; [[from+0x20]] = to; from[+0x20] = 0
0x786BF     zero slot words +4/+6/+8/+0xA/+0xC/+0x14/+0x16
```

`FUN_00045001` (`0x45001..0x45024`) is `return [0x67EC] & 0xF0`; the input byte
`0x67EC` is written by the input layer (`FUN_00045D0D 0x45DD2`,
`FUN_00045F6A 0x45F99/0x46146`, `FUN_00046246 0x462C5`). Installer context:

```
0x8A29F  EAX = [0x5888F] (record)
0x8A2BC  ESI = [0x5888F]
0x8A2C2  LEA EAX,[ECX*4]; MOVSX DI,[ESI+0x8D]; EBX=[ESI+0x8A]>>24
0x8A2D1  EBX += EAX - ECX; EDX = ECX*0x20 - ECX - ECX*4 ...
0x8A2E6  AL = byte [0x5888D]; AH = byte [EBX+0x57B90]; AH += AL
0x8A2FB  byte [EBX+0x57B90] = AH
0x8A306  DL = byte [EBX+0x57B90]
0x8A312  byte [EAX + 0x4C3A0] = DL
0x8A31A  EAX = (DL & 0x7F); if (EAX < 2) skip
0x8A326  EDX = 0x18; EAX = ESI; ECX=1; EBX=0; CALL 0x7D9A4   ; install 0x18 now
```

(decoded from the raw bytes; the Ghidra listing for this block is desynced,
Method). `[0x5888F]` is a record pointer, `0x58897` is a 12-byte vector written
by `FUN_0008A3FC` (`0x8A405/0x8A415..0x8A437` copies a position triple there)
and `FUN_0008A43C`; the block identity is an open leg (8). Ported as
`fifa96_action_duel_step` (resolution's side-select and NSEARCH+SWAP remain
caller work).

## 8. Duel resolution and the tie to the ported ball model

Three possession-related tracks and their ports:

| track | original | port |
|---|---|---|
| per-record possession flag | `record+0x9B` set 1 at `0x74567`, `0x755C0` (code 0x1E), `0x76DAE`; cleared 0 at `0x71D2D`, `0x79A6C`, `0x89903` (+ action `0x19` at `0x8990A`); read at `0x6E34D`, `0x7509A`, `0x75B29` | `fifa96_ball_pair_possess` / `fifa96_ball_pair_release` (FU-73) |
| event level carrier/balls | block `0x58724..0x5872F` (carrier, dir bytes, countdowns) | `fifa96_action_possession_*` (this slice) |
| ball staging | block `0x58730..0x58746` (actor/receiver/vector/trajectory/code) | `fifa96_ball_pair_*` + `fifa96_action_kick_*` (FU-73/76) |

Code 5/0x21 never write `+0x9B`; the flag moves with the take/release sites
only. Code 5 claims the *event* carrier block and code 0x21 hands the ball
actor `[0x58730]` to code 4. `FUN_0007D430` (FU-73 §3.4, ported
`fifa96_ball_pair_decide`) is the per-frame snap of the two controlled records
`team+0x7B2` called from `FUN_0004B100 0x4B2DA`; it writes only the
interceptor's output position (predicted closing `< current`, `< 0x40`) and
touches no flag/block field.

The carrier/chaser split at code 4 (`0x7F0D6..0x7F133`, quoted in §1) is the
action-level duel result: the record becomes code 5 (invoked now), and the
type-4/5 opponent gets code 6 (pursuit) when its `word[+0x6B]` is `>=` the
record's. Ported as `fifa96_action_duel_split` (`own_code = 5`, `opp_code = 6`
only when `opp_is_duel_type && opp_metric >= own_metric`).

## 9. Port: `fifa96_action_possession` / `fifa96_action_tackle` / `fifa96_action_duel`

Extends `include/fifa96_loader/fifa96_action_handlers.h` +
`src/fifa96_loader/fifa96_action_handlers.c` (caller-owned state, no globals,
no comments, `-fifa96_err_t`; links `fifa96_entity_update` for the metric and
reuses `fifa96_action_kick_angle`).

| original | port |
|---|---|
| `FUN_0007F144 0x7F144..0x7F180` + code-5 claim clear `0x7F1C7..0x7F205` | `fifa96_action_possession_reset` / `fifa96_action_possession_claim` (claim reports whether it changed) |
| code-5 timer `0x7F221..0x7F23A` | `fifa96_action_possession_timer` (`< 0x4B0` adds the zero-extended delta) |
| code-5 stage 0 head `0x7F2BF..0x7F386` | `fifa96_action_possession_dribble_dir` (type arm `>0x38` speed `0x60`, slot arm speed `0x30`, `resolved=0` = unported `FUN_0007F7E0`) |
| code-0x21 stage/offset/timer machine `0x85269..0x85361` + reset path `0x85363..0x8536A` | `fifa96_action_receive_step` (`reset` = `FUN_0007DAB4`, `handoff` = install 4 + `FUN_00079B58`) |
| `FUN_00082DD0 0x82DD0..0x82F82` | `fifa96_action_tackle_attempt` (`install_0e` = code `0x0E` invoke-now) |
| code-0x23 machine `0x82F84..0x8314C` | `fifa96_action_tackle_step` (`install_0e`/`install_0f` = codes `0x0E`/`0x0F`, `reset` = `FUN_0007DAB4`, tail reset) |
| code-0x18 machine `0x849B0..0x84AD5` | `fifa96_action_duel_step` (`stride=2` always, target `(0x900,0)` metric, window, `handoff` = NSEARCH+SWAP arm) |
| code-4 split `0x7F0F6..0x7F133` | `fifa96_action_duel_split` |
| `FUN_00079B58`, `FUN_00079B1C`, `FUN_00079C50`, `FUN_0007F7E0`, `FUN_00092820`, `FUN_0006E598`, `FUN_00071B9C`, `FUN_00045001`, `FUN_0008DB6C`, `FUN_000786A0`, `FUN_0004C324`, target arms | not ported (globals/RNG/camera/cut bodies; open legs 1–9) |

Struct `fifa96_action_possession` mirrors `0x58724` (carrier, index, rotation,
dir_x/dir_z, counters); `fifa96_action_receive` mirrors the code-0x21 inputs
(`timer89` = `+0x89`, `offset_word` = word `[rec+0x6B]`, `stage` = `+0x92`,
`active` = `+0x8D`, `event_flag` = `+0x44`, `is_team_target` = `rec ==
[team+0x7B2]`); `fifa96_action_tackle` mirrors the attempt + window inputs
(`cam_f8` = word `0x577F8`, `cam_f2/fa/100/fe` = `0x577F2/FA/57800/577FE`,
`close_word` = word `[rec+0x6B]`, `own_bound` = word `[rec+0x77]`,
`opp_close`/`opp_bound` = the opponent's `+0x6B`/`+0x77`, `vector_x/z` =
`0x57788/0x57790`, `slot_button_40` = `word[slot+0x10]&0x40`, `is_own` =
`team+0x7C7 == rec`); `fifa96_action_duel` mirrors `+0x89`, position, the
`+0x65/+0x67/+0x69` metric triple, `+0x92`, `[rec+0x28][0]`, `+0x20`, `+0x7B`.
`_Static_assert`s in the test pin the layouts.

## 10. Tests (`tests/test_action_possession.c`, suite 66 → 67)

* Layout `_Static_assert`s on every port struct offset/size.
* `possession_reset`: all fields cleared; NULL.
* `possession_claim`: same carrier keeps fields and reports `0`; new carrier
  clears + sets and reports `1`; NULL.
* `possession_timer`: add, `0x4AF`+1 = `0x4B0`, cap at `0x4B0`, above-cap and
  delta 0 unchanged, NULL.
* `possession_dribble_dir`: type arm at `distance 0x39` (table values, speed
  `0x60`), slot arm at `0x38` (`0x30`), unresolved `0x38`/no-slot, NULL
  tables/out.
* `receive_step`: inactive reset (no hand-off); offset `0x41` timer `0x3C`
  wait vs `0x3D` reset (hand-off only for `is_team_target`); offset `0x40`
  advances and clears the timer; stage 1 event flag and `0x41` reset; stage 2
  no-op; NULL.
* `tackle_attempt`: happy install `0x0E`; phase, tracked, camera-timer `0x13`,
  close `0x181`, height `0x1F`/`0x61`, distance `0xF1`, both x bounds, z
  bounds by side, side-1 cone `0x80`, facing cone `0x100`/`0x101`; NULL.
* `tackle_step`: stage 0 advance zeroes the timer and installs `0x0E`; stage 0
  inactive and phase resets; window installs `0x0F`; lob/timer `0x78`/`0x79`
  reset boundary; each window gate (slot `0x40`, `+0x99`, `+0x5D`,
  `cam_fa<=cam_f2`, `is_own`, opp `0x120`, opp bound, camera sum, vector
  `0x60`); tail reset `[+0x6B] > [+0x77]`; stage 2; NULL.
* `duel_step`: `stride=2` always; animation `0x55`/`0x6A` abort; stage 0/1
  fall-through to stage 2 with the `(0x900,0)` metric (`0x100/-0x100` →
  `0x160`); wait `< 0x78`, resolve at `0x78` with distance `< 0x20`, wait with
  distance `>= 0x20`, input high-nibble `0x40` resolves the low-nibble-only
  `0x0F` waits, `0x12C`/`0x12D` boundary, hand-off vs reset; NULL.
* `duel_split`: tie → opponent 6, own 5; strict own `>` suppresses 6;
  non-duel type suppresses 6; signed boundaries `-0x8000/0x7FFF`; NULL.

ASan+UBSan: `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
tests/test_action_possession.c src/fifa96_loader/fifa96_action_handlers.c
src/fifa96_loader/fifa96_entity_update.c` runs clean (likewise re-run for
`test_action_handlers.c`/`test_action_locomotion.c`). `make test`: 66/66
before, **67/67 after**.

## 11. Errata (quoted)

* FU-73 §3.1 "written 1 by the two take sites" — **extended**: there are
  three take sites; the additional `0x755C0` (`MOV byte [EAX+0x9B],0x1`) is in
  the code-0x1E keeper region (`FUN_0007550C`), followed by the same camera
  reset `0x700F4` and `[0x57A83] = rec` pattern (`0x755C7..0x755CF`). The
  consumer list also gains `0x7509A` (`MOV AL,[EDX+0x9B]` in the same region).
* FU-76 §2 code 0x18 "interception/duel" — **refined**: the NSEARCH+SWAP
  (`0x84AC7/0x84AD5`) transfers the record's control slot to the nearest
  eligible teammate of its own team around the `0x58897` vector
  (`FUN_0008DB6C`) via `FUN_000786A0`; the only located installer is inside
  `FUN_00089EB0 0x8A326` gated by the `0x57B90` counter (`&0x7F >= 2`) on the
  record tracked at `0x5888F`. A "ball duel" meaning is not evidenced; the role
  label stays open.
* FU-76 §2 code 0x23 "TMR93" — **clarified**: `TMR93` is `FUN_00079B58`
  (`0x79B58`), which sets `byte [rec+0x93] = 0x10` only when `[rec+0x99]==0`;
  code 0x23 calls it from the stage-1 target arm (`0x83057`), it is not the
  action timer (`+0x89`).
* §5 tackle-attempt camera gate `0x82E85` — **corrected (M2 Task 12 / FU-139
  §10.5)**: the listing's `[0x5774C] < rec[+0x59]` was inverted. First-hand
  `0x82E8E MOV EAX,[0x15774C]; 0x82E93 CMP EAX,[ESI+0x59]; 0x82E96 JLE
  0x82EA3` continues when `camera_x <= pos_x`, so the attempt returns only
  when `camera_x > pos_x`. The pre-Task-12 `fifa96_action_tackle_attempt` had
  the inverted gate; it is fixed and pinned by the discriminating fixture
  (`pos_x = 0x211`: `camera_x = 0` installs, `camera_x = 0x300` refuses).
  Cross-reference: FU-139 §10.5 erratum and the FU-137 Task-12 errata.
* FU-76 §2 code 05 "body reads the `0x58724` block (`[0x58724..0x5872F]`),
  `RESET`" — **extended**: the claim/timer/target selection and the four-stage
  table `0x7F184` are derived; the block field map is §2.
* FU-76 §2 code 0x21 "reads `[0x58730]`/`[0x58734]`, `NEAREST`, `METRIC`,
  `ANGLE`, INSTALL 4" — **confirmed** and extended with the phase/user gate,
  the offset/timer gates and the stage-1 event/offset gate.
* FU-76 §1.3 code-0x18 install at `0x8A32F` (census) — **refined**: the true
  call is at `0x8A32E`, inside `FUN_00089EB0`, whose Ghidra listing is desynced
  there (Method); the guard and the manual decode are quoted in §7.
* FU-77 §2.4 "installs 6 / 5 (type-4/5 opponent duel)" — **extended**: the
  split compares `word [rec+0x6B]` against `word [opp+0x6B]` (`0x7F0FE`, signed
  `JLE`); the opponent only gets code 6 when its metric is `>=` the record's,
  and the record always gets code 5 invoked (`0x7F133`).
* FU-73 §3.4 duel snap `FUN_0007D430` — **confirmed** as the controlled-entity
  snap; this slice adds only that no handler in the possession/tackle family
  reads or writes its state (the snap writes the output position triple for one
  frame).
* Tooling: code-5 listing truncation, code-0x23/row-handler desync and
  `FUN_00089EB0` desync (Method).

## 12. Open legs

1. **`FUN_0007F7E0`** (`0x7F7E0`, 454 attributed instructions): the no-slot
   dribble/target arm called from code 5 stage 0 `0x7F36F`; installs code 7
   (`0x7F928/0x7F970`), rotates `0x58729`, writes `0x5872C/0x5872E/0x5872F`,
   calls `FUN_0007E528`, `FUN_0008DCD4`, `FUN_00092AC8`, `FUN_0006DBCC`,
   `FUN_000741B4`, `FUN_0008DD70`, `FUN_00092820`, `FUN_0007D9A4`; not
   decomposed.
2. **Code 5 stage-0 common tail** `0x7F3A1..0x7F57B`: local target algebra,
   `FUN_00092820`/`FUN_00071C94`/`FUN_00079CCC`/`FUN_0006DA64` and the
   `[0x57A4F]` gate are cited by address only.
3. **Code 0x21 claim arm** `0x852AD..0x8534A`: the nearest-opponent angle
   folding and event `FUN_0006E598(rec, 0x4A, type8, 0)` semantics are not
   decomposed.
4. **`FUN_00089EB0`** (code-0x18 installer): the `0x57B90` counter, the
   `0x4C3A0` byte array, `0x5888C..0x5888F` and the eligibility gate
   (`[rec+0x8D]`, `[rec+0x8A]>>24`) are quoted but their meaning is not
   derived.
5. **`0x5888F`/`0x58897` block**: writers `FUN_0008A3FC`/`FUN_0008A43C`
   (`0x8A405`, `0x8A489`, `0x8A761`) and callers `0x93F0D`/`0x94740` are
   cited; the identity of the tracked record/vector is not asserted.
6. **Camera block words** `0x577EE`/`0x577F2`/`0x577F8`/`0x577FA`/`0x577FE`/
   `0x57800` and the vectors `0x57788`/`0x57794`: writers/readers are inside
   `FUN_000700F4`/`FUN_00070544`/`FUN_000736AC`/`FUN_00070DE0`/`FUN_0007E7C8`;
   the constants are ported with neutral names.
7. **Code 0x23 stage-1 target arm** `0x82FF3..0x83055`: the `0x57794` /
   `0x57788` vector selection and the `±0xC0` side nudge are quoted, not
   ported.
8. **Codes `0x0E`/`0x0F` bodies** (the attempt/decision and tackle-success
   install targets) are FU-76 class rows only.
9. **`FUN_0004C324`** (code-0x18 resolution side select): no defined function;
   raw bytes start with `CALL 0x4C7C4` and the visible tail reads
   `[0x57AC2]`/`[0x587D4]`; not ported.
10. **`FUN_00071B9C` index semantics**: the `0x12` argument of the attempt
    target predictor (`0x82E28`) and the `FUN_00070B94` height table are
    quoted, not interpreted.
11. **Possession counters** `0x58728`/`0x5872A`/`0x5872B`/`0x5872C`/`0x5872E`/
    `0x5872F` beyond the writers listed in §2.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`get_function_xrefs` 0x7F144, 0x82DD0, 0x8DB6C, 0x786A0, 0x7A084, 0x7A490,
0x7B9C4; `get_function_by_address` 0x8A2E0/0x8A32F/0x8A0A0 (undefined);
`disassemble_function` 0x7F194, 0x85214, 0x82F84, 0x849B0, 0x82DD0, 0x7E600,
0x71B9C, 0x8DD70, 0x45001, 0x8DB6C, 0x786A0, 0x7F144, 0x7DAB4, 0x79B58,
0x79B1C, 0x7F7E0, 0x73E28, 0x89EB0, 0x8A3FC; `disassemble_bytes` 0x7F274
(264 B), 0x7F57C (120 B), 0x7F5F6 (112 B), 0x7F37B (40 B), 0x7F386 (27 B),
0x4C324 (96 B), 0x4C340 (64 B), 0x7D010 (200 B), 0x7D0D7 (240 B), 0x7F0F0
(100 B), 0x7F060 (160 B), 0x7A2E0 (96 B), 0x8A240 (168 B), 0x755A0, 0x75080,
0x75B50, 0x4B140, 0x844C0, 0x84D80, 0x8A070; `read_memory` 0x7F184, 0x10F334,
0x4C320, 0x4C340, 0x8A2D0, 0x7D190; `search_instructions` operand patterns
`5872`, `5888f`, `58897`, `577ee`, `577fa`, `67ec`, `+ 0x9b]`, mnemonic `CALL`
operand `7d9a4` (69 sites), `EDX, 0x23`, `EDX, 0x18`; `search_byte_patterns`
`ba18000000`; `run_script_inline` (function range scan 0x89E00..0x8A500);
`read_memory` 0x58880 (code at the raw flat, confirming the stored-address
mapping).

Analysis-only outside the port: no tool, capture-rig, ISO or Ghidra-project
change. Port write set:
`include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c`,
`tests/test_action_possession.c`, `CMakeLists.txt` (one test block).
`game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not staged.
