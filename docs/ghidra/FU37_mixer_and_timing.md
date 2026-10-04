# FU-37 — audio mixer/voice model and video timing/sentinels

Date: 2026-10-04. Program `/fifa96_le.bin` (flat LE link image, base 0).
Scope: (A) the software mixer behind EACS voice playback — voice table,
per-format sample fetch, mix formula, output formats, loop and rate step;
(B) the `vgt_stream_poll` player — chunk sentinels and the frame/audio
pacing model. Companion slices: FU-29/FU-30 (chunk walker), FU-35 (EACS
queue/header), FU-31/FU-32 (fVGT frames), FU-4 (LE image).

Everything below cites instruction addresses in the link-time image;
decompiler output is quoted only where it is unambiguous, otherwise the
disassembly is. No claim is made without a cited read.

## 0. Address frame (read first) — object-relative operands

The flat image places each LE object at its link base and does **not**
apply LE fixups (FU-4). Absolute operands are therefore *object-relative*:
object 1 (code/constants) at `0x10000`, object 4 (data/stack) at
`0x100000`. Evidence collected in this slice:

* Voice table operand `[ESI*4+0x406d8]` (`0xB7F8E`): the 16-entry table is
  present at image `0x1406D8` (read_memory: 16 dwords, first `0x35FD8`,
  stride `0x6C`), not at `0x406D8` (object-1 code bytes there).
* Callback immediates `0xa9a46`/`0xa8ae1`/… stored by `FUN_000b81f0` land
  on coherent mixer callbacks at `0xB9A46`/`0xB8AE1` (FU-35 fixup caveat).
* Mixer accumulator `0x36698` (`FUN_000b9f82`) is zero-filled at image
  `0x136698`; the volume table built at `0x15fd8` (`FUN_000b8741`) is
  zero-filled at image `0x115FD8`; pitch table operand `0xa6b88`
  (`0xB86E6`) holds `table[0] = 0x00010000` at image `0xB6B88`.
* `FUN_000a7d3c`'s queue globals `0x5db98/0x5db9c` (FU-35) are object 4 →
  image `0x105db98/0x105db9c`.

Citations below quote the operand text as Ghidra renders it; a
parenthesised `(→ 0x…)` gives the relocated link address. This closes
FU-35 open leg 2 (the `0x15FD8` table is real and in object 4; the raw
bytes at `0x15FD8` that "disassemble as code" are object-1 code).

## Part A — mixer/voice model

### A.1 Voice table, allocation and free

The mixer owns a fixed array of **16 channel structs**; there is no
allocation call. A table of 16 pointers lives at runtime `0x1406D8`
(stored operand `0x406d8`):

```
0x1406D8: 0x035FD8 0x036044 0x0360B0 ... 0x03662C     (16 dwords)
          stride 0x6C; entry i = 0x35FD8 + i*0x6C  (stored form)
```

* `FUN_000b7f62` (`0xB7F7D..0xB7F98`) increments the voice index, stops at
  16, loads `MOV ESI,[ESI*4+0x406d8]`, and tests `CMP byte [ESI],1` —
  a struct is *active* iff byte 0 == 1.
* `FUN_000b80fa` frees: `MOV byte [ESI],0` + `FUN_000ba00e(voice)`
  (resets the EACSNDF record, below).
* `FUN_000b817b` returns the state byte or `-1`; `FUN_000b81a0` reports
  frames remaining (via `+0x10 - +0x14`, or `+0x4C - +0x38` for the
  second cursor).

Channel struct layout (offsets set by `FUN_000b7fe8` disassembly
`0xB7FE8..0xB80F9` unless noted; all fields 32-bit unless marked):

