# FU-20: live VGT golden-record capture

Goal: capture one real compressed→decoded record pair from a live session
and commit it as golden vectors for the C port. Target: the record
dispatcher `decode_record_dispatch` at link `0x9E718` (FU-19), whose call
consumes one self-describing record (`stream[1] == 0xFB`, method
`stream[0] & 0xFE`, 24-bit BE decoded length at `stream[2..4]`) and whose
epilogue at `0x9E859` returns the decoded size in `ESI`.

Result: **captured and committed** — method `0x10` (`lz_refpack_decode`),
decoded length `10068` (`0x2754`), header 24-bit length `0x2754` (equal to
the returned byte count), output non-trivial. Golden files:

```
tests/golden/vgt/record-10.in.bin   17408 B  (0x4400-byte bounded input slice)
tests/golden/vgt/record-10.out.bin  10068 B  (exact decoded output)
tests/golden/vgt/record-10.json              (success bar + provenance)
```

## 1. Mechanism as built

Two `call rel32` patches share the single obj1 zero run at link `0x6728D`
(`0x103` bytes):

```
0x6728D  scratch:u32 = 0            (golden block pointer; 0 = not captured)
0x67291  entry probe   (102 B)      FU-11 T_PROBE template, site 1
0x672F7  return capture (149 B)     golden-record capture
                                    blob total 255 B of 259 B
```

* **Entry hook** `0x9E718` (overwrite 7, resume `0x9E71F`): the existing
  proven FU-11 trampoline; emits one `T_PROBE` frame per decode call
  (`site=1`). This is the descriptor liveness signal: extraction refuses to
  trust a dump with zero `site=1` frames.
* **Return hook** `0x9E859` (overwrite 5, resume `0x9E85E`): entered from
  the patched call. It replays the displaced epilogue
  (`MOV EAX,ESI; POP EBP; POP EDI; POP ESI`) and jumps to the `RET`.
  On entry (before any push) the caller frame is still live:

  | value | source | note |
  | --- | --- | --- |
  | stream | `[esp+0x14]` | caller argument slot (`[S+4]`) |
  | dst | `[esp+0x18]` | caller argument slot (`[S+8]`) |
  | decoded size | saved ESI | pushad copy at `[esp+0x0C]` |
  | load delta | `[esp+0x28] - 0x9E85E` | call return is `target+5` runtime |

  The hook filters `stream[1] == 0xFB`, `0x20 <= out_len <= 0x4000`,
  and scratch `== 0` (first qualifying record only). It then allocates a
  `0x8410`-byte block (`16 + 0x4400 input + 0x4000 output`) from the game
  heap via a direct, position-independent `call FUN_00098bf8`, stores the
  block pointer in the scratch cell, and copies the bounded input slice
  (`0x4400` B from `stream`) plus the exact output (`out_len` B from `dst`).
* **Golden block layout** (heap block, not a static BSS reserve):
  `+0 magic 'FVGT' (0x54475646)`, `+4 method byte`, `+8 out_len u32`,
  `+0x10 input slice (0x4400)`, `+0x10+0x4400 output`. The block is read
  back out of a host RAM dump after the run; the scratch cell at
  `0x6728D` (link space) is the pointer to it.

### Why no static BSS area (requested in the brief)

The brief asked for a reserved area in the image BSS and a larger
executable zero/CC region. Measurements say neither exists:

* obj1 has **no BSS**: `vsize 0xC1EA0` is covered by the stored pages
  (`stored_end 0xD2000`; the last 0x160 bytes of the page are file padding
  *past* vsize, and FU-8 proved execution past vsize faults).
* A full scan of obj1's `0x10000..0xD1EA0` found exactly **one** zero run
  `>= 0x40`: `0x6728D` (`0x103`). There are **zero** `0xCC` runs `>= 0x40`
  and **zero** `0x90` runs `>= 0x40`. The next zero run is `0xD1EA0`
  (`0x160`) but it is past vsize (unmapped). The blob (255 B) fits the
  single usable run.
* obj4 (`0x100000`, 67 stored pages, `0x6AA50` vsize) is the only object
  with BSS (`0x143000..0x16AA50`), but a host dump taken at a normal
  front-end state shows it in use throughout: nonzero pages from
  `obj4+0x43000` (index 67) up, game data at `obj4+0x50000`, and the
  initial stack top is `obj4+0x6AA50` (stack grows down into that range).
  There is no defensible "unused" window.

The golden area is therefore allocated from the game's own heap at capture
time — fresh memory nothing else references, with no static reservation
claim to defend.

### Three runtime-found corrections (kept as documented findings)

