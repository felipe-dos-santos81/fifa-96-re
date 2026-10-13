# FU-202 — the situation-machine live arm + the extra-time flag (phase-18 wave R50)

Provenance: M2 phase-18 wave **R50**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; this session `get_current_program_info` = `watcom:LE:32:default`,
`min_address` `0x10000`, `max_address` `.image::00174a2a`, 2706 functions). Every
address/byte below is a fresh `read_memory` / `disassemble_bytes` /
`get_xrefs_to` / `get_function_by_address` of **this** session unless marked
*cross-cited* (then the frozen FU doc is named). The alias traps are applied
(`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`); the addresses
are `/FIFA96.EXE` runtime addresses (LE globals sit at `+0x100000` from the file
offset). No writes: no rename/comment/label/function/script/project save;
`/FIFA96.EXE` untouched; no git commands.

Scope (R50 brief): derive the situation machine's **live arm** — the
`fifa96_match_run_period_setup` / `period_setup_live` un-gating (FU196-L8); the
**extra-time flag** producer (OL-85, `0x8B2AC`/`0x8B2C1`, the `[0x157AC0]`
writer); the situations `0`/`1`/`8` reachability once live; which **Tier-B
act-tail rows** (`0x1D`/`0x1E`/`0x20`) flip live; a **fresh writer census**; and
the **case-by-case tape verdict** — does wiring `period_setup_live` at `begin`
move presented frames? Extends FU-196 (§1/§3/§4/§5/§6 L8), FU-194 (§3/§4/§6),
FU-188 (§5/§6/§10), FU-189 (§3/§10), FU-143 §10 (OL-85) and FU-195 (§4/§6/§7).

---

## 1. The native producer family (fresh re-verification)

### 1.1 the period/situation producer `FUN_0004B02C` (`0x4B02C`)

Fresh `disassemble_bytes 0x4B090` (112 B) re-reads the `mode == 1` arm and the
common tail byte-for-byte (cross-cited FU-196 §3.1/§3.2, FU-194 §3.4):

```
0x4B095 CALL 0x74034                     ; the setup sink (FU196-L2)
0x4B09A CALL 0x7417C                     ; the restart sink
0x4B09F XOR EBX,EBX ; 0x4B0A1 XOR EDX,EDX ; 0x4B0A3 XOR EAX,EAX
0x4B0A5 CALL 0x8A938                     ; SITUATION 0, side 0, BX 0
0x4B0AA MOV EAX,ESI ; 0x4B0AC CALL 0x8C418  ; both-team refresh
0x4B0B1 MOV EAX,ECX ; 0x4B0B3 CALL 0x8C418  ; both-team refresh
0x4B0B8 XOR ECX,ECX ; 0x4B0BA XOR EAX,EAX
0x4B0BC MOV CX, WORD[0x14C1D4]           ; the input/range word side 0
0x4B0C3 MOV AX, WORD[0x14C1D6]           ; the input/range word side 1
0x4B0C9 OR EAX,ECX                       ; OR of the two words
0x4B0CB TEST AH,0x1                      ; bit 8 (0x100) — extra-time request
0x4B0CE JZ 0x4B0E5
0x4B0D0 MOV EAX,8 ; 0x4B0D5 XOR EBX,EBX ; 0x4B0D7 XOR EDX,EDX
0x4B0D9 CALL 0x8A938                     ; SITUATION 8, side 0, BX 0
0x4B0DE MOV byte[0x157AC2],4             ; the period index := 4 (extra time)
0x4B0E5 XOR EBX,EBX
0x4B0E7 MOV word[0x158822],BX ; 0x4B0EE MOV word[0x157A68],BX
0x4B0F5 CALL 0xCB2B6 ; RET
```

`TEST AH,0x1` is exactly bit 8 of the OR (the FU196-L1 alias trap in the other
direction: `TEST AH,1` tests bit 8 of `EAX`, i.e. `0x100`). Fresh caller census:
`get_xrefs_to 0x4B02C` = **2** (`0x49415`, `0x4950A`, both in `FUN_000493a0`);
`get_xrefs_to 0x493A0` = **3** (the front-end `0x180AA` `FUN_00018014`, `0x18359`
`FUN_00018108`, `0x18819` `FUN_00018680`) — unchanged from FU-196 §3.1.

