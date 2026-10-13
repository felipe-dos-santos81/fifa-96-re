# FU-206 — census/audit + the phase-18 envelope + the per-producer tape verdict (phase-18 wave R54)

Provenance: M2 phase-18 wave **R54**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` —
`/FIFA96.EXE`, `watcom:LE:32:default`, 2706 functions, `.image` overlay
present). Every xref count, table byte, call byte and instruction below is a
fresh `get_xrefs_to` / `read_memory` / `disassemble_bytes` of **this session**
unless marked *cross-cited* (then the frozen FU doc is named). Engine source
read first-hand (`include/fifa96_engine/fifa96_match_handlers.h`,
`src/fifa96_engine/fifa96_match_handlers.c`, `fifa96_match_run.c`,
`fifa96_match_act.c`, `src/fifa96_loader/fifa96_action_handlers.c`,
`tests/test_engine_m2.c`). **No writes**: no rename, comment, label, function,
script or project save; `/FIFA96.EXE` untouched; no git commands. The alias
traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`).

Scope (R54 brief): (1) re-verify the count machinery first-hand (the two native
tables `0x1106E0`/`0x110794`, the `0x7D9A4` installer xref count, the act table
`0x1107EC`); (2) state the post-phase-18 arithmetic; (3) rank the remaining legs
by honest reachability; (4) **compile the per-producer tape-reachability
register** (the case-by-case posture's decision input — for each producer
cluster, is its live wiring render-visible on the M2 tape, with evidence).

This audit is contemporaneous with the phase-18 wave (R47–R53 + FU-199…FU-205
are being written in parallel); the register below is the **decision input**, not
a landing claim.

---

## 1. Fresh census machinery — the "N/80 wired" claim

The engine census counts non-NULL `fn` rows of the two native dispatch tables
(`include/fifa96_engine/fifa96_match_handlers.h:34..39`: action
`FIFA96_MATCH_ACTION_ROWS 45`, phase `FIFA96_MATCH_PHASE_ROWS 35` — 80 rows;
`FIFA96_MATCH_PHASE_INT3_SLOT 0x16`). Fresh native re-verification this session:

* **Action table `0x1106E0`** — fresh `read_memory` (180 B = 45 dwords) returns
  the full table, rows `0x00..0x2C`, ending at `0x110794` exactly (45 × 4 =
  `0xB4`), where the phase table begins (contiguity first-hand). Decoded dwords:
  `00:07DB10 01:07DBC0 02:07DFCC 03:07E1A4 04:07E7C8 05:07F194 06:0801B4
  07:0814B0 08:081068 09:080A00 0A:081738 0B:081908 0C:081C90 0D:08251C
  0E:082710 0F:082AD0 10:0855F0 11:085DE4 12:083D68 13:084B00 14:08784C
  15:087CD0 16:084630 17:084730 18:0849B0 19:0746E4 1A:07662C 1B:076D28
  1C:077728 1D:074EB0 1E:07550C 1F:076380 20:084EEC 21:085214 22:08539C
  23:082F84 24:086510 25:0880CC 26:0866F4 27:086820 28:0870E8 29:0874E4
  2A:086A34 2B:087738 2C:084598` — **byte-identical to FU-182 §1 / FU-190 §1 /
  FU-198 §1**.
* **Phase table `0x110794`** — fresh `read_memory` (140 B = 35 dwords), rows
  `0x00..0x22`, ending at `0x110820` (35 × 4 = `0x8C`). Fresh values re-confirm
  FU-177 §3 / FU-190 §1 / FU-198 §1 **byte-for-byte**:
  `[0]=06DE34 [1]=06E1D0 [2]=06DCC8 [3]=06DE44 [4]=06DE44 [5]=06E05C [6]=06DD9C
  [7]=06DE44 [8]=06DD6C [9]=06DD6C [0A]=06DE34 [0B]=06DE34 [0C]=06DF4C
  [0D]=06DE34 [0E]=06DE34 [0F]=06DE34 [10]=06E004 [11]=06E1C8 [12]=06E1D0
  [13]=06E244 [14]=06E244 [15]=06DCC8 [16]=00000000` (the INT3 slot)
  `[17]=088DC8 [18]=08922C [19]=089FA4 [1A]=089620 [1B]=0890EC [1C]=089110
  [1D]=089868 [1E]=08A798 [1F]=088F4C [20]=08B688 [21]=08B874 [22]=08B900`.