| off | meaning | citation |
|---|---|---|
| `0x00` | state: 0 free, 1 active (2 transient during arm) | `0xB7FF9`, `0xB80F1` |
| `0x01` | loop flag (header loop-length != 0) | `0xB800D..0xB8013` |
| `0x02` | second-cursor flag (`flags&2`, f10==2) | `0xB8068..0xB80A3` |
| `0x03` | signed-samples flag (`flags&0x10`, EACS voice>=0) | `0xB801B..0xB8024` |
| `0x04` | format flags (`8`=f8==2, `4`=f9==2, `2`=f10==2, `0x10`=signed) | `0xB8003..0xB8006` |
| `0x08` | unit-shift count (one per f8/f9==2); cursor = data >> shift | `0xB802B..0xB8044` |
| `0x0C` | sample rate (Hz, header+4) | `0xB80C6..0xB80C9` |
| `0x10` | end cursor (units; = cursor + blocks - 1) | `0xB8051..0xB8059` |
| `0x14` | cursor integer part (unit index) | `0xB8047` |
| `0x18` | cursor fraction (low 32 of 32.32) | `0xB804A`; readers add/adc |
| `0x1C` | loop-start cursor (data + loop-start) | `0xB805C..0xB805F` |
| `0x20` | loop-end cursor (+ start + length) | `0xB8062..0xB8065` |
| `0x24` | step integer part (masked 8 bits) | `0xB86FF..0xB8704` |
| `0x28` | step fraction (scaled 2^32) | `0xB8707..0xB872B` |
| `0x2C` | packed step 8.24 = `(+0x24<<24)｜(+0x28>>8)` | `0xB872E..0xB873C` |
| `0x30` | second-cursor state/fraction | `0xB8075`; `0xB855E..` |
| `0x34` | second-cursor state/long | `0xB807C`; `0xB856F..0xB8580` |
| `0x38` | second cursor (nibble position) | `0xB8083`; `0xB84FE` |
| `0x3C` | second cursor fraction (advance) | `0xB808A`, `0xB85BA` |
| `0x40` | second data base (payload, or payload+0x14 when signed) | `0xB809D..0xB80C2` |
| `0x44` | second loop start / second end | `0xB8091..0xB8094`; `0xB83FB` |
| `0x48` | second loop end | `0xB8097..0xB809A` |
| `0x4C` | second end count | `0xB809D..0xB80C2`; `0xB8552` |
| `0x50`,`0x54` | saved second-cursor loop state | `0xB8420`, `0xB8426` |
| `0x58` | second-cursor (nibble) decoder, set only for f10==2 | `FUN_000b81f0` |
| `0x5C` | mix callback (per-format reader) | `FUN_000b81f0` |
| `0x60` | producer/advance callback | `FUN_000b81f0` |
| `0x64` | left volume ×1024 | `0xB80DF..0xB80E5` |
| `0x68` | right volume ×1024 | `0xB80E8..0xB80EE` |

A second, independent per-voice record (EACSNDF state, stride 0x28) sits at
runtime `0x161994` (stored `0x61994`): `FUN_000ba00e` resets it
(`+0x0` = 0, `+0x4` = -1, `+0x24/+0x25` = 0x7F, `+0x27` = 0x40, volume at
`+0x26` via `FUN_000b9fdd`); `FUN_000a662c` reads the record at
`0x161994 + voice*0x28` and converts pan+volume to the packed
`(L<<16)｜R` passed to the mixer. Voice index for both tables is
EACS payload byte `+0x0B`, copied to `+0x1C` by `FUN_000a6579`
(`0xA65C3..0xA65D0`).

### A.2 Arm path and the EACS queue consumer

Video audio arms through `FUN_000a7ce4` → `FUN_000a6579` → `FUN_000b7fe8`:

* `FUN_000a7ce4` (`0xA7CE4`) requires audio state `[0x15fc8]` in 1..5,
  voice `[0x149d0]` >= 0, state `[0x149cc] == 3`; zeroes the chunk counter
  `[0x149e8]`, calls `FUN_000b9fdd(voice)` then `FUN_000a6579(header)`,
  and sets state 2 (`0xA7D0E..0xA7D2C`).
* `FUN_000a6579` builds `flags` from payload bytes 8/9/10/0xB, then calls
  `FUN_000b7fe8` with ten stack arguments pushed in this order
  (`0xA65FD..0xA661D`): pitch, flags, R, L, rate=payload+4,
  looplen=payload+0x14, loopstart=payload+0x10, blocks=payload+0xC,
  data=payload+0x18, voice=payload+0x1C. The pitch argument is
  `rec[0xC] + ((rec[0x10]-0x40)*rec[0x11]*100)/64` (signed `/64` idiom,
  `0xA65DC..0xA65FD`).
* `FUN_000b7fe8` writes the struct fields above, calls `FUN_000b86c8`
  (rate→step, §A.5) and `FUN_000b81f0` (callback picker, §A.4), then sets
  state 1.

The **queue consumer** is `eacs_dequeue_next` (`0xA7BD7`), called from
exactly two sites in the mixer: `0xB83A6` (main cursor, producer
`0xB8372`) and `0xB8532` (second cursor, producer `0xB84FE`) — this
closes FU-35 open leg 3. Disassembly `0xA7BDF..0xA7C5F` (argument frame:
the two pushes land in `[EBP+0x10]/[EBP+0x14]`; the callers pass
`&cursor` first and `&end` second):