The two `FUN_000493a0` call sites, fresh (`disassemble_bytes 0x493A0` 256 B +
`0x494F0` 48 B): `0x49410 MOV EAX,1 / 0x49415 CALL 0x4B02C` is **mode 1**;
`0x49508 MOV EAX,EBX / 0x4950A CALL 0x4B02C` is **mode EBX** (the register),
followed by `0x4950F CMP EDX,4 / JL 0x49523` — the one call site where the
period register is compared to 4. So the match-init `FUN_000493a0` runs the
producer twice (mode 1 then mode EBX); the FU196-L2 mode choice stands.

The period-end producer `FUN_00088860` (`0x88860`) is unchanged
(*cross-cited* FU-196 §3.2): fresh `get_xrefs_to 0x88860` = **2** (`0x4B08E`
inside `FUN_0004b02c`; `0x74244` the period-end body with no auto-created
function — FU196-L6).

### 1.2 the situation dispatcher `FUN_0008A938` — fresh producer census

Fresh `get_xrefs_to 0x8A938` = **39** (byte-identical site set to FU-196 §3.3 /
FU-143 §10). The situation-**0** and situation-**8** producers of this slice:

* **sit 0** — `0x4B0A5` (`FUN_0004b02c` mode-1 arm), the only sit-0 producer.
* **sit 8** — `0x4B0D9` (`FUN_0004b02c` extra-time gate) and `0x888C8`
  (`FUN_00088860` period-end with `[0x157AC2]==4`).
* **sit 1** — `0x888F2` (`FUN_00088860` period-end, non-extra-time) and the
  act-0xA / act-8 terminals (`0x8B85D` / `0x8A8CE`).
* **the act-8 fallback** — `0x8A8CE` (`FUN_0008A798` stage-2 terminal,
  re-dispatching the stored situation), *cross-cited* FU-194 §4.1/§4.3.

The sit-8 table-2 arm `0x8AE08..0x8AEAE` is fresh (`disassemble_bytes 0x8AE08`
64 B): `MOV EAX,[0x157A49]; SAR 0x18; CMP 1; JZ 0x8AEAE` — if the live phase byte
`[0x157A4D] != 1` it runs the match re-init (`0x8AE19..0x8AE44`: `CL=1`,
`[0x157AAF]=side`, `[0x157A4C]=1`, the two `CALL 0x897F4(…,0xA)` at `0x8AE30`/
`0x8AE3F`); the shared tail `0x8AEAE` then does `MOV EBX,1; MOV EAX,7; …; CALL
0x888FC` (act 7). **So situation 8 → act 7 (phase 0x1D), invoke-now.**

---

## 2. The engine seam (first-hand engine source)

`fifa96_match_run_period_setup` — `src/fifa96_engine/fifa96_match_run.c:2989`
(*cross-cited* landed by phase-17 T2/FU-196 §4 rec 1). It mirrors `FUN_0004B02C`:

* `mode == 1` (`:2998`): `pass_parity = 0` then
  `fifa96_match_run_set_piece(mr, 0u, 0u, 0u)` (the `0x4B0A5` mirror).
* `mode == 0` (`:2999..3015`): gated on `state.phase == 0x0C`; the
  `global_157ac2 == 4` arm dispatches `set_piece(mr, 8, rng&1, 0)` (the `0x888C8`
  mirror), else `set_piece(mr, 1, side_controlled, 0)` (the `0x888F2` mirror).
* the shared extra-time gate (`:3019`):
  `if ((mr->input_range[0] | mr->input_range[1]) & 0x0100u) { set_piece(mr, 8, 0,
  0); mr->global_157ac2 = 4u; }` (the `0x4B0CB`/`0x4B0D9`/`0x4B0DE` mirror).

The live `begin` call is **gated**: `fifa96_match_run.c:2244`
`if (mr->period_setup_live) (void)fifa96_match_run_period_setup(mr, 1);`, with
`period_setup_live` reset 0 in `fifa96_match_run_reset` (`:2068`) and `begin`
(`:2193`). The header documents the gate (`include/fifa96_engine/fifa96_match_run.h`
`:787..793`, `:1288`). Fixtures (`tests/test_engine_match_handlers.c:3577..3652`)
exercise all five arms directly; the tape-neutral default is pinned
(`:3723 test_producer_gates_default_off`).

**`fifa96_match_run_begin` order (fresh, first-hand):** `period_setup` gated at
`:2244` → `match_run_formation_seed` (`:2248`) → `match_run_slot_bind` (`:2252`)
→ `fifa96_match_run_kickoff_entry` (`:2269`, writes phase 1) → the placement
commit → `fifa96_match_lifecycle_begin` (`:2303`) → the **begin-tail
screen-install block** (`:2335..2347`), which ends:

```c
fifa96_match_run_screen_install(mr, 0, 0, 0);   /* :2340 */
fifa96_match_act_init(&mr->act);                /* :2341  <<< CLEARS the act slot */
mr->state.phase = phase_before;                 /* :2343  (restores 1) */
mr->state.prev_phase = prev_before;             /* :2344  (restores prev) */
mr->phase_machine.state = pm_state_before;      /* :2345 */
mr->phase_machine.phase = pm_phase_before;      /* :2346 */
```

So at the end of every `begin`, the act slot is **re-initialised to zero** and
the phase cells are restored to their pre-block values. `fifa96_match_state_set_phase`
(`src/fifa96_loader/fifa96_match_state.c:33`) sets `prev_phase = phase` before the
write; `match_run_write_phase` (`fifa96_match_run.c:2438`) mirrors the
`phase_machine` and runs the per-team `record_phase_walk` only when
`fifa96_match_phase_table[phase].fn != NULL` (`:2452`) — so a NULL phase body is a
structural no-op.

---

## 3. The M2 tape verdict — does wiring `period_setup_live` at `begin` move frames?

**The tape runs `begin`.** The M2 tape enters MATCH through
`fifa96_match_bridge.c:23 fifa96_match_run_begin(mr, eng, selector)` (selector 0;
`tests/test_engine_m2.c` samples `match_start_step` and asserts
`match_run.lc.selector == 0`). So the `begin`-time producer is on the tape's
match-start path — fresh-confirmed first-hand in the bridge/engine source.

**Case A — `period_setup_live = 1` with the current placement (`:2244`) and the
Tier-B bodies unregistered: TAPE-NEUTRAL (structural).** With `input_range == 0`
(the tape stages the input config zero; `fifa96_match_run_begin` builds
`input_range` at `:2235` from the zero `input_cfg`), the mode-1 call runs
`fifa96_match_run_set_piece(mr, 0, 0, 0)`:

1. situation 0 takes the direct path (`:2914` skips the queue for sit 0) →
   `match_run_situation_side(mr, 0, 0, 1)` (`:2977`);
2. `fifa96_action_phase_situation(0, &out)` → `out = {phase 0x11, act 0x0A,
   stage 0}` (the loader table `fifa96_action_phase_situations[0]`,
   `src/fifa96_loader/fifa96_action_handlers.c:1129`, fresh-read this session);
3. `match_run_write_phase(mr, 0x11)` — phase 0x11's body is NULL
   (`fifa96_match_phase_table[0x11]`, `fifa96_match_handlers.c:5090`), so **no
   record walk**; `fifa96_match_run_phase_arm(mr, 0x11)` returns 0 immediately
   (`:2567`, 0x11 not in the arm set);
4. `fifa96_match_act_dispatch(mr, 0x0A, 0, 0)` arms `mr->act = {id 0xA, stage 0,
   timer 0}` with **invoke 0** (`:2741`, `out.act == 2u ? 1 : 0`) — the body is
   **not** run.

`begin` then continues through `kickoff_entry` (`:2269`), which writes **phase 1**
(overwriting 0x11), and finally runs the begin-tail block, whose
`fifa96_match_act_init(&mr->act)` (`:2341`) **zeroes the armed act 0xA before the
first granted frame**. Net end-of-begin state vs. the current tape:

| cell | today (`period_setup_live 0`) | wired (`=1`, as placed) |
|---|---|---|
| `state.phase` | 1 | 1 (0x11 transient, overwritten by kickoff 1) |
| `phase_machine.state/phase` | 1 | 1 (restored by `:2345/:2346`) |
| `mr->act` | 0 | **0** (cleared by `:2341`) |
| `state.prev_phase` | 0 | **0x11** |
| `pass_parity` | 0 | 0 |

The only surviving delta is `state.prev_phase`. **`prev_phase` has no reader** —
a fresh `grep` over `src/` + `include/` finds the cell only at its
declaration/init/`set_phase` and the `begin` save/restore (`fifa96_match_run.c`
`:2337/:2344`, `fifa96_match_state.c:24/:35`, the header). It is a **write-only
mirror**; the tape records `state.phase` (not prev_phase) and the render never
reads it. Therefore the as-placed wiring is **structurally tape-neutral**, and the
same holds **with the Tier-B phase-0x20 body registered**: the act slot is cleared
before any tick, so the `fifa96_match_act_tick` phase-0x20 dispatch
(`fifa96_match_act.c:63`, which no-ops on a NULL body) never fires. A `cmp`
remains **mandatory** (R2/R4) — this is a structural proof, not a run.

