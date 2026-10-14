# FU-233 — the extra-time-half clock + the phase-`0x13`/`0x14` rows (phase-22 wave W22-3)

Provenance: M2 phase-22 Stage-1 recon slice **W22-3**, 2026-10-13, read-only.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument; confirmed this session by
`get_current_program_info` — `FIFA96.EXE` / path `/FIFA96.EXE`, executable
`game/hdd/FIFA96/FIFA96.EXE`, `Linear Executable (LE-Style DOS)`,
`watcom:LE:32:default`, image base `0`, min `0x10000`, max `.image::00174a2a`,
**2706** functions, **15239** symbols, `memory_size` 2757479, `.image` overlay
present). Every address/byte/instruction/xref-count below is a **fresh**
`read_memory` / `disassemble_bytes` / `get_xrefs_to` / `get_function_by_address`
of **this** session unless marked *cross-cited* (then the frozen FU doc is
named). The alias traps are applied (`dword[x]>>16` = the word at `x+2`,
`>>24` = the byte at `x+3`); LE globals sit at `+0x100000` from the file offset;
addresses are `/FIFA96.EXE` runtime addresses. Engine/loader source read
first-hand. **No writes**: no rename, comment, label, function, script or
project save; `/FIFA96.EXE` untouched; the ISO / `game/hdd/` untouched; no git
commands.

Scope (W22-3 brief): derive the **extra-time halves** (the clock's periods
`2`/`3`) and the **phase-`0x13`/`0x14` handler rows** — (1) the FU-62 clock's
`period < 2 ? period_length : extra_length` route (FU-62/FU-223) and the
periods `2`/`3` completion; (2) the engine's phase rows `0x13`/`0x14`
(`fifa96_match_handlers.c`) plus the native phase-table slots and the act-slot
anchors `0x1107EC`/`0x110808`; (3) how an ET half completes and what it writes
(the **FU-231** terminal — the end machine, cited here to FU-228); (4) verdict
= the ET-half route + its live preconditions + numbered legs. This extends
FU-62, FU-143, FU-192, FU-211, FU-213, FU-223, FU-224, FU-225, FU-227, FU-228,
FU-230.

---

## Answer in one line

**The ET-half clock route is already the engine's — the FU-62 library selects
`extra_length` for periods `2`/`3` (`fifa96_match_state.c:63-65`) and completes
them by `period++` (`:84-89`), byte-for-byte the native `0x8AFD6..0x8B33F` limit
select + the index-`2`/`3` blocks — but the ET-half *phase rows* `0x13`/`0x14`
are NULL (`fifa96_match_handlers.c:5227,5229`), so the native per-record body
`0x6E244` (the `[0x10F364]`/`[0x10F368]` camera-scatter, referenced only by the
phase slots `0x1107E0`/`0x1107E4`) never runs; the live ET-half behaviour the
engine carries is the FU-142a phase-machine `state 0x13/0x14` arm
(`fifa96_match_phase_machine_step`) plus the counted act-`0xC` → phase-`0x22`
row.** The ET half's **completion write** (`[0x158822]=2`) is the end-machine
terminal `FUN_0008BAF0` phase-`0x13`/`0x14` block (`0x8BD8A`/`0x8BDD4`; FU-228
§2.2, owned whole by **FU-231**/W22-1) — unported in the engine (FU213-L3). A
subtlety worth carrying: both `0x13` and `0x14` are **class `0`** in the phase
class table (`0x1106AD[0x13]=0`, `[0x14]=0`; engine
`fifa96_match_state_class_table[0x13/0x14]=0`), so the clock is **stopped**
while the ET-half screen phase is current — the ET half only ticks in a class-`1`
phase.

---

## 1. The ET-half clock route (FU-62 / FU-223)

### 1.1 The limit select — periods `2`/`3` use `extra_length` (fresh source)

`src/fifa96_loader/fifa96_match_state.c` is the FU-62 clock port. The period
length select is (`:63-65`, read first-hand):

```c
if (s->period < 4u) {
  limit = (s->period < 2u) ? s->period_length : s->extra_length;
}
```

So the engine's `limit` is `state.period_length` (the regulation halves,
periods `0`/`1`) **or** `state.extra_length` (the extra-time halves, periods
`2`/`3`). The four length constants are `fifa96_match_run.h:72..75`
(`DEFAULT 120/40`, `RESET 60/30`), staged by `fifa96_match_run_set_period`
(`fifa96_match_run.c:2425..2432`) from the `begin` `selector`
(`:2366..2372`). FU-223 §1.1/§1.4/§4 pins the values (`selector 0` → `120/40`;
`selector != 0` → `60/30`; both byte-exact to the native `[0x15881A]`/
`[0x15881C]` default).

