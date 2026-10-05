# FU-52 — the music sequencer: CRDF loader, track records, event scheduling, tempo math and the tick state machine

Date: 2026-10-05. Program `/fifa96_le.bin` (flat LE link image, base 0).
Scope: the sequencer behind the SFX layer — the `CRDF` resource loader
`FUN_000a73b2`, the arm/reset loop `FUN_000a7499`, the event scheduler
`FUN_000a72e7`, the tempo/limit calculator `FUN_000a7040`, the per-frame tick
`FUN_000a7136` with the track-volume updater `FUN_000a6ead`, the public
interface entries at `0x65160..0x652B8`, and the shared epilogue
`FUN_000a7039`. This closes FU-51 open leg 1 (the stride-0x74 table).
Companions: FU-43 (`.BNK` descriptors), FU-47 (`FUN_000a62fa` allocator,
`FUN_000cbc4c` RNG), FU-49 (SFX event registry), FU-50 (voice control),
FU-51 (play-by-id / arm / channel handlers, `FUN_000a780e` start-one-voice).
Every claim cites an instruction address or an image read. The decompiler
marks `FUN_000cbc4c` and `FUN_000a6c48` non-returning; `0xA7040`, `0xA6EAD`
and `0xA7136` are re-derived from raw bytes at the pruned addresses
(`disassemble_bytes`), which are ordinary instructions in all three cases.

## 0. Address frame and the CRDF blob

As in FU-35/FU-43/FU-51, code operands use object-relative addresses and the
flat image resolves them by adding the loader's relocation base, e.g. object-4
BSS `+0x100000` (`[0x148F6]` at image `0x1148F6`, `[0x14912]` at `0x114912`).
`FUN_000a73b2` copies `0xE3` dwords (`REP MOVSD`, `0xA73E8..0xA73F4`) from the
EAX pointer to `0x5D808`; the source dword at `+0` must be `0x46445243`
("CRDF", `CMP dword [EAX],0x46445243` `0xA73E0`). The copy size is `0x38C`, so
the loaded struct occupies `0x5D808..0x5DB93`. The fields the rest of the
slice reads pin the source layout exactly:

| src off | dst | bytes | role | evidence |
| --- | --- | --- | --- | --- |
| `+0x00` | `0x5D808` | 4 | magic `"CRDF"` | `0xA73E0` |
| `+0x10` | `0x5D818` | 4 | volume-formula position coefficient | `a6ead` `0xA6ED2` |
| `+0x14` | `0x5D81C` | 4 | volume-formula intensity coefficient | `a6ead` `0xA6EE3` |
| `+0x18` | `0x5D820` | 4 | position (16.16) | `0xA7209`, `0xA733D` |
| `+0x1C` | `0x5D824` | 4 | intensity ramp value | `0xA71B8..0xA71F9` |
| `+0x20` | `0x5D828` | 4 | limit / next waypoint (16.16) | `0xA735A`, `0xA7241` |
| `+0x24` | `0x5D82C` | 4 | track count | `0xA7520` loop bound |
| `+0x28` | `0x5D830` | 4 | event-record count | `0xA730A` bound; dest clamped ≤0x10 at `0xA744A` |
| `+0x2C` | `0x5D834` | 4 | random-limit scale | `0xA70B1`, `0xA70E2` |
| `+0x30` | `0x5D838` | 4 | up-direction period scale | `0xA70FE` |
| `+0x34` | `0x5D83C` | 4 | down-direction period scale | `0xA7107` |
| `+0x38` | `0x5D840` | 4 | initial event index | `MOV EAX,[0x5D840]` `0xA755B` |
| `+0x3C` | `0x5D844` | 0x40 | tempo table, 8 pairs `{a,b}` stride 8 | `0xA7071`, `0xA70D1` |
| `+0x7C` | `0x5D884` | 0x1D0 | 4 track records, stride 0x74 | `0xA74D5`, `0xA7412` |
| `+0x24C` | `0x5DA54` | 0x140 | 16 event records, stride 0x14 | `0xA7316` |
| — | — | `0x38C` | total (fits the copy exactly) | `0xA73E8` |

