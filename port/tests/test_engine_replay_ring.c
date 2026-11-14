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

  /* The camera cluster: roll, staged-zero queue bytes, the six-byte queue lane
   * (`0xFF` default, the object-array producer is a numbered leg), the `=1`
   * word, the frame-delta stamp, and the six-dword matrix in the *view*
   * offsets. */
  assert(view[0x1AA] == 0xA5);
  assert(view[0x1AB] == 0 && view[0x1AC] == 0 && view[0x1AD] == 0);
  assert(view[0x1C6] == 0xFF && view[0x1C7] == 0xFF && view[0x1C8] == 0xFF);
  assert(view[0x1C9] == 0xFF && view[0x1CA] == 0xFF && view[0x1CB] == 0xFF);
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

  /* RR-L5: class 3 is the sub-8 VIEW variant. A valid fourth state wins over
   * the action fallback; the invalid class-3 case above keeps that fallback. */
  mr.render.native_view_valid[3] = 1;
  mr.render.native_view_cam[3].pos_x = 11;
  mr.render.native_view_cam[3].pos_y = 12;
  mr.render.native_view_cam[3].pos_z = 13;
  mr.render.native_view_cam[3].yaw = 14;
  mr.render.native_view_cam[3].pitch = 15;
  mr.render.native_view_cam[3].ratio = 16;
  fifa96_match_replay_compose(&mr);
  assert((int32_t)get_u32(view, 0x1AE) == 11);
  assert((int32_t)get_u32(view, 0x1B2) == 12);
  assert((int32_t)get_u32(view, 0x1B6) == 13);
  assert((int32_t)get_u32(view, 0x1BA) == 14);
  assert((int32_t)get_u32(view, 0x1BE) == 15);
  assert((int32_t)get_u32(view, 0x1C2) == 16);
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
  mr.replay_mode_108e0c = 4;
  mr.replay_mode_acc_108e10 = 5;
  assert(fifa96_match_run_replay_begin(&mr) == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_ARMED);
  assert(mr.replay_mode_108e0c == 0);
  assert(mr.replay_mode_acc_108e10 == 0);

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

/* Task 3 (RR-L10): the 20-dword FIFO and the drain gate. The C ring uses
 * conventional indexing (`head` = next write, `tail` = oldest, both 0..19), so
 * push stores at `value[head]`, pop reads `value[tail]`, and a full push evicts
 * the tail first — preserving the native observable order/eviction, not the
 * native's pre-increment address offset (push index 1..20 of 0x155C8C vs the
 * C index 0..19 of 0x155C90). */
static void test_replay_fifo_order(void) {
  struct fifa96_match_run mr;
  uint32_t v, i;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  fifa96_match_run_replay_fifo_reset(&mr);

  for (i = 1; i <= 20; i++)
    assert(fifa96_match_run_replay_fifo_push(&mr, i) == 1);
  assert(mr.replay_fifo.count == 20);
  assert(mr.replay_fifo.head == 0);
  assert(mr.replay_fifo.tail == 0);

  for (i = 1; i <= 20; i++) {
    assert(fifa96_match_run_replay_fifo_pop(&mr, &v) == 1);
    assert(v == i);
  }
  assert(fifa96_match_run_replay_fifo_pop(&mr, &v) == 0);   /* empty */
  assert(mr.replay_fifo.count == 0);
}

static void test_replay_fifo_eviction(void) {
  struct fifa96_match_run mr;
  uint32_t v, i;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  fifa96_match_run_replay_fifo_reset(&mr);

  for (i = 1; i <= 20; i++)
    assert(fifa96_match_run_replay_fifo_push(&mr, i) == 1);
  assert(fifa96_match_run_replay_fifo_push(&mr, 21) == 1);
  assert(mr.replay_fifo.count == 20);
  assert(mr.replay_fifo.head == 1);
  assert(mr.replay_fifo.tail == 1);

  for (i = 2; i <= 21; i++) {
    assert(fifa96_match_run_replay_fifo_pop(&mr, &v) == 1);
    assert(v == i);
  }
}

