# FU-48 — audio edge cases: out-of-table pitch, the #DE step tail, the pan gain quirk, the two-voice split cap and the 100 Hz ISR tick model

Date: 2026-10-04. Program `/fifa96_le.bin` (flat LE link image, base 0;
throwaway copy `/tmp/opencode/fu48/fifa96_le.bin`, byte-identical at every
cited offset, sha256 `bba699e98b3d657e…`). Scope: close the named open legs of
FU-46 and FU-47 — out-of-table pitch behavior and the `step_int >= 256` divide
fault (FU-46 §Open legs 1/2), the `>0x7F` pan gain quirk (FU-46 §Open legs 3),
the `>0x7F` two-voice split cap and the CLI/static-only status (FU-47 §Open
legs 1/2), and the `DAT_00012E88` clock ISR model (FU-47 §Open legs 3).
Companion slices: FU-37 (mixer/timing), FU-43 (.BNK), FU-46 (calibration),
FU-47 (voice alloc/RNG/gate). Every claim cites an instruction address or an
image read; the throwaway model is `/tmp/opencode/fu48/model.py` (not
committed).

## 0. Address frame

* Code addresses are flat image addresses; data operands inside the code are
  object-4 offsets whose bytes land at image + 0x100000 (FU-41 §0). The pitch
  table dword operand `[EBX*4+0xA6B88]` at `0xB86E6` resolves to image
  **0xB6B88** (FU-46 §0), 1200 dwords ending at 0xB7E48.
* `FUN_000a6717`, `FUN_000a7728`, `FUN_000a780e` are the two-voice split,
  play lookup and arm (FU-43 §3.2, FU-47 §1.3).
* The PIT ISR is at `0x9F5E4`; its setup is `FUN_0009F754` (0x9F754); the
  clock reader block is 0xCB2A4..0xCB315. `[0x12E88]` -> image 0x112E88,
  `[0x12E8C]` -> image 0x112E8C (BSS).

## 1. Out-of-table pitch (`FUN_000b86C8` head, 0xB86C9..0xB86E6)

The head (FU-46 §1.1, re-read here) is:

```
0xB86C9  ADD EBX,0x2000              ; E = pitch + 0x2000
0xB86CF  MOV EDX,0x40d0              ; W = 0x40D0
0xB86D4  MOV CL,0x9                  ; shift = 9
0xB86D6  CMP EBX,EDX                 ; signed
0xB86D8  JGE 0xB86E4
0xB86DA  SUB EDX,0x4b0               ; W -= 1200
0xB86E0  INC CL
0xB86E2  JMP 0xB86D6
0xB86E4  SUB EBX,EDX                 ; index = E - W
0xB86E6  MOV EDX,[EBX*4+0xA6B88]     ; ratio = image[0xB6B88 + 4*index]
```

### 1.1 Index past the table (pitch > 9599): reads code bytes

`0xB86E6` has **no bounds check**: the index is `E - W` and for `pitch >= 9600`
the `E < W` loop never runs (`W = 0x40D0`, `E >= 0x4580`), so `index >= 1200`
and the load reads image bytes immediately after the table. Image read at
0xB7E40: `68 ff 01 00 b4 ff 01 00 | 55 8b ec 50 8b 45 08 a3 9c 06 04 00 …`.
`table[1199]` is at 0xB7E44 (`0x0001FFB4`); the first out-of-table dword is at
**0xB7E48 = 0x50EC8B55** (`55 8B EC 50` = `PUSH EBP; MOV EBP,ESP; PUSH EAX`,
the prologue of the code that follows). Pitch 9600 selects index 1200 ->
0x50EC8B55; pitch 10000 selects index 1600 -> image 0xB8488, and so on, i.e.
the "ratio" is arbitrary function code and can run arbitrarily far past the
table (a 32-bit `EBX*4` wrap is possible for extreme pitches). This is an
unstable image-layout artifact, not a behavior the port can reproduce from the
1200-entry data it ships. **Disposition: documented-only; the port keeps
rejecting index >= 1200 (`TRUNCATED`).** Tests pin 9599 -> table[1199] and
9600 -> rejected.

### 1.2 Shift wrap (pitch < -18000): `SHRD` masks CL to 5 bits

