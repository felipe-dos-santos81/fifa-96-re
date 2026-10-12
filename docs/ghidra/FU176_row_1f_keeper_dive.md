# FU-176 — row 0x1F keeper dive/arm body (phase-15 wave R24)

Provenance: M2 phase-15 wave **R24**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument). Every address/byte claim below is a fresh `read_memory` /
`disassemble_bytes` / `get_xrefs_to` of this session unless marked *cross-cited*
(then the frozen FU doc is named). Engine/loader source read first-hand
(`fifa96_match_handlers.c`, `fifa96_match_run.c`, `fifa96_match_entities.c`,
`fifa96_loader/fifa96_keeper.c`, `include/fifa96_loader/fifa96_keeper.h`,
`include/fifa96_engine/fifa96_match_run.h`). No writes: no rename, comment,
label, function, script or project save; `/FIFA96.EXE` untouched; no git
commands. Alias traps applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at
`x+3`).

Scope: the row-0x1F handler `0x76380..0x765C2` byte-level (4 stages), every
record/global cell it writes, its `[rec+0x20]`/team-merge prologue gate, the one
phase read the body performs, the four-arm stage dispatch, the second-order
**0x1B install** at `0x76550` and its live phase-6 installer (`0x8D4A2` /
`0x8D558`); the wiring of the ported `fifa96_keeper_dive_target` /
`_arm_step` / `_arm_camera` / `_input_decide` onto the body; and the minimal
live port (T2 wiring rec).

---

## 1. Fresh table + handler re-verification

* **Action table `0x1106E0[0x1F] = 0x76380`** — fresh `get_xrefs_to 0x76380`
  returns **1** reference: `DATA` at `0x0011075C`. `0x1106E0 + 0x1F*4 =
  0x1106E0 + 0x7C = 0x11075C` (the row-0x1F slot, contiguity first-hand). This
  matches FU-140 §1 (`0x1F 0x076380`) and FU-174 §1 (`[0x1F]=0x76380`).
* **Stage dispatch table `0x76370`** — fresh `read_memory 0x76370` (16 B) =
  `3a640700 7e640700 b0640700 80650700` =
  **`{0x7643A, 0x7647E, 0x764B0, 0x76568}`** (4 dwords, stages 0..3). The
  `JMP dword ptr CS:[EAX*4 + 0x76370]` at `0x76432` (fresh bytes
  `2eff248570630700`) indexes it with `EAX = byte[+0x92] & 0xFF` (stages 0..3;
  stage > 3 exits at `0x76425 CMP AL,3 / JA 0x765BC`). Confirms the FU-136 §2 /
  FU-140 §2 "`0x76370` 4 arms".
* **Installer reader `0x7D9A4`** — fresh `get_xrefs_to` = **77** call sites
  (matches FU-174 §1). The row-1F no-slot dive install at `0x7655C` is one of
  them (fresh `e8 43 74 00 00`).
* **Input handler `0x76130`** — fresh `get_xrefs_to` = **4**:
  `FUN_000782d0` (`0x7845D` computed call), the input-row tables
  `0x110954`/`0x11097C` (DATA), and **`0x76599`** (the row-1F tail call, the
  only direct instruction caller in the row). Matches FU-140 §3.1 / FU-151.

Engine state (source, first-hand): the action table row is a NULL stub —
`fifa96_match_handlers.c:3457` `{0x1F, NULL, "FU-140 §2/§3: row 1F not ported
(partial); keeper_dive_target/arm_step + input_decide; OL-36"}` — and the four
helpers are ported+tested but referenced only by `tests/test_keeper_bodies.c`
(no handler wires them).

---

## 2. Byte-level body `0x76380..0x765C2`

Fresh `disassemble_bytes 0x76370` (672 B; 199 insns, window covers the body and
the following `FUN_000765C4`). Register convention: **EAX = rec** (stored to
`EBP` at `0x76386`), so every `[EBP+n]` below is the native record byte/word/
dword at `+n`. `[rec+0]` is the team back-pointer (`0x7638E MOV EAX,[EAX]`).

### 2.1 Prologue + prologue gates (`0x76380..0x763B0`)

