# FU-138: M2 action cluster A — locomotion/move/control rows (codes 0x00–0x13)

Task 5 of the M2 child plan (`docs/superpowers/plans/2026-10-07-fifa96-m2-match.md`).
The spec's cluster A is "Locomotion / move / control action codes (action
`00`-family, `0x0A`–`0x13`)" (design §3.3); this slice re-verifies those action
rows read-only in the authoritative program, derives their record-visible
bodies, ports the pure parts into `fifa96_action_handlers`, and wires the first
fully covered row through the FU-137 dispatch seam with a minimal engine
record.

Result in one line: **all 20 cluster-A rows re-verify against the
`0x1106E0` table dump; row `00` is now ported and wired
(`fifa96_match_action_00` runs the FU-76 §3.1 locomotion step/target over
`mr->record` and records the `3`/`0x19` install request); rows `01`/`02`/`03`/
`0D` gain six derived, tested pure helpers (`stage_wait`,
`sequence_marker_target`, `locomotion_restart_wait`,
`locomotion_placement_counter`, `locomotion_phase1_clamp`,
`sequence_velocity_scale`) but stay `UNSUPPORTED` because their arm/support
bodies are unported; rows `04`–`13` are documented per row with handler
address, gate/timer/stage/install facts and a numbered open leg — no row is
guessed and no constant is invented.**

## Method

* Authoritative program **`/FIFA96.EXE`** (explicit in every call). Ghidra
  **read-only**: no renames, comments, labels, function creation, scripts or
  project saves. `/fifa96_le.bin` and the MZ loader were not used.
* Tool calls made this slice:
  * `read_memory /FIFA96.EXE 0x1106E0` (80 B) — the action table slice
    `0x00..0x13`, matching FU-136 §1.1/FU-137 §1.1 dword for dword;
  * `read_memory /FIFA96.EXE 0x7DBB0` (16 B) — row `01`'s 4-arm table;
  * `disassemble_bytes` — row `00` full body `0x7DB10..0x7DBB0`; row `01`
    head `0x7DBC0..0x7DC60` and arm 0 `0x7DC6B..0x7DCB0`; row `02` phase head
    `0x7DFE6..0x7E0B0` and arm 0 `0x7E0B0..0x7E1A4`; row `03` slot/counter
    `0x7E1D6..0x7E2A4` and phase-2/phase-1 blocks `0x7E2D3..0x7E418`; the
    shared mover head `0x7BF20..0x7BFD0`; row `0D` arm 0
    `0x82567..0x82620`; row `09` stage-0 target `0x80A87..0x80AE0`; and the
    entry windows of every remaining row `04..13` (48 B each).
* **Address mapping.** The native database resolves everything already: data
  immediates render at their flat addresses (`[0x00157a64]`, not `[0x57A64]`)
  and the action table holds runtime handler addresses, matching FU-137 §1.
  `FIFA96.EXE` defines no function bodies at these table targets, so every
  citation is `disassemble_bytes` output.
* Baseline at task start: `make check` = **98/98** (FU-137 commit `41160f2`).

## 1. Table re-verification (`0x1106E0[0x00..0x13]`)

`read_memory 0x1106E0` (80 B) returns the first 20 dwords:

```
0x00 07DB10 0x01 07DBC0 0x02 07DFCC 0x03 07E1A4 0x04 07E7C8
0x05 07F194 0x06 0801B4 0x07 0814B0 0x08 081068 0x09 080A00
0x0A 081738 0x0B 081908 0x0C 081C90 0x0D 08251C 0x0E 082710
0x0F 082AD0 0x10 0855F0 0x11 085DE4 0x12 083D68 0x13 084B00
```

These are exactly FU-136 §1.1/FU-137 §1.1's resolved targets for codes
`0x00..0x13`; the sole reader remains `FUN_0007D9A4 @ 0x7DA77` (FU-137 §2) and
the call contract remains `EAX = record` only. Row `01`'s arm table at
`0x7DBB0` reads `{0x7DC6B, 0x7DCAF, 0x7DD29, 0x7DFB2}` (stored values resolved,
FU-81 §2.1).