**Case B — the producer relocated (or the act preserved) + phase-0x20 registered:
TAPE-MOVING.** If T3 moves the `period_setup` call to the end of `begin` (after
`:2347`) — or otherwise preserves the armed act — the act-0xA survives into the
first `fifa96_match_run_step`, whose clock tail `fifa96_match_act_tick`
(`:3956`) dispatches phase 0x20. `fifa96_match_action_phase_20`
(`fifa96_match_run.c:5617`, fresh-read) **stage 0 installs code `0x24` on all 22
records** (`:5621..5624`, both teams × 11 — the FU-188 §3 lane/anim machine), and
the stage-2 terminal re-dispatches situation 1 (`:5639`). That is a render-visible
record mutation to every player. **So wiring the producer *effectively* moves the
tape.** The move is native-faithful *in isolation* (the real match-init
`FUN_000493a0 → FUN_0004B02C(1) → FUN_0008A938(0,0,0)` does install code 0x24),
but the engine's `begin` **also** runs the ad-hoc `fifa96_match_run_kickoff_entry`
(`:2269`) which the native replaces with the act-0xA terminal's sit-1 dispatch —
a **double-run**, so the combined engine sequence is *not* bit-for-bit native.
T3 must resolve the double-run (route the kickoff through the terminal, or keep
the producer gated) before a re-pin can be justified.

**Verdict (one line):** *wiring `period_setup_live` at `begin` at its current
placement is tape-neutral (the armed act 0xA is cleared by the begin-tail
`act_init` before any tick, the phase transient 0x11→1 nets to phase 1, and the
only residual — the write-only `state.prev_phase` — has no reader); wiring it
where the act survives moves the tape (the phase-0x20 code-0x24 install), and
that move double-runs the ad-hoc kickoff, so it is not purely native-faithful.*

---

## 4. The extra-time flag `[0x157AC0]` (OL-85) — fresh writer census

Fresh `get_xrefs_to 0x157AC0` = **19**: **7 writes** + 12 reads.

| writer | function | fresh bytes / effect |
|---|---|---|
| `0x73F58` | `FUN_00073ee0` | the match-reset store (writes 0) |
| `0x8B2AC` | `FUN_0008af38` | `MOV byte[0x157AC0],1` — OL-85 scores-differ arm |
| `0x8B2C1` | `FUN_0008af38` | `MOV byte[0x157AC0],1` — OL-85 scores-equal + `[0x14C2EE]!=0` |
| `0x8B3A2` | `FUN_0008af38` | `MOV byte[0x157AC0],1` — a **third** clock-harness arm (new) |
| `0x8B5CD` | `FUN_0008af38` | `MOV byte[0x157AC0],DL(1)` — the period-1 rollover writer |
| `0x389A4` | `FUN_00038630` | **front-end**: `MOV byte[0x157AC0],1` **and** `MOV dword[0x14C2EE],1` (new) |
| `0x89E8C` | (the act-7 phase-0x1D arm-9 terminal) | `MOV byte[0x157AC0],DH(1)` (new in the OL-85 census) |

Fresh re-read of the OL-85 window `0x8B220..0x8B2E7` (`disassemble_bytes`, 200 B):

```
0x8B22F MOV AX,[0x157AB6] ; 0x8B235 MOV DX,WORD[0x15881A] ; CMP AX,DX
0x8B23F JNZ 0x8B274
0x8B241 CMP WORD[0x157ABA],0 ; 0x8B249 JZ 0x8B295
0x8B24B MOV byte[0x15882D],1 ; 0x8B252 CALL 0x8EF38 ; …
0x8B274 XOR EAX,EAX ; … ; 0x8B28D CMP EDX,EAX ; 0x8B28F JNZ 0x8AFB7
0x8B295 MOV AX,[0x157AC5] ; 0x8B29B MOV DX,WORD[0x157AC7] ; 0x8B2A7 CMP AX,DX
0x8B2AA JZ 0x8B2B8
0x8B2AC MOV byte[0x157AC0],1 ; JMP 0x8AFB7        ; scores differ -> extra time
0x8B2B8 CMP dword[0x14C2EE],0 ; 0x8B2BF JZ 0x8B2CD
0x8B2C1 MOV byte[0x157AC0],1 ; JMP 0x8AFB7        ; scores equal + force -> extra time
0x8B2CD CMP byte[0x14C1D0],0 ; JNZ 0x8AFB7
0x8B2DA MOV dword[0x10F378],1 ; JMP 0x8AFB7
```

