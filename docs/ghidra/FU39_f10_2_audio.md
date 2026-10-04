# FU-39 — f10==2 audio: the adaptive-delta nibble path

Date: 2026-10-04. Program `/fifa96_le.bin` (flat LE link image, base 0).
Scope: the f10==2 (nibble-packed) EACS playback leg that
`fifa96_mixer_render` leaves `UNSUPPORTED`: the 20-byte block framing, the
decoder input unit and nibble packing, the value mapping, the 4x sample
accounting, and the interaction with the 32.32 resampler and the ordinary
format readers. This is a transcription slice plus structural validation;
no code is ported. Companion slices: FU-35 (EACS header/queue), FU-37
(mixer/voices/timing), FU-4 (LE image).

Everything below cites image addresses. The LE fixup rule of FU-37 §0
applies: callback constants stored by `FUN_000b81f0` are object-relative
and need `+0x10000` to land on the coherent code (e.g. stored `0xb4cc4`
→ `0xC4CC4`; image `0xB4CC4` itself is in unrelated `FUN_000b4870`).
Code operands to data globals likewise relocate object 4 (`0x38698` →
runtime `0x138698`, tables `0x41668`/`0x42ca8` → `0x141668`/`0x142ca8`).

## 1. Selection: producer, decoder, reader

`FUN_000b81f0` (disasm 0xB81F0..0xB82E2) picks the three per-voice
callbacks. Relevant branches:

* Reader `[ch+0x5c]` by `flags & 0xC` (`[ch+4]`): `0xA929D`→0xB929D /
  `0xA9A46`→0xB9A46 for `0xC` (f8=2,f9=2), `0xA8E8F`→0xB8E8F for `4`
  (f8=1,f9=2), `0xA8AE1`→0xB8AE1 / `0xA96A8`→0xB96A8 for `8` (f8=2,f9=1)
  (0xB81FF..0xB8257). The f10==2 readers are the **same** functions the
  f10==0 path uses.
* Decoder `[ch+0x58]`, only when the second-cursor byte `[ch+2] != 0`
  (f10==2, set in `FUN_000b7fe8`): `TEST [ch+4],4` (f9==2) selects stored
  `0xB4D6C` (→0xC4D6C), else stored `0xB4CC4` (→0xC4CC4)
  (0xB825A..0xB8278).
* Producer `[ch+0x60]` by `[ch+1]` (loop) and `flags & 0x12`:
  * no loop, `flags&0x12 == 0x12` (f10==2 **and** voice≥0/signed) →
    stored `0xA84FE` (→0xB84FE) (0xB8298..0xB82C4): the video case;
  * no loop, `== 2` (f10==2, voice<0) → `0xA8610` (→0xB8610): bank case;
  * no loop, `== 0x10` → `0xA8372` (→0xB8372): f10==0;
  * other → `0xA82E3`;
  * loop set: `== 2` → `0xA83F1`, else `0xA832D`.

All 11 retail f10==2 videos have (f8,f9)=(2,2), voice 7 or 0 — both
`>= 0`, hence signed — and no loops, so they run:
producer `0xB84FE`, decoder `0xC4D6C`, reader `0xB929D`.

Pipeline (f10==2, signed, no loop):

```
1SNd payload p (20-byte block header + data)
  producer 0xB84FE: decode p[0x14 : 0x14+count] into 0x138698 staging
  producer sets main cursor ch+0x14 = 0x138698 >> ch+8
  mixer FUN_000b7f62: n = (*ch+0x60)() ; (*ch+0x5c)() mixes n frames
```

The producer's return convention is worth stating because it is split
across the call: `FUN_000b7f62` calls `[ch+0x60]` with no argument
(0xB7FAE `CALL dword [ESI+0x60]`); the producer pushes its frame count as
the decoder's seventh (unused) argument and recovers it with `POP EAX`
after the decoder returns, so EAX = n frames to mix
(0xB85A5, 0xB85F3..0xB860F).

## 2. Chunk framing and packing

### 2.1 The 20-byte block header

In the signed/video case every f10==2 data region starts with a 20-byte
header (the arm consumes the first one; §2.2). The producer parses it on
each dequeue (0xB8546..0xB8580):

```
u32 [0]  = count      -> ch+0x4c   (data unit count)
u32 [4]  = L row code -> ch+0x30 low 16  << 6
u32 [8]  = R row code -> ch+0x30 high 16 << 6
u32 [0xc]= L accumulator (s16) -> ch+0x34 low 16
u32 [0x10]=R accumulator (s16) -> ch+0x34 high 16
data at +0x14
```

