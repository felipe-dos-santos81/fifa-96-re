# FU-192 — the per-record phase handlers `0x00–0x15` (phase-17 wave R40)

Provenance: M2 phase-17 wave **R40**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; `get_current_program_info` this session: watcom `LE:32:default`, image
base `0`, `min_address 0x10000`). Every byte/address/xref/instruction below is a
fresh `read_memory` / `disassemble_bytes` / `decompile_function` /
`get_xrefs_to` / `get_function_by_address` of **this session** unless marked
*cross-cited* (then the frozen FU doc is named). No writes: no rename, comment,
label, function, script or project save; `/FIFA96.EXE` untouched; no git
commands. The alias traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` =
byte at `x+3`). **Ruling R1: `0x6DCC8` is owned by T1 — this slice verifies it
only, it does not derive or port it.**

Scope: (1) fresh phase-table `0x110794[0x00..0x15]` read + the family partition;
(2) derive **every distinct** per-record body byte-level — its address range,
what it reads from the record, what it writes, and its side effects; (3) the
M2-tape phase set (fresh golden read) and a per-row **tape-dormancy** verdict;
(4) the ranked producer-live set + a per-body T3 wiring rec; (5) numbered legs.

---

## 1. Fresh phase-table read + family partition

Fresh `read_memory 0x110794` (140 B = 35 dwords), byte-identical to FU-177 §3 /
FU-190 §1:

```
[00]=06DE34 [01]=06E1D0 [02]=06DCC8 [03]=06DE44 [04]=06DE44 [05]=06E05C
[06]=06DD9C [07]=06DE44 [08]=06DD6C [09]=06DD6C [0A]=06DE34 [0B]=06DE34
[0C]=06DF4C [0D]=06DE34 [0E]=06DE34 [0F]=06DE34 [10]=06E004 [11]=06E1C8
[12]=06E1D0 [13]=06E244 [14]=06E244 [15]=06DCC8 [16]=00000000 (INT3)
[17]=088DC8 [18]=08922C [19]=089FA4 [1A]=089620 [1B]=0890EC [1C]=089110
[1D]=089868 [1E]=08A798 [1F]=088F4C [20]=08B688 [21]=08B874 [22]=08B900
```

The `0x00..0x15` (per-record) region has **eleven distinct target bodies**
(the brief's enumeration omits one — **errata** below):

| body | rows (phase) | bytes (fresh) | xref (`get_xrefs_to`) |
|---|---|---|---|
| `0x6DE34` | `00 0A 0B 0D 0E 0F` | `0x6DE34..0x6DE40` (13) | **6** DATA (`0x110794/0x1107BC/0x1107C0/0x1107C8/0x1107CC/0x1107D0`) |
| `0x6E1D0` | `01 12` | `0x6E1D0..0x6E241` (114) | **4** — 2 DATA (`0x110798`,`0x1107DC`) + 2 CALL (`0x866E7`,`0x876D0`) |
| `0x6E1C8` | `11` | `0x6E1C8..0x6E1CF` (8) | **1** DATA (`0x1107D8`) |
| `0x6DCC8` | `02 15` | `0x6DCC8..0x6DD3B` (116) | **5** — 2 DATA (`0x11079C`,`0x1107E8`) + 3 CALL (`0x6DD61`,`0x6DD91`,`0x6DF37`) — **T1** |
| `0x6DE44` | `03 04 07` | `0x6DE44..0x6DF4A` (263) | **3** DATA (`0x1107A0`,`0x1107A4`,`0x1107B0`) |
| `0x6E05C` | `05` | `0x6E05C..0x6E1C5` (362) | **1** DATA (`0x1107A8`) |
| `0x6DD9C` | `06` | `0x6DD9C..0x6DE31` (150) | **1** DATA (`0x1107AC`) |
| `0x6DD6C` | `08 09` | `0x6DD6C..0x6DD98` (45) | **2** DATA (`0x1107B4`,`0x1107B8`; table read) |
| `0x6DF4C` | `0C` | `0x6DF4C..0x6E001` (182) | **1** DATA (`0x1107C4`) |
| `0x6E004` | `10` | `0x6E004..0x6E05B` (88) | **1** DATA (`0x1107D4`) |
| `0x6E244` | `13 14` | `0x6E244..0x6E32A` (231) | **2** DATA (`0x1107E0`,`0x1107E4`) |

**Errata (brief enumeration):** the R40 brief lists `0x6DE34` as the "shared
reset" family and does not name `0x6DE44`. The fresh table shows **`0x6DE44` is a
distinct body** used by rows `03/04/07` (3 DATA refs, fresh) — not a variant of
`0x6DE34` (8 bytes longer, a different prologue). It is derived below (§2.4).
Row `03` is FU-177's hot path; its phase body is this `0x6DE44`.

Adjacent non-target code found in the same window (fresh; kept for xref
completeness, **not** phase handlers):
* `0x6DD3C..0x6DD68` — an **orphan** body (fresh `get_xrefs_to 0x6DD3C` = **0**)
  that calls `0x6DCC8` at `0x6DD61` (the first of T1's three internal callers).
  Dead code (no refs); leg FU192-L5.
* `0x6E330..` — a helper reached by `FUN_0006e490` (`0x6E4E2`) and
  `FUN_0006e8e8` (`0x6FC84`); it reads `[EAX+0x8D]` and returns
  `0x11052A`/`0x1105CA` pointers — **not** a phase target.

The phase dispatch ABI (fresh `decompile_function 0x6D920`, `0x8D098`;
*cross-cited* FU-191 §9): the per-record stager stores
`[rec+0x1C] = 0x110794[[0x157A4A]>>24]` (the byte at `0x157A4D`), and
`FUN_0008D098` calls it as `handler(EAX=rec, EDX=&rec+0x4D)` per non-skipped
record. Every body below therefore reads `EAX` = the record and writes the
**output triple `EDX[0..2]` = `[rec+0x4D]/[rec+0x51]/[rec+0x55]`**.

---

## 2. Per-body derivations (fresh `disassemble_bytes 0x6DD3B` 1440 B + `0x6E2DA` 144 B)

Every body writes **only** the target triple (`EDX[0]`, `EDX[+4]`, `EDX[+8]`) —
plus, for `0x6DF4C` only, the record's own `+0x89`/`+0x7B` cells. None writes the
phase byte, an animation id, a frame, or the position triple; none writes any
other record. (Audited instruction-by-instruction this session.)

### 2.1 `0x6DE34` — shared reset / hold (rows `00 0A 0B 0D 0E 0F`)

```
0x6DE34 PUSH ESI ; 0x6DE35 PUSH EDI ; 0x6DE36 MOV EDI,EDX
0x6DE38 LEA ESI,[EAX+0x59] ; MOVSD ; MOVSD ; MOVSD   ; copies [rec+0x59/+0x5D/+0x61]
0x6DE3E POP EDI ; POP ESI ; RET
```
Reads: the record **position triple** `+0x59/+0x5D/+0x61`.
Writes: `target[0..2] := position[0..2]` (3× MOVSD). No globals, no state — a
pure "hold / held-position copy" (FU-83). **Zero prologue state.**

### 2.2 `0x6DD6C` — rows `08 09` (thin wrapper over `0x6DCC8`)

```
0x6DD6E MOV ECX,[0x157AAC] ; SAR 0x18        ; ECX = byte 0x157AAF (controlled side)
0x6DD74 MOV EBX,[EAX] ; MOVZX ESI,byte[EBX+0x826]   ; team side
0x6DD80 MOV EBX,0xBC ; CMP ESI,ECX ; JZ +5 ; MOV EBX,0x6F
0x6DD91 CALL 0x6DCC8      ; (EAX=rec, EDX=target, EBX=0xBC|0x6F)
```
Reads: `[rec]` (team), `[team+0x826]` (side), global `[0x157AAC]>>24`.
Writes: via `0x6DCC8(rec, target, idx)` → `target[0]=x, target[1]=0, target[2]=z`
from the `[rec+0xC]` direction pair at index **0xBC** (same side) or **0x6F**
(other side), × `±0x26 / ±0x21` (side-flipped). **Depends on `0x6DCC8` + the
`[rec+0xC]` OL-13 metadata** (T1 / R44). No new target field.

### 2.3 `0x6DD9C` — row `06`

```
0x6DDA2 MOV CL,byte[team+0x826] ; 0x6DDA8 MOV EAX,[EAX+0x10]   ; [rec+0x10] pointer
0x6DDAD JNZ (side) ; branches on side, indexes [rec+0x10] by ([0x157754]>0)
... target[0] = (int16)word[idx] * 1.25 ; target[2] = (dword idx)>>16 * 1.0625
0x6DE15 CMP dword[0x157754],0 ; JLE +6 ; SUB target[2],0x60  else ADD target[2],0x60
```
Reads: `[rec]` (team), `[team+0x826]` (side), **`[rec+0x10]`** (a metadata/geometry
pointer — **not** `[rec+0xC]`), global `[0x157754]` (dword sign).
Writes: `target[0]`, `target[1]=0`, `target[2]` (with a `±0x60` bias by
`[0x157754]`'s sign). **Source is `[rec+0x10]`** (distinct from `0x6DCC8`'s
`[rec+0xC]`).

### 2.4 `0x6DE44` — rows `03 04 07` (the FU-177 hot-path family)

```
0x6DE4B EBP=rec ; ECX=target ; EAX=[rec] ; AL=byte[team+0x826] ; EBX=([0x157AAC]>>24)
0x6DE65 DX = word[0x1577D8 + gside*4]        ; per-side selector word
0x6DE73 JL 0x6DEE5                            ; DX<0 -> RNG/table branch
DX>=0:  EDX = [0x1577D6 + gside*4] >> 16 ; EAX = [rec+0x14] + idx*8
        ... target[0]/target[2] = (int16)word[EAX] * 1.25 ; target[1]=0
