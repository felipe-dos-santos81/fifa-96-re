# FU-196 — the situation producer machine (situations `0`/`8`/`0xB`)

Provenance: M2 phase-17 wave **R44**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; this session `get_current_program_info` = `watcom:LE:32:default`,
`min_address` `0x10000`). Every address/byte below is a fresh `read_memory` /
`disassemble_bytes` / `get_xrefs_to` / `decompile_function` /
`get_function_by_address` of **this** session unless marked *cross-cited* (then
the frozen FU doc is named). The alias traps are applied (`dword[x]>>16` = the
word at `x+2`, `>>24` = the byte at `x+3`); the addresses quoted are
`/FIFA96.EXE` runtime addresses (LE globals sit at `+0x100000` from the file
offset). No writes: no rename/comment/label/function/script/project save;
`/FIFA96.EXE` untouched; no git commands.

Scope: the **situation machine** — (1) the situation-selector cell(s) and their
writers; (2) the situation→act map (the table-2 jump table `0x8A904`); (3) the
native producers of situations **`0`**, **`8`** and **`0xB`**; (4) the engine
parity and the **T2 producer-machine** rec; (5) the per-situation reachability
verdicts and the act-tail/per-record rows that flip producer-live once each
situation is drivable; (6) the remainder as numbered legs. Extends FU-149 §1.1,
FU-175 §1/§4, FU-188 §5 and FU-189 §10.

---

## 1. The situation selector cell(s) — there is no single one

**There is no global cell that holds "the current situation".** The situation is
the **`AX` argument** of the dispatcher `FUN_0008A938` (fresh
`disassemble_bytes 0x8A938`, `0x8A93E MOV ECX,EAX` / `0x8A991 XOR EAX,EAX` /
`0x8A993 MOV AX,CX`); every producer passes an immediate. What persists is the
**queue latch** and the **act-8 replay latch**. Fresh `get_xrefs_to` censuses:

| cell | name (engine) | writers (fresh xrefs) | readers | role |
|---|---|---|---|---|
| `[0x15B6A8]` dword | `situation_id` | dispatcher queue arms `0x8A9A5/0x8A9B6/0x8A9CD/0x8A9ED/0x8AA08/0x8AA2F/0x8AA45` + `0x8AA60` (8 arms); `FUN_000948ac` ×5; `FUN_00092d8c` `0x92DCD`-family | the FU-146 screen legs `0x93D51/0x93D79/0x93FD5/0x94008/0x9419F/0x941C7/0x94442/0x9446A/0x94620/0x94648/0x9480D/0x94812` | **the queue/latched situation id** (the screen selector) |
| `[0x15B6C0]` dword | `situation_pending` | dispatcher `0x8A964`(read)/`0x8AA6A`/`0x8A9BC/0x8A9D7/0x8A9F7/0x8AA12/0x8AA34/0x8AA4F`; `FUN_000948ac` ×5; `FUN_00092d8c` `0x92DCD` | dispatcher head `0x8A964`; screen legs | the pending latch |
| `[0x15B6B8]` dword | `sit_side_pending` | dispatcher `0x8A982`; `FUN_00092e2c` `0x92E3F` | — (only the latch) | the queue side latch |
| `[0x15882B]` byte | `store_15882b` | dispatcher direct fallback `0x8AA9D`; act-8 tail reset `0x8A8D3` (`=0xFF`) | act-8 tail `0x8A8C3` (`[0x158828]>>24`) | **the act-8 replay situation** |
| `[0x15882C]` byte | `store_15882c` | dispatcher `0x8AA93`; act-0xA terminal `0x8B84A` | act-8 tail `0x8A8BD` (`[0x158829]>>24`) | the act-8 replay side |
| `[0x157AAF]` byte | (side store) | dispatcher `0x8AB66`/`0x8AE23`; `FUN_00088860` `0x8887E`; `FUN_000740A0` `0x740BA`; `FUN_00092e2c` `0x92E89` | dispatcher consumers | the current side |