Cited: `MOV EDI,[ESI+0x38]; MOV [ESI+0x40],EDI; ADD [ESI+0x40],0x14`
(0xB8548..0xB854E); `MOV EAX,[EDI]; MOV [ESI+0x4c],EAX` (0xB8552..0xB8554);
`MOV EBX,[EDI+4]; SHL EBX,6; MOV EAX,[EDI+8]; SHL EAX,0x16; OR EBX,EAX;
MOV [ESI+0x30],EBX` (0xB855E..0xB856C); `MOV EBX,[EDI+0xc]; MOV EAX,[EDI+0x10];
SHL EAX,0x10; AND EBX,0xffff; OR EBX,EAX; MOV [ESI+0x34],EBX`
(0xB856F..0xB8580). The producer rewinds `ch+0x38 = 0` per chunk, so the
cursor is chunk-relative; `[ch+0x40]` is the current data base.

### 2.2 Arm path (first block)

`FUN_000b7fe8` initializes the second cursor only when `flags&2`
(0xB806C..0xB8073). For signed voices (`[ch+3] != 0`, `flags&0x10`) it
consumes the first block header itself (0xB80AD..0xB80C5):

```
ch+0x40 = data + 0x14        ; data = *(header+0x18) = EACS payload+0x20
ch+0x4c = u32[data] - 1      ; arming end (see §4.3)
ch+0x30 = ch+0x34 = 0        ; stored header state is ignored
```

For unsigned voices (banks) the signed branch is skipped and the values
set just before it stand: `ch+0x4c = blocks` (arg+0x10, 0xB809D..0xB80A0)
and `ch+0x40 = data` (arg+0xc, 0xB80A7) — the bank path (FU-35 §6.5).

### 2.3 Decoder input units

The cursor `ch+0x38` counts decoder-input units, and `count` from the
block header is in those units. There are two decoders:

* **f9==2, `FUN_000c4d6c`** (video): one input **byte = one stereo frame**.
  Each byte is consumed once; the high nibble updates the L lane and
  writes `word [dest + i*4]`, the low nibble updates the R lane and writes
  `word [dest + i*4 + 2]` (0xC4DB0..0xC4DB5, 0xC4DDF; 0xC4E0C..0xC4E11,
  0xC4E3B). Source advances once per byte (0xC4E64 `INC ESI`). An f8=2,
  f9=2 block is 4 bytes → 4 stereo frames = `DAT_000149ec = 4` units.
* **f9!=2, `FUN_000c4cc4`** (bank): one input **nibble = one mono sample**.
  Even cursor position takes the high nibble (`SHR BL,4`), odd takes the
  low (`AND BL,0xf`) and then advances the source byte
  (0xC4CED..0xC4CFD). One 16-bit word is written per nibble with `EDI += 2`
  (0xC4D15..0xC4D18). An f8=2, f9=1 block is 2 bytes → 4 nibbles = 4 units.

Nibbles are therefore **MSB-first**; in stereo the MSB nibble of a byte is
the **L** channel. The producer converts the unit cursor to a byte pointer
only for the mono case (0xB85DB..0xB85F1): for `flags&4 == 0` it passes
`src = ch+0x40 + (ch+0x38 >> 1)` and `arg1 = ch+0x38 & 1` (start parity),
otherwise `src = ch+0x40 + ch+0x38` and `arg1` is don't-care
(`FUN_000c4d6c` never reads `[EBP+8]`).

## 3. Value mapping: adaptive delta, not linear nibbles

Each unit decodes one nibble `n` into the running accumulator:

```
sample = clamp16(sample + DELTA[row/4 + n])   ; row is a byte offset, /4 = row index
row    = clamp(row + ADAPT[n & 7], 0, 0x1600)
```

Cited (stereo, 0xC4DC0..0xC4E5C; mono, 0xC4CFE..0xC4D32): `ADD EDX,
[EAX + EBX*4 + 0x41668]`, `CMP EDX,0x7fff/JG -> 0x7fff`,
`CMP EDX,-0x8000/JL -> 0x8000`, `ADD EAX,[EBX*4 + 0x42ca8]`,
`CMP EAX,0x1600/JG -> 0x1600`, `CMP EAX,0/JL -> 0`. The accumulator is
persisted as 16-bit per lane (`ch+0x34`), the row as a 16-bit byte offset
(`ch+0x30`).

