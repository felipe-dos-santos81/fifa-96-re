# FU-32 — fVGT paired pre/post capture (delta-frame vector)

Date: 2026-10-04. Program `/fifa96_le.bin` (LE link-time flat addresses).
Rig: `tools/fifa96_patch.py --fvgt-pair` + `tools/fifa96_fvgt_extract.py
--pair` + two trampolines in obj1's zero runs (entry marker `0x18A01` /
`0x18A05`, pair return `0x6728D` / `0x67291`) hooked over `vgt_decode_f`'s
prologue (`0xADEFC`) and epilogue (`0xAE20B`).

Goal: pair the FU-31 post-frame capture with the pre-call canvas state so
the delta frame can be validated end-to-end. Result: **captured and
committed** — the post surface is byte-identical to the FU-31
`fvgt-01.out.bin`, and `fvgt-01.pre.bin` is the pre-call surface of the
same `vgt_decode_f` call.

```
tests/golden/vgt/fvgt-01.in.bin    16384 B  (bounded input slice)
tests/golden/vgt/fvgt-01.out.bin   76800 B  (post, UNCHANGED from FU-31)
tests/golden/vgt/fvgt-01.pre.bin   76800 B  (pre, new)
tests/golden/vgt/fvgt-01.json               (pair layout, pre/post provenance)
sha256 in  45844f059594a28279f1e07463e8dbbdcc3d89b016365bcb6581d2cfb2a2046d
sha256 out 4b5c900d6df16f8a5a7534e0c19725a92d816d0b757ba08c7f84120645a87ccd
sha256 pre 8fdcba20ca2f086e8fe63efb26afc7c58d096884582ddd7a409f393b81db4347
```

Rendered grayscale, both surfaces are the FIFA 96 title screen
(`FIFA 96 / Virtual Stadium Soccer / EA Sports`); the pre frame is the
previous loop iteration of the same screen, so `pre != post` while the
scene is identical.

## 1. Mechanism as built

| item | value |
|---|---|
| entry hook | `0xADEFC`, overwritten 6 bytes, resume `0xADF02` |
| entry displaced | `56 57 55 83 EC 28` (`PUSH ESI/EDI/EBP`, `SUB ESP,0x28`) |
| entry cave | `0x18A05`, 39 B; blob 43 B of the 47 B zero run at `0x18A01` |
| marker cell | `0x18A01` (4 B), entry records `ctx[10]` (pre-swap) there |
| return hook | `0xAE20B`, overwritten 7 bytes, resume `0xAE212` |
| return displaced | `8B 44 24 14 83 C4 28` (`MOV EAX,[ESP+0x14]`, `ADD ESP,0x28`) |
| return cave | `0x67291`, 249 B; blob 253 B of the 259 B zero run at `0x6728D` |
| scratch cell | `0x6728D` (4 B), capture-latest block pointer |
| ctx / chunk | `[esp+0x64]` / `[esp+0x68]` at the return hook |
| post surface | `ctx[10]+0x10` (index 10 = byte `0x28`, post-swap) |
| pre surface | marker cell value `+0x10` (entry `ctx[10]`, pre-swap) |
| out_len | `ctx[0] * ctx[1]` (pitch x height) |
| allocator | `FUN_00098bf8(tag=scratch cell, size=0x84010, 0)` |
| block | `+0 next`, `+4 method=0x66`, `+8 out_len`, `+0xC 'FVGT'`, `+0x10` input slice, then post `out_len`, then pre `out_len` |

The entry marker is a 39-byte stub that recovers the load delta from the
patched call's return address, stores `ctx[10]` (cdecl arg 1) into
`0x18A01`, and replays the displaced prologue. It clobbers only EAX
(caller-saved) and restores ESI; flags are left to the displaced `SUB`.
The return cave gates on `chunk[0..3] == 'fVGT'`, chunk fields `+8`/`+10`
nonzero, nonzero `ctx[0]`/`ctx[1]` with `pitch*height <= 0x40000`, a
nonzero post pointer and a nonzero marker, then capture-latest copies the
input slice, post and pre into one block (a lost `'FVGT'` magic
re-allocates). `vgt_decode_f` has exactly one caller (`vgt_dispatch`, tag
`fVGT`), so the marker is only written by fVGT decodes.

### Erratum caught in review: EBX spill