The geometry is self-consistent: `0x3C + 8*8 = 0x7C`,
`0x7C + 4*0x74 = 0x24C`, `0x24C + 16*0x14 = 0x38C`. This is why the tempo
table holds exactly 8 entries and the event table 16.

## 1. Loader `FUN_000a73b2` (`0xA73B2..0xA7471`) and the resource path

```
0xA73BD  CMP [0x15FC8],1..5     ; sound-state gate (same as FU-51 §5.1)
0xA73CF  MOV EAX,-1             ; out of range -> -1
0xA73D9  TEST EAX,EAX; JZ -1    ; null blob -> -1
0xA73E0  CMP dword [EAX],0x46445243; JNZ -1   ; "CRDF"
0xA73E8  ECX=0xE3; EDI=0x5D808; REP MOVSD     ; copy 0x38C bytes
0xA73F6  MOV dword [0x148FA],0           ; intensity control = 0
0xA7400  CMP dword [EAX+0x24],4; JLE; MOV [EAX+0x24],4   ; clamp SOURCE track count
0xA740D  ESI=0x5D884; EBX=0
         per track i < [source+0x24]:
0xA7419    ECX = [track+0x40]; ECX += EDX (CRDF base); [track+0x40] = ECX   ; relocate
0xA741F    byte [track+0x19] = 0x7F
0xA7432    byte [track+0x14] = 0x65        ; priority for a780e
0xA743C    [track+0x04] = track+0x28       ; EACS pointer
0xA744A  CMP [0x5D830],0x10; JLE; MOV [0x5D830],0x10   ; clamp DEST event count
0xA745D  byte [0x148F4] = 1                ; music enabled
0xA7464  byte [0x148F5] = 0                ; not paused
0xA746B  XOR EAX,EAX                       ; success
0xA746D  JMP 0xA7039                       ; shared epilogue
```

Note the asymmetry: the track count is clamped on the **source** record
(so `[0x5D82C]` keeps the raw value), while the event count is clamped on the
**copy** (`[0x5D830]`). `EDX` at entry is the CRDF base pointer (the caller
passes the blob in EAX; `EDX` was saved from the same pointer by the entry
prologue `PUSH EDX`, so the relocation base equals the blob).

Resource path (entry `0x65160`, no Ghidra function object):

```
0x65165  SUB ESP,0x100                     ; 0x100-byte scratch
0x6516B  CMP [0x55CE0],0; JZ out           ; sound system up
0x65174  CMP [0xA0C8],0; JNZ out           ; already loaded
0x6517D  EDX = EAX (caller arg); EAX = ESP
0x65180  EBX = 0x20; CALL 0x65118
0x6518D  EAX = 0x8A03C; CALL 0x68FD0       ; resource lookup
0x65197  [0xA0C8] = EAX                    ; loaded CRDF blob
0x6519C  TEST EAX,EAX; JZ out
0x651A0  CALL 0xA73B2                      ; load into 0x5D808
0x651A5  TEST EAX,EAX; JGE success
0x651A9  free [0xA0C8] (CALL 0x993EC); [0xA0C8] = 0
0x651C2  MOV dword [0x55D38],1             ; enable channel 2
```

`[0x55D38]` is channel 2's enable flag (FU-51 §2.3): loading the music enables
the music channel. The blob pointer lives at `[0xA0C8]`.

## 2. Track records (`0x5D884 + i*0x74`) and the arm/reset loop `FUN_000a7499`

The record base is `0x5D884`; the brief's "track/handle array `0x5D8D4`" is
the same records seen at `+0x50` (`0x5D8D4 - 0x5D884 = 0x50`), which is why
both the descriptor loop (`0x5D884`) and the handle stores (`0x5D8D4`) use
stride 0x74.