**Native analog (fresh `disassemble_bytes 0x8AFD0`, 32 B):**

```
0x8AFD0 MOV DL, byte ptr [0x00157AC2]        ; the period index
0x8AFD6 CMP DL, 0x1 ; 0x8AFD9 JBE 0x8AFE2    ; index <= 1 -> regulation
0x8AFDB CMP DL, 0x3 ; 0x8AFDE JBE 0x8AFEB    ; index <= 3 -> extra
0x8AFE0 JMP 0x8AFF2
0x8AFE2 MOV SI, word ptr [0x0015881A]        ; regulation length ([0x15881A])
0x8AFEB MOV SI, word ptr [0x0015881C]        ; extra length      ([0x15881C])
```

The engine's `period < 2 ? period_length : extra_length` **is** the native
`[0x15881A]`/`[0x15881C]` select (`match_state.c:63-65` vs `0x8AFD6..0x8AFEB`);
this is the same equivalence FU-223 §5.2 already pins. No change.

### 1.2 The periods-`2`/`3` completion predicate and the `period++` (fresh)

The completion lives in the same function (`fifa96_match_state.c:77-89`):

```c
if (s->period < 4u) {
  if ((uint32_t)s->period_seconds == (uint32_t)limit + s->aux_seconds) {
    complete = 1;                                  /* :78-79 */
  } else if (s->period_seconds == limit && s->aux_seconds != 0) {
    s->aux_flag = 1;                               /* :80-81 (the [0x15882D] display flag) */
  }
}
if (complete) {
  s->period++;                                     /* :85 */
  s->period_seconds = 0;                           /* :86 */
  *period_ended = 1;                               /* :87 */
  break;
}
```

So on an ET half (`period == 2` or `3`) the completion predicate is
`period_seconds == extra_length + aux_seconds`, and completion advances
`period` to `3` or `4` respectively. The FU-62 `period < 4u` guard (`:77`) is
the native `0x8B1C3 CMP AL,3 ; JA 0x8AFB7` (period index `> 3` → no block; fresh
`disassemble_bytes 0x8B1C3`), and the period-length/aux block is the native
`0x8B1DD..0x8B3A9` dispatch.

**Fresh `disassemble_bytes 0x8B1B0` (80 B) — the dispatch head + index-`0`:**

```
0x8B1B1 MOV EAX,0x11 ; XOR DL,DL ; CALL 0x4C374 ; MOV byte[0x15882D],DL   ; the display-flag clear
0x8B1C3 MOV AL, byte ptr [0x00157AC2]        ; the period index
0x8B1C8 CMP AL, 0x3 ; 0x8B1CA JA 0x8AFB7     ; index > 3 -> loop (no completion)
0x8B1D0 AND EAX, 0xFF
0x8B1D5 JMP dword ptr CS:[EAX*0x4 + 0x8AF28] ; the period-index dispatch
0x8B1DD MOV AX, [0x00157AB6]                 ; index 0: sec ([0x157AB6])
0x8B1E3 MOV BX, word ptr [0x0015881A]        ; limit = [0x15881A]
0x8B1EA CMP AX, BX ; 0x8B1ED JNZ 0x8B205
0x8B1EF CMP word ptr [0x00157ABA], 0x0 ; 0x8B1F7 JZ 0x8B225   ; aux 0 -> complete
0x8B1F9 MOV byte ptr [0x0015882D], 0x1                        ; else display flag
```

Fresh `read_memory 0x8AF28` (16 B) = `dd b1 08 00 | 2f b2 08 00 | e9 b2 08 00 |
3f b3 08 00` = **`{0x8B1DD, 0x8B22F, 0x8B2E9, 0x8B33F}`** (indices `0..3`), the
FU-213 §1 / FU-224 §3.1 table.

### 1.3 The native index-`2`/`3` blocks (fresh — the ET-half completions)

**Index `2` (the first ET half), fresh `disassemble_bytes 0x8B2E9` (120 B):**

```
0x8B2E9 MOV AX, [0x00157AB6]                 ; sec
0x8B2EF MOV BX, word ptr [0x0015881C]        ; limit = [0x15881C]  <<< extra length
0x8B2F6 CMP AX, BX ; 0x8B2F9 JNZ 0x8B315
0x8B2FB CMP word ptr [0x00157ABA], 0x0 ; 0x8B303 JZ 0x8B225    ; aux 0 -> CX=1 (complete)
0x8B309 MOV byte ptr [0x0015882D], 0x1                        ; else display flag
0x8B315.. (sec != limit): EDX = limit + aux; if sec == EDX -> 0x8B335 MOV ECX,1 (complete)
```