`FUN_00098bf8` preserves only ESI/EDI (FU-21 note). The pair cave keeps
the pre pointer in EBX across the allocation, so the template spills it
(`push ebx` before the allocator args, `pop ebx` after `add esp,0xc`).
The first assembled build lacked the spill and was discarded before the
first live run; `test_pair_cave_preserves_pre_pointer_across_allocator`
pins it.

## 2. Why the entry hook records a pointer instead of a snapshot

The brief's entry cave was to allocate a block and copy the pre canvas at
entry. That does not fit: obj1's only spare zero runs are `0x18A01`
(0x2F B), `0x1B139` (0x25 B) and `0x6728D` (0x103 B, already fully used
by the FU-31 layout). A snapshot entry cave (alloc + 76800 B copy) needs
roughly 150-200 bytes. The compact alternative is sound because the pre
buffer survives the call:

* `vgt_decode_f` swaps `ctx[10]`/`ctx[0xb]` at entry, so the pre-call
  surface (entry `ctx[10]`) becomes `ctx[0xb]` for the body.
* `composite_4x4_blocks` (`0xBA994`, only caller `vgt_decode_f`) reads
  from `param_1[0xb]+0x10` and writes only to `param_1[10]+0x10`; no other
  write touches the source surface. The pre content is therefore intact
  at the return, and copying it there from the entry-recorded pointer
  yields the exact pre-call bytes of the same call.

The two spare runs have no Ghidra xrefs (`analyze_data_region` on
`0x18A01`/`0x1B139`: 0 references, undefined `DAT_` bytes), the same
evidence class as the FU-31 run; the live runs stayed healthy for 300 s
and produced the byte-identical FU-31 post, so the marker cell was not
clobbered by the game.

## 3. Extraction (`tools/fifa96_fvgt_extract.py --pair`)

`pair_block_view` parses the extended block (`in_cap` slice, then post,
then pre, each `out_len`). `extract_pair` maps it via the WATCOM banner
delta, reads the marker cell for provenance, and applies the success bar:

* `tag_ok` — `input[0:4] == 'fVGT'`.
* `dims_ok` — `in_len == chunk_size(+8, +10, +12, +14, out_len)` (the
  FU-31 structural equation, unchanged).
* `out_nontrivial` / `pre_nontrivial` — more than one distinct byte.
* `pair_differs` — pre and post are not identical.

`write_golden_pair` writes `.in.bin` / `.out.bin` / `.pre.bin` and a JSON
with `layout: pair`, both records' `checks`, per-record `provenance`
(`pre`: entry `ctx[10]` marker at `0x18A01`; `post`: `ctx[10]+0x10` at the
return) and `sha256` for all three files.

`fvgt-01.in.bin` was refreshed: its first `in_len` (9972) bytes are
byte-identical to the FU-31 fixture; only the trailing heap residue of
the 0x4000 slice differs (first difference at offset 9976).
`fvgt-01.out.bin` is byte-identical and was not touched.

## 4. Session provenance (verbatim)

```
# patched ISO (retail ISO read-only; copy under build/)
python3 tools/fifa96_patch.py --iso game/FIFAPCCD96.iso \
    --out build/fifa96-fvgt2.iso --target 0xADEFC \
    --return-target 0xAE20B --fvgt-pair --entry-cave 0x18A01 \
    --cave 0x6728D
# -> fvgt entry 0xadefc overwrite: 6 bytes (resume 0xadf02)
#    fvgt return 0xae20b overwrite: 7 bytes (resume 0xae212)
#    entry cave 0x18a01: marker 0x18a01, entry 0x18a05 (39 B), blob 43 B
#    return cave 0x6728d: scratch 0x6728d, return 0x67291 (249 B), blob 253 B
#    file-size delta: 0

# capture sessions (dumpable DOSBox-X, no keys: looping intro plays)
DOSBOX_X=/tmp/opencode/dosbox-x-nocap \
python3 tools/fifa96_vgt_capture.py run --iso build/fifa96-fvgt2.iso \
    --session <id> --wait 420 --min-frames 0 --settle 300

# extraction (committed vector)
python3 tools/fifa96_fvgt_extract.py \
    --dump captures/session-fvgt-2b/guest.bin \
    --trace captures/session-fvgt-2b/trace.bin --session fvgt-2b --pair
# -> tag=fVGT dims=239x5 in_len=9972 out_len=76800 ptr=0x4f6308
#    marker=0x4de068
#    checks={'tag_ok': True, 'dims_ok': True, 'out_nontrivial': True,
#            'pre_nontrivial': True, 'pair_differs': True}
```