## 2. Cluster-A per-row derivation

"Verified" marks the windows read first-hand this slice; the body column is the
strongest of the fresh read and the named FU derivation. Field numbers are the
native record offsets.

| code | handler | gate / entry (verified) | timer | stage / arms | record-visible body | class / open leg |
|---|---|---|---|---|---|---|
| 00 | `07DB10` | none; phase-2 install gate | `+0x89 -= [0x157A64]` while `>0` | — | slot move target `FUN_00079C20(rec, slot+0x1D>>24, slot+0x1E>>24)` clamped by `FUN_0007D3E4`; install `3` when `+0x8D!=0` else `0x19` when phase 2, `+0x81==0`, `+0x89<=0` (full body FU-76 §3.1, re-read at `0x7DB10..0x7DBAC`) | **ported + wired (FU-138 §4)** |
| 01 | `07DBC0` | phase `==1` else tail `0x7DFB8` | `+0x89 += delta` (`0x7DC42`) | `0x7DBB0` 4 arms | marker `+0x8F>>24 >=2` copies `+0x59` triple; else the camera-reset x picks `x = (x<0)?-0x30:0x30`, `z=0` (`0x7DBDC..0x7DC39`); arm 0 waits `+0x89>=0x3C` then sound `0x974DC(0x1E)` + advance (`0x7DC80..0x7DCB4`) | partial: `sequence_marker_target`, `stage_wait` ported; celebration chain/nearest/ball stage/sound arms open (OL-17) |
| 02 | `07DFCC` | phase `1`/`2`, else reset `0x7E192` | phase-2 `+0x89 += delta` (`0x7E06B..0x7E081`) | inline `0..2` | phase 1: `target=( -[team+0x7B2].x, 0)`, `+0x89=0`, `+0x92=0` (`0x7DFEB..0x7E016`); phase 2 with slot: install `4` invoke-now (`0x7E04C..0x7E05A`); else target = ball `0x5774C` triple; arm 0 waits `0x78` when `dword[+0x69]>>16 > 0x40` (reset) else `0xA` (`0x7E0B0..0x7E0F0`) | partial: `locomotion_restart_target`, `locomotion_restart_wait` ported; nearest/vector/`0x92820`/ball stage arms open (OL-18) |
| 03 | `07E1A4` | phases `0x13/0x14` special; else slot/hold chain | — | inline | `0x13/0x14`: `target=(0x780,0,0)`, metric via `0x8DCD4`, `target.x=0xAE0` when metric `<0x30` (`0x7E1E0..0x7E20A`); slot arm writes `FUN_00079C20` dir step and `+0x7B=+0x79` (`0x7E25F..0x7E27C`); counter `+0x7B=min((+0x63>>22)+1,+0x79)` (`0x7E2E0..0x7E2F4`); phase-2 bounds `team+0x80C[+0x8D]` + opponent line `±0x60` (`0x7E309..0x7E399`); phase-1 side clamp `∓0x20` (`0x7E3CA..0x7E417`) | partial: `locomotion_hold`, `locomotion_clamp_placement`, `locomotion_placement_counter`, `locomotion_phase1_clamp` ported; `[rec+0x1C]`, `FUN_00079F3C`, phase-7 ball scan open (OL-19) |
| 04 | `07E7C8` | phase `==2` else reset (`0x7E7E2`); `+0x81!=0` return | — | inline | camera-lead arm `camera + vel<<2` (`0x7EB95`, FU-77 §2.4); wing vectors `0x57794`/`0x57788`; installs `4/0x19/0xF/0xB/7/6/5` | not ported: ranked decision/RNG/vectors (OL-20; `locomotion_camera_lead` already ported) |
| 05 | `07F194` | phase `==2` else reset (`0x7F1AE`) | `+0x89 += delta` while `<0x4B0` | jump table `+0x92` | carrier `0x58724` state, camera hold (FU-77 §2.5) | not ported: cluster B possession body (OL-21) |
| 06 | `0801B4` | phase `==2` else reset + clear `team+0x7B2/0x7B6` (`0x801D4`) | — | — | ball pursuit target via camera/own-position + `0x114E04` fold; installs `8/9/4` (FU-77 §2.6) | not ported: cluster B/D pursuit (OL-21) |
| 07 | `0814B0` | phase `==2` else tail `0x81702`; `+0x81!=0` tail | `+0x89 += delta` (duration meter) | inline `0..2` | stage 0 target from `[0x10F331/0x10F339+type8]<<4` or camera; stage 1 KICK `0x7B9C4` + opponent `0x22` invoke; tail installs `4` (FU-76 §3.2) | not ported: cluster B kick (OL-21) |
| 08 | `081068` | phase `==2` else reset (`0x8107E`) | — | — | `FUN_00079C50` face + event + `0x79B1C` tail (FU-75 chase body) | not ported: cluster D chase (OL-20) |
| 09 | `080A00` | phase `==2` else reset (`0x80A13`); stage `>3` tail | — | `0x809F0` 4 arms | stage 0: `x=[rec+0x6D]+[0x1577C0]*0xA`, `z=[rec+0x6F]+[0x1577C2]*0xA`, `0x79C50` face, angle fold `0x114E04` with `min(dword[+0x69]>>17, 0x30)` (`0x80A87..0x80ADB`) | not ported: arms/`0x14E04` fold (OL-22) |
| 0A | `081738` | phase `==2` else tail `0x818F5`; stages 0/1 | — | inline stage 0/1 | ball-relative run/placement: `NEAREST`, `ANGLE`, `FMUL`, `TMR93`, `RESET` (FU-76 §2) | not ported: body unported; installer `0x7CDD8` has no xrefs (OL-23) |
| 0B | `081908` | phase `==2` else `0x81C62`; stages 0..2 | — | inline `0..2` | duel event choice via `sequence_duel_event`; arm 1 installs `0x0C` on the nearest record (`0x81BEB`) (FU-82 §3.3) | not ported: nearest/ball/team globals (OL-22) |
| 0C | `081C90` | none | `+0x89 += delta` (`0x81CA3`) | `0x81C74` 7 arms | `+0x5D==0` copies `+0x59` triple; stage 0 ball-record target (FU-81 §2.1) | not ported: arms (OL-22) |
| 0D | `08251C` | none; clears `[0x157A83]` when it is `rec` (`0x82540`) | `+0x89 += delta` (`0x82532`) | `0x8250C` 4 arms | arm 0: `+0x83=0x20`, `0x702F8` → `+0x85`, animation `0x6E598`, velocity `= type[0x10F334/0x10F33C] × (both nonzero ? 3 : 4)` into `+0x73/+0x75`, distance `0x8DC68` (`0x82574..0x82619`); arm 1 offsets `[0x10F331/0x10F339]+6<<` | partial: `sequence_velocity_scale` ported; `0x702F8`/animation/arms 1-3 open (OL-22) |
| 0E | `082710` | phase `==2` else reset (`0x82726`) | `+0x89 += delta` (`0x8273A`) | `0x82700` 4 arms | stage 0: `0x71B9C(0x12,&+0x4D,&+0x65)`, `0x79C50`, then wait `+0x89>=0xC`, copy `0x57794` (FU-81 §2.1) | not ported: arms (OL-22) |
| 0F | `082AD0` | phase `==2` else reset (`0x82AE6`) | `+0x89 += delta` | — | second kick action `KICK 0x7B9C4` (FU-76 §2) | not ported: cluster B kick (OL-21) |
| 10 | `0855F0` | phases `2`/`3` else reset `0x85D8D` (`0x85603`) | `+0x89 += delta` (`0x8561F`) | `0x855B8` 7 arms + `0x855D4` event table | marker `+0x8F<3` leader / `<5` hold (`0x85630`); stages ball/`0x8DE8C` nearest then install `4` (`0x85CA1`) (FU-81 §2.1, FU-82 §3.7) | not ported: arms (OL-24) |
| 11 | `085DE4` | phases `2`/`4` else reset `0x864FF` (`0x85DF2`) | `+0x89 += delta` (`0x85E19`) | `0x85DA0` 10 arms + `0x85DC8` event table | same camera/ball `<3/<5` blocks; KICK `0x7B9C4` in an arm (FU-81 §2.1) | not ported: arms (OL-24) |
| 12 | `083D68` | phases `2`/`7` else `[0x157AB0]=0` + reset (`0x83D7E..0x83D93`) | `+0x89 += delta` (`0x83E03`) | `0x83D2C` 8 arms + `0x83D4C` 7 arms | stage 0 waits `+0x89>=0x3C` when `[0x4C32A]==0` (`0x83E26`); later arm installs `4` on the selected record (`0x844E2..0x844E7`) (FU-81 §2.1) | not ported: arms (OL-24) |
| 13 | `084B00` | phases `2`/`6` else reset `0x84EBE` (`0x84B13`) | `+0x89 += delta` (`0x84B8A`) | `0x84AE4` 7 arms | marker `<5` re-asserts `[0x57A83]`; stage 0 `0x73E08`; RESET arm installs `0x13` on `[team+0x7B2]` (`0x84DAB`) (FU-81 §2.1) | not ported: arms/globals (OL-24) |

