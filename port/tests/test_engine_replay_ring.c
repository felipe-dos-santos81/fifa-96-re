/* port/tests/test_engine_replay_ring.c — recorded-frame replay ring Task 1/2:
 * the native 0x155AA4 view block, the 0x118 record pack/apply pair, and the
 * ring cursors with the recorder gate, the packer cadence and the 90-frame
 * mode-window budget.
 *
 * The record map asserted here is spec §5.1 (fresh disassembly of the packer
 * `0x6408C` and the applier `0x63B80`): the byte lanes pre-increment, so the
 * appearance lane starts at record +0xC4 and the frame lane at +0xDB, and the
 * +0x116 stamp is the one asymmetric lane (pack writes the accumulator,
 * unpack writes it into the view's +0x1CE word).
 *
 * The capture cadence is spec §5.3: the budget gate runs first and returns
 * before the accumulator moves, so an uninterrupted increment-mode window
 * yields 44 records (packs at frames 2, 4, .., 88; the 90th frame is blocked),
 * a reset phase clears the budget and packing resumes, and the 151st pack
 * evicts the oldest record. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_match_replay_ring.h"
#include "fifa96_engine/fifa96_match_run.h"

/* The host is little-endian (the record/view are native byte blocks); the
 * helpers copy bytes, never a cast through an unaligned pointer. */
static void put_u16(uint8_t *block, size_t off, uint16_t value) {
  memcpy(block + off, &value, sizeof value);
}

static void put_u32(uint8_t *block, size_t off, uint32_t value) {
  memcpy(block + off, &value, sizeof value);
}

static uint16_t get_u16(const uint8_t *block, size_t off) {
  uint16_t value;
  memcpy(&value, block + off, sizeof value);
  return value;
}

static uint32_t get_u32(const uint8_t *block, size_t off) {
  uint32_t value;
  memcpy(&value, block + off, sizeof value);
  return value;
}

/* Sign-extend a native word the way the applier's MOVSX does. */
static int32_t sign_extend16(uint16_t word) {
  return (word & 0x8000u) ? (int32_t)word - 0x10000 : (int32_t)word;
}

/* Fill the view so a pack/apply round trip can be exact: the position dwords
 * are sign-extended 16-bit values, the heading dwords are 16-bit values with a
 * zero high word, and every other byte carries a distinct fill byte. */
static void fill_view(uint8_t *view) {
  static const uint32_t focus[3] = {0x11223344u, 0xDEADBEEFu, 0x0BADF00Du};
  static const uint32_t matrix[6] = {0x01020304u, 0x80000000u, 0x7FFFFFFFu,
                                     0xFEDCBA98u, 0x13579BDFu, 0x2468ACE0u};
  size_t i;

  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SIZE; i++)
    view[i] = (uint8_t)(i * 7u + 3u);

  for (i = 0; i < 3; i++) put_u32(view, 4 * i, focus[i]);

  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++) {
    size_t slot = 0x0Cu + 0x0Cu * i;
    int16_t x = (int16_t)(-0x7000 + (int)i * 0x51F);
    int16_t y = (int16_t)(0x2000 - (int)i * 0x2E7);
    int16_t z = (int16_t)((int)i * 0x7F1 - 0x5000);
    put_u32(view, slot + 0x0, (uint32_t)(int32_t)x);
    put_u32(view, slot + 0x4, (uint32_t)(int32_t)y);
    put_u32(view, slot + 0x8, (uint32_t)(int32_t)z);
    put_u32(view, 0x120 + 4 * i, (uint32_t)(uint16_t)(0x1000u + i * 0x3FFu));
  }

  for (i = 0; i < 6; i++) put_u32(view, 0x1AE + 4 * i, matrix[i]);
  put_u16(view, 0x1CC, 0xBEEFu);
  put_u16(view, 0x1CE, 0xAAAAu);
}

