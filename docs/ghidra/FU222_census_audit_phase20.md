# FU-222 — census/audit + the phase-20 honest ceiling + the sit-8-tail double yield (phase-20 wave R70)

Provenance: M2 phase-20 wave **R70**, 2026-10-13, read-only. Authoritative
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

Scope (R70 brief): (1) independently recompute the census against the native
binary — the action table `0x1106E0` + phase table `0x110794` — and cross-check
the port's claimed `53/80`; (2) the honest phase-20 ceiling with the phase-19
landings in place, and each attainable/unattainable NULL row's reason; (3) rank
the residual frontier legs (`FU207-L2 … FU194-L2`) by census yield ×
tractability; (4) the sit-8 tail's exact census yield; (5) the
README/ENGINE/AGENTS consistency check. This slice audits the phase-19 landing
(`FU207`…`FU214` + the `30e57d8` period-end spine) against the phase-18 audit
(`FU-206`).

---

## 0. One-line answer

**The census recomputes exactly to `53/80 = 66%` (`32 action + 21 phase`;
dispatch `26 UNSUP / 53 OK / 1 NOTF`, the `0x16` INT3 slot the NOTF) — the port
agrees with the native tables, which are byte-identical to FU-206/FU-214; the
honest phase-20 ceiling is `55/80 = 68%` (`+` phase `0x10` via FU214-L4 and
action `0x20` via FU210-L3 — **two** sit-8-tail movers, not one), rising to
`56/80 = 70%` only if the deliberately-pinned phase-`0x1E` de-compression
(FU212-L1) is lifted; the frontier ranks FU214-L4 first (cheapest +1), FU210-L3
second, FU212-L1 third (and every other listed leg is a pure producer/audio leg
with zero census yield); all four census surfaces agree.**

---

## 1. Fresh census machinery — the two native tables

`include/fifa96_engine/fifa96_match_handlers.h:34..39` declares the census
domain: action `FIFA96_MATCH_ACTION_ROWS 45` (`0x1106E0[45]`, codes `0x00..0x2C`),
phase `FIFA96_MATCH_PHASE_ROWS 35` (`0x110794[35]`, phases `0x00..0x22`), the
INT3 slot `FIFA96_MATCH_PHASE_INT3_SLOT 0x16` — **80 rows**.

**Action table `0x1106E0`** — fresh `read_memory` (180 B = 45 dwords) returns the
full table byte-for-byte identical to FU-206 §1 / FU-214 §1:
`00:07DB10 01:07DBC0 02:07DFCC 03:07E1A4 04:07E7C8 05:07F194 06:0801B4
07:0814B0 08:081068 09:080A00 0A:081738 0B:081908 0C:081C90 0D:08251C
0E:082710 0F:082AD0 10:0855F0 11:085DE4 12:083D68 13:084B00 14:08784C
15:087CD0 16:084630 17:084730 18:0849B0 19:0746E4 1A:07662C 1B:076D28
1C:077728 1D:074EB0 1E:07550C 1F:076380 20:084EEC 21:085214 22:08539C
23:082F84 24:086510 25:0880CC 26:0866F4 27:086820 28:0870E8 29:0874E4
2A:086A34 2B:087738 2C:084598`. **All 45 slots non-NULL.**

**Phase table `0x110794`** — fresh `read_memory` (140 B = 35 dwords), byte-for-
byte identical to FU-206 §1 / FU-214 §1:
`[0]=06DE34 [1]=06E1D0 [2]=06DCC8 [3]=06DE44 [4]=06DE44 [5]=06E05C [6]=06DD9C
[7]=06DE44 [8]=06DD6C [9]=06DD6C [0A]=06DE34 [0B]=06DE34 [0C]=06DF4C
[0D]=06DE34 [0E]=06DE34 [0F]=06DE34 [10]=06E004 [11]=06E1C8 [12]=06E1D0
[13]=06E244 [14]=06E244 [15]=06DCC8 [16]=00000000 (INT3) [17]=088DC8 [18]=08922C
[19]=089FA4 [1A]=089620 [1B]=0890EC [1C]=089110 [1D]=089868 [1E]=08A798
[1F]=088F4C [20]=08B688 [21]=08B874 [22]=08B900`. **34 non-NULL + the single
`[0x16] = 0` INT3 slot.**