DX<0:   copy [0x15774C] (3 dwords) ; ±0x1E0 by byte 0x157AAF
        CALL 0x6D870(cell-map) ; CALL 0x6DCC8(rec,target,ang) ; target[1]=0
```
Reads: `[rec]` (team), `[team+0x826]` (side), **`[rec+0x14]`** (metadata pointer),
globals `[0x157AAC]>>24`, tables `[0x1577D6 + gside*4]`/`[0x1577D8 + gside*4]`,
`[0x157AAF]`, base `[0x15774C]`.
Writes: `target[0..2]`; `target[1]=0`. Calls `0x6D870` (a pitch-cell mapper,
`0x6D870..0x6D8xx`, writes only its stack out-params) and `0x6DCC8` (the DX<0
branch). This is the body FU-177 §2 derived at the **action-row** level
(`0x7E1A4` reads `[rec+0x1C]`); its **phase**-row instance (`0x110794[3/4/7]`) is
this `0x6DE44`.

### 2.5 `0x6DF4C` — row `0C` (stateful restart-line timer)

```
0x6DF55 EDX=[rec+0x89] ; AX=[0x157A64] ; [rec+0x89] += AX   ; TIMER ACCUMULATE
0x6DF69 CMP EDX,0x3C ; JL exit
0x6DF8B word[rec+0x7B] = byte[0x157A38 + ([rec+0x4]→[+0xA])>>24] >> 1
0x6DF8F target[0]=0x780 ; target[2]+=([rec+0x8A]>>0x19)*5*± ; CALL 0x8DCD4(metric)
... if |word[rec+0x69]| < 0x20 -> target[0]=0xCC0, target[2]=0
```
Reads: `[rec+0x89]` (timer), word `[0x157A64]`, `[rec+0x4]` (pointer → `[+0xA]`),
`[0x157A38]`, `[rec+0x8A]`, `[rec+0x8D]`, `[rec+0x59]`, `[rec+0x67]`/`[rec+0x69]`.
Writes: **`[rec+0x89]` (accumulated every call)**, `[rec+0x7B]`, and the target
triple (`0x780` / `0xCC0` / offset). Calls `0x8DCD4`. **Stateful** — the only
body that mutates a record field besides the target.

### 2.6 `0x6E004` — row `10` (variant table)

```
0x6E005 BL=[rec+0x8D] ; JZ default
0x6E013 EBX = (int8)BL - 1 ; ECX = (int8)byte[0x1105EA + 2*EBX] << 5  ; target[0]
                             EBX = (int8)byte[0x1105EB + 2*EBX] << 5  ; target[2]
