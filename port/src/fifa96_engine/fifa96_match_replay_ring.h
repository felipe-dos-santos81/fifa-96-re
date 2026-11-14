/* port/src/fifa96_engine/fifa96_match_replay_ring.h — the recorded-frame
 * replay ring: the native `0x155AA4` view block and the 150 x `0x118` record
 * ring.
 *
 * Evidence (spec `2026-11-12-recorded-replay-ring-design.md` §5.1, fresh
 * `disassemble_function 0x6408C` (packer) and `0x63B80` (applier) on
 * `/FIFA96.EXE`): both loops run 23 iterations and pre-increment their byte
 * pointers, so the record is `0x118` bytes with the appearance lane at
 * `+0xC4` and the frame lane at `+0xDB` (not `+0xC3`/`+0xDA`). The view block
 * is `0x1D0` bytes: the focus triple at `+0x00/+0x04/+0x08`, 23 slots of
 * 0x0C bytes from `+0x0C`, the packed headings at `+0x120 + 4*i`, the
 * appearance/frame byte lanes at `+0x17C`/`+0x193`, and the camera cluster
 * `+0x1AA..+0x1CF`.
 *
 * The record map (record -> view):
 *   +0x00/+0x04/+0x08  -> +0x00/+0x04/+0x08   focus triple (dword x3)
 *   +0x0C+2i           -> +0x0C+0x0C*i        x      (word, apply sign-extends)
 *   +0x3A+2i           -> +0x14+0x0C*i        z      (word, apply sign-extends)
 *   +0x68+2i           -> +0x10+0x0C*i        y      (word, apply sign-extends)
 *   +0x96+2i           -> +0x120+4*i          heading (word, apply zero-extends)
 *   +0xC4+i            -> +0x17C+i            appearance byte
 *   +0xDB+i            -> +0x193+i            frame byte
 *   +0xF2..+0xF5       -> +0x1AA..+0x1AD      roll / queue pop / band / flag
 *   +0xF6..+0x10D      -> +0x1AE..+0x1C5      camera matrix (dword x6)
 *   +0x10E..+0x113     -> +0x1C6..+0x1CB      queue6 bytes
 *   +0x114             -> +0x1CC              `word_55C70`
 *   +0x116             -> +0x1CE              stamp (asymmetric: see below)
 *
 * The `+0x116` lane is asymmetric by design: the packer writes the packer
 * accumulator (`[0x109AAC]`), the applier writes it into the view's `+0x1CE`
 * (`word_55C72`, the composer's `FUN_000492CC` word). Every other lane is a
 * byte-exact inverse.
 *
 * The appearance lane is the identity, not a mapping (RR-L4, census
 * `2026-11-13-replay-chain-census.md`): the composer writes
 * `view[+0x17C+i] = stage.anim_id` byte-exact, where a team slot's
 * `stage.anim_id` is `byte[[rec+0x28]]` (the native `0x36D44 MOV ECX,[EDX+0x28];
 * 0x36D47 MOV CL,[ECX]`) and the ball slot 22's is `byte[[0x158866]]`
 * (`0x36E20 MOV ECX,[0x158866]; 0x36E2D MOV CL,[ECX]`) — one pointer load then
 * one byte load at offset 0 (`row[0]`), carried by the source-to-stage link
 * `ball->stage.anim_id = mr->entities.ball.appear_row`. The packer copies it
 * to `record[+0xC4+i]` and the applier returns it; there is no row+offset
 * composition on the appearance byte.
 *
 * The ring cursors are the native cells `*[0x109AB8]` (read), `*[0x109ABC]`
 * (write), `*[0x109AC0]` (count), `[0x109AAC]` (the packer accumulator),
 * `[0x107310]` (the 90-frame mode-window budget) and `[0x109A90]` (the
 * recorder gate). The playback index is the existing
 * `render.replay.cursor` (`[0x109A94]`), so the ring carries no second copy.
 */
#pragma once
#include <stdint.h>

struct fifa96_match_run;   /* defined in fifa96_match_run.h */

#define FIFA96_MATCH_REPLAY_RING_RECORDS 150u   /* the native wrap (0x96) */
#define FIFA96_MATCH_REPLAY_RECORD_SIZE 0x118u
#define FIFA96_MATCH_REPLAY_VIEW_SIZE 0x1D0u    /* through 0x155C73 */
#define FIFA96_MATCH_REPLAY_VIEW_SLOTS 23u      /* 22 team records + the ball */
#define FIFA96_MATCH_REPLAY_VIEW_HIDDEN_Y (-10000)  /* native dword 0xFFFFD8F0 */

struct fifa96_match_replay_ring {
  uint8_t record[FIFA96_MATCH_REPLAY_RING_RECORDS]
                [FIFA96_MATCH_REPLAY_RECORD_SIZE];
  uint32_t read;         /* *[0x109AB8] */
  uint32_t write;        /* *[0x109ABC] */
  uint32_t count;        /* *[0x109AC0] */
  uint32_t stamp_acc;    /* [0x109AAC]: the packer accumulator / record stamp */
  uint32_t budget;       /* [0x107310]: the 90-frame mode-window budget */
  uint8_t enabled;       /* [0x109A90]: the recorder gate */
};

