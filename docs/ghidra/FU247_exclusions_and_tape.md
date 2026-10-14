# FU-247 — the permanent-exclusion register: action `0x22`/OL-65, the phase
# `0x01`/`0x11`/`0x12` double-wire trio, and the tape / re-pin term inventory

Provenance: M2 phase-23 wave **W23-8** (planned FU-247), 2026-10-14, **read-only**.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument; standard tools only; **no** rename / comment /
label / function-create / script / project save). `get_current_program_info` this
session: name `FIFA96.EXE`, path `/FIFA96.EXE`, `watcom:LE:32:default`, image base
`0`, `min_address 0x10000`, **2706 functions / 15239 symbols** (matches the
phase-23 baseline). Every address/byte/instruction/xref below is a fresh
`read_memory` / `disassemble_bytes` / `get_xrefs_to` / `get_current_program_info`
of **this session** unless marked *cross-cited* (then the frozen FU doc is named).
Engine/loader source is read first-hand. The alias traps are applied
(`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`); the row-07 CALL rel32 is
verified byte-level. **No repo file was modified beyond this draft + the report;
no git; no subagents.**

Question (plan Task 8 / spec §4 slice 8): derive (1) the structurally-ungated row
**action `0x22` / OL-65** and why `fifa96_entity_angle(lane_z, 0)` can never yield
the `< 0x55` diff (FU185-L6); (2) the permanent double-wire register **phase
`0x01`/`0x11`/`0x12`** (`0x6E1D0`/`0x6E1C8`) and its seeding-path realization
(FU192-L6/L7); (3) the **tape / re-pin term inventory** for the Stage-2 producer
plan. End with the census wire-vs-ceiling for action `0x22` and the
permanent-exclusion verdict for the trio.

---

## 1. Headline

- **Action `0x22` is a permanent ceiling leg.** The slot `0x1106E0[0x22] = 0x8539C`
  (fresh) is NULL in the engine; the body is ported but the only installer
  (row-07 `0x816DF` invoke-now, fresh-verified) never fires because the engine
  stages the angle inputs 0 / mis-sourced (§2.3). Owner **OL-65** (FU185-L6).
- **The phase `0x01`/`0x11`/`0x12` trio is a permanent exclusion** (R10): `0x01`
  is realized by the seeding path (`fifa96_match_entities_seed_formation` →
  `0x6E1D0`); `0x11`/`0x12` share the same body and are never written. Wiring any
  member double-runs the formation placement (FU192-L6/L7).
- **Census unchanged: 55/80 = 68%** (33 action + 22 phase). No row in this slice
  is wireable.
- **Tape (fresh v22, 6522 lines):** the walk visits **8 phases**
  `{0x00,0x01,0x02,0x04,0x0C,0x13,0x14,0x15}`. The NULL phase rows **on-tape**
  are `0x01` (953 frames), `0x13` (2640), `0x14` (20); the rest are dormant. The
  only **tape-visible** NULL *action* row is `0x03` (frame 29, in the `0x14`
  window); action `0x22` is install-live but tape-dormant. **FU192 §4's tape
  counts are stale** (older 792-line golden) — see §4.2.

---

## 2. Action `0x22` / OL-65 — the structurally-ungated row

### 2.1 The slot and the ported body

Fresh `read_memory 0x1106E0` (180 B) — slot `0x22` (offset `0x88`) = bytes
`9c 53 08 00` ⇒ `0x1106E0[0x22] = 0x8539C`. The engine action table
(`handlers.c:5164`) holds `{0x22, NULL, …}` ⇒ `fifa96_match_dispatch_action(mr,
0x22)` returns `-FIFA96_ERR_UNSUPPORTED` (the rows-27/29/2C precedent). The body
`fifa96_match_action_22` (`handlers.c:4323`) is ported and exported
(`fifa96_match_handlers.h:202`) for direct unit testing only.

### 2.2 The installer — row-07 `0x816DF` invoke-now (fresh byte-level)

Fresh `disassemble_bytes 0x8164B` (170 B). The post-kick opponent-invoke arm:

| addr | bytes | instruction |
|---|---|---|
| `0x8164B` | `66 85 c0` | `TEST AX,AX` (kick-staged guard, tail of the prior arm) |
| `0x8164E` | `0f 84 90 00 00 00` | `JZ 0x816E4` |
| `0x81654` | `a1 3c 87 15 00` | `MOV EAX,[0x15873C]` |
| `0x81659` | `c1 f8 10` | `SAR EAX,0x10` (`word[0x15873E]`, kick traj) |
| `0x8165C` | `83 f8 30` | `CMP EAX,0x30` |
| `0x8165F` | `0f 8d 7f 00 00 00` | `JGE 0x816E4` (`traj >= 0x30` skip) |
| `0x81665` | `8b 45 00` | `MOV EAX,[EBP]` (team base) |
| `0x81668` | `8b b0 a6 07 00 00` | `MOV ESI,[EAX+0x7A6]` (opponent team) |
| `0x8166E` | `8b 95 89 00 00 00` | `MOV EDX,[EBP+0x89]` (`timer89`) |
| `0x81674` | `8b b6 b2 07 00 00` | `MOV ESI,[ESI+0x7B2]` (opponent **target**) |
| `0x8167A` | `83 fa 05` | `CMP EDX,5` |
| `0x8167D` | `7d 65` | `JGE 0x816E4` (`timer89 >= 5` skip) |
| `0x8167F` | `85 f6` | `TEST ESI,ESI` |
| `0x81681` | `74 61` | `JZ 0x816E4` (no target skip) |
| `0x81683` | `8b 86 8e 00 00 00` | `MOV EAX,[ESI+0x8E]` |
| `0x81689` | `c1 f8 18` | `SAR EAX,0x18` (`EAX = byte[opp+0x91]` = opp **code**) |
| `0x8168C` | `83 f8 06` | `CMP EAX,6` |
| `0x8168F` | `75 53` | `JNZ 0x816E4` (opp code ≠ 6 skip) |
| `0x81691` | `83 7e 20 00` | `CMP dword[ESI+0x20],0` (`has_slot`) |
| `0x81695` | `75 4d` | `JNZ 0x816E4` |
| `0x81697` | `66 81 7e 6b d0 00` | `CMP word[ESI+0x6B],0xD0` (lane band) |
| `0x8169D` | `7d 45` | `JGE 0x816E4` (`lane >= 0xD0` skip) |
| `0x8169F` | `8b 56 6d` | `MOV EDX,[ESI+0x6D]` |
| `0x816A2` | `8b 46 6b` | `MOV EAX,[ESI+0x6B]` |
| `0x816A5` | `c1 fa 10` | `SAR EDX,0x10` (**z = word `+0x6D`**) |
| `0x816A8` | `c1 f8 10` | `SAR EAX,0x10` (**x = word `+0x6B`**) |
| `0x816AB` | `e8 c0 c6 00 00` | `CALL 0x8DD70` (angle thunk → `0xCD474`) |
| `0x816B0` | `66 2b 46 7d` | `SUB AX,word[ESI+0x7D]` (`angle − face_word`) |
| `0x816B4` | `80 e4 03` | `AND AH,0x3` (fold to 10 bits) |
| `0x816B7` | `0f bf d0` | `MOVSX EDX,AX` |
| `0x816BA` | `81 fa 00 02 00 00` | `CMP EDX,0x200` |
| `0x816C0` | `7e 09` | `JLE 0x816CB` |
| `0x816C2` | `ba 00 04 00 00` | `MOV EDX,0x400` |
| `0x816C7` | `29 c2` | `SUB EDX,EAX` |
| `0x816C9` | `89 d0` | `MOV EAX,EDX` |
| `0x816CB` | `98` | `CWDE` |
| `0x816CC` | `83 f8 55` | `CMP EAX,0x55` |
| `0x816CF` | `7d 13` | `JGE 0x816E4` (diff ≥ 0x55 skip) |
| `0x816D1` | `b9 01 00 00 00` | `MOV ECX,1` (**invoke-now**) |
| `0x816D6` | `ba 22 00 00 00` | `MOV EDX,0x22` (**code 0x22**) |
| `0x816DB` | `89 f0` | `MOV EAX,ESI` (**opponent target record**) |
| `0x816DD` | `31 db` | `XOR EBX,EBX` (**staged = 0**) |
| `0x816DF` | `e8 c0 c2 ff ff` | `CALL 0x7D9A4` — rel32 `0xFFFFC2C0 = -0x3D40`; `0x816E4 − 0x3D40 = 0x7D9A4` **verified** |
| `0x816E4` | … | advance: `+0x89 = 0`, `stage++` (the store at `0x816E4..`) |

