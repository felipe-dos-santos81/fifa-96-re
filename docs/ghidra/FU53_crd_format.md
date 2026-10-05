# FU-53 — the CRDF music container: file layout, producer path, loader copy and payload streams

Date: 2026-10-05. Program `/fifa96_le.bin` (flat LE link image, base 0).
Scope: the resource behind the FU-52 sequencer — the file `/SOUND/CRD_CRD0.CRD`
on the CD image, the loader `FUN_000a73b2` that copies its first `0x38C` bytes
into `0x5D808`, the producer path that opens it (`FUN_00065160` →
`FUN_00065118` → `FUN_00068FD0`), the `+0x10`/`+0x14` coefficient fields, and
the tail EACS sample blocks addressed by the track records. This closes FU-52
open leg 1 (the CRDF producer/file id) to the extent below.
Companions: FU-35/FU-39/FU-41/FU-43 (EACS), FU-51 (channel glue), FU-52 (the
sequencer itself). Every claim cites an instruction address or an image/file
read. Addresses in code operands are object-relative; the flat image resolves
object-4 data at `+0x100000` (e.g. object `0xA098` = image `0x10A098`,
object `0x5D808` = image `0x15D808`), as in FU-52 §0.

## 0. The blob and the loader copy

`FUN_000a73b2` (`0xA73B2..0xA7471`) is the only consumer of the resource:

```
0xA73BD  CMP [0x15FC8],1..5          ; sound-state gate (FU-51 §5.1)
0xA73D9  TEST EAX,EAX; JZ -1         ; null blob -> -1
0xA73E0  CMP dword [EAX],0x46445243  ; "CRDF" at blob+0
0xA73E8  MOV ECX,0xE3                ; 227 dwords = 0x38C bytes
0xA73ED  MOV EDI,0x5D808
0xA73F2  MOV ESI,EAX                 ; source = blob base (no leading skip)
0xA73F4  REP MOVSD ES:EDI,ESI
0xA73F6  MOV [0x148FA],0             ; intensity control = 0
0xA7400  CMP [EAX+0x24],4; JLE; MOV [EAX+0x24],4   ; clamp SOURCE track count
0xA740D  per track i < [source+0x24]:
0xA7419    ECX = [EAX + 0x5D8C4]     ; record +0x40 (0x5D884+0x40)
0xA7426    ADD ECX,EDX               ; relocate by blob base
0xA7429    [EAX + 0x5D8C4] = ECX
0xA741F    byte [EAX + 0x5D89D] = 0x7F   ; record +0x19
0xA7432    byte [EAX + 0x5D898] = 0x65   ; record +0x14
0xA743C    [EAX + 0x5D888] = ESI+0x28    ; record +0x04 = &record+0x28
0xA744A  CMP [0x5D830],0x10; JLE; MOV [0x5D830],0x10 ; clamp DEST event count
0xA745D  byte [0x148F4] = 1          ; music enabled
0xA7464  byte [0x148F5] = 0          ; not paused
0xA746B  XOR EAX,EAX                 ; success
```

Copy evidence: `ECX=0xE3` at `0xA73E8` (0xE3·4 = 0x38C), `EDI=0x5D808` at
`0xA73ED`, `ESI=EAX` at `0xA73F2` where `EAX` is the very pointer whose dword
`+0` was required to be `"CRDF"` at `0xA73E0`. There is **no skipped leading
header**: the source start equals the file start, and the copied prefix is the
file's first `0x38C` bytes. The record base is `0x5D884 = 0x5D808 + 0x7C`
(the `IMUL EAX,EBX,0x74` at `0xA7416` plus the `LEA ECX,[ESI+0x28]` EACS
pointer at `0xA742F`); slot `i`'s `+0x40` lives at `0x5D8C4 + i*0x74`.
The asymmetry noted in FU-52 stands: `+0x24` is clamped in the **source**
record (`0xA7400`, so `0x5D82C` keeps the raw value) while `+0x28` is clamped
in the **copy** (`0xA744A`, `0x5D830`).

