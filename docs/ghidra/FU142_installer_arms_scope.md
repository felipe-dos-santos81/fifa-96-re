# FU-142: M2 cluster G scope probe — installer arms 0x26–0x2C + blocker legs

Task 9 of the M2 child plan (`docs/superpowers/plans/2026-10-07-fifa96-m2-match.md`),
the §12 decomposition gate for cluster G. The task brief scoped the probe over
(a) the `0x26`–`0x2C` installer arms and their handler bodies, and (b) the arms
that block the other rows (FU-139 OL-26..OL-32, FU-141 OL-38/OL-41), and
required a split ruling when the derivable work exceeds ~4 implementation
tasks.

Result in one line: **all seven `0x26`–`0x2C` handler bodies are now bounded in
`/FIFA96.EXE` (six real bodies, 0x2B is the shared epilogue RET of the 0x29
body), the install-arm machinery for `0x26`/`0x28`/`0x2A` re-verifies
(`FUN_0008CEB8` + the `FUN_0008D098` state 0x13/0x14 arm block), but wiring any
of these rows also needs the unported 1928-byte phase machine `FUN_0008D098`
(OL-5) and six unported helper families; the six bodies alone total ~1222
instructions, and the blocker legs add ten-thousand-plus bytes of unported
machine code — **6–7 implementation tasks for cluster G alone and 10–14 more
for the blocker legs, far over the 4-task threshold → `M2_SCOPE: split`
(parent spec §12). Do not port rows in Task 9.**

## Method

* Authoritative program **`/FIFA96.EXE`** (explicit `program` argument in every
  call; `/fifa96.exe` and `/fifa96_le.bin` untouched). Ghidra **read-only**: no
  renames, comments, labels, function creation, scripts or project saves.
* Tool calls made this slice (all on `/FIFA96.EXE`):
  * `get_function_by_address` — `0x866F4`, `0x86820`, `0x870E8`, `0x874E4`,
    `0x86A34`, `0x87738`, `0x84598` (all seven: **no function definition**);
    `0x8CEB8`, `0x8D098`, `0x7DAB4`, `0x6E598`, `0x6E1D0`, `0x8DCD4`,
    `0x36200`, `0x513EC`, `0x7A490`, `0x7B9C4`, `0x7E600`, `0x7A084`,
    `0x928F0`, `0x79C50`, `0x8D824`, `0x795B4`, `0x79B58`, `0x92820`,
    `0x8DB6C`, `0x786A0`, `0x4C324`, `0x71C94`, `0x79CCC`, `0x7F7E0`,
    `0x7B194`, `0x7B57C`;
  * `disassemble_bytes` — the seven row windows `0x866F4`(300 B),
    `0x86820`(520 B), `0x870E8`(1020 B), `0x874E4`(596 B), `0x86A34`(1716 B),
    `0x87738`(276 B), `0x84598`(152 B), all with instructions; the arm block
    `0x8D720`(244 B) with instructions; the row-05 end `0x7F7C0`(64 B);
  * `get_xrefs_to 0x8CEB8` (15, all in `FUN_0008D098` — re-verified);
  * repo `grep` over `src/`, `include/`, `docs/` for the helper ground truth.
* **Bounding method for the row bodies.** None of the seven action-table
  entries is a Ghidra function (they are table-referenced code with no direct
  call xrefs; `get_function_by_address` returns "No function found" for all
  seven). Each body was therefore bounded by its own prologue/RET and the next
  action-table entry: the six real bodies run exactly to their final RET,
  followed only by alignment bytes or the next row entry; row 2A's inline jump
  table (`0x86A04`, 12 dwords filling `0x86A04..0x86A33`) sits between row 27's
  pad and the row-2A entry.
* Baseline: `make check` = **100/100** (verified this slice, all 100 tests
  pass); M1 golden/render pins untouched (no source change).

## 1. Probe — the seven `0x26`–`0x2C` handler bodies

The action table `0x1106E0` (FU-137 §1.1) resolves row `c` to
`0x1106E0[c]`; the seven entries and their bodies, first-hand windows:

| code | handler | body (first-hand) | insns | calls (first-hand) | class |
|---|---|---|---|---|---|
| 26 | `0x0866F4` | `0x866F4..0x8681C` RET (last 3 B alignment to `0x86820`) | 90 | `0x8DCD4` (distance/staging) | real body; arm `0x8D74D`; FU-137 "unanalyzed" |
| 27 | `0x086820` | `0x86820..0x86A02` RET + NOP pad | 136 | `0x8DCD4`, `0x79C50` (face), `0x6E598` (anim selector); tables `0x1103CB`, globals `0x158782`, `0x10F372/374` | real body; **no static install arm** |
| 28 | `0x0870E8` | `0x870E8..0x874E3` RET | 294 | `0x8DCD4`, `0x79C50`, `0x36200` (stub), `0x92AC8` (RNG, many), `0x6E598`, internal `0x87014`; 4-arm jump table `0x870D8`; tables `0x7D8B0/C0`, `0x114E04`; globals `0x10F358/35C/364/368/36C`, `0x157AA3`, `0x157AC2`, `team+0x830/0x831` | real body; arm `0x8D7CF`; FU-137 "unanalyzed" |
| 29 | `0x0874E4` | `0x874E4..0x87738` RET (the RET byte is shared with the row-2B entry, §1.1) | 187 | `0x7DAB4` (reset), `0x7D9A4` (self-install 3), `0x8DE8C` (nearest), `0x36200`, `0x92AC8`, `0x6E598`, `0x6E1D0` (phase cell), `0x8DCD4`; globals `0x10F36C`, `0x157AA3`, `0x157A9F` | real body; **no static install arm** |
| 2A | `0x086A34` | main `0x86A34..0x87010` RET, preceded by the 12-dword jump table `0x86A04..0x86A33` and followed by helper `0x87014..0x870D5` RET | 417 + 48 helper | `0x79C50`, `0x36200`, `0x513EC` (camera stop, FU-118), `0x6E598`, `0x92AC8` (heavy); 12-arm jump table `0x86A04`; globals `team+0x830/0x831`, `0x10F358/35C`, `0x157AA3` | real body; largest of the seven; arm `0x8D807` |
| 2B | `0x087738` | the byte at `0x87738` is the final `RET` of the row-29 body; padding follows | 1 | — | **dead handler entry**; FU-137 "one-byte RET" confirmed and refined |
| 2C | `0x084598` | `0x84598..0x8462D` RET + `MOV EAX,EAX` pad | 48 | `0x6E598`, `0x7DAB4`; stage latch `+0x92` 0/1/2, `+0x8D`, `+0x89`, `+0x8B` type | real body; **no static install arm**; refines FU-137 "prologue-only" |

Bodies: **six real (90+136+294+187+417+48 = 1172 insns plus the 48-insn 2A
helper = 1220), one dead (1 RET).** Every real body is self-contained in the
record-visible sense (record pointer in EAX; no stack args), but each calls
unported helpers: `0x8DCD4` distance/staging, `0x6E598` animation selector
(FU-84, unported port), `0x79C50` face helper (FU-71 §10, unported),
`0x92AC8` RNG (ported, FU-141), `0x36200` 5-byte stub, `0x513EC` camera stop
(FU-118, unported), `0x7DAB4` reset (FU-141 OL-44 subset), `0x8DE8C` nearest
selection (pool-side equivalent exists in `fifa96_match_entities`), `0x6E1D0`
phase cell (FU-138 `fifa96_action_phase_cell` covers the pure part).

### 1.1 The row-2B entry is the row-29 epilogue

Row 29's final tail (`0x8772F ADD ESP,4`; `POP EBP/EDI/ESI/EDX/ECX/EBX`) ends
with its `RET` at exactly `0x87738` — the same byte the action table stores for
hand-picked code 2B. Calling row 2B therefore returns immediately; there is no
distinct 2B body. The bytes after `0x8773C` are two unrelated helpers
(`0x8773C..0x8776B`, `0x8776C..0x8784B`, 91 insns in the 276-byte window) that
must not be attributed to 2B. This confirms FU-137's "one-byte RET" and records
*why* it is one byte: the entry aliases row 29's epilogue.

## 2. Probe — installer-arm machinery

| item | first-hand evidence | port status |
|---|---|---|
| `FUN_0008CEB8` multi-record arm helper | body `0x8CEB8..0x8CF5D` (165 B); `get_xrefs_to` = **15 refs, all inside `FUN_0008D098`** (sites `0x8D19F`, `0x8D1C1`, `0x8D2B3`, `0x8D36B`, `0x8D45C`, `0x8D489`, `0x8D4B2`, `0x8D58B`, `0x8D5E9`, `0x8D64B`, `0x8D681`, `0x8D74D`, `0x8D78B`, `0x8D7AD`, `0x8D7CF` — re-verified) | unported |
| phase machine `FUN_0008D098` | body `0x8D098..0x8D820` (1928 B); the arms live in its state switch (`[0x157A4D]`/jump table `0x8D040`; `[0x157A4A]>>24` phase read) | unported (FU-137 OL-5; FU-141 §2.2/OL-44) |
| arm `0x26` @ `0x8D74D` | `0x8D728` side test: `byte[team+0x826] != byte[[0x157AAC]>>24]` -> `PUSH -1; ECX=0x26; EBX=0xA; EAX=EBP; EDX=0; CALL FUN_0008CEB8`; arm returns | not wired |
| arm `0x3` @ `0x8D78B`, arm `0x25` @ `0x8D7AD` | phase `[0x157A4A]>>24 == 0x13`; `[0x157AC5] == [0x157AC7]` -> install 3 else install 0x25 (neighbour arms, out of range, context) | not wired |
| arm `0x28` @ `0x8D7CF` | player side, phase `!= 0x13` (i.e. the 0x14 half of the state 0x13/0x14 block) -> `FUN_0008CEB8(team,0,10,0x28,-1)` | not wired |
| arm `0x2A` @ `0x8D807` | after the 0x28 arm: `EDX=1; EAX=team+0xB2`; loop `while (EDX<0xB && [EAX+0x9A]!=0) { EDX++; EAX+=0xB2 }`; then `[team+0x831]=EAX; EDX=0x2A; ECX=0; EBX=0; CALL FUN_0007D9A4` | not wired; **hazard**: when no record 1..10 is free the loop exits with `EAX = team+0x7A6` (one past record 10, inside the team field block) and installs 0x2A there; the port must reproduce the native scan, not "fix" it silently |
| installer `FUN_0007D9A4` | ported in `fifa96_match_entities_install` (FU-141 §3.1); invoke arm ECX!=0 is OL-43 | ported (staging) |
| record pool / team blocks | ported (FU-141 §1: 2 × `0x835` team blocks, 11 × `0xB2` records, side `+0x826`) | ported |

**Arm reachability.** The arm block only runs when `FUN_0008D098` is in state
0x13/0x14 and the phase byte is 0x13/0x14; the engine's
`fifa96_match_dispatch_phase` is still a row resolver with no machine/record
walk (FU-137 §9 concern, FU-143's task). So wiring rows 26/28/2A requires
porting at least the state 0x13/0x14 subset of `FUN_0008D098` plus the
`FUN_0008CEB8` loop and the `[0x157AAC]`/`[0x157AC5/AC7]`/`team+0x830/831`
bindings — that machinery does not exist in C today.

**No static install arm for 27/29/2C.** Re-citing FU-137 §5.3's whole-program
constant census (no `MOV EDX,0x27/0x29/0x2C` installer call sites in the match
code; 0x27 appears only as a `FUN_0006E598` animation-row argument; 0x29/0x2C
have no match-code references). The bodies exist but their entry is not
statically bounded; a dynamic/reachability pass is required before they can be
scheduled as ports (0x2B is dead, §1.1).

## 3. Probe — blocker legs OL-26..OL-32 / OL-38 / OL-41

First-hand bounds where checked this slice; the action row spans are cited from
FU-138/FU-139/FU-141 where already bounded there.

| leg | native surface (first-hand bounds unless cited) | port bits needed | task est. |
|---|---|---|---|
| OL-26 ball staging tail | `FUN_0007A490` `0x7A490..0x7AE37` (2471 B); tail `0x7A8D1..0x7AE2F` | code-keyed sub-code/animation tail, inactive reset block, `[0x158744/45]` stack writes | 1 |
| OL-27 event append sinks | `FUN_000928F0` `0x928F0..0x92994` (164 B) + `FUN_00092820` `0x92820..0x92861` (65 B) | 25-entry ring `0x5B440` stride 0x15 + `0x5B650` sink | 1 |
| OL-28 full kick path | `FUN_0007B9C4` `0x7B9C4..0x7BF16` (1362 B); mode arms `FUN_0007B194` (962 B), `FUN_0007B57C` (761 B); mode-bit arms `0x7BC34..0x7BC80`, code-4 RNG/divisor `0x7BE40..0x7BEC0` (FU-139 §6) | target selection + wing/slot + angle fold `0x114E04` | 1–2 |
| OL-29 row 05 carrier arms | row 05 `0x7F194..0x7F7C9` RETs (padding to `0x7F7E0`, first-hand); fallback `FUN_0007F7E0` `0x7F7E0..0x801B2` (2514 B); `FUN_00071C94` (350 B), `FUN_00079CCC` (141 B); stages 1–3 `0x7F57C..0x7F665` (FU-139 §6) | stage-0 target algebra, snap/hand-off animation, ball actor/receiver hand-off | 1–2 |
| OL-30 row 06 pursuit | row 06 `0x801B4..0x809EF` (~597 insns, FU-139 §2; the `..0x81067` span end is the row-09 body `0x80A00`, slot `0x1106E0[9]`) | target construction, `0x114E04` folds, RNG gates, installs 8/9/4 | **closed (Task 13, Appendix J; FU-139 §11); remainder OL-69** |
| OL-31 rows 07/0F kick machines | `FUN_0007E600` `0x7E600..0x7E7C4` (452 B); row 07 `0x814B0`, row 0F `0x82AD0..0x82DD0` (FU-139 §2) | decision, opponent 0x22 invoke, ball-actor install 4, fun-0F predictor/RNG/timer reload, `FUN_0007DAB4` tail | 2 |
| OL-32 reception/tackle/duel arms | `FUN_0007A084` `0x7A084..0x7A456` (978 B); NSEARCH `FUN_0008DB6C` `0x8DB6C..0x8DC49` (221 B); SWAP `FUN_000786A0` `0x786A0..0x786EB` (75 B); `FUN_0004C324` `0x4C324..0x4C372` (78 B); row 18/21/23 arms (FU-139 §6) | special-class/RNG arms, sound arms, NSEARCH/SWAP, row-21 claim, row-23 target, row-18 resolution | 1–2 |
| OL-38 outfield decide/chase wiring | rows 04 `0x7E7C8` / 08 `0x81068..0x814AF` (FU-138/FU-141 §7; the FU-141 `..0x81188` head is extended to row 08's RET at `0x814AF` by first-hand bytes here); `FUN_0007DAB4` `0x7DAB4..0x7DB0C` (88 B); `FUN_00079B58` (16 B); `FUN_00079C50` `0x79C50..0x79C98` (72 B); per-type gate `0x110680`; input tables `0x1109D0`/`0x1109E4` (FU-137 §4.1, FU-141 §7) | input-row dispatch, no-edge arm, forced decision, chase gate; then rows 04/08 wire | **machine subset closed (Task 14, Appendix K); row bodies split → OL-70 (04) / OL-70a (08), both closed and wired (Tasks 1/K.5, 2/K.6)** |
| OL-41 interception tail | `FUN_0008D824` `0x8D824..0x8D8EB` (199 B) + `FUN_000795B4` `0x795B4..0x795F0` (60 B) | bind call; distance band feeding `team+0x7BE` | **closed (Task 14, Appendix K); NULL-record band read OL-71** |

## 4. Derivability count and task estimate

| work item | items | tasks |
|---|---|---|
| G-a installer-arm machinery: `FUN_0008D098` state 0x13/0x14 subset + `FUN_0008CEB8` + record scan + team/side bindings + pool wiring | 1 machine slice | 1–2 |
| G-b row bodies 26 + 27 + 2C + shared helpers (`0x8DCD4`, `0x79C50`, `0x6E598`) | 274 insns | 1 |
| G-c row 29 body (needs `0x7DAB4`, `0x6E1D0`, `0x8DE8C`, installer invoke, RNG) | 187 insns | 1 |
| G-d row 28 body (4-arm table, angle fold, team flags, RNG) | 294 insns | 1 |
| G-e row 2A body + helper (12-arm table, RNG-heavy) | 465 insns | 1 |
| G-f dynamic entry probe 27/29/2C + 2B dead-code verdict + classification/errata closure | 3 rows | 1 |
| **cluster G subtotal** | | **6–7** |
| blocker legs OL-26..OL-32 (7 legs) + OL-38 + OL-41 | ~9.5 KB machine code across 16 first-hand functions | 10–14 |
| **total** | | **16–21** |

The threshold is the parent spec §12 / child-plan self-review rule: **~4
implementation tasks**. Cluster G alone is 6–7, so the split is forced even
before the blocker legs are counted.

## 5. Split ruling and recommendation

**`M2_SCOPE: split` — cluster G does not port in Task 9.** Recommended
decomposition (recording the split in the ledger per the plan's self-review
note):

1. **G1 / FU-142a — installer-arm machinery (1–2 tasks).** Port the state
   0x13/0x14 arm block of `FUN_0008D098` (arms 0x26/0x28/0x2A and the
   neighbouring 0x25/3 arms), `FUN_0008CEB8`, the record scan and the
   `[0x157AAC]`/`[0x157AC5/AC7]`/`team+0x830/831` bindings onto the FU-141
   pool; per-arm fixtures; rows stay unwired until their bodies land.
2. **G2 / FU-142b — small bodies + first wiring (1 task).** Port rows 26, 27,
   2C and the shared helper surface; wire row 26 (the only row whose arm, body
   and pool binding are all bounded), keep 27/2C unwired pending G6.
3. **G3 / FU-142c — row 29 body (1 task).** Port the phase-5 stage machine
   incl. the self-install of code 3; wire only if G4 resolves its entry.
4. **G4 / FU-142d — row 28 body (1 task).** Port the 4-arm jump-table stage
   machine, `0x114E04` angle fold, team+0x830/831 flags.
5. **G5 / FU-142e — row 2A body (1 task).** Port the 12-arm jump table + helper
   and its RNG-driven fields.
6. **G6 / FU-142f — dynamic entry/classification closure (1 task).** Probe the
   runtime entry of rows 27/29/2C (no static arm; e.g. DOSBox-X trace or
   reachability argument), confirm 2B dead, then update the FU-137
   classification + handler tests for exactly the rows that become fully
   derivable.
7. **Blocker legs stay with their owning clusters**, not with cluster G: OL-26
   with the ball-staging chain (FU-139), OL-27 with the event sequence cluster,
   OL-28 with the kick row 07/0F cluster, OL-29/OL-30/OL-31 with rows
   05/06/07/0F, OL-32 with rows 18/21/23, OL-38 with the cluster-D outfield
   rows 04/08, OL-41 with FU-141's interception chain (10–14 tasks total).

Ledger line to record: `M2 cluster G scope: split (6–7 tasks; blocker legs
OL-26..OL-32/OL-38/OL-41 = 10–14 tasks separately)`.

## 6. Open legs

* **OL-46 — installer-arm machinery.** `FUN_0008D098` is 1928 B and wholly
  unported (FU-137 OL-5); the state 0x13/0x14 arm block (`0x8D720..0x8D813`),
  `FUN_0008CEB8` (165 B) and the `0x2A` record scan are bounded here but need
  the FU-142a port; the unsigned 0x2A no-free-record edge (install into
  `team+0x7A6`) is recorded as a porting hazard. **Status (Task 2): the state
  0x13/0x14 arm block (`0x8D728..0x8D820`), `FUN_0008CEB8` and the 0x2A scan
  are ported and tested (`fifa96_match_phase_machine_step`, FU-142 Appendix
  B); the remaining states of the 1928-B function (0..0x15 switch arms, the
  pre-switch record walk `0x8D0A6..0x8D166`, and the state-entry RNG block
  `0x8D693..0x8D727` that precedes the ported arm block) stay unported.**
* **OL-47 — cluster-G row bodies.** Rows 26/27/28/29/2A/2C bodies are bounded
  (six bodies, ~1220 insns) but unported; each waits for its FU-142b..e task
  and for the unported helpers `0x8DCD4`, `0x79C50`, `0x6E598`, `0x513EC`,
  `0x7DAB4`, `0x6E1D0` (pure part covered by `fifa96_action_phase_cell`).
  **Status (Task 3): row 26's body is ported (`fifa96_arm_26_step`, Appendix
  C.2) with the shared helper `0x8DCD4` (`fifa96_arm_dist_stage`, C.3) and the
  `0x36200` stub surface (C.4); row 26 is wired (`fifa96_match_action_26`, the
  first cluster-G wiring), and the row's descriptor byte and stub-gate
  remainders are OL-50/OL-51. Status (Task 4): row 27's body is ported
  (`fifa96_arm_27_step`, Appendix D.3) with the shared helpers `0x79C50`
  (`fifa96_arm_face`, D.4) and `0x6E598` (`fifa96_arm_anim_select`, D.5); the
  row stays unwired (no entry, OL-48) and its remainder surfaces are
  OL-52/OL-53. Status (Task 5): row 2C's body is ported (`fifa96_arm_2c_step`,
  Appendix E.2) with the shared `FUN_0007DAB4` reset subset
  (`fifa96_arm_reset`, E.3); the row stays unwired (no entry, OL-48) and its
  remainder surface is OL-54. Status (Task 6): row 29's body is ported
  (`fifa96_arm_29_step`, Appendix F.2) with the `0x8DE8C` nearest search, the
  `0x6E1D0` phase cell (`fifa96_action_phase_cell`), the `0x92AC8` RNG draw
  and the `0x6E598` selector; the row stays unwired (no entry, OL-48) and its
  remainder surface is OL-55. Status (Task 7): row 28's body is ported
  (`fifa96_arm_28_step`, Appendix G) with the internal `0x87014` stage-gate
  helper; row 28 is wired (`fifa96_match_action_28`, the second cluster-G
  wiring) because the `0x8D7CF` arm, the full 4-arm body and the pool binding
  are bounded, and the row's remainder surfaces are OL-56..OL-58. Status
  (Task 8): row 2A's body is ported (`fifa96_arm_2a_step`, Appendix H) with the
  `0x513EC` camera-stop derived no-op; row 2A is wired
  (`fifa96_match_action_2A`, the third cluster-G wiring) because the `0x8D807`
  arm, the full 12-arm body and the pool binding (staged `+0x65` distance, team
  `+0x830`, the `[0x10F358]`/`[0x10F35C]` globals) are bounded, and the row's
  remainder surfaces are OL-59..OL-61.**
* **OL-48 — dynamic entry for 27/29/2C; 2B dead verdict.** No static install
  arm exists (FU-137 §5.3 re-cited); a runtime/reachability pass is the
  precondition for scheduling those three rows. 0x2B is confirmed a shared-RET
  dead entry (§1.1) pending that pass's ratification.
  **Status (Task 4): row 27's body is ported but the entry stays unresolved.**
  First-hand this slice (Appendix D.1): `get_xrefs_to 0x86820` returns exactly
  one reference — the action-table slot `0x11077C`; `search_byte_patterns
  20 68 08 00` finds that same slot as the only occurrence of the body
  pointer; `MOV EDX,0x27` has two sites (`0x756D5` inside the keeper handler,
  the `CALL 0x6E598` animation arg; `0x1F52E` in `FUN_0001F440`, whose tail
  `0x1F55D MOV EAX,0x6B; CALL 0x13600` is not an installer), `MOV DX,0x27`
  returns the same two, `MOV ECX,0x27` has no real site, and `0x8CEB8`'s 15
  arms (Appendix A.4) stage no code 0x27. A register-derived installer
  argument cannot be excluded by a constant census, so the FU-142f
  runtime/reachability pass remains the precondition; row 27 keeps `fn ==
  NULL` and the `-FIFA96_ERR_UNSUPPORTED` dispatch.
  **Status (Task 5): row 2C's body is ported but the entry stays unresolved.**
  First-hand this slice (Appendix E.4): `get_xrefs_to 0x84598` returns exactly
  one reference — the action-table slot `0x110790`; `search_byte_patterns
  98 45 08 00` finds that same slot as the only occurrence of the body
  pointer; `MOV EDX,0x2C` has three sites (`0x3035E` in `FUN_000302DC`,
  `0x40A8E`/`0x40C5C` in `FUN_00040A5C`), none an installer arm (the FU-137
  §5.3 census records none calls `0x7D9A4`), and `MOV ECX,0x2C` has no site;
  `0x8CEB8`'s 15 arms (Appendix A.4) stage no code 0x2C. A register-derived
  installer argument cannot be excluded by a constant census, so the FU-142f
  runtime/reachability pass remains the precondition; row 2C keeps `fn ==
  NULL` and the `-FIFA96_ERR_UNSUPPORTED` dispatch.
  **Status (Task 6): row 29's body is ported but the entry stays unresolved.**
  First-hand this slice (Appendix F.3): `get_xrefs_to 0x874E4` returns exactly
  one reference — the action-table slot `0x110784`; `search_byte_patterns
  e4 74 08 00` finds that same slot as the only occurrence of the body
  pointer; `MOV EDX,0x29` has exactly one real site (`0x1F53C` in
  `FUN_0001F440`, whose jump-table tail `0x1F55D MOV EAX,0x6B; CALL 0x13600`
  is not an installer) and `MOV ECX,0x29` has no site; the phase-5 handler
  `0x6E05C..0x6E1B2` (the handler of the body's own phase gate) contains no
  `FUN_0007D9A4` call, and `0x8CEB8`'s 15 arms (Appendix A.4) stage no code
  0x29. The body's own `0x8753C` install is code 3. A register-derived
  installer argument cannot be excluded by a constant census, so the FU-142f
  runtime/reachability pass remains the precondition; row 29 keeps `fn ==
  NULL` and the `-FIFA96_ERR_UNSUPPORTED` dispatch.
  **Status (Task 9): the FU-142f pass is complete as a written reachability
  argument (runtime capture unavailable, Appendix I.1) and its verdict is
  negative for all three rows.** All 77 `FUN_0007D9A4` call sites are
  classified (Appendix I.3), every register-derived argument is resolved
  (I.4), control-flow bypasses are checked (I.5) and indirect/stored-pointer
  entries are excluded (I.6): the reachable installer code domain is
  {0x00..0x26} ∪ {0x28,0x2A}, so **no installer invocation passes
  0x27/0x29/0x2C**. Rows 27/29/2C stay unwired (`fn == NULL`,
  `-FIFA96_ERR_UNSUPPORTED`, FU-137 class `unwired`) and OL-48 now records the
  final negative census rather than an open precondition. 2B is confirmed
  dead (§1.1/I.8).
* Carried: FU-139 OL-26..OL-32, FU-141 OL-38/OL-41. FU-137 OL-15's actionable
  part is closed by Task 9 (26/28/2A ported; 27/29/2C negative census; 2B dead,
  FU-137 §8).
* **OL-27/OL-32 closed (M2 arms-and-wiring Task 12).** The event append sinks
  (`FUN_000928F0`/`FUN_00092820`) are ported as
  `fifa96_event_ring_append`/`fifa96_event_sink_store` and the rows
  18/21/23 resolution arms as `fifa96_action_duel_search/_swap/_bind` plus the
  extended `fifa96_action_duel_step`/`_receive_step`/`_tackle_step`/
  `_tackle_attempt`; rows 18/21/23 are wired (`fifa96_match_action_18/_21/_23`)
  — see FU-139 §10. The residual surfaces are the new legs below.
* **OL-67 — event-ring binding (Task 12).** The ring/sink are loader-tested
  (FU-139 §10.2) but no engine path drives them; the `0x157758` triple (FU-71
  track writers `FUN_0006FFC0`/`0x736AC`) and the `[0x112E88]` stamp are caller
  inputs, and codes >= 0x28 (or negative sign-extended) leave the embedded
  0x28-byte 0x110F1C eligibility table (the native reads adjacent data).
* **OL-68 — rows 18/21/23 unmodeled record/presentation inputs (Task 12).**
  Row 18's `[[rec+0x28]][0]` abort byte (staged 0; shares OL-52) and the bind
  globals `[0x157AC2]`/`[0x1587D4]`/`[0x1587E3]`/`[0x1074A4]`/`[0x14E574]`;
  the SWAP slot-word clears; row 21's `+0x44` (staged 0), the camera-velocity
  zero, the `0x6E598` record writes (OL-52) and the `[0x79B58]` +0x99 gate
  (the pool has no +0x99); row 23's `[0x1577CA]` exclusion (`is_tracked`
  stand-in), the `0x71B9C` predictor (camera stand-in), the
  `word[+0x85]`/`+0x99`/`+0x5D` byte gates and the `[team+0x7C7]` record
  (`is_own`/`opp_*` staged 0), the slot `+0x10` button byte and the
  `word[+0x77]` bound (shared OL-65). See FU-139 §10.8.
* **OL-T11-8 — formation/record placement — landed (M2 visible-match Task 1).**
  The resource half is first-hand and corrected in FU-89 §11's new erratum
  bullet: `FUN_0006D920` resolves each record's formation pointer as
  `FUN_0004AFB8(6*formation_id) + byte[rec+0x8D]*4` (the `+0x8D` index is the
  record position, set by `FUN_0008C2E0` `0x8C324..0x8C336`); `FUN_0004A6BC`'s
  formats are `%s.fmt`/`%s.dat`/`%s.lfsh`/`%s.qfs` over the `0x107370` name
  table (not `t%s.dat`/`lay%s.fmt`), and the `.fmt` files are BIGF entries of
  `art/gameart0.pvi`; the phase cell `FUN_0006E1D0` maps each 4-byte record
  {opp x/z, own x/z} to `target = ((int8)x*0x26, 0, (int8)z*0x21)` with the
  own/opp pair chosen by the controlled side (`[0x157AAC]>>24`) and both
  components negated for team side 1. The engine now loads `352ko.fmt`
  (formation id 0) at match begin, seeds both teams
  (`fifa96_match_entities_seed_formation`) and commits the targets, so the
  records receive real non-zero positions and the M2 tape/`make game` draw.
  Still open: the front-end formation-id producer
  (`[0x14C1E4]`/`[0x14C1E5]`, BSS 0; the engine derives id 0), the
  `.dat`/`.lfsh`/`.qfs` slots and the `[team+0x7DB]`/`[team+0x7DF]`
  (`6*id+3`/`6*id+5`) pointers, the roster `+0x90` line code and the `+0x9A`
  marks (the kickoff commit `FUN_0008CF60` does not consume them).
* **OL-49 — `FUN_0008CEB8` index-11 overflow — resolved as a bounded model
  (Task 2).** The native helper clamps `last >= 0xB` to `0xB`
  (`0x8CEE2`/`0x8CEE7`), so a caller with `last >= 0xB` also stages record
  index 11 at `team+0x7A6` — the same spill the `0x2A` arm drives at its scan
  exhaustion (`0x8D7F6` exit, install code byte `team+0x837` = the next team
  block `+0x2`; Appendix B.4). All 15 static call sites pass `BX=0xA`, so the
  helper path is unreachable from the program; the derived pool models records
  0..10, so the helper keeps its clamp to 10 and the `0x2A` scan models
  exhaustion as `chosen831 = FIFA96_MATCH_ENTITY_NONE` + `arm2a_overflow = 1`
  with no pool record written. Nothing observable is lost: the aliased tail
  byte is outside the derived record surface. See Appendix B.4.
* **OL-52 — row-27 animation-selector record surface.** `fifa96_arm_anim_select`
  ports the non-zero id clamp and the native `kind == 0` special-byte keep
  rule (row bytes 0/0x62..0x65; Appendix D.5). The native `[rec+0x28]`
  row-pointer source — the derived `row` argument is a caller stand-in (the
  step passes its last resolved id, not the `0x10EF00` row byte) — the RNG
  reroll `RNG & 3 -> {0, 0x62, 0x65}` (which the native takes for a NULL
  pointer and for a non-zero non-special row byte), the record writes — row
  pointer `[rec+0x28]`, flag fan-out `+0x43/+0x44/+0x45/+0x46/+0x3F`, frame
  resolve `FUN_0006E490`, accumulator `[rec+0x32] = 0` — and the
  side-sensitive `[0x57A6C]` write stay unported. Row 27's `0x1103CB` ids are
  never 0, so the branch is unreachable from the row and its stage-2 `+0x44`
  consumer is the staged `flag44` stand-in. Full model: FU-84 §1.
* **OL-53 — row-27 pair-walk globals and bounds.** Native `[0x158782]` (the
  0..7 cycle, seeded `RNG & 7` by `FUN_0007D8D0` `0x7D8FC`) and the high word
  of `[0x10F372]` (`0x10F374`, the pair cursor, BSS zero) are process globals
  shared by every record; the derived record carries them per record. The
  native cursor is unbounded and walks dword pairs past the 96-byte `0x1103CB`
  table into the adjacent `0x11042B` data; the derived model stops at the 24th
  pair and sets `anim_overflow` instead (no adjacent-data read). Multi-record
  interleaving is not modelled.
