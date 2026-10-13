# FU-208 — the situation-8 selector + the act-7 / phase-0x1D route (phase-19 wave R56)

Provenance: M2 phase-19 wave **R56**, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE`,
path `/FIFA96.EXE`, `watcom:LE:32:default`, `min_address` `0x10000`,
`max_address` `.image::00174a2a`, 2706 functions, `.image` overlay present). Every
address/byte below is a fresh `read_memory` / `disassemble_bytes` /
`get_xrefs_to` / `get_function_by_address` of **this** session unless marked
*cross-cited* (then the frozen FU doc is named); the engine source was read
first-hand. The alias traps are applied (`dword[x]>>16` = word at `x+2`,
`>>24` = byte at `x+3`); addresses are `/FIFA96.EXE` runtime addresses (LE
globals sit at `+0x100000` from the file offset). **No writes**: no rename,
comment, label, function, script or project save; `/FIFA96.EXE` untouched; the
ISO/HDD untouched; no git commands. Model: FU-206.

Scope (R56 / phase-19 brief): derive **exactly** how native situation `8` is
selected and how it routes to act `7` (phase `0x1D`): (1) the situation
tables/indexing; (2) the period-index-4 / bit-8 gate; (3) the precise input that
selects `8`; (4) the exact engine extension points (function / line region / new
gate). Extends FU-196 (§1–§5), FU-202 (§1/§3/§4/§5), FU-194 (§3/§6.1), FU-189
(§1/§3), FU-148 §12.2 and FU-146 (the session gate).

**Answer in one line.** Situation `8` is **not** selected by a `7→8` progression
(there is none — sit `7` is a different row, phase `0xD`): it is selected by the
*extra-time request* — **bit 8 (`0x100`) of `[0x14C1D4] | [0x14C1D6]`** (the two
input/range words, `FUN_0004B02C` `0x4B0CB` `TEST AH,1`), or, once the **period
index `[0x157AC2] == 4`**, by the phase-`0xC` period-end (`FUN_00088860`
`0x888C8`). Both producers dispatch `FUN_0008A938(8, …, BX=0)`; the dispatcher
takes the **direct** path only when the session gate `[0x14C32A]` is closed or a
situation is pending, then the jump table `0x8A904[8] = 0x8AE08` runs
**act 7 invoke-now** (`0x8AEAE`). The engine already carries every piece of this
routing (situation table + `period_setup` + the act seam) and its fixtures prove
`8 → act 7`; what the engine **lacks** is (a) the producer trigger that sets the
extra-time condition, and (b) the **phase-0x1D slot registration** so the act-7
arm actually dispatches the ported `fifa96_match_action_phase_1D`.

---

## 1. The situation tables and their indexing (fresh)

There are **two** native situation tables, both reached from `FUN_0008A938`;
neither is the "current situation" store (that is the `AX` argument — FU-196 §1).

### 1.1 the queue table `0x8A8E0` (table 1)

Fresh `read_memory 0x8A8E0` (64 B) — the 9 live dwords `0x8A8E0..0x8A904` are
the queue arms, indexed **situation − 2** (sit 2..10); the remaining bytes of the
64-B read are the head of the jump table (see §1.2, contiguity confirmed):

| sit | idx | queue arm |
|---|---|---|
| 2/3/4 | 0/1/2 | `0x8A99E` |
| 5 | 3 | `0x8A9CD` |
| 6 | 4 | `0x8A9E8` |
| **7** | 5 | `0x8A9CD` |
| **8** | 6 | **`0x8AA60`** |
| 9/10 | 7/8 | `0x8AA23` |

`0x8A8E0 + 9×4 = 0x8A904` exactly — the queue table is 9 dwords, immediately
followed by the jump table. Every queue arm latches `[0x15B6C0] = 1` and `RET`s
(the queue path latches an id and **does not dispatch the act**; §2.1). For sit 8
the queued id is `0xA` (`0x8AA60`; FU-196 §2.1).

### 1.2 the jump table `0x8A904` (table 2)

Fresh `read_memory 0x8A904` (64 B) — 13 dwords `0x8A904..0x8A938`, indexed
**situation 0..0xC**:

| sit | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | **8** | 9 | A | B | C |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| target | `8AB82` | `8ABAB` | `8ABDB` | `8ABF3` | `8ADC0` | `8ADD8` | `8AC28` | `8ADF0` | **`8AE08`** | `8AEC6` | `8AEDE` | `8AEF6` | `8AF0E` |

Byte-identical to FU-196 §2.2. The base sits exactly after the queue table, and
`0x8A904 + 13×4 = 0x8A938` — the dispatcher head (§2.1). The table-2 jump site
is fresh-confirmed at `0x8AB7A`: `2eff248504a90800` = `JMP dword CS:[EAX*4 +
0x8A904]` (EAX = `CX` = situation, `0x8AB75..0x8AB77`).

**Indexing verdict.** With the queue index `sit − 2` (table 1) and the direct
index `sit` (table 2) both first-hand, the selector for `8` is
`0x8A904[8] = 0x8AE08` on the direct path and `0x8A8E0[6] = 0x8AA60` on the queue
path. **Situation `7` is unrelated**: `0x8A904[7] = 0x8ADF0` (phase `0xD`), and
the queue table shares `0x8A9CD` for 5/7 — there is no `7 → 8` transition.

---

## 2. The dispatcher head `0x8A938` — the direct/queue split

Fresh `disassemble_bytes 0x8A938` (200 B). The head is exactly:

```
0x8A93E MOV ECX,EAX                 ; ECX = situation
0x8A940 MOV word[ESP],DX            ; save side
0x8A944 TEST AX,AX / JZ 0x8AA7B      ; sit 0     -> DIRECT
0x8A94E CMP EAX,0xB / JZ 0x8AA7B     ; sit 0xB   -> DIRECT
0x8A957 CMP byte[0x14C32A],0 / JZ 0x8AA7B ; session gate CLOSED -> DIRECT
0x8A964 CMP dword[0x15B6C0],0 / JNZ 0x8AA7B; pending != 0 -> DIRECT
0x8A971 MOV ESI,[ESP] / TEST SI,SI / SETZ AL / AND EAX,0xFF ; (side==0)
0x8A97F SUB ECX,2
0x8A982 MOV [0x15B6B8],EAX          ; side latch (sit_side_pending)
0x8A987 CMP CX,8 / JA 0x8AA60       ; sit-2 > 8 -> default id 0xA
0x8A991 XOR EAX,EAX / MOV AX,CX
0x8A996 JMP CS:[EAX*4+0x8A8E0]      ; QUEUE table (sit-2) -> latch + RET
```

**The co-condition that the brief does not name.** Situation 8 dispatches act 7
**only on the direct path** — i.e. only when the session gate `[0x14C32A] == 0`
**or** a situation is already pending (`[0x15B6C0] != 0`). On the queue path
(open gate, nothing pending) sit 8 merely latches id `0xA` and returns without
touching the act (`0x8A99E`-family arm). `[0x14C32A]` is a widely-read gate —
fresh `get_xrefs_to 0x14C32A` = **60** (writes `0x1B7C8`/`0x1B7D5`
`FUN_0001B7B8`, `0x32DFF` `FUN_00032DE0`; readers include the dispatcher
`0x8A957`, `FUN_000886D4` `0x8872A`, `FUN_0008AF38`, `FUN_0008D098`); the
dispatcher is only one of its consumers.

### 2.1 the direct path `0x8AA7B` and the table-2 hub `0x8AB63`

Fresh `disassemble_bytes 0x8AA7B` (96 B) + `0x8AB60` (48 B):

```
0x8AA7B TEST BX,BX / JZ 0x8AAA8      ; BX==0 -> table-2
0x8AA80.. (BX!=0) -> the act-8 replay (store_15882b/c; FU-196 §3.4)
0x8AAA8 CMP CX,1 / JC 0x8AB63        ; sit 0 -> hub
0x8AAB2 JBE 0x8AABF                  ; sit 1 -> the sit-1 twin (0x8AABF)
0x8AAB4 CMP CX,4 / JBE 0x8AB08       ; sit 2..4 -> the order arm (0x8AB08)
0x8AABA JMP 0x8AB63                  ; else (incl. 8) -> hub
0x8AB63 MOV AL,[ESP] ; MOV [0x157AAF],AL   ; store the side
0x8AB6B CMP CX,0xC / JA 0x8AF1F      ; sit > 0xC -> act-8 fallback
0x8AB75 XOR EAX,EAX ; MOV AX,CX
0x8AB7A JMP CS:[EAX*4+0x8A904]       ; TABLE-2 (sit) -> 0x8AE08 for sit 8
```

So sit 8 with `BX=0` lands on `0x8AB63` → `0x8A904[8] = 0x8AE08`.

---

## 3. The sit-8 arm `0x8AE08` → act 7 invoke-now

Fresh `disassemble_bytes 0x8AE08` (80 B) + `0x8AE9E` (64 B):

```
0x8AE08 MOV EAX,[0x157A49] / SAR EAX,0x18 / CMP EAX,1 / JZ 0x8AEAE
        ; gate on byte [0x157A4C] (0x157A49 >> 24). If != 1:
0x8AE19 MOV CL,1 ; AL=[ESP](side) ; [0x157AAF]=AL ; [0x157A4C]=1
0x8AE30 CALL 0x897F4(EDX=0xA)        ; the match re-init pair (FU196-L5)
0x8AE3F CALL 0x897F4(EDX=0xA, EAX=1)
0x8AE44 MOV EAX,[ESP-2] / SAR EAX,0x10
0x8AE4B MOV EAX,[EAX*4+0x1587CC] / MOV [0x1587D4],EAX  ; the actor bind
0x8AE9E XOR EBX,EBX / word[0x157AC7]=BX / word[0x157AC5]=BX  ; clear score pair
0x8AEAE MOV EBX,1                     ; invoke-now
0x8AEB3 MOV EAX,7                     ; ACT 7
0x8AEB8 XOR EDX,EDX                   ; side 0
0x8AEBA CALL 0x888FC                  ; the act dispatcher
```

So situation 8 → **act 7, `BX=1` (invoke-now), side 0** — a *side-0 forced*
invocation (the producer always passes side 0 for sit 8 except the `FUN_00088860`
arm, which passes `rng & 1` as the register arg used by the re-init bind).

**Action → phase.** Fresh `read_memory 0x1107EC` (72 B): `0x1107EC[7] =
0x00089868`. Fresh `read_memory 0x110794` (140 B): `0x110794[0x1D] = 0x00089868`.
The act table is `0x110794 + 0x16*4` (act `a` → phase `0x16+a`; FU-175/FU-196),
so **act 7 = phase `0x1D` = `0x89868`** (byte-identical to FU-196 §2.2 /
FU-202 §5.2).

**Fresh xref of the phase-0x1D body:** `get_xrefs_to 0x89868` = **1**, the DATA
reference `0x110808` (= `0x110794 + 0x1D*4`). The act-7 route reaches the body
**only** through the phase-table seam — never by a direct call.

---

## 4. The two sit-8 producers and the period-index-4 / bit-8 gate (fresh)

Situation 8 has exactly **two** native producers (fresh `get_xrefs_to 0x8A938` =
**39**; the sit-8 sites are `0x4B0D9` and `0x888C8`).

### 4.1 producer A — the extra-time request bit (`FUN_0004B02C`)

Fresh `disassemble_bytes 0x4B090` (112 B). `FUN_0004B02C` is the match-init /
period-setup machine (callers `0x49415` mode 1 and `0x4950A` mode EBX, both in
`FUN_000493A0` — FU-196 §3.1/§3.2). The **common tail** runs for both modes:

```
0x4B0AA MOV EAX,ESI / CALL 0x8C418    ; both-team refresh (side 0)
0x4B0B1 MOV EAX,ECX / CALL 0x8C418    ; both-team refresh (side 1)
0x4B0B8 XOR ECX,ECX / XOR EAX,EAX
0x4B0BC MOV CX, word[0x14C1D4]        ; input/range word side 0
0x4B0C3 MOV AX, word[0x14C1D6]        ; input/range word side 1
0x4B0C9 OR  EAX,ECX
0x4B0CB TEST AH,0x1                   ; <<< BIT 8 (0x100) of the OR
0x4B0CE JZ  0x4B0E5
0x4B0D0 MOV EAX,8 / XOR EBX,EBX / XOR EDX,EDX
0x4B0D9 CALL 0x8A938                  ; <<< SITUATION 8, side 0, BX 0
0x4B0DE MOV byte[0x157AC2],4          ; <<< PERIOD INDEX := 4
0x4B0E5 XOR EBX,EBX ; [0x158822]=BX ; word[0x157A68]=BX ; CALL 0xCB2B6
```

`TEST AH,1` is **bit 8 (`0x100`)** of `EAX` (AH = `EAX[15:8]`), on the **OR** of
the two range words. This is the brief's "bit-8 gate". It also **sets the period
index to 4** (`0x4B0DE`) — the brief's "period-index-4".

### 4.2 producer B — the phase-0xC period-end with period index 4 (`FUN_00088860`)

Fresh `disassemble_bytes 0x88860` (160 B). `FUN_00088860` is the period-end body
(callers `0x4B08E` inside `FUN_0004B02C`, and `0x74244` — FU196-L6):

```
0x88863 MOV EAX,[0x157A4A] / SAR EAX,0x18 / CMP EAX,0xC / JNZ 0x888F7
        ; gate on byte [0x157A4D] (0x157A4A >> 24) == 0xC (period end)
