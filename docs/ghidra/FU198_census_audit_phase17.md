# FU-198 — census/audit + phase cohort ranking + row-03 completion (phase-17 wave R46)

Provenance: M2 phase-17 wave **R46**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` —
`/FIFA96.EXE`, `watcom:LE:32:default`, 2706 functions, `.image` overlay
present). Every xref count, table byte, call byte and instruction below is a
fresh `get_xrefs_to` / `read_memory` / `disassemble_bytes` /
`disassemble_function` / `decompile_function` of **this session** unless marked
*cross-cited* (then the frozen FU doc is named). Engine source read first-hand
(`include/fifa96_engine/fifa96_match_handlers.h`,
`src/fifa96_engine/fifa96_match_handlers.c`, `fifa96_match_run.c`,
`fifa96_match_act.c`, `src/fifa96_loader/fifa96_action_handlers.c`).
**No writes**: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; no git commands. The alias traps are applied
(`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`).

Scope (R46 brief): (1) re-verify the count machinery first-hand (the two native
tables `0x1106E0`/`0x110794`, the `0x7D9A4` installer xref count, the act table
`0x1107EC`); (2) state the post-phase-17 arithmetic; (3) rank the phase cohort
by honest reachability; (4) derive the completion path for action row `03`
(`0x6DCC8` + the per-record `[rec+0x1C]` seam, FU190-L4).

This audit is contemporaneous with the phase-17 wave (R39–R45 + FU-191…FU-197
are being written in parallel); where a mover's outcome is a T1–T6 landing, the
§3/§4 status is **prospective**, not a claim.

---

## 1. Fresh census machinery — the "N/80 wired" claim

The engine census counts non-NULL `fn` rows of the two native dispatch tables
(`include/fifa96_engine/fifa96_match_handlers.h:33..38`: action
`FIFA96_MATCH_ACTION_ROWS 45`, phase `FIFA96_MATCH_PHASE_ROWS 35` — 80 rows;
`FIFA96_MATCH_PHASE_INT3_SLOT 0x16`). Fresh native re-verification this session:

* **Action table `0x1106E0`** — fresh `read_memory` (180 B = 45 dwords) returns
  the full table, rows `0x00..0x2C`, ending at `0x110794` exactly (45 × 4 =
  `0xB4`), where the phase table begins (contiguity first-hand). Decoded:
  `00:07DB10 01:07DBC0 02:07DFCC 03:07E1A4 04:07E7C8 05:07F194 06:0801B4
  07:0814B0 08:081068 09:080A00 0A:081738 0B:081908 0C:081C90 0D:08251C
  0E:082710 0F:082AD0 10:0855F0 11:085DE4 12:083D68 13:084B00 14:08784C
  15:087CD0 16:084630 17:084730 18:0849B0 19:0746E4 1A:07662C 1B:076D28
  1C:077728 1D:074EB0 1E:07550C 1F:076380 20:084EEC 21:085214 22:08539C
  23:082F84 24:086510 25:0880CC 26:0866F4 27:086820 28:0870E8 29:0874E4
  2A:086A34 2B:087738 2C:084598` — **byte-identical to FU-182 §1 / FU-190 §1**.
* **Phase table `0x110794`** — fresh `read_memory` (140 B = 35 dwords), rows
  `0x00..0x22`, ending at `0x110820` (35 × 4 = `0x8C`). Fresh values re-confirm
  FU-177 §3 / FU-190 §1 **byte-for-byte**:
  `[0]=06DE34 [1]=06E1D0 [2]=06DCC8 [3]=06DE44 [4]=06DE44 [5]=06E05C [6]=06DD9C
  [7]=06DE44 [8]=06DD6C [9]=06DD6C [0A]=06DE34 [0B]=06DE34 [0C]=06DF4C
  [0D]=06DE34 [0E]=06DE34 [0F]=06DE34 [10]=06E004 [11]=06E1C8 [12]=06E1D0
  [13]=06E244 [14]=06E244 [15]=06DCC8 [16]=00000000` (the INT3 slot)
  `[17]=088DC8 [18]=08922C [19]=089FA4 [1A]=089620 [1B]=0890EC [1C]=089110
  [1D]=089868 [1E]=08A798 [1F]=088F4C [20]=08B688 [21]=08B874 [22]=08B900`.
  **11 distinct per-record bodies** cover the 22 rows `0x00..0x15` (the shared
  map FU-190 §1 / the engine `fifa96_action_phase_rows[]` rely on):
  `0x6DE34` (rows 00/0A/0B/0D/0E/0F), `0x6DCC8` (02/15), `0x6E1D0` (01/12),
  `0x6DE44` (03/04/07), `0x6DD6C` (08/09), `0x6E05C` (05), `0x6DD9C` (06),
  `0x6DF4C` (0C), `0x6E004` (10), `0x6E1C8` (11), `0x6E244` (13/14).
* **Installer `0x7D9A4`** — fresh `get_xrefs_to` = **77** call sites (the
  byte-for-byte same set as FU-174 §1 / FU-182 §1 / FU-190 §1). Fresh
  `disassemble_function 0x7D9A4` (73 insns) confirms the dispatch:
  `0x7DA74 SHL EAX,0x2; 0x7DA77 ADD EAX,0x1106E0; 0x7DA7C MOV EAX,[EAX]`, the
  store `0x7DA8F MOV [ESI+0x18],EAX`, the code-3→0x19 inactive remap
  `0x7DA26..0x7DA3B` (`CMP byte[ESI+0x8D],0 → CMP EAX,3 → MOV word[ESP+4],0x19`),
  the code store `0x7DA67 MOV [ESI+0x91],AL`, and the invoke-now
  `0x7DAA3 TEST DI,DI; 0x7DAA8 MOV EAX,ESI; 0x7DAAA CALL [ESI+0x18]`.
* **Action-table `0x1106E0` reader** — fresh `get_xrefs_to` = **2**, both in
  `FUN_0007D9A4` (`0x7DA77`/`0x7DA7C`): the installer is the sole native reader.
* **Phase-table `0x110794` reader** — fresh `get_xrefs_to` = **2**, both in
  `FUN_0006D920` (`0x6D9B3`/`0x6D9B8`): the per-record stager is the sole reader.
  Fresh `disassemble_bytes 0x6D9A0` (32 B) re-confirms the tail:
  `0x6D9A8 MOV EAX,[0x157A4A]; 0x6D9AD SAR EAX,0x18; 0x6D9B0 SHL EAX,0x2;
  0x6D9B3 ADD EAX,0x110794; 0x6D9B8 MOV EAX,[EAX]; 0x6D9BA MOV [EDX+0x1C],EAX`.
* **Act table `0x1107EC`** — fresh `read_memory` (72 B) returns
  `0,0x88DC8,0x8922C,0x89FA4,0x89620,0x890EC,0x89110,0x89868,0x8A798,0x88F4C,
  0x8B688,0x8B874,0x8B900` then unrelated data `0x006007FF,0x0007CE38,
  0x004007FF,0x0007CF20,0x008007FF` — byte-identical to FU-175 §1 / FU-190 §1.
  The base is exactly `0x110794 + 0x16*4`, so **act `a` → phase `0x16+a`**.
  Fresh `get_xrefs_to 0x1107EC` = **2**, both `FUN_000888FC` (`0x88913`/
  `0x88920`). Fresh `disassemble_bytes 0x888FC` (64 B) re-confirms the
  dispatcher: `0x88905 CMP AX,CX` (vs the latch `[0x158828]`) → `0x8890A MOV
  [0x158828],AL` → `0x8890F CWDE; 0x88910 SHL EAX,0x2; 0x88913 ADD
  EAX,0x1107EC` → `0x88920 MOV EAX,[EAX]` → `0x88929 MOV [0x158808],EAX` →
  `0x88937 CALL dword[0x158808]`.
* **The per-record walk `FUN_0008D098`** — fresh `get_xrefs_to 0x8D098` = **2**,
  both in `FUN_000740A0` (`0x740C8`/`0x740DB`, the phase writer). Fresh
  `disassemble_bytes 0x8D0F0` (64 B) re-confirms the loop: `0x8D0F7 CMP
  byte[ECX+0x9A],0; JNZ` (skip inactive) → `0x8D107 CALL 0x6D920` (stage
  `[rec+0x1C]`) → `0x8D110 CALL dword[ECX+0x1C]` → the `0xB2` stride at
  `0x8D131/0x8D137/0x8D13D` and the guard `0x8D159..0x8D166` (`MOV EAX,[ESP+0xFA];
  SAR EAX,0x10; CMP EAX,0xB; JL` — 11 records `0..10`).
* **The phase writer `0x740A0`** — fresh `get_xrefs_to` = **27** call sites
  (unchanged since FU-165 §1/§3 / FU-175 §4). `FUN_000740A0` is the bridge: an
  act handler that writes a phase calls this setter, which runs the
  `FUN_0008D098` walk once per team.
* **Situation 0xB → phase 2** — fresh `disassemble_bytes 0x8AEF0` (32 B):
  `0x8AEF6 MOV EDX,[ESP-2]; 0x8AEFA MOV EAX,0x2; 0x8AEFF SAR EDX,0x10;
  0x8AF02 CALL 0x740A0` — the sole phase-2 writer (FU-175 §4/§7, fresh bytes
  agree).

Engine-side count (source `fifa96_match_handlers.c:4547..4679`, first-hand): the
action table has **31 non-NULL `fn`s** (`00 01 02 04 05 06 07 08 09 0B 0C 0E 0F
10 11 12 13 14 16 18 19 1B 1C 1D 1E 1F 21 23 26 28 2A`) and **14 NULL**
(`03 0A 0D 15 17 1A 20 22 24 25 27 29 2B 2C`); the phase table has **3 non-NULL**
(`0x18`, `0x19`, `0x1F`) and **31 NULL** (incl. the INT3 slot). Dispatch
arithmetic: **34 OK + 45 UNSUP (14 action + 31 phase) + 1 NOTF (phase `0x16`) =
80**. The README title `42%` = `floor(100 × 34/80) = floor(42.5) = 42`.

**Audit verdict: the `34/80` claim is provable as stated.** ENGINE.md §"Action
dispatch" says `34/80 rows wired` (31 action + phase `0x18`/`0x19`/`0x1F`) and
README title `42%`; both match the fresh source table. FU-190 §9's `45 UNSUP /
34 OK / 1 NOTF` is this same state.

---

## 2. The verified current census (34/80)

| | count |
|---|---|
| action rows wired (non-NULL `fn`) | **31** |
| action rows UNSUP (NULL `fn`) | 14 |
| phase rows wired | **3** (`0x18`, `0x19`, `0x1F`) |
| phase rows UNSUP (NULL, ex-`0x16`) | 31 |
| phase `0x16` (INT3) | 1 NOTF |
| **wired / total** | **34 / 80 → 42%** |

Dispatch: `34 OK / 45 UNSUP / 1 NOTF` (sum 80; action 45 = 31 OK + 14 UNSUP,
phase 35 = 3 OK + 31 UNSUP + 1 NOTF).

---

## 3. The post-phase-17 arithmetic

Phase-17 (`docs/superpowers/plans/2026-10-12-fifa96-m2-phase17-phase-act-seam.md`)
targets **one action row** (`03`, T1) and a **producer-live phase cohort**:

* **T1/R39 (the per-record seam + row 03):** lands the native walk
  (`FUN_0008D098`) and ports `0x6DCC8` (phase row `0x02`; shares phase `0x15`),
  then wires action row `03`.
* **T2/R44+R45 (producer machine):** situations `0/8/0xB` + the act-4 sit-2..4
  formation-order arm + the `[rec+0xC]`/OL-13 source.
* **T3/R40 (per-record bodies `0x00–0x15`):** the remaining 10 distinct
  `0x6Dxxx` bodies (T1 owns `0x6DCC8`).
* **T4/R41:** act-tail `0x17`/`0x1A`/`0x1C`. **T5/R42:** `0x1D`/`0x1E`/`0x20`.
  **T6/R43:** `0x21`/`0x22` + the act-5/`0x1B` verdict.

**Action arithmetic.** Row `03` is the only action target. Per controller Ruling
R3 (§plan), row `03` counts **only if** its `0x6DCC8` target rewrite is pinned
inert on the tape's state; if the rewrite provably fires, the row is a **leg**
(uncounted, owner FU177-L1/OL-13). So:

| scenario | action | phase | wired | % |
|---|---|---|---|---|
| current (verified) | 31 | 3 | **34** | **42%** |
| T1, row 03 counted (R3 holds) + phase `0x02` | 32 | 4 | **36** | **45%** |
| T1, row 03 leg'd (R3 fails) + phase `0x02` only | 31 | 4 | **35** | **43%** |

**Phase cohort envelope.** The phase table can add at most **31** rows (35 − 3
counted − 1 INT3): 22 per-record (`0x00..0x15`, 11 distinct bodies) + 8
act-tail (`0x17 0x1A 0x1C 0x1D 0x1E 0x20 0x21 0x22`; `0x1B` blocked; `0x18
0x19 0x1F` already counted) + `0x1B` (blocked). With `N` = the rows the tasks
prove producer-live, the post-phase-17 title is `floor(100 × (32+N_action? + 3
+ N_phase)/80)`. The **honest envelope**:

| scenario | wired | % |
|---|---|---|
| current (verified) | 34 | **42%** |
| T1 only (row 03 + phase 0x02), R3 holds | 36 | 45% |
| + all per-record rows producer-live (T3, N=22) | 58 | 72% |
| + act-live act-tail (T4/T6: 0x17/0x1C/0x21/0x22, N=4) | 62 | 77% |
| + situation-gated act-tail (T4 0x1A + T5: 0x1D/0x1E/0x20, N=4) | 66 | 82% |
| absolute ceiling (every cohort row live; `0x1B` never) | 66 | **82%** |

`floor(100 × 36/80) = 45`; `floor(100 × 58/80) = 72`; `floor(100 × 62/80) = 77`;
`floor(100 × 66/80) = 82`. **The exact post-phase-17 title is whatever the
T1–T6 gates prove** (AGENTS.md rule; no hand-set number). The per-record rows
and the situation-gated act rows are contingent on T2's producer machine; if T2
lands no producer, the cohort stays at the act-live act-tail (T4/T6) only.

**Erratum (this audit).** The plan's §"Global Constraints" line 18 says
"action non-NULL `31` (unchanged)" while its own R5/T1 say row `03` is counted
→ action `32`. The two cannot both hold. Per the plan's R5 ("T1 `34→35` (row
03, per R3)") and T1's *Produces* ("action row `03` dispatches it (counted)"),
line 18's "(unchanged)" is the stale text; the action count moves `31→32` iff
R3 holds. Flagged for the controller (§6 leg FU198-L5).

---

## 4. Ranked phase cohort by honest reachability

Ranked by **(act/situation producer live in the engine _today_?)** ×
(**body coverage**) × (**reachability cost**). Cross-cited where the producer
claim is FU-175 §7's, fresh where marked.

**Tier A — producer live today (act machine already running).**

| # | phase | act | handler | producer (fresh / cross-cited) | task |
|---|---|---|---|---|---|
| A1 | **`0x02`** | — (per-record) | `0x6DCC8` | situation `0xB` → phase 2 (fresh `0x8AEF6`; the sole phase-2 writer) + the forced-decision walk (`handlers.c:2181`) | **T1** |
| A2 | **`0x17`** | 1 | `0x88DC8` | sit 1 (`0x8ABAB`, fresh set-piece call `match_run.c:5444`) + the derived kickoff/act-1 path (*cross-cited* FU-175 §7 rank 2) | **T4** |
| A3 | **`0x1C`** | 6 | `0x89110` | the contact/referee chain (`FUN_0008A43C` `0x8A789`; the engine's dispatched-act set is `1/2/3/6/9/0xB/0xC`, ENGINE.md:409) | **T4** |
| A4 | **`0x21`** | 0xB | `0x8B874` | the period-end chooser (`0x8BAE7`, *cross-cited* FU-175 §7 rank 4 / FU-143 §9) | **T6** |
| A5 | **`0x22`** | 0xC | `0x8B900` | the period-end chooser (extra-time set `0x8BACF`, *cross-cited* FU-175 §4) | **T6** |

**Tier B — producer needs T2's situation machine.**

| # | phase | act | handler | producer | task |
|---|---|---|---|---|---|
| B1 | **`0x1A`** | 4 | `0x89620` | the sit-3 direct arm `0x8AB08` (`BX=1`) + act-1 stage-1 hand-off (*cross-cited* FU-175 §7 rank 5); the engine's sit-3 table row carries **no act** (`fifa96_action_phase_situations[3] = {4, NONE, …}`), so the arm is the unported FU-149 L12 machinery | **T2→T4** |
| B2 | **`0x1D`** | 7 | `0x89868` | sit 8 (`0x8AEB3`); `fifa96_action_phase_situations[8] = {NONE, 7, …}` exists but **no engine source/test dispatches situation 8** (FU-188 §6 / FU-189 §1) | **T2→T5** |
| B3 | **`0x1E`** | 8 | `0x8A798` | the `BX!=0` fallback `0x8AAA3` + act-8's re-dispatch (engine `match_run.c:2810..2828`, derived) | **T2→T5** |
| B4 | **`0x20`** | 0xA | `0x8B688` | sit 0 (`0x8AB82`); `fifa96_action_phase_situations[0] = {0x11, 0xA, …}` exists but **no engine source/test dispatches situation 0** (FU-188 §1) | **T2→T5** |

**Tier C — per-record family `0x00–0x15` (T1 walk + T3 bodies).** Once T1 lands
the walk, a per-record row is honestly reachable iff the current phase byte
equals it while the walk runs. The phase bytes with a live/known writer are
`2` (situation `0xB`, fresh) and the engine's `match_run_phase_arm` set
(`match_run.c:2456..2569`: phases `3–9`/`0x0D` install codes per FU-182 §5);
the remaining placement phases (`0/1/0xA/0xB/0xC/0xF/0x10/0x11/0x13/0x14/0x15`,
per FU-175 §7B's `FUN_000740A0` argument set) need their producers (T2/T3).
Ranking within the family is **R40/FU-192's** verdict; this audit holds that
the walk makes the family *mechanically* reachable and the producers gate the
*count*. The 11 distinct bodies mean T1 (`0x6DCC8`) + T3 (10 bodies) cover all
22 rows.

**Blocked / no-move.**

* **`0x1B` / act 5** — no static `CALL 0x888FC` passes `EAX=5` (FU-175 §7 rank 9,
  FU175-L2; the fresh `0x1107EC[5] = 0x890EC` reads but is never installed), and
  a pure-reset body with no invoker cannot move the census. T6's verdict.
* **`0x16`** — the native zero/INT3 slot (`0x110794[0x16] = 0`, fresh §1);
  NOTF by design (never counts).
* **`0x18`/`0x19`/`0x1F`** — already wired/counted (§1); not cohort movers.

---

## 5. Row-03 completion path (`0x6DCC8` + the per-record `[rec+0x1C]` seam)

Row `03` (`fifa96_match_handlers.c:4554`, NULL) is **install-live** today — the
row-04 body installs code 3 (`0x7E872`) or the inactive remap 0x19
(`0x7E879`), reached through the forced-decision walk (`fifa96_match_run.c:960`;
FU-177 §1, fresh cross-cite).

### 5.1 The `0x6DCC8` handler (fresh decompile this session)

`decompile_function 0x6DCC8` (`void FUN_0006dcc8(int *param_1,int *param_2,short
param_3)`) reproduces FU-190 §6.2 exactly:

```c
if (param_3 < 0) iVar5 = *(int*)(&DAT_001577de + (uchar)(*param_1 + 0x826)*4) >> 0x10;
else             iVar5 = (int)param_3;
pcVar1 = (char*)(param_1[3] + iVar5*2);          /* param_1[3] = [rec+0xC] */
sVar6 = (short)*pcVar1 * 0x26;  sVar4 = (short)pcVar1[1] * 0x21;
if (*(char*)(*param_1 + 0x826) != 0) { sVar6 = -sVar6; sVar4 = -sVar4; }
param_2[1] = 0;  *param_2 = (int)sVar6;  param_2[2] = (int)sVar4;
```

It reads `[rec+0xC]` (the OL-13 metadata pointer), indexes a per-record
direction pair, scales by `±0x26`/`±0x21` (side-flipped by `[team+0x826]`), and
**overwrites the row-03 target block `rec+0x4D/+0x51/+0x55`** (`param_2` = the
`EDX = &rec[0x4D]` argument) *before* the phase-2 clamp runs. This is the real
hot-path side effect FU-177 §3/§7 warns about.

Fresh `get_xrefs_to 0x6DCC8` = **5**: **2 DATA** refs from the phase table
(`0x11079C` = phase `[2]`, `0x1107E8` = phase `[0x15]`) and **3 internal
`CALL`s** (`0x6DD61`, `0x6DD91`, `0x6DF37` in the `0x6DD6C`/shared bodies). **No
action-table reference** — `0x6DCC8` is reached only per-record.

### 5.2 The `[rec+0x1C]` seam (fresh bytes this session)

Row 03 has two live `CALL dword ptr [ESI+0x1C]` sites, both `EAX=rec`,
`EDX=&rec[0x4D]`, `EBX=-1`:

* `0x7E250 MOV EAX,ESI; 0x7E252 CALL dword[ESI+0x1C]` (the slot branch A,
  `0x7E230..0x7E25E`, phase 6/0x15/`[0x157A49]>>24==1`). Fresh
  `disassemble_bytes 0x7E244` (32 B).
* `0x7E2DB MOV EAX,ESI; 0x7E2DD CALL dword[ESI+0x1C]` (the **idle core**,
  `0x7E2D3..0x7E308`). Fresh `disassemble_bytes 0x7E2D0` (40 B).

`[rec+0x1C]` is staged by `FUN_0006D920` (`0x6D9BA`) from the phase table; on
the forced-decision walk the record is in phase 2, so `0x7E2DD` invokes
`0x110794[2] = 0x6DCC8`. This is **exactly the same seam** the native walk
`FUN_0008D098` uses (`0x8D110 CALL [ECX+0x1C]` after `0x8D107 CALL 0x6D920`,
fresh §1) — row 03 is a **second live caller shape** of the per-record seam.

Fresh `get_xrefs_to 0x7E1A4` (row-03 handler) = **1**, from `0x1106EC`
(action-table slot `0x03`); the row is reachable only via `fifa96_match_dispatch_action`.

### 5.3 Completion rec (two dependencies; owner T1)

1. **Port `0x6DCC8`** (and its `[rec+0xC]` source — the OL-13 metadata pointer)
   as the phase-table row-`0x02` body; phase `0x15` shares it. **Owner FU177-L1 /
   OL-13; T1 scope.**
2. **Land the per-record `[rec+0x1C]` seam** — the native walk (or an
   equivalent record-threaded dispatch) so row 03's `CALL [rec+0x1C]` reaches
   `0x6DCC8`. **Owner FU175-L1 / R39-T1.**
3. **Wire `fifa96_match_action_03`** to its ported body over the FU-138 helpers
   (hold/`clamp_placement`/`placement_counter`/`phase1_clamp` + the
   already-ported `fifa96_match_entities_camera_place` for `0x79F3C`, FU-177 §4
   errata).

**Standalone wiring without `0x6DCC8` is a fake** (drop the hot-path target
rewrite — FU-177 §7 T3-R2, rejected). **R3 gate:** T1 counts row `03` only if
the `0x6DCC8` rewrite is pinned inert on the tape's state; else row `03` stays
`NULL`/uncounted (leg FU177-L1). The phase row `0x02` is the same T1 unit and
moves independently of the R3 gate (the phase table counts on the walk + the
sit-`0xB` producer, both of which T1 lands).

**Census:** `+1` action (row 03, R3-gated) `+1` phase (row `0x02`) → `36/80 =
45%`; if R3 fails, `+1` phase only → `35/80 = 43%`.

---

## 6. Numbered legs

1. **FU198-L1 — the per-record body set is R40's call.** This audit verifies the
   phase table, the 11 distinct bodies and the walk mechanics only. Which of
   `0x00–0x15` become producer-live is R40/FU-192's verdict; do not count a
   per-record row without its producer.
2. **FU198-L2 — the act-live act-tail set.** Tier A2–A5 rest on FU-175 §7's
   producers (sit 1; the contact act 6; the period-end 0xB/0xC) and the
   ENGINE.md `1/2/3/6/9/0xB/0xC` dispatched set. This slice directly verified
   only sit `0xB`→phase 2 and the act-table dispatcher; re-verify each act's
   engine producer (R41/R43) before counting `0x17`/`0x1C`/`0x21`/`0x22`.
3. **FU198-L3 — the situation-gated act-tail.** `0x1A`/`0x1D`/`0x1E`/`0x20` are
   gated by T2's situations `3`(arm)/`8`/`0`; the engine's situation table has
   the rows (`fifa96_action_phase_situations[3/8/0]`) but no caller dispatches
   situations 8/0 today (fresh source read; FU-188 §1/§6). R44/R45 own the
   producer proof.
4. **FU198-L4 — `0x1B`/act 5.** No invoker (FU-175 §7 rank 9, FU175-L2);
   T6/R43's bounded re-census decides leg vs reachable. Do not invent one.
5. **FU198-L5 — the plan's action-count text conflict.** The phase-17 plan's
   §"Global Constraints" line 18 ("action non-NULL `31` (unchanged)") conflicts
   with R5/T1 (row `03` counted → `32`). The controller must reconcile; this
   audit takes R5/T1 as the authority (line 18 stale).
6. **FU198-L6 — row-03 remainder.** Even after `0x6DCC8` + the seam, the
   phase-7 `0x158747` wall scan (FU177-L2), `FUN_00079B58` (FU177-L3), the
   phase-0x13/0x14 arm (FU177-L4), the `[rec+0x1C]` `EBX=-1` third argument
   (FU177-L5) and the phase-8/0xC tails (FU177-L6) are legs. They do not gate
   the phase-2 behaviour (the count-relevant path).

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` — confirms `/FIFA96.EXE`, `watcom:LE:32:default`.
* `read_memory` — `0x1106E0` (180 B), `0x110794` (140 B), `0x1107EC` (72 B).
* `get_xrefs_to` — `0x7D9A4` (77), `0x1106E0` (2), `0x110794` (2), `0x1107EC`
  (2), `0x6DCC8` (5), `0x7E1A4` (1), `0x8D098` (2), `0x740A0` (27).
