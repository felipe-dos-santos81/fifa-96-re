# FU-86: sprite resource loading — PLAYART.PVI, the SHPI bank and the byte-plane renderer path

Follow-on to FU-85 §2/§5 (sprite bank loading cited only; `0x14720` and the
"4-plane" reading left open). This slice derives the player sprite source
file, the per-bank container and decode chain, the frame format, the
animator-stride rule, and the renderer's pixel path instruction-by-instruction;
the clean bank/frame/geometry/remap math is ported as `fifa96_sprite`.

Result in one line: **`art/playart.pvi` is loaded by `FUN_0004AAE0` (called
from the match art init `FUN_0004AC6C`) through `FUN_0004A344` (open
`FUN_00068FD0`, size probe `FUN_0009E890`, allocate `FUN_00098BF8`, decode
`FUN_0009E860`, free) and its *decoded* form is stored at `0x4C0DC`, which is
`FUN_0004AFB8`'s resource-table slot `0x47`; the decoded file is a **BIGF v2**
container of 91 entries (`FUN_000A2718` count, `FUN_000A275C` by-index), and
`FUN_00078F1C` fills the 0x18-stride animator records `0x57DE8` with
`{index, entry, probe, handle}`; an entry is either a raw **SHPI** bank
(`.fsh`) or a **nested huff(0x31) → refpack(0x10) → tree(0x46) → SHPI**
stream (`.qfs`) that `0x78DAC` decodes on demand (first stage into the
`afile%d` buffer; when the selector's low bit is set, two further
`0x9E860` passes through `TEMPBUF` and back); the animator stride `rec+4` is
`count/divisor` selected by the `0x78B5C` switch on the bank index; a SHPI
bank is `{"SHPI", u32 size, u32 count, "GIMX", count×{char[4] name, u32
offset}, frames}` — exactly the "segmented handle" of `FUN_00078FAC`
(`[+8]` count, `data = handle + dword[handle+off*8+0x14]`); each frame is a
16-byte header (`u8 0x7B`, `u24 second_offset`, `u16 w`, `u16 h`, `u16
pivot_x`, `u16 pivot_y`, `u32 word12`) followed by **w×h 8-bit chunky
pixels** at `sprite+0x10`; the renderer's `0x5BFB4` is **not a 4-plane
pointer table** — `FUN_000B1730` fills it with a w-entry scaled
**column-index** table and `FUN_000CEABC` writes one remapped byte per output
pixel through the 256-byte `0x14720` palette buffer with `0xFF` transparent
(FU-85 §2/§5 corrected).**

## Method

* Static work on the open Ghidra MCP session for `/fifa96_le.bin` (FU-4 linear
  image, link addresses). `disassemble_function`/`disassemble_bytes` are the
  citation source; the mis-analyzed spans at `0x4AAE0` and `0x4A6E3..0x4A7F6`
  were re-decoded linearly (`ndisasm -b32`) and cross-checked against
  `disassemble_bytes` (the same FU-79/FU-84/FU-85 method). No decompiler output
  is quoted where the listing is clean.
* **Address mapping (FU-76/FU-84/FU-85, restated).** Code/function addresses
  equal true link addresses; a **data immediate** `A` is storage at flat
  `A+0x100000` (`[0x4C0DC]` → `0x14C0DC`, `0x1D4C` → `0x101D4C`); stored code
  pointers and inline `CS:` tables resolve through `+0x10000` (the `0x68B5C`
  `CS:` jump table is flat `0x78B5C`).
* **Data cross-check.** The retail container bytes were taken from the
  read-only `game/FIFAPCCD96.iso` (`/ART/PLAYART.PVI`, 807380 B;
  `/ART/GAMEART0.PVI`, 154387 B) and decoded with the *committed* ported
  codecs (`fifa96_refpack_decode`, `fifa96_huff_decode`, `fifa96_tree_decode`);
  all 87 `.qfs` payloads reach SHPI at the declared sizes (77 from the exact
  record slice, 10 with the container tail supplied — §3 caveat), and the SHPI
  structure was validated across all 91 entries. These are asset quotes, not
  Ghidra memory quotes.
* Every numeric claim is quoted from the listings/reads/decodes; unproven items
  are open legs (no guessed labels).

## 1. Loading path: `art/playart.pvi` → resource slot `0x47`

`FUN_0004AAE0` (hand-decoded from the read at `0x4AAE0`, 0x40 B; `ndisasm`):

