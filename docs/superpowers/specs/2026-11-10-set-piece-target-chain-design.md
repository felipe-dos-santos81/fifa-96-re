# FIFA 96 — set-piece target chain design

## Intent

Close the remaining set-piece target chain in `FUN_00083428` / `FUN_00085498` so
the native three-point delivery path drives the throw/corner target instead of
the derived timer/draw stand-in. The camera spine and the delivery-code tail are
already live; this is the last live set-piece producer that still stages a
derived target.

## Why now

The `0x83937` receiver cut and the delivery returns `1/6/4/3/5/2` are wired
live-only. The native machine's `0x158780` sub-index interpolation, the
`FUN_0006DAB4` path builder, and the `0x157A6F` camera-focus consumer are the
only pieces still missing. Without them the live set-piece path has no native
target lane and no visible focus.

## Goals

- Port `FUN_0006DAB4` (the three-point path builder) exactly.
- Port the `0x158780` / `0x15878C` / `0x158786` / `0x158788` interpolation and
  the `0x158767/6B/6F` target triple.
- Port the rest of the input tail that feeds the path: the `0x15877B._3` /
  `0x15877F` direction bytes, the `0x1587B5` toggle, and the `0x1587B0..B6`
  state.
- Stage the native slot-5 `sp` table (`352sp.fmt` / `442sp.fmt` / `swsp.fmt` /
  `424sp.fmt` / `433sp.fmt`) from the existing `GAMEART0.PVI` bank.
- Reuse `fifa96_match_pitch_cell` (the existing `FUN_0006D870` port) for the
  path cell; do not add a third cache.
- Wire the native `0x157A6F` camera-focus override into the live camera follow.
- Keep M1/M2 byte-exact and `make check` green.

## Non-goals

- The non-goal `FUN_0008ED40` activity updates (35 sites).
- The `0x515A6` / `0x3BB1C` replay UI trigger.
- The full `FUN_00047298` 23-descriptor pool.
- Row-29's live entry (no entry exists).
- The `FUN_0004F274` / `0x505D0` config-record path (superseded).
- The `0x157770` second camera vector (`0x1577E6/EA`); only the camera pair is
  in scope.

## Anti-goals

- Do not move the M1/M2 goldens.
- Do not invent a path table or hard-code a derived table as if it were native.
- Do not make the native path run on the characterization tape.
- Do not change the camera target-source or delivery-code semantics already
  closed. Adding the `0x157A6F` focus override is the native consumer, not a
  redefinition of those sources.

## Constraints

- Every address/byte claim cites a Ghidra read in the slice record.
- Unproven behavior becomes a numbered leg, never an asserted value.
- The native path is live-only; M2 keeps the derived timer/draw arm.
- ISO-gated tests skip cleanly when the ISO is absent; no tool writes the ISO.
- Strict build (`-Wall -Wextra -Werror`) and ASan/UBSan for engine tests.

## Current state (evidence read this session)

- `FUN_00083428` is the input-driven machine; `FUN_00085498` is its probe. The
  delivery tail is `match_sp_delivery_step`; the receiver cut is
  `match_sp_receiver_cut`.
- `FUN_0006DAB4` (`0x6DAB4`, fresh decompile) writes `[team+0x7E7] = 0`, then
  `[team+0x7E8] = DAT_0015774C`, `[team+0x7F0] = DAT_00157754`; for direction
  `param_2` in 0..3 and a valid cell index it writes the two control points from
  `[team+0x7DF] + (cell*6 + dir*2)*2` as `[+0x7F4]/[+0x7FC]` and
  `[+0x800]/[+0x808]` (`byte * 0x26` / `byte * 0x21`), negates both for side 1,
  and sets `[team+0x7E7] = 1`.
- `FUN_00083428` calls `FUN_0006DAB4` at `0x83441` (head, previous frame's
  direction), `0x839BC` (receiver cut) and `0x83AF1` (`0x40` return);
  `FUN_00085498` calls it at `0x85572` (no-slot timer/draw arm). Fresh
  `get_function_xrefs 0x6DAB4` = exactly those four sites.
