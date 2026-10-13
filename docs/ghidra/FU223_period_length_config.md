# FU-223 — the period-length + config producer: the clock length is a match-init constant the engine already stages (phase-21 wave W21-1)

Provenance: M2 phase-21 Stage-1 recon slice **W21-1**, 2026-10-13, read-only.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument; confirmed this session by
`get_current_program_info` — `FIFA96.EXE` / path `/FIFA96.EXE`, executable
`game/hdd/FIFA96/FIFA96.EXE`, `Linear Executable (LE-Style DOS)`,
`watcom:LE:32:default`, image base `0`, min `0x10000`, max
`.image::00174a2a`, **2706** functions, **15239** symbols, image size 2757479,
`.image` overlay present). Every address/byte/instruction/xref-count below is a
**fresh** `read_memory` / `disassemble_bytes` / `get_xrefs_to` /
`get_function_xrefs` of **this** session unless marked *cross-cited* (then the
frozen FU doc is named). The alias traps are applied; LE globals sit at
`+0x100000` from the file offset; **addresses are `/FIFA96.EXE` flat/runtime
addresses** (`[0x15881A]`, not FU-62's encoded `[0x5881A]` — `+0x100000`; the
program displays the flat form). Engine/loader source read first-hand. **No
writes**: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; the ISO / `game/hdd/` untouched; no git commands.

Scope (W21-1 brief): decide what sets a live match's period length (the FU-62
clock's length `limit` and its `aux` add-on) and whether the engine's staged
default suffices or a producer must be ported. Extends **FU-62 §4**, **FU-68
§4**, and settles **FU-213-L5**; corrects the brief's premise that the
`0x105270`-family option block and the range words `[0x14C1D4]`/`[0x14C1D6]`
feed the clock length. Cross-slice: Task 2 (FU-224) owns the cells
`[0x157AC2]`/`[0x157AC0]`; Task 3 (FU-225) owns the chooser `FUN_00088860`;
this slice establishes its own facts and depends on neither draft.

**Answer in one line.** A live period length is **(b) a match-init constant the
engine already stages**: the clock reads **`[0x15881A]`** (regular periods 0/1)
and **`[0x15881C]`** (extra periods 2/3), written only by `FUN_0003749C`
(pre-match settings hand-off, FU-68), `FUN_000886D4` (match start re-derive) and
`FUN_0004B508` (60 s/30 s reset override) — all three from the configured
half-length minutes **`[0x14C1D1]`**. The default half (settings index `0x0E` =
0 → table `0x37170[0]` = 2 min) yields `[0x15881A] = 2*0x3C = 120` and
`[0x15881C] = 2*0x14 = 40`, **exactly the engine's staged default**
(`state.period_length = 120`, `state.extra_length = 40`,
`fifa96_match_run.c:2368..2372`). **No producer is to be added** for the
ordinary live match; the only unported input is the front-end half-length menu
(`settings[0x0E]`, FU-65), and its translator `fifa96_settings_handoff` is
already ported (FU-68). The brief's two "clock-completion input" cells are **not
length producers** — they are the input/control-options block and the range
words feeding the sit-8 chooser gate (Task 3), not the clock. Census does not
move; no golden re-pin.

---

## 1. The engine's current clock/config model (first-hand)

### 1.1 The FU-62 clock consumes `state.period_length` / `state.extra_length`

`src/fifa96_loader/fifa96_match_state.c` is the FU-62 clock port. The period-end
completion lives in `fifa96_match_state_frame` (`:47..92`); the length select is
(`:63..65`):

```c
uint16_t limit = 0;
if (s->period < 4u)
  limit = (s->period < 2u) ? s->period_length : s->extra_length;
...
if ((uint32_t)s->period_seconds == (uint32_t)limit + s->aux_seconds) {  /* :78 */
  complete = 1;                                                        /* :79 */
}
```