So the OL-85 producer is fresh-confirmed: `[0x157AC5]`/`[0x157AC7]` (the score
pair) compared at `0x8B2A7`; unequal → `0x8B2AC`, equal → `[0x14C2EE] != 0` →
`0x8B2C1`. **Erratum (FU-195 §4):** FU-195 §4 cited the period-1 rollover writer
as `0x8B5D3 MOV [0x157AC0],DL(1)`; the fresh `MOV` is at **`0x8B5CD`**
(`bytes 8815c07a1500`); `0x8B5D3` is the following `CMP AX,CX` (`0x8B5A1 CMP
AX,[0x157AC7]; 0x8B5AE JZ 0x8B601; 0x8B5B0 CMP [0x157AC0],0; …; 0x8B5CD MOV
[0x157AC0],DL`). The FU-195 §4 semantics (period-1 rollover, scores differ) stand;
only the address drifts by 6 bytes.

**New writer relations (fresh).**

* **`0x389A4` (`FUN_00038630`)** sets **both** `[0x157AC0]=1` and `[0x14C2EE]=1`
  (`0x38990 MOV byte[0x157AC2],DH; 0x38998 MOV EAX,0xFFFFFFFE; 0x3899D MOV DL,1;
  0x389A4 MOV byte[0x157AC0],DL; 0x389AA MOV dword[0x14C2EE],ECX(1)`). So the
  front-end match-screen install can request extra time; `[0x14C2EE]` is exactly
  the OL-85 equal-score override cell (`0x8B2B8`), closing the loop on FU-143
  OL-85's "`[0x14C2EE]`" reader.
* **`0x89E8C`** is the act-7 (phase-0x1D) arm-9 terminal (`disassemble_bytes
  0x89E70` 48 B: `0x89E78 MOV DH,1; 0x89E84 MOV byte[0x158828],BL(0); 0x89E8C
  MOV byte[0x157AC0],DH(1); 0x89E92 MOV dword[0x158808],EBX(0)`), the
  `[0x157AC0]=1` FU-194 §3.3 arm-9 reported. **The phase-0x1D terminal itself
  raises the extra-time flag** — a coupling between the Tier-B row and OL-85.
* **`0x8B3A2` (`FUN_0008af38`)** is a third clock-harness writer (`0x8B394 MOV
  ECX,1; 0x8B399 CMP AX,DX; 0x8B39C JZ 0x8AFB7; 0x8B3A2 MOV byte[0x157AC0],1`).

**Engine status.** The engine carries **no `[0x157AC0]` cell**: `fifa96_match_run_phase_drive`
(`fifa96_match_run.c:2498`) passes `extra_time = 0u` hardcoded to
`fifa96_action_phase_period_end` (`:2517`), so `out.act` is always `0x0B` and
phase 0x22/act 0xC is producer-dormant (FU-195 §7, re-confirmed). The
`fifa96_match_run.h:1555` comment and the `match_run.c:5838` comment both record
"`[0x157AC0]=1` has no engine cell". The tape never reaches a period-1 completion
(short fixture; the run exits at the goal), so all four clock-harness writers are
**tape-dormant**.

**OL-85 verdict:** the writer is **derived (5 live native sites + reset + the
front-end request) but unported in the engine** → **numbered leg FU202-L1**. Its
trigger needs a period-1 completion with a differing score (or the front-end
`[0x14C2EE]` request) — never reached by the M2 tape and unmodeled by
`phase_drive`. Do not fake a producer.

---

## 5. Reachability once live + which Tier-B rows flip

### 5.1 the three situations

| sit | native producer (fresh) | engine producer today | engine route when live |
|---|---|---|---|
| **0** | `0x4B0A5` (`FUN_0004b02c` mode 1) | `period_setup(mode 1)` at `begin :2244` — **gated off** | `set_piece(0,0,0)` → phase 0x11 + act 0xA |
| **1** | `0x888F2` (`FUN_00088860` non-extra-time) + the act-0xA/act-8 terminals | `period_setup(mode 0)` gated off; `fifa96_match_run_situation(mr,1)` live (act-0xA terminal `:5639`) | `set_piece(1, side, 0)` → act 1 |
| **8** | `0x4B0D9` (extra-time gate) + `0x888C8` (`global_157ac2==4`) | `period_setup` both arms — **gated off**; needs `input_range & 0x100` (tape 0) or `global_157ac2==4` | `set_piece(8, side, 0)` → act 7 |

