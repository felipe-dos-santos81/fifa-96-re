# FU-316 — the pan re-arm and the camera walk (can claims fuse the place past 0x120?)

**Phase/Provenance:** M2 phase-31 Stage-1 recon-ahead wave (Task 3/4), 2026-10-22,
read-only. Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument). This session's `get_current_program_info` =
`FIFA96.EXE`, `watcom:LE:32:default`, image base 0, `.image` overlay, **2708**
functions. Every address/byte below is a fresh `disassemble_bytes` /
`decompile_function` / `get_xrefs_to` / `get_function_by_address` of **this**
session unless marked *carried* (then the source is named). The alias traps are
applied (`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`); the
`[0x1577EE]`/`[0x157750]`/`[0x15774C]` overlaps are read as the word/dword at the
quoted offset. No writes: no rename/comment/label/function/script/save;
`/FIFA96.EXE` untouched; the ISO untouched; no git. Two read-only measurements
of the existing `build/test_engine_m2` are cited as measurements, not as source
authority.

Question (plan Task 3 / spec §3 item 3): **can repeated natural claims fuse the
camera place past the scripted `0x120` plateau, and how far toward the armer?**
Deliverables: the exact re-arm conditions (what clears/rebinds the tracked
record; why the third scripted pan failed), the cumulative-place model, the
reachable maximum in natural play, the armer/scanner consequence, and a
GO/NO-GO on the camera walk. Legs to settle: FU310-L9's plateau, the tracked
lifecycle, the pan decay.

---

## 1. Verdict in one line

