# FU-46 — mixer calibration: pitch table and packed pan / L-R gains

Date: 2026-10-04. Program `/fifa96_le.bin` (flat LE link image, base 0;
throwaway copy `/tmp/opencode/fu39/fifa96_le.bin`, byte-identical at every
cited offset, sha256 `bba699e98b3d657e…`). Scope: close the two mixer legs
carried by FU-37/FU-38/FU-45 — (1) the `FUN_000b86C8` pitch-table head so
the 32.32 rate step is the real formula, and (2) the `FUN_000a662c` pan /
gain → per-channel L/R conversion the readers apply. Companion slices:
FU-37 (mixer/voice model), FU-38 (`fifa96_mixer_step_from_rate` port), FU-39
(f10==2 delta), FU-45 (bank arm path). Everything below cites instruction
addresses or image bytes; no claim is made without a cited read.

## 0. Address frame

* Pitch table operand `[EBX*4+0xA6B88]` at `0xB86E6` is object-relative
  (FU-37 §0): the 1200 dwords live at image **0xB6B88**, ending at 0xB7E48,
  one code byte before `FUN_000b7e57` (`0xB7E57`).
* The EACSNDF record array is at runtime `0x161994` (stored `0x61994`,
  stride 0x28); `FUN_000a662c` reads `+0x27` (pan) and `+0x26` (gain).
* `[0x406A0]` (runtime `0x1406A0`) is the output rate (`FUN_000b7e57`
  stores its rate argument; retail 22050, FU-37 §A.4/§A.5).
* Both the video arm `FUN_000a6579` and the bank arm `FUN_000a780e` reach
  `FUN_000a662c` and `FUN_000b7fe8` through the same call at `0xA65D7` /
  `0xA661D` (`FUN_000a780e` calls `FUN_000a6579` at `0xA7991`), so the two
  derivations below cover the whole retail audio path.

## 1. Pitch → 32.32 step (`FUN_000b86C8`, 0xB86C8..0xB8740)

Inputs: `EAX` = sample rate (Hz), `EBX` = signed pitch, `ESI` = channel
struct. `FUN_000b7fe8` calls it at `0xB80CC..0xB80CF` with `EAX = [EBP+0x1c]`
(rate) and `EBX = [EBP+0x2c]` (pitch); `FUN_000a6579` builds that pitch at
`0xA65DC..0xA65FA` as
`rec[0xC] + ((rec[0x10]-0x40) * rec[0x11] * 100)/64` (signed `/64` idiom:
`CDQ; SHL EDX,6; SBB EAX,EDX; SAR EAX,6`). `rec[0xC]` is a dword, `rec[0x10]`
and `rec[0x11]` are bytes. In retail both arms resolve pitch 0 (FU-45 §Open
legs), but the head is now derived for the full domain.

### 1.1 Head — pitch to table window

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
0xB86E6  MOV EDX,[EBX*4+0xA6B88]     ; ratio = table[index]
```

`W` walks down in 1200-entry windows while (signed) `E < W`, so
`index = E-W ∈ [0,1199]` and `shift = 9 + k` where `k` is the number of
windows passed. Since the window step is 1200 and the shift step is 1, one
window is one octave; pitch is in cents. The +0x2000 offset and 0x40D0 base
are calibrated so that **pitch 0 → index 0, shift 16**:

```
pitch 0: E=0x2000; W 0x40D0 → 0x3C20 → … → 0x2000 (k=7), CL=16
         index 0, ratio table[0] = 0x00010000  (unity)