static void test_replay_fifo_wrap(void) {
  struct fifa96_match_run mr;
  uint32_t v, i;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  fifa96_match_run_replay_fifo_reset(&mr);

  for (i = 1; i <= 20; i++)
    assert(fifa96_match_run_replay_fifo_push(&mr, i) == 1);
  for (i = 0; i < 5; i++)
    assert(fifa96_match_run_replay_fifo_pop(&mr, &v) == 1);
  for (i = 6; i <= 10; i++)
    assert(fifa96_match_run_replay_fifo_push(&mr, i) == 1);
  assert(mr.replay_fifo.count == 20);
  assert(mr.replay_fifo.head == 5);
  assert(mr.replay_fifo.tail == 5);

  for (i = 6; i <= 20; i++) {
    assert(fifa96_match_run_replay_fifo_pop(&mr, &v) == 1);
    assert(v == i);
  }
  for (i = 6; i <= 10; i++) {
    assert(fifa96_match_run_replay_fifo_pop(&mr, &v) == 1);
    assert(v == i);
  }
}

static void test_replay_fifo_drain_gate(void) {
  struct fifa96_match_run mr;
  uint32_t v;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  fifa96_match_run_replay_fifo_reset(&mr);

  /* state 0x80 → no push */
  mr.render.replay.state = 0x80;
  assert(fifa96_match_run_replay_fifo_drain(&mr, 42) == 0);
  assert(mr.replay_fifo.count == 0);

  /* state 0 + screen 5 → no push */
  mr.render.replay.state = 0;
  mr.replay_screen_id_105ffc = 5;
  assert(fifa96_match_run_replay_fifo_drain(&mr, 42) == 0);
  assert(mr.replay_fifo.count == 0);

  /* state 0 + screen 7 → no push */
  mr.replay_screen_id_105ffc = 7;
  assert(fifa96_match_run_replay_fifo_drain(&mr, 42) == 0);
  assert(mr.replay_fifo.count == 0);

  /* state 0 + screen 0 → push */
  mr.replay_screen_id_105ffc = 0;
  assert(fifa96_match_run_replay_fifo_drain(&mr, 42) == 1);
  assert(mr.replay_fifo.count == 1);
  assert(fifa96_match_run_replay_fifo_pop(&mr, &v) == 1);
  assert(v == 42);
}

/* Task 4 (RR-L3): the composer writes the band/flag/queue-pop bytes and merges
 * the band-latch flag bits into the roll byte. The band gate fires once per
 * goal (the bit-0x40 latch); the queue pop runs outside the gate. */