Table facts (`read_memory 0x141668`, `0x142ca8`):

* `DELTA` = 89 rows × 16 int32 at `0x141668`, `0x1640` bytes exactly up to
  the adapt table. Row `r` holds the deltas for nibble values `0..15`.
* Nibble bit 3 is the **sign**: all 712 pairs satisfy
  `DELTA[r][n+8] == -DELTA[r][n]`. Row 0 is
  `(0,1,3,4,7,8,10,11, 0,-1,-3,-4,-7,-8,-10,-11)`; the largest row is
  `(4095,12286,20478,28669,36862,45053,53245,61436, ...)` — magnitudes
  exceed int16, so the clamp can bite.
* `ADAPT` = `(-64,-64,-64,-64, 128,256,384,512)`: nibble magnitude `n&7`
  in 0..3 shrinks the step, 4..7 grows it. Rows are byte offsets into the
  delta table (stride `0x40`), so the state is `row_index << 6`.
* The 8-bit volume table at `0x115FD8` (FU-37 §A.3) plays no role here:
  f10==2 nibbles are deltas, not volume-scaled bytes; output is signed
  16-bit and scaled by the reader exactly like f10==0 PCM.

The decoder's state is re-initialized from the stored block header at every
dequeue; the running value need not match the stored one at runtime, but it
does (see §6), which proves the arithmetic.

## 4. The 4x accounting and the 32.32 resampler

### 4.1 Where 4 comes from

`FUN_000a79dc` sets `DAT_000149ec = 4` when payload[0xA] == 2
(0xA7AD4..0xA7AE5, FU-35) and `FUN_000a7e31` applies it to block counts
(selector 1: `sum / block_size * DAT_000149ec`). This is not a stereo
multiplier nor padding: a block is `f8*f9` bytes and holds exactly **four
decoder units** in both f10==2 layouts — 4 bytes/4 stereo frames (f9=2) or
2 bytes/4 nibbles/4 mono samples (f9=1). For f9=2 the apparent "2 nibbles
per byte" is the L/R pair of one 16000 Hz frame, not two time samples;
FU-37's 2× duration reading came from counting nibbles as mono samples.
For f9=1 one nibble really is one sample, and the declared count is
`2 * data_bytes`, as FU-35 measured on the banks.

### 4.2 Step and count arithmetic

The step is the FU-37 8.24 value at `ch+0x2c` (`FUN_000b86c8`). The
producer (0xB84FE) computes, per call:

```
left        = max(0, ch+0x4c - ch+0x38)
frames_left = (left << 24) / ch+0x2c            ; 0xB84FE..0xB8520
n           = min(frames_left, [0x406b0])       ; 0xB8593..0xB85A0
count       = ceil(n * step + ch+0x3c / 2^32)   ; 0xB85AE..0xB85C9
ch+0x3c     = frac(n * step + ch+0x3c / 2^32)   ; second-cursor fraction
src, arg1   = unit -> byte conversion (above)   ; 0xB85DB..0xB85F1
decoder(arg1, src, 0x38698, count, &ch+0x34, &ch+0x30)   ; 0xB85F3
ch+0x38    += count                              ; 0xB85F9..0xB85FE
ch+0x14     = 0x38698 >> ch+8                    ; 0xB8601..0xB860B
return n
```

The count computation is the `MUL [ch+0x2c]; SHLD EDX,EAX,8; SHL EAX,8;
ADD [ch+0x3c],EAX; ADC EDX,0; CMP [ch+0x3c],0; JZ; INC EDX` sequence
(0xB85AE..0xB85C9) — a 64-bit fixed-point accumulate with round-up. Both
cursors advance by the same step: the second cursor consumes source units
at `step` units per output frame, and the ordinary reader walks the
decoded staging with its own 32.32 cursor (`ch+0x14`/`ch+0x18`) at the same
`step`. This is the interaction FU-37 flagged: the nibble path is
**decoded through the normal readers** (0xB929D / 0xB8AE1) — the producer
just refills staging and re-points the main cursor each call; there is no
separate mixing path.

### 4.3 Declared vs played length

