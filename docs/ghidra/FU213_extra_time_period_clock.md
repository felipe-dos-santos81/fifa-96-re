# FU-213 — the extra-time period / half / clock machinery: `FUN_0008AF38`, the period-index roll, the chooser and the end-of-match word (phase-19 wave R61)

Provenance: M2 phase-19 wave **R61**, 2026-10-13, read-only. Authoritative program
**`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program argument;
confirmed this session by `get_current_program_info` — `FIFA96.EXE`,
`watcom:LE:32:default`, `image_base 0`, `min_address 0x10000`,
`max_address .image::00174a2a`, 2706 functions, `.image` overlay present). Every
address/byte/xref-count/instruction below is a fresh `disassemble_function` /
`disassemble_bytes` / `read_memory` / `get_xrefs_to` of **this** session unless
marked *cross-cited* (then the frozen FU doc is named). Engine/loader source read
first-hand (`src/fifa96_engine/fifa96_match_run.c`,
`src/fifa96_engine/fifa96_match_handlers.c`,
`src/fifa96_loader/fifa96_action_handlers.c`,
`src/fifa96_loader/fifa96_match_state.c`, `include/fifa96_engine/fifa96_match_run.h`).
The alias traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at
`x+3`); LE globals sit at `+0x100000` from the file offset; addresses are
`/FIFA96.EXE` program (runtime) addresses. **No writes**: no rename, comment,
label, function, script or project save; `/FIFA96.EXE` untouched; the ISO /
`game/hdd/` untouched; no git commands. Model: FU-206.

Scope (R61 brief): derive the surrounding **extra-time period / half / clock
machinery** the phase-19 cohort depends on — (1) the period-index roll
(`[0x157AC2]`, the period-4 skip); (2) the extra-time half start/end; (3) the
per-frame clock `FUN_0008AF38` (its period-completion detection, the score
tiebreak, the `[0x157AC0]` set); (4) the end-of-match logic. Map every native
site to the engine's `fifa96_match_run_period_setup` (and the clock/period
machinery it shares) and state exactly what the engine must extend (function +
line region) and which pieces are honestly legs (no reachable producer). This
extends FU-143 §3.3/§4, FU-195 §3/§4, FU-202 §4, FU-207, FU-208, FU-209, FU-211.

---

## 1. The cells — fresh map (widths, roles, engine cells)

| cell | width | role (fresh) | engine cell |
|---|---|---|---|
| **`0x1106AD`** | byte × 0x30 | the **phase clock-class table** (`DI = byte[0x1106AD + phase]`; 0/1/2) | `fifa96_match_state_class_table[0x30]` (`fifa96_match_state.c:4..8`) — **byte-identical** |
| **`0x157A64`** | word | the granted-frame delta | `state.frame_delta` |
| **`0x157A68`** | word | the frame accumulator (`+= [0x157A64]` every clock frame) | `state.tick_total` |
| **`0x157AC1`** | byte | the **1/60 sub-second** accumulator (0..0x3B) | `state.second_acc` |
| **`0x157AB6`** | word | the **period seconds** (`sec`) | `state.period_seconds` |
| **`0x157AB4`** | word | the per-second **total counter** (sec 0.. limit) | `state.total_seconds` |
| **`0x157ABA`** | word | the **aux seconds** (`aux`, the stoppage add-on) | `state.aux_seconds` |
| **`0x157ABC`** | word | the **aux tick** (0..0x1E) | `state.aux_tick` |
| **`0x157AC2`** | byte | the **period index** (0..4; the sit-8/ET selector) | `struct … .global_157ac2` (`fifa96_match_run.h:610`) |
| **`0x157AC0`** | byte | the **OL-85 extra-time flag** (0/1) | **none** (FU-207 §1) |
| **`0x15881A`** | word | the **regulation period limit** (60 s reset) | `state.period_length` |
| **`0x15881C`** | word | the **extra-time period limit** (30 s reset) | `state.extra_length` |
| **`0x15882D`** | byte | the **last-minute/overtime display flag** | none |
| **`0x158822`** | word | the **match-over word** (1 reg, 2 ET) — the harness return | **none** |
| **`0x14C302`** | dword | the class-2 **clock-halt** gate | `clock_halt` always 0 (leg) |
| **`0x14C2EE`** | dword | the **force-extra-time override** (W3) | none |
| **`0x14C1D0`** | byte | the **OL-85 else-arm config** cell | none |
| **`0x10F378`** | dword | the **no-extra-time latch** | none |
| **`0x10F360`** | dword | the **ET-over latch** (phase 0x13/0x14) | none |
| **`0x8AF28`** | dword × 4 | the **period-index jump table** | none (the FU-62 clock) |

Fresh `read_memory 0x1106AD` (48 B) =
`02 00 01 02 02 00 02 02 02 02 00 00 00 02 00 00 00 00 00 00 00 00 00 01 00 00
01 00 01 01 01 00 …` — **byte-identical** to the engine's
`fifa96_match_state_class_table` (§7). Fresh `read_memory 0x8AF28` (16 B) =
`dd b1 08 00 2f b2 08 00 e9 b2 08 00 3f b3 08 00` =
**`{0x8B1DD, 0x8B22F, 0x8B2E9, 0x8B33F}`** (period index 0..3).

---

## 2. `FUN_0008AF38` — the per-frame clock harness (fresh `disassemble_function`, 475 insns, `0x8AF38..0x8B687`)

`FUN_0008AF38` is the **per-frame match clock**, called once per granted frame
from the frame loop `FUN_0004B100` (`0x4B1A6`) and from `0x7440B` (fresh
`get_xrefs_to 0x8AF38` = **2**). It returns `AX = [0x158822]` (the match-over
word, §5). The frame loop calls it unconditionally (`0x4B1A6 CALL 0x8AF38`);
the preceding `[0x14C32A]` check only selects an extra `0x948AC` call on the
open-gate arm (`0x4B198 CMP byte[0x14C32A],0; 0x4B19F JZ 0x4B1A6; 0x4B1A1 CALL
0x948AC`).

### 2.1 The class gate (`0x8AF41..0x8AF80`)

```
0x8AF41 EAX = [0x157A4A] >> 24            ; = byte[0x157A4D] (CURRENT phase)
0x8AF49 XOR ECX,ECX
0x8AF4B DI  = byte[EAX + 0x1106AD]        ; class = table[phase]
0x8AF53 AL  = byte[EAX + 0x1106AD]
0x8AF5E CMP EAX,0x2 ; 0x8AF61 JNZ 0x8AF73
0x8AF63 CMP dword[0x14C302],0 ; 0x8AF6A JNZ 0x8AF73
0x8AF6C MOV EAX,1 ; JMP 0x8AF75
0x8AF73 XOR EAX,EAX
0x8AF75 MOVSX EDX,DI ; CMP EDX,0x1 ; 0x8AF7B JZ 0x8AF86   ; class==1 -> CLOCK RUNS
0x8AF7D TEST AX,AX ; 0x8AF80 JZ 0x8B590                   ; else (not class2||halt) -> TAIL
```

So the clock body runs iff `class == 1` (always) **or** `class == 2` **and**
`[0x14C302] == 0`. Class `0` never runs; class `2` with `[0x14C302] != 0` halts.
`CX = 0` is the "period completed" flag initialised here.

### 2.2 The clock body (`0x8AF86..0x8B3AC`) — one second per loop iteration

```
0x8AF86 AX = word[0x157A64]               ; frame delta
0x8AF8C word[0x157A68] += AX              ; the frame accumulator
0x8AF95 EAX = [0x157A4A]>>24 ; CMP 2 ; JNZ 0x8AFAC
0x8AFA2 EAX=0; AX=DX(limit=frame delta); CALL 0x89EB0   ; phase-2 per-frame side-effect
0x8AFAC ADD byte[0x157AC1], DL            ; sub-second += delta_lo
0x8AFB2 EBP = 4
0x8AFB7 (loop head)  EAX = byte[0x157AC1]
0x8AFBE CMP EAX,0x3C ; 0x8AFC1 JL 0x8B3AE ; < 60 -> not a second boundary
0x8AFC7 TEST CX,CX ; 0x8AFCA JNZ 0x8B3AE  ; already completed -> exit
0x8AFD0 DL = byte[0x157AC2]               ; period index
0x8AFD6..0x8AFEB  limit select: index<=1 -> SI=word[0x15881A]; 2..3 -> SI=word[0x15881C]
0x8AFF2..0x8B048  class-2 aux block:
    if (sec + 0x14 < aux + limit) { if (aux_tick == 0x1E) aux++; else aux_tick++; }
    else aux++;