```
head = [0x5db9c]; tail = [0x5db98]
if head == tail:
    if [head] == -1:  out1 = 0; [0x149cc] = 1 (audio start); out2 = 0; ret
    out1 = head+8; out2 = ([head+4]-8) / [0x149d4]; ret          ; 0xA7C13
else:
    out1 = [[head]] + 8                                          ; payload
    out2 = ([[[head]]+4] - 8) / [0x149d4]                        ; blocks
    [0x149e8]++ ; [0x5db9c] = [head] ; [head] = 0xfffffffe       ; 0xA7C2D
```

`[0x149d4]` is the EACS block size (`f8*f9`, FU-35), so the count is in
block units; the producer then shifts the *pointer* by `[ch+8]` and adds
`count-1` to form the end (`0xB83B4..0xB83C3`). `[0x149e8]` is the chunk
counter that the video player uses as its clock when audio is active
(Part B). `0xfffffffe` (-2) is the consumed-node mark.

### A.3 Per-format sample fetch

All readers share a prologue (each address below is one specific reader):

```
EBP = [0x406b4] * 8                       ; accumulator frame index -> byte
ECX = [0x406b8]                           ; frames to produce this call
EDI = [ESI+0x14] ; EBX = [ESI+0x18]       ; 32.32 cursor (int : frac)
[0x406d0] = [ESI+0x24] ; [0x406cc] = [ESI+0x28]   ; step high : low
volL = [ESI+0x64] >> 10 ; volR = [ESI+0x68] >> 10
loop over ECX frames, 16 unrolled:
    ... fetch sample(s) ... ADD [EBP+0x36698], L ; ADD [EBP+0x3669C], R
    ADD EBX,[0x406cc] ; ADC EDI,[0x406d0]         ; cursor += step
```

* **16-bit stereo** — `0xB929D` (f8=2,f9=2, `[0x4069C]!=1`; block 4 B):
  `0xB92DB MOVSX EAX,word [EDI*4]` (L), `0xB92F0 MOVSX EAX,word [EDI*4+2]`
  (R), `IMUL EAX,EDX/ESI`, `SAR 7`, `ADD` to `0x36698`/`0x3669C`.
  Signed little-endian L/R frame pairs.
* **16-bit stereo, 8-bit output variant** — `0xB9A46` (`[0x4069C]==1`):
  reads only the high bytes `[EDI*4+1]` / `[EDI*4+3]` through the volume
  table (`0xB9A81..0xB9AA4`).
* **8-bit stereo** — `0xB8E8F` (f8=1,f9=2; block 2 B): L =
  `byte [EDI*2]` (`0xB8ECB`), R = `byte [EDI*2+1]` (`0xB8EE0`), each
  scaled by table lookup `[row + byte*4 + 0x15fd8]` (`0xB8ED2`,
  `0xB8EE7`) where `row = [ch+0x64]` (raw = volume × 0x400).
* **16-bit (bank) mono** — `0xB8AE1` (f8=2,f9=1,f10=2): one
  `MOVSX word [EDI*2]` per 2-byte block duplicated to both accumulators
  with separate L/R gains (`0xB8B21..0xB8B3E`); `0xB96A8` is the
  `[0x4069C]==1` high-byte variant.

**8-bit polarity (FU-35 open leg 2), resolved.** `FUN_000b8741`
(`0xB8741..0xB8740`) builds a 128×256 dword table at stored `0x15FD8`
(runtime `0x115FD8`):

```
for vol in 0..127:
  for b in 0..127:   tbl[vol][b] = b * 2 * vol
  for b in 0..127:   tbl[vol][128+b] = (b-128) * 2 * vol
```

`MOVSX`-style interpretation: `tbl[vol][b] = 2*vol*signed8(b)`. The
8-bit streams are therefore **signed bytes** (bias 0, two's complement),
scaled ×2 to match the 16-bit readers' full-scale output (16-bit:
`(s*vol)>>7`; 8-bit: `2*vol*s8`).

### A.4 Mix formula and output buffer

* Mix is a plain 32-bit **accumulate, no saturation** in the readers:
  `ADD [EBP+0x36698],EAX` / `ADD [EBP+0x3669C],EAX`. The accumulator is
  `2 dwords per frame` (L at `+0`, R at `+4`), `DAT_406a4` frames long,
  based at runtime `0x136698` (stored `0x36698`). `FUN_000b9f82` zeroes
  `DAT_406a4*2` dwords with `STOSD.REP` (`0xB9F89..0xB9F99`).
* `FUN_000b7f62` (`0xB7F62`) is the per-buffer loop: every active voice
  runs producer `(*[ch+0x60])` then reader `(*[ch+0x5c])` until the
  buffer (`[0x406a4]`) is filled; after all 16 voices it calls
  `(*[0x406d4])` (`0xB7FDF`), the output converter. `[0x406a8]` is the
  output pointer, set from its stack argument (decompiler `param_3`) at
  `0xB7F6B/0xB7F6E`.
