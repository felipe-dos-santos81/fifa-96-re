# W26-7 / FU-270 — the replay / overlay / sub-strip live producers (FU-152)

Draft for phase-26 recon wave W26, task 7 (FU-270). Promotes to
`docs/ghidra/FU270_replay_overlay.md`.

Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; explicit case-sensitive
program argument; scripts disabled; standard read-only tools only). Every
address/byte/census claim below is a **fresh** `disassemble_bytes` /
`disassemble_function` / `decompile_function` / `read_memory` /
`get_xrefs_to` / `get_function_xrefs` / `search_instructions` /
`search_byte_patterns` / `search_strings` of this session; engine source cites
are read-only reads. Code addresses equal the FU-doc link addresses; data
immediates render as flat addresses (the `+0x100000` rule applies only when
mapping a flat-bin/file offset to a flat address). Alias traps applied: the
`[0x14E674] & 0x8000` overlay-arm bit is read by Ghidra as
`DAT_0014e674._1_1_ & 0x80` = the **byte at `0x14E675`** (`>>16` of the dword);
the `[0x1577F0]`-style word reads are not used here. I did **no** rename /
comment / label / create_function / script / project save; `/FIFA96.EXE`
untouched; no git command; the only file written is this draft and its report.

**Question (brief, FU-152 legs 1/2/3/7):** derive the native producers/writers
for the four zero-gated residual presenter cells — `replay.state`
(`[0x109A98]`), `sub.active` (the `[0x1587D4]` substitution-record block behind
R1), `overlay.armed` (`[0x14E674] & 0x8000`) and the R5 `ball_row` NULL seam
(`[0x14E660]`) — and rule which live path (if any) raises them.

**Result in one line:** all four rows are **zero-gated with no ported producer**
— every native raiser lives in the **unported match-menu / overlay / event
cluster** (`FUN_000510DC`, the `FUN_000515C8` overlay driver, `FUN_00054104`,
`FUN_00054238`, `FUN_0008A938`, and the `0x63F84` arm sibling); the only writers
that touch the live cells are the **init-time reset** `FUN_000537F8` (single
caller `FUN_0004A228 0x4A272`) and the always-called-with-0
`FUN_00056690`, so the ruling is **LEG (zero-gated; unported live trigger)** for
replay/sub/overlay and **pinned dormant** for `ball_row`, matching
`docs/ENGINE.md`'s "blocked" line. The port's API (`fifa96_match_run_replay_step`
/ `_overlay_arm` / `_sub_*` / `_ball_row_reachable`) is called **only from
tests**, never from `src/`.

---

## Step 1 — the gating cells: fresh writer census and the native raisers

### 1.1 The presenter dispatch (fresh, re-confirms FU-152 §2.2)

Fresh `decompile_function 0x565BC` — the five rows and their exact gates:

```
FUN_000565BC:
  FUN_0004B380();                                                   ; mode = [0x157A4A]>>24
  if ([0x14E538] == 0) {                                            ; asset-ready gate
    if (FUN_00037AE4() == 0) {                                      ; menu probe
      if (FUN_0006400C() != 0 && mode ∉ {0xC,0x13,0x14}) {          ; live, not menu
        FUN_000560C8(); FUN_0005619C(0); FUN_0005619C(1); }}}       ; R1 sub strip
  if (FUN_00063FD0() != 0) {                                        ; [0x109A98] & 0x80
    if (FUN_00063FDC() == 0) {                                      ; != 0x81
      if (FUN_00053D58() == 0) {                                    ; not ramp 0xF1..0x168
        if ([0x14E58C] == 0) FUN_00064C34();  else FUN_000564A0(); }}}   ; R2 replay HUD / blink
  if ([0x14E688] == 0) {                                            ; not paused/suspended
    if ((DAT_0014e674._1_1_ & 0x80) != 0) { FUN_000542D4(); FUN_000550E4(); }  ; R3 overlay (arm bit)
    if ([0x14E510] == 0) {
      if (FUN_0006400C() != 0 && FUN_0004B5F4() < 4 && [0x14E59C] == 0)
        FUN_00055C24(); }                                           ; R4 match HUD (FU-148)
    if ([0x14E660] != 0) FUN_00056518(); }                          ; R5 ball row (dormant)
```

The R3 gate is the **alias-verified** arm bit: the decompiler's `._1_1_` is the
byte at `0x14E675` (`dword[0x14E674] >> 16`), and `& 0x80` there is exactly
`[0x14E674] & 0x8000`. Fresh `disassemble` of the gate line is the `FUN_000550E4`
self-arm below; the `FUN_000565BC` decompile is the load-bearing read.

### 1.2 `replay.state` = `[0x109A98]` (dword)

Fresh censuses **reconcile exactly**:

| query | fresh result |
|---|---|
| `get_xrefs_to 0x109A98` | **29** (WRITE + READ; no other address in the set) |
| `search_instructions operand 109a98` | **29** sites, all in `0x63CBC..0x64EC3` |
| `search_byte_patterns c705989a100080000000` (`MOV dword[0x109A98],0x80`) | **No matches** |

The 29 refs give the writer set (fresh `get_xrefs_to` types):