```
0x4AAE0  PUSH EBX/ECX/EDX
0x4AAE3  TEST EAX,EAX
0x4AAE5  JZ 0x4AAFD
0x4AAE7  MOV EDX,0x1D44              ; memory-block name (flat 0x101D44 = "PLAYER")
0x4AAEC  MOV EAX,0x1D4C              ; file name  (flat 0x101D4C = "art/playart.pvi")
0x4AAF1  CALL 0x4A344                ; load+decode file -> caller-freeable buffer
0x4AAF6  MOV [0x4C0DC],EAX           ; resource table slot: 0x4BFC0 + 0x47*4 = 0x4C0DC
0x4AAFB  JMP 0x4AB17
0x4AAFD  MOV EAX,[0x4C0DC]           ; EAX == 0 branch: free old container ...
0x4AB02  TEST EAX,EAX / JZ 0x4AB0F
0x4AB07  CALL 0x993EC / ADD ESP,4
0x4AB0F  XOR EDX,EDX
0x4AB11  MOV [0x4C0DC],EDX           ; ... and zero the slot
0x4AB17  CALL 0x78F1C                ; (re)build the 0x57DE8 animator records
0x4AB1C  POP EDX/ECX/EBX / RET
```

The slot arithmetic is exact: `FUN_0004AFB8` = `MOV EAX,[EAX*4+0x4BFC0]`, so
index `0x47` reads flat `0x14C0DC` = `[0x4C0DC]`. `FUN_0004A344`
(`0x4A344..0x4A38D`) is the whole-file loader:

```
0x4A34A  EBX = 0x20                                  ; mode/flag
0x4A351  EAX = 0x8A020; CALL 0x68FD0                 ; open file context (FU-41 family)
0x4A360  CALL 0x9E890                                ; probe: decoded size (0 = raw)
0x4A36A  CALL 0x98BF8(name, size, 0)                 ; allocate named buffer
0x4A376  CALL 0x9E860(raw_file, buffer)              ; decode record
0x4A37F  CALL 0x993EC(raw_file)                      ; free the raw file image
```

`FUN_0004AC6C` is the match art init (guarded once by `[0x74A0]`): it calls
`0x4AAE0(1)` (`0x4ACCC MOV EAX,1; 0x4ACD1 CALL 0x4AAE0`) and then
`FUN_0004A6BC` (`0x4ACE5`), which loads `art/gameart0.pvi` (`0x1C6C`,
name `GAMEART` `0x1C64`) and fills resource slots `0..0x3E` by name lookup
(FU-85 leg 4 context). Its callers are `FUN_00018108`, `FUN_00018680` and the
match setup `FUN_0004AD4C` (`0x4AD4C`), which immediately follows with the
`0x280×0x1E0` surface allocation (`FUN_0009A45C(0x280,0x1E0,0)`, decompiled)
and the FU-85 drawable-table allocator `FUN_00058E00`. So the sprite container
is a **one-shot match/stadium art load**, not a per-frame resource.

`FUN_0004A344`'s probe (`FUN_0009E890`, `0x9E890..0x9E8C6`) resolves the record
pointer (`0x9E3EC`, skipping a leading NUL-terminated name when
`FUN_000AF37B(9,"Copyright") == 0`), classifies the codec
(`FUN_0009E420`), and for classes `1..0x1E` returns the 24-bit big-endian
length at bytes `[2..4]` (`BSWAP dword[ptr+2] >> 8`, `0x9E8B0..0x9E8C2`);
raw containers
("BIGF", "SHPI") classify to 0 and return 0.

## 2. Container: refpack → BIGF v2 → 91 banks

`PLAYART.PVI` starts `10 FB 0E 09 56` — an even-selector refpack stream
(`0x10 FB`, `FUN_0009E890` class 0xC = `lz_refpack_decode @ 0xB18F8` via the
`0x9E7E4..0x9E7F2` dispatch). Decoding the file with the committed
`fifa96_refpack_decode` yields **919894** bytes (declared `0x0E0956`) whose
first four bytes are `42 49 47 46` = **"BIGF"**. The BIGF v2 directory (FU-41
§1) parses exactly:

```
+0x00 "BIGF"        +0x04 BE32 919894 (file size == decoded size)
+0x08 BE32 91      +0x0C BE32 0x6FD (directory end)
+0x10 record[91]: [BE32 data offset][BE32 size][name NUL-terminated]
```