### Session-count honesty

Five sessions were run (four long, one diagnostic): `fvgt-2-diag`
(20 s settle, early 320x240 flythrough pair, `dims=222x72`, not
committed); `fvgt-2` (300 s, later 160x120 segment, `out_len=19200`,
`dims=16x382`); `fvgt-2b` (300 s, title-screen pair, `dims=239x5`, the
committed vector); `fvgt-2` again (300 s, 160x120 segment, `dims=43x443`).
The game's wall-clock phase at the 300 s dump is not deterministic across
runs: three of the four long runs landed on the later 160x120 stadium
segment and one caught the 320x240 title loop. `fvgt-2b` is the same
invocation as `fvgt-2`, only a different run.

The first `fvgt-2` long dump was unusable: `process_vm_readv` over the
61 MB emulated-RAM region stopped at a mid-region hole after 0x1101000
bytes, so the banner was beyond the saved prefix. `read_mem` now reads in
1 MB chunks and zero-fills a page past a short read, preserving the
region layout; `test_vgt_capture.TestReadMem` covers the hole case. All
post-fix dumps are the full 42,307,584 bytes.

## 5. Captured pair facts

| item | value |
|---|---|
| session | `fvgt-2b`, dump `captures/session-fvgt-2b/guest.bin` |
| block ptr | `0x4F6308` (same deterministic heap slot as FU-31) |
| marker | `0x4DE068` (pre surface recorded at `0xADEFC`) |
| chunk | `dims=239x5`, `in_len=9972`, fields `(239, 5, 409, 10)` |
| post | 76800 B, sha `4b5c900d…` (== FU-31 out) |
| pre | 76800 B, sha `8fdcba20…`, 253 distinct byte values |
| checks | all five true; pre != post |

The marker (`0x4DE068`) is a heap surface pointer, and the pre bytes are
not garbage or a table (renders as the previous title-screen loop frame),
which is the empirical answer to "which buffer is pre vs post": pre is
the entry `ctx[10]` surface, post the `ctx[10]+0x10` surface at the
return.

## 6. Open legs

1. **No in-cave marker==ctx[0xb] compare.** The marker is used as the pre
   source, so the pair is correct by construction, but the cave does not
   also compare the marker with `ctx[0xb]` at the return (12 bytes over
   the 0x103 run budget). The identity is argued from the decompiler
   (swap + read-only source) and from the marker semantics.
2. **Pre pointer not stored in the block.** The JSON records the marker
   cell value read from the dump; it is not in-band in the block.
3. **Capture-latest phase variance.** Which intro segment is current at
   the dump depends on wall-clock timing; the 160x120 pairs from the
   other runs are not committed (dumps are git-ignored).
4. **`0x1B139` (37 B) remains unused**; the entry marker used the 47 B
   run. Both are xref-free.
5. **`ctx[0xd]` producer** remains FU-19's open leg; the pre/post pair is
   one more observable datum.

## 7. Provenance

* Ghidra `/fifa96_le.bin` (bridge 2026-10-04):
  `decompile_function 0xADEFC` (swap, field reads, composite call
  `param_1[0xb]+0x10 -> param_1[10]+0x10`), `0xBA994` (only writes
  `param_3`), `0xAE218`, `0x67BA8`; `get_function_xrefs 0xADEFC` (only
  `vgt_dispatch`), `0xBA994` (only `vgt_decode_f`);
  `analyze_data_region 0x18A01` / `0x1B139` (no xrefs);
  `disassemble_bytes 0xADEFC` (overwrite boundary 6).
* `tools/fifa96_patch.py` templates are pinned by `tests/test_patch.py`
  (default and FU-21/FU-31 templates byte-identical; new
  `TestFvgtPairCave`, 30 -> 46 tests).
* `tests/test_fvgt_extract.py` covers the pair block, checks and
  committed vector (15 -> 24 tests); `tests/test_vgt_capture.py` covers
  the chunked `read_mem` (18 -> 20 tests).
* `make test`: **28/28** (same 28 CTest entries; test-case count grew
  inside them). Capture dumps are git-ignored; `game/FIFAPCCD96.iso` was
  never written (sha `d57c5f50818b71115e23863c…`). TSR `build/FIFACAP.COM`
  sha `83dd6a1de341e0bbd53ccc0a…` (unchanged).