| off | field | writer / reader |
| --- | --- | --- |
| `+0x00` | voice bitmask (a780e `[desc]`) | arm `0xA780E` via FU-51 §5.1 |
| `+0x04` | EACS pointer = `track+0x28` | loader `0xA743C`; a780e magic check |
| `+0x08` | raw count (`record+4 = desc[8]-1`) | FU-51 §5.2 |
| `+0x0C` | unknown (CRDF data) | — |
| `+0x14` | priority, loader writes `0x65` | `0xA7432`; a780e reads |
| `+0x17` | pan scale (a780e reads `desc[0x17]`) | FU-51 §5.2 |
| `+0x19` | loader writes `0x7F` | `0xA741F`; no reader found (open leg) |
| `+0x1C` | flags (a780e reads) | FU-51 §5.2 |
| `+0x28` | embedded EACS header (0x28 bytes) | arm checks `"EACS"` |
| `+0x40` | dword file offset, relocated `+= CRDF base` | `0xA7419..0xA7429` |
| `+0x50` | **voice handle** (runtime; a780e return) | `0xA751A`, `0xA7589` |
| `+0x54` | computed volume | `a6ead` `0xA6FC5` |
| `+0x58` | computed pan | `a6ead` `0xA6FCE`/`0xA6FD7` |
| `+0x5C` | volume-curve threshold | `a6ead` `0xA6F29` |
| `+0x60` | unknown (CRDF data) | no reader found (open leg) |
| `+0x64` | base volume | `a6ead` `0xA6F8D`, `0xA6FA0` |
| `+0x68` | far volume (fade target) | `a6ead` `0xA6F40`, `0xA6F48` |

`FUN_000a7499(volume in EAX)` (`0xA7499..0xA7570`):

```
0xA74A2  CMP byte [0x148F4],0; JZ 0xA7039   ; disabled -> epilogue, no clamp
0xA74AF  [0x148F6] = EAX; clamp 0..0x7F     ; signed <0 -> 0, >0x7F -> 0x7F
0xA74D5  loop i < [0x5D82C]:
0xA74E2    EDX = -1 (id), EBX = -1 (pan), ECX = 0 (volume), push 0 (dead)
0xA74FA    EAX = 0x5D884 + i*0x74; CALL 0xA780E      ; arm one track voice
0xA7506    [0x1499E + i*4] = 0 ; [0x1498E + i*4] = 0 ; [0x5D8D4 + i*0x74] = handle
0xA7528  byte [0x15FCD] = 1
0xA752F  if byte [0x148F5] == 0:            ; not paused
0xA7538    [0x5D820] = 0; [0x5D828] = 0
0xA754C    [0x148FE] = 2                   ; state RANDOM
0xA7556    CALL 0xA7040                    ; random period/limit
0xA755B    EAX = [0x5D840]; CALL 0xA72E7   ; start initial event
0xA7565  byte [0x148F5] = 0                 ; clear pause
0xA756C  JMP 0xA7039
```

The disabled branch writes `[0x15FCD] = 0xD0` and returns (`0xA74A9..`), without
touching `[0x148F6]`. `[0x15FCD]` is the "music subsystem active" byte: 1 after
an enabled reset (`0xA7528`), 0 after stop (`0xA757C`), `0xD0` when the reset
ran with music disabled. `FUN_000b6ab3` calls the tick only when it is nonzero
(`0xB6AC5`), and the tick itself re-gates on `[0x148F4]` (`0xA7138`), so the
`0xD0` sentinel is inert (open leg).

## 3. Event records (`0x5DA54 + i*0x14`) and scheduler `FUN_000a72e7`

Record fields, all read by `FUN_000a72e7`:

| off | image | role |
| --- | --- | --- |
| `+0x00` | `0x15DA54` | nonzero = valid (`0xA7319`) |
| `+0x04` | `0x15DA58` | rate numerator (16.16 after `SHL 0x10`) |
| `+0x08` | `0x15DA5C` | time / end waypoint |
| `+0x0C` | `0x15DA60` | tempo-table index |
| `+0x10` | `0x15DA64` | sound id passed to the play path |

`FUN_000a72e7(ev in EAX)` (`0xA72E7..0xA7395`):

