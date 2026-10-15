Promoted canonical slice — phase-26b (FU274); read-only Ghidra session, authoritative program **`/FIFA96.EXE`**; case-sensitive. No rename/comment/label/function/script/project save.

Phase/Provenance: M2 phase-26b recon-ahead wave `2026-10-14-fifa96-m2-phase26b-recon-ahead`, Task 2 (bounded, read-only derivation), promoted to canonical by the phase-26b close-out. No production code; the derivation and its ruling are unchanged by promotion.

# W26b-2 / FU274 — the set-piece / restart live arm (the taker rows `0x10`/`0x12`/`0x13`)

Provenance: M2 phase-26b wave W26b-2, 2026-10-14, **read-only** recon. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `/FIFA96.EXE`,
`watcom:LE:32:default`, image base `0`, 2706 functions, `.image` overlay present,
min address `0x10000`). Every address/byte below is a fresh `disassemble_bytes` /
`disassemble_function` / `read_memory` / `get_xrefs_to` / `get_current_program_info`
of **this** session unless marked *cross-cited* (then the frozen FU doc is named).
LE-image globals sit at **`+0x100000`** from the file offset; every address quoted
is a `/FIFA96.EXE` runtime (program) address. The alias traps are applied
(`dword[x]>>16` = the word at `x+2`; `>>24` = the byte at `x+3`; CALL bytes
verified; sign/width and inverted-gate checks done). Engine source read
first-hand (`src/fifa96_engine/fifa96_match_run.c`, `fifa96_match_handlers.c`,
`src/fifa96_loader/fifa96_action_handlers.c`, `fifa96_camera.c`,
`fifa96_settings.c`, `fifa96_referee.c`, `include/fifa96_engine/…`).
**No writes**: no rename/comment/label/function/script/project save;
`/FIFA96.EXE` untouched; the ISO never touched; no git commands. This is one new
draft file (plus the run report).

Scope (brief): derive the **live engine arm** that dispatches the taker rows
`0x10`/`0x12`/`0x13` — the throw-in scanner + the foul adjudicator — and the
`goal_armed`→phase 3/4 producers the scanner gates them on. Builds on FU-149 §7 /
L13 (frozen), FU-150 (fouls/referee), FU-266 (taker-arm residuals, W26-3),
FU-267 (situation-0xB producers, W26-4), FU-271 (live-trigger census), FU-272
(R11-L4). Engine state cells read first-hand. Prior sections are not rewritten;
divergences land as errata.

**Live-chain rule (binding).** A taker row `0x10..0x13` is **live** only when a
**ported producer raiser** installs its code on a real engine path. Staging the
gate (or the phase) in a fixture, or a stand-in cell, is fixture-only / a fake.
A producer that is *wired but whose trigger never fires* is a **LIVE-LEG**
(ported raiser, live path structurally present, unproven end-to-end).

---

## Step 1 — the taker state cells (fresh xrefs + writers)

### 1.1 `[0x157821]` — the row-`0x10` far-ball re-queue gate

Fresh `get_xrefs_to 0x157821` = **11** (byte cell). Full set:

```
0x739BB  READ        FUN_000736AC
0x70A4C  READ        FUN_000709D0
0x70A65  READ_WRITE  FUN_000709D0   (INC)
0x7063D  READ        FUN_00070544
0x70153  WRITE       FUN_000700F4   (= 0)
0x7812E  READ        FUN_00077EAC
0x78144  READ        FUN_00077EAC
0x85CE4  READ        row 0x10 (0x855F0)   the re-queue gate
0x7489C  READ
0x74A8C  READ
0x7E295  READ
```

**Writers (fresh bytes):**

```
FUN_000700F4 0x70144 XOR AH,AH
             0x70153 MOV byte ptr [0x00157821],AH        ; b8? no: 88 25 21 78 15 00 -> = 0
FUN_000709D0 0x709D3 CMP word ptr [0x001577f0],0x0       ; pan event active?
             0x709DB JZ 0x70A3F
             ...
             0x70A3F MOV EAX,[0x0015781e]                 ; ALIAS dword[0x15781E]>>24 = byte[0x157821]
             0x70A44 SAR EAX,0x18
             0x70A47 CMP EAX,0x64
             0x70A4A JGE 0x70A6B                          ; cap 0x64
             0x70A4C CMP byte ptr [0x00157821],0x0
             0x70A53 JNZ 0x70A5F
             0x70A55 CALL 0x000cb2a4                      ; seed once on 0->1
             0x70A5A MOV [0x001575c3],EAX
             0x70A5F INC byte ptr [0x001575cb]
             0x70A65 INC byte ptr [0x00157821]            ; fe 05 21 78 15 00
```