So the engine's `limit` **is** `state.period_length` (periods 0/1) /
`state.extra_length` (periods 2/3), and `aux` **is** `state.aux_seconds`. The
class-2 aux add-on accumulates (`:66..73`: `aux_tick` saturates at `0x1E` then
`aux_seconds++`), matching the native `[0x157ABC]`/`[0x157ABA]` pair. These are
the direct analogs of the native `[0x15881A]`/`[0x15881C]` and
`[0x157ABA]`/`[0x157ABC]` (FU-213 §3 table, *cross-cited*).

### 1.2 How `begin` stages the lengths (the exact current values)

`fifa96_match_run_begin` (`src/fifa96_engine/fifa96_match_run.c`) installs two
independent copies:

* **The staged match config** (`:2249..2253`): a *local* default settings block
  is hand-off'd into `mr->config`:
  ```c
  struct fifa96_settings settings;
  fifa96_settings_defaults(&settings);
  (void)fifa96_settings_handoff(&settings, 0u, &mr->config);
  ```
  With defaults `settings.value[0x0E] = 0` (`fifa96_settings.c:14..30`), the
  hand-off picks `half = fifa96_match_config_half_lengths[0] = 2`
  (`fifa96_settings.c:69..70, :95..99`) and writes
  `config.period_length = half*0x3C = 120`, `config.extra_length = half*0x14 = 40`.

* **The clock lengths** (`:2366..2372`): from the `selector` argument, **not**
  from `mr->config`:
  ```c
  uint16_t period = selector == 0 ? FIFA96_MATCH_RUN_PERIOD_SECONDS_DEFAULT  /* 120 */
                                  : FIFA96_MATCH_RUN_PERIOD_SECONDS_RESET;   /* 60  */
  uint16_t extra  = selector == 0 ? FIFA96_MATCH_RUN_EXTRA_SECONDS_DEFAULT   /* 40  */
                                  : FIFA96_MATCH_RUN_EXTRA_SECONDS_RESET;    /* 30  */
  (void)fifa96_match_run_set_period(mr, period, extra);
  ```
  `fifa96_match_run_set_period` (`:2425..2432`) writes
  `mr->state.period_length = period_seconds; mr->state.extra_length =
  extra_seconds;`.

`include/fifa96_engine/fifa96_match_run.h:72..75` fixes the four constants
(`DEFAULT 120/40`, `RESET 60/30`). So a **selector-0 (the menu/boot path)
live match** runs `limit = 120 s` (periods 0/1) and `140 s` (periods 2/3: extra
length 40 s), and a non-zero selector runs `60/30`.

### 1.3 How `mr.clock_period_ended` is raised and consumed

`fifa96_match_run_frame` (`:4014..4040`) drives one 100 Hz pace tick through
`fifa96_match_state_tick(...)` (`:4026`), whose `period_ended` output is staged
verbatim (`:4032`):

```c
mr->clock_period_ended = (uint8_t)period_ended;
```

`fifa96_match_run_phase_drive` (`:2584..`) consumes it **one-shot** (`:2592..2594`):
`ended = mr->clock_period_ended; mr->clock_period_ended = 0;`. The completion is
therefore a per-granted-frame event the FU-62 library derives; the run carries
only the flag across the frame boundary (FU-143 wiring, *cross-cited*).

### 1.4 Recorded engine values (fresh source read)

| engine cell | value (selector 0) | value (selector != 0) | native analog |
|---|---|---|---|
| `state.period_length` | **120** | **60** | `[0x15881A]` |
| `state.extra_length` | **40** | **30** | `[0x15881C]` |
| `mr->config.period_length` | **120** | 120 (unwired) | (same native derive) |
| `mr->config.extra_length` | **40** | 40 (unwired) | (same native derive) |
| `state.aux_seconds` | 0 → accumulated | 0 → accumulated | `[0x157ABA]` |
| `state.aux_tick` | 0 → 0..0x1E | 0 → 0..0x1E | `[0x157ABC]` |

---

## 2. The native live period-length producer chain (fresh)

### 2.1 The cells are BSS zero at file load

Fresh `read_memory 0x14C1D0` (8 B) → all zero; `read_memory 0x158818` (16 B) →
all zero. So `[0x14C1D1]` (half-length minutes), `[0x15881A]`/`[0x15881C]`
(period/extra seconds) are **runtime-built**, not imaged.