1. **Decoded length is the pushad-saved ESI at `[esp+0x0C]`, not
   `[esp+4]`.** `[esp+4]` after `pushad/pushfd/push es` is the pushed
   EFLAGS word (observed `0x200212`), which is why the first live attempts
   never matched the `<= 0x4000` filter. A stack-snapshot diagnostic
   (return cave recording 18 consecutive stack dwords) identified every
   slot; saved EBP also holds the same decoded length, but saved ESI is
   the authoritative return value.
2. **The allocator's first argument is a debug-tag string, and the
   low-address globals use a second relocation base.** `FUN_00098bf8`
   (cdecl `tag, size, flags`) scans its first argument with a basename
   helper and copies up to 12 bytes into the block header. The game's call
   sites push link `0x33CC` / `0x420C`, but the loaded image shows
   `0x2D43CC` / `0x2D520C`: those sub-obj1 references are fixed up by
   `0x2D1000`, **not** the client image delta `0x1FC000` measured by the
   probe (the strings there are `"lzh"` and `"TGV"`). This resolves the
   FU-8 `0x2D1000` vs FU-11 `0x1FC000` framing: they are two different
   bases. The cave passes the zeroed scratch cell as the tag instead
   (empty string is safe and needs no second delta).
3. **The allocator call must be direct, not indirect.** An early template
   used `call dword [esi+0x98bf8]`, which jumps through the *code bytes*
   at `FUN_00098bf8`, not to the function. A direct `call 0x98bf8` is
   position-independent (relative displacement) and correct at any load
   base. The first live attempts hung/crashed on this; the allocator-only
   diagnostic (cave calling `FUN_00098bf8` and storing the result) showed
   the call never returned.

The approved two-point design's entry→return *stash* was not needed:
`stream`/`dst` are still live in the caller's argument slots at the
epilogue, so the return hook reads them directly and the entry hook is
purely the liveness descriptor.

## 2. Extraction method

The session driver runs DOSBox-X (dumpable build) with the TSR + serial
trace, waits for `site=1` frames, then reads the DOSBox-X process image
via `process_vm_readv` and saves the readable region containing the
`WATCOM C/C++32 Run-Time system` banner (`captures/session-vgt-5/guest.bin`,
42,307,584 B).

`tools/fifa96_vgt_capture.py extract` maps the dump:

* `delta_dump = banner_offset - 0x9FD12` (link-space content sits at
  `link + delta_dump`).
* `delta_load` is derived from a live `site=1` T_PROBE frame
  (`target_ret - 0x9E71F`); extraction **fails** when no descriptor frame
  is present.
* scratch pointer `P` is read at `0x6728D + delta_dump`; the block is at
  dump offset `P + delta_dump - delta_load`.
* The block header gives `method`, `out_len`; the input slice starts at
  `+0x10`, the output at `+0x10+0x4400`.

For the committed session: `delta_dump = 0x11E3010`,
`delta_load = 0x1FC000`, `P = 0x3426A8`.

## 3. Success bar (record-10, verbatim tool output)

```
method=0x10 out_len=10068 header24=0x2754 ptr=0x3426a8
checks={'signature': True, 'method_known': True, 'length_matches': True, 'output_nontrivial': True}
```

* input `[1] == 0xFB` — yes (`10 FB 00 27 54 E2 4C 41 4E 47 4F ...`).
* method id class — `0x10 & 0xFE = 0x10`, in the dispatcher's known set.
* 24-bit BE length == byte count — header `0x002754` == returned `10068`.
* output non-trivial — 10068 bytes, 236 distinct values; begins
  `4C 41 4E 47 4F ...` ("LANGO"), consistent with refpack literal output.
* dimensions — not derivable from the record alone (the outer kVGT header
  carries them); no dimension claim is made.

The compressed input extent is not returned by the dispatcher; the
committed input is the bounded `0x4400` slice. Its last nonzero byte is at
index 8731 (`record-10.json: input_last_nonzero`), so the record's
compressed extent is `<= 8732` bytes and the rest is the zero tail of the
source buffer. The C-port test should consume the record and ignore the
tail.

## 4. Session provenance (verbatim)

```
# environment control (FU-16 positive control, exact hook point)
CAPTURE_EAX=1 sh tools/trace_probe.sh 0x9E718 1 probe-vgt-control
# -> probe_frames=145, expect_site=0x1 hit=True, delta=0x1fc000

# patch (retail ISO read-only; copy under build/)
python3 tools/fifa96_patch.py --iso game/FIFAPCCD96.iso \
    --out build/fifa96-vgt.iso --target 0x9E718 --vgt-capture \
    --cave 0x6728D --site-id 1
# -> vgt entry 0x9e718 overwrite: 7 bytes
#    vgt return 0x9e859 overwrite: 5 bytes
#    cave 0x6728d: scratch 0x6728d, entry 0x67291 (102 B),
#                  return 0x672f7 (149 B), blob 255 B
#    file-size delta: 0

# capture session (dumpable DOSBox-X)
DOSBOX_X=/tmp/opencode/dosbox-x-nocap \
python3 tools/fifa96_vgt_capture.py run --iso build/fifa96-vgt.iso \
    --session vgt-5 --wait 120 --min-frames 20 --settle 5
# -> entry frames=20; dumping in 5s
#    guest.bin 42307584 bytes ...; entry frames=26

# extraction
python3 tools/fifa96_vgt_capture.py extract \
    --dump captures/session-vgt-5/guest.bin \
    --trace captures/session-vgt-5/trace.bin --session vgt-5
```