Fresh `get_xrefs_to` counts: `0x15B6A8` = **25** (8 dispatcher writes, 5
`FUN_000948ac` writes, 1 `FUN_00092d8c` write, 12 screen reads), `0x15B6C0` =
**32**, `0x15B6B8` = **2**, `0x15882B` = **5**, `0x157AAF` = **11**.

**Derived naming decision (for the port).** The brief's "situation selector" is
`[0x15B6A8]` (the latched situation id, engine `situation_id`): it is written
**only** from the dispatcher's table-1 queue arms (`0x8A8E0`, §2) plus the
FU-146 screen producers, and it is **not** the value that drives the act — the
act is driven by the `AX` argument. The second candidate, `[0x15882B]`, is the
**act-8 replay** latch (the BX!=0 fallback stores the situation there and act 8's
tail re-dispatches it, §3.4). The engine already carries both
(`mr->situation_id` / `mr->store_15882b`).

---

## 2. The situation→act map

### 2.1 The dispatcher head (fresh `0x8A938..0x8AA7A`, 160 B + 96 B)

```
0x8A944 TEST AX,AX / JZ 0x8AA7B            ; situation 0 -> direct path
0x8A94E CMP EAX,0xB / JZ 0x8AA7B          ; situation 0xB -> direct path
0x8A957 CMP byte [0x14C32A],0 / JZ 0x8AA7B; session gate closed -> direct
0x8A964 CMP dword [0x15B6C0],0 / JNZ 0x8AA7B; pending -> direct
0x8A97F SUB ECX,2
0x8A982 MOV [0x15B6B8],EAX                ; = (side==0)
0x8A987 CMP CX,8 / JA 0x8AA60             ; situation >10 -> id 0xA
0x8A996 JMP dword CS:[EAX*4+0x8A8E0]      ; QUEUE table
```

Fresh `read_memory 0x8A8E0` (64 B) queue table (index = situation−2):

| sit | arm | outcome |
|---|---|---|
| 2/3/4 | `0x8A99E` | `[0x15B6A8] = (side==0) ? 9 : 0` |
| 5/7 | `0x8A9CD` | `= 7` |
| 6 | `0x8A9E8` | `= 5`/`= 6` |
| 8 | `0x8AA60` | `= 0xA` |
| 9/10 | `0x8AA23` | `= (side==1) ? 1 : 2` |

Every queue arm latches `[0x15B6C0] = 1` and `RET`s.

### 2.2 The table-2 jump (fresh `read_memory 0x8A904`, 52 B = 13 dwords)

`sit → target`: `0=0x8AB82, 1=0x8ABAB, 2=0x8ABDB, 3=0x8ABF3, 4=0x8ADC0,
5=0x8ADD8, 6=0x8AC28, 7=0x8ADF0, 8=0x8AE08, 9=0x8AEC6, 0xA=0x8AEDE,
0xB=0x8AEF6, 0xC=0x8AF0E`. Fresh bodies (bytes quoted where re-disassembled):

| sit | fresh body | outcome |
|---|---|---|
| **0** | `0x8AB82` (`0x8AB87 MOV EAX,0xA / 0x8AB8E CALL 0x888FC` then `0x8AB97 MOV EAX,0x11 / 0x8AB9F CALL 0x740A0`) | **act 0xA** (BX=1, stage 0) **then phase 0x11** |
| 1 | `0x8ABAB` (*cross-cited* FU-175 §4) | act 1 (kickoff) |
| 2 | `0x8ABDB` (`0x8ABDF MOV EAX,3 / 0x8ABE7 CALL 0x740A0`) | phase 3 |
| 3 | `0x8ABF3` (`0x8ABFA CALL 0x741B4 / +0x157AD4[BX]++ / 0x8AC17 EAX=4 / CALL 0x740A0`) | corner counter++ then phase 4 |
| 4 | `0x8ADC0` | phase 8 |
| 5 | `0x8ADD8` | phase 9 |
| 6 | `0x8AC28` (*cross-cited* FU-175 §4) | possession/period block → phase 5 |
| 7 | `0x8ADF0` | phase 0xD |
| **8** | `0x8AE08`/`0x8AEAE` (`0x8AE10 CMP EAX,1` (phase grep) / re-init / `0x8AEAE MOV EBX,1 / 0x8AEB3 MOV EAX,7 / 0x8AEBA CALL 0x888FC`) | **act 7** (match re-init first) |
| 9 | `0x8AEC6` (`EAX=2,BX=1,EDX=0`) | act 2 stage 0 |
| 0xA | `0x8AEDE` (`EAX=2,EDX=EBX=1`) | act 2 stage 1 |
| **0xB** | `0x8AEF6` (`0x8AEFA MOV EAX,2 / SAR EDX,0x10 / 0x8AF02 CALL 0x740A0`) | **phase 2** (the sole phase-2 writer) |
| 0xC | `0x8AF0E` (*cross-cited* FU-175 §4) | act 9 |

