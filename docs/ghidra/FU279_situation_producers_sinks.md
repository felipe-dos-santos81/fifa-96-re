Promoted canonical slice — phase-26b (FU279); read-only Ghidra session, authoritative program **`/FIFA96.EXE`**; case-sensitive. No rename/comment/label/function/script/project save.

Phase/Provenance: M2 phase-26b recon-ahead wave `2026-10-14-fifa96-m2-phase26b-recon-ahead`, Task 7 (bounded, read-only derivation), promoted to canonical by the phase-26b close-out. No production code; the derivation and its ruling are unchanged by promotion.

# W26b-7 / FU-279 — the situation-0xB producers (the six dormant) + row-01's sinks + the act-8 tail

Provenance: M2 phase-26b wave W26b-7, 2026-10-14, **read-only** recon. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `/FIFA96.EXE`,
`watcom:LE:32:default`, 2706 functions, 15239 symbols, `.image` overlay present,
min address `0x10000`, max `.image::0x174a2a`, image base 0). Every address/byte
below is a fresh `disassemble_bytes` / `read_memory` / `disassemble_function` /
`get_xrefs_to` / `get_function_by_address` of **this** session unless marked
*cross-cited* (then the frozen FU doc is named). LE-image globals sit at
**`+0x100000`** from the file offset; the addresses are `/FIFA96.EXE` runtime
(program) addresses. Alias traps applied: `dword[x]>>16` = the word at `x+2`;
`>>24` = the byte at `x+3`; CALL bytes verified; sign/width and inverted-gate
checks done. Engine/loader/tests/golden read first-hand this session:
`src/fifa96_engine/fifa96_match_handlers.c`, `fifa96_match_run.c`,
`fifa96_match_act.c`, `src/fifa96_loader/fifa96_action_handlers.c`,
`tests/test_engine_m2.c`, `tests/test_engine_match_frame.c`, the goldens
`tests/golden/engine/{m1,m2}-frames.txt`. **No writes**: no rename/comment/label/
function/script/project save; `/FIFA96.EXE` untouched; the ISO never touched; no
git commands. This is one new draft file plus the run report.

Scope (brief): extends **FU-267** (`docs/ghidra/FU267_situation_producers.md`).
Derive the **six dormant situation-0xB producers** (rows `0x10`/`0x12`/`0x13` +
keeper `1D`/`1E`, i.e. the six `0x8A938(0xB, side, BX=0)` call sites), the
**row-01 sinks** (`0x974DC(0x1E)` / `0x8F188` / `0x8DCD4`→`0x158738` / `0x7A490`
and the `0x92820`/`0x8DE8C`/`0x7876C`/`0x700F4` ring), and the **`FU241-L1` /
`FU241-L3`** act-8 residues. This slice does **not** re-derive the taker-arm
(`W26b-2`/FU274), the taker residuals (`W26b-3`/FU275) or the keeper-restart
trigger (`W26b-5`/FU277) — it reads them only for the **live-arm** ruling.

**The live-chain rule (binding).** A producer is **live** only when a **ported
producer raiser** raises the gate on a real engine path; a fixture, a stand-in, or
a synthetic press does not count. Every "only/sole/exactly" or caller-count claim
below rests on a **fresh** `get_xrefs_to` this session.

---

## 0. Fresh anchors — the situation→act map (unchanged since FU-267)

Fresh `read_memory 0x8A904` (52 B, table-2, index = situation, dword-index
`0xB*4 = 0x2C` → offset `0x8A930`): fresh `read_memory` bytes give
`[0xB] = 0x0008AEF6`. Fresh `disassemble_bytes 0x8AEE0` (48 B) — the **sit-0xB
arm**:

```
0x8AEF6 MOV EDX,[ESP-2]         ; the side word ([ESP] set by 0x8A940 MOV [ESP],DX)
0x8AEFA MOV EAX,2
0x8AEFF SAR EDX,0x10
0x8AF02 CALL 0x740A0            ; FUN_000740A0(2, side) -> PHASE 2
0x8AF07 ADD ESP,0x4 ; POP×3 ; RET
```

The following `0x8AF0E MOV EBX,1` is the sit-0xC arm (falls into the shared
`0x8AF1A CALL 0x888FC`), so the sit-0xB arm is a leaf. Fresh
`get_xrefs_to 0x740A0` = **27** (the dispatcher arms `0x8AA89`/`0x8AB9F`/`0x8ABE7`/
`0x8AC1C`/`0x8ADB4`/`0x8ADCC`/`0x8ADE4`/`0x8ADFC`/`0x8AF02`, `FUN_0008B9CC 0x8BAC1`,
`FUN_0008BAF0 0x8BCCF`, the front-end/screen family). So **every** situation-0xB
producer's sole effect on the phase cell is this one `FUN_000740A0(2, side)` →
**phase 2** — the sole phase-2 writer (FU-196 §2.2).

