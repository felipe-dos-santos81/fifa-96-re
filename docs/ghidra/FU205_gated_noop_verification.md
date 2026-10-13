# FU-205 — gated-no-op verification + the coupled row-`0x20` installer (phase-18 wave R53)

Provenance: M2 phase-18 wave **R53**, 2026-10-12, read-only. Authoritative program
**`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program argument;
this session `get_current_program_info` = `FIFA96.EXE`, `watcom:LE:32:default`,
`min_address 0x10000`, `max_address .image::00174a2a`, image base `0`). Every
address/byte/xref/decompile below is a fresh `read_memory` / `disassemble_bytes`
/ `decompile_function` / `get_xrefs_to` / `get_current_program_info` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named). LE-image
globals sit at `+0x100000`; the addresses here are `/FIFA96.EXE` program (runtime)
addresses. The alias traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` =
byte at `x+3`). No writes: no rename, comment, label, function, script or project
save; `/FIFA96.EXE` untouched; no git commands.

Scope (the R53 brief): (1) re-verify first-hand the **7 dispatched-but-gated
per-record rows** gate truth-table (the `phase_meta` NULL path — the exact
disjuncts) as held by `src/fifa96_engine/fifa96_match_handlers.c`; (2) re-verify
the **coupled phase-`0x1D`/row-`0x20` installer `0x899A2` route**; (3) state
whether **R50's** situation-machine live wiring makes the row-`0x20` installer
**fire**; (4) pin the remaining gate cells; (5) the honest T5 wiring rec + the
census delta, or numbered legs. This slice is read-only and unblocks task **T5**.

---

## 1. The 7 gated rows — the gate truth-table (fresh)

The plan's gated set is the seven per-record rows `0x03`/`0x04`/`0x06`/`0x07`/
`0x08`/`0x09`/`0x15` **plus phase `0x02`** (the body-twin of `0x15`). Fresh
`read_memory 0x110794` (92 B, the LE phase table) re-confirms the row → body
assignment byte-for-byte (FU-192 §1 / FU-177 §3):

```
[02]=06DCC8 [03]=06DE44 [04]=06DE44 [06]=06DD9C [07]=06DE44
[08]=06DD6C [09]=06DD6C [15]=06DCC8
```

So the 8 gated table rows collapse onto **four distinct bodies** that read **three
metadata pointers**, each with a `NULL` early-return. Fresh `decompile_function` /
`disassemble_bytes` this session confirms each native read:

| body | rows | native read (fresh) | gate pointer |
|---|---|---|---|
| `0x6DCC8` | `02`,`15`,`08`,`09` | `param_1[3]` = `[rec+0xC]` (`0x6DCC8` decompile: `pcVar1 = (char *)(param_1[3] + iVar5*2)`) | `phase_meta` |
| `0x6DD9C` | `06` | `0x6DDA8 MOV EAX,[EAX+0x10]` (EAX still = rec) | `phase_meta10` |
| `0x6DE44` | `03`,`04`,`07` | `0x6DE7E MOV EAX,[EBP+0x14]` (EBP = rec) | `phase_meta14` |

`0x6DD6C` (rows `08`/`09`) is the thin wrapper: it computes `idx = (side ==
gside) ? 0xBC : 0x6F` and falls into `0x6DCC8`, so it shares the `phase_meta`
gate.

### 1.1 The exact disjuncts (engine, HEAD `2a29245`)

Fresh read of `src/fifa96_engine/fifa96_match_handlers.c`. **Each gated body's
early-return disjuncts** (the walk cursor is staged by
`fifa96_match_dispatch_phase_record`, `:5154-5179`; `phase_walk_active` gates
every body):

* `match_phase_dcc8_apply` (`:4578-4601`; rows `02`/`08`/`09`/`15`):
  1. `tt >= FIFA96_MATCH_ENTITY_TEAMS` → return (`:4587`, team index invalid);
  2. `meta == NULL` → return (`:4594`) — **the OL-13 gate**.
  No side gate; `side` only flips the sign of `tx`/`tz` (`:4597`).
* `fifa96_match_phase_04` (`:4644-4681`; rows `03`/`04`/`07`):
  1. `rec->team >= FIFA96_MATCH_ENTITY_TEAMS` → return (`:4662`);
  2. `(int16_t)sel_1577d8[gside&1] < 0` → return (`:4665`) — the native DX<0
     cell-map arm (`0x6DE73 JL 0x6DEE5`), leg FU192-L3;
  3. `p == NULL` → return (`:4669`) — **the OL-13 gate** (`phase_meta14`).
