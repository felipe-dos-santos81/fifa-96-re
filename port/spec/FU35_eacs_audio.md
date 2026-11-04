# FU-35 — EACS audio: the `1SNh`/`1SNd` companion chunks

Date: 2026-10-04. Program `/fifa96_le.bin` (flat link image, base 0).
Scope: the audio subsystem behind the `VIDEO/*.TGV` companion chunks —
tag roles, the EACS payload header, the sample coding that the mixer
proves, and the queue/rate path into the sound library (EACSNDF.LIB).
This is a transcription slice; no ported code.

## 1. Tag and chunk roles

The TGV container is `[u32 tag][u32 len][len-8 payload]` (FU-29/FU-30).
`vgt_stream_poll` (0x67BA8) reads a second, companion stream
(`PTR_DAT_000563f4`) next to the frame stream and passes each chunk to
`FUN_000a7d3c` (decompile; call site 0x67D57 per xrefs):

```
DAT_000563d8 = FUN_00095cb3(PTR_DAT_000563f4);      // next chunk
...
lVar6 = FUN_000a7d3c(...);                          // route it
lVar6 = FUN_000a7e31(0, ...);                       // queued byte count
if (0x20000 < lVar6) FUN_000a7ce4(...);             // start when >128 KiB
```

`FUN_000a7d3c` (chunk tag in `*in_EAX`) dispatches:

| tag | bytes | action |
|---|---|---|
| `0x644E5331` | `1SNd` | if `DAT_000149cc != 0`: `*chunk = 0`, append to the queue (head `PTR_DAT_0005db9c`, tail `PTR_DAT_0005db98`, link at `*chunk`); else return `-1` |
| `0x684E5331` | `1SNh` | `FUN_000a79dc` — EACS header parse (below); on success the chunk is appended to the same queue too (0xA7AF8..0xA7B1D) |
| `0x654E5331` | `1SNe` | `*chunk = -1`, then `FUN_000a7b2b` queues it as the end marker |
| `0x6C4E5331` | `1SNl` | `FUN_00094eff(PTR_LAB_000149e0, chunk[2]); FUN_00095dd2(ctx, chunk)` — payload dword 0 seeks the companion stream, then the chunk is released (tag `-2`). No retail `.TGV` contains one (73/73 scanned) |

Stream helpers: `FUN_00095cb3` returns the next chunk pointer and handles
the `-1` rewind and `-3` skip sentinels (`param[4] = *param` on rewind,
marks `-2` and returns `-1` on skip); `FUN_00095dd2` writes `0xfffffffe`
(released); `FUN_00094eff` sets `ctx[0x48] = ctx[0x38] + arg`,
`ctx[0x1C] = 2` (a seek).

Queue bookkeeping:

* `FUN_000a7b2b` appends an end node exactly like the `1SNd` path.
* `FUN_000a7e06` flushes: walks the list from the tail and writes
  `0xfffffffe` into every node.
* `eacs_dequeue_next` (0xA7BD7, unnamed in Ghidra; created for this
  slice) hands out the next buffer: if head != tail it advances the head
  and returns `(chunk+8, (len-8)/DAT_000149d4)` through its two stack
  out-params, increments `DAT_000149e8` (chunk counter), and marks the
  consumed node `-2`; when only the `-1` node remains it returns
  `(0,0)` and sets `DAT_000149cc = 1` (start). It has no static caller —
  it must be reached through a runtime pointer (open leg, §6).
* `FUN_000a7e31` reports stream position by summing `chunk[1]` over the
  queue: selector 0 = bytes, 1 = `blocks * DAT_000149ec`, 2 =
  `(blocks << 12) / (rate >> 4) * DAT_000149ec` (1/65536 s units).
* `FUN_000a7ebc` returns the chunk counter `DAT_000149e8`; `FUN_000a7df8`
  returns `DAT_000149cc != 0` ("audio active").

State machine `DAT_000149cc`: `FUN_000a7ca4` resets it to 3 (0x7CAE in
decompile) with `DAT_000149d0 = -1` and an empty queue;
`FUN_000a7ce4` requires state 3, then `FUN_000b9fdd(voice)` +
`FUN_000a6579(header)` and sets state 2 (`0xA7D0E..0xA7D2C`);
`eacs_dequeue_next` sets state 1 at the `1SNe` marker;
`FUN_000a6cdc` sets state 0 and flushes the queue (`0xA6D3A..0xA6D4E`).