* **OL-54 — `FUN_0007DAB4` remainder (reset callbacks and installer tail).**
  The full native reset also calls `FUN_00078B00([rec+0x20])` when the slot
  pointer is non-zero (`0x7DAD2`) and takes the phase-2 forced-decision arm
  `FUN_0007C990(rec)` when `[0x157A4A]>>24 == 2 && [rec+0x8D] != 0`
  (`0x7DAEF`; FU-141 OL-44 already documents the forced decision as the pool
  call). The `FUN_0007D9A4(rec,0,0,0)` tail may overwrite `+0x92` with its
  staged byte 0, clear `+0x9E`, copy `+0x79 -> +0x7B` and clear the carrier
  bit when it accepts the code; the derived subset adopts FU-141 §3.4's
  `stage92 = 0xFF` init-seed reading and represents the code-0 re-install as
  `code = 0` (Appendix E.3). `fifa96_arm_reset` models none of those; the
  pool's installer/drain owns their derived equivalent when a wired row
  requests the reset (Task 6/7).
  **Task 6 decision (pool drain).** The reset keeps the derived subset
  (`stage92 = 0xFF`, `timer89 = 0`, `code = 0`) as the record-visible
  intermediate and the drain does **not** replay a reset tail: the native
  accepted-install tail (`+0x92 = staged 0`, `+0x9E` clear,
  `+0x7B <- +0x79`, carrier bit) is unobservable from the derived dispatch
  because `code = 0` dispatches row `00` (which never reads `+0x92`) and any
  later re-install restages `+0x92` from its staged byte through
  `fifa96_match_entities_install` (FU-137 §2). What the engine drain does
  replay is the explicit install request a row's step records: row 29's
  `install = 3` is consumed by `fifa96_match_entities_install` (verified
  `3 -> 0x19` coercion when `active == 0`, `+0x92 = 0` staged, `+0x9E` clear,
  `+0x7B <- +0x79`), so the phase != 5 net state (code 3/0x19,
  `stage92 = 0`) is reproduced once Task 9 binds the row. The one carried
  divergence is the occupied record (`+0x9A != 0`): the native reset's code-0
  install is rejected (code unchanged, `stage92 = 0xFF`) while the derived
  subset writes `code = 0`; recorded here, not silently fixed (row 29's own
  `+0x9A` pre-check still suppresses its code-3 request).