This matches the engine gate in `fifa96_action_handlers.c:1695-1709`
(`s->kick_staged && s->kick_traj < 0x30 && s->timer89 < 5 && s->opp_present &&
s->opp_type == 6 && s->opp_has_slot == 0 && s->opp_lane_word < 0xD0`).

### 2.3 The angle gate — fresh re-derivation (refines FU185-L6)

`0x8DD70` is a **thunk** (fresh `disassemble_bytes 0x8DD70`, 32 B): it pushes
`EBX`/`ECX`, then `MOVSX EDX,DX` + `PUSH EDX`, `CWDE` + `PUSH EAX`, `CALL
0xCD474`, cleans the stack, restores `EBX`/`ECX`, `RET`. So the row-07 gate calls
`fifa96_entity_angle(x = AX, z = DX)` — i.e.

```
angle = entity_angle( word[opp+0x6B], word[opp+0x6D] )
diff  = fold10( angle − word[opp+0x7D] );  fire iff diff < 0x55
```

The three native inputs are therefore **`word[+0x6B]` (lane_x), `word[+0x6D]`
(lane_z), `word[+0x7D]` (face7d)** — *not* `+0x6F`.

The engine's kick-machine-07 staging (`handlers.c:983-985`, read first-hand):

```
s->opp_angle_x  = opp->lane_z;   /* +0x6D */
s->opp_angle_z  = 0;             /* comment: "word[+0x6F] unmodeled (OL-65)" */
s->opp_face_word = 0;            /* comment: "+0x7D facing word unmodeled (OL-65)" */
```

so the loader (`fifa96_action_handlers.c:1701`) evaluates
`fifa96_action_kick_angle(s->opp_angle_x, s->opp_angle_z)` =
`fifa96_entity_angle(lane_z, 0)`, and the folded diff is against
`opp_face_word == 0`.

**Why the diff can never be `< 0x55`.** Fresh `disassemble_bytes 0xCD474` (96 B)
confirms the entity-angle body. For any `x` with `z == 0`: if `x > 0` the flags
path is `case 4` ⇒ `value = 0x100`; if `x == 0` ⇒ `value = 0x80`; if `x < 0`
the flags are `16|4` ⇒ `value = −0x100`. Folded against `face_word == 0`:
`0x100`, `0x80`, or (`−0x100 & 0x3FF` =) `0x300 → 0x400−0x300 = 0x100`. All are
`> 0x55`. A brute-force sweep of every `lane_z ∈ [−0x1200, 0x1200]` confirms
`out.opponent_invoke` never fires (FU185-L6, cross-cited; the arithmetic is
re-derived first-hand here). **The row-07 installer is structurally ungated
in-engine.**

**Erratum/refinement of FU185-L6's input map (new, first-hand).** FU185-L6 (and
the ENGINE.md/`fifa96_match_handlers.h:192-194` restatements) name `word[+0x6F]`
as the *second* angle input. The fresh disassembly shows the second input is
`word[+0x6D]` (`0x8169F`/`0x816A5`), and the *third* native term is
`word[+0x7D]` (`0x816B0`). No `+0x6F` read exists anywhere in `0x8164B..0x816E4`.
Moreover all three native inputs **are** pool fields — `entity->lane_x` (+0x6B),
`entity->lane_z` (+0x6D), `entity->face7d` (+0x7D)
(`fifa96_match_entities.h:134,151`). So the gate's deadness is caused by the
kick-struct **staging** (`opp_angle_x` mis-sourced from `lane_z`, and
`opp_angle_z`/`opp_face_word` hardcoded 0), not by an absent `+0x6F` producer.
Whether re-sourcing those three fields would let the gate fire on a live replay
is a differential question outside this read-only slice — recorded as
**FU247-L1** (do not over-claim; the *outcome* — the gate never fires today —
is unchanged).