## 2. EACS header (`1SNh` payload, 32 bytes)

`FUN_000a79dc` checks payload+0 == `0x53434145` ("EACS", 0xA79E9) and
reads `EBX = chunk+8` as the payload base. Offsets are payload-relative;
each row cites the instruction(s).

| off | width | meaning | citation |
|---|---|---|---|
| 0x00 | char[4] | `"EACS"` | `CMP [EBX],0x53434145` 0xA79E9 |
| 0x04 | u32 | sample rate (Hz; 16000 on 72/73 videos, 16384 once) | `MOV EAX,[EBX+4]` 0xA7ABA (stored `DAT_000149d8`) |
| 0x08 | u8 | format byte f8 (1 or 2 observed) | `MOVZX ESI,[EBX+8]` 0xA7A80 |
| 0x09 | u8 | format byte f9 (2 on all videos, 1 on banks) | `MOVZX EAX,[EBX+9]` 0xA7A7C |
| 0x0A | u8 | format byte f10 (0 or 2) | `MOVZX EAX,[EBX+0xA]` 0xA7AC9 |
| 0x0B | s8 | voice index, required 0..15 (rejects `<0` or `>15`) | `MOVSX EAX,[EBX+0xB]` 0xA7A05; bounds 0xA7A14..0xA7A1E |
| 0x0C | u32 | declared stream length in samples (video: whole cue); the parser overwrites it with this chunk's block count | `MOV EAX,[ECX+4]; SUB EAX,0x28; DIV ESI; MOV [EBX+0xC],EAX` 0xA7A8F..0xA7AA9 |
| 0x10 | s32 | loop start in blocks (inferred: channel+0x1C = cursor+field); parser force-writes `-1` | `MOV [EBX+0x10],0xffffffff` 0xA7ABD |
| 0x14 | u32 | loop length in blocks (inferred: channel+0x20 = +field); parser force-writes 0; nonzero arms the looping producer callbacks in `FUN_000b7fe8` | `MOV [EBX+0x14],0` 0xA7ACD; `[EBP+0x18]` → channel+1 at 0xB800D..0xB8013 |
| 0x18 | u32 | data pointer; parser writes `chunk+0x28` (payload+0x20) | `MOV EAX,ECX; ADD EAX,0x28; MOV [EBX+0x18],EAX` 0xA7A87..0xA7A8C |
| 0x1C | u8 | voice copy; `FUN_000a6579` writes payload[0x0B] here before use | `MOV [EBX+0x1C],AL` 0xA65D0 in `FUN_000a6579` 0xA65C3..0xA65D0 |
| 0x1D | u8 | per-voice volume (0x7F in videos); stored to voice+0x25 | `MOV DL,[EBX+0x1D]` 0xA7A34, `MOV [EAX+0x25],DL` 0xA7A6A |
| 0x1E,0x1F | u8 | unread by the `1SNh` path (0x7F 0x7F in videos) | — |

Derived parse (0xA7A7C..0xA7A99):

```
block_size = payload[0x08] * payload[0x09]          // IMUL ESI,EAX
blocks     = (chunk_len - 0x28) / block_size        // DIV ESI
DAT_000149d4 = block_size
DAT_000149e8 = 0
DAT_000149e4 = payload                             // header ptr
DAT_000149d0 = voice
DAT_000149d8 = rate
DAT_000149ec = (payload[0x0A] == 2) ? 4 : 1         // 0xA7AD4..0xA7AE5
```

The same 32-byte header starts every `SOUND/*.VIV` entry (see §5), but
the library API `FUN_000ba41d` (which validates `*EAX == 'EACS'` then
calls `FUN_000a6579`) takes the voice as an argument and copies
payload[8]→voice+0x1D, payload[9]→voice+0x1E, i.e. the bank path reads
the same bytes with different intent. The bank header's +0x18 is `0x20`
(data offset) while the video parser writes an absolute pointer —
flagged in §6.

## 3. Coding

### 3.1 Block model

`samples = blocks * DAT_000149ec`, with `DAT_000149ec = 4` when
f10 == 2 and 1 otherwise (`FUN_000a7e31` applies the multiplier). Every
audio byte of a cue belongs to a block of `f8*f9` bytes; 1SNd payloads
are `block_size`-aligned (all lengths observed: `0x1068/0x106C` =
1048/1049 blocks of 4 B; bank `spc` entries: `size-0x20` bytes of 2-byte
blocks).