and is the structure `FUN_00078F1C` walks (below): `FUN_000A2718` = v2 count at
`+8`, `FUN_000A275C(base,base,i)` = `base + record[i].offset` (by-index BIGF
walk; FU-41 §1). Entry names:

```
  0 xstandd.fsh   1 walk.fsh    2 jog.fsh   3 run.fsh
  4 jump.qfs      5 throwin.qfs 6 kick.qfs  7 shoot.qfs 8 header.qfs ...
 90 strutb.qfs
```

Four raw `.fsh` banks and 87 `.qfs` banks; all 91 names have size and offset
inside the file and the records are contiguous (`entry[i].off+size ==
entry[i+1].off`).

### 2.1 `FUN_00078F1C` record build (`0x78F1C..0x78FAB`)

```
0x78F25  EAX=0x47; CALL 0x4AFB8          ; container pointer (slot 0x47)
0x78F36  CALL 0xA2718                    ; count = BIGF entry count (91)
loop i in 0..count-1:
  0x78F4C  [0x57DE8 + i*0x18 + 0] = i                       ; bank index
  0x78F52  CALL 0xA275C(container, i) -> EAX                ; entry pointer
  0x78F5B  [rec+0x10] = EAX                                 ; raw entry
  0x78F61  CALL 0x9E890(EAX) -> EAX                         ; final decoded size probe
  0x78F69  [rec+0xC] = EAX
  0x78F73  if (EAX != 0) [rec+0x14] = 0        else [rec+0x14] = entry; CALL 0x78C68
0x78F9D  CALL 0x78B20                    ; 0x57DE8 array clear (FU-84)
```

So `+0xC` is the compression probe (`0` for `.fsh`, final size for `.qfs`),
`+0x10` the on-disk entry, `+0x14` the handle (raw entry when uncompressed;
`0` until `0x78DAC` lazily decodes a `.qfs`). `0x57DE8` itself is BSS.

## 3. `.qfs` decode chain: huff → refpack → tree → SHPI

`FUN_0009E860(src,dest)` = one `FUN_0009E718(src,dest,strict=1)` dispatch. The
selector (byte 0, masked `&0xFE`) maps to the committed decoders:

| selector | class | dispatch | ported decoder |
|---|---|---|---|
| `0x10/0x11` | 0xC | `0x9E7E4` → `CALL 0xB18F8` | `fifa96_refpack_decode` |
| `0x16` | 0xF | `0x9E7F4` → `CALL 0x9E1E4` | (16FB variant; not needed here) |
| `0x30/0x32/0x34` | 0x19/0xB/0x17 | `0x9E7FF` → `CALL 0x9C7E0` | `fifa96_huff_decode` |
| `0x46` | 7 | `0x9E80F` → `CALL 0x9DA14` | `fifa96_tree_decode` |
| `0x60/0x62/0x66` etc. | — | `0x9E81A` → `CALL 0x9DC1C` | other sunpack methods (raw copies skipped) |

`PLAYART.PVI`'s `.qfs` entries start `31 FB 00 91 54 00 2C 7B` (e.g.
`jump.qfs`): odd selector `0x31` (huff), a two-size 8-byte header —
**bytes `[2..4]` = final decoded size `0x009154` = 37204**, bytes `[5..7]` =
first-stage size `0x002C7B` = 11387 (the odd form the decoder reads after
skipping 3 bytes; `fifa96_refpack`/`fifa96_huff` both implement it). Decoding
the chain with the committed decoders:

```
jump.qfs : huff   -> 11387 B (starts "10 FB" refpack)
           refpack-> 11886 B (starts "46 FB" tree)
           tree   -> 37204 B (starts 53 48 50 49 "SHPI")
kick.qfs : 9469 -> 9752 -> 31148 = SHPI
gstand.qfs: 2148 -> 2163 -> 5422 = SHPI   (and 74 other .qfs entries)
```

Decoded-slice caveat: 77 of the 87 `.qfs` entries decode from their exact BIGF
record slice; the other 10 (`stumble`, `kneesld`, `vollkcka`, `gldsa`,
`dummyrfa`, `collairb`, `duckflpa`, `duckflpb`, `bodychk`, `xrready`) make the
committed, bounds-checked `fifa96_huff_decode` return `TRUNCATED` on the exact
slice but decode to the declared first-stage size (12667/8983/12339/... when the
tail of the container is supplied — the original `0x9C7E0` has no input bound
and reads the bytes after the record, which in the live container are the next
BIGF entry). With the container tail supplied all 87 `.qfs` payloads reach
`SHPI` at the declared sizes. The game passes `rec+0x10` (an interior pointer,
not a bounded slice), so this over-read exists only as a port-safety
divergence (open leg 9).

