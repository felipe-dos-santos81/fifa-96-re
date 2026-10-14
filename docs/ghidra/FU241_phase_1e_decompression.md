# W23-2 / FU-241 — phase `0x1E` / act 8 restart-sweep de-compression (re-derivation)

Provenance: M2 phase-23 wave W23-2, 2026-10-14, read-only recon. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `/FIFA96.EXE`,
`watcom:LE:32:default`, 2706 functions, 15239 symbols, `.image` overlay present,
min address `0x10000`, max `.image::0x174a2a`). Every address/byte below is a
fresh `disassemble_bytes` / `read_memory` / `disassemble_function` /
`get_xrefs_to` / `get_current_program_info` of **this** session unless marked
*cross-cited* (then the frozen FU doc or the engine source is named). LE-image
globals sit at **`+0x100000`** from the file offset; the addresses here are
`/FIFA96.EXE` runtime (program) addresses. Alias traps applied: `dword[x]>>16` =
the word at `x+2`, `>>24` = the byte at `x+3`. Engine source read first-hand:
`src/fifa96_engine/fifa96_match_run.c`, `fifa96_match_handlers.c`,
`fifa96_loader/fifa96_action_handlers.c`, `include/fifa96_engine/fifa96_match_run.h`,
`tests/test_engine_match_frame.c`, `tests/test_engine_match_act.c`,
`tests/test_engine_m2.c`, the golden `tests/golden/engine/m2-frames.txt`.
**No writes**: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; the ISO never touched; no git commands. This is one new
file of the slice (draft) plus the run report.

> **This is a re-derivation, not a restatement of FU-212 §6.** Every load-bearing
> claim below was independently re-read from the live program and the live
> engine/golden this session. Where the result matches FU-212 it is stated as a
> reconfirmation; where the frozen doc's anchors have gone stale it is flagged
> (§7-E1).

---

## 1. Fresh anchors — the slot, the handler, the sole xref

- Fresh `read_memory 0x110794` (140 B, spanning the phase table and the following
  act table) + fresh `get_xrefs_to 0x8A798` = **1** reference, **DATA, from
  `0x11080C`** = `0x110794 + 0x1E*4`. So the phase-table slot `0x110794[0x1E] =
  0x8A798` and the **phase-table slot is the sole reference** — there is no direct
  call site. (FU-212 §1 reconfirmed first-hand.)
- Fresh `read_memory 0x1107EC` (52 B, the act table for acts 0..0xC) — index
  `8` (offset `8*4 = 0x20`) bytes `98 a7 08 00` → `0x1107EC[8] = 0x8A798`. Thus
  **act 8 → phase `0x16+8 = 0x1E`** (the FU165-L9 dual identity): the same handler
  is reachable as the phase-`0x1E` body and as act 8.
- `disassemble_function 0x8A798` is not re-run here, but `disassemble_bytes
  0x8A798` (328 B) succeeds and yields **91 instructions** ending `0x8A8DD RET`
  (`0x8A8DE MOV EAX,EAX` = padding; `0x8A8E0` begins the BX==0 dispatch table).
  Handler body = **`0x8A798..0x8A8DD`**.

---

## 2. The complete body timeline (fresh `disassemble_bytes 0x8A798`, 91 insns)

### 2.0 Head — the stage read

```
0x8A798 PUSH EBX ; PUSH ECX ; PUSH EDX
0x8A79B MOV AL,[0x00158829]     ; the stage byte
0x8A7A0 CMP AL,0x1
0x8A7A2 JC  0x8A7B2             ; AL<1 -> stage 0
0x8A7A4 JBE 0x8A7D6             ; AL==1 -> stage 1
0x8A7A6 CMP AL,0x2
0x8A7A8 JZ  0x8A895             ; AL==2 -> stage 2
0x8A7AE POP EDX ; POP ECX ; POP EBX ; RET
```

No timer add in the head (it reads the stage directly) — unlike the `phase_1D`/
`_21`/`_22` heads. Reconfirmed first-hand.

