# FU-56 — Mode-X VGA blitter (`FUN_000ae7f0` 0xAE7F0)

Date: 2026-10-05. Program `/fifa96_le.bin` (FU-4 linear image, flat link
addresses, bridge 2026-10-04). Result: the planar blitter that presents
decoded TGV surfaces is derived, its clip/page semantics are ported
(`fifa96_blit`), and the derived model is cross-checked by executing the
original function in an emulator against a Python/port reference. The port
completes the chain stream -> player -> blit.

## 1. Entry chain and callers

`get_function_callers 0xae7f0` = `{FUN_00068194}` only. `FUN_00068194`
(103 insns, movie host) opens the player (`FUN_000679f4`, 0x681af), polls
`vgt_stream_poll` (0x681ce), centers the returned surface, blits it and
flips the page; `FUN_00068108` (the other movie host) presents through
`FUN_000a6040` -> `FUN_000b4fd0` (a sprite/span rasterizer) and never calls
this blitter. See §8 errata 1.

The clip rectangle lives in four globals read by the blitter; writers:

| global | role | writers |
|---|---|---|
| `0x131c8` | clip left | `FUN_000ce7bc` 0xce7fc, `FUN_000d0780` 0xd07c5 |
| `0x131cc` | clip top | `FUN_000ce7bc` 0xce801, `FUN_000d0780` 0xd07d0 |
| `0x131d0` | clip right | `FUN_000ce7bc` 0xce807, `FUN_000d0780` 0xd07ca |
| `0x131d4` | clip bottom | `FUN_000ce7bc` 0xce80d, `FUN_000d0780` 0xd07d6 |

`FUN_000ce7bc(left, right, top, bottom)` clamps negative left/top to 0 and
caps right/bottom at `0x131c0`/`0x131c4` before storing (9 callers);
`FUN_000d0780` stores the four fields of a sprite object (`+8/+0xc/+0x10/
+0x14`) into both the object and the globals when the object list matches
(`[obj+0x2c] == [0x131ec]`, 3 callers). The blitter treats the rect as
half-open `[left,right) x [top,bottom)`: each comparison subtracts the clip
bound and branches on `> 0` (top 0xae819, bottom 0xae839, left 0xae847,
right 0xae85c).

Page state is global `0xa2ac` (parity), flipped by the host at 0x68291;
the per-frame y the host passes already includes `parity*0xf0` (240)
(0x68210/0x6821f), so the blitter itself has no page concept beyond the
address arithmetic.

## 2. Blitter (`FUN_000ae7f0`, 120 insns)

cdecl `blit(surface, x, y)`; after the 3 pushes + `SUB ESP,0x1c` the args
are `[ESP+0x2c]` surface, `[ESP+0x30]` x, `[ESP+0x34]` y (0xae7f6..0xae806).
Surface header: width = `dword[+2] >> 16` (0xae813/0xae81b), height =
`dword[+4] >> 16` (0xae816/0xae81e), pixels at `+0x10` (0xae810).

**Clip pre-pass** (all in pixels, source pointer `ECX = surface+0x10`):

1. Top: `EDI = 0x131cc - y`; if `> 0` then
   `src += EDI*width; h -= EDI; y += EDI` (0xae80a/0xae819..0xae82e) —
   whole source rows are skipped with the **header width** (0xae827 IMUL).
2. Bottom: `if (y+h) - 0x131d4 > 0` then `h -= over` (0xae830..0xae83f).
3. Left: `EDI = 0x131c8 - x`; if `> 0` then
   `src += EDI; w -= EDI; x += EDI` (0xae841..0xae851).
4. Right: `if (x+w) - 0x131d0 > 0` then `w -= over` (0xae853..0xae862).
5. `w <= 0 || h <= 0` -> return (0xae864/0xae866/0xae86e).

