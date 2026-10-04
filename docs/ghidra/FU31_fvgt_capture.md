# FU-31 — live fVGT frame capture (input chunk + decoded canvas)

Date: 2026-10-04. Program `/fifa96_le.bin` (LE link-time flat addresses).
Rig: `tools/fifa96_patch.py --fvgt-capture` + `tools/fifa96_fvgt_extract.py` +
the single return trampoline in obj1's zero run (`0x6728D` scratch,
`0x67291` cave) hooked over `vgt_decode_f`'s epilogue (`0xAE20B`).

Goal: capture one real fVGT frame from a live intro-video run and commit it
as a golden vector. Result: **captured and committed** — chunk tag `fVGT`,
decoder fields `(239, 5, 409, 10)`, chunk length `9972` (`0x26F4`), canvas
`76800` bytes (`320x240`), rendered grayscale it is the FIFA 96 title screen
with the Virtual Stadium Soccer and EA Sports logos.

```
tests/golden/vgt/fvgt-01.in.bin   16384 B  (0x4000 bounded input slice)
tests/golden/vgt/fvgt-01.out.bin  76800 B  (exact decoded canvas, 320x240)
tests/golden/vgt/fvgt-01.json              (chunk facts, checks, provenance)
sha256 in  d3910da507058384537604306469cb992dcc6dccb52e8853b3e3081cc859f0c1
sha256 out 4b5c900d6df16f8a5a7534e0c19725a92d816d0b757ba08c7f84120645a87ccd
```

## 1. Mechanism as built

| item | value |
|---|---|
| hook | `0xAE20B`, overwritten 7 bytes, resume `0xAE212` |
| displaced | `8B 44 24 14 83 C4 28` (`MOV EAX,[ESP+0x14]`, `ADD ESP,0x28`) |
| cave | `0x67291`, 245 B; blob 249 B of the `0x103` zero run (10 B spare) |
| scratch | `0x6728D`, block head cell (link space; cell = allocator tag) |
| call return | `[esp+0x28] = 0xAE210 + load delta` (5-byte `call` pushes it) |
| ctx | `[esp+0x28+0x3c] = [esp+0x64]` (cdecl arg 1) |
| chunk | `[esp+0x28+0x40] = [esp+0x68]` (cdecl arg 2) |
| canvas | `ctx[10]+0x10`: index **10** (byte `0x28`) after the entry swap |
| out_len | `ctx[0] * ctx[1]` (pitch × height) |
| allocator | `FUN_00098bf8(tag=scratch cell, size, 0)`, size `0x44010` |
| block | `+0 next=0`, `+4 method=0x66`, `+8 out_len`, `+0xC 'FVGT'`, `+0x10` input, then output |

The cave gates on `chunk[0..3] == 'fVGT'`, chunk fields `+8`/`+10` nonzero,
nonzero `ctx[0]`/`ctx[1]` with `pitch*height <= 0x40000`, and a nonzero
surface pointer. One block is allocated on the first qualifying call and
every later qualifying call overwrites its input/output in place
(capture-latest); a lost `'FVGT'` magic or method re-allocates.

## 2. Brief errata found by measurement

The FU-31 brief's established facts were wrong in five places; each was
caught by the binary/dump and corrected:

1. **Overwrite/resume.** `MOV EAX,[ESP+0x14]` is 4 bytes (`8B 44 24 14`),
   not 3, so the whole-instruction prefix is **7** (`…83 C4 28`), resume
   `0xAE212`, not 6/`0xAE211`. Patching 6 would split the `ADD` and resume
   mid-instruction. `tools/fifa96_patch.overwrite_len` already returned 7.
2. **Canvas pointer.** The brief's `ctx[0xc]` (byte `0x30`) is the
   `width*4` index array; byte `0x40` (index `0x10`) is the row table. The
   composited surface is `ctx[10]` (index 10 = **byte `0x28`**) `+0x10`;
   `vgt_dispatch` (`0xAE4BC`) returns `ctx[10]`. Sessions fvgt-1/2/4
   captured the index array / row table and produced table-shaped noise.