* `fifa96_match_phase_06` (`:4712-4743`; row `06`):
  1. `rec->team >= FIFA96_MATCH_ENTITY_TEAMS` → return (`:4727`);
  2. `p == NULL` → return (`:4730`) — **the OL-13 gate** (`phase_meta10`).

**Truth-table (as held today):** each gated row is *wired* (its phase-table `fn`
is non-NULL — `handlers.c:5060/5062/5064/5068/5070/5072/5074/5098`) and *counted*
(see §5), but the body's **only** semantic gate is the metadata pointer `NULL`:
the native work is a structural no-op until R47/T1 writes `phase_meta` /
`phase_meta10` / `phase_meta14`. The predicates that are **not** gates (they keep
the body running with a staged value) are `phase_dir_default` (`edge < 0` index),
`sel_1577d6`/`sel_1577d8` (the DX≥0 scale + index), `cell_157754` (the row-06
index) and `phase_machine.side_controlled` — all pinned in §4.

**Erratum (plan windows).** The plan R53 window cites the gate sites as
`:4594/:4603/:4668/:4729/:4748`. Fresh, the actual gate lines are `:4594`
(`meta == NULL`), `:4669` (`p == NULL`), `:4730` (`p == NULL`); `:4603` and
`:4748` are the *function definitions* of `fifa96_match_phase_02` /
`fifa96_match_phase_08` (not gates), and the `phase_04` gate is `:4669`, not
`:4668`. Noted, not rewritten.

---

## 2. The coupled phase-`0x1D` / row-`0x20` installer `0x899A2` (fresh)

### 2.1 The installer bytes + its sole route

Fresh `read_memory 0x8999C` (40 B) — the arm-0 tail of the phase-`0x1D` handler
`0x89868`:

```
0x8999c 89 90 b2 07 00 00   MOV [EAX+0x7B2], EDX      ; [actor-team+0x7B2] = actor rec
0x899a2 ba 20 00 00 00      MOV EDX, 0x20            ; <<< ROW-0x20 CODE
0x899a7 8b 44 24 08         MOV EAX, [ESP+0x8]       ; EAX = [0x1587D4] (actor rec)
0x899ab e8 f4 3f ff ff      CALL 0x7D9A4            ; install row 0x20 (stage=0, invoke=0)
```

Bytes byte-exact to FU-189 §3.3 / FU-194 §3.2 (`BA 20 00 00 00`; `E8 F4 3F FF
FF`). Fresh `get_xrefs_to 0x89868` = **1** (DATA, from `0x110808`); fresh
`read_memory 0x110808` = `68 98 08 00` = **`0x89868`** = `0x110794 + 0x1D*4`
(phase-table slot `[0x1D]`). **The only dispatch route to `0x89868` is the phase
seam** — there is no direct call. Fresh `read_memory 0x110760` = `ec 4e 08 00` =
**`0x84EEC`** (action-table slot `0x20`, `0x1106E0 + 0x80`).

### 2.2 The installer contract (`0x7D9A4`, fresh degcompile)