For negative pitches the loop always stops with `index = E - W` in 0..1199 (by
construction), but `CL` grows one per extra octave window:

| pitch | index | CL | ratio |
|---|---|---|---|
| -18000 | 0 | 31 | table[0] 0x10000 |
| -18001 | 1199 | 32 | table[1199] 0x1FFB4 |
| -19200 | 0 | 32 | table[0] 0x10000 |
| -19201 | 1199 | 33 | table[1199] 0x1FFB4 |

`SHRD EAX,EDX,CL` (`0xB86EF`) is the x86 instruction, whose count is masked to
5 bits for 32-bit operands (Intel SDM, SHRD): pitch -18001 shifts by `32 & 31
= 0`, -19201 by `33 & 31 = 1`, and so on. The intended 64-bit shift therefore
never happens for pitch < -18000; v is instead the full (or nearly full)
product, and at retail-scale rates the result is a huge quotient that reaches
the §3 second-DIV fault. Hand check at `rate 16000 -> out 22050`:
pitch -18001 -> effective shift 0, table[1199] 0x1FFB4, v = low32(16000 *
0x1FFB4) = 0x7CED7200, q = v/22050 = 95053 >= 256 -> the original's #DE;
pitch -18000 -> shift 31, v = 0, q = 0, step 0.

The port's head loop is capped at `CL = 32`, so very negative pitches
(including `INT32_MIN`) terminate quickly and are rejected (`shift > 31`);
modelling the wrap exactly would still lead to §3's fault at any realistic
output rate. **Disposition: documented-only (reject).** Tests pin -18000 ->
table[0]/shift 31, -18001 -> rejected, and the two extreme `int32_t` pitches
terminating with a reject.

## 2. Two-voice split cap and the signed read-back (`FUN_000a6717`)

### 2.1 `FUN_000a6717` (0xA6717..0xA67EF)

Caller `FUN_000a7728` passes `EAX = caller pan`, `EDX = volume`:

```
0xA7770  MOV EAX,[EBP-0x10]          ; EAX = pan
0xA7773  MOV EDX,EBX                 ; EDX = volume
0xA7775  CALL 0xA6717
```

Inside the split (`EAX` = pan, `EDI = EDX` = volume, `ESI` untouched):

* pan in 0..0x7F (`0xA6721..0xA672A` guard): `local = pan`; `ESI = pan`, and
  if `pan > 0x7A`, `ESI = 0x7F - pan` (`0xA672F..0xA673D`); if `ESI < 5`,
  `left = (ESI+5)*volume/10` (IDIV, `0xA6744..0xA6752`), else `left = volume`;
  `right = volume - left` (`0xA6758..0xA675A`).
* pan in 0x80..0xFF (`0xA6765..0xA67A3`): `local = 0xFF - pan`; the same
  `ESI` attenuation is applied to `right`, and `left = volume - right`
  (`0xA679F..0xA67A1`).
* pan < 0 or pan >= 0x100 (`0xA67A5..0xA67AE`): `local = 0x40`;
  `left = volume`; `right = 0`; **ESI is whatever the caller left** — the
  invalid branch never writes ESI, and `FUN_000a7728` set `ESI = id` at
  `0xA772E` and has not touched it since.

All paths reach the table scaling at `0xA67B0`:

```
0xA67B0  CMP ESI,0x5
0xA67B3  JGE 0xA67DD
0xA67B5  MOV EAX,[ESI*4+0x148E0]    ; image 0x1148E0 = {150,140,130,120,110}
0xA67BC  IMUL EAX,EBX ; CDQ; IDIV 0x64     ; left  = table[ESI]*left/100
0xA67C9  MOV EAX,[ESI*4+0x148E0]
0xA67D0  IMUL EAX,ECX ; CDQ; IDIV 0x64     ; right = table[ESI]*right/100
0xA67DD  MOV EAX,[EBP-0x4]                 ; local
0xA67E0  SHL EBX,0x8
0xA67E3  SHL EAX,0x10
0xA67E6  OR EAX,EBX ; OR EAX,ECX           ; packed = local<<16|left<<8|right
```

### 2.2 The cap: a >0x7F split byte is read back signed