3. **Chunk fields are not pixel dims.** Ghidra `vgt_decode_f`
   (`0xADF34..0xADFA3`) reads `LE16(+8)`→`ctx[4]` index pairs,
   `LE16(+0xA)`→`ctx[5]` raw 16-byte blocks, `LE16(+0xC)`→`ctx[6]` 8-byte
   palette records, `LE16(+0xE)`→`ctx[7]` row-stream bits. A TGV-asset
   census shows `(80,0,352,9)`, `(1,43,20,6)`, `(1,0,0,1)`, … — never a
   plain `(320,240)`. `dims_ok` is therefore the structural equation below,
   not `width*height == out_len`.
4. **Input cap.** The brief's `0x3000` truncated the first converged live
   chunk (`in_len 0x3774`, session fvgt-3). The measured maximum fVGT chunk
   over the ISO's TGV assets is `0x3AB8` (VID_SCB0); the rig uses `0x4000`.
5. **Copy-once → capture-latest.** The first qualifying chunk composites
   from an unpainted/previous surface, so its canvas is an early frame
   (session fvgt-2: `(1,43,20,6)`, canvas mostly zeros/noise). Capturing
   the latest qualifying call leaves the converged intro frame for the
   300 s dump to read. One block per run either way.

## 3. Extraction (`tools/fifa96_fvgt_extract.py`)

There is no entry probe, so no live `T_PROBE` frame can derive the guest
load delta. The extractor defaults `--delta-load 0x1FC000` (the value
observed in every FU-4/FU-20/FU-21 session) and fails loudly unless the
mapped block header is a `'FVGT'` block with method `0x66`, `next == 0`,
and `out_len <= 0x40000`. `fifa96_runtime.find_delta` supplies
`delta_dump` from the WATCOM banner.

Success bar:

* `tag_ok` — `input[0:4] == 'fVGT'`.
* `dims_ok` — `in_len == chunk_size(+8, +10, +12, +14, out_len)`, where

  ```
  chunk_size = 0x14
             + ((index_count*20 + 31) & ~31) >> 3
             + raw_count*16 + palette_count*8
             + ((row_bits*(out_len/16) + 31) & ~31) >> 3
  ```

  This ties the declared chunk length to the canvas size and held for the
  captured vector: `0x14 + 600 + 80 + 3272 + 6000 = 9972`.
* `out_nontrivial` — more than one distinct output byte.

## 4. Session provenance (verbatim)

```
# patched ISO (retail ISO read-only; copy under build/)
python3 tools/fifa96_patch.py --iso game/FIFAPCCD96.iso \
    --out build/fifa96-fvgt.iso --target 0xAE20B --fvgt-capture \
    --cave 0x6728D
# -> fvgt return 0xae20b overwrite: 7 bytes (resume 0xae212)
#    cave 0x6728d: scratch 0x6728d, return 0x67291 (245 B), blob 249 B
#    file-size delta: 0

# capture session (dumpable DOSBox-X, no keys: looping intro plays)
DOSBOX_X=/tmp/opencode/dosbox-x-nocap \
python3 tools/fifa96_vgt_capture.py run --iso build/fifa96-fvgt.iso \
    --session fvgt-5 --wait 420 --min-frames 0 --settle 300
# -> guest region ...; entry frames=0; dumping in 300s
#    guest.bin 42307584 bytes ...; entry frames=0

# extraction
python3 tools/fifa96_fvgt_extract.py \
    --dump captures/session-fvgt-5/guest.bin \
    --trace captures/session-fvgt-5/trace.bin --session fvgt-5
# -> tag=fVGT dims=239x5 in_len=9972 out_len=76800 ptr=0x4f6308
#    checks={'tag_ok': True, 'dims_ok': True, 'out_nontrivial': True}
```