* Output converter is chosen once by `FUN_000b7e57` from
  `format & 0x1C` (`0xB7EAC..0xB7F55`), relocated:
  `0xC → 0xB9E53`, `0x14 → 0xB9EB4`, `0x10 → 0xB9F11`,
  `0x1C/0x18/0x8/0x4/0 → 0xB9F7D/0xB9F7E/0xB9F7F/0xB9F80/0xB9F81` (all
  single `RET` stubs). Converters:

| addr | reads | clamps | writes |
|---|---|---|---|
| `0xB9E53` | `[ECX*4+0x36698]` dword/frame-channel | ±32767 | `word [EDI+ECX*2]` = signed 16-bit stereo (`0xB9E7B`) |
| `0xB9EB4` | same | ±32767 | `AH^0x80`; `byte [ECX+EDI]` = unsigned 8-bit stereo (`0xB9EDC..0xB9EDF`) |
| `0xB9F11` | `([ECX*8+0x36698]+[ECX*8+0x3669C])>>1` | ±32767 | `AH^0x80`; `byte [EDI]` = unsigned 8-bit mono (`0xB9F28..0xB9F70`) |

* The game's own mixer setup is at `0xB5911..0xB5928`:
  `PUSH 0x200` (512 frames), `PUSH 0x5622` (22050 Hz), `PUSH 0xC`
  (format), `CALL 0xB7e57` — so output is **22050 Hz, 16-bit signed
  stereo, 512-frame buffers**, written through converter `0xB9E53` to the
  buffer passed to `FUN_000b7f62`.

### A.5 Step/rate conversion (resampling) and loops

`FUN_000b86C8` disassembly (`0xB86C8..0xB8740`, inputs EAX=rate Hz,
EBX=pitch, ESI=channel):

```
EBX += 0x2000; EDX = 0x40d0; CL = 9
while EBX < EDX: EDX -= 0x4b0; CL++            ; coarse exponent
EBX -= EDX                                     ; window index 0..0x4af
EDX = [EBX*4 + 0xA6B88]                        ; pitch ratio table
EDX:EAX = rate * ratio
EAX = (rate*ratio) >> CL                       ; SHRD EAX,EDX,CL
step_int = EAX / out_rate ; masked to 8 bits -> +0x24
rem = EAX - step_int*out_rate
+0x28 = (rem * 2^32) / out_rate                ; fraction
+0x2C = (step_int<<24) | (+0x28>>8)            ; 8.24 step
```

* `out_rate` is `[0x406a0]` (runtime 0x1406A0), set by `FUN_000b7e57`
  from its rate argument (22050 in the retail call); `[0x406a4]` is the
  frame count (512).
* The cursor is a **32.32 fixed-point resampler** (integer in `+0x14`,
  fraction in `+0x18`); advancement is `ADD frac,step_low` /
  `ADC int,step_high` in the readers. There is **no interpolation**: the
  integer part alone indexes the source (`[EDI*4]`, `[EDI*2]`), i.e.
  nearest-lower sample selection. The EACS rate need not match the output.
* The pitch table at stored `0xA6B88` (runtime `0xB6B88`) is monotone,
  `table[0] = 0x00010000` (unity); 1200 entries per loop window
  (observed `table[80] = 0x10C1B`). A zeroed EACSNDF record
  (`rec[0xC]=rec[0x10]=rec[0x11]=0`) yields pitch 0 → window index 80,
  CL 16; the exact neutral-pitch calibration depends on the record fields
  and is a capture target (§Open legs).
* **Loop handling**: `FUN_000b7f62` itself has no loop logic. The
  producer callbacks do:
  * main cursor, loop armed (`0xB832D`): when
    `([ch+0x20]-[ch+0x14])/[ch+0x2C] == 0`, set cursor `[ch+0x14]=[ch+0x1C]`
    (loop start) and fraction 0, then recompute (`0xB8354..0xB8361`).
  * second cursor, loop armed (`0xB83F1`): wraps the second cursor to
    `[ch+0x44]`, restores saved state `[ch+0x34]=[ch+0x50]` etc.
    (`0xB8457..0xB846E`).
  * `FUN_000a79dc` force-writes EACS payload `+0x10 = -1` and
    `+0x14 = 0` for video (FU-35), so **retail videos never arm a loop**;
    the loop producers belong to the bank/sample path.