So index `2` completing sets `CX=1` (the completion flag) with **no** flag/tiebreak
side-effect.

**Index `3` (the second ET half), fresh `disassemble_bytes 0x8B33F` (150 B):**

```
0x8B33F MOV AX, [0x00157AB6]                 ; sec
0x8B345 CMP AX, word ptr [0x0015881C]        ; limit = [0x15881C]  <<< extra length
0x8B34C JNZ 0x8B364
0x8B34E CMP word ptr [0x00157ABA], 0x0 ; 0x8B356 JZ 0x8B387    ; aux 0 -> 0x8B387
0x8B358 MOV byte ptr [0x0015882D], 0x1                        ; else display flag
0x8B364.. (sec != limit): EDX = [0x15881C] + aux; if sec == EDX -> 0x8B387
0x8B387 MOV AX, [0x00157AC5] ; MOV DX, word ptr [0x00157AC7]
0x8B394 MOV ECX, 1                           ; the completion flag
0x8B399 CMP AX, DX ; 0x8B39C JZ 0x8AFB7      ; scores equal -> hold
0x8B3A2 MOV byte ptr [0x00157AC0], 0x1       ; <<< W4: differing score -> OL-85 flag
```

So index `3` completing sets the completion flag `CX=1` **and** raises the OL-85
flag `[0x157AC0]` when the score pair `[0x157AC5]`/`[0x157AC7]` differs (the
**W4** site, FU-213 §2.3 / FU-224 §3.1). Both index-`2`/`3` blocks read the
**extra** limit `[0x15881C]`, confirming the ET-half route.

### 1.4 The completion exit and the roll (fresh — both ET halves)

After the dispatch the completion exit runs (`0x8B3AE`; fresh
`disassemble_bytes 0x8B3AE`, 60 B): `CMP byte[0x15882D],0 / JZ 0x8B3C7` (display
sink), `TEST CX,CX / JZ 0x8B590` (no completion → the tail), then
`0x8B3D2 MOV AL,[0x157AC2] / CMP EAX,1 / JNZ 0x8B555` — for the ET halves
(index `2`/`3`, `!= 1`) the exit jumps straight to `0x8B555`.

**Fresh `disassemble_bytes 0x8B555` (48 B):**

```
0x8B555 TEST AL, AL ; 0x8B557 JNZ 0x8B574      ; index != 0 -> the chooser
0x8B559.. (index 0 only) MOV EAX,0xC2 ; CALL 0x8F188 ; MOV EAX,0x12 ; CALL 0x4C374
0x8B574 CALL 0x0008B9CC                        ; <<< THE CHOOSER (with the OLD index)
0x8B579 MOV BH, byte ptr [0x00157AC2] ; 0x8B581 INC BH
0x8B583 MOV word ptr [0x00157AB6], SI(0)       ; sec = 0
0x8B58A MOV byte ptr [0x00157AC2], BH          ; <<< the period-index roll (+1)
```

So **at every ET-half completion** the chooser `FUN_0008B9CC` runs with the OLD
period index (`2`, then `3`), `sec` is zeroed, and the index is incremented
(`2→3`, then `3→4`). This is the FU-213 §2.5 mechanism and the FU-224 §3.2 roll,
re-read fresh here at the ET-half indices.

### 1.5 The class-`0` ET-half phases (fresh — a live precondition)

The clock body only runs under a class-`1` phase (or class `2` with the halt
gate clear). Fresh `read_memory 0x1106AD` (48 B) is the phase **class table**
(`DI = byte[0x1106AD + phase]`; FU-213 §1). Decoding it:

| phase | `0x02` | `0x0D` | **`0x13`** | **`0x14`** | `0x17` | `0x1D` |
|---|---|---|---|---|---|---|
| class | 1 | 2 | **0** | **0** | 1 | 1 |

So **both ET-half phases `0x13`/`0x14` are class `0`** — the clock is
**stopped** while the ET-half screen phase is the current phase
(`0x1106AD[0x13]=0`, `[0x14]=0`; byte-identical to the engine's
`fifa96_match_state_class_table` at `fifa96_match_state.c:4-8`, where index
`0x13`/`0x14` are `0`). The ET halves therefore only accrue seconds **in a
class-`1` phase** (e.g. phase `2`/`0x17`/`0x1D`); the `0x13`/`0x14` screen is a
hold. This is the native
`0x8AF75 MOVSX EDX,DI ; CMP EDX,0x1 ; JZ 0x8AF86 ; TEST AX,AX ; JZ 0x8B590`
(FU-213 §2.1). (FU-230 §1.4 observes the same: the tape's clock runs only in the
20-step phase-`2` window.)