`FUN_000a7728` unpacks with `AND 0xFF` and stores the byte value into the arm
arguments (`0xA777A..0xA77A0`):

```
0xA777C  AND EDX,0xFF              ; right  -> [EBP-0x18], arm id+1's ECX
0xA7782  SAR EAX,0x8
0xA778F  SAR [EBP-0x4],0x8         ; local  -> arm's EBX (pan)
0xA7796  AND ECX,0xFF              ; left   -> arm id's ECX (caller volume)
```

The arm stores the caller volume as a byte (`FUN_000a780e` `0xA7972 MOV
AL,[EBP-0x8]; 0xA7975 MOV [EDI+0x24],AL`) and `FUN_000b9fdd` reads it back
**signed** (`0xB9FEE MOVSX EAX,byte [EBX+0x24]`). With the retail-shaped
invalid branch (`pan = -1`, `volume = 0x7F`) and `id < 5`:

| id | `table[id]` | left = table[id]*127/100 | byte | s8 | gain = (0x7F*s8*0x7F)/0x3F01 | centre-pan L/R |
|---|---|---|---|---|---|---|
| 0 | 150 | 190 | 0xBE | -66 | 0xBE (AL of -66) | 78 / 1998 |
| 1 | 140 | 177 | 0xB1 | -79 | 0xB1 | 65 / 1989 |
| 2 | 130 | 165 | 0xA5 | -91 | 0xA5 | 53 / 1973 |
| 3 | 120 | 152 | 0x98 | -104 | 0x98 | 40 / 1964 |
| 4 | 110 | 139 | 0x8B | -117 | 0x8B | 27 / 1951 |

`FUN_000b9fdd` (`0xB9FEA..0xBA007`) is
`(s8)record+0x25 * (s8)record+0x24 * (s8)[0x15FD6] / 0x3F01` with `IDIV` and
`MOV [EBX+0x26],AL` — no clamp. With descriptor volume 0x7F and master 0x7F
the product is exactly `16129 * s8`, so the gain byte equals the split byte;
the pan conversion (§4) then sign-extends it. `right = 0` for the invalid
branch, so voice B's caller byte (and gain) is 0.

The port previously capped left/right at 0x7F (`l > 0x7F ? 0x7F : l`). FU-48
removes the cap (`(uint8_t)` of the exact 32-bit product; in-domain products
are <= 190 so the low byte is the original's packed byte), drops the
`sfx_gain` clamp, and lets `fifa96_mixer_start` accept the raw 0..0xFF gain
byte. The arm's public `opts->volume` input validation (0..0x7F) is unchanged,
so only the split outputs can exceed 0x7F. For volume <= 0x7F the valid/mirror
branches never exceed 0x7F; a brute-force over pan -1..0xFF, volume 0..0x7F,
id 0..0x7F finds `max left = 190` (invalid branch, id 0) and `max right =
127`. The `>0x7F` split byte is therefore reachable only through the
static-only two-voice descriptor path (§5) with the default pan; the port
still models it exactly.

## 3. The `step_int >= 256` divide fault (`FUN_000b86C8` tail)

```
0xB86ED  MUL EDX                     ; EDX:EAX = rate * ratio (v ignored high)
0xB86EF  SHRD EAX,EDX,CL             ; v = low32(p >> (CL&31))
0xB86F2  MOV EDI,EAX                 ; EDI = v
0xB86F4  MOV EDX,0x0
0xB86F9  DIV [0x406A0]               ; q = v / out, rem r in EDX
0xB86FF  AND EAX,0xff                ; step_int = q & 0xFF
0xB8704  MOV [ESI+0x24],EAX
0xB870E  MOV EAX,[ESI+0x24]          ; masked step_int
0xB8711  MUL [0x406A0]               ; step_int * out
0xB8717  SUB EDI,EAX                 ; rem = v - step_int*out
0xB8719  MOV EAX,EDI
0xB871B  MOV EDX,0x1000000
0xB8720  MUL EDX                     ; EDX:EAX = rem * 2^24
0xB8722  DIV [0x406A0]               ; #DE iff EDX >= out
0xB8728  SHL EAX,0x8                 ; 8.24 fraction -> 32.32 low word
```

Write `q = v / out`, `r = v mod out`. `step_int = q & 0xFF`, so
`rem = v - (q & 0xFF)*out = 256*floor(q/256)*out + r`. The second DIV's
dividend is `rem * 2^24`, whose high dword is `rem >> 8 =
floor(q/256)*out + (r >> 8)`. This is `>= out` **iff `q >= 256`**; the
instruction's `EDX >= divisor` precondition makes the DIV raise #DE for every
such input. For `q < 256`, `rem = r < out` and `rem >> 8 < out`, so the DIV is
safe and the port's 64-bit computation is exact. Hand-computed boundaries at
`out = 22050`, `ratio = 0x10000`, `shift = 16` (`v = rate`):

| rate | q | rem | step |
|---|---|---|---|
| 5622750 = 255*22050 | 255 | 0 | 0xFF00000000 |
| 5622751 | 255 | 1 | 0xFF0002F800 |
| 5644799 | 255 | 22049 | 0xFFFFFD0700 |
| 5644800 = 256*22050 | 256 | 5644800 | **#DE (original)** |

The port now returns `-(FIFA96_ERR_TRUNCATED)` when `q >= 256` instead of
FU-46 §1.3's fabricated masked step (the old synthetic 65535/100 -> 615683561728
is now rejected). This is a documented behavior change; `q < 256` results are
untouched (the `AND 0xFF` is then a no-op).