```
0x76380 PUSH EBX/ECX/EDX/ESI/EDI/EBP ; MOV EBP,EAX          ; save, EBP = rec
0x76388 CMP dword[EBP+0x20],0 ; JNZ 0x763A0                 ; slot gate
0x7638E MOV EAX,[EBP+0]                                     ; team = rec->team
0x76390 CMP byte[EAX+0x828],0 ; JZ 0x763A0                  ; team slot-pool gate
0x76399 MOV EAX,EBP ; CALL 0x7876C                          ; FUN_0007876C(rec)
0x763A0 MOV EAX,[EBP+0x8F] ; SAR EAX,0x18                   ; stage = byte[+0x92]
0x763A6 XOR DH,DH ; MOV byte[0x00157C5C],DH                 ; [0x157C5C]=0
```

* Gate (matches FU-140 §2 row 1F): `[rec+0x20]==0 && byte[team+0x828]!=0` →
  `FUN_0007876C(rec)` (slot merge). Note the alias read: `[EBP+0x8F]>>24` is the
  **stage byte at `+0x92`** (confirmed again at `0x76419` where the body reads
  the bare `byte[EBP+0x92]`).
* Global write: **`[0x157C5C]=0`** (the cell the input-decide port calls
  `clear_c5c`).

### 2.2 Prologue camera target (`0x763B1..0x76409`)

```
0x763B1 CMP EAX,2 ; JGE 0x76409                 ; stage >= 2: leave target
0x763B6 MOV dword[EBP+0x4D],0                   ; target.x = 0
0x763C0 MOV dword[EBP+0x51],0                   ; target.y = 0
0x763C7 CMP byte[team+0x826],0 ; JNZ 0x763DD     ; team side != 0 -> +0xB10
0x763D0 EAX=[0x157A4A]; SAR 0x18; CMP 0x10; JNZ 0x763E4  ; phase==0x10 -> +0xB10
0x763DD EAX=0xB10 ; JMP 0x763E9
0x763E4 EAX=0xFFFFF4F0                           ; -0xB10
0x763E9 MOV dword[EBP+0x55],EAX                  ; target.z
0x763EC EAX=[EBP+0x8F]; SAR 0x18; CMP 1; JGE 0x76409   ; stage >= 1: no hook
0x763FA LEA EDX,[EBP+0x4D] ; MOV EBX,0x15774C ; MOV EAX,EBP ; CALL 0x79B6C
```

* `stage < 2` → `target = (0, 0, (team_side != 0 || phase == 0x10) ? 0xB10 :
  -0xB10)`; `stage < 1` → `FUN_00079B6C(rec, 0x15774C, &target)` camera hook.
  This is exactly the ported `fifa96_keeper_arm_camera(stage, team_side, phase,
  &target, &camera_hook)` (FU-140 §3.3).
* The **only phase read in the row body** is here: `[0x157A4A]>>24 == 0x10`
  (`0x763D0..0x763DB`). There is **no top-level phase gate** at the row entry
  (unlike rows 1A/1B, which gate on `phase == 0xD`); the row runs whenever it is
  installed, and it is installed live by the phase-6 arm (§4).

### 2.3 Common timer + stage dispatch (`0x76409..0x76438`)

```
0x76409 XOR EAX,EAX ; MOV EBX,dword[EBP+0x89]   ; timer
0x76411 MOV AX,word[0x157A64]                   ; delta (word)
0x76417 ADD EBX,EAX
0x76419 MOV AL,byte[EBP+0x92]                   ; stage
0x7641F MOV dword[EBP+0x89],EBX                 ; timer += delta  (every stage)
0x76425 CMP AL,3 ; JA 0x765BC                   ; stage > 3 -> epilogue
0x7642D AND EAX,0xFF
0x76432 JMP dword CS:[EAX*4 + 0x76370]          ; 4-arm dispatch
```

The `+0x89` action timer accrues `[0x157A64]` on **every** entry (the FU-151
row-1E convention).

### 2.4 Stage 0 (`0x7643A`) — event arm

Fresh window `0x7643A..0x7647D`:

```
0x7643A MOV BH,byte[0x15882A] ; MOV byte[EBP+0x9E],1 ; TEST BH,BH ; JZ 0x765BC
0x7644F MOV EDX,0x28 ; MOV EAX,EBP ; MOV ECX,[EBP+0x8B] ; XOR EBX,EBX
0x7645E SAR ECX,0x18 ; CALL 0x6E598              ; FUN_0006E598(rec, 0x28, sector, 0)
0x76466 MOV CL,byte[EBP+0x92] ; MOV dword[EBP+0x89],0 ; INC CL ; MOV byte[EBP+0x92],CL
```

* Writes `[rec+0x9E]=1`; if `[0x15882A]==0` → epilogue (no advance).
* `[0x15882A] != 0` → event **0x28** via `FUN_0006E598(rec, 0x28, sector, 0)`;
  the `ECX` arg is `[EBP+0x8B]>>24` = the **sector byte at `+0x8E`** (FU-151
  erratum 1), `EBX=0`. Then `timer=0; stage++` (2→? here 0→1), falling into
  stage 1. Matches `fifa96_keeper_arm_step`: `flag_9e=1`, `event_code=0x28`,
  `stage=1` (event_flag ← `[0x15882A]`).

### 2.5 Stage 1 (`0x7647E`) — ball-ack gate

Fresh window `0x7647E..0x764AF`:

```
0x7647E CMP dword[0x158730],0 ; JZ 0x765BC        ; ball actor present
0x7648B CMP byte[0x158746],0 ; JZ 0x765BC         ; pair ack
0x76498 MOV AL,byte[EBP+0x92] ; MOV dword[EBP+0x89],0 ; INC AL ; MOV byte[EBP+0x92],AL
```

* Gate: `[0x158730] != 0 && byte[0x158746] != 0` (the ball staging actor and its
  ack, FU-139 §2 / FU-148 §/`0x7A2F0`); else epilogue with no advance.
* Pass → `timer=0; stage++`, falling into stage 2. Matches
  `fifa96_keeper_arm_step` `ball_actor`/`ball_flag` gate.

### 2.6 Stage 2 (`0x764B0`) — slot branch and the no-slot RNG dive

```
0x764B0 CMP dword[EBP+0x20],0 ; JNZ 0x76568       ; slot -> stage-3 tail
```

**No-slot arm (`0x764BA..0x76567`) — the RNG dive, the second-order 0x1B**
**install.** Fresh window:

```
0x764BA LEA EDI,[EBP+0x4D] ; LEA ESI,[EBP+0x59] ; MOVSD x3   ; target = pos
0x764C3 MOV EAX,[EBP+4]                                     ; desc = dword[rec+4]
0x764C6 MOV EDX,[EAX+0x10] ; 0x764C9 MOV EAX,[EAX+0xF]
0x764CC SAR EDX,0x18 ; 0x764CF SAR EAX,0x18                 ; signed bytes desc+0x13, desc+0x12
0x764D2 ADD EAX,EDX ; 0x764D9 ADD EAX,EAX                   ; 2*(b12+b13)
0x764D4 MOV ECX,0x64 ; SUB ECX,EAX                          ; divisor = 0x64 - 2*sum
0x764DF CALL 0x92AC8 ; DIV ECX ; CMP EDX,0x19 ; JNC 0x764FC ; rng_a % divisor < 0x19?
   ; < 0x19: 0x764EB CMP word[0x15873A],0 ; SETG AL ; ... sign (trajectory_z>0)
   ; >= 0x19: 0x764FC CALL 0x92AC8 ; ... AND DL,1             ; sign = rng_b & 1
0x76508 TEST DX,DX ; JNZ 0x7650E ; DEC EDX                  ; sign in {+1,-1}
0x7650E MOV EAX,[EBP+0x61] ; LEA ECX,[EBP+0x55] ; TEST EAX,EAX ; JLE 0x7651F
0x76518 EAX=0xFFFFFFD0 ; JMP 0x76524                        ; pos_z > 0 -> -0x30
0x7651F EAX=0x30                                            ; else +0x30
0x76524 ADD [ECX],EAX                                       ; target.z = pos_z + (±0x30)
0x76526 CALL 0x92AC8 ; AND EAX,0x3F ; MOVSX EDX,DX ; ADD EAX,0x30 ; IMUL EDX,EAX
0x7653C MOV dword[EBP+0x4D],EDX                             ; target.x = sign*((rng_c&0x3F)+0x30)
0x76537 MOV ECX,0x30 ; CALL 0x92AC8 ; DIV ECX ; ADD EDX,0x20
0x76548 MOV ECX,1
0x76550 MOV EAX,EBP ; MOV EBX,ECX ; MOV dword[EBP+0x51],EDX  ; target.y = rng_d%0x30+0x20
0x76557 MOV EDX,0x1B
0x7655C CALL 0x7D9A4                                        ; install 0x1B, BL=1, ECX=1
0x76561..0x76567 POP x5 ; RET
```