---

## 2. The phase-`0x13`/`0x14` rows (engine + native slots)

### 2.1 The engine rows are NULL (fresh source)

`src/fifa96_engine/fifa96_match_handlers.c`, the phase handler table
`fifa96_match_phase_table[FIFA96_MATCH_PHASE_ROWS]` (`:5188`):

* **`{0x13, NULL, "FU-192 §2.10/L2 … phase 13 = the native 0x6E244
  (0x6E244..0x6E32A), the camera-scatter body. LEGGED (R10): it consumes the
  PRNG 0x92AC8 twice per active record and reads 0x10F364/0x10F368, whose
  producer is the unported FUN_0008D098 case-0x13/0x14 scatter … The slot stays
  NULL/uncounted; OL-13" }` (`:5227`).
* **`{0x14, NULL, "… phase 14 = the native 0x6E244 (same as row 13). LEGGED
  (R10, on-tape 40 frames): the RNG consumption + the unported
  0x10F364/0x10F368 producer (FU192-L2). The slot stays NULL/uncounted;
  OL-13" }`** (`:5229`).

The per-record walk is `match_run_write_phase` (`fifa96_match_run.c:2512-2532`),
which runs `fifa96_match_run_record_phase_walk` **only when the row's `fn !=
NULL`** (`:2526-2530`) — so for phase `0x13`/`0x14` the walk is a **structural
no-op**. (The FU-213 §6 parity table cites "phase `0x13`/`0x14` rows … `NULL` |
`fifa96_match_handlers.c:5097,5099`" — the addresses `5097`/`5099` are the
**action** table (`fifa96_match_action_table` at `:5097`); the **phase** rows are
at `:5227`/`:5229`. Recorded as an erratum candidate; owner FU-213/FU-192.)

### 2.2 The native phase table and the ET-half body `0x6E244` (fresh)

The native phase **handler** table base is `0x110794` (indexed by phase; the act
slots are its tail `0x110794 + 0x16*4 = 0x1107EC`, FU-227 §2.4). Fresh
`read_memory 0x110794` (128 B; 32 dwords) decodes to:

| idx | handler | idx | handler |
|---|---|---|---|
| `0x00` | `0x6DE34` | `0x10` | `0x6E004` |
| `0x01` | `0x6E1D0` | `0x11` | `0x6E1C8` |
| `0x02` | `0x6DCC8` | `0x12` | `0x6E1D0` |
| `0x03`/`0x04` | `0x6DE44` | **`0x13`** | **`0x6E244`** |
| `0x05` | `0x6E05C` | **`0x14`** | **`0x6E244`** |
| `0x06` | `0x6DD9C` | `0x15` | `0x6DCC8` |
| `0x07` | `0x6DE44` | `0x16` | `NULL` (INT3 stub) |
| `0x08`/`0x09` | `0x6DD6C` | `0x17` | `0x88DC8` (act 1) |
| `0x0A`/`0x0B` | `0x6DE34` | `0x18` | `0x8922C` (act 2) |
| `0x0C` | `0x6DF4C` | `0x19` | `0x89FA4` (act 3) |
| `0x0D`/`0x0E`/`0x0F` | `0x6DE34` | `0x1A` | `0x89620` (act 4) |

Fresh `read_memory 0x1107E0` (8 B) = `44 e2 06 00 | 44 e2 06 00` = **`0x6E244`,
`0x6E244`** — the two **phase-`0x13`/`0x14` slots** (the brief's
"phase-table slots"): both point at the same body. Fresh `get_xrefs_to 0x6E244`
= **2** — the ONLY references are the DATA slots `0x1107E0`/`0x1107E4`; fresh
`get_function_by_address 0x6E244` = **none** (a raw, otherwise-unclaimed body).

Fresh `disassemble_bytes 0x6E244` (200 B) — the ET-half per-record handler:
`MOV EDX,[0x157AAC] >> 24` (the controlled side), `[EAX]` → `[EAX+0x826]` (the
record's team side) compared to the side; then `[0x10F368]` (BX) / `[rec+0x8D]`
/ `[0x10F364]` (SI); the `0x92AC8` **PRNG** twice with divisor `0x90`; and the
target write `MOV dword[ECX+0x8],EAX` (the scatter offset). So the ET-half row
body is a per-record **camera-scatter offset** driven by two process cells
(`0x10F364`/`0x10F368`) whose producer is the unported `FUN_0008D098`
case-`0x13`/`0x14` (FU-192 §2.10 / FU192-L2). It is **render-visible** (a target
triple) and **RNG-consuming**, hence left NULL/uncounted.

### 2.3 The act-slot anchors `0x1107EC` / `0x110808` (fresh)

Fresh `read_memory 0x1107EC` (52 B; 13 dwords) — the **act** handler slot table
`0x110794[0x16 + act]` (act `0..0xC`):

| act | slot | handler | act | slot | handler |
|---|---|---|---|---|---|
| `0x0` | `0x1107EC` | `NULL` (phase-`0x16` INT3) | `0x7` | `0x110808` | `0x89868` (phase `0x1D`) |
| `0x1` | `0x1107F0` | `0x88DC8` (phase `0x17`) | `0x8` | `0x11080C` | `0x8A798` (phase `0x1E`) |
| `0x2` | `0x1107F4` | `0x8922C` (phase `0x18`) | `0x9` | `0x110810` | `0x88F4C` (phase `0x1F`) |
| `0x3` | `0x1107F8` | `0x89FA4` (phase `0x19`) | `0xA` | `0x110814` | `0x8B688` (phase `0x20`) |
| `0x4` | `0x1107FC` | `0x89620` (phase `0x1A`) | `0xB` | `0x110818` | `0x8B874` (phase `0x21`) |
| `0x5` | `0x110800` | `0x890EC` (phase `0x1B`) | `0xC` | `0x11081C` | `0x8B900` (phase `0x22`) |
| `0x6` | `0x110804` | `0x89110` (phase `0x1C`) | | | |

So the brief's two anchors are: **`0x1107EC`** = the act-table **base** (act `0`,
the phase-`0x16` INT3 slot) and **`0x110808` = `0x1107EC + 0x1C`** = the **act-7
slot** → `0x89868` (**phase `0x1D`**, the extra-time sit-8 arm). They are the
act-side anchors, not the phase-`0x13`/`0x14` rows (those are `0x1107E0`/
`0x1107E4`, §2.2). The **act-`0xC`** slot (`0x11081C → 0x8B900`, phase `0x22`)
is the ET-half-end timer the chooser installs on the extra-time arm (§3.3).

### 2.4 The engine's live ET-half behaviour (the rows the machine already carries)

Because the phase-`0x13`/`0x14` rows are NULL, the native per-record `0x6E244`
does not run in the engine. The behaviour the engine **does** carry for the ET
half is:

* **The FU-142a phase-machine arm.** Writing phase `0x13`/`0x14` sets both
  `mr->state` and `mr->phase_machine` (`match_run_write_phase`,
  `fifa96_match_run.c:2512-2516`); `fifa96_match_phase_machine_step`
  (called at `fifa96_match_run_frame` `:4125`) then runs the `FUN_0008D098`
  **state `0x13`/`0x14` arm block** for teams on each side — installing codes
  `0x26`/`0x25`/`3` (phase `0x13`) and `0x28`/`0x2A` (phase `0x14`)
  (`fifa96_match_phase_machine.c:52-101`; FU-142 §Appendix B, OL-49/OL-56).
* **The counted act-`0xC` → phase-`0x22` row.** The chooser's extra-time arm
  returns `out.act = 0x0C`; the engine dispatches it
  (`fifa96_match_run.c:2648`) and the act-tail maps act `0xC` → phase `0x22`
  (`fifa96_action_phase_act`, `fifa96_action_handlers.c:1117`), whose row
  `fifa96_match_action_phase_22` is **COUNTED** (`fifa96_match_handlers.c:5257`,
  FU-211/FU-195). That is the ET-half-end three-stage timer.

So the engine's ET-half **presentation** is the FU-142a arm-install plus the
act-`0xC` timer; the native per-record `0x6E244` scatter and the RNG consume are
the deleted rows (leg, §6).

---

## 3. How an ET half completes and what it writes (the FU-231 terminal)

### 3.1 The completion + the chooser re-write

An ET half (period `2`/`3`) completes exactly like any period: the FU-62 clock
tests `period_seconds == extra_length + aux_seconds` (`match_state.c:78`;
native index-`2` `0x8B2E9` / index-`3` `0x8B33F`, §1.3), advances `period`
(`:85`), and stages `*period_ended = 1` (`:87`). `fifa96_match_run_frame` carries
it as `mr->clock_period_ended` (`fifa96_match_run.c:4032`), and
`fifa96_match_run_phase_drive` consumes it one-shot (`:2603-2604`):

* the **W4** tie-break (`:2628-2629`: `completed == 3 && differ → global_157ac0 = 1`)
  mirrors the native `0x8B3A2`;
* the chooser `fifa96_action_phase_period_end(completed = period-1, …)`
  (`:2633-2636`) — for the extra-time arm (`extra_time != 0`) it returns
  `out.act = 0x0C` and `out.phase = 0x13` or `0x14` (the native `0x8B9CC`
  tie-break, `fifa96_action_handlers.c:1180..1208`);
* the phase write (`match_run_write_phase`, `:2638-2641`) re-installs phase
  `0x13`/`0x14`; the act dispatch (`:2648`) installs act `0xC` (→ phase `0x22`);
* the **roll** `mr->global_157ac2 = mr->state.period` (`:2653`; native `0x8B58A`).

So the ET-half completion re-writes the **phase `0x13`/`0x14` current screen**
and the **act `0xC` timer**, and rolls the period index — exactly the native
`0x8B574 → 0x8B58A` sequence (§1.4).

### 3.2 The end machine's ET terminal (`[0x158822]=2`) — cite FU-228 / FU-231

The ET-half **terminal** (what writes the end-of-match word) is the end machine
`FUN_0008BAF0`'s phase-`0x13`/`0x14` block (`0x8BCE7..0x8BDDD`). Per FU-228 §2.2
(*cross-cited*; the whole body is **FU-231**/W22-1):

* the block accumulates the end timer `word[0x158818] += [0x157A64]` and applies
  the `word[0x158818]` timeline gates (FU-228 §2.2 / **FU228-L2** erratum — the
  cell is `word[0x158818]`, which FU-213 §5 renders as `[0x15881A]`);
* `0x8BD45 EAX = phase ; CMP EAX,0x14 ; JNZ 0x8BD95` — the phase-`0x14` arm
  `0x8BD81 CMP dword[0x10F360],0 ; JNZ 0x8BDDD ; 0x8BD8A MOV word[0x158822],2`
  (W-e, ET over);
* the phase-`0x13` arm `0x8BD95 MOV AX,[0x157AC5] ; CMP AX,[0x157AC7] ; JNZ hold`
  (scores differ → hold) else `0x8BDD4 MOV word[0x158822],2` (W-f, ET over),
  `0x8BDDD MOV dword[0x10F360],1` (the ET-over latch).

So an ET half "completes and writes" `[0x158822]=2` (ET over) via the
end-machine terminal — the **FU-231** end-machine body, cited to **FU-228 §2.2**.
The engine does **not** model the end word or `FUN_0008BAF0` (FU-228 §3 /
FU213-L3: `[0x158822]` has no engine cell; fresh `get_xrefs_to 0x158822` = **10**
— 3 setters in `FUN_0008BAF0` `0x8BCD9`/`0x8BD8A`/`0x8BDD4`, 5 clears, 2 reads,
matching FU-228 §2.1). The terminal *decision* (tie→shootout) is **FU-237**/W22-7;
this slice only places the ET-half completion and cites the write.

---

## 4. Verdict — the ET-half route + its live preconditions

**Route (statically complete, engine-side already faithful):**

```
period in {2,3}  (ET halves)
  clock limit  = state.extra_length        [fifa96_match_state.c:64; native 0x8AFD6-0x8AFEB]
  completion   = period_seconds == extra_length + aux_seconds   [:78; native 0x8B2E9 / 0x8B33F]
  completion   -> period++  (2->3, 3->4)   [:85; native 0x8B58A roll]
  frame        -> clock_period_ended=1     [:4032]
  phase_drive  -> W4 (index 3, differ)     [:2628-2629; native 0x8B3A2]
               -> chooser(index 2/3) -> phase 0x13/0x14 + act 0xC  [:2633-2648]
               -> roll global_157ac2 = state.period                [:2653; native 0x8B58A]
  end machine  -> phase 0x13/0x14 terminal writes [0x158822]=2     [FU-228 §2.2; FU-231]
