# FU-214 — census/audit + the act-7-8 cohort arithmetic + the per-producer tape register (phase-19 wave R62)

Provenance: M2 phase-19 wave **R62**, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` —
`FIFA96.EXE` / path `/FIFA96.EXE`, `watcom:LE:32:default`, image base `0`,
min `0x10000`, max `.image::00174a2a`, 2706 functions, 15239 symbols, `.image`
overlay present). Every table byte, xref count, slot byte and instruction below
is a **fresh** `read_memory` / `get_xrefs_to` / `disassemble_bytes` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named). The alias
traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`); LE
globals sit at `+0x100000` from the file offset. Engine source read first-hand
(`include/fifa96_engine/fifa96_match_handlers.h`,
`src/fifa96_engine/fifa96_match_handlers.c`, `fifa96_match_run.c`,
`fifa96_match_act.c`, `src/fifa96_loader/fifa96_action_handlers.c`,
`tests/test_engine_m2.c`). The M1/M2 goldens were **run first-hand** this
session. **No writes**: no rename, comment, label, function, script or project
save; `/FIFA96.EXE` untouched; the ISO / `game/hdd/` untouched; no git commands.

Scope (R62 brief): (1) re-verify the current census first-hand and reconcile the
`floor(100 × wired / 80)` figure; (2) the honest arithmetic for the act-7-8
cohort (phase `0x1D`, phase `0x1E`, phase `0x22`, action `0x20`, the situation-8
producer), each candidate's census delta under the AGENTS.md rule (count only
when a ported path installs **and** dispatches); (3) the cohort's per-producer
**TAPE** register against the M2 golden; (4) re-rank the remaining legs across
all tiers. This slice audits the phase-19 recon wave
(`FU207`…`FU211`) against the phase-18 audit (`FU-206`).

---

## 1. Fresh census machinery — the two native tables

`include/fifa96_engine/fifa96_match_handlers.h:34..39` declares the census
domain: action `FIFA96_MATCH_ACTION_ROWS 45` (`0x1106E0[45]`, codes `0x00..0x2C`),
phase `FIFA96_MATCH_PHASE_ROWS 35` (`0x110794[35]`, phases `0x00..0x22`), the
INT3 slot `FIFA96_MATCH_PHASE_INT3_SLOT 0x16` — **80 rows**.

**Action table `0x1106E0`** — fresh `read_memory` (180 B = 45 dwords) returns the
full table byte-for-byte identical to FU-206 §1 / FU-198 §1:
`00:07DB10 01:07DBC0 02:07DFCC 03:07E1A4 04:07E7C8 05:07F194 06:0801B4
07:0814B0 08:081068 09:080A00 0A:081738 0B:081908 0C:081C90 0D:08251C
0E:082710 0F:082AD0 10:0855F0 11:085DE4 12:083D68 13:084B00 14:08784C
15:087CD0 16:084630 17:084730 18:0849B0 19:0746E4 1A:07662C 1B:076D28
1C:077728 1D:074EB0 1E:07550C 1F:076380 20:084EEC 21:085214 22:08539C
23:082F84 24:086510 25:0880CC 26:0866F4 27:086820 28:0870E8 29:0874E4
2A:086A34 2B:087738 2C:084598`.

**Phase table `0x110794`** — fresh `read_memory` (140 B = 35 dwords), byte-for-
byte identical to FU-206 §1 / FU-198 §1:
`[0]=06DE34 [1]=06E1D0 [2]=06DCC8 [3]=06DE44 [4]=06DE44 [5]=06E05C [6]=06DD9C
[7]=06DE44 [8]=06DD6C [9]=06DD6C [0A]=06DE34 [0B]=06DE34 [0C]=06DF4C
[0D]=06DE34 [0E]=06DE34 [0F]=06DE34 [10]=06E004 [11]=06E1C8 [12]=06E1D0
[13]=06E244 [14]=06E244 [15]=06DCC8 [16]=00000000 (INT3) [17]=088DC8 [18]=08922C
[19]=089FA4 [1A]=089620 [1B]=0890EC [1C]=089110 [1D]=089868 [1E]=08A798
[1F]=088F4C [20]=08B688 [21]=08B874 [22]=08B900`.

