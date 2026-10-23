# FU-326 — the no-slot seed algebra and the port-fidelity audit (phase-34 R2-L8)

**Phase/Provenance:** M2 phase-34 Stage-1 recon-ahead wave, **Task 1 of 4**, 2026-10-23, read-only. Authoritative program **`/FIFA96.EXE`** (explicit argument; Ghidra MCP, scripts disabled), fresh `get_current_program_info` = `watcom:LE:32:default`, image base 0, `.image` overlay, **2708** functions. Every address/byte below is a fresh `disassemble_bytes` / `decompile_function` / `read_memory` / `search_instructions` of **this** session unless marked *carried* (then the source is named). The alias traps are applied (`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`); the `0x1577B8/BA/BC` and `0x1577C0/C2` pair reads are read as the word at the quoted offset. **No measurement:** this slice runs no binary, stages no cell and touches no golden — the measured numbers it cites are quoted (FU316, FU323, `tests/test_engine_m2.c:4116-4125`). The only repo write is this draft; no git; the ISO untouched; scratch under `/tmp/opencode/fu326/`.

---

## 1. The question

> **What exactly is the native no-slot seed algebra — the `0x7F056` arm (`camY == 0 && !has_slot`), the seed words and every packing term, the `v0` derivation, the `T(s)` travel — and does `fifa96_outfield_row04_step` + `fifa96_camera_event_set` reproduce it, byte for byte, over the whole edge-case domain (sign, width, the parked ±1, the moving ±3/±5, the `0x730` armer reach)?**

Spec `docs/superpowers/specs/2026-10-23-fifa96-m2-phase34-noslot-pan-design.md` §3/§4 and §9 item 1; plan Task 1. Bindings: FU316 §3.2/§4.3 + its Review erratum, the port at `src/fifa96_loader/fifa96_outfield.c:685-720` and the camera consumer.

## 2. The one-page answer

**The arm is exact, and the port matches it.** `0x7EFFA` tests the camera Y dword and takes three arms: `camY != 0` → the per-type dirs `<<5`; `camY == 0 && [rec+0x20]` → the slot dirs `<<6`; otherwise the no-slot arm — `|cam_x| <= 0x630 && |cam_z| <= 0xA20` gate, seed = `word[+0x73] << 2`, `word[+0x75] << 2`; gate fail → `(0,0)` **but the event still fires**. The pair is copied verbatim into `0x1577B8/BA/BC` by `0x71C94` and turned into `v0 = trunc(seed/0xC)` by `0x70544`'s fast path (`0x70646..0x7066a`, the signed `IDIV` helper `0xA2AEE`). The decay is `v ← trunc(0x18·v/0x20)` per pan step, `F4 = 0xC` with frame delta 2, i.e. **6 frames at `v0`, 7 at each successor**, so `T(v0) = 2·(6·v0 + 7·v1 + …)`; the measured `s = 0x40 → v0 = 5 → 5,3,2,1 → T = 0x90` is reproduced by the arithmetic, and the derived `s = 0x80 → T = 0x174` (FU316 §4.3) is arithmetic-correct.

**But the no-slot arm cannot reach `s = 0x80`.** The seed words `+0x73/+0x75` are the record's *stride-velocity* words: FU77 (§1, carried) establishes they ramp toward the flat `0x10F680` row, and a fresh `read_memory 0x10F680` (2560 B, 1280 words) gives **min −15 / max +15**. The index is `(desired & 0x3F0) | stride ≤ 0x3FF < 0x500`, so the read always lands inside the scanned table. Hence the live no-slot seed is **|s| ≤ 0x3C**, `v0 = trunc(w/3) ∈ [−5, 5]`, and **`T ≤ 0x90` per axis — never the 0x174**. The FU316 §4.3 clause "fewer pans for larger no-slot seeds" is therefore **inverted**: `T = 0x90` is the *maximum*, so `floor(0x730/T)+1 = 13` pans (x) / `20` (z) is the **floor** over the reachable domain, and every smaller seed needs strictly more pans (49/75 at `v0 = 2`, 154/238 at `v0 = 1`).

**Port verdict: parity** on every seed/`v0` staging cell and on the `v0`/decay/travel arithmetic (§6). Three latent, non-observable-in-window items are named as legs (int16 camY narrowing, the single-slot dir model for the *slot* arm, the hardcoded `0x1104AB` rate pair); the carried FU316-L3/L4 port gaps are re-affirmed, not re-opened. The phase-33 "silent bind" is algebra, not an accident: `w = ±1 → s = ±4 → v0 = 0 → T = 0`, and the event still binds the tracked cell.

## 3. The arm, fresh bytes