The **act base is `0x1107EC = 0x110794 + 0x16*4`**, so act `a` selects phase
`0x16+a` and the same handler (fresh `read_memory 0x1107EC`, 56 B): act 7 →
`0x89868` = phase-`0x1D` slot `0x110808`; act 0xA → `0x8B688` = phase-`0x20`
slot `0x110814`. Fresh `get_xrefs_to 0x888FC` = **11** (the act invokers:
`0x8AB57/0x8AF1A/0x8BAE7/0x8A6FE/0x8A789/0x8AB8E/0x8ABCF/0x8AEBA/0x8AED2/
0x8AEEA/0x88EE8`) — unchanged from FU-175 §1.

Fresh `read_memory 0x110794` (140 B): `[0x02]=0x6DCC8`, `[0x11]=0x6E1C8`,
`[0x1D]=0x89868`, `[0x20]=0x8B688` — the per-record phase handlers for sit 0xB
(phase 2), sit 0's phase 0x11, and the act-7/act-0xA tails.

---

## 3. The producers (fresh)

### 3.1 situation `0` — the native match-init (`FUN_0004b02c(1)`)

Fresh `decompile_function 0x4B02C` and `disassemble_bytes 0x493A0` (160 B):

```c
void FUN_0004b02c(int param_1) {                     /* 0x4B02C */
  DAT_00157ab3 = 0; DAT_00157a4f = 0; DAT_00157a5c = 0; DAT_00157a62._2_2_ = 0;
  if (param_1 == 0) {                                 /* 0x4B064 */
    FUN_0007412c();
    if (DAT_0014c2f2 != 0) { FUN_0008cbb0(0x1588a4,..); FUN_0008cbb0(..); }
    if (DAT_0014c32c != 0) { FUN_00078824(); }
    FUN_00088860();                                   /* sit 8 or sit 1 (3.3) */
  } else if (param_1 == 1) {                          /* 0x4B095 */
    FUN_00074034(); FUN_0007417c();
    FUN_0008a938(0,0,0);                              /* 0x4B0A5 -> SITUATION 0 */
  }
  FUN_0008c418(0x1588a4,..); FUN_0008c418(..);        /* both team refresh */
  if (((DAT_0014c1d6 | DAT_0014c1d4) & 0x100) != 0) {
    FUN_0008a938(8,0,0);                              /* 0x4B0D9 -> SITUATION 8 */
    DAT_00157ac2 = 4;
  }
  DAT_00158822 = 0; _DAT_00157a68 = 0; FUN_000cb2b6();
}
```

* `FUN_0004b02c` has exactly **2 callers**, both inside `FUN_000493a0` (fresh
  `get_xrefs_to 0x4B02C` = 2): `0x49415 MOV EAX,1; CALL` and `0x4950a`.
* `FUN_000493a0` (`0x493A0..0x495AF`, the native **match-init**) has **3
  callers**, all in the front-end `0x18xxx` region (fresh `get_xrefs_to
  0x493A0` = 3: `0x180AA` `FUN_00018014`, `0x18359` `FUN_00018108`, `0x18819`
  `FUN_00018680`).