## 4. Pan gain > 0x7F quirk (`FUN_000a662c`, 0xA662C..0xA66AA)

The factor selection is unchanged (FU-46 §2.2). The scaling/packing tail is:

```
0xA6681  MOVSX ESI,byte [ESI+0x26]   ; signed gain byte
0xA6685  IMUL EAX,ESI                ; EAX = Lfac * s8(gain)
0xA6688  XOR EDX,EDX
0xA668A  MOV EBX,0x7f
0xA668F  DIV EBX                     ; ql = (uint32)(Lfac*s8(gain)) / 0x7F
0xA6691  MOV EBX,EAX
0xA6693  MOV EAX,ECX                 ; Rfac
0xA6695  IMUL EAX,ESI
0xA6698  XOR EDX,EDX
0xA669A  MOV ECX,0x7f
0xA669F  DIV ECX                     ; qr = (uint32)(Rfac*s8(gain)) / 0x7F
0xA66A1  SHL EAX,0x10
0xA66A4  OR EAX,EBX                  ; packed = (qr<<16) | ql  (mod 2^32)
```

`FUN_000a6579` extracts `L = packed & 0x7F` (`0xA6605 AND ESI,0x7f`) and
`R = packed >> 16` (`0xA6601 SHR EDI,0x10`), so exactly:

```
L = ql & 0x7F
R = ((qr & 0xFFFF) | (ql >> 16)) & 0xFFFF      ; packed >> 16
```

A retail 0..0x7F gain byte keeps both quotients <= 0x7F (unchanged); a >0x7F
byte is negative after MOVSX, and the unsigned DIV turns the negative 32-bit
product into a large quotient. Hand-computed (exact integer DIV; the model
prints the same):

| pan | gain | ql | qr | L | R |
|---|---|---|---|---|---|
| 0x40 | 0x80 (-128) | 0x02040790 | 0x02040790 | 16 | 1940 |
| 0x40 | 0xBF (-65) | 0x020407CF | 0x020407CF | 79 | 1999 |
| 0x40 | 0xFF (-1) | 0x0204080F | 0x0204080F | 15 | 2575 |
| 0x00 | 0x80 | 0x02040790 | 0 | 16 | 516 |
| 0x7F | 0x80 | 0 | 0x02040790 | 0 | 1936 |

Example: pan 0x40, gain 0x80 -> product `127 * -128 = -16256` as uint32
`0xFFFFC080`; `/127 = 33818512 = 0x02040790`; L = `0x790 & 0x7F = 0x10 = 16`;
R = `0x0790 | 0x0204 = 0x0794 = 1940`.

