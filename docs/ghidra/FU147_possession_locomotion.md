# FU-147 — possession/locomotion

Provenance: recon draft w3, phase-6 wave-1, frozen 2026-10-09; evidence review: PASS with corrections listed inline.

Phase 6 wave 1, read-only recon on `/FIFA96.EXE` (Ghidra MCP; never the flat
bin). Scope: row-04 controlled pad arm, the `+0x6B` lane word producer, the
`+0x8D` active seed, the AI-side mover, the situation-0xB possession invokers
(`0x7546E`/`0x75B58`/`0x76072`), the pad→record movement chain, and the port
contract for pad-driven possession changes.

**Address model.** In `/FIFA96.EXE` code and data are flat: code address `A`
equals the FU-doc link address; a data immediate renders as the true flat
address (`0x15774C`, `0x1106E0`). The le.bin render of the same datum is
`A-0x100000` (`0x5774C`, `0x106E0`). Plain-hex tool calls resolve in the
default `ram` space; the `.image` overlay is empty for this program. All
`program` arguments below are `/FIFA96.EXE` unless stated.

---

## 1. Scope

1. Row-04 (code 4, `0x7E7C8`) body: dispatch, pad reads, the `ecx` selector,
   the camera arm and the `FUN_00079C20` pad step.
2. `+0x6B` lane word producer (`0x7C7AF` in `FUN_0007BF20`; helper
   `FUN_000795B4`), plus the companion producers `+0x6D`/`+0x6F`/`+0x77`.
3. `+0x8D` active seed: the consumer window `0x7DB84..0x7DB99` (row 00) and
   the producer `FUN_0008C2E0 0x8C329`.
4. AI-side mover: the per-frame record driver `FUN_0008D8EC`, the two record
   machines (`FUN_000782D0`/`FUN_0007CA54`), and the `FUN_0008E244` twin
   (formation passes) — with a correction to FU-77 §1.1's attribution.
5. Situation-0xB possession invokers `0x7546E` (row 1D stage-3 tail), `0x75B58`
   (row 1E stage-3 tail), `0x76072` (row 1E stage-7 tail); gate conditions,
   enclosing bodies, `FUN_0008A938` case-0xB routing, and what flips
   possession.
6. Pad→record consumer chain and the minimal engine seam.

## 2. Method and census floor

* `disassemble_bytes`/`read_memory`/`decompile_function`/`get_xrefs_to`/
  `search_instructions`/`get_function_by_address` on `/FIFA96.EXE` only;
  no writes, no analysis, no project change.
* `search_instructions` scans **234,864 instructions** (`truncated: false`).
  Only instructions the program has disassembled are covered; regions with no
  defined flow are not. Undefined-function regions (`0x74xx0..0x76xxx`,
  `0x7DB10..0x7F665` handler cluster) are read as raw byte windows; their
  text is quoted verbatim from the listing.
* Fresh xref counts: `get_xrefs_to 0x8A938` = **39**; `0x8E244` = **3**;
  `0x795B4` = **12**; `0x8C2E0` = **2**; `0x8C33C` = **4**; `0x7CA54` = **1**;
  `0x782D0` = **1**; `0x8D8EC` = **4**; `0x7997C` = **1**.

## 3. Evidence

### 3.1 Dispatch (action table A `0x1106E0`)

`read_memory 0x1106E0` (128 B) first 32 dwords:

```
idx 0x00: 10db0700  idx 0x01: c0db0700  idx 0x02: ccdf0700  idx 0x03: a4e10700
idx 0x04: c8e70700  idx 0x05: 94f10700  idx 0x06: b4010800  idx 0x07: b0140800
idx 0x08: 68100800  idx 0x09: 000a0800  idx 0x0A: 38170800  idx 0x0B: 08190800
idx 0x0C: 901c0800  idx 0x0D: 1c250800  idx 0x0E: 10270800  idx 0x0F: d02a0800
idx 0x10: f0550800  idx 0x11: e45d0800  idx 0x12: 683d0800  idx 0x13: 004b0800
idx 0x14: 4c780800  idx 0x15: d07c0800  idx 0x16: 30460800  idx 0x17: 30470800
idx 0x18: b0490800  idx 0x19: e4460700  idx 0x1A: 2c660700  idx 0x1B: 286d0700
idx 0x1C: 28770700  idx 0x1D: b04e0700  idx 0x1E: 0c550700  idx 0x1F: 80630700
```

So `[4]=0x0007E7C8` (row 04), `[0x1D]=0x00074EB0`, `[0x1E]=0x0007550C` — the
row/entry attribution used below is the table itself, not a doc.

### 3.2 Row-04 pad arm (first-hand windows)

`disassemble_bytes 0x7E990` (144 B) and `0x7EB80` (144 B):