* **OL-55 — row-29 remainder (Task 6).** The body's unmodeled surfaces: the
  `[0x157AA3] = [nearest+0]` cross-record store (`0x875A7`; only the process
  global is written, no derived consumer), the `0x36200` call's native
  `EAX = 2` value (the derived stub is the argument-less no-op, shared with
  OL-51), the `[rec+8]` phase-cell descriptor-pointer source and the `[rec+0]`
  team-base pointer source (the derived `cell[2][2]`/`team_candidates` are
  caller-supplied stand-ins), and the `[0x10F36C]` identity scope (the derived
  per-record `chase` bit models only "the global equals this record"; the
  global-points-at-another-record state is not representable per call — the
  same per-record-global treatment as row 27's OL-53). The `[0x157A9F]` ball
  pointer is likewise a caller-supplied position/skip pair.
* **OL-56 — row-28 process-global inputs and the match RNG seed (Task 7).**
  The body reads five process globals whose producers are unported:
  `[0x10F358]` (writers: `FUN_0007D8D0` init `0x7D97D`, row 2A `0x86AB0`/
  `0x86F70`; row-28 reads `0x87384`/`0x8746A`), `[0x10F35C]` (writers init
  `0x7D983`, `FUN_000886D4` `0x887EC`, row 2A `0x86AAB`/`0x86FFE`, the arm-0x2A
  installer clear `0x8D80E`; row-28 read `0x873CF`), `[0x10F364]` (`FUN_0008D098`
  entry block `0x8D6D4`/`0x8D71D`, init `0x7D995`; row-28 read `0x87159`),
   `[0x10F368]` (entry block `0x8D6DB`/`0x8D722` plus its `0x8D6FB` read-write,
   init `0x7D98F`; row-28 read `0x87161`) and `[0x157AC2]` (writers `0x73F82`,
   `0x4B0DE`, `0x4BA20/31`, `0x38990`, `0x88D6A`; entry-block read `0x8D69F`;
   row-28 read `0x8716B`). The derived engine keeps the five as run-level staging fields
   (`fifa96_match_run.global_*`) defaulting to 0; the derived step consumes them
   through `fifa96_arm_record.global_*`. The native match RNG seed is
   `FUN_0001D940(0x18)` = `settings[0x18]` at the match-init call `0x493F2`
   (FU-60 §0x493E8); the engine seeds its `struct fifa96_rng` with the derived 0
   in `fifa96_match_run_begin` (the corpus has no static default for the runtime
   settings block).
   **Status (Task 8): the `[0x10F35C]` arm-tail clear (`0x8D80E`) is ported in
   `fifa96_match_phase_machine_step` (FU-137 §5.2 errata); row 2A's arm 0/9/10
   writes to `[0x10F358]`/`[0x10F35C]` are staged through
   `mr->record.global_10f358/35c` and repacked to the run by the frame dispatch
   (H.6). The remaining producers (init/entry block, `FUN_000886D4`) stay
   unported.**
* **OL-57 — row-28 `[0x157AA3]` store and the `[rec+0x28]` row-byte gate
  (Task 7).** Arm 0 stores the team back-pointer `[rec+0]` into the process
  global `[0x157AA3]` (`0x8714A`; the same global row 29/2A write) — dropped,
  no derived consumer. The `0x87213..0x87237` gate reads `byte[[rec+0x28]]`
  (the current-row descriptor) and skips the constant `0x6E598` id 0x15 when it
  is already 0x15; the derived step uses `anim_sel` (the last resolved id) as
  the stand-in, and the `0x36200` call's native `EAX = 2` value stays the
  argument-less no-op of OL-51.
* **OL-58 — row-28 chosen-record resolution (Task 7).** The native
  `[team+0x831]` is a record **pointer** (the 0x2A scan stores `EAX =
  team+0xB2*(i+1)` at `0x8D801`; arm 2 copies `[that+0x59..0x61]` into the
  target at `0x87433`). The derived pool stores the Task-2 encoded id
  (`11*team+record` or NONE), so `match_run_dispatch_entity` resolves it to a
  `chosen_pos` triple (`chosen_ok`). When the id is NONE (the 0x2A overflow
  bounded model, OL-49) the derived arm 2 leaves the target unchanged instead
  of dereferencing the native aliased tail pointer — a bounded-model
  divergence, pinned by `test_arm_28_arm2_chosen_missing_bounded`. The native
  chosen-pointer chain into record 11 stays outside the derived record surface.
* **OL-59 — row-2A `[0x157AA3]` stores (Task 8).** The prologue (`0x86A65`,
  stage92 > 2) and arm 2 (`0x86B78`) store the record pointer into the process
  global `[0x157AA3]` — the same global rows 28/29 write (OL-57/OL-55). The
  derived arm drops both stores: the pool binds records directly and no derived
  consumer reads the global.
* **OL-60 — row-2A `+0x65` distance model (Task 8).** Row 2A never computes the
  distance; it reads the `+0x65` word (`[rec+0x63] SAR 16` at `0x86AFE` etc.)
  that the unported `FUN_0008D098` pre-switch walk writes (`0x8D11E`:
  `0x8DCD4(pos, target)` per non-occupied record, before the installer arms).
  The derived frame staging recomputes the word from the dispatch call's
  pos/target (H.5); the native one-frame staleness (the row body runs in the
  next frame's record-machine tail, after the walk) is not modeled. The walk's
  `+0x67`/`+0x69` writes are not restaged because no wired row consumes a
  walk-written lane (rows 26/27/28/29 recompute it in their own prologues).
* **OL-61 — the `0x513EC` camera stop (Task 8).** First-hand (`0x513EC..0x51440`,
  33 instructions; EXE operands, i.e. the FU-118 doc's `0x4E5xx` + `0x100000`)
  every effect is on the camera-mode/recorder block: the
  `[0x14E584]`/`[0x14E580]` clears (`0x513F9`/`0x513FF`), the `[0x14E5A8]`-gated
  callback through `[0x14E570]+0x38` (`0x51413`), `[0x14E578] = 2` (`0x51421`)
  and the `[0x14E574]` first-entry latch + `FUN_00064074` recorder clear
  (`0x5142B..0x51435`). The derived engine models none of those globals, so
  `fifa96_arm_camera_stop` is a documented no-op returning FIFA96_OK (FU-118
  surface; the camera-mode consumers `FUN_000505D0`/FU-103 stay unported).
* **OL-62 — staging-tail residual (M2 arms-and-wiring Task 10).**
  `FUN_0007A490 0x7AA3C..0x7AE2F` after the animation resolution: the per-code
  jump table flat `0x7A458` (14 arms: `0x7AACB/0x7AB83/0x7AC57/0x7AD0F/0x7ADA0/
  0x7ADBA/0x7ADEC`, five defaults to `0x7AE2F`), the `0x78B00` slot callback
  (`0x7AA37`), the `FUN_0007A084` reception body (the `receive` flag models the
  call), the `0x92820`/`0x8F188` sinks, `0x92AC8` RNG draws and the `0x157736`
  speed source. The derived `fifa96_ball_pair_stage_tail` ends at `0x7AA2F`
  (FU-139 §8.2/§8.7); its `0x6E598` `reserved45` (EBX) input feeds only the
  unmodeled `FUN_0006E490` frame resolve, so it is not the derived `row`
  stand-in and is not bound.
* **OL-63 — row-05 residual and non-wiring (Task 10).** Stage 0's target
  algebra `0x7F3A1..0x7F57B` (camera/local target copies, the `0x8DC68`
  accumulator, `FUN_00092820(rec,0x26)`, `FUN_00071C94`,
  `FUN_00079CCC`+`FUN_0006DA64`, the `0x15872D` write and the `0x157A4F` gate)
  and the `FUN_0007F7E0` fallback `0x7F7E0..0x801B2` (installs code 7/0x11,
  rotates `0x158729`) are unported; the plan's wiring gate therefore keeps
  `fifa96_match_action_table[0x05].fn` NULL (evidence names this leg; FU-137
  §6.1/§7 Task-10 errata, FU-139 §8.6). The `0x7F19F` `+0x9E` latch is tracked
  through `out.ran_set` so a future wiring cannot drop it silently.
* **OL-64 — row-05 stage 0 → 1 edge (Task 10).** The row-05 body writes `+0x92`
  only at `0x7F5D9`/`0x7F616`/`0x7F630`; the stage-0 path never advances the
  latch, so the native 0→1 transition is an external re-install of code 5 whose
  caller is not statically located in the `0x7F194..0x7F7C9` window. The port
  treats `+0x92` as an input (FU-139 §8.7).

* **OL-65 — row 07/0F unmodeled record/presentation auxiliaries (Task 11).**
  The machines in `fifa96_action_kick_machine` emit tracked requests for the
  native calls whose bodies stay unported: the `0x78B00` slot clear
  (`slot_callback`), the `0x78A84`/`0x78AA4` slot backup/restore
  (`slot_backup`/`slot_restore`; the engine's FU-70 slot block has no
  `+6/+10` pair), the `0x79B58` receiver timer (the engine sets the pool
  receiver's `+0x93 = 0x10` directly), the `0x79B1C` snap and the `0x79B6C`
  re-anchor (modelled through the target/face outputs), the `0x7C990` install
  decision (ported bounded, `reset_code`) and the `0x7E600` defender decision
  (ported bounded; the `0x71B9C` predictor triple is a caller input, FU-139
  §9.4). The record bytes `+0x44` (row 0F anim-row terminal), `+0x85`/`+0x87`
  (row 0F reload operands), `+0x99`, `+0x9D`, the roster descriptor bytes
  `rec[+4][+0xD..+0x15]`, the `[0x1577CA]` decision exclusion, the
  `[0x14C32A]`/`[0x15B680]` downgrade gates, `[0x1586D7]`, the team `+0x7CB`
  callback record and the `[0x157A4D]` mode-state byte are all staged zero
  (their producers are unported); `word[+0x77]` (the lane bound) has no pool
  producer, so the derived row-0F stage-1 lane gate waits (the loader fixture
  pins the bounded pass).
* **OL-66 — kick-path external block inputs (Task 11).** The two mode arms
  `FUN_0007B194`/`FUN_0007B57C` read the camera block (`0x15774C/50/54`), the
  goal-side phase `[0x157A49]`, the mode-state `[0x157A4D]`, the
  `[0x14C2F6]`/`[0x14C326]` gates and the per-side `0x14C1D4` range words
  (zero in the image; runtime-populated by an unported producer); the
  `0x8DE8C` nearest arm takes the caller's team candidates. The engine passes
  its FU-71 camera triple and zeros for the rest; the `0x71B9C`/`0x70B94`
  predictor block and the `0x6DBCC` corner table (`[team+0x7DB]` +
  `0x1577D6`) are caller inputs. `FUN_0007E600`'s `0x110680` type gate, the
  `0x1104BB` recompute table, the `0x1104CA` sector mask and the four 10-byte
  event-row tables (0x1102FE/0x11016E/0x110196/0x11024A) are embedded from
  the EXE image in `fifa96_match_handlers.c` (first-hand reads).
* **OL-28/OL-31 status (Task 11).** OL-28 (the `FUN_0007B9C4` target
  selection + mode arms + code-4 RNG/divisor) and OL-31 (the row 07/0F kick
  machines) are ported as `fifa96_ball_kick_target` and
  `fifa96_action_kick_machine` with rows 07/0F wired (FU-139 §9); the
  residuals are OL-62 (staging-tail algebra, shared) and OL-65/OL-66 above.
* **OL-38 status (Task 14, Appendix K).** The machine subset is ported and
  tested: `fifa96_outfield_input_row` (`0x7CABA..0x7CC82` + the no-edge arm
  `0x7CC13..0x7CC7D` + the `0x7CC82..0x7CD24` forced-decision/chase tail),
  `fifa96_outfield_chase_gate` plus the per-type gate flat `0x110680`
  (`fifa96_outfield.c`), with the input tables already ported by FU-75. The
  rows 04/08 full record-visible bodies are **not** ported; per the plan's
  split rule (a row window exceeding one task) they are registered below as
  OL-70 (row 04) / OL-70a (row 08), so rows 04/08 stay `fn == NULL`
  (`-FIFA96_ERR_UNSUPPORTED`). Both split rows are now closed: row 04 by
  Task 1 (K.5) and row 08 by Task 2 (K.6), both wired over the FU-141 pool;
  the machine subset stays a separate unwired seam.
* **OL-41 status (Task 14, Appendix K).** Closed: `FUN_0008D824` is
  `fifa96_entity_intercept_bind` and `FUN_000795B4` is
  `fifa96_entity_intercept_band` (`fifa96_entity_update.{h,c}`), sharing the
  moved-down `0xCD474`/`0x114E04` primitives
  (`fifa96_entity_angle`/`fifa96_entity_sine`); `team_select_intercept` now
  feeds `team+0x7BE` (`0x8DA94..0x8DAE7`). The slot-rejected NULL-record band
  read is OL-71.
* **OL-70 — row 04 full record-visible body (split from OL-38, Task 14).**
  Row 04's handler `0x7E7C8..0x7F141` is 574 Ghidra defined-code instructions
  (the textual span also covers inline jump-table/padding data; FU-77 §2.4
  counts 649 across the span): camera-lead arm, ranked `FUN_0008DDE0` pick,
  wing vectors
  `0x157794`/`0x157788`, `0x92AC8` RNG gates, `0x8DCD4`/`0x8DC68` metrics,
  `0x741B4` score fold, `0x7876C`/`0x78A84`/`0x78AA4` slot calls,
  `0xE600` decision, installs 4/0x19/0xF/0xB/7/6/5).
  **Status (M2 playability-legs Task 1, Appendix K.5): the full
  record-visible body is ported as `fifa96_outfield_row04_step` and wired as
  `fifa96_match_action_04` over the FU-141 pool; row 04 flips to `ported`
  (`FIFA96_OK`). The body is standalone — the `0x7CA54` machine subset
  (`fifa96_outfield_input_row`/`_chase_gate`, Appendix K.3) belongs to the
  record machine and stays a separate unwired seam. The unmodeled
  record/team bytes, the `0x1577F0..0x157806` track words, the `0x71B9C`
  predictor and the event/audio sinks are OL-72 (K.5.8).**
* **OL-70a — row 08 full record-visible body (split from OL-38, Task 14).**
  Row 08's handler
  `0x81068..0x814AF` is 213 Ghidra defined-code instructions (about 231 across
  the span with its inline data) (stage machine 0/1/2, camera+lead
  stage-0 target, `0x79C50` face, `0x6E598` anim, the facing-projection
  `0x795A4` scan, `0x8ED40`/`0x8F188`/`0x92820`/`0x7A490` requests,
  `0x79B1C` snap, the new record fields `+0x3D`/`+0x44`/`+0x7D` and the
  `rec[+4]` descriptor bytes `+0xC`/`+0x16`, the `[0x15877D]`/`[0x15872F]`
  process bytes). Both bodies are bounded spans but neither fits the task's
  remaining budget; porting them and wiring rows 04/08 is the follow-up
  scheduled from this split (one task per row: OL-70 row 04, OL-70a row 08).
  Until then the rows dispatch `-FIFA96_ERR_UNSUPPORTED` and the FU-137 §6.1
  evidence names the row's leg.
  **Status (Task 1): row 04 is ported and wired (K.5); OL-70 closed, row 08
  still OL-70a.**
  **Status (M2 playability-legs Task 2, Appendix K.6): the full record-visible
  body is ported as `fifa96_outfield_row08_step` and wired as
  `fifa96_match_action_08` over the FU-141 pool; row 08 flips to `ported`
  (`FIFA96_OK`). Row 08 installs no code (no `FUN_0007D9A4` call), so the
  wiring gate's install-arm clause is vacuous and the body is standalone — the
  `0x7CA54` machine subset stays the record machine's separate unwired seam.
  The unmodeled record/process bytes, the `0x7A490` staging and the
  event/audio sinks are OL-82 (K.6.7); the +0x3D frame gate keeps the
  projection scan inert live until the OL-80 producer lands.**
* **OL-72 — row-04 unmodeled inputs/sinks (Task 1, from Appendix K.5).** The
  row-04 body reads record bytes the FU-141 pool does not model (`+0x99`,
  `+0x9D`, `+0x44`, `[[rec+0x28]]`, the `rec[+4]` descriptor byte `+0xE`), the
  team bytes `+0x7C7`/`+0x7CB`/`+0x7D7`/`+0x7E7` and the `+0x7E8` corner
  triples, the `[0x1586D7]` merge gate, the `[0x157ABE]` half-flip, the
  `0x1577F0..0x157806` track words, the `0x1577C0/C2` lead words and the
  `0x71B9C(4)` predictor, and calls the unported event/sound sinks
  (`0x974DC`, `0x8F188`, `0x92820`, `0x71C94`, `0x974F0`, `0x651F0`). The
  engine stages zero / camera stand-ins and the step returns them as requests;
  the affected branches (`0xF` unlock, the `0x1F` tilt threshold, corner,
  `team_7e7` install 7, `3*team_7D7`, the `0x1577F0` bands, the predictor
  gates) stay inert until the producers land. `[0x158777]` (the carrier) and
  `[0x1577CA]` (the ball track) are pool stand-ins; the `0x6E598`/`0x78A84`/
  `0x78AA4` requests stay OL-52/OL-65.
* **OL-71 — interception NULL-record band read (Task 14).** When the nearest
  is rejected by the `+0x20` slot gate, the native clears `[team+0x7BA]`
  (`0x8DA66`) and still runs `FUN_000795B4` with the NULL record, reading
  absolute low-memory words at `0x4D..0x69` (`LEA EDX,[EAX+0x4D]` /
  `ADD EAX,0x59` with EAX=0). The derived engine stages a zero record there
  (band word 0, dx/dz 0, lane/height 0), so the gate refuses; those absolute
  words have no derived producer.
* **OL-81 — `ac5`/`ac7` word-width carry (T2 review).** The native 3/0x25
  decision compares the full 16-bit words `[0x157AC5]`/`[0x157AC7]`
  (`0x8D76C MOV AX,[0x157AC5]`; `0x8D772 CMP AX,word [0x157AC7]`), but
  `struct fifa96_match_phase_machine` stores `ac5`/`ac7` as `uint8_t` low
  bytes (Appendix B.3), so the port compares bytes and loses the high bytes.
  No producer binds either word yet (both fields are seeded 0), so the
  divergence is currently unobservable; when a real `[0x157AC5]`/`[0x157AC7]`
  binding lands the fields/compare must be forced to 16-bit, or the
  divergence must be accepted explicitly and recorded then. Referenced from
  the `ac5`/`ac7` field comment in `fifa96_match_phase_machine.h`.
* **OL-83 — `record.type`/`record.actor_type` split for the native `+0x8E`
  byte (T2 review).** Both engine fields model the native byte at `+0x8E`
  (`0x79C50` writes it at `0x79C8E`; row 08's type tables and its 0x6E598
  argument read `[rec+0x8B]>>24`), but
  `struct fifa96_match_run_record.actor_type` is only *read* from the pool
  (`fifa96_match_run.c` `r->actor_type = e->actor_type`) and nothing ever
  writes `e->actor_type`, while rows 28/2A (their `rec.type` staging), the
  0x79C50 face repacks (row 07/0F) and the row-08 face persist the byte
  through `record.type` (`fifa96_match_handlers.c`; `fifa96_match_run.c`
  `e->type = r->type`). Rows 04/06/07/18 read `r->actor_type` /
  `mr->record.actor_type` as their `type8` (`handlers.c` `match_kick_from_record`,
  rows 18/04/06). So a row-08 face write is invisible to the rows that
  natively share the byte, and those `type8` reads are always 0 in production.
  Options for the reconciliation task: (a) unify on one field
  (`record.type`/`e->type`) and repoint the four readers; (b) make the
  producers write both fields; (c) declare the divergence verified and keep
  it if every reader's staged byte is proven unused on the reachable paths.
  The divergence is **currently unobservable on the tape**: `make check`
  104/104 at `9f63258` is byte-identical on the M2 tape (`test_engine_m2`,
  `mask_final == M2_WIRED_MASK`), no golden moved, and the zeroed pool leaves
  `actor_type` 0 for every dispatch. Referenced from the row-08/row-04
  FU-137 §6.1 evidence and the K.6.5/K.6.7 notes (the byte/field split also
  carries the row-28/0F `record.type` writes).

## 7. Refinements to FU-137 (to be recorded as errata in the port slices)

* **§6.1 row `2C`** — "prologue-only body" is wrong: `0x84598..0x8462D` is a
  real 48-instruction stage machine (stage latch `+0x92` 0/1/2; calls
  `0x6E598`, `0x7DAB4`). Its *entry* is still unresolved (OL-48).
  **Status (Task 5): the body is ported as `fifa96_arm_2c_step` with the
  shared `fifa96_arm_reset` (Appendix E); the row stays unwired (OL-48) and
  its remainder is OL-54.**
* **§6.1 row `2B`** — the one-byte RET is the final instruction of the row-29
  body (shared epilogue at `0x87738`), not a standalone stub; the two helpers
  that follow (`0x8773C`, `0x8776C`) belong to no action row.
  **Status (Task 9): confirmed dead — the installer census passes no 0x2B
  (Appendix I.8); FU-137 §6.1 now classes it `dead entry` and OL-15 is closed
  by the verdict.**
* **§6.1 row `29`** — the body exists and self-installs code 3 via
  `FUN_0007D9A4` (`0x8753C`); "no match-code reference" remains true only for
  an *installer of* 0x29, so the row's open-leg status is unchanged.
  **Status (Task 6): the body is ported as `fifa96_arm_29_step` (Appendix F);
  the entry stays unresolved (OL-48, F.3) and the row's remainder is OL-55.**
* **§5.2 arm `0x28` condition** — the arm fires when `[0x157A4A]>>24 != 0x13`
  in the state 0x13/0x14 block (i.e. the 0x14 half); when phase is 0x13 the
  0x25/3 arms run instead (first-hand `0x8D75F..0x8D7CF`). **Status (Task 7):
  the arm's full body is ported and wired (Appendix G, `fifa96_match_action_28`
  with `fifa96_arm_28_step`).**
* First-hand instruction counts for the seven row windows are in §1; FU-137's
  "body unanalyzed/unported" cells can cite §1/§3 of this doc when they are
  updated.
* **§5.2 arm `0x2A` tail (Task 8).** The arm clears `[0x10F35C]` at
  `0x8D80C..0x8D80E` on both the found and the overflow fall-through path;
  Task 2's port omitted the write (OL-56 recorded it as an unported writer) and
  Task 8 completed it in `fifa96_match_phase_machine_step` because row 2A's
  arm 10 sets the flag and the derived frame reads it back. See the FU-137
  §5.2 errata and Appendix H.
* **§2 rows 28/2A `0x36200` "stub"** — first-hand it is
  `MOV [0x105FC4],EAX; RET` (5-byte instruction + RET), not a bare no-op; the
  store gates the unported camera/coordinate step `FUN_00036208`
  (`0x3621E CMP dword [0x105FC4],1; SETZ/JZ return`; the `0x36211` gate above
  tests `[0x105FB4] < 0`, FU-62 §3.3). Task 3 ports the
  derived surface as a documented no-op (OL-51) because neither row 26 nor the
  engine models `0x105FC4`; the callers' appendices (Task 7/8) pin the value.
  **Status (Task 7): Appendix G pins row 28's call value (`EAX = 2`, `0x8714F`)
  and keeps the derived no-op (the global stays OL-51).**
* **§5.1 loop bound** — FU-137's pseudo-code `for (i = first; i <=
  min(last,10); i++)` is off by one against the first-hand bytes: the native
  clamp is `last >= 0xB -> 0xB` (`0x8CEE2` `CMP ESI,0xB` / `0x8CEE7`
  `MOV word [ESP+4],0xB`; raw `83 FE 0B 7C 07 66 C7 44 24 04 0B 00`), i.e.
  `min(last,11)` with a first guard of `first in 0..10`. The port clamps to 10
  because the derived pool has no index 11 (OL-49).
* **§5.2 state vs phase** — the state switch byte `[0x157A4D]` (`0x8D178`) and
  the phase `[0x157A4A]>>24` are the *same* native byte (`0x157A4D` is byte 3
  of the `0x157A4A` dword; both 0x13/0x14 switch entries target `0x8D693`).
  The Task 2 port keeps the plan's two-field gate (`pm->state` +
  `mr->state.phase`) and reads the arm-block phase from `mr->state.phase`
  (Appendix B.2/B.3). The native `0x26` side compare is a zero-extended side
  byte against an arithmetic-shifted controlled byte, and the 3/0x25 compare
  is a full 16-bit word compare; both port asymmetries are recorded in
  Appendix B.3 and the FU-137 §5.2 errata.
* **§5.3 constant census (Task 9)** — the census's "no installer arm found" is
  refined from a constant-only search to a whole-call-site classification:
  all 77 `FUN_0007D9A4` call sites have a bounded EDX code set (Appendix I.3)
  and no stored installer pointer exists (I.6), so the installer code domain
  is {0x00..0x26} ∪ {0x28,0x2A} and 0x27/0x29/0x2B/0x2C are never passed.
  FU-137 §5.3 carries the corresponding errata.

## Appendix A (FU142a / M2 arms-and-wiring Task 1) — `FUN_0008CEB8` first-hand window

Task 1 of the follow-up plan (`docs/superpowers/plans/2026-10-07-fifa96-m2-arms-and-wiring.md`)
ports the helper. This appendix is its evidence gate: the exact fields,
constants and the 15 call-site shapes below are read first-hand this slice and
are what the port (`fifa96_match_arm_install_multi`, `fifa96_match_phase_machine.c`)
implements.

### A.1 Tool calls (Ghidra read-only, explicit `/FIFA96.EXE`)

* `decompile_function 0x8CEB8`, `disassemble_function 0x8CEB8` — 52
  instructions, body `0x8CEB8..0x8CF5D` (the FU-142 §2 span; final `RET 0x4`
  at `0x8CF5B`), `FUN_0008D098` sole caller;
* `read_memory 0x8CEE2` (32 B) — raw clamp/guard bytes (the quoted clamp
  slice starts at the `CMP ESI,0xB`);
* `get_xrefs_to 0x8CEB8` — 15 refs, all `UNCONDITIONAL_CALL` from
  `FUN_0008D098`;
* `disassemble_bytes` windows: `0x8D170..0x8D1D0` (`0x8D19F`, `0x8D1C1`),
  `0x8D280..0x8D2D0` (`0x8D2B3`), `0x8D340..0x8D380` (`0x8D36B`),
  `0x8D400..0x8D448`/`0x8D430..0x8D4C0` (`0x8D45C`, `0x8D489`, `0x8D4B2`),
  `0x8D550..0x8D610` (`0x8D58B`, `0x8D5E9`), `0x8D620..0x8D6A0` (`0x8D64B`,
  `0x8D681`), `0x8D720..0x8D824` (`0x8D74D`, `0x8D78B`, `0x8D7AD`, `0x8D7CF`).

No writes: no rename/comment/label/function/script/project save. Repo:
`make check` 100/100 before, 101/101 after (new
`test_engine_match_phase_machine`).

### A.2 Register contract (re-derived from the 52 instructions)

Prologue `PUSH ESI; PUSH EDI; SUB ESP,8`; BX is parked at `[ESP+4]`, CX at
`[ESP]`; the skip argument is read back at `[ESP+0x14]`.

| register | meaning | first-hand site |
|---|---|---|
| EAX | team block base (record 0) | `0x8CEF7 ADD ESI,EAX` |
| DX | first record index (signed word) | `0x8CEC6 TEST DX,DX` |
| BX | last record index (signed word) | `0x8CEC2 MOV [ESP+4],BX` |
| CX | install code (word) | `0x8CEC2 MOV [ESP],CX` |
| stack `[esp+4]` | skip-if-current code (word) | `0x8CF04 MOV ECX,[ESP+0x14]` |

### A.3 Decision logic (site-annotated)

* **first guard** `0x8CEC6..0x8CED5`: `first < 0` or `first >= 0xB` exits
  (`TEST DX,DX / JL`; `MOVSX ESI,DX / CMP ESI,0xB / JGE`).
* **last clamp** `0x8CEE2..0x8CEED`: `last >= 0xB` becomes `0xB`; raw bytes
  `83 FE 0B 7C 07 66 C7 44 24 04 0B 00` (`CMP ESI,0xB` at `0x8CEE2`,
  `MOV word [ESP+4],0xB` at `0x8CEE7`).
* **record walk** `0x8CEEE..0x8CF45`: record pointer `base + i*0xB2`
  (`IMUL ESI,EDI,0xB2` 0x8CEF1), advanced by `ADD ESI,0xB2` (0x8CF45);
  loop test `EDI <= SAR([ESP+2],0x10)` (`0x8CF4B..0x8CF54`).
* **skip occupied** `0x8CEFB`: `CMP byte [ESI+0x9A],0 / JNZ`.
* **skip current code** `0x8CF04..0x8CF13`: `MOVSX DX,byte [ESI+0x91]` (the
  record's `+0x91` sign-extended) vs the skip word; equal -> skip.
* **index-0 code-3 pre-coercion** `0x8CF15..0x8CF25`: `TEST EDI,EDI / JNZ`;
  parked install code `SAR([ESP-2],0x10) == 3` -> `EDX = 0x19`. It keys on
  the loop index, not on the record's `+0x8D` active flag (the installer's
  own inactive-3 coercion in `fifa96_match_entities_install` would run after).
* **stage** `0x8CF2A..0x8CF3F`: `EAX = rec`, `EDX = code`, `ECX = 0`,
  `EBX = 0` -> `CALL FUN_0007D9A4` (FU-137 §2: not invoke-now, staged byte 0).
* Native return is void; the derived C surface returns the staged count
  (records whose `fifa96_match_entities_install` returned 1).

### A.4 The 15 call sites (all `EAX=EBP`, `EBX=0xA`)

| site | first | code | skip | shape |
|---|---|---|---|---|
| `0x8D19F` | 0 | 0 | `0xC` | state 0/0xA/0x0F arm (target `0x8D192`) |
| `0x8D1C1` | 0 | 3 | -1 | state arm |
| `0x8D2B3` | 0 | 3 | -1 | side compare follows (`0x8D2B8` vs `[EBP+0x826]`) |
| `0x8D36B` | 0 | 3 | -1 | side compare follows |
| `0x8D45C` | 1 | 0x15 | -1 | side-equal branch (`0x8D449`) |
| `0x8D489` | 0 | 0 | -1 | side-mismatch branch of the same compare |
| `0x8D4B2` | 0 | 3 | -1 | side compare follows |
| `0x8D58B` | 0 | 3 | -1 | side compare follows |
| `0x8D5E9` | 0 | 3 | -1 | side compare, then phase==9 compare |
| `0x8D64B` | 0 | 0 | -1 | side-mismatch loop edge (`0x8D63E..0x8D66F`) |
| `0x8D681` | 1 | 0 | -1 | side-equal counterpart of `0x8D64B` |
| `0x8D74D` | 0 | 0x26 | -1 | state 0x13/0x14 + `[EBP+0x826] != [0x157AAC]>>24` |
| `0x8D78B` | 0 | 3 | -1 | phase `== 0x13` + `word[0x157AC5] == word[0x157AC7]` |
| `0x8D7AD` | 0 | 0x25 | -1 | phase `== 0x13` + the two words differ |
| `0x8D7CF` | 0 | 0x28 | -1 | phase `!= 0x13` (the 0x14 half) |

Every site pushes `last` as `0xA`; only the state-arm `0x8D19F` uses a
non-`-1` skip (`0xC`), and only `0x8D45C`/`0x8D681` start at record 1.

### A.5 Port mapping and derived surfaces

* `fifa96_match_arm_install_multi(pool, team, first, last, code, skip_code)`
  in `src/fifa96_engine/fifa96_match_phase_machine.c`; the loop bound is
  `min(last, FIFA96_MATCH_ENTITY_RECORDS-1)` = `min(last,10)` for the reasons
  in OL-49; `first >= FIFA96_MATCH_ENTITY_RECORDS` returns 0 (0x8CED2).
* `pool->phase` supplies the installer's `[0x157A4A]>>24` argument.
* `struct fifa96_match_team` gains `flag830` (`+0x830`) and `chosen831`
  (`+0x831`, encoded entity id or NONE; `fifa96_match_entities_init` seeds
  NONE like `target`/`second`/`chosen`/`intercept`). Task 1's helper does not
  write them; the Task 2 `0x2A` arm does (`0x8D801`).
* `struct fifa96_match_phase_machine` holds the `FUN_0008D098` gate globals
  (`[0x157A4D]`, `[0x157A4A]>>24`, `[0x157AAC]>>24`, `[0x157AC5]`/`[0x157AC7]`
  low bytes) plus `arm2a_overflow` and the per-team `chosen831` cache;
  `fifa96_match_phase_machine_init` zeroes it and seeds the cache NONE.
* Tested: `tests/test_engine_match_phase_machine.c`
  (`test_init_defaults`, `test_install_multi_stages`, `test_install_multi_skips`,
  `test_install_multi_record0_coerce`, `test_install_multi_bounds`).

Write set of this appendix (Task 1): `include/fifa96_engine/fifa96_match_phase_machine.h`,
`src/fifa96_engine/fifa96_match_phase_machine.c`,
`include/fifa96_engine/fifa96_match_entities.h` (team tail fields + init seed),
`tests/test_engine_match_phase_machine.c`, `CMakeLists.txt`, this appendix and
the §6/§7 additions. §8 below describes the Task 9 probe slice, not this one.

## Appendix B (FU142a / M2 arms-and-wiring Task 2) — `FUN_0008D098` state 0x13/0x14 arm block first-hand window

Task 2 of the follow-up plan ports the state 0x13/0x14 subset of the 1928-B
`FUN_0008D098` phase machine — the installer arms `0x26`/`0x28`/`0x2A` and the
context arms `3`/`0x25` — into `fifa96_match_phase_machine_step`. This appendix
is its evidence gate: the exact fields, constants and branch sites below are
read first-hand this slice and are what the port implements.

### B.1 Tool calls (Ghidra read-only, explicit `/FIFA96.EXE`)

* `get_function_by_address 0x8D098` — defined, body `0x8D098..0x8D820`
  (1928 B);
* `disassemble_bytes` windows: `0x8D098` (220 B: prologue, pre-switch record
  walk, state switch), `0x8D174` (140 B: switch targets), `0x8D600` (300 B:
  the state 0x13/0x14 entry block `0x8D693..0x8D727`), `0x8D720` (260 B: the
  arm block `0x8D722..0x8D820`);
* `read_memory 0x8D040` (88 B = 22 dwords, the state jump table);
* `get_xrefs_to 0x8D098` — 2 refs, both `FUN_000740A0` (`0x740C8`,
  `0x740DB`); `get_xrefs_to 0x8D693` — table-data refs `0x8D08C`/`0x8D090`
  (entries 0x13/0x14) plus the computed jump `0x8D18A`;
* `disassemble_function 0x740A0` (25 insns, the per-team caller).

No writes: no rename/comment/label/function/script/project save.

### B.2 State dispatch and entry block

Prologue `PUSH EBX/ECX/EDX/ESI/EDI/EBP; SUB ESP,0x100; MOV EBP,EAX`
(`0x8D098..0x8D0A4`) takes EAX = team block base. Before the switch the
function runs a record walk over the 11 records (skip `+0x9A`; calls
`FUN_0006D920`, the record's `+0x1C` callback and `FUN_0008DCD4`) at
`0x8D0A6..0x8D166` — common to every state and outside this slice's subset
(the helpers are unported; `0x8DCD4` is Task 3).

State switch: `0x8D178 MOV AL,[0x157A4D]`; `0x8D17D CMP AL,0x15`;
`0x8D17F JA 0x8D814` (epilogue); `0x8D185 AND EAX,0xFF`;
`0x8D18A JMP dword CS:[EAX*4+0x8D040]`. The 22-entry table (read first-hand)
sends **both 0x13 and 0x14 to `0x8D693`** (entries `0x8D08C`/`0x8D090`);
0x15 goes to `0x8D77B` (the bare code-3 arm), 0x11/0x12 to the epilogue. The
switch byte is byte 3 of the `0x157A4A` dword (`0x157A4A + 3 = 0x157A4D`), so
`0x8D75F MOV EAX,[0x157A4A]; SAR EAX,0x18` re-reads the value the switch
selected on: **state and phase are the same native byte**.

Entry block `0x8D693..0x8D727` (runs before the arms; out of the derived
subset — row-28/2A body context): `MOV EAX,0xD; CALL 0x651F0`
(`0x8D693`/`0x8D698`); `MOV AL,[0x157AC2]; CMP EAX,4; JGE 0x8D703`
(`0x8D69F..0x8D6A7`); below 4 an RNG block (`CALL 0x92AC8`) writes
`[0x10F364]`/`[0x10F368]`; at/above 4 the side test
`CMP byte [EBP+0x826],0` selects `[0x10F364] = 0xFFFFFD30` (side 0) / `0x2D0`
(side 1) and `[0x10F368] = 0x5A0` (`0x8D703..0x8D722`). Those globals belong
to the FU-142 §1 row-28 body; the derived step does not model them (Task 7).

### B.3 Arm block decision logic `0x8D728..0x8D820` (site-annotated)

* **side test** `0x8D728 MOV EDX,[0x157AAC]`; `0x8D72E XOR EAX,EAX`;
  `0x8D730 SAR EDX,0x18`; `0x8D733 MOV AL,byte [EBP+0x826]`;
  `0x8D739 CMP EAX,EDX`; `0x8D73B JZ 0x8D75F`. Native equality is
  `zero_extend(side) == sign_extend(byte3 [0x157AAC])`.
* **arm 0x26** (side mismatch) `0x8D73D..0x8D75E`: `PUSH -1`,
  `MOV ECX,0x26` (`0x8D73F`), `MOV EBX,0xA`, `MOV EAX,EBP`, `XOR EDX,EDX`,
  `CALL FUN_0008CEB8` (`0x8D74D`), then the full epilogue
  `ADD ESP,0x100; POP EBP/EDI/ESI/EDX/ECX/EBX; RET` (`0x8D752..0x8D75E`) —
  the per-team call ends there; no later arm runs for that team.
* **phase test** `0x8D75F..0x8D76A`: `[0x157A4A]>>24 == 0x13` -> the
  3/0x25 pair; else `0x8D7BF` (the 0x28/0x2A half).
* **word compare** `0x8D76C MOV AX,[0x157AC5]`;
  `0x8D772 CMP AX,word [0x157AC7]`; `0x8D779 JNZ 0x8D79D`.
* **arm 3** (words equal) `0x8D77B..0x8D79C`: `PUSH -1`, `MOV ECX,3`
  (`0x8D77D`), `MOV EBX,0xA`, `MOV EAX,EBP`, `XOR EDX,EDX`,
  `CALL FUN_0008CEB8` (`0x8D78B`), epilogue.
* **arm 0x25** (words differ) `0x8D79D..0x8D7BE`: same shape, `MOV ECX,0x25`
  (`0x8D79F`), `CALL` (`0x8D7AD`), epilogue.
* **arm 0x28** (phase != 0x13) `0x8D7BF..0x8D7D3`: `PUSH -1`,
  `MOV ECX,0x28` (`0x8D7C1`), `MOV EBX,0xA`, `MOV EAX,EBP`, `XOR EDX,EDX`,
  `CALL FUN_0008CEB8` (`0x8D7CF`); no RET — execution falls into the 0x2A
  scan (B.4).

Hazards (recorded, not silently fixed):

* **side-test asymmetry.** The native equality holds iff
  `side_controlled < 0x80 && side == side_controlled` (the arithmetic shift
  makes a controlled byte >= 0x80 negative, while the zero-extended side is
  0..255). The port implements the plan's byte compare
  (`(uint8_t)pm->side_controlled != team->side`); the two forms differ only
  when `side == side_controlled >= 0x80`, where the native installs 0x26 and
  the byte model would not. The pool's team `side` is 0/1 (`init` seeds
  `(uint8_t)t`), so the divergence is unreachable; the Task 2 fixture pins the
  plan's required edge (`side_controlled = 0x80`, `side = 0` -> both teams
  take the 0x26 arm).
* **word vs low-byte `[0x157AC5]`/`[0x157AC7]`.** The native compares the full
  16-bit words (`MOV AX`/`CMP AX`); Task 1's `ac5`/`ac7` fields store the low
  bytes, so the port compares bytes. The forms differ only when the two words
  differ solely above bit 7.
* The 0x26 arm and the 3/0x25 arms end the per-team call with a RET; only the
  phase-0x14 half continues into the scan.

### B.4 The 0x2A scan and the bounded overflow model (resolves OL-49)

Native `0x8D7D4..0x8D820`:

| site | instruction |
|---|---|
| `0x8D7D4` | `MOV EDX,1` (scan starts at record 1, not 0) |
| `0x8D7D9` | `LEA EAX,[EBP+0xB2]` (record 1) |
| `0x8D7DF` | `JMP 0x8D7F0` (loop test first) |
| `0x8D7E1` | `CMP byte [EAX+0x9A],0` |
| `0x8D7E8` | `JZ 0x8D7F8` (first `+0x9A == 0` wins) |
| `0x8D7EA` | `INC EDX` |
| `0x8D7EB` | `ADD EAX,0xB2` |
| `0x8D7F0` | `MOVSX ECX,DX` |
| `0x8D7F3` | `CMP ECX,0xB` |
| `0x8D7F6` | `JL 0x8D7E1` (loop while index < 11) |
| `0x8D7F8` | `MOV EDX,0x2A` |
| `0x8D7FD` | `XOR ECX,ECX` (staged byte 0) |
| `0x8D7FF` | `XOR EBX,EBX` |
| `0x8D801` | `MOV [EBP+0x831],EAX` (chosen record pointer) |
| `0x8D807` | `CALL FUN_0007D9A4` (install 0x2A into EAX) |
| `0x8D80C` | `XOR EBX,EBX` |
| `0x8D80E` | `MOV [0x10F35C],EBX` (row-28 global, not modeled) |
| `0x8D814..0x8D820` | epilogue `ADD ESP,0x100; POP *; RET` |

Exhaustion: when no record 1..10 has `+0x9A == 0` the loop exits at
`EDX = 0xB` with `EAX = EBP + 0xB2*11 = team+0x7A6` (record 11, one past the
11-record array: 11 * 0xB2 = 0x7A6), stores that pointer into `+0x831` and
installs 0x2A there — the code byte is `record11 + 0x91 = team+0x837`, i.e.
**the next team block + 0x2** (team 0: `0x1588A4 + 0x837 = 0x1590DB =
0x1590D9 + 2`).

Bounded model (OL-49 resolution): the derived pool models records 0..10 only.
On exhaustion the step writes `chosen831 = FIFA96_MATCH_ENTITY_NONE` and
`arm2a_overflow = 1` and stages nothing; any scan that finds a record clears
the flag (it reports the latest scan's outcome). On a find, `chosen831` holds
the derived encoded id `11*team + record` (the pool id scheme) instead of the
native pointer, and the direct stage is
`fifa96_match_entities_install(&records[i], pool->phase, 0x2A, 0)`. The 0x28
arm does not set `+0x9A`, so (exactly as natively) the scan may pick the
record the 0x28 arm just staged and overwrite it with 0x2A; record 0 is never
eligible because the scan starts at 1.

### B.5 Caller and cadence

`get_xrefs_to 0x8D098` = 2, both in `FUN_000740A0` (`0x740C8`, `0x740DB`).
It does `MOV AH,[0x157A4D]; MOV [0x157A4E],AH; MOV [0x157A4D],AL` (installs
the new state, saves the previous), then
`EAX = 0x1588A4 + DX*0x835; XOR DL,0x1; CALL FUN_0008D098` and repeats with
the flipped side (`0x740B4..0x740DB`), then handles phase 2 (`0x740E0`). One
state transition therefore invokes the machine once per team; the derived
frame hook calls `fifa96_match_phase_machine_step` once per granted frame and
the step loops both team blocks.

### B.6 Port mapping and derived surfaces

* `struct fifa96_match_phase_machine`: `state`/`phase` (two views of the
  native `0x157A4D` byte; the step gates on the machine `state` and the run
  `state.phase` and reads the arm-block phase from the latter),
  `side_controlled`, `ac5`/`ac7`, `arm2a_overflow`, `chosen831[2]` (Task 1
  shape). The native per-team `+0x831` is the pool's
  `struct fifa96_match_team.chosen831` (encoded id or NONE): the Task 2 arms
  write that field, so the machine's `chosen831[2]` array stays a seeded
  Task-1 field with no Task-2 consumer.
* `fifa96_match_phase_machine_step(struct fifa96_match_run *)` in
  `src/fifa96_engine/fifa96_match_phase_machine.c`; the machine is embedded in
  `struct fifa96_match_run` (`phase_machine`), reset by init/begin, and called
  from `fifa96_match_run_frame` after the FU-141 chain when the run phase is
  0x13/0x14 (the per-team `FUN_000740A0` order).
* Tested: `tests/test_engine_match_phase_machine.c` (`test_step_gate`,
  `test_step_arm26_side_mismatch`, `test_step_arm25_3_phase13`,
  `test_step_arm28_2a_phase14`, `test_step_2a_overflow`,
  `test_step_side_hi_bit`) and `tests/test_engine_match_frame.c`
  (`test_phase_machine_hook` plus the init/begin reset assertions).

Write set of this appendix (Task 2): `include/fifa96_engine/fifa96_match_phase_machine.h`,
`src/fifa96_engine/fifa96_match_phase_machine.c`,
`include/fifa96_engine/fifa96_match_run.h`,
`src/fifa96_engine/fifa96_match_run.c`,
`tests/test_engine_match_phase_machine.c`, `tests/test_engine_match_frame.c`,
this appendix, the §6 OL-46/OL-49 updates and the §7 addition.

## Appendix C (FU142b / M2 arms-and-wiring Task 3) — row 0x26 body + `0x8DCD4` helper + `0x36200` stub first-hand window

Task 3 of the follow-up plan (`docs/superpowers/plans/2026-10-07-fifa96-m2-arms-and-wiring.md`)
ports action row 0x26 (`0x866F4..0x8681C`) and its shared helper `FUN_0008DCD4`
(`0x8DCD4..0x8DD5B`), produces the `0x36200` stub surface and performs the first
cluster-G wiring (`fifa96_match_action_26`, table row 0x26 `fn` set). This
appendix is its evidence gate: the exact fields, constants and branch sites
below are read first-hand this slice and are what the port implements.

### C.1 Tool calls (Ghidra read-only, explicit /FIFA96.EXE)

* `get_function_by_address 0x866F4` — no function (table-referenced body), as
  FU-142 §1 records;
* `disassemble_bytes 0x866F4` (304 B) — the full 90-instruction body to the RET
  at `0x8681C` plus the row-27 prologue (`instructions_total 95`);
* `read_memory 0x110778` (8 B) — action-table entries `f4 66 08 00` (code 0x26
  -> `0x000866F4`) and `20 68 08 00` (code 0x27 -> `0x00086820`);
* `get_function_by_address 0x8DCD4` — defined, body `0x8DCD4..0x8DD5B` (61 insns);
* `decompile_function 0x8DCD4` + `disassemble_function 0x8DCD4` — the octagonal
  distance formula and the 6-byte out vector;
* `read_memory 0x10F394` (40 B) — the 32-byte table behind `[0x157A38]`
  (`06×8 07×8 0F 0F 0F 0E 0D 0D 0C 0B 0B 0A 09 09 08 07 07 06`; FU-84 §6
  corroborates the pointer value `0xF394` installed by `FUN_00073CD0`);
* `read_memory 0x157A38` (16 B) — zero in the file image (BSS; the runtime
  pointer is installed by `FUN_00073CD0`), so the port resolves the indirection
  to flat `0x10F394`;
* `disassemble_bytes 0x36200` (16 B) — `A3 C4 5F 10 00 C3` =
  `MOV [0x105FC4],EAX; RET`.

No writes: no rename/comment/label/function/script/project save. Repo:
`make check` 101/101 before, 103/103 after (two new suites).

### C.2 Row 0x26 body `0x866F4..0x8681C` (site-annotated)

Entry: EAX = record; no stack args. Prologue `PUSH EBX/ECX/EDX/ESI; MOV ECX,EAX`
(`0x866F4..0x866F8`).

| # | site | instruction (first-hand) | derived effect |
|---|---|---|---|
| 1 | `0x866FC..0x8670D` | `MOV EDX,[ECX+0x89]; XOR EAX,EAX; MOV AX,[0x157A64]; ADD EDX,EAX; MOV [ECX+0x89],EDX` | `timer89 += zero-extend(delta word)`, 32-bit wrap |
| 2 | `0x8670A..0x86729` | `MOV EAX,[ECX+0x4]` (P); `MOV EAX,[EAX+0xA]; SAR 0x18` (`P[+0xD]`); `MOV EDX,[0x157A38]; MOV AL,[EDX+EAX]; AND 0xFF; SAR 1; MOV [ECX+0x7B],AX` | `timer7b = 0x10F394[rec[+4][+0xD]] >> 1` |
| 3 | `0x8672D..0x86740` | `MOV AL,[ECX+0x92]; CMP AL,1; JC 0x8673E; JBE 0x86782` | latch: 0 -> stage-0 arm; 1 -> placement; >= 2 -> `POP…RET` (`0x86739..0x8673D`) |
| 4 | stage 0 `0x86746..0x8677C` | `P=[ECX+4]; EDX=[P+0xB]; SAR 0x18` (`P[+0xE]`); `EAX=EDX<<4; EAX-=EDX; EAX<<=4; EAX>>=4` (=15·d); `CMP EAX,[ECX+0x89]; JG RET`; else `[ECX+0x89]=0; MOV AH,[ECX+0x92]; INC AH; MOV [ECX+0x92],AH` | wait until `timer89 >= 15 * rec[+4][+0xE]` (signed), then `timer89=0; stage92=1`; falls into the placement |
| 5 | placement `0x86782..0x867C2` | `EDX=[ECX+0x8A]; SAR 0x19` (= `(int8)rec[+0x8D] >> 1`); `EAX=6·EDX`; `AL=[ECX+0x8D]&1`; `[ECX+0x4D]=0x780`; `[ECX+0x55]=0`; `EDX=MOVSX(DX)`; sign? `+` : `NEG`; `[ECX+0x55] += EDX` | `target.x=0x780`; `target.z = ±(int16)(6·((int8)active>>1))`, `+` when `active & 1`, `-` otherwise |
| 6 | `0x867C4..0x867D1` | `EBX=&rec+0x65; EDX=&rec+0x4D; EAX=&rec+0x59; CALL 0x8DCD4` | helper writes `{distance +0x65, dx +0x67, dz +0x69}` |
| 7 | `0x867D6..0x867F0` | `CMP word [ECX+0x69],0; JL negate; MOV EAX,[ECX+0x67]; SAR 0x10; CMP EAX,0x20; JGE RET` | `|lane| = |dz| = |target.z - pos.z|`; `>= 0x20` -> RET |
| 8 | `0x867F2..0x8681C` | `[ECX+0x4D]=0xCC0; [ECX+0x55]=0; MOV DL,[ECX+0x92]; [ECX+0x89]=0; INC DL; MOV [ECX+0x92],DL`; `POP…RET` | `target=(0xCC0,0); timer89=0; stage92++` (byte wrap) |

Constants: `0x780`, `0xCC0`, `0x20` lateral gate, `6` (`4d - d` then doubled),
`15` (`d<<4 - d`), table `0x10F394`, delta word `0x157A64`. The body writes no
install code and no `+0x9E`, so the derived `install`/`ran` staging passes
through untouched.

### C.3 The shared helper `FUN_0008DCD4` (`0x8DCD4..0x8DD5B`, 61 insns)

Register contract (Watcom): EAX = from (position triple), EDX = to (target
triple), EBX = out (`record+0x65`). Only the low words are read: `from[0]`,
`from[4]` (i.e. `+0x59`, `+0x61`) and `to[0]`, `to[4]` (`+0x4D`, `+0x55`); the
y components are never read.

```
dx = (int16)(to.x_word - from.x_word)
dz = (int16)(to.z_word - from.z_word)
out[1] = dx; out[2] = dz            ; the 6-byte {distance, dx, dz} layout FU-79 §2.9
ma = |dx|; mb = |dz|                 ; 16-bit negate
if ma < mb:  d = ma>>2; if (mb>>1) < ma: d = ((d + (ma>>1))>>1); d = d + mb
elif mb < ma: d = mb>>2; tmp = ma; if (ma>>1) < mb: d = ((d + (mb>>1))>>1); d = d + tmp
else:         d = ((ma>>1) + (ma>>2))>>1; d = d + ma
out[0] = (int16)d
```

Two derived-surface notes:

* the 16-bit truncations matter: `(0x7FFF, 0x7FFF) -> 45054 -> -20482`, pinned
  by `test_arm_helpers::test_dist_stage_distance_wraps16`; the port keeps the
  native short arithmetic step for step;
* the C surface exposes `out_distance` and `out_lane` (the `+0x69` dz word,
  sign-extended). The `+0x67` dx word has no Task-3 consumer; it is recorded
  here as not exposed, and a later row body that reads it extends the surface
  then (never repurposes `out_lane`).

### C.4 The `0x36200` stub

`0x36200`: `A3 C4 5F 10 00` `MOV [0x105FC4],EAX`; `C3` RET (5-byte instruction
+ RET). The store target gates the unported camera/coordinate step
`FUN_00036208` (`0x3621E CMP dword [0x105FC4],1; SETZ/JZ return`; the
`0x36211` gate above tests `[0x105FB4] < 0`, FU-62 §3.3). Neither
row 0x26 (this task) nor the derived engine models `0x105FC4` or that camera
step, so `fifa96_arm_stub_36200` is a **documented derived no-op** returning
FIFA96_OK; the stored value's surface is deferred to the Task 7/8 appendices
that port its callers (rows 28/2A).

### C.5 Port mapping and derived surfaces

* `fifa96_arm_helpers.{h,c}`: `fifa96_arm_vec`, `struct fifa96_arm_record` (all
  cluster-G native fields by offset; Task 3 adds `delta`, `timer7b`,
  `player_d`, `player_e`, `lane`), `fifa96_arm_dist_stage` (C.3).
* `fifa96_arm_bodies.{h,c}`: `fifa96_arm_stub_36200` (C.4) and
  `fifa96_arm_26_step` (C.2). Validation hardening: NULL `rec` or `player_d`
  outside 0..31 returns `-FIFA96_ERR_INVALID` before any state write (the
  native reads adjacent memory for a stray index; the derived table domain is
  exactly 32 and the pool/record cannot produce a stray index).
* Engine: `fifa96_match_run_record` gains `stage92`/`timer7b`/`lane` and the
  `player_d`/`player_e` descriptor stand-ins; `match_run_dispatch_entity`
  stages them (descriptor bytes as the derived 0 default) and repacks
  `stage92`/`timer7b`/`lane`; `fifa96_match_action_26` maps the record through
  `fifa96_arm_26_step`; action-table row 0x26 `fn` is set (first wiring), which
  flips the FU-137 §6.1 class to `ported`.
* Tested: `tests/test_arm_helpers.c` (`test_dist_stage_*`, 8 cases),
  `tests/test_arm_bodies.c` (`test_arm_stub_36200`, `test_arm_26_*`, 11
  cases), `tests/test_engine_match_handlers.c::test_action_26_runs_body` plus
  the flipped `action_expect[0x26]`.

### C.6 Open legs (numbered)

* **OL-50 — row-26 descriptor bytes.** `rec[+0x4]` is a per-player descriptor
  pointer whose dword `+0xA`/`+0xB` high bytes (`P[+0xD]`/`P[+0xE]`) feed
  `timer7b` and the stage-0 gate. The FU-141 pool does not model it; the derived
  staging supplies `player_d = player_e = 0`. The native descriptor's
  producer/roster binding stays unported.
* **OL-51 — `0x36200` gate global.** The `[0x105FC4]` store and its
  `FUN_00036208` reader (FU-62 §3.3) are unported; the stub stays a derived
  no-op until the row-28/2A callers' appendices land.

Write set of this appendix (Task 3): `include/fifa96_loader/fifa96_arm_helpers.h`,
`src/fifa96_loader/fifa96_arm_helpers.c`,
`include/fifa96_loader/fifa96_arm_bodies.h`,
`src/fifa96_loader/fifa96_arm_bodies.c`, `tests/test_arm_helpers.c`,
`tests/test_arm_bodies.c`, `src/fifa96_engine/fifa96_match_handlers.c`,
`include/fifa96_engine/fifa96_match_run.h`,
`src/fifa96_engine/fifa96_match_run.c`, `tests/test_engine_match_handlers.c`,
`CMakeLists.txt`, this appendix and the §6 OL-47 / §7 additions.

## Appendix D (FU142b / M2 arms-and-wiring Task 4) — row 0x27 body + `0x79C50` face + `0x6E598` anim selector first-hand window

Task 4 of the follow-up plan ports action row 0x27 (`0x86820..0x86A02`, 136
instructions), its two helpers `FUN_00079C50` (`0x79C50..0x79C98`, 28 insns)
and `FUN_0006E598` (`0x6E598..0x6E713`) and the `0x1103CB` animation table
into the tested loader symbols `fifa96_arm_27_step` / `fifa96_arm_face` /
`fifa96_arm_anim_select`. The row has **no static entry** and therefore stays
unwired (OL-48): D.1 records what an entry would be, where the slices
searched and the negative result. This appendix is its evidence gate: the
exact fields, constants and branch sites below are read first-hand this slice
and are what the port implements.

### D.1 Entry probe (what would constitute an entry; where searched; negative result)

An entry for the row body would be one of: (a) a direct `CALL`/code pointer to
`0x86820`; (b) a match-code `MOV EDX,0x27` (installer argument) or
`MOV ECX,0x27` (FU-142a arm argument) feeding `FUN_0007D9A4` (FU-137 §2
installer) or `FUN_0008CEB8`; (c) any non-action-table pointer to the body.

| probe (read-only `/FIFA96.EXE`) | result |
|---|---|
| `get_xrefs_to 0x86820` | **1 reference: DATA from `0x11077C`** — the action-table slot `0x1106E0 + 0x27*4` |
| `search_instructions operand 0x86820` | 0 matches |
| `search_byte_patterns 20 68 08 00` (the little-endian body pointer) | **1 hit: `0x11077C`** — the action-table slot itself; no other pointer table holds the body |
| `search_instructions mnemonic=MOV operand="EDX, 0x27"` | 2 sites. `0x756D5` inside the keeper handler: `MOV EDX,0x27; MOV EAX,[EBP-0xC]; MOV ECX,[ECX+0x8B]; XOR EBX,EBX; SAR ECX,0x18; CALL 0x6E598` (`0x756D5..0x756E8`, the FU-137 §5.3 animation-row argument). `0x1F52E` in `FUN_0001F440`: a jump-table selector (EDX = 0x25/0x27/0x2B/0x29/0x26/0x28/0x2A) whose common tail `0x1F55D MOV EAX,0x6B; CALL 0x13600` is a generic call, not an installer |
| `search_instructions mnemonic=MOV operand="DX, 0x27"` | the same two sites |
| `search_instructions mnemonic=MOV operand="ECX, 0x27"` | 0 (only `ECX,0x270F`/`0x2710` constants) |
| `get_xrefs_to 0x8CEB8` | 15 refs, all `UNCONDITIONAL_CALL` inside `FUN_0008D098`; Appendix A.4 lists the staged codes 0, 3, 0x15, 0x25, 0x26, 0x28, 0x2A — **no 0x27** |
| `get_xrefs_to 0x7D9A4` | 77 call sites, none reached by an `EDX=0x27` constant site above |

**Negative result: the body has no static entry; the action-table slot is the
only reference to it anywhere in the program image.** A register-derived
(`EDX` loaded from a record field) installer argument cannot be excluded by a
constant census, so the entry stays unresolved and the FU-142f
runtime/reachability pass remains the precondition (OL-48). The row keeps
`fifa96_match_action_table[0x27].fn == NULL` and dispatches
`-FIFA96_ERR_UNSUPPORTED` (evidence string names FU-142b/OL-48).

### D.2 The `0x1103CB` animation table (first-hand read, 96 B)

`read_memory 0x1103CB` returned 96 bytes; `get_xrefs_to 0x1103CB` shows the
only code reader is the row-27 `MOV EDI,0x1103CB` at `0x86846`. The table is
24 little-endian `{id word, time word}` pairs (the native reads the low word as
the id and treats the high word as a signed time):

| offset | pairs (id, time) |
|---|---|
| `+0x00` | `{0x006B, 0x0168}`, `{0x0068, 0xFFFF}`, `{0x0003, 0x0078}` |
| `+0x0C` | `{0x0003, 0x001E}`, `{0x0016, 0xFFFF}`, `{0x0003, 0x00F0}` |
| `+0x18` | `{0x0015, 0x001E}`, `{0x0056, 0x00F0}`, `{0x0015, 0x00F0}` |
| `+0x24` | `{0x0067, 0x0168}`, `{0x0003, 0x0078}`, `{0x0003, 0x0078}` |
| `+0x30` | `{0x0015, 0x001E}`, `{0x0050, 0xFFFF}`, `{0x0003, 0x00F0}` |
| `+0x3C` | `{0x0003, 0x001E}`, `{0x001A, 0xFFFF}`, `{0x0015, 0x00F0}` |
| `+0x48` | `{0x0003, 0x001E}`, `{0x0019, 0xFFFF}`, `{0x0015, 0x00F0}` |
| `+0x54` | `{0x0015, 0x001E}`, `{0x0068, 0xFFFF}`, `{0x0003, 0x00F0}` |

The eight nominal 12-byte rows are the `[0x158782]` 0..7 cycle; the stage-2
walk treats the table as one flat 24-pair sequence (`3*cycle + cursor`), so a
cursor can continue into the following nominal row. The 96th byte ends at
`0x11042A`; the adjacent `0x11042B` data (a separate word-table family) is the
native unbounded-read target (OL-53).

### D.3 Row 0x27 body `0x86820..0x86A02` (site-annotated)

Entry: EAX = record; no stack args. Prologue `PUSH EBX/ECX/EDX/ESI/EDI;
MOV ESI,EAX` (`0x86820..0x86825`).

| # | site | instruction (first-hand) | derived effect |
|---|---|---|---|
| 1 | `0x86827..0x86837` | `XOR EAX,EAX; MOV EDX,[ESI+0x89]; MOV AX,[0x157A64]; ADD EDX,EAX; MOV [ESI+0x89],EDX` | `timer89 += zero-extend(delta word)`, 32-bit wrap |
| 2 | `0x8683d..0x86843` | `MOV EDX,[ESI+0x8A]; SAR EDX,0x19` | `d = (int8)active >> 1` (byte `+0x8D`) |
| 3 | `0x86846..0x8687d` | `EDI=0x1103CB; EAX=4d; [ESI+0x4D]=0x780; EAX-=EDX (3d); DL=[ESI+0x8D]&1; [ESI+0x55]=0; EAX+=EAX (6d); EBX=MOVSX(DL); CWDE; (NEG); ADD [ESI+0x55],EAX` | `target.x=0x780`; `target.z = ±(int16)(6d)`, `+` when `active & 1` |
| 4 | `0x8687f..0x86888` | `LEA EBX,[ESI+0x65]; LEA EDX,[ESI+0x4D]; LEA EAX,[ESI+0x59]; CALL 0x8DCD4` | helper writes `{distance +0x65, dx +0x67, dz +0x69}` |
| 5 | `0x8688d..0x868b5` | `CMP word [ESI+0x69],0; JL negate; EAX=[ESI+0x67] SAR 16` (= the `+0x69` word); `CMP EAX,0x20; JGE 0x868b7`; else `[ESI+0x4D]=0xCC0; [ESI+0x55]=0` | `|lane| = |dz| = |target.z - pos.z|`; `< 0x20` -> retarget `(0xCC0, 0)` |
| 6 | `0x868b7..0x868c6` | `CMP byte [ESI+0x8D],0; SETZ AL; JNZ 0x869FD` | `active != 0` returns after the placement |
| 7 | `0x868cc..0x868e7` | `AL=[ESI+0x92]; CMP AL,1 JC 0x868e5; JBE 0x86905; CMP AL,2 JZ 0x86981; JMP 0x869FD` | stage latch: 0/1 -> S1, 2 -> S2, else return |
| 8 | `0x868ed..0x868ff` | `DL=[ESI+0x92]; [ESI+0x89]=0; INC DL; MOV [ESI+0x92],DL` | stage 0: `timer89=0; stage92=1`, falls into S1 |
| 9 | S1 `0x86905..0x86913` | `EBX=[ESI+0x67] SAR 16` (dz); `EDX=[ESI+0x65] SAR 16` (dx); `EAX=ESI; CALL 0x79C50` | face (`D.4`) on the pre-retarget `(dx, dz)` |
| 10 | S1 `0x86918..0x86933` | `BX=[0x158782]; INC EBX; AX=BX; [0x158782]=BX; CMP AX,8; JL; [0x158782]=0` | `anim_cycle = (anim_cycle + 1) mod 8` (row seed `RNG & 7`, D.6) |
| 11 | S1 `0x8693a..0x8695c` | `AX=[0x158782]; EDX=2AX; EAX=3AX; EDX=MOVSX word [EDI+EAX*4]; EAX=ESI; EBX=0; CALL 0x6E598` | select `pairs[3*cycle].id` (`0x1103CB + 12*cycle`) |
| 12 | S1 `0x86961..0x8697b` | `[ESI+0x89]=0; DH=[ESI+0x92]; EAX=0; INC DH; [0x10F374]=AX; [ESI+0x92]=DH` | `timer89=0; anim_cursor=0; stage92=2`; falls into S2 |
| 13 | S2 `0x86981..0x869a4` | `AX=[0x158782]; EAX=3AX; EAX<<=2; EDI+=EAX; EAX=[0x10F372] SAR 16; EAX<<=2; EAX+=EDI; BX=[EAX+2]` | `pair = 3*cycle + cursor`; read `pairs[pair].time` |
| 14 | S2 `0x869a8..0x869c4` | `TEST BX,BX; JGE positive; CMP byte [ESI+0x44],0; JZ no-fire` / positive: `EBX=[EAX] SAR 16; ECX=[ESI+0x89]; CMP EBX,ECX; JGE no-fire` | fire iff (`time < 0` && `+0x44 != 0`) or (`time >= 0` && `timer89 > time`) |
| 15 | S2 fire `0x869cc..0x869f8` | `DI=[0x10F374]; EAX+=4; [ESI+0x89]=0; EBX=0; EDI++; EDX=MOVSX word [EAX]; EAX=ESI; [0x10F374]=DI; CALL 0x6E598` | `timer89=0; cursor++`; select `pairs[pair+1].id` |
| 16 | `0x869fd..0x86A02` | `POP EDI/ESI/EDX/ECX/EBX; RET` | return (one NOP pad at `0x86A03`) |

Constants: `0x780`, `0xCC0`, `0x20` lateral gate, `6` (`4d - d` then doubled),
the `0x1103CB` table, `8` (cycle wrap), delta word `0x157A64`, cycle
`[0x158782]`, cursor `[0x10F372]` high word (`0x10F374`), the `+0x44`
negative-time gate. The body requests no install code and writes no `+0x9E`,
so the derived `install`/`ran` staging passes through untouched.

### D.4 The face helper `FUN_00079C50` (`0x79C50..0x79C98`, 28 insns)

Register contract: EAX = entity/record, DX/BX = two signed 16-bit direction
components. `TEST DX,DX / JNZ`; `TEST BX,BX / JNZ`
(`0x79C54..0x79C5C`): both zero -> `MOVSX AX, byte [EAX+0x8E]` and return
(`0x79C5E..0x79C68`), no writes. Otherwise
`PUSH BX; PUSH DX; CALL 0xCD474` (cdecl: `[EBP+8] = DX`, `[EBP+0xC] = BX`),
`MOV word [ESI+0x7D],AX` (`0x79C76`), then
`EAX = [ESI+0x7B] >> 16 = word [ESI+0x7D]` (the angle just stored),
`+0x40 & 0x3FF >> 7`, `MOV byte [ESI+0x8E],AL`, `CBW`, return
(`0x79C7A..0x79C98`).

Row 27's call (`0x86905..0x86913`) passes `DX = [rec+0x65]>>16 = dx` and
`BX = [rec+0x67]>>16 = dz`, i.e. the `0x8DCD4` out triple's x/z deltas (the
dword/SAR-16 loads read the `+0x67`/`+0x69` words of the 6-byte vector). The
angle primitive is the already-ported `FUN_000CD474` =
`fifa96_action_kick_angle(x, z, &angle)` (FU-76 §5.77, 257-byte atan table
`0x14072C`; port argument order x = the native first cdecl argument = DX).

### D.5 The animation selector `FUN_0006E598` (`0x6E598..0x6E713`)

First-hand block map (the full model is FU-84 §1; only the id-resolution
subset is ported here):

* `0x6E59C..0x6E608` side-sensitive `[0x57A6C]` write (ball-z sign vs team
  side, then `DX in {0x2C,0x2D,0x2E,0x35,0x37,0x39,0x3B,0x5C}` -> 1 else 0);
  **unmodeled (OL-52)**;
* `0x6E608..0x6E61C` `([0x57A4A]>>24 != 2 && DX == 0)` gate for the reroll;
  **unmodeled (OL-52)**;
* `0x6E622..0x6E655` + `0x6E687` current row `[rec+0x28]`: `EDI == 0` (NULL
  pointer) -> RNG (`0x6E627` -> `0x6E659`); row byte 0 `CL` == 0
  (`0x6E62D..0x6E62F`) or in `{0x62,0x63,0x64,0x65}`
  (`0x6E638..0x6E64A`) -> EAX=0 -> `0x6E657 JZ 0x6E687`, which **keeps that
  byte** (`EDX = byte[[rec+0x28]]`); a non-zero non-special byte -> EAX=1
  (`0x6E64C..0x6E651`) -> RNG;
* `0x6E659..0x6E685` RNG `CALL 0x92AC8` and
  `switch (RNG & 3) { 0,3 -> 0; 1 -> 0x62; 2 -> 0x65 }` (jump table
  `0x6E588`); **unmodeled (OL-52)**;
* `0x6E68E..0x6E69B` clamp `(int16)DX < 0 || >= 0x6F` -> 0;
* `0x6E69D..0x6E713` row pointer `[0x157588] + DX*9` -> `[rec+0x28]`, flag
  fan-out `bit4 -> +0x3F=2`, `bit5 -> +0x3F=-2`, `bit0 -> +0x44`, `bit1 ->
  +0x43`, `bit2 -> +0x45`, `+0x46=1`, frame resolve `FUN_0006E490(rec,
  &rec+0x28, BX)` and `word [rec+0x32] = 0`; **unmodeled (OL-52)**.

Row 27's two calls (`0x8695C`, `0x869F8`) pass `EBX = 0` and the `0x1103CB`
id word in `EDX`; every table id is in `3..0x6B`, so the current-row/RNG
branch is never entered from this row. The derived
`fifa96_arm_anim_select` covers the non-zero clamp and the native
special-byte keep rule, with the caller-supplied `row` argument as a stand-in
for `byte[[rec+0x28]]` (the row-27 step passes its last resolved id
`anim_sel`; the `0x10EF00` row-table byte is neither staged nor established to
equal it), and the step records the resolved id in `anim_sel`; the RNG draws
and the native row-pointer source stay OL-52. `kind` is `uint8_t`, so the
native signed 16-bit id is representable only for 0..0xFF (low-byte-lossless
for the `0x1103CB` ids 0..0x6B; a 16-bit argument outside that range is not
modelled). `flag44` is the staged stand-in for the native `+0x44` written by
the unmodeled fan-out.

### D.6 Port mapping and derived surfaces

* `fifa96_arm_helpers.{h,c}`: record-view additions `anim_cycle` (`0x158782`),
  `anim_cursor` (`0x10F374`), `anim_sel` (last resolved id, derived
  observation), `flag44` (native `+0x44` stand-in), `anim_overflow` (derived
  bound flag); `fifa96_arm_face` (D.4) and `fifa96_arm_anim_select` (D.5).
  `fifa96_arm_face` calls the ported `fifa96_action_kick_angle` (link adds
  `fifa96_action_handlers`).
* `fifa96_arm_bodies.{h,c}`: the 24-pair `0x1103CB` table (D.2) and
  `fifa96_arm_27_step` (D.3). NULL `rec` -> `-FIFA96_ERR_INVALID`.
* Derived-surface decisions, all recorded here: (a) the native cycle/cursor
  globals are carried per record (OL-53); (b) the pair walk is bounded at 24
  pairs with `anim_overflow` instead of the native adjacent read (OL-53);
  (c) the face helper recomputes the `(dx, dz)` words from its position/target
  arguments and the step passes the pre-retarget target (the native call at
  `0x86913` reads the `0x8DCD4` output written before the `0x868a9`
  retarget); (d) the zero-direction guard leaves `*out_lane` unchanged (the
  caller seeds it with `type` = `+0x8E`); (e) the native `+0x7D` angle word
  has no row-27 consumer and is not exposed.
* Engine: `fifa96_match_action_table[0x27].fn` stays **NULL** (no static
  entry, D.1); the evidence string becomes `"...body ported (FU-142b); entry
  unresolved (FU-142f/OL-48); -UNSUPPORTED"`; `tests/test_engine_match_handlers.c`
  asserts the UNSUP dispatch and the OL-48 marker. No `fifa96_match_run`
  staging change (the row is unwired).
* Tested: `tests/test_arm_helpers.c` (`test_face_*` 5 functions,
  `test_anim_select_*` 5 functions), `tests/test_arm_bodies.c`
  (`test_arm_27_*` 11 functions), `tests/test_engine_match_handlers.c`
  `test_action_27_unwired_entry`.

### D.7 Open legs (numbered)

* OL-48 (entry) — D.1; row stays unwired.
* OL-52 (selector record surface) and OL-53 (pair-walk globals/bounds) — §6.

Write set of this appendix (Task 4): `include/fifa96_loader/fifa96_arm_helpers.h`,
`src/fifa96_loader/fifa96_arm_helpers.c`,
`include/fifa96_loader/fifa96_arm_bodies.h`,
`src/fifa96_loader/fifa96_arm_bodies.c`, `tests/test_arm_helpers.c`,
`tests/test_arm_bodies.c`, `src/fifa96_engine/fifa96_match_handlers.c`,
`tests/test_engine_match_handlers.c`, `CMakeLists.txt`, the FU-137 errata and
this appendix. No engine record change: row 27 stays unwired.

## Appendix E (FU142b / M2 arms-and-wiring Task 5) — row 0x2C body + `FUN_0007DAB4` reset subset first-hand window

Task 5 of the follow-up plan ports action row 0x2C (`0x84598..0x8462D`, 48
instructions) and the `FUN_0007DAB4` derived reset subset
(`fifa96_arm_reset`, `0x7DAB4..0x7DB0C`, 35 instructions) into the tested
loader symbols `fifa96_arm_2c_step` / `fifa96_arm_reset`. The row has **no
static entry** and therefore stays unwired (OL-48): E.4 records the entry
probe and its negative result. This appendix is its evidence gate: the exact
fields, constants and branch sites below are read first-hand this slice and
are what the port implements.

### E.1 Tool calls (Ghidra read-only, explicit /FIFA96.EXE)

* `get_function_by_address 0x84598` — "No function found" (table-referenced
  body, like the other six row entries);
* `disassemble_bytes 0x84598` (152 B) — 48 instructions to the `RET` at
  `0x8462D`, plus the `MOV EAX,EAX` pad at `0x8462E` (49 total; the next
  action row 0x16 starts at `0x84630`);
* `read_memory 0x110790` (4 B) — action-table entry `98 45 08 00` (code 0x2C
  -> `0x00084598`);
* `get_xrefs_to 0x84598` — **1 reference: DATA from `0x110790`** (the
  action-table slot itself);
* `search_byte_patterns 98 45 08 00` — **1 hit: `0x110790`**;
* `search_instructions mnemonic=MOV operand="EDX, 0x2C"` — 3 sites
  (`0x3035E` in `FUN_000302DC`, `0x40A8E`/`0x40C5C` in `FUN_00040A5C`), all
  non-match constants (the FU-137 §5.3 census records none calls `0x7D9A4`);
  `operand="ECX, 0x2C"` — 0 sites;
* `get_function_by_address 0x7DAB4` — defined, body `0x7DAB4..0x7DB0C`;
* `disassemble_function 0x7DAB4` (35 instructions) + `decompile_function`
  (source-equivalent reset listing).

No writes: no rename/comment/label/function/script/project save.

### E.2 Row 0x2C body `0x84598..0x8462D` (site-annotated)

Entry: EAX = record; no stack args. Prologue `PUSH EBX/ECX/EDX/ESI; MOV
ESI,EAX` (`0x84598..0x8459C`).

| # | site | instruction (first-hand) | derived effect |
|---|---|---|---|
| 1 | `0x8459E..0x845AC` | `MOV AL,[ESI+0x92]; CMP AL,1; JC 0x845B3; JBE 0x845D8; CMP AL,2; JZ 0x84607` | stage latch: 0 -> S0, 1 -> S1, 2 -> S2 |
| 2 | `0x845AE..0x845B2` | `POP ESI/EDX/ECX/EBX; RET` | stage >= 3 returns untouched |
| 3 | `0x845B3..0x845BE` | `TEST AL,AL; JNZ 0x84629` (unreachable: AL == 0 here); `CMP byte [ESI+0x8D],0; JZ 0x84622` | S0: `active == 0` -> reset |
| 4 | `0x845C0..0x845D2` | `MOV DL,[ESI+0x92]; MOV dword [ESI+0x89],0; INC DL; MOV [ESI+0x92],DL` | S0: `timer89 = 0; stage92 = 1` (falls into S1) |
| 5 | `0x845D8..0x845EA` | `MOV EDX,0x5D; MOV EAX,ESI; MOV ECX,[ESI+0x8B]; XOR EBX,EBX; SAR ECX,0x18; CALL 0x6E598` | select id `0x5D` (ECX dead, D.5/FU-84 §1; EBX = frame 0) |
| 6 | `0x845EF..0x84601` | `MOV DH,[ESI+0x92]; MOV dword [ESI+0x89],0; INC DH; MOV [ESI+0x92],DH` | `timer89 = 0; stage92++` (1 -> 2) |
| 7 | `0x84607..0x84620` | `XOR ECX,ECX; MOV EDX,[ESI+0x89]; MOV CX,word [0x157A64]; SUB EDX,ECX; MOV [ESI+0x89],EDX; TEST EDX,EDX; JG 0x84629` | `timer89 -= zero-extend(delta word)`; `> 0` waits |
| 8 | `0x84622..0x84624` | `MOV EAX,ESI; CALL 0x7DAB4` | reset (E.3) |
| 9 | `0x84629..0x8462D` | `POP ESI/EDX/ECX/EBX; RET` | return |

Constants: `0x5D` (the constant selector id), the `+0x157A64` delta word.
The body requests no install code and writes no `+0x9E`, so the derived
`install`/`ran` staging passes through untouched.

Derived-control-flow note: because S1 zeroes `timer89` before the S2
countdown, *any* call that reaches S1 (from S0 with `active != 0`, or an
entry at S1) resets in the same call; only an entry at S2 with
`timer89 > delta` waits. The port reproduces this exactly — it is not a
"machine waits out the animation" reading. The `0x845B5 JNZ` is dead under
the `JC` (AL == 0); the port has no corresponding branch. The `MOV
ECX,[ESI+0x8B]>>24` load is dead for `FUN_0006E598` (FU-84 §1: ECX is not
an input and the selector overwrites it); the port does not stage a type
argument.

### E.3 The reset `FUN_0007DAB4` (`0x7DAB4..0x7DB0C`, 35 insns) and the derived subset

First-hand listing:

| site | instruction | effect |
|---|---|---|
| `0x7DABA` | `MOV byte [EAX+0x92],0xFF` | `stage92 = 0xFF` |
| `0x7DAC1..0x7DAC4` | `MOV EDX,[EAX+0x20]; MOV dword [EAX+0x89],0` | `timer89 = 0` |
| `0x7DACE..0x7DAD4` | `TEST EDX,EDX; JZ; MOV EAX,EDX; CALL 0x78B00` | slot callback if `[rec+0x20] != 0` |
| `0x7DAD9..0x7DAE4` | `EAX=[0x157A4A]; SAR 0x18; CMP 2; JNZ 0x7DAFB` | phase test |
| `0x7DAE6..0x7DAF1` | `CMP byte [ESI+0x8D],0; JZ 0x7DAFB; MOV EAX,ESI; CALL 0x7C990` | phase-2 forced-decision arm for active records |
| `0x7DAFB..0x7DB03` | `XOR ECX,ECX; XOR EBX,EBX; XOR EDX,EDX; CALL 0x7D9A4` | code-0 re-install (staged, no invoke) |
| `0x7DB08..0x7DB0C` | `POP *; RET` | return |

Derived `fifa96_arm_reset` (FU-141 §3.4's init-seed subset): `stage92 = 0xFF`,
`timer89 = 0`, `code = 0`. The `[rec+0x20]` callback, the phase-2 forced
arm and the native installer's accepted-install tail (`+0x92` overwrite,
`+0x9E` clear, `+0x7B <- +0x79`, carrier bit) stay OL-54; the FU-141 §3.4
phase-2 arm is the documented pool call (OL-44) and the re-install is
represented by the record's `+0x91` code byte (the same reading
`fifa96_match_entities_init` uses, `fifa96_match_entities.c` lines 39-40).