* **Installer `0x7D9A4`** — fresh `get_xrefs_to` = **77** call sites
  (byte-identical set to FU-174/FU-182/FU-190/FU-198/FU-206). The four
  phase-0x1D arm-0 sites are present first-hand: `0x8990A` (row `0x19`),
  `0x8994C` (code `3`), `0x899AB` (row `0x20`), `0x899CD` (row `0x1F`).
* **Act table `0x1107EC`** — fresh `read_memory` (72 B):
  `[0]=0 [1]=088DC8 [2]=08922C [3]=089FA4 [4]=089620 [5]=0890EC [6]=089110
  [7]=089868 [8]=08A798 [9]=088F4C [A]=08B688 [B]=08B874 [C]=08B900`, then the
  unrelated `0x006007FF…` data. Base is exactly `0x110794 + 0x16*4`, so
  **act `a` → phase `0x16+a`**.
* **Cohort slots (fresh)**: `0x110760` = `ec 4e 08 00` = **action `0x20` =
  `0x84EEC`**; `0x110808` = `68 98 08 00` = **phase `0x1D` = `0x89868`**;
  `read_memory 0x110808+4` = `98 a7 08 00` = phase `0x1E` = `0x8A798`;
  `0x11081C` = `00 b9 08 00` = phase `0x22` / act `0xC` = `0x8B900`.

**Fresh xrefs (cohort):** `get_xrefs_to 0x89868` = **1** (DATA from `0x110808`);
`get_xrefs_to 0x84EEC` = **1** (DATA from `0x110760`). Neither body has a direct
`CALL` — both are reached only through the table seam. **Fresh disassembly of the
cohort sites** (this session): `0x4B0CB TEST AH,0x1 ; 0x4B0D9 CALL 0x8A938
(EAX=8) ; 0x4B0DE MOV byte[0x157AC2],4`; `0x8AEAE MOV EBX,1 ; 0x8AEB3 MOV EAX,7 ;
0x8AEBA CALL 0x888FC`; `0x899A2 MOV EDX,0x20 ; 0x899A7 MOV EAX,[ESP+8] ;
0x899AB CALL 0x7D9A4`; `0x8B900` prologue (`PUSH …; MOV AX,[0x157A64] ; …`) —
all byte-exact to the FU-207…FU-211 derivations.

**Audit verdict: the native count machinery and the cohort slot bytes are
provable as stated and unchanged since FU-206.** No table byte, xref count or
slot moved.

### 1.1 Engine-side count (fresh first-hand source read)

`src/fifa96_engine/fifa96_match_handlers.c:4967..5056` (action table) and
`:5058..5129` (phase table), read first-hand this session:

* **Action table — 32 non-NULL `fn`s**: `00 01 02 04 05 06 07 08 09 0B 0C 0E 0F
  10 11 12 13 14 16 18 19 1B 1C 1D 1E 1F 21 23 24 26 28 2A`. **13 NULL**:
  `03 0A 0D 15 17 1A 20 22 25 27 29 2B 2C`.
* **Phase table — 19 non-NULL `fn`s**: `00 02 03 04 05 06 07 08 09 0A 0C 0F 15
  17 18 19 1F 20 21`. **15 NULL** (ex-INT3): `01 0B 0D 0E 10 11 12 13 14 1A 1B
  1C 1D 1E 22`. Plus the INT3 `0x16` = **1 NOTF**.

`tests/test_engine_m2.c:2959..2964` (and the mirror at `:3326..3331`) pins the
shape first-hand: `assert(action_wired == 32u && phase_wired == 19u);  /* 51/80 =
63% */`. Dispatch arithmetic: **51 OK + 28 UNSUP (13 action + 15 phase) + 1 NOTF
(phase `0x16`) = 80**.

