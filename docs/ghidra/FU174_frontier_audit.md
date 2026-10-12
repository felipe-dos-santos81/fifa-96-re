# FU-174 — frontier audit (phase-14 wave R22): live-install gaps + the wired census

Provenance: M2 phase-14 wave **R22**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument). Every xref count, table byte and call byte below is a fresh
`get_xrefs_to` / `read_memory` / `disassemble_bytes` of this session unless
marked *cross-cited* (then the frozen FU doc is named). Engine source read
first-hand (`fifa96_match_handlers.c`, `fifa96_match_run.c`,
`fifa96_loader/fifa96_outfield.c`, `fifa96_action_handlers.c`). No writes:
no rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; no git commands. The alias traps are applied (`dword[x]>>16` =
word at `x+2`, `>>24` = byte at `x+3`).

Scope: (1) the phase-14 frontier — which exercised-but-unwired paths break or
degrade the live loop once the continuity landings (T1 row-0x0E else arm +
row-0x5E installer, T2 sustained follow, T3 post-goal continuation) are in,
and the cheapest next wiring, ranked for phase-15; (2) the census audit —
which of the 80 dispatch rows would move the wired count next, and a
re-verification of the count machinery with fresh in-session evidence.

---

## 1. Fresh census machinery — the "22/80 wired" claim

The engine's census counts **non-NULL `fn` rows of the two native dispatch
tables** (`include/fifa96_engine/fifa96_match_handlers.h`: action
`FIFA96_MATCH_ACTION_ROWS 45`, phase `FIFA96_MATCH_PHASE_ROWS 35` — 80 rows).

Fresh native re-verification this session:

* **Action table `0x1106E0`** — fresh `read_memory` (180 B = 45 dwords) returns
  the full table, rows `0x00..0x2C`; the table ends at `0x110794` exactly
  (45 × 4 = 0xB4), where the phase table begins (contiguity first-hand).
  Spot values: `[0]=0x7DB10`, `[4]=0x7E7C8`, `[5]=0x7F194`, `[9]=0x80A00`,
  `[0x0D]=0x8251C`, `[0x0E]=0x82710`, `[0x17]=0x84730`, `[0x1B]=0x76D28`,
  `[0x1C]=0x77728`, `[0x1D]=0x74EB0`, `[0x1E]=0x7550C`, `[0x1F]=0x76380`,
  `[0x2B]=0x87738`, `[0x2C]=0x84598`.
* **Phase table `0x110794`** — fresh `read_memory` (140 B = 35 dwords),
  rows `0x00..0x22`; `[0x16] = 0` (the native zero/INT3 slot) and the tail
  handlers `[0x17]=0x88DC8, [0x18]=0x8922C, [0x19]=0x89FA4, [0x1A]=0x89620,
  [0x1B]=0x890EC, [0x1C]=0x89110, [0x1D]=0x89868, [0x1E]=0x8A798,
  [0x1F]=0x88F4C, [0x20]=0x8B688, [0x21]=0x8B874, [0x22]=0x8B900`; the table
  ends at `0x110820`, where a word structure starts (*cross-cited*
  FU-164 §2.2 item 3).
* **Installer reader `0x7D9A4`** — fresh `get_xrefs_to` = **77** call sites
  (matches FU-164 §1's fresh count; the dispatcher stores `byte[rec+0x91] = DX`
  and `dword[rec+0x18] = 0x1106E0[code]`).

Engine-side count (source, first-hand): the action table has **22 non-NULL
`fn`s** (`00,01,02,04,05,06,07,08,0E,0F,10,11,12,13,18,1D,1E,21,23,26,28,2A`)
and **23 NULL**; the phase table has **35 NULL** (including the INT3 slot).
Dispatch arithmetic: 22 OK + 57 UNSUP (23 action + 34 phase) + 1 NOTF (phase
`0x16`) = 80. The README title `27%` = `floor(100 × 22/80)`.

**Audit verdict: the 22/80 claim is provable as stated.** Discrepancy notes in
§7.

---

## 2. The first-order frontier — live-installed codes with no handler

The engine's installers write `e->code` through
`fifa96_match_entities_install` (`fifa96_match_entities.c:73..92`, the native
`0x7D9A4` subset: the `active == 0 && code == 3 -> 0x19` remap at `0x7DA26`,
the `e->code = code` store at `0x7DA63`), and every later dispatch of that
record runs `fifa96_match_dispatch_action` — which returns
`-FIFA96_ERR_UNSUPPORTED` for a NULL row. The record's body never runs: no
stage advance, no timer, no target — it is inert until something re-installs a
wired code. Five **unwired** codes are installed by **already-wired live
paths** today (fresh installer bytes this session):

