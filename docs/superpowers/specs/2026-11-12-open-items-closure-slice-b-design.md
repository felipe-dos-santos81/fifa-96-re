# Open-items closure — Slice B (live chains) design

Date: 2026-11-12. Status: proposed; awaiting operator approval.
Predecessor: `2026-11-12-open-items-closure-slice-a-design.md` (proofs and
errata; landed at `573e540`).

## 1. Intent

Close the engine items Slice A deliberately handed over: the live camera
chain and its activity stats, the row-1C tail, the `FUN_0008A938`
`[0x157A9B]` arm, the replay recorder/queue chain, and the remap/appearance
producers. The campaign's measure is not "documents written" but native
behavior the engine can execute and a test can pin.

The operator chose the full-campaign approach: port the bodies and wire every
Slice-B `0x8ED40` site, then the replay chain. Fresh 2026-11-12 reads show the
four camera bodies are not greenfield: `FUN_00070DE0` already has a derived
core (`fifa96_camera_reposition`), `FUN_000736AC` is modeled in
`fifa96_camera_update_walk` and the engine's goal-arm/boundary path, and
`FUN_0007131C`'s arming window is `fifa96_match_goal_arm`. The missing pieces
are the stat-bearing arms, the `FUN_00072AC4` save block, the
`FUN_00072478` tracker/decision body, the armer head, the walk's head jitter
and pause bridge, and the row-1C tail.

## 2. Goals

