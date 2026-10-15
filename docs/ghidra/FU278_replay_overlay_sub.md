# FU-278 — the replay / overlay / sub live caller (period/screen machine)

Promoted canonical slice — phase-26b (FU278); read-only Ghidra session, authoritative program **`/FIFA96.EXE`**; case-sensitive. No rename/comment/label/function/script/project save.

Phase/Provenance: M2 phase-26b recon-ahead wave `2026-10-14-fifa96-m2-phase26b-recon-ahead`, Task 6 (bounded, read-only derivation), promoted to canonical by the phase-26b close-out. No production code; the derivation and its ruling are unchanged by promotion.

# W26b-6 / FU278 — the replay / overlay / sub live caller (period/screen machine)

Draft slice for phase-26b recon-ahead (Task 6). Promotes to `docs/ghidra/FU278_replay_overlay_sub.md`.
Read-only Ghidra session, authoritative program **`/FIFA96.EXE`** (case-sensitive
program argument; scripts disabled; standard MCP tools only). Every
address/byte/xref-count claim below is a **fresh** `disassemble_bytes` /
`disassemble_function` / `decompile_function` / `read_memory` / `get_xrefs_to` /
`get_function_xrefs` / `search_instructions` / `search_byte_patterns` of this
session. No Ghidra writes, no project save. Alias traps noted inline. Data
immediates render as flat addresses (the `+0x100000` rule applies only when
mapping a flat-bin/file offset to a flat address).

Supersedes-in-part FU270 (W26-7 / FU-270), which established the **zero-gated**
verdict for these rows; this slice derives the **live native caller chain** that
FU270 left as "the unported live trigger". Where FU270 said "no ported producer",
that stands; what is new here is the **exact period/screen-machine path** and its
descriptor mechanism (the missing live caller the Task-6 brief asks for).

**Question (brief, Task 6 steps 1–4):** derive the live **caller from the
period/screen machine** for the native raisers `FUN_000510DC` (replay arm),
`FUN_00054104` / `FUN_000515C8` (overlay), and the substitution-event writer
`FUN_0008A938` case 6/8; plus the `ball_row` `[0x14E660]` verdict.

**Result in one line:** the live chain is the **per-tick gameplay dispatcher
`FUN_0004B100`** (called from the match loop `FUN_000493a0`), which (a) ticks the
match clock `FUN_0008AF38` (→ `FUN_00054104(0)` period overlay) and (b) invokes
the screen-descriptor handler `(*[0x14E5A8])([0x14E5AC])` via `FUN_000513EC` /
`FUN_00053BB8`; the descriptor is installed by the **mode/screen machine
`FUN_00052030`** (via the state machine `FUN_00053A48`), whose replay handler set
`[0x108C7C]` holds `FUN_00051270` / `0x51370` (replay start) and `FUN_000512B0`
(step), each of which calls `FUN_000510DC` → `FUN_00064270` → `[0x109A98]=0x81`.
The overlay driver `FUN_000515C8` is installed as a descriptor handler from
`[0x108D7C]`. `ball_row [0x14E660]` is **pinned dormant** (its only nonzero-capable
writer is called with `EDX=0`). **Ruling for the port: none — LEG** (zero-gated;
the whole native chain is unported), confirmed by `docs/ENGINE.md`.

---

## Step 1 — the replay arm entry and its live caller

### 1.1 `FUN_000510DC` and the arm, fresh (re-confirms FU270 §1.2)

Fresh `get_function_xrefs 0x510DC` = **3**:

| site | caller | role |
|---|---|---|
| `0x51281` | `FUN_00051270` | replay **start** handler (`FUN_000510DC(1,0)`) |
| `0x512B8` | `FUN_000512B0` | replay **step / pause** handler (`FUN_000510DC(0,…)`) |
| `0x5138A` | the `0x51370` handler (no Ghidra function) | second replay **start** variant |

Fresh `decompile_function 0x510DC` — the non-zero `param_1` branch is the arm entry:

```
FUN_000510DC(param_1, param_2):
  if (param_1 == 0) { ... step machine ... }
  else {
    [0x14E58C] = 1;                 ; replay phase counter
    FUN_00064270();                 ; -> [0x109A98] = 0x81   (the arm)
    FUN_00037114(4);
    [0x108E0C] = 1; [0x108E10] = 0;
  }
```

Fresh `disassemble_function 0x64270` (8 insns) — the arm writer:

```
0x64270  PUSH EBX
0x64271  CMP dword ptr [0x00109ab0],0x0     ; replay buffer allocated?
0x64278  JZ 0x0006428a
0x6427a  MOV EBX,0x81
0x6427f  CALL 0x0005865c
0x64284  MOV dword ptr [0x00109a98],EBX     ; [0x109A98] = 0x81   <-- the arm
0x6428a  POP EBX
0x6428b  RET
```

Fresh `get_function_xrefs 0x64270` = **3**: `0x383B3` (`FUN_00038004`), `0x38E81`
(`FUN_00038630`), `0x5112D` (`FUN_000510DC`). Fresh `get_xrefs_to 0x109A98` =
**29** refs (write `0x64284` is the arm; no `MOV dword[0x109A98],0x80` exists —
no 0x80 writer, FU270 confirmed).