Captured-vector mapping: `delta_dump = 0x11E3010`,
`delta_load = 0x1FC000`, block ptr `0x4F6308` (dump offset `0x14DD?`
computed from the two deltas).

### Session-count honesty

Five capture sessions were run (the brief implies one): fvgt-1 (index-array
canvas), fvgt-2 (row-table canvas; vector generated but not committed),
fvgt-3 (`0x3000` cap truncated the latest chunk), fvgt-4 (row-table canvas
with `0x4000` cap), fvgt-5 (success). The three pointer errors and the cap
error are documented in §2 rather than retried silently.

## 5. Open legs

1. **`delta_load` is assumed, not probed.** FU-31 has no entry hook, so
   `0x1FC000` is validated only by the block header. A future variant can
   add a T_PROBE entry point if the loader delta ever changes.
2. **Canvas geometry is not stored.** Only `out_len` (pitch × height) is in
   the block; the JSON `width`/`height` keys are the chunk's decoder fields
   (brief naming; see §2.3). The 320x240 split was recovered by correlation
   score over candidate factorizations during review, not recorded in-band.
3. **The frame is a delta.** `vgt_decode_f` composites the committed chunk
   over the previous surface using `ctx[0xd]` (row-offset table), `ctx[0xb]`
   (previous surface) and the index/row streams. Reproducing the canvas
   from the chunk alone therefore needs the pre-call context, which the
   rig does not capture; the vector is a real reference pair for an
   instrumented/emulated `f96_vgt_decode_f`, and pins the parser contract
   (`chunk_size`, field gates) exactly.
4. **Capture-latest side effects.** The run allocates one `0x44010` block
   from the game heap and overwrites it on every later qualifying call.
   Harmless for a measurement rig; the retail ISO is never written
   (patched copies only under `build/`).
5. **`ctx[0xd]` producer** remains FU-19's open leg; the committed canvas
   is one more observable datum for locating it in FU-32.

## 6. What this enables for FU-32

* A byte-exact target for the `f96_vgt_decode_f` port: fields, gates,
  `chunk_size`, and the final 320x240 surface are all pinned by the
  committed triple and the `test_fvgt_extract` golden-vector test.
* The surface semantics are now cited end to end: `vgt_dispatch` returns
  `ctx[10]`; the compositor writes `param_3 = ctx[10]+0x10`; pitch/height
  are `ctx[0]`/`ctx[1]`; the four chunk u16s are decoder counts.
* The additive `chunk_size` equation gives FU-32 a cheap parser check
  before touching pixel logic, and the 5 observed `(index, raw, palette,
  bits)` shapes give a shape-diversity floor for tests.

## 7. Provenance

* Ghidra `/fifa96_le.bin` (bridge 2026-10-04):
  `decompile_function 0xADEFC` (field reads, swap, allocs, composite
  call), `decompile_function 0xAE4BC` (dispatch returns `ctx[10]`),
  `decompile_function 0xBA994` (writes `param_3 = ctx[10]+0x10`),
  `disassemble_function` push order at `0xAE1D3..0xAE208` (destination
  `ctx[10]+0x10` pushed at `0xAE1F7`).
* `tools/fifa96_patch.py` templates are pinned by `tests/test_patch.py`
  (default and FU-21 templates byte-identical; new `TestFvgtCave`).
* `tests/test_fvgt_extract.py` covers extraction, the structural equation,
  and the committed vector (SHA-256 pins).
* `make test`: **28/28** (27 pre-existing + `test_fvgt_extract`).
* Capture dump: `captures/session-fvgt-5/guest.bin` (42,307,584 B,
  git-ignored); trace `captures/session-fvgt-5/trace.bin`. TSR
  `build/FIFACAP.COM` sha256
  `83dd6a1de341e0bbd53ccc0a47ebf5071818efe9351ffa4155f21e775b89a30b`
  (unchanged from FU-20/21). `game/FIFAPCCD96.iso` was never written.