Cross-family state shared by the table: `+0x89` action timer, `+0x92` stage
byte, `+0x91` current code, `+0x8D` active flag, `+0x9E` "ran" byte, `+0x44`
animation-row terminal bit (FU-82 §2.1), `[0x157A4A]>>24` phase, `[0x157A64]`
frame delta, `[0x157A83]` controlled-record pointer.

## 3. Ported pure functions

All six functions are caller-owned, table-free and negative-error on NULL, the
FU-76/77/81/82 port convention. Byte citations are first-hand this slice except
where the site was previously quoted by the named FU doc.

| function | native site | semantics |
|---|---|---|
| `fifa96_action_stage_wait(timer89, threshold, &ready)` | row 01 `0x7DC80`; row 0E `0x8276A`; row 12 `0x83E26` | `ready = timer89 >= threshold` — the shared timed-arm gate (FU-81 §2 chain step 4) |
| `fifa96_action_sequence_marker_target(marker, pos, lead_x, &out)` | row 01 `0x7DBDC..0x7DC39` | `marker = +0x8F>>24`; `>=2` copies the position triple (`MOVSD ×3` at `0x7DC31..0x7DC39`); else `out.x = lead_x < 0 ? -0x30 : 0x30` (`0x7DC06..0x7DC18`), `out.y = out.z = 0` |
| `fifa96_action_locomotion_restart_wait(lane, timer89, &ready, &reset)` | row 02 arm 0 `0x7E0B0..0x7E0F0` | `lane = dword[+0x69]>>16` (`0x7E0B0..0x7E0B6`); threshold `0x78` when `lane > 0x40` else `0xA` (`0x7E0BB`, `0x7E0D1`); `reset` marks the `lane > 0x40` ready path that calls `FUN_0007DAB4` (`0x7E0CA`) |
| `fifa96_action_locomotion_placement_counter(move_attr, limit, &counter)` | row 03 `0x7E2E0..0x7E2F4` | `counter = min((uint16)((move_attr>>22)+1), limit)` (native `SAR 0x16`, `INC`, `CMP AX,DX`/`JBE`) |
| `fifa96_action_locomotion_phase1_clamp(side, &target_z)` | row 03 `0x7E3CA..0x7E417` | side 0 caps `target_z` at `-0x20`; side 1 floors it at `+0x20` (native `JLE -0x20` / `JGE 0x20`) |
| `fifa96_action_sequence_velocity_scale(type_x, type_z, &vel_x, &vel_z, &speed)` | row 0D arm 0 `0x825B2..0x82613` | `scale = (type_x != 0 && type_z != 0) ? 3 : 4` (`0x825CB..0x825E1`); `vel = type*scale` (`IMUL`, 16-bit store); `speed = fifa96_entity_distance(vel_x, vel_z)` (`0x825F7 CALL 0x8DC68`) |