Fresh `decompile_function 0x7D9A4` (`FUN_0007d9a4(rec, code, stage, invoke)`):
guard `rec != NULL && byte[rec+0x9A]==0 && byte[rec+0x91] != code`; writes
`byte[rec+0x91] = code`, `rec[6]` (`[rec+0x18]`) = `(&PTR_LAB_001106e0)[code]`
(the action table), `[rec+0x89]=0`, `byte[rec+0x9E]=0`, `byte[rec+0x92]=stage`,
`word[rec+0x7B]=word[rec+0x79]`, and **calls `[rec+0x18]` iff `invoke != 0`**. At
`0x899A2` the arm 0 has `0x89998 XOR ECX,ECX` + `0x8999A XOR EBX,EBX` before the
call, so the install is **deferred** (no invoke-now): the body runs only when the
record's per-record action machine next calls `[rec+0x18]`. Fresh `get_xrefs_to
0x7D9A4` = **77** (byte-identical to FU-174/FU-182/FU-189/FU-190); both `0x899AB`
(row `0x20`) and `0x899CD` (the sibling ball row `0x1F`) are in the set.

### 2.3 The engine coupling (fresh source)

* Action-table `[0x1D]` is `fifa96_match_action_1D` (keeper-closedown —
  `handlers.c:5021`), a **different** code from the row-`0x20` installer:
  the phase-`0x1D` handler is the act-7 body.
* **Phase-table `[0x1D]` slot is `NULL`** (`handlers.c:5114`). The body
  `fifa96_match_action_phase_1D` is **ported** (`match_run.c:5802`) and exported
  for direct unit testing, but its arm 0 does **not** implement the `0x899A2`
  install — the line `5821-5823` explicitly records the row-`0x20` installer
  (`0x899A2`), the ball row-`0x1F` install (`0x899CD`) and the `0x7A028` reset as
  **FU194-L7/L8 legs** (the actor `[0x1587D4]` / ball-record producers are
  unported; a fake install is forbidden).
* The engine's act seam refuses a NULL phase row: `fifa96_match_act_dispatch`
  (`fifa96_match_act.c:51`) and `fifa96_match_act_tick` (`:63`) both do
  `if (fifa96_match_phase_table[phase].fn == NULL) return FIFA96_OK;` **before**
  `fifa96_match_dispatch_phase` (which itself returns `-UNSUPPORTED` for a NULL
  `fn`, `handlers.c:5131`).

---

## 3. Does R50's live wiring make the row-`0x20` installer fire?

**No.** R50 (→ FU-202) derives the situation-machine live arm (`period_setup_live`
un-gating) and the situations `0`/`1`/`8` reachability. Freshly traced, the
situation-8 chain is:

1. `fifa96_match_run_period_setup(mode)` (`match_run.c:2989`) calls
   `fifa96_match_run_set_piece(mr, 8, side, 0)` at the two extra-time branches
   (`0x888C8`, `:3008`; `0x4B0D9`, `:3020`). It is reached **only** when
   `mr->period_setup_live` is set (`:2244`; defaults `0`, R9/FU196-L8).
2. `fifa96_match_run_set_piece(.., 8, ..)` (`:2908`): if `situation != 0` and
   `session_gate_14c32a` and `!situation_pending`, it takes the **queue arm**
   (`:2914-2938`) — situation 8's default latches `situation_id = 0xA` and
   returns **without any act dispatch**. Otherwise (`:2977`) it falls to
   `match_run_situation_side(mr, 8, side, 1)`.
3. `match_run_situation_side` (`:2699`): `fifa96_action_phase_situation(8)` =
   `{phase = NONE, act = 7, stage = 0}` (`fifa96_action_handlers.c:1142`), and
   `out.act != NONE` ⇒ `fifa96_match_act_dispatch(mr, 7, 0, invoke=0)`
   (`:2741`; invoke is 1 only for act 2, `:2742`).
4. `fifa96_match_act_dispatch(7,0,0)` (`fifa96_match_act.c:38`) latches
   `mr->act.id = 7` and returns (no invoke).
5. The clock tail `fifa96_match_act_tick` (`:55`) maps act 7 → phase `0x1D` via
   `fifa96_action_phase_act(7)` (`0x16 + 7 = 0x1D`) and hits
   `fifa96_match_phase_table[0x1D].fn == NULL` → **early return.**

**Verdict.** R50's live wiring would make **act 7 reachable** (the situation-8 →
act-7 route becomes drivable), but the row-`0x20` installer does **not** fire,
for **two independent reasons**:

* **(b) the phase-`0x1D` engine slot is NULL** (`handlers.c:5114`) — the act tick
  bails at `fifa96_match_act.c:63`, so the arm-0 body never runs;
* **(c) even with the slot wired, the ported arm 0 does not implement the
  `0x899A2` install** (`match_run.c:5821-5823`, a FU194-L8 leg: the actor
  `[0x1587D4]` producer has no engine cell — fresh grep finds no `1587d4` cell).

So the row-`0x20` installer needs **three** landings, in order: **R50/T3** the
live situation-8 producer → **T3** the phase-`0x1D` slot registration → **T5**
the arm-0 `0x899A2` install (the actor-record producer). R50 alone lands none.

There is a **third** gate between (a) and (b): the **`session_gate_14c32a` queue
trap** (§4 cell 7). With the session gate open and no pending situation, the
native (and the port) route situation 8 to the queue arm (`situation_id = 0xA`),
**not** to act 7. So the live situation-8 → act-7 route additionally requires the
session gate clear **or** a pending situation (`FU194 §3.4`).

---

## 4. The remaining gate cells (pinned)

| # | cell | native | engine status | owner / leg |
|---|---|---|---|---|
| 1 | `phase_meta` | `[rec+0xC]` | `NULL`; no writer | R47/T1; FU192-L1 / OL-13 |
| 2 | `phase_meta10` | `[rec+0x10]` | `NULL`; no writer | R47/T1; FU192-L1 / OL-13 |
| 3 | `phase_meta14` | `[rec+0x14]` | `NULL`; no writer | R47/T1; FU192-L1 / OL-13 |
| 4 | `phase_dir_default[2]` | `word[0x1577DE + side*4] >> 16` | staged `0` (`match_run.c:2034-2035`, `:2150-2151`) | the `edge < 0` index for rows `02`/`08`/`09`/`15`; FU189/FU191 leg |
| 5 | `sel_1577d8[2]` / `sel_1577d6[2]` | `word[0x1577D8/D6 + gside*4]` | staged `0` (`handlers.c:4647-4648`) | keeps phase-`04` on the DX≥0 arm; FU192-L3 |
| 6 | `cell_157754` | `[0x157754]` | staged `0` (`handlers.c:4715`) | the row-`06` index; FU192-L1 |
| 7 | `session_gate_14c32a` | `byte[0x14C32A]` | modelled (`mr->session_gate_14c32a`) | the sit-8 queue trap (`set_piece:2914`); must be clear/pending for act 7; FU194 §3.4 |
| 8 | `global_157ac2` (extra-time) | `[0x157AC2]` | modelled; producer **OL-85** | the phase-0xC sit-8 branch (`:3005`); R50 scope |
| 9 | `input_range[0..1] & 0x100` | the `0x4B0CB TEST AH,1` | modelled (`:3019`) | the `0x4B0D9` sit-8 branch; FU196-L1 |
| 10 | actor `[0x1587D4]` / ball `[team+0x7A6]` | — | **no engine cell** | the row-`0x20`/row-`0x1F` installer **targets**; FU194-L8 |

Fresh confirmations: `0x6DE44` reads `word[EBX+0x1577D8]` at `0x6DE65` (EBX =
`gside*4`) and branches `0x6DE73 JL 0x6DEE5` (the DX<0 cell-map arm — cell 5);
`0x6DE7E MOV EAX,[EBP+0x14]` (cell 3).

---

## 5. T5 rec + the census delta

**T5 rec (per row / per route):**

* **The 7 gated rows + phase `0x02`** — they are **already wired and counted**
  (ENGINE.md: the phase rows `0x02…0x15` cohort). T1 must make them *genuinely
  live* by writing cells 1–3 (`phase_meta`/`phase_meta10`/`phase_meta14`) on the
  live begin path; the gate truth-table is a **single disjunct per body**
  (`meta == NULL`, §1.1). Until then they stay the documented
  dispatched-but-gated no-ops. **T1 must pin the tape-dormancy** of the metadata
  write (R47/R48's verdict) — the tape visits phases `0x02`/`0x04`/`0x15`
  (FU192 §4), so a live write that changes `target_*` on those records is
  tape-visible and must be cmp-pinned or leg'd.
* **The row-`0x20` installer** — do **not** count yet. It needs the **coupled
  triple** (§3): the live situation-8 producer (R50/T3), the phase-`0x1D` slot
  registration (T3), **and** the arm-0 `0x899A2` install + its actor-record
  producer (T5). R50's live wiring alone fires nothing. Keep the coupled
  phase-`0x1D`/row-`0x20` pair as one unit (FU-194 §6.1): landing phase `0x1D`
  **without** the arm-0 install leaves the row-`0x20` slot NULL (no fake
  install), and landing the arm-0 install without the live producer is inert.

**Census delta = 0.** HEAD `2a29245` already counts the phase rows
`0x02/0x03/0x04/0x06/0x07/0x08/0x09/0x15` as wired (`docs/ENGINE.md:8-10`,
`264-265`); the row-`0x20` action slot and the phase-`0x1D` slot are both
NULL/uncounted. R53 is read-only and lands nothing, so the verified count stays
**48/80 = 60%** (31 action + 17 phase). No row is asserted live.

---

## 6. Numbered legs

1. **FU205-L1 — the `phase_meta`/`phase_meta10`/`phase_meta14` writers.** The
   three OL-13 metadata pointers ([rec+0xC]/[rec+0x10]/[rec+0x14]) are `NULL`;
   the writers are R47/T1's formation/line-up setup. Their **tape-dormancy** on
   the on-tape phases `0x02`/`0x04`/`0x15` is unproven here (R47/R48's scope).
   Owner: T1 / FU192-L1. Not decomposed.
2. **FU205-L2 — the row-`0x20` installer triple.** Firing `0x899A2` requires
   (i) the live situation-8 producer (R50/T3), (ii) the phase-`0x1D` slot
   registration, and (iii) the arm-0 install + its actor-record `[0x1587D4]`
   producer. R50's situation wiring alone fires none of (ii)/(iii) — verified
   first-hand (§3). Owner: FU194-L8 / FU196-L8 / T3 / T5.
3. **FU205-L3 — the `session_gate_14c32a` queue trap on the sit-8 route.**
   With the gate open and no pending situation, situation 8 routes to the queue
   arm (`situation_id = 0xA`), never to act 7 (`set_piece:2914-2938`). The
   live-wiring rec must stage the gate clear (or a pending situation) or the
   act-7 route is unreachable. Cited, not resolved (FU194 §3.4).
4. **FU205-L4 — the extra-time flag (OL-85) for the phase-0xC sit-8 branch.**
   `global_157ac2 == 4` gates the `0x888C8` sit-8 dispatch (`:3005`); its writer
   (`0x8B2AC`/`0x8B2C1` / `[0x157AC0]`, per ENGINE.md:3534) is R50's scope. If
   unproduced, only the `0x4B0D9` (`input_range & 0x100`) sit-8 branch can fire.
5. **FU205-L5 — cells 4/5/6 (the staged-zero value gates).** `phase_dir_default`,
   `sel_1577d6`/`sel_1577d8` and `cell_157754` are unported (staged `0`). They
   do **not** no-op the body — they select which native branch/row the body
   takes — but they keep the rows on the derived sub-arm; the true producers are
   FU192-L1/L3 legs. Listed for completeness.

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` (program `FIFA96.EXE`, watcom LE, image base 0,
  min `0x10000`).
