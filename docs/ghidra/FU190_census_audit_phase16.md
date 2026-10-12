# FU-190 — census/act/phase audit + rows 03/0x0D completion path (phase-16 wave R38)

Provenance: M2 phase-16 wave **R38**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument). Every xref count, table byte, call byte and instruction below is a
fresh `get_xrefs_to` / `read_memory` / `disassemble_bytes` /
`decompile_function` of **this session** unless marked *cross-cited* (then the
frozen FU doc is named). Engine source read first-hand
(`src/fifa96_engine/fifa96_match_handlers.c`, `fifa96_match_handlers.h`).
No writes: no rename, comment, label, function, script or project save;
`/FIFA96.EXE` untouched; no git commands. The alias traps are applied
(`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`).

Scope: (1) re-verify the count machinery first-hand (the two native tables
`0x1106E0`/`0x110794`, the `0x7D9A4` installer xref count, the act table
`0x1107EC`); (2) state the verified census and the post-phase-16 arithmetic;
(3) rank the phase-16 movers and restate the blocked/no-move set; (4) derive
the completion paths for the two **ported-but-uncounted** rows — **0x0D**
(installer = row-09 arm 1 `0x80F61`, FU178-L1) and **03** (the live phase-2
path `[rec+0x1C] = 0x6DCC8`, FU177-L1).

This audit is contemporaneous with the phase-16 wave (R31–R37 + FU-183…189 are
being written in parallel); where a mover's outcome is a T1–T5 landing, the
rank below is its **prospective** status, not a claim.

---

## 1. Fresh census machinery — the "N/80 wired" claim