static void test_ring_compose_band_queue(void) {
  struct fifa96_match_run mr;
  uint8_t *view;
  uint8_t record[FIFA96_MATCH_REPLAY_RECORD_SIZE];

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  mr.running = 1;
  mr.live_start = 1;
  mr.state.frame_delta = 2;
  view = mr.render.replay.replay_view;
  memset(view, 0xCC, FIFA96_MATCH_REPLAY_VIEW_SIZE);

  /* Stage the focus triple, the band gate, the roll byte and the FIFO. */
  mr.render.render_focus_valid = 1;
  mr.render.render_focus_x = 0x47;
  mr.render.render_focus_y = 0x6A;
  mr.render.render_focus_z = 0;
  mr.goal_counter_15781c = 1;
  mr.render.ball_roll = 0x05;
  fifa96_match_run_replay_fifo_reset(&mr);
  assert(fifa96_match_run_replay_fifo_push(&mr, 0x2A) == 1);
  assert(fifa96_match_run_replay_fifo_push(&mr, 0x30) == 1);

  fifa96_match_replay_compose(&mr);

  assert(view[0x1AC] == 4);    /* band = classifier(0x47, 0x6A) */
  assert(view[0x1AD] == 1);    /* flag */
  assert(view[0x1AB] == 0x2B); /* queue pop = 0x2A + 1 */
  assert(view[0x1AA] == 0x45); /* roll 0x05 | the bit-0x40 latch */
  assert(mr.replay_view_flags_155c4e == 0x40);
  assert(mr.replay_band_155c50 == 4 && mr.replay_band_flag_155c51 == 1);
  assert(mr.replay_queue_pop_155c4f == 0x2B);

  /* Pack: the camera-byte lane maps view +0x1AA to record +0xF2. */
  memset(record, 0, sizeof record);
  fifa96_match_replay_pack(view, record, 1);
  assert(record[0xF2] == 0x45);

  /* Second compose: the flag is already set, so band/flag stay even when the
   * focus moves to a point that classifies differently (proving the gate
   * skipped reclassification); the queue pop consumes the next FIFO value
   * (0x30 + 1) outside the band gate. */
  mr.render.render_focus_x = -0x47;
  mr.render.render_focus_y = 0;
  fifa96_match_replay_compose(&mr);
  assert(view[0x1AC] == 4);    /* unchanged: the bit-0x40 latch skipped the band */
  assert(view[0x1AD] == 1);
  assert(view[0x1AB] == 0x31);
  assert(view[0x1AA] == 0x45);

  /* Third compose: the FIFO is empty, so the queue-pop byte is 0. */
  fifa96_match_replay_compose(&mr);
  assert(view[0x1AB] == 0);
  assert(mr.replay_queue_pop_155c4f == 0);
}

/* Task 4 (RR-L3): the `FUN_0006428C` resync sign-extends the queue-pop byte
 * and drains `value - 1` through the gate without storing back to the cell. */
static void test_replay_resync_queue_pop(void) {
  struct fifa96_match_run mr;
  uint32_t v;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  fifa96_match_run_replay_fifo_reset(&mr);

  /* State 0 (the gate passes): the cell is never stored back, so a repeated
   * resync drains the same value (2 - 1 = 1) each time. */
  mr.render.replay.state = 0;
  mr.replay_queue_pop_155c4f = 2;
  fifa96_match_run_replay_resync(&mr);
  assert(mr.replay_queue_pop_155c4f == 2);
  assert(mr.replay_fifo.count == 1);
  assert(fifa96_match_run_replay_fifo_pop(&mr, &v) == 1);
  assert(v == 1);

  fifa96_match_run_replay_resync(&mr);
  assert(mr.replay_queue_pop_155c4f == 2);
  assert(mr.replay_fifo.count == 1);
  assert(fifa96_match_run_replay_fifo_pop(&mr, &v) == 1);
  assert(v == 1);                          /* still 2 - 1, not 0 */

  /* State 0x83 (bit 0x80 set): the gate blocks, the cell is still unchanged. */
  fifa96_match_run_replay_fifo_reset(&mr);
  mr.render.replay.state = 0x83;
  mr.replay_queue_pop_155c4f = 2;
  fifa96_match_run_replay_resync(&mr);
  assert(mr.replay_queue_pop_155c4f == 2);
  assert(mr.replay_fifo.count == 0);

  /* A zero cell is a no-op: no push, no change. */
  fifa96_match_run_replay_fifo_reset(&mr);
  mr.render.replay.state = 0;
  mr.replay_queue_pop_155c4f = 0;
  fifa96_match_run_replay_resync(&mr);
  assert(mr.replay_queue_pop_155c4f == 0);
  assert(mr.replay_fifo.count == 0);

  /* Sign extension: cell 0x81 reads as -127 (SAR 0x18), so the drain receives
   * -127 - 1 = -128 = 0xFFFFFF80. */
  fifa96_match_run_replay_fifo_reset(&mr);
  mr.render.replay.state = 0;
  mr.replay_queue_pop_155c4f = 0x81;
  fifa96_match_run_replay_resync(&mr);
  assert(mr.replay_queue_pop_155c4f == 0x81);
  assert(mr.replay_fifo.count == 1);
  assert(fifa96_match_run_replay_fifo_pop(&mr, &v) == 1);
  assert(v == 0xFFFFFF80u);
}