```asm
; 0x7EFEE (the tracked exclusion) .. 0x7F0D1 (the event set)
0x7efee  3b2dca771500     CMP EBP,dword[0x1577CA]   ; the camera-tracked record
0x7eff4  0f84dc000000     JZ 0x7F0D6               ; tracked -> skip the event, still the tail
0x7effa  833d5077150000   CMP dword[0x157750],0x0   ; *** camY ***
0x7f001  7432             JZ 0x7F035               ; camY == 0 -> the slot test
0x7f003  8b858b000000     MOV EAX,dword[EBP+0x8b]  ; >>24 = byte +0x8E (type8)
0x7f009  c1f818           SAR EAX,0x18
0x7f00c  660fbe8034f31000 MOVSX AX,byte[EAX+0x10F334]   ; the type dir x table
0x7f014  c1e005           SHL EAX,0x5              ; << 5
0x7f017  6689442402       MOV word[ESP+0x2],AX     ; seed_x
0x7f01c  8b858b000000     MOV EAX,dword[EBP+0x8b]
0x7f022  c1f818           SAR EAX,0x18
0x7f025  660fbe803cf31000 MOVSX AX,byte[EAX+0x10F33C]   ; the type dir z table
0x7f02d  c1e005           SHL EAX,0x5
0x7f030  e96d000000       JMP 0x7F0A2
0x7f035  8b7520           MOV ESI,dword[EBP+0x20]  ; *** the slot sub-object pointer ***
0x7f038  85f6             TEST ESI,ESI
0x7f03a  741a             JZ 0x7F056               ; no slot -> the no-slot arm
0x7f03c  660fbe4620       MOVSX AX,byte[ESI+0x20]  ; slot dir x
0x7f041  c1e006           SHL EAX,0x6              ; << 6
0x7f044  6689442402       MOV word[ESP+0x2],AX
0x7f049  8b4520           MOV EAX,dword[EBP+0x20]
0x7f04c  660fbe4021       MOVSX AX,byte[EAX+0x21]  ; slot dir z
0x7f051  c1e006           SHL EAX,0x6
0x7f054  eb4c             JMP 0x7F0A2
0x7f056  8b3d4c771500     MOV EDI,dword[0x15774C]  ; *** cam X ***
0x7f05c  85ff             TEST EDI,EDI
0x7f05e  7c04             JL 0x7F064
0x7f060  89f8             MOV EAX,EDI
0x7f062  eb04             JMP 0x7F068
0x7f064  89f8             MOV EAX,EDI
0x7f066  f7d8             NEG EAX
0x7f068  3d30060000       CMP EAX,0x630            ; |cam_x| > 0x630 -> (0,0)
0x7f06d  7f12             JG 0x7F081
0x7f06f  a154771500       MOV EAX,[0x157754]       ; *** cam Z ***
0x7f074  85c0             TEST EAX,EAX
0x7f076  7d02             JGE 0x7F07A
0x7f078  f7d8             NEG EAX
0x7f07a  3d200a0000       CMP EAX,0xA20            ; |cam_z| > 0xA20 -> (0,0)
0x7f07f  7e0e             JLE 0x7F08F
0x7f081  31c0             XOR EAX,EAX              ; *** the (0,0) arm: both words cleared ***
0x7f083  6689442404       MOV word[ESP+0x4],AX
0x7f088  6689442402       MOV word[ESP+0x2],AX
0x7f08d  eb18             JMP 0x7F0A7               ; -> the event still fires
0x7f08f  668b4573         MOV AX,word[EBP+0x73]    ; *** the no-slot seed X: the stride velocity word ***
0x7f093  c1e002           SHL EAX,0x2              ; << 2 (16-bit result)
0x7f096  6689442402       MOV word[ESP+0x2],AX
0x7f09b  668b4575         MOV AX,word[EBP+0x75]    ; *** the no-slot seed Z ***
0x7f09f  c1e002           SHL EAX,0x2
0x7f0a2  6689442404       MOV word[ESP+0x4],AX
0x7f0a7  8b542402         MOV EDX,dword[ESP+0x2]   ; (seed_x,seed_z)
0x7f0ab  8b0424           MOV EAX,dword[ESP]
0x7f0ae  c1fa10           SAR EDX,0x10             ; seed_z
0x7f0b1  c1f810           SAR EAX,0x10
0x7f0b4  31c9             XOR ECX,ECX
0x7f0b6  e8adeb0000       CALL 0x8DC68             ; the octagonal distance -> word[ESP]
0x7f0bb  ba1d000000       MOV EDX,0x1D             ; event code 0x1D
0x7f0c0  66890424         MOV word[ESP],AX
0x7f0c4  89e8             MOV EAX,EBP
0x7f0c6  31db             XOR EBX,EBX
0x7f0c8  e853370100       CALL 0x92820             ; the sound/presentation sink
0x7f0cd  89e2             MOV EDX,ESP
0x7f0cf  89e8             MOV EAX,EBP
0x7f0d1  e8be2bffff       CALL 0x71C94             ; *** the event set + tracked bind ***
```

The triple handed to `0x71C94` is **`{dist, seed_x, seed_z}`**: `0x71CC8 MOVSD` copies the dword at `[EDX]` (= `dist | seed_x<<16`) to `0x1577B8`, `0x71CC9 MOVSW` copies `[EDX+4]` (= `seed_z`) to `0x1577BC`. Fresh `0x71C94` body:

```asm
0x71c99  803d6c7a150000   CMP byte[0x157A6C],0      ; the suspension gate
0x71ca0  0f8549010000     JNZ 0x71DEF               ; -> no clear, no bind
0x71cbc  89d6             MOV ESI,EDX
0x71cbe  bfb8771500       MOV EDI,0x1577B8
0x71cc3  e82ce4ffff       CALL 0x700F4              ; the clear
0x71cc8  a5               MOVSD ES:[EDI],ES:[ESI]   ; 0x1577B8 = dist | seed_x<<16
0x71cc9  66a5             MOVSW ES:[EDI],ES:[ESI]   ; 0x1577BC = seed_z
0x71ccb  0fbfc3           MOVSX EAX,BX              ; the height arg (EBX = 0 here)
0x71cce  8b1550771500     MOV EDX,dword[0x157750]   ; camY
0x71cd4  66891df0771500   MOV word[0x1577F0],BX     ; event_param = height
0x71cdb  39d0             CMP EAX,EDX
0x71cdd  7d0c             JGE 0x71CEB
0x71cdf  66a150771500     MOV AX,[0x157750]         ; param = max(param, camY)
0x71ce5  66a3f0771500     MOV [0x1577F0],AX
0x71ceb  a1ee771500       MOV EAX,[0x1577EE]
0x71cf0  c1f810           SAR EAX,0x10
0x71cf3  3d40060000       CMP EAX,0x640
0x71cfa  66c705f07715004006 MOV word[0x1577F0],0x640 ; the 0x640 clamp
0x71d03  85ed             TEST EBP,EBP
0x71d05  7459             JZ 0x71D60               ; EBP == 0 -> no bind
0x71d1d  892c85ce771500   MOV [EAX*4+0x1577CE],EBP
0x71d27  892dca771500     MOV [0x1577CA],EBP       ; *** the bind ***
```