The port adds `fifa96_mixer_pan_gains_wide` implementing the above exactly,
widens the voice's `gain_l`/`gain_r` (L 0..0x7F, R 0..0xFFFF), and has
`fifa96_mixer_set_pan` use it. `fifa96_mixer_pan_gains` keeps its `uint8_t`
signature as a low-byte view (unchanged for the retail domain). The quirk is
reachable end-to-end with a raw gain byte >= 0x80 (the §2 two-voice chain or a
direct `fifa96_mixer_start` call with such a byte); the PCM readers apply the
wide gains with the cited `MOVSX sample * gain, SAR 7`, e.g. gain byte 0xBE at
centre gives L=78, R=1998 and renders (1000,-1000) -> (609,-15610),
(-1000,2000) -> (-610,31218).

## 5. Two-voice CLI and static-only status

* `FUN_000a7728` wraps both arm calls in an interrupt critical section:
  `0xA778D PUSHFD; 0xA778E CLI`, with `POPFD` on the first-arm failure
  (`0xA77AF`), the missing id+1 slot (`0xA77C4`) and after the second arm
  (`0xA77DD`). The port has no interrupt model, so this remains
  documented-only (FU-47 §Open legs 2); the port's equivalent guarantee is
  that an error leaves no voice armed.
* The path is static-only in retail: the new census test walks all 128 slots
  of `tests/golden/sfx_game.bnk` and asserts `(desc[0x1C] & 1) == 0` for every
  present descriptor, matching FU-43 §4 / FU-47 §6 leg 2. The port's
  two-voice harness (ids 0..4, §2) exercises the disassembly-derived path.
  The original allocates the second record inside its second `FUN_000a780e`;
  the port allocates `voice + 1` (FU-47 §6 leg 4) — failure-path rotor state
  is the only difference.

## 6. The `DAT_00012E88` clock ISR model

### 6.1 Setup (`FUN_0009F754`, 0x9F754..0x9F7CD)

```
0x9F754  MOV ECX,0x8 ; 0x9F759 MOV EAX,0x5bae4 ; 0x9F75E XOR EDX,EDX
0x9F760  CALL 0x9E907                ; zero the 8 callback slots at 0x5BAE4
0x9F765  CMP dword [0x12E90],0 ; JNZ 0x9F7CD      ; already installed
0x9F76E  CMP word [0x12E94],0  ; JNZ 0x9F7CD
0x9F778  MOV EDX,0x21 ; 0x9F77D IN AL,DX ; 0x9F780 OR AL,0x3 ; OUT DX,AL
0x9F783  MOV EAX,0x8 ; CALL 0xAE9A0              ; save the old vector
0x9F78D  MOV word [0x12E94],DX ; 0x9F794 MOV [0x12E90],EAX
0x9F799  MOV AL,0x9c ; MOV EDX,0x40 ; OUT DX,AL
0x9F7A6  MOV AL,0x2e ; MOV CX,CS ; OUT DX,AL      ; PIT ch0 divisor 0x2E9C
0x9F7B5  PUSH 0x8f6f0 ; CALL 0xCBDA0              ; install the PM entry
```

`0x9F760` is a memset-like fill (`FUN_0009e907`: fill ECX dwords with EDX),
i.e. the eight 32-bit callback slots at 0x5BAE4 start zero; the ISR's
per-tick loop calls the nonzero ones. `FUN_0009F64C` inserts a callback into
the first free slot and `FUN_0009F684` clears it. `[0x12E90]/[0x12E94]` hold
the old vector the ISR chains on 20 Hz boundaries (`CALLF [0x12E90]` at
0x9F619). The PIT divisor `0x9C`/`0x2E` = 0x2E9C gives
`1193182 / 11932 = 100.0 Hz`.

### 6.2 ISR (`0x9F5E4..0x9F64B`)