`0xA6EA7` is **not** a caller of the loader. Immediately after the fade-restart
function's `RET` (`0xA6EA6`), the bytes at `0xA6EA7` are
`B8 08 D8 05 00 C3` = `MOV EAX,0x5D808; RET` — a 7-byte getter returning the
runtime CRDF struct address. It has no static xrefs in this frame. The loader's
only caller is `0x651A0` (see §1).

## 1. Producer path and how the file is opened

Entry `FUN_00065160` (no Ghidra function object; raw bytes `0x65160..0x651D7`):

```
0x65165  SUB ESP,0x100                    ; scratch path buffer
0x6516B  CMP [0x55CE0],0; JZ out          ; sound system up
0x65174  CMP [0xA0C8],0; JNZ out          ; already loaded
0x6517D  EDX = EAX                        ; caller's resource name
0x6517F  EAX = ESP                        ; dest buffer
0x65186  CALL 0x65118                     ; build path
0x6518B  EDX = EAX = path
0x6518D  EAX = 0x8A03C                    ; resource signature sentinel
0x65192  CALL 0x68FD0                     ; resolve/open/load -> blob
0x65197  [0xA0C8] = blob
0x651A0  CALL 0xA73B2                    ; CRDF validate + copy
0x651C2  [0x55D38] = 1                    ; enable channel 2
```

`FUN_00065118` (`0x65118..0x65152`) is a bounded `strncpy`/`strncat` chain:

```
0x6511B  ECX = dest (EAX), ESI = arg (EDX)
0x6511F  EBX = 0x100
0x65124  EDX = 0xA098; 0x65129 CALL 0xA1764   ; strncpy(dest,[0xA098],0x100)
0x65133  EDX = 0x2904;  0x6513A CALL 0xA1956   ; strncat(dest,[0x2904],0x100)
0x65144  EDX = ESI;     0x65148 CALL 0xA1956   ; strncat(dest,arg,0x100)
```

`FUN_000a1764` is the copy loop with the count in EBX and trailing NUL fill;
`FUN_000a1956` is its append twin (decompiled both). The buffers are
runtime-initialized image data: image `0x10A098` = `"SOUND"` (object `0xA098`)
and image `0x102904` = `"/"` (object `0x2904`). The path is therefore
`"SOUND" + "/" + caller_name`. The `EBX=0x20` set at `0x65181` is dead — the
callee overwrites EBX with `0x100` at `0x6511F` before its first use.

`FUN_00068FD0` (`0x68FD0..0x692D8`) loads the resource:

```
0x68FF3  CMP EAX,0x8A03C; SETNZ AL; [0x56625] = AL  ; mode flag
0x6900E  CALL 0x68E00 (EAX = name) -> EDI           ; 0x16-byte catalog record
0x69077  CALL 0x68B58 (EAX = name, EDX = buf) -> path ; canonicalize/recase
0x6907D  CALL 0x9A03C (path, 0) -> ESI              ; file bytes
0x6914B  C,R,C,'F' test on [ESI..]                  ; "CRCF" compressed variant
0x691F5  CALL 0x98BF8 (+0xCD390) when CRCF          ; decompress
0x69228  otherwise ESI is the payload
0x692C6  return ESI                                 ; blob in EAX
```

So the `0x8A03C` argument is a **signature sentinel**, not a name: the actual
lookup key is the name built by `FUN_00065118`. The catalog searched by
`FUN_00068E00` is the object-relative table at `0xA2E4` (image `0x10A2E4`),
`0x2A8` entries of `0x16` bytes: `{u32 hash, u16 kind, char name[16]}`. The
retail image contains the sole CRD asset's name in that table: record #196 at
image `0x10B3BC` is `87 63 F4 E8 | 00 00 | "CRD_CRD0.CRD\0\0\0\0"`
(hash `0xE8F46387`), so `("CRD_CRD0.CRD" + 6) = image 0x10B3C2`; the next
record (`81 F8 78 AB 06 00 "PLR_T086.VIV"`) confirms the stride. The name also
appears at image `0x10022C` inside a loose lowercase name pool
(`"CRD_CRD0.CRD\0at.TITLE.FSH..."`), which is not the catalog. FU-52's claim
that `0x8A03C` "is not a printable name" was correct about the argument but
missed that the name is found via EDX and does exist in the image (errata §8).