* **Installer `0x7D9A4`** — fresh `get_xrefs_to` = **77** call sites (the
  byte-for-byte same set as FU-174 §1 / FU-182 §1 / FU-190 §1 / FU-198 §1).
  Fresh `disassemble_bytes 0x7DA60` (80 B) confirms the dispatch:
  `0x7DA74 SHL EAX,0x2; 0x7DA77 ADD EAX,0x1106E0; 0x7DA7C MOV EAX,[EAX]`, the
  stage store `0x7DA8F MOV [ESI+0x18],EAX`, the code store `0x7DA67 MOV
  [ESI+0x91],AL`, and the invoke-now `0x7DAA3 TEST DI,DI; 0x7DAA8 MOV EAX,ESI;
  0x7DAAA CALL [ESI+0x18]`.
* **Action-table `0x1106E0` reader** — fresh `get_xrefs_to` = **2**, both in
  `FUN_0007D9A4` (`0x7DA77`/`0x7DA7C`): the installer is the sole native reader.
* **Phase-table `0x110794` reader** — fresh `get_xrefs_to` = **2**, both in
  `FUN_0006D920` (`0x6D9B3`/`0x6D9B8`): the per-record stager is the sole
  reader. Fresh `disassemble_bytes 0x6D9A0` (40 B) re-confirms the tail:
  `0x6D9A8 MOV EAX,[0x157A4A]; 0x6D9AD SAR EAX,0x18; 0x6D9B0 SHL EAX,0x2;
  0x6D9B3 ADD EAX,0x110794; 0x6D9B8 MOV EAX,[EAX]; 0x6D9BA MOV [EDX+0x1C],EAX`.
* **Act table `0x1107EC`** — fresh `read_memory` (72 B) returns
  `0,0x88DC8,0x8922C,0x89FA4,0x89620,0x890EC,0x89110,0x89868,0x8A798,0x88F4C,
  0x8B688,0x8B874,0x8B900` then unrelated data `0x006007FF,0x0007CE38,
  0x004007FF,0x0007CF20,0x008007FF` — byte-identical to FU-175 §1 / FU-190 §1 /
  FU-198 §1. The base is exactly `0x110794 + 0x16*4`, so **act `a` → phase
  `0x16+a`**. Fresh `get_xrefs_to 0x1107EC` = **2**, both `FUN_000888FC`
  (`0x88913`/`0x88920`). Fresh `disassemble_bytes 0x888FC` (80 B) re-confirms the
  dispatcher: `0x88905 CMP AX,CX` (vs the latch `[0x158828]`) → `0x8890A MOV
  [0x158828],AL` → `0x8890F CWDE; 0x88910 SHL EAX,0x2; 0x88913 ADD
  EAX,0x1107EC` → `0x88920 MOV EAX,[EAX]` → `0x88929 MOV [0x158808],EAX` →
  the `0x88937 CALL dword[0x158808]` (BX-gated invoke).
* **The per-record walk `FUN_0008D098`** — fresh `disassemble_bytes 0x8D0F0`
  (64 B): `0x8D0F7 CMP byte[ECX+0x9A],0; JNZ 0x8D123` (skip inactive) →
  `0x8D107 CALL 0x6D920` (stage `[rec+0x1C]`) → `0x8D110 CALL dword[ECX+0x1C]`
  (EAX=rec / EDX=ESI=`EBP+0x4D`) → `0x8D11E CALL 0x8DCD4` (the distance triple).

**Audit verdict: the native count machinery is provable as stated and unchanged
since FU-190/FU-198.** No table byte, xref count or dispatch instruction moved.

### 1.1 Engine-side count (fresh first-hand source read)

