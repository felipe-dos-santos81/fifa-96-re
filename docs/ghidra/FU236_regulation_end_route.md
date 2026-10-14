# FU-236 — the regulation-end (word `1`) route and its contrast with the ET end (phase-22 wave W22-6)

Provenance: M2 phase-22 Stage-1 recon, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE` /
path `/FIFA96.EXE`, `watcom:LE:32:default`, image base `0`, min `0x10000`, max
`.image::00174a2a`, **2706** functions, `memory_size 2757479`, `.image` overlay
present, `address_space_warning` overlay-qualified). Every address / byte /
instruction / xref-count below is a **fresh** `read_memory` /
`disassemble_bytes` / `get_function_by_address` / `decompile_function` /
`get_xrefs_to` of **this** session unless marked *cross-cited* (then the frozen
FU doc is named). The alias traps are applied (`dword[x]>>16` = the word at
`x+2`, `>>24` = the byte at `x+3`); LE globals sit at `+0x100000` from the file
offset; addresses are `/FIFA96.EXE` runtime addresses. Engine source read
first-hand. **No writes**: no rename, comment, label, function, script or
project save; `/FIFA96.EXE` untouched; the ISO / `game/hdd/` untouched; no git
commands.

Scope (W22-6 brief): derive the **regulation-end route** — the end word value
**`1`** written by the phase-`0xC` flag-clear terminal — and how it differs from
the **extra-time end** (word `2`). Steps: (1) the phase-`0xC` flag-clear
terminal `0x8BCBF CMP byte[0x157AC0],0 ; JZ 0x8BCD9 ; MOV word[0x158822],1`
(FU228 §2.2) + its `word[0x158818]` preconditions; (2) the consumer — the
`AX==1` + period-index `>= 2` → `mark_over` decision and the match-over screen;
(3) contrast the routes — the branch that distinguishes the regulation over
(word `1`) from the ET end (word `2`) and the cell that selects it; (4) verdict
+ numbered legs. This slice reads `FUN_0008BAF0`'s phase-`0xC` block (shared with
W22-1) and `FUN_0004B100`'s `AX==1` path (shared with W22-2) read-only;
cross-cites FU-64, FU-143, FU-207, FU-213, FU-228.

---

## Answer in one line

**The regulation-end route is the end word `[0x158822]=1`, written by
`FUN_0008BAF0`'s phase-`0xC` block only after the phase-`0xC` screen has held
past its display timeline (`word[0x158818] > 0x78`, deepened to `> 0xB4` or
`> 0x168` by the `FUN_00045001` predicate and cleared at the `[0x10F378]`
no-ET latch), and only when the extra-time flag `[0x157AC0]` is **clear**; the
consumer `FUN_0004B100` then reads the word back (`AX`) and, for `AX==1` **and
the period index `[0x157AC2] >= 2`**, enters the match-over screen
`FUN_00036BC0(2)` (`[0x105FFC]=2`, `mark_over`).** The ET end is the **same
machine, different block**: with `[0x157AC0]` set the chooser writes phase
`0x13`/`0x14`, `FUN_0008BAF0`'s ET block writes **`2`** (gated on the ET
timeline `word[0x158818] > 0x4B0` and the `[0x10F360]` ET-over latch; phase
`0x13` additionally requires the scores **equal**), and the consumer takes the
**`AX==2`** path (`0x4B257`, which clears the word and writes no
`[0x105FFC]=2`). **The cell that selects the route is the OL-85 extra-time flag
`[0x157AC0]`** (writer-side: it picks phase `0x0C` vs `0x13`/`0x14`; consumer-side:
the resulting word value `1` vs `2` selects the screen branch). The engine models
**only** the word-`1` route (`fifa96_match_run.c:4185`
`end_word_decided = (state.phase == 0x0C && global_157ac0 == 0)`); the word-`2`
ET-over event is deliberately excluded (FU228-L4), so landing the ET end is the
Stage-2 extension. No new action/phase row; the census does not move.

---

## 1. The phase-`0xC` flag-clear terminal and its `word[0x158818]` preconditions

### 1.1 The terminal (fresh bytes)

`FUN_0008BAF0` (`0x8BAF0..0x8C065`) dispatches on the **current phase** read at
`0x8BAFB EAX = [0x157A4A] >> 24` = the byte `[0x157A4D]`:

```
0x8BB0E CMP EAX,6   ; JNZ 0x8BB55      ; phase 6  block -> tail
0x8BB55 CMP EAX,2   ; JNZ 0x8BC01      ; phase 2  block -> tail
0x8BC01 CMP EAX,0xC ; JNZ 0x8BCE7      ; <<< phase 0x0C block (this slice)
0x8BCE7 CMP EAX,0x13 ; JZ 0x8BCF5
0x8BCEC CMP EAX,0x14 ; JNZ tail        ; <<< phase 0x13/0x14 block (the ET end)
```

Fresh `disassemble_bytes 0x8BC01` (229 B) — the phase-`0xC` block, and
`0x8BC62` (140 B) recovers the terminal:

```
0x8BC0A CMP dword[0x158808],0 ; JNZ 0x8BC20     ; an act installed? skip the timer
0x8BC13 MOV AX,[0x157A64] ; 0x8BC19 ADD word[0x158818],AX   ; end timer += frame delta
0x8BC20..0x8BC52  the 0x960 camera triple + CALL 0x36200 + CALL 0x8DCD4 sinks
0x8BC57 MOV EAX,[0x158816] ; SAR EAX,0x10 ; CMP EAX,0x78 ; JLE 0x8BDE7   ; gate 1
0x8BC68 CALL 0x45001 ; TEST AL,AL ; JNZ 0x8BC80
0x8BC71 MOV EAX,[0x158816] ; SAR EAX,0x10 ; CMP EAX,0xB4 ; JLE 0x8BCA3
0x8BC80 CMP dword[0x10F378],0 ; JZ 0x8BCA3      ; the no-ET latch
0x8BC89 CALL 0x37F54 ; 0x8BC92 [0x10F378]=0 ; 0x8BC97 word[0x158818]=0 ; JMP tail
0x8BCA3 CALL 0x45001 ; TEST AL,AL ; JNZ 0x8BCBF
0x8BCAC MOV EAX,[0x158816] ; SAR EAX,0x10 ; CMP EAX,0x168 ; JLE 0x8BDE7
0x8BCBF CMP byte[0x157AC0],0 ; JZ 0x8BCD9       ; <<< the EXTRA-TIME FLAG gate
0x8BCC8 MOV EAX,0x13 ; XOR EDX,EDX ; CALL 0x740A0  ; flag SET  -> phase 0x13
0x8BCD9 MOV word[0x158822],1                     ; <<< flag CLEAR -> MATCH OVER (1)
0x8BCE2 JMP 0x8BDE7
```

Fresh byte encodings: `0x8BCBF` = `80 3d c0 7a 15 00 00` (`CMP byte[0x157AC0],0`),
`0x8BCC6` = `74 11` (`JZ 0x8BCD9`), `0x8BCD9` = `66 c7 05 22 88 15 00 01 00`
(`MOV word[0x158822],1`), the `[0x158818]` reads `0x8BC57` = `a1 16 88 15 00`
(`MOV EAX,[0x158816]`) + `0x8BC5C` = `c1 f8 10` (`SAR EAX,0x10`) + `0x8BC5F`
= `83 f8 78` (`CMP EAX,0x78`). The fresh read sequence
`MOV EAX,[0x158816] ; SAR EAX,0x10` is the **byte-address trap**: the high word
of the dword at `0x158816` is `word[0x158818]` (AGENTS.md alias rule).

> **Fresh confirmation of FU228-L2 (erratum candidate against FU-213 §5).**
> FU-213 §5 (*quoted*) renders `0x8BC57 EAX = word[0x15881A]`; the fresh bytes
> are `MOV EAX,[0x158816] ; SAR EAX,0x10` = **`word[0x158818]`** — the end/act
> timer this same block accumulates at `0x8BC13`, not the period-limit cell
> `[0x15881A]`. FU228-L2's correction stands; recorded here as independently
> re-read.

### 1.2 The `word[0x158818]` preconditions (exact)

`word[0x158818]` is a **runtime BSS word** — fresh `read_memory 0x158818` (16 B) =
`00 × 16` (all-zero in the file image). The terminal (`0x8BCBF`) is reached iff
all of:

1. **`word[0x158818] > 0x78`** (`0x8BC57..0x8BC62`: `<= 0x78` → tail). The
   phase-`0xC` screen must survive its first display timeline.
2. **The `[0x10F378]` no-ET latch is clear.** Both paths that reach
   `0x8BC80` take `JZ 0x8BCA3` only when `[0x10F378]==0`; if the latch is set
   the block **resets** (`CALL 0x37F54`, clear `[0x10F378]`, `word[0x158818]=0`)
   and tails without a decision.
3. **`(FUN_00045001() != 0)` OR `word[0x158818] > 0x168`.** The predicate
   `FUN_00045001` (fresh `decompile_function 0x45001`:
   `return DAT_001067ec & 0xf0;`) is evaluated at `0x8BC68` and again at
   `0x8BCA3`. If it is **true** the terminal is reached directly (given (1)/(2));
   if it is **false** the timer must additionally exceed an intermediate `0xB4`
   (`0x8BC71..0x8BC7E`) and finally `0x168` (`0x8BCAC..0x8BCB9`).

So the regulation over is not decided the instant the period completes: the
phase-`0xC` **screen** holds (camera/sinks + the `word[0x158818]` timeline) until
the terminal, then the word-`1`/phase-`0x13` split is taken on the flag. (The
same `word[0x158818]` cell gates the ET halves at `0x4B0`/`0x258` and `0x4B0`/
`0x12C`, §3; fresh `get_xrefs_to 0x158818` = **83**, a widely shared cell.)

> **Adjacent-cell caution.** The FU-62 period-limit cell — the engine's
> `state.period_length` — is `[0x15881A]` (*cross-cited* FU-213 §1), a
> **different** address one dword on. The end timer is `word[0x158818]`.

---

## 2. The consumer `FUN_0004B100`: `AX==1` + index `>= 2` → `mark_over`

### 2.1 The branch (fresh bytes)

`FUN_0004B100` (`0x4B100..0x4B2E7`, fresh `decompile_function` / 139-insn
`disassemble_bytes`) is the per-frame match-state loop. Fresh bytes:

```
0x4B106 CMP word[0x158822],0 ; JZ 0x4B11A ; 0x4B110 EAX=1 ; JMP 0x4B2E1   ; head guard
...
0x4B1A6 CALL 0x8AF38                        ; AX = the end word (harness return)
0x4B1AB CMP AX,0x1 ; JC 0x4B2A9             ; AX == 0 -> ongoing-match path
0x4B1B5 JBE 0x4B1C6                         ; AX == 1 -> regulation-side path
0x4B1B7 CMP AX,0x2 ; JZ 0x4B257             ; AX == 2 -> ET-over path
0x4B1C1 JMP 0x4B2A9
0x4B1C6 XOR EAX,EAX ; MOV AL,[0x157AC2] ; CMP EAX,2 ; JGE 0x4B220   ; period index
0x4B1D2..0x4B21B (index < 2) the HALF-TIME block (0x513EC/0x53DC4/0x4B678)
0x4B220 JL 0x4B227 ; CMP EAX,4 ; JL 0x4B244
0x4B244 MOV EAX,0x2 ; CALL 0x36BC0          ; <<< [0x105FFC]=2 == mark_over
0x4B227 XOR EAX,EAX ; MOV AL,[0x157AC2] ; CMP EAX,4 ; JNZ 0x4B2DF
0x4B237 CMP byte[0x157AC0],0 ; JNZ 0x4B2DF  ; index==4 && flag set -> tail
0x4B244 (falls through) MOV EAX,0x2 ; CALL 0x36BC0
0x4B24E XOR EAX,EAX ; … RET                 ; word-1 path (word NOT cleared here)
0x4B297 XOR ESI,ESI ; MOV word[0x158822],SI ; the AX==2 arm clears the word
```

Fresh encodings: `0x4B106` = `66 83 3d 22 88 15 00 00`, `0x4B1A6` =
`e8 8d fd 03 00` (`CALL 0x8AF38`), `0x4B1AB` = `66 3d 01 00`
(`CMP AX,1`), `0x4B1B5` = `76 0f` (`JBE 0x4B1C6`), `0x4B1C6` = `31 c0` +
`a0 c2 7a 15 00` (`MOV AL,[0x157AC2]`), `0x4B244` = `b8 02 00 00 00`
(`MOV EAX,2`) + `0x4B249` = `e8 72 b9 fe ff` (`CALL 0x36BC0`), `0x4B29B` =
`66 89 35 22 88 15 00` (`MOV word[0x158822],SI`).

### 2.2 The `AX==1` regulation path (exact)

`AX` is the word returned by `FUN_0008AF38`'s tail
(`0x8B678 MOV AX,[0x158822]`; *cross-cited* FU-228 §2.3). For `AX==1`:

| period index `[0x157AC2]` | site | effect |
|---|---|---|
| `< 2` | `0x4B1D2` | the **half-time** block (`0x513EC` / `0x53DC4` / `0x4B678`) — **no** `mark_over` |
| `2` or `3` | `0x4B244` | **`mark_over`**: `FUN_00036BC0(2)` → `[0x105FFC]=2` |
| `== 4`, flag `[0x157AC0]==0` | `0x4B244` | **`mark_over`** (fallthrough) |
| `== 4`, flag `!= 0` | `0x4B2DF` | tail (return 0) — no screen |
| `> 4` | `0x4B2DF` | tail (return 0) |

So **the native match-over screen is entered only on the end word `1` *and* a
past-half-time period index** — the half-time completion (index `< 2`) takes the
`0x4B1D2` block, and a period-4 index with the flag set is skipped. This is the
`mark_over` site FU-64 §2.1/§6 maps to `fifa96_match_lifecycle_mark_over`
(*cross-cited*).

### 2.3 The match-over screen cell

Fresh `decompile_function 0x36BC0` (`0x36BC0..0x36BC5`):

```
void FUN_00036bc0(undefined4 param_1) { DAT_00105ffc = param_1; return; }
```

Fresh `get_xrefs_to 0x105FFC` = **7** (writers `0x36BC0`/`0x36BD8`/`0x36C0C`/
`0x36C3C`, readers `0x36BC8`/`0x36C0C`/`0x36C3C`). So `mark_over` is the
`[0x105FFC]=2` write (FU-64's `[0x5FFC]` naming; the runtime cell is
`0x105FFC`). The `AX==2` path (§3.3) writes **no** `[0x105FFC]`.

### 2.4 The head guard

`0x4B106` returns `EAX=1` **before running the frame body** while
`[0x158822] != 0`. This makes the word "sticky" — once set to `1`, subsequent
frames short-circuit at the guard until a reset writer clears it (the word-`1`
path does **not** clear it; only the `AX==2` path clears at `0x4B297` plus the
reset writers `0x4B0E7`/`0x8871B`/`0x74342`/`0x88D42`; *cross-cited* FU-228
§2.1). Which reset writer clears the word after the word-`1` screen is left is
not traced here (FU236-L4).

---

## 3. Contrast — the word-`1` (regulation) route vs the word-`2` (ET) route

### 3.1 The route (writer side): phase `0x0C` vs `0x13`/`0x14`

Both end values are written by `FUN_0008BAF0`, but by **different phase blocks**
selected by the current phase `[0x157A4D]` (`0x8BC01` phase `0x0C` vs `0x8BCE7`
phase `0x13`/`0x14`). The phase itself is written by the chooser
`FUN_0008B9CC` (*cross-cited* FU-213 §3 / FU-207 §4.2): flag `[0x157AC0]`
**clear** → phase `0x0C` + act `0xB`; flag **set** → phase `0x13`/`0x14` +
act `0xC`. So the **selecting cell is the OL-85 extra-time flag
`[0x157AC0]`**:

```
[0x157AC0] == 0 : chooser -> phase 0x0C, act 0xB -> FUN_0008BAF0 phase-0x0C block
                  -> (timeline holds) -> 0x8BCBF flag-clear -> word[0x158822] = 1