## 2. Container layout (file evidence: tests/golden/audio/crd-crd0.crd)

All values read from the committed fixture; runtime destinations are the
`0x5D808` copy. `off` is the file offset. The file has exactly the layout the
`0x38C` copy implies: `0x3C + 8*8 = 0x7C`, `0x7C + 4*0x74 = 0x24C`,
`0x24C + 16*0x14 = 0x38C`.

| off | size | fixture value | runtime | role | evidence |
| --- | --- | --- | --- | --- | --- |
| `+0x00` | 4 | `"CRDF"` | `0x5D808` | magic | `0xA73E0` |
| `+0x04` | 4 | `2` | `0x5D80C` | version (validated by the port; no reader found) | fixture |
| `+0x08` | 4 | `0` | `0x5D810` | unknown; no reader found | fixture |
| `+0x0C` | 4 | `0` | `0x5D814` | unknown; no reader found | fixture |
| `+0x10` | 4 | `0xB334` | `0x5D818` | position coefficient | `IMUL` `0xA6ED2` |
| `+0x14` | 4 | `0x4CCC` | `0x5D81C` | intensity coefficient | `IMUL` `0xA6EE3` |
| `+0x18` | 4 | `0` | `0x5D820` | position (16.16), file copy zero | `0xA7209`, `0xA733D` |
| `+0x1C` | 4 | `0` | `0x5D824` | intensity ramp, file copy zero | `0xA71B8..` |
| `+0x20` | 4 | `0` | `0x5D828` | limit / next waypoint, file copy zero | `0xA735A`, `0xA7241` |
| `+0x24` | 4 | `4` | `0x5D82C` | track count (loader clamps source to 4) | `0xA7520`, `0xA7400` |
| `+0x28` | 4 | `0x10` | `0x5D830` | event count (loader clamps copy to 16) | `0xA730A`, `0xA744A` |
| `+0x2C` | 4 | `0x96` | `0x5D834` | random-limit scale | `0xA70B1` |
| `+0x30` | 4 | `0x20000` | `0x5D838` | up-direction period scale | `0xA70FE` |
| `+0x34` | 4 | `0x40000` | `0x5D83C` | down-direction period scale | `0xA7107` |
| `+0x38` | 4 | `2` | `0x5D840` | initial event index | `0xA755B` |
| `+0x3C` | 0x40 | 8 tempo pairs | `0x5D844` | `{a,b}` stride 8 | `0xA7071`, `0xA70D1` |
| `+0x7C` | 0x1D0 | 4 track slots | `0x5D884` | stride 0x74 | `0xA74D5`, `0xA7412` |
| `+0x24C` | 0x140 | 16 events | `0x5DA54` | stride 0x14 | `0xA7316` |
| `+0x38C` | 0xE568 | 4 EACS sample streams | — | addressed by track `+0x40` | `0xA7419`, track record |

### 2.1 Tempo table (file `0x3C + i*8`)

| i | a | b | i | a | b |
| --- | --- | --- | --- | --- | --- |
| 0 | `0xB333` | `0x38000` | 4 | `0x10000` | `0x3840000` |
| 1 | `0x8000` | `0x40000` | 5 | `0x10000` | `0x10000` |
| 2 | `0xB333` | `0x190000` | 6 | `0x10000` | `0xA0000` |
| 3 | `0x20000` | `0xA0000` | 7 | `0x10000` | `0x50000` |

`a` feeds state 0, `b` feeds state 1 (FU-52 §4); states 2's direction scales are
`+0x30`/`+0x34`.

### 2.2 Event records (file `0x24C + i*0x14`)