### 2.1 Stage 0 (`0x8A7B2`) — reset, then fall through into stage 1

```
0x8A7B2 TEST AL,AL ; JNZ 0x8A8DA        ; unreachable-taken (AL==0 here)
0x8A7BA MOV [0x15882E],AL        ; team cursor  = 0
0x8A7BF XOR BL,BL
0x8A7C1 MOV [0x15882F],AL        ; record cursor= 0
0x8A7C6 XOR BH,BH
0x8A7C8 INC AL                   ; AL = 1
0x8A7CA MOV word[0x158818],BX    ; timer = 0
0x8A7D1 MOV [0x158829],AL        ; stage = 1
0x8A7D6 (fall through, no jump)
```

`JNZ 0x8A8DA` is unreachable-taken (`JC 0x8A7A2` admitted only `AL==0`).

### 2.2 Stage 1 (`0x8A7D6`) — the 22-record sweep, one record per call

```
0x8A7D6 CALL 0x4BEC8 ; TEST AX,AX ; JNZ 0x8A8DA       ; wait gate (§4, always open)
0x8A7E4 MOV EAX,[0x15882B] ; SAR EAX,0x18             ; byte 0x15882E = TEAM idx
0x8A7EC IMUL EAX,EAX,0x835 ; EDX=0x1588A4 ; ADD EDX,EAX  ; team base
0x8A7F9 MOV EAX,[0x15882C] ; SAR EAX,0x18             ; byte 0x15882F = RECORD idx
0x8A801 IMUL EAX,EAX,0xB2 ; ADD EAX,EDX               ; rec ptr
0x8A809 MOV EDX,[EAX+0x8E] ; XOR EBX,EBX ; SAR EDX,0x18  ; byte[rec+0x91] = CODE
0x8A814 MOV dword[0x1587F0],EBX (0)                   ; clear the selection cell
0x8A81A CMP EDX,0xC ; JNZ 0x8A840                     ; code != 0xC -> next
0x8A81F CMP byte[EAX+0x99],0 ; JZ 0x8A840             ; +0x99 == 0 -> next
0x8A828 CMP byte[EAX+0x9A],0 ; JNZ 0x8A840            ; +0x9A != 0 -> next
0x8A831 MOV [0x1587F0],EAX                            ; SELECT this record
0x8A836 MOV EAX,0x19 ; CALL 0x4C374                   ; clears word[0x157A68] (§4)
0x8A840 INC byte[0x15882F]                            ; record++
0x8A846 MOV EAX,[0x15882C] ; SAR EAX,0x18
0x8A84E CMP EAX,0xB ; JL 0x8A8DA                      ; record < 11 -> return
0x8A857 DL=[0x15882E] ; INC DL ; XOR AH,AH            ; team++
0x8A861 MOV [0x15882E],DL ; MOV [0x15882F],AH(0)      ; team++ ; record = 0
0x8A86D MOV EAX,[0x15882B] ; SAR EAX,0x18
0x8A875 CMP EAX,0x1 ; JLE 0x8A8DA                     ; team <= 1 -> return
0x8A87A DH=[0x158829] ; XOR ECX,ECX ; INC DH
0x8A884 MOV word[0x158818],CX(0)                      ; timer = 0
0x8A88B MOV [0x158829],DH                             ; stage = 2
0x8A891 POP EDX ; POP ECX ; POP EBX ; RET
```

**The sweep order.** Team cursor `byte[0x15882E]`, record cursor `byte[0x15882F]`,
record stride `0xB2`, team stride `0x835`, team base `0x1588A4`. Each stage-1
call processes exactly **one** record and returns (except the team rollover).
Traced from the bytes: call #1 enters via stage 0 (reset, `stage=1`) then
**falls through** into stage 1 and processes **(0,0)**; calls #2..#11 process
`(0,1)…(0,10)`; after `(0,10)` → `team 1, record 0`; calls #12..#22 process
`(1,0)…(1,10)`; after `(1,10)` → `team 2` → `timer=0, stage=2`. Call #23 is
stage 2. So the sweep is **22 records = 2 teams × 11**, **one per granted frame**,
terminating on the **23rd** granted frame.

