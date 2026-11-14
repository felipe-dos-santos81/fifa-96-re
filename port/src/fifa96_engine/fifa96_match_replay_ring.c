/* port/src/fifa96_engine/fifa96_match_replay_ring.c — the native packer/
 * applier pair (`FUN_0006408C` / `FUN_00063B80`) over the `0x155AA4` view
 * block and the `0x118` record.
 *
 * Evidence: spec `2026-11-12-recorded-replay-ring-design.md` §5.1 — a fresh
 * `disassemble_function 0x6408C` (packer) and `0x63B80` (applier) on
 * `/FIFA96.EXE`, both 23-iteration loops with pre-incremented byte pointers
 * (the `+0xC4` appearance / `+0xDB` frame lanes). The header carries the full
 * record -> view map; the offsets below are that map.
 *
 * The host is little-endian and the native block is a little-endian byte
 * image, so every multi-byte lane goes through `memcpy` (never a cast through
 * an unaligned pointer). */
#include <string.h>

#include "fifa96_engine/fifa96_match_replay_ring.h"
#include "fifa96_engine/fifa96_match_run.h"

/* The map's fixed lane offsets (header). */
enum {
  FIFA96_REPLAY_FOCUS = 0x00,        /* focus triple: +0x00/+0x04/+0x08 */
  FIFA96_REPLAY_SLOTS = 0x0C,        /* 23 slots x 0x0C bytes (x, y, z) */
  FIFA96_REPLAY_SLOT_STRIDE = 0x0C,
  FIFA96_REPLAY_HEADINGS = 0x120,    /* packed heading dwords, 4 bytes each */
  FIFA96_REPLAY_APPEARANCE = 0x17C,  /* 23 appearance bytes */
  FIFA96_REPLAY_FRAMES = 0x193,      /* 23 frame bytes */
  FIFA96_REPLAY_CAMERA_BYTES = 0x1AA,/* roll / queue pop / band / flag */
  FIFA96_REPLAY_MATRIX = 0x1AE,      /* six camera dwords */
  FIFA96_REPLAY_QUEUE6 = 0x1C6,      /* six bytes */
  FIFA96_REPLAY_WORD_1CC = 0x1CC,    /* `word_55C70` */
  FIFA96_REPLAY_WORD_1CE = 0x1CE,    /* `word_55C72`: the stamp on apply */

  FIFA96_REPLAY_REC_FOCUS = 0x00,
  FIFA96_REPLAY_REC_X = 0x0C,        /* 23 words */
  FIFA96_REPLAY_REC_Z = 0x3A,        /* 23 words */
  FIFA96_REPLAY_REC_Y = 0x68,        /* 23 words */
  FIFA96_REPLAY_REC_HEADINGS = 0x96, /* 23 words */
  FIFA96_REPLAY_REC_APPEARANCE = 0xC4,
  FIFA96_REPLAY_REC_FRAMES = 0xDB,
  FIFA96_REPLAY_REC_CAMERA_BYTES = 0xF2,
  FIFA96_REPLAY_REC_MATRIX = 0xF6,
  FIFA96_REPLAY_REC_QUEUE6 = 0x10E,
  FIFA96_REPLAY_REC_WORD_1CC = 0x114,
  FIFA96_REPLAY_REC_STAMP = 0x116,
};

/* The recorder's native constants (spec §5.3). */
enum {
  FIFA96_REPLAY_BUDGET_LIMIT = 0x5A,  /* `[0x107310] < 0x5A` gates the pack */
  FIFA96_REPLAY_EVICT_COUNT = 0x95,    /* evict while the count exceeds 149 */
};

static uint16_t replay_get_word(const uint8_t *block, size_t off) {
  uint16_t value;
  memcpy(&value, block + off, sizeof value);
  return value;
}

static uint32_t replay_get_dword(const uint8_t *block, size_t off) {
  uint32_t value;
  memcpy(&value, block + off, sizeof value);
  return value;
}

static void replay_put_word(uint8_t *block, size_t off, uint16_t value) {
  memcpy(block + off, &value, sizeof value);
}