Existing tested helpers reused with the fresh derivation: `move_step` /
`move_target` (row 00), `locomotion_restart_target` (row 02 phase 1),
`locomotion_hold` / `locomotion_clamp_placement` (row 03), `stage_enter` /
`stage_tick` / `stage_advance` / `stage_finish` / `stage_marker`,
`sequence_select`, `sequence_duel_event`, `camera_lead`.

## 4. Engine wiring (row 00)

The FU-137 seam's handler typedef takes only the run (`fn(mr)`), because the
native handler receives only the record (FU-137 §2). To bind row 00, the engine
gains the minimal derived record surface (no pool, no team walk):

* `struct fifa96_match_run_record` in `fifa96_match_run.h` with the FU-76 §3.1
  fields: `pos_x/+0x59`, `pos_z/+0x61`, `target_x/+0x4D`, `target_z/+0x55`,
  `timer89/+0x89`, `timer81/+0x81`, `delta/[0x157A64]`, `active/+0x8D`,
  `has_slot/+0x20`, `dir_x`/`dir_z` (`slot+0x1D/+0x1E>>24`) and `install` (the
  derived `FUN_0007D9A4` request stand-in).
* `mr->record`, zeroed by `fifa96_match_run_init` and `fifa96_match_run_begin`
  (fresh match).