| site | native body | stores | class |
|---|---|---|---|
| `0x64284` | `FUN_00064270` | `0x81` (via EBX) | **the arm** |
| `0x63FC5` | `0x63F84` block (**no analyzed function**) | `0x81` (ESI) | the arm sibling |
| `0x64326` | `FUN_000642FC` | `0x81` (0x80→0x81 promotion) | state driver |
| `0x6436E` | `FUN_000642FC` | EDX (the 0x81 handover) | state driver |
| `0x643E9`/`0x6443D` | `FUN_000642FC` | `0x82` | driver |
| `0x6440E` | `FUN_000642FC` | EBP | driver |
| `0x644E2`/`0x644F3`/`0x64504` | `FUN_000642FC` | `0x85`/`0x84`/`0x86` | driver |
| `0x64473` | `FUN_000642FC` | EAX (exit path) | driver |
| `0x63D27` | `FUN_00063CBC` | EBP (`0x82` at ring end) | ring |
| `0x63EAE` | `FUN_00063E54` | `0x82` | ring step end |
| `0x64E67` | `FUN_00064DFC` | `0x82` | step/rewind |
| `0x64E1F` | `FUN_00064DFC` | ESI = `0x83` (`0x64E15` `MOV ESI,0x83`) | step/rewind |
| `0x64E94` | `FUN_00064E8C` | EDX (`0` on exit) | exit |
| `0x63F41` | `FUN_00063EBC` | ECX (`0` on the param_1==0 free) | buffer release |

Fresh `disassemble_function 0x64270` (the arm, 8 insns):

```
0x64270  PUSH EBX
0x64271  CMP dword ptr [0x00109ab0],0x0     ; the replay buffer allocated?
0x64278  JZ 0x0006428a
0x6427a  MOV EBX,0x81
0x6427f  CALL 0x0005865c
0x64284  MOV dword ptr [0x00109a98],EBX     ; [0x109A98] = 0x81  <-- the arm
0x6428a  POP EBX
0x6428b  RET
```

Fresh `decompile_function 0x510DC` (the replay driver) — the non-zero `param_1`
branch is the **live arm entry point**:

```
FUN_000510DC(param_1, param_2):
  ... (param_1 == 0 branch: FUN_00064E74/64DFC/64E8C step machine)
  else:
    [0x14E58C] = 1;                 ; the replay phase counter
    FUN_00064270();                 ; -> [0x109A98] = 0x81   <-- the live arm
    FUN_00037114(4);
    [0x108E0C] = 1; [0x108E10] = 0;
```

Fresh `get_function_xrefs 0x510DC` = **3**: `0x51281` (`FUN_00051270`, called with
`param_1=1` — the *replay-start key handler*), `0x512B8` (`FUN_000512B0`, with
`param_1=0` — the *replay step / pause-toggle*), and `0x5138A` (inside the
unanalyzed `0x513xx` replay cluster). Fresh `get_function_xrefs 0x64270` = **3**:
`0x383B3` (`FUN_00038004`), `0x38E81` (`FUN_00038630`), `0x5112D` (`FUN_000510DC`).

**The 0x80 state is not statically writable.** No `MOV dword[0x109A98],0x80`
bytes exist (`search_byte_patterns` no match) and no register store in the
29-site set carries 0x80 (the register stores resolve to 0 / 0x81 / 0x82 / 0x83). The
driver's `if ([0x109A98] == 0x80) [0x109A98] = 0x81;` at `0x64326` is therefore
**defensive/dead**: the arm goes **directly to 0x81**. (Contrast the engine's
enum, which keeps the `0x80→ARMED` promotion in `fifa96_match_run_replay_step`
— harmless, but it is a code path with no native writer.)

### 1.3 `sub.active` = the `[0x1587D4]` substitution-record block (R1)

There is **no native `sub.active` cell**: the fresh `FUN_000565BC` R1 arm draws
whenever `[0x14E538]==0 && menu-probe==0 && FUN_0006400C()!=0 && mode∉{0xC,0x13,0x14}`.
The strip's *content* is the record pointer `[0x1587D4]` and the flag block
`[0x1587DA..0x1587E8]`. Fresh `get_xrefs_to 0x1587D4` = **14**:

* WRITE `0x8AE54` (`FUN_0008A938`), WRITE `0x89DCF` (unanalyzed);
* READ `FUN_0004B100`/`FUN_0004B6FC`/`FUN_0004B7D0`/`FUN_0004BD38`/`FUN_0004BE30`/
  `FUN_0004C324`/`FUN_0006E330`/`FUN_00088940` and the unanalyzed `0x87D0F`/`0x89871`.

Fresh `decompile_function 0x8A938` (the match **event dispatcher**, `param_1` =
event id) — the substitution arm is **case 6/8**:

```
case 6:  ... (&DAT_001587da)[(k)*2 + side] = 1;   ; the per-side strip flags
             (&DAT_001587d8)[side] += 1;
case 8:  if ([0x157A49]>>24 != 1) {
             [0x1587D4] = (&DAT_001587cc)[idx*4];      ; <-- the record pointer (0x8AE54)
             [0x1587E8]=0; [0x1587D9]=0; [0x1587D8]=0;
             [0x1587E4] = ...;
             for (i<6) for (s<2) (&DAT_001587da)[i*2+s] = 0; }   ; clear the flag block
```

So the native substitution state is set by `FUN_0008A938(6|8, …)`, which has
**39 callers** (fresh `get_function_xrefs 0x8A938`) spread across the gameplay
event band (`0x4B0xx`, `0x742xx`, `0x754xx`…`0x777xx`, `0x7DFxx`, `0x844xx`,
`0x85Dxx`, `0x888xx`, `0x892xx`, `0x8A3xx`, `0x8B8xx`, `0x93Cxx`…`0x947xx`).
The port's `sub.active` is a **new engine gate** (native R1 has no such cell);
the port seeds it from this block per FU-152 §4.1.