### 2.4 Verdict — action `0x22`

**Ceiling leg.** The body is ported; the only installer is the wired row-07
`0x816DF`, whose angle gate is dead in-engine. The slot stays `NULL`/uncounted
(rows-27/29/2C precedent; census rule). Owner **OL-65 / FU185-L6** (with the
FU247-L1 input-map refinement). It becomes countable only if/when the gate can
actually fire (a staging fix or the OL-65 producer). **Do not wire.**

---

## 3. The phase `0x01`/`0x11`/`0x12` double-wire trio

### 3.1 The bodies and the table (fresh)

Fresh `read_memory 0x110794` (140 B) — byte-identical to FU192 §1:
`[0x01] = 0x6E1D0`, `[0x11] = 0x6E1C8`, `[0x12] = 0x6E1D0`.
Fresh xrefs:

* `get_xrefs_to 0x6E1D0` = **4** — `CALL 0x866E7`, `CALL 0x876D0` (action-body
  callers), `DATA 0x110798`, `DATA 0x1107DC` (the phase-table slots `0x01`/`0x12`).
* `get_xrefs_to 0x6E1C8` = **1** — `DATA 0x1107D8` (the phase-table slot `0x11`).

Fresh `disassemble_bytes 0x6E1C8` (130 B) confirms the shim: `0x6E1C8 MOV
EBX,0xFFFFFFFF`; `0x6E1CD LEA EDX,[EAX+0x4D]`; then falls into `0x6E1D0`, whose
epilogue (`0x6E1D6 MOV ECX,EDX`; `0x6E1D8 MOV EBX,[0x157AAC]`) **overwrites both
EBX and EDX**. So **row `0x11` ≡ rows `0x01`/`0x12`** under the walk ABI (FU192-L6
re-confirmed byte-level). `0x6E1D0` reads `[rec+0x8]` (the formation byte pair),
`[rec]`/`[team+0x826]` (side), `[0x157AAC]>>24`; writes
`target[0]=byte*0x26`, `target[1]=0`, `target[2]=byte*0x21` side-flipped.

### 3.2 The seeding-path realization (first-hand source)

* `match_run_formation_seed` (`fifa96_match_run.c:1915`, **called** at `:2280`)
  loads the `.fmt` blob and calls
  `fifa96_match_entities_seed_formation(&mr->entities, &formation, side)`
  (`:1929`).
* `fifa96_match_entities_seed_formation` (`fifa96_match_entities.c:346`) loops
  both teams × 11 records and calls `fifa96_scene_formation_place(formation, i,
  t, controlled_side, &target_x, &target_y, &target_z)` — the port of the native
  `0x6E1D0`.
* `match_run_write_phase` (`fifa96_match_run.c:2514`) dispatches the per-record
  walk **only** when `fifa96_match_phase_table[phase].fn != NULL` (`:2530-2535`).
  The phase-1 write is `match_run_write_phase(mr, FIFA96_MATCH_RUN_KICKOFF_PHASE)`
  (`:2544`; `KICKOFF_PHASE == 1u`, `fifa96_match_run.h:115`). Because slot `0x01`
  is `NULL`, the walk is a structural no-op — the placement happens **once** via
  the seeding path.

**Double-wire mechanism.** Wiring slot `0x01` makes `match_run_write_phase(1)`
additionally run `fifa96_match_run_record_phase_walk(mr, 1, side)` for both teams
(one per-record `0x6E1D0` dispatch each) on top of the seed. That is the
double-run R10 forbids: *"do not double-wire"* (`docs/ENGINE.md:629-632`;
`docs/ghidra/FU222_census_audit_phase20.md:215`; FU192-L7). Whether the
re-placement is idempotent (and thus tape-neutral) is **not** the criterion — the
ruling is structural, and the native walk's own per-frame cadence is not modelled
by the engine (the engine realizes phase `0x01` through the act-1 kickoff
handler, `docs/ENGINE.md` / `fifa96_match_phase_machine.c:136` `mr->entities.phase
= 1u`).

