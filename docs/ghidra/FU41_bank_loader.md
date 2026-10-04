# FU-41 — audio bank loader: `SOUND/*.VIV` banks and the EACS entries inside

Date: 2026-10-04. Program `/fifa96_le.bin` (flat LE link image, base 0).
Scope: the retail sample banks — the BIGF container, the directory record,
how a bank entry's EACS payload is located, and how the engine registers and
arms an entry for playback. Closes FU-39 §7 leg 3 (bank arm mapping, `d0-1`
truncation) and the loader half of FU-35 §6.5. Companion slices: FU-4 (LE
image), FU-35 (EACS header/queue), FU-37 (mixer/voices), FU-39 (f10==2
decode), FU-40 (f10==2 port).

Address frame (FU-37 §0, FU-4): the flat image does not apply LE fixups.
Listing addresses below are image addresses; operands to object 4 data are
stored object-relative and land at image+0x100000 (e.g. the path strings at
`0x102934` = "TM000000.SPC", `0x10a098` = "SOUND"), while callback constants
inside object 1 need `+0x10000` (e.g. stored `0xa8084` → `0xA8084`).
Every claim cites an instruction address or a probe output.

## 1. The bank container is BIGF v2

Retail `SOUND/*.VIV` are EA "lumpy" BIGF files. `FUN_000a2410` sniffs the
format from the first big-endian word (`0xA2429..0xA2438`): `0xC0FB` ⇒ version
1, `"BI"` ⇒ version 2 (`0xA2446..0xA2452`). All 488 retail `.VIV` are version 2.
The directory is walked by `FUN_000a275c` (by index) and `FUN_000a246c` (by
name; case-insensitive compare `FUN_000a81a5` at `0xA24C9`/`0xA2555`), with a
thin pointer wrapper `FUN_000a263c` (base + returned offset).

BIGF v2 layout, instruction-cited from `FUN_000a275c`'s version-2 branch
(`0xA2829..0xA28C1`):

| off | width | meaning | citation |
|---|---|---|---|
| 0x00 | char[4] | `"BIGF"` | version sniff `0xA2429..0xA244D` |
| 0x04 | BE32 | file size (validated == file size, 488/488) | — |
| 0x08 | BE32 | entry count (validated == walked, 488/488) | — |
| 0x0C | BE32 | directory end == first data offset | `ADD EAX,0xc; MOV EAX,[EAX]; BSWAP` `0xA2836..0xA283F`; loop bound `0xA284F..0xA2853` |
| 0x10 | entry[] | directory records | `ADD EDX,0x10` `0xA284C` |

Each record is `[BE32 data offset][BE32 size][name NUL-terminated]`:

```
EDI = EDX+8                          ; 0xA2855  name pointer
offset = bswap32([EDX])              ; 0xA285C..0xA2876  ([EDX] read, BSWAP)
size   = bswap32([EDX+4])            ; 0xA2878..0xA288F
stride = strlen(name) + 9            ; 0xA28A0..0xA28AD (REPNE SCASB + 9)
```

On a hit `FUN_000a275c` stores the name pointer at `[0x5bfac]`, the size at
`[0x5bfb0]` (`0xA2863`, `0xA2891`) and returns `base + offset` (`0xA2896`,
`0xA289A`). The by-name walk `FUN_000a246c` is identical but returns the
record offset (0 on miss) and lands the size in `[0x5bfb0]`; the thin wrapper
`FUN_000a263c` (`0xA263C..0xA265F`) adds the base, returning `base + offset`
or 0. There is no hash table, no lookup cache, and no per-entry header
outside the record.

Observed properties (probe §4.1): records are packed with no inter-entry
padding (`stride = 8 + strlen + 1`); the last name is followed by 0–3 pad
bytes (arbitrary values) before the `+0xC` end, which the walker simply skips
(measured pad lengths: 0 ×243, 1 ×4, 2 ×240, 3 ×1); directories are
contiguous and monotone (`entry[i].off + size == entry[i+1].off`,
17865/17865), and the header `+0xC` always equals the first entry's data
offset (488/488). Names are lowercase `xxxxxxxx.spc`; PHR phrase banks carry
one final non-EACS filler `ZZZZZZZZ.DAT` (9 files, always 1728 bytes) which
the by-name game paths never request.

