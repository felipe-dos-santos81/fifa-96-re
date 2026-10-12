# FU-182 — census audit + next frontier (phase-15 wave R30)

Provenance: M2 phase-15 wave **R30**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument). Every xref count, table byte, call byte and instruction below is a
fresh `get_xrefs_to` / `read_memory` / `disassemble_bytes` /
`disassemble_function` / `search_instructions` of **this session** unless marked
*cross-cited* (then the frozen FU doc is named). Engine source read first-hand
(`fifa96_match_handlers.c`, `fifa96_match_run.c`,
`fifa96_loader/fifa96_action_handlers.c`). No writes beyond this slice and the
workspace report; no rename/label/comment/function/script/project save; no git.
The alias traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at
`x+3`).

Scope: (1) re-verify the count machinery first-hand; (2) state the current
verified census and the exact post-phase-15 arithmetic; (3) list the remaining
honest movers and the blocked/no-move set; (4) rank the phase-16 frontier.

---

## 1. Fresh census machinery — the "N/80 wired" claim

The engine census (`include/fifa96_engine/fifa96_match_handlers.h`: action
`FIFA96_MATCH_ACTION_ROWS 45`, phase `FIFA96_MATCH_PHASE_ROWS 35` — 80 rows)
counts non-NULL `fn` rows of the two native dispatch tables. Fresh native
re-verification this session:

* **Action table `0x1106E0`** — fresh `read_memory` (180 B = 45 dwords) returns
  the full table, rows `0x00..0x2C`, ending at `0x110794` exactly (45 × 4 =
  0xB4), where the phase table begins (contiguity first-hand). Decoded dwords:
  `00:07DB10 01:07DBC0 02:07DFCC 03:07E1A4 04:07E7C8 05:07F194 06:0801B4
  07:0814B0 08:081068 09:080A00 0A:081738 0B:081908 0C:081C90 0D:08251C
  0E:082710 0F:082AD0 10:0855F0 11:085DE4 12:083D68 13:084B00 14:08784C
  15:087CD0 16:084630 17:084730 18:0849B0 19:0746E4 1A:07662C 1B:076D28
  1C:077728 1D:074EB0 1E:07550C 1F:076380 20:084EEC 21:085214 22:08539C
  23:082F84 24:086510 25:0880CC 26:0866F4 27:086820 28:0870E8 29:0874E4
  2A:086A34 2B:087738 2C:084598`.
* **Phase table `0x110794`** — fresh `read_memory` (140 B = 35 dwords), rows
  `0x00..0x22`; `[0x16] = 0` (the native zero/INT3 slot); tail handlers
  `[0x17]=0x88DC8 [0x18]=0x8922C [0x19]=0x89FA4 [0x1A]=0x89620 [0x1B]=0x890EC
  [0x1C]=0x89110 [0x1D]=0x89868 [0x1E]=0x8A798 [0x1F]=0x88F4C [0x20]=0x8B688
  [0x21]=0x8B874 [0x22]=0x8B900`; the table ends at `0x110820`.
* **Action reader `0x1106E0`** — fresh `get_xrefs_to` = **2**, both in
  `FUN_0007d9a4` (`0x7DA77`/`0x7DA7C`): the *sole* reader (installer).
* **Phase reader `0x110794`** — fresh `get_xrefs_to` = **2**, both in
  `FUN_0006d920` (`0x6D9B3`/`0x6D9B8`): the *sole* reader.