0x8B04A AL=byte[0x157AC1]; DX=word[0x157AB4]; BX=word[0x157AB6]
0x8B05D SUB AL,0x3C ; 0x8B05F INC EDX ; 0x8B060 INC EBX
0x8B061 byte[0x157AC1]=AL ; 0x8B066 word[0x157AB4]=DX ; 0x8B073 word[0x157AB6]=BX
0x8B07A..0x8B19E  the per-second display/announcement block (fires the 0x79/0x7A
                  events via 0x8F188 when `sec` crosses limit - k*60; the
                  `FUN_0008EF38` "period approaching" probe at 0x8B0F3/0x8B177)
0x8B19E CMP byte[0x15882D],0 ; JZ 0x8B1C3
0x8B1A7 CALL 0x4BEC8 ; TEST AX,AX ; JNZ 0x8B1C3
0x8B1B1 EAX=0x11; DL=0; CALL 0x4C374 ; byte[0x15882D]=0
0x8B1C3 AL = byte[0x157AC2]
0x8B1CA CMP AL,3 ; 0x8B1CA JA 0x8AFB7     ; period index > 3 -> loop
0x8B1D2 JMP dword CS:[EAX*4 + 0x8AF28]    ; period-index dispatch (0..3)
```

So each loop iteration consumes one clock second and then runs the
**period-completion detection** for the current period index (`0x8AF28`).

### 2.3 The period-completion detection (the `0x8AF28` dispatch)

| index | block | limit | effect on completion |
|---|---|---|---|
| 0 | `0x8B1DD` | `[0x15881A]` | `CX=1` when `sec == limit` (aux==0) or `sec == limit+aux`; else `[0x15882D]=1` |
| 1 | `0x8B22F` | `[0x15881A]` | `CX=1` on the same equality; **then the score/flag block (§2.4)** |
| 2 | `0x8B2E9` | `[0x15881C]` | `CX=1`; `[0x15882D]=1` |
| 3 | `0x8B33F` | `[0x15881C]` | `CX=1`; **then W4 (`0x8B3A2`)** |

Fresh `0x8B1DD`/`0x8B22F` (the `[0x15881A]` blocks):
```
0x8B1DD MOV AX,[0x157AB6]; MOV BX,word[0x15881A]; CMP AX,BX; JNZ 0x8B205
0x8B1EF CMP word[0x157ABA],0; JZ 0x8B225     ; aux==0 -> CX=1
0x8B1F9 MOV byte[0x15882D],1; JMP 0x8AFB7    ; aux!=0 -> display flag only
0x8B205..0x8B225 (sec != limit): EDX = limit + aux; if sec == EDX -> 0x8B225 MOV ECX,1
```
Fresh `0x8B33F` (index 3): `… MOV EAX,0 … CMP EAX,EDX; JNZ 0x8AFB7` → `0x8B387
MOV AX,[0x157AC5]; MOV DX,[0x157AC7]; MOV ECX,1; CMP AX,DX; JZ 0x8AFB7;
0x8B3A2 MOV byte[0x157AC0],1` — **W4** (the `[0x15881C]`-limit completion with
differing scores).

**The completion predicate** is therefore `sec == limit` (aux==0) or
`sec == limit + aux`; this is exactly the FU-62 library's
`period_seconds == limit + aux_seconds` (`fifa96_match_state.c:78`), with the
`aux != 0 && sec == limit` case setting the display flag `[0x15882D]` instead of
completing (`:80`).

### 2.4 The score tiebreak and the `[0x157AC0]` set (index-1 block, `0x8B274..0x8B2E4`)

```
0x8B295 MOV AX,[0x157AC5]; MOV DX,[0x157AC7]; MOV ECX,1; CMP AX,DX
0x8B2AA JZ 0x8B2B8                        ; scores EQUAL -> override check
0x8B2AC MOV byte[0x157AC0],1; JMP 0x8AFB7 ; <<< W2: scores DIFFER -> flag=1
0x8B2B8 CMP dword[0x14C2EE],0; JZ 0x8B2CD
0x8B2C1 MOV byte[0x157AC0],1; JMP 0x8AFB7 ; <<< W3: override != 0 -> flag=1
0x8B2CD CMP byte[0x14C1D0],0; JNZ 0x8AFB7
0x8B2DA MOV dword[0x10F378],1; JMP 0x8AFB7 ; the no-extra-time latch
```

**The score "tiebreak" is `[0x157AC5]` (score A) vs `[0x157AC7]` (score B)** — a
per-team word pair. W2 sets the flag when they **differ** (with the `[0x14C2EE]`
override and the `[0x14C1D0]` else-arm as the equal-score escape). This is the
same score pair the chooser's ET branch reads (§3).

### 2.5 The rollover, the chooser call and the period-index INCREMENT (`0x8B3AE..0x8B58A`)

`0x8B3AE` is the completion exit:
```
0x8B3AE CMP byte[0x15882D],0 ; JZ 0x8B3C7
0x8B3B7 CMP byte[0x14C32A],0 ; JNZ 0x8B3C7
0x8B3C0 XOR EAX,EAX; CALL 0x54104
0x8B3C7 TEST CX,CX ; JZ 0x8B590           ; NO completion -> the tail (§2.6)
0x8B3D0 AL = byte[0x157AC2]
0x8B3D7 CMP EAX,1 ; JNZ 0x8B555           ; completed index != 1 -> 0x8B555
0x8B3E0..0x8B553  the completed-index-1 decision block (the half/ET decision)
0x8B555 TEST AL,AL ; JNZ 0x8B574          ; completed index != 0 -> chooser
0x8B559 EAX=0xC2; EBX=0x20; CALL 0x8F188  ; (index-0 completion event)
0x8B56A EAX=0x12; CALL 0x4C374 ; 0x8B56F CALL 0x4C374
0x8B574 CALL 0x8B9CC                      ; <<< THE CHOSER (phase + act), §3
0x8B579 BH = byte[0x157AC2]; XOR ESI,ESI; INC BH
0x8B583 word[0x157AB6] = SI(0)            ; sec = 0
0x8B58A byte[0x157AC2] = BH               ; <<< PERIOD INDEX INCREMENT
```

So on every completed period: the chooser `FUN_0008B9CC` runs **with the OLD
period index** (the increment is at `0x8B58A`, after), `sec` is zeroed, and the
period index is incremented by 1. The completed-index-1 block (`0x8B3E0..0x8B553`)
is the extra-time/half decision: it tests `[0x14C1D0]` (`0x8B3E0 CMP EAX,3`), the
score pair, the `FUN_0004C2C0` gate and the `FUN_00012230`/`FUN_00012250` probes,
fires the `0xB8..0xBC` events, and sets `EAX=0x13` before joining the chooser at
`0x8B56F`/`0x8B574`.

### 2.6 The no-completion tail and the period-4 skip (`0x8B590..0x8B621`)

```
0x8B590 XOR EAX,EAX; MOV AL,[0x157AC2]
0x8B597 CMP EAX,0x1 ; JLE 0x8B601         ; index <= 1 -> 0x8B601
0x8B59C CMP EAX,0x4 ; JGE 0x8B601         ; index >= 4 -> 0x8B601
0x8B5A1 MOV AX,[0x157AC5]; CMP AX,[0x157AC7]; JZ 0x8B601   ; tied -> 0x8B601
0x8B5B0 CMP byte[0x157AC0],0; JNZ 0x8B601  ; flag already set -> 0x8B601
0x8B5B9 MOV DL,1 ; MOV EBX,0x20
0x8B5CD MOV byte[0x157AC0],DL             ; <<< W5: mid-extra-time flag set
0x8B5D3..0x8B5F5  SETA + FUN_000741b4 + event 0xC0
0x8B5FA CALL 0x8B9CC                       ; the chooser
0x8B5FF JMP 0x8B65B
0x8B601 XOR EAX,EAX; MOV AL,[0x157AC2]
0x8B608 CMP EAX,0x4 ; JNZ 0x8B623          ; <<< THE PERIOD-4 SKIP
0x8B60D CMP byte[0x157AC0],0; JZ 0x8B623
0x8B616 CALL 0x8B9CC                       ; period 4 + flag -> chooser
0x8B61B INC byte[0x157AC2]                 ; period index := 5
0x8B621 JMP 0x8B65B
0x8B623 MOV EAX,[0x157A4A]>>24 ; CMP 2 / 0x10 / 0xC …   ; the phase-screen block
0x8B655 MOV word[0x157ABA],0; MOV byte[0x15882D],0       ; clear aux + display
```

**The period-4 skip** is `0x8B608 JNZ 0x8B623`: when the period index is `4` and
the extra-time flag is set, the chooser runs and the index is bumped to `5`
(`0x8B61B`); otherwise (`index != 4`, i.e. the ordinary `2/3` case with tied
scores) the code falls to the phase-screen block `0x8B623`. `CX == 0` here means
the frame produced no completion.

> **Erratum (quoted) — FU-211 §3/§4.3.** FU-211 §3 renders the W5 set-site
> (`0x8B5CD`) as a "**period-boundary** tie/decider test" and §4.3 says "the flag
> is raised to 1 **when the period ends decisive**". Fresh disassembly shows
> `0x8B5CD` sits inside the **no-completion tail** — `0x8B3C7 TEST CX,CX; JZ
> 0x8B590` when `CX==0` (the frame's period did **not** complete). W5 therefore
> fires **per-frame** while the **period index is 2 or 3** and the scores differ
> and the flag is clear — a mid-extra-time flag set, not a completion edge. The
> address (`0x8B5CD`) and the `[0x157AC0]=1` semantics stand; only the
> "period ends / boundary" framing is corrected. (The completion-edge set-sites
> are W2 `0x8B2AC`/W3 `0x8B2C1` and W4 `0x8B3A2`; §2.4/§2.3.)

### 2.7 The harness tail (`0x8B65B..0x8B687`)

```
0x8B65B CMP byte[0x14C32A],0 ; JNZ 0x8B669
0x8B664 CALL 0x8BAF0                      ; <<< THE END MACHINE, §5
0x8B669 CMP dword[0x158808],0 ; JZ 0x8B678
0x8B672 CALL dword[0x158808]              ; <<< the installed-act tick (clone of 0x8B669/72)
0x8B678 MOV AX,[0x158822]                 ; <<< RETURN the match-over word
0x8B687 RET
```

The tail (a) runs the phase-`0xC`/`0x13`/`0x14` end machine `FUN_0008BAF0` when
the session gate `[0x14C32A] == 0`, (b) ticks the installed act handler once per
granted frame (`[0x158808]`, the FU-175 §2 clock-tail seam), and (c) returns
`[0x158822]`.

---

## 3. The chooser `FUN_0008B9CC` (`0x8B9CC..0x8BAEF`, fresh `disassemble_function`, 85 insns)

The chooser is called from three harness sites — `0x8B574` (every completion),
`0x8B5FA` (the mid-ET W5 tail) and `0x8B616` (the period-4 skip). It is the
`[0x157AC0]` **reader** (there is no writer inside it): `0x8B9CF CMP
byte[0x157AC0],0; JZ 0x8BAA7`.

```
if [0x157AC0] != 0:                        ; EXTRA-TIME branch
  EBX = 0x13 (phase); EDX = side selector
  if period < 4:  edx = (score_own <= score_other) ? 1 : 0   ; the score tiebreak
  else:           edx = (int8)[0x1587D8] <= (int8)[0x1587D9] ? 1 : 0
  if edx in {0,1}: probe = FUN_00012230(edx); if hit -> phase = 0x14
  else:            probe = FUN_00012250; if hit -> phase = 0x14
  side = {edx==0: [0x157ABE]; edx==1: [0x157ABF]; else: FUN_00036200(3) side}