- `FUN_000832A8` does **not** call `FUN_0006DAB4`; it seeds
  `0x15877B._3 = 3`, `0x15877F = 3`, `0x1587B5 = 0`.
- The interpolation guard is `DAT_001587B5 == 0` and direction 0..2;
  `DAT_00158780` is the sub-index, `DAT_0015878C` the accumulator,
  `DAT_00158788` holds the duration (high word) and z velocity (low word),
  `DAT_00158786` the x velocity. The target is `0x158767/6B/6F`.
- `LAB_000834c3` sets `0x158780 = 1` and falls into the entry-1 load on the
  same call when the sub-index passes 2; it does not wait a frame at 0.
- `FUN_0008DCD4` is the distance/delta metric; `FUN_000702F8` maps
  `metric >> 4` through the static byte table at `0x10F4EE`; the height table is
  the 149-word table at `0x10F3C4`. `FUN_0006FFC0` sets
  `DAT_00157744 = 0x10F3C4` and `DAT_00157748 = 0x10F4EE`; both are static
  image data.
- `FUN_0006D9C4` sets `[team+0x7DB] = FUN_0004C384(form*6+3)` and
  `[team+0x7DF] = FUN_0004C384(form*6+5)`. The `0x107370` name table gives the
  five slot-5 names `352sp.fmt`, `442sp.fmt`, `swsp.fmt`, `424sp.fmt`,
  `433sp.fmt`; the retail entries are 3588 bytes = 299 × 12, matching the
  `0x12A` cell ceiling.
- `FUN_0006D870` maps a world position to `{row, col, index}` with
  `row = (z + 0xC1E) / 0x10E` clamped 0..0x16, `col = (x + 0x86B) / 0x14B`
  clamped 0..0xC, `index = row*0xD + col`; side 1 is the full mirror
  `0x12A - index` (`col` mirror `0xC - col`). The engine already ports this as
  `fifa96_match_pitch_cell`; it is used by the phase-04 cell path.
- `0x157A6F` is a **pointer to the camera triple** `0x158767`
  (`0x83471 MOV dword[0x157A6F], 0x158767`); the gate-fail arm
  (`0x83629..0x8363D`) copies the FU-71 triple into `0x158767` and clears the
  pointer. `FUN_00036C70` (`0x36C76..0x36CFA`) uses it as the live focus when
  set and the replay bit is clear.
- The engine's `camera_override_157a6f` is reset-only; `match_run_live_camera_follow`
  currently uses `cam->pos_*`. This is the missing consumer (FU154 R2-L2).

## Approaches

### A. Full native path (chosen)

Load the slot-5 `sp` file from `GAMEART0.PVI`, port `FUN_0006DAB4`, reuse
`fifa96_match_pitch_cell`, port the `0x158780` interpolation and the input tail,
and wire the `0x157A6F` focus override.

Trades a new asset load and a live focus path for full fidelity. The table and
arithmetic are already visible in the image, and the engine reads the same bank
for the placement file.

### B. Derived path from the slot-0 placement table

Reuse the existing placement pairs to synthesize the three points. No new asset
load, but not native, leaves the real `sp` table unused, and still needs the
interpolation. Rejected: it does not close the item.

### C. Document the leg

Record the table and arithmetic as an open leg. Rejected: the evidence is
complete enough to port.

## Design

### 1. Grid cell (reuse the existing port)

Use `fifa96_match_pitch_cell(focus_x, focus_z, side)` directly. It already
implements `FUN_0006D870`'s bins, clamps and the `0x12A - cell` mirror. Do not
add a camera copy and do not touch `phase_cell_side` / `phase_dir_default`
(the M2-staged cells). The set-piece path is live-only, so reading the live cell
does not move the tapes.

### 2. Path table

Add to `fifa96_match_team`:

- `uint8_t sp_table[3588]; uint32_t sp_table_len;` (a fixed per-team copy; the
  asset buffer is freed after staging, and the run owns no extra heap block).
- `uint8_t sp_table_ready;`