`FUN_000a6579` turns the format bytes into mixer flags
(disasm 0xA6582..0xA65D0):

```
flags = 0
if f8 == 2: flags |= 8        // one address-unit shift; f8*f9 = cursor unit
if f9 == 2: flags |= 4        // second shift when both set (block = 4 B)
if f10 == 2: flags |= 2       // second (stereo) cursor in FUN_000b7fe8
if voice >= 0: flags |= 0x10  // signed samples; also copies voice to +0x1C
```

`FUN_000b7fe8` (channel arm; disasm 0xB7FEF..0xB80F9) shifts the buffer
cursor by one for each of flags 8/4 (`SHR EDX,1` 0xB8034/0xB8042,
`INC [ESI+8]`), so the source pointer advances in units of
`f8*f9` bytes; flag 2 initializes the second cursor at channel+0x40..0x4C;
flag 0x10 marks the stream signed (channel+3). `FUN_000b81f0` then picks
the per-format callbacks: `[channel+0x5c]` (mix) by `flags & 0xC`, and
`[channel+0x60]` (produce/advance) by loop flag and `flags & 0x12`.

**Fixup caveat.** The callback constants stored by `FUN_000b81f0`
(disasm 0xB8214..0xB82E2) are `0xA8xxx`/`0xB4xxx`; object 1's LE base is
`0x10000` (FU-4), and the flat rebuild leaves those fixup sources
unrelocated. Adding `0x10000` lands on coherent mixer code, while the raw
values land inside string/path utilities. All addresses below are the
relocated ones.

| `flags & 0xC` | condition | callback (relocated) |
|---|---|---|
| 0xC | f8=2,f9=2, `[0x4069C]!=1` | 0xB929D |
| 0xC | f8=2,f9=2, `[0x4069C]==1` | 0xB9A46 |
| 4 | f8=1,f9=2 | 0xB8E8F |
| 8 | f8=2,f9=1, `[0x4069C]!=1` | 0xB8AE1 |
| 8 | f8=2,f9=1, `[0x4069C]==1` | 0xB96A8 |
| 0 | other | 0xB879D |

### 3.2 Proven PCM layouts

**16-bit stereo (f8=2, f9=2 — 29 videos f10=0, 11 f10=2).** Mix
callback 0xB929D (disasm):

```
cursor = channel+0x14                       // in 4-byte units
volL = channel+0x64 >> 10; volR = channel+0x68 >> 10
while ([0x406b8] bytes left):
    l = (int16) [cursor*4 + 0]              // 0xB92DB MOVSX word [EDI*4]
    r = (int16) [cursor*4 + 2]              // 0xB92F0 MOVSX word [EDI*4+2]
    accumL += (l * volL) >> 7               // 0xB92E3..0xB92E9
    accumR += (r * volR) >> 7               // 0xB92F8..0xB92FE
    cursor += fixed_step                    // channel+0x24/+0x28 via 0x406CC/0x406D0
```

So the stream is little-endian, **signed 16-bit, L/R interleaved by
frame** (4 bytes/frame). The `[0x4069C]==1` variant (0xB9A46) reads only
the high bytes (`[EDI*4+1]`, `[EDI*4+3]`) through a 128x256 volume table
at `0x15FD8` — an 8-bit-quality truncation of the same 16-bit source.
`FUN_000b86c8` builds the fixed-point cursor step from the header rate
and the output-mix rate (`PTR_PTR_000406a0`).

**8-bit stereo (f8=1, f9=2 — 33 videos).** Callback 0xB8E8F reads
`[EDI*2+0]` (left) and `[EDI*2+1]` (right) per 2-byte frame and scales
each byte through the volume table at `0x15FD8`. Layout is proven;
table polarity (signed vs unsigned) is not (open leg — that table's
address/relocation is inconsistent in the flat image; the raw bytes at
0x15FD8/0x25FD8 disassemble as code).