```
0xA72EC  gate [0x148F5]==0, 0 <= EAX < 0x10, EAX <= [0x5D830],
         record[ev].valid != 0
0xA7326  EBX = record.time << 16; EBX--            ; end = time*0x10000 - 1
0xA732C  ECX = record.rate << 16
0xA733B  if (ECX <= 0 signed) goto schedule
0xA733D  if (end >= [0x5D820]) goto schedule
0xA7345  EAX = record.id; if (id <= 0) return       ; immediate path
0xA738D  CALL 0xA76EA                               ; play id (EAX)
schedule (0xA7351, interrupt-masked PUSHFD/CLI):
0xA7353  ECX += [0x5D820]; [0x5D828] = ECX
0xA7360  if (end < ECX) [0x5D828] = end            ; limit = min(rate<<16 + pos, end)
0xA736A  [0x148FE] = 0                              ; state EVENT
0xA7374  [0x14902] = EDX (= ev)
0xA737A  CALL 0xA7040                               ; recompute period
0xA7380  EAX = record[ev].id; if (id <= 0) return
0xA738D  CALL 0xA76EA                               ; play id
```

`FUN_000a76ea` (`0xA76EA`) leaves EAX (the id) alone and calls
`FUN_000a7728(ECX=0x40, EBX=0x7F volume, EDX=-1 pan)` — the all-defaults
play-by-id wrapper (FU-51 §4.1); `0x40` is a dead argument.