**Per-record effect = a selection + a `[0x157A68]` clear.** For
`code[rec+0x91]==0xC AND byte[rec+0x99]!=0 AND byte[rec+0x9A]==0`, the sweep
stores the record pointer in `[0x1587F0]` (`0x8A831`) and calls `0x4C374` with
`EAX=0x19` (`0x8A836`). It writes **no record field**.

### 2.3 Stage 2 (`0x8A895`) — the terminal re-dispatch

```
0x8A895 CALL 0x4BEC8 ; TEST AX,AX ; JNZ 0x8A8DA       ; wait gate (§4)
0x8A89F MOV [0x158818],AX (0)                         ; timer = 0
0x8A8A5 XOR EDX,EDX ; XOR EBX,EBX ; XOR AH,AH
0x8A8AB MOV dword[0x158808],EDX (0)                   ; act slot clear
0x8A8B1 MOV byte[0x158829],AH (0)                     ; stage clear
0x8A8B7 MOV byte[0x158828],AH (0)                     ; act id clear
0x8A8BD MOV EDX,[0x158829] ; SAR EDX,0x18             ; = byte 0x15882C = SIDE
0x8A8C3 MOV EAX,[0x158828] ; SAR EAX,0x18             ; = byte 0x15882B = SITUATION
0x8A8CE CALL 0x8A938                                  ; re-dispatch (sit, side), BX=0
0x8A8D3 MOV byte[0x15882B],0xFF                       ; clear stored situation
0x8A8DA POP EDX ; POP ECX ; POP EBX ; RET
```

**Alias reconciliation (fresh).** The clears at `0x8A8B1`/`0x8A8B7` land on the
**low** bytes `0x158829`/`0x158828`; the later dword reads `[0x158829]`
(`0x8A8BD`) and `[0x158828]` (`0x8A8C3`) have `>>24` = **byte `0x15882C`
(side)** and **byte `0x15882B` (situation)** respectively — so the clears do **not**
erase the re-dispatch arguments. `CALL 0x8A938` bytes `e8 65 00 00 00` → target
`0x8A8D3 + 0x65 = 0x8A938` (agrees with Ghidra). `0x8A8D3` writes `0x15882B =
0xFF`.

Act 8 is therefore a **restart sweep that re-dispatches the stored
situation/side**, reachable only after 22 stage-1 records (or the rollover path).

---

## 3. The producer chain — `FUN_0008A938` BX!=0 fallback (fresh `0x8A938`)

Fresh `disassemble_bytes 0x8A938` (430 B, 113 insns). Head routes to the act-8
fallback `0x8AA7B` when:

```
0x8A944 TEST AX,AX ; 0x8A947 JZ 0x8AA7B      ; situation == 0
0x8A94E CMP EAX,0xB ; 0x8A951 JZ 0x8AA7B     ; situation == 0xB
0x8A957 CMP byte[0x14C32A],0 ; 0x8A95E JZ 0x8AA7B   ; session gate closed
0x8A964 CMP dword[0x15B6C0],0 ; 0x8A96B JNZ 0x8AA7B ; a situation pending
```

Otherwise situations 2..0xA take `0x8A996 JMP dword CS:[EAX*4+0x8A8E0]` and
>0xA takes the `0x8AA60` default (id 0xA) — the table-1 queue arm. The fallback:

```
0x8AA7B TEST BX,BX ; 0x8AA7E JZ 0x8AAA8      ; BX==0 -> table-2 route
0x8AA80 XOR EDX,EDX ; XOR EAX,EAX ; MOV EBX,1
0x8AA89 CALL 0x740A0                          ; write PHASE 0 (EAX=0, EDX=0)
0x8AA8E MOV AL,[ESP]  ; XOR EDX,EDX
0x8AA93 MOV [0x15882C],AL                     ; stored SIDE
0x8AA98 MOV EAX,8                             ; b8 08 00 00 00   <<< ACT 8
0x8AA9D MOV byte[0x15882B],CL                 ; stored SITUATION (CL = situation)
0x8AAA3 JMP 0x8AF1A                           ; e9 72 04 00 00
```