**GO on the walk model, NO-GO on a natural armer reach.** The `0x120` plateau is
**not** a lifecycle cap: the tracked cell has exactly **three** writers (one bind
`0x71D27`, two clears `0x70258`/`0x70054`) and **eleven** `FUN_00071C94` event
sites, so any claim/receive/keeper event *rotates* the exclusion and the previous
panning record's exclusion is lifted. What actually ends the scripted
spike is (a) the pan's own tail installing **row 5** on the firing record
(`0x7F125..0x7F133`, invoke-now) and (b) the half-line cone's geometry — only
two records ever satisfy `lane <= 0x40 - timer81` in the measured formation.
Each pan adds a **fixed lattice step** `T(s)` to the place (`T = 0x90` at the
measured seed `0x40`); the armer needs `|x| > 0x730`, i.e. **13** same-direction
pans at `T = 0x90` (12 pans land exactly on the OOB handler's `0x6C0`), or
**20** in z (19 land exactly on `0xAB0`). The measured ceiling in every run —
the T1 witness and the seven spike drives — is **2 pans / `0x120`**, and the
natural probe never pans at all (3 598 granted frames with every camera cell
zero; its sampled `lane_x` is 2541 vs the `<= 0x40` gate). Pan-only accumulation
*can* arm, but only in one uninterrupted
same-direction open-play stretch of >= 13 pans, and arming then produces a
**situation-2 throw-in** (the only scanner arm reachable at `|snap z| ~ 0`),
whose own place resets the camera.

---

## 2. The tracked lifecycle (fresh, this session)

### 2.1 Exactly three writers, 88 references

Fresh `get_xrefs_to 0x1577CA` = **88** references; the WRITE set is exactly three:

| site | body | effect |
|---|---|---|
| `0x71D27` | `FUN_00071C94` (the event setter) | `[0x1577CA] = EBP` — **bind** the event's actor record |
| `0x70258` | `FUN_000700F4` (the lean place reset) | `[0x1577CA] = 0` — **clear** |
| `0x70054` | `FUN_0006FFC0` (the table/reset helper) | `[0x1577CA] = 0` — **clear** |

Fresh bytes (verbatim):

```
; FUN_000700F4 tail (0x70246..0x7025E)
0x70246  30d2             XOR DL,DL
0x70248  30ff             XOR BH,BH
0x7024a  668915c8771500   MOV word[0x1577C8],DX
0x70251  31d2             XOR EDX,EDX
0x70253  b84c771500       MOV EAX,0x15774C
0x70258  8915ca771500     MOV [0x1577CA],EDX        ; *** tracked clear ***
0x7025e  89e2             MOV EDX,ESP
```

```
; FUN_0006FFC0 tail (0x7004C..0x70065)
0x7004c  31c0             XOR EAX,EAX
0x7004e  883514781500     MOV byte[0x157814],DH
0x70054  a3ca771500       MOV [0x1577CA],EAX        ; *** tracked clear ***
0x70059  a3ce771500       MOV [0x1577CE],EAX        ; per-side slot 0
0x70060  a3d2771500       MOV [0x1577D2],EAX        ; per-side slot 1
0x70065  66891512781500   MOV word[0x157812],DX
```

```
; FUN_00071C94 head/tail (0x71C99..0x71D27)
0x71c99  803d6c7a150000   CMP byte[0x157A6C],0
0x71ca0  0f8549010000     JNZ 0x71DEF               ; suspended -> NO reset, NO bind
0x71ca6  6a00             PUSH 0
0x71ca8  8b3554771500     MOV ESI,[0x157754]
0x71cae  56               PUSH ESI
0x71caf  8b3d50771500     MOV EDI,[0x157750]
0x71cb5  57               PUSH EDI
0x71cb6  a14c771500       MOV EAX,[0x15774C]
0x71cbb  50               PUSH EAX
0x71cbc  89d6             MOV ESI,EDX               ; the 6-byte step triple
0x71cbe  bfb8771500       MOV EDI,0x1577B8
0x71cc3  e82ce4ffff       CALL 0x700F4              ; *** the clear runs here ***
...
0x71d03  85ed             TEST EBP,EBP
0x71d05  7459             JZ 0x71D60                ; null actor -> no bind
0x71d12  8a8026080000     MOV AL,byte[EAX+0x826]    ; the actor's team side
0x71d1d  892c85ce771500   MOV [EAX*4+0x1577CE],EBP  ; the per-side tracked array
0x71d27  892dca771500     MOV [0x1577CA],EBP        ; *** the bind ***
0x71d2d  c6859b00000000   MOV byte[EBP+0x9B],0
```

So the cell is a **single-slot rotation**, not a lock: every `0x71C94` call first
clears it (via `0x700F4`) and then binds its own actor (unless the actor is null
or the `0x157A6C` suspension bails at `0x71CA0`). The clear at `0x70054` is only
reached on a match/period reset (`FUN_0006FFC0`'s fresh callers = `0x73D9B`
`FUN_00073D90`, `0x88883` `FUN_00088860`), and that helper also calls `0x700F4`
at `0x7000F`, so a reset clears *and* re-places the camera — it cannot be used to
accumulate.

**Alias correction for Stage-2:** the plan's "clears `0x700F4`/`0x70258`/`0x70054`"
are **two** clears, not three: `0x70258` is the store *inside* `FUN_000700F4`
(fresh `get_function_by_address 0x70258` -> `FUN_000700f4`, body
`0x700F4..0x702F6`); `0x70054` is the store inside `FUN_0006FFC0` (body
`0x6FFC0..0x70072`). This matches the engine comment at
`src/fifa96_engine/fifa96_match_run.c:4423-4424` ("0x71D27 the store,
0x70258/0x70054 the clears").

### 2.2 The eleven event sites (fresh `get_xrefs_to 0x71C94`)

| # | call site | body | live at `camY=0`, `speed=0`? |
|---|---|---|---|
| 1 | `0x7EFCF` | row-04 arm A (row byte `0x13`) | yes, cone-gated |
| 2 | `0x7F0D1` | row-04 half-line pan | **yes** (the walk's only writer) |
| 3 | `0x7A219` | `FUN_0007A084` receive arm | **yes** (no camera-state gate) |
| 4 | `0x82A6C` | row-0E stage-2 pan arm (`0x82820`) | no (`0x20 < camY < 0x70`) |
| 5 | `0x75DC4` | row-1E keeper release | needs a keeper claim |
| 6 | `0x77423` | keeper band place | needs a prior far place |
| 7 | `0x77DC6` | the unattributed `0x77D82` place (FU310-L4) | unproven |
| 8 | `0x6FA62` | aux-0x32 close cut | unported (`0x6E8E8`, FU153-T1-L1) |
| 9 | `0x71B8A` | the armer's own goal-view pan | only after arming |
| 10 | `0x70DD1` | `FUN_00070C08` record event | no (`speed > 4` + `camY != 0`) |
| 11 | `0x7F4E7` | the **row-05 carrier** camera event (`0x7F194..0x7F665` band) | yes at the same near-field geometry: `lane <= 0x28` + `lane <= bound`; **unported** (L8) |

**Consequence:** in natural open play with the camera parked at `Y = 0`,
`speed = 0`, the only *live* tracked rebinders are the row-04 pan (`0x7F0D1`), the
receive arm (`0x7A219`), and — because the pan installs row 5 on its own firing
record — the row-05 carrier event (`0x7F4E7`) when that record stays inside
`lane <= 0x28`. That is exactly the phase-31 hypothesis: **the claim is the re-arm
producer**, and its camera effect is independent of the pan
(native `0x7A207..0x7A219`: the seed triple `0x158738` and height
`[0x15873C]>>16` are staged before the call; no camera gate exists on this
path). The receive's height is its staged trajectory word; a value `> 0xF0` also
arms the follow (`fifa96_camera_rate_event`, `camera.c:530`), which is the
phase-30 waiver, not this slice.

Fresh row-05 gate bytes (`0x7F44A..0x7F4E7`):

```
0x7f44a  8b4569         MOV EAX,dword[EBP+0x69]   ; >>16 = the +0x6B lane word
0x7f44d  c1f810         SAR EAX,0x10
0x7f450  83f828         CMP EAX,0x28
0x7f453  0f8f03020000   JG 0x7F65C                ; lane > 0x28 -> no event
0x7f459  668b456b       MOV AX,word[EBP+0x6B]
0x7f45d  663b4577       CMP AX,word[EBP+0x77]    ; bound
0x7f461  0f8ff5010000   JG 0x7F65C
0x7f467  668b4571       MOV AX,word[EBP+0x71]    ; speed71
0x7f46f  c1e003         SHL EAX,0x3               ; the step seed
0x7f472  6685db         TEST BX,BX                ; the slot word
...
0x7f4d0  833d5077150010 CMP dword[0x157750],0x10 ; camY
0x7f4d7  0f9fc0         SETG AL                   ; ramp param = (camY > 0x10)
0x7f4e7  e8a827ffff     CALL 0x71C94
```

The port has no counterpart: `fifa96_action_carrier_arm`
(`src/fifa96_loader/fifa96_action_handlers.c:439`) emits no camera event, so the
pan's own row-5 hand-off cannot rebind the tracked cell in the port today
(LEG L8).

---

## 3. Why the third scripted pan failed (FU310-L9's plateau)

Three independent blocks, all fresh or measured:

### 3.1 Block 1 — the half-line event gate excludes the tracked record

```
; the row-04 half-line arm (fresh 0x7ED80..0x7F12F)
0x7ed89  b840000000     MOV EAX,0x40
0x7ed8e  668b9d81000000 MOV BX,word[EBP+0x81]     ; timer81
0x7ed95  668b4d6b       MOV CX,word[EBP+0x6B]     ; lane (the +0x6B lane word)
0x7ed99  29d8           SUB EAX,EBX
0x7ed9b  6639c8         CMP AX,CX
0x7ed9e  7d2e           JGE 0x7EDCE               ; lane <= 0x40-timer81 -> near path
...
0x7edce  663b4d77       CMP CX,word[EBP+0x77]     ; bound
0x7edd2  0f8f60030000   JG 0x7F138                ; bail
0x7edd8  833d5077150050 CMP dword[0x157750],0x50  ; camY (the dword at 0x157750)
0x7eddf  7e6a           JLE 0x7EE4B               ; camY <= 0x50 -> the pan path
...
0x7ee4b  8b556d         MOV EDX,dword[EBP+0x6D]   ; >>16 = word[+0x6D]
0x7ee4e  8b456b         MOV EAX,dword[EBP+0x6B]   ; >>16 = word[+0x6B]
0x7ee51  c1fa10         SAR EDX,0x10
0x7ee54  c1f810         SAR EAX,0x10
0x7ee57  e814ef0000     CALL 0x8DD70
0x7ee5c  662b457d       SUB AX,word[EBP+0x7D]     ; the facing byte
0x7ee60  80e403         AND AH,0x3                ; fold to [0,0x400]
0x7ee66  81fa00020000   CMP EDX,0x200
0x7ee6c  7e09           JLE 0x7EE77
0x7ee6e  ba00040000     MOV EDX,0x400
0x7ee73  29c2           SUB EDX,EAX
0x7ee78  3dab000000     CMP EAX,0xAB
0x7ee7d  0f8fb5020000   JG 0x7F138                ; ** the 0xAB cone **
0x7ee83  8b0dca771500   MOV ECX,[0x1577CA]        ; tracked
0x7ee8d  39cd           CMP EBP,ECX
...
0x7ef42  8b4528         MOV EAX,dword[EBP+0x28]   ; the anim row byte
0x7ef47  25ff000000     AND EAX,0xFF
0x7ef4c  83f813         CMP EAX,0x13
0x7ef4f  0f8599000000   JNZ 0x7EFEE               ; row != 0x13 -> the pan arm
...
0x7efee  3b2dca771500   CMP EBP,[0x1577CA]        ; *** is this the tracked record? ***
0x7eff4  0f84dc000000   JZ 0x7F0D6                ; *** skip the event, still go to the tail ***
```

The gate set of the walk is therefore: `lane <= 0x40 - timer81` **and**
`lane <= bound` **and** `camY <= 0x50` **and** folded angle `<= 0xAB` **and**
`EBP != [0x1577CA]`. The port reproduces every term
(`src/fifa96_loader/fifa96_outfield.c:610-635, 682-707`;
`src/fifa96_engine/fifa96_match_handlers.c:2667-2673`).

### 3.2 Block 2 — the pan's own tail installs row 5 on the firing record

```
; the shared tail (fresh 0x7F0D6..0x7F140)
0x7f0d6  80bd8d00000000   CMP byte[EBP+0x8D],0      ; active
0x7f0dd  7446             JZ 0x7F125
0x7f0e7  8b828e000000     MOV EAX,dword[EDX+0x8E]   ; >>24 = the other's +0x91 code
0x7f0f0  83f805           CMP EAX,0x5
0x7f0f3  7405             JZ 0x7F0FA
0x7f0f5  83f804           CMP EAX,0x4
0x7f0f8  752b             JNZ 0x7F125
0x7f0fe  668b456b         MOV AX,word[EBP+0x6B]
0x7f102  663b426b         CMP AX,word[EDX+0x6B]
0x7f106  7e0b             JLE 0x7F113
0x7f108  ba06000000       MOV EDX,0x6               ; row 6 on self, no invoke
0x7f111  eb1e             JMP 0x7F131
0x7f113  ba06000000       MOV EDX,0x6               ; row 6 on the other, no invoke
0x7f120  e87fe8ffff       CALL 0x7D9A4
0x7f125  b901000000       MOV ECX,0x1               ; *** invoke-now ***
0x7f12a  ba05000000       MOV EDX,0x5               ; *** row 5 (the carrier machine) ***
0x7f12f  89e8             MOV EAX,EBP
0x7f131  31db             XOR EBX,EBX
0x7f133  e86ce8ffff       CALL 0x7D9A4
0x7f138  ...              RET
```

`0x7F125` is reached **unconditionally** from `0x7F0DD`/`0x7F0E5`/`0x7F0F8` and
from the event fall-through, so *any* record that passes the half-line gate —
including the **tracked** record that skips the event at `0x7EFEE` — leaves row
04 with row 5 installed. The port models the installs
(`outfield.c:709-720`, `handlers.c:2797-2810`); the `invoke` field is carried,
not consumed, per the standing invoke-now convention
(`include/fifa96_loader/fifa96_outfield.h:222-229`).

### 3.3 Block 3 — the formation supplies only two eligible records

Measured (carried, FU310 §"Pan-cumulative armer spike" + its two errata): over
**3 598 granted frames** (12 000 steps) in seven drives, only records 9 and 10
ever satisfied the cone; the second pan bound record 10 (`tracked = 10`), record
10 then crossed the arm's *own* gates on gf 224..226 (`lane` 12/10/7, angle
`0xA8/0x97/0xA0`) with no event, and no other record entered
`lane <= 0x40`/`angle <= 0xAB` in the remaining **3 372** frames. The
never-re-entering claim key is the `0x7CD60` code gate (carried, FU310 T6
erratum §3 item 4).

**So the plateau is not a native cap on the place.** Records 9 and 10 were spent
by *two* independent mechanisms (the tracked slot and the row-5 install), and the
cone was never re-entered. A third pan needs a *record that is still on row 04
and not tracked*, or a tracked clear/rebind by one of the other ten `0x71C94`
sites followed by a re-entry into the cone.

---

## 4. The pan integration, the decay and the stop (fresh)

### 4.1 The per-frame integrator `FUN_000736AC`

Fresh `decompile_function 0x736AC` + bytes `0x73790..0x7380F`:

```
0x737a1  6685c9           TEST CX,CX            ; event_param (word at 0x1577F0)
0x737a4  7413             JZ 0x737B9
0x737a6  a1f8771500       MOV EAX,[0x1577F8]    ; >>16 = the timer word
0x737ab  c1f810           SAR EAX,0x10
0x737ae  e8e1d3ffff       CALL 0x70B94          ; the Y arc (height only)
0x737b4  a350771500       MOV [0x157750],EAX
0x737b9  66a1fa771500     MOV AX,[0x1577FA]     ; timer
0x737bf  663b05f4771500   CMP AX,word[0x1577F4] ; timer_limit
0x737c6  7f12             JG 0x737DA            ; -> the pan step
0x737c8  833d5c77150000   CMP dword[0x15775C],0 ; *** prev Y (the 0x157750 dword at +0xC) ***
0x737cf  7e0e             JLE 0x737DF
0x737d1  833d5077150000   CMP dword[0x157750],0 ; current Y
0x737d8  7f05             JG 0x737DF
0x737da  e8f1d1ffff       CALL 0x709D0          ; *** the landing disjunct ***
0x737df  66833dbe77150000 CMP word[0x1577BE],0  ; speed (word)
0x737e7  0f8483030000     JZ 0x73B70            ; no speed, no height -> freeze
0x737ed..0x73832         acc = vel*delta (the word loop)
0x73834..0x7384A         step = |acc|
0x73850..0x73883         pos_x += acc_x; pos_z += acc_z; anchors
```

The block is entered when `speed != 0 || event_param != 0`; it exits (the place
freezes) when `speed == 0` and the height is 0. The OOB branch at the tail
(`fresh decompile 0x736AC`: `if (|pos_x| >= 0x6C1 || |pos_z| >= 0xAB1)
FUN_0007131C(...)`) is the armer's *only* caller (fresh `get_xrefs_to 0x7131C`
= 1: `0x73B9B`).

### 4.2 The pan step `FUN_000709D0` and the idle ramp `FUN_00070544`

Fresh `decompile_function 0x709D0`: the height band sink `0x65CF8`; the
`[0x157821]` counter capped at 100; `event_param = event_param * [0x157819] / 0x20`; the
`(input & 4)` random walk (both velocity words non-zero); then
`FUN_00070544(0)` over the stored step pair `0x1577B8`. Fresh
`decompile_function 0x70544` gives the idle arm constants: `F2 = 6`,
`F4 = 0xC` (`timer_limit`), `F6 = 0` (`timer`), `F8 = 0xC` (`ramp_divisor`) — the
same values the port seeds at `src/fifa96_loader/fifa96_camera.c:753-762`.

The decay is the **slow path** of `0x70544` (`k = [0x15781B]`, image `24`, fresh
`0x700F4` decompile: `0x15781B = 0x15781A + (0x20 - 0x15781A)/2 = 16 + 8`):
each pan step multiplies both velocity words by `24/32`. Measured decay
`5 -> 3 -> 2 -> 1 -> 0` (port assert `tests/test_engine_m2.c:3725-3727`; fresh
run below), i.e. integer truncation, and **the pan stops when the velocity
reaches 0** (`speed = 0` -> the integrator's `JZ 0x73B70`).

### 4.3 The cumulative-place model

Each pan is an *affine* add to the place, independent of the starting place:

```
v0 = trunc(s / 0xC)                ; s = the seed word (the fast path: F8 = 0xC, pan_counter = 0)
v_{i+1} = trunc(24 * v_i / 32)     ; the pan-step slow path
T(s) = 2 * (6*v0 + 7*v1 + 7*v2 + ...)   ; delta = 2, the 6/7/7/7 frame pattern
place += T(s) * sign                   ; the direction is the seed's sign (an 8-way lattice for a slot dir)
```

Measured instance: `s = 0x40` -> `v = 5,3,2,1` -> `T = 144 = 0x90` (two pans =
`0x120`; carried FU310 T6 + the fresh run below). The seed source is the
`0x7EFEE` arm's three-way switch (fresh bytes `0x7F003..0x7F054`):

| arm | seed | when |
|---|---|---|
| `0x7F003` | `(int8)dir[type] << 5` (`0x10F334/0x10F33C`) | `camY != 0` |
| `0x7F035` | `(int8)slot_dir << 6` = `+-0x40` | `camY == 0` and a slot |
| `0x7F056` | `word[+0x73] << 2`, `word[+0x75] << 2` | `camY == 0`, no slot, and `|cam_x| <= 0x630`, `|cam_z| <= 0xA20`; else `(0,0)` |

`T(s)` is monotone in `s`. At the measured `0x40` a pan is `0x90`; a no-slot
record whose velocity word is `0x10` gives the same `0x40`, and `0x20` gives
`s = 0x80` -> `T = 0x174` (372). Pans needed to cross the armer (`0x730` x /
`0xB20` z):

| seed `s` | `T` | pans to `> 0x730` | pans to `> 0xB20` |
|---|---|---|---|
| `0x40` (measured) | `0x90` | **13** (12 = `0x6C0` exactly) | **20** (19 = `0xAB0` exactly) |
| `0x60` | `0x140` | 6 (`0x780`) | 9 (`0xB40`) |
| `0x80` | `0x174` | 5 (`0x744`) | 8 (`0xBA0`) |
| `0xA0` | `0x1FA` | 4 (`0x7E8`) | 6 (`0xBDC`) |
| `0x100` | `0x3B8` | 2 (`0x770`) | 3 (`0xB28`) |

Note: the `0x6C0`/`0xAB0` coincidences are exact because `144 * 12 = 1728` and
`144 * 19 = 2736`; both thresholds are *strictly* `>`, so 12 pans in x and 19 in
z land exactly on the OOB handler's edge and do **not** call it.

---

## 5. The armer and the scanner (fresh)

### 5.1 The gate

```
0x71390  a14a7a1500       MOV EAX,[0x157A4A]   ; the phase dword, >>24 = the phase byte
0x71398  83f802           CMP EAX,0x2
0x7139b  7409             JZ 0x713A6
0x7139d  83f810           CMP EAX,0x10
0x713a0  0f8503050000     JNZ 0x718A9
0x713a6  8b44240e         MOV EAX,[ESP+0xE]    ; >>16 = abs z word
0x713ad  3d200b0000       CMP EAX,0xB20
0x713b2  7f12             JG 0x713C6
0x713b4  8b44240a         MOV EAX,[ESP+0xA]    ; >>16 = abs x word
0x713bb  3d30070000       CMP EAX,0x730
0x713c0  0f8ee3040000     JLE 0x718A9          ; not armed -> the mirror/tail
0x713c6  b701             MOV BH,0x1           ; *** the armed flag ***
0x713c8  8d542408         LEA EDX,[ESP+0x8]    ; the snapshot source = the camera triple
```

Fresh `decompile_function 0x7131C` adds the arming body: `[0x15781D] = 1`, the
snapshot triple `0x15777C/80/84`, `[0x157ACB] = 1`, the goal classifier
`0x70074`, and the **announce/threshold machine** (`0x71400..0x718A4`) — of
which only the mirror is ported. The mirror (`0x718A9..`) requires
`(0x14C1D4|0x14C1D6) & 1`; the image default of those range words is **0**
(carried FU-148 §13.1; the port derives the bit at
`match_run.c:466-470`). **Finding:** with the reflect bit **on**, the mirror
inside `0x7131C` fires at `|x| > 0x720` (`|z| > 0xB10`) — 0x10 *below* the armer
— and at the **measured** seed a pan's largest frame step is 10 (`vel 5 x delta
2`), so the mirror always intervenes in the `0x720 -> 0x730` gap and **a
reflect-bit config blocks the pan route to the armer**. The default (bit 0)
leaves the armer as the first boundary. A larger no-slot seed (L1: `s = 0x80`
gives `vel 10`, a step of 20) could jump the 0x10 gap and arm before the mirror
on that frame — a corner case that only L1's measurement can settle.

### 5.2 The consequence: armed -> a situation-2 throw-in

The scanner's gate is `phase 2/0x10 && goal_armed`
(`match_run.c:3500-3501`; native `0x8B623..`). Fresh scanner bytes:

```
; the corner/goal-kick arm (0x88B53..): the queued side is the tracked record's team
0x88b53  a14a7a1500       MOV EAX,[0x157A4A]
0x88b5b  83f802           CMP EAX,0x2
0x88b64  b81e000000       MOV EAX,0x1E
0x88b6e  a1ca771500       MOV EAX,[0x1577CA]        ; *** dereferenced without a null test ***
0x88b73  8b00             MOV EAX,[EAX]
0x88b75  8a9026080000     MOV DL,byte[EAX+0x826]    ; the tracked actor's team byte
0x88b7d  80f201           XOR DL,0x1                ; the queued side = tracked ^ 1
...
; the throw-in arm (0x88BCC..)
0x88bcc  a14a7a1500       MOV EAX,[0x157A4A]
0x88bd1  c1f818           SAR EAX,0x18
0x88bd4  83f802           CMP EAX,0x2              ; phase 2 only
0x88bd7  752c             JNZ 0x88C05
0x88be3  a1ca771500       MOV EAX,[0x1577CA]
0x88be8  8b00             MOV EAX,[EAX]
0x88bea  8a8026080000     MOV AL,byte[EAX+0x826]
0x88bf0  31d2             XOR EDX,EDX
0x88bf2  3401             XOR AL,0x1
0x88bf4  bb01000000       MOV EBX,0x1
0x88bfb  b802000000       MOV EAX,0x2               ; situation 2
```

The port's arm order (`match_run.c:3509-3536`) matches the native: `|snap z| <=
0xB20` -> situation 2 (throw-in) **before** the `goal_zone == 0` corner/goal-kick
arm. A pan-armed camera at `z ~ 0` therefore queues **situation 2**, never a
corner or goal kick, and the side is the *tracked* actor's team `^ 1` (the port
uses `match_run_ball_side`, a stand-in — leg L5). The throw-in's own place (row
`0x10` stage 3, `Y := 0x80`, native `0x8596B`) runs through `0x700F4`, so the
arming is **transient**: it re-places and clears the camera, and the accumulated
`|x|` is spent.

The armer's *own* pan (`0x71B8A` `FUN_00071C94(tracked, &local_30, sVar9)`, with
the height `sVar9 = camY + 0x40`) is the first honest **Y** source on this
route — but it lives inside the unported armed body (LEG L6), and it needs the
far-band branch (`|z| > 0x8BC`/`0xE73`, or `|x| > 0x6D3` with `camY < 0x1E`).

---

## 6. Port evidence and the port shape

### 6.1 The port's pan path (fresh source read)

| stage | port | matches native |
|---|---|---|
| the row-04 gate | `outfield.c:610-635` | every term (`0x7ED89..0x7EE7D`) |
| the tracked exclusion | `outfield.c:682` + `handlers.c:2667-2673` | the cell compare; **a controlled fallback is extra** (L3) |
| the seed switch | `outfield.c:685-701` | the three arms `0x7F003/0x7F035/0x7F056` |
| the event set + clear | `camera.c:900-957` (`tracked = NONE` at `:937`) | `0x71C94` -> `0x700F4` (`0x70258`) |
| the bind | `handlers.c:2788`, `match_run.c:4043`, `match_run.c:624` | `0x71D27` (but L4) |
| the ramp / fast+slow path | `camera.c:732-830` | `0x70544` (`F2/F4/F6/F8`, `k = 0x10/0x18`, divisor `0x20`) |
| the pan step | `camera.c:959-991` | `0x709D0` (band, cap 100, decay, walk, re-ramp) |
| the integrator | `camera.c:74-151` (call at `match_run.c:4411-4417`) | `0x736AC` |
| the OOB + armer | `match_run.c:4443-4445` -> `:3198-3234` | `0x73B70..0x73B9B` -> `0x7131C` |
| the follow gate | `match_run.c:3967-3993` | `0x739C6..0x73B6B` |
| the receive rebind | `match_run.c:4034-4044` | `0x7A207..0x7A219` |
| the insert of rows 5/6 | `outfield.c:709-720` -> `handlers.c:2797-2810` | `0x7F0D6..0x7F133` |
| the row-05 rebinder | **absent** (`fifa96_action_handlers.c:439` emits no event) | `0x7F4E7` (L8) |

### 6.2 The measurements (fresh, this session)

`build/test_engine_m2` (the existing binary; no source change) run as
`./build/test_engine_m2` printed:

* natural probe: `situation-0 kickoff at step 12 (match start 5), ... phase 2 at
  step 1802, 3598 granted frames (101 s), camera zero 3598/3598, ladder seam
  6118 calls / stage_select ran=0 ... cam=(0,0,0) lane_x=2541 fspd=0`;
* pan witness: `scripted claim/kick (copied tape, the cutscene isolated) place
  0 -> 0x90 -> 0x120 at steps 425/605 (first motion 339, tracked 9), Y=0, 358
  granted frames`.

(The `lane_x = 2541` is the keeper-ladder seam's sampled record, not a
formation-wide minimum; the stronger natural fact is the zero pan count over
3 598 granted frames with every camera cell at zero.)

The witness asserts (`tests/test_engine_m2.c:3713-3727`) `max_pos_x == 0x120`,
`y_nonzero_frames == 0`, `z_nonzero_frames == 0`, `decrease_frames == 0`,
`first_tracked == 9`, and the velocity sequence `5,3,2,1`. Both fixtures are
scripted/isolated (`:3623-3631`), never natural open play.

### 6.3 What Stage-2 must not change

* **The pan's arithmetic is frozen**: the seed/`0xC` fast path, the `0x18/0x20`
  slow path, the `6/7/7/7` cadence and the `0x90` step are asserted; a "longer
  pan" must come from the seed (the no-slot arm), never from a changed decay.
* **The tracked cell stays single-slot**: `event_set` clears, the handler binds.
  Do not add a "stack" or a "previous tracked" cell — the native has none.
* **The pan is one-shot per bind**: a repeat by the same record requires a
  rebind by another event *and* a row-04 re-entry (row 5/6 was installed).
* **The armer is not a phase-31 target**: the acceptance should pin
  `place > 0x120` / `pan_count >= 2`, not `goal_armed` (L7).

---

## 7. Numbered legs

* **FU316-L1 — the no-slot seed distribution (per-pan travel).** The measured
  pans all used `s = 0x40` (`T = 0x90`); the `camY == 0 && !has_slot` arm
  (`0x7F056`, `word[+0x73]<<2`) admits larger seeds and therefore longer pans
  (`s = 0x80 -> T = 0x174`). No measurement covers it, and the "pans to arm"
  count is `ceil(0x730/T(s))`. Owner: the `0x7F056` arm +
  `fifa96_outfield_row04_step` (`outfield.c:692-701`); measure with FU317's probe
  (sample the seed word and the resulting `pos` delta per event).
* **FU316-L2 — the landing pan-step disjunct is unported.** Native
  `0x737C8..0x737DA`: the pan step also runs when `(int32)[0x15775C] > 0 &&
  (int32)[0x157750] <= 0` (the camera *lands*), not only when
  `timer > timer_limit`. The port's site (`camera.c:97`) has the timer term only;
  FU-148 §12.4 item 3 understates the gate (FU-71 §2 row 10 has both). Latent for
  the Y-less walk, live after any height event. Owner:
  `fifa96_camera_update_walk` + `0x737B9..0x737DA`.
* **FU316-L3 — the controlled fallback in the tracked gate.** When
  `camera.tracked == NONE` the port sets `is_ball_track = (id == controlled)`
  (`handlers.c:2672`, the FU171-L2 stand-in); the native `0x7EFEE` compares the
  cell only, so with `[0x1577CA] == 0` the controlled record **can** pan. The
  port blocks the controlled record's first pan whenever the cell is empty.
  Owner: the row-04 state fill + FU171-L2, with the FU316 fixture (stage
  `tracked = NONE`, controlled record in the cone).
* **FU316-L4 — the unguarded bind after a bailed event set.** Native
  `0x71C99..0x71CA0` returns before `0x71D27` when `[0x157A6C] != 0`; the port's
  row-04 handler binds outside the `rc2 == FIFA96_OK` guard
  (`handlers.c:2777-2788`). Latent (`cam->event_suspended` is never set today —
  `camera.c:43`, `handlers.c:4178` only clear it). Owner:
  `fifa96_match_action_04` + `0x71C99`/`0x157A6C` (writers `0x6E5F7`/`0x6E602`
  `FUN_0006E598`, `0x73E7B`, `0x7741A`).
* **FU316-L5 — the scanner's side source.** Native `0x88B6E`/`0x88BE3` queue
  `[[0x1577CA]+0x826] ^ 1` (the *tracked* actor's team); the port uses
  `match_run_ball_side` (controlled else carrier, `match_run.c:3481-3488`,
  `:3517`, `:3530-3533`). Only observable once the armer fires. Owner:
  `fifa96_match_run_goal_scan` + `0x88940`.
* **FU316-L6 — the armed camera body.** Beyond the flags/snapshot the port drops
  the whole `FUN_0007131C` body (`0x71400..0x718A4` the announce/threshold arms,
  `0x718A9..0x71972` the mirror/tail, `0x719A0..0x71B98` the far-band pan). The
  first honest **Y** producer on the armer route is the `0x71B8A` setter with
  height `camY + 0x40`. Carried FU310-L7 / FU-71 leg 9.6; owner
  `fifa96_match_goal_arm` + the `0x7131C` body.
* **FU316-L7 — the natural pan rate (the acceptance).** The plateau's cause is
  measured, but the *natural* frequency of cone crossings is not: the natural
  probe is 3 598 granted frames with a sampled `lane_x` of 2541 (the gate needs
  `<= 0x40`), and the witness/spike need the isolated formation to reach `0x120`.
  Whether real `make game` play produces >= 2 pans (let alone 13) is unmeasured.
  Owner: the FU317 harness (pan count, `tracked` state, seed word, place delta
  per event), not a code change.
* **FU316-L8 — the row-05 carrier camera event is unported.** The pan's
  invoke-now row-5 install (`0x7F125..0x7F133`) hands the firing record to the
  carrier body, which fires its own `FUN_00071C94` at `0x7F4E7` when
  `lane <= 0x28 && lane <= bound` (seed `word[+0x71]<<3` / the slot, height the
  timer + camY clamped `0x3C`, ramp param `camY > 0x10`). The port's
  `fifa96_action_carrier_arm` (`fifa96_action_handlers.c:439`) has no camera
  event, so the pan → row-5 → rebind half of the fusion cycle does not exist in
  the engine. Whether the native chain actually fires on the pan frame (the
  target-match gates at `0x7F414..0x7F428` and the `BX==0` bail at `0x7F40E`)
  is unmeasured. Owner: `fifa96_match_action_05`/`fifa96_action_carrier_arm` +
  `0x7F44A..0x7F4E7`; measure with FU317 (a pan followed by a second place
  delta on the same granted frame).

### 7.1 Leg dispositions carried into Stage-2

* **FU310-L9 (the pan-cumulative armer spike) — closed by this slice.** The
  plateau is three named blocks (§3), not a native cap; the model reach is
  `ceil(0x730/T(s))` pans with `T = 0x90` at the measured seed (§4.3); the
  measured ceiling stays `0x120`. Nothing here re-opens a FU310 leg.
* **The tracked lifecycle — settled.** Three writers (§2.1), eleven event sites
  (§2.2), the clear-then-bind order inside `0x71C94`, and the lasting block being
  the pan's own row-5 install (§3.2) rather than the cell. The *port* gaps are
  L3/L4/L8.
* **The pan decay — settled for the idle (Y-less) arm.** `F2 = 6`, `F4 = 0xC`
  (`timer_limit`), `F6 = 0` (`timer`), `F8 = 0xC` (`ramp_divisor`), the slow-path
  `k = 0x18`/`0x20` scale, the measured `5 -> 3 -> 2 -> 1 -> 0`, and the freeze at
  `speed == 0`. The *height* arm's ramp (`F2/F4/F6` from `FUN_000702F8`) is
  carried from FU-152/FU-148 and is not re-measured here; its one port gap is the
  landing pan-step trigger (L2).

---

## 8. GO / NO-GO

**GO — the camera walk (the pan is cumulative and the re-arm is event-driven).**
The exact natural chain, every link first-hand this session or measured:

```
phase 2 (kickoff -> sit 0x0B -> phase 2)
  -> a driven record in the half-line cone:
       lane <= 0x40 - timer81  (0x7ED89..0x7ED9E)
       lane <= bound           (0x7EDCE..0x7EDD2)
       camY <= 0x50            (0x7EDD8..0x7EDDF)
       |folded angle| <= 0xAB  (0x7EE4B..0x7EE7D)
       EBP != [0x1577CA]       (0x7EFEE)
  -> the seed arm (slot dir << 6 | vel_int << 2 | type dir << 5)
  -> FUN_00071C94: 0x700F4 clears [0x1577CA] and the triple; 0x70544 seeds vel
  -> 0x71D27 binds tracked = rec; the tail 0x7F125 installs row 5 on rec
  -> next granted frame: FUN_000736AC integrates vel*delta; place += T(s)
     (measured T(0x40) = 0x90 with the 6/7/7/7 decay 5->3->2->1)
  -> FUN_000709D0 re-ramps k=0x18/0x20 per phase; vel -> 0; speed 0; the place freezes
  -> a claim/receive (0x7A219) rebinds [0x1577CA] -> the previous record is free again
  -> or the pan's own row 5 runs the carrier event (0x7F4E7, lane <= 0x28) and rebinds
  -> pan 2 -> 0x120 (measured)
  -> N same-direction pans with no 0x700F4 place writer:
       |x| > 0x6C0 at 12 pans (the boundary handler's first call)
       |x| > 0x730 at 13 pans (the armer latches)
  -> the scanner's armed gate opens; |snap z| ~ 0 -> situation 2 (throw-in),
     side = [[0x1577CA]+0x826] ^ 1
  -> the throw-in place (0x8596B) re-places/clears the camera: the arming is transient
```

Cost: **zero new camera code** for the walk. The Stage-2 work is the claim
producer (FU315's verdict) plus the FU316-L3/L4 port-fidelity fixes, the L8
row-05 rebinder (if the fusion needs it), and the FU317 measurement.

**NO-GO — a natural armer reach in phase-31's acceptance window.** Every measured
window ceilings at **2 pans / `0x120`** (the witness and all seven spike drives;
the natural probe pans 0 times over 3 598 granted frames with the sampled
`lane_x` at 2541). The model reach is 13
same-direction pans at the measured seed (fewer for larger no-slot seeds, §4.3 /
L1) with **no
place writer in between**, i.e. an uninterrupted one-directional open-play
stretch no measurement has produced. Arming also does not by itself produce a
set-piece place — it produces a situation-2 throw-in whose own cut re-places the
camera, so the armer is a transient, not a camera lock. The phase-31 Layer-1
target should be `pan_count >= 2` / `place != (0,0,0)` over the natural route
(spec §5), with `goal_armed` recorded as a *consequence*, never asserted.

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, image base
  0, `.image` overlay, 2708 functions.
* `get_xrefs_to` — `0x1577CA` (88 refs; 3 WRITE: `0x70054`/`0x70258`/`0x71D27`),
  `0x1577CE` (3: `0x70059`/`0x71D1D`/`0x88AAC`), `0x71C94` (11 callers),
  `0x700F4` (29), `0x6FFC0` (2: `0x73D9B`/`0x88883`), `0x736AC` (2:
  `0x4B193`/`0x743EF`), `0x7131C` (1: `0x73B9B`), `0x157A6C` (6).
* `get_function_by_address` — `0x70054` -> `FUN_0006FFC0` (body
  `0x6FFC0..0x70072`), `0x70258` -> `FUN_000700F4` (`0x700F4..0x702F6`),
  `0x7ED89` -> *no function* (a raw action-stage block).
* `disassemble_bytes` — `0x7ED80..0x7F12F` (the full row-04 block),
  `0x7F0D6..0x7F150` (the tail installs), `0x71C94..0x71E1F` (the setter),
  `0x70040..0x700F3` + `0x70230..0x7028F` (the two tracked clears),
  `0x73790..0x7380F` (the integrator head + the landing disjunct),
  `0x739A0..0x739EF` (the follow gates), `0x71390..0x713CF` (the armer gate),
  `0x88B50..0x88BFF` (the scanner's tracked-side arms),
  `0x7A1E0..0x7A23F` (the receive setter), `0x7A084..0x7A0DF` (the receive head),
  `0x71C99..0x71CAF` (the suspension bail), `0x7F3E0..0x7F451` + `0x7F450..0x7F520`
  (the row-05 carrier camera event).
* `decompile_function` — `0x736AC`, `0x7131C` (the armer), `0x709D0`, `0x70544`,
  `0x700F4` (incl. the `0x157819/1A/1B` table reads).
* Measurement (read-only, the existing binary): `./build/test_engine_m2` — the
  natural probe and the P6 pan witness lines quoted in §6.2.

Engine/loader source read (read-only): `src/fifa96_loader/fifa96_camera.c`
(74-151, 170-191, 510-535, 732-830, 900-991), `fifa96_outfield.c` (610-721),
`src/fifa96_engine/fifa96_match_run.c` (466-470, 570-626, 1498-1506,
3198-3234, 3481-3564, 3967-3993, 4400-4445), `fifa96_match_handlers.c`
(1330-1430, 2605-2819), `fifa96_action_handlers.c:439`
(`fifa96_action_carrier_arm`, the missing row-05 event), `fifa96_match_entities.c`
(183-239, 679-690);
`include/fifa96_loader/fifa96_outfield.h` (222-229);
`tests/test_engine_m2.c` (2318-2341, 3623-3730).

Carried docs: FU310 (§2 row I, §3.1, §6, the T6 spike + its two errata),
FU313 (§5.3 P6 partial), FU148 (§12.2-§12.4, §13.1), FU71 (§2 rows 10-26,
§14 leg 6), FU171-L2, FU305/FU309 (the harness), `docs/ENGINE.md` (the 60/80
census line).

Status: recon complete; **this draft is the only file written**; no census
change (60/80); no golden touched; no git.

## Review erratum (wave review, 2026-10-22)

**Quote (§3.2):** "`0x7F125` is reached **unconditionally** ... so *any* record that passes the half-line gate ... leaves row 04 with row 5 installed." **Fresh:** false for the row-6-on-self arm. `0x7F106 JLE 0x7F113`; the `> lane` arm at `0x7F108..0x7F111` (`EDX=6`, `ECX=0`, `JMP 0x7F131`) installs row 6 on self and skips `0x7F125`. The measured witness took row 5 (`code 5` at f=1513; `review_rec2`), so the verdict stands, but the re-arm model must not assume pan→row-5. Stage-2: count the row-6 diversion; FU316-L8's "whether the chain fires" is a gate question, not a guess.

**Quote (§7.1):** "**FU310-L9 ... — closed by this slice.**" **Fresh:** L9 was closed by the phase-30 T6 measurement (`docs/ENGINE.md:2582-2599`: ceiling `0x120` in seven drives; "no producer and no new leg"). This slice supplies the **mechanism** (three blocks) and the `T(s)` model, not the closure. **FU316-L1 (no-slot seed)** is a *new* leg (the `0x7F056` arm / per-pan travel), not a re-open: the phase-30 residual already named the ≥13-turn Stage-3/4 question. Stage-2: cite the phase-30 closure; keep L1 out of the phase-31 acceptance (Stage-3/4 probe).

**Quote (§5.1):** "with the reflect bit **on**, the mirror inside `0x7131C` fires at `|x| > 0x720` ... **a reflect-bit config blocks the pan route to the armer**." **Fresh:** the native condition is exact (`0x718B0 > 0xB10 || 0x718BE > 0x720`, then `0x718D0/0x718D6` OR, `0x718F5 AND AL,1`; the mirror returns via `0x71996`), and the 13th `0x90` pan at the measured seed does enter `(0x720,0x730)` (steps ≤10), so a set bit blocks the *armer*, not the pan. The measured run has `input_bit0=0` (`review_rec2`; `match_run.c:468`, `fifa96_input_range_words:96`), and the FU317 witness max `|x|=0x90 < 0x6C1`, so the bit is irrelevant to the pan. **Port gap:** `fifa96_match_goal_arm` only calls `fifa96_camera_reflect` when `goal_armed` is already set (`match_run.c:3200-3212`); the native mirror is also reached on the unarmed armer-gate fail path (`0x713C0 JLE 0x718A9`), and that arm is not modeled, so the port would arm where the native mirrors. Stage-2: no armer work; if a later phase targets it, port the unarmed mirror + test both bit values (**new leg FU316-L9**).

**Quote (§7.1):** "the pan decay — settled ... `5 -> 3 -> 2 -> 1 -> 0`." **Fresh:** verified on the FU317 trace: vel `5` f=1514..1519 (6 frames), `3` 1520..1526 (7), `2` 1527..1533 (7), `1` 1534..1540 (7); `T=0x90`; pan counter `0→4`. The `T(s)` table and the 13-pan x / 20-pan z arithmetic are internally consistent (144·12=`0x6C0`; 144·19=`0xAB0`; strict `>0x6C0`/`>0xAB0` at `0x73B7B`/`0x73B94`). The armer NO-GO stands.

**Stage-2 consequence:** the walk model is GO; the armer is NO-GO; the row-5 install is conditional (row-6 diversion); L1 is a new Stage-3/4 leg; L7 re-scopes to re-arm/second pan (FU317 measured 1 pan driven, 0 undriven).

## Review erratum (phase-34 wave, 2026-10-23)

**Quote (§4.3 table; §7-L1; §8):** "`s = 0x80` -> `T = 0x174`" / "admits larger
seeds and therefore longer pans" / "(fewer for larger no-slot seeds, §4.3 /
L1)". **Corrected (FU326, fresh read of `/FIFA96.EXE`):** the seed words are
the stride-velocity table at `0x10F680` — 1 280 words, fresh **min −15 / max
+15** — so the no-slot seed is **≤ 0x3C**, `v0 = trunc(w/3) ∈ [−5,5]`, and
**`T ≤ 0x90`** for the locomotion writers. The `0x174` row is arithmetic-only
(a hypothetical `v0 = 10` beyond the table); the "fewer pans for larger
no-slot seeds" reading **inverts** to a floor (13 x / 20 z — FU321 §3.5
already carried the `|w| ≤ 15` bound, so this reconciles the two frozen docs).
The 87/51 **non-locomotion** `+0x73/+0x75` writers are the only formal escape
(FU326-L1, open); the measured tape's `|w| ≤ 1` on every record-frame (FU327),
and the natural no-slot witness is **NO-GO** because the native placement
timer `+0x79` has no port producer (FU328-L1 — the silent bind's root cause).
The §4.3 `T(s)` arithmetic, the decay `5→3→2→1→0` and the 13/20-pan reach
stand unchanged.