`src/fifa96_engine/fifa96_match_handlers.c:4964..5126` (the two tables,
first-hand this session): the action table has **31 non-NULL `fn`s**
(`00 01 02 04 05 06 07 08 09 0B 0C 0E 0F 10 11 12 13 14 16 18 19 1B 1C 1D 1E 1F
21 23 26 28 2A`) and **14 NULL** (`03 0A 0D 15 17 1A 20 22 24 25 27 29 2B 2C`);
the phase table has **17 non-NULL** (`00 02 03 04 05 06 07 08 09 0A 0C 0F 15 18
19 1F 21`) and **18 NULL** (`01 0B 0D 0E 10 11 12 13 14 16 17 1A 1B 1C 1D 1E 20
22`, incl. the INT3 slot). Dispatch arithmetic: **48 OK + 31 UNSUP (14 action +
17 phase) + 1 NOTF (phase `0x16`) = 80**. `tests/test_engine_m2.c:2839..2844`
pins the same shape first-hand: `assert(action_wired == 31u && phase_wired ==
17u)`.

**Audit verdict: the `48/80` claim is provable as stated.** The README title
`60%` = `floor(100 × 48/80) = floor(60.0) = 60`; ENGINE.md §"Action dispatch" and
its census line both say `48/80` (`31 UNSUP / 48 OK / 1 NOTF`, sum 80); the
phase-18 plan §"Global Constraints" and the SDD ledger baseline both say
`48/80 = 60% (31 action + 17 phase)`. All four agree.

---

## 2. The verified current census (48/80)

| | count |
|---|---|
| action rows wired (non-NULL `fn`) | **31** |
| action rows UNSUP (NULL `fn`) | 14 |
| phase rows wired | **17** |
| phase rows UNSUP (NULL, ex-`0x16`) | 17 |
| phase `0x16` (INT3) | 1 NOTF |
| **wired / total** | **48 / 80 → 60%** |

Dispatch: `48 OK / 31 UNSUP / 1 NOTF` (sum 80; action 45 = 31 OK + 14 UNSUP,
phase 35 = 17 OK + 17 UNSUP + 1 NOTF).

The 17 NULL phase rows (the phase-side movers, ex-INT3):
`01 0B 0D 0E 10 11 12 13 14 17 1A 1B 1C 1D 1E 20 22`.
The 14 NULL action rows (the action-side movers):
`03 0A 0D 15 17 1A 20 22 24 25 27 29 2B 2C`.

---

## 3. The post-phase-18 arithmetic

Phase-18 (`docs/superpowers/plans/2026-10-12-fifa96-m2-phase18-producer-spine.md`)
moves the phase/act cohort from *dispatched-but-gated / producer-leg'd* to
**producer-live** via three producer clusters: (a) the formation/line-up setup,
(b) the situation-machine live wiring, (c) the act-tail producers. Per the
plan's **no hand-set number** rule, the exact target is whatever the T1–T6 gates
prove.

**The honest envelope.** The baseline is **48** (31 action + 17 phase). The
`floor(100 × wired/80)`:

| scenario | wired | % |
|---|---|---|
| current (verified §1) | **48** | **60%** |
| + every reachable NULL phase row (§4 Tier 1–3) | ≤ 60 | ≤ 75% |
| + the action row `03` (R3-gated) | ≤ 61 | ≤ 76% |
| FU-198 §3 phase-cohort ceiling (`32 action + 34 phase`, `0x1B` never) | **66** | **82%** |

`floor(100 × 48/80) = 60`; `floor(100 × 60/80) = 75`; `floor(100 × 61/80) = 76`;
`floor(100 × 66/80) = 82`.

**Key structural fact for the reviewers.** The FU-198 §3 ceiling `66` counts
exactly **one** action mover (row `03`) + **all 34** non-INT3 phase rows
(`32 + 34`). **Every action-side target of the phase-18 plan**
(`0x17`/`0x1A`/`0x1C`/`0x20`/`0x22`/`0x0D`/`0x24`, plus `0x25`) lies **outside**
that ceiling — so a phase-18 landing that wires any of those action rows lifts
the ceiling **above 66**, and `66/80 = 82%` must be read as FU-198's
**phase-cohort** envelope, not a whole-table maximum. This slice does not
assert a new ceiling: the exact number is whatever T1–T6 prove.

