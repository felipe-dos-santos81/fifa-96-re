# Slice-B camera census (2026-11-12)

Date: 2026-11-12. Status: complete. Workstream **B0** of
`2026-11-12-open-items-closure-slice-b-design.md`.

The 19 Slice-B `0x8ED40` camera/stat sites, freshly read from `/FIFA96.EXE`
(Ghidra MCP) and mapped to their enclosing body, native arm/gate, argument
entity, delta, engine owner, and port status. This table is the starting map
for Tasks 3–11; it is **not** a substitute for the port tasks' own reads.

## Method / evidence reads

All reads this slice, program `/FIFA96.EXE`:

| read | result |
|---|---|
| `get_xrefs_to 0x8ED40` | 35 `UNCONDITIONAL_CALL` sites (the §"35, not 29" census, FU72 Closure). The 19 Slice-B addresses are present; the 16 non-Slice-B sites are the 12 already-wired and the 4 carried (`0x775FD`/`0x77613`, `0x7ADD2`/`0x7AE04`). |
| `disassemble_function 0x72478` | `FUN_00072478` 0x72478..0x72AC0 (477 insns). Authoritative for the four tracker sites. |
| `disassemble_function 0x72AC4` | `FUN_00072AC4` 0x72AC4..0x736A9 (766 insns). Authoritative for the two save-block sites. |
| `decompile_function 0x70DE0` | `FUN_00070DE0` (the reposition body; the two `+3` arms). |
| `decompile_function 0x7131C` | `FUN_0007131C` (the goal-mouth/pan body; the six arms). |
| `disassemble_bytes 0x779DE` (len 0x140) + `0x77B10` (len 0x380) | row-1C tail head 0x779DE..0x77E8F, covering the four tail sites. |
| `disassemble_bytes 0x8AD2E` (len 0x80) | `MOV EBP,[0x157A9B]` and the case-6 arm through `0x8AD54`. |
| `disassemble_bytes 0x70EF0` (len 0x140) | the `FUN_00070DE0` boundary arms (`0x70F44`/`0x70FCE`), byte-exact gates. |
| `disassemble_bytes 0x714B0`/`0x714A0` | the six `FUN_0007131C` sites' `MOV EDX,imm` deltas, byte-exact. |
| `disassemble_bytes 0x71460`/`0x715B0`/`0x71690`/`0x71700`/`0x71760` | the `FUN_0007131C` goal-arm gates (review fix): `0x714BE` cam-X admission `abs([0x15774C]) > 0x1E0`, the `0x71732` prev/current-phase pair, the `0x717E3` `[0x15758D] != 0` guard. |

## Census — 19 sites

