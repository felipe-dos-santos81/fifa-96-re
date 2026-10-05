# FU-65: competition screens (states 7–11) — setters, handlers and the site-7 static re-check

Roadmap slice #4 of FU-58 §7 ("Competition screens (states 7..11)") plus a
static re-check of FU-17's parked site-7 story with the state map in hand.
Derives the four setter sites' enclosing functions and caller chain, decodes
the five state handlers 7–11 from the `0x43DC` targets, and corrects two
FU-17 readings of the site-7 gates. Ports the clean state-gate/selector logic
as `fifa96_competition_gate`.

Result in one line: **the state 6/7/8/9 setters live in `FUN_0002C94C` and
the state 10/11 setters in `FUN_0002D684`, reachable only through the
competition module driver `FUN_00028960` (case `0x28A78` → `FUN_0002C380` →
`FUN_0002C94C`; case `0x28A89` → `FUN_0002D684`), itself a
case (`0x1892E`) of the boot tail dispatcher after `FUN_00018680`; the state 7
handler is a composite of three widget setups, 8/9 share one setup pair
(`EAX=0x65,EDX=0x83`), 10/11 share two (`0x5F,0x0E` then `0,0x7D`) followed
by the shared event pump at `0x14B56`; and the site-7 gates compare the last
input action code (`FUN_00016350`), not a mode/state — `FUN_00024B00` fires
site 7 on action `10` or `11` then dispatches state 16, the `0x26AA8` cluster
fires it on action `10` or `11` (0x27685/0x27693/0x27B1A) then state 18 when
`[0x5094]==0`, so FU-17's "state parameter `EBX∈{10,11}`" and "`ESI==10`"
readings are errata and the mouse-gated claim is an input-binding question
that remains runtime-blocked.**

## Method

* Static work on the open Ghidra MCP session for `/fifa96_le.bin` (flat LE
  link addresses, FU-4/FU-58..FU-64).
* The setter and state-handler regions are Ghidra analysis gaps
  (`find_code_gaps`: `0x14451..0x14B55` size 1797 "orphaned instructions";
  no function contains `0x2CFAB`/`0x2D067`/`0x2D955`/`0x2DA0C`). The
  authoritative byte source is the flat LE image **rebuilt from the ISO's
  `FIFA96.EXE`** with `python3 tools/fifa96_le.py /tmp/opencode/fu58/FIFA96.EXE
  -o /tmp/opencode/fu65/fifa96_le.bin` (md5 of source `9a461768d610121c2bb5869a7c4cfc9d`,
  same as FU-58/FU-64; 1,485,392-byte image). Regions were decoded with
  `ndisasm -b32 -o<addr>` and cross-checked against Ghidra `read_memory`
  (identical bytes at every quoted address); the rebuilt image also matches
  the program at `0x1442C`/`0x14648`/`0x2CFAB`.
* `disassemble_bytes` was **not** used for the gap regions: it lists
  overlapping pre-existing instructions and mis-decodes some windows
  (e.g. `0x14719` shows a phantom `TEST AL,1`; the true stream at `0x14718`
  is `PUSH 0x1A8; PUSH 0x280; ...`). All handler/setter bytes below are from
  the raw image. Errata in §7.