* **Queue streaming** (no loop): `0xB8372` for the main cursor computes
  frames left `= (([ch+0x10]-[ch+0x14]) * 2^24) / [ch+0x2C]` via
  `SHRD/SHR` + `DIV` (`0xB8372..0xB8394`); on 0 it calls
  `eacs_dequeue_next(&ch+0x14, &ch+0x10)` (`0xB83A6`), then
  `SHR [ch+0x14],shift` and `[ch+0x10] += [ch+0x14]-1`
  (`0xB83AE..0xB83C3`); on a null pointer (stream end) with no frames
  left it stops the voice (`0xB83C5..0xB83E1`, `CALL 0xB80fa`). The
  producer returns `min(frames_left, [0x406b0])`, the frames still needed
  for the buffer.
* **f10==2 second cursor** (`0xB84FE`, signed+second-cursor, no loop):
  dequeues into `[ch+0x38]`/`[ch+0x4C]` (`0xB8532`), then reads a 20-byte
  block header from the payload: `[0]`=data byte count (stored to
  `[ch+0x4C]`), `[4..0x13]`=decoder/cursor state → `[ch+0x30]/[ch+0x34]`,
  data at `+0x14` → `[ch+0x40]`; calls the `[ch+0x58]` decoder with the
  source, destination `0x38698` (runtime `0x138698` staging buffer), and
  count `[0x406c8]` (`0xB8546..0xB85F3`); finally points the main cursor
  at the staging buffer (`SHR EBX(0x38698),CL; [ch+0x14]=EBX`,
  `0xB8601..0xB860B`) so the normal `[ch+0x5c]` reader mixes it.
* The `[ch+0x58]` decoders are `0xC4CC4`/`0xC4D6C` (f8=2,f9=2 selected by
  `flags&4`, `FUN_000b81f0`). `0xC4CC4` splits nibbles (`SHR BL,4` on even
  index, `AND BL,0xF` on odd, `0xC4CEB..0xC4CFD`), adds a step from the
  table at stored `0x41668` (runtime `0x141668`), clamps to ±32767 and
  writes one 16-bit word per nibble (`0xC4D15`); `0xC4D6C` is the other
  nibble-order variant.

### A.6 Part A pseudocode (mixer tick)

```
mix_buffer(out_ptr):
    zero accumulators[0 .. 2*[0x406a4])          ; FUN_000b9f82
    for v in 0..15:
        ch = [0x1406d8 + v*4]
        if ch.state != 1: continue
        todo = [0x406a4]; done = 0
        while todo > 0:
            n = (*ch.producer)()                  ; frames available
            if n <= 0: break
            (*ch.reader)()                        ; mix n frames
            todo -= n; done += n
    (*[0x406d4])()                                ; clamp+format -> out_ptr
```

Reader per frame: `accL[f] += scale(sampleL)`, `accR[f] += scale(sampleR)`,
cursor += step (32.32).

## Part B — playback timing and sentinels

### B.1 Stream walker and sentinels (`FUN_00095cb3`, `0x95CB3`)

Context fields (decompile `0x95CB3`): `ctx[0]`=base, `ctx[1]`=base+size,
`ctx[3]`=end, `ctx[4]`=cursor, `ctx[5]`=tracked chunk, `ctx[7]`=mode.
Chunks are `[u32 tag][u32 len]` (FU-30). The walker:

```
if ctx[7] in {1,2}: return 0                      ; closed/linear mode
if ctx[4] == ctx[3]: return 0                     ; end
if *ctx[4] == -1:                                 ; REWIND sentinel
    ctx[4] = ctx[0]
    if ctx[3] == ctx[4]: return 0
len = *(u32*)(ctx[4]+4)
remaining check (wrapped or not) -> return 0      ; truncated
chunk = ctx[4]; ctx[4] += len
if *chunk == -3:                                  ; SKIP sentinel
    if ctx[5] == chunk: ctx[5] = ctx[4]           ; tracked: just advance
    else: *chunk = -2                              ; release mark
    return -1
return chunk
```

* `-1` (`0xFFFFFFFF`) is a **rewind**: the cursor resets to the stream
  base and the walker transparently reads the first chunk again; it is
  never returned to the caller.
* `-3` (`0xFFFFFFFD`) is a **skip/release**: the chunk is consumed and the
  walker returns `-1`; if the chunk is the tracked `ctx[5]` it is left
  alone (the caller moved on), otherwise its tag is overwritten with
  `-2`.
* `-2` (`0xFFFFFFFE`) is the **released/dead** marker, written by the
  walker's skip path and by `FUN_00095dd2` (`*param = 0xfffffffe`), which
  the player calls on every non-frame chunk (`0x67C8E`, FU-35).
* The `1SNe` end-of-audio marker is *not* a stream sentinel: it is routed
  to `FUN_000a7b2b` and later makes `eacs_dequeue_next` set `[0x149cc]=1`
  (FU-35). The walker sentinels and the EACS queue markers are distinct
  layers.

### B.2 Clock: `FUN_000cb2a4` is a 100 Hz software tick