### E.4 Entry probe (what would constitute an entry; where searched; negative result)

Same method as D.1. An entry would be (a) a `CALL`/code pointer to
`0x84598`, (b) a match-code `MOV EDX,0x2C` (installer argument) or `MOV
ECX,0x2C` (FU-142a arm argument), (c) any non-action-table pointer to the
body.

| probe (read-only `/FIFA96.EXE`) | result |
|---|---|
| `get_xrefs_to 0x84598` | **1 reference: DATA from `0x110790`** — the action-table slot `0x1106E0 + 0x2C*4` |
| `search_byte_patterns 98 45 08 00` | **1 hit: `0x110790`** — no other pointer table holds the body |
| `search_instructions mnemonic=MOV operand="EDX, 0x2C"` | 3 sites, all non-match (`0x3035E` in `FUN_000302DC`; `0x40A8E`/`0x40C5C` in `FUN_00040A5C`); the FU-137 §5.3 census records none calls `0x7D9A4` |
| `search_instructions mnemonic=MOV operand="ECX, 0x2C"` | 0 |
| `get_xrefs_to 0x8CEB8` | 15 refs, all inside `FUN_0008D098`; Appendix A.4 lists the staged codes — **no 0x2C** |

**Negative result: the body has no static entry; the action-table slot is
the only reference to it anywhere in the program image.** A register-derived
installer argument cannot be excluded by a constant census, so the entry
stays unresolved and the FU-142f runtime/reachability pass remains the
precondition (OL-48). The row keeps `fifa96_match_action_table[0x2C].fn ==
NULL` and dispatches `-FIFA96_ERR_UNSUPPORTED` (evidence string names
FU-142b/OL-48).

### E.5 Port mapping and derived surfaces

* `fifa96_arm_helpers.{h,c}`: `fifa96_arm_reset` (E.3), shared with Task 6's
  row 29; NULL `rec` -> `-FIFA96_ERR_INVALID`.
* `fifa96_arm_bodies.{h,c}`: `fifa96_arm_2c_step` (E.2). The stage-1 select
  reuses `fifa96_arm_anim_select` (Task 4) with the constant id `0x5D`; the
  resolved id is recorded in the derived `anim_sel` observation (as row 27
  does), the native row-pointer/flags writes stay OL-52. The stage-2
  countdown uses the derived `delta` word (zero-extended) with the native
  signed `JG` test. NULL `rec` -> `-FIFA96_ERR_INVALID`.
* Engine: `fifa96_match_action_table[0x2C].fn` stays **NULL** (no static
  entry, E.4); the evidence string becomes `"...body ported (FU-142b);
  entry unresolved (FU-142f/OL-48); -UNSUPPORTED"`.
* Tested: `tests/test_arm_helpers.c` (`test_arm_reset_fields`,
  `test_arm_reset_invalid`), `tests/test_arm_bodies.c` (`test_arm_2c_*`, 9
  cases), `tests/test_engine_match_handlers.c::test_action_2C_unwired_entry`.

### E.6 Open legs (numbered)

* OL-48 (entry) — E.4; row stays unwired.
* OL-54 (reset callbacks/installer tail) — §6.
* OL-52 (selector record surface) carries the row-2C selector call's
  unmodeled native writes — D.5.

Write set of this appendix (Task 5): `include/fifa96_loader/fifa96_arm_helpers.h`,
`src/fifa96_loader/fifa96_arm_helpers.c`,
`include/fifa96_loader/fifa96_arm_bodies.h`,
`src/fifa96_loader/fifa96_arm_bodies.c`, `tests/test_arm_helpers.c`,
`tests/test_arm_bodies.c`, `src/fifa96_engine/fifa96_match_handlers.c`,
`tests/test_engine_match_handlers.c`, the FU-137 errata and this appendix.
No engine record change: row 2C stays unwired.

## Appendix F (FU142c / M2 arms-and-wiring Task 6) — row 0x29 body + helpers first-hand window

Task 6 of the follow-up plan ports row `0x29`'s body. The row has **no static
entry** and therefore stays unwired (F.3): F.4 records the helper contracts,
F.5 the port mapping and the OL-54 pool-drain decision, F.6 the open legs.

### F.1 Tool calls (Ghidra read-only, explicit `/FIFA96.EXE`)

* `get_function_by_address` — `0x874E4` (absent), `0x6E05C` (absent);
* `disassemble_bytes` — `0x874E4` (596 B, 186 instructions), `0x1F510`
  (320 B), `0x6E05C` (256 B), `0x6E15B` (96 B), `0x8DE8C` (256 B), `0x6E1D0`
  (256 B), `0x7D8D0` (256 B), `0x7D985` (32 B), all with instructions;
* `read_memory` — `0x1106E0` (180 B; the row-0x29 dword at `0x110784` is
  `e4 74 08 00`), `0x110794` (140 B; index 5 = `5c e0 06 00`);
* `get_xrefs_to` — `0x874E4` (1), `0x10F36C` (4), `0x157A9F` (18),
  `0x157AA3` (14), `0x8CEB8` (15);
* `search_byte_patterns` — `e4 74 08 00` (1 hit);
* `search_instructions` — `MOV "EDX, 0x29"` (4, one real), `MOV "DX, 0x29"`
  (same), `MOV "ECX, 0x29"` (0), `CALL "0x0007d9a4"` (77 sites).

Baseline: `make check` **103/103** at task start; M1 golden/render pins
untouched. No Ghidra writes.

### F.2 Row 0x29 body `0x874E4..0x87738` (site-annotated)

Prologue `0x874E4..0x874EA` (`PUSH EBX/ECX/EDX/ESI/EDI/EBP; SUB ESP,4`),
`0x874ED MOV EBP,EAX` (rec). `0x874EF..0x874F5` `MOV word [EAX+0x7B],2`.
`0x874F5..0x87500` `MOV EAX,[0x157A4A]; SAR EAX,0x18; CMP EAX,5; JZ
0x87546` — the phase gate (`[0x157A4A]>>24` sign-extended).

**Phase != 5 (`0x87502..0x87541`).** `0x87502..0x87519` the target sync
(`LEA EDI,[EBP+0x4D]; LEA ESI,[EBP+0x59]` + 3 x `MOVSD`; `word [EBP+0x71]=0`,
`AX=[EBP+0x71]`, `word [EBP+0x75]=AX`, `word [EBP+0x73]=AX`).
`0x8751D..0x8751F` `MOV EAX,EBP; CALL 0x7DAB4` (the reset, Appendix E.3).
`0x87524..0x8752B` `CMP byte [EBP+0x9A],0; JNZ 0x8772F` — an occupied record
skips the install. `0x87531..0x8753C` `MOV EDX,3; MOV EAX,EBP; XOR ECX,ECX;
XOR EBX,EBX; CALL 0x7D9A4` — the self-install of code 3 (staged byte 0,
no-invoke). `0x87541 JMP 0x8772F`.

**Phase 5 latch (`0x87546..0x8757D`).** `0x87548..0x8755C`
`EDX=[EBP+0x89]; AX=[0x157A64]; ADD EDX,EAX; [EBP+0x89]=EDX` (zero-extended
delta add) after `0x87556 MOV AL,[EBP+0x92]`. `0x87562..0x87564 CMP AL,1;
JC 0x8757E` (stage 0), `0x87566 JBE 0x87620` (stage 1), `0x8756C..0x8756E
CMP AL,2; JZ 0x87714` (stage 2), otherwise the `0x87574..0x8757D` epilogue.
The `0x8757E TEST AL,AL; JNZ 0x8772F` under the `JC` is dead (AL == 0).

**Stage 0 (`0x8757E..0x8761A`).** `0x87586..0x87599`
`EAX=[0x157A9F]; EDX=[EBP]; EBX=[EAX+0x8A]>>24; EAX+=0x59; ECX=0` — the
nearest call's `position = [0x157A9F]+0x59`, `records = [rec+0]`,
`skip = byte[[0x157A9F]+0x8D]`, out = NULL. `0x8759C CALL 0x8DE8C` (F.4).
`0x875A1..0x875A3 CMP EBP,EAX; JNZ 0x87608` — only a nearest == this record
continues. Self path `0x875A5..0x87603`: `EAX=[EAX]` (`[rec+0]`) stored to
`[0x157AA3]` (`0x875A7`); `EAX=2; CALL 0x36200`; the target sync
(`0x875B6..0x875D3`); `MOV [0x10F36C],EBP` (`0x875CD`); `CALL 0x92AC8`
(`0x875D7`, the RNG); `TEST AL,1; JZ` -> `EAX=0x46` else `EAX=0x5D`
(`0x875E0..0x875EC` stages AL to `[ESP]`; `0x875F7 MOV EDX,[ESP-3]; SAR
0x18` re-reads the staged byte as the selector id); `EAX=EBP;
ECX=[EBP+0x8B]>>24; EBX=0; CALL 0x6E598` (`0x875FE..0x87603`; the ECX/EBX
loads are dead per FU-84 §1, as in row 2C). Converge `0x87608..0x8761A`:
`AH=[EBP+0x92]; dword [EBP+0x89]=0; INC AH; [EBP+0x92]=AH` — timer89 = 0,
stage92 = old+1, falling into stage 1.

**Stage 1 (`0x87620..0x8770E`).** `0x87620..0x8762C EBX=[0x10F36C]; TEST;
JZ 0x87676; CMP EBP,EBX; JNZ 0x87676`. Chase branch `0x8762E..0x8766C`:
the target sync on EBX (== rec), `CMP byte [EBX+0x44],0; JZ 0x8772F`; when
`+0x44 != 0` the selector re-runs on the staged byte (`0x87653 MOV
EDX,[ESP-3]; SAR 0x18`, sign-extended) and returns. Timer path
`0x87676..0x87694`: `EDX=[EBP+4]; EDX=[EDX+0xB]; SAR 0x18` (= `P[+0xE]`),
`EAX=EDX; SHL 4; SUB EAX,EDX; SHL 3; SAR 4` (= floor(120*d/16)),
`CMP EAX,[EBP+0x89]; JG 0x87714` — the wait jumps to the stage-2 sync block.
Below the gate: `0x8769A..0x876A1 CMP byte [EBP+0x8D],0; JNZ 0x876C0`.
`active == 0` (`0x876A3..0x876BE`) syncs and jumps to the distance call.
`active != 0` (`0x876C0..0x876D0`): `EDX=[EBP+0x4D]; EBX=-1; EAX=EBP;
word [EBP+0x7B]=2; CALL 0x6E1D0` (the phase cell, F.4). Distance
`0x876D5..0x876EC`: `EBX=[EBP+0x65]; EDX=[EBP+0x4D]; EAX=[EBP+0x59]; CALL
0x8DCD4`; `EAX=[EBP+0x63]; SAR 0x10` (the out[0] distance at +0x65);
`CMP EAX,0x20; JG 0x8772F`. Chase re-check `0x876EE..0x876FA`: `EDI=
[0x10F36C]; TEST; JZ 0x876FC; CMP EBP,EDI; JZ 0x8772F`. Advance
`0x876FC..0x8770E`: timer89 = 0, stage92++.

**Stage 2 (`0x87714..0x8772B`).** The target sync only (the `JG` wait and a
stage-2 latch entry share it). Epilogue `0x8772F..0x87738`
(`ADD ESP,4` + six pops + `RET`); the RET byte is the row-2B entry (§1.1).

### F.3 Entry probe (what would constitute an entry; negative result)

An entry would be (a) a `CALL`/code pointer to `0x874E4`, (b) a match-code
`MOV EDX,0x29` (installer argument) or `MOV ECX,0x29` (FU-142a arm argument),
(c) any non-action-table pointer to the body, or (d) an installer call inside
the phase-5 handler the body itself gates on.

