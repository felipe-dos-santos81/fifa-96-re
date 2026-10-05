# FU-49 — SFX event registry and the 26-slot settings table

Date: 2026-10-05. Program `/fifa96_le.bin` (flat LE link image, base 0).
Scope: the SFX **event registry** (register/play/stop/volume/param), the
two-event ambience state machine that drives it, and the 26-slot **settings**
table with its volume sliders. Companion slices: FU-43 (`.BNK` register /
select / arm), FU-47 (voice alloc, RNG, sound-state gate), FU-48 (audio edge
cases), FU-41 (bank loader), FU-37 (clock/pacing). Every claim cites an
instruction address or an image read of the flat link image.

## 0. Address frame and static storage

Code addresses are flat image addresses (Ghidra listing). Data operands are
stored object-relative and relocated by the LE loader (FU-41 §0, FU-4 §0):
object 1 (code/constants, base `0x010000`) operands land at image + `0x10000`
(e.g. the name-pointer array operand `0x9f78` -> image `0x109f78`, the
sound-byte table operand `0xa050` -> image `0x10a050`); object 4 (data/stack,
base `0x100000`) operands land at image + `0x100000` (the settings values
operand `0x49278` -> image `0x149278`, the descriptor table operand `0x5344`
-> image `0x105344`, the event arrays `0x55d60`/`0x55dbc`/`0x55e18` -> images
`0x155d60`/`0x155dbc`/`0x155e18`). All byte reads below use the object-4
image address; instruction operands are quoted as stored.

Event storage (object 4, all zero statically except the handle array):

| stored | image | size | role |
| --- | --- | --- | --- |
| `0x55d60` | `0x155d60` | 23 dwords | event -> id (the array entry the registrar loop reads) |
| `0x55dbc` | `0x155dbc` | 23 dwords | event -> bank buffer (allocation flag) |
| `0x55e18` | `0x155e18` | 23 dwords | event -> bank slot returned by `FUN_000a75aa` |
| `0xa0cc` | `0x10a0cc` | 23 dwords | event -> playback handle; **static image = -1 x8 then 0** |
| `0xa128` | `0x11a128` | dword | registered event count (`FUN_000653d0`) |

The 23-slot size is fixed by the array spacing: `0x55dbc - 0x55d60 = 0x5C`
and `0x55e18 - 0x55dbc = 0x5C`, i.e. 23 dwords each, matching the registrar
loop's id bound `0x17` (`CMP EDI,0x17` at `0x65398`). The handle array's
first eight dwords are statically `0xFFFFFFFF` in the flat image
(`read 0x10a0cc`: `ff ff ff ff ff ff ff ff 00 ...`), the remaining fifteen
are zero; the port reproduces that initial state.

Static side tables (object 1 unless noted):

* **Name pointers** `0x9f78` -> image `0x109f78`: dword pointers to 16-byte
  records at image `0x102514`, `0x102524`, ... (`CHN_ARG0.BNK`,
  `CHN_ARG1.BNK`, `CHN_BRA0.BNK`, ...; read `0x102514`). Used only by the
  registrar (`0x65313`) for `FUN_00065118`'s name copy. The table is static
  in the link image (FU-43 §3.1); it is object-4 data, so its entries are
  relocated at load time.
* **Sound bytes** `0xa050` -> image `0x10a050`: 23 bytes
  `{41,40,33,42,39,43,45,46,47,34,35,36,37,38,48,49,50,51,52,55,56,57,58}`
  followed by `0x00`. `FUN_00065544` loads `[[0x55d60+event*4]+0xa050] &
  0xff` (`0x65588..0x65594`) and passes it to `FUN_000a7705`; the registered
  id is the index into this table, and the table byte is the global SFX id
  armed in the event's bank.
* **Settings values** `0x49278` -> image `0x149278`: 26 dwords, all zero
  (`read 0x149278`). `FUN_0001de94` zeroes `0x68` bytes here.
* **Settings descriptors** `0x5344` -> image `0x105344`: 26 x 9 bytes,
  **static** (see §2).

## 1. SFX event registry

### 1.1 Registrar `FUN_000652f0` (`0x652F0..0x6536B`)

EAX = event, EDX = id:

```
0x652F9  MOV ECX,EAX
0x652FB  LEA EBX,[EAX*4]
0x65302  CMP [EBX+0x55DBC],0
0x65309  JNZ 0x65362                 ; already allocated -> return
0x6530D  MOV [EBX+0x55D60],EDX       ; id
0x65313  MOV EDX,[EDX*4+0x9F78]      ; name pointer
0x6531A  CALL 0x65118                ; EAX=ESP buffer, copy/log the name
0x6531F  MOV EDX,0x220
0x65324  MOV EAX,ESP
0x65326  CALL 0x68F74                ; allocate 0x220
0x6532B  MOV [EBX+0x55DBC],EAX
0x65331  TEST EAX,EAX
0x65333  JNZ 0x65344
0x65335  MOV EAX,ESP
0x65337  XOR EDX,EDX
0x65339  CALL 0x68F74                ; retry with size 0
0x6533E  MOV [EBX+0x55DBC],EAX
0x65344  LEA EBX,[ECX*4]
0x6534B  MOV EDI,[EBX+0x55DBC]
0x65351  TEST EDI,EDI
0x65353  JZ 0x65362
0x65355  MOV EAX,EDI
0x65357  CALL 0xA75AA                ; register the buffer as a bank (FU-43 §3.1)
0x6535C  MOV [EBX+0x55E18],EAX       ; bank slot
```

The bank half is FU-43 §3.1 (the `0x220` allocation, the name copy via
`FUN_00065118`, and `FUN_000a75aa` registering the buffer). The registry half
is: allocated flag at `[0x55dbc+event*4]`, id at `[0x55d60+event*4]`, bank
slot at `[0x55e18+event*4]`. `FUN_000652f0` never touches the handle array.

### 1.2 Registrar loop (`0x6536C`, no direct xrefs)

Entry `0x6536C` is not a Ghidra function and has **no xrefs**
(`get_xrefs_to 0x6536c` = 0), so it is reached through a runtime pointer
table, as the brief states. EAX = caller id-array pointer, EDX = count:

```
0x65370  MOV EBX,[0xA128]            ; saved for the disabled path
0x65376  MOV ECX,EAX                 ; id array
0x65378  MOV ESI,EDX                 ; count
0x6537A  CMP [0x55CE0],0
0x65381  JZ 0x653C2                  ; disabled: [0xA128] = old value (no-op)
0x65383  XOR EBX,EBX                 ; i = 0
0x65385  TEST ESI,ESI
0x65387  JLE 0x653B8                 ; count <= 0 -> set armed, count = 0
0x65389  ...  loop over i:
0x65392  MOV EDI,[EAX]               ; id = arr[i]
0x65394  TEST EDI,EDI
0x65396  JGE 0x6539D                 ; id >= 0 -> register
0x65398  CMP EDI,0x17
0x6539B  JGE 0x653B3                 ; id >= 0x17 -> skip
0x6539D  MOV EAX,EBX                 ; event = i
0x6539F  MOV EDX,[ECX+EBX*4]         ; id
0x653A2  MOV [0xA128],EBX            ; count = i while running
0x653A8  CALL 0x652F0
0x653AD  MOV EBX,[0xA128]
0x653B3  INC EBX; CMP EBX,ESI; JL loop
0x653B8  MOV [0x55D34],1             ; armed
0x653C2  MOV [0xA128],EBX            ; count
```

**Literal negative-id behavior.** The `TEST EDI,EDI / JGE register` at
`0x65394..0x65396` sends non-negative ids to the registrar, but a negative id
falls through to `CMP EDI,0x17 / JGE skip` (`0x65398..0x6539B`), a *signed*
compare: every negative id is `< 0x17`, so it **is registered too**. The
guard is only effective for `id >= 0x17`. The port reproduces this literal
behavior.

### 1.3 Play `FUN_00065544` (`0x65544..0x655A8`)

EAX = event, returns void. Gates (`0x65546..0x65577`):

```
[0x55CE0] != 0  &&  [0x55D34] != 0  &&  event < [0xA128]
&& [0x55D04] > 0  &&  [0x55DBC + event*4] != 0
```

Then:

```
0x65579  MOV EAX,[EBX+0xA0CC]        ; old handle
0x6557F  CALL 0xA6E1F                ; ready/gate on the old handle
0x65584  TEST EAX,EAX
0x65586  JZ  0x655A6                 ; 0 -> skip
0x65588  MOV EAX,[EBX+0x55D60]       ; id
0x6558E  MOV AL,[EAX+0xA050]         ; sound byte
0x65594  AND EAX,0xFF
0x65599  XOR EDX,EDX
0x6559B  CALL 0xA7705                ; arm/play: FUN_000a7705(id, 0)
0x655A0  MOV [EBX+0xA0CC],EAX        ; store the raw result (handle or error)
```

`FUN_000a7705` (`0xA7705..0xA771A`) is the FU-43 id wrapper: `ECX=0x40`,
`EDX=-1`, then `FUN_000a7728(id, ...)`. The play call stores **whatever** the
arm returns, including negative errors; the handle array therefore encodes
"playing" as `> 0`, "never played / not playing" as `<= 0`, and "stopped" as
`-1`.

### 1.4 Volume `FUN_00065488` (`0x65488..0x654C9`)

EAX = event, EDX = volume. Gates are **only** `[0x55CE0] != 0 &&
[0x55D34] != 0` (`0x65489..0x65499`); there is no `event < [0xa128]` and no
`[0x55d04] > 0` check. Clamp `EDX` to `[0, 0x7F]` (`0x6549B..0x654A6`), then
`[0x55dbc+event*4] != 0 && [0xa0cc+event*4] > 0` (`0x654AB..0x654BF`), then
`FUN_000a6bb3(handle, clamped)` with the handle in EAX (`0x654C1..0x654C3`).

### 1.5 Set-param `FUN_00065510` (`0x65510..0x65541`)

EAX = event, EDX = param. Gates: `[0x55CE0] != 0 && [0x55D34] != 0 &&
[0x55dbc+event*4] != 0 && [0xa0cc+event*4] > 0` (`0x65511..0x65537`); no
bound and no `[0x55d04]` check. `FUN_000a6aa6(handle, param)` with the
handle in EAX (`0x65539..0x6553B`); no clamp.

### 1.6 Stop `FUN_000653d8` (`0x653D8..0x65447`)

EAX = event, EDX = param. Gates are only `[0x55CE0] != 0 && [0x55D34] != 0`
(`0x653E0..0x653F0`). Clamp `EBX = param` to `[0, 0x7F]`
(`0x653F2..0x653FD`). If `[0x55dbc+event*4] != 0 && [0xa0cc+event*4] > 0`:

```
0x6541C  MOV EAX,[ECX+0xA0CC]        ; handle
0x6541E  MOV EDX,EBX                 ; clamped param
0x65420  CALL 0xA6B21                ; FUN_000a6b21(handle, param)
0x65425  TEST EBX,EBX
0x65427  JG  0x65434
0x65429  MOV EAX,[ECX+0xA0CC]
0x6542F  CALL 0xA6CDC                ; FUN_000a6cdc(handle)
```

Then, **outside** that block but inside the two global gates:

```
0x65434  TEST EBX,EBX
0x65436  JG  0x65443
0x65438  MOV [ESI*4+0xA0CC],0xFFFFFFFF   ; handle = -1
```

So `param < 1` sets the handle to `-1` even when the slot was never
allocated or had a non-positive handle.

### 1.7 Stop-all `FUN_000655AC` (`0x655AC..0x655FE`)

**No argument.** Gates `[0x55CE0] != 0 && [0x55D34] != 0`, then it walks
`i = 0 .. [0xA128)-1` and for every slot with `[0x55dbc+i*4] != 0 &&
[0xa0cc+i*4] > 0` calls `FUN_000a6cdc(handle)` (`0x655D1..0x655F7`). It does
not set the handles to `-1` and does not use the incoming EAX.

### 1.8 Stop-range `0x65448..0x65486`

An unnamed entry (`0x65448`, no Ghidra function) with the same two global
gates walks `i = 0 .. [0xA128)-1` and calls `FUN_000653d8(event=i,
param=ECX)` (`0x6546D..0x6547F`); i.e. "stop all events with a param".

### 1.9 The `0xA6xxx` wrappers (resolved)

Disassembly, not the decompiler (all six decompile to a single range check
because the decompiler loses the bodies):