/* RR-L3 apply latch: `fifa96_match_replay_apply` unpacks a record into the
 * view, so the current queue-pop byte is the record's (view +0x1AB = record
 * +0xF3). The native `FUN_0006428C` reads `[0x155C4C]; SAR EAX,0x18` — the
 * current view byte at `0x155C4F` — so resync must consume the applied
 * record's byte, not the last live-compose mirror. */
static void test_replay_apply_latches_queue_pop(void) {
  struct fifa96_match_run mr;
  uint32_t v;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  fifa96_match_run_replay_fifo_reset(&mr);

  /* Stage one record whose queue-pop byte (record +0xF3) is a known nonzero
   * value, and leave the live mirror at a different value so the apply
   * overwrite is observable. */
  memset(mr.replay_ring.record[0], 0, FIFA96_MATCH_REPLAY_RECORD_SIZE);
  mr.replay_ring.record[0][0xF3] = 0x2A;
  mr.replay_ring.read = 0;
  mr.replay_ring.write = 1;
  mr.replay_ring.count = 1;
  mr.replay_queue_pop_155c4f = 0x7F;   /* the stale live-compose value */

  assert(fifa96_match_replay_apply(&mr, 0) == 1);
  assert(mr.render.replay.replay_view[0x1AB] == 0x2A);
  assert(mr.replay_queue_pop_155c4f == 0x2A);   /* latched from the record */

  /* Resync with the drain gate passing (state 0, screen id 0): the FIFO
   * receives the sign-extended record byte minus one (0x2A - 1 = 0x29). */
  mr.render.replay.state = 0;
  mr.replay_screen_id_105ffc = 0;
  fifa96_match_run_replay_resync(&mr);
  assert(mr.replay_fifo.count == 1);
  assert(fifa96_match_run_replay_fifo_pop(&mr, &v) == 1);
  assert(v == 0x2Au - 1u);
}

/* Task 5 (RR-L3): the six-byte queue lane. The composer copies `replay_queue6`
 * to the view `+0x1C6..+0x1CB` lane; the packer already maps it to record
 * `+0x10E..+0x113` and the applier returns it. A fresh run (init, no staging)
 * composes the native `0xFF` default six times. */