**Destination offsets** (0xae874..0xae89f): `start = y*320` (`0x140`,
0xae8a3), `end = (y+h)*320` (0xae889), width saved at `[ESP+0xc]`
(0xae876), x saved at `[ESP+0x14]` (0xae894), plane counter at
`[ESP+0x10]` = 0.

**Plane pass** (0xae8a7..0xae94c), `p = 0..3`:

6. `out(0x3C4, (mask<<8)|2)` with `mask = 1 << ((x+p)&3)` (0xae8ab..0xae8c5,
   `FUN_000d064b` decompile: sequencer index 2 = Map Mask).
7. `col = ((x+p) & ~3) >> 2` (0xae8c8..0xae8d9); source row pointer
   `= src + p` (0xae8dc); **count = w - p** (0xae8de loads `[ESP+0xc]`,
   0xae93a DECs it).
8. For each row `off = y*320 .. (y+h)*320` step 320:
   `dst = 0xA0000 + (off>>2) + col`, gather `count` source bytes at stride 4
   (`FUN_000baa48`, 0xae90f); `src += width` (0xae928); emit bytes with the
   Map Mask still selecting plane `(x+p)&3`.
9. `w_slot--; x++; p++` until 4 passes (0xae92e..0xae94c).

**Gather/write granularity (`FUN_000baa48`, 110 insns).** `src=EAX`,
`dst=EDX`, span `EBX`. Two equivalent code paths selected by `[0x12ae0]`
(0xbaa49): the 386+ path reads bytes at `[ECX]`, `[ECX+4]`, `[ECX+8]`,
`[ECX+0xc]` and packs them big-endian into one dword with BSWAP
(0xbaa60..0xbaa6d) — i.e. the dword written to the plane is
`src[0] | src[4]<<8 | src[8]<<16 | src[0xc]<<24` (0xbaae7..0xbab2a is the
shift-based equivalent). Chunks: 0x40 source-span -> 16 plane bytes
(0xbaa56..0xbaaab), 0x10 -> 4 (0xbaaad..0xbaaca), 4 -> 1
(0xbaacc..0xbaade), remainder < 4 dropped (0xbaacf/0xbaad1). Net:
**floor(count/4) plane bytes, gathering every 4th source byte.**

Consequently pass `p` writes `B_p = floor((w-p)/4)` bytes to plane
`(x+p)&3` at byte column `(x+p)>>2`, taking source pixels `p+4k`
(`k < B_p`), i.e. destination pixels `x+p+4k`. For `w` a multiple of 4 the
three trailing pixels `x+w-3..x+w-1` are **never written** (each pass
loses its last remainder); for arbitrary `w` the covered pixel count is
`sum_{p=0..3} floor((w-p)/4)`. This is the original's write granularity,
not an emulation artifact: §7 leg 5.

## 3. Destination model

Mode X (planar) layout, 320 pixels/row: each plane row is 80 bytes
(`320/4`), pixel `(X,Y)` -> plane `X&3`, plane byte `Y*80 + (X>>2)`.
The blitter computes exactly `0xA0000 + y*80 + (#group)` (0xae8f4..0xae903
divides the 320-stride offset by 4). The host's y already contains the page
offset, so a page is 240 rows = 0x4B00 plane bytes = 0x12C00 linear bytes.
The Map Mask writes and per-plane addressing only take effect with VGA
chain-4 off; the host's mode set (not traced here) must establish that.

Registers: only the Sequencer Map Mask (`0x3C4` index 2, value
`1<<(x&3)`) is programmed per plane pass (`FUN_000d064b`, 0xAE8B5/0xAE8C0);
writes are ordinary byte/dword stores to `0xA0000` (the original never
touches `0x3CE`/`0x3CF` here).

## 4. Presentation sequence (`FUN_00068194`)