0x6E036 if byte[[rec]+0x826]!=0 NEG target[2] ; target[1]=0
default (active==0): target[0]=0x3C0 ; target[2]=0x840 ; target[1]=0
```
Reads: `[rec+0x8D]` (active index), the 2-stride signed-byte table at
`0x1105EA`/`0x1105EB` (fresh `read_memory 0x1105E0` 32 B; via the raw
`[EBX*2+0x1105E7/8]>>24` alias), `[rec]` (team side).
Writes: `target[0]`, `target[1]=0`, `target[2]`. Pure (no state).

### 2.7 `0x6E05C` — row `05` (ball-relative line)

```
0x6E066 EBP=target ; EDX=[0x157A9F] (ball) ; CMP [rec]==[ball.team] ; JNZ pos-copy
... dist metric 0x8DC68 over ball/rec [rec+0x8A] ; copy ball[+0x59..+0x61] -> target
    target[0] += 0xC0*cos([rec+0x8A]) ; target[2] += 0xC0*sin(...)   (0x114E04 fold)
0x6E1B3 (else) MOVSD x3: target := rec position
```
Reads: `[rec]` (team), `[0x157A9F]` (ball), `[ball+0x59/+0x61]`,
`[rec+0x59/+0x61]`, `[rec+0x8A]`, tables `0x114E04`, globals `0x157AC2`,
`0x157AC5`, `0x15881A`, `0x157AB6`.
Writes: `target[0..2]` (ball position + `0xC0`·(cos,sin)) **or** the rec position
copy. Calls `0x8DC68`. Pure.

### 2.8 `0x6E1C8` — row `11` (shim into `0x6E1D0`)

```
0x6E1C8 MOV EBX,-1 ; 0x6E1CD LEA EDX,[EAX+0x4D] ; <fall-through> 0x6E1D0 PUSH ECX
```
8 bytes, no RET of its own: it sets `EDX=&rec+0x4D`, `EBX=-1`, then **falls
through into `0x6E1D0`**. Under the walk ABI (`EDX=&rec+0x4D` already) and since
`0x6E1D0` overwrites both `EBX` (`0x6E1D8`) and `EDX` (`0x6E1EA`), **row 11 ≡
rows 01/12**. The `EBX=-1` is dead on this path. Leg FU192-L6.

### 2.9 `0x6E1D0` — rows `01 12` = `fifa96_scene_formation_place` (already ported)

```
0x6E1D6 ECX=EDX ; EBX=[0x157AAC]>>24 ; ESI=byte[[rec]+0x826]
0x6E1EA EDX=[rec+0x8] ; if side==gside EDX+=2
0x6E1F4 BX=(int8)byte[EDX] ; ... target[0]=BX*0x26 ; target[2]=(int8)byte[EDX+1]*0x21
0x6E21C if byte[[rec]+0x826]!=0 NEG BX,NEG DX ; target[1]=0
```
Reads: **`[rec+0x8]`** (the formation byte pair — the native `[team+0x7AE]`
formation blob), `[rec]` (team), `[team+0x826]` (side), `[0x157AAC]>>24`.
Writes: `target[0]=byte*0x26`, `target[1]=0`, `target[2]=byte*0x21`, side-flipped.
**This body is already ported and wired**: `fifa96_scene_formation_place`
(`port/src/fifa96_loader/fifa96_scene.c:181`), used by
`fifa96_match_entities_seed_formation` (`fifa96_match_entities.c:346`). Fresh
`get_xrefs_to 0x6E1D0` also shows two **action-body** callers (`0x866E7` in
`FUN_0006E490`, `0x876D0`) — the formation placement is shared with those bodies.

### 2.10 `0x6E244` — rows `13 14` (camera-scatter; consumes RNG)

```
0x6E24C EDX=[0x157AAC] ; EBX=byte[team+0x826] ; CMP EBX,EDX ; JNZ RET  ; gate
0x6E267 BX=word[0x10F368] ; SI=word[0x10F364] ; DL=[rec+0x8D] (active)
inactive:  target[2]=±BX (by [0x157AC2]&side) ; target[0]=SI ; RET
active:    CALL 0x92AC8 ; /0x90 ; CALL 0x92AC8 ; TEST AL,1 ; ± delta ; target[0]=SI±delta
```
Reads: `[rec]` (team), `[team+0x826]` (side), `[rec+0x8D]` (active), globals
`[0x10F368]` (word), `[0x10F364]` (word), `[0x157AC2]` (byte), RNG `0x92AC8`.
Writes: **`target[0]` and `target[2]` only — `target[1]` is NOT written** — and
**only when `side == [0x157AAC]>>24`**; otherwise a bare RET. Consumes the PRNG
`FUN_00092ac8` (fresh `decompile_function`: an xorshift/LCG over `0x110F44..58`)
**twice per active record**. `0x10F364`/`0x10F368` are set by the `FUN_0008D098`
case `0x13/0x14` arm (fresh `decompile_function 0x8D098`), which is **not
modelled** in the engine (the pool only clears them — `fifa96_match_run.c:2013`).

### 2.11 `0x6DCC8` — rows `02 15` (**T1-owned; verified only, per R1**)

Fresh `decompile_function 0x6DCC8` matches FU-190 §6.2 / FU-177 §3:
`if (param_3 < 0) iVar5 = word[0x1577DE + byte[[rec]+0x826]*4]>>16; else iVar5 =
param_3; pcVar1 = [rec+0xC] + iVar5*2; target[0]=byte0*±0x26; target[1]=0;
target[2]=byte1*±0x21` (side-flipped by `[team+0x826]`). Reads `[rec+0xC]`
(param_1[3]), `[rec]`/`[team+0x826]`; writes the target triple. Not re-derived
here (R1).

---

## 3. The record inputs the bodies read (pool binding)

| native read | meaning | engine pool |
|---|---|---|
| `[rec]` | team pointer | `records[][].team` |
| `[team+0x826]` | team side | `team[].side` |
| `[rec+0x59/+0x5D/+0x61]` | position triple | `pos_x/pos_y/pos_z` |
| `[rec+0x4D/+0x51/+0x55]` (EDX) | **output** target triple | `target_x/target_y/target_z` |
| `[rec+0x89]` | action timer | `timer89` |
| `[rec+0x7B]` | counter word | `timer7b` |
| `[rec+0x8A]` | angle selector (24-bit) | *not a distinct field* (leg FU192-L1) |
| `[rec+0x8D]` | active flag/index | `active` |
| `[rec+0x8]` / `+0xC` / `+0x10` / `+0x14` | **metadata/direction pointers** | *not modelled* — legs FU192-L1 |

The four pointer fields `[rec+8/+0xC/+0x10/+0x14]` are the **OL-13 metadata**
(this slice's `get_xrefs_to`/disasm evidence; *cross-cited* FU-137 §3/FU-177 §3):
`0x6E1D0` reads `[+8]` (already modelled as the formation file), `0x6DCC8` reads
`[+0xC]`, `0x6DD9C` reads `[+0x10]`, `0x6DE44` reads `[+0x14]`. Only `[+8]` has a
derived engine source today; the other three are the R44/T2 producer machine's
scope.

---

## 4. Tape-dormancy — the M2 tape's phase set is decisive

FU-191 §10 established the walk's render argument. This slice adds the
**decisive, tape-grounded** half: the M2 golden's `state=<phase>/…` suffix *is*
the per-record phase the walk dispatches (`state` = native `[0x157A4A]>>24` =
byte `0x157A4D`; test header `test_engine_m2.c:18`). Fresh `grep -o 'state=…'`
over `port/tests/golden/engine/m2-frames.txt` (792 lines) gives the visited phases:

```
frame  6: phase 0x13 (19)   frames   6..25
frame 26: phase 0x14 (20)   frames  26..45
frame 46: phase 0x02 ( 2)   frames  46..65
frame 66: phase 0x00 ( 0)   frames  66..369
frame 370: phase 0x15 (21)  frames 370..469
frame 470: phase 0x04 ( 4)  frames 470..772
frame 773: phase 0x02 ( 2)  frames 773..792
```

**Visited phases = {0x00, 0x02, 0x04, 0x13, 0x14, 0x15}.** Mapping to bodies:

| phase | body | on the M2 tape? |
|---|---|---|
| `0x00` | `0x6DE34` (hold) | **yes** (304 frames) |
| `0x02` | `0x6DCC8` (T1) | **yes** |
| `0x04` | `0x6DE44` | **yes** (303 frames) |
| `0x13`,`0x14` | `0x6E244` | **yes** (40 frames) |
| `0x15` | `0x6DCC8` (T1) | **yes** (100 frames) |
| `0x01 0x03 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F 0x10 0x11 0x12` | (their bodies) | **never** |

**Dormancy verdicts:**

* **Off-tape rows** (`01 03 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12`) — **tape-dormant by
  construction.** The M2 tape never enters their phase, and none of these bodies
  writes the phase byte, the position triple, an animation id, a frame, `skip_9a`
  or any other record (audited §2: target-only, except `0x6DF4C`'s own
  `+0x89/+0x7B`). Wiring any of them into the phase table therefore **cannot move
  M2** — the handler is simply never dispatched on the tape. This is the clean,
  provable class (proved twice: never-reached **and** writes-off-render-path).
* **`0x6DE34` (phase `0x00`)** — *structurally a hold* (`target := position`).
  Render-invisible **iff** the phase-0 records are already at rest
  (`target == position`); if the engine's live phase-0 chain leaves a residual
  target, the write would freeze them. **Conditional — pin with the tape `cmp`
  (FU192-L4).**
* **`0x6DE44` (phase `0x04`, 303 frames)** — the largest on-tape body; reads
  `[rec+0x14]` + the `0x1577D6/D8` side tables + `0x6D870`/`0x6DCC8`. **Not
  structurally dormant; requires its own pinning (FU192-L3).**
* **`0x6E244` (phases `0x13/0x14`, 40 frames)** — writes `target[0]/target[2]`
  gated on `side == gside`, and **consumes the PRNG** (`0x92AC8`×2 per active
  record). PRNG-state divergence on top of the target write ⇒ **highest-risk,
  not structurally dormant (FU192-L2).**
* **`0x6DCC8` (phases `0x02/0x15`)** — **T1's**, per R1; the dormancy proof is
  T1's gate (FU-191 §10 names it the first honest mover).

Note the two live on-tape families (`0x6DE44`, `0x6E244`) are the *largest* and
*RNG-using* bodies respectively — the M2 tape is **not** trivially neutral when
`T3` starts wiring them.

---

## 5. Ranked producer-live set + T3 wiring rec

"Producer-live" here = the per-record walk actually dispatches the body on the
M2 tape's state (the only state R40 can ground without running the engine).
Ranked by (on-tape?) × (dormancy cost) × (dependency count):

| # | body (rows) | on-tape | dormancy | deps | T3 rec |
|---|---|---|---|---|---|
| 1 | `0x6E1D0` (01,12) | no | **trivial** | **none** — already ported (`fifa96_scene_formation_place`) | wire the row; tape-neutral |
| 2 | `0x6E004` (10) | no | **trivial** | none (pure table) | wire; tape-neutral |
| 3 | `0x6DD9C` (06) | no | **trivial** | `[rec+0x10]` pointer (FU192-L1) | wire body + leg the pointer; tape-neutral |
| 4 | `0x6E05C` (05) | no | **trivial** | ball/`0x114E04` | wire; tape-neutral |
| 5 | `0x6DF4C` (0C) | no | trivial (off-tape) but **stateful** | timer mutation | wire; note the statefulness |
| 6 | `0x6DD6C` (08,09) | no | **trivial** | `0x6DCC8` + `[rec+0xC]` | wire after T1's `0x6DCC8` |
| 7 | `0x6E1C8` (11) | no | trivial | ≡ `0x6E1D0` (shim) | wire as the `0x6E1D0` alias |
| 8 | `0x6DE34` (00,0A,0B,0D,0E,0F) | **yes** (row 00) | **conditional** (hold) | none | **pin row 00 before counting** |
| 9 | `0x6DE44` (03,04,07) | **yes** (row 04) | **not dormant** | `[rec+0x14]`, `0x6D870`, `0x6DCC8` | derive+pin (FU192-L3); row 03 is FU177-L1 |
| 10 | `0x6E244` (13,14) | **yes** | **not dormant + RNG** | `0x10F364/368` producer (unported) | **do not wire until FU192-L2 resolves** |

**T3 shape:** wire bodies **1–7 first** (all off-tape ⇒ provably tape-neutral),
then `0x6DE34` (#8) after a `cmp` pin of row 00, then `0x6DE44` (#9) with its own
derivation+pin. `0x6E244` (#10) stays a leg until the `0x10F364/368` producer and
the PRNG consumption are resolved. Census: each wired row is one non-NULL phase
row (the `0x6DE34` reset body fills **6** rows `00/0A/0B/0D/0E/0F`; `0x6E1D0`
fills `01/12`; `0x6DE44` fills `03/04/07`; `0x6E244` fills `13/14`).

---

## 6. Numbered legs

1. **FU192-L1 — the metadata pointer fields are unmodelled.** The pool has no
   `[rec+8/+0xC/+0x10/+0x14]` fields; `0x6DCC8`/`0x6DD9C`/`0x6DE44` read
   `[+0xC]`/`[+0x10]`/`[+0x14]` (only `[+8]` has a source — the formation file).
   The source is the OL-13 loader metadata (R44/T2 producer machine).
2. **FU192-L2 — `0x6E244` (rows 13/14) is not tape-dormant-as-derived.** It
   reads `0x10F364`/`0x10F368` (producer = the unported `FUN_0008D098` case
   `0x13/0x14` RNG scatter) and consumes the PRNG `0x92AC8` twice per active
   record. Whether a faithful port perturbs the M2 tape is unproven; do not wire
   it before resolving the producer + the PRNG contract.
3. **FU192-L3 — `0x6DE44` (rows 03/04/07) needs a dedicated derivation.** Its
   DX≥0/DX<0 branches (`0x1577D6/D8` side tables, `[rec+0x14]` indexing, the
   `0x6D870` cell map) are only sketched here; row 04 is on the tape (303
   frames), so it must be pinned before counting. Row 03's action path is
   FU177-L1 (T1).
4. **FU192-L4 — `0x6DE34` (row 00) dormancy pin.** Structurally a hold
   (`target := position`); whether the M2 phase-0 records are already at rest
   (write = no-op) must be pinned with a tape `cmp`.
5. **FU192-L5 — the orphan body `0x6DD3C`.** 0 xrefs (fresh `get_xrefs_to`); it
   calls `0x6DCC8` at `0x6DD61` (T1's first internal caller). Dead code; not a
   phase target; do not port.
6. **FU192-L6 — the `0x6E1C8` row-11 shim.** Sets `EDX=&rec+0x4D`, `EBX=-1`, then
   falls into `0x6E1D0`; both are overwritten there, so row 11 ≡ rows 01/12 under
   the walk ABI. Whether the shim carries a distinct native meaning is unproven.
7. **FU192-L7 — `0x6E1D0` is shared.** It is also called by two action bodies
   (`0x866E7` in `FUN_0006E490`, `0x876D0`); a future port must not double-count
   the formation placement when both the phase row and those bodies run.

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` (program `/FIFA96.EXE`, watcom LE, image base 0).
* `read_memory` — `0x110794` (140 B, the phase table), `0x1106E0` (180 B, the
  action table), `0x1105E0` (32 B, the row-10 variant table), `0x10F360` (16 B),
  `0x1577D0` (16 B).