The game's path is the same with its own buffers (`FUN_00078DAC`,
`0x78DAC..0x78EFF`; the loader is only called when `[rec+0x14] == 0`):

```
0x78DB7  byte[0x58674 + rec[0]]++                 ; per-bank load counter / cache tick
0x78DBF  local_1c = rec[0xC] + 0x10               ; final decoded size + 16
0x78DD6  loop (max 0x1E) over FUN_00099CD8() free-block query >= 2*local_1c:
             FUN_00078D5C frees/advances the 0x57D70 LRU slots
0x78E30  FUN_00099E9F(buf, "afile%d", rec[0])     ; format at 0x2CAC, arg = bank index
0x78E47  handle = FUN_00098C38(buf, local_1c, 0)  ; allocate named buffer; [rec+0x14] = handle
0x78E5E  FUN_0009E860(rec[0x10], handle)          ; stage 1
0x78E66  if (entry[0] & 1) {                      ; odd selector = nested wrapper
0x78E80     second = FUN_00098C38("TEMPBUF", local_1c, 0x20)   ; name at 0x2CB4
0x78EAB     FUN_0009E860(handle, second)          ; stage 2
0x78EB7     FUN_0009E860(second, handle)          ; stage 3 (overwrites stage 1)
0x78EC0     FUN_000993EC(second)                  ; free temp
         }
0x78ECA  CALL 0x78C68(rec)                        ; stride (below)
0x78ED4  ring[0x57D70 + 0x58670] = &rec[0x14]; LRU index++
```

The buffer-sizing `2*(size+16)` is only a free-block gate (`0x78DE2 CMP
EDX,EDI; JGE`); the allocation is `size+16` (`0x78E3E PUSH EBX` = local_1c).
The three stages fit because each intermediate is smaller than the final
declared size. Buffer names: `"afile%d"` (`0x102CAC`), `"TEMPBUF"`
(`0x102CB4`), block name `"PLAYER"` for the container itself.

## 4. Bank format: SHPI

A raw `.fsh` entry and every fully decoded `.qfs` payload is:

```
+0x00 char[4] "SHPI"           (4B 50 48 53 LE)
+0x04 u32 LE total_size        (== the BIGF entry size for raw banks)
+0x08 u32 LE count             (== [handle+8] read by 0xA1918)
+0x0C char[4] "GIMX"           (tag; observed on all 91 entries)
+0x10 count x { char[4] name; u32 LE offset }     ; offset absolute in the bank
...        frame data (offsets point here)
```

This *is* the segmented handle the resolver uses: `FUN_000A1918`
(`0xA1918..0xA191F`) is `return dword[handle+8]`, and `FUN_000A1920`
(`0xA1920..0xA1936`) is
`return off < count ? handle + dword[handle + off*8 + 0x14] : 0` — the dword
at `+0x14 + i*8` is exactly the `u32 offset` field of directory record `i`
(`+0x10 + i*8`), so a resolver frame pointer is `SHPI + entry_frame_offset`.
The 4-byte name is the entry's second dword (FU-85: "second field not read by
this path").

### 4.1 Animator stride `rec+4` — the `0x78B5C` switch

`FUN_00078C68` (`0x78C68..0x78D59`) sets `rec+4`:

```
0x78C70  handle = rec[5]; if (handle == 0) { rec[1] = 0; return }
0x78C7F  EDX = rec[0] - 0x18; if (EDX > 0x42) goto default
0x78C8D  JMP CS:[EDX*4 + 0x68B5C]         ; flat 0x78B5C, 67 stored code pointers
...
default 0x78D38: rec[1] = FUN_000A1918(handle) / 5     (signed IDIV)
```

The table targets (link = stored + 0x10000): `0x78CD6` = count/4, `0x78C95` =
count/3, `0x78CB7` = count/2, `0x78CF9` = count/8, `0x78D1C` = fixed 8,
`0x78D2A` = fixed 5, `0x78D38` = count/5. Bank indices (table slot
`bank-0x18`): `/4` at `18,19,25,26,28,29,2C,2D`; `/3` at
`22,37,3A,3E,41,45,4E,56,5A`; `/2` at `23,38,3B,3F,42,46,4F,57,5B`; `/8` at
`2E`; fixed 8 at `27,2A`; fixed 5 at `2F`; `/5` for every other bank `18..5A`
and for all banks outside `18..5A`. So a sprite bank's direction stride is
`count/5` in the common case and the divisor encodes how many directions the
bank stores (5 mirrored directions for walk: see below).