The three `FUN_000510DC` callers sit in the **replay handler cluster** `0x512xx..0x513xx`.

### 1.2 The handlers are table entries: `[0x108C7C]`

Fresh `read_memory 0x108C7C` (48 B) — the replay handler table; the raw bytes
(each fnptr followed by zero dwords; **not a uniform stride**: 16 B from
`0x108C7C`→`0x108C8C`→`0x108C9C`, then 8 B to `0x108CA4`):

```
hex: 70 12 05 00 00 00 00 00 00 00 00 00 00 00 00 00
     b0 12 05 00 00 00 00 00 00 00 00 00 00 00 00 00
     68 13 05 00 00 00 00 00 70 13 05 00 00 00 00 00

0x108C7C: 70 12 05 00  -> 0x00051270   FUN_00051270  (replay start)
0x108C80: 00 00 00 00
0x108C84: 00 00 00 00
0x108C88: 00 00 00 00
0x108C8C: b0 12 05 00  -> 0x000512B0   FUN_000512B0  (replay step/pause)
0x108C90: 00 00 00 00
0x108C94: 00 00 00 00
0x108C98: 00 00 00 00
0x108C9C: 68 13 05 00  -> 0x00051368   (JMP FUN_0004FD50 thunk)
0x108CA0: 00 00 00 00
0x108CA4: 70 13 05 00  -> 0x00051370   (second replay-start handler)
0x108CA8: 00 00 00 00
```

So the four fnptrs are `0x51270 @0x108C7C`, `0x512B0 @0x108C8C`,
`0x51368 @0x108C9C`, `0x51370 @0x108CA4` (three zero dwords between the first
three, one before `0x51370`); the surrounding region is the fresh
`read_memory 0x108C3C` (256 B) table block.

The `0x51370` handler is `FUN_000510DC(1)` bound to `[0x109A98]` — fresh
`disassemble_bytes 0x51360`:

```
0x51370  PUSH EDX
0x51371  MOV EDX,EAX
0x51373  CALL 0x0004ff0c                 ; a probe
0x51378  TEST EAX,EAX
0x5137a  JZ 0x000513b1
0x5137c  CALL 0x0008f178
0x51381  TEST EDX,EDX
0x51383  JNZ 0x00051391
0x51385  MOV EAX,0x1
0x5138a  CALL 0x000510dc                 ; <-- the second replay-start arm
0x5138f  JMP 0x000513aa
...
```

`0x51368` (the 5th table entry) is `JMP 0x0004fd50`; `FUN_00051270` is the same
shape (fresh `decompile_function 0x51270`: `FUN_0008f178(); if (EDX==0)
{ x = FUN_000510dc(1,0); ... }`).

Fresh `get_function_xrefs 0x512B0` = **2**: `0x5152C`, `0x5197E` (both inside the
`0x515xx`/`0x519xx` handler cluster).

So the replay arm is reached **only** through these handler functions; the two
start handlers and the step handler are entries in `[0x108C7C]`.

### 1.3 The live caller: the screen-descriptor walk

The handler functions never carry their own arguments — they are installed as
screen-descriptor slots and invoked indirectly. The descriptor is the block at
**`0x14E570`** (fresh `search_instructions operand "14e570"` = 5 sites:
`FUN_000513EC 0x51405`, `FUN_000537F8 0x53814`, `FUN_00053A48 0x53A4E/0x53B6A`,
`FUN_00053BB8 0x53BBE`).

Fresh `disassemble_bytes 0x513C0` — `FUN_000513EC` reads the descriptor and calls
the first installed handler:

```
0x51405  MOV EDX,0x14e570                ; descriptor base
0x5140a  TEST ECX,ECX
0x5140c  JZ 0x00051416
0x5140e  MOV EAX,[0x0014e5ac]            ; arg1 = descriptor+0x3c
0x51413  CALL dword ptr [EDX + 0x38]     ; (*descriptor[0x38])(descriptor[0x3c])  <-- indirect
```

Fresh `decompile_function 0x53BB8` — the descriptor **state-walker** invokes all
four handler slots:

```
void FUN_00053BB8(void):
  puVar5 = 0x14E570;
  ...state machine on *(puVar5+8)... case 1:
     (*[0x14E5A8])([0x14E5AC]);          ; handler1(arg1)
  case 2: (*[0x14E5B0])([0x14E5B4]);     ; handler2(arg2)
  case 3: (*[0x14E5B8])([0x14E5BC]);     ; handler3(arg3)
  case 4: (*[0x14E5C0])([0x14E5C4]);     ; handler4(arg4)
```

Fresh `get_xrefs_to 0x14E5A8` = **5**: READ `0x513F3`/`0x51413` (`FUN_000513EC`),
READ `0x53C29` (`FUN_00053BB8`), WRITE `0x5385A` + DATA `0x53808`
(`FUN_000537F8`, the match reset). Fresh `get_function_xrefs 0x513EC` = **3**
(`FUN_0004B100 0x4B1D2`, `0x86B6E`, `0x87C9D`); fresh `get_function_xrefs
0x53BB8` = **2** (`FUN_000495B0 0x49720` and `0x4976B`).

