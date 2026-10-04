# FU-30 — TGV stream walker (video chunk layer)

Date: 2026-10-04. Program `/fifa96_le.bin`. The `VIDEO/*.TGV` chunk
iteration is ported: `fifa96_tgv_walk_next` walks the length-prefixed
chunk stream that `vgt_stream_poll` (0x67BA8) consumes via
`FUN_00095cb3`, and hands kVGT frames to `fifa96_kvgt_decode` (FU-29).

## 1. Format and citation

Chunk layout (confirmed on `VID_BULL.TGV`, FU-29 errata):

```
[u32 tag][u32 length][length-8 payload bytes]
```

* `FUN_00095cb3` (0x95CB3) reads the length at chunk+4
  (`iVar1 = *(int *)(param_1[4] + 4)`), advances the cursor by it
  (`param_1[4] += iVar1`), and returns the chunk pointer; sentinel tags
  `0xFFFFFFFF` (rewind/loop) and `0xFFFFFFFD` (skip) are handled inside.
* `vgt_stream_poll` reads the tag (`_DAT_000563c0 = *chunk`) and only
  dispatches VGT tags (`0x54475665 < tag`), skipping others via
  `FUN_00095dd2`.
* Observed chunk sequence in `VID_BULL.TGV`: `kVGT` (0x169C) @0,
  tag `0x684E5331` (0x1088) @0x169C, `kVGT` (0x169C) @0x2724, ...

## 2. Port

* `include/fifa96_loader/fifa96_tgv_stream.h` +
  `src/fifa96_loader/fifa96_tgv_stream.c`:
  `fifa96_tgv_walk_next(walk, &chunk, &chunk_len, &is_frame)` — returns
  1 per chunk (with `is_frame` for the kVGT tag), 0 at the end, negated
  `FIFA96_ERR_TRUNCATED` for a length < 8 or one running past the buffer.
  Frame chunks feed `fifa96_kvgt_decode` unchanged.
* `tests/test_tgv_stream.c`: the committed real frame chunk walks as one
  kVGT frame; a synthetic three-chunk stream (companion/frame/companion)
  yields exactly one frame decoded to 9600 bytes; malformed cases
  (short header, zero length, overrun, empty, NULL). Suite 26 -> **27**,
  ASan/UBSan clean.

## 3. Open legs

1. **Companion chunk tags** (`0x684E5331`, `0x644E5331`, bytes
   `31 53 4e 68`/`31 53 4e 64`, lengths 0x1068-0x1088) are unidentified
   — audio or inter-frame delta candidates. They are skipped by the
   player and the walker.
2. Sentinel handling (`-1` rewind, `-3` skip) lives in `FUN_00095cb3`;
   the port's walker treats the file as a linear sequence (sufficient for
   the committed fixture). A looping player port would need the sentinel
   semantics.
3. Playback timing (`0x563c4..0x563f8`, `FUN_000cb2a4` clock,
   `FUN_000a7d3c`/`0xa7e31`/`0xa7ce4`/`0xa7ebc` rate helpers) is not
   ported — out of scope for the file-format layer.

## 4. Provenance

* Ghidra: `decompile_function 0x95CB3` (walker), `0x67BA8`
  (`vgt_stream_poll`, quoted in FU-29), `0xAE4BC` (tag dispatch).
* Asset: `VID_BULL.TGV` chunk walk (offsets above); the first frame chunk
  is the committed `tests/golden/vgt/kvgt-frame-01.bin`.
* `make test`: 27/27.

## Errata (2026-10-04): the companion chunks are the audio stream

* §3.1 "unidentified" is resolved. `vgt_stream_poll` reads a second
  stream (`PTR_DAT_000563f4`) alongside the frame stream and routes its
  chunks through `FUN_000a7d3c` (0xA7D3C), the audio event dispatcher:
  tag `0x644E5331` (`'1SNd'`, data) queues into the sound subsystem
  (`0xA7D55..`), `0x684E5331` (`'1SNh'`, header) routes to
  `FUN_000a79dc`, `0x654E5331` (`'1SNe'`) sets -1 and calls
  `FUN_000a7b2b`, `0x6c4E5331` (`'1SNl'`) releases via `FUN_00094eff`.
  (`search_instructions CMP 0x684e5331/0x644e5331`: single hits at
  `0xA7D51`/`0xA7D6A`, both inside `FUN_000a7d3c`.)
* Empirical corroboration: the first `'h'` chunk payload is
  `45 41 43 53` ("EACS") + `80 3e` (LE16 16000 = sample rate) + format
  bytes `00 00 02 02 00 07 00 ac` then silence; `'d'` payloads are
  high-entropy sample bytes (~0x1068 bytes per chunk).
* Porting audio is a separate subsystem and stays out of scope for the
  file-format layer; the walker skips these chunks.