So `[0x157821]` is a **signed byte counter** (0 → 0x64), cleared by the camera
reset `FUN_000700F4` and incremented per pan frame by `FUN_000709D0` **only while
`word[0x1577F0]` (the pan-event height) is nonzero**. The row-0x10 reader
(first-hand `0x85CE4..0x85CEB`) is `CMP byte[0x157821],0 ; JLE 0x85D94` — a signed
`<= 0` exit that skips the re-queue.

**Engine divergence (real, not an unported leg).** The native cell is **fully
ported** — `fifa96_camera.pan_counter` is at the same address
(`src/fifa96_loader/fifa96_camera.c:863` clears, `:893` caps at 100) and
`fifa96_camera_pan_step` increments it. But the ported row 0x10 reads a **separate
shadow** `mr->sp_157821` (`fifa96_match_handlers.c:1724`), seeded 0 only at
init/begin (`fifa96_match_run.c:2087`, `:2219`) and **never written again**
(fresh `rg sp_157821` in `src/` → only the header decl `:832`, the read `:1724`,
the two seeds `:2087`/`:2219`, and `:4049` a probe). So even if a live camera pan
raised the native counter, the ported row would not see it. Native owners:
`FUN_000700F4` (clear) + `FUN_000709D0` (INC).

### 1.2 `[0x157A6A]` — the offside-suppression timer

Fresh `get_xrefs_to 0x157A6A` = **6** (word cell):

```
0x73E85  WRITE        FUN_00073E28   (= 0)
0x79D79  READ         FUN_00079D5C   (the offside check)
0x85D19  WRITE        row 0x10 (0x855F0)   (= 0x12C)
0x863DF  WRITE        row 0x11 (0x85DE4)   (= 0x12C)
0x7438D  READ         the per-frame countdown
0x743A5  READ_WRITE   SUB word[0x157A6A],AX
```

**Writers (fresh bytes):**

```
FUN_00073E28 0x73E83 XOR DH,DH
             0x73E85 MOV word ptr [0x00157a6a],BX     ; BX=0 (XOR BX,BX 0x73E36)
row 0x10     0x85D19 MOV word ptr [0x00157a6a],0x12C  ; 66 c7 05 6a 7a 15 00 2c 01
row 0x11     0x863D8 MOV ECX,0x12C
             0x863DF MOV word ptr [0x00157a6a],CX     ; 66 89 0d 6a 7a 15 00
countdown    0x7438D MOV AX,[0x00157a6a]
             0x7439A CMP BX,AX ; JGE 0x743AC
             0x743A5 SUB word ptr [0x00157a6a],AX     ; -= the frame delta [0x157A64]
```

`FUN_00073E28` is the **match-reset** helper (it also clears `[0x157A4C]`,
`[0x157A4D]`, `[0x157A5C]`, `[0x157A60]`, `[0x157A9F]`, `[0x157A83]`,
`[0x157A64]`, `[0x157AB0]`, `[0x157A6F]`, `[0x157A73]` — fresh `0x73E28..0x73EDC`).
The engine models the cell as `referee.offside_suppress` (ported countdown in
`fifa96_match_run_referee_step`, `fifa96_match_run.c:6664-6670`; rows arm it via
`match_sp_resolve`, `fifa96_match_handlers.c:1555`). Native owner: `FUN_00073E28`
(reset) + rows 0x10/0x11 (arm).

---

## Step 2 — the throw-in scanner + the foul adjudicator (fresh bytes)

### 2.1 The scanner gate `FUN_0008AF38 0x8B623..0x8B64E`

Fresh bytes (the clock-tail block that carries the scanner and the period-0xC
clear):

```
0x8B623 MOV EAX,[0x00157a4a]      ; the phase dword
0x8B628 SAR EAX,0x18              ; >>24 = byte[0x157A4D] = the live phase
0x8B62B CMP EAX,0x2
0x8B62E JZ 0x8B635
0x8B630 CMP EAX,0x10
0x8B633 JNZ 0x8B645               ; phase must be 2 or 0x10
0x8B635 CMP byte ptr [0x0015781d],0x0
0x8B63C JZ 0x8B65B                ; goal_armed [0x15781D] must be set
0x8B63E CALL 0x00088940           ; the scanner
0x8B643 JMP 0x8B65B
0x8B645 CMP EAX,0xC               ; the sibling: phase 0xC
0x8B648 JNZ 0x8B65B
0x8B64A XOR EBX,EBX
0x8B64C XOR CH,CH
0x8B64E MOV word ptr [0x00157aba],BX   ; clears the period-0xC stall word, no scan
```

