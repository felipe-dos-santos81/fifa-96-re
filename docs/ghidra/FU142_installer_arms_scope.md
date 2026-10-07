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
  OL-52/OL-53. Rows 28/29/2A/2C stay unported.**
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
* Carried: FU-139 OL-26..OL-32, FU-141 OL-38/OL-41, FU-137 OL-15 (closes
  nothing new; the 0x26/0x28/0x2A arms stay as classified until their bodies
  land).
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

## 7. Refinements to FU-137 (to be recorded as errata in the port slices)

* **§6.1 row `2C`** — "prologue-only body" is wrong: `0x84598..0x8462D` is a
  real 48-instruction stage machine (stage latch `+0x92` 0/1/2; calls
  `0x6E598`, `0x7DAB4`). Its *entry* is still unresolved (OL-48).
* **§6.1 row `2B`** — the one-byte RET is the final instruction of the row-29
  body (shared epilogue at `0x87738`), not a standalone stub; the two helpers
  that follow (`0x8773C`, `0x8776C`) belong to no action row.
* **§6.1 row `29`** — the body exists and self-installs code 3 via
  `FUN_0007D9A4` (`0x8753C`); "no match-code reference" remains true only for
  an *installer of* 0x29, so the row's open-leg status is unchanged.
* **§5.2 arm `0x28` condition** — the arm fires when `[0x157A4A]>>24 != 0x13`
  in the state 0x13/0x14 block (i.e. the 0x14 half); when phase is 0x13 the
  0x25/3 arms run instead (first-hand `0x8D75F..0x8D7CF`).
* First-hand instruction counts for the seven row windows are in §1; FU-137's
  "body unanalyzed/unported" cells can cite §1/§3 of this doc when they are
  updated.
* **§2 rows 28/2A `0x36200` "stub"** — first-hand it is
  `MOV [0x105FC4],EAX; RET` (5-byte instruction + RET), not a bare no-op; the
  store gates the unported camera/coordinate step `FUN_00036208`
  (`0x3621E CMP dword [0x105FC4],1; SETZ/JZ return`; the `0x36211` gate above
  tests `[0x105FB4] < 0`, FU-62 §3.3). Task 3 ports the
  derived surface as a documented no-op (OL-51) because neither row 26 nor the
  engine models `0x105FC4`; the callers' appendices (Task 7/8) pin the value.
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