### 4.2 Frame layout

```
+0x00 u8  0x7B                 (tag; all 185 frames of the 4 PLAYART .fsh banks)
+0x01 u24 second_offset        (LE, 0 = no second chunk)
+0x04 u16 w                    ([sprite+4] read by FUN_000A2C50 0xA2C6E)
+0x06 u16 h                    ([sprite+6] 0xA2C71)
+0x08 u16 pivot_x              ([sprite+8] 0xA2E5A)
+0x0A u16 pivot_y              ([sprite+10] 0xA2E9C)
+0x0C u32 word12               (bank-constant; 0 for most, 53 for walk)
+0x10 w*h bytes                (8-bit indices, row-major, stride == w)
     second_offset bytes       (when nonzero: pad to it, then the second chunk)
```

The header words are read as `SAR`-16 of dwords at `+2,+4,+6,+8` (the compiler
idiom for 16-bit fields at `+4,+6,+8,+10`). Real bytes (decoded file):
`xstandd.fsh` `p001` at `0x38` is `7B E4 03 00 14 00 31 00 0A 00 2E 00 00 00
00 00` → `second=0x3E4`, `w=20`, `h=49`, pivot `(10,46)`, `word12=0`, pixels
at `+0x10` (980 B), and the bank's five frames are the five standing poses.
`walk.fsh` (60 frames = 12 per direction × 5 directions, stride `60/5 = 12`)
names the groups `w0xx`, `w2xx`, `w4xx`, `w6xx`, `w8xx` — i.e. the fifth
direction grouping that the `count/5` stride encodes. `jump.qfs` decodes to
30 frames = 6 × 5 (`j004..j026`, `j204..j226`, ...). `second_offset` cases
observed: player banks `second = 16 + align4(w*h)` with a 24-byte second chunk
(identical for every frame of a bank; bytes for walk
`7C 00 00 00 02 00 00 00 7B 00 00 00 B4 FF FF FF 10 00 00 00 00 00 00 00`),
`jog.fsh`/`ball.fsh` `second = 0`, `PALteam.fsh` `second = 20` with a 784-byte
second chunk (16+768 palette-sized), `PALsys.fsh` `second = 256`. The second
chunk's payload semantics are open (leg 1).

## 5. Renderer consumption (FU-85 §2 corrected)

The blit chain is unchanged from FU-85 §2 — `FUN_00057080` (scale) →
`FUN_000A2E24` (pivot, `0xA2E24`) → `FUN_000A2C50` (clip + stepping,
`0xA2C50`) — but the pixel path reads:

```
0xA2DCA  PUSH 0x5BFB4; CALL 0xB1730(src_x, 0, dx, 0, <row width>, w)
0xA2DDB  PUSH 0x5BFB4
0xA2DE0  EAX = sprite + 0x10
0xA2DE4  CALL 0xCE9D8                  ; [0x14828]=sprite+0x10, [0x14824]=0x5BFB4
per row (0xA2DF2..0xA2E19):
  0xA2E05  CALL 0xCEABC(rowbase, (src_y>>16)*sprite_w, w)
  0xA2E15  rowbase += [0x131C0]; src_y += dy
```

`FUN_000B1730` (`0xB1730..0xB17FB`) writes `2*ceil(count/2)` dwords:
`cols[2k] = (row>>16)*width + (col>>16)` and
`cols[2k+1] = ((row+rowstep)>>16)*width + ((col+step)>>16)`, then
`row += 2*rowstep`, `col += 2*step` (power-of-two width uses a shift at
`0xB175D..0xB17B0`, the general branch an `IMUL` at `0xB17B2..0xB17F6`).
Called from the sprite path with `row = rowstep = 0`, so `0x5BFB4` holds the w
**source column indices** for the scaled output row (nearest-neighbour x
scaling; the y scaling is the `src_y` 16.16 stepping).

`FUN_000CEABC` (`0xCEABC..0xCEB5B`) then:

```
0xCEACA  ESI = arg1(src_offset) + [0x14828]     ; + sprite+0x10
0xCEAD0  EDI = arg2(dest_offset) + [0x131EC]    ; VGA window write base
0xCEAD6  EDX = [0x14824]                        ; 0x5BFB4 (column table)
loop 4 at a time (0xCEAE3..0xCEB36):
  EBX = [EDX];   AL = [EBX+ESI]; AL = [0x14720+AL]; if (AL != 0xFF) [EDI] = AL
  EBX = [EDX+4]; ... [EDI+1] ... [EDX+8] ... [EDI+2] ... [EDX+0xC] ... [EDI+3]
  EDX += 0x10; EDI += 4; ECX -= 4
tail 0xCEB38..0xCEB57: same for the last 1..3 columns, [EDX += 4]
```

So each of the four reads per unrolled iteration is **one output pixel column**
(`cols[4k..4k+3]`), not a plane; the source is a single 8-bit chunky image at
`sprite+0x10`, fetched at `src + row*w + col`. The destination is the VGA
byte surface: stride `[0x131C0]`, y-indexed scanline table `[0x131E8]`, clip
`[0x131C8..0x131D4]`, workspace `[0x131EC]` (FU-85 §5). Mirroring is not a
per-pixel flag: `FUN_000A2C50` builds `src_x` as `sprite_w<<16 + dx/2` when the
destination width is negative (`0xA2D29..0xA2D36`), so the column table itself
walks the source right-to-left; `FUN_000A2E24` mirrors the pivot
(`0xA2E78..0xA2ED6`).

**`0x14720`** is a 256-byte palette-translation buffer: `FUN_000CE980`
copies 0x40 dwords *into* it (`0xCE985 MOV EDI,0x14720; 0xCE992 MOVSD.REP
ECX=0x40`) and `FUN_000CE998` copies out; `FUN_00048DC0` installs the
per-entity palette from `[slot*4+0x4BF60]` (FU-85 §2). A sprite byte is a
palette-relative colour index; after translation `0xFF` means transparent
(the check is on the **translated** byte, `0xCEAF1 CMP AL,0xFF`), so a palette
entry may also map an index to transparency.

## 6. Overlay tables and the composite classes

* `0x10F2E7` (FU-85 §1.3, leg 7) is indexed by the composite raw frame
  **index** `rec[+4]*dir + sprite` (not a byte offset): the handler reads
  `m = byte[raw + 0x10F2E7]` and, when nonzero, resolves overlay frame `m-1`
  through the second bank. Its runs of consecutive non-zero values now have a
  bank-side explanation: the overlay banks also store 5 directions
  (`count/5` stride), so the table supplies the overlay frame for the 5 valid
  direction slots and leaves the mirrored slots zero. Verified shape remains
  as FU-85 quoted; exact per-animation grouping stays open (leg 7 there).
* `0x1584D8`/`0x1584F0` (ids `0x60/0x61`) remain BSS with **no static
  writer** (`get_xrefs_to` on both flat addresses returns none); the runtime
  populator is still open (FU-85 leg 8).
* The frame's `second_offset` chunk (24 B for player banks, 784 B for
  `PALteam/PALsys`) is new; its payload is not consumed by the derived blit
  path (leg 1).

## 7. Port: `fifa96_sprite`

`include/fifa96_loader/fifa96_sprite.h` +
`src/fifa96_loader/fifa96_sprite.c` (caller-owned buffers, no globals,
negative `fifa96_err_t` for invalid arguments, no comments).

| original | port |
|---|---|
| SHPI header/directory read (`0xA1918` count at `+8`, directory `+0x10+i*8`, offsets `+0x14+i*8`) | `fifa96_sprite_bank_parse` + `fifa96_sprite_bank_entry` |
| frame header `{0x7B, u24 second, w, h, pivot_x, pivot_y, word12}` + `w*h` pixels at `+0x10` (`0xA2C6E..0xA2C77`, `0xA2E56..0xA2E9F`, `0xA2DD7..0xA2DE4`) | `fifa96_sprite_frame_parse` |
| `FUN_00078C68` `0x78B5C` switch (`count/4,3,2,8,5`, fixed 8/5) | `fifa96_sprite_stride` |
| `FUN_000B1730` pair-written column table (`0xB1747..0xB17F6`) | `fifa96_sprite_columns` |
| `FUN_000CEABC` remap + `0xFF` skip (`0xCEACA..0xCEB57`) | `fifa96_sprite_span` |
| file open/decode chain (`FUN_0004AAE0`/`FUN_0004A344`/`0x78DAC`), BIGF/C container, buffer allocation, palette install, VGA window, `0x5BFB4`/`0x14720` globals | not ported (resources/hardware; cited in §1–§5) |