So the scanner's **only** entry is `phase ∈ {2,0x10} && goal_armed != 0`
(the `0xC` sibling does not call it). Native owner: `FUN_0008AF38`.

### 2.2 The scanner `FUN_00088940` — the arms

Fresh `get_xrefs_to 0x88940` = **1**: only `FUN_0008AF38 0x8B63E`. Fresh
`disassemble_function 0x88940` (205 insns). The situation arms:

**Head** (fresh `0x8896B..0x88996`): `ECX = [0x157784]` (the goal-snapshot z);
`CMP |ECX|, 0xB20` → `JLE 0x88BCC` (**throw-in arm**); else
`CMP byte[0x15781E],0 ; JZ 0x88B53` (**corner/goal-kick arm**, when the goal-zone
classifier `[0x15781E]` is 0); else the **goal-side select arm** `0x88996..0x88B44`.

**Throw-in arm `0x88BCC` (fresh):**

```
0x88BCC MOV EAX,[0x00157a4a] ; SAR EAX,0x18 ; CMP EAX,0x2 ; JNZ 0x88C05   ; phase == 2
0x88BD9 MOV EAX,0x1e ; CALL 0x000974dc                                   ; the sound sink
0x88BE3 MOV EAX,[0x001577ca] ; MOV EAX,[EAX] ; MOV AL,[EAX+0x826]
0x88BF0 XOR EDX,EDX ; XOR AL,0x1 ; MOV EBX,0x1 ; MOV DL,AL
0x88BFB MOV EAX,0x2
0x88C00 CALL 0x0008a938            ; FUN_0008A938(situation 2, ball-team ^ 1, BX=1)
```

**Corner/goal-kick arm `0x88B53` (fresh):**

```
0x88B53 MOV EAX,[0x00157a4a] ; SAR EAX,0x18 ; CMP EAX,0x2 ; JNZ 0x88C05   ; phase == 2
0x88B64 MOV EAX,0x1e ; CALL 0x000974dc
0x88B6E MOV EAX,[0x001577ca] ; MOV EAX,[EAX] ; MOV DL,[EAX+0x826]
0x88B7B XOR EBX,EBX ; XOR DL,0x1 ; AND EDX,0xff                          ; side = ball-team ^ 1
0x88B86 MOV AL,[EAX+0x826] ; AND EAX,0xff ; CMP EAX,0x1 ; SETZ AL        ; (ball-team == 1)
0x88B99 MOV ESI,[0x00157784] ; TEST ESI,ESI ; SETL AL                    ; (snap z < 0)
0x88BAF XOR EAX,ECX ; SETZ AL
0x88BB9 ADD EAX,0x3 ; CWDE
0x88BBD CALL 0x0008a938            ; FUN_0008A938(3 + cond, side, BX=0)
```

So the scanner emits:
* **sit 2** (throw-in) → table-2 → **phase 3** → row **0x10**;
* **sit 3** (corner) → table-2 → **phase 4** → row **0x11**;
* **sit 4** (goal-kick) → table-2 → **phase 8** → keeper rows 0x1D/0x1E (not a taker);
* the goal arm (sit 6) → **phase 5** (celebration).

The situation→phase map is the ported `fifa96_action_phase_situations` table
(`src/fifa96_loader/fifa96_action_handlers.c:1129`): `[2]={phase 3}`, `[3]={phase 4}`,
`[4]={phase 8}`, `[6]={phase 5}`, `[9]={act 2 stage 0}`, `[0xA]={act 2 stage 1}`,
`[0xB]={phase 2}`, `[0xC]={act 9}`.

### 2.3 The phase arm — the **sole** raiser of the taker installs

Fresh `get_xrefs_to 0x8D098` = **2**, both inside `FUN_000740A0` (`0x740C8`,
`0x740DB`). Fresh `get_xrefs_to 0x740A0` = **27** callers. So a taker code
`0x10/0x11/0x12/0x13` can be installed **only** through
`FUN_000740A0(phase, side) → FUN_0008D098(team, …)`.

Fresh `disassemble_function 0x8D098`; the dispatch is
`0x8D178 MOV AL,[0x157A4D] ; CMP AL,0x15 ; JA 0x8D814 ;
JMP dword[EAX*4 + 0x8D040]`. Fresh `read_memory 0x8D040` (22 dwords) decodes the
phase→arm mapping:

| phase | arm | installs |
|---|---|---|
| 3 | `0x8D2A3` | **0x10** (`0x8D33C MOV EDX,0x10 ; CALL 0x7D9A4`) |
| 4 | `0x8D35B` | **0x11** (`0x8D412 MOV EDX,0x11`) |
| 5 | `0x8D433` | 0x15/0x14 celebration |
| 6 | `0x8D4A2` | **0x13** (`0x8D532 MOV EDX,0x13`) / non-controlled **0x1F** (`0x8D558`) |
| 7 | `0x8D57B` | **0x12** (`0x8D5B8 MOV EDX,0x12`) |

`FUN_000740A0` (fresh `0x740A0..0x7410D`) writes the phase, stores the side byte
at `[0x157AAF]` (= `dword[0x157AAC]>>24`, the arm's controlled-side gate
`0x8D1E7`), runs `FUN_0008D098` for **both** teams, and then — only when the new
phase is 2 — clears `goal_armed`:

```
0x740AC MOV [0x00157a4d],AL              ; write the new phase
0x740BA MOV byte ptr [0x00157aaf],DL     ; the arm's controlled-side gate byte
0x740C8 CALL 0x0008d098                  ; team = side
0x740DB CALL 0x0008d098                  ; team = side ^ 1
0x740E0 MOV EAX,[0x00157a4a] ; SAR EAX,0x18 ; CMP EAX,0x2 ; JNZ 0x7410D
0x740F6 MOV byte ptr [0x0015781d],DL     ; goal_armed = 0 on a phase-2 write
```

### 2.4 `goal_armed [0x15781D]` — the scanner's live gate producer/clear

Fresh `get_xrefs_to 0x15781D` = **11**:

```
0x7137D  READ         FUN_0007131C   (already-armed check)
0x713DB  WRITE        FUN_0007131C   (= 1, arm)
0x71908  WRITE        FUN_0007131C   (= 0, the reflect/decline clear)
0x72AD3  READ         FUN_00072AC4
0x740F6  WRITE        FUN_000740A0   (= 0, phase-2 clear)
0x8B635  READ         FUN_0008AF38   (the scanner gate)
0x84F90  WRITE        row 0x20 (0x84EEC)
0x89A7A  READ
0x93C87  WRITE        restart handler
0x9437A  WRITE        restart handler
0x9453C  WRITE        restart handler
```

**Producer `FUN_0007131C`** (fresh `disassemble_function`; the camera-track
boundary handler the engine calls from the frame body when the camera is out of
bounds). Arm path (fresh bytes):

```
0x7137D MOV BL,byte[0x15781D] ; TEST BL,BL ; JNZ 0x718A9    ; already armed -> decline
0x71390 MOV EAX,[0x157A4A] ; SAR EAX,0x18
0x71398 CMP EAX,0x2 ; JZ 0x713A6
0x7139D CMP EAX,0x10 ; JNZ 0x718A9                          ; phase ∈ {2,0x10}
0x713A6 |camZ| vs 0xB20 ; 0x713B4 |camX| vs 0x730
0x713C0 JLE 0x718A9                                          ; need |z|>0xB20 || |x|>0x730
0x713DB MOV byte ptr [0x0015781d],BH(1)                      ; ARM
0x713E3..0x713E5 MOVSD 0x15774C -> 0x15777C                  ; the goal snapshot triple
0x713EC MOV dword[0x00157780],ECX(0)                         ; snapshot y = 0
0x713F2 CALL 0x00070074 ; 0x713F7 MOV [0x0015781e],AL        ; the zone classifier -> [0x15781E]
```

So `goal_armed` is raised **only** when the camera leaves the field bounds
(`|x|>0x6C0 || |z|>0xAB0`, per the engine's `fifa96_camera_out_of_bounds`,
`fifa96_camera.c:187`) **while phase ∈ {2,0x10}**, and it is cleared on any
phase-2 write (`FUN_000740A0 0x740F6`). Native owner: `FUN_0007131C`.

### 2.5 The foul adjudicator — the native act-2 stage-4 terminal

The engine's `match_run_ref_step_restart` (`fifa96_match_run.c:5956`) writes
**phase 7 or 6** and arms (`:5996`/`:5998`). Its native owner is the act-2 body
`0x8922C` (fresh `disassemble_bytes 0x89200`). The stage dispatch is
`0x892FE MOV AL,[0x158829] ; CMP AL,5 ; JA 0x89615 ;
JMP dword[EAX*4 + 0x89214]`. Fresh `read_memory 0x89214` (6 dwords) =
`{0x89318, 0x8938E, 0x893BC, 0x894A2, 0x894DB, 0x895CC}`.