* `read_memory`: `0x8999C` (40 B, the row-`0x20` install + the `0x899CD` sibling),
  `0x110808` (4 B, phase slot `[0x1D]`), `0x110760` (4 B, action slot `0x20`),
  `0x110794` (92 B, the phase table).
* `disassemble_bytes`: `0x89868` (220 B, the phase-`0x1D` head + arm-0 prologue),
  `0x6DD9C` (70 B, the `[rec+0x10]` read), `0x6DE44` (128 B, the `[rec+0x14]` +
  `0x1577D8` read).
* `decompile_function`: `0x6DCC8` (the `[rec+0xC]` read), `0x7D9A4` (the installer
  contract).
* `get_xrefs_to`: `0x89868` (**1**, DATA `0x110808`), `0x7D9A4` (**77**).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(`:4578-4601` `match_phase_dcc8_apply`; `:4603-4611` `fifa96_match_phase_02`;
`:4644-4681` `fifa96_match_phase_04`; `:4712-4743` `fifa96_match_phase_06`;
`:4748-4763` `fifa96_match_phase_08`; `:5021/5027/5037` the action rows
`0x1D`/`0x20`/`0x25`; `:5055-5126` the phase table; `:5128-5179` the dispatch
functions); `src/fifa96_engine/fifa96_match_act.c` (`:38-65`, the act seam +
tick); `src/fifa96_engine/fifa96_match_run.c` (`:2244` the `period_setup_live`
gate; `:2418-2458` the walk + `match_run_write_phase`; `:2699-2745`
`match_run_situation_side`; `:2908-2978` `set_piece`; `:2989-3025`
`fifa96_match_run_period_setup`; `:5802-5848` `fifa96_match_action_phase_1D`);
`src/fifa96_loader/fifa96_action_handlers.c` (`:1129-1147` the situation table);
`include/fifa96_engine/fifa96_match_entities.h` (`:179/186/187` the metadata
pointers); `include/fifa96_engine/fifa96_match_run.h` (`:1486-1509`).
Docs cross-cited: FU-174, FU-189 (§1/§3/§6/§10), FU-190 (§6.2), FU-192
(§1/§2/§4), FU-194 (§3.2/§3.4/§4.3/§6.1/§6.2/§7), FU-196 (§3/§4, L1/L8),
FU-175, FU-191, FU-198, FU-183. `docs/ENGINE.md` (`:8-10`, `:264-265`, `:564-566`,
`:713-714`), `README.md` title.

