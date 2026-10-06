# FU-83: the phase-driver table (flat `0x110794`, 35 slots)

Follow-on to FU-81 (the `0x2D..0x4F` phase-handler table) and FU-82 (event/
sequence family): fully derive the 35-slot phase table read by
`FUN_0006D920`, enumerate every slot with its target function, classify the
targets from their bodies (formation/placement vs timeline/cinematic vs
none), derive the two family models and `FUN_0006D920`'s resolver, and port
the clean dispatch/install model plus family state math as
`fifa96_action_phase_*`.

Result in one line: **the table at flat `0x110794` is indexed by the phase
byte itself (`[0x57A4A]>>24`, range evidenced `0..0x22`; the FU-81 "slot
`0x2D..0x4F`" labels are flat-block offsets = phase+`0x2D`), holds 22
formation/placement targets (`0x6Dxxx`, phases `00..15`), one zero slot
(phase `16`, loader→`0x10000` INT3), and 12 phase-timeline targets
(`0x8xxxx`, phases `17..22`) that all tick the shared timer `[0x58818] +=
[0x57A64]`, stage on `[0x58829]`, drive camera (`FUN_000700F4`), sound
(`0x974DC/0x974F0`) and record/action installs (`0x6E598`, `0x7D9A4`), with
one pure reset (`0x890EC`); `FUN_0006D920` is a per-record resolver (not a
loop): it resolves four per-record pointers from the `0x4C384`/`0x4AFB8`
indexed lookup over `0x4BFC0` and installs `table[phase]` at `[rec+0x1C]`
with no bounds check, and the loops live in its four callers.**

## Method

* Static work on the open Ghidra MCP session for `/fifa96_le.bin` (FU-4 linear
  image, link addresses). `disassemble_function`/`disassemble_bytes` are the
  citation source; jump tables are quoted as raw dwords read with
  `read_memory` (stored value + `0x10000`); the decompiler is not used for any
  quote. Bodies the listing prunes/mis-decodes are recovered with
  `disassemble_bytes` + `read_memory` and marked.
* **Address mapping (FU-76/FU-81, restated).** Code/function addresses equal
  true link addresses; a **data immediate** `A` is flat `A+0x100000` (so the
  phase-table immediate `0x10794` is storage at `0x110794`, `[0x57A4A]` at
  `0x157A4A`); stored code pointers and inline `CS:` tables are object-1
  relative and resolve through `+0x10000` (e.g. `CS:` operand `0x79868` →
  true table `0x89868`).
* Every numeric claim is quoted; unproven items are open legs (no guessed
  labels).

## 1. `FUN_0006D920` (0x6D920..0x6D9C1, 51 insns) — the per-record resolver

### 1.1 Full body

```
0x6D924  EDX = EAX                     ; rec
0x6D926  EAX = [EAX]                   ; team
0x6D928  EAX = [EAX+0x7AE]             ; 29-byte formation row (set by 0x6D9C4)
0x6D92E  BX  = (int8)[EAX]             ; row[0] = formation id
0x6D932  EBX = BX * 6
0x6D935  ESI = [EDX+0x8A]              ; subtype<<24
0x6D93B  CX  = (int8)[EDX+0x8D]        ; "active"/variant flag
0x6D943  SAR ESI,0x18                  ; subtype
0x6D948  ECX = (int16)CX
0x6D94E  ESI <<= 2
0x6D951  CALL 0x4C384 (EAX = 6*id)     ; lookup(6*id)
0x6D956  EAX += ESI                    ; + subtype*4
0x6D95E  [EDX+0x8]  = EAX              ; resolver pointer 1
0x6D961-0x6D969  CALL 0x4C384 (6*id+1)
0x6D96E  EAX += ECX*0x256
0x6D970  [EDX+0xC]  = EAX              ; resolver pointer 2
0x6D973-0x6D98C  CALL 0x4C384 (6*id+2)
0x6D98C  CALL 0x4C384; 0x6D991  EAX += ECX*8
0x6D996  [EDX+0x10] = EAX              ; resolver pointer 3
0x6D993  EBX += 4
0x6D999-0x6D99E  CALL 0x4C384 (6*id+4)
0x6D9A3  EAX += ECX*0x958
0x6D9A5  [EDX+0x14] = EAX              ; resolver pointer 4
0x6D9A8  EAX = [0x57A4A]
0x6D9AD  SAR EAX,0x18                  ; phase byte
0x6D9B0  EAX <<= 2
0x6D9B3  ADD EAX,0x10794               ; flat 0x110794 + phase*4
0x6D9B8  EAX = [EAX]
0x6D9BA  [EDX+0x1C] = EAX              ; installed phase handler
0x6D9BD-0x6D9C1  POPs; RET
```

* **Per-record resolver, not a loop.** One call installs four pointers and
  one handler for a single record; the 11-record loops are in the callers
  (§2). The records use stride `0xB2` (0x8CF95/0x8CF9B, 0x8D131..0x8D152).
* **Bounds behavior:** the phase index has **no bounds check** — any
  `[0x57A4A]>>24` value reads `0x110794 + phase*4`; the table's 35 entries
  cover phase `0x00..0x22` (the next flat dword at `0x110820` is already the
  outfield pressed-row block `{0x07FF,0x60,0x6CE38}`, read_memory `0x110820`).
  So phases `0x23+` read unrelated flat data as a handler pointer.
* **`lookup`:** `0x4C384` = `AND EAX,0xFFFF; JMP 0x4AFB8` (bytes
  `25 ff ff 00 00 e9 2a ec ff ff`) and `0x4AFB8` =
  `MOV EAX,[EAX*4+0x4BFC0]; RET` — a 16-bit-masked indexed pointer table at
  flat `0x14BFC0`. That table's first 64 bytes at Ghidra `0x14BFC0` are zero
  in the static image (read_memory), so it is populated before/at match setup
  (open leg 4).
* The formation-row layout follows from `FUN_0006D9C4` (0x6D9C4, 52 insns):
  `EBX = 0x1033A + (int16)DX*0x1D` (row for formation id DX), stored at
  `[team+0x7AE]`; then four groups at `row+1+7*g`: `[base]` = sub-id written
  to `rec+0x90`, `[base+1]` = record count, `[base+2..6]` = the group's
  record slot indices; each record gets `CALL 0x6D920` (0x6DA0A). After the
  groups it installs two more lookups (`6*id+3` → `[team+0x7DB]`,
  `6*id+5` → `[team+0x7DF]`; 0x6DA25..0x6DA56). The `+3`/`+5` indices are the
  gap left by D920's `6*id`, `+1`, `+2`, `+4` set.

### 1.2 Callers (4 static xrefs; `get_function_xrefs 0x6D920`)

| caller | site | behavior (quoted) |
|---|---|---|
| `FUN_0006D9C4` | `0x6DA0A` | team setup: writes `rec+0x90`, calls the resolver for each record of each of the 4 formation groups |
| `FUN_0007997C` | `0x79983` | placement/teleport entry: if incoming `EDX != 0` copies that triple to `rec+0x59`, else `CALL [rec+0x1C]` with `EAX=rec, EDX=&rec+0x4D, EBX=-1` and copies `rec+0x4D` to `rec+0x59` (0x79983..0x799B1) |
| `FUN_0008CF60` | `0x8CF73` | loop `EDI=0..0xA`, `ESI/ECX` stride `0xB2`: resolver → `CALL [ECX+0x1C]` (`EDX=ESI=&rec+0x4D`) → `FUN_00079F3C` (camera placement, `EBX=0x5774C`) → `FUN_00079B6C` (0x8CF60..0x8CFAB) |
| `FUN_0008D098` | `0x8D107` | per-frame loop, same stride, but only while `[rec+0x9A] == 0` (`0x8D0F7 CMP byte [ECX+0x9A],0; 0x8D0FE JNZ skip`); after `CALL [ECX+0x1C]` it calls `FUN_0008DCD4(dest=&rec+0x59, src=&rec+0x4D, aux=&rec+0x65)` (0x8D100..0x8D11E) |

`FUN_0008D098` then dispatches on **match phase** `[0x57A4D]` (`MOV AL,
[0x57A4D]; CMP AL,0x15; JA default; JMP CS:[EAX*4+0x7D040]`, 0x8D178..
0x8D18A; flat table `0x8D040`, 22 entries) and runs each phase's team logic
(camera/ball staging `0x700F4`, action installs `0x7D9A4`, etc.). It also
calls `FUN_0006D9C4` first (0x8D0A6..0x8D0B1).

Two engine entry points wrap the install loops:

* `FUN_00073E08` (`0x73E08`): `FUN_0008C24C(0x58830)`, then
  `FUN_0008CF60(0x588A4)` and `FUN_0008CF60(0x590D9)` — installs phase
  handlers for all 11 records of both teams (`0x588A4` team base, `0xB2`
  stride, 11 records).
* `FUN_000740A0` (`0x740A0`): `[0x57A4E] = [0x57A4D]; [0x57A4D] = AL` (new
  match phase), `[0x57AAF] = DL` (side), then `FUN_0008D098(0x588A4 +
  EDX*0x835)` and `FUN_0008D098` for `DL^1`; if new phase == 2 also clears
  `[0x5781D]`, sets `[0x57AB2]=1`, `[0x57A73]=0x5774C` (0x740A0..0x7410D).

So the runtime chain is: `phase entry (0x740A0) → per-team FUN_0008D098 →
per-record resolver + handler (+ movement vector 0x8DCD4)`, with the
one-shot `0x73E08 → FUN_0008CF60` install used at setup/restart.

## 2. The 35-slot table (flat `0x110794`, `read_memory` 140 B)

Phase `p` → `stored = [0x110794 + p*4]`, runtime = stored + `0x10000`
(code pointer). Flat-block slot = `p + 0x2D`.

| phase | slot | stored | runtime | family | body model (cited) |
|---|---|---|---|---|---|
| 00 | 2D | `05DE34` | `06DE34` | placement | out = `rec+0x59` triple (`0x6DE34..0x6DE40` MOVSD×3) |
| 01 | 2E | `05E1D0` | `06E1D0` | placement | resolver ptr 1 (`[rec+8]`, +2 when side==`[0x57AAC]>>24`): `out.x = ±b0*0x26`, `out.z = ±b1*0x21`, `out.y=0` (`0x6E1D0..0x6E241`) |
| 02 | 2F | `05DCC8` | `06DCC8` | placement | resolver ptr 2 (`[rec+0xC]`); `BX<0` → `word[0x577DE+side*4]>>16`, else `BX`; same cell math (`0x6DCC8..0x6DD3B`) |
| 03 | 30 | `05DE44` | `06DE44` | placement | formation slot from ptr 4 (`[rec+0x14]`) + `[0x577D6/0x577D8+side*4]`; `out.x=±(v+v/4)`, `out.z=±(w+w/4)`, `out.y=0` (`0x6DE44..0x6DF4A`) |
| 04 | 31 | `05DE44` | `06DE44` | placement | same |
| 05 | 32 | `05E05C` | `06E05C` | placement | if `[rec]` == ball record (`[0x57A9F]`) → own-copy arm; else distance via `0x8DC68`, threshold `0x5A0` normally / `0x12C0` when score diff `[0x5881A]-[0x57AB6] >= 0x3C` and `[0x57AC2] >= 3` (`0x6E0AF..0x6E0DF`); `distance >= threshold` or same-record → own `+0x59` copy (`0x6E1B3..0x6E1C5`), else ball-record `+0x59` with `0x14E04` fold (`0x6E147..0x6E1A8`) |
| 06 | 33 | `05DD9C` | `06DD9C` | placement | ptr 3 (`[rec+0x10]`); entry 0/1 by `sign([0x57754])` (ball z) and side; `±(v+v/4)`; `out.z ±= 0x60` by ball z; `out.y=0` (`0x6DD9C..0x6DE31`) |
| 07 | 34 | `05DE44` | `06DE44` | placement | same as 03 |
| 08 | 35 | `05DD6C` | `06DD6C` | placement | wrapper: `EBX = (side==[0x57AAC]>>24) ? 0xBC : 0x6F; CALL 0x6DCC8` (`0x6DD6C..0x6DD98`) |
| 09 | 36 | `05DD6C` | `06DD6C` | placement | same |
| 0A | 37 | `05DE34` | `06DE34` | placement | same as 00 |
| 0B | 38 | `05DE34` | `06DE34` | placement | same as 00 |
| 0C | 39 | `05DF4C` | `06DF4C` | placement (timer) | `[rec+0x89] += [0x57A64]`; if `>=0x3C`: `out.x=0x780`, `out.z=±5*([rec+0x8A]>>25)` (negated when `([rec+0x8D]&1)==0`), `[rec+0x7B] = ([0x57A38+idx])>>1`; `|lateral|<0x20` → `out.x=0xCC0,out.z=0`; then `CALL 0x8DCD4` (`0x6DF55..0x6DFF7`) |
| 0D | 3A | `05DE34` | `06DE34` | placement | same as 00 |
| 0E | 3B | `05DE34` | `06DE34` | placement | same as 00 |
| 0F | 3C | `05DE34` | `06DE34` | placement | same as 00 |
| 10 | 3D | `05E004` | `06E004` | placement | `[rec+0x8D]!=0`: `out.x=(int8)([0x105E7+(b-1)*2]>>24)<<5`, `out.z=(int8)([0x105E8+(b-1)*2]>>24)<<5`, z negated for side 1; else `(0x3C0,0,0x840)` (`0x6E004..0x6E05A`; tail recovered) |
| 11 | 3E | `05E1C8` | `06E1C8` | placement | `EBX=-1; EDX=&rec+0x4D`; falls into `06E1D0` (`0x6E1C8..0x6E1CF`) |
| 12 | 3F | `05E1D0` | `06E1D0` | placement | same as 01 |
| 13 | 40 | `05E244` | `06E244` | placement | side must equal `[0x57AAC]>>24` else return with out untouched (`0x6E261 JNZ 0x6E326`); `[rec+0x8D]==0` → `out.x=±[0xF364]`, `out.z=±[0xF368]`; else both axes get `±rand%0x90` scatter (RNG `0x92AC8`, divisor `0x90`; `0x6E267..0x6E32A`) |
| 14 | 41 | `05E244` | `06E244` | placement | same |
| 15 | 42 | `05DCC8` | `06DCC8` | placement | same as 02 |
| 16 | 43 | `000000` | `010000` | none | zero entry; loader `+0x10000` → `0x10000` (INT3) |
| 17 | 44 | `078DC8` | `088DC8` | timeline | tick `[0x58818]+=[0x57A64]`; stage override `[0x58829]` 0/1 (`0x88E39..0x88E46`); on period end (`[0x57AC5]` vs `[0x57AC7]`) resets `[0x58818]/[0x58808]/[0x58828]/[0x58829]`; camera reset `FUN_000700F4([0xF328],[0xF32C],[0xF330])`, `0x73E28`, `0x740A0(1)`, ball stage `0x4C324` (`0x88DC8..0x88F4A`) |
| 18 | 45 | `07922C` | `08922C` | timeline | tick; camera/ball lead vector `[0x58830]/[0x58838]` stepped `±0x60`, `CALL 0x7D388` (`0x8922C..`) |
| 19 | 46 | `079FA4` | `089FA4` | timeline | `[0x57AA3]=[0x5888F]`, `[0x57A73]=0x587C0`, `CALL 0x36200`; tick; stage 0..6, `CS:[EAX*4+0x79F88]` → flat `0x89F88`; stage 0 builds `0x587C0..0x587C8` line, sound `0x974DC(0x1E)`/`0x974F0(0xC)`, `0x740A0(0xF)`, installs action `0x16` on `[0x5888F]` via `0x7D9A4` (`0x89FA4..0x8A0A4`) |
| 1A | 47 | `079620` | `089620` | timeline | tick; stage `[0x58829]` 0/1; `[0x57B8E]` counter, table `0x57BD2` vs `0x4C360`, record addressing `0x741B4`/`0x835`/`0xB2` (`0x89620..`) |
| 1B | 48 | `0790EC` | `0890EC` | timeline (reset) | zeroes `[0x58818]` (dword!), `[0x58828]`, `[0x58829]`, `[0x58808]`; RET (`0x890EC..0x8910D`) |
| 1C | 49 | `079110` | `089110` | timeline | tick; stage 0..2 inline; stage 0: `0x4C374(0x15)`, `0x740A0(0xA)`, `0x974DC(0x1E)`, `0x974F0(0x190)`, stage++ (`0x89110..0x891CF`) |
| 1D | 4A | `079868` | `089868` | timeline | tick; stage 0..9; reads `[0x57A4A]==0x10` (`0x898CA`); installs action `0x19` (`0x8990A`) and loops records stride `0xB2` (`+0x14C/+0x14D`), installing action `3` and calling `0x6E598` (`0x89868..`) |
| 1E | 4B | `07A798` | `08A798` | timeline | stage 0..2 (no timer tick in head): stage 0 sets `[0x5882E/0x5882F]=0`, stage++; stages wait on `0x4BEC8`; record address `[0x5882B]>>24 * 0x835 + [0x5882C]>>24 * 0xB2 + 0x588A4`; `[0x587F0]=0` (`0x8A798..`) |
| 1F | 4C | `078F4C` | `088F4C` | timeline | tick; stage 0..3, `CS:[EAX*4+0x78F3C]` → flat `0x88F3C`; stage 0 camera reset, `0x73E28`, `0x740A0(0x15)`, ball stage `0x5774C`, `[0x5B680]` mode checks (`0x88F4C..0x890EB`) |
| 20 | 4D | `07B688` | `08B688` | timeline | tick; stage 0..2; stage 0 camera fields `[0x5880C]=0x960`, `[0x58814]=0`, `[0x58830]=0x1E0`, `[0x58838]=0`, `[0x5884A]=0`, `[0x58852]=3`, pushes `[0xF328/2C/30]`, `0x73E28` (`0x8B688..0x8B873`) |
| 21 | 4E | `07B874` | `08B874` | timeline | tick; stage 0..1; stage 0 `0x974DC(0x1E)`, stage++; stage 1 waits `[0x58816]>>16 >= 0x3C` → sound, clears `[0x58808]/[0x58828]/[0x58829]` (`0x8B874..`) |
| 22 | 4F | `07B900` | `08B900` | timeline | tick; stage 0..2; stage 0 `0x974DC(0x1E)`, stage++; stage 1 waits `[0x58816]>>16 >= 0x3C` → sound, stage++ (`0x8B900..`) |

Shared timeline fields: `[0x57A64]` (16-bit delta), `[0x58818]` (16-bit timer),
`[0x58829]` (stage byte), `[0x58808]`/`[0x58828]` (state words), `[0x58816]`
(timer), `[0x5880C]/[0x58814]` (limits), `[0x58830]/[0x58838]` (camera/ball
lead vector). Sound calls: `0x974DC`/`0x974F0`; camera: `0x700F4` (FU-71's
`fifa96_camera_init` original) and `0x73E28`/`0x740A0`; record/animation:
`0x6E598` (FU-82 animation selector), action installs `0x7D9A4`.

## 3. Family models

### 3.1 `0x6Dxxx` — formation/placement (phases `00..15`, 22 slots)

Every target writes the caller's out triple `EDX = &rec+0x4D` and reads the
four resolver pointers installed by `FUN_0006D920` (`rec+8/+0xC/+0x10/+0x14`)
and/or the shared tables `0x577D4..0x577DE`, `0x57754` (ball z),
`0x57AAC>>24` (side reference), `0xF364/0xF368` (camera bounds). The clean
sub-models:

1. **Held-position copy** (`06DE34`, phases `00,0A,0B,0D,0E,0F`): three
   `MOVSD` from `rec+0x59` to out (`0x6DE34..0x6DE40`); `06E05C`'s fallback
   branch is the same copy (`0x6E1B3..0x6E1C5`).
2. **Cell placement** (`06E1D0` phases `01,12`, `06DCC8` phases `02,15`):
   read two int8 cell coordinates and scale x by `0x26`, z by `0x21`
   (`SHL 5 + SHL 2 + ADD` = ×38; `SHL 5 + ADD` = ×33), negate both when
   `[team+0x826] != 0`, `out.y = 0`. `06E1D0` uses ptr 1 and adds the `+2`
   side-variant; `06DCC8` uses ptr 2 and, when `EBX<0`, looks up
   `word[0x577DE+side*4]>>16` (`0x6DCD0..0x6DCED`). `06DD6C` wraps `06DCC8`
   with `EBX = 0xBC` or `0x6F` by side equality (`0x6DD79..0x6DD91`).
3. **Formation slot** (`06DE44`, phases `03,04,07`): selects
   `[0x577D6+side*4]>>16*8 + [0x577D8+side*4]` (plus `+4` when
   `[0x57AAF] != side`) into ptr 4, then `out.x = v+v/4`, `out.z = w+w/4`,
   negated for side 1, `out.y = 0` (`0x6DE6C..0x6DEE0`); the `DX<0` arm
   stages the ball triple, `CALL 0x6D870`, and delegates to `06DCC8`
   (`0x6DEE5..0x6DF37`).
4. **Ball line** (`06DD9C`, phase `06`): ptr 3 entry selected by ball-z sign
   and side (`0x6DDAD..0x6DDDD` and `0x6DDE1..0x6DE12`), `v+v/4`, then
   `out.z ±= 0x60` by ball-z sign, `out.y = 0` (`0x6DE15..0x6DE2A`).
5. **Timer line** (`06DF4C`, phase `0C`): `[rec+0x89] += [0x57A64]` is
   consumed only when `>= 0x3C` (`0x6DF55..0x6DF6C`); out x/z from
   `0x780`/`0xCC0` and `±5*([rec+0x8A]>>25)`; side effect `[rec+0x7B] =
   ([0x57A38+idx])>>1`; then `CALL 0x8DCD4` (movement vector).
6. **Variant table** (`06E004`, phase `10`): `[rec+0x8D]` indexes int8 cells
   in `[0x105E7 + (b-1)*2]` / `[0x105E8 + (b-1)*2]` (`>>24 <<5`), z negated
   for side 1; zero → `(0x3C0,0,0x840)`.
7. **Camera-bound scatter** (`06E244`, phases `13,14`): only for
   `side == [0x57AAC]>>24`; `(±[0xF364], ±[0xF368])`, or RNG-scattered
   `±rand%0x90` on both axes when `[rec+0x8D]` is set and `[0x57AC2] < 4`.
8. **Distance line** (`06E05C`, phase `05`): same-record-as-ball test
   (`0x6E10B`), distance `0x8DC68` vs `0x5A0`/`0x12C0`, then ball-position
   copy with the `0x14E04` fold (`0x6E12C..0x6E1A8`) or the own `+0x59`
   copy (`0x6E1B3..0x6E1C5`).

Cross-check: FU-81 §1.4 already classified the same bodies; this slice adds
the full table indexing, the recovered `06E244` RNG path, the `06E05C` tail,
and the `06E004` recovered tail.

### 3.2 `0x8xxxx` — phase timeline/cinematic (phases `17..22`, 12 slots)

Every target starts (except `0890EC`, pure reset, and `08A798`, whose head
starts at the stage dispatch) with the same timer tick:
`DX = [0x58818]; AX = [0x57A64]; DX += AX; [0x58818] = DX` (e.g.
`0x88DD7..0x88DF5`, `0x89114..0x89128`, `0x89FC8..0x89FDC`,
`0x8B691..0x8B6A5`), then dispatches on the stage byte `[0x58829]` with
inline compares (`0x88E39..0x88E46`, `0x8912F..0x89137`, `0x89644..0x89650`,
`0x89FE3`, `0x8B6AC..0x8B6BE`, `0x8B892..0x8B898`, `0x8B91F..0x8B927`) or a
`CS:` jump table (flat `0x88F3C`, `0x89F88`). Actions:

* **Camera**: `FUN_000700F4` with `[0xF328]/[0xF32C]/[0xF330]` triples
  (`0x88E4D..0x88E6A`, `0x88F81..0x88FA0`), `0x73E28`/`0x73E08`,
  `0x740A0(EAX)` (match-phase entry, §1.2), lead vector via
  `[0x58830]/[0x58838]` (`0x89280..0x892BD`, `0x8B6EB..0x8B721`).
* **Sound/event**: `0x974DC(id)` / `0x974F0(id)` (`0x89160/0x8916A`,
  `0x8A02B/0x8A044`, `0x8B8A3`, `0x8B93B`, `0x8B96E`).
* **Action/record installs**: `0x7D9A4(rec, code)` (`0x8A0A0` code `0x16`,
  `0x8990A` code `0x19`, `0x8994C` code `3`), animation `0x6E598(rec, code)`
  (`0x89923`, `0x89960`).
* **Reset**: `0890EC`; `089868`/`08B874` clear `[0x58808]/[0x58828]/
  [0x58829]` at their ends.

Relation to the ports: these are the callers of the **camera** subsystem
(`FUN_000700F4` is the original of FU-71's `fifa96_camera_init`; the lead
vector step `0x89280..0x892BD` is camera-ball-follow math, not the same
block as `fifa96_camera_update`), the **sound** subsystem (`0x974DC/0x974F0`
are the sfx-id calls, FU-57/FU-58 family), the **animation selector**
`0x6E598` (ported in FU-82 as `fifa96_action_sequence_*`), and the **action
installer** `0x7D9A4` (FU-81/FU-82 family). They emit into the FU-63 event
queue only indirectly through those calls; no driver dispatches through
`fifa96_event_queue` itself.

### 3.3 Classification summary

* Formation/placement: phases `00..15` (22 slots over 11 distinct stored
  values / functions).
* None: phase `16` (zero → INT3).
* Phase timeline: phases `17..22` (12 functions, one of them pure reset).
No target in the table is an action dispatcher or a posture/animation body.

## 4. Port: `fifa96_action_phase_*`

`include/fifa96_loader/fifa96_action_handlers.h` +
`src/fifa96_loader/fifa96_action_handlers.c` (caller-owned state, no globals,
negative `fifa96_err_t` for invalid arguments, no comments).

| original | port |
|---|---|
| handler install `EAX=[0x57A4A]>>24; EAX<<2; ADD 0x10794; EAX=[EAX]; [rec+0x1C]=EAX` (`0x6D9A8..0x6D9BA`) plus the caller loops `0x8CF73`, `0x8D107` | `fifa96_action_phase_install(records, count, phase, table, table_count)`: writes `records[i].handler = table[phase]` for every caller-owned record; `phase >= table_count` → error (hardening divergence; the original has no bounds check and reads the next flat block); zero table entries install `0` |
| install gate `CMP byte [rec+0x9A],0; JNZ skip` (`0x8D0F7..0x8D10E`) | `fifa96_action_phase_drive(active, &drive)`: `drive = (active == 0)` |
| cell math (`0x6E1F4..0x6E238`, `0x6DCF6..0x6DD35`) | `fifa96_action_phase_cell(x, z, side, &out)`: `out.x = side ? -x*0x26 : x*0x26`, `out.y = 0`, `out.z = side ? -z*0x21 : z*0x21` |
| `v + v/4` slot math (`0x6DEA2..0x6DEE0`, `0x6DDC4..0x6DE12`) | `fifa96_action_phase_slot(x, z, side, &out)`: arithmetic `v + (v>>2)` per axis, side negation, `out.y = 0` |
| ball-z entry pick (`0x6DDAD..0x6DDDD` / `0x6DDE1..0x6DE12`) | `fifa96_action_phase_ball_entry(ball_z, side, &index)`: side 0 → `ball_z>0`, else `ball_z<0` |
| full ball line including `±0x60` (`0x6DD9C..0x6DE31`) | `fifa96_action_phase_ball_line(entries, ball_z, side, &out)`: picks the entry pair, slot math, `out.z += (ball_z>0 ? -0x60 : +0x60)` |
| timer `[rec+0x89] += [0x57A64]` + `>=0x3C` gate (`0x6DF55..0x6DF6C`) | `fifa96_action_phase_line_timer(&timer89, delta, &ready)`: 32-bit wrapping accumulate, signed `>= 0x3C` |
| timer line `0x780`/`0xCC0` + `±5*([rec+0x8A]>>25)` by `[rec+0x8D]&1` (`0x6DF8F..0x6DFF7`) | `fifa96_action_phase_restart_line(axis, offset, lateral, &out)`: `out.x = 0x780`, `out.z = (axis ? 1 : -1) * 5*(offset>>25)`; `|lateral| < 0x20` → `(0xCC0, z=0)`; `out.y` not written (matches original) |
| `FUN_000700F4` camera push, `0x974DC/0x974F0` sound, `0x7D9A4/0x6E598` installs, `CS:` stage tables, resolver pointers/`0x4BFC0`, RNG scatter `0x92AC8` | not ported (camera/sound/action subsystem calls, pointer tables, runtime table, cut bodies; open legs) |

## 5. Tests (`tests/test_phase_drivers.c`, suite 71 → 72)

* Layout `_Static_assert`s on `fifa96_action_phase_record`.
* `phase_install`: all records get the slot handler; zero slot installs `0`;
  last slot `0x22`; bounds `0x23` → error with records unchanged;
  `records=NULL,count=0` OK; `NULL`/empty table invalid.
* `phase_drive`: `0→1`, non-zero → `0`, `NULL` invalid.
* `phase_cell`: `(2,3)` → `(0x4C,0,0x63)`, side negation, sign pairs, `NULL`.
* `phase_slot`: `v + (v>>2)` including negative arithmetic shift, side
  negation, `NULL`.
* `phase_ball_entry`: all four sign/side combinations + zero + `NULL`.
* `phase_ball_line`: entry pick, scale, `±0x60` by ball-z sign, `y==0`,
  `NULL` entries/out.
* `phase_line_timer`: below/at `0x3C`, 32-bit wrap to a small value,
  `NULL`.
* `phase_restart_line`: axis sign, offset `>>25` arithmetic (incl. a
  non-multiple `-0x01800000` → `-5`), `|lateral|<0x20` branch, `out.y`
  untouched.
* Suite: **71/71 before, 72/72 after**. The new test also runs clean under
  `-fsanitize=address,undefined` together with
  `fifa96_action_handlers.c`/`fifa96_entity_update.c`.

## 6. Errata (quoted)

* FU-81 §1.2 table header "slots `0x2D..0x4F`" — **clarified**: `0x2D..0x4F`
  are flat-block positions in `0x1106E0`; the runtime index added to
  `0x10794` is the **phase byte itself**, so the table's 35 entries are
  phases `0x00..0x22` and `slot = phase + 0x2D`. FU-81's own phase labels
  (`slot 0x43` = phase `0x16`, `0x22` last in its test) already used this;
  this doc states the arithmetic explicitly.
* FU-81 §1.5 "`0x8xxxx` group (`0x44..0x4F`)" — **corrected mapping**:
  those slots are phases `0x17..0x22`; the group's shared tick is
  `[0x58818] += [0x57A64]` (16-bit), confirmed at `0x8B691/0x8B697/0x8B69E`
  etc.
* FU-81 §1.4 `06E244` "listing mis-decodes at `0x6E280`" — **extended**:
  the byte at `0x6E27F` is `31` (`XOR EDX,EDX`), then `8A 15 C2 7A 05 00`
  = `MOV DL,[0x57AC2]` at `0x6E281`; the recovered RNG-scatter path and the
  side-mismatch return (`0x6E326`) are quoted in §3.1.
* FU-81 §1.4 `06E05C` tail "0x14E04 fold offsets" — **verified**: the tail
  is two `SHRD`-scaled folds of the record's `+0x8A>>24` and the ball
  record's `+0x8A>>24` through the `0x14E04` dword table into out x/z
  (`0x6E12C..0x6E1A8`), then the fallback copy (`0x6E1B3..0x6E1C5`).
* FU-82 §1 closed FU-81 open leg 1: the 8-slot stage bucket is
  `09,0C,0E,10,11,12,13,17` (FU-82 derived partition). This table's phases
  are unrelated to that action-code bucket; the two share only the `[0x57A4A]`
  phase byte as a gate.

## 7. Open legs

1. **`[0x57A4A]` writers**: this slice cites dozens of readers; which code
   sets the phase byte (and how phases `0x00..0x22` are sequenced) is not
   derived.
2. **`0x4BFC0`/flat `0x14BFC0` resolver table**: zero in the static image;
   the runtime builder and the four pointer families' element layouts are
   not derived.
3. **`0x1033A` formation rows**: the 29-byte row fields beyond group
   sub-id/count/index (and the `0x105E7/0x105E8` int8 cells) are cited only.
4. **`0x8xxxx` semantics**: `[0x58818]/[0x58829]/[0x58808]/[0x58828]`
   meanings, the `[0xF328..]` camera triples, sound ids `0x974DC/0x974F0`,
   `[0x5B680]`/`[0x57AC2]` mode bytes, and the `0x73E28/0x73E08/0x740A0`
   engine wrappers are cited, not decomposed.
5. **Stage arms**: the `CS:` tables `0x88F3C`, `0x89F88` and the inline
   stage bodies are cited by address and entry block only.
6. **`089620`/`08A798`**: the `0x57BD2`/`0x4C360`/`0x57B8E` competition-state
   path and the `[0x5882B..0x5882C]` player-id addressing are cited, not
   decomposed.
7. **`FUN_0008D098`'s `[0x57A4D]` jump table `0x8D040`**: 22 phase bodies
   beyond the install loop are not derived.
8. **`0x92AC8` RNG** and **`0x8DCD4` movement-vector** semantics: cited as
   call sites; internals not derived.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x6D920, 0x6D9C4, 0x4AFB8, 0x7997C, 0x8CF60, 0x8D098,
0x8DCD4, 0x740A0, 0x73E08, 0x6E1D0, 0x6DCC8; `disassemble_bytes` 0x6DE34,
0x6DE44, 0x6DD6C, 0x6DD9C, 0x6DF4C, 0x6E004, 0x6E1C8, 0x6E244 (raw
`read_memory` 0x6E244, 256 B), 0x6E2F5, 0x6E05C, 0x6E12B, 0x88DC8, 0x88F4C,
0x890EC, 0x89110, 0x8922C, 0x89620, 0x89868, 0x89FA4, 0x8A798, 0x8B688,
0x8B874, 0x8B900; `read_memory` 0x110794 (140 B), 0x110820 (48 B), 0x4BFC0
(64 B), 0x14BFC0 (64 B); `search_instructions` operand `57a4a` (100+ readers);
`get_function_xrefs` 0x6D920 (4), `get_function_callers` 0x8CF60, 0x8D098,
0x6D9C4, 0x7997C, 0x8CE78. Analysis-only outside the port: no tool,
capture-rig, ISO or Ghidra-project change. Port write set:
`include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c`, `tests/test_phase_drivers.c`,
`CMakeLists.txt` (one library/test block). `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