So the descriptor handler slots (`0x14E5A8`/`0x14E5AC`, `0x14E5B0`/`0x14E5B4`,
`0x14E5B8`/`0x14E5BC`, `0x14E5C0`/`0x14E5C4`) are the dispatch vector; the replay
handlers are copied into them by the mode machine below.

### 1.4 The installer: `FUN_00052030` (mode/screen machine), via `FUN_00053A48`

Fresh `get_function_xrefs 0x52030` = **1**: `FUN_00053A48 0x53A82`. Fresh
`get_function_xrefs 0x53A48` = **2**: `FUN_000493A0 0x49523`, `FUN_00049B28
0x4A045`.

Fresh `decompile_function 0x52030` — the mode switch `switch(uVar3)` with
`uVar3 = FUN_0004B380()`; fresh `decompile_function 0x4B380` = `return
[0x157A4A]>>24;` (the **game mode**). The `case 7` block loads the replay
template `[0x108C7C]`; fresh `disassemble_bytes 0x52870` (the case-7 install
window, exact bytes):

```
0x52878  MOV EAX,0x2
0x5287d  MOV ECX,0x8
0x52882  MOV ESI,0x108c7c                ; <-- the replay template
0x52887  CALL 0x00063654
0x5288c  MOV [0x0014e5ac],EAX            ; arg slot 1
0x52891  MOV [0x0014e5bc],EAX            ; arg slot 3
0x52896  LEA EDI,[EDX + 0x38]            ; descriptor + 0x38
0x52899  MOV [0x0014e5b4],EAX            ; arg slot 2
0x5289e  MOV [0x0014e5c4],EAX            ; arg slot 4
0x528a3  MOVSD.REP  ES:EDI,ESI           ; copy ECX dwords 0x108C7C -> [EDX+0x38]
```

**Narrower proven claim (per review).** The four descriptor arg slots
(`0x14E5AC`/`B4`/`BC`/`C4`) are each written with the `FUN_00063654` **return**
(`0x5288C/0x52891/0x52899/0x5289E`), and the `MOVSD.REP` count is **ECX as left by
`CALL 0x63654`**, not the literal `8` set at `0x5287D`: fresh
`disassemble_bytes 0x63654` shows `FUN_00063654` saves only EBX/EDX (`PUSH
EBX; PUSH EDX`) and does not preserve ECX, so the copy count is not statically
the literal 8 and the installed window is **not proven**. What *is* proven is the
high-level chain: the descriptor handler slot `[0x14E5A8]` is invoked by
`FUN_000513EC`/`FUN_00053BB8`, and the template `[0x108C7C]` (fresh `read_memory
0x108C7C`, §1.2) holds `FUN_00051270` and `FUN_000512B0` — so the descriptor
install lands a **replay-arm handler** (`FUN_00051270` / `FUN_000512B0`) into the
invoked slot. The **per-slot mapping of handlers 1..4** (and whether
`0x51368`/`0x51370` are inside the copy window) is **not proven** — FU278-L7.

`[0x108C7C]` is the replay template; the other table loads are fresh
byte-search-confirmed: `be7c8d1000` (`MOV ESI,0x108d7c`) → `0x529D9`
(case 0x13); `bebc8c1000` (`0x108cbc`) → `0x52573`; `be3c8c1000` (`0x108c3c`) →
`0x521CD`,`0x524A2`,`0x5294B`,`0x52E2C` (+`0x5380F` in the reset).

`FUN_00052030` also invokes the installed handler itself: fresh
`search_instructions` (CALL, `0x38`, program-wide) = 4 sites —
`0x51413`/`FUN_000513EC`, `0x5212E`/`FUN_00052030`, `0x52A7B`, `0x53C29`/
`FUN_00053BB8`. Fresh `disassemble_bytes 0x52100`:

```
0x5210e  MOV [EDX + 0x14],EAX
0x52111  MOV ESI,0x108cdc                ; the [0x108CDC] handler set
0x52116  LEA EDI,[EDX + 0x38]
0x52120  MOVSD.REP  ES:EDI,ESI
0x52122  MOV ECX,[EDX + 0x38]            ; handler1
0x52127  TEST ECX,ECX
0x52129  JZ 0x00052131
0x5212b  MOV EAX,[EDX + 0x3c]            ; arg1
0x5212e  CALL dword ptr [EBX + 0x38]     ; invoke descriptor[0x38]
```

So the machine both installs and invokes the descriptor handler; the replay set
is installed in mode 7 and afterwards invoked by `FUN_000513EC` / `FUN_00053BB8`
on the per-tick path.

### 1.5 The per-tick live origin: `FUN_0004B100`

Fresh `decompile_function 0x4B100` (the per-tick gameplay dispatcher) calls, in
one body:

```
FUN_0004B100(param_1, param_2):
  ...
  uVar6 = FUN_0008af38(...);               ; <-- the match clock (→ FUN_00054104(0))
  if (DAT_00157ac2 < 2) {                  ; period 0/1 phases
    FUN_000513ec();                        ; <-- invoke descriptor handler1
    FUN_00053dc4(); ...
  }
```