**The honest phase-cohort blockers (§4–§5).** Of the 17 NULL phase rows, seven
cannot honestly move the census without new machinery:

* `0x16` — the native zero/INT3 slot (`0x110794[0x16] = 0`, fresh §1); NOTF by
  design, never counts.
* `0x1B` — no invoker: a fresh 11-xref `FUN_000888FC` census finds no `EAX=5`
  site and no `FUN_000740A0` phase-`0x1B` writer (FU195-L1, owner FU175-L2).
* `0x01`/`0x11`/`0x12` — double-wire-excluded (R10): the formation placement is
  already realized by `fifa96_match_entities_seed_formation`; wiring the phase
  row would double-run it (FU192-L6/L7).
* `0x13`/`0x14` — need the unported `FUN_0008D098` case-0x13/0x14 PRNG scatter
  (the `0x10F364/0x10F368` producer, FU192-L2).
* `0x0D` — its sole writer is situation 7 with **no live dispatcher** (the
  phase-17 final-review C1 correction; owner FU-146).

So the honest **phase-cohort reachable set** is at most
`{0B,0E,10,17,1A,1C,1D,1E,20,22}` (10 rows; `0B`/`0E`/`0x10` are written only
from inside the act-tail bodies that are themselves producers, so they close
only with Tier-3). With action row `03` the honest **whole-table** reach is
≤ `48 + 10 + 1 = 59` before any new action-row wiring, and the plan's
action-side targets are the only path above that.

---

## 4. Ranked remaining legs by honest reachability

Ranked by **(producer statically present?)** × **(engine reaches it without new
machinery?)** × **(reachability cost)**. Cross-cited to the owning FU slice;
the tape column is §5's verdict.

**Tier 1 — producer present and gated on ONE un-gate (least machinery).**

| # | row | producer (fresh / cross-cited) | un-gate | tape |
|---|---|---|---|---|
| 1 | phase **`0x20`** + action **`0x24`** | situation `0` → act `0xA` → phase `0x20` (the T2 `fifa96_match_run_period_setup`, wired at `begin` — `match_run.c:2244`); the phase body installs code `0x24` (`match_run.c:5624`) | `period_setup_live` (OL-13/T3) + the phase-`0x20`/row-`0x24` slots | **VISIBLE** (§5 PR-2) |
| 2 | phase **`0x1D`** + action **`0x20`** | situation `8` → act `7` → phase `0x1D` (the coupled row-`0x20` installer `0x899A2`, FU189-L1) | the extra-time flag (OL-85) + the phase-`0x1D` slot | dormant (§5 PR-7) |
| 3 | phase **`0x22`** | act `0xC` extra-time SET (`0x8B900`) | the extra-time flag `[0x157AC0]` (OL-85, FU195-L2) | dormant (§5 PR-3) |

**Tier 2 — producer present but blocked by a double-run or a tape-visible
derivation.**

| # | row | producer | blocker |
|---|---|---|---|
| 4 | phase **`0x17`** | sit-1 arm `0x8ABCF` / act-1 entry `0x88DC8` | the engine's live act-1 is the ad-hoc `fifa96_match_run_kickoff_entry` (`match_run.c:2269`) + the `global_5882a` arm; wiring the row **double-runs the kickoff** (FU193-L1/L2) |
| 5 | phase **`0x1A`** | act-4 formation arm / `0x89620` | needs the desired-order matrices (T2) **and** the act-1 stage-1 hand-off (FU193-L3) |
| 6 | phase **`0x1C`** | act-6 pre-restart `0x89110` | no engine source dispatches act 6; the offside machine drives it → **double-run** (FU193-L4) |
| 7 | phase **`0x1E`** | act-8 `0x8A798` | the engine already models act-8 as a **compressed** stored-situation re-dispatch; registering the full sweep **de-compresses a tape-visible derivation** (FU194-L2) |
| 8 | action **`0x03`** | the live phase-2 `[rec+0x1C]` = `0x6DCC8` (OL-13) | wiring the row **moved M2 at transcribed frame 29** (R7); owner FU177-L1 |
| 9 | action **`0x0D`** | row-09 arm-1 `0x80F61` | row-09 arm 1 unported + its gates (`[0x15877C]`, `[rec+0x3D]`, `[rec+0x24]`) (FU178-L1) |
| 10 | action **`0x17`** | act-4 stage-0 `0x89740` | the sit-2..4 formation-order arm is gated dormant on the unported order matrix (T2); owner FU181-L1 — **T2/T4 conflict (R11: T2 owns the row)** |
| 11 | action **`0x1A`** | the `FUN_00077EAC` CPU ladder | the ladder + the `FUN_0008D8EC → FUN_000782D0` pre-pass are unported (FU186-L5) |