static void test_ring_compose_queue6(void) {
  struct fifa96_match_run mr;
  uint8_t *view;
  uint8_t record[FIFA96_MATCH_REPLAY_RECORD_SIZE];
  uint8_t applied[FIFA96_MATCH_REPLAY_VIEW_SIZE];
  static const uint8_t staged[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  size_t i;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  mr.running = 1;
  mr.live_start = 1;
  view = mr.render.replay.replay_view;
  memset(view, 0xCC, FIFA96_MATCH_REPLAY_VIEW_SIZE);

  /* Stage the six bytes and compose: the lane carries them. */
  memcpy(mr.replay_queue6, staged, sizeof staged);
  fifa96_match_replay_compose(&mr);
  for (i = 0; i < 6; i++)
    assert(view[0x1C6 + i] == staged[i]);

  /* Pack with stamp 0x1234: the lane lands at record +0x10E..+0x113. */
  memset(record, 0, sizeof record);
  fifa96_match_replay_pack(view, record, 0x1234u);
  assert(memcmp(record + 0x10E, staged, 6) == 0);

  /* Unpack into a cleared view: the lane returns. */
  memset(applied, 0, sizeof applied);
  fifa96_match_replay_unpack(record, applied);
  assert(memcmp(applied + 0x1C6, staged, 6) == 0);

  /* A fresh run without staging composes the native 0xFF default. */
  {
    struct fifa96_match_run fresh;
    memset(&fresh, 0, sizeof fresh);
    fifa96_match_run_init(&fresh);
    fresh.running = 1;
    fresh.live_start = 1;
    memset(fresh.render.replay.replay_view, 0xCC,
           FIFA96_MATCH_REPLAY_VIEW_SIZE);
    fifa96_match_replay_compose(&fresh);
    for (i = 0; i < 6; i++)
      assert(fresh.render.replay.replay_view[0x1C6 + i] == 0xFF);
  }
}

/* Task 7 (RR-L4): the appearance-byte identity. The composer writes
 * `view[+0x17C+i] = stage.anim_id` byte-exact (no transform), the packer copies
 * it to record `+0xC4+i`, and the applier returns it. The ball slot 22's byte
 * is the staged row id (`0x2B`), not a composed row+offset — the same value
 * the link test proves the real stage path produces from `appear_row`. */
static void test_ring_appearance_identity(void) {
  struct fifa96_match_run mr;
  uint8_t *view;
  uint8_t record[FIFA96_MATCH_REPLAY_RECORD_SIZE];
  uint8_t applied[FIFA96_MATCH_REPLAY_VIEW_SIZE];
  size_t i;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  mr.running = 1;
  mr.live_start = 1;
  mr.state.frame_delta = 2;
  view = mr.render.replay.replay_view;
  memset(view, 0xCC, FIFA96_MATCH_REPLAY_VIEW_SIZE);

  /* Stage the appearance bytes: slots 0..21 as i + 0x20, the ball 0x2B. */
  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++)
    mr.render.entities[i].stage.anim_id =
        (i == 22) ? 0x2B : (uint8_t)(i + 0x20);

  fifa96_match_replay_compose(&mr);

  /* Identity: the composed view byte equals the staged row id, untransformed. */
  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++)
    assert(view[0x17C + i] == mr.render.entities[i].stage.anim_id);

  /* Pack: the record byte copies the composed lane verbatim. */
  memset(record, 0, sizeof record);
  fifa96_match_replay_pack(view, record, 1);
  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++)
    assert(record[0xC4 + i] == mr.render.entities[i].stage.anim_id);

  /* Unpack: the applied lane returns the same row id. */
  memset(applied, 0, sizeof applied);
  fifa96_match_replay_unpack(record, applied);
  for (i = 0; i < FIFA96_MATCH_REPLAY_VIEW_SLOTS; i++)
    assert(applied[0x17C + i] == mr.render.entities[i].stage.anim_id);

  /* The ball's record byte is the row id 0x2B, not a composed row+offset. */
  assert(mr.render.entities[22].stage.anim_id == 0x2B);
  assert(view[0x17C + 22] == 0x2B);
  assert(record[0xC4 + 22] == 0x2B);
  assert(applied[0x17C + 22] == 0x2B);
}

/* Task 6 (C4R-L1): the arm's projection-reset flag `[0x109014]`. The native
 * `FUN_0005865C` arm writes `[0x109014]=0` (0x58666) before its three
 * presentation calls — `FUN_00056B88(0x2B)`, the `0x14E9B8 >= 0x80` /3 loop
 * over 23 dwords, and `FUN_00056894` — which stay numbered legs.
 * `FUN_000585E4` is the separate setup writer that sets 1 (0x585F2); the flag
 * has several consumer functions (including the read-write FUN_000586C0), so
 * only the state is modeled here. init and the replay arm clear it; the
 * replay exit leaves it 0. */
static void test_replay_arm_view_reset(void) {
  struct fifa96_match_run mr;

  /* Fresh init writes the field even on a 0xAA-filled run (the run is not
   * memset by init). */
  memset(&mr, 0xAA, sizeof mr);
  fifa96_match_run_init(&mr);
  assert(mr.replay_view_reset_109014 == 0);

  /* The arm clears a pre-set flag and leaves it 0 through exit. */
  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  mr.running = 1;
  mr.paused = 0;
  mr.lc.screen = FIFA96_MATCH_SCREEN_ACTIVE;
  mr.replay_view_reset_109014 = 1;
  assert(fifa96_match_run_replay_begin(&mr) == 0);
  assert(mr.replay_view_reset_109014 == 0);
  assert(mr.render.replay.state == FIFA96_MATCH_REPLAY_ARMED);
  fifa96_match_run_replay_exit(&mr);
  assert(mr.replay_view_reset_109014 == 0);
}

