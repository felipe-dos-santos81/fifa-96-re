# FU-57 — Frame presentation around the Mode-X blitter

Date: 2026-10-05. Program `/fifa96_le.bin` (FU-4 linear image, flat link
addresses, bridge 2026-10-04). Result: the presentation loop that drives the
FU-56 blitter is derived end to end — host `FUN_00068194`, CRTC page select
`FUN_000d0620`, retrace wait `FUN_000ce8d7`, DAC upload `FUN_000ce754`, the
alternate presenter `FUN_00068108` (palette translation + quad rasterizer, no
DAC/CRTC/vsync), the CPU-path selector `[0x12ae0]`, and the object-4 fixup
shift. The presentation order and the upload/parity arithmetic are ported as
`fifa96_vga` (suite 46 -> 47); hardware timing stays documented, not modeled.

## 1. Hosts and entry chain

| function | size | role | callers (`get_function_callers`) |
|---|---|---|---|
| `FUN_00068194` | 103 insns | Mode-X movie host: poll -> blit -> page flip -> optional palette | `FUN_0006844c` |
| `FUN_00068108` | 45 insns | alternate host: poll -> palette translation -> quad rasterizer | `FUN_00033d4c`, `FUN_000638b0` |
| `FUN_0006844c` | 0x6844c | wrapper: `[0x1a2b4] = param`, open `FUN_00067fe4`, loop host while state 2 | — |

`FUN_0006844c` (decompile) stores its second argument in `[0x1a2b4]`, opens the
player with `FUN_00067fe4`, then calls `FUN_00068194` until it returns 3 (exit
key) or 5 (stream end), then clears `[0x1a2b0]` and closes the player. The
movie-active gate is `[0x1a2b8]` (checked at 0x6819a and 0x6810e); the player
filename pointer passed to `FUN_000679f4` is the immediate 0x55fc0 (true link
0x155fc0, §8; the buffer is zero at rest and filled by the caller).

## 2. Presentation loop (`FUN_00068194`)

Full disassembly quoted (103 insns, 0x68194..0x682cd). The loop has no frame
wait of its own; pacing lives in `vgt_stream_poll` (FU-55 §7). Per decoded
surface:

1. `surface = vgt_stream_poll()` (0x681ce); `surface == 0` -> state 5, close,
   return (0x681d5..0x682a8).
2. Geometry (0x681dd..0x6821f): `w = dword[surface+2] >> 16`,
   `h = dword[surface+4] >> 16` (FU-56 §1);
   `x = ((w0 - w)/2 + 3) & ~3`;
   `y = ((h0 - h)/2 + 3) & ~3 + parity*240`;
   `w0 = [0x12abc]`, `h0 = [0x12ac0]` (object-4 BSS, §8),
   `parity = [0xa2ac]` (byte), the `*240` is the immediate `0xf0` at
   0x68210 (`IMUL EDX,[0xa2ac],0xf0`).
3. `FUN_000ae7f0(surface, x, y)` (0x68221..0x68224) — the Mode-X blitter
   (FU-56 §2); the host y already contains `parity*240`, so the blitter
   writes the undisplayed page.
4. Palette gate (0x68229/0x68232): if `[0x563fc] != 0` then `CLI` (0x68236).
5. Page select, always (0x68237..0x68250):
   `FUN_000d0620((signed)[0xa2ac] * 0x12c00 / 4)` = `parity * 0x4b00`
   (the `IMUL/SAR` sequence is a signed divide-by-4 of 0 or 0x12c00).
6. If and only if the palette flag was set (0x68255..0x6827c):
   `FUN_000ce8d7()` (retrace wait), `FUN_000ce754(0, 0x100, 0x560c0)` (DAC
   upload), `STI`, `CLD`, `[0x563fc] = 0`.
7. Flip (0x68282..0x68291): `AH = [0xa2ac]; AH ^= 1; [0xa2ac] = AH` —
   unconditional, unthrottled, after the page select.
8. Exit check (0x68297..0x682a6): if `EBP (3) == [0x1a2b4]`, call
   `FUN_00014df0` (keyboard/exit poll); non-zero -> state 3.
9. If state != 2, `FUN_00067e94` closes the player; loop while state != 3 and
   != 5 (0x682ad..0x682bf).

Order, with the conditional steps marked: **blit -> [CLI] -> CRTC start high
-> CRTC start low -> [vsync wait -> DAC upload -> STI] -> parity flip**. The
CRTC write is *inside* the interrupts-off window when the palette changed, and
is unthrottled otherwise.