The arm sets `ch+0x4c = u32[data] - 1` (0xB80B7..0xB80BC), so the
engine's nominal stream is one unit shorter than the first block declares:
for all 11 f10==2 videos, `d0 + Σ count == declared` exactly while the
arm-truncated model `(d0 - 1) + Σ count` is `declared - 1` (the per-call
cursor rounding in §4.2 is a runtime detail, but the block-header counts
are the authoritative length). `FUN_000a79dc`'s
`samples = blocks * 4` and `FUN_000a7e31`'s position formula instead count
the per-chunk 20-byte block headers as data (and round down), which is why
FU-35's `blocks*4` model ran ~1.8% high on VID_INTR (1024696 vs 1006616).
The authoritative length is the sum of the per-chunk block-header counts.

## 5. Queue: why the decoder never sees the `1SNh` bytes

`eacs_dequeue_next` (`0xA7BD7`) hands out the **successor** of the head
node, not the head (disasm 0xA7C2D..0xA7C56):

```
MOV EBX,EAX          ; EAX = [0x5db9c] = head node address
MOV EAX,[EAX]        ; EAX = head->link = successor
ADD EAX,8            ; out1 = successor + 8   (payload)
MOV [EDX],EAX
MOV EAX,[EBX]; MOV EAX,[EAX+4]; SUB EAX,8
MOV ESI,[0x149d4]; CDQ; IDIV ESI
MOV [ECX],EAX        ; out2 = (succ_len-8)/block
INC [0x149e8]; MOV [0x5db9c],EAX   ; head := successor
MOV [EBX],0xfffffffe               ; old head marked consumed
```

With the `1SNh` as the first queued node (FUN_000a79dc append
0xA7AEF..0xA7B1D), the first dequeue marks the `1SNh` consumed and returns
the **first `1SNd` payload**. That is why the arm can play the EACS header
chunk's data directly (§2.2) and why neither the f10==0 nor the f10==2
producer ever mixes the `"EACS"` header bytes as PCM: the header node is
marked consumed and skipped, not handed to the producer.
The head==tail branch returns the last node itself (`head+8`, 0xA7C13) or,
for the `1SNe` end marker (`[head] == -1`), `(0,0)` and sets
`[0x149cc] = 1` (0xA7BF4..0xA7C09).

## 6. Structural validation (throwaway, not committed)

Assets: `tools/fifa96_bind.iso_files` on the read-only
`game/FIFAPCCD96.iso` → `/tmp/opencode/fu39/tgv` (73 `VIDEO/*.TGV`) and
`FIFAPCCD96.EXE`; flat image rebuilt with `tools/fifa96_le.py -o
/tmp/opencode/fu39/fifa96_le.bin` (1485392 bytes; table bytes match the
Ghidra reads at `0x141668`/`0x142ca8`). Probe:
`/tmp/opencode/fu39/fu39_probe.py`. Decode = §2.3/§3 exactly; the chain
feeds each block header's stored state into the next block and compares it
with the running state.

```
$ python3 /tmp/opencode/fu39/fu39_probe.py
file              d0 chunks   mism declared   encoder    engine blocks*4  zeroR
VID_CH01.TGV    1063    310      0   330752    330752    330751   336516    833
VID_CH02.TGV    1063    308      0   328704    328704    328703   334668   1446
VID_CH03.TGV    1063    308      0   328704    328704    328703   334668   1453
VID_CRED.TGV    1065    898      0   956416    956416    956415   973448   8279
VID_CTDN.TGV    1064    174      0   186368    186368    186367   189700   1022
VID_DEEP.TGV    1065    570      0   607232    607232    607231   618232    232
VID_FEAT.TGV    1064    534      0   569344    569344    569343   579940    775
VID_GAME.TGV    1064    582      0   620544    620544    620543   631972    726
VID_INTR.TGV    1065    945      0  1006616   1006616   1006615  1024696   8383
VID_NGEN.TGV    1065    471      0   501760    501760    501759   510480    462
VID_STPB.TGV    1064    600      0   640000    640000    639999   651484      1
== files=11 chunks=5700 state_mismatches=0
```

`d0` = first block count; `declared` = EACS +0x0C; `encoder` = d0 + Σ
1SNd counts; `engine` = d0-1 + Σ counts; `blocks*4` = FU-35's model;
`zeroR` = longest run of L==0 frames inside the 1SNd blocks.

Decisive results:

* `declared == d0 + Σ counts` in **11/11** files (5700 `1SNd` chunks; every
  payload satisfies `u32[0] == payload_len - 0x14`).
* Decoder state continuity: the block header of every chunk equals the
  running `(row,acc)` state after the previous chunk — **0 mismatches in
  5700/5700**. A wrong nibble order, delta table, adapt table or clamp
  would desynchronize within one block and never resync. All stored rows
  are multiples of 64 within `[0, 0x1600]`, as the `index<<6` encoding and
  the `n&7` adaptation steps require.