| off | role | note |
| --- | --- | --- |
| `+0x00` | valid | all 16 records are `1` in the fixture |
| `+0x04` | rate numerator | `0x96..0x3FF` |
| `+0x08` | time / end waypoint | `0x258..0x3FF` |
| `+0x0C` | tempo index | `0..7` |
| `+0x10` | sound id | mostly `0`, some `5`/`0xF`/`0x15` |

First record: `{valid 1, rate 0x12C, time 0x28A, tempo 0, id 0}`; last:
`{1, 0x96, 0x258, 0, 0}`. The full 16-record table is pinned by
`tests/test_crd.c`; field semantics are FU-52 §3.

### 2.3 Track slots (file `0x7C + i*0x74`) and the payload

Each slot embeds a 32-byte EACS header at `slot+0x28` (file
`0xA4/0x118/0x18C/0x200`), followed at `slot+0x40` by the absolute file offset
of that track's sample stream (relocated to a pointer at `0xA7419`) and at
`slot+0x50` by the runtime voice handle (FU-52 §2).

| slot | EACS@ | rate | f8,f9,f10 | voice | declared +0x0C | data +0x18 | extent |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | `0xA4` | `0x5622` (22050) | 1,1,0 | -1 | `0x40C8` | `0x38C` | `0x40C8` |
| 1 | `0x118` | `0x3E80` (16000) | 1,1,0 | -1 | `0x272E` | `0x4454` | `0x2730` |
| 2 | `0x18C` | `0x5622` | 1,1,0 | -1 | `0x4B02` | `0x6B84` | `0x4B04` |
| 3 | `0x200` | `0x3E80` | 1,1,0 | -1 | `0x326A` | `0xB688` | `0x326C` |

The four streams are contiguous from `0x38C` to the file end `0xE8F4`;
`f8=f9=1, f10=0` is the raw 8-bit mono layout (FU-35 §2). The declared count
equals the extent for slot 0 and is exactly 2 lower for slots 1–3, where the
extent ends on a 4-byte boundary (`0x2730`, `0x4B04`, `0x326C`) — i.e. declared
bytes exclude 4-byte alignment padding (no reader consumes the pad). Loop
start/length at EACS `+0x10`/`+0x14` are `5/0x40BF`, `0x447/0x21AD`,
`0x3C/0x4A6F`, `0x10F/0x2FA2`; EACS `+0x1D` volume is 0 in all four; EACS
`+0x20` is `1` and `+0x27` is `2..5` (track index + 2) with no reader found
(open leg).

## 3. `+0x10`/`+0x14`: coefficients, not offsets

The two fields are read exactly once each in the whole image, both as
multipliers in the FU-52 track-volume updater `FUN_000a6ead`:

```
0xA6ECD  EAX = [0x5D800]          ; position >> 16 (integer position)
0xA6ED2  IMUL EAX,[0x5D818]       ; * (+0x10)
0xA6ED9  [0x5D7FC] = product
0xA6EDE  EAX = [0x5D824]          ; intensity ramp
0xA6EE3  IMUL EAX,[0x5D81C]       ; * (+0x14)
0xA6EEA  [0x5D7F8] = product
0xA6EEF  EAX = [0x5D7FC]
0xA6EF4  ADD EAX,[0x5D7F8]
0xA6EFF  SAR [0x5D804],0x13       ; () >> 19
0xA6F06  clamp to 0x7F
```

The xref lists are exactly `0xA6ED2` (READ) for `0x5D818` and `0xA6EE3`
(READ) for `0x5D81C`. The values are complementary 16.16 weights:
`0xB334 + 0x4CCC = 0x10000` (≈0.70/0.30). Following them as file offsets lands
strictly **inside** the sample streams — `0x4CCC` is 0x878 bytes into slot 1's
block (`0x4454..0x6B84`), `0xB334` is 0x47B0 bytes into slot 2's block
(`0x6B84..0xB688`) — and the bytes there are ordinary signed 8-bit samples
(`0x4CCC`: `ee 04 16 23 18 f4 e4 f1`; `0xB334`: `c0 b3 ae bb d0 e8 fe 15`);
no magic, header, extent or pattern block begins at either address. They are
not offsets/sizes: the brief's "offset fields" reading is an errata (§8).