`FUN_00070544` then derives the velocity. The fast path (fresh bytes):

```asm
0x705f0  be06000000       MOV ESI,0x6              ; F2 = 6
0x705f5  bf0c000000       MOV EDI,0xC              ; F4 = 0xC (timer_limit)
0x705fc  31c2             XOR EDX,EAX
0x705fe  66890df0771500   MOV word[0x1577F0],CX    ; height 0
0x70605  668935f2771500   MOV word[0x1577F2],SI    ; F2
0x7060c  66893df4771500   MOV word[0x1577F4],DI    ; F4
0x70613  66893df8771500   MOV word[0x1577F8],DI    ; *** F8 = 0xC, the v0 divisor ***
0x7061a  66890dfa771500   MOV word[0x1577FA],CX    ; timer = 0
0x70621  668915f6771500   MOV word[0x1577F6],DX    ; F6 = 0
0x70628  660fb6351b781500 MOVZX ESI,byte[0x15781B] ; *** k = 24 (the slow-path decay) ***
0x70630  66a1f8771500     MOV AX,[0x1577F8]
0x70638  6685c0           TEST AX,AX
0x7063b  742f             JZ 0x7066C
0x7063d  803d2178150000   CMP byte[0x157821],0     ; the pan counter
0x70644  7f26             JG 0x7066C               ; > 0 -> the decay path
0x70646  0fbff0           MOVSX ESI,AX             ; ESI = F8
0x70649  a1b8771500       MOV EAX,[0x1577B8]       ; {dist, seed_x}
0x7064e  56               PUSH ESI
0x7064f  c1f810           SAR EAX,0x10             ; seed_x
0x70652  50               PUSH EAX
0x70653  e896240300       CALL 0xA2AEE             ; *** trunc(seed_x / 0xC) ***
0x7065b  66a3c0771500     MOV [0x1577C0],AX        ; vel_x
0x70661  a1ba771500       MOV EAX,[0x1577BA]       ; {seed_x, seed_z}
0x70666  56               PUSH ESI
0x70667  c1f810           SAR EAX,0x10             ; seed_z
0x7066a  eb5f             JMP 0x706CB              ; -> trunc(seed_z / 0xC) -> 0x1577C2
```

`0xA2AEE` is a plain signed divide (fresh decompile + bytes: `MOV EAX,[ESP+4]; CDQ; MOV EBX,[ESP+8]; IDIV EBX; RET`) — **truncation toward zero**, not floor. The slow path is `0x7066C..0x706D9`: `if (vel_x != 0) vel_x = trunc(k·vel_x/0x20); if (vel_z != 0) vel_z = trunc(k·vel_z/0x20)` — the alias traps matter here: `dword[0x1577BE]>>16` is `vel_x` and `dword[0x1577C0]>>16` is `vel_z`. `k = [0x15781B] = 24`, derived in `0x700F4` from the pair at `0x1104AB` (fresh `read_memory 0x1104AB` = `0a 10 09 0f 08 0e 07 0d`): decay `10`, hi `16` → `lo = 16 + (0x20−16)/2 = 24` (`0x7012B..0x70146`).

## 4. The algebra

### 4.1 The seed switch (A1)

| arm | gate (fresh) | seed x | seed z | live value | `v0` | `T_axis` |
|---|---|---|---|---|---|---|
| type dirs | `camY != 0` (`0x7EFFA`, dword) | `(int16)((int8)0x10F334[type] << 5)` (`0x7F003`) | `(int16)((int8)0x10F33C[type] << 5)` (`0x7F01C`) | `0x10F334` = `{0,1,1,1,0,−1,−1,−1}`; `0x10F33C` = `{1,1,0,−1,−1,−1,0,1}` → `±0x20` (`±0x20,±0x20` diagonal) | `±2` | `0x26` (38) |
| slot dirs | `camY == 0 && [rec+0x20] != 0` (`0x7F035`) | `(int16)((int8)slot[+0x20] << 6)` | `(int16)((int8)slot[+0x21] << 6)` | `±0x40` (unit dirs) | `±5` | `0x90` (144) |
| **no-slot** | `camY == 0 && [rec+0x20] == 0 && \|cam_x\| <= 0x630 && \|cam_z\| <= 0xA20` (`0x7F056`) | `(int16)(word[+0x73] << 2)` | `(int16)(word[+0x75] << 2)` | **`±0x3C` max** (§4.2) | `±5` max | **`0x90` max** |
| else | the no-slot gate fails (`0x7F081`) | `0` | `0` | `(0,0)` | `0` | `0` |

All three sets are `(int16)`-wrapped at the store (`MOV word[ESP+2],AX` / `[ESP+4]`), and the pair is consumed sign-extended (`SAR 0x10`). The `type8` index is `byte[rec+0x8E]` (`dword[EBP+0x8B] SAR 24`), read `MOVSX AX` from the 8-byte tables — the native indexes unchecked; the live type domain is the 8 kick-dir octants.

### 4.2 The seed word domain (A2)

`FUN_0008E244`/FU77 (carried, FU77 §1 lines 19-21) establishes that `+0x73/+0x75` are the record's **stride-velocity** words, ramped toward the flat `0x10F680` row `(desired & 0x3F0) | stride`, with the stride nibble in `[1, 0xF]` (FU77 §1, the `+0x7B & 0xF` base and the min-1 clamp). Fresh `read_memory 0x10F680` (0x500 words = 2560 B): **min −15, max +15** (445 negative, 660 positive, 175 zero). The index is therefore `≤ 0x3FF`, so the locomotion target always lies inside the scanned table.

Therefore, for the no-slot arm:

```
s = 4·w,  w = word[+0x73] ∈ [−15, +15]  ->  s ∈ [−0x3C, +0x3C]
v0 = trunc(4·w / 0xC) = trunc(w / 3)  ->  v0 ∈ [−5, +5]
```

The identity `trunc(4w/12) = trunc(w/3)` holds for both signs (C truncation, `IDIV`): `w = −4 → −1`, `w = −6 → −2`.