### 1.4 `overlay.armed` = `[0x14E674] & 0x8000`

Fresh `get_xrefs_to 0x14E674` = **83** refs. The arm writers (set bit 15) and the
reset writer, fresh:

| site | native body | effect |
|---|---|---|
| `0x5414C` | `FUN_00054104` (`0x54147 OR AH,0x80`) | `[0x14E674] = id \| 0x8000` |
| `0x542A2` | `FUN_00054238` | `[0x14E674] = id \| 0x8000` (or retarget) |
| `0x5518C` | `FUN_000550E4` (`0x55187 OR AH,0x80`) | self-arm `[0x14E674] = id \| 0x8000` |
| `0x515FC`/`0x51830` | `FUN_000515C8` (overlay driver, unanalyzed) | clear / fold |
| `0x5389E` | `FUN_000537F8` (`XOR EDI,EDI`) | **reset to 0** |

Fresh `disassemble_bytes 0x54140` (the armer, alias-confirming `OR AH,0x80`):

```
0x54145  MOV EAX,EDX
0x54147  OR AH,0x80               ; EDX = id; AH|=0x80 -> bit 15 = the arm bit
0x5414a  XOR EBP,EBP
0x5414c  MOV [0x0014e674],EAX     ; [0x14E674] = id | 0x8000
0x54151  LEA EAX,[EDX + -0x5]
0x54154  MOV dword ptr [0x0014e684],EBP
0x5415a  CMP EAX,0xd ; JA 0x541ce ; switch on (id-5) -> the {5,9,0xB..0x12} second-overlay seed
0x54163  JMP dword ptr CS:[EAX*0x4 + 0x540cc]
```

Fresh `disassemble_bytes 0x55178` (the `FUN_000550E4` self-arm):

```
0x5517b  MOV EAX,0x14e674
0x55180  CALL 0x00053e2c
0x55185  MOV EAX,EDI
0x55187  OR AH,0x80
0x5518a  MOV EBX,EDI
0x5518c  MOV [0x0014e674],EAX     ; [0x14E674] = id | 0x8000
0x55193  LEA EDX,[EDI + -0x5] ; ... switch 0x55064
```

Fresh `disassemble_bytes 0x53890` (the reset writer `FUN_000537F8`):

```
0x5389e  MOV dword ptr [0x0014e674],EDI   ; EDI = 0 -> disarmed
0x538a4  MOV dword ptr [0x0014e678],EDI   ; the id source, cleared
0x538b6  MOV dword ptr [0x0014e684],EDI   ; the overlay timer, cleared
0x538bc  MOV dword ptr [0x0014e688],EDI   ; the suspend gate, cleared
```

**Raiser callers (fresh).** `get_function_xrefs 0x54104` = **5**:
`0x8B3C2` (`FUN_0008AF38`, the match **clock/period** machine → `FUN_00054104(0,…)`
= the period announcement), `0x3BBC5` (`FUN_0003BB1C`), `0x503B8`
(`FUN_0004FF8C`, the **view-mode 0x14** camera handler → `FUN_00054104(0xB,…)`
= the substitution/team screen), `0x93762` (`FUN_000935A0`), and `0x51586`
(inside the unanalyzed `0x5154C` sibling).
`get_function_xrefs 0x54238` = **4**, all inside the **overlay driver**
`FUN_000515C8`: `0x51694` (id 5/9), `0x51701`, `0x51775` (id 0xF), `0x517D6`
(id 0xD).

Fresh `disassemble_bytes 0x51590..0x516CF` — the overlay driver `FUN_000515C8`
(jump table `CS:[EAX*4 + 0x515a8]`, switch on `[0x108E24]` 0..7):

```
0x515d3  CALL 0x0004937c            ; tick
0x515f4  JMP dword ptr CS:[EAX*0x4 + 0x515a8]   ; the 8-way overlay switch
0x51631  MOV EDX,[0x00108e14]       ; the overlay timer accumulator
0x51643  CMP ESI,0x3c ; JLE ...     ; the arm cooldown gate
0x5167d  CALL 0x00045069
0x51694  CALL 0x00054238            ; arm [0x14E674] with id 5 or 9
0x51775  CALL 0x00054238            ; id 0xF
0x517d6  CALL 0x00054238            ; id 0xD
```

`get_xrefs_to 0x108E24` = **10**, every ref inside `0x515E0..0x518E7` (the driver
only). The driver has no Ghidra function xref, but it **is** table-dispatched:
fresh `search_byte_patterns c8150500` (= the pointer `0x000515C8`) = **1** at
**`0x108D84`** (the pointer dword; the following flag dword `6` sits at
`0x108D88` — fresh `read_memory 0x108D84` = `c8 15 05 00 / 06 00 00 00 / b0 12 05
00 / 06 00 00 00`), in the menu/overlay function-pointer table
(entries `{0x51460,0x512B0,0x51444,0x515C8,0x512B0,0x51444,0x518B0,0x5197C,…}`).
The sibling armer `FUN_0005154C` (retargets `[0x14E678]` when already armed,
fresh `disassemble_bytes 0x51540`) has **no** static xref and no pointer bytes
(`4c150500` → no match) — it is unreferenced in the analyzed image (a
computed/indirect target at most).