## 4. Port mapping

New API in `fifa96_crd.h/.c`, TDD tests in `tests/test_crd.c`, fixture
`tests/golden/audio/crd-crd0.crd` (59636 bytes; ISO byte `0xEFD000`, LBA 7674).

| port | original |
| --- | --- |
| `FIFA96_CRD_MAGIC`/`_VERSION`/`_HEADER_SIZE` (`0x38C`) | `0xA73E0` magic, header geometry, `0xA73E8` copy size |
| `FIFA96_CRD_TRACK_MAX` 4 / `_EVENT_MAX` 16 / `_TEMPO_MAX` 8 | fixed geometry `0x38C`; clamps `0xA7400`/`0xA744A` |
| `struct fifa96_crd_header` | file `+0x04..+0x38` fields, per §2 |
| `struct fifa96_crd_tempo` (`a`,`b`) | tempo pairs `+0x3C`, readers `0xA7071`/`0xA70D1` |
| `struct fifa96_crd_event` | event records `+0x24C`, FU-52 §3 |
| `struct fifa96_crd_track` | slot `+0x28` EACS header fields and `+0x40` data offset; extent derived from the next `data_off`/file end |
| `fifa96_crd_parse` | `FUN_000a73b2`: `"CRDF"` check, fixed `0x38C` region, counts, contiguous stream bounds; no copy, no relocation (offsets exposed as stored) |
| `fifa96_crd_header` accessors | bounds-checked slot/tempo/event getters over the caller-owned struct |

Only the container is ported: the EACS streams are exposed as offsets/extents
(the existing `fifa96_eacs_*` and `fifa96_music_*` layers are unchanged and not
wired in). Faithfulness notes: the parser accepts only the fixed 4/16/8
geometry (track `+0x24` ≤ 4, event `+0x28` ≤ 0x10); `+0x18`/`+0x1C`/`+0x20`
file zeros are exposed as stored; `+0x40` stays an absolute file offset exactly
as on disk, since the relocation at `0xA7419` is runtime state. Not ported
(documented): the producer path itself (§1), the EACS decoding of the tail
streams, and the `FUN_00068FD0` CRCF decompression variant.

## 5. Errata (quoted brief/controller facts vs the evidence)

1. Controller: "caller `0xa6ea7`" of the CRDF copy. Actual: `0xA6EA7` is
   `MOV EAX,0x5D808; RET` — a getter, not a caller. `FUN_000a73b2` is called
   once, from `0x651A0` inside the music load entry `FUN_00065160`.
2. Controller: "the string `CRD_CRD0.CRD` is not in the flat image". Actual:
   image `0x10B3BC` is resource-catalog record #196
   `{hash 0xE8F46387, kind 0, "CRD_CRD0.CRD"}` (name at `0x10B3C2`), and
   image `0x10022C` holds a second, loose copy. The string is in the image
   twice; the `0x8A03C` argument to `FUN_00068FD0` is a signature sentinel and
   the name arrives in EDX.
3. Controller: "the `+0x10`/`+0x14` offset fields (0xB334/0x4CCC) ...
   determine what they point to". Actual: they are coefficients read only by
   `IMUL` at `0xA6ED2`/`0xA6EE3`; they sum to `0x10000`; followed as file
   offsets they land mid-stream in slots 1/2 on sample bytes with no
   structure. Not offsets.
4. FU-52 §1 "resource lookup `FUN_00068FD0(0x8A03C)`": `EAX` is a mode/signature
   (stored as `[0x56625] = (EAX != 0x8A03C)` at `0x68FF3`) while the lookup
   key is `EDX` (the built `"SOUND/"+name` path). Refined, not contradicted.