```

The valid window is `E < 0x40D0` (index < 1200); for `E ≥ 0x40D0` the loop
never runs, `shift = 9`, and `index = E - 0x40D0` is in range only for pitch
8400..9599. Positive pitches above 9599 and shifts above 31 are outside the
table (the original would read past `0xB7E48`; the port rejects).

### 1.2 Table (image 0xB6B88)

1200 dwords; entry `i` is the 1/1200-octave (cent) ratio `2^(i/1200)` in
0.16 fixed point (`table[0]=0x00010000`, `table[80]=0x00010C1B`,
`table[1199]=0x0001FFB4`; `table[1199] < 2*table[0]`). Extracted bytes:
sha256 `7f1d8ef2da2ef831df75637e4e84b5a7143b99d58a66b8de3016ff7de87e57d8`.
73 of the 1200 entries differ from `floor(65536*2^(i/1200))` computed in
IEEE double (the original generator used a lower-precision method), so the
port embeds the image bytes verbatim rather than recomputing them
(`src/fifa96_loader/fifa96_mixer_tables.c`; the calibration test pins an
FNV-1a/32 over all 1200 entries, `0xDAFFEBCF`).

### 1.3 Tail — rate, ratio, shift to 32.32 step

```
0xB86ED  MUL EDX                     ; EDX:EAX = rate * ratio (unsigned)
0xB86EF  SHRD EAX,EDX,CL             ; v = low 32 of (rate*ratio) >> CL
0xB86F2  MOV EDI,EAX                 ; keep v
0xB86F4  MOV EDX,0
0xB86F9  DIV [0x406A0]               ; v / out_rate
0xB86FF  AND EAX,0xff                ; step_int (8-bit mask)
0xB8704  MOV [ESI+0x24],EAX          ; +0x24 = step integer
0xB8707  MOV [ESI+0x28],0
0xB870E  MOV EAX,[ESI+0x24]
0xB8711  MUL [0x406A0]               ; (step_int & 0xFF) * out_rate
0xB8717  SUB EDI,EAX                 ; rem = v - step_int*out_rate
0xB8719  MOV EAX,EDI
0xB871B  MOV EDX,0x1000000
0xB8720  MUL EDX                     ; rem * 2^24
0xB8722  DIV [0x406A0]               ; 24-bit fraction
0xB8728  SHL EAX,0x8                 ; scale to the 32.32 low word
0xB872B  MOV [ESI+0x28],EAX          ; +0x28 = fraction
0xB872E..0xB873C                     ; +0x2C = step_int<<24 | frac>>8 (8.24)
```

So, exactly:

```
v        = (uint32)(((uint64)rate * ratio) >> CL)        ; x86 SHRD, CL&31
step_int = (v / out_rate) & 0xFF
rem      = v - step_int * out_rate
frac     = (uint32)((((uint64)rem << 24) / out_rate) << 8)
step     = ((uint64)step_int << 32) | frac
packed   = (step_int << 24) | (frac >> 8)               ; the true 8.24 step
```

**This corrects FU-37 §A.5 / the FU-38 port.** FU-37 wrote the fraction as
`(rem * 2^32) / out_rate`; the instructions compute a **24-bit** fraction
first and then scale it by 256 (`MUL 0x1000000`, `DIV`, `SHL 8`), so the
low 8 bits of `+0x28` are always zero. The two formulas differ in that byte:
for 16000→22050 unity the FU-38 literal 3116529557 becomes **3116529408**;
for 48000→22050 9349588671 → 9349588480; for table[80] 3263785578 →
3263785472. The readers add `+0x28` to the 32.32 cursor
(`ADD EBX,[0x406cc]; ADC EDI,[0x406d0]`, FU-37 §A.3), so the port's step is
the same 32.32 value the engine stores.

Latent divide fault: with the 8-bit mask applied, `rem = 256*floor(v/out/256)
*out + (v mod out)`, so `rem*2^24/out ≥ 2^32` whenever the unmasked
`v/out_rate` is ≥ 256 — the original's second `DIV` raises #DE for every
such input, i.e. the `AND EAX,0xFF` is unreachable in the sense that no input
that reaches it can survive. The port keeps 64-bit intermediates and the
32-bit store truncation, so the documented synthetic case (rate 65535,
out_rate 100, step_int 655 masked to 143) is defined as 615683561728; the
original would fault there. This is the only intentional generalization in
the tail.

### 1.4 Neutral calibration

* Neutral pitch is **0**: ratio 0x10000, shift 16, `v = rate`, i.e. the
  resampler advances one source sample per output frame when
  `rate == out_rate` and `rate/out_rate` otherwise.
* Octaves: pitch ±1200 selects the same table[0] with shift ∓1
  (0x10000/2^15 = 2×, /2^17 = ½×); pitch -18000 is the lowest in-domain
  value (shift 31), pitch 9599 the highest (index 1199, shift 9).
* FU-37 §A.5's "pitch 0 → window index 80" is wrong: table[80] corresponds
  to pitch 80 (`index = pitch` for pitch 0..1199). The retail pitch-0 path
  is table[0]/shift 16, which the FU-38 tests already exercised as a
  literal; only the tail fraction is re-pinned.

## 2. Pan / L-R gains (`FUN_000a662c`, 0xA662C..0xA66AA)

### 2.1 Record gain (`FUN_000b9fdd`, 0xB9FDD..0xB9FF2)

`MOVSX` record+0x25 (descriptor volume), record+0x24 (caller volume) and
`[0x15FD6]` (master) are multiplied and `IDIV 0x3F01` (127²), the quotient's
low byte stored at record+0x26 (`MOV [EBX+0x26],AL`, `0xB9FF2`). For the
retail 0..0x7F operands the gain is 0..0x7F; FU-45's `sfx_gain` already ports
this (with clamps).

### 2.2 Pan conversion

```
0xA663A  MOVZX EAX,byte [ESI+0x27]   ; pan byte p0
0xA663E  CMP EAX,0x7f
0xA6641  JLE 0xA664C
0xA6643  MOV EBX,0xff; SUB EBX,EAX   ; p0 > 0x7F: p = 0xFF - p0 (mirror)
0xA664C  CMP EAX,0x40
0xA664F  JGE 0xA665B
0xA6651  MOV EBX,0x7f                ; p < 0x40: L factor 0x7F
0xA6656  LEA ECX,[EAX+EAX]           ;           R factor 2p
0xA6659  JMP 0xA667F
0xA665B  JLE 0xA6678                 ; p == 0x40
0xA665D  MOV EBX,0x7f
0xA6662  SUB EBX,EAX
0xA6664  IMUL EAX,EBX,0x7e
0xA6667  MOV EBX,0x3e; CDQ; IDIV EBX ; p > 0x40: L factor (0x7F-p)*0x7E/0x3E
0xA666F  MOV ECX,0x7f                ;           R factor 0x7F
0xA6674  MOV EBX,EAX
0xA6678  MOV EBX,0x7f; MOV ECX,EBX   ; p == 0x40: both 0x7F
0xA667F  MOV EAX,EBX
0xA6681  MOVSX ESI,byte [ESI+0x26]   ; signed gain
0xA6685  IMUL EAX,ESI                ; L factor * gain
0xA6688  XOR EDX,EDX
0xA668A  MOV EBX,0x7f
0xA668F  DIV EBX                     ; unsigned /0x7F
0xA6691  MOV EBX,EAX                 ; L
0xA6693  MOV EAX,ECX; IMUL EAX,ESI
0xA6698  XOR EDX,EDX; DIV ECX
0xA66A1  SHL EAX,0x10; OR EAX,EBX    ; packed = (R << 16) | L
```

Semantics:

```
p = pan > 0x7F ? 0xFF - pan : pan          ; 0..0x7F, 0 = left, 0x40 = centre
Lfac = p < 0x40 ? 0x7F : p == 0x40 ? 0x7F : (0x7F-p)*0x7E/0x3E
Rfac = p < 0x40 ? 2p   : 0x7F
L = (uint8)( (uint32)(Lfac * (int8)gain) / 0x7F )
R = (uint8)( (uint32)(Rfac * (int8)gain) / 0x7F )
```

The multiply is a 32-bit `IMUL` and the divide a zero-extended `DIV`, low
byte kept; a valid 0..0x7F gain yields 0..0x7F. `p<0x40` keeps the left
channel at full and ramps the right from 0 to 126; `p==0x40` is 127/127;
`p>0x40` drops the left 127→126→…→0 while the right stays full. Pan bytes
above 0x7F mirror (0x80 → hard right, 0xFF → hard left).

### 2.3 Consumers

`FUN_000a6579` (`0xA65D3..0xA6608`) calls `FUN_000a662c` with the payload
voice byte, then extracts `L = packed & 0x7F` (`AND ESI,0x7f`) and
`R = packed >> 16` (`SHR EDI,0x10`) and pushes them as `FUN_000b7fe8` args
`[EBP+0x20]`/`[EBP+0x24]`. `FUN_000b7fe8` stores `L<<10` at ch+0x64 and
`R<<10` at ch+0x68 (`0xB80DF..0xB80EE`). The readers then apply them
per channel:

* PCM16 stereo `0xB92C6..0xB92F0`: `L = [ch+0x64]>>10`, `R = [ch+0x68]>>10`;
  `MOVSX` sample × gain, `SAR 7`, accumulate to `0x36698`/`0x3669C`.
* PCM8 stereo `0xB8EB9..0xB8EE7`: `[ch+0x64]`/`[ch+0x68]` are used raw as
  byte row offsets into the 0x15FD8 volume table (row = gain<<10 = gain×0x400),
  i.e. `2*gain*signed8(byte)`.
* PCM16 mono `0xB8B0B..0xB8B3E`: one sample × L and × R, `SAR 7`.

`FUN_000a780e` (bank arm) fills the same record and calls `FUN_000a6579` at
`0xA7991`, so the port's `fifa96_sfx_arm` applies the same conversion.

### 2.4 Hand-computed cases (gain 0x7F)

| pan | p | L | R | packed |
|---|---|---|---|---|
| 0x00 | 0x00 | 127 | 0 | 0x0000007F |
| 0x20 | 0x20 | 127 | 64 | 0x0040007F |
| 0x3F | 0x3F | 127 | 126 | 0x007E007F |
| 0x40 | 0x40 | 127 | 127 | 0x007F007F |
| 0x41 | 0x41 | 126 | 127 | 0x007F007E |
| 0x50 | 0x50 | 95 | 127 | 0x007F005F |
| 0x60 | 0x60 | 63 | 127 | 0x007F003F |
| 0x7F | 0x7F | 0 | 127 | 0x007F0000 |
| 0x80 | 0x7F | 0 | 127 | 0x007F0000 |
| 0xC0 | 0x3F | 127 | 126 | 0x007E007F |
| 0xFF | 0x00 | 127 | 0 | 0x0000007F |

## 3. Port

* `fifa96_mixer_pitch_ratio(pitch, &ratio, &shift)` — §1.1 head; rejects
  index ≥ 1200 (pitch > 9599) and shift > 31 (pitch < -18000) with
  `TRUNCATED`; NULL out args likewise.
* `fifa96_mixer_step_from_pitch(rate, out_rate, pitch, &step)` — §1.1 + §1.3;
  used by `fifa96_sfx_arm` (replacing the retail table[0] constant).
* `fifa96_mixer_step_from_rate(rate, out_rate, ratio, shift, &step)` — §1.3
  tail, signature unchanged; `shift > 31` is now rejected (x86 `SHRD` reads
  CL&31, the head never emits > 31).
* `fifa96_mixer_pan_gains(pan, gain, &left, &right)` — §2.2 exactly.
* `fifa96_mixer_set_pan(m, voice, pan)` — applies §2.2 to an armed voice
  using the combined gain `fifa96_mixer_start` stored; called from
  `fifa96_sfx_arm` after the arm. The voice keeps `volume` (combined gain)
  and gains `gain_l`/`gain_r`; `fifa96_mixer_render` uses the pair.
* Table: `src/fifa96_loader/fifa96_mixer_tables.c` (§1.2 bytes).

## 4. Validation (throwaway, not committed)

`/tmp/opencode/fu46/validate.py` against the byte-identical image copy;
decisive output:

```
table[0]=0x10000 table[80]=0x10c1b table[1199]=0x1ffb4
mismatch vs floor(65536*2^(i/1200)): 73 [135, 148, 190, …]
pitch      0 -> idx     0 cl 16 ratio 0x10000
pitch   1200 -> idx     0 cl 15 ratio 0x10000
pitch  -1200 -> idx     0 cl 17 ratio 0x10000
pitch   8400 -> idx     0 cl  9 ratio 0x10000
pitch   9600 -> idx  1200 cl  9 ratio OUT
 16000 22050 0x10000 16  old   3116529557  new   3116529408  packed 0x00b9c277
 48000 22050 0x10000 16  old   9349588671  new   9349588480  packed 0x022d4766
 16000 22050 0x10c1b 16  old   3263785578  new   3263785472  packed 0x00c2896a
 65535   100 0x10000 16  old 615683561881  new 615683561728  packed 0x8f599999  q=0x200599999  <-- DIV #DE
