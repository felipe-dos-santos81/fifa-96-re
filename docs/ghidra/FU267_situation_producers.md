# W26-4 / FU-267 — the situation-0xB producers (OL-84) + row-01's sinks

Provenance: M2 phase-26 wave W26-4, 2026-10-14, **read-only recon**. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `/FIFA96.EXE`,
`watcom:LE:32:default`, 2706 functions, 15239 symbols, `.image` overlay present,
min address `0x10000`, max `.image::0x174a2a`). Every address/byte below is a
fresh `disassemble_bytes` / `read_memory` / `disassemble_function` /
`get_xrefs_to` / `get_function_by_address` / `read_memory` of **this** session
unless marked *cross-cited* (then the frozen FU doc is named). LE-image globals
sit at **`+0x100000`** from the file offset; the addresses are `/FIFA96.EXE`
runtime (program) addresses. Alias traps applied: `dword[x]>>16` = the word at
`x+2`; `>>24` = the byte at `x+3`; CALL bytes verified; sign/width and
inverted-gate checks done. Engine/loader source read first-hand
(`src/fifa96_engine/fifa96_match_run.c`, `fifa96_match_handlers.c`,
`src/fifa96_loader/fifa96_action_handlers.c`, `include/fifa96_engine/…`).
**No writes**: no rename/comment/label/function/script/project save;
`/FIFA96.EXE` untouched; the ISO never touched; no git commands. This is one new
draft file (plus the run report).

Scope (brief): the **remaining situation-0xB producers** of `OL-84` (row 02,
rows `0x10..0x13`, the keeper/restart bodies, the computed act-8 call
`0x8A8CE`/`0x8B85D`) plus **row-01's camera/event/ball-stage sinks**. Extends
FU-143 §10/§11 (`OL-84a…h`), FU-196 §2/§3.3/§5, FU-202, FU-212, FU-241. This
slice does **not** re-derive the taker-arm producers (W26-3 / FU-266 owns FU-149
L13) nor the keeper stage ladders (W26-6 / FU-269 owns the `1D`/`1E` bodies):
here the keeper/restart bodies are read **only for their situation-0xB route**.

---

## 0. Fresh anchors — the situation→act map

Fresh `read_memory 0x8A904` (52 B, 13 dwords) — the table-2 jump (index =
situation):

```
[0]=0x8AB82 [1]=0x8ABAB [2]=0x8ABDB [3]=0x8ABF3 [4]=0x8ADC0 [5]=0x8ADD8
[6]=0x8AC28 [7]=0x8ADF0 [8]=0x8AE08 [9]=0x8AEC6 [0xA]=0x8AEDE [0xB]=0x8AEF6
[0xC]=0x8AF0E
```

Fresh `disassemble_bytes 0x8AEE0` (48 B) — the **situation-0xB arm**:

```
0x8AEF6 MOV EDX,[ESP-2]        ; the side word (the dispatcher's [ESP]=DX)
0x8AEFA MOV EAX,2
0x8AEFF SAR EDX,0x10
0x8AF02 CALL 0x8A938+0xFE1C    ; FUN_000740A0(2, side)  -> phase 2
```

`FUN_000740A0` is the phase setter (**fresh `get_xrefs_to 0x740A0` = 27** callers:
the dispatcher arms `0x8AA89`/`0x8AB9F`/`0x8ABE7`/`0x8AC1C`/`0x8ADB4`/`0x8ADCC`/
`0x8ADDC`/`0x8ADFC`/`0x8AF02`, `FUN_0008B9CC 0x8BAC1`, `FUN_0008BAF0 0x8BCCF`, the
front-end family, screens). So **every situation-0xB producer's only effect on
the phase cell is this one `FUN_000740A0(2, side)` call → phase 2** (FU-196 §2.2:
"phase 2 = the sole phase-2 writer; per-record phase-2 handler `0x6DCC8`").

Fresh `disassemble_bytes 0x8A938` (240 B) — the dispatcher head routes to the
direct path `0x8AA7B` when `AX == 0`, `EAX == 0xB`, `[0x14C32A] == 0`, or
`[0x15B6C0] != 0`:

```
0x8A944 TEST AX,AX / 0x8A947 JZ 0x8AA7B        ; situation 0
0x8A94E CMP EAX,0xB / 0x8A951 JZ 0x8AA7B       ; situation 0xB
0x8A957 CMP byte [0x14C32A],0 / 0x8A95E JZ 0x8AA7B   ; session gate closed
0x8A964 CMP dword [0x15B6C0],0 / 0x8A96B JNZ 0x8AA7B  ; a situation pending
```

and fresh `disassemble_bytes 0x8AA70` (64 B) shows the `BX==0` branch into the
table-2 route:

```
0x8AA7B TEST BX,BX / 0x8AA7E JZ 0x8AAA8         ; BX==0 -> table-2
0x8AAA8 CMP CX,0x1 / 0x8AAAC JC 0x8AB63         ; CX=0 -> table-2 route
```

So a sit-`0xB`, `BX=0` call resolves `0x8AB63 → … → table2[0xB] = 0x8AEF6 →
phase 2`. `ECX` set by a caller is **clobbered** at `0x8A93E MOV ECX,EAX` before
the route test, so the `ECX=1` at the row-1E site (`0x75B53`) does **not** change
the sit-0xB route.

---

## Step 1 — The situation machine: the remaining situation-0xB producers