else:                                      ; NO-EXTRA-TIME branch
  if period < 4: phase = 0xC; side = [0x157AAC]>>24
  else: no phase write
0x8BAC1 CALL 0x740A0(phase, side)          ; the phase write (when one is chosen)
0x8BAC6 CMP byte[0x157AC0],0; JZ 0x8BADB
0x8BACF EBX=1; EAX=0xC                     ; EXTRA TIME -> ACT 0xC
0x8BAD9 JMP 0x8BAE5
0x8BADB EBX=1; EAX=0xB                     ; NO EXTRA TIME -> ACT 0xB
0x8BAE5 XOR EDX,EDX; CALL 0x888FC          ; install act(EAX), stage 0, invoke-now
```

So the chooser writes **phase `0x13`/`0x14`** (extra time) or **phase `0x0C`**
(period-end screen; period `< 4`), then always installs+invokes **act `0xC`**
(flag set) or **act `0xB`** (flag clear) through the act dispatcher
`FUN_000888FC` at `0x8BAE7`.

This is the native body the engine ports as `fifa96_action_phase_period_end`
(`src/fifa96_loader/fifa96_action_handlers.c:1163..1210`): the `extra_time == 0`
arm returns `out.phase = 0x0C` (period `< 4`, side = controlled) and `out.act =
0x0B`; the `extra_time != 0` arm derives `edx` (the score tiebreak at `period <
4`, the `[0x1587D8]/[0x1587D9]` comparison at `period >= 4`) and picks phase
`0x13`/`0x14` with `out.act = 0x0C` — a direct mirror of §3.

---

## 4. The extra-time half start/end

* **Regulation half start**: `FUN_0004B508` (`0x4B508`) sets `[0x15881A]=0x3C`
  (60 s) and `[0x15881C]=0x1E` (30 s) — fresh bytes `ba3c000000`/`bb1e000000` and
  `6689151a881500`/`66891d1c881500`. The engine's
  `FIFA96_MATCH_RUN_PERIOD_SECONDS_RESET 60` / `EXTRA_SECONDS_RESET 30`
  (`fifa96_match_run.h:74..75`) mirror it.
* **Regulation end → extra time**: the completed-index-1 block
  (`0x8B3E0..0x8B553`) sets `EAX=0x13` (or `0x14`); the chooser (`0x8BAC1`) writes
  phase `0x13`/`0x14` and installs act `0xC` when the flag is set.
* **The extra-time half bodies**: phase `0x13`/`0x14` are handled by the end
  machine `FUN_0008BAF0` (§5): camera + the `[0x15881A]`-timeline match-over
  gates (`0x8BD45..0x8BDDD`), and `[0x158822]=2` (ET over) on the terminal.
* **Extra-time half end**: at each ET completion the chooser writes phase
  `0x13`/`0x14` and act `0xC` (the phase-`0x22` three-stage beep timer, *cross-cited*
  FU-211), and the harness increments the period index (`0x8B58A`). The
  act-7/phase-`0x1D` terminal `0x89E8C` writes `[0x157AC0]=1` (*cross-cited*
  FU-209 §2.10/FU-207 §3.4).

---

## 5. The end machine `FUN_0008BAF0` and the match-over word `[0x158822]`

`FUN_0008BAF0` (`0x8BAF0..0x8BC65`, fresh `disassemble_function`, 379 insns) is
the phase-indexed end/transition machine, called from the harness tail `0x8B664`
when `[0x14C32A] == 0`. Its phase dispatch (`0x8BAFB EAX=[0x157A4A]>>24`):

* **phase 6** (`0x8BB0E`): the `0x930`/`0x4B0` camera/screen setup block.
* **phase 2** (`0x8BB55`): when `[0x157AC2] > 3` (`0x8BB65 CMP EAX,3; JLE …`) it
  copies the `0x15880C` triple to `0x158830`; else the `FUN_0008DC68` pan block.
* **phase 0x0C** (`0x8BC01`) — **the period-end screen / end-of-match**:
  ```
  0x8BC0A CMP dword[0x158808],0 ; JNZ 0x8BC20
  0x8BC13 word[0x158818] += [0x157A64]          ; tick the act timer if no act
  0x8BC20..0x8BC52 the 0x960 camera + 0x36200(3) + 0x8DCD4 sinks
  0x8BC57 EAX = word[0x15881A]; CMP EAX,0x78; JLE 0x8BDE7     ; timeline gate
  0x8BC80 CMP dword[0x10F378],0 ; JZ 0x8BCA3                  ; the no-ET latch
  0x8BC89 CALL 0x37F54; [0x10F378]=0; [0x158818]=0; JMP 0x8BDE7
  0x8BCBF CMP byte[0x157AC0],0 ; JZ 0x8BCD9
  0x8BCC8 EAX=0x13; EDX=0; CALL 0x740A0          ; <<< flag SET -> phase 0x13 (ET)
  0x8BCD9 MOV word[0x158822],1                    ; <<< flag CLEAR -> MATCH OVER (1)
  ```
* **phase 0x13 / 0x14** (`0x8BCE7`) — the extra-time halves:
  ```
  0x8BCF5 CMP dword[0x158808],0 ; JNZ 0x8BD0B
  0x8BCFE word[0x158818] += [0x157A64]
  0x8BD0B..0x8BD36 the 0x780 camera + [0x158814] hold
  0x8BD45 EAX=[0x157A4A]>>24; CMP EAX,0x14; JNZ 0x8BD95
  0x8BD52..0x8BD81 (phase 0x14): word[0x15881A] timeline gates
  0x8BD8A MOV word[0x158822],2                   ; ET OVER (2)
  0x8BD95 (phase 0x13): CMP [0x157AC5],[0x157AC7]; JNZ 0x8BDE7   ; scores differ -> hold
  0x8BDD4 MOV word[0x158822],2                   ; ET OVER (2)
  0x8BDDD MOV dword[0x10F360],1                  ; the ET-over latch
  ```
* **the tail** (`0x8BDE7..0x8BC65`): the `0x8DCD4` position triple, the
  `[0x15883C]`/`[0x158881]` heading-damping block and the `0x6FE84` team refresh.

**The match-over word is `[0x158822]`** (values `1` = regulation complete/no ET,
`2` = extra-time complete), written by `FUN_0008BAF0` and cleared by
`FUN_0004B02C` (`0x4B0E7`), `FUN_0004B100` (`0x4B29B`) and `FUN_0007…/0x88D42`
(fresh `get_xrefs_to 0x158822` = **10**: 3 writers in `FUN_0008baf0`
0x8BCD9/0x8BD8A/0x8BDD4, 1 reset in `FUN_0004b02c` 0x4B0E7, 1 in `FUN_0004b100`
0x4B29B, 1 in `FUN_000886d4` 0x8871B, plus raw `0x74342`/`0x88D42`; readers
`0x4B106`/`0x8B678`). It is **consumed** by the frame loop `FUN_0004B100`:

```
0x4B1A6 CALL 0x8AF38                  ; AX = [0x158822]
0x4B1AB CMP AX,0x1 ; 0x4B1AF JC 0x4B2A9   ; AX==0 -> the ongoing-match path
0x4B1B5 JBE 0x4B1C6                   ; AX==1 -> regulation-over path
0x4B1B7 CMP AX,0x2 ; 0x4B1BB JZ 0x4B257   ; AX==2 -> extra-time-over path
0x4B297..0x4B2A8 word[0x158822]=0; RET     ; the harness result is consumed + cleared
```

So the harness return AX drives the frame loop's match-state branch, then
`[0x158822]` is cleared. **The engine has no `[0x158822]` cell and no producer**
(`grep 158822 src include` = comments only, `fifa96_match_run.c:3097,3758`; the
lifecycle owns exits). This is a leg (§6/§9).

---

## 6. Engine parity map — native site → engine function + line region

| native site (fresh) | engine | cite |
|---|---|---|
| class table `0x1106AD` | `fifa96_match_state_class_table[0x30]` (byte-identical) | `fifa96_match_state.c:4..8` |
| harness class gate `0x8AF41..0x8AF80` | `fifa96_match_state_frame` class gate + `phase_drive` `clock_class` gate | `fifa96_match_state.c:57..58`; `fifa96_match_run.c:2575..2580` |
| `sec == limit(+aux)` completion `0x8B1EA/0x8B21D/0x8B37F` | `period_seconds == limit + aux_seconds` | `fifa96_match_state.c:77..83` |
| `[0x157AB6]` sec / `[0x157AC1]` sub-second / `[0x157AB4]` total | `period_seconds` / `second_acc` / `total_seconds` | `fifa96_match_state.c:59..76` |
| `[0x157ABA]`/`[0x157ABC]` aux/aux-tick (class-2) | `aux_seconds` / `aux_tick` | `fifa96_match_state.c:66..73` |
| period-index JUMP `0x8AF28` + INCREMENT `0x8B58A` | `fifa96_match_state_frame` `s->period++` (`:85`) | `fifa96_match_state.c:84..89` |
| chooser `FUN_0008B9CC` (`0x8B9CC`) | `fifa96_action_phase_period_end` | `fifa96_action_handlers.c:1163..1210` |
| chooser period write `0x8BAC1` | `match_run_write_phase(mr, out.phase)` in `phase_drive` | `fifa96_match_run.c:2588..2591` |
| chooser act tail `0x8BAE7 CALL 0x888FC(0xB/0xC)` | `fifa96_match_act_dispatch(mr, out.act, 0u, 1)` | `fifa96_match_run.c:2599` |
| `[0x157AC2]` read by the chooser (`0x8B9E3`) | `(uint8_t)(mr->state.period - 1u)` passed to the chooser | `fifa96_match_run.c:2583` |
| the `[0x157AC0]` argument (`extra_time`) | **hardwired `0u`** | `fifa96_match_run.c:2583` |
| `FUN_0004B02C` mode-1 match-init (situation 0) | `fifa96_match_run_period_setup(mr, 1)` at begin | `fifa96_match_run.c:2408, 3069..3072` |
| `FUN_0004B02C` mode-0 (`FUN_00088860` period-end) | `fifa96_match_run_period_setup(mr, 0)` (no live caller) | `fifa96_match_run.c:3073..3089` |
| bit-8 ET gate `0x4B0CB TEST AH,1` | `(input_range[0]|input_range[1]) & 0x100` | `fifa96_match_run.c:3093..3095` |
| the period-limit setter `FUN_0004B508` | `set_period(60,30)` for selector != 0; `(120,40)` for selector 0 | `fifa96_match_run.c:2360..2364, 2417` |
| the phase-0xC end machine `FUN_0008BAF0` | **unported** | `fifa96_match_run.c:2560..2562` (comment) |
| `[0x158822]` match-over word | **no cell** | `fifa96_match_run.c:3758` (comment) |
| phase `0x13`/`0x14` rows | phase table rows `NULL` | `fifa96_match_handlers.c:5097,5099` |
| the phase-0x1D/0x22 cohorts | phase `0x1D`/`0x22` `NULL` | `fifa96_match_handlers.c:5117,5128` |

The engine's clock/chooser pair is therefore **structurally present and tested**
(`tests/test_engine_match_handlers.c:3692/3721` pin the sit-8 route; FU-208 §6),
but its **inputs are inert**: `global_157ac2` never increments (no `++` anywhere,
FU-208 §6.2) and `extra_time` is hardwired `0u` (`fifa96_match_run.c:2583`).

---

## 7. What the engine must extend (function + line region)

To honestly wire the extra-time period/half/clock machinery, four landings, in
order:

1. **Thread the live extra-time flag into the chooser — replace the hardcoded
   `0u`.** `fifa96_match_run_phase_drive` (`src/fifa96_engine/fifa96_match_run.c:2583`)
   passes `extra_time = 0u` to `fifa96_action_phase_period_end`. A live producer
   for the native `[0x157AC0]` (the W2/W3/W4/W5 set-sites, **or** least the
   period-1 completion W2 at `0x8B2AC`) must feed it. The chooser already derives
   `out.act = 0x0C` and `out.phase = 0x13/0x14` for a non-zero flag
   (`fifa96_action_handlers.c:1180..1201`), so **no chooser change** is needed.
2. **Advance the period index and re-invoke `period_setup(mode 0)`.** The engine
   mirrors the native increment only via `fifa96_match_state_frame`
   (`s->period++`, `fifa96_match_state.c:85`), and `fifa96_match_run_period_setup
   (mr, 0)` (`fifa96_match_run.c:3073..3089`) — the `FUN_00088860` period-end
   arm that dispatches situation 8 / situation 1 and applies the bit-8 gate — has
   **no live engine caller**. Wire it into `fifa96_match_run_phase_drive`
   (`:2564..2602`, after the chooser act dispatch at `:2599`), mirroring the native
   `0x74244` period-end call sequence. The native `0x88860` is reached from
   `FUN_0004B02C` (`0x4B08E`), which itself is the period-boundary producer.
3. **Register the cohort rows** so the extra-time phases/acts actually run:
   phase `0x13`/`0x14` bodies (`fifa96_match_handlers.c:5097,5099`, `NULL`),
   phase `0x1D` (`:5117`), phase `0x22` (`:5128`), action `0x20`
   (`:5034`) — *cross-cited* FU-208 §6.2 / FU-209 §6.4 / FU-211 §6.1. Without the
   phase rows the `match_run_write_phase` per-record walk is a structural no-op
   (`fifa96_match_run.c:2518..2522`).
4. **Model the end-of-match word `[0x158822]` (and the phase-0xC end machine
   `FUN_0008BAF0`)** if the match lifecycle is to end on the native condition.
   Today the engine lifecycle owns exits (`fifa96_match_run.c:3758`); the harness
   return `[0x158822]` and the phase-`0xC`/`0x13`/`0x14` machine are unported.
   This landing is optional for the census but required for native-faithful
   period termination.

**Native install/invoke order to mirror** (period completion): score pair
`[0x157AC5]`/`[0x157AC7]` tiebreak (W2) → `[0x14C2EE]` override (W3) → the chooser
`0x8B574` (phase `0x13`/`0x14`/`0xC` write via `0x740A0`, then act `0xB`/`0xC`
install+invoke at `0x8BAE7`) → `sec=0` (`0x8B583`) → period index++ (`0x8B58A`).

---

## 8. Numbered open legs

1. **FU213-L1 — the `[0x157AC0]` extra-time flag has no engine cell or
   producer.** Fresh: `extra_time` is hardwired `0u` at
   `fifa96_match_run.c:2583`; the flag's four harness set-sites
   (`0x8B2AC`/`0x8B2C1`/`0x8B3A2`/`0x8B5CD`) plus the front-end `0x389A4` and the
   act-7 terminal `0x89E8C` are unported. This slice maps the mechanism, not a
   producer. Owner: the extra-time flag producer (FU-195-L2 / FU-207-L1). Do
   **not** count phase `0x22`/`0x1D` off registration alone (no hand-set census).
2. **FU213-L2 — `fifa96_match_run_period_setup(mr, 0)` has no live caller.**
   Fresh grep: the only caller is the `begin` `mode 1` arm (`match_run.c:2408`,
   gated by `period_setup_live`, seeded 1 at `:2217`). The mode-0 arm
   (`:3073..3089`) is exercised only by fixtures. The native mode-0 is reached
   from the period-boundary `FUN_0004B02C` (`0x4B08E`). Owner: the period-rollover
   wiring. Do not count the mode-0 outputs until a live caller lands.
3. **FU213-L3 — the end-of-match word `[0x158822]` and `FUN_0008BAF0` are
   unported.** Fresh `get_xrefs_to 0x158822` = 10 (writers in `FUN_0008baf0`,
   `FUN_0004b02c`, `FUN_0004b100`, `FUN_000886d4` + raw `0x74342`/`0x88D42`); the
   engine has no cell and the lifecycle owns exits (`match_run.c:3758`). The
   harness return AX selects the frame-loop match-state branch (`0x4B1AB..0x4B1C1`).
   Owner: the match-lifecycle/end-machine slice.
4. **FU213-L4 — the class-2 clock halt `[0x14C302]` is unmodelled (latent).**
   Native class-2 clock frames halt when `[0x14C302] != 0` (`0x8AF63..0x8AF6A`);
   the engine's `fifa96_match_state_frame` always runs class 2 (`clock_halt = 0`,
   `fifa96_match_run.c:3903`). The `[0x14C302]` writer `FUN_0003749C`
   (`0x3759C`/`0x375C2`) is unported; the image default is 0, so the tape is
   unaffected — a latent divergence only if a live writer lands. Owner: the
   match-config slice.
5. **FU213-L5 — the period limit / display-flag config writers are unported.**
   `[0x15881A]`/`[0x15881C]` writers `FUN_0003749C` (`0x375E4`/`0x375ED`),
   `FUN_0004B508` (`0x4B514`/`0x4B51B`, the 60/30 reset the engine's selector != 0
   path models) and `FUN_000886D4` (`0x88710`/`0x88730`); the display flag
   `[0x15882D]` and the counters `[0x157AB4]`/`[0x157ABC]` are process cells. The
   engine models the *values* (`state.period_length`/`extra_length`), not the
   native writers. Owner: the match-config slice.
6. **FU213-L6 — the clock-signalled events `0x79/0x7A/0xB8..0xBC/0xC0/0xC2` are
   unported.** The harness's per-second display block (`0x8B07A..0x8B19E`) and the
   rollover events fire through `0x8F188`/`0x4C374`; the engine carries none (the
   `ref_whistle`/event-queue legs, FU-195-L3/FU-211-L3). Whether they are
   render-read is unverified; assume not until a `cmp` proves otherwise.
7. **FU213-L7 — the `[0x157AC0]` score polarity is recorded, not asserted.**
   Fresh §2.4: W2 sets the flag when the score pair `[0x157AC5]`/`[0x157AC7]`
   **differ** at the completed-index-1 boundary (the `[0x14C2EE]` override and the
   `[0x14C1D0]` cell are the equal-score escapes). The football-level meaning
   ("extra time when *not* tied", i.e. the tournament aggregate/away-goals rule)
   is **not** decomposed here; the front-end writer `0x389A4` also sets
   `[0x14C2EE]=1` from the aggregate (`FUN_00038630` ← `FUN_000493A0`,
   *cross-cited* FU-207 §3.3/L2). Recorded as observed; do not over-claim the rule.
8. **FU213-L8 — the extra-time halves' body semantics ride on the NULL phase
   `0x13`/`0x14` rows.** The chooser can select phase `0x13`/`0x14`
   (`fifa96_action_handlers.c:1189..1201`), and the end machine reads
   `[0x15881A]` for their termination (`0x8BD52..0x8BD81`), but the engine's
   phase rows are `NULL` (`fifa96_match_handlers.c:5097,5099`) and the per-record
   walk is a no-op (`fifa96_match_run.c:2518..2522`). Owner: the phase-`0x13`/`0x14`
   body slice.

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `watcom:LE:32:default`, `image_base
  0`, `min_address 0x10000`, `max_address .image::00174a2a`, 2706 functions,
  `.image` overlay present.
* `disassemble_function` — `0x8AF38` (475 insns), `0x8BAF0` (379 insns),
  `0x8B9CC` (85 insns).
* `disassemble_bytes` (start, length B) — `0x8EF38` (96), `0x4B508` (96),
  `0x4B180` (144), `0x4B280` (64).
* `read_memory` — `0x1106AD` (48 B), `0x8AF28` (16 B).
* `get_xrefs_to` — `0x158822` (**10**), `0x15881A` (**21**), `0x15881C` (**9**),
  `0x14C302` (**3**), `0x8AF38` (**2**), `0x157AC2` (**56**).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(`:2020..2089`/`:2130..2199` the reset/begin cell seeds; `:2217` `period_setup_live
