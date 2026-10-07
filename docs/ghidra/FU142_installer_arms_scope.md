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
  `team+0x7A6`) is recorded as a porting hazard.
* **OL-47 — cluster-G row bodies.** Rows 26/27/28/29/2A/2C bodies are bounded
  (six bodies, ~1220 insns) but unported; each waits for its FU-142b..e task
  and for the unported helpers `0x8DCD4`, `0x79C50`, `0x6E598`, `0x513EC`,
  `0x7DAB4`, `0x6E1D0` (pure part covered by `fifa96_action_phase_cell`).
* **OL-48 — dynamic entry for 27/29/2C; 2B dead verdict.** No static install
  arm exists (FU-137 §5.3 re-cited); a runtime/reachability pass is the
  precondition for scheduling those three rows. 0x2B is confirmed a shared-RET
  dead entry (§1.1) pending that pass's ratification.
* Carried: FU-139 OL-26..OL-32, FU-141 OL-38/OL-41, FU-137 OL-15 (closes
  nothing new; the 0x26/0x28/0x2A arms stay as classified until their bodies
  land).

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