Fresh `get_function_xrefs 0x4B100` = **3**: `FUN_000493A0 0x4951E`, `FUN_00049B28
0x49FBB`, `FUN_0004A228 0x4A25A`. Fresh `disassemble_bytes 0x4B190` shows the
clock call inline at `0x4B1A6` (`CALL 0x0008af38`). Fresh `get_function_xrefs
0x8AF38` = **2**: `FUN_0004B100 0x4B1A6`, `0x7440B` (unanalyzed, `0x744xx`
band — fresh `disassemble_bytes 0x743F0`: `0x7440B CALL 0x8af38`).

`FUN_000493A0` is the **match main loop**: fresh `get_function_xrefs 0x493A0` =
**3** (`FUN_00018014 0x180AA`, `FUN_00018108 0x18359`, `FUN_00018680 0x18819` —
the front-end match-entry), and it calls `FUN_0004A228` (setup, `0x49430`),
`FUN_0004B02C` (`0x49415`,`0x4950A`), `FUN_0004B100` (`0x4951E`), `FUN_000495B0`
(`0x494B5`), `FUN_00053A48` (`0x49523`). Fresh `get_function_xrefs 0x495B0` = **1**
(`FUN_000493A0 0x494B5`); it calls `FUN_00049B28` (`0x496D5`) and `FUN_00053BB8`
(`0x49720`/`0x4976B`).

**Step-1 live chain (native):**

```
FUN_000493A0 (match loop; from front-end FUN_00018014/…/FUN_00018680)
  └─ FUN_0004B100 (per-tick dispatcher)
       ├─ FUN_0008AF38 (clock) ──► FUN_00054104(0,…)   [period overlay]
       └─ FUN_000513EC / FUN_00053BB8 ──► (*[0x14E5A8])([0x14E5AC])
  └─ FUN_00053A48 (descriptor state machine)
       └─ FUN_00052030 (mode switch, FUN_0004B380 = [0x157A4A]>>24)
            └─ case 7 loads template [0x108C7C] into descriptor [EDX+0x38]
                 (replay template holds FUN_00051270 / FUN_000512B0 / 0x51368 / 0x51370)
                 → the invoked [0x14E5A8] slot is a replay-arm handler
                     FUN_00051270 / 0x51370 (start) ──► FUN_000510DC(1,0)
                     FUN_000512B0 (step/pause)      ──► FUN_000510DC(0,…)
                                                        └─► FUN_00064270 ──► [0x109A98]=0x81
```

---

## Step 2 — the overlay and the substitution event

### 2.1 `overlay.armed` = `[0x14E674] & 0x8000`

Fresh `get_xrefs_to 0x14E674` = **83** (matches FU270). Fresh arm writers:

```
FUN_00054104 0x54147 OR AH,0x80 / 0x5414C MOV [0x0014e674],EAX   ; id | 0x8000
FUN_000550E4 0x55187 OR AH,0x80 / 0x5518C MOV [0x0014e674],EAX   ; self-arm
FUN_00054238 0x542A2 MOV [0x0014e674],EAX                         ; driver armer
FUN_000537F8 0x5389E MOV dword ptr [0x0014e674],EDI              ; reset (EDI=0)
```

Fresh `disassemble_bytes 0x5413C` (the `FUN_00054104` armer, alias-confirming
`OR AH,0x80` → bit 15):

```
0x54145  MOV EAX,EDX
0x54147  OR AH,0x80
0x5414c  MOV [0x0014e674],EAX
0x54151  LEA EAX,[EDX + -0x5] ; CMP EAX,0xd ; JA 0x541ce
0x54163  JMP dword ptr CS:[EAX*0x4 + 0x540cc]   ; the {5,9,0xB..0x12} second-overlay seed
```

Fresh `disassemble_bytes 0x55170` (the `FUN_000550E4` self-arm, `0x55187 OR
AH,0x80` / `0x5518C MOV [0x0014e674],EAX`); fresh `disassemble_bytes 0x53890`
(the `FUN_000537F8` reset: `0x5389E MOV [0x0014e674],EDI`, `0x538A4
[0x14E678]=0`, `0x538B6 [0x14E684]=0`, `0x538BC [0x14E688]=0`).

**Armer callers, fresh.** `get_function_xrefs 0x54104` = **5**:
`0x8B3C2` (`FUN_0008AF38` — the **match clock**, `FUN_00054104(0,…)` = period
announcement, fresh `decompile_function 0x8AF38` at `LAB_0008b3ae`:
`if (DAT_0015882b._2_1_ != 0 && DAT_0014c32a == 0) FUN_00054104(0,…)`),
`0x3BBC5` (`FUN_0003BB1C`), `0x503B8` (`FUN_0004FF8C` — the **view-mode-0x14**
team/sub substitution screen, fresh `decompile_function 0x4FF8C` at
`LAB_000503ac`: `FUN_00054104(0xb,…)`), `0x93762` (`FUN_000935A0`), and `0x51586`
(the unanalyzed `0x5154C` sibling). `get_function_xrefs 0x54238` = **4**, all
inside the overlay driver `FUN_000515C8` (`0x51694`,`0x51701`,`0x51775`,`0x517D6`).
`get_function_xrefs 0x550E4` = **1** (`FUN_000565BC 0x5664D` — the presenter R3 gate).

### 2.2 The overlay driver `FUN_000515C8` is a descriptor handler

Fresh `disassemble_bytes 0x515A8` — the driver (no Ghidra function; switch on
`[0x108E24]` 0..7, jump table `0x515A8`):

