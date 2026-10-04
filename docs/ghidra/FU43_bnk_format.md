# FU-43 — the `SOUND/*.BNK` sound-effect bank format

Date: 2026-10-04. Program `/fifa96_le.bin` (flat LE link image, base 0).
Scope: the `.BNK` sibling of FU-41's BIGF `.VIV` banks — the file-start
offset table, the entry descriptor, the embedded EACS payload, the loader /
registrar path, and the play/arm path. Closes FU-41 §5 leg 6 (".BNK
container"). Companion slices: FU-4 (LE image), FU-35 (EACS header), FU-37
(mixer/voices), FU-39 (f10==2 decode), FU-41 (`.VIV` bank loader and the
shared arm tail). Every claim cites an instruction address or a probe output.

Address frame (FU-41 §0, FU-4): the flat image applies no LE fixups. Operands
to object-4 data are stored object-relative and land at image+0x100000
(e.g. `MOV EAX,0x20b4` at `0x639E4` reads image `0x1020b4` =
`"SFX_FRWK.BNK"`). Callback constants inside object 1/2/3 are stored
segment-relative and need their object base at run time (the error
dispatcher's `JMP CS:[EAX*4+0x969b7]` at `0xA6A15` addresses image
`0xA69B7`). Listing addresses below are image addresses.

## 1. Container layout

The file is `[128 x u32 LE offset table][descriptor array][payload pool]`:

| off | width | meaning | citation |
|---|---|---|---|
| 0x000 | u32[128] | global-SFX-id -> descriptor file offset (0 = id absent) | registrar loop bound `CMP [EBP-4],0x80; JC` `0xA7646..0xA764D`; slot read `[EDX]` `0xA760C..0xA760F` |
| 0x200 | entry[] | 0x48-byte descriptors, packed | corpus: first descriptor at 0x200 in 25/25 banks; stride `0x48` |
| 0x200+0x48n | bytes | payloads, 4-byte aligned, gaps 0..3 | EACS `+0x18` per entry; corpus gaps `{0,1,2,3}` |

`FUN_000a75aa` is the registrar (decompiled and disassembled). It walks
**exactly 128 slots** from the buffer base (`0xA75EF..0xA7651`):

```
base = FUN_00099e6c(bank_object)              ; 0xA75F7 (buffer data pointer)
for (i = 0; i < 0x80; i++, table++) {         ; 0xA7646  bound 0x80
  if (*table == 0) continue;                  ; 0xA760C  zero slot = absent id
  if (DAT_00061c14[i] != 0) return -0x11;     ; 0xA7611/0xA7618  duplicate id
  entry = base + *table;                      ; 0xA761A/0xA761D
  DAT_00061c14[i] = entry;                    ; 0xA7622
  entry[4] = base + entry[4];                 ; 0xA7628..0xA762B
  eacs = entry[4];                            ; 0xA762E..0xA7634
  eacs[0x18] = base + eacs[0x18];             ; 0xA7637..0xA763D
}
DAT_000149b8[slot] = bank_object;             ; 0xA7651 (slot 0..4, found at 0xA75CE..0xA75E2)
```

So the **table is indexed by global SFX id**, not by local entry number, and
the id space is shared across every registered bank: the registrar rejects a
duplicate id across banks (`-0x11`; the library's error-string dispatcher,
jump table `CS:[EAX*4+0x969b7]` `0xA6A15` over the table at image `0xA69B7`,
maps index 3 to `MOV EAX,0x3992` `0xA6A86` = "Error: could not resolve sfx
bank"). `FUN_000a765f`
unregisters one bank (clears every `DAT_00061c14[i]` whose table slot was
nonzero, `0xA76AF..0xA76C8`; bank slot bounds 0..4 -> `-0x12` at
`0xA767B..0xA7684`; `-1` = all five).

Slot 0 is a **normal id slot**, not a sentinel: the registrar starts at
`i=0`, and the play API accepts id 0 (`0xA7738..0xA7742` rejects only
`<0` or `>=0x80`). All 25 retail banks leave slot 0 zero, so it is unused in
practice; the `0` at file offset 0 is simply an empty id-0 slot (this
corrects FU-41 §1's "sentinel" reading of `SFX_GAME.BNK`).

### 1.1 Descriptor (0x48 bytes) = 0x28-byte instrument header + EACS header

On disk `entry+0x04 == entry+0x28` in 59/59 entries (corpus check), i.e. the
instrument header points at the embedded EACS immediately behind it; the
registrar relocates both that pointer and the EACS data pointer.

| off | width | field (bank arm reads) | citation |
|---|---|---|---|
| 0x00 | u32 | mixer-voice bitmask, passed to `FUN_000a62fa` | `MOV EAX,[EAX]` `0xA7823` |
| 0x04 | u32 | offset to embedded EACS (relocated +base) | `MOV EDX,[ESI+4]` `0xA783A`; `CMP [EDX],'EACS'` `0xA7840` |
| 0x08 | u32 | pitch randomization range (0 retail) | `MOV EDX,[ESI+8]` `0xA7874` |
| 0x0C | u32 | pitch randomization numerator (0 retail) | `[ESI+0xC]` `0xA789D..0xA78AC` |
| 0x10 | u32 | pitch base (0 retail) | `[ESI+0x10]` `0xA78C0..0xA78C6` |
| 0x14 | u8 | voice priority (0x64 = 100 retail) | `MOVZX EDX,byte [EAX+0x14]` `0xA781F`; `MOV DL,[ESI+0x14]; MOV [EAX+0x13],DL` `0xA7887..0xA788A` |
| 0x15 | u8 | 0x80 retail, unread by the traced arm | corpus histogram §4 |
| 0x16 | u8 | 0 retail, unread | corpus |
| 0x17 | u8 | 0 retail -> EACSNDF record +0x11 | `MOV DL,[ESI+0x17]` `0xA7881..0xA7884` |
| 0x18 | u8 | pan fallback (0x40 centre) | `MOV AL,[ESI+0x18]` `0xA7967` |
| 0x19 | u8 | volume base (0x7f, 0x3c, 0x32 retail) | `MOV AL,[ESI+0x19]` `0xA791F` |
| 0x1A | s8 | volume randomization span (0 retail) | `CMP byte [ESI+0x1A],0` `0xA78D4..0xA78DA` |
| 0x1B | u8 | 0x0a or 0 retail, unread | corpus |
| 0x1C | u8 | flags; bit0 selects the two-voice path | `MOV DL,[EDI+0x1C]; AND DL,1` `0xA775F..0xA7765`; copied to record+0x0A `0xA7893..0xA789A` |
| 0x1D | u8 | pan override when the caller passes pan = -1 | `CMP byte [ESI+0x1D],0` `0xA792B` |
| 0x1E,0x1F | u8 | unread by the traced arm | corpus |
| 0x20..0x27 | u32[2] | unread by the traced arm; `SFX_GAME` constant `d5 4c d4 ca` at +0x24 | corpus histogram §4 |

The descriptor carries **no rate/format**; all sample metadata lives in the
embedded EACS. The only descriptor fields the arm consumes are the voice
mask (+0x00), priority (+0x14), pitch range/base (+0x08/+0x0C/+0x10),
volume/pan/randomization (+0x17..+0x1A, +0x18, +0x1D) and flags (+0x1C).

### 1.2 Embedded EACS (entry+0x28, 32 bytes)

Same 32-byte header as FU-35/FU-41 §2, with one bank-container difference:
`+0x18` holds the **absolute file offset of the payload** (not the literal
`0x20` of a `.VIV` `.spc` entry). Corpus (59/59):

| off | field | retail values |
|---|---|---|
| 0x00 | `"EACS"` | 59/59 |
| 0x04 | rate | 16000 x33, 16384 x20, 11025 x6 |
| 0x08/0x09/0x0A | f8/f9/f10 | (2,1,2) x58, (2,2,2) x1 |
| 0x0B | voice | -1 (0xff) x59 (bank form, FU-41 §2) |
| 0x0C | declared units | see §2 |
| 0x10/0x14 | loop start / loop length | 0 for all SFX; nonzero for all 23 CHN entries |
| 0x18 | payload file offset | relocated to a pointer by the registrar `0xA7637..0xA763D` |
| 0x1C..0x1F | unread by the bank arm; +0x1C is overwritten with the runtime voice | +0x1F in {0,0x7c,0x9c,0xdc,0xcc} |

## 2. Entry payload forms

All 59 entries of the 25 retail `.BNK` files embed EACS — there is no
raw-PCM entry form in the retail corpus. The payload form is selected by the
EACS format bytes, and the **voice byte -1 is what removes the 20-byte block
header** from both forms:

`FUN_000a6579` builds the mixer flags (`0xA6592..0xA65D0`):

```
flags = 0
if (f8  == 2) flags |= 8;      ; 0xA659D..0xA65A2
if (f9  == 2) flags |= 4;      ; 0xA65A9..0xA65B2
if (f10 == 2) flags |= 2;      ; 0xA65B6..0xA65BF
if ((s8)voice >= 0) {          ; 0xA65C3/0xA65C7
    flags |= 0x10;             ; 0xA65CC
    header[0x1C] = voice;      ; 0xA65D0  (video form)
}                              ; voice == -1 -> no 0x10 (bank form)
```

and `FUN_000b7fe8` reads the 20-byte block header only on the signed path
(`ch+3 = flags & 0x10` at `0xB801B..0xB8024`; the `TEST [ESI+3]` guard at
`0xB80AD` skips `0xB80B4..0xB80C5`, which is where `ch+0x4C` would be
overwritten from the block header). Therefore:

* **f9 = 1 (58/59 retail entries) — mono delta.** Flags `0x0A`. Payload =
  `ceil(declared/2)` bytes; `declared` is the sample count in nibbles
  (MSB-first). No per-chunk header (FU-39's mono bank producer `0xB8610`,
  FU-41 §3.3). Corpus: `2*payload - declared == 0` for all 58.
* **f9 = 2 (1/59: `SFX_GAME.BNK` id 29) — stereo delta.** Flags `0x0E`
  (8|4|2, no 0x10). Payload = `declared` bytes; one byte is one (L,R) frame
  (high nibble L, low R, FU-39 §2.3). **No 20-byte block header**: id 29
  declares 16929 frames at file offset 0x25234 and its payload runs exactly
  16929 bytes to 0x29455, with the next payload at 0x29458; the bytes at
  0x25234 (`dd 99 55 22 ...`) are not a count. The producer `0xB8610`
  branches on `ch+4` bit 2 (`TEST [ESI+4],4` `0xB8696`): mono shifts the
  cursor `>>1` and passes a nibble parity, stereo leaves the byte cursor
  unshifted (`0xB8699..0xB86A6`).

Decoder/producer selection (`FUN_000b81f0`, `0xB81F0..0xB82E1`):

| flags | reader | decoder | producer (loop length 0) | producer (loop) |
|---|---|---|---|---|
| `0x0A` mono | `0xA8AE1` (`flags&0xC==8`, `0xB8242..0xB8257`) | `0xC4CC4` (`flags&4==0`, `0xB8273..0xB8278`) | `0xA8610` (`flags&0x12==2`, `0xB82CB`) | `0xA83F1` (`0xB82B3`) |
| `0x0E` stereo | `0xA929D`/`0xA9A46` (`flags&0xC==0xC`, `0xB821E..0xB8233`) | `0xC4D6C` (`0xB8269..0xB826E`) | `0xA8610` | (not observed) |

**Looping.** FU-41 §3.3 concluded "all bank playback is one-shot" from the
`.VIV` corpus, where every `.spc` stores loop length 0. That is not true of
`.BNK`: all 23 `CHN_*.BNK` entries store a nonzero `+0x14` and a nonzero
`+0x10`, and arm the looping producer `0xA83F1` (`ch+1 = looplen != 0`,
`0xB800D..0xB8013`; producer selection `0xB8281..0xB82B3`). Corpus:
`loopstart + looplen <= declared` in 23/23 loops (e.g. `CHN_ARG1.BNK` id 40:
start 102, length 64011, declared 64144). `SFX_GAME`/`SFX_FRWK` store 0 and
play one-shot.

## 3. Load, register, select, arm

### 3.1 Load

Two static load paths feed `FUN_000a75aa`:

* `FUN_00063974` (sound-module setup) calls `MOV EAX,0x20b4` (`0x639E4`),
  i.e. image `0x1020b4` = `"SFX_FRWK.BNK"`, into `FUN_00065d90`.
  `FUN_00065d90` builds the `SOUND/` path with `FUN_00065118`, loads the
  whole file with `FUN_00068f74` and registers it (`0x65D90..0x65DCF`;
  decompiled), storing the buffer at `DAT_0000a25c` and the bank slot at
  `DAT_0000a260`.
* `FUN_000652f0(index, name_idx)` is the per-team chant loader: the name is
  `*(char**)(&DAT_00009f78 + name_idx*4)` — an object-4 offset array (at
  image `0x109f78`; e.g. entry 0 = `0x2524` -> image `0x102524` =
  `"CHN_ARG1.BNK"`). It builds `SOUND/` + name (`FUN_00065118`), loads with
  `FUN_00068f74(...,0x220)`, stores the buffer at `DAT_00055dbc[index]` and
  registers it, storing the slot at `DAT_00055e18[index]` (decompiled
  `0x652F0..0x6536B`). It has no static caller (reached through the game's
  sound pointer tables) — open leg §6.

`FUN_00068f74` is the shared open path (`FUN_00068e00` filename,
`FUN_00068b58` buffer, `FUN_0009a090` whole-file read). `SFX_GAME.BNK` is
the runtime binding `/SOUND/SFX_GAME.BNK` from FU-2 (trace name
`D:\SOUND\SFX_`, 11 reads) and is the committed golden fixture; its static
call site was not located (no immediate operand points at the lowercase
`"SFX_game.bnk"` string at image `0x10021c`, and a dword scan finds no
non-code pointer to object-4 offset `0x21c` or to the manifest's
`0xb07e`) — open leg §6.

### 3.2 Select and arm

The game plays by **global SFX id** through the EACSNDF API:

```
FUN_000a76ea:  EAX=id, defaults (EDX=-1, EBX=0x7f, ECX=0x40)  ; 0xA76EA..0xA76FC
FUN_000a7705:  EAX=id, EDX=caller volume             ; 0xA7705..0xA7713
FUN_000a771b:  EAX=id, EDX=caller pan, EBX=caller volume  ; 0xA771B..0xA7721
        -> FUN_000a7728(EAX=id, EDX, EBX, ECX)
```

`FUN_000a7728` (`0xA7728..0xA780D`):

```
if (id < 0 || id >= 0x80) return -0x13;              ; 0xA7738..0xA7742 ("invalid sfx number")
entry = DAT_00061c14[id];                            ; 0xA774D..0xA7752
if (entry == 0) return -0x13;                        ; 0xA775B/0xA775D
if (entry[0x1C] & 1) { ... two-voice split via FUN_000a6717, ids id and id+1 ... }  ; 0xA775F..0xA77FA
else FUN_000a780e(entry, id, pan, volume);           ; 0xA77FB..0xA7805
```

`FUN_000a780e` (`0xA780E..0xA79A1`) is the BNK arm and is where the
descriptor header is consumed:

```
record = FUN_000a62fa(mask=[entry], priority=byte[entry+0x14]);  ; 0xA781F..0xA7825
if (!record) return -0x14;                                       ; 0xA782A..0xA7830
eacs = entry[4]; if (eacs[0] != 'EACS') return -10;              ; 0xA783A..0xA784D
if (sound_state[0x15fc8] not in 1..5) return -4;                 ; 0xA7852..0xA7869
fill EACSNDF record from entry bytes (+0x17,+0x14,+0x1C,vol/pan);; 0xA786E..0xA7922
voice_index = (s8)record[0x12];                                  ; 0xA797B
FUN_000b9fdd(...);                                               ; 0xA7986
eacs[0x1C] = voice_index;                                        ; 0xA798B..0xA798E
FUN_000a6579(eacs);                                              ; 0xA7993 (flags + FUN_000b7fe8/FUN_000b81f0, §2)
return voice_index;
```

`FUN_000a62fa` (`0xA62FA..0xA6383`) is the voice allocator: it scans the 16
EACSNDF records (`0x61994 + voice*0x28`) starting at `DAT_000148ac`, accepts a
voice whose bit is set in the descriptor mask (`1 << voice & mask`,
`0xA630E..0xA6314`), prefers a free record (`record[0x16] == 0`, `0xA631F`)
and otherwise the first whose stored priority byte (`record[0x13]`) is
<= the descriptor priority (`0xA635E..0xA636D`); it stores the chosen voice
in `record[0x12]` (`0xA6325`) and advances `DAT_000148ac` (`0xA6328..0xA6330`),
returning the record pointer.

**Contrast with the `.VIV` path (FU-41 §3).** `.VIV` bank entries are
registered into the 20-track ring by `FUN_000a8084` (which relocates
`+0x18 == 0x20` to `header+0x20`), then armed per mixer tick by
`FUN_000a7edf -> FUN_000ba41d`. `.BNK` entries are relocated **once at
registration** (`entry+4`, `eacs+0x18`, §1), selected by a **global id table**
(`DAT_00061c14`), and armed directly by `FUN_000a7728 -> FUN_000a780e ->
FUN_000a6579`, bypassing the ring and `FUN_000ba41d`. Both paths converge on
`FUN_000a6579`/`FUN_000b7fe8`/`FUN_000b81f0`. The stop path is unchanged:
producer zero frames -> `FUN_000b80fa` -> `FUN_000ba00e` reset.

## 4. Structural validation (throwaway, not committed)

Assets: all 25 `.BNK` extracted from the read-only `game/FIFAPCCD96.iso`
with `tools/fifa96_bind.iso_files` to `/tmp/opencode/fu43/banks/`; the
golden `tests/golden/sfx_game.bnk` is byte-identical to
`/SOUND/SFX_GAME.BNK`:

```
$ python3 /tmp/opencode/fu43/fu43_extract.py
extracted 25
golden==iso: True fdbdf0972e595f0a 173016
```

Probe: `/tmp/opencode/fu43/fu43_census.py` (parse the table/descriptors/
payloads, check monotonicity/bounds/contiguity, histogram fields).

```
file            size    ids                 n  loops stereo tail
CHN_ARG0.BNK      21576 41                    1     1      0    0
CHN_ARG1.BNK      32656 40                    1     1      0    0
CHN_BRA0.BNK      25896 33                    1     1      0    2
CHN_BRA1.BNK      27720 42                    1     1      0    0
CHN_BRA2.BNK      54344 39                    1     1      0    0
CHN_CHA0.BNK      33352 43                    1     1      0    0
CHN_CHA1.BNK      27720 45                    1     1      0    0
CHN_DEM0.BNK      14920 46                    1     1      0    0
CHN_DEM1.BNK      28744 47                    1     1      0    0
CHN_ENGA.BNK      24992 34                    1     1      0    2
CHN_ENGB.BNK      48880 35                    1     1      0    0
CHN_ENGC.BNK      16104 36                    1     1      0    0
CHN_ENGD.BNK      33584 37                    1     1      0    0
CHN_ENGE.BNK      29352 38                    1     1      0    0
CHN_ENGF.BNK      26184 48                    1     1      0    0
CHN_ENGG.BNK      26696 49                    1     1      0    0
CHN_FRAN.BNK      22600 50                    1     1      0    0
CHN_GEN1.BNK      58952 55                    1     1      0    0
CHN_GEN2.BNK      27720 56                    1     1      0    0
CHN_GEN3.BNK      61192 57                    1     1      0    0
CHN_GEN4.BNK      54760 58                    1     1      0    0
CHN_ITA0.BNK      53320 51                    1     1      0    0
CHN_ITA1.BNK      29256 52                    1     1      0    0
SFX_FRWK.BNK      44392 85..89                5     0      0    0
SFX_GAME.BNK     173016 1..31                31     0      1    0

entries: 59 ids unique: 59
rates: Counter({16000: 33, 16384: 20, 11025: 6})
(f8,f9,f10): Counter({(2, 1, 2): 58, (2, 2, 2): 1})
voice: Counter({255: 59})
desc+0 voice-mask: Counter({'0x8080': 16, '0x2000': 9, '0x800': 8, '0x40': 8,
                            '0x8000': 7, '0x4000': 6, '0x1000': 4, '0x400': 1})
desc+4 == off+0x28: True
desc+8/+0xC/+0x10 all zero: True
desc+0x14..0x1F: Counter({'64800000407f000a000008ff': 28,
                          '64800000407f000000000000': 18,
                          '64800000407f000a000000ff': 5,
                          '64800000407f000a000001ff': 5,
                          '648000004032000a000008ff': 2,
                          '64800000403c000a000008ff': 1})
desc+0x20..0x27: Counter({'00000a10d54cd4ca': 31, '0000000000000000': 18, ...})
eacs+0x1C..0x1F: Counter({'0000007c': 32, '00000000': 18, '0000009c': 5,
                          '000000dc': 3, '000000cc': 1})
loopstart+looplen>declared: 0 []   ; 23 loops, all within declared
2*payload-declared (f9=1): Counter({0: 58})
stereo payload==declared (f9=2): True
```

Bounds/contiguity checks all pass: descriptor stride 0x48 and first
descriptor 0x200 in 25/25; `entry+4 == entry+0x28` in 59/59; payloads
4-byte aligned, gaps `{0,1,2,3}`, tail `{0,2}`; no payload overlap; global
ids unique across all 25 banks. Global id map: `1..31` `SFX_GAME`, `33..58`
`CHN_*`, `85..89` `SFX_FRWK` (32, 44, 53, 54 and 59..84 unused).

### 4.1 The committed EACS parser over real entries

`/tmp/opencode/fu43/fu43_validate.c` (throwaway) drives the committed
`fifa96_eacs_parse` + `fifa96_eacs_delta_unit` over every entry: it copies
the 32-byte EACS header, normalizes the container-specific `+0x18`
file-offset back to the parser's implicit `0x20` (the same normalization the
game performs at registration, §1), appends the payload, parses, and decodes
all declared units:

```
$ ./fu43_validate banks/*.BNK
...
banks/CHN_ARG1.BNK size=  32656 entries= 1 parsed= 1 rejected=0 decoded_units=64144 payload_bytes=32072 range=[-19239,19136]
...
banks/SFX_FRWK.BNK size=  44392 entries= 5 parsed= 5 rejected=0 decoded_units=87040 payload_bytes=43520 range=[-31953,32599]
  id29 parse rc=-4 f9=2 decl=16929 pay=16929
banks/SFX_GAME.BNK size= 173016 entries=31 parsed=30 rejected=1 decoded_units=306620 payload_bytes=153310 range=[-32768,32767]
```

58/59 entries parse and decode fully as `DELTA_MONO`; all decoded samples
stay in int16 with plausible ranges. The single rejection is the stereo bank
form: the committed parser classifies `f8=2,f9=2,f10=2` as
`FIFA96_EACS_FMT_DELTA_STEREO` and requires the signed/video 20-byte block
header (fifa96_eacs.c:82-89), but the bank form has none (voice -1 removes
flag 0x10, §2), so id 29 fails `count <= data_len-0x14` with rc `-4`
(`FIFA96_ERR_TRUNCATED`). This is a port gap, not a bank defect — open leg
§6.

## 5. What the `.BNK` format is, in one paragraph

`SOUND/*.BNK` is a global-id-indexed sound-effect bank: a 128-slot u32
offset table at file start (slot = global SFX id), then packed 0x48-byte
descriptors (a 0x28-byte EACSNDF instrument header pointing at a 0x20-byte
EACS header), then 4-byte-aligned payloads. The registrar relocates the two
stored offsets (`entry+4`, `eacs+0x18`) to pointers and installs each
descriptor into the 128-entry `DAT_00061c14` id map (max 5 banks). Playback
is by id: the game's wrappers call `FUN_000a7728`, which picks the voice
from the descriptor's bitmask/priority via `FUN_000a62fa`, writes the voice
into the embedded EACS `+0x1C`, and hands the EACS to the shared format arm
`FUN_000a6579`. Because the bank voice byte is -1, the arm uses flags `0x0A`
(mono delta, one nibble per sample, `ceil(declared/2)` bytes) or `0x0E`
(stereo delta, one byte per L/R frame, `declared` bytes) and never reads a
per-chunk 20-byte block header; loop length != 0 (all `CHN_*` chants) arms
the looping producer `0xA83F1`.

## 6. Open legs

1. **`SFX_GAME.BNK` static load site.** `SFX_FRWK.BNK` loads at `0x639E4`
   and the CHN banks through `FUN_000652f0`'s object-4 name array, but no
   static reference to `"SFX_game.bnk"` (image `0x10021c`) or
   `"SFX_GAME.BNK"` (manifest `0x10b07e`) exists as an immediate or data
   pointer; FU-2's runtime binding is the only evidence for its open.
   Capture target: break on `FUN_00068f74`/`FUN_00065d90` at boot and dump
   the filename argument.
2. **Bank stereo parser gap.** `fifa96_eacs_parse` should branch on
   `voice == -1` for `f10==2 && f9==2` (bank stereo: no block header,
   `delta_units = declared`), as it already does for `f9==1` (FU-41 §4.3).
   Evidence: `SFX_GAME` id 29 (decl 16929, payload 16929 B, no header; flags
   `0x0E` skips `0xB80AD`).
3. **Unread descriptor bytes.** `+0x15` (0x80), `+0x1B` (0x0a/0),
   `+0x1E/+0x1F` (0x08 0xff etc.) and `+0x20..+0x27` (constant only in
   `SFX_GAME`) are not read by the traced bank arm. They may belong to a
   different EACSNDF API or to the bank builder; meaning unknown.
4. **Two-voice path unexercised.** `entry[0x1C] & 1` selects a stereo split
   that arms id and id+1 through `FUN_000a6717` (`0xA776A..0xA77FA`); no
   retail entry sets bit 0 (histogram §4), so it is static-only.
5. **`SFX_LOAD.BNK`.** Named in the object-4 manifest (`0x10dc68`) but not
   present on the ISO (25 `.BNK` shipped); role unknown.
6. **Game event -> id mapping.** `FUN_00065544` indexes `DAT_00055dbc` and
   `FUN_000a72e7` walks 0x14-stride records at `0x5da54` before calling the
   id wrappers; the full event table is not mapped here. Capture target:
   dump `DAT_00055dbc` / `0x5da54` records while a match sfx plays.
7. **CHN loop units.** The arm stores `loopstart`/`looplen` into `ch+0x44/
   ch+0x48` and `declared` into `ch+0x4C` (`FUN_000b7fe8` `0xB8091..0xB80A3`),
   i.e. the same unit as the declared nibble count; a runtime probe of the
   looping producer `0xA83F1` while a chant loops would confirm the cursor
   wrap. Capture target: break at `0xA83F1`/`0xB8610` with `ch+0x38` near
   `ch+0x48`.

## 7. Provenance (Ghidra calls, 2026-10-04, `/fifa96_le.bin`)

* Decompiled: `FUN_000a75aa`, `FUN_000a765f`, `FUN_000a7728`, `FUN_000a780e`,
  `FUN_000a62fa`, `FUN_000a72e7`, `FUN_00065544`, `FUN_00065cc0`,
  `FUN_00065cf8`, `FUN_000652f0`, `FUN_00065d90`, `FUN_00063974`,
  `FUN_00068f74`, `FUN_00068fd0`, `FUN_000670f8`, `FUN_00069338`,
  `FUN_000cbc4c`.
* Disassembled: `0xA75AA..0xA7727` (registrar/unregister),
  `0xA7728..0xA780D` (id lookup / two-voice branch),
  `0xA780E..0xA79DF` (arm), `0xA62FA`, `0xA6579`, `0xB7FE8`, `0xB81F0`,
  `0xB8610`, `0xBA41D..0xBA42F` (contrast), `0x63974..0x63A6B`.
* Searches: strings `SFX`, `BNK`, `.bnk`, `CHN_`, `SOUND`; operands
  `0x21c`, `0x3992`, `0x39cf`, `0xba41d`, `0x10021c`, `0x10b07e`; dword
  scans for object-4 offsets `0x21c`, `0xb07e`, `0xdc68` (no data pointer
  to `SFX_game.bnk`/`SFX_GAME.BNK`); `*0x4` index scan over
  `0x9F000..0xC8000` (found `0xA6717`, `0xA75AA`).
* Memory: `0x100200` (lowercase `SFX_game.bnk`), `0x1020b4`
  (`SFX_FRWK.BNK`), `0x102510` (CHN name strings), `0x109f00..0x10a300`
  (object-4 offset arrays incl. `0x109f78`), `0x103960` (sfx/bank error
  strings), `0x969b7`/`0xA69B7` (error-string jump table).
* Xrefs: `FUN_000a75aa` callers (`FUN_000652f0`, `FUN_00065d90`),
  `FUN_000a7728` callers (`FUN_000a76ea`, `FUN_000a7705`),
  `FUN_000a76ea` caller (`FUN_000a72e7`), `FUN_000a7705` callers
  (`FUN_00065544`, `FUN_00065cc0`, `FUN_00065cf8`), `DAT_00061c14` xrefs,
  `FUN_000a62fa`/`FUN_000a6579`/`FUN_000b9fdd` callers.
* Assets: 25 `.BNK` from `/SOUND` via `tools/fifa96_bind.iso_files`
  (extracted to `/tmp/opencode/fu43/banks/`); golden
  `tests/golden/sfx_game.bnk` byte-identical to `/SOUND/SFX_GAME.BNK`.
  Probes: `fu43_census.py`, `fu43_validate.c` (throwaway, not committed).
* Baseline: `make test` 33/33; this doc is the only tracked change.