### 3.3 Phase-write census (fresh)

All fresh `match_run_write_phase` call sites in `fifa96_match_run.c`:
`:2544` (phase 1 = KICKOFF), `:2658`/`:2704` (`out.phase` from
`fifa96_action_phase_period_end`), `:2911`, `:2933` (0x15), `:3153` (0),
`:3494` (0), `:5890` (`out->phase_write` from the ref sequence), `:5963` (0x0A),
`:5996` (`phase` ∈ {6,7}, the foul arm), `:6207` (0x0B), `:6286` (0x10). **None
writes `0x11` or `0x12`**; phase `0x01` is written only at `:2544`. The
indirect sites (`out.phase` / `out->phase_write`) derive from the FU-149 /
FU-191 act tables (phases 0/2/6/7/0x0A/0x0C/0x10/0x15/0x1D…), corroborated by the
tape (§4.1: no `state=17`/`state=18`). Re-confirms FU192 §2.9 ("phase `0x12` is
never written by an engine source") and extends it to `0x11`.

### 3.4 Verdict — the trio (permanent-exclusion register)

| row | body | written? | on tape? | verdict |
|---|---|---|---|---|
| `0x01` | `0x6E1D0` | yes (`:2544`) | **yes, 953 frames** | **permanent double-wire exclusion (R10)** — realized by `seed_formation`; wiring double-runs |
| `0x11` | `0x6E1C8` (≡ `0x6E1D0`) | **no** | no | **permanent exclusion** — never-written (fake count) **and** the `0x01` body (FU192-L6) |
| `0x12` | `0x6E1D0` | **no** | no | **permanent exclusion** — never-written **and** the `0x01` body |

All three stay `NULL`/uncounted. **Do not wire.**

---

## 4. The tape / re-pin term inventory

### 4.1 The fresh M2 phase census (first-hand)

`tests/golden/engine/m2-frames.txt` = **6522 lines**, sha
`b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (v22, matches
the phase-23 baseline). The `state=<phase>/<home>-<away>` suffix is the native
`[0x157A4A]>>24` (`test_engine_m2.c:23`). Run-length over the state field:

| frames | phase | count |
|---|---|---|
| 0..5 | `0x00` | 6 |
| 6..25 | `0x13` | 20 |
| 26..45 | `0x14` | 20 |
| 46..65 | `0x02` | 20 |
| 66..369 | `0x00` | 304 |
| 370..469 | `0x15` | 100 |
| 470..772 | `0x04` | 303 |
| 773..845 | `0x02` | 73 |
| 846..849 | `0x0C` | 4 |
| **850..1802** | **`0x01`** | **953** |
| 1803..2502 | `0x02` | 700 |
| 2503..2702 | `0x13` | 200 |
| 2703..3402 | `0x02` | 700 |
| 3403..3602 | `0x13` | 200 |
| 3603..4302 | `0x02` | 700 |
| 4303..6522 | `0x13` | 2220 |

**Visited phase set = {0x00, 0x01, 0x02, 0x04, 0x0C, 0x13, 0x14, 0x15}.**
`tests/golden/engine/m1-frames.txt` = 691 lines, sha
`09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (**immutable**;
carries no `state=` field).

**NULL-row tape classification (the deliverable's inventory):**

* **Phase rows on-tape:** `0x01` (953), `0x13` (2640 = 20+200+200+2220), `0x14`
  (20). *(These are the per-record walk's phase byte; the walk visits them but
  their slots are NULL → dispatched-UNSUPPORTED no-ops.)*
* **Phase rows tape-dormant** (never visited): `0x0B, 0x0D, 0x0E, 0x11, 0x12,
  0x1A, 0x1B, 0x1C, 0x1E`.
* **Action rows:** `0x03` is the sole **tape-visible** NULL action row — wiring it
  moved M2 at **frame 29** (inside the `0x14` window, frames 26..45) via the live
  phase-2 `[rec+0x1C] = 0x6DCC8` target-rewrite chain (`test_engine_m2.c:1352`,
  FU177-L1). Action `0x22` is **install-live / tape-dormant** (M1/M2 byte-identical
  in the phase-16 T2 landing, FU185 §10). The remaining NULL action rows
  (`0x0A,0x0D,0x15,0x17,0x1A,0x25,0x27,0x29,0x2B,0x2C`) have no live installer
  and are tape-dormant.

### 4.2 Errata to prior tape counts (quoted, first-hand)

* **FU192 §4 quote:** *"phase `0x13` (19) frames 6..25; phase `0x14` (20) frames
  26..45 … `0x01 0x03 … 0x11 0x12` … never [on the tape]."* **Stale.** That
  census ran over the older **792-line** golden (`test_engine_m2.c:1340` cites
  "792 lines"). The fresh v22 golden (6522 lines) visits `0x01` for **953**
  frames, `0x13` for **2640** (not 20), and `0x0C` for 4 (also new). The
  double-wire verdict is **unchanged** (`0x01`/`0x11`/`0x12` still excluded), but
  the *tape-term* consequence is stronger: wiring phase `0x01` would put the
  formation re-placement onto **953 on-tape frames**, not zero.
* **Spec §1/§2/§5 "phase `0x14`'s 40 frames"** (and FU192's `0x13` = 20 frames):
  fresh = `0x14` **20** frames, `0x13` **2640** frames. The A-mover tape risk for
  `0x13`/`0x14` (FU240/FU192-L2) must be sized against **2660** on-tape frames,
  not 60.

### 4.3 Re-pin policy (pre-specified for the Stage-2 producer plan)

1. **Prefer additive / no re-pin.** A mover that is off-tape (or proven
   render-invisible + dormant) must leave M1/M2 **byte-exact**; the Stage-2
   acceptance asserts the mover is absent from the observed-OK set and `cmp`s the
   golden. Only the strata label may change.
2. **If a mover is tape-visible** (`0x03`; `0x13`/`0x14` iff not proven inert): a
   re-pin requires a **written reason + frame diff** (first divergent frame +
   count of differing lines), committed as its own task (T(k+1)).
3. **M1 `09b726b7…` never moves** (immutable).
4. **Any *unexplained* drift is a regression to investigate, never a re-pin.**

### 4.4 Mutation-RED rider classes a Stage-2 acceptance must add

Per newly-wired row, coupled with its RED/GREEN dispatch test (the phase-17/21/22
precedent, `test_engine_m2.c:1360-1390` rider block):

1. **Census rider** — the wired-count assertion (`action_wired`/`phase_wired`,
   pins at `test_engine_m2.c:3666`/`:4041`) moves by exactly the expected delta;
   NULLing the new row aborts.
2. **Dispatch rider** — the row's `fn != NULL` **and** dispatches `FIFA96_OK` on a
   staged fixture; stubbing the body aborts at the row's own stage assert.
3. **Producer/installer rider** — the live installer/producer actually reaches the
   row (force the producer; assert the row ran / its `dispatched_ok` bit is set),
   so a `NULL → fn` flip with no live producer aborts (the census rule).
4. **Tape rider** — on-tape rows: `cmp` the golden; off-tape rows: assert
   byte-identity **and** the row absent from the observed-OK mask. A mutation that
   puts an off-tape row onto the tape aborts.
5. **RNG rider (phase `0x13`/`0x14`, if attempted)** — the `0x92AC8` draw
   sequence matches the native: **4 calls per active record** (two pairs: draws
   1–2 → `target[2]`, draws 3–4 → `target[0]`; fresh `disassemble_bytes 0x6E244`,
   `0x6E2B3/0x6E2BA/0x6E302/0x6E309`). *(FU192-L2's "twice per active record" is
   an undercount — see FU247-L2.)* A draw-count/order mutation aborts.
6. **No-double-wire rider (phase `0x01`/`0x11`/`0x12`)** — wiring any of the trio
   aborts the exclusion pin.

---

## 5. Census verdict (wire-vs-ceiling)

* **Action `0x22`:** **CEILING.** Not countable — the only installer's gate is
  structurally dead in-engine (OL-65 / FU185-L6, refined by FU247-L1). Slot
  `NULL`; `dispatch_action(0x22) == UNSUPPORTED`.
* **Phase `0x01`/`0x11`/`0x12`:** **PERMANENT EXCLUSION (R10).** Not countable —
  `0x01` double-wires the seeding path; `0x11`/`0x12` are never written and share
  the `0x01` body.
* **Census holds at 55/80 = 68%** (33 action + 22 phase; dispatch
  `24 UNSUP / 55 OK / 1 NOTF`). No action/phase row in this slice is wireable.

---

## 6. Numbered legs (new)

1. **FU247-L1 — the action-`0x22` gate input map is mis-stated.** The native
   row-07 angle gate reads `word[+0x6B]`/`word[+0x6D]` (x/z → `0xCD474`) and
   subtracts `word[+0x7D]` (fresh §2.2/§2.3); FU185-L6 and the ENGINE.md /
   header restatements name `word[+0x6F]` as the second input. All three native
   inputs are pool fields (`lane_x`/`lane_z`/`face7d`); the gate is dead because
   the kick-struct staging hardcodes `opp_angle_z=0`/`opp_face_word=0` and
   mis-sources `opp_angle_x` from `lane_z`. **Owner OL-65 / FU185; the re-map
   belongs to whoever takes OL-65.** (Does not change the verdict: the gate never
   fires today.)
2. **FU247-L2 — the phase-`0x13`/`0x14` PRNG contract is 4 draws/active record.**
   Fresh `disassemble_bytes 0x6E244` shows two draw-pairs
   (`0x6E2B3`/`0x6E2BA` → `target[2]`, `0x6E302`/`0x6E309` → `target[0]`), each
   `CALL 0x92AC8` + `DIV 0x90`; FU192-L2 / ENGINE.md say "twice per active
   record". **Owner FU240 / FU192-L2.** (Sizes the RNG rider, §4.4 class 5.)
3. **FU247-L3 — the `0x01` double-wire is on-tape 953 frames.** Wiring phase
   `0x01` would run the per-record `0x6E1D0` placement across the 850..1802
   `state=1` block on top of the seed. Whether idempotent-for-the-tape is
   unproven; R10 excludes it structurally. **Owner FU192-L7 / the Stage-2
   tape-touch task.**
4. **FU247-L4 — the tape/re-pin terms (§4.3) are the Stage-2 T(k+1) acceptance
   contract.** Owner: the Stage-2 producer plan.
5. **FU247-L5 — `0x6E1D0` action-body double-run.** The two action-body callers
   (`0x866E7` in `FUN_0006E490`, `0x876D0`) share the placement body with the
   phase row; a future port must not double-count when both run (FU192-L7
   restated, fresh xrefs §3.1). Owner FU192-L7.

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info`; `read_memory 0x110794` (140 B), `0x1106E0` (180 B);
`disassemble_bytes 0x8164B` (170 B), `0x815D0` (123 B), `0x6E1C8` (130 B),
`0x6E244` (232 B), `0x8DD70` (32 B), `0xCD474` (96 B);
`get_function_by_address 0x8164B` (**no function** — a bare action-table/row-07
code region); `get_xrefs_to 0x6E1D0` (4), `0x6E1C8` (1), `0x6E244` (2),
`0x816DF` (0 — a CALL *site*, not a target).
Engine/loader source read first-hand:
`src/fifa96_engine/fifa96_match_handlers.c` (974-985, 1160-1170, 4313-4410,
5095-5313 — the action/phase tables), `src/fifa96_engine/fifa96_match_entities.c`
(344-361, 460-480), `include/fifa96_engine/fifa96_match_entities.h` (134, 151,
192-194, 377, 492), `src/fifa96_engine/fifa96_match_run.c` (1915-1945,
2280, 2500-2560, 2650-2710, 5878-5895, 5980-6000), `src/fifa96_engine/
fifa96_match_phase_machine.c` (120-150), `src/fifa96_loader/
fifa96_action_handlers.c` (1640-1730), `src/fifa96_loader/fifa96_entity_update.c`
(60-120). Tests/golden read first-hand: `tests/test_engine_m2.c` (1-120,
1340-1410, 3660-3670, 4030-4050); `tests/golden/engine/m2-frames.txt` (6522
lines; sha `b281a9b2…`), `tests/golden/engine/m1-frames.txt` (691 lines; sha
`09b726b7…`). Docs cross-cited: FU185, FU192, FU222 (`:215`), FU177-L1,
`docs/ENGINE.md` (`:339-346`, `:476`, `:620-632`), AGENTS.md census rule. No
writes beyond this draft and the workspace report; no rename/label/comment/
function/script/project save; no git; no subagents.

---

## 8. Stage-2 handoff

Direct input to the Stage-2 producer plan's tape/census close-out task
(spec §5): action `0x22` and phase `0x01`/`0x11`/`0x12` are **ceiling legs**
(this slice's frozen register); the tape terms (§4.3) and the mutation-RED rider
classes (§4.4) define the tape-touch (T(k+1)) and per-row (T2..Tk) acceptance;
the census pins (`tests/test_engine_m2.c:3666`/`:4041`) close out at
`floor(100 × 55/80) = 68` unless the wave's other slices wire rows.

---

## 9. Errata (quoted — prior sections not rewritten)

> **FU247 §2.3 "Erratum/refinement of FU185-L6's input map" (lines 153-166) and
> `FU247-L1` (lines 367-375) are themselves wrong — a second `dword[x]>>16` alias
> trap (`FU250` §5.1).** §2.3 reads: *"the second input is `word[+0x6D]`
> (`0x8169F`/`0x816A5`), and the *third* native term is `word[+0x7D]`
> (`0x816B0`). No `+0x6F` read exists anywhere in `0x8164B..0x816E4` …"*, and the
> §2.2 table labels `0x816A5 SAR EDX,0x10` as *"z = word `+0x6D`"* / `0x816A8
> SAR EAX,0x10` as *"x = word `+0x6B`"*. **Corrected (fresh; `FU250`
> §5.1/§5.2):** `0x8169F MOV EDX,dword[ESI+0x6D]` and `0x816A2 MOV
> EAX,dword[ESI+0x6B]` are **dword** loads; the `SAR 0x10` (`0x816A5`/`0x816A8`)
> selects the high word of each, so the row-07 angle gate reads
> `fold10(entity_angle(word[+0x6D], word[+0x6F]) − word[+0x7D]) < 0x55` —
> `word[+0x6D]` from `dword[+0x6B]>>16` (`0x816A2`/`0x816A8`) and `word[+0x6F]`
> from `dword[+0x6D]>>16` (`0x8169F`/`0x816A5`), minus `word[+0x7D]` (`0x816B0`).
> There is indeed **no literal `+0x6F` operand**, but the read target is
> `word[+0x6F]` via the alias — exactly the trap AGENTS.md warns about.
> Consequently **`FU185-L6`'s map (`+0x6D`/`+0x6F`/`+0x7D`) stands**;
> `fifa96_action_handlers.h:918-919` is correct; and `opp_angle_x = opp->lane_z`
> is correct (not "mis-sourced"). The gate's deadness was caused only by the two
> hardcoded zeros (`opp_angle_z`/`opp_face_word`) in the kick-struct staging; that
> staging was fixed by the later OL-65 closure (FU-259; `s->opp_angle_z :=
> opp->cam_dz6f`, `s->opp_face_word := opp->face7d`) and action `0x22` is now
> wired/counted (`fifa96_match_handlers.c:5530`) — subsequent to this read-only
> slice and **not** a rewrite of it. The §2.4/§7 verdicts are unaffected.
> Source: `FU250` §5.1/§5.2 (`FU250-L1`); AGENTS alias traps; `FU263` Erratum 1.