| site | body | native arm/gate | argument | delta | engine owner | port status | test |
|---|---|---|---|---|---|---|---|
| `0x70F44` | `FUN_00070DE0` | `local_30 > 0x90` boundary arm (`0x70F2C CMP [ESP+0x28],0x90` / `0x70F34 JLE 0x70FAC` fall-through) | `[0x1577CA]` (TRACKED) | `+3` | `fifa96_camera_reposition` | block missing (tail) | B1 |
| `0x70FCE` | `FUN_00070DE0` | `local_30 <= 0x90` && (`local_34 < -0xC0` or `local_34 > 0xC1`) (`0x70FB0 CMP EDX,0xFFFFFF40`/`JL`; `0x70FB8 CMP EDX,0xC1`/`0x70FBE JLE 0x710DF`) | `[0x1577CA]` (TRACKED) | `+3` | `fifa96_camera_reposition` | block missing (tail) | B1 |
| `0x714BE` | `FUN_0007131C` | `[0x157A4A]>>24 == 2` && `abs([0x15774C]) > 0x1E0` && `[0x157A83]==0` && `[0x1577CA]!=0` && `FUN_0008F078(tracked,tracked) > 0x1260` | `[0x1577CA]` (TRACKED) | `+5` | `fifa96_match_goal_arm` | block missing (arms) | B2 |
| `0x7157C` | `FUN_0007131C` | `[0x157A4A]>>24 == 0x10`; `abs([0x157754]) > 0x960`; `abs([0x15774C]) < 0x3C0`; `abs([0x15774C]) < 0xD0` | `[0x1577CA]` (TRACKED) | `-100` | `fifa96_match_goal_arm` | block missing (arms) | B2 |
| `0x715DD` | `FUN_0007131C` | `[0x157A4A]>>24 != 0x10`; `abs([0x15759F]) < 0x1E0` && `[0x1575A7] > 0x840` | `[0x1577CA]` (TRACKED) | `-5` | `fifa96_match_goal_arm` | block missing (arms) | B2 |
| `0x716C7` | `FUN_0007131C` | `[0x157A4E]==0x10` or `[0x157A4A]>>24 == 0x10`; `abs([0x15774C]) >= 0xD0`; `[0x15758D] < 0x1E0` | `[0x1577CA]` (TRACKED) | `-100` | `fifa96_match_goal_arm` | block missing (arms) | B2 |
| `0x71732` | `FUN_0007131C` | previous phase `[0x157A4E] != 0x10` && current phase `[0x157A4A]>>24 != 0x10`; `abs([0x15759F]) < 0x1E0` && `[0x1575A7] > 0x840` | `[0x1577CA]` (TRACKED) | `-5` | `fifa96_match_goal_arm` | block missing (arms) | B2 |
| `0x717E3` | `FUN_0007131C` | `[0x15758D] != 0` && `[0x15758D] < 0x30` tail (the `0x52`/`0x44` event arm) | `[0x1577CA]` (TRACKED) | `+5` | `fifa96_match_goal_arm` | block missing (arms) | B2 |
| `0x72649` | `FUN_00072478` | fall-through pair, first (`0x7263F MOV EDX,0x32`) | `[0x1577CA]` (TRACKED) | `+0x32` | engine-owned tracker (unported) | body missing | B3 |
| `0x72660` | `FUN_00072478` | fall-through pair, second (`0x72655 MOV EDX,0xFFFFFFB5`; `0x7265A MOV EAX,[EAX+0x7A6]`) | `[team+0x7A6]` (BALL_SUB7A6) | `-0x4B` | engine-owned tracker (unported) | body missing | B3 |
| `0x7270F` | `FUN_00072478` | `[0x15B43C]` staging arm: `record[0]==0xB` (`0x726F6 MOVSX/CMP 0xB`) then `0x72702 MOV EDX,0x19` / `0x72707 MOV EAX,[EAX+0x5]` | `[0x15B43C]+5` (REC_15B43C) | `+0x19` | engine-owned tracker (unported) | body missing | B3 |
| `0x72A52` | `FUN_00072478` | previous phase `0x10` (`0x724D9 CMP EAX,0x10`/`0x724DC JZ 0x72A48`; `0x72A48 MOV EDX,0xF`) | `[0x1577CA]` (TRACKED) | `+0xF` | engine-owned tracker (unported) | body missing | B3 |
| `0x72DE3` | `FUN_00072AC4` | save/follow block, same-side (`0x72DC1 CMP DL,byte[EBX+0x826]`/`JNZ`); `0x72DD9 MOV EDX,0x1` | `[0x1575DA]` (REC_1575DA) | `+1` | `fifa96_camera_save_tail` (loader) | body missing | B4 |
| `0x72ED1` | `FUN_00072AC4` | `[0x1575B3] > 0x750` && `abs([0x1575AB]) < 0x2D0` (`0x72EA2 CMP`/`0x72EC0 CMP`); `0x72EC7 MOV EDX,0x2` | `[0x1575DA]` (REC_1575DA) | `+2` | `fifa96_camera_save_tail` (loader) | body missing | B4 |
| `0x77DFD` | row-1C tail | phase-0x10 negative product (`0x77DD3 CMP EAX,0x10`/`JNZ 0x77E0B`; `0x77DE6 IMUL`; `0x77DE9 TEST`/`0x77DEB JGE 0x77E6F`; `0x77DF1 MOV EDX,0x64`) | `EBP` (LOCAL_EBP) | `+0x64` | `fifa96_match_action_1C` | block missing (tail) | B5 |
| `0x77E38` | row-1C tail | phase-non-0x10 nonnegative product (`0x77E0B`; `0x77E19 IMUL`; `0x77E1E JL 0x77E55` fall-through; `0x77E31 MOV EDX,0x19`) | `EBP` (LOCAL_EBP) | `+0x19` | `fifa96_match_action_1C` | block missing (tail) | B5 |
| `0x77E4E` | row-1C tail | phase-non-0x10 nonnegative product, optional tracked `+5` (`0x77E3D MOV ECX,[0x1577CA]`/`0x77E43 TEST`/`JZ`; `0x77E47 MOV EDX,0x5`) | `[0x1577CA]` (TRACKED) | `+5` | `fifa96_match_action_1C` | block missing (tail) | B5 |
| `0x77E61` | row-1C tail | phase-non-0x10 negative product (`0x77E1E JL 0x77E55`; `0x77E55 MOV EDX,0x5`) | `EBP` (LOCAL_EBP) | `+5` | `fifa96_match_action_1C` | block missing (tail) | B5 |
| `0x8AD54` | `FUN_0008A938` case 6 | `[0x157A9B] != NULL` (`0x8AD49 TEST EBP,EBP`/`0x8AD4B JZ 0x8AD69`; `0x8AD4D MOV EDX,0xFFFFFF6A`) | `[0x157A9B]` (REC_157A9B) | `-0x96` | `match_run_replay_note_goal` | unproved producer (open leg B0-L3) | B6 |