The container sibling `SOUND/*.BNK` is a different, older form — a stride-4
little-endian u32 offset table at file start (the shape the port already
models with `fifa96_viv_entry_at`; `SFX_GAME.BNK` starts `0, 0x200, 0x248,
0x290, …`). Some `.BNK` entries embed EACS payloads, but the `.BNK` path is
not needed for `.VIV` extraction and is out of scope here.

## 2. Entry → EACS mapping

A directory record's `offset` is absolute from the BIGF base and `size` is the
complete entry, EACS header included. The entry begins with the same 32-byte
EACS header defined in FU-35 §2, but the bank stores it with different intent
and values. Corpus histogram (all 18344 EACS entries, every retail
`SOUND/*.VIV`, probe §4.1):

| off | width | video `1SNh` | bank `.spc` | corpus |
|---|---|---|---|---|
| 0x00 | char[4] | `"EACS"` | `"EACS"` | 18344/18344 |
| 0x04 | u32 LE | rate (16000/16384) | rate | 16000 ×18344 |
| 0x08 | u8 | f8 | f8 | 2 ×18344 |
| 0x09 | u8 | f9 | f9 | 1 ×18344 |
| 0x0A | u8 | f10 | f10 | 2 ×18344 |
| 0x0B | s8 | voice 0..15 (signed) | **-1** (unsigned) | -1 ×18344 |
| 0x0C | u32 LE | declared samples | **declared nibbles** | see below |
| 0x10 | s32 LE | forced -1 | -1 | -1 ×18344 |
| 0x14 | u32 LE | forced 0 | 0 (no loop) | 0 ×18344 |
| 0x18 | u32 LE | absolute data pointer | **relative offset 0x20** | 0x20 ×18344 |
| 0x1C | u32 LE | voice copy | unused metadata | 0x158 ×7414, 0x168 ×10925, 5 odd |
| 0x1D | u8 | volume | 1 (mostly) | 1 ×18339, 0 ×5 |
| 0x1E/0x1F | u8 | unread | unread | 0 ×18341 / 0 ×18342 |