static void replay_put_dword(uint8_t *block, size_t off, uint32_t value) {
  memcpy(block + off, &value, sizeof value);
}

/* The applier's MOVSX: a native position word becomes a signed dword. */
static int32_t replay_sign_extend(uint16_t word) {
  return (word & 0x8000u) ? (int32_t)word - 0x10000 : (int32_t)word;
}

void fifa96_match_replay_pack(const uint8_t *view, uint8_t *record,
                              uint16_t stamp) {
  size_t i;

  if (!view || !record) return;

  /* The focus triple copies verbatim. */
  for (i = 0; i < 3; i++)
    replay_put_dword(record, FIFA96_REPLAY_REC_FOCUS + 4 * i,
                     replay_get_dword(view, FIFA96_REPLAY_FOCUS + 4 * i));

  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++) {
    size_t slot = FIFA96_REPLAY_SLOTS + FIFA96_REPLAY_SLOT_STRIDE * i;
    /* The three position arrays are stored as words in the native order
     * x, z, y (`+0x0C`, `+0x3A`, `+0x68`). */
    replay_put_word(record, FIFA96_REPLAY_REC_X + 2 * i,
                    replay_get_word(view, slot + 0x0));
    replay_put_word(record, FIFA96_REPLAY_REC_Z + 2 * i,
                    replay_get_word(view, slot + 0x8));
    replay_put_word(record, FIFA96_REPLAY_REC_Y + 2 * i,
                    replay_get_word(view, slot + 0x4));
    replay_put_word(record, FIFA96_REPLAY_REC_HEADINGS + 2 * i,
                    replay_get_word(view, FIFA96_REPLAY_HEADINGS + 4 * i));
    record[FIFA96_REPLAY_REC_APPEARANCE + i] =
        view[FIFA96_REPLAY_APPEARANCE + i];
    record[FIFA96_REPLAY_REC_FRAMES + i] = view[FIFA96_REPLAY_FRAMES + i];
  }

  memcpy(record + FIFA96_REPLAY_REC_CAMERA_BYTES,
         view + FIFA96_REPLAY_CAMERA_BYTES, 4);
  memcpy(record + FIFA96_REPLAY_REC_MATRIX, view + FIFA96_REPLAY_MATRIX, 0x18);
  memcpy(record + FIFA96_REPLAY_REC_QUEUE6, view + FIFA96_REPLAY_QUEUE6, 6);
  memcpy(record + FIFA96_REPLAY_REC_WORD_1CC, view + FIFA96_REPLAY_WORD_1CC, 2);
  /* The asymmetric lane: the packer stamps the record's `+0x116` word. */
  replay_put_word(record, FIFA96_REPLAY_REC_STAMP, stamp);
}