### 2.2 The three writers of `[0x15881A]` / `[0x15881C]`

Fresh `get_xrefs_to 0x15881A` = **21** refs and `get_xrefs_to 0x15881C` = **9**
refs. The write sites are **exactly three functions**, both cells:

| # | site | function | instruction | effect |
|---|---|---|---|---|
| W1 | `0x375E4` | `FUN_0003749C` | `IMUL EAX,EDX,0x3c / MOV [0x15881A],AX` | `:= [0x14C1D1]*0x3C` |
| W2 | `0x375ED` | `FUN_0003749C` | `IMUL EAX,EDX,0x14 / MOV [0x15881C],AX` | `:= [0x14C1D1]*0x14` |
| W3 | `0x88710` | `FUN_000886D4` | `IMUL EAX,EDX,0x3c / MOV [0x15881A],AX` | `:= [0x14C1D1]*0x3C` (re-derive) |
| W4 | `0x88730` | `FUN_000886D4` | `IMUL EAX,EDX,0x14 / MOV [0x15881C],AX` | `:= [0x14C1D1]*0x14` (re-derive) |
| W5 | `0x4B514` | `FUN_0004B508` | `MOV EDX,0x3c / MOV [0x15881A],DX` | `:= 60` (override) |
| W6 | `0x4B51B` | `FUN_0004B508` | `MOV EBX,0x1e / MOV [0x15881C],BX` | `:= 30` (override) |

Fresh `disassemble_bytes 0x375A9` (80 B) shows the W1/W2 tail verbatim
(`0x375db MOV DL,byte[0x14C1D1]`; `0x375e1 IMUL EAX,EDX,0x3c`;
`0x375e4 MOV [0x15881a],AX`; `0x375ea IMUL EAX,EDX,0x14`;
`0x375ed MOV [0x15881c],AX`). Fresh `disassemble_bytes 0x886D4` (112 B) shows
W3/W4 (`0x88701 MOV DL,byte[0x14C1d1]`; `0x8870d IMUL EAX,EDX,0x3c`;
`0x88710 MOV [0x15881a],AX`; `0x88716 IMUL EAX,EDX,0x14`;
`0x88730 MOV [0x15881c],AX`). Fresh `disassemble_bytes 0x4B508` (48 B) shows
`MOV EDX,0x3c / MOV EBX,0x1e / MOV [0x15881a],DX / MOV [0x15881c],BX / RET`.
**No other writer exists** (every remaining ref is a READ; the fresh xref lists
are exhaustive for this session).

### 2.3 The configured half-length minutes `[0x14C1D1]`

Fresh `get_xrefs_to 0x14C1D1` = **6** refs: writes `FUN_0003749C` `0x375A1`
(=1, the competition arm) and `0x375D4` (=`table[settings[0x0E]]`), a read in
`FUN_0003749C` `0x375DB`, and reads `FUN_000886D4` `0x88701`, `0x8F6CC` (no
Ghidra function) and `FUN_0004B564` `0x4B5A2`. Fresh `disassemble_bytes 0x375A9`
shows the settings arm:

```
0x375c7 MOV EAX,0xe / 0x375cc CALL 0x1d940          ; EAX = settings[0x0E]
0x375d1 MOV AL, byte ptr [ESP + EAX*4]              ; half-length table[idx]
0x375d4 MOV [0x14c1d1], AL
```

Fresh `read_memory 0x37170` (32 B) = dwords **`{2,4,6,8,10,20,45,0}`** — the
half-length-minutes table (`fifa96_match_config_half_lengths`,
`fifa96_settings.c:69..70`, *cross-cited* FU-68 §4.1). The competition arm
(`[0x14C32A] != 0`) instead forces `[0x14C1D1] = 1` (`0x375a1`, fresh).

### 2.4 The clock's length read (fresh)

`FUN_0008AF38` (the clock/period machine) selects the limit from the period
index `[0x157AC2]` — fresh `disassemble_bytes 0x8AFD0` (32 B):

