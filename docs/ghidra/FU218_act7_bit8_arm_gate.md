# FU-218 — the bit-8 act-7 arm (FU209-L1) + the `[0x14C32A]` session gate (FU208-L8) (phase-20 wave R66)

Provenance: M2 phase-20 wave **R66**, 2026-10-13, read-only. Authoritative program
**`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program argument;
this session `get_current_program_info` = `FIFA96.EXE`, path `/FIFA96.EXE`,
`watcom:LE:32:default`, `image_base 0`, `min_address 0x10000`,
`max_address .image::00174a2a`, 2706 functions, `.image` overlay present). Every
address/byte/xref/instruction below is a fresh `read_memory` /
`disassemble_bytes` / `disassemble_function` / `get_xrefs_to` /
`get_current_program_info` of **this** session unless marked *cross-cited* (then
the frozen FU doc is named). LE-image globals sit at `+0x100000`; the addresses
here are `/FIFA96.EXE` program (runtime) addresses. The alias traps are applied
(`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`). No writes: no
rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; the ISO/HDD untouched; no git commands.

Scope (the R66 brief): extend FU-209 (the act-7 / phase-`0x1D` timeline) and FU-208
§2 (the direct/queue split at `0x8A957`) with the **bit-8 act-7 arm** (FU209-L1)
and the **`[0x14C32A]` session gate** (FU208-L8): (1) the gate byte-level and its
writers/readers; (2) the direct-path `0x8AA7B → 0x8AB63 → 0x8AE08 → 0x8AEAE`
chain; (3) whether a *bit-8-specific* arm/stage exists in the phase-`0x1D` body;
(4) the convergence of the bit-8 path and the period-index-4 path on one act; (5)
the engine extension point and the M2 tape verdict.

**Answer in one line.** The **bit-8 "arm" is not an arm of the act-7 body** — the
body `0x89868` is producer-agnostic and reads neither the input/range words
(`0x14C1D4`/`0x14C1D6`) nor the period index (`0x157AC2`); the bit-8
(`0x4B0CB TEST AH,1`) and period-index-4 (`0x888AB CMP [0x157AC2],4`) producers
are two upstream sites of `FUN_0004B02C`/`FUN_00088860` that both call
`FUN_0008A938(8, …, BX=0)`, both take the **direct** path when the session gate
`[0x14C32A] == 0` (or a situation is pending), both land on the table-2 arm
`0x8A904[8] = 0x8AE08`, and both invoke **act 7 now** (`0x8AEAE MOV EBX,1`) into
the identical phase-`0x1D` body. `[0x14C32A]` = `([0x14C1D0] == 4)` is a
session-constant byte opened only by the single-team **Practise** launcher (mode
4); every ordinary played match leaves it **0**, so the ordinary-match direct path
is the engine-live route. The engine already carries the whole structure; the
bit-8 arm is inert **only** because `fifa96_input_range_words` never sets bit 8
(FU208-L1/FU214-L2), and the M2 tape reaches neither path (no period completion,
no bit-8).

---

## 1. The `[0x14C32A]` session gate (fresh)

### 1.1 The gate site is byte-exact (fresh `disassemble_bytes 0x8A938`)

```
0x8A957 CMP byte[0x14C32A],0x0     ; 80 3d 2a c3 14 00 00
0x8A95E JZ  0x8AA7B                ; 0f 84 17 01 00 00   gate CLOSED (==0) -> DIRECT
0x8A964 CMP dword[0x15B6C0],0x0    ; 83 3d c0 b6 15 00 00
0x8A96B JNZ 0x8AA7B                ; 0f 85 0a 01 00 00   situation pending != 0 -> DIRECT
0x8A971 … (the queue path: [0x15B6B8]=side, table-1 0x8A8E0, latch + RET)
```

Fresh `get_xrefs_to 0x14C32A` = **60** refs — **3 WRITE, 57 READ** — matching
FU-208 §2 / FU-155 §1 to the reference count.

### 1.2 The three writers (fresh; all front-end session entries)

| addr | fn | first-hand semantics |
|---|---|---|
| `0x1B7C8` | `FUN_0001B7B8` | mode `== 4` → `MOV byte[0x14C32A],1` |
| `0x1B7D5` | `FUN_0001B7B8` | mode `!= 4` → `MOV byte[0x14C32A],AH(0)` (+ `[0x15B680]=0`) |
| `0x32DFF` | `FUN_00032DE0` | result-code-4 arm → `MOV byte[0x14C32A],1`, then `FUN_0001B7B8(4)` |

Fresh `disassemble_function 0x1B7B8` (14 insns): `0x1B7B9 MOV [0x14C1D0],AL` (the
mode companion byte) → `0x1B7C3 CMP EAX,4 / JNZ 0x1B7D1` → `0x1B7C8 MOV
byte[0x14C32A],1` (mode 4) **else** `0x1B7D1 XOR AH,AH; 0x1B7D5 MOV
byte[0x14C32A],AH` (0). Fresh `disassemble_function 0x32DE0` (18 insns):
`0x32DE0 MOV EAX,1; CMP; …; 0x32DEC CMP EAX,4; 0x32DEF JZ 0x32DFF; 0x32DFF MOV
byte[0x14C32A],1; 0x32E06 MOV EAX,4; 0x32E0B CALL 0x1B7B8`.

**So the gate is `[0x14C32A] = ([0x14C1D0] == 4)`** (the Practise launcher mode 4),
*first-hand confirmed* and agreeing with the frozen FU-148 §18 / FU-155 §3
(cross-cited). **It closes (:=0) for every other match mode** (0/1/2/3/5). There is
**no in-match writer** — the byte is session-constant (FU-155 §5, cross-cited).

### 1.3 The readers (fresh; 57)

The 57 fresh READ sites (`search_instructions operand_pattern=14c32a`, the same 60
total; the 3 writes are §1.2). Polarity-relevant consumers include the dispatcher
`0x8A957`; the frame gate `0x4B198` (`!=0` → scheduler `CALL 0x948AC`); the clock
tail `0x8B3B7`/`0x8B65B` (`==0` → ball machine `CALL 0x8BAF0`); the phase walk
`0x8D168`; the goal/screen machine `FUN_0003BB1C` (`0x3BCA8`..`0x3BF71`); the
front-end screens `FUN_000372BC`/`FUN_0003749C`/`FUN_000396E4`/`FUN_00041494`; and
the ball/record systems `FUN_000886D4`/`FUN_0008C24C`/`0x84BB2`/`0x89244`. The
dispatcher `0x8A957` is **one consumer among 57** — the gate is far wider than the
sit-8 route.

**Gate semantics (one line).** `[0x14C32A] == 0` (the **ordinary-match** session —
all modes but 4) → the dispatcher takes the **direct** path; `== 1` (the
single-team **Practise** session) → the **queue** path (unless a situation is
already pending). Situation 8 therefore dispatches act 7 immediately on an
ordinary match and is queued on Practise.

---

## 2. The direct path `0x8AA7B → 0x8AB63 → 0x8AE08 → 0x8AEAE`

### 2.1 The sit-8 chute (fresh `disassemble_bytes`)

```
0x8AA7B TEST BX,BX                  ; 66 85 db
0x8AA7E JZ  0x8AAA8                 ; 74 28     BX==0 -> table-2 chute (both sit-8 arms pass BX=0)
0x8AAA8 CMP CX,1                    ; 66 83 f9 01
0x8AAAC JC  0x8AB63                 ; 0f 82 b1 00 00  (CX<1 -> hub)
0x8AAB2 JBE 0x8AABF                 ; 76 0b     (CX==1 -> sit-1 twin)
0x8AAB4 CMP CX,4                    ; 66 83 f9 04
0x8AAB8 JBE 0x8AB08                 ; 76 4e     (CX 2..4 -> the order arm)
0x8AABA JMP 0x8AB63                 ; e9 a4 00 00 00  (else, incl. 8 -> hub)
; --- hub 0x8AB63 ---
0x8AB63 MOV AL,[ESP]                ; 8a 04 24        (side)
0x8AB66 MOV [0x157AAF],AL           ; a2 af 7a 15 00  store the side
0x8AB6B CMP CX,0xC                  ; 66 83 f9 0c
0x8AB6F JA  0x8AF1F                 ; 0f 87 aa 03 00 00  (> 0xC -> act-8 fallback)
0x8AB75 XOR EAX,EAX / MOV AX,CX
0x8AB7A JMP CS:[EAX*4+0x8A904]      ; 2e ff 24 85 04 a9 08 00  TABLE-2
```

Fresh `read_memory 0x8A904` (64 B): `[8] = 08 ae 08 00` = **`0x8AE08`** (and
`[7] = f0 ad 08 00 = 0x8ADF0`, `[0] = 0x8AB82` — byte-identical to FU-208 §1.2).
Sit 8 with `BX=0` therefore lands `0x8AB63 → 0x8A904[8] = 0x8AE08`.

### 2.2 The sit-8 arm `0x8AE08` → act-7 invoke-now (fresh)

```
0x8AE08 MOV EAX,[0x157A49] / SAR EAX,0x18   ; a1 49 7a 15 00 / c1 f8 18
0x8AE10 CMP EAX,0x1 / JZ 0x8AEAE            ; 83 f8 01 / 0f 84 95 00 00  (byte [0x157A4C]==1 -> skip re-init)
0x8AE19..  (re-init: CL=1; [0x157AAF]=side; EDX=0xA; [0x157A4C]=1 (0x8AE2A `88 0d 4c 7a 15 00`);
            0x8AE30 CALL 0x897F4(0xA); 0x8AE3F CALL 0x897F4(0xA,1);
            0x8AE44 EAX=[ESP-2]>>16; 0x8AE4B MOV EAX,[EAX*4+0x1587CC]; 0x8AE54 MOV [0x1587D4],EAX)