### 4.3 `T(s)` — the decay sum (A3)

`T(v0) = 2·(6·v0 + 7·v1 + 7·v2 + …)`, `v_{i+1} = trunc(0x18·v_i / 0x20)`, stop at 0. The 6/7 cadence is `F4 = 0xC` + delta 2 (`0x705FA`/`0x70613`, `0x737BF` compare): six timer increments reach the limit, the seventh frame is the re-arm and integrates the new velocity.

| `v0` | chain | `T` | pans to x-armer `>0x730` | pans to z-armer `>0xB20` |
|---|---|---|---|---|
| 0 | 0 | **0** | — | — |
| 1 | 1 | **12** (0xC) | 154 | 238 |
| 2 | 2,1 | **38** (0x26) | 49 | 75 |
| 3 | 3,2,1 | **78** (0x4E) | 24 | 37 |
| 4 | 4,3,2,1 | **132** (0x84) | 14 | 22 |
| 5 | 5,3,2,1 | **144** (0x90) — measured | **13** | **20** |
| 6 | 6,4,3,2,1 | 212 | 9 | 14 |
| 7 | 7,5,3,2,1 | 238 | 8 | 12 |
| 8 | 8,6,4,3,2,1 | 320 | 6 | 9 |
| 9 | 9,6,4,3,2,1 | 332 | 6 | 9 |
| 10 | 10,7,5,3,2,1 | **372** (0x174) — *unreachable seed* | 5 | 8 |
| 25 (the `0x19` clamp) | 25,18,13,9,6,4,3,2,1 | 1084 (0x43C) | 2 | 3 |

Rows `v0 = 6…25` are **not reachable from the no-slot arm** (they need `|w| ≥ 18`); they are tabulated so the armer arithmetic can be checked and so the `0x19` clamp has a known shape. `T(0x40) = 0x90` is the measured instance (`tests/test_engine_m2.c:4116-4125`: `max_pos_x == 0x120` after two pans, `vel_seq` `5,3,2,1`); `T(0x80) = 0x174` is arithmetic-consistent but **no measurement covers it and the no-slot arm cannot produce it**.

For a no-slot pan the delta is a **vector**, not a scalar: `Δx = sign(w_x)·T(v0_x)`, `Δz = sign(w_z)·T(v0_z)`. The octagonal `speed` (`0x706F9`) only gates the integrator and the clamp; it never scales the travel. An axis with `w = 0` never moves. This refines FU316 §4.3's `place += T(s)·sign` (which is the axis-aligned slot/type case).

### 4.4 A2 detail — the word → `v0` → `T` map for the live domain

```
w : −15 −14 −13 −12 −11 −10  −9  −8  −7  −6  −5  −4  −3  −2  −1   0
v0:  −5  −4  −4  −4  −3  −3  −3  −2  −2  −2  −1  −1  −1   0   0   0
T : 144 132 132 132  78  78  78  38  38  38  12  12  12   0   0   0
w :  +1  +2  +3  +4  +5  +6  +7  +8  +9 +10 +11 +12 +13 +14 +15
v0:   0   0  +1  +1  +1  +2  +2  +2  +3  +3  +3  +4  +4  +4  +5
T :   0   0  12  12  12  38  38  38  78  78  78 132 132 132 144
```

The seed quantisation is coarse: **three consecutive velocity words share one travel step**. `|w| ≤ 2` is a pure silent bind; `|w| = 3..5` is a 12-unit creep; only `|w| = 15` reaches the measured `0x90`.

### 4.5 The `0x730` armer reach (A4)

Fresh `0x73B70..0x73B9B`: the boundary call is `if (|x| > 0x6C0 || |z| > 0xAB0) FUN_0007131C(...)`; fresh `0x71390..0x713C6`: the armer latches when `phase ∈ {2, 0x10}` **and** (`|z| > 0xB20` **or** `|x| > 0x730`), both strictly `>`. So:

```
pans_to_arm_x(T) = floor(0x730 / T) + 1        (T = 0x90 -> 13; 12 pans = 0x6C0 exactly, no call)
pans_to_arm_z(T) = floor(0xB20 / T) + 1        (T = 0x90 -> 20; 19 pans = 0xAB0 exactly, no call)
```

**The floor over the reachable no-slot domain is 13 pans (x) / 20 (z), at `v0 = 5`.** FU316's `s = 0x80 → 5 pans` row is algebraically right and **not** a no-slot route. The 13th pan crosses `0x6C0` and `0x730` in the same boundary call (13·144 = 1872 = 0x750); for `T = 132` the boundary call first fires at pan 14 (1848), for `T = 78` at pan 23 (1794, no arm) with the arm at 24 (1872). The `0x7131C` body is unported (FU316-L6), and the reflect bit (input bit 0, default 0) mirrors at `|x| > 0x720` before the arm (FU316 §5.1, carried) — so the armer route stays a *recorded consequence*, never a phase-34 target.

## 5. The edge cases