**Stage 4 `0x894DB` — the phase 7/6 decision (fresh bytes):**

```
0x894DB MOV AL,[0x15888C] ; CMP EAX,0x3 ; JNZ 0x894F2       ; contact kind
0x894E7 MOV word ptr [ESP],0x7 ; JMP 0x89556                ; kind == 3 -> phase 7
0x894F2 MOV word ptr [ESP],0x7                             ; default phase 7
0x894F8 MOV EDI,[0x158897]                                  ; incident x
0x89503 TEST EDI,EDI ; ... NEG                              ; |incident x|
0x8950F CMP EDX,0x420 ; JGE 0x89556                         ; >= 0x420 -> phase 7
0x89517 MOV EDX,[0x15888F] ; MOV EDX,[EDX]                  ; rec_first -> team
0x8951F CMP byte[EDX+0x826],0x0 ; JNZ 0x8953D               ; fouler-side band
0x89528 MOV EBX,[EAX+0x8]
0x8952B CMP EBX,-0xB10 ; JL 0x89556
0x89533 CMP EBX,-0x7B0 ; JG 0x89556
0x8953B JMP 0x89550                                         ; in band -> phase 6
0x8953D MOV EBP,[EAX+0x8] ; CMP EBP,0x7B0 ; JL 0x89556 ; CMP EBP,0xB10 ; JG 0x89556
0x89550 MOV word ptr [ESP],0x6                             ; side-1 band -> phase 6
0x89556 MOV EAX,[ESP-2] ; ... MOV byte[0x15882A],0         ; clear staging
0x8956A MOV EBX,4 ; MOV EAX,0x23 ; CALL 0x8f188            ; speech 0x23 (phase 6)
0x8957B MOV EBX,4 ; MOV EAX,0x2a ; CALL 0x8f188            ; speech 0x2A (phase 7)
0x89590 MOV EAX,[0x15888F] ; MOV EAX,[EAX] ; MOV AL,[EAX+0x826] ; XOR AL,1
0x895A3 XOR DL,AL                                          ; side = fouled side
0x895AC CALL 0x000740a0                                    ; FUN_000740A0(phase, side)
```

**`0x895AC CALL 0x000740A0` is the live arm for rows `0x12`/`0x13`** — it writes
phase 7 (→ install 0x12) or phase 6 (→ install 0x13) and runs the per-team arm.
This is a **fresh, first-hand** confirmation of the engine's
`match_run_ref_step_restart` shape (`fifa96_match_run.c:5956-5999`): the band
`|incident x| < 0x420` + `z ∈ [-0xB10,-0x7B0]` (side 0) / `[0x7B0,0xB10]` (side 1)
→ phase 6, else phase 7; `[0x15888C]` (contact kind) == 3 short-circuits to
phase 7. Native owner: `0x8922C` (act-2), phase-0x18 slot.

**The entry to act 2 (the foul chain, fresh engine + fresh native):**
row 0x0C `match_sp_action_0c_foul` (`fifa96_match_handlers.c:3329`) →
`fifa96_match_run_contact` (`fifa96_match_run.c:6489`) → `fifa96_ref_foul_decide`
(`fifa96_referee.c:45`) → act 3 invoke-now (`run.c:6530`) → phase 0x19 foul
sequence (`fifa96_ref_foul_sequence_step`) → stage 6 dispatches sit 0xA
(`fifa96_referee.c:303`) → `match_run_ref_apply` → `set_piece(0xA, side, 1)`
(`run.c:5907`) → act-2 stage 1 → the stage-4 terminal above. The other branch
(`foul_kind == 0`) dispatches sit 9 → act-2 stage 0 → the same terminal. The
native sit-0xA / sit-9 arms are the table-2 rows (FU-267 §0).

**The settings gate is OPEN live.** `fifa96_settings_defaults`
(`fifa96_settings.c:14`) sets `value[0x0A] = 2`; the begin handoff
(`fifa96_settings_handoff`, `:72`) runs with `match_type 0` (not competition), so
`config.field_4c306 = 2` — fouls **enabled at level 2** (the `match_run_contact`
gate `field_4c306 == 0` is therefore **not** taken; the severity block
`field_4c306 > 1` runs). Offside is `value[0x10] = 0` → `field_4c2f2 = 0`
(offside **disabled**). *(Erratum to the `run.c:2110` comment "fouls disabled":
the seeded `config` is zeroed there, but `begin` installs the derived defaults,
`run.c:2250-2253`; the live `field_4c306` is 2.)*