| wrapper | body | semantics |
| --- | --- | --- |
| `FUN_000a6e1f` `0xA6E1F` | handle in EAX; `handle < 0 \|\| handle >= 0x10` -> `-0xB`; `[0x15fc8]` not in 1..5 -> `-4`; else `MOVZX EAX,byte [handle*0x28+0x619AA]`; `==1` -> 0 else 1 | play-ready gate on the old handle |
| `FUN_000a7705` `0xA7705` | `ECX=0x40; EBX=EDX; EDX=-1; CALL 0xA7728` | FU-43 id arm wrapper |
| `FUN_000a6bb3` `0xA6BB3` | voice/handle range + state checks, then the per-voice volume path | set volume |
| `FUN_000a6aa6` `0xA6AA6` | voice/handle range + state checks, then the per-voice parameter path | set parameter |
| `FUN_000a6b21` `0xA6B21` | voice/handle range + state checks, then the per-voice stop path | stop with param |
| `FUN_000a6cdc` `0xA6CDC` | voice/handle range + state checks, then the per-voice release path | release/panic |

`FUN_000a6e1f` is fully decoded above; the other five share the
`TEST EAX,EAX / JL` + `CMP EAX,0x10 / JL` prologue and dispatch into the
per-voice record at `[handle*0x28 + 0x619AA]`, but their inner bodies are
outside this slice (open leg §4).

### 1.10 Two-event ambience state machine

Memory (object 4): timer `[0x5a980 + 4i]`, state `[0x5a990 + 4i]` for
`i in {0,1}`; per-slot ramp record `[0x5ac20 + 0xc*i]`; ambience id pair
`[0x5b42e + 2i]` whose high word is the SFX id for slot `i`; tick counter
`[0x5b438]`. `FUN_00091bc4` (`0x91BC4`, called from `FUN_0004a228` at
`0x4A28B`) initializes `state[0]=state[1]=1`, `timer[0]=timer[1]=-1`,
`[0x5b430]=0`, `[0x5b432]=1` (`0x91D64..0x91D88`: `MOV [0x5a994],1`;
`MOV [0x5a990],1`; `MOV [0x5a984],-1`; `MOV [0x5a980],-1`;
`MOV word [0x5b430],0`; `MOV word [0x5b432],1`), so slot 0 starts bound to
event 0 and slot 1 to event 1.

`FUN_00091f60` (`0x91F60`, called from `FUN_00091dd8` at `0x91DEF`), once
per tick with `t = FUN_000cb2a4()` (`[0x12e88]`, the FU-37/FU-48 100 Hz
counter), for `i = 0,1`:

* state 0 (`[0x5a990+4i] == 0`):
  * if `i == 1` and `FUN_000653d0()` (returns `[0xa128]`) `> 2`
    (`0x91F90..0x91F9D`): draw `r = FUN_000cbc4c()`, then
    `[0x5b432] = (uint16)(1 + r % (count-1))` (`0x91FAC..0x91FB7`) — slot 1
    rotates through events `1..count-1`; slot 0 stays on its current id.
  * if `t >= timer[i]`: `FUN_0001d940(1)` reads setting 1; if nonzero
    (`0x91FD4..0x91FD6`) initialize the ramp record:
    `[0x5ac20+0xc*i] = 0xDE0` (negated for `i == 1`), `[0x5ac28+0xc*i] = 0`
    (`0x91FD8..0x91FF6`); then `FUN_000920c0(param=0x1F4, slot=i)`
    (`0x91FFC..0x92001`).
* state 1: if `t >= timer[i]`, `FUN_00092040(param=0x1F4, slot=i)`
  (`0x92008..0x9201C`).

After the loop `FUN_00091e64()` runs the volume ramp.

`FUN_000920c0` (`0x920C0..0x92191`), param in EAX, slot in EDX; returns
unless `state[slot] == 0`:

```
0x920DA  EAX = [0x5b42e + slot*2] >> 16 (SAR)
0x920E4  CALL FUN_00065544            ; play the slot's current id
0x920E9  CALL FUN_000cbc4c
0x920EE  EDX = r % 0x1c
0x920F7  ECX = 0x54 - EDX
0x920FE  EDX = (slot == 0) ? 1 : 0
0x92108  EAX = 56*EDX
0x92114  EDX = (0x54 - r%0x1c) - 56*(slot == 0)
0x92118  EAX = 0x11; CALL FUN_0001d940   ; read setting 0x11 (music volume)
0x92122  EAX = setting_0x11 * EDX
0x92125  DIV 0x64                        ; /100
0x9213D  EDX = param (0x1F4)
0x92141  CALL FUN_00065510               ; set-param(id, volume)
0x92146  CALL FUN_000cbc4c
0x92154  EDX = 0x14 + r2 % 0x14
0x9215D  EDI = EDX*60
0x92160  CALL FUN_000cb2a4
0x92167  timer[slot] = tick + EDI        ; (0x14 + r2%0x14)*60 ticks
0x92175  state[slot] = (old state == 0)  ; 0 -> 1
```