### 1(a) The native producers (fresh bytes)

**Fresh `get_xrefs_to 0x8A938` = 39** — byte-identical to FU-196 §3.3 / FU-202
§1.2 / FU-143 §10. The **situation-0xB (`EAX=0xB`, `BX=0`) row tails** are eight
sites; all are `EAX=0xB`, `EDX = rec team side`, `EBX=0`:

| # | site | row (fresh `0x1106E0[code]`) | fresh bytes |
|---|---|---|---|
| 1 | `0x7DF90` | row **01** (`0x7DBC0`), stage-2 tail | `0x7DF83 MOV EAX,0xB` / `0x7DF88 AND EDX,0xFF` / `0x7DF8E XOR EBX,EBX` / `0x7DF90 CALL 0x8A938` (`e8 a3 c9 00 00`) |
| 2 | `0x85D38` | row **0x10** (`0x855F0`) | `0x85D2B MOV EAX,0xB` / `0x85D30 AND EDX,0xFF` / `0x85D36 XOR EBX,EBX` / `0x85D38 CALL` (`e8 fb 4b 00 00`); prefixed `0x85D19 MOV word[0x157A6A],0x12C` |
| 3 | `0x863F9` | row **0x11** (`0x85DE4`) | `0x863EC MOV EAX,0xB` / `0x863F1 AND EDX,0xFF` / `0x863F7 XOR EBX,EBX` / `0x863F9 CALL` (`e8 3a 45 00 00`) |
| 4 | `0x84495` | row **0x12** (`0x83D68`) | `0x84488 MOV EAX,0xB` / `0x8448D AND EDX,0xFF` / `0x84493 XOR EBX,EBX` / `0x84495 CALL` (`e8 9e 64 00 00`); prefixed `0x8447B CALL 0x4C374` (the `word[0x157A68]` clear) |
| 5 | `0x84E8F` | row **0x13** (`0x84B00`) | gated `0x84E70 CMP byte[0x158746],0 / 0x84E77 JZ 0x84EC5`; `0x84E82 MOV EAX,0xB` / `0x84E87 AND EDX,0xFF` / `0x84E8D XOR EBX,EBX` / `0x84E8F CALL` (`e8 a4 5a 00 00`) |
| 6 | `0x7546E` | row **0x1D** (`0x74EB0`, keeper close-down) | `0x75461 MOV EAX,0xB` / `0x75466 AND EDX,0xFF` / `0x7546C XOR EBX,EBX` / `0x7546E CALL` (`e8 c5 54 01 00`) |
| 7 | `0x75B58` | row **0x1E** (`0x7550C`, keeper claim) | `0x75B46 MOV EAX,0xB` / `0x75B4B AND EDX,0xFF` / `0x75B51 XOR EBX,EBX` / `0x75B53 MOV ECX,1` / `0x75B58 CALL` (`e8 db 4d 01 00`) |
| 8 | `0x76072` | row **0x1E** (`0x7550C`, close-down tail) | `0x76063 MOV EAX,0xB` / `0x76068 AND EDX,0xFF` / `0x7606E XOR EBX,EBX` / `0x76070 XOR CL,CL` / `0x76072 CALL` (`e8 c1 48 01 00`) |

The row→body map is fresh-read this session from the **action table**
`read_memory 0x1106E0` (180 B, 45 dwords): row `0x10 = 0x855F0`, `0x11 = 0x85DE4`,
`0x12 = 0x83D68`, `0x13 = 0x84B00`, `0x1D = 0x74EB0`, `0x1E = 0x7550C`,
`0x1F = 0x76380` (so `0x76072` sits inside row `0x1E`, not `0x1F`).

**Post-call tails (fresh):** row 0x11 sets `0x86403 MOV byte[EAX+0x9A],1`; row
0x10 sets `0x85D3D CALL 0x4C380` (`FUN_0004C380` = a bare `RET` — a no-op); row
0x13 and the keeper 1D/1E tails likewise call `0x4C380`; row 0x12 sets
`0x844A4 MOV byte[0x157AB0],BL(0)`; keeper 1D sets `0x75475 MOV byte[0x157AB2],0`;
keeper 1E (`0x75B5D..0x75B67`) installs action `5` (`EDX=5; EAX=[EBP-0xC];
CALL 0x7D9A4`); `0x76077 MOV byte[0x157AB2],CL(0)`.

### 1(b) Fresh reachability census

Full fresh classification of the **39** `0x8A938` callers by `(situation, BX)`
(the "only/sole" claims below each rest on this single fresh `get_xrefs_to`):