* `target := pos` (3 MOVSD), then `target.z = pos_z + (pos_z > 0 ? -0x30 :
  0x30)`, `target.x = sign*((rng_c & 0x3F)+0x30)`,
  `target.y = (rng_d % 0x30)+0x20`, with `sign = (rng_a % divisor < 0x19) ?
  (trajectory_z > 0 ? +1 : -1) : ((rng_b & 1) ? +1 : -1)`,
  `trajectory_z = word[0x15873A]` (`word` at `+0x15873A`).
* The `divisor` reads `dword[rec+4]` then the dwords at `desc+0xF`/`desc+0x10`
  arithmetic-shifted `0x18`: by the alias rule these are the **signed bytes at
  `desc+0x12` and `desc+0x13`**, i.e. `divisor = 0x64 - 2*(int8)desc[0x12] -
  2*(int8)desc[0x13]` (the roster/animation descriptor; its layout is not
  modeled in the pool — leg FU176-L4). `divisor` is consumed unsigned by
  `DIV`; the port takes it as the `divisor` argument and returns `-INVALID` on
  0 where the native faults.
* **The second-order install**: `0x76557 MOV EDX,0x1B` + `0x7655C CALL
  0x7D9A4` with `EAX=rec`, `EBX=ECX=1`. Per FU-137 §2 the installer contract is
  `EAX=rec, DX=code, BL=staged, ECX=invoke-now` (fresh `0x7D9A9..0x7D9B4`), so
  this is **install code 0x1B, staged byte 1, invoke-now**. The `0x76550`
  address in the brief is the sequence head (`MOV EAX,EBP`); the `MOV
  EDX,0x1B`/`CALL` sit at `0x76557`/`0x7655C`.
* The whole no-slot arm is a faithful extraction of
  `fifa96_keeper_dive_target(pos, rng_a, divisor, rng_b, trajectory_z, rng_c,
  rng_d, out)` → `out.install_code = 0x1B, out.invoke = 1` (FU-140 §3.4),
  **except** the port takes `divisor`/`trajectory_z` as arguments (the native
  reads `[rec+4]` and `[0x15873A]`).

### 2.7 Stage 2-slot / stage 3 tail (`0x76568`) — commit + input handler

Both the `[rec+0x20] != 0` branch (from `0x764B4`) and the direct stage-3
dispatch entry (`0x76370[3] = 0x76568`) run this tail:

```
0x76568 MOV AH,byte[EBP+0x92] ; MOV dword[EBP+0x89],0 ; INC AH ; MOV byte[EBP+0x92],AH
0x76580 LEA EDI,[EBP+0x4D] ; LEA ESI,[EBP+0x59] ; MOVSD x3    ; target = pos
0x76589 MOV EDI,dword[EBP+0x20] ; TEST EDI,EDI ; JZ 0x7659E
0x76590 CMP word[EDI+4],0 ; JZ 0x7659E                      ; slot button
0x76597 MOV EAX,EBP ; CALL 0x76130                          ; FUN_00076130(rec)
0x7659E MOV EAX,[EBP+0x8E] ; SAR EAX,0x18 ; CMP EAX,0x1F ; JNZ 0x765BC
0x765AC CMP byte[0x157A4C],0 ; JNZ 0x765BC
0x765B5 MOV EAX,EBP ; CALL 0x7DAB4                           ; FUN_0007DAB4(rec)
0x765BC..0x765C2 POP x5 ; RET                               ; epilogue
```

* `timer=0; stage++`; `target := pos`; if slot present **and**
  `word[slot+4] != 0` → `FUN_00076130(rec)` (the keeper input handler, ported as
  `fifa96_keeper_input_decide`).