void fifa96_match_replay_unpack(const uint8_t *record, uint8_t *view) {
  size_t i;

  if (!record || !view) return;

  for (i = 0; i < 3; i++)
    replay_put_dword(view, FIFA96_REPLAY_FOCUS + 4 * i,
                     replay_get_dword(record, FIFA96_REPLAY_REC_FOCUS + 4 * i));

  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++) {
    size_t slot = FIFA96_REPLAY_SLOTS + FIFA96_REPLAY_SLOT_STRIDE * i;
    /* The applier sign-extends each position word into the slot's dword. */
    replay_put_dword(
        view, slot + 0x0,
        (uint32_t)replay_sign_extend(
            replay_get_word(record, FIFA96_REPLAY_REC_X + 2 * i)));
    replay_put_dword(
        view, slot + 0x4,
        (uint32_t)replay_sign_extend(
            replay_get_word(record, FIFA96_REPLAY_REC_Y + 2 * i)));
    replay_put_dword(
        view, slot + 0x8,
        (uint32_t)replay_sign_extend(
            replay_get_word(record, FIFA96_REPLAY_REC_Z + 2 * i)));
    /* The heading word; the dword's high word is left for the composer. */
    replay_put_word(view, FIFA96_REPLAY_HEADINGS + 4 * i,
                    replay_get_word(record, FIFA96_REPLAY_REC_HEADINGS + 2 * i));
    view[FIFA96_REPLAY_APPEARANCE + i] = record[FIFA96_REPLAY_REC_APPEARANCE + i];
    view[FIFA96_REPLAY_FRAMES + i] = record[FIFA96_REPLAY_REC_FRAMES + i];
  }

  memcpy(view + FIFA96_REPLAY_CAMERA_BYTES,
         record + FIFA96_REPLAY_REC_CAMERA_BYTES, 4);
  memcpy(view + FIFA96_REPLAY_MATRIX, record + FIFA96_REPLAY_REC_MATRIX, 0x18);
  memcpy(view + FIFA96_REPLAY_QUEUE6, record + FIFA96_REPLAY_REC_QUEUE6, 6);
  memcpy(view + FIFA96_REPLAY_WORD_1CC, record + FIFA96_REPLAY_REC_WORD_1CC, 2);
  /* The stamp lands in the view's `+0x1CE` (`word_55C72`), not `+0x1CC`. */
  replay_put_word(view, FIFA96_REPLAY_WORD_1CE,
                  replay_get_word(record, FIFA96_REPLAY_REC_STAMP));
}

/* The mode-window increment set (spec §5.3, FU-111 §1): the phases whose
 * frames advance `[0x107310]`. Every other phase resets the counter to 0, so
 * packing resumes immediately in a reset mode. */
static int replay_phase_increments_budget(uint8_t phase) {
  switch (phase) {
    case 0x00:
    case 0x01:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x13:
    case 0x14:
      return 1;
    default:
      return 0;
  }
}

void fifa96_match_replay_ring_reset(struct fifa96_match_run *mr) {
  if (!mr) return;

  mr->replay_ring.read = 0;
  mr->replay_ring.write = 0;
  mr->replay_ring.count = 0;
  mr->replay_ring.stamp_acc = 0;
  mr->replay_ring.budget = 0;
  /* The native `FUN_00064030` writes `*[0x109AB8]`, `*[0x109ABC]`,
   * `[0x109A94]`, `[0x109AAC]`, `*[0x109AC0]` (all zero); it does NOT write
   * the recorder gate `[0x109A90]` or the mode-window budget `[0x107310]`. The
   * engine zeroes the budget as an adapter but preserves the gate — the gate
   * is toggled only by `FUN_00064074`/`FUN_00064080` (the overlay latch's
   * `fifa96_match_replay_ring_record` edges). */
  mr->render.replay.cursor = 0;
  mr->render.replay.cursor_limit = 0;
}

void fifa96_match_replay_ring_record(struct fifa96_match_run *mr, int on) {
  if (!mr) return;

  mr->replay_ring.enabled = on ? 1 : 0;
}

/* The native folded heading (spec §5.2): `((0x400 - face7d) & 0x3FF) << 6`.
 * The staged slot carries the raw high word (`face7d << 16`), so the composer
 * folds it once; the replay staging unfolds it back. */
static uint32_t replay_packed_heading(int32_t heading) {
  return ((0x400u - ((uint32_t)heading >> 16)) & 0x3FFu) << 6;
}

/* One slot's view lane (`x/y/z` dwords at `+0x0C+0x0C*i`, the packed heading
 * dword at `+0x120+4*i`, appearance and frame bytes). A hidden slot replaces
 * only y with the native sentinel. */
static void replay_view_put_slot(uint8_t *view, size_t slot,
                                 const fifa96_render_entity *stage) {
  size_t base = FIFA96_REPLAY_SLOTS + FIFA96_REPLAY_SLOT_STRIDE * slot;

  replay_put_dword(view, base + 0x0, (uint32_t)stage->pos.x);
  replay_put_dword(view, base + 0x4,
                   stage->hidden
                       ? (uint32_t)(int32_t)FIFA96_MATCH_REPLAY_VIEW_HIDDEN_Y
                       : (uint32_t)stage->pos.y);
  replay_put_dword(view, base + 0x8, (uint32_t)stage->pos.z);
  replay_put_dword(view, FIFA96_REPLAY_HEADINGS + 4 * slot,
                   replay_packed_heading(stage->heading));
  view[FIFA96_REPLAY_APPEARANCE + slot] = stage->anim_id;
  view[FIFA96_REPLAY_FRAMES + slot] = stage->frame;
}