| sit | BX | sites | class |
|---|---|---|---|
| 0 | 0 | `0x4B0A5` (`FUN_0004b02c` mode 1), `0x38B2E`/`0x38C95` (front-end `FUN_00038630`) | match-init / front-end |
| 0 | caller EBX | `0x742DE` (`FUN_000742xx` match-init twin) | match-init |
| 0xB | 0 | **`0x7DF90`, `0x85D38`, `0x863F9`, `0x84495`, `0x84E8F`, `0x7546E`, `0x75B58`, `0x76072`** | **the OL-84 row tails (this slice)** |
| 1 | 0 | `0x888F2` (`FUN_00088860` period-end), `0x8B85D` (act-0xA terminal) | kickoff re-dispatch |
| 2 | 1 | `0x85D0A` (row 0x10), `0x88C00`/`0x88B44`/`0x88BBD` (`FUN_00088940` throw-in) | queue arms |
| 5 | 1 | `0x76B1C` (row 1A), `0x77707` (row 1B) | queue arms |
| 7 | 0 | `0x76A90` (row 1A), `0x77589` (row 1B) | queue arms |
| 8 | 0 | `0x4B0D9` (`FUN_0004b02c` extra-time gate), `0x888C8`, `0x74312` (twin) | extra-time |
| 9 | 1 | `0x8920A` (`FUN_000891xx`) | act-2 queue |
| 0xA | 1 | `0x8A3F0` (`FUN_0008A3xx`, side^1) | act-2 queue |
| 4 | 1 | `0x897E3` (`FUN_000897xx`) | act-4 stored re-dispatch |
| **stored** | 0 | `0x8A8CE` (act-8 terminal) | terminal re-dispatch |
| screens | — | `0x93C41`, `0x93CE4`, `0x93EC1`, `0x93F75`, `0x94120`, `0x94332`, `0x943D7`, `0x945A5`, `0x947AB` | FU-146 goal/celebration |
| other | — | `0x8A729` (`FUN_0008a43c`) | phase driver |

The **`0x8A904` table-2 sit-0xB arm `0x8AEF6` is the sole phase-2 writer** (fresh
§0). No `0x8A938` caller lies in the row-02 body (`0x7DFCC..0x7E1A3`) — see 1(d).

### 1(c) Live-vs-leg ruling

| producer | engine today | live trigger | ruling |
|---|---|---|---|
| **row 01 `0x7DF90`** | `handlers.c:276` `fifa96_match_run_situation(mr, 0x0B)` | row 01 is installed by the derived kickoff entry; stage 0 (`global_5882a`) then stage 1 needs the native kick-press slot arm (`word[slot+6] & 0x70`), and stage 2 times out at `+0x89 >= 0x3C` | **LIVE** — headless proof `tests/test_engine_match_frame.c:860 test_kickoff_enters_phase2_when_cutscene_isolated`, which supplies the synthetic `FIFA96_ENGINE_KEY_KICK` press/release (`:911`) the native kickoff waits for; on the M2 tape the KICK burst reaches phase 2 naturally (ENGINE.md known-gaps: "KICK burst → clock `00:02`") |
| **row 0x11 `0x863F9`** | `match_sp_resolve` → `fifa96_match_run_situation(mr, 0x0B)` (the taker tail) | **live-on-tape** — the derived FU-170 restart wire (SETUP0 sit-0xC → act-9 → SETUP2 sit-3 → the corner arm → phase 2) dispatches the corner taker row 0x11 on the M2 tape | **LIVE (Layer 1, tape/granted-frame)** — `tests/test_engine_m2.c:1195` "the first tape dispatch of the corner taker row 0x11", `:1902 M2_RESTART_ROWS_MASK (1ull << 0x11)`, `:5087 assert(res.mask_final == (M2_WIRED_MASK \| M2_RESTART_ROWS_MASK))`; `docs/ENGINE.md:1753` "row 0x11 now dispatches on the tape" (its tail resolves back to phase 2) |
| **rows 0x10 / 0x12 / 0x13** (`0x85D38`/`0x84495`/`0x84E8F`) | `match_sp_resolve` → `fifa96_match_run_situation(mr, 0x0B)` (`handlers.c:1556` + the shared taker tail) | **none live** — the taker arm for these rows (the setup/probe/vector/RNG) is unported (FU-149 L13; W26-3/FU-266) | **LEG** — ported bodies, dispatch-dead on the tape |
| **keeper 1D/1E** (`0x7546E`/`0x75B58`/`0x76072`) | `fifa96_match_action_1D`/`_1E` → `out->situation_0b` → `fifa96_match_run_situation(0xB)` (`handlers.c:532/631`) | **none live** — the keeper stage ladder producers (W26-6/FU-269, FU-151) are the gap | **LEG** — ported bodies, producer-dormant |
| row 0x10's sibling `0x85D0A` (sit 2, BX=1) | `set_piece(bx=1)` re-queue | none live (the throw-in/ref arm) | **LEG** (FU-149) |

**Net:** of the eight situation-0xB producers, **two are live** — row 01
(`0x7DF90`, the derived kickoff route) and row 0x11 (`0x863F9`, the derived FU-170
restart wire, tape-proven); the other **six** (rows 0x10/0x12/0x13 and keeper
1D/1E) are ported-but-dormant bodies whose live arm belongs to W26-3 (takers) /
W26-6 (keeper). No new row is wired by this ruling; the census does not move.

### 1(d) Correction — row 02 is NOT a situation-0xB producer

`OL-84d` (FU-143 §11.5) lists "rows 02/0x10..0x13" among the unported
situation-0xB producers. Fresh evidence says **row 02 is not a `FUN_0008A938`
caller**:

* the fresh 39-site `get_xrefs_to 0x8A938` contains **no** address in row 02's
  `0x7DFCC..0x7E1A3` range (fresh `read_memory 0x1106E0` gives the action table
  `0x1106E0[3] = 0x7E1A4`, so row 02 is `0x7DFCC..0x7E1A3`; the nearest
  `0x8A938` caller is row 01's `0x7DF90`);
* fresh `disassemble_bytes 0x7DFCC` (120 B) shows row 02's head gates on
  `[0x157A4A]>>24 == 1` (phase 1) / `== 2` (phase 2) and, in phase 2 with
  `[+0x20] != 0`, installs action **4** invoke-now (`0x7E051 MOV EDX,4; 0x7E05A
  CALL 0x7D9A4`, i.e. `FUN_0007D9A4(rec, code=4, …)`).

So row 02 is the **phase-2 *consumer*** that hands the resolved restart record to
the row-04 locomotion machine — it is downstream of the sit-0xB → phase-2 write,
not a producer of it. This narrows `OL-84d` to "rows `0x10..0x13`, the
keeper/restart bodies `0x7546E`/`0x75B58`/`0x76072`, and the computed act-8
`0x8A8CE`".

### 1(e) What each producer un-blocks

Each producer emits `0x8A938(0xB, side, BX=0)` → table-2 `0x8AEF6` →
`FUN_000740A0(2, side)`. That write un-blocks the **in-play phase 2** — the sole
phase-2 writer (fresh §0): the per-record phase-2 handler `0x6DCC8` (phase-table
`0x110794[2]`), the `FUN_0008D098` per-team arm driven with the new phase byte
(`0x740C8/0x740DB`), and the record-action machinery (rows 01/02/04) that then
runs the kickoff/locomotion. The row-01 producer additionally clears the
restart's record staging; the keeper rows return the keeper to open play.

---

## Step 2 — The computed act-8 call (`0x8A8CE`/`0x8B85D`) + its live producer

### 2(a) The native producers (fresh bytes)

**`0x8A8CE` — the act-8 stage-2 terminal re-dispatch (computed stored
situation).** Fresh `disassemble_bytes 0x8A8A0` (80 B) — the act-8 body
`FUN_0008A798` (`0x110794[0x1E]`, fresh `get_xrefs_to 0x8A798` = **1**, DATA from
`0x11080C`):

```
0x8A8B1 MOV byte[0x158829],AH(0)     ; stage clear (low byte)
0x8A8B7 MOV byte[0x158828],AH(0)     ; act id clear (low byte)
0x8A8BD MOV EDX,[0x158829] ; 0x8A8C8 SAR EDX,0x18   ; >>24 = byte 0x15882C = SIDE
0x8A8C3 MOV EAX,[0x158828] ; 0x8A8CB SAR EAX,0x18   ; >>24 = byte 0x15882B = SITUATION
0x8A8CE CALL 0x8A938                                 ; (sit, side), BX=0
0x8A8D3 MOV byte[0x15882B],0xFF
```

This is the **computed** (stored) situation: the re-dispatch argument is
`byte[0x15882B]`, not an immediate. `0x8A8CE`'s producer is the
`FUN_0008A938` **BX!=0 fallback** — fresh `disassemble_bytes 0x8AA70`:
`0x8AA98 MOV EAX,8` (act 8), `0x8AA9D MOV byte[0x15882B],CL` (store situation),
`0x8AAA3 JMP 0x8AF1A` → fresh `get_xrefs_to 0x8AF1A` = **1**
(`UNCONDITIONAL_JUMP` from `0x8AAA3`) → `CALL 0x888FC` (the act invoker, fresh
`get_xrefs_to 0x888FC` = **11**, the same 11 as FU-196 §2.2). So act 8 is
dispatched **invoke-now**; its stage-2 terminal fires ~22 granted frames later
and re-dispatches the stored situation.

**`0x8B85D` — the act-0xA (phase-0x20) stage-2 terminal re-dispatch (immediate
situation 1).** Fresh `disassemble_bytes 0x8B840` (96 B):

```
0x8B840 MOV [0x158825],AL ; 0x8B845 MOV [0x157AAE],AL ; 0x8B84A MOV [0x15882C],AL
0x8B84F MOV EDX,[0x157AAB]           ; the side dword
0x8B855 MOV EAX,1                     ; situation 1
0x8B85A SAR EDX,0x18
0x8B85D CALL 0x8A938                  ; (1, side), BX=0
0x8B864 CALL 0x36200
```

Unlike `0x8A8CE`, this terminal re-dispatches **situation 1** (the kickoff) by
immediate. The two are the sibling **terminal re-dispatches** the brief groups:
`0x8A8CE` = computed stored sit, `0x8B85D` = immediate sit 1.

**Engine mapping (landed).** The native act-8 route is now faithfully modelled:
`fifa96_match_run.c:3144-3157` writes phase 0 then dispatches act 8 invoke-now
(the `0x8AAA3 JMP 0x8AF1A CALL 0x888FC(8,0,BX=1)` mirror), so the engine's
`fifa96_match_action_phase_1E` sweep reaches its own `0x8A8CE` stage-2 terminal
and defers the stored-situation re-dispatch there. The phase-`0x1E` slot is
registered (`handlers.c:5629`); see §2(c)/the erratum.

### 2(b) Fresh reachability census

* **`0x8A8CE`**: the act-8 slot is the phase-table `0x110794[0x1E]` and the
  act-table `0x1107EC[8]` (dual identity, FU-165-L9). Its **only** dispatch route
  is the global act slot / the `0x888FC` invoker; there is **no direct call site**
  (fresh xrefs = 1 DATA). Entry to act 8 requires a `FUN_0008A938` call that
  reaches the `BX!=0` fallback — i.e. a sit-0/0xB (or closed-gate/pending) call
  with `BX != 0`.
* **`0x8B85D`**: within the act-0xA / phase-0x20 body (`0x8B688`, no
  auto-created function — table-dispatched); reached by the clock-tail act tick
  when act 0xA / phase 0x20 is resident.

### 2(c) Live-vs-leg ruling (the de-compression has LANDED)

| terminal | engine today | ruling |
|---|---|---|
| **`0x8A8CE`** | the engine **now de-compresses** the act-8 route (`src/fifa96_engine/fifa96_match_run.c:3144-3157`): the `set_piece` `bx != 0` branch writes phase 0 (`match_run_write_phase(mr, 0u)`), stores `store_15882c/b`, then `return fifa96_match_act_dispatch(mr, 8, 0, 1);` — the native `0x8AAA3 JMP 0x8AF1A CALL 0x888FC(8,0,BX=1)` invoke-now. The phase-`0x1E` slot is **registered/counted**: `fifa96_match_handlers.c:5629 {0x1E, fifa96_match_action_phase_1E, "FU-241 §6.3 (M2 phase-23 T4): … is now COUNTED (+1)"}`; phase `0x1E` is in the **59/80** wired set (`docs/ENGINE.md:8`). `0x8A8CE` = the sweep's **stage-2 terminal**, and the former in-frame re-dispatch + immediate `[0x15882B]=0xFF` are **deferred ~22 granted frames** to it | **LIVE (Layer 1, engine-reach)** — the act-8 sweep is dispatched invoke-now on the live `bx!=0` route; it is render-invisible (stores a pointer + clears `word[0x157A68]`, rewrites no record) and **dormant on M2** (no `state=3`; golden byte-unchanged). Re-scoped residual: FU267-L2 |
| **`0x8B85D`** | `fifa96_match_action_phase_20` is **registered/counted** (FU-188 §2/§6 (phase-16 T4) + FU-202 §6 rec (b) (M2 phase-18 T3)); its stage-2 terminal re-dispatches situation 1 (`handlers.c` phase-0x17 comment: "native `0x8B85D` … the producer is LIVE") | **LIVE** — the phase-0x20 terminal fires on the live kickoff route (act 1 → phase 0x17), proven headlessly (FU-193/FU-203). This is the **sit-1** producer, not 0xB. |

**Net:** **both** terminal re-dispatches are live — `0x8A8CE` (the act-8 sweep's
stage-2 terminal, live on the `bx!=0` route after the FU-241 §6.3 de-compression)
and `0x8B85D` (the act-0xA terminal, sit 1 → the kickoff). The `0x8A8CE` sweep is
render-invisible and tape-dormant, so no golden/tape change and no census move.
The earlier "engine compresses / phase-`0x1E` NULL" premise (FU-241 §5.2) is
**superseded** — see §2d erratum below.

### 2(d) erratum — the de-compression is landed (fix round 1)

> The original §2(c) (and `§4.1` item 4 / `FU267-L2`) asserted that the engine
> **compresses** the act-8 route and that "the phase-`0x1E` slot stays NULL
> (`handlers.c` `{0x1E, NULL, …}`, FU-241 §5.2)". That premise is **stale**: the
> de-compression **landed** in M2 phase-23 T4 (FU-241 §6.3, `55/80 → 56/80`). Fresh
> first-hand engine reads this fix round:
> `src/fifa96_engine/fifa96_match_handlers.c:5629` = `{0x1E, fifa96_match_action_phase_1E,
> "FU-241 §6.3 (M2 phase-23 T4): … is now COUNTED (+1) …"}`, and
> `src/fifa96_engine/fifa96_match_run.c:3144-3157` = the `bx != 0` branch:
> `match_run_write_phase(mr, 0u)` then `return fifa96_match_act_dispatch(mr, 8, 0, 1);`,
> with the comment "The engine now de-compresses: … The former in-frame re-dispatch
> and immediate `[0x15882B]=0xFF` are therefore deferred to the sweep terminal."
> `docs/ENGINE.md:8` lists phase `0x1E` in the **59/80** wired rows. So `0x8A8CE`
> is live (engine-reach) and the FU267-L2 residual is re-scoped to the non-tape
> equivalence + act-slot residency legs (FU241-L1/L3), not "do not fake a producer".

---

## Step 3 — Row-01's camera / event / ball-stage sinks

### 3(a) The native sinks (fresh bytes)

Row 01 (`0x7DBC0..0x7DFC8`) — fresh `disassemble_bytes 0x7DBB0` (176 B) and
`0x7DEE0` (192 B):

* **stage-0 camera sink** — `0x7DC01 CALL 0x700F4` with the constant triple
  `[0x10F328]/[0x10F32C]/[0x10F330]` (first-hand FU-143 §11.2 reads the triple as
  `(0,0,0)`) pushed as `FUN_000700F4(ECX=0, EBX=0, EDX=0, 0)`. Fresh
  `get_xrefs_to 0x700F4` = **29** callers (`0x7DC01` is row 01's).
* **stage-0 event sink (sound)** — `0x7DC8D MOV EAX,0x1E; 0x7DC92 CALL 0x974DC`.
  `FUN_000974DC` (fresh `disassemble_function`) = `PUSH EDX; MOV EDX,EAX; CALL
  0x64EC0; MOV EAX,EDX; CALL 0x65CC0; POP EDX; RET` (the audio wrapper). Fresh
  `get_xrefs_to 0x974DC` = **42** callers.
* **stage-2 event sink (ring)** — `0x7DEF5 CALL 0x8F188` with `EAX=0x57`,
  `EDX=EBP` (the record), `EBX=4`. `FUN_0008F188` guards on
  `0x8F192 CMP byte[0x14C32A],0 / JNZ 0x8F2BF` **and**
  `0x8F19F CMP dword[0x14C312],0 / JZ 0x8F2BF` — i.e. the sink is inert unless
  `[0x14C32A] == 0` **and** `[0x14C312] != 0`, then stages a 32-byte ring entry at
  `[0x15aae0 + 32*(n%10)]`. Fresh `get_xrefs_to 0x8F188` = **130**.
* **stage-2 ball / metric sinks:**
  * `0x7DF0B CALL 0x8DE8C` — nearest record from `&rec+0x59`, skipping
    `[+0x8A]>>24`; result kept in ESI.
  * `0x7DF1F CALL 0x8DCD4` with `EAX=0x15774C`, `EDX=nearest+0x59`,
    `EBX=0x158738` — the metric (the `FUN_0008DCD4` distance word written into
    `[0x158738]`).
  * `0x7DF24 MOV DI,word[0x15873A] / 0x7DF30 SAR DI,1 / 0x7DF35 MOV
    word[0x15873A],DI` — the halve.
  * `0x7DF3C CALL 0x92820` with `EAX=EBP`, `EDX=1` — the record-slot staging
    (`FUN_00092820` writes `[0x15B650]`, the `0x157758` triple, `[0x15B655]=rec`).
  * `0x7DF53 CALL 0x7A490` with `EAX=EBP`, `EDX=0x158738`, `ECX=2`, `EBX=0`,
    stack `push 6; push 0` — the ball/target staging machine (`RET 0x8`;
    `FUN_0007A490` writes `[0x158730]=rec`, copies the vec to `[0x158738]`,
    `[0x15873E]`, `[0x158743]`).
  * `0x7DF5B MOV [EAX+0x7B2],ESI` — binds the team's nearest record (the restart
    target).
  * `0x7DF75 conditional CALL 0x7876C` — the slot merge (no-op while the slot
    pool is unmodelled).
* **`0x4C380`** — `0x7DF95 CALL 0x4C380`; fresh `disassemble_function 0x4C380` =
  **a bare `RET`**. Not a sink (matches FU-143 §11.2's parenthetical).
* **stage-3 ack** — `0x7DFB2 CMP byte[EBP+0x44],0 / 0x7DFB6 JZ epilogue` else
  `0x7DFB8 MOV EAX,EBP; CALL 0x7DAB4` (reset). The `+0x44` ack is **W26-2's**
  T1-L2, not a sit sink.

### 3(b) Fresh reachability census

`0x700F4` = 29 callers, `0x8F188` = 130, `0x974DC` = 42, `0x92820`/`0x8DCD4`/
`0x7A490`/`0x8DE8C` = the per-row staging helpers (fresh `get_xrefs_to` counts
from this session). Every row-01 sink is reached through the row-01 body only
when its stage arm runs — i.e. only on the live kickoff route (stage 0 at
`global_5882a`, stage 2 after the stage-1 nearest probe).

### 3(c) Live-vs-leg ruling

| sink | engine today | ruling |
|---|---|---|
| camera reset `0x700F4(0,0,0)` | modelled through the derived **place request** (row 01's camera reset; the place/pan chain is R2-L4, W26-2) | **LIVE** (place request); the *pan* reach stays R2-L4 |
| sound `0x974DC(0x1E)` | **no engine consumer** | **LEG** (OL-84a) |
| ring `0x8F188(0x57, rec, 4)` | **no engine consumer**; native guard `[0x14C32A]==0 && [0x14C312]!=0` | **LEG** (OL-84a; the OL-67 event-ring seam) |
| metric `0x8DCD4` → `0x158738` | **unported** (the `0x158738`/`0x15873A` staging block) | **LEG** (OL-84b) |
| `[0x15873A] >>= 1` | **unported** | **LEG** (OL-84b) |
| `0x92820(rec,1)` | record-slot staging partly modelled by the pool; the row-01 call is unported | **LEG** (OL-84b) |
| ball staging `0x7A490` | **unported** (the FU-139 staging block is the eventual home) | **LEG** (OL-84b) |
| `[[rec]+0x7B2]=nearest` | the derived nearest uses `fifa96_match_entity_find_nearest` over the pool | **partial** (OL-84c) |
| `0x7876C` slot merge | no-op while the slot pool is unmodelled | **LEG** (OL-84f) |
| `0x4C380` | n/a (a `RET`) | **not a sink** |

**Net:** the camera sink is live through the derived place request; the event
(sound + ring), metric, and ball-stage sinks are all **legs** — no engine
consumer. No census move.

---

## Step 4 — Port shapes + the live-trigger ruling (hand off)

### 4.1 The port shapes

1. **Situation-0xB producer (row 01)** — already landed (`handlers.c:276`). No
   port work; keep as the **first of two live** sit-0xB producers.
2. **Situation-0xB producers (rows 0x11, 0x10, 0x12, 0x13; keeper 1D/1E)** — the
   shared engine path is `match_sp_resolve` → `fifa96_match_run_situation(mr, 0x0B)`
   and `match_keeper_*_apply` → `out->situation_0b`. **Row 0x11 (`0x863F9`) is
   already live on the M2 tape** (its row is dispatched by the derived FU-170
   restart wire — no port work). The port work is **not** the `0x0B` call (landed)
   — it is the **live arm** for rows 0x10/0x12/0x13 and keeper 1D/1E: W26-3
   (FU-266) and W26-6 (FU-269). A stand-in that stages the taker/keeper gate is a
   **fake** (spec §2 non-goal).
3. **`0x8B85D` (act-0xA terminal → sit 1)** — landed (phase-0x20 registered).
   Keep.
4. **`0x8A8CE` (act-8 sweep stage-2 terminal)** — the de-compression has
   **landed** (FU-241 §6.3, phase-23 T4): the phase-`0x1E` slot is registered
   (`handlers.c:5629`) and the `set_piece` `bx!=0` branch now writes phase 0 then
   `fifa96_match_act_dispatch(mr, 8, 0, 1)` (`match_run.c:3144-3157`). No port work
   remains; the sweep defers the stored-situation re-dispatch to `0x8A8CE` and is
   render-invisible/tape-dormant — **no golden re-pin**.
5. **Row-01 sinks** — port only when a consumer exists (the OL-67 event-ring seam
   for `0x8F188`/`0x974DC`; the FU-139 staging block for `0x8DCD4`/`0x158738`/
   `0x7A490`). Each sink carried by the existing taker-arm slice (W26-3) at its
   own call site; do not double-port.

### 4.2 The live-trigger ruling (Layer 1 / Layer 2)

* **Layer 1 (engine-reach)** — the sit-0xB → phase-2 gate is raised **live** by
  **two** producers: **row 01 `0x7DF90`** (the derived kickoff route; headless
  proof `tests/test_engine_match_frame.c:860
  test_kickoff_enters_phase2_when_cutscene_isolated`, supplying the native kick
  press) and **row 0x11 `0x863F9`** (the derived FU-170 restart wire, tape-proven
  `test_engine_m2.c:1195`/`:1902`/`:5087`). The other **six** sit-0xB producers
  (rows 0x10/0x12/0x13, keeper 1D/1E) have **no live arm** — owners
  W26-3/W26-6. The `0x8A8CE` act-8 terminal and the `0x8B85D` act-0xA terminal
  are both live (engine-reach).
* **Layer 2 (visible)** — this slice's chains are **not** visible on the smoke
  (the sit-0xB route is a phase-2 write; the row-01 camera reset is a place, not
  a follow). The visible anchor remains the follow-cam (W26-1/FU-264), which is
  granted-frame-only. **No Layer-2 claim from this slice.**

### 4.3 Handoff

* **W26-3 / FU-266 (taker arm):** the dormant sit-0xB producers are
  `0x85D38`/`0x84495`/`0x84E8F` (rows 0x10/0x12/0x13); row 0x11 (`0x863F9`) is
  already live via the FU-170 wire — the residual is the arm for the other rows,
  not the `0x0B` call. Row 0x10 additionally has the sit-2 BX=1 re-queue `0x85D0A`
  and writes `[0x157A6A]=0x12C`.
* **W26-6 / FU-269 (keeper):** the sit-0xB producers are `0x7546E` (row 1D),
  `0x75B58` + `0x76072` (row 1E); the live trigger is the keeper ladder.
* **W26-2 / FU-265 (place/pan):** the row-01 camera reset `0x700F4` is the place
  sink; the stage-3 `+0x44` ack (`0x7DFB2`) is T1-L2.
* **The act-8 owner:** `0x8A8CE` is **landed** (FU-241 §6.3) — the open residuals
  are the non-tape equivalence (FU241-L1) and act-slot residency (FU241-L3), not a
  port; `0x8B85D` stays live.

---

## Numbered open legs (native owners)

1. **FU267-L1 — the six dormant situation-0xB producers.** Rows 0x10/0x12/0x13
   (`0x85D38`/`0x84495`/`0x84E8F`) and keeper 1D/1E (`0x7546E`/`0x75B58`/
   `0x76072`) call `0x8A938(0xB)` and are ported, but **no live engine path
   dispatches their rows**. Owners: W26-3/FU-266 (taker arm, FU-149 L13) and
   W26-6/FU-269 (keeper ladder, FU-151). The `0x0B` call itself is live-landed —
   do not re-port it. (Row 0x11 `0x863F9` is **not** in this leg: it is live on
   the M2 tape via the derived FU-170 restart wire.)
2. **FU267-L2 — the act-8 terminal `0x8A8CE` (re-scoped; landed).** The
   de-compression landed (FU-241 §6.3, M2 phase-23 T4): phase `0x1E` is
   registered/counted (`handlers.c:5629`) and the `set_piece` `bx!=0` branch now
   dispatches act 8 invoke-now (`match_run.c:3144-3157`), deferring the
   stored-situation re-dispatch to the sweep's stage-2 terminal `0x8A8CE` (~22
   granted frames). **No producer is missing.** The open residuals are the
   **non-tape** compression equivalence (FU241-L1) and the act-slot residency
   across the sweep (FU241-L3); the `word[0x157A68]` reader stays FU194-L4. The
   sweep is render-invisible and dormant on M2 (no `state=3`; golden
   byte-unchanged).
3. **FU267-L3 — row 02 mis-classed.** `OL-84d` groups row 02 with the
   situation-0xB producers; fresh bytes + the 39-site `get_xrefs_to 0x8A938`
   prove row 02 (`0x7DFCC`) is **not** a producer (it installs action 4 in phase
   2). Corrected here; the corrected residual is FU267-L1. Owner: FU-143 §11.5
   promoter.
4. **FU267-L4 — row-01 event sinks.** The sound `0x974DC(0x1E)` (`0x7DC92`) and
   the ring `0x8F188` (`0x7DEF5`, guard `[0x14C32A]==0 && [0x14C312]!=0`) have no
   engine consumer. Owner: the OL-67 event-ring seam (FU-149 L13.3). Reference:
   OL-84a.
5. **FU267-L5 — row-01 ball/metric staging.** `0x8DCD4` → `0x158738` (`0x7DF1F`),
   the `[0x15873A] >>= 1` (`0x7DF30`), `0x92820` (`0x7DF3C`) and the `0x7A490`
   ball staging (`0x7DF53`) stay unported. Owner: the FU-139 staging block.
   Reference: OL-84b.
6. **FU267-L6 — `0x4C374`/`0x4C380` no-ops.** `0x4C380` is a bare `RET`; `0x4C374`
   ignores `EAX` and only clears `word[0x157A68]` (reconfirmed). No engine cell for
   `word[0x157A68]`; reader untraced. Owner: FU194-L4 / FU241-L2.
7. **FU267-L7 — the `FUN_0008A938` twin producers.** `0x742DE` (sit 0) and
   `0x74312` (sit 8) duplicate `FUN_0004B02C`'s body inside the `0x742xx`
   match-init/period-end family (no auto-created function). Whether they are a
   second live match-init/period-end producer for the derived entry is not
   decomposed here. Owner: the period-setup slice (FU196-L2/FU202-L2).

---

## Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled;
`.image` overlay present; `get_current_program_info` confirmed this session):

* `disassemble_bytes` (start, length in B): `0x7DFCC` (120), `0x7E043` (96),
  `0x85CF0` (80), `0x863E0` (48), `0x84478` (48), `0x84E70` (48), `0x75458` (64),
  `0x75B40` (64), `0x76058` (64), `0x76A78` (64), `0x76B08` (64), `0x77578` (64),
  `0x776F8` (64), `0x8A3E8` (48), `0x8A3C0` (48), `0x89200` (48), `0x891E0` (48),
  `0x897D8` (48), `0x897C0` (48), `0x8B840` (96), `0x742D8` (48), `0x74308` (48),
  `0x7DBB0` (176), `0x7DC60` (64), `0x7DD20` (64), `0x7DEE0` (192), `0x7DFA0`
  (64), `0x8A938` (240), `0x8A8A0` (80), `0x8AA70` (64), `0x8AEE0` (48),
  `0x88B40` (48), `0x88BB8` (48), `0x88BF8` (48), `0x38B20` (40), `0x38C88` (40).
* `disassemble_function`: `0x974DC` (7 insns), `0x92820` (27), `0x8DCD4` (61),
  `0x4C380` (1), `0x8F188` (93), `0x7A490` (703), `0x700F4` (133).
* `read_memory`: `0x1106E0` (180 B, the action table), `0x8A904` (52 B, table-2).
* `get_xrefs_to`: `0x8A938` (39), `0x740A0` (27), `0x8A798` (1), `0x888FC` (11),
  `0x8AF1A` (1), `0x7D9A4` (77), `0x700F4` (29), `0x8F188` (130), `0x974DC` (42).
* `get_function_by_address` (all `none`): `0x7DBC0`, `0x7DF90`, `0x7546E`,
  `0x75B58`, `0x76072`, `0x85D38`, `0x863F9`, `0x84495`, `0x84E8F`, `0x8A3F0`,
  `0x8920A`, `0x897E3`, `0x742DE`, `0x8B688`.

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(`:240..300` the action table, `:470..534` keeper-claim apply, `:549..646`
keeper close-down + `action_1D`/`_1E`, `:1500..1580` `match_sp_resolve`,
`:1609..2130` `action_10..13`, `:5476..5520` the action/phase tables,
**`:5629` `{0x1E, fifa96_match_action_phase_1E, …}` — the phase-`0x1E` slot
registered/COUNTED**); `src/fifa96_engine/fifa96_match_run.c` (`:2948`
`fifa96_match_run_situation`, `:6105` the phase-0x20 terminal, **`:3144-3157`
the de-compressed `bx != 0` branch: `match_run_write_phase(mr, 0u)` then
`fifa96_match_act_dispatch(mr, 8, 0, 1)`**); `include/fifa96_engine/
fifa96_match_run.h` (`:1261`). Tests/tape read first-hand this fix round:
`tests/test_engine_match_frame.c:860 test_kickoff_enters_phase2_when_cutscene_isolated`
(+ `:911` the `FIFA96_ENGINE_KEY_KICK` press); `tests/test_engine_m2.c:1195`
(the first tape dispatch of row 0x11), `:1902` (`M2_RESTART_ROWS_MASK`),
`:5087` (`mask_final == (M2_WIRED_MASK | M2_RESTART_ROWS_MASK)`).
Docs cross-cited: FU-143 §10/§11 (OL-84a…h), FU-196 §2/§3.3/§5, FU-202, FU-212
(§2/§6), FU-241 (§5.2 superseded / §6.3 the landing), `docs/ENGINE.md:8` (59/80
incl. phase `0x1E`) / `:1753` (row 0x11 dispatches on the tape). No writes beyond
this draft + the run report; no rename/label/comment/function/script/project
save; no git; the ISO never touched.