[0x157AC0] != 0 : chooser -> phase 0x13/0x14, act 0xC -> FUN_0008BAF0 ET block
                  -> (ET timeline holds) -> 0x8BD8A/0x8BDD4 -> word[0x158822] = 2
```

The phase-`0xC` block **re-reads** `[0x157AC0]` at `0x8BCBF`: clear → word `1`;
set → `CALL 0x740A0(0x13,0)` (phase `0x13`). The two reads are consistent (the
chooser already routed on the same flag), so the flag is the single route
selector.

### 3.2 The ET terminal (fresh bytes, `0x8BCE7` 256 B)

```
0x8BCF5 CMP dword[0x158808],0 ; JNZ 0x8BD0B ; 0x8BCFE word[0x158818] += [0x157A64]
0x8BD0B..0x8BD3B  the 0x780 camera hold + [0x158814]
0x8BD45 MOV EAX,[0x157A4A] ; SAR EAX,0x18 ; CMP EAX,0x14 ; JNZ 0x8BD95   ; phase 0x13 vs 0x14
0x8BD52 (phase 0x14) MOV EAX,[0x158816]; SAR 0x10; CMP EAX,0x4B0; JG 0x8BD81
0x8BD61 CALL 0x45001 ; … ; 0x8BD76 CMP EAX,0x258 ; JLE tail
0x8BD81 CMP dword[0x10F360],0 ; JNZ 0x8BDDD     ; the ET-over latch
0x8BD8A MOV word[0x158822],2                    ; phase 0x14 -> ET OVER (2)
0x8BD95 (phase 0x13) MOV AX,[0x157AC5] ; CMP AX,[0x157AC7] ; JNZ tail   ; scores differ -> HOLD
0x8BDAC CMP EAX,0x4B0 ; JG 0x8BDCB ; 0x8BDC4 CMP EAX,0x12C ; JLE tail
0x8BDCB CMP dword[0x10F360],0 ; JNZ 0x8BDDD
0x8BDD4 MOV word[0x158822],2                    ; phase 0x13 + EQUAL -> ET OVER (2)
0x8BDDD MOV dword[0x10F360],1                   ; latch
```

### 3.3 The contrast table

| axis | regulation over (word `1`) | ET over (word `2`) |
|---|---|---|
| writer block | phase `0x0C` (`0x8BC01`) | phase `0x13`/`0x14` (`0x8BCE7`) |
| writer site | `0x8BCD9` `MOV word[0x158822],1` | `0x8BD8A` (ph 0x14) / `0x8BDD4` (ph 0x13) |
| route selector | `[0x157AC0] == 0` at chooser + `0x8BCBF` | `[0x157AC0] != 0` at chooser |
| timeline cell | `word[0x158818]` > `0x78` (deeper `0xB4`/`0x168`) | `word[0x158818]` > `0x4B0` (drop `0x258`/`0x12C`) |
| latch | `[0x10F378]` **no-ET** (must be clear) | `[0x10F360]` **ET-over** (must be clear; set after) |
| score condition | none | phase `0x13` only: `[0x157AC5]==[0x157AC7]` (tied); differ → hold |
| consumer branch | `0x4B1B5 JBE 0x4B1C6` (`AX==1`) | `0x4B1B7 CMP AX,2; JZ 0x4B257` (`AX==2`) |
| consumer screen | `mark_over`: `FUN_00036BC0(2)` → `[0x105FFC]=2` (index `>= 2`) | the `0x4B257` path (`0x53DC4`/`0x4B678`/`0x1587D4`/`0x36200`) — **no** `[0x105FFC]=2` |
| word afterwards | **not** cleared on this path | cleared at `0x4B297` |

**The branch that distinguishes the two** is, on the writer side, the phase
dispatch `0x8BC01` (phase `0x0C`) vs `0x8BCE7` (phase `0x13`/`0x14`), **selected
by the current phase `[0x157A4D]`** — which the chooser sets from the
extra-time flag `[0x157AC0]`; and, on the consumer side, the
`0x4B1AB`–`0x4B1BB` dispatch (`JC`/`JBE`/`JZ`), **selected by the end word
`[0x158822]`**. The single upstream cell that decides regulation vs ET is
**`[0x157AC0]`**.

### 3.4 The `AX==2` ET-over screen (fresh, `0x4B257..0x4B2A8`)

```
0x4B257 CALL 0x53DC4                       ; one-shot (FUN_00064074)
0x4B25C CALL 0x4B678                       ; bool = 3 < [0x157AC2]
0x4B261..0x4B292  the [0x1587D4] screen body (0x36200 sink)
0x4B297 XOR ESI,ESI ; 0x4B29B MOV word[0x158822],SI(0)   ; clear the word
0x4B2A2..0x4B2A8  pops ; RET
```

`FUN_0004B678` (fresh `decompile_function`) is `return 3 < DAT_00157ac2;`. The
`AX==2` arm enters a **different** screen (the `[0x1587D4]`/`0x36200` block) and
**clears** the word; it does not write `[0x105FFC]=2`. The exact screen id is not
written by a `CALL 0x36BC0` on this path (FU236-L3).

---

## 4. The engine model and the divergence

The engine (`src/fifa96_engine/fifa96_match_run.c`) mirrors **only** the
word-`1` route. Fresh first-hand reads:

* `:4184` `int past_half_time = (mr->global_157ac2 >= 2u);`
  `:4185` `int end_word_decided = (mr->state.phase == 0x0Cu && mr->global_157ac0 == 0u);`
  `:4186` `if (period_ended && past_half_time && end_word_decided && …) mark_over();`
  — a **derived proxy for `[0x158822]==1`** from the live cells
  (`global_157ac2` = the roll `0x8B58A`; `global_157ac0` = the flag). It matches
  §2.2: index `>= 2` + flag clear + the phase-`0xC` screen.
* `:4177..4180` (comment): the proxy **intentionally EXCLUDES the `==2`
  extra-time-over event (`0x8BD8A`/`0x8BDD4`)** — i.e. the word-`2` route (this
  slice's ET end) has **no engine lifecycle path** (FU228-L4 leg).
* `:2575..2583` / `:2628..2633` / `:2672..2679`: the flag `mr->global_157ac0`
  is live (W2/W4/W5), threaded into the chooser; the phase-`0xC` machine
  `FUN_0008BAF0` and the `[0x158822]` cell are **not ported**
  (`:3226`/`:3889` comments; *cross-cited* FU-213 §10/§11).

So the engine's regulation-end landing today is a **condition proxy, not the
word**: `state.phase == 0x0C && global_157ac0 == 0 && global_157ac2 >= 2 &&
period_ended` ⇒ `mark_over`. The **ET end** (word `2`, the `0x4B257` screen) is
unmodelled. This is the concrete Stage-2 gap this slice names.

> **Caveat (cross-slice).** The existing contract tests
> `tests/test_engine_match_completion.c` pin the current proxy behaviour
> (*cross-cited* FU228-L4); any landing that adds the word-`2` route must
> re-derive them — a Stage-2 concern, not a recon write.

---

## 5. Census impact

**None.** No new action/phase row is wired here: the phase-`0xC`/`0x13`/`0x14`
rows and the chooser are already registered/reported (*cross-cited* FU-213 §6 /
FU-211); this slice only reads the end-word routes and contrasts them. The
README title (`floor(100 × 55/80) = 68`) and the ENGINE.md census line stand; no
golden re-pin.

---

## 6. Residual legs

1. **FU236-L1 — the phase-`0xC` timeline predicate `FUN_00045001`
   (`DAT_001067ec & 0xf0`) is not decomposed.** Fresh `decompile_function
   0x45001` = `return DAT_001067ec & 0xf0;`; it gates the terminal twice
   (`0x8BC68`, `0x8BCA3`) and chooses the `0xB4`/`0x168` threshold. Its cell
   `[0x1067EC]` producer/semantics are unlocated. Owner: the phase-`0xC`
   screen-timing slice (W22-5 / FU235).
2. **FU236-L2 — the `word[0x158818]` writers/consumers are not enumerated
   here.** Fresh `get_xrefs_to 0x158818` = **83** (a widely shared cell): the
   phase-`0xC` accumulate (`0x8BC13`) and reset (`0x8BC97`) are confirmed, but
   the other writers (`0x886FA`, `0x88922`, `0x89E7A`/W7, the many `0x89xxx`
   sites) and the timeline consumers are the FU-213/FU228-L2 surface. FU228-L2's
   `word[0x158818]` reading is independently re-confirmed at `0x8BC57` (§1.1).
3. **FU236-L3 — the `AX==2` ET-over screen target is not identified.** The
   `0x4B257` path (`0x53DC4` → one-shot `FUN_00064074`; `0x4B678`;
   `[0x1587D4]`; `0x36200`) writes no `CALL 0x36BC0`, so which screen the ET end
   enters is open. Owner: W22-2 / FU232.
4. **FU236-L4 — the head-guard "sticky" clear is untraced.** `0x4B106` returns
   `EAX=1` while `[0x158822] != 0`, and the word-`1` path does not clear the
   word (only `AX==2` at `0x4B297` + the reset writers). Which reset writer
   clears the word after the word-`1` screen is not traced here. Owner: W22-2 /
   FU232.
5. **FU236-L5 — the `0x8BDE7` machine tail is not decomposed.** Every terminal
   branch of `FUN_0008BAF0` funnels to the shared tail at `0x8BDE7` (the
   `0x15883C`/`0x158881` heading-damping + `0x6FE84` team refresh, *cross-cited*
   FU-213 §5); recorded, not mapped.
6. **FU236-L6 — the engine models the word-`1` route only, not the word-`2` ET
   end.** `fifa96_match_run.c:4185` `end_word_decided = (state.phase == 0x0C &&
   global_157ac0 == 0)` is the word-`1` proxy; the `==2` ET-over event
   (`0x8BD8A`/`0x8BDD4` → the `0x4B257` screen) has no engine path (explicitly
   excluded, `:4177..4180`). Landing it is the Stage-2 extension (extends
   FU228-L4 / FU213-L3).
7. **FU236-L7 — the flag semantics (why a *differing* score sets `[0x157AC0]`)
   are recorded, not asserted.** The route selectors above are structural; the
   football-level meaning is *cross-cited* FU-213-L7 / FU-207-L1 and not
   re-derived here. Do not over-claim the tie/aggregate rule.
8. **FU236-L8 — the no-ET latch reset `FUN_00037F54` is not decomposed.** Fresh
   `get_function_by_address 0x37F54` = `0x37F54..0x37F8F`; called at `0x8BC89`
   before clearing `[0x10F378]`/`word[0x158818]`. Owner: the phase-`0xC`
   screen-timing slice.

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, `image_base 0`, min `0x10000`, max
  `.image::00174a2a`, `.image` overlay.
* `get_function_by_address` — `0x8BAF0` (`FUN_0008baf0`,
  `0x8BAF0..0x8C065`), `0x4B100` (`FUN_0004b100`, `0x4B100..0x4B2E7`),
  `0x8AF38` (`FUN_0008af38`, `0x8AF38..0x8B687`), `0x45001`
  (`FUN_00045001`), `0x740A0`, `0x36BC0`, `0x4B678` (`FUN_0004b678`,
  `0x4B678..0x4B68A`), `0x37F54` (`0x37F54..0x37F8F`), `0x53DC4`
  (`0x53DC4..0x53DDF`).
* `decompile_function` — `0x45001` (`DAT_001067ec & 0xf0`), `0x740A0`,
  `0x36BC0` (`DAT_00105ffc = param_1`), `0x4B678` (`3 < [0x157AC2]`), `0x53DC4`,
  `0x8AF38`.
* `disassemble_bytes` — `0x8BAF0` (273), `0x8BC01` (229), `0x8BCE7` (256),
  `0x4B100` (487).
* `get_xrefs_to` — `0x158822` (**10**), `0x157AC0` (**19**), `0x157AC2`
  (**56**), `0x158818` (**83**), `0x105FFC` (**7**).
* `read_memory` — `0x158818` (16 B, all zero), `0x157AC0` (16 B, all zero).

Engine source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(`:2560..2583` the chooser comment + `extra_time` supersession; `:2584..2696`
`phase_drive` incl. the tie-break `:2628` and the roll `:2653`; `:4155..4192`
the frame tail + the `mark_over` proxy `:4184..4190`; `:4195..4229`
`resolve`/`step`; `:3226`/`:3889` the `[0x158822]`/`[0x105FFC]` comments).
Docs cross-cited: FU-64 §2.1/§5/§6 (the `[0x5FFC]` screens; the
`0x4B244..0x4B249` `[0x5FFC]=2` → `mark_over` mapping), FU-143, FU-207
(OL-85 flag census/readers), FU-213 (§1 cells, §2.5 roll, §3 chooser, §5 end
machine, §10/§11 errata), FU-211, FU-225, FU-228 (§2.1 writer census, §2.2
phase-`0xC` block, §2.3 consumer, §4.2 the proxy). No writes beyond this slice
file; no rename/label/comment/function/script/project save; `/FIFA96.EXE` and
the ISO untouched; no git commands.