* Fresh disassembly of the `0x4B095` arm: `0x4B09F XOR EBX,EBX; 0x4B0A1 XOR
  EDX,EDX; 0x4B0A3 XOR EAX,EAX; 0x4B0A5 CALL 0x8A938` — **situation 0, side 0,
  BX=0** (direct path → table-2 sit-0 arm → act 0xA + phase 0x11).

So `FUN_0004b02c(1)` is the **period/match-setup** producer; situation 0 is
dispatched **once per match/period start**.

### 3.2 situation `8` — the extra-time gate (`FUN_0004b02c`) + `FUN_00088860`

Two producers (fresh):

1. `0x4B0D0 MOV EAX,8 / 0x4B0D5 XOR EBX,EBX / 0x4B0D7 XOR EDX,EDX / 0x4B0D9
   CALL 0x8A938` — **situation 8, side 0, BX=0**, gated on
   `([0x14C1D6] | [0x14C1D4]) & 0x100` (bit 8 of the OR of the two input/range
   words; `TEST AH,1` at `0x4B0CB`), then `[0x157AC2] = 4`.
2. `FUN_00088860` (`0x88860`, fresh decompile) — **phase-`0xC` gated**
   (`0x88863 EAX=[0x157A4A]; SAR 0x18; CMP EAX,0xC / JNZ`):
   ```c
   if (phase == 0xC) {                      /* period-end / half-time */
     FUN_0007417c(); DAT_00157aaf = DAT_00157aab._3_1_; FUN_0006ffc0();
     FUN_000700f4(); FUN_00073e28();
     if (DAT_00157ac2 == 4) {               /* extra time */
       b = FUN_00092ac8(); FUN_0008a938(8, b & 1, 0);   /* 0x888C8 SIT 8 */
       FUN_0008f188(0xc1,0,8); return;
     }
     FUN_0008a938(1, DAT_00157aab >> 0x18, 0);          /* 0x888F2 SIT 1 */
   }
   ```
   `FUN_00088860` has **2 callers** (fresh `get_xrefs_to 0x88860` = 2):
   `0x4B08E` (inside `FUN_0004b02c`, the param-0 arm) and `0x74244` (a
   period-end family body; the containing function is not auto-created —
   leg FU196-L6). So `[0x157AC2]` is the **period index** (4 = extra time) and
   situation 8 is the **extra-time/period-4** dispatch.

### 3.3 situation `0xB` — the restart-resume rows

The 8 native producers are the row resolution tails (fresh xrefs of `0x8A938`;
site list from FU-149 §1.2, re-verified count 39 this session): `0x7DF90`
(row 01 stage 2), `0x85D38` (row 0x10), `0x863F9` (row 0x11), `0x84495`
(row 0x12), `0x84E8F` (row 0x13), `0x7546E`/`0x75B58`/`0x76072` (keeper rows
0x1D/0x1E). Each is `EAX=0xB, EDX=rec team, EBX=0` → the table-2 sit-`0xB` arm
= `FUN_000740A0(2, side)` → **phase 2**.

### 3.4 the BX!=0 fallback (fresh `0x8AA7B..0x8AA9D`, `0x8A8A5..0x8A8DE`)

```
0x8AAA3 JMP 0x8AF1A
0x8AF1A CALL 0x888FC                 ; act 8 (EAX set to 8 at 0x8AA98)
```
with `0x8AA93 MOV [0x15882c],AL(side)` and `0x8AA9D MOV byte [0x15882b],CL
(situation)`. Act 8's tail (`0x8A8A5..0x8A8DE`) reads `[0x158829]>>24` = byte
`0x15882C` (side) and `[0x158828]>>24` = byte `0x15882B` (situation), calls
`FUN_0008A938(situation, side, BX=0)` at `0x8A8CE`, then writes
`[0x15882B] = 0xFF` (`0x8A8D3`). **The replay selector is `[0x15882B]`.**