So the 0 -> 1 transition plays the event, scales the music volume slider
into the set-param value, and arms a `20..39`-second (100 Hz ticks x 60)
timer; state 1 means "ambience playing".

`FUN_00092040` (`0x92040..0x920BC`), param in EAX, slot in EDX; returns
unless `state[slot] == 1`:

```
0x9205A  ECX = [0x5b42e + slot*2] >> 16
0x92069  EDX = param
0x9206D  CALL FUN_00065510               ; set-param(id, param)
0x92072  CALL FUN_000cbc4c
0x9207E  EDX = r % 0x14
0x92080  EBP = (0x28 + r%0x14) * 60
0x9208D  CALL FUN_000cb2a4
0x9209A  timer[slot] = tick + EBP        ; 40..59 seconds
0x920A0  state[slot] = (old state == 0)  ; 1 -> 0
```

The match call sites pass `param = 0xC8` (200); the state machine passes
`0x1F4` (500) in both directions.

`FUN_00091e64` (`0x91E64..0x91F5D`) runs once per `FUN_00091f60` after both
slots; it increments `[0x5b438]` (`0x91E6D..0x91E78`), reads a tick with
`FUN_00051074`/`FUN_00051068` (`0x91E7E..0x91E88`), and for each slot with
`state == 1`:

```
0x91EAB  CALL FUN_000795b4(ESP, &record[0xc*i])   ; fill a 16-bit pair
0x91EC2  CALL FUN_000a2a10(...)
0x91ECC  v = (v - tick) & 0xffff
0x91ED2  CALL FUN_000a60a0(v)
0x91EDA  EDX:EAX = v * 0x37; += 0x8000; SHRD 16
0x91EED  s16 = -(short)EAX
0x91F00  if (abs(s16) > 0x38) FUN_0009bfd0(&DAT_0x2d68, s16)   ; diagnostic
0x91F16  s16 += 0x3f
0x91F1C  if (s16 > 0x75) s16 = 0x75
0x91F28  else if (s16 < 10) s16 = 10
0x91F32  EAX = [0x5b42e + i*2] >> 16
0x91F42  CALL FUN_00065488(id, s16)
```

So the ambience volume is `clamp(s16 + 0x3F, 10, 0x75)` where `s16` is the
negated high word of `0x37 * FUN_000a60a0(...)`; the port leaves the ramp
math to a later slice (open leg §4).

### 1.11 Call sites

* `FUN_00072478` `0x7253B/0x7254A` and `0x725B5/0x725C4`: four
  `FUN_00092040` calls, `(param=0xC8, event=0)` then `(param=0xC8, event=1)`
  in two branches; followed by `FUN_000651f0(4)`.
* `FUN_00088940` `0x88B02/0x88B11`: same `(0xC8, 0)` / `(0xC8, 1)` pair.
* `0x81C0E/0x81C1D` and `0x8933C/0x8934B`: two more identical pairs in code
  regions Ghidra did not promote to functions (raw disassembly), same
  `MOV EAX,0xC8 / XOR EDX,EDX / CALL` then `MOV EDX,1 / MOV EAX,0xC8 / CALL`
  shape.
* `FUN_00091f60` `0x9201C`: `(0x1F4, slot)`.
* `FUN_000920C0` `0x92001` only caller.
* `FUN_00065544` `0x920E4` only caller: play is reachable only through the
  ambience state machine.

## 2. Settings subsystem

### 2.1 Values `0x49278`

26 dwords, zero statically (image `0x149278`), zeroed by `FUN_0001de94`
(`0x1DE97..0x1DEA3`: `MOV ECX,0x68; MOV EDI,0x49278; REP STOSB`).

### 2.2 Descriptors `0x5344` (static, not runtime-loaded)

The brief calls this table runtime-loaded; the image says otherwise. Image
`0x105344` holds 26 records of 9 bytes:

```
+0  uint32 opts;    /* object-4 pointer to an option-value dword array */
+4  uint8  max;     /* option count; 0 = unlimited */
+5  uint32 label;   /* object-4 pointer to a label/format string */
```

The max field is read by `MOV EBX,[EDX+EAX*8+0x5345]; SAR EBX,0x18` at
`0x1DADC..0x1DAE3` (EAX = EDX = setting), i.e. a **dword load at
descriptor+1 whose top byte is descriptor+4**. The brief's "max = byte at
`[0x5345 + setting*9]` ... max field at +1" conflates the load address with
the field offset: the max byte is at `0x5348 + 9*setting`, descriptor +4.
Cross-check with the retail table: settings `0x11/0x12/0x13` have max 100
and defaults 91/93/98 (`FUN_0001de94`), and the other non-zero maxes
(2/3/5/6/7) bound the defaults (`0:1<2`, `8:2<3`, `0xC:4<5`, ...). Retail
maxes, settings `0..0x19`:
`2,2,2,6,2,2,2,3,3,2,3,2,5,2,7,2,2,100,100,100,0,2,0,0,0,0`.
`FUN_0001da48` (`0x1DA48`) is the max getter (same load/shift), and
`FUN_0001d95c` tests `load32(0x5344+9*setting) != 0` (`0x1D9F0`), the
`opts` pointer.

### 2.3 Set / cycle / decrement `FUN_0001da58` (`0x1DA58..0x1DC33`)

EAX = setting, EDX = value. Reads `max = (int8)(load32(0x5345+9*setting) >>
24)` into EBX (`0x1DAD8..0x1DAE3`).

* **setting 0 rate limit** (`0x1DA61..0x1DA95`): only when EAX == 0, read
  `t = FUN_000cb2a4()`, `elapsed = t - [0x5444]`; if `elapsed/100 < 1` and
  `[0x5444] >= 0`, return `FUN_0001d95c(0)` without mutating; else
  `[0x5444] = t`.
* **setting 0x15 guard** (`0x1DA9A..0x1DAD6`): `FUN_00015444(2)`; when it is
  zero and `FUN_00015444(3)` is zero (or `FUN_00015444(4)` is nonzero), the
  value is forced to 0.
* **volume sliders 0x11/0x12/0x13 with value < 0** (`0x1DAF9..0x1DBAC`):
  * `value == -1`: `cur -= 5; if (cur < 0) cur = 0`.
  * any other negative value (including `-2`): `cur += 5; if (max <= cur)
    cur = max - 1`. There is **no cycle** for sliders.
  * then draw the UI: `percent = 100*cur / (max-1)`, slider index 0/1/2, and
    `FUN_00099e9f(0x4935c + index*10, 0x9b0, percent, 0x25)`; the function
    returns early with the UI address.
* **non-slider `value == -1`** (`0x1DBBB..0x1DBDC`): `cur == 0` -> `cur =
  max - 1`; else `cur -= 1`.
* **non-slider `value == -2`** (`0x1DBDE..0x1DBF7`): `cur = (cur + 1) % max`
  (signed `IDIV`; `max == 0` is a divide fault in the original).
* **store** (`0x1DBF9..0x1DC05`): store when `(value >= 0 && value < max) ||
  max == 0`; otherwise ignore.
* **setting 0x15 post** (`0x1DC0C..0x1DC22`): `cur == 0` -> `FUN_00015474()`,
  else `FUN_000154b0()`.
* return `FUN_0001d95c(setting)` (`0x1DC27..0x1DC29`).

### 2.4 Getters / UI

* `FUN_0001d940` (`0x1D940`): `return [0x49278 + setting*4]` (raw value).
* `FUN_0001da48` (`0x1DA48`): max getter (see §2.2).
* `FUN_0001d95c` (`0x1D95C`): for sliders, `FUN_0001da48(setting)` gives max,
  the value is multiplied by 100 (`IMUL` chain `0x1D97C..0x1D98F` and
  `0x1D997..0x1D99C`), the slider UI is updated through `FUN_00099e9f`, and
  the function returns the UI address; for non-sliders with a nonzero `opts`
  pointer it returns `FUN_0001771c()`'s value, otherwise 0.

### 2.5 Defaults

`FUN_0001ddc8` (`0x1DDC8..0x1DE90`), 15 calls to `FUN_0001da58(setting,
value)`:

| setting | 0 | 1 | 2 | 4 | 6 | 7 | 8 | 9 | 0xA | 0xB | 0xC | 0xD | 0xE | 0xF | 0x10 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| value | 1 | 1 | 1 | 0 | 1 | 0 | 2 | 0 | 2 | 1 | 4 | 0 | 0 | 0 | 0 |

Settings 3 and 5 are not touched by the map.

`FUN_0001de94` (`0x1DE94..0x1DF0D`): zero `0x68` bytes at `0x49278`; call
`FUN_0001ddc8`; `[0x49284] = FUN_00068cfc(0)` (EAX = 0) and, if nonzero,
`FUN_0001da58(2, 0)`; `[0x492cc] = FUN_00068cfc(2)` (EAX = 2);
`[0x492c8] = 0`; finally `FUN_0001da58(0x11, 0x5B)`, `(0x12, 0x5D)`,
`(0x13, 0x62)`. Note `0x49284` is setting **3** and `0x492cc` is setting
**0x15** (`0x49278 + 4*3`, `0x49278 + 4*0x15`), so the resource query results
live in settings 3 and 0x15, and setting 0x14 is cleared.

### 2.6 Override loader `FUN_0001dcac` (`0x1DCAC`)

`FUN_00018bf0()` yields the resource name; `FUN_00068f30` tests presence;
`FUN_00069338` loads it; on success the 0x68-byte block is copied over
`0x49278` (`0x1DCFC..0x1DD1C`). Any failure falls back to `FUN_0001de94`.
This is the "settings override" path: the *values* come from a resource,
the descriptor table stays static.

### 2.7 Correction: `0x49278` is settings, not events

The event registry lives at `0x55d60`/`0x55dbc`/`0x55e18` (object 4) plus the
handle array `0xa0cc`; `0x49278` is the settings values block. `0x5da54` is
neither: it is a zeroed per-voice record array with stride `0x14`, walked by
`FUN_000a72e7` (`0xA72E7`, bounds `[0x5d830]`, `FUN_000a76ea` arm), i.e. the
voice/EACSNDF side (FU-43 §6.6).

## 3. Port mapping

`include/fifa96_loader/fifa96_sfx_event.h` / `src/.../fifa96_sfx_event.c`:

| port | original |
| --- | --- |
| `fifa96_sfx_event_init` | BSS zeroes + the static `-1 x8` handle prefix (`0x10a0cc`) |
| `fifa96_sfx_event_register` | `FUN_000652f0` registry half (`0x65302..0x6530D`): no-op when allocated |
| `fifa96_sfx_event_register_all` | loop `0x6536C..0x653CC`: id `< 0x17` registers (including negatives), `count`/`armed` at the end |
| `fifa96_sfx_event_play` | `FUN_00065544`: five gates, `ready(handle)`, `play(id)`, store the raw handle |
| `fifa96_sfx_event_set_volume` | `FUN_00065488`: two gates + slot + handle `> 0`, clamp `[0,0x7F]` |
| `fifa96_sfx_event_set_param` | `FUN_00065510`: two gates + slot + handle `> 0`, no clamp |
| `fifa96_sfx_event_stop` | `FUN_000653d8`: two gates, clamp `[0,0x7F]`, stop/release when `handle > 0`, `-1` when `param < 1` |
| `struct fifa96_sfx_event_backend` | `FUN_000a6e1f/7705/6bb3/6aa6/6b21/6cdc` |
| `FIFA96_SFX_EVENT_MAX` 23 | `0x55e18 - 0x55dbc` and the `0x17` id bound |

`include/fifa96_loader/fifa96_settings.h` / `src/.../fifa96_settings.c`:

| port | original |
| --- | --- |
| `FIFA96_SETTINGS_COUNT` 26 | `0x68 / 4` and the 26 descriptor records |
| `struct fifa96_settings.max` | caller-provided descriptor max (0 = unlimited, the `max == 0` store branch) |
| `fifa96_settings_init` | BSS zeroes (image `0x149278`) |
| `fifa96_settings_defaults` | `FUN_0001ddc8` map + `FUN_0001de94` sliders 0x5B/0x5D/0x62 |
| `fifa96_settings_set` | `FUN_0001da58`: `-1`/`-2` semantics, slider `+/-5`, store bounds |
| `fifa96_settings_get` | `FUN_0001d940` |