Divergences (documented): the original trusts the SHPI directory and reads
`w*h` pixels with no bound (the only guard is the buffer allocation); the port
rejects a truncated bank/frame, an out-of-range entry index, a `second_offset`
that lands inside the first image, and a column index beyond the caller's
`src_len`. The original's `FUN_000B1730` writes one extra dword for odd `count`
(its `2*ceil(count/2)` shape); the port reproduces that shape faithfully, so
callers size the table `2*((count+1)/2)`. The port's `fifa96_sprite_columns`
takes `width` explicitly (the original reads it from the caller's `EBP`, dead
when `rowstep == 0`).

## 8. Tests (`tests/test_sprite.c`, suite 74 → 75)

* Layout `_Static_assert`s (`fifa96_sprite_bank`/`entry`/`frame`).
* `bank_parse`: synthetic two-entry SHPI, magic/size/count/tag reads, bad
  magic, short buffer, `total > len`, directory overflow (`count` huge), count
  0, NULL.
* `bank_entry`: name/offset read, index boundary, NULL.
* `frame_parse`: real `xstandd p001` header bytes (`7B E4 03 00 14 00 31 00 0A
  00 2E 00 00 00 00 00`) → `second 0x3E4`, 20×49, pivot `(10,46)`, 980 pixels;
  zero `second` (ball-like), second inside the image rejection, offset past
  total, truncated pixels (`w*h` exceeds the bank), NULL.
* `stride`: every switch class (`0x18 /4`, `0x22 /3`, `0x23 /2`, `0x2E /8`,
  `0x27`/`0x2A` = 8, `0x2F` = 5, default `/5`, out-of-range `/5`), count 0,
  real walk (bank 1, count 60 → 12) and jump (bank 4, count 30 → 6).
* `columns`: count 1/2/3 (odd count writes the extra pair), zero step,
  power-of-two vs general width, nonzero row/row_step, negative `src_x`, NULL.
* `span`: identity remap, palette translation, `0xFF` skip (including a remap
  entry that maps an opaque index to `0xFF`), column past `src_len`, NULLs,
  count 0.
* `make test`: 74/74 before, **75/75 after**;
  `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
  tests/test_sprite.c src/fifa96_loader/fifa96_sprite.c` runs clean.

## 9. Errata (quoted)

* FU-85 §2 "reads the four source **planes** from the pointer table `0x5BFB4`
  (`[0x5BFB4+p*4] + src_offset`, `0xCEAE3..0xCEB20`)" and §5 "the sprite source
  planes are pointed by the scratch table `0x5BFB4`" — **corrected**: the
  table is filled by `FUN_000B1730` (`0xA2DCF`, `0x9B24F`) with w scaled
  **column indices**, and the four reads per unrolled iteration are four
  consecutive columns of one 8-bit chunky image (`FUN_000CEABC`: `EBX =
  [EDX+4p]`, `AL = [EBX+ESI]`, `[0x14720+AL]`, one byte per output pixel).
  Sprite frames are `16 + w*h` bytes with stride `w`, not planar; "plane" in
  the FU-85 table is a misnomer for the column table.
* FU-84 §11 leg 1 / FU-85 leg 5 "row `+8` sprite-bank semantics ... the
  `0x57DE8` record count (resource entry `0x47` length)" — **closed**: slot
  `0x47` is `art/playart.pvi` (§1), the count is the BIGF directory count (91),
  and the record's `+4` stride is the `0x78B5C` switch on the bank index over
  the SHPI `count` at `+8` (§4.1).
* FU-85 §1.2 "a bank is a segmented resource: a 4-byte entry count at `+8`,
  then 8-byte entries at `+0x14` whose first dword is a displacement" —
  **extended**: the handle is a SHPI container; the entry's first dword is the
  frame's **absolute offset in the SHPI**, and the entry's second dword (the
  `[+0x14+i*8-4]` name at `+0x10+i*8`) is the 4-char frame name (`w001` etc.).
* FU-85 §1 "sprite pixels are 4-plane banks ... written by `0xA2C50`→
  `0xCEABC` into the VGA surface block" — **corrected** as the first erratum;
  the destination is a byte surface, and the span writer applies the palette
  remap with `0xFF` = transparent.
* Brief "sprite banks may come from `.VIV`/`.PVI`/`.SPR`-style containers; the
  ported record codecs (refpack/huff/tree) may decode sprites" — **confirmed
  and pinned**: `/ART/PLAYART.PVI` (raw refpack), BIGF v2 inside, `.qfs`
  entries decode through exactly `huff(0x31) → refpack(0x10) → tree(0x46)` to
  SHPI; all 87 `.qfs` entries decode with the committed ported codecs at the
  declared sizes (77 from the exact record slice, 10 with the container tail
  supplied — §3 caveat).