* The first blocks store zero state (asserted), matching the arm's
  zeroing; decoding the full `d0` reproduces the second chunk's state.
* Values stay in int16 (clamped by construction; observed min/max -32768 /
  +32767), and the corpus contains musically plausible silence
  (VID_INTR: 8383-frame zero run, ~0.52 s; CRED 8279).
* Bank cross-check (`f9=1`, mono nibble decoder, `TEM_T519.VIV` 6/6,
  `TEM_T169.VIV` 2/6 exact): `declared == 2 * (entry_size - 0x20) ==
  nibble count`. T169's other four entries declare 2–6 fewer nibbles than
  the payload holds (FU-35's "rest differ by 2-6"), consistent with
  trailing slack rather than a decode fault. First decoded samples are
  ~±11 and final values are small against full scale; no state chain exists
  to test because the unsigned path skips the block header (§2.2).

Residual uncertainty is only runtime behaviour (which bytes are actually
mixed per tick), not the offline decode; capture targets are listed in §7.

## 7. Open legs / capture targets

1. **Runtime confirmation of the staging flow**: at `0xB85F3` (before the
   decoder call) and `0xB8601` (after), break with `ESI` = channel struct;
   dump `ch+0x14/0x30/0x34/0x38/0x3c/0x40/0x4c`, `[0x406c8]`, and
   `0x138698` for `[0x406c8]*4` bytes. Compare against the probe's
   per-call state; this also confirms `count` and the arm `-1`.
2. **Arm one-unit truncation** (§4.3): audible only as one dropped 16-bit
   frame at stream start; capture `ch+0x4c` at first producer entry to
   confirm `u32[data]-1`.
3. **Bank arm mapping** (§2.2, FU-35 §6.5): `FUN_000ba41d` passes the
   header whose `+0x18` is the `0x20` offset; trace the in-memory data
   pointer/end for the unsigned producer `0xB8610`.
4. **`1SNe` end-of-stream**: the producer's `out1 == 0` stop path
   (`FUN_000b80fa`, 0xB858A..0xB8591 / 0xB863C..0xB864F) is not exercised
   offline; capture the last dequeue.
5. **Delta-table origin**: the 89×16 table has no static xref (stored
   operand `0x41668`); if the port needs the table committed, its
   provenance is the LE image object 4 at `0x141668` (this doc's bytes).

## 8. Provenance (Ghidra calls, 2026-10-04, `/fifa96_le.bin`)

* Decompiled: `FUN_000b84fe` (created at `0xB84FE`), `FUN_000b8610`
  (created), `FUN_000c4d6c`, `FUN_000c4cc4` (created), `FUN_000a6579`,
  `FUN_000a79dc`, `FUN_000a7d3c`, `FUN_000a7ca4`, `FUN_000a7ce4`,
  `eacs_dequeue_next` (`0xA7BD7`), `FUN_000a7e31`, `FUN_000a7b2b`,
  `FUN_000b7f62`, `FUN_000b7fe8`, `vgt_stream_poll`, `FUN_000679f4`.
* Disassembled: `FUN_000b81f0`, `FUN_000b7f62`, `FUN_000b7fe8`,
  `0xB8372..0xB86C7` (producer family, raw), `0xA7BD7..0xA7C5F`
  (dequeue), `FUN_000c4d6c`, `0xC4CC4..0xC4D6A` (raw), `0xA7AE0..0xA7B2F`.
* Memory: `0x141668` (delta table head; full table verified in the probe),
  `0x142ca8` (adapt table + state slots).
* Search: operands `38698` (6 hits), `42ca8`, `42cd0`, `41668`, `4cc4`,
  `4d6c`; xrefs `0xA7CA4`, `0xA7CE4`.
* Assets: `/tmp/opencode/fu39/tgv` (73 TGV), `TEM_T519.VIV`/`TEM_T169.VIV`,
  `fifa96_le.bin`; probe `fu39_probe.py` (throwaway, not committed).
* Port anchors: `fifa96_eacs.h` (`samples = blocks*4` is the nominal
  formula only; the authoritative count is the per-chunk block header),
  `fifa96_mixer.h` (f10==2 UNSUPPORTED → producer 0xB84FE, decoders
  0xC4CC4/0xC4D6C, staging 0x138698, tables 0x141668/0x142ca8).