**Audit verdict: the `51/80 = 63%` claim is provable as stated.** `floor(100 ×
51/80) = floor(63.75) = 63`. All four census surfaces agree this session:
`README.md:1` (`63%`), `docs/ENGINE.md:8` + the phase-18 T6/T7 lines (`51/80`,
dispatch `28 UNSUP / 51 OK / 1 NOTF`, sum 80), `docs/PROGRESS.md` (`51/80 = 63%`),
`AGENTS.md:90` (`51/80 → 63%`), and the `test_engine_m2` rider. The M1 golden
`09b726b7…` and the M2 golden `441415fc…` are **byte-identical** this session
(first-hand `cmp`).

---

## 2. The verified current census (51/80 = 63%)

| | count |
|---|---|
| action rows wired (non-NULL `fn`) | **32** |
| action rows UNSUP (NULL) | 13 |
| phase rows wired | **19** |
| phase rows UNSUP (NULL, ex-`0x16`) | 15 |
| phase `0x16` (INT3 slot) | 1 NOTF |
| **wired / total** | **51 / 80 → 63%** |

Dispatch: `51 OK / 28 UNSUP / 1 NOTF` (sum 80; action 45 = 32 OK + 13 UNSUP;
phase 35 = 19 OK + 15 UNSUP + 1 NOTF).

The 15 NULL phase rows (the phase-side movers, ex-INT3):
`01 0B 0D 0E 10 11 12 13 14 1A 1B 1C 1D 1E 22`.
The 13 NULL action rows (the action-side movers):
`03 0A 0D 15 17 1A 20 22 25 27 29 2B 2C`.

Since FU-206 (`48/80`) the phase-18 wave (`c74a68b` and its parents) landed three
rows, all accounted: action `0x24` (T3), phase `0x20` (T3), phase `0x17` (T4).

---

## 3. The act-7-8 cohort arithmetic (honest deltas)

The cohort activates the **situation-8 → act 7 → phase `0x1D`** route plus its
two companion extra-time rows. The AGENTS.md rule (§Completion percentage): the
census counts a row only when a **ported path installs AND dispatches** — a
`NULL → fn` flip with no live producer is **not** counted (the rows-`27`/`29`/`2C`
precedent, restated at `handlers.c:5030`/`:5117`).

