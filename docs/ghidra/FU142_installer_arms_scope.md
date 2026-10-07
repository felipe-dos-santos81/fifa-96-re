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
| OL-30 row 06 pursuit | row 06 `0x801B4..0x81067` (597 insns, FU-139 §2) | target construction, `0x114E04` folds, RNG gates, installs 8/9/4 | 1–2 |
| OL-31 rows 07/0F kick machines | `FUN_0007E600` `0x7E600..0x7E7C4` (452 B); row 07 `0x814B0`, row 0F `0x82AD0..0x82DD0` (FU-139 §2) | decision, opponent 0x22 invoke, ball-actor install 4, fun-0F predictor/RNG/timer reload, `FUN_0007DAB4` tail | 2 |
| OL-32 reception/tackle/duel arms | `FUN_0007A084` `0x7A084..0x7A456` (978 B); NSEARCH `FUN_0008DB6C` `0x8DB6C..0x8DC49` (221 B); SWAP `FUN_000786A0` `0x786A0..0x786EB` (75 B); `FUN_0004C324` `0x4C324..0x4C372` (78 B); row 18/21/23 arms (FU-139 §6) | special-class/RNG arms, sound arms, NSEARCH/SWAP, row-21 claim, row-23 target, row-18 resolution | 1–2 |
| OL-38 outfield decide/chase wiring | rows 04 `0x7E7C8` / 08 `0x81068..0x81188` (FU-138/FU-141 §7); `FUN_0007DAB4` `0x7DAB4..0x7DB0C` (88 B); `FUN_00079B58` (16 B); `FUN_00079C50` `0x79C50..0x79C98` (72 B); per-type gate `0x110680`; input tables `0x1109D0`/`0x1109E4` (FU-137 §4.1, FU-141 §7) | input-row dispatch, no-edge arm, forced decision, chase gate; then rows 04/08 wire | 1–2 |
| OL-41 interception tail | `FUN_0008D824` `0x8D824..0x8D8EB` (199 B) + `FUN_000795B4` `0x795B4..0x795F0` (60 B) | bind call; distance band feeding `team+0x7BE` | 1 |

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
  0x13/0x14 subset (`0x8D693..0x8D820`), `FUN_0008CEB8` and the 0x2A scan are
  ported and tested (`fifa96_match_phase_machine_step`, FU-142 Appendix B);
  the remaining states of the 1928-B function (0..0x15 switch arms, the
  pre-switch record walk `0x8D0A6..0x8D166`, the entry RNG block
  `0x8D693..0x8D727`) stay unported.**
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
* `read_memory 0x8CEDB` (32 B) — raw clamp/guard bytes;
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
* **last clamp** `0x8CEDB..0x8CEE7`: `last >= 0xB` becomes `0xB`; raw bytes
  `83 FE 0B 7C 07 66 C7 44 24 04 0B 00`.
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
* `disassemble_bytes 0x84598` (152 B) — 49 instructions to the `RET` at
  `0x8462D` plus the `MOV EAX,EAX` pad at `0x8462E` (the next action row 0x16
  starts at `0x84630`);
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
9. `0x782bf` (arm `0x77EAC`): linear `0x782b6 MOV EDX,0x1c`; bypass `0x78200
   JMP 0x782bb` reached after `0x781fb MOV EDX,0x4`; the `0x77E98` table arms
   stage 0x1A/0x1B — code {4,0x1a,0x1b,0x1c}.
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
case (#4). The remaining 72 calls have an immediately preceding constant write
with no bypassing reference.

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

## 8. No-write statement

No C source, header, test, CMake, asset, ISO or Ghidra state was changed:
Task 9 stops at the split gate. Row classification and dispatch results are
exactly the BASE state: **2 × `FIFA96_OK` (actions 00, 1E), 77 ×
`-FIFA96_ERR_UNSUPPORTED` (73 not-ported + 4 open legs), 1 ×
`-FIFA96_ERR_NOT_FOUND` (phase 0x16)**; `tests/test_engine_match_handlers.c`
expectations unchanged; `make check` **100/100**; M1 golden and pinned render
hashes unchanged.

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