* Direct-call censuses used `python3 tools/fifa96_callers.py <addr>` (E8 rel32
  over the image rebuilt from the ISO, FU-64's method) and Ghidra
  `get_function_xrefs` where a function exists.

## 1. Setter functions and the caller chain

The four FU-58 anchors are the tail `CALL 0x1442C` instructions of two
functions; the state is in `EAX` at the call:

| anchor | enclosing function | dispatch | gate |
|---|---|---|---|
| `0x2CFC0` | `FUN_0002C94C` (0x2C94C) | state 8 or 9 | `0x2CFAB CMP [0x49FBC],1` → 8 (0x2CFB4), else 9 (0x2CFBB) |
| `0x2D083` | `FUN_0002C94C` | state 7/8/9 | `0x2D067 CMP ESI,2` → 7; `TEST ESI,ESI` → 8; else 9; reached only when `[0x5094]==0` (`0x2D057`) |
| `0x2D972` | `FUN_0002D684` (0x2D684) | state 10 | unconditional after VIV `0x14C90` + helpers (`0x2D955`/`0x2D95A`) |
| `0x2DA16` | `FUN_0002D684` | state 11 | `0x2D9F5 MOV EBX,[0x5094]` / `0x2D9FA TEST EBX` — skip (cleanup `FUN_0001A280`) when set; else VIV `0x14BB0` then 11 |

### 1.1 `FUN_0002C94C` — states 6, 8, 9, 7

Entry/prologue byte-verified: `PUSH EBX/ECX/EDX/ESI/EDI/EBP; SUB ESP,0x64` at
`0x2C94C..0x2C952`, `MOV EDI,EAX` (face selector, 0..2) at `0x2C955`;
epilogue `ADD ESP,0x64; POP EBP/EDI/ESI/EDX/ECX/EBX; RET` at
`0x2D08A..0x2D093`. (Ghidra's `FUN_0002c94c` body ends at `0x2C9DD` and its
decompile prunes the rest — errata §7.) Body evidence, byte-verified:

```
0x2CF0C  MOV EAX,[0x49FBC] / XOR EDX / CALL 0x16A3C
0x2CF1D  MOV EDX,[0x47DC0] / MOV EAX,[0x47C30] / CALL 0x14C90   ; VIV load
0x2CF2D  CMP dword [0x49FAC],0 / JZ 0x2CF48
0x2CF36  XOR EBP,EBP / MOV EAX,6 / MOV [0x49FAC],EBP / JMP 0x2CFC0
0x2CF48  ...draw setup...  0x2CFAB CMP [0x49FBC],1 / JNZ -> EAX=9
0x2CFC0  CALL 0x1442C                     ; state 8 (EAX=8) or 9
0x2CFC5  PUSH 1 / ... / CALL [0x12AFC]    ; post-screen redraw
0x2CFEA  CALL 0x65920 / 0x125C4 / 0x16350 ; input loop until ESI >= 0
0x2CFFB  CMP ESI,-10 -> FUN_0001771C(0x1C2,1); FUN_0001D26C; if 1: ESI=2,[0x5094]=1
0x2D02F  JL 0x2CFEA
0x2D033  CALL 0x16C10 / 0x2D03E MOV [0x49FBC],EAX   ; store last input code
0x2D043  VIV [0x47C30]/[0x47DC0] / CALL 0x14BB0 / 0x138BC / 0x138F4
0x2D057  CMP [0x5094],0 / JZ 0x2D067; else FUN_0001A280 / JMP 0x2D088
0x2D067  CMP ESI,2 -> EAX=7 / TEST ESI,ESI -> EAX=8 / else EAX=9
0x2D083  CALL 0x1442C / 0x2D088 MOV EAX,ESI / epilogue
```

So one call can dispatch up to two states: first the *initial* screen
(6 one-shot, else 8 when the **previous** input was 1, else 9), then after the
keyboard/event loop the *next* screen (7 when input 2, 8 when input 0, else 9)
unless `[0x5094]` is set, in which case no state is dispatched and the
function returns the last input code in `EAX`.

The one-shot: `[0x49FAC]` is written 1 only by `FUN_0002C380` (`0x2C392`) and
cleared at `0x2CF3D`; `[0x49FBC]` is written 1 by `0x2C398` and with the input
code by `0x2D03E` (`get_xrefs_to 0x49FAC` = {0x2C392 W, 0x2CEC2 R, 0x2CF2D R,
0x2CF3D W}; `0x49FBC` = {0x2C398 W, 0x2CF0C R, 0x2CFAB R, 0x2D03E W}).

### 1.2 `FUN_0002C380` — the face/team cycler that calls it

`0x2C380..0x2C419` (prologue `PUSH EBX/ECX/EDX/ESI`; no frame): sets
`[0x49FAC]=1` (`0x2C392`) and `[0x49FBC]=1` (`0x2C398`), calls
`FUN_0002C41C` (`0x2C39E`), then loops calling `FUN_0002C94C` with `EAX=EDX`
cycling 0..2: `EDX+1` with wrap at `0x2C3B7..0x2C3C1`, `EDX-1` with wrap at
`0x2C3CA..0x2C3D6`, exit when the call returns 2 (`ECX=1` at `0x2C3DF`); then
three `FUN_00018F04` sampler calls on `[0x49FB0]/[0x49FB4]/[0x49FB8]`
(`0x2C3E8..0x2C410`). Its only direct caller is `0x28A78` (ret `0x28A7D`),
and `FUN_0002C94C` has exactly the two callers `0x2C3C1`/`0x2C3D6`.

### 1.3 `FUN_0002D684` — states 10 and 11

Entry `0x2D684` (`PUSH EBX/ECX/EDX/ESI/EDI/EBP`), frame `SUB ESP,0x64` at
`0x2D68A`, epilogue `0x2DA1B..0x2DA24`. Body evidence:

```
0x2D955  CALL 0x2D1BC / 0x2D95A XOR EAX / CALL 0x2D2F8
0x2D961  XOR EDX / MOV EAX,[0x47C30] / CALL 0x14C90      ; VIV still
0x2D96D  MOV EAX,0xA / 0x2D972 CALL 0x1442C              ; state 10
0x2D977  CALL [0x12AFC] / 0x2D97D CMP ESI,2 / JZ 0x2D9D6
0x2D982  loop: FUN_00065920 / 0x125C4 / 0x16350->ECX
0x2D993  if EAX==3: CALL 0x2D604
0x2D99D  if ECX==-10: FUN_0001771C; FUN_0001D26C; if 1: ECX=2,[0x5094]=1
0x2D9D1  CMP ECX,2 / JNZ 0x2D982
0x2D9D6  [0x5868]=FUN_00018F04([0x5868]); [0x49FA4]=FUN_00018F04([0x49FA4])
0x2D9F5  MOV [0x49FA4],EAX / MOV EBX,[0x5094] / 0x2D9FA TEST EBX
         nonzero: FUN_0001A280 / 0x2DA03 JMP 0x2DA1B
0x2DA05  VIV [0x47C30]/[0x47DC0] / CALL 0x14BB0
0x2DA11  MOV EAX,0xB / 0x2DA16 CALL 0x1442C               ; state 11
```

Only direct caller: `0x28A89` (ret `0x28A8E`). The loop exits on input code 2
(or was skipped when `ESI==2` on entry) — the state-10 screen's own exit
condition; `[0x5094]` again suppresses the state-11 dispatch.

### 1.4 `FUN_00028960` — the competition module driver

`0x28960` (`PUSH EBX/ECX/EDX/ESI/EDI/EBP`), dispatches on `EAX` through
`JMP [cs:eax*4+0x18940]` (`0x289B4`, table runtime-populated by LE fixups,
FU-58's limitation) for `EAX<=7`. Decoded case blocks:

| case | code | action |
|---|---|---|
| `0x28A78` | `CALL 0x2C380` | the state 6/7/8/9 setter wrapper (face loop), returns `EAX=3,ESI=1` |
| `0x28A89` | `CALL 0x2D684` | the state 10/11 setter, returns `EAX=3,ESI=2` |
| `0x28A32` | `FUN_0001B1C8(9,0x64)`; `[0x5858]==0 → FUN_00028200([0x49FE4])`; `FUN_00029F68(ESI)` | |
| `0x28A62` | `FUN_0002BC94([0x5858])` | |

`FUN_00028960` has exactly one direct caller: `0x1892E` (ret `0x18933`).
That site is a case block of the tail dispatcher after `FUN_00018680`
(Ghidra body ends `0x188C0`): the first table `JMP [eax*4+0x8644]`
(`0x188C8`) has case blocks including `0x188CF CALL 0x1FC50`, `0x188D9
CALL 0x1DD40` (the FU-58 "caller unknown" outer driver), and `0x188E3`; the
latter begins a second sub-table `JMP [cs:edx*4+0x8670]` (`0x188F7`,
`EDX=EAX-1`, 4 cases: `0x188FF FUN_00024B00`, `0x18908`/`0x18911
FUN_00026CC0`, `0x1891A FUN_00018014`), and further case blocks follow at
`0x18927 FUN_000206FC`, `0x1892E FUN_00028960`, `0x18935 FUN_0002F9DC`,
`0x1893C FUN_00031AA4`, `0x18943 FUN_00032DE0`, `0x1894E/0x18953/0x1895A
FUN_00017F80/FUN_00018108/FUN_00017DF0`.

So the states 6–11 setters are reached from the competition module, which is
itself a boot-flags dispatcher case above `FUN_00018680` (whose own call chain
from `FUN_000B26B1` is FU-64 §1.3). Two FU-58 open legs fall out of this:
the `0x1DD40` driver's caller is the `0x188D9` table case (still no static
table decode), and the `0x8644` table's case set is now enumerated but its
entries remain fixup-populated.

## 2. State handlers 7–11 (from the FU-58 `0x43DC` decode)

Dispatcher `FUN_0001442C` (byte-verified): `PUSH EBX/ECX/EDX/ESI/EDI/EBP`,
`MOV ESI,EAX`, `CALL 0x18BA8`, `MOV EBP,1`, `MOV EDI,EAX`,
`CMP ESI,0x13`, `JA 0x14B56`, `JMP [cs:esi*4+0x43DC]` (`0x14449`). The
handlers are tail continuations of this frame and end with the shared
`POP`/`RET` at `0x14B8F..0x14B93`. Addresses and bytes below are from the
rebuilt image (each target independently decodes as a coherent setup block,
re-verifying FU-58's fixup mapping):

| state | entry | parameter blocks (EAX,EDX) | notes |
|---:|---|---|---|
| 7 | `0x14648` | `(0x1E4,0x1B7)` → CALL 0x13BD8; `(0,0x7D)` → CALL; `(0x65,0x83)` → JMP tail | composite of three setups |
| 8 | `0x146DC` | `(0x65,0x83)` → JMP `0x14B4F` | |
| 9 | `0x1470E` | `(0x65,0x83)`, `ECX=0x1A8,EBX=0x280` → JMP `0x14B51` | EBX differs (0x280 vs -1) |
| 10 | `0x14742` | `(0x5F,0x0E)` → CALL; `(0,0x7D)` → JMP `0x14B4B` | |
| 11 | `0x14798` | `(0x5F,0x0E)` → CALL; `(0,0x7D)` → JMP `0x14B4B` | |

Shared tail, byte-verified: `0x14B4B PUSH 0; 0x14B4D XOR EAX,EAX;
0x14B4F MOV EBX,ECX; 0x14B51 CALL 0x13BD8; 0x14B56 MOV EAX,2; CALL 0x659F8;
0x14B60 MOV EAX,0x1C; CALL 0x65CC0; TEST EBP,EBP; JZ skip; loop
{FUN_00013F10; FUN_00018B18} until EDX==0; MOV [0x46560],0; MOV [0x4F34],0;
CALL 0x9A16C; POP EBP/EDI/ESI/EDX/ECX/EBX; RET` (`0x14B41..0x14B93`).

`FUN_00013BD8` (decompiled) stores its EAX input and EDX, EBX, ECX and the
stack argument block into the view/widget record globals at the
`0xB7EB7xx` runtime addresses, plus time fields from `FUN_000CB2A4` and
`param_3`/`param_9` scaling. So each handler draws/sets up widget records;
the pair in parentheses is the `(EAX,EDX)` input pair and is the only
per-state discriminator recovered. **What the pair means in asset terms and
the on-screen identity of each state is not evidenced here** (open legs).

Two groupings are evidenced by the constants: states 8 and 9 share exactly
one `(0x65,0x83)` block; states 10 and 11 share exactly the two blocks
`(0x5F,0x0E)` and `(0,0x7D)`; state 7 is a three-block composite containing
the 8/9 block. This corroborates FU-16's classifier grouping ({8,9} vs
{10,11}, §3) without by itself naming screens.

Transitions observed statically (setter logic, §1): 6/8/9 → (input)
7/8/9 through `FUN_0002C94C`, 10 → 11 through `FUN_0002D684`, with
`[0x5094]` suppressing the second dispatch in both.

## 3. Classifier `0x26B45` re-verified (context, not changed)

Byte-verified this slice: `0x26B45 LEA EAX,[EAX+0]; PUSH ECX; PUSH EDX;
CMP EAX,8 / JZ; CMP EAX,9 / JNZ; MOV [0x5524],1 : 0; CMP EAX,0xA / JZ;
CMP EAX,0xB / JNZ; MOV [0x5528],1 : 0; CALL 0x26AA8; POP EDX; POP ECX; RET`.
It has **no static caller** (raw `E8` scan target 0x26B45 = 0; Ghidra xrefs
none), so the mode→`[0x5524]`/`[0x5528]` dispatch is runtime-only; its
consumer is the `0x26AA8` render path (FU-17 §Static notes, not re-derived).
The `0x43DC` table and this entry are the two runtime-populated dispatch
points of the competition family.

## 4. Site-7 status update (static re-check of FU-17)

`FUN_00014C18` (decompile): allocates/fetches the `0x280x0x1E0` buffer at
`[0x4F34]` via `FUN_0009A45C`, `FUN_000CE6F0`, then `decode_record_strict`
of the EAX argument and `FUN_0009AE70` of the EDX argument. Exactly two
static callers (raw scan, matching FU-14/FU-16): `0x25D64` (ret `0x25D69`)
and `0x27B42` (ret `0x27B47`).

### 4.1 `0x25D4F` gate — `FUN_00024B00`

`FUN_00024B00` (entry `0x24B00`, `PUSH`×6 + `SUB ESP,0x20`; callers
`0x18901`, `0x2098B`, `0x32DF8`) is a menu/match state machine. Its local
`[esp+0x14]` is the **last menu input action**: initialised `-1` (`0x24B22`),
written from `FUN_00016350`'s return at `0x2571C`, assigned `0xC` on the
confirm paths (`0x25805`/`0x25957`/`0x25A50`), and `-0xC` by the
`0x25772..0x257CF` sequence. A byte scan of the whole function finds **no**
assignment of `0xA`/`0xB`. The gate:

```
0x25D4B  MOV EBX,[esp+0x14]
0x25D4F  CMP EBX,0xA / JZ 0x25D59
0x25D54  CMP EBX,0xB / JNZ 0x25D7D
0x25D59  VIV [0x47C30]/[0x47DC0] / 0x25D64 CALL 0x14C18
0x25D69  MOV EAX,0x10 / 0x25D70 CALL 0x1442C   ; state 16
0x25D75  MOV [0x5484],0
0x25D7D  ESI=1 / FUN_0001A280 / [0x5484]=1     ; no blit, no state
```

So it fires when the incoming action code is 10 or 11 (from the input
dispatcher, keyboard or any bound device), then dispatches **state 16**.
FU-17's phrasing "the state parameter `EBX` is 10 or 11" is an **errata**:
`EBX` is not a mode/state parameter, it is the last resolved input action.

### 4.2 `0x27B1A` gate — the `0x26AA8` competition cluster

Inside the multi-entry function cluster `0x26AA8..0x27B87` (shared epilogue
`0x27B87`; the cluster also contains the `0x26B45` classifier and calls
`FUN_00016350` once at `0x276A6`), site 7 is reached from the input-event
jump path:

```
0x27685  CMP ESI,0xA / JZ 0x27B23
0x27693  CMP ESI,0xB / JZ 0x27B23       ; loop-continuation check
0x27B1A  CMP ESI,0xA / JNZ 0x27693      ; FU-58's quoted site
0x27B23  FUN_000125C4 / FUN_00015F78 / FUN_00012728 / FUN_00018B18
0x27B37  VIV [0x47C30]/[0x47DC0] / 0x27B42 CALL 0x14C18
0x27B47  CMP [0x5094],0 / JNZ: FUN_0001A280; else 0x27B57 MOV EAX,0x12
0x27B5C  CALL 0x1442C                   ; state 18
```

`ESI` is set from `FUN_00016350` at `0x276AB` and forced to `0xB` on the
confirm path (`0x276E0`, after `FUN_0001D26C`). The gates therefore accept
action codes **10 or 11** — FU-58/FU-17 quoted only `ESI==10` at `0x27B1A`,
missing the `0x27693` catch of 11 — and dispatch **state 18** when
`[0x5094]==0`.

### 4.3 What the state map resolves, and what stays parked

* Resolved: both site-7 callers live in the competition/menu module now
  mapped in §1 (states 6–11); the gates are input-action gates; the states
  they dispatch after the blit are 16 (`0x25D69`) and 18 (`0x27B57`). The
  mouse statement in FU-17 is an input-binding campaign question, not a
  `0x43DC`-state question: the gates read `FUN_00016350` action codes, and
  which keyboard/mouse binding produces 10/11 on those screens is not
  decidable statically here.
* Still runtime-blocked: the 7–11 handlers contain no edge to
  `FUN_00024B00` or the `0x26AA8` cluster (they only build widget records via
  `FUN_00013BD8` and pump events, §2), so nothing in this mapping proves any
  keyboard route reaches the site-7 gates. The parked status stands:
  keyboard-exhausted (FU-14/15/16/17/18, zero site-7 frames), no
  `caller_link` claimed, and no new runtime run was made this slice.

## 5. Port: `fifa96_competition_gate`

`include/fifa96_loader/fifa96_competition_gate.h` +
`src/fifa96_loader/fifa96_competition_gate.c` (caller-owned struct, no
globals, no comments, `-fifa96_err_t`; `FIFA96_ERR_INVALID` for NULL). Scope:
the clean, testable selector logic of §1.1/§1.3.

| original | port |
|---|---|
| `[0x49FAC]` (`0x2C392` set, `0x2CF2D` read, `0x2CF3D` clear) | `one_shot` |
| `0x2CF36..0x2CF43` EAX=6 dispatch | `fifa96_competition_gate_initial` → `FIFA96_COMPETITION_STATE_6` |
| `[0x49FBC]` (`0x2C398`/`0x2D03E` write, `0x2CFAB` read) | `last_input` (written by `after_input`) |
| `0x2CFAB` `[0x49FBC]==1` → 8 else 9 | `initial` else-branch |
| `0x2D03E` store, `0x2D057` confirm, `0x2D067` 2/0/other | `after_input` (returns 0 when `confirm`) |
| `0x2D9FA` confirm, `0x2DA11` state 11 | `after_state10` |
| state-10 dispatch (`0x2D96D`) | caller-owned `FIFA96_COMPETITION_STATE_10` |
| `0x2D9D6` `FUN_00018F04` sampler writes, redraw/geometry calls, VIV loads, input loops, `FUN_0001A280` cleanup | not ported (app-side/event-pump domain) |

Tests (`tests/test_competition_gate.c`, suite 53 → **54**): init zeroing;
one-shot state 6 consumes the flag; last-input 1→8/2→9/0xFFFFFFFF→9;
input mapping 2→7/0→8/1/−1/INT32_MIN→9 with `last_input` write-back;
confirm suppresses the post-input dispatch while still storing the input;
`after_state10` 0→11 and confirm→suppressed; confirm does not suppress the
initial dispatch; an end-to-end sequence mirrors §1.1 (6 → input 2 → 7 →
initial 9 → input 0 → 8 → input 1 → 9 → 11); NULL handling for every entry
point. ASan+UBSan build of `test_competition_gate` clean
(`cc -fsanitize=address,undefined -Iinclude tests/test_competition_gate.c
src/fifa96_loader/fifa96_competition_gate.c`). `make test`: 53/53 before,
**54/54 after**.

## 6. Roadmap item status (FU-58 §7 slice 4)

* Setters `0x2CFAB`/`0x2D067`/`0x2D96D`/`0x2DA11`: derived with enclosing
  functions, caller chain and gates (§1). Deliverable "state→screen identity
  for 7..11 with VIV ids" is **partial**: widget parameter pairs and the
  {8,9}/{10,11} grouping are evidenced; asset/screen names are not.
* `[0x49FBC]` semantics: the last input action code (written 1 by the face
  cycler, otherwise by the input loop), selecting 8 vs 9 on re-entry.
* `[0x5524]`/`[0x5528]` semantics: classifier flags for modes {8,9}/{10,11},
  consumer path set up at `0x26AA8` (FU-17 evidence); dispatch runtime-only
  (§3).
* Site-7 gates: re-derived and corrected (§4); story remains parked as a
  runtime input-binding question.

## 7. Errata (quoted)

* FU-17 §Static notes: "`0x25D4F–0x25D64` calls `0x14c18` only when the
  **state parameter** `EBX` is 10 or 11" — **corrected**: `EBX` is
  `FUN_00024B00`'s last input action code from `FUN_00016350` (`0x2571C`);
  the only non-input assignments to that local are `0xC`/`-0xC` (§4.1).
* FU-17 §Static notes and FU-58 §4: "`0x27B1A–0x27B42` calls it only when
  `ESI==10`" — **extended**: the same site accepts `11` through
  `0x27693 CMP ESI,0xB / JZ 0x27B23` (and `0x27685` from the event table),
  with `ESI=0xB` being forced by the confirm path at `0x276E0` (§4.2).
* FU-58 §4 setter table lists only the 8/9 branch at `0x2CFAB`; the same
  function first dispatches **state 6** when `[0x49FAC]!=0`
  (`0x2CF36..0x2CF43`) — an unlisted setter.
* FU-58 §2/§7 call the handler region undefined; it is an "orphaned
  instructions" gap in `find_code_gaps` (`0x14451..0x14B55`, 1797 bytes) and
  `disassemble_bytes` mis-decodes windows inside it (e.g. a phantom
  `TEST AL,1` at `0x14719`); raw-image decoding is required (Method).
* Ghidra function boundaries are truncated for the two setter functions:
  `FUN_0002c94c` `body_end=0x2C9DD` (true end `0x2D093`) and `FUN_0002D684`
  is not a function; decompiles prune most setter code.
* FU-64 §1.4 open leg "the target table is not decoded" — **partially
  closed**: the case blocks of the `0x8644`/`0x8670` dispatchers are
  enumerated (§1.4), including `0x188D9 CALL 0x1DD40`, closing FU-58's
  "driver caller unknown" to the extent that the `0x1DD40` call site is
  identified (the table entries themselves remain fixup-populated).

## 8. Open legs

* Screen/asset identity for states 6–11: the `(EAX,EDX)` widget constants and
  the `FUN_00013BD8` argument blocks are recorded but not bound to VIV assets
  or screen names; `FUN_00013BD8`'s record layout is not decomposed.
* `0x26B45`'s dispatch pointer (LE fixup stream) is still unparsed; the
  classifier has no static caller.
* `FUN_00024B00`'s internal state machine: the `0x25772..0x257CF` sequence
  (`FUN_000CBB38(0x1D/0x38/0x52)` plus `[0x5470]==0xB`, `[0x5474]==5`,
  `[0x5478]==4`, `[0x547C]==9` → `[0x5270]=1`) is not identified; only the
  gate-relevant locals are derived.
* `FUN_00028960`'s jump table `0x18940`, the boot tables `0x8644`/`0x8670`
  and the `0x16C68` event table are fixup-populated; case sets are partial.
* `FUN_0002D684`'s head (`0x2D684..0x2D955`) and `FUN_0002C94C`'s head UI
  setup (`0x2C955..0x2CE00`, many `FUN_00015960` sprite setups) are not
  itemised.
* The "competition match start is mouse-gated" question needs a runtime
  campaign that delivers action codes 10/11 on the schedule/tournament
  screens; no new probe was run this slice (deliverable is static + port).

## Provenance

Ghidra MCP on `/fifa96_le.bin`:
`decompile_function` 0x13BD8, 0x14C18, 0x24B00, 0x2C94C, 0x183A0, 0x16350;
`read_memory` 0x14648 (512 B), 0x14B40, 0x26AA8, 0x26B45, 0x24B00, 0x2CF90,
0x2D050, 0x2D930, 0x2D9F0;
`get_function_by_address` 0x14648/0x14742/0x2CFAB/0x2D067/0x2D955/0x2DA0C
(all undefined), 0x2C94C, 0x2D684 (undefined), 0x24B00, 0x26AA8, 0x18680,
0x183A0, 0x18901/0x4AEA4-style gaps; `get_function_xrefs` 0x2C94C, 0x24B00;
`get_xrefs_to` 0x49FAC, 0x49FBC; `find_code_gaps` (top 100);
`disassemble_bytes` 0x2C380, 0x1442C (contrast).
Off-Ghidra (analysis-only, `/tmp/opencode/fu65/`): `tools/fifa96_le.py` from
`/tmp/opencode/fu58/FIFA96.EXE` (md5 `9a461768d610121c2bb5869a7c4cfc9d`) and
`ndisasm -b32` regions 0x1442C, 0x14648–0x14B56, 0x256D0–0x25A80,
0x275C0–0x27740, 0x188C0–0x18960, 0x26B45–0x26BE0, 0x2C94C–0x2D0C0,
0x2D684–0x2DA80, 0x28960–0x28AB0; `tools/fifa96_callers.py` for 0x2C94C,
0x2D684, 0x14C18, 0x2C380, 0x28960, 0x24B00, 0x26AA8, 0x26B45, 0x2D604,
0x183A0; byte scans for `[esp+0x14]` assignments and CALL targets.

Analysis-only: no tool, capture-rig, ISO or Ghidra-project change staged.
Port write set: `include/fifa96_loader/fifa96_competition_gate.h`,
`src/fifa96_loader/fifa96_competition_gate.c`,
`tests/test_competition_gate.c`, `CMakeLists.txt` (one library/test block).
`make test`: 53/53 before, **54/54 after**; ASan+UBSan
`test_competition_gate` clean. `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