```
0x8afd0 MOV DL, byte ptr [0x157ac2]
0x8afd6 CMP DL, 0x1 ; 0x8afd9 JBE 0x8afe2           ; period <= 1 -> regular
0x8afdb CMP DL, 0x3 ; 0x8afde JBE 0x8afeb           ; period <= 3 -> extra
0x8afe2 MOV SI, word ptr [0x15881a]                 ; regular length
0x8afeb MOV SI, word ptr [0x15881c]                 ; extra length
```

and the period-end case 0/1 compares `[0x157AB6]` (period seconds) to
`[0x15881A]` — fresh `disassemble_bytes 0x8B1D0` (80 B):

```
0x8b1dd MOV AX, [0x157ab6] ; 0x8b1e3 MOV BX, word ptr [0x15881a]
0x8b1ea CMP AX, BX ; 0x8b1ed JNZ 0x8b205
0x8b1ef CMP word ptr [0x157aba], 0x0 ; 0x8b1f7 JZ 0x8b225   ; aux==0 -> complete
0x8b1f9 MOV byte ptr [0x15882d], 0x1                        ; else display flag
0x8b207 XOR EAX,EAX / MOV DX, word ptr [0x157aba] / MOV AX,BX
0x8b213 ADD EDX,EAX / MOV AX, [0x157ab6] / CMP EAX,EDX      ; sec == length+aux -> complete
```

This is the native condition the FU-62 port reproduces (engine §1.1). Cases 2/3
read `[0x15881C]` (`0x8B2EF`/`0x8B345`/`0x8B368` — fresh `get_xrefs_to`).

### 2.5 When each writer runs

* `FUN_0003749C` (the settings→match-config translator) — fresh
  `get_function_xrefs 0x3749C` = **6** callers: `0x11BA1` (`FUN_00011B7C`, the
  main pre-match path), `0x115B6` (`FUN_000115A0`), `0x1D8B6`
  (`FUN_0001D7CC`), `0x1DCA3` (`FUN_0001DC80`, code-11 settings reload),
  `0x1DD12` (`FUN_0001DCAC`, settings loader), `0x384C7` (`FUN_00038448`).
  Every settings (re)load refreshes the lengths (*cross-cited* FU-68 §4).
* `FUN_000886D4` (match-start re-derive) — fresh `get_function_xrefs` = **2**:
  `0x74089` (inside `FUN_00074034`, the match begin/reset) and `0x74298` (the
  period-end family body, no Ghidra function — FU-196-L6 family).
* `FUN_0004B508` (60/30 override) — fresh `get_function_xrefs` = **1**: called
  from the match reset `FUN_0004A228` at `0x4A26D`, gated
  `0x4A264 CMP dword ptr [0x1072F8],0 ; JZ skip` (fresh `disassemble_bytes
  0x4A250`). So the override fires **only when `[0x1072F8] != 0`** (a
  match-setup cell; fresh `get_xrefs_to 0x1072F8` = 5, written by
  `FUN_000493A0` `0x493AB`). This confirms FU-62 §4.6's "`FUN_0004B508`
  overrides to 0x3C/0x1E" and the engine comment's "reset override 0x3C/0x1E".

---

## 3. The brief's two "clock-completion input" cells are NOT length producers

The W21-1 brief names the `0x105270`-family config block (**FU215-L5**) and the
range words `[0x14C1D4]`/`[0x14C1D6]` (**FU208-L1/FU215-L1**) as "the native
clock-completion inputs". First-hand, **neither feeds the clock length**:

* `[0x105270]` (fresh `get_xrefs_to` = **2**: read `FUN_000156B0` `0x15835`,
  write `FUN_00024B00` `0x257CA`; fresh `read_memory 0x105270` = 64 B zero at
  file load) is the **input/control-options block** (the eight config cells
  `0x105278/7C/88/8C/90/9C/A0/A4`, FU-148 §12, *cross-cited* FU215 §2.2), read
  by `FUN_0001C9BC` to build the per-side range words — not by `FUN_0008AF38`.