Fresh `get_xrefs_to 0x8AF1A` = **1**, UNCONDITIONAL_JUMP from `0x8AAA3`. Fresh
`disassemble_bytes 0x8AF00` (40 B): `0x8AF0E MOV EBX,1 ; 0x8AF13 MOV EAX,9 ;
0x8AF18 XOR EDX,EDX ; 0x8AF1A CALL 0x888FC` (`e8dd d9ff ff` → `0x8AF1F +
0xFFFFD9DD = 0x888FC`). The `0x8AF0E` sit-0xC arm **falls through** into the same
`CALL`, so `0x8AAA3` is the sole branch xref into the shared site.

**Producer verdict.** Act 8's producer is the **`FUN_0008A938` BX!=0 fallback**,
reached with situation 0/0xB or a closed-gate/pending state — **not** a literal
`EAX=8` site (reconfirmed). It dispatches act 8 **invoke-now** (`EBX=1`,
`EDX=0`); the sweep spans the next ~22 granted frames; stage 2 re-dispatches the
stored situation.

---

## 4. The gates — `0x4BEC8` and `0x4C374` (fresh, first-hand)

- Fresh `disassemble_function 0x4BEC8` = **`XOR EAX,EAX ; RET`** (2 insns) →
  returns **0 unconditionally**. Both wait gates (`0x8A7D6`, `0x8A895`) are
  therefore **always open**; the sweep cadence is the granted-frame tick alone.
- Fresh `disassemble_function 0x4C374` =
  **`PUSH EDX ; XOR EDX,EDX ; MOV word[0x157A68],DX ; POP EDX ; NOP`** (5 insns).
  It **ignores EAX**. The `0x8A836 MOV EAX,0x19 ; CALL 0x4C374` therefore does
  **not** install/rewrite code `0x19` (or anything else): it only clears
  `word[0x157A68]`.

> **Erratum (reconfirmed, FU-212-E1).** The sweep is **not** a code-`0xC`→`0x19`
> rewrite. Its whole per-record effect is the pointer store into `[0x1587F0]`
> plus the `word[0x157A68]` clear. **No record's `code` changes.**

The engine carries no cell for `word[0x157A68]` (fresh `grep -rn 157A68
src/ include/`: only comments at `match_run.c:3245/6353/6382`, `match_run.h:1660`,
`handlers.c:5250`, and the act-8 body comment); its reader is untraced
(owner FU194-L4).

---

## 5. The engine today — compressed model vs the ported sweep

### 5.1 The ported sweep (uncounted)

`fifa96_match_action_phase_1E` (`src/fifa96_engine/fifa96_match_run.c:6359..6407`)
is a faithful port of §2: stage 0 resets `act8_team`/`act8_record`/`act.timer` and
falls into stage 1; stage 1 sweeps one record per call, clears `act8_selected`
before every gate test (`0x8A814`), and on `code==0xC && skip_99!=0 &&
skip_9a==0` stores the record in `act8_selected` (native `[0x1587F0]`; the
`0x4C374` clear is the FU194-L3 leg); stage 2 re-dispatches
`store_15882b`/`store_15882c` via `fifa96_match_run_set_piece(..., BX=0)` then
`store_15882b = 0xFF`. Entity fields first-hand: `code` = `+0x91`
(`match_entities.h:65`), `skip_99` = `+0x99`, `skip_9a` = `+0x9A`. The body is
**exported** (`match_run.h:1666`) and **directly unit-tested**
(`tests/test_engine_match_act.c:611..646`), but its phase-table slot is **NULL**
(`handlers.c:5249` `{0x1E, NULL, …}`) — **uncounted**.