No writes beyond this slice file and the workspace report; no rename/label/comment
/function/script/project save; no git.

---

## Addendum — T3 landing status (2026-10-12, additive)

This addendum records the **T3** verdict against §3; it does not rewrite any
section above.

- **§3's "does R50's live wiring make the row-`0x20` installer fire?" — still
  NO.** T3 did **not** register phase `0x1D` or the row `0x20`; the pair stays
  `NULL/uncounted` (**leg FU205-L2**), because the route needs the unported
  extra-time flag (OL-85, FU202-L1) **and** the arm-0 `0x899A2` actor-record
  producer (`[0x1587D4]`, no engine cell). No fake install.
- **What T3 DID land (the sibling situation-0 chain):** the match-init situation
  **0** → act `0xA` → phase `0x20` → code `0x24` → row `0x24` chain is now live
  (the producer re-placed after the begin-tail act reset), and phase `0x20` +
  action row `0x24` are wired/counted (**+2 → `50/80 = 62%`**, M2 re-pinned to
  `441415fc…`; FU-202 addendum). The two chains are independent: situation 0 is
  live, situation 8 stays dormant.

---

## Addendum — T5 landing status (2026-10-12, additive)

This addendum records the **T5** verdict; it does not rewrite any section above.
T5 is a verification + legs slice: **no production code**, `+0`.