---

## Step 3 — live vs fixture (fresh engine + fresh census)

| row | install owner (native) | producer raiser | tape status (fresh test source) | ruling |
|---|---|---|---|---|
| **0x11 corner taker** | phase-4 arm `0x8D35B`/`0x8D412` | the derived FU-170 post-goal restart wire (sit 3 → phase 4) | live: `tests/test_engine_m2.c:5087` `assert(res.mask_final == (M2_WIRED_MASK \| M2_RESTART_ROWS_MASK))`, `M2_RESTART_ROWS_MASK = 1<<0x11` (`:1902`) | **LIVE** |
| **0x10 throw-in taker** | phase-3 arm `0x8D2A3`/`0x8D33C` | the throw-in scanner `FUN_00088940 0x88C00` → `set_piece(2, ball^1, 1)` (`run.c:3300`) | absent: `assert((mr->dispatched_ok & M2_TAKER_ROWS_MASK) == 0u)` at m 41 (`:2480`) and m 62 (`:2859`); `M2_TAKER_ROWS_MASK = {0x10,0x11,0x12,0x13}` (`:1896`) | **fixture-only on tape; live-arm present** |
| **0x12 free-kick taker** | phase-7 arm `0x8D57B`/`0x8D5C7` | the foul adjudicator `0x895AC` → `FUN_000740A0(7, side)` | absent (same assertion) | **LIVE-LEG** (FU271 §4.1 / FU268-L1) |
| **0x13 penalty taker** | phase-6 arm `0x8D4A2`/`0x8D532` | the foul adjudicator `0x895AC` → `FUN_000740A0(6, side)` | absent (same assertion) | **LIVE-LEG** |

**Why row 0x10 stays fixture-only on the tape.** The scanner *is* wired to the
frame body (`fifa96_match_run_phase_drive` `:4213` → `fifa96_match_run_goal_scan`
`:4243`; `fifa96_match_goal_arm` called at `:4179` when
`fifa96_camera_out_of_bounds`), and the armer *does* fire on the tape (the natural
goal drives the camera out of bounds in x/z). But the throw-in arm needs
`phase == 2` **and** `|snap z| <= 0xB20`: on the M2 tape the armed camera sits
beyond the goal line (`|snap z| > 0xB20` → the goal arm, sit 6), and the
post-goal hold parks phase 0, so the phase-2 gate is closed. The
`M2_TAKER_ROWS_MASK` comment (`:1889-1895`) states the same. The **live arm
exists and is faithful**; the row needs a live **phase-2 sideline excursion**
(`|camX| > 0x730`, `|snap z| <= 0xB20`).

**Why rows 0x12/0x13 stay fixture-only.** The whole chain is ported and the
settings gate is open, but its **entry** — the row-0x0C contact (`0x855F0`'s
sibling action row `0x0C`, the only `fifa96_match_run_contact` caller,
`fifa96_match_handlers.c:3329`) — is **not dispatched** on the M2 tape
(`M2_WIRED_MASK` at `:1876` lacks `0x0C`; rows 0x0C/0x0D/0x11 are "wired but
dormant" per FU-271). FU271 §4.1 already labels the foul chain **LIVE-LEG**;
this slice's fresh native read of `0x895AC` confirms the terminal that would
install 0x12/0x13 once the contact gate fires.

**Net.** One taker row is live (`0x11`); the two producer raisers that would
dispatch `0x10` (throw-in scanner) and `0x12`/`0x13` (foul adjudicator) are both
**wired and faithful** but their **triggers** (a phase-2 sideline camera excursion
/ a row-0x0C contact duel) never fire on the M2 tape. No new row is wired by this
ruling; the census does not move.

---

## Step 4 — the port shapes + the live-trigger ruling (hand-off)

### 4.1 The port shapes

1. **Throw-in scanner (row 0x10 arm).** Already landed: `fifa96_match_run_goal_scan`
   (`run.c:3274`) reproduces the `FUN_00088940` arms — throw-in sit 2 with BX=1
   (`:3300`), corner sit `3+cond` with BX=0 (`:3316`), the goal arm (`:3344`). The
   phase/armed gate (`:3284-3285`), the `|snap z| <= 0xB20` split (`:3293`) and
   the `goal_zone == 0` split (`:3305`) match the fresh native bytes. **No port
   work.** The only modelled-away piece is the scanner's `0x974DC(0x1E)` sound
   (FU266-L5 leg).
2. **Foul adjudicator (rows 0x12/0x13 arms).** Already landed:
   `match_run_ref_step_restart` (`run.c:5956`) reproduces the `0x8922C` stage-4
   terminal exactly (band → phase 6/7, the `[0x15888C]==3` short-circuit, the
   speech 0x23/0x2A, the `FUN_000740A0(phase, side)` arm). **No port work.** The
   native owner is `0x8922C` (phase-0x18 slot); the entry is the row-0x0C contact.
3. **`goal_armed` armer.** Already landed: `fifa96_match_goal_arm` (`run.c:2982`)
   reproduces `FUN_0007131C`'s arm gate (`phase ∈ {2,0x10}` + the out-of-bounds
   test), the snapshot copy and the zone call; the clear is the phase-2 write
   (`FUN_000740A0 0x740F6`). **No port work.**