| code | native installer (fresh) | engine live installer (source) | row state | cost |
|---|---|---|---|---|
| **3** | row-04 body `0x7E866..0x7E883`: `[EBP+0x8D] != 0 -> EAX=3` then `JMP 0x7F133` (the shared install tail) | the wired row-04 apply loop (`handlers.c:2555..2568`) + the L4.1 walk (row-04 `out.installs`) | row 03 partial — FU-138 `locomotion_hold`/`clamp_placement`/`placement_counter`/`phase1_clamp` ported+tested; OL-19 (3 pieces) | medium |
| **9** | input-row handler `0x7CF54`: `0x7CFA9 MOV EDX,0x9; 0x7CFB2 CALL 0x7D9A4` (ECX=1 invoke); **plus** row-06 body `0x807A9` arm — `fifa96_action_handlers.c:2046 install = 9` | the input-row seam (`match_run.c:718..729`) and the wired row-06 `out.install` drain (`handlers.c:2921`) | row 09 partial — FU-136 "not ported (partial) … arms unported"; the 4-arm table `0x809F0` and the stage-0 sketch are FU-138/FU-142 quoted, not ported | high |
| **0x0B** | input-row handler `0x7CE38`: `0x7CE8A MOV EDX,0xB` (invoke); **plus** row-04 body `0x7ED61 MOV EDX,0xB; 0x7ED6A CALL` | the input-row seam (`match_run.c:693..700`) and the wired row-04 score arm | row 0B partial — `fifa96_action_sequence_duel_event` ported (`fifa96_action_handlers.c:1338`) | medium-high |
| **0x19** | row-04 body `0x7E86E..0x7E883` (`[EBP+0x8D]==0 -> EAX=0x19`, JMP 0x7F133) and `0x7E8C1 MOV EDX,0x19; 0x7E8CA CALL` | the wired row-04 apply loop | row 19 partial — FU-140 `keeper_hold_*` helpers ported+tested; OL-33 predictor/guard arms | medium-high |
| **0x1F** | phase-6 arm: `0x8D4A2` head, `CMP EAX,EDX; JNZ 0x8D558`; `0x8D558 MOV EDX,0x1F; CALL 0x7D9A4` (the non-controlled team's keeper, EAX=EBP=team base) | the wired phase arm `match_run.c:2445` (phase 6 arms its own keeper 0x1F) | row 1F partial — FU-140 `dive_target`/`arm_step`/`arm_camera`/`input_decide` tested; OL-36 (row body + install exec) | medium |

Second-order (installer exists but is not live in the engine today):

| code | native installer (fresh/cross-cited) | first blocker |
|---|---|---|
| 0x0D | row-09 body `0x80F61 MOV EDX,0xD; 0x80F6A CALL 0x7D9A4` (fresh) | row 09 unwired |
| 0x17 | act-4 stage 0 `0x89736 MOV EDX,0x17; 0x89740 CALL` (fresh) — cross-cited FU-165 §3.6 | the act machine (`FUN_000888FC` + the `0x8B669` tick) is unported; installer fires only under T3-R2 |
| 0x16 | phase-0x19 handler `0x89FA4` (`0x7D9A4(0x16, [0x15888F])` — *cross-cited* FU-148 §16.2; the handler head is fresh, the `MOV EDX,0x16` sits deeper in the body — leg FU174-L2) | the foul chain has no live producer; act 3 is unported |

Held by decision (cannot honestly move the census): rows **27/29/2C** (bodies
ported, no installer invocation — FU-142f census) and the dead entry **2B**.

---

## 3. What phase-14's continuity exercises (T1/T2/T3) and what it breaks

**T1 — row-0x0E stage-2 else arm (FU160-L4) + row-0x5E live installer.**
Both are inside already-wired machinery (row 0x0E is wired since phase-13 T3;
the row-0x5E install rides `fifa96_match_run_record_anim_install` /
`_successor`). No census movement. What T1 can surface: the else arm's
slot-dir/camera-follow writes feed the same camera cell the T2 follow reads —
a wrong gate here degrades T2 rather than dispatching a new row.

**T2 — sustained follow (R16/R19).** The top-ranked pan arm is **row-0x0E arm
2**, re-verified fresh: row 0D RETs at `0x826FE`; row 0x0E's prologue starts
`0x82710`, the phase-2 gate is `0x82723`, the 4-arm table `0x82700` (fresh
`read_memory` = `{0x8275D, 0x827B1, 0x82806, 0x82AB8}`), and the `0x71C94`
call at `0x82A6C` (`e8 23 f2 fe ff`) sits in arm 2 (event height
`(rand & 0x3F) + [0x157750]`, fresh `0x82A56`). **The arm is inside a wired
row → FU-168 cannot move the census.** What T2 changes in the loop: once the
follow is sustained in open play, the *missing* pan producers become visible
gaps — the row-05 stage-0 tail event on the live claim frame (FU-164 rank 2),
the keeper dive/rush events (rows 1F/1B/1C), and the armer angle arm once the
camera can travel out of bounds (`0x71B8A`; gate `|camX| > 0x6C0 || |camZ| >
0xAB0`, FU-164 §3.4).

**T3 — post-goal continuation (R18/R20/R21).** The native leg-0 restart chain
(*cross-cited* FU-165 §3, all windows fresh there): step 0 queues situation
0xC → `FUN_000888FC` invokes **act 9 = phase-handler `0x88F4C`** (stage 0
writes phase 0x15); step 2 queues situation 3 → the `0x8AB08` arm → **act 4 =
`0x89620`** (stage 0 writes phase 0xB and installs **action 0x17** at
`0x89736`, fresh); BX=1 re-dispatch → **act 8 = `0x8A798`** → BX=0 re-dispatch
→ phase 4 (corner arm) → taker row 0x11 → situation 0xB → phase 2. The
downstream rows are wired; the engine's derived route (R1 in FU-165 §7) closes
the hold with `set_piece(0xC)` + a derived act-9 gate + `set_piece(3)` + the
phase-0x15 arm — **no action row needed**. The native-fidelity route (R2) needs
**action row 0x17** (body head fresh: `0x84743 CMP EAX,0xB; JZ` else the
`0x849A1` reset tail — the simple phase-0xB gate FU-81 §2 describes) plus the
phase handlers 0x1A/0x1E/0x1F.

---

## 4. Ranked frontier for phase-15 (cheapest next wiring)

Ranked by (installer already live?) × (derived-helper coverage) × (phase-14
relevance). All five top entries **move the wired count 22 → 23** on landing;
all are honest (a native installer invocation exists and the engine already
reaches it).

1. **Row 0x1F — keeper dive/arm (installer live: phase-6 arm).** The engine
   installs 0x1F in the phase-6 penalty arm (`match_run.c:2445`) and the row is
   UNSUP; T3's leg-1/5 penalty routes make phase 6 reachable live. The four
   decision helpers are ported+tested (`fifa96_keeper_dive_target`,
   `_arm_step`, `_arm_camera`, `_input_decide`); the work is the 4-stage row
   body `0x76380..0x765C2` + the install execution (OL-36). Wiring it also
   arms its own second-order install of 0x1B.
2. **Row 03 — hold/clamp (installer live: the L4.1 walk and row-04 active
   records).** The forced-decision walk writes code 3 (→0x19 when inactive)
   for unbound records every live frame; the FU-138 helpers are ported+tested.
   Remaining: the `[rec+0x1C]` phase-handler call, `FUN_00079F3C` camera
   placement and the phase-7 ball scan (OL-19).
3. **Row 09 — sequence/steer (two live installers: the pad release edge and
   the wired row-06 RNG arm).** Highest install traffic, highest cost: the
   body `0x80A00..0x81065` (597 insns) has the FU-138 stage-0 slice and the
   `0x809F0` arm table, but the arms are unported (FU-136). Its body also
   installs 0x0D (second-order).
4. **Row 0x0B — duel/sequence (installers live: pad press edge + row-04 score
   arm).** `fifa96_action_sequence_duel_event` is ported; the body
   `0x81908..0x81C8F` is the remainder (FU-136/138 OL-22 family).
5. **Row 0x19 — keeper hold (installer live: row-04 non-active arm).** The
   `keeper_hold_*` helpers are ported+tested; the predictor block
   `0x74832..0x74ACC` is OL-33.
6. **Ball-machine non-phase-2 branches (L14.8.a)** — the phase-6 goal-area lead
   (`0x8BB13`, the penalty route T3's leg 1/5 can reach) and the phase-0xC/0x13/
   0x14 fixed leads + match-over words. Small, engine-internal; keeps the ball
   moving through a T3 penalty instead of parking it.
7. **Row-05 stage-0 tail (`0x7F3A1..0x7F57B` + `FUN_0007F7E0`, OL-63)** — the
   live claim frame's second pan event and the `rec+0x4D` target writes;
   medium (FU-164 rank 2). Directly visible once T2's follow is sustained.
8. **Action row 0x17 + the act machine (T3-R2 fidelity)** — the line-up walk
   (simple body, fresh head) plus the act handlers `0x1A/0x1E/0x1F`; a full
   slice (FU-165 §9 risk: double restart writes).
9. **Keeper chain rows 1B/1C + `FUN_00076130`/`FUN_00077EAC`/record-0 machine**
   — deep (FU-140 OL-34/35/36); after T2 the dive/rush cameras are the visible
   missing piece.
10. **`0x71B8A` armer angle arm** — fidelity once the follow carries the camera
    out of bounds (helpers exist; medium).
11. **Row 0x16 (fouled-player face-the-ball)** — body derived (FU-148 §16.2),
    installer behind the non-live phase-0x19 chain; low-medium once the foul
    chain or T3's act work lands.

Blocked / no-move: aux-0x22 arm `0x6FA62` (no executable frame produces aux
0x22 — FU-164 §3.3), rows 27/29/2C/2B (no installer), row 0x0A (installer
`0x7CDD8` has no xrefs, FU-137), the phase rows until an act/phase call site
exists (§7 note 2).

---

## 5. The six `FUN_00071C94` pan callers after T2

Fresh `get_xrefs_to 0x71C94` = **11** (same set); the two wired row-04 sites
(`0x7EFCF/0x7F0D1`), row-1E `0x75DC4`, reposition `0x70DD1` and ball-staging
`0x7A219` are wired. The six from FU-156:

| site | attribution (fresh where marked) | status after phase-14 |
|---|---|---|
| `0x82A6C` | **row-0x0E arm 2** (fresh: `0x82700[2]=0x82806`; call bytes `e8 23 f2 fe ff`) | **closed by T2/R16** (FU-168) — inside a wired row |
| `0x7F4E7` | row-05 stage-0 tail (row wired; tail unported) | frontier §4 rank 7 (medium) |
| `0x71B8A` | `FUN_0007131C` angle arm (out-of-bounds; fresh call bytes `e8 05 01 00 00`, `0x71B85 MOV EAX,[0x1577CA]`) | frontier rank 10 (fidelity after the follow) |
| `0x6FA62` | `FUN_0006E8E8` aux-0x22 arm | blocked by the missing aux producer (FU-164 §3.3) |
| `0x77423` / `0x77DC6` | rows 1B/1C keeper rush/dive (unwired rows) | frontier rank 9 (deep keeper chain) |

---

## 6. Census audit — which rows move the count next

Among the **45 action rows**, the next honest movers (native installer
invocation exists **and** the engine already reaches it) are, in order:
**0x1F, 03, 09, 0x0B, 0x19** (§2 table) — each flips one NULL `fn` to a
ported body and moves the dispatch to `23 OK / 56 UNSUP / 1 NOTF`, then
`floor(100 × 23/80) = 28%`. The second wave behind T3/foul work: **0x17**
(act 4) and **0x16** (phase 0x19). The phase table (35 rows) stays 0/35 until
a live act/phase call site exists (§7 note 2); wiring a phase handler today
would move the number with no live effect.

---

## 7. Discrepancy notes (audit)

1. **Wording.** ENGINE.md's "**22/80 rows wired**" / AGENTS.md's "N/M action
   rows" spans *both* native tables (45 action + 35 phase). The count is
   correct; the phrase is a shorthand. The listing in ENGINE.md §"Action
   dispatch" names exactly the 22 non-NULL rows (verified against the source
   table this session).
2. **The phase seam has no live caller.** Fresh `grep`: the only
   `fifa96_match_dispatch_phase` callers are the unit tests. Native act/phase
   dispatch (`FUN_000888FC` -> `[0x158808]` -> the clock-tail tick
   `0x8B669..0x8B672`) is unported (FU-165 §6). The census can move on a phase
   row only once that call site lands (T3/R18) — a lever, never a table edit.
3. **Table-window corrections are already folded.** FU-156 §2 read 132 B of
   the action table (rows `0x00..0x20`); FU-164 §2.2 item 3 corrected the full
   180 B / 45-dword extent. This slice's full read re-confirms the 45-slot
   bound and the `0x1106E0 + 0xB4 = 0x110794` contiguity.
4. **`0x82A6C` attribution.** FU-156 §3.6 read it as "row 0D arm 3"; FU-164
   §2.2 corrected it to row-0x0E arm 2 (fresh call-bytes and arm-table read
   here confirm). No census effect (both are rows, and only row 0x0E is
   wired).
5. **Historical per-phase counts** (e.g. FU-140 §5's "2 × OK … 77 × UNSUP")
   are snapshots of their task time, superseded by the current 22/80 — not
   live discrepancies.

---

## 8. Numbered legs

1. **FU174-L1 — the act/phase dispatch seam.** `fifa96_match_dispatch_phase`
   has no engine caller (tests only); the native act machine
   (`FUN_000888FC` + the `0x8B669` tick) is unported (FU-165 §6). Whether a
   phase row may join the census before T3/R18's act work is a design
   decision; today it would be a count with no live effect.
2. **FU174-L2 — row-0x16 install instruction.** The handler head `0x89FA4` is
   fresh; the `MOV EDX,0x16` instruction sits deeper in the body and is
   *cross-cited* FU-148 §16.2. Re-read before wiring row 0x16.
3. **FU174-L3 — the row-03/09/0B/19/1F bodies.** This slice verified the
   installers, gates and live engine paths only; the per-arm body derivations
   remain the named FU-136/138/139/140/142 legs (OL-19/22/33/34/36, FU-136
   row-09 "arms unported").
4. **FU174-L4 — the keeper-chain upstream** (`FUN_000782D0` record-0 machine,
   `FUN_00076130`, `FUN_00077EAC`, the `0x76B28` steering) stays
   OL-34/OL-35/OL-36; a keeper slice should be co-scoped with the FU-151
   family (FU-164 §3.5).
5. **FU174-L5 — ball-machine non-phase-2 branches** (FU-148 §16 L14.8.a/b/c)
   carried; the phase-6 goal-area lead and the 0xC/0x13/0x14 leads are the
   T3-relevant pair.
6. **FU174-L6 — the pan-caller count is a snapshot.** Fresh this session = 11
   sites; re-check on any future wiring (the FU-148/FU-156/FU-164 counts
   agree).

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_xrefs_to 0x71C94` (11), `0x7D9A4` (77); `read_memory 0x1106E0` (180 B),
`0x110794` (140 B), `0x82700` (16 B); `disassemble_bytes` `0x826E0` (128 B,
the row-0D RET/row-0E prologue/arm dispatch), `0x82A50` (48 B, the arm-2
event), `0x71B80` (16 B), `0x84730` (64 B, row-0x17 head), `0x7CF40` (48 B),
`0x7CF6D` (75 B, the 0x7CF54 install 9), `0x7CE20` (64 B), `0x7CE5F` (75 B,
the 0x7CE38 install 0x0B), `0x7ED40` (64 B, the row-04 install 0x0B),
`0x7E840` (144 B, the row-04 4/3/0x19 installs), `0x80F50` (48 B, the row-09
install 0x0D), `0x8972C` (32 B, the act-4 install 0x17), `0x89F9C` (64 B, the
phase-0x19 head), `0x76550` (48 B, the row-1F install 0x1B), `0x8D4A0`
(192 B, the phase-6 arm 0x13/0x1F installs). Engine/loader source read:
`include/fifa96_engine/fifa96_match_handlers.h` (33..79),
`src/fifa96_engine/fifa96_match_handlers.c` (125, 488, 1008..1012, 2312,
2443..2456, 2540..2577, 2921, 2939..3078),
`src/fifa96_engine/fifa96_match_run.c` (610..820, 1160..1509, 2400..2480,
5060..5130), `src/fifa96_engine/fifa96_match_entities.c` (73..105),
`src/fifa96_loader/fifa96_outfield.c` (387..520).
Docs cross-cited: FU-136/137/138/139/140/142/148/156/164/165; the phase-14
plan R22 section; AGENTS.md census rule. No writes beyond this slice file and
the workspace report.