The payload is `entry + 0x20 .. entry + size`, raw adaptive-delta nibble data
(`f8=2,f9=1,f10=2` ⇒ FU-39's mono nibble decoder `0xC4CC4`). Unlike the video
`1SNd` stream, a bank entry has **no per-chunk 20-byte block header**: the whole
entry is one block and the cursor starts at the first nibble of `+0x20` (MSB
nibble of byte 0 first). The declared `+0x0C` is the authoritative played
length in nibbles; the payload often carries 0–7 unused trailing nibbles
(histogram in §4.1). `declared <= 2 * payload` in all 18344 entries.

**Data-pointer relocation.** `+0x18` holds the literal `0x20` (offset to the
payload), never an absolute pointer. Three direct-play wrappers normalize it
just before arming:

```
CMP dword [EAX+0x18], 0x20        ; 0xBA3C6 / 0xBA3E8 / 0xBA407
JNZ skip
LEA EDI/ECX,[EAX+0x20]
MOV [EAX+0x18], EDI/ECX           ; 0xBA3CF / 0xBA3F1 / 0xBA410
CALL 0x000ba41d
```

`FUN_000a8084` (the stream registrar, §3) does the same unconditionally:
`LEA EBX,[EAX+0x20]; MOV [EAX+0x18],EBX` (`0xA809D..0xA80A0`). After this the
header's `+0x18` is a true pointer and the shared arm path works unmodified.

**Differences from video `1SNh` EACS**, summarised:

* voice byte is -1 (the video parser requires 0..15, `FUN_000a79dc`); the
  runtime voice is supplied separately (header `+0x1C`, §3);
* `+0x18` is a relative offset relocated to `header+0x20`, not a parser-written
  absolute pointer into a queued chunk;
* the payload has no 20-byte per-chunk block header and no `1SNd`/`1SNe`
  queue around it (the bank is self-contained);
* declared counts nibbles (2 per byte), not samples; block size is 2 bytes.

## 3. How the game locates and arms a bank entry

### 3.1 Locate

Bank file names are built from a 4-character id into two templates in object
4: `FUN_00065e70` copies `"TEM_0000.VIV"` (image `0x102914`, `MOV ESI,0x2914`
`0x65E77`) and `FUN_00065ea0` copies `"PLR_0000.VIV"` (image `0x102924`,
`MOV ESI,0x2924` `0x65EA7`) before overwriting bytes 4..7 with the id
(`0x65E82..0x65E96` and `0x65EB2..0x65EC6` respectively). `FUN_00065118`
builds the full path as `"SOUND"` (image `0x10a098`, strcpy `0xA1764`) +
`"/"` (image `0x102904`) + name (`0x6511F..0x65148`). The sample engine
`FUN_00066eb4` builds that path, opens/reads the file (`FUN_00068fd0`), builds
per-entry query names with `FUN_00065f10` (`"00000000.SPC"` template, image
`0x102944`) and resolves each one in the opened bank through `FUN_000a263c`
(call `0x66F5D`), i.e. the BIGF by-name lookup of §1.

The streaming path (`FUN_00067800`) enumerates the opened bank's directory
itself (`FUN_000a2718` count, `FUN_000a28c4` name-by-index), builds a 0x404-byte
name list at `0x55fb0`, then calls `FUN_000675a0`; that walks the list, looks
each name up with `FUN_000a263c` (`0x676A0`) and registers every hit with
`FUN_000a8084` (`0x67708`). Either way, the located object handed to playback
is the entry pointer (`BIGF base + record offset`), whose first 32 bytes are
the EACS header.

### 3.2 Register (track ring) and arm

`FUN_000a8084` (`0xA8084..0xA80E1`) registers one EACS header:

```
if ([0x15fcc] == 0) return -1          ; streaming not enabled  0xA8086..0xA8096
[EAX+0x18] = EAX+0x20                  ; bank data-pointer fixup 0xA809D..0xA80A0
idx = [0x14a90]; [0x14a90] = idx+1; if (idx+1 == 0x14) [0x14a90] = 0
[0x149f0 + idx*8] = EAX                ; ring[20] of {header, duration}
[0x149f0 + idx*8 + 4] = ([EAX+4] * EDX) / 100   ; rate-based duration 0xA80C7..0xA80D6
```

`FUN_000a7fd4` (`0xA7FD4..0xA8063`) enables streaming: it takes two child
voice indices (validated 0..15), clears the ring, sets the current-voice
selector `[0x14aa5]`, stores the two voices at `[0x14a9d]`/`[0x14aa1]`, the
volume at `[0x14a99]`, and sets `[0x15fcc] = 1`.

Each mixer tick calls `FUN_000a7edf` (from `FUN_000b6ab3`, the per-frame voice
tick). It checks the currently armed voice's remaining frames
(`FUN_000a6de2` → `FUN_000b81a0`, `0xA7F06..0xA7F0E`) against the current
track's duration; when the track is due it advances the ring index and swaps
the current voice between `[0x14a9d]`/`[0x14aa1]` (`0xA7F35..0xA7F4E`), then
arms the next track directly:

```
EBX = 0x40                       ; pan centre
ECX = [0x14a99]                  ; volume
EAX = [0x14aa5]                  ; current child voice
ESI = [EDX]                      ; ring entry = EACS header pointer
EDX = EAX; EAX = ESI
CALL 0x000ba41d                  ; FUN_000ba41d(header, voice)
```

(`0xA7F53..0xA7F69`). This is the normal retail route for streamed bank
phrases; the three direct-play wrappers at `0xBA3B6`/`0xBA3DE`/`0xBA403` are
the same operation for a one-shot play. They normalize `+0x18` and call
`FUN_000ba41d` with default record volume/pan: wrapper `0xBA3B6` sets volume
`ECX=0x7f`, pan `EBX=0x40` (`0xBA3BA..0xBA3BF`); wrapper `0xBA3DE` sets pan
`0x40` and takes the volume from the caller's `EBX` (`0xBA3E1..0xBA3F8`);
wrapper `0xBA403` uses the caller's `CL`/`BL` as-is (`FUN_000ba41d` stores
them at record `+0x24`/`+0x27`, `0xBA4A8`/`0xBA4AE`).