/* The §5.1 pack map, lane by lane. */
static void assert_pack_map(const uint8_t *view, const uint8_t *record) {
  size_t i;

  for (i = 0; i < 0x0C; i++) assert(record[i] == view[i]);

  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++) {
    size_t slot = 0x0Cu + 0x0Cu * i;
    assert(get_u16(record, 0x0C + 2 * i) == get_u16(view, slot));          /* x */
    assert(get_u16(record, 0x3A + 2 * i) == get_u16(view, slot + 0x8));    /* z */
    assert(get_u16(record, 0x68 + 2 * i) == get_u16(view, slot + 0x4));    /* y */
    assert(get_u16(record, 0x96 + 2 * i) == get_u16(view, 0x120 + 4 * i)); /* heading */
    assert(record[0xC4 + i] == view[0x17C + i]);
    assert(record[0xDB + i] == view[0x193 + i]);
  }

  assert(memcmp(record + 0xF2, view + 0x1AA, 4) == 0);
  assert(memcmp(record + 0xF6, view + 0x1AE, 0x18) == 0);
  assert(memcmp(record + 0x10E, view + 0x1C6, 6) == 0);
  assert(memcmp(record + 0x114, view + 0x1CC, 2) == 0);
  /* The asymmetric lane: the packer writes the accumulator, not a view byte. */
  assert(record[0x116] == 0x34 && record[0x117] == 0x12);
}

static void test_ring_pack_apply_roundtrip(void) {
  uint8_t view[FIFA96_MATCH_REPLAY_VIEW_SIZE];
  uint8_t record[FIFA96_MATCH_REPLAY_RECORD_SIZE];
  uint8_t applied[FIFA96_MATCH_REPLAY_VIEW_SIZE];
  size_t i;

  fill_view(view);

  /* The filled position words must actually straddle the sign bit, or the
   * sign-extension assertion below proves nothing. */
  assert(sign_extend16(get_u16(view, 0x0C)) < 0);
  assert(sign_extend16(get_u16(view, 0x10)) > 0);
  assert(sign_extend16(get_u16(view, 0x14)) < 0);

  memset(record, 0xCC, sizeof record);
  fifa96_match_replay_pack(view, record, 0x1234u);
  assert_pack_map(view, record);

  memset(applied, 0, sizeof applied);
  fifa96_match_replay_unpack(record, applied);

  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SIZE; i++) {
    if (i == 0x1CE || i == 0x1CF) continue;
    assert(applied[i] == view[i]);
  }
  /* The applier writes the record's stamp into the view's +0x1CE word; the
   * composer's pre-apply value is not restored. */
  assert(applied[0x1CE] == 0x34 && applied[0x1CF] == 0x12);
  assert(get_u16(applied, 0x1CE) == 0x1234u);

  /* Positions: the applier sign-extends the word into the dword. */
  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++) {
    size_t slot = 0x0Cu + 0x0Cu * i;
    assert((int32_t)get_u32(applied, slot + 0x0) ==
           sign_extend16(get_u16(applied, slot + 0x0)));
    assert((int32_t)get_u32(applied, slot + 0x4) ==
           sign_extend16(get_u16(applied, slot + 0x4)));
    assert((int32_t)get_u32(applied, slot + 0x8) ==
           sign_extend16(get_u16(applied, slot + 0x8)));
    /* Headings: the applier writes the word only, leaving the high word. */
    assert(get_u32(applied, 0x120 + 4 * i) ==
           (uint32_t)get_u16(view, 0x120 + 4 * i));
    assert(get_u32(applied, 0x120 + 4 * i) <= 0xFFFFu);
  }
}

/* Task 2: the mode-window budget, the two-tick packer cadence and the reset
 * phase's budget clear. A 90-frame increment window (phase 0) packs at the
 * even frames 2..88 and blocks the 90th with the accumulator left at 1. */