* `[0x14C1D4]` (fresh `get_xrefs_to` = **21**; writers `FUN_0001C9BC` and
  `FUN_00024B00`; fresh `read_memory 0x14C1D4` = 8 B zero) is the **side-0
  input range word**; the sit-8 gate reads it — fresh
  `disassemble_bytes 0x4B0B8` (40 B):
  ```
  0x4b0bc MOV CX, word ptr [0x14c1d4] ; 0x4b0c3 MOV AX, [0x14c1d6]
  0x4b0c9 OR EAX,ECX ; 0x4b0cb TEST AH,0x1 ; 0x4b0ce JZ 0x4b0e5
  ```
  `TEST AH,1` is the **bit-8 dead gate** (FU-215 §2), and the indexed
  `[0x14C1D4 + side*2]` reads (fresh refs `0x77EDE`, `0x8C7A6`, `0x7A6A9`,
  `0x7B8F9`, `0x7B41F`, `0x7BD19`) plus the period-end family body `0x742F5`
  feed the **sit-8 chooser path** (`FUN_0004B02C`, `FUN_00088860` — Task 3),
  not the clock length.

So the range-word/options chain is the **input-control model + the period-end
chooser gate**; the **clock length** is the separate `[0x14C1D1] →
[0x15881A]`/`[0x15881C]` chain of §2. The brief conflated the two "period-end"
surfaces (the chooser that *fires* at a period end vs. the length that
*terminates* the period).

---

## 4. The decisive answer

**A live period length is (b) a match-init constant the engine already stages**,
with the qualification that its *origin* is a front-end settings option:

* The live cell the clock reads is **`[0x15881A]`** (regular, periods 0/1) /
  **`[0x15881C]`** (extra, periods 2/3), produced from the configured minutes
  **`[0x14C1D1]`** (= `table[settings[0x0E]]`, table flat `0x37170`; or 1 min
  for the type-4 competition arm) by `FUN_0003749C` (pre-match) and re-derived
  by `FUN_000886D4` (match start), with the `FUN_0004B508` 60/30 override under
  `[0x1072F8] != 0`. Exact addresses: **`0x15881A`/`0x15881C`** (§2.2),
  minutes source **`0x14C1D1`**.
* The `aux` add-on the completion uses is **`[0x157ABA]`** (aux seconds,
  class-2 accumulator) / **`[0x157ABC]`** (aux tick) — a runtime accumulator,
  not a config producer (zeroed at the match reset `FUN_00073EE0`, *cross-cited*
  FU-62 §4.6; engine `state.aux_seconds`/`aux_tick`).
* The **engine already stages the matching default**: half index 0 → 2 min →
  `[0x15881A] = 120`, `[0x15881C] = 40`, and begin installs
  `state.period_length = 120`, `state.extra_length = 40`
  (`fifa96_match_run.c:2368..2372`, §1.2). **Byte-exact to the native default.**
  A selector-0 live match therefore needs **no producer port**.
* The only unported input is the **front-end half-length menu**
  (`settings[0x0E]`, a FU-65 front-end option); its translator
  **`fifa96_settings_handoff` is already ported** (FU-68 §6), so even a future
  non-default length needs no new producer — only a wiring hook (§5.3).