1. Wire the **19 Slice-B `0x8ED40` sites** (one row per address; the table is
   the census's starting map, not a substitute for its fresh read):

| Site | Enclosing body | Argument | Delta |
|---|---|---|---|
| `0x70F44` | `FUN_00070DE0` | `[0x1577CA]` | `+3` |
| `0x70FCE` | `FUN_00070DE0` | `[0x1577CA]` | `+3` |
| `0x714BE` | `FUN_0007131C` | `[0x1577CA]` | `+5` |
| `0x7157C` | `FUN_0007131C` | `[0x1577CA]` | `-100` |
| `0x715DD` | `FUN_0007131C` | `[0x1577CA]` | `-5` |
| `0x716C7` | `FUN_0007131C` | `[0x1577CA]` | `-100` |
| `0x71732` | `FUN_0007131C` | `[0x1577CA]` | `-5` |
| `0x717E3` | `FUN_0007131C` | `[0x1577CA]` | `+5` |
| `0x72649` | `FUN_00072478` | `[0x1577CA]` | `+0x32` |
| `0x72660` | `FUN_00072478` | `[ball[0]+0x7A6]` | `-0x4B` |
| `0x7270F` | `FUN_00072478` | `[0x15B43C]+5` | `+0x19` |
| `0x72A52` | `FUN_00072478` | `[0x1577CA]` | `+0xF` |
| `0x72DE3` | `FUN_00072AC4` | `[0x1575DA]` | `+1` |
| `0x72ED1` | `FUN_00072AC4` | `[0x1575DA]` | `+2` |
| `0x77DFD` | row-1C tail | `EBP` | `+0x64` |
| `0x77E38` | row-1C tail | `EBP` | `+0x19` |
| `0x77E4E` | row-1C tail | `[0x1577CA]` | `+5` |
| `0x77E61` | row-1C tail | `EBP` | `+5` |
| `0x8AD54` | `FUN_0008A938` case 6 non-NULL | `[0x157A9B]` | `-0x96` |

The `FUN_00072478` rows follow FU-67: `0x72A52` is the previous-phase-0x10
arm; `0x72649`/`0x72660` are the fall-through pair; `0x7270F` is the
`[0x15B43C]` staging arm. They are not in address order.

2. Complete the camera spine those sites hang from: the `FUN_00070DE0` tail,
   the `FUN_0007131C` counter head (`0x71325..0x71349`) and its
   `FUN_00070DE0` call at `0x7134F`, the `FUN_00072AC4` save block, the
   `FUN_00072478` body, and the `FUN_000736AC` head jitter
   (`0x736B2..0x73723`) and pause bridge. The `0x737C8` descent admission is
   **already ported** (`fifa96_camera.c:407-413`); it needs a regression test,
   not new code.
3. Close the replay-chain legs: RR-L2 recorder latch, RR-L3 band/queue
   producers, RR-L4 appearance mapping, RR-L5 `0x20` camera cycle/matrix
   precedence, RR-L7 mode-7 request/timed exit, RR-L10's `0x974DC` drain half,
   and C4R-L1's `0x5865C` view reset.
4. Close the remap/appearance producers: B2-L1 (the non-zero `side_swap`
   flip), A1-R (the 23rd-slot runtime consumer), FU348-L2c/L2d (option-cell
   producer and the non-default A4 branch), and RR-L9 (the `0x15887B`
   ball-frame producer).
5. Leave no silent residue. The other four `0x8ED40` sites are **not** in this
   slice: `0x775FD`/`0x77613` stay in the unported row-1B tail (FU285-L8) and
   `0x7ADD2`/`0x7AE04` in the `FUN_0007A490` tail. B11 records them as carried.

## 3. Non-goals

- Reworking `FUN_0004B100` (the full entity frame chain). The camera spine is
  driven from the engine's existing frame point; the rest of `0x4B100` stays a
  numbered leg.
- Re-pinning `m1-frames.txt` (immutable). M2 moves only per the §9 ruling.
- Closing the four non-Slice-B `0x8ED40` sites.
- The 80-slot completion census: none of the five bodies is a dispatch row. If
  the census finds a row body, the README/ENGINE numbers are recomputed in
  that commit.
- Inventing handlers for the unsupported record selectors (`0x16`, `0x60`,
  `0x62`, `0x66`, `0x72`, `0x7A`, FU-28).

## 4. Approaches considered

1. **Reachability-first, live code only.** Census, then port only what the
   live match can reach. Rejected by the operator: leaves documented dead
   sites unported.
2. **Replay/camera fidelity first.** Land the replay chain before the stats
   campaign. Rejected: hides the 19-site debt behind visible work.
3. **Full campaign (chosen).** Census every site/leg to its block, port the
   block, wire the call, test it. A site is recorded dead only when
   unreachability is **proved**; otherwise it stays a numbered open leg.

## 5. Architecture and interfaces

- The camera functions stay in `port/src/fifa96_loader/fifa96_camera.c`
  (`fifa96_camera` and the derived bodies). They must not call engine code or
  receive an engine callback.
- **Statistics contract.** A stat-bearing arm fills a caller-supplied event
  array:

  ```c
  typedef enum {
    FIFA96_CAMERA_STAT_TRACKED = 0,   /* cam->tracked ([0x1577CA]) */
    FIFA96_CAMERA_STAT_BALL_SUB7A6,   /* [ball[0]+0x7A6] */
    FIFA96_CAMERA_STAT_REC_15B43C,    /* [0x15B43C]+5 */
    FIFA96_CAMERA_STAT_REC_157A9B,    /* [0x157A9B] */
    FIFA96_CAMERA_STAT_LOCAL_EBP      /* the body's own record (row-1C tail) */
  } fifa96_camera_stat_entity;

  typedef struct {
    uint16_t site;        /* the native CALL address, e.g. 0x70F44 */
    int16_t delta;        /* the native EDX value */
    uint8_t entity;       /* fifa96_camera_stat_entity */
  } fifa96_camera_stat_event;

  #define FIFA96_CAMERA_STAT_MAX 8
  typedef struct {
    fifa96_camera_stat_event event[FIFA96_CAMERA_STAT_MAX];
    uint8_t count;
    uint8_t overflow;
  } fifa96_camera_stat_events;
  ```

  The engine drains the events in native order into
  `fifa96_match_run_stats_add`. It resolves each entity kind against the ids
  it already owns: `TRACKED` is `cam->tracked`; `LOCAL_EBP` is the handler's
  record id; `BALL_SUB7A6`, `REC_15B43C` and `REC_157A9B` are engine-owned
  cells the census must map. A kind the engine cannot resolve is a numbered
  leg, never a fabricated id. `FUN_00072478` emits two events in one arm
  (`+0x32` then `-0x4B`), which is why the contract is an array.
- **Frame order.** The camera spine runs at the existing
  `match_run_frame_camera` / `fifa96_match_goal_arm` point: after the
  slot/camera update and **before** `match_run_entity_frame`, matching
  `FUN_0004B100`'s `0x4B193` call (`fifa96_match_run.c:8296-8318`, FU145
  §1.8/§2). Do not move the existing call later.
