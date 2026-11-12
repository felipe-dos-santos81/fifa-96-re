# Recorded-frame replay ring — design

Date: 2026-11-12
Status: implemented
Campaign: close the recorded-frame replay ring, the first of the three
remaining engine-level blocked items (ring, 19-site `0x8ED40` campaign,
`FUN_0003BB1C` UI loop).

## 1. Intent and why

The live replay trigger, state machine, camera selector, overlay and the
live-match freeze are ported. What the replay plays is still the **frozen
present**: `fifa96_match_run_frame` stops the live body and draws the current
entities while the state machine advances. The native replay plays the
**recorded past**: every recordable live frame is composed into the `0x155AA4`
block and packed into a 150-slot `0x118`-byte ring; playback applies a record
back into the block and the renderer draws that block.

This slice makes the port's replay a true recorded replay. It closes the
FU-108/FU-109/FU-112 core (buffer, cursors, record format, composer) and
unblocks the mode-7 request/timed exit (`FUN_0004B100` → `FUN_000510DC` →
`FUN_00064270`), which currently cannot be built without the ring cursors and
stamps.

## 2. Goals

- Model the native `0x155AA4` view block and the `0x118` record exactly
  (offset-for-offset), with a packer and applier that are exact inverses.
- Capture the composed frame into a 150-slot ring at the native packer
  cadence and under the native mode-window budget, while the recorder gate is
  enabled.
- Play the ring back through the native state transitions: `0x81` arm
  (apply record 0), `0x82` idle/display, `0x83` active countdown, `0x84`
  forward double-step, `0x85` back double-step, `0x86` slow, exit.
- Render the applied view (22 player slots + the ball + camera pose) instead
  of the frozen live pool; the live pool is never written by playback.
- Keep the existing HUD/overlay, the live freeze and the audio duck.
- Test every cursor/stamp/edge case; no golden movement.

## 3. Non-goals and anti-goals

Non-goals:

- The mode-7 request cells and timed exit (`FUN_0004B100`/`FUN_000510DC`).
  The ring makes them buildable; they stay a follow-on slice.
- The full `0x20` replay camera record cycle. The existing selector stays;
  the recorded matrix drives the default pose camera, the other cameras stay
  on the existing source-4 path (RR-L5).
- The native allocator/descriptor (`FUN_0004A448`, `&0xA41C`,
  `FUN_000993EC`) and the presentation-overlay recorder gating
  (`FUN_00053DC4`/`FUN_00053A48`, `[0x14E574]`) (RR-L1/RR-L2).
- The 19-site `0x8ED40` campaign and the `FUN_0003BB1C` UI loop.
- The team-editor preview.

Anti-goals (the slice fails even if tests pass):

- Playback overwrites the live entity/ball pool, or the post-replay match
  differs from a no-replay run beyond the recorded view.
- A record whose layout is not the native `0x118` map, or a pack/apply pair
  that is not a byte-exact inverse (the `+0x116` stamp is the one metadata
  lane that is not a view→record→view round trip; see §5.1/§7.4).
- Playback advancing on wall time instead of the record stamp, or a replay
  that can end the match (`mark_over` must stay deferred until exit).
- Moving M1/M2 goldens, changing a completion number, or adding a dispatch
  body/row.

## 4. Constraints

- C11, `-Wall -Wextra -Werror`, ASan/UBSan under the engine tests.
- `make check` 124/124; M1/M2 `cmp` byte-exact; windowed smoke exit 124.
- Work on `main`, one slice per spec/plan; commit and push per slice
  (AGENTS.md).
- Every address/byte claim cites a fresh Ghidra read from this design
  session; anything unproved is a numbered leg, never asserted.
- The ring is embedded in the run as a fixed 150-slot array (150 × 0x118 =
  42,000 bytes). The run grows from ~26 KB to ~68 KB; the suite's stack
  fixtures remain single-run, and the embedded buffer avoids an allocator
  lifecycle/leak surface. The native allocator stays RR-L1.

## 5. Evidence (fresh, `/FIFA96.EXE`)

### 5.1 The record map (fresh disassembly, corrected)

Fresh `disassemble_function 0x6408C` (packer) and `0x63B80` (applier). The
record is `0x118` bytes. Both loops run 23 iterations (`EAX` advances 2 bytes
until `record+0x2E`, `EDX` advances 0xC, `EDI` 4, `ESI`/`EBX` 1). The packer
pre-increments the view/record byte pointers, so the byte lanes are `+0xC4`
and `+0xDB` (not `+0xC3`/`+0xDA`); the applier pre-increments identically.