**Banks (f8=2, f9=1, f10=2).** Callback 0xB8AE1 reads one `MOVSX word
[EDI*2]` (signed 16-bit sample) per 2-byte block; the `[0x4069C]==1`
variant 0xB96A8 reads byte `[EDI*2+1]` through the same table. The
declared count is 2 samples per byte though (see §5), so the f10=2
sample accounting is not the plain one-block-one-sample model — the exact
arithmetic stays open, with these functions as the capture targets.

## 4. Queue → rate → output path

```
vgt_stream_poll (companion stream)
  FUN_00095cb3  -> next chunk
  FUN_000a7d3c  -> 1SNh parse + queue ; 1SNd queue ; 1SNe end marker
  FUN_000a7e31(0) > 0x20000  -> FUN_000a7ce4 (state 3 -> 2)
        FUN_000a6579(header):
            flags = f8/f9/f10/voice            (0xA6582..0xA65D0)
            vol = FUN_000a662c(voice)          (pan/volume -> L,R)
            FUN_000b7fe8(voice, header+0x18, header+0x0C, header+0x10,
                         header+0x14, header+4, L, R, flags, gain)
                fill DAT_000406d8[voice] channel struct
                FUN_000b86c8  rate -> fixed-point cursor step
                FUN_000b81f0  pick 0xB8/0xB9 callbacks
  mixer tick (game frame; FUN_000b7f62):
       for each active channel: (*[ch+0x60])() produce
                                (*[ch+0x5c])() mix into accumulators
```

`FUN_000b7f62` is called from eleven game sites (0xB59AD..0xB6969) and
is the per-buffer mixing loop: it pulls the channel's producer
(`+0x60`) and mixer (`+0x5c`) until `_DAT_000406a4` bytes are produced.
`FUN_000b6ab3` is the per-frame voice tick (fade fields +0x18/+0x1C/+0x20,
calls `FUN_000a6cdc`/`FUN_000a6b21`, and on state 1 stops the voice
`FUN_000b80fa` then flushes via `FUN_000a7e06`). Sample-bank playback
uses the same library through `FUN_00065920`/`FUN_000659F8` (stream
`DAT_0000a134`, `FUN_000a7ce4`/`FUN_000a7d3c`/`FUN_000a7e31` identical).

## 5. Structural validation

Probe `/tmp/opencode/fu35/eacs_probe.py` (throwaway, not committed);
assets extracted with `tools/fifa96_bind.iso_files` from the read-only
ISO. Commands:

```
python3 -c "from tools.fifa96_bind import iso_files; ..."   # extract to /tmp/opencode/fu35
python3 /tmp/opencode/fu35/eacs_probe.py
```

Observed, per the derived spec:

* **VID_BULL.TGV**: 86 chunks = 43 kVGT, 1 `1SNh`, 41 `1SNd`, 1 `1SNe`
  (len 8, empty). Header at 0x169C/len 0x1088: rate 16000, f8=2 f9=2
  f10=0, voice 7, declared count **44032**, volumes 127. Model: header
  data (0x1088-0x28)/4 = 1048 blocks; 1SNd lengths 0x1068 x25 (1048) and
  0x106C x16 (1049) -> 1048 + 42984 = **44032 == declared count exactly**.
  No `1SNd` before the header.
* **16-bit interpretation** of the stream: sample pairs in [-11236,9949],
  mean ~0, small adjacent step; chunk-to-chunk frame delta smooth at
  quiet boundaries (`dL=9,dR=1`), a 1508-step at the header->first-data
  cue attack -> no per-chunk predictor reset; consistent with raw PCM.
* **VID_INTR.TGV**: 1854 chunks = 830 fVGT (`0x54475666`), 77 kVGT,
  1 `1SNh` (rate 16000, f8=f9=2, f10=2, voice 7, declared 1006616),
  945 `1SNd` (len 0x440/0x441/0x445), 1 `1SNe`. The f10=2 model gives
  1024696 samples (+1.8% vs declared); parser overwrites the field, so
  the game never trusts it — recorded as the one count mismatch.
* **Corpus**: all 73 `VIDEO/*.TGV` carry exactly one `1SNh` + one `1SNe`
  and zero `1SNl`; rates 16000 x72 / 16384 x1; (f8,f9,f10) =
  (2,2,0) x29, (2,2,2) x11, (1,2,0) x33; voice 7 or 0.