Fresh `disassemble_bytes 0x8A938` (160 B) reconfirms the dispatcher head: the
`EAX==0xB` (`0x8A94E CMP EAX,0xB / 0x8A951 JZ 0x8AA7B`, bytes `83 f8 0b` /
`0f 84 24 01 00 00`) routes sit-0xB into the `BX==0` table-2 route (with the
`[0x14C32A]`/`[0x15B6C0]` gate arms at `0x8A957`/`0x8A964`). `0x8A93E MOV ECX,EAX`
clobbers any caller `ECX`, so the row-`1E` site's `0x75B53 MOV ECX,1` does **not**
change the sit-0xB route.

Fresh `read_memory 0x1106E0` (180 B, the action table) — row → body map:

| row | body | row | body | row | body |
|---|---|---|---|---|---|
| `0x10` | `0x855F0` | `0x11` | `0x85DE4` | `0x12` | `0x83D68` |
| `0x13` | `0x84B00` | `0x1D` | `0x74EB0` | `0x1E` | `0x7550C` |
| `0x1F` | `0x76380` | | | | |

(so `0x76072` sits inside row `0x1E`, not `0x1F`). Fresh
`get_xrefs_to 0x8A938` = **39** — byte-identical to FU-267 §1(a). The eight
sit-0xB (`EAX=0xB`, `BX=0`) sites are `0x7DF90` (row 01), `0x85D38` (row `0x10`),
`0x863F9` (row `0x11`), `0x84495` (row `0x12`), `0x84E8F` (row `0x13`), `0x7546E`
(row `1D`), `0x75B58` + `0x76072` (row `1E`).

---

## Step 1 — The six dormant producers (fresh bytes) + what live arm each needs

### 1(a) Native producers (fresh, this session)

All six are `EAX=0xB`, `EDX = rec team side` (`MOV EDX,[EBP*]; MOV DL,[EDX+0x826];
AND EDX,0xFF`), `EBX=0`, tail `CALL 0x8A938`:

| # | site | row | fresh pre-tail bytes / CALL bytes | post-call tail |
|---|---|---|---|---|
| 1 | `0x85D38` | `0x10` | `0x85D19 MOV word[0x157A6A],0x12C` (`66 c7 05 6a 7a 15 00 2c 01`); `0x85D2B MOV EAX,0xB`; `0x85D30 AND EDX,0xFF`; `0x85D36 XOR EBX,EBX`; `0x85D38 CALL` (`e8 fb 4b 00 00`) | `0x85D3D CALL 0x4C380`; `0x85D42 MOV CH,[EBP+0x92]` |
| 2 | `0x84495` | `0x12` | `0x8447B CALL 0x4C374` (the `word[0x157A68]` clear); `0x84488 MOV EAX,0xB`; `0x8448D AND EDX,0xFF`; `0x84493 XOR EBX,EBX`; `0x84495 CALL` (`e8 9e 64 00 00`) | `0x8449A MOV EAX,[0x158740]`; `0x844A4 MOV byte[0x157AB0],BL` |
| 3 | `0x84E8F` | `0x13` | gated `0x84E70 CMP byte[0x158746],0 / 0x84E77 JZ 0x84EC5`; `0x84E82 MOV EAX,0xB`; `0x84E87 AND EDX,0xFF`; `0x84E8D XOR EBX,EBX`; `0x84E8F CALL` (`e8 a4 5a 00 00`) | `0x84E94 CALL 0x4C380`; `0x84E9F MOV dword[EBP+0x89],0` |
| 4 | `0x7546E` | `1D` | `0x7545B MOV DL,[EDX+0x826]`; `0x75461 MOV EAX,0xB`; `0x75466 AND EDX,0xFF`; `0x7546C XOR EBX,EBX`; `0x7546E CALL` (`e8 c5 54 01 00`) | `0x75475 MOV byte[0x157AB2],AH(0)`; `0x7547B CALL 0x4C380` |
| 5 | `0x75B58` | `1E` | `0x75B46 MOV EAX,0xB`; `0x75B4B AND EDX,0xFF`; `0x75B51 XOR EBX,EBX`; `0x75B53 MOV ECX,1`; `0x75B58 CALL` (`e8 db 4d 01 00`) | `0x75B5D MOV EDX,5`; `0x75B62 MOV EAX,[EBP-0xC]`; `0x75B67 CALL 0x7D9A4` (install action 5) |
| 6 | `0x76072` | `1E` | `0x76063 MOV EAX,0xB`; `0x76068 AND EDX,0xFF`; `0x7606E XOR EBX,EBX`; `0x76070 XOR CL,CL`; `0x76072 CALL` (`e8 c1 48 01 00`) | `0x76077 MOV byte[0x157AB2],CL(0)`; `0x7607D CALL 0x4C380` |