### 5.2 The compressed model (the live substitute)

`fifa96_match_run_set_piece(mr, situation, side, bx)`
(`match_run.c:3112..3200`) models the native `FUN_0008A938`. Its `bx != 0`
branch (`match_run.c:3145..3157`) is the **compression** of the whole act-8
timeline:

```
mr->store_15882c = side;                 /* 0x8AA93 stored side   */
mr->store_15882b = situation;            /* 0x8AA9D stored sit    */
match_run_write_phase(mr, 0u);           /* 0x8AA89 phase-0 write */
fifa96_match_run_set_piece(mr, mr->store_15882b, mr->store_15882c, 0u);  /* 0x8A8CE */
mr->store_15882b = 0xFFu;                /* 0x8A8D3 */
```

It performs the phase-0 write and the stored-situation re-dispatch **in the same
frame**, and never dispatches act 8 (fresh `grep -n
"fifa96_match_run_set_piece(" src/` shows the only callers; the bx=1 callers are
`handlers.c:1718`, `match_run.c:3300`, `:5907`, `:6162`, `:6535`). The engine's
state cells `act8_team`/`act8_record`/`act8_selected` exist and are reset at
`match_run.c:2102..2104` and `:2236..2238`, but nothing live drives them.

### 5.3 The render-read set (fresh source)

`match_run_scene_stage` (`match_run.c:1595..1621`) reads, per pool record, exactly
**`pos_x`/`pos_y`/`pos_z`** (`:1602..1604`), **`anim_id`** (`:1605`), **`frame`**
(`:1606`), and **`hidden = skip_9a ? 1 : 0`** (`:1607`); the ball slot reads the
ball record fields (`:1611..1620`). The sweep's complete write set is
`act8_team`/`act8_record`/`act8_selected` (`mr->act8_*`), `mr->act.timer`,
`mr->act.stage`, `mr->act.id`, `store_15882c`, `store_15882b` — plus the native
`word[0x157A68]` (no engine cell) and the `[0x1587F0]` pointer. **None** of these
is a render-read field, and the sweep only *reads* `code`/`skip_99`/`skip_9a`.
Hence the sweep is **render-invisible**.

---

## 6. THE DE-COMPRESSION VERDICT (fresh re-derivation)

**Claim.** Replacing the compressed `set_piece` `bx != 0` model (§5.2) with the
faithful 22-record sweep (registering the phase-`0x1E` slot and routing `bx!=0`
to a real act-8 dispatch) moves **no** M2 presented frame and **no** M1 frame.

### 6.1 The sweep rewrites no record code (first-hand, §4)

`0x4C374` ignores `EAX` and only clears `word[0x157A68]`; the select arm's sole
writer is that call. The sweep stores a pointer and advances cursors/stage/timer;
it touches no `code`, position, animation, frame or `skip_9a`. Against §5.3, **no
record and no render-read field moves**; the only delta vs the compression is
**timing** — the stored-situation re-dispatch (and any phase it writes / act it
arms) is deferred by ~22 granted frames instead of firing in-frame.

### 6.2 The golden never enters the compressed `bx != 0` path (fresh tape evidence)

The compressed path is entered **only** from `bx=1` `set_piece` calls. First-hand
classification of every `bx=1` site and its tape status:

| site | producer | output signature | M2 status |
|---|---|---|---|
| `match_run.c:3300` (goal_scan throw-in, `set_piece(2, …, 1)`) | requires `state.phase==2 && goal_armed && zone!=0 && abs_z<=0xB20` | sit 2 → **phase 3** | not taken — golden has **no `state=3`** |
| `handlers.c:1718` (set-piece stage-4 re-queue, `set_piece(2, …, 1)`) | the set-piece/ref sequence machine | sit 2 → **phase 3** | not taken — **no `state=3`** |
| `match_run.c:6162` (`match_run_phase_1A_end`, `set_piece(store_15881e, …, 1)`) | the phase-`0x1A` terminal | sit ∈ {2,3,4} → phase 3/4/8 | **structurally dead** — phase-`0x1A` slot is NULL (`handlers.c:5245`, rider `test_engine_m2.c:3698`); the terminal never runs |
| `match_run.c:5907` (`ref_apply`, `out->situation`, bx=1) | the referee/foul sequence machine | sit-dependent | dormant — tape asserts `ref_machine == FIFA96_MATCH_RUN_REF_NONE` (`test_engine_m2.c:2447`, `:2849`, inside the `m2_directives` driver) and `ref_whistle == 0` |
| `match_run.c:6535` (referee restart decision, `set_piece(9, …, 1)`) | the referee decision (foul restart) | sit 9 → act 2 → phase `0x18` | dormant — no foul; golden has **no `state=18`** |