- **Armer head.** `fifa96_match_goal_arm` gains the `[0x15781C]` counter head
  (`0x71325..0x71349`) — a new run cell, aged by `frame_delta` and capped at
  `0x1E` — then calls `fifa96_camera_reposition` unconditionally at the
  `0x7134F` point, before the armed/phase/boundary branches. The function has
  no production caller today (FU145 leg L1, FU164 §2); this is the caller.
- **Save block.** Native `FUN_000736AC` calls `FUN_00072AC4` at `0x73756`,
  then copies the triple to `0x157758` and checks pause. The port's
  `fifa96_camera_update_walk` currently starts with the prev-triple capture;
  the `FUN_00072AC4` adapter must run **before** that capture.
- The replay-chain legs are run cells and ring composer/consumer changes in
  `fifa96_match_run.h` / `fifa96_match_replay_ring.c`, with the native
  addresses in the comments.

## 6. Workstreams

### B0 — the site/leg census (docs + evidence, no behavior)

Fresh Ghidra reads confirm each of the 19 rows above: enclosing block, exact
delta and entity argument, native order, and port status (already modeled,
block missing, body missing). It also maps the replay-chain legs and the
remap producers. Output: a table appended to FU72 and cross-referenced from
ENGINE/PROGRESS. The census is allowed to move a row to "proved unreachable"
(§9) or "kind unresolved, open leg"; it is not allowed to guess.

### B1 — armer head, `FUN_00070DE0` tail, and the walk gaps

Port the `[0x15781C]` counter head and call `fifa96_camera_reposition` at
`0x7134F`; complete the reposition tail (the two `+3` sites); add the
`FUN_000736AC` head jitter and the `flag_157820` → `cam->paused` bridge. Keep
the already-ported descent admission and add its regression test. Tests:
`test_camera.c` for the reposition tail and the counter; a goal-arm ordering
test proving the reposition call precedes the armed/phase branches; frame
tests for the jitter/pause bridge.

### B2 — the `FUN_0007131C` stat arms

Port the six stat-bearing arms of the goal-mouth/pan body on top of the live
`fifa96_match_goal_arm` / `match_run_goal_boundary` model, in native order
(`0x714BE` +5, `0x7157C` -100, `0x715DD` -5, `0x716C7` -100, `0x71732` -5,
`0x717E3` +5). Tests: goal-arm fixtures per arm, including the phase-2 and
phase-0x10 gates.

### B3 — `FUN_00072478`, the tracker/command-ring decision pass

Port `0x72478..0x72AC0` (threshold-driven command-ring decision and posts,
not a goal detector) and its four stat calls in the FU-67 pairing. Resolve
the `[ball[0]+0x7A6]` and `[0x15B43C]+5` entity kinds; if the engine has no
cell for either, the event is a numbered leg. Tests: staged tracker fixtures
for the `+0xF` previous-phase-0x10 arm, the `+0x32`/`-0x4B` pair, and the
`+0x19` staging arm.

### B4 — `FUN_00072AC4`, the save block

Port `0x72AC4..0x736A9` (the save/follow block, one `FUN_00072478` call at
`0x72AE5`, and the two `0x72DE3`/`0x72ED1` stat sites) and insert the adapter
before the current prev-triple capture. Tests: `test_camera.c` for the block;
a stats fixture for the two sites.

### B5 — the row-1C tail

Port the `0x779DE..0x77E8B` tail of `fifa96_match_action_1C` (FU186-L2) and
wire its four sites. Tests: extend the row-1C handler fixture with the tail's
`+0x64`, `+0x19`, `+5` (tracked) and `+5` (self) expectations.

### B6 — the `FUN_0008A938` `[0x157A9B]` arm

Port the `0x8AD54` arm of case 6 (the non-NULL branch calls
`8ED40([0x157A9B], -0x96)`; the NULL branch keeps `FUN_0008ECC4` /
`stats_bump`). Three outcomes: proved live → wire at
`match_run_replay_note_goal`; proved unreachable → port to the gate and record
dead with the proof; unproved → numbered open leg. No fabricated test.

### B7 — the replay recorder and queue chain