pan 0x50 gain 127 -> p 0x50 L  95 R 127 packed 0x007f005f
pan 0x80 gain 127 -> p 0x7f L   0 R 127 packed 0x007f0000
```

The transcribed table file was diffed against the image slice: 1200/1200
entries equal, sha256 `7f1d8ef2…`. The calibrated 16000→22050 pitch-0 step
was run on the real `tests/golden/eacs/bank-h.eacs` chunk: cursor units
0,0,1,2,2,3,4,5 select (-32,-38) (-32,-38) (-32,-40) (-35,-42) (-35,-42)
(-36,-41) (-31,-42) (-32,-44), pinned in
`tests/test_mixer_calibration.c` (the FU-38 1:1 first frames stay pinned in
`tests/test_mixer.c`).

## Open legs

1. Original out-of-table pitch behavior: the head's `MOV EDX,[EBX*4+0xA6B88]`
   reads past the table for pitch > 9599 and its `SHRD` wraps CL mod 32 for
   pitch < -18000; both are unreachable in retail (all descriptors and video
   records resolve pitch 0). The port rejects them.
2. `step_int ≥ 256` divide fault (§1.3): the port defines a truncated value
   where the original raises #DE; no retail rate/pitch reaches it.
3. Pan bytes with a gain > 0x7F (negative signed gain): §2.2's unsigned
   `DIV` of the negative 32-bit product is ported byte-for-byte, but only
   0..0x7F gains occur in retail.
4. FU-37 legs unchanged: f10==2 sample accounting, the queue
   producer/streaming path, `ctx[5]`, `VID_HIPP`, `0x563e0` EOF.

## Provenance (Ghidra calls, 2026-10-04, `/fifa96_le.bin`)

* Decompiled: `FUN_000b86c8`, `FUN_000a662c`, `FUN_000b9fdd`,
  `FUN_000a780e`.
* Disassembled: `FUN_000b86C8` (full), `FUN_000a662c` (full),
  `FUN_000a6579` (full), `FUN_000b7fe8` (full), `FUN_000b9fdd` (full),
  PCM readers `0xB8E8F`, `0xB929D`, `0xB8AE1`.
* Xrefs/callers: `FUN_000a662c` (1 caller `0xA65D7`), `FUN_000b7fe8`
  (1 caller `0xA661D`), `FUN_000a780e` call `0xA7991`.
* Memory: image `0xB6B88` (1200 dwords) and `0xB7E30..0xB7E48` tail.