| candidate | kind | census delta | why (extension point) |
|---|---|---|---|
| **situation-8 producer** | producer | **+0** | not a table row; it is the enabler for `0x1D`. Both native sit-8 gates are inert in the port (FU209 §6.1): `input_range & 0x100` is never set (`fifa96_input.c:86`, no `0x100` bit) and `global_157ac2` never reaches `4` (only `=0` reset, `match_run.c:2034/2152`; never `++`). `match_run.c:3079`/`:3093`. |
| **phase `0x1D`** | phase row | **+1** | `handlers.c:5117` `NULL → fifa96_match_action_phase_1D` (body ported, `match_run.c:5998`). Honest **only** with the sit-8 producer live (above) so the act-7 seam dispatches it; registration alone leaves the act tick early-returning at `fifa96_match_act.c:63`. |
| **action `0x20`** | action row | **+1** | body `0x84EEC` is **unported** (FU-210 §7.2); requires porting `fifa96_match_action_20` + `handlers.c:5030` `NULL → fn` + the coupled arm-0 installer `0x899A2` (currently leg'd at `match_run.c:6017`). A mechanical flip is explicitly uncounted (FU210-L5). |
| **phase `0x22`** | phase row | **+1** | `handlers.c:5127` `NULL → fifa96_match_action_phase_22` (body ported, `match_run.c:6158`) **plus** the OL-85 flag producer threaded into `fifa96_match_run_phase_drive` (replace the hardwired `extra_time = 0u` at `match_run.c:2583`). Registration alone is wired-but-unreachable (FU211 §5). |
| **phase `0x1E`** | phase row | **+0** | blocked: the engine keeps act-8 **compressed** (`fifa96_match_run_set_piece` BX!=0); registering the full 22-record sweep would de-compress a tape-visible derivation (FU194-L2, FU-206 §4 Tier-2 #7). Body ported (`match_run.c:6061`) but the slot stays NULL. |
| *(phase `0x10`, contingent)* | phase row | **+1\*** | the sole writer is `match_run_write_phase(mr, 0x10)` **inside** the phase-`0x1D` arm 0 (`match_run.c:6007`; native `0x898DD`). It dispatches only once phase `0x1D` dispatches **and** the phase-`0x10` row is registered (`handlers.c:5091`, body `fifa96_match_phase_10` at `:4807`). Closes *with* `0x1D` — not an independent mover. |

**Cohort honest ceiling.** `51 + 1 (0x1D) + 1 (0x20) + 1 (0x22) = 54/80`
(`floor(100 × 54/80) = 67`). With the contingent `0x10` free-rider, `55/80`
(`floor(68.75) = 68`). **The cohort has no honest mover until the OL-85 /
`[0x157AC2]` extra-time producer lands**; every named row above is downstream of
it. This restates FU-208 §6.2's `+2 → 53/80 = 66%` for `{0x1D, 0x20}` and adds
`0x22` (`+1`), while correctly excluding `0x1E`.

**The sharpest new blocker (FU207/FU208).** There are **two** extra-time cells,
and the cohort keystone was mis-named in the brief: **`[0x157AC2]` (the period
index) selects situation 8**, not `[0x157AC0]` (the OL-85 flag, which gates the
*companion* act `0xC`/phase `0x22` and the phase-`0x1D` terminal write-back).
Worse, the sit-8 gate tests **bit 8** (`0x100`) of `[0x14C1D4]|[0x14C1D6]`, but
the only two range-word builders write bit **9** (`0x200`) or zero — **bit 8 has
no located writer** (FU208-L1 / FU209-L1). So the sit-8 producer is blocked by an
unlocated config producer, not merely by an un-gate.

---

## 4. The cohort's per-producer TAPE register (M2 golden)

**Ground truth (fresh first-hand, re-confirmed).** `match_run_scene_stage` reads
exactly four record fields — `pos_x/pos_y/pos_z`, `anim_id`, `frame`, `hidden`
(`fifa96_match_run.c:1595..1607`). A producer is **tape-visible** iff its live
wiring changes one of those; otherwise **tape-dormant**. The M2 v18 tape
(`tests/test_engine_m2.c:1..97`) drives boot → intro → front-end → `begin` →
kickoff → move → kick → the forced `0x13`/`0x14` window → the band scenario → the
natural goal → the post-goal hold → stop; it stages **no extra-time period, no
foul/offside, no period completion** (`ref_machine == REF_NONE`; FU-207 §6: "the
M2 fixture never reaches a period completion (it ends at the goal)").

| # | producer | live wiring | render fields | tape verdict | sibling verdict |
|---|---|---|---|---|---|
| **C-1** | situation-8 / extra-time producer (`FUN_0004B02C` bit-8 gate `0x4B0CB`; `FUN_00088860` `global_157ac2==4` `0x888C8`) | the `match_run.c:3079`/`:3093` gate inputs | none (selects act 7) | **dormant** — no period completion on M2; bit-8 structurally unwritten | FU207 §6, FU208 §6.2, FU209 §6.1 |
| **C-2** | phase `0x1D` (act 7 set-piece timeline `0x89868`) | the act-7 seam dispatch | installs row `0x19`; coupled row `0x20` (deferred) | **dormant** — the body is not live-reachable (slot NULL + no producer) | FU209 §6.2, FU-206 §5 PR-7/PR-8 |
| **C-3** | action `0x20` (body `0x84EEC`) | the coupled `0x899A2` install (deferred, invoke 0) | **`pos_x/pos_y/pos_z` + `anim_id` + `frame`** (arm 0 `0x79B6C`/`0x8CFAC`; arm 4 `0x7B9C4→0x7A490`) | **dormant** — no record ever carries code `0x20`; `hidden` never moves | FU210 §7.4 |
| **C-4** | phase `0x22` (act-0xC timer `0x8B900`) | the `fifa96_match_act_dispatch(0xC, 0, 1)` path | **none** — writes only the act slot + `mr->ref_whistle` (`0x974DC(0x1E)`, audio, not render-read) | **dormant / tape-neutral even if fired** | FU211 §5 |
| **C-5** | phase `0x1E` (act-8 restart sweep `0x8A798`) | the act-8 fallback (engine models it compressed) | the 22-record code-`0xC`→`0x19` sweep | **dormant, but risky** — registering it de-compresses a tape-visible derivation | FU-206 §5 PR-9, FU194-L2 |
| *(C-6)* | phase `0x10` (contingent, inside `0x1D`) | `match_run_write_phase(mr,0x10)` | target triple only (render-invisible) | **dormant** | FU209 §5.4 |

**Tape verdict (one line).** **No cohort candidate moves M2.** As dormant
registrations (rows `0x1D`/`0x1E`/`0x22`, action `0x20`) the tape is
byte-identical (`441415fc…`, first-hand `cmp`). The *only* way the cohort becomes
tape-visible is if the coupled sit-8 + row-`0x20` installer is made to **fire on
M2** — which requires the tape to stage an extra-time period (a change to the
tape's content, not to engine code). **No re-pin is forced by any honest dormant
landing.** The single code-side re-pin risk is **C-5/phase `0x1E`**: registering
the sweep in place of the compressed act-8 model could move M2 if the act-8
fallback fires; keep the slot NULL until a `cmp` pins it. The AGENTS.md golden
rule stands: any landing that could reach the canvas must `cmp`-pin M2 first.

---

## 5. Re-ranked remaining legs (from FU-206 + the phase-19 slices)

Cross-cited to the owning FU slice; the tape column is §4's verdict. Phase-18
moved `0x20`/`0x24`/`0x17` off the board (now counted), so the tiers below are
the residual.

**Tier 1 — producer present; one honest un-gate; census movers.**

| # | row(s) | producer | un-gate | tape |
|---|---|---|---|---|
| 1 | phase **`0x22`** (`+1`) | act-`0xC` slice `0x1107EC[C]` = `0x8B900` (body ported) | the OL-85 flag producer (`FUN_0008AF38` set-sites) threaded into `phase_drive` (`match_run.c:2583`) | dormant / neutral (C-4) |
| 2 | phase **`0x1D`** (`+1`) + action **`0x20`** (`+1`) | sit-8 → act 7 → phase `0x1D` (`0x89868`); coupled installer `0x899A2` | the extra-time producer (OL-85 / `global_157ac2`) **and** porting body `0x84EEC` + the coupled arm-0 install | dormant (C-1..C-3); visible only if fired |
| 3 | phase **`0x10`** (`+1`, contingent) | the phase-`0x1D` arm-0 writer (`0x898DD`) | closes *with* row `0x1D` + registering `handlers.c:5091` | dormant (C-6) |

**Tier 2 — producer present but blocked by a double-run / tape-visible derivation
/ an unported arm.**

| # | row | producer | blocker |
|---|---|---|---|
| 4 | phase **`0x1E`** | act-8 sweep `0x8A798` (body ported) | de-compression of the tape-visible derivation (FU194-L2; C-5) |
| 5 | phase **`0x1A`** | act-4 formation-order walk `0x89620` | no live act-4 producer; the sit-2..4 arm gate is a permutation residual (`FUN_0008C974`; FU-201/FU193-L3) |
| 6 | phase **`0x1C`** | act-6 pre-restart `0x89110` | no live act-6 producer + the offside double-model (FU193-L4) |
| 7 | action **`0x03`** | live phase-2 `[rec+0x1C]` = `0x6DCC8` | wiring the row **moved M2 at frame 29** (R7); owner FU177-L1 |
| 8 | action **`0x0D`** | row-09 arm-1 `0x80F61` | row-09 arm 1 unported + its gates (FU178-L1) |
| 9 | action **`0x17`** | act-4 stage-0 `0x89740` | the sit-2..4 formation-order arm gated dormant (FU181-L1/FU201) |
| 10 | action **`0x1A`** | the `FUN_00077EAC` CPU ladder | the ladder + `FUN_0008D8EC→FUN_000782D0` pre-pass unported (FU186-L5) |
| 11 | action **`0x22`** | row-07 `0x816DF` invoke-now | OL-65 angle inputs staged 0 → `out.opponent_invoke` never fires (FU185-L6) |

**Tier 3 — producer present only behind a Tier-2 body that itself is a leg.**

| # | row | producer | blocker |
|---|---|---|---|
| 12 | phase **`0x0B`** | `match_run_write_phase(mr, 0x0B)` at `match_run.c:5732` | the writer sits inside the unwired phase-`0x1A` body (FU192 §1 / FU-206 §4 #13) |
| 13 | phase **`0x0E`** | — | no live writer (the reach set excludes it, FU-191 §10) |

**Tier 4 — dead / no-move (cannot honestly move the census).**

* action `0x0A`/`0x15` — installer `0x7CDD8` has no xrefs; head mis-decoded stub
  (FU-137 §6, OL-14).
* action `0x25` — no installer exists (bounded `MOV EDX/DX,0x25` scan, FU189-L2).
* action `0x27`/`0x29`/`0x2C` — ported bodies, no invocation in the 77-site
  `0x7D9A4` census (FU-142f/OL-48).
* action `0x2B` — dead entry: native `0x87738` is the shared row-29 epilogue RET
  (FU-142f/OL-15).
* phase `0x1B` (no invoker, FU195-L1) / `0x16` (INT3 slot).

**Re-rank delta vs FU-206.** The two phase-18 movers are gone; the frontier's
top is now the **extra-time pair**: `0x22` (cheapest — registration + threading
the flag) then the `0x1D`+`0x20` coupled pair (heaviest — a producer plus a
not-yet-ported body plus the coupled installer). Both are gated on the same
unlocated producer (OL-85 / `[0x157AC2]` / bit-8, FU208-L1). `0x10` is a
contingent rider on `0x1D`; `0x1E` should stay leg'd to protect the tape.

---

## 6. Numbered open legs

1. **FU214-L1 — the cohort has no honest census mover until the extra-time
   producer lands.** Every named cohort row (`0x1D`, `0x1E`, `0x22`, action
   `0x20`, and the contingent `0x10`) is downstream of the OL-85 / `[0x157AC2]`
   producer, which is unported (`extra_time = 0u` hardwired, `match_run.c:2583`;
   no `global_157ac0` cell). Do not `NULL → fn` flip any cohort slot for a count;
   the honest ceiling is `54/80 = 67%` (`55/80 = 68%` with `0x10`), reached only
   by a producer-faithful landing. Owners: FU195-L2 / FU207-L1 / FU208-L3.
2. **FU214-L2 — the bit-8 vs bit-9 range-word gap is the sharpest blocker
   (extends FU196-L1 / FU208-L1 / FU209-L1).** The sit-8 gate tests **bit 8**
   (`0x4B0CB TEST AH,0x1`), but the only two range-word builders write bit **9**
   (`0x200`) or zero; **bit 8 has no located writer**. Until the extra-time config
   producer is located, the sit-8 route (and hence phase `0x1D` / action `0x20`)
   cannot be honestly produced. Do not fake a bit-8 producer.
3. **FU214-L3 — the row-`0x20` body `0x84EEC` is unported (OL-9).** A mechanical
   `handlers.c:5030` flip moves `51 → 52` but is uncounted (R8 / the
   rows-`27`/`29`/`2C` precedent); port the 7-arm body + the coupled arm-0
   installer `0x899A2` as one unit. Owner: FU-189 / FU210-L1.
4. **FU214-L4 — phase `0x10` is a contingent `+1`, not an independent mover
   (extends FU-206 §4 Tier-3 #12).** Its sole writer sits inside the phase-`0x1D`
   arm 0 (`match_run.c:6007`); it dispatches only once `0x1D` dispatches **and**
   `handlers.c:5091` is registered. Do not double-count it with the `0x1D`+`0x20`
   pair. Owner: the phase-`0x1D` landing.
5. **FU214-L5 — phase `0x1E` is the sole cohort re-pin risk.** Registering the
   act-8 restart sweep in place of the engine's compressed stored-situation
   re-dispatch could move M2 if the act-8 fallback fires (FU194-L2). Keep the
   slot NULL until a `cmp` pins the fallback. Owner: FU194-L2.
6. **FU214-L6 — the cohort is tape-dormant on the v18 M2 tape; no re-pin is
   forced by an honest dormant landing.** The tape never completes a period
   (ends at the goal; FU-207 §6) and stages no extra-time period, so the sit-8
   producer, phase `0x1D`, action `0x20`, phase `0x22` and phase `0x10` are all
   dormant; phase `0x22` is tape-neutral even if fired (act slot + audio only).
   Any landing that could reach the canvas must `cmp`-pin M2 first (AGENTS.md).
7. **FU214-L7 — the two extra-time cells are distinct and must not be
   conflated.** `[0x157AC2]` (period index) selects situation 8; `[0x157AC0]`
   (OL-85 flag) gates act `0xC`/phase `0x22` and the phase-`0x1D` terminal
   write-back (`0x89E8C`). The phase-19 brief's "keystone = `[0x157AC0]`"
   attribution is corrected (FU-208 §5, FU-208 errata).

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, 2706
  functions, min `0x10000`, max `.image::00174a2a`, `.image` overlay.
* `read_memory` — `0x1106E0` (180 B), `0x110794` (140 B), `0x1107EC` (72 B).
* `get_xrefs_to` — `0x7D9A4` (**77**), `0x89868` (**1**, DATA `0x110808`),
  `0x84EEC` (**1**, DATA `0x110760`).
* `disassemble_bytes` — `0x4B0C0` (40 B, the bit-8 sit-8 producer), `0x8AE9E`
  (32 B, the sit-8 arm invoke-now), `0x89998` (32 B, the row-`0x20` coupled
  install), `0x8B900` (24 B, the phase-`0x22` head).
* M1/M2 goldens run first-hand: `./build/test_engine_m1 | cmp - tests/golden/
  engine/m1-frames.txt` (clean, `09b726b7…`); `./build/test_engine_m2 | cmp -
  tests/golden/engine/m2-frames.txt` (clean, `441415fc…`).

Engine/loader source read first-hand: `include/fifa96_engine/fifa96_match_handlers.h`
(34..39, the census domain); `src/fifa96_engine/fifa96_match_handlers.c`
(4967..5056 the action table incl. the NULL `0x20` at `:5030`; 5058..5129 the
phase table incl. the NULL `0x1D`/`0x1E`/`0x10` at `:5117`/`:5119`/`:5091` and
the NULL `0x22` at `:5127`; 5138..5154 the dispatch resolvers);
`src/fifa96_engine/fifa96_match_run.c` (1595..1607 the render reads; 2564..2602
`phase_drive` incl. `extra_time = 0u` at `:2583`; 3063..3099 `period_setup` incl.
the bit-8 gate `:3093`; 5979..6044 the phase-`0x1D` port; 6046..6111 the
phase-`0x1E` port; 6127..6183 the phase-`0x21`/`0x22` ports);
`src/fifa96_engine/fifa96_match_act.c` (the NULL-row tick guard);
`src/fifa96_loader/fifa96_action_handlers.c` (1114 `fifa96_action_phase_act`;
1129..1147 the situation table, sit 8 = `{NONE, 7, 0}` at `:1142`; 1163..1210
`fifa96_action_phase_period_end`); `src/fifa96_loader/fifa96_input.c`
(`fifa96_input_range_words`); `include/fifa96_engine/fifa96_match_run.h`
(`:278` the `extra_time` overlay field; `:772`/`:1571` the phase-`0x1E` decl);
`tests/test_engine_m2.c` (1..97 the tape; 2952..2964 + 3326..3331 the census
riders).

Docs cross-cited: FU-174, FU-175, FU-177, FU-178, FU-181, FU-182, FU-185,
FU-186, FU-189, FU-190, FU-191, FU-192, FU-193, FU-194, FU-195, FU-196, FU-198,
FU-201, FU-202, FU-203, **FU-206** (the phase-18 audit), **FU-207** (OL-85 flag),
**FU-208** (situation-8 selector), **FU-209** (act 7 / phase `0x1D`), **FU-210**
(action row `0x20`), **FU-211** (phase `0x22` / act `0xC`); AGENTS.md census rule.
No census change and no engine change: `51/80 = 63%` and the README/ENGINE/PROGRESS
titles stand unchanged. No writes beyond this slice file; no
rename/label/comment/function/script/project save; `/FIFA96.EXE` and the ISO
untouched; no git commands.

---

## 8. Landing note (M2 phase-19 T5) — the residual-leg register

Phase-19 T1–T4 landed the front half of this audit's cohort (the extra-time
producer, the sit-8 selector + phase `0x1D`, the row-`0x20` body/installer, phase
`0x22`). T5 registers what remains open — each with its **native owner** — and
confirms the audit's honest ceiling was not over-reached. **No row is wired by
T5: census `53/80 = 66%` unchanged** (`26 UNSUP / 53 OK / 1 NOTF`, sum 80).

### 8.1 The phase-`0x1E` bound (this audit's FU214-L5)

Upheld: the phase-`0x1E` slot stays `NULL`. FU-212 §6/§9 proves the faithful
22-record restart sweep is a **timing-only** delta (render-invisible; dormant on
M2), so the engine keeps its compressed act-8 model and the model is **pinned**
(`tests/test_engine_match_frame.c::test_phase_1E_compressed_model_pin`; mutation-
RED: registering the slot aborts the pin). De-compression owner **FU212-L1** /
FU194-L2. M2 `441415fc…` byte-identical, M1 `09b726b7…` unmoved.

### 8.2 The residual-leg register (native owners kept)

| leg | status at T5 | native owner / remainder |
|---|---|---|
| **FU207-L1** — OL-85 flag's front-end producer | **partly resolved** | the in-match **cell + writer** landed in T1/FU-213 (`mr->global_157ac0`); the **front-end competition match-init producer `0x389A4`** (`FUN_00038630 ← FUN_00039054 ← FUN_000493a0`, FU207-L2/FU202-L7) is still unported. Owner FU-207. |
| **FU208-L1** — bit-8 sit-8 range-word writer gap | **OPEN** | the sit-8 gate's bit 8 (`0x4B0CB TEST AH,0x1`) still has no located writer (the two builders write bit 9 `0x200` or 0). Owner FU-208 / FU196-L1. |
| **FU208-L8** — sit-8 session-gate co-condition | **OPEN** | the engine's `session_gate_14c32a` is seeded 0 with no derived producer, so the live direct-path reachability of sit 8 is unproven (FU-166/R14). Owner FU-208. |
| **FU209-L1** — act-7 producer inert path | **resolved for the `global_157ac2 == 4` arm** | T2/FU-208 made the roll live (see FU209 §10); the **bit-8 arm stays FU208-L1**. |
| **FU211-L1** — phase `0x22` off registration alone | **RESOLVED** | T4 registered the slot on a live-path fixture (FU211 §10). Do **not** re-leg. |
| **FU213-L1** — `[0x157AC0]` cell/producer | **resolved** | T1 landed the cell (`mr->global_157ac0`) + the W2/W4/W5 in-match producers; the **front-end producer stays FU207-L2**. |
| **FU213-L2** — `period_setup(mode 0)` live re-arm | **resolved for the sit-8 path** | T2 calls the mode-0 producer from `phase_drive` on the `global_157ac2 == 4` gate. |
| **FU214-L1** — cohort's producer dependency | **partly resolved** | producer landed (T1); `0x1D`+`0x22` counted (T2/T4). Remainder — action `0x20` — still blocked by the actor-cell producer (**FU210-L3**). |
| **FU214-L2** — bit-8 vs bit-9 range word | **OPEN** | sibling of **FU208-L1** (unchanged). |
| **FU214-L3** — row-`0x20` body | **resolved** | body + coupled installer ported by T3/FU-210; the row stays `NULL` on FU210-L3/L5. |
| **FU214-L4** — phase `0x10` contingent | **OPEN** | owner: the phase-`0x1D` landing (its arm-0 writer). |
| **FU214-L7** — the two cells distinct | **stands** | `[0x157AC2]` (sit-8 selector) vs `[0x157AC0]` (OL-85 flag); the attribution correction is honoured by T1/T2/T4. |

No fake producer: every OPEN row above is un-dispatched on the live path and
carries no `NULL → fn` flip for a count.