- RR-L2: the `[0x14E574]` latch owned by `FUN_00053DC4` (disable the recorder
  gate and set the latch) and cleared by `FUN_00053A48`'s state-5→6
  transition (`FUN_00064074`/`FUN_00064080`). Tests: disable, latch
  persistence, state-5→6 re-enable, and no recording while `[0x109A90]` is
  closed.
- RR-L3: the `0x155C6A` six-byte queue (contents/producers), the camera band,
  the `FUN_00064EE4` queue-pop byte, and the `0x155C51` flag. The inherited
  `0x55C6A` is a truncated address; B11 appends the erratum.
- RR-L10: the 20-dword FIFO at `0x155C90..0x155CDC` (pre-increment, store,
  wrap, eviction, count/head/tail, notify) and the `0x974DC` drain half
  (`FUN_00064EC0` gate → `FUN_00064EF0` push; `FUN_00065CC0` → `[0x10A254]`).
- C4R-L1: the `FUN_0005865C` view reset at the arm (`0x6427F`).

Tests: `test_engine_replay_ring.c` and `test_engine_match_bridge.c` fixtures
for the latch, queue6 vs FIFO, drain, and view reset.

### B8 — the replay camera cycle and request path

- RR-L4: appearance-byte mapping (`***(byte**)(rec+0x28)` / `[0x158866]` vs
  the port's composed row id, OL-80).
- RR-L5: the `0x20` camera record cycle and matrix precedence. The closure
  condition includes proving the native `[0x7DC8]` active-view object identity
  with the port's `native_view_cam`/`action_cam`; test the default pose
  camera's recorded-matrix precedence and each non-pose cycle's source.
- RR-L7: `FUN_0004B100` → `FUN_000510DC` → `FUN_00064270` request/timed exit.

Tests: `test_engine_replay_ring.c` / bridge fixtures for the appearance byte,
the camera cycle/matrix, and the request/exit path.

### B9 — B2-L1 and A1-R

- B2-L1: prove the `[0x157AC2]` mapping, the `0x742B4` writer's caller, and
  the stats-key/block consistency. A non-zero flip ships only with a re-key
  or preservation test; otherwise ship the gate test proving the flip remains
  unwired.
- A1-R: prove the 23rd-slot runtime consumer
  (`(&DAT_0014BF60)[param_1]`, `param_1` 0/0xB; owner `FUN_00048DC0`). Wire it
  or record the negative proof; a "no static xref" result alone is not enough.

### B10 — FU348-L2c/L2d and RR-L9

- FU348-L2c: establish the actual `0x105284`/`0x105298` producer and the
  engine option-cell contract; do not assume `FUN_0001CAEC` is the writer
  (the record only proves reads there).
- FU348-L2d: the non-default A4 branch through `FUN_000481EC` (`0x4AE76`),
  `FUN_00047298`, and the `0x47769` branch, with non-default `0x16`/`0x17`
  selector fixtures.
- RR-L9: port the `0x15887B` producer chain (`FUN_0008BAF0` → `FUN_0006FE84`
  → `FUN_0006E518`, `0x6E538`) and wire the ball-frame byte only after its
  live reach is proved.

### B11 — closure and gate

Append the closure records (FU72/FU-351/FU-348/FU158/FU108/FU111/FU-350,
ENGINE, PROGRESS), recompute the three completion numbers, run `make check`,
the M1/M2 `cmp`, and the windowed smoke. State explicitly that the four
non-Slice-B `0x8ED40` sites remain carried. Any moved golden is handled per
AGENTS.md and §9.

## 7. Tests and gates

- New loader tests in `port/tests/test_camera.c`; engine tests in
  `test_engine_match_handlers.c`, `test_engine_match_frame.c`,
  `test_engine_replay_ring.c`, `test_engine_match_bridge.c`, and the relevant
  row tests.
- Every test asserts a native value/delta from the census table, not a
  hand-picked number. A site proved unreachable gets a gate test and a
  documented dead-arm note, not a fabricated expectation.
- `make check` stays green (125/125 plus new tests); M1 stays byte-exact; M2
  is compared after each behavioral landing and moves only per §9; smoke exit
  124.
- Documentation is append-only; each workstream appends its evidence section
  and quotes the leg it closes or narrows.

## 8. Risks and mitigations

- **M2 movement.** Completing the armer head and the walk jitter/pause bridge
  may change live camera behavior on the M2 natural tape. Mitigation: land
  the risky gaps last in B1, compare M2 immediately, and stop for the §9
  per-incident ruling if it moves.
- **Dead paths.** Only proved unreachability produces a dead record;
  otherwise the site stays a numbered open leg. No test asserts an
  unreachable value.
- **`FUN_0004B100` remains unported.** The spine runs at the engine's
  existing live-frame point; the design does not claim the whole chain.
- **Entity-kind resolution.** `BALL_SUB7A6`, `REC_15B43C` and `REC_157A9B`
  may have no engine cell. Mitigation: the census resolves them first; an
  unresolved kind is a numbered leg, not a fabricated id.
- **B2-L1 stats corruption.** A wrong flip moves the stats block. Mitigation:
  prove the mapping and the re-key before wiring; keep `side_swap = 0` until
  then.
- **RR-L9 indirect writer.** The writer chain is proved; the producer's live
  reach must be proved before the byte is wired.

## 9. Operator decisions (2026-11-12)

- **M2 movement is ruled per incident.** The default is to stage the new walk
  so M2 stays byte-exact; a re-pin happens only after the frame diff exists
  and the operator approves that specific incident. A slice-wide standing
  authority is not assumed.
- **A census-proven unreachable site is ported to its gate, gets the
  gate/dead-path test, and is recorded dead with the proof.** A site whose
  unreachability is not proved stays a numbered open leg. This follows the
  operator's full-campaign choice (option 3).

## 10. Post-review corrections (2026-11-12)

The plan review found six design-level corrections. Quoted superseded text is
corrected here; §5/§6 keep the old wording as the record.

- **`site` width.** The event's `site` is `uint32_t`, not `uint16_t`:
  `0x70F44`, `0x72DE3` and `0x8AD54` all exceed `0xFFFF`.
- **`FUN_00072478` ownership.** The event-array contract is for the loader
  bodies (`fifa96_camera_reposition`, `fifa96_camera_save_tail`).
  `FUN_00072478` is engine-owned and calls `fifa96_match_run_stats_add`
  directly; its ordered `+0x32`/`-0x4B` pair is not a loader event array.
- **Armer head.** `[0x15781C] == 0` skips aging (`0x7132B TEST/JZ 0x7134F`);
  a nonzero counter adds `frame_delta` as a byte, and resets only when the
  result is strictly `> 0x1E`. `FUN_00070DE0` reseeds the cell at `0x71231`
  under its boundary arm; the port must model that or carry it as a leg.
- **`FUN_00070DE0` site mapping.** `0x70F44` is the `local_30 > 0x90` arm;
  `0x70FCE` is the `local_30 <= 0x90` arm only when
  `local_34 < -0xC0 || local_34 > 0xC1`. They are not a plain if/else pair,
  and the existing bit-0x8-clear fixture reaches neither.
- **Save tail and frame order.** The loader tail starts at `0x72AEA` (the
  post-call jump), not `0x72AE5`. Native `0x73723 CALL 0x92864` and the
  `[0x157825]/[0x157827]/[0x157829]` shadow stores sit between the jitter and
  `FUN_00072AC4`; the port must include them or record a numbered leg.
- **Row-1C, case 6 and closure.** The row-1C tail has three separate stat
  routes (phase-0x10 negative product; phase-non-0x10 nonnegative product,
  optional tracked `+5`; phase-non-0x10 negative product `+5`), not one
  four-stat sequence. The `FUN_0008A938` arm is in the direct goal fallback,
  not `fifa96_match_run_situation(6)`; `[0x157A9B]`'s writers are
  `FUN_00088940` at `0x8895B/0x88A1C/0x88AA4`, and an unproved producer stays
  an open leg. The closure reports the actual wired count, not necessarily 19.

## 11. Replay-chain corrections (2026-11-13)

The B7–B8 plan review and fresh `/FIFA96.EXE` reads correct the §B7/§B8
wording below. The §B7/§B8 text is left verbatim as the record.

- **RR-L2 ownership.** `[0x14E574]` has 35 xrefs across eight functions
  (`FUN_000513EC`, `FUN_000537F8`, `FUN_00053DC4`, `FUN_00053DE0`,
  `FUN_00052030`, `FUN_00053A48`, `FUN_00052D68`, `FUN_00053BB8`); it is a
  shared overlay-cluster latch, not owned by `FUN_00053DC4`. The
  `FUN_00053DC4`/`FUN_00053A48` pair is the recorder disable/re-enable edge:
  `0x53DD3 CALL 0x64074` clears `[0x109A90]` and sets the latch; the
  state-5→6 arm (`0x53A8E`) calls `0x64080` and clears the latch. The packer
  gate is `0x640A2 CMP [0x109A90],0`. The engine's live call site/state
  machine stays a numbered leg until the overlay state is ported.
- **Ring reset.** `FUN_00064030` writes only the three cursors, the playback
  index `[0x109A94]` and the accumulator `[0x109AAC]`; it does **not** write
  the recorder gate `[0x109A90]` or the mode budget `[0x107310]`. A
  gate-preserving reset is correct; the engine's budget zeroing is an adapter,
  and the `fifa96_match_replay_ring.h` comment that attributes both to the
  native reset is corrected.
- **RR-L3 is three bytes plus the queue6 lane.** `0x155C50` is the focus band
  from `FUN_000627F4(focus_x, focus_y)` (x>0x46→1, x<-0x46→4, y<0x6A→+8,
  y>0x35→+2, all +1); `0x155C51` is the band flag; `0x155C4F` is the FIFO
  pop (`FUN_00064EE4` = pop, returns `value+1`). The six-byte queue6
  `0x155C6A..0x155C6F` is a separate lane: native initializes it to `0xFF`
  and overlays the four `0x157C64` objects' `+0x8D` bytes (plus `0x0B` when
  the object's team byte matches `[0x157ABF]`) and the `FUN_0004BA44`/
  `FUN_0004BA00` bytes. The band/flag/pop gate is `[0x15781C] != 0` and
  `(0x155C4E & 0x40)==0`; the queue-pop is outside that gate. The
  `0x157C64` object producer remains a numbered leg.
- **RR-L5 identity is partial.** `[0x7DC8]` points into a seven-object pool
  (POSE `0x107968`, VIEW `0x107CE8` ×4, ACTION `0x107B98`, BALL `0x107B28`,
  plus the base camera records). The port models the three VIEW classes and
  ACTION; POSE is fed from the recorded matrix and BALL from the ball
  position. The selector index→base table is exact, but the `camera_sub`
  value has no consumer and BALL has no dedicated source. The engine's
  `native_view_cam`/`native_view_valid` arrays currently hold three entries;
  the plan widens them to four so sub 8 (index 6) maps to class 3 without an
  out-of-bounds read. The closure either implements the source selection or
  keeps explicit RR-L5 residuals.
- **RR-L7 request chain.** The request source is `FUN_00051270`/
  `FUN_000512B0` → `FUN_000510DC` → `FUN_00064270`; `FUN_0004B100` is the
  live-frame chain and is **not** the request source. The arm writes
  `[0x14E58C]=1` and calls `FUN_00064270`; the per-frame arm drives
  `[0x108E0C]`/`[0x108E10]` and ramps `[0x14E58C]` through `0xF0`/`0x168`.
- **Timed exit is conditional.** The ramp alone does not exit: `FUN_00064E74`
  returns `cursor >= count` and gates the exit countdown at `0x511DC`;
  `FUN_00064DFC` sets state `0x83` and advances playback while records remain.
  With an empty/one-record ring the native can exit right after the mode
  window; with records, the cursor must advance. The plan either maps
  `FUN_00064DFC` to the existing `replay_step` PLAYING path or records it as
  a leg, and does not claim a universal "after both thresholds" exit.
- **C4R-L1 is presentation-only except the flag.** `[0x109014]=0` is the
  projection-active flag (the image default 0; `FUN_000585E4` writes 1 as the
  setup writer). `FUN_00056B88(0x2B)`, the `0x14E9B8 >= 0x80` /3 loop and
  `FUN_00056894` are projection-transform bodies, not audio. The engine
  models the flag (init 0, arm 0) and records the three bodies as legs.
- **Appearance notation.** The native appearance is
  `**(byte **)(rec+0x28)` (one pointer load, one byte load); the ball is
  `*[0x158866]`. The earlier `***(byte**)(rec+0x28)` in the plan review focus
  had one extra dereference.