### 5.2 the act/phase table (fresh)

Fresh `read_memory 0x1107EC` (72 B) and `0x110794` (140 B) re-confirm the FU-194
§1 / FU-198 §1 tables byte-for-byte: act 7 → phase 0x1D → `0x89868`; act 8 →
phase 0x1E → `0x8A798`; act 0xA → phase 0x20 → `0x8B688`; phase 0x11 → `0x6E1C8`.
Fresh `get_xrefs_to 0x89868` = **1** (DATA from `0x110808` = phase slot 0x1D) —
the coupled row-0x20 installer's handler reaches only through the phase seam.

Engine slots (fresh `fifa96_match_handlers.c`): **phase 0x1D = NULL** (`:5114`,
body `fifa96_match_action_phase_1D` ported, `:5802`), **phase 0x1E = NULL**
(`:5116`, body `:5865`), **phase 0x20 = NULL** (`:5120`, body `:5617` — 22-record
code-0x24 install); **action row 0x20 = NULL** (`:5027`), **row 0x24 = NULL**
(`:5035`). Phase 0x11 = NULL (`:5090`).

### 5.3 the Tier-B flips (honest)

| Tier-B row | native producer | flips live iff | tape? |
|---|---|---|---|
| **phase 0x20** + **action 0x24** | sit 0 (`0x4B0A5`) → act 0xA | the producer fires **and the act-0xA survives to a tick** (relocated call, or the begin-tail `act_init` removed/preserved); `set_piece(0,0,0)` alone arms but never invokes the act | **moves** (22-record code-0x24 install, §3B) |
| **phase 0x1D** + **action 0x20** | sit 8 (`0x4B0D9`/`0x888C8`) → act 7 | the extra-time path is produced (`global_157ac2==4` at a phase-0xC period-end, or `input_range & 0x100`) **and** the slot is registered | **tape-dormant** (input_range 0; no period-end on the tape) |
| **phase 0x1E** | the `FUN_0008A938` BX!=0 fallback (`0x8AA98`) | — (T3 rec: leave NULL) | n/a — the engine *compresses* the act-8 fallback (`set_piece` BX!=0, `:2941..2953`); registering the 22-record sweep de-compresses a tape-visible derivation (FU194-L2) |

Phase 0x1D's terminal raises `[0x157AC0]` (§4, `0x89E8C`): the phase-0x1D row and
the extra-time flag are **coupled** — landing one affects the other's producer.

---

## 6. The T3 wiring rec

1. **`period_setup_live` at `begin` — the honest landing is gated or re-placed,
   not blanket-live.** Flipping the gate live at the current `:2244` placement is
   proven tape-neutral (§3A) but also **ineffective**: the begin-tail
   `fifa96_match_act_init` (`:2341`) clears the armed act before the first tick,
   so situation 0 does not actually drive act 0xA — the row would be wired-but-
   dead. Two honest options:
   * **(a) keep the gate gated on the live tape** (the R9/FU196-L8 posture): land
     the producer + fixtures, keep `period_setup_live` default 0, pin the
     dormancy. No census move.
   * **(b) re-place the producer** at the end of `begin` (native order: the
     act-0xA terminal *is* the kickoff) **and retire the ad-hoc `kickoff_entry`
     double-run**, then register phase 0x20 + row 0x24 — a coupled slice. This
     moves the tape (the code-0x24 install) and is the only route that makes
     situation 0 genuinely producer-live; the controller authorises the re-pin
     only if the reworked chain is proven native-faithful.
2. **Phase 0x20 + action row 0x24 — coupled, count only with (b).** The installer
   IS the phase-0x20 stage-0 loop (`:5621`); registering the body without a
   surviving act is the rows-27/29/2C "ported body, no live installer" class.
3. **Phase 0x1D + action row 0x20 — coupled to situation 8 + the extra-time
   path.** Land the bodies; keep the slots NULL until the extra-time producer
   exists (FU202-L1). The coupled installer `0x899A2` fires only when act 7 is
   driven (FU-194 §6.1, FU-189 §3).