0x88874..0x888A4  the setup sinks (0x7417C / 0x6FFC0 / 0x700F4 / 0x73E28)
0x888AB MOV AL,[0x157AC2] / CMP EAX,4 / JNZ 0x888E2   ; <<< PERIOD INDEX == 4
0x888B5 CALL 0x92AC8 / AND AL,1 / MOVSX EDX,AX / EAX=8
0x888C8 CALL 0x8A938                  ; <<< SITUATION 8, side rng&1, BX 0
0x888CD.. CALL 0x8F188(0xC1,0,8)      ; sink (FU196-L2 leg)
0x888E2 EAX=1 / EDX=[0x157AAB]>>24 / CALL 0x8A938   ; else SITUATION 1
```

So producer B is gated on **`phase byte [0x157A4D] == 0xC` AND period index
`[0x157AC2] == 4`**. Note `FUN_00088860` reads `[0x157A4A]>>24` (byte
`0x157A4D`), while producer A's sit-8 *arm* gate reads `[0x157A49]>>24` (byte
`0x157A4C`) — two distinct adjacent bytes (§5.3, erratum).

### 4.3 The precise input that selects situation 8 (the brief's "7→8" flips)

* **There is no `7 → 8` transition.** Sit 7 is a separate row
  (`0x8A904[7] = 0x8ADF0` → phase `0xD`); the queue table maps 5 and 7 to the
  same id `0x8A9CD`. Situation 8 is *entered*, never *promoted from 7*.
* **The input is the extra-time request:** bit 8 (`0x100`) of
  `[0x14C1D4] | [0x14C1D6]`, tested at `0x4B0CB`. On set, producer A dispatches
  sit 8 **and** forces `[0x157AC2] = 4` (`0x4B0DE`). Thereafter, at a
  phase-`0xC` period-end, producer B re-dispatches sit 8 while `[0x157AC2] == 4`.
* **The period index `[0x157AC2]`** is also advanced by the clock-harness
  rollover fresh at `0x8B58A` (`MOV BH,[0x157AC2]; INC BH; MOV [0x157AC2],BH` in
  `FUN_0008AF38`) and by the front-end `0x38990`; when it reaches 4 the mode-0
  producer fires (fresh `get_xrefs_to 0x157AC2` = **56**).

### 4.4 the bit-8 writer — an open gap (extends FU196-L1)

The two input/range words are written by only two functions (fresh xrefs of
`0x14C1D4`/`0x14C1D6`):

* **`FUN_0001C9BC`** (the `FUN_0001C9BC` eight-config-cell builder, ported as
  `fifa96_input_range_words`): fresh `disassemble_bytes 0x1C9BC` (224 B) + tail
  `0x1CA96` (48 B). It zeroes both words (`0x1C9CD`/`0x1C9D4`) and ORs in these
  bits: word0 {0,1,2,3,4,5,6,7,**9**} and word1 {3,4,5,6,7,**9**}
  (`[0x105288]==1` → `OR byte[0x14C1D5],0x2` = 0x200; `[0x10529C]==1` →
  `OR byte[0x14C1D7],0x2` = 0x200). **Bit 8 (`0x100`) is never written.**
* **`FUN_00024B00`** (`0x24B91`/`0x24B98`): writes both words **zero** on its
  `ESI==2` arm (`XOR EDX,EDX; MOV word[0x14C1D6],DX; XOR EBX,EAX; MOV
  word[0x14C1D4],BX`, with EAX=EBX=4 → 0).

So the sit-8 gate tests **bit 8**, but the only two range-word builders write
bit **9** (`0x200`, the `[0x105288]`/`[0x10529C]` "high byte := 2" arms) and
zero — **bit 8 has no located writer**. This is an honest gap: either the
extra-time config cell / gate bit is misattributed by one bit, or a third
producer exists outside these two builders. Leg **FU208-L1**; do not assert a
resolution.

---

## 5. The extra-time state has TWO native cells — `[0x157AC0]` ≠ `[0x157AC2]`

The brief names the OL-85 flag `[0x157AC0]` as the cohort keystone. Fresh
evidence separates it from the sit-8 selector:

### 5.1 `[0x157AC2]` — the period index (the sit-8 gate)

Fresh `get_xrefs_to 0x157AC2` = **56**. Writers include the reset `0x73F82`,
the bit-8 gate `0x4B0DE` (:=4), the rollover INC `0x8B58A`, the front-end
`0x38990` and `FUN_0004BA00` (`0x4BA20`/`0x4BA31`). The **sit-8 producers read
this cell** (§4.1/§4.2).

### 5.2 `[0x157AC0]` — the OL-85 extra-time flag (NOT read by any sit-8 producer)

Fresh `get_xrefs_to 0x157AC0` = **19** (7 writes + 12 reads): the reset
`0x73F58`; the clock-harness writers `0x8B2AC`/`0x8B2C1`/`0x8B3A2`/`0x8B5CD`
(`FUN_0008AF38`); the front-end `0x389A4` (`FUN_00038630`); and the act-7 /
phase-`0x1D` **terminal** `0x89E8C`. **Neither `FUN_0004B02C` nor
`FUN_00088860` is in this xref set** — the sit-8 route keys off `[0x157AC2]`
(and the bit-8 input), never `[0x157AC0]`.

### 5.3 the coupling (why the two cells are easy to conflate)

* The `FUN_0008AF38` rollover fresh at `0x8B579..0x8B5CD`: it INCs `[0x157AC2]`
  (`0x8B58A`), and when `1 < [0x157AC2] < 4` **and** the score pair
  `[0x157AC5]`/`[0x157AC7]` differ **and** `[0x157AC0] == 0`, it sets
  `[0x157AC0] = 1` (`0x8B5CD`) — the period-2/3 rollover that warrants extra
  time. `0x8B60D` then reads `[0x157AC0]` again. So `[0x157AC0]` is the
  *"extra-time is warranted"* flag and `[0x157AC2] == 4` is the *"we are in the
  extra-time period"* index.
* `[0x157AC0]` gates the **act-0xC / phase-0x22** arm (the `FUN_0008B9CC` tail
  `0x8BAE7 CALL 0x888FC(0xC)` reads it, `0x8B9CF`/`0x8BAC6`) and the goal-scan
  period-4 skip; the phase-`0x1D` terminal writes it (`0x89E8C`). It does **not**
  select situation 8.

**Correction to the brief.** The phase-19 keystone for *selecting situation 8*
is **`[0x157AC2] == 4` + the bit-8 extra-time request**, not the OL-85 flag
`[0x157AC0]`. `[0x157AC0]` remains the keystone for the *companion* cohort rows
(phase `0x22`/act `0xC`, and the phase-`0x1D` terminal write-back). The two are
coupled but not interchangeable.

---

## 6. Engine parity — exactly where to extend

Engine source read first-hand this session. **The routing already exists and its
fixtures prove `8 → act 7`:** `tests/test_engine_match_handlers.c:3692
test_period_setup_mode1_extra_time_sit8` (bit-8 → `global_157ac2 == 4`,
`act.id == 7`) and `:3721 test_period_setup_mode0_extra_time_sit8`
(`phase 0xC` + `global_157ac2 == 4` → `act.id == 7`).

### 6.1 what is already right

| native | engine | cite |
|---|---|---|
| sit 8 → act 7 | `fifa96_action_phase_situations[8] = {NONE, 7, 0, OPEN_LEG, 0}` | `fifa96_action_handlers.c:1142` |
| act 7 → phase 0x1D | `fifa96_action_phase_act`: `0x16 + act` | `fifa96_action_handlers.c:1114` |
| dispatcher head (direct/queue) | `fifa96_match_run_set_piece` head gate | `fifa96_match_run.c:2983` |
| sit-8 direct → `match_run_situation_side` | `fifa96_match_run.c:3051` | |
| `FUN_0004B02C` sit-8 producer | `period_setup` mode-0 `global_157ac2==4` + bit-8 gate | `fifa96_match_run.c:3079, 3093` |
| mode-1 live at match-init | `period_setup_live = 1`; call at begin | `fifa96_match_run.c:2217, 2408` |
| the ported phase-0x1D body | `fifa96_match_action_phase_1D` | `fifa96_match_run.c:5998` |

### 6.2 what is missing — the exact extension points

**(1) The phase-0x1D slot is NULL — the act-7 route dead-ends here.**
`fifa96_match_phase_table[0x1D].fn == NULL` (`fifa96_match_handlers.c:5117`,
`{0x1D, NULL, "…NOT counted…"}`). The engine's act tick maps act 7 →
`0x16+7 = 0x1D` and then **early-returns on the NULL body**
(`src/fifa96_engine/fifa96_match_act.c:63`:
`if (fifa96_match_phase_table[phase].fn == NULL) return FIFA96_OK;`). So even
when sit 8 arms act 7 (the fixtures prove the arm), the ported body never runs.
**The single required registration** is
`fifa96_match_handlers.c:5117`: `NULL` → `fifa96_match_action_phase_1D` (body at
`fifa96_match_run.c:5998`, declared `include/fifa96_engine/fifa96_match_run.h:1551`).

**(2) The producer trigger — the "new gate".** The engine's `period_setup`
already *contains* the gate, but no engine source ever satisfies it:

* **bit 8 of `mr->input_range`** is never set: `fifa96_input_range_words`
  (`src/fifa96_loader/fifa96_input.c:86`) mirrors `FUN_0001C9BC` and, like it,
  writes bits {0..7,9}; `mr->input_cfg` is staged zero (`fifa96_match_run.c:2011`).
* **`global_157ac2` never increments**: fresh grep of `src/` finds only `= 0`
  (resets `fifa96_match_run.c:2034, 2152`) and `= 4` (the bit-8 gate
  `fifa96_match_run.c:3095`); **no `++`**.
* **`period_setup(mr, 0)` is never called by a live engine path**: the mode-0
  arm (`fifa96_match_run.c:3073..3089`) is exercised only by fixtures
  (`tests/test_engine_match_handlers.c:3706..3745`). `fifa96_match_run_phase_drive`
  (`fifa96_match_run.c:2564`) is the native period-end analog but does **not**
  call `period_setup(mode 0)`, and it passes `extra_time = 0u`
  (`fifa96_match_run.c:2583`) so `out.act` is always `0x0B`.

  Therefore the engine needs **one of**: a producer for bit 8 of `input_range`
  (match the native extra-time config cell — blocked by §4.4), or a period-index
  advance + a `period_setup(mode 0)` call inside `phase_drive` (mirror
  `FUN_0008AF38`'s rollover `0x8B58A` and the `0x74244` period-end call). The
  gate *site* is `fifa96_match_run.c:3079` (mode-0) / `:3093` (bit-8).

**(3) The coupled action row `0x20`.** `fifa96_match_action_table[0x20].fn ==
NULL` (`fifa96_match_handlers.c:5030`); the install is the phase-0x1D arm-0
`0x899A2` (fresh bytes `ba20000000 MOV EDX,0x20; 8b442408 MOV EAX,[ESP+8];
e8f43fffff CALL 0x7D9A4`), which the ported `fifa96_match_action_phase_1D` does
**not** yet model (leg FU194-L7/L8). The row-0x20 body `0x84EEC` is unported.
Registering phase 0x1D without this row leaves a dead pair.

**(4) The invoke cadence.** Native sit 8 invokes act 7 **now** (`0x8AEAE
MOV EBX,1` → `0x8AEBA CALL 0x888FC`). The engine's `match_run_situation_side`
passes `invoke = (out.act == 2u ? 1 : 0)` (`fifa96_match_run.c:2807`), so act 7
installs with `invoke = 0` (deferred to the clock tail). For exactness the sit-8
arm must invoke-now (parallel to FU196-L3/FU202-L3 for act `0xA`).

**Minimal phase-19 landing to select situation 8 and route to phase 0x1D:**
register `fifa96_match_handlers.c:5117` (phase 0x1D) + supply the extra-time
producer gate (the `fifa96_match_run.c:3079/3093` inputs) + register
`fifa96_match_handlers.c:5030` (row 0x20) and port `0x84EEC`, with the
invoke-now fix at `fifa96_match_run.c:2807`. Census `+2` (phase 0x1D + action
0x20) → `53/80 = 66%`, subject to the tape verdict (extra-time is never staged
on the M2 tape; the cohort rows are tape-dormant — FU-206 §5 PR-7/PR-8).

---

## 7. Numbered legs

1. **FU208-L1 — the bit-8 vs bit-9 range-word mismatch (extends FU196-L1).** The
   sit-8 gate tests **bit 8** (`0x4B0CB TEST AH,1`), but the only two range-word
   writers set bit **9** (`0x200`, `FUN_0001C9BC` `0x1CA15`/`0x1CA25`) or zero
   (`FUN_00024B00` `0x24B91`/`0x24B98`); bit 8 has no located writer. Either the
   "extra-time config cell" attribution or the gate bit is off by one bit, or a
   third producer exists. Settle by tracing the `[0x105288]`/`[0x10529C]` cell's
   front-end producer and the extra-time menu path. Do not fake a bit-8 producer.
2. **FU208-L2 — the engine phase-0x1D slot is NULL (the exact dead-end).**
   `fifa96_match_handlers.c:5117` (`{0x1D, NULL, …}`) blocks the act-7 route at
   `fifa96_match_act.c:63`; the body `fifa96_match_action_phase_1D`
   (`fifa96_match_run.c:5998`) is ported but unregistered. Register to select/
   dispatch. Owner: phase-19.
3. **FU208-L3 — the extra-time producer trigger (the "new gate").** No engine
   source satisfies `fifa96_match_run.c:3079` (`global_157ac2 == 4`) or `:3093`
   (`input_range & 0x100`): `global_157ac2` never increments and `period_setup
   (mode 0)` is un-wired from `phase_drive` (`fifa96_match_run.c:2564`;
   `extra_time = 0u` at `:2583`). Owner: the period-rollover wiring.
4. **FU208-L4 — the act-7 invoke cadence.** Native sit 8 invokes act 7 now
   (`0x8AEAE`/`0x8AEBA`); the engine defers (`fifa96_match_run.c:2807`,
   `invoke = (out.act == 2u)`). Carried with FU196-L3/FU202-L3.
5. **FU208-L5 — the `[0x157A4C]` vs `[0x157A4D]` byte drift.** The sit-8 arm gate
   is `[0x157A49]>>24` = byte **`0x157A4C`** (`0x8AE08..0x8AE2A`); the
   mode-0 producer and the phase-0x1D body read `[0x157A4A]>>24` = byte
   `0x157A4D` (`0x88863`, `0x898B6`). Erratum below.
6. **FU208-L6 — the coupled action row 0x20.** Slot `NULL`
   (`fifa96_match_handlers.c:5030`); installer `0x899A2` (phase-0x1D arm 0) is a
   leg in the ported body; body `0x84EEC` unported. Register + port with L2.
7. **FU208-L7 — the situation-8 match re-init (FU196-L5, unported).** The
   `0x8AE19..0x8AE54` body (`[0x157AAF]`/`[0x157A4C]` writes, the
   `FUN_000897F4(…,0xA)` pair, the `[0x1587CC+side*4] → [0x1587D4]` actor bind)
   has no engine model beyond the derived side store.
8. **FU208-L8 — the session-gate co-condition.** Sit 8 dispatches act 7 only on
   the direct path (`[0x14C32A] == 0` or a pending situation); the engine's
   `session_gate_14c32a` is seeded 0 (`fifa96_match_run.c:2194`) and has no
   derived producer, so the live direct-path reachability of sit 8 is not proven
   (FU-166/R14 boundary).

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`, `watcom:LE:32:default`,
  2706 functions.