/* `FUN_0006408C`'s write body: pack the 0x1D0-byte view block into one
 * 0x118-byte record and write `stamp` at record `+0x116`. `view` and `record`
 * must be non-NULL (a NULL pair is a no-op). */
void fifa96_match_replay_pack(const uint8_t *view, uint8_t *record,
                              uint16_t stamp);

/* `FUN_00063B80`'s apply body: unpack a record back into the view block. The
 * stamp lands in the view's `+0x1CE` word and the heading high words are left
 * untouched (the composer sets them to 0). */
void fifa96_match_replay_unpack(const uint8_t *record, uint8_t *view);

/* `FUN_00064030`: zero the read/write cursors, the count, the packer
 * accumulator and the playback index (`render.replay.cursor`), with the count
 * mirror (`cursor_limit`) following. The native body does NOT write the
 * mode-window budget (`[0x107310]`) or the recorder gate (`[0x109A90]`): the
 * engine zeroes the budget as an adapter and preserves the gate, which is
 * toggled only by `FUN_00064074`/`FUN_00064080`. */
void fifa96_match_replay_ring_reset(struct fifa96_match_run *mr);

/* `FUN_00064074`/`FUN_00064080`: clear/set the `[0x109A90]` recorder gate. */
void fifa96_match_replay_ring_record(struct fifa96_match_run *mr, int on);

/* The composer (`FUN_00036C70`'s view half, spec §5.2/§7.3): fill the view
 * block from the staged render slots — the values the renderer draws — plus
 * the focus triple, the ball roll and the camera matrix. The focus is
 * `render.render_focus_*` when valid, else `camera_override_triple` when
 * `camera_override_157a6f`, else `render.camera.pos_*`, clamped with
 * `fifa96_camera_focus_clamp`. The packed heading is the native folded
 * `((0x400 - (stage.heading >> 16)) & 0x3FF) << 6`, a hidden slot writes
 * `FIFA96_MATCH_REPLAY_VIEW_HIDDEN_Y` in y, the six queue bytes are copied
 * from `mr->replay_queue6` (staged `0xFF` by default),
 * `+0x1CC` is the `FUN_000974D0` stub 1 and `+0x1CE` the `FUN_000492CC`
 * stand-in (`state.frame_delta`). The matrix comes from
 * `native_view_cam[native_view_class]` when that class is a valid 0..3 view
 * (class 3 is RR-L5's staged sub-8 variant), else `action_cam`. */
void fifa96_match_replay_compose(struct fifa96_match_run *mr);

/* One live frame of the recorder: the 90-frame mode-window budget gate
 * (`[0x107310] < 0x5A`), the recorder gate and the `FUN_0006408C` packer
 * cadence. `ticks` is 1 per live frame; 0 is the native flush (write now,
 * stamp 1). */
void fifa96_match_replay_capture(struct fifa96_match_run *mr, uint32_t ticks);

/* The ring count (`*[0x109AC0]`), 0 on NULL. */
uint32_t fifa96_match_replay_count(const struct fifa96_match_run *mr);

/* `FUN_00063B80`'s playback apply: unpack ring record `index` into the view
 * block and latch the roll byte from the applied `+0x1AA`. Returns 1 on an
 * applied record, 0 when `index` is past the ring (or `mr` is NULL). */
int fifa96_match_replay_apply(struct fifa96_match_run *mr, uint32_t index);

/* `FUN_00063D34` / `FUN_00063CBC`: one forward / back playback step. Each
 * applies the record under the cursor and moves the cursor one record; the
 * boundary (cursor >= count for forward, cursor == 0 for back) is a complete
 * no-op and the IDLE state write sits inside the applied guard. Both set
 * `cursor_limit` to the ring count and return 1 when a record was applied. */
int fifa96_match_replay_step_forward(struct fifa96_match_run *mr);
int fifa96_match_replay_step_back(struct fifa96_match_run *mr);

/* The `+0x116` stamp word of the record under the cursor (0 past the ring or
 * on NULL). */
uint16_t fifa96_match_replay_stamp(const struct fifa96_match_run *mr);

/* `FUN_0004D134`'s POSE feed: when the selected replay camera is the POSE
 * kind, set the render eye/valid/yaw/pitch/ratio from the applied view
 * matrix `+0x1AE..+0x1C5`. */
void fifa96_match_replay_pose(struct fifa96_match_run *mr);

/* The frozen-frame render stage (spec §7.5): write `render.entities[0..22]
 * .stage` from the applied view block — pos, the unfolded heading
 * `(0x400 - (packed >> 6)) & 0x3FF` shifted into the high word, appearance →
 * `anim_id`, frame, `hidden = (y == FIFA96_MATCH_REPLAY_VIEW_HIDDEN_Y)` — and
 * re-derive each slot's `bank_index` from the applied appearance via
 * `fifa96_match_run_anim_bank`. Sets `render.entity_count = 23`. Writes render
 * cells only (the live pool is untouched). */
void fifa96_match_replay_stage(struct fifa96_match_run *mr);