* `[EBP+0x8E]>>24` = the **action code at `+0x91`** (alias; FU-151 erratum 1);
  `code == 0x1F && byte[0x157A4C] == 0` → `FUN_0007DAB4(rec)` (reset).
* Maps to `fifa96_keeper_arm_step` stage 3: `out->stage = 3; copy_pos = 1;
  run_handler = (has_slot && slot_pressed); reset = (code == 0x1F &&
  phase_latch == 0)`, with `slot_pressed = word[slot+4] != 0`, `code = +0x91`,
  `phase_latch = [0x157A4C]`.

### 2.8 Cells written by the row (summary)

| cell | where | value |
|---|---|---|
| `[rec+0x9E]` | stage 0 `0x76440` | 1 |
| `[rec+0x89]` | every entry `0x7641F`; stages 0/1/3 `0x7646C`/`0x7649E`/`0x7656E` | `+= [0x157A64]` then 0 |
| `[rec+0x92]` | stages 0/1/3, prologue camera hook gate | stage increment (byte at `+0x8F`+3) |
| `[rec+0x4D]/+0x51/+0x55` | prologue `0x763B6..0x763E9`; no-slot `0x764C0`/`0x7653C`/`0x76554`; tail `0x76586` | target triple |
| `[0x157C5C]` | prologue `0x763AB` | 0 |
| `[rec+0x91]` (via installer) | `0x7655C` install | 0x1B |
| `[rec+0x92]` (via installer) | `0x7655C` install | 1 (BL) |

Reads: `[0x157A4A]>>24` (phase, `0x763D0`), `[0x15882A]` (stage-0 gate),
`[0x158730]`/`[0x158746]` (stage-1 gate), `[0x15873A]` (dive trajectory),
`[0x157A64]` (delta), `[0x157A4C]` (reset gate), `word[slot+4]` (button). The
row **does not write `+0x9B`** (possession) directly.

---

## 3. The live installer: the phase-6 arm `0x8D4A2`

Fresh `disassemble_bytes 0x8D490` (208 B) confirms the only live installer of
code 0x1F (FU-174 §2):

```
0x8D4A2 PUSH -1 ; MOV ECX,3 ; MOV EBX,0xA ; MOV EAX,EBP ; XOR EDX,EDX ; CALL 0x8CEB8
0x8D4B7 MOV EDX,[0x157AAC] ; XOR EAX,EAX ; SAR EDX,0x18
0x8D4C2 MOV AL,byte[EBP+0x826]                 ; EAX = this team's side
0x8D4C8 CMP EAX,EDX ; JNZ 0x8D558              ; non-controlled team -> 0x1F
   ; controlled side: 0x8D4D0..0x8D547 the FUN_00073DC4 penalty-spot camera
   ; reset, 0x700F4, [team+0x7B2]=pick, install code 0x13 (0x8D532 EDX,0x13)
0x8D558 MOV EDX,0x1F ; MOV EAX,EBP ; XOR ECX,ECX ; XOR EBX,EBX
0x8D563 MOV dword[EBP+0x7B2],EBP               ; [team+0x7B2] = record 0 / team
0x8D569 CALL 0x7D9A4                           ; install 0x1F, BL=0, ECX=0 (no invoke)
```