= 1`; `:2259` the range-word rebuild; `:2360..2364` `set_period`; `:2408` the
live `period_setup(mode 1)`; `:2417..2424` `set_period`; `:2504..2524`
`match_run_write_phase`; `:2564..2602` `fifa96_match_run_phase_drive` (the clock
gate `:2575..2580`, the `extra_time = 0u` call `:2583`, the act dispatch `:2599`);
`:3063..3099` `fifa96_match_run_period_setup`; `:3758` the match-over comment;
`:3891..3909` `fifa96_match_run_frame` + the `clock_period_ended` staging);
`src/fifa96_loader/fifa96_match_state.c` (whole, 103 lines — the FU-62 clock);
`src/fifa96_loader/fifa96_action_handlers.c` (`:1114` `phase_act`, `:1129..1147`
the situation table, `:1163..1210` `fifa96_action_phase_period_end`);
`src/fifa96_engine/fifa96_match_handlers.c` (`:5005`/`:5007` rows `0x13`/`0x14`,
`:5032`/`:5034` rows `0x21`/`0x22`, `:5091`/`:5097`/`:5099`/`:5117`/`:5125`/`:5128`
the phase rows); `include/fifa96_engine/fifa96_match_run.h` (`:65..75` the period
lengths; `:520`/`:610` `global_157ac2`; `:579..587` `clock_period_ended`;
`:1302..1319` `period_setup`).

Docs cross-cited: FU-143 §3.3/§4 (the period chooser), FU-175 §2 (the clock-tail
act seam), FU-195 §3/§4, FU-196, FU-202 §4, FU-205, FU-206 §4 Tier-1/§5 PR-3,
FU-207 (OL-85), FU-208 (situation 8), FU-209 (act 7 / phase 0x1D), FU-211
(phase 0x22 / act 0xC). AGENTS.md census rule. **No writes beyond this slice
file**; no rename/label/comment/function/script/project save; no git commands.