---

## 4. Engine parity + T2 producer-machine rec

Engine source read first-hand this session:
`src/fifa96_engine/fifa96_match_run.c` (`fifa96_match_run_set_piece`
2776..2830, `match_run_situation_side` 2614..2661,
`fifa96_match_run_situation` 2663..2665, `fifa96_match_run_begin` 2077..2302,
the screen `match_run_screen_post` 3014..3065),
`src/fifa96_loader/fifa96_action_handlers.c`
(`fifa96_action_phase_situations` 1129..1147).

**What is already right.** `fifa96_action_phase_situations[0x0D]` already
carries the derived table-2 rows (sit 0 = `{phase 0x11, act 0x0A}`, sit 8 =
`{NONE, act 7, OPEN_LEG}`, sit 0xB = `{2, NONE}`); `match_run_situation_side`
dispatches `out.act` through `fifa96_match_act_dispatch`, so act 0xA → phase
0x20 and act 7 → phase 0x1D are already routed through the act seam. The
selector cells exist (`mr->situation_id` `[0x15B6A8]`, `mr->situation_pending`
`[0x15B6C0]`, `mr->sit_side_pending` `[0x15B6B8]`, `mr->store_15882b/c`
`[0x15882B/C]`) and `fifa96_match_run_set_piece` implements the head gates, the
queue ids and the BX!=0 act-8 fallback.

**What is missing — the producers.** A fresh `grep` over `src/` + `tests/` finds
**no `set_piece(…, 0, …)` / `run_situation(…, 0)` and no `…, 8, …` call**. The
engine dispatches situations `{1,2,3,6,9,0xA,0xB,0xC}` only. The native
match-init `FUN_000493a0 → FUN_0004b02c(1)` and the period-end
`FUN_00088860`/`FUN_0004b02c` have **no engine analog**, even though the
engine's `fifa96_match_run_begin` is `FUN_000493a0`'s analog (it seeds the RNG
at the same native call, FU-142d) and `fifa96_match_run_phase_drive` is the
period-end analog.

### T2 producer-machine rec

1. **Port the native setup producer** as a new seam,
   `fifa96_match_run_period_setup(mr, mode)`, mirroring `FUN_0004b02c`:
   * `mode == 1` (`FUN_000493a0` match-init, `0x49415`): the `FUN_00074034`/
     `FUN_0007417c` sinks stay legs, then
     `fifa96_match_run_set_piece(mr, 0, 0, 0)` (the `0x4B0A5` mirror).
   * the extra-time gate (`0x4B0CB TEST AH,1`): after the two team refreshes,
     if `(input_range[side] & 0x0100) != 0` →
     `fifa96_match_run_set_piece(mr, 8, 0, 0)` and `mr->global_157ac2 = 4`
     (leg FU196-L1 for the bit semantics).
   * `mode == 0` (`FUN_00088860`): gate on `mr->state.phase == 0xC`; if
     `mr->global_157ac2 == 4` →
     `fifa96_match_run_set_piece(mr, 8, rng & 1, 0)`; else
     `fifa96_match_run_set_piece(mr, 1, side, 0)`.
2. **Wire the mode-1 producer at `fifa96_match_run_begin`** — the analog of the
   native match-init. **Tape caveat (must not be fudged):** situation 0 fires
   act 0xA (phase 0x20) and phase 0x11; situation 8 fires act 7 (phase 0x1D).
   Those phase bodies are `NULL` until T5, so dispatching situation 0/8 at
   `begin` today changes the live `state.phase`/act slot and can move the M2
   tape. Land the producer API + fixture tests in T2 **with the live `begin`
   path gated** (a flag defaulting to the current tape-neutral sequence); wire
   it live only in the same commit as T5's phase-0x20 / phase-0x1D bodies.
   This is the R2 rule (producer-only; no count before the full route lands)
   and the FU-188 §6 census caveat, now sharpened: **both** sit-0 and sit-8
   are producer-gated on the same T5 bodies, so T2 should land the machine and
   leave the count to T5.