```
0x515c8  PUSH EBX ; ... SUB ESP,0x8
0x515d3  CALL 0x0004937c            ; tick
0x515e0  MOV EAX,[0x00108e24]
0x515eb  CMP EAX,0x7 ; JA 0x00051869
0x515f4  JMP dword ptr CS:[EAX*0x4 + 0x515a8]
```

fresh `read_memory 0x108D7C` (64 B) — the record `{0x51444, 0, 0x515C8, 6}` at
`0x108D7C`: the driver pointer is at **`0x108D84`** (`c8 15 05 00` →
`0x000515C8`, param `6` at `0x108D88`), inside the `PTR_LAB_00108d7c` group
(`0x108D7C`). That group is loaded by `FUN_00052030 case 0x13` (fresh byte search
`be7c8d1000` → `0x529D9`) through the same descriptor loader — so the overlay
driver is installed and invoked by the same mode/screen machine as the replay
handlers.

`get_xrefs_to 0x108E24` (FU270 = 10) is driver-local; the driver's only armer is
`FUN_00054238` (4 call sites inside it). Fresh `disassemble_bytes 0x51830`
(driver tail) confirms the driver clears/INCs `[0x108E24]` and arms via
`FUN_000550E4`-style `[0x14E674]=id|0x8000` paths.

### 2.3 `sub.active` = the `[0x1587D4]` record (FU-152 / FU270 §1.3)

Fresh `get_xrefs_to 0x1587D4` = **14**; the single WRITE from `FUN_0008A938` is
`0x8AE54`. Fresh `decompile_function 0x8A938` — the substitution arm is case 6/8:

```
case 6:  (&DAT_001587da)[(k)*2 + side] = 1;      ; per-side strip flags
             (&DAT_001587d8)[side] += 1;
case 8:  if ([0x157A49]>>24 != 1) {
             [0x1587D4] = (&DAT_001587cc)[idx*4];  ; <-- the record pointer (0x8AE54)
             [0x1587E8]=0; [0x1587D9]=0; [0x1587D8]=0;
             for (i<6) for (s<2) (&DAT_001587da)[i*2+s] = 0; }
```

Fresh `get_function_xrefs 0x8A938` = **39** callers (FU270 census re-confirmed
exactly). Of these, **10** sites carry a `from_function` (the analyzed
live-driving callers): `FUN_0004B02C` (`0x4B0A5`,`0x4B0D9`), `FUN_00088860`
(`0x888C8`,`0x888F2`), `FUN_00088940` (`0x88B44`,`0x88BBD`,`0x88C00`),
`FUN_0008A43C` (`0x8A729`), `FUN_00038630` (`0x38B2E`,`0x38C95`); the **remaining
29** are the unanalyzed gameplay-event band (`0x742DE`,`0x74312`, `0x7546E`…
`0x77707`, `0x7DF90`, `0x84495`,`0x84E8F`, `0x85D0A`,`0x85D38`, `0x863F9`,
`0x8920A`,`0x897E3`,`0x8A3F0`,`0x8A8CE`,`0x8B85D`), of which the
user-substitution cluster `0x93C41…0x947AB` holds **9** sites.

The **match-driver** live caller is `FUN_0004B02C` (fresh
`get_function_xrefs 0x4B02C` = **2**, both `FUN_000493A0` `0x49415`/`0x4950A`);
fresh `decompile_function 0x4B02C` fires the substitution record at period end:

```
FUN_0004B02C(param_1):
  if (param_1 == 1) FUN_0008a938(0,0,0);
  ...
  if (((DAT_0014c1d6 | DAT_0014c1d4) & 0x100) != 0) {
    FUN_0008a938(8,0,0);      ; <-- the record write, idx=0
    DAT_00157ac2 = 4;
  }
```

(`FUN_0004B100` reads the record back at `0x4B1E5`/`0x4B265`: `if (… ||
DAT_001587d4 == 0) … else DAT_001074a4 = *(byte*)(*DAT_001587d4 + 0x826)`, so the
record is consumed live.) The **user-substitution** driver is the unanalyzed
`0x93Cxx..0x947xx` band (10 call sites) — the team-management screen path.

### 2.4 `ball_row` = `[0x14E660]`

Fresh `get_xrefs_to 0x14E660` = **3**:

| site | type | body |
|---|---|---|
| `0x53901` | WRITE | `FUN_000537F8` reset (`[0x14E660] = 0`) |
| `0x5667C` | READ | `FUN_000565BC` (the R5 gate) |
| `0x56690` | WRITE | `FUN_00056690` (`[0x14E660] = p`) |

Fresh `get_function_xrefs 0x56690` = **1**: `FUN_00056CF4 0x56E41`. Fresh
`disassemble_bytes 0x56DE0`:

```
0x56e1b  XOR EDX,EDX                 ; EDX := 0
0x56e1d  MOV byte ptr [EAX],0x0
0x56e20  XOR EAX,EAX
0x56e22  MOV ESI,[0x001543b0]
0x56e28  LEA EDI,[EAX + 0x14eaf0]
0x56e2e  ADD EAX,0xc
0x56e31  MOVSD ES:EDI,ESI            ; copy loop (never writes EDX)
0x56e32  MOVSD ES:EDI,ESI
0x56e33  MOVSD ES:EDI,ESI
0x56e34  CMP EAX,0x3c ; JNZ 0x56e22
0x56e39  MOV EAX,EDX
0x56e3b  MOV dword ptr [0x00109004],EDX
0x56e41  CALL 0x00056690             ; FUN_00056690(0)  <-- always EDX = 0
```