### 3.3 Arm internals and stop

`FUN_000ba41d` (`0xBA41D..0xBA4D4`): validates `[EAX] == "EACS"`
(`0xBA427`) and the voice argument 0..15 (`0xBA437..0xBA43E`), requires sound
state `[0x15fc8]` in 1..5 (`0xBA448..0xBA458`), initializes the EACSNDF record
`0x61994 + voice*0x28` (defaults; `0xBA462..0xBA4AE`), calls `FUN_000b9fdd`
(`0xBA4B1`), copies record bytes +8/+9 into the header's `+0x1D`/`+0x1E`,
writes the **runtime voice to header `+0x1C`** (`0xBA4C2..0xBA4C5`) and calls
`FUN_000a6579` (`0xBA4CA`). Note what it does *not* do: header `+0x0B` stays
-1.

`FUN_000a6579` (`0xA6579..0xA6628`) derives the format flags from header
+8/+9/+10 and the sign of +0x0B (`0xA6592..0xA65D2`) — bank flags are
`0x0A` (f8=2 → 8, f10=2 → 2; f9=1 gives no 4; +0x0B=-1 gives no 0x10, so
**unsigned**) — and calls `FUN_000b7fe8(voice=+0x1C, pitch, flags, R, L,
rate=+4, looplen=+0x14, loopstart=+0x10, blocks=+0xC, data=+0x18)` (pushes
`0xA65FD..0xA661D`).

`FUN_000b7fe8` (`0xB7FE8..0xB80F9`) with flags `0x0A`:

* shift = 1 (flag 8 only): `ch+0x14 = data >> 1` (`0xB8034..0xB8047`),
  `ch+0x10 = blocks + (data>>1) - 1` (`0xB8051..0xB8059`);
* loop flag `ch+1 = (looplen != 0) = 0` (`0xB800D..0xB8013`);
* second cursor (flag 2): zeroes `ch+0x30/0x34/0x38/0x3C`, `ch+0x44 = loopstart`,
  `ch+0x48 = loopstart + looplen`, `ch+0x4C = blocks` (header `+0xC`), `ch+0x40
  = data` (header `+0x18`, now `header+0x20`) and sets `ch+2 = 2`
  (`0xB8068..0xB80AA`);
* the signed branch (`TEST [ESI+3]`, `0xB80AD`) is skipped because flags has no
  `0x10`, so the code at `0xB80B4..0xB80C5` that reads the first 20-byte block
  header and sets `ch+0x4C = u32[data] - 1` **never runs for banks**. This is
  the resolution of FU-39 §2.2's `d0-1` question: the truncation belongs to
  the signed/video arm only; a bank plays exactly the declared nibbles.

`FUN_000b81f0` (`0xB81F0..0xB82E2`) then selects, for these flags:
reader `0xB8AE1` (`flags&0xC == 8`, `0xB8242..0xB8257`); nibble decoder
`0xC4CC4` (`ch+2 != 0`, `flags&4 == 0`, `0xB825A..0xB8278`); producer
`0xB8610` (no loop, `flags&0x12 == 2`, `0xB8298..0xB82CB`). All three are the
FU-39 bank functions.

Producer `0xB8610` (`0xB8610..0xB86C7`):

```
left   = max(0, ch+0x4C - ch+0x38)          ; nibbles left (declared - consumed)
frames = (left << 24) / ch+0x2c             ; 8.24 step (FU-37 §A.5)
if (frames == 0) { FUN_000b80fa(voice); return 0; }   ; stop
n = min(frames, [0x406b0])
count = ceil(n * step + ch+0x3c/2^32)       ; units this call
src   = ch+0x40 + (ch+0x38 >> 1)            ; byte address, flags&4 == 0
parity= ch+0x38 & 1                         ; first nibble of that byte
(*ch+0x58)(parity, src, 0x38698, count, &ch+0x34, &ch+0x30)
ch+0x38 += count
ch+0x14 = 0x38698 >> ch+8                   ; readers mix the staging buffer
```

