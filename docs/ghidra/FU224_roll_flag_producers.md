# FU-224 — the roll + extra-time-flag cells and their front-end producers (phase-21 wave W21-2)

Provenance: M2 phase-21 wave **W21-2**, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE` /
path `/FIFA96.EXE`, `watcom:LE:32:default`, image base `0`, min `0x10000`, max
`.image::00174a2a`, **2706** functions, **15239** symbols, memory size 2757479,
`.image` overlay present). Every address/byte/instruction/xref-count below is a
**fresh** `read_memory` / `disassemble_bytes` / `disassemble_function` /
`decompile_function` / `search_instructions` / `get_xrefs_to` /
`get_function_xrefs` / `get_function_by_address` of **this** session unless
marked *cross-cited* (then the frozen FU doc is named). The alias traps are
applied (`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`); LE
globals sit at `+0x100000` from the file offset; addresses are `/FIFA96.EXE`
runtime addresses. Engine/loader source read first-hand. **No writes**: no
rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; the ISO / `game/hdd/` untouched; no git commands.

Scope (W21-2 brief): pin the two **live** cells the phase-21 period-end spine
seeds — (1) the period-index roll `mr->global_157ac2` ≙ `[0x157AC2]` (FU-213)
and (2) the OL-85 extra-time flag `mr->global_157ac0` ≙ `[0x157AC0]`
(FU-213/FU-207) — first-hand (identity, width, adjacency, writers); derive their
**front-end match-init producers** (`FUN_00038630` `0x38990` / `0x389A4`); trace
the W2/W4/W5 tie-break setters and the roll semantics (when the index reaches 4);
and rule on reachability (ported front-end vs. the match-config-default
stand-in). Extends / consolidates **FU-207**, **FU-213**, **FU-202**, **FU-211**,
**FU-215**, **FU-216**.

**Answer in one line.** Both cells are **1-byte BSS bytes** (zero at file-load)
inside the clock struct `[0x157AC0..]` — `[0x157AC0]` = OL-85 flag, `[0x157AC1]`
= the seconds accumulator, `[0x157AC2]` = the period index — and their only
**front-end** producers are the two arms of `FUN_00038630` **case `0x13`
(`DAT_0014B018 == 0x13`)** at `0x38990` (`[0x157AC2] := 4`, aggregate `== 1`) and
`0x389A4` (`[0x157AC0] := 1` **and** `[0x14C2EE] := 1`, aggregate `== 2`), read
from `*[0x14AFC4] + *[0x14AFC8]`. That case is reached **only** through
`FUN_00038630 ← FUN_00039054` — which gates the call on the **screen id
`[0x105FFC] ∈ {5,7}`** (a menu/UI state, *not* a raw keyboard/mouse scan) — `←
FUN_000493A0` (match-init). The competition-screen route is keyboard/action-code
gated (FU-65: *runtime-blocked*), and the ported front-end
(`fifa96_frontend.c` / `fifa96_frontend_run.c`) models neither `[0x105FFC]` nor
`DAT_0014B018`; so **the FE producers are unreachable from the ported front-end
and must stay unported legs** (`FU215-L2` / `FU207-L2`). Extra time enters a live
engine match **only** via the in-match clock roll (native `FUN_0008AF38`
`0x8B58A`), which the engine already mirrors (T1/FU-213); the cells' faithful
engine expression is the **staged-zero default + the in-match roll** — no new
producer is added and nothing is faked.

---

## 1. The two live cells (fresh)

### 1.1 Identity, widths, adjacency

Fresh `read_memory 0x157AC0` (8 B) = `0000000000000000` — both cells are **zero at
file-load** (runtime-built; §3). Fresh `read_memory 0x157AB8` (16 B) is likewise
all zero.

Widths, from the encoding of the writes themselves (fresh
`disassemble_bytes 0x38988`):

```
0x38990 MOV byte ptr [0x00157ac2], DH    ; bytes 88 35 c2 7a 15 00   (1 byte)
0x389a4 MOV byte ptr [0x00157ac0], DL    ; bytes 88 15 c0 7a 15 00   (1 byte)
```

so both are **`uint8_t`** (the engine's `mr->global_157ac2` /
`mr->global_157ac0` are `uint8_t`, `fifa96_match_run.h:610`/`:622`).

The two cells are **not adjacent** — the clock struct interleaves them (fresh
`disassemble_function 0x8AF38` and `0x73EE0`):

| off | cell | role | fresh witness |
|---|---|---|---|
| `0x157AC0` | OL-85 extra-time flag (byte) | | `0x389A4`, `0x8B2AC`, `0x8B3A2`, `0x8B5CD` |
| `0x157AC1` | seconds accumulator (byte) | | `0x8AFAC ADD byte[0x157AC1],DL`; reset `0x73F01` |
| `0x157AC2` | period index (byte) | | `0x8B58A`, `0x8B61B`, `0x38990`, `0x73F82` |
| `0x157AC3` | byte (companion, FU207-L4) | | reset `0x73F5E` |
| `0x157AC5` / `0x157AC7` | score words (side 0 / side 1) | | `0x8B295`/`0x8B39F` (the differ test) |

Reference counts (fresh `get_xrefs_to`): **`[0x157AC2]` = 56**, **`[0x157AC0]` =
19**; a program-wide `search_instructions` on the operand `157ac2` returns
**56** matches and on `105ffc` returns **7** (the gate cell, §2.2).

### 1.2 The complete writer set (fresh `search_instructions`, 235 026 instructions scanned)

**`[0x157AC2]` — period index — 8 writers:**

| # | site | function | instruction | bytes | role |
|---|---|---|---|---|---|
| R1 | `0x8B58A` | `FUN_0008AF38` | `MOV byte[0x157AC2],BH` (BH=[·]+1) | `88 3d c2 7a 15 00` | **the roll** (§3.2) |
| R2 | `0x8B61B` | `FUN_0008AF38` | `INC byte[0x157AC2]` | `fe 05 c2 7a 15 00` | **period-4 skip → 5** (§3.2) |
| R3 | `0x38990` | `FUN_00038630` | `MOV byte[0x157AC2],DH` (DH=4) | `88 35 c2 7a 15 00` | **front-end**: `:= 4` |
| R4 | `0x73F82` | `FUN_00073EE0` | `MOV byte[0x157AC2],DH` (DH=0) | `88 35 c2 7a 15 00` | reset `:= 0` |
| R5 | `0x4B0DE` | `FUN_0004B02C` | `MOV byte[0x157AC2],0x4` | `c6 05 c2 7a 15 00 04` | **dead** bit-8 arm (FU208-L1/FU215) |
| R6 | `0x4BA20` | `FUN_0004BA00` | `MOV byte[0x157AC2],0x8` | `c6 05 c2 7a 15 00 08` | park (`:= 8`) around a sub-call |
| R7 | `0x4BA31` | `FUN_0004BA00` | `MOV byte[0x157AC2],DL` | `88 15 c2 7a 15 00` | restore the parked value |
| R8 | `0x88D6A` | *(no function)* | `MOV byte[0x157AC2],BL` (BL=[·]+1) | `88 1d c2 7a 15 00` | act-0xC tail roll (§3.2) |

R6/R7 are a **save/restore** pair (fresh `disassemble_function 0x4BA00`: read
`[0x157A83]`, bail `-1` if 0; if `[0x157AC2] >= 5` then `:= 8`, `CALL 0x4BE44`,
restore `DL`; else `CALL 0x4BE44` directly) — not a roll producer.

**`[0x157AC0]` — OL-85 flag — 7 writers:** the reset (`0x73F58`,
`FUN_00073EE0`), the four clock-harness setters W2 `0x8B2AC` / W3 `0x8B2C1` /
W4 `0x8B3A2` / W5 `0x8B5CD` (all `FUN_0008AF38`, §3.1), the **front-end** W6
`0x389A4` (`FUN_00038630`), and the act-7 terminal W7 `0x89E8C`
(`MOV byte[0x157AC0],DH(DH=1)`, inside the phase-`0x1D` arm-9 block — *cross-cited*
FU-207 §3.4 / FU-194 §3.3; it is downstream of the flag, not an independent
producer).

**Verdict for §1.** The cells are two 1-byte BSS bytes in the clock struct;
`[0x157AC2]` has one roll writer (`0x8B58A`), one skip increment (`0x8B61B`) and
one **front-end** setter (`0x38990`); `[0x157AC0]` has four in-match tie-break
setters and one **front-end** setter (`0x389A4`). No third front-end producer
exists.

---

## 2. The front-end producers (fresh)

### 2.1 `FUN_00038630` case `0x13` — the two arms

`FUN_00038630` is a 28-case jump-table dispatcher on the app-state cell
`DAT_0014B018` (fresh `disassemble_function 0x38630`: entry
`0x3863d MOV EAX,[0x0014b018]; CMP EAX,0x1c; JA 0x39021;
JMP dword CS:[EAX*4 + 0x38544]`). The case-`0x13` table slot is fresh
`read_memory 0x38590` = `54 89 03 00` → **`0x38954`** (fresh
`read_memory 0x38544` = `0a 8e 03 00` → case 0 = `0x38E0A`, sanity). Fresh
`disassemble_bytes 0x38988` (40 B) gives both arms in full:

```
0x38989 MOV DH,0x4                     ; bytes b6 04
0x3898b MOV EAX,0xfffffffe
0x38990 MOV byte ptr [0x00157ac2],DH   ; >>> R3 (period index := 4)   [aggregate == 1]
0x38996 JMP 0x389b0
0x38998 MOV EAX,0xfffffffe
0x3899d MOV DL,0x1                     ; bytes b2 01
0x3899f MOV ECX,0x1                    ; bytes b9 01 00 00 00
0x389a4 MOV byte ptr [0x00157ac0],DL   ; >>> W6 (OL-85 flag := 1)     [aggregate == 2]
0x389aa MOV dword ptr [0x0014c2ee],ECX ; >>> AND force-extra-time override := 1
```

The controlling index is the **aggregate** `*[0x14AFC4] + *[0x14AFC8]`, selected
just above (fresh `disassemble_bytes 0x38954..`, corroborated by the
`decompile_function 0x38630` `case 0x13`):

```
0x38954 CALL 0x3ab34 ; TEST EAX,EAX ; JL 0x39021     ; state-0x13 input accessor; bail if < 0
0x38961 MOV EAX,[0x14afc4] ; MOV EDX,[0x14afc8]
0x3896c MOV EAX,[EAX] ; ADD EAX,[EDX]                 ; aggregate = *[0x14AFC4] + *[0x14AFC8]
0x38970 CMP EAX,1 ; JC 0x3897e ; JBE 0x38989          ; == 0 -> nothing ; == 1 -> R3
0x38977 CMP EAX,2 ; JZ 0x38998 ; JMP 0x389b0          ; == 2 -> W6 ; else -> nothing
```

So: aggregate `0` → no-op (`EAX := 0xFFFFFFFE`), `1` → **period index := 4**
(`0x38990`), `2` → **flag := 1 + `[0x14C2EE] := 1`** (`0x389A4`/`0x389AA`), else
no-op. This is the **"force extra time" competition arm** — the same two cells
FU-207 §3.3 and FU-215 §3.2 name.

### 2.2 The caller chain and the FU-65 gate — *not* keyboard/mouse-runtime-gated at the producer body

`FUN_00038630`'s only caller is `FUN_00039054` (fresh `get_xrefs_to 0x38630` =
**1**, `0x390dc`). Fresh `disassemble_function 0x39054` + `decompile_function
0x39054`:

```
0x39059 MOV EAX,7 ; CALL 0x36bc8 ; TEST EAX,EAX        ; FUN_00036BC8(7)  == ([0x105FFC]==7)
0x39059..0x390cd (else) MOV EAX,5 ; CALL 0x36bc8 ...   ; FUN_00036BC8(5)  == ([0x105FFC]==5)
0x390dc CALL 0x38630                                    ; the state-machine step
```

and fresh `decompile_function 0x36bc8` = `return param_1 == DAT_00105ffc;`. So the
producer runs **iff `[0x105FFC] ∈ {5,7}`** — a **screen/menu id**, not a
keyboard/mouse scan. `[0x105FFC]`'s own writers are the `0x36Bxx` screen-id
cluster (fresh `search_instructions 105ffc` = 7 sites; writers
`FUN_00036BC0 0x36BC0` `MOV [0x105FFC],EAX`, `FUN_00036BD8 0x36BE3`,
`FUN_00036C0C 0x36C2F`, `FUN_00036C3C 0x36C60`).

`FUN_00039054` is called from `FUN_000493A0` (`0x4949A`, the match-init machine
— fresh `get_function_xrefs 0x39054` = 4: `0x4949A` + three sites in
`FUN_00049B28`) and `FUN_000493A0`'s callers are the front-end match-screen arms
(*cross-cited* FU-196 §3.1 / FU-202 §1.1 / FU-207 §3.3). The case-`0x13` state is
itself entered at `0x37EA9 MOV dword[0x14B018],0x19` (fresh
`search_instructions 14b018`; un-attributed body in the `0x37Exx` range beside
`FUN_00037E20 0x37E80 :=0x18`) — an open leg (§5).

**FU-65 answer.** The producer body reads **no** keyboard/mouse port; the
immediate gate is the **screen-id cell `[0x105FFC]`**, and the FU-65 competition
screens (states 7–11) are entered on the **last input action code** (`FUN_00016350`;
FU-65 §1: "the mouse-gated claim is an input-binding question that remains
runtime-blocked"). So the route is **UI/competition- and input-gated** (runtime-
blocked), *not* a match-simulation path.

---

## 3. The setters and the roll (fresh)

### 3.1 W2 / W3 / W4 / W5 — the clock harness `FUN_0008AF38`

`FUN_0008AF38` is the per-frame clock/period-end machine; its callers are
`FUN_0004B100` `0x4B1A6` and `0x7440B` (fresh `get_function_xrefs 0x8AF38` = 2).
It selects a per-period block through a jump table **indexed by `[0x157AC2]`**
(fresh `read_memory 0x8AF28` (16 B) = `dd b1 08 00 | 2f b2 08 00 | e9 b2 08 00 |
3f b3 08 00` → `{0x8B1DD, 0x8B22F, 0x8B2E9, 0x8B33F}`; the gate is fresh
`0x8B1C3 MOV AL,[0x157AC2]; CMP AL,0x3; JA skip; JMP [EAX*4+0x8AF28]`):

* **index 1 block** → `0x8B295` (fresh `disassemble_bytes 0x8B290`, 40 B):

  ```
  0x8B295 MOV AX,[0x157AC5] ; MOV DX,[0x157AC7] ; MOV ECX,1 ; CMP AX,DX ; JZ 0x8B2B8
  0x8B2AC MOV byte[0x157AC0],0x1      ; >>> W2  (regulation end, scores differ)
  0x8B2B3 JMP 0x8AFB7
  0x8B2B8 CMP dword[0x14C2EE],0 ; JZ 0x8B2CD
  0x8B2C1 MOV byte[0x157AC0],0x1      ; >>> W3  (equal scores, [0x14C2EE] override set)
  ```

* **index 3 block** → `0x8B387` (fresh `disassemble_bytes 0x8B388`, 32 B):

  ```
  0x8B387 MOV AX,[0x157AC5] ; MOV DX,[0x157AC7] ; MOV ECX,1 ; CMP AX,DX ; JZ 0x8AFB7
  0x8B3A2 MOV byte[0x157AC0],0x1      ; >>> W4  (extra-time-2 end, scores differ)
  ```

* **no-completion tail** → `0x8B5A1..0x8B5CD` (fresh `disassemble_bytes 0x8B5C0`,
  32 B): with the period index in `{2,3}`, the scores differing and the flag clear,
  `0x8B5CD MOV byte[0x157AC0],DL` (`DL=1`) — **>>> W5** (mid-extra-time per-frame
  set).

So W2/W4 raise the flag on a **regulation (index 1)** or **extra-time-2 (index 3)**
completion with **differing `[0x157AC5]`/`[0x157AC7]`**; W3 is the equal-score
escape via the `[0x14C2EE]` override; W5 is the per-frame mid-extra-time set. All
four are the **in-match** producers the engine already mirrors (§4).

### 3.2 The roll — when the index reaches 4

Fresh `disassemble_function 0x8AF38` (475 instructions):

1. **accumulate**: `0x8AFAC ADD byte[0x157AC1],DL`; `0x8AFB9 MOV AL,[0x157AC1];
   CMP EAX,0x3c; JL 0x8B3AE` — one period minute = `0x3C` seconds.
2. **period end gate**: `0x8B3C7 TEST CX,CX; JZ 0x8B590` (`CX` = the completion
   flag set by the W2/W4 branches above).
3. **the roll**: `0x8B579 MOV BH,[0x157AC2]; 0x8B581 INC BH;
   0x8B58A MOV byte[0x157AC2],BH` — **period index := period index + 1** on a
   completed period (so `3 → 4` on the third completion).
4. **period-4 skip**: `0x8B601 MOV AL,[0x157AC2]; CMP EAX,4; JNZ 0x8B623;
   0x8B60D CMP byte[0x157AC0],0; JZ 0x8B623; CALL 0x8B9CC;
   0x8B61B INC byte[0x157AC2]` — when the index is `4` **and** the OL-85 flag is
   set, the index is incremented **to 5** (the post-extra-time hop).
5. **second roll (extra-time route)**: `0x88D56 MOV BL,[0x157AC2]; 0x88D62 INC BL;
   0x88D6A MOV byte[0x157AC2],BL` — the act-0xC tail (`0x88Dxx`) advances the
   index independently (fresh `disassemble_bytes 0x88D48`, 48 B; the enclosing
   body has **no Ghidra function** — fresh `get_function_by_address 0x88D6A` =
   none).

The **front-end** setter `0x38990` (`:= 4`) **bypasses** the roll entirely — it
jumps the index straight to the extra-time period. So "the index reaches 4" either
by **three period completions** (the live roll, `0x8B58A`) or by the **competition
force** (`0x38990`).

---

## 4. The engine mirror and the verdict (source read first-hand)

### 4.1 Both cells already exist and are live (T1/FU-213)

`include/fifa96_engine/fifa96_match_run.h`: `uint8_t global_157ac2;` (`:610`) and
`uint8_t global_157ac0;` (`:622`, FU-213 §2.4/§2.5 comment `:611..621`). Staged
zero on init/begin (`src/fifa96_engine/fifa96_match_run.c:2034/2035`,
`:2156/2157`). The mirror of `FUN_0008AF38` is
`fifa96_match_run_phase_drive`:

* **W2/W4** → `fifa96_match_run.c:2618-2619`
  (`if ((completed == 1u || completed == 3u) && differ) mr->global_157ac0 = 1u;`);
* **W5** → `:2664-2666` (period `2..3`, differ, flag clear → flag `1`);
* **the roll** → `:2643` (`mr->global_157ac2 = mr->state.period;` — the FU-62
  library's already-advanced period; the native `0x8B58A`);
* **the period-4 skip** → `:2682-2684` (`state.period == 4 → 5`, `global_157ac2 = 5`;
  the native `0x8B61B`);
* the dead bit-8 arm is mirrored dead → `fifa96_match_run_period_setup` `:3214-3216`
  (guarded on `input_range & 0x100`, which is never set — FU215).

### 4.2 The ported front-end does not reach the producers

`src/fifa96_loader/fifa96_frontend.c` + `src/fifa96_engine/fifa96_frontend_run.c`
model a **menu/panel/exit** state machine (`FIFA96_FRONTEND_PHASE_FRONTEND /
PANEL / EXIT`, a `match_start` flag, and an exit classification consumed by
`fifa96_match_bridge_from_frontend`, `fifa96_engine.c:205-209`). They model **no**
`[0x105FFC]` screen id and **no** `DAT_0014B018` state machine; the case-`0x13`
route (`FUN_00038630 ← FUN_00039054 ← FUN_000493A0`) and the `[0x14AFC4]`/
`[0x14AFC8]` competition aggregate are entirely unported. (`FU207` §6 already
records this; `fifa96_match_run.c:2582`/`:2615` name the FE writer `0x389A4` as an
unported leg.)

### 4.3 Verdict

**The front-end producers are unreachable from the ported front-end and must stay
unported legs — the faithful engine form is the match-config-default stand-in:
stage both cells zero (the native BSS default) and let the in-match clock roll +
score tie-break produce them.** This is already the committed behavior (T1/FU-213,
§4.1). The FE arm would only matter to **pre-arm** extra time from a competition
(the `[0x14C2EE]` force route), which has no engine cell (`FU213-L1`). **No new
producer is added; nothing is faked.** Consequently this slice's cells change
**no** census row: the phase-`0x1D` row was already COUNTED on the live
producer-B (period-index-4) path (`fifa96_match_handlers.c:5248`), and the FE
producers are pure legs.

---

## 5. Residual legs (numbered)

1. **FU224-L1 — the front-end `[0x157AC2] := 4` producer `0x38990`
   (`FUN_00038630` case `0x13`).** Reached only via `FUN_00039054` (screen id
   `[0x105FFC] ∈ {5,7}`) ← `FUN_000493A0`; the competition route is unported.
   Owner: the front-end setup slice (FU202-L7 / FU-207). Extends **FU215-L2**.
2. **FU224-L2 — the front-end `[0x157AC0] := 1` + `[0x14C2EE] := 1` producer
   `0x389A4` (`FUN_00038630` case `0x13`).** Same route; the force-extra-time
   override has no engine cell. Owner: **FU-207** (extends **FU207-L2**,
   FU202-L7).
3. **FU224-L3 — the `[0x14C2EE]` "force extra time" override cell.** Set by W6
   (`0x389AA`) and read by the clock harness (`0x8B2B8`, `0x88CAB`); no engine
   cell. Owner: FU-213 (extends **FU213-L1**).
4. **FU224-L4 — the `FUN_0003AB34` / `[0x14AFC4]`/`[0x14AFC8]` aggregate
   semantics.** `FUN_0003AB34` self-gates on `[0x14B018]==0x13` (fresh
   `0x3AC09`); the two pointer-cells' aggregate (`0 = none`, `1 = index 4`,
   `2 = flag`) is not decomposed here. Owner: FU-207 (extends **FU207-L2**).
5. **FU224-L5 — the `[0x105FFC]` screen-id producers and the state-`0x13`
   transition.** The writers are the `0x36Bxx` cluster
   (`0x36BC0`/`0x36BE3`/`0x36C2F`/`0x36C60`); the state-`0x13` entry is the
   un-attributed `0x37EA9 MOV dword[0x14B018],0x19`. Entry to the FU-65
   competition screens is input/action-code gated (runtime-blocked). Owner:
   FU-65 / FU-196.
6. **FU224-L6 — the un-attributed writers.** R8 `0x88D6A` (act-0xC tail roll) and
   W7 `0x89E8C` (act-7 terminal flag) sit in bodies with **no Ghidra function**
   (fresh `get_function_by_address` = none for both). Owner: FU196-L6.
7. **FU224-L7 — `FUN_0004BA00`'s park/restore (`0x4BA20`/`0x4BA31`).** A
   save/restore of `[0x157AC2]` around `FUN_0004BE44` on the controlled actor
   `[0x157A83]`; the sub-call's use of the parked index is not decomposed. Owner:
   the actor/`0x4BE44` slice.
8. **FU224-L8 — the `FUN_0008AF38` limit identities.** The index jump table
   blocks compare `[0x157AB6]` against `[0x15881A]`/`[0x15881C]` (the period
   limits; *cross-cited* FU-143 §3.5 / FU207-L3); the arm map is not re-derived
   here. Owner: FU-207 (extends **FU207-L3**).

---

## 6. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, 15239 symbols, min `0x10000`, max
  `.image::00174a2a`, `.image` overlay.
* `read_memory` — `0x157AC0` (8 B), `0x157AB8` (16 B), `0x38590` (4 B),
  `0x38544` (4 B), `0x8AF28` (16 B).
* `disassemble_bytes` — `0x38988` (40 B, the two producers), `0x8B290` (40 B, W2/W3),
  `0x8B388` (32 B, W4), `0x8B5C0` (32 B, W5), `0x88D48` (48 B, R8), `0x89E70`
  (48 B, W7).
* `disassemble_function` — `0x38630` (705 instructions), `0x39054` (54),
  `0x8AF38` (475), `0x4BA00` (22), `0x73EE0` (86).
* `decompile_function` — `0x38630`, `0x39054`, `0x36BC8`, `0x36BC0`, `0x36BD8`.
* `search_instructions` (program, 235 026–235 032 scanned) — operand `157ac2`
  (**56**), `14b018` (**62**), `105ffc` (**7**).
* `get_xrefs_to` — `0x157AC0` (**19**), `0x157AC2` (**56**), `0x38630` (1),
  `0x105FFC` (7), `0x14B018` (62).
* `get_function_xrefs` — `0x39054` (4), `0x8AF38` (2), `0x4BA00` (4), `0x73EE0`
  (3), `0x49B28` (1), `0x493A0` (3), `0x4B100` (3).
* `get_function_by_address` — `0x88D6A` = none, `0x89E8C` = none.

Engine/loader source read first-hand:
`include/fifa96_engine/fifa96_match_run.h` (`:610` `global_157ac2`, `:622`
`global_157ac0`, `:611..621` the FU-213 flag comment); `src/fifa96_engine/
fifa96_match_run.c` (`:2034/2035`, `:2156/2157` staged zero; `:2618-2619` W2/W4;
`:2643` roll; `:2643..2684` skip; `:3214-3216` dead bit-8 arm; `:2582`/`:2615` the
FE `0x389A4` leg); `src/fifa96_loader/fifa96_frontend.c`; `src/fifa96_engine/
fifa96_frontend_run.c`; `src/fifa96_engine/fifa96_engine.c:205-209`.
Docs cross-cited: **FU-65**, FU-143 §3.5, FU-194 §3.3, FU-196 §3.1/L6, FU-202
§1.1/§4, **FU-207** §3.3/§3.4/§6/§7, **FU-211**, **FU-213**, **FU-215** §3.2,
**FU-216**. No writes beyond this slice file; no
rename/label/comment/function/script/project save; `/FIFA96.EXE` and the ISO
untouched; no git commands.

---

## Landing note

This slice **pins** the two live cells `[0x157AC0]` / `[0x157AC2]` (1-byte BSS
bytes in the clock struct) and derives their front-end match-init producers
`0x389A4` / `0x38990` (`FUN_00038630` case `0x13`, gated on the screen id
`[0x105FFC] ∈ {5,7}` via `FUN_00039054` ← `FUN_000493A0`). Verdict: the front-end
producers are a **UI/competition route** (input-action-code gated per FU-65),
**unreachable from the ported front-end**; the faithful engine form is the staged-
zero default + the in-match clock roll the engine already carries (T1/FU-213).
**No new producer; nothing faked; the cells change no census row** (the
phase-`0x1D` row was already COUNTED on the live period-index-4 path,
`fifa96_match_handlers.c:5248`). `FU215-L2` / `FU207-L2` remain the residual legs,
now narrowed to the exact case-`0x13` bodies; the new legs are FU224-L1..L8.