* `fifa96_match_action_00` (in `fifa96_match_handlers.c`) unpacks the record
  into `fifa96_action_move_state`, runs `fifa96_action_move_step`, repacks
  `timer89`, and on `move` runs `fifa96_action_move_target` into
  `target_x/target_z`; an install request lands in `record.install`
  (`3` active / `0x19` inactive).
* `fifa96_match_action_table[0] = {00, fifa96_match_action_00, ...}` — the
  FU-137 §6.1 row 00 class moves `unwired -> ported` (FU-137 errata).

Not wired / not modelled: the record pool and team binding (OL-16, carrying
FU-137 OL-1), the `FUN_0007D9A4` installer itself (the request is recorded, not
staged into `[rec+0x18]`), the full `FUN_0007D9A4` side effects, and every
row-specific arm handled in §2/§6.

## 5. Totals

| group | rows | state |
|---|---|---|
| ported + wired | `00` | `FIFA96_OK` through `fifa96_match_dispatch_action` |
| pure part ported, wiring open | `01`, `02`, `03`, `0D` | still `-FIFA96_ERR_UNSUPPORTED`; six new tested helpers |
| documented, unported | `04`–`0C`, `0E`–`13` | `-FIFA96_ERR_UNSUPPORTED` with the leg named in §2/§6 |

The FU-137 action table now dispatches `1 × OK + 44 × UNSUPPORTED`; the phase
table is unchanged (`34 × UNSUPPORTED + 1 × NOT_FOUND`).

## 6. Open legs

* **OL-16 — record pool / team binding (carries FU-137 OL-1).** The engine keeps
  one record; the native action dispatch runs per record of a 0xB2-strided team
  pool (FU-137 §3) and stages `[rec+0x18]` through `FUN_0007D9A4`. C8/C11 own
  the pool; row `1E` (keeper) and future rows bind there.
* **OL-17 — row 01 arms.** Celebration-id chain, `FUN_0008DE8C` nearest, ball
  stage `FUN_0007A490`, `FUN_000974DC` sound and the `FUN_0008F188` ring
  signals are unported (FU-82 §3.1).
* **OL-18 — row 02 arms.** Stage 1/2 nearest/vector/`FUN_00092820`/ball stage
  and the `FUN_0007DAB4` chooser remain unported (FU-82 §3.2, FU-76 §1.1).
* **OL-19 — row 03 indirect pieces.** The `[rec+0x1C]` phase-handler call
  (`0x7E252`), camera placement `FUN_00079F3C` (`0x7E3A8`) and the phase-7 ball
  scan (`0x7E418`) are unported (FU-77 §7 legs 1/5).
* **OL-20 — rows 04/08 chase machinery.** Focused/wing vectors, ranked pick,
  chase-gate installer and the 6-tap RNG `FUN_00092AC8` are unported
  (FU-77 §2.4, FU-75 §1.6).
* **OL-21 — rows 05/06/07/0F on-ball/kick/possession.** The plan's cluster B
  bodies plus the ball staging/resolver row support (FU-76 §3, FU-77 §2.5/2.6,
  FU-78).