```
0x7E994  MOV ECX,[0x1577CA]            ; tracked record ptr
0x7E99A  ... AX = (EBP==[0x1577CA] && [0x157750] > 0x50) ? 1 : 0   ; 0x7E9A2..0x7E9B2
0x7E9B4  EDI = [EBP+0x20]              ; slot ptr
0x7E9B7  if (!slot) { ECX=1; goto 0x7EA18 }                     ; 0x7E9BB
0x7E9C2  DX = word[slot+0x10]; DH=0; DL &= 0x20                 ; 0x7E9C6/0x7E9C8
0x7E9D1  if (DL==0) goto 0x7E9F1
0x7E9D3  EDX = dword[0x1577EE]>>16; if (<=0x70) goto 0x7E9F1    ; word 0x1577F0
0x7E9E1  MOVSD 0x157770 triple -> [EBP+0x4D]; JMP 0x7EC13        ; vector target
0x7E9F1  if (AX!=0) goto 0x7EA16                                ; -> ECX=0
0x7E9F6  EDX = [0x157A83]              ; controlled actor
0x7E9FC  if (EDX==0 || EBP==EDX) {
0x7EA04     EDX = dword[EBP+0x69]>>16  ; = word[+0x6B] lane
0x7EA0A     if ((int16)lane < 0x60) { ECX=1; goto 0x7EA18 }     ; 0x7EA0F
0x7EA16  } ECX = 0
0x7EA18  if (AX!=0) goto 0x7EB95                                ; camera arm
...
0x7EB95  TEST CX,CX; JZ 0x7EBF8                                  ; ecx==0 -> after_0f
0x7EB9A  ESI = 0x15774C; EAX=[0x1577BE]>>16; SHL 2              ; = word[0x1577C0]
0x7EBA4.. MOVSD camera triple -> rec+0x4D..55; +0x4D += lead_x<<2
0x7EBB5  EAX=[0x1577C0]>>16; SHL 2; +0x55 += lead_z<<2          ; = word[0x1577C2]
0x7EBC5  EAX = dword[EBP+0x69]>>16; if (>=0x3C0) goto 0x7EBF8   ; lane
0x7EBD8  EBX = [0x157754]; if (|EBX| > 0x570) CALL 0x79B58
0x7EBF8  TEST CX,CX; JNZ 0x7EC13                                 ; ecx!=0 -> clamp
0x7EBFD  EDX = [EBP+0x20]; EBX=[EDX+0x1E]; EDX=[EDX+0x1D]
0x7EC08/0x7EC0B  SAR EBX,0x18; SAR EDX,0x18
0x7EC0E  CALL 0x79C20                   ; target = pos + dir<<7, y=0, clamp
```

`disassemble_bytes 0x79C20` (64 B), the pad step:

```
0x79C20  PUSH ECX; MOVSX EDX,DX; ECX=EDX<<7; EDX=[EAX+0x59]+ECX; EAX+=0x4D
0x79C34  [EAX] = EDX                    ; target.x = pos.x + dir_x<<7
0x79C39  EDX = [EAX+0x14] + (MOVSX BX)<<7; [EAX+4]=0; [EAX+8]=EDX
0x79C48  CALL 0x7D3E4                   ; clamp x +-0x720 / z +-0xB10
```

Row 00's pad arm and its `+0x8D` window, `disassemble_bytes 0x7DB10` (176 B):

```
0x7DB3D  ECX = [ESI+0x20]; if (slot) {                        ; 0x7DB40..0x7DB4F
0x7DB44     EAX=[0x157A4A]>>24; if (phase != 6) {
0x7DB51        EAX=ESI; EBX=[ECX+0x1E]; EDX=[ECX+0x1D]; SAR x2,0x18
0x7DB5F        CALL 0x79C20 } }                               ; pad step (row 00)
0x7DB64  EAX=[0x157A4A]>>24; if (phase!=2) return             ; 0x7DB6C JNZ 0x7DBA8
0x7DB71  if (word[ESI+0x81]!=0) return            ; 0x7DB79
0x7DB7B  if (dword[ESI+0x89] > 0) return          ; 0x7DB82 JG 0x7DBA8
0x7DB84  AH = byte[ESI+0x8D]                      ; the "active" byte
0x7DB8E  if (AH==0) EAX=0x19 else EAX=3           ; 0x7DB92/0x7DB99
0x7DB9E  CALL 0x7D9A4                             ; install now (EDX=code, ECX=0, EBX=0)
```

So `0x7DB84..0x7DB99` is the **consumer** window (install code 3 for active,
0x19 for the inactive record), not the seed.

Pad→slot mapping, `disassemble_bytes 0x78A10` (80 B): the stores at
`0x78A39` (`[ECX]=DL`, `ECX=&slot+0x20`), `0x78A46` (`[ESI]=DL`,
`ESI=&slot+0x21`), `0x78A48` (`[EBX+0x1F]=AL`) index the tables
`0x110E1DC`/`0x110E1EC`/`0x110E1F5` from the masked pad nibble. The engine's
`fifa96_control_slot_update` + `match_run_slot_map`/`match_run_anim_a/b/c`
(`src/fifa96_engine/fifa96_match_run.c:17-30`, call at `:1057`) are these
tables.

### 3.3 `+0x6B` lane word (and `+0x6D`/`+0x6F`/`+0x77`) producers

Census (`search_instructions`, 234,864 insns, not truncated). The `+N],`
operand searches below are **MOV-mnemonic-scoped** unless noted; the full
all-mnemonic scans of `+ 0x8d],`/`+ 0x6b],`/`+ 0x77],` return 30/3/2 sites,
the extras being CMP readers only:

| field | direct MOV writer(s) | indirect writer |
|---|---|---|
| `+0x6B` | **1 MOV site** (full scan 3, +2 CMP readers): `0x7C7AF MOV word[EBP+0x6B],AX` (`FUN_0007BF20`) | `FUN_000795B4` via `EBX=&rec+0x6B` (call sites `0x799D5`, `0x8DA8F` among 12 callers) |
| `+0x6D` | **1 MOV site**: `0x7C78E MOV word[EBP+0x6D],AX` | same helper |
| `+0x6F` | **1 MOV site**: `0x7C79A MOV word[EBP+0x6F],AX` | same helper |
| `+0x77` | 2 MOV sites (full scan 2): `0x79A19` (`FUN_0007997C`), `0x7C77E` (`FUN_0007BF20`) | — |
| `+0x8D` | **0 MOV sites** with `+ 0x8d],` (full scan 30, CMP readers only) | 1 alias: `0x8C329 MOV byte[EAX-0x25],DL` (`FUN_0008C2E0`; `EAX` = next record, so `EAX-0x25 = rec+0x8D`). `+ 0x13f],` writes: 0. |

The BF20 lane block, `disassemble_bytes 0x7C760` (112 B) + `0x7C7D0` (96 B):

```
0x7C776  AX = word[EBP+0x6B]                 ; old lane
0x7C77E  word[EBP+0x77] = AX                 ; bound := old lane
0x7C782  EAX=[0x15774C]; EDI=[EBP+0x59]; EAX-=EDI
0x7C78E  word[EBP+0x6D] = AX                 ; cam.x - pos.x
0x7C792  EAX=[0x157754]; EDX=[EBP+0x61]; EAX-=EDX
0x7C79A  word[EBP+0x6F] = AX                 ; cam.z - pos.z
0x7C79E  EDX=dword[EBP+0x6D]>>16 (=+0x6F); EAX=dword[EBP+0x6B]>>16 (=+0x6D)
0x7C7AA  CALL 0x8DC68                        ; metric(dx,dz)
0x7C7AF  word[EBP+0x6B] = AX                 ; lane = distance(rec, 0x15774C/54)
0x7C7B3  EAX=[EBP] (team); ESI=[EAX+0x7C7]
0x7C7C0  DX = word[EBP+0x6B] (new lane)
0x7C7C4  if (ESI != 0 && DX >= word[ESI+0x6B]) goto 0x7C7D3   ; keep tracker
0x7C7CD  [team+0x7C7] = EBP                  ; else tracker := rec (nearest-to-camera)
0x7C7D3..0x7C901  further visibility arm: word[+0x69]>>16 <= 0x10, +0x6B < +0x77,
          [0x157820]==0, [0x157822]==0, word[0x1577BC]>>16 > 4, [0x157750]!=0,
          [0x1577CA] ... (block-level, leg 7)
```

The helper, `disassemble_bytes 0x795B4` (80 B): with `EAX`=pos triple ptr,
`EDX`=0x15774C, `EBX=&rec+0x6B`:
`+0x6D = cam.x-pos.x`, `+0x6F = cam.z-pos.z`, then
`word[+0x6B] = FUN_000CD514(cam-pos dx, dz)` — the same metric family as
`0x8DC68`. `FUN_0007997C` (`disassemble_bytes 0x799C0`, 96 B) is the record
reset that calls it and sets `+0x77 = word[+0x6B]` (`0x79A19`) after zeroing
`+0x75/+0x83/+0x85/+0x81/+0x87/+0x89/+0x91`.

`FUN_0008C33C` (`disassemble_bytes 0x8C33C`, 64 B) loops all 11 records
(`ECX=0..0xA`, stride 0xB2) calling `FUN_0007997C`, then
`FUN_0008DDE0(team,-1)` -> `[team+0x7B2]` (ranked lane pick), `[team+0x7B6]=0`.
Callers: `FUN_0007417C` x2 + `0x742C9/0x742D2`.

### 3.4 `+0x8D` active seed

`disassemble_bytes 0x8C2E0` (96 B), `FUN_0008C2E0` (callers `FUN_00073D90` x2):

```
0x8C2F7  byte[team+0x826] = DL                    ; side
0x8C31C  [team+0x7A6] = 0x1588A4 + side*0x835     ; other team
0x8C322  XOR EDX,EDX
0x8C324  EAX = team + 0xB2                        ; record 1 .. record 11
0x8C329  byte[EAX-0x25] = DL                      ; = [rec+0x8D] = i
0x8C32C  INC EDX
0x8C32D  dword[EAX-0xB2] = EBX                    ; [rec+0] = team
0x8C333  CMP EDX,0xB; JL 0x8C324
```

So `[rec+0x8D]` = the record ordinal **0..10** (0 = the first record). The
engine's `active` must be seeded with the record index; today it is never set
(`grep` shows only reads/staging). With `active==0` the row-00 window installs
0x19 (keeper hold) instead of 3 — this is exactly T1 leg 3.

### 3.5 Per-frame driver and the AI side

`disassemble_bytes 0x8DB00` (96 B), `FUN_0008D8EC` record loop:

```
0x8DB2E  EAX=EBP (team); EBX=1; CALL 0x782D0          ; record 0 machine
0x8DB3A  EDX = team+0xB2; EBX=1
0x8DB42  loop: if (byte[EDX+0x9A]==0) { EAX=EDX; CALL 0x7CA54 }  ; records 1..10
0x8DB52  EBX++; EDX+=0xB2; if (EBX < 0xB) loop
```

Callers: `FUN_0004B100 0x4B2AB` and `0x4B2B2` (twice per tick, EBX/EDX as the
two team bases) and `0x74429/0x74430`.

Record-machine tail (`disassemble_bytes 0x7CD20`, 64 B):