The `MOVSD` loop never writes `EDX`, so `FUN_00056690` is **always called with 0**;
`[0x14E660]` is zero in the image (fresh `read_memory 0x14E670` = all zero) and
`FUN_00056518` never draws. **Dormancy re-confirmed fresh.**

---

## Step 3 — the live reach and the ruling

### 3.1 Native reachability (the producers exist and are live-reachable)

| row / cell | live native path (fresh) | trigger context |
|---|---|---|
| `replay.state` `[0x109A98]` | `FUN_0004B100` → (`FUN_000513EC`/`FUN_00053BB8` → `(*[0x14E5A8])`) → `FUN_00051270`/`0x51370`/`FUN_000512B0` → `FUN_000510DC` → `FUN_00064270` → `0x81`; installed by `FUN_00052030 case 7` via `FUN_00053A48` | the replay **screen/mode 7** (entered from `FUN_000493A0`) |
| `sub.active` `[0x1587D4]` | `FUN_0004B02C` (from `FUN_000493A0`) → `FUN_0008A938(8,…)`; the `0x93Cxx..0x947xx` substitution screen; the gameplay-event band (39 callers) | period-end / substitution event |
| `overlay.armed` `[0x14E674]&0x8000` | `FUN_0004B100` → `FUN_0008AF38` (clock) → `FUN_00054104(0)`, and `FUN_0004FF8C` (view 0x14) → `FUN_00054104(0xB)`; driver `FUN_000515C8` (installed by `FUN_00052030 case 0x13`) → `FUN_00054238`; `FUN_000550E4` self-arm (R3) | period announcement / team screen / extra-time |
| `ball_row` `[0x14E660]` | **none** — `FUN_00056690`'s only caller passes 0 | dormant |

The single unified **period/screen machine** is: match loop `FUN_000493A0` →
per-tick `FUN_0004B100` → clock `FUN_0008AF38` + descriptor invoker
`FUN_000513EC`/`FUN_00053BB8`; and `FUN_000493A0` → `FUN_00053A48` →
`FUN_00052030` (mode machine) which installs the handler sets `[0x108C7C]`
(replay) and `[0x108D7C]` (overlay driver) into the descriptor `0x14E570`.

### 3.2 Port reachability = zero (the ruling input)

`grep` of `src/` + `include/` (read-only): `fifa96_match_run_replay_step`
(`run.c:4624`), `_overlay_arm` (`:4756`), `_overlay_timeout_step` (`:4785`), the
`_sub_*` setters, and `fifa96_match_run_ball_row_reachable` are referenced **only
from `tests/`** — no engine caller. `fifa96_match_run_replay_step`'s only
cross-calls are internal (`run.c:4638/4657/4662`) plus the render consumer
(`:4860`); `fifa96_camera_replay_select` (`fifa96_camera.c:535`) is reached only
from `_replay_camera_set` (`run.c:4608`). `render.replay.state` is reset to
`FIFA96_MATCH_REPLAY_LIVE` at init (`run.c:4620`) and otherwise untouched;
`render.sub.active` / `render.overlay.armed` / `render.overlay.id` are never set
outside tests. The M2 tape pins `replay.state=0`, `sub.active=0`,
`overlay.armed=0`, `overlay.id=0`, `ball_row==NULL`.

### 3.3 The live-vs-leg ruling