* `read_memory`: `0x8A8E0` (64 B), `0x8A904` (64 B), `0x1107EC` (72 B),
  `0x110794` (140 B).
* `disassemble_bytes` (start, length B): `0x8A938` (200), `0x8AA7B` (96),
  `0x8AB60` (48), `0x8AB08` (16), `0x8AE08` (80), `0x8AE9E` (64), `0x4B090`
  (112), `0x88860` (160), `0x89868` (200), `0x89980` (96), `0x89E70` (48),
  `0x8B560` (176), `0x1C9BC` (224), `0x1CA96` (48), `0x24B00` (176).
* `get_xrefs_to`: `0x89868` (1), `0x8A938` (39), `0x157AC0` (19), `0x157AC2`
  (56), `0x14C32A` (60), `0x14C1D4` (21), `0x14C1D6` (12).
* `get_function_by_address`: `0x88C4B` (no function).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(`:2160..2413` the reset/begin incl. `period_setup_live` `:2217` and the
producer call `:2408`; `:2564..2602` `phase_drive` + the `extra_time = 0u`
call at `:2583`; `:2765..2812` `match_run_situation_side` + the act dispatch
`:2804..2810`; `:2977..3052` `set_piece` head/direct; `:3063..3099`
`period_setup`; `:5998..6044` `action_phase_1D`); `src/fifa96_engine/fifa96_match_act.c`
(`:55..65` the tick NULL-body gate); `src/fifa96_engine/fifa96_match_handlers.c`
(`:5030` action `0x20` NULL, `:5058..5129` the phase table, `:5117` phase `0x1D`
NULL); `src/fifa96_loader/fifa96_action_handlers.c` (`:1114` `phase_act`,
`:1129..1161` the situation table); `src/fifa96_loader/fifa96_input.c`
(`:86` `fifa96_input_range_words`); `include/fifa96_engine/fifa96_match_run.h`
(`:520` `global_157ac2`, `:741` `session_gate_14c32a`, `:1551`
`action_phase_1D`); `tests/test_engine_match_handlers.c` (`:3675..3749` the
`period_setup` fixtures); `tests/test_input.c` (`:275..314` the range-word
builder). Docs cross-cited: FU-148 §12.2, FU-146, FU-166, FU-175 §1/§4, FU-189
§1/§3/§10, FU-194 §3/§6.1, FU-196 §1–§6, FU-202 §1/§3/§4/§5/§7, FU-206 §4/§5.
No writes beyond this slice file; no rename/label/comment/function/script/project
save; no git commands.