```
0x7CD29  EAX=EBP; CALL [EBP+0x18]          ; action handler
0x7CD30  CALL 0x6E8E8
0x7CD35  if (EBP == [0x157AA7]) CALL 0x79B1C
0x7CD48  CALL 0x7BF20                      ; the shared mover, every record
```

Driver pre-pass (`disassemble_bytes 0x8DA40`, 112 B): picks
`[team+0x7BA]` via `FUN_0008DE8C` from `0x10F37C + idx*4`, slot/`[0x157A83]`
handling (`FUN_0008D824`), then for that tracked record
`EAX=rec+0x59; EDX=rec+0x4D; EBX=&[ESP+0xC]; CALL 0x795B4` (`0x8DA8F`) —
the tracked record's lane is refreshed before the machines; the BF20 block
(§3.3) refreshes every record's lane after its move.

`FUN_0008E244` census: 3 callers — `0x8E5B4` (wrapper `0x8E5A4`, direct
callers 0 -> pointer invoked), `0x8E7F3` in `FUN_0008E748`,
`0x8E8F2` in `FUN_0008E810`. The two latter are formation writers:
`FUN_0008E810` is called only from `0x3C848`, `FUN_0008E748` only from
`0x3D2DC`; both `0x3C848`/`0x3D2DC` are called from `FUN_00038004` and
`FUN_00038630` (team-management code, which also emits `0x8A938` at
`0x38B2E/0x38C95`). Their target write (`disassemble_bytes 0x8E770`/`0x8E860`):

```
0x8E7A1  EAX = (int8)[ESP-3] * 0x16 + (rec[+0x8A]>>24)*2
0x8E7B1  ECX = (int8)byte[0x1109FC + EAX]; INC EAX
0x8E7C4  [rec+0x4D] = ECX (then <<4); [rec+0x55] = (int8)byte[0x1109FC+EAX] (then <<5)
0x8E7E6  EAX=EDX; CALL 0x8E008; ... (E244)
0x8E8B7  + word[0x110A6A + ECX + recidx] added to [rec+0x55]  ; E810 only
0x8E86F note: byte[0x1109F8] gates (both functions), `+0x7B = 0xF`
```

**FU-77 §1.1 correction.** These E244 passes are *formation-position
application* routines invoked from team-management code (roster/formation
setup), not a per-frame "AI team-positioning" replacement of the action tail.
The per-frame AI target source is the record machines themselves: every
record's handler writes its target (code 3 placement via
`FUN_0008CEB8`/the team passes, code 4 chase, code 6 pursuit, ...) and
`FUN_0007BF20` integrates it (`0x7CD48`; record 0's machine is
`FUN_000782D0` called at `0x8DB35`, its mover tail `0x785C5` per FU-77). The
signed-byte tables are `0x1109FC + k` (x/z per formation row/index) and
`0x110A6A + ...` (z offset), not 0x109F9/0x10A67 as rendered in the le.bin
docs (those are the raw immediates; flat = +0x100000).

### 3.6 Situation 0xB — the possession invoker

`decompile_function 0x8A938` (defined; signature `(uint id, uint side, short
ebx)`): for `id==0xB` the `EBX==0` branch routes
`case 0xB: FUN_000740A0(2, side)`. `FUN_000740A0` (`decompile` +
FU-143 §1.2) sets `[0x157A4D] = phase` (2), runs the per-team `FUN_0008D098`
loop, and on phase 2 sets `[0x15781D]=0`, `[0x157AB2]=1`,
`[0x157A73]=0x15774C`, calling the `0x4C380` RET thunk. So **situation 0xB =
"enter phase 2" (restart/kickoff transition)**.

Fresh xref total = **39**. The `EBX=0`/`EAX=0xB` producers found (byte
patterns `b80b000000` at `0x75461/0x75B46/0x76063/0x7DF83/0x84488/0x84E82/
0x85D2B/0x863EC`, all followed by `XOR EBX,EBX`/`AND EDX,0xFF` and a
`CALL 0x8A938`):

| site | enclosing body | role | verified |
|---|---|---|---|
| `0x7546E` | row 1D `0x74EB0` stage-3 tail (window `0x753E2..0x7547B`) | keeper clear/throw | byte-exact this slice |
| `0x75B58` | row 1E `0x7550C` stage-3 tail (window `0x75A40..0x75B6F`) | keeper claim/place → carrier | byte-exact this slice |
| `0x76072` | row 1E stage-7 tail (window `0x76030..0x7608F`) | restart hand-off | byte-exact this slice |
| `0x7DF90` | row 01 `0x7DBC0` phase-1 tail | kickoff completion | byte window `0x7DF70` (this slice) |
| `0x85D38` | row 0x10 `0x855F0` | restart row | byte window `0x85D20` (this slice) |
| `0x863F9` | row 0x11 `0x85DE4` | restart row | FU-143 + pattern |
| `0x84495` | row 0x12 `0x83D68` | restart row | FU-143 + pattern |
| `0x84E8F` | row 0x13 `0x84B00` | restart row | FU-143 + pattern |

Other `MOV EAX,0xB` sites (`0x82349/0x82391/0x8970C/0x8971F/0x8BAE0/
0x8F06E/0x8F6FD/0x8F71F`, ...) are not among the 39 callers.

**The three in-scope producers, first-hand.**