static void test_ring_capture_budget(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_replay_ring *ring;
  uint32_t i;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  fifa96_match_replay_ring_reset(&mr);
  fifa96_match_replay_ring_record(&mr, 1);
  ring = &mr.replay_ring;

  mr.state.phase = 0;   /* the increment set */
  for (i = 1; i <= 90; i++) {
    mr.render.replay.replay_view[0] = (uint8_t)i;
    fifa96_match_replay_capture(&mr, 1);
    if (i == 2) {
      assert(fifa96_match_replay_count(&mr) == 1);
      assert(get_u16(ring->record[0], 0x116) == 2);
    }
  }
  assert(fifa96_match_replay_count(&mr) == 44);
  assert(ring->stamp_acc == 1);
  assert(ring->budget == 90);
  for (i = 0; i < 44; i++) {
    assert(get_u16(ring->record[i], 0x116) == 2);
    /* The packer stores the view's focus dword, so the write order shows. */
    assert(ring->record[i][0] == (uint8_t)(2u * (i + 1u)));
  }

  /* The reset phase clears the budget; the persisted accumulator packs. */
  mr.state.phase = 2;
  mr.render.replay.replay_view[0] = 0xEE;
  fifa96_match_replay_capture(&mr, 1);
  assert(fifa96_match_replay_count(&mr) == 45);
  assert(ring->budget == 0);
  assert(get_u16(ring->record[44], 0x116) == 2);

  /* The next two ticks pack #46; the third leaves the accumulator at 1. */
  mr.render.replay.replay_view[0] = 0xEF;
  fifa96_match_replay_capture(&mr, 1);
  fifa96_match_replay_capture(&mr, 1);
  fifa96_match_replay_capture(&mr, 1);
  assert(fifa96_match_replay_count(&mr) == 46);
  assert(ring->stamp_acc == 1);
}

/* Task 2: the ring holds 150 records. The 151st pack evicts the oldest, so
 * record[read] is the second pack and record[read + 149] is the newest. A
 * reset phase every frame keeps the budget at 0, so the packer always runs. */
static void test_ring_eviction(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_replay_ring *ring;
  uint32_t pack, tick, oldest;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  fifa96_match_replay_ring_reset(&mr);
  fifa96_match_replay_ring_record(&mr, 1);
  ring = &mr.replay_ring;

  mr.state.phase = 2;   /* the reset set: budget 0, the packer always runs */
  for (pack = 1; pack <= 151; pack++) {
    /* Slot 0's appearance lane packs to record +0xC4. */
    mr.render.replay.replay_view[0x17C] = (uint8_t)pack;
    /* The packer needs two ticks per record. */
    for (tick = 0; tick < 2; tick++) fifa96_match_replay_capture(&mr, 1);
  }

  assert(fifa96_match_replay_count(&mr) == 150);
  assert(ring->read == 1);
  assert(ring->write == 1);
  /* record[(read + 0) % 150] is the second pack; record[(read + 149) % 150]
   * is the newest (the 151st). */
  oldest = (ring->read + 149u) % FIFA96_MATCH_REPLAY_RING_RECORDS;
  assert(ring->record[ring->read % FIFA96_MATCH_REPLAY_RING_RECORDS][0xC4] == 2);
  assert(ring->record[oldest][0xC4] == 151);
  assert(ring->record[1][0xC4] == 2);
  assert(ring->record[0][0xC4] == 151);
}

/* Task 2: an empty or disabled ring records nothing, the accumulator still
 * counts two ticks per pack, and `ticks == 0` is the native flush (stamp 1,
 * written immediately). Resetting clears every cursor and the gate. */
static void test_ring_empty_reset(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_replay_ring *ring;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  /* Set the gate before the reset: the native FUN_00064030 does not write
   * [0x109A90], so the reset must preserve it. */
  fifa96_match_replay_ring_record(&mr, 1);
  fifa96_match_replay_ring_reset(&mr);
  ring = &mr.replay_ring;

  assert(fifa96_match_replay_count(&mr) == 0);
  assert(fifa96_match_replay_count(NULL) == 0);
  assert(ring->read == 0 && ring->write == 0 && ring->count == 0);
  assert(ring->stamp_acc == 0 && ring->budget == 0 && ring->enabled == 1);
  assert(mr.render.replay.cursor == 0 && mr.render.replay.cursor_limit == 0);

  /* The gate is off: two ticks record nothing. */
  fifa96_match_replay_ring_record(&mr, 0);
  fifa96_match_replay_capture(&mr, 1);
  fifa96_match_replay_capture(&mr, 1);
  assert(fifa96_match_replay_count(&mr) == 0);

  /* The gate on: the first tick only accumulates, the second packs. */
  fifa96_match_replay_ring_record(&mr, 1);
  fifa96_match_replay_capture(&mr, 1);
  assert(fifa96_match_replay_count(&mr) == 0);
  assert(ring->stamp_acc == 1);
  fifa96_match_replay_capture(&mr, 1);
  assert(fifa96_match_replay_count(&mr) == 1);
  assert(get_u16(ring->record[0], 0x116) == 2);

  /* `ticks == 0` is the flush: stamp 1 and write immediately. */
  fifa96_match_replay_ring_reset(&mr);
  fifa96_match_replay_ring_record(&mr, 1);
  fifa96_match_replay_capture(&mr, 0);
  assert(fifa96_match_replay_count(&mr) == 1);
  assert(ring->stamp_acc == 0);
  assert(get_u16(ring->record[0], 0x116) == 1);

  fifa96_match_replay_ring_reset(&mr);
  /* The gate is still 1 from the flush test's `record(1)`: the reset preserves
   * the recorder gate. */
  assert(ring->read == 0 && ring->write == 0 && ring->count == 0);
  assert(ring->stamp_acc == 0 && ring->budget == 0 && ring->enabled == 1);
  assert(mr.render.replay.cursor == 0 && mr.render.replay.cursor_limit == 0);
}