## 3. Page flip (`FUN_000d0620`, 10 insns)

```
MOV EBX,[ESP+4]        ; start (dword units)
MOV EDX,0x3D4          ; CRTC index port
MOV AL,0x0C  MOV AH,BH ; index 0x0C = start address high
MOV CL,0x0D  MOV CH,BL ; index 0x0D = start address low
OUT DX,AX              ; high byte first
MOV EAX,ECX
OUT DX,AX              ; low byte second
```

The page unit is the dword: 240 rows x 80 plane bytes = 0x4B00 CRTC units =
0x12C00 plane bytes. The host passes `parity * 0x4b00` (0/0x4b00). No retrace
wait and no `0x3DA` read here; FU-56 §1's page arithmetic (`0xA0000 + y*80`,
`parity*0x4b00` display start) is confirmed. FU-56 §8 errata 3 stands: the
flip is unthrottled.

## 4. Palette upload (`FUN_000ce754`, 29 insns)

cdecl `upload(first, count, palette)`; host call `(0, 0x100, 0x560c0)`
(0x68267..0x68272; true buffer 0x1560c0, FU-55 §5).

1. `DX = 0x3C8`; `MOV EAX,first`; `OUT DX,AL` — the DAC write index is written
   **once**, resetting the internal auto-increment cursor (0xce75b..0xce764).
   No `0x3C6`, no `0x3C7` read, no pixel mask.
2. Per entry (`count` iterations, 0xce772..0xce798):
   * load source dword `EAX = [EBP]` (bytes R,G,B,pad), `OR 0xFF000000`, store
     to the shadow `[0x15bbac + first*4 + i*4]` (0xce77a);
   * `SHR EAX,2; AND EAX,0x3f3f3f` — per channel 6-bit = 8-bit `>> 2`
     (no rounding) (0xce77c..0xce784);
   * `LEA ESI,[ESP]; MOV ECX,3; OUTSB.REP DX,ESI` with `DX = 0x3C9` — three
     byte writes per entry in source memory order (the kVGT palette's R,G,B
     triples, FU-55 §5, so the hardware sees R,G,B) (0xce787..0xce78f).
3. Total: **1 + 3*count byte-port writes, no per-byte retrace check, no latch
   handling**. The 0x3DA wait is the single `FUN_000ce8d7` call the host makes
   before this function (0x68262).

The shadow at true link 0x15bbac (0x400 bytes) is the current-DAC record; it
is also written by `FUN_000ce70c` (same `out 0x3c8` / byte `out 0x3c9` shape,
shadow value `(rgb & 0x3f3f3f)*4 | 0xFF000000`) and by `FUN_000479a0` (builds
a 256-entry palette from the table at 0x4b200 into a caller buffer and the
shadow). It is bookkeeping, not presentation; the port does not model it.

## 5. Vertical retrace (`FUN_000ce8d7`, 17 insns)

```
MOV DX,0x3DA
MOV ECX,[0x14618]      ; timeout budget (static 0x186a0 = 100000)
CMP [0x15b18],1
JG  no_wait            ; mode >= 2: skip, reset budget to 0x186a0
JZ  wait_start         ; mode == 1: loop while bit3 clear
wait_end:              ; mode == 0 (or anything <1): loop while bit3 set
  CALL 0xce8d0
  TEST AL,8
  LOOPNZ wait_end
  JECXZ timeout
wait_start:
  CALL 0xce8d0
  TEST AL,8
  LOOPZ wait_start
  JECXZ timeout
timeout: [0x14618] = 0x2710 (10000); return
```

`FUN_000ce8d0` is `IN AL,DX; MOV AH,AL; IN AL,DX; AND AL,AH` — the status
port is read twice and ANDed before the bit test. `[0x115b18]` has **no
static writer** in the image: at rest it is 0, so the Mode-X host's upload
waits for the end of vertical retrace (bit 3 = 1 -> wait until clear) with a
100000-iteration budget; a timeout degrades the next wait to 10000. The
sibling `FUN_000ce943` (12 insns) waits start-then-end in the opposite order
and is not on this path.

Placement: the wait is called only in the palette-changed branch (0x68262),
after the CRTC select and inside the CLI window. It gates the DAC upload only.

## 6. `[0x12ae0]` — CPU path selector, not VGA

`FUN_0009fc98` (44 insns, called once by `FUN_00018680` right after
`FUN_00096c53`, 0x18680 decompile) probes the CPU:

* `SMSW EAX; AND EAX,1; [0x12ad8]=1; [0x12adc]=1; [0x12ae8]=CR0.PE`
  (0x9fc98..0x9fcb2);