* `disassemble_bytes` — `0x6DD3B` (1440 B: every `0x6Dxxx`/`0x6Exxx` body
  `0x6DD3B..0x6E2DA`), `0x6E2DA` (144 B: the row-13/14 tail through `0x6E369`).
* `decompile_function` — `0x6DCC8`, `0x6D920`, `0x8D098`, `0x92AC8`, `0x6D870`,
  `0x8DCD4`, `0x8DC68`.
* `get_function_by_address` — `0x6DCC8` (defined `0x6DCC8..0x6DD3B`), `0x6DE34`
  /`0x6DD6C`/`0x6DD9C`/`0x6DE44` (all **not defined** — raw code regions).
* `get_xrefs_to` — `0x6DCC8` (5), `0x6DE34` (6), `0x6DE44` (3), `0x6E1D0` (4),
  `0x6E244` (2), `0x6E05C` (1), `0x6DD9C` (1), `0x6DF4C` (1), `0x6E004` (1),
  `0x6E1C8` (1), `0x6DD3C` (0), `0x6E330` (2).

Engine/loader source read first-hand: `port/src/fifa96_loader/fifa96_scene.c`
(181..207, `fifa96_scene_formation_place`), `port/src/fifa96_loader/fifa96_arm_bodies.c`
(259..302, the `0x87159` `[0x10F364/368]` consumer), `port/src/fifa96_engine/
fifa96_match_handlers.c` (4638..4705, the phase table + `fifa96_match_dispatch_phase`),
`port/src/fifa96_engine/fifa96_match_act.c` (whole), `port/src/fifa96_engine/
fifa96_match_entities.c` (344..361), `port/src/fifa96_engine/fifa96_match_entities.h`
(61..164, 433..492), `port/src/fifa96_engine/fifa96_match_run.c` (1595..1608 the render
reads; 2425..2504 the phase arm), `port/tests/golden/engine/m2-frames.txt` (the
`state=` phase census). Docs cross-cited: FU-83, FU-136, FU-137 §3, FU-148 §3,
FU-175, FU-177 §1/§2/§3, FU-190 §1/§6.2, FU-191 §9/§10.