/* Task 9 (RR-L7): the request starts the native mode window, then the ramp
 * waits for playback completion before spending its countdown. This loop
 * mirrors the frozen frame's order in fifa96_match_run.c — control(1) first,
 * then replay_step — so the pinned indices describe the integrated exit
 * timing, not the plan's Task 9 step-then-control test wording (superseded).
 * A full ring stamped 4 completes on the 600th step, but control spends the
 * phase before that step, so the phase first equals 0xF0 on control call 479
 * and 0x168 on control call 599; the wired order then clamps the phase to
 * 0x169 on call 600 (it does not stop at 0x168, and the completion lands one
 * control call later than the step-then-control order), and the 360-tick
 * countdown exits on control call 961. The empty ring completes immediately,
 * so its phase-1 countdown exits on call 241, directly after the 240-tick mode
 * window. */
static void test_replay_request_timed_exit(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_run_replay *r;
  int first_f0 = 0;
  int first_168 = 0;
  int clamped_169 = 0;
  int exit_index = 0;
  int exits = 0;
  int empty_exit = 0;
  int empty_exits = 0;

  memset(&mr, 0, sizeof mr);
  fifa96_match_run_init(&mr);
  mr.running = 1;
  mr.paused = 0;
  mr.lc.screen = FIFA96_MATCH_SCREEN_ACTIVE;
  r = &mr.render.replay;

  assert(fifa96_match_run_replay_complete(NULL) == 0);
  assert(fifa96_match_run_replay_request(&mr, 1) == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_ARMED);
  assert(r->phase_counter == 1);
  assert(mr.replay_mode_108e0c == 1);
  assert(mr.replay_mode_acc_108e10 == 0);

  fill_ring_150(&mr);
  for (uint32_t i = 0; i < FIFA96_MATCH_REPLAY_RING_RECORDS; i++)
    put_u16(mr.replay_ring.record[i], 0x116, 4);
  r->state = FIFA96_MATCH_REPLAY_PLAYING;
  r->cursor = 0;
  r->cursor_limit = mr.replay_ring.count;
  r->advance = 0;
  assert(fifa96_match_run_replay_complete(&mr) == 0);

  for (int i = 1; i <= 0x400; i++) {
    int complete_before_control;
    int rc;
    complete_before_control = fifa96_match_run_replay_complete(&mr);
    rc = fifa96_match_run_replay_control(&mr, 1);
    assert(fifa96_match_run_replay_step(&mr, 0) == 0);
    if (first_f0 == 0 && r->phase_counter >= 0xF0) {
      assert(r->phase_counter == 0xF0);
      first_f0 = i;
    }
    if (first_168 == 0 && r->phase_counter >= 0x168) {
      assert(r->phase_counter == 0x168);
      first_168 = i;
    }
    if (clamped_169 == 0 && r->phase_counter == 0x169)
      clamped_169 = i;
    if (rc != 0) {
      assert(rc == 1);
      assert(complete_before_control == 1);
      exits++;
      exit_index = i;
      assert(r->state == FIFA96_MATCH_REPLAY_LIVE);
      assert(r->phase_counter == 0);
      break;
    }
  }

  assert(exits == 1);
  assert(first_f0 == 479);
  assert(first_168 == 599);
  assert(clamped_169 == 600);
  assert(exit_index == 961);
  assert(exit_index > clamped_169 && clamped_169 > first_168 &&
         first_168 > first_f0);
  assert(fifa96_match_run_replay_complete(&mr) == 1);
  assert(r->state == FIFA96_MATCH_REPLAY_LIVE);
  assert(r->phase_counter == 0);

  /* An empty ring is already complete. Keep running the ordinary PLAYING
   * state machine (in the same control-then-step order) to prove that the
   * timed exit needs no cursor advance. */
  fifa96_match_replay_ring_reset(&mr);
  assert(fifa96_match_run_replay_complete(&mr) == 1);
  assert(fifa96_match_run_replay_request(&mr, 1) == 0);
  r->state = FIFA96_MATCH_REPLAY_PLAYING;
  r->cursor = 0;
  r->cursor_limit = 0;
  r->advance = 0;
  for (int i = 1; i <= 0x400; i++) {
    int rc;
    rc = fifa96_match_run_replay_control(&mr, 1);
    assert(fifa96_match_run_replay_step(&mr, 0) == 0);
    assert(r->cursor == 0);
    if (rc != 0) {
      assert(rc == 1);
      empty_exits++;
      empty_exit = i;
      break;
    }
  }
  assert(empty_exits == 1);
  assert(empty_exit == 241);
  assert(r->cursor == 0);
  assert(r->state == FIFA96_MATCH_REPLAY_LIVE);
  assert(r->phase_counter == 0);

  printf("replay request: phase 0xF0=%d, phase 0x168=%d, clamp 0x169=%d, "
         "exit=%d; empty exit=%d\n",
         first_f0, first_168, clamped_169, exit_index, empty_exit);
}