The stop path is `FUN_000b80fa` (`0xB80FA`): channel `state = 0` and
`FUN_000ba00e(voice)` resets the EACSNDF record. There is no `1SNd`/`1SNe`
participation anywhere in the bank path — those tags belong to the TGV
companion queue (FU-35 §1, FU-39 §5), and bank entries are armed directly
from memory. A repeating bank phrase would need header `+0x14 != 0` (loop
producer), but every retail `.spc` stores 0 (corpus §4.1), so all bank
playback is one-shot.

## 4. Structural validation (throwaway, not committed)

Assets were extracted from the read-only ISO with `tools/fifa96_bind.iso_files`
to `/tmp/opencode/fu41/banks/` (plus a full-ISO corpus pass in memory). Probe
scripts: `/tmp/opencode/fu41/fu41_bank_probe.py`,
`fu41_bank_probe2.py`, `fu41_validate.c` (drives the committed
`fifa96_eacs_parse`/`fifa96_eacs_delta_unit`), `fu41_voice.c`.

### 4.1 Whole-corpus census (488 banks, 18353 entries)

```
$ python3 fu41_bank_probe.py
files=488 entries=18353 eacs_ok=18344 magic_bad=9 data_bytes=120244820
rates={16000: 18344}
(f8,f9,f10)={(2, 1, 2): 18344}
voices={-1: 18344} dataptr={'0x20': 18344}
loop(start,len)={(-1, 0): 18344} vol={1: 18339, 0: 5}
slack=2*payload-declared: {0: 2551, 1: 2290, 2: 2341, 3: 2225,
                           4: 2290, 5: 2217, 6: 2159, 7: 2271}
$ python3 fu41_bank_probe2.py
files=488 entries=18353 eacs=18344 non_eacs=9 pad_bytes=487
== filler entries ==
  ('ZZZZZZZZ.DAT', 1728, '01000000010000000100000001000000') x1
  ('ZZZZZZZZ.DAT', 1728, '01000000000000000100000001000000') x1
  ('ZZZZZZZZ.DAT', 1728, '00000000000000000000000000000000') x3
  ('ZZZZZZZZ.DAT', 1728, '02000000010000000200000002000000') x2
  ('ZZZZZZZZ.DAT', 1728, '06000000040000001200000006000000') x1
  ('ZZZZZZZZ.DAT', 1728, '05000000030000001000000005000000') x1
$ python3 -c "...wall checks..."        # inline, same ISO
pad_len_hist {2: 240, 3: 1, 0: 243, 1: 4}
header_first_data==entry0.off 488 / 488
contiguous entries 17865 / 17865 banks_with_gaps 0 []
```

(The table-end/pad checks are the second probe's fixed regression; the
by-bank entry tallies are printed per file by `fu41_bank_probe.py`.)

Entry-count tallies match FU-35/FU-39 where they overlap: `TEM_T519.VIV` and
`TEM_T169.VIV` have exactly 6 `.spc` entries each; `PLR_T002.VIV` has 64;
`PHR_FULL.VIV` has 619 records, of which 618 are EACS and the last is the
1728-byte `ZZZZZZZZ.DAT` filler.

### 4.2 The committed parser over real entries

`fu41_validate.c` forces the header voice byte to 0 (the runtime substitutes a
voice at `+0x1C`, §3.3) and then runs `fifa96_eacs_parse` plus a full
`fifa96_eacs_delta_unit` decode of every entry:

```
$ ./fu41_validate banks/TEM_T519.VIV
  [0] tmt51901.spc off=0x000090 size= 2516 payload= 2484 decl= 4968 slack=0
      first=11,41,104,-32 range=[-14515,18018] clamp=0
  ...
  entries=6 eacs=6 filler=0 walked_end=0x8e table_end=0x90 pad=2
  declared_units=54280 payload_bytes=27140 nominal_nibbles=54280
  slack_hist=[6,0,0,0,0,0,0,0] global_range=[-20943,21315]
$ ./fu41_validate banks/TEM_T169.VIV
  entries=6 eacs=6 filler=0 walked_end=0x8e table_end=0x90 pad=2
  declared_units=60328 payload_bytes=30172 nominal_nibbles=60344
  slack_hist=[2,0,1,0,2,0,1,0] global_range=[-24704,22471]
  (2/6 declared == 2*payload; the rest 2-6 nibbles short, FU-35 §6.1)
$ ./fu41_validate banks/PLR_T002.VIV
  entries=64 eacs=64 filler=0 walked_end=0x550 table_end=0x550 pad=0
  declared_units=873427 payload_bytes=436816 nominal_nibbles=873632
  slack_hist=[14,8,10,0,8,7,9,8] global_range=[-32768,32538]
$ ./fu41_validate banks/PHR_FULL.VIV
  entries=619 eacs=618 filler=1 walked_end=0x32d7 table_end=0x32d8 pad=1
  declared_units=14360758 payload_bytes=7181480 nominal_nibbles=14362960
  slack_hist=[68,72,78,83,88,76,78,75] global_range=[-32768,32767]
```

Every EACS entry of the four harnessed banks (694 = 6 + 6 + 64 + 618) parses
as `DELTA_MONO` with `data_off == 0x20`, `block_size == 2`,
`delta_units == declared`; decoding all declared units with zero start state
stays in int16 and yields musically plausible ranges — the FU-39 §6 bank
cross-check generalised from 12 to 694 entries. The Python census applies the
same header rules to all 18344 entries. The T519/T169 slack picture matches
FU-35 §6.1's "8/12 exact, rest 2-6 short".

### 4.3 The bank/parser gap (decisive)

```
$ ./fu41_voice0 banks/TEM_T519.VIV
stored voice=-1 parse rc=-4 (0=OK)        ; FIFA96_ERR_TRUNCATED: voice < 0
after byte 0x0B:=0 rc=0 format=5 units=4968
```

The committed `fifa96_eacs_parse` (src/fifa96_loader/fifa96_eacs.h:29-32 and
fifa96_eacs.c:29-32) rejects the stored bank voice -1, because it models the
video `1SNh` rule. The game's bank path never validates that byte; it passes
the runtime voice to `FUN_000ba41d` and leaves `+0x0B` at -1 for the unsigned
flag decision. A bank-aware caller must therefore bypass or override the
voice check (or call a bank variant); everything else in the parser is
correct for banks once the voice is supplied.

## 5. Port implications and open legs

1. **BIGF directory is unported.** `fifa96_viv_entry_at` models the stride-4
   u32 offset table of the `.BNK` sibling, not `.VIV` BIGF. Extracting
   samples from `SOUND/*.VIV` needs the §1 walk: `"BIGF"`, BE32 file size,
   BE32 count, BE32 end at `+0xC`, records from `+0x10`
   `[BE32 off][BE32 size][name\0]`, bound check against the end offset.
2. **Bank voice reject** (§4.3). Suggested shape: parse with a
   caller-supplied voice (the arm writes `+0x1C`), keep `+0x0B` as the
   signed/unsigned discriminator.
3. **Wrapper dispatch.** `0xBA3B6`/`0xBA3DE`/`0xBA403` have no static
   callers in the flat image (no immediate or stored-pointer reference).
   They are presumably addressed through an LE fixup/pointer table the flat
   rebuild does not apply. Capture target: break at `0xBA3B6`/`0xBA3DE`/
   `0xBA403` or at `FUN_000ba41d` (`0xBA41D`) while a chant/phrase plays; the
   normal route is `FUN_000a7edf` → `0xBA41D` (`0xA7F69`).
4. **Playlist → sample mapping.** `FUN_000675a0` registers the directory
   names it finds, but which playlist entries map to which match events, and
   the role of the `[TEAMLS]`/`[TEAMFO]`/`[TEAMOT]` markers (image
   `0x1028e0..`) and `FUN_000674bc`/`FUN_0009e974`, are not traced.
   Capture target: dump the 0x404-byte list at `0x55fb0` and the ring at
   `0x149f0` when a phrase starts.