/* Task 2 (RR-L2): the recorder overlay gate. The disable edge closes the gate
 * and latches the overlay; the re-enable edge reopens the gate and clears the
 * latch; a repeat of either edge is a no-op (the `[0x14E574]` one-shot). A
 * closed gate packs nothing even with the two-tick cadence satisfied, and the
 * ring reset preserves the gate (the native FUN_00064030 does not write
 * [0x109A90]). */
static void test_recorder_overlay_gate(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_replay_ring *ring;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  mr.replay_ring.enabled = 1;
  ring = &mr.replay_ring;

  /* disable: gate 0, latch 1, returns 1; a closed gate packs nothing. */
  assert(fifa96_match_run_recorder_overlay_disable(&mr) == 1);
  assert(ring->enabled == 0);
  assert(mr.recorder_latch_14e574 == 1);
  fifa96_match_replay_compose(&mr);
  fifa96_match_replay_capture(&mr, 1);
  fifa96_match_replay_capture(&mr, 1);
  assert(fifa96_match_replay_count(&mr) == 0);

  /* a repeat disable is a no-op: state unchanged. */
  assert(fifa96_match_run_recorder_overlay_disable(&mr) == 0);
  assert(ring->enabled == 0);
  assert(mr.recorder_latch_14e574 == 1);

  /* re-enable: gate 1, latch 0, returns 1; the two-tick cadence packs one. */
  assert(fifa96_match_run_recorder_overlay_reenable(&mr) == 1);
  assert(ring->enabled == 1);
  assert(mr.recorder_latch_14e574 == 0);
  fifa96_match_replay_compose(&mr);
  fifa96_match_replay_capture(&mr, 1);
  fifa96_match_replay_capture(&mr, 1);
  assert(fifa96_match_replay_count(&mr) == 1);

  /* the ring reset preserves the gate and the latch. */
  fifa96_match_replay_ring_reset(&mr);
  assert(ring->enabled == 1);
  assert(mr.recorder_latch_14e574 == 0);
}

/* Task 2 (RR-L2): init clears the gate and the latch even on a 0xAA-filled run
 * (the run is not memset by init). */
static void test_ring_init_clears_gate(void) {
  struct fifa96_match_run mr;
  memset(&mr, 0xAA, sizeof mr);
  fifa96_match_run_init(&mr);
  assert(mr.replay_ring.enabled == 0);
  assert(mr.recorder_latch_14e574 == 0);
}

/* Task 3: the composer fills the native `0x155AA4` view block from the staged
 * render slots (spec §5.2/§7.3). The view is pre-filled with 0xCC so every
 * asserted lane proves a write, not a zero-initialised coincidence. The first
 * block drives the `render_focus_*` arm and native view class 0; the second
 * drives the `0x157A6F` override triple through the native clamp fallback and
 * the class-3 `action_cam` matrix fallback. */