4. **Phase 0x1E — leave NULL** (the compressed fallback; FU194-L2). No census
   move.
5. **The extra-time flag — leg FU202-L1** (§4). Do not fake `[0x157AC0]`.
6. **The `?` (the escape hatch):** `fifa96_match_run_begin`'s documented
   assumption (`:2242` FU196-L2) that the producer sits at `:2244` is what makes
   case (a) work; T3 must not "fix" it to `=1` without the re-placement, or the
   feature silently no-ops.

**Conditional census (no hand-set number).** Baseline `48/80 = 60%`. If T3 lands
option (b) and proves situation 0 live → **phase 0x20 + action 0x24 = +2 →
`50/80 = 62%`**. If a later slice also produces `[0x157AC0]` and proves situation
8 live → **phase 0x1D + action 0x20 = +2 → `52/80 = 65%`**. The implementer
recomputes `floor(100 × wired/80)` from the landed set (AGENTS.md rule); no number
is asserted here.

---

## 7. Numbered legs

1. **FU202-L1 — the extra-time flag `[0x157AC0]` producer (OL-85).** The writers
   are fresh-derived (`0x8B2AC`/`0x8B2C1`/`0x8B3A2`/`0x8B5CD` in `FUN_0008af38`,
   the front-end `0x389A4` `FUN_00038630` with `[0x14C2EE]`, the reset `0x73F58`,
   and the act-7 terminal `0x89E8C`); the engine has no cell
   (`phase_drive` passes `extra_time = 0u`, `match_run.c:2517`). Owner: the
   period-1-completion wiring. Not tape-reachable (no period-1 completion in the
   M2 fixture). Do not fake it.
2. **FU202-L2 — the `period_setup_live` placement / kickoff double-run.**
   `begin` runs the producer (`:2244`) **and** the ad-hoc `kickoff_entry`
   (`:2269`); the native routes the kickoff *through* the act-0xA terminal's sit-1
   dispatch. So the producer is (a) defeated by the begin-tail `act_init`
   (`:2341`) at its current placement and (b) a double-run if re-placed without
   retiring `kickoff_entry`. Settle by decomposing `FUN_0004B02C`'s caller
   `FUN_000493a0` order vs. the engine `begin` order (extends FU196-L2). Owner:
   T3.
3. **FU202-L3 — the act-0xA/phase-0x20 invoke-now cadence on the tape.** The
   native situation-0 arm calls `FUN_000888FC(0xA, 0, BX=1)` (invoke-now); the
   engine's `match_run_situation_side` passes `invoke = (out.act == 2)`
   (FU196-L3), so act 0xA installs with `invoke=0`. Whether the same-frame
   invoke is observable in the phase-0x20 body is untested; carried with FU196-L3.
4. **FU202-L4 — the `[0x157AC2]` period-index writers.** Fresh `get_xrefs_to
   0x157AC2` = **56**; the writers include `0x73F82` (reset), `0x4B0DE`
   (`FUN_0004b02c`, :=4 on the extra-time gate), `0x4BA20`/`0x4BA31`
   (`FUN_0004ba00`), `0x8B58A`/`0x8B61B` (`FUN_0008af38` rollover INCs), `0x38990`
   (`FUN_00038630` front-end). The engine mirrors only the `global_157ac2`
   increments in `phase_drive`; the front-end/setup writers are unported. The
   full writer graph is not decomposed here.
5. **FU202-L5 — `FUN_00088860`'s second caller (`0x74244`).** Unchanged from
   FU196-L6: the containing period-end body has no auto-created function and the
   `0x7417C`/`0x6FFC0`/`0x700F4`/`0x73E28` sinks stay cited only.
6. **FU202-L6 — the `0x8B…` clock-harness arm map (`FUN_0008af38`).** The fresh
   `[0x157AC0]` census exposes **four** `FUN_0008af38` writers (`0x8B2AC`,
   `0x8B2C1`, `0x8B3A2`, `0x8B5CD`) plus the `0x8B60D`/`0x8B616` readers; the arm
   conditions around `0x8B3A2` are not decomposed here (extends FU-195 §4).