Add `match_run_formation_sp_seed_team(mr, team, id)`:

- read `MATCH_RUN_FORMATION_BANK` (`/ART/GAMEART0.PVI`) through
  `fifa96_asset_read`;
- append `.fmt` to the five-entry name table (`352sp`, `442sp`, `swsp`,
  `424sp`, `433sp`);
- copy the entry bytes (3588 expected) into the team's `sp_table` and set the
  length. Bound every read by `sp_table_len`; a short or missing entry leaves
  `sp_table_ready = 0` (no path) rather than a synthetic table.

Seed both teams at match begin. Reseed a team when its formation id changes or
`sub_rebuild` changes the lineup; a missing bank entry clears the table and is
recorded as a leg (not a crash).

### 3. Path builder (`FUN_0006DAB4`)

Add to `fifa96_outfield`:

```c
void fifa96_sp_build_path(int32_t focus_x, int32_t focus_z,
                          const uint8_t *table, uint32_t table_len,
                          int32_t cell_index, int32_t dir, uint8_t side,
                          int32_t path[3][2], uint8_t *valid);
```

Exact behavior:

- `valid = 0` when `dir < 0 || dir > 3`, `cell_index < 0 || cell_index > 0x12A`,
  the table is NULL, or `offset + 4 > table_len`.
- `path[0] = {focus_x, focus_z}`.
- `offset = cell_index * 12 + dir * 4` (the native
  `(cell*6 + dir*2)*2`).
- `path[1] = {(int8_t)table[offset] * 0x26, (int8_t)table[offset+1] * 0x21}`;
  `path[2] = {(int8_t)table[offset+2] * 0x26, (int8_t)table[offset+3] * 0x21}`.
- side 1 negates both x and z of `path[1]`/`path[2]`.
- The last cell (`0x12A`) with `dir == 3` is out of bounds by the native
  arithmetic (offset `0xE04`, +4 > 3588); the guard returns `valid = 0`. This is
  the documented safety divergence, not a stride error.

### 4. Interpolation (`0x158780`)

Add to `fifa96_match_run`:

```c
int32_t sp_path[3][2];
uint8_t sp_path_valid;
uint8_t sp_sub_index;      /* 0x158780 */
int16_t sp_accum;          /* 0x15878C */
int16_t sp_duration;       /* 0x158788 high */
int16_t sp_vel_x;          /* 0x158786 */
int16_t sp_vel_z;          /* 0x158788 low */
int32_t sp_target[3];      /* 0x158767/6B/6F */
uint8_t sp_dir_15877b;     /* 0x15877B._3 */
uint8_t sp_dir_15877f;     /* 0x15877F */
uint8_t sp_b5_1587b5;      /* 0x1587B5 */
```

Add `fifa96_sp_target_step(mr, delta)`:

- Guard fails (`sp_b5_1587b5 != 0` or `sp_dir_15877b > 2`): reset the staging
  and leave the target at the focus.
- `sp_sub_index == 0`: `sp_target = focus` (`0x15774C/50/54`),
  `sp_sub_index = 1`, load entry 1.
- `sp_sub_index != 0`: `sp_accum += delta`; if `2*duration < sp_accum`,
  `sp_sub_index++`; if `sp_sub_index > 2`, set `sp_sub_index = 1` and load
  entry 1 **on the same call** (native `LAB_000834c3`); else load entry
  `sp_sub_index`.
- Load entry `i`:
  - `dx = (int16_t)(path[i].x - sp_target[0])`,
    `dz = (int16_t)(path[i].z - sp_target[2])`; the native `FUN_0008DCD4`
    subtracts the low 16-bit words. `fifa96_arm_dist_stage` does not expose dx
    (its `out_lane` is dz only), so the step computes dx directly and uses
    `fifa96_entity_distance(dx, dz)` for the metric (the same octagonal body).
  - `duration = sp_duration_from_metric(metric >> 4)`:
    - `> 0x63F` -> `0x94`;
    - `< 1` -> `3`;
    - else `s = 0x94 - 0x10F4EE[(0x640 - v) >> 2]`; `s < 0 ? 0 : s`.
    - `duration == 0 ? 8 : duration`.
  - `vel_x = dx / (2*duration)`, `vel_z = dz / (2*duration)` (signed 16-bit
    division, native `IDIV`).
  - `sp_accum = 0`.