### T5.1 The gate truth-table — verified and pinned

§1's truth-table is now exercised cell-per-row by
`tests/test_engine_match_handlers.c::test_gated_rows_gate_truth_table`: for each
of the eight gated rows (`0x02`/`0x03`/`0x04`/`0x06`/`0x07`/`0x08`/`0x09`/`0x15`)

1. **asset-absent** — the row's metadata pointer `NULL` (plus its siblings): the
   body is a **disciplined no-op** (the target triple is untouched);
2. **asset-present** — the pointers staged by the live bind
   `fifa96_match_entities_bind_record_meta` (T1/FU-199 §7.2): the body **runs**
   and (re)writes the target triple; and
3. **wrong-field** — only the *other* two pointers bound, the row's own `NULL`:
   still a no-op, so the gate is **exactly** the body's own metadata field
   (`phase_meta` `[rec+0xC]` rows `0x02`/`0x08`/`0x09`/`0x15`; `phase_meta14`
   `[rec+0x14]` rows `0x03`/`0x04`/`0x07`; `phase_meta10` `[rec+0x10]` row
   `0x06`) and not a sibling. All eight rows are asserted wired (`fn != NULL`)
   and counted (`phase_expect == OK`).

The engine gate sites are unchanged and remain the native single disjuncts:
`handlers.c:4595` (`meta == NULL`), `:4671` (`p == NULL`, `phase_meta14`),
`:4733` (`p == NULL`, `phase_meta10`) — matching §1's fresh body reads
(`0x6DCC8` `[rec+0xC]`, `0x6DDE4`… `0x6DE44` `[rec+0x14]`, `0x6DD9C`
`[rec+0x10]`).

**Evidence (T5):** GREEN — `./build/test_engine_match_handlers` → `OK`; full
`make check` **110/110**. mutation-RED — in `match_phase_dcc8_apply` replacing
the gate with a fallback (`if (meta == NULL) meta =
(const int8_t *)rec->phase_meta10;`) aborts **only** the new truth-table test at
its wrong-field assertion (`... test_gated_rows_gate_truth_table: Assertion
'e->target_x == 0x444 && …' failed`); the three pre-existing per-body tests do
not catch that mutation, so the new cell adds independent coverage. Restored
byte-identical.

### T5.2 Do T3/T4 open any remaining T5 row? — NO (`+0`)

- **T3** landed the situation-**0** chain (situation 0 → act `0xA` → phase
  `0x20` → row `0x24`). **T4** landed the act-1 route (situation 1 → act 1 →
  phase `0x17`). Neither route installs row `0x0D`, row `0x1A` or row `0x20`,
  nor dispatches phase `0x1D`/`0x1E`.
- The situation-**8** producer (act 7 → phase `0x1D` → row `0x20`) is still
  gated: the extra-time flag (`global_157ac2 == 4`, OL-85) and the
  `input_range & 0x100` extra-time branch (§4 cells 8/9) are the only sit-8
  gates, and the phase-`0x1D` engine slot stays `NULL`
  (`fifa96_match_handlers.c:5117`); the row-`0x20` action slot stays `NULL`
  (`:5030`). The arm-0 `0x899A2` install is still absent (§3 (c)).