**LEG (zero-gated; the live native caller is unported)** for `replay.state` /
`sub.active` / `overlay.armed`; **pinned dormant** for `ball_row`. The
period/screen machine above (`FUN_0004B100` → `FUN_000513EC`/`FUN_00053BB8` →
`FUN_00051270`; `FUN_0008AF38` → `FUN_00054104(0)`; `FUN_0004B02C` →
`FUN_0008A938(8)`) is entirely in the unported match-menu / overlay / event
cluster, so on the port **no ported producer raises the gates** — a claim that any
of the three rows is live on the port is a fixture-only stand-in. This confirms
`docs/ENGINE.md` ("replay / overlay / sub strip (live) | blocked | zero-gated rows
with no producers yet").

**The live caller, in one line (native):** `FUN_0004B100` (per-tick dispatcher,
called from the match loop `FUN_000493A0`) → `FUN_000513EC`/`FUN_00053BB8`
(screen-descriptor handler invoker) → the installed `[0x14E5A8]` =
`FUN_00051270`/`0x51370` → `FUN_000510DC(1,…)` → `FUN_00064270` → `[0x109A98]=0x81` —
with `FUN_00052030` (mode machine, via `FUN_00053A48`) as the installer. For the
**port**, this caller does not exist: **none — leg**.

---

## Step 4 — the port shapes and the hand-off

* **`replay.state`.** Port the arm entry: the `FUN_000510DC` non-zero branch
  (`[0x14E58C]=1; FUN_00064270(); FUN_00037114(4); [0x108E0C]=1`) and its three
  handler callers (`FUN_00051270`, `0x51370`, `FUN_000512B0`). The missing live
  piece is the **screen-descriptor install/invoke**: `FUN_00052030 case 7`
  (install `[0x108C7C]` into `0x14E570+0x38`) plus `FUN_000513EC`/`FUN_00053BB8`
  (invoke `(*[0x14E5A8])([0x14E5AC])`), driven per-tick from `FUN_0004B100`.
* **`sub.active`.** Port the substitution-event writer `FUN_0008A938` case 6/8
  (the `[0x1587D4]`/`[0x1587DA]` record seed) as the setter of
  `run.render.sub.active` + block fields, and its live caller `FUN_0004B02C`
  (`FUN_0008A938(8,0,0)` at period end). The R1 gate is already in
  `match_run_draw_sub_strip` (`run.c:4860-4861`).
* **`overlay.armed`.** Port the armer `FUN_00054104` (`id | 0x8000` + the
  `{5,9,0xB..0x12}` second-overlay seed) and its live callers: the clock
  `FUN_0008AF38` (id 0), the view-mode-0x14 handler `FUN_0004FF8C` (id 0xB), and
  the overlay driver `FUN_000515C8` (ids 5/9/0xD/0xF via `FUN_00054238`), gated by
  the `[0x108E14]>/[0x108E24]` cooldown and the `FUN_00045069` probe.
* **`ball_row`.** No producer to port (always-0 writer). Keep the NULL seam.

---

## Numbered open legs (FU278-L*)

* **FU278-L1 — the mode-7 entry (replay-screen trigger).** The replay handler set
  `[0x108C7C]` is installed only by `FUN_00052030 case 7` (mode
  `[0x157A4A]>>24 == 7`); I have not statically proven that mode 7 is entered from
  open-play (the replay-key/screen transition) — that transition is the true live
  trigger. Native owners: `FUN_00052030` (`0x52882`), `FUN_00053A48` (`0x53A82`).
* **FU278-L2 — the port's per-tick dispatcher has no counterpart.** `FUN_0004B100`
  (per-tick, calls both `FUN_0008AF38` and `FUN_000513EC`) and the descriptor
  walkers `FUN_000513EC`/`FUN_00053BB8` are unported. Native owners: those bodies.
* **FU278-L3 — the substitution-event live caller.** 39 callers of
  `FUN_0008A938`; only `FUN_0004B02C` (match driver) is analyzed-live; the
  user-substitution screen is the unanalyzed `0x93Cxx..0x947xx` band (9 calls).
  Native owner: `FUN_0008A938` (`0x8AE54`) / that band.
* **FU278-L4 — the overlay driver `FUN_000515C8`.** Table-dispatched
  (`[0x108E24]` 0..7, jump table `0x515A8`, pointer slot `0x108D84` + flag 6),
  and its `0x5154C` sibling has no static xref (FU270-L4). Native owner: the driver.
* **FU278-L5 — `ball_row` resurrection.** Dormancy proven for static writers only;
  a runtime/computed writer into `0x14E6xx` would resurrect the row. Native owner:
  `0x14E660`.
* **FU278-L6 — the 0x80 replay state has no native writer** (arm goes straight to
  0x81; the port's `0x80→ARMED` promotion is native-dead). Native owner: the
  29-site `[0x109A98]` writer set.
* **FU278-L7 — the descriptor install window / per-slot mapping.** `FUN_00052030
  case 7` loads template `[0x108C7C]` and `MOVSD.REP`-copies to `[EDX+0x38]`
  with count = ECX as left by `CALL 0x63654` (not the literal `8`), so the exact
  copy window and the handler1..4 ↔ `0x51270`/`0x512B0`/`0x51368`/`0x51370`
  mapping are unproven. Native owner: `FUN_00052030` (`0x52870`..`0x528A3`).

---

## Evidence table (this session)

| claim | evidence (fresh) |
|---|---|
| `FUN_000510DC` callers = 3 | `get_function_xrefs 0x510DC` |
| the arm = `FUN_00064270` → `[0x109A98]=0x81` | `disassemble_function 0x64270` |
| `FUN_00064270` callers = 3 | `get_function_xrefs 0x64270` |
| `[0x109A98]` census = 29 | `get_xrefs_to 0x109A98` |
| replay handler template `[0x108C7C]` fnptrs at `{0x51270@0x108C7C, 0x512B0@0x108C8C, 0x51368@0x108C9C, 0x51370@0x108CA4}` | `read_memory 0x108C7C` (48 B), `read_memory 0x108C3C` (256 B) |
| `0x51370` → `FUN_000510DC(1)` | `disassemble_bytes 0x51360` |
| `FUN_000512B0` callers = 2 | `get_function_xrefs 0x512B0` |
| descriptor base = `0x14E570`; handler invoke `CALL [EDX+0x38]` | `search_instructions 14e570`; `disassemble_bytes 0x513C0`; `decompile_function 0x53BB8` |
| `FUN_000513EC`/`FUN_00053BB8` callers | `get_function_xrefs 0x513EC` (3); `get_function_xrefs 0x53BB8` (2) |
| installer `FUN_00052030` case 7 loads `[0x108C7C]` (template) + writes arg slots with the `FUN_00063654` return; its only caller | `decompile_function 0x52030`; `disassemble_bytes 0x52870`; `disassemble_bytes 0x63654`; `get_function_xrefs 0x52030` (1) |
| mode = `[0x157A4A]>>24` | `decompile_function 0x4B380` |
| `FUN_00052030 case 0x13` loads `[0x108D7C]`; invoke at `0x5212E` | `search_byte_patterns be7c8d1000` → `0x529D9`; `disassemble_bytes 0x52100` |
| per-tick `FUN_0004B100` → `FUN_0008AF38`/`FUN_000513EC` | `decompile_function 0x4B100`; `disassemble_bytes 0x4B190` |
| `FUN_0004B100` callers = 3; `FUN_00053A48` callers = 2; `FUN_000495B0` caller = 1 | `get_function_xrefs 0x4B100`/`0x53A48`/`0x495B0` |
| `FUN_0008AF38` (clock) → `FUN_00054104(0)` | `decompile_function 0x8AF38`; `get_function_xrefs 0x54104` (5) |
| `FUN_0004FF8C` (view 0x14) → `FUN_00054104(0xB)` | `decompile_function 0x4FF8C` |
| overlay arm/reset writers | `disassemble_bytes 0x5413C`/`0x55170`/`0x53890`; `get_xrefs_to 0x14E674` (83) |
| `FUN_00054238` callers = 4 (driver); `FUN_000550E4` caller = 1 | `get_function_xrefs 0x54238`/`0x550E4` |
| driver slot `0x515C8` at `0x108D84` | `read_memory 0x108D40` |
| sub writer `FUN_0008A938` case 6/8; 39 callers | `decompile_function 0x8A938`; `get_function_xrefs 0x8A938`; `get_xrefs_to 0x1587D4` (14) |
| `FUN_0004B02C` → `FUN_0008A938(8,0,0)` | `decompile_function 0x4B02C`; `get_function_xrefs 0x4B02C` (2) |
| reset `FUN_000537F8` caller = 1 (init) | `get_function_xrefs 0x537F8` |
| `ball_row` census = 3; dormant (writer always 0) | `get_xrefs_to 0x14E660`; `get_function_xrefs 0x56690`; `disassemble_bytes 0x56DE0`; `read_memory 0x14E670` |
| descriptor writes/handlers `[0x14E5A8]` | `get_xrefs_to 0x14E5A8` (5); `get_xrefs_to 0x14E570` (10) |

Port cites (read-only): `src/fifa96_engine/fifa96_match_run.c`
(4620/4624/4638/4608/4657/4662/4756/4785/4860), `include/fifa96_engine/fifa96_match_run.h`,
`src/fifa96_loader/fifa96_camera.c:535`, `tests/test_engine_m2.c` (2451-2454 /
2853-2856), `tests/test_engine_match_render.c`. Carried docs:
`docs/ghidra/FU270_replay_overlay.md`, `docs/ghidra/FU152_presentation_residual.md`,
`docs/ENGINE.md`.

## Provenance (Ghidra MCP, read-only, `/FIFA96.EXE`)

`decompile_function` 0x510DC, 0x51270, 0x512B0, 0x64270, 0x52030, 0x53A48,
0x53BB8, 0x513EC, 0x4B380, 0x4B5F4, 0x4B100, 0x4B02C, 0x8AF38, 0x4FF8C, 0x8A938,
0x63654;
`disassemble_bytes` 0x51360, 0x513C0, 0x515A8, 0x51830, 0x51940, 0x514C0, 0x52100,
0x520D8, 0x527C0, 0x52870, 0x63654, 0x5413C, 0x55170, 0x53890, 0x56DE0, 0x4B190,
0x743F0, 0x64270;
`read_memory` 0x108C3C (256), 0x108C7C (48 + 64), 0x108D40 (256), 0x108D7C (64),
0x14E670 (32);
`get_xrefs_to` 0x109A98 (29), 0x14E674 (83), 0x14E660 (3), 0x1587D4 (14),
0x14E5A8 (5), 0x14E570 (10), 0x108C7C (4), 0x108CDC (2);
`get_function_xrefs` 0x510DC (3), 0x64270 (3), 0x51270 (6), 0x512B0 (2),
0x513EC (3), 0x53BB8 (2), 0x52030 (1), 0x53A48 (2), 0x495B0 (1), 0x4B100 (3),
0x4B380 (12), 0x4B02C (2), 0x54104 (5), 0x54238 (4), 0x550E4 (1), 0x537F8 (1),
0x8A938 (39), 0x56690 (1), 0x8AF38 (2), 0x493A0 (3), 0x4A228 (1);
`search_instructions` (CALL, `0x38`, program-wide → 4; operand `14e570` → 5);
`search_byte_patterns` `be7c8d1000`→0x529D9, `bebc8c1000`→0x52573,
`be3c8c1000`→5 sites, `c0140500`→none, `40190500`→none;
`get_function_by_address` 0x514FF/0x51880/0x5194F/0x514C0/0x51460/0x51B44/0x93C41/
0x7546E/0x8A3F0/0x52A7B/0x7440B = "No function", 0x51AB8 = thunk.
No Ghidra writes, no project saves, no repo edits; this draft and its report are
the only files written. No git command.