- Integrate every call: `sp_target[0] += sp_vel_x * delta`;
  `sp_target[2] += sp_vel_z * delta`; `i = |duration - accum|`;
  `sp_target[1] = 0x10F3C4[0x94 - i] - 0x10F3C4[0x94 - duration]`; clamp
  negative to 0.

Copy the height/duration tables as static `int16_t`/`uint8_t` arrays from the
image (`0x10F3C4` 0x12A bytes, `0x10F4EE` 0x190 bytes), with a comment naming
the source. They are image data, so this is a direct port, not a derived table.

### 5. Input tail

Extend `match_sp_delivery_step` to write the native direction state:

- `pressed & 0x40` clears `sp_b3_1587b3`.
- `state.phase != 8 && != 9 && (prev_mapped & 0x40)` and any of raw bits
  `8/1/4/2` sets `sp_b3_1587b3 = 1`.
- `0x1587B5` toggles on `pressed & 0xB0` (native `0x8369A`).
- The raw direction bits set `sp_dir_15877b` / `sp_dir_15877f` to 3/2/1/0 and
  clear the low nibble of `[param_2+0x10]` (`0x836C4..0x837xx`).
- `FUN_000832A8` seeds `sp_dir_15877b = 3`, `sp_dir_15877f = 3`,
  `sp_b5_1587b5 = 0`.
- The `0x40` return path calls `FUN_0006DAB4` with `sp_dir_15877f` before it
  resets the sub-index; the cut path calls it with the current direction.

### 6. Wiring

- `FUN_000832A8` (`match_sp_setup_832a8`): seed the direction bytes and `b5`;
  do not build the path here.
- `FUN_00083428` head (`0x83441`): call `fifa96_sp_build_path` with the previous
  frame's `sp_dir_15877b`, the camera focus, the team `sp_table`, the
  `fifa96_match_pitch_cell` cell for the team side, and the team side. Store the
  path and `sp_path_valid`.
- `0x839BC` (receiver cut) and `0x83AF1` (`0x40` return): rebuild with
  `sp_dir_15877f` at the same branches.
- `0x85572` (no-slot timer/draw arm): store the draw remainder in the team
  `+0x7E7` byte, build the path with that direction, and keep the existing
  `sp_delivery` draw.
- `match_sp_probe_85498` (live slot and no-slot arms): call
  `fifa96_sp_target_step(mr, delta)` once per granted frame before the delivery
  step.
- The delivery tail stays `match_sp_delivery_step`. The `0x1587A8` / `0x1587AC`
  staging copies and the `0x157AA7` focus handoff remain the existing engine
  model; any branch that writes a path uses the builder, otherwise the previous
  path is kept.
- Live-only: `mr->live_start` gates the native path; the non-live M2 tape keeps
  the derived timer/draw arm.

### 7. Camera consumer (`0x157A6F`)

- Change `camera_override_157a6f` from a reset-only scalar to a pointer-like
  flag plus `camera_override_triple[3]` (or reuse `sp_target` with a flag).
- `FUN_00083428` sets the override when the guard passes (`0x83471`); the
  gate-fail arm copies the FU-71 focus into the triple and clears it
  (`0x83629..0x8363D`).
- `match_run_live_camera_follow` uses the override triple when set; otherwise
  it keeps the current `cam->pos_*` focus.
- `fifa96_camera_focus_clamp` still applies the native ±6000/0..16000 bounds.
- Test: set-piece path active -> the live focus is the interpolated triple; the
  override cleared -> the FU-71 place returns.

If this consumer is scoped out, the interpolated triple is internal state and
the visible outcome is missing; that is the one open scope choice.

### 8. Sinks