Fresh `get_function_by_address` on all six sites (`0x85D38`, `0x84495`, `0x84E8F`,
`0x7546E`, `0x75B58`, `0x76072`) and on `0x7DF90` (row 01) = **"No function
found"** — they are table-dispatched record bodies, not auto-created functions
(identical to FU-267 §2(a)).

### 1(b) The engine today + the live-arm each needs

The `0x0B` call is **already landed** for every one of the eight producers:

* takers — `match_sp_resolve` (`fifa96_match_handlers.c:1553-1557`) calls
  `fifa96_match_run_situation(mr, 0x0Bu)` (`:1556`); rows `0x10`/`0x12`/`0x13`
  reach it at `:1730`/`:2002`/`:2154` with `set_offside_timer` = `1`/`0`/`0`
  (matching the fresh `0x85D19`/none/none offside-timer writes).
* keepers — `match_keeper_claim_apply` (`:532`) and `match_keeper_closedown_apply`
  (`:631`) each `return fifa96_match_run_situation(mr, 0x0Bu)` on
  `out->situation_0b`.
* row 01 — `fifa96_match_handlers.c:276` (`0x7DF90`).

Rows `0x10`/`0x11`/`0x12`/`0x13` are **wired in the action table**
(`handlers.c:5486-5492`), keeper `1D`/`1E` at `:5518-5520`. So the missing piece
is **not** the `0x0B` call — it is the **raiser that installs the row live**
(the "live arm"). Fresh engine reads give the per-producer requirement:

| producer | engine body | fresh engine state | the live arm it needs | owner |
|---|---|---|---|---|
| row `0x10` `0x85D38` | `fifa96_match_action_10` (`:1609`), gate `{2,3}` | ported/wired; the phase-3 **throw-in scanner** installer is unported — `match_run_goal_scan`'s throw-in arm `set_piece(2,…,1)` (`run.c:3300`) needs `state.phase==2 && goal_armed && zone!=0 && |z|<=0xB20` | the **throw-in scanner** (`FUN_00088940`, `0x88c00`) on a live route | W26b-2 / FU274 |
| row `0x12` `0x84495` | `fifa96_match_action_12` (`:1897`), gate `{2,7}` | ported/wired; no live phase-7 installer | the **foul adjudicator / ref restart** front-end (`match_run_ref_apply` `:5907`, sit-9 restart `:6535`) | W26b-2 / FU274 |
| row `0x13` `0x84E8F` | `fifa96_match_action_13` (`:2038`), gate `{2,6}` | ported/wired; no live phase-6 installer | the **penalty hand-off** at row-`0x13` stage 5 (`0x84D5B`, FU266 L266-10) | W26b-2 / FU274 |
| keeper `1D` `0x7546E` | `fifa96_match_action_1D` (`:635`) | ported; `out->situation_0b` → `0x0B` landed | the **phase-8 goal-kick** installer `FUN_0008D098` arm `0x8D607` ← situation 4 (`0x8ADC0`); upstream = the keeper CPU ladder | W26b-5 / FU277 (+ W26b-1 / FU273) |
| keeper `1E` `0x75B58`+`0x76072` | `fifa96_match_action_1E` (`:536`) | ported; `out->situation_0b` → `0x0B` landed | the **phase-9 keeper-rush** installer ← situation 5 (`0x8ADD8`) | W26b-5 / FU277 (+ W26b-1 / FU273) |

### 1(c) Live-vs-leg ruling — **0 / 6** of the six dormant producers are live

Fresh this session:

* The **M2 golden census** has **no `state=3`, no `state=6`, no `state=7`, no
  `state=8`, no `state=9`** — fresh `grep -o "state=[0-9-]*" … | sort | uniq -c`
  over `tests/golden/engine/m2-frames.txt` (sha `b281a9b25b26…`, 6522 lines) =
  `{0:304, 1:953, 2:2193, 4:303, 12:4, 19:2640, 20:20, 21:100}`. The only taker
  phase present is **4** (the corner arm).
* Consequently the **only** taker row dispatched on the tape is **row `0x11`**
  (`0x863F9`) — `tests/test_engine_m2.c:1902 #define M2_RESTART_ROWS_MASK
  (1ull << 0x11)` and `:5087 assert(res.mask_final == (M2_WIRED_MASK |
  M2_RESTART_ROWS_MASK))` (row-`0x11` comment at `:1194-1195`). Rows
  `0x10`/`0x12`/`0x13` and the keeper `1D`/`1E` are **never** installed on the
  tape.