The immediate path fires only when the event is **overdue**: rate positive and
`end < [0x5D820]` (current position past the event's end). It plays the id
without touching pos, limit, state or `[0x14902]`.

## 4. Tempo / limit math `FUN_000a7040` (`0xA7040..0xA7135`)

The function dispatches on `[0x148FE]` (state):

* **state 0** (`0xA705C..0xA70A7`, `0xA712D`):
  `idx = record[current_event].tempo`; `p = (tempo[idx].a * 100) >> 16`
  (arithmetic `SAR`, `0xA7079`); if `p <= 0` then `p = 1`; `period =
  |(record.rate << 16) / p|` via `CDQ; IDIV EBX` (`0xA709A`) then
  `CDQ; XOR; SUB` abs (`0xA70A2`). Stored to `[0x5DB94]` (`0xA712D`).
* **state 1** (`0xA70AC..0xA70D9`):
  `r = FUN_000cbc4c()`; `[0x5D828] = (r & 0xFFFF) * [0x5D834]` (`0xA70B1`);
  `p = (tempo[idx].b * 100) >> 16` (`0xA70D1`, the **second** dword of the
  pair); clamp; `period = |(rate << 16) / p|`; store.
* **state 2** (`0xA70DB..0xA712D`):
  `r = FUN_000cbc4c()`; `[0x5D828] = (r & 0xFFFF) * [0x5D834]`;
  `p = ([0x5D838] if new_limit > [0x5D820] else [0x5D83C]) * 100 >> 16`
  (`0xA70FE`/`0xA7107`); clamp `p >= 1`; `period = |new_limit - pos| / p`
  (`abs` **before** the `IDIV`, `0xA7125..0xA712B`); store.
* **state ≥ 3**: return (`0xA7052`).

So the table is 8 pairs `{a,b}`: state 0 takes `a`, state 1 takes `b`, state 2
uses the fixed up/down scales. `FUN_000cbc4c` is the 192-bit add-with-carry
RNG of FU-47 §2 (it returns; the decompiler's "does not return" warning is
spurious — a `RET` is at `0xCBCB7`).

## 5. Per-frame tick `FUN_000a7136` (`0xA7136..0xA72E6`)

Called by the FU-51 voice tick `FUN_000b6ab3` (`CALL 0xA7136` `0xB6ACE`),
itself reached from `0xB6AA7`; the caller gates on `[0x15FCD] != 0` first
(`0xB6AC5`).

```
0xA7138  CMP [0x148F4],0; JZ ret             ; music enabled
0xA7145  CMP [0x15FCD],0; JZ ret
0xA7152  CMP [0x148F5],0; JNZ ret            ; paused
0xA715F  volume ramp: if [0x14906] != 0:
           [0x1490E] += [0x14906]; [0x148F6] = [0x1490E] >> 16 (SAR)
           if [0x148F6] >= [0x1490A]: [0x14906] = 0; [0x148F6] = [0x1490A]
0xA719C  [0x149B2]++; every 4th call (EAX%4 == 0): CALL 0xA6EAD
0xA71B8  ramp [0x5D824] toward [0x148FA]*8 by ±8, clamped to 0..0x3FF
0xA7203  EBX = [0x148FE]
0xA7209  EDX = [0x5D820] + [0x5DB94]          ; next up
0xA720F  EAX = [0x5D820] - [0x5DB94]          ; next down
0xA7220  state 0/1/2 share: if pos < limit:
           pos = up;  if (up < limit) return
         else:
           pos = down; if (down > limit) return
0xA725B  state 0 -> [0x148FE] = 1
0xA7299  state 1 -> [0x148FE] = 2
0xA72DF  CALL 0xA7040; return                ; state 2 keeps state
0xA7230  state >= 3 -> return
```

The position is a bounce oscillator: each tick moves `pos` by `period` toward
`limit`; on reaching or crossing it, state steps `0 -> 1 -> 2` (2 stays 2) and
`FUN_000a7040` recomputes the limit (states 1/2, random) and period. `reset`
sets state 2 (`0xA754C`); `start_event` sets state 0 (`0xA736A`).

The tick also runs two volume paths not modelled by the port: the `[0x148F6]`
fade ramp (driven by `[0x14906]` = step, `[0x1490A]` = target, `[0x1490E]` =
accumulator, set by `0xA6E6B`) and, every 4th call, the track-volume updater
below. The `[0x5D824]` ramp chases the intensity control `[0x148FA]
(0..0x7F, clamped by `FUN_000a7396` `0xA7396..0xA73A4`).

## 6. Track volume updater `FUN_000a6ead` (`0xA6EAD..0xA7038`)

Called every 4th tick. It computes the master curve and then walks the tracks:

```
0xA6EB6  [0x5D800] = [0x5D820] >> 16 (SAR)          ; integer position
0xA6EC3  [0x5D7F4] = [0x5D824]
0xA6ECD  [0x5D804] = ([0x5D800]*[0x5D818] + [0x5D824]*[0x5D81C]) >> 19
0xA6F06  if [0x5D804] > 0x7F: [0x5D804] = 0x7F
per track i < [0x5D82C]:
0xA6F29    curve = [track+0x5C] >> 3
0xA6F38    if curve < [0x5D804]:
0xA6F3A      v = [track+0x64] + ([0x5D804]-curve)*([track+0x68]-[track+0x64])
                                                        / (0x7F-curve)
0xA6F68      CALL 0xCBC4C (result discarded)
0xA6F73    else v = [track+0x64]
0xA6F7E    clamp v 0..0x7F
0xA6F9D    scaled = v * [0x148F6] / 0x7F (IDIV)
0xA6FC5    [track+0x54] = scaled
0xA6FD4    [track+0x58] = v (byte into EACS+0x10 pan)
0xA6FDA    if [0x149A2 + i*4] != scaled:
             a6b21(voiceIndex = EACS+0x12, [0x14912 + scaled])   ; FU-50 §2.3
0xA6FF8    if [0x14992 + i*4] != v: a6c48(voiceIndex, v)
0xA7015    [0x149A2 + i*4] = scaled; [0x14992 + i*4] = v
```

`[0x14992 + i*4]` caches the last applied pan-ish value and
`[0x149A2 + i*4]` the last applied scaled volume; reset clears both
(`0xA7506`/`0xA7510`, first store at `i=0` hits `0x14992`/`0x149A2` because
ESI is pre-incremented). `0x14912` is a static 128-byte response curve (image
read `0x114912`: `0,0,1,2,3,4,4,5,...,0x7F`) — the volume-to-gain map. The
`0xA6F68` RNG call's result is thrown away (open leg).

## 7. Public interface entries at `0x65160..0x652B8` and the auxiliary entries

| entry | role | evidence |
| --- | --- | --- |
| `0x65160` | load music resource (gates `[0x55CE0]`, `[0xA0C8]==0`) | `0xA0C8` store `0x65197` |
| `0x651D8` | channel-2 volume handler: clamp into `[0x148F6]` | FU-51 §2.3, `JMP 0xA7472` `0x651EA` |
| `0x651F0` | trigger event: gates + `[0x55D08] > 0`; `[0x55D50]=ev`; `JMP 0xA72E7` | `0x6520B..0x65210`; 44 call sites |
| `0x65218` | set intensity: gates + `[0x55D08] > 0`; `[0x55D54]=value`; `JMP 0xA7396` (clamp 0..0x7F into `[0x148FA]`) | `0x65233`, `0x65238` |
| `0x65240` | stop tracks, keep loaded: `CALL 0xA7571`; `[0x55D4C]=0` | `0x65255..0x6525A` |
| `0x65264` | full off: stop as above then `CALL 0xA73A5` (sets `[0x148F4]=0`) | `0x65287..0x65292` |
| `0x652B8` | start: gates + `[0x55D4C]==0` -> `[0x55D4C]=1`, `EAX=[0x55D08]`, `CALL 0xA7499` | `0x652D4..0x652E4`; caller `FUN_00091dd8` `0x91DE1` |
| `0xA6E5E` | pause: `CALL 0xA7571`; `[0x148F5]=1` | no static xrefs (open leg) |
| `0xA6E6B` | fade restart: `step=[0x148F6]<<16/target`, `[0x1490A]=[0x148F6]`, `[0x1490E]=0`, `CALL 0xA7499(0)` | no static xrefs (open leg) |
| `0xA7571` | stop: `[0x15FCD]=0`; for each track `a6cdc(handle)`; `[0x148F5]=0` | `0xA7573..0xA75A0` |
| `0xA73A5` | stop + disable: `CALL 0xA7571`; `[0x148F4]=0` | `0xA73A5..0xA73B1` |
| `0xA7396` | intensity clamp `[0x148FA] = 0..0x7F` | reached by `JMP` from `0x65238` |

`[0x55D08]` is channel 2's current volume (FU-51 §2.3's current array at
`0x55D00 + ch*4`), `[0x55D38]` its enable flag, `[0x55D4C]` the
"music started" latch, and `[0x55D50]`/`[0x55D54]` the last event/intensity.

`FUN_000a7039` (`0xA7039`) is **not** a bare RET: its bytes are
`C9 5F 5E 5A 59 5B C3` = `LEAVE; POP EDI; POP ESI; POP EDX; POP ECX; POP EBX;
RET`. It is the shared epilogue of the `PUSH EBX/ECX/EDX/ESI/EDI + ENTER`
functions in this cluster, reached by `JMP` from `a7499`/`a73b2` (`0xA756C`,
`0xA73D4`, `0xA746D`) and by fallthrough from `a6ead` (`0xA7038`).

## 8. Port mapping

New API in `fifa96_music.h/.c` (existing APIs unchanged), TDD tests in
`tests/test_music.c`:

| port | original |
| --- | --- |
| `struct fifa96_music_config` | CRDF fields `[0x5D82C]`/`[0x5D830]` counts, event records, tempo pairs, `[0x5D834]`/`[0x5D838]`/`[0x5D83C]` scales, `[0x5D840]` initial event |
| `fifa96_music_init` | static BSS defaults (`[0x148F6]=0x7F`, `[0x148FE]=2`) and config copy |
| `fifa96_music_reset(m, volume)` | `FUN_000a7499`: disabled -> `active=0xD0` and no clamp; else clamp 0..0x7F, arm one handle per track (backend `arm`), clear per-track caches, `active=1`; if not paused: `pos=0`, `limit=0`, `state=RANDOM`, `tempo_calc`, `start_event(initial)`; clear paused |
| `fifa96_music_start_event(m, ev)` | `FUN_000a72e7`: bounds/valid gates, overdue -> play id, else `limit = min(rate<<16+pos, end)`, state EVENT, `tempo_calc`, play id |
| `fifa96_music_tempo_calc(m)` | `FUN_000a7040` states 0/1/2 incl. signed `IDIV`, abs, period clamp, RNG limit |
| `fifa96_music_tick(m)` | `FUN_000a7136` timing core: gates, ±`period` toward `limit`, state `0->1->2`, `tempo_calc` on crossing |
| `FIFA96_MUSIC_VOLUME_MAX` (`0x7F`) | reset clamp and response-curve domain |

Faithfulness notes: division is **signed** `IDIV` (the brief's "unsigned div"),
then abs. The schedule min is the original's **signed** `JGE` comparison
(`0xA7360`), so a non-positive rate keeps the wrapped `rate<<16 + pos` limit
even when it is negative (pinned by a test). `start_event` treats
`event >= event_count` as absent (the original inspects slot `ev == count`,
which the 16-entry CRDF table makes safe). `tempo_calc` clamps a record's tempo
index and the track/event counts to the CRDF geometry (`8`/`4`/`16`); the
original would read out of bounds for an out-of-range index. `rate << 16` and
the position arithmetic wrap in 32 bits exactly as the original.
Not ported (documented): the `a780e` arm internals (backend `arm` hook), the
per-track volume updater `a6ead`, the `[0x148F6]` fade ramp, the `[0x5D824]`
intensity ramp, and the channel/event registry glue.

Tests: `tests/test_music.c` pins the tempo math (period clamp, signed division
truncation, abs), event scheduling (bounds, invalid record, limit/min path,
overdue/else path, non-positive rate), the reset loop over 0/1/4/>4 tracks,
volume clamp 0/0x7F/0x80, and the tick state transitions (up/down, gates,
state >= 3).

## 9. Errata (quoted brief facts vs the disassembly)

1. Brief: "`u = ([0x5da58 + cur*0x14] << 16) / period` (unsigned div)".
   Actual: `CDQ; IDIV EBX` (`0xA709A`, `0xA709B`) — signed division, followed by
   `CDQ; XOR EAX,EDX; SUB EAX,EDX` abs (`0xA70A2..0xA70A5`). The schedule guard
   likewise tests the shifted rate with `JLE` (`0xA733B`), not `JC`; rates with
   bit 15 set schedule. Errata for "unsigned".
2. Brief: "`FUN_000a7039` ... bare RET (stub? runtime-patched?)". Actual:
   shared `LEAVE; POP EDI/ESI/EDX/ECX/EBX; RET` epilogue (`0xA7039`, bytes
   `C9 5F 5E 5A 59 5B C3`), used as a return target by `a7499`/`a73b2`/`a6ead`.
   Not patched and not a stub.
3. Brief: "state 1 or 2 -> `FUN_000cbc4c()` ... derive why". Actual: the RNG
   supplies the next bounce endpoint, `[0x5D828] = (rand & 0xFFFF) *
   [0x5D834]` (`0xA70B1..0xA70BF`, `0xA70E2..0xA70F0`); state 1 takes the
   period from `tempo[idx].b`, state 2 from `[0x5D838]`/`[0x5D83C]` by
   direction. FU-47 §2 already documents the routine.
4. Brief: "Track/handle array `0x5d8d4`, stride `0x74`, count `[0x5d82c]`".
   Actual: the records start at `0x5D884`; `0x5D8D4 = 0x5D884 + 0x50` is the
   handle word of the same 0x74-stride records. Also `[0x5D82C]` is the raw
   CRDF track count: the loader clamps only the source record (`0xA7400`),
   while the event count is clamped in place at `0x5D830` (`0xA744A`).
5. Brief: "`FUN_000a72e7` ... if `id > 0` call `FUN_000a76ea()`; else path
   uses `[0x5da64 + ev*0x14]` directly". Actual: both branches load the id and
   call `a76ea` when `id > 0` (`0xA7380..0xA738D` schedule, `0xA7345..0xA734F`
   immediate). The immediate/else branch runs only when rate is positive and
   `end < [0x5D820]` (overdue event).
6. Brief: "Event/sequence records at `0x5da54`, stride `0x14`" — actual, and
   the count bound `[0x5D830]` is CRDF `+0x28` clamped to 16; the brief's field
   table (`+0` valid, `+4` rate, `+8` time, `+0xC` tempo index, `+0x10` id) is
   confirmed.
7. Brief: "`FUN_000a7499` ... if `[0x148f4]!=0`: `[0x148f6] = clamp(vol...)`;
   loop ... `[0x15fcd]=1`; if `[0x148f5]==0`: ...; `[0x148f5]=0`;
   `FUN_000a7039()`. Else `[0x15fcd]=0xd0`." Confirmed, with the precision
   that the disabled branch does **not** clamp `[0x148f6]`, and the
   `[0x14992]`/`[0x149a2]` clears are `0x1498E + i*4` / `0x1499E + i*4` with
   the counter pre-incremented (`0xA7506..0xA751A`), i.e. bases `0x14992`/
   `0x149A2`.
8. Brief: "`FUN_000a72e7` is called from the SFX event path (`FUN_000651f0`)
   and from `FUN_000a7499`." Confirmed (`0x65210`, `0xA7560`); `FUN_000651f0`
   has 44 call sites but none inside `FUN_000651f0`'s own cluster.

## 10. Open legs

1. **CRDF producer.** The blob comes from `FUN_00068FD0(0x8A03C)` after
   `FUN_00065118(0x20)` (`0x65180..0x65197`); `0x8A03C` is not a printable
   name and no file mapping is derived. `[0xA0C8]` holds the blob.
2. **Track fields `+0x0C`, `+0x60`** (CRDF data) and **`+0x19`**: the loader
   writes `0x7F` at `0xA741F` but no reader was found in this frame; the
   `+0x40` relocated pointer's target is likewise untraced.
3. **`0xD0` sentinel.** Why the disabled reset writes `0xD0` (not 0/1) to
   `[0x15FCD]` (`0xA74A9`) — both readers only test nonzero.
4. **`0xA6E5E` (pause) and `0xA6E6B` (fade restart) have no static xrefs**;
   they are reached indirectly (or dead) in this build.
5. **Discarded RNG calls.** `a6ead` calls `FUN_000cbc4c` at `0xA6F68` and
   ignores EAX before applying the fade value; the original source intent is
   unknown.
6. **`FUN_000a6c48`** (the second per-track applier in `a6ead`) is
   decompiler-pruned; only its call site (`0xA700A`) is derived here.
7. **`[0x149B2]`'s data reference** at `0xB7260` (outside the resolved code
   xrefs) — the 4-tick cadence is clear, the other consumer is not.
8. **CRDF numeric semantics.** `[0x5D818]`/`[0x5D81C]` coefficients,
   `[0x5D834]` random-limit scale and the `[0x5D838]`/`[0x5D83C]` period
   scales are only known structurally; retail values live in the resource.

## 11. Provenance (Ghidra, 2026-10-05, `/fifa96_le.bin`)

* Disassembled: `FUN_000a73b2`, `FUN_000a7499`, `FUN_000a72e7`,
  `FUN_000a7040` (raw bytes at `0xA70B1`/`0xA70E0`/`0xA7104`), `FUN_000a7136`,
  `FUN_000a6ead` (raw bytes at `0xA6F68`), `FUN_000a6e6b`, `FUN_000a73a5`,
  `FUN_000a7571`, `FUN_000a7396`, `FUN_000cbc4c`, `FUN_000b6ab3`, `0x65150`,
  `0x65180`, `0x65240`, `0x6525F`.
* Decompiled: `FUN_000a7040`, `FUN_000a72e7`, `FUN_000a7499`, `FUN_000a7136`,
  `FUN_000a6ead`, `FUN_000a76ea` (register check), `FUN_000a6b21`,
  `FUN_000a6c48` (pruned).
* Xrefs: `0x5D820` (15), `0x5D828` (12), `0x5DB94` (5), `0x5D840` (1),
  `0x5D824` (9), `0x15FCD` (4), `0x148F4` (5), `0x148F5` (7), `0x148FA` (3),
  `0x149B2` (3), `0x55D08` (3), `0x55D4C` (4), `0x55D38` (8),
  `0xA7039` (4), `0xA7136` (1), `0xA7499` (2), `0xA72E7` (2), `0xA73B2` (1),
  `0x651F0` (44 call sites), `0xA76EA`, `0xA7396` (1), `0x55D50`/`0x55D54` (1).
* Image reads: `0x1148F6` (`0x7F`; `[0x148FE]=2` static), `0x115FCD` (0),
  `0x114912` (the 128-byte response curve), `0xA7030` (epilogue bytes),
  `0xA7039` (`C9 5F 5E 5A 59 5B C3`).
* Baseline: `ctest -N` 42 tests before the port; 43 after it
  (`tests/test_music.c`); `make test` green; ASan/UBSan clean. This doc is the
  first tracked change of the slice.