5. FU-52 open leg 1 ("CRDF producer ... no file mapping is derived") is closed
   for the load path: name → catalog record #196 → `"SOUND/CRD_CRD0.CRD"` →
   `FUN_00068FD0` open → `FUN_000a73b2`. The indirect caller of `FUN_00065160`
   (which UI state passes the name) remains open (§6).

## 6. Open legs

1. **Indirect caller of `FUN_00065160`.** The music load entry has no static
   xref (the public entries `0x65160..0x652B8` are reached indirectly; only
   `0x652B8` has one direct caller, `0x91DE1`). Which state passes
   `"CRD_CRD0.CRD"` is not resolved; the name itself is in the catalog (§1).
2. **Loose name pool at image `0x10022C`** (lowercase fragments
   `at.TITLE.FSH`, `me.slick25i.ffn`, ...) — a second CRD name copy with no
   xref.
3. **Embedded EACS `+0x20`..`+0x27`**: `+0x20 = 1`, `+0x27 = track index + 2`,
   no reader found in this frame.
4. **Declared vs padded extent**: slots 1–3 declare 2 fewer bytes than their
   4-byte-aligned extents; no reader consumes the trailing pad.
5. **`CRCF` variant**: `FUN_00068FD0` checks `"CRCF"` and decompresses via
   `0x98BF8`/`0xCD390`; no relationship to the CRD asset is established here.
6. **Header `+0x08`/`+0x0C`** and the version field `+0x04` have no reader in
   this frame (the version is validated by the port from the file evidence).

## 7. Provenance (Ghidra, 2026-10-05, `/fifa96_le.bin`)

* Disassembled: `FUN_000a73b2`, raw bytes `0x65160..0x651DF`,
  `FUN_00065118`, `FUN_00068fd0`, `FUN_00068e00`, `FUN_000670f8`,
  `FUN_000a6ead` (all 97 instructions), raw bytes `0xA6E5E..0xA6EBD`
  (`FUN_000a6e5e` pause, `FUN_000a6e6b` fade restart, `0xA6EA7` getter),
  `FUN_00091dd8`.
* Decompiled: `FUN_000a1764` (strncpy-like), `FUN_000a1956` (strncat-like),
  `FUN_00068c6c`, `FUN_00068e00`, `FUN_000670f8`.
* Xrefs: `0x5D808` (WRITE `0xA73F4`, DATA `0xA73ED`, DATA `0xA6EA7`),
  `0xA73B2` (call `0x651A0`), `0x5D818`/`0x5D81C` (READ
  `0xA6ED2`/`0xA6EE3`), `0xa6ea7` (none), `0x65160` (none), `0xa2e4`
  (`0x68EA4`, `0x68E77`, `0x68E84`, `0x694E7`), `0x651d8`/`0x65218`/`0x65240`/
  `0x65264` (none), `0x652b8` (call `0x91DE1`), `0x651f0` (44 calls).
* Image reads: `0x10A098` = `"SOUND"`, `0x102904` = `"/"`, `0x10A2E4..`
  catalog (record #196 at `0x10B3BC`), `0x10022C` name pool, `0xA6EA7` bytes
  `B8 08 D8 05 00 C3`.
* File/ISO reads: ISO9660 PVD LBA 16, root LBA 20, `SOUND` dir LBA 1478;
  `/SOUND/CRD_CRD0.CRD` at LBA 7674 length 59636 (ISO byte `0xEFD000`),
  extents from its own header; the only other `CRDF` byte hits in the ISO are
  at `0x271636` (/FIFA96.EXE), `0xC772A5` (/SOCCER/INST.EXE) and `0xD39DD0`
  (/SOCCER/INSTALL.EXE) — all inside executables (copies of the loader code),
  not assets. Fixture sha256
  `2bb529abbec31ee39232511f9c4e00ab61ff12311ce6233decfff91081377648`.
* Baseline: `ctest -N` 43 tests before; 44 after (`tests/test_crd.c`);
  `make test` green; ASan/UBSan clean. This doc is the first tracked change of
  the slice.