### 1.5 `ball_row` = `[0x14E660]` (R5, the NULL seam)

Fresh `get_xrefs_to 0x14E660` = **3** (matches FU-152 §2.5):

| site | type | body |
|---|---|---|
| `0x53901` | WRITE | `FUN_000537F8` reset (`[0x14E660] = 0`) |
| `0x5667C` | READ | `FUN_000565BC` (the R5 gate) |
| `0x56690` | WRITE | `FUN_00056690` (`[0x14E660] = p`) |

Fresh `get_function_xrefs 0x56690` = **1**: `FUN_00056CF4` at `0x56E41`. Fresh
`disassemble_bytes 0x56DE0` (the call tail):

```
0x56e1b  XOR EDX,EDX                 ; EDX := 0
0x56e1d  MOV byte ptr [EAX],0x0
0x56e20  XOR EAX,EAX
0x56e22  MOV ESI,[0x001543b0]        ; the source triple
0x56e28  LEA EDI,[EAX + 0x14eaf0]
0x56e2e  ADD EAX,0xc
0x56e31  MOVSD ES:EDI,ESI            ; the copy loop (no EDX write)
0x56e32  MOVSD ES:EDI,ESI
0x56e33  MOVSD ES:EDI,ESI
0x56e34  CMP EAX,0x3c ; JNZ 0x56e22
0x56e39  MOV EAX,EDX
0x56e3b  MOV dword ptr [0x00109004],EDX
0x56e41  CALL 0x00056690             ; FUN_00056690(0)  <-- always EDX = 0
```

The MOVSD loop never writes EDX, so `FUN_00056690` is **always called with 0**:
`[0x14E660]` is zero in the image and `FUN_00056518` never draws. **Dormancy
re-confirmed fresh** (the earlier FU-152 §2.5 claim stands).

### 1.6 Port-side zero-gating (read-only source cites)

Fresh grep of `src/` + `include/`: the engine's residual-row API is referenced
**only from tests**:

* `fifa96_match_run_replay_step` — callers: `tests/test_engine_match_render.c`
  (`:1576..1607`), none in `src/`;
* `fifa96_match_run_overlay_arm` / `_timeout_step` — tests only;
* `fifa96_match_run_sub_mark` / `_sub_numbers` — the internal draw
  `match_run_draw_sub_strip` (`src/fifa96_engine/fifa96_match_run.c:4868`) plus
  tests;
* `fifa96_match_run_ball_row_reachable` — the header contract only.

`render.replay.state` is written to `FIFA96_MATCH_REPLAY_LIVE` at reset
(`fifa96_match_run.c:4620`, `fifa96_match_run_replay_exit`) and by
`fifa96_match_run_replay_step` (test-driven); `render.sub.active`,
`render.overlay.armed` and `render.overlay.id` are **never** set outside tests
(`tests/test_engine_m2.c:2451` = `replay.state`/`hud_armed`, `:2452` =
`sub.active`, `:2453` = `overlay.armed`/`id`, `:2454` = `ball_row == NULL`, and
the same block at `:2853`/`:2854`/`:2855`/`:2856`); `render.ball_row`
stays NULL. The render body calls all four row draws unconditionally
(`fifa96_match_run.c:5188-5192`), each early-outing on its zero/NULL gate
(`match_run.c:4856`, `4908`, `4925`, `4944`).

---

## Step 2 — legs 1/2/3/7: the producer shapes

### 2.1 Leg 1 — replay HUD strings and the caption id table (R2, `FUN_00064C34`)

Fresh `read_memory 0x102100` (48 B):

```
72 65 70 6c 61 79 20 6c 6f 61 64 65 64 0a 00   "replay loaded\n"
31 25 73 20 31 00                               "1%s 1"      (the "%s 1" at 0x102110)
74 30 32 25 73 20 32 00                         "t02%s 2"    (the "%s 2" at 0x102118)
74 30 33 25 73 00 00                            "t03%s"      (the "%s"   at 0x102120)
```

Fresh `disassemble_bytes 0x64C60` — `FUN_00064C34` pushes those exact literals:
`PUSH 0x102110` at `0x64C73`, `PUSH 0x102118` at `0x64C8B`, `PUSH 0x102120` at
`0x64C9E` (so the `"%s 1"` base is **`0x102110`**, not the `0x10210F` FU-152
rendered as "`0x10210F`+1").