| probe (read-only `/FIFA96.EXE`) | result |
|---|---|
| `get_xrefs_to 0x874E4` | **1 reference: DATA from `0x110784`** — the action-table slot `0x1106E0 + 0x29*4` (first-hand `read_memory 0x1106E0` shows `e4 74 08 00` there) |
| `search_byte_patterns e4 74 08 00` | **1 hit: `0x110784`** — no other pointer table holds the body |
| `search_instructions mnemonic=MOV operand="EDX, 0x29"` | 4 matches; 3 are the unrelated `0x292c21` literal, the one real `MOV EDX,0x29` is `0x1F53C` in `FUN_0001F440` (first-hand `0x1F516..0x1F55D`: a 0..7 jump-table selector whose tail is `MOV EAX,0x6B; CALL 0x13600`, not `0x7D9A4`) |
| `search_instructions mnemonic=MOV operand="DX, 0x29"` | same sites (substring census) |
| `search_instructions mnemonic=MOV operand="ECX, 0x29"` | 0 |
| `search_instructions mnemonic=CALL operand="0x0007d9a4"` | 77 sites; **none inside `0x6E05C..0x6E1B2`** (the phase-5 handler — the handler of the body's own gate) and none in rows 28/2A/2B; the only site in the row-29 window is the body's own `0x8753C` (code 3) |
| `disassemble_bytes 0x6E05C` + `0x6E15B` | the full 77+33-instruction phase-5 body (`0x6E05C..0x6E1B2`): distance/angle arithmetic on `[0x157A9F]`, no `FUN_0007D9A4` call and no `[rec+0x91]` write |
| `get_xrefs_to 0x8CEB8` | 15 refs, all inside `FUN_0008D098`; Appendix A.4 lists the staged codes — **no 0x29** |

**Negative result: the body has no static entry; the action-table slot is
the only reference to it anywhere in the program image**, and the one
handler-level candidate (phase 5) is installer-free. A register-derived
installer argument cannot be excluded by a constant census, so the entry
stays unresolved and the FU-142f runtime/reachability pass remains the
precondition (OL-48). The row keeps `fifa96_match_action_table[0x29].fn ==
NULL` and dispatches `-FIFA96_ERR_UNSUPPORTED` (evidence string names
FU-142c/OL-48).

### F.4 Helper contracts (first-hand)

* **`FUN_0008DE8C` (`0x8DE8C..0x8DEFF`, 49 insns)** — the nearest search the
  row calls at `0x8759C`. EAX = the target position block (reads words +0
  x/+8 z), EDX = the `[rec+0]` team base, BX = a skip index (the native
  sign-extends it; `i == skip` is skipped), ECX = an out pointer. It walks
  11 records at 0xB2 stride (`0x8DEEC CMP ECX,0xB`), skipping `+0x9A != 0`
  and `+0x98 != 0`, computes `0x8DC68(dx, dz)`, keeps the strictly-smaller
  unsigned word and returns the record pointer (or NULL). Row 29 passes
  `ECX = 0`, so the native stores the best word to linear address 0 (real
  mode is writable — the derived port stores into a local instead; no effect
  on the row's logic). The derived surface is the already-ported
  `fifa96_entity_find_nearest` (`include/fifa96_loader/fifa96_entity_update.h`,
  the FU-141 port used by `fifa96_match_entities_team_select`, which cites the
  same native at 0x8DE8C).
* **`0x6E1D0` (`0x6E1D0..0x6E241`, 38 insns)** — the phase cell the row calls
  at `0x876D0`. EDX = out triple (`&rec+0x4D`), EAX = rec. It takes the
  `[rec+8]` descriptor, selecting the byte pair at +0/+1, or +2/+3 when the
  zero-extended `[rec+0x826]` side equals the sign-extended
  `[0x157AAC]>>24`; scales the first byte by 38 (`SHL 5`/`SHL 2` sum at
  `0x6E1F8..0x6E211`) and the second by 33 (`SHL 5` add at `0x6E213`), negates
  both when `[rec+0x826] != 0`, and writes `{x, 0, z}`. The derived
  `fifa96_action_phase_cell(x, z, side, out)` is that pure function (38/33
  via `0x26`/`0x21`), already tested (FU-138).
* **`0x92AC8`** — the FU-141-ported six-word additive RNG
  (`fifa96_rng_step`); the row uses `value & 1`.
* **`0x6E598` / `0x8DCD4` / `0x7DAB4` / `0x36200` / `0x7D9A4`** — the Task
  4 selector subset (`fifa96_arm_anim_select`), the Task 3 helper
  (`fifa96_arm_dist_stage`), the Task 5 reset subset (`fifa96_arm_reset`),
  the Task 3 stub (`fifa96_arm_stub_36200`) and the engine installer
  (`fifa96_match_entities_install`), all cited in their appendices.

### F.5 Port mapping and derived surfaces

* `fifa96_arm_bodies.{h,c}`: `fifa96_arm_29_step` (F.2). The prologue writes
  `timer7b = 2`; the phase != 5 path syncs target/velocity, runs
  `fifa96_arm_reset` and, when `skip_9a == 0`, records `install = 3` (the
  pool installer consumes it; see the §6 OL-54 decision). The step clears
  `install` at entry (after the phase-5 input validation) because the request
  is per call — a stale request must not survive into the binder/drain.
  Phase 5 runs the derived stage latch with the native falls-through exactly
  as F.2 (stage 0 -> stage 1 in one call; the wait `JG` syncs target/pos;
  stage 2 syncs). The stage-1 `flag44 != 0` re-select is a **modeled no-op**:
  for the native id domain (0x5D/0x46) the derived selector is the identity,
  so the branch changes no observable state and is kept for site fidelity
  (the native selector's record writes stay OL-52).
* Derived fields the appendix adds to `struct fifa96_arm_record`:
  `phase` (`[0x157A4A]>>24`), `skip_9a` (`+0x9A`), `team_index` (the
  record's index in the candidate array), `ball_skip`
  (`byte[[0x157A9F]+0x8D]`), `chase` (derived `[0x10F36C] == this record`),
  `side_controlled` (`[0x157AAC]>>24`), `install` (the derived request),
  `int8_t cell[2][2]` (the `[rec+8]` descriptor pairs), `ball_pos`
  (`[[0x157A9F]+0x59]`, x/z used) and `team_candidates` (the `[rec+0]`
  team base, 11 entries). No field is repurposed.
* Derived stand-ins (F.6/OL-55): the selector id re-read from the stack is
  the directly staged byte; the chase identity is the per-record `chase` bit
  (the two native tests only compare equality with this record, so the bit is
  exact for the step; the global-other-record state is not representable);
  the phase-cell descriptor and team-base pointer sources are caller-supplied
  (the pointer chains `[rec+8]`/`[rec+0]` are unmodeled); the `[0x157AA3]`
  nearest store is dropped (no derived consumer).
* Validation/hardening: NULL `rec` -> `-FIFA96_ERR_INVALID`; a phase-5 call
  with NULL `rng`/`team_candidates` -> `-FIFA96_ERR_INVALID` before the
  prologue write (the native has no NULL concept; the two inputs are required
  on the phase-5 path). The phase != 5 path needs neither (pinned by test).
* Engine: `fifa96_match_action_table[0x29].fn` stays **NULL** (no static
  entry, F.3); the evidence string becomes `"FU-142c App. F/FU-137 §5.3: ...
  entry unresolved (FU-142f/OL-48); -UNSUPPORTED"`.
* Tested: `tests/test_arm_bodies.c` (`test_arm_29_*`, 16 cases) and
  `tests/test_engine_match_handlers.c::test_action_29_unwired_entry`.
* **OL-54 pool-drain decision (recorded here, §6):** the reset keeps the
  derived subset and the drain does not replay a reset tail (the native tail
  is unobservable behind the `code = 0` dispatch); explicit install requests
  (row 29's `install = 3`) ARE replayed through
  `fifa96_match_entities_install`, which reproduces the phase != 5 net state
  (code 3/0x19, `stage92 = 0`, `+0x9E` clear, `+0x7B <- +0x79`) once Task 9
  binds the row. The occupied-record divergence (`+0x9A != 0`: native code
  unchanged vs derived `code = 0`) stays recorded on OL-54.

### F.6 Open legs (numbered)

* OL-48 (entry) — F.3; row stays unwired.
* OL-55 (row-29 remainder) — the `[0x157AA3]` store, the `0x36200` value 2
  (shared OL-51), the `[rec+8]`/`[rec+0]` pointer sources, the
  `[0x10F36C]`/`[0x157A9F]` per-record stand-ins and the multi-record
  identity scope; §6.
* OL-52 (selector record surface) carries the row's `0x6E598` call's
  unmodeled native writes — D.5.
* OL-54 (drain decision) — decided in §6/F.5; the occupied-record divergence
  remains.

Write set of this appendix (Task 6):
`include/fifa96_loader/fifa96_arm_helpers.h`,
`include/fifa96_loader/fifa96_arm_bodies.h`,
`src/fifa96_loader/fifa96_arm_bodies.c`, `tests/test_arm_bodies.c`,
`src/fifa96_engine/fifa96_match_handlers.c`,
`tests/test_engine_match_handlers.c`, `CMakeLists.txt` (arm-bodies link deps),
the FU-137 errata and this appendix. No engine record change: row 29 stays
unwired.

## Appendix G (FU142d / M2 arms-and-wiring Task 7) — row 0x28 body + `0x87014` helper + `0x114E04` fold first-hand window

Task 7 of the follow-up plan
(`docs/superpowers/plans/2026-10-07-fifa96-m2-arms-and-wiring.md`) ports action
row 0x28 (`0x870E8..0x874E3`, 294 instructions, the 4-arm jump table `0x870D8`)
and its internal `0x87014` gate helper, exercises the fold through the
already-ported `0x114E04` sine table (`fifa96_projection_sincos`, FU-88), and
performs the second cluster-G wiring (`fifa96_match_action_28`; the FU-142a arm
`0x8D7CF` installs code 0x28). This appendix is its evidence gate: the exact
fields, constants and branch sites below are read first-hand this slice and are
what the port implements.

### G.1 Tool calls (Ghidra read-only, explicit `/FIFA96.EXE`)

* `get_function_by_address 0x870E8` — no function (table-referenced body);
* `disassemble_bytes 0x870E8` (1024 B; 298 instructions through `0x874E7`, the
  row RET at `0x874E3` followed by row 29's prologue pushes);
* `read_memory 0x110780` (8 B: `e8 70 08 00 e4 74 08 00` = action-table row
  0x28 `0x870E8`, row 0x29 `0x874E4`);
* `read_memory 0x870D8` (24 B: the 4-dword table `0x87147`, `0x87276`,
  `0x87364`, `0x874DA`, then the body prologue bytes `53 51 52 56 57 55 83 EC`);
* `disassemble_bytes 0x87014` (200 B; 54 instructions, body `0x87014..0x870D5`);
* `disassemble_function 0x8DCD4` (61 insns, `0x8DCD4..0x8DD5B`) and
  `disassemble_function 0x79C50` (28 insns, `0x79C50..0x79C98`) — the prologue
  helpers, carried from Appendices C/D;
* `get_xrefs_to 0x870E8` — 1 ref, the action-table slot `0x110780` (DATA);
* `get_xrefs_to 0x87014` — 2 refs, exactly the row-28 call sites `0x872C5` and
  `0x873A9`;
* `get_xrefs_to 0x10F358` (5: writers `0x7D97D` `FUN_0007D8D0`, `0x86AB0`/
  `0x86F70`; readers `0x87384`/`0x8746A`), `0x10F35C` (6: writers `0x7D983`,
  `0x887EC` `FUN_000886D4`, `0x8D80E` `FUN_0008D098`, `0x86AAB`/`0x86FFE`;
  reader `0x873CF`), `0x10F364` (5: writers `0x7D995`, `0x8D6D4`, `0x8D6FB`
  read-write, `0x8D71D`, `0x8D722`; readers `0x87159`, `0x6E274`), `0x10F368`
  (6: same shape; readers `0x87161`, `0x6E267`), `0x157AC2` (56: writers
  include `0x73F82`, `0x4B0DE`, `0x4BA20/31`, `0x38990`, `0x88D6A`; entry-block
  read `0x8D69F`; row-28 read `0x8716B`), `0x157AA3` (14: row-28 write
  `0x8714A`, row-2A writes `0x86A65`/`0x86B78`, row-29 `0x875A7`, other row
  arms);
* `read_memory 0x7D8B0` (32 B: dwords `0x15`, `0x19`, `0x03`, `0x02`,
  `0x00`, `0x58`, `0x5B`, `0x6B`);
* `read_memory 0x114E04` (4096 B; the table is 257 dwords, `0x114E04..0x115207`,
  ending `0x0000FFFE`, `0x00010000`);
* `read_memory 0x10F358` (28 B, file image all zero — runtime/BSS) and
  `read_memory 0x157AA0` (40 B zeros);
* `disassemble_bytes 0x8D6C0` (128 B) / `0x8D7B8` (88 B) — the entry block and
  0x2A scan context re-cited from Appendix B.

No writes: no rename/comment/label/function/script/project save.

### G.2 Entry census and body layout (first-hand)

`get_xrefs_to 0x870E8` returns exactly one reference, the action-table slot
`0x110780` (read this slice: `0x000870E8`). Unlike rows 27/29/2C the entry is
**resolved**: the FU-142a arm `0x8D7CF` (`FUN_0008D098` state 0x13/0x14,
phase != 0x13) stages code 0x28 into all free team records through
`FUN_0008CEB8` -> `FUN_0007D9A4`, and the installer's action-code byte is what
`FUN_0006D920` later resolves through the action table (FU-137 §1.3/§2). No
constant census is needed: the arm is a static installer of row 28.

Body layout (`0x870E8..0x874E3`):

| site | block |
|---|---|
| `0x870E8..0x87114` | prologue: `EBP = rec`; 16 B from `0x7D8B0` -> `[ESP+0x10]`, 16 B from `0x7D8C0` -> `[ESP]`; `EBX = rec+0x65`, `EDX = rec+0x4D`, `EAX = rec+0x59`; `CALL 0x8DCD4` |
| `0x87119..0x87127` | `EAX = rec`; `EBX = word [rec+0x69]` (`SAR 16` of the `+0x67` dword), `EDX = word [rec+0x67]` (`SAR 16` of the `+0x65` dword); `CALL 0x79C50` |
| `0x8712C..0x8713F` | `AL = [rec+0x92]`; `CMP AL,3; JA 0x874DA`; `JMP CS:[EAX*4+0x870D8]` |
| arm 0 `0x87147..0x87270` | store `[0x157AA3]`; stub `0x36200(EAX=2)`; target = `[0x10F364]`/`[0x10F368]` (+ side-0 negate below mode 4); the two `0x114E04` folds; the `[rec+0x28]`-gated constant id 0x15; the distance gate; one RNG draw, latch + `+0xA2`, falls through to arm 1 |
| arm 1 `0x87276..0x87336` + tail `0x8733A..0x87363` | `timer89 += delta` vs `+0xA2`; re-arm `+0xA2`; `flag830 == 0` -> (tail) target = pos, id 1; `flag830 != 0` -> `0x87014` setup + four draws, latch |
| arm 2 `0x87364..0x874D5` | `timer89 += delta`; `+0xAA` gate + `[0x10F358]` -> `0x87014` + the `0x7D8B0` id; `[0x10F35C]` -> (0xCC0, `+0xA6`) + id 0x15 + latch, else the chosen-record copy + `+0xA2`/`+0xA6`, distance `>= 0x20` return, `[0x10F358]` hard-approach sync/velocity + the `0x7D8C0` id |
| epilogue `0x874DA..0x874E3` | `ADD ESP,0x20`; POP EBP/EDI/ESI/EDX/ECX/EBX; RET (also jump-table arm 3) |

Hazard notes: the jump selector is the **entry** `stage92` read after the
prologue (the prologue does not touch `+0x92`); the `0x8733A` block is
physically before arm 2 and is reachable only from arm 1's `flag830 == 0`
branch; arm 0's offset when the mode byte is below 4 is a **negate of the
whole z target**, not a sign-magnitude class; the two arm-1/arm-2 gate compares
are signed (`JL`/`JLE`/`JGE`).

### G.3 The internal helper `0x87014` (`0x87014..0x870D5`, 54 insns)

First-hand the helper takes `EAX = rec` and draws from `0x92AC8` six times:
`+0xA2 = max(RNG0 & 0xFF, 0x48)` then negated when `RNG1 & 1`; `+0xA6 =
max(RNG2 & 0xFF, 0x48)` then negated when `RNG3 & 1`; `[rec+0x89] = 0`;
`+0xAA = (RNG4 & 0xFF) + 0x78`; `+0xAE = +0xAA + (RNG5 & 0xFF)` (the `max`
stores are `0x87021..0x8702C`/`0x87060..0x8706B`; the sign arms
`0x87043..0x87056`/`0x87085..0x87098`). `get_xrefs_to 0x87014` is exactly the
two row-28 sites, so — contrary to the plan's Task 8 note that row 2A consumes
it — the helper is **row-28-only**; row 2A's window merely precedes it in the
image. The derived port keeps it file-local in `fifa96_arm_bodies.c` for the
row-28 step.

### G.4 The `0x114E04` fold (`0x87184..0x871C6` x / `0x871CD..0x87216` z)

Both folds load `EAX = [rec+0x8A] SAR 24` (the `(int8)+0x8D` active byte),
`SHL EAX,6`, then run a byte-shift quadrant idiom: the x fold computes
`ECX = EAX + 0x100` and shifts `CH`, the z fold shifts `AH`; each ends
`MOV EAX,[EAX*4+0x114E04]` with an `XOR/SUB` sign fix and applies
`IMUL EDX` by `0x90`, `ADD EAX,0x8000`, `ADC EDX,0`, `SHRD EAX,EDX,16`. The
first-hand table is 257 dwords (`0x114E04..0x115207`; index 256 = `0x10000`).

The port **reuses the already-ported table + quadrant fold**
(`fifa96_projection_sincos`, FU-88) at the 1024-step angle
`(int8)active * 0x40 << 6 = (int8)active << 12` and applies the same
`(component * 0x90 + 0x8000) >> 16`: x uses `cos16`, z uses `sin16`, matching
the fold's `+0x100` x offset (a 90-degree phase shift). Equivalence was
brute-forced over all 256 active values with a Python transcription of the
native instruction sequence against the projection call (0 mismatches) and is
pinned by the `test_arm_28_arm0_fold_table_pins` fixture (0/22.5/45/67.5/90/
-22.5/180 degrees -> (144,0)/(133,55)/(102,102)/(55,133)/(0,144)/(133,-55)/
(-144,0)).

### G.5 The `0x7D8B0`/`0x7D8C0` animation-id tables

`read_memory 0x7D8B0` (32 B) = dwords `{0x15, 0x19, 0x03, 0x02}` then
`{0x00, 0x58, 0x5B, 0x6B}`. The first arm-2 block loads
`EDX = [ESP + EAX*4 + 0xE]; SAR EDX,0x10` with `EAX = RNG & 3`: the frame
copies land at `[ESP]` (`0x7D8C0`) and `[ESP+0x10]` (`0x7D8B0`), so the
`+0xE` dword displacement + `SAR 16` selects the low word of
`0x7D8B0[RNG & 3]` = `{0x15, 0x19, 0x03, 0x02}`. The hard-approach block loads
`EDX = [ESP + EAX*4 - 0x2]; SAR EDX,0x10`, i.e. the low word of
`0x7D8C0[RNG & 3]` = `{0x00, 0x58, 0x5B, 0x6B}`. All ids are passed through
`fifa96_arm_anim_select` (Appendix D.5).

### G.6 Port mapping and derived surfaces

* `fifa96_err_t fifa96_arm_28_step(struct fifa96_arm_record *rec, uint8_t arm)`
  in `src/fifa96_loader/fifa96_arm_bodies.c`; `arm` is the entry stage92 jump
  selector (`0..3`; > 3 and arm 3 take the epilogue).
* `struct fifa96_arm_record` gains the row-28 scratch (`scratch_a0/a1/a2/a6/
  aa/ae`), the five process-global stand-ins (`global_10f358/35c/364/368/
  157ac2`), the resolved `chosen_pos`/`chosen_ok` and `lane` reuse (already
  present). No field is repurposed.
* Engine: `struct fifa96_match_run_record` gains `target_y`, `vel_x/vel_z`,
  `type`, `side`, `flag830`, `chosen_ok/chosen_x/chosen_z`, the six scratch
  cells and the five globals; `struct fifa96_match_entity` gains the six
  scratch cells (the native record `+0xA0..+0xAE` persist across frames);
  `struct fifa96_match_run` gains `struct fifa96_rng rng` and the five globals.
  `match_run_dispatch_entity` stages/repacks them and resolves the pool's
  `chosen831` into `chosen_x/chosen_z/chosen_ok`; `fifa96_match_run_begin`
  seeds the RNG with the derived 0 (OL-56).
* `fifa96_match_action_table[0x28].fn = fifa96_match_action_28`; evidence
  names FU-142d; dispatch expectation flips to `FIFA96_OK`; FU-137 §6.1 class
  `ported` (OL-56..OL-58 carry the remainder).
* Validation/hardening: NULL `rec` -> `-FIFA96_ERR_INVALID`; a draw on a NULL
  `rng` -> `-FIFA96_ERR_INVALID` at that site (the native has no NULL
  concept); no-draw paths (arm 3/>3, gates below their thresholds) tolerate a
  NULL `rng`.
* Tested: `tests/test_arm_bodies.c` (`test_arm_28_*`, 16 cases),
  `tests/test_engine_match_handlers.c::test_action_28_runs_body` and
  `tests/test_engine_match_frame.c::test_action_28_repack_round_trips_fields`
  (pool staging/repack round-trip: scratch persistence, `type`, the resolved
  chosen triple and the arm latches).

### G.7 Open legs (numbered)

* OL-56 — the five process-global inputs and the match RNG seed (§6).
* OL-57 — the `[0x157AA3]` store, the `[rec+0x28]` row-byte stand-in and the
  `0x36200` value 2 (shared OL-51) (§6).
* OL-58 — the chosen-record pointer/ID resolution and the missing-chosen
  bounded model (§6).
* OL-52 (selector record surface) carries the arm's `0x6E598` calls' unmodeled
  native writes; OL-49/OL-54 also apply where the 0x2A scan/reset feed the
  row's inputs.

Write set of this appendix (Task 7):
`include/fifa96_loader/fifa96_arm_helpers.h`,
`include/fifa96_loader/fifa96_arm_bodies.h`,
`src/fifa96_loader/fifa96_arm_bodies.c`,
`include/fifa96_engine/fifa96_match_entities.h`,
`include/fifa96_engine/fifa96_match_run.h`,
`src/fifa96_engine/fifa96_match_run.c`,
`src/fifa96_engine/fifa96_match_handlers.c`, `tests/test_arm_bodies.c`,
`tests/test_engine_match_handlers.c`, `tests/test_engine_match_frame.c`,
`CMakeLists.txt` (arm-bodies link dep),
the §6 OL-47/OL-56..OL-58 updates, the §7 refinements, the FU-137 errata and
this appendix.

## Appendix H (FU142e / M2 arms-and-wiring Task 8) — row 0x2A body + `0x513EC` camera stop first-hand window

Task 8 of the follow-up plan (`docs/superpowers/plans/2026-10-07-fifa96-m2-arms-and-wiring.md`)
ports action row 0x2A (`0x86A34..0x87010`, 409 instructions + the 12-dword
jump table `0x86A04`), its `0x513EC` camera-stop call and the engine wiring.
This appendix is its evidence gate: the exact fields, constants and branch
table below are fixed from the first-hand windows it cites.

### H.1 Tool calls (Ghidra read-only, explicit `/FIFA96.EXE`)

* `disassemble_bytes 0x86A34` (1500 B, 409 instructions, body
  `0x86A34..0x8700F`); `read_memory 0x87010` (4 B) = `C3 8D 40 00`: the final
  RET at `0x87010` plus the 3-byte `LEA EAX,[EAX]` pad before the row-28-only
  helper `0x87014` (G.3 — **row 2A does not consume `0x87014`**; the plan's
  Task 8 note is superseded);
* `read_memory 0x86A04` (48 B) — the 12-dword arm jump table;
* `read_memory 0x1106E0` (180 B) — action-table row `0x2A` at offset 0xA8 =
  `0x00086A34` (file bytes `34 6a 08 00`);
* `disassemble_bytes 0x8D7CF` (72 B, 19 instructions) — the `0x8D807` arm, its
  scan and the `0x8D80C..0x8D80E` `[0x10F35C]` clear;
* `disassemble_bytes 0x513EC` (100 B, 33 instructions, body
  `0x513EC..0x51440`);
* `get_xrefs_to 0x86A34` — 1 reference, the action-table slot `0x110788`;
  `search_byte_patterns 34 6A 08 00` — that same slot is the only occurrence;
* `get_xrefs_to 0x8DCD4` — 78 call sites; `0x8D11E` is the `FUN_0008D098`
  pre-switch walk and none lies inside the record machines
  `FUN_0007CA54`/`FUN_000782D0`, so `+0x65` has no later per-frame writer;
* (fix round 1) `disassemble_bytes 0x86F83` (135 B, 30 instructions) — the
  arm-10 re-read that corrected the `0x86FBD` callee from `0x6E598` to
  `0x79C50` (register setup `EDX = 0x64`, `EAX = rec`, `EBX = 0`);
* repo `grep` over `src/`, `include/`, `tests/` for the helper ground truth.

### H.2 Entry census and decision (first-hand)

The body is not a Ghidra function (`get_function_by_address 0x86A34` =
"no function definition", §1). Entry census: `get_xrefs_to 0x86A34` returns
exactly the action-table slot `0x110788`; the body pointer bytes occur nowhere
else. The static entry is the FU-142a arm `0x8D807` (Appendix B.4, re-read
this slice): state 0x13/0x14, player side, the 0x28 install, the records 1..10
first-`+0x9A == 0` scan (`0x8D7D4..0x8D7F6`), `[team+0x831] = EAX`
(`0x8D801`), `FUN_0007D9A4(rec, 0x2A, 0, 0)` (`0x8D807`) and the
`[0x10F35C] = 0` tail (`0x8D80C..0x8D80E`). Arm + body + pool binding are all
bounded, so **row 2A is wired** (the same gate as rows 26/28).

Layout: jump table `0x86A04..0x86A33`, main body `0x86A34..0x87010` (RET),
3-byte pad, then the unrelated row-28 helper `0x87014..0x870D5`.

### H.3 The 12-arm jump table and per-arm windows

`read_memory 0x86A04` = `{0x86A91, 0x86AFE, 0x86B5F, 0x86BA2, 0x86BDC, 0x86C7D,
0x86D1E, 0x86DBF, 0x86E60, 0x86F2F, 0x86F83, 0x8700A}`; entry 11 is the shared
epilogue at `0x8700A` (six POPs + RET, pop order EBP/EDI/ESI/EDX/ECX/EBX,
matching the six prologue pushes).

Prologue `0x86A34..0x86A89`: `EDX = [rec+0x89]`, `AX = word [0x157A64]`,
`EDX += EAX`, `[rec+0x89] = EDX` (`0x86A3E..0x86A55`, zero-extended delta);
`EAX = dword [rec+0x8F] SAR 24` = the sign-extended `[rec+0x92]` byte,
`CMP EAX,2; JLE` — for signed values > 2 the block `0x86A60..0x86A71` runs
(`EAX = 2`; `[0x157AA3] = EBP`; `word [rec+0x7B] = 4`; `CALL 0x36200`); then
`AL = [rec+0x92]`, `CMP AL,0xB; JA 0x8700A`, `JMP CS:[EAX*4+0x86A04]`.

| arm | window (RET/jump) | first-hand effects |
|---|---|---|
| 0 | `0x86A91..0x86AFD` | `target.x = -0x720`, `target.z = 0`, `[team+0x830] = 0`, `[0x10F35C] = 0`, `[0x10F358] = 0`; `0x79C50(rec, 0, 0)`; `0x6E598` id `0x60`; `timer89 = 0`; `stage92++` |
| 1 | `0x86AFE..0x86B5E` | `(int16)+0x65 > 0x20` -> epilogue; `0x6E598` id `0x61`; `[team+0x830] = 1`; `target = (-0x540, 0)`; `timer89 = 0`; `stage92++` |
| 2 | `0x86B5F..0x86BA1` | `+0x65 > 0x20` -> epilogue; `CALL 0x513EC`; `EAX = 2`; `[0x157AA3] = rec`; `CALL 0x36200`; `timer89 = 0`; `stage92++` |
| 3 | `0x86BA2..0x86BDB` | `timer89 < 0x78` (signed) -> epilogue; `target = (-0x540, -0x930)`; `timer89 = 0`; `stage92++` |
| 4 | `0x86BDC..0x86C7C` | `+0x65 > 0x240` -> epilogue; `d = 0x930 - |pos.z|`; `target.x = -0x540 + {0x90 if d>0x180, 0x120 if d>0xC0, 0x1B0 if d>0x60, else 0x240}`; `pos.z > -0x900` -> epilogue; else `target = (0x540, -0x930)`, `timer89 = 0`, `stage92++` |
| 5 | `0x86C7D..0x86D1D` | `+0x65 > 0x240` -> epilogue; `d = 0x540 - |pos.x|`; `target.z = -0x930 + offset(d)`; `pos.x < 0x510` -> epilogue; else `target = (0x540, 0x930)`, `timer89 = 0`, `stage92++` |
| 6 | `0x86D1E..0x86DBE` | `+0x65 > 0x240` -> epilogue; `d = 0x930 - |pos.z|`; `target.x = 0x540 - offset(d)`; `pos.z < 0x900` -> epilogue; else `target = (-0x540, 0x930)`, `timer89 = 0`, `stage92++` |
| 7 | `0x86DBF..0x86E5F` | `+0x65 > 0x240` -> epilogue; `d = 0x540 - |pos.x|`; `target.z = 0x930 - offset(d)`; `pos.x > -0x510` -> epilogue; else `target = (-0x540, 0x588)`, `timer89 = 0`, `stage92++` |
| 8 | `0x86E60..0x86F2E` | `+0x65 > 0x240` -> epilogue; `d = 0x930 - |pos.z|`; `target.x = -0x540 + offset(d)`; `pos.z > 0x5B8` -> epilogue; four `CALL 0x92AC8`: `target.x = ±(d1 & 0x1FF)` by `d3` bit 0, `target.z = ±(d2 & 0x1FF)` by `d4` bit 0; `timer89 = 0`; `stage92++` |
| 9 | `0x86F2F..0x86F82` | `+0x65 > 0x20` -> epilogue; 3 x MOVSD `target = pos`; `word +0x71/+0x75/+0x73 = 0`; `timer89 = 0`; `[0x10F358] = 1`; `stage92++` |
| 10 | `0x86F83..0x87009` | `0x6E598` id `0x60`; `0x79C50(rec, DX = 0, BX = -100)`; `timer89 < 0x708` -> epilogue; `0x79C50(rec, DX = 0x64, BX = 0)` (the +x octant 2, `0x86FBD`); `target.x = 0xCC0`; `target.z = 0`; `0x6E598` id `0x61`; `timer89 = 0`; `[0x10F35C] = 1`; `stage92++` |
| 11 | `0x8700A` | shared epilogue RET (the pre-dispatch block already ran for signed selectors 3..127) |

The `offset(d)` chain is `0x180 -> 0x90 / 0xC0 -> 0x120 / 0x60 -> 0x1B0 /
else 0x240` (`0x86C0C..0x86C3A` and its four siblings); the position tails are
32-bit `CMP [rec+0x59]/[rec+0x61], imm; JL/JG` (`0x86C43`, `0x86CE4`,
`0x86D85`, `0x86E26`). Arm 0's `0x79C50` call passes `DX = BX = 0`
(`XOR EAX,EAX; MOVSX EBX,AX; MOVSX EDX,AX`), i.e. the zero-direction no-op;
arm 10's first passes `DX = 0`, `BX = 0xFFFFFF9C` (-100), i.e. the -z octant 4,
and its post-gate second `0x79C50` (`0x86FBD`) passes `DX = 0x64`, `BX = 0`,
i.e. the +x octant 2 (a face call, **not** a `0x6E598` id — first-hand
`0x86FB4..0x86FBD`; corrected in fix round 1).

### H.4 The `0x513EC` camera stop (first-hand, re-verified on `/FIFA96.EXE`)

`0x513EC..0x51440`, 33 instructions (operands as the EXE image addresses; the
FU-118 doc's `0x4E5xx` are these minus the `0x100000` LE image delta):
`[0x14E584] = 0` / `[0x14E580] = 0` (`0x513F9`/`0x513FF`), the
`[0x14E5A8] != 0` callback `CALL [EDX+0x38]` with `EDX = 0x14E570`
(`0x51413`), `[0x14E578] = 2` (`0x51421`), and the `[0x14E574] == 0`
first-entry arm `FUN_00064074()` + `[0x14E574] = 1` (`0x5142B..0x51435`). This
matches FU-118 §1 (its provenance was `/fifa96_le.bin`; the same bytes are in
the authoritative `/FIFA96.EXE`, with the image-address delta above). None
of those globals is modeled by the derived engine, so the derived surface is
`fifa96_arm_camera_stop()` = documented no-op returning FIFA96_OK (OL-61).

### H.5 The `+0x65` distance input (row 2A does not compute it)

Row 2A's gates read the record's `+0x65` word (`MOV EAX,[EBP+0x63]; SAR
0x10`) but no call in its window writes it. First-hand `get_xrefs_to 0x8DCD4`:
the only relevant writer is the `FUN_0008D098` pre-switch record walk
(`0x8D11E`, FU-137 §3): per record with `[+0x9A] == 0`, the phase handler runs
and then `0x8DCD4(rec+0x59, rec+0x4D, rec+0x65)` writes the out triple
`{distance +0x65, dx +0x67, dz +0x69}` (Appendix C.3). The 0x2A install
(ECX = 0) does not invoke the handler, so the row body runs in the next frame's
record-machine tail and reads that walk value (one frame stale). The derived
binding models the walk's record-visible effect by recomputing the distance
word from this dispatch call's staged pos/target
(`fifa96_arm_dist_stage`, `match_run_dispatch_entity`) — a bounded model
recorded as OL-60; the `+0x69` lane stays owned by rows 26/27/28/29, which
recompute it in their own prologues.

### H.6 Port mapping and derived surfaces

* `fifa96_err_t fifa96_arm_2a_step(struct fifa96_arm_record *rec, uint8_t arm)`
  in `src/fifa96_loader/fifa96_arm_bodies.c`; `arm` is the raw `[rec+0x92]`
  selector byte: the signed > 2 pre-dispatch gate and the unsigned > 0xB
  epilogue exit are applied exactly as the native (`(int8_t)arm > 2`,
  `arm > 11`), then selectors 0..10 run their arm and 11 takes `default`.
* `struct fifa96_arm_record` gains `distance` (`+0x65`); no field is
  repurposed. `fifa96_arm_camera_stop` lives in
  `include/fifa96_loader/fifa96_arm_helpers.h` /
  `src/fifa96_loader/fifa96_arm_helpers.c` with the other shared family ports.
* Engine: `struct fifa96_match_run_record` gains `distance`;
  `match_run_dispatch_entity` stages it (H.5) and repacks the row-2A outputs
  `target_x/y/z`, `timer89`, `timer7b`, `stage92`, `type`, `flag830`,
  `vel_x/vel_z` and `global_10f358/35c`; the `flag830` and global writes land
  on `mr->entities.team[e->team].flag830` and `mr->global_10f358/35c` so the
  later records of the same frame see them (the native writes are
  process-wide). The globals-comment in `fifa96_match_run.h` (row 28, OL-56)
  now also covers row 2A.
* `fifa96_match_action_table[0x2A].fn = fifa96_match_action_2A`; evidence names
  FU-142e; dispatch expectation flips to `FIFA96_OK`; FU-137 §6.1 class
  `ported` (OL-59..OL-61 carry the remainder; `0x36200` value stays OL-51).
* Arm-tail completion (this task): `fifa96_match_phase_machine_step` now
  performs the arm's `[0x10F35C] = 0` write (`0x8D80C..0x8D80E`) on both the
  found and overflow paths. Task 2 had ported the arm without it; row 2A's
  arm 10 sets the flag and the derived frame reads it back, so the write is
  part of the bounded arm (FU-137 §5.2 errata; OL-56 status).
* Validation/hardening: NULL `rec` -> `-FIFA96_ERR_INVALID`; arm 8's draws on a
  NULL `rng` -> `-FIFA96_ERR_INVALID` at the draw site (the native has no NULL
  concept); no-draw paths tolerate a NULL `rng`.
* Tested: `tests/test_arm_helpers.c` (`test_camera_stop_returns_ok`,
  `test_camera_stop_stateless_repeat`); `tests/test_arm_bodies.c`
  (`test_arm_2a_arm0..arm11` + `test_arm_2a_prologue_delta` +
  `test_arm_2a_invalid`, 14 cases); `tests/test_engine_match_handlers.c`
  (`test_action_2A_runs_body`, `test_action_2A_overflow_fixture` + the flipped
  `action_expect[0x2A]`); `tests/test_engine_match_frame.c`
  (`test_action_2A_repack_round_trips_fields`: pool distance staging, the
  flag830/global repacks and the arms 0/1/9/10 latches);
  `tests/test_engine_match_phase_machine.c` (the `0x8D80E` clear in
  `test_step_arm28_2a_phase14`).

### H.7 Open legs (numbered)

* OL-59 — the `[0x157AA3]` store (prologue + arm 2) (§6).
* OL-60 — the `+0x65` dispatch-time recompute vs the native walk staleness (§6).
* OL-61 — the entire `0x513EC` camera-mode/recorder block (§6).
* OL-52 (selector record surface) carries the arm's non-zero `0x6E598` calls'
  unmodeled native writes; OL-51 carries the `0x36200` EAX=2 values; OL-49/OL-54
  feed the row's install path.

Write set of this appendix (Task 8):
`include/fifa96_loader/fifa96_arm_helpers.h`,
`src/fifa96_loader/fifa96_arm_helpers.c`,
`include/fifa96_loader/fifa96_arm_bodies.h`,
`src/fifa96_loader/fifa96_arm_bodies.c`,
`include/fifa96_engine/fifa96_match_run.h`,
`src/fifa96_engine/fifa96_match_run.c`,
`src/fifa96_engine/fifa96_match_handlers.c`,
`src/fifa96_engine/fifa96_match_phase_machine.c`, `tests/test_arm_helpers.c`,
`tests/test_arm_bodies.c`, `tests/test_engine_match_handlers.c`,
`tests/test_engine_match_frame.c`, `tests/test_engine_match_phase_machine.c`,
the FU-137 §5.2/§6.1/§7 errata and this appendix. `CMakeLists.txt` needed no
change (the arm bodies/engine libraries already link `fifa96_arm_helpers`).

## Appendix I (FU142f / M2 arms-and-wiring Task 9) — dynamic entry closure for rows 27/29/2C and the 2B dead verdict

Task 9 closes OL-48. The plan's method is the operator-approved DOSBox-X
runtime trace of `FUN_0007D9A4` (entry recording, controller-serialized) or,
if capture is unavailable, a written reachability argument. The verdict here
is the written reachability argument, extended from the plan's
`FUN_0008D098`-state-machine fallback to the whole installer call graph: all
**77** first-hand `FUN_0007D9A4` call sites are classified, every
register-derived argument is resolved, every control-flow bypass is checked,
and the stored-pointer/indirect-entry class is excluded. Result: **no
installer invocation anywhere in the program passes 0x27/0x29/0x2C**; the
only codes absent from the observed 0x00..0x2C domain are exactly
0x27/0x29/0x2B/0x2C. Rows 27/29/2C stay unwired (`fn == NULL`,
`-FIFA96_ERR_UNSUPPORTED`) with OL-48 recorded as the final negative census;
2B is a dead entry (shared row-29 epilogue RET, §1.1).

### I.1 Why the runtime-capture path was not used

* The probe rig's templates (`tools/fifa96_patch.py`: `CAVE_TEMPLATE`,
  `VGT_ENTRY_TEMPLATE`, `FVGT_*`) report the call's return halves in the
  EBX/EDX slots (lines 30–35, 45–47) and can pack only an entry **EAX** into
  the site word (`--capture-eax`); no template captures the installer's EDX
  code argument. An entry recording would need new rig code, outside this
  task's file set.
* `tools/keys/` holds front-end/visitor key flows only; no sequence drives a
  live match, and a finite trace can only ever prove a positive entry — it
  cannot establish the negative verdict the three rows need.
* The operator-serialized live session is a controller/human action, not
  available to this implementer slice. The brief's fallback therefore applies.

### I.2 Tool calls (Ghidra read-only, explicit `/FIFA96.EXE`)

| call | result |
|---|---|
| `get_xrefs_to 0x7D9A4` | **77 references, all `UNCONDITIONAL_CALL`**; no DATA ref (I.3) |
| `search_instructions operand=7d9a4` | `match_count=77`, `instructions_scanned=234711`; every match a `CALL` (no `MOV reg,0x7D9A4`, no indirect operand) |
| `search_byte_patterns a4 d9 07 00` | **no matches** — the installer address bytes occur nowhere in the image (no stored function pointer) |
| decompiler scan (DecompInterface Pcode over the 17 defined containing functions) | 35 call ops with argument expressions; the 42 calls in undefined/table-only bodies covered by the raw listing + I.5 |
| `disassemble_bytes` windows | `0x79A70`/`0x78520`/`0x7C9F0`/`0x76640`/`0x7DB80`/`0x80780`/`0x7D9A4`/`0x7DA42`/`0x7DAC1`/`0x8CEB8`/`0x80230`/`0x80670`/`0x7F0F0`/`0x7E870`/`0x76220`/`0x781E0` (bytes in I.4) |
| script: row-06 `[ESP+0x50]` writer scan | 3 writers: `0x80280` (BX = 0 after `0x80270 XOR EBX,EBX`), `0x8069B` (8), `0x807A2` (9) |
| FU-137 §1.1 (carried) | action table `0x1106E0` sole reader `0x7DA77 ADD EAX,0x1106e0` |

### I.3 The 77-site installer census

`EDX source` is the last write on the linear path to each call; I.5 proves no
control-flow entry bypasses it (the four flagged sites are the ones whose code
set below already unions every path). All sites call `FUN_0007D9A4`
(`EDX` = code, FU-137 §2).

| call site | EDX source (first-hand) | possible code(s) |
|---|---|---|
| `00079aac` | MOVSX EDX,DX / MOV EDX,0x19\|0x3 | {0x19,0x3} |
| `00078576` | MOVSX EDX,AX / MOV EAX,0x19\|0x4 | {0x19,0x4} |
| `000782bf` | MOV EDX,0x1c + JMP 0x78200 after MOV EDX,0x4 | {0x4,0x1a,0x1b,0x1c} |
| `0006dcbc` | MOV EDX, 0x7 | {0x7} |
| `0007cd24` | MOV EDX, 0x8 | {0x8} |
| `0007ca48` | MOVSX EDX,DX / MOV EDX,6\|4\|3 | {0x3,0x4,0x6} |
| `00078236` | MOV EDX, 0x1a | {0x1a} |
| `0007825e` | MOV EDX, 0x1a | {0x1a} |
| `00078285` | MOV EDX, 0x1a | {0x1a} |
| `000782a2` | MOV EDX, 0x1b | {0x1b} |
| `0008cf3f` | MOV EDX,[ESP-2]; SAR EDX,0x10 (helper param) | A.4: {0,0x19,0x15,0x25,0x26,0x28} |
| `0008d200` | MOV EDX, 0x1 | {0x1} |
| `0008d238` | MOV EDX, 0x2 | {0x2} |
| `0008d291` | MOV EDX, 0x4 | {0x4} |
| `0008d349` | MOV EDX, 0x10 | {0x10} |
| `0008d421` | MOV EDX, 0x11 | {0x11} |
| `0008d475` | MOV EDX, 0x14 | {0x14} |
| `0008d546` | MOV EDX, 0x13 | {0x13} |
| `0008d569` | MOV EDX, 0x1f | {0x1f} |
| `0008d5c7` | MOV EDX, 0x12 | {0x12} |
| `0008d62c` | MOVSX EDX,AX = 0x1d + (phase==9) | {0x1d,0x1e} |
| `0008d807` | MOV EDX, 0x2a | {0x2a} |
| `00080190` | MOV EDX, 0x7 | {0x7} |
| `000762a4` | MOV EDX, 0x1c | {0x1c} |
| `0007634b` | MOVSX EDX,AX / AX=0x1b+ge; JMPs after EDX=4\|0x19 | {0x4,0x19,0x1b,0x1c} |
| `0007ce93` | MOV EDX, 0xb | {0xb} |
| `0007d149` | MOV EDX, 0x7 | {0x7} |
| `0007d1b9` | MOV EDX, 0x23 | {0x23} |
| `0007db03` | XOR EDX, EDX | {0} |
| `00075b67` | MOV EDX, 0x5 | {0x5} |
| `0007655c` | MOV EDX, 0x1b | {0x1b} |
| `000766b4` | no EDX write; caller MOV EDX,0x4 survives FUN_0007DAB4 | {0x4} |
| `00077719` | MOV EDX, 0x19 | {0x19} |
| `0007f928` | MOV EDX, 0x7 | {0x7} |
| `0007f970` | MOV EDX, 0x7 | {0x7} |
| `0007e5c9` | MOV EDX, 0x7 | {0x7} |
| `0007f791` | MOV EDX, 0x21 | {0x21} |
| `0007f7b6` | MOV EDX, 0x21 | {0x21} |
| `0007f64d` | MOV EDX, 0x4 | {0x4} |
| `00080efa` | MOV EDX, 0xc | {0xc} |
| `00080f6a` | MOV EDX, 0xd | {0xd} |
| `000844e7` | MOV EDX, 0x4 | {0x4} |
| `00084dab` | MOV EDX, 0x13 | {0x13} |
| `00085ca1` | MOV EDX, 0x4 | {0x4} |
| `00086436` | MOV EDX, 0x4 | {0x4} |
| `000899ab` | MOV EDX, 0x20 | {0x20} |
| `0008990a` | MOV EDX, 0x19 | {0x19} |
| `0008994c` | MOV EDX, 0x3 | {0x3} |
| `000899cd` | MOV EDX, 0x1f | {0x1f} |
| `0008a0a0` | MOV EDX, 0x16 | {0x16} |
| `0008a32f` | MOV EDX, 0x18 | {0x18} |
| `0007dba3` | MOVSX EDX,AX / MOV EAX,0x19\|0x3 | {0x19,0x3} |
| `0007e05a` | MOV EDX, 0x4 | {0x4} |
| `0007e85c` | MOV EDX, 0x4 | {0x4} |
| `0007e8ca` | MOV EDX, 0x19 | {0x19} |
| `0007eb5c` | MOV EDX, 0xf | {0xf} |
| `0007ed6a` | MOV EDX, 0xb | {0xb} |
| `0007ef33` | MOV EDX, 0x7 | {0x7} |
| `0007f120` | MOV EDX, 0x6 | {0x6} |
| `0007f133` | MOV EDX,0x5 + JMPs after EDX=6\|MOVSX AX | {0x3,0x19,0x5,0x6} |
| `0007e7b2` | MOV EDX, 0xe | {0xe} |
| `0008025e` | MOV EDX, 0x4 | {0x4} |
| `000807c1` | MOV EDX,[ESP+0x4e]; SAR 0x10 (stack word 0/8/9) | {0x8,0x9} |
| `000816df` | MOV EDX, 0x22 | {0x22} |
| `00081722` | MOV EDX, 0x4 | {0x4} |
| `00081beb` | MOV EDX, 0xc | {0xc} |
| `00083145` | MOV EDX, 0xf | {0xf} |
| `00082f70` | MOV EDX, 0xe | {0xe} |
| `00085383` | MOV EDX, 0x4 | {0x4} |
| `0008753c` | MOV EDX, 0x3 | {0x3} |
| `00089740` | MOV EDX, 0x17 | {0x17} |
| `0008b750` | MOV EDX, 0x24 | {0x24} |
| `0008b78b` | MOV EDX, 0x24 | {0x24} |
| `0007ce1a` | MOV EDX, 0xa | {0xa} |
| `0007cf05` | MOV EDX, 0x8 | {0x8} |
| `0007cfb2` | MOV EDX, 0x9 | {0x9} |
| `0007d046` | MOV EDX, 0x21 | {0x21} |

Union across the 77 sites: **0x00..0x26 (complete) + 0x28 + 0x2A**. Codes in
0x00..0x2C that are never passed: **0x27, 0x29, 0x2B, 0x2C** — exactly rows
27, 29, 2B, 2C.

### I.4 Register-derived sites (first-hand windows, bytes cited)

1. `0x79aac` (`FUN_0007997C`): `0x79a95 JNZ 0x79a9e`; `0x79a97 MOV EDX,0x19`
   (`ba19000000`); `0x79a9e MOV EDX,0x3` (`ba03000000`); `0x79aa3 MOVSX
   EDX,DX` (`0fbfd2`) — code {0x19,3}.
2. `0x78576` (`FUN_000782D0`, keeper machine): `0x7854C MOV EAX,0x4` /
   `0x78553 MOV EAX,0x19`; `0x78558 MOVSX DX,byte [EBP+0x91]`; `0x78567
   MOVSX EDX,AX` (`0fbfd0`) — code {0x19,4}.
3. `0x7ca48` (`FUN_0007C990`): `0x7ca0c MOV EDX,0x6` / `0x7ca28 MOV EDX,0x4`
   / `0x7ca2f MOV EDX,0x3`; `0x7ca41 MOVSX EDX,DX` — code {6,4,3}.
4. `0x766b4` (row-1A window `0x7662C`): `0x766a6 MOV EDX,0x4`
   (`ba04000000`); `0x766ab CALL 0x7DAB4`; FUN_0007DAB4's `0x7DAB6 PUSH EDX`
   (`52`) is paired with `POP EDX` on **both** returns (`0x7DAF7` `5a`,
   `0x7DB09` `5a`), so the caller's 4 survives the reset — code {4}.
5. `0x7dba3` (row-00 body): `0x7db90 JNZ 0x7db99`; `0x7db92 MOV EAX,0x19` /
   `0x7db99 MOV EAX,0x3`; `0x7db9e MOVSX EDX,AX` — code {0x19,3}.
6. `0x8cf3f` (`FUN_0008CEB8`): `0x8cf32 MOV EDX,[ESP-2]` (`8b5424fe`) /
   `0x8cf3c SAR EDX,0x10` (`c1fa10`); the parked CX with the index-0 code-3
   coercion (`0x8cf19..0x8cf25`) — the 15 callers stage only
   {0,3→0x19,0x15,0x25,0x26,0x28} (Appendix A.4, re-read).
7. `0x807c1` (row-06 body): `0x807b1 MOV EDX,[ESP+0x4e]` / `0x807be SAR
   EDX,0x10` = word `[ESP+0x50]`; its only writers are `0x80280 MOV
   [ESP+0x50],BX` with `0x80270 XOR EBX,EBX` (0), `0x8069B` (8) and `0x807A2`
   (9); `0x807A9 CMP word [ESP+0x50],0 / JZ 0x807C6` skips the call at 0 —
   code {8,9}.
8. `0x8d62c` (`FUN_0008D098`): `0x8d607 MOV EAX,[0x157A4A]`; `SAR 0x18`;
   `AND EAX,0xFF`; `CMP EAX,9`; `SETZ AL`; `ADD EAX,0x1D`; `0x8d621 MOVSX
   EDX,AX` — code {0x1D,0x1E}.
9. `0x782bf` (arm `0x77EAC`): linear `0x782b6 MOV EDX,0x1c` (the `0x77E98`
   table's CX=4 entry `0x782b1` also falls into the call); bypass `0x78200
   JMP 0x782bb` reached after `0x781fb MOV EDX,0x4` — **reachable code
   {4,0x1c}** here. The `0x77E98` table's 0x1A/0x1B arms are separate
   `FUN_0007D9A4` call sites (`0x7821d`/`0x78245`/`0x7826d` stage 0x1A,
   `0x78294` stages 0x1B; I.3 rows), so the I.3 cell's
   {4,0x1a,0x1b,0x1c} is a conservative superset for this site.
10. `0x7634b` (`FUN_00076130`): `0x7632c MOV EAX,[EBP+0x63]`; `SAR 0x10`;
    `CMP EAX,0x70`; `SETGE AL`; `ADD EAX,0x1B`; `0x76346 MOVSX EDX,AX` (code
    {0x1B,0x1C}); bypasses `0x76241 JMP 0x7634b` after `0x76238 MOV EDX,0x4`
    and `0x7626a JMP 0x7634b` after `0x76256 MOV EDX,0x19` — code
    {4,0x19,0x1b,0x1c}.
11. `0x7f133`: linear `0x7f12a MOV EDX,0x5`; bypass `0x7f108 MOV EDX,0x6` +
    `0x7f111 JMP 0x7f131`; bypass `0x7e87e MOVSX EDX,AX` (AX from
    `0x7e872/0x7e879` = {3,0x19}) + `0x7e883 JMP 0x7f133` — code
    {5,6,3,0x19}.

No register-derived site can produce a code outside its set, and no set
contains 0x27/0x29/0x2B/0x2C.

### I.5 Control-flow bypass check (all 77 sites)

For every call site the nearest preceding instruction writing
DX/EDX/DL/DH was found via Ghidra's instruction result objects, then every
reference (including computed jumps) targeting any address in
`[write_end, call]` was enumerated: only 4 sites had an entry bypassing the
linear write — `0x782bf`, `0x8cf3f`, `0x7634b`, `0x7f133` — and each bypass
value is one of the constants already unioned in I.4 (#9, #6, #10, #11).
`0x766b4` has no EDX write in 60 instructions and is the callee-preservation
case (#4). The remaining 72 calls (77 − 4 bypass − 1 callee-preserved) have no
bypassing reference: 66 write a constant immediately before the call and six
load EDX from a constant-bounded source through `MOVSX`/a stack word (I.4
#1/#2/#3/#5/#7/#8), so the I.3 cells' code sets still cover them.

### I.6 No indirect installer entry

* All 77 references are direct `CALL`; `search_byte_patterns a4 d9 07 00`
  finds the installer address nowhere in the image, so no static pointer
  (data word, table entry, immediate `MOV reg,imm32`) to `0x7D9A4` exists.
* The action table `0x1106E0` has exactly one reader, `0x7DA77`
  (`ADD EAX,0x1106E0`, FU-137 §1.1), so code 0x27/0x29/0x2C cannot load its
  handler slot except through the installer.
* The record handler slot `[rec+0x18]` is written by the installer
  (`0x7DA8F MOV [ESI+0x18],EAX`); the body pointers themselves
  (`0x86820`, `0x84598`, `0x874E4`) appear nowhere in the image except their
  action-table slots (Appendices D.1/E.4/F.3, re-read).
* Therefore a 27/29/2C body can execute only if an installer invocation
  passes its code; I.3–I.5 exclude every such invocation.

### I.7 Reachability conclusion and per-row verdict

The installer code domain actually reachable in the program is
{0x00..0x26} ∪ {0x28, 0x2A}. **Rows 27/29/2C have no entry**: no direct,
register-derived or stored-pointer installer path passes their code, and the
bodies are unreachable under the static program model. Wiring them would
claim a native entry the program does not have, so per the evidence gate they
stay `fn == NULL`, dispatch `-FIFA96_ERR_UNSUPPORTED`, FU-137 class `unwired`,
and OL-48 carries the verdict record: **negative census, no entry found**
(the residual, unobservable-by-static-means class is a runtime-constructed
call to the installer; no byte in the image supports one; a DOSBox-X entry
trace would only ratify the negative). The rows' ported bodies and fixtures
remain as the Task 4/5/6 artifacts.

### I.8 The 2B dead verdict (final)

`0x87738` is the `RET` byte of row 29's epilogue (shared entry, §1.1), not a
standalone stub; the two helpers after it (`0x8773C..`/`0x8776C..`) belong to
no action row. The census finds no 0x2B invocation anywhere. **2B is a dead
entry**: `fn == NULL`, expectation stays `UNSUP`, FU-137 §6.1 records the
refinement (class `dead entry`, OL-15 closed by the verdict).

### I.9 Wiring decision and write set

No row was wired (no positive entry): `fifa96_match_action_table[0x27/0x29/
0x2C].fn` stay NULL and `0x2B` stays NULL; `include/fifa96_engine/
fifa96_match_run.h` and `src/fifa96_engine/fifa96_match_run.c` are untouched
(no binders). Task 9 write set: `src/fifa96_engine/fifa96_match_handlers.c`
(evidence strings + header), `tests/test_engine_match_handlers.c` (FU-142f
pins + `test_dead_2b_evidence`), the FU-137 §5.3/§6.1/§7 errata and this
appendix.

### I.10 Carried leg — the score event source (child C3-OL2, Task 15)

The M2-B acceptance tape's score step needs the native goal writer. First-hand
check on `/FIFA96.EXE` (Task 15, read-only): `get_xrefs_to 0x93944` returns 11
`UNCONDITIONAL_CALL` references, all in the `0x93D98..0x9486E` goal-handler
cluster, and `search_instructions` (mnemonic `CALL`, operand pattern `93944`)
matches the same 11 sites. No wired row body lies in that cluster (all wired bodies end below
`0x88000`: row 00 `0x7DB10..0x7DBAC`, row 1E `0x7550C..0x755D3`, rows
26/28/2A `0x866F4..0x874E3`, rows 06/07/0F `0x801B4..0x82DCF`, rows 18/21/23
`0x82F84..0x8539B`), and no Gate-G2/G3 appendix records a `FUN_00093944` call
among the wired bodies. The tape therefore drives the derived
`fifa96_match_run_add_goal(mr, side)` directly (its `state=2/1-0` lines pin the
replay) and the native event source stays **carried as child C3-OL2**.

**Erratum (M2 playable-match Task 3).** The tape no longer drives
`add_goal`: since `19c7bc1` its score step calls the derived
`fifa96_match_run_score_event` (L.6), and Task 3 established that no native
writer invoker is reachable from the ported rows/state (census and verdict:
L.9).

## 8. No-write statement (original Task-9 split-gate slice; historical)

No C source, header, test, CMake, asset, ISO or Ghidra state was changed:
Task 9 stops at the split gate. Row classification and dispatch results are
exactly the BASE state: **2 × `FIFA96_OK` (actions 00, 1E), 77 ×
`-FIFA96_ERR_UNSUPPORTED` (73 not-ported + 4 open legs), 1 ×
`-FIFA96_ERR_NOT_FOUND` (phase 0x16)**; `tests/test_engine_match_handlers.c`
expectations unchanged; `make check` **100/100**; M1 golden and pinned render
hashes unchanged.

This statement records the classification at the original Task-9 split gate;
the later M2 tasks moved the counts (current totals in FU-137 §7: 13
`FIFA96_OK` / 66 `-UNSUPPORTED` / 1 `-NOT_FOUND`; `make check` 104/104).

## Provenance

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit): `get_function_by_address`
0x866F4, 0x86820, 0x870E8, 0x874E4, 0x86A34, 0x87738, 0x84598 (all absent),
0x8CEB8, 0x8D098, 0x7DAB4, 0x6E598, 0x6E1D0, 0x8DCD4, 0x36200, 0x513EC,
0x7A490, 0x7B9C4, 0x7E600, 0x7A084, 0x928F0, 0x79C50, 0x8D824, 0x795B4,
0x79B58, 0x92820, 0x8DB6C, 0x786A0, 0x4C324, 0x71C94, 0x79CCC, 0x7F7E0,
0x7B194, 0x7B57C; `disassemble_bytes` 0x866F4 (300 B), 0x86820 (520 B), 0x870E8
(1020 B), 0x874E4 (596 B), 0x86A34 (1716 B), 0x87738 (276 B), 0x84598 (152 B),
0x8D720 (244 B), 0x7F7C0 (64 B); `get_xrefs_to 0x8CEB8` (15, all
FUN_0008D098); repo greps over
`src/`, `include/`, `docs/`. No writes: no rename/comment/label/function/
script/project save. `/fifa96_le.bin` and `/fifa96.exe` untouched.

Repo: `make check` 100/100 before and after the probe (no source change). Write
set: this doc only.

## Appendix J (M2 arms-and-wiring Task 13 / FU-139 §11) — row 06 pursuit machine first-hand window

Task 13 closes OL-30. The native row-06 body is bounded first-hand as
`0x801B4..0x809EF` (~597 instructions; RET at `0x809EF`), ported as
`fifa96_action_pursuit_step` and wired through `fifa96_match_action_06` over
the FU-141 pool. The detailed site-annotated derivation, the engine mapping
and the tests live in FU-139 §11; this appendix records the first-hand window,
the boundary erratum and the interface mapping.

### J.1 Tool calls (Ghidra read-only, explicit `/FIFA96.EXE`)

* `disassemble_bytes`: `0x801B4..0x803B4` (155), `0x803B4..0x80534` (116),
  `0x80534..0x806B4` (124), `0x806B4..0x808B4` (138), `0x808B4..0x80960` (50);
  the row-09 prologue at `0x80A00`;
* `read_memory`: `0x809F0` (16 B; the row-09 stage table
  `{0x80A3F,0x80BFB,0x80FCC,0x8103A}`), `0x1106E0` (slot 9 = `0x80A00`);
* `decompile_function`: `0x8DCD4`, `0x8DD70`, `0x8DC68`, `0x795A4`, `0x8DE8C`,
  `0x79CCC`, `0x79C20`, `0x7D3E4`, `0x79B58`, `0x7D9A4`, `0x7DAB4`, `0x741B4`,
  `0x6DA64`, `0x4B100`, `0x4B02C`;
* `get_xrefs_to`: `0x158724`, `0x15872A`, `0x15872F`, `0x157ABE`, `0x157A4F`;
* no renames, comments, labels, functions, scripts or project saves.

### J.2 Boundary erratum

FU-142 §3's `0x801B4..0x81067` (and the plan/brief) is the span between row 06
and row 08, not one function: row 06 ends at `0x809EF`; the row-09 handler
(`0x80A00..0x81065`, the action-table slot `0x1106E0[9]` with its `0x809F0`
jump table) belongs to action row 09, stays `not ported (partial)` and keeps
`fn == NULL` (OL-9). The Task-13 port and its install-arm census
(`0x8025E` code 4 with `ECX=1` invoke; `0x807C1` codes 8/9 with `ECX=1`)
cover row 06 only.

### J.3 Interface mapping (native → `fifa96_action_pursuit_step`)

| native input | derived field |
|---|---|
| EBP record identity | `actor`/`self_index` |
| `+0x89`, `+0x59/+0x5D/+0x61` | `timer89`, `pos_*` |
| `dword[+0x69]` (low dz, high `+0x6B`) | `lane_dword` |
| `dword[+0x6F]>>16` (`+0x71` word) | `vel_x` |
| `word[+0x6D]/[+0x6F]` | `word6d`/`word6f` |
| `+0x81`, `[0x157A64]`, `[0x157A4A]>>24`, `+0x8D`, `+0x20` | `timer81`, `delta`, `phase`, `active`, `has_slot` |
| `byte[slot+0x10]&0x30`, slot `+0x1D/+0x1E` (`+0x20/+0x21` bytes) | `slot_gate`, `slot_dir_x/z` |
| `team+0x826`, `+0x8B>>24`, `[[rec+0x28]]`, `+0x99`, `+0x9D`, `rec[+4][+0xC]/[+0xE]`, `+0x90` | `side`, `type8`, `row_byte`, `byte99`, `byte9d`, `desc_c/e`, `byte90` |
| `[0x157A4F]`, `[0x15872F]`, `[0x15872A]` | `parity`, `byte_15872f`, `adjust_x` |
| `word[0x157AC5 + 2*idx]` pair | `score_own`/`score_other` |
| `[0x158724]` and its `+0x6B`/`+0x71`/`+0x59..61` | `carrier`, `carrier_lane`, `carrier_speed`, `carrier_pos_*` |
| `team+0x7B2/+0x7B6` identities, `[team+0x7B2]+0x61` | `team_target`/`team_second`, `teammate_z` |
| `[0x157750]`, camera `0x15774C..54`, `word[0x1577C0/C2]`, `team block` (`0xB2` stride, `+0x59/+0x61/+0x98/+0x9A`) | `ball_height`, `camera_*`, `lead_x/z`, `mates[]` |
| `FUN_00092AC8` | `rng` |

The output side maps the native requests: `install` (4/8/9, invoke-now),
`target_*`, `receiver_timer` (`0x79B58`), `anim` (`0x6E598`), the
`team_target_index`/`team_second_index` writes (NONE/SELF/`mates` index),
`swap` (`0x6DA64`), `reset` (`0x7DAB4`) and the `clear_target`/`clear_second`
identity clears. All unmodeled inputs are the numbered OL-69 leg (FU-139
§11.7), staged zero/stand-in in the engine; no parity claim is made over them.

## Appendix K (M2 arms-and-wiring Task 14) — outfield decision machine + interception tail first-hand window

### K.1 Tool calls (Ghidra read-only, explicit `/FIFA96.EXE`)

* `disassemble_bytes`: `0x7CA54..0x7CBF7` (420 B, the machine head through both
  row scans), `0x7CC05..0x7CCFE` (250 B, the no-edge arm and the forced/chase
  tail), `0x7CCFE..0x7CD4D` (80 B, the code-8 install and the machine's own
  `CALL [rec+0x18]`/`0x6E8E8`/`0x79B1C`/`0x7BF20` tail), `0x8D824..0x8D8EB`
  (200 B, `FUN_0008D824`), `0x795B4..0x79603` (80 B, `FUN_000795B4`),
  `0x795A4..0x795E9` (the `0x795A4` multiply), `0xCD474..0xCD4C3` (26 insns),
  `0xCD514..0xCD563` (31 insns), `0xCE364..0xCE38B` / `0xCE386..0xCE3AD`
  (the two fold helpers), `0x8DA30..0x8DB01` (210 B, the interception bind
  call and the `+0x7BE` gate), `0x8D9BD..0x8DA11` (85 B, the interception
  gates); row-04 head `0x7E7C8..0x7EA47` and mid-body `0x7EA47..0x7F19E`;
  row-08 body `0x81068..0x81193` and `0x81193..0x814B2`.
* `decompile_function`: `0x8DDE0` (ranked lane pick), `0x7D3E4` (target clamp),
  `0x8DE8C` (nearest search), `0x741B4` (score-index fold), `0x7E528`
  (corner/`0x158738` staging), `0x8D824`-caller context.
* `read_memory`: `0x110680` (32 B) — the per-type flag table
  `{3,0,0,3,3,3,3,2,0,0,0,0,0,0,0,0,2,2,2,2,0,0,0,0,0,3,...}` (the
  `&1` gate is on for types 0/3/4/5/6/25).
* No writes: no rename/comment/label/function/script/project save.

### K.2 `FUN_0008D824` / `FUN_000795B4` (OL-41, closed)

`0x8D824..0x8D8EB` (81 insns), EAX = the `[0x157A83]` controlled actor
(`0x8DA75 MOV EAX,[0x157A83]` at the call), EDX = nearest record,
EBX = out triple (`&nearest+0x4D` at the `0x8DA7A` call). The z source is the
**actor's** `+0x61` (`0x8D82A MOV EDI,[EAX+0x61]`), not the nearest's:

```
0x8D82A  EDI=[actor+0x61]; ECX=[actor+0x59]; EBP=[nearest+0x59]
0x8D836  EDI = 0xB10-|actor.z|; EAX = EDI/2 + 0x120; EDX = 0xB10-EAX
0x8D865  [out+4] = 0; [out+8] = EDX
0x8D871  if ([[actor]+0x826] != 0) [out+8] = -[out+8]      ; side negation
0x8D87F  if (|actor.x| >= 0x180) [out] = (actor.x+0x180)/3 + 0xC0
         else if (actor.x > 0)   [out] = actor.x-0x240
         else                    [out] = 0x240-actor.x
```
(The `0x8D8AC` `TEST EBP,EBP` arm selects on |nearest.x|, never negative, so
the `(actor.x-0x180)/3-0xC0` branch is dead compiler output and `nearest.x`
feeds nothing live.)

`0x795B4..0x795F0` (28 insns), EAX = position triple, EDX = target triple,
EBX = out triple: `out+2 = (int16)(target.w0 - pos.w0)`,
`out+4 = (int16)(target.w8 - pos.w8)`, `out+0 = (int16)` of
`0xCD514(sign_ext(out+2 word), sign_ext(out+4 word))`. `0xCD514` normalizes
the `0xCD474` angle by `|a|`, mirrors `a > 0x100` over `0x200`, then divides
`(|dx| << 16)` by `0xCE364(a)` for `a > 0x80` and `0xCE386(a)` otherwise;
both fold helpers were verified equal to the `0x114E04` sine primitive
(`sine(a)` and `sine(a+0x100)`) for every `a` in `0..0x100`.

Engine (`team_select_intercept`): bind writes the nearest `+0x4D` triple from
the actor's x/z; the band call passes the nearest's `+0x59` (position) and
the freshly bound `+0x4D` (target) with a **stack scratch** output
(`0x8DA88 LEA EBX,[ESP+0xC]`) — the record's target triple is an input only
and is not written back. The `0x8DA94..0x8DAE7` gate (`(int16)band < 0xF0`
and (`lane word +0x6B > 0x1E0` or `|pos.z| > |[0x157754]| + 0x90`)) sets
`team+0x7BE`; the flag is cleared at `0x8D9C5` on every team update. The
slot-rejected path is OL-71.

### K.3 The `FUN_0007CA54` machine subset (OL-38 half, closed)

Ported as `fifa96_outfield_input_row` (`fifa96_outfield.c`); first-hand
corrections to FU-75 §1.3/§1.7:

* `0x7CABA`: the whole input block runs only when `[rec+0x20] != 0`; a
  slot-less record jumps directly to the `0x7CC82` tail.
* `0x7CAC4..0x7CB08` pre-gate (FU-75 §1.4's "direct arm"): with
  `byte[0x157AB0] != 0`, `rec != [0x1587AC]`, type 3 and
  `(word[slot+6] & 0x20) != 0`, the native calls `0x7D1D4(rec)` and jumps to
  the tail — it does **not** run the code selection.
* The pressed/released scan (`0x7CB7E..0x7CC11`) is **either/or**: the pressed
  word `&0xFF0` zero jumps into the released scan, but once the pressed scan
  runs its terminator (`handler == 0`, `0x7CBB0`) or an accepting handler
  (`0x7CBBD`) jumps to `0x7CC82`, so the released table never runs after a
  pressed scan (FU-75 §1.3's pseudo-code implies a fall-through; erratum).
* No-edge arm `0x7CC13..0x7CC7D`: `(slot[+0x10] & 0xF0) != 0` and phase 2,
  then `d = (int16)(dword[rec+0x69]>>16)`: `d > 0x30 && d < 0x90` fires
  unfiltered; otherwise the side filter (`[0x157A83]` exists and shares the
  side -> refuse) plus `(slot[+0x10] & 0xC0) != 0`. Fire copies the camera
  `0x15774C` triple into `+0x4D/+0x51/+0x55` and calls `0x79B58`.
* Tail `0x7CC82..0x7CD24`: phase 2 and `flat[0x110680+type]&1` gate the
  `0x7C990` forced decision (`fifa96_outfield_forced_action`) and then the
  code-8 gate (`0x7CCB9` not `+0x7B2`, `0x7CCC5` not `+0x7B6`,
  `0x7CCCD` lane `< 0x50`, `0x7CCD8` `[0x157750] < 0x30`, `0x7CCE1` user
  present, `0x7CCED` sides differ, `0x7CD03` unbound, `0x7CD09` timer zero,
  `0x7CD13` `+0x5D` zero). Ported as `fifa96_outfield_chase_gate` + the
  shared `fifa96_outfield_chase_action`; the `0x7D9A4` install itself is the
  engine's request surface.

`fifa96_outfield_chase_gate(state, type, current, next)` is the 26-entry
`0x110680` table (`&1`) composed with the chase predicate; the native
unbounded read beyond type 0x19 is not modelled (no real record type reaches
it).

### K.4 Row bodies 04/08 — split (OL-70)

Row 04 `0x7E7C8..0x7F141` (574 Ghidra defined-code insns; FU-77 §2.4's 649
counts the span, which covers inline data) and row 08 `0x81068..0x814AF`
(213 defined-code; ~231 across the span; FU-141 §7's `..0x81188` head is not
the row end — the row-08 RET is first-hand at `0x814AF`, row 07 starts
`0x814B0`, FU-77 §2.7) are bounded spans whose full record-visible ports
exceed this task. They are registered as OL-70 (row 04) / OL-70a (row 08)
(FU-142 §6) and rows 04/08 keep `fn == NULL`; the FU-137 §6.1 evidence names
the ported machine subset and the row's leg. No parity claim is made over
them.

**Erratum (M2 playability-legs Task 1).** The row-04 half landed in K.5:
`fifa96_outfield_row04_step` + `fifa96_match_action_04`, row 04 wired. Row 08
still keeps `fn == NULL` (OL-70a).

**Erratum (M2 playability-legs Task 2).** The row-08 half landed in K.6:
`fifa96_outfield_row08_step` + `fifa96_match_action_08`, row 08 wired; the
span's defined-code count is corrected to 329 linearly decoded instructions
including the terminal RET (`0x814AF`).

## Appendix K.5 (M2 playability-legs Task 1 / OL-70) — row-04 body first-hand window

Task 1 ports and wires the row-04 handler. /FIFA96.EXE, read-only:
`disassemble_bytes` `0x7E7C8..0x7E9C8`, `0x7E9C8..0x7EBC8`,
`0x7EBC5..0x7EDC0`, `0x7EDBF..0x7EFC0`, `0x7EFC0..0x7F141`, `0x8DE8C..0x8DF1F`
(nearest), `0x8DDE0..0x8DE8B` (ranked pick), `0x8DC68..0x8DD70` (metric),
`0x8DD70..0x8DD84` (angle), `0x7D3E4..0x7D42C` (clamp), `0x79C20..0x79C4E`
(slot target), `0x79B58..0x79B68` (receiver timer), `0x7E528..0x7E5FD`
(corner arm), `0x7E600..0x7E6C0` (decision head), `0x7D9A4..0x7DAD2`
(installer), `0x92820..0x92861`/`0x71C94..0x71CDF` (sinks); no writes.

### K.5.1 Tool calls

* `disassemble_bytes`: the windows above; `disassemble_bytes` was not able to
  define `0x7E7C8` as a Ghidra function (it is reachable only through the
  action-table slot), so the whole body was decoded linearly and each branch
  target was read from its raw bytes;
* helpers already first-hand elsewhere were only re-checked at their entry
  bytes: `0x7DAB4` (K/E), `0x8DCD4` (C/G), `0x79C50`/`0x6E598` (D),
  `0x114E04` folds (J).
* no renames, comments, labels, functions, scripts or project saves.

### K.5.2 Register contract (first-hand)

| register | meaning at the row-04 entry |
|---|---|
| EBP | the record pointer |
| `[ESP+8]` | the record's team block (`[EBP]`), latched at `0x7E80A` |
| `[ESP+0xC]` | the opponent team's target record `[[team+0x7A6]+0x7B2]` (`0x7E994`) |
| EBX/ECX/EDX | per-call scratch; all `0x7D9A4` calls set them explicitly |

Record bytes read (all first-hand): `+0x9E` (ran), `[0x157A4A]>>24` (phase),
`+0x81`, `+0x8D` (active), `+0x20` (slot), `+0x28` (row pointer, byte 0),
`+0x4D/51/55` (target triple), `+0x59/5D/61` (position), `+0x69` (dword; word
`+0x6B` = lane), `+0x6D/6F`, `+0x73`/`+0x75` (word reads of the velocity
halves), `+0x77` (bound), `+0x7D`, `+0x89`, `+0x8B>>24` (the byte at `+0x8E`),
`+0x8E>>24` (the byte at `+0x91`), `+0x99`, `+0x9D`, `rec[+4][+0xB]>>24` (the
byte at `rec[+4][+0xE]`); slot bytes `+0x10` (word), `+0x1D>>24`/`+0x1E>>24`
(the bytes at slot `+0x20`/`+0x21`), `+0x6` (word). The
`dword[addr]>>16`-style word-pair reads (`[0x1577EE]>>16` = word `0x1577F0`,
`[0x1577F8]>>16` = word `0x1577FA`, `[0x157800]>>16` = word `0x157802`,
`[0x1577FE]>>16` = word `0x157800`, `[0x1577BE]>>16` = word `0x1577C0`,
`[0x1577C0]>>16` = word `0x1577C2`, `[0x1577F0]>>16` = word `0x1577F2`) were
the Task-1 misread guard and are pinned here, not as `addr[0]`.

### K.5.3 Word-pair/constant erratum to FU-77 §2.4

* the camera lead is `word[0x1577C0]`/`word[0x1577C2]` (`0x7EBA4` reads
  `[0x1577BE]>>16`, `0x7EBB5` reads `[0x1577C0]>>16`), each `<<2` added to
  the camera triple copy — FU-77's "camera vx/vz lead" wording is right but
  its `0x577C0`/`0x577C2` names were flat offsets (Ghidra `0x1577C0/C2`);
* the `0x13`-row event reload is `word[0x1577FA] = word[0x1577F2] +
  (word[0x1577F2] >> 2)` (`0x7EFD4` reads `[0x1577F0]>>16`), and the
  install-0xF bucket gate sums `word[0x1577FA] + word[0x157802]` against
  `word[0x157800]` (`0x7EB02` reads `[0x1577F8]>>16`, `0x7EB07` reads
  `[0x157800]>>16`, `0x7EB15` reads `[0x1577FE]>>16`);
* the byte at `+0x91` (not `+0x8E`) is the action code: the installer writes
  `AL -> [ESI+0x91]` (`0x7DA67`) and both the `0x110680` gates (`0x7E83C`,
  `0x7E620`, `0x7CC9C`) and the `+0x91 == 4` hand-off test (`0x7E964`) read
  `[rec+0x8E]>>24`. The pool's `type`/`code` naming note in FU-142 K.3's
  `fifa96_outfield_chase_gate` wrapper is unaffected (its callers pass the
  record's `+0x91` value).

### K.5.4 Decision logic (site-annotated)

Prologue and carrier arm:

```
0x7E7D3  byte[+0x9E] = 1
0x7E7E2  if ([0x157A4A]>>24 != 2) { FUN_0007DAB4(rec); return }
0x7E7F3  if (word[+0x81] != 0) return
0x7E80E  if (rec == [0x158777]) {                     ; carrier arm
0x7E812    skip = byte[[0x158777]+0x8D]
0x7E818    ECX = 0                                     ; NULL out
0x7E824    nearest = FUN_0008DE8C(0x157770, team, skip, 0)
0x7E82D    [team+0x7B2] = nearest
0x7E833    code = byte[nearest+0x91]
0x7E83C    if ((flat[0x110680+code] & 1) == 0) return
0x7E85C    FUN_0007D9A4(rec, 4, invoke, 0)
0x7E866/83 FUN_0007D9A4(rec, byte[+0x8D] ? 3 : 0x19, invoke, 0); return }
0x7E888  if (byte[+0x8D] == 0) {                      ; inactive arm
0x7E895    opp = [team+0x7A6]
0x7E89F    picked = [opp+0x7C7] ? that pointer : FUN_0008DDE0(opp, 0)
0x7E8B2    if (word[picked+0x6B] > word[rec+0x6B]) goto 0x7E92F
0x7E8C8    FUN_0007D9A4(rec, 0x19, invoke, 0)
0x7E8D3    if (rec == [team+0x7B2]) [team+0x7B2] = FUN_0008DE8C(0x157770, team, 0, 0)
0x7E8FF    else if (rec == [team+0x7B6]) [team+0x7B6] = FUN_0008DE8C(0x157770, team, 0, 0)
             return }
0x7E92F  if (rec == [team+0x7B6]) {
0x7E93D    t = [team+0x7B2]
0x7E943    if (t == rec) [team+0x7B6] = 0
0x7E957    else if (t != 0 && byte[t+0x91] == 4 && word[t+0x6B] <= word[rec+0x6B]) {
0x7E973      FUN_0007DAB4(rec); return } }
```

Target arms (`AX = rec == [0x1577CA] && dword[0x157750] > 0x50`, `ECX` the
lead latch):

```
0x7E984  other = [[team+0x7A6]+0x7B2]
0x7E99E  AX = ([0x1577CA] == rec && dword[0x157750] > 0x50)
0x7E9B7  if (slot != 0) {
0x7E9C2    if ((word[slot+0x10] & 0x20) && word[0x1577F0] > 0x70) {
0x7E9E1      +0x4D..55 = 0x157770 triple; goto clamp }
0x7E9F1    if (!AX && ([0x157A83] == 0 || [0x157A83] == rec) && word[+0x6B] < 0x60)
             ECX = 1 }
         else ECX = 1
0x7EA18  if (AX) goto 0x7EB95
0x7EA21  if (word[0x1577F0] > 0x70) {
0x7EA32    if (word[0x1577FA] < word[0x157800]) {
0x7EA44      if (!ECX) goto 0x7EB95
0x7EA4A      +0x4D..55 = 0x157788 triple
0x7EA55/68   x += (rng & 0x20) - 0x10 ; z += (rng & 0x20) - 0x10 }
0x7EA7B    if (slot && (word[slot+0x10] & 0x50) == 0) goto 0x7EBF8
0x7EA9B    if (!active || byte[+0x99] || dword[+0x5D]) goto 0x7EBF8
0x7EABF    if (word[0x1577FA] <= word[0x1577F2]) goto 0x7EBF8
0x7EAD2    if (rec != [team+0x7C7]) goto 0x7EBF8
0x7EAE9    if (word[+0x6B] >= 0x120) goto 0x7EBF8
0x7EAF4    if (word[+0x6B] > word[+0x77]) goto 0x7EBF8
0x7EB02    if (word[0x1577FA] + word[0x157802] < word[0x157800]) goto 0x7EBF8
0x7EB3F    if (0x8DC68(0x157788.x-rec.x, 0x157788.z-rec.z) >= 0x60) goto 0x7EBF8
0x7EB5C    FUN_0007D9A4(rec, 0xF, invoke, 0); RETURN }   ; before the clamp/timer
0x7EB6B  else if (word[0x1577F0] > 0x50) {
0x7EB70    if (word[0x1577FA] >= word[0x157806]) goto 0x7EBF8
0x7EB86    if (!ECX) goto 0x7EB95
0x7EB88    +0x4D..55 = 0x157794 triple; goto 0x7EBF8
         } else goto 0x7EB95
0x7EB95  if (!ECX) goto 0x7EBF8
0x7EB9A  +0x4D..55 = 0x15774C camera triple
0x7EBA4  x += word[0x1577C0] << 2 ; z += word[0x1577C2] << 2
0x7EBCD  if (word[+0x6B] < 0x3C0 && |dword[0x157754]| > 0x570) FUN_00079B58(rec)
0x7EBF8  if (!ECX) FUN_00079C20(rec, byte[slot+0x1D]>>24, byte[slot+0x1E]>>24)
0x7EC13  FUN_0007D3E4(+0x4D)                          ; clamp ±0x720 / ±0xB10
0x7EC1B  dword[+0x89] += (uint16)[0x157A64]
```

Slot/no-slot/install block and tail:

```
0x7EC34  if (slot) { if (word[slot+6]) FUN_00078A84(slot); goto 0x7ED89 }
0x7EC40  else if (byte[team+0x828]) {
0x7EC49    if (byte[team+0x7E7]==0 && word[+0x6B] < 0xF0 && dword[team+0x7BF]==0 &&
             active && byte[0x1586D7]==0) FUN_0007876C(rec) }
0x7EC87  else if (FUN_0007E600(rec)) { FUN_0007D9A4(rec, 0xE, invoke, 0); RETURN }
0x7EC97  if (active && other != 0) {
0x7ECAF    if ((rng & 0x3F) == 0 && 0x8DCD4(rec.pos, other.pos) <= 0x50) {
0x7ECDD      own = word[0x157AC5 + 2*0x741B4(side)]
0x7ED09      other_score = word[0x157AC5 + 2*0x741B4(side^1)]
0x7ED19      if (own+2 < other_score || 3*dword[team+0x7D7] < dword[opp+0x7D7]) {
0x7ED3D        thresh = (int8)rec[+4][+0xE] | byte[+0x9D]
0x7ED50        if ((rng & 0x1F) < thresh) { FUN_0007D9A4(rec, 0xB, invoke, 0); RETURN } } } }
0x7ED89  if ((int16)(0x40 - word[+0x81]) >= word[+0x6B]) goto 0x7EDCE
0x7EDA0  if (rec == [0x1577CA]) return
0x7EDB2  if ((int16)word[+0x6B] >= 0x90) return
0x7EDBD  FUN_00079B58(rec); return
0x7EDCE  if (word[+0x6B] > word[+0x77]) return
0x7EDDF  if (dword[0x157750] <= 0x50) goto 0x7EE4B
0x7EDE1  if (!active || dword[+0x5D]) return
0x7EDF8  if (0x8DCD4(rec.pos, 0x157770) >= 0x30) return
0x7EE17  if (byte[[rec+0x28]] == 0x13) return
0x7EE2A  FUN_0006E598(rec, 0x13, byte[+0x8E], 0); return
0x7EE4B  angle = FUN_0008DD70(word[+0x6D], word[+0x6F])
0x7EE5C  angle = (angle - word[+0x7D]) & 0x3FF ; mirror over 0x200
0x7EE78  if (angle > 0xAB) return
0x7EE83  if ([0x1577CA] && rec != [0x1577CA] && team side == [[0x1577CA]]side)
           0x92AC8 -> 0x974F0 / 0x651F0
0x7EEC0  if (rec == [team+0x7B6]) { [team+0x7B6] = 0; [team+0x7B2] = rec }
0x7EEE8  [opp+0x7E7] = 0                               ; opponent team byte
0x7EEEF  if (byte[team+0x7E7]) {
0x7EEFC    if (FUN_0007E528(rec)) return               ; install 7 + increment
0x7EF0C    goto 0x7EF42 }
0x7EF0E  else if (slot) { FUN_00078AA4(slot);
0x7EF1A    if (rec == [team+0x7CB]) { FUN_0007D9A4(rec, 7, invoke, 1); return } }
0x7EF42  if (byte[[rec+0x28]] == 0x13) {
0x7EF55    0x974DC(4,4); 0x8F188(rec,0x6B)
0x7EF70    x = (int8)0x10F334[byte[+0x8E]] << 6 ; z = (int8)0x10F33C[byte+0x8E] << 6
0x7EFB9    0x92820(rec, 0, 0x15) ; 0x71C94(rec, &triple)
0x7EFD4    word[0x1577FA] = word[0x1577F2] + (word[0x1577F2] >> 2) }
0x7EFEE  else if (rec != [0x1577CA]) {
0x7F003    if (dword[0x157750])    x = (int8)0x10F334[..] << 5 ; z = ..0x10F33C.. << 5
0x7F035    else if (slot)        x = (int8)slot[+0x20] << 6 ; z = (int8)slot[+0x21] << 6
0x7F056    else if (|cam.x| <= 0x630 && |cam.z| <= 0xA20)
                                   x = word[+0x73] << 2 ; z = word[+0x75] << 2 else 0
0x7F0A7    0x8DC68(x, z)
0x7F0BB    0x92820(rec, 0, 0x1D) ; 0x71C94(rec, &triple) }
0x7F0D6  if (active && other && (byte[other+0x91] == 4 || byte[other+0x91] == 5)) {
0x7F106    if (word[+0x6B] > word[other+0x6B]) { FUN_0007D9A4(rec, 6, 0, 0); RETURN }
0x7F113    else FUN_0007D9A4(other, 6, 0, 0) }
0x7F125  FUN_0007D9A4(rec, 5, invoke, 0)              ; always unless an earlier RETURN
0x7F138  epilogue RET
```

`FUN_0007E528` (`0x7E528..0x7E5FD`): returns 0 unless `byte[team+0x7E7]` is
1 or 2; otherwise it stages `byte[0x158743] = 3` for the side-0 z >= 0xB10
arm (or side 1 z <= -0xB10), else 2, calls `0x8DCD4(rec.pos, team triple,
0x158738)`, `FUN_0007D9A4(rec, 7, invoke, 0)`, increments `byte[team+0x7E7]`
and wraps it to 0 when it exceeds 2, and returns 1.

### K.5.5 Helper signatures (first-hand, for the port)

* `0x8DE8C(EAX=origin triple, EDX=team, BX=skip index, ECX=out dist word)`:
  11-record loop, `+0x9A`/`+0x98` skips, `0x8DC68` word distance, strict `<`
  (unsigned), best 0xFFFF, returns the record pointer or 0 and writes the best
  distance through ECX (the carrier call passes ECX=0).
* `0x8DDE0(EAX=record base, EDX=skip)`: same loop/skips on the `+0x6B` word,
  returns the pointer with the smallest lane or 0; the row-04 call passes
  EDX=0 (record 0 skipped).
* `0x8DCD4(EAX=from triple, EDX=to triple, EBX=out triple)`: `out+2 = dx`,
  `out+4 = dz`, `out+0 = 0x8DC68(dx, dz)`.
* `0x8DC68(EAX=x, EDX=z)`: octagonal distance word.
* `0x8DD70(EAX=x, EDX=z)`: `FUN_000CD474(x, z)` angle.
* `0x7D3E4(EAX=target triple)`: clamp x to ±0x720 and z to ±0xB10.
* `0x79C20(EAX=rec, DX=dir_x, BX=dir_z)`: `target = (pos_x + dx<<7, 0,
  pos_z + dz<<7)` then `0x7D3E4`.
* `0x79B58(EAX=rec)`: `if (byte[+0x99] == 0) byte[+0x93] = 0x10`.
* `0x7D9A4(EAX=rec, EDX=code, ECX=invoke, BL=stage)`: the FU-137 §2 installer
  (`0x7DA67` writes the code byte, `0x7DA95` the stage; ECX != 0 calls
  `[rec+0x18]`).
* `0x7E600(EAX=rec)` decision: `0x7E617` gates on
  `flat[0x110680 + byte[rec+0x91]] & 1`, excludes `rec == [0x1577CA]`, and
  otherwise follows the bounded gates the kick machine already ports.

### K.5.6 Port mapping (native -> derived)

| native | derived |
|---|---|
| EBP record | `mr->record` staging + the resolved pool record `e` |
| `[ESP+8]` team / `[team+0x7A6]` opp | `mr->entities.team[team]` / `.team[1-team]` |
| `[0x158777]` carrier | `id == mr->entities.ball.carrier` (stand-in) |
| `[0x1577CA]` | `id == mr->entities.controlled` (stand-in) |
| `[0x157A83]` | `mr->entities.controlled >= 0` + `id == controlled` |
| team `+0x828` / `+0x7BF` | pool `slot_pool` / `chosen >= 0` |
| team `+0x7B2`/`+0x7B6`/`[[+0x7A6]+0x7B2]` | pool target/second/opponent-target ids resolved to `mates`/`opps` indices |
| `0x8DE8C`/`0x8DDE0` | `row04_nearest`/`row04_ranked_pick` over caller team views |
| `0x8DCD4`/`0x8DC68`/`0x8DD70`/`0x7D3E4`/`0x79C20` | `row04_metric`/`fifa96_entity_distance`/`fifa96_entity_angle`/inline clamp/slot-dir compose |
| `0x7D9A4` | `out.installs[]` applied in order by `fifa96_match_action_04` |
| `0x7DAB4` | `out.reset` -> `match_row_reset` |
| `0x79B58` | `out.receiver_timer` -> `timer93 = 0x10` (when `+0x99 == 0`) |
| `0x7876C`/`0x78A84`/`0x78AA4` | `out.slot_merge` (pool merge) / backup+restore requests (no consumer, OL-65) |
| `0x6E598` | `out.anim` (kind 0x13; no consumer, OL-52) |
| `0x7E528` | `out.corner`/`corner_code`/`team7e7_inc` (team byte unmodeled, OL-72) |
| `0x974DC`/`0x8F188`/`0x92820`/`0x71C94`/`0x974F0`/`0x651F0` | `out.events`/`event_code`/`event_x/z`/`event_track_reload` (sinks unported, OL-67/OL-72) |

### K.5.7 Tests

`tests/test_outfield.c` fixtures (hand-computed from K.5.4): prologue
reset/`+0x81`; carrier arm (nearest skip, the `0x110680` code gate, installs
4 + 3/0x19); inactive ranked pick; active second/target reset; the camera+lead,
`0x157788` jitter, `0x157794`, slot-direction and clamp target arms; the
slot bit-0x20 direct `0x157770` copy; install-0xF; install-0xB (seed-0 draws
512/1829); the no-slot `0x7E600` install-0x0E; the bound/angle tail and the
6/5 installs (both the self-6 return and the other-6 + 5 fall-through); the
`0x7E528` corner codes; the row-byte `0x13` event 0x15 and the `0x1577FA`
reload; the `team+0x828` merge and slot `+6` backup requests; the
`[opp+0x7C7]` pick short-circuit.
`tests/test_engine_match_handlers.c::test_action_04_wired_and_08_unwired` runs
the wired row over the pool (reset, camera target, receiver timer), and
`action_expect[0x04]` is `FIFA96_OK`.

### K.5.8 Open legs (numbered)

* **OL-72 — row-04 unmodeled inputs/sinks.** The pool models neither the
  record bytes `+0x99`/`+0x9D`/`+0x44`/`[[rec+0x28]]` nor the roster
  descriptor `rec[+4][+0xE]`, the team bytes `+0x7C7`/`+0x7CB`/`+0x7D7`/
  `+0x7E7`/the `+0x7E8` corner triples, the `[0x1586D7]` merge gate, the
  `[0x157ABE]` half-flip, the `0x1577F0..0x157806` track words, the
  `0x1577C0/C2` lead words or the `0x71B9C(4)` predictor; the engine stages
  zero / camera stand-ins (K.5.6) and the corresponding branches
  (`0xF` unlock, the `0x1F` tilt threshold, corner, `team_7e7` install 7,
  `3*team_7D7`, the `0x1577F0` bands, the predictor gates) stay inert until
  the producers land. `[0x158777]` and `[0x1577CA]` are pool stand-ins.
* **OL-67 (carried) — the event/audio sinks.** The `0x92820`/`0x71C94`/
  `0x974DC`/`0x8F188`/`0x974F0`/`0x651F0` calls remain requests.
* **OL-52 (carried) — the `0x6E598` anim request** stays unconsumed.
* **Invoke-now modeling.** The native installs invoke the new handler
  synchronously (`0x7D9A4` ECX != 0); the derived binder applies the install
  sequence to the record fields in order (the carrier arm's 4-then-3/0x19
  net, the tail's 6-then-5 pair) without running the intermediate action
  bodies — the same standing convention as rows 06/07/0F/18/21/23 (the
  installed action dispatches next frame).

## Appendix K.5b — FU-137 §6.1/§7 errata (Task 1)

Row `04` is `ported`: `fifa96_match_action_04` binds
`fifa96_outfield_row04_step` (`0x7E7C8..0x7F141`) to `mr->record`/the pool
(K.5). The `0x7CA54` machine subset stays its own unwired seam. The FU-137
§6.1 `04` cell and §7 counts become 12 OK / 67 UNSUP / 1 NOTF. Row `08` is
unchanged (OL-70a). FU-75 §6's machine-subset row and §9 leg 10 gain the same
pointer; FU-141
§7's OL-38 status gains a Task-1 note.

## Appendix K.6 (M2 playability-legs Task 2 / OL-70a) — row-08 body first-hand window

Task 2 ports and wires the row-08 handler. /FIFA96.EXE, read-only:
`disassemble_bytes` `0x81068..0x8127F` and `0x81280..0x814AF` (one full-span
call reports `instructions_total` 329 including the terminal RET), helpers
`0x79C50..0x79C97` (face), `0x79B58..0x79B68` (receiver timer),
`0x79B1C..0x79B56` (snap), `0x795A4..0x795B2` (fold), `0xCD474..0xCD4C2`
(angle), `0x8ED40..0x8EE03` (event selector), `0x8F188..0x8F1BF` (event sink
head), `0x7A490..0x7A4EF` (staging head), `0x92AC8` (RNG);
`get_function_by_address` `0x8ED40` (body end `0x8EE03`); `get_xrefs_to`
`0x15877D`/`0x15872F`; no writes.

### K.6.1 Tool calls

* `disassemble_bytes`: the two row windows and the helper windows above; the
  body is reachable only through the action-table slot (no Ghidra function at
  `0x81068`), so the whole body was decoded linearly and every branch target
  and operand was read from the raw bytes;
* helpers already first-hand elsewhere were re-checked at their entry bytes:
  `0x114E04`/`0x8DC68`/`0x8DCD4` (entity_update), `0x79C50`/`0x79B58`/
  `0x79B1C`, `0x92AC8` (`fifa96_rng_step`);
* no renames, comments, labels, functions, scripts or project saves.

### K.6.2 Register contract (first-hand)

| register | meaning at the row-08 entry |
|---|---|
| EBP | the record pointer |
| `[ESP+0x18]` | the q radius word (written `0x81279`, read via `[ESP+0x16]>>16`) |
| `[ESP+0x1C]`/`[ESP+0x14]` | sine(word[+0x7D]) / sine(word[+0x7D]+0x100) words |
| `[ESP]`/`+4`/`+8` | the scan's local position triple |
| `[ESP+0xC..+0x10]` | the `0x8DCD4` out triple, then the final metric and X/Z |
| EBX/ECX/EDX | per-call scratch |

Record bytes read (all first-hand): `+0x92` (stage), `+0x89` (dword timer),
`+0x8D` (active), `+0x20` (slot), `+0x6B` (word, `[+0x69]>>16` and the face
DX), `+0x6D` (word, face BX), `+0x7D` (word, the face angle), `+0x3D`
(`[+0x3A]>>24`), `+0x44`, `+0x9E`, `+0x8E` (`[+0x8B]>>24`, the type-table
index and face octant), `+0x59/+0x5D/+0x61` (position triple),
`+0x4D/+0x51/+0x55` (target triple), `rec[+4][+0xC]`/`[+0x16]`, the opponent
target's `rec[+4][+0xC]`/`[+0xF]`, `byte[[user+0x28]]`, the `[0x157A83]` user
pointer, `[opp+0x7B2]`, `[team+0x7B2]`/`[team+0x7B6]`. Globals: `[0x157A4A]>>24`
(phase), `[0x157A64]` (delta word), `[0x157750]` (dword), `[0x1577C0]`/
`[0x1577C2]` (lead words via `[0x1577BE]>>16`/`[0x1577C0]>>16`), `0x15774C`
camera triple, `[0x15877D]`, `[0x15872F]`, `[0x1577CA]`. The `0x110680` gate
is not used by this body.

Word-pair guard: `[0x1577BE]>>16` = word `0x1577C0`, `[0x1577C0]>>16` = word
`0x1577C2`, `[ESP+0x16]>>16` = the word stored at `[ESP+0x18]` (the q radius).
The type tables are indexed by `MOV EAX,[EBP+0x8B]; SAR EAX,0x18` = the byte
at `+0x8E`, and `0x79C50` writes that byte.

### K.6.3 Decision logic (site-annotated)

Prologue / stage dispatch:

```
0x81073  if ([0x157A4A]>>24 != 2) { FUN_0007DAB4(rec); return }
0x8108E  timer89 += (uint16)[0x157A64]                 ; store 0x810A2
0x810A8  if (stage < 1) goto 0x810C4                   ; stage 0
0x810AC  if (stage <= 1) goto 0x811D6                  ; stage 1
0x810B2  if (stage == 2) goto 0x8147C                  ; stage 2
         return                                        ; stage >= 3
```

Stage 0 (`0x810C4..0x811D4`):

```
0x810CC  if (byte[+0x8D] == 0) {                       ; inactive
0x810D5    FUN_0007DAB4(rec)
0x810DF    if (rec == [team+0x7B2]) [team+0x7B2] = 0
0x810F4    if (rec == [team+0x7B6]) [team+0x7B6] = 0
           return }
0x81114  +0x4D..+0x55 = 0x15774C camera triple
0x8112A  +0x4D += word[0x1577C0] << 3
0x81140  +0x55 += word[0x1577C2] << 3
0x81148  if (slot != 0) goto 0x81185
0x8114C  if ((int16)word[+0x6B] <= 0x50 && dword[0x157750] <= 0x38) goto 0x81185
0x81160  FUN_00079B58(rec)                             ; +0x93 = 0x10 when +0x99 == 0
0x81167  if ((int32)[+0x89] <= 0x3C) return
0x81174  FUN_0007DAB4(rec); return                     ; timer > 0x3C reset
0x81185  FUN_00079C50(rec, DX=word[+0x6B], BX=word[+0x6D])  ; +0x7D angle, +0x8E octant
0x81198  FUN_0006E598(rec, 0xB, byte[+0x8E], 0)
0x811AF  byte[+0x9E] = 1
0x811B6  dword[+0x89] = 0
0x811C0  byte[+0x92]++ ; [0x15877D] = 0
```

Stage-1 gate / stage-0 continuation (`0x811D6..0x81214`):

```
0x811D6  if ([0x15877D] != 0 || byte[+0x44] != 0) { byte[+0x92]++; [+0x89] = 0; return }
0x81207  if (byte[+0x3D] != 1) return
```

Projection scan (`0x81216..0x81472`):

```
0x81216  opp = [[rec]+0x7A6]; user = [0x157A83]; opp_target = [opp+0x7B2]
0x8122B  if (user != 0 && opp_target == user)
           delta = ((int8)rec[+4][+0xC] + (int8)rec[+4][+0x16])
                 - ((int8)opp_target[+4][+0xC] + (int8)opp_target[+4][+0xF])
         else delta = (int8)rec[+4][+0xC] + (int8)rec[+4][+0x16]
0x81266  q = (int16)(6*delta + (int8)[0x15872F]*8 + 0x20)
0x81285  if (q > 0x40) q = 0x40 else if (q < 8) q = 8
0x8129F  sine1 = sine(word[+0x7D])            ; low word, sign-extended by the caller
0x812C7  sine2 = sine(word[+0x7D] + 0x100)
0x812F5  for (cx = 0; cx < 0x40; cx += 0x10) {
0x812FC    local = [rec+0x59..0x61]
0x8130D    local.x += FUN_000795A4(cx, sine1)
0x81326    local.z += FUN_000795A4(cx, sine2)
0x81347    d = FUN_0008DCD4(0x15774C, &local).distance
0x8134C    if ((int16)q <= (int16)d) continue
0x81357    X = (int8)0x10F334[byte +0x8E] * 0xA0
0x81373    Z = (int8)0x10F33C[byte +0x8E] * 0xA0
0x8139D    dist = FUN_0008DC68((int16)d, X)
0x813A2    if ([0x157A83] != 0 && byte[[[0x157A83]+0x28]] == 0x4A) {
0x813C3      0x8ED40(user, 2, 4); 0x8F188(0x67, user); 0x651F0(1)
           } else {
0x813EE      0x8ED40(rec, 2, 0); 0x92820(rec, 0x16)
0x81406      0x7A490(rec, &{dist,X,Z}, 0, 9, -1, 0); [0x15877D] = 1 }
0x81423    r = FUN_00092AC8(); 0x974F0(0x190 + (r & 0x7F)); 0x651F0(1)
0x81441    byte[+0x92]++ ; dword[+0x89] = 0; return }
0x81472  return                                        ; loop miss
```

Stage 2 (`0x8147C..0x814AF`):

```
0x8147E  [0x15877D] = 0
0x81484  FUN_00079B1C(rec)                     ; snap: target = pos, lane/velocity zero
0x8148B  if (byte[+0x44] != 0) {
0x81493    FUN_0007DAB4(rec)
0x81498    if (rec != [0x1577CA]) byte[0x15872F]++ }
```

### K.6.4 Helper signatures (first-hand, for the port)

* `0x79C50(EAX=rec, DX=(int16)word[+0x6B], BX=(int16)word[+0x6D])`: `DX|BX ==
  0` returns `byte[+0x8E]` untouched (`0x79C59..0x79C68`); otherwise
  `word[+0x7D] = FUN_000CD474(DX,BX)` and
  `byte[+0x8E] = ((angle+0x40)&0x3FF)>>7`, returning the sign-extended byte.
* `0x79B58(EAX=rec)`: `if (byte[+0x99] == 0) byte[+0x93] = 0x10`.
* `0x79B1C(EAX=rec)`: `+0x4D..+0x55 = +0x59..+0x61`; `word[+0x69] = 0`,
  `word[+0x71] = 0`, `byte[+0x9C] = 0`, `word[+0x67] = word[+0x69]`,
  `word[+0x65] = word[+0x69]`, `word[+0x75] = word[+0x73] = word[+0x71]`.
* `0x795A4(EAX=a, EDX=b)`: `IMUL EDX`, `ADD/ADC 0x8000`, `SHRD 0x10`; the
  callers `MOVSX` the low word.
* `0x114E04(angle)`: the 257-entry sine fold (ported as `fifa96_entity_sine`);
  the row stores the low word and the folds sign-extend it.
* `0x8DCD4(from,to)`: the {distance,dx,dz} triple (the derived row metric uses
  the distance); `0x8DC68(x,z)`: the octagonal distance.
* `0x7DAB4(rec)`: the reset (FU-137 §2; `fifa96_arm_reset`/`match_row_reset`).
* `0x8ED40` (`0x8ED40..0x8EE03`): a 16-slot `0x15A998`-family event/stat
  selector; no record writes (unported sink).
* `0x6E598(rec, 0xB, byte[+0x8E], 0)`: the animation id resolution request
  (OL-52).
* `0x7A490(rec, EDX=&{dist,X,Z}, EBX=0, ECX=9, stack -1/0)`: the FU-73 §1 ball
  staging core (actor/6-byte vector/traj/code 9); the derived
  `fifa96_ball_pair_stage` core plus the unported tail (OL-62).

### K.6.5 Port mapping (native -> derived)

| native | derived |
|---|---|
| EBP record | `mr->record` staging + the resolved pool record `e` |
| `[ESP+8]` team / `[team+0x7A6]` opp | `mr->entities.team[team]` / `.team[1-team]` |
| `[0x157A83]` user | `mr->entities.controlled` (stand-in) |
| `[0x1577CA]` | `id == mr->entities.controlled` (stand-in) |
| `[team+0x7B2]`/`[team+0x7B6]` | pool target/second ids |
| `[[team+0x7A6]+0x7B2]` | opponent team target id |
| `0x79C50` | `out.face`/`face_angle`/`face_octant` -> `record.type` (+0x8E; the row-04/06/07/18 readers still use the never-written `record.actor_type` — OL-83) |
| `0x79B58` | `out.receiver_timer` -> `timer93 = 0x10` (the +0x99 gate is the binder's) |
| `0x79B1C` | `out.snap` -> target = pos (the lane/velocity zeroes OL-82) |
| `0x795A4` | `row08_fold` |
| `0x114E04` | `fifa96_entity_sine` |
| `0x8DCD4`/`0x8DC68` | the row-04 metric / `fifa96_entity_distance` |
| `0x7DAB4` | `out.reset` -> `match_row_reset` |
| `0x6E598` | `out.anim` (OL-52) |
| `0x8ED40`/`0x8F188`/`0x92820`/`0x7A490`/`0x974F0`/`0x651F0` | `out.events`/`event_code`/`event_sound`/`ball_stage*` requests (OL-62/OL-82) |
| stage/timer/process writes | `out.stage92`/`timer89`/`byte_15877d*`/`byte_15872f*` |

### K.6.6 Tests

`tests/test_outfield.c` fixtures (hand-computed from K.6.3): the prologue
phase reset and the stage >= 3 prologue add; the inactive-arm team clears (all
four combinations); the stage-0 camera+lead copy, the face octant, the zero
direction guard, the +0x9E latch and the single stage advance; the no-slot
receiver gate (lane/height boundaries, the timer 0x3C/0x3D split); the
stage-1 gates ([0x15877D], +0x44, +0x3D); the projection scan record arm
(offset-0 miss then offset-0x10 hit, metric and 6-byte staging vector, seed-0
sound 0x190), the scan miss (stage-0 vs stage-1 timer89), the user arm (row
byte 0x4A and the 0x49 counter-case); the descriptor/radius clamp and the
[0x15872F] shift; the stage-2 snap and the +0x44 reset/[0x15872F] increment.
`tests/test_engine_match_handlers.c::test_action_04_and_08_wired` runs the
wired row over the pool (reset, stage-0 camera/face, the +0x3D scan, the
stage-2 snap) and `action_expect[0x08]` is `FIFA96_OK`.

### K.6.7 Open legs (numbered)

* **OL-82 — row-08 unmodeled inputs/sinks.** The pool models neither the
  record bytes `+0x3D` (staged as `record.frame`, producer OL-80), `+0x44`,
  `+0x99` nor `+0x7D` (the 0x79C50 face write has no pool field), the
  `rec[+4]` descriptor bytes `+0xC`/`+0x16` and the opponent target's
  `+0xC`/`+0xF`, the `byte[[user+0x28]]` row byte, the process bytes
  `[0x15877D]`/`[0x15872F]` (their writes have no derived home), the
  `0x1577C0/C2` lead words nor the `0x10F334`/`0x10F33C` type tables beyond
  their 32 bytes. The engine stages zero / camera stand-ins (K.6.5) and the
  affected branches (`+0x44` reset, the user-0x4A arm, the descriptor radius,
  the [0x15872F]/[0x15877D] carry, the lead add) stay inert until the
  producers land; the +0x3D gate keeps the projection scan inert live until
  the OL-80 animation frame is staged (Task 5). The loader fixtures pin the
  ported behavior. `[0x1577CA]`/`[0x157A83]` are pool stand-ins. The
  `0x79B58` `+0x99` gate is asymmetric between the two outfield binders —
  row 08's binder applies `timer93 = 0x10` unconditionally while row 04's
  checks its staged `s.byte99` — but both are equivalent against the
  zero-staged native byte until a `+0x99` producer lands. The `+0x8E`
  `record.type`/`record.actor_type` field split is **OL-83** (§6).
* **OL-52 (carried) — the `0x6E598` anim request** stays unconsumed.
* **OL-62 (carried) — the `0x7A490` staging tail** (`0x7A4F0..0x7AE2F`) stays
  unported; the row-08 call is the core request.
* **OL-67 (carried) — the event/audio sinks** remain requests.
* Invoke-now modeling: n/a (row 08 installs no code).

**Task-5 erratum (M2 playability-legs, OL-80):** the `+0x3D` producer is no
longer missing. The FU-141 pool record carries `frame` (native `+0x3D`,
staged by `FUN_00036C70` at `0x36D4F`; first-hand this slice), the frame
staging writes the derived FU-84 advance back into it, and
`fifa96_match_action_08` stages it into `record.frame` as before — so the
row-08 `+0x3D == 1` projection-scan gate is live from the staged pool value
and the K.6.7 "inert until the OL-80 producer lands" caveat applies only to
the *values* being zero until the caller/arm bodies set the pool field.
**Task-5 fix round 1:** the kickoff `FUN_00079B6C` tail (`0x79C13`,
first-hand `get_function_by_address 0x79BB5` -> `body_end 0x79C1C`) is a
second `+0x28`/`+0x3D` producer: `FUN_0006E598(rec, active ? 0 : 0x26, 0)`
gives inactive records row id 0x26 and resets the frame; it is ported in
`fifa96_match_entities_kickoff_place`, so the pool's `anim_id`/`frame` are
non-zero from match begin.

### K.6.8 Errata to prior maps

* FU-137 §6.1/FU-142 K.4's "213 defined-code insns" for the row-08 span is
  corrected by the first-hand linear sweep: `0x81068..0x814AF` decodes **329**
  instructions including the terminal RET (one `disassemble_bytes` call,
  `instructions_total` 329).
* The `0x114E04` sine fold result is stored as a **16-bit word** and the
  `0x795A4` callers `MOVSX` it; in particular `sine(0x100)` is `65536` and
  truncates to `0`, and `sine(0x80)=46340` sign-extends to `-19196`. The K.4
  "facing-projection" description is right; the port's fixtures pin the word
  semantics.
* `0x79C50`'s row-08 call passes `DX = word[+0x6B]`, `BX = word[+0x6D]` (not
  a pos->target difference); `fifa96_arm_face` models the row-27 caller-side
  difference, so row 08 folds inline through `fifa96_entity_angle`.

## Appendix L (M2 playability Task 4 / C3-OL2) — score-event writer first-hand window

Task 4 closes the carried leg C3-OL2. The native score writer `FUN_00093944`
(`0x93944..0x93B7D`) is derived instruction-by-instruction; the eleven call
sites censused in I.10 are re-verified first-hand and all resolve to the six
period-indexed goal-screen handlers, so the writer is ported as the derived
score-event source and wired into the run's match state
(`fifa96_action_score_event` / `fifa96_match_run_score_event`).
`fifa96_match_run_add_goal` stays the FU-72 plain increment for the gameplay
paths whose native invokers remain unported (OL-77/OL-87/OL-88).

### L.1 Tool calls (Ghidra read-only, explicit `/FIFA96.EXE`)

* `get_xrefs_to`: `0x93944` (11 `UNCONDITIONAL_CALL`), `0x15B6A8` (25),
  `0x15B6C0` (32), `0x15B6D4` (7), `0x948AC` (2), `0x92E2C` (1), `0x92D8C` (2),
  `0x88940` (1), `0x93BBC`/`0x93E20`/`0x940A4`/`0x94270`/`0x944FC`/`0x946C4`
  (each one `DATA` ref, from the `0x110F78` table);
* `search_instructions` operand patterns `15b6b4` (16 hits), `15b6a4` (4),
  `15b670` (3);
* `disassemble_function 0x93944` (168 instructions);
* `decompile_function`: `0x93944`, `0x92D8C`, `0x92E2C`;
* `disassemble_bytes`: `0x93B40` (200 B), `0x93C00` (768 B), `0x93F00`
  (848 B), `0x94230` (210 B), `0x94300` (1000 B), `0x946E7` (600 B),
  `0x4B180` (64 B), `0x8B5E0` (120 B), `0x9252C` (32 B), `0x92D8C` (156 B),
  `0x92E2C` (212 B);
* `read_memory`: `0x110F78` (24 B), `0x93B80` (72 B), `0x93DE4` (24 B),
  `0x93DFC` (36 B), `0x94074` (116 B), `0x94234` (88 B), `0x944D4` (56 B),
  `0x946B4` (24 B), `0x1106E0` (180 B), `0x110794` (140 B);
* no renames, comments, labels, functions, scripts or project saves.

### L.2 The writer `FUN_00093944` (`0x93944..0x93B7D`)

Register argument EAX = side (0/1). Exact body (sites first-hand; the
decompiler confirms the disassembly walk):

```
0x9394B  INC word [EAX*2 + 0x157AC5]        ; score[side]++ (16-bit wrap)
0x93953  EDX = [0x15B6B4]                   ; tracked side
0x93959  [0x15B670] = EAX                   ; last scoring side
0x9395E  if (EDX == -1) return              ; 0x93961 JZ epilogue
0x93967  other = (EDX == 0) ? 1 : 0         ; 0x93969 SETZ / AND EAX,0xFF
0x93971  AX = score[other]; BX = score[EDX]
0x93983  EDX = AX - BX                      ; 32-bit, then truncated:
0x9398B  EAX = MOVSX DX                     ; (int16) difference
0x9398E  if (EAX > [0x15B6A4]) [0x15B6A4] = EAX    ; max_diff
0x93997  if (side == [0x15B6B4]) goto 0x93AA6
```

Non-tracked arm (`side != tracked`, `0x9399D..0x93AA5`):

```
0x939B0  if (score[side] == 1 && score[side^1] < 3):
0x939D1    CALL 0xCBC4C; if (AL & 3) POST 0xD3 and return   ; 0x939DA/0x93B73
0x939E4  if (score[side] == 4 && score[side^1] < 2) POST 0x9E and return
0x93A22  if (score[side] == 7 && score[side^1] < 3) POST 0x9F and return
0x93A60  if (score[side] == 9 && score[side^1] < 4) POST 0xA0 and return
         return
```

Tracked arm (`side == tracked`, `0x93AA6..0x93B77`):

```
0x93AA6  EAX = MOVSX DX + 3; EBP = [0x15B6A4]
0x93AB2  if (EAX == EBP && EBP > 3) POST 0x9A and return
0x93ACB  if (score[side] == 3 && score[side^1] == 0) POST 0x9B and return
0x93B02  if (score[side] == 5 && score[side^1] < 3) POST 0x9C and return
0x93B40  if (score[side] == 9 && score[side^1] < 5) POST 0x9D and return
         return
```

Both `other` reads are `side^1` on the arms that can fire (the first arm's
checks use `(side == 0)`, the same value). Every POST arm returns through the
shared epilogue (`0x93B78`), so at most one id is posted per call. The id goes
to `FUN_0009252C` (`0x9252C..0x92544`, first-hand: `EBX=id; if
(FUN_00066E70()==0) FUN_00066724(id, 0)`), which FU-63 §5 listed as an
uncalled gap helper — **corrected**: it has the writer's call sites (the goal
cluster is unanalyzed, so Ghidra's xref list is empty).

Erratum to FU-72 §2.4: the summary omits the **0xD3** post (the
`FUN_000CBC4C`-gated arm above) and the `-1` early return's exact position
(the increment and `[0x5B670]` write happen first). The threshold pair
"3/4/5/7/9" is the tracked arm's 0x9A/0x9B/0x9C/0x9D; 0x9E/0x9F/0xA0/0xD3 are
the non-tracked arm.

### L.3 The writer's state cells and their producers

| cell | role | writer | first-hand sites |
|---|---|---|---|
| `0x157AC5`/`0x157AC7` | per-side goal words | `FUN_00092E2C` | zeroed `0x92E7B`/`0x92E82` |
| `0x15B670` | last scoring side | `FUN_00093944` | `0x93959` |
| `0x15B6B4` | tracked goal-difference side (-1 sentinel) | `FUN_00092D8C` | `0x92DDF` (param when `[0x15B684]!=0`), `0x92DEF` (=1 when `[0x1590CC]==0`), `0x92E00` (=0 when `[0x159901]==0`), `0x92E08` (=-1) |
| `0x15B6A4` | max tracked-side goal difference | `FUN_00092E2C` / `FUN_00093944` | `0x92E5B` (=0), `0x93992` |

`FUN_00092D8C` is called only from `FUN_0003BB1C`/`FUN_00038630` (the
replay/screen setup cluster), not from the match init; the team flags
`0x1590CC`/`0x159901` are the FU-72 §2.4 `team+0x828` pair whose producers stay
unported. `FUN_00092E2C` additionally zeroes `[0x157AB4/AB6/AC9/AAF/AAE/AB1]`,
`[0x15B68C/690/698/69C]`, calls `FUN_000700F4` and installs the goal-screen
handler from the period-indexed table (L.4).

### L.4 The eleven call sites

`get_xrefs_to 0x93944` returns exactly eleven `UNCONDITIONAL_CALL` references:
`0x93D98`, `0x93DA1`, `0x94026`, `0x9402F`, `0x941E5`, `0x941EE`, `0x94489`,
`0x94492`, `0x94667`, `0x94670`, `0x9486E` — all inside the goal-screen
handler family, **not** the action cluster (I.10's census confirmed; the action
table `0x1106E0[45]` and phase table `0x110794[35]` re-read this slice contain
no `0x93xxx` slot).

The six handlers are selected by the **game leg** dword `[0x15B680]`:
`FUN_00092E2C` `0x92EC4..0x92EF7` reads `(int16)[0x15B680]`, indexes the table
at flat `0x110F78` and installs the result into `[0x15B6D4]`. `[0x15B680]`'s
only writers are `FUN_00092D8C 0x92DC5` (its argument) and `FUN_0001B7B8
0x1B7DB`; its identity with the FU-62 period byte `[0x157AC2]` is not asserted
here (the consumers compare it to 2/4/5 and index it by period-shaped values):

| table slot | handler | period |
|---|---|---|
| `0x110F78` | `0x93BBC` | 0 |
| `0x110F7C` | `0x93E20` | 1 |
| `0x110F80` | `0x940A4` | 2 |
| `0x110F84` | `0x94270` | 3 |
| `0x110F88` | `0x944FC` | 4 |
| `0x110F8C` | `0x946C4` | 5 |

Each handler runs a `[0x15B6B0]`-indexed state sequence and dispatches the
pending situation id `[0x15B6A8] - 1` (range 0..8) through an inline table; the
matching arm sets EAX and calls the writer. Side sources and id tables
(first-hand bytes):

| site | handler | state jump table | id table | id -> side |
|---|---|---|---|---|
| `0x93D98` | `0x93BBC` | `0x93B80` (6) | `0x93B98` (9) | 1/3/9 -> 1 (`0x93D93 MOV EAX,1`) |
| `0x93DA1` | `0x93BBC` | — | `0x93B98` | 2/5 -> 0 (`0x93D9F XOR EAX,EAX`) |
| `0x94026` | `0x93E20` | `0x93DE4` (6) | `0x93DFC` (9) | 1/3/9 -> 1 (`0x94021`) |
| `0x9402F` | `0x93E20` | — | `0x93DFC` | 2/5 -> 0 (`0x9402D`) |
| `0x941E5` | `0x940A4` | `0x94074` (5) | `0x94088` (7) | 1/3/6 -> 1 (`0x941E0`) |
| `0x941EE` | `0x940A4` | — | `0x94088` | 2/4/5/7 -> 0 (`0x941EC`) |
| `0x94489` | `0x94270` | `0x94234` (6) | `0x9424C` (9) | 1/3/6/9 -> 1 (`0x94484`) |
| `0x94492` | `0x94270` | — | `0x9424C` | 2/5 -> 0 (`0x94490`) |
| `0x94667` | `0x944FC` | `0x944D4` (4) | `0x944E4` (6) | 1/3/6 -> 1 (`0x94662`) |
| `0x94670` | `0x944FC` | — | `0x944E4` | 2/5 -> 0 (`0x9466E`) |
| `0x9486E` | `0x946C4` | `0x946B4` (4) | none | `[0x15B6A8]==5` -> 0 (`0x9484A..58`; the `[0x15B6B4]==0` test at `0x9484A` gates only the `0x94853` `[0x15B68C]` store); else -> 1 (`0x94869 MOV EAX,1`; the `[0x15B6B4]` test at `0x9485C..64` gates only the `[0x15B68C]` store) |

**L.4 erratum (fix round 1).** The period-4 row above is wrong for id 3.
First-hand bytes: the inline table `0x944E4` =
`[0x94662, 0x9466E, 0x94677, 0x94677, 0x9466E, 0x94662]`; the dispatcher is
`0x94606 MOV ESI,1` / `0x94648 EAX=[0x15B6A8]` / `0x9464D SUB EAX,ESI` /
`0x94655 CMP EAX,5` / `0x94658 JA 0x94677`, so the table indexes `id-1` and
`0x94677` is `INC dword [0x15B6A0]`. The handler `0x944FC` map is therefore
**1/6 -> 1, 2/5 -> 0, 3/4 -> no-score counter** (not 1/3/6). The other five
id tables were re-read this round and stand.

The complete per-leg id→side/no-score map (including the queued id 6: legs 0/1
map it to the no-score counter, legs 2..5 score it side 1) is FU-146 §4.

Ids outside a table's range (including the queued 0/0xA) jump to the
`INC [0x15B6A0]` no-score counter (`0x93DA8`/`0x94036`/`0x941F5`/`0x94499`/
`0x94677`). Every handler also checks `[0x157A4A]>>24 == 2` (the in-play
phase) before its state machine proceeds, and after the score sequence runs
`FUN_000740A0(0,0)` (phase 0) at `0x93DB2`/`0x94040`/`0x941FF`/`0x944A3`/
`0x94681`/`0x94877` (FU-143 §3.1's six sites).

### L.5 The invoker path (all unported)

The writer is reached only from this screen cluster:

* **Goal scan** — the clock `FUN_0008AF38` (`0x8AF38`) calls `FUN_00088940`
  at `0x8B63E` when the phase byte is 2/0x10 (`0x8B623..0x8B633`) and
  `[0x15781D] != 0` (`0x8B635`). `FUN_00088940` classifies the **camera pan
  snapshot** (not the ball) — `[0x157784]` = camera Z, `[0x15781E]` =
  `FUN_00070074(camera pos)` — and queues the goal situation 6 for the
  possession record's side (L.9 erratum; FU-143 §3.2 callers).
* **Situation queue** — `FUN_0008A938` queues `[0x15B6A8]` (ids 0..0xA) with
  `[0x15B6C0]=1` (FU-143 table 1); the frame body `FUN_0004B100`
  (`0x4B198 CMP byte[0x14C32A],0` / `0x4B1A1 CALL 0x948AC`) calls the
  scheduler `FUN_000948AC` only when the `[0x14C32A]` gate is open.
* **Scheduler** — `FUN_000948AC` (`0x948AC..0x949F7`) queues ids 3/4/7/8/9 by
  period/controlled-side/timer (`0x948BF` (period!=5 ? 8 : 7), `0x948EE..0x94919`
  id 3, `0x9492a..0x94978` id 4, `0x94996..0x949b5` id 9, `0x949ca..0x949da`
  id 7) and calls the installed handler `CALL dword [0x15B6D4]` (`0x949E9`).
* **State advance** — each handler's tail (`CALL 0x935A0` when the `[0x15B688]`
  timer passes 0xB4, e.g. `0x93DD6`) advances/reinstalls the sequence.

Gate defaults in the engine: `[0x14C32A]` and `[0x15781D]` have no **engine**
producer/field (an EXE image starts both at 0), so the ported frame body keeps
the native skip behavior (no scheduler/scanner call wired). Natively the gate
is live: `[0x14C32A]` is set to 1 by the session-mode entry `FUN_0001B7B8`
(`0x1B7C8`, mode 4) and the setup entry `FUN_00032DE0` (`0x32DFF`), and
`[0x15781D]` is armed
by the camera-pan arm `FUN_0007131C` (`0x713DB`) and cleared by the phase
setter on a phase-2 write (`0x740F6`) and by the restart body `0x84F90`
(L.9).

### L.6 Wiring decision and port (M2 Task 4)

* **Derived writer** — `fifa96_action_score_event` (`fifa96_action_handlers.c`)
  carries the four cells, the `probe` input (the `FUN_000CBC4C` byte) and the
  post id; every branch is fixtured in `tests/test_action_handlers.c`
  (`test_score_event_untracked`, `_tracked_9a`, `_tracked_thresholds`,
  `_untracked_thresholds`, `_invalid`).
* **Engine source** — `fifa96_match_run_score_event` runs the writer over
  `mr->score[2]` + `score_last_side`/`score_tracked_side`/`score_max_diff` and
  records the post in `score_last_event` (0 = none). The carried defaults are
  `last_side = tracked_side = -1`, `max_diff = 0` (init/begin/teardown); the
  `-1` tracked default reduces the source to the FU-72 increment + last-side
  record. Tests: `test_engine_match_frame::test_score_event_wired_run_path`.
* **Tape** — the M2-B goal step now calls `fifa96_match_run_score_event(mr, 0,
  0)` (was `add_goal`). With the carried default the transcript is
  **byte-identical** (`tests/golden/engine/m2-frames.txt` unchanged, no frame
  moved); `add_goal` stays for the unported gameplay paths.
* **No wired dispatch reaches a site** — I.10's conclusion is re-confirmed
  first-hand (L.4): the action/phase tables hold no `0x93xxx` slot; every
  site's invoker chain is unported. The plan's "goal reached through a wired
  dispatch" is therefore carried as the leg below, and the score source is
  wired at the run API.

### L.7 Open legs (extend FU-143 §8 / FU-142 K.6.7)

* **OL-87 — goal-screen handler cluster (the writer's invokers).** The six
  period-indexed handlers (`0x110F78`), their `[0x15B6B0]` state machines,
  timers and pending-id consumption; the tracked-side producer `FUN_00092D8C`
  (`[0x1590CC]`/`[0x159901]` team flags); the scheduler `FUN_000948AC` and the
  `[0x14C32A]` frame gate. Consequence: the engine carries tracked side -1
  (`add_goal`-equivalent) until this lands.
* **OL-88 — goal detection.** `FUN_0008AF38 0x8B623..0x8B63E` (phase 2/0x10 +
  `[0x15781D]`) -> `FUN_00088940` -> `FUN_0008A938(6, side)`: the gameplay goal
  producer is unported, so no gameplay goal reaches the derived source yet.
* **OL-89 — writer side effects.** The posted ids are captured
  (`score_last_event`) but not dispatched: `FUN_0009252C` ->
  `FUN_00066E70`/`FUN_00066724` and the `FUN_000CBC4C` probe result are
  unported; the loader takes the probe as an input and the engine passes 0.

### L.8 Tests and gate

`tests/test_action_handlers.c` writer fixtures (every arm: -1 sentinel, word
wrap, 0x9A with the `max_diff > 3` guard, 0x9B/0x9C/0x9D, 0xD3 with the probe
short-circuit, 0x9E/0x9F/0xA0, signed max-diff, side-1 mirrors, invalid
args); `tests/test_engine_match_frame.c::test_score_event_wired_run_path`
(live run, defaults, tracked-side bookkeeping, posts, live frame, errors);
`tests/test_engine_m2.c` goal step upgraded, golden byte-identical. `make
check` **104/104** (no new test executables).

### L.9 Reachability window (M2 playable-match Task 3 / OL-87/88/89)

Task 3 asked which `FUN_00093944` invoker is reachable from the ported rows and
run state. First-hand verdict (this slice, `/FIFA96.EXE` read-only): **none**.
The writer stays wired only at the run API (`fifa96_match_run_score_event`,
L.6) and no ported gameplay path reaches it. Windows:

* **All eleven writer sites re-verified.** `get_xrefs_to 0x93944` = 11
  `UNCONDITIONAL_CALL`, exactly `0x93D98`, `0x93DA1`, `0x94026`, `0x9402F`,
  `0x941E5`, `0x941EE`, `0x94489`, `0x94492`, `0x94667`, `0x94670`, `0x9486E`,
  all inside the six handlers of the `0x110F78` table (L.4). No action/phase
  row body contains one.
* **Handler installation is screen-driven.** `FUN_00092E2C` is called only
  from `FUN_00092D8C 0x92E1F`; `FUN_00092E2C 0x92EC4..0x92EF7` reads game leg
  word `[0x15B680]` (`0x92EC4 MOV AX,[0x15B680]` / `CWDE`), indexes
  `0x110F78[leg]` (`0x92ED5 ADD EAX,0x110F78`) and stores `[0x15B6D4]`
  (`0x92EEE`), then calls it (`0x92EF7`). `FUN_00092D8C` writes the game leg
  (`0x92DC5 MOV [0x15B680],EAX`) and the tracked side
  (`0x92DDF`/`0x92DEF`/`0x92E00`/`0x92E08`); its only callers are the
  front-end screen machine `FUN_00038630` case 0xF (`0x38DCC`, the
  match-screen entry: `FUN_00092D8C(short[0x14AF7C], short[0x14AF74], ...)`,
  `[0x14B018]=0x14`) and `FUN_0003BB1C 0x3BF9E`. The tracked-side team flags
  `[0x1590CC]`/`[0x159901]` stay unported.
* **The only goal producer is camera-driven.** `get_xrefs_to 0x8A938` = 39;
  the situation argument was read at every site (census below). Exactly one
  queues situation 6: `0x88B44` in `FUN_00088940` (`0x88B37 MOV EAX,6` /
  `0x88B42 XOR EBX,EBX` / `0x88B44 CALL`; side = the selected possession
  record's `[+0x826]`), reached only from `FUN_0008AF38 0x8B63E` when the
  phase is 2/0x10 (`0x8B623..0x8B633`) and `[0x15781D] != 0` (`0x8B635`).
  `FUN_00088940` classifies the camera pan snapshot: `[0x157784]` magnitude vs
  `0xB20` (`0x8896B MOV ECX,[0x157784]`, `0x88971..0x88983`; `JLE` to the
  throw-in path), `[0x15781E]` (`0x88989 CMP byte[0x15781E],0` / `0x88990 JZ`
  to the corner path), side from `[0x157A49]>>24 == 1` -> the `0x1587D4`
  record's `[+0x826]` (`0x88996..0x889AC`) else the snapshot sign
  (`0x889B6 CMP [0x157784],0` / `SETL`), call at `0x88B44`. **L.5 erratum**
  (recorded in place): the scan position is the camera, not the ball.
  `[0x15781D]`/`[0x15781E]` are armed by the camera-pan arm `FUN_0007131C`:
  `0x713A6..0x713C6` (phase 2/0x10, `[0x15781D]==0`, `|camZ| > 0xB20 ||
  |camX| > 0x730`), `0x713DB MOV [0x15781D],BH` (BH=1 at `0x713C6`), camera
  snapshot copy `0x713E3..0x713E5`, `0x713F2 CALL 0x70074` (`FUN_00070074`
  goal-mouth classifier) / `0x713F7 MOV [0x15781E],AL`; `[0x15781D]` is
  cleared by the phase setter on every phase-2 write (`0x740F2 XOR DL,DL` /
  `0x740F6 MOV [0x15781D],DL`) and by the restart body `0x84F90`
  (`0x84F90 MOV [0x15781D],AH`, AH=0). The FU-71 port exposes only
  `fifa96_camera_reflect`/`fifa96_camera_out_of_bounds`; the pan arming and
  the clock scan call are unported and the run's camera never leaves spawn.
* **Act 8 is not a goal path (FU-143 §10 lead resolved).** `0x8A8CE` lies in
  `FUN_0008A798`, the phase-table `0x110794[0x1E]` handler (act 8, invoked via
  `FUN_000888FC` at `0x8AF1A` from the situation-dispatcher fallback
  `0x8AAA3 JMP 0x8AF1A`). The fallback stores the situation/side first:
  `0x8AA89 CALL 0x740A0` (phase 0), `0x8AA93 MOV [0x15882C],AL` (side),
  `0x8AA98 MOV EAX,8`, `0x8AA9D MOV byte [0x15882B],CL` (situation). Act 8's
  `0x8A8BD/0x8A8C3` dword loads shift to exactly those bytes
  (`0x8A8C8 SAR EDX,0x18` -> `[0x15882C]`; `0x8A8CB SAR EAX,0x18` ->
  `[0x15882B]`), so `0x8A8CE` re-dispatches the stored situation and then
  writes `0x8A8D3 MOV byte [0x15882B],0xFF`. Goal situations queue through
  table 1 with `BX=0` (`0x8A9E8`: `[0x15B6A8]=5` when the side word `SI==0`,
  else `6`; `0x8A9F7 [0x15B6C0]=1`) and never take this path.
* **Goal consumption is the scheduler + handlers.** The frame body
  `FUN_0004B100 0x4B198 CMP byte[0x14C32A],0 / 0x4B19F JZ 0x4B1A6 /
  0x4B1A1 CALL 0x948AC` calls the scheduler only when the session gate is
  open; the scheduler tail invokes the installed handler
  (`0x949E0 CMP [0x15B6D4],0` / `0x949E7 JZ` / `0x949E9 CALL [0x15B6D4]`).
  Neither call site nor machine is ported.

Census of the 39 `FUN_0008A938` call sites (first-hand windows; situation in
EAX, side in EDX unless noted). The only site that can request a goal is
`0x88B44`:

| situation | sites | containers |
|---|---|---|
| 0 | `0x4B0A5`, `0x38B2E`, `0x38C95`, `0x742DE` | `FUN_0004B02C` setup, `FUN_00038630` screen machine, match setup/reset block (`0x74281..0x74318`) |
| 1 | `0x888F2`, `0x8B85D`, `0x945A5` | `FUN_00088860`, act 0xA stage 2, handler `0x944FC` tail |
| 2 | `0x88C00` (BX=1), `0x85D0A` (BX=1), `0x943D7` | `FUN_00088940` throw-in, row 0x10 body, handler `0x94270` tail |
| 3\|4 | `0x88BBD` | `FUN_00088940` corner/keeper |
| 3 | `0x93CE4` | handler `0x93BBC` tail |
| 4 | `0x94120` | handler `0x940A4` tail |
| 5 | `0x76B1C`, `0x77707` | keeper bodies (row 1A/1B family) |
| 6 | **`0x88B44`** | **`FUN_00088940` goal arm — the only goal producer** |
| 7 | `0x76A90`, `0x77589` | keeper bodies (row 1A/1B family) |
| 8 | `0x4B0D9`, `0x888C8`, `0x74312` | `FUN_0004B02C` bit-1 arm, `FUN_00088860`, setup/reset block |
| 9 | `0x8A729`, `0x8920A` | `FUN_0008A43C` event body (`0x8A43C..0x8A794`, called from `FUN_00079D5C 0x79F2B` and `0x81EBF`), phase-0x18 body ending `0x89213` |
| 0xA | `0x8A3F0`, `0x93F75`, `0x947AB` | act-2 stage body (`0x8A351..0x8A3FA`), handler `0x93E20`/`0x946C4` tails |
| 0xB | `0x7DF90` (row 01, ported), `0x7546E` (row 1D tail), `0x75B58`, `0x76072` (keeper bodies), `0x84495` (row 0x12), `0x84E8F` (row 0x13), `0x85D38` (row 0x10), `0x863F9` (row 0x11) | kickoff/restart producers |
| 0xC | `0x93C41`, `0x93EC1`, `0x94332` | handler tails |
| computed | `0x897E3`, `0x8A8CE` | word `[0x15881E]` staged by the dispatcher's act-1/act-2 arms (FU-143 §10 item 3), and the act-8 stored `[0x15882B]` (above); neither is a goal path |

The `0x9...` sites are the handlers re-queueing the next situation in their
state tails (the same handler family as the writer's callers).

**Verdict and engine landing (Task 3).** No invoker of `FUN_00093944` is
reachable from the ported rows or run state: the producer (camera-pan arming +
clock scan + `FUN_00088940`), the queue (`[0x15B6A8]`/`[0x15B6C0]`) and the
consumer (scheduler `FUN_000948AC`, installer `FUN_00092D8C`/`FUN_00092E2C`,
the six handlers) are all unported, and the engine's camera is static. The
writer stays wired only as `fifa96_match_run_score_event` (L.6); the M2 tape's
goal step stays the direct call. Task 3 added no engine behavior; the negative
result is pinned by
`tests/test_engine_match_frame.c::test_goal_situation_dispatch_is_not_the_writer`
and `::test_natural_phase2_never_scores`, the row-01 score-freshness
assertions in `tests/test_engine_match_handlers.c`, and the M2 tape's
pre-score freshness assertions (`tests/test_engine_m2.c`). The engine's
`fifa96_match_run_situation` intentionally models only the table-2 immediate
arm (header contract; the table-1 queue is OL-73), so a situation-6 call
through that seam takes the table-2 phase-5 row rather than the native queue
(`0x8A9E8`); no ported caller passes 6. OL-87/OL-88 (L.7) stay open with the
four machinery blocks above as their exact requirement.