This settles **FU-213-L5** ("the period limit / display-flag config writers are
unported ... the engine models the values"): the values are the match-init
constants, the staged default is exact, and the writers are the FU-68 hand-off
+ the match-start re-derive + the gate-only 60/30 reset override.

---

## 5. The engine extension point / producer plan

### 5.1 What must NOT be added

No period-length producer is to be added for the ordinary live match: the
native default (`[0x15881A]=120`, `[0x15881C]=40`) is already the engine's
staged `state.period_length`/`extra_length`. Injecting any other value as a
"producer" would fabricate a length the native default does not have.

### 5.2 What is already faithful

* The clock length select (`period < 2 ? period_length : extra_length`) — the
  native `[0x15881A]`/`[0x15881C]` select, `match_state.c:64` vs `0x8AFE2/Eb`.
* The completion `period_seconds == limit + aux_seconds` — the native case-0/1
  block, `match_state.c:78` vs `0x8B1E3..0x8B21D`.
* The class-2 aux accumulator — `match_state.c:66..73` vs `0x8AFFA..0x8B04A`.
* The 60/30 reset length — the engine `selector != 0` path (`RESET` constants)
  models `FUN_0004B508` (the `[0x1072F8] != 0` override).

### 5.3 The one residual wiring (Stage-2 candidate, not a producer)

`mr->config.period_length`/`extra_length` are **staged but not consumed**:
`begin` computes the installed clock lengths from the `selector` constants
(`:2368..2372`), not from `mr->config` (`:2252`). They agree today only because
both derive from the default half (2 min). If a non-default settings path ever
lands, the hook is to feed `mr->config.period_length`/`extra_length` into
`fifa96_match_run_set_period` instead of the constants — a wiring change, no new
native producer (the translator already exists, FU-68 §6). Minted as
**FU223-L3**.

---

## 6. Residual legs

1. **FU223-L1 — the `selector` ↔ `[0x1072F8]` mapping.** The engine maps
   `selector == 0` → 120/40 and `selector != 0` → 60/30 (`RESET`, the native
   `FUN_0004B508` override gated `[0x1072F8] != 0` at `0x4A264`). Whether the
   engine `selector` value corresponds to the native `[0x1072F8]` cell — and
   what sets `[0x1072F8]` (fresh `get_xrefs_to` = 5, written by `FUN_000493A0`
   `0x493AB`) — is **not derived here**. Owner: the match-setup/reset slice
   (FU-64 / FU-68).
2. **FU223-L2 — the front-end half-length menu producer.** `settings[0x0E]` →
   table `0x37170` → `[0x14C1D1]` (`FUN_0003749C` `0x375D1..0x375D4`) is a
   front-end option; `begin` always installs `fifa96_settings_defaults`
   (`settings.value[0x0E] = 0`), so a non-default half length never reaches
   `state.period_length`. The menu cell's live writer is unported. Owner: the
   front-end/settings slice (FU-65 / FU-49 §2).
3. **FU223-L3 — `mr->config` lengths are staged but unwired.** begin installs
   the clock lengths from the `selector` constants, not from
   `mr->config.period_length`/`extra_length` (`:2368..2372` vs `:2252`); they
   coincide only at the default half (2 min). Owner: the match-config wiring
   (this slice's Stage-2 plan).
4. **FU223-L4 — the type-4 competition arm.** `[0x14C32A] != 0` forces
   `[0x14C1D1] = 1` minute (`FUN_0003749C` `0x375A1`; fresh
   `get_xrefs_to 0x14C32A` = 60, writers `FUN_0001B7B8` `0x1B7C8/0x1B7D5` and
   `FUN_00032DE0` `0x32DFF`). The engine seeds `session_gate_14c32a = 0`
   (ordinary match) and never sets it, so the competition half length (60/20 s)
   is unmodeled. Owner: the competition/front-end slice (FU-65 / FU-213-L7).
5. **FU223-L5 — the other `[0x14C1D1]` readers.** `0x8F6CC` (no Ghidra function)
   and `FUN_0004B564` `0x4B5A2` (a getter) are not characterized; whether they
   consume the minutes value beyond the length derive is unknown. Owner: the
   match-config slice.
6. **FU223-L6 — the extra `[0x15881A]`/`[0x15881C]` readers are unported.**
   `FUN_0008EF38` (`0x8EF53`/`0x8EFC9`), `FUN_00092548` (`0x9260B`) and the raw
   `0x8F4EC`/`0x8FC90`/`0x8FF7E`/`0x8FFFA`/`0x908A1`/`0x908CD` sites (the
   per-second display/event blocks and the phase-0x13/0x14 end timelines) read
   the lengths; the engine models only the clock value, not those readers.
   Owner: the clock-signalled-events slice (FU-213-L6 / FU-213-L8).
7. **FU223-L7 — the match-start re-derive caller `0x74298`.** `FUN_000886D4`'s
   second caller `0x74298` lies in the period-end family body with **no Ghidra
   function**; the re-derive at a period boundary is disassembled-only. Owner:
   FU-196-L6 / FU-215-L3 (the `0x74244` family).

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `game/hdd/FIFA96/FIFA96.EXE`, `watcom:LE:32:default`, 2706 functions, 15239
  symbols, image base 0, min `0x10000`, max `.image::00174a2a`, size 2757479,
  `.image` overlay.
* `read_memory` — `0x37170` (32 B = `{2,4,6,8,10,20,45,0}`), `0x14C1D0` (8 B),
  `0x158818` (16 B), `0x105270` (64 B), `0x14C1D4` (8 B).
* `disassemble_bytes` — `0x375A9` (80 B), `0x886D4` (112 B), `0x8B1D0` (80 B),
  `0x4B508` (48 B), `0x4A250` (48 B), `0x8AFD8` (32 B), `0x8AFC0` (32 B),
  `0x4B0B8` (40 B).
* `get_xrefs_to` — `0x15881A` (**21**), `0x15881C` (**9**), `0x14C1D1`
  (**6**), `0x14C32A` (**60**), `0x14C302` (**3**), `0x1072F8` (**5**),
  `0x105270` (**2**), `0x14C1D4` (**21**).
* `get_function_xrefs` — `0x3749C` (**6** callers), `0x886D4` (**2** callers),
  `0x4B508` (**1** caller).

Engine/loader source read first-hand:
`include/fifa96_engine/fifa96_clock.h` (whole, 22 lines — the 100 Hz PIT driver,
**not** the match clock);
`src/fifa96_engine/fifa96_clock.c` (whole, 27 lines — tick_ns = 10 ms; no length);
`include/fifa96_engine/fifa96_match_run.h` (`:65..75` the four length
constants + the FU-62 §4.6/FU-68 §4.1 comment; `:579..587` `clock_period_ended`;
`:875..883` `mr->config`);
`src/fifa96_engine/fifa96_match_run.c` (`:1992..2060` init staging;
`:2130..2253` begin staging incl. the settings hand-off `:2249..2253`;
`:2258..2268` the range-word rebuild; `:2366..2372` `set_period`;
`:2425..2432` `fifa96_match_run_set_period`; `:2548..2614` the phase driver;
`:4014..4032` the frame body + `clock_period_ended` staging);
`include/fifa96_loader/fifa96_match_state.h` (whole, 31 lines);
`src/fifa96_loader/fifa96_match_state.c` (whole, 103 lines — the FU-62 clock,
`:61..89` the limit/aux/completion block);
`include/fifa96_loader/fifa96_settings.h` (`:20..38` `fifa96_match_config`);
`src/fifa96_loader/fifa96_settings.c` (`:14..30` defaults, `:69..70` the
half-length table, `:72..100` the hand-off). Docs cross-cited: **FU-62**
§4.3/§4.4/§4.6, **FU-68** §4/§5/§6/§8, **FU-148** §12, **FU-208**, **FU-213**
§3/§8 (FU213-L5/L6/L7/L8), **FU-215** §1.2/§2.2. No writes beyond this slice
file; no rename/label/comment/function/script/project save; `/FIFA96.EXE` and
the ISO untouched; no git commands.

---

## Landing note

FU-223 settles **FU-213-L5** and corrects the W21-1 brief's premise: a live
period length is a **match-init constant** (`[0x15881A]`/`[0x15881C]`, from
`[0x14C1D1]`), and the engine's staged default (`state.period_length = 120`,
`state.extra_length = 40`) is **byte-exact** to the native default (half index 0
= 2 min). **No producer is added**; the only residual is the front-end
half-length menu (FU223-L2) whose translator is already ported. The `0x105270`
option block and the range words `[0x14C1D4]`/`[0x14C1D6]` are **not** clock
inputs — they are the input-control model + the sit-8 chooser gate (Task 3).
Census unchanged; **no golden re-pin.** FU223-L1..L7 are the residual legs.