* **Banks**: `SOUND/TEM_T519.VIV` is `BIGF` + BE32 file size + BE32 count
  + BE32 first-data offset, then 21-byte entries `[BE32 off][BE32 size]
  [12-char name + NUL]` (first entry at 0x10; first data 0x90). All six
  `.spc` entries start with a valid EACS header: rate 16000, f8=2, f9=1,
  f10=2, voice -1 (unsigned by the flags rule), block=2 B, unit=4.
  Declared count == 2 x (size-0x20) for 8/12 entries across two banks
  (the rest differ by 2-6) -> **exactly 2 samples per byte** (4-bit
  samples) in every entry. Nibble histograms are bimodal (mean 6.2-6.6),
  so the packing/decoding arithmetic is left open.

## 6. Open legs

1. **f10==2 sample accounting**: same mixer callback class as PCM but
   `DAT_000149ec=4`, a second channel cursor, and count = 4x blocks.
   Capture: hook `FUN_000b7f62` (mixer loop) or the format callbacks
   `0xB96A8`/`0xB8AE1` (f8=2,f9=1) and `0xB9A46`/`0xB929D` (f8=2,f9=2)
   and dump the input block plus the mixed accumulators; `FUN_000b7fe8`
   logs the channel struct (`DAT_000406d8[voice]`).
2. **8-bit table polarity**: the volume table at `0x15FD8` is unrelocated
   in the flat image (raw bytes there disassemble as code) — needs the
   LE fixup applied, or a capture of `0xB8E8F`.
3. **`eacs_dequeue_next` caller**: no static xref; find the runtime
   pointer/IRQ that pulls `1SNd` buffers (hook 0xA7BD7 while playing).
4. **VID_INTR count mismatch** (1024696 model vs 1006616 declared).
5. **Bank in-memory form**: header+0x18 = 0x20 offset vs video absolute
   pointer; loader (`FUN_000a780e`/`FUN_000a62fa`/`FUN_000ba41d`) not
   fully traced.
6. **`1SNl` payload semantics** (`FUN_00094eff` seek target) — no retail
   sample to validate.

## 7. Provenance

* Ghidra: `search_instructions 0x53434145` (3 hits: 0xA7840, 0xA79E9,
  0xBA427), `search_byte_patterns 45 41 43 53`,
  `search_strings EACS/compression` (EACSNDF.LIB banner 0x1036EC,
  "Error: not an EACS format file" 0x1038DD).
* Decompiled: `FUN_000a7d3c`, `FUN_000a79dc`, `FUN_000a7b2b`,
  `FUN_000a7ca4`, `FUN_000a7ce4`, `FUN_000a7e06`, `FUN_000a7e31`,
  `FUN_000a7ebc`, `FUN_000a7df8`, `FUN_000a6579`, `FUN_000a662c`,
  `FUN_000b7fe8`, `FUN_000b81f0`, `FUN_000b7f62`, `FUN_000b86c8`,
  `FUN_000b80fa`, `FUN_000b9fdd`, `FUN_000ba00e`, `FUN_000ba41d`,
  `FUN_000a6b21`, `FUN_000a6cdc`, `FUN_000a6ead`, `FUN_00094eff`,
  `FUN_00095cb3`, `FUN_00095dd2`, `vgt_stream_poll`, `FUN_00065920`,
  `FUN_000659f8`, `FUN_0006572c`, `FUN_000679f4`, `FUN_000ba555`.
* Disassembled: `FUN_000a79dc`, `FUN_000a6579`, `FUN_000a7ce4`,
  `FUN_000b7fe8`, `FUN_000b81f0`, `FUN_000ba41d`, raw 0xA7BE0..0xA7CA7;
  relocated callbacks 0xB8610, 0xB8AE1, 0xB8E8F, 0xB929D, 0xB96A8,
  0xB9A46 (the `0xB8xxx` disassembly is coherent where the stored
  `0xA8xxx` values land mid-function in unrelated utilities).
* Xrefs: 0x149CC/0x149D0/0x149D4/0x149E4/0x149E8, 0x5DB98/0x5DB9C,
  0x4069C/0x406D8; `create_function eacs_dequeue_next @0xA7BD7`.
* Assets: `/VIDEO/VID_BULL.TGV`, `/VIDEO/VID_INTR.TGV`,
  `/SOUND/TEM_T519.VIV`, `/SOUND/TEM_T169.VIV` extracted to
  `/tmp/opencode/fu35` (read-only ISO; probe not committed).