> The `FUN_00072478` rows follow the FU-67 pairing and are **not** in address
> order: `0x72A52` is the previous-phase-0x10 arm; `0x72649`/`0x72660` are the
> fall-through pair (an ordered `+0x32` then `-0x4B` in the same arm, the
> native two-events-in-one-arm case); `0x7270F` is the `[0x15B43C]` staging arm.

## The row-1C tail: three routes, not one sequence

The `0x779DE..0x77E8B` tail posts its four sites across three separate routes
(design §10 correction; not a single four-stat sequence):

1. **phase-0x10 negative product** → `0x77DFD` (`+0x64`, `EBP`).
2. **phase-non-0x10 nonnegative product** → `0x77E38` (`+0x19`, `EBP`) and,
   only when `[0x1577CA] != 0`, `0x77E4E` (`+5`, tracked).
3. **phase-non-0x10 negative product** → `0x77E61` (`+5`, `EBP`).

## Entity-kind table

| kind | native argument | engine cell | note |
|---|---|---|---|
| `TRACKED` | `[0x1577CA]` | `mr->render.camera.tracked` (`fifa96_camera.tracked`) → `fifa96_match_run_stats_add` | resolves; the entity_id is in the 0..21 team-record range. |
| `BALL_SUB7A6` | `[team+0x7A6]` | **OPEN** | the pointer immediately after the team block's eleven `0xB2` records (FU-67 §3.1 `+0x7A6` / §3.2 `11×0xB2 = 0x7A6`), i.e. the ball record — **not** the tracked record's sub-object. The engine models the ball as `pool->ball` (a separate `fifa96_match_ball`, `fifa96_match_entities.h`), which has no stats-table roster id: `fifa96_match_run_stats_add` only accepts `entity_id` in `0..21`. |
| `REC_15B43C` | `[0x15B43C]+5` | **OPEN** | the last ring-search result's entity (FU-72 §1.4/§6: `[0x15B43C]` = `FUN_00092998(1,4,any)` result, written `0x71463`, read `0x726E9`). The engine ports the ring mechanics (`fifa96_ring_stats`) but has no match-run cell for `[0x15B43C]`. |
| `REC_157A9B` | `[0x157A9B]` | **OPEN** | the backup-record pointer written by `FUN_00088940` at `0x8895B`/`0x88A1C`/`0x88AA4`. The engine has **no** `[0x157A9B]` producer (FU-72 B1 closure); the NULL branch (`FUN_0008ECC4` → `fifa96_match_run_stats_bump`) is the live scorer path. |
| `LOCAL_EBP` | handler's own record | `mr->record.entity_id` → `fifa96_match_run_stats_add` | resolves; the row-1C handler's `r->entity_id`. |
| `REC_1575DA` | `[0x1575DA]` | **OPEN** | the save block's own record — `[0x1575DA] = [0x157A87]` (`0x72BAC`), the previous-current entity of `FUN_00072AC4`. The save block is unported (B4), so the engine has no cell yet; the closest existing cell is `pool->controlled` (`[0x157A83]`), which is not the same record. |

## Open legs

- **B0-L1 — `BALL_SUB7A6`.** The ball record has no stats-table entity_id in
  the engine; `fifa96_match_run_stats_add`'s range `0..21` excludes it. B3 must
  either project a ball id into the table or record a numbered leg — never a
  fabricated id.
- **B0-L2 — `REC_15B43C`.** No match-run cell holds the last ring-search
  result's entity; B3 resolves it or carries a numbered leg.
- **B0-L3 — `REC_157A9B`.** Resolved by Task 11 (B6): **unproved** — the native
  arm is reachable through `FUN_00088940`'s team-mismatch write (`0x88A1C`,
  FU145 L2/L3) but the engine has no `[0x157A9B]` cell, so it is neither live
  nor census-proven dead. Stays an open leg; no test, no fabricated `-300`.