No writes beyond this slice file and the workspace report.

---

## 8. Erratum (M2 phase-23 T5, 2026-10-14) — the §4 tape counts are stale

The §4 tape-dormancy census ran over the older **792-line** golden; it is
superseded by the fresh v22 golden. Quoted from
`port/spec/FU247_exclusions_and_tape.md` §4.2 (first-hand):

> **FU192 §4 quote:** *"phase `0x13` (19) frames 6..25; phase `0x14` (20) frames
> 26..45 … `0x01 0x03 … 0x11 0x12` … never [on the tape]."* **Stale.** That
> census ran over the older **792-line** golden (`test_engine_m2.c:1340` cites
> "792 lines"). The fresh v22 golden (6522 lines) visits `0x01` for **953**
> frames, `0x13` for **2640** (not 20), and `0x0C` for 4 (also new). The
> double-wire verdict is **unchanged** (`0x01`/`0x11`/`0x12` still excluded), but
> the *tape-term* consequence is stronger: wiring phase `0x01` would put the
> formation re-placement onto **953 on-tape frames**, not zero.
>
> * **Spec §1/§2/§5 "phase `0x14`'s 40 frames"** (and FU192's `0x13` = 20
>   frames): fresh = `0x14` **20** frames, `0x13` **2640** frames. The A-mover
>   tape risk for `0x13`/`0x14` (FU240/FU192-L2) must be sized against **2660**
>   on-tape frames, not 60.