Row-1E stage table `0x754E4` (10 dwords, `read_memory`):
`{0x75611, 0x7565A, 0x75770, 0x75795, 0x75B90, 0x75D17, 0x75DF3, 0x75E82,
0x7609D, 0x760C6}` -> `0x75B58` lies in the stage-3 span
(`0x75795..0x75B8F`), `0x76072` in the stage-7 span (`0x75E82..0x7609C`).
Row-1E claim/body head (`disassemble_bytes 0x755C0`, 96 B):

```
0x755C0  byte[EAX+0x9B] = 1                ; possession flag
0x755C7  CALL 0x700F4                      ; camera reset
0x755CF  [0x157A83] = rec                  ; controlled actor
0x755DA  word[rec+0x7B] = 3; +0x89 += delta; stage = [rec+0x92]
0x75609  JMP CS:[EAX*4+0x754E4]            ; 10-stage machine
```

Row-1E stage-3 tail (`disassemble_bytes 0x75A40`):

```
0x75A4E  EDX=(int8)byte[0x10F334+type]<<4  ; via dword[EAX+0x10F331]>>24
0x75A54  EAX=(int8)byte[0x10F33C+type]<<4  ; via dword[EAX+0x10F339]>>24
0x75A6A  [0x15774C] = pos.x + off_x; [0x157754] = pos.z + off_z
0x75A79  if (byte[rec+0x9B] != 0) CALL 0x74CDC
0x75A87  EAX = word[0x157C42]; if (<= 0xF0) goto 0x75B26
0x75B10  [0x15774C] = EDX; EAX<<=6; [0x157754] = pos.z + EAX
0x75B29  if (byte[rec+0x9B] != 0) goto 0x760DF     ; already has ball -> other arm
0x75B36  EDX=[EBP-0xC]; CALL 0x7DAB4               ; RESET the record
0x75B3E  EDX=[EDX]; DL=byte[EDX+0x826]; AND EDX,0xFF
0x75B46  EAX = 0xB; 0x75B51 XOR EBX,EBX; 0x75B53 ECX=1
0x75B58  CALL 0x8A938                              ; situation 0xB (phase -> 2)
0x75B5D  EDX = 5; EAX=[EBP-0xC]; 0x75B65 XOR EBX,EBX
0x75B67  CALL 0x7D9A4                              ; install code 5 on the record
```

(Native install argument ECX at `0x75B67` is not statically provable: the
`MOV ECX,1` precedes `CALL 0x8A938`; leg 5.)

Row-1E stage-7 tail (`disassemble_bytes 0x76030`):

```
0x76030  CALL 0x7A490                       ; ball staging (FU-73)
0x76038  if (slot) { CALL 0x8DE8C(0x157A77, team, 0); CALL 0x786A0(rec,nearest) }
0x76058  EDX=[rec]; DL=byte[EDX+0x826]; AND EDX,0xFF
0x76063  EAX=0xB; 0x7606E XOR EBX,EBX; 0x76070 XOR CL,CL
0x76072  CALL 0x8A938                       ; situation 0xB
0x76077  byte[0x157AB2] = 0
0x7607D  CALL 0x4C380
0x76082  stage = byte[rec+0x92]; dword[rec+0x89] = 0
```

Row-1D stage-3 tail (`disassemble_bytes 0x753E0`, `0x75420`):

```
0x7542E  CALL 0x8F188(0x22, rec, 4)
0x75433  if (slot) { CALL 0x8DE8C(0x157A77, team, 0); CALL 0x786A0(rec,nearest) }
0x75456  EDX=[rec]; DL=byte[EDX+0x826]; AND EDX,0xFF
0x75461  EAX=0xB; 0x7546C XOR EBX,EBX
0x7546E  CALL 0x8A938                       ; situation 0xB
0x75473  byte[0x157AB2] = 0; CALL 0x4C380
```

Row-01 producer (`disassemble_bytes 0x7DF70`, 72 B): `0x7DF7A` side,
`0x7DF83 EAX=0xB`, `0x7DF8E XOR EBX,EBX`, `0x7DF90 CALL 0x8A938`,
`0x7DF95 CALL 0x4C380`, stage++. Row-0x10 (`disassemble_bytes 0x85D20`,
48 B) is the same shape at `0x85D2B/0x85D36/0x85D38`.

## 4. Derived semantics

### 4.1 Pad -> target -> velocity (controlled record)

1. Each granted frame `fifa96_control_slot_update` (native `FUN_00078950`)
   maps the pad through `0x11064E` and `0x110E1DC -> 0x110E1EC/0x110E1F5`,
   writing slot `+0x1F`/`+0x20`/`+0x21` (byte-exact window `0x78A11..0x78A52`).
2. Dispatch reads the slot: row 00 calls `FUN_00079C20(rec, T2, T3)` when the
   slot is bound and phase != 6 -> `target = pos + sign8(dir)<<7`, y=0,
   clamp `x +-0x720`, `z +-0xB10` (`0x7DB3D..0x7DB5F`, `0x79C20`).