1. `w = dword[surface+2]>>16`; `h = dword[surface+4]>>16` (0x681dd/0x681f6).
   `x = ((320-w)/2 + 3) & ~3` (0x681e0..0x681f9, 320 = `[0x12abc]`);
   `y = ((240-h)/2 + 3) & ~3 + parity*240` (0x68200..0x6821f,
   240 = `[0x12ac0]`, parity = `[0xa2ac]`).
2. `FUN_000ae7f0(surface, x, y)` (0x68224).
3. If the palette flag `[0x563fc]` is set, disable interrupts and, after
   the CRTC write below, wait for vertical retrace, upload the 256-entry
   DAC and re-enable interrupts (0x68232..0x6827c).
4. Page select: `FUN_000d0620(parity*0x12c00/4 = parity*0x4b00)`
   (0x68237..0x68250) writes CRTC start address `0x3D4` index 0xC = high
   byte, index 0xD = low byte (`FUN_000d0620` decompile) — the page that
   was just drawn is displayed.
5. `FUN_000ce8d7` (0x68262) is the only vsync code: `DX=0x3DA`, bit 3
   (vertical retrace) polled with a `[0x14618]` timeout (100000; set to
   10000 on timeout, 0xce8d7..0xce917); `[0x15b18] >= 2` skips the wait
   entirely. It runs **only when the palette changed**; the page flip
   itself is not vsync-throttled.
6. `parity ^= 1` (0x68282..0x68291).

## 5. Port mapping

`include/fifa96_loader/fifa96_blit.h` + `src/fifa96_loader/fifa96_blit.c`:

| original | port |
|---|---|
| `FUN_000ae7f0` clip pre-pass | `fifa96_blit_modex` internal, identical order/thresholds, int64 arithmetic |
| plane pass p, count `w-p` | same pass loop: `B_p = (w-p)/4`, plane `(x+p)&3`, col `(x+p)>>2`, stride-4 source gather |
| `FUN_000baa48` gather | byte loop `dst[base+k] = src[p+4k]` (same final bytes, no dword packing) |
| destination `0xA0000`, 0x140 stride | caller-owned `fifa96_modex_image.planes[4]` + `plane_cap`; 80-byte plane stride |
| Map Mask / CRTC / retrace | not modeled (no hardware); exposed as constants + page helpers |
| page y addend `parity*240` | `fifa96_blit_page_rows(page)` |
| CRTC start `parity*0x4b00` | `fifa96_blit_page_start(page)` |
| `[0x131c8..0x131d4]` clip | `fifa96_blit_clip {left,top,right,bottom}`, half-open |
| silent early-out | `FIFA96_OK` with no writes for empty/off-canvas windows |

Bounds-check conventions: NULL args or NULL plane -> `-FIFA96_ERR_TRUNCATED`;
`src_len < width*height` -> error; `x/y < 0` after clipping (caller clip with
negative left/top) -> error; the highest written plane byte
`max_p((y+h-1)*80 + ((x+p)>>2) + (w-p)/4 - 1)` over the passes with
`w-p >= 4` must be `< plane_cap` -> else error, checked before any write.
`fifa96_fvgt_decode`'s multiple-of-4 restriction means real canvases never
hit the `w%4` drop edge; the port reproduces it anyway. No globals, no I/O,
no allocation.

## 6. Tests (`tests/test_blit.c`, suite 45 -> 46)

* Plane interleave/coverage: 4x1 (only plane 0), 8x1 (all four planes plus
  the dropped 5,6,7), unaligned x=2, 4x2 column, width 1 and 6 partial
  groups.
* Clip: top (y=-2), left (x=-3), right (x=316,w=8), bottom (y=238), a
  sub-window `[1,1,7,5]`, a zero window, off-canvas left/right/bottom.
* Page: `page_rows`/`page_start` values; blit at y=242; boundary capacity.
* Errors: plane_cap one byte short, short `src_len`, NULL clip/src/image/
  plane; destination canary `0xA5` proves no partial writes.