4. **Row-0x10 re-queue gate (L266-8).** Not unified: the row reads
   `mr->sp_157821`, the native producer is `fifa96_camera.pan_counter` at the
   same address. **Port fix** (do not blind-wire): unify on `pan_counter` *or*
   document the shadow; pin with a fixture before touching the tape.
5. **The remaining gap is a TRIGGER, not a body.** Rows 0x10/0x12/0x13 need a
   live path that drives `goal_armed && phase==2 && |snap z| <= 0xB20` (throw-in)
   or a row-0x0C contact (foul). Those are the W26b-4 contact-gate slice and the
   W26b-8 live-trigger census — **not** this slice's owner.

### 4.2 Where this hands off

* **W26b-4 / FU276 (the contact gate):** the foul-adjudicator entry is the
  row-0x0C contact duel (`fifa96_match_run_contact`); FU276 owns the contact
  invoker. Rows 0x12/0x13 ride that verdict.
* **W26b-7 / FU279 (situation-0xB producers):** the six dormant taker/keeper
  row tails; the `0x8A938(0xB)` call itself is live-landed.
* **W26b-8 / FU280 (live-trigger census):** the phase-2 sideline throw-in
  precondition and the row-0x0C contact precondition.
* **W26b-3 / FU275 (taker-arm residuals):** the `[0x157821]` shadow (L266-8/L266-13)
  and the `FUN_000832A8`/`FUN_00085498` staging depth.

---

## Numbered open legs (native owners)

1. **W26b-L1 — the throw-in precondition (`goal_armed && phase==2 && |snap z| <= 0xB20`).**
   Which live path drives the camera out of bounds on the **sideline** (`|camX| > 0x730`,
   `|snap z| <= 0xB20`) in phase 2? The M2 tape arms only the goal arm. Native
   owners: `FUN_0007131C` (armer), `FUN_0008AF38` (gate), `FUN_00088940 0x88BCC`
   (arm). Engine: `fifa96_match_goal_arm`, `fifa96_match_run_goal_scan :3300`.
2. **W26b-L2 — the foul-adjudicator entry (the row-0x0C contact gate).** No live
   engine path dispatches row 0x0C; `fifa96_match_run_contact` is its sole engine
   caller. Native owner: row 0x0C (`0x81DFE..`); engine `run.c:6489`. Cross-task:
   W26b-4/FU276.
3. **W26b-L3 — the phase-6/7 band discriminator source.** `[0x15888C]` (contact
   kind) and `[0x158897]` (incident) feed the `0x894DB` band; their registrars
   (`FUN_0008A3FC`/`FUN_0008A43C`) are unported producers. Native owner: `0x8922C`
   stage 4 + the registrar. Engine: `match_run_ref_step_restart :5981-5991`.
4. **W26b-L4 — `[0x157821]` shadow cell (carry L266-8/L266-13).** The row 0x10
   gate reads `sp_157821`; the native producer is `fifa96_camera.pan_counter` at
   the same address. Native owners: `FUN_000700F4`/`FUN_000709D0`; port owns
   `fifa96_camera.c`.
5. **W26b-L5 — `[0x157A6A]` reset owner (carry L266-11).** `FUN_00073E28` is the
   match-reset helper; whether it is the sole reset in the live loop is
   unexercised (the row arms are the only other writers). Native owner:
   `FUN_00073E28`.

---

## Corrections / errata (quoted, not rewritten)