`FUN_000cb2a4` is `MOV EAX,[0x12e88]; RET` (`0xCB2A4`). `0x12E88` is
maintained by a PIT-driven interrupt handler installed by
`FUN_0009F754` (decompile + disasm):

* `FUN_0009F754` chains the old INT 8 vector (`FUN_0009e907(8,0)`,
  saved at `0x12E90/0x12E94`), unmasks IRQ0 (`in 0x21; or 3; out 0x21`),
  and programs PIT channel 0 with divisor bytes `0x9C,0x2E` → `0x2E9C`
  (`out 0x40`). `1193182 / 0x2E9C = 100.0 Hz` — the counter is a
  centisecond clock.
* The ISR at `0x9F5E4` (raw bytes `0x9F5F5..`: `8B 15 88 2E 01 00`
  `MOV EDX,[0x12E88]`; `42` `INC EDX`; `BB 05 00 00 00` `MOV EBX,5`;
  `89 15 88 2E 01 00` `MOV [0x12E88],EDX`) increments `0x12E88` every
  interrupt, then `IDIV EBX` with `EBX=5`, and on every 5th tick
  increments `0x12E8C` (20 Hz) and far-calls the old handler; on other
  ticks it only sends EOI (`MOV AL,0x20; OUT DX,AL`).
* `FUN_000cb2aa` = elapsed (`[0x12E88]-arg`), `FUN_000cb2b6` = delta from
  a saved value, `0xCB2C9` = reset to 0.

### B.3 `vgt_stream_poll` pacing (`0x67BA8`)

Outer loop = one video frame per iteration (function `0x67BA8`; the
decompile is quoted in full in the session log):

1. `start = clock()`. Inner loop A walks the **frame stream**
   (`FUN_00095cb3([0x563d4])`), dispatches VGT chunks
   (`vgt_dispatch`, `[0x563cc]`), and stops after `clock() <= start+500`
   (5 s watchdog at 100 Hz) or when a frame was decoded. A `-1` return
   from the walker returns 0 immediately.
2. `[0x563c4] = [0x563dc]` (target = frames decoded so far),
   `[0x563f0] = [0x563dc] + 2` (on-time bound).
3. Inner loop B pumps the **companion/audio stream**
   (`FUN_00095cb3([0x563f4])`): if sound is present
   (`[0x563d0] != 0`), chunks go to `FUN_000a7d3c`; when queued bytes
   (`FUN_000a7e31(0)`) exceed `0x20000`, `FUN_000a7ce4` starts audio and
   `[0x563ec]` latches 1 (`0x67D46..0x67DE2`). If sound is absent, chunks
   are released (`FUN_00095dd2`). A `-1` sets `[0x563e0] = 1`, which
   disables further companion reads for the rest of the movie.
   The loop runs while `[0x563e4] < [0x563c4] || [0x563ec] == 0`, with a
   30-tick (300 ms) no-progress break per chunk-fetch iteration
   (`0x67E3B..0x67E41`): `if (fetch_clock + 0x1e < now) break`.
4. Progress `[0x563e4]` (single store `0x67E41`; both branches converge):
   * sound absent: `[0x563e4] = clock()*15/100 - [0x563f8]`; at 100 Hz
     this is exactly **elapsed frames at 15 fps**
     (`ticks*15/100 = seconds*15`).
   * sound present: `[0x563e4] = FUN_000a7ebc() - [0x563f8]`, i.e. the
     number of **dequeued EACS chunks** (`[0x149e8]`, FU-35) since the
     frame time base. Audio consumption is the master clock.
   `[0x563f8]` is that base, written when the frame's audio starts
   (`clock()*15/100` or `FUN_000a7ebc()`; `0x67DFF`, and `0x67B1D` at
   stream init). The audio route call itself is at `0x67D57` (FU-35).
5. `[0x563dc]++` (frame counter). If `[0x563e4] < [0x563f0]` the poll
   returns (`[0x563c8]++`, the on-schedule counter, `0x67E66..0x67E8C`);
   otherwise it loops and decodes the next frame immediately — a
   catch-up path used when audio has advanced ≥ 2 chunks past the
   displayed frame.

Frame pacing summary: the player is **audio-clocked when audio exists**
(one frame per dequeued 1SNd chunk; no fixed timer needed), and a
**measured 15 fps timer** (`ticks*15/100`) when the sound card is absent.
At end of audio, `[0x563e0]` latches and the companion stream is no
longer read, so the audio counter freezes; the remaining frames drain
through the frame branch until that stream also ends (the exact frame-EOF
predicate `FUN_000949f8`/`FUN_00095eb8` is an open leg). The
`1SNe` marker is queued by `FUN_000a7b2b` and on a later dequeue makes
`eacs_dequeue_next` set `[0x149cc]=1` (FU-35); a companion-stream `-1`
is what latches `[0x563e0]`.