void fifa96_match_replay_compose(struct fifa96_match_run *mr) {
  struct fifa96_match_run_render *r;
  const fifa96_camera_handler_state *cam;
  fifa96_camera_focus focus;
  uint8_t *view;
  int32_t fx, fy, fz;
  size_t i;

  if (!mr) return;
  r = &mr->render;
  view = r->replay.replay_view;

  /* The focus triple: the native FU-71 place (or its override), clamped. */
  if (r->render_focus_valid) {
    fx = r->render_focus_x;
    fy = r->render_focus_y;
    fz = r->render_focus_z;
  } else if (mr->camera_override_157a6f) {
    fx = mr->camera_override_triple[0];
    fy = mr->camera_override_triple[1];
    fz = mr->camera_override_triple[2];
  } else {
    fx = r->camera.pos_x;
    fy = r->camera.pos_y;
    fz = r->camera.pos_z;
  }
  (void)fifa96_camera_focus_clamp(fx, fy, fz, &focus);
  replay_put_dword(view, FIFA96_REPLAY_FOCUS + 0x0, (uint32_t)focus.x);
  replay_put_dword(view, FIFA96_REPLAY_FOCUS + 0x4, (uint32_t)focus.y);
  replay_put_dword(view, FIFA96_REPLAY_FOCUS + 0x8, (uint32_t)focus.z);

  /* All 23 slots: 22 team slots plus the staged ball at slot 22. */
  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++)
    replay_view_put_slot(view, i, &r->entities[i].stage);

  /* RR-L3: the band gate and the queue pop run before the cluster is written.
   * The band gate sets the bit-0x40 latch in `replay_view_flags_155c4e`; that
   * latch persists until init/begin (the per-goal clear arm is not ported, so
   * the gate does not re-fire for a later goal). The queue pop is outside the
   * gate. */
  if (mr->goal_counter_15781c != 0 &&
      (mr->replay_view_flags_155c4e & 0x40) == 0) {
    mr->replay_view_flags_155c4e |= 0x40;
    mr->replay_band_155c50 =
        (uint8_t)fifa96_camera_replay_band(focus.x, focus.y);
    mr->replay_band_flag_155c51 = 1;
  }
  {
    uint32_t raw = 0;
    if (fifa96_match_run_replay_fifo_pop(mr, &raw))
      mr->replay_queue_pop_155c4f = (uint8_t)(raw + 1);
    else
      mr->replay_queue_pop_155c4f = 0;
  }

  /* The camera cluster: the roll byte (merged with the band-latch flag bits),
   * the queue-pop byte, the band and the flag, the six queue bytes (staged via
   * `replay_queue6`; the object-array producer is a numbered leg), the
   * `FUN_000974D0` stub 1 and the `FUN_000492CC` frame-delta stand-in
   * (RR-L6). */
  view[FIFA96_REPLAY_CAMERA_BYTES + 0] =
      (uint8_t)(r->ball_roll | (mr->replay_view_flags_155c4e & 0xC0));
  view[FIFA96_REPLAY_CAMERA_BYTES + 1] = mr->replay_queue_pop_155c4f;
  view[FIFA96_REPLAY_CAMERA_BYTES + 2] = mr->replay_band_155c50;
  view[FIFA96_REPLAY_CAMERA_BYTES + 3] = mr->replay_band_flag_155c51;
  memcpy(view + FIFA96_REPLAY_QUEUE6, mr->replay_queue6,
         sizeof mr->replay_queue6);
  replay_put_word(view, FIFA96_REPLAY_WORD_1CC, 1);
  replay_put_word(view, FIFA96_REPLAY_WORD_1CE, mr->state.frame_delta);

  /* The matrix is the selected live view class when it is a valid 0..2 view,
   * else the class-3 action camera. */
  if (r->native_view_class >= 0 && r->native_view_class < 3 &&
      r->native_view_valid[r->native_view_class])
    cam = &r->native_view_cam[r->native_view_class];
  else
    cam = &r->action_cam;
  replay_put_dword(view, FIFA96_REPLAY_MATRIX + 0x00, (uint32_t)cam->pos_x);
  replay_put_dword(view, FIFA96_REPLAY_MATRIX + 0x04, (uint32_t)cam->pos_y);
  replay_put_dword(view, FIFA96_REPLAY_MATRIX + 0x08, (uint32_t)cam->pos_z);
  replay_put_dword(view, FIFA96_REPLAY_MATRIX + 0x0C, (uint32_t)cam->yaw);
  replay_put_dword(view, FIFA96_REPLAY_MATRIX + 0x10, (uint32_t)cam->pitch);
  replay_put_dword(view, FIFA96_REPLAY_MATRIX + 0x14, (uint32_t)cam->ratio);
}