* `PUSHFD/POP EAX; XOR EAX,0x40000; PUSHFD/POPFD; PUSHFD; POP EAX;
  XOR EAX,ECX; SHR EAX,0x12; AND EAX,1; [0x12ae0]=EAX` (0x9fcb7..0x9fcd7) —
  the EFLAGS.AC (bit 18) toggle test, i.e. **486+**;
* if that is 1, a second probe checks EFLAGS.ID and `CPUID` family
  (`CPUID 1; EAX>>8 & 0xF >= 5`), setting `[0x12ae4]` (**Pentium+**)
  (0x9fce1..0x9fd05).

`FUN_000baa48` reads the flag at 0xbaa49: `[0x12ae0] != 0` selects the
BSWAP dword-packing path (0xbaa56..0xbaae1), else the shift path
(0xbaae2..0xbab6d); both gather every 4th source byte identically (FU-56 §2).
So `[0x12ae0]` is a CPU capability flag used to pick an instruction sequence,
not a display or mode selector: FU-56 §7.2's "CPU-path selector" leg is closed
as a 486+ detect, and the brief's "`[0x12ae0]` path" is this one.

## 7. `FUN_00068108` — palette translation + quad rasterizer

Not a second Mode-X host: it never programs the DAC (no 0x3C8/0x3C9
writes), never writes the CRTC (0x3D4) and never waits on 0x3DA.

1. Gate `[0x1a2b8]`, state `[0x1a2b0]` (`!= 2` -> open), poll (0x6813b).
   `surface == 0` -> state 5, close, return (0x6816e..0x6818b).
2. `FUN_000ae760(0, 0x100, 0x560c0)` (0x68146..0x68152) then
   `[0x563fc] = 0` (0x6815e). The surface is passed with the caller's quad
   (`PUSH EDI`, EDI = original EAX) to `FUN_000a6040 (surface, quad)`
   (0x6815a..0x68164).
3. `FUN_000ae760` (45 insns): calls `FUN_000a21fc(0, 0x100, local)` — the DAC
   **read-back** (`out 0x3C7, index` then `in 0x3C9`, expanding 6-bit values
   as `b<<2 | (b&0x3f)>>4`, with the indirect hook `[0x1471c]`, true
   0x11471c) into a stack palette (0xae76c..0xae77a); sets `[0x14474]` to it
   (0xae79b); then for source entry `j = 0..count-1` reads RGB from the movie
   palette, calls `FUN_000a0980(R<<16|G<<8|B)` and stores the returned index
   at `[0x14720 + start + j]` — the pre-increment at 0xae7c2/0xae7cb turns
   the `[EDI+0x1471f]` store into table base **0x114720**; finally
   `[0x14474] = 0` (0xae7d7).
4. `FUN_000a0980` with `[0x14478] == 0` searches the 256 read-back entries for
   the minimum squared RGB distance, skipping entries whose flag
   `[0x1447c+i]` is 0; with `[0x14478] != 0` it indexes a 15-bit LUT instead.
5. `FUN_000a6040` builds a local window from the surface dimensions and calls
   `FUN_000b4fd0`, an affine/perspective textured-span rasterizer: it clips to
   the same half-open rect globals `[0x1131c8..0x1131d4]` and drives
   `FUN_000c2d54`, which maps every source pixel `p` through
   `[0x114720 + p]` and writes the destination byte only when the mapping is
   not `0xFF` (-1). Destination addressing comes from the host-installed row
   tables `[0x1131e8]`/`[0x1131ec]` (out of scope).

So the movie's indexed pixels are remapped to the palette already installed by
some other path (the DAC is read back, not rewritten); pixels with no
near-enough color are left as-is (transparent). FU-56 §8 errata 1 and FU-55
§10.1 are extended: `FUN_000a6040 -> FUN_000b4fd0` is a software quad
rasterizer with nearest-color translation, not a planar blitter.

## 8. Object-4 address shift (FU-56 §7.3 resolved)

FU-4's flat image places each LE object at its `RelocBaseAddress`: object 1
(code/constants) at 0x10000, object 4 (data/stack) at 0x100000. Inter-object
references are stored as displacements and patched by the DOS/4GW loader, so
an object-4 global that Ghidra renders as address `A` has true link address
`A + 0x100000` — the bytes at `A` are object-1 code. Measured:

* static reads: 0x131c8 = `89 4c 24 0e 8b 41 02 …` (object-1 code);
  0x1031c8 = `"NULL pointer\n"`; 0x1131c0 = `{640,480}` (clip caps),
  0x1131c8 = `{0,0,640,480}` (clip rect) — the initialized object-4 data;