- `FUN_000786A0` is the record-list move: it links the picked receiver into
  the taker's `+0x20` list and clears the receiver's control slots. The engine's
  `has_slot` is the list-head flag already used by `fifa96_sp_pick_receiver`;
  the cut sets `picked.has_slot = 1` and clears the taker's link if the engine
  models one. If the existing `fifa96_action_duel_swap`/`has_slot` model cannot
  represent the move without inventing fields, record a numbered leg and keep
  the visible behavior (the receiver is excluded from the next pick).
- `FUN_0004C374` is exactly `[0x157A68] = 0`; `FUN_000361A4` is exactly
  `[0x105FB0] = 0`. Search the engine cells; if either has no home, add a
  documented reset byte to the run and write 0 at the cut, naming the native
  address.

### 9. Tests

- `test_outfield`:
  - path builder: synthetic 299×12 table, dirs 0..3, side 0/1, signed bytes,
    last-cell/dir-3 guard, short-table/invalid-cell guards, NULL table;
  - interpolation: sub 0 -> 1 -> 2 -> 1 reload, accumulator crossing, duration
    clamps, y table arc, velocity division.
- `test_engine_match_handlers`:
  - live setup seeds the direction and the head build; the probe advances
    `sp_target`; the delivery codes and receiver cut still work;
  - the non-live fixture never enters the native path;
  - the no-slot draw path builds with the drawn direction.
- ISO-gated engine test:
  - load the real `352sp.fmt` / `442sp.fmt` entries from `GAMEART0.PVI`;
    assert 3588 bytes and a known cell/dir four-byte read;
  - skip cleanly without the ISO.
- Camera test: the `0x157A6F` override drives the live focus; clear restores
  the FU-71 place.
- Gate: `make check` 124/124, M1/M2 `cmp`, windowed smoke, docs.

### 10. Docs

- `port/spec/FU275_taker_arm_residuals_2.md` §3: implementation note and the
  remaining sink legs.
- `port/spec/FU-351_class3_camera.md` §22: update the `0x83937` row and the
  remaining set-piece sentence.
- `port/spec/FU154_live_camera_place_ball_visibility.md`: close R2-L2 if the
  consumer is in scope.
- `port/ENGINE.md`: the set-piece status and the `FUN_0006DAB4`/`0x158780`
  closure.
- `docs/PROGRESS.md`: phase 112 narrative.
- Plan/ledger under `docs/superpowers/plans/` and `.superpowers/sdd/`.

## Decided

- Full native path (A), not the derived shortcut.
- Live-only boundary, so M1/M2 stay byte-exact.
- The `sp` table is loaded from the existing `GAMEART0.PVI` bank and bounded;
  the path builder rejects the last-cell/dir-3 read.
- The height/duration tables are static image copies.
- The grid cell reuses `fifa96_match_pitch_cell`; no third cache.
- The path build belongs to `FUN_00083428`'s head/branches, not `FUN_000832A8`.
- The sub-index reset is 1 + entry-1 load on the same call.
- The input tail (direction bytes, `b5`) is in scope.
- The no-slot `0x85572` path build is in scope.
- The `0x157A6F` camera consumer is recommended in scope.

## Left to the builder

- The exact `sp` table allocation strategy (fixed buffer vs heap copy) and the
  maximum size guard.
- Whether the existing `has_slot`/`fifa96_action_duel_swap` model can express
  the `FUN_000786A0` list move, or whether that becomes a numbered leg.
- The engine homes for `0x157A68` and `0x105FB0`; if absent, the reset-byte
  names and placement.
- The precise live call order between the path step, the delivery step and the
  receiver cut, once the native branch order is re-read against the ported
  probe.

## Risks

- The `sp` file may be absent or short under a different bank name; a missing
  entry becomes a numbered leg, not a synthetic table.
- The `fifa96_match_pitch_cell` side mirror is the full `0x12A - cell`; a wrong
  side would negate the path.
- The `has_slot` field may be the list head but not the full `+0x20` list
  linkage; a partial sink is a leg, not a reason to break the cut.
- The camera override touches the live focus; the test must prove the non-set
  frames still read the FU-71 place.