| record | view block (`0x155AA4`) | width |
|---|---|---|
| `+0x00/+0x04/+0x08` | `+0x00/+0x04/+0x08` (focus triple) | dword ×3 |
| `+0x0C+2i` | `+0x0C+0xCi` (x) | word (apply sign-extends) |
| `+0x3A+2i` | `+0x14+0xCi` (z) | word (apply sign-extends) |
| `+0x68+2i` | `+0x10+0xCi` (y) | word (apply sign-extends) |
| `+0x96+2i` | `+0x120+4i` (packed heading) | word (apply zero-extends) |
| `+0xC4+i` | `+0x17C+i` = `0x155C20+i` (appearance) | byte |
| `+0xDB+i` | `+0x193+i` = `0x155C37+i` (`+0x3D` frame) | byte |
| `+0xF2` | `+0x1AA` = `0x155C4E` (roll byte) | byte |
| `+0xF3` | `+0x1AB` = `0x155C4F` (queue pop) | byte |
| `+0xF4` | `+0x1AC` = `0x155C50` (camera band) | byte |
| `+0xF5` | `+0x1AD` = `0x155C51` (band flag) | byte |
| `+0xF6..+0x10D` | `+0x1AE..+0x1C5` (camera matrix) | dword ×6 |
| `+0x10E..+0x113` | `+0x1C6..+0x1CB` (queue6) | byte ×6 |
| `+0x114` | `+0x1CC` (`word_55C70`) | word |
| `+0x116` | `[0x109AAC]` stamp (pack) → `+0x1CE` (`word_55C72`, apply) | word |

The `+0x116` lane is asymmetric by design: the packer writes the accumulator,
the applier writes it into the view's `+0x1CE` (`0x155C72`). The view's
pre-apply `+0x1CE` value is the composer's `FUN_000492CC` word and is not
restored. All other lanes are byte-exact inverses; the byte lanes are
contiguous (23 appearance bytes at `+0xC4..+0xDA`, 23 frame bytes at
`+0xDB..+0xF1`), which is the layout check.

### 5.2 The live block is the composer's 23-slot snapshot

FU-112 §1 maps `FUN_00036C70`: the **focus triple** at `0x155AA4/8/C` from
`[0x57A6F]` (override) or `0x15774C` (the FU-71 place), clamped
`|x|,|z| <= 6000` else `(-4000,0,-4000)` and `0 <= y <= 16000`; 22 team
records (each with the `+0x59/+0x5D/+0x61` position, `+0x7D` heading,
`+0x28` appearance and `+0x3D` frame) + the ball record at slot 22
(`0x15880C/10/14` position, `0x15885A` heading, `0x158866` appearance,
`0x15887B` frame), and the camera/input cluster at `0x155C4C..0x155C72`. FU-112's "ball x/y/z" label for the first
triple is an erratum: the source is the camera focus, not the ball record.
FU-112's "its `0x55C37+0x16` byte is written 0" is also an erratum: the
composer copies `[0x15887B]` (FU-158 §2, fresh `0x36E38`). The port
already models the focus as `render_focus_*` (FU-351-L1) and composes the same
23 slot fields in `match_run_scene_stage_positions`; the ring composer copies
those **staged render slots** (the values the renderer draws) plus the camera
and focus state into the view. It runs after the scene staging, inside the
render-enabled branch; a headless run (`render.enabled == 0`) stages no slots
and is not a meaningful capture path. The native composer reads the live
model, but the port's staged slots are its composer output and are the
faithful source for "what was drawn".

The composer's packed heading is `((0x400 - face7d) & 0x3FF) << 6` (FU-112).
The port's `render.entities[].stage.heading` is the raw high word
(`face7d << 16`) and `fifa96_render_slot_stage` folds it at draw time, so the
ring stores the **native folded value** and the replay staging unfolds it
back (or the stage path gains a native-angle source); a direct copy would
fold twice.

### 5.3 The recorder and its mode-window budget

- `FUN_0006408C(ticks)` is the **packer**, called from the live frame event
  dispatch `FUN_00049B28` at `0x4A040` with `EAX = 1`, right after the
  composer call at `0x49FF5`. With `ticks == 0` it sets `[0x109AAC] = 1` and
  **writes a record immediately** (a flush, stamp 1); with `ticks != 0` it
  adds and returns while the accumulator is `< 2`. On a write it uses the
  write cursor, wraps at 150, evicts when the count exceeds 0x95, and stamps
  `+0x116 = [0x109AAC]`.