Timing stores, one line each:

| addr | role | citation (xrefs) |
|---|---|---|
| `0x563c0` | current chunk tag | write `0x67C34` |
| `0x563c4` | frame target (frames done at frame start) | write `0x67CBD`, read `0x67E3B` |
| `0x563c8` | on-schedule return counter | write `0x67E86`, read `0x67E7A` |
| `0x563cc` | `vgt_dispatch` result | write `0x67C6A`, read `0x67CAC/0x67E81` |
| `0x563d0` | sound-present flag (from `FUN_00064f70`) | write `0x67A3A` |
| `0x563d4` | frame stream context | write `0x67A83` |
| `0x563d8` | current chunk (frame or companion) | write `0x67BD5/0x67CD7/0x67CF0` |
| `0x563dc` | frames displayed | write `0x67B3B`, inc `0x67E6C` |
| `0x563e0` | audio-EOF latch | write `0x67D2E`, read `0x67CD1` |
| `0x563e4` | progress (frames or audio chunks) | write `0x67E41`, read `0x67E60` |
| `0x563e8` | frame decode context | write `0x67A26`, reads `0x67C55/0x67C78` |
| `0x563ec` | audio-started latch | write `0x67B41/0x67D8E/0x67DDA` |
| `0x563f0` | `[0x563dc]+2` catch-up bound | write `0x67CC5` |
| `0x563f4` | companion/audio stream context | write `0x67AA8`, reads `0x67CE1/0x67DBC/0x67F7C` |
| `0x563f8` | progress time base | write `0x67B1D/0x67DFF`, read `0x67E33` |
| `0x563fc` | kVGT palette-copy flag | write `0x67C49`, read `0x67C61` |

## Validation (throwaway, not committed)

Assets extracted from the read-only ISO with `tools/fifa96_bind.iso_files`
(73 `VIDEO/*.TGV` → `/tmp/opencode/fu37/tgv/`); probe
`/tmp/opencode/fu37/tgv_probe.py` plus inline runs. Decisive results:

```
$ python3 /tmp/opencode/fu37/tgv_probe.py
== tag census over 73 TGV ==
   1SNd 15067   1SNe 73   1SNh 73   fVGT 11453   kVGT 3607
== sentinel occurrence ==
  files with sentinels: Counter()          # 0 / 73 of tags -1/-2/-3
== EACS header census ==
  rates: {16000: 72, 16384: 1}
  (f8,f9,f10): {(2,2,0):29, (2,2,2):11, (1,2,0):33}
  voice: {7:28, 0:45}
```

* **Declared count == model** for all 62 f10=0 files once the `1SNh`
  chunk's own data blocks are counted:
  `declared = ((len_h-8-0x20)/(f8*f9) + Σ (len_d-8)/(f8*f9))`
  (FU-35's VID_BULL equality generalises to 62/62; the 11 f10=2 files are
  the f10=2 accounting case below).
* **f10=2 chunk header**: 5700/5700 `1SNd` payloads satisfy
  `u32[0] == payload_len - 0x14`; VID_INTR's header-chunk data begins
  `29 04 00 00` (1065) with 16 zero state bytes, and the first `1SNd`
  payload begins the same. The 20-byte block header (count + state +
  data) is real, and the engine's `blocks*4` unit for f10=2 equals
  payload bytes.
* **15 fps pacing**: audio-duration/frame-duration ratio
  (samples/rate ÷ frames/15) is in [0.93,1.05] for 61/73 files at the
  `blocks*unit` model; mean `1SNd` chunk duration at 16000 Hz is
  **66.93 ms** (min 66.57, max 68.13) vs 66.67 ms at 15 fps; mean
  frames/chunks = **1.01**. This is consistent with one dequeued chunk
  per displayed frame. The single outlier is `VID_HIPP.TGV`
  (37 frames / 31 chunks, 33792 samples, ratio 0.856).
* **f10=2 sample accounting** (open item): with 2 nibbles per data byte
  (`[ch+0x58]` writes one word per nibble) the audio duration is exactly
  **2×** the 15 fps video duration for all 11 files (ratios 1.99–2.08);
  with the engine's `blocks*4` (payload bytes) unit it is ~1×. Either
  the playback consumes one nibble per engine sample or the second-cursor
  staging is resampled by half; not resolvable offline.
* **Voice table**: image `0x1406D8` holds 16 dwords at stride `0x6C`
  (`0x35FD8 + i*0x6C`), matching the `[ESI*4+0x406d8]` operand and the
  0x6C field map in §A.1.