* **Installer `0x7D9A4`** — fresh `get_xrefs_to` = **77** call sites
  (byte-identical set to FU-174/FU-182/FU-190/FU-198/FU-206; the four
  phase-0x1D arm-0 sites `0x8990A`/`0x8994C`/`0x899AB`/`0x899CD` present
  first-hand).
* **Act table `0x1107EC`** — fresh `read_memory` (56 B):
  `[0]=0 [1]=088DC8 [2]=08922C [3]=089FA4 [4]=089620 [5]=0890EC [6]=089110
  [7]=089868 [8]=08A798 [9]=088F4C [A]=08B688 [B]=08B874 [C]=08B900`, then the
  unrelated `0x006007FF`. Base is exactly `0x110794 + 0x16*4`, so **act `a` →
  phase `0x16+a`** — byte-identical to FU-206 §1 / FU-214 §1.

**Audit verdict: the native count machinery is provable as stated and unchanged
since FU-206/FU-214.** No table byte, xref count or slot moved.

### 1.1 Engine-side count (fresh first-hand source read)

`src/fifa96_engine/fifa96_match_handlers.c:5086..5175` (action table) and
`:5177..5248` (phase table), read first-hand this session and enumerated slot by
slot:

* **Action table — 32 non-NULL `fn`s**: `00 01 02 04 05 06 07 08 09 0B 0C 0E 0F
  10 11 12 13 14 16 18 19 1B 1C 1D 1E 1F 21 23 24 26 28 2A`. **13 NULL**:
  `03 0A 0D 15 17 1A 20 22 25 27 29 2B 2C`.
* **Phase table — 21 non-NULL `fn`s**: `00 02 03 04 05 06 07 08 09 0A 0C 0F 15
  17 18 19 1D 1F 20 21 22`. **14 NULL** (ex-INT3): `01 0B 0D 0E 10 11 12 13 14
  1A 1B 1C 1E`. Plus the INT3 `0x16` = **1 NOTF**.

`tests/test_engine_m2.c:3020` (and the mirror at `:3387`) pins the shape
first-hand: `assert(action_wired == 32u && phase_wired == 21u);  /* 53/80 = 66%
(T2: +phase 0x1D; T4: +phase 0x22) */`. Dispatch arithmetic: **53 OK + 26 UNSUP
(13 action + 13 phase) + 1 NOTF (phase `0x16`) = 80**.

**Audit verdict: the `53/80 = 66%` claim is provable as stated, and the port
count mirrors the native domain exactly.** The native tables carry 79 non-NULL
slots + the 1 INT3 (a *possibility* domain); the census numerator is the
**wired** rows (53), so "native count vs `53/80`" is not a like-for-like
comparison — there is **no discrepancy**: the port's `fn != NULL` set is exactly
the 32 + 21 enumerated above. `floor(100 × 53/80) = floor(66.25) = 66`.

---

## 2. The verified current census (53/80 = 66%)

| | count |
|---|---|
| action rows wired (non-NULL `fn`) | **32** |
| action rows UNSUP (NULL) | 13 |
| phase rows wired | **21** |
| phase rows UNSUP (NULL, ex-`0x16`) | 13 |
| phase `0x16` (INT3 slot) | 1 NOTF |
| **wired / total** | **53 / 80 → 66%** |

Dispatch: `53 OK / 26 UNSUP / 1 NOTF` (sum 80; action 45 = 32 OK + 13 UNSUP;
phase 35 = 21 OK + 13 UNSUP + 1 NOTF).

The 14 NULL phase rows (the phase-side movers, ex-INT3):
`01 0B 0D 0E 10 11 12 13 14 1A 1B 1C 1E`.
The 13 NULL action rows (the action-side movers):
`03 0A 0D 15 17 1A 20 22 25 27 29 2B 2C`.

Since FU-206 (`48/80`) the phase-17/18/19 waves landed rows, all accounted:
phase-17 T3's per-record cohort (`0x00 03 04 05 06 07 08 09 0A 0C 0F 15`), T1
phase `0x02`, phase-18 T3 phase `0x20` + action `0x24`, phase-18 T4 phase
`0x17`, phase-19 T2 phase `0x1D` and T4 phase `0x22`.