The tape's own set-piece calls are all **`bx=0`**: `begin → period_setup(1) →
set_piece(0,0,0)` (`match_run.c:3220`); the natural goal → `goal_queue` →
`set_piece(6, side, 0)` (`:3059`); the post-goal restart chain →
`set_piece(0x0C,0,0)` (`:3525`, act 9) and the sit-3 corner arm
`set_piece(3,0,0)` (`:3574`).

**Decisive fresh tape evidence — the golden's per-frame phase census.** The
transcript appends `state=<phase>/<home>-<away>` per live frame
(`test_engine_m2.c:23`). Fresh `grep -o "state=[0-9-]*" tests/golden/engine/
m2-frames.txt | sort | uniq -c` (sha `b281a9b2…`, 6522 lines) yields exactly:

| `state=` phase | count |
|---|---|
| `0` | 304 |
| `1` | 953 |
| `2` | 2193 |
| `4` | 303 |
| `12` (=0x0C) | 4 |
| `19` (=0x13) | 2640 |
| `20` (=0x14) | 20 |
| `21` (=0x15) | 100 |

**There is no `state=3`** (and no `state=18`). Phase 3 is precisely what the
sit-2 compressed entry produces (the pin `test_phase_1E_compressed_model_pin`
asserts `set_piece(2,1,1)` → phase 3; §6.3). Its absence rules out both live
sit-2 `bx=1` arms; the phase-`0x1A` site is structurally dead; the two referee
sites are pinned off. Together: **the golden never enters the compressed
`bx != 0` path** — so neither the compression nor its replacement (the faithful
sweep) is ever observed by M2.

> **Reconfirmed.** This matches FU-212 §6.2's conclusion. It is re-derived here
> against the **current** golden (`b281a9b2…`); see §7-E1 for the stale anchor in
> the frozen doc.

### 6.3 Exact pin-lift terms + which tests/pins flip

To honestly lift the compressed-model pin, the following must change (tests and
census only — **no golden re-pin**):

1. **Register the slot** `fifa96_match_phase_table[0x1E]`: `handlers.c:5249`
   `{0x1E, NULL, …}` → `{0x1E, fifa96_match_action_phase_1E, …}`.
2. **Re-route the `bx != 0` branch** (`match_run.c:3145..3157`): after
   `match_run_write_phase(mr, 0u)`, replace the in-frame
   `fifa96_match_run_set_piece(..., 0u)` call with
   `fifa96_match_act_dispatch(mr, 8, 0, 1)` (the native
   `CALL 0x888FC(8,0,BX=1)`); remove the immediate `store_15882b = 0xFF` (now
   deferred to the sweep's stage-2 terminal, `match_run.c:6405`).
3. **Flip `tests/test_engine_match_frame.c::test_phase_1E_compressed_model_pin`
   (`:3642..3663`)**: the `fn == NULL` assert must become `fn != NULL`; the
   `set_piece(2,1,1) → phase 3` and `store_15882b == 0xFF` asserts must be
   re-expressed (now: `state.phase == 0` from the phase-0 write; `store_15882b
   == 2u` until the terminal; `act.id == 8`; `act8_team/record` reset/advanced by
   the sweep's stage 0→1).
4. **Flip `tests/test_engine_match_act.c::test_phase_1E_not_counted`
   (`:598..603`)**: `fn == NULL` → `fn != NULL` and
   `fifa96_match_dispatch_phase(&mr, 0x1E) == FIFA96_ERR_UNSUPPORTED` → `… ==
   FIFA96_OK`.
5. **Flip the census riders** `tests/test_engine_m2.c`:
   `run_census_riders` `:3666` `action_wired == 33u && phase_wired == 22u` →
   `phase_wired == 23u` (the `:3701` `phase_table[0x1E].fn == NULL` pin →
   `fn != NULL`); the `main` mirror `:4041` likewise. `55/80` → `56/80`.
6. **Recompute the derived headline**: README title `68%` → `70%` and the
   `docs/ENGINE.md` census line (`:8`, `:268`, `:1236`, `:1258`, `:1315`), per
   the AGENTS.md derived-percentage rule. Update the doc comments
   (`handlers.c:5249..5250`, `match_run.c:6344..6358`, `match_run.h:1648..1665`).
7. **Golden pins: unchanged.** M2 `tests/golden/engine/m2-frames.txt` stays
   `b281a9b2…` **byte-identical**; M1 `tests/golden/engine/m1-frames.txt` stays
   `09b726b7…` (the AGENTS.md immutable pin) unmoved.

---

## 7. Numbered legs

1. **FU241-L1 — non-tape compression equivalence unproven.** This slice proves
   only that the compression is **dormant on M2**. Whether the compressed
   `set_piece(bx != 0)` is behaviourally equivalent to the faithful sweep on the
   **non-tape** paths (referee/foul restart, goal_scan throw-in, phase-`0x1A`
   terminal) is not decomposed here; the ~22-frame deferral could matter there.
   Owner: the act-8 landing. Reference: FU212-L1 / FU194-L2.
2. **FU241-L2 — `word[0x157A68]`.** Cleared by `0x4C374` (ignoring EAX) at
   `0x8A836`; no engine cell, reader untraced. Owner: FU194-L4.
3. **FU241-L3 — act-slot residency across the sweep.** The native act slot
   `[0x158808]` holds the act-8 handler and is ticked each frame for the ~22
   sweep frames. The engine's single `mr->act` slot must survive untouched for
   those frames while the per-record phase walk and clock tail run; whether any
   concurrent engine sub-machine would overwrite it is not proven here. Owner:
   the act-8 landing. Reference: FU212-L3.
4. **FU241-L4 — the selection predicate `+0x99`.** `byte[rec+0x99]` is read as a
   non-zero gate in the sweep (`skip_99`); its semantics/owner is undocumented in
   the port (`+0x9A` is the occupied/skip byte). Owner: FU212-L4.
5. **FU241-L5 — the stage-2 re-dispatch target.** `CALL 0x8A938(sit, side, BX=0)`
   resolves through the table-2 route; the interaction of the deferred
   re-dispatch with the global act slot is not traced. Moot while act 8 is
   undispatched. Reference: FU212-L5.
6. **FU241-L6 — the `0x8A8E0` BX==0 dispatch table.** The 9-entry table
   (`0x8A99E, 0x8A99E, 0x8A99E, 0x8A9CD, 0x8A9E8, 0x8A9CD, 0x8AA60, 0x8AA23,
   0x8AA23` for situations 2..0xA) is the BX==0 route; its per-situation arms are
   only partially mirrored in `fifa96_match_run_set_piece`'s table-2 block
   (first-hand `0x8AA23`/`0x8AA45` arm writes `[0x15B6A8] ∈ {2, 1}`). Owner: the
   FU-146/FU-149 situation-machine legs. Reference: FU212-L6.
7. **FU241-L7 — the M2 golden anchor has moved since FU-212.** The frozen
   FU-212 §6.2 census (`792` lines, sha `441415fc…`) does not match the current
   golden (`6522` lines, sha `b281a9b2…`, the v22 ET spine with the terminal at
   step 6502). The load-bearing property (`no state=3`) still holds, but the
   FU-212 table is **stale**; any doc promotion must cite the current golden.
   Owner: this slice / the doc promoter. See §7-E1.

### §7-E1. Erratum — stale FU-212 golden anchor

> FU-212 §6.2 quotes the M2 golden as `441415fc…` (792 lines) with a state census
> `{0:304, 2:40, 4:303, 19:20, 20:20, 21:100}`. The **current** committed golden
> is sha `b281a9b2…`, 6522 lines, census `{0:304, 1:953, 2:2193, 4:303, 12:4,
> 19:2640, 20:20, 21:100}` (fresh `sha256sum` + `grep -o` this session). The
> golden was re-pinned by later waves (the phase-21/22 extra-time spine). The
> de-compression verdict is unaffected because the decisive absence (**no
> `state=3`**) is preserved; but FU-212 §6.2's counts and sha must not be quoted
> as current.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled;
`.image` overlay present; `get_current_program_info` confirmed this session):

* `disassemble_bytes` — `0x8A798` (328 B, 91 insns), `0x8A938` (430 B, 113
  insns), `0x8AF00` (40 B).
* `disassemble_function` — `0x4C374` (5 insns), `0x4BEC8` (2 insns).
* `read_memory` — `0x110794` (140 B), `0x1107EC` (52 B).
* `get_xrefs_to` — `0x8A798` (1, DATA, from `0x11080C`), `0x8AF1A` (1,
  UNCONDITIONAL_JUMP, from `0x8AAA3`).
* `get_current_program_info`.

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(`:1595..1621` the render reads; `:2102..2104`/`:2236..2238` the act-8 resets;
`:2897..2947` `match_run_situation_side`; `:3055..3060` goal_queue;
`:3112..3200` `set_piece` incl. the `bx!=0` compression `:3145..3157`;
`:3211..3247` period_setup; `:3288..3320` goal_scan incl. the throw-in arm
`:3300`; `:3525`/`:3574` the restart chain; `:5907` ref_apply; `:6158..6164`
phase_1A_end; `:6359..6407` `fifa96_match_action_phase_1E`; `:6535` the restart
arm); `src/fifa96_engine/fifa96_match_handlers.c` (`:5156` action row 0x1E = the
keeper-claim machine — the name collision; `:5245` phase 0x1A NULL; `:5249..5250`
the phase-0x1E NULL row + comment; `:1718` the sp re-queue); `src/fifa96_loader/
fifa96_action_handlers.c` (`:1129..1147` the situation→phase table: sit 2→3, sit
9→act 2, sit 0xA→act 2 stage 1); `include/fifa96_engine/fifa96_match_run.h`
(`:1648..1666` the phase_1E header; `:807..814` the act-8 cells);
`tests/test_engine_match_frame.c` (`:3642..3663` the pin; `:5950` its call);
`tests/test_engine_match_act.c` (`:598..646` the phase_1E unit tests);
`tests/test_engine_m2.c` (`:2447..2450`/`:2849..2852` the ref-machine asserts in
`m2_directives`; `:3321` `run_tape`; `:3649..3718` `run_census_riders`, incl.
`:3666` `action_wired==33 && phase_wired==22` and `:3701` the `0x1E` NULL pin;
`:4041` the `main` census mirror; `:4863..4914` the restart-chain pins); the
golden `tests/golden/engine/m2-frames.txt` (sha `b281a9b2…`, the per-frame
`state=` census) and `m1-frames.txt` (sha `09b726b7…`); README title + `docs/
ENGINE.md` census line. Docs cross-cited: FU-212 (§1/§2/§3/§4/§6/§9),
FU-194 (§4/§6.2/L2/L3), FU-83 (the phase-driver spine). **No writes** beyond this
draft + the run report; no rename/label/comment/function/script/project save; no
git; the ISO never touched.