So the HUD format strings are `"%s 1"`/`"%s 2"`/`"%s"` (FU-152 §2.3 agrees; the
fresh bytes show the `1`/`t02`/`t03` prefixes are the tail of an adjacent
literal). Fresh `read_memory 0x109AD0` (16 B) = `e4 01 00 00 / e5 01 00 00 /
e5 01 00 00 / e6 01 00 00` → the caption cells hold the exact four-cell row
**`[0x109AD0]=0x1E4, [0x109AD4]=0x1E5, [0x109AD8]=0x1E5, [0x109ADC]=0x1E6`**
(string-table ids, not zero, not raw text — resolved by `FUN_0001771C` at draw
time). **Erratum to FU-152 leg 1** ("the caption ids `[0x109AD0..0x109AD8]` are
runtime-filled"): in this image the row is the populated constants
`0x1E4/0x1E5/0x1E5/0x1E6`; the *text* of those ids lives in the language
resource, which is the part not statically derivable.

### 2.2 Leg 2 — the blinking caption id 0x18C (R2, `FUN_000564A0`)

Fresh `decompile_function 0x564A0`:

```
FUN_000564A0:
  [0x108FC4] += FUN_000492CC();          ; the blink counter += tick
  if ([0x108FC4] < 0x14) { if (9 < [0x108FC4]) return; }   ; 10..19 -> no draw
  else [0x108FC4] = 0;                   ; >= 0x14 -> reset
  FUN_00019E9C(1);                       ; colour 1
  FUN_0001771C(0x18C);                   ; string-table id 0x18C (the caption)
  FUN_000A1720();                        ; draw at the current cursor
```

Fresh `get_function_xrefs 0x564A0` = **1** (`FUN_000565BC 0x5662A`). The caption
text for id 0x18C is a language-resource string (not in the EXE) — leg.

### 2.3 Leg 3 — replay HUD button semantics (R2, `FUN_00064C34` / `FUN_00064690`)

Fresh `get_xrefs_to 0x155C78` = **3**: READ `0x64D49`/`0x64D58` (`FUN_00064C34`,
the button-highlight switch) and WRITE `0x6490F` (`FUN_00064690`, the button
descriptor installer). So the button handles `[0x155C78/0x155C7C/0x155C88/
0x155C8C]` are installed by `FUN_00064690` and selected by the state switch in
`FUN_00064C34` (FU-152 §2.3: `0x82->0x155C8C`, `0x83->0x155C88`,
`0x84->0x155C7C`, `0x85->0x155C78`). The glyphs those handles resolve to are
runtime assets — leg 3 (FU-152).

### 2.4 Leg 7 — the overlay case arms (R3, `FUN_000550E4`)

Fresh `get_function_xrefs 0x550E4` = **1** (`FUN_000565BC 0x5664D`). The body is
the R3 screen switch (`switch([0x14E674]&0xFF)`) already catalogued in FU-152
§2.7; the per-case helpers (`FUN_0004BB2C`/`FUN_0004BB5C`/`FUN_0004BEAC`/
`FUN_0004B840`/`FUN_0004B860`) and the id tables stay cited-not-decomposed.
Fresh `read_memory 0x151090` (48 B) = **all zero** — the period id table is
runtime-filled (matches FU-152 §2.7). Fresh `read_memory 0x1510A8` (via FU-152)
likewise zero.

---

## Step 3 — the live reach and the `ball_row` NULL case

### 3.1 Native reachability (the producers exist and are natively reachable)

The residual rows are **not** dead on the native side — each has a
live-reaching native raiser, all in code the port has **not** wired:

| row / cell | native live raiser (fresh xrefs) | trigger context |
|---|---|---|
| `replay.state` | `FUN_000510DC` (the replay driver) via `FUN_00051270` (replay key), `FUN_000512B0` (step/pause toggle), `0x5138A` | match **replay menu** (`FUN_00064270` → 0x81; driver → 0x82) |
| `sub.active` | `FUN_0008A938(6\|8,…)` (39 callers) writes `[0x1587D4]`/flags | substitution **event** in the gameplay band |
| `overlay.armed` | `FUN_00054104` via `FUN_0008AF38` (clock, id 0), `FUN_0004FF8C` (view-mode 0x14, id 0xB); `FUN_00054238` via the `FUN_000515C8` overlay driver (ids 5/9/0xD/0xF); `FUN_000550E4` self-arm | period/annex, substitution screen, extra-time; the 5/9/0xD/0xF driver |
| `ball_row` | **none** — `FUN_00056690` only caller passes 0 | dormant |

### 3.2 Port reachability = zero (the ruling input)

The port exposes the *effects* of those producers (FU-152 P4: `fifa96_match_run_
replay_step/sub_*/overlay_arm/overlay_timeout_step`, `fifa96_*_replay_select`),
but **no ported producer raises the gates on a real engine path**:

* the replay armer (`FUN_000510DC`/`FUN_00064270`) has **no engine caller**;
* the substitution event (`FUN_0008A938`) and the D4 record block are **not
  staged** (the `sub` struct ships zeroed; `sub.active` never set);
* the overlay armers (`FUN_00054104`/`FUN_00054238`/`FUN_000550E4` self-arm) are
  **not** called; the period/screen machine that drives them is unported;
* `render.ball_row` is left NULL.

The M2 tape therefore pins `replay.state = 0`, `sub.active = 0`,
`overlay.armed = 0`, `overlay.id = 0`, and `ball_row == NULL`
(`tests/test_engine_m2.c:2451`-`2454` / `:2853`-`2856`), and the four row draw
steps early-out (`match_run.c:4856/4908/4925/4944`).

### 3.3 The `ball_row` NULL case (explicit)

`FUN_00056518` is the R5 ball-sprite row; its gate `[0x14E660] != 0` is never
true because the **only** nonzero-capable writer `FUN_00056690` is called from a
single site (`FUN_00056CF4 0x56E41`) with `EDX = 0` (fresh `XOR EDX,EDX` at
`0x56E1B`, no EDX write in the MOVSD loop), and the reset `FUN_000537F8 0x53901`
zeroes it. The port mirrors this with `render.ball_row == NULL` and
`fifa96_match_run_ball_row_reachable()` returning
`mr != NULL && mr->render.ball_row != NULL` (`match_run.c:4829`). **Pinned
dormant** — a runtime/computed writer into `0x14E6xx` is the only way the row
could resurrect (leg 6).

### 3.4 The live-vs-leg ruling

**LEG (zero-gated; unported live trigger)** for `replay.state`, `sub.active` and
`overlay.armed`; **pinned dormant** for `ball_row`. On the port there is no live
producer, so a claim that any of the three rows is live is a **fixture-only
stand-in** (the tests stage the cells by hand). This confirms `docs/ENGINE.md`
("replay / overlay / sub strip (live) | blocked | zero-gated rows with no
producers yet (FU-152 legs 1/2/3/7)").

### 3.5 Layer-1 / Layer-2 reachability framing (brief)

The phase-26 design defines two acceptance layers
(`docs/superpowers/specs/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-design.md:42-46`):
**Layer 1 — engine-reach** (a live engine path raises the gate cell, proven by a
headless granted-frame test) and **Layer 2 — visible** (the chain is visible in
the running smoke). Applied to this slice, fresh:

* **Layer 1 (engine-reach) = NOT reached.** No ported producer raises
  `replay.state` / `sub.active` / `overlay.armed`; the cells are pinned 0/NULL
  and the only ported calls into the row API are the tests
  (`tests/test_engine_match_render.c`, `tests/test_engine_m2.c`). The rows are
  **granted-frame / fixture-only**, not headless-live. `ball_row` is not merely
  unreached but **dormant by construction** (the native writer always passes 0).
* **Layer 2 (visible) = NOT reached.** The static camera/HUD tape never stages
  the cells, so `match_run_draw_sub_strip` / `_replay` / `_overlay` / `_ball_row`
  all early-out (`match_run.c:4856/4908/4925/4944`) and the smoke draws none of
  the three residual rows. There is no visible anchor for this slice (the
  Layer-2 anchor of the wave is the follow-cam, W26-1/FU264).
* **Consequence for Stage 2.** Neither layer is reachable until a live caller of
  the native raisers is ported (FU270-L1/L2/L3); the `ball_row` seam is final.

---

## Step 4 — the port shapes and the hand-off

### 4.1 Producer port shapes (what a live trigger would add)

* **`replay.state`.** The port already carries `fifa96_match_run_replay_step`
  (0x80→ARMED, the 0x81 arm, button 1 toggle, 0x20 camera cycle mod 6, 8/4/2
  step modes, the bit-0x80 exit) and `fifa96_match_run_replay_exit`. The missing
  piece is the **arm entry**: a live caller of the native `FUN_000510DC(1,…)`
  (`fifa96_match_run_replay_step` currently starts only from an externally
  staged non-LIVE state). The native arms to port: `FUN_000510DC` non-zero branch
  (`[0x14E58C]=1; FUN_00064270(); FUN_00037114(4); [0x108E0C]=1`) and its key
  callers (`FUN_00051270` replay-start, `FUN_000512B0` step/pause).
* **`sub.active`.** Port the substitution-event writer `FUN_0008A938` case 6/8
  (the `[0x1587D4]`/`[0x1587DA]` record seed) as the setter of
  `run.render.sub.active` + the block fields; the R1 gate itself (live, not menu,
  mode ∉ {0xC,0x13,0x14}) is already in `match_run_draw_sub_strip`
  (`match_run.c:4860-4861`).
* **`overlay.armed`.** Port the armer `FUN_00054104` (`id | 0x8000` + the
  `{5,9,0xB,0xC,0xD,0xE,0xF,0x12}` second-overlay seed) as
  `fifa96_match_run_overlay_arm` (already present) **and** its live caller(s):
  the clock `FUN_0008AF38` (id 0), the view-mode-0x14 handler `FUN_0004FF8C`
  (id 0xB), and the overlay driver `FUN_000515C8` (ids 5/9/0xD/0xF, gated by the
  `[0x108E14] > 0x3c` / `> 0x12c` cooldown + the `FUN_00045069` probe).
* **`ball_row`.** No producer to port — the native writer is always 0. Keep the
  seam NULL and `fifa96_match_run_ball_row_reachable` reporting it (already
  done).

### 4.2 Live-trigger ruling (hand-off)

The three arm cells are **not live** in the port: their native raisers sit in the
unported match-menu / overlay / event-dispatch cluster
(`FUN_000510DC`, `FUN_000515C8`, `FUN_00054104`, `FUN_00054238`, `FUN_0008A938`).
The only writers that touch the live cells are the **init-time reset**
`FUN_000537F8` (fresh `get_function_xrefs 0x537F8` = 1: `FUN_0004A228` at
`0x4A272` — match setup, not the open-play frame body) and the always-0
`FUN_00056690`; every other writer is behind the unported menu/overlay gates.
Stage 2 must port at least the replay arm entry, the
substitution-event writer and the overlay armer **plus** a live caller from the
period/screen machine, or the rows stay zero-gated. The `ball_row` NULL seam is
final (dormant).

---

## Numbered open legs (FU270-L*)

* **FU270-L1 — the replay arm entry.** No ported producer raises
  `replay.state` off LIVE. Native owners: `FUN_000510DC` (the arm, non-zero
  `param_1`), `FUN_00064270` (`0x64284`), `0x63F84` (unanalyzed sibling), with
  key callers `FUN_00051270` (`0x51281`) / `FUN_000512B0` (`0x512B8`) /
  `0x5138A`. Also open: the **0x80 state has no static writer** (the arm goes
  straight to 0x81); the port's `0x80→ARMED` promotion is a native-dead path.
* **FU270-L2 — the substitution-event writer.** `FUN_0008A938` case 6/8 sets the
  `[0x1587D4]` record + flags; 39 callers, none ported. The record field
  meanings and the `[0x1587CC]` record table stay FU-145 L4. Native owner:
  `FUN_0008A938` (`0x8AE54` write).
* **FU270-L3 — the overlay armers and their live callers.** `FUN_00054104`
  (5 callers: `FUN_0008AF38` id 0, `FUN_0004FF8C` id 0xB, `FUN_0003BB1C`,
  `FUN_000935A0`, the `0x5154C` sibling), `FUN_00054238` (4 callers, all inside
  the overlay driver `FUN_000515C8`), the `FUN_000550E4` self-arm, and the
  `FUN_000512B0` suspend arm. None ported. Native owners: those bodies.
* **FU270-L4 — the overlay driver `FUN_000515C8`.** The 8-way `[0x108E24]`
  switch (jump table `0x515A8`, dispatch pointer `0x000515C8` at `0x108D84`, flag
  `6` at `0x108D88`) with the
  `[0x108E14] > 0x3c`/`> 0x12c` cooldown + `FUN_00045069` probe and the
  `{5,9,0xD,0xF}` arms; and the **unreferenced sibling `FUN_0005154C`** (no
  static xref, no pointer bytes). Native owner: the driver.
* **FU270-L5 — the replay HUD/leg text (legs 1/2/3).** The caption-id row
  `[0x109AD0]=0x1E4 / [0x109AD4]=0x1E5 / [0x109AD8]=0x1E5 / [0x109ADC]=0x1E6`
  and the blink id `0x18C` resolve to
  language-resource strings not in the EXE; the format strings
  (`0x102110`/`0x102118`/`0x102120`) and the button-descriptor install
  `FUN_00064690` (`[0x155C78]` `0x6490F`) are the native side. Native owners:
  `FUN_00064C34`, `FUN_000564A0`, `FUN_00064690`.
* **FU270-L6 — the `ball_row` resurrection condition.** Dormancy is proven for
  the static writers only (`FUN_00056690` single caller, `EDX=0`); a
  runtime/computed writer into `0x14E6xx` would resurrect the row (ball.fsh
  entry 37, 8 frames — FU-152 leg 6). Native owner: the `0x14E660` cell.
* **FU270-L7 — the overlay case-arm helpers (leg 7).** `FUN_000550E4`'s
  `FUN_0004BB2C`/`FUN_0004BB5C`/`FUN_0004BEAC`/`FUN_0004B840`/`FUN_0004B860`
  and the runtime-filled id tables `0x151090`/`0x1510A8` (fresh = all zero).
  Native owner: `FUN_000550E4`.

---

## Evidence table (this session)

| claim | evidence (fresh) |
|---|---|
| presenter rows + gates (R1/R2/R3/R4/R5) | `decompile_function 0x565BC` |
| `[0x109A98]` census = 29 (xrefs == operand search) | `get_xrefs_to 0x109A98`; `search_instructions operand 109a98` |
| no `MOV dword[0x109A98],0x80` exists | `search_byte_patterns c705989a100080000000` → no match |
| the arm = `FUN_00064270` → 0x81 | `disassemble_function 0x64270` |
| the live arm = `FUN_000510DC` non-zero branch | `decompile_function 0x510DC`; `get_function_xrefs 0x510DC` = 3 |
| `FUN_00064270` callers = 3 | `get_function_xrefs 0x64270` |
| `[0x1587D4]` census = 14; writer `FUN_0008A938` case 6/8 | `get_xrefs_to 0x1587D4`; `decompile_function 0x8A938`; `get_function_xrefs 0x8A938` = 39 |
| `[0x14E674]` census = 83; armer `FUN_00054104` / `FUN_00054238` / self-arm / reset | `get_xrefs_to 0x14E674`; `disassemble_bytes 0x54140/0x55178/0x53890` |
| armer callers = 5 / 4 | `get_function_xrefs 0x54104`; `get_function_xrefs 0x54238` |
| overlay driver `FUN_000515C8` (switch `0x515A8`, `[0x108E24]`), dispatch pointer `0x108D84` | `disassemble_bytes 0x51590`; `get_xrefs_to 0x108E24` = 10; `search_byte_patterns c8150500` → `0x108D84`; `read_memory 0x108D84` |
| `FUN_0005154C` unreferenced | `disassemble_bytes 0x51540`; no xref, `search_byte_patterns 4c150500` → no match |
| `[0x14E660]` census = 3; dormant (writer always 0) | `get_xrefs_to 0x14E660`; `get_function_xrefs 0x56690` = 1; `disassemble_bytes 0x56DE0` |
| replay HUD format strings (`0x102110`/`0x102118`/`0x102120`) | `disassemble_bytes 0x64C60` (`PUSH 0x102110` at `0x64C73`); `read_memory 0x102100` |
| replay HUD caption ids / caption read | `read_memory 0x109AD0`; `search_strings replay` |
| blink id 0x18C | `decompile_function 0x564A0`; `get_function_xrefs 0x564A0` = 1 |
| button handles `[0x155C78]` | `get_xrefs_to 0x155C78` = 3 |
| overlay case table / id tables zero | `get_function_xrefs 0x550E4` = 1; `read_memory 0x151090` |
| reset `FUN_000537F8` invoked at match init | `get_function_xrefs 0x537F8` = 1 (`FUN_0004A228 0x4A272`) |
| port API is test-only; cells zero at rest | grep of `src/`, `tests/test_engine_m2.c:2451/2452/2453/2454` + `:2853/2854/2855/2856`; `fifa96_match_run.c:4829/4860/4908/4925/4944/5188-5192` |

Engine source read: `src/fifa96_engine/fifa96_match_run.c` (4556-4712, 4746-4875,
4890-4949, 5175-5193), `include/fifa96_engine/fifa96_match_run.h` (131-303),
`tests/test_engine_m2.c` (407-408, 2451-2454, 2853-2856),
`tests/test_engine_match_render.c` (1576-1710). Carried docs: `docs/ENGINE.md`
(known-gaps ~1749-1756), `docs/ghidra/FU152_presentation_residual.md` §2.2-2.7/§5.
Layer framing: `docs/superpowers/specs/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-design.md:42-46`.

## Provenance (Ghidra MCP, read-only, `/FIFA96.EXE`)

`decompile_function` 0x565BC, 0x564A0, 0x642FC, 0x63EBC, 0x63E54, 0x64270,
0x64DFC, 0x64E8C, 0x63CBC, 0x510DC, 0x51270, 0x512B0, 0x54104, 0x54028, 0x54238,
0x537F8, 0x8A938, 0x8AF38, 0x4FF8C;
`disassemble_function` 0x64270;
`disassemble_bytes` 0x51540 (80 B), 0x51590 (320 B), 0x512D0 (200 B),
0x5149C (176 B), 0x51700 (224 B), 0x56DE0 (112 B), 0x53890 (64 B),
0x54140 (48 B), 0x55178 (48 B), 0x63F80 (80 B), 0x63E90 (40 B),
0x64C60 (64 B), 0x64E0C (48 B);
`read_memory` 0x102100 (48 B), 0x109AD0 (16 B), 0x151090 (48 B),
0x108C3C (256 B), 0x108D60 (192 B), 0x108D84 (32 B);
`get_xrefs_to` 0x109A98 (29), 0x14E674 (83), 0x14E660 (3), 0x14E678 (5),
0x1587D4 (14), 0x109AC4 (6), 0x155C78 (3), 0x108E14 (27), 0x108E24 (10),
0x109AB0 (7);
`get_function_xrefs` 0x510DC (3), 0x64270 (3), 0x54104 (5), 0x54238 (4),
0x8A938 (39), 0x56690 (1), 0x564A0 (1), 0x550E4 (1), 0x537F8 (1);
`get_function_by_address` 0x5154C / 0x515C8 / 0x51694 / 0x5138A / 0x5194F /
0x51880 / 0x518B0 = "No function"; `search_instructions operand 109a98` (29),
`5154` (2); `search_byte_patterns` `c705989a100080000000` (none),
`4c150500` (none), `c8150500` (0x108D84); `search_strings replay` (4).
No Ghidra writes, no project saves, no repo edits; this draft (and its report)
are the only files written. No git command.

---

## Fix round 1 (2026-10-14) — review corrections, all re-verified first-hand

1. **Format-string address.** Corrected `"%s 1"` from `0x10210F` to **`0x102110`**
   (§2.1, FU270-L5, evidence table). Fresh `disassemble_bytes 0x64C60`:
   `PUSH 0x102110` at `0x64C73` (`68 10 21 10 00`); `PUSH 0x102118` at `0x64C8B`
   and `PUSH 0x102120` at `0x64C9E` (already correct).
2. **Overlay dispatch pointer slot.** Corrected the `0x000515C8` pointer slot from
   `0x108D88` to **`0x108D84`** (flag `6` at `0x108D88`) in §1.4, FU270-L4 and the
   evidence table; and the table's 7th pointer from `0x51880` to **`0x518B0`**.
   Fresh `read_memory 0x108D84` = `c8 15 05 00 / 06 00 00 00 / b0 12 05 00 /
   06 00 00 00`; `search_byte_patterns c8150500` → `0x108D84`. The
   table-dispatched-reachability claim is unchanged.
3. **`[0x109A98]` writer enumeration.** Added `0x6440E`
   (`MOV [0x109A98],EBP`, `FUN_000642FC`) and split `0x64E1F` out of the
   `0x64E67` row, noting it stores `0x83` (fresh `disassemble_bytes 0x64E0C`:
   `MOV ESI,0x83` at `0x64E15`, `MOV [0x109A98],ESI` at `0x64E1F`). The
   no-static-`0x80` conclusion stands.
4. **Reset caller traced.** `get_function_xrefs 0x537F8` = **1**
   (`FUN_0004A228` at `0x4A272`), so the reset is init-time; the "only writers
   that touch the live cells" claims in the summary and §4.2 now cite the caller
   and no longer rest on an untraced edge. `0x53901` added to §1.5.
5. **Test line ranges.** Corrected to the per-assert lines
   `tests/test_engine_m2.c:2451/2452/2453/2454` and `:2853/2854/2855/2856`
   (fresh `sed`: `replay.state` 2451, `sub.active` 2452, `overlay.armed/id`
   2453, `ball_row` 2454; same block at 2853-2856).
6. **Leg-1 erratum.** Now states the exact four-cell row
   `[0x109AD0]=0x1E4, [0x109AD4]=0x1E5, [0x109AD8]=0x1E5, [0x109ADC]=0x1E6`.
7. **Layer-1 / Layer-2 framing added** (§3.5): Layer-1 engine-reach = NOT reached
   (no ported producer; tests-only API); Layer-2 visible = NOT reached (the rows
   never draw; the slice has no visible anchor).
