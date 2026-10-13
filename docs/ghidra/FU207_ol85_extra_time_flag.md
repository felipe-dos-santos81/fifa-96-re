# FU-207 — the OL-85 extra-time flag `[0x157AC0]` / `global_157ac2` (phase-19 wave R55)

Provenance: M2 phase-19 wave **R55**, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` —
`/FIFA96.EXE`, `watcom:LE:32:default`, 2706 functions, min `0x10000`, max
`.image::00174a2a`, `.image` overlay present). Every xref count, cell byte,
writer byte and instruction below is a fresh `get_xrefs_to` / `read_memory` /
`disassemble_bytes` of **this session** unless marked *cross-cited* (then the
frozen FU doc is named). The alias traps are applied (`dword[x]>>16` = the word
at `x+2`, `>>24` = the byte at `x+3`). The LE-image globals are quoted at their
runtime addresses; LE globals sit at `+0x100000` from the file offset. Engine
source read first-hand. **No writes**: no rename, comment, label, function,
script or project save; `/FIFA96.EXE` untouched; no git commands.

Scope (R55 brief): derive **`[0x157AC0]`** (the OL-85 extra-time flag) and the
companion cell **`[0x157AC2]`** (`global_157ac2`, the period index) byte-level;
find **every** native writer (fresh `get_xrefs_to` + `read_memory` +
`disassemble_bytes`), its exact semantics (byte vs bit vs word), its consumers,
and whether any **live native code path** (match setup / competition state /
period setup / lineup) can set it — i.e. is the extra-time state reachable in
the **ported** match flow (a live producer), or only via **match configuration**
(a leg). Record the situation selector that it feeds (situation 8) and the
period-index-4 / bit-8 gate. Give a clear reachability verdict. This slice
extends FU-143 §8/§10 (OL-85), FU-194 §3, FU-195 §3/§4, FU-196 §3/§4,
FU-202 §4/§5/§7 and FU-205 §4.

---

## 1. The cells — fresh byte-level map

Fresh `read_memory 0x157AB0` (32 B) and the companion reads below return
**all-zero** static bytes (the cells are runtime `.image` BSS, uninitialised in
the file image) — so the cells are provably runtime state, not constants:

```
0x157AB0..0x157ACF : 00 × 32
0x14C1D0..0x14C1DF : 00 × 16
0x14C2E8..0x14C2F7 : 00 × 16
0x10F378..0x10F37F : 00 × 8
```

| cell | width | role (fresh) | engine cell |
|---|---|---|---|
| **`0x157AC0`** | **byte** | the **extra-time flag** (values 0 / 1; every writer is a `MOV byte`) | **none** (`fifa96_match_run.h` carries no `global_157ac0`; only `157ac2`) |
| **`0x157AC2`** | **byte** | the **period index** (0..4; `=4` on the extra-time gate / front-end) | `struct fifa96_match_run.global_157ac2` (`fifa96_match_run.h:610`) |
| `0x157AC3` | byte | companion, reset-cleared with the flag (leg, §6/L4) | none |
| `0x157AC5` / `0x157AC7` | word | the score comparison pair (`[0x157AC5]` vs `[0x157AC7]`) | `score[]` (derived) |
| `0x157ABE` / `0x157ABF` | byte | the two side cells (chooser side select) | derived `side` |
| `0x157AB6` / `0x157ABA` | word | the per-second clock `sec` / `aux` | `match_state` clock |
| `0x5881A` / `0x5881C` | word | the period limits (`period<=1` / `period<=3`) | `period_seconds`/`extra` |
| `0x14C2EE` | **dword** | the **force-extra-time override** (the equal-score escape) | none |
| `0x14C1D0` | byte | the OL-85 else-arm config cell (`0x8B2CD`) | none |
| `0x14C1D4` / `0x14C1D6` | word | the per-side **input/range words** (the **bit-8** gate) | `input_range[2]` |
| `0x10F378` | dword | the "no-extra-time" latch set at `0x8B2DA` | none |
| `0x58822` | word | the match-over word | derived |

**Width verdict:** `[0x157AC0]` is a **boolean byte**, not a bit-within-a-word.
All seven writers write the standalone byte `0x157AC0` (bytes `c6 05 c0 7a 15 00
01`, `88 15 c0 7a 15 00`, `88 35 c0 7a 15 00`; §2). There is **no** bit-mask
(`AND/OR/TEST`) writer in the census — the "bit-8" semantics live on a
*different* cell, the input/range word pair (`TEST AH,0x1`, §4.1).

---

## 2. The `[0x157AC0]` xref census — 7 writers + 12 readers (fresh)

Fresh `get_xrefs_to 0x157AC0` = **19** (byte-identical site set to FU-202 §4):

| # | writer | fn | fresh bytes | effect |
|---|---|---|---|---|
| W1 | `0x73F58` | `FUN_00073ee0` | `88 15 c0 7a 15 00` = `MOV byte[0x157AC0],DL` (DL=0, `0x73F50 XOR DL,DL`) | **reset to 0** |
| W2 | `0x8B2AC` | `FUN_0008af38` | `c6 05 c0 7a 15 00 01` = `MOV byte[0x157AC0],1` | period-1 completion, scores differ |
| W3 | `0x8B2C1` | `FUN_0008af38` | `c6 05 c0 7a 15 00 01` = `MOV byte[0x157AC0],1` | period-1 completion, scores equal + `[0x14C2EE]!=0` |
| W4 | `0x8B3A2` | `FUN_0008af38` | `c6 05 c0 7a 15 00 01` = `MOV byte[0x157AC0],1` | the `[0x5881C]`-limit completion, scores differ |
| W5 | `0x8B5CD` | `FUN_0008af38` | `88 15 c0 7a 15 00` = `MOV byte[0x157AC0],DL` (DL=1, `0x8B5B9 MOV DL,1`) | the period-1 rollover writer |
| W6 | `0x389A4` | `FUN_00038630` | `88 15 c0 7a 15 00` = `MOV byte[0x157AC0],DL` (DL=1) | **front-end / competition** arm (also `[0x14C2EE]=1`) |
| W7 | `0x89E8C` | (act-7 phase-`0x1D` arm-9 terminal) | `88 35 c0 7a 15 00` = `MOV byte[0x157AC0],DH` (DH=1, `0x89E78 MOV DH,1`) | the extra-time act terminal side-effect |

All seven fresh `read_memory` re-reads byte-for-byte confirm the encodings above
(e.g. `0x8B2AC` = `c6 05 c0 7a 15 00 01`; `0x389A4` = `88 15 c0 7a 15 00`).
There is **no** W-type with a bit/word store.

**The 12 readers (fresh):** `0x4B237` (`FUN_0004b100` period-screen gate),
`0x4B37A` (`FUN_0004b378` — the getter `AL=[0x157AC0]; RET`), `0x8B5B0` /
`0x8B60D` (`FUN_0008af38` rollover guard / period-4 skip), `0x8B9CF` /
`0x8BAC6` (`FUN_0008b9cc` chooser + act 0xB/0xC select), `0x8BCBF`
(`FUN_0008baf0` phase-`0x0C` end machine), `0x28598` (`FUN_00028588`
competition screen), `0x2AA71` (`FUN_00029f68`), `0x31C05` (`FUN_00031aa4`),
`0x88C4B` / `0x88CAB` (inside the `0x889xx` restart/announce block).

---

## 3. The writers — exact semantics (fresh disassembly)

### 3.1 The reset writer W1 (`0x73F58`, `FUN_00073ee0`)

Fresh `disassemble_bytes 0x73F40` (48 B):

```
0x73F50 XOR DL,DL ; 0x73F52 XOR BL,BL ; 0x73F54 XOR AL,AL ; 0x73F56 XOR CL,CL
0x73F58 MOV byte[0x157AC0],DL      ; flag := 0
0x73F5E MOV byte[0x157AC3],BL      ; the companion := 0
0x73F64 MOV [0x157AC7],AX          ; the away-score word := 0
0x73F6A MOV [0x157AD4],AX          ; companion word := 0
```

`FUN_00073ee0`'s callers (fresh `get_xrefs_to 0x73EE0` = 3): `0x7407f`
(`FUN_00074034`), `0x4B025` (`FUN_0004B020`), `0x7428e` — the restart/setup
sinks. So the flag is **cleared with the score pair and the line-up/restart
state**, confirming the flag is a per-match runtime cell.

### 3.2 The clock-arm writers W2–W5 (`FUN_0008af38`)

`FUN_0008af38` is the **per-frame match clock body** (fresh `get_xrefs_to
0x8AF38` = 2: `0x4B1A6` in `FUN_0004b100` — the per-frame match loop — and
`0x7440b`). Fresh `disassemble_bytes 0x8B220` (208 B):

```
0x8B22F MOV AX,[0x157AB6] ; CMP AX,[0x5881A] ; JNZ 0x8B274   ; sec == period-1 limit
0x8B241 CMP word[0x157ABA],0 ; JZ 0x8B295                    ; aux == 0
0x8B295 MOV AX,[0x157AC5] ; MOV DX,[0x157AC7] ; CMP AX,DX
0x8B2AA JZ 0x8B2B8
0x8B2AC MOV byte[0x157AC0],1 ; JMP 0x8AFB7                   ; W2: scores differ -> extra time
0x8B2B8 CMP dword[0x14C2EE],0 ; JZ 0x8B2CD
0x8B2C1 MOV byte[0x157AC0],1 ; JMP 0x8AFB7                   ; W3: scores equal + override
0x8B2CD CMP byte[0x14C1D0],0 ; JNZ 0x8AFB7
0x8B2DA MOV dword[0x10F378],1 ; JMP 0x8AFB7                  ; the no-extra-time latch
```

Fresh `disassemble_bytes 0x8B360` (56 B): the `[0x5881C]`-limit completion arm
`0x8B364 XOR EDX,EDX; … MOV DX,[0x5881C]; MOV AX,[0x157ABA]; ADD EDX,EAX; … MOV
AX,[0x157AB6]; CMP EAX,EDX; JNZ 0x8AFB7` (the equality `sec == limit + aux`),
then `0x8B387 MOV AX,[0x157AC5]; MOV DX,[0x157AC7]; MOV ECX,1; CMP AX,DX` →
`0x8B3A2 MOV byte[0x157AC0],1` (**W4**).

Fresh `disassemble_bytes 0x8B5A0` (120 B) — the **period-1 rollover** writer:

```
0x8B5A1 MOV AX,[0x157AC5] ; CMP AX,[0x157AC7] ; JZ 0x8B601   ; scores differ?
0x8B5B0 CMP byte[0x157AC0],0 ; JNZ 0x8B601                    ; flag already set?
0x8B5B9 MOV DL,1 ; 0x8B5BB MOV EBX,0x20
0x8B5CD MOV byte[0x157AC0],DL                                 ; W5: flag := 1
0x8B5D3 CMP AX,CX ; … ; 0x8B5FA CALL 0x8B9CC                  ; then the chooser
```

So the clock arms set the flag at a **period completion with differing scores**
(W2 period-1, W4 the `[0x5881C]` limit) or via the `[0x14C2EE]` override (W3),
and the rollover re-arms it (W5). `0x8B60D` (same function, fresh: `AL=[0x157AC2];
CMP EAX,4; JNZ …; CMP byte[0x157AC0],0; JZ …` then `CALL 0x8B9CC`) is the
**period-index-4 + flag** read (the period-4 skip).

> **Erratum (quoted).** FU-195 §4 cited this rollover writer as
> `0x8B5D3 MOV [0x157AC0],DL(1)`; the fresh `MOV` is at **`0x8B5CD`** (bytes
> `88 15 c0 7a 15 00`; `0x8B5D3` is the following `CMP AX,CX`). FU-202 §4 already
> recorded this; restated here so the reader of FU-207 does not re-import the
> stale address.

### 3.3 The front-end / competition writer W6 (`0x389A4`, `FUN_00038630`)

Fresh `disassemble_bytes 0x38940` (80 B) + `0x38980` (64 B):

```
0x38954 CALL 0x3AB34 ; TEST EAX,EAX ; JL 0x39021
0x38961 MOV EAX,[0x14AFC4] ; MOV EDX,[0x14AFC8]
0x3896C MOV EAX,[EAX] ; ADD EAX,[EDX]        ; the aggregate of two pointers' first dwords
0x38970 CMP EAX,1 ; JC 0x3897E ; JBE 0x38989 ; CMP EAX,2 ; JZ 0x38998 ; JMP 0x389B0
0x38989 MOV DH,4 ; 0x3898B MOV EAX,0xFFFFFFFE
0x38990 MOV byte[0x157AC2],DH                ; period index := 4
0x38998 MOV EAX,0xFFFFFFFE ; MOV DL,1 ; MOV ECX,1
0x389A4 MOV byte[0x157AC0],DL                ; W6: flag := 1
0x389AA MOV dword[0x14C2EE],ECX              ; AND the force-extra-time override := 1
```

So the **front-end / competition** path (the aggregate in
`[0x14AFC4]`/`[0x14AFC8]`, computed via `FUN_0003ab34`) sets the period index
`[0x157AC2]=4` (aggregate ==1) or the flag `[0x157AC0]=1` **and** the override
`[0x14C2EE]=1` (aggregate ==2). `FUN_00038630` is reached from `FUN_00039054`
(fresh `get_xrefs_to 0x39054` = 4: `0x4949a` in `FUN_000493a0` — **the
match-init** — and three sites in `FUN_00049b28`), and `FUN_000493a0`'s own
callers are the front-end match-screen arms (`0x180AA`/`0x18359`/`0x18819`,
*cross-cited* FU-196 §3.1 / FU-202 §1.1). **This is a live pre-match producer
reachable from the competition/match-setup state** — the "match configuration"
route.

### 3.4 The act-7 terminal writer W7 (`0x89E8C`)

Fresh `disassemble_bytes 0x89E70` (48 B):

```
0x89E73 TEST AX,AX ; JNZ 0x89EA6
0x89E78 MOV DH,1 ; 0x89E7A MOV [0x158818],AX(0)
0x89E84 MOV byte[0x158828],BL(0)
0x89E8C MOV byte[0x157AC0],DH(1)             ; W7: flag := 1
0x89E92 MOV dword[0x158808],EBX(0)
0x89E9A MOV byte[0x157A4C],CL(0)
```

This is the **phase-`0x1D` (act 7) arm-9 terminal** *cross-cited* FU-194 §3.3 /
FU-202 §4: it raises the flag as a **side-effect of the extra-time act finishing**
— i.e. it is **downstream** of the flag, not an independent producer (it can only
fire once the extra-time route is already running).

---

## 4. The consumers (fresh)

### 4.1 The situation-8 selector + the period-index-4 / bit-8 gate

**Situation 8** is produced by two native sites, and both are gated on the
**period index `[0x157AC2]` or the bit-8 of the input/range words** — *not*
directly on `[0x157AC0]`:

* `0x4B0CB` (`FUN_0004b02c`, fresh `disassemble_bytes 0x4B0C0` 80 B):
  `MOV CX,word[0x14C1D4]; MOV AX,[0x14C1D6]; OR EAX,ECX; TEST AH,0x1` — **bit 8**
  of the OR of the two per-side range words (`0x14C1D4`/`0x14C1D6`); taken →
  `MOV EAX,8; … CALL 0x8A938` (**situation 8**) and `0x4B0DE MOV byte[0x157AC2],4`
  (the period index := 4).
* `0x888C8` (`FUN_00088860`, fresh `disassemble_bytes 0x88890` 72 B):
  `MOV AL,[0x157AC2]; CMP EAX,4; JNZ 0x888E2` — when `[0x157AC2]==4`:
  `CALL 0x92AC8` (RNG) `; AND AL,1; MOV EAX,8; CALL 0x8A938` (**situation 8**,
  RNG side). This is the phase-`0x0C` period-end producer.

The loader situation table (`fifa96_action_phase_situations[8]`,
`src/fifa96_loader/fifa96_action_handlers.c:1142`) is
`{phase NONE, act 7, stage 0, flags OPEN_LEG}` — **situation 8 → act 7 → phase
`0x1D`**. So the flag's *own* role is the period-end **act 0xB vs 0xC** choice
and the period-4 skip; the period index `[0x157AC2]==4` (set by the same gate)
is what selects situation 8.

### 4.2 The flag's direct readers (fresh)

* `0x8B9CF` (`FUN_0008b9cc`, the period-change chooser): `CMP byte[0x157AC0],0;
  JZ 0x8BAA7` — set → the extra-time branch (`EBX=0x13`, score/period logic →
  phase 0x13/0x14); clear → the `0x8BAA7` path (act 0xB; phase `0x0C` if
  period<4).
* `0x8BAC6` (`FUN_0008b9cc` tail): `CMP byte[0x157AC0],0; JZ 0x8BADB` — set →
  `EAX=0xC` (act 0xC); clear → `EAX=0xB` (act 0xB), then `0x8BAE7 CALL 0x888FC`.
* `0x8BCBF` (`FUN_0008baf0`, the phase-`0x0C` end machine): `CMP
  byte[0x157AC0],0; JZ 0x8BCD9` — set → `CALL 0x740A0(0x13,0)` (phase 0x13);
  clear → `MOV word[0x58822],1` (match over).
* `0x4B237` (`FUN_0004b100`): the period-screen gate — `[0x157AC2]==4 && flag==0`
  → show the screen (`0x4B244 EAX=2; CALL 0x36bc0`); `flag!=0` → skip.
* `0x4B37A` (`FUN_0004b378`): the accessor `XOR EAX,EAX; MOV AL,[0x157AC0]; RET`.
* `0x8B5B0` / `0x8B60D` (`FUN_0008af38`): the rollover guard and the period-4
  skip (§3.2).
* `0x28598` (`FUN_00028588`): `MOV AL,[0x157AC0]; CMP EAX,1; JNZ …` — a
  **competition/standings screen** consumer; `0x2AA71` (`FUN_00029f68`):
  `flag != 1 → MOV dword[0x149FE0],0`; `0x31C05` (`FUN_00031aa4`): `flag == 1`
  → a standings computation.
* `0x88C4B` / `0x88CAB` (the `0x889xx` restart block): `0x88CAB CMP
  byte[0x157AC0],0; JZ 0x88CC4; CMP dword[0x14C2EE],0; JNZ 0x88CC4; MOV
  EAX,0x14`.

---

## 5. The coupled act-7 / phase-`0x1D` route (fresh)

Fresh `read_memory 0x110794+0x1D*4` (`0x110808`) = `68 98 08 00` →
**phase `0x1D` = `0x89868`** (fresh `get_xrefs_to 0x89868` = **1**, DATA from
`0x110808`). Fresh `read_memory 0x1106E0+0x20*4` (`0x110760`) = `ec 4e 08 00` →
**action row `0x20` = `0x84EEC`**. Fresh `disassemble_bytes 0x89990` (64 B)
confirms the coupled installer:
`0x899A2 MOV EDX,0x20; 0x899AB CALL 0x7D9A4` (install action row `0x20` on the
actor record `[[ESP+0x8]]`) and `0x899C4 MOV EDX,0x1F; 0x899CD CALL 0x7D9A4`
(the ball-record row `0x1F` install). The row/phase bodies are `NULL` in the
engine (`fifa96_match_handlers.c:5114` phase `0x1D`, `:5027` action `0x20`).

The armed route: bit-8 / `[0x157AC2]==4` → **situation 8** → act 7 → phase
`0x1D` (`0x89868`) → the arm-9 terminal (`0x89E8C`) **raises `[0x157AC0]`**.
So the phase-`0x1D` row and the flag are **coupled**: landing the row would also
land a flag producer (in the terminal), and vice versa.

---

## 6. Reachability verdict

**Three classes of native producer** (fresh):

1. **Match configuration / competition** — `0x389A4` (`FUN_00038630` ←
   `FUN_00039054` ← `FUN_000493a0` match-init) driven by the competition
   aggregate (`FUN_0003ab34` + `[0x14AFC4]`/`[0x14AFC8]`). Live pre-match; the
   "match configuration" route.
2. **In-match clock harness** — W2/W3/W4/W5 in `FUN_0008af38`, live every frame
   but fire only on a **period completion** (scores differ, or the `[0x14C2EE]`
   override). The M2 fixture never reaches a period completion (it ends at the
   goal), so all four are tape-dormant.
3. **The act-7 terminal** — W7 (`0x89E8C`), downstream of the flag (not
   independent).

**Is the extra-time state reachable in the ported match flow?** **No.**

* The engine has **no `[0x157AC0]` cell** (fresh `grep 'global_157ac0\|157ac0'
  src include` = empty; only `157ac2` exists, `fifa96_match_run.h:610`).
  `fifa96_match_run_phase_drive` calls `fifa96_action_phase_period_end(...,
  extra_time = 0u, ...)` **hardcoded** (`fifa96_match_run.c:2583`), so
  `out.act` is always `0x0B` and phase `0x22`/act 0xC is producer-dormant
  (re-confirms FU-195 §4/§7, FU-202 §4).
* `global_157ac2` (the period index) and `input_range` (the bit-8 gate) are
  **seeded 0** in `reset`/`begin` (`fifa96_match_run.c:2034`, `:2152`;
  `fifa96_match_run.h:560-565`), so neither **situation-8** producer fires.
* `FUN_00038630` is **not ported**: `begin` runs the *derived*
  `fifa96_match_run_screen_install(mr,0,0,0)` (`:2387`, the goal-screen
  installer `FUN_00092D8C`, **not** `FUN_00038630`) and the re-placed
  `fifa96_match_run_period_setup(mr,1)` (`:2408`) which never touches the flag;
  the `FUN_00038630` flag/`[0x14C2EE]` arm and the `FUN_0008af38` period-1
  completion side-effect are unported.

**Verdict (one line):** *`[0x157AC0]` / `global_157ac2` is a boolean byte whose
native ON-state is produced by the front-end/competition match-init and by a
live in-match period completion, but the port carries no cell, no writer and no
consumer — the extra-time state is reachable only via match configuration (a
leg, FU202-L1), never through any live ported path.*

---

## 7. Numbered open legs

1. **FU207-L1 — the flag has no engine cell (extends FU202-L1).** Fresh
   `grep` finds no `157ac0` in `src`/`include`; `phase_drive` hardwires
   `extra_time = 0`. The whole OL-85 arm chain (W2–W7 + the `probe[4]` upgrade
   inputs) is unported. Do not fake a producer.
2. **FU207-L2 — the front-end/competition producer `0x389A4`.** Fresh: it is
   reached via `FUN_00038630 ← FUN_00039054 ← FUN_000493a0` (the match-init) and
   driven by `FUN_0003ab34` over `[0x14AFC4]`/`[0x14AFC8]`; it also sets
   `[0x14C2EE]=1`. This narrows FU202-L7 to the `FUN_000493a0` match-init route;
   the `FUN_0003ab34` aggregate semantics (0 = none, 1 = period-index 4,
   2 = flag+override) are not decomposed here.
3. **FU207-L3 — the clock-harness arm map (extends FU202-L6).** W2 is the
   period-1 completion, W4 the `[0x5881C]`-limit completion, W5 the period-1
   rollover; the intermediate block `0x8B274`/`0x8B2E9` (`[0x5881C]`
   `sec`-vs-`limit+aux`) is not fully arm-mapped. The `[0x5881A]`/`[0x5881C]`
   limit identities (FU-143 §3.5) are cross-cited.
4. **FU207-L4 — the companion cells.** `[0x157AC3]` (reset at `0x73F5E`, read at
   `0x74181`/`0x288fc`) and the reset-cleared `[0x157AD4]`, plus the config cell
   `[0x14C1D0]` (the OL-85 else-arm), are recorded but their semantics are not
   decomposed.
5. **FU207-L5 — the act-7 terminal coupling.** W7 (`0x89E8C`) raises the flag
   *inside* the phase-`0x1D` row: the coupled installer `0x899A2` and the flag
   producer land together (extends FU-194 §3.3 / FU-202 §5).
6. **FU207-L6 — the `[0x157AC2]` period-index writers (extends FU202-L4).**
   Fresh `get_xrefs_to 0x157AC2` = **56**; the writers are `0x73F82` (reset),
   `0x4B0DE` (`FUN_0004b02c`, :=4 on the bit-8 gate), `0x4BA20`/`0x4BA31`
   (`FUN_0004ba00`), `0x8B58A`/`0x8B61B` (`FUN_0008af38` rollover INCs),
   `0x38990` (`FUN_00038630`, :=4) and `0x88D6A`. The engine mirrors only the
   rollover increments; the front-end/setup writers are unported.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, 2706
  functions, `.image` overlay.
* `get_xrefs_to` — `0x157AC0` (**19**), `0x157AC2` (**56**), `0x157AC1` (5),
  `0x157AC3` (4), `0x14C2EE` (5), `0x157ABE` (9), `0x157ABF` (9), `0x14C1D0`
  (19), `0x73EE0` (3), `0x38630` (1), `0x39054` (4), `0x8AF38` (2), `0x8B9CC`
  (3), `0x89868` (1), `0x14AFC4` (39).
* `read_memory` — `0x157AB0` (32), `0x14C1D0` (16), `0x14C2E8` (16),
  `0x10F378` (8), `0x157AC5` (4), `0x8B2AC` (7), `0x8B2C1` (7), `0x8B3A2` (7),
  `0x8B5CD` (6), `0x389A4` (6), `0x89E8C` (6), `0x73F58` (6), `0x110760` (4),
  `0x110808` (4), `0x1107EC` (52).
* `disassemble_bytes` (start, length in B): `0x73F40` (48), `0x8B220` (208),
  `0x8B388` (48), `0x8B5A0` (120), `0x8B360` (56), `0x8B2F0` (112), `0x8B9C0`
  (64), `0x8BAC0` (64), `0x8BCB0` (64), `0x4B0C0` (80), `0x4B220` (64),
  `0x4B370` (48), `0x38940` (80), `0x38980` (64), `0x89E70` (48), `0x89990`
  (64), `0x88890` (72), `0x88C40` (48), `0x88C98` (40), `0x28588` (48),
  `0x29F60` (48), `0x31AA0` (48), `0x39054` (96), `0x49480` (48), `0x3AB34`
  (48).
* `get_function_by_address` — `0x38630` (`FUN_00038630`),
  `0x39054` (`FUN_00039054`), `0x3AB34` (`FUN_0003ab34`),
  `0x8AF38` (`FUN_0008af38`). `0x88C4B` and `0x89868` are inside
  non-auto-created bodies (explicit).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(`:2034`/`:2152` the reset/begin BSS seeds; `:2382..2413` the begin
screen-install + the re-placed period-setup `:2408`; `:2564..2602` `phase_drive`
incl. the `extra_time = 0u` call `:2583`; `:3063..3099` `period_setup` incl. the
bit-8 gate `:3093`; `:3113..3120` the goal-scan period-4 note; `:5998..6044`
`action_phase_1D`; `:6148..6183` `action_phase_22`); `include/fifa96_engine/
fifa96_match_run.h` (`:552..569` the input/range note + `global_157ac2:610`;
`:1675..1700` the screen installer); `src/fifa96_loader/
fifa96_action_handlers.c` (`:1129..1147` the situation table; `:1163..1210`
`fifa96_action_phase_period_end`).

Docs cross-cited: FU-143 §3.3/§3.4/§3.5/§4.2/§8/§10 (OL-85), FU-194 §3.3,
FU-195 §3/§4/§7, FU-196 §1/§3, FU-202 §1/§4/§5/§7, FU-205 §4, FU-206 §4/§5.
No writes beyond this slice file; no rename/label/comment/function/script/
project save; `/FIFA96.EXE` untouched; no git commands.