- **B0-L4 — `REC_1575DA`.** No engine cell until the save block (B4) lands one.

## Task 4 (B1) outcome — the `FUN_00070DE0` boundary arm (2026-11-12)

Task 4 ports the `0x70E3C..0x710DA` bit-0x8 boundary arm into
`fifa96_camera_reposition` (`fifa96_camera.c`). The two `+3` sites are now
live:

- `0x70F44` (`{0x70F44, +3, TRACKED}`) — boundary_y > 0x90 (the `0x70F2C CMP
  [ESP+0x28],0x90` arm), plus the corner `0x8F188(0x49, tracked, 0x40)`.
- `0x70FCE` (`{0x70FCE, +3, TRACKED}`) — boundary_y <= 0x90 && (boundary_x <
  -0xC0 || boundary_x > 0xC1) (the `0x70FB0`/`0x70FB8` arm), plus the corner
  `0x8F188(0x48, tracked, 0x40)`.

Both record the corner `0x8F188` request in the reposition's `events` (0x49 /
0x48) and `event_code` (0x40) out-params; the `0x8F188` ring post and its
phase-2 gate (`[0x157A4A] >> 24 == 2`) stay the consumer's (OL-27). The
"target" clamp (z snapped to ±0xAD4, sign from the entering current z) and the
boundary-point projection (a 32-bit IMUL/IDIV per axis onto z = ±0xB0F/±0xB11)
are ported. The boundary classify gates (`& 0x4` reclassify, `& 0x13` direct)
re-enter the existing step-back. The high arm also folds
`vel_z = ±max(4, |vel_z|/2)` (loader-owned, `0x70F6B..0x70FA1`); only the low
arm's `FUN_00092AC8` RNG velocity stays a leg.

**New open leg (Task 4):**

- **T4-L1 — the `0x71231` reseed.** The step-back tail reseeds
  `[0x15781C] = [0x157A64]` (the armer counter := the frame-delta low byte)
  when `changed != 0 && |cur_z| > 0xB21 && [0x15781C] == 0 && speed >= 0xA`
  (native `0x711FB..0x71236`). Both inputs — the counter (`mr->
  goal_counter_15781c`) and the frame delta (`mr->state.frame_delta`) — are
  engine-owned, so the loader (`fifa96_camera_reposition`, no signature
  change) cannot reseed them; the cell stays a leg. The Task 3 zero-guard
  test remains the recorded behavior.