Not ported here (documented only): the setting-0 rate limit, the setting-0x15
`FUN_00015444` guard and `FUN_00015474`/`FUN_000154b0` calls, the slider UI
(`FUN_00099e9f`), the override loader `FUN_0001dcac`, and the whole ambience
state machine (its registry calls are covered by the port API).

## 4. Open legs

1. **Ramp math.** `FUN_000795b4` (record fill), `FUN_000a2a10`,
   `FUN_000a60a0`, and the `0x37` scale are only structurally decoded; the
   `0xDE0` ramp constant and the diagnostic `FUN_0009bfd0(&0x2d68, ...)` are
   not ported. Capture target: break at `0x91EAB` with state 1.
2. **`[0x55d04]` producer.** Only two accesses are recorded: the play gate
   read (`0x65560`) and the sound-init zeroing (`0x64FD2`, which clears
   `0x55d00..0x55d14`); `FUN_0006504c` copies `[0x55ce8+i]` into
   `[0x55d00+i]`. The writer that makes it positive is unresolved; the port
   models it as the caller-owned `audio` gate.
3. **`FUN_00065118` logging.** Calls `FUN_000a1764(&DAT_0xa098)` and
   `FUN_000a1956(&DAT_0x2904, name)`; the exact format strings are not
   decoded.
4. **`0xA6xxx` inner bodies.** Only `FUN_000a6e1f` is fully decoded (§1.9);
   the per-voice dispatch of the other five wrappers is outside this slice.
5. **`FUN_00068cfc`.** Device/resource query used by `FUN_0001de94` for
   settings 3/0x15; not resolved.
6. **Descriptor tail.** Image `0x10542E` follows the 26 records
   (`12 01 11 02 13 03 04 06 07 15 ...`); role unknown.
7. **Registrar-loop dispatcher.** Entry `0x6536C` has no xrefs; the runtime
   pointer table that calls it was not located.

## 5. Provenance (Ghidra, 2026-10-05, `/fifa96_le.bin`)

* Disassembled/decompiled: `FUN_000652F0`, `0x6536C` loop, `FUN_000653D0`,
  `FUN_000653D8`, `0x65448` loop, `FUN_00065488`, `FUN_00065510`,
  `FUN_00065544`, `FUN_000655AC`, `FUN_00064F70`, `FUN_0006504C`,
  `FUN_00065118`, `FUN_000651F0`, `FUN_000a6e1f`, `FUN_000a7705`,
  `FUN_000a75aa`, `FUN_00091bc4`, `FUN_00091f60`, `FUN_000920c0`,
  `FUN_00092040`, `FUN_00091e64`, `FUN_00091dd8`, `FUN_000cb2a4`,
  `FUN_0001d940`, `FUN_0001d95c`, `FUN_0001da48`, `FUN_0001da58`,
  `FUN_0001dcac`, `FUN_0001ddc8`, `FUN_0001de94`, `FUN_000a72e7`.
* Xrefs: `0x92040` (11 callers), `0x920c0` (1), `0x91f60` (1), `0x65544`
  (1), `0x653d8` (1), `0x652f0` (1), `0x6536c` (0), `0x55ce0` (47),
  `0x55d04` (2), `0x55d34` (9), `0x9f78` (1), `0xa050` (1), `0xa0cc` (1),
  `0x5da54` (1), `0x91bc4` (1), `0x91dd8` (1).
* Image reads: `0x102514` (CHN names), `0x109f78` (name pointers),
  `0x10a050` (sound bytes), `0x10a0cc` (handle prefix), `0x11a128` (count),
  `0x149278` (settings values), `0x105344` (descriptors),
  `0x155d60/0x155dbc/0x155e18` (event arrays), `0x15a980/0x15a990` (state).
* Cross-checked against the rebuilt flat image
  (`tools/fifa96_le.py /tmp/opencode/fu49/iso/FIFA96.EXE -o
  /tmp/opencode/fu49/fifa96_le.bin`, size `0x16AA50`), byte-identical at
  every cited offset.
* Companion: FU-43 §3.1/§3.2/§6.6 (bank half, `0xA6xxx` arm, event table
  open leg), FU-47 §2 (`FUN_000cbc4c`), FU-48 §6 (tick model), FU-37
  (`FUN_000cb2a4`).