/* Task 9: begin validates before writing any request cell. The on=0 entry is
 * the request API's no-op; per-frame work belongs to replay_control. */
static void test_replay_request_failure_atomic(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_run_replay *r;

  memset(&mr, 0xAA, sizeof mr);
  fifa96_match_run_init(&mr);
  assert(mr.replay_mode_108e0c == 0);
  assert(mr.replay_mode_acc_108e10 == 0);
  r = &mr.render.replay;
  r->phase_counter = 0x55;
  mr.replay_mode_108e0c = 7;
  mr.replay_mode_acc_108e10 = 9;

  assert(fifa96_match_run_replay_request(&mr, 1) == -FIFA96_ERR_STATE);
  assert(r->phase_counter == 0x55);
  assert(mr.replay_mode_108e0c == 7);
  assert(mr.replay_mode_acc_108e10 == 9);
  assert(fifa96_match_run_replay_request(&mr, 0) == 0);
  assert(r->phase_counter == 0x55);
  assert(mr.replay_mode_108e0c == 7);
  assert(mr.replay_mode_acc_108e10 == 9);

  mr.running = 1;
  mr.paused = 1;
  mr.lc.screen = FIFA96_MATCH_SCREEN_ACTIVE;
  assert(fifa96_match_run_replay_request(&mr, 1) == -FIFA96_ERR_STATE);
  assert(r->phase_counter == 0x55);
  assert(mr.replay_mode_108e0c == 7);
  assert(mr.replay_mode_acc_108e10 == 9);

  mr.paused = 0;
  mr.lc.screen = FIFA96_MATCH_SCREEN_OVER;
  assert(fifa96_match_run_replay_request(&mr, 1) == -FIFA96_ERR_STATE);
  assert(r->phase_counter == 0x55);
  assert(mr.replay_mode_108e0c == 7);
  assert(mr.replay_mode_acc_108e10 == 9);
  assert(fifa96_match_run_replay_request(NULL, 1) == -FIFA96_ERR_INVALID);
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
  test_replay_fifo_order();
  test_replay_fifo_eviction();
  test_replay_fifo_wrap();
  test_replay_fifo_drain_gate();
  test_ring_compose_band_queue();
  test_replay_resync_queue_pop();
  test_replay_apply_latches_queue_pop();
  test_ring_compose_queue6();
  test_ring_appearance_identity();
  test_replay_arm_view_reset();
  test_replay_request_timed_exit();
  test_replay_request_failure_atomic();
  puts("test_engine_replay_ring OK");
  return 0;
}