So the non-controlled team's record 0 gets `code = 0x1F, stage byte = 0, no
invoke-now` (`[team+0x7B2] = EBP`). Engine mirror (first-hand
`match_run.c:2517..2530`): phase 6, the non-controlled team installs
`0x1Fu` on `&team->records[0]` (`match_run.c:2529`). This is the row's live
reachability: a T3 leg-1/5 penalty route drives `fifa96_match_run_phase_arm(6)`
once it is reached. The row then self-installs 0x1B (§2.6) invoke-now, which is
what makes the second-order install honest.

---

## 4. Wiring the ported helpers onto the body

The four ported helpers are exact extractions of the row (FU-140 §3.4); the
mapping the wiring must use:

| ported helper | native site | row wiring (inputs ← native cells) |
|---|---|---|
| `fifa96_keeper_arm_camera(stage, team_side, phase, &target, &hook)` | `0x763B1..0x76404` | `stage = byte[+0x92]`, `team_side = byte[team+0x826]`, `phase = [0x157A4A]>>24`; hook = the `0x79B6C(rec, 0x15774C, &target)` camera arm (unported body) |
| `fifa96_keeper_arm_step(stage, event_flag, ball_actor, ball_flag, has_slot, slot_pressed, code, phase_latch, &out)` | stages 0/1/2/3 `0x7643A..0x765BB` | `event_flag=[0x15882A]`, `ball_actor=[0x158730]`, `ball_flag=[0x158746]`, `has_slot=[rec+0x20]`, `slot_pressed=word[slot+4]!=0`, `code=byte[+0x91]`, `phase_latch=[0x157A4C]` |
| `fifa96_keeper_dive_target(pos, rng_a, divisor, rng_b, trajectory_z, rng_c, rng_d, &out)` | no-slot arm `0x764BA..0x76567` | 4 RNG draws (`0x764DF`/`0x764FC`/`0x76526`/`0x76541`), `divisor = 0x64 - 2*(int8)desc[0x12] - 2*(int8)desc[0x13]`, `trajectory_z = word[0x15873A]`; `out.install_code=0x1B, out.invoke=1` |
| `fifa96_keeper_input_decide(&in, &out)` | tail call `0x76599` → `FUN_00076130` | full `FUN_00076130` (FU-140 §3.1); outputs install 4 / 0x19 / 0x1C / 0x1B+carry, copy flags, `clear_c5c` |

`fifa96_keeper_arm_step` also emits `event_code=0x28` (stage 0, the
`FUN_0006E598` event), `copy_pos` (target = pos), `flag_9e` (`+0x9E=1`),
`reset` (the `FUN_0007DAB4` tail) — every one of which the native body performs.

**Fidelity gap (leg FU176-L1).** The native tail at `0x76568` increments the
stage **from the entry value**: a stage-2 slot entry writes `+0x92 = 3`, but a
direct stage-3 entry writes `+0x92 = 4` (`0x76568..0x76578 INC AH`). The port
returns `out.stage = 3` for both entries (test-pinned at
`test_keeper_bodies.c:250/252/256`). A wiring that stores `out.stage` would
re-enter stage 3 forever after the first tail; the port must be reconciled
(either the helper advances stage-3 → 4, or the caller adds the increment).

---

## 5. Minimal live port (T2 wiring rec)

Row 0x1F is `NULL` in `fifa96_match_action_table` (`match_handlers.c:3457`); the
installer is live (`match_run.c:2529`). The minimal honest port — **T2 engine
wiring**, no new loader code — is:

1. **`fifa96_match_action_1F(struct fifa96_match_run *mr)`** in
   `fifa96_match_handlers.c`, following the `fifa96_match_action_02` /
   `_1E` pattern: resolve the record (`r = &mr->record`, `e`), pack
   `fifa96_keeper_point pos/target` from `pos_x/y/z`, `target_x/y/z`, and:
   * prologue: build the `fifa96_keeper_input`/arm inputs, call
     `fifa96_keeper_arm_camera` (target triple write; hook → a request bit).
   * dispatch on `r->stage92`: `fifa96_keeper_arm_step` for stages 0/1/2/3; on
     the no-slot stage-2 return, draw the four RNG values from `mr->rng` and
     call `fifa96_keeper_dive_target`; write the target triple; drain the
     install with `fifa96_match_entities_install(e, phase, code, 1)` then
     `fifa96_match_dispatch_action(mr, code)` (the invoke-now pair, exactly
     `match_handlers.c:351..358`), for codes 0x1B / 4 / 0x19 / 0x1C.
   * `run_handler` → stage `fifa96_keeper_input_decide` over the record cells
     and drain its install; `reset` → `match_row_reset(mr, e)`; `event_code`
     0x28 → the event request; `flag_9e` → `r->ran = 1`.
   * apply `r->stage92 = out.stage` (after FU176-L1 is reconciled),
     `r->timer89`, `r->target_*`.
2. **`fifa96_match_action_table[0x1F] = fifa96_match_action_1F`** — flips one
   NULL `fn` to a ported body. Census moves **22 → 23** (`23 OK / 56 UNSUP / 1
   NOTF`) → `floor(100 × 23/80) = 28%` (README/ENGINE re-pin in the same commit,
   AGENTS.md census rule).
3. No loader change is needed: all four helpers exist and are tested; add a
   `test_engine_match_handlers` fixture pinning the dive/stage tail. The
   pool-side sinks (`0x7876C` merge, `0x79B6C` hook, `0x6E598` event) ride the
   existing request bits (`helper_request`, etc.) — the FU-140/FU-151 request
   convention.

Ranked alternatives if T2 slips: (a) wire only the no-slot dive stage
(0x1B invoke) as a partial with the stage machine stubbed — lower value, splits
the row; (b) leave 0x1F as the next-wave leg and take R25/R29's simpler rows
first. The full-row port is the honest minimal unit.

---

## 6. Numbered legs

1. **FU176-L1 — the `fifa96_keeper_arm_step` stage-3 off-by-one.** Native
   `0x76568` writes `+0x92 = entry+1`; the port returns `out.stage = 3` for
   both the stage-2-slot and the direct stage-3 entries (test-pinned). Reconcile
   before wiring or the tail repeats.
2. **FU176-L2 — unported sinks.** `FUN_0007876C` (slot merge, `0x7639B`),
   `FUN_00079B6C` (camera hook, `0x76404`), `FUN_0006E598` event 0x28
   (`0x76461`), `FUN_0007DAB4` reset (`0x765B7`) and the `FUN_00092AC8` dive
   RNG are the same OL-36 family; the port models them as request bits.
3. **FU176-L3 — `FUN_00076130` install execution.** The input handler's own
   `FUN_0007D9A4` installs (4/0x19/0x1C/0x1B+carry) are `fifa96_keeper_input_decide`
   outputs; the wiring must drain them invoke-now (FU-140 OL-36).
4. **FU176-L4 — the `dword[rec+4]` descriptor.** The dive `divisor` reads the
   signed bytes at `desc+0x12`/`desc+0x13` (`0x764C6..0x764D2`); the pool does
   not model `rec[+4]` (the FU-142b `player_d`/`player_e` are a different read).
   Stage 0 for the derived pool.
5. **FU176-L5 — live reachability depth.** The phase-6 arm installs 0x1F only
   on the non-controlled team's record 0; whether a T3 leg-1/5 penalty reaches
   `fifa96_match_run_phase_arm(6)` in a live loop is the R23/T3 open question,
   not this slice's.
6. **FU176-L6 — erratum candidate, FU-140 §2 row 1F.** The FU-140 row table
   lists the stage-3 tail as "(`0x79B1C`, input handler …)"; the fresh window
   `0x76568..0x765BC` contains **no `0x79B1C` call** — the tail is the
   `MOVSD` pos copy + the `0x76130` input handler + the `0x1F && [0x157A4C]==0`
   `FUN_0007DAB4` reset. `0x79B1C` belongs to the row-1E stage-8/9 shape
   (FU-151 §2.4), not row 1F.

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_xrefs_to` `0x76380` (1), `0x76130` (4), `0x7D9A4` (77);
`read_memory` `0x76370` (16 B);
`disassemble_bytes` `0x76370` (672 B — the full row-1F body `0x76380..0x765C2`
plus `0x765C4`), `0x8D490` (208 B, the phase-6 arm head → `0x8D558`), `0x8D558`
(24 B, the 0x1F install), `0x7D9A4` (64 B, the installer argument staging).
Engine/loader source read first-hand:
`src/fifa96_engine/fifa96_match_handlers.c` (295..374, 535..645, 3404..3457),
`src/fifa96_engine/fifa96_match_run.c` (2448..2569),
`src/fifa96_engine/fifa96_match_entities.c` (73..120),
`src/fifa96_loader/fifa96_keeper.c` (165..403),
`include/fifa96_loader/fifa96_keeper.h` (51..148),
`include/fifa96_engine/fifa96_match_run.h` (400..509, 590..610),
`tests/test_keeper_bodies.c` (237..260).
Docs cross-cited: FU-136 §2, FU-137 §2, FU-140 §1/§2/§3, FU-151 §2.4/§3.5,
FU-164, FU-174 §1/§2/§9. No writes beyond this slice file and the workspace
report.