* **OL-22 — rows 09/0A/0B/0C/0D/0E stage/sequence arms.** The `CS:` arm bodies,
  the `0x14E04` angle→vector fold, `FUN_00079C50`/`FUN_000702F8`/`FUN_0006E598`
  and the nearest searches are unported (FU-81 §2.1, FU-82 §3.3/3.4).
* **OL-23 — row 0A entry.** The body is unported and the FU-75 §1.9 installer
  `0x7CDD8` has no xrefs; reachability is unproven (FU-136 §7 leg 3).
* **OL-24 — rows 10/11/12/13 arms.** Camera/ball staging, event tables
  `0x855D4`/`0x85DC8`, `0x8DE8C` nearest and the install-4/0x13 arms are
  unported (FU-81 §2.1, FU-82 §3.7).
* **OL-25 — `FUN_0007DAB4` reset/chooser.** Shared by rows 01/02/04..13; its
  body is unported (FU-76 §1.1, FU-137 OL-2).

## 7. Concerns

* The native action table targets are not Ghidra functions in `/FIFA96.EXE`,
  so all citations are raw disassembly windows; the addresses are identical to
  `/fifa96_le.bin` (FU-130 §6), which lets the FU-76/77/81/82 derivations stand
  as body evidence.
* Rows `01`/`02`/`03`/`0D` have their record-visible cores ported and tested but
  are deliberately **not** wired: their arms call unported helpers, and wiring
  them now would either silently skip native side effects or invent them. The
  task's rule — port the pure part, leave the wiring as a numbered leg — is
  what OL-17..OL-24 record.
* `word[+0x6B]` (the dword `[+0x69]>>16` the row-02 wait reads) is not written
  by the shared mover (`0x7BF20..0x7BF47` writes `+0x67`/`+0x69` only); FU-75
  calls `+0x6B/+0x6D` a lane direction pair, so the wait's `lane` argument is
  caller-supplied and the field's writer is an OL-18/OL-25 concern.
* The FU-137 §6.1 evidence strings changed for row 00 (class) and gained FU-138
  cross-refs on rows 01/02/03/0D; the FU-137 doc carries the matching errata.

## Provenance

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit): `read_memory 0x1106E0`
(80 B), `read_memory 0x7DBB0` (16 B); `disassemble_bytes 0x7DB10..0x7DBB0`,
`0x7DBC0..0x7DC60`, `0x7DC6B..0x7DCB0`, `0x7DFE6..0x7E0B0`, `0x7E0B0..0x7E1A4`,
`0x7E1D6..0x7E2A4`, `0x7E2D3..0x7E418`, `0x7BF20..0x7BFD0`,
`0x7E7C8..0x7E7F0`, `0x7F194..0x7F1C0`, `0x801B4..0x801E0`, `0x814B0..0x814E0`,
`0x81068..0x81090`, `0x80A00..0x80A30`, `0x81738..0x81768`, `0x81908..0x81938`,
`0x81C90..0x81CC0`, `0x8251C..0x8254C`, `0x82710..0x82740`, `0x82AD0..0x82B00`,
`0x855F0..0x85620`, `0x85DE4..0x85E14`, `0x83D68..0x83D98`, `0x84B00..0x84B30`,
`0x82567..0x82620`, `0x80A87..0x80AE0`. No writes: no rename/comment/label/
function/script/project save. `/fifa96_le.bin` and `/fifa96.exe` untouched.

Repo: `make check` = **98/98** at task start and after the task (ASan/UBSan
engine tests included); M1 golden and pinned render hashes unchanged (no render
path touched). Port write set: `include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c`, `tests/test_action_handlers.c`,
`include/fifa96_engine/fifa96_match_run.h`,
`src/fifa96_engine/fifa96_match_run.c`,
`src/fifa96_engine/fifa96_match_handlers.c`,
`tests/test_engine_match_handlers.c`, `CMakeLists.txt` (engine links
`fifa96_action_handlers`), `docs/ghidra/FU137_dispatch_mechanics.md` (errata).
`game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not staged.