1. **Sign / truncation.** `v0 = trunc(s/0xC)` via `IDIV` (toward zero), and the whole decay is truncating division by 32. A negative seed mirrors exactly: `s = −0x3C → v0 = −5 → −(5,3,2,1) → −0x90`. No asymmetry exists; `−1..−11` seeds give `v0 = 0` (the silent band is `|s| ≤ 0xB`, i.e. `|w| ≤ 2`).
2. **Width.** The seed store is 16-bit: `(int16)((uint16)w << 2)` would wrap at `|w| ≥ 0x4000` — outside the locomotion domain but reachable only through a non-locomotion writer (L1). The later divides are 32-bit `IDIV` on sign-extended operands; the velocity store truncates to 16 bits (`MOV [0x1577C0],AX`), which the port mirrors with `(int16_t)`.
3. **The parked ±1 case (phase 33's silent bind).** `w = ±1 → s = ±4 → v0 = 0` → both velocity words 0 → the `0x706DF TEST BX,BX` path skips the speed computation, and `0x700F4` had already zeroed `0x1577BE` (`0x70204 MOV word[0x1577BE],SI`, SI = 0) → `speed = 0` → the integrator's `0x737DF JZ 0x73B70` freezes the place. **The event still fires**: code `0x1D`, `0x92820`, `0x71C94` (the clear + the bind at `0x71D27`), and the tail installs rows 5/6 (`0x7F0D6..0x7F133`). The place never moves — measured in phase 33 (FU323 §2/§4) and reproduced by the algebra.
4. **The moving cases.** `w = ±3..±5 → v0 = ±1 → T = 12`; `±6..±8 → ±2 → 38`; `±9..±11 → ±3 → 78`; `±12..±14 → ±4 → 132`; `±15 → ±5 → 144`. A `T = 12` pan is six frames of ±1 world unit (delta 2) — a creep, not a pan: the phase's Layer-1 tuple (`|v0| ≥ 1`) is satisfied while the place moves only 12 units.
5. **The `(0,0)` else arm.** `|cam_x| > 0x630 || |cam_z| > 0xA20` → both seeds zero, **the event and the bind still run**. So the no-slot pass is never silent at the *tracked* level; it is silent only in the place. Important for the phase-33 reading: "silent bind" is the place, not the event.
6. **Diagonal seeds.** `w = (+15,−15) → v0 = (+5,−5)` → `Δx = +0x90, Δz = −0x90` per pan; both arms accumulate simultaneously. The octagonal `speed` of `(5,5)` is `6`, so the `0x19` clamp is untouched.
7. **The speed clamp.** `0x70704 CMP 0x19` rebuilds `(vel_x,vel_z)` from `0xCD474` (the atan octant) and `0x795A4(0x19, sine)` when `speed > 25` — it needs `|v0| > 25`, i.e. `|w| ≥ 77`. Unreachable from the stride table; only a non-locomotion writer could enter it (L1). The port's `fifa96_entity_angle` is the same octant family: fresh `decompile 0x8DD70` is a one-line `CALL 0xCD474`.
8. **The pan counter.** `0x709D0` caps `byte[0x157821]` at 100 but only tests `> 0` for the decay path, so the decay never changes at the cap; the port's `if (pan_counter < 100) pan_counter++` is the same.
9. **The height arm is not this arm.** The row-04 call passes `EBX = 0` (`0x7EFC4`/`0x7F0C6`), so `event_param = max(0, camY) = 0` at `camY == 0` → the idle constants `F2 = 6, F4 = 0xC, F6 = 0, F8 = 0xC`. Any non-zero camY would take the type arm and the height ramp; that is a different path.

## 6. The port-fidelity audit

Sources read fresh: `src/fifa96_loader/fifa96_outfield.c` (340-345, 610-721), `fifa96_camera.c` (30-62, 74-151, 726-830, 900-991), `src/fifa96_engine/fifa96_match_handlers.c` (2605-2827), `fifa96_match_run.c` (507-571, 1343-1348, 1457-1463), `fifa96_entity_update.c` (63-73, 211-229), `include/fifa96_loader/fifa96_outfield.h` (242-339), `fifa96_camera.h` (7-50).

| # | cell | native (fresh) | port | verdict |
|---|---|---|---|---|
| P1 | arm order / gates | `0x7EFFA` → `0x7F035` → `0x7F056` → `0x7F081` | `outfield.c:685` / `:689` / `:692` / `:683` | **parity** |
| P2 | type-dir packing | `MOVSX AX,[type+0x10F334]; SHL 5`, word store (`0x7F003..0x7F030`) | `outfield.c:687-688` `(int16_t)((int16_t)(int8_t)t[type8] << 5)` | **parity** (16-bit wrap kept) |
| P3 | slot-dir packing | `MOVSX AX,[slot+0x20]; SHL 6` (`0x7F03C..0x7F054`) | `outfield.c:690-691` | **parity**; dir source caveat **L3** |
| P4 | no-slot camera bounds | `NEG`/`CMP 0x630` then `CMP 0xA20`, 32-bit (`0x7F056..0x7F07F`) | `outfield.c:693-696` `int32` abs + `<= 0x630` / `<= 0xA20` | **parity** |
| P5 | no-slot seed pack | `MOV AX,word[+0x73]; SHL 2`, word store (`0x7F08F..0x7F09F`) | `outfield.c:697-698` `(int16_t)((uint16_t)vel_int_x << 2)` | **parity** |
| P6 | the `(0,0)` fallthrough | clears both, jumps to the event (`0x7F081..0x7F0A7`) | `outfield.c:683-684` init 0, falls to `:702` | **parity** |
| P7 | event code / dist | `EDX=0x1D`, `0x8DC68(ex,ez)` → `word[ESP]` (`0x7F0B4..0x7F0C8`) | `outfield.c:702-706` `code 0x1D`, `row04_metric(0,0,ex,ez)` | **parity**; `event_dist` is **dead** — the native overwrites `0x1577B8` at `0x707EB`, and the port has no consumer (`grep event_dist`) |
| P8 | the seed source words | `word[EBP+0x73]` / `word[EBP+0x75]` (`0x7F08F`/`0x7F09B`) | `handlers.c:2644-2645` `(int16)((uint32)r->vel_x >> 16)` / `r->vel_z`, composed at `match_run.c:567-570` / `:1345-1347` | **parity** (the dword views are bit-exact `{+0x71,+0x73}` / `{+0x73,+0x75}`) |
| P9 | the `camY` gate | `CMP dword[0x157750],0` (`0x7EFFA`), `CMP dword,0x50` (`0x7EDD8`) | `outfield.c:685` / `:618` on `ball_height`, staged `handlers.c:2707` `(int16_t)camera.pos_y` | value **parity**; **int16 narrowing** (L2) and a misleading field name (`ball_height` *is* the camera Y — `outfield.h:287`, FU-171 comment `handlers.c:2703-2707`) |
| P10 | `v0` divide | `MOVSX ESI,F8; SAR; CALL 0xA2AEE` (`IDIV`) `0x70646..0x7066A` | `camera.c:770-772` `vel = seed / f8` | **parity** (truncation toward zero) |
| P11 | decay scale | slow path `trunc(k·v/0x20)`, `k = [0x15781B] = 24` (`0x7066C..0x706D9`, `0x7012B..0x70146`) | `camera.c:774-776` `k = has_height ? 16 : 24`, `/0x20` | **parity** |
| P12 | fast/slow selector | `F8 != 0 && [0x157821] == 0` (`0x70638..0x70644`) | `camera.c:770` | **parity** |
| P13 | speed recompute | conditional on a non-zero velocity (`0x706DF`), else the word keeps the `0x700F4` clear (event path) or the prior frame's tail recompute (`0x73Bxx`, pan-step path) | `camera.c:779` unconditional | **parity in value** (both paths leave 0) |
| P14 | the `0x19` clamp | `0x70704..0x70798`, atan `0xCD474`, `0x795A4(0x19,·)`, `speed = 0x19` | `camera.c:784-791` | **parity**, unreachable (edge 7) |
| P15 | the clear | `0x700F4`: speed `0x70204`, seed pair `0x701D8/0x701FD`, vel pair `0x701EC/0x701E5`, counter `0x70153`, tracked `0x70258` | `camera.c:906-944` | **parity** (same cells, same order) |
| P16 | the bind | `0x71D27` only when `EBP != 0` (`0x71D05`) | `handlers.c:2796` unconditional | **parity in the live path**; carried **FU316-L4** (the suspension bail) |
| P17 | the tracked exclusion | `0x7EFEE CMP EBP,[0x1577CA]` | `outfield.c:682` `is_ball_track == 0` | **parity**; carried **FU316-L3** (`tracked == NONE` fallback blocks the controlled record only) |
| P18 | the integrator | `for i<delta: acc += vel` (16-bit), `pos += acc`, speed recompute (`0x737ED..0x73883`, `0x73Bxx`) | `camera.c:101-111`, `:148` | **parity** |
| P19 | the cadence constants | `F2=6,F4=0xC,F6=0,F8=0xC`, timer `+delta`, step when `timer > limit` (`0x705F0..0x70621`, `0x737BF`) | `camera.c:753-762`, `:87`, `:97` | **parity** |
| P20 | the rate pair | `0x1104AB` = `{10,16,9,15,…}`; `[0x15781B] = 16 + 8 = 24` | `camera.c:942-944` hardcodes 10/16/24 | **parity for the image defaults**; **L4** (a non-zero `[0x14C2FE]/[0x14C2FA]` is unproven) |
| P21 | the stored step pair | `0x1577BA/BC = vel·F8` (`0x707C4..0x707EB`) | `camera.c:793-794` | **parity** |
| P22 | the pan-step re-ramp | `FUN_00070544(0)` over the stored pair (`0x70B86`) | `camera.c:990` passes `event_step_x/z` | **parity** (the slow path scales the live vel; the argument is the stored pair exactly as the native re-reads it) |

**Consumers (grep, fresh).** `out.event_x/event_z` has exactly one consumer: `handlers.c:2786` → `fifa96_camera_event_set` (height 0, `ramp_param = out.event_track_reload`; the field is set only on the row-`0x13` arm, `outfield.c:681`, so the `0x1D` arm passes 0 — and on the idle-height path the param is inert anyway, since it only flips the `F6` sign in the `has_height` branch, `camera.c:749`). Inside the camera: the seed → `vel_x/vel_z` → `speed` → the integrator (`camera.c:101-111`) and the follow gate (`gate_speed`); `event_step_x/z` → the pan-step re-ramp (`:990`) and the record-event correction (`:1098-1099`); `speed` → `fifa96_camera_event_dir`'s zero test (`:867`). **No other consumer of the seed exists** — the tracked cell, the anchors and the Y arc never read `event_x/z`.

**Verdict: parity, no measured divergence.** There is no seed-staging value the native computes and the port computes differently over the whole live domain (`|w| ≤ 15`, `camY ∈ [0, 0x640]`, `|cam_x| ≤ 0x630`, `|cam_z| ≤ 0xA20`). The three named divergences are latent (L2, L3, L4) and none of them can move a phase-34 frame.

**Test coverage today.** The *loader* no-slot staging is pinned (`tests/test_outfield.c:910-918`: `vel_int (3,−2)` → `event_x 12`, `event_z −8`, `dist 0x0F`, code `0x1D`). The *camera* `v0`/decay is pinned on the measured slot seed (`tests/test_camera.c`; `tests/test_engine_m2.c:4116-4125`). **Nothing pins a no-slot seed crossing the loader→camera boundary** — that is the missing pin the Stage-2 plan needs if it lands a witness (handed to FU329).

## 7. Numbered legs

* **FU326-L1 — the non-locomotion `+0x73/+0x75` writers.** The `±15` bound (§4.2) is the *locomotion* target. Fresh `search_instructions` finds 87 `[+0x73]` and 51 `[+0x75]` operands across all widths; the word writers outside `FUN_0008E244`/`FUN_0007BF20`/`FUN_00076B28` (e.g. `0x80B5E`, `0x80D18`, `0x81E45`, `0x825E7`, `0x865C0`, `0x86F6C`, `0x87486`, `0x87515`, `0x875C9`, `0x87641`, `0x876B6`, `0x87727`) are handler-local and their reachability at the row-04 gate is unproven. If any of them can leave `|w| > 15` at the gate, `v0 > 5` and `T > 0x90` become possible; the armer floor of 13 drops accordingly. Owner: `fifa96_outfield_row04_step`'s `vel_int` staging + a native writer census; settle with FU327's measured `|seed|` distribution (it samples exactly this word).
* **FU326-L2 — the `camY` width.** Native `0x7EFFA`/`0x7EDD8` compare the **32-bit** `0x157750`; the port stages `(int16_t)camera.pos_y` (`handlers.c:2707`) into an `int32` field, so `|Y| > 0x7FFF` sign-flips both gates. Unreachable in the live window (`Y ∈ [0, 0x640]`); latent for any future Y producer. Owner: `fifa96_match_action_04` staging.
* **FU326-L3 — the slot-arm dir source (out of the no-slot path).** The native reads the *record's own* slot sub-object bytes `[+0x20]/[+0x21]`; the port models one `mr->slot` and falls back to `e->dir_x` when `mr->slot.entity != id` (`match_run.c:1457-1463`). For a `has_slot` record that is not the current slot holder the two can differ. Dead on the no-slot arm (`has_slot == 0`), so it cannot affect phase 34; recorded for the slot arm. Owner: `fifa96_match_run` slot model.
* **FU326-L4 — the rate-pair table.** The port hardcodes the `0x1104AB` image defaults (`camera.c:942-944`); the native indexes `0x1104AB + [0x14C2FE]·8 + [0x14C2FA]·2` (`0x700FC..0x70119`). Fresh `read_memory 0x1104AB` confirms the first pair `(10,16)`, but no writer of the two index words is proven dead. If either is ever non-zero, `pan_decay`/`k` change and so does `T(s)`. Owner: `fifa96_camera_event_set` + the `0x14C2FE`/`0x14C2FA` cells.
* **Carried, re-affirmed (not re-opened):** FU316-L3 (`tracked == NONE` fallback), FU316-L4 (the bind outside the suspension guard), FU316-L6 (the `0x7131C` body), FU316-L8 (the row-05 rebinder). None of them touches the no-slot seed staging; L3/L8 do touch *whether* a no-slot record can re-enter row 04 for a second pan.

## 8. GO / NO-GO

**GO — the algebra and the port parity.** Every packing term, the `v0` divide, the decay, the cadence and the integrator are first-hand native bytes with a matching port expression (§6, 22 cells, all parity). The phase-33 silent bind is explained exactly (`w = ±1 → s = ±4 → v0 = 0`), and the port reproduces it for the same reason. No seed-staging change is warranted; **the Stage-2 plan's "no production change unless the audit finds a divergence" clause is satisfied with no divergence.**

**NO-GO — the carried "larger no-slot seed" premise.** FU316 §4.3's `s = 0x80 → T = 0x174` (and the spec §4 "fewer pans for larger no-slot seeds") is **not reachable from the no-slot arm**: the seed is the stride velocity word, bounded to `±15` by the `0x10F680` table, so `|s| ≤ 0x3C` and `T ≤ 0x90`. Consequences for phase 34, stated so the wave can re-scope:

1. The no-slot pan is **never longer than the measured slot pan**; `0x90` is the ceiling, `12` the common case at `|w| = 3..5`.
2. The armer floor is **13 pans (x) / 20 (z)** at `T = 0x90`; smaller seeds need strictly more (49/75 at `v0 = 2`; 154/238 at `v0 = 1`). A no-slot witness is therefore *harder* than the slot-arm witness, not easier — FU328's sweep should look for `|v0| ≥ 1` at any `T`, and must not expect `place > 0x120` from a single no-slot pan.
3. The spec §6 Layer-1 tuple (`has_slot 0`, `|v0| ≥ 1`, the place delta = `T(s)`) remains sound, but `T(s)` must be read as `T_axis ∈ {0xC, 0x26, 0x4E, 0x84, 0x90}` and the delta as a **vector**, not a scalar.

## 9. Known limits

* **Static only.** No run, no trace, no backward staging; the phase-33 "place never moves" is quoted (FU323), not re-measured. The `v0` distribution over the natural window is FU327's.
* **The `0x7131C` body is unread here.** The armer's consequence (the situation-2 throw-in and the place reset) is carried from FU316 §5; only the gate and the boundary-call thresholds are fresh (§4.5).
* **The reflect bit.** `|x| > 0x720` mirrors before the arm when the input bit 0 is set; the port's `fifa96_camera_reflect` is called only after `goal_armed` (FU316 review erratum), so this is not a no-slot-seed question. Recorded, not settled.
* **The `0x10F680` bound assumes the locomotion is the last writer before the gate.** L1 owns the escape.
* **`T(v0)` is arithmetic + the measured `0x40` instance.** No measurement exists at any other seed; the 6/7 cadence is derived from `F4 = 0xC` + delta 2 and corroborated by the measured `5,3,2,1` frame counts (FU316 erratum) — not independently traced.
* **The type-dir arm's live domain** (`type8 ∈ [0,7]`) is inferred from the 8-byte native tables (`0x10F334`/`0x10F33C`, fresh reads). The port's arrays are 32-entry image copies (`match_handlers.c:951-958`); entries 0..15 match the fresh native bytes, 16..31 are unverified here. Out of this slice (the no-slot arm never reads the tables).

## 10. Self-audit

| claim | citation |
|---|---|
| the three seed arms, their gates and packing | fresh `disassemble_bytes 0x7EEC0..0x7F140` (§3) |
| the row-04 gate reaching the switch (cone, bound, `camY <= 0x50`, `EBP != tracked`) | fresh `0x7ED80..0x7EF00`; port `outfield.c:610-682` |
| the triple `{dist, x, z}` and the bind | fresh `0x71C94..0x71D5F` + `decompile 0x71C94` |
| `v0 = trunc(seed/0xC)` toward zero | fresh `0x70646..0x7066A` + `0xA2AEE` bytes (`CDQ; IDIV`) |
| the clear zeroes speed, both seed words, both vel words | fresh `0x700F4..0x70205` (`0x701D8`, `0x701FD`, `0x701EC`, `0x701E5`, `0x70204`) |
| the decay `k = 24` and the `0x1104AB` pair | fresh `0x7012B..0x70146` + `read_memory 0x1104AB` = `0a 10 …` |
| `T(v0)` and the 6/7 cadence | fresh `0x705F0..0x70621` (F2/F4/F6/F8), `0x737BF` (timer compare) + the measured `5,3,2,1` (`test_engine_m2.c:4116-4125`) |
| the seed-word domain `±15` | fresh `read_memory 0x10F680` (1280 words, min −15, max +15) + FU77 §1 (the ramp target) |
| the type tables | fresh `read_memory 0x10F334` / `0x10F33C` (8-byte unit dirs) |
| the `0x730` / `0xB20` armer gate | fresh `0x71388..0x713CF` |
| the `0x6C0` / `0xAB0` boundary call | fresh `0x73B60..0x73B9F` |
| the port's staging cells and values | `outfield.c:685-706`, `handlers.c:2644-2645`, `:2703-2710`, `match_run.c:567-570`, `:1345-1347`, `:1457-1463` |
| the port's `v0` / decay / integrator | `camera.c:770-779`, `:753-762`, `:101-111`, `:148`, `:900-957` |
| `fifa96_entity_distance` = `0x8DC68` | `entity_update.c:211-229` vs fresh `decompile 0x8DC68`; `row04_metric` at `outfield.c:340-345`; the pin `test_outfield.c:918` (`0x8DC68(12,8) = 0x0F`, recomputed) |
| `fifa96_entity_angle` = the `0xCD474` family | fresh `decompile 0x8DD70` (a single `CALL 0xCD474`); `entity_update.c:75-…` |
| the measured pan `0x90` / `0x120` / `5,3,2,1` | `tests/test_engine_m2.c:4116-4125` (carried as the measurement; not re-run) |
| the live frame delta = 2 | `tests/test_engine_match_input.c:339-341`, `tests/test_engine_match_frame.c:75` |
| the phase-33 silent bind | FU323 §2/§4 (carried) + the algebra (§5.3) |
| FU316's `s = 0x80 → 0x174` is arithmetic-only | §4.3 recomputation; FU316 §7 L1 states no measurement covers it |

Status: recon complete; **this draft is the only file written**; no census change (60/80); no golden touched; no git; no binary run.

## Review erratum (wave review, 2026-10-23)

**1. The zero-seed speed mechanism (§5 item 3; §6 P13).** Quote (§5 item 3): "both velocity words 0 → the `0x706DF TEST BX,BX` path skips the speed computation, and `0x700F4` had already zeroed `0x1577BE` ... → `speed = 0`." **Fresh this review (`disassemble_bytes`):** false for the event path. The fast path sets the flag unconditionally — `0x706D1 MOV EBX,0x1` after the z divide (`0x706CB..0x706D9`); `0x70636 XOR EBX,EBX` is only the pre-branch init. So `0x706DF TEST BX,BX` sees `BX = 1` and **recomputes** `0x1577BE` at `0x706FE` from `(vel_x,vel_z) = (0,0)`, giving 0. The skip arm (`0x706E2 JZ 0x7079F`) is reachable only on the slow (pan-step) path when **both input velocity words were already zero**; in that case the prior value is already 0. P13's verdict (frame-end parity) still stands — the integrator tail also recomputes `0x1577BE` at `0x73CB8` — but its "else the word keeps the `0x700F4` clear (event path)" clause and the §5 item 3 "skips" explanation are wrong. The phase-33 silent bind's conclusion (`speed = 0`, place frozen) is unchanged.

**2. The scope of the seed bound (one-page answer; §8).** Quote: "**But the no-slot arm cannot reach `s = 0x80`.**" and "`s = 0x80 → T = 0x174` ... is **not reachable from the no-slot arm**." **Corrected:** the proof is the **locomotion writer's** domain: `FUN_0008E244`/`FUN_0007BF20` ramp `+0x73/+0x75` toward `0x10F680` (fresh read: 1280 words, min −15, max +15, 445/660/175 — exact), so `|s| ≤ 0x3C`, `|v0| ≤ 5`, `T ≤ 0x90` **for that writer**. The 87/51 non-locomotion `+0x73/+0x75` operands are FU326-L1; their reachability at the row-04 gate is unproven, so a static "cannot reach 0x80" is **not** established. The measured natural tape is stronger for the wave's purpose: `|w| ≤ 1` on every record-frame (FU327 §4.4), and the phase-34 NO-GO is measured, not static. State the bound as "the locomotion/live measured domain", not as an impossibility.

**3. Prior authority for the bound (§8).** Quote: "FU316 §4.3's `s = 0x80 → T = 0x174` ... is **not reachable from the no-slot arm**". **Missing cite:** FU321 §3.5 (frozen phase-32) already carried the same `|w| ≤ 15` bound and the same conclusion ("the no-slot arm's maximum is the same `0x90`"; `|w| ≤ 2` the zero seed). The "inversion" is a reconciliation of FU316 §4.3/§7.1 with **FU321 §3.5**, now corroborated by the fresh native read — not a new discovery. The spec §4 premise was already contradicted by a frozen phase-32 doc; the controller's spec erratum should cite both.

**4. The `0x19` clamp bound (§5 item 7).** Quote: "it needs `|v0| > 25`, i.e. `|w| ≥ 77`." **Corrected:** the trigger is the octagonal `speed > 0x19` (`0x70704 CWDE; CMP EAX,0x19`), not a per-axis `|v0| > 25`. For an axis-aligned seed, `|v0| ≥ 26` → `|w| ≥ 78` (`w = 78 → s = 0x138 → v0 = 26`); `|w| = 77 → s = 0x134 → v0 = 25`, speed 25, no clamp. A diagonal pair can trip the clamp at smaller components. Unreachable from the no-slot domain either way (`|v0| ≤ 5`), so the verdict is unaffected.

**5. The `T = 12` wording (§5 item 4).** Quote: "A `T = 12` pan is six frames of ±1 world unit (delta 2)". **Corrected:** the velocity word is ±1 and the integrator applies `delta = 2`, so the place steps **2 units/frame** for six frames (total 12); it is not 1 place unit/frame. (FU327 §4.4's correction has the same point.)