* runtime patch (session-fvgt-2b `guest.bin`, object-1 physical base =
  entry 0x1282d20 - link 0x9fd10 = 0x11e3010): instruction bytes at
  0x124b1f0 (`MOV EAX,[w]`) = `A1 BC 3A 2E 00` -> operand 0x2e3abc =
  0x12abc + **0x2d1000**; the same load base reproduces every presentation
  ref tested: `[0x68200] A1 C0 3A 2E 00` (h, 0x12ac0+0x2d1000),
  `[0x68210] 69 15 AC B2 2D 00` (parity), `[0x68229] 8B 0D FC 73 32 00`
  (palette flag, 0x563fc+0x2d1000), `[0xae841] 8B 3D C8 41 2E 00` (clip
  left, 0x131c8+0x2d1000), `[0x681aa] B8 C0 6F 32 00` (open-name immediate,
  0x55fc0+0x2d1000).
* the object-4 copy in the same dump sits at physical +0x11b8010
  (`"NULL pointer\n"` 0x1031c8 -> 0x12bb1d8) with live values there:
  0x112abc = 320, 0x112ac0 = 200, 0x1131c0 = caps 320x200, clip
  `{0,0,320,200}`.

True link addresses of the presentation globals (Ghidra displacement +
0x100000): screen dims 0x112abc/0x112ac0; parity 0x11a2ac; host state
0x11a2b0/0x11a2b4/0x11a2b8; palette flag 0x1563fc; palette buffer 0x1560c0;
clip caps 0x1131c0/0x1131c4 and rect 0x1131c8/cc/d0/d4;
retrace budget 0x114618; wait mode 0x115b18; CPU flags 0x112ad8/…/0x112ae8;
translation table 0x114720; palette pointer/flag/LUT 0x114474/0x114478/
0x11447c; DAC read-back hook 0x11471c; DAC shadow 0x15bbac.

**Erratum (FU-56 §7.3):** the shift is `+0x100000`, not `+0xF0000` (0xF0000 is
only the difference between the two object bases; the stored displacement is
object-relative, so the whole object base applies). FU-55/FU-56 address labels
(0xa2ac, 0x563fc, 0x131c8, …) remain correct as *displacement-space* handles,
but their true link addresses are the +0x100000 values above.

## 9. Port mapping (`fifa96_vga`)

`include/fifa96_loader/fifa96_vga.h` + `src/fifa96_loader/fifa96_vga.c`:

| original | port |
|---|---|
| CRTC start `parity*0x4b00`, high-then-low, index 0x0C/0x0D | `fifa96_vga_page_start`, `fifa96_vga_crtc_start_high/low`, `FIFA96_VGA_CRTC_*` |
| host y addend `parity*240` | `fifa96_vga_page_rows` (reuses `fifa96_blit_page_rows`) |
| page byte offset `parity*0x12c00` | `fifa96_vga_page_bytes` |
| `XOR AH,1` flip (0x68288..0x68291) | `fifa96_vga_flip(parity)` |
| upload `out 0x3C8,first` + `3*count` x `out 0x3C9, byte>>2` | `fifa96_vga_palette_upload` -> ordered `{port,value}` records |
| full order incl. conditional vsync/DAC | `fifa96_vga_present_order(palette_changed,…)` |
| CLI/STI window, port timing, 0x3DA wait, DAC shadow | not modeled (documented) |
| blitter, page memory, quad path | unchanged (`fifa96_blit`); `FUN_00068108` path not ported |

Divergences: the original writes `first` unmasked and wraps the DAC cursor;
the port rejects `first > 255`, `count > 256` or `first+count > 256` as
`-FIFA96_ERR_TRUNCATED`. The original emits the `0x3C8` write even for
`count == 0` (the port returns one record and accepts `rgb == NULL`). No
globals, no I/O, no allocation in the port; it returns the sequence as data.

## 10. Port tests (`tests/test_vga.c`, suite 46 -> 47)

* Pages: `page_rows` 0/240/480, `page_start` 0/0x4b00/0x9600, `page_bytes`
  0/0x12c00/0x25800; CRTC high/low split of 0x4b00/0x12c00/0x1234.