```

**Live preconditions:**

1. **A class-`1` phase must be current** for the ET half to tick (the ET-half
   screen phase `0x13`/`0x14` is class `0`; §1.5). The ET halves accrue seconds
   in a class-`1` phase (phase `2`/`0x17`/`0x1D`), not during the `0x13`/`0x14`
   hold.
2. **The OL-85 flag `[0x157AC0]` must be set** for the chooser to take the
   extra-time arm (`out.phase = 0x13/0x14`, `out.act = 0x0C`); otherwise the
   no-ET arm yields phase `0x0C`/act `0xB` (`fifa96_action_handlers.c:1170-1178`).
   The engine's producer is the W2/W4 tie-break (`:2628-2629`) plus the W5
   mid-ET set (`:2672-2674`; FU-213/FU-224).
3. **The period index reaches `2`/`3`** — i.e. two/three prior completions (the
   roll) or the front-end force (`0x38990`). Whether a *tied* regulation end
   reaches ET is conditional on the unported `[0x14C2EE]` override (FU213-L1 /
   FU224-L3 / FU230-L2).
4. **The ET-half per-record body `0x6E244` has no engine row** — the row is NULL
   and its `[0x10F364]`/`[0x10F368]` producer is unported (FU-192-L2). The
   engine's ET-half presentation is the FU-142a arm-install + the counted
   act-`0xC`/phase-`0x22` timer.

**No census move:** the change is a NULL row + an already-faithful clock branch;
no wire count changes (`floor(100 × 55/80) = 68`).

---

## 5. Residual legs (numbered; each with its owner)

1. **FU233-L1 — the phase-`0x13`/`0x14` rows are NULL; the native `0x6E244`
   per-record camera-scatter body is unported.** Fresh `get_xrefs_to 0x6E244` =
   2 (the DATA slots `0x1107E0`/`0x1107E4`); `get_function_by_address 0x6E244` =
   none. The body reads `[0x157AAC]>>24`, `[rec]→[+0x826]`, `[rec+0x8D]`,
   `[0x10F364]`/`[0x10F368]` and consumes the PRNG `0x92AC8`. Owner: the
   phase-`0x13`/`0x14` body slice (extends FU192-L2/FU213-L8).
2. **FU233-L2 — the `[0x10F364]`/`[0x10F368]` producer.** Fresh `read_memory`
   of the body shows both read; FU-192 §2.10 attributes their producer to
   `FUN_0008D098` case-`0x13`/`0x14` (the FU-142a arm). Whether the engine's
   FU-142a arm sets an equivalent cell is not proven here. Owner: FU-192/FU-142a.
3. **FU233-L3 — the ET-half terminal `[0x158822]=2` / `FUN_0008BAF0` is
   unported.** Fresh `get_xrefs_to 0x158822` = **10** (setters `0x8BCD9`/
   `0x8BD8A`/`0x8BDD4`); the engine has no cell and the lifecycle owns exits
   (FU-228 §3 / FU213-L3). Owner: **FU-231** (the end machine, W22-1) + FU-237
   (the terminal decision, W22-7).
4. **FU233-L4 — the ET-half phases `0x13`/`0x14` are class `0` (clock stopped).**
   Fresh `0x1106AD[0x13]=0`, `[0x14]=0`; the ET half therefore only accrues
   seconds in a class-`1` phase. Which phase actually drives the live ET-half
   clock (phase `2`? `0x17`? `0x1D`?) is **not** pinned by this static read — a
   period-end trace owns it. Owner: the period/lifecycle slice (FU-213 family).
5. **FU233-L5 — the periods-`2`/`3` completion is untested on the M2 tape.**
   FU-230 §1.4 records that the v20 golden carries no completion; the ET halves
   are only reachable after the regulation-end/ET-entry transition (FU-230 §2).
   The completion schedule (`extra_length` + the class-2 aux growth) is the T1
   probe's. Owner: FU-230 / the phase-22 tape extension.
6. **FU233-L6 — the FU-213 §6 parity line cites the wrong addresses for the
   phase-`0x13`/`0x14` rows.** FU-213 §6 says "phase `0x13`/`0x14` rows … NULL |
   `fifa96_match_handlers.c:5097,5099`"; those are the **action** table
   (`:5097`) and action row `0x00` (`:5099`). The **phase** rows are `:5227`/
   `:5229`. Erratum candidate (prior text is not rewritten). Owner: FU-213.

---

## 6. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `game/hdd/FIFA96/FIFA96.EXE`, `watcom:LE:32:default`, 2706 functions, 15239
  symbols, image base 0, min `0x10000`, max `.image::00174a2a`, `.image` overlay.
* `read_memory` — `0x110794` (128 B; the phase table, 32 dwords),
  `0x1107E0` (8 B = `0x6E244`,`0x6E244`), `0x1107EC` (52 B; the act slots
  `0..0xC`), `0x1106AD` (48 B; the class table), `0x8AF28` (16 B; the
  period-index jump table).
* `disassemble_bytes` — `0x8AFC0` (64 B; the limit select), `0x8B1B0` (80 B; the
  dispatch head + index 0), `0x8B2E9` (120 B; index 2), `0x8B33F` (150 B; index
  3), `0x8B3D0` (96 B; the completion exit + index-1 decision), `0x8B555` (48 B;
  the chooser call + roll), `0x6E244` (200 B; the ET-half per-record body).
* `get_xrefs_to` — `0x6E244` (**2**, both DATA: `0x1107E0`/`0x1107E4`),
  `0x158822` (**10**).
* `get_function_by_address` — `0x6E244` = **none**.

Engine/loader source read first-hand:
`src/fifa96_loader/fifa96_match_state.c` (whole, 103 lines; the class table
`:4-8`; the limit select `:63-65`; the completion + `period++` `:77-89`),
`include/fifa96_loader/fifa96_match_state.h` (whole; `:6-7` the step/second
constants), `src/fifa96_engine/fifa96_match_handlers.c` (`:5188` the phase table;
`:5227`/`:5229` the NULL rows `0x13`/`0x14`; `:5257` the counted phase-`0x22`
row; `:5261-5285` the dispatchers),
`src/fifa96_loader/fifa96_action_handlers.c` (`:1114-1119`
`fifa96_action_phase_act`; `:1163-1210` the chooser),
`src/fifa96_engine/fifa96_match_run.c` (`:2512-2532` `match_run_write_phase` +
the NULL-row no-op; `:2592-2694` `fifa96_match_run_phase_drive` incl. the W2/W4
tie-break `:2628-2629`, the chooser `:2633`, the act dispatch `:2648`, the roll
`:2653`, the W5/period-4 tail `:2668-2694`; `:3183-3228`
`fifa96_match_run_period_setup`; `:4032` the `clock_period_ended` staging;
`:4125` the phase-machine step), `src/fifa96_engine/fifa96_match_phase_machine.c`
(whole, 156 lines; `:52-101` the state-`0x13`/`0x14` arm),
`include/fifa96_engine/fifa96_match_run.h` (`:587` `clock_period_ended`; `:610`
`global_157ac2`; `:622` `global_157ac0`; `:72-75` the length constants).

Docs cross-cited: FU-62, FU-64, FU-142 (§App. B), FU-143, FU-192 §2.10/L2,
FU-195, FU-211, **FU-213** (§1/§2.3/§2.5/§5/§6/§8), FU-223 (§1/§4/§5),
FU-224 (§1/§3), FU-225, FU-227 (§2.4), **FU-228** (§2.2/§3/FU228-L2), FU-230
(§1.4/§2), FU-231 (the end machine, W22-1 — same wave), FU-237 (W22-7).
`docs/ENGINE.md:7-9` (the `55/80` census); README title `68%`. No writes beyond
this slice file; no rename/label/comment/function/script/project save;
`/FIFA96.EXE` and the ISO untouched; no git commands.

---

## Landing note

The ET-half **clock route** is complete and already faithful: the FU-62 library
selects `extra_length` for periods `2`/`3` (`fifa96_match_state.c:63-65`) and
completes them by `period++` (`:84-89`), the byte-for-byte native
`0x8AFD6..0x8AFEB` limit select + the `0x8B2E9`/`0x8B33F` index-`2`/`3` blocks,
with the W4 tie-break (`0x8B3A2`) on the second ET half. The ET-half **phase
rows** `0x13`/`0x14` are NULL (`fifa96_match_handlers.c:5227,5229`); the native
per-record body `0x6E244` (referenced only by the phase slots `0x1107E0`/
`0x1107E4`; the act anchors `0x1107EC`/`0x110808` are the act-table base and the
act-7 → phase-`0x1D` slot) is unported, and its `[0x10F364]`/`[0x10F368]`
producer is the unported `FUN_0008D098` case. The engine's live ET-half
behaviour is the FU-142a phase-machine arm plus the counted act-`0xC` →
phase-`0x22` row. The ET-half **terminal write** `[0x158822]=2` is the end
machine `FUN_0008BAF0`'s phase-`0x13`/`0x14` block (FU-228 §2.2; **FU-231**
owns the body, FU-237 the decision) — unported (FU213-L3). Key live
precondition: the ET-half screen phase `0x13`/`0x14` is **class `0`** (clock
stopped), so the ET half ticks only in a class-`1` phase. No census move;
FU233-L1..L6 are the residual legs.