- Fresh first-hand confirmation (this session, `/FIFA96.EXE`): phase slot
  `0x110808` = `0x89868` (fresh `get_xrefs_to 0x89868` = 1, DATA from
  `0x110808`); phase slot `0x11080C` = `0x8A798`; action slot `0x110760` =
  `0x84EEC`; `0x80F61` = `BA 0D 00 00 00` / `0x80F6A CALL 0x7D9A4`; `0x899A2`
  = `BA 20 00 00 00` / `CALL 0x7D9A4`; `get_xrefs_to 0x77EAC` = 1 (into
  `FUN_000782D0`). So the coupled phase-`0x1D`/row-`0x20` pair and the row-`0x0D`
  / row-`0x1A` installers are all still unported → **count nothing**.

### T5.3 Stale-reference correction (erratum to the T5 brief)

The T5 brief's frozen list named **row `0x24`/phase `0x20` = FU188-L1** as
still frozen. That is **superseded**: T3 wired/counted both (FU-202 addendum /
§Addendum above; census `50/80 = 62%`). The frozen **sit-8** pair is
phase-`0x1D`/**row `0x20`** (a different row: `0x20`, not `0x24`), owner
**FU205-L2**. No consumer of this slice sees a false frozen `0x24`/phase-`0x20`
claim.

### T5.4 Census

`+0`: action `32` + phase `19` = **`51/80 = 63%`** unchanged (`floor(100 ×
51/80) = 63`); dispatch `28 UNSUP / 51 OK / 1 NOTF` (sum 80). README title
`63%` unchanged. Tape: M2 `cmp` clean against the v18 `441415fc…` golden (**no
re-pin**); M1 `09b726b7…` unmoved.

### T5.5 Residual legs (honest, numbered)

1. **FU178-L1 — row `0x0D`.** Its only installer is row-09 arm 1 `0x80F61`
   (fresh bytes `BA 0D 00 00 00` / `CALL 0x7D9A4`), inside the unported arm-1
   stage; the gate producers `[0x15877C]`/`[rec+0x3D]`/`[rec+0x24]`/`[[rec+4]+
   0x13]` and the arm-1 helpers/sinks (`0x8F188`/`0x8ED40`/`0x92820`/`0x7A490`/
   `0x8A3FC`) are unported (FU-204 §4). The body exists as
   `fifa96_action_sequence_0d_step`; wire nothing.
2. **FU186-L5 — row `0x1A`.** Its only installer is the keeper CPU ladder
   `FUN_00077EAC` (the three `#x1A` sites `0x78227`/`0x7824F`/`0x78277`; fresh
   `get_xrefs_to 0x77EAC` = 1 into the unported `FUN_0008D8EC → FUN_000782D0`
   pre-pass). The body is ported (`fifa96_match_action_1A`) but the entry is
   unported; the FU-204 §2 ECX→arm map is the eventual port spec. Wire nothing.
3. **FU205-L2 — the coupled phase-`0x1D`/row-`0x20` pair.** Still requires the
   live situation-8 producer (the extra-time flag + the `input_range & 0x100`
   branch) **and** the arm-0 `0x899A2` install + its actor-record `[0x1587D4]`
   producer. Fresh: `0x899A2` = `MOV EDX,0x20; CALL 0x7D9A4` inside `0x89868`
   (sole ref the phase slot `0x110808`); the pair stays `NULL`/uncounted. No fake
   install.
4. **FU194-L2 — phase `0x1E` (act 8).** The engine already models the act-8
   fallback as the **compressed** stored-situation re-dispatch
   (`fifa96_match_run_set_piece` BX≠0); registering the full 22-record sweep
   would de-compress a tape-visible derivation. Fresh: phase slot `0x11080C` =
   `0x8A798`. Leave `NULL`.
5. *(unchanged)* FU205-L1 (the metadata writers) is **closed** by T1:
   `fifa96_match_entities_bind_record_meta` + `match_run_record_meta_bind`
   (FU-199 §7.2 / FU-200 §10). FU205-L3/L4 (the `session_gate_14c32a` queue trap;
   the OL-85 extra-time flag) stand. FU205-L5 (the staged-zero value gates,
   FU200-L3) stands.