---

## 3. The honest phase-20 ceiling

The AGENTS.md rule (§Completion percentage): the census counts a row only when a
**ported path installs AND dispatches** — a `NULL → fn` flip with no live
producer is **not** counted (the rows-`27`/`29`/`2C` precedent). The phase-19
landings made the **phase-`0x1D` slot counted and dispatched** (FU214 §8),
which is the hinge for this section.

Fresh first-hand proof of the hinge — the phase-`0x1D` **stage-0 arm** (native
`0x898B4..0x898DD`, fresh `disassemble_bytes 0x898B4`, 46 B) writes phase `0x10`
on the live dispatch path:

```
0x898B4  XOR CL,CL
0x898B6  MOV EAX,[0x157A4A]        ; the phase dword
0x898BB  MOV byte[0x15882A],CL
0x898C1  SAR EAX,0x18              ; EAX = phase
0x898C4  MOV byte[0x157AB1],CL
0x898CA  CMP EAX,0x10              ; if state.phase != 0x10
0x898CD  JZ  0x898E2
0x898CF  MOV EDX,[0x157AAC]
0x898D5  MOV EAX,0x10
0x898DD  CALL 0x740A0              ; FUN_000740A0(0x10)  <-- the phase-0x10 write
```

and the port mirrors it at `fifa96_match_run.c:6090..6091`
(`if (mr->state.phase != 0x10u) (void)match_run_write_phase(mr, 0x10u);`). The
sit-8 producer census is matched: situation `8` = `{phase NONE, act 7, stage 0}`
(`fifa96_action_handlers.c:1142`), so `match_run_situation_side` dispatches
`fifa96_match_act_dispatch(mr, 7, 0, 0)` (`fifa96_match_run.c:2889`) and the
clock-tail `fifa96_match_act_tick` maps act 7 → phase `0x1D` and runs the body
(`fifa96_match_act.c:55..65`) **from stage 0** — arm 0 fires.

`match_run_write_phase` (`fifa96_match_run.c:2508..2528`) runs the per-record
walk **only when the phase row is non-NULL**
(`if (phase < FIFA96_MATCH_PHASE_ROWS && fifa96_match_phase_table[phase].fn !=
NULL)`). The phase-`0x10` row is currently NULL (`handlers.c:5210`), so the
write is a structural no-op today; registering it dispatches the walk → the
row counts.

### 3.1 The attainable rows (each with its evidence)

| row | kind | delta | why it is attainable now |
|---|---|---|---|
| **phase `0x10`** | phase row | **+1** | the writer is live — the phase-`0x1D` stage-0 arm (`0x898DD` `FUN_000740A0(0x10)`; port `match_run.c:6091`) — and the row is registerable (`handlers.c:5210` NULL → `fifa96_match_phase_10`, body ported at `:4806`). Tape-inert: phase `0x1D` is extra-time-dormant on M2, so the write never fires on the tape, and the body writes only the render-invisible target triple. **This closes FU214-L4.** |
| **action `0x20`** | action row | **+1** | body + coupled installer already ported (phase-19 T3); the sole blocker is the actor-cell producer `[0x1587CC+side*4] → [0x1587D4]` (fresh `disassemble_bytes 0x8AE3C`: `0x8AE4B MOV EAX,[EAX*0x4+0x1587CC]; 0x8AE54 MOV [0x1587D4],EAX`), unported (FU210-L3). |
| **phase `0x13` / `0x14`** | phase rows | **+2** | in the engine reach set; the blocker is the unported `FUN_0008D098` case-`0x13/0x14` PRNG scatter producer (`0x10F364`/`0x10F368`, FU192-L2). **Out of this brief's frontier** — a phase-17 leg, counted here only for the wide bound. |
| **phase `0x1E`** | phase row | **+1** | body ported; the blocker is deliberate — the engine pins the *compressed* act-8 model (`tests/test_engine_match_frame.c::test_phase_1E_compressed_model_pin`; `test_engine_match_act.c:599`), so de-compressing it (FU212-L1 / FU194-L2) is the only path. Attainable only by lifting the pin. |