* **Timer**: `FUN_0009F754` writes PIT divisor `0x2E9C` (1193182/11932 =
  100.00 Hz) and installs the ISR; `FUN_000cb2a4` reads `0x12E88`. At
  15 fps the code's `clock*15/100` yields one progress unit per 6.667
  ticks = 66.67 ms.

Baseline/tests: `make test` **30/30** (30 CTest entries, before and after
this doc; the doc is the only tracked change).

## Open legs / capture targets

1. **f10=2 sample accounting** (§A.5, validation): hook `0xB84FE` entry
   and `0xB4CC4`/`0xB4D6C` (or dump the staging buffer at runtime
   `0x138698` and `[ch+0x38]/[ch+0x4C]`) and count nibbles/samples
   produced per payload. Dump the accumulator region `0x136698` after
   `FUN_000b7f62` for the same frame.
2. **Neutral pitch/record fields**: dump the EACSNDF record
   `0x161994 + voice*0x28` at `FUN_000a6579` entry (bytes `+0xC`, `+0x10`,
   `+0x11`) and `[0x406a0]` after `FUN_000b7e57`, to pin the exact step
   for 16000→22050.
3. **`ctx[5]` producer** (the walker's tracked chunk): capture
   `FUN_00095cb3`'s context for the frame/companion streams at a `-3`
   skip if one is ever injected; retail assets contain none.
4. **`VID_HIPP.TGV`** is the one timing outlier (0.856); check whether
   its audio is truncated or its frame count includes a stale frame.
5. **`0x563e0` behaviour after companion EOF** (no further audio reads,
   audio clock frozen) — confirm with a capture at movie end; the video
   branch/`FUN_000949f8` exit predicate is not fully traced.
6. **`FUN_000b7f62` callers** are only `FUN_000b6451`/`FUN_000b6960` per
   Ghidra (FU-35 counted eleven game sites); the wrapper chain between
   them and the 0xB59xx sites is not traced.

## Provenance (Ghidra calls, 2026-10-04, `/fifa96_le.bin`)

* Decompiled: `FUN_000b7f62`, `FUN_000b7fe8`, `FUN_000b81f0`,
  `FUN_000b80fa/811b/8155/817b/81a0`, `FUN_000b9f82`, `FUN_000b7e57`,
  `FUN_000b8741`, `FUN_000b86c8`, `FUN_000b9fdd`, `FUN_000ba00e`,
  `FUN_000a6579`, `FUN_000a662c`, `FUN_000a7ce4`, `eacs_dequeue_next`
  (`0xA7BD7`), `FUN_00095cb3`, `vgt_stream_poll` (`0x67BA8`),
  `FUN_000cb2a4/aa/b6/d1/e1/ef/ff`, `FUN_000cb2b6` region, `FUN_000679f4`,
  `FUN_0009f754`, `FUN_00095eb8/95ed4/95dd2/94eff`.
* Disassembled: `0xB7F62`, `0xB7E57`, `0xB7FE8`, `FUN_000b86C8`,
  readers `0xB8AE1`, `0xB8E8F`, `0xB929D`, `0xB96A8`, `0xB9A46`;
  converters `0xB9E53`, `0xB9EB4`, `0xB9F11`, `0xB9F7D..0xB9F81`;
  producers `0xB82E3`, `0xB832D`, `0xB8372`, `0xB83F1`, `0xB84FE`,
  `0xB8610`; nibble decoders `0xC4CC4`, `0xC4D6C`; `0xA7BD7`; `0x9F5E4`
  (ISR); mixer init call `0xB58F0..0xB5928`; `FUN_000b7fe8` call site in
  `FUN_000a6579`.
* Memory: `0x1406D8` (voice table), `0x115FD8` (volume table, zero),
  `0x136698` (accumulators, zero), `0xB6B88` (pitch table),
  `0x406D8`/`0x9F5E4` raw bytes.
* Xrefs/callers: `0x406D8`, `0x406A4`, `0x406A0`, `0x406D4`, `0x12E88`,
  `0x563C4/C8/D0/D4/DC/E4/F0/F4`, `0xA7BD7` (2 callers), `0xB7E57`
  (5 callers), `vgt_stream_poll` (`FUN_00068108/68194/6847c`).
* Search: operand `12e88`, `406d8`, `2e9c`, `8f5e4`; byte pattern
  `a1 98 7c 04 00 8b 40 08 c1 f8 10`.
* Assets: `/VIDEO/*.TGV` (73) extracted to `/tmp/opencode/fu37/tgv`;
  `tests/golden/eacs/bank-h.eacs` (EACS header chunk: 16000, 2/2/0,
  voice 7, declared 44032, data at +0x20 = 1048 blocks) checked.