## 10. Open legs

1. **Second chunk** (`second_offset`, 24 B player / 784 B PAL): payload
   semantics; the 24-byte player record is bank-constant (walk/run
   `7C 00 00 00 02 00 00 00 7B 00 00 00 B4 FF FF FF 10 00 00 00 00 00 00
   00`, xstandd `...57...`). Candidate readers not found (the derived blit
   path ignores it).
2. **`word12`** (frame `+0x0C`): 0 for most banks, `53` for walk; no reader
   found in the derived path.
3. **`0x14720` producers beyond the palette path**: `FUN_00048DC0` installs
   `[slot*4+0x4BF60]`; the table's writers and the `[0x68E0]` mode branch stay
   FU-85 leg 9.
4. **`0x1584D8`/`0x1584F0` populator** (FU-85 leg 8): no static writer; the
   frame-second-chunk/overlay record link is unproven.
5. **`0x10F2E7` grouping** (FU-85 leg 7): indexed by frame index; the run
   pattern now matches 5-direction overlay banks, but the per-animation tables
   are not enumerated.
6. **`.qfs` inner format for non-sprite entries**: the other 87 `.qfs` names
   (actions/keeper/referee) decode to SHPI banks with the same chain, but only
   `jump` and a sample were fully decoded; their `word12`/second-chunk usage
   may differ.
7. **`0x9E3EC` name-skip condition**: `FUN_000AF37B(9,"Copyright")` decides
   whether the record starts with a NUL-terminated name; the helper's
   semantics are not derived (raw BIGF/SHPI unaffected).
8. **LF/LRU cache**: `0x58670`/`0x57D70` ring and `FUN_00078D5C` recycle order
   are cited but not derived beyond the reload path.
9. **Huff record over-read**: 10 `.qfs` entries need bytes past their declared
   BIGF size to finish the Huffman bitstream (the original is unbounded); the
   reason (trailing partial symbol vs. a missing flush) is not derived. The
   port never decodes, so only the doc/harness is affected.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x78F1C, 0x78DAC, 0x78C68, 0xA1918, 0xA1920, 0x9E890,
0x9E860, 0x9E718, 0x9E420, 0x9E3EC, 0xA2410, 0xA2718 (decompiled), 0xA275C
(decompiled), 0xA2C50, 0xA2E24, 0xB1730, 0xCEABC, 0xCE9D8, 0xCE980, 0xCE998,
0x57080, 0x9B0D0, 0x4A6BC, 0x4AC6C, 0x4A344;
`disassemble_bytes` 0x4AAE0 (ndisasm cross-check), 0x4A700 (256 B), 0x78C7F
(186 B), 0x4AB00, 0x4AC90;
`read_memory` 0x4AAD0 (64 B), 0x68B5C (268 B = code, hence the `+0x10000`
rule), 0x78B5C (268 B jump table), 0x102CA0, 0x1033C0, 0x101C50, 0x101D40;
`search_instructions` operands `4afb8`, `4c0dc`, `4bfc0`, `1d5c`, `1d4c`,
`1d68`, `1cb0`;
`get_xrefs_to` 0x14BFC0, 0x4AB17, 0x4A6BC, 0x1584D8, 0x1584F0;
`get_function_callers` 0x4AC6C;
`get_function_callees` 0x78F1C, 0x78DAC, 0x9E890, 0x9E860.
Asset probes (read-only ISO): `/ART/PLAYART.PVI` (807380 B) and
`/ART/GAMEART0.PVI` (154387 B) extracted with `tools/fifa96_bind.iso_files`
and decoded with the committed `fifa96_refpack_decode`, `fifa96_huff_decode`,
`fifa96_tree_decode` (throwaway harness in `/tmp/opencode/fu86`, not
committed); BIGF/SHPI structure checked across all 91 PLAYART entries and the
raw `.fsh` frames. Analysis-only outside the port: no tool, capture-rig, ISO or
Ghidra-project change. Port write set:
`include/fifa96_loader/fifa96_sprite.h`,
`src/fifa96_loader/fifa96_sprite.c`, `tests/test_sprite.c`, `CMakeLists.txt`
(one library/test block). `make test`: 74/74 before, **75/75 after**;
ASan+UBSan `test_sprite` clean. `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