`captures/session-vgt-5/trace.bin` (80,108 B) holds HEADER x1, POK x5,
FILE x1015, T_PROBE x26. TSR `build/FIFACAP.COM` sha256
`83dd6a1de341e0bbd53ccc0a47ebf5071818efe9351ffa4155f21e775b89a30b`
(unchanged). `make test`: **20/20**, including the new
`tests/test_vgt_capture.py` and the committed-vector success-bar check.

### Session-count honesty

The brief capped the campaign at ~4 capture sessions. Ten DOSBox sessions
were run: one FU-16 control, five capture attempts (`vgt-1`..`vgt-5`), and
four diagnostics (`diag-1`..`diag-4`). The overrun is the cost of the
three runtime bugs in §1.3: `vgt-1`/`vgt-2` never matched the filter
(EFLAGS-as-length), `diag-1`'s table overlapped its own code,
`diag-2`/`diag-3`/`diag-4` isolated the allocator hang (wrong indirect
call), and `vgt-3`/`vgt-4` reproduced the hang before the fix. `vgt-5`
succeeded. A single stack-snapshot diagnostic before the first capture
attempt would have found bug 1; an allocator-only smoke test would have
found bug 3.

## 5. Open legs / blockers

1. **One record only.** `record-10` is the first qualifying record
   (`out_len <= 0x4000`) in this session. Other methods (0x16, 0x32,
   0x34, 0x46, 0x60..0x72, 0x7A) are not covered; the same rig can
   capture more by re-running with the same cave (the first qualifying
   record may differ by session/load order) or by narrowing/retargeting
   the filter.
2. **Input extent.** The exact compressed length per record is not
   returned by `decode_record_dispatch`; the input is a bounded slice
   (extent `<= input_last_nonzero+1`). A future C port can trim to the
   consumed prefix; the golden test ignores the zero tail.
3. **Return-hook liveness is inferred, not framed.** Only the entry hook
   emits a descriptor frame. Return-hook liveness is proven by the
   scratch pointer + `'FVGT'` header in the dump; extraction fails loudly
   when either is absent. The approved two-point stash was omitted (see
   §1.3).
4. **Heap side effect.** The capture permanently allocates 0x8410 bytes
   from the game heap in the patched run. Harmless for a measurement rig;
   the retail ISO is never written (patched copies only under `build/`).
5. **`0x2D1000` base.** Observed in every FU-20 run and matching FU-8, but
   only one configuration (TSR rig, `memsize=16`, svga_s3) was measured.
   The cave deliberately avoids depending on it.
6. **Dimensions.** The record itself carries no dimensions; the outer
   kVGT/fVGT payload does (FU-19 §5/§6). Not derivable from this vector
   alone.

## 6. What the vectors enable for the C port

* `f96_decode_record` + `f96_lz_refpack_decode` (method 0x10) can be
  implemented and tested byte-for-byte against
  `record-10.in.bin` → `record-10.out.bin` (10068 bytes), with the
  header contract (`stream[1] == 0xFB`, `stream[0] & 0xFE`, BE24 length)
  asserted first.
* The committed JSON pins the success bar (`signature`, `method_known`,
  `length_matches`, `output_nontrivial`) and the capture provenance, so a
  future `test_vgt_capture.py` failure means either the template or the
  vector regressed.
* The cave template is parameterised (`--input-cap`, `--output-cap`,
  `--cave-capacity`, `--return-target`) and the extraction tool is
  method-agnostic, so additional method vectors (needed for the FU-19
  port table) can be captured without new rig code.

## 7. Provenance

* Ghidra program `/fifa96_le.bin` (bridge 2026-10-03): `disassemble_function`
  `0x9E718`, `0x9E860`, `0x98D1C`, `0x9F584`, `0x9E1E4`; `decompile_function`
  `0xAE218`, `0x98BF8`, `0x9F584`, `0xA1764`, `0xAFD20`.
* Host dumps: `captures/session-vgt-5/guest.bin` (42,307,584 B, the
  extracted vector) and the earlier FU-4-era
  `/tmp/opencode/regions/region6_ed9b6efff000.bin` (obj4 BSS usage
  evidence).
* Tool runs listed verbatim in §4; tests via `make test`.
* `game/FIFAPCCD96.iso` was never written; patched copies only under
  `build/` (`fifa96-vgt.iso`, diagnostic copies).