- **The 90-frame mode-window budget (FU-111 §1, fresh re-read).** The packer
  call sits inside `if ([0x107310] < 0x5A)`. Each frame, `FUN_0004B380()`
  returns the mode/phase (`[0x157A4A] >> 24` = byte `[0x157A4D]`; the port's
  `state.phase`): modes `{0,1,3..8,0xA..0xC,0x13,0x14}` increment
  `[0x107310]`; modes `{2,9,0xD..0x12,>=0x15}` set it to 0. The reset modes
  **do not pause recording** — they reset the counter to 0, and the packer then
  runs because `0 < 0x5A`. The counter is incremented **before** the gate, so
  an uninterrupted increment-mode run yields at most **44** records (packs at
  frames 2,4,..,88; the 90th frame's pack is blocked). A reset mode clears the
  budget and packing resumes immediately; the ring fills across windows, not
  as a continuous 300-frame window.
- `FUN_00063CBC` is the **playback forward step** (apply at
  `(read + cursor) mod 150`, `cursor++`, mode `0x82` when
  `cursor >= count`), not the append. FU-108 §2's "append — `FUN_00063CBC`"
  label is an erratum.
- `FUN_00063E54` steps back; `FUN_00063D34` reads the current record's
  `+0x116` stamp (0 when `cursor >= count`); `FUN_00064030` resets the
  cursors; `FUN_00064074`/`FUN_00064080` clear/set the `[0x109A90]` recorder
  gate.
- `FUN_0004A228` (match setup) composes, resets the ring and enables the
  recorder. The ring is not reset per period; it spans the match.

### 5.4 The state machine (`FUN_000642FC`, fresh)

- The input word (low byte) sets `[0x9A98]`: `8 → 0x85`, `4 → 0x84`,
  `2 → 0x86`; `1` toggles `0x82`/`0x83`; `0x20` cycles the camera; `0x80`
  exits.
- `0x84` = two `FUN_00063CBC` (forward); `0x85` = two `FUN_00063E54`
  (back); `0x86` = slow (decrement `[0x9A9C]`, else step forward and reload
  from `[0x9AA0]`); `0x83` = `[0x9AA8] += ticks`, then the countdown loop
  (`stamp = FUN_00063D34`; while `0 < stamp <= [0x9AA8]`: subtract, step
  forward, re-sync, re-read); `0x82` = idle/display (`[0x9A9C] = 0`).
- The `0x81` arm zeroes the play index `[0x9A94]`, calls `FUN_00063CBC` once
  (applies record 0 and advances the index to 1), selects camera 0 via
  `FUN_0004D134`, sets `0x82`, and zeroes the accumulators.
- **State timing.** The input branch sets `0x84`/`0x85`/`0x86`; the same
  call's switch performs the step; the mode byte is not rewritten to `0x82`
  until the *next* call (the top-of-call reset at `0x643C9..0x643E9`). The
  state therefore survives the frame (the HUD highlights the step button) and
  resets on the next call. The port's current next-call reset is right; the
  missing piece is the actual double-step in the same call.
- **Idle at the end rewinds.** The `0x82` play/pause arm, when
  `[0x9A94] >= count`, sets `[0x9A94] = 0` and calls `FUN_00063CBC` (applies
  record 0, cursor = 1) before entering `0x83`. The port currently wraps
  `cursor` without applying.
- **Erratum (0x82/0x83 labels).** The native `0x82` is the **idle/display**
  state (the current record is shown; `[0x9A9C] = 0`) and `0x83` is the
  **active playback countdown** (the ring advances by the stamp). The port's
  enum names `PLAY = 0x82` / `PAUSE = 0x83` are semantically inverted; the
  slice renames them `IDLE`/`PLAYING` (values unchanged) and updates the
  tests/HUD naming. The wire values `0x82`/`0x83` stay.
- **Erratum (port labels).** The port's enum has `STEP_BACK = 0x84` and
  `STEP_FWD = 0x85`; the native is the opposite (`0x84` = forward two
  `63CBC`, `0x85` = back two `63E54`). The slice corrects the values and keeps
  the button mapping native (8 → `0x85` back, 4 → `0x84` forward). The
  existing tests assert names, not raw values, and will be updated.

### 5.5 The camera bytes and the matrix consumer

- The four record bytes `+0xF2..+0xF5` are the four separate view bytes
  `0x155C4E` (roll/flag, low nibble = ball roll), `0x155C4F` (queue pop),
  `0x155C50` (camera band from `FUN_000627F4`) and `0x155C51` (the `=1` flag).
  They are not the dword at `0x155C4C`; byte 3 of that dword *is* `0x155C4F`.
  The producers are unported (RR-L3); the view stores staged values. On
  apply, the **whole** roll byte is copied to `render.ball_roll` (the draw
  reads the low nibble; the high nibble carries the native flag), not just
  the low nibble.
- `FUN_0006408C` copies six dwords from `0x155C52..0x155C69` (the composer's
  `FUN_0004C904()` snapshot of `[0x7DC8]+0x10/0x14/0x18/0x58/0x5C/0x4C`;
  FU-121) and `FUN_00063B80` restores them. Fresh `get_xrefs_to 0x155C52`
  shows the readers `FUN_0004D134` (`0x4D183`) and `FUN_0004D2D4` (`0x4D310`)
  — the replay camera selection path. The six fields map directly onto
  `fifa96_camera_handler_state`: `pos_x/pos_y/pos_z`, `yaw`, `pitch`,
  `ratio`.
- `0x155C70` is `FUN_000974D0` (returns 1), `0x155C72` is `FUN_000492CC`
  (`[0x10732C]`). The port's active view state is
  `render.native_view_cam[render.native_view_class]` when
  `native_view_class < 3 && native_view_valid[native_view_class]` (the array
  is indexed by class, not slot), else `render.action_cam`.

## 6. Approaches

**Approach 1 — native block and record, rendered from a replay view
(chosen).** The `0x155AA4` block is a `replay_view`; the 0x118 records are a
fixed ring; the composer fills the view from the focus triple and the
composed render slots; the packer/applier are exact; playback applies into
the view and the renderer stages the view. The live pool is untouched.
Closes the FU-108/109/112 core and makes the mode-7 path buildable. Costs
~42 KB of embedded ring and a new view staging path; the camera matrix
consumer is scoped to the pose camera (RR-L5).

**Approach 2 — native record shape, entity-only view.** Same ring/record,
but the camera cluster is stored and not rendered; the source-4 camera stays.
Faster, true for players/ball, but the recorded camera angle is a leg and the
mode-7 work still needs the ring. Not chosen: it stops short of the "true
recorded replay" the user asked for.

**Approach 3 — port snapshot ring.** Record the port's `fifa96_match_entity`
fields directly. Smallest diff, but not the native ring, no stamp/cursor
semantics, no mode-7 path. Dropped.

## 7. The design

### 7.1 Data model

Both the record and the view are byte blocks with explicit offsets; no struct
padding can silently move a lane. Accessors are the only accessors.

```c
#define FIFA96_MATCH_REPLAY_RING_RECORDS 150   /* 0x96 */
#define FIFA96_MATCH_REPLAY_RECORD_SIZE  0x118
#define FIFA96_MATCH_REPLAY_VIEW_SIZE    0x1D0   /* through 0x155C73 */
#define FIFA96_MATCH_REPLAY_VIEW_SLOTS   23    /* 22 team records + ball */
#define FIFA96_MATCH_REPLAY_VIEW_HIDDEN_Y (-10000)  /* native dword 0xFFFFD8F0 */

struct fifa96_match_replay_ring {
  uint8_t record[FIFA96_MATCH_REPLAY_RING_RECORDS]
                [FIFA96_MATCH_REPLAY_RECORD_SIZE];
  uint32_t read;         /* *[0x109AB8] */
  uint32_t write;        /* *[0x109ABC] */
  uint32_t count;        /* *[0x109AC0] */
  uint32_t stamp_acc;    /* [0x109AAC] */
  uint32_t budget;       /* [0x107310] */
  uint8_t enabled;       /* [0x109A90] */
};
```

The play index is the **existing** `render.replay.cursor` (`[0x109A94]`);
the ring adds only the read/write cursors, the count, the packer accumulator,
the budget and the enable gate. `render.replay.cursor_limit` mirrors the
count for the HUD progress (`FUN_000642B0`).

The ring is a top-level `struct fifa96_match_replay_ring replay_ring;` in
`struct fifa96_match_run`. The view is `uint8_t replay_view[0x1D0];` in
`struct fifa96_match_run_replay`, with accessors for the fields in §5.1
(`replay_view_dword`, `replay_view_word`, `replay_view_byte`, and the
slot-array addresses). The record and view offsets in §5.1 are the binding
contract; tests pin raw bytes at those offsets.

### 7.2 Public seam

```c
void fifa96_match_replay_ring_reset(struct fifa96_match_run *mr);
void fifa96_match_replay_ring_record(struct fifa96_match_run *mr, int on);
void fifa96_match_replay_compose(struct fifa96_match_run *mr);
void fifa96_match_replay_capture(struct fifa96_match_run *mr, uint32_t ticks);
uint32_t fifa96_match_replay_count(const struct fifa96_match_run *mr);
int fifa96_match_replay_apply(struct fifa96_match_run *mr, uint32_t index);
int fifa96_match_replay_step_forward(struct fifa96_match_run *mr);
int fifa96_match_replay_step_back(struct fifa96_match_run *mr);
uint16_t fifa96_match_replay_stamp(const struct fifa96_match_run *mr);
void fifa96_match_replay_pose(struct fifa96_match_run *mr);
void fifa96_match_replay_stage(struct fifa96_match_run *mr);
```

`replay_begin` keeps its gate; it additionally clears `cursor` (`play`), the
playback `advance` (`[0x9AA8]`) and the input latch (`[0x9AC8]`); the ring's
`stamp_acc` is not reset by the arm. The `0x81` arm in `replay_step` applies
record 0 when `count > 0`, advances `cursor` to 1, sets
`cursor_limit = count`, and then runs the existing camera-0/target selection.
An empty ring still enters `0x82`. `replay_exit` leaves the ring intact (the
next replay sees it) and restores the live camera.

### 7.3 Composer and capture

- `fifa96_match_replay_compose` fills the view:
  - `+0x00/+0x04/+0x08`: the focus triple from `render_focus_*` when
    `render_focus_valid`, else `camera_override_triple` when
    `camera_override_157a6f`, else `render.camera.pos_*`, clamped with
    `fifa96_camera_focus_clamp` (±6000, 0..16000).
  - Slots 0..21: the staged `render.entities[slot].stage` (the values the
    renderer draws); `x/y/z = stage.pos`; the packed heading is
    `((0x400 - (stage.heading >> 16)) & 0x3FF) << 6`; appearance =
    `stage.anim_id` (OL-80), frame = `stage.frame`; `stage.hidden` =>
    `y = 0xFFFFD8F0`.
  - Slot 22 (the ball): the staged `render.entities[22].stage`; the packed
    heading `((0x400 - (stage.heading >> 16)) & 0x3FF) << 6`, appearance =
    `stage.anim_id` (the ball's `0x158866` row id), frame = `stage.frame`
    (0 until the `0x15887B` producer lands, RR-L9).
  - `+0x1AA..+0x1AD` (roll/queue/band/flag): the port's `render.ball_roll`
    and staged 0 for the queue/band/flag (RR-L3); `+0x1CC = 1` (the
    `FUN_000974D0` stub); `+0x1CE = (uint16_t)mr->state.frame_delta`
    (the `FUN_000492CC` stand-in; RR-L6).
  - Camera matrix: from
    `render.native_view_cam[render.native_view_class]` when
    `native_view_class >= 0 && native_view_class < 3 &&
    native_view_valid[native_view_class]`, else `render.action_cam`;
    **view** `+0x1AE..+0x1C5` (record `+0xF6..+0x10D`) = `pos_x/pos_y/pos_z,
    yaw, pitch, ratio`.
- The live frame body calls `compose` then `capture(mr, 1)` once per granted
  live frame, immediately after `match_run_scene_stage` **inside the
  `render.enabled` branch**, so the record is exactly the frame the renderer
  draws. The freeze/replay path does not call it.
- `capture` first updates the mode-window budget exactly (§5.3): if
  `mr->state.phase` is in the increment set, `budget++`; else `budget = 0`.
  **If `budget >= 0x5A`, return before touching `stamp_acc`** — the native
  does not call the packer at all, so the accumulator does not advance while
  the budget blocks. Otherwise mimic `FUN_0006408C`: gated on `enabled`; if
  `ticks == 0`, set `stamp_acc = 1` and write immediately (the native flush);
  else add `ticks` and return while `< 2`; evict when `count > 0x95`
  (`read = (read+1) % 150`, `count--`); write at `write`, pack the view, stamp
  `+0x116 = stamp_acc` (low word), `write = (write+1) % 150`, `count++`,
  `stamp_acc = 0`.
- `ring_reset` zeroes the read/write cursors, the count, the play cursor
  (`render.replay.cursor`), its `cursor_limit`, the packer accumulator, the
  budget and the recorder gate. The **match** `fifa96_match_run_begin` calls
  reset and then enables the recorder with `ring_record(mr, 1)`;
  `fifa96_match_run_init` resets (gate stays off) and `fifa96_match_run_end`
  resets. `fifa96_match_run_replay_begin` does **not** reset the ring. The
  act-9 reset does not reset the ring (the native setup caller is the only
  `FUN_00064030` caller).

### 7.4 Playback

- `record_at(i)` = `record[(read + i) % 150]`.
- `apply(mr, i)` unpacks the record into `replay_view` (all lanes, including
  `+0x1CE = record+0x116`) and syncs `render.ball_roll` from the applied roll
  byte. It does **not** write the live view angles/eye; `fifa96_match_replay_pose`
  does that after the live pose feed (below).
- `fifa96_match_replay_pose(mr)`: when
  `camera_kind == FIFA96_CAMERA_REPLAY_POSE`, set
  `render.render_eye_* = matrix[0..2]`, `render_eye_valid = 1`,
  `render.yaw = matrix[3]`, `render.pitch = matrix[4]`,
  `render.view_ratio = matrix[5]`. Other camera kinds keep the existing
  source-4 follow (RR-L5). The eye path is live-run only (the renderer's
  `fifa96_match_run_render_eye` uses `render_eye_*` only when `live_start`),
  so M1/M2 stay untouched.
- `step_forward`: if `cursor < count`, apply(`cursor`), `cursor++`; **inside
  that same guard**, when `cursor >= count`, set the state to `0x82` (the
  native `FUN_000974D8` + mode 0x82; the hook is RR-L10). At the boundary
  (`cursor >= count` on entry) the step is a complete no-op and the state is
  left unchanged. `cursor_limit = count`. Returns 1 when a record was applied.
- `step_back`: if `cursor > 0`, `cursor--`, apply(`cursor`); **inside that
  same guard**, when `cursor < 1`, set the state to `0x82`. At `cursor == 0`
  the step is a no-op. `cursor_limit = count`.
- `stamp`: the `+0x116` word of `record_at(cursor)`, or 0 when
  `cursor >= count`.
- The `0x83` countdown uses the **ring tick** (1 per granted replay frame,
  not the Q8 `frame_delta`): `advance += 1`; while `stamp > 0 &&
  stamp <= advance`: `advance -= stamp`, `step_forward`, `FUN_0006428C`'s
  re-sync (RR-L10), re-read. The Q8 delta still drives the
  animation/staging; the two units are separate.
  (Recording ticks 1 and stamps 2; playback adds 1 per frame, so 150 records
  play over 300 frames — 10 s at 30 Hz, the native cadence.)
- `0x84`/`0x85` perform two `step_forward`/`step_back` calls in the same
  `replay_step` call and leave the state at `0x84`/`0x85`; the next
  `replay_step` call resets it to `0x82` before reading the input (the native
  top-of-call reset). `0x86` decrements `wait_timer`; at 0 it steps forward
  and reloads from `wait_reload`. `0x82` resets `wait_timer`.
- Button mapping: `1` toggles idle (`0x82`) ↔ playing (`0x83`); `8 → step
  back`; `4 → step forward`; `2 → slow`; `0x20` camera; `0x80` exit. In
  idle (`0x82`), pressing `1` when `cursor >= count` rewinds (`cursor = 0`,
  apply record 0, `cursor = 1`) before entering `0x83`. `replay_family`
  remains `(state & 0x80) != 0`.

### 7.5 Rendering

- Add `fifa96_match_replay_stage(mr)` that writes
  `render.entities[slot].stage` from the view slots: pos, the **unfolded**
  heading `(0x400 - (packed >> 6)) & 0x3FF` shifted into the high word (so
  `fifa96_render_slot_stage` folds it correctly), appearance → `anim_id`,
  frame; `hidden = (y == 0xFFFFD8F0)`; slot 22 from the view's ball record.
  It writes render cells only. The replay stage must also re-derive
  `bank_index` from the applied appearance (the record has no bank field):
  `fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
  applied_anim_id, &row)` → `bank_index = row.sprite_bank`, the same resolver
  `match_run_animate_entity` uses.
- The frozen frame order is: `replay_step` (advance + apply the view) →
  pose feed → live view driver (**non-pose cameras only**) →
  `fifa96_match_replay_pose` (pose camera, applied **after** the pose feed so
  it is not overwritten) → display update → replay scene stage → camera
  trace. The live pan/event walk stays skipped (C3) so no shared RNG is
  spent.
- The HUD/overlay rows, audio duck and `mark_over` deferral are unchanged.

### 7.6 Tests

Add to `port/tests/test_engine_match_frame.c` and
`port/tests/test_engine_match_render.c`:

- `test_replay_ring_pack_apply_roundtrip`: fill a view byte block with 23
  distinct slots/matrix/roll/queue/band/flag/words, pack, clear, apply,
  assert every byte and the record bytes at the pinned offsets
  (`+0xC4+i`, `+0xDB+i`, `+0xF2..+0xF5`, `+0x114`, `+0x116`); assert the
  applied `+0x1CE` equals the record's `+0x116` (the one asymmetric lane).
- `test_replay_ring_capture_budget`: a reset phase (2) resets the budget and
  **packs** (the gate is a counter, not a pause); an increment phase (0) packs
  one record per 2 frames and stops once the budget reaches 90 (44 records:
  frames 2..88, the 90th blocked); a reset phase clears the budget and
  packing resumes. A `ticks == 0` call is a flush (one record, stamp 1).
  Fill to 151 packs across windows; assert `count = 150`,
  `read = 1` (one eviction), the oldest record is the second pack, and stamp 2.
- `test_replay_ring_empty`: arm before any capture → idle (`0x82`), count 0,
  steps are no-ops, exit works.
- `test_replay_playback_cadence`: 150 records; 300 replay grants advance
  exactly one record per 2 grants; `cursor`/`cursor_limit` follow; the last
  step leaves idle (`0x82`) at `cursor == count`.
- `test_replay_step_modes`: `8` sets `0x85` and performs two back steps; `4`
  sets `0x84` and performs two forward steps; the state resets to `0x82` on
  the next call; `2` slow timer; the `0x83` playing countdown consumes stamps;
  step back at 0 → `0x82`; pressing `1` at the end rewinds to record 0.
- `test_replay_view_render`: a record with distinct slot values stages into
  `render.entities`; the live pool is unchanged; hidden uses the y sentinel;
  the heading is folded once (not twice); with `live_start = 1`, the pose
  camera sets `render_eye_*`/`yaw`/`pitch`/`view_ratio` from the record after
  the pose feed; the applied roll byte sets `render.ball_roll`.
- The existing replay tests are updated for the native enum values and the
  same-call double-step; `test_replay_input_edges` stays.

### 7.7 Acceptance

- `make check` 124/124, strict-clean; M1/M2 `cmp` exit 0; windowed smoke
  exit 124 with a clean log.
- No README/port-table/census/headline/completion-number movement (no new
  dispatch body or row).
- The replay draws the recorded past: a test that moves an entity between two
  captured frames and arms the replay sees frame N's position, not the live
  position.

### 7.8 Legs

- **RR-L1 — allocator/lifecycle.** `FUN_0004A448`/`&0xA41C`/`FUN_000993EC`
  and the ring's free/allocate lifecycle. The port embeds 150 records.
- **RR-L2 — recorder overlay gating.** `FUN_00053DC4`'s `[0x14E574]` latch
  and `FUN_00053A48`'s state-5 re-enable. The port records every live ACTIVE
  frame while the mode-window budget allows.
- **RR-L3 — camera band/queue producers.** `FUN_000627F4`, `FUN_00064EE4`
  and the `0x55C6A` six-byte queue. The roll byte is wired to
  `render.ball_roll`; the queue/band/flag are stored staged 0.
- **RR-L4 — appearance byte mapping.** `***(byte**)(rec+0x28)` and
  `[0x158866]` vs the port's composed row id (OL-80). The record stores the
  composed render value.
- **RR-L5 — the `0x20` camera cycle / matrix precedence.** The replay camera
  record cycle and the non-pose camera views. The default pose camera uses
  the recorded matrix; the others keep the existing selector. The native
  matrix source is the `[0x7DC8]` active view object (FU-121); the port
  captures its six fields from `native_view_cam`/`action_cam`, whose identity
  with that object is part of this leg.
- **RR-L6 — the tick source.** `FUN_000492CC` is `[0x10732C]`; the port uses
  1 per granted frame and `frame_delta` for `+0x1CE`. The exact live
  frame-rate/tick reconciliation stays open.
- **RR-L7 — mode-7 request/timed exit.** `FUN_0004B100` →
  `FUN_000510DC` → `FUN_00064270`; the ring now provides the cursors/stamps
  this path needs.
- **RR-L8 — the exact mode-window phase mapping.** The port's
  `state.phase` is the `[0x157A4A]>>24` model; the increment/reset sets are
  modeled as FU-111 records them. If a live phase is ever re-mapped, the
  budget gate follows the mode byte, not the phase table.
- **RR-L9 — the ball frame byte.** The native composer copies `0x15887B`
  (`obj+0x8F`, FU-158 L14.8.b) into the 23rd slot; the port does not model
  the producer and stages 0.
- **RR-L10 — the re-sync hooks.** `FUN_000974D8`/`FUN_000974DC` are called
  on every replay transition (the `0x81` arm, play/pause, the step modes and
  the countdown); the port's transitions are state-only and the hooks remain
  unported (FU-108 §6 leg 2 / FU-107 leg 4).

### 7.9 Documentary errata the slice appends

Append-only, in the source docs:

- **FU-108 §2.** `FUN_00063CBC` is the playback forward step, not the
  append; the recorder/packer is `FUN_0006408C`.
- **FU-109 §2.** The byte lanes are record `+0xC4+i` / `+0xDB+i` from view
  `+0x17C` / `+0x193` (the decompiler's `puVar6 + 0x31` display is pointer
  arithmetic); the axis order is x at `+0xC`, z at `+0x3A`, y at `+0x68`;
  the `+0x116` stamp applies into the view's `+0x1CE`.
- **FU-112 §1.** The first triple is the camera focus, not the ball record;
  the 23rd frame byte is `[0x15887B]`, not 0.
- **FU-107 / ENGINE.** The port enum labels were swapped; the native
  `0x84` is forward (two `63CBC`) and `0x85` is back (two `63E54`), and the
  input maps 8 → `0x85`, 4 → `0x84`. The port's `PLAY = 0x82` / `PAUSE = 0x83`
  labels are also inverted: native `0x82` is idle/display and `0x83` is the
  active playback countdown. The slice renames them `IDLE`/`PLAYING` with the
  wire values unchanged.
- **FU-111 §1.** The gate is a budget cap, not a pause: the reset modes set
  the counter to 0 and the packer runs; an uninterrupted increment-mode run
  yields 44 records (the 90th frame's pack is blocked).
- **FU-351 §26 / FU72 remaining-leg paragraphs.** The recorded-frame ring is
  no longer a leg; the replay camera record cycle (RR-L5) and the mode-7
  request/timed exit (RR-L7) remain.

## 8. What is left to the builder

- The exact placement of the compose/capture call inside the live frame body
  (after scene staging, before the render tail); the requirement is that the
  record is the frame the renderer draws.
- The focus-triple fallback order (`render_focus_*` vs
  `camera_override_triple` vs `camera.pos_*`) and the clamp call; the
  native source is `[0x57A6F]` or `0x15774C`.
- The view accessor helpers (byte/dword/word get/set and the slot-array
  addresses); the §5.1 offsets are binding.
- The exact `fifa96_render_slot_stage` input order (fold once) and the pose
  application seam after the live pose feed.
- The test fixture mechanics for the 90-frame budget and the 300-frame
  cadence (the existing `one_granted_frame` helper vs a direct pace loop).

## 9. Provenance

Fresh Ghidra MCP on `/FIFA96.EXE` (2026-11-12):
`disassemble_function` 0x6408C (131 insns), 0x63B80 (85), 0x63EBC (45),
0x645B0 (64); `decompile_function` 0x63CBC, 0x63D34, 0x63E54, 0x64030,
0x6408C, 0x63B80, 0x6428C, 0x64DFC, 0x642FC, 0x4A228, 0x53A48, 0x53DC4,
0x492CC, 0x4A068, 0x510DC; `get_xrefs_to` 0x155AA4 (35), 0x155C52 (8),
0x6408C (2), 0x63CBC (8), 0x63EBC (2), 0x64030 (1), 0x64074 (2), 0x64080
(12), 0x36C70 (7), 0x642FC (2); `disassemble_bytes` 0x4AE20, 0x4AF00,
0x4A020, 0x49F70, 0x645B0, 0x36DF0; `get_function_by_address`
0x70DE0/0x7131C/0x72AC4/0x72478/0x8A938 (the 19-site campaign scope, not
this slice). FU-111 §1 is the prior record of the 90-frame budget; FU-158
§2/§4 is the prior record of the ball `0x15887B` frame. Address mapping as
FU-88.

## 10. Closure (2026-11-12, Tasks 1–6)

Status: **implemented**. Tasks 1–5 landed the ring; Task 6 appends the
documentary errata (§7.9) and closes the slice. The design's status line is
the only prior text changed to `implemented`; every correction below is a
quoted erratum in the named source doc, never a rewrite of a prior section.

**Landed (Task 1 — pack/apply, Task 2 — cursors/capture/budget, Task 3 —
composer, Task 4 — playback states, Task 5 — pose/stage and the frozen-frame
integration).** The `0x155AA4` view block (`uint8_t replay_view[0x1D0]`), the
150 × `0x118` record ring (`struct fifa96_match_replay_ring`), and the §7.2
seam (`_ring_reset`/`_ring_record`/`_compose`/`_capture`/`_count`/`_apply`/
`_step_forward`/`_step_back`/`_stamp`/`_pose`/`_stage`) are all landed in
`port/src/fifa96_engine/fifa96_match_replay_ring.{c,h}` and
`fifa96_match_run.{c,h}`. The pack/applier are byte-exact inverses over the
§5.1 map (the `+0x116` stamp is the one asymmetric lane — pack writes the
accumulator, apply writes it into the view `+0x1CE`). The capture cadence is
the two-tick packer under the 90-frame mode-window budget (44 records per
uninterrupted increment window; a reset phase clears the budget and packing
resumes; the 151st pack evicts). The composer fills the view from the staged
render slots + focus + camera cluster. Playback applies through the native
states (`0x81` arm → `0x82` idle/display → `0x83` playing countdown, `0x84`
forward double-step, `0x85` back double-step, `0x86` slow). `_pose`/`_stage`
render the applied view instead of the frozen live pool; the live pool is
never written by playback.

**Tests and gate.** New `port/tests/test_engine_replay_ring.c` pins the pack
map lane-by-lane and the round trip, the budget (44 records, the blocked 90th
frame, the reset-phase clear), the 150-record eviction, the empty/disabled/
flush (`ticks == 0` → stamp 1) edges, the composer (focus/override/clamp,
hidden sentinel, folded heading, ball slot, camera cluster), the 300-grant
playback cadence (one record per two grants, `cursor`/`cursor_limit`), the
step modes (`8`→`0x85` two back steps, `4`→`0x84` two forward steps, `2`
slow, `1` at the end rewinds to record 0), and the empty-ring arm.
`test_engine_match_frame` adds `test_replay_capture_in_frame` and
`test_replay_draws_recorded_past`; `test_engine_match_render` adds
`test_replay_view_render` and keeps the state-machine/input/freeze tests.
Gate: `make check` **125/125** strict-clean; M1/M2 `cmp` byte-exact; windowed
smoke `timeout 20s env DISPLAY=:1 make game` exit 124, clean log.

**Unchanged (per §4 anti-goals).** The port coverage stays 78/78, the wired
counts 61/80 (64/80 install-live), the README title and the M1/M2 goldens are
unmoved — no new dispatch body/row and no golden re-pin.

**Errata appended (Task 6).** The §7.9 list is now recorded in the source
docs: FU-108 §2 (`FUN_00063CBC` is the playback forward step, the packer is
`FUN_0006408C`); FU-109 §2 (byte lanes `+0xC4+i`/`+0xDB+i`, axis order x/z/y,
stamp applies into view `+0x1CE`); FU-112 §1 (first triple is the camera
focus, the 23rd frame byte is `[0x15887B]`); FU-107/ENGINE (wire values
`0x84`=forward two, `0x85`=back two, input `8`→`0x85` and `4`→`0x84`;
`0x82`=idle/display, `0x83`=playing countdown); FU-111 §1 (budget cap, 44
records per uninterrupted run); FU-351 §26/FU72 (the ring leg is landed;
RR-L5 and RR-L7 remain).

**Remaining legs (unchanged from §7.8).** RR-L1 allocator/lifecycle; RR-L2
recorder overlay gating; RR-L3 camera band/queue producers; RR-L4 appearance
byte mapping; RR-L5 the `0x20` camera cycle / matrix precedence; RR-L6 the
tick source; RR-L7 mode-7 request/timed exit; RR-L8 the exact mode-window
phase mapping; RR-L9 the ball frame byte; RR-L10 the re-sync hooks.