**Still legs (OL-T11-79, unchanged):** the low arm's `FUN_00092AC8`
global-RNG velocity (not owned by the loader), the `0x974DC/0x651F0/0x974F0`
sound sinks, and the elevation loop. (The high arm's `vel_z = ±max(4,
|vel_z|/2)` fold is loader-owned and **is** ported — `fifa96_camera.c`, the
`0x70F6B..0x70FA1` arm.)

## Task 5 outcome — the `FUN_00092864` commit and the shadow stores (2026-11-12)

Task 5 ports the native `0x73723 CALL 0x92864` commit point and the
`0x73728..0x73750` word shadow stores (`fifa96_camera_commit` in
`fifa96_camera.c`, wired in `match_run_frame_camera` before the walk; the
jitter is Task 6, the save tail Task 9).

The camera-visible half is the three previous-frame shadows, all plain 16-bit
MOVs (fresh `disassemble_bytes 0x73710`, byte-exact):

| store | source | field | width/sign |
|---|---|---|---|
| `0x7372E` `[0x157825] = AX` | `word[0x1577BE]` (`speed`) | `shadow_speed_157825` | `uint16_t` |
| `0x7373F` `[0x157827] = AX` | `word[0x1577C0]` (`vel_x`) | `shadow_vel_157827` | `int16_t` |
| `0x73750` `[0x157829] = AX` | `word[0x1577C2]` (`vel_z`) | `shadow_vel_157829` | `int16_t` |

(`0x7373A MOV EDI,0x157758` and `0x7374B MOV ESI,0x15774C` in the same range
are the `0x73756 CALL 0x72AC4` save-tail's `MOVSD` setup, not shadow stores.)

**New open leg (Task 5):**

- **T5-L1 — the `FUN_00092864` event-ring append.** Fresh
  `disassemble_bytes 0x92864`: the commit reads the live `speed`/`vel_x`/
  `vel_z`, compares them against the shadows, and — when the triple changed and
  the sink code `byte[0x15B650] != 0x26` — advances the ring index
  `byte[0x15B665] = (index + 1) % 25` and copies the 21-byte sink record
  `0x15B650..0x15B664` to `0x15B440 + index*0x15`, then clears the sink byte
  (`XOR AH,AH ; MOV [0x15B650],AH`). This is the camera-motion feed into the
  replay event ring (FU-142 OL-27): the append mechanics are already ported as
  `fifa96_event_ring_append` (`FUN_000928F0`), but the camera commit's
  motion-change gate is **not** wired — the loader `fifa96_camera` owns no
  event ring, and the engine's `fifa96_event_queue` has no driver for the
  camera-motion commit. The three shadow stores are ported and tested
  (`test_camera_commit_shadows`).

## Task 11 outcome — the `FUN_0008A938` case-6 `[0x157A9B]` arm (2026-11-13)

Fresh reads, program `/FIFA96.EXE` (Ghidra MCP, read-only):

| read | result |
|---|---|
| `get_xrefs_to 0x157A9B` | 9 refs: WRITE `0x8895B`/`0x88A1C`/`0x88AA4` (all `FUN_00088940`); READ `0x88A3A`/`0x88A5B` (`FUN_00088940`), `0x4BB61`/`0x4BB7E` (`FUN_0004BB5C`), `0x8AD2E`/`0x8AD59` (`FUN_0008A938`). No engine-side producer. |
| `disassemble_bytes 0x8AD2E` (len 0x80) | the case-6 arm: `0x8AD2E MOV EBP,[0x157A9B]`; `0x8AD49 TEST EBP,EBP`/`0x8AD4B JZ 0x8AD69`; non-NULL arm `0x8AD4D MOV EDX,0xFFFFFF6A` (`-0x96`)/`0x8AD52 MOV EAX,EBP`/`0x8AD54 CALL 0x8ED40`; NULL arm `0x8AD69 MOV EAX,[0x157A9F]`/`0x8AD77 CALL 0x8ECC4`. |
| `disassemble_function 0x88940` | the producer: `0x8895B` clears `[0x157A9B]=0`; the team-mismatch arm `0x88A16..0x88A1C` writes `[0x157A9B]=[search+5]`; `0x88AA4` writes `[0x157A9B]=EAX` (0) on the no-search-result path; the same function writes `[0x157A9F]` at `0x889E0` and calls `FUN_0008A938` with `EAX=6` at `0x88B44`. |
| `disassemble_function 0x4bb5c` | a consumer: reads `[0x157A9B]`, falls back to `[0x157A9F]` when NULL. |

**Outcome 3 — unproved (open leg), no port, no test.**

- The native arm **is reachable**: `FUN_00088940`'s team-mismatch arm writes
  `[0x157A9B]` non-NULL at `0x88A1C` and then reaches `0x88B44 CALL 0x8A938`
  with `EAX=6`. So this is **not** a census-proven-unreachable (native-dead)
  site — and it is **not** engine-live, since the engine has no `[0x157A9B]`
  cell.
- The engine's derived goal scanner `fifa96_match_run_goal_scan` (FU-145 S2)
  writes only `mr->goal_record` (`[0x157A9F]`); the `[0x157A9B]` producer chain
  — the unported `FUN_00092998(1,4,-1)` possession search and the
  team-mismatch arm — is FU145 L2/L3. The `FIFA96_CAMERA_STAT_REC_157A9B` kind
  stays OPEN in the drain (skipped, never entity 0).
- **Why no test:** outcome 1 (live) requires an engine-produced `[0x157A9B]`
  cell and outcome 2 (dead) requires native unreachability; neither holds. A
  `-300` test would have to fabricate a producer — forbidden. No code is added.

**Row/leg update.** The `0x8AD54` row's port status moves from "block missing
(non-NULL arm)" to **unproved producer — open leg B0-L3** (FU145 L2/L3). B0-L3
is resolved *unproved* and stays open; the NULL branch
(`fifa96_match_run_stats_bump`) remains the engine's live scorer path.

## Provenance

Ghidra MCP on `/FIFA96.EXE` (read-only): the reads listed in §Method. No
tool, capture-rig, ISO, Ghidra-project, or code change. Write set: this file
and `port/spec/FU72_ring_stats.md` (append-only). The 19 sites are the fixed
list `0x70F44, 0x70FCE, 0x714BE, 0x7157C, 0x715DD, 0x716C7, 0x71732, 0x717E3,
0x72649, 0x72660, 0x7270F, 0x72A52, 0x72DE3, 0x72ED1, 0x77DFD, 0x77E38,
0x77E4E, 0x77E61, 0x8AD54` — all 19 unique, no duplicates.