static void test_ring_compose(void) {
  struct fifa96_match_run mr;
  uint8_t *view;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  mr.running = 1;
  mr.live_start = 1;
  mr.state.frame_delta = 2;
  view = mr.render.replay.replay_view;
  memset(view, 0xCC, FIFA96_MATCH_REPLAY_VIEW_SIZE);

  /* Slot 0: a visible team slot. */
  mr.render.entities[0].stage.pos.x = 11;
  mr.render.entities[0].stage.pos.y = 22;
  mr.render.entities[0].stage.pos.z = 33;
  mr.render.entities[0].stage.heading = 0x3FF << 16;
  mr.render.entities[0].stage.anim_id = 7;
  mr.render.entities[0].stage.frame = 4;
  mr.render.entities[0].stage.hidden = 0;
  /* Slot 1: hidden, so only its y lane carries the sentinel. */
  mr.render.entities[1].stage.pos.x = 1;
  mr.render.entities[1].stage.pos.y = 2;
  mr.render.entities[1].stage.pos.z = 3;
  mr.render.entities[1].stage.hidden = 1;
  /* Slot 22: the staged ball. */
  mr.render.entities[22].stage.pos.x = 44;
  mr.render.entities[22].stage.pos.y = 55;
  mr.render.entities[22].stage.pos.z = 66;
  mr.render.entities[22].stage.heading = 0x123 << 16;
  mr.render.entities[22].stage.anim_id = 9;
  mr.render.entities[22].stage.frame = 0;

  mr.render.entity_count = 23;
  mr.render.ball_roll = 0xA5;
  mr.render.native_view_class = 0;
  mr.render.native_view_valid[0] = 1;
  mr.render.native_view_cam[0].pos_x = -10;
  mr.render.native_view_cam[0].pos_y = 20;
  mr.render.native_view_cam[0].pos_z = 30;
  mr.render.native_view_cam[0].yaw = 0x111;
  mr.render.native_view_cam[0].pitch = 0x222;
  mr.render.native_view_cam[0].ratio = 0x333;
  mr.render.render_focus_valid = 1;
  mr.render.render_focus_x = 100;
  mr.render.render_focus_y = 200;
  mr.render.render_focus_z = 300;

  fifa96_match_replay_compose(&mr);

  /* Focus triple: `render_focus_*` wins and is inside the clamp box. */
  assert((int32_t)get_u32(view, 0x00) == 100);
  assert((int32_t)get_u32(view, 0x04) == 200);
  assert((int32_t)get_u32(view, 0x08) == 300);

  /* Slot 0: the staged x/y/z, the folded heading, appearance and frame. */
  assert((int32_t)get_u32(view, 0x0C) == 11);
  assert((int32_t)get_u32(view, 0x10) == 22);
  assert((int32_t)get_u32(view, 0x14) == 33);
  assert(get_u32(view, 0x120) == (uint32_t)(((0x400 - 0x3FF) & 0x3FF) << 6));
  assert(view[0x17C] == 7);
  assert(view[0x193] == 4);

  /* Slot 1: hidden writes the native -10000 y sentinel. */
  assert((int32_t)get_u32(view, 0x10 + 0xC) == FIFA96_MATCH_REPLAY_VIEW_HIDDEN_Y);
  assert((int32_t)get_u32(view, 0x10 + 0xC) == -10000);

  /* Slot 22: the staged ball, same heading derivation. */
  assert((int32_t)get_u32(view, 0x0C + 0xC * 22) == 44);
  assert((int32_t)get_u32(view, 0x10 + 0xC * 22) == 55);
  assert((int32_t)get_u32(view, 0x14 + 0xC * 22) == 66);
  assert(get_u32(view, 0x120 + 4 * 22) ==
         (uint32_t)(((0x400 - 0x123) & 0x3FF) << 6));
  assert(view[0x17C + 22] == 9);
  assert(view[0x193 + 22] == 0);

  /* The camera cluster: roll, staged-zero queue bytes, the `=1` word, the
   * frame-delta stamp, and the six-dword matrix in the *view* offsets. */
  assert(view[0x1AA] == 0xA5);
  assert(view[0x1AB] == 0 && view[0x1AC] == 0 && view[0x1AD] == 0);
  assert(view[0x1C6] == 0 && view[0x1C7] == 0 && view[0x1C8] == 0);
  assert(view[0x1C9] == 0 && view[0x1CA] == 0 && view[0x1CB] == 0);
  assert(get_u16(view, 0x1CC) == 1);
  assert(get_u16(view, 0x1CE) == 2);
  assert((int32_t)get_u32(view, 0x1AE) == -10);
  assert((int32_t)get_u32(view, 0x1B2) == 20);
  assert((int32_t)get_u32(view, 0x1B6) == 30);
  assert((int32_t)get_u32(view, 0x1BA) == 0x111);
  assert((int32_t)get_u32(view, 0x1BE) == 0x222);
  assert((int32_t)get_u32(view, 0x1C2) == 0x333);

  /* Second block: the override triple is out of the clamp box, so all three
   * fall back to (-4000, 0, -4000); class 3 selects the action camera. */
  mr.render.render_focus_valid = 0;
  mr.camera_override_157a6f = 1;
  mr.camera_override_triple[0] = 9000;
  mr.camera_override_triple[1] = 20000;
  mr.camera_override_triple[2] = -9000;
  mr.render.native_view_class = 3;
  mr.render.action_cam.pos_x = 1;
  mr.render.action_cam.pos_y = 2;
  mr.render.action_cam.pos_z = 3;
  mr.render.action_cam.yaw = 4;
  mr.render.action_cam.pitch = 5;
  mr.render.action_cam.ratio = 6;

  fifa96_match_replay_compose(&mr);

  assert((int32_t)get_u32(view, 0x00) == -4000);
  assert((int32_t)get_u32(view, 0x04) == 0);
  assert((int32_t)get_u32(view, 0x08) == -4000);
  assert((int32_t)get_u32(view, 0x1AE) == 1);
  assert((int32_t)get_u32(view, 0x1B2) == 2);
  assert((int32_t)get_u32(view, 0x1B6) == 3);
  assert((int32_t)get_u32(view, 0x1BA) == 4);
  assert((int32_t)get_u32(view, 0x1BE) == 5);
  assert((int32_t)get_u32(view, 0x1C2) == 6);
}