### 3.2 The unattainable rows (22; reason per the AGENTS.md `fn != NULL` rule)

Action side — **13 NULL minus action `0x20` = 12**:

* `03` — ported body, but the live phase-2 hot path is **tape-visible**: wiring
  the slot moved M2 at transcribed frame 29 (controller R7; FU177-L1). Uncounted
  until the tape chain is resolved.
* `0A` / `15` — installer `0x7CDD8` has **no xrefs**; head mis-decoded stub
  (FU-137 §6, OL-14). No producer exists.
* `0D` — body ported; its only installer `0x80F61` sits inside row-09 **arm 1**
  (unported) behind unported producers (FU178-L1).
* `17` — body ported; its only installer (act-4 stage 0 `0x89740`) is behind the
  sit-2..4 formation-order arm, which FU-201 proves is a **permutation residual**
  (residual 0) — un-gating it is a fake producer (FU181-L1/FU201).
* `1A` — body ported; its only installer is the unported `FUN_00077EAC` CPU
  ladder (FU186-L5).
* `22` — body ported; its only installer (row-07 `0x816DF` invoke-now) is
  **structurally ungated**: the OL-65 angle inputs are staged 0, so
  `out.opponent_invoke` never fires (FU185-L6).
* `25` — **no installer exists** (bounded `MOV EDX/DX,0x25` scan, FU189-L2).
* `27` / `29` / `2C` — ported bodies, but the FU-142f census of all 77
  `0x7D9A4` call sites finds **no invocation** (FU-142f/OL-48).
* `2B` — **dead entry**: native `0x87738` is the shared row-29 epilogue RET
  (FU-142f/OL-15).

Phase side — **14 NULL minus phase `0x10`/`0x13`/`0x14`/`0x1E` = 10**:

* `01` / `11` / `12` — **double-wire excluded** (R10): the formation placement is
  already realized by `fifa96_match_entities_seed_formation`; wiring the phase row
  would double-run it (FU192-L6/L7).