3. **No table change.** `fifa96_action_phase_situations` is correct.
   `match_run_situation_side` needs only the act-0xA invoke fix below.
4. **Situation 0xB is already live.** `fifa96_match_action_01` (row 01,
   `handlers.c:276`) calls `fifa96_match_run_situation(mr, 0x0Bu)` on its
   stage-2 ready arm, and the set-piece taker rows 0x10/0x11/0x12/0x13 call it
   at `handlers.c:1548` etc. Row 01 is installed by the kickoff entry
   (`fifa96_match_run_kickoff_entry`), so the sit-0xB → phase-2 path is
   reachable in a live match today.

---

## 5. Reachability verdicts + rows that flip producer-live

| sit | native producer (fresh) | engine producer today | verdict | act / phase → rows |
|---|---|---|---|---|
| **0** | `FUN_0004b02c(1)` `0x4B0A5` (from `FUN_000493a0` `0x49415`) | **none** | **NOT producer-live** — T2 adds the `FUN_0004b02c`-mirror | act 0xA → phase 0x20 = `0x8B688` (act-tail row, installs **action 0x24** on 22 records); + phase 0x11 (per-record `0x6E1C8`) |
| **8** | `FUN_0004b02c` `0x4B0D9` (extra-time flag) and `FUN_00088860` `0x888C8` (period==4) | **none** | **NOT producer-live** — T2 adds the `FUN_0004b02c`/`FUN_00088860` mirror | act 7 → phase 0x1D = `0x89868` (act-tail row, arm 0 installs **rows 0x19/3** + **0x20** `0x899A2` + **0x1F**) |
| **0xB** | 8 restart-resume rows (`0x7DF90` row 01 … keeper tails) | **live** (row 01 `handlers.c:276` + taker rows) | **producer-live** | phase 2 = the sole phase-2 writer; per-record phase-2 handler `0x6DCC8` (T1/T3) |

**Producer-live flips (coupled to handler landing):**

* Situation 0 → phase `0x20` + action row `0x24` (both ported by phase-16 T4,
  both **uncounted** because sit 0 never fired; FU-188 §6/§10). Landing T2's
  producer makes them producer-live; they can be **counted** only once T5's
  phase-0x20 body is registered (the R2 rule).
* Situation 8 → phase `0x1D` + action row `0x20` (installer `0x899A2` inside
  `0x89868`, FU-189 §3). Landing T2's producer makes the installer
  producer-live; the row counts only with T5's phase-0x1D body.
* Situation 0xB → phase `0x2` (already live). No new row.

**Net census effect of T2 alone: 0** (producer-only; no body, no count).

---

## 6. Numbered legs

1. **FU196-L1 — the extra-time flag `([0x14C1D4]|[0x14C1D6]) & 0x100`.** Bit 8 of
   the OR of the two input/range words (`0x4B0CB TEST AH,1`); the engine builds
   `input_range` at begin (`fifa96_input_range_words`) but the meaning of bit 8
   (extra-time / period-4 request) and its front-end producer are not derived.
   Settle by decomposing the `FUN_0001C9BC` range-word builder and a period-4
   trace.
2. **FU196-L2 — `FUN_000493a0` and the `FUN_0004b02c` mode choice.** The two
   `FUN_0004b02c` call sites (`0x49415` mode 1, `0x4950a` mode `EBX`) and the
   three front-end callers of `FUN_000493a0` are located, but which mode is the
   match-start vs the period transition (and whether the engine's `begin` is the
   exact analog) is not proven. Settle by decomposing `FUN_000493a0` fully.
3. **FU196-L3 — the act-0xA invoke-now cadence.** The native situation-0 arm
   calls `FUN_000888FC(0xA, 0, BX=1)` (invoke-now, `0x8AB8A`), but the engine's
   `match_run_situation_side` passes `invoke = (out.act == 2)` — so act 0xA
   installs with `invoke=0` (deferred to the tick). Whether the same-frame
   invoke is observable in the phase-0x20 body is untested. Carried with the
   table comment at `fifa96_action_handlers.c:1130`.