1. **`[0x157AAC]` is *not* an unwritten gate.** FU271 §1 table row 11 records
   "`[0x157AAC]` … **no store anywhere** (fresh operand search `157aac` = 27
   `MOV r,[..]`)" and FU271 §3 calls it a shared arm gate "value seeded 0; **no
   writer**". The arm's controlled-side gate byte is the **alias**
   `dword[0x157AAC]>>24` = `byte[0x157AAF]`, which fresh `FUN_000740A0 0x740BA`
   **writes** (`MOV byte [0x157AAF],DL`, the `side` parameter). The 27 reads of
   the dword are the arms' `MOV EDX,[0x157AAC] ; SAR EDX,0x18` idiom. So the arm
   gate is written by every live phase write (`FUN_000740A0`), and the engine's
   `phase_machine.side_controlled` (written at `run.c:5889/5962/5995/6205`) is a
   faithful model. The "no writer" claim concerns the low byte only. (Fresh
   `read_memory 0x157AAC` (8 B) = all zero — the BSS default.)
2. **"the foul producer is FU-150/P2 (unported)" (FU-266 §4.3) is stale.** The
   FU-150 P2 chain *is* ported (`fifa96_referee.c`, `match_run_ref_step_restart`,
   `match_run_contact`); the settings gate is **open** live (`field_4c306 = 2`).
   The correct residual is the **entry** (row-0x0C contact, W26b-L2) — a
   trigger/liveness gap, not an unported producer. FU271 §4.1 already renders
   this as LIVE-LEG (FU268-L1).
3. **`run.c:2110` comment "fouls disabled".** `fifa96_match_run_init` zeroes
   `mr->config` (`run.c:2110`), but `fifa96_match_run_begin` installs the derived
   settings defaults (`run.c:2250-2253`, `fifa96_settings_defaults` →
   `value[0x0A]=2`), so the live `field_4c306` is **2** (fouls enabled). The
   comment describes the *pre-begin* state.

---

## Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled;
`.image` overlay present; `get_current_program_info` confirmed this session):

* `disassemble_function`: `0x700F4` (133 insns), `0x709D0` (130), `0x88940` (205),
  `0x8D098` (583), `0x7131C` (571), `0x73E28` (51), `0x740A0` (25).
* `disassemble_bytes`: `0x8B5F0` (96 B, the scanner gate), `0x89200` (320 B, the
  act-2 head + stage 0), `0x894A0` (512 B, stages 3/4/5 + the phase 7/6 terminal).
* `read_memory`: `0x8D040` (92 B, the phase→arm jump table), `0x89214` (24 B, the
  act-2 stage table), `0x157AAC` (8 B).
* `get_xrefs_to`: `0x157821` (11), `0x157A6A` (6), `0x15781D` (11), `0x88940` (1),
  `0x8D098` (2), `0x740A0` (27), `0x832A8` (6), `0x85498` (3), `0x8A938` (39).
* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, 2706
  functions, image base 0, `.image` overlay present.

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(`:2982` goal arm, `:3274` goal scan, `:5956-6013` `match_run_ref_step_restart`,
`:4177-4179` the arm call, `:4243` the scan call, `:6489` `match_run_contact`,
`:5878-5911` `match_run_ref_apply`, `:2087`/`:2219` the `sp_157821` seeds);
`fifa96_match_handlers.c` (`:1724` row-0x10 gate, `:1553-1558` `match_sp_resolve`,
`:1609/1756/1897/2038` rows 0x10..0x13, `:3302` the row-0x0C foul branch,
`:5478/5593` the row-0x0C registration); `fifa96_action_handlers.c`
(`:1129` the situation→phase table); `src/fifa96_loader/fifa96_camera.c`
(`:187` `out_of_bounds`, `:863`/`:893` `pan_counter`); `fifa96_settings.c`
(`:14` defaults, `:85-86` the config handoff); `fifa96_referee.c`
(`:45` `foul_decide`, `:233` the foul sequence, `:99` the sit-9 branch).
Tests read first-hand: `tests/test_engine_m2.c` (`:1876` `M2_WIRED_MASK`, `:1896`
`M2_TAKER_ROWS_MASK`, `:1902` `M2_RESTART_ROWS_MASK`, `:1889-1895` the taker
dormancy comment, `:2480`/`:2859` the absence asserts, `:5087` the restart-mask
assert). Docs cross-cited: FU-149 §7/L13, FU-150, FU-266 (L266-1..14), FU-267,
FU-271 (§1.4/§3/§4.1), FU-272 (R11-L4), `docs/ENGINE.md:8` (59/80). No writes
beyond this draft + the run report; no rename/label/comment/function/script/
project save; no git; the ISO never touched.