* `0B` — its sole writer sits **inside the unwired row-`0x1A` body** (FU-206 §4
  #12; `handlers.c:5200`).
* `0D` — the sole argument is situation 7, and **no live caller dispatches
  situation 7** (final-review C1; owner FU-146).
* `0E` — **no live writer** (the reach set excludes it; FU-191 §10).
* `16` — the native zero/INT3 slot (fresh §1); **NOTF by design**, never counts.
* `1A` — body ported; **no act-4 producer** (FU-193 §3/FU197; FU201).
* `1B` — body ported; **no invoker**: a fresh `FUN_000888FC` census finds no
  `EAX=5` site and no phase-`0x1B` writer (FU195-L1).
* `1C` — body ported; **no act-6 producer** (the offside machine drives it; wiring
  it double-runs) (FU193-L4).

### 3.3 The ceiling (three scopes; never hand-set)

| scope | wired | % |
|---|---|---|
| current (verified §1/§2) | **53** | **66%** |
| + the sit-8 tail (`0x10` + action `0x20`) — **the phase-20 target** | **55** | **68%** |
| + phase `0x1E` (lift the compressed-model pin, FU212-L1) | 56 | 70% |
| + phase `0x13`/`0x14` (FU192-L2, out-of-frontier) | 58 | 72% |

`floor(100 × 55/80) = floor(68.75) = 68`. **The phase-20 honest ceiling is
`55/80 = 68%`**, reachable by two independent registrations/producer ports with
no tape re-pin (both rows are extra-time-dormant on M2). The `66 → 68` step is
the whole of the reachable frontier unless the phase-`0x1E` pin is deliberately
lifted.

---

## 4. The sit-8 tail's yield — the double `+1`

The situation-8 route (`global_157ac2 == 4` → situation 8 → act 7 → phase
`0x1D`) has **two** remaining unwired downstream *table rows*, not one:

1. **action `0x20`** — installed by the phase-`0x1D` arm 0 (`0x899A2`
   `FUN_0007D9A4([0x1587D4], 0x20, 0, 0)`); target cell NONE on the live path →
   **FU210-L3** (the actor-cell producer). Body/installer already ported.
2. **phase `0x10`** — written by the same arm 0 (`0x898DD` `FUN_000740A0(0x10)`;
   port `match_run.c:6091`); row NULL → **FU214-L4**. Already (re)attainable.

The phase-`0x1D` arm 0 also installs **action `0x19`** (`0x89903/0x8990A`) and the
ball row **`0x1F`** (`0x899CD`) — both already counted (phase-15 T2), so they add
no row.

**Verdict: `FU210-L3` is NOT the only sit-8-tail `+1`.** The sit-8 tail yields
exactly **`+2`** — `{action 0x20 (FU210-L3), phase 0x10 (FU214-L4)}` — and the
phase-`0x10` mover is the **cheaper** of the two (a registration + dispatch test,
versus a producer port). No other sit-8-tail work adds a row (act-8 / phase
`0x1E` is the *compressed restart fallback*, not the act-7 tail; it is ranked
separately in §5).

---

## 5. Ranked remaining legs (the ENGINE.md frontier) — census yield × tractability

Each leg below is a frozen owner from `docs/ENGINE.md`; the yield column is the
**census delta** if the leg lands honestly, the tractability the effort.

| rank | leg | row / output | census yield | tractability |
|---|---|---|---|---|
| **1** | **FU214-L4** | phase `0x10` | **+1** | **HIGH** — register the ported body on a live-writer fixture (the phase-`0x1D` arm-0 write already exists); tape-inert (§3.1). This is the single cheapest honest mover in the whole frontier. |
| **2** | **FU210-L3** | action `0x20` | **+1** | **MEDIUM** — port the native actor-cell producer `[0x1587CC+side*4] → [0x1587D4]` (fresh `0x8AE4B`) + the `[0x1587CC]` table producer it reads; body/installer already landed (T3). FU210-L5 (the fixtured installer) is a rider, not a mover. |
| **3** | **FU212-L1 / FU194-L2** | phase `0x1E` | **+1** | **LOW** — requires lifting the deliberate compressed-act-8 pin (`test_phase_1E_compressed_model_pin`); FU-212 §6 proves the sweep render-invisible + dormant, but the pin is an intentional bound. |
| — | **FU214-L1** | (cohort producer) | 0 | PARTLY RESOLVED — the OL-85/`[0x157AC2]` producer landed (T1/T2); the only residual row it gates is action `0x20` → **FU210-L3**. |
| — | **FU207-L2** | (OL-85 front-end `0x389A4`) | **0** | pure producer leg — the in-match cell/writer landed (FU-213); no new table row. |
| — | **FU208-L1 / FU208-L8 / FU209-L1 / FU214-L2** | (sit-8 bit-8 range word + session gate + act-7 inert arm) | **0** | pure producer/co-condition legs — phase `0x1D` is already counted via the `global_157ac2 == 4` arm; the bit-8 disjunct stays a located-writer gap. |
| — | **FU210-L5** | (row-`0x20` installer) | **0** | fixtured install — no count without FU210-L3's live target. |
| — | **FU211-L2 / FU211-L3** | (word `[0x15881A]` cell + audio ring-push) | **0** | pure gate-cell/audio legs — phase `0x22` is already counted and tape-neutral. |
| — | **FU213-L3** | (end-of-match word `[0x158822]` / `FUN_0008BAF0`) | **0** | pure sink leg — no table row. |

**Which single leg unlocks the most rows?** **FU214-L4** unlocks one row for the
least work, but by *yield* it ties FU210-L3 and FU212-L1 (all `+1`); no single
leg unlocks more than **one** row. **Pure legs (no census move):** FU207-L2,
FU208-L1/L8, FU209-L1, FU210-L5, FU211-L2/L3, FU213-L3, FU214-L1 (residual) and
FU214-L2 — eight-plus legs that advance fidelity/reachability without moving the
`fn != NULL` count. Re-ranking delta vs FU-214 §5: the phase-19 movers are gone;
the frontier's top is now a **clean, cheap registration** (phase `0x10`), then a
**producer port** (action `0x20`), then a **deliberately-bounded de-compression**
(phase `0x1E`).

---

## 6. Consistency check

All four census surfaces agree this session (fresh first-hand read):

* `README.md:1` — title `# FIFA 96 (DOS, 1995) — Reverse Engineering (66%)`.
* `docs/ENGINE.md:8` and `:268` — `53/80 dispatch rows wired (32 action + …)`;
  the phase-19 dispatch lines (`:1170`, `:1197`, `:1226`) — `26 UNSUP / 53 OK /
  1 NOTF` (sum 80); `floor(100 × 53/80) = 66`.
* `AGENTS.md:90` — `(currently 53/80 → 66%)`.
* `docs/PROGRESS.md:1325` — `53/80 = 66% (32 action + 21 phase; dispatch
  `26 UNSUP / 53 OK / 1 NOTF`, sum 80)`.
* `tests/test_engine_m2.c:3020` + `:3387` — `action_wired == 32u && phase_wired
  == 21u`.

**Verdict: consistent.** No discrepancy. (`docs/PROGRESS.md:1377..1379` also
names the `0x16` INT3 slot as the sole NOTF — matching §2.)

**Stale-evidence finding (non-census).** Two censused *comments* predate the
phase-19 phase-`0x1D` landing and are now factually superseded (they describe a
blocker that no longer blocks a *registration*):

* `handlers.c:5211` (phase-`0x10` evidence) still says "phase 0x10 is never
  written by a live engine source … the sole writer sits inside the **unwired**
  row 0x1D body". Row `0x1D` is wired/counted (phase-19 T2), so the writer is
  live — this is exactly what makes phase `0x10` a `+1` (§3.1). The slot stays
  NULL for a **pin/count** reason now, not a "never-written" reason.
* `tests/test_engine_m2.c:3045` (`assert(fifa96_match_phase_table[0x10].fn ==
  NULL); /* never written (body ported) */`) and its mirrors (`:3310`, `:3366`)
  carry the same stale rationale.

Neither is a census error (the row is correctly NULL/uncounted) — they are
**evidence-string drift** for the phase-20 phase-`0x10` landing to correct when
it flips the slot. Recorded, not rewritten (AGENTS.md errata rule).

---

## 7. Numbered open legs

1. **FU222-L1 — the phase-20 honest ceiling is `55/80 = 68%`, reached only by
   registering phase `0x10` (FU214-L4) and porting the actor-cell producer
   (FU210-L3).** Do not `NULL → fn` flip any other NULL row for a count; the 22
   unattainable rows carry the reasons in §3.2. The `53/80` baseline stands until
   a landing installs **and** dispatches.
2. **FU222-L2 — the phase-`0x10` mover is the cheapest honest `+1` and closes
   FU214-L4.** The writer (`0x898DD` `FUN_000740A0(0x10)`, port
   `match_run.c:6091`) is live via the counted phase-`0x1D` arm 0; registering
   `handlers.c:5210` (NULL → `fifa96_match_phase_10`) makes
   `match_run_write_phase`'s walk gate open (`:2522`) and dispatch the row. The
   pin at `test_engine_m2.c:3045` must flip; the phase-`0x10` evidence string is
   stale (§6). Owner: FU214-L4 / phase-19 phase-`0x1D` landing.
3. **FU222-L3 — action `0x20` is gated solely by the actor-cell producer
   (FU210-L3), not by any remaining installer work.** Body + coupled installer
   are ported (T3); fresh `0x8AE4B` confirms the `[0x1587CC+side*4] → [0x1587D4]`
   bind is the missing producer. Do not register the row until the cell can be
   non-NONE on a live path (the rows-27/29/2C precedent). Owner: FU210-L3 (agent
   R65/FU-217).
4. **FU222-L4 — the sit-8 tail is a double `+1`, not a single.** `FU210-L3` is
   the *second*-cheapest mover; `FU214-L4` is cheaper. Do not conflate them: they
   are distinct rows (`action 0x20` vs `phase 0x10`) written by the same arm 0.
   Owner: the sit-8 tail landing.
5. **FU222-L5 — phase `0x1E` remains the sole deliberately-bounded mover.**
   `+1` only by lifting the compressed-act-8 pin (FU212-L1); the row is
   render-invisible/dormant (FU-212 §6) but the pin is intentional. Owner:
   FU212-L1 / FU194-L2.
6. **FU222-L6 — the `0x10` evidence-string/test-pin drift must be corrected in
   the same commit that flips the slot, not as a silent rewire.** The phase-19
   landing updated the phase-`0x1D` evidence but left the phase-`0x10` comment
   (`handlers.c:5211`) and the `test_engine_m2.c:3045/3310/3366` "never written"
   rationale stale. Owner: the phase-`0x10` landing.
7. **FU222-L7 — the wider bound (`58/80 = 72%`) is out of this frontier.**
   Phase `0x13`/`0x14` (`+2`) require the unported `FUN_0008D098` case-`0x13/0x14`
   PRNG scatter producer (FU192-L2); recorded as the bound only, not a phase-20
   target. Owner: FU192-L2.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, 2706
  functions, 15239 symbols, min `0x10000`, max `.image::00174a2a`, `.image`
  overlay.
* `read_memory` — `0x1106E0` (180 B), `0x110794` (140 B), `0x1107EC` (56 B).
* `get_xrefs_to` — `0x7D9A4` (**77**).
* `disassemble_bytes` — `0x8AE3C` (40 B, the sit-8 actor-cell bind `0x8AE4B`),
  `0x898B4` (46 B, the phase-`0x1D` arm-0 phase-`0x10` write `0x898DD`).
* M1/M2 goldens run first-hand: `./build/test_engine_m1 | cmp - tests/golden/
  engine/m1-frames.txt` (clean, `09b726b7…`, 691 lines); `./build/test_engine_m2
  | cmp - tests/golden/engine/m2-frames.txt` (clean, `441415fc…`, 792 lines).

Engine/loader source read first-hand: `include/fifa96_engine/fifa96_match_handlers.h`
(34..39, the census domain); `src/fifa96_engine/fifa96_match_handlers.c`
(5086..5175 the action table incl. the NULL `0x20` at `:5149`; 5177..5248 the
phase table incl. the NULL `0x10`/`0x1E` at `:5210`/`:5238` and the counted
`0x1D`/`0x22` at `:5236`/`:5246`; 4806..4834 the phase-`0x10` body);
`src/fifa96_engine/fifa96_match_run.c` (2488..2498 the per-record walk;
2508..2528 `match_run_write_phase` incl. the `fn != NULL` walk gate at `:2522`;
2580..2684 `phase_drive`; 2847..2898 `match_run_situation_side`; 2034/2156 the
`actor_1587d4` reset; 6082..6145 the phase-`0x1D` body incl. the phase-`0x10`
write at `:6090` and the null actor install at `:6110`); 
`src/fifa96_engine/fifa96_match_act.c` (38..65 the act dispatch/tick seam);
`src/fifa96_loader/fifa96_action_handlers.c` (1129..1147 the situation table,
sit 8 = `{NONE, 7, 0}` at `:1142`); `tests/test_engine_m2.c` (3003..3057 the
census riders; 3020/3387 the `32u && 21u` pins);
`tests/test_engine_match_act.c` (220/506/599/798); `tests/test_engine_match_frame.c`
(677, 3535 the phase-`0x1E` pin).

Docs cross-cited: FU-137, FU-142b/f, FU-146, FU-174, FU-175, FU-177, FU-178,
FU-181, FU-182, FU-185, FU-186, FU-188, FU-189, FU-190, FU-191, FU-192, FU-193,
FU-194, FU-195, FU-198, FU-201, FU-202, FU-203, **FU-206** (the phase-18 audit),
FU-207, FU-208, FU-209, **FU-210** (action row `0x20`), **FU-211** (phase `0x22`),
**FU-212** (phase `0x1E`), **FU-213** (extra-time period clock), **FU-214** (the
phase-19 audit); AGENTS.md census rule.

No census change and no engine change: `53/80 = 66%` and the README/ENGINE/
PROGRESS titles stand unchanged. No writes beyond this slice file; no
rename/label/comment/function/script/project save; `/FIFA96.EXE`, the ISO and
`game/hdd/` untouched; no git commands.