4. **FU196-L4 — the sit-0 act/phase order.** The native does act 0xA (`0x8AB8E`)
   **then** phase 0x11 (`0x8AB9F`); the engine's `match_run_situation_side`
   writes the phase **before** dispatching the act. Already flagged in the
   table comment (FU-149 §1.1); resolved only when a fixture measures the two
   orders.
5. **FU196-L5 — the situation-8 match re-init.** `0x8AE08..0x8AEAE`: the
   `FUN_000897F4(0,0xA)`/`FUN_000897F4(1,0xA)` calls (`0x8AE30`/`0x8AE3F`), the
   `[0x1587CC + side*4] → [0x1587D4]` record bind, the `[0x1587E6..]`/
   `[0x157AC5/7]` clears and the 6×2 `[0x1587DA]` table clear are unported.
6. **FU196-L6 — `FUN_00088860`'s second caller.** `0x74244` is inside a
   period-end body that has no Ghidra function (fresh `get_function_by_address
   0x74244` = none) and the `0x7417c`/`0x6ffc0`/`0x700f4`/`0x73e28` sinks are
   cited only. Settle by resolving the containing function at `0x74244`.
7. **FU196-L7 — `[0x15B6A8]`'s non-dispatcher writers.** `FUN_000948ac` (5
   writes, `0x948AC..0x949F7`) and `FUN_00092d8c` (`0x92DCD`) also write the
   queue id / pending latch from the FU-146 screen machinery; whether any is a
   situation-machine producer (vs a consumer re-latch) is not derived here (the
   FU-146 boundary). The 12 screen-leg readers are the FU-146 consumer.
8. **FU196-L8 — the situation-0 / situation-8 tape-gate.** The exact engine
   condition under which wiring the mode-1 producer at `begin` stays
   byte-identical (M2 `7b4b241a…`) is not derived; it depends on T5's phase
   bodies. Fixture + `cmp` required (R2/R4).

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` (this session).
* `read_memory`: `0x8A8E0` (64 B), `0x8A904` (52 B), `0x1107EC` (56 B),
  `0x110794` (140 B).
* `disassemble_bytes` (start, length in B): `0x8A938` (160), `0x8AA60` (96),
  `0x4B02C` (200), `0x493A0` (160), `0x88860` (130), `0x888E2` (80),
  `0x8A8A5` (64), `0x8AF1A` (48), `0x8ADB0` (256), `0x8AABF` (180),
  `0x8AB78` (48), `0x8ABDB` (48), `0x8ABF3` (80), `0x8AC28` (60),
  `0x8AEC0` (80), `0x8AEA8` (40), `0x92DC0` (32), `0x494F0` (48),
  `0x74240` (16).
* `decompile_function`: `0x4B02C`, `0x88860`.
* `get_function_by_address`: `0x493A0`, `0x948AC` (`0x74244` = none).
* `get_xrefs_to`: `0x8A938` (39), `0x15B6A8` (25), `0x15B6C0` (32),
  `0x15B6B8` (2), `0x15882B` (5), `0x157AAF` (11), `0x888FC` (11),
  `0x4B02C` (2), `0x493A0` (3), `0x88860` (2), `0x897F4` (3).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(2077..2312, 2580..2695, 2770..2859, 2980..3139),
`src/fifa96_loader/fifa96_action_handlers.c` (1050..1161),
`src/fifa96_engine/fifa96_match_handlers.c` (240..299, 4540..4560). Docs
cross-cited: FU-142d (the RNG seed at `0x493F2`), FU-143 (§10/§11),
FU-146 (§3/§7), FU-149 (§1.1/§1.2/§1.3/§1.4), FU-175 (§1/§4/§10),
FU-188 (§5/§6/§10), FU-189 (§1/§3/§10). No writes beyond this slice file and
the workspace report.