void fifa96_match_replay_capture(struct fifa96_match_run *mr, uint32_t ticks) {
  struct fifa96_match_replay_ring *ring;

  if (!mr) return;
  ring = &mr->replay_ring;

  /* The budget updates first and blocks before the accumulator moves: the
   * native does not call the packer at all on a blocked frame. */
  if (replay_phase_increments_budget(mr->state.phase))
    ring->budget++;
  else
    ring->budget = 0;
  if (ring->budget >= FIFA96_REPLAY_BUDGET_LIMIT) return;

  if (!ring->enabled) return;

  if (ticks == 0) {
    /* The native flush: stamp 1 and write the record immediately. */
    ring->stamp_acc = 1;
  } else {
    ring->stamp_acc += ticks;
    if (ring->stamp_acc < 2) return;
  }

  if (ring->count > FIFA96_REPLAY_EVICT_COUNT) {
    ring->read = (ring->read + 1) % FIFA96_MATCH_REPLAY_RING_RECORDS;
    ring->count--;
  }

  fifa96_match_replay_pack(mr->render.replay.replay_view,
                           ring->record[ring->write],
                           (uint16_t)ring->stamp_acc);
  ring->write = (ring->write + 1) % FIFA96_MATCH_REPLAY_RING_RECORDS;
  ring->count++;
  ring->stamp_acc = 0;
}

uint32_t fifa96_match_replay_count(const struct fifa96_match_run *mr) {
  return mr ? mr->replay_ring.count : 0;
}

int fifa96_match_replay_apply(struct fifa96_match_run *mr, uint32_t index) {
  struct fifa96_match_replay_ring *ring;
  uint32_t idx;

  if (!mr) return 0;
  ring = &mr->replay_ring;
  if (index >= ring->count) return 0;
  idx = (ring->read + index) % FIFA96_MATCH_REPLAY_RING_RECORDS;
  fifa96_match_replay_unpack(ring->record[idx],
                             mr->render.replay.replay_view);
  /* The applier also latches the roll byte from the camera cluster. */
  mr->render.ball_roll =
      mr->render.replay.replay_view[FIFA96_REPLAY_CAMERA_BYTES];
  return 1;
}

int fifa96_match_replay_step_forward(struct fifa96_match_run *mr) {
  struct fifa96_match_replay_ring *ring;
  struct fifa96_match_run_replay *r;

  if (!mr) return 0;
  ring = &mr->replay_ring;
  r = &mr->render.replay;
  r->cursor_limit = ring->count;
  if (r->cursor >= ring->count) return 0;   /* the boundary is a no-op */
  fifa96_match_replay_apply(mr, r->cursor);
  r->cursor++;
  if (r->cursor >= ring->count) r->state = FIFA96_MATCH_REPLAY_IDLE;
  return 1;
}