```
0x9F5E4  PUSHAD; PUSH DS; PUSH ES; PUSH FS; PUSH GS; MOV EBP,ESP; CLD
0x9F5EE  CALL 0x9FFDC                ; MOV DS,CS:[0x8FFE5]; RET
0x9F5F3  STI
0x9F5F5  MOV EDX,[0x12E88]
0x9F5FB  INC EDX
0x9F5FC  MOV EBX,5
0x9F601  MOV [0x12E88],EDX           ; tick++ (32-bit wrap)
0x9F607  MOV EAX,EDX
0x9F609  SAR EDX,0x1F
0x9F60C  IDIV EBX                    ; EDX = tick % 5 (signed remainder)
0x9F60E  TEST EDX,EDX
0x9F610  JNZ 0x9F621                 ; not a 20 Hz boundary: EOI only
0x9F612  INC dword [0x12E8C]         ; 20 Hz divider counter
0x9F618  PUSHFD
0x9F619  CALLF [0x12E90]             ; saved-handler call on the boundary
0x9F621  MOV AL,0x20; MOV EDX,0x20; OUT DX,AL     ; EOI (else path)
0x9F629  loop EDX=0,4,..,0x1C: if [0x5BAE4+EDX] != 0 CALL it
0x9F644  POP GS/FS/ES/DS; POPAD; IRETD
```

So `[0x12E88]` increments once per PIT interrupt (100 Hz) with a plain 32-bit
`INC`, and every 5th tick (20 Hz) increments `[0x12E8C]` and chains the saved
handler; other ticks send EOI directly. The game-side clock consumers are
0xCB2A4 `MOV EAX,[0x12E88]; RET`, `FUN_000cb2aa` (`now - arg`), `FUN_000cb2b6`
(`now - [0x127D4]`, storing `now`), 0xCB2C9 (`[0x12E88] = 0`), 0xCB2D1
(`deadline = arg + now`), 0xCB2E1 (spin while `(now - [0x127D0]) < 0`, signed),
0xCB2EF (`SBB/INC`: 1 once `now >= deadline` signed) and 0xCB2FF (spin until
`now >= entry_now + arg`). The ISR's `IDIV` remainder is zero exactly when the
counter is divisible by 5 (divisibility is sign-independent, `2^32 ≡ 1 mod 5`),
so the unsigned `ticks % 5` is equivalent.

`fifa96_pacing` gains the model without changing its existing APIs:
`fifa96_pacing_clock` (`ticks` = 0x12E88, `ticks20` = 0x12E8C),
`fifa96_pacing_clock_init`, `fifa96_pacing_clock_isr` (returns 1 on the 20 Hz
boundary), `fifa96_pacing_clock_elapsed` (0xCB2AA) and
`fifa96_pacing_deadline_reached` (0xCB2EF). `fifa96_pacing_frames_due` remains
the same counter's 15 fps consumer (`ticks*15/100`, 0x67E41).

## 7. Port summary

* `fifa96_mixer_step_from_rate` rejects `v / out_rate >= 256` (the original's
  #DE) and otherwise keeps the FU-46 tail (64-bit intermediates; the fraction
  is `(rem<<24)/out << 8`).
* `fifa96_mixer_start` accepts the raw 0..0xFF record+0x26 gain byte; the
  voice's `gain_l`/`gain_r` are now `uint32_t` and hold the reader gains
  (L 0..0x7F, R 0..0xFFFF).
* `fifa96_mixer_pan_gains_wide` is the exact FUN_000a662c conversion;
  `fifa96_mixer_pan_gains` is its low-byte view (retail-identical);
  `fifa96_mixer_set_pan` uses the wide form.
* `sfx_split` returns the raw split bytes (no 0x7F cap) and `sfx_gain` keeps
  the exact signed IDIV low byte (no clamp), so the two-voice id < 5 default
  path carries the original's signed read-back.
* `fifa96_pacing` adds the §6 tick model; `fifa96_pacing_frames_due` and
  `fifa96_pacing_catch_up` are unchanged.