3. Row 04 calls the same `FUN_00079C20` (`0x7EC0E`) but only on the `ecx==0`
   path. `ecx` (`0x7E9B4..0x7EA16`): 1 when there is no slot, or when
   (slot word `+0x10` bit 0x20 clear or track `0x1577F0 <= 0x70`) and the
   record is the controlled one (`[0x157A83]==0 || ==rec`) with
   **lane = word[+0x6B] < 0x60** and no high-ball track (`AX==0`); otherwise
   `ecx=0`. `ecx==1` takes the camera-follow target (camera triple +
   `word[0x1577C0/0x1577C2]<<2`, `0x7EB95..0x7EBC5`) or the `0x157770` /
   `0x157788` / `0x157794` vectors; `ecx==0` takes the pad step. Because
   lane is distance-to-camera-focus, a controlled record within `0x60` of
   the focus (with `AX==0`) follows the camera and beyond it is pad-steered;
   the high-ball case (`AX=1` = the record is `[0x1577CA]` and ball height
   `>0x50`) forces `ecx=0` and therefore the pad step.
4. The mover `FUN_0007BF20` then integrates the written target into
   `+0x73/+0x75` and `+0x59/+0x61` (FU-77 blocks A-E; unchanged), and at its
   tail (§3.3) recomputes `lane`, `bound (+0x77 := old lane)`,
   `+0x6D/+0x6F` and the team `+0x7C7` nearest tracker. So the pad path has
   one frame of latency only in the mover feedback (lane/bound read by the
   next frame's row-04 gates); target->velocity->position is same-frame.

### 4.2 Possession flip triggers

There are two independent possession tracks (FU-78) plus the restart
transition:

* **Per-record `+0x9B` ball flag**: set on a keeper claim
  (`0x755C0`, and `0x74567`/`0x76DAE` per FU-78), cleared at
  `0x71D2D/0x79A6C/0x89903`. Row-1E stage 3 only runs the reset+0xB+code-5
  arm while `+0x9B == 0` (`0x75B29`).
* **Event carrier block `0x58724`**: claimed by code 5 (`0x7F1C7`/`0x7F1FF`),
  i.e. the record installed as code 5 — by the keeper arm `0x75B67`, by the
  row-1E claim (row 1E never writes the block itself), or by the code-4
  duel split `0x7F133` (FU-78 §8) — becomes the carrier on its next dispatch.
* **Restart transition**: situation 0xB -> `FUN_000740A0(2, side)` ->
  phase := 2, `[0x15781D]=0`, `[0x157AB2]=1`, `[0x157A73]=&0x15774C`, and the
  per-team `FUN_0008D098` phase arm (which installs codes). The producers
  clear `[0x157AB2]` right after (`0x75475`, `0x76077`). The row-1D/1E tails
  also move the human control slot to the nearest teammate
  (`FUN_0008DE8C(0x157A77,...)` + `FUN_000786A0`) — this is the "hand-off"
  side of a possession change.

## 5. Port contract (engine names)

| native | engine target |
|---|---|
| BF20 lane block `0x7C776..0x7C7AF` + helper `0x795B4` | new `fifa96_action_locomotion_track(pos_x, pos_z, cam_x, cam_z, &lane, &cam_dx, &cam_dz)` in `fifa96_action_handlers.h/.c`; metric `fifa96_entity_distance` (= `0x8DC68`/`0xCD514` family); caller sets `bound = old_lane` and `team.camera_nearest` (`0x7C7C4`/`0x7C7CD`) |
| record fields `+0x6B/+0x6D/+0x6F/+0x77` | pool `fifa96_match_entity`: `lane_x`/`lane_z` exist; add `bound` (+0x77), `cam_dz6f` (+0x6F); `fifa96_match_run.c` writes them after the mover |
| driver loop `0x8DB2E..0x8DB5F` | `match_run_dispatch_entity`: run the mover (+ track) for **every** dispatched record (keep the `+0x9A` skip), not only `mr->slot.entity == id` |
| `FUN_0008C2E0 0x8C329` (`+0x8D = index`) | `fifa96_match_entities` setup/reset seeds `record.active = index`; the engine's `fifa96_match_entities_install`'s `active==0 && code==3 -> 0x19` then matches row 00's `0x7DB84` window |
| row-04 staging | `fifa96_match_action_04` (`fifa96_match_handlers.c:1208-1211`): replace `s.bound = 0`, `s.word6f = 0` with the pool fields; keep `s.lane = e->lane_x`; the pad arm (`s.lane >= 0x60` or `AX`) becomes reachable with no change to `fifa96_outfield_row04_step` |
| situation 0xB | `fifa96_match_run_situation_0b(mr, side)`: `fifa96_match_state_set_phase(mr, 2)` + phase-2 fields (`0x740F3..0x74107`); keeper arm = `match_row_reset(rec)` + `fifa96_match_entities_install(e, 5, invoke)`; restart tail = nearest teammate + slot move (`fifa96_match_entities_bind_slot` nearest search + slot transfer) |
| carrier flip | already ported: `fifa96_action_possession_claim` (`0x7F1C7`), `fifa96_ball_pair_possess/release` (`+0x9B`); wire the install -> next-dispatch -> claim in the fixture |
| input | unchanged: `fifa96_control_slot_update` + `match_run_slot_map`/`match_run_anim_a/b/c` |

**API reconciliation (freeze pass):** `fifa96_match_run_situation_0b`
overlaps the shared situation-0xB entry resolved in FU-146 §7.1 — keep a
single 0xB entry (`fifa96_match_run_situation` stays the table-2/0xB path) and
do not add a parallel `_0b` mechanism.

Minimal seam files: `include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c` (track), 
`include/fifa96_engine/fifa96_match_entities.h` + `.c` (fields + active seed),
`src/fifa96_engine/fifa96_match_run.c` (per-record mover + track + 0xB seam),
`src/fifa96_engine/fifa96_match_handlers.c` (row-04 staging), tests.

## 6. Numbered legs

1. Row-1E stage-3 entry `0x75795` -> `0x75A40` control flow not read
   end-to-end (the arm is quoted mid-span; the table bounds the stage only).
2. The `0x760DF` arm (reached when `+0x9B != 0` and from the stage-switch
   `JA`) is not derived.
3. Row-1E stage-7 (0x76072) entry gate from `0x75E82` not read.
4. Row-1D (0x7546E) stage-3 gate chain into `0x753E2` not re-derived
   (FU-143's "close-down stage-3 tail" + the byte window here).
5. Install invoke flag at `0x75B67`: `ECX` is set before `CALL 0x8A938` and
   not provably preserved (decompiler shows `extraout_ECX`); invoke-now is
   inferred, not proven.
6. `FUN_0007997C`/`FUN_0008C33C` reset-lane path (`+0x77` seed, `+0x6B`
   refresh at team reset) not fully decomposed.
7. BF20 lane-block tail `0x7C7D3..0x7C901` (visibility/relocation arm,
   `[0x157820]/[0x157822]/[0x1577BC]/[0x157750]`) not decomposed.
8. `[0x157AB2]` semantics unnamed (set at phase-2 entry `0x74101`, cleared at
   `0x75475`/`0x76077`; also touched by row 1D per FU-140).
9. Driver pre-pass `0x8DA43..0x8DA90`: `0x10F37C` tracked-point table and the
   `[team+0x7BA]` selection not decomposed; `FUN_0008D824` unread.
10. `FUN_000CD514` identity/parameter order (the `0x795B4` metric) not
    derived beyond "distance of two words".
11. Situation-0xB producers `0x863F9`/`0x84495` (rows 0x11/0x12) verified by
    byte pattern + FU-143 only, not by a fresh window.
12. E244 formation writers' roster context (`0x3C848`/`0x3D2DC` called from
    `FUN_00038004`/`FUN_00038630`) not decomposed.
13. `0x15774C/50/54` is the match camera-focus point written by keeper/
    restart arms (`0x75A6A`, `0x75B10`); its relation to the engine's
    `render.camera.pos_*` stand-in is asserted, not proven.

## 7. Risks

* **Tape movement.** Seeding `active`, producing `lane`/`bound`, and running
  the mover for all records are all behavior changes; M2 re-pin needs the
  written reason + frame diff (M1 must stay byte-identical).
* **Lane source.** The native lane uses the match camera-focus point
  `0x15774C/54`; if the engine's staged camera diverges, row-04's `lane<0x60`
  and `lane vs bound` gates will diverge in exact frames. Keep the
  stand-in documented until leg 13 lands.
* **Sign/width.** Lane comparisons are signed 16-bit (`JGE/JG` on words)
  while the metric is an unsigned 16-bit result; `fifa96_outfield_row04_state`
  already takes `int16_t lane`/`bound` — preserve the casts.
* **BF20-only block.** The lane block is absent from the E244 twin; the
  formation (E244) path must not run track or it would overwrite lanes the
  BF20 path owns.
* **Two writers.** `FUN_000795B4` also refreshes lane at team reset and for
  the driver's tracked record; the port's per-frame track models the BF20
  instance. The reset instance only matters for the first frame after a
  reset (leg 6).
* **`+0x8D` name.** "active" is the record ordinal; using it as a boolean
  is correct (`!= 0`) but code that writes it in the engine must write the
  index, not 1.

## 8. Port landing (S1, 2026-10-09)

Frozen slice ported in phase-6 wave-2 S1. Landed, with first-hand re-verification
this slice (Ghidra MCP `/FIFA96.EXE`, read-only):

| contract item | engine landing |
|---|---|
| BF20 lane block | `fifa96_action_locomotion_track` (`fifa96_action_handlers.h/.c`) — cam deltas at `0x7C782..0x7C79A`, metric at `0x7C7AA` |
| pool fields | `fifa96_match_entity.bound` (+0x77), `.cam_dz6f` (+0x6F); `fifa96_match_team.camera_nearest` (+0x7C7, team-relative index) |
| driver loop | `match_run_dispatch_entity`: the shared mover now runs for **every** dispatched record (pool walk keeps the `+0x9A` skip), then the track writes lane/deltas/bound and the team tracker |
| `+0x8D` seed | `fifa96_match_entities_init` seeds `record.active = index` (`FUN_0008C2E0` `0x8C329`, re-verified byte-exact) |
| row-04 staging | `s.bound = e->bound`, `s.word6f = e->cam_dz6f`, `s.is_team_7c7` from `camera_nearest` |
| situation 0xB producers | row-1E stage-3 tail: `match_row_reset` -> `fifa96_match_run_situation(mr, 0x0B)` (the shared row-01 table-2 entry; no `_0b` mechanism per the freeze ruling) -> code-5 install request; `+0x9B` (`has_ball`) and `[0x157A83]` (`controlled`) write-back from the row-1E claim to the pool |
| input | unchanged (`fifa96_control_slot_update` + the slot map/anim tables) |

**Errata / corrections (first-hand this slice).**

1. **§3.3 metric conflation.** The BF20 lane block calls **`0x8DC68`**
   (`0x7C7AA` bytes `e8 b9 14 01 00` = CALL 0x8DC68), i.e.
   `fifa96_entity_distance`; the `0x795B4` helper's out[0] is the
   **`0xCD514`** divide (FU-142 Appendix K.2). The §3.3 sentence "the same
   metric family as 0x8DC68" merges two different functions; the port uses
   `fifa96_entity_distance` (the block's own call) and leg 10 stays open for
   the helper path.
2. **Tracker compare width.** `0x7C7C4 CMP DX, word[ESI+0x6B]` / `0x7C7C8
   JGE` (first-hand) is a signed 16-bit compare; a NONE tracker (native NULL)
   or a strictly smaller fresh lane replaces it. The pool stores the
   team-relative index, not the native pointer.
3. **Reset forced-decision consequence of the seed.** `FUN_0007DAB4`
   (re-verified) calls `0x7C990` for `phase == 2 && byte[+0x8D] != 0` and
   **returns** (`0x7DAF6..0x7DAFA`); the code-0 re-install `0x7DAFB` runs only
   on the skip path (phase != 2 or `+0x8D == 0`). With the active seed the
   engine's `match_row_reset` therefore installs a decision code on active
   records at phase 2; the pre-S1 tape's "row 00 dispatches through a reset"
   was an unseeded-active artifact (all records read inactive). The M2 tape
   now stages code 0 explicitly; row 00's natural code-3 runs are unchanged.
4. **Row-01 deferred-helper ordering.** `0x7DC2A CALL 0x7876C` no-ops when
   the requester already holds the slot; the engine defers `helper_request`
   to the frame drain, which runs after the same dispatch's `0x7DF61`
   stage-2 merge (`0x7DEFA`). The port gates the request on `has_slot == 0`
   at the call site so the deferred drain cannot invert the native order.

**Legs status after S1.** Legs 1–5 and 9/10/12 remain open (stage-flow entry
gates, the `0x760DF` arm, the install `ECX`/invoke flag, the reset-lane path,
the driver pre-pass / `FUN_0008D824`, the `0xCD514` helper identity, the E244
roster context); leg 6 unchanged; leg 7 (BF20 visibility/relocation arm) still
not decomposed; leg 8 (`[0x157AB2]`) still unmodeled; leg 11 unchanged; **leg
13 is live** — the track uses the engine render camera as the
`0x15774C/0x157754` match-focus stand-in, so lane values are stand-in-derived
until it lands. New note: the pool `lane` dword (+0x69) keeps the FU-142b
"dz word, sign-extended" model while the S1 track writes `lane_x` (+0x6B);
row-04 reads `lane_x`, but rows 01/26/28/2A read `r->lane >> 16` — a
pre-existing alias divergence now more visible (row 04 staging is the contract
scope).

## 9. Provenance

Ghidra MCP on `/FIFA96.EXE`, read-only:
`get_current_program_info`; `get_function_by_address` 0x8C2E0/0x740A0/0x4C380;
`decompile_function` 0x8A938/0x740A0 (by address); `get_xrefs_to` 0x8A938
(39), 0x8E244 (3), 0x8E5A4 (0), 0x8E748 (1), 0x8E810 (1), 0x7CA54 (1),
0x782D0 (1), 0x8D8EC (4), 0x795B4 (12), 0x7997C (1), 0x8C2E0 (2), 0x8C33C
(4), 0x3C848 (2), 0x3D2DC (2); `disassemble_bytes` 0x7E990/0x7EB80/0x7DB10/
0x79C20/0x78A10/0x7C760/0x7C7D0/0x795B4/0x799C0/0x8C33C/0x8C2E0/0x8DB00/
0x8DA40/0x7CD20/0x75A40/0x755C0/0x753E0/0x75420/0x76030/0x7DF70/0x85D20/
0x8E5A4/0x8E770/0x8E860/0x8DA40/0x4B290; `read_memory` 0x1106E0 (128 B),
0x754E4 (48 B), 0x1109F0/0x110A60 (32 B each); `search_instructions` 234,864
insns (MOV-scoped operands `+ 0x6b],` 1, `+ 0x6d],` 1, `+ 0x6f],` 1,
`+ 0x77],` 2, `+ 0x8d],` 0, `+ -0x25],` 1, `+ 0x13f],` 0, `+ 0x8c],`
stack-only, `EAX, 0xb` 155 raw / 8 relevant; full all-mnemonic scans of
`+ 0x8d],`/`+ 0x6b],`/`+ 0x77],` return 30/3/2 sites, extras CMP-readers only).

No write outside this draft; no tool, ISO, capture-rig, or Ghidra-project
change. Engine files were read only.

**S1 re-verification (2026-10-09, read-only).** `disassemble_bytes` on
`/FIFA96.EXE`: `0x7C776` (`0x7C776..0x7C7C5`, the lane block + tracker
compare), `0x7C7C5..0x7C7DC`, `0x8DC68` (the octagonal metric body),
`0x8C2E0`, `0x75B29..0x75B68`, `0x76030..0x7605F`, `0x7F1A6..0x7F1F5`,
`0x8DB20..0x8DB5F`, `0x7CD20..0x7CD4F`, `0x785B0..0x785CF`, `0x8CEE0..0x8CF2F`,
`0x7D9A4..0x7D9E3`, `0x7DA20..0x7DA5F`, `0x8CF30..0x8CF57`, `0x7DAB4..0x7DB0C`,
`0x7C990..0x7C9DF`. No writes.