* `disassemble_function` — `0x7D9A4` (73 insns).
* `disassemble_bytes` — `0x6D9A0` (32 B), `0x8D0F0` (64 B), `0x7E244` (32 B),
  `0x7E2D0` (40 B), `0x8AEF0` (32 B), `0x8D150` (32 B), `0x888FC` (64 B).
* `decompile_function` — `0x6DCC8` (`FUN_0006DCC8`).

Engine/loader source read first-hand:
`include/fifa96_engine/fifa96_match_handlers.h` (33..79);
`src/fifa96_engine/fifa96_match_handlers.c` (4547..4705, the two tables + both
dispatch resolvers); `src/fifa96_engine/fifa96_match_run.c` (2360..2410,
2590..2670, 2810..2829); `src/fifa96_engine/fifa96_match_act.c` (whole, 95
lines); `src/fifa96_loader/fifa96_action_handlers.c` (1050..1161, the phase-row
desc + the situation table + the act map).

Docs cross-cited: FU-136, FU-137, FU-138, FU-142f, FU-143, FU-149, FU-165,
FU-170, FU-174, FU-175, FU-177, FU-178, FU-180, FU-181, FU-182, FU-183, FU-184,
FU-186, FU-187, FU-188, FU-189, FU-190; the phase-17 plan; AGENTS.md census
rule. No writes beyond this slice file and the workspace report.