**Tier 3 — producer present only behind a phase-cohort body that itself is a
Tier-2 leg.**

| # | row | producer | blocker |
|---|---|---|---|
| 12 | phase **`0x10`** | `match_run_write_phase(mr, 0x10)` at `match_run.c:5811` | the writer sits **inside** the unwired phase-`0x1D` body (FA-16 T5/FU194-L9) |
| 13 | phase **`0x0B`** | `match_run_write_phase(mr, 0x0B)` at `match_run.c:5732` | the writer sits **inside** the unwired phase-`0x1A` body (FU192 §1) |
| 14 | phase **`0x0E`** | — | no live writer (the reach set excludes it, FU-191 §10) |
| 15 | action **`0x22`** | row-07 `0x816DF` invoke-now | structurally ungated: the OL-65 angle inputs are staged 0, so `out.opponent_invoke` never fires (FU185-L6) |

**Tier 4 — dead / no-move (cannot honestly move the census).**

* `0x0A`/`0x15` (action) — installer `0x7CDD8` has no xrefs; head mis-decoded
  stub (FU-137 §6, OL-14).
* `0x25` — no installer exists (bounded `MOV EDX/DX,0x25` scan, FU189-L2).
* `0x27`/`0x29`/`0x2C` — ported bodies, but the FU-142f census of all 77
  `0x7D9A4` call sites finds no invocation (FU-142f/OL-48).
* `0x2B` — dead entry: native `0x87738` is the shared row-29 epilogue RET
  (FU-142f/OL-15).
* phase `0x1B`/`0x16` — §3.

---

## 5. The per-producer tape-reachability register

This is the phase-18 case-by-case posture's **decision input** (plan §"Determinism"):
for each producer, does wiring it live move a presented frame on the M2 tape?

**The render-invisibility ground truth (fresh first-hand).** The M2 scene staging
`match_run_scene_stage` reads exactly **four** record fields — `pos_x/pos_y/pos_z`
(`e->pos_x/pos_y/pos_z`), `anim_id` (`e->anim_id`), `frame` (`e->frame`) and
`hidden` (`e->skip_9a`) — `fifa96_match_run.c:1602..1607`. Nothing else about a
record reaches the presented canvas (FU-191 §10 item 3, re-verified first-hand
this session). Therefore a producer is **tape-visible** iff its live wiring
changes a record's **position / anim_id / frame / hidden**, and **tape-dormant**
iff its only outputs are the target triple (`rec+0x4D/+0x51/+0x55`), the
distance triple (`+0x65/+0x67/+0x69`), or the metadata pointers
(`+0x8/+0xC/+0x10/+0x14/+0x1C`).

**The tape's reach (fresh `tests/test_engine_m2.c:40..97, 1345..1370`).** The
v17 tape (792 lines, `7b4b241a…`) drives spec §5: boot → intro → front-end →
**start match (runs `begin`)** → kickoff → move → kick → the forced 0x13/0x14
window → the band scenario → the natural goal → the post-goal hold → stop. It
**does** run `begin` and the per-granted-frame clock tail (`fifa96_match_act_tick`),
and it **does** force the live phase at m 1/m 21/m 41. It **never** stages a
foul, offside, extra-time period, or a code-9/code-0x25 record; `ref_machine ==
REF_NONE` (ENGINE.md).