* **Installer `0x7D9A4`** — fresh `get_xrefs_to` = **77** call sites (matches
  FU-174 §1's fresh count). Fresh `disassemble_function` confirms the dispatch:
  `0x7DA74 SHL EAX,0x2; 0x7DA77 ADD EAX,0x1106e0; 0x7DA7C MOV EAX,[EAX]`,
  the store `0x7DA8F MOV [ESI+0x18],EAX`, the code-3→0x19 inactive remap
  `0x7DA26..0x7DA3B` (`CMP byte[ESI+0x8d],0 → CMP EAX,3 → MOV word[ESP+4],0x19`),
  the code store `0x7DA67 MOV [ESI+0x91],AL`, and the invoke-now
  `0x7DAA3 TEST DI,DI; 0x7DAA8 MOV EAX,ESI; 0x7DAAA CALL [ESI+0x18]`.
* **Phase reader body `FUN_0006D920`** — fresh: `0x6D9A8 MOV EAX,[0x157a4a];
  SAR EAX,0x18; 0x6D9B0 SHL EAX,0x2; 0x6D9B3 ADD EAX,0x110794; MOV EAX,[EAX];
  0x6D9BA MOV [EDX+0x1c],EAX` — the verified phase-handler-to-record store.
* **The per-record phase caller `FUN_0008D098`** — fresh
  `disassemble_function`: the record loop at `0x8D0F7 CMP byte[ECX+0x9a],0;
  JNZ` (skip inactive) → `0x8D107 CALL 0x6d920` → `0x8D110 CALL dword[ECX+0x1c]`
  (the phase handler), walking 0xB (11) records at 0xB2 stride
  (`0x8D163 CMP EAX,0xb; 0x8D166 JL`). The **phase seam's call site** is real
  and per-record.
* **The act-slot dispatcher `FUN_000888FC`** — fresh `disassemble_bytes`:
  `0x88905 CMP AX,CX` (vs current act `[0x158828]`) → `0x8890A MOV [0x158828],AL`
  (latch the new act) → `0x8890F CWDE; 0x88910 SHL EAX,0x2; 0x88913 ADD
  EAX,0x1107ec; 0x88920 MOV EAX,[EAX]; 0x88929 MOV [0x158808],EAX` → guarded
  `0x88935 JZ` → `0x88937 CALL dword[0x158808]`. `get_xrefs_to 0x1107EC` = **2**,
  both `FUN_000888fc` — a **third, dedicated table base** whose `[0] = 0` and
  whose entries `1..` are the phase-table tail handlers (act N → `0x1107EC +
  N*4` = phase `0x16+N`), an overlay the loader/code chose deliberately; see
  leg **FU182-L1**.

Engine-side count (source `fifa96_match_handlers.c:3404..3519`, first-hand):
the action table has **24 non-NULL `fn`s** (`00 01 02 04 05 06 07 08 0B 0C 0E
0F 10 11 12 13 18 1D 1E 21 23 26 28 2A`) and **21 NULL**; the phase table has
**35 NULL** (including the INT3 slot). Dispatch arithmetic: **24 OK + 55 UNSUP
(21 action + 34 phase) + 1 NOTF (phase `0x16`) = 80**. The README title `30%` =
`floor(100 × 24/80)`. ENGINE.md §"Action dispatch" says `24/80 rows wired` and
names exactly the 24 codes above (cross-checked against the source this
session).

**Audit verdict: the 24/80 claim is provable as stated.** FU-174's `22/80` was
the pre-phase-14-T1 snapshot; row `0x0B` (`FU-169 §3`) is wired — errata note
§8.1.

---

## 2. The verified current census (24/80)

| | count |
|---|---|
| action rows wired (non-NULL `fn`) | **24** |
| action rows UNSUP (NULL `fn`) | 21 |
| phase rows wired | **0** |
| phase rows UNSUP (NULL, ex-`0x16`) | 34 |
| phase `0x16` (INT3) | 1 NOTF |
| **wired / total** | **24 / 80 → 30%** |

Dispatch: `24 OK / 55 UNSUP / 1 NOTF`.

---

## 3. The exact post-phase-15 arithmetic

Phase-15 (plan `docs/superpowers/plans/2026-10-12-fifa96-m2-phase15-census-frontier.md`)
targets **seven action rows** — R24→**0x1F**, R25→**03**, R26→**09**, R27→
**0x19**, R28→**0x1B**+**0x0D**, R29→**0x17** — all currently NULL this session
(source table §1). If all seven land: `24 → 31` non-NULL action rows →
`31 OK / 48 UNSUP / 1 NOTF` → `floor(100 × 31/80) = floor(38.75) = 38%`.

The phase table stays **0/35** unless R23/T1 lands *both* a live
`fifa96_match_dispatch_phase` caller *and* at least one ported phase-handler
body *and* a live producer of that phase; the native per-record call site exists
(§1, `0x8D110`) but the act-machine producer is unported (FU-165 §6). R23/FU-175
is the authority on any live phase row; **this audit verifies no phase row as a
mover today**. Any `N` live phase rows make the post-phase-15 title
`floor(100 × (31+N)/80)`.

---

## 4. First-order movers the phase-15 plan missed (0x14, 0x22)

Applying FU-174 §2's criterion (a native installer invocation exists **and** the
engine already reaches it), two more currently-NULL rows qualify — both fresh,
both absent from FU-174 §2/§4/§6 and from the phase-15 plan:

| code | native installer (fresh) | engine live installer (source) | row state |
|---|---|---|---|
| **0x14** | act-5 arm `0x8D461 MOV EDX,0x14; 0x8D466 MOV EAX,[0x157a9f]; 0x8D475 CALL 0x7D9A4` (fresh; the `0x8D433..0x8D4A1` celebration set, gated on the goal record's team) | the engine phase-5 arm `fifa96_match_run_phase_arm`, `fifa96_match_run.c:2480` `fifa96_match_entities_install(&team->records[idx], phase, 0x14u, 0)` (gated on `mr->goal_record >= 0 && team->side == goal_record_side`) | row 0x14 NULL — FU-136 scatter_celebration helpers; OL-9 |
| **0x22** | row-07 body `0x816D6 MOV EDX,0x22; 0x816DF CALL 0x7D9A4` (fresh `disassemble_bytes 0x816D0`: `0x816D1 MOV ECX,1` = invoke-now, `EAX=ESI`) | the wired row-07 `fifa96_match_action_07`, `fifa96_match_handlers.c:1160` → `match_kick_invoke_opponent` → `fifa96_match_entities_install(…, 0x22, 0)` (`handlers.c:1122`), gated on `out.opponent_invoke` (`fifa96_action_handlers.c:1612`, `diff < 0x55`) | row 0x22 NULL — FU-136 sequence_press_event; OL-9 |

Nuance for **0x22**: the installer fires but its target is the other team's
`target` record, whose producer is unported (FU-164/OL-56: the team-target
producer); the arm is therefore *code-live but producer-dormant* until OL-56
lands. Under the strict "engine reaches it" test it still qualifies (the call
runs whenever `opponent_invoke` is set and a target id is valid).

**Errata (audit):** FU-174 §2/§4/§6 did not list 0x14/0x22 among the movers.
0x14 was invisible because the phase-5 celebration arm was derived in FU-173
(phase-14 T3) *after* FU-174's §2 table; 0x22 was classed only as a "row-07
request" and not traced to the installer. Neither is a prior misread; both are
later evidence.

---

## 5. Second-order movers — the act seam, the keeper chain, the phase arms

Fresh `search_instructions` + confirmed call bytes (installer = `CALL 0x7D9A4`):

| code | installer (fresh) | first blocker |
|---|---|---|
| **0x16** | phase-0x19 handler region `0x8A096 MOV EDX,0x16; 0x8A09B MOV EAX,[0x15888f]; 0x8A0A0 CALL 0x7D9A4` (fresh `disassemble_bytes 0x8A080`) | needs the act seam + the phase-0x19 live producer (foul chain / act 3); body *cross-cited* FU-148 §16.2 |
| **0x24** | phase-0x20 handler `0x8B747` (loop ESI=0x1588A4, 11 rec) and `0x8B782` (loop ESI=0x1590D9) — `MOV ECX,1; MOV EDX,0x24; CALL 0x7D9A4` (fresh `disassemble_bytes 0x8B688`, 491 B) | needs the act seam to reach phase 0x20; body OL-9 |
| **0x1A** | `0x773DC`, `FUN_00077eac` `0x78227/0x7824F/0x78277`, `FUN_0009e420` `0x9E665` (`MOV EDX,0x1a`; fresh search) | the keeper chain (`FUN_00077EAC`/`FUN_000782D0`/`FUN_00076130`) unported — FU-140 OL-34/OL-35 |
| **0x1C** | `FUN_00070c08` `0x70DAF`, `FUN_00076130` `0x76291`, `FUN_00077eac` `0x782B6`, `0x80814` (`MOV EDX,0x1c`; fresh search) | same keeper chain — FU-140 OL-35 |
| **0x20** | unverified this session (fresh `search_instructions EDX, 0x20` truncated before the `0x8B…` region); FU-82 7-arm table `0x84ED0` (*cross-cited*) | body + installer both legs |
| **0x25** | **no installer found this session**: the only `0x7xxxx` `MOV EDX,0x25` (`0x7E460`) is `CALL 0x6e598` (an anim/sink, cross-cited OL-27/OL-52), not `0x7D9A4`; FU-82 7-arm table `0x880B0` | leg |

Install-time caveat: rows **0x14/0x1F** install via the engine's own
`fifa96_match_run_phase_arm` (`match_run.c:2456..2569`), which is *already wired*
for phases 3–9/0x0D and installs codes `0x15 0x14 0 0x13 0x1F 0x12 0x1E 0x1D
0x10 0x11` (fresh source read); of that set only **0x14** and **0x1F** are
currently NULL.

---

## 6. Blocked / no-move set (cannot honestly move the census)

* **0x0A** — installer `0x7CDD8` has no xrefs (*cross-cited* FU-137 §6).
* **0x15** — head mis-decoded, stub bucket (OL-14).
* **0x27 / 0x29 / 0x2C** — ported bodies, but the FU-142f census of all
  `0x7D9A4` call sites finds no `0x27`/`0x29`/`0x2C` invocation; entry
  unresolved (FU-142f/OL-48) → `-UNSUPPORTED` by decision.
* **0x2B** — dead entry: native `0x87738` is the shared row-29 epilogue RET, not
  a standalone stub (FU-142f/OL-15).
* **phase rows `0x00..0x22`** — all NULL; no live `fifa96_match_dispatch_phase`
  caller (`grep`: tests only) and no act-machine producer (§3).

---

## 7. Ranked phase-16 frontier

Ranked by (installer live?) × (derived-helper coverage) × (reachability cost).
All action entries below flip one NULL `fn` → `+1` on the wired count.

1. **Row 0x14 — scatter/celebration (installer live: phase-5 arm,
   `match_run.c:2480`).** Cheapest: the installer already runs on any goal;
   FU-136 has scatter_celebration helpers; body `0x876xx` + the goal-record
   release (stage-2 phase-1 write `0x87C71`, FU173-L2/L3 *cross-cited*).
   **Moves 24→25 (and 31→32 post-phase-15).**
2. **Row 0x22 — sequence/press (installer live: wired row-07,
   `handlers.c:1122`).** Small body (OL-9); the install arm is dormant until the
   OL-56 team-target producer lands, so it is honest but low-impact. Moves the
   count if the target producer exists.
3. **Rows 0x1A + 0x1C — keeper reposition/lunge (installer live only after the
   keeper chain).** Co-scope with R28/FU-180 + T2's keeper work; the installs
   (`FUN_00077EAC` `0x782xx`, `FUN_00076130` `0x76291`, `FUN_00070C08` `0x70DAF`,
   fresh) need `FUN_000782D0`/`FUN_00076130`/`FUN_00077EAC` ported (FU-140
   OL-34/OL-35). Medium-deep.
4. **Row 0x16 — fouled-player face-the-ball (installer: phase-0x19 handler
   `0x8A096`, fresh).** Needs T1's act seam + the phase-0x19 live producer
   (foul/act-3); body derived FU-148 §16.2. Medium.
5. **Row 0x24 — sequence/lane (installer: phase-0x20 handler `0x8B747`/`0x8B782`,
   fresh).** Needs T1's act seam to reach phase 0x20; body OL-9. Medium.
6. **Row 0x20 — FU-82 7-arm table `0x84ED0`.** Installer unverified this session
   (leg FU182-L2); body OL-9. Deep.
7. **Row 0x25 — no installer found (fresh).** Mostly blocked pending a leg
   (FU182-L2).
8. **0x0A / 0x15 / 0x27 / 0x29 / 0x2B / 0x2C** — blocked/dead (§6).

**Phase-table frontier (conditional, `0/35`):** once R23/T1 wires the live
`fifa96_match_dispatch_phase` caller (the per-record `[rec+0x1C]` call at
`0x8D110`), the honest phase movers are the handlers whose bodies R23 proves
live-reachable through the act machine (the `FUN_0008D098` `0x8d040` jump-table
arms). This audit does not have that producer, so **no phase row is a verified
mover**; treat the phase rows as R23/FU-175's call, not a table edit.

---

## 8. Discrepancy notes (audit)

1. **Baseline correction.** FU-174 §1/§4/§6 recorded `22/80` (README `27%`)
   with the set `00 01 02 04 05 06 07 08 0E 0F 10 11 12 13 18 1D 1E 21 23 26 28
   2A`. The current verified baseline is **`24/80` (README `30%`)**: that set
   plus `0x0B` and `0x0C`, both phase-14 T1/FU-169 (row-0B duel body; row-0C
   contact body) — a `+2` delta. Re-derived from the source table this session
   (§1); the per-row history is ENGINE.md's, not re-litigated here.
2. **Table extents re-confirmed.** The 45-slot action table (`0x1106E0 + 0xB4 =
   0x110794`) and the 35-slot phase table (`… + 0x8C = 0x110820`) match FU-174
   §1/§7 note 3.
3. **The third table `0x1107EC`.** `FUN_000888FC`'s act-handler base coincides
   with the phase-table `0x16` slot address; see leg FU182-L1. No census effect
   (the act machine is unported either way).
4. **`0x22` install call.** FU-174 §2 named "opponent 0x22/ball 4 requests" for
   row 07 but did not classify 0x22 as a mover; the fresh call bytes (`0x816D6`)
   and the engine row-07 path (§4) make it a conditional mover.
5. **Historical per-phase counts** (FU-140 §5's snapshots etc.) are superseded;
   not live discrepancies.

---

## 9. Numbered legs

1. **FU182-L1 — the `0x1107EC` base.** `FUN_000888FC` reads a dedicated
   sole-reader table at `0x1107EC` (`get_xrefs_to` = 2, both `FUN_000888FC`);
   `[0] = 0` and entries `1..` equal the phase-table tail (`act 1 → 0x88DC8 =
   phase 0x17`). Whether this is a relocated act table, an intentional overlay,
   or a mis-based read is R23/FU-175's to reconcile. Re-read before trusting any
   act-handler address.
2. **FU182-L2 — the 0x20 / 0x25 installers.** No installer call found for
   `0x25` (only the `0x6E598` sink at `0x7E460`); `0x20`'s installer not located
   this session (the `EDX, 0x20` search truncated before the `0x8B…` region).
   Re-search with the FU-82 arm tables `0x84ED0`/`0x880B0` before wiring.
3. **FU182-L3 — the 0x14/0x1A/0x1C/0x16/0x24 bodies.** This slice verified the
   installers, tables and live engine paths only; the per-row bodies remain
   FU-136/140/148/173 (OL-9/33/34/35, FU173-L2/L3) legs.
4. **FU182-L4 — the phase-row frontier.** Conditional on R23/T1's act seam; no
   phase row is a verified mover this session.

---

## 10. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_xrefs_to` `0x7D9A4` (77), `0x1106E0` (2), `0x110794` (2), `0x1107EC` (2);
`read_memory` `0x1106E0` (180 B), `0x110794` (140 B), `0x1107E0` (64 B);
`disassemble_function` `0x7D9A4` (73 insns), `0x6D920` (51), `0x8D098` (583);
`disassemble_bytes` `0x888FC` (180 B), `0x813E0` (48 B), `0x80D90` (48 B),
`0x8A080` (48 B), `0x816D0` (48 B), `0x7E450` (48 B), `0x70200` (48 B),
`0x8B740` (64 B), `0x8B688` (491 B);
`search_instructions` `EDX, 0x1a` (11) / `0x1c` (11) / `0x16` (39) / `0x14`
(145) / `0x22` (6) / `0x24` (11) / `0x25` (11) / `0x20` (truncated).
Engine/loader source read: `include/fifa96_engine/fifa96_match_handlers.h`
(33..79), `src/fifa96_engine/fifa96_match_handlers.c` (1108..1163,
3404..3519), `src/fifa96_engine/fifa96_match_run.c` (2456..2569),
`src/fifa96_loader/fifa96_action_handlers.c` (1612). Docs cross-cited:
FU-136/137/140/142f/148/151/164/165/169/173/174; AGENTS.md census rule. No
writes beyond this slice and the workspace report.