* Behavior changes to note: FU-46's fabricated `step_int >= 256` value is now
  `TRUNCATED`; `fifa96_mixer_start` no longer rejects `volume > 0x7F` (it is
  the original's unclamped byte); two-voice split volumes above 0x7F are no
  longer capped but read back signed. No retail path changes (all retail
  gains/splits stay in 0..0x7F and every retail pitch is 0).

## 8. Validation (throwaway, not committed)

`/tmp/opencode/fu48/model.py` transcribes the cited instruction streams and
runs against the byte-identical image copy. Decisive output:

```
pitch    9599 -> idx 1199 cl  9 ratio 0x1ffb4 (in table)
pitch    9600 -> idx 1200 cl  9 reads image[0xb7e48] = 0x50ec8b55
pitch  -18000 -> idx    0 cl 31 ratio 0x10000
pitch  -18001 -> idx 1199 cl 32 -> SHRD CL&31 = 0, q=95053 at 16000/22050 (#DE)
step: 5622750 -> 0xff00000000 ; 5622751 -> 0xff0002f800
      5644799 -> 0xfffffd0700 ; 5644800 -> q=256 (rem>>8 == out, #DE)
pan 0x40 gain 0x80 -> ql=0x02040790 L=16 R=1940 ; gain 0xbf -> L=79 R=1999
split(-1, 0x7f, id 0..4) left = 190,177,165,152,139 (0xbe,0xb1,0xa5,0x98,0x8b)
      -> gain bytes equal, centre pan L/R = (78,1998) (65,1989) (53,1973)
         (40,1964) (27,1951)
ISR: ticks 1..4 no callback, 5 and 10 -> ticks20 1 and 2; 0xffffffff+1 -> 0 -> cb
```

`make test`: 37/37 before the slice, **38/38 after** (new
`tests/test_audio_edge.c` with pitch bounds, step boundaries, the split
read-back chain, the wide pan table, the render quirk, the retail two-voice
census and the ISR tick model; `tests/test_mixer.c` updated for the §3/§4
behavior changes). ASan+UBSan build of the whole suite passes with
`detect_leaks=0` (the repo has pre-existing leaks in unrelated fixtures); the
audio tests are leak-clean with leak detection on.

## Open legs

1. The §1.1 out-of-table read is image-dependent code (0x50EC8B55 at the
   first entry); the port rejects rather than carrying the neighbouring code
   bytes. The §1.2 SHRD wrap is not modelled either (the port rejects
   shift > 31); modelling it exactly would still hit §3's #DE at retail-scale
   output rates.
2. `FUN_000a6717` called with a split volume > 0x7F can produce products
   >= 256 whose `SHL 8` bits fold into the neighbouring packed field before
   the callers' `AND 0xFF`; the port's arm input validation (opts->volume
   <= 0x7F) makes that unreachable, so only the byte products' signed
   read-back is modelled.
3. The two-voice CLI window (§5) is not modelled; the port's error paths
   leave no voice armed instead.
4. The second `FUN_000a780e` is replaced by `voice + 1`; only failure-path
   rotor state differs (FU-47 §6 leg 4).
5. FU-37 legs unchanged: f10==2 sample accounting, the queue
   producer/streaming path, `ctx[5]`, `VID_HIPP`, `0x563e0` EOF. FU-46 §2.3's
   PCM8 row-offset detail and the signed arm's one-unit truncation stay as
   documented.

## Provenance (Ghidra calls, 2026-10-04, `/fifa96_le.bin`)

* Decompiled: `FUN_000b86c8`, `FUN_000a662c`, `FUN_000a6717`, `FUN_000a7728`,
  `FUN_000a780e`, `FUN_000a76ea`, `FUN_000a7705`, `FUN_000b9fdd`,
  `FUN_0009f754`, `FUN_000cb2a4`, `FUN_000cb2aa`/`cb2b6`/`cb2d1`/`cb2ef`,
  `FUN_000ae9a0`, `FUN_000ae9d4`.
* Disassembled: `FUN_000b86C8` (full), `FUN_000a662c` (full),
  `FUN_000a6717` (full), `FUN_000a7728` (full), `FUN_000a780e` (full),
  `FUN_000a76ea`/`FUN_000a7705` (full), `FUN_000b9fdd` (full),
  `FUN_0009F754` (full), ISR `0x9F5E4..0x9F64B` (raw bytes),
  `0xCB2A4..0xCB315`; decompiled `FUN_0009E907` (fill), `FUN_0009F64C`
  (callback insert) and `FUN_0009F684` (callback clear), xrefs to `0x5BAE4`.
* Memory/raw reads: image `0xB7E40..0xB7E60` (table end + first out-of-table
  dword), image `0x112E68` context for the clock block, golden
  `tests/golden/sfx_game.bnk` descriptor census.
* Search: operand `12e88`; instruction scopes `0x9F5xx`, `0xCB2xx`.
* Baseline: `make test` 37/37 before the port; 38/38 after it. This doc is
  the first tracked change of the slice.