The engine census counts non-NULL `fn` rows of the two native dispatch tables
(`include/fifa96_engine/fifa96_match_handlers.h:33..38`: action
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
  2A:086A34 2B:087738 2C:084598` — byte-identical to FU-182 §1.
* **Phase table `0x110794`** — fresh `read_memory` (140 B = 35 dwords), rows
  `0x00..0x22`, ending at `0x110820`. Fresh values re-confirm FU-177 §3:
  `[0]=06DE34 [1]=06E1D0 [2]=06DCC8 [3]=06DE44 [4]=06DE44 [5]=06E05C [6]=06DD9C
  [7]=06DE44 [8]=06DD6C [9]=06DD6C [0A]=06DE34 [0B]=06DE34 [0C]=06DF4C
  [0D]=06DE34 [0E]=06DE34 [0F]=06DE34 [10]=06E004 [11]=06E1C8 [12]=06E1D0
  [13]=06E244 [14]=06E244 [15]=06DCC8 [16]=00000000` (the INT3 slot)
  `[17]=088DC8 [18]=08922C [19]=089FA4 [1A]=089620 [1B]=0890EC [1C]=089110
  [1D]=089868 [1E]=08A798 [1F]=088F4C [20]=08B688 [21]=08B874 [22]=08B900`.
* **Installer `0x7D9A4`** — fresh `get_xrefs_to` = **77** call sites
  (byte-for-byte the same set as FU-174 §1 / FU-182 §1). The reader body
  `FUN_0007D9A4` resolves `EAX=0x1106E0[code]` and stores `[rec+0x18]`; the two
  table reads are the only `DATA` refs to `0x1106E0`.
* **Action-table `0x1106E0` reader** — fresh `get_xrefs_to` = **2**, both in
  `FUN_0007D9A4` (`0x7DA77`/`0x7DA7C`): the installer is the sole native reader.
* **Phase-table `0x110794` reader** — fresh `get_xrefs_to` = **2**, both in
  `FUN_0006D920` (`0x6D9B3`/`0x6D9B8`): the per-record stager is the sole reader.
* **Act table `0x1107EC`** — fresh `read_memory` (64 B) returns
  `0,_88DC8,_8922C,_89FA4,_89620,_890EC,_89110,_89868,_8A798,_88F4C,_8B688,
  _8B874,_8B900` then unrelated dwords (`006007FF`,`07CE38`,`004007FF`); the
  base is exactly `0x110794 + 0x16*4`, so act `a` → phase `0x16+a`. Fresh
  `get_xrefs_to 0x1107EC` = **2**, both in `FUN_000888FC` (`0x88913`/`0x88920`).
  Byte-identical to FU-175 §1.

Engine-side count (source `fifa96_match_handlers.c`, first-hand): the action
table has **28 non-NULL `fn`s** (`00 01 02 04 05 06 07 08 09 0B 0C 0E 0F 10 11
12 13 18 19 1B 1D 1E 1F 21 23 26 28 2A`) and **17 NULL**; the phase table has
**1 non-NULL** (`0x1F`, `fifa96_match_action_phase_1F`) and **34 NULL** (incl.
the INT3 slot). Dispatch arithmetic: **29 OK + 50 UNSUP (17 action + 33 phase) +
1 NOTF (phase `0x16`) = 80**. The README title `36%` = `floor(100 × 29/80) =
floor(36.25) = 36`.

**Audit verdict: the `29/80` claim is provable as stated.** FU-182 §1's `24/80`
was the *pre-phase-15* snapshot (its §3 projected 24→31 on the phase-15 plan);
phase-15 T1 landed the act seam + phase row `0x1F` and T2/T3 landed rows
`09`/`1B`/`1F`, so the verified count is now **29/80 = 36%** (ENGINE.md §title +
§"Action dispatch" both say `29/80`; README title `36%`). The action non-NULL
set grew by 4 over FU-182's `24`: `09` (FU-178), `1B` (FU-180), `1F` (FU-176),
and the previously-wired `0B`/`0C` re-counted in FU-182 §8.1 — the per-row
history is ENGINE.md's, not re-litigated here.

---

## 2. The verified current census (29/80)

| | count |
|---|---|
| action rows wired (non-NULL `fn`) | **28** |
| action rows UNSUP (NULL `fn`) | 17 |
| phase rows wired | **1** (`0x1F`) |
| phase rows UNSUP (NULL, ex-`0x16`) | 33 |
| phase `0x16` (INT3) | 1 NOTF |
| **wired / total** | **29 / 80 → 36%** |

Dispatch: `29 OK / 50 UNSUP / 1 NOTF`.

Errata (audit): FU-182 §2's table (`24/80`, phase `0/35`) is the pre-phase-15
snapshot; FU-182's own §3 projected the phase-15 landing and its §10 was not
written for the phase-15 outcome. This slice supersedes it with the phase-15
landed state. No prior read is contradicted — only the snapshot date differs.

---

## 3. The post-phase-16 arithmetic

Phase-16 (`docs/superpowers/plans/2026-10-12-fifa96-m2-phase16-act-machine-frontier.md`)
targets **nine action rows** and a producer-live phase cohort:

* **Act-machine producer (T1/R31):** row **0x17** (body already ported FU-181;
  its only installer is act-4 stage 0 `0x89740` — fresh below).
* **First-order live-install bodies (T2/T3):** **0x14**, **0x22**, **0x1A**,
  **0x1C**.
* **Act-seam bodies (T4):** **0x16**, **0x24**.
* **Completion-path rows (T5):** **0x0D** and **03** (§6).

Projection (not a claim — each landing is proven per task; **no hand-set
number**, per the plan's census rule):
`floor(100 × wired/80)` with the action non-NULL `28 → 28+9 = 37` and the phase
`1 → 1+N` (N = the producer-live phase rows T1/R31 proves; `N = 0` if the act
machine proves none):

| scenario | wired | % |
|---|---|---|
| current (verified) | 29 | **36%** |
| + all nine action rows, `N = 0` | 38 | **47%** |
| + all nine, `N = 1` | 39 | **48%** |
| + all nine, `N = 2` | 40 | **50%** |
| + four first-order only (`0x14 0x22 0x1A 0x1C`), `N = 0` | 33 | 41% |

`floor(100 × 38/80) = floor(47.5) = 47`; `floor(100 × 39/80) = 48`;
`floor(100 × 40/80) = 50`. The exact post-phase-16 title is whatever the T1–T5
gates prove; the arithmetic above is the honest envelope. The phase-15 T4 note
(ENGINE.md) already names the phase-16 owners for the act-seam follow-on
(FU175-L1/L5), row-03 (FU177-L1..L6), row-09 arms/0x0D (FU178-L1/L7),
row-0x19 remainder (FU179), keeper chain (FU180), row-0x17 producer (FU181) and
the R30 frontier (`0x14`/`0x22`/`0x1A`/`0x1C`/`0x16`/`0x24`/`0x20`/`0x25`,
FU182-L1..L4).

---

## 4. Ranked phase-16 movers

Ranked by (installer live in the engine **today**?) × (body coverage) ×
(reachability cost). All entries flip one NULL action `fn` → `+1` on the wired
count. Installer bytes below are fresh unless marked.

| # | row | installer (fresh) | live today? | body | gate |
|---|---|---|---|---|---|
| 1 | **0x14** | phase-5 arm `0x8D461 MOV EDX,0x14; 0x8D466 MOV EAX,[0x157A9F]; 0x8D475 CALL 0x7D9A4` (fresh; ECX=0/EBX=0, after `0x8D45C CALL 0x8CEB8`) | **yes** — the engine phase-5 arm `fifa96_match_run_phase_arm` (`match_run.c:2480`) installs `0x14` on a goal | FU-136 scatter_celebration + the goal-record release (FU-173 L2/L3) | FU184 (T2) |
| 2 | **0x22** | row-07 body `0x816D1 MOV ECX,1; 0x816D6 MOV EDX,0x22; 0x816DF CALL 0x7D9A4` (fresh; invoke-now) | **yes** (wired row-07 → `match_kick_invoke_opponent`), but the target record's producer is unported | sequence_press_event (OL-9) | FU185 (T2); OL-56 |
| 3 | **0x17** | act-4 stage 0 `0x89736 MOV EDX,0x17; 0x8973B MOV EAX,[0x1587EC]; 0x89740 CALL 0x7D9A4` (fresh) | after T1 (act machine + sit-2..4 arm) | **already ported** (`fifa96_match_action_17`, FU-181) | FU181-L1 (T1) |
| 4 | **0x0D** | row-09 arm 1 `0x80F61 MOV EDX,0xD; 0x80F6A CALL 0x7D9A4` (fresh, `ba0d000000`/`e835caffff`) | after FU178-L1 (row-09 arm 1) | **already ported** (`fifa96_action_sequence_0d_step`, FU-180 §3) | FU178-L1 (§6) |
| 5 | **0x1A** | keeper chain `FUN_00077EAC` `0x782xx` + `FUN_0009E420` `0x9E665` (*cross-cited* FU-182 §5) | after the keeper chain | OL-34 | FU186 (T3) |
| 6 | **0x1C** | `FUN_00070C08` `0x70DAF` + `FUN_00076130` `0x76291` (*cross-cited* FU-182 §5) | after the keeper chain | OL-35 | FU186 (T3) |
| 7 | **0x16** | phase-0x19 handler `0x8A096 MOV EDX,0x16; 0x8A09B MOV EAX,[0x15888F]; 0x8A0A0 CALL 0x7D9A4` (fresh; after `0x8A091 CALL 0x740A0`) | after T1 (act-3 foul producer) | FU-148 §16.2 | FU187 (T4) |
| 8 | **0x24** | phase-0x20 handler `0x8B782 MOV EDX,0x24; 0x8B78B CALL 0x7D9A4` (fresh; ECX=1; loop `ESI=0x1588A4`/`0x1590D9`) | after T1 (act-0xA) | OL-9 | FU188 (T4) |
| 9 | **03** | row-04 body `0x7E866..0x7E883` code-3/0x19 select (*cross-cited* FU-177 §1) | **yes** (L4.1 forced-decision walk) | **already ported** over FU-138 helpers | 0x6DCC8 + per-record seam (§6) |
| — | **0x20** | not located (FU182-L2) | no | OL-9 | R37/FU-189 |
| — | **0x25** | none found (only the `0x6E598` sink at `0x7E460`) | no | OL-9 | R37/FU-189 |

**Phase-row cohort (conditional, `1/35` today).** Once T1/R31's act producer gives
`fifa96_match_dispatch_phase` a live *producer* (not just a caller — phase-15 T1
already gave it the act-seam caller and counted `0x1F`), the producer-live rows
are R31/FU-183's verdict. This audit makes **no phase-row claim**: the phase
table stays `fn == NULL` for every row except `0x1F`, and a row must be proven
producer-live before it is counted (the FU-175 §7 ranking is the candidate
order: `0x1F` done, then `0x17`/act1, `0x18`/act2, `0x21`/act0xB, `0x1A`/act4,
`0x1E`/act8, `0x1D`/act7, `0x20`/act0xA, `0x19`/act3, `0x1C`/act6; act 5 /
phase `0x1B` blocked).

---

## 5. Blocked / no-move set (cannot honestly move the census)

* **0x0A** — installer `0x7CDD8` has no xrefs (*cross-cited* FU-137 §6/FU-182 §6).
* **0x15** — head mis-decoded, stub bucket (OL-14).
* **0x27 / 0x29 / 0x2C** — ported bodies, but the FU-142f census of all `0x7D9A4`
  call sites finds no `0x27`/`0x29`/`0x2C` invocation; entry unresolved
  (FU-142f/OL-48) → `-UNSUPPORTED` by decision.
* **0x2B** — dead entry: native `0x87738` is the shared row-29 epilogue RET, not
  a standalone stub (FU-142f/OL-15).
* **phase `0x16`** — the native zero/INT3 slot (`0x110794[0x16] = 0`, fresh
  §1); NOT_FOUND by design.
* **phase `0x1B` / act 5** — no static `CALL 0x888FC` passes `EAX=5` (fresh
  11-xref census, FU-175 §1/§7; FU175-L2). The pure-reset body has no invoker.
* **phase rows `0x00..0x15, 0x17..0x22`** — all NULL; producer-live set is
  T1/R31's call (§4); none is a verified mover this session.

---

## 6. Completion paths for the two ported-but-uncounted rows

Both rows already have a ported body and an install-live (or install-gated)
path, but sit NULL in the action table — the rows-27/29/2C precedent (a ported
body with no live installer stays uncounted).

### 6.1 Row 0x0D — the row-09 arm-1 installer

**Fresh installer bytes (`0x80F50`, 64 B):**

```
0x80F50 MOV EAX,0x5D ; 0x80F55 MOV EDX,EBP ; 0x80F57 MOV ECX,0x1
0x80F5C CALL 0x8F188                          ; emit event 0x5D
0x80F61 MOV EDX,0xD   (ba0d000000)
0x80F66 MOV EAX,ESI ; 0x80F68 XOR EBX,EBX ; 0x80F6A CALL 0x7D9A4 (e835caffff)
0x80F6F JMP 0x80F82 ; 0x80F71 MOV EBX,0x4 ; 0x80F76 MOV EAX,0x60 ; ...; 0x80F7D CALL 0x8F188
```

Fresh sibling (`0x80EE0`, 40 B): the **0x0C** install at `0x80EFA` (`0x80EEC
MOV ECX,1; 0x80EF1 MOV EDX,0xC; 0x80EFA CALL 0x7D9A4`, `e8a5caffff`). Both
installs sit in **row-09 arm 1** (`0x80BFB..0x80FCB`), which fires only after
row-09 stage 0 advances `+0x92` (`0x80BEB`, `[rec+0x92]++`; FU-178 §3.3/§4.1).

* **Row 0x0D handler** — fresh `get_xrefs_to 0x8251C` = **1**, from `0x110714`
  (action-table slot `0x0D`); reached only through `[rec+0x18]` dispatch.
* **Row-09 status** — the row is **wired** (stage-0-only, FU-178 §10); the
  unported arms 1/2/3 park at `mr->match09_leg_stage` (`fifa96_match_handlers.c`
  `fifa96_match_action_09`). So the installer never runs today.
* **Body** — already ported as `fifa96_action_sequence_0d_step`
  (`fifa96_action_handlers.c`, FU-180 §3); its NULL slot is documented at
  `fifa96_match_handlers.c` (`{0x0D, NULL, "FU-180 §3 …"}`).

**Completion (single dependency):** port row-09 **arm 1** (FU178-L1) so
`0x80F61` fires, then wire the `0x0D` action-table slot to
`fifa96_action_sequence_0d_step`. The same arm-1 also installs `0x0C`
(already wired → no double count). **Census: +1 action row** once arm 1 lands.
Recommended co-wire under **T5** (`feat(engine,fu189)`), body-only remainder
FU180-L8 (`0x702F8`/`0x6E598`/`0x79C50`/`0x8DCD4` sinks).

### 6.2 Row 03 — the live phase-2 `[rec+0x1C]` path

**Fresh `decompile_function 0x6DCC8`** (`void FUN_0006dcc8(int *param_1,int
*param_2,short param_3)`): reads `param_1[3]` (= `[rec+0xC]`, the metadata
pointer), indexes `[rec+0xC] + iVar5*2`, reads two bytes, scales by
`0x26`/`0x21` (sign-flipped when `[team+0x826] != 0`), and writes
`param_2[1]=0; param_2[0]=x; param_2[2]=z` — it **overwrites the row-03 target
block** `rec+0x4D/+0x51/+0x55` *before* the clamp. Fresh `get_xrefs_to 0x6DCC8`
= **5**: two `DATA` refs from the phase table (`0x11079C` = phase `[2]`,
`0x1107E8` = phase `[0x15]`) and three internal `CALL`s (`0x6DD61`, `0x6DD91`,
`0x6DF37` in the `0x6DD6C`/shared handler bodies).

* **Row 03 handler** — fresh `get_xrefs_to 0x7E1A4` = **1**, from `0x1106EC`
  (action-table slot `0x03`); table-only reference.
* **The hot path** — on the forced-decision walk (`fifa96_match_run.c:960`; the
  native L4.1 walk writes code 3 / remaps inactive→0x19, FU-177 §1) the record
  is in phase 2, so the row-03 idle core `0x7E2DD CALL dword ptr [ESI+0x1C]`
  invokes `0x110794[2] = 0x6DCC8` (fresh phase-table read §1, fresh call bytes
  `ff561c` FU-177 §3). `0x6DCC8` is unported (OL-13); `phase[2]`'s engine slot
  is `{0x02, NULL, "…phase_cell, ptr-2 lookup unported; OL-13"}`.
* **Body** — ported over the FU-138 helpers (hold/clamp_placement/
  placement_counter/phase1_clamp) + the already-ported `camera_place`
  (FU-177 §4 errata); the residual is per-record wiring + a small
  `FUN_00079C20` slot step (FU177-L2..L4).

**Completion (two dependencies):** (a) port `0x6DCC8` and its `[rec+0xC]`
source (**FU177-L1**/OL-13); (b) give the row its `[rec+0x1C]` seam — the
per-record walker / record-threaded handler signature (**FU175-L1**), the same
seam the phase cohort needs. Then wire `fifa96_match_action_03` (FU-177 §7
T3-R1). Standalone wiring without `0x6DCC8` would **drop the hot-path target
rewrite — a fake** (FU-177 §7 T3-R2 rejected). **Census: +1 action row** once
`0x6DCC8` + the per-record seam land; coupled with the phase cohort (same seam).

---

## 7. Numbered legs

1. **FU190-L1 — the post-phase-16 count is a live envelope.** The §3 table is
   arithmetic over the plan's target set, not a claim; each row's count depends
   on its T1–T5 gate. Recompute after each landing (AGENTS.md rule).
2. **FU190-L2 — the phase-row cohort.** Which of `0x17/0x18/0x21/0x1A/0x1E/
   0x1D/0x20/0x19/0x1C` (the FU-175 §7 ranks 2–9) become producer-live is
   R31/FU-183's verdict; this audit holds the phase table at `1/35`.
3. **FU190-L3 — 0x0D needs FU178-L1 only.** Body ported (FU-180 §3); the sole
   gate is row-09 arm 1 (`0x80BFB..0x80FCB`). No other dependency found this
   session.
4. **FU190-L4 — 03 needs 0x6DCC8 + FU175-L1.** Both are phase-16 T1/T5 scope;
   the row is otherwise body-complete over FU-138.
5. **FU190-L5 — 0x20/0x25 installer hunt.** Unresolved (FU182-L2); R37/FU-189's
   bounded search. If no installer exists, the rows stay legs — do not invent
   one.
6. **FU190-L6 — the `0x14`/`0x22` first-order movers.** Fresh installer bytes
   confirm both; `0x22`'s honest count is conditional on the OL-56 team-target
   producer (FU-164), so it may land code-live/producer-dormant (the FU-182 §4
   nuance).

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `read_memory` — `0x1106E0` (180 B), `0x110794` (140 B), `0x1107EC` (64 B).
* `get_xrefs_to` — `0x7D9A4` (77), `0x1106E0` (2), `0x110794` (2), `0x1107EC`
  (2), `0x6DCC8` (5), `0x8251C` (1), `0x7E1A4` (1), `0x80F61` (0 — an
  instruction, not a symbol).
* `disassemble_bytes` — `0x80F50` (64 B, the 0x0D install), `0x80EE0` (40 B,
  the 0x0C sibling), `0x8D455` (40 B, the 0x14 install), `0x816D0` (24 B, the
  0x22 install), `0x8A090` (24 B, the 0x16 install), `0x89730` (24 B, the 0x17
  act-4 install), `0x8B775` (32 B, the 0x24 install).
* `decompile_function` — `0x6DCC8` (`FUN_0006DCC8`).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(3900..4176, the two tables + both dispatch resolvers),
`include/fifa96_engine/fifa96_match_handlers.h` (33..79). Docs cross-cited:
FU-96, FU-136/137/138/142f/148/149/151/164/165/169/173/174/175/176/177/178/179/
180/181/182; the phase-16 plan; AGENTS.md census rule. No writes beyond this
slice file and the workspace report.