int fifa96_match_replay_step_back(struct fifa96_match_run *mr) {
  struct fifa96_match_replay_ring *ring;
  struct fifa96_match_run_replay *r;

  if (!mr) return 0;
  ring = &mr->replay_ring;
  r = &mr->render.replay;
  r->cursor_limit = ring->count;
  if (r->cursor == 0) return 0;             /* the boundary is a no-op */
  r->cursor--;
  fifa96_match_replay_apply(mr, r->cursor);
  if (r->cursor < 1) r->state = FIFA96_MATCH_REPLAY_IDLE;
  return 1;
}

uint16_t fifa96_match_replay_stamp(const struct fifa96_match_run *mr) {
  uint32_t idx;

  if (!mr) return 0;
  if (mr->render.replay.cursor >= mr->replay_ring.count) return 0;
  idx = (mr->replay_ring.read + mr->render.replay.cursor) %
        FIFA96_MATCH_REPLAY_RING_RECORDS;
  return replay_get_word(mr->replay_ring.record[idx],
                         FIFA96_REPLAY_REC_STAMP);
}

void fifa96_match_replay_pose(struct fifa96_match_run *mr) {
  struct fifa96_match_run_render *render;
  const uint8_t *view;

  if (!mr) return;
  render = &mr->render;
  if (render->replay.camera_kind != FIFA96_CAMERA_REPLAY_POSE) return;
  view = render->replay.replay_view;
  render->render_eye_x = (int32_t)replay_get_dword(view, FIFA96_REPLAY_MATRIX + 0x00);
  render->render_eye_y = (int32_t)replay_get_dword(view, FIFA96_REPLAY_MATRIX + 0x04);
  render->render_eye_z = (int32_t)replay_get_dword(view, FIFA96_REPLAY_MATRIX + 0x08);
  render->render_eye_valid = 1;
  render->yaw = (int32_t)replay_get_dword(view, FIFA96_REPLAY_MATRIX + 0x0C);
  render->pitch = (int32_t)replay_get_dword(view, FIFA96_REPLAY_MATRIX + 0x10);
  render->view_ratio = (int32_t)replay_get_dword(view, FIFA96_REPLAY_MATRIX + 0x14);
}

void fifa96_match_replay_stage(struct fifa96_match_run *mr) {
  struct fifa96_match_run_render *r;
  const uint8_t *view;
  size_t i;

  if (!mr) return;
  r = &mr->render;
  view = r->replay.replay_view;

  /* All 23 slots: 22 team records plus the composed ball at slot 22. The
   * composer already filled slot 22 from the staged ball, so this loop covers
   * the whole view block uniformly. */
  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++) {
    struct fifa96_match_run_entity *e = &r->entities[i];
    size_t base = FIFA96_REPLAY_SLOTS + FIFA96_REPLAY_SLOT_STRIDE * i;
    uint16_t packed = replay_get_word(view, FIFA96_REPLAY_HEADINGS + 4 * i);

    e->stage.pos.x = (int32_t)replay_get_dword(view, base + 0x0);
    e->stage.pos.y = (int32_t)replay_get_dword(view, base + 0x4);
    e->stage.pos.z = (int32_t)replay_get_dword(view, base + 0x8);
    /* The composer folded the raw heading once (`((0x400 - face7d) & 0x3FF)
     * << 6`); unfold it back into the high word so `fifa96_render_slot_stage`
     * folds it exactly once at draw time. */
    e->stage.heading =
        (int32_t)(((0x400u - ((uint32_t)packed >> 6)) & 0x3FFu) << 16);
    e->stage.anim_id = view[FIFA96_REPLAY_APPEARANCE + i];
    e->stage.frame = view[FIFA96_REPLAY_FRAMES + i];
    e->stage.hidden =
        (e->stage.pos.y == FIFA96_MATCH_REPLAY_VIEW_HIDDEN_Y) ? 1 : 0;
    (void)fifa96_match_run_anim_bank((int32_t)e->stage.anim_id, &e->bank_index);
  }
  r->entity_count = FIFA96_MATCH_REPLAY_VIEW_SLOTS;
}