7. **FU202-L7 — the front-end extra-time request (`0x389A4`).** `FUN_00038630`
   sets `[0x157AC0]=1` **and** `[0x14C2EE]=1`; the `FUN_00038630` context (a
   match-screen install family per FU-146) and its menu caller are not traced.
   This is a live pre-match producer of the flag the engine does not model.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` (program `/FIFA96.EXE`, 2706 functions).
* `read_memory`: `0x1107EC` (72 B, act table), `0x110794` (140 B, phase table).
* `disassemble_bytes` (start, length in B): `0x4B090` (112, the producer body),
  `0x493A0` (256, the match-init + mode-1 call), `0x494F0` (48, the mode-EBX
  call), `0x8B220` (200, the OL-85 window), `0x8B5A0` (120, the rollover writer),
  `0x8B390` (64, the third writer), `0x89E70` (48, the act-7 terminal),
  `0x38990` (32, the front-end writer), `0x8AE08` (64, the sit-8 table arm).
* `get_xrefs_to`: `0x157AC0` (19), `0x8A938` (39), `0x4B02C` (2), `0x493A0` (3),
  `0x88860` (2), `0x89868` (1), `0x157AC2` (56).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(`:2068..2348` the reset/begin incl. the `period_setup_live` call `:2244` and the
begin-tail `act_init` `:2341`; `:2438..2472` the phase write / kickoff entry;
`:2498..2536` `phase_drive`; `:2565..2640` `phase_arm`; `:2699..2978`
`match_run_situation_side` / `set_piece`; `:2989..3025` `period_setup`;
`:3930..3963` the frame tail / act tick; `:5617..5640` `action_phase_20`;
`:5802..` `action_phase_1D`/`1E`), `src/fifa96_engine/fifa96_match_act.c` (whole),
`src/fifa96_engine/fifa96_match_handlers.c` (`:4964..5053` action table,
`:5055..5126` phase table), `src/fifa96_loader/fifa96_action_handlers.c`
(`:1129..1160` the situation table), `src/fifa96_loader/fifa96_match_state.c`
(`:1..70`), `src/fifa96_engine/fifa96_match_bridge.c` (`:23`),
`include/fifa96_engine/fifa96_match_run.h` (`:247..268`, `:787..793`,
`:1149..1155`, `:1555`), `tests/test_engine_m2.c` (the tape runner `:2445..2530`),
`tests/test_engine_match_handlers.c` (`:3572..3730`). Docs cross-cited: FU-143
§10 (OL-85), FU-188 §5/§6/§10, FU-189 §3/§10, FU-194 §3/§4/§6, FU-195 §4/§6/§7,
FU-196 (§1/§3/§4/§5/§6 L8), FU-198 §1. No writes beyond this slice file and the
workspace report; no rename/label/comment/function/script/project save; no git.

---

## Addendum — T3 landing status (2026-10-12, additive)

This addendum records the **T3** landing against §6's rec (b); it does not
rewrite any section above.

- **The rec-(b) re-placement landed.** The match-init producer call is
  **re-placed** to run after the begin-tail `fifa96_match_act_init` (the §3A
  dead-gate fix), so the situation-0 act `0xA` survives into the first granted
  frame's clock-tail act tick. Phase `0x20` (`fifa96_action_phase_20`) and
  action row `0x24` (`fifa96_match_action_24`) are **registered/wired and
  counted**; the row-`0x24` stage-0 `pos_x = 0x9F0` write is render-read, so
  **M2 re-pins** (`7b4b241a…` → `441415fc…`, frames 12..792 hash-only, every
  `state=` suffix unchanged). **Census `+2` → `50/80 = 62%`**.
- **The ad-hoc kickoff_entry is RETAINED, not retired.** §6's "(b) … and retire
  the ad-hoc `kickoff_entry`" is **not fully realisable in T3**: the native
  kickoff phase-1 entry is the act-1 route (situation 1 → act 1 → phase `0x17`),
  which fires only at the phase-`0x20` terminal (~`0x3E8` frames) and is
  **unported**. Retiring the derived kickoff without it leaves the match stuck in
  the cutscene (no natural phase 2). T3 therefore keeps the derived
  `fifa96_match_run_kickoff_entry` (the derived act-1 stage-0 stand-in) and
  leaves phase `0x17` `NULL/uncounted` → **leg FU203-L2/FU202-L2**. The move is
  **not** a double-run of the ad-hoc arm: disabling the re-placed producer stops
  the row-`0x24` dispatch (isolated `mask_final` proof).
- **OL-85 (`[0x157AC0]`) stays leg FU202-L1** — no live engine cell; not faked.
  The coupled phase-`0x1D`/row-`0x20` pair stays `NULL` (FU205-L2).