* Fixture: `tests/golden/vgt/fvgt-01.out.bin` (76800 B) full frame at
  (0,0) and at page 1 (y=240) against Python-computed FNV-1a-64 per-plane
  and concatenated checksums, spot rows, and a per-pixel structural check
  (columns 317-319 remain zero; all others equal the fixture).
* Out-of-band: `FUN_000ae7f0` was executed instruction-by-instruction
  (Unicorn 2.1.4) from the exact Ghidra-dumped image (sha256
  `bba699e98b3d657e6553fdd1...`, 1485392 B) with Map-Mask tracking; its
  four plane images are byte-identical to `fifa96_blit_modex` for all hand
  cases and for the 320x240 fixture. Emulated plane sha256:
  `48301e77...`, `ba8d9dbe...`, `e0b02c01...`, `5cdc48b2...`.

## 7. Open legs

1. VGA sequencing/timing (Map Mask latching, dword write batching, DAC
   update window, retrace) is not modeled; the port produces the exact
   post-blit plane bytes.
2. The `[0x12ae0]` CPU-path selector in `FUN_000baa48` (BSWAP vs shifts)
   is not modeled — both paths gather identically.
3. Clip-rect initial values and runtime writes are host state; the Ghidra
   image's object-4 data references are shifted by `0xF0000` from their
   true link addresses (FU-4 rebasing), so the bytes at `0x131c8` at rest
   are object-1 code, not the globals.
4. `FUN_00068108`'s present path (`FUN_000a6040` -> `FUN_000b4fd0`) is a
   different rasterizer and is not derived.
5. The trailing-column drop (up to 3 pixels per row) is real in this build;
   whether a retail disc build behaves the same is not checked.

## 8. Errata (quoted)

1. Brief: "`FUN_000ae7f0` ... called by players `FUN_00068108`/
   `FUN_00068194`". Ghidra `get_function_callers 0xae7f0` = only
   `FUN_00068194`; `FUN_00068108` (0x68108 decompile) calls
   `FUN_000a6040`, which reaches `FUN_000b4fd0`, not this blitter.
2. Brief: "The decoded canvas is linear 8-bit indexed" — confirmed
   (fixture stride-1 similarity, exact linear reconstruction, and the
   live decoded surface in `captures/session-fvgt-2b/guest.bin` at
   0x14e1328).
3. Brief: page flip/vsync — the CRTC start-address write is unthrottled;
   `FUN_000ce8d7`'s 0x3DA poll only guards the palette DAC upload.

## 9. Provenance

* Ghidra `/fifa96_le.bin`: disassembled `0xae7f0` (120 insns, quoted),
  `0xbaa48` (110), `0x68194` (103), `0xce8d7` (17); decompiled `0xae7f0`,
  `0xbaa48`, `0x68194`, `0x68108`, `0xd064b`, `0xd0620`, `0xce7bc`,
  `0xd0780`, `0xce8d7`, `0xce754`, `0xa6040`, `0xb4fd0`; xrefs
  `get_function_callers 0xae7f0`, `0xce7bc`, `0xd0780`,
  `get_xrefs_to 0x131c8/0x131cc/0x131d0/0x131d4`, `0x12ae0`; memory reads
  `0x12ab8`, `0xa2c0`, `0x131c8`, `0xae80a`; instruction search `0x3da`
  (2 sites: 0xce8d7/0xce943).
* Emulation: `/tmp/opencode/ghidra_le.bin` (Ghidra `ram` block dump),
  Unicorn 2.1.4, INS/OUT + memory hooks, nine hand cases + fixture.
* Assets: `tests/golden/vgt/fvgt-01.out.bin` (76800 B, sha256
  `4b5c900d6df16f8a5a7534e0...`).
* `make test` 45/45 before; 46/46 after. `test_blit` is ASan/UBSan-clean
  (leak detection included); the full suite passes under ASan/UBSan with
  LSAN disabled (six pre-existing tests leak file buffers).