* **Row 01** `0x7DF90` is live via the derived kickoff route — headless proof
  `tests/test_engine_match_frame.c:860
  test_kickoff_enters_phase2_when_cutscene_isolated` (the `FIFA96_ENGINE_KEY_KICK`
  press at `:911`).

**Ruling.** Of the eight situation-0xB producers **two are live** (row 01
`0x7DF90`, row `0x11` `0x863F9`) and **six are dormant** (rows
`0x10`/`0x12`/`0x13`, keeper `1D`/`1E`/`1E`). The six are ported, wired bodies
whose `0x0B` call lands — they simply have **no live raiser**. This is the same
net FU-267 §1(c) reached; no new producer is wired by this ruling and the census
does not move.

---

## Step 2 — Row-01's sinks (fresh bytes + fresh xrefs)

Row 01 body `0x7DBC0..0x7DFC8`, fresh `disassemble_bytes 0x7DBB0` (224 B) /
`0x7DEC0` (224 B). The stage jump table `0x7DBB0` (fresh `read_memory`, 16 B) =
`{0x7DC6B, 0x7DCAF, 0x7DD29, 0x7DFB2}` (stages 0..3). Fresh sink reads:

* **stage-0 camera reset** — `0x7DC01 CALL 0x700F4` pushing the constant triple
  `[0x10F328]/[0x10F32C]/[0x10F330]` (`0x7DBEC..0x7DC00`). Fresh
  `get_xrefs_to 0x700F4` = **29** (row 01's `0x7DC01` present); fresh
  `disassemble_function 0x700F4` = 133 insns (the camera/place init).
* **stage-0 sound** — `0x7DC8D MOV EAX,0x1E / 0x7DC92 CALL 0x974DC` (bytes
  `b8 1e 00 00 00` / `e8 45 98 01 00`). Fresh `disassemble_function 0x974DC` = 7
  insns (`PUSH EDX; MOV EDX,EAX; CALL 0x64EC0; MOV EAX,EDX; CALL 0x65CC0;
  POP EDX; RET`) — the audio wrapper. Fresh `get_xrefs_to 0x974DC` = **42**.
* **stage-2 ring** — the shared call site `0x7DEF5 CALL 0x8F188`; for row 01 the
  code-`0x57` variant is reached via `0x7DEE0 CALL 0xCBC4C / 0x7DEE5 TEST AL,1 /
  0x7DEE7 JZ 0x7DEFA` then `0x7DEE9 MOV EBX,4 / 0x7DEEE MOV EAX,0x57 /
  0x7DEF3 MOV EDX,EBP`. The
  same tail is shared with the ball-contest beeps `0x9B` (`0x7DEC4`) and `0x9E`
  (`0x7DED2`). Fresh `disassemble_function 0x8F188` = 93 insns; guard:
  `0x8F192 CMP byte[0x14C32A],0 / 0x8F199 JNZ 0x8F2BF` **and**
  `0x8F19F CMP dword[0x14C312],0 / 0x8F1A6 JZ 0x8F2BF` — inert unless
  `[0x14C32A] == 0` **and** `[0x14C312] != 0`. Ring stride: index
  `n = [0x15AADC] + 9`, entry at `[0x15AAE0 + 32*(n % 10)]`; then
  `INC dword[0x15AAD8]` and `[0x15AADC] = n % 10`. Fresh `get_xrefs_to 0x8F188`
  = **130** (row 01's `0x7DEF5` present).
* **stage-2 ball/metric staging:**
  * `0x7DF0B CALL 0x8DE8C` (`EAX=EBP+0x59`, `EBX=[EBP+0x8A]>>24` = the skip
    index, `ECX=0`, `EDX=[EBP]` = team base) → fresh `disassemble_function
    0x8DE8C` = 42 insns (the nearest record over `+0x59` with the `+0x98`/`+0x9A`
    skips; result in EBP, distance word written to `[ECX]`).
  * `0x7DF1F CALL 0x8DCD4` with `EAX=0x15774C`, `EDX=nearest+0x59`, `EBX=0x158738`
    → fresh `disassemble_function 0x8DCD4` = 61 insns (the metric write: `[ECX+2]`
    / `[ECX+4]` deltas and the 3/4-weighted distance at `[ECX]=word`).
  * `0x7DF24 MOV DI,[0x15873A] / 0x7DF30 SAR DI,1 / 0x7DF35 MOV [0x15873A],DI`
    (the halve).
  * `0x7DF3C CALL 0x92820` (`EAX=EBP`, `EDX=1`) → fresh `disassemble_function
    0x92820` = 27 insns (writes `[0x15B650]=DL`, the `0x157758` triple to
    `[0x15B659]`, `[0x15B655]=rec`, gated by `[0x110F1C+DL] & 1`).
  * `0x7DF53 CALL 0x7A490` (`EAX=EBP`, `EDX=0x158738`, `ECX=2`, `EBX=0`, stack
    `push 6; push 0`) → fresh `disassemble_function 0x7A490` = 703 insns; it
    writes `[0x158730]=rec`, copies the vec to `[0x158738]`/`[0x15873E]`/`[0x158743]`
    and runs the ball/target staging machine (including its own many `0x8F188`
    ring calls).
  * `0x7DF5B MOV [EAX+0x7B2],ESI` (the team's nearest-record bind).
  * `0x7DF75`/`0x7DC2A conditional `CALL 0x7876C` → fresh
    `disassemble_function 0x7876C` = 67 insns (the slot merge: over the 0xB
    mates, `0x8DC68` distance pick, then the `[rec+0x20]` slot swap +
    `CALL 0x78670`).
* **`0x4C380`** — `0x7DF95 CALL 0x4C380`; fresh `disassemble_function 0x4C380` =
  **`RET`** only. **Not a sink.** `0x4C374` (fresh 5 insns) = `PUSH EDX; XOR
  EDX,EDX; MOV word[0x157A68],DX; POP EDX; NOP` — **ignores EAX** (the
  FU-212-E1/FU-255 §4 no-op; `word[0x157A68]` is write-only/inert).
* **stage-3 ack** — `0x7DFB2 CMP byte[EBP+0x44],0 / JZ epilogue` else
  `0x7DFB8 MOV EAX,EBP; CALL 0x7DAB4` (reset) — the `+0x44` ack is W26-2's
  T1-L2, not a situation sink.

### 2(b) Live-vs-leg ruling

| sink | engine today | ruling |
|---|---|---|
| camera reset `0x700F4(0,0,0)` | modelled through the derived **place request** (row-01 camera reset; the place/pan chain is W26-2/FU265) | **LIVE** (place request); the *pan* reach is the W26-2 R2-L4 leg |
| sound `0x974DC(0x1E)` (`0x7DC92`) | **no engine consumer** | **LEG** |
| ring `0x8F188(0x57, rec, 4)` (`0x7DEF5`, guard `[0x14C32A]==0 && [0x14C312]!=0`) | **no engine consumer** | **LEG** (the OL-67 event-ring seam) |
| metric `0x8DCD4` → `0x158738` (`0x7DF1F`) | **unported** | **LEG** |
| `[0x15873A] >>= 1` (`0x7DF30`) | **unported** | **LEG** |
| `0x92820(rec,1)` (`0x7DF3C`) | record-slot staging partly modelled by the pool; the row-01 call is unported | **LEG** |
| ball staging `0x7A490` (`0x7DF53`) | **unported** (the FU-139 staging block is the eventual home) | **LEG** |
| `[[rec]+0x7B2]=nearest` (`0x7DF5B`) | the derived nearest uses `fifa96_match_entity_find_nearest` over the pool | **partial** |
| `0x8DE8C` nearest (`0x7DF0B`) | stand-in (`match_sp_nearest`/`match_row01_nearest`) | **partial** |
| `0x7876C` slot merge (`0x7DF75`/`0x7DC2A`) | no-op while the slot pool is unmodelled | **LEG** |
| `0x4C380` | n/a | **not a sink** (a bare `RET`) |

**Net:** the camera sink is live through the derived place request; the event
(sound + ring), metric, ball-stage and merge sinks are all **legs** — no engine
consumer. No census move.

---

## Step 3 — The act-8 / act-0xA tail (`FU241-L1` / `FU241-L3`)

### 3(a) The act-8 terminal is landed and live (fresh)

Fresh `disassemble_bytes 0x8A878` (104 B) reconfirms the stage-2 terminal
`0x8A895..0x8A8DD`:

```
0x8A895 CALL 0x4BEC8 ; TEST AX,AX ; JNZ 0x8A8DA   ; wait gate (0x4BEC8 = XOR EAX,EAX; RET, always open)
0x8A89F MOV [0x158818],AX(0)                      ; timer = 0
0x8A8AB MOV dword[0x158808],EDX(0)                ; act slot clear
0x8A8B1 MOV byte[0x158829],AH(0)                  ; stage clear
0x8A8B7 MOV byte[0x158828],AH(0)                  ; act id clear
0x8A8BD MOV EDX,[0x158829] ; SAR EDX,0x18         ; = byte 0x15882C = SIDE
0x8A8C3 MOV EAX,[0x158828] ; SAR EAX,0x18         ; = byte 0x15882B = SITUATION
0x8A8CE CALL 0x8A938                              ; re-dispatch (sit, side), BX=0
0x8A8D3 MOV byte[0x15882B],0xFF
```

`0x8A8CE` bytes `e8 65 00 00 00` → `0x8A8D3 + 0x65 = 0x8A938` (agrees with
Ghidra); it is in the fresh 39-set, `BX=0` (the table-2 route). The engine's
de-compression has **landed** — fresh `src/fifa96_engine/fifa96_match_run.c:3144-3157`
(`set_piece` `bx != 0`): `mr->store_15882c = side; mr->store_15882b = situation;
match_run_write_phase(mr, 0u); return fifa96_match_act_dispatch(mr, 8, 0, 1);`,
and fresh `fifa96_match_handlers.c:5629` registers
`{0x1E, fifa96_match_action_phase_1E, "FU-241 §6.3 (M2 phase-23 T4): … COUNTED(+1) …"}`.
So both terminal re-dispatches are engine-live: `0x8A8CE` (the act-8 sweep's
stage-2 terminal, live on the `bx!=0` route) and `0x8B85D` (the act-0xA terminal,
sit 1 → the kickoff; `handlers.c:5616`).

### 3(b) `FU241-L1` — non-tape compression equivalence: **CLOSED as a port**

**Cross-cited (frozen, verified consistent with the live engine this session):**
`docs/ghidra/FU255_phase23_residuals.md` §1/§2 rules **"FU241-L1 — CLOSED, no
port needed"**: the retired FU-212 compression is a *dormant-on-M2-only* in-frame
approximation, whereas the landed model (act-8 invoke-now + one sweep record per
granted frame, terminal on the 23rd call) is **exactly the native cadence**; the
seam `fifa96_match_act_dispatch`/`_tick` reproduce `FUN_000888FC` / `0x8B669..`
instruction-for-instruction. FU-255 §5 gives the **proof shape** (a non-tape
`set_piece(sit,1,1)` fixture: `state.phase==0` + `act.id==8` immediately; the
successor phase only after 22 ticks; the pre-tick same-frame double-run).

**Fresh reconciliation.** `docs/ENGINE.md:1489` and `docs/PROGRESS.md:1905` still
list `FU241-L1` among the "residual legs" (non-tape compression equivalence). The
two are **not in conflict**: FU-255 closed the *port* question (no port remains —
the landed model is the native), and what stays open is the **test-coverage**
question (the non-tape deferral is unexercised by a landed fixture). So
`FU241-L1` here is a **test-coverage residual, not a producer gap** — see
FU-279-L5.

### 3(c) `FU241-L3` — act-slot residency across the sweep: **OPEN (fidelity leg)**

The native act slot is the dword `[0x158808]` (handler pointer) with the byte
`[0x158828]` (act id) and `[0x158829]` (stage); `FUN_000888FC` latches
(`0x88905 CMP AX,CX / JZ 0x8893D`) else overwrites `[0x158828]`. The engine's
single `mr->act` slot mirrors this exactly — fresh
`src/fifa96_engine/fifa96_match_act.c:38-52`: `if (a->id == act) return FIFA96_OK;`
(the latch), else `a->id = act; a->timer = 0; a->stage = stage;`. During the ~22
sweep frames the native slot holds act 8 and the per-record phase walk + clock
tail run; the engine's slot must likewise survive untouched for those frames.

**The open leg:** whether some **competing engine sub-machine** dispatches a
*different* act during those ~22 granted frames (e.g. a concurrent `goal_scan`/
ref/set-piece situation dispatch on a non-tape path) and thereby overwrites
`mr->act.id` (id != act → the sweep is abandoned) is **not proven** here. The
native has the same single-slot latch, so the fidelity question is whether the
engine's **frame order** reproduces the native's competing dispatches across the
sweep window. No port gap — a frame-order/residency proof (FU-279-L6).

---

## Step 4 — Port shapes + the live-trigger ruling (hand off)

### 4.1 The port shapes

1. **Situation-0xB producer (row 01 `0x7DF90`)** — landed (`handlers.c:276`); live.
   Keep.
2. **Situation-0xB producers (rows `0x10`/`0x12`/`0x13`, keeper `1D`/`1E`)** — the
   shared `0x0B` call is **landed** (`match_sp_resolve` :1556; keeper
   `out->situation_0b` :532/:631). **The port work is not the `0x0B` call** — it
   is the **live raiser** for the six rows: the taker installer
   (`FUN_0008D098` per-phase arm — phases 3/4/6/7) for rows `0x10`/`0x12`/`0x13`
   (W26b-2/FU274) and the keeper phase-8/9 arm `0x8D607` upstream (W26b-5/FU277 +
   W26b-1/FU273). A stand-in that stages the gate is a **fake**.
3. **`0x8A8CE` (act-8 sweep terminal)** — landed (FU-241 §6.3); no port remains.
   The residual legs are the `FU241-L1` non-tape fixture proof and the
   `FU241-L3` residency proof — **not** a producer.
4. **`0x8B85D` (act-0xA terminal)** — landed (phase-0x20 registered); live. Keep.
5. **Row-01 sinks** — port only when a consumer exists (the OL-67 event-ring seam
   for `0x8F188`/`0x974DC`; the FU-139 staging block for `0x8DCD4`/`0x158738`/
   `0x7A490`). Each sink carried by its own slice at its own call site; do not
   double-port. Both sync to W26b-3/FU275.

### 4.2 The live-trigger ruling (Layer 1 / Layer 2)

* **Layer 1 (engine-reach)** — the sit-0xB → phase-2 gate is raised live by
  **two** producers: row 01 `0x7DF90` (the derived kickoff route) and row `0x11`
  `0x863F9` (the derived FU-170 restart wire, tape-proven). The **six** dormant
  producers have **no live arm** — owners W26b-2/FU274 (takers) and
  W26b-5/FU277 (keeper). The `0x8A8CE`/`0x8B85D` terminals are both engine-live.
* **Layer 2 (visible)** — this slice's chains are **not** visible on the smoke:
  the sit-0xB route is a phase-2 write, and the row-01 camera reset is a place,
  not a follow. The visible anchor remains the follow-cam (W26-1/FU264). **No
  Layer-2 claim from this slice.**

### 4.3 Handoff

* **W26b-2 / FU274 (taker live arm):** the dormant producers are `0x85D38`
  (row `0x10`), `0x84495` (row `0x12`), `0x84E8F` (row `0x13`); row `0x11`
  (`0x863F9`) is already live via the FU-170 wire. The residual is the **raiser**
  (throw-in scanner / foul adjudicator / penalty hand-off), not the `0x0B` call.
* **W26b-5 / FU277 (keeper restart trigger) + W26b-1 / FU273 (keeper CPU chain):**
  the dormant producers are `0x7546E` (row `1D`), `0x75B58` + `0x76072` (row
  `1E`); the live raiser is the phase-8/9 arm upstream.
* **W26b-3 / FU275 (taker residuals):** the row-01 `0x974DC(0x1E)`/`0x8F188`
  sinks and the `0x8DCD4`/`0x7A490` staging block are the same seams; reconcile
  at promotion.
* **The act-8 owner:** `0x8A8CE` is landed; the open residuals are `FU241-L1`
  (proof/fixture) and `FU241-L3` (residency), not a port.

---

## Numbered open legs (native owners)

1. **FU279-L1 — the six dormant situation-0xB producers.** Rows `0x10`/`0x12`/
   `0x13` (`0x85D38`/`0x84495`/`0x84E8F`) and keeper `1D`/`1E` (`0x7546E`/
   `0x75B58`/`0x76072`) call `0x8A938(0xB, side, BX=0)` and are ported/wired,
   but **no live engine path installs their rows**; 0/6 live. Owners: W26b-2/FU274
   (taker installer, FU-149 L13) and W26b-5/FU277 + W26b-1/FU273 (keeper phase-8/9
   arm). The `0x0B` call itself is landed — do not re-port it.
2. **FU279-L2 — row-01 event sinks.** The sound `0x974DC(0x1E)` (`0x7DC92`) and
   the ring `0x8F188` (`0x7DEF5`, guard `[0x14C32A]==0 && [0x14C312]!=0`, entry
   `[0x15AAE0+32*(n%10)]`) have no engine consumer. Owner: the OL-67 event-ring
   seam (W26b-3/FU275; FU-149 L13).
3. **FU279-L3 — row-01 ball/metric staging.** `0x8DCD4` → `0x158738` (`0x7DF1F`),
   the `[0x15873A] >>= 1` (`0x7DF30`), `0x92820` (`0x7DF3C`) and the `0x7A490`
   ball staging (`0x7DF53`) stay unported. Owner: the FU-139 staging block
   (W26b-3/FU275).
4. **FU279-L4 — row-01 nearest/merge.** The `0x8DE8C` nearest (`0x7DF0B`) and the
   `0x7876C` slot merge (`0x7DF75`/`0x7DC2A`) are stand-in/no-op; the
   `[[rec]+0x7B2]=nearest` bind (`0x7DF5B`) is only partial. Owner: FU-149.
5. **FU279-L5 — `FU241-L1` non-tape fixture proof.** No port remains (FU-255 §2);
   the open item is the non-tape `set_piece(sit,1,1)` fixture asserting the
   ~22-frame phase-0 dwell and the pre-tick double-run (FU-255 §5). Owner: the
   act-8 landing / test coverage. Cited in `docs/ENGINE.md:1489` /
   `docs/PROGRESS.md:1905`.
6. **FU279-L6 — `FU241-L3` act-slot residency across the sweep.** The engine's
   single `mr->act` slot (`fifa96_match_act.c:38-52`, the `a->id==act` latch)
   must survive the ~22 sweep frames against any competing act dispatch; whether
   the engine's frame order reproduces the native's competing dispatches is
   unproven. Owner: the act-8 landing. Reference: FU212-L3.
7. **FU279-L7 — `word[0x157A68]` / the `0x4C374`/`0x4C380` no-ops.**
   `0x4C380` is a bare `RET`; `0x4C374` ignores `EAX` and clears the write-only
   `word[0x157A68]` (fresh `get_xrefs_to` per FU-255 §4 = 4, no reader). No engine
   cell; inert. Owner: FU194-L4 / FU255-L3. Reference: FU267-L6.

---

## Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled; `.image`
overlay present; `get_current_program_info` confirmed this session — 2706
functions, 15239 symbols, min `0x10000`, max `.image::0x174a2a`):

* `disassemble_bytes` (start, length in B): `0x85D10` (56), `0x84470` (64),
  `0x84E68` (64), `0x75450` (48), `0x75B38` (56), `0x76050` (48), `0x8A938` (160),
  `0x8AEE0` (48), `0x7DBB0` (224), `0x7DEC0` (224), `0x7DC88` (40), `0x7DE98`
  (72), `0x7DCAF` (128), `0x8A878` (104).
* `disassemble_function`: `0x974DC` (7), `0x4C380` (1), `0x4C374` (5), `0x8F188`
  (93), `0x8DCD4` (61), `0x92820` (27), `0x7A490` (703), `0x700F4` (133),
  `0x7876C` (67), `0x8DE8C` (42).
* `read_memory`: `0x8A904` (52 B, table-2), `0x1106E0` (180 B, the action table),
  `0x7DBB0` (16 B).
* `get_xrefs_to`: `0x8A938` (39), `0x740A0` (27), `0x974DC` (42), `0x8F188`
  (130), `0x700F4` (29).
* `get_function_by_address` (all "No function found"): `0x85D38`, `0x84495`,
  `0x84E8F`, `0x7546E`, `0x75B58`, `0x76072`, `0x7DF90`.
* `get_current_program_info`.

Engine/loader/tests read first-hand this session:
`src/fifa96_engine/fifa96_match_handlers.c` (`:276` row-01 `0x7DF90`; `:532`/`:631`
keeper `out->situation_0b`; `:1553-1557` `match_sp_resolve`; `:1609`/`:1897`/`:2038`
rows 10/12/13; `:5629` phase-`0x1E` registered; `:5591` phase-`0x0B` NULL;
`:5486-5492`/`:5518-5520` the action table); `src/fifa96_engine/fifa96_match_run.c`
(`:2896-2947` `match_run_situation_side`; `:3144-3157` the de-compressed `bx!=0`
branch; `:6207` the sole `match_run_write_phase(0x0Bu)` inside the unwired row-0x1A
body); `src/fifa96_engine/fifa96_match_act.c` (`:38-65` the dispatch/tick seam);
`src/fifa96_loader/fifa96_action_handlers.c` (`:1129-1147` the situation→phase
table: sit `0xB` → phase 2); tests `tests/test_engine_m2.c` (`:1194-1195`, `:1902`,
`:5087`), `tests/test_engine_match_frame.c` (`:860`, `:911`); the goldens
`tests/golden/engine/m2-frames.txt` (sha `b281a9b25b26…`, 6522 lines, census
`{0:304,1:953,2:2193,4:303,12:4,19:2640,20:20,21:100}` — no `state=3/6/7/8/9`) and
`m1-frames.txt` (sha `09b726b7a751…`, 691 lines). Docs cross-cited: FU-267
(§1/§2/§3/§4), FU-241 (§6.3), FU-255 (§1/§2/§4/§5), FU-212 (§6/E1), FU-196 §2.2,
FU-143 §11, FU-266 (L266-5/6/8/10), FU-269, FU-149 L13, `docs/ENGINE.md:1489`,
`docs/PROGRESS.md:1905`, README (73%). **No writes** beyond this draft + the run
report; no rename/label/comment/function/script/project save; no git; the ISO
never touched.