---

## Errata

**Erratum — FU-202 §1.2.** FU-202 §1.2 reads the sit-8 table-2 arm as
"`MOV EAX,[0x157A49]; SAR 0x18; CMP 1; JZ 0x8AEAE` — if the live phase byte
`[0x157A4D] != 1`". The dword is right (`0x157A49`), but `>>24` selects byte
**`0x157A4C`**, not `[0x157A4D]` (fresh `disassemble_bytes 0x8AE08`:
`a1497a1500`/`c1f818`; the written byte is `[0x157A4C]` at `0x8AE2A`, bytes
`880d4c7a1500`). The distinct byte `0x157A4D` (from `[0x157A4A]>>24`) is what
`FUN_00088860` (`0x88863`) and the phase-0x1D body (`0x898B6`) gate on. The
FU-202 §1.2 semantics ("runs the match re-init when the flag ≠ 1") stand; only
the byte address drifts by one. (Leg FU208-L5.)

**Erratum — FU-202 §4.** FU-202 §4 states "the engine mirrors only the
`global_157ac2` increments in `phase_drive`". Fresh grep over `src/` finds **no**
`global_157ac2` increment anywhere — the cell is only ever assigned `0`
(`fifa96_match_run.c:2034, 2152`) or `4` (`:3095`, the bit-8 gate). The
`phase_drive` body (`fifa96_match_run.c:2564..2602`) does not touch
`global_157ac2` at all. The FU-202 §4 remainder (the front-end/setup writers are
unported) stands. (Leg FU208-L3.)