0x8AE9E XOR EBX,EBX ; word[0x157AC7]=BX ; word[0x157AC5]=BX   ; clear the score pair
0x8AEAE MOV EBX,0x1                 ; bb 01 00 00 00   <<< BX = invoke-now
0x8AEB3 MOV EAX,0x7                 ; b8 07 00 00 00   <<< ACT 7
0x8AEB8 XOR EDX,EDX                 ; 31 d2              side 0
0x8AEBA CALL 0x888FC                ; e8 3d da ff ff
```

So the sit-8 arm **installs act 7 and forces invoke-now** (`BX := 1`,
`0x8AEAE`). The actor pointer `[0x1587D4]` used by the phase-`0x1D` arm-0 coupled
install is **bound here** (`0x8AE4B..0x8AE54`, `[0x1587CC + side*4] →
[0x1587D4]`) — the FU-210-L3 producer (cross-cited).

### 2.3 Where act 7 is installed (fresh `disassemble_function 0x888FC`, 20 insns)

```
0x888FD MOVSX CX,byte[0x158828] ; 0x88905 CMP AX,CX / JZ 0x8893D   (latch no-op)
0x8890A MOV [0x158828],AL                             (act id)
0x88910 SHL EAX,2 ; 0x88913 ADD EAX,0x1107EC
0x88918 MOV byte[0x158829],DL                         (stage)
0x88920 MOV EAX,[EAX]                                (handler)
0x88922 MOV word[0x158818],DX                         (timer=0)
0x88929 MOV [0x158808],EAX          <<< ACT SLOT INSTALLED
0x8892E TEST BX,BX / 0x88931 JZ 0x8893D
0x88933 TEST EAX,EAX / 0x88935 JZ 0x8893D
0x88937 CALL dword[0x158808]        <<< INVOKE-NOW iff BX!=0
```

Fresh `read_memory 0x1107EC` (32 B): `0x1107EC[7] = 68 98 08 00` = **`0x89868`**
(the act table base `0x1107EC = 0x110794 + 0x16*4`; act 7 → phase `0x1D`). So the
seam **installs `[0x158808] = 0x89868` at `0x88929` and, because `BX=1`, invokes it
on the same dispatch at `0x88937`** — the phase-`0x1D` body's stage-0 arm runs on
the dispatch frame (FU-209 §1.3, cross-cited).

**The direct-path chain (one line):** `0x8A957` (gate closed) / `0x8A96B` (pending)
→ `0x8AA7B` (BX=0) → `0x8AAA8` → `0x8AB63` → `0x8A904[8]=0x8AE08` → `0x8AEAE`
(`EBX=1`, `EAX=7`) → `0x888FC` (install `0x89868` + invoke-now).

---

## 3. The bit-8 "arm" — there is **no** bit-8-specific arm in the body (fresh)

The brief's premise ("which arm/stage of the act-7 body is the bit-8-specific
path") is **not correct, and this slice proves it first-hand**: the phase-`0x1D`
body is producer-agnostic.

* Fresh `search_instructions operand_pattern=14c1d` (all `0x14C1D*` sites, 66
  program-wide): **none lies in `0x89868..0x89EAF`.** The act-7 body never reads
  the input/range words (`0x14C1D4`/`0x14C1D6`) — the bit-8 signal.
* Fresh `search_instructions operand_pattern=157ac2` (56 sites): **none lies in
  `0x89868..0x89EAF`.** The body never reads the period index — the
  period-index-4 signal.

The body is the fixed 10-arm cascade FU-209 §2 derived (arms 0–9, no producer
branches). The bit-8 vs period-index-4 distinction lives **entirely in the two
producers**, both upstream:

### 3.1 Producer A — the bit-8 extra-time request (`FUN_0004B02C`, fresh)

```
0x4B0BC MOV CX,word[0x14C1D4]       ; 66 8b 0d d4 c1 14 00
0x4B0C3 MOV AX,[0x14C1D6]           ; 66 a1 d6 c1 14 00
0x4B0C9 OR  EAX,ECX                 ; 09 c8
0x4B0CB TEST AH,0x1                 ; f6 c4 01           <<< BIT 8 (0x100) of the OR
0x4B0CE JZ  0x4B0E5                 ; 74 15
0x4B0D0 MOV EAX,0x8 ; XOR EBX,EBX ; XOR EDX,EDX
0x4B0D9 CALL 0x8A938                ; e8 5a f8 03 00     <<< SITUATION 8, side 0, BX 0
0x4B0DE MOV byte[0x157AC2],0x4      ; c6 05 c2 7a 15 00 04  <<< period index := 4
```

### 3.2 Producer B — the phase-`0xC` / period-index-4 period-end (`FUN_00088860`, fresh)

```
0x888AB MOV AL,[0x157AC2]           ; a0 c2 7a 15 00
0x888B0 CMP EAX,0x4 / JNZ 0x888E2   ; 83 f8 04 / 75 2d
0x888B5 CALL 0x92AC8                ; RNG
0x888BC AND AL,0x1 ; 0x888BE XOR EBX,EBX ; 0x888C0 MOVSX EDX,AX
0x888C3 MOV EAX,0x8
0x888C8 CALL 0x8A938                ; e8 6b 20 00 00     <<< SITUATION 8, side rng&1, BX 0
```

Both producers pass **`EAX=8`, `BX=0`** into `FUN_0008A938`; the only register
difference is the side (`EDX`: producer A = 0; producer B = `rng & 1`), which lands
in `[0x157AAF]` at the hub `0x8AB66` and drives the `0x8AE08` re-init/actor-bind.

**Consequence.** There is no `[0x157AC2]`- vs bit-8-specific stage in the phase
body; the two "arms" are the two producer sites, and both reach the body
identically (see §1, §2, §4).

---

## 4. Convergence — both paths → one act, one body (fresh)

| | bit-8 path (producer A) | period-index-4 path (producer B) |
|---|---|---|
| gate | `0x4B0CB TEST AH,1` (`FUN_0004B02C`) | `0x888AB CMP [0x157AC2],4` (`FUN_00088860`) |
| dispatch | `CALL 0x8A938(8, side=0, BX=0)` | `CALL 0x8A938(8, side=rng&1, BX=0)` |
| dispatcher | direct path (`[0x14C32A]==0` or pending `[0x15B6C0]!=0`) | direct path (same) |
| table-2 arm | `0x8A904[8] = 0x8AE08` | `0x8A904[8] = 0x8AE08` |
| act | **act 7** invoke-now (`0x8AEAE MOV EBX,1`) | **act 7** invoke-now (same) |
| phase slot | `0x1107EC[7] = 0x89868` = phase `0x1D` | same |
| body | `0x89868` (arms 0–9) | **the identical body** |

**No arm-level divergence.** The only per-producer difference is the `side`
register (A=0, B=`rng&1`), consumed by the shared `0x8AE08` re-init and stored to
`[0x157AAF]`; the act-7 body and the phase-`0x1D` arm cascade are byte-identical
for both. This re-confirms and sharpens FU-207 §4.1 and FU-208 §5.3.

---

## 5. Engine extension point + tape reachability

### 5.1 The engine already models both producers and the routing

Both producers are in `fifa96_match_run_period_setup`
(`src/fifa96_engine/fifa96_match_run.c:3145`):

* **producer B** — mode 0, `:3160` `if (mr->state.phase == 0x0Cu)` → `:3161`
  `if (mr->global_157ac2 == 4u)` → `:3164` `fifa96_match_run_set_piece(mr, 8u,
  r & 1u, 0u)` (native `0x888C8`);
* **producer A** — the **common tail**, `:3175` `if ((mr->input_range[0] |
  mr->input_range[1]) & 0x0100u)` → `:3176` `fifa96_match_run_set_piece(mr, 8u, 0u,
  0u)` + `:3177` `mr->global_157ac2 = 4u` (native `0x4B0CB`/`0x4B0D9`/`0x4B0DE`).

Both then route `set_piece(mr, 8, …)` → the head gate (`:3059`, direct when
`!session_gate || situation_pending`, `:3065-3066`; direct path `:3091`) →
`match_run_situation_side` (`:2847`) → `fifa96_match_act_dispatch(mr, out.act, …)`
(`:2889`). The act-7 slot (phase `0x1D`) is registered
(`fifa96_match_handlers.c:5236`, `{0x1D, fifa96_match_action_phase_1D}`) and the
body (`match_run.c:6082`) is the fixed stage cascade — **it carries no bit-8
branch**, matching the native (§3).

### 5.2 The session gate is engine-live and correctly valued

`mr->session_gate_14c32a` is **seeded 0** in the engine
(`fifa96_match_run.c:1772`, `:2071`, `:2198`) and has **no `= 1` writer anywhere in
`src/`** (only fixtures set it). Fresh first-hand: **0 is the native ordinary-match
value** (§1.2/§1.3: `[0x14C1D0] != 4` for modes 5/0/1/2/3). So the engine's live
session is the native **byte-0 ordinary match** → the dispatcher's **direct path**
is the engine's live path; the sit-8 arm is not routed to the queue on the live
model. The residual (narrower than FU-208 §7 L8) is only that the engine never
models the **mode-4 Practise byte-1 queue session**.

### 5.3 The exact gaps

1. **The bit-8 range-word writer (the sharpest, FU208-L1/FU214-L2).**
   `fifa96_input_range_words` (`src/fifa96_loader/fifa96_input.c:86`) mirrors
   native `FUN_0001C9BC` and writes bits `{0x1,0x2,0x4,0x8,0x10,0x20,0x40,0x80,
   0x200}` (`:94-116`); **bit `0x100` is never set** (the native builders write bit
   9 = `0x200` or zero). So `(input_range & 0x100) == 0` **always** and producer A
   never fires. Do not fake a bit-8 producer.
2. **The period-index-4 path is structurally live.** `fifa96_match_run_phase_drive`
   advances `mr->global_157ac2 = mr->state.period` (`:2639`) and, at a completion
   with `== 4u`, calls `fifa96_match_run_period_setup(mr, 0)` (`:2650-2653`), so
   producer B can fire. (FU-214 item §2 read "`global_157ac2` never reaches 4"; that
   holds **on the tape** (no completion) but not structurally in the current code —
   §5.4/erratum.)
3. **The act-7 invoke cadence (FU208-L4, unchanged).** The native sit-8 arm
   invokes now (`0x8AEAE MOV EBX,1`); `match_run_situation_side` passes `invoke = 0`
   for act 7 (`:2889`, `out.act == 2u ? 1 : 0`).
4. **The sit-8 arm match re-init / actor bind (FU208-L7, unchanged).** The
   `0x8AE19..0x8AE54` block (`[0x157A4C]=1`, the `FUN_000897F4(…,0xA)` pair, the
   `[0x1587CC+side*4] → [0x1587D4]` actor bind) has no engine model beyond the
   derived side store.

### 5.4 The M2 tape reaches **neither** path

Fresh first-hand: the M2 tape is a single-match replay whose period completes once
and exits (`tests/test_engine_m2.c:30-99`; the `clock_period_ended` consumption at
`:81`). Reaching `global_157ac2 == 4` requires **four** period completions
(`mr->global_157ac2 = mr->state.period`, `match_run.c:2639`); the tape never gets
there. And bit 8 is never set (§5.3.1). So:

* the bit-8 path (producer A) — **tape-dormant** (bit-8 structurally unwritten);
* the period-index-4 path (producer B) — **tape-dormant** (no index-4 completion).

Both sit-8 producers write only the act slot (select act 7), never a render field,
so even a fired dormant landing on the tape is render-invisible — consistent with
FU-214 §4 (C-1 **dormant**) and FU-214 §7 L6. **No M2 re-pin is forced**, and the
current phase-`0x1D` registration (T2) is already in place (census `53/80 = 66%`,
`docs/ENGINE.md:8`).

---

## 6. Residual legs (FU-218)

1. **FU-218-L1 — the bit-8 range-word writer is unlocated (extends FU208-L1 /
   FU196-L1 / FU214-L2).** The sit-8 gate tests bit 8 (`0x4B0CB TEST AH,1`); the
   only two range-word builders (`FUN_0001C9BC` `0x1CA15`/`0x1CA25` = bit 9
   `0x200`; `FUN_00024B00` `0x24B91`/`0x24B98` = 0) never write it. This is the
   **sole** blocker of the bit-8 act-7 arm. Settle by tracing the
   `[0x105288]`/`[0x10529C]` config cell's front-end producer and the extra-time
   menu path. **Do not fake a bit-8 producer.**
2. **FU-218-L2 — the act-7 invoke cadence** (carries FU208-L4). Native invoke-now
   (`0x8AEAE EBX=1`); engine defers (`match_run.c:2889`). A landing must parallel
   FU196-L3/FU202-L3.
3. **FU-218-L3 — the `[0x14C32A]` session model (narrows FU208-L8).** The engine's
   settled seed 0 equals the native ordinary-match value (direct path correct); the
   unmodelled branch is the **mode-4 Practise byte-1 queue session** and the queue
   id space (FU-146 S3 / FU-149 L4). No in-match writer exists natively either
   (session-constant, FU-155 §5) — so this is not a missing producer, only an
   unmodelled session.
4. **FU-218-L4 — the sit-8 arm re-init/actor bind** (carries FU208-L7). The
   `0x8AE19..0x8AE54` block is unported; the actor `[0x1587D4]` cell is FU210-L3.
5. **FU-218-L5 — the period-index-4 producer's live reachability.** Structurally
   present (`match_run.c:2639`/`:2650`), but reached only at a period-4 completion;
   the tape never completes a period. Owner: the period-rollover wiring
   (FU213-L2, resolved for the sit-8 path).

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, image base
  `0`, min `0x10000`, max `.image::00174a2a`, 2706 functions.
* `read_memory`: `0x8A904` (64 B, the table-2 dwords), `0x1107EC` (32 B, the act
  table).
* `disassemble_bytes` (start, length B): `0x8A938` (200, the dispatcher head),
  `0x8AA7B` (96, the direct path), `0x8AB60` (48, the hub), `0x8AE08` (80, the
  sit-8 arm), `0x8AE58` (80, the re-init/score-clear tail), `0x8AEA8` (64, the
  act-7 invoke + the following arms), `0x8AD60` (168, the surrounding table-2
  arms), `0x4B0B0` (56, producer A), `0x888A8` (48, producer B).
* `disassemble_function`: `0x1B7B8` (14 insns, writer 1), `0x32DE0` (18 insns,
  writer 2), `0x888FC` (20 insns, the act seam).
* `get_xrefs_to`: `0x14C32A` (**60** = 3 W + 57 R).
* `search_instructions`: `operand_pattern=157ac2` (56 sites — none in the act-7
  body), `operand_pattern=14c32a` (60 sites), `operand_pattern=14c1d` (66 sites —
  none in the act-7 body).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(`:2564-2694` `phase_drive` + the `:2639` roll / `:2650-2653` mode-0 call;
`:2847-2894` `match_run_situation_side` + the `:2889` dispatch/invoke;
`:3059-3133` `set_piece` head/direct; `:3145-3181` `period_setup` incl. the
`:3160-3170` producer B and `:3175-3178` producer A; `:1772`/`:2071`/`:2198` the
`session_gate_14c32a = 0` seeds; `:6082-6145` `fifa96_match_action_phase_1D`);
`src/fifa96_engine/fifa96_match_handlers.c` (`:5236` phase `0x1D` registered);
`src/fifa96_loader/fifa96_input.c` (`:86-120` `fifa96_input_range_words` — no
`0x100` bit). Tests: `tests/test_engine_m2.c` (`:30-129` the tape provenance;
`:3054`/`:3550-3645` the sit-8/phase-`0x1D` fixtures). Docs cross-cited: FU-148
(§18), FU-155 (§1/§3/§5), FU-196 (§1/§2), FU-205, FU-206 (§4/§5), FU-207
(§4/§5/§6), FU-208 (§1/§2/§4/§5/§6/§7), FU-209 (§1/§2/§3/§6), FU-210 (§5/§7),
FU-214 (§2/§4/§7). No writes beyond this slice file; no rename/label/comment/
function/script/project save; no git.

---

## Errata / corrections (prior sections are not rewritten)

**Correction — the R66 brief's premise ("the bit-8 act-7 arm is an arm of the
phase-`0x1D` body").** There is **no bit-8-specific arm or stage** inside
`0x89868`: the body reads neither `0x14C1D4`/`0x14C1D6` nor `0x157AC2` (§3, fresh
`search_instructions`). The bit-8 vs period-index-4 distinction is entirely at the
two **producer** sites (`0x4B0CB` vs `0x888AB`); both converge on the identical
table-2 arm `0x8AE08`, act-7 invoke-now, and phase-`0x1D` body (§4).

**Erratum — FU208 §6.2 / FU214 §2 status of `[0x14C32A]` / `global_157ac2`.**
(a) FU208 §7 L8 and FU214 §3 read `session_gate_14c32a` "seeded 0 with no derived
producer, so the live direct-path reachability of sit 8 is unproven." Fresh: **0
is the native ordinary-match value** (`[0x14C1D0] != 4`, §1.2/§1.3), so the
engine's live model is on the direct path by construction; the residual is only
the unmodelled mode-4 Practise queue session (FU-218-L3). (b) FU214 §2's
"`global_157ac2` never reaches 4" holds **on the M2 tape** (no completion) but not
structurally: `match_run.c:2639` advances it from `state.period` and `:2650` gates
the mode-0 producer on `== 4u`.