The §4 conclusion that phase rows `0x01`/`0x11`/`0x12` stay excluded is
**unchanged**; only the frame counts move. Fresh v22 golden = 6522 lines, sha
`b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a`.

---

## 9. Erratum (rows 03/04/07 + row 05 bodies, 2026-11-04) — FU192-L3 landed; §2.7's "ball" reading corrected

Fresh read-only Ghidra window, program exactly `/FIFA96.EXE` (scripts disabled;
`LE:32:default`, image base 0): `disassemble_bytes 0x6DE44` (263 B),
`0x6D870` (0x6D870..0x6D901), `0x6DCC8` (0x6DCC8..0x6DD3B), `0x6E05C` (361 B),
`0x8DC68` (0x8DC68..0x8DD26), `0x6DAB4` (16 insns), `0x70270` (24 insns),
`0x73C00` (39 insns); `get_xrefs_to 0x1577D8` (3), `0x1577D6` (7),
`0x15774C` (100 of 260), `0x157AAF` (11), `0x157A9F` (18); `read_memory
0x114E04` (256 B), `0x1577D0` (32 B). The alias traps apply (`dword[x]>>16` =
word `x+2`, `>>24` = byte `x+3`).

### 9.1 `0x6DE44` (rows 03/04/07) — §2.4's DX<0 sketch, completed