**Correction — the phase-19 brief's keystone.** The brief names the OL-85 flag
`[0x157AC0]` as the cohort keystone for situation 8. Fresh evidence shows
`[0x157AC0]` is read by neither sit-8 producer (§5.2); the sit-8 selector is
`[0x157AC2] == 4` + the bit-8 extra-time request. `[0x157AC0]` remains the
keystone for the companion cohort rows (act `0xC`/phase `0x22`, and the
phase-`0x1D` terminal write-back `0x89E8C`). (Leg FU208-L5-adjacent; §5.)

---

## Landing (M2 phase-19 T2, 2026-10-13)

T2 implements the sit-8 selector's `[0x157AC2] == 4` arm and registers the
phase-`0x1D` slot, so the routing derived here is now engine-live.

* **FU208-L2 (the engine phase-0x1D slot is NULL) — RESOLVED.**
  `fifa96_match_handlers.c` `{0x1D, NULL}` → `fifa96_match_action_phase_1D`
  (`fifa96_match_run.c`).
* **FU208-L3 (the extra-time producer) — RESOLVED for the `[0x157AC2] == 4`
  arm.** The T1/FU-213 period-index roll (`fifa96_match_run_phase_drive`,
  `mr->global_157ac2 = state.period`) reaches 4 at an extra-time-entering
  completion; `phase_drive` then runs the mode-0 period-end producer
  `fifa96_match_run_period_setup(mr, 0)` (native `FUN_00088860` `0x888C8`),
  whose `state.phase == 0x0C && global_157ac2 == 4` gate selects situation 8.
* **FU208-L1 (the bit-8 vs bit-9 range-word gap) — OPEN, not faked.** No located
  writer sets bit 8; the disjunct stays a leg.
* **§6.2 (3)/(4) remain legs:** the coupled row-`0x20` body/installer (`0x899A2`,
  FU194-L7/L8) is T3; the act-7 invoke-now cadence is **FU208-L4**.

Census `+1` (phase `0x1D`) → `52/80 = 65%`; M1 `09b726b7…` and M2 `441415fc…`
byte-identical (the tape reaches no `global_157ac2 == 4` completion). No prior
section rewritten.