/* Task 4: fill the ring with 150 records, each stamped 2 with a slot-0
 * appearance marker equal to its index. The markers ride the `+0xC4`
 * appearance lane and the stamp the `+0x116` word, so the playback apply can
 * be observed through the view's `+0x17C` byte. */
static void fill_ring_150(struct fifa96_match_run *mr) {
  struct fifa96_match_replay_ring *ring = &mr->replay_ring;
  for (uint32_t i = 0; i < FIFA96_MATCH_REPLAY_RING_RECORDS; i++) {
    ring->record[i][0xC4] = (uint8_t)i;
    put_u16(ring->record[i], 0x116, 2);
  }
  ring->read = 0;
  ring->write = 0;
  ring->count = FIFA96_MATCH_REPLAY_RING_RECORDS;
}

/* Task 4: the PLAYING countdown. With every record stamped 2, the ring tick
 * (1) accumulates in `advance` and one record plays every two calls. After 300
 * calls the cursor reaches the end and the state settles at IDLE. Re-seeding
 * IDLE at cursor 0 shows button 1 enters PLAYING without applying (the native
 * only rewinds/applies at the end). */
static void test_ring_playback_cadence(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_run_replay *r;
  uint32_t i;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  r = &mr.render.replay;
  fill_ring_150(&mr);

  r->state = FIFA96_MATCH_REPLAY_PLAYING;
  r->cursor = 0;
  r->cursor_limit = 150;
  r->advance = 0;

  fifa96_match_run_replay_step(&mr, 0);
  fifa96_match_run_replay_step(&mr, 0);
  assert(r->cursor == 1);   /* 2 ticks of the stamp-2 cadence = 1 record */

  for (i = 2; i < 300; i++) fifa96_match_run_replay_step(&mr, 0);
  assert(r->cursor == 150);
  assert(r->state == FIFA96_MATCH_REPLAY_IDLE);
  assert(mr.render.replay.replay_view[0x17C] == 149);   /* the last record */

  /* IDLE at cursor 0: button 1 enters PLAYING with no apply. */
  r->state = FIFA96_MATCH_REPLAY_IDLE;
  r->cursor = 0;
  r->cursor_limit = 150;
  r->advance = 0;
  mr.render.replay.replay_view[0x17C] = 0xEE;   /* the untouched sentinel */
  fifa96_match_run_replay_step(&mr, 1);
  assert(r->state == FIFA96_MATCH_REPLAY_PLAYING);
  assert(r->cursor == 0);
  assert(mr.render.replay.replay_view[0x17C] == 0xEE);
}