5. **Metadata oddballs.** 5 of 18344 entries store unusual
   `+0x1C/+0x1E/+0x1F` values (`PHR_FULL`/`PHR_MOST` `14i00300.spc`:
   `+0x1C=0xFFC10048`, `+0x1E=193`, `+0x1F=255`; `PLR_T065` two entries
   `+0x1C=0`; `PLR_T551` `t5511410.spc` `+0x1C=0x160000`, `+0x1E=22`). These
   fields are overwritten at arm (`+0x1C` by `FUN_000ba41d`, `+0x1D/+0x1E`
   from the record), so they do not affect playback, but the on-disk meaning
   is unknown.
6. **`.BNK` container.** Offset-table sibling; some entries embed EACS
   payloads (`SFX_GAME.BNK` 31 `EACS` hits). Not traced here.

## 6. Provenance (Ghidra calls, 2026-10-04, `/fifa96_le.bin`)

* Decompiled: `FUN_000a2410`, `FUN_000a246c`, `FUN_000a275c`, `FUN_000a263c`,
  `FUN_000a81a5`, `FUN_000a8084`, `FUN_000a7fd4`, `FUN_000a7f94`,
  `FUN_000a810b`, `FUN_000a8172`, `FUN_000a80e2`, `FUN_000a7edf`,
  `FUN_000ba41d`, `FUN_000ba555`, `FUN_000b9fdd` call sites, `FUN_000a6579`,
  `FUN_000b7fe8`, `FUN_000b81f0`, `FUN_000b8610`, `FUN_000b80fa`,
  `FUN_000b6ab3`, `FUN_00065e70`, `FUN_00065ea0`, `FUN_00065ed0`,
  `FUN_00065f10`, `FUN_00065118`, `FUN_00066eb4`, `FUN_00066d6c`,
  `FUN_00066db0`, `FUN_000670f8`, `FUN_00067800`, `FUN_000675a0`,
  `FUN_00067948`, `FUN_00025e9c`.
* Disassembled: `0xBA300..0xBA41F` (wrappers + `FUN_000ba41d`),
  `0xA7EDF..0xA7F93`, `0xA7F94..0xA81A5` (stream API), `0xA2410`,
  `0xA275C` (`0xA2829..0xA28C1` v2 walk), `0xA6579`, `0xB7FE8`,
  `0xB81F0`, `0xB8610`, `0xB80FA`, `0x65118`, `0x65EA0`.
* Searches: operands `ba41d`, `ba3b6`, `ba3de`, `ba403`, `a8084`, `a7fd4`,
  `a263c`, `a81a5`, `2924`, `2914`, `42494746`, `a098`, `2484`, `2934`;
  strings `SOUND`, `.spc`, `.VIV`, `EACSNDF`, `TM000000.SPC`;
  byte patterns `b6 a3 0b 00`, `de a3 0b 00`, `03 a4 0b 00` (no hits —
  wrappers unreferenced in the flat image).
* Memory: image `0x1028e0..0x102980` (path/name templates),
  `0x114a80` (track state).
* Xrefs: `0x14a98/0x14a94/0x14a9d/0x14aa1/0x14aa5` (stream state),
  `0x55f70/0x55fb0` (bank buffers/playlist), `0xA7F94/A7FD4/A8084` callers,
  `FUN_000ba41d` callers.
* Assets: `/SOUND/*.VIV` (488 files) and `SFX_*.BNK`/`CHN_*.BNK` from the
  read-only `game/FIFAPCCD96.iso` via `tools/fifa96_bind.iso_files`;
  specimens extracted to `/tmp/opencode/fu41/banks/`. Probe outputs:
  `fu41_bank_census.txt`, `fu41_bank_census2.txt`,
  `fu41_bank_header_extra.txt`; harness `fu41_validate`/`fu41_voice0`
  (throwaway, not committed).
* Baseline: `make test` 32/32; this doc is the only tracked change.