| producer | live wiring | outputs | tape verdict | evidence |
|---|---|---|---|---|
| **PR-1** formation/line-up setup (Cluster 1, R47/T1) | writes `[rec+0xC/+0x10/+0x14]` metadata + the `0x14C360`/`0x157BD2` order matrices | pointers + matrices (neither is render-read) | **dormant**, *unless* T1 also wires action row `03`: the only render-relevant consumer of `[rec+0xC]` is row `03`'s rewrite, which **moved M2 at frame 29** | render reads §5 ground truth; row-03 move ENGINE.md phase-17 T1 / FU-198 §5.3; R4 (ENGINE.md T3) |
| **PR-2** situation machine live (`period_setup_live` at `begin`) (Cluster 2, R50/T3) | `begin` → `period_setup(mode 1)` → situation `0` → phase `0x11` + act `0xA` (`fifa96_action_phase_situations[0] = {0x11,0xA}`) → act `0xA` = phase `0x16+0xA = 0x20` | phase `0x11` = per-record walk (target writes, invisible); act `0xA` installs the phase-`0x20` body whose stage 0 installs code `0x24` → **row `0x24` stage 0 writes `pos_x = 0x9F0`** (render-read) | **VISIBLE** (`begin` runs on the tape; `pos_x` is render-read) — land gated or re-pin only if native-faithful | `match_run.c:2244` (the `begin` gate), `:2998` (sit 0), `fifa96_action_handlers.c:1129ff` (sit table), `fifa96_match_act.c:50,62` (act→phase), `match_run.c:5617..5624` (phase-`0x20` install), `fifa96_match_handlers.c:4512` (`pos_x = 0x9F0`), render `:1602..1607` |
| **PR-3** extra-time flag (OL-85, R50) | `[0x157AC0]`/`global_157ac2` live | flips act `0xC` (phase `0x22`) and the period-index-4/bit-8 situation `8` | **dormant** on the tape (extra_time is 0; the tape stages no extra-time period) | `match_run.c:3005,3019` (flag gates), `:3046` (`extra_time 0`), ENGINE.md OL-85 |
| **PR-4** act-1 sit-1 / row `0x17` (Cluster 3, R51/T4) | the sit-1 arm `0x8ABCF` → the phase-`0x17` body `0x88DC8` | runs `fifa96_match_run_kickoff_entry` (stage 0) | **VISIBLE** (double-run): the tape **runs the kickoff**; the engine already runs the ad-hoc `kickoff_entry` at `:2269` | `match_run.c:2269`; phase-`0x17` body `:5652..5663`; FU193-L1/L2 |
| **PR-5** act-6 contact / row `0x1C` (Cluster 3, R51/T4) | the contact/referee chain `0x89110` | the act-6 pre-restart timeline | **dormant** (no engine source dispatches act 6; the tape has `ref_machine == REF_NONE`, no foul/offside); **double-run risk** on the fixture offside path only | FU193-L4; ENGINE.md `ref_machine REF_NONE` |
| **PR-6** phase `0x20`/row `0x24` (situation 0/act `0xA`) | same chain as PR-2 | `pos_x = 0x9F0` | **VISIBLE** (same as PR-2) | see PR-2 |
| **PR-7** phase `0x1D`/row `0x20` (situation 8/act 7) | the coupled installer `0x899A2` inside `0x89868` arm 0 | installs action `0x20` (body `0x84EEC` unported) | **dormant** (needs the extra-time route; the tape stages none) | FU189-L1/FU194-L1/L8; `match_run.c:3005` gate |
| **PR-8** phase `0x1D` (act 7) | the `0x89868` timeline | installs row `0x19` + phase `0x10` | **dormant** (same extra-time gate) | FU194 §3 |
| **PR-9** phase `0x1E` (act 8) | the `0x8A798` restart sweep | the 22-record code-`0xC`→`0x19` sweep | **dormant, but risky**: the engine models act-8 compressed; registering the full sweep would de-compress a tape-visible derivation | FU194-L2 |
| **PR-10** action row `0x0D` (row-09 arm 1) | `0x80F61` | the row-`0x0D` sequence step | **dormant** (the tape carries no code-9 record; row 09's installers are live but never fire on it) | FU178-L1; FU-198 §4 |
| **PR-11** action row `0x1A` (keeper ladder) | `FUN_00077EAC` | the keeper-reposition body | **dormant** (ladder + pre-pass unported; nothing installs the row) | FU186-L5 |
| **PR-12** action row `0x22` (OL-65 gate) | row-07 `0x816DF` invoke-now | the sequence/press body | **dormant** (the gate is structurally closed: `out.opponent_invoke` never fires) | FU185-L6 |
| **PR-13** phase `0x02` / OL-13 metadata (T1) | the per-record walk dispatches phase `0x02` per `match_run_write_phase` | the `0x6DCC8` target write (target triple) | **dormant** (targets are render-invisible; the walk writes nothing while no placement body resolves a render field) | `fifa96_match_handlers.c:5060` (phase `0x02` evidence); FU-191 §10 |

**Register summary.** Of the three clusters, only **Cluster 2 (PR-2/PR-6)** is
tape-**visible at `begin`** — and only once the phase-`0x20` slot **and** the
row-`0x24` slot land (both currently NULL; the phase-`0x20` body alone installs
code `0x24` but a NULL row `0x24` slot cannot run its `pos_x` write). Cluster 1
(PR-1) is render-invisible **as long as action row `03` stays un-wired**. Cluster
3 (PR-4) is a **double-run of the kickoff** (visible); PR-5 is dormant. The plan's
R9/FU196-L8 ruling (`period_setup_live` stays 0) is therefore **correct and
necessary**: wiring it live before the tape verdict is a regression risk, not an
upgrade.

---

## 6. Numbered legs

1. **FU206-L1 — the phase-`0x20`+row-`0x24` coupling is the one true tape risk.**
   This slice proves the situation-0 → act-`0xA` → phase-`0x20` → code-`0x24` →
   `pos_x = 0x9F0` chain statically and that `pos_x` is render-read. It does
   **not** prove the chain fires end-to-end on the tape (it needs both the
   phase-`0x20` and row-`0x24` slots to land). T3/T5 must pin the tape verdict by
   `cmp` before any `period_setup_live` flip; treat any presented-frame move as a
   regression to investigate (AGENTS.md rule).
2. **FU206-L2 — the action-side ceiling is above FU-198's 66.** The FU-198 §3
   envelope counts only action row `03` + all non-INT3 phase rows. Any phase-18
   action-row landing (`0x17`/`0x1A`/`0x1C`/`0x20`/`0x22`/`0x24`/`0x0D`) raises
   the ceiling; this slice asserts no new number (no hand-set value).
3. **FU206-L3 — the 7 blocked phase rows.** `0x01/0x11/0x12` (double-wire,
   FU192-L6/L7), `0x13/0x14` (FU192-L2), `0x0D` (FU-146), `0x1B` (FU195-L1) and
   `0x16` (INT3) are the honest blockers. Do not count one without its producer.
4. **FU206-L4 — the row-`0x17` T2/T4 conflict (R11).** The ledger's R11 assigns
   row `0x17` to T2 (the act-4 install `0x89740`); T4 counts it only if T2 proves
   the arm dormant. This audit flags it a **double-count risk** (§4 #10/Tier 2);
   the census sum + the review are the catch.
5. **FU206-L5 — the reach set.** The engine-reach set for the per-record walk is
   the game-phase family `{0,1,2,3,4,5,6,7,8,9,0xA,0xC,0xF,0x11,0x13,0x14,0x15}`
   (FU-175 §7B / FU-195 §5 / ENGINE.md I1), excluding `0x0B/0x0D/0x0E/0x10`. Any
   claim that a per-record row is "live-written" must cite a writer inside that
   set; `0x0B`/`0x10` writers sit inside unwired act-tail bodies (Tier 3).

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` — confirms `/FIFA96.EXE`, `watcom:LE:32:default`.
* `read_memory` — `0x1106E0` (180 B), `0x110794` (140 B), `0x1107EC` (72 B).
* `get_xrefs_to` — `0x7D9A4` (77), `0x1106E0` (2), `0x110794` (2), `0x1107EC`
  (2).
* `disassemble_bytes` — `0x7DA60` (80 B, the installer dispatch), `0x6D9A0`
  (40 B, the phase stager tail), `0x888FC` (80 B, the act dispatcher),
  `0x8D0F0` (64 B, the per-record walk).

Engine/loader source read first-hand: `include/fifa96_engine/fifa96_match_handlers.h`
(34..39); `src/fifa96_engine/fifa96_match_handlers.c` (4964..5126, the two tables
+ the dispatch resolvers; 4479..4534, the row-`0x24` body); `src/fifa96_engine/fifa96_match_run.c`
(1602..1607, the render reads; 2225..2304, `begin`; 2694..2749, the situation
runner; 2908..2978, `set_piece`; 2989..3025, `period_setup`; 5602..5640, phase
`0x20`); `src/fifa96_engine/fifa96_match_act.c` (whole, 95 lines);
`src/fifa96_loader/fifa96_action_handlers.c` (1117..1165, `fifa96_action_phase_act`
+ the situation table); `tests/test_engine_m2.c` (40..97, 1345..1370, 2831..2900,
the tape + the census riders).

Docs cross-cited: FU-174, FU-175, FU-177, FU-178, FU-181, FU-182, FU-185, FU-186,
FU-188, FU-189, FU-190, FU-191, FU-192, FU-193, FU-194, FU-195, FU-196, FU-197,
FU-198; the phase-18 plan; the SDD ledger; AGENTS.md census rule. No writes
beyond this slice file and the workspace report.

---

## Addendum — T6 acceptance (phase-18 landing record)

This addendum records the phase-18 acceptance against this slice's audit
(T6/FU-206); the sections above are R54's contemporaneous recon and are kept
verbatim.

**Reconciled census.** The audit's `48/80` baseline held at the wave; the
phase-18 landings moved it to **`51/80 = 63%`**: action `31 → 32` (T3 row
`0x24`), phase `17 → 19` (T3 phase `0x20`, T4 phase `0x17`). Dispatch `48 OK /
31 UNSUP / 1 NOTF` → **`51 OK / 28 UNSUP / 1 NOTF`** (sum 80). The §1 machinery
(the two tables `0x1106E0`/`0x110794`, the `0x7D9A4` installer xref count, the
act table `0x1107EC`) is byte-identical to FU-190/FU-198 (re-verified first-hand
in the R54 session). README title + `docs/ENGINE.md` title + the dispatch
sum-to-80 line + `docs/PROGRESS.md` + `AGENTS.md:90` are consistent at `63%`.

**The §5 register, resolved.** PR-1 (Cluster 1) landed **dormant** as predicted
(the T1 bind is render-invisible; action row `03` stays NULL). PR-2/PR-6
(Cluster 2) landed **VISIBLE** — the earned T3 re-pin (`7b4b241a… → 441415fc…`).
PR-3 (extra-time) dormant/leg. PR-4 (Cluster 3, act-1) was **wrongly** assessed
"VISIBLE (double-run)" in §5: T4 proved the double-run is avoided by retiring the
derived call (the live act-1 route reads the same `fifa96_match_run_kickoff_entry`
through the phase-`0x17` body), and the registered body fires **past the scripted
window** → M2 byte-identical. PR-5 dormant/leg. PR-7..PR-13 dormant/leg.

**The §6 legs — owners at acceptance.** FU206-L1 (the phase-`0x20`/row-`0x24`
tape risk) is **closed by T3**: the chain fires live on a begun run and the tape
was `cmp`-pinned before the authorised re-pin. FU206-L2 (the action-side ceiling
> FU-198's 66) stands (the plan asserts no new number). FU206-L3 (the 7 blocked
phase rows) stands. FU206-L4 (the row-`0x17` T2/T4 double-count risk) resolved by
R11/T4 — the **PHASE** row `0x17` is the counted one; the **ACTION** row `0x17`
stays NULL (**FU-181/FU-201**). FU206-L5 (the per-record reach set) stands.

**Provenance (T6).** `tests/test_engine_m2.c` (the v18 label + `run_phase18_riders`
+ the tape `cmp`); `docs/ENGINE.md` (the phase-18 T6 bullet);
`docs/PROGRESS.md` (the phase-18 T6 section); `AGENTS.md:90`; the plan's landing
notes. No writes to `/FIFA96.EXE` or the ISO.