/* Task 4: the step/slow states on a full ring. Buttons 8/4/2 map to
 * back/forward/slow; the same call performs two back/forward steps or the
 * slow step+reload; the next call resets the step state to IDLE; button 1 at
 * the end rewinds and applies record 0. */
static void test_ring_step_modes(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_run_replay *r;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  r = &mr.render.replay;
  fill_ring_150(&mr);

  r->state = FIFA96_MATCH_REPLAY_IDLE;
  r->cursor = 10;

  fifa96_match_run_replay_step(&mr, 8);
  assert(r->state == FIFA96_MATCH_REPLAY_STEP_BACK);
  assert(r->cursor == 8);                              /* two back steps */
  assert(mr.render.replay.replay_view[0x17C] == 8);    /* applied record 8 */

  fifa96_match_run_replay_step(&mr, 0);
  assert(r->state == FIFA96_MATCH_REPLAY_IDLE);

  fifa96_match_run_replay_step(&mr, 4);
  assert(r->state == FIFA96_MATCH_REPLAY_STEP_FWD);
  assert(r->cursor == 10);                             /* two forward steps */

  r->wait_timer = 0;
  r->wait_reload = 3;
  fifa96_match_run_replay_step(&mr, 2);
  assert(r->state == FIFA96_MATCH_REPLAY_SLOW);
  assert(r->wait_timer == 3);
  assert(r->cursor == 11);                             /* steps then reloads */

  fifa96_match_run_replay_step(&mr, 2);
  assert(r->state == FIFA96_MATCH_REPLAY_SLOW);
  assert(r->wait_timer == 2);                          /* the countdown, no step */
  assert(r->cursor == 11);

  r->state = FIFA96_MATCH_REPLAY_IDLE;
  r->cursor = 150;                                     /* == count */
  fifa96_match_run_replay_step(&mr, 1);
  assert(r->state == FIFA96_MATCH_REPLAY_PLAYING);
  assert(r->cursor == 1);                              /* end rewind applied 0 */
}

/* Task 4: an empty ring arms cleanly. `replay_begin` returns 0; the first
 * step runs the 0x81 arm to IDLE with cursor 0; button 1 enters PLAYING,
 * button 0 stays, button 0x80 exits to LIVE; the view is never touched. */
static void test_ring_empty(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_run_replay *r;
  uint8_t before;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  mr.running = 1;
  mr.lc.screen = FIFA96_MATCH_SCREEN_ACTIVE;
  r = &mr.render.replay;

  assert(fifa96_match_replay_count(&mr) == 0);
  assert(fifa96_match_run_replay_begin(&mr) == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_ARMED);

  fifa96_match_run_replay_step(&mr, 0);
  assert(r->state == FIFA96_MATCH_REPLAY_IDLE);
  assert(r->cursor == 0);

  before = mr.render.replay.replay_view[0x17C];
  fifa96_match_run_replay_step(&mr, 1);
  assert(r->state == FIFA96_MATCH_REPLAY_PLAYING);
  assert(mr.render.replay.replay_view[0x17C] == before);

  fifa96_match_run_replay_step(&mr, 0);
  assert(r->state == FIFA96_MATCH_REPLAY_PLAYING);
  assert(mr.render.replay.replay_view[0x17C] == before);

  fifa96_match_run_replay_step(&mr, 0x80);
  assert(r->state == FIFA96_MATCH_REPLAY_LIVE);
  assert(r->state == 0);
}

int main(void) {
  test_ring_pack_apply_roundtrip();
  test_ring_capture_budget();
  test_ring_eviction();
  test_ring_empty_reset();
  test_recorder_overlay_gate();
  test_ring_init_clears_gate();
  test_ring_compose();
  test_ring_playback_cadence();
  test_ring_step_modes();
  test_ring_empty();
  puts("test_engine_replay_ring OK");
  return 0;
}