Fresh instruction text for the DX<0 arm (`0x6DEE5..0x6DF37`):

```
6DEE5 EDI=ESP ; 6DEE7 ESI=0x15774C ; 6DEEC AH=byte[0x157AAF]
6DEF2..6DEF4 MOVSD x3          ; local vec3 := [0x15774C],[0x157750],[0x157754]
6DEF5 TEST AH,AH ; 6DEF7 JNZ 6DF00
6DEF9 EAX=0x1E0 ; 6DEFE JMP 6DF05
6DF00 EAX=0xFFFFFE20           ; -0x1E0
6DF05 EDX=[ESP+8] ; 6DF0D EDX+=EAX ; 6DF11 [ESP+8]=EDX   ; z +- 0x1E0
6DF09 EBX=ESP+0x10 ; 6DF0F EAX=ESP ; 6DF15 EDX=ESP+0xC ; 6DF19 CALL 0x6D870
6DF1E CMP word[ESP+0x14],0 ; 6DF26 EBX=[ESP+0xC] / 6DF2C EBX=[ESP+0x10]
6DF34 SAR EBX,0x10 ; 6DF37 CALL 0x6DCC8
6DF3C [ECX+4]=0
```

`0x6D870..0x6D901` (fresh) is the pitch-cell mapper: `x_bin = (vec_x+0x86B) /
0x14B` and `z_bin = (vec_z+0xC1E) / 0x10E` (IDIV, truncating), each clamp-tested
through its **low word** (`0x6D8A4..0x6D8BA` x: [0, 0xC];
`0x6D8BA..0x6D8CB` z: [0, 0x16]), `cell = z_bin*0xD + x_bin` (`0x6D8D0`/
`0x6D8D3`); out1 = `{z_bin, x_bin, (int16)cell}` (`0x6D8D9..0x6D8DE`) and out2 =
`{0x16-z_bin, 0xC-x_bin, (int16)(0x12A-cell)}` (`0x6D8E6..0x6D8FA`). The arm
picks out1 for side 0 / out2 for side 1 and passes the struct's **cell** word
(`SAR EBX,0x10`) to `0x6DCC8`, i.e. an explicit `[rec+0xC]` direction-pair index.

**The two selectors are one cell.** `word[0x1577D8+side*4]` is the high word
of the dword at `0x1577D6+side*4` (the alias rule), and the DX>=0 index is
`dword[0x1577D6+side*4] >> 16` — the same value. Fresh `FUN_000700F4`
(0x70290..0x702A2) maps the focus with `EAX=0x15774C, EDX=0x1577D6,
EBX=0x1577DA`, so side 0 reads the primary cell and side 1 the mirrored cell;
`0x702A4..0x702D7` then copies the pairs to `0x1577DE` (whose `>>16` is the
`0x6DCC8` default index) and `0x1577E2`/`0x1577E6`/`0x1577EA`. `0x1577D8` has
**no direct writer**: its three xrefs are all reads (`0x6DAF3`, `0x83DC4`,
`0x6DE65`); the writer is `0x6D870`'s EDX out-pointer at `0x7029F`. Because the
mapper clamps both bins to >= 0, `cell` in [0, 0x12A] and the word is never
negative — **the DX<0 arm is a dead arm in the shipped image** (it needs a
staged/negative cell).