* Flip: 0 -> 1 -> 0, 2 -> 3, 0xFFFFFFFF -> 0xFFFFFFFE (strict XOR).
* DAC component: 0x00/0x03/0x3F/0x40/0x7F/0x80/0xFF -> shift-only 6-bit.
* Palette records: exact 1 + 3*count ordering for a two-entry upload,
  `first = 255` boundary, `count = 0` (index-only), full 256-entry upload
  (769 records, last value = `rgb[767] >> 2`); capacity short leaves the
  buffer untouched; NULL `out`/`rgb`, `first = 256`, `first+count > 256`,
  `count > 256` all `-FIFA96_ERR_TRUNCATED`.
* Present order: palette-changed 6-step and plain 4-step sequences in exact
  order, plus capacity/NULL errors.
* ASan/UBSan clean (`-fsanitize=address,undefined`, leak detection on).

## 11. Open legs

1. Real VGA timing (retrace phase, DAC update window, page tearing, the
   `[0x115b18]` wait mode) is hardware behavior; the port records the sequence,
   not the timing.
2. `[0x115b18]` has no static writer in the enumerated image; at rest it is 0
   (wait for retrace end). Which runtime or config path sets 1/>=2 is not
   resolved.
3. `[0x14618]`'s second dword (0x11461c = 0x2e9c) is not used by
   `FUN_000ce8d7`/`FUN_000ce943`; its owner is not derived.
4. The `FUN_00068108` quad rasterizer internals (edge tables 0x1131e8/
   0x1131ec, span stepping, the `[0x1447c]`/`[0x14478]` palette-search state)
   are not ported.
5. Whether `[0x112abc]`/`[0x112ac0]` hold 240 during Mode-X movie playback:
   they are runtime-written (static zero); the committed front-end dump holds
   320x200. Only the page addend 240 is hardcoded.
6. Retail-build equivalence of the trailing-column drop and these register
   sequences is unverified (as FU-56 §7.5).

## 12. Errata (quoted)

1. FU-56 §7.3: "object-4 data references are shifted by `0xF0000`" — measured
   fixup is `+0x100000` (§8). The open leg is closed with that correction.
2. FU-56 §4: "320 = `[0x12abc]`; 240 = `[0x12ac0]`" — the globals are
   object-4, zero at rest and written at runtime; the committed dump holds
   320/200. The 240 page addend is the immediate at 0x68210, not `[0x12ac0]`.
3. Brief: "do they wait for retrace before each byte? use the latch bit? how
   many entries?" — one wait per upload (`FUN_000ce8d7`, mode 0 -> retrace
   end), no per-byte wait, no 0x3DA latch/bit-0 handling, 256 entries
   (0x6826c), 1 + 768 byte-port writes.
4. Brief: "FUN_00068108/FUN_00068194 call the blitter" — only `FUN_00068194`
   reaches `FUN_000ae7f0` (FU-56 errata 1 stays); `FUN_00068108` presents
   through the quad rasterizer and never programs the DAC or CRTC.

## 13. Provenance

* Ghidra `/fifa96_le.bin`: disassembled 0x68194 (103 insns, quoted
  address-by-address), 0x68108 (45), 0xae7f0 (120), 0xbaa48 (110), 0xce754
  (29), 0xce8d7 (17), 0xce943 (12), 0xce8d0 (5), 0xd0620 (10), 0xd064b (5),
  0x9fc98 (44); decompiled 0x68194, 0x68108, 0x6844c, 0xce70c, 0x9fc98,
  0x18680, 0xa6040, 0xb4fd0, 0xae760, 0xa21fc, 0xa0980, 0xc2d54, 0x479a0,
  0x14df0; xrefs
  `get_function_callers 0x68194` (1), `0x68108` (2), `0x9fc98` (1),
  `get_xrefs_to 0x12ae0` (1 read/1 write), `0x15b18` (1 read, 0 writes),
  `0x14618` (5 refs), `0x5bbac` (7), `0x1471f` (1); instruction search
  `1471f` (1 site); memory reads 0x131c8, 0x1031c8, 0x1131c0, 0x1131c8,
  0x114618, 0x115b18, 0x112abc, 0x112ad8, 0x155fc0.
* Runtime ground truth: `captures/session-fvgt-2b/guest.bin` (42307584 B,
  physical dump): object-1 base delta 0x11e3010 (entry anchor 0x1282d20 vs
  link 0x9fd10), object-4 patch base 0x2d1000 (operand deltas), object-4
  static copy at +0x11b8010 (`NULL pointer\n` 0x12bb1d8 = 0x1031c8 + delta).
* Port: `include/fifa96_loader/fifa96_vga.h`,
  `src/fifa96_loader/fifa96_vga.c`, `tests/test_vga.c`; `make test` 46/46
  before, 47/47 after; `test_vga` ASan/UBSan-clean with leak detection.
