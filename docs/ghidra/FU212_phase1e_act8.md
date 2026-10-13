# FU-212 — phase `0x1E` / act 8 (the `0x8A798` restart sweep) + the de-compression verdict (phase-19 wave R60)

Provenance: M2 phase-19 wave **R60**, 2026-10-13, read-only. Authoritative program
**`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program argument;
confirmed this session by `get_current_program_info` — `/FIFA96.EXE`,
`watcom:LE:32:default`, 2706 functions, `.image` overlay present). Every
address/byte below is a fresh `disassemble_bytes` / `read_memory` /
`disassemble_function` / `get_xrefs_to` / `get_current_program_info` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named). LE-image
globals sit at **`+0x100000`** from the file offset; the addresses here are
`/FIFA96.EXE` program (runtime) addresses. The alias traps are applied
(`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`). Engine source
read first-hand (`src/fifa96_engine/fifa96_match_run.c`,
`fifa96_match_handlers.c`, `include/fifa96_engine/fifa96_match_run.h`,
`fifa96_match_entities.h`, `tests/test_engine_m2.c`,
`tests/test_engine_match_frame.c`, `tests/test_engine_match_act.c`, the golden
`tests/golden/engine/m2-frames.txt`). **No writes**: no rename, comment, label,
function, script or project save; `/FIFA96.EXE` untouched; the ISO never
touched; no git commands. This is the one new file of the slice.

Scope (R60 brief): derive native **phase `0x1E` / act 8** (`0x8A798`,
first-hand); timeline the body completely (the 22-record sweep, its gates and
stage chain); then answer the **de-compression question** FU-194 raised: does the
engine's current *compressed* act-8 model, if replaced by the faithful 22-record
sweep, change M2 (the tape)? Identify exactly which records/fields move, whether
the tape ever stages act 8, what the engine must add/change, and whether a re-pin
would be forced.

---

## 1. Fresh anchors — the slot, the handler, the xref

Fresh `read_memory 0x110794` (140 B, the phase table) confirms
`0x110794[0x1E] = 0x0008A798` (bytes at offset `0x1E*4 = 0x78`:
`98 a7 08 00`). Fresh `read_memory 0x1107EC` (52 B, the act table) confirms
`0x1107EC[8] = 0x0008A798` (bytes at offset `8*4 = 0x20`: `98 a7 08 00`, the
base being `0x110794 + 0x16*4`). So **act 8 → phase `0x16+8 = 0x1E`** is the
FU165-L9 dual act/phase identity, byte-identical to FU-194 §1 / FU-206 §1.

Fresh `get_xrefs_to 0x8A798` = **1**, DATA, from **`0x11080C`**
(`0x110794 + 0x1E*4`): the phase-table slot is the **sole** reference. There is
no direct call site — the act/phase seam (`FUN_0008D098` → `FUN_0006D920` →
`[rec+0x1C]`, and the global act slot `[0x158808]` ticked by the clock tail,
*cross-cited* FU-175 §2/§5) is the only dispatch route.

Fresh `disassemble_function 0x8A798` = **"No function found at or containing
address 0x8a798"** (table-only target; not auto-created, as FU-194 §4 noted).

Fresh `disassemble_bytes 0x8A798` (through `0x8A8DF`) = **91 instructions**
(`0x8A8DE MOV EAX,EAX` is padding; `0x8A8E0` begins the BX==0 dispatch
table/next data — see §3). Handler body = **`0x8A798..0x8A8DD`**.

---

## 2. The complete body timeline (first-hand disassembly)

Raw bytes (`0x8A798..0x8A8DF`, 328 B) disassembled this session; every address
below is one of those instructions.

### 2.0 Head + stage dispatch

```
0x8A798 PUSH EBX ; PUSH ECX ; PUSH EDX
0x8A79B MOV AL,[0x158829]      ; the stage byte
0x8A7A0 CMP AL,0x1
0x8A7A2 JC  0x8A7B2            ; AL<1 (unsigned) -> stage 0
0x8A7A4 JBE 0x8A7D6            ; AL==1 -> stage 1
0x8A7A6 CMP AL,0x2
0x8A7A8 JZ  0x8A895            ; AL==2 -> stage 2
0x8A7AE POP EDX ; POP ECX ; POP EBX ; RET    ; AL>2 -> epilogue
```

Note the head performs **no timer add** (unlike `phase_1D`/`_21`/`_22`, which
head with `word[0x158818] += word[0x157A64]`); it reads the stage directly. This
matches the engine port's header note (`match_run.h:1561`).

### 2.1 Stage 0 (`0x8A7B2`) — reset, then fall through into stage 1

```
0x8A7B2 TEST AL,AL ; JNZ 0x8A8DA        ; (redundant: reached only with AL==0)
0x8A7BA MOV [0x15882E],AL      ; team cursor  = 0
0x8A7BF XOR BL,BL
0x8A7C1 MOV [0x15882F],AL      ; record cursor= 0
0x8A7C6 XOR BH,BH
0x8A7C8 INC AL                 ; AL = 1
0x8A7CA MOV word[0x158818],BX  ; timer = BX = 0
0x8A7D1 MOV [0x158829],AL      ; stage = 1
0x8A7D6 (falls through — no jump)
```

The `JNZ 0x8A8DA` at `0x8A7B4` is **unreachable-taken**: `0x8A7A2 JC` admitted
only `AL < 1`, i.e. `AL == 0`, so `TEST AL,AL` always zeroes ZF=1 and the branch
is never taken. It is a defensive/compiler artifact, recorded here so it is not
mistaken for a gate.

### 2.2 Stage 1 (`0x8A7D6`) — the 22-record sweep, one record per call

```
0x8A7D6 CALL 0x4BEC8 ; TEST AX,AX ; JNZ 0x8A8DA      ; the wait gate (§4)
0x8A7E4 MOV EAX,[0x15882B] ; SAR EAX,0x18            ; = byte 0x15882E = TEAM idx
0x8A7EC IMUL EAX,EAX,0x835 ; EDX=0x1588A4 ; ADD EDX,EAX   ; team base
0x8A7F9 MOV EAX,[0x15882C] ; SAR EAX,0x18            ; = byte 0x15882F = RECORD idx
0x8A801 IMUL EAX,EAX,0xB2 ; ADD EAX,EDX              ; rec ptr
0x8A809 MOV EDX,[EAX+0x8E] ; XOR EBX,EBX ; SAR EDX,0x18  ; = byte[rec+0x91] = CODE
0x8A814 MOV dword[0x1587F0],EBX (0)                  ; clear the selection cell
0x8A81A CMP EDX,0xC ; JNZ 0x8A840                    ; code != 0xC -> next
0x8A81F CMP byte[EAX+0x99],0 ; JZ 0x8A840            ; +0x99 == 0 -> next
0x8A828 CMP byte[EAX+0x9A],0 ; JNZ 0x8A840           ; +0x9A != 0 -> next
0x8A831 MOV [0x1587F0],EAX                           ; SELECT this record
0x8A836 MOV EAX,0x19 ; CALL 0x4C374                  ; clears word[0x157A68] (§4)
0x8A840 INC byte[0x15882F]                           ; record++
0x8A846 MOV EAX,[0x15882C] ; SAR EAX,0x18
0x8A84E CMP EAX,0xB ; JL 0x8A8DA                     ; record < 11 -> return
0x8A857 DL=[0x15882E] ; INC DL ; XOR AH,AH           ; team++
0x8A861 MOV [0x15882E],DL ; MOV [0x15882F],AH(0)     ; team++ ; record = 0
0x8A86D MOV EAX,[0x15882B] ; SAR EAX,0x18
0x8A875 CMP EAX,0x1 ; JLE 0x8A8DA                    ; team <= 1 -> return
0x8A87A DH=[0x158829] ; XOR ECX,ECX ; INC DH
0x8A884 MOV word[0x158818],CX(0)                     ; timer = 0
0x8A88B MOV [0x158829],DH                            ; stage = 2
0x8A891 POP EDX ; POP ECX ; POP EBX ; RET
```

**The sweep order.** The team index is `byte[0x15882E]`, the record index
`byte[0x15882F]`; the record stride is `0xB2` and the team stride `0x835`
(team-base `0x1588A4`). Each stage-1 call processes exactly **one** record and
returns (unless it rolls the team). Traced:

* call #1 enters via stage 0 (resets cursors, `stage=1`) then **falls through**
  and processes record **(0,0)**; `record → 1`; `1 < 11` → return.
* calls #2..#11 process **(0,1)…(0,10)**; after (0,10), `record → 11`,
  `11 < 11` false → `team → 1, record → 0`; `team <= 1` → return.
* calls #12..#22 process **(1,0)…(1,10)**; after (1,10), `record → 11` →
  `team → 2`; `team <= 1` false → `timer = 0, stage = 2`; return.
* call #23 is **stage 2** (the terminal, §2.3).

So the sweep covers **22 records = 2 teams × 11** — team 0 records 0..10, then
team 1 records 0..10 — **one per granted frame**, and reaches the terminal on the
**23rd** granted frame. (The gate at `0x8A7D6` is always open, §4, so the only
cadence is the granted-frame tick.)

**The per-record action is only a selection + a `[0x157A68]` clear.** For a
record with `code [rec+0x91] == 0xC` AND `byte[rec+0x99] != 0` AND
`byte[rec+0x9A] == 0`, the sweep stores the record pointer in `[0x1587F0]` and
calls `0x4C374` with `EAX=0x19`. It writes **no record field** — it does not
change the record's `code`, position, animation, frame or `+0x9A`.

### 2.3 Stage 2 (`0x8A895`) — the terminal re-dispatch

```
0x8A895 CALL 0x4BEC8 ; TEST AX,AX ; JNZ 0x8A8DA      ; the wait gate (§4)
0x8A89F MOV [0x158818],AX (0)                        ; timer = 0
0x8A8A5 XOR EDX,EDX ; XOR EBX,EBX ; XOR AH,AH
0x8A8AB MOV dword[0x158808],EDX (0)                  ; act slot clear
0x8A8B1 MOV byte[0x158829],AH (0)                    ; stage clear
0x8A8B7 MOV byte[0x158828],AH (0)                    ; act id clear
0x8A8BD MOV EDX,[0x158829] ; SAR EDX,0x18            ; = byte 0x15882C = STORED SIDE
0x8A8C3 MOV EAX,[0x158828] ; SAR EAX,0x18            ; = byte 0x15882B = STORED SITUATION
0x8A8CE CALL 0x8A938                                 ; re-dispatch (sit, side), BX=0
0x8A8D3 MOV byte[0x15882B],0xFF                      ; clear the stored situation
0x8A8DA POP EDX ; POP ECX ; POP EBX ; RET            ; the shared epilogue
```

**Alias-trap reconciliation.** The two clears at `0x8A8B1`/`0x8A8B7` land on the
**low** bytes `0x158829`/`0x158828`; the later dword reads at `0x8A8BD`/`0x8A8C3`
are `[0x158829]`/`[0x158828]` whose `>>24` top byte is `0x15882C`/`0x15882B`
respectively. So the clears do **not** erase the re-dispatch arguments: the
stored **side** (`0x15882C`) and stored **situation** (`0x15882B`) survive and
are passed to `FUN_0008A938` (`EAX=situation`, `EDX=side`, `BX=0`). This
confirms FU-194 §4.1's reading; the `CALL 0x8A938` bytes are `e8 65 00 00 00`
→ target `0x8A8D3 + 0x65 = 0x8A938`, agreeing with Ghidra. `0x8A8D3` then
writes `0x15882B = 0xFF`.

So act 8 is a **restart sweep that re-dispatches the stored situation/side**,
exactly as FU-194 §4.1 stated. Stage 2 is reachable only after 22 stage-1
records (or the rollover path).

---

## 3. The producer chain — the `FUN_0008A938` BX!=0 fallback (first-hand)

Fresh `disassemble_bytes 0x8A938` (through `0x8AADF`, 424 B). The head routes to
the act-8 fallback `0x8AA7B` when:

```
0x8A944 TEST AX,AX ; 0x8A947 JZ 0x8AA7B        ; situation == 0
0x8A94E CMP EAX,0xB ; 0x8A951 JZ 0x8AA7B       ; situation == 0xB
0x8A957 CMP byte[0x14C32A],0 ; 0x8A95E JZ 0x8AA7B   ; the session gate closed
0x8A964 CMP dword[0x15B6C0],0 ; 0x8A96B JNZ 0x8AA7B ; a situation pending
```

Otherwise situations 2..0xA take the `0x8A996 JMP dword CS:[EAX*4+0x8A8E0]`
table route and >0xA takes the `0x8AA60` default (id 0xA) — the table-1 queue
arm. The fallback itself:

```
0x8AA7B TEST BX,BX ; 0x8AA7E JZ 0x8AAA8        ; BX==0 -> the table-2 route
0x8AA80 XOR EDX,EDX ; XOR EAX,EAX ; MOV EBX,1
0x8AA89 CALL 0x740A0                            ; write PHASE 0 (EAX=0, side 0)
0x8AA8E MOV AL,[ESP]  ; XOR EDX,EDX
0x8AA93 MOV [0x15882C],AL                       ; stored SIDE
0x8AA98 MOV EAX,8                               ; b8 08 00 00 00   <<< ACT 8
0x8AA9D MOV byte[0x15882B],CL                   ; stored SITUATION (CL = situation)
0x8AAA3 JMP 0x8AF1A                             ; e9 72 04 00 00
```

Fresh `get_xrefs_to 0x8AF1A` = **1** (UNCONDITIONAL_JUMP from `0x8AAA3`);
`0x8AF1A` = `CALL 0x888FC` (`e8 dd d9 ff ff` → `0x8AF1F + 0xFFF D9DDD =
0x888FC`, agreeing with Ghidra). It is entered with `EAX=8` (act), `EDX=0`
(`DL=0` stage) and `EBX=1` (invoke-now), because the `0x8AA80..0x8AA9D`
fallback zeroes EDX and sets EBX=1 and neither is re-touched before the JMP. A
single call site (`0x8AF1A`) is shared: `0x8AF0E MOV EBX,1 ; 0x8AF13 MOV EAX,9`
(the table-2 sit-0xC arm) falls into the same `CALL`.

**Producer verdict.** Act 8's producer is the **`FUN_0008A938` BX!=0 fallback**
reached with situation 0/0xB or a closed-gate/pending state — *not* a literal
`EAX=8` site — confirming FU-194 §4.3. It dispatches act 8 **invoke-now**, so the
handler's stage 0 runs in the same frame; the sweep then spans the next ~22
granted frames; stage 2 re-dispatches the stored situation.

---

## 4. The gates — `0x4BEC8` and `0x4C374`, both first-hand (the erratum)

Fresh `disassemble_function 0x4BEC8` = **`XOR EAX,EAX ; RET`** (2 insns). It
returns **0 unconditionally**. Therefore the two wait gates
(`0x8A7D6 CALL 0x4BEC8; TEST AX,AX; JNZ epilogue` and the stage-2 twin
`0x8A895`) are **always open** — the sweep is never blocked by them. (FU-194
already noted "the `0x4BEC8` wait gates are derived ready"; here the reason is
first-hand: the function is a constant zero.)

Fresh `disassemble_function 0x4C374` =
**`PUSH EDX ; XOR EDX,EDX ; MOV word[0x157A68],DX ; POP EDX ; NOP`** (5 insns).
It **ignores EAX**. The `0x8A836 MOV EAX,0x19 ; CALL 0x4C374` therefore does
**not** install/rewrite code `0x19` (nor anything else): it only clears
`word[0x157A68]`. This is FU-194-L3, now first-hand reconfirmed.

> **Erratum (FU-212-E1).** The brief (and the engine comment it quotes,
> `fifa96_match_handlers.c:5120`) characterise act 8 as "the full 22-record
> code-`0xC`→`0x19` sweep". That is **not** a code rewrite: the sole writer in
> the select arm is `0x4C374`, which ignores its `EAX=0x19` argument (fresh §4).
> The sweep's whole per-record effect is to store the selected record pointer at
> `[0x1587F0]` and clear `word[0x157A68]`. No record's `code` changes.

The engine carries no cell for `word[0x157A68]` (fresh grep of `src/`+`include/`
finds it only in act-8 comments and the `period_setup` leg note at
`match_run.c:3097`); its reader is untraced (owner FU194-L4).

---

## 5. The engine today — the compressed model vs the ported sweep

### 5.1 The ported sweep (uncounted)

`fifa96_match_action_phase_1E` (`src/fifa96_engine/fifa96_match_run.c:6061..6111`)
is a faithful port of §2: stage 0 resets `act8_team`/`act8_record`/timer and
falls into stage 1; stage 1 sweeps one record per call and, on a `code==0xC &&
skip_99!=0 && skip_9a==0` record, stores it in `act8_selected` (native
`[0x1587F0]`; the `0x4C374` clear is the FU194-L3 leg); stage 2 re-dispatches
`store_15882b`/`store_15882c` via `fifa96_match_run_set_piece(..., BX=0)` then
`store_15882b = 0xFF`. The entity fields are first-hand: `code` = `+0x91`
(`match_entities.h:65`), `skip_99` = `+0x99` (`:89`), `skip_9a` = `+0x9A`
(`:90`). The body is **exported** (`match_run.h:1571`) and **directly unit-tested**
(`tests/test_engine_match_act.c:576..641`), but its phase-table slot is **NULL**
(`handlers.c:5119`) — i.e. **uncounted**.

### 5.2 The compressed model (the live substitute)

`fifa96_match_run_set_piece(mr, situation, side, bx)`
(`match_run.c:2977..3052`) models the native `FUN_0008A938`. Its `bx != 0` branch
(`match_run.c:3010..3023`) is the **compression** of the whole act-8 timeline:

```
mr->store_15882c = side;                 /* 0x8AA93 stored side   */
mr->store_15882b = situation;            /* 0x8AA9D stored sit    */
match_run_write_phase(mr, 0u);           /* 0x8AA89 phase-0 write */
fifa96_match_run_set_piece(mr, store_15882b, store_15882c, 0u);  /* 0x8A8CE re-dispatch */
store_15882b = 0xFF;                     /* 0x8A8D3              */
```

It performs the phase-0 write, then the stored-situation re-dispatch **in the same
frame**, and never dispatches act 8 (no engine source calls
`fifa96_match_act_dispatch(mr, 8, …)` — fresh grep). The engine's state cells
`act8_team`/`act8_record`/`act8_selected` exist and are reset at
`match_run.c:2096..2099` and `:2225..2228`, but nothing live drives them.

### 5.3 The census and the rider

The v18 census is `51/80 = 63%` (`test_engine_m2.c:2964`, `:3331`:
`action_wired == 32 && phase_wired == 19`). The phase-1E NULL is explicitly
asserted: `tests/test_engine_m2.c:2999`
`assert(fifa96_match_phase_table[0x1E].fn == NULL);  /* T5: ported, act-8
compression kept (FU194-L2) */`.

---

## 6. THE DE-COMPRESSION ANSWER

**Question.** If the engine's current compressed act-8 model
(`set_piece` `bx != 0`, §5.2) were replaced by the faithful 22-record sweep
(`fifa96_match_action_phase_1E`, §2/§5.1), would M2 (the tape) change?

**Answer — No. M2 does not move; no golden re-pin is forced.** The reason is
that the tape never enters the compressed path, so neither the compression nor
its replacement is ever observed by M2.

### 6.1 Which records/fields move under the faithful sweep (general)

The faithful sweep's **complete** write set is (from §2):

* `0x15882E` / `0x15882F` — the walk cursors (engine `act8_team`/`act8_record`);
* `word[0x158818]` — the timer (engine `mr->act.timer`);
* `byte[0x158829]` — the stage byte;
* `[0x1587F0]` — the selected-record pointer (engine `mr->act8_selected`);
* `word[0x157A68]` — cleared by `0x4C374` (no engine cell);
* at stage 2: `[0x158808]` (act slot), `[0x158829]`, `[0x158828]`, then the
  `CALL 0x8A938` re-dispatch, then `byte[0x15882B] = 0xFF`.

Against the **render-read set** — `match_run_scene_stage` reads exactly
`pos_x/pos_y/pos_z`, `anim_id`, `frame`, and `hidden = skip_9a ? 1 : 0`
(`match_run.c:1598..1607`, re-verified first-hand this session) — **none** of the
sweep's writes is render-visible. The sweep never writes any record's
position/anim/frame/`skip_9a`; it only *reads* `code`/`skip_99`/`skip_9a`.
Therefore **no record and no render-read field moves**; the only behavioural
delta of the sweep vs the compression is **timing** — the stored-situation
re-dispatch (and hence its downstream phase writes) is delayed by **~22 granted
frames** (the sweep) instead of firing in the same frame (the compression).

### 6.2 Does the tape ever stage act 8? — No (fresh tape evidence)

The compressed `bx != 0` path is entered only from the engine's `bx=1`
`set_piece` call sites; every one is dormant on M2:

| site | producer | M2 status |
|---|---|---|
| `match_run.c:5628` (`ref_apply`, `out->situation`) | the referee/foul sequence machine | **dormant** — tape `ref_machine == FIFA96_MATCH_RUN_REF_NONE` (`match_run.c:2108`/`:2237`; nothing sets it on the goal path) |
| `match_run.c:3137` (throw-in arm, `set_piece(2, side, 1)`) | `fifa96_match_run_goal_scan`, arm taken only when `abs_z <= 0xB20` | **not taken** — see §6.2 golden census |
| `match_run.c:5883` (`match_run_phase_1A_end`) | the phase-`0x1A` terminal (act-4) | **dormant** — phase 0x1A slot NULL, act-4 producer unported (FU201-L1) |
| `match_run.c:6233` (`out.restart`, `set_piece(9, side, 1)`) | the referee decision (foul restart) | **dormant** — no foul on the tape |

The tape's own set-piece calls are **all `bx=0`**: `begin` →
`period_setup(1)` → `set_piece(0,0,0)` (`match_run.c:3072`); the natural goal →
`goal_queue` → `set_piece(6, side, 0)` (`:2924`); the post-goal restart chain →
`set_piece(0x0C, 0, 0)` (`:3362`) and the sit-3 corner arm `set_piece(3, 0, 0)`
(`:3411`).

**Decisive tape evidence — the golden's per-frame phase census.** The transcript
appends `state=<phase>/<home>-<away>` per live frame with `%u`
(`test_engine_m2.c:1930..1936`). Fresh `grep -o "state=…" ` over the committed
golden `tests/golden/engine/m2-frames.txt` (sha `441415fc…`) yields exactly:

| `state=` phase | count |
|---|---|
| `0` | 304 |
| `2` | 40 |
| `4` | 303 |
| `19` (=0x13) | 20 |
| `20` (=0x14) | 20 |
| `21` (=0x15) | 100 |

**There is no `state=3`.** Phase 3 is precisely what the throw-in arm
(`set_piece(2, …)`) produces (a unit test pins `set_piece(2,1,1)` → phase 3,
`test_engine_match_frame.c:3353`). Its absence, together with the pinned restart
sequence `0x15 → 4 (corner, `corner_count[0]==1`) → 2` (`test_engine_m2.c:3560..3599`)
and the exact `M2_WIRED_MASK`/`M2_RESTART_ROWS_MASK`, proves the tape never takes
the throw-in arm and never reaches the compressed `bx != 0` path. (The tape's
`session_gate_14c32a == 0`, `test_engine_m2.c:2663`, so a firing `set_piece(2,…)`
would take the direct path and hit the compression — hence the phase-3 absence is
load-bearing.)

So the tape **never stages act 8** — not as a dispatched phase, not as a record
code (the tape's action-code 0x1E bit in `M2_STAGED_ROWS_MASK` is the **ACTION
row `0x1E`** `fifa96_match_action_1E`, `handlers.c:5026`, a name-collision
distinct from this phase `0x1E`), and not through the compressed path.

### 6.3 What the engine must add/change (if the faithful sweep is landed)

Since M2 is unaffected, the change is behavioural/structural, not tape-visible:

1. **Re-route the `bx != 0` branch** (`match_run.c:3010..3023`) from the
   in-frame re-dispatch to a real act-8 dispatch, e.g.
   `fifa96_match_act_dispatch(mr, 8, 0, 1)` after the phase-0 write, and let the
   act tick drive `fifa96_match_action_phase_1E` across the sweep frames
   (the native `CALL 0x888FC(8,0,BX=1)`).
2. **Register the phase-`0x1E` slot** in `fifa96_match_phase_table`
   (`handlers.c:5119` NULL → `fifa96_match_action_phase_1E`).
3. **Update the census riders + headline** (not the golden): `phase_wired`
   `19 → 20`, `51 → 52/80 = 65%` (`test_engine_m2.c:2964`, `:3331`;
   `run_census_riders` `:2999`); recompute the README title and the
   `docs/ENGINE.md` census line per the AGENTS.md derived-percentage rule.
4. **Rewrite the `bx=1` unit tests** (`tests/test_engine_match_frame.c:3344..3368`,
   `test_set_piece_bx_fallback`), which today pin the *compressed* behaviour —
   e.g. `set_piece(2,1,1)` resolving to phase 3 in the same call and
   `store_15882b == 0xFF` in the same frame. Under the faithful sweep the
   re-dispatch is deferred, so these assertions must be re-expressed (dispatch +
   tick), and the `phase_1E` unit tests (`test_engine_match_act.c:576..641`)
   already cover the sweep itself.

### 6.4 Is a re-pin forced?

**No golden re-pin.** M2's `tests/golden/engine/m2-frames.txt`
(`441415fc…`, 792 lines) is **not** forced to move: the compressed path is
dormant on the tape (§6.2), so no presented frame changes. What *is* forced is the
**census assertion block** in `test_engine_m2.c` (a test artifact, not the
golden) and the derived README/ENGINE.md percentage — because registering the
slot changes the wired count. This is the ordinary "mask/strata grows, golden
holds" shape (the v15/v17 precedents, `test_engine_m2.c:1554..1560`,
`:1389..1393`). If a future tape *did* exercise the throw-in/ref/restart `bx=1`
path, the ~22-frame deferral **would** be tape-visible and would force a
discussed re-pin — but that is a property of a hypothetical tape, not of M2.

**One-sentence verdict.** The 22-record sweep is render-invisible (selects a
pointer, clears `[0x157A68]`, never rewrites a code), the M2 tape never enters
the compressed `bx != 0` path (its golden carries no `state=3`), so replacing the
compression with the faithful sweep moves M2 not at all — the cost is a
`phase_wired 19→20` test/census update, not a golden re-pin.

---

## 7. Numbered legs

1. **FU212-L1 — non-tape compression equivalence is unproven.** This slice
   establishes only that the compression is *dormant on M2*. Whether the
   compressed `set_piece(bx != 0)` is behaviourally equivalent to the faithful
   sweep on the **non-tape** paths (the referee/foul restart, the goal_scan
   throw-in arm, the phase-`0x1A` terminal) is not decomposed here; the ~22-frame
   deferral could matter there. Owner: the act-8 landing (if any); FU-149 L6 /
   FU194-L2 remain the reference.
2. **FU212-L2 — `word[0x157A68]`.** Cleared by `0x4C374` (ignoring EAX) and by
   `FUN_0004B02C`; no engine cell, reader untraced. Owner: FU194-L4.
3. **FU212-L3 — act-slot residency across the sweep.** The native act slot
   `[0x158808]` holds the act-8 handler and is ticked each frame for the ~22
   sweep frames (stage 1). The engine's single `mr->act` slot must survive
   unchanged for those frames while the per-record phase walk and clock tail run;
   whether any concurrent engine sub-machine would overwrite it is not proven
   here (it is moot while the slot is unregistered).
4. **FU212-L4 — the selection predicate `+0x99`.** `byte[rec+0x99]` is read as a
   non-zero gate in the sweep (`match_entities.h:89` `skip_99`, unlabelled) and
   its semantics are not documented in the port; `+0x9A` is the occupied/skip
   byte (`skip_9a`). The exact meaning of `+0x99` (owner of the write) is open.
5. **FU212-L5 — the stage-2 re-dispatch target.** `CALL 0x8A938(sit, side, BX=0)`
   resolves through the table-2 route (`§3`); the engine's equivalent
   `set_piece(..., 0)` is faithful, but the interaction of the deferred
   re-dispatch with the global act slot (does it re-arm the same act?) is not
   traced. Moot while act 8 is undispatched.
6. **FU212-L6 — the `0x8A8E0` BX==0 dispatch table.** The 9-entry table at
   `0x8A8E0` (fresh read: `0x8A99E, 0x8A99E, 0x8A99E, 0x8A9CD, 0x8A9E8, 0x8A9CD,
   0x8AA60, 0x8AA23, 0x8AA23`, for situations 2..0xA) is the BX==0 route; its
   per-situation arms are only partially mirrored in
   `fifa96_match_run_set_piece`'s table-2 block and are not re-derived here
   (already owned by the FU-146/FU-149 situation-machine legs).

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled;
`.image` overlay present; `get_current_program_info` confirmed this session):

* `disassemble_function` — `0x8A798` (no function), `0x4C374` (5 insns),
  `0x4BEC8` (2 insns).
* `disassemble_bytes` — `0x8A798` (328 B, 91 insns, the full act-8 body),
  `0x8A938` (424 B, the `FUN_0008A938` head + BX!=0 fallback + table-2 arms),
  `0x8AF00` (37 B, the shared `0x8AF1A CALL 0x888FC`).
* `read_memory` — `0x110794` (140 B, the phase table), `0x1107EC` (52 B, the act
  table), `0x8A8E0` (36 B, the BX==0 dispatch table).
* `get_xrefs_to` — `0x8A798` (1, DATA, from `0x11080C`), `0x8AF1A` (1,
  UNCONDITIONAL_JUMP, from `0x8AAA3`).
* `get_current_program_info`.

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(1598..1607 the render reads; 2076/2096..2099/2203/2225..2228 the act-8 resets;
2920..2925 goal_queue; 2977..3052 set_piece incl. the `bx!=0` compression
3010..3023; 3063..3099 period_setup; 3113..3185 goal_scan incl. the throw-in arm
3137; 3990..4029 the clock tail + goal_scan call; 5560..5632 ref_apply;
5883 phase_1A_end; 6061..6111 phase_1E; 6233 the restart arm; 6338..6375 the
ref supervisor); `src/fifa96_engine/fifa96_match_handlers.c` (5026 the action
row 0x1E; 5119..5128 the phase table; 5120 the compression comment);
`include/fifa96_engine/fifa96_match_run.h` (766..775 the act-8 cells; 1230..1232
the set_piece contract; 1555..1571 the phase_1E header);
`include/fifa96_engine/fifa96_match_entities.h` (65 code / 89 skip_99 / 90
skip_9a); `tests/test_engine_m2.c` (1549..1590 the masks; 1930..1936
`append_state`; 2525..2847 `run_tape`; 2964/2999/3331 the census riders;
3488..3615 `main` pins; 2663 the session-gate assert); `tests/test_engine_match_frame.c`
(3344..3368 `test_set_piece_bx_fallback`; 3353 the sit-2→phase-3 pin);
`tests/test_engine_match_act.c` (576..641 the phase_1E unit tests); the golden
`tests/golden/engine/m2-frames.txt` (sha `441415fc…`, the per-frame `state=`
census). Docs cross-cited: FU-149 L6, FU-175 §2/§5, FU-188, FU-189, FU-194
(§4/§6.2/L2/L3), FU-206 §5 (PR-9). No writes beyond this slice file; no
rename/label/comment/function/script/project save; no git.