Engine: `fifa96_match_phase_04` implements both arms; `match_phase_04_cell`
ports `0x6D870` (including the low-word-only clamp quirk); `mr->phase_cell_side
[2]` models `word[0x1577D8+side*4]` and replaces the former
`sel_1577d6`/`sel_1577d8` static zeros. It is staged 0 by init/begin (same
DX>=0 index 0 as before), and the DX<0 arm is unit-tested by staging a negative
cell. **Residual leg FU192-L8**: the `FUN_000700F4` cell-map producer
(`0x70290..0x702D7`) is not modelled (`fifa96_camera_event_set` ports the
reset subset only), so the live DX>=0 index stays the staged 0 rather than the
camera-focus cell. Phase `0x04` is **on-tape** (303 v22 frames), so wiring the
producer needs its own `cmp` pin.

### 9.2 `0x6E05C` (row 05) — §2.7's "ball" reading is a misnomer; the goal-team ring

Fresh `0x6E066..0x6E072`: `EDX=[0x157A9F]; EAX=[rec]; CMP EAX,[EDX]; JNZ
0x6E1B3` — `[0x157A9F]` is the **scanner-selected record pointer**, not the
ball. `get_xrefs_to 0x157A9F` = 18; the writer is `FUN_00088940 0x88AAC
EAX=[EAX*4+0x1577CE]; MOV [0x157A9F],EAX` (FU-303 §3.1: the per-side tracked
array `[0x1577CE+side*4]`; FU-184 §3 calls `[0x157A9F]` "the goal record").
The body therefore compares rec's team block with the selected record's and
then `rec == selected` (`0x6E10B/0x6E10D`, EBX is callee-preserved by
`0x8DC68`). §2.7's "ball/ball-team" wording is corrected here.

Fresh `0x6E078..0x6E0D8` — the distance limit, gated on the score pair:
`word[0x157AC5+side*2] == word[0x157AC5+(1-side)*2] + 1` (the record team's
side; `[0x157AC5]`/`[0x157AC7]` = `mr->score[0/1]`), then
`word[0x15881A] - word[0x157AB6] < 0x3C` (`0x6E0C2 JL`) **or**
`byte[0x157AC2] >= 3` (`0x6E0CE`) -> `ecx = 0x12C0`; else `ecx = 0x5A0`. The
period cells are live in the engine: `[0x15881A]` is the period length
(`state.period_length`, FU-170 §4) and `[0x157AB6]` the elapsed period seconds
(`state.period_seconds`, FU-213 §2). The **ac2 test is nested inside** the
score-equality branch, not an independent gate.

Fresh `0x6E0E8..0x6E0FF`: `dx = (int16)(word[sel+0x59] - word[rec+0x59])`,
`dz` likewise on `+0x61`, then `0x8DC68(dx, dz)`; its exact port is
`fifa96_entity_distance` (the `(a>>2 + a>>1)>>1` octagonal blend). `metric >=
limit` or `rec == selected` -> the copy. Fresh `0x6E11B..0x6E1A8`: `target :=
[sel+0x59/+0x5D/+0x61]` (y included) then `target[0] += round16(0xC0 *
fold(angle + 0x100))`, `target[2] += round16(0xC0 * fold(angle))` with
`angle = ((int32)(dword[rec+0x8A]) >> 24) << 6` = `(int8)byte[rec+0x8D] * 0x40`
(the FU255 alias finding; engine `active`, seeded to the record index).
`0x6E1B3` is the `target := rec position` copy.

The two folds are the 0x114E04 byte-shift idiom with the **bit-15** sign
(`SHL CH,7; SBB EDX,EDX; ADD CH,CH; SBB ECX,ECX; XOR/AND/SUB` and the `SHL AH,7`
sibling), i.e. `fold16(u)`: index `= ((u & 0xFF) ^ -bit8) + bit8`, negate on
bit 15. This differs from `fifa96_entity_sine`'s bit-9 sign for positive
operands above 0x1FF: record 9 (`0x240` = 202.5 deg) yields `(177, 73)` where
the bit-9 fold would negate z. The engine normalizes exactly
(`u &= 0xFFFF; n = u & 0x1FF; if (u & 0x8000) n = -n;`) then uses the canonical
257-entry `0x114E04` table fold (`match_phase05_fold16`; cell-equal for every
16-bit operand). Read `0x114E04` (256 B) matches the port table prefix
`0,402,804,1206,...`; entry 256 = 65536.

Engine: `fifa96_match_phase_05` implements both arms; `[0x157A9F]` is
`mr->goal_record` (`NONE` -> the copy arm), `[0x157AC5]`/`[0x157AC7]` are
`mr->score[]`, `[0x15881A]`/`[0x157AB6]` are the clock words, `[0x157AC2]` is
`mr->global_157ac2`, all live — no staged cell on this body. **Residual leg
FU192-L9**: the non-goal scanner arm of the `[0x157A9F]` producer
(throw-in/corner writes) stays FU173-L1, so a non-goal stale/`NONE` cell takes
the copy arm. Row `05` is off-tape in the v22 golden.

### 9.3 Tape verdicts (unchanged)

The v22 golden visits `{0x00,0x01,0x02,0x04,0x0C,0x13,0x14,0x15}` (fresh
`grep -o 'state=…'`, 6522 lines); row `0x05` is absent and row `0x04` is
present. The new DX<0 cell-map arm is staged 0, so `test_engine_m2` remains
byte-identical (6522-line `cmp`, this slice); `test_engine_m1` is unchanged.
