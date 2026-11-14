#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_match_handlers.h"
#include "fifa96_loader/fifa96_action_handlers.h"
#include "fifa96_loader/fifa96_animation.h"
#include "fifa96_loader/fifa96_arm_bodies.h"
#include "fifa96_loader/fifa96_arm_helpers.h"
#include "fifa96_loader/fifa96_bigf.h"
#include "fifa96_loader/fifa96_entity_update.h"
#include "fifa96_loader/fifa96_match_remap_native.h"
#include "fifa96_loader/fifa96_outfield.h"
#include "fifa96_loader/fifa96_projection.h"
#include "fifa96_loader/fifa96_record.h"
#include "fifa96_loader/fifa96_ring_stats.h"
#include "fifa96_loader/fifa96_scene.h"
#include "fifa96_loader/fifa96_sprite.h"

/* FU-145 S2: the shared phase write (definition in the phase-driver
 * section below; used by begin/reset paths above it). */
/* Phase 70: the ball-settle release (definitions in the referee section below;
 * used by the init/reset/begin and frame paths above them). */
static void match_run_ball_settle_clear(struct fifa96_match_run *mr);
static void match_run_ball_settle_step(struct fifa96_match_run *mr);
static int match_run_offside_reception_at(struct fifa96_match_run *mr,
                                          int32_t receiver,
                                          int32_t query_x, int32_t query_z,
                                          const struct fifa96_ref_metric *metric,
                                          int32_t camera_ref, uint8_t mirror,
                                          uint8_t *offside);

/* FU-72/`FUN_0008ED40`: the replay roster id (definition with the replay
 * helpers below). The stats seeding/wrapper above need it. */
static int32_t match_run_replay_roster_id(const struct fifa96_match_run *mr,
                                          uint32_t side, uint32_t slot);

/* FU-72/`FUN_0008ED40`: the 2 x 16 activity table. `reset` is the native
 * clear; `add` walks one block's slots for the key, accumulates the weighted
 * delta and updates the max triple (the native compare is on the sign-extended
 * low word, `0x8EDD5`/`0x8EDD8`). The modeled weight base `+0x8D` is always
 * 0, so the live arm is the native `2x`. */
void fifa96_match_stats_reset(struct fifa96_match_stats *s) {
  if (!s) return;
  for (uint32_t side = 0; side < 2u; side++)
    for (uint32_t slot = 0; slot < FIFA96_MATCH_STATS_SLOTS; slot++) {
      s->key[side][slot] = FIFA96_MATCH_ENTITY_NONE;
      s->count[side][slot] = 0;
      s->value[side][slot] = 0;
    }
  s->max_value = 0;
  s->max_key = FIFA96_MATCH_ENTITY_NONE;
  s->max_block = 0;
}

int fifa96_match_stats_add(struct fifa96_match_stats *s, uint32_t block,
                           int32_t key, int32_t delta) {
  if (!s || block > 1u) return 0;
  for (uint32_t slot = 0; slot < FIFA96_MATCH_STATS_SLOTS; slot++) {
    if (s->key[block][slot] != key) continue;
    s->value[block][slot] += fifa96_stats_weight(delta, 0, 0);
    {
      /* The native max compares the sign-extended low word
       * (`0x8EDD5`/`0x8EDD8`). */
      int16_t v = (int16_t)s->value[block][slot];
      if (v > (int16_t)s->max_value) {
        s->max_value = v;
        s->max_key = key;
        s->max_block = (uint16_t)block;
      }
    }
    return 1;
  }
  return 0;
}

void fifa96_match_run_stats_seed(struct fifa96_match_run *mr) {
  if (!mr) return;
  fifa96_match_stats_reset(&mr->stats);
  /* The native re-key has no counter preservation: the `+4` words reset with
   * the table, so the `[0x5B358]` third-selection flag clears too. */
  mr->stats_third_selection = 0;
  /* The seed blocks are the team indices. `fifa96_match_run_stats_add` keys
   * the same blocks as `(team+0x826 ^ [0x57ABE]) & 1`; `entities_init` sets
   * `team+0x826 = team index` and `side_swap` is staged 0 (OL-72), so the two
   * agree today. When a `[0x57ABE]` producer lands, seed block b must use the
   * roster of the team whose `side` is `b ^ side_swap`. */
  for (uint32_t side = 0; side < 2u; side++)
    for (uint32_t slot = 0; slot < FIFA96_MATCH_STATS_SLOTS; slot++)
      mr->stats.key[side][slot] = match_run_replay_roster_id(mr, side, slot);
}

int fifa96_match_run_stats_add(struct fifa96_match_run *mr, int32_t entity_id,
                               int32_t delta) {
  uint32_t side, block, pitch, slot;
  if (!mr || entity_id < 0 ||
      entity_id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  side = (uint32_t)entity_id / FIFA96_MATCH_ENTITY_RECORDS;
  pitch = (uint32_t)entity_id % FIFA96_MATCH_ENTITY_RECORDS;
  /* Native block = `(team+0x826 ^ [0x57ABE]) & 1` (`0x8ED83..0x8ED95`). The
   * modeled `side_swap` is staged 0 (`[0x57ABE]` producer unported, OL-72), so
   * the display block equals the team index the table was seeded by. */
  block = ((uint32_t)mr->entities.team[side].side ^ mr->side_swap) & 1u;
  slot = pitch;
  if (mr->roster_order_ok[side] != 0) {
    for (uint32_t i = 0; i < FIFA96_MATCH_STATS_SLOTS; i++)
      if (mr->stats.key[block][i] ==
          match_run_replay_roster_id(mr, side, pitch)) { slot = i; break; }
  }
  if (fifa96_match_stats_add(&mr->stats, block, mr->stats.key[block][slot],
                             delta) == 0)
    return 0;
  /* The replay selector indexes `activity_count` by the pitch/order slot, so
   * the table slot (seeded key at slot i = `match_run_replay_roster_id(i)`)
   * is also the pitch slot. */
  if (slot < FIFA96_CAMERA_REPLAY_SLOTS)
    mr->render.replay.activity_count[block][slot] = mr->stats.value[block][slot];
  return 1;
}

int fifa96_match_run_stats_drain(struct fifa96_match_run *mr,
                                 const fifa96_camera_stat_events *events,
                                 int32_t local_ebp_id) {
  uint32_t i;
  int drained = 0;
  if (!mr || !events) return 0;
  for (i = 0; i < events->count && i < FIFA96_CAMERA_STAT_MAX; i++) {
    int32_t entity_id;
    switch (events->event[i].entity) {
      case FIFA96_CAMERA_STAT_TRACKED:
        entity_id = mr->render.camera.tracked;   /* [0x1577CA] */
        break;
      case FIFA96_CAMERA_STAT_LOCAL_EBP:
        entity_id = local_ebp_id;                /* the handler's own record */
        break;
      default:
        /* OPEN kinds (BALL_SUB7A6, REC_15B43C, REC_157A9B, REC_1575DA) and
         * any out-of-range entity byte are skipped — never entity 0. */
        continue;
    }
    if (fifa96_match_run_stats_add(mr, entity_id, events->event[i].delta))
      drained++;
  }
  return drained;
}

int fifa96_match_run_stats_bump(struct fifa96_match_run *mr, int32_t entity_id) {
  uint32_t side, block, pitch;
  if (!mr || entity_id < 0 ||
      entity_id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  side = (uint32_t)entity_id / FIFA96_MATCH_ENTITY_RECORDS;
  pitch = (uint32_t)entity_id % FIFA96_MATCH_ENTITY_RECORDS;
  /* Native block = `(team+0x826 ^ [0x57ABE]) & 1`, the same resolution as
   * `fifa96_match_run_stats_add`; `FUN_0008ECC4` scans that block only
   * (`off = n*0xA + block*0xA0`), so a key that exists solely in the other
   * block is a miss. */
  block = ((uint32_t)mr->entities.team[side].side ^ mr->side_swap) & 1u;
  {
    int32_t key = match_run_replay_roster_id(mr, side, pitch);
    for (uint32_t slot = 0; slot < FIFA96_MATCH_STATS_SLOTS; slot++) {
      if (mr->stats.key[block][slot] != key) continue;
      mr->stats.count[block][slot]++;   /* native word wraps at 65536 */
      mr->stats_third_selection =
          (uint8_t)(mr->stats.count[block][slot] == 3u);
      return 1;
    }
  }
  return 0;
}

uint8_t fifa96_match_side_swap_value(uint8_t native_157ac2,
                                     uint8_t native_157ac3) {
  /* `FUN_0007417C`: `[0x157ABE] = ([0x157AC2] ^ byte[0x157AC3]) & 1`, with
   * `[0x157ABF]` the complement; the unnamed `0x742B4` writer repeats the
   * same formula. The reset sites below pass `(global_157ac2, 0)`, and the
   * match reset `FUN_00073EE0` stores `byte[0x157AC3] = BL` (0) at `0x73F5E`.
   * The non-zero flip is deliberately not wired: the native `[0x157AC2]`
   * phase/mode byte has many writers and the stats-key/block mapping across
   * a flip is unproved (B2-L1, FU72). */
  return (uint8_t)((native_157ac2 ^ native_157ac3) & 1u);
}

/* FU-70 §1.2 slot tables (flat 0x11064E direction map and the animation chain
 * T1 0x10E1DC -> T2 0x10E1EC / T3 0x10E1F5). */
static const uint8_t match_run_slot_map[16] = {
  0, 5, 10, 0, 6, 4, 2, 0, 9, 1, 8, 0, 0, 0, 0, 0
};
static const uint8_t match_run_anim_a[16] = {
  0, 1, 5, 0, 3, 2, 4, 0, 7, 8, 6, 0, 0, 0, 0, 0
};
static const uint8_t match_run_anim_b[16] = {
  0, 1, 1, 0, 0xFF, 0xFF, 0xFF, 0, 1, 0, 0, 0xFF, 0xFF, 0xFF, 0, 1
};
static const uint8_t match_run_anim_c[16] = {
  0, 0, 0xFF, 0xFF, 0xFF, 0, 1, 1, 1, 0, 0, 0x40, 0, 0, 0, 0
};

/* FU-84 §3.1: the animation row table is static executable data at flat
 * 0x10EF00 (111 rows x 9 bytes; installed into [0x57588] by 0x73D91). Row
 * layout {id, last_frame, flags, next_id, frame_table, sprite_bank}; the
 * frame_table dword is the relocated flat pointer (0x10E2xx..). First-hand
 * /FIFA96.EXE read_memory 0x10EF00: row 0 `00 00 07 00 00 E2 10 00 00`,
 * row 1 `01 0B 07 01 06 E2 10 00 01`, row 0x6E `6E 01 07 00 66 EE 10 00 53`.
 * The render chain consumes only +1 (last frame, the advance wrap) and +8
 * (the FU-85 resolver bank index); the frame tables are the staged FU-84
 * records (Task 2 open leg keeps the executable frame data unported). */
static const uint8_t match_run_anim_rows[FIFA96_ANIM_ROW_COUNT * FIFA96_ANIM_ROW_SIZE] = {
    /* row 0x00 */ 0x00, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x01 */ 0x01, 0x0B, 0x07, 0x01, 0x06, 0xE2, 0x10, 0x00, 0x01,
    /* row 0x02 */ 0x02, 0x0B, 0x07, 0x02, 0x42, 0xE2, 0x10, 0x00, 0x02,
    /* row 0x03 */ 0x03, 0x0B, 0x07, 0x03, 0x7E, 0xE2, 0x10, 0x00, 0x03,
    /* row 0x04 */ 0x04, 0x05, 0x00, 0x00, 0xBA, 0xE2, 0x10, 0x00, 0x04,
    /* row 0x05 */ 0x05, 0x05, 0x02, 0x00, 0xD8, 0xE2, 0x10, 0x00, 0x05,
    /* row 0x06 */ 0x06, 0x03, 0x00, 0x00, 0xF6, 0xE2, 0x10, 0x00, 0x06,
    /* row 0x07 */ 0x07, 0x04, 0x00, 0x00, 0x0A, 0xE3, 0x10, 0x00, 0x06,
    /* row 0x08 */ 0x08, 0x06, 0x00, 0x00, 0x3E, 0xE3, 0x10, 0x00, 0x07,
    /* row 0x09 */ 0x09, 0x04, 0x00, 0x00, 0x62, 0xE3, 0x10, 0x00, 0x08,
    /* row 0x0A */ 0x0A, 0x04, 0x00, 0x00, 0x7C, 0xE3, 0x10, 0x00, 0x09,
    /* row 0x0B */ 0x0B, 0x03, 0x00, 0x00, 0x96, 0xE3, 0x10, 0x00, 0x0A,
    /* row 0x0C */ 0x0C, 0x05, 0x00, 0x00, 0xAA, 0xE3, 0x10, 0x00, 0x0B,
    /* row 0x0D */ 0x0D, 0x05, 0x00, 0x0E, 0xC8, 0xE3, 0x10, 0x00, 0x0C,
    /* row 0x0E */ 0x0E, 0x08, 0x00, 0x0E, 0x04, 0xE4, 0x10, 0x00, 0x0D,
    /* row 0x0F */ 0x0F, 0x04, 0x00, 0x00, 0x32, 0xE4, 0x10, 0x00, 0x0E,
    /* row 0x10 */ 0x10, 0x06, 0x00, 0x00, 0x4C, 0xE4, 0x10, 0x00, 0x0F,
    /* row 0x11 */ 0x11, 0x04, 0x00, 0x00, 0x70, 0xE4, 0x10, 0x00, 0x10,
    /* row 0x12 */ 0x12, 0x05, 0x00, 0x0F, 0x8A, 0xE4, 0x10, 0x00, 0x11,
    /* row 0x13 */ 0x13, 0x03, 0x02, 0x00, 0xA8, 0xE4, 0x10, 0x00, 0x12,
    /* row 0x14 */ 0x14, 0x06, 0x00, 0x7F, 0xBC, 0xE4, 0x10, 0x00, 0x13,
    /* row 0x15 */ 0x15, 0x05, 0x06, 0x15, 0xE0, 0xE4, 0x10, 0x00, 0x14,
    /* row 0x16 */ 0x16, 0x04, 0x00, 0x00, 0xFE, 0xE4, 0x10, 0x00, 0x15,
    /* row 0x17 */ 0x17, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x18 */ 0x18, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x19 */ 0x19, 0x06, 0x00, 0x02, 0x54, 0xE5, 0x10, 0x00, 0x16,
    /* row 0x1A */ 0x1A, 0x08, 0x00, 0x00, 0x92, 0xE5, 0x10, 0x00, 0x17,
    /* row 0x1B */ 0x1B, 0x04, 0x00, 0x00, 0x78, 0xE5, 0x10, 0x00, 0x18,
    /* row 0x1C */ 0x1C, 0x03, 0x04, 0x1C, 0xC0, 0xE5, 0x10, 0x00, 0x1A,
    /* row 0x1D */ 0x1D, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x1E */ 0x1E, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x1F */ 0x1F, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x20 */ 0x20, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x21 */ 0x21, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x22 */ 0x22, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x23 */ 0x23, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x24 */ 0x24, 0x00, 0x04, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x25 */ 0x25, 0x04, 0x04, 0x25, 0xD4, 0xE5, 0x10, 0x00, 0x1B,
    /* row 0x26 */ 0x26, 0x00, 0x07, 0x26, 0xEE, 0xE5, 0x10, 0x00, 0x1C,
    /* row 0x27 */ 0x27, 0x00, 0x07, 0x27, 0xF4, 0xE5, 0x10, 0x00, 0x1D,
    /* row 0x28 */ 0x28, 0x0D, 0x07, 0x28, 0xFA, 0xE5, 0x10, 0x00, 0x4C,
    /* row 0x29 */ 0x29, 0x0B, 0x07, 0x29, 0x06, 0xE2, 0x10, 0x00, 0x01,
    /* row 0x2A */ 0x2A, 0x0B, 0x07, 0x2A, 0x7E, 0xE2, 0x10, 0x00, 0x03,
    /* row 0x2B */ 0x2B, 0x02, 0x07, 0x26, 0x40, 0xE6, 0x10, 0x00, 0x1E,
    /* row 0x2C */ 0x2C, 0x03, 0x00, 0x27, 0x50, 0xE6, 0x10, 0x00, 0x1F,
    /* row 0x2D */ 0x2D, 0x03, 0x00, 0x27, 0x64, 0xE6, 0x10, 0x00, 0x20,
    /* row 0x2E */ 0x2E, 0x05, 0x00, 0x27, 0x78, 0xE6, 0x10, 0x00, 0x21,
    /* row 0x2F */ 0x2F, 0x08, 0x00, 0x26, 0x96, 0xE6, 0x10, 0x00, 0x22,
    /* row 0x30 */ 0x30, 0x03, 0x00, 0x26, 0xF6, 0xE2, 0x10, 0x00, 0x06,
    /* row 0x31 */ 0x31, 0x04, 0x00, 0x26, 0x24, 0xE3, 0x10, 0x00, 0x06,
    /* row 0x32 */ 0x32, 0x0A, 0x02, 0x26, 0x8C, 0xE9, 0x10, 0x00, 0x24,
    /* row 0x33 */ 0x33, 0x08, 0x02, 0x26, 0x92, 0xE5, 0x10, 0x00, 0x17,
    /* row 0x34 */ 0x34, 0x07, 0x20, 0x3F, 0xC4, 0xE6, 0x10, 0x00, 0x25,
    /* row 0x35 */ 0x35, 0x07, 0x20, 0x40, 0xEC, 0xE6, 0x10, 0x00, 0x25,
    /* row 0x36 */ 0x36, 0x07, 0x10, 0x41, 0x14, 0xE7, 0x10, 0x00, 0x25,
    /* row 0x37 */ 0x37, 0x07, 0x10, 0x42, 0x3C, 0xE7, 0x10, 0x00, 0x25,
    /* row 0x38 */ 0x38, 0x07, 0x20, 0x3F, 0x64, 0xE7, 0x10, 0x00, 0x28,
    /* row 0x39 */ 0x39, 0x07, 0x20, 0x40, 0x8C, 0xE7, 0x10, 0x00, 0x28,
    /* row 0x3A */ 0x3A, 0x07, 0x10, 0x41, 0xB4, 0xE7, 0x10, 0x00, 0x28,
    /* row 0x3B */ 0x3B, 0x07, 0x10, 0x42, 0xDC, 0xE7, 0x10, 0x00, 0x28,
    /* row 0x3C */ 0x3C, 0x05, 0x00, 0x0F, 0x04, 0xE8, 0x10, 0x00, 0x2B,
    /* row 0x3D */ 0x3D, 0x07, 0x20, 0x3F, 0x22, 0xE8, 0x10, 0x00, 0x2C,
    /* row 0x3E */ 0x3E, 0x07, 0x10, 0x41, 0x4A, 0xE8, 0x10, 0x00, 0x2C,
    /* row 0x3F */ 0x3F, 0x04, 0x20, 0x26, 0x8C, 0xE8, 0x10, 0x00, 0x2E,
    /* row 0x40 */ 0x40, 0x04, 0x20, 0x27, 0xC0, 0xE8, 0x10, 0x00, 0x2E,
    /* row 0x41 */ 0x41, 0x04, 0x10, 0x26, 0x72, 0xE8, 0x10, 0x00, 0x2E,
    /* row 0x42 */ 0x42, 0x04, 0x10, 0x27, 0xA6, 0xE8, 0x10, 0x00, 0x2E,
    /* row 0x43 */ 0x43, 0x05, 0x07, 0x43, 0xDA, 0xE8, 0x10, 0x00, 0x30,
    /* row 0x44 */ 0x44, 0x05, 0x02, 0x26, 0xF8, 0xE8, 0x10, 0x00, 0x31,
    /* row 0x45 */ 0x45, 0x06, 0x00, 0x26, 0x16, 0xE9, 0x10, 0x00, 0x32,
    /* row 0x46 */ 0x46, 0x0C, 0x00, 0x41, 0x3A, 0xE9, 0x10, 0x00, 0x33,
    /* row 0x47 */ 0x47, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x48 */ 0x48, 0x03, 0x00, 0x00, 0xE8, 0xE9, 0x10, 0x00, 0x34,
    /* row 0x49 */ 0x49, 0x02, 0x00, 0x00, 0xFC, 0xE9, 0x10, 0x00, 0x35,
    /* row 0x4A */ 0x4A, 0x0A, 0x00, 0x02, 0x0C, 0xEA, 0x10, 0x00, 0x36,
    /* row 0x4B */ 0x4B, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x4C */ 0x4C, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x4D */ 0x4D, 0x08, 0x00, 0x02, 0x44, 0xEA, 0x10, 0x00, 0x38,
    /* row 0x4E */ 0x4E, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x4F */ 0x4F, 0x08, 0x00, 0x1C, 0x72, 0xEA, 0x10, 0x00, 0x39,
    /* row 0x50 */ 0x50, 0x07, 0x00, 0x0F, 0xA0, 0xEA, 0x10, 0x00, 0x3C,
    /* row 0x51 */ 0x51, 0x0B, 0x00, 0x0F, 0xFE, 0xEB, 0x10, 0x00, 0x3D,
    /* row 0x52 */ 0x52, 0x05, 0x00, 0x7E, 0x3A, 0xEC, 0x10, 0x00, 0x3F,
    /* row 0x53 */ 0x53, 0x33, 0x00, 0x41, 0xC8, 0xEA, 0x10, 0x00, 0x3B,
    /* row 0x54 */ 0x54, 0x09, 0x00, 0x0F, 0xCC, 0xEB, 0x10, 0x00, 0x3D,
    /* row 0x55 */ 0x55, 0x07, 0x00, 0x00, 0x2E, 0xEE, 0x10, 0x00, 0x54,
    /* row 0x56 */ 0x56, 0x19, 0x00, 0x0F, 0x58, 0xEC, 0x10, 0x00, 0x40,
    /* row 0x57 */ 0x57, 0x05, 0x00, 0x00, 0xBA, 0xE2, 0x10, 0x00, 0x04,
    /* row 0x58 */ 0x58, 0x09, 0x06, 0x00, 0x0C, 0xED, 0x10, 0x00, 0x42,
    /* row 0x59 */ 0x59, 0x06, 0x00, 0x00, 0x3E, 0xED, 0x10, 0x00, 0x43,
    /* row 0x5A */ 0x5A, 0x0B, 0x07, 0x5A, 0x06, 0xE2, 0x10, 0x00, 0x44,
    /* row 0x5B */ 0x5B, 0x08, 0x06, 0x00, 0x62, 0xED, 0x10, 0x00, 0x46,
    /* row 0x5C */ 0x5C, 0x08, 0x00, 0x27, 0xBE, 0xED, 0x10, 0x00, 0x47,
    /* row 0x5D */ 0x5D, 0x06, 0x00, 0x00, 0xEC, 0xED, 0x10, 0x00, 0x48,
    /* row 0x5E */ 0x5E, 0x05, 0x00, 0x0F, 0xE6, 0xE3, 0x10, 0x00, 0x0C,
    /* row 0x5F */ 0x5F, 0x08, 0x06, 0x00, 0x90, 0xED, 0x10, 0x00, 0x46,
    /* row 0x60 */ 0x60, 0x00, 0x07, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x49,
    /* row 0x61 */ 0x61, 0x05, 0x06, 0x61, 0xE0, 0xE4, 0x10, 0x00, 0x14,
    /* row 0x62 */ 0x62, 0x00, 0x07, 0x62, 0x00, 0xE2, 0x10, 0x00, 0x4F,
    /* row 0x63 */ 0x63, 0x00, 0x07, 0x63, 0x00, 0xE2, 0x10, 0x00, 0x50,
    /* row 0x64 */ 0x64, 0x00, 0x07, 0x64, 0x00, 0xE2, 0x10, 0x00, 0x51,
    /* row 0x65 */ 0x65, 0x00, 0x07, 0x65, 0x00, 0xE2, 0x10, 0x00, 0x52,
    /* row 0x66 */ 0x66, 0x00, 0x07, 0x66, 0x60, 0xEE, 0x10, 0x00, 0x53,
    /* row 0x67 */ 0x67, 0x05, 0x06, 0x67, 0x10, 0xEE, 0x10, 0x00, 0x4D,
    /* row 0x68 */ 0x68, 0x03, 0x00, 0x0F, 0x9E, 0xEE, 0x10, 0x00, 0x57,
    /* row 0x69 */ 0x69, 0x04, 0x00, 0x00, 0xB2, 0xEE, 0x10, 0x00, 0x58,
    /* row 0x6A */ 0x6A, 0x08, 0x00, 0x01, 0x70, 0xEE, 0x10, 0x00, 0x55,
    /* row 0x6B */ 0x6B, 0x09, 0x00, 0x01, 0xCC, 0xEE, 0x10, 0x00, 0x59,
    /* row 0x6C */ 0x6C, 0x00, 0x06, 0x00, 0x00, 0xE2, 0x10, 0x00, 0x00,
    /* row 0x6D */ 0x6D, 0x01, 0x07, 0x66, 0x56, 0xEE, 0x10, 0x00, 0x53,
    /* row 0x6E */ 0x6E, 0x01, 0x07, 0x00, 0x66, 0xEE, 0x10, 0x00, 0x53,
};

/* FU-153 / M2 phase-12 T1 (15.a): the executable FU-84 frame records
 * (`0x10E200..0x10EF00`, 0xD00 bytes; first-hand `read_memory 0x10E200` /
 * `0x10E400` reconciled byte-for-byte with the image file, this task). Row `i`
 * of `match_run_anim_rows` points at `frame_table` (relocated flat 0x10E2xx);
 * frame `j` sits at `frame_table - 0x10E200 + j*5` as {duration u16, aux u16,
 * sprite u8}. Only the `aux` word (+2) is consumed here: the frame resolver
 * FUN_0006E490 copies it to `rec+0x36` (`0x6E50A`), whose low byte is the
 * FUN_0006E8E8 switch index-1 (`(word[rec+0x36] & 0xFF) - 1`, `0x6E8F3..`).
 * The duration/sprite columns stay on the render's synthetic FU-84 leg
 * (`r->frames`, `match_run_render_load`) so the ported aux read does not move
 * the live tape; the real records are carried here because they are
 * executable-image data, not ISO assets. */
static const uint8_t match_run_frame_records[0xD00] = {
    0x40, 0x00, 0x00, 0x00, 0x00, 0x07, 0x50, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x01,
    0x50, 0x00, 0x00, 0x00, 0x02, 0x50, 0x00, 0x00, 0x00, 0x03, 0x50, 0x00, 0x00, 0x00, 0x04, 0x50,
    0x00, 0x00, 0x00, 0x05, 0x50, 0x00, 0x00, 0x00, 0x06, 0x50, 0x00, 0x00, 0x00, 0x07, 0x50, 0x00,
    0x00, 0x00, 0x08, 0x50, 0x00, 0x00, 0x00, 0x09, 0x50, 0x00, 0x00, 0x00, 0x0A, 0x50, 0x00, 0x00,
    0x00, 0x0B, 0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00,
    0x02, 0x40, 0x00, 0x00, 0x00, 0x03, 0x40, 0x00, 0x00, 0x00, 0x04, 0x40, 0x00, 0x00, 0x00, 0x05,
    0x40, 0x00, 0x00, 0x00, 0x06, 0x40, 0x00, 0x00, 0x00, 0x07, 0x40, 0x00, 0x00, 0x00, 0x08, 0x40,
    0x00, 0x00, 0x00, 0x09, 0x40, 0x00, 0x00, 0x00, 0x0A, 0x40, 0x00, 0x00, 0x00, 0x0B, 0x20, 0x00,
    0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x20, 0x00, 0x00, 0x00, 0x02, 0x20, 0x00, 0x00,
    0x00, 0x03, 0x20, 0x00, 0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00,
    0x06, 0x20, 0x00, 0x00, 0x00, 0x07, 0x20, 0x00, 0x00, 0x00, 0x08, 0x20, 0x00, 0x00, 0x00, 0x09,
    0x20, 0x00, 0x00, 0x00, 0x0A, 0x20, 0x00, 0x00, 0x00, 0x0B, 0x40, 0x00, 0x0A, 0x00, 0x00, 0x80,
    0x00, 0x07, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x03, 0x80, 0x00,
    0x02, 0x00, 0x04, 0xC0, 0x00, 0x00, 0x00, 0x05, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
    0x00, 0x01, 0x80, 0x00, 0x21, 0x00, 0x02, 0x80, 0x00, 0x05, 0x00, 0x03, 0x80, 0x00, 0x05, 0x00,
    0x04, 0xC0, 0x00, 0x05, 0x00, 0x05, 0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x1D, 0x00, 0x01,
    0x40, 0x00, 0x13, 0x00, 0x02, 0x40, 0x00, 0x1C, 0x00, 0x03, 0x20, 0x00, 0x00, 0x00, 0x00, 0x40,
    0x00, 0x1D, 0x00, 0x01, 0x40, 0x00, 0x13, 0x05, 0x02, 0x60, 0x00, 0x05, 0x00, 0x03, 0x40, 0x00,
    0x05, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x1D, 0x13, 0x01, 0x40, 0x00,
    0x1E, 0x00, 0x02, 0x60, 0x00, 0x10, 0x02, 0x03, 0x40, 0x00, 0x05, 0x00, 0x04, 0x00, 0x20, 0x00,
    0x05, 0x00, 0x00, 0x40, 0x00, 0x1D, 0x05, 0x01, 0x20, 0x00, 0x13, 0x00, 0x02, 0x60, 0x00, 0x05,
    0x00, 0x03, 0x40, 0x00, 0x05, 0x00, 0x04, 0x40, 0x00, 0x05, 0x00, 0x05, 0x60, 0x00, 0x05, 0x00,
    0x06, 0x20, 0xC0, 0x00, 0x13, 0x00, 0x02, 0x40, 0x00, 0x0D, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x00,
    0x04, 0x40, 0x00, 0x02, 0x00, 0x05, 0x80, 0x00, 0x05, 0x00, 0x06, 0x00, 0x40, 0x00, 0x05, 0x00,
    0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x05, 0x00, 0x02, 0xC0, 0x00, 0x05, 0x00, 0x03,
    0x80, 0x00, 0x05, 0x00, 0x04, 0x05, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01,
    0x80, 0x00, 0x10, 0x00, 0x02, 0x80, 0x00, 0x05, 0x00, 0x03, 0x40, 0x00, 0x29, 0x00, 0x00, 0x80,
    0x00, 0x29, 0x00, 0x01, 0x40, 0x00, 0x29, 0x00, 0x02, 0xC0, 0x00, 0x29, 0x00, 0x03, 0x40, 0x00,
    0x00, 0x00, 0x04, 0x40, 0x00, 0x00, 0x00, 0x05, 0x80, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x01, 0x80, 0x00, 0x02, 0x03, 0x02, 0x80, 0x00, 0x0C, 0x00, 0x03, 0x80, 0x00, 0x0C, 0x00,
    0x04, 0x40, 0x01, 0x05, 0x00, 0x05, 0x40, 0x00, 0x0E, 0x00, 0x01, 0x80, 0x00, 0x2A, 0x00, 0x01,
    0x40, 0x00, 0x2B, 0x03, 0x02, 0x80, 0x00, 0x0C, 0x00, 0x03, 0x80, 0x00, 0x0C, 0x00, 0x04, 0x40,
    0x01, 0x05, 0x00, 0x05, 0x00, 0x01, 0x17, 0x00, 0x00, 0x00, 0x01, 0x17, 0x00, 0x01, 0x80, 0x00,
    0x17, 0x00, 0x05, 0x80, 0x00, 0x17, 0x00, 0x02, 0x80, 0x00, 0x17, 0x00, 0x04, 0x00, 0x01, 0x17,
    0x00, 0x03, 0x80, 0x00, 0x17, 0x00, 0x04, 0x80, 0x00, 0x17, 0x00, 0x02, 0x80, 0x00, 0x17, 0x00,
    0x05, 0x00, 0xC0, 0x00, 0x05, 0x18, 0x00, 0x80, 0x00, 0x05, 0x18, 0x01, 0x80, 0x00, 0x05, 0x18,
    0x02, 0x80, 0x00, 0x05, 0x18, 0x03, 0x80, 0x00, 0x05, 0x18, 0x04, 0x00, 0x40, 0x00, 0x0A, 0x00,
    0x00, 0x80, 0x00, 0x11, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x02, 0xC0, 0x00, 0x02, 0x00, 0x03,
    0x40, 0x00, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05, 0x00, 0x05, 0x40, 0x00, 0x05, 0x00, 0x06, 0x04,
    0xC0, 0x00, 0x29, 0x2A, 0x00, 0x80, 0x00, 0x2A, 0x00, 0x01, 0x00, 0x01, 0x2A, 0x00, 0x02, 0x00,
    0x01, 0x04, 0x00, 0x03, 0x80, 0x00, 0x18, 0x05, 0x04, 0x80, 0x40, 0x00, 0x0A, 0x00, 0x00, 0x40,
    0x00, 0x0A, 0x10, 0x01, 0x80, 0x00, 0x0E, 0x00, 0x01, 0x40, 0x00, 0x02, 0x00, 0x02, 0xC0, 0x00,
    0x03, 0x0C, 0x03, 0x40, 0x01, 0x12, 0x00, 0x04, 0x80, 0x00, 0x05, 0x00, 0x00, 0x40, 0x01, 0x08,
    0x00, 0x01, 0x80, 0x00, 0x05, 0x00, 0x02, 0x40, 0x00, 0x05, 0x00, 0x03, 0x40, 0x00, 0x0F, 0x00,
    0x00, 0x80, 0x00, 0x0A, 0x00, 0x01, 0x40, 0x00, 0x0A, 0x00, 0x02, 0x80, 0x00, 0x13, 0x0B, 0x03,
    0x80, 0x00, 0x02, 0x00, 0x04, 0xC0, 0x00, 0x05, 0x00, 0x05, 0x00, 0x01, 0x05, 0x12, 0x06, 0x00,
    0x60, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x01, 0x60, 0x00, 0x00, 0x00, 0x02, 0x60,
    0x00, 0x00, 0x00, 0x03, 0x60, 0x00, 0x00, 0x00, 0x04, 0x60, 0x00, 0x00, 0x00, 0x05, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x40, 0x01, 0x04, 0x00, 0x01, 0xC0, 0x00, 0x05, 0x00, 0x02, 0x80, 0x00, 0x05,
    0x00, 0x03, 0x40, 0x01, 0x05, 0x00, 0x04, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00,
    0x00, 0x03, 0x40, 0x00, 0x00, 0x00, 0x04, 0x40, 0x00, 0x00, 0x00, 0x05, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01,
    0x40, 0x00, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0x00, 0x00, 0x04, 0x80,
    0x00, 0x00, 0x00, 0x05, 0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00,
    0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x04, 0x80, 0x00, 0x00,
    0x00, 0x05, 0xC0, 0x00, 0x00, 0x00, 0x06, 0x80, 0x80, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x05,
    0x00, 0x01, 0x00, 0x01, 0x13, 0x00, 0x02, 0x80, 0x00, 0x05, 0x00, 0x03, 0x40, 0x00, 0x05, 0x00,
    0x04, 0x60, 0x40, 0x00, 0x29, 0x00, 0x00, 0x40, 0x00, 0x29, 0x00, 0x01, 0xC0, 0x00, 0x29, 0x00,
    0x02, 0x40, 0x00, 0x29, 0x00, 0x03, 0x40, 0x01, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05, 0x00, 0x05,
    0x40, 0x00, 0x05, 0x00, 0x06, 0xC0, 0x00, 0x05, 0x00, 0x07, 0x40, 0x00, 0x05, 0x00, 0x08, 0x01,
    0x00, 0x01, 0x1B, 0x00, 0x00, 0x80, 0x00, 0x1B, 0x00, 0x01, 0xC0, 0x00, 0x1B, 0x00, 0x02, 0xC0,
    0x00, 0x1B, 0x00, 0x03, 0x80, 0x00, 0x1A, 0x00, 0x00, 0xC0, 0x00, 0x1A, 0x00, 0x01, 0x80, 0x02,
    0x1A, 0x00, 0x02, 0xC0, 0x00, 0x1A, 0x00, 0x01, 0x80, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x20,
    0x00, 0x00, 0x00, 0x01, 0x20, 0x00, 0x00, 0x00, 0x02, 0x20, 0x00, 0x00, 0x00, 0x03, 0x20, 0x00,
    0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x06, 0x20, 0x00, 0x00,
    0x00, 0x07, 0x20, 0x00, 0x00, 0x00, 0x06, 0x20, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x20, 0x00, 0x00, 0x00, 0x03, 0x20, 0x00, 0x00, 0x00, 0x02, 0x10, 0x00, 0x38, 0x00, 0x01,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x40, 0x00, 0x32, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x02, 0x40,
    0x00, 0x00, 0x00, 0x03, 0x40, 0x00, 0x32, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x40, 0x00,
    0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x00, 0x03, 0x40, 0x00, 0x0A, 0x00, 0x00, 0x40, 0x00, 0x00,
    0x00, 0x01, 0x40, 0x00, 0x32, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x03, 0x40, 0x00, 0x02, 0x00,
    0x04, 0x40, 0x00, 0x00, 0x00, 0x05, 0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01,
    0x40, 0x00, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x00, 0x03, 0x40, 0x00, 0x00, 0x00, 0x04, 0x40,
    0x00, 0x00, 0x00, 0x05, 0x40, 0x00, 0x00, 0x00, 0x06, 0x40, 0x00, 0x00, 0x00, 0x07, 0x40, 0x00,
    0x00, 0x00, 0x08, 0x00, 0x40, 0x00, 0x0A, 0x00, 0x00, 0x40, 0x00, 0x0A, 0x00, 0x01, 0x40, 0x00,
    0x0A, 0x00, 0x02, 0x80, 0x00, 0x09, 0x00, 0x03, 0x80, 0x00, 0x02, 0x03, 0x04, 0x80, 0x00, 0x31,
    0x00, 0x05, 0x80, 0x00, 0x31, 0x00, 0x06, 0x40, 0x01, 0x15, 0x00, 0x07, 0x40, 0x00, 0x0A, 0x00,
    0x00, 0x40, 0x00, 0x0A, 0x00, 0x01, 0x40, 0x00, 0x0A, 0x00, 0x02, 0x80, 0x00, 0x09, 0x00, 0x03,
    0x80, 0x00, 0x02, 0x03, 0x04, 0x80, 0x00, 0x31, 0x00, 0x05, 0x80, 0x00, 0x31, 0x00, 0x06, 0x40,
    0x01, 0x06, 0x00, 0x07, 0x40, 0x00, 0x0A, 0x00, 0x00, 0x40, 0x00, 0x0A, 0x00, 0x01, 0x40, 0x00,
    0x0A, 0x00, 0x02, 0x80, 0x00, 0x09, 0x00, 0x03, 0x80, 0x00, 0x02, 0x03, 0x04, 0x80, 0x00, 0x31,
    0x00, 0x05, 0x80, 0x00, 0x31, 0x00, 0x06, 0x40, 0x01, 0x06, 0x00, 0x07, 0x40, 0x00, 0x0A, 0x00,
    0x00, 0x40, 0x00, 0x0A, 0x00, 0x01, 0x40, 0x00, 0x0A, 0x00, 0x02, 0x80, 0x00, 0x09, 0x00, 0x03,
    0x80, 0x00, 0x02, 0x03, 0x04, 0x80, 0x00, 0x31, 0x00, 0x05, 0x80, 0x00, 0x31, 0x00, 0x06, 0x40,
    0x01, 0x06, 0x00, 0x07, 0x40, 0x00, 0x0A, 0x00, 0x00, 0x40, 0x00, 0x0A, 0x00, 0x01, 0x40, 0x00,
    0x0A, 0x00, 0x02, 0x80, 0x00, 0x09, 0x00, 0x03, 0x80, 0x00, 0x02, 0x03, 0x04, 0x80, 0x00, 0x00,
    0x00, 0x05, 0x80, 0x00, 0x00, 0x00, 0x06, 0x40, 0x01, 0x15, 0x00, 0x07, 0x40, 0x00, 0x0A, 0x00,
    0x00, 0x40, 0x00, 0x0A, 0x00, 0x01, 0x40, 0x00, 0x0A, 0x00, 0x02, 0x80, 0x00, 0x09, 0x00, 0x03,
    0x80, 0x00, 0x02, 0x03, 0x04, 0x80, 0x00, 0x00, 0x00, 0x05, 0x80, 0x00, 0x00, 0x00, 0x06, 0x40,
    0x01, 0x06, 0x00, 0x07, 0x40, 0x00, 0x0A, 0x00, 0x00, 0x40, 0x00, 0x0A, 0x00, 0x01, 0x40, 0x00,
    0x0A, 0x00, 0x02, 0x80, 0x00, 0x09, 0x00, 0x03, 0x80, 0x00, 0x02, 0x03, 0x04, 0x80, 0x00, 0x00,
    0x00, 0x05, 0x80, 0x00, 0x00, 0x00, 0x06, 0x40, 0x01, 0x06, 0x00, 0x07, 0x40, 0x00, 0x0A, 0x00,
    0x00, 0x40, 0x00, 0x0A, 0x00, 0x01, 0x40, 0x00, 0x0A, 0x00, 0x02, 0x80, 0x00, 0x09, 0x00, 0x03,
    0x80, 0x00, 0x02, 0x03, 0x04, 0x80, 0x00, 0x00, 0x00, 0x05, 0x80, 0x00, 0x00, 0x00, 0x06, 0x40,
    0x01, 0x06, 0x00, 0x07, 0xC0, 0x00, 0x0A, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x06, 0x00,
    0x09, 0x00, 0x02, 0x80, 0x00, 0x02, 0x00, 0x03, 0x80, 0x00, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05,
    0x00, 0x05, 0x40, 0x00, 0x0A, 0x00, 0x00, 0x40, 0x00, 0x0A, 0x00, 0x01, 0x40, 0x00, 0x0A, 0x00,
    0x02, 0x80, 0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0x02, 0x00, 0x04, 0x80, 0x00, 0x00, 0x00, 0x05,
    0x80, 0x00, 0x00, 0x00, 0x06, 0x40, 0x01, 0x15, 0x00, 0x07, 0x40, 0x00, 0x0A, 0x00, 0x00, 0x40,
    0x00, 0x0A, 0x00, 0x01, 0x40, 0x00, 0x0A, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x03, 0x80, 0x00,
    0x02, 0x00, 0x04, 0x80, 0x00, 0x00, 0x00, 0x05, 0x80, 0x00, 0x00, 0x00, 0x06, 0x40, 0x01, 0x06,
    0x00, 0x07, 0x40, 0x00, 0x05, 0x00, 0x00, 0x40, 0x00, 0x05, 0x00, 0x01, 0x40, 0x00, 0x05, 0x00,
    0x02, 0x40, 0x00, 0x05, 0x00, 0x03, 0x40, 0x00, 0x05, 0x00, 0x04, 0x00, 0x40, 0x00, 0x05, 0x00,
    0x00, 0x40, 0x00, 0x05, 0x00, 0x01, 0x40, 0x00, 0x05, 0x00, 0x02, 0x40, 0x00, 0x05, 0x00, 0x03,
    0x40, 0x00, 0x2F, 0x00, 0x04, 0x05, 0x40, 0x00, 0x05, 0x00, 0x00, 0x40, 0x00, 0x05, 0x00, 0x01,
    0x40, 0x00, 0x05, 0x00, 0x02, 0x40, 0x00, 0x05, 0x00, 0x03, 0x40, 0x00, 0x05, 0x00, 0x04, 0x80,
    0x40, 0x00, 0x05, 0x00, 0x00, 0x40, 0x00, 0x05, 0x00, 0x01, 0x40, 0x00, 0x05, 0x00, 0x02, 0x40,
    0x00, 0x05, 0x00, 0x03, 0x40, 0x00, 0x05, 0x00, 0x04, 0x00, 0x50, 0x00, 0x19, 0x00, 0x00, 0x50,
    0x00, 0x19, 0x00, 0x01, 0x50, 0x00, 0x19, 0x00, 0x02, 0x50, 0x00, 0x19, 0x00, 0x03, 0x50, 0x00,
    0x19, 0x00, 0x04, 0x50, 0x00, 0x19, 0x00, 0x05, 0x80, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00,
    0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x00, 0x03, 0x40, 0x00, 0x00, 0x00,
    0x04, 0xC0, 0x00, 0x00, 0x00, 0x05, 0x80, 0x00, 0x2A, 0x00, 0x00, 0x80, 0x00, 0x10, 0x2A, 0x01,
    0x80, 0x00, 0x2A, 0x00, 0x02, 0xC0, 0x00, 0x11, 0x2A, 0x03, 0x80, 0x00, 0x2A, 0x00, 0x04, 0x80,
    0x00, 0x2B, 0x00, 0x05, 0x00, 0x01, 0x05, 0x00, 0x06, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0xC0,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x01,
    0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00,
    0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00,
    0x02, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x04, 0x02, 0xC0, 0x00, 0x05, 0x00,
    0x00, 0xC0, 0x00, 0x05, 0x00, 0x01, 0xC0, 0x00, 0x05, 0x00, 0x02, 0x00, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x03,
    0x80, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x05, 0x80, 0x00, 0x00, 0x00, 0x06, 0x80,
    0x00, 0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x00, 0x00, 0x05, 0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00,
    0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x04, 0x80, 0x00, 0x00,
    0x00, 0x05, 0xC0, 0x00, 0x00, 0x00, 0x06, 0xC0, 0x40, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x05,
    0x00, 0x01, 0x40, 0x00, 0x05, 0x00, 0x02, 0x00, 0x05, 0x05, 0x00, 0x03, 0x80, 0x00, 0x05, 0x00,
    0x00, 0x40, 0x00, 0x05, 0x00, 0x01, 0xC0, 0x01, 0x05, 0x00, 0x02, 0x01, 0x40, 0x00, 0x00, 0x00,
    0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x1F, 0x00, 0x02, 0x80, 0x00, 0x1F, 0x05, 0x03,
    0x40, 0x00, 0x00, 0x00, 0x04, 0xC0, 0x00, 0x1F, 0x00, 0x05, 0x80, 0x00, 0x1F, 0x22, 0x06, 0x80,
    0x00, 0x1F, 0x00, 0x07, 0x40, 0x00, 0x00, 0x00, 0x08, 0x40, 0x00, 0x21, 0x00, 0x09, 0x40, 0x00,
    0x00, 0x00, 0x0A, 0x00, 0x40, 0x00, 0x2A, 0x00, 0x00, 0x40, 0x00, 0x2A, 0x00, 0x01, 0x40, 0x00,
    0x2A, 0x00, 0x02, 0x80, 0x00, 0x2A, 0x00, 0x03, 0x40, 0x00, 0x1E, 0x13, 0x04, 0xC0, 0x00, 0x2A,
    0x00, 0x04, 0x40, 0x00, 0x2B, 0x00, 0x05, 0x40, 0x00, 0x2A, 0x00, 0x06, 0xC0, 0x00, 0x29, 0x00,
    0x07, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00,
    0x02, 0x40, 0x00, 0x00, 0x00, 0x03, 0x40, 0x00, 0x00, 0x00, 0x04, 0x40, 0x00, 0x00, 0x00, 0x05,
    0x40, 0x00, 0x00, 0x00, 0x06, 0x40, 0x00, 0x00, 0x00, 0x07, 0x40, 0x00, 0x00, 0x00, 0x08, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x01, 0x00, 0x00, 0x03, 0x80, 0x00, 0x29, 0x00, 0x04, 0x40, 0x01, 0x0C, 0x00, 0x05, 0x40, 0x00,
    0x0C, 0x00, 0x06, 0x00, 0x01, 0x05, 0x00, 0x06, 0x40, 0x00, 0x05, 0x00, 0x00, 0x40, 0x00, 0x05,
    0x00, 0x01, 0x80, 0x00, 0x05, 0x00, 0x02, 0x80, 0x00, 0x05, 0x00, 0x03, 0xC0, 0x00, 0x05, 0x00,
    0x04, 0x80, 0x00, 0x05, 0x00, 0x05, 0x80, 0x00, 0x05, 0x00, 0x06, 0xC0, 0x00, 0x05, 0x00, 0x07,
    0xC0, 0x00, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05, 0x00, 0x05, 0x80, 0x00, 0x05, 0x00, 0x06, 0xC0,
    0x00, 0x05, 0x00, 0x07, 0xC0, 0x00, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05, 0x00, 0x05, 0x80, 0x00,
    0x05, 0x00, 0x06, 0xC0, 0x00, 0x05, 0x00, 0x07, 0xC0, 0x00, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05,
    0x00, 0x05, 0x80, 0x00, 0x05, 0x00, 0x06, 0xC0, 0x00, 0x05, 0x00, 0x07, 0xC0, 0x00, 0x05, 0x00,
    0x04, 0x80, 0x00, 0x05, 0x00, 0x05, 0x80, 0x00, 0x05, 0x00, 0x06, 0xC0, 0x00, 0x05, 0x00, 0x07,
    0xC0, 0x00, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05, 0x00, 0x05, 0x80, 0x00, 0x05, 0x00, 0x06, 0xC0,
    0x00, 0x05, 0x00, 0x07, 0xC0, 0x00, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05, 0x00, 0x05, 0x80, 0x00,
    0x05, 0x00, 0x06, 0xC0, 0x00, 0x05, 0x00, 0x07, 0xC0, 0x00, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05,
    0x00, 0x05, 0x80, 0x00, 0x05, 0x00, 0x06, 0xC0, 0x00, 0x05, 0x00, 0x07, 0xC0, 0x00, 0x05, 0x00,
    0x04, 0x80, 0x00, 0x05, 0x00, 0x05, 0x80, 0x00, 0x05, 0x00, 0x06, 0xC0, 0x00, 0x05, 0x00, 0x07,
    0xC0, 0x00, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05, 0x00, 0x05, 0x80, 0x00, 0x05, 0x00, 0x06, 0xC0,
    0x00, 0x05, 0x00, 0x07, 0xC0, 0x00, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05, 0x00, 0x05, 0x80, 0x00,
    0x05, 0x00, 0x06, 0xC0, 0x00, 0x05, 0x00, 0x07, 0xC0, 0x00, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05,
    0x00, 0x05, 0x80, 0x00, 0x05, 0x00, 0x06, 0xC0, 0x00, 0x05, 0x00, 0x07, 0x00, 0x01, 0x24, 0x00,
    0x03, 0x80, 0x00, 0x24, 0x00, 0x04, 0x80, 0x00, 0x03, 0x24, 0x05, 0x40, 0x00, 0x24, 0x00, 0x06,
    0x80, 0x00, 0x24, 0x00, 0x07, 0x40, 0x01, 0x24, 0x00, 0x08, 0xC0, 0x00, 0x24, 0x00, 0x09, 0x80,
    0x00, 0x05, 0x00, 0x0A, 0x40, 0x00, 0x05, 0x00, 0x0B, 0xC0, 0x03, 0x05, 0x00, 0x0C, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x24, 0x00, 0x03, 0x40, 0x00, 0x24,
    0x02, 0x04, 0x40, 0x00, 0x03, 0x24, 0x05, 0x40, 0x00, 0x24, 0x00, 0x06, 0x80, 0x00, 0x27, 0x24,
    0x07, 0x40, 0x01, 0x24, 0x00, 0x08, 0xC0, 0x00, 0x24, 0x00, 0x09, 0x80, 0x00, 0x00, 0x00, 0x0A,
    0x40, 0x00, 0x00, 0x00, 0x0B, 0xC0, 0x03, 0x00, 0x00, 0x0C, 0x80, 0x00, 0x2D, 0x00, 0x00, 0x40,
    0x00, 0x02, 0x03, 0x01, 0x80, 0x00, 0x29, 0x0C, 0x02, 0x80, 0x00, 0x0C, 0x00, 0x03, 0x80, 0x00,
    0x0C, 0x00, 0x04, 0xC0, 0x00, 0x05, 0x00, 0x05, 0x80, 0x00, 0x05, 0x29, 0x00, 0x80, 0x00, 0x05,
    0x29, 0x01, 0x80, 0x00, 0x05, 0x29, 0x02, 0x80, 0x00, 0x05, 0x29, 0x03, 0x80, 0x00, 0x05, 0x29,
    0x04, 0x80, 0x00, 0x05, 0x29, 0x05, 0x80, 0x00, 0x05, 0x29, 0x06, 0x80, 0x00, 0x05, 0x29, 0x07,
    0x80, 0x00, 0x05, 0x29, 0x08, 0x80, 0x00, 0x05, 0x29, 0x09, 0x80, 0x00, 0x05, 0x29, 0x02, 0x80,
    0x00, 0x05, 0x29, 0x03, 0x80, 0x00, 0x05, 0x29, 0x04, 0x80, 0x00, 0x05, 0x29, 0x05, 0x80, 0x00,
    0x05, 0x29, 0x06, 0x80, 0x00, 0x05, 0x29, 0x07, 0x80, 0x00, 0x05, 0x29, 0x08, 0x80, 0x00, 0x05,
    0x29, 0x09, 0x80, 0x00, 0x05, 0x29, 0x02, 0x80, 0x00, 0x05, 0x29, 0x03, 0x80, 0x00, 0x05, 0x29,
    0x04, 0x80, 0x00, 0x05, 0x29, 0x05, 0x80, 0x00, 0x05, 0x29, 0x06, 0x80, 0x00, 0x05, 0x29, 0x07,
    0x80, 0x00, 0x05, 0x29, 0x08, 0x80, 0x00, 0x05, 0x29, 0x09, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x00,
    0x00, 0x00, 0x04, 0x80, 0x01, 0x00, 0x00, 0x05, 0x80, 0x00, 0x00, 0x00, 0x06, 0x80, 0x00, 0x00,
    0x00, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x09, 0x80, 0x00, 0x05, 0x00,
    0x00, 0x80, 0x00, 0x05, 0x00, 0x01, 0x80, 0x00, 0x05, 0x00, 0x02, 0x80, 0x00, 0x05, 0x00, 0x03,
    0x80, 0x00, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05, 0x00, 0x05, 0x80, 0x00, 0x05, 0x00, 0x04, 0x80,
    0x00, 0x05, 0x00, 0x03, 0x80, 0x00, 0x05, 0x00, 0x02, 0x10, 0x00, 0x05, 0x30, 0x02, 0x40, 0x00,
    0x2A, 0x00, 0x00, 0xC0, 0x00, 0x2A, 0x00, 0x01, 0x80, 0x00, 0x2A, 0x00, 0x02, 0x40, 0x00, 0x05,
    0x00, 0x03, 0x80, 0x00, 0x05, 0x00, 0x04, 0xC0, 0x00, 0x05, 0x00, 0x05, 0x80, 0x01, 0x05, 0x00,
    0x06, 0x05, 0x80, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x01, 0x80, 0x00, 0x05, 0x00,
    0x02, 0x80, 0x00, 0x05, 0x00, 0x03, 0x80, 0x00, 0x05, 0x00, 0x04, 0x80, 0x00, 0x05, 0x00, 0x05,
    0x80, 0x00, 0x05, 0x00, 0x06, 0x80, 0x00, 0x05, 0x00, 0x07, 0x10, 0x00, 0x05, 0x2C, 0x07, 0x00,
    0x80, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x01, 0x80, 0x00, 0x05, 0x00, 0x02, 0x80,
    0x00, 0x05, 0x00, 0x07, 0x80, 0x00, 0x05, 0x00, 0x06, 0x80, 0x00, 0x05, 0x00, 0x05, 0x80, 0x00,
    0x05, 0x00, 0x04, 0x80, 0x00, 0x05, 0x00, 0x03, 0x10, 0x00, 0x05, 0x2C, 0x03, 0x00, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x40, 0x00, 0x32, 0x00, 0x01, 0x00, 0x01, 0x3A, 0x24, 0x02, 0x80, 0x00, 0x24,
    0x00, 0x03, 0x80, 0x00, 0x03, 0x24, 0x04, 0x80, 0x00, 0x24, 0x00, 0x05, 0x00, 0x01, 0x24, 0x00,
    0x06, 0x00, 0x01, 0x05, 0x00, 0x07, 0x00, 0x01, 0x05, 0x00, 0x08, 0x40, 0x80, 0x00, 0x05, 0x00,
    0x00, 0x80, 0x00, 0x29, 0x05, 0x01, 0xC0, 0x00, 0x05, 0x00, 0x02, 0xC0, 0x00, 0x05, 0x00, 0x03,
    0x80, 0x02, 0x05, 0x00, 0x04, 0xC0, 0x00, 0x05, 0x00, 0x03, 0x00, 0x0F, 0x05, 0x00, 0x04, 0x80,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x02, 0x80,
    0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0x00, 0x00, 0x04, 0x80, 0x00, 0x00, 0x00, 0x05, 0xC0, 0x00,
    0x05, 0x00, 0x00, 0xC0, 0x00, 0x29, 0x05, 0x01, 0x00, 0x01, 0x29, 0x05, 0x02, 0xC0, 0x00, 0x29,
    0x05, 0x03, 0x80, 0x00, 0x29, 0x05, 0x04, 0x80, 0x00, 0x29, 0x05, 0x05, 0x80, 0x00, 0x05, 0x00,
    0x06, 0x00, 0x03, 0x05, 0x00, 0x07, 0x80, 0x00, 0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x01,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x02,
    0x80, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x01, 0x80, 0x00, 0x05, 0x00, 0x02, 0x80,
    0x00, 0x05, 0x00, 0x03, 0x80, 0x00, 0x36, 0x05, 0x04, 0xC0, 0x00, 0x05, 0x00, 0x05, 0xC0, 0x00,
    0x05, 0x00, 0x06, 0xC0, 0x00, 0x05, 0x00, 0x07, 0x10, 0x00, 0x37, 0x00, 0x07, 0x80, 0x80, 0x00,
    0x24, 0x00, 0x00, 0x80, 0x00, 0x24, 0x00, 0x01, 0x80, 0x00, 0x24, 0x00, 0x02, 0x00, 0x14, 0x05,
    0x00, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x00,
    0x02, 0xC0, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x04, 0x80, 0x80, 0x00, 0x19, 0x00,
    0x00, 0x80, 0x00, 0x19, 0x00, 0x01, 0x80, 0x00, 0x19, 0x00, 0x02, 0x80, 0x00, 0x19, 0x00, 0x03,
    0x80, 0x00, 0x19, 0x00, 0x04, 0x80, 0x00, 0x19, 0x00, 0x05, 0x80, 0x00, 0x19, 0x00, 0x06, 0x80,
    0x00, 0x19, 0x00, 0x07, 0x80, 0x00, 0x19, 0x00, 0x08, 0x80, 0x00, 0x19, 0x34, 0x09, 0x00, 0x03,
};


/* FU-89 §6/FUN_000589E0 (first-hand disassemble_function 0x589E0): the
 * near-depth threshold is `max((0x14<<16)/(2*obj[5]), 0x78)`, or the
 * `[0x9088]` fallback when `obj[1] < [0x908C]`; `read_memory 0x109088` gives
 * `[0x9088] = 0x78` and `[0x908C] = 0x140`. obj[1] is the camera y and obj[5]
 * the camera record +0x4C ratio dword, whose static loader default is
 * `0x1500` (first-hand `read_memory 0x107554` = `00 15 00 00`, and the
 * sibling record 0x1075C4 likewise; the writer `0x4D836 MOV [EDX+0x4C],EBX`
 * stores the full dword from the camera-type entry[5]. The FU-96/FU-97 "21"
 * is the **byte at +0x4D**, not the dword.) The camera-type setup
 * (FUN_0004D7E8, values 4608/3048/3464..) is unported (open leg). */
#define MATCH_RUN_NEAR_LIMIT 0x140
#define MATCH_RUN_VIEW_RATIO 0x1500
/* FU-154 §2.2/§5.1 (M2 phase-12 T2, L14.8.e): the fresh-match render view's
 * orientation. The native view object is camera 0 — image [0x107DC8] =
 * 0x107508 (first-hand `read_memory 0x107DC8`) — whose static matrix fields
 * are +0x58 yaw = 0xBF9C and +0x5C pitch = 200 (`read_memory 0x107560` =
 * 9c bf 00 00 c8 00 00 00). FUN_0004C904 copies +0x58/+0x5C into 0x14E4EC/F0
 * for the projection matrix (first-hand 0x4C918..0x4C928), and the live
 * pose feed's mode-0 default arm writes no pose, so these statics are the
 * reset/non-live seed; Phase 80 overrides the live yaw/pitch/ratio with the
 * side seed. Same object whose +0x4C = 0x1500 is MATCH_RUN_VIEW_RATIO. */
/* The view/camera constants live in fifa96_match_run.h so the renderer and
 * the live-eye test share the same named values. */

/* FU-61 §2.3 sampler mapping rows: row 0/1 is the identity pinned by
 * port/tests/test_input.c (`row_identity`). The view-dependent row index [0x7DEC]
 * (FUN_0004CAE4/FUN_0004CA70) belongs to the match camera (Task 15), so the
 * engine keeps the default identity row here. */
static const uint8_t match_run_input_row[FIFA96_INPUT_MAP_ROW] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

/* Engine keys -> FU-61 §1.2 keyboard-handler codes. Directions: the handler
 * ORs 0x1 (0x46934, flag 0x112BEC = scancode 0x48 keypad 8/up), 0x2 (0x46941,
 * 0x50 keypad 2/down), 0x8 (0x46945, 0x4B keypad 4/left) and 0x4 (0x46952,
 * 0x4D keypad 6/right); the diagonal flags at 0x4695F..0x4698F (keypad
 * 7/9/1/3) OR 0x9/0x5/0xA/0x6, cross-checking those axis assignments.
 * Buttons: KICK 0x10, PASS 0x20, LONG_BALL 0x40 (the FU-75 §1.4 `0x7CD60`
 * request word, M2 phase-11 T1; the 0x80 keeper arm has no engine key).
 * P8: the pad's South/East buttons arrive as CONFIRM/DECLINE (platform_sdl3.c)
 * and double as KICK/PASS in live play; the front-end keeps its own
 * CONFIRM/DECLINE mapping (fifa96_frontend_run.c). Those two codes are only
 * accepted while the match is ACTIVE and unpaused, so the pause menu's A
 * resume / B exit never also stages a match kick or pass. */
static int match_run_map_key(int32_t raw_code, uint8_t *code) {
  switch (raw_code) {
    case FIFA96_ENGINE_KEY_UP:       *code = 0x01u; return 1;
    case FIFA96_ENGINE_KEY_DOWN:     *code = 0x02u; return 1;
    case FIFA96_ENGINE_KEY_LEFT:     *code = 0x08u; return 1;
    case FIFA96_ENGINE_KEY_RIGHT:    *code = 0x04u; return 1;
    case FIFA96_ENGINE_KEY_KICK:     *code = 0x10u; return 1;
    case FIFA96_ENGINE_KEY_PASS:     *code = 0x20u; return 1;
    case FIFA96_ENGINE_KEY_LONG_BALL: *code = 0x40u; return 1;
    case FIFA96_ENGINE_KEY_CONFIRM:  *code = 0x10u; return 1;   /* pad A */
    case FIFA96_ENGINE_KEY_DECLINE:  *code = 0x20u; return 1;   /* pad B */
    default:                         return 0;
  }
}

/* P8: CONFIRM/DECLINE are menu keys first. They reach the match only while
 * play is ACTIVE and the pause panel is closed; keyboard KICK/PASS (Z/C) are
 * not gated here, preserving their existing match bindings. */
static int match_run_key_gated(const struct fifa96_match_run *mr, int32_t raw_code) {
  if (raw_code != FIFA96_ENGINE_KEY_CONFIRM && raw_code != FIFA96_ENGINE_KEY_DECLINE)
    return 0;
  return mr->paused || mr->lc.screen != FIFA96_MATCH_SCREEN_ACTIVE;
}

int fifa96_match_run_input(struct fifa96_match_run *mr, const fifa96_platform_key *keys,
                           size_t count) {
  uint8_t current[FIFA96_INPUT_PLAYERS] = { 0, 0, 0, 0 };
  uint8_t raw = 0;
  uint8_t mapped = 0;
  int switch_down = 0;
  int tackle_down = 0;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (fifa96_match_run_replay_family(&mr->render.replay)) {
    uint8_t held = 0;
    if (keys) {
      for (size_t i = 0; i < count; i++) {
        if (keys[i].state != 1) continue;
        switch (keys[i].raw_code) {
          case FIFA96_ENGINE_KEY_CONFIRM: held |= 1u; break;
          case FIFA96_ENGINE_KEY_DOWN:    held |= 2u; break;
          case FIFA96_ENGINE_KEY_LEFT:    held |= 4u; break;
          case FIFA96_ENGINE_KEY_RIGHT:   held |= 8u; break;
          case FIFA96_ENGINE_KEY_PASS:    held |= 0x20u; break;
          case FIFA96_ENGINE_KEY_QUIT:    held |= 0x80u; break;
          default: break;
        }
      }
    }
    if (!mr->render.replay.input_primed) {
      /* First poll after `fifa96_match_run_replay_begin`: prime the held
       * level only. A key already held across the arm (the host kickoff
       * auto-PASS is the reachable case) must not produce an edge. */
      mr->render.replay.input_buttons_held = held;
      mr->render.replay.input_buttons = 0;
      mr->render.replay.input_primed = 1;
      return 0;
    }
    mr->render.replay.input_buttons |=
        (uint8_t)(held & (uint8_t)~mr->render.replay.input_buttons_held);
    mr->render.replay.input_buttons_held = held;
    return 0;
  }
  mr->render.replay.input_buttons = 0;
  mr->render.replay.input_buttons_held = 0;
  if (keys) {
    for (size_t i = 0; i < count; i++) {
      uint8_t code = 0;
      if (keys[i].state != 1) continue;   /* press events only */
      if (keys[i].raw_code == FIFA96_ENGINE_KEY_SWITCH) {
        switch_down = 1;                  /* Phase 76: not an FU-61 word */
        continue;
      }
      if (keys[i].raw_code == FIFA96_ENGINE_KEY_TACKLE) {
        tackle_down = 1;                  /* Phase 77: not an FU-61 word */
        continue;
      }
      if (match_run_key_gated(mr, keys[i].raw_code)) continue;
      if (!match_run_map_key(keys[i].raw_code, &code)) continue;
      raw |= code;
    }
  }
  int switch_accept = mr->live_start && !mr->paused &&
                      mr->lc.screen == FIFA96_MATCH_SCREEN_ACTIVE &&
                      mr->state.phase == 2u && mr->live_kickoff_touch == 0u;
  if (mr->input_resync) {
    /* First non-replay poll after `fifa96_match_run_replay_exit`: adopt the
     * current physical levels and drop any queued edge. Skipping the edge
     * branches for this one poll prevents a key held through the replay from
     * firing on exit; a fresh press on the next poll still edges. */
    mr->live_switch_down = switch_down ? 1u : 0u;
    mr->live_tackle_down = tackle_down ? 1u : 0u;
    mr->live_switch_press = 0u;
    mr->live_tackle_press = 0u;
    mr->input_resync = 0;
  } else {
    if (switch_down && !mr->live_switch_down && switch_accept)
      mr->live_switch_press = 1u;
    else if (!switch_accept)
      mr->live_switch_press = 0u;
    mr->live_switch_down = switch_down ? 1u : 0u;
    if (tackle_down && !mr->live_tackle_down && switch_accept)
      mr->live_tackle_press = 1u;
    else if (!switch_accept)
      mr->live_tackle_press = 0u;
    mr->live_tackle_down = tackle_down ? 1u : 0u;
  }
  (void)fifa96_input_map(raw, match_run_input_row, &mapped);
  current[0] = mapped;
  (void)fifa96_input_update(&mr->input, current);
  memcpy(mr->input_state, current, sizeof current);   /* sampled slot input */
  return 0;
}

/* M2 phase-10 T2 (FU-148 §13.1): the FUN_0001C9BC range-word consumers derive
 * three bits from `(word[0x14C1D4] | word[0x14C1D6])`: bit 0 = the boundary
 * reflect input (`fifa96_match_goal_arm` -> `fifa96_camera_reflect`, FU-145
 * §3), bit 1 = the FUN_000736AC interpolation arm (0x73A07..0x73A18 `TEST
 * AL,2`, threaded as `render.input_bit2` into `fifa96_camera_update_walk`),
 * and bit 2 = the FUN_000709D0 pan-step random walk gate (0x70ab1..0x70ac2,
 * the frame's local `range & 4`). The native reads the words at each consumer
 * site; the engine derives the two render bits whenever the words are rebuilt
 * (match init FUN_00011B7C, and the config producer path below). */
static void match_run_input_bits_update(struct fifa96_match_run *mr) {
  uint16_t range = (uint16_t)(mr->input_range[0] | mr->input_range[1]);
  mr->render.input_bit0 = (range & 1u) ? 1 : 0;
  mr->render.input_bit2 = (range & 2u) ? 1 : 0;
}

/* M2 phase-10 T2 (FU-148 §12/§13): the `0x105278`-family config-cell producer
 * path. The front-end options block (13 dwords 0x105274..0x1052A4, edited by
 * FUN_0001C728 and queried by FUN_0001CAEC) feeds FUN_0001C9BC at match init.
 * This setter stages the eight consumed cells from the block and rebuilds the
 * range words + the two derived render bits, so a front-end/config caller can
 * set the walk gate / reflect / interpolation bits from real settings. */
int fifa96_match_run_set_input_options(
    struct fifa96_match_run *mr,
    const int32_t options[FIFA96_INPUT_OPTION_COUNT]) {
  if (!mr || !options) return -FIFA96_ERR_INVALID;
  if (fifa96_input_options_to_cells(options, mr->input_cfg) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  (void)fifa96_input_range_words(mr->input_cfg, 0, mr->input_range);
  match_run_input_bits_update(mr);
  return FIFA96_OK;
}

/* M2 interactive Task 1 / FU-77 §1: the shared per-record locomotion mover
 * `FUN_0007BF20` blocks A-E (`0x8E24B..0x8E507`, twin `FUN_0008E244`). The
 * native calls it from both record-machine tails after the action handler
 * (`0x7CD48`/`0x785C5`) for every record; FU-147 S1 removes the
 * controlled-record guard so the driver loop `0x8DB2E..0x8DB5F` runs it for
 * every dispatched record (the pool walk keeps the `+0x9A` skip). The fields
 * the native reads/writes persist on the pool record (`face7d` +0x7D,
 * `speed71` +0x71, `vel73`/`vel75` +0x73/+0x75, `body_timer9c` +0x9C,
 * `timer7b` +0x7B as the stride nibble); the remaining unported input is
 * staged as its derived default: `+0x6F` stride rate 0. The `+0x43`
 * direct-face latch and the `0x157A73` focus point are staged LIVE (FU319 §5,
 * phase-33): the latch is the record's own pool byte and the point is the
 * engine's camera triple. `move_attr` is the native `dword[rec+0x63]`,
 * whose high word is the block-A distance `+0x65` (the low word falls out of
 * the `>>19` attr extract); it is recomputed here from the post-action target
 * so the stride index sees the fresh distance, exactly as the native
 * recomputes `+0x65` before the table read (`0x8E267..0x8E278`). */
static void match_run_record_mover(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  fifa96_action_locomotion s;
  memset(&s, 0, sizeof s);
  s.pos_x = r->pos_x;
  s.pos_z = r->pos_z;
  s.target_x = r->target_x;
  s.target_z = r->target_z;
  s.delta = r->delta;
  s.facing = r->face7d;
  s.speed = r->speed71;
  s.vel_x = r->vel73;
  s.vel_z = r->vel75;
  /* FU319 §5 (phase-33): the live +0x43 latch and the default focus point —
   * the turn's close branch (`move_attr >> 16 <= 0x60`) steers the facing at
   * the camera triple `[0x157A73] -> 0x15774C/0x157754`; the words are the
   * native read width (MOV CX,[EDX] / MOV AX,[EDX+8]). */
  s.direct_face = r->row43;
  s.face_x = (int16_t)mr->render.camera.pos_x;
  s.face_z = (int16_t)mr->render.camera.pos_z;
  /* FU324 §3.2/§6 (phase-33 task 2): the staged high word is the native
   * block-A distance — FUN_0008DC68 over the record's low-word
   * target-minus-pos, stored at word +0x65 by the mover's own block A
   * (fresh bytes `0x8E273 CALL 0x0008dc68`, `0x8E278 MOV word[EBX+0x65],AX`);
   * `fifa96_entity_distance` is that metric's verbatim C port. The
   * 0x8DCD4-shaped `fifa96_arm_dist_stage` call previously staged here
   * computes the SAME value (FU324 §3.3: 4 000 800 sampled pairs, 0 diffs;
   * the mover's lane output was already unused), so the VALUE does not
   * change — this is the staging-SOURCE alignment, a measured no-op
   * (FU324 §5: 0 frames moved alone and 0 composed on the latch build).
   * The low half of the staged dword stays 0 — the native +0x63..+0x64 has
   * no writer and no word-width reader (FU324 §3.1, leg L2). The contract
   * pin's `staged_shape` (port/tests/test_action_locomotion.c:200-203) is a SHADOW
   * of this expression: any shape change here must land there in the same
   * commit (the pin re-states the staging because the value is never
   * persisted). */
  s.move_attr = (int32_t)((uint32_t)(uint16_t)fifa96_entity_distance(
      (int16_t)((uint16_t)r->target_x - (uint16_t)r->pos_x),
      (int16_t)((uint16_t)r->target_z - (uint16_t)r->pos_z)) << 16);
  s.heading = r->type;
  s.has_slot = r->has_slot;
  s.body_timer = r->body_timer9c;
  s.stride = (uint8_t)(r->timer7b & 0x0Fu);
  if (fifa96_action_locomotion_step(&s, fifa96_match_heading_table,
                                    fifa96_match_stride_table) != FIFA96_OK)
    return;
  r->pos_x = s.pos_x;
  r->pos_z = s.pos_z;
  r->face7d = s.facing;
  r->speed71 = s.speed;
  r->vel73 = s.vel_x;
  r->vel75 = s.vel_z;
  r->body_timer9c = s.body_timer;
  r->type = s.heading;
  /* The pool keeps both views of the native velocity bytes: the dword at
   * +0x71 = {word +0x71 speed, word +0x73 vel_x} and the dword at +0x73 =
   * {word +0x73 vel_x, word +0x75 vel_z}. Recompose them from the mover's
   * word writes so the live dword consumers (ball pairing `(int16_t)vel_x`
   * = +0x71, row 04/2A `vel_x >> 16` = +0x73, carrier `+0x71`) read the
   * native bytes, not stale copies. */
  r->vel_x = (int32_t)((uint32_t)(uint16_t)r->speed71 |
                       ((uint32_t)(uint16_t)r->vel73 << 16));
  r->vel_z = (int32_t)((uint32_t)(uint16_t)r->vel73 |
                       ((uint32_t)(uint16_t)r->vel75 << 16));
}

/* M2 phase-10 T2 (FU-147 §3.3 leg 7): the FUN_0007BF20 tail
 * `0x7C7D3..0x7C8FC` — the per-record camera event the native runs for every
 * record after the BF20 lane block, immediately before the record's own tail
 * exits. First-hand gate bytes (`disassemble_bytes 0x7C7D3..0x7C907`):
 *
 *   0x7C7D3  (int16)(lane) > 0x10                        -> skip
 *   0x7C7E2  word[+0x6B] >= word[+0x77] (fresh lane/bound) -> skip
 *   0x7C7F0  [0x157820] != 0                             -> skip
 *   0x7C7FD  [0x157822] != 0                             -> skip
 *   0x7C80A  word[0x1577BE] (bearing) <= 4               -> skip
 *   0x7C81B  dword[0x157750] == 0 (ball height)           -> skip
 *   0x7C828  tracked != 0: tracked[+0x8D]==0 || rec==tracked ||
 *            tracked[+0x91] in {8,9}                      -> skip
 *   0x7C861  dword[rec+0x5D] > dword[0x157750]            -> skip
 *   0x7C871  dword[rec+0x5D] + 0x70 < dword[0x157750]     -> skip
 *   then (after the 0x8F188 command-ring post, an OL-27 leg)
 *   0x7C8FC  CALL 0x70C08(rec)   (the camera event port)
 *
 * The dword/word aliases are the pool's low-word staging (the same derivation
 * convention the chase gate's `[0x157750] < 0x30` and `+0x5D == 0` use;
 * FU-75 §1.6): `pos_y` the +0x5D word, `lane_x` the +0x6B word, `bound` the
 * +0x77 word, `camera.speed` the 0x1577BE bearing and `camera.tracked` the
 * [0x1577CA] pool id. **M2 phase-11 T1 (L14.7):** the height word is the
 * exact native cell `dword[0x157750]` = the camera Y (`camera.pos_y`, the
 * FUN_00070B94 arc producer) — the earlier `ball.y` stand-in is superseded
 * now that the producer is live. **M2 phase-29 T4 (FU306 §5.2 item 2 /
 * L3):** `live_pos_y` is the record's just-stepped `+0x5D` (`r->pos_y`,
 * after the `0x7C057..0x7C113` jump arc), not the pool value the dispatch
 * writes back later — the native reads the live dword. */
static void match_run_record_camera_event(struct fifa96_match_run *mr,
                                          struct fifa96_match_entity *e,
                                          int16_t live_pos_y) {
  fifa96_camera *cam = &mr->render.camera;
  fifa96_camera_record_event_in in;
  fifa96_camera_record_event_out out;
  int32_t id = (int32_t)((uint32_t)e->team * FIFA96_MATCH_ENTITY_RECORDS +
                         e->index);
  int16_t ball = (int16_t)cam->pos_y;         /* 0x157750 (T1/L14.7) */
  int16_t height_5d = live_pos_y;
  if ((int16_t)e->lane_x > 0x10) return;                    /* 0x7C7D3 */
  if ((int16_t)e->lane_x >= (int16_t)e->bound) return;      /* 0x7C7E2 */
  if (mr->flag_157820 != 0 || mr->flag_157822 != 0) return; /* 0x7C7F0/FD */
  if ((int16_t)cam->speed <= 4) return;                     /* 0x7C80A */
  if (ball == 0) return;                                    /* 0x7C81B */
  if (cam->tracked != FIFA96_CAMERA_TRACKED_NONE) {         /* 0x7C828 */
    if (cam->tracked >= 0 &&
        cam->tracked <
            (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
      const struct fifa96_match_entity *t =
          &mr->entities
               .team[(uint32_t)cam->tracked / FIFA96_MATCH_ENTITY_RECORDS]
               .records[(uint32_t)cam->tracked % FIFA96_MATCH_ENTITY_RECORDS];
      if (t->active == 0) return;             /* tracked[+0x8D] == 0 */
      if (id == cam->tracked) return;         /* rec == tracked */
      if (t->code == 8u || t->code == 9u) return;   /* tracked[+0x91] in {8,9} */
    }
  }
  if (height_5d > ball) return;                   /* 0x7C861 */
  if ((int32_t)height_5d + 0x70 < (int32_t)ball) return;    /* 0x7C871 */
  in.skip_9a = e->skip_9a;
  in.action_91 = e->code;
  in.height_5d = height_5d;
  in.ball_height = ball;
  in.has_slot = e->has_slot;      /* FU303/FU294-L2 (phase-28 T8) */
  in.dir_x = e->dir_x;
  in.dir_z = e->dir_z;
  /* The FUN_0008F188 command-ring post (`0x7C8A2..0x7C8F8`) is the OL-27 leg;
   * the camera event runs unconditionally after it. The 0x92820 sink request
   * the port reports has no engine consumer yet (OL-27). FUN_00071C94 stores
   * its player argument into [0x1577CA] (0x71D27) after the 0x71C99 bail, so
   * an applied event rebinds the tracked record. */
  {
    int rc = fifa96_camera_record_event(cam, &mr->rng, &in, &out);
    if (rc == 1 && out.applied != 0)
      (void)fifa96_camera_set_tracked(cam, id);
  }
}

/* ===== T3 (OL-T4-1/FU-75): the FUN_0007CA54 input-row dispatch ============
 *
 * The native outfield record machine (records 1..10; record 0 runs the keeper
 * machine `FUN_000782D0` with its own tables) runs, before its current-handler
 * tail, the slot-edge dispatch `0x7CB0D..0x7CC11` (FU-75 §1.2/§1.3,
 * re-verified first-hand this task): with a bound control slot it selects a
 * dispatch code from the raw slot edge words (code 1 when `byte[rec+0x91] ==
 * 5`, `0x7CB36`) and runs the pressed (else released) 8-byte row table, whose
 * handlers install action codes through `FUN_0007D9A4` or write the team role
 * fields. `fifa96_outfield_input_row` is the ported subset; this seam binds it
 * over the pool for the slot-bearing outfield record and executes the
 * reachable handler subset, every gate read from the handler bytes this task
 * (Ghidra `disassemble_bytes` 0x7CE38/0x7CEB0/0x7CF20/0x7CF54/0x7CFD0/
 * 0x7D010/0x7D054/0x7D08C/0x7D110/0x7D174/0x7D0C4):
 *
 *   0x7CE38 install 0x0B invoke    0x7CEB0 install 8 invoke
 *   0x7CF20 receiver timer         0x7CF54 install 9 invoke
 *   0x7CFD0 chosen = rec, mode 0   0x7D010 install 0x21 invoke
 *   0x7D054 mode = 1               0x7D08C mode = 2
 *   0x7D110 install 7 invoke       0x7D174 install 0x23 no-invoke
 *   0x7D0C4 the 0x7E600 arm returns `type == 0x0E` (call leg L4.3)
 *   0x7D1D4 the self-check returns 0 (body leg)
 *   0x7CD60 the long-ball request (M2 phase-11 T1, FU-75 §1.4: the
 *          `[team+0x7CB]` callback + `+0x820 = 0x3C` arm; returns 1)
 *
 * Timing mirrors the native: a handler installs immediately and an `ECX=1`
 * install invokes the new handler in the same frame (`0x7D149`); the
 * record-machine tail then calls it again (`0x7CD29 CALL [rec+0x18]`), which
 * this seam reproduces by re-dispatching the installed row before the tail
 * dispatch below, so row 07's stage 0 and stage 1 both run on the install
 * frame while the FU-70 released word is still live in `word[slot+6]`.
 *
 * Staged stand-ins (this consumer's inputs): the `0x1577F0` track word
 * (`high_577ee_ge_50`, staged 0; the cell itself is the live
 * `camera.event_param` — the row-04 consumer staging remains OL-72), the
 * `[0x157AB0]`/`[0x1587AC]` flags and the
 * record `+0x99` byte (OL-72; the native handler zero-gates
 * `0x7CE63`/`0x7CF7F`/`0x7D19F` pass with the staged 0). The live `slot_word10`
 * is the FU-70 slot's previous-mapped word (native `slot[+0x10] = mapped`,
 * FU-70 §1.1) and the chase flags are wired from the pool (ball height
 * `0x157750`, the side compare, the `+0x5D` y byte).
 *
 * L4.1/L4.2 (M2 phase-9 T1): the machine tail's computed outputs are applied.
 * `out.no_edge_arm` (`0x7CC70`) copies the camera triple `0x5774C/50/54` into
 * the staging target and runs `FUN_00079B58` (`+0x93 = 0x10`, the `+0x99`
 * gate staged 0); `out.forced` (`FUN_0007C990` -> `0x7CA48
 * FUN_0007D9A4(rec, code, EBX=0, ECX=0)`) and `out.chase` (the code-8 gate
 * `0x7CD24`) install their codes through the pool installer, no invoke. The
 * native machine runs the forced/chase tail for every record (input dispatch
 * only for the slot-bearing ones); M2 phase-10 T2 widens the seam to every
 * outfield record (the unbound-record walk), so the chase arm's
 * `[rec+0x20] == 0` gate now fires for unbound records exactly as the native
 * `FUN_0008D8EC` walk calls `FUN_0007CA54` for records 1..10. */
struct match_input_ctx {
  struct fifa96_match_run *mr;
  struct fifa96_match_entity *e;
  uint32_t team;
  int32_t id;
  uint8_t invoke_code;   /* an ECX=1 install the seam dispatches after the scan */
};

_Static_assert(FIFA96_OUTFIELD_SELECT_RECORDS == FIFA96_MATCH_ENTITY_RECORDS,
               "the 0x7D1D4 NSEARCH block is the 11-record team block");

/* ===== M2 phase-52 T2 / FU-75 L4.5: the 0x7D1D4 engine binder ==============
 *
 * `FUN_0007D1D4` (`0x7D1D4..0x7D35E`; recon R12 §1) is a function with two
 * native callers: the released-row tables (`released_0[1]`, `released_2/3/4[0]`,
 * handler `0x7D1D4`) and the direct-arm pre-gate `0x7CB01 MOV EAX,EBP; 0x7CB03
 * CALL 0x7D1D4; 0x7CB08 JMP 0x7CC82`. Both dispatch through this binder so the
 * input-row handler and the `out.direct_arm` consumer apply one body.
 *
 * The state is staged from the pool in the native units (R12 §3 engine map):
 * `rec`'s absolute id / team base / `+0x59/+0x61` position word / `+0x6B`
 * `lane_x`; `[0x157A83]` = `entities.controlled`; `[team+0x7B2]` = `team->target`;
 * `[team+0x829]` = `team->search_gate`; `[0x157A4A]>>24` = `state.phase`;
 * `team+0x826` = `team->side`; `[0x157750]` dword = `camera.pos_y`;
 * `0x157788/8C/90` = `camera.anchor_x/y/z`; `word[0x1577C0]`/`word[0x1577C2]`
 * = the signed `camera.vel_x`/`vel_z` leads; the slot `+0x20`/`+0x21` bytes =
 * `slot.anim_b`/`anim_c` (the same source as the `match_run_dispatch_entity`
 * frame-staging block's `r->dir_x = mr->slot.anim_b; r->dir_z =
 * mr->slot.anim_c`). The selection binder's `[0x1587A8]`/`[0x1587AC]`
 * inputs stage `NONE` (the set-piece receiver id is instead carried by
 * `mr->sp_recv_1587ac` for the outfield skip); the derived `NONE/NONE`
 * comparison reproduces the native unstaged-pointer reject edge (Task 1).
 *
 * Apply, native order (`0x7D332..0x7D34C`):
 *   - `[team+0x7CB] = 0` (`0x7D33B`) unconditionally on a switch result; the
 *     helper co-sets `clear_team_7cb`/`slot_clear` with `switched`, and the
 *     binder consumes both flags instead of the `switched` shortcut;
 *   - `fifa96_match_run_slot_move` (`0x786A0`) runs the native from-held
 *     (`slot.entity == rec`, the `[rec+0x20]` pointer) / to-free precondition,
 *     moves the record-visible `has_slot` pair, the `[[slot]] = to` store and
 *     the dirs, and runs the moved block's clears: the `0x786A0` tail via
 *     `fifa96_control_slot_merge_reset` (words +4/+6/+8/+A/+C/+14/+16) plus
 *     the `0x78B00` `+0x24` byte. If the record is not the pointer owner the
 *     native call is a no-op and `0x78B00([result+0x20])` reads NULL.
 * The `0x7D33B` clear sits before the move because the two cells are disjoint
 * and the brief keeps the native store order in the binder.
 * FU75-L4.5a (tail ordering): the frame seam snapshots `chase.unbound` from
 * `e->has_slot` before the scan, while the native `0x7CD03` reads `[rec+0x20]`
 * after this call; a switch frame can defer the code-8 chase install one
 * frame (owner: the `fifa96_match_run` outfield seam). The same ordering
 * applies to the latent `0x7CB03` direct-arm consumer.
 * Returns the native handler return 0/1 (1 stops the row scan). */
int fifa96_match_run_select_step(struct fifa96_match_run *mr, int32_t id) {
  struct fifa96_match_team *team;
  struct fifa96_match_entity *e;
  fifa96_outfield_select_state s;
  fifa96_outfield_select_out out;
  fifa96_action_duel_candidate mates[FIFA96_OUTFIELD_SELECT_RECORDS];
  uint32_t t;
  uint32_t i;
  if (mr == NULL || id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  t = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  i = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  team = &mr->entities.team[t];
  e = &team->records[i];
  memset(&s, 0, sizeof s);
  memset(mates, 0, sizeof mates);
  for (uint32_t m = 0; m < FIFA96_OUTFIELD_SELECT_RECORDS; m++) {
    const struct fifa96_match_entity *mate = &team->records[m];
    mates[m].pos_x = (int16_t)mate->pos_x;
    mates[m].pos_z = (int16_t)mate->pos_z;
    mates[m].has_slot = mate->has_slot;
    mates[m].skip_98 = mate->skip_98;
    mates[m].skip_9a = mate->skip_9a;
    mates[m].is_chosen =
        team->chosen == (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + m) ? 1u : 0u;
  }
  s.rec_id = id;
  s.team_base = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS);
  s.pos_x = (int16_t)e->pos_x;                            /* rec +0x59 */
  s.pos_z = (int16_t)e->pos_z;                            /* rec +0x61 */
  s.lane_x = e->lane_x;                                   /* word rec+0x6B */
  s.controlled_id = mr->entities.controlled;              /* [0x157A83] */
  s.target_id = team->target;                             /* [team+0x7B2] */
  s.spawn_id = FIFA96_OUTFIELD_SELECT_NONE;               /* [0x1587A8] leg */
  s.recv_id = FIFA96_OUTFIELD_SELECT_NONE;                /* [0x1587AC] leg */
  s.spawn_side = 0;                                       /* no spawn record */
  s.spawn_8d = 0;
  s.record0_gate = team->search_gate;                     /* [team+0x829] */
  s.phase = (uint8_t)mr->state.phase;                     /* [0x157A4A]>>24 */
  s.side = team->side;                                    /* [team+0x826] */
  s.camera_y = mr->render.camera.pos_y;                   /* dword [0x157750] */
  s.anchor_x = mr->render.camera.anchor_x;                /* 0x157788 */
  s.anchor_y = mr->render.camera.anchor_y;                /* 0x15778C */
  s.anchor_z = mr->render.camera.anchor_z;                /* 0x157790 */
  s.lead_x = mr->render.camera.vel_x;                     /* word 0x1577C0 */
  s.lead_z = mr->render.camera.vel_z;                     /* word 0x1577C2 */
  s.slot_anim_b = mr->slot.entity == id ? (int8_t)mr->slot.anim_b : 0;
  s.slot_anim_c = mr->slot.entity == id ? (int8_t)mr->slot.anim_c : 0;
  s.mates = mates;
  s.mate_count = FIFA96_OUTFIELD_SELECT_RECORDS;
  if (fifa96_outfield_select_step(&s, &out) != FIFA96_OK) return 0;
  if (out.switched == 0 || out.new_controlled == FIFA96_OUTFIELD_SELECT_NONE)
    return out.ret;
  if (out.new_controlled < 0 ||
      out.new_controlled >=
          (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return out.ret;
  /* 0x7D33B: the team `+0x7CB` clear runs before the 0x7D348 slot clear (the
   * two cells are disjoint). The helper co-sets `clear_team_7cb` with
   * `switched`, so consuming the flag is behavior-identical and keeps the
   * advisory field honest. */
  if (out.clear_team_7cb != 0) team->timer7cb = 0;        /* 0x7D33B */
  /* 0x7D348: the 0x786A0 hand-off (owner, back-pointer, dirs, seven tail
   * words) plus the 0x78B00 `+0x24` clear. The helper applies the native
   * from-held/to-free precondition itself. */
  if (out.slot_clear != 0)
    (void)fifa96_match_run_slot_move(mr, id, out.new_controlled, 1);
  return out.ret;
}

int fifa96_match_run_slot_move(struct fifa96_match_run *mr, int32_t from_id,
                               int32_t to_id, int clear_byte24) {
  struct fifa96_match_entity *from;
  struct fifa96_match_entity *to;
  uint32_t from_team, from_index, to_team, to_index;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (from_id < 0 || to_id < 0 || from_id == to_id) return 0;
  if (from_id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS) ||
      to_id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  from_team = (uint32_t)from_id / FIFA96_MATCH_ENTITY_RECORDS;
  from_index = (uint32_t)from_id % FIFA96_MATCH_ENTITY_RECORDS;
  to_team = (uint32_t)to_id / FIFA96_MATCH_ENTITY_RECORDS;
  to_index = (uint32_t)to_id % FIFA96_MATCH_ENTITY_RECORDS;
  from = &mr->entities.team[from_team].records[from_index];
  to = &mr->entities.team[to_team].records[to_index];
  /* 0x786A2/0x786AC: from must be the native pointer owner, to must be free. */
  if (mr->slot.entity != from_id) return 0;
  if (to->has_slot != 0 || mr->slot.entity == to_id) return 0;
  /* 0x786B0..0x786B5: the pointer moves; the slot back-pointer follows. */
  mr->slot.entity = to_id;
  from->has_slot = 0;
  to->has_slot = 1;
  /* 0x786BF..0x786E3: the seven tail words. The native clears the moved
   * block; the block is the engine's single slot. */
  (void)fifa96_control_slot_merge_reset(&mr->slot);
  if (clear_byte24) mr->slot.byte24 = 0;
  /* The dirs live in the block (+0x20/+0x21), so they survive the move; the
   * per-record copies are projections and must follow. */
  to->dir_x = (int8_t)mr->slot.anim_b;
  to->dir_z = (int8_t)mr->slot.anim_c;
  from->dir_x = 0;
  from->dir_z = 0;
  /* There is one engine slot block. Heal every stale projection, not just the
   * donor, and cancel a queued merge so its drain cannot overwrite this move. */
  for (uint32_t team = 0; team < FIFA96_MATCH_ENTITY_TEAMS; team++) {
    for (uint32_t index = 0; index < FIFA96_MATCH_ENTITY_RECORDS; index++) {
      int32_t id = (int32_t)(team * FIFA96_MATCH_ENTITY_RECORDS + index);
      struct fifa96_match_entity *e = &mr->entities.team[team].records[index];
      if (id == to_id) continue;
      e->has_slot = 0;
      e->dir_x = 0;
      e->dir_z = 0;
    }
  }
  mr->entities.slot_merge = FIFA96_MATCH_ENTITY_NONE;
  if (mr->record.entity_id == from_id) {
    mr->record.has_slot = 0;
    mr->record.dir_x = 0;
    mr->record.dir_z = 0;
  } else if (mr->record.entity_id == to_id) {
    mr->record.has_slot = 1;
    mr->record.dir_x = (int8_t)mr->slot.anim_b;
    mr->record.dir_z = (int8_t)mr->slot.anim_c;
  }
  return 1;
}

static void match_input_install(struct match_input_ctx *c, uint8_t code, uint8_t invoke) {
  if (fifa96_match_entities_install(c->e, (uint8_t)c->mr->state.phase, code, 0) == 1 &&
      invoke != 0)
    c->invoke_code = code;
}

static int match_input_handler(uint32_t handler, void *context) {
  struct match_input_ctx *c = context;
  struct fifa96_match_entity *e = c->e;
  struct fifa96_match_team *team = &c->mr->entities.team[c->team];
  uint8_t phase = (uint8_t)c->mr->state.phase;
  uint8_t bits;
  switch (handler) {
    case 0x7CE38u:   /* pressed code-0 want 0x60: install 0x0B (invoke) */
      if (phase != 2u) return 0;
      bits = fifa96_outfield_type_bits(e->code);
      if ((bits & 1u) == 0u) return 0;
      if (e->anim_id == 0x0Cu || e->anim_id == 0x59u || e->anim_id == 0x5Eu)
        return 0;
      match_input_install(c, 0x0B, 1);
      return e->code == 0x0Bu;
    case 0x7CEB0u:   /* released code-0 want 0x30: install 8 (invoke) */
      if (phase != 2u) return 0;
      bits = fifa96_outfield_type_bits(e->code);
      if ((bits & 1u) == 0u) return 0;
      if (c->mr->entities.controlled < 0) return 0;
      if (team->side ==
          c->mr->entities
              .team[(uint32_t)c->mr->entities.controlled / FIFA96_MATCH_ENTITY_RECORDS]
              .side)
        return 0;
      match_input_install(c, 8u, 1);
      return e->code == 8u;
    case 0x7CF20u:   /* pressed/keeper arms want 0x80: receiver timer */
      if (phase != 2u) return 1;
      bits = fifa96_outfield_type_bits(e->code);
      if ((bits & 1u) != 0u) e->timer93 = 0x10;   /* 0x79B58 (+0x99 staged 0) */
      return 1;
    case 0x7CF54u:   /* released code-0 want 0x10: install 9 (invoke) */
      if (phase != 2u) return 0;
      bits = fifa96_outfield_type_bits(e->code);
      if ((bits & 1u) == 0u) return 0;
      if (c->mr->entities.controlled < 0) return 0;
      if (team->side ==
          c->mr->entities
              .team[(uint32_t)c->mr->entities.controlled / FIFA96_MATCH_ENTITY_RECORDS]
              .side)
        return 0;
      match_input_install(c, 9u, 1);
      return e->code == 9u;
    case 0x7CFD0u:   /* pressed code-3 want 0x10: chosen = rec, mode = 0 */
      if (phase == 2u) {
        bits = fifa96_outfield_type_bits(e->code);
        if ((bits & 3u) != 0u) {
          team->chosen = c->id;
          team->mode_82a = 0;
        }
      }
      return 1;
    case 0x7D010u:   /* released code-1 want 0x50: install 0x21 (invoke) */
      if (phase == 2u) {
        bits = fifa96_outfield_type_bits(e->code);
        if ((bits & 1u) != 0u) match_input_install(c, 0x21u, 1);
      }
      return 1;
    case 0x7D054u:   /* pressed code-4 want 0x10: mode = 1 */
      if (phase == 2u) {
        bits = fifa96_outfield_type_bits(e->code);
        if ((bits & 3u) != 0u) team->mode_82a = 1;
      }
      return 1;
    case 0x7D08Cu:   /* pressed code-4 want 0x40: mode = 2 */
      if (phase == 2u) {
        bits = fifa96_outfield_type_bits(e->code);
        if ((bits & 3u) != 0u) team->mode_82a = 2;
      }
      return 1;
    case 0x7D110u:   /* released code-1 wants 0x10..0x60: install 7 (invoke) */
      if (phase != 2u) return 0;
      bits = fifa96_outfield_type_bits(e->code);
      if ((bits & 1u) == 0u) return 0;
      match_input_install(c, 7u, 1);
      return e->code == 7u;
    case 0x7D174u:   /* pressed code-2 wants 0x10/0x40: install 0x23 */
      if (phase != 2u) return 0;
      bits = fifa96_outfield_type_bits(e->code);
      if ((bits & 1u) == 0u) return 0;
      if (e->pos_y != 0) return 0;
      match_input_install(c, 0x23u, 0);
      return e->code == 0x23u;
    case 0x7D0C4u:   /* released code-0 want 0x40: the 0x7E600 decision arm */
      /* The native CALL 0x7E600 install is leg L4.3; every path returns
       * `byte[rec+0x91] == 0x0E` (0x7D105 SETZ), so the no-install stand-in is
       * the handler's own return value. */
      return e->code == 0x0Eu;
    case 0x7CD60u: { /* pressed code-3/released code-0 want 0x40: the long-ball
                      * request (M2 phase-11 T1 / FU-75 §1.4, L14.6) */
      /* First-hand `disassemble_bytes 0x7CD60..0x7CDD5`:
       *   0x7CD6F  if ([0x157A4A]>>24 != 2) return 1;
       *   0x7CD76  if ([team+0x7CB] != 0) return 1;
       *   0x7CD7F  EBX = [team+0x7B2] (the team target record)
       *   0x7CD85  if (rec == EBX) { [team+0x7CB] = rec; [team+0x820] = 0x3C;
       *            return 1; }
       *   0x7CD9E  ESI = [0x1577CA]; require rec == ESI, then EBX != 0 and
       *            [EBX+0x20] == 0 (the target must be slot-free)
       *   0x7CDB4  FUN_000786A0(rec, EBX) (the slot swap; from-held/to-free)
       *   0x7CDB9  [team+0x7CB] = EBX; [team+0x820] = 0x3C;
       * The handler returns 1 on every path (the row scan stops). The
       * 0x815D0 code-7 stage-1 arm reads [team+0x7CB] == rec as `is_team_cb`
       * (mode 0x40). */
      struct fifa96_match_entity *target;
      int32_t id = c->id;
      if (phase != 2u) return 1;
      if (team->timer7cb != 0) return 1;
      /* The port stores the id with 0 as "none" where the native stores the
       * record pointer (never NULL). An id-0 (team-0 record 0) request would
       * collide with the sentinel and read as unarmed (`0x815D0`; record 0 is
       * the keeper machine's own record, never a pad/request actor) —
       * unreachable in the wired subset, left verbatim (final review). */
      if (id == team->target) {
        team->timer7cb = (uint32_t)id;
        team->timer820 = 0x3C;
        return 1;
      }
      if (id != c->mr->render.camera.tracked) return 1;
      if (team->target < 0 ||
          team->target >=
              (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
        return 1;
      target = &c->mr->entities
                    .team[(uint32_t)team->target / FIFA96_MATCH_ENTITY_RECORDS]
                    .records[(uint32_t)team->target % FIFA96_MATCH_ENTITY_RECORDS];
      if (target->has_slot != 0) return 1;
      /* 0x7CDB4: FUN_000786A0(rec, EBX). The helper owns the whole move — the
       * record-visible `has_slot` pair, the engine slot rebind (the FU-70
       * 0x57C64 block belongs to one entity at a time), the dirs and the seven
       * tail words. */
      (void)fifa96_match_run_slot_move(c->mr, c->id, team->target, 0);
      team->timer7cb = (uint32_t)team->target;   /* same id-0 sentinel caveat */
      team->timer820 = 0x3C;
      return 1;
    }
    case 0x7D1D4u:   /* the control-selection switch (M2 phase-52 T2 / L4.5) */
      /* `0x7D1D4..0x7D35E`: the full body through the pool binder (self gate,
       * camera-keyed lane gate, the anchor triple, the NSEARCH arm and the
       * `0x786A0` slot move + `0x7CB` clear + `0x78B00` clears). The scan's
       * first matching released word is 0x20 (PASS release). */
      return fifa96_match_run_select_step(c->mr, c->id);
    default:
      return 0;
  }
}

/* Returns the ECX=1 invoke code (> 0), 0 for no invoke, or a negative error. */
static int match_run_outfield_input(struct fifa96_match_run *mr,
                                    struct fifa96_match_entity *e,
                                    struct fifa96_match_run_record *r) {
  struct match_input_ctx c;
  fifa96_outfield_input_state s;
  fifa96_outfield_input_out out;
  uint32_t team = e->team;
  int32_t id = (int32_t)(team * FIFA96_MATCH_ENTITY_RECORDS + e->index);
  int32_t controlled = mr->entities.controlled;
  memset(&s, 0, sizeof s);
  s.has_slot = e->has_slot;
  /* P7 / FU-345: the live control follow owns the human slot. An AI record's
   * native `FUN_0007876C` merge can move `+0x20` earlier in the same frame's
   * record walk (lower team index), which would hide the human's input edges
   * from the authoritative slot record. The control follow restores the slot at
   * the frame tail; treat the run's current slot owner as bound for this
   * dispatch so the kick/release row still sees the human input. Live-only; the
   * M1/M2 tapes never set live_start. */
  if (mr->live_start && mr->slot.entity == id) s.has_slot = 1;
  s.phase = (uint8_t)mr->state.phase;
  if (mr->slot.entity == id) {
    s.pressed = mr->slot.pressed;
    s.released = mr->slot.released;
    s.slot_word10 = mr->slot.prev_mapped;   /* native slot[+0x10] = mapped */
  }
  /* FU311-L5 (phase-30 T2): the native `dword[+0x69]` — the low `+0x69` dz
   * word plus the `+0x6B` lane high word (`fifa96_outfield.c` reads it as
   * `lane >> 16`, 0x7CC2F). The port keeps the two words apart, so pack them
   * back here; the low word alone made the no-edge lane band read -1. */
  s.lane = (int32_t)((uint32_t)(uint16_t)e->lane |
                     ((uint32_t)(uint16_t)e->lane_x << 16));
  s.user_present = controlled >= 0 ? 1u : 0u;
  s.user_side =
      controlled >= 0
          ? mr->entities.team[(uint32_t)controlled / FIFA96_MATCH_ENTITY_RECORDS].side
          : 0u;
  s.side = mr->entities.team[team].side;
  s.type = e->code;             /* native byte +0x91: the dispatch-code gate */
  s.high_577ee_ge_50 = 0;       /* staged 0 (OL-72); the cell is live camera.event_param */
  s.tracked = (controlled >= 0 && id == controlled) ? 1u : 0u;
  s.user_absent_or_self = (controlled < 0 || id == controlled) ? 1u : 0u;
  s.chaser = (mr->entities.team[team].chosen >= 0 &&
              id == mr->entities.team[team].chosen)
                 ? 1u
                 : 0u;
  s.flag_157ab0 = mr->flag_157ab0;
  s.is_1578ac = (mr->sp_recv_1587ac >= 0 && id == mr->sp_recv_1587ac) ? 1u : 0u;
  s.forced.is_team_controlled =
      (id == mr->entities.team[team].target) ? 1u : 0u;
  s.forced.is_team_second = (id == mr->entities.team[team].second) ? 1u : 0u;
  s.forced.type_5 = e->code == 5u ? 1u : 0u;
  {
    int32_t oid = mr->entities.team[1u - team].target;
    int32_t tid = mr->entities.team[team].target;   /* native `ctrl` */
    if (oid >= 0 &&
        oid < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
      s.forced.opponent_has_ball =
          mr->entities.team[(uint32_t)oid / FIFA96_MATCH_ENTITY_RECORDS]
              .records[(uint32_t)oid % FIFA96_MATCH_ENTITY_RECORDS]
              .carrier & 1u;
    /* FUN_0007C990 `0x7CA13`: `ctrl = [team+0x7B2]` (the team target), not
     * `[0x157A83]` (`mr->entities.controlled`); the same block's
     * `is_team_controlled` gate reads `[team+0x7B2]` too, so both ball-bit
     * sources derive from the team target. */
    if (tid >= 0 &&
        tid < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
      s.forced.controlled_has_ball =
          mr->entities.team[(uint32_t)tid / FIFA96_MATCH_ENTITY_RECORDS]
              .records[(uint32_t)tid % FIFA96_MATCH_ENTITY_RECORDS]
              .carrier & 1u;
  }
  s.current_code = e->code;
  s.chase.phase = (uint8_t)mr->state.phase;
  s.chase.type_gate = (uint8_t)(fifa96_outfield_type_bits(e->code) & 1u);
  s.chase.not_team_controlled =
      (id != mr->entities.team[team].target) ? 1u : 0u;
  s.chase.not_team_second = (id != mr->entities.team[team].second) ? 1u : 0u;
  s.chase.distance = (uint16_t)e->lane_x;   /* 0x7CCCD dword[+0x69]>>16 */
  /* FU308 §4 site 4 (phase-29 T2): `0x7CCD8 CMP dword[0x157750],0x30; JGE
   * 0x7CD29` — the camera triple's Y (`camera.pos_y`), a signed 32-bit
   * compare; the FU-75 §1.6 ball-record stand-in is retired. */
  s.chase.camera = (int32_t)mr->render.camera.pos_y;
  s.chase.user_present = s.user_present;
  s.chase.sides_differ =
      (s.user_present != 0u && s.side != s.user_side) ? 1u : 0u;
  /* FU75-L4.5a (phase-52 fix round 1): this snapshot is taken before the
   * released-row scan, while native `0x7CD03` reads `[rec+0x20]` after the
   * `0x7D1D4` call, so a switch frame defers the code-8 chase install by one
   * frame for the record that gave up the slot. Owner: this outfield seam. */
  s.chase.unbound = e->has_slot == 0 ? 1u : 0u;
  s.chase.timer = e->timer81;
  s.chase.third_zero = ((uint32_t)e->pos_y == 0u) ? 1u : 0u;  /* +0x5D */
  c.mr = mr;
  c.e = e;
  c.team = team;
  c.id = id;
  c.invoke_code = 0;
  if (fifa96_outfield_input_row(&s, match_input_handler, &c, &out) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  /* 0x7CB03: the direct-arm pre-gate calls `FUN_0007D1D4` and jumps straight to
   * the 0x7CC82 tail (`0x7CB01..0x7CB08`), replacing the row scan. The gate is
   * live when the set-piece setup has latched `[0x157AB0]`; the receiver id
   * supplies the native 0x7CACD exclusion while that set piece is active. The
   * consumer is wired to the same binder the released-row handler uses. */
  if (out.direct_arm != 0) (void)fifa96_match_run_select_step(mr, id);
  /* The native tail (step 5) runs after the input-row scan (step 3) and so
   * gates on the post-install `byte[+0x91]`; the loader state is the entry
   * snapshot, so when a scan handler installed a code, recompute the
   * forced/chase outputs against the live code (the type gate and the
   * type-5 keep both read `+0x91`). The role fields stay the entry snapshot
   * (`is_team_controlled`/`is_team_second`, and the chase
   * `not_team_controlled`/`not_team_second`): the native tail re-reads the
   * live `[team+0x7B2]`/`[team+0x7B6]`, which can differ only if a scan
   * handler writes them in the same frame — the reachable handler subset
   * writes neither (the `0x7CFD0` chosen arm writes `team->chosen`, the
   * team `+0x7BF`), so the snapshot is exact today; revisit when a
   * role-writing scan handler lands. */
  if (e->code != s.current_code) {
    uint8_t next = 0;
    out.forced = 0;
    out.forced_code = 0;
    out.chase = 0;
    s.current_code = e->code;
    s.type = e->code;
    s.forced.type_5 = e->code == 5u ? 1u : 0u;
    if (s.phase == 2u && (fifa96_outfield_type_bits(e->code) & 1u) != 0u) {
      if (fifa96_outfield_forced_action(&s.forced, e->code, &next) == 1) {
        out.forced = 1;
        out.forced_code = next;
      }
      if (fifa96_outfield_chase_gate(&s.chase, e->code, e->code, &next) == 1)
        out.chase = 1;
    }
  }
  /* 0x7CC70 no-edge arm (L4.2): the camera triple into the output target and
   * `FUN_00079B58` (+0x99 staged 0). */
  if (out.no_edge_arm != 0) {
    r->target_x = mr->render.camera.pos_x;
    r->target_y = mr->render.camera.pos_y;
    r->target_z = mr->render.camera.pos_z;
    e->timer93 = 0x10;
  }
  /* 0x7CD24/L4.1: the forced decision and chase installs, no invoke. */
  if (out.forced != 0)
    (void)fifa96_match_entities_install(e, s.phase, out.forced_code, 0);
  if (out.chase != 0)
    (void)fifa96_match_entities_install(e, s.phase, 8u, 0);
  return (int)c.invoke_code;
}

/* FU-153 / M2 phase-12 T1 (15.a): the resolved animation frame's aux word —
 * FUN_0006E490 copies `word[frame+2]` into `rec+0x36` (`0x6E50A`) and the
 * FUN_0006E8E8 switch consumes its low byte. The engine reads the frame record
 * from the ported `match_run_frame_records` (the row table's relocated
 * `frame_table` pointer minus the region base plus the frame index*5). Returns
 * 0 when the row/frame is out of the ported bounds. */
static uint16_t match_run_frame_aux(uint8_t anim_id, uint8_t frame) {
  fifa96_anim_row row;
  uint32_t off;
  if (fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
                                  (int32_t)anim_id, &row) != FIFA96_OK)
    return 0;
  if (row.frame_table < 0x10E200u) return 0;
  off = (row.frame_table - 0x10E200u) + (uint32_t)frame * 5u + 2u;   /* aux word */
  if (off + 2u > (uint32_t)sizeof match_run_frame_records) return 0;
  return (uint16_t)((uint16_t)match_run_frame_records[off] |
                    ((uint16_t)match_run_frame_records[off + 1u] << 8));
}

/* FU-186-L2 (Slice-B Task 10): FUN_0006E444 (0x6E444..0x6E488) — the
 * animation-duration summation the row-1C tail calls at 0x77E74 to reseed
 * `timer81`. Native: `ESI = rec+0x28`, `idx = (int8)byte[rec+0x3D]` (0x6E45E
 * MOVSX AX, byte[EAX+0x15]), `ptr = [rec+0x28]`, `base = [ptr+0x4]`
 * (frame_table), `count = byte[ptr+0x1]` (last_frame); then sums
 * `word[base + j*5]` (the frame duration word) for `j = idx..count` and
 * returns `((uint16)sum) >> 4` (0x6E480 SHR AX,0x4). The engine models the
 * native row pointer / frame byte as `anim_id` / `frame`; the frame durations
 * come from the ported `match_run_frame_records`. */
uint16_t fifa96_match_run_anim_duration(uint8_t anim_id, uint8_t frame) {
  fifa96_anim_row row;
  uint32_t sum = 0;
  int32_t idx = (int32_t)(int8_t)frame;   /* 0x6E45E MOVSX AX, byte[rec+0x3D] */
  if (idx < 0) return 0;                  /* native OOB read; never live */
  if (fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
                                  (int32_t)anim_id, &row) != FIFA96_OK)
    return 0;
  if (row.frame_table < 0x10E200u) return 0;
  for (; idx <= (int32_t)row.last_frame; idx++) {   /* 0x6E479 CMP AX,CX / JLE */
    uint32_t off = (row.frame_table - 0x10E200u) + (uint32_t)idx * 5u;
    if (off + 2u > (uint32_t)sizeof match_run_frame_records) break;
    sum += (uint16_t)match_run_frame_records[off] |        /* 0x6E46A word[EDX] */
           ((uint16_t)match_run_frame_records[off + 1u] << 8);
  }
  return (uint16_t)(((uint16_t)sum) >> 4);                /* 0x6E480 SHR AX,0x4 */
}

/* FU-159 §4.1 (T3): the native +0x44 producer — `row_flags & 1` written by
 * every row install (`FUN_0006E598` `0x6E6CC`; pool twin `FUN_0008DF84`
 * `0x8DFC1`). The engine derives it from the live `anim_id` (the same value
 * the install wrote, since `anim_id` only changes at install/successor). */
static uint8_t match_run_row_terminal_flag(uint8_t anim_id) {
  fifa96_anim_row row;
  if (fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
                                  (int32_t)anim_id, &row) != FIFA96_OK)
    return 0;
  return (uint8_t)(row.flags & 0x01u);
}

/* The FU-85 resolver's `row +8` sprite bank for an animation row id (the
 * replay stage re-derives `bank_index` from the applied appearance — the
 * record carries no bank field). */
int fifa96_match_run_anim_bank(int32_t anim_id, uint8_t *bank_index) {
  fifa96_anim_row row;
  if (!bank_index) return 0;
  if (fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
                                  anim_id, &row) != FIFA96_OK)
    return 0;
  *bank_index = row.sprite_bank;
  return 1;
}

/* FU319 §5 writer 3 (phase-33): the kickoff placement's narrow +0x43 install —
 * the native tail `FUN_0006E598(rec, active ? 0 : 0x26, 0)` (`0x79BF1..0x79C13`)
 * writes `byte[rec+0x43] = row_flags & 2` (0x6E6BF..0x6E6C4). The pool-side
 * placement (`fifa96_match_entities.c`) installs the row without routing the
 * full `FUN_0006E598` (whose `anim_timer`/`aux_seen` side effects are
 * collateral there), so it asks for the narrow write: `row43` only; the
 * native `+0x44 = 1` already reproduces through the same `row.flags & 1`
 * derivation at the dispatch staging. No-op on a lookup miss (row43 keeps
 * its pool-init 0). */
void fifa96_match_run_record_kickoff_row43(struct fifa96_match_entity *e,
                                           int32_t anim_id) {
  fifa96_anim_row row;
  if (e == NULL) return;
  if (fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
                                  anim_id, &row) != FIFA96_OK)
    return;
  e->row43 = (uint8_t)(row.flags & 0x02u);
}

/* FU-159 §5.4 / FU-160 §1.4 (T3): the FUN_0006E598 store core (see the header
 * doc for the fresh evidence). */
int fifa96_match_run_record_anim_install(struct fifa96_match_run *mr,
                                         struct fifa96_match_entity *e,
                                         int32_t id, uint8_t frame) {
  fifa96_anim_row row;
  uint32_t slot;
  if (mr == NULL || e == NULL) return -FIFA96_ERR_INVALID;
  if (id < 0 || id > 0x6E) id = 0;                    /* 0x6E68E..0x6E69B */
  if (fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
                                  id, &row) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  if ((int32_t)frame > (int32_t)row.last_frame) frame = 0;   /* FUN_0006E490 */
  e->anim_id = (uint8_t)id;
  e->frame = frame;
  e->row44 = (uint8_t)(row.flags & 0x01u);            /* 0x6E6CC */
  e->row43 = (uint8_t)(row.flags & 0x02u);            /* 0x6E6BF..0x6E6C4
                                                       * (FU319 §5 writer 1) */
  e->aux_seen = FIFA96_MATCH_ENTITY_AUX_SEEN_NONE;    /* force the +0x36 re-arm */
  slot = (uint32_t)e->team * FIFA96_MATCH_ENTITY_RECORDS + e->index;
  if (slot < FIFA96_MATCH_RUN_RENDER_SLOTS)
    mr->render.entities[slot].anim_timer = 0;         /* word[+0x32] = 0 */
  return FIFA96_OK;
}

/* FU172 §4 (Tier 2): the pool movement/steering tables `0x11052A`/`0x1105AA`,
 * 16 `{row, duration}` pairs indexed by `speed71` (2*speed byte offset). The
 * terminal row re-selects here on every frame-timer expiry; the duration is
 * also the terminal frame duration (FU172 §5). The `+0x99` row-0x5A special
 * case is inert (no engine record writes it; FU172-L4). */
typedef struct match_run_steering_pair {
  uint8_t row;
  uint8_t duration;
} match_run_steering_pair;

static const match_run_steering_pair match_run_steering_active[16] = {
    {0x00, 0x50}, {0x01, 0x50}, {0x01, 0x44}, {0x02, 0x40},
    {0x02, 0x38}, {0x02, 0x34}, {0x03, 0x30}, {0x03, 0x24},
    {0x03, 0x23}, {0x03, 0x22}, {0x03, 0x21}, {0x03, 0x20},
    {0x03, 0x1F}, {0x03, 0x1E}, {0x03, 0x1D}, {0x03, 0x1C},
};
static const match_run_steering_pair match_run_steering_inactive[16] = {
    {0x26, 0x40}, {0x29, 0xB0}, {0x29, 0x90}, {0x2A, 0x70},
    {0x2A, 0x60}, {0x2A, 0x50}, {0x2A, 0x40}, {0x2A, 0x3F},
    {0x2A, 0x3E}, {0x2A, 0x3D}, {0x2A, 0x3C}, {0x2A, 0x3B},
    {0x2A, 0x3A}, {0x2A, 0x39}, {0x2A, 0x38}, {0x2A, 0x37},
};

static const match_run_steering_pair *match_run_steering_table(
    const struct fifa96_match_entity *e) {
  return e->active ? match_run_steering_active : match_run_steering_inactive;
}

static int match_run_steering_index(const struct fifa96_match_entity *e,
                                    int8_t turn) {
  int index = (int)(uint16_t)e->speed71;
  if (index > 15) index = 15;                 /* native reads past the table;
                                               * the engine clamps the metric */
  if (turn < 0 && e->speed71 != 0) index++;   /* the native +2 path */
  if (index > 15) index = 15;
  return index;
}

/* FU-159 §3.1/§5.2 (T3): the FUN_0008E008 frame-wrap row successor (see the
 * header doc for the fresh evidence). */
int fifa96_match_run_record_anim_successor_turn(struct fifa96_match_entity *e,
                                                int8_t turn) {
  fifa96_anim_row row;
  fifa96_anim_row next_row;
  uint8_t next;
  int32_t frame;
  if (e == NULL) return -FIFA96_ERR_INVALID;
  if (fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
                                  (int32_t)e->anim_id, &row) != FIFA96_OK)
    return FIFA96_OK;
  /* Byte44 is the derived row-flags bit 0. Terminal rows use the movement
   * steering tables at every expiry; a same-row cell keeps the current frame
   * (the frame resolve path). The install frame is frame + lean (EBX at
   * 0x8E0F4/0x8E156; the lean is the render driver's anim_turn, default +1). */
  if ((row.flags & 0x01u) != 0) {
    int index = match_run_steering_index(e, turn);
    next = match_run_steering_table(e)[index].row;
    if (next == e->anim_id) return FIFA96_OK;
    if (fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
                                    (int32_t)next, &next_row) != FIFA96_OK)
      return FIFA96_OK;
    frame = (int32_t)e->frame + (int32_t)turn;
    if (frame < 0) frame = (int32_t)next_row.last_frame;   /* FUN_0008DF00 */
    if (frame > (int32_t)next_row.last_frame) frame = 0;
    e->anim_id = next;
    e->frame = (uint8_t)frame;
    e->row44 = (uint8_t)(next_row.flags & 0x01u);
    e->row43 = (uint8_t)(next_row.flags & 0x02u);
    e->aux_seen = FIFA96_MATCH_ENTITY_AUX_SEEN_NONE;
    return FIFA96_OK;
  }
  next = row.next_id;                                  /* row+3 */
  frame = (int32_t)row.last_frame + 1;   /* byte[+0x46] = 1 at install (0x6E6D8) */
  if (next == 0x6Fu) {
    next = row.anim_id;                                /* keep the current row */
  } else if (next >= 0x70u) {
    /* `FUN_00079C50(rec, -dir[oct])`: the 0x10F334/0x10F33C vectors negate to
     * the (oct + 4) & 7 direction, so the facing octant flips by 4. */
    e->type = (uint8_t)((e->type + 4u) & 7u);
    next = (uint8_t)(next - 0x70u);
  }
  if (fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
                                  (int32_t)next, &next_row) != FIFA96_OK)
    return FIFA96_OK;
  if (frame > (int32_t)next_row.last_frame) frame = 0;  /* FUN_0006E490 clamp */
  e->anim_id = next;
  e->frame = (uint8_t)frame;
  e->row44 = (uint8_t)(next_row.flags & 0x01u);
  e->row43 = (uint8_t)(next_row.flags & 0x02u);       /* 0x8DFB6..0x8DFBA
                                                       * (FU319 §5 writer 2) */
  e->aux_seen = FIFA96_MATCH_ENTITY_AUX_SEEN_NONE;
  return FIFA96_OK;
}

int fifa96_match_run_record_anim_successor(struct fifa96_match_entity *e) {
  fifa96_anim_row row;
  if (e == NULL) return -FIFA96_ERR_INVALID;
  if (fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
                                  (int32_t)e->anim_id, &row) != FIFA96_OK)
    return FIFA96_OK;
  /* The legacy/non-live callers keep the terminal no-op; the live scene stage
   * opts into the FU172 steering through the _turn entry. */
  if ((row.flags & 0x01u) != 0) return FIFA96_OK;
  return fifa96_match_run_record_anim_successor_turn(e, 0);
}

/* FU-161 §5.4 (T3): the arm dispatch consumes the aux word's two bytes as two
 * passes (low first, then high; CL 0/8) and each one-shot arm XORs its own
 * byte out of `rec+0x36` (`0x6FBE6..0x6FBFF`), so it fires once per frame
 * resolve; sustained arms leave the byte and re-fire every frame the frame
 * holds. The engine's ported arms: 0x0E/0x3A one-shot, 0x0F/0x05 sustained. */

/* FU-159 §2.3 (T3): the `FUN_00079B1C` snap store set (fresh decompile
 * `0x79B1C..0x79B4B`): target triple := position triple; `word[+0x69] = 0`
 * (lane), `word[+0x71] = 0` (speed71), `byte[+0x9C] = 0` (body timer), then
 * `word[+0x67] = word[+0x65] = word[+0x69]` (dx/distance) and
 * `word[+0x75] = word[+0x73] = word[+0x71]` (the velocity words). The engine
 * keeps both velocity views in lockstep (the mover consumes the words; the
 * +0x65/+0x67 words are recomputed by the mover/lane staging, FU-161 rec). */
static void match_run_record_snap(struct fifa96_match_run_record *r) {
  r->target_x = r->pos_x;
  r->target_y = r->pos_y;
  r->target_z = r->pos_z;
  r->lane = 0;
  r->speed71 = 0;
  r->vel73 = 0;
  r->vel75 = 0;
  r->vel_x = 0;
  r->vel_z = 0;
  r->body_timer9c = 0;
}

/* The `FUN_0006E490` frame clamp (`0x6E490..0x6E4C4`): `frame < 0 -> last`,
 * `frame > last -> 0`. */
static uint8_t match_run_frame_clamp(uint8_t anim_id, int32_t frame) {
  fifa96_anim_row row;
  if (fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
                                  (int32_t)anim_id, &row) != FIFA96_OK)
    return 0;
  if (frame < 0) return row.last_frame;
  if (frame > (int32_t)row.last_frame) return 0;
  return (uint8_t)frame;
}

/* FU-161 §3.1/§5.2 (T3): the facing-relative impulsion — the case-0x0D
 * (`0x6EE1B..0x6EE5B`, `<<2`), case-0x29 (`0x6F5E8..0x6F647`, `<<2` for the
 * row set `{0x4D,0x59,0x11,0x5E}` else `×2`) and case-0x2A (`0x6E997..0x6E9FB`,
 * `0x5E` `<<2` else `×2`) common store set:
 *   `oct = byte[+0x8E]` (the fresh `MOV EAX,[EBP+0x8B]; SAR EAX,0x18`),
 *   `vel73 = dir_x[oct] << shift`, `vel75 = dir_z[oct] << shift`
 *   (`0x10F334`/`0x10F33C`, first-hand) and
 *   `speed71 = FUN_0008DC68(vel73, vel75)` (`0x6EE52` = the ported
 *   `fifa96_entity_distance`: 4 axis / 5 diagonal).
 * The dword views (+0x71/+0x73) recompose the word writes for the mover
 * (match_run.c:544-547 convention). */
static void match_run_record_impulse(struct fifa96_match_run_record *r, int shift) {
  int oct = (int)(r->type & 0x7u);
  int32_t vx = (int32_t)fifa96_match_dir_x[oct] * (1 << shift);
  int32_t vz = (int32_t)fifa96_match_dir_z[oct] * (1 << shift);
  r->vel73 = (int16_t)vx;
  r->vel75 = (int16_t)vz;
  r->speed71 = (int16_t)fifa96_entity_distance(vx, vz);
  r->vel_x = (int32_t)((uint32_t)(uint16_t)r->speed71 |
                       ((uint32_t)(uint16_t)r->vel73 << 16));
  r->vel_z = (int32_t)((uint32_t)(uint16_t)r->vel73 |
                       ((uint32_t)(uint16_t)r->vel75 << 16));
}

/* FU-153 / M2 phase-12 T1 (15.a): the FUN_0006E8E8 jump arms, first-hand
 * `disassemble_bytes` this task (`0x6ED08..0x6ED69`, `0x6EDE6..0x6EE3E`,
 * `0x6EE90..0x6EEB7`, `0x6FB2F..0x6FB5C`) and `decompile_function 0x6E8E8`.
 * The switch index is `(aux & 0xFF) - 1`, so the arm for aux `0x0E` is case
 * `0xD` etc. Ported subset (the height/timer writes; the arm's velocity,
 * sine-step and `+0x83`-marker writes stay legs — FU153-L1):
 *  - aux 0x0E (case 0xD): `+0x83 = 0x10` (`0x6EDE6`), then
 *    `+0x85 = +0x87 = ramp((dword[+0x81]) >> 16) = ramp(word[+0x83]) = ramp(0x10)`
 *    (`0x6EDEF`/`0x6EDF5`/`0x6EDF8`, stores `0x6EDFD`/`0x6EE04`), `pos_y = 0x10`
 *    (`0x6EE14`); **one-shot** (`0x6EE5F..0x6EE6C XOR`).
 *  - aux 0x3A (case 0x39): `+0x83 = 0x18` (`0x6ED08`), then the same alias
 *    read `ramp(word[+0x83]) = ramp(0x18)` (`0x6ED11`/`0x6ED17`/`0x6ED1A`,
 *    stores `0x6ED1F`/`0x6ED26`), `pos_y = 0x18` (`0x6ED33`); **one-shot**
 *    (`0x6EDB8..0x6EDC3 XOR`).
 *  - (the unported aux 0x25 arm has the same shape: `+0x83 = 0x50` at
 *    `0x6FB2F`, `ramp(word[+0x83])` at `0x6FB38`/`0x6FB3E`/`0x6FB41`);
 *  - aux 0x0F (case 0xE): `if (pos_y != 0) frame := 3 via FUN_0006E490`
 *    (`0x6EE7B..0x6EE8B`, bytes `bb 03 00 00 00` / `e8 05 f6 ff ff`); else the
 *    `FUN_00079B1C` snap then `if (+0x85 == 0) +0x85 = ramp(pos_y + 0x1C);
 *    pos_y = 0; +0x87 = 0` (`0x6EE90..0x6EEBE`); **sustained**.
 *  - aux 0x05 (case 4): `MOV EAX,EBP; CALL 0x79B1C` (`0x6EA63`, bytes
 *    `e8 b2 b0 00 00`), the stop; **sustained**.
 * **Erratum (T1 fix round 1, fifth `dword>>16` alias strike):** the ramp
 * argument is the arm's own `+0x83` marker (the just-stored 0x10/0x18/0x50),
 * NOT `word[+0x81]`: `dword[+0x81] >> 16` is the word at `+0x83` (the trap);
 * the decayed countdown `word[+0x81]` is a different cell (`0x7CA5C`
 * `CMP word[EAX+0x81],0` / `0x7CA78` zeroes it). `ramp(0x10) = 15` (crest 16)
 * and `ramp(0x18) = 18` are the live bases. Returns `aux` with each fired
 * one-shot arm's byte consumed (0x0E/0x3A; FU-161 §5.4) — the caller writes
 * the return back to `r->aux_word`. */
static uint16_t match_run_record_jump_arm(struct fifa96_match_run_record *r,
                                          uint16_t aux) {
  uint16_t word = aux;
  for (int pass = 0; pass < 2; pass++) {
    uint8_t b = (uint8_t)(pass == 0 ? (word & 0xFFu) : (word >> 8));
    int16_t base;
    if (b == 0) continue;
    switch (b) {
      case 0x0E:
        base = fifa96_camera_ramp(0x10);   /* ramp(word[+0x83]), the marker above */
        r->pos_y = 0x10;
        r->jump_base = base;
        r->jump_timer = base;
        match_run_record_impulse(r, 2);    /* 0x6EE1B..0x6EE5B: dir[oct]<<2 */
        word &= (uint16_t)(pass == 0 ? 0xFF00u : 0x00FFu);   /* one-shot */
        break;
      case 0x3A:
        base = fifa96_camera_ramp(0x18);
        r->pos_y = 0x18;
        r->jump_base = base;
        r->jump_timer = base;
        word &= (uint16_t)(pass == 0 ? 0xFF00u : 0x00FFu);   /* one-shot */
        break;
      case 0x0F:
        if (r->pos_y != 0) {
          r->frame = match_run_frame_clamp(r->anim_id, 3);   /* 0x6EE7B */
        } else {
          match_run_record_snap(r);                          /* 0x6EE92 */
          if (r->jump_base == 0)
            r->jump_base = fifa96_camera_ramp((int16_t)(r->pos_y + 0x1C));
          r->pos_y = 0;
          r->jump_timer = 0;
        }
        break;   /* sustained */
      case 0x05:
        match_run_record_snap(r);                            /* 0x6EA65 */
        break;   /* sustained */
      case 0x2A: {
        /* 0x6F597..0x6F647: row `{0x4D,0x59,0x11,0x5E}` -> `<<2`, else `×2`;
         * sustained (the carry re-applies while its frame holds). */
        int i;
        int shift = 1;
        static const uint8_t rows[] = {0x4D, 0x59, 0x11, 0x5E};
        for (i = 0; i < 4; i++)
          if (r->anim_id == rows[i]) shift = 2;
        match_run_record_impulse(r, shift);
        break;
      }
      case 0x2B:
        /* 0x6E960..0x6EA09: while `jump_timer < 2*jump_base` re-apply the
         * impulse (row 0x5E -> `<<2`, else `×2`) and freeze the frame timer
         * (`word[+0x32] = 0`, `0x6EA03`); sustained, so it re-fires every
         * frame the frame holds. */
        if ((int32_t)(int16_t)r->jump_timer <
            2 * (int32_t)(int16_t)r->jump_base) {
          match_run_record_impulse(r, r->anim_id == 0x5Eu ? 2 : 1);
          r->frame_freeze = 1;
        }
        break;
      case 0x02:
        /* FU-172 §7 (phase-14 T3, Tier 1; fresh `0x6E930..0x6E95F`): the
         * row-0x12 f3 airborne freeze-only twin of 0x2B — while
         * `jump_timer < 2*jump_base` zero the frame timer (`word[EAX+0xA] =
         * 0`, the rec+0x32 cell; no impulse, no XOR); sustained. */
        if ((int32_t)(int16_t)r->jump_timer <
            2 * (int32_t)(int16_t)r->jump_base)
          r->frame_freeze = 1;
        break;
      case 0x0A:
        /* FU-172 §7 (phase-14 T3, Tier 1; fresh case-index 9 -> `0x6EEB7`):
         * the row-0x12 f0/f1 landing tail — `MOV dword[EBP+0x5D],0` (pos_y)
         * then `MOV word[EBP+0x87],0` (jump_timer), then the common
         * second-pass tail (sustained: the byte is not XORed out). Guards a
         * stale arc at the row head. */
        r->pos_y = 0;
        r->jump_timer = 0;
        break;
      case 0x0C:
        /* 0x6EBFB..0x6EC48: grounded target step — `if (pos_y != 0) skip;
         * target := pos; target_x += dir_x[oct]<<5; target_z +=
         * dir_z[oct]<<5` (the `0x10F331`/`0x10F339` dword bases resolve to
         * the same 0x10F334/0x10F33C dir bytes through the `>>24` alias,
         * fresh this task); sustained. */
        if (r->pos_y == 0) {
          int oct = (int)(r->type & 0x7u);
          r->target_x = r->pos_x + (int32_t)fifa96_match_dir_x[oct] * 0x20;
          r->target_y = r->pos_y;
          r->target_z = r->pos_z + (int32_t)fifa96_match_dir_z[oct] * 0x20;
        }
        break;
      default:
        break;
    }
  }
  return word;
}

/* FU-153 / M2 phase-12 T1 (15.a): the FUN_0007BF20 jump arc block
 * `0x7C057..0x7C113` (first-hand `disassemble_bytes`): the `+0x85 != 0` gate
 * at `0x7C057/0x7C05F`, the `+0x87 += delta` advance (`0x7C065..0x7C07A`),
 * the landing edge `+0x87 > 2*(+0x85)` (`0x7C094/0x7C096`) zeroing pos_y and
 * both words and copying position -> target (`0x7C098..0x7C0BA`), else the
 * shared 0x10F3C4 arc into pos_y (`0x7C0BF..0x7C10C`, `fifa96_camera_arc_height`).
 * Runs for every dispatched record right after the mover (the native block
 * sits between the mover blocks and the `0x7C776` lane block). */
static void match_run_record_jump_step(struct fifa96_match_run_record *r) {
  int32_t base;
  int32_t t;
  if (r->jump_base == 0) return;                                  /* 0x7C05F */
  t = (int16_t)(uint16_t)((uint16_t)r->jump_timer + (uint16_t)r->delta);
  r->jump_timer = (int16_t)t;                                     /* 0x7C07A */
  base = (int16_t)r->jump_base;
  if (t > 2 * base) {                                             /* 0x7C096 */
    r->pos_y = 0;                                                 /* 0x7C098 */
    r->jump_base = 0;                                             /* 0x7C09F */
    r->jump_timer = 0;                                            /* 0x7C0AE */
    r->target_x = r->pos_x;                                       /* 0x7C0B7 */
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
    return;
  }
  r->pos_y = fifa96_camera_arc_height((int16_t)base, (int16_t)t); /* 0x7C101 */
}

/* FU-142b (phase-12 T1) / T4 review (phase-28): the post-handler velocity
 * decomposition. The arm rows write the dword views `vel_x`/`vel_z`
 * (+0x71/+0x73 as a 16.16 pair), so the word views the mover reads
 * (`speed71` +0x71, `vel73` +0x73, `vel75` +0x75) are rebuilt from them after
 * every action body and before `match_run_record_mover`. A row that writes a
 * native word directly (row 0x1A's `'T'` metric) must mirror it into the dword
 * view or this pass would replace it with the previous frame's value — the
 * row-0x1A T4 review finding. Exported so the row-level pins exercise the
 * exact dispatch pass (NULL is a no-op). */
void fifa96_match_run_decompose_velocity_words(struct fifa96_match_run_record *r) {
  if (r == NULL) return;
  r->speed71 = (int16_t)r->vel_x;
  r->vel73 = (int16_t)((uint32_t)r->vel_x >> 16);
  r->vel75 = (int16_t)((uint32_t)r->vel_z >> 16);
}

/* FU-141: one pool record -> the FU-138/FU-140 staging record -> the FU-137
 * action dispatch, then the handler's requests back into the pool record. The
 * bound FU-70 slot's animation/direction bytes (native slot +0x20/+0x21, the
 * `slot+0x1D`/`slot+0x1E` high bytes row 00 reads) feed the move target. The
 * row classification is unchanged: an unported row returns
 * -FIFA96_ERR_UNSUPPORTED and the pool chain skips it. */
static int match_run_dispatch_entity(void *ctx, struct fifa96_match_entity *e) {
  struct fifa96_match_run *mr = ctx;
  struct fifa96_match_run_record *r = &mr->record;
  int32_t id = (int32_t)((uint32_t)e->team * FIFA96_MATCH_ENTITY_RECORDS + e->index);
  int rc;
  r->pos_x = e->pos_x;
  r->pos_y = e->pos_y;
  r->pos_z = e->pos_z;
  r->jump_base = e->jump_base;
  r->jump_timer = e->jump_timer;
  r->target_x = e->target_x;
  r->target_y = e->target_y;
  r->target_z = e->target_z;
  r->timer89 = e->timer89;
  r->timer81 = e->timer81;
  r->delta = (uint16_t)mr->state.frame_delta;
  r->active = e->active;
  r->has_slot = e->has_slot;
  /* P7 / FU-345: the live control follow owns the FU-70 slot while the human
   * record is the run's slot owner; a same-frame native merge request may have
   * moved the pool byte already. Keep the authoritative owner bound for this
   * record's dispatch (live-only; M1/M2 never set live_start). */
  if (mr->live_start && mr->slot.entity == id) r->has_slot = 1;
  r->has_ball = e->has_ball;
  r->stage = e->stage;
  r->stage92 = e->stage92;
  r->timer7b = e->timer7b;
  r->lane = e->lane;
  r->lane_x = e->lane_x;   /* FU311-L5: the +0x6B high word of dword[+0x69] */
  /* FU-142e: reproduce the unported FUN_0008D098 pre-switch walk (`0x8D11E`),
   * which calls 0x8DCD4(pos, target) into `+0x65/+0x67/+0x69` for every free
   * record before the installer arms; the derived staging computes the word
   * row 2A gates on from this call's pos/target. The +0x69 lane stays owned by
   * the rows that consume it (26/27/28/29 recompute it in their prologues). */
  {
    fifa96_arm_vec from = { r->pos_x, r->pos_y, r->pos_z };
    fifa96_arm_vec to = { r->target_x, r->target_y, r->target_z };
    int32_t distance = 0;
    int32_t lane_unused = 0;
    (void)fifa96_arm_dist_stage(&from, &to, &distance, &lane_unused);
    r->distance = distance;
  }
  r->type = e->type;
  /* FU-159 §4.1 (T3): +0x44 is the current anim row's flags bit 0, produced
   * by every row install (0x6E6CC); deriving it from the live `anim_id` is
   * equivalent (anim_id only changes at install/successor) and replaces the
   * always-zero pool pass-through that blocked the kick-07 stage-2 reset. */
  r->row44 = match_run_row_terminal_flag(e->anim_id);
  /* FU319 §5 (phase-33): the +0x43 latch is staged from the pool cell, never
   * derived from anim_id — it survives the 0x799BF row-pointer clear and the
   * row-0x22 clear, so a derivation would re-invent it (FU319 §5 rejected
   * shortcut). */
  r->row43 = e->row43;
  r->code = e->code;   /* native +0x91: the kick/row-04 gates index this byte */
  r->anim_id = e->anim_id;   /* OL-80: byte[[rec+0x28]] live row id */
  r->frame = e->frame;       /* OL-80: native +0x3D live frame index */
  r->entity_id = id;
  r->vel_x = e->vel_x;
  r->vel_z = e->vel_z;
  /* FU-142d: the row-28 pool fields. The team-side/flag830 and the resolved
   * [team+0x831] chosen-record position are this call's inputs; the five
   * process globals come from the run (producers unported, OL-56). */
  r->side = mr->entities.team[e->team].side;
  r->flag830 = mr->entities.team[e->team].flag830;
  r->scratch_a2 = e->scratch_a2;
  r->scratch_a6 = e->scratch_a6;
  r->scratch_aa = e->scratch_aa;
  r->scratch_ae = e->scratch_ae;
  r->scratch_a0 = e->scratch_a0;
  r->scratch_a1 = e->scratch_a1;
  r->global_10f358 = mr->global_10f358;
  r->global_10f35c = mr->global_10f35c;
  r->global_10f364 = mr->global_10f364;
  r->global_10f368 = mr->global_10f368;
  r->global_157ac2 = mr->global_157ac2;
  {
    const struct fifa96_match_team *team = &mr->entities.team[e->team];
    r->chosen_ok = 0;
    r->chosen_x = 0;
    r->chosen_y = 0;
    r->chosen_z = 0;
    if (team->chosen831 >= 0 &&
        (uint32_t)team->chosen831 <
            FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS) {
      uint32_t ct = (uint32_t)team->chosen831 / FIFA96_MATCH_ENTITY_RECORDS;
      uint32_t ci = (uint32_t)team->chosen831 % FIFA96_MATCH_ENTITY_RECORDS;
      r->chosen_x = mr->entities.team[ct].records[ci].pos_x;
      r->chosen_y = mr->entities.team[ct].records[ci].pos_y;
      r->chosen_z = mr->entities.team[ct].records[ci].pos_z;
      r->chosen_ok = 1;
    }
  }
  r->player_d = (int8_t)e->desc_d; /* native rec[+4][+0xD]: the pool's
                      * producer byte (phase-35 `desc_d`; the roster index
                      * itself is still unmeasured/staged 0 — FU330-L1), read
                      * by row 26 as the 0x10F394 table index */
  r->player_e = 0;   /* native rec[+4][+0xE] (same) */
  r->desc_c = 0;     /* FU301: rec[+4][+0xC] (unlocated producer, staged 0) */
  r->desc_f = 0;     /* FU301: rec[+4][+0xF] */
  r->desc_1b = 0;    /* FU301: rec[+4][+0x1B] */
  r->desc_16 = 0;    /* rows09-0c: rec[+4][+0x16] (FU178-L7/FU245-L3) */
  r->steer83 = 0;    /* FU302: +0x83 (written by the 0x76B28 port) */
  r->place_offset_x = e->place_offset_x;
  r->place_offset_z = e->place_offset_z;
  r->ran = e->ran;
  r->helper_request = 0;
  r->controlled = 0;
  r->place_valid = 0;
  r->install = 0;
  if (mr->slot.entity == id) {
    r->dir_x = (int8_t)mr->slot.anim_b;   /* native slot +0x20 */
    r->dir_z = (int8_t)mr->slot.anim_c;   /* native slot +0x21 */
  } else {
    r->dir_x = e->dir_x;
    r->dir_z = e->dir_z;
  }
  r->face7d = e->face7d;
  r->speed71 = e->speed71;
  r->vel73 = e->vel73;
  r->vel75 = e->vel75;
  r->body_timer9c = e->body_timer9c;
  /* T3/L4.1 (M2 phase-10 T2): the outfield record machine's per-frame step for
   * every outfield record (native records 1..10 via FUN_0007CA54; record 0 is
   * the keeper machine's own tables). The slot-edge dispatch (step 3) runs only
   * for the slot-bearing record inside `fifa96_outfield_input_row`; the
   * forced-decision/chase tail (step 5) runs for every record, so unbound
   * records now install codes 3/4/6 and chase 8 exactly as the native
   * `0x8DB2E..0x8DB5F` walk does (`e->skip_9a` records are already skipped by
   * the pool walk). An ECX=1 install invoked the new row during the scan;
   * re-stage the fields the install changed and run it, so the tail dispatch
   * below runs it a second time in the same frame (the native installer invoke
   * + `0x7CD29 CALL [rec+0x18]` pair). */
  if (e->index != 0u) {
    int invoke = match_run_outfield_input(mr, e, r);
    if (invoke < 0) return invoke;
    /* The machine tail's installs (the input-row handlers' invoke installs and
     * the L4.1 forced/chase installs) write `+0x91/+0x92/+0x89/+0x9E` on the
     * record before the same frame's `0x7CD29 CALL [rec+0x18]`; re-stage them
     * so the tail dispatch runs the installed row against its own fields. */
    r->code = e->code;
    r->stage92 = e->stage92;
    r->timer89 = e->timer89;
    r->ran = e->ran;
    if (invoke > 0) {
      rc = fifa96_match_dispatch_action(mr, e->code);
      if (rc != FIFA96_OK && rc != -FIFA96_ERR_UNSUPPORTED) return rc;
    }
  }
  /* FU290/FU292 (phase-27 T5): the native `[0x157C5C]` is ONE process cell — the
   * keeper ladder writes it (`0x7802D`/`0x780D7`) and the row-`0x1B` body reads
   * it at `0x76FF4` in the SAME frame. The T5 in-frame publish predates FU299
   * (phase-28 T2, Option B): the per-step hug now publishes the ladder's write
   * to `mr->global_157c5c` right after the record-0 step (before this
   * dispatch), so this re-publish is value-identical (the view was bound from
   * the cell at that step); it only consumes `latch_written`. The flag fires
   * only when the ladder ran (`permit != 0`), so M2 is untouched. */
  if (mr->keeper_cpu.latch_written) {
    mr->global_157c5c = mr->keeper_cpu.latch_157c5c;
    mr->keeper_cpu.latch_written = 0;
  }
  rc = fifa96_match_dispatch_action(mr, e->code);
  /* FU-153 / T1: the native per-record tail runs FUN_0006E8E8 (the animation
   * event machine) right after the action handler and before the mover
   * (`0x7CD2B CALL [rec+0x18]` -> `0x7CD30 CALL 0x6E8E8` -> `0x7CD48 CALL
   * 0x7BF20`). Its arms are keyed on the resolved frame's aux word
   * (`rec+0x36`); FU-161 §5.4 (T3): the word is re-armed on the (anim_id,
   * frame) resolve edge and the one-shot arms clear their own byte, so the
   * engine reads `e->aux_word`, not the raw frame aux every tick. The action
   * handler may have installed a row through the helper (which sets
   * `aux_seen = FIFA96_MATCH_ENTITY_AUX_SEEN_NONE`), so the edge check runs
   * after the dispatch. */
  {
    uint16_t seen = (uint16_t)(((uint16_t)e->anim_id << 8) | e->frame);
    if (seen != e->aux_seen) {                    /* the +0x36 resolve edge */
      e->aux_seen = seen;
      e->aux_word = match_run_frame_aux(e->anim_id, e->frame);
    }
  }
  r->aux_word = e->aux_word;
  r->frame_freeze = 0;
  r->aux_word = match_run_record_jump_arm(r, r->aux_word);
  /* The arm handlers (rows 26/28/2A) write the dword views of the velocity
   * bytes; decompose them back onto the word views the mover reads so the
   * native `+0x71/+0x73/+0x75` bytes stay in lockstep (an arm's `vel_x=0`
   * therefore clears the words too, and a pass-through is identity because the
   * staging invariant holds). Shared with the row-level pins. */
  /* FU319 §5 (phase-33) + the final-review correction: the action bodies
   * install rows on the pool entity (`fifa96_match_run_record_anim_install`
   * writes `e->row43`), while the staging record's latch was copied at the
   * frame head (`r->row43 = e->row43` above). Re-stage the pool byte after
   * the dispatch, before the mover: without this the end-of-dispatch
   * write-back (`e->row43 = r->row43` below) restores the STALE pre-dispatch
   * byte and reverts every in-dispatch install (rows 0x4F/0x0F/0x5E/0x12/…),
   * leaving writer 1 inert in-engine; the mover's same-frame `direct_face`
   * read would stay stale too. The row-0x22 clear already writes both views,
   * so the re-stage is value-identical for it. */
  r->row43 = e->row43;
  fifa96_match_run_decompose_velocity_words(r);
  /* FU-147 S1: every dispatched record runs the shared mover (native
   * FUN_0007CA54 tail 0x7CD48 / FUN_000782D0 tail 0x785C5), then the BF20
   * lane block refreshes the +0x6B lane / +0x6D/+0x6F camera deltas / +0x77
   * bound and the per-team camera-nearest tracker. Native
   * `0x7C776..0x7C7CF`, first-hand this slice; the metric call is 0x8DC68 =
   * `fifa96_entity_distance`. The camera focus 0x15774C/0x157754 is the
   * engine render-camera stand-in (FU-147 leg 13). */
  match_run_record_mover(mr);
  /* FU-153 / T1: the BF20 jump arc runs after the mover blocks and before the
   * 0x7C776 lane block (`0x7C057..0x7C113`). */
  match_run_record_jump_step(r);
  /* FU-341 (phase-51 T2): the BF20 tail's last-defender tracker
   * (`0x7C741..0x7C770`, between the `0x7C73E` z integration and the
   * `0x7C776` lane block): active records only (`byte[rec+0x8D] != 0`), then
   * the extreme-z store toward the team's own goal. The jump arc does not
   * touch z, so `r->pos_z` is the native `[rec+0x61]` the block reads. */
  if (r->active != 0)
    (void)fifa96_match_entities_last_defender_step(&mr->entities.team[e->team],
                                                   (int32_t)e->index, r->pos_z);
  {
    int16_t lane = 0;
    int16_t cam_dx = 0;
    int16_t cam_dz = 0;
    if (fifa96_action_locomotion_track(r->pos_x, r->pos_z, mr->render.camera.pos_x,
                                       mr->render.camera.pos_z, &lane, &cam_dx,
                                       &cam_dz) == FIFA96_OK) {
      struct fifa96_match_team *team = &mr->entities.team[e->team];
      int32_t tracker = team->tracker7c7;
      e->bound = e->lane_x;             /* 0x7C77E: +0x77 := old +0x6B */
      e->lane_z = cam_dx;               /* 0x7C78E: +0x6D */
      e->cam_dz6f = cam_dz;             /* 0x7C79A: +0x6F */
      e->lane_x = lane;                 /* 0x7C7AF: +0x6B */
      /* 0x7C7B3..0x7C7CF: the tracker keeps an incumbent with an equal or
       * smaller lane (signed word JGE at 0x7C7C8); a NONE tracker or a
       * strictly smaller fresh lane replaces it. The native `[team+0x7C7]`
       * is a record pointer; the pool stores the team-relative index. */
      if (tracker == FIFA96_MATCH_ENTITY_NONE ||
          (int16_t)e->lane_x < (int16_t)team->records[tracker].lane_x)
        team->tracker7c7 = (int32_t)e->index;
    }
  }
  /* The BF20 tail's per-record camera event (native 0x7C8FC, after the lane
   * block): FU-147 §3.3 leg 7, first-hand gate bytes at the helper above.
   * FU306 §5.2 item 2: the native tail reads the live `+0x5D`; `r->pos_y`
   * carries the just-stepped jump arc (the pool write-back below lags it). */
  match_run_record_camera_event(mr, e, (int16_t)r->pos_y);
  e->pos_x = r->pos_x;
  e->pos_y = r->pos_y;
  e->pos_z = r->pos_z;
  e->jump_base = r->jump_base;
  e->jump_timer = r->jump_timer;
  e->target_x = r->target_x;
  e->target_y = r->target_y;
  e->target_z = r->target_z;
  e->timer89 = r->timer89;
  e->timer81 = r->timer81;   /* native +0x81: handler reloads must persist */
  e->stage92 = r->stage92;
  e->timer7b = r->timer7b;
  e->lane = r->lane;
  e->type = r->type;
  e->row44 = r->row44;       /* FU-159 §4.1: the derived row-flags bit 0 */
  e->row43 = r->row43;       /* FU319 §5: the pool and the staging record agree
                              * (the row-0x22 clear writes both directly) */
  e->aux_word = r->aux_word; /* FU-161 §5.4: the one-shot word write-back */
  e->frame_freeze = r->frame_freeze;   /* FU-161 §5.2: the freeze request */
  e->anim_id = r->anim_id;   /* OL-80: the arm bodies' `anim_sel` write-back */
  e->frame = r->frame;
  e->vel_x = r->vel_x;
  e->vel_z = r->vel_z;
  e->scratch_a2 = r->scratch_a2;
  e->scratch_a6 = r->scratch_a6;
  e->scratch_aa = r->scratch_aa;
  e->scratch_ae = r->scratch_ae;
  e->scratch_a0 = r->scratch_a0;
  e->scratch_a1 = r->scratch_a1;
  e->ran = r->ran;
  e->has_ball = r->has_ball;   /* FU-147 S1: the +0x9B possession-flag
                                * producer (row-1E claim 0x755C0) must persist
                                * on the pool, not only on the staging record */
  e->install = r->install;
  e->helper_request = r->helper_request;
  e->controlled = r->controlled;
  e->place_valid = r->place_valid;
  e->place_x = r->place_x;
  e->place_y = r->place_y;
  e->place_z = r->place_z;
  e->dir_x = r->dir_x;
  e->dir_z = r->dir_z;
  e->face7d = r->face7d;
  e->speed71 = r->speed71;
  e->vel73 = r->vel73;
  e->vel75 = r->vel75;
  e->body_timer9c = r->body_timer9c;
  /* FU-142e: row 2A writes the team `+0x830` flag (arms 0/1) and the
   * `[0x10F358]`/`[0x10F35C]` process globals (arms 0/9/10); the native writes
   * are process-wide, so the staged values land back on the pool/run for the
   * later records of the same frame (the frame staging reads them again). */
  mr->entities.team[e->team].flag830 = r->flag830;
  mr->global_10f358 = r->global_10f358;
  mr->global_10f35c = r->global_10f35c;
  return rc;
}

/* FU-141 frame inputs: the phase/delta plus the FU-71 camera triple used as
 * the three selection vectors (the native 0x157770/0x157788/0x157794 shadow
 * the 0x15774C camera position at init, FU-67 §4.1). `word[0x1577FA]` is the
 * live camera timer (bound to `timer_a`); only its sibling bucket timers
 * 0x157800/0x157806 and the FU-67 S3 selection producer remain unported/zero.
 * The 0x10F37C/0x10F388 intercept targets are the
 * static table `{(0,0,0x990),(0,0,-0xA90)}` (FU-151 §2.8, first-hand
 * `read_memory 0x10F37C`), staged live this slice. */
static void match_run_entity_frame(const struct fifa96_match_run *mr,
                                   struct fifa96_match_entities_frame *f) {
  memset(f, 0, sizeof *f);
  f->phase = (uint8_t)mr->state.phase;
  f->delta = (uint16_t)mr->state.frame_delta;
  for (int v = 0; v < (int)FIFA96_MATCH_ENTITY_SELECT_VECTORS; v++) {
    f->select_vector[v][0] = mr->render.camera.pos_x;
    f->select_vector[v][1] = mr->render.camera.pos_y;
    f->select_vector[v][2] = mr->render.camera.pos_z;
  }
  f->cam_z = (int16_t)mr->render.camera.pos_z;
  /* FU290 (phase-27 T5): the stage-select machine inputs. */
  f->cam_pos_x = mr->render.camera.pos_x;
  f->cam_pos_y = mr->render.camera.pos_y;
  f->cam_pos_z = mr->render.camera.pos_z;
  /* FU308 (phase-29 T1): the keeper CX=1 gate `0x78046 MOV EAX,[0x1577BC];
   * 0x7804B SAR EAX,0x10; 0x7804E CMP EAX,0xA` reads the LIVE word
   * `0x1577BE` — `(int16)camera.speed`, written by the walk tail (0x73CB8)
   * and the follow/pan arms. `camera.follow_speed` is the derived pre-tail
   * snapshot captured at the 0x739CE follow gate (before those writes); it
   * stays for that gate only (`fifa96_match_run_camera_follow`), not here. */
  f->follow_speed = (int16_t)mr->render.camera.speed;
  f->pan_counter = mr->render.camera.pan_counter;
  f->global_157a4c = mr->global_157a4c;
  /* FU300 (phase-28 T4; fix-wave unification): the native raw dwords
   * 0x1577BE (low word `camera.speed`, high word `camera.vel_x`) and
   * 0x1577C0 (low word `camera.vel_x`, high word `camera.vel_z`). The lead
   * words are the one camera pan pair — row 0x1A and row 0x1B Block E write
   * them, so there is no second run-level cell. `[0x1577FA]` (timer_a) is the
   * live camera timer; the sibling bucket timers 0x157800/0x157806 and their
   * selection producer remain FU-67 S3/z (zero). */
  f->lead_x = (int32_t)((uint32_t)(uint16_t)mr->render.camera.speed |
                        ((uint32_t)(uint16_t)mr->render.camera.vel_x << 16));
  f->lead_z = (int32_t)((uint32_t)(uint16_t)mr->render.camera.vel_x |
                        ((uint32_t)(uint16_t)mr->render.camera.vel_z << 16));
  f->timer_a = (int16_t)mr->render.camera.timer;
  /* FU303-L10 (phase-28 T8): the keeper-CPU ladder's FU292-L2 band cells are
   * LIVE — `band_fa` is the camera timer (`word[0x1577FA]`) and `band_f6` is
   * its ramp start (`word[0x1577F6]`, folded by `camera_event_ramp` as
   * `timer_limit - ramp_divisor`; the 0x705e8 `F8 = F4 - F6` identity,
   * FU303-L7). */
  f->band_f6 = mr->render.camera.band_1577f6;
  f->band_fa = (int16_t)mr->render.camera.timer;
  f->latch_c44 = mr->predict_c44;               /* FU-179 §4 (FU292-L1) */
  f->target_c48[0] = mr->predict_c48_x;
  f->target_c48[1] = mr->predict_c48_y;
  f->target_c48[2] = mr->predict_c48_z;
  f->intercept_x[0] = 0;
  f->intercept_x[1] = 0;
  f->intercept_y[0] = FIFA96_MATCH_ENTITY_INTERCEPT_Z0;
  f->intercept_y[1] = FIFA96_MATCH_ENTITY_INTERCEPT_Z1;
}

/* FU-141 drains the pool leaves to the engine: a successful slot merge moves
 * the FU-70 slot binding (and runs the FUN_00078670 clear), and a pending
 * row-1E placement triple resets the FU-71 camera through the existing
 * `fifa96_camera_init` (the FU-71 port of FUN_000700F4's field reset). */
static void match_run_entity_drain(struct fifa96_match_run *mr) {
  int32_t merged = fifa96_match_entities_take_slot_merge(&mr->entities);
  int32_t px = 0;
  int32_t py = 0;
  int32_t pz = 0;
  if (merged >= 0) {
    mr->slot.entity = merged;
    (void)fifa96_control_slot_merge_reset(&mr->slot);
  }
  if (fifa96_match_entities_take_place(&mr->entities, &px, &py, &pz) == 1) {
    (void)fifa96_camera_init(&mr->render.camera, px, py, pz);
  }
}

/* FU-343 (P1): the derived live camera follow. The native FU-71 event/pan
 * producers are event-driven (the live path has no kickoff event; A2 §2), so
 * the live windowed match uses the ball as the focus while no native pan event
 * is active. `render.camera` stays the FU-71 focus triple; the rendered eye is
 * the separate class-3 side stand-in (fifa96_match_run_eye). The `tracked` bind is kept
 * on the controlled record (the native 0x1577CA writer is the camera event;
 * the engine's row-04/ball-receive producers may rebind it later). Derived
 * stand-in for the unported FU-310 camera place, not a native producer; the
 * native rig smoothing and the live orientation director stay open legs. */
/* Phase 71 — the live render-eye first-frame seed [D].
 *
 * The FU-71 focus cell is the simulation camera; the rendered eye is the
 * native class-3 side camera now owned by FUN_0004DB38. This seed gives the
 * first live frame the side framing (focus + side offset) before the native
 * handler takes over. It keeps the simulation cell exact and clamps the
 * render-only eye to the native FU-71 bounds (`|x| <= 0x6C0`, `|z| <= 0xAB0`)
 * with a 0x30 step. It is not a claim of the native director (P10-L6). */
#define MATCH_RUN_RENDER_EYE_MAX_STEP 0x30
#define MATCH_RUN_RENDER_EYE_SNAP 0x600
#define MATCH_RUN_RENDER_EYE_BOUND_X 0x6C0
#define MATCH_RUN_RENDER_EYE_BOUND_Z 0xAB0

static int32_t match_run_render_eye_abs(int32_t v) {
  return v < 0 ? -v : v;
}

static int32_t match_run_render_eye_step(int32_t cur, int32_t target) {
  int32_t d = target - cur;
  if (d > MATCH_RUN_RENDER_EYE_MAX_STEP) d = MATCH_RUN_RENDER_EYE_MAX_STEP;
  if (d < -MATCH_RUN_RENDER_EYE_MAX_STEP) d = -MATCH_RUN_RENDER_EYE_MAX_STEP;
  if (match_run_render_eye_abs(target - (cur + d)) <= 2) return target;
  return cur + d;
}

static void match_run_render_eye_update(struct fifa96_match_run *mr) {
  int32_t fx, fy, fz, tx, ty, tz;
  struct fifa96_match_run_render *r;
  if (!mr || !mr->live_start) return;
  r = &mr->render;
  if (r->render_focus_valid) {
    fx = r->render_focus_x;
    fy = r->render_focus_y;
    fz = r->render_focus_z;
  } else if (r->view_focus_valid) {
    fx = r->view_focus_x;
    fy = r->view_focus_y;
    fz = r->view_focus_z;
  } else {
    fx = r->camera.pos_x;
    fy = r->camera.pos_y;
    fz = r->camera.pos_z;
  }
  if (fx > MATCH_RUN_RENDER_EYE_BOUND_X) fx = MATCH_RUN_RENDER_EYE_BOUND_X;
  if (fx < -MATCH_RUN_RENDER_EYE_BOUND_X) fx = -MATCH_RUN_RENDER_EYE_BOUND_X;
  if (fz > MATCH_RUN_RENDER_EYE_BOUND_Z) fz = MATCH_RUN_RENDER_EYE_BOUND_Z;
  if (fz < -MATCH_RUN_RENDER_EYE_BOUND_Z) fz = -MATCH_RUN_RENDER_EYE_BOUND_Z;
  tx = fx + FIFA96_MATCH_RUN_SIDE_X;
  ty = fy + FIFA96_MATCH_RUN_SIDE_Y;
  tz = fz + FIFA96_MATCH_RUN_SIDE_Z;
  if (r->render_eye_valid == 0u ||
      match_run_render_eye_abs(tx - r->render_eye_x) > MATCH_RUN_RENDER_EYE_SNAP ||
      match_run_render_eye_abs(ty - r->render_eye_y) > MATCH_RUN_RENDER_EYE_SNAP ||
      match_run_render_eye_abs(tz - r->render_eye_z) > MATCH_RUN_RENDER_EYE_SNAP) {
    r->render_eye_x = tx;
    r->render_eye_y = ty;
    r->render_eye_z = tz;
    r->render_eye_valid = 1u;
    return;
  }
  r->render_eye_x = match_run_render_eye_step(r->render_eye_x, tx);
  r->render_eye_y = match_run_render_eye_step(r->render_eye_y, ty);
  r->render_eye_z = match_run_render_eye_step(r->render_eye_z, tz);
}

/* FU-351: the class-3 side-camera constants. [0x108B7C] = 500 is the live
 * chase distance; FUN_00049388 returns [0x107300] (the 100 Hz elapsed time
 * x12/5), which is 4x the port's frame_delta at the 30 Hz grant. */
#define MATCH_RUN_CLASS3_DISTANCE 500
#define MATCH_RUN_CLASS3_TICK_SCALE 4

/* FU351-L2c-3: the native view records 0x107508+slot*0x70 all start at
 * (-1920,160,30), ratio 0x1500, yaw 0xBF9C, pitch 0xC8. The behavior row is
 * the record +0xC; slots 0/1/2 carry 0/1/2. The native view-change arm
 * (FUN_0004D04C) runs FUN_0004D908 20 times after copying 0x109A70 into the
 * record target/history; the engine keeps the per-frame focus refresh and
 * uses the same 20-step settle on a slot/variant change. */
#define MATCH_RUN_NATIVE_VIEW_SETTLE 20

/* FUN_0004CE34's selector gate: FUN_00053D9C() is false when the overlay
 * state [0x14E578] is 0 or 6, and FUN_00053D50() values 0x13/0x14/0x1C/0x1D
 * skip FUN_0004D04C. */
static int match_run_view_selector_allowed(
    const struct fifa96_match_run_render *r) {
  uint32_t mode = r->camera_pose.view_mode;
  if (mode == 0x13u || mode == 0x14u || mode == 0x1Cu || mode == 0x1Du)
    return 0;
  return r->display.state == 0 || r->display.state == 6;
}

static int32_t match_run_native_target_clamp(int32_t v, int32_t lo, int32_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static void match_run_native_view_init(struct fifa96_match_run_render *r,
                                       int32_t class_of) {
  fifa96_camera_handler_state *st = &r->native_view_cam[class_of];
  memset(st, 0, sizeof *st);
  st->pos_x = -1920;
  st->pos_y = 160;
  st->pos_z = 30;
  st->yaw = 0xBF9C;
  st->pitch = 0xC8;
  st->ratio = 0x1500;
  /* The active view record 0x107968+slot*0x70 carries behavior row 3/4/5
   * (camera record 0x107508+slot*0x70 carries 0/1/2; the rows are duplicates
   * in the image). FUN_0004F1C8 overlays the variant. */
  (void)fifa96_camera_behavior_block_variant(
      &r->native_view_block[class_of], (uint32_t)(class_of + 3),
      r->native_view_variant);
  r->native_view_valid[class_of] = 1u;
}

/* FUN_0004F1C8's per-selection overlay: target0 0x340/0xB4 and the selected
 * C record half. The mutable pan/cell state is preserved. */
static void match_run_native_view_apply_variant(
    struct fifa96_match_run_render *r, int32_t class_of) {
  const fifa96_camera_behavior_table_entry *src =
      &fifa96_camera_behavior_blocks[class_of + 3];
  fifa96_camera_handler_block *block = &r->native_view_block[class_of];
  block->target0 = r->native_view_variant == 1 ? 0xB4 : src->target0;
  if (r->native_view_variant == 1) {
    block->c0 = src->v1_c0;
    block->c1 = src->v1_c1;
    block->c2 = src->v1_c2;
    block->c3 = src->v1_c3;
    block->sub10 = src->v1_sub10;
    block->sub14 = src->v1_sub14;
    block->sub18 = src->v1_sub18;
    block->sub1c = src->v1_sub1c;
  } else {
    block->c0 = src->c0;
    block->c1 = src->c1;
    block->c2 = src->c2;
    block->c3 = src->c3;
    block->sub10 = src->sub10;
    block->sub14 = src->sub14;
    block->sub18 = src->sub18;
    block->sub1c = src->sub1c;
  }
}

static int match_run_native_view_handler(int32_t class_of,
                                         fifa96_camera_handler_state *st,
                                         fifa96_camera_handler_block *block,
                                         int32_t tick,
                                         fifa96_camera_handler_out *out) {
  switch (class_of) {
    case 0:
      return fifa96_camera_behavior_steady(st, block, tick, out);
    case 1:
      return fifa96_camera_behavior_sidetrack(st, block, tick, out);
    case 2:
      return fifa96_camera_behavior_staged(st, block, tick, out);
    default:
      return -FIFA96_ERR_INVALID;
  }
}

/* FUN_00036C70's 23-slot staging subset: team-0 records 0..10, team-1
 * records 0..10, then the ball at slot 22. */
static void match_run_camera_action_table(
    const struct fifa96_match_run *mr,
    int32_t table[FIFA96_CAMERA_ACTION_SLOTS][3]) {
  for (int i = 0; i < 11; i++) {
    table[i][0] = mr->entities.team[0].records[i].pos_x;
    table[i][1] = mr->entities.team[0].records[i].pos_y;
    table[i][2] = mr->entities.team[0].records[i].pos_z;
    table[11 + i][0] = mr->entities.team[1].records[i].pos_x;
    table[11 + i][1] = mr->entities.team[1].records[i].pos_y;
    table[11 + i][2] = mr->entities.team[1].records[i].pos_z;
  }
  table[22][0] = mr->entities.ball.x;
  table[22][1] = mr->entities.ball.y;
  table[22][2] = mr->entities.ball.z;
}

/* Run the selected native class and publish its record cells to the render
 * eye/orientation. Returns 1 on success. */
static int match_run_native_view_step(struct fifa96_match_run *mr,
                                      int32_t class_of, int32_t tick,
                                      int settle) {
  struct fifa96_match_run_render *r = &mr->render;
  fifa96_camera_handler_state *st;
  fifa96_camera_handler_block *block;
  fifa96_camera_handler_out out;
  int steps = settle ? MATCH_RUN_NATIVE_VIEW_SETTLE : 1;
  if (class_of < 0 || class_of > 2) return 0;
  if (r->native_view_valid[class_of] == 0u)
    match_run_native_view_init(r, class_of);
  st = &r->native_view_cam[class_of];
  block = &r->native_view_block[class_of];
  st->view_variant = r->native_view_variant;
  /* FUN_0004D908's live call is the proven (0,0) argument form. */
  st->pan_axis_a = 0;
  st->pan_axis_b = 0;
  match_run_native_view_apply_variant(r, class_of);
  /* FUN_000365B0's [0x105FC4] source switch; FUN_00036544 copies the
   * selected triple into +0x34/+0x3C and clamps x to +-0x720, z to
   * +-0xB10. Source 3/4 needs the 23-slot table. */
  {
    const int32_t focus[3] = {r->render_focus_x, r->render_focus_y,
                              r->render_focus_z};
    int32_t table[FIFA96_CAMERA_ACTION_SLOTS][3];
    const int32_t *target;
    if (r->camera_target.source == 3 || r->camera_target.source == 4)
      match_run_camera_action_table(mr, table);
    target = fifa96_camera_target_select(
        r->camera_target.source, r->camera_target.force_focus,
        r->camera_target.replay_active, -1, focus, r->camera_target.pan,
        r->camera_target.record, r->camera_target.fallback,
        (r->camera_target.source == 3 || r->camera_target.source == 4)
            ? table
            : NULL,
        r->camera_target.index);
    if (target == NULL) target = focus;
    st->track_x = match_run_native_target_clamp(target[0], -0x720, 0x720);
    st->track_y = target[1];
    st->track_z = match_run_native_target_clamp(target[2], -0xB10, 0xB10);
  }
  if (settle) {
    st->hist_x = st->track_x;
    st->hist_y = st->track_y;
    st->hist_z = st->track_z;
  }
  memset(&out, 0, sizeof out);
  for (int i = 0; i < steps; i++) {
    if (match_run_native_view_handler(class_of, st, block, tick, &out) !=
        FIFA96_OK)
      return 0;
  }
  r->render_eye_x = st->pos_x;
  r->render_eye_y = st->pos_y;
  r->render_eye_z = st->pos_z;
  r->render_eye_valid = 1u;
  r->yaw = st->yaw;
  r->pitch = st->pitch;
  r->view_ratio = st->ratio;
  return 1;
}

static void match_run_live_camera_follow(struct fifa96_match_run *mr) {
  struct fifa96_camera *cam = &mr->render.camera;
  struct fifa96_match_run_render *r = &mr->render;
  /* P6 task 2 / P6-L1 (FU-344): the native follow lead is the ball velocity
   * times 8, clamped to the ±0x120 band. Compute it once for the FU-71 focus
   * and the separate presentation focus. */
  int32_t lead_x = (int32_t)mr->entities.ball.vel_x * 8;
  int32_t lead_z = (int32_t)mr->entities.ball.vel_z * 8;
  if (lead_x > 0x120) lead_x = 0x120;
  if (lead_x < -0x120) lead_x = -0x120;
  if (lead_z > 0x120) lead_z = 0x120;
  if (lead_z < -0x120) lead_z = -0x120;
  int32_t focus_x = mr->entities.ball.x + lead_x;
  int32_t focus_z = mr->entities.ball.z + lead_z;
  /* FU-351-L1: the native focus triple staged at 0x155AA4 is the FU-71 place
   * `0x15774C/50/54` (the override `[0x157A6F]` is itself seeded from
   * `0x15774C`; FU-154 §2.1), clamped ±6000 (x/z) and 0..16000 (y). The
   * composer runs after the pan update and before the view driver, so read
   * the current place here, not this frame's new ball follow. This is the
   * class-3 target; the Phase-85 controlled-record blend is retired. */
  fifa96_camera_focus native_focus;
  int32_t src_x = mr->camera_override_157a6f ? mr->camera_override_triple[0]
                                             : cam->pos_x;
  int32_t src_y = mr->camera_override_157a6f ? mr->camera_override_triple[1]
                                             : cam->pos_y;
  int32_t src_z = mr->camera_override_157a6f ? mr->camera_override_triple[2]
                                             : cam->pos_z;
  if (fifa96_camera_focus_clamp(src_x, src_y, src_z, &native_focus) !=
      FIFA96_OK) {
    native_focus.x = src_x;
    native_focus.y = src_y;
    native_focus.z = src_z;
  }
  r->view_focus_x = focus_x;
  r->view_focus_y = 0x50;
  r->view_focus_z = focus_z;
  r->view_focus_valid = 1u;
  r->render_focus_x = native_focus.x;
  r->render_focus_y = native_focus.y;
  r->render_focus_z = native_focus.z;
  r->render_focus_valid = 1u;
  /* FU351-L2c-3: select the native view from the live phase/option/screen
   * leg. Slots 0/1/2 are classes 0/1/2 and run the completed steady/sidetrack/
   * staged bodies; slot 5 stays the class-3 action fallback (FUN_0004DB38).
   * A slot/variant change takes FUN_0004D04C's 20-step settle. Non-live tapes
   * keep the old camera path byte-exact. */
  if (mr->live_start) {
    fifa96_camera_view view;
    int native_ok = 0;
    int changed = 0;
    int32_t tick = (int32_t)mr->state.frame_delta * MATCH_RUN_CLASS3_TICK_SCALE;
    if (match_run_view_selector_allowed(r) &&
        fifa96_camera_view_select(mr->state.phase, mr->difficulty_14c2f6,
                                  mr->screen_leg, &view) == FIFA96_OK) {
      r->native_view_preset = view.preset;
      if (view.slot != r->native_view_slot ||
          view.variant != r->native_view_variant)
        changed = 1;
      r->native_view_slot = view.slot;
      r->native_view_variant = view.variant;
      r->native_view_class = view.class_of;
    }
    if (r->native_view_class >= 0 && r->native_view_class <= 2)
      native_ok = match_run_native_view_step(mr, r->native_view_class, tick,
                                             changed);
    if (!native_ok) {
      if (r->render_eye_valid == 0u) match_run_render_eye_update(mr);
      fifa96_camera_handler_state *acs = &r->action_cam;
      fifa96_camera_handler_out ac_out = {0};
      acs->track_x = r->render_focus_x;
      acs->track_y = r->render_focus_y;
      acs->track_z = r->render_focus_z;
      acs->pos_x = r->render_eye_x;
      acs->pos_y = r->render_eye_y;
      acs->pos_z = r->render_eye_z;
      acs->yaw = r->yaw;
      acs->pitch = r->pitch;
      if (fifa96_camera_behavior_action(acs, 1, MATCH_RUN_CLASS3_DISTANCE, tick,
                                        &ac_out) == FIFA96_OK) {
        r->render_eye_x = acs->pos_x;
        r->render_eye_y = acs->pos_y;
        r->render_eye_z = acs->pos_z;
        r->render_eye_valid = 1u;
        r->yaw = acs->yaw;
        r->pitch = acs->pitch;
      }
    }
  }
  if ((cam->event_param != 0 || cam->speed != 0) &&
      mr->live_kick_active == 0u && mr->live_save_hold == 0u)
    return;   /* native pan active: keep the FU-71 focus frozen */
  int32_t tracked = mr->entities.controlled;
  if (tracked == FIFA96_MATCH_ENTITY_NONE) tracked = mr->slot.entity;
  if (tracked != FIFA96_MATCH_ENTITY_NONE && cam->tracked != tracked)
    (void)fifa96_camera_set_tracked(cam, tracked);
  /* P6 task 2 / P6-L1 (FU-344): the live FU-71 focus keeps the native height
   * 0x50 and the ball lead. The render focus above is independent. */
  cam->pos_x = r->view_focus_x;
  cam->pos_y = 0x50;
  cam->pos_z = r->view_focus_z;
}

/* FUN_00036C70 stages the record/ball facing word in the high half of the
 * native +0x7B dword. Keep the 16.16 convention in one place so the render
 * fold (`heading >> 16`) sees the same sign-extended 10-bit word for both. */
static int32_t match_run_stage_heading(int16_t word) {
  return (int32_t)((uint32_t)(uint16_t)word << 16);
}

/* Phase-84: the native 0xA2A10 view/position angle addend. The native
 * function is a 16-bit atan2 over the camera-relative (x, z) pair with
 * +z = 0x0000, +x = 0x4000, -z = 0x8000 and -x = 0xC000 (capstone decode of
 * 0xA2A10 and the 0xA29F0 quadrant table). `fifa96_entity_angle` is the same
 * atan in the native 10-bit unit, so scaling by 0x40 reproduces the 16-bit
 * result. */
int32_t fifa96_match_run_view_addend(int32_t rel_x, int32_t rel_z) {
  int32_t angle10 = 0;
  if (fifa96_entity_angle(rel_x, rel_z, &angle10) != FIFA96_OK) return 0;
  return (int32_t)(uint16_t)((uint16_t)angle10 * 0x40u);
}

/* FU-85 §4/FUN_00036C70 (first-hand disassemble_function 0x36C70) stages the
 * 23 render slots per frame body: team 0 records 0..10 (base 0x1588A4) then
 * team 1 records 0..10 (0x1590D9), each from rec+0x59/5D/61 with hidden set by
 * rec+0x9A (staged y = -10000, 0x36D61), then slot 22 from the ball record
 * 0x15880C with the 0x15885A heading. The native record angle is
 * `((0x400 - (rec[+0x7B]>>16)) & 0x3FF) << 6`, i.e. the facing word +0x7D.
 * The pool now models that word (`face7d`, written by the live locomotion
 * mover), so the record slot stages it in the same 16.16 heading form as the
 * ball; `fifa96_render_slot_stage` performs the native fold.
 *
 * OL-80 (links FU-141 OL-42): the same staging passes `anim_id` from
 * `byte[[rec+0x28]]` (0x36D44) and `frame` from `byte[rec+0x3D]` (0x36D4F).
 * The pool now models both (the FU-84 selector's row id and the frame index),
 * so the staging seeds the slot from the live pool record, runs the derived
 * FU-84 advance and writes the advanced index back into the record for the
 * next frame's dispatch (the arm bodies' `fifa96_arm_anim_select` reads).
 *
 * FU-84 `FUN_0008E008` (first-hand FU-84 §5): each slot's row id (`stage.
 * anim_id`) selects the 0x10EF00 row; the row's +8 byte is the FU-85 resolver
 * sprite bank, and the row's last frame plus the slot's frame record duration
 * (the staged 0x10E200-family records) advance the accumulator once with
 * `delta<<4` (the whole-frame delta `[0x57A64]`, FU-62 §4.3). The non-terminal
 * next_id chain is ported; terminal steering/duration is live-gated below;
 * the height-gated row-0x5A special case remains open. */
/* One native composer animation pass for a staged slot (FUN_0008E008).
 * `delta` is the whole-frame delta of the granted 30 Hz frame; the scene
 * composer runs once per granted frame from the frame body. */
static void match_run_animate_entity(struct fifa96_match_run *mr, uint32_t i,
                                     uint16_t delta) {
  struct fifa96_match_run_render *r = &mr->render;
  struct fifa96_match_run_entity *e = &r->entities[i];
  struct fifa96_match_entity *rec = NULL;
  if (i < FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)
    rec = &mr->entities.team[i / FIFA96_MATCH_ENTITY_RECORDS]
               .records[i % FIFA96_MATCH_ENTITY_RECORDS];
  if (e->stage.anim_id >= 0x6F) return;   /* the resolver's signed id gate */
  fifa96_anim_row row;
  if (fifa96_animation_row_lookup(match_run_anim_rows, FIFA96_ANIM_ROW_COUNT,
                                  (int32_t)e->stage.anim_id, &row) != FIFA96_OK)
    return;
  e->bank_index = row.sprite_bank;
  /* FU-161 §5.2 (T3): the aux-0x2B frame freeze — the arm zeroed the frame
   * timer (`word[+0x32] = 0`, 0x6EA03) for this frame while the arc is
   * airborne, so the frame holds (native: the timer never expires; the
   * engine skips the advance). Consumption is take-once per staging. */
  if (rec != NULL && rec->frame_freeze) {
    rec->frame_freeze = 0;
    e->anim_timer = 0;
    return;
  }
  fifa96_anim_frame record;
  if (fifa96_animation_frame(r->frames, (int32_t)e->stage.frame, row.last_frame,
                             &record) != FIFA96_OK)
    return;
  /* FU172 §5 (Tier 2): terminal rows take the steering table's duration
   * (the walk cadence); non-terminal rows keep the frame record duration. */
  uint16_t duration = record.duration;
  /* Live-only presentation gate: the tape/headless paths keep the prior
   * terminal no-op so M1/M2 stay byte-identical. The live match uses the
   * native walk-family selection and cadence. */
  int terminal = mr->live_start && rec != NULL && (row.flags & 0x01u) != 0;
  if (terminal) {
    /* FU172 §5: terminal duration is table[speed*2 + 1] — no lean offset. */
    int index = match_run_steering_index(rec, 0);
    duration = match_run_steering_table(rec)[index].duration;
  }
  fifa96_anim_advance state;
  state.timer = e->anim_timer;
  state.delta = delta;
  state.frame_index = e->stage.frame;
  state.turn = e->anim_turn;
  uint8_t prev_frame = (uint8_t)e->stage.frame;
  fifa96_anim_advance_out out;
  if (fifa96_animation_advance(&state, duration, row.last_frame, &out) !=
      FIFA96_OK)
    return;
  e->anim_timer = state.timer;
  e->stage.frame = out.frame_index;
  if (rec) {
    rec->frame = (uint8_t)out.frame_index;   /* OL-80 +0x3D */
    /* FU-159 §5.2/§3.1 + FU172 §4 (Tier 2): terminal rows re-select the
     * walk family at every timer expiry; non-terminal rows keep the
     * `prev_frame >= last_frame` next_id chain. The install changes the
     * row, so re-stage the same frame's slot from the record (the native
     * `FUN_00036C70` reads after the machine's install). */
    if (terminal && out.advanced) {
      uint8_t old_id = rec->anim_id;
      /* The native install uses the pre-expiry +0x3D frame plus the lean;
       * do not add the already-applied advance a second time. */
      rec->frame = prev_frame;
      (void)fifa96_match_run_record_anim_successor_turn(rec, e->anim_turn);
      if (rec->anim_id == old_id) rec->frame = (uint8_t)out.frame_index;
      e->stage.anim_id = (int32_t)rec->anim_id;
      if (fifa96_animation_row_lookup(match_run_anim_rows,
                                      FIFA96_ANIM_ROW_COUNT,
                                      (int32_t)rec->anim_id, &row) == FIFA96_OK)
        e->bank_index = row.sprite_bank;
      e->stage.frame = (int32_t)rec->frame;
    } else if (!terminal && out.advanced && prev_frame >= row.last_frame) {
      (void)fifa96_match_run_record_anim_successor(rec);
      e->stage.anim_id = (int32_t)rec->anim_id;
      if (fifa96_animation_row_lookup(match_run_anim_rows,
                                      FIFA96_ANIM_ROW_COUNT,
                                      (int32_t)rec->anim_id, &row) == FIFA96_OK)
        e->bank_index = row.sprite_bank;
      e->stage.frame = (int32_t)rec->frame;
    }
  }
}

/* FU-85 §4/FUN_00036C70's position half: stage the 23 render slots from the
 * pool (positions, heading, staged anim id/frame, hidden) and advance the ball
 * roll. Writes render cells only. The C3 frozen frame calls this without the
 * animator: `match_run_animate_entity` writes the sim-visible record fields
 * (`+0x28` anim id / `+0x3D` frame) the entity machine reads. */
static void match_run_scene_stage_positions(struct fifa96_match_run *mr) {
  struct fifa96_match_run_render *r = &mr->render;
  uint32_t slot = 0;
  for (uint32_t team = 0; team < FIFA96_MATCH_ENTITY_TEAMS; team++) {
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++, slot++) {
      struct fifa96_match_entity *e = &mr->entities.team[team].records[i];
      struct fifa96_match_run_entity *re = &r->entities[slot];
      re->stage.pos.x = e->pos_x;
      re->stage.pos.y = e->pos_y;
      re->stage.pos.z = e->pos_z;
      /* FUN_00036C70: the record angle is the +0x7D face word in the high
       * half of the native +0x7B dword; the render slot folds it. */
      re->stage.heading = match_run_stage_heading(e->face7d);
      re->stage.anim_id = (int32_t)e->anim_id;   /* 0x36D44 byte[[rec+0x28]] */
      re->stage.frame = (int32_t)e->frame;       /* 0x36D4F byte[rec+0x3D] */
      re->stage.hidden = e->skip_9a ? 1 : 0;
    }
  }
  struct fifa96_match_run_entity *ball = &r->entities[slot];
  ball->stage.pos.x = mr->entities.ball.x;
  ball->stage.pos.y = mr->entities.ball.y;
  ball->stage.pos.z = mr->entities.ball.z;
  /* M2 phase-11 T2: the ball's appearance row (the reset's FUN_0006E724 row 3
   * install, 0x887D5) selects the FU-84 row the FU-85 resolver draws; the
   * earlier zeroed stand-in resolved row 0. */
  ball->stage.anim_id = (int32_t)mr->entities.ball.appear_row;
  ball->stage.heading = match_run_stage_heading(mr->entities.ball.heading);
  ball->stage.hidden = 0;
  r->entity_count = slot + 1;

  /* FU-158 §4.3/§8.2 (phase-13 T2): the composer's ball roll byte. The native
   * arm (FUN_00036C70 0x36F4F..0x36F83) advances `[0x155C4E]`'s low nibble +1
   * per composer frame while the camera rate words 0x1577C0/0x1577C2 are
   * non-zero; the engine's stand-in for those words (the T1/R2-L4 live-camera
   * leg) is the ball record's motion (FU-158 L158.2). The +1 is the exact
   * native byte op: `AL = (byte & 0xF) + 1` OR'd into the high nibble, so the
   * carry lands in bit 4. The presenter's out-of-range sanitizer (0x5739D: a
   * frame >= the bank count draws frame 0 and clears the low nibble) is
   * applied here — the drawn-frame-equivalent site; natively it runs at draw
   * time (L158.2). This composer runs once per granted 30 Hz frame from the
   * frame body, so the ball roll and sprite timers share the native cadence. */
  if (mr->entities.ball.vel_x != 0 || mr->entities.ball.vel_z != 0) {
    uint8_t lo = (uint8_t)((r->ball_roll & 0x0Fu) + 1u);
    r->ball_roll = (uint8_t)((r->ball_roll & 0xF0u) | lo);
  }
  if (r->ball_bank != FIFA96_MATCH_RUN_BALL_BANK_NONE &&
      r->ball_bank < r->bank_count &&
      (uint32_t)(r->ball_roll & 0x0Fu) >= r->banks[r->ball_bank].count)
    r->ball_roll &= 0xF0u;
}

/* FU-84 `FUN_0008E008`'s animator half: advance every staged slot by the
 * granted frame's whole-frame delta. This is the pass that writes the
 * sim-visible record fields (the successor install), so it stays on the live
 * path only. */
static void match_run_scene_animate(struct fifa96_match_run *mr) {
  struct fifa96_match_run_render *r = &mr->render;
  if (!r->frames) return;
  for (uint32_t i = 0; i < r->entity_count; i++)
    match_run_animate_entity(mr, i, (uint16_t)mr->state.frame_delta);
}

static void match_run_scene_stage(struct fifa96_match_run *mr) {
  match_run_scene_stage_positions(mr);
  match_run_scene_animate(mr);
}

/* Stable slot identity: the lifecycle cancels by function pointer, so every
 * run shares this one trampoline and the engine bridge keeps a single match
 * callback in the clock's tick table. The trampoline is the sole driver of the
 * frame body: one call per PIT tick, so the pace stays at 100 Hz even when an
 * engine step spans 0..N ticks (FU-60 drives the pace from the INT-8 ISR). */
static void fifa96_match_run_tick(void *user) {
  struct fifa96_match_run *mr = user;
  if (!mr) return;
  mr->ticks++;
  (void)fifa96_match_run_frame(mr);
}

static int fifa96_match_run_register(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr || !mr->engine) return -FIFA96_ERR_INVALID;
  int slot = fifa96_tick_register(&mr->engine->clock.ticks, fifa96_match_run_tick,
                                  mr, 1u); /* registered at 100 Hz (FU-64 §2.2) */
  return slot < 0 ? slot : 0;
}

static int fifa96_match_run_cancel(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr || !mr->engine) return -FIFA96_ERR_INVALID;
  return fifa96_tick_cancel(&mr->engine->clock.ticks, fifa96_match_run_tick);
}

static void match_run_reset_screen_state(struct fifa96_match_run *mr);

/* FU313/FU309-L5 (phase-30 T1): the default-off camera-cell trace. Closing is
 * idempotent; `fifa96_match_run_begin` opens the sink only when the environment
 * names a path, so every other run pays one NULL test per granted frame. */
static void match_run_camera_trace_close(struct fifa96_match_run *mr) {
  if (!mr || !mr->cam_trace) return;
  fclose(mr->cam_trace);
  mr->cam_trace = NULL;
}

/* FU313 §4.1: opened "w" at the end of `fifa96_match_run_begin`, after the
 * lifecycle callback is registered and the run is live, so a failed begin
 * creates no file. Unset/empty is off; an open failure is one stderr line and
 * off (never stdout, never the goldens). The frame index is run-relative. */
static void match_run_camera_trace_open(struct fifa96_match_run *mr) {
  const char *path;
  FILE *f;
  mr->cam_trace = NULL;
  mr->cam_trace_frame = 0;
  path = getenv("FIFA96_CAMERA_TRACE");
  if (!path || path[0] == '\0') return;
  f = fopen(path, "w");
  if (!f) {
    fprintf(stderr, "fifa96: camera trace: cannot open %s\n", path);
    return;
  }
  fprintf(f, "# fifa96-camera-trace v1 frame clock phase pos vel spd fspd ev cur "
             "trk pan tim ramp rate follow tick\n");
  fflush(f);
  mr->cam_trace = f;
}

static int fifa96_match_run_teardown(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr) return -FIFA96_ERR_INVALID;
  match_run_camera_trace_close(mr);
  /* The foundation owns no match assets yet; clear the per-match counters,
   * the match clock/period block and the per-side goal words. */
  mr->ticks = 0;
  mr->steps = 0;
  fifa96_match_state_init(&mr->state);
  mr->score[0] = 0;
  mr->score[1] = 0;
  /* C3-OL2: the carried writer state defaults (native producers OL-87). */
  mr->score_last_side = -1;
  mr->score_tracked_side = -1;
  mr->score_max_diff = 0;
  mr->score_last_event = 0;
  /* T4/OL-89: the FUN_0009252C gate cells at the image defaults and no
   * dispatch yet. */
  mr->score_sound_device = 0;
  mr->score_sound_midi = 0;
  mr->score_display_event = 0;
  mr->global_5882a = 0;                /* drop the kickoff gate with the match */
  mr->present_side = 0;                /* FU-181 L5: drop the [0x1074A4] bind */
  mr->clock_saved_157a60 = 0;          /* FU-181 L5: drop the saved Q8 cell */
  mr->deepest_keeper_157aa7 = 0;
  mr->flag_157aab = 0;
  mr->camera_height_157a6d = 0;
  mr->flag_157ab0 = 0;
  mr->camera_override_157a6f = 0;
  mr->camera_override_triple[0] = 0;
  mr->camera_override_triple[1] = 0;
  mr->camera_override_triple[2] = 0;
  /* FU-145 S2: the restart-body clear (`0x84F90`) drops the goal-arm state
   * with the match; the session gate returns to 0 with the session. */
  mr->goal_armed = 0;
  mr->goal_counter_15781c = 0;
  mr->goal_zone = 0;
  mr->goal_snap_x = 0;
  mr->goal_snap_y = 0;
  mr->goal_snap_z = 0;
  mr->live_out_kind = 0;
  mr->live_out_pending = 0;
  mr->live_out_situation = 0;
  mr->live_out_side = 0;
  mr->live_out_frames = 0;
  mr->live_shot_pending = 0;
  mr->camera_arm_announce = 0;         /* FU-71 leg 9.6: no 0x974DC request */
  mr->situation_id = 0;
  mr->situation_pending = 0;
  mr->session_gate_14c32a = 0;
  mr->ball_row_flag_158882 = 0;        /* the native reset clear 0x73D3A */
  match_run_ball_settle_clear(mr);     /* Phase 70: no staged settle */
  match_run_reset_screen_state(mr);    /* FU-146 S3: no installed handler */
  (void)fifa96_match_entities_release(&mr->entities);
  return 0;
}

static int fifa96_match_run_post_exit(void *ctx) {
  struct fifa96_match_run *mr = ctx;
  if (!mr || !mr->engine) return -FIFA96_ERR_INVALID;
  mr->engine->mode = FIFA96_ENGINE_MODE_FRONTEND;
  return 0;
}

static void fifa96_match_run_install_defaults(struct fifa96_match_run *mr) {
  mr->backend.register_callback = fifa96_match_run_register;
  mr->backend.cancel_callback = fifa96_match_run_cancel;
  mr->backend.teardown = fifa96_match_run_teardown;
  mr->backend.post_exit = fifa96_match_run_post_exit;
  mr->backend.ctx = mr;
}

/* Fresh-match input state: zero the FU-61 edge/held model and bind the
 * controlled player's FU-70 slot (player 0, raw map select 1 per FU-70 §1.4,
 * ordinal 0, side 0). */
static void fifa96_match_run_reset_input(struct fifa96_match_run *mr) {
  fifa96_input_init(&mr->input);
  memset(mr->input_state, 0, sizeof mr->input_state);
  memset(&mr->slot, 0, sizeof mr->slot);
  (void)fifa96_control_slot_init(&mr->slot, 0, 1, 0, 0);
}

/* Fresh-match presentation state (Task 15): zero the FU-71 camera, FU-92
 * window, FU-69 display block and FU-84/85 scene/sprite assets; rendering is
 * opt-in so the Task 12-14 engine fixtures keep their surface untouched. The
 * indexed remap starts as the identity translation with index 0 as the derived
 * colour key (FU-85 §5: the 0x14720 buffer is the installed palette's
 * translation buffer, `FUN_000CE980` copying its source in and 0xCEABC
 * treating 0xFF as transparent; without the per-sprite palette install
 * `FUN_00048DC0` the engine's identity translation is the stand-in -- open
 * leg). FU-84: the selector seeds the driver turn +1 (`FUN_0006E598` 0x6E6D8
 * `[rec+0x46] = 1`). */
static void fifa96_match_run_reset_render(struct fifa96_match_run *mr) {
  struct fifa96_match_run_render *r = &mr->render;
  memset(r, 0, sizeof *r);
  (void)fifa96_camera_init(&r->camera, 0, 0, 0);
  /* Slice-B Task 9: the 0x72AC4 cells are independent of FUN_000700F4. This
   * is the whole-render BSS reset, not a camera cut, so the native NULL caches
   * are initialized here and subsequently persist across camera_init calls. */
  r->camera.save.controlled_157a83 = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.tracked_157a87 = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.target_157a8b = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.rec_1575da = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.team_1575d2 = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.ball_record_1575d6 = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.controlled_1575de = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.lane_best_a_1575e9 = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.lane_best_b_1575ed = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.actor_15b33c = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.opponent_target = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.opponent_157726 = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.target_15771e = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.stat_actor_157722 = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  r->camera.save.controlled_15773b = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  /* FUN_000927E0 0x9280A/0x92815: the native reset delay is 100, not zero.
   * history/lane/class/actor_known stay zero: unported producers are UNKNOWN,
   * never known-empty. */
  r->camera.save.delay_15771a = 100;
  (void)fifa96_window_init(&r->window, 0, 0);
  (void)fifa96_match_display_init(&r->display, 0);
  /* FU-158 §8.1 (phase-13 T2): no ball bank until a pitch container stages
   * one (the FU-84 resolve is the fallback). */
  r->ball_bank = FIFA96_MATCH_RUN_BALL_BANK_NONE;
  /* FU-154 §5.1 (M2 phase-12 T2): seed the view orientation from the native
   * camera-0 statics (the projection matrix consumes r->yaw/r->pitch at the
   * render call); the prior zero stand-in left the phase-2 ball lead off
   * canvas. */
  r->yaw = FIFA96_MATCH_RUN_VIEW_YAW;
  r->pitch = FIFA96_MATCH_RUN_VIEW_PITCH;
  r->view_ratio = MATCH_RUN_VIEW_RATIO;
  /* FU351-L2c-3: the image [0x107DD0]/[0x107DD4] start as slot 1 / variant 0,
   * so a first phase-2 phase does not take FUN_0004D04C's view-change arm. */
  r->native_view_preset = 0;
  r->native_view_slot = 1;
  r->native_view_variant = 0;
  r->native_view_class = 1;
  /* FU351-L2f image defaults: [0x105FC4] = 0, [0x108E04] = -1. */
  r->camera_target.source = 0;
  r->camera_target.index = -1;
  r->pitch_art.sky_index = FIFA96_PITCH_SKY_INDEX;
  r->pitch_art.line_index = FIFA96_PITCH_LINE_INDEX;
  for (int i = 0; i < 256; i++) r->remap[i] = (uint8_t)(i == 0 ? 0xFF : i);
  for (int i = 0; i < 256; i++) {
    r->pitch_shade[i] = (uint8_t)i;
    r->shadow_shade[i] = (uint8_t)i;
  }
  /* FU-343 (P1): both per-team translations start as the identity; the live
   * kit stage rewrites team 1's shirt/shorts bands into the 230..245 slots
   * (A7 §4). The non-live draw never selects remap_team. */
  for (int team = 0; team < 2; team++)
    for (int i = 0; i < 256; i++)
      r->remap_team[team][i] = (uint8_t)(i == 0 ? 0xFF : i);
  /* The live ball's own translation: the retail `ball.fsh` paints its outline
   * with palette index 0 and its background with 0xFF. Keep 0 opaque (the
   * span writer still treats a remapped 0xFF as transparent). */
  for (int i = 0; i < 256; i++) r->ball_remap[i] = (uint8_t)i;
  r->team_remap_ready = 0;
  /* FU-343 (P4): the live HUD colour defaults. The fallback body index is the
   * staging-measured near-white (30); the shade table is a monotone halving
   * until a palette stages (the live path always stages one via stage_pitch).
   * `hud_colors_ready` records the palette-derived build. */
  r->hud_ink = FIFA96_MATCH_HUD_INK_DEFAULT;
  for (int i = 0; i < 256; i++) r->hud_shade[i] = (uint8_t)(i >> 1);
  r->hud_shade[0] = 0;
  /* OL-T11-71 errata: the live shaded-text fallback is a linear black->ink
   * index ramp; a staged palette replaces it with the nearest-colour [D] ramp. */
  r->hud_text_shades[0] = 0;
  for (int i = 1; i < 16; i++)
    r->hud_text_shades[i] =
        (uint8_t)(((unsigned)i * FIFA96_MATCH_HUD_INK_DEFAULT + 7u) / 15u);
  r->hud_colors_ready = 0;
  for (int i = 0; i < (int)FIFA96_MATCH_RUN_RENDER_SLOTS; i++) r->entities[i].anim_turn = 1;
}

/* FU-146 S3: the fresh-match goal-consumer state. The native image seeds the
 * `FUN_000CBC4C` probe limbs (`0x112E68..0x112E7C`: `56 0e 2d f2 e9 26 31 88
 * 2f dd 24 c6 9c c4 02 07 7d 3f 35 9e 64 3b df 6f`); `screen_leg` restarts as
 * -1 (no installed handler, the image's `[0x15B6D4]==0`) and `begin` installs
 * the derived leg 0 / mode 0 / side 0 defaults. Runs on init, on the fresh
 * match reset in begin and on teardown. */
static void match_run_reset_screen_state(struct fifa96_match_run *mr) {
  static const uint32_t probe_seed[6] = {
      0xF22D0E56u, 0x883126E9u, 0xC624DD2Fu,
      0x0702C49Cu, 0x9E353F7Du, 0x6FDF3B64u,
  };
  mr->screen_leg = -1;
  mr->screen_mode = 0;
  mr->screen_step = 0;
  mr->screen_timer = 0;
  mr->screen_period_frames = 0;
  mr->screen_install_hint = 0;
  mr->screen_actor_age = 0;
  mr->goal_no_score = 0;
  mr->goal_last_id = 0;
  mr->goal_minute = 0;
  mr->goal_screen_accum = 0;
  mr->goal_log_prev_total = 0;
  mr->goal_total = 0;
  memset(mr->goal_log, 0, sizeof mr->goal_log);
  memcpy(mr->goal_probe_limb, probe_seed, sizeof probe_seed);
  /* M2 phase-9 T3: the tracked-side flags ([0x1590CC]/[0x159901], zeroed by
   * FUN_00078824 0x7882a/0x78830) and the [0x15B684] game-mode default. The
   * range words are rebuilt by begin from the config cells (FUN_0001C9BC). */
  mr->tracked_flag[0] = 0;
  mr->tracked_flag[1] = 0;
  mr->screen_record_mode = 0;
  mr->input_range[0] = 0;
  mr->input_range[1] = 0;
}

/* FU-89 §11 / OL-T11-8 (M2 visible-match Task 1) + FU-148 §3 (S4): the
 * resource-loaded formation seed. `FUN_0004A6BC` resolves the `0x14BFC0`
 * table's `.fmt` slots (0..29, one family of six per formation id) — slot
 * `6*formation_id` is the placement file — and the BSS
 * default for an unselected team is formation id 0 (first-hand: `[0x14C1E4]`
 * is BSS 0 and `FUN_0006D9C4` derives `[team+0x7AE] = 0x11033A + id*0x1D`, a
 * 0x1D-byte roster row whose `byte[0]` is the id). S4 replaces the hard-coded
 * 0 with the run's derived `formation[controlled_side]` and the pinned
 * 0x107370 placement names (id 0 `352ko.fmt`, 1 `442ko.fmt`, 2 `swko.fmt`,
 * 3 `424ko.fmt`, 4 `433ko.fmt`; FU-148 §3 leg: the FUN_00011620 team-record
 * producer). Slot 0's file is a BIGF entry of the match art container
 * `/ART/GAMEART0.PVI` (first-hand directory: entry 0; the engine already
 * stages the same file). This is the engine's **only** match-begin formation
 * consumer; it reads the `.fmt` **placement blob** — distinct from the
 * `0x14C360` descriptor-pointer **order matrix** (FU-201 §3.2/§6), which has no
 * match-begin engine consumer, so the order matrices stay a staged leg
 * (FU201-L4). The seed is a soft failure: no ISO/asset/entry leaves
 * the zero targets the pre-T1 path had. The controlled side is the
 * `[0x157AAC]>>24` mirror (BSS 0). */
#define MATCH_RUN_FORMATION_BANK "/ART/GAMEART0.PVI"

/* FU-144 / OL-T11-6: the native match palette source. `FUN_00048B60`
 * (0x48B6E..0x48B83) reads resource slot 0x32 -- the 0x107370 loader table's
 * slot 50 = 0x101BA4 `"PALsys"`, formatted with `"%s.fsh"` (0x101C90; the
 * `"lfsh"` string starts one byte earlier) -- loads its frame 2
 * (`FUN_000A1920(handle, 2)`) and takes the type-0x22 chunk through
 * `FUN_00047814`. `FUN_00048ED8` then appends the chunk's [0xF0,+0x54) and
 * [0x186,+0x4E) byte ranges over the built palette and `FUN_00048C8C`
 * installs it with the `FUN_000479A0` 8-bit copy (`v << 2`). */
#define MATCH_RUN_PALETTE_BANK "PALsys.fsh"
#define MATCH_RUN_PALETTE_FRAME 2u
#define MATCH_RUN_PALETTE_APPEND_A_OFF 0xF0u
#define MATCH_RUN_PALETTE_APPEND_A_LEN 0x54u
#define MATCH_RUN_PALETTE_APPEND_B_OFF 0x186u
#define MATCH_RUN_PALETTE_APPEND_B_LEN 0x4Eu
#define MATCH_RUN_PALETTE_COUNT 256u

/* FU-158 §5/§8.1 (phase-13 T2): the dedicated ball draw's bank. The native
 * presenter FUN_000572E8 preloads resource slot 0x29 = the 0x107370 name
 * table's entry 41 "ball" (FU-152 §2.5) and draws it through GAMEART0 — the
 * retail entry 37 `ball.fsh` (8 frames, 16x16; first-hand `fifa96_play`
 * `--name ball.fsh`/`--entry 37` this task). Captured by BIGF name here so the
 * render's ball slot sources it at the player-container offset. */
#define MATCH_RUN_BALL_BANK "ball.fsh"
#define MATCH_RUN_NET_BANK "Net.fsh"
#define MATCH_RUN_POST_BANK "Npost.fsh"

/* FU275 L266-7: the slot-5 `.fmt` names from flat 0x107370 (indices 5, 11,
 * 17, 23, 29; the retail bank reads 3588 bytes each). */
static const char *const match_sp_fmt_names[5] = {
    "352sp.fmt", "442sp.fmt", "swsp.fmt", "424sp.fmt", "433sp.fmt",
};

static void match_run_formation_sp_seed_team(struct fifa96_match_run *mr,
                                             uint32_t team, uint8_t id) {
  struct fifa96_match_team *t;
  uint8_t *bank = NULL;
  uint8_t *entry = NULL;
  size_t bank_len = 0;
  size_t entry_len = 0;
  if (!mr || team >= FIFA96_MATCH_ENTITY_TEAMS) return;
  t = &mr->entities.team[team];
  t->sp_table_len = 0;
  t->sp_table_ready = 0;
  if (!mr->engine || !mr->engine->assets || id >= 5u) return;
  if (fifa96_asset_read(mr->engine->assets, MATCH_RUN_FORMATION_BANK, &bank,
                        &bank_len) != FIFA96_OK)
    return;
  if (fifa96_scene_bank_entry_load(bank, bank_len, match_sp_fmt_names[id],
                                   &entry, &entry_len) == FIFA96_OK &&
      entry_len != 0u && entry_len <= sizeof t->sp_table) {
    memcpy(t->sp_table, entry, entry_len);
    t->sp_table_len = (uint32_t)entry_len;
    t->sp_table_ready = 1;
  }
  free(entry);
  fifa96_asset_free(bank);
}

static int match_run_formation_seed_team(struct fifa96_match_run *mr, uint32_t team,
                                         uint8_t id) {
  struct fifa96_scene_formation formation;
  const char *name;
  uint8_t *bytes = NULL;
  size_t len = 0;
  int seeded = 0;
  if (!mr->engine || !mr->engine->assets) return 0;
  name = fifa96_match_formation_fmt_name(id);
  if (!name) return 0;
  if (fifa96_asset_read(mr->engine->assets, MATCH_RUN_FORMATION_BANK, &bytes, &len) !=
      FIFA96_OK)
    return 0;
  if (fifa96_scene_formation_load(bytes, len, name, &formation) == FIFA96_OK)
    seeded = fifa96_match_entities_seed_formation_team(
                 &mr->entities, &formation, team,
                 mr->phase_machine.side_controlled) == FIFA96_OK;
  fifa96_asset_free(bytes);
  match_run_formation_sp_seed_team(mr, team, id);
  return seeded;
}

static int match_run_formation_seed(struct fifa96_match_run *mr) {
  struct fifa96_scene_formation formation;
  const char *name;
  uint8_t *bytes = NULL;
  size_t len = 0;
  int seeded = 0;
  if (!mr->engine || !mr->engine->assets) return 0;
  /* P6: a Team Select handout seeds both teams from the home `.fmt` (the
   * native per-side record bind follows in record_meta_bind); without a
   * handout the pre-P6 controlled-side choice is unchanged. */
  uint8_t id = mr->matchup_active ? mr->formation[0]
                                  : mr->formation[mr->phase_machine.side_controlled & 1u];
  name = fifa96_match_formation_fmt_name(id);
  if (!name) return 0;
  if (fifa96_asset_read(mr->engine->assets, MATCH_RUN_FORMATION_BANK, &bytes, &len) !=
      FIFA96_OK)
    return 0;
  if (fifa96_scene_formation_load(bytes, len, name, &formation) == FIFA96_OK)
    seeded = fifa96_match_entities_seed_formation(&mr->entities, &formation,
                                                  mr->phase_machine.side_controlled) ==
             FIFA96_OK;
  fifa96_asset_free(bytes);
  match_run_formation_sp_seed_team(mr, 0u, id);
  match_run_formation_sp_seed_team(mr, 1u, id);
  return seeded;
}

/* M2 interactive Task 1 / FU-70 §1.3/§1.4: the derived match-setup slot bind
 * (`FUN_00078824` -> `FUN_000785E0`). The engine models the single human
 * controller (player 0, the `0x4C1DC` map select 1); the native four `0x4C1E0`
 * mode rows are unported, so the derived default is mode 0 = the controlled
 * side (`0x4C1E0[0] == 0` -> `[0x57ABE]`, the `[0x57AAC]>>24` side the engine
 * carries as `side_controlled`). The bind seeds the donor record's `+0x20` and
 * `team+0x828` (slot_pool); the kickoff state-1 arm's `FUN_0007876C` merge
 * then moves the slot onto the taker. */
static void match_run_slot_bind(struct fifa96_match_run *mr) {
  uint32_t side = (uint32_t)(mr->phase_machine.side_controlled & 1);
  struct fifa96_match_team *team = &mr->entities.team[side];
  int32_t id = fifa96_match_entities_bind_slot(&mr->entities, side,
                                               (int16_t)mr->render.camera.pos_x,
                                               (int16_t)mr->render.camera.pos_z);
  if (id == FIFA96_MATCH_ENTITY_NONE) return;
  mr->slot.entity = id;
  mr->slot.player = 0;
  mr->slot.map_select = 1;                /* 0x4C1DC default (FU-70 §1.4) */
  mr->slot.ordinal = (uint8_t)(team->slot_pool - 1u);  /* native +0x1D */
  mr->slot.active = (int8_t)team->side;   /* native +0x22 */
}

/* P7 / FU-345: the derived live-control producer.
 *
 * The natural kickoff reaches the controllable state through the phase-1 wait:
 * row 01 stage 0 publishes `[0x157A83]` (`entities.controlled`), row 02's
 * `0x7E041` merge request moves the FU-70 slot to the second kickoff pick, and
 * row 04's coda installs the carrier state 5 (`0x7F133`) through the real
 * `FUN_0007D9A4` installer. The live bootstrap (`fifa96_match_run_begin`'s
 * `live_start` block) deliberately skips that wait and writes phase 2, so the
 * human slot is left on the setup-bind donor while `controlled` stays NONE and
 * the taker never reaches code 5; the kick row (released code-1 input row ->
 * row 07) then has no carrier to fire. This helper seeds the exact state the
 * natural route reaches with the native helpers: the team-target select
 * (`FUN_0008D8EC`), the `FUN_0007876C` slot merge, the `FUN_0008C3BE` actor
 * lift and the `FUN_0007D9A4` code-5 install.
 *
 * This is a labelled [D] stand-in for the unported FU-295 live caller chain
 * (the native setup bind/merge and the row-04/05 possession claim caller
 * order); it is live-only and never writes the score or the goal cells. Returns
 * 1 when the record is a valid controlled-side outfield slot holder, 0 when it
 * is not (keeper, opponent, skipped or no slot available). */
/* Phase 77: the human's team for live control. It is captured at live begin
 * and survives the native side_controlled restart flip after a human foul. */
static uint8_t match_run_live_human_side(const struct fifa96_match_run *mr) {
  if (!mr) return 0;
  return mr->live_start ? (uint8_t)(mr->live_human_side & 1u)
                        : (uint8_t)(mr->phase_machine.side_controlled & 1u);
}

static int match_run_live_control_bind(struct fifa96_match_run *mr, int32_t id,
                                       int seed_carrier) {
  uint8_t side;
  uint32_t t;
  uint32_t idx;
  struct fifa96_match_team *team;
  struct fifa96_match_entity *rec;
  int switched = 0;
  if (!mr || !mr->live_start) return 0;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  side = match_run_live_human_side(mr);
  t = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  idx = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  team = &mr->entities.team[t];
  if (team->side != side) return 0;            /* never take an opponent record */
  if (idx == 0u) return 0;                     /* the keeper is never selected */
  rec = &team->records[idx];
  if (rec->skip_98 != 0 || rec->skip_9a != 0) return 0;
  if (rec->has_slot == 0) {
    if (team->slot_pool == 0) {
      /* No slot exists for the side: the setup bind's `FUN_000785E0` is the
       * native producer. Bind a free record, then merge the slot onto the
       * wanted one through the same native helper. */
      if (fifa96_match_entities_bind_slot(&mr->entities, t,
                                          (int16_t)rec->pos_x,
                                          (int16_t)rec->pos_z) ==
          FIFA96_MATCH_ENTITY_NONE)
        return 0;
      mr->slot.ordinal = (uint8_t)(team->slot_pool - 1u);
    }
    if (rec->has_slot == 0 &&
        fifa96_match_entities_merge_slot(&mr->entities, t, idx) == 1) {
      int32_t merged = fifa96_match_entities_take_slot_merge(&mr->entities);
      if (merged >= 0) mr->slot.entity = merged;
      switched = 1;
    }
  }
  if (rec->has_slot == 0) return 0;
  /* `FUN_0008C3BE`: publish the team target as the controlled actor. */
  team->target = id;
  if (fifa96_match_entities_actor_lift(&mr->entities, t, side) != FIFA96_OK ||
      mr->entities.controlled != id)
    return 0;
  if (mr->slot.entity != id || switched) {
    mr->slot.entity = id;
    mr->slot.player = 0;
    mr->slot.map_select = 1;                  /* 0x4C1DC default (FU-70 §1.4) */
    mr->slot.ordinal = (uint8_t)(team->slot_pool - 1u);
    mr->slot.active = (int8_t)team->side;
    if (switched) (void)fifa96_control_slot_merge_reset(&mr->slot); /* 0x786A0 */
  }
  (void)fifa96_camera_set_tracked(&mr->render.camera, id);
  if (seed_carrier != 0) {
    /* The row-04 coda reachable state: `FUN_0007D9A4(rec, 5)` sets the carrier
     * bit in the installer. The ball block is reset to its kickoff posture
     * (`FUN_0007A490` clear) and the possession identity is the taker. */
    (void)fifa96_match_entities_install(rec, (uint8_t)mr->state.phase, 5u, 0);
    mr->entities.ball.carrier = id;
    (void)fifa96_ball_pair_clear(&mr->entities.ball.pair);
  }
  return 1;
}

/* P7 / FU-345: the pool team index carrying `side`, or -1 when neither team
 * block matches. The live control follow, the outfield shot block and the
 * keeper save all locate a side through this scan. */
static int32_t match_run_live_team_for_side(const struct fifa96_match_run *mr,
                                            uint8_t side) {
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    if (mr->entities.team[t].side == side) return (int32_t)t;
  return -1;
}

/* P7 / FU-345: the controlled team index (the side byte the phase machine
 * carries), or -1 when neither team block matches. */
static int32_t match_run_live_control_team(const struct fifa96_match_run *mr) {
  return match_run_live_team_for_side(mr, match_run_live_human_side(mr));
}

/* P11: the native kickoff cutscene places the taker on the ball; the live
 * bootstrap skips that cutscene and keeps the `.fmt` formation, so the
 * nearest record is 0x199 from the ball and the opening KICK has no carrier.
 * Put the seeded taker on the ball (a derived [D] placement for the unported
 * cutscene) and face it at the opponent goal. Live-only; the clock itself is
 * phase-class driven and is not gated on this touch. */
static void match_run_live_control_place_taker(struct fifa96_match_run *mr,
                                               int32_t taker) {
  uint32_t team;
  uint32_t idx;
  struct fifa96_match_entity *e;
  if (!mr || !mr->live_start || taker < 0 ||
      taker >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return;
  team = (uint32_t)taker / FIFA96_MATCH_ENTITY_RECORDS;
  idx = (uint32_t)taker % FIFA96_MATCH_ENTITY_RECORDS;
  if (idx == 0u) return;                      /* keeper: never the taker */
  e = &mr->entities.team[team].records[idx];
  e->pos_x = mr->entities.ball.x;
  e->pos_y = mr->entities.ball.y;
  e->pos_z = mr->entities.ball.z;
  e->target_x = e->pos_x;
  e->target_y = e->pos_y;
  e->target_z = e->pos_z;
  e->dir_x = 0;
  e->dir_z = mr->entities.team[team].side == 0u ? 1 : -1;
}

/* P7 / FU-345: the live kickoff seed. Called once at the end of the live
 * begin, after the kickoff placement committed the formation: select the
 * controlled side's nearest outfield record to the ball (the native
 * `FUN_0008D8EC` select skips record 0 / the keeper), merge the FU-70 slot onto
 * it, publish it as the actor, install the carrier state and (when the
 * resource `.fmt` seed moved the pool) put it on the ball (the skipped kickoff
 * cutscene's placement). */
static void match_run_live_control_seed(struct fifa96_match_run *mr) {
  int32_t t;
  int32_t taker = FIFA96_MATCH_ENTITY_NONE;
  if (!mr || !mr->live_start) return;
  t = match_run_live_control_team(mr);
  if (t < 0) return;
  {
    int index = fifa96_match_entities_team_select(&mr->entities, (uint32_t)t,
                                                  (int16_t)mr->entities.ball.x,
                                                  (int16_t)mr->entities.ball.z);
    if (index >= 0)
      taker = (int32_t)((uint32_t)t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)index);
  }
  if (taker != FIFA96_MATCH_ENTITY_NONE &&
      match_run_live_control_bind(mr, taker, 1) == 1) {
    if (mr->formation_seeded)
      match_run_live_control_place_taker(mr, taker);
    mr->live_kickoff_touch = 1u;
    mr->live_control_taker = taker;
  }
}

/* P7 / FU-345: the per-frame live control follow. The native control cell is
 * re-published by the possession machinery; while that caller chain is
 * unported the engine keeps the human on the controlled team's ball carrier
 * when the pool has one, else on the native nearest-outfield select shape. The
 * opponent block and the keeper are never selected; a staged phase (goal,
 * celebration, restart) is left alone. Live-only.
 *
 * Derived possession rules (labelled [D], part of the FU-295 stand-in):
 *  - the live kickoff seed holds the seeded taker as the carrier until the
 *    first FU-73 release (`live_kickoff_touch`), so the opening kick does not
 *    depend on the human reacting before the camera-relative ball drift.
 *  - a carrier is only "the controlled team's ball" while it is within
 *    `MATCH_RUN_LIVE_CONTROL_RANGE` of the ball. The row-05 claim cell is
 *    republished by the unported possession AI and otherwise latches a team
 *    target hundreds of units away; the native claim is a local contact, so the
 *    far latch is treated as loose. The range is a stand-in constant, not a
 *    native threshold.
 *  - when the ball is loose and the controlled side's nearest outfield record
 *    is at least as close to the ball as the opponent's nearest, the human
 *    takes the possession cell ([D] for the unported row-05 claim). The
 *    opponent records are only read; none is written.
 *  - when the FU-73 pair records the selected record's release (KICK/PASS/
 *    LONG_BALL) while the record has already left the carrier state 5, the
 *    kick released possession: clear `ball.carrier` so the next frame selects
 *    the nearest chaser instead of instantly re-attaching the kicker.
 *  - while the pool carrier is the selected record, the real installer keeps
 *    it in the carrier state 5 (a [D] repair for the unported possession
 *    caller; the transient kick code 7 is never overwritten).
 */
#define MATCH_RUN_LIVE_CONTROL_RANGE 0x400
/* The live possession glue only follows a ball already at the carrier's feet;
 * a stale/far manual staging is left to the loose ball machine. */
#define MATCH_RUN_LIVE_CARRIER_GLUE_RANGE 0x80
/* P13 live ball-boundary/restart constants (the derived live armer). */
#define MATCH_RUN_LIVE_TOUCHLINE_X 0x7E0
#define MATCH_RUN_LIVE_GOALLINE_Z 0xC1E
#define MATCH_RUN_LIVE_RESTART_WAIT 90u
#define MATCH_RUN_LIVE_CONTROL_HOLD_FRAMES 30u

static int match_run_live_control_near(const struct fifa96_match_run *mr,
                                       const struct fifa96_match_entity *e) {
  int32_t dx = mr->entities.ball.x - e->pos_x;
  int32_t dz = mr->entities.ball.z - e->pos_z;
  return fifa96_entity_distance(dx, dz) <= MATCH_RUN_LIVE_CONTROL_RANGE;
}

/* The native `FUN_0008D8EC` nearest search without the team-target write (the
 * control follow only needs the opponent distance to decide possession). */
static int match_run_live_control_nearest(const struct fifa96_match_run *mr,
                                          uint32_t t, int16_t *best_distance) {
  fifa96_entity_candidate candidates[FIFA96_MATCH_ENTITY_RECORDS];
  int16_t best = 0;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[t].records[i];
    candidates[i].x = (int16_t)e->pos_x;
    candidates[i].y = (int16_t)e->pos_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  /* skip_index 0: record 0 is the keeper, never the outfield selection. */
  {
    int index = fifa96_entity_find_nearest(candidates, FIFA96_MATCH_ENTITY_RECORDS, 0,
                                           (int16_t)mr->entities.ball.x,
                                           (int16_t)mr->entities.ball.z, &best);
    if (best_distance) *best_distance = best;
    return index;
  }
}

/* Phase 76 [D]: the manual switch cycle. It reuses the bind's gates (own
 * side, non-keeper, not hidden/occupied) but cycles record indices 1..10 in
 * order, so repeated presses are deterministic and do not depend on distance.
 * The native FU-295 auto-switch/AI caller remains P7-L1. */
#define MATCH_RUN_LIVE_MANUAL_FRAMES 90u

static int match_run_live_control_switchable(const struct fifa96_match_run *mr,
                                             uint32_t t, int32_t id) {
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  if ((uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS != t) return 0;
  uint32_t index = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  if (index == 0u) return 0;
  const struct fifa96_match_entity *e = &mr->entities.team[t].records[index];
  if (e->active == 0u || e->skip_98 != 0u || e->skip_9a != 0u) return 0;
  return 1;
}

static int32_t match_run_live_control_switch_next(const struct fifa96_match_run *mr,
                                                  uint32_t t, int32_t from) {
  int start = match_run_live_control_switchable(mr, t, from)
                  ? (int)((uint32_t)from % FIFA96_MATCH_ENTITY_RECORDS)
                  : 0;
  for (int step = 1; step < (int)FIFA96_MATCH_ENTITY_RECORDS; step++) {
    int index = (start + step) % (int)FIFA96_MATCH_ENTITY_RECORDS;
    if (index == 0) continue;
    int32_t id = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)index);
    if (match_run_live_control_switchable(mr, t, id)) return id;
  }
  return FIFA96_MATCH_ENTITY_NONE;
}

/* P7 / FU-345 (flight): latch the human kick's staged FU-73 block into a
 * derived ball-flight target. The native per-code target algebra
 * (`0x7AA3C..0x7AE2F`, OL-62) is unported, so the mapping below is explicitly
 * derived from the staged semantics already ported:
 *   - `pair.vector.x` is the kick speed (the row's 0x390..0x5A0 family);
 *   - `pair.vector.height`/`pair.vector.z` are the speed-scaled direction
 *     components (`FUN_000795A4` sine folds), used natively as local target
 *     offsets from the camera (`0x7AA68..`);
 *   - `pair.traj` is the loft word (capped at 0x460).
 * Derived mapping [D]: PASS/LONG_BALL project the speed-scaled pair direction
 * to `speed * 0x60` units ahead of the kicker, speed
 * `clamp(|vector.x| >> 6, 8, 16)`, bounded to 300 frames. No per-code arm,
 * loft arc, receive hand-off or out-of-play rule is claimed as native; the
 * flight consumer only steers the existing ball machine. */
#define MATCH_RUN_LIVE_FLIGHT_FRAMES 300u
#define MATCH_RUN_LIVE_FLIGHT_GOAL_FRAMES 600u
#define MATCH_RUN_LIVE_FLIGHT_SPEED_MIN 8
#define MATCH_RUN_LIVE_FLIGHT_SPEED_MAX 16

/* P8 (FU-346): drop the consumed-kick value signature. The duplicate guard is
 * the consumed-pair marker below; both are quoted by the live tests/docs. On a
 * block the pair is cleared too; on a flight end only the signature is
 * dropped, while the consumed-pair marker keeps the still-staged FU-73 word
 * from relaunching itself (a new dispatch clears the marker). */
static void match_run_live_kick_sig_clear(struct fifa96_match_run *mr) {
  mr->live_kick_sig_actor = FIFA96_MATCH_ENTITY_NONE;
  mr->live_kick_sig_traj = 0;
  mr->live_kick_sig_vx = 0;
  mr->live_kick_sig_vh = 0;
  mr->live_kick_sig_vz = 0;
  mr->live_kick_sig_flags = 0;
  mr->live_kick_sig_code = 0;
}

/* The pair consumed by the last latch is still the staged FU-73 word. Only the
 * signature fields are compared: the native tail also updates receiver/angle/
 * ack while the ball is live, and those must not re-arm the flight. */
static int match_run_live_kick_pair_matches_consumed(const struct fifa96_match_run *mr) {
  const struct fifa96_ball_pair_state *p = &mr->entities.ball.pair;
  const struct fifa96_ball_pair_state *c = &mr->live_kick_consumed_pair;
  return mr->live_kick_pair_consumed != 0u && p->actor == c->actor &&
         p->traj == c->traj && p->vector.x == c->vector.x &&
         p->vector.height == c->vector.height &&
         p->vector.z == c->vector.z && p->flags == c->flags &&
         p->code == c->code;
}

static void match_run_live_kick_consumed_clear(struct fifa96_match_run *mr) {
  mr->live_kick_pair_consumed = 0u;
  memset(&mr->live_kick_consumed_pair, 0,
         sizeof mr->live_kick_consumed_pair);
}

/* P9 / FU-347: reset the shot aim. One helper owns the latch; `reset_edge`
 * also drops the action/depth sample. The latch-consume, save and flight-end
 * resets keep the sample so a held action+depth cannot re-arm the aim from
 * the same press, while init/begin, a phase change and a restart start a
 * fresh edge. */
static void match_run_live_aim_reset(struct fifa96_match_run *mr,
                                     int reset_edge) {
  mr->live_aim_goal_x = 0;
  if (reset_edge) mr->live_aim_prev_input = 0;
}

/* Phase 78 [D]: the live free-kick/penalty taker seam. The native phase arm
 * installs row 0x12 (free kick) or 0x13 (penalty) on the fouled team and the
 * row auto-fires at stage 4. The native input words 0x4C114/18/1C and the slot
 * word[+6] 0x20 edge are unported (FU-266/FU-275 legs), so this seam binds the
 * installed taker, holds the row, and maps the live arrows/KICK to a derived
 * goal-mouth delivery through the existing live flight primitive. Non-human
 * takers keep the native auto path; the human timeout auto-delivers the same
 * derived strike so the unported penalty ack wait cannot hold phase 6. */
#define MATCH_RUN_LIVE_SETPIECE_WAIT 300u
#define MATCH_RUN_LIVE_SETPIECE_AIM_STEP 0x20
#define MATCH_RUN_LIVE_SETPIECE_AIM_MAX 0xC0
#define MATCH_RUN_LIVE_SETPIECE_POWER_MIN 1u
#define MATCH_RUN_LIVE_SETPIECE_POWER_MAX 4u
#define MATCH_RUN_LIVE_SETPIECE_INPUT_MASK 0x1Fu

static void match_run_live_setpiece_reset(struct fifa96_match_run *mr) {
  if (!mr) return;
  mr->live_sp_active = 0;
  mr->live_sp_hold = 0;
  mr->live_sp_kind = 0;
  mr->live_sp_power = 0;
  mr->live_sp_prev_input = 0;
  mr->live_sp_frames = 0;
  mr->live_sp_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->live_sp_aim_x = 0;
  mr->flag_157ab0 = 0;       /* the set-piece outfield skip ends with the row */
}

static int32_t match_run_live_setpiece_find_taker(const struct fifa96_match_run *mr,
                                                  int32_t team, uint8_t phase) {
  uint8_t code = phase == 6u ? 0x13u : 0x12u;
  int32_t target;
  if (team < 0 || team >= (int32_t)FIFA96_MATCH_ENTITY_TEAMS) return -1;
  target = mr->entities.team[(uint32_t)team].target;
  if (target >= 0 &&
      target < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS) &&
      (uint32_t)(target / (int32_t)FIFA96_MATCH_ENTITY_RECORDS) == (uint32_t)team &&
      target % (int32_t)FIFA96_MATCH_ENTITY_RECORDS != 0) {
    const struct fifa96_match_entity *e =
        &mr->entities.team[(uint32_t)team]
             .records[(uint32_t)target % FIFA96_MATCH_ENTITY_RECORDS];
    if (e->active != 0u && e->skip_98 == 0u && e->skip_9a == 0u &&
        e->code == code)
      return target;
  }
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e =
        &mr->entities.team[(uint32_t)team].records[i];
    if (e->active != 0u && e->skip_98 == 0u && e->skip_9a == 0u &&
        e->code == code)
      return (int32_t)((uint32_t)team * FIFA96_MATCH_ENTITY_RECORDS + i);
  }
  return -1;
}

static int match_run_live_setpiece_activate(struct fifa96_match_run *mr) {
  int32_t team = match_run_live_control_team(mr);
  uint8_t kind = (uint8_t)mr->state.phase;
  int32_t taker;
  if (team < 0) return 0;
  taker = match_run_live_setpiece_find_taker(mr, team, kind);
  if (taker < 0) return 0;
  if (match_run_live_control_bind(mr, taker, 0) != 1) {
    /* The phase arm's taker may not carry a FU-70 slot in the headless pool.
     * The derived live seam still controls the installed record; the native
     * bind/merge path is attempted first. */
    uint32_t t = (uint32_t)taker / FIFA96_MATCH_ENTITY_RECORDS;
    mr->slot.entity = taker;
    mr->entities.controlled = taker;
    mr->entities.team[t].target = taker;
    (void)fifa96_camera_set_tracked(&mr->render.camera, taker);
  }
  mr->live_sp_active = 1;
  mr->live_sp_hold = 1;
  mr->live_sp_kind = kind;
  mr->live_sp_power = 3;
  mr->live_sp_prev_input = mr->input.held & MATCH_RUN_LIVE_SETPIECE_INPUT_MASK;
  mr->live_sp_frames = 0;
  mr->live_sp_taker = taker;
  mr->live_sp_aim_x = 0;
  return 1;
}

/* Phase 78 [D]: consume the player aim/power/confirm. The goal is the opponent
 * mouth in z; x is the derived aim within the native mouth bound. The confirm
 * resets the taker row and hands the ball to the phase-2 live flight, so the
 * existing keeper save/goal armer/out-of-play consumers remain authoritative. */
static void match_run_live_setpiece_kick(struct fifa96_match_run *mr) {
  int32_t taker = mr->live_sp_taker;
  uint32_t team;
  struct fifa96_match_entity *e;
  int32_t target_z;
  int16_t speed;
  if (taker < 0 ||
      taker >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
    match_run_live_setpiece_reset(mr);
    return;
  }
  team = (uint32_t)taker / FIFA96_MATCH_ENTITY_RECORDS;
  e = &mr->entities.team[team]
           .records[(uint32_t)taker % FIFA96_MATCH_ENTITY_RECORDS];
  target_z = mr->entities.team[team].side == 0u ? 0xB40 : -0xB40;
  speed = (int16_t)(8 + (int16_t)mr->live_sp_power * 2);
  e->timer89 = 0;
  (void)fifa96_match_entities_install(e, 2u, 0u, 0);
  mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  (void)fifa96_ball_pair_clear(&mr->entities.ball.pair);
  match_run_live_kick_consumed_clear(mr);
  match_run_live_kick_sig_clear(mr);
  mr->live_kick_target_x = mr->live_sp_aim_x;
  mr->live_kick_target_z = target_z;
  mr->live_kick_speed = speed;
  mr->live_kick_frames = MATCH_RUN_LIVE_FLIGHT_GOAL_FRAMES;
  mr->live_kick_active = 1u;
  mr->live_kick_shot = 1u;
  mr->live_shot_pending = 1u;
  mr->live_kick_hold = MATCH_RUN_LIVE_CONTROL_HOLD_FRAMES;
  mr->live_restart_pending = 0;
  match_run_ball_settle_clear(mr);
  mr->ref_restart_stage = 0;
  (void)fifa96_match_run_situation(mr, 0x0Bu);
  match_run_live_setpiece_reset(mr);
}

static void match_run_live_setpiece_update(struct fifa96_match_run *mr) {
  uint8_t sample;
  uint8_t rising;
  int32_t team;
  if (!mr || !mr->live_start) return;
  if (mr->state.phase != 6u && mr->state.phase != 7u) {
    match_run_live_setpiece_reset(mr);
    return;
  }
  if (mr->live_sp_active == 0u) {
    if (match_run_live_setpiece_activate(mr) == 0) return;
  }
  team = match_run_live_control_team(mr);
  if (team < 0 || mr->live_sp_taker < 0 ||
      match_run_live_setpiece_find_taker(mr, team, mr->live_sp_kind) !=
          mr->live_sp_taker) {
    match_run_live_setpiece_reset(mr);
    return;
  }
  if (++mr->live_sp_frames >= MATCH_RUN_LIVE_SETPIECE_WAIT) {
    /* Bounded player window: auto-deliver the same derived strike. The native
     * row-0x13 stage-5 ack wait is one of the unported FU-266/FU-275 legs and
     * can hold phase 6 forever, so the human timeout does not hand back to
     * that row. Non-human takers still use the native auto row untouched. */
    mr->live_sp_hold = 0;
    match_run_live_setpiece_kick(mr);
    return;
  }
  sample = mr->input.held & MATCH_RUN_LIVE_SETPIECE_INPUT_MASK;
  rising = (uint8_t)(sample & (uint8_t)~mr->live_sp_prev_input);
  mr->live_sp_prev_input = sample;
  if ((rising & 0x04u) != 0u) mr->live_sp_aim_x += MATCH_RUN_LIVE_SETPIECE_AIM_STEP;
  if ((rising & 0x08u) != 0u) mr->live_sp_aim_x -= MATCH_RUN_LIVE_SETPIECE_AIM_STEP;
  if (mr->live_sp_aim_x > MATCH_RUN_LIVE_SETPIECE_AIM_MAX)
    mr->live_sp_aim_x = MATCH_RUN_LIVE_SETPIECE_AIM_MAX;
  if (mr->live_sp_aim_x < -MATCH_RUN_LIVE_SETPIECE_AIM_MAX)
    mr->live_sp_aim_x = -MATCH_RUN_LIVE_SETPIECE_AIM_MAX;
  if ((rising & 0x01u) != 0u &&
      mr->live_sp_power < MATCH_RUN_LIVE_SETPIECE_POWER_MAX)
    mr->live_sp_power++;
  if ((rising & 0x02u) != 0u &&
      mr->live_sp_power > MATCH_RUN_LIVE_SETPIECE_POWER_MIN)
    mr->live_sp_power--;
  if ((rising & 0x10u) != 0u) {
    mr->live_sp_hold = 0;
    match_run_live_setpiece_kick(mr);
    return;
  }
}

/* Phase 75 [D]: clear the derived support-run selection. Called on init,
 * begin, possession loss, restarts and any non-play phase. */
/* Phase 76 [D]: clear the manual switch override/queued edge. The physical
 * `live_switch_down` level survives a phase change until the next input poll. */
static void match_run_live_manual_reset(struct fifa96_match_run *mr) {
  if (!mr) return;
  mr->live_manual_id = FIFA96_MATCH_ENTITY_NONE;
  mr->live_manual_frames = 0;
  mr->live_switch_press = 0;
}

/* Phase 77 [D]: clear a queued human contact edge/cooldown. */
static void match_run_live_tackle_reset(struct fifa96_match_run *mr) {
  if (!mr) return;
  mr->live_tackle_press = 0;
  mr->live_human_tackle_cd = 0;
}

static void match_run_live_support_reset(struct fifa96_match_run *mr) {
  if (!mr) return;
  int32_t runner = mr->live_support_runner;
  if (runner >= 0 &&
      runner < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS *
                         FIFA96_MATCH_ENTITY_RECORDS)) {
    struct fifa96_match_entity *e =
        &mr->entities.team[(uint32_t)runner / FIFA96_MATCH_ENTITY_RECORDS]
             .records[(uint32_t)runner % FIFA96_MATCH_ENTITY_RECORDS];
    if (e->code == 3u && e->has_slot == 0u) {
      /* Stop a stale run immediately; the next off-ball shape pass may assign
       * the formation/restart target. */
      e->target_x = e->pos_x;
      e->target_z = e->pos_z;
    }
  }
  mr->live_support_runner = FIFA96_MATCH_ENTITY_NONE;
  mr->live_support_timer = 0;
}

static void match_run_live_kick_latch(struct fifa96_match_run *mr) {
  struct fifa96_ball_pair_state *pair;
  struct fifa96_match_entity *e;
  int32_t id;
  int32_t dir_x;
  int32_t dir_z;
  int32_t speed;
  int kick_row;
  uint32_t team8;
  if (!mr || !mr->live_start) return;
  pair = &mr->entities.ball.pair;
  if ((pair->flags & 0x70u) == 0u) return;
  id = pair->actor;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return;
  /* Only the human's slot/actor starts a flight; an opponent/AI pair is left
   * to the native ball machine. `live_control_taker` is accepted only on the
   * opening-kick release frame (`live_kickoff_touch` still set); afterwards the
   * old taker is an AI record and must not arm a human flight. */
  if (id != mr->slot.entity && id != mr->entities.controlled &&
      !(id == mr->live_control_taker && mr->live_kickoff_touch != 0u))
    return;
  /* P8 (FU-346): the same staged FU-73 pair cannot relaunch after its flight.
   * The flag is cleared by a new kick dispatch (`match_kick_run`) and by the
   * block; the signature values are still recorded/cleared for the live tests
   * and docs. */
  if (match_run_live_kick_pair_matches_consumed(mr)) return;
  team8 = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  e = &mr->entities.team[team8].records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
  dir_x = pair->vector.height;
  dir_z = pair->vector.z;
  if (dir_x == 0 && dir_z == 0) {
    dir_x = e->dir_x;
    dir_z = e->dir_z;
  }
  if (dir_x == 0 && dir_z == 0)
    dir_z = mr->entities.team[team8].side == 0u ? 1 : -1;
  speed = pair->vector.x;
  if (speed < 0) speed = -speed;
  speed >>= 6;
  if (speed < MATCH_RUN_LIVE_FLIGHT_SPEED_MIN) speed = MATCH_RUN_LIVE_FLIGHT_SPEED_MIN;
  if (speed > MATCH_RUN_LIVE_FLIGHT_SPEED_MAX) speed = MATCH_RUN_LIVE_FLIGHT_SPEED_MAX;
  /* P7 / FU-345 (shot aim): the KICK row (mode 0x10) is the human shot; the
   * staged direction is the record's static facing and a normal kickoff shot
   * then misses the mouth. For the KICK row only, aim the flight at the
   * opponent goal mouth (z=±0xB30) instead of the derived kicker-relative
   * target. P9 / FU-347: the held **depth** input aims the mouth target —
   * mapped Up (0x01) -> x=+0x80, Down (0x02) -> x=-0x80, none/both -> x=0 —
   * inside the `FIFA96_MATCH_LIVE_GOAL_MOUTH_X` mouth (Left/Right are the
   * goal-side z axis, not aim inputs). PASS/LONG_BALL (0x20/0x40) keep the
   * staged direction. This
   * remains a labelled [D] shot-aim stand-in for the unported aim/camera
   * director; it never writes score, phase or the goal cells. */
  kick_row = ((pair->flags & 0x10u) != 0u && (pair->flags & 0x60u) == 0u);
  if (kick_row) {
    /* P9 / FU-347: consume the depth aim captured by the live update (the
     * slot's released word only arrives after the held keys drop). */
    mr->live_kick_target_x = mr->live_aim_goal_x;
    match_run_live_aim_reset(mr, 0);     /* consume: next kick needs a new edge */
    mr->live_kick_target_z =
        mr->entities.team[team8].side == 0u ? 0xB30 : -0xB30;
  } else {
    /* PASS/LONG_BALL (the FU-73 pair's own direction): the native vector is
     * speed-scaled, so normalise it and project it to a visible pass reach.
     * The old fixed x2 scale put the target two units away, which made the
     * derived flight arrive on the next frame (a pass looked like a stuck
     * ball). The reach grows with the staged speed word; the pitch clamp is
     * the native mover bound. */
    int32_t ax = dir_x < 0 ? -dir_x : dir_x;
    int32_t az = dir_z < 0 ? -dir_z : dir_z;
    int32_t norm = ax > az ? ax : az;
    if (norm < 1) norm = 1;
    int32_t reach = speed * 0x60;
    int32_t tx = e->pos_x + (dir_x * reach) / norm;
    int32_t tz = e->pos_z + (dir_z * reach) / norm;
    if (tx > 0x720) tx = 0x720;
    else if (tx < -0x720) tx = -0x720;
    if (tz > 0xB10) tz = 0xB10;
    else if (tz < -0xB10) tz = -0xB10;
    mr->live_kick_target_x = tx;
    mr->live_kick_target_z = tz;
  }
  mr->live_kick_speed = (int16_t)speed;
  mr->live_kick_frames =
      kick_row ? MATCH_RUN_LIVE_FLIGHT_GOAL_FRAMES : MATCH_RUN_LIVE_FLIGHT_FRAMES;
  mr->live_kick_active = 1u;
  mr->live_kick_shot = kick_row ? 1u : 0u;   /* P8: the shot block's gate */
  mr->live_shot_pending = kick_row ? 1u : 0u; /* P13: mouth target latch */
  mr->live_kick_sig_actor = id;
  mr->live_kick_sig_traj = pair->traj;
  mr->live_kick_sig_vx = pair->vector.x;
  mr->live_kick_sig_vh = pair->vector.height;
  mr->live_kick_sig_vz = pair->vector.z;
  mr->live_kick_sig_flags = pair->flags;
  mr->live_kick_sig_code = pair->code;
  mr->live_kick_consumed_pair = *pair;
  mr->live_kick_pair_consumed = 1u;
}

/* The ball-machine override: while a live kick is in flight, use the latched
 * target/speed instead of the camera-relative phase-2 target. Returns 1 when
 * the override was applied. */
static int match_run_live_kick_flight_step(struct fifa96_match_run *mr,
                                           struct fifa96_match_ball *b) {
  if (!mr || !mr->live_start || !mr->live_kick_active) return 0;
  if (mr->state.phase != 2u || mr->goal_armed) {
    mr->live_kick_active = 0;
    mr->live_kick_shot = 0;
    match_run_live_kick_sig_clear(mr);
    return 0;
  }
  b->target_x = mr->live_kick_target_x;
  b->target_y = 0;
  b->target_z = mr->live_kick_target_z;
  b->max_speed = mr->live_kick_speed;
  return 1;
}

/* Clear the flight after the frame's integration: arrival, out-of-play,
 * goal/phase transition or the bounded window. */
static int match_run_live_shot_on_target(const struct fifa96_match_run *mr) {
  int32_t target_z;
  if (!mr || !mr->live_kick_shot) return 0;
  target_z = mr->live_kick_target_z;
  if (target_z < 0) target_z = 0 - target_z;
  return mr->live_kick_target_x > -FIFA96_MATCH_LIVE_GOAL_MOUTH_X &&
         mr->live_kick_target_x < FIFA96_MATCH_LIVE_GOAL_MOUTH_X &&
         target_z >= 0xB10;
}

static void match_run_live_kick_flight_end(struct fifa96_match_run *mr,
                                           const struct fifa96_match_ball *b) {
  int32_t ax;
  int32_t az;
  int32_t dx;
  int32_t dz;
  int keep_on_target;
  if (!mr || !mr->live_start || !mr->live_kick_active) return;
  ax = b->x < 0 ? -b->x : b->x;
  az = b->z < 0 ? -b->z : b->z;
  dx = b->x - mr->live_kick_target_x;
  dz = b->z - mr->live_kick_target_z;
  keep_on_target = match_run_live_shot_on_target(mr);
  if (mr->live_kick_frames != 0u) mr->live_kick_frames--;
  if (mr->live_kick_frames == 0u || mr->goal_armed || mr->state.phase != 2u ||
      ax > 0x6C0 || (az > 0xC00 && !keep_on_target) ||
      (dx <= 0x20 && dx >= -0x20 && dz <= 0x20 && dz >= -0x20)) {
    int keep_pending = keep_on_target && az > 0xC00;
    mr->live_kick_active = 0;
    mr->live_kick_shot = 0;
    if (!keep_pending) mr->live_shot_pending = 0;
    match_run_live_aim_reset(mr, 0);     /* P9-fix2: no stale aim */
    match_run_live_kick_sig_clear(mr);
  }
}

/* P14 derived CPU ball action [D]. There is no native AI kick chooser; this
 * reuses the existing live flight/ball-machine primitive for an opponent pass
 * or shot. It writes no score/phase cell; the native goal armer/keeper chain
 * remain the scoring authority. */
static void match_run_live_cpu_kick(struct fifa96_match_run *mr, int32_t target_x,
                                    int32_t target_z, int16_t speed,
                                    uint16_t frames, int shot) {
  struct fifa96_match_ball *b = &mr->entities.ball;
  b->carrier = FIFA96_MATCH_ENTITY_NONE;
  (void)fifa96_ball_pair_clear(&b->pair);
  match_run_live_kick_consumed_clear(mr);
  match_run_live_kick_sig_clear(mr);
  match_run_live_aim_reset(mr, 0);
  mr->live_kick_target_x = target_x;
  mr->live_kick_target_z = target_z;
  mr->live_kick_speed = speed;
  mr->live_kick_frames = frames;
  mr->live_kick_active = 1u;
  mr->live_kick_shot = shot ? 1u : 0u;
  mr->live_shot_pending = shot ? 1u : 0u;
}

/* P14 derived keeper outlet [D]. The P9 save pins the ball on the keeper for
 * 30 frames but never releases it; after the hold, clear toward a teammate /
 * the team's attacking half with the shared CPU primitive. Live-only. */
static void match_run_live_keeper_outlet(struct fifa96_match_run *mr) {
  int32_t record = mr->live_save_record;
  uint32_t team;
  int32_t dir;
  int32_t target_x;
  int32_t target_z;
  if (!mr || !mr->live_start || record == FIFA96_MATCH_ENTITY_NONE) return;
  if (record < 0 ||
      record >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return;
  team = (uint32_t)record / FIFA96_MATCH_ENTITY_RECORDS;
  dir = mr->entities.team[team].side == 0u ? 1 : -1;
  target_x = 0;
  target_z = dir > 0 ? 0x500 : -0x500;
  {
    int index = fifa96_match_entities_team_select(&mr->entities, team,
                                                  (int16_t)target_x,
                                                  (int16_t)target_z);
    if (index >= 0) {
      const struct fifa96_match_entity *e = &mr->entities.team[team].records[index];
      target_x = e->pos_x;
      target_z = e->pos_z + dir * 0x80;
    }
  }
  match_run_live_cpu_kick(mr, target_x, target_z, 10, 300, 0);
  mr->live_save_record = FIFA96_MATCH_ENTITY_NONE;
  mr->live_save_hold = 0;
}

/* P7 / FU-345 (restart): bind the human control to its nearest outfield record
 * at a phase-1 -> phase-2 restart, put it on the ball and install the carrier
 * state. The native restart's own control producer is the unported FU-295
 * caller chain; the human-restart possession and placement are a labelled [D]
 * stand-in (the native conceding-side kickoff split is unported), applied only
 * when the resource `.fmt` seed moved the pool. The same native helpers as the
 * live seed rebind the FU-70 slot / actor / team target / camera. */
static void match_run_live_restart_bind(struct fifa96_match_run *mr) {
  int32_t t;
  int32_t id = FIFA96_MATCH_ENTITY_NONE;
  if (!mr || !mr->live_start || mr->state.phase != 2u) return;
  t = match_run_live_control_team(mr);
  if (t < 0) return;
  {
    int index = fifa96_match_entities_team_select(&mr->entities, (uint32_t)t,
                                                  (int16_t)mr->entities.ball.x,
                                                  (int16_t)mr->entities.ball.z);
    if (index >= 0)
      id = (int32_t)((uint32_t)t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)index);
  }
  if (id == FIFA96_MATCH_ENTITY_NONE) return;
  if (!mr->formation_seeded) {
    (void)match_run_live_control_bind(mr, id, 0);
    return;
  }
  if (match_run_live_control_bind(mr, id, 1) == 1) {
    match_run_live_control_place_taker(mr, id);
    mr->live_kickoff_touch = 1u;
    mr->live_control_taker = id;
  }
}

/* P7-L2 (tackle/possession stand-in): the static AI restart can park an
 * opponent carrier on the ball, and the nearest-only claim can never beat
 * distance 0. When the human's selected record is within the contact range of
 * a loose/stationary opponent-carried ball (and no live kick is in flight),
 * clear the opponent possession cell and hand the carrier state to the human.
 * This is a derived contact tackle, not a native row-05 steal; it writes only
 * the ball possession cell and the human record's carrier code, never the
 * score/phase/goal cells. */
static int match_run_live_control_tackle(struct fifa96_match_run *mr, uint32_t t,
                                         int32_t human_id,
                                         struct fifa96_match_entity *human) {
  int32_t carrier;
  int32_t dx;
  int32_t dz;
  if (!mr || !mr->live_start || mr->state.phase != 2u) return 0;
  if (mr->live_ai_tackle_cd != 0u) return 0;     /* opponent just won it */
  if (mr->live_kick_active != 0u) return 0;      /* no claim while in flight */
  if (mr->live_kick_hold != 0u) return 0;        /* post-kick release hold */
  carrier = mr->entities.ball.carrier;
  if (carrier < 0 ||
      carrier >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  if ((uint32_t)(carrier / (int32_t)FIFA96_MATCH_ENTITY_RECORDS) == t) return 0;
  if (mr->entities.ball.speed > 4) return 0;     /* only a loose/stationary ball */
  dx = mr->entities.ball.x - human->pos_x;
  dz = mr->entities.ball.z - human->pos_z;
  if (fifa96_entity_distance(dx, dz) > 0x40) return 0;
  mr->entities.ball.carrier = human_id;
  if (human->code != 5u)
    (void)fifa96_match_entities_install(human, (uint8_t)mr->state.phase, 5u, 0);
  return 1;
}

/* P9 / FU-347: sample the held **depth** input into the live shot-aim cell.
 * The live view maps world x to the near/far (goal-mouth width) axis: the
 * native movement maps Up (mapped 0x01) -> +x and Down (0x02) -> -x, so the
 * shot aim uses Up -> +0x80, Down -> -0x80, none/both -> 0. Left/Right are the
 * goal-side z axis (Left -> +z, Right -> -z) and are deliberately not aim
 * inputs. The native FU-70 slot only publishes its released action word when
 * every held key drops, so the aim is captured on a real action-button rising
 * edge (or a depth-key edge while the action is held) and retained until the
 * KICK latch consumes it. A new action press always recomputes the aim from
 * the held depth bits, so an unaimed KICK pressed while another direction is
 * held clears a stale ±0x80 instead of inheriting it. */
static void match_run_live_aim_update(struct fifa96_match_run *mr) {
  uint8_t current =
      (uint8_t)(mr->input.held & FIFA96_MATCH_LIVE_AIM_SAMPLE_MASK);
  uint8_t rising = (uint8_t)(current & (uint8_t)~mr->live_aim_prev_input);
  uint8_t depth;
  mr->live_aim_prev_input = current;
  if ((rising & FIFA96_MATCH_LIVE_AIM_ACTION_MASK) == 0u &&
      !((rising & FIFA96_MATCH_LIVE_AIM_DEPTH_MASK) != 0u &&
        (current & FIFA96_MATCH_LIVE_AIM_ACTION_MASK) != 0u))
    return;
  depth = (uint8_t)(current & FIFA96_MATCH_LIVE_AIM_DEPTH_MASK);
  if (depth == FIFA96_MATCH_LIVE_AIM_UP)
    mr->live_aim_goal_x = FIFA96_MATCH_LIVE_SHOT_AIM_X;
  else if (depth == FIFA96_MATCH_LIVE_AIM_DOWN)
    mr->live_aim_goal_x = -FIFA96_MATCH_LIVE_SHOT_AIM_X;
  else
    mr->live_aim_goal_x = 0;            /* none/both depth keys = centre */
}

/* P13 live out-of-play restart fallback [D]. The native taker rows can hold
 * at stage 1 without a keeper/restart slot (the goal-kick dead end); this
 * bounded fallback places the ball and taker, returns to phase 2 and clears
 * the restart machinery. It writes no score or goal cell. */
static void match_run_live_out_force(struct fifa96_match_run *mr) {
  struct fifa96_match_ball *b = &mr->entities.ball;
  int32_t x;
  int32_t z;
  int32_t team;
  uint8_t side = mr->live_out_side;
  int32_t sign_x = mr->goal_snap_x < 0 ? -1 : 1;
  int32_t sign_z = mr->goal_snap_z < 0 ? -1 : 1;
  int32_t setpiece_taker = mr->live_sp_active ? mr->live_sp_taker
                                              : FIFA96_MATCH_ENTITY_NONE;
  if (side > 1u) side = 0u;
  /* Phase 78 [D]: an out-of-play force can land during a staged free kick or
   * penalty. Reset the installed taker before the fallback placement; if the
   * fallback selects the same record it is re-installed as the throw/corner
   * taker below, otherwise the stale 0x12/0x13 row cannot auto-fire in phase 2. */
  if (setpiece_taker >= 0 &&
      setpiece_taker <
          (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
    struct fifa96_match_entity *se =
        &mr->entities.team[(uint32_t)setpiece_taker / FIFA96_MATCH_ENTITY_RECORDS]
             .records[(uint32_t)setpiece_taker % FIFA96_MATCH_ENTITY_RECORDS];
    se->timer89 = 0;
    (void)fifa96_match_entities_install(se, 2u, 0u, 0);
  }
  match_run_live_setpiece_reset(mr);
  switch (mr->live_out_situation) {
    case 2u: {   /* throw-in */
      x = sign_x * 0x7D0;
      z = mr->goal_snap_z;
      if (z > 0xB00) z = 0xB00;
      if (z < -0xB00) z = -0xB00;
      break;
    }
    case 3u:     /* corner */
      x = sign_x * 0x7D0;
      z = sign_z * 0xBF0;
      break;
    case 4u:     /* goal kick */
      x = 0;
      z = sign_z * 0xB90;
      break;
    default:
      x = 0;
      z = 0;
      break;
  }
  mr->live_kick_active = 0;
  mr->live_kick_shot = 0;
  mr->live_kick_frames = 0;
  mr->live_kick_hold = 0;
  mr->live_shot_pending = 0;
  mr->live_save_hold = 0;
  mr->live_save_record = FIFA96_MATCH_ENTITY_NONE;
  mr->live_cpu_hold = 0;
  mr->live_cpu_actions = 0;
  mr->live_cpu_carrier = FIFA96_MATCH_ENTITY_NONE;
  mr->live_ai_tackle_cd = 0;
  match_run_live_support_reset(mr);
  match_run_live_manual_reset(mr);
  match_run_live_tackle_reset(mr);
  match_run_live_aim_reset(mr, 0);
  match_run_live_kick_sig_clear(mr);
  match_run_live_kick_consumed_clear(mr);
  b->x = x;
  b->y = 0;
  b->z = z;
  b->target_x = x;
  b->target_y = 0;
  b->target_z = z;
  b->vel_x = 0;
  b->vel_z = 0;
  b->speed = 0;
  b->carrier = FIFA96_MATCH_ENTITY_NONE;
  (void)fifa96_ball_pair_clear(&b->pair);
  team = match_run_live_team_for_side(mr, side);
  if (team >= 0) {
    int index = fifa96_match_entities_team_select(&mr->entities, (uint32_t)team,
                                                  (int16_t)x, (int16_t)z);
    if (index >= 0) {
      int32_t id = (int32_t)((uint32_t)team * FIFA96_MATCH_ENTITY_RECORDS +
                             (uint32_t)index);
      b->carrier = id;
      if ((uint8_t)(mr->phase_machine.side_controlled & 1u) == side &&
          match_run_live_control_bind(mr, id, 1) == 1) {
        match_run_live_control_place_taker(mr, id);
        mr->live_kickoff_touch = 1u;
        mr->live_control_taker = id;
      } else {
        (void)fifa96_match_entities_install(
            &mr->entities.team[(uint32_t)team].records[(uint32_t)index],
            (uint8_t)mr->state.phase, 5u, 0);
      }
    }
  }
  (void)fifa96_match_run_write_phase(mr, 2u);
  mr->live_out_pending = 0;
  mr->live_out_frames = 0;
  mr->live_out_kind = 0;
  mr->live_out_situation = 0;
  mr->goal_armed = 0;
  mr->goal_zone = 0;
  mr->situation_pending = 0;
  mr->situation_id = 0;
  (void)fifa96_match_act_init(&mr->act);
}

static int match_run_live_human_tackle(struct fifa96_match_run *mr, uint32_t t,
                                       int32_t human_id);

static void match_run_live_control_update(struct fifa96_match_run *mr) {
  int32_t t;
  int32_t id = FIFA96_MATCH_ENTITY_NONE;
  if (!mr || !mr->live_start) return;
  if (mr->state.phase == 2u) {
    match_run_live_setpiece_reset(mr);
    mr->live_halftime_pending = 0;
  }
  if (mr->live_halftime_pending && mr->state.phase != 2u) {
    if (++mr->live_halftime_frames >= 90u) {
      fifa96_match_run_period_over_rearm(mr);
      mr->live_halftime_frames = 0;
    }
  } else {
    mr->live_halftime_frames = 0;
  }
  /* Goal/second-half restart at phase 1 or the phase-0x0C kickoff flash: if
   * the PASS gate has not released it after the dwell, force phase 2 and let
   * the existing restart bind place the taker. This does not reset the half
   * clock (unlike the half-time period re-arm). */
  if (!mr->live_halftime_pending &&
      (mr->state.phase == 1u ||
       (mr->state.phase == 0x0Cu && mr->global_157ac2 < 2u))) {
    if (++mr->live_kickoff_frames >= 90u) {
      mr->live_restart_pending = 1u;
      (void)fifa96_match_run_write_phase(mr, 2u);
      mr->live_kickoff_frames = 0;
    }
  } else {
    mr->live_kickoff_frames = 0;
  }
  if (mr->live_out_pending) {
    if (mr->state.phase == 2u) {
      mr->live_out_pending = 0;
      mr->live_out_frames = 0;
    } else if (++mr->live_out_frames >= MATCH_RUN_LIVE_RESTART_WAIT) {
      match_run_live_out_force(mr);
    }
  }
  if (mr->state.phase != 2u) {
    /* A staged phase (goal/celebration/restart) ends the opening latch and the
     * old taker's human authority; a phase-1 restart arms the re-bind below. */
    if (mr->state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE)
      mr->live_restart_pending = 1u;
    mr->live_kickoff_touch = 0u;
    mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
    mr->live_kick_shot = 0u;
    match_run_live_support_reset(mr);
    match_run_live_manual_reset(mr);
    match_run_live_tackle_reset(mr);
    match_run_live_aim_reset(mr, 1);
    mr->live_save_hold = 0u;
    mr->live_save_record = FIFA96_MATCH_ENTITY_NONE;
    /* Phase 78 [D]: the free-kick/penalty taker owns the aim/confirm window
     * after the ordinary staged-phase cleanup; every other staged phase drops
     * the window. */
    if (mr->state.phase == 6u || mr->state.phase == 7u)
      match_run_live_setpiece_update(mr);
    else
      match_run_live_setpiece_reset(mr);
    return;
  }
  if (mr->live_restart_pending != 0u) {
    mr->live_restart_pending = 0u;
    /* A restart is a fresh possession phase: drop the opening/post-kick hold
     * and any stale flight so the contact tackle/claim is live again. */
    mr->live_kick_hold = 0u;
    mr->live_kick_active = 0u;
    mr->live_kick_shot = 0u;
    match_run_live_aim_reset(mr, 1);
    match_run_live_setpiece_reset(mr);
    mr->live_save_hold = 0u;
    mr->live_save_record = FIFA96_MATCH_ENTITY_NONE;
    match_run_live_support_reset(mr);
    match_run_live_manual_reset(mr);
    match_run_live_tackle_reset(mr);
    match_run_live_restart_bind(mr);
  }
  match_run_live_aim_update(mr);
  match_run_live_kick_latch(mr);
  t = match_run_live_control_team(mr);
  if (t < 0) return;
  if (mr->live_human_tackle_cd != 0u) mr->live_human_tackle_cd--;
  if (mr->live_kickoff_touch != 0u) {
    int32_t taker = mr->live_control_taker;
    if (taker >= 0 &&
        taker < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS) &&
        (uint32_t)(taker / (int32_t)FIFA96_MATCH_ENTITY_RECORDS) == (uint32_t)t &&
        (taker % (int32_t)FIFA96_MATCH_ENTITY_RECORDS) != 0) {
      struct fifa96_match_entity *e =
          &mr->entities.team[(uint32_t)t]
               .records[(uint32_t)taker % FIFA96_MATCH_ENTITY_RECORDS];
      if ((mr->dispatched_ok & (1ull << 7u)) != 0ull ||
          (mr->entities.ball.pair.actor == taker &&
           (mr->entities.ball.pair.flags & 0x70u) != 0u && e->code != 5u)) {
        /* The first kick fired (the FU-137 row-07 dispatch is the persistent
         * live signal; the pair can be cleared by the row's tail). Release the
         * opening possession and fall into the normal per-frame follow. */
        mr->live_kickoff_touch = 0u;
        mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
        mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
        mr->live_kick_hold = MATCH_RUN_LIVE_CONTROL_HOLD_FRAMES;
      } else {
        /* Keep the seeded taker as the carrier while the opening kick is
         * pending (the ball machine's camera-relative drift is not possession). */
        mr->entities.ball.carrier = taker;
        if (match_run_live_control_bind(mr, taker, 0) == 1 && e->code != 5u &&
            e->code != 7u)
          (void)fifa96_match_entities_install(e, (uint8_t)mr->state.phase, 5u, 0);
        return;
      }
    } else {
      mr->live_kickoff_touch = 0u;
      mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
    }
  }
  {
    int32_t carrier = mr->entities.ball.carrier;
    if (carrier >= 0 &&
        carrier < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS) &&
        (uint32_t)(carrier / (int32_t)FIFA96_MATCH_ENTITY_RECORDS) == (uint32_t)t &&
        (carrier % (int32_t)FIFA96_MATCH_ENTITY_RECORDS) != 0) {
      struct fifa96_match_entity *e =
          &mr->entities.team[(uint32_t)t]
               .records[(uint32_t)carrier % FIFA96_MATCH_ENTITY_RECORDS];
      if (e->skip_98 == 0 && e->skip_9a == 0) {
        if (mr->entities.ball.pair.actor == carrier &&
            (mr->entities.ball.pair.flags & 0x70u) != 0u && e->code != 5u) {
          /* The kick pair is live and the record left the carrier state: the
           * ball is in flight, not held. */
          mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
          mr->live_kick_hold = MATCH_RUN_LIVE_CONTROL_HOLD_FRAMES;
        } else if (match_run_live_control_near(mr, e)) {
          id = carrier;
        }
      }
    }
  }
  if (mr->live_kick_hold != 0u) mr->live_kick_hold--;
  if (id == FIFA96_MATCH_ENTITY_NONE) {
    int index = fifa96_match_entities_team_select(&mr->entities, (uint32_t)t,
                                                  (int16_t)mr->entities.ball.x,
                                                  (int16_t)mr->entities.ball.z);
    if (index >= 0)
      id = (int32_t)((uint32_t)t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)index);
  }
  /* Phase 76 [D]: manual switch. It yields to an own ball carrier; otherwise
   * a queued edge cycles from the currently controlled record and holds the
   * selection for 90 granted frames. */
  int carrier_selected = id != FIFA96_MATCH_ENTITY_NONE &&
                         id == mr->entities.ball.carrier;
  if (carrier_selected) {
    const struct fifa96_match_entity *ce =
        &mr->entities.team[(uint32_t)t]
             .records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
    /* A stale/far possession cell is not the live carrier (the same range gate
     * the carrier block uses); let the manual switch run in that case. */
    if (!match_run_live_control_near(mr, ce)) carrier_selected = 0;
  }
  if (carrier_selected) {
    match_run_live_manual_reset(mr);
  } else {
    if (mr->live_switch_press != 0u) {
      mr->live_switch_press = 0u;
      int32_t base = mr->entities.controlled;
      if (!match_run_live_control_switchable(mr, (uint32_t)t, base)) base = id;
      if (!match_run_live_control_switchable(mr, (uint32_t)t, base))
        base = mr->entities.team[t].target;
      int32_t next = match_run_live_control_switch_next(mr, (uint32_t)t, base);
      if (next != FIFA96_MATCH_ENTITY_NONE) {
        mr->live_manual_id = next;
        mr->live_manual_frames = MATCH_RUN_LIVE_MANUAL_FRAMES;
      }
    }
    if (mr->live_manual_frames != 0u) {
      if (match_run_live_control_switchable(mr, (uint32_t)t, mr->live_manual_id)) {
        id = mr->live_manual_id;
        if (--mr->live_manual_frames == 0u) mr->live_manual_id = FIFA96_MATCH_ENTITY_NONE;
      } else {
        match_run_live_manual_reset(mr);
      }
    }
  }
  if (mr->live_tackle_press != 0u) {
    mr->live_tackle_press = 0u;
    int32_t human = match_run_live_control_switchable(mr, (uint32_t)t,
                                                      mr->entities.controlled)
                        ? mr->entities.controlled
                        : id;
    if (human != FIFA96_MATCH_ENTITY_NONE &&
        match_run_live_human_tackle(mr, (uint32_t)t, human) != 0)
      return;   /* a referee decision owns the phase/restart now */
  }
  if (id != FIFA96_MATCH_ENTITY_NONE) {
    struct fifa96_match_entity *e =
        &mr->entities.team[(uint32_t)t]
             .records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
    /* The live possession claim (the FU-295 stand-in): when the ball is loose
     * and the controlled side's nearest outfield record is at least as close
     * to the ball as the opponent's nearest, the human takes possession. The
     * native row-05 claim is the unported producer; the opponent block is only
     * read (its records are never written). */
    if (mr->live_kick_hold == 0u && mr->live_kick_active == 0u &&
        (mr->entities.ball.carrier == FIFA96_MATCH_ENTITY_NONE ||
         mr->live_ai_tackle_cd == 0u) &&
        mr->entities.ball.carrier != id) {
      int16_t own_distance = 0;
      int16_t opp_distance = 0;
      int own_index = match_run_live_control_nearest(mr, (uint32_t)t, &own_distance);
      int opp_index = match_run_live_control_nearest(mr, (uint32_t)(1u - (uint32_t)t),
                                                     &opp_distance);
      (void)opp_index;
      if (own_index >= 0 && own_distance <= MATCH_RUN_LIVE_CONTROL_RANGE &&
          (opp_index < 0 || own_distance < opp_distance) &&
          (int32_t)((uint32_t)t * FIFA96_MATCH_ENTITY_RECORDS +
                    (uint32_t)own_index) == id)
        mr->entities.ball.carrier = id;
    }
    /* P7-L2:: the contact tackle/claim for a static opponent restart carrier. */
    if (mr->entities.ball.carrier != id)
      (void)match_run_live_control_tackle(mr, (uint32_t)t, id, e);
    int bind_rc = match_run_live_control_bind(mr, id, 0);
    if (bind_rc != 1 && id == mr->live_manual_id) {
      mr->live_manual_id = FIFA96_MATCH_ENTITY_NONE;
      mr->live_manual_frames = 0;
    }
    if (bind_rc == 1 &&
        mr->entities.ball.carrier == id && e->code != 5u && e->code != 7u)
      (void)fifa96_match_entities_install(e, (uint8_t)mr->state.phase, 5u, 0);
  }
}

/* P8 / live opponent contest (derived [D], live_start only).
 *
 * The native pre-pass `FUN_0008D8EC` re-selects a team target only while
 * `[team+0x7B2] == 0 || [0x157A83] == 0` (fresh gate bytes `0x8D932 CMP
 * dword[EBP+0x7B2],0` / `0x8D93B CMP dword[0x157A83],0`), and the native
 * clears that re-open the gate live in the unported row bodies /
 * `FUN_0007F7E0` (P9). The port therefore keeps the kickoff target forever and
 * only that one outfield record moves; row 03 (the hold/clamp body) is NULL, so
 * the rest sit at code 3. The live repair re-runs the native nearest select at
 * the native **third template** anchor (`0x8D948..0x8D9B7`: template 2 =
 * `0x157770` + `dword[0x1577BE]>>16<<5` / `dword[0x1577C0]>>16<<5`; the port
 * packs those live lead words as `camera.speed`/`camera.vel_x`/`camera.vel_z`,
 * see `match_run_entity_frame`). With the FU-300 unpacking that anchor is
 * `camera.pos + camera.vel*32`, and the select is the same ported
 * `fifa96_match_entities_team_select` (`0x8D9B2 CALL 0x8DE8C`, nearest with
 * skip 0) that `team_select_target` would call. Only the opponent team block is
 * re-targeted (never the human's selected record). A hysteresis
 * (`MATCH_RUN_LIVE_OPPONENT_RETARGET`) keeps a target that is already close to
 * the anchor, so the old code-5 record is not abandoned every frame (P8-L5).
 *
 * The second half is the [D] far-carrier repair (P8-L2): row 05 maps the native
 * possession block `[0x158724]` onto `ball.carrier` with no distance gate
 * (`0x7F1BF..0x7F205`); the probe measured a target record claiming at ~1127
 * units. A live opponent carrier farther than the contact range from the ball
 * is cleared, so it can only hold possession at contact. This writes only the
 * ball possession cell, never a record, score, phase or goal cell. */
#define MATCH_RUN_LIVE_OPPONENT_RETARGET 0x400

/* Phase 70 — the live contact decision [D].
 *
 * The derived opponent tackle is the one live contact producer that bypassed
 * the native referee chain. It now registers the contact through the ported
 * `fifa96_match_run_contact` (the native registrar + row-0x0C re-call /
 * decision shape) before the possession swap. The contact point is the
 * challenged record's position (the native row-0x0B arm-1 point,
 * `0x81BC3 LEA ECX,[ESI+0x59]`); the decision kind is the native duel kind 2.
 * Returns 1 when the decision ran, 0 when the settings/skip gate left the
 * contact unresolved and the tackle should still win the ball. */
static int match_run_live_contact_tackle(struct fifa96_match_run *mr,
                                         int32_t tackler, int32_t carrier) {
  const struct fifa96_match_entity *victim;
  int32_t point[3];
  if (!mr || !mr->live_start || mr->state.phase != 2u) return 0;
  if (tackler < 0 || carrier < 0) return 0;
  if (mr->act.id != 0u) return 0;                  /* a stoppage is running */
  if (mr->referee.sequence != FIFA96_REF_SEQ_NONE) return 0;
  if (mr->ball_settle_active != 0u) return 0;      /* a restart is settling */
  victim = &mr->entities.team[(uint32_t)carrier / FIFA96_MATCH_ENTITY_RECORDS]
               .records[(uint32_t)carrier % FIFA96_MATCH_ENTITY_RECORDS];
  point[0] = victim->pos_x;
  point[1] = victim->pos_y;
  point[2] = victim->pos_z;
  return fifa96_match_run_contact(mr, 2u, tackler, carrier, point) > 0;
}

/* Phase 77 [D]: a deliberate human contact attempt. The human must be within
 * the same contact range as the AI duel; the contact/referee seam decides a
 * foul, otherwise the clean challenge takes possession. Returns 1 when a
 * referee decision ran (the caller must not also perform the normal bind),
 * 0 when no decision ran. */
#define MATCH_RUN_LIVE_HUMAN_TACKLE_RANGE 0x40
#define MATCH_RUN_LIVE_HUMAN_TACKLE_CD 45u

static int match_run_live_human_tackle(struct fifa96_match_run *mr, uint32_t t,
                                       int32_t human_id) {
  int32_t carrier;
  int32_t dx;
  int32_t dz;
  if (!mr || !mr->live_start || mr->state.phase != 2u) return 0;
  if (mr->live_human_tackle_cd != 0u) return 0;
  if (human_id < 0 ||
      human_id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  if ((uint32_t)human_id / FIFA96_MATCH_ENTITY_RECORDS != t) return 0;
  carrier = mr->entities.ball.carrier;
  if (carrier < 0 ||
      carrier >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  if ((uint32_t)carrier / FIFA96_MATCH_ENTITY_RECORDS == t) return 0;
  {
    const struct fifa96_match_entity *h =
        &mr->entities.team[t].records[(uint32_t)human_id % FIFA96_MATCH_ENTITY_RECORDS];
    const struct fifa96_match_entity *c =
        &mr->entities.team[(uint32_t)carrier / FIFA96_MATCH_ENTITY_RECORDS]
             .records[(uint32_t)carrier % FIFA96_MATCH_ENTITY_RECORDS];
    if (h->skip_98 != 0u || h->skip_9a != 0u || c->skip_98 != 0u ||
        c->skip_9a != 0u)
      return 0;
    dx = c->pos_x - h->pos_x;
    dz = c->pos_z - h->pos_z;
    if (fifa96_entity_distance(dx, dz) > MATCH_RUN_LIVE_HUMAN_TACKLE_RANGE)
      return 0;
  }
  if (match_run_live_contact_tackle(mr, human_id, carrier) != 0) {
    mr->live_human_tackle_cd = MATCH_RUN_LIVE_HUMAN_TACKLE_CD;
    return 1;
  }
  /* Clean challenge: no decision ran, so the human wins the ball. */
  {
    struct fifa96_match_entity *h =
        &mr->entities.team[t].records[(uint32_t)human_id % FIFA96_MATCH_ENTITY_RECORDS];
    mr->entities.ball.carrier = human_id;
    mr->entities.team[t].target = human_id;
    if (h->code != 5u && h->code != 7u)
      (void)fifa96_match_entities_install(h, (uint8_t)mr->state.phase, 5u, 0);
  }
  mr->live_human_tackle_cd = MATCH_RUN_LIVE_HUMAN_TACKLE_CD;
  return 0;
}

/* Phase 72 — the live off-ball team shape [D].
 *
 * Native row 03 (the off-ball target producer) is the `pl.fmt` cell-table
 * lookup at 0x6DE44/0x6DCC8, but its dispatch slot is NULL in the shared table
 * (wiring it moved the M2 tape at frame 29). The live path consumes the same
 * bound metadata directly: map the camera focus through 0x6D870, then apply
 * the per-record 0x6DCC8 direction pair to every idle code-3 outfield record
 * that is not the carrier / team target / second pick. This is a live-only
 * stand-in for the unported row-03 producer; it writes only targets. */
static void match_run_live_offball_shape(struct fifa96_match_run *mr) {
  int32_t cells[2];
  if (!mr || !mr->live_start || mr->state.phase != 2u) return;
  cells[0] = fifa96_match_pitch_cell(mr->render.camera.pos_x,
                                     mr->render.camera.pos_z, 0u);
  cells[1] = fifa96_match_pitch_cell(mr->render.camera.pos_x,
                                     mr->render.camera.pos_z, 1u);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    uint32_t side = mr->entities.team[t].side & 1u;
    int32_t base = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS);
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      struct fifa96_match_entity *e = &mr->entities.team[t].records[i];
      int32_t id = base + (int32_t)i;
      if (e->skip_98 != 0u || e->skip_9a != 0u) continue;
      if (id == mr->entities.ball.carrier) continue;
      if (id == mr->entities.team[t].target) continue;
      if (id == mr->entities.team[t].second) continue;
      if (e->code != 3u) continue;         /* the forced off-ball code */
      if (e->phase_meta == NULL) continue; /* unbound -> native gate */
      fifa96_match_phase_apply_cell(mr, e, cells[side]);
    }
  }
}

/* Phase 75 [D]: one possessing-team support runner on top of the off-ball
 * shape. This is deliberately not a port of FUN_0007F7E0 (FU251-L8): it picks
 * the most advanced unused forward (role 3), then midfielder (role 2), and
 * gives him a forward target ahead of the carrier. It writes only the runner's
 * target and leaves the carrier/team target/second/control cells alone. */
#define MATCH_RUN_LIVE_SUPPORT_DEPTH 0x600
#define MATCH_RUN_LIVE_SUPPORT_Z_LIMIT 0xB00
#define MATCH_RUN_LIVE_SUPPORT_X_LIMIT 0x760
#define MATCH_RUN_LIVE_SUPPORT_FRAMES 120u

static int match_run_live_support_eligible(const struct fifa96_match_run *mr,
                                           uint32_t t, int32_t id) {
  if (id < 0 || id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS *
                                FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  if ((uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS != t) return 0;
  uint32_t index = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  if (index == 0) return 0;   /* keeper */
  const struct fifa96_match_entity *e = &mr->entities.team[t].records[index];
  if (e->active == 0u || e->code != 3u) return 0;
  if (e->skip_98 != 0u || e->skip_9a != 0u || e->has_slot != 0u) return 0;
  if (id == mr->entities.ball.carrier) return 0;
  if (id == mr->entities.team[t].target) return 0;
  if (id == mr->entities.team[t].second) return 0;
  if (id == mr->entities.team[t].intercept) return 0;
  if (id == mr->entities.team[t].chosen) return 0;
  if (id == mr->entities.controlled) return 0;
  return 1;
}

static int32_t match_run_live_support_pick(const struct fifa96_match_run *mr,
                                           uint32_t t, int32_t dir) {
  uint8_t role[FIFA96_MATCH_ENTITY_RECORDS];
  fifa96_match_formation_block blocks[4];
  int have_layout = 0;
  memset(role, 0, sizeof role);
  uint8_t formation = mr->formation[t] <= 4u ? mr->formation[t] : 0u;
  if (fifa96_match_formation_layout(formation, blocks) == 4) {
    have_layout = 1;
    for (int b = 0; b < 4; b++) {
      unsigned count = blocks[b].count > 5u ? 5u : blocks[b].count;
      for (unsigned s = 0; s < count; s++) {
        uint8_t pitch = blocks[b].slots[s];
        if (pitch < FIFA96_MATCH_ENTITY_RECORDS) role[pitch] = blocks[b].role;
      }
    }
  }
  for (int want = 3; want >= 2; want--) {
    int32_t best = FIFA96_MATCH_ENTITY_NONE;
    int32_t best_progress = 0;
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      if (have_layout && role[i] != (uint8_t)want) continue;
      if (!have_layout && i < 5u) continue;
      int32_t id = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + i);
      if (!match_run_live_support_eligible(mr, t, id)) continue;
      int32_t progress = dir * mr->entities.team[t].records[i].pos_z;
      if (best == FIFA96_MATCH_ENTITY_NONE || progress > best_progress) {
        best = id;
        best_progress = progress;
      }
    }
    if (best != FIFA96_MATCH_ENTITY_NONE) return best;
  }
  /* Formation unseeded/role-less: keep the runner out of the back line. */
  for (uint32_t i = 5; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    int32_t id = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + i);
    if (match_run_live_support_eligible(mr, t, id)) return id;
  }
  return FIFA96_MATCH_ENTITY_NONE;
}

static void match_run_live_support_run(struct fifa96_match_run *mr) {
  int32_t carrier;
  int32_t runner;
  int32_t dir;
  uint32_t t;
  if (!mr || !mr->live_start || mr->state.phase != 2u) {
    match_run_live_support_reset(mr);
    return;
  }
  carrier = mr->entities.ball.carrier;
  if (carrier < 0 ||
      carrier >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS *
                           FIFA96_MATCH_ENTITY_RECORDS) ||
      mr->live_kickoff_touch != 0u || mr->live_kick_active != 0u ||
      mr->live_kick_hold != 0u || mr->live_save_hold != 0u ||
      mr->ball_settle_active != 0u || mr->act.id != 0u) {
    match_run_live_support_reset(mr);
    return;
  }
  t = (uint32_t)carrier / FIFA96_MATCH_ENTITY_RECORDS;
  dir = (mr->entities.team[t].side & 1u) == 0u ? 1 : -1;
  runner = mr->live_support_runner;
  if (!match_run_live_support_eligible(mr, t, runner) ||
      mr->live_support_timer >= MATCH_RUN_LIVE_SUPPORT_FRAMES) {
    match_run_live_support_reset(mr);
    runner = match_run_live_support_pick(mr, t, dir);
    mr->live_support_runner = runner;
    mr->live_support_timer = 0;
  }
  if (runner == FIFA96_MATCH_ENTITY_NONE) return;
  if (mr->live_support_timer < MATCH_RUN_LIVE_SUPPORT_FRAMES)
    mr->live_support_timer++;
  struct fifa96_match_entity *e =
      &mr->entities.team[t].records[(uint32_t)runner % FIFA96_MATCH_ENTITY_RECORDS];
  const struct fifa96_match_entity *c =
      &mr->entities.team[t].records[(uint32_t)carrier % FIFA96_MATCH_ENTITY_RECORDS];
  int32_t front = dir * e->pos_z;
  int32_t want = dir * c->pos_z + MATCH_RUN_LIVE_SUPPORT_DEPTH;
  if (want < front) want = front;
  if (want > MATCH_RUN_LIVE_SUPPORT_Z_LIMIT) want = MATCH_RUN_LIVE_SUPPORT_Z_LIMIT;
  if (want < -MATCH_RUN_LIVE_SUPPORT_Z_LIMIT) want = -MATCH_RUN_LIVE_SUPPORT_Z_LIMIT;
  e->target_z = dir * want;
  e->target_x = e->pos_x;
  if (e->target_x > MATCH_RUN_LIVE_SUPPORT_X_LIMIT)
    e->target_x = MATCH_RUN_LIVE_SUPPORT_X_LIMIT;
  if (e->target_x < -MATCH_RUN_LIVE_SUPPORT_X_LIMIT)
    e->target_x = -MATCH_RUN_LIVE_SUPPORT_X_LIMIT;
}

static void match_run_live_opponent_update(struct fifa96_match_run *mr) {
  int32_t human;
  uint32_t opp;
  int32_t anchor_x;
  int32_t anchor_z;
  int32_t target;
  int32_t base;
  if (!mr || !mr->live_start || mr->state.phase != 2u) return;
  if (mr->live_ai_tackle_cd != 0u) mr->live_ai_tackle_cd--;
  human = match_run_live_control_team(mr);
  if (human < 0) return;
  opp = 1u - (uint32_t)human;
  base = (int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS);
  anchor_x = mr->render.camera.pos_x +
             (int32_t)(int16_t)mr->render.camera.vel_x * 32;
  anchor_z = mr->render.camera.pos_z +
             (int32_t)(int16_t)mr->render.camera.vel_z * 32;
  target = mr->entities.team[opp].target;
  if (target < base ||
      target >= base + (int32_t)FIFA96_MATCH_ENTITY_RECORDS) {
    (void)fifa96_match_entities_team_select(&mr->entities, opp,
                                            (int16_t)anchor_x,
                                            (int16_t)anchor_z);
  } else {
    const struct fifa96_match_entity *e =
        &mr->entities.team[opp].records[(uint32_t)(target - base)];
    int32_t dx = e->pos_x - anchor_x;
    int32_t dz = e->pos_z - anchor_z;
    if (fifa96_entity_distance(dx, dz) > MATCH_RUN_LIVE_OPPONENT_RETARGET)
      (void)fifa96_match_entities_team_select(&mr->entities, opp,
                                              (int16_t)anchor_x,
                                              (int16_t)anchor_z);
  }
  {
    int32_t carrier = mr->entities.ball.carrier;
    if (carrier >= base &&
        carrier < base + (int32_t)FIFA96_MATCH_ENTITY_RECORDS) {
      const struct fifa96_match_entity *ce =
          &mr->entities.team[opp].records[(uint32_t)(carrier - base)];
      int32_t dx = mr->entities.ball.x - ce->pos_x;
      int32_t dz = mr->entities.ball.z - ce->pos_z;
      if (fifa96_entity_distance(dx, dz) >
          FIFA96_MATCH_LIVE_OPPONENT_CLAIM_RANGE)
        mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
    }
  }
  /* P14 derived opponent contact tackle [D], Phase 70: when the human is
   * carrying a stationary ball and an opponent outfield record is in contact,
   * the native contact chain decides first. A decision (foul/restart) keeps
   * the human's ball and starts the ported referee sequence; only a clean
   * contact hands possession to the defender. A cooldown keeps one tackle
   * from chaining every frame. */
  {
    int32_t hcarrier = mr->entities.ball.carrier;
    if (hcarrier >= (int32_t)((uint32_t)human * FIFA96_MATCH_ENTITY_RECORDS) &&
        hcarrier < (int32_t)(((uint32_t)human + 1u) * FIFA96_MATCH_ENTITY_RECORDS) &&
        mr->live_kick_active == 0u && mr->live_kick_hold == 0u &&
        mr->live_save_hold == 0u && mr->entities.ball.speed <= 4 &&
        mr->live_ai_tackle_cd == 0u) {
      int32_t best = FIFA96_MATCH_ENTITY_NONE;
      int32_t best_d = 0x7FFF;
      for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
        const struct fifa96_match_entity *o = &mr->entities.team[opp].records[i];
        int32_t dx = o->pos_x - mr->entities.ball.x;
        int32_t dz = o->pos_z - mr->entities.ball.z;
        int32_t d;
        if (o->skip_98 != 0u || o->skip_9a != 0u) continue;
        d = fifa96_entity_distance(dx, dz);
        if (d < best_d) {
          best_d = d;
          best = (int32_t)(base + (int32_t)i);
        }
      }
      if (best != FIFA96_MATCH_ENTITY_NONE && best_d <= 0x30) {
        struct fifa96_match_entity *o =
            &mr->entities.team[opp].records[(uint32_t)(best - base)];
        if (match_run_live_contact_tackle(mr, best, hcarrier) != 0) {
          /* The referee decided: possession stays with the fouled carrier
           * until the restart places the taker. */
          mr->live_ai_tackle_cd = 45u;
          return;
        }
        mr->entities.ball.carrier = best;
        mr->entities.team[opp].target = best;
        if (o->code != 5u)
          (void)fifa96_match_entities_install(o, (uint8_t)mr->state.phase, 5u, 0);
        mr->live_ai_tackle_cd = 45u;
      }
    }
  }
}

/* P14 derived CPU carrier action [D]. The native AI chooser is unported; after
 * a short hold, pass to a forward teammate or shoot at the human goal through
 * the existing flight/ball-machine primitive. The keeper/shot/out-of-play
 * consumers remain native. Live-only. */
static void match_run_live_cpu_update(struct fifa96_match_run *mr) {
  int32_t human;
  uint32_t opp;
  int32_t base;
  int32_t carrier;
  int32_t dir;
  int32_t target_x;
  int32_t target_z;
  int near_goal;
  int shoot;
  if (!mr || !mr->live_start || mr->state.phase != 2u) return;
  if (mr->live_kick_active != 0u || mr->live_kick_hold != 0u ||
      mr->live_save_hold != 0u)
    return;
  human = match_run_live_control_team(mr);
  if (human < 0) return;
  opp = 1u - (uint32_t)human;
  base = (int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS);
  carrier = mr->entities.ball.carrier;
  if (carrier < base || carrier >= base + (int32_t)FIFA96_MATCH_ENTITY_RECORDS) {
    mr->live_cpu_hold = 0;
    mr->live_cpu_carrier = FIFA96_MATCH_ENTITY_NONE;
    return;
  }
  if (carrier != mr->live_cpu_carrier) {
    mr->live_cpu_carrier = carrier;
    mr->live_cpu_hold = 0;
  }
  if (mr->live_cpu_hold < 30u) {
    mr->live_cpu_hold++;
    return;
  }
  dir = mr->entities.team[opp].side == 0u ? 1 : -1;
  near_goal = dir > 0 ? mr->entities.ball.z > 0x700
                      : mr->entities.ball.z < -0x700;
  shoot = near_goal || (mr->live_cpu_actions % 3u) == 2u;
  if (shoot) {
    target_x = (mr->live_cpu_actions & 1u) != 0u ? 0x60 : -0x60;
    target_z = dir * 0xB30;
    match_run_live_cpu_kick(mr, target_x, target_z, 12, 600, 1);
  } else {
    int32_t best = -1;
    int32_t best_progress = dir * mr->entities.ball.z;
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      const struct fifa96_match_entity *e = &mr->entities.team[opp].records[i];
      int32_t progress;
      if (e->skip_98 != 0u || e->skip_9a != 0u) continue;
      if ((int32_t)(base + (int32_t)i) == carrier) continue;
      progress = dir * e->pos_z;
      if (progress > best_progress) {
        best_progress = progress;
        best = (int32_t)(base + (int32_t)i);
      }
    }
    if (best >= 0) {
      const struct fifa96_match_entity *e =
          &mr->entities.team[opp].records[(uint32_t)(best - base)];
      target_x = e->pos_x;
      target_z = e->pos_z + dir * 0x80;
    } else {
      target_x = mr->entities.ball.x;
      target_z = mr->entities.ball.z + dir * 0x300;
    }
    match_run_live_cpu_kick(mr, target_x, target_z, 10, 300, 0);
  }
  mr->live_cpu_actions++;
  mr->live_cpu_hold = 0;
  mr->live_cpu_carrier = FIFA96_MATCH_ENTITY_NONE;
}

/* P8/P9 / live save application (derived [D]): shared by the outfield shot
 * block and the keeper save. Clears the live flight and the FU-73 launch pair,
 * drops the consumed signature/pair marker so an identical repeat can latch,
 * pins the ball for the hold frames through the ball-machine hold branch,
 * records the defending pool id and gives it the ball possession cell. It
 * writes no score/phase/goal cell; the native armer/scan keep that authority,
 * and the hold is a pin, not a possession install on the record. */
static void match_run_live_save_apply(struct fifa96_match_run *mr, int32_t record) {
  mr->live_kick_active = 0u;
  mr->live_kick_frames = 0u;
  mr->live_kick_shot = 0u;
  mr->live_shot_pending = 0u;
  match_run_live_aim_reset(mr, 0);     /* P9-fix2: no stale aim */
  match_run_live_kick_sig_clear(mr);
  match_run_live_kick_consumed_clear(mr);
  mr->live_save_hold = FIFA96_MATCH_LIVE_SAVE_HOLD;
  mr->live_save_record = record;
  (void)fifa96_ball_pair_clear(&mr->entities.ball.pair);
  mr->entities.ball.carrier = record;
  mr->entities.ball.target_x = mr->entities.ball.x;
  mr->entities.ball.target_y = mr->entities.ball.y;
  mr->entities.ball.target_z = mr->entities.ball.z;
  mr->entities.ball.vel_x = 0;
  mr->entities.ball.vel_z = 0;
  mr->entities.ball.speed = 0;
}

/* P8 / live shot block (derived [D], live_start only; P8-L3/L4).
 *
 * No native open-play save exists: the native goal scan `FUN_00088940` and mouth
 * classifier `FUN_00070074` are record-free (recon §2), and the only keeper dive
 * producer is the phase-6 penalty arm (`0x8D558`, `match_run_phase_arm`). This
 * live stand-in runs immediately before `match_run_live_goal_arm` (so it sees
 * the same frame-start shot the armer would) and, while a live KICK flight is
 * in its final approach zone (`|ball.z| >= FIFA96_MATCH_LIVE_SAVE_ZONE`), finds
 * the nearest defending outfield record (1..10) within
 * `FIFA96_MATCH_LIVE_SAVE_RADIUS` of the ball. The keeper is excluded: its live
 * charge reaches ~0x1F from the shipped shot path: the 0x28 debug hit and a 0x18
 * windowed run both left the shipped centre shot unscored (P8-L3). The scan
 * skips the native +0x98/+0x9A hidden/occupied records, and a ball already in
 * the classifier's mouth is left to the native armer (a mouth block could
 * record a save and still concede). On a hit it clears the flight,
 * clears the FU-73 pair so the launch pair cannot re-latch, drops the consumed
 * kick signature and pair marker so an identical repeat can latch again, pins
 * the ball for
 * `FIFA96_MATCH_LIVE_SAVE_HOLD` frames through the ball-machine hold branch,
 * records the defending pool id and gives it the ball possession cell. It
 * writes no score/phase/goal cell; the native armer/scan keep that authority.
 *
 * Beatable by construction [D]: the shipped kickoff shot's closest defender is
 * the keeper at ~0x2F sampled (> 0x28), and the early rotating-defender
 * interception (~0x6B2) is outside the final zone, so the shipped path still
 * reaches the native goal. A staged/near defender inside the radius stops it.
 * The hold is a pin, not a possession install on the record (no code write). */
static int match_run_live_shot_block(struct fifa96_match_run *mr) {
  int32_t defending_side;
  int32_t team;
  int32_t best = -1;
  int16_t best_distance = 0;
  uint32_t magnitude;
  if (!mr || !mr->live_start) return 0;
  if (mr->state.phase != 2u || !mr->live_kick_active || mr->live_kick_shot == 0u ||
      mr->goal_armed)
    return 0;
  /* P8 (FU-346): the goal mouth belongs to the native armer/scan. A block on a
   * ball already inside `FUN_00070074`'s classifier would record a save and
   * then let the same frame's armer/scan score, so a blocked shot could still
   * reach the goal cells. Leave the mouth to the native chain. */
  if (fifa96_match_goal_zone(mr->entities.ball.x, mr->entities.ball.y,
                             mr->entities.ball.z))
    return 0;
  magnitude = (uint32_t)mr->entities.ball.z;
  if (mr->entities.ball.z < 0) magnitude = 0u - magnitude;
  if ((int16_t)(uint16_t)magnitude < FIFA96_MATCH_LIVE_SAVE_ZONE) return 0;
  /* The shot is aimed at the goal in `live_kick_target_z`'s direction: that
   * side is the defending team. */
  defending_side = mr->live_kick_target_z > 0 ? 1 : 0;
  team = match_run_live_team_for_side(mr, (uint8_t)defending_side);
  if (team < 0) return 0;
  /* Defender-only [D]: the live keeper charges the camera/ball (windowed
   * measurement: 0x1F from the ball at z 0x9D2, debug tick 467), so a
   * keeper-inclusive 0x28 block always stopped the shipped centre shot and made
   * the match unwinnable. A keeper-inclusive 0x18 variant also left a second
   * windowed run 0-0 (00:09). Outfield defenders 1..10 only; the keeper's
   * open-play save has no native producer (P8-L3). */
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e =
        &mr->entities.team[(uint32_t)team].records[i];
    int32_t dx;
    int32_t dz;
    int32_t d;
    /* The native nearest select (`FUN_0008DE8C`, the shared
     * `fifa96_entity_find_nearest`) excludes hidden/occupied records through
     * the +0x98/+0x9A bytes; an off-pitch record parked on the ball must not
     * block. */
    if (e->skip_98 != 0u || e->skip_9a != 0u) continue;
    dx = mr->entities.ball.x - e->pos_x;
    dz = mr->entities.ball.z - e->pos_z;
    d = fifa96_entity_distance(dx, dz);
    if (best < 0 || d < (int32_t)best_distance) {
      best = (int32_t)i;
      best_distance = (int16_t)d;
    }
  }
  if (best < 0 || best_distance > FIFA96_MATCH_LIVE_SAVE_RADIUS) return 0;
  match_run_live_save_apply(
      mr, (int32_t)((uint32_t)team * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)best));
  return 1;
}

/* P9 / live keeper save (derived [D], live_start only; P8-L3/P8-A3 stand-in).
 *
 * No native open-play save exists (P8 recon §2: `0x88940`/`0x70074` are
 * record-free; the only dive producer is the phase-6 penalty arm). This runs
 * immediately after the P8 outfield shot block and before
 * `match_run_live_goal_arm`, so a defender on the ball keeps the P8 authority
 * and a centre shot the outfielders miss is stopped by the defending keeper
 * (the non-controlled side's record 0). It is gated exactly like the shot
 * block (live phase-2 KICK flight, not armed, not already inside the native
 * mouth classifier) and additionally requires the latched target to be inside
 * the goal mouth (`|target_x| < FIFA96_MATCH_LIVE_GOAL_MOUTH_X`,
 * `|target_z| >= FIFA96_MATCH_LIVE_GOAL_MOUTH_Z`). The keeper
 * covers the shot when its own x is within
 * `FIFA96_MATCH_LIVE_KEEPER_SAVE_REACH` of the target x; a ±0x80 aim is
 * outside that band and beats it. The near-ball gate is the honesty guard for
 * the no-ISO/headless fixture, where the formation seed is absent and the
 * keeper starts at the origin: a keeper thousands of units from the ball must
 * not "save" from the halfway line. The shipped ISO keeper goal spot
 * `(0, ±0x9EA)` is the P8 AI recon (`/tmp/opencode/play/P8-ai-recon.md`)
 * finding-4 engine probe (the keeper tracks `(0,2706)` -> `(0,2538)` and the
 * centre shot passes at distance `0x2F`), not a static formation constant. It
 * writes no score/phase/goal cell. */
static int match_run_live_keeper_save(struct fifa96_match_run *mr) {
  int32_t defending_side;
  int32_t team;
  int32_t keeper;
  int32_t target_x;
  int32_t target_z;
  int32_t target_mag;
  int32_t ball_mag;
  int32_t lateral;
  const struct fifa96_match_entity *e;
  int32_t dx;
  int32_t dz;
  if (!mr || !mr->live_start) return 0;
  if (mr->state.phase != 2u || !mr->live_kick_active || mr->live_kick_shot == 0u ||
      mr->goal_armed)
    return 0;
  /* The native mouth classifier owns a ball already on the goal line (same
   * P8 mouth-edge rule: a mouth save could record and still concede). */
  if (fifa96_match_goal_zone(mr->entities.ball.x, mr->entities.ball.y,
                             mr->entities.ball.z))
    return 0;
  target_x = mr->live_kick_target_x;
  target_z = mr->live_kick_target_z;
  target_mag = target_z < 0 ? -target_z : target_z;
  if (target_mag < FIFA96_MATCH_LIVE_GOAL_MOUTH_Z)
    return 0;                                          /* not a goal-mouth shot */
  if (target_x <= -FIFA96_MATCH_LIVE_GOAL_MOUTH_X ||
      target_x >= FIFA96_MATCH_LIVE_GOAL_MOUTH_X)
    return 0;                                          /* outside the mouth */
  if (target_z > 0 ? mr->entities.ball.z <= 0 : mr->entities.ball.z >= 0)
    return 0;                                          /* ball must head goal-side */
  ball_mag = mr->entities.ball.z < 0 ? -mr->entities.ball.z : mr->entities.ball.z;
  if (ball_mag < FIFA96_MATCH_LIVE_KEEPER_SAVE_ZONE) return 0;
  /* The shot is aimed at the goal in `live_kick_target_z`'s direction: that
   * side is the defending team (the non-controlled side for a human KICK). */
  defending_side = target_z > 0 ? 1 : 0;
  team = match_run_live_team_for_side(mr, (uint8_t)defending_side);
  if (team < 0) return 0;
  keeper = (int32_t)((uint32_t)team * FIFA96_MATCH_ENTITY_RECORDS);
  e = &mr->entities.team[(uint32_t)team].records[0];
  /* The native nearest/visibility convention (+0x98/+0x9A): a hidden keeper
   * does not cover the shot. */
  if (e->skip_98 != 0u || e->skip_9a != 0u) return 0;
  /* Goal-side gate (P9-fix2): a keeper whose z is already past the ball on
   * the goal side cannot save it. For a +z goal the keeper must be at
   * pos_z >= ball.z; for -z, pos_z <= ball.z. Inclusive equality still allows
   * a smother at the keeper's feet. Without this a close-range shot from
   * inside the box (ball already behind the keeper) was pinned. */
  if (target_z > 0 ? e->pos_z < mr->entities.ball.z
                   : e->pos_z > mr->entities.ball.z)
    return 0;
  lateral = e->pos_x - target_x;
  if (lateral < 0) lateral = -lateral;
  if (lateral > FIFA96_MATCH_LIVE_KEEPER_SAVE_REACH) return 0;
  dx = mr->entities.ball.x - e->pos_x;
  dz = mr->entities.ball.z - e->pos_z;
  if (fifa96_entity_distance(dx, dz) > FIFA96_MATCH_LIVE_KEEPER_SAVE_NEAR)
    return 0;
  match_run_live_save_apply(mr, keeper);
  return 1;
}

/* Release the Task 2 staging arena. Runs only on initialized runs: init must
 * accept uninitialized memory (tests memset 0xAA and re-init), so the owner
 * slot is assigned NULL there and never freed; begin/end/stage call this only
 * after init. A run whose render state was set without staging (owner NULL) is
 * left untouched. */
static void match_run_release_stage(struct fifa96_match_run *mr) {
  /* FU-343 (P1): the pitch arena is independent of stage_owner, so release it
   * before the stage_owner early return. */
  fifa96_pitch_release(&mr->render.pitch_art);
  if (!mr->stage_owner) return;
  free(mr->stage_owner);
  mr->stage_owner = NULL;
  mr->render.frames = NULL;
  mr->render.banks = NULL;
  mr->render.bank_count = 0;
  mr->render.ball_bank = FIFA96_MATCH_RUN_BALL_BANK_NONE;
  mr->render.ball_roll = 0;
  mr->render.sprite_data = NULL;
  mr->render.sprite_data_len = 0;
  mr->render.enabled = 0;
  mr->render.palette_ready = 0;
}

/* FU-199 §7.2 (M2 phase-18 T1): drop the OL-13 metadata — clear every record's
 * `phase_meta`/`phase_meta10`/`phase_meta14` (so a stale pointer never outlives
 * its owner) and free the owned `.fmt` family blobs. Idempotent; the pool must
 * be initialized (begin/end call it only after fifa96_match_entities_init). */
static void match_run_release_meta(struct fifa96_match_run *mr) {
  if (!mr) return;
  /* Per-team through the pool API (a NULL blob clears the pointers), not a
   * hand-walk of the pool's record array — the pool owns that layout. */
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    (void)fifa96_match_entities_bind_record_meta(&mr->entities, t, NULL, NULL, NULL);
  free(mr->meta_owner);
  mr->meta_owner = NULL;
}

/* P15: clear the derived substitution menu state. The roster/lineup arrays are
 * cleared separately by init/begin (the bridge refills them before staging). */
static void match_run_sub_menu_reset(struct fifa96_match_run *mr) {
  if (!mr) return;
  mr->sub_menu_open = 0;
  mr->sub_menu_side = 0;
  mr->sub_menu_stage = 0;
  mr->sub_menu_cursor = 0;
  mr->sub_menu_out = FIFA96_MATCH_RUN_SUB_NONE;
  mr->sub_menu_in = FIFA96_MATCH_RUN_SUB_NONE;
  mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_NONE;
}

static void match_run_sub_roster_reset(struct fifa96_match_run *mr) {
  if (!mr) return;
  memset(mr->roster_pos, 0, sizeof mr->roster_pos);
  memset(mr->roster_rating, 0, sizeof mr->roster_rating);
  memset(mr->roster_hair_code, '0', sizeof mr->roster_hair_code);
  memset(mr->roster_skin_code, '4', sizeof mr->roster_skin_code);
  memset(mr->roster_order, 0, sizeof mr->roster_order);
  memset(mr->roster_order_ok, 0, sizeof mr->roster_order_ok);
  memset(mr->sub_lineup, 0xFF, sizeof mr->sub_lineup);
  memset(mr->sub_bench_avail, 0, sizeof mr->sub_bench_avail);
  mr->sub_lineup_ready = 0;
  mr->sub_used[0] = 0;
  mr->sub_used[1] = 0;
  mr->sub_notice[0] = '\0';
  mr->sub_notice_ticks = 0;
  match_run_sub_menu_reset(mr);
}

void fifa96_match_run_init(struct fifa96_match_run *mr) {
  if (!mr) return;
  fifa96_match_lifecycle_init(&mr->lc);
  fifa96_match_pace_init(&mr->pace);
  fifa96_match_state_init(&mr->state);
  mr->score[0] = 0;
  mr->score[1] = 0;
  mr->score_last_side = -1;       /* C3-OL2 writer state (OL-87 defaults) */
  mr->score_tracked_side = -1;
  mr->score_max_diff = 0;
  mr->score_last_event = 0;
  mr->score_sound_device = 0;     /* T4/OL-89 gate cells (image [0x115FCC]) */
  mr->score_sound_midi = 0;       /* T4/OL-89 ([0x114A98]) */
  mr->score_display_event = 0;    /* T4/OL-89: nothing dispatched */
  /* M2 phase-9 T3: the tracked-side flags/mode and the input/range words (the
   * native BSS defaults; FUN_00078824/FUN_0001C9BC rebuild them per match). */
  mr->tracked_flag[0] = 0;
  mr->tracked_flag[1] = 0;
  mr->screen_record_mode = 0;
  memset(mr->input_cfg, 0, sizeof mr->input_cfg);
  mr->input_range[0] = 0;
  mr->input_range[1] = 0;
  mr->engine = NULL;
  mr->backend.register_callback = NULL;
  mr->backend.cancel_callback = NULL;
  mr->backend.teardown = NULL;
  mr->backend.post_exit = NULL;
  mr->backend.ctx = NULL;
  mr->ticks = 0;
  mr->steps = 0;
  mr->cam_trace = NULL;      /* FU313: the trace is off until begin opens it */
  mr->cam_trace_frame = 0;
  mr->running = 0;
  mr->paused = 0;            /* FU-343 P4: no pause/POST hold at rest */
  mr->post_timer = 0;
  mr->post_skip = 0;
  mr->stage_owner = NULL;
  mr->meta_owner = NULL;   /* FU-199 §7.2: no OL-13 metadata loaded yet */
  memset(&mr->record, 0, sizeof mr->record);
  memset(&mr->celebration, 0, sizeof mr->celebration); /* FU-184 row-0x14 scatter */
  (void)fifa96_match_entities_init(&mr->entities);
  (void)fifa96_match_phase_machine_init(&mr->phase_machine);
  memset(&mr->rng, 0, sizeof mr->rng);
  mr->global_10f358 = 0;
  mr->global_10f35c = 0;
  mr->global_10f364 = 0;
  mr->global_10f368 = 0;
  mr->global_157ac2 = 0;
  mr->present_side = 0;                /* FU-181 L5: fresh [0x1074A4] */
  mr->clock_saved_157a60 = 0;          /* FU-181 L5: fresh 0x157A60 saved Q8 */
  mr->deepest_keeper_157aa7 = 0;       /* 0x157AA7: no deepest keeper (BSS) */
  mr->flag_157aab = 0;                 /* 0x157AAB: fresh BSS byte */
  mr->camera_height_157a6d = 0;        /* 0x157A6D: fresh BSS word */
  mr->flag_157ab0 = 0;                 /* 0x157AB0: OL-72 default */
  mr->camera_override_157a6f = 0;      /* 0x157A6F: no override pointer */
  mr->camera_override_triple[0] = 0;
  mr->camera_override_triple[1] = 0;
  mr->camera_override_triple[2] = 0;
  mr->ball_gate_15b37a = 0;            /* FU-188: fresh [0x15B37A] BSS word */
  memset(mr->row17_stash_1587fc, 0, sizeof mr->row17_stash_1587fc); /* FU-181 */
  mr->global_157ac0 = 0;               /* FU-213 T1: no extra-time flag yet */
  mr->probe_a[0] = 0;                  /* 0x142FE0: ordinary setup zeroes the pair */
  mr->probe_a[1] = 0;
  mr->probe_b[0] = 0;                  /* 0x142FE8: same for the B pair */
  mr->probe_b[1] = 0;
  mr->end_word_158822 = 0;             /* FU-231 T2: no end word yet */
  mr->actor_1587d4 = FIFA96_MATCH_ENTITY_NONE; /* FU-210 L3: no sit-8 actor bind */
  mr->actor_1587cc[0] = FIFA96_MATCH_ENTITY_NONE; /* FU-217 T3: no sit-8 per-side bind */
  mr->actor_1587cc[1] = FIFA96_MATCH_ENTITY_NONE;
  mr->global_5882a = 0;
  /* M2 phase-15 T2 (FU-176/FU-179/FU-180): fresh keeper-cluster process
   * cells ([0x157A4C]/[0x157C5C]/[0x157C5D]/[0x157C54..5B]/the predictor
   * latches; producers unported, staged zero). */
  mr->global_157a4c = 0;
  mr->global_157c5c = 0;
  mr->global_157c5d = 0;
  memset(mr->keeper_latch, 0, sizeof mr->keeper_latch);
  mr->keeper_latch_157ab2 = 0;         /* FU296: the [0x157AB2] force latch */
  /* Phase-29 T2 self-review: the row-1E/1D keeper process cells the header
   * block at fifa96_match_run.h ("Init/begin zero them") promises were never
   * cleared — a fixture reading them before the first row-1B/1E write saw the
   * caller's stack. Zero the native BSS defaults. */
  mr->keeper_cam_x = 0;
  mr->keeper_cam_z = 0;
  mr->keeper_reset_x = 0;
  mr->keeper_reset_y = 0;
  mr->keeper_reset_z = 0;
  mr->keeper_vec_band = 0;
  mr->keeper_vec_dx = 0;
  mr->keeper_vec_dz = 0;
  mr->keeper_saved_x = 0;
  mr->keeper_saved_z = 0;
  mr->keeper_gauge = 0;
  mr->flag_157820 = 0;
  mr->flag_157822 = 0;
  mr->difficulty_14c2f6 = 0;           /* FU301: the image [0x14C2F6] default */
  /* FU300 T4: the row-0x1A process cells (the ladder stamp and the direction
   * grids arrive live from the camera; only the bounce cells stay staged). */
  mr->global_157c2c = 0;
  mr->bounce_15759f = 0;
  mr->bounce_1575cc = 0;
  mr->bounce_1575d0 = 0;
  mr->bounce_1575a7 = 0;       /* FU-71 §6 post-arm gate cells (Task 7; staged) */
  mr->bounce_15758d = 0;
  mr->flag_1575e8 = 0;
  mr->tracker_15759d = 0;      /* FU-71 §6 tracker-decision latch (Task 8) */
  mr->match1a_event_id = 0;
  mr->match1a_anim = 0;
  mr->match1d_event_id = 0;
  fifa96_keeper_cpu_init(&mr->keeper_cpu); /* FU296 Seam A: fresh machine view */
  mr->match09_seq_flag_15877c = 0;    /* FU-178 arm 1: [0x15877C] */
  mr->match09_event_id = 0xFF;        /* FU-178: no row-09 event yet */
  mr->match0c_event_id = 0xFF;        /* FU-169: no row-0C event yet */
  mr->process_15877d = 0;             /* FU142/OL-82: [0x15877D] */
  /* FU302 (phase-28 T7): the row-0x1B wide-path observables and the
   * `[0x157823]`/`[0x157ACC]`/`[0x157AD0]` sit cells. */
  mr->match1b_hypot = 0;
  mr->match1b_pan_x = 0;
  mr->match1b_pan_z = 0;
  mr->match1b_vec_mag = 0;
  mr->match1b_vec_x = 0;
  mr->match1b_vec_z = 0;
  mr->match1b_sign_match = 0;
  mr->match1b_event_id = 0xFFu;
  mr->match1b_stat_id = 0xFFu;
  mr->match1b_stat_tracked = 0;
  mr->match1b_sound_id = 0;
  mr->match1b_sit_code = 0;
  mr->match1b_sit_emit = 0;
  mr->flag_157823 = 0;
  mr->counter_157acc[0] = 0;
  mr->counter_157acc[1] = 0;
  mr->counter_157ad0[0] = 0;
  mr->counter_157ad0[1] = 0;
  mr->predict_c44 = 0;
  mr->predict_c48_x = 0;
  mr->predict_c48_y = 0;
  mr->predict_c48_z = 0;
  (void)fifa96_match_act_init(&mr->act); /* FU-175: fresh act slot */
  mr->act9_gate = 0;
  mr->phase_walk_rec = NULL;           /* FU-191: fresh per-record walk cursor */
  mr->phase_walk_edge = 0;
  mr->phase_walk_active = 0;
  mr->phase_dir_default[0] = 0;        /* [0x1577DE] producer unported */
  mr->phase_dir_default[1] = 0;
  mr->phase_cell_side[0] = 0;          /* [0x1577D6/D8] cell map (dead arm) */
  mr->phase_cell_side[1] = 0;
  mr->pass_parity = 0;
  mr->clock_period_ended = 0;          /* FU-143: no staged clock completion */
  mr->dispatched_ok = 0;
  /* FU-145 S2: fresh goal-arm/queue state (the session gate seeds 0 here and
   * 1 in begin: a live match). */
  mr->goal_armed = 0;
  mr->goal_counter_15781c = 0;
  mr->goal_zone = 0;
  mr->goal_snap_x = 0;
  mr->goal_snap_y = 0;
  mr->goal_snap_z = 0;
  mr->live_out_kind = 0;
  mr->live_out_pending = 0;
  mr->live_out_situation = 0;
  mr->live_out_side = 0;
  mr->live_out_frames = 0;
  mr->live_shot_pending = 0;
  mr->camera_arm_announce = 0;         /* FU-71 leg 9.6: no 0x974DC request */
  mr->goal_record = FIFA96_MATCH_ENTITY_NONE;   /* FU-173 §4.2: no scan yet */
  mr->goal_record_side = 0;
  mr->situation_id = 0;
  mr->situation_pending = 0;
  mr->session_gate_14c32a = 0;
  mr->ball_row_flag_158882 = 0;        /* the native reset clear 0x73D3A */
  match_run_ball_settle_clear(mr);     /* Phase 70: no staged settle */
  /* FU-149 P1: fresh set-piece dispatcher state (the match-reset seeds:
   * FUN_00073EE0 zeroes 0x157AD4/0x157AD6 and the act-8 replay bytes). */
  mr->sit_side_pending = 0;
  mr->corner_count[0] = 0;
  mr->corner_count[1] = 0;
  /* [0x157ABE] via FUN_0007417C; global_157ac2 cleared above */
  mr->side_swap = fifa96_match_side_swap_value(mr->global_157ac2, 0u);
  mr->store_15882b = 0;
  mr->store_15882c = 0;
  mr->incident_x = 0;
  mr->incident_y = 0;
  mr->incident_z = 0;
  /* FU-149 L13 (T1): the throw/corner staging cells (the native BSS defaults). */
  mr->sp_flag_158784 = 0;
  mr->sp_delivery = 0;
  mr->sp_triple_15879c[0] = 0;
  mr->sp_triple_15879c[1] = 0;
  mr->sp_triple_15879c[2] = 0;
  mr->sp_recv_1587ac = FIFA96_MATCH_ENTITY_NONE;
  mr->sp_b0_1587b0 = 0;
  mr->sp_b3_1587b3 = 0;
  mr->sp_b6_1587b6 = 0;
  mr->sp_b7_1587b7 = 0;
  memset(&mr->sp_target, 0, sizeof mr->sp_target);
  mr->sp_dir_15877b = 3;
  mr->sp_dir_15877f = 3;
  mr->sp_b5_1587b5 = 0;
  mr->sp_path[0][0] = mr->sp_path[0][1] = 0;
  mr->sp_path[1][0] = mr->sp_path[1][1] = 0;
  mr->sp_path[2][0] = mr->sp_path[2][1] = 0;
  mr->sp_path_valid = 0;
  mr->sp_clock_157a68 = 0;
  mr->sp_global_105fb0 = 0;
  /* FU274 §1.1: `byte[0x157821]` is the camera's `render.camera.pan_counter`
   * (cleared by reset_render's fifa96_camera_init), not a separate cell. */
  /* FU-196/FU-197 (M2 phase-17 T2) / FU-201 (phase-18 T2): the producer gates
   * default off and the act-4 formation state is the zero stand-in — the order
   * matrices are a staged leg (FU201-L4: no match-begin engine consumer; the
   * arm's trigger is a runtime non-permutation, FU201-L1). */
  mr->period_setup_live = 0;
  mr->live_start = 0;                  /* FU-343 P1: native begin order default */
  mr->match_mode = 0;                  /* P13: friendly until the front-end says otherwise */
  mr->live_kickoff_touch = 0;          /* P7: no live kickoff possession latch */
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  match_run_live_support_reset(mr);    /* Phase 75: no support runner */
  match_run_live_manual_reset(mr);     /* Phase 76: no manual switch */
  match_run_live_tackle_reset(mr);     /* Phase 77: no human contact edge */
  mr->live_switch_down = 0;
  mr->live_tackle_down = 0;
  mr->input_resync = 0;                /* C1: no replay-exit re-sync pending */
  mr->replay_audio_armed = 0;          /* C4: no replay-arm audio edge pending */
  mr->live_human_side = 0;
  mr->live_kick_hold = 0;              /* P7: no live possession hold */
  mr->live_kick_active = 0;            /* P7: no live ball flight */
  mr->live_kick_frames = 0;
  mr->live_kick_target_x = 0;
  mr->live_kick_target_z = 0;
  mr->live_kick_speed = 0;
  match_run_live_kick_sig_clear(mr);
  match_run_live_kick_consumed_clear(mr);
  mr->live_kick_shot = 0;              /* P8: no live shot flight */
  match_run_live_aim_reset(mr, 1);     /* P9: centre shot aim */
  match_run_live_setpiece_reset(mr);   /* Phase 78: no set-piece window */
  mr->live_save_hold = 0;              /* P8: no live save hold */
  mr->live_save_record = FIFA96_MATCH_ENTITY_NONE;
  mr->live_cpu_hold = 0;
  mr->live_cpu_actions = 0;
  mr->live_cpu_carrier = FIFA96_MATCH_ENTITY_NONE;
  mr->live_ai_tackle_cd = 0;
  memset(mr->roster_name, 0, sizeof mr->roster_name);
  memset(mr->roster_number, 0, sizeof mr->roster_number);
  match_run_sub_roster_reset(mr);
  fifa96_match_stats_reset(&mr->stats);   /* FU-72: no inherited activity */
  mr->stats_third_selection = 0;          /* FU-72 §2.3: no inherited [0x5B358] */
  mr->live_restart_pending = 0;
  mr->live_halftime_pending = 0;
  mr->live_halftime_frames = 0;
  mr->live_kickoff_frames = 0;
  mr->formation_order_ok = 0;
  memset(mr->formation_order, 0, sizeof mr->formation_order);
  memset(mr->formation_current, 0, sizeof mr->formation_current);
  memset(mr->roster_order, 0, sizeof mr->roster_order);
  memset(mr->roster_order_ok, 0, sizeof mr->roster_order_ok);
  mr->formation_mismatch[0] = 0;
  mr->formation_mismatch[1] = 0;
  mr->store_158820 = 0;
  mr->store_15881e = 0;
  /* FU-194 (M2 phase-17 T5): a fresh run has no act-8 sweep in flight. */
  mr->act8_team = 0;
  mr->act8_record = 0;
  mr->act8_selected = NULL;
  /* FU-148 S4: the native [0x14C1E4]/[0x14C1E5] BSS default is 0. */
  mr->formation[0] = 0;
  mr->formation[1] = 0;
  mr->formation_seeded = 0;
  /* P6: no Team Select handout is staged until set_matchup. */
  memset(&mr->matchup, 0, sizeof mr->matchup);
  mr->matchup_pending = 0;
  mr->matchup_active = 0;
  /* FU-150 P2: fresh referee state, machine dispatch and request slots; the
   * settings config is zero until begin installs the derived default handoff
   * (fouls disabled, offside disabled). */
  memset(&mr->referee, 0, sizeof mr->referee);
  memset(&mr->config, 0, sizeof mr->config);
  mr->ref_machine = FIFA96_MATCH_RUN_REF_NONE;
  mr->ref_restart_stage = 0;
  mr->ref_whistle = 0;
  mr->ref_speech = 0;
  mr->whistle_seq = 0;                 /* FU-343 P3 fix: no staged whistle */
  match_run_reset_screen_state(mr);    /* FU-146 S3: fresh consumer state */
  fifa96_match_run_reset_input(mr);
  fifa96_match_run_reset_render(mr);
  /* RR-L1/RR-L2: the recorded-frame ring is reset at init; the recorder gate
   * and the overlay latch are cleared explicitly (the run is not memset, and
   * the native reset does not write the gate). */
  fifa96_match_replay_ring_reset(mr);
  fifa96_match_replay_ring_record(mr, 0);
  mr->recorder_latch_14e574 = 0;
  /* RR-L10: the replay FIFO cursors and the staged drain screen id. */
  fifa96_match_run_replay_fifo_reset(mr);
  mr->replay_screen_id_105ffc = 0;
  /* RR-L3: the composer's band/flag/queue-pop cells start clear; the six-byte
   * queue lane defaults to the native `0xFF` fill (the object-array producer
   * is a numbered leg). */
  mr->replay_view_flags_155c4e = 0;
  mr->replay_queue_pop_155c4f = 0;
  mr->replay_band_155c50 = 0;
  mr->replay_band_flag_155c51 = 0;
  memset(mr->replay_queue6, 0xFF, sizeof mr->replay_queue6);
}

int fifa96_match_run_begin(struct fifa96_match_run *mr, struct fifa96_engine *eng,
                           uint32_t selector) {
  if (!mr || !eng) return -FIFA96_ERR_INVALID;
  if (!eng->booted || eng->mode == FIFA96_ENGINE_MODE_QUIT) return -FIFA96_ERR_STATE;
  if (mr->running) return -FIFA96_ERR_STATE;
  if (eng->match && eng->match != mr) return -FIFA96_ERR_STATE;
  if (!mr->backend.register_callback || !mr->backend.cancel_callback ||
      !mr->backend.teardown || !mr->backend.post_exit) {
    fifa96_match_run_install_defaults(mr);
  }
  mr->engine = eng;
  mr->ticks = 0;
  mr->steps = 0;
  mr->paused = 0;                      /* FU-343 P4: fresh pause/POST state */
  mr->post_timer = 0;
  mr->post_skip = 0;
  mr->live_kickoff_touch = 0;          /* P7: fresh live kickoff latch */
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  match_run_live_support_reset(mr);    /* Phase 75: fresh support runner */
  match_run_live_manual_reset(mr);     /* Phase 76: no manual switch */
  match_run_live_tackle_reset(mr);     /* Phase 77: no human contact edge */
  mr->live_switch_down = 0;
  mr->live_tackle_down = 0;
  mr->input_resync = 0;                /* C1: no replay-exit re-sync pending */
  mr->replay_audio_armed = 0;          /* C4: a fresh match starts unducked */
  mr->live_kick_hold = 0;              /* P7: fresh live possession hold */
  mr->live_kick_active = 0;            /* P7: fresh live ball flight */
  mr->live_kick_frames = 0;
  mr->live_kick_target_x = 0;
  mr->live_kick_target_z = 0;
  mr->live_kick_speed = 0;
  match_run_live_kick_sig_clear(mr);
  match_run_live_kick_consumed_clear(mr);
  mr->live_kick_shot = 0;              /* P8: fresh live shot marker */
  match_run_live_aim_reset(mr, 1);     /* P9: fresh centre shot aim */
  match_run_live_setpiece_reset(mr);   /* Phase 78: no set-piece window */
  mr->live_save_hold = 0;              /* P8: fresh live save hold */
  mr->live_save_record = FIFA96_MATCH_ENTITY_NONE;
  mr->live_cpu_hold = 0;
  mr->live_cpu_actions = 0;
  mr->live_cpu_carrier = FIFA96_MATCH_ENTITY_NONE;
  mr->live_ai_tackle_cd = 0;
  memset(mr->roster_name, 0, sizeof mr->roster_name);
  memset(mr->roster_number, 0, sizeof mr->roster_number);
  match_run_sub_roster_reset(mr);
  fifa96_match_run_stats_seed(mr);    /* FU-72: key by the replay roster id */
  mr->live_restart_pending = 0;
  mr->live_halftime_pending = 0;
  mr->live_halftime_frames = 0;
  mr->live_kickoff_frames = 0;
  fifa96_match_state_init(&mr->state); /* fresh match clock */
  mr->score[0] = 0;                    /* fresh match score pair */
  mr->score[1] = 0;
  mr->final_score[0] = 0;
  mr->final_score[1] = 0;
  mr->final_score_valid = 0;
  mr->result_pending = 0;
  mr->score_last_side = -1;            /* C3-OL2 fresh writer state (OL-87) */
  mr->score_tracked_side = -1;
  mr->score_max_diff = 0;
  mr->score_last_event = 0;
  mr->score_sound_device = 0;          /* T4/OL-89: fresh [0x115FCC] gate cell */
  mr->score_sound_midi = 0;            /* T4/OL-89: fresh [0x114A98] gate cell */
  mr->score_display_event = 0;         /* T4/OL-89: fresh dispatch boundary */
  memset(&mr->record, 0, sizeof mr->record); /* fresh FU-138/FU-140 record */
  memset(&mr->celebration, 0, sizeof mr->celebration); /* FU-184 row-0x14 scatter */
  (void)fifa96_match_entities_init(&mr->entities); /* fresh FU-141 pool */
  (void)fifa96_match_phase_machine_init(&mr->phase_machine); /* fresh FU-142a machine */
  /* FU-142d: a fresh match seeds the RNG (the native FUN_000493A0 match-init
   * seed call 0x493F2) with the derived seed 0 and clears the row-28 process
   * globals (OL-56). */
  (void)fifa96_rng_seed(&mr->rng, 0);
  mr->global_10f358 = 0;
  mr->global_10f35c = 0;
  mr->global_10f364 = 0;
  mr->global_10f368 = 0;
  mr->global_157ac2 = 0;
  mr->present_side = 0;                /* FU-181 L5: fresh [0x1074A4] */
  mr->clock_saved_157a60 = 0;          /* FU-181 L5: fresh 0x157A60 saved Q8 */
  mr->deepest_keeper_157aa7 = 0;       /* 0x157AA7: no deepest keeper (BSS) */
  mr->flag_157aab = 0;                 /* 0x157AAB: fresh BSS byte */
  mr->camera_height_157a6d = 0;        /* 0x157A6D: fresh BSS word */
  mr->flag_157ab0 = 0;                 /* 0x157AB0: OL-72 default */
  mr->camera_override_157a6f = 0;      /* 0x157A6F: no override pointer */
  mr->camera_override_triple[0] = 0;
  mr->camera_override_triple[1] = 0;
  mr->camera_override_triple[2] = 0;
  mr->ball_gate_15b37a = 0;            /* FU-188: fresh [0x15B37A] word */
  memset(mr->row17_stash_1587fc, 0, sizeof mr->row17_stash_1587fc); /* FU-181 */
  mr->global_157ac0 = 0;               /* FU-213 T1: fresh extra-time flag */
  mr->probe_a[0] = 0;                  /* 0x142FE0: fresh-match setup zeroes A */
  mr->probe_a[1] = 0;
  mr->probe_b[0] = 0;                  /* 0x142FE8: fresh-match setup zeroes B */
  mr->probe_b[1] = 0;
  mr->end_word_158822 = 0;             /* FU-231 T2: fresh end word */
  mr->actor_1587d4 = FIFA96_MATCH_ENTITY_NONE; /* FU-210 L3: no sit-8 actor bind */
  mr->actor_1587cc[0] = FIFA96_MATCH_ENTITY_NONE; /* FU-217 T3: no sit-8 per-side bind */
  mr->actor_1587cc[1] = FIFA96_MATCH_ENTITY_NONE;
  mr->global_5882a = 0;                /* fresh kickoff gate (OL-84 residual) */
  /* M2 phase-15 T2 (FU-176/FU-179/FU-180): fresh keeper-cluster cells. */
  mr->global_157a4c = 0;
  mr->global_157c5c = 0;
  mr->global_157c5d = 0;
  memset(mr->keeper_latch, 0, sizeof mr->keeper_latch);
  mr->keeper_latch_157ab2 = 0;         /* FU296: fresh [0x157AB2] force latch */
  /* Phase-29 T2 self-review: the row-1E/1D keeper process cells (the header's
   * "Init/begin zero them"); without this a fresh match inherits the previous
   * match's keeper band. */
  mr->keeper_cam_x = 0;
  mr->keeper_cam_z = 0;
  mr->keeper_reset_x = 0;
  mr->keeper_reset_y = 0;
  mr->keeper_reset_z = 0;
  mr->keeper_vec_band = 0;
  mr->keeper_vec_dx = 0;
  mr->keeper_vec_dz = 0;
  mr->keeper_saved_x = 0;
  mr->keeper_saved_z = 0;
  mr->keeper_gauge = 0;
  mr->flag_157820 = 0;
  mr->flag_157822 = 0;
  mr->difficulty_14c2f6 = 0;           /* FU301: fresh [0x14C2F6] default */
  /* FU300 T4: fresh row-0x1A process cells (the camera carries the direction
   * grid/leads; only the bounce cells are staged). */
  mr->global_157c2c = 0;
  mr->bounce_15759f = 0;
  mr->bounce_1575cc = 0;
  mr->bounce_1575d0 = 0;
  mr->bounce_1575a7 = 0;       /* FU-71 §6 post-arm gate cells (Task 7; staged) */
  mr->bounce_15758d = 0;
  mr->flag_1575e8 = 0;
  mr->tracker_15759d = 0;      /* FU-71 §6 tracker-decision latch (Task 8) */
  mr->match1a_event_id = 0;
  mr->match1a_anim = 0;
  mr->match1d_event_id = 0;
  fifa96_keeper_cpu_init(&mr->keeper_cpu); /* FU296 Seam A: fresh machine view */
  mr->match09_seq_flag_15877c = 0;
  mr->match09_event_id = 0xFF;
  mr->match0c_event_id = 0xFF;
  mr->process_15877d = 0;
  /* FU302 (phase-28 T7): fresh wide-path observables + sit cells. */
  mr->match1b_hypot = 0;
  mr->match1b_pan_x = 0;
  mr->match1b_pan_z = 0;
  mr->match1b_vec_mag = 0;
  mr->match1b_vec_x = 0;
  mr->match1b_vec_z = 0;
  mr->match1b_sign_match = 0;
  mr->match1b_event_id = 0xFFu;
  mr->match1b_stat_id = 0xFFu;
  mr->match1b_stat_tracked = 0;
  mr->match1b_sound_id = 0;
  mr->match1b_sit_code = 0;
  mr->match1b_sit_emit = 0;
  mr->flag_157823 = 0;
  mr->counter_157acc[0] = 0;
  mr->counter_157acc[1] = 0;
  mr->counter_157ad0[0] = 0;
  mr->counter_157ad0[1] = 0;

  mr->predict_c44 = 0;
  mr->predict_c48_x = 0;
  mr->predict_c48_y = 0;
  mr->predict_c48_z = 0;
  (void)fifa96_match_act_init(&mr->act); /* FU-175: fresh act slot */
  mr->act9_gate = 0;
  mr->phase_walk_rec = NULL;           /* FU-191: fresh per-record walk cursor */
  mr->phase_walk_edge = 0;
  mr->phase_walk_active = 0;
  mr->phase_dir_default[0] = 0;        /* [0x1577DE] producer unported */
  mr->phase_dir_default[1] = 0;
  mr->phase_cell_side[0] = 0;          /* [0x1577D6/D8] cell map (dead arm) */
  mr->phase_cell_side[1] = 0;
  mr->pass_parity = 0;                 /* FU-139 §11: fresh [0x157A4F] */
  mr->clock_period_ended = 0;          /* FU-143: fresh clock staging */
  mr->dispatched_ok = 0;               /* Task 15: fresh dispatch observation */
  /* FU-166 (phase-13 T3, R14): a fresh match drops the goal-arm/queue state
   * and seeds the session gate 0 ([0x14C32A]) — the settled value of every
   * ordinary played match (Friendly/Leagues/Tournaments/Playoffs: mode 0,
   * `FUN_00024B00` 0x24BEB `XOR EAX,EAX`). Byte 1 is the **Practise**
   * session only (`FUN_00032DE0` -> `FUN_0001B7B8(4)`), a different session
   * whose scheduler/queue family is natively dead in the regular match; the
   * earlier seed 1 was the disclosed composite (L14.8.d). Under the settled 0
   * the per-frame scheduler is off and the goal scan/queue takes the native
   * byte-0 direct arm (`0x8A957 JZ 0x8AA7B`), the value the FU-146/FU-155
   * polarity pair carries for the engine's live match type. */
  mr->goal_armed = 0;
  mr->goal_counter_15781c = 0;
  mr->goal_zone = 0;
  mr->goal_snap_x = 0;
  mr->goal_snap_y = 0;
  mr->goal_snap_z = 0;
  mr->live_out_kind = 0;
  mr->live_out_pending = 0;
  mr->live_out_situation = 0;
  mr->live_out_side = 0;
  mr->live_out_frames = 0;
  mr->live_shot_pending = 0;
  mr->camera_arm_announce = 0;         /* FU-71 leg 9.6: no 0x974DC request */
  mr->goal_record = FIFA96_MATCH_ENTITY_NONE;   /* FU-173 §4.2: no scan yet */
  mr->goal_record_side = 0;
  mr->situation_id = 0;
  mr->situation_pending = 0;
  mr->session_gate_14c32a = 0;
  mr->ball_row_flag_158882 = 0;        /* the native reset clear 0x73D3A */
  match_run_ball_settle_clear(mr);     /* Phase 70: no staged settle */
  /* FU-149 P1: a fresh match drops the set-piece dispatcher state (the native
   * match reset FUN_00073EE0 zeroes the corner counters 0x157AD4/0x157AD6;
   * the [0x157ABE] swap follows the modeled FUN_0007417C formula from the
   * reset global_157ac2 = 0, and the incident triple keeps its
   * unported-producer default). */
  mr->sit_side_pending = 0;
  mr->corner_count[0] = 0;
  mr->corner_count[1] = 0;
  mr->side_swap = fifa96_match_side_swap_value(mr->global_157ac2, 0u);
  mr->store_15882b = 0;
  mr->store_15882c = 0;
  mr->incident_x = 0;
  mr->incident_y = 0;
  mr->incident_z = 0;
  /* FU-149 L13 (T1): a fresh match drops the throw/corner staging cells. */
  mr->sp_flag_158784 = 0;
  mr->sp_delivery = 0;
  mr->sp_triple_15879c[0] = 0;
  mr->sp_triple_15879c[1] = 0;
  mr->sp_triple_15879c[2] = 0;
  mr->sp_recv_1587ac = FIFA96_MATCH_ENTITY_NONE;
  mr->sp_b0_1587b0 = 0;
  mr->sp_b3_1587b3 = 0;
  mr->sp_b6_1587b6 = 0;
  mr->sp_b7_1587b7 = 0;
  memset(&mr->sp_target, 0, sizeof mr->sp_target);
  mr->sp_dir_15877b = 3;
  mr->sp_dir_15877f = 3;
  mr->sp_b5_1587b5 = 0;
  mr->sp_path[0][0] = mr->sp_path[0][1] = 0;
  mr->sp_path[1][0] = mr->sp_path[1][1] = 0;
  mr->sp_path[2][0] = mr->sp_path[2][1] = 0;
  mr->sp_path_valid = 0;
  mr->sp_clock_157a68 = 0;
  mr->sp_global_105fb0 = 0;
  /* FU274 §1.1: `byte[0x157821]` is the camera's `render.camera.pan_counter`. */
  /* FU-196/FU-197 (M2 phase-17 T2) / FU-201 (phase-18 T2) / FU-202 §6 rec (b)
   * (M2 phase-18 T3): a fresh match ACTIVATES the native situation producer —
   * the live `begin` path runs `fifa96_match_run_period_setup(mode 1)` after the
   * begin-tail act-slot reset (native `FUN_000493A0 -> FUN_0004B02C(1)`,
   * 0x4B095). The act-4 formation state stays the zero stand-in (the order
   * matrices are a staged leg, FU201-L4). */
  mr->period_setup_live = 1;
  mr->formation_order_ok = 0;
  memset(mr->formation_order, 0, sizeof mr->formation_order);
  memset(mr->formation_current, 0, sizeof mr->formation_current);
  mr->formation_mismatch[0] = 0;
  mr->formation_mismatch[1] = 0;
  mr->store_158820 = 0;
  mr->store_15881e = 0;
  /* FU-194 (M2 phase-17 T5): a fresh match drops any act-8 sweep cursor. */
  mr->act8_team = 0;
  mr->act8_record = 0;
  mr->act8_selected = NULL;
  /* FU-148 S4: a fresh match restarts at the BSS formation default 0 (the
   * native FUN_00011620 team-record producer is leg). P6: a pending Team Select
   * handout applies here, after the reset and before the kickoff formation
   * seed/meta bind below, so the whole match sees the real per-side ids (the
   * old post-begin set_formation changed only the observable cell). A begin
   * with no pending handout explicitly clears the active flag, so a reused run
   * cannot inherit a previous match's handout. */
  mr->formation[0] = 0;
  mr->formation[1] = 0;
  mr->formation_seeded = 0;
  mr->matchup_active = 0;
  if (mr->matchup_pending) {
    mr->formation[0] = mr->matchup.formation[0];
    mr->formation[1] = mr->matchup.formation[1];
    mr->matchup_active = 1;
    mr->matchup_pending = 0;
  }
  /* FU-150 P2: a fresh referee machine and the derived FU-68 settings handoff
   * (the menu defaults: foul level 2, offside disabled; the front-end's live
   * settings state is unported). */
  memset(&mr->referee, 0, sizeof mr->referee);
  mr->ref_machine = FIFA96_MATCH_RUN_REF_NONE;
  mr->ref_restart_stage = 0;
  mr->ref_whistle = 0;
  mr->ref_speech = 0;
  mr->whistle_seq = 0;                 /* FU-343 P3 fix: no staged whistle */
  {
    struct fifa96_settings settings;
    fifa96_settings_defaults(&settings);
    (void)fifa96_settings_handoff(&settings, 0u, &mr->config);
  }
  match_run_reset_screen_state(mr);    /* FU-146 S3: fresh consumer state */
  fifa96_match_run_reset_input(mr);    /* fresh input edges/held and slot */
  match_run_release_stage(mr);         /* drop the previous match's staged arena */
  fifa96_match_run_reset_render(mr);   /* fresh camera/window/display/scene */
  /* M2 phase-9 T3 (FU-148 §12.2): the native match-init FUN_00011B7C calls
   * FUN_0001C9BC (0x11BA6) to rebuild the per-side input/range words
   * [0x14C1D4]/[0x14C1D6] from the 0x105278-family config cells. The entry
   * gate `FUN_0006D1B2() == 1` is `[0x15753C] == 0` (the image/BSS default;
   * its FUN_0006D5D5/FUN_0006D7AE producers are unported), so the engine
   * passes the open gate. The config cells are staged zero by init (the image
   * default) and the producer path `fifa96_match_run_set_input_options` can
   * stage the front-end block (FU-148 §13.1) before/after begin; the derived
   * words and bits are rebuilt here and in that setter. */
  (void)fifa96_input_range_words(mr->input_cfg, 0, mr->input_range);
  match_run_input_bits_update(mr);
  /* FU-202 §6 rec (b) (M2 phase-18 T3): the native match-init situation
   * producer `FUN_000493A0 -> FUN_0004B02C(1)` (0x4B095, situation 0 at
   * 0x4B0A5) now runs AFTER the begin-tail act-slot reset (below) so the armed
   * act 0xA survives into the first granted frame's clock-tail act tick. Its
   * old position here (before `kickoff_entry`) was dead: the begin-tail
   * `fifa96_match_act_init` cleared the arm before any tick (FU-202 §3A). */
  /* FU-89 §11 / OL-T11-8: seed the records' targets from the resource-loaded
   * formation before the commit (the native `FUN_0008D098` phase-cell order at
   * `FUN_000740A0`, then `FUN_00073E08`). Soft-fails to zero targets. */
  int formation_seeded = match_run_formation_seed(mr);
  mr->formation_seeded = (uint8_t)(formation_seeded != 0);
  /* P6: when the two Team Select formations differ, team 1 keeps its own
   * `.fmt` placement (the home seed above laid out both teams). Equal ids skip
   * the reseed, so every direct-begin/M2 path is byte-identical. */
  if (mr->matchup_active && mr->formation[0] != mr->formation[1])
    (void)match_run_formation_seed_team(mr, 1u, mr->formation[1]);
  /* FU-199 §7.2 / FU-200 §1 (M2 phase-18 T1; re-placed native-order by the
   * two-axis review): the OL-13 metadata bind — the native
   * `FUN_0007412C -> FUN_0008CE78 -> FUN_0006D9C4 -> FUN_0006D920` producer.
   * Native order (fresh `disassemble_function`): `FUN_00092e2c` runs the match
   * begin `FUN_00074034` (which contains the phase-cell seed
   * `FUN_0008D098`), THEN the bind `FUN_0007417c` (`FUN_0007412C` + the slot
   * bind `FUN_00078824`), THEN the screen handler `0x92EF7 CALL [0x15B6D4]`
   * that writes phase 0x15. So the bind precedes both the derived slot bind and
   * the screen-install handler; the phase-0x15 write legitimately sees the
   * bound metadata (it is NOT the pre-bind front-end head). Stages each record's
   * `[rec+0xC]/[rec+0x10]/[rec+0x14]` into the formation id's `.fmt` family
   * (the `352pl`/`352pk`/`freekick` blobs of GAMEART0, FU-199 §5.1/FU-200 §2),
   * opening the `phase_meta` NULL gate for the per-record phase bodies (rows
   * 0x02/0x03/0x04/0x06/0x07/0x08/0x09/0x15). Soft-fails (pointers stay NULL)
   * with no assets/ISO or a missing family entry. Per-field: a missing/short
   * `.fmt` blob clears only its own pointer, per side (the FU-205 gate is each
   * body's own field). */
  (void)fifa96_match_run_record_meta_bind(mr);
  /* M2 interactive Task 1: the derived setup slot bind (`FUN_00078824` ->
   * `FUN_000785E0`) runs before the kickoff state-1 arm, whose `FUN_0007876C`
   * merge moves the bound slot onto the taker (the arm's own `0x8D243` call). */
  match_run_slot_bind(mr);
  /* FU-143 §8/OL-84 (M2 visible-match Task 2): the derived kickoff phase
   * entry, between the native `FUN_000740A0(1, side)` write (0x88E82, act 1 =
   * phase-0x17 handler FUN_00088DC8 stage 0) and the `FUN_00073E08` placement
   * commit begin models below. The run leaves the reset default phase 0 and
   * enters the kickoff-placement phase 1 (class 0: the clock stops). The live
   * phase 2 follows on that chain: the state-1 arm above installs actions 1/2,
   * and action row 01 calls FUN_0008A938 situation 0xB at 0x7DF90 once the
   * derived act-1 producer arms `mr->global_5882a` (M2 playable-match Task 2 /
   * FU-143 §11). */
  /* M2 playable-match Task 2 / OL-84 residual + FU-184 §5/§7 T2-2: the shared
   * kickoff phase entry (native 0x740A0): the phase-1 latch, the per-team
   * FUN_0008D098 state-1 arm (0x740C8/0x740DB, staging the kickoff actions 1/2
   * and the team targets) and the FUN_0007876C merge drain. It runs before the
   * placement commit below, matching the native order (0x88E82 FUN_000740A0
   * precedes 0x88E87 FUN_00073E08; the 0x8D1D1 FUN_00079CCC pick reads the
   * placement outputs the formation seed set). M2 phase-18 T4 / FU-203: the
   * direct ad-hoc call is RETIRED — the kickoff is now entered through the
   * native act-1 route (register phase 0x17), so `fifa96_match_act_dispatch(1,
   * 0, 1)` arms act 1 and invokes `fifa96_match_action_phase_17` stage 0 (which
   * runs `fifa96_match_run_kickoff_entry`). The arm is re-initialised by the
   * begin-tail `fifa96_match_act_init` below; the observable begin state (phase
   * 1 + the state-1 arm) is unchanged. Post-cutscene the phase-0x20 terminal
   * re-dispatches situation 1 -> act 1 -> phase 0x17 (the real match kickoff). */
  (void)fifa96_match_act_dispatch(mr, 1, 0, 1);
  /* FU-96 leg 5 (M2 interactive T2): the per-record camera place
   * `FUN_00079F3C` runs inside the native `FUN_0008CF60` loop between the
   * phase handler and the `FUN_00079B6C` commit, on the non-controlled team's
   * records only, snapping targets within 0x180 of the camera onto the
   * 0x180 ring. The commit leaves targets untouched, so this whole-pool pass
   * before the commit pass is observationally identical. The kickoff camera is
   * the reset triple (0,0,0) and the live phase is 1 (both gates pass). The
   * resource-less degradation (no formation seed) keeps the documented zero
   * targets: the native phase handler always precedes the place, so a zero
   * target is an engine-only state and must not fabricate a ring position. */
  if (formation_seeded)
    (void)fifa96_match_entities_camera_place(&mr->entities,
                                             mr->phase_machine.side_controlled,
                                             (uint8_t)mr->state.phase,
                                             mr->render.camera.pos_x,
                                             mr->render.camera.pos_z);
  /* FU-89 §kickoff placement / OL-T11-8: the derived kickoff pass after the
   * camera reset (native `FUN_00088DC8` stage 0 order: `FUN_000700F4` camera
   * -> `FUN_00073E08` placement). The act-1 ball spawn (0x1E0/0), the
   * `FUN_00079B6C` per-record commit, the camera-vs-target face and the
   * `0x79C13` selector run over the fresh pool; the kickoff camera is the
   * `[0x10F328/2C/30]` reset triple the engine's fresh camera models. */
  (void)fifa96_match_entities_kickoff_place(&mr->entities, mr->render.camera.pos_x,
                                            mr->render.camera.pos_y,
                                            mr->render.camera.pos_z);
  if (eng->surface) {
    /* FU-92: the derived window setter clamps to the surface; the live match's
     * 160x100 sequence / zoom window selection is an open leg, so the fresh
     * match starts from the full surface window (define_full sets the screen
     * dimensions and applies the window). */
    (void)fifa96_window_define_full(&mr->render.window, eng->surface->width,
                                    eng->surface->height);
  }
  int rc = fifa96_match_lifecycle_begin(&mr->lc, &mr->backend, &mr->pace, selector);
  if (rc != 0) {
    mr->engine = NULL;
    return rc;
  }
  mr->running = 1;
  eng->match = mr;
  eng->mode = FIFA96_ENGINE_MODE_MATCH;
  /* FU-62 §4.6: the state-init zero length would complete a period on the
   * first second, so begin installs the derived lengths for the selector. */
  uint16_t period = selector == 0 ? FIFA96_MATCH_RUN_PERIOD_SECONDS_DEFAULT
                                  : FIFA96_MATCH_RUN_PERIOD_SECONDS_RESET;
  uint16_t extra = selector == 0 ? FIFA96_MATCH_RUN_EXTRA_SECONDS_DEFAULT
                                 : FIFA96_MATCH_RUN_EXTRA_SECONDS_RESET;
  (void)fifa96_match_run_set_period(mr, period, extra);
  /* FU-146 S3 / the match-screen installer callers (FUN_00038630 0x38DCC,
   * FUN_0003BB1C 0x3BF9E): the derived match-screen install. leg 0 / mode 0 /
   * side 0 are the derived front-end defaults (the [0x14AF7C]/[0x14AF74]/
   * [0x14AF60] producers are FU-146 legs 1/3). It arms the goal-screen step
   * machine so the frame scheduler consumes queued goals. */
  /* FU-170 §7 item 1 (phase-14 T3): the installer runs its handler once
   * (0x92EF7), and under the derived act-9 route the leg-0 step-0 sit-0xC
   * queue arms the act-9 machine and writes phase 0x15. Natively the screen
   * install happens at the front-end match-screen entry, before the match
   * reset (FUN_000886D4) and the kickoff entry (FUN_00088DC8) supersede the
   * act slot and the phase; the engine's install runs last in begin, so the
   * derived reset mirror clears the act-9 machinery and restores the phase
   * cells without a second setter edge (`prev_phase` keeps the reset-0
   * value). The ordering itself is the derived assumption (the native
   * front-end install precedes the reset/kickoff entry; no native run
   * measures the two orders) — minted as leg **FU170-L10** in the FU-170 T4
   * status. */
  {
    uint8_t phase_before = mr->state.phase;
    uint8_t prev_before = mr->state.prev_phase;
    uint8_t pm_state_before = mr->phase_machine.state;
    uint8_t pm_phase_before = mr->phase_machine.phase;
    (void)fifa96_match_run_screen_install(mr, 0, 0, 0);
    (void)fifa96_match_act_init(&mr->act);
    mr->act9_gate = 0;
    /* FU-202 §6 rec (b) (M2 phase-18 T3): the native match-init situation
     * producer `FUN_000493A0 -> FUN_0004B02C(1)` (0x4B095, situation 0 at
     * 0x4B0A5) is RE-PLACED to run AFTER the act-slot reset so the armed act
     * 0xA survives into the first granted frame's clock-tail act tick (the
     * phase-0x20 body then installs code 0x24). At its old pre-kickoff
     * position the begin-tail `fifa96_match_act_init` cleared the arm before
     * any tick (FU-202 §3A: the producer was dead). The phase cells are
     * restored below (the derived front-end install ordering), so the derived
     * kickoff-phase value is kept while the native act arm survives. The
     * kickoff is entered through the now-LIVE native act-1 route (M2 phase-18
     * T4 / FU-203: the phase-0x20 stage-2 re-dispatch -> situation 1 -> act 1 ->
     * phase 0x17). The begin kickoff is a `fifa96_match_act_dispatch(mr,1,0,1)`
     * re-expression of that same route (up at `:2302`), not an ad-hoc
     * `fifa96_match_run_kickoff_entry` stand-in; its phase-0x17 body stage 0
     * runs `fifa96_match_run_kickoff_entry`, and it produces no part of the
     * code-0x24 move. The retained frame-body `global_5882a` arm (`:3921`) is the
     * remaining derived stage-1 remainder — **leg FU203-L2** (idempotent with
     * the phase-0x17 stage-1 arm; no double-run). */
    if (!mr->live_start && mr->period_setup_live)
      (void)fifa96_match_run_period_setup(mr, 1);
    mr->state.phase = phase_before;
    mr->state.prev_phase = prev_before;
    mr->phase_machine.state = pm_state_before;
    mr->phase_machine.phase = pm_phase_before;
  }
  /* FU-343 (P1): the live-start bootstrap. The windowed bridge opted in (the
   * M2 tape/direct begin never do): the begin kickoff placement above is the
   * 352ko formation commit and must survive, so the native pre-match
   * situation-0/act-0xA collapse is skipped above. Enter the live phase 2
   * directly (FU-143's class-1 play phase), seed the one derived camera event
   * (A2 §4 P4: native has no kickoff event; the FU-310 camera place is
   * unported) and bind the tracked record to the controlled player. These are
   * the engine's live stand-ins, not native producers. */
  if (mr->live_start) {
    /* Phase 80/81 [D]: the native default Friendly capture uses the class-3
     * side camera (slot 5), not the type-0 end preset installed by FUN_0004D7E8.
     * Seed the class-3 pitch/distance envelope; the native position/yaw/pitch
     * integrators are still an open leg. The non-live/tape path keeps the image
     * default. */
    mr->render.yaw = FIFA96_MATCH_RUN_SIDE_YAW;
    mr->render.pitch = FIFA96_MATCH_RUN_SIDE_PITCH;
    mr->render.view_ratio = FIFA96_MATCH_RUN_SIDE_RATIO;
    (void)fifa96_match_run_write_phase(mr, 2u);
    (void)fifa96_camera_event_set(&mr->render.camera, 0, 0, 0x50, 0);
    mr->render.view_focus_x = mr->render.camera.pos_x;
    mr->render.view_focus_y = mr->render.camera.pos_y;
    mr->render.view_focus_z = mr->render.camera.pos_z;
    mr->render.view_focus_valid = 1u;
    (void)fifa96_camera_event_dir(&mr->render.camera, 0, 0, 0);
    /* FU-343 (P1): bind the controlled record. The native [0x157A83] actor
     * cell (entities.controlled) is cleared by the act-9 reset and only
     * republished by the entity chain; the human-controlled record lives in
     * the FU-70 slot (`slot.entity`). Prefer the native actor when live, else
     * the slot bind. */
    int32_t tracked = mr->entities.controlled;
    if (tracked == FIFA96_MATCH_ENTITY_NONE) tracked = mr->slot.entity;
    if (tracked != FIFA96_MATCH_ENTITY_NONE)
      (void)fifa96_camera_set_tracked(&mr->render.camera, tracked);
    /* P7 / FU-345: the live phase-2 entry skips the phase-1 kickoff wait that
     * drives row 01's `[0x157A83]` publish, the row-02 slot merge and row 04's
     * code-5 install. Seed that reachable state from the committed kickoff
     * placement (nearest controlled-side outfield record to the ball) so the
     * human's FU-70 slot has a carrier the kick row can fire. Labelled [D]
     * stand-in for the unported FU-295 live caller chain. */
    /* Phase 77: the human's team is fixed for the run; the native
     * side_controlled cell flips to the fouled side on a restart. */
    mr->live_human_side = (uint8_t)(mr->phase_machine.side_controlled & 1u);
    match_run_live_control_seed(mr);
  }
  match_run_camera_trace_open(mr);   /* FU313 §4.1: immediately before return 0 */
  /* RR-L1: the match begin resets the recorded-frame ring and turns the
   * recorder on (the live frame body then composes + captures once per granted
   * frame inside the render-enabled branch). RR-L2: the overlay latch is
   * cleared so the recorder starts from the open-gate, no-overlay state. */
  fifa96_match_replay_ring_reset(mr);
  mr->recorder_latch_14e574 = 0;
  fifa96_match_replay_ring_record(mr, 1);
  /* RR-L10: the replay FIFO cursors and the staged drain screen id. */
  fifa96_match_run_replay_fifo_reset(mr);
  mr->replay_screen_id_105ffc = 0;
  /* RR-L3: the composer's band/flag/queue-pop cells start clear; the six-byte
   * queue lane defaults to the native `0xFF` fill (the object-array producer
   * is a numbered leg). */
  mr->replay_view_flags_155c4e = 0;
  mr->replay_queue_pop_155c4f = 0;
  mr->replay_band_155c50 = 0;
  mr->replay_band_flag_155c51 = 0;
  memset(mr->replay_queue6, 0xFF, sizeof mr->replay_queue6);
  return 0;
}

int fifa96_match_run_set_live_start(struct fifa96_match_run *mr, int on) {
  if (!mr) return -FIFA96_ERR_INVALID;
  mr->live_start = on ? 1u : 0u;
  return 0;
}

int fifa96_match_run_set_mode(struct fifa96_match_run *mr, uint8_t mode) {
  if (!mr || mode > 4u) return -FIFA96_ERR_INVALID;
  mr->match_mode = mode;
  /* FUN_0001B7B8: only mode 4 (Practise) sets [0x14C32A]; the ordinary
   * played modes clear it. Set after begin, which seeds the direct arm. */
  mr->session_gate_14c32a = mode == 4u ? 1u : 0u;
  return 0;
}

void fifa96_match_run_eye(const struct fifa96_match_run *mr, fifa96_render_pos *out) {
  if (!mr || !out) return;
  if (mr->live_start && mr->render.render_eye_valid != 0u) {
    /* FU351-L2c-3: the native selected view record's eye. */
    out->x = mr->render.render_eye_x;
    out->y = mr->render.render_eye_y;
    out->z = mr->render.render_eye_z;
  } else if (mr->live_start) {
    /* Pre-selection fallback [D]: the old class-3 side envelope. */
    int32_t fx = mr->render.view_focus_valid ? mr->render.view_focus_x
                                             : mr->render.camera.pos_x;
    int32_t fy = mr->render.view_focus_valid ? mr->render.view_focus_y
                                             : mr->render.camera.pos_y;
    int32_t fz = mr->render.view_focus_valid ? mr->render.view_focus_z
                                             : mr->render.camera.pos_z;
    out->x = fx + FIFA96_MATCH_RUN_SIDE_X;
    out->y = fy + FIFA96_MATCH_RUN_SIDE_Y;
    out->z = fz + FIFA96_MATCH_RUN_SIDE_Z;
  } else {
    out->x = mr->render.camera.pos_x;
    out->y = mr->render.camera.pos_y;
    out->z = mr->render.camera.pos_z;
  }
}

void fifa96_match_run_render_eye(const struct fifa96_match_run *mr,
                                 fifa96_render_pos *out) {
  if (!mr || !out) return;
  if (mr->live_start && mr->render.render_eye_valid != 0u) {
    out->x = mr->render.render_eye_x;
    out->y = mr->render.render_eye_y;
    out->z = mr->render.render_eye_z;
    return;
  }
  fifa96_match_run_eye(mr, out);
}

int fifa96_match_run_set_period(struct fifa96_match_run *mr, uint16_t period_seconds,
                                uint16_t extra_seconds) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->state.period_length = period_seconds;
  mr->state.extra_length = extra_seconds;
  return 0;
}

/* P6: the FUN_0003749C apply shape for a live run. The caller has already
 * built the block with fifa96_settings_handoff (field cells + the half-index
 * minute/0x3C/0x14 mapping), so this copies it wholesale and installs the
 * period lengths the clock reads ([0x5881A]/[0x5881C]). */
int fifa96_match_run_set_config(struct fifa96_match_run *mr,
                                const struct fifa96_match_config *config) {
  if (!mr || !config) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->config = *config;
  return fifa96_match_run_set_period(mr, (uint16_t)config->period_length,
                                     (uint16_t)config->extra_length);
}

/* FU-148 §3 (S4): the formation-id writer `FUN_0008EA70(side, id)`:
 * `(&0x14C1E4)[side] = id; FUN_0007412C();`. The consumer (FUN_0007412C ->
 * FUN_0008CE78/FUN_0006D9C4) installs the 0x11033A layout into the team block
 * (a leg: the engine has no team+0x7AE/record+0x90 role fields); the id is
 * observable through `formation[]` and the layout through
 * `fifa96_match_formation_layout`. The native writer does not range-check the
 * side (an OOB write); the port hardens. */
int fifa96_match_run_set_formation(struct fifa96_match_run *mr, uint32_t side,
                                   uint8_t id) {
  if (!mr || side > 1u) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->formation[side] = id;
  /* FUN_0008EA70 -> FUN_0007412C -> FUN_0006D9C4 reloads the per-team
   * `FUN_0004C384(form*6+5)` table on a formation change. */
  match_run_formation_sp_seed_team(mr, side, id);
  return 0;
}

/* P6: stage the Team Select handout for the next begin. The bridge has already
 * run the menu's team handoff, but begin's formation reset happens first, so
 * the real ids must be pre-begin data — see struct fifa96_match_matchup. The
 * handout is copied and marked pending; begin consumes it after the reset. */
int fifa96_match_run_set_matchup(struct fifa96_match_run *mr,
                                 const struct fifa96_match_matchup *matchup) {
  if (!mr || !matchup) return -FIFA96_ERR_INVALID;
  if (mr->running) return -FIFA96_ERR_STATE;
  mr->matchup = *matchup;
  mr->matchup_pending = 1;
  return 0;
}

/* The FU-72 §2.4 increment only. Kept for the paths whose native writers stay
 * unported (the period-indexed goal-screen handler cluster and its situation
 * queue, OL-77/OL-87): gameplay paths have no derived writer invocation yet.
 * The derived full writer is fifa96_match_run_score_event below. */
int fifa96_match_run_add_goal(struct fifa96_match_run *mr, uint32_t side) {
  if (!mr || side > 1u) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->score[side]++;
  return 0;
}

/* FU351-L2k: FUN_0008A938 case 6 appends the `0x157B02` side byte and the
 * `0x157B16` scorer descriptor to the 10-entry goal ring. The engine stores
 * the scorer's roster slot; the ring shifts left when full (0x8ACFD). */
static void match_run_replay_note_goal(struct fifa96_match_run *mr,
                                       uint32_t side) {
  struct fifa96_match_run_replay *r = &mr->render.replay;
  uint8_t roster = 0;
  if (mr->goal_record >= 0) {
    uint32_t team = (uint32_t)mr->goal_record / FIFA96_MATCH_ENTITY_RECORDS;
    uint32_t picked = (uint32_t)mr->goal_record % FIFA96_MATCH_ENTITY_RECORDS;
    if (team < FIFA96_MATCH_ENTITY_TEAMS)
      roster = mr->entities.team[team].records[picked].desc_d;
    /* The native `[0x157A9B] == NULL` branch calls `FUN_0008ECC4` with the
     * scorer descriptor; the engine has no `[0x157A9B]` producer, so this is
     * the live scorer path. */
    (void)fifa96_match_run_stats_bump(mr, mr->goal_record);
  }
  if (r->goal_count >= FIFA96_CAMERA_REPLAY_EVENTS) {
    memmove(r->goal_side, r->goal_side + 1,
            FIFA96_CAMERA_REPLAY_EVENTS - 1);
    memmove(r->goal_roster, r->goal_roster + 1,
            FIFA96_CAMERA_REPLAY_EVENTS - 1);
    r->goal_count = FIFA96_CAMERA_REPLAY_EVENTS - 1;
  }
  r->goal_side[r->goal_count] = (uint8_t)(side & 1u);
  r->goal_roster[r->goal_count] = roster;
  r->goal_count++;
}

int fifa96_match_run_score_event(struct fifa96_match_run *mr, uint32_t side, uint8_t probe) {
  fifa96_action_score state;
  fifa96_action_score_out out;
  fifa96_err_t rc;
  uint16_t old_score;
  if (!mr || side > 1u) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  old_score = mr->score[side];
  state.score[0] = mr->score[0];
  state.score[1] = mr->score[1];
  state.last_side = mr->score_last_side;
  state.tracked_side = mr->score_tracked_side;
  state.max_diff = mr->score_max_diff;
  state.sound_device = mr->score_sound_device;   /* T4/OL-89 gate cells */
  state.sound_midi = mr->score_sound_midi;
  rc = fifa96_action_score_event(&state, side, probe, &out);
  if (rc != FIFA96_OK) return (int)rc;   /* run untouched on a writer failure */
  mr->score[0] = state.score[0];
  mr->score[1] = state.score[1];
  mr->score_last_side = state.last_side;
  mr->score_tracked_side = state.tracked_side;
  mr->score_max_diff = state.max_diff;
  mr->score_last_event = out.posted ? out.post_id : 0u;
  mr->score_display_event = out.dispatched ? out.post_id : 0u;
  if (mr->score[side] != old_score) match_run_replay_note_goal(mr, side);
  return 0;
}

/* FU-191 §2/§9 (M2 phase-17 T1): the native per-record phase walk
 * `FUN_0008D098`'s loop (`0x8D0F2..0x8D166`). 11 records at the `0xB2` stride;
 * skip `[rec+0x9A] != 0` (`0x8D0F7`); stage/call the phase row's handler with
 * the EAX=rec/EDX=&rec[0x4D]/EBX=-1 ABI (`0x8D107`/`0x8D110`, threaded through
 * the walk cursor). The native also calls FUN_0008DCD4 per record (the distance
 * triple `+0x65/+0x67/+0x69`); the engine defers it with the per-record bodies
 * (FU191-L2). */
int fifa96_match_run_record_phase_walk(struct fifa96_match_run *mr, uint8_t phase,
                                       uint32_t team) {
  if (mr == NULL || team >= FIFA96_MATCH_ENTITY_TEAMS) return -FIFA96_ERR_INVALID;
  if (phase >= FIFA96_MATCH_PHASE_ROWS) return -FIFA96_ERR_INVALID;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *e = &mr->entities.team[team].records[i];
    if (e->skip_9a != 0) continue;            /* 0x8D0F7 CMP byte[ECX+0x9A],0 */
    (void)fifa96_match_dispatch_phase_record(mr, phase, e, -1);  /* 0x8D110 */
  }
  return FIFA96_OK;
}

/* FU-181 L5 / FU-342 (phase-53 T2): the native `FUN_00073E28`
 * (`0x73E28..0x73EDC`, fresh `disassemble_bytes` this slice). One store group
 * per native store; see the run.h contract for the cell map and the unmodelled
 * consumers. The `0x157A73 = 0x15774C` focus-pointer store is represented by
 * the engine's live camera triple (the same native cell), not duplicated. */
int fifa96_match_run_act9_reset(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  mr->deepest_keeper_157aa7 = 0;        /* 0x73E3C dword [0x157AA7] */
  mr->global_157a4c = 0;                /* 0x73E42 byte [0x157A4C] */
  mr->state.prev_phase = 0;             /* 0x73E48 byte [0x157A4E] */
  mr->state.phase = 0;                  /* 0x73E4E byte [0x157A4D] */
  mr->phase_machine.state = 0;          /* the FU-142a mirror of [0x157A4D] */
  mr->phase_machine.phase = 0;
  mr->clock_saved_157a60 = 0;           /* 0x73E54 dword [0x157A60] saved Q8 */
  mr->state.frame_acc = 0;              /* 0x73E5A dword [0x157A5C] live Q8 */
  /* 0x73E60 dword [0x157A73] = 0x15774C: the focus pointer is the camera
   * triple; the engine's focus readers use `render.camera.pos_*` directly. */
  mr->goal_record = FIFA96_MATCH_ENTITY_NONE;        /* 0x73E66 [0x157A9F] */
  mr->entities.controlled = FIFA96_MATCH_ENTITY_NONE;/* 0x73E6C [0x157A83] */
  mr->render.camera.event_suspended = 0;             /* 0x73E7B [0x157A6C] */
  mr->referee.offside_suppress = 0;                  /* 0x73E85 word [0x157A6A] */
  mr->state.frame_delta = 0;                         /* 0x73E8E word [0x157A64] */
  mr->flag_157aab = 0xFFu;                           /* 0x73E95 byte [0x157AAB] */
  /* 0x73E9B..0x73EB8: mode = `[0x14C2F6] == 0 ? 0x40 : 0x30` (EDX), then
   * `CALL 0x702F8` and `ADD EDX,0x70`. Fresh `0x702F8` bytes show the helper
   * returns in EAX and preserves EDX (PUSH EDX / POP EDX), so the stored
   * anchor is mode + 0x70 = 0xB0/0xA0; the helper's EAX return is not
   * consumed by this caller. */
  mr->camera_height_157a6d = (int16_t)((mr->difficulty_14c2f6 == 0 ? 0x40 : 0x30) + 0x70);
  mr->flag_157ab0 = 0;                  /* 0x73EC1 byte [0x157AB0] */
  mr->camera_override_157a6f = 0;       /* 0x73EC7 dword [0x157A6F] */
  mr->camera_override_triple[0] = 0;
  mr->camera_override_triple[1] = 0;
  mr->camera_override_triple[2] = 0;
  /* 0x73ECD CALL 0x7F144: the full possession/carrier block 0x158724..0x15872F
   * (fresh bytes: the dword at 0x158724 plus the eight bytes 0x158728..0x15872F). */
  mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;   /* 0x158724 pointer */
  mr->entities.ball.pos_index = 0;      /* 0x158728 */
  mr->entities.ball.pos_rotation = 0;   /* 0x158729 */
  mr->entities.ball.pos_dir_x = 0;      /* 0x15872A */
  mr->entities.ball.pos_dir_z = 0;      /* 0x15872B */
  mr->entities.ball.pos_counter_c = 0;  /* 0x15872C */
  mr->entities.ball.pos_release = 0;    /* 0x15872D */
  mr->entities.ball.pos_counter_e = 0;  /* 0x15872E */
  mr->entities.ball.pos_counter_f = 0;  /* 0x15872F */
  /* 0x73ED2 CALL 0x7A028: the ball-pair block 0x158730..0x158746, flags 0x20,
   * code 2 (the exact `FUN_0007A028` clear). */
  fifa96_match_run_stats_seed(mr);        /* FU-72: keys follow the roster reset */
  return (int)fifa96_ball_pair_clear(&mr->entities.ball.pair);
}

/* FU-145 §3 item 3 / 0x740F6 (S2): the shared phase write. Every engine
 * phase write goes through the FU-62 setter and mirrors the FU-142a machine
 * (the native [0x157A4D] switch byte is the [0x157A4A] phase dword's high
 * byte). When the new phase is 2, the native FUN_000740A0 (0x740E0..0x740F6)
 * clears the pan arm [0x15781D]; the port also clears the zone byte (the
 * native leaves [0x15781E] alone, but it is consumed only while armed and
 * the camera reset 0x7026C refreshes it, so the extra clear is
 * observer-clean). Returns 0 or the setter's error. */
int fifa96_match_run_write_phase(struct fifa96_match_run *mr, uint8_t phase) {
  int rc;
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  rc = fifa96_match_state_set_phase(&mr->state, phase);
  if (rc != 0) return rc;
  mr->phase_machine.state = phase;
  mr->phase_machine.phase = phase;
  if (phase == 2u) {
    mr->goal_armed = 0;
    mr->goal_zone = 0;
  } else {
    mr->live_shot_pending = 0;
  }
  /* FU-191 §4/§9: the native FUN_000740A0 runs the per-team FUN_0008D098 walk
   * (0x740C8 team[side], 0x740DB team[side^1]) immediately after the [0x157A4D]
   * phase write. Gated on a ported phase row (fn != NULL) so it is a structural
   * no-op until a per-record body lands (FU-191 §10; the divergence from the
   * native unconditional FUN_0008DCD4 distance write is FU191-L2). For phases
   * 0x13/0x14 the same FUN_0008D098 call runs its 0x8D693..0x8D722 scatter arm
   * after each team's walk (the native walk -> scatter -> installs order), so
   * the cells 0x10F364/0x10F368 are re-seeded once per write, never per frame
   * (FU-340). */
  if (phase < FIFA96_MATCH_PHASE_ROWS &&
      fifa96_match_phase_table[phase].fn != NULL) {
    (void)fifa96_match_run_record_phase_walk(mr, phase, 0u);
    if (phase == 0x13u || phase == 0x14u)
      (void)fifa96_match_phase_machine_scatter(mr, 0u);
    (void)fifa96_match_run_record_phase_walk(mr, phase, 1u);
    if (phase == 0x13u || phase == 0x14u)
      (void)fifa96_match_phase_machine_scatter(mr, 1u);
  }
  return 0;
}

/* FU-181 L5 / FU-342 (phase-53 T1): the native FUN_0004C324 presentation bind
 * (`0x4C324..0x4C372`, `RET 8`; every register and stack argument is unused).
 * It runs the `FUN_00053DC4` recorder-arm latch (unmodelled leg) and then
 * writes the dword `[0x1074A4]`: when `byte[0x157AC2] >= 4` and the
 * `[0x1587D4]` record pointer is live, `zero_extend(byte[[[0x1587D4]]+0x826])`
 * plus `0x36200(0)`; else `sign_extend(byte[0x1587E6])`
 * (`dword[0x1587E3]>>24`). The arithmetic is the shared duel bind
 * (`fifa96_action_duel_bind`); the engine's `[0x1587D4]` home is
 * `mr->actor_1587d4` (pool id or NONE) and the record team's side is the pool
 * block's `byte[team+0x826]`. */
int fifa96_match_run_present_bind(struct fifa96_match_run *mr) {
  fifa96_action_duel_bind_in in;
  fifa96_action_duel_bind_out out;
  uint8_t side_826 = 0;
  uint8_t team_present = 0;
  int rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (mr->actor_1587d4 >= 0 &&
      mr->actor_1587d4 <
          (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
    uint32_t team = (uint32_t)mr->actor_1587d4 / FIFA96_MATCH_ENTITY_RECORDS;
    team_present = 1;
    side_826 = mr->entities.team[team].side;   /* [*[0x1587D4] + 0x826] */
  }
  in.mode_157ac2 = mr->global_157ac2;
  in.team_present = team_present;
  in.side_826 = side_826;
  in.fallback_e6 = mr->render.sub.current_side;  /* [0x1587E3] >> 24 */
  rc = fifa96_action_duel_bind(&in, &out);
  if (rc != FIFA96_OK) return rc;
  mr->present_side = out.bound;                  /* 0x4C33E / 0x4C36B */
  if (out.stub_36200 != 0) (void)fifa96_arm_stub_36200();  /* 0x4C359 */
  return FIFA96_OK;
}

/* FU-184 §5/§7 T2-2 (phase-16 T2): the shared kickoff phase entry, factored
 * out of begin so the row-0x14 celebration release (native `0x87C71`
 * `FUN_000740A0(1, side)`) runs the identical sequence. The native order is the
 * `FUN_000740A0` phase-1 write, the per-team `FUN_0008D098` state-1 arm, and
 * the `FUN_0007876C` merge drain; begin then runs the `FUN_00073E08` placement
 * commit (its caller keeps that step, matching `0x88E82` before `0x88E87`). */
int fifa96_match_run_kickoff_entry(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  (void)fifa96_match_run_write_phase(mr, FIFA96_MATCH_RUN_KICKOFF_PHASE);
  (void)fifa96_match_phase_machine_kickoff(mr);
  match_run_entity_drain(mr);
  return 0;
}

/* The native period-end probe-cell setters `FUN_00012228` (A, dword cell
 * `0x142FE0`) / `FUN_00012248` (B, dword cell `0x142FE8`). The engine keeps
 * the four values the chooser reads (`probe_a[0..1]`/`probe_b[0..1]`) and this
 * is the front-end hand-off seam for the unported competition-screen producer
 * (season mode 1 / cup mode 3). The native setters are plain dword stores with
 * no run gate; the port keeps that (init/begin still stage the ordinary-setup
 * zero). The native indexes blindly; the port hardens. No producer is faked. */
int fifa96_match_run_set_probe(struct fifa96_match_run *mr, uint8_t side,
                               uint8_t a, uint8_t b) {
  if (!mr || side > 1u) return -FIFA96_ERR_INVALID;
  mr->probe_a[side] = a;
  mr->probe_b[side] = b;
  return 0;
}

/* FU-143 wiring (M2 playability Task 3): the engine-side phase driver. The
 * native funnel is FUN_0008AF38's second rollover: when the phase-class gate is
 * open (class 1 always; class 2 while [0x14C302]==0; class 0 stops the clock;
 * first-hand 0x8AF41..0x8AF80) and the derived completion test
 * `period_seconds == limit + aux_seconds` holds (first-hand 0x8B1EA/0x8B21D;
 * the FU-62 library evaluates it and fifa96_match_run_frame stages the result
 * in `clock_period_ended`), the rollover calls FUN_0008B9CC (first-hand
 * 0x8B574) with the completed period, then zeroes [0x57AB6] and increments
 * [0x157AC2] (0x8B583/0x8B58A). FUN_0008B9CC's derived port is
 * fifa96_action_phase_period_end: under the selector-0/no-extra-time default
 * (extra_time clear, period < 4) it derives phase 0x0C on the controlled side
 * and act 0xB (first-hand 0x8BAA7..0x8BABE, 0x8BADB..0x8BAE7; the native then
 * invokes the act selector, whose handler body is unported). The
 * driver applies the gate to the live phase (fifa96_action_phase_row) and the
 * chooser to the completed period `state.period - 1` (the FU-62 library
 * already incremented the period; the native chooser sees [0x157AC2] before
 * its 0x8B58A increment). Chooser inputs whose native producers are unported
 * are passed as their derived defaults: extra_time 0 ([0x157AC0], the
 * selector-0 default; its writer is a period-1 completion side effect), side
 * 0x157ABE/0x157ABF 0 (OL-75) and the d8/d9 counters 0, plus the zeroed
 * 0x12230/0x12250 probes (OL-74). The post-call act body, the phase-0xC
 * machine FUN_0008BAF0 and the FUN_0004B02C(0)/FUN_00088860 kickoff reset stay
 * unported (FU-143 §4/§5), so the engine lifecycle owns the exit; the
 * 0x0C -> 0 reset is the run_end teardown. */
/* FU-235 §1.2/§4 + FU-236 §1.1/§2.2 (M2 phase-22 T4): the phase-0xC
 * match-over screen dwell. The native phase-0xC block (`FUN_0008BAF0`
 * `0x8BC01`) holds the screen while the end/act timer `word[0x158818]`
 * accrues (`0x8BC13`) and writes the end word only at the terminal
 * (`0x8BCD9`); the period-over re-arm (native `FUN_0004B02C(0)` ->
 * `FUN_00088860`, the sit-1/sit-8 dispatcher) is withheld while the screen
 * dwells, so the class-0 phase `0x0C` persists across frames. Active iff the
 * flag-clear terminal holds phase `0x0C` at the regulation/ET-half indices
 * (`global_157ac2` 2/3), the regulation match-over screen (FU-236 §2.2). The
 * half-time phase-`0x0C` flash (`global_157ac2 == 1`) is not a terminal and the
 * post-extra-time `global_157ac2 == 4` phase-`0x0C` is the sit-8 re-kickoff
 * (FU-217/FU-208, `0x888C8`) — both are unaffected (no suppression). */
static int match_run_terminal_dwelling(const struct fifa96_match_run *mr) {
  return mr->state.phase == 0x0Cu && mr->global_157ac0 == 0u &&
         (mr->global_157ac2 == 2u || mr->global_157ac2 == 3u);
}
/* FU-213 §2.4/§2.5/§7 (M2 phase-19 T1): the comment above is SUPERSEDED in
 * part, not rewritten. The chooser no longer receives the hardwired
 * `extra_time = 0u`: the score tie-break now produces the native OL-85 flag
 * `[0x157AC0]` (`mr->global_157ac0`) on a regulation (index 1) or extra-time-2
 * (index 3) completion with differing scores (§2.4 W2/W4), plus the mid-extra-
 * time W5 set (§2.6), and the live flag is threaded into the chooser so the
 * extra-time arm (act 0xC / phase 0x13-0x14) is selected when it is set. The
 * period-index roll (`mr->global_157ac2 = mr->state.period`, the native
 * 0x8B58A) and the period-4 skip (0x8B608 -> index 5) also run here. The
 * chooser's side/d8/d9/probe inputs remain the derived defaults (OL-74/OL-75);
 * the equal-score override `[0x14C2EE]`, the front-end writer `0x389A4` and
 * the no-extra-time latch `[0x10F378]` stay unported legs (FU213-L1/L7). */
/* ERRATUM (probe-ab, native-faithful probe inputs): the two sentences above
 * that leave the chooser's `0x12230`/`0x12250` probe inputs as the hardwired
 * derived defaults are superseded. The run now carries the native cells
 * (`probe_a[0..1]` = `0x142FE0`, `probe_b[0..1]` = `0x142FE8`, setters
 * `FUN_00012228`/`FUN_00012248`) and threads them into the chooser in native
 * call order `{A0, A1, B0, B1}`. init/begin still stage all four zero (the
 * ordinary match-setup write), so the Friendly path remains phase 0x13; the
 * competition-screen producer is unported and `fifa96_match_run_set_probe`
 * exposes only the seam (no fake caller, FU213-L1/L7 unchanged). */
int fifa96_match_run_phase_drive(struct fifa96_match_run *mr) {
  const fifa96_action_phase_row_desc *row;
  fifa96_action_phase_period_end_out out;
  uint8_t probe[4];
  uint8_t side;
  uint8_t ended;
  int rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  probe[0] = mr->probe_a[0];   /* 0x12230(0) */
  probe[1] = mr->probe_a[1];   /* 0x12230(1) */
  probe[2] = mr->probe_b[0];   /* 0x12250(0) */
  probe[3] = mr->probe_b[1];   /* 0x12250(1) */
  /* FU-225 §4 / D10 (T4): the native `FUN_0004B02C(mode 0)` runs EVERY frame
   * (FUN_000493A0 loop 0x4950A), before the clock; its own gates
   * (state.phase == 0x0C, global_157ac2 == 4) select the sit-1/sit-8 arm.
   * Hoisting it here is the per-frame cadence; the index-4 completion call
   * below is removed as redundant (native-faithful, one call per frame).
   * NB (FU-225 D6/D10): the shared prefix 0x4B038 also clears `pass_parity`
   * ([0x157A4F]) on EVERY call now — that pervasive clear is intended and
   * native-faithful (the prefix is part of the every-frame FUN_0004B02C
   * body), not drift. */
  if (!match_run_terminal_dwelling(mr))
    (void)fifa96_match_run_period_setup(mr, 0);
  /* One-shot: the staged completion belongs to the latest granted frame. */
  ended = mr->clock_period_ended;
  mr->clock_period_ended = 0;
  row = fifa96_action_phase_row(mr->state.phase);
  if (!row) return 0;   /* native has no bounds check; the port hardens (FU-83) */
  /* Class gate: only class 1 (always) and class 2 (halt gate clear) can
   * complete a period; class 0 stops the clock. */
  if (row->clock_class != 1u && row->clock_class != 2u) return 0;
  side = mr->phase_machine.side_controlled;
  {
    int differ =
        mr->score[side & 1u] != mr->score[(side ^ 1u) & 1u];
    if (ended) {
      uint8_t completed;
      if (mr->state.period == 0u) return 0;   /* no completed period to choose */
      /* The completed (old) period index: the FU-62 library already advanced
       * `state.period`; the native chooser sees [0x157AC2] before its 0x8B58A
       * increment. */
      completed = (uint8_t)(mr->state.period - 1u);
      /* FU-213 §2.4 (M2 phase-19 T1): the score tie-break. When the completed
       * period is the regulation end (index 1; W2 0x8B2AC) or the extra-time-2
       * end (index 3; W4 0x8B3A2) AND the score pair [0x157AC5]/[0x157AC7]
       * differs, the OL-85 flag [0x157AC0] is raised. W3's equal-score override
       * escape ([0x14C2EE], no engine cell) and the W6 front-end writer stay
       * unported legs (FU213-L1/L7); the equal-score else-arm is the
       * [0x10F378] no-extra-time latch (its producer is resolved below; the
       * cell itself stays unmodelled).
       *
       * P8 (FU-346, fresh native re-read): the flag is NOT an "extra-time"
       * switch in the football sense, and phase 0x13/0x14 are NOT clocked ET
       * halves. Fresh `read_memory 0x1106AD` puts both phases in clock class 0
       * (indices 19/20 = 0x00), while the live ET half is class-1 phase 2
       * (index 2 = 0x01) on the period index 2/3. Phase 0x13 installs the
       * row-0x25 stats sequence `0x880CC` when the score pair differs
       * (`0x8D76C CMP AX,[0x157AC7]; 0x8D772` install code 0x25) and the
       * end machine holds the decisive screen (`0x8BD95 CMP AX,[0x157AC7];
       * 0x8BDA2 JNZ 0x8BDE7`). So the native decisive regulation end is the
       * post-match stats/result presentation, and the 0x13 -> phase-2 re-arm
       * (`match_run_period_over_rearm`) is the competition/tie ET route
       * (`[0x14C2EE]` override, FRESH `FUN_0003749C 0x374D1/0x374E9`: the
       * override is 1 unless the Game-Select mode is 0 or 3; Friendly is mode
       * 0, `FUN_00024B00 0x24BEB XOR EAX,EAX` -> `FUN_0001B7B8`). Friendly also
       * never plays extra time on a draw: W2's equal-score else-arm sets the
       * `[0x10F378]` no-ET latch (0x8B2DA) and the phase-0x0C end machine
       * takes `0x37F54` -> `[0x105FFC]=5` (0x8BC80..0x8BC97).
       *
       * P8-L1 (labelled live/port stand-in): the native phase-0x13 stats/
       * result screen body is unported (row 0x25 is ported but the screen is
       * not driven here), so the shipped windowed Friendly winner keeps the
       * native flag clear and takes the existing phase-0x0C result terminal —
       * the same terminal a Friendly draw uses. The competition path (the
       * non-live tape and any future competition mode) keeps the native
       * flag/phase-0x13 route byte-exactly. `live_start` is the windowed
       * menu path, whose only shipped Game Select mode is Friendly. */
      if ((completed == 1u || completed == 3u) && differ &&
          !(mr->live_start && completed == 1u))
        mr->global_157ac0 = 1u;
      /* FU-213 §3: the chooser receives the LIVE extra_time flag (replacing the
       * former hardwired 0u), so the chooser's flag-set arm (act 0xC / phase
       * 0x13-0x14) is selected when the flag is set (the live Friendly gate
       * above keeps a decisive regulation end on the flag-clear arm). */
      rc = fifa96_action_phase_period_end(completed, mr->global_157ac0,
                                          mr->score[side & 1u],
                                          mr->score[(side ^ 1u) & 1u], side, 0u, 0u, 0u,
                                          0u, probe, &out);
      if (rc != FIFA96_OK) return rc;
      if (out.camera_source != 0u)
        mr->render.camera_target.source = out.camera_source;
      if (out.phase != FIFA96_ACTION_PHASE_NONE) {
        rc = fifa96_match_run_write_phase(mr, out.phase);
        if (rc != 0) return rc;
      }
      /* FU-195 §4/§6.1 (M2 phase-17 T6): the native FUN_0008B9CC tail (0x8BAE7
       * CALL 0x000888FC) always invokes the act selector after the phase write:
       * BX=1 (invoke-now), DL=0 (stage 0). `out.act` is 0x0B (extra-time clear)
       * or 0x0C (extra-time set). (The native is unconditional on
       * LAB_0008bac6; the derived dispatch is outside the phase-write
       * conditional.) */
      rc = fifa96_match_act_dispatch(mr, out.act, 0u, 1);
      if (rc < 0) return rc;
      /* FU-213 §2.5 (0x8B58A): the period-index roll — the engine's [0x157AC2]
       * cell (`global_157ac2`) follows the FU-62 library's already-advanced
       * `state.period`. This is the live producer T2's situation-8 gate reads. */
      mr->global_157ac2 = mr->state.period;
      if (mr->live_start && mr->state.period == 1u)
        mr->live_halftime_pending = 1u;
      /* FU-208 §4.2 / FU-209 §6 (M2 phase-19 T2) + FU-225 §4/D10 (M2 phase-21
       * T4): the situation-8 selector. The roll (`:2649`, native 0x8B58A) lands
       * the index on 4 (native `[0x157AC2] == 4`, the extra-time period); the
       * mode-0 period-end producer (native `FUN_00088860` 0x888C8; its
       * `state.phase == 0x0C` gate the native co-condition) then selects
       * situation 8 -> act 7 -> phase 0x1D, else the sit-1 arm (0x888F2). That
       * producer now runs at the TOP of phase_drive every frame (the native
       * cadence, FU225-L5): on this completion frame the pre-roll phase was not
       * yet 0x0C, so the selection lands on the NEXT frame's top call, where the
       * sit-1 else-arm is likewise reachable. The bit-8 disjunct (0x4B0CB
       * `TEST AH,1`) remains the FU208-L1 leg — its range-word writer is
       * unlocated, so it is never faked. */
      return 1;
    }
    /* FU-213 §2.6: the no-completion tail (0x8B590). W5 (0x8B5CD) raises the
     * flag per-frame while the period index is 2 or 3, the scores differ and
     * the flag is clear; the period-4 skip (0x8B608) runs the chooser with the
     * index at 4 and the flag set. */
    if (mr->state.period >= 2u && mr->state.period < 4u && differ &&
        mr->global_157ac0 == 0u) {
      mr->global_157ac0 = 1u;
    } else if (mr->state.period != 4u || mr->global_157ac0 == 0u) {
      return 0;
    }
    rc = fifa96_action_phase_period_end((uint8_t)mr->state.period,
                                        mr->global_157ac0,
                                        mr->score[side & 1u],
                                        mr->score[(side ^ 1u) & 1u], side, 0u, 0u, 0u,
                                        0u, probe, &out);
    if (rc != FIFA96_OK) return rc;
    if (out.camera_source != 0u)
      mr->render.camera_target.source = out.camera_source;
    if (out.phase != FIFA96_ACTION_PHASE_NONE) {
      rc = fifa96_match_run_write_phase(mr, out.phase);
      if (rc != 0) return rc;
    }
    rc = fifa96_match_act_dispatch(mr, out.act, 0u, 1);
    if (rc < 0) return rc;
    if (mr->state.period == 4u) {          /* 0x8B61B INC: period index := 5 */
      mr->state.period = 5u;
      mr->global_157ac2 = 5u;
    }
    return 1;
  }
}

/* ===== FU-149 P1 — set pieces & restarts ================================== */

/* The `FUN_00079CCC` pick over the derived pool (FU-149 §1.5, L7; the FU-143
 * §11.1 kickoff precedent): the record of `team` nearest `(from_x, from_z)`
 * over the records' target triples, skipping array index 0 and the
 * `+0x98`/`+0x9A` exclusions with the native strict minimum; the native
 * no-candidate return is the team base (record 0, `0x79CE4`/`0x79D4F`). The
 * native per-record distance source is the record's phase handler
 * `[rec+0x1C]` placement output; the phase handlers are unported, so the
 * target triple stands in (same ruling as the kickoff pick). */
static int32_t match_run_phase_pick(const struct fifa96_match_run *mr, uint32_t team,
                                    int16_t from_x, int16_t from_z) {
  fifa96_entity_candidate candidates[FIFA96_MATCH_ENTITY_RECORDS];
  int16_t best = 0;
  int index;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[team].records[i];
    candidates[i].x = (int16_t)e->target_x;
    candidates[i].y = (int16_t)e->target_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  index = fifa96_entity_find_nearest(candidates, FIFA96_MATCH_ENTITY_RECORDS, 0,
                                     from_x, from_z, &best);
  return index >= 0 ? (int32_t)index : 0;
}

/* FU-217 §3.2/§8 (M2 phase-20 T3): the native FUN_000897F4 free-record
 * selector (0x897F4..0x8983C). Scans the team's records in the wrap order
 * start, start+1, …, 10, 1, …, start-1 (index 0 is never scanned), returning
 * the pool id (team*RECORDS + index) of the first record with skip_9a == 0,
 * or FIFA96_MATCH_ENTITY_NONE if none is free. `start` clamps to 1..10 (the
 * native 0x897FC: 0 or >= 0xB -> 1). */
static int32_t match_run_free_record(const struct fifa96_match_run *mr,
                                     uint32_t team, int start) {
  if (team >= FIFA96_MATCH_ENTITY_TEAMS) return FIFA96_MATCH_ENTITY_NONE;
  if (start < 1 || start > 10) start = 1;
  for (int n = 0; n < 10; n++) {
    int index = start + n;
    if (index > 10) index -= 10;                 /* wrap 11 -> 1 (0x89830 JMP 0x897FC) */
    if (mr->entities.team[team].records[index].skip_9a == 0u)
      return (int32_t)(team * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)index);
  }
  return FIFA96_MATCH_ENTITY_NONE;               /* native loops forever; engine sentinels */
}

int fifa96_match_run_phase_arm(struct fifa96_match_run *mr, uint8_t phase) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (phase != 3u && phase != 4u && phase != 5u && phase != 6u && phase != 7u &&
      phase != 8u && phase != 9u && phase != 0x0Du)
    return 0;
  /* The native FUN_000740A0 writes [0x157A4A] before FUN_0008D098, and the
   * installs read the latched phase (`FUN_0008CEB8` passes [0x157A4A]>>24). */
  mr->entities.phase = phase;
  if (phase == 5u) {
    /* FU-173 §4.2 / R21-2 (phase-14 T3, first-hand 0x8D433..0x8D4A1): the
     * byte-0 celebration install set. Per team (EBP = the team base): when
     * the team's side byte equals the goal record's team (the `[0x157A9F]`
     * cell dereferenced to its team pointer, `0x8D433..0x8D447`), install
     * code 0x15 over records 1..10 (`PUSH -1; ECX=0x15; EBX=0xA; EDX=1` ->
     * 0x8CEB8), write the goal record into `[team+0x7B2]` and install code
     * 0x14 on it (`0x8D461..0x8D475`); else install code 0 over records 0..10
     * (`0x8D47C`). Both install paths then fall to the trailing native
     * presentation sink the port drops: `0x8D48E XOR EAX,EAX` + `0x8D490
     * CALL 0x513BC` (EAX=0 -> `0x513CC CALL 0x566D0`, the `[0x108FE4/
     * 0x108FE0/0x108FDC]` reader, whose AL&1 arm writes `[0x108E1C]=0x1D`;
     * fresh `0x8D47C..0x8D4A3`/`0x513BC..0x513D3`/`0x566D0..0x566EF` this
     * round) — minted as leg **FU173-L8** in the FU-173 T4 status. Without a
     * scanned record the native dereferences the stale
     * cell; the port hardens to no record-keyed installs (leg FU173-L1 for the
     * other scanner arms). The release (row-0x14 stage 2's phase-1 write
     * 0x87C71) stays unported: FU173-L2/L3.
     *
     * QUOTED ERRATA (phase-16 T2, Ruling W8): the sentence above is
     * SUPERSEDED, not rewritten — T2's `fifa96_match_action_14` stage 2 ports
     * exactly this release (native `0x87C71` -> `fifa96_match_run_kickoff_entry`,
     * the shared phase-1 entry factored from begin). The old text is kept
     * verbatim; this errata supersedes it. */
    for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
      struct fifa96_match_team *team = &mr->entities.team[t];
      if (mr->goal_record >= 0 && team->side == mr->goal_record_side) {
        int32_t idx = mr->goal_record % FIFA96_MATCH_ENTITY_RECORDS;
        (void)fifa96_match_arm_install_multi(&mr->entities, t, 1, 10, 0x15, -1);
        team->target = mr->goal_record;
        (void)fifa96_match_entities_install(&team->records[idx], phase, 0x14u, 0);
      } else if (mr->goal_record >= 0) {
        (void)fifa96_match_arm_install_multi(&mr->entities, t, 0, 10, 0, -1);
      }
    }
    return 0;
  }
  if (phase == 0x0Du) {
    /* 0x8D65D: no install-3 prefix (first-hand, unlike the set-piece arms):
     * controlled -> code 0 over records 1..10 (record 0 kept), the other team
     * -> code 0 over records 0..10 (0x8D63E, the shared 0xB/0xE tail). */
    for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
      uint8_t controlled = (uint8_t)mr->phase_machine.side_controlled ==
                                   mr->entities.team[t].side
                               ? 1u
                               : 0u;
      (void)fifa96_match_arm_install_multi(&mr->entities, t,
                                           controlled != 0u ? 1u : 0u, 10, 0, -1);
    }
    return 0;
  }
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    struct fifa96_match_team *team = &mr->entities.team[t];
    uint8_t controlled =
        (uint8_t)mr->phase_machine.side_controlled == team->side ? 1u : 0u;
    (void)fifa96_match_arm_install_multi(&mr->entities, t, 0, 10, 3, -1);
    if (phase == 7u) {
      /* 0x8D57B: controlled only; probe = the incident triple (the FU-150
       * foul adjudicator is the producer, P2). */
      if (controlled != 0u) {
        int32_t idx = match_run_phase_pick(mr, t, (int16_t)mr->incident_x,
                                           (int16_t)mr->incident_z);
        team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)idx);
        (void)fifa96_match_entities_install(&team->records[idx], phase, 0x12u, 0);
      }
      continue;   /* 0x8D5A2 JNZ -> the non-controlled early return 0x8D814 */
    }
    if (phase == 6u) {
      /* 0x8D4A2: the FUN_00073DC4 penalty spot (0, 0, ±0x8D0) is the probe and
       * the camera reset; the non-controlled team arms its own keeper 0x1F. */
      if (controlled != 0u) {
        int32_t pz = team->side == 1u ? -0x8D0 : 0x8D0;
        int32_t idx;
        (void)fifa96_camera_init(&mr->render.camera, 0, 0, pz);
        idx = match_run_phase_pick(mr, t, 0, (int16_t)pz);
        team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)idx);
        (void)fifa96_match_entities_install(&team->records[idx], phase, 0x13u, 0);
      } else {
        team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS);
        (void)fifa96_match_entities_install(&team->records[0], phase, 0x1Fu, 0);
      }
      continue;
    }
    if (phase == 8u || phase == 9u) {
      /* 0x8D5D9: the controlled team's team base is record 0 (the keeper);
       * code = (phase == 9) ? 0x1E : 0x1D (0x8D607..0x8D62C). */
      if (controlled != 0u) {
        team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS);
        (void)fifa96_match_entities_install(&team->records[0], phase,
                                            phase == 9u ? 0x1Eu : 0x1Du, 0);
      }
      continue;
    }
    if (controlled == 0u) continue;   /* phases 3/4: 0x8D814 early return */
    if (phase == 3u) {
      /* 0x8D2A3: camera reset to the goal snapshot triple (0x15777C/80/84),
       * chosen = FUN_00079CCC(0x15777C), install 0x10 (0x8D32A..0x8D349). */
      int32_t idx = match_run_phase_pick(mr, t, (int16_t)mr->goal_snap_x,
                                         (int16_t)mr->goal_snap_z);
      (void)fifa96_camera_init(&mr->render.camera, mr->goal_snap_x,
                               mr->goal_snap_y, mr->goal_snap_z);
      team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)idx);
      (void)fifa96_match_entities_install(&team->records[idx], phase, 0x10u, 0);
    } else {
      /* 0x8D35B: probe = FUN_0007D360(snapshot) = (±0x710, 0, ±0xB00) from the
       * snapshot sign bits; camera reset to the probe; the 0x92AC8 & 3 draw is
       * consumed (the 0x1A event sink is dropped, L7); install 0x11. */
      int32_t px = mr->goal_snap_x < 0 ? -0x710 : 0x710;
      int32_t pz = mr->goal_snap_z < 0 ? -0xB00 : 0xB00;
      int32_t idx;
      uint16_t r = 0;
      (void)fifa96_camera_init(&mr->render.camera, px, 0, pz);
      idx = match_run_phase_pick(mr, t, (int16_t)px, (int16_t)pz);
      team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)idx);
      (void)fifa96_rng_step(&mr->rng, &r);
      (void)fifa96_match_entities_install(&team->records[idx], phase, 0x11u, 0);
    }
  }
  return 0;
}

/* The FU-149 shared table-2 row runner: the derived `fifa96_action_phase_situation`
 * outcome (phase/act/stage/flags + the sit-3 counter request), the phase write
 * and the FU-149 phase arm. `count_corner` enables the corner-counter
 * increment; only the dispatcher head knows the side (the side-less shared
 * entry passes 0). */
static int match_run_situation_side(struct fifa96_match_run *mr, uint8_t situation,
                                    uint8_t side, int count_corner) {
  fifa96_action_phase_situation_out out;
  fifa96_err_t rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  rc = fifa96_action_phase_situation(situation, &out);
  if (rc != FIFA96_OK) return (int)rc;
  if (count_corner != 0 && out.corner_increment != 0u)
    mr->corner_count[(uint32_t)(side ^ mr->side_swap) & 1u] += 1u; /* 0x8ABFF/0x8AC0F */
  /* The native table-2 arm runs FUN_000740A0 at the phase-writing rows (which
   * also runs the per-team FUN_0008D098 arm); the act-handler invocation
   * (FUN_000888FC, the unported phase-row bodies, OL-13/OL-78) stays
   * unported. */
  if (out.phase != FIFA96_ACTION_PHASE_NONE) {
    rc = (fifa96_err_t)fifa96_match_run_write_phase(mr, out.phase);
    if (rc != 0) return (int)rc;
    rc = (fifa96_err_t)fifa96_match_run_phase_arm(mr, out.phase);
    if (rc != 0) return (int)rc;
  }
  /* FU-150 P2 / FU-183 §5 (phase-16 T1): the act row's FUN_000888FC invocation.
   * Act 2 (the sit-9/sit-0xA rows) is phase 0x18 = the derived free-kick /
   * penalty hand-off (`fifa96_match_action_phase_18` -> `match_run_ref_step_restart`).
   * The ACT MACHINE owns the row: seed the body's restart stage and dispatch
   * act 2 invoke-now (the native `FUN_000888FC(2, stage, BX=1)`), so the
   * stage-0 hand-off runs on this call exactly as the retired direct route did;
   * the clock-tail tick (`fifa96_match_act_tick`) then advances it each granted
   * frame. The ad-hoc `ref_machine = REF_RESTART` route (the FU175-L7
   * double-run) is retired — the act slot is the single owner. Act 9 (sit 0xC,
   * FU-170 §7) installs act 9 **invoke-now**; the phase-0x1F stage-0 handler
   * owns the phase-0x15 write (`0x88F9A`..`0x88FB0`, moved there in phase 41).
   * The native sit-0xC arm is
   * `0x8AF0E MOV EBX,1 / 0x8AF13 MOV EAX,9 / 0x8AF18 XOR EDX,EDX /
   * 0x8AF1A CALL 0x888FC`, so `0x8892E TEST BX,BX` passes and `0x88937 CALL
   * dword[0x158808]` runs the phase-0x1F stage-0 body on the install call.
   * The begin-time install is one-shot: the native match-init
   * (`0x4B095`/`0x4B0A5`, situation 0) supersedes the slot with act 0xA, so
   * this invoke is the ordinary-match reach. The clock tail then ticks the
   * installed handler each granted frame (the native same-frame second run is
   * preserved). Other acts install the slot (0x8890A) but their phase rows
   * stay unported (OL-13). Do NOT clear `situation_pending`: the native
   * sit-0xC dispatch does not touch `[0x15B6C0]`. */
  if (out.act == 2u) {
    mr->ref_restart_stage = out.stage;
  }
  if (out.act != FIFA96_ACTION_PHASE_NONE) {
    /* Act 2 is invoke-now (the stage-0 hand-off runs on this call). Act 7 is
     * likewise invoke-now (FU-219: native `0x8AEAE MOV EBX,1` → the seam
     * `0x88937 CALL dword[0x158808]` runs the phase-`0x1D` arm 0 on the
     * dispatch frame). Act 9 is invoke-now too: the sit-0xC arm is
     * `0x8AF0E MOV EBX,1` → the same `0x88937` call runs the phase-0x1F
     * stage-0 body (the begin-time install is superseded by the match-init
     * act-0xA install at `0x4B095`/`0x4B0A5`, so this is the one-shot install
     * invoke). Every action installs the slot (0x8890A); the clock-tail tick
     * then advances the installed handler each granted frame. */
    rc = (fifa96_err_t)fifa96_match_act_dispatch(
        mr, out.act, out.stage,
        (out.act == 2u || out.act == 7u || out.act == 9u) ? 1 : 0);
    if (rc != 0) return (int)rc;
  }
  return 0;
}

int fifa96_match_run_situation(struct fifa96_match_run *mr, uint8_t situation) {
  return match_run_situation_side(mr, situation, 0u, 0);
}

/* FU-145 §1.3 (first-hand /FIFA96.EXE this slice): the armer's absolute value
 * is stored as a word and re-read sign-extended for the comparison
 * (`0x71354..0x71384`: `abs` in 32 bits, `MOV word[ESP+..],AX`, then
 * `MOV EAX,[ESP+..]; SAR EAX,0x10`). A magnitude whose low word sign-extends
 * negative therefore fails every positive bound — the native trap. */
static int16_t match_run_goal_abs_word(int32_t value) {
  uint32_t magnitude = (uint32_t)value;
  if (value < 0) magnitude = 0u - magnitude;
  return (int16_t)(uint16_t)magnitude;
}

/* Full-32-bit absolute value with the native NEG wrap (INT_MIN stays negative,
 * so `abs(INT_MIN) > bound` is false) — the `0x71475..0x71487` /
 * `0x71519..0x7152B` / `0x71536..0x71548` and the bounce-cell forms. */
static int32_t match_run_goal_abs32(int32_t value) {
  uint32_t magnitude = (uint32_t)value;
  if (value < 0) magnitude = 0u - magnitude;
  return (int32_t)magnitude;
}

int fifa96_match_goal_zone(int32_t x, int32_t y, int32_t z) {
  uint32_t magnitude = (uint32_t)z;
  int32_t zw;
  int32_t bits = 0;
  int32_t h;
  int32_t d;
  if (z < 0) magnitude = 0u - magnitude;
  zw = (int16_t)(uint16_t)magnitude;       /* 0x70084 MOVSX DX */
  if (zw < 0xB10) bits = 8;                /* 0x70087..0x70093 */
  else if (zw >= 0xB90) bits = 4;          /* 0x70095..0x700A1 */
  if (x < -0xD0) bits |= 1;                /* 0x700A7..0x700B1 */
  else if (x >= 0xD0) bits |= 2;           /* 0x700B3..0x700BB */
  d = (int16_t)(uint16_t)((uint32_t)magnitude - 0xB10u);  /* 0x700BD..0x700C3 */
  if (d <= 0x30) h = 0xA0;                 /* 0x700C6..0x700D9 */
  else h = 0xA0 - (d - 0x30);              /* 0x700CB..0x700D7 */
  if (h < y) bits |= 0x10;                 /* 0x700DE..0x700E3 */
  return bits == 0;                        /* 0x700E8..0x700EB TEST/SETZ */
}

/* FU-71 leg 9.6 (phase-51 (FU-71)): the native `0x718A9` boundary body. The
 * `0x718EF`/`0x718FF` dispatch selects the far-wall damp/event arm
 * (`0x719A0..0x71B98`) when `[0x15781E] != 0` (the engine `goal_zone`) or the
 * dispatch input bit 0 is clear — the front-end INVISIBLE WALLS option row 2
 * (`Disabled` = 0), image-default 0, so the arm is the default path. The
 * mirror (`0x71908..0x71972`, FU316-L9) stays the option-enabled path: zone 0
 * and input bit 0 set. The gate is reached on the armed path and on both
 * disarmed fail paths (`0x713A0` phase fail, `0x713C0` armer-gate fail). */
static int match_run_goal_boundary(struct fifa96_match_run *mr) {
  struct fifa96_camera *cam = &mr->render.camera;
  if (mr->goal_zone != 0u || mr->render.input_bit0 == 0) {
    fifa96_camera_wall_event ev;
    int32_t tracked;
    int rc;
    /* 0x71B85: `EAX = [0x1577CA]` is read before FUN_00071C94's 0x700F4
     * subset resets the cell (`0x70258`). */
    tracked = cam->tracked;
    rc = fifa96_camera_wall_arm(cam, &ev);
    if (rc < 0) return rc;
    if (rc == 0) return 0;                  /* head return / gate / flag 0 */
    /* 0x71B5B: the 0x974DC request. The audio sink is unported; the engine
     * records the request (the `match1b_sound_id` convention). */
    mr->camera_arm_announce = ev.announce;
    /* 0x71C94 (0x71CC3 FUN_000700F4 + 0x70544 ramp). 0x71C99 bails with
     * `[0x157A6C] != 0`: no reset, no bind, no zone refresh (`event_set`
     * returns 1 on the engine `event_suspended` cell). */
    rc = fifa96_camera_event_set(cam, ev.seed_x, ev.seed_z, ev.height, 0);
    if (rc < 0) return rc;
    if (rc != FIFA96_OK) return 0;
    {
      /* 0x71D03..0x71D27: a non-NULL tracked record re-binds and arms the dir
       * byte (0x71D70 checks the record's +0x20 slot; a NULL record skips the
       * whole block, leaving the 0x700F4 clear). */
      if (tracked != FIFA96_CAMERA_TRACKED_NONE &&
          tracked >= 0 &&
          tracked < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
        uint32_t t = (uint32_t)tracked / FIFA96_MATCH_ENTITY_RECORDS;
        const struct fifa96_match_entity *e =
            &mr->entities.team[t]
                 .records[(uint32_t)tracked % FIFA96_MATCH_ENTITY_RECORDS];
        (void)fifa96_camera_event_dir(cam, e->has_slot, e->dir_x, e->dir_z);
        (void)fifa96_camera_set_tracked(cam, tracked);     /* 0x71D27 */
      }
      /* 0x7026C: FUN_000700F4 refreshes [0x15781E] from the camera triple. */
      mr->goal_zone = (uint8_t)fifa96_match_goal_zone(cam->pos_x, cam->pos_y,
                                                      cam->pos_z);
    }
    return 1;
  }
  if (fifa96_camera_reflect(cam, mr->render.input_bit0,
                            mr->goal_zone != 0 ? 1 : 0) != 0) {
    mr->goal_armed = 0;
    mr->goal_zone = 0;
    return 1;
  }
  return 0;
}

/* Slice-B Task 7 (FU-71 §6): `FUN_0008F078(record)` — the distance of `record`
 * from its attacking goal line. `0x8F07B..0x8F0A5`: `EDX = [record]` (the team
 * block), `side = byte[EDX+0x826]`; side != 0 -> `-0xB10 - record.pos_z`, else
 * `0xB10 - record.pos_z` (`0x8F086/0x8F096` + the `+0x61` pos_z read). The
 * gate is `> 0x1260` (site 1, 0x714A8). */
static int32_t match_run_goal_line_dist(const struct fifa96_match_run *mr,
                                        int32_t id) {
  uint32_t team = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  const struct fifa96_match_entity *e =
      &mr->entities.team[team].records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
  int32_t z = e->pos_z;                    /* native +0x61 */
  if (mr->entities.team[team].side != 0u) return -0xB10 - z;
  return 0xB10 - z;
}

/* Slice-B Task 7 (FU-71 §6): the six post-arming activity-stat sites of
 * FUN_0007131C, in native order, gated byte-exactly:
 *   `0x714BE` +5   — phase 2, `abs(pos_x) > 0x1E0`, `[0x157A83]==0`,
 *                    `[0x1577CA]!=0`, `FUN_0008F078(tracked) > 0x1260`.
 *   `0x7157C` -100 — goal_zone 0, phase 0x10, `abs(pos_z) > 0x960`,
 *                    `abs(pos_x) < 0x3C0`, `word(abs(pos_x)) < 0xD0`.
 *   `0x715DD` -5   — goal_zone 0, phase != 0x10 (side check), the same
 *                    distance gates, `abs(bounce) < 0x1E0 && bounce_1575a7 >
 *                    0x840`.
 *   `0x716C7` -100 — goal_zone 0, `word(abs(pos_x)) >= 0xD0`, the `0x71665`
 *                    approach update, `bounce_15758d < 0x1E0`, prev or current
 *                    phase 0x10.
 *   `0x71732` -5   — prev and current phase != 0x10, `abs(bounce) < 0x1E0 &&
 *                    bounce_1575a7 > 0x840`.
 *   `0x717E3` +5   — `bounce_15758d != 0 && < 0x30`; when tracked |pos_z| <
 *                    0x750 and signed speed > 0x17, the 0x717AF draw runs
 *                    before the sibling 0x8F188 selection and unconditional stat.
 * The `0x8F188`/`0x6E598` siblings, the `0x71400..0x71463` goal_zone!=0
 * history-ring block (`0x928F0` push / `0x92998` -> `[0x15B43C]`), and the
 * `0x71811..0x7189E` no-stat event arms stay numbered legs. */
static void match_run_goal_post_arm(struct fifa96_match_run *mr) {
  struct fifa96_camera *cam = &mr->render.camera;
  int32_t tracked = cam->tracked;         /* [0x1577CA] */
  int32_t pos_x = cam->pos_x;             /* [0x15774C] */
  int32_t pos_z = cam->pos_z;             /* [0x157754] */
  const int32_t valid = (tracked >= 0 &&
      tracked < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS));

  /* Site 1 (0x71468..0x714CE) — runs regardless of goal_zone. */
  if (mr->state.phase == 2u && valid &&
      mr->entities.controlled == FIFA96_MATCH_ENTITY_NONE &&
      match_run_goal_abs32(pos_x) > 0x1E0 &&
      match_run_goal_line_dist(mr, tracked) > 0x1260) {
    (void)fifa96_match_run_stats_add(mr, tracked, 5);
  }

  /* Sites 2..6 require goal_zone == 0 (0x714D3) and a live tracked record. */
  if (mr->goal_zone != 0u) return;
  if (!valid) return;

  /* Side check (0x714E0..0x71513): a non-0x10 phase needs side ^ (pos_z>0). */
  if (mr->state.phase != 0x10u) {
    uint32_t team = (uint32_t)tracked / FIFA96_MATCH_ENTITY_RECORDS;
    uint32_t side = mr->entities.team[team].side;
    if ((side ^ ((pos_z > 0) ? 1u : 0u)) == 0u) return;
  }

  /* Distance gates (0x71519..0x7155F). */
  if (match_run_goal_abs32(pos_z) <= 0x960) return;
  if (match_run_goal_abs32(pos_x) >= 0x3C0) return;
  if (match_run_goal_abs_word(pos_x) >= 0xD0) {
    /* The 0x71665 approach block — sites 4..6. */
    int32_t new_dist = (int32_t)match_run_goal_abs_word(pos_x) - 0xD0;
    if (mr->flag_1575e8 == 0u || new_dist < mr->bounce_15758d)
      mr->bounce_15758d = new_dist;
    if (mr->bounce_15758d >= 0x1E0) return;
    if (mr->state.prev_phase == 0x10u || mr->state.phase == 0x10u) {
      (void)fifa96_match_run_stats_add(mr, tracked, -100);   /* site 4 */
      return;
    }
    if (match_run_goal_abs32(mr->bounce_15759f) < 0x1E0 &&
        mr->bounce_1575a7 > 0x840) {
      (void)fifa96_match_run_stats_add(mr, tracked, -5);     /* site 5 */
      return;
    }
    if (mr->bounce_15758d != 0 && mr->bounce_15758d < 0x30) {
      const struct fifa96_match_entity *e =
          &mr->entities.team[(uint32_t)tracked / FIFA96_MATCH_ENTITY_RECORDS]
               .records[(uint32_t)tracked % FIFA96_MATCH_ENTITY_RECORDS];
      if (match_run_goal_abs32(e->pos_z) < 0x750 && (int16_t)cam->speed > 0x17) {
        uint16_t draw = 0;
        /* 0x717AF: the draw's low bits select only the unported 0x8F188
         * sibling; consuming it here preserves every later RNG consumer. */
        (void)fifa96_rng_step(&mr->rng, &draw);
      }
      (void)fifa96_match_run_stats_add(mr, tracked, 5);      /* site 6 */
    }
    return;
  }

  /* The word(abs(pos_x)) < 0xD0 arm — sites 2/3 (0x71565..0x71616). */
  if (mr->state.phase == 0x10u) {
    (void)fifa96_match_run_stats_add(mr, tracked, -100);     /* site 2 */
    return;
  }
  if (match_run_goal_abs32(mr->bounce_15759f) < 0x1E0 &&
      mr->bounce_1575a7 > 0x840)
    (void)fifa96_match_run_stats_add(mr, tracked, -5);       /* site 3 */
}

int fifa96_match_goal_arm(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  /* FU-145 S2 / Slice-B Task 3 (FUN_0007131C 0x71325..0x7134F): the counter
   * head. Read [0x15781C]; a zero counter skips aging (0x7132B TEST/JZ
   * 0x7134F); a nonzero counter adds the frame-delta low byte (0x7132F
   * MOV AL,[0x157A64]; 0x71334..0x7133C stores the byte sum back) and resets
   * to 0 only when the zero-extended sum is strictly > 0x1E (0x71342 CMP /
   * 0x71345 JLE; 0x71347..0x71349). Then the FUN_00070DE0 reposition call at
   * 0x7134F runs unconditionally (its own overlap/boundary gates are inside),
   * and the stat events it fills (the two boundary-arm +3 events, Slice-B
   * Task 4) are drained in native order. */
  if (mr->goal_counter_15781c != 0u) {
    uint8_t counter =
        (uint8_t)(mr->goal_counter_15781c + (uint8_t)mr->state.frame_delta);
    mr->goal_counter_15781c = counter;
    if (counter > 0x1Eu) mr->goal_counter_15781c = 0u;
  }
  {
    uint8_t events = 0;
    uint8_t event_code = 0;
    fifa96_camera_stat_events stats;
    memset(&stats, 0, sizeof stats);
    (void)fifa96_camera_reposition(&mr->render.camera, mr->render.camera.prev_x,
                                   mr->render.camera.prev_y,
                                   mr->render.camera.prev_z, NULL, NULL,
                                   &events, &event_code, &stats);
    (void)fifa96_match_run_stats_drain(mr, &stats, mr->record.entity_id);
  }
  if (mr->goal_armed) return match_run_goal_boundary(mr);
  /* 0x71390..0x713A0: phase 2/0x10; any other phase still reaches the
   * 0x718A9 boundary gate (the native `JNZ 0x718A9`). */
  if (mr->state.phase != 2u && mr->state.phase != 0x10u)
    return match_run_goal_boundary(mr);
  /* 0x713A6..0x713C0: |camZ|_w > 0xB20 || |camX|_w > 0x730 (the word
   * truncation trap is reproduced by match_run_goal_abs_word). A failed gate
   * also reaches 0x718A9, where the tighter 0xB10/0x720 gate can fire. */
  if (!(match_run_goal_abs_word(mr->render.camera.pos_z) > (int16_t)0xB20 ||
        match_run_goal_abs_word(mr->render.camera.pos_x) > (int16_t)0x730))
    return match_run_goal_boundary(mr);
  /* 0x713DB..0x713EC: arm, freeze the snapshot triple (y forced 0), then
   * 0x713F2 the goal-mouth classifier over the live camera triple. The
   * native also latches [0x157ACB] at 0x713E6 (leg L7: dropped, its only
   * consumer is the unported 0x8FCC8 read). */
  mr->goal_armed = 1u;
  mr->goal_snap_x = mr->render.camera.pos_x;
  mr->goal_snap_y = 0;
  mr->goal_snap_z = mr->render.camera.pos_z;
  mr->goal_zone = (uint8_t)fifa96_match_goal_zone(mr->render.camera.pos_x,
                                                  mr->render.camera.pos_y,
                                                  mr->render.camera.pos_z);
  /* Slice-B Task 7: the post-arm continuation — the six activity-stat arms at
   * 0x71468..0x718A9 (after the native 0x713FC TEST/JZ continuation; the
   * 0x71400..0x71463 goal_zone!=0 history-ring block stays a leg). */
  match_run_goal_post_arm(mr);
  return 1;
}

/* Slice-B Task 8 (FU-71 §6): the stat-bearing subset of FUN_00072478
 * (0x72478..0x72AC0), the tracker/command-ring decision pass, in native
 * emitting order. See the match_run.h declaration for the arm table. */
int fifa96_match_tracker_decision(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;

  /* Head (0x7247E..0x724C8): the three FUN_00092998 history searches and the
   * `[0x15759D] = 1` latch store. The searches are numbered legs. Their
   * register dataflow (fresh disassembly): `0x72492 MOVZX EDI,AL` makes EDI the
   * precomputed `pos_z < 0` flag (from `0x7247E CMP [0x157754],0`/`0x72485
   * SETL AL`), NOT a search result; the first search's EAX result is
   * overwritten at `0x724A9 MOV EAX,EDX` (discarded); the second result is
   * saved to ESI at `0x724BA`; the third result to EDX at `0x724C6`. The latch
   * is live engine state the save block (Task 9) reads and clears. */
  mr->tracker_15759d = 1;

  /* Prev-phase arm (0x724CF..0x724DC -> 0x72A48..0x72A52): `prev_phase ==
   * 0x10` (`0x724D9 CMP EAX,0x10` / `0x724DC JZ 0x72A48`). */
  if (mr->state.prev_phase == 0x10u) {
    /* 0x72A48 MOV EDX,0xF / 0x72A4D MOV EAX,[0x1577CA] */
    (void)fifa96_match_run_stats_add(mr, mr->render.camera.tracked, 0xF); /* 0x72A52 */
    /* 0x72A57..0x72AC0: the pos_x vs +0xA0/-0xA0 `0x8F188` event posts
     * (0x33/0x34/0x32) are numbered legs (ring post, OL-27). */
    return FIFA96_OK;
  }

  /* Fall-through (prev_phase != 0x10). Two intervening arms can bypass the
   * fall-through pair entirely, so they gate whether `0x72649` may fire:
   *   - the `[0x157823]` one-shot sit-latch arm (`0x724E2..0x724FA`) clears the
   *     latch and increments the `[0x157ACC]` counter, then falls through to
   *     `0x72502` — it is not itself a bypass;
   *   - the third-search-result arm (`0x72502..0x72563`): taken when the
   *     `FUN_00092998(1,4,-1)` result (EDX) is non-NULL, its `[+5]` entity is
   *     non-NULL, and `team[entity].side == (pos_z >= 0)`; it posts
   *     `0x92040`/`0x651F0`/`0x8F188(0x35)` and `JMP 0x72AAF` -> epilogue
   *     `0x72ABA` (RET), never reaching `0x725E8`;
   *   - the controlled-actor arm (`0x72568..0x725E3`): taken when `[0x157A83]`
   *     is non-NULL, its `[+0x8D]` is non-zero, and `team[controlled].side !=
   *     ([0x157784] < 0)`; it posts `0x92040`/`0x651F0`/`0x8F188(0x35)` and
   *     `JMP 0x72AB5` -> epilogue `0x72ABA` (RET), never reaching `0x725E8`.
   * The search-result arm depends on the unported `FUN_00092998(1,4,-1)`
   * result and the controlled arm on the unported `[0x157784]` cell, so
   * neither early exit can be evaluated from engine state today.

   * Fall-through pair `0x72649` (`+0x32`, tracked, gate `0x72615`/`0x72637`:
   * `[0x1577CA] != 0` && `team[tracked].side == (pos_z < 0)`): numbered OPEN
   * leg. Emitting it without the two early-exit gates would fabricate `+0x32`
   * where native emits nothing, so the arm stays unwired until those producers
   * exist. Its ordered second call `0x72660` (`-0x4B`, `[team+0x7A6]`, the ball
   * record) is also a leg (BALL_SUB7A6 has no stats-table id). The
   * `0x72665..0x726E8` `tracked == [0x157A8B]` event-post arm is a leg too. */

  /* Staging arm (0x726E9..0x7270F): `[0x15B43C] != 0 && byte[0] == 0xB`
   * (`0x726EE`/`0x726F0`, `0x726F6`/`0x726F9`). `[0x15B43C]` is the last
   * FUN_00092998(1,4,any) ring-search result (written 0x71463); the engine
   * ports the ring mechanics but keeps no match-run cell for it, so the
   * `+0x19` on `[0x15B43C]+5` (REC_15B43C) is a numbered leg. The trailing
   * 0x8F188 posts and the 0x727D8..0x72A47 FUN_0008EEC8/event-post region are
   * legs too. */
  return FIFA96_OK;
}

/* FU-145 §1.6 (S2): the scanner's derived possession nearest — the native
 * `FUN_0008DE8C(&0x15777C, team_block + side*0x835, 0, NULL)` call at
 * `0x88ADF`-family (EBX = 0 -> record 0 skipped), over the pool records'
 * (x,z) words with the shared `fifa96_entity_distance` metric (the FU-143
 * §11.1 row-01 precedent). The selected record's `+0x826` team byte is the
 * queued side; it equals the searched block's side, so the result itself
 * only feeds the unported announce/outcome sinks (L2/L3). */
static int32_t match_run_goal_nearest(const struct fifa96_match_run *mr, uint32_t team,
                                      int32_t from_x, int32_t from_z) {
  fifa96_entity_candidate candidates[FIFA96_MATCH_ENTITY_RECORDS];
  int16_t best = 0;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[team].records[i];
    candidates[i].x = (int16_t)e->pos_x;
    candidates[i].y = (int16_t)e->pos_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  return fifa96_entity_find_nearest(candidates, FIFA96_MATCH_ENTITY_RECORDS, 0,
                                    (int16_t)from_x, (int16_t)from_z, &best);
}

/* FU-146 §7 item 1 / FU-149 §1.1 (0x8AC28/0x8AC88..0x8ADB4): the situation-6
 * direct fallback = the score increment plus the table-2 row 6 phase-5 write
 * through the shared entry. The native direct arm skips the phase write when
 * [0x157AC2] (the period counter) is 2 or 3 (0x8AD96/0x8AD9F); the score
 * increment still runs. */
static int match_run_goal_fallback(struct fifa96_match_run *mr, uint8_t side) {
  int rc = fifa96_match_run_add_goal(mr, side);
  if (rc != 0) return rc;
  if (mr->global_157ac2 == 2u || mr->global_157ac2 == 3u) return 0;
  /* P6 task 2 (FU-344): `0x8AC28` writes the goal side through
   * `FUN_000740A0(5, side)`. The old side-less entry hardcoded side 0, so an
   * away-side goal scored 0-1 but armed the phase-5 presentation as the home
   * side and never released from the away record. Pass the scanned side. */
  return match_run_situation_side(mr, 6u, side, 0u);
}

int fifa96_match_run_goal_queue(struct fifa96_match_run *mr, uint8_t side) {
  /* FU-149 P1 freeze reconciliation: the situation-6 entry is the generic
   * dispatcher head (queue ids 5/6, the [0x15B6B8] side latch) with the goal
   * fallback on the direct path — one shared implementation. */
  return fifa96_match_run_set_piece(mr, 6u, side, 0u);
}

/* FU-197 §4 (M2 phase-17 T2): the `FUN_0008C974` formation-order sorter and
 * residual counter, over the derived order matrices. The native body
 * (`0x8C974..0x8CBAD`) computes the display side index `(side ^ [0x157ABE]) & 1`
 * (`0x8C98B..0x8C998` via `0x741B4`), then selection-sorts the current order
 * `0x157BD2 + side*0x2C` onto the desired order `0x14C360 + side*0x7B` (the
 * `0x8C9A6..0x8CAF8` loop: on a hit swap the two dwords and restart `i = 0`),
 * and finally stores the post-sort residual mismatch count into
 * `[0x157B8E + side]` (`0x8CB1C..0x8CB5E`). The residual is 0 for a valid
 * permutation and nonzero only when a desired entry cannot be matched (FU197
 * §4.4). Gated on `mr->formation_order_ok`: the native desired-order producer
 * is the front-end menu roster builder `FUN_00011620`/`FUN_00012010` (FU-199/
 * FU-201 §3.2), which the engine does not model, so the live default 0 makes
 * this a no-op. FU-201 §6 (R49) proves un-gating on a faithful fill alone is
 * still a no-op (the reset copies desired→current, residual 0) or, with current
 * left zeroed, a fake producer (R45) — the arm's real trigger is a runtime
 * non-permutation (FU201-L1). The companion arrays (`0x157B90` bytes /
 * `0x157BA6` words), the
 * `0x8C758` entity re-link and the `0x4728C` placement-commit sink stay
 * FU197-L2 legs; the exact no-match arm is FU197-L1. */
int fifa96_match_run_formation_sort(struct fifa96_match_run *mr, uint32_t team) {
  if (!mr || team > 1u) return -FIFA96_ERR_INVALID;
  if (!mr->formation_order_ok) return 0;   /* the matrices are unported */
  {
    uint32_t side = ((uint32_t)mr->entities.team[team].side ^ mr->side_swap) & 1u;
    int32_t *cur = mr->formation_current[side];
    const int32_t *des = mr->formation_order[side];
    uint32_t i = 0;
    while (i < FIFA96_MATCH_ENTITY_RECORDS) {
      if (cur[i] == des[i]) { i++; continue; }
      /* find j > i holding the desired entry; on a hit swap and restart
       * (0x8CAF3), else leave the entry as a residual (0x8CB2E) and advance. */
      uint32_t j = i + 1u;
      while (j < FIFA96_MATCH_ENTITY_RECORDS && cur[j] != des[i]) j++;
      if (j < FIFA96_MATCH_ENTITY_RECORDS) {
        int32_t t = cur[i]; cur[i] = cur[j]; cur[j] = t;
        i = 0;
        continue;
      }
      i++;
    }
    {
      uint32_t residual = 0;
      for (uint32_t k = 0; k < FIFA96_MATCH_ENTITY_RECORDS; k++)
        if (cur[k] != des[k]) residual++;
      mr->formation_mismatch[side] = (uint8_t)residual;   /* 0x8CB5E */
    }
  }
  return 0;
}

int fifa96_match_run_set_piece(struct fifa96_match_run *mr, uint8_t situation,
                               uint8_t side, uint8_t bx) {
  int rc;
  if (!mr || side > 1u) return -FIFA96_ERR_INVALID;
  /* 0x8A944..0x8A96B: situation 0 and 0xB, a closed session gate or a pending
   * situation take the direct path; else the table-1 queue arm. */
  if (situation != 0u && situation != 0x0Bu && mr->session_gate_14c32a &&
      !mr->situation_pending) {
    /* 0x8A971..0x8A996: [0x15B6B8] = (side == 0) then the table-1 id
     * (0x8A8E0) and the pending latch. The id space is the FU-146 S3 screen
     * machinery's (the FU-149 L4 boundary); the head only writes it. */
    mr->sit_side_pending = side == 0u ? 1u : 0u;
    switch (situation) {
      case 2u: case 3u: case 4u:
        mr->situation_id = side == 0u ? 9u : 0u;
        break;
      case 5u: case 7u:
        mr->situation_id = 7u;
        break;
      case 6u:
        mr->situation_id = side == 0u ? 5u : 6u;
        break;
      case 9u: case 10u:
        mr->situation_id = side == 1u ? 1u : 2u;
        break;
      default:   /* sit 1, 8, 0xC and >10: the 0x8AA60 default id 0xA */
        mr->situation_id = 0x0Au;
        break;
    }
    mr->situation_pending = 1u;
    return 0;
  }
  /* 0x8AA7B: the direct path. */
  if (bx != 0u) {
    /* FU-241 §6.3 (M2 phase-23 T4): 0x8AA80..0x8AAA3 — FUN_000740A0(0, 0), the
     * stored side/situation, then the act-8 INVOKE-NOW (`0x8AAA3 JMP 0x8AF1A`
     * -> `CALL 0x888FC` with EAX=8, EBX=1, EDX=0). The engine now de-compresses:
     * it dispatches act 8, whose phase-0x1E sweep (0x8A798) resets its cursor,
     * sweeps one record per granted frame for ~22 frames, and only at its
     * stage-2 terminal (0x8A8CE) re-dispatches the stored situation/side (BX=0)
     * and clears [0x15882B]. The former in-frame re-dispatch and immediate
     * [0x15882B]=0xFF are therefore deferred to the sweep terminal. */
    mr->store_15882c = side;
    mr->store_15882b = situation;
    rc = fifa96_match_run_write_phase(mr, 0u);
    if (rc != 0) return rc;
    return fifa96_match_act_dispatch(mr, 8, 0, 1);
  }
  /* 0x8AAA8..0x8AB7A: the BX==0 table-2 route. The native sit 2..4
   * [0x157B8E]/[0x157B8F] formation-order gate + act-4 arm (0x8AB08..0x8AB62)
   * is ported gated (FU-197 §2; M2 phase-17 T2): the two `FUN_0008C974` sorts
   * run, then when the goal-side flag [0x157A4C] != 1 and some team's residual
   * is nonzero the arm stores the re-dispatch cells and invokes act 4
   * (phase 0x1A). The engine's desired-order producer is unported (FU197-L4),
   * so `formation_order_ok` is 0 live and the arm is provably dormant — the
   * table-2 row runs instead (the sit-1 twin 0x8AABF..0x8AB06 stays FU-149
   * L12). FU-201 §6 (R49) pins that the dormancy is not merely "unported":
   * the arm gate is a **permutation residual** (`FUN_0008C974` sorts current
   * onto desired) and the reset `FUN_00073EE0` copies desired→current, so even
   * a faithful fill leaves the residual 0 — un-gating would be a no-op or, with
   * current zeroed, a fabricated residual. The genuine trigger (FU201-L1) has
   * no writer. Situation 6 keeps its score fallback (0x8AC28); situation 3
   * counts before the write. */
  if ((situation == 2u || situation == 3u || situation == 4u) &&
      mr->formation_order_ok) {
    (void)fifa96_match_run_formation_sort(mr, 0u);   /* 0x8AB0D */
    (void)fifa96_match_run_formation_sort(mr, 1u);   /* 0x8AB17 */
    if (mr->global_157a4c != 1u &&                   /* 0x8AB24 CMP EAX,1 */
        (mr->formation_mismatch[0] != 0u || mr->formation_mismatch[1] != 0u)) {
      mr->store_158820 = side;                       /* 0x8AB45 */
      mr->store_15881e = situation;                  /* 0x8AB50 */
      return fifa96_match_act_dispatch(mr, 4, 0, 1); /* 0x8AB57 */
    }
  }
  if (situation == 6u) return match_run_goal_fallback(mr, side);
  /* FU-217 §3.1/§8 (M2 phase-20 T3): the sit-8 re-init/actor bind
   * (0x8AE08..0x8AE54), guarded by byte[0x157A4C] != 1. Native binds both
   * per-side cells then the active side's record into [0x1587D4], and runs
   * act 7 invoke-now. Runs only on the direct path (the queue arm 0x8AA60
   * never touches the cell); the `[0x157AAF] = side` store (0x8AE23) has no
   * engine cell — a leg (FU217-L5). */
  if (situation == 8u && mr->global_157a4c != 1u) {
    mr->global_157a4c = 1u;                                   /* 0x8AE2A */
    mr->actor_1587cc[0] = match_run_free_record(mr, 0u, 0xA);  /* 0x8AE30 */
    mr->actor_1587cc[1] = match_run_free_record(mr, 1u, 0xA);  /* 0x8AE3F */
    mr->actor_1587d4 =
        mr->actor_1587cc[(side < 2u) ? side : 0u];             /* 0x8AE4B/0x8AE54 */
  }
  return match_run_situation_side(mr, situation, side, 1);
}

/* FU-196 §3/§4 T2 rec (M2 phase-17 T2): the native `FUN_0004B02C` situation
 * producer — the match-init / period-end machine the engine lacked. `mode == 1`
 * is the `FUN_000493A0` match-init arm (0x4B095): dispatch situation 0
 * (0x4B0A5). `mode == 0` is the `FUN_00088860` period-end arm (0x4B064): gated
 * on phase 0xC, on the extra-time period index dispatch situation 8 with the
 * RNG side (0x888C8), else situation 1 on the controlled side (0x888F2). Both
 * modes then apply the extra-time gate (0x4B0CB, `TEST AH,1`). Producer-only:
 * no table slot, no row install (R45/R9). See the header for the leg set and
 * the live-begin gate (`period_setup_live`). */
int fifa96_match_run_period_setup(struct fifa96_match_run *mr, int mode) {
  if (!mr) return -FIFA96_ERR_INVALID;
  /* 0x4B038..0x4B062: the shared prefix. The native clears [0x157AB3],
   * [0x157A4F], [0x157A5C] and word[0x157A62+2]; only [0x157A4F] (the FU-139
   * §11 pass toggle) is modelled, the rest are FU196-L2 legs. */
  mr->pass_parity = 0;                                 /* 0x4B038 */
  if (mode == 1) {
    /* 0x4B095..0x4B0A5: the FUN_00074034 / FUN_0007417C restart sinks
     * (FU196-L6/L2 legs), then the match-init situation 0 (side 0, BX 0). */
    (void)fifa96_match_run_set_piece(mr, 0u, 0u, 0u);
  } else if (mode == 0) {
    /* 0x4B064..0x4B095: the FUN_0007412C / FUN_0008CBB0 / FUN_00078824 setup
     * sinks (FU196-L2 legs), then FUN_00088860's phase-0xC body. Natively
     * FUN_00088860 returns to FUN_0004B02C, which continues to the common
     * extra-time gate below (no early exit). */
    if (mr->state.phase == 0x0Cu) {                    /* 0x88863 */
      if (mr->global_157ac2 == 4u) {                   /* extra time (0x888A0) */
        uint16_t r = 0;
        (void)fifa96_rng_step(&mr->rng, &r);           /* FUN_00092AC8 */
        (void)fifa96_match_run_set_piece(mr, 8u, (uint8_t)(r & 1u), 0u); /* 0x888C8 */
        /* FUN_0008F188(0xC1,0,8) sink stays a leg (FU196-L2). */
      } else {
        (void)fifa96_match_run_set_piece(
            mr, 1u, mr->phase_machine.side_controlled, 0u);              /* 0x888F2 */
      }
    }
  }
  /* 0x4B0CB..0x4B0E0: the both-team FUN_0008C418 refresh (FU196-L2 leg) then
   * the extra-time gate — bit 8 of the OR of the two input/range words
   * (the 0x4B0CB `TEST AH,1`; bit semantics FU196-L1). */
  if ((mr->input_range[0] | mr->input_range[1]) & 0x0100u) {
    (void)fifa96_match_run_set_piece(mr, 8u, 0u, 0u);  /* 0x4B0D9 */
    mr->global_157ac2 = 4u;                            /* 0x4B0E0 */
  }
  /* 0x4B10B: [0x158822] = 0 / [0x157A68] = 0 / FUN_000CB2B6() stay legs. */
  return 0;
}

/* FU-239 §1: the period-over re-arm dispatcher (native FUN_0004B02C(mode 0) ->
 * FUN_00088860). Gate: phase 0x0C. Discriminator: dword[0x157AC2]==4 -> the
 * extra-time half (sit-8, act 7, row 0x1D); else the half-time half (sit-1,
 * act 1, row 0x17). The live half then runs in class-1 phase 2 (FU-239 §3). */
void fifa96_match_run_period_over_rearm(struct fifa96_match_run *mr) {
  if (!mr) return;
  mr->state.phase = 2u;          /* sit-8/sit-1 both reach the class-1 play phase 2 */
  mr->state.period_seconds = 0u;
  /* 0x8ABCF/0x8AEBA install act 1/7; the kickoff restart writes phase 1/0x10
     (class 0) then phase 2 (0x8AEFA MOV EAX,2). The engine collapses the
     staging frames to the play phase; the exact route is FU239-L1. */
}

/* FU-145 L4 / FU-149 §1.3: the `[0x1577CA]` ball-record team-byte source is
 * unported; the derived stand-in is the pool controlled actor ([0x157A83],
 * the row-04 convention), then the ball carrier, else side 0. */
static uint8_t match_run_ball_side(const struct fifa96_match_run *mr) {
  int32_t id = mr->entities.controlled;
  if (id < 0) id = mr->entities.ball.carrier;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0u;
  return mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS].side;
}

/* P6 task 2 / P6-L1 (FU-344): the live ball-position goal armer. The native
 * armer consumes a camera pan (`0x71390..0x713EC`, |camZ| > 0xB20 || |camX| >
 * 0x730) and the live follow replaces the FU-310 camera place/focus, so the
 * pan input is absent by construction. When a live phase-2 ball actually
 * enters the native goal-mouth classifier `FUN_00070074` (`0xB10..0xB90` in
 * |z|, |x| < 0xD0, y <= height), arm the same cells the native pan does: the
 * `[0x15781D]` flag, the `0x15777C/80/84` snapshot (y forced 0) and the
 * `[0x15781E]` zone. The scan/direct-fallback chain then runs unchanged. This
 * is a labelled stand-in for the unported live producer (native camera
 * director/FU-310); it never writes the score. */
static void match_run_live_out_force(struct fifa96_match_run *mr);

static void match_run_live_goal_arm(struct fifa96_match_run *mr) {
  const struct fifa96_match_ball *ball = &mr->entities.ball;
  int32_t abs_x;
  int32_t abs_z;
  uint8_t side;
  if (!mr->live_start || mr->state.phase != 2u || mr->goal_armed ||
      mr->live_out_pending)
    return;
  if (fifa96_match_goal_zone(ball->x, ball->y, ball->z)) {
    mr->goal_armed = 1u;                       /* 0x713DB [0x15781D] = 1 */
    mr->goal_snap_x = ball->x;                 /* 0x713E3 MOVSD x3 */
    mr->goal_snap_y = 0;                       /* the native y force */
    mr->goal_snap_z = ball->z;
    mr->goal_zone = 1u;                        /* 0x713F2 classifier true */
    mr->live_out_kind = 0;
    return;
  }
  /* A sampled ball can skip the 0xB10..0xB90 mouth band at the live flight
   * speed. If a live shot's latched target is inside the mouth and the ball is
   * on that target's goal side, arm the mouth instead of classifying the
   * overshoot as a goal-line exit. This is derived [D]; it never writes score
   * itself, only the native armer cells the scan consumes. */
  if ((mr->live_kick_active && mr->live_kick_shot) || mr->live_shot_pending) {
    int32_t target_z = mr->live_kick_target_z;
    int32_t target_mag = target_z < 0 ? 0 - target_z : target_z;
    if (mr->live_kick_target_x > -FIFA96_MATCH_LIVE_GOAL_MOUTH_X &&
        mr->live_kick_target_x < FIFA96_MATCH_LIVE_GOAL_MOUTH_X &&
        target_mag >= 0xB10 &&
        ((target_z > 0 && ball->z > 0xB10) ||
         (target_z < 0 && ball->z < -0xB10))) {
      mr->goal_armed = 1u;
      mr->goal_snap_x = ball->x;
      mr->goal_snap_y = 0;
      mr->goal_snap_z = ball->z;
      mr->goal_zone = 1u;
      mr->live_out_kind = 0;
      return;
    }
  }
  abs_x = ball->x < 0 ? 0 - ball->x : ball->x;
  abs_z = ball->z < 0 ? 0 - ball->z : ball->z;
  side = match_run_ball_side(mr);
  /* Goal line/corner first: a ball beyond both lines is a corner, not a
   * throw-in. */
  if (abs_z > MATCH_RUN_LIVE_GOALLINE_Z) {
    mr->goal_armed = 1u;
    mr->goal_snap_x = ball->x;
    mr->goal_snap_y = 0;
    mr->goal_snap_z = ball->z;
    mr->goal_zone = 0;
    mr->live_out_kind = 2u;                    /* goal line: corner/goal kick */
    mr->live_out_side = side;
    return;
  }
  if (abs_x > MATCH_RUN_LIVE_TOUCHLINE_X) {
    mr->goal_armed = 1u;
    mr->goal_snap_x = ball->x;
    mr->goal_snap_y = 0;
    mr->goal_snap_z = ball->z;
    mr->goal_zone = 0;
    mr->live_out_kind = 1u;                    /* touchline: throw-in */
    mr->live_out_side = (uint8_t)(side ^ 1u);
    return;
  }
}

int fifa96_match_run_goal_scan(struct fifa96_match_run *mr) {
  int32_t abs_z;
  int32_t side;
  if (!mr) return -FIFA96_ERR_INVALID;
  /* 0x8B623..0x8B643: phase 2/0x10 and armed. The native period-4 branch
   * (0x8B60D..0x8B621) joins the tail without the scan only on the
   * extra-time path ([0x157AC0] != 0); since T1/FU-213 the flag is live
   * (`mr->global_157ac0`) and the period-4 skip is modelled in
   * `fifa96_match_run_phase_drive`, but this scan itself is unchanged (the
   * ET-skip branch is not taken here — leg L8). */
  if (mr->state.phase != 2u && mr->state.phase != 0x10u) return 0;
  if (!mr->goal_armed) return 0;
  if (mr->live_out_pending) return 0;
  if (mr->live_out_kind == 1u) {
    /* P13 live touchline exit [D]: the native armer is camera-pan driven and
     * absent live; the ball boundary now selects the throw-in arm directly. */
    int rc;
    if (mr->state.phase != 2u) return 0;
    rc = fifa96_match_run_set_piece(mr, 2u, mr->live_out_side, 1u);
    if (rc != 0) return rc;
    mr->live_out_situation = 2u;
    mr->live_out_pending = 1u;
    mr->live_out_frames = 0;
    mr->live_out_kind = 0;
    mr->goal_armed = 0;
    mr->goal_zone = 0;
    return 1;
  }
  if (mr->live_out_kind == 2u) {
    /* P13 live goal-line exit [D]: corner vs goal kick uses the native
     * condition (`(snap z < 0) == (ball side == 1)`) with the ball-side byte
     * captured at the boundary. */
    int rc;
    uint8_t ball = mr->live_out_side;
    uint8_t cond;
    if (mr->state.phase != 2u) return 0;
    cond = ((mr->goal_snap_z < 0) == (ball == 1u)) ? 1u : 0u;
    rc = fifa96_match_run_set_piece(mr, (uint8_t)(3u + cond),
                                    (uint8_t)(ball ^ 1u), 0u);
    if (rc != 0) return rc;
    mr->live_out_situation = (uint8_t)(3u + cond);
    mr->live_out_side = (uint8_t)(ball ^ 1u);
    mr->live_out_pending = 1u;
    mr->live_out_frames = 0;
    mr->live_out_kind = 0;
    mr->goal_armed = 0;
    mr->goal_zone = 0;
    return 1;
  }
  /* 0x8896B..0x88983: |snapshot z| (full 32-bit magnitude; the native NEG
   * wraps INT_MIN to itself) <= 0xB20 selects the throw-in arm 0x88BCC. */
  {
    uint32_t mag = (uint32_t)mr->goal_snap_z;
    if (mr->goal_snap_z < 0) mag = 0u - mag;
    abs_z = (int32_t)mag;   /* 0x80000000 wraps to INT_MIN like the native */
  }
  if (abs_z <= 0xB20) {
    /* 0x88BCC..0x88C00: the throw-in arm (phase-2 only, 0x88BD4). EDX = the
     * ball record's team ^ 1, BX=1 — the head queues id 9/0 or, when the gate
     * is closed/a situation is pending, takes the phase-0 + act-8 fallback to
     * the table-2 phase 3. The 0x974DC(0x1E) scanner sound is dropped (L7). */
    int rc;
    if (mr->state.phase != 2u) return 0;
    rc = fifa96_match_run_set_piece(mr, 2u,
                                    (uint8_t)(match_run_ball_side(mr) ^ 1u), 1u);
    if (rc != 0) return rc;
    return 1;
  }
  if (mr->goal_zone == 0) {
    /* 0x88989/0x88990 + 0x88B53..0x88BBD: the corner/goal-kick arm (phase-2
     * only, 0x88B5B): situation = 3 + ((snap z < 0) == (ball team == 1)) with
     * EDX = ball team ^ 1 and BX=0. Under the end convention `team 0 defends
     * −z`: cond false -> sit 3 (corner), true -> sit 4 (goal kick). */
    int rc;
    uint8_t ball;
    uint8_t cond;
    if (mr->state.phase != 2u) return 0;
    ball = match_run_ball_side(mr);
    cond = ((mr->goal_snap_z < 0) == (ball == 1u)) ? 1u : 0u;
    rc = fifa96_match_run_set_piece(mr, (uint8_t)(3u + cond),
                                    (uint8_t)(ball ^ 1u), 0u);
    if (rc != 0) return rc;
    return 1;
  }
  /* 0x88996..0x889C5: side select. The native tests the goal-side flag
   * [0x157A4C]; when it is not 1 the side is the snapshot sign (SETL at
   * 0x889BD). The flag's writers/record identity ([0x1587D4]) are unported
   * (leg L4), so the derived side is always the snapshot sign. */
  side = mr->goal_snap_z < 0 ? 1 : 0;
  /* 0x889C7..0x88B31: the possession nearest over the side's team block;
   * the sinks (0x795B4/0x79C50/0x6E598/0x741B4/0x651F0/0x974F0) stay legs.
   * FU-173 §4.2 / R21-2 (phase-14 T3): the selected record is the native
   * `[0x157A9F]` cell the phase-5 celebration arm reads; the four scanner
   * arms all write it natively, but only the goal arm's selection is derived
   * here (the throw-in/corner writes stay FU173-L1). The derived no-candidate
   * fallback mirrors `match_run_phase_pick` (record 0). */
  {
    int32_t picked =
        match_run_goal_nearest(mr, (uint32_t)side, mr->goal_snap_x, mr->goal_snap_z);
    if (picked < 0) picked = 0;
    mr->goal_record = (int32_t)(side * FIFA96_MATCH_ENTITY_RECORDS) + picked;
    mr->goal_record_side = mr->entities.team[(uint32_t)side].side;
  }
  /* 0x88B37/0x88B42/0x88B44: MOV EAX,6; XOR EBX,EBX;
   * CALL FUN_0008A938(6, record_side, 0) — the queued side is the selected
   * record's team byte, which is the searched block's side. */
  {
    int rc = fifa96_match_run_goal_queue(mr, (uint8_t)side);
    if (rc != 0) return rc;
  }
  return 1;
}

/* ------------------------------------------------------------------------- *
 * FU-146 S3 — the goal-consumer chain: the FUN_000CBC4C probe, the installed
 * 0x110F78 period handlers, FUN_000948AC scheduler, FUN_000935A0 advance and
 * the FUN_00092D8C/FUN_00092E2C installer.
 * ------------------------------------------------------------------------- */

uint8_t fifa96_match_run_goal_probe(struct fifa96_match_run *mr) {
  uint32_t *limb;
  uint32_t eax;
  uint64_t wide;
  uint32_t carry;
  if (!mr) return 0;
  limb = mr->goal_probe_limb;              /* C0..C5 (0x112E68..0x112E7C) */
  /* 0xCBC4C..0xCBC83: the fold chain EAX = C5; EAX += C4; C4 = EAX; ADC ... */
  wide = (uint64_t)limb[5] + limb[4];
  limb[4] = (uint32_t)wide;
  carry = (uint32_t)(wide >> 32);
  for (int i = 3; i >= 0; i--) {
    wide = (uint64_t)limb[i + 1] + limb[i] + carry;
    limb[i] = (uint32_t)wide;
    carry = (uint32_t)(wide >> 32);
  }
  eax = limb[0];
  /* 0xCBC88..0xCBCB6: C5++, then the wrap carry propagates up through C0 */
  for (int i = 5; i >= 0; i--) {
    limb[i]++;
    if (limb[i] != 0u) return (uint8_t)eax;
  }
  eax++;                                   /* the carry out of C0 */
  return (uint8_t)eax;
}

/* FU-146 §4: the six handler step shapes (the native step tables at 0x93B80,
 * 0x93DE4, 0x94074, 0x94234, 0x944D4, 0x946B4). Every step advance zeroes the
 * handler timer. */
enum {
  MATCH_RUN_SCREEN_SETUP0 = 0,      /* the per-leg first step (falls through) */
  MATCH_RUN_SCREEN_SETUP0_RET,      /* leg 2's first step (native RET 0x9413E) */
  MATCH_RUN_SCREEN_GATE,            /* the [0x15882A] gate (legs 0/1/3) */
  MATCH_RUN_SCREEN_SETUP2,          /* the leg-0/1/3 second setup step */
  MATCH_RUN_SCREEN_ADVANCE,         /* leg 2's bare step advance (0x9413F) */
  MATCH_RUN_SCREEN_PHASE,           /* phase-2 latch clear */
  MATCH_RUN_SCREEN_POST,            /* the latch-gated consume step */
  MATCH_RUN_SCREEN_TAIL             /* timer > 0xB4 -> FUN_000935A0 */
};
static const uint8_t match_run_screen_kinds[6][6] = {
    {MATCH_RUN_SCREEN_SETUP0, MATCH_RUN_SCREEN_GATE, MATCH_RUN_SCREEN_SETUP2,
     MATCH_RUN_SCREEN_PHASE, MATCH_RUN_SCREEN_POST, MATCH_RUN_SCREEN_TAIL},
    {MATCH_RUN_SCREEN_SETUP0, MATCH_RUN_SCREEN_GATE, MATCH_RUN_SCREEN_SETUP2,
     MATCH_RUN_SCREEN_PHASE, MATCH_RUN_SCREEN_POST, MATCH_RUN_SCREEN_TAIL},
    {MATCH_RUN_SCREEN_SETUP0_RET, MATCH_RUN_SCREEN_ADVANCE,
     MATCH_RUN_SCREEN_PHASE, MATCH_RUN_SCREEN_POST, MATCH_RUN_SCREEN_TAIL, 0u},
    {MATCH_RUN_SCREEN_SETUP0, MATCH_RUN_SCREEN_GATE, MATCH_RUN_SCREEN_SETUP2,
     MATCH_RUN_SCREEN_PHASE, MATCH_RUN_SCREEN_POST, MATCH_RUN_SCREEN_TAIL},
    {MATCH_RUN_SCREEN_SETUP0, MATCH_RUN_SCREEN_PHASE, MATCH_RUN_SCREEN_POST,
     MATCH_RUN_SCREEN_TAIL, 0u, 0u},
    {MATCH_RUN_SCREEN_SETUP0, MATCH_RUN_SCREEN_PHASE, MATCH_RUN_SCREEN_POST,
     MATCH_RUN_SCREEN_TAIL, 0u, 0u},
};
static const uint8_t match_run_screen_step_count[6] = {6u, 6u, 5u, 6u, 4u, 4u};

/* FU-146 §4: the per-leg id tables (0x93B98, 0x93DFC, 0x94088, 0x9424C,
 * 0x944E4). Leg 5 has no table and no bound: `0x94845 CMP EDX,5 / JNZ
 * 0x9485C` -> `0x94869 MOV EAX,1`, so id 5 -> side 0 and every other id
 * (including 0 and >= 7) -> side 1, with no no-score path; the post handles
 * leg 5 directly. -1 = the no-score counter (legs 0-4 only). The
 * `id - 1 > count - 1 -> no-score` bound is each table's own dispatch; the
 * `0x94655/0x94677` bound belongs to leg 4's table, not leg 5. */
#define MATCH_RUN_SCREEN_NO_SCORE (-1)
static const int8_t match_run_screen_ids_leg0[9] = {1, 0, 1, -1, 0, -1, -1, -1, 1};
static const int8_t match_run_screen_ids_leg1[9] = {1, 0, 1, -1, 0, -1, -1, -1, 1};
static const int8_t match_run_screen_ids_leg2[7] = {1, 0, 1, 0, 0, 1, 0};
static const int8_t match_run_screen_ids_leg3[9] = {1, 0, 1, -1, 0, 1, -1, -1, 1};
static const int8_t match_run_screen_ids_leg4[6] = {1, 0, -1, -1, 0, 1};
static const int8_t *const match_run_screen_ids[6] = {
    match_run_screen_ids_leg0, match_run_screen_ids_leg1,
    match_run_screen_ids_leg2, match_run_screen_ids_leg3,
    match_run_screen_ids_leg4, NULL,   /* leg 5: no table (see above) */
};
static const uint8_t match_run_screen_id_count[6] = {9u, 9u, 7u, 9u, 6u, 0u};

/* FU-146 §3: the per-mode duration table `0x1110EC[mode*24 + leg]` dwords
 * (first-hand read): modes 0..2 are `{15,15,30,30,60,5}` seconds and mode 3 is
 * `{5,1,5,1,3,2}`; `screen_install` seeds `value * 60` frames (0x9410A..0x94111
 * `*15 *4`). */
static const uint16_t match_run_screen_durations[4][6] = {
    {15u, 15u, 30u, 30u, 60u, 5u},
    {15u, 15u, 30u, 30u, 60u, 5u},
    {15u, 15u, 30u, 30u, 60u, 5u},
    {5u, 1u, 5u, 1u, 3u, 2u},
};

/* The handler post step (the native 0x93D2A/0x94179/0x94219/0x9441B/0x945F9/
 * 0x947EF bodies): consume the pending situation through the per-leg id table.
 * Returns 1 when the step ran (the caller advances), 0 when the latch is clear
 * (native JZ epilogue), or a negative -fifa96_err_t. */
static int match_run_screen_post(struct fifa96_match_run *mr) {
  const int8_t *ids = match_run_screen_ids[mr->screen_leg];
  uint8_t count = match_run_screen_id_count[mr->screen_leg];
  uint8_t id = mr->situation_id;
  int32_t tracked = mr->score_tracked_side;
  int32_t side = MATCH_RUN_SCREEN_NO_SCORE;
  uint8_t probe = 0;
  int rc;
  if (!mr->situation_pending) return 0;       /* 0x93D31 JZ epilogue */
  mr->situation_pending = 1;                  /* 0x93D46 re-latch */
  /* The 0x974DC presentation call (EAX=0x1E, the leg-specific EBX/ECX/EDI
   * args) stays FU-146 §8 leg 8 and is not called. */
  mr->goal_last_id = id;                      /* 0x93D5C [0x15B674] */
  if (mr->screen_leg == 5) {
    /* 0x94824: leg 5 forces the minute 0 and folds 0x400/0x401 into the
     * screen accumulator (0x94834..0x94864). */
    uint32_t base = mr->goal_screen_accum;
    uint32_t plus400 = base + 0x400u;
    uint32_t plus401 = base + 0x401u;
    mr->goal_screen_accum = plus400;
    if ((id == 5u && tracked == 0) || (id != 5u && tracked == 1))
      mr->goal_screen_accum = plus401;
    mr->goal_minute = 0;
  } else {
    mr->goal_minute = (uint16_t)((int32_t)mr->screen_timer / 0x3C);  /* 0x93D66 */
    mr->goal_screen_accum += mr->screen_timer;                       /* 0x93D73 */
  }
  /* The id dispatch. Leg 5 has no table and no bound (0x94845 `CMP EDX,5 /
   * JNZ 0x9485C` -> 0x94869 `MOV EAX,1`): id 5 -> side 0, every other id
   * (including 0 and >= 7) -> side 1. Legs 0-4 use their table with the
   * native `EAX = [0x15B6A8] - 1; CMP EAX,count-1; JA no-score` bound. */
  if (mr->screen_leg == 5) {
    side = (id == 5u) ? 0 : 1;
  } else if ((uint8_t)(id - 1u) < count) {
    side = ids[id - 1u];
  }
  if (side < 0) {
    mr->goal_no_score++;                     /* INC dword [0x15B6A0] */
  } else {
    /* The native writer calls FUN_000CBC4C only on the untracked
     * `score[side] == 1 && score[other] < 3` arm (0x939D1), i.e. with the
     * pre-increment score 0; the probe is computed exactly then. */
    if (tracked != -1 && side != tracked &&
        mr->score[side] == 0u && mr->score[side ^ 1] < 3u)
      probe = fifa96_match_run_goal_probe(mr);
    rc = fifa96_match_run_score_event(mr, (uint32_t)side, probe);
    if (rc < 0) return rc;
  }
  rc = fifa96_match_run_write_phase(mr, 0);          /* 0x93DB2 FUN_000740A0(0,0) */
  if (rc < 0) return rc;
  return 1;
}

int fifa96_match_run_screen_step(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (mr->screen_leg < 0 || mr->screen_leg > 5) return 0;   /* no handler */
  /* The handler head (0x93BC4..0x93BD7): [0x15B688] += word[0x157A64]. */
  mr->screen_timer = (uint16_t)(mr->screen_timer + mr->state.frame_delta);
  for (;;) {
    uint8_t kind;
    if (mr->screen_step >= match_run_screen_step_count[mr->screen_leg]) return 0;
    kind = match_run_screen_kinds[mr->screen_leg][mr->screen_step];
    switch (kind) {
      case MATCH_RUN_SCREEN_SETUP0:
        /* The modelled-cell subset of the per-leg first step. Legs 0/1/3 clear
         * the kickoff gate [0x15882A] (0x93C30; legs 1/3 take the clear under
         * the staged [0x15B684]==0 path, FU-146 leg 10); leg 4 clears the pan
         * arm and the snapshot (0x9453C..0x9454E). The presentation/staging
         * remainder (the ±0x720 hint, the 0x10F328/0x15B6C8/0x158897 copies,
         * the situation re-queue, FUN_0004C324) is FU-146 §8 leg 8.
         * FU-170 §7 item 1 (phase-14 T3): the leg-0 step's sit-0xC queue
         * (`0x93C36 MOV EAX,0xC` + `0x93C41 CALL 0x8A938`, EBX/EDX 0) runs
         * under the latched-direct route (the POST re-latch keeps
         * `situation_pending` 1), reaching the act-9 entry `0x8AF0E`. Legs
         * 1/3 queue their own situations at their step 2 (FU170-L3). */
        if (mr->screen_leg == 0) {
          mr->global_5882a = 0;
          mr->act9_gate = 0;
          (void)fifa96_match_run_set_piece(mr, 0x0Cu, 0u, 0u);
        } else if (mr->screen_leg == 1 || mr->screen_leg == 3) {
          mr->global_5882a = 0;
        } else if (mr->screen_leg == 4) {
          mr->goal_armed = 0;
          mr->goal_snap_x = 0;
          mr->goal_snap_y = 0;
          mr->goal_snap_z = 0;
        }
        mr->screen_step++;
        mr->screen_timer = 0;
        continue;
      case MATCH_RUN_SCREEN_SETUP0_RET:       /* leg 2 0x940D6..0x9413E */
        mr->screen_step++;
        mr->screen_timer = 0;
        return 0;
      case MATCH_RUN_SCREEN_GATE:             /* 0x93C59/0x93EDB/0x9434C */
        if (!mr->global_5882a) return 0;
        mr->screen_step++;
        mr->screen_timer = 0;
        continue;
      case MATCH_RUN_SCREEN_SETUP2:
        /* The leg-0/1/3 re-arm/setup step (0x93C7B/0x93EFB/0x9436E): its
         * staging writes ([0x15781D]=1, the 0x158897 snapshot/camera copies,
         * the clock display words, the situation re-queue) are FU-146 §8
         * leg 8; `screen_install` seeds the duration instead, and the
         * [0x15781D] re-arm is not applied because the 0x158897 snapshot
         * producer is unported (arming with a zero triple would poison the
         * FU-145 camera armer).
         * FU-170 §7 item 4 (phase-14 T3): the leg-0 step's sit-3 queue
         * (`0x93CDD MOV EAX,3` + `0x93CE4 CALL 0x8A938`, EDX=0) runs under
         * the latched-direct route and lands the corner counter + phase 4 +
         * the 0x11 taker arm through the table-2 row. It is gated on the
         * derived act-9 → GATE handshake (`act9_gate`, the header doc): the
         * window-scoped statement — the leg-0 SETUP0 clears the native gate
         * first (`0x93C30`), act-9 stage 0 clears it again (`0x88F9A`) while
         * writing phase 0x15, and the machine then waits at step 1 with the
         * singleton act slot holding act 9, so the only producer that can
         * open *that window's* gate is the act-9 stage-3 completion
         * (`0x890C4`). The refused non-act-9 values: act 1's kickoff set
         * (`0x88F07`, cleared `0x88E56`), act 2's tail (`0x895EC`), the reset
         * (`0x886F4`) and the harness-staged m 42 compression. The produced
         * gate carries the corner arm; a staged gate (the M2 v13 tape's m 42
         * compression) advances without it — the disclosed hardening that
         * keeps the v13 strata additive. The two
         * [0x157AB4]/[0x157AB6] clears need fields that do not exist
         * (display-only, FU170-L2). */
        if (mr->screen_leg == 0 && mr->act9_gate != 0u) {
          mr->act9_gate = 0;
          (void)fifa96_match_run_set_piece(mr, 3u, 0u, 0u);
        }
        mr->screen_step++;
        mr->screen_timer = 0;
        continue;
      case MATCH_RUN_SCREEN_ADVANCE:          /* leg 2 0x9413F */
        mr->screen_step++;
        mr->screen_timer = 0;
        continue;
      case MATCH_RUN_SCREEN_PHASE:
        /* 0x93CFE/0x9414D/0x943F1/0x945CF/0x947C3: the phase dword's high byte
         * ([0x157A4A]>>24) is the engine phase. */
        if (mr->state.phase != 2u) return 0;
        mr->situation_pending = 0;            /* the latch clear */
        mr->screen_step++;
        mr->screen_timer = 0;
        continue;
      case MATCH_RUN_SCREEN_POST: {
        int rc = match_run_screen_post(mr);
        if (rc < 0) return rc;
        if (rc == 0) return 0;
        mr->screen_step++;
        mr->screen_timer = 0;
        /* The native falls through into the tail step (0x93DCA); the tail's
         * `timer > 0xB4` test is a guaranteed no-op here because the post zeroed
         * the timer, so returning directly is observationally identical. */
        return 1;
      }
      case MATCH_RUN_SCREEN_TAIL:
        if (mr->screen_timer > 0xB4u)
          return fifa96_match_run_screen_advance(mr);
        return 0;
      default:
        return 0;
    }
  }
}

int fifa96_match_run_screen_advance(struct fifa96_match_run *mr) {
  int32_t total;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  /* 0x935C8..0x935D5: word[score0] + word[score1] (zero-extended words). */
  total = (int32_t)(uint16_t)mr->score[0] + (int32_t)(uint16_t)mr->score[1];
  if (total != mr->goal_log_prev_total) {     /* 0x935E2 CMP; JZ 0x9362B */
    if (total <= 20) {                        /* 0x935E9 JLE: slot = total */
      mr->goal_log[total][0] = mr->score_last_side;
      mr->goal_log[total][1] = (int32_t)mr->goal_last_id;
      mr->goal_log[total][2] = (int32_t)mr->goal_minute;
    } else {
      /* 0x935EB..0x93613: shift slots 0..18 <- 1..19 and write slot 19 */
      for (int32_t i = 0; i < 19; i++) {
        mr->goal_log[i][0] = mr->goal_log[i + 1][0];
        mr->goal_log[i][1] = mr->goal_log[i + 1][1];
        mr->goal_log[i][2] = mr->goal_log[i + 1][2];
      }
      mr->goal_log[19][0] = mr->score_last_side;
      mr->goal_log[19][1] = (int32_t)mr->goal_last_id;
      mr->goal_log[19][2] = (int32_t)mr->goal_minute;
    }
  }
  mr->goal_total = total;                     /* 0x935DC [0x15B69C] */
  mr->goal_log_prev_total = total;            /* 0x93638 [0x15B698] := [0x15B69C] */
  mr->screen_install_hint = 1;                /* 0x936ED [0x15B6C4] */
  mr->screen_timer = 0;                       /* 0x936F3 */
  mr->screen_step = 0;                        /* 0x936FB */
  /* 0x9370A: re-install the same-leg handler and run it once. The 0x9343C /
   * 0x937DC screen installs, the 10/0x14 thresholds and the
   * FUN_0004C324/FUN_00054104 exits stay FU-146 §8 leg 7. */
  return fifa96_match_run_screen_step(mr);
}

int fifa96_match_run_screen_schedule(struct fifa96_match_run *mr) {
  int side1 = 0;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  /* 0x948B2..0x948E0: the screen-over rollover. */
  if (mr->screen_timer > mr->screen_period_frames) {
    mr->situation_id = (uint8_t)(mr->screen_leg != 5 ? 8 : 7);
    mr->situation_pending = 1;
    return fifa96_match_run_screen_step(mr);          /* JMP 0x949E0 */
  }
  if (mr->screen_leg == 2) {                          /* 0x948E5 / 0x9492A */
    if (mr->entities.controlled != FIFA96_MATCH_ENTITY_NONE) {
      uint32_t team =
          (uint32_t)mr->entities.controlled / FIFA96_MATCH_ENTITY_RECORDS;
      side1 = mr->entities.team[team & 1u].side != 0u;
    }
    if (mr->screen_actor_age > 0xF0) {                /* [0x157A97] > 0xF0 */
      mr->situation_id = (uint8_t)(side1 ? 3 : 4);
      mr->situation_pending = 1;
      return fifa96_match_run_screen_step(mr);
    }
  }
  if (mr->screen_leg != 4 && mr->screen_leg != 2 &&   /* 0x9497D..0x949B5 */
      mr->render.camera.pos_z < 0 && mr->render.camera.vel_z < 0) {
    mr->situation_id = 9;
    mr->situation_pending = 1;
    return fifa96_match_run_screen_step(mr);
  }
  if (mr->screen_leg == 5 && mr->render.camera.vel_z < 0) { /* 0x949B7..0x949DA */
    mr->situation_id = 7;
    mr->situation_pending = 1;
    return fifa96_match_run_screen_step(mr);
  }
  return fifa96_match_run_screen_step(mr);            /* 0x949E9 CALL */
}

int fifa96_match_run_screen_install(struct fifa96_match_run *mr, int16_t leg,
                                    int16_t mode, int16_t side) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (leg < 0 || leg > 5 || mode < 0 || mode > 3) return -FIFA96_ERR_INVALID;
  /* FUN_00092D8C 0x92DC5/0x92DD3: leg/mode, step/timer clear, installer latch.
   * M2 phase-9 T3: the head FUN_00078824 (0x92D92 -> 0x78824 0x7882a/0x78830)
   * zeroes the team flags [0x1590CC]/[0x159901] (the sole writers: fresh
   * get_xrefs_to = one WRITE each), then the tracked-side pick 0x92DDF..:
   * `[0x15B684] != 0` -> the side argument; else `[0x1590CC] == 0` -> 1;
   * else `[0x159901] == 0` -> 0; else -1. With the flags just zeroed the
   * image-default mode-0 pick is 1. The 0x10F328 camera copy stays a leg. */
  mr->tracked_flag[0] = 0;
  mr->tracked_flag[1] = 0;
  if (mr->screen_record_mode != 0) {
    mr->score_tracked_side = side;
  } else if (mr->tracked_flag[0] == 0) {
    mr->score_tracked_side = 1;
  } else if (mr->tracked_flag[1] == 0) {
    mr->score_tracked_side = 0;
  } else {
    mr->score_tracked_side = -1;
  }
  mr->screen_leg = leg;
  mr->screen_mode = mode;
  mr->screen_step = 0;
  mr->screen_timer = 0;
  mr->situation_pending = 1;                  /* 0x92DCD */
  /* FUN_00092E2C: the score pair / writer / goal-log-total resets, the
   * duration seed, the hint clear, then the installed handler runs once
   * (0x92EF7 CALL [0x15B6D4]). The [0x15B6B8] side flag (write-only: fresh
   * xrefs = its two writes), the 0x74034/0x7417C resets (the engine's begin
   * covers the fresh-match camera/slot reset) and the clock display cells stay
   * legs. */
  mr->score[0] = 0;                           /* [0x157AC5] */
  mr->score[1] = 0;                           /* [0x157AC7] */
  mr->score_max_diff = 0;                     /* [0x15B6A4] */
  mr->goal_screen_accum = 0;                  /* [0x15B68C] */
  mr->goal_log_prev_total = 0;                /* [0x15B698] */
  mr->goal_total = 0;                         /* [0x15B69C] */
  mr->screen_period_frames =
      (uint16_t)(match_run_screen_durations[mode][leg] * 60u);
  mr->screen_install_hint = 0;                /* 0x15B6C4 */
  return fifa96_match_run_screen_step(mr);
}

/* M2 phase-9 T3 (FU-152 §2.9 / FU-145 §1.2): the every-frame FUN_00071DF4
 * call site. FUN_000736AC 0x739C6..0x739FD gates the call: `[0x157821] == 0`
 * (the pan counter), the bearing word `[0x1577BE] != 0`, `[0x1577EE].hi >
 * 0x10`, and both rate bytes `[0x157816]/[0x157817] == 0` (the nonzero path
 * takes the interpolation arm and jumps over 0x73B5B); the call itself then
 * requires `[0x1577CA] != 0` and `[[rec]+0x20] != 0`. The first arm inside
 * FUN_00071DF4 (0x71E1C..0x71EE5) additionally gates on the record class
 * (`[[rec]+4][0] == 0x18D8`) and the event height `[0x1577EE].hi > 0xF0`,
 * then sets the velocity words from the slot dir bytes
 * `subobj[+0x20]/[+0x21] * 0xF` clamped to +-15 and recomputes the bearing.
 * `fifa96_camera_rate_event` is that arm (the height gate included). The
 * fall-through second arm (`0x71ef0..0x7200f`, FU303/FU294-L2 phase-28 T8) is
 * `fifa96_camera_rate_table`: the `[0x157815]` dir byte + the tracked
 * record's dir pair resolve the 0x10E169/0x11042B rate pair into
 * `camera.rate_x/rate_z`, which the next frame's integrator pan-push consumes.
 * The keeper gate's `FUN_00092998(1,2,-1)` ring result is unported (OL-27),
 * so it is staged 0: the rate write is live, the 0x1D/0x1E sink stays a leg.
 * Caveat (FU303-L9): `e->dir_x`/`e->dir_z` is the entity-pool copy — the same
 * mapping the pre-existing first arm uses — so for a slot-bound tracked record
 * the pair can trail the native slot dirs by one pool drain. That is the
 * established arm's semantics (not changed here; changing it would move the
 * first arm's live behavior).
 * Returns 1 when either arm ran, 0 when a gate refused. */
int fifa96_match_run_camera_follow(struct fifa96_match_run *mr) {
  struct fifa96_camera *cam;
  int32_t tracked;
  struct fifa96_match_entity *e;
  uint8_t keeper_code = 0;
  if (!mr) return -FIFA96_ERR_INVALID;
  cam = &mr->render.camera;
  if (cam->pan_counter != 0) return 0;              /* 0x739C6 */
  if (cam->follow_speed == 0) return 0;             /* 0x739CE [0x1577BE] */
  if ((int16_t)cam->event_param <= 0x10) return 0;  /* 0x739DC */
  if (cam->rate_x != 0 || cam->rate_z != 0) return 0;  /* 0x739ED..0x739FD */
  tracked = cam->tracked;
  if (tracked < 0 ||
      tracked >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  e = &mr->entities.team[(uint32_t)tracked / FIFA96_MATCH_ENTITY_RECORDS]
           .records[(uint32_t)tracked % FIFA96_MATCH_ENTITY_RECORDS];
  if (!e->has_slot) return 0;               /* [[rec]+0x20] != 0, 0x73B65 */
  if (fifa96_camera_rate_event(cam, e->dir_x, e->dir_z,
                               (int16_t)cam->event_param) == 1)
    return 1;                               /* the 0x71e1c..0x71eef early return */
  return fifa96_camera_rate_table(
      cam, (int16_t)cam->event_param, (int16_t)cam->dir_157815,
      (int16_t)cam->timer, 1, e->dir_x, e->dir_z,
      0 /* the 0x92998 ring is unported (OL-27) */, cam->anchor_x, cam->pos_z,
      (int16_t)cam->vel_z, &keeper_code);
}

/* M2 phase-10 T1 (FU-148 §13.5 / FU-142 OL-88): the FUN_0007A084 derived
 * receive arm (contract and first-hand windows in the header declaration).
 * The staging tail's receive gate (fifa96_ball_pair_stage_tail) already
 * proves every receive code has `[0x1104BB+code] == 0`, so the native entry
 * gate 0x7A0A4 is a pass-through on this path; the 0x18E2-class vector
 * rebuild, the sound-id selection/sink, the timer81 loop and the [team+0x7E7]
 * clear are the numbered legs. The 0x7A448 FUN_00079D5C offside call is ported
 * (FU276-L2, phase-26b Task 5) behind the [0x14C2F2] `field_4c2f2` enable and
 * the receiving-record `code` (+0x91) eligibility gate. */
int fifa96_match_run_ball_receive(struct fifa96_match_run *mr, int32_t actor,
                                  const fifa96_ball_pair_vector *vector,
                                  int16_t traj) {
  struct fifa96_ball_pair_state *pair;
  struct fifa96_camera *cam;
  struct fifa96_match_entity *actor_rec;
  struct fifa96_match_team *team;
  fifa96_entity_candidate cands[FIFA96_MATCH_ENTITY_RECORDS];
  fifa96_ball_pair_vec3i base;
  fifa96_ball_pair_vec3i target;
  fifa96_ball_pair_actor rec;
  int32_t actor_id;
  int index = -1;
  uint32_t t;
  if (!mr || !vector) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  pair = &mr->entities.ball.pair;
  cam = &mr->render.camera;
  actor_id = actor;
  if (actor_id < 0 ||
      actor_id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  t = (uint32_t)actor_id / FIFA96_MATCH_ENTITY_RECORDS;
  actor_rec = &mr->entities.team[t].records[(uint32_t)actor_id % FIFA96_MATCH_ENTITY_RECORDS];
  team = &mr->entities.team[t];
  /* 0x7A207..0x7A219: the camera event (seed pair = the staged vector's
   * middle/z words, height = the trajectory word) and the tracked bind. The
   * FUN_00071C94 [0x157A6C] bail (0x71C99) returns 1 without the reset and
   * without the 0x71D27 tracked store, but FUN_0007A084 *continues* (the
   * ack/scan/release below still run) — only a negative error aborts. */
  {
    int ev = fifa96_camera_event_set(cam, (int16_t)vector->height,
                                     (int16_t)vector->z, traj, 0);
    if (ev < 0) return ev;
    if (ev == FIFA96_OK) {
      /* FU303/FU294-L2 (phase-28 T8): FUN_00071C94's actor dir arm over the
       * receiving record (the native EBP). */
      (void)fifa96_camera_event_dir(cam, actor_rec->has_slot, actor_rec->dir_x,
                                    actor_rec->dir_z);
      (void)fifa96_camera_set_tracked(cam, actor_id);
    }
  }
  /* 0x7A227: [0x157A83] = 0 — the controlled actor is released. */
  mr->entities.controlled = FIFA96_MATCH_ENTITY_NONE;
  /* 0x7A2F0: [0x158746] = 1 — the staged event is acknowledged. */
  pair->ack = 1;
  /* 0x7A2F6: phase 1 exits before the receiver scan (the camera event ran). */
  if (mr->state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE) return FIFA96_OK;
  /* 0x7A2FF..0x7A331: the [0x157770] base (the engine stand-in is its camera
   * triple) plus the camera velocity words' leads. */
  base.x = mr->render.camera.pos_x;
  base.y = mr->render.camera.pos_y;
  base.z = mr->render.camera.pos_z;
  if (fifa96_ball_pair_receive_target(
          &base, (int32_t)((uint32_t)(uint16_t)cam->vel_x << 16),
          (int32_t)((uint32_t)(uint16_t)cam->vel_z << 16), &target) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  /* 0x7A335..0x7A376: the actor's own-team record scan. */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &team->records[i];
    cands[i].x = (int16_t)e->pos_x;
    cands[i].y = (int16_t)e->pos_z;
    cands[i].skip_98 = e->skip_98;
    cands[i].skip_9a = e->skip_9a;
  }
  memset(&rec, 0, sizeof rec);
  /* 0x7A335..0x7A349: the skip source is the +0x91 action byte (`MOV
   * EAX,dword[EBP+0x8E]; SAR EAX,0x18` selects the dword alias's high byte —
   * the same idiom as dword[+0x69]>>16 = +0x6B); +0x8E is the facing octant
   * (0x79C50) and is not read here, so the helper's `kind` stays 0. */
  rec.action = actor_rec->code;    /* +0x91 byte */
  rec.flag = actor_rec->active;    /* +0x8D byte */
  if (fifa96_ball_pair_receive(&rec, cands, FIFA96_MATCH_ENTITY_RECORDS,
                               (int16_t)target.x, (int16_t)target.z,
                               &index) < 0)
    return -FIFA96_ERR_INVALID;
  /* 0x7A37B..0x7A3A2: the receiver and team-target writes (the native NULL is
   * the engine NONE). */
  pair->receiver =
      index < 0 ? FIFA96_MATCH_ENTITY_NONE
                : (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)index);
  team->target = pair->receiver;
  team->second = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.team[1u - t].target = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.team[1u - t].second = FIFA96_MATCH_ENTITY_NONE;
  /* 0x7A3AC..0x7A3B9: code 3 clears the actor team's +0x7E7 (unmodeled cell,
   * OL-72 — leg). */
  /* 0x7A40A..0x7A412: the actor being [0x158724] (the possession carrier)
   * arms the release countdown [0x15872D] = 0x14. */
  if (actor_id == mr->entities.ball.carrier)
    mr->entities.ball.pos_release = 0x14;
  /* FU276-L2 (phase-26b Task 5): 0x7A419..0x7A448 — the receive arm's own
   * `code` (+0x91) eligibility gate and the FUN_00079D5C offside check call
   * (the former declared leg). The native gate reads the `code` byte
   * (`dword[EBP+0x8E] >> 24` = `byte[+0x91]`, FU-150's `+0x8E>>24` is the alias
   * trap) on the receiving record (EBP = [0x158730] = the actor) and calls the
   * check with EDX = the staged 0x158738 block (its +0 word = tol2, +4 word =
   * side_gate) and EAX = the same record. The check's own [0x14C2F2] settings
   * gate (0x79D87) is the `field_4c2f2` pre-gate here, so the arm only enters
   * when the enable is set: the dormant path (field_4c2f2 == 0, the default)
   * stays a byte-exact no-op. */
  if (mr->config.field_4c2f2 != 0) {
    uint8_t rcode = actor_rec->code;             /* the +0x91 action byte */
    if (rcode != 0x11u && rcode != 0x10u && rcode != 0x1Du && rcode != 0x1Eu) {
      struct fifa96_ref_metric metric;
      uint8_t offside = 0;
      metric.tol2 = (int16_t)vector->x;          /* the 0x158738 block +0 word */
      metric.side_gate = (int16_t)vector->z;     /* the 0x158738 block +4 word */
      /* The camera inputs: native [0x157754] is the camera z (the engine
       * camera.pos_z) and [0x157823] the one-shot pan/keeper mirror latch
       * (the port carries the cell; its native producer is unported). The
       * own-nearest query uses the reception target computed above — the
       * engine stand-in for the native 0x157770 triple. */
      (void)match_run_offside_reception_at(mr, actor_id, target.x, target.z,
                                           &metric, mr->render.camera.pos_z,
                                           mr->flag_157823, &offside);
    }
  }
  return FIFA96_OK;
}

/* ===== M2 phase-11 T2 (L14.8): the FUN_0008BAF0 ball machine ==============
 *
 * First-hand /FIFA96.EXE this task: `disassemble_function 0x8BAF0` (379
 * insns; the tail window 0x8BDEC..0x8C065 re-read), the FUN_0008AF38 gate
 * (0x8B65B..0x8B669), `0x8C24C` (the commit), `0x8C068` (the head helper),
 * `0x6FE84`/`0x6E724` (the appearance machine), the writer census
 * (`search_instructions 15880c / 158830 / 15884a / 158848`) and
 * `read_memory 0x1104D2 / 0x1104AE / 0x110492` (the 0x1104D2 byte-granular
 * octant table, shared as `fifa96_match_heading_table`; the pre-fix
 * "0x1104CF dword-top-byte" phrase was the stride-4 misread, FU-148 §16.5).
 * The native runs the machine from the clock tail
 * (`FUN_0008AF38` 0x8B664) whenever `[0x14C32A] == 0`, after the entity
 * chain; it is the only per-frame producer of the ball record position
 * 0x15880C/14 (the FU-120 writer census: resets, commits, the penalty
 * placement and this tail's `ADD [0x1587EC+0x20/0x28]` integration) — i.e.
 * the ball the composer stages into render slot 22 (`FUN_00036C70` 0x36E12).
 *
 * Ported subset (the phase-2 open-play lead + the common tail):
 *
 *  - entry `[0x15884E] = 5` (0x8BB08).
 *  - phase 2 (0x8BB55..0x8BBFC): when the period byte [0x157AC2] <= 3 the
 *    lead is camera-derived — `x = -3*camZ/8 ± 0x120`, `z = 3*camZ/4 ±
 *    0x120` (y = 0), the ± offsets selected by the sign of `dx = -3*camZ/8 -
 *    camX` / `dz = three_q - camZ` (0x8BBAD) and applied only while
 *    `|(dx,dz)| < 0x120`; that
 *    offset arm stores `[0x15884E] = 7` (0x8BBE6). Period > 3 freezes the
 *    target onto the ball record (0x8BB74). The camera triple is the engine
 *    `render.camera.pos_x/pos_z` (native 0x15774C/0x157754).
 *  - common tail (0x8BDEC..0x8C052): the FUN_0008DCD4 metric of
 *    (target - ball) into 0x15883C (components 0x15883E/0x158840); when the
 *    ball sits on the target the velocity words zero and the integration is
 *    skipped; else:
 *      - target_heading 0x15885E := the metric angle (0x8DD70 = FUN_000CD474;
 *        the ball's own heading when the metric is 0);
 *      - the 0x158881 gate (the FUN_0006E724 row-flags probe: the reset's
 *        row-3 install `FUN_0006E724(0x1587EC, 3, 0)` stores row 3's +2
 *        flags & 2 = 2) admits the heading steer: within 0x60 of the lead the
 *        aim switches to the focus record ([0x157A73], default the camera
 *        triple 0x15774C — engine `render.camera`), and the heading
 *        0x15885A is turned toward the target heading, wrapped 10-bit
 *        (`& 0x3FF`, fold over 0x200) and clamped to
 *        `±((0x10 - speed) << 2)`; the visual byte 0x158852 is the 0x1104D2
 *        octant table (`[EAX+0x1104CF] >> 24` without a SIB scale) at
 *        `((uint16)heading & 0x3FF) >> 5`;
 *      - speed = metric >> 3, clamped by `max_speed` (0x15884E) and, when
 *        the previous speed exceeds max/2, by `max >> (head_error >> 6)`
 *        (0x158862; image default 0 — L14.8.c); the `SAR AX,CL` shift
 *        count is the CL & 0x1F mask (the 8086 uses the full CL; the 186+
 *        and the 386 this 32-bit LE image requires mask to 5 bits — the
 *        `0e19173` commit subject's "8086 CL width" is inverted; the code
 *        is exact);
 *      - the per-axis velocity targets are the 0x114E04 sine fold of the
 *        target heading (`fifa96_entity_sine(a)`,
 *        `fifa96_entity_sine(a + 0x100)`), ramped ±1 whenever the 0x158856
 *        timer rolls (period `speed/2` frames); the speed word 0x158848
 *        := the metric of (vel_x, vel_z) (0x8DC68 = the same primitive);
 *      - the integration: `ball.x += vel_x * delta`, `ball.z += vel_z *
 *        delta` (0x8C036..0x8C052). y is untouched (stays 0).
 *    The appearance/animation machine `FUN_0006FE84` (+0x84 timer, the
 *    0x11052A row successor, the 0x15887B frame byte) is the L14.8 visual
 *    leg — the engine stages the heading and the reset's row-3 appearance;
 *    the FU-85 frame data is synthetic and does not consume the bytes.
 *
 * Legs: L14.8.a the phase-6 goal-area and phase-0xC/0x13/0x14 lead arms and
 * their match-over state words (0x158822; the engine lifecycle owns exits);
 * L14.8.b the FUN_0006FE84/FUN_0006E518 appearance animation machine and the
 * 0x15887B frame byte; L14.8.c the FUN_0008C068 0x158862 head-error producer
 * (modeled as the image-default field); L14.8.d the native `[0x14C32A] == 0`
 * ball-machine gate at 0x8B65B (`CMP byte [0x14C32A],0 / JNZ` around the
 * 0x8B664 `CALL FUN_0008BAF0`) — **resolved by FU-166/R14 (phase-13 T3)**:
 * `[0x14C32A]` is the per-GAME-SELECT-item session flag (every ordinary
 * played match passes mode 0 -> byte 0; byte 1 is the Practise session), and
 * the two consumers are **consistent per session**: byte 0 runs the machine
 * (the `== 0` arm) with the scheduler skipped (`0x4B198 != 0` requires byte
 * != 0), byte 1 the reverse. The engine's settled default is byte 0
 * (`fifa96_match_run_begin`), so the unconditional machine + the gated
 * scheduler are byte-0-exact; under a byte-1 session the native machine is
 * dead and the engine would over-run it (residual; FU-148 §17 errata). */
int32_t fifa96_match_run_ball_head_error_shift(int32_t head_error) {
  /* 0x8BF41..47: the native shift count CL is masked to 5 bits on the 386+
   * this 32-bit LE image requires (the 8086 uses the full CL). */
  return ((head_error >> 6) & 0x1F);
}

static void match_run_ball_machine(struct fifa96_match_run *mr) {
  struct fifa96_match_ball *b = &mr->entities.ball;
  fifa96_camera *cam = &mr->render.camera;
  int32_t delta = (int32_t)mr->state.frame_delta;
  int live_flight = 0;
  b->max_speed = 5;                                         /* 0x8BB08 */
  /* P14 derived keeper outlet: after the P9 save hold, release the keeper's
   * ball toward a teammate/attacking half instead of leaving it pinned. */
  if (mr->live_start && mr->live_save_hold == 0u &&
      mr->live_save_record != FIFA96_MATCH_ENTITY_NONE &&
      (mr->live_save_record % (int32_t)FIFA96_MATCH_ENTITY_RECORDS) == 0) {
    if (mr->entities.ball.carrier == mr->live_save_record)
      match_run_live_keeper_outlet(mr);
    else
      mr->live_save_record = FIFA96_MATCH_ENTITY_NONE;
  }
  /* P8 / live shot block: while the save hold is nonzero, pin the ball on its
   * current record position and skip the phase-2 lead/velocity/integration.
   * The hold is decremented here (one per granted frame); it is live-only and
   * init/begin clear it. This is the derived stand-in for the unported
   * keeper-hold callers (P8-L4); it writes only ball cells. */
  if (mr->live_start && mr->live_save_hold != 0u) {
    mr->live_save_hold--;
    b->target_x = b->x;
    b->target_y = b->y;
    b->target_z = b->z;
    b->vel_x = 0;
    b->vel_z = 0;
    b->speed = 0;
    b->metric = 0;
    b->metric_dx = 0;
    b->metric_dz = 0;
    b->machine_ran = 1;
    return;
  }
  /* Live possession glue [D]. The native ball staging/possession algebra
   * (`0x7A490`, the row-05 `0x7F3A1..0x7F57B` target block) is unported, so
   * the FU-8BAF0 machine only had its loose camera-relative target and a
   * carried ball stayed behind the player. Pin the ball to the carrier's feet
   * (plus the last pad direction, the derived dribble offset) while possession
   * is live; the contact range keeps a stale far carrier from teleporting it.
   * A kick clears the carrier before this runs, so flight/restart behaviour is
   * unchanged. Live-only: the M1/M2 tapes never set live_start. The save-hold
   * block above returns first, so a keeper pin is never moved. */
  if (mr->live_start && mr->state.phase == 2u && mr->live_kick_active == 0u) {
    int32_t carrier = b->carrier;
    if (carrier >= 0 &&
        carrier < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
      struct fifa96_match_entity *c =
          &mr->entities.team[(uint32_t)carrier / FIFA96_MATCH_ENTITY_RECORDS]
               .records[(uint32_t)carrier % FIFA96_MATCH_ENTITY_RECORDS];
      int32_t dx = c->pos_x - b->x;
      int32_t dz = c->pos_z - b->z;
      if (c->active != 0u && c->skip_98 == 0u && c->skip_9a == 0u &&
          fifa96_entity_distance(dx, dz) <= MATCH_RUN_LIVE_CARRIER_GLUE_RANGE) {
        int32_t ox = (int32_t)c->dir_x * 0x20;
        int32_t oz = (int32_t)c->dir_z * 0x20;
        /* Phase-86: when the pad direction is neutral the carrier still faces
         * an octant; keep the ball in front of that facing instead of pinning
         * it inside the sprite (the derived stand-in for the unported native
         * 0x7A490 possession staging). */
        if (ox == 0 && oz == 0) {
          uint32_t oct = (uint32_t)c->type & 7u;
          ox = (int32_t)fifa96_match_dir_x[oct] * 0x20;
          oz = (int32_t)fifa96_match_dir_z[oct] * 0x20;
        }
        b->x = c->pos_x + ox;
        b->z = c->pos_z + oz;
        b->y = 0;
        b->target_x = b->x;
        b->target_y = 0;
        b->target_z = b->z;
        b->vel_x = c->vel_x;
        b->vel_z = c->vel_z;
        b->speed = c->speed71;
        b->metric = 0;
        b->metric_dx = 0;
        b->metric_dz = 0;
        b->machine_ran = 1;
        return;
      }
    }
  }
  /* P7 / FU-345: the live kick flight override (derived; see
   * match_run_live_kick_latch). Non-live runs never set live_start. */
  if (mr->live_start) live_flight = match_run_live_kick_flight_step(mr, b);
  if (mr->state.phase == 2u) {                              /* 0x8BB55 */
    if (live_flight == 0) {
      if (mr->state.period > 3u) {                            /* 0x8BB65 */
        b->target_x = b->x;                                   /* 0x8BB74 */
        b->target_y = b->y;
        b->target_z = b->z;
      } else {
        int32_t cam_z = cam->pos_z;
        int32_t three_q = cam_z - (cam_z >> 2);               /* 0x8BB8A */
        int32_t neg_eighth = (-three_q) >> 1;                 /* 0x8BB8E/96 */
        int32_t tx = neg_eighth;                              /* ECX */
        int32_t tz = three_q;                                 /* EBX */
        int32_t dx = neg_eighth - cam->pos_x;                 /* 0x8BB9E EDI */
        int32_t dz = three_q - cam_z;                         /* 0x8BBAD ESI */
        b->target_y = 0;                                      /* 0x8BBA7 */
        if (fifa96_entity_distance(dx, dz) < 0x120) {         /* 0x8BBBB */
          tx += dx < 0 ? -0x120 : 0x120;                      /* 0x8BBC2..CC */
          tz += dz < 0 ? -0x120 : 0x120;                      /* 0x8BBD4..E0 */
          b->max_speed = 7;                                   /* 0x8BBE6 */
        }
        b->target_x = tx;                                     /* 0x8BBF0 */
        b->target_z = tz;                                     /* 0x8BBF6 */
      }
    }
  }
  /* 0x8BDEC: the FUN_0008DCD4 metric of (target - ball). */
  b->metric_dx = (int16_t)((uint16_t)b->target_x - (uint16_t)b->x);
  b->metric_dz = (int16_t)((uint16_t)b->target_z - (uint16_t)b->z);
  b->metric = (int16_t)fifa96_entity_distance(b->metric_dx, b->metric_dz);
  if (b->metric == 0) {                                     /* 0x8BDFB */
    b->target_heading = b->heading;                         /* 0x8BE20 */
  } else {
    int32_t angle = 0;
    (void)fifa96_entity_angle(b->metric_dx, b->metric_dz, &angle);   /* 0x8BE12 */
    b->target_heading = (int16_t)angle;                     /* 0x8BE25 */
  }
  if (b->head_gate != 0) {                                  /* 0x8BE2A */
    int32_t tgt = b->target_heading;
    if (b->metric <= 0x60) {                                /* 0x8BE42 */
      /* 0x8BE4C..0x8BE82: aim at the focus record [0x157A73] — the default
       * camera triple 0x15774C/54 (the foul-spot override 0x1587C0 is the
       * unported foul machine's, FU-150). */
      int16_t fdx = (int16_t)((uint16_t)cam->pos_x - (uint16_t)b->x);
      int16_t fdz = (int16_t)((uint16_t)cam->pos_z - (uint16_t)b->z);
      if (fdx == 0 && fdz == 0) tgt = b->heading;
      else {
        int32_t fangle = 0;
        (void)fifa96_entity_angle(fdx, fdz, &fangle);
        tgt = (int16_t)fangle;
      }
      b->target_heading = (int16_t)tgt;                     /* 0x8BE25 store */
    }
    {
      int32_t steer = (int16_t)((uint16_t)tgt - (uint16_t)b->heading);
      if (steer != 0) {                                     /* 0x8BE94 */
        steer &= 0x3FF;                                     /* 0x8BE9A */
        if (steer > 0x200) steer -= 0x400;                  /* 0x8BEA0..A5 */
        {
          int32_t turn = (0x10 - (int32_t)b->speed) << 2;   /* 0x8BEAD */
          if (steer > turn) steer = turn;                   /* 0x8BEB9..C3 */
          if (steer < -turn) steer = -turn;                 /* 0x8BEC5..D3 */
        }
        {
          int32_t sum = (int16_t)((uint16_t)b->heading + (uint16_t)steer);
          int32_t u = sum & 0x3FF;                          /* 0x8BEDC */
          b->heading = (int16_t)(u > 0x200 ? u - 0x400 : u); /* 0x8BEE2..F5 */
          /* The 0x1104D2 byte table (read natively as `[EAX+0x1104CF] >> 24`
           * with no SIB scale); shared with the locomotion heading fold. */
          b->visual_dir = (int8_t)fifa96_match_heading_table[u >> 5]; /* 0x8BEFA..F01 */
        }
      }
    }
  }
  {
    int32_t speed = b->metric >> 3;                         /* 0x8BF0F */
    if (speed == 0) {                                       /* 0x8BF1D */
      b->speed = 0;                                         /* 0x8BF1F..27 */
      b->vel_x = 0;
      b->vel_z = 0;
    } else {
      int32_t max = b->max_speed;
      if ((int32_t)b->speed > max / 2)                      /* 0x8BF2F..3F */
        max >>= fifa96_match_run_ball_head_error_shift((int32_t)b->head_error); /* 0x8BF41..47 */
      if (speed > max) speed = max;                         /* 0x8BF4A..4F */
      {
        int32_t vx = (int32_t)(((int64_t)speed *
                                fifa96_entity_sine(b->target_heading) + 0x8000) >> 16);
        int32_t vz = (int32_t)(((int64_t)speed *
                                fifa96_entity_sine((int32_t)b->target_heading + 0x100) +
                                0x8000) >> 16);
        /* 0x8BFC0..0x8C035: the per-axis ±1 ramp on the speed/2 timer. */
        int32_t period = (int32_t)b->speed >> 1;
        b->ramp_timer = (int16_t)(b->ramp_timer + delta);
        if ((int32_t)b->ramp_timer > period) {
          b->ramp_timer = 0;
          if ((int32_t)vx > (int32_t)b->vel_x) b->vel_x = (int16_t)(b->vel_x + 1);
          else if ((int32_t)vx < (int32_t)b->vel_x) b->vel_x = (int16_t)(b->vel_x - 1);
          if ((int32_t)vz > (int32_t)b->vel_z) b->vel_z = (int16_t)(b->vel_z + 1);
          else if ((int32_t)vz < (int32_t)b->vel_z) b->vel_z = (int16_t)(b->vel_z - 1);
        }
        b->speed = (int16_t)fifa96_entity_distance(b->vel_x, b->vel_z); /* 0x8C023 */
      }
      /* 0x8C036..0x8C052: the integration. */
      b->x += (int32_t)b->vel_x * delta;
      b->z += (int32_t)b->vel_z * delta;
    }
  }
  match_run_live_kick_flight_end(mr, b);
  b->machine_ran = 1;
}

/* FU-231 §3: the FUN_0008BAF0 end machine. word 1 = regulation-over (the
 * flag-clear phase-0xC terminal, 0x8BCD9); word 2 = extra-time-over (the
 * phase-0x13/0x14 arm's dwell past `word[0x158818] > 0x4B0` — the arm's `CMP
 * EAX,0x4B0` at 0x8BD52/0x8BDA4; the `word[0x158822]=2` write is
 * 0x8BD8A/0x8BDD4, latched at 0x8BDDD); 0 = none.
 * The no-ET screen 0x37F54 ([0x105FFC]=5) has no engine cell -> FU-231 leg.
 * The `[0x158818]` gate is a SIGNED word compare (native `MOV EAX,[0x158816] ;
 * SAR EAX,0x10 ; CMP EAX,0x4B0 ; JG`), so `mr->act.timer` (int16_t) is compared
 * signed. */
uint16_t fifa96_match_run_end_word(const struct fifa96_match_run *mr) {
  if (!mr) return 0u;
  if (mr->state.phase == 0x0Cu && mr->global_157ac0 == 0u) return 1u;
  /* FU239-L5 (owner FU-237 / FU-239): the native phase-`0x13` arm ALONE carries
   * a score-equality guard — `0x8BD95 MOV AX,[0x157AC5] ; 0x8BD9B CMP AX,
   * word[0x157AC7] ; 0x8BDA2 JNZ 0x8BDE7` (hold) — first-hand here (FU-239 §5;
   * FU-233 §3; t4b-recon/g2-phase-1314-dwell.md:155-164). The phase-`0x14` arm
   * (`0x8BD52`) has NO such test. The engine collapses `0x13`/`0x14` into this
   * single arm and applies no score test, so an ET-over reached with differing
   * scores — the forced-ET tape's terminal (phase `0x13`, score 1-0) — is a
   * **Reading-B model reach**: the `0x13`-vs-`0x14` pick is the unpinned
   * FU237-L1, so the engine's differing-score ET-over is reached via the
   * collapsed arm, NOT strictly the native `0x14` arm. Recorded as a leg, not
   * re-modelled (adding the guard would hold the tape's 1-0 terminal). */
  if ((mr->state.phase == 0x13u || mr->state.phase == 0x14u) &&
      mr->act.timer > 0x4B0) return 2u;
  return 0u;
}

/* FU-232 §4.1/§4.3: the native over-condition of the frame-loop consumer
 * `FUN_0004B100`. `mark_over` (`[0x5FFC]=2`, `0x4B244 MOV EAX,2 ; CALL
 * 0x36BC0`) fires iff the end word `word[0x158822]` is `1` AND the period index
 * `byte[0x157AC2] >= 2` (`0x4B1C6 CMP EAX,2 ; JGE`), except at index `4` with
 * the extra-time flag `byte[0x157AC0] != 0` (`0x4B237 CMP byte[0x157AC0],0 ;
 * JNZ 0x4B2DF` -> no write). Word `2` (the ET-over arm `0x4B257`) is a transient
 * consume, not a match-over (FU-232 §3): it clears the word (`0x4B297 MOV
 * word[0x158822],SI`) and writes no screen, so it returns 0 here. Pure over the
 * live cells; NULL -> 0. */
int fifa96_match_run_should_mark_over(const struct fifa96_match_run *mr) {
  return mr != NULL && mr->end_word_158822 == 1u && mr->global_157ac2 >= 2u &&
         !(mr->global_157ac2 == 4u && mr->global_157ac0 != 0u);
}

/* C3 (design §6.10): the live frame's presentation/camera path. The camera
 * pan/follow/pose feed are presentation state, not match simulation, but they
 * are also live-camera motion: the C3 review moved the frozen replay frame off
 * this helper (it spends the shared match RNG in `fifa96_camera_update_walk`
 * and its FU-71 armer is live camera motion). The frozen frame runs the pose
 * feed plus the replay view driver (`match_run_live_camera_follow`) instead.
 *
 * - FU-148 §12.2 / FU-145 §1.8 (M2 phase-9 T3): the native FUN_000736AC reads
 *   `([0x14C1D4]|[0x14C1D6]) & 4` at 0x70ab1..0x70ac2 for the pan-step walk
 *   gate; the engine derives it from the range words FUN_0001C9BC built. The
 *   image default is 0 (the config cells are BSS).
 * - FU-152 §2.9 / FU-145 §1.2 (M2 phase-9 T3): the every-frame FUN_00071DF4
 *   call site (FUN_000736AC 0x73B5B..0x73B6B) runs after the integrator and
 *   before the armer. The first arm follows the tracked record's slot dir
 *   bytes (`[[rec]+0x20] != 0`, event height > 0xF0) into the camera velocity;
 *   the record class gate `[[rec]+4][0] == 0x18D8` reduces to the pool-record
 *   identity (all three writers of [0x1577CA] store pool records or 0 — 0x71D27
 *   the store, 0x70258/0x70054 the clears; the +4 descriptor is built at
 *   runtime, statically unresolvable).
 * - FU-148 §2.1(a)/§6.2 (S4): the FUN_000505D0 pose feed. The native driver
 *   FUN_0004D2D4 runs from the draw loop (FUN_000495B0) with the replay/
 *   `[0x107DD8]`/pad-idle gates (legs); the engine applies the staged pose
 *   here, after the FU-71 update and before the armer, so a fed pose is
 *   observable to the same frame's armer (the engine `pos` doubles as the
 *   native 0x15774C event target per the FU-71 port mapping). view_mode 0
 *   (the fresh-match default) is the unported handler arm: a no-op, so the
 *   static tape is unaffected.
 *
 * Returns the camera-follow verdict for this frame's trace line. */
static int match_run_frame_camera(struct fifa96_match_run *mr) {
  uint16_t range = (uint16_t)(mr->input_range[0] | mr->input_range[1]);
  fifa96_camera_save_state *save = &mr->render.camera.save;
  int32_t controlled = mr->entities.controlled;
  fifa96_camera_stat_events save_stats;
  /* Slice-B Task 9 + Task 6 + Task 5: the native order is the
   * `0x736B2..0x73722` head jitter -> the `0x73723 CALL 0x92864` commit and
   * 0x73728..0x73750 shadow stores -> the 0x73756 save-head gate / tracker ->
   * the 0x72AEA..0x736A9 save tail -> the 0x7375B..0x7375D prev-triple capture
   * -> the 0x7375E pause gate (`[0x157820]`) -> the walk. */
  (void)fifa96_camera_walk_jitter(&mr->render.camera, &mr->rng);
  (void)fifa96_camera_commit(&mr->render.camera);
  /* 0x72ACA..0x72AEA: FUN_00072478 sets the latch at its own 0x724C8 head;
   * do not write it a second time here. */
  if (mr->goal_zone == 0 || mr->goal_armed == 0) {
    mr->tracker_15759d = 0;
  } else if (mr->tracker_15759d == 0) {
    (void)fifa96_match_tracker_decision(mr);
  }

  /* Engine-to-loader staging for the save tail. The two native side reads are
   * team back-pointer +0x826 reads; ids 0..21 map to the two engine teams. */
  save->phase_157a4d = mr->state.phase;
  save->frame_delta_157a64 = mr->state.frame_delta;
  save->controlled_157a83 = controlled;
  save->controlled_side =
      (controlled >= 0 &&
       controlled < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS *
                              FIFA96_MATCH_ENTITY_RECORDS))
          ? mr->entities.team[controlled / FIFA96_MATCH_ENTITY_RECORDS].side
          : 0;
  save->tracked_side =
      (save->tracked_157a87 >= 0 &&
       save->tracked_157a87 < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS *
                                        FIFA96_MATCH_ENTITY_RECORDS))
          ? mr->entities.team[save->tracked_157a87 /
                              FIFA96_MATCH_ENTITY_RECORDS].side
          : 0;
  save->rng_92ac8 = &mr->rng;
  save->tick_112e88 = (int32_t)mr->ticks;
  save->side_swap_157abe = mr->side_swap;
  save->global_158743 = mr->entities.ball.pair.code;
  /* controlled_class/class_known and actor_15b33c/actor_known are not forced:
   * their unported producers remain unknown rather than known zero. */
  save->controlled_pos[0] = 0;
  save->controlled_pos[1] = 0;
  save->controlled_pos[2] = 0;
  save->controlled_vel73 = 0;
  save->controlled_vel75 = 0;
  save->opponent_target = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  save->opponent_pos[0] = 0;
  save->opponent_pos[1] = 0;
  save->opponent_pos[2] = 0;
  save->opponent_lane_x = 0;
  if (controlled >= 0 &&
      controlled < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS *
                             FIFA96_MATCH_ENTITY_RECORDS)) {
    const struct fifa96_match_entity *actor =
        &mr->entities.team[controlled / FIFA96_MATCH_ENTITY_RECORDS]
             .records[controlled % FIFA96_MATCH_ENTITY_RECORDS];
    uint32_t opponent_team =
        1u - (uint32_t)controlled / FIFA96_MATCH_ENTITY_RECORDS;
    int32_t opponent = mr->entities.team[opponent_team].target;
    save->controlled_pos[0] = actor->pos_x;
    save->controlled_pos[1] = actor->pos_y;
    save->controlled_pos[2] = actor->pos_z;
    save->controlled_vel73 = actor->vel73;
    save->controlled_vel75 = actor->vel75;
    save->opponent_target = opponent;
    if (opponent >= 0 &&
        opponent < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS *
                             FIFA96_MATCH_ENTITY_RECORDS)) {
      const struct fifa96_match_entity *other =
          &mr->entities.team[opponent / FIFA96_MATCH_ENTITY_RECORDS]
               .records[opponent % FIFA96_MATCH_ENTITY_RECORDS];
      save->opponent_pos[0] = other->pos_x;
      save->opponent_pos[1] = other->pos_y;
      save->opponent_pos[2] = other->pos_z;
      save->opponent_lane_x = other->lane_x;
    }
  }
  /* These cells already have goal/armer consumers in the run. Bind their
   * current values in both directions rather than creating duplicate latches. */
  save->prev_x_15759f = mr->bounce_15759f;
  save->prev_z_1575a7 = mr->bounce_1575a7;
  save->band_1575cc = mr->bounce_1575cc;
  save->dz_1575d0 = mr->bounce_1575d0;
  save->approach_15758d = mr->bounce_15758d;
  save->flag_1575e8 = mr->flag_1575e8;
  save->age_157a97 = mr->screen_actor_age;
  memset(&save_stats, 0, sizeof save_stats);
  (void)fifa96_camera_save_tail(&mr->render.camera, &save_stats);
  /* REC_1575DA remains an OPEN drain kind: both events are emitted by the
   * loader and honestly skipped here until its stats-table projection exists. */
  (void)fifa96_match_run_stats_drain(mr, &save_stats, mr->record.entity_id);
  mr->bounce_15759f = save->prev_x_15759f;
  mr->bounce_1575a7 = save->prev_z_1575a7;
  mr->bounce_1575cc = save->band_1575cc;
  mr->bounce_1575d0 = save->dz_1575d0;
  mr->bounce_15758d = save->approach_15758d;
  mr->flag_1575e8 = save->flag_1575e8;
  mr->screen_actor_age = save->age_157a97;

  mr->render.camera.paused = mr->flag_157820;
  (void)fifa96_camera_update_walk(&mr->render.camera, &mr->rng,
                                  (int16_t)mr->state.frame_delta,
                                  mr->difficulty_14c2f6 != 0,
                                  mr->render.input_bit2,
                                  (int)(range & 4u));
  int follow_ran = fifa96_match_run_camera_follow(mr);
  (void)fifa96_camera_pose_feed(&mr->render.camera, &mr->render.yaw,
                                &mr->render.pitch, &mr->render.view_ratio,
                                &mr->render.camera_pose);
  return follow_ran;
}

/* FU313 §4.3: the per-granted-frame camera line at the loop tail. The follow
 * verdict is the frame's saved return, so `follow` pairs this frame's verdict
 * with the event/pan cells as the frame left them; `tick` is the 100 Hz
 * counter for sub-second capture alignment. Default-off: one NULL test when
 * the sink is not open. */
static void match_run_frame_camera_trace(struct fifa96_match_run *mr,
                                         int follow_ran) {
  if (!mr->cam_trace) return;
  const struct fifa96_camera *cam = &mr->render.camera;
  fprintf(mr->cam_trace,
          "f=%u clock=%02u:%02u ph=%u pos=%d,%d,%d vel=%d,%d spd=%u "
          "fspd=%d ev=0x%X cur=0x%X trk=%d pan=%u tim=%u/%u ramp=%d "
          "rate=%d,%d follow=%d tick=%u\n",
          mr->cam_trace_frame++, (unsigned)(mr->state.total_seconds / 60u),
          (unsigned)(mr->state.total_seconds % 60u),
          (unsigned)mr->state.phase, cam->pos_x, cam->pos_y, cam->pos_z,
          (int)cam->vel_x, (int)cam->vel_z, (unsigned)cam->speed,
          (int)cam->follow_speed, (unsigned)cam->event_param,
          (unsigned)cam->event_cursor, (int)cam->tracked,
          (unsigned)cam->pan_counter, (unsigned)cam->timer,
          (unsigned)cam->timer_limit, (int)cam->ramp_divisor,
          (int)cam->rate_x, (int)cam->rate_z, follow_ran,
          (unsigned)mr->ticks);
  fflush(mr->cam_trace);
}

int fifa96_match_run_frame(struct fifa96_match_run *mr) {
  uint32_t pending;
  uint32_t granted;
  int period_ended = 0;
  int rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (mr->sub_notice_ticks != 0u) mr->sub_notice_ticks--;
  /* C3 (design §6.10): the live-match freeze. The native draw loop
   * FUN_000495B0 tests FUN_00063FD0 at 0x4974F and, while the replay family
   * is set, runs the replay path and skips the live path. The engine frame
   * branches here the same way: the pace grant is consumed directly (the
   * state library couples its grant to the clock, which would starve the
   * replay step), `state.frame_delta` is the same whole-frame 0x200 >> 8 the
   * library would stage, and the replay step runs with it. The clock,
   * `tick_total`, the kickoff timers, the live input/control-slot model and
   * the entity/ball pool are live-only and do not run, so the match is frozen
   * until the exit (the next non-family frame resumes
   * `fifa96_match_state_tick`). The HUD sub-notice still ages (the
   * `sub_notice_ticks` decrement runs before this branch), while the
   * `mark_over` tail is part of the skipped live path: the OVER screen is
   * deferred until the first post-exit frame, so a replay cannot end the
   * match. */
  if (fifa96_match_run_replay_family(&mr->render.replay)) {
    int frozen_granted = 0;
    rc = fifa96_match_pace_tick(&mr->pace, 0, &frozen_granted);
    if (rc != 0) return rc;
    if (frozen_granted == 0) return 0;
    mr->state.frame_delta = (uint16_t)(FIFA96_MATCH_STATE_STEP >> 8);
    for (uint32_t i = 0; i < (uint32_t)frozen_granted; i++) {
      (void)fifa96_match_run_replay_step(mr, mr->render.replay.input_buttons);
      mr->render.replay.input_buttons = 0;
      /* Review F1: the replay view. `replay_target` selects the source-4
       * camera on the 0x81 arm; the 0x20 cycle only stores the camera-record
       * selection (`camera_kind`/`camera_sub`/`record_index`), which the view
       * does not consume (leg). `match_run_live_camera_follow` is the only
       * source-4 camera/view consumer and live `render_eye_*` writer;
       * `replay_exit` only resets the source. Without the driver a frozen
       * live replay stays on the frozen live view and the camera cycle
       * changes only the caption. The pose feed keeps the staged orientation
       * input in the live-frame order (pose before the view driver). Review
       * F3: the live pan/event camera walk (`match_run_frame_camera`) is NOT
       * run on the frozen path — it spends the shared match RNG on pan/event
       * state the replay driver replaces, and its FU-71 armer is live camera
       * motion. The frozen view is the replay driver plus the pose feed, so
       * the match RNG stream is untouched by replay frames. */
      (void)fifa96_camera_pose_feed(&mr->render.camera, &mr->render.yaw,
                                    &mr->render.pitch, &mr->render.view_ratio,
                                    &mr->render.camera_pose);
      /* The live view driver (source-4 replay camera) runs only for a live
       * run on a non-POSE replay camera; the POSE camera draws the recorded
       * matrix instead. A non-live frozen run keeps the pose feed above and
       * nothing else. */
      if (mr->live_start &&
          mr->render.replay.camera_kind != FIFA96_CAMERA_REPLAY_POSE)
        match_run_live_camera_follow(mr);
      /* The POSE feed lands after the pose feed/driver so the recorded matrix
       * wins over the live pose feed (spec §7.5). */
      fifa96_match_replay_pose(mr);
      (void)fifa96_match_display_update(&mr->render.display,
                                        mr->state.frame_delta, 0);
      /* Review F2: stage the recorded view (the frozen past), not the live
       * pool. The animator half of the live staging writes the sim-visible
       * +0x28/+0x3D record fields the entity machine reads, which would let
       * the first post-replay frame resolve a different row/aux word; the
       * replay stage writes render cells only (and the ball roll via the
       * applied roll byte). */
      if (mr->render.enabled) fifa96_match_replay_stage(mr);
      /* `follow=0` records that the FU-71 armer/follow did not run on the
       * frozen path (the live `match_run_frame_camera` is skipped). */
      match_run_frame_camera_trace(mr, 0);
    }
    return 1;
  }
  /* One 100 Hz pace tick (FU-60), driven from fifa96_match_run_tick:
   * fifa96_match_state_tick consumes the pace grant and, when granted,
   * advances the Q8 clock by one FIFA96_MATCH_STATE_STEP with clock_halt = 0.
   * The pending count is the grant observable, so this reports whether the
   * 30 Hz frame actually ran. */
  pending = fifa96_match_pace_pending(&mr->pace);
  rc = fifa96_match_state_tick(&mr->state, &mr->pace, 0, 0, &period_ended);
  if (rc != 0) return rc;
  granted = fifa96_match_pace_pending(&mr->pace) - pending;
  /* FU-143 wiring (Task 3): stage the FU-62 clock's derived completion
   * (`sec == limit + aux`, fifa96_match_state.c) for the granted frame's
   * phase driver; a completion can only happen on a granted frame. */
  mr->clock_period_ended = (uint8_t)period_ended;
  /* FU-70 §1.1: the slot machine runs from the frame body once per granted
   * 30 Hz frame with the FU-62 §4.3 whole-frame delta (2 at the 0x200 step,
   * i.e. 60 counter units/s). FU-71's FUN_000736AC and the FU-90 display
   * update run on the same frame-body cadence (the original calls the camera
   * from FUN_0004B100, not from the render driver). One pace tick grants at
   * most one frame today; the loop keeps one update per grant if that ever
   * changes. */
  for (uint32_t i = 0; i < granted; i++) {
    /* FU-143 §10/§11 (M2 playable-match Task 2 / OL-84 residual): the act-1
     * (phase-0x17 handler FUN_00088DC8) stage-1 derived remainder. The native
     * body ticks the shared timeline timer [0x58818] by the frame delta and at
     * `[0x58818] >= 0x78` sets [0x5882A]=1 (0x88EF3..0x88F07), the gate action
     * row 01 stage 0 reads (0x7DC6B). begin models the act's stage 0 (phase
     * entry + placement), so only the timer completion is derived here; the
     * engine's `state.tick_total` is the same whole-delta accumulation
     * ([0x58818] += [0x57A64], first-hand FU-81 §1.5) and begin zeroes it. */
    if (mr->state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE &&
        mr->state.tick_total >= 0x78u)
      mr->global_5882a = 1u;
    /* FU-139 §11: the native FUN_0004B100 toggles [0x157A4F] at frame entry
     * (0x4B11A/0x4B129), before the entity chain; row 06's claim arm reads it. */
    mr->pass_parity ^= 1u;
    (void)fifa96_control_slot_update(&mr->slot, mr->input_state[0],
                                     (uint8_t)mr->state.frame_delta, match_run_slot_map,
                                     match_run_anim_a, match_run_anim_b, match_run_anim_c);
    int follow_ran = match_run_frame_camera(mr);
    /* FU-145 S2 (0x73B70..0x73B9B): the native camera track FUN_000736AC
     * calls the boundary handler FUN_0007131C only when the camera is out of
     * bounds (|camX| > 0x6C0 || |camZ| > 0xAB0); the armer's own
     * phase/bound gates run inside. Runs after the camera update, before the
     * entity chain (the FUN_000736AC position inside FUN_0004B100). */
    if (fifa96_camera_out_of_bounds(mr->render.camera.pos_x,
                                    mr->render.camera.pos_z) &&
        (!mr->live_start ||
         fifa96_match_goal_zone(mr->entities.ball.x, mr->entities.ball.y,
                                mr->entities.ball.z)))
      (void)fifa96_match_goal_arm(mr);
    (void)fifa96_match_display_update(&mr->render.display, mr->state.frame_delta, 0);
    /* FU-141: the FU-67 entity chain (selection, record walk with the FU-137
     * action dispatch, ball pairing) after the slot/camera updates, matching
     * FUN_0004B100's order. FU281 (phase-26c T6): this chain IS the native
     * `FUN_0008D8EC` pre-pass reconciled by AUGMENT — its per-record `action`
     * call is the single native dispatch (`FUN_000782D0`/`FUN_0007CA54` ->
     * `CALL dword[rec+0x18]`), and `fifa96_match_entities_team_update` hosts
     * the pre-pass state (`+0x82C` counter, phase-2 target/intercept/flag), so
     * no standalone pre-pass is wired. */
    struct fifa96_match_entities_frame ef;
    match_run_entity_frame(mr, &ef);
    /* FU296 Seam A + FU299 (phase-28 T2, Option B): hand the record-0 machine
     * view to the walk, which steps it in the `i == 0` branch before the single
     * record dispatch. `side_xor` is a per-frame config cell; the three process
     * cells (`[0x157C5D]`/`[0x157C5C]`/`[0x157AB2]`) travel as pointers and are
     * bound/published PER STEP inside the walk — bind-before/publish-after the
     * step, reproducing the native gate -> ladder -> clear -> dispatch order
     * (FU299 §4), so a row-`0x19` set survives to the next step's gate read
     * (the one-step pulse; team 1's step sees team 0's same-frame set). */
    mr->keeper_cpu.side_xor = mr->side_swap;
    ef.keeper_cpu = &mr->keeper_cpu;
    ef.interlock_157c5d = &mr->global_157c5d;
    ef.latch_157c5c = &mr->global_157c5c;
    ef.latch_157ab2 = &mr->keeper_latch_157ab2;
    ef.stamp_157c2c = &mr->global_157c2c;   /* FU300 T4 / G2 */
    /* Phase 72: the live off-ball cell shape, before the entity chain so the
     * targets are consumed by the same frame's mover. */
    if (mr->live_start) {
      match_run_live_offball_shape(mr);
      match_run_live_support_run(mr);
    }
    rc = fifa96_match_entities_update(&mr->entities, &ef, match_run_dispatch_entity,
                                      mr);
    if (rc != FIFA96_OK) return rc;
    match_run_entity_drain(mr);
    /* P7 / FU-345: the live control follow, after the entity drain so a
     * possession/merge request issued this frame is consumed first. It keeps
     * `entities.controlled`, the FU-70 slot, the team target and the tracked
     * camera on the controlled side's carrier (else its nearest outfield
     * record). Live-only; the M1/M2 headless tapes never set live_start. */
    if (mr->live_start) match_run_live_control_update(mr);
    /* FUN_0008BAF0 0x8BC43: every phase-0xC frame calls FUN_00036200(3), so
     * the camera target source is the ball slot for the period-end view. */
    if (mr->state.phase == 0x0Cu) mr->render.camera_target.source = 3;
    /* FU-343 (P1): the live focus follow, after the entity drain and before
     * the referee/phase tail (the frame draws this camera). Non-live runs keep
     * the event/pan-only camera (M2 byte-identical). */
    if (mr->live_start) match_run_live_camera_follow(mr);
    /* P8 / [D]: the live opponent contest. Runs after this frame's camera
     * follow (the third-template anchor is camera.pos + camera.vel*32) and
     * after the entity chain, so the re-selected opponent target is consumed by
     * the next frame's outfield machine / row-05 dispatch. Live-only. */
    if (mr->live_start) match_run_live_opponent_update(mr);
    /* P14 derived CPU carrier action after the target rotation (live-only). */
    if (mr->live_start) match_run_live_cpu_update(mr);
    /* FU-150 P2: the referee machine (the native phase-body/act invocation
     * runs inside the entity/phase chain) once per granted frame. Idle when no
     * machine is active, so a fresh run's tape is unchanged. */
    rc = fifa96_match_run_referee_step(mr);
    if (rc < 0) return rc;
    /* FU-142a: the FUN_0008D098 state 0x13/0x14 arm block runs after the
     * FU-141 chain, once per granted frame (the FUN_000740A0 order: the
     * state byte is set, then FUN_0008D098 runs for both teams). */
    if (mr->state.phase == 0x13u || mr->state.phase == 0x14u)
      (void)fifa96_match_phase_machine_step(mr);
    /* FU-146 §7 item 7 / 0x4B198..0x4B1A6: the native session-gated scheduler
     * runs after the camera track (0x4B193 FUN_000736AC, which carries the
     * armer) and before the clock body FUN_0008AF38 (the engine's
     * phase_drive + goal_scan). The native order means a goal queued by this
     * frame's scan is consumed by the next frame's handler. */
    if (mr->session_gate_14c32a) {
      rc = fifa96_match_run_screen_schedule(mr);
      if (rc < 0) return rc;
    }
    /* FU-143 wiring (Task 3): the clock's phase funnel, after the entity chain
     * (the native FUN_0008AF38 runs after the FU-67/entity block at 0x4B1A6
     * and inside it calls FUN_0008B9CC at 0x8B574). A completion staged above
     * runs the derived selector-0 chooser (2 -> 0x0C) on this frame. */
    rc = fifa96_match_run_phase_drive(mr);
    if (rc < 0) return rc;
    /* FU-239 §1/§3/§4 + FU-235 §1.4 (M2 phase-22 T4): the extra-time-entry
     * screen dwell + the live re-arm. The chooser parks the ET entry in the
     * class-0 phase `0x13`/`0x14` and installs the act-`0xC` screen (`0x8B900`,
     * the three-stage timer); the ET-entry screen holds until that screen's
     * cycle completes (the act slot clears, `0x8B9B3`/`0x8B9BF`), then the
     * period-over re-arm lands the live half in the class-1 play phase `2`
     * (FU-239 §3: the only class-1 phase with a static writer). The exact
     * native route `0x13/0x14 -> 0x0C -> sit-8 -> phase 2` is FU239-L1 (the
     * situation dispatch is indirect); the dwell bound is that act-`0xC`
     * screen completion, > the v21 20-frame window. Gated on the live ET-entry
     * state (`global_157ac0 == 1`, the differing regulation end's roll).
     * M2 phase-22 T4b (FU-239 / g8 §3.1): the index term is the ET-play window
     * `{2,3}`, NOT `>= 2`. The native re-arms once per ET half (at the ET entry
     * index 2 and the ET-half-1 -> ET-half-2 boundary index 3; `0x8B58A`), but
     * at the ET-half-2 COMPLETION (index 4) the chooser parks phase
     * `0x13`/`0x14` and the native takes the post-ET terminal (the end machine
     * `word[0x158822]=2`), never another re-arm. The un-bounded `>= 2` stayed
     * true as `global_157ac2` climbed 2->3->4->5, so the re-arm re-fired after
     * every half and the run held at `period=5, phase=2`. A bare phase-
     * `0x13`/`0x14` state (the forced kickoff window) still does not re-arm. */
    if ((mr->state.phase == 0x13u || mr->state.phase == 0x14u) &&
        mr->act.id == 0 && mr->global_157ac0 == 1u &&
        (mr->global_157ac2 == 2u || mr->global_157ac2 == 3u))
      fifa96_match_run_period_over_rearm(mr);
    /* P8 / [D]: the live defender/keeper shot block. It must see the same
     * frame-start ball as the armer and clear the flight before the armer, so
     * a blocked shot never reaches the goal cells. Live-only. The P8 outfield
     * defender scan runs first; the P9 keeper save (record 0 of the defending
     * side) runs immediately after it, so a defender on the ball keeps the
     * block authority and the keeper stops the centre shot the outfielders
     * miss. */
    if (mr->live_start) {
      (void)match_run_live_shot_block(mr);
      (void)match_run_live_keeper_save(mr);
    }
    /* P6 task 2 / P6-L1 (FU-344): the live ball-position armer runs after
     * this frame's phase/entity ball update and immediately before the native
     * clock-tail scan (0x8B623 FUN_00088940), so a ball in the mouth arms and
     * is consumed by the same scan. Live-only; the M2 tape never sets
     * live_start. */
    match_run_live_goal_arm(mr);
    /* FU-145 S2 (0x8B63E): the native clock tail FUN_0008AF38 calls the goal
     * scanner FUN_00088940 after the period-end handling; the derived scan
     * carries the phase/armed gate and queues situation 6 (or the wired
     * table-2 fallback). */
    rc = fifa96_match_run_goal_scan(mr);
    if (rc < 0) return rc;
    /* M2 phase-11 T2 (L14.8): the FUN_0008BAF0 ball machine — the native
     * clock tail (FUN_0008AF38 0x8B664, after the period/goal block) and the
     * only per-frame producer of the ball record position the render slot 22
     * stages. Runs after the goal scan, before the scene staging, so the
     * frame draws the advanced record. */
    match_run_ball_machine(mr);
    /* FU-235 §1.1/§4 (0x8BC13, M2 phase-22 T4): the phase-`0xC` terminal
     * dwell. While the match-over screen holds phase `0x0C` and no act is
     * installed, the end machine accrues the end/act timer `word[0x158818]`
     * by the frame delta (`0x8BC0A CMP dword[0x158808],0 ; JNZ 0x8BC20` skips
     * the accumulate when an act is installed). The accrued timer is what lets
     * the staged end word reach `>= 0x78` and the `mark_over` screen dwell
     * past `0x168` below. M2 phase-22 T4b (FU-231 §3 / g2 §1, native `0x8BCFE`):
     * the SAME cell is accrued by the end machine's phase-`0x13`/`0x14` arm
     * (`0x8BCFE MOV AX,[0x157A64] ; 0x8BD04 ADD word[0x158818],AX`, gated the
     * same way on no act) — the ET-over dwell. Extending the gate to `0x13`/
     * `0x14` is what lets `act.timer` pass `0x4B0` so the ET-over word `2`
     * stages (`fifa96_match_run_end_word`). Without it the ET path caps at the
     * act-`0xC` reset (~0xD6) and the ET-over is unreachable. */
    if ((mr->state.phase == 0x0Cu || mr->state.phase == 0x13u ||
         mr->state.phase == 0x14u) && mr->act.id == 0)
      mr->act.timer = (int16_t)(mr->act.timer + (int16_t)mr->state.frame_delta);
    /* FU-231 §3 (M2 phase-22 T2): the FUN_0008BAF0 end machine — the native
     * clock-tail call at `0x8B664` (after the period/goal block, before the
     * act tick at `0x8B669`). Stage the end word `[0x158822]` once. The
     * consumer absorber `0x4B1C6 CMP [0x157AC2],2` (past half-time) and the
     * FU-235 end-timer dwell (`mr->act.timer` = `word[0x158818]`, `>= 0x78`)
     * gate the write, so the half-time phase-`0xC` terminal cannot fire the
     * word early (FU-234-L7: never the raw `period_ended`). The no-ET screen
     * `0x37F54` (`[0x105FFC]=5`) has no engine cell -> leg FU231-L2. */
    if (mr->end_word_158822 == 0u && mr->global_157ac2 >= 2u &&
        mr->act.timer >= 0x78) {
      mr->end_word_158822 = fifa96_match_run_end_word(mr);
    }
    /* FU-232 §3 (M2 phase-22 T3): the ET-over consume. `word[0x158822]==2` is
     * not a match-over — the native `AX==2` arm (`0x4B257`) writes no `[0x5FFC]`
     * screen and clears the word (`0x4B297 XOR ESI,ESI ; MOV word[0x158822],SI`),
     * so the frame loop resumes. The frame that stages the ET-over consumes it
     * here. */
    if (mr->end_word_158822 == 2u)
      mr->end_word_158822 = 0u;
    /* Phase 70: the native ball appearance pass runs before the act handler
     * (`0x8B664 CALL 0x8BAF0` before `0x8B672 CALL [0x158808]`), so a settle
     * that ends this frame is visible to this frame's act tick. */
    match_run_ball_settle_step(mr);
    /* M2 phase-15 T1 / FU-175 §2: the native act tick — the clock tail's
     * `0x8B669 CMP dword[0x158808],0 / 0x8B670 JZ / 0x8B672 CALL
     * dword[0x158808]` — runs after the ball machine (`0x8B664 CALL 0x8BAF0`)
     * and inside the byte-0 arm, ungated by phase. `fifa96_match_act_tick`
     * runs the installed act handler through `fifa96_match_dispatch_phase`
     * (the act-9 phase-0x1F row sets `[0x15882A]` at its stage-3 timer, the
     * leg-0 GATE producer). The FU-170 §7 ad-hoc `act9_armed`/`act9_timer`
     * pair is absorbed into the act slot. */
    rc = fifa96_match_act_tick(mr);
    if (rc < 0) return rc;
    /* FU-85 §4: FUN_00036C70 stages the render slots after the FU-4B100
     * update chain (0x49523, after the 0x49514 CALL 0x4B100), so the staged
     * positions/animations reflect this frame. Rendering stays opt-in: the
     * Task 12-14 fixtures keep an unstaged scene. */
    if (mr->render.enabled) {
      match_run_scene_stage(mr);
      /* RR capture: compose the staged render slots into the view block and
       * record one live frame, so the ring holds exactly the frame the
       * renderer draws. The composer must run after the scene staging; a
       * headless run (`render.enabled == 0`) stages no slots and captures
       * nothing. */
      fifa96_match_replay_compose(mr);
      fifa96_match_replay_capture(mr, 1);
    }
    match_run_frame_camera_trace(mr, follow_ran);
  }
  /* FU-232 §4.1/§4.3 (M2 phase-22 T3): the native frame-loop `mark_over` tail.
   * The trigger is the end-word consumer's exact condition — word 1
   * (regulation-over) with the period index `>= 2`, except index 4 while the
   * extra-time flag is set (`0x4B244`, `0x4B237`) — single-sourced in
   * `fifa96_match_run_should_mark_over`, which supersedes the phase-21
   * `end_word_decided` proxy (errata E4 / FU228-L4). That proxy keyed off
   * `state.phase == 0x0C && flag clear` rather than the word, so it could mark
   * over a state whose `end_word_158822` was not `1`; the word is what the T2
   * end-machine staging above writes (FU-231 §3) and this tail reads.
   * Word 2 is consumed in the frame body (FU-232 §3) and never reaches here.
   * M2 phase-22 T4 (FU-235 §1.2/§4): the tail also requires the phase-`0xC`
   * terminal dwell (`mr->act.timer` = `word[0x158818] > 0x168`, the native
   * no-input terminal gate `0x8BCAC`) — the OVER screen is held through the
   * dwell, and `period_ended` is dropped because the dwell keeps the terminal
   * phase `0x0C` across frames (it is no longer a one-frame completion event).
   * Ordering guard (phase-21 final review): a staged `request_exit` must not be
   * clobbered by EXIT->OVER (a staged EXIT wins). */
  if (fifa96_match_run_should_mark_over(mr) && mr->act.timer > 0x168 &&
      mr->lc.screen == FIFA96_MATCH_SCREEN_ACTIVE &&
      !fifa96_match_lifecycle_should_exit(&mr->lc)) {
    rc = fifa96_match_lifecycle_mark_over(&mr->lc);
    if (rc != 0) return rc;
  }
  return granted != 0;
}

int fifa96_match_run_resolve(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (mr->lc.screen == FIFA96_MATCH_SCREEN_OVER) {
    (void)fifa96_match_lifecycle_resolve_over(&mr->lc);   /* OVER -> POST */
  }
  if (mr->lc.screen == FIFA96_MATCH_SCREEN_POST) {
    mr->result_pending = 1;
    (void)fifa96_match_lifecycle_request_exit(&mr->lc);   /* POST -> EXIT */
  }
  if (fifa96_match_lifecycle_should_exit(&mr->lc)) return fifa96_match_run_end(mr);
  return 0;
}

int fifa96_match_run_set_paused(struct fifa96_match_run *mr, int paused) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (!mr->live_start) return -FIFA96_ERR_UNSUPPORTED;
  if (paused) {
    mr->paused = 1;
    (void)fifa96_match_pace_hold(&mr->pace);
  } else {
    mr->paused = 0;
    match_run_sub_menu_reset(mr);   /* P15: resume closes the menu */
    (void)fifa96_match_pace_resume(&mr->pace);
  }
  return 0;
}

/* Pitch-record -> roster-slot map. With retail PCCD data the bridge supplies
 * the native 16-slot desired order (0x14C360 / FUN_00012010) in
 * `roster_order`; the first 11 entries are the starting line-up, exactly the
 * `FUN_0008C758` binding `rec+4 = 0x157BD2[side][rec+0x8D]`.
 *
 * The derived fallback (no PCCD data / direct begin fixtures) walks the native
 * 0x11033A role layout and selects, for each pitch record in block order, the
 * highest-rated unused player with the block's position, then the
 * highest-rated unused non-GK, then any unused player. The map is immutable
 * after the first substitution; a substitution still rebuilds the per-pitch
 * appearance remap from the updated map (FU-348). */
int fifa96_match_run_sub_rebuild(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (mr->sub_used[0] != 0 || mr->sub_used[1] != 0) return 0;

  for (uint32_t side = 0; side < FIFA96_MATCH_ENTITY_TEAMS; side++) {
    uint8_t used[FIFA96_PCCD_ROSTER_COUNT];
    memset(used, 0, sizeof used);
    memset(mr->sub_lineup[side], 0xFF, sizeof mr->sub_lineup[side]);

    /* The bridge supplies the native FUN_00012010 order when retail PCCD data
     * is present. Validate it as a permutation before trusting it; a direct
     * begin fixture or malformed handoff falls back to the derived builder. */
    int native = mr->roster_order_ok[side] != 0;
    if (native) {
      uint8_t seen_roster[FIFA96_PCCD_ROSTER_COUNT] = {0};
      for (uint32_t i = 0; i < FIFA96_PCCD_ROSTER_COUNT; i++) {
        uint8_t roster = mr->roster_order[side][i];
        if (roster >= FIFA96_PCCD_ROSTER_COUNT || seen_roster[roster]) {
          native = 0;
          break;
        }
        seen_roster[roster] = 1;
      }
    }
    if (native) {
      /* Native FUN_0008C758: current order[pitch] is the roster descriptor
       * for pitch record `pitch`. */
      for (uint8_t pitch = 0; pitch < FIFA96_MATCH_ENTITY_RECORDS; pitch++) {
        uint8_t roster = mr->roster_order[side][pitch];
        mr->sub_lineup[side][pitch] = roster;
        used[roster] = 1;
      }
    } else {
      uint8_t pitch_role[FIFA96_MATCH_ENTITY_RECORDS];
      uint8_t order[FIFA96_MATCH_ENTITY_RECORDS];
      uint8_t seen[FIFA96_MATCH_ENTITY_RECORDS];
      int order_count = 0;
      memset(pitch_role, 0, sizeof pitch_role);
      memset(order, 0, sizeof order);
      memset(seen, 0, sizeof seen);

      uint8_t id = mr->formation[side] <= 4u ? mr->formation[side] : 0u;
      fifa96_match_formation_block blocks[4];
      int block_count = fifa96_match_formation_layout(id, blocks);
      if (block_count < 0) {
        memset(blocks, 0, sizeof blocks);
        block_count = 0;
      }
      for (int b = 0; b < block_count; b++) {
        unsigned count = blocks[b].count > 5u ? 5u : blocks[b].count;
        char role = blocks[b].role == 0u ? 'G'
                    : blocks[b].role == 1u ? 'D'
                    : blocks[b].role == 2u ? 'M'
                                           : 'F';
        for (unsigned s = 0; s < count; s++) {
          uint8_t pitch = blocks[b].slots[s];
          if (pitch >= FIFA96_MATCH_ENTITY_RECORDS || seen[pitch]) continue;
          seen[pitch] = 1;
          pitch_role[pitch] = (uint8_t)role;
          order[order_count++] = pitch;
        }
      }
      for (uint8_t pitch = 0; pitch < FIFA96_MATCH_ENTITY_RECORDS; pitch++)
        if (!seen[pitch]) order[order_count++] = pitch;

      for (int i = 0; i < order_count; i++) {
        uint8_t pitch = order[i];
        char role = (char)pitch_role[pitch];
        int best = -1;
        int best_rating = -1;
        int best_non_gk = -1;
        int best_non_gk_rating = -1;
        int best_any = -1;
        int best_any_rating = -1;
        for (uint8_t r = 0; r < FIFA96_PCCD_ROSTER_COUNT; r++) {
          if (used[r]) continue;
          int rating = mr->roster_rating[side][r];
          if (rating > best_non_gk_rating && mr->roster_pos[side][r] != 'G') {
            best_non_gk_rating = rating;
            best_non_gk = r;
          }
          if (rating > best_any_rating) {
            best_any_rating = rating;
            best_any = r;
          }
          if (role != '\0' && mr->roster_pos[side][r] == role &&
              rating > best_rating) {
            best_rating = rating;
            best = r;
          }
        }
        if (best < 0) best = best_non_gk >= 0 ? best_non_gk : best_any;
        if (best < 0) break;
        mr->sub_lineup[side][pitch] = (uint8_t)best;
        used[best] = 1;
      }
    }
    uint16_t bench = 0;
    for (uint8_t r = 0; r < FIFA96_PCCD_ROSTER_COUNT; r++)
      if (!used[r]) bench |= (uint16_t)(1u << r);
    mr->sub_bench_avail[side] = bench;
  }
  mr->sub_lineup_ready = 1;
  match_run_sub_menu_reset(mr);
  return 1;
}

/* A bench slot's GK class must match the outgoing player's class. */
static int match_run_sub_is_gk(const struct fifa96_match_run *mr, uint8_t side,
                               uint8_t roster) {
  return roster < FIFA96_PCCD_ROSTER_COUNT && mr->roster_pos[side][roster] == 'G';
}

static int match_run_sub_in_count(const struct fifa96_match_run *mr, uint8_t side,
                                  uint8_t out_pitch) {
  uint8_t out_roster = mr->sub_lineup[side][out_pitch];
  int count = 0;
  if (out_roster == FIFA96_MATCH_RUN_SUB_NONE) return 0;
  int out_gk = match_run_sub_is_gk(mr, side, out_roster);
  for (uint8_t r = 0; r < FIFA96_PCCD_ROSTER_COUNT; r++) {
    if ((mr->sub_bench_avail[side] & (uint16_t)(1u << r)) == 0) continue;
    if (match_run_sub_is_gk(mr, side, r) != out_gk) continue;
    count++;
  }
  return count;
}

static int match_run_sub_in_at(const struct fifa96_match_run *mr, uint8_t side,
                               uint8_t out_pitch, int index, uint8_t *out) {
  uint8_t out_roster = mr->sub_lineup[side][out_pitch];
  if (out_roster == FIFA96_MATCH_RUN_SUB_NONE) return 0;
  int out_gk = match_run_sub_is_gk(mr, side, out_roster);
  for (uint8_t r = 0; r < FIFA96_PCCD_ROSTER_COUNT; r++) {
    if ((mr->sub_bench_avail[side] & (uint16_t)(1u << r)) == 0) continue;
    if (match_run_sub_is_gk(mr, side, r) != out_gk) continue;
    if (index-- == 0) {
      *out = r;
      return 1;
    }
  }
  return 0;
}

/* FU-348: the per-pitch appearance table is a projection of sub_lineup, so a
 * substitution must rebuild it (native FUN_00047298 reads the current roster
 * descriptor every template pass; it is not a stage-time snapshot). Defined
 * with the live kit builder below. */
static void match_run_rebuild_entity_remaps(struct fifa96_match_run *mr);

int fifa96_match_run_pause_sub_apply(struct fifa96_match_run *mr,
                                     int out_pitch, int in_roster) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (!mr->live_start || !mr->paused) return -FIFA96_ERR_UNSUPPORTED;
  if (out_pitch < 0 || out_pitch >= (int)FIFA96_MATCH_ENTITY_RECORDS ||
      in_roster < 0 || in_roster >= (int)FIFA96_PCCD_ROSTER_COUNT)
    return -FIFA96_ERR_INVALID;
  if (!mr->sub_lineup_ready) (void)fifa96_match_run_sub_rebuild(mr);
  uint8_t side = mr->sub_menu_open ? mr->sub_menu_side
                                   : match_run_live_human_side(mr);
  if (mr->sub_used[side] >= FIFA96_MATCH_RUN_SUB_LIMIT) {
    mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_LIMIT;
    return 0;
  }
  uint16_t bit = (uint16_t)(1u << (unsigned)in_roster);
  if ((mr->sub_bench_avail[side] & bit) == 0) {
    mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_USED;
    return 0;
  }
  uint8_t out_roster = mr->sub_lineup[side][out_pitch];
  if (out_roster == FIFA96_MATCH_RUN_SUB_NONE) {
    mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_NO_MATCH;
    return 0;
  }
  if (match_run_sub_is_gk(mr, side, out_roster) !=
      match_run_sub_is_gk(mr, side, (uint8_t)in_roster)) {
    mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_GK_MISMATCH;
    return 0;
  }

  const char *out_name = mr->roster_name[side][out_roster][0]
                             ? mr->roster_name[side][out_roster]
                             : "PLAYER";
  const char *in_name = mr->roster_name[side][in_roster][0]
                            ? mr->roster_name[side][in_roster]
                            : "PLAYER";
  mr->sub_lineup[side][out_pitch] = (uint8_t)in_roster;
  mr->sub_bench_avail[side] &= (uint16_t)~bit;
  match_run_rebuild_entity_remaps(mr);
  (void)snprintf(mr->sub_notice, sizeof mr->sub_notice, "%.18s > %.18s",
                 out_name, in_name);
  mr->render.sub.active = 1u;
  mr->render.sub.cursor_raw = (uint8_t)(mr->sub_used[side] % 5u);
  mr->render.sub.current_side = side;
  mr->render.sub.record_side = side;
  mr->render.sub.mode = 0u;
  mr->render.sub.special = 0u;
  mr->render.sub.flags[mr->sub_used[side] * 2u + side] = 1u;
  mr->render.sub.record[1] = (uint8_t)out_pitch;
  mr->render.sub.record[5] = (uint8_t)in_roster;
  mr->render.sub.frame5_height = 41u;
  mr->render.sub.name_width = 0;
  (void)snprintf(mr->render.sub.name[side], sizeof mr->render.sub.name[side],
                 "%.15s", in_name);
  mr->sub_used[side]++;
  mr->sub_notice_ticks = 180u;
  mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_ACCEPTED;
  return 1;
}

int fifa96_match_run_pause_sub(struct fifa96_match_run *mr, int action) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (!mr->live_start) return -FIFA96_ERR_UNSUPPORTED;
  if (action == FIFA96_MATCH_RUN_SUB_OPEN) {
    if (!mr->paused) return -FIFA96_ERR_UNSUPPORTED;
    if (mr->sub_menu_open) return 0;
    if (!mr->sub_lineup_ready) (void)fifa96_match_run_sub_rebuild(mr);
    uint8_t side = match_run_live_human_side(mr);
    if (mr->sub_used[side] >= FIFA96_MATCH_RUN_SUB_LIMIT) {
      mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_LIMIT;
      (void)snprintf(mr->sub_notice, sizeof mr->sub_notice,
                     "NO SUBSTITUTIONS LEFT");
      mr->sub_notice_ticks = 180u;
      return 0;
    }
    if (mr->sub_bench_avail[side] == 0) {
      mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_NO_MATCH;
      (void)snprintf(mr->sub_notice, sizeof mr->sub_notice, "NO BENCH MATCH");
      mr->sub_notice_ticks = 180u;
      return 0;
    }
    mr->sub_menu_open = 1;
    mr->sub_menu_side = side;
    mr->sub_menu_stage = 0;
    mr->sub_menu_cursor = 0;
    mr->sub_menu_out = FIFA96_MATCH_RUN_SUB_NONE;
    mr->sub_menu_in = FIFA96_MATCH_RUN_SUB_NONE;
    mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_NONE;
    return 1;
  }
  if (action == FIFA96_MATCH_RUN_SUB_BACK) {
    if (!mr->sub_menu_open) return 0;
    if (mr->sub_menu_stage != 0) {
      mr->sub_menu_stage = 0;
      mr->sub_menu_cursor = 0;
      mr->sub_menu_out = FIFA96_MATCH_RUN_SUB_NONE;
      mr->sub_menu_in = FIFA96_MATCH_RUN_SUB_NONE;
      mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_NONE;
      return 1;
    }
    match_run_sub_menu_reset(mr);
    return 1;
  }
  if (!mr->sub_menu_open) return 0;
  if (action == FIFA96_MATCH_RUN_SUB_UP || action == FIFA96_MATCH_RUN_SUB_DOWN) {
    int count = mr->sub_menu_stage == 0
                    ? (int)FIFA96_MATCH_ENTITY_RECORDS
                    : match_run_sub_in_count(mr, mr->sub_menu_side,
                                             mr->sub_menu_out);
    if (count <= 0) return 0;
    int cursor = mr->sub_menu_cursor;
    cursor = action == FIFA96_MATCH_RUN_SUB_UP ? (cursor + count - 1) % count
                                               : (cursor + 1) % count;
    mr->sub_menu_cursor = (uint8_t)cursor;
    mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_NONE;
    return 1;
  }
  if (action == FIFA96_MATCH_RUN_SUB_CONFIRM) {
    if (mr->sub_menu_stage == 0) {
      mr->sub_menu_out = mr->sub_menu_cursor;
      mr->sub_menu_stage = 1;
      mr->sub_menu_cursor = 0;
      mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_NONE;
      if (match_run_sub_in_count(mr, mr->sub_menu_side, mr->sub_menu_out) == 0)
        mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_NO_MATCH;
      return 1;
    }
    uint8_t in_roster = FIFA96_MATCH_RUN_SUB_NONE;
    if (!match_run_sub_in_at(mr, mr->sub_menu_side, mr->sub_menu_out,
                             mr->sub_menu_cursor, &in_roster)) {
      mr->sub_menu_status = FIFA96_MATCH_RUN_SUB_STATUS_NO_MATCH;
      return 0;
    }
    mr->sub_menu_in = in_roster;
    if (fifa96_match_run_pause_sub_apply(mr, mr->sub_menu_out, in_roster) == 1) {
      match_run_sub_menu_reset(mr);
      return 1;
    }
    return 0;
  }
  return -FIFA96_ERR_INVALID;
}

int fifa96_match_run_result_continue(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (!mr->live_start) return -FIFA96_ERR_UNSUPPORTED;
  if (mr->lc.screen != FIFA96_MATCH_SCREEN_OVER &&
      mr->lc.screen != FIFA96_MATCH_SCREEN_POST)
    return 0;   /* nothing to skip: a harmless no-op on a live ACTIVE match */
  mr->post_skip = 1;
  return 0;
}

int fifa96_match_run_step(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  mr->steps++;
  if (fifa96_match_lifecycle_should_exit(&mr->lc)) return fifa96_match_run_end(mr);
  /* G1 live exit path: the frame body runs on the registered 100 Hz trampoline
   * during this engine step's clock advance, so a period end has just marked
   * the FU-64 lifecycle OVER when the step runs. The non-live path keeps the
   * one-step compression through the single resolve entry
   * (OVER -> POST -> EXIT -> run_end), byte-identical for the M2 tape.
   * FU-343 (P4): a live run holds the native POST screen (screen 3) for
   * FIFA96_MATCH_RUN_POST_DWELL_TICKS engine steps with the match pace held
   * (the world/clock freeze); the FULL TIME panel is drawn by the render path.
   * `result_continue` skips the dwell. The POST hold is the engine stand-in
   * for the unported native screen-3 timer (leg P4-L2). Re-entry guard:
   * run_end clears running and the engine linkage, so a later step returns
   * -FIFA96_ERR_STATE and the engine's MATCH dispatch leaves through its
   * !e->match arm instead of stepping an ended run. */
  if (mr->live_start) {
    if (mr->lc.screen == FIFA96_MATCH_SCREEN_OVER) {
      (void)fifa96_match_lifecycle_resolve_over(&mr->lc);   /* OVER -> POST */
      (void)fifa96_match_pace_hold(&mr->pace);              /* world freeze */
      mr->post_timer = FIFA96_MATCH_RUN_POST_DWELL_TICKS;
      /* Keep a skip staged in the same engine step (an ESC/ENTER edge on the
       * OVER frame) so it is not swallowed by this transition; end/begin clear
       * it for the next match. */
      return 0;
    }
    if (mr->lc.screen == FIFA96_MATCH_SCREEN_POST) {
      if (mr->post_timer != 0) mr->post_timer--;
      if (mr->post_timer == 0 || mr->post_skip) {
        mr->result_pending = 1;
        (void)fifa96_match_lifecycle_request_exit(&mr->lc); /* POST -> EXIT */
        return fifa96_match_run_end(mr);
      }
      return 0;
    }
  }
  if (mr->lc.screen == FIFA96_MATCH_SCREEN_OVER)
    return fifa96_match_run_resolve(mr);
  /* The frame body does NOT run here: the registered 100 Hz trampoline
   * (fifa96_match_run_tick) already consumed this step's PIT ticks from the
   * engine clock, so pace/state advance once per 10 ms regardless of how many
   * ticks one engine step spans. */
  return 0;
}

int fifa96_match_run_end(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  /* P13: preserve the regulation/extra-time result before lifecycle_end zeroes
   * the live score pair. A pause/abandon exit is not a result. */
  if (mr->lc.screen == FIFA96_MATCH_SCREEN_OVER ||
      mr->lc.screen == FIFA96_MATCH_SCREEN_POST || mr->result_pending) {
    mr->final_score[0] = mr->score[0];
    mr->final_score[1] = mr->score[1];
    mr->final_score_valid = 1;
  } else {
    mr->final_score_valid = 0;
  }
  mr->result_pending = 0;
  match_run_camera_trace_close(mr);   /* FU313 §4.1: idempotent with teardown */
  int rc = fifa96_match_lifecycle_end(&mr->lc, &mr->pace);
  struct fifa96_engine *eng = mr->engine;
  match_run_release_stage(mr);
  match_run_release_meta(mr);   /* FU-199 §7.2: drop the owned `.fmt` blobs */
  mr->running = 0;
  mr->paused = 0;               /* FU-343 P4: fresh pause/POST state on reuse */
  match_run_sub_menu_reset(mr); /* P15: no stale menu across runs */
  fifa96_match_stats_reset(&mr->stats); /* FU-72: a reused run starts clean */
  mr->stats_third_selection = 0;        /* FU-72 §2.3: [0x5B358] cleared too */
  mr->post_timer = 0;
  mr->post_skip = 0;
  /* FU-343 P3 fix: clear the live-only mode and its one-shot stage flags so a
   * reused struct cannot inherit live mode. begin/reset_render rebuilds the
   * render state anyway; this keeps the post-end state honest (and the
   * live_start render gates closed) between end and the next begin. */
  mr->live_start = 0;
  mr->match_mode = 0;
  mr->render.team_remap_ready = 0;
  mr->render.hud_colors_ready = 0;
  mr->render.camera.tracked = FIFA96_CAMERA_TRACKED_NONE;
  /* RR-L1/RR-L2: the match end closes the recorder gate (the gate-preserving
   * reset no longer writes it) and then resets the recorded-frame ring (the
   * cursors and count). */
  fifa96_match_replay_ring_record(mr, 0);
  fifa96_match_replay_ring_reset(mr);
  if (eng) {
    if (eng->match == mr) eng->match = NULL;
    if (eng->mode == FIFA96_ENGINE_MODE_MATCH) eng->mode = FIFA96_ENGINE_MODE_FRONTEND;
  }
  return rc;
}

/* Scaled indexed span copy: FU-85 §2/FU-88 §5 reduce the resolved sprite to the
 * pivot-placed destination rectangle (fifa96_render_place) and the clipped
 * cover rectangle (fifa96_render_cover_rect); each output row then samples the
 * sprite at the 16.16 source step and writes through the remap, 0xFF
 * transparent (the FU-85 §2 0xCEABC span writer's semantics) into the indexed
 * canvas.
 *
 * `scale` is the signed FU-85 §2 size factor (`size * scale >> 16`); a negative
 * scale mirrors both axes through the library's signed placement/source
 * stepping, and the resolver's out1 flag flips x only. A zero destination size
 * is off-canvas (skipped). */
static int match_run_draw_sprite(struct fifa96_surface *s, const fifa96_sprite_frame *sprite,
                                 const uint8_t *remap, int32_t x, int32_t y, int32_t scale,
                                 int mirrored, const fifa96_render_clip *clip) {
  int32_t dst_w = (int32_t)(((int64_t)sprite->width * scale) >> 16);
  int32_t dst_h = (int32_t)(((int64_t)sprite->height * scale) >> 16);
  if (dst_w == 0 || dst_h == 0) return 0;
  if (mirrored) dst_w = -dst_w;
  int32_t dst_x = 0;
  int32_t dst_y = 0;
  fifa96_err_t err = fifa96_render_place(sprite->width, sprite->height, sprite->pivot_x,
                                         sprite->pivot_y, dst_w, dst_h, x, y, &dst_x, &dst_y);
  if (err != FIFA96_OK) return (int)err;
  fifa96_render_cover cover;
  uint8_t visible = 0;
  err = fifa96_render_cover_rect(dst_x, dst_y, dst_w, dst_h, sprite->width, sprite->height,
                                 clip, &cover, &visible);
  if (err != FIFA96_OK || !visible) return (int)err;
  int32_t cols[FIFA96_SURFACE_MAX_W + 1];
  for (int32_t row = 0; row < cover.dst_h; row++) {
    int32_t src_row =
        (int32_t)((uint32_t)cover.src_y + (uint32_t)row * (uint32_t)cover.src_dy);
    err = fifa96_sprite_columns(cols, (uint32_t)cover.dst_w, cover.src_x, cover.src_dx,
                                src_row, 0, sprite->width);
    if (err != FIFA96_OK) return (int)err;
    uint8_t *dst = s->indexed + (size_t)(cover.dst_y + row) * (size_t)s->width +
                   (size_t)cover.dst_x;
    err = fifa96_sprite_span(dst, sprite->pixels, sprite->pixel_len, cols,
                             (uint32_t)cover.dst_w, remap);
    if (err != FIFA96_OK) return (int)err;
  }
  return 0;
}

/* OL-T11-7 (P0.2 HUD; FU-148 §1.4/§6.1). The native bar blit scales the
 * Frames.fsh frame-13 panel by `h*0xB800>>16` in the zoomed window
 * (FUN_0009BB20/FUN_0009B850) and 1:1 in the full one (FUN_0009AFD0); the
 * layout anchor `[0x14E634]+6` = the Frames.fsh frame-4 height (41) is
 * rounded the same way in FUN_00053240's zoomed branch. */
#define MATCH_HUD_BAR_ZOOM 0xB800

/* FU-343 (P4): derive the live HUD body index and the `0xFE` destination
 * shade from the staged palette (A4 §4.1/§4.2). The body scan is the report's
 * max-min near-white pick; the shade entry is the nearest palette colour to
 * half the destination's RGB (squared distance, first on ties). The native
 * shade table `0x114274` is built by the unported kit/palette translation
 * (FU-148 leg 10/11), so this is the documented stand-in; `shade[0]` is pinned
 * to the static table's zero entry (A4 §1.3). */
void fifa96_match_run_hud_colors_from_palette(const uint8_t palette[768],
                                              uint8_t *ink, uint8_t shade[256]) {
  if (ink) {
    int best = 0;
    int best_min = -1;
    if (palette) {
      for (int i = 0; i < 256; i++) {
        int r = palette[i * 3 + 0];
        int g = palette[i * 3 + 1];
        int b = palette[i * 3 + 2];
        int m = r < g ? r : g;
        if (b < m) m = b;
        if (m > best_min) {
          best_min = m;
          best = i;
        }
      }
    }
    *ink = palette ? (uint8_t)best : FIFA96_MATCH_HUD_INK_DEFAULT;
  }
  if (shade) {
    for (int i = 0; i < 256; i++) {
      if (!palette) {
        shade[i] = (uint8_t)(i >> 1);
        continue;
      }
      int tr = palette[i * 3 + 0] / 2;
      int tg = palette[i * 3 + 1] / 2;
      int tb = palette[i * 3 + 2] / 2;
      int best = 0;
      long best_d = -1;
      for (int j = 0; j < 256; j++) {
        int dr = palette[j * 3 + 0] - tr;
        int dg = palette[j * 3 + 1] - tg;
        int db = palette[j * 3 + 2] - tb;
        long d = (long)dr * dr + (long)dg * dg + (long)db * db;
        if (best_d < 0 || d < best_d) {
          best_d = d;
          best = j;
        }
      }
      shade[i] = (uint8_t)best;
    }
    shade[0] = 0;
  }
}

/* FU-148 OL-T11-71 errata: the live HUD's 16-entry text ramp. The native
 * FUN_0009FFF0 shaded arm indexes the runtime `0x15B998` table, which
 * FUN_00019E9C copies from the selected palette bank (`+0x1000`); those exact
 * entries are not statically derivable. Build the [D] nearest-palette ramp
 * from black to `ink` instead: entry 0 is the transparent/zero entry, entries
 * 1..14 interpolate the body's RGB, and entry 15 is pinned to `ink`. The HUD
 * fonts are 4bpp, so every glyph nibble has a ramp entry. */
void fifa96_match_run_hud_text_shades_from_palette(const uint8_t palette[768],
                                                   uint8_t ink,
                                                   uint8_t shades[16]) {
  if (!shades) return;
  shades[0] = 0;
  if (!palette) {
    for (int i = 1; i < 16; i++)
      shades[i] = (uint8_t)(((unsigned)i * ink + 7u) / 15u);
    return;
  }
  int ir = palette[(size_t)ink * 3u + 0u];
  int ig = palette[(size_t)ink * 3u + 1u];
  int ib = palette[(size_t)ink * 3u + 2u];
  for (int i = 1; i < 16; i++) {
    int tr = (ir * i) / 15;
    int tg = (ig * i) / 15;
    int tb = (ib * i) / 15;
    int best = 0;
    long best_d = -1;
    for (int j = 0; j < 256; j++) {
      int dr = palette[(size_t)j * 3u + 0u] - tr;
      int dg = palette[(size_t)j * 3u + 1u] - tg;
      int db = palette[(size_t)j * 3u + 2u] - tb;
      long d = (long)dr * dr + (long)dg * dg + (long)db * db;
      if (best_d < 0 || d < best_d) {
        best_d = d;
        best = j;
      }
    }
    shades[i] = (uint8_t)best;
  }
  /* The native body entry is the selected colour; a duplicate palette entry
   * must not steal the 15-nibble body pixel on a tie. */
  shades[15] = ink;
}

/* P10 graphics: palette maps for the mowing stripes (90%) and the player
 * ground shadows (55%). Built once per staged palette alongside the HUD
 * shade. */
static void match_run_palette_shade_from_palette(const uint8_t palette[768],
                                                 uint8_t shade[256],
                                                 int percent) {
  if (shade == NULL) return;
  for (int i = 0; i < 256; i++) {
    if (palette == NULL) { shade[i] = (uint8_t)i; continue; }
    int tr = palette[i * 3 + 0] * percent / 100;
    int tg = palette[i * 3 + 1] * percent / 100;
    int tb = palette[i * 3 + 2] * percent / 100;
    int best = 0;
    long best_d = -1;
    for (int j = 0; j < 256; j++) {
      int dr = palette[j * 3 + 0] - tr;
      int dg = palette[j * 3 + 1] - tg;
      int db = palette[j * 3 + 2] - tb;
      long d = (long)dr * dr + (long)dg * dg + (long)db * db;
      if (best_d < 0 || d < best_d) { best_d = d; best = j; }
    }
    shade[i] = (uint8_t)best;
  }
  shade[0] = 0;
}

static void match_run_pitch_shade_from_palette(const uint8_t palette[768],
                                               uint8_t shade[256]) {
  match_run_palette_shade_from_palette(palette, shade, 90);
}

static void match_run_shadow_shade_from_palette(const uint8_t palette[768],
                                                uint8_t shade[256]) {
  match_run_palette_shade_from_palette(palette, shade, 55);
}

static void match_run_hud_colors_refresh(struct fifa96_match_run *mr) {
  struct fifa96_match_run_render *r = &mr->render;
  /* FU-343 P4 note: on the final live palette the kit stage forces the pitch
   * marking slot 0xFE to (255,255,255), so the max-min scan below selects
   * 0xFE as the live text body. The compiled fallback 30 is only the pre-stage
   * value (the measured staging palette's near-white). */
  if (r->palette_ready) {
    fifa96_match_run_hud_colors_from_palette(r->palette, &r->hud_ink, r->hud_shade);
    fifa96_match_run_hud_text_shades_from_palette(r->palette, r->hud_ink,
                                                  r->hud_text_shades);
    match_run_pitch_shade_from_palette(r->palette, r->pitch_shade);
    match_run_shadow_shade_from_palette(r->palette, r->shadow_shade);
    r->hud_colors_ready = 1;
  } else {
    fifa96_match_run_hud_colors_from_palette(NULL, &r->hud_ink, r->hud_shade);
    fifa96_match_run_hud_text_shades_from_palette(NULL, r->hud_ink,
                                                  r->hud_text_shades);
    match_run_pitch_shade_from_palette(NULL, r->pitch_shade);
    match_run_shadow_shade_from_palette(NULL, r->shadow_shade);
    r->hud_colors_ready = 0;
  }
}

static const struct fifa96_font *match_run_hud_font(
    const struct fifa96_match_run_render *r) {
  return r->window_zoomed ? &r->hud_font[1] : &r->hud_font[0];
}

static int match_run_hud_font_ready(const struct fifa96_match_run_render *r) {
  const struct fifa96_font *font = match_run_hud_font(r);
  return (r->window_zoomed ? r->hud_font_ready[1] : r->hud_font_ready[0]) != 0 &&
         font->data != NULL;
}

/* The native FUN_000565BC gates (period < 4 via FUN_0004B5F4, not paused via
 * [0x14E688]) plus the asset-readiness mapping of the FUN_0001D940(6) /
 * [0x14E510] / [0x14E538] / [0x14E59C] gates (FU-148 §6.1 leg): the HUD needs
 * its staged bar and the font the window branch selects. The engine has no
 * replay sub-mode, so the [0x109A98]==0 idle gate is vacuous. */
static int match_run_hud_ready(const struct fifa96_match_run_render *r) {
  return r->hud_bar_ready && r->hud_bar.pixels != NULL &&
         match_run_hud_font_ready(r);
}

/* The bar panel blit. FU-343 (P4): the live path applies the native pixel
 * rule (A4 §1.3, FUN_000D11CB 0xD12DB.. / FUN_000CEB9A 0xCEBBC..):
 *   0x00..0xFD opaque; 0xFE = shade the DESTINATION; 0xFF = transparent.
 * The non-live path keeps the legacy FU-148 convention (0 transparent, every
 * other byte literal) so the M2 tape is byte-identical. The destination rect
 * is the native's `size * factor >> 16` and the sampling is the derived
 * nearest-neighbour step `dst * src_w / dst_w` (FU-148 leg: the native
 * FUN_0009B850 span stepper is not decomposed). */
static void match_run_hud_bar(struct fifa96_surface *s,
                              const struct fifa96_match_run *mr, int bar_x,
                              int bar_y, int scale_x, int scale_y) {
  const struct fifa96_match_run_render *r = &mr->render;
  const fifa96_sprite_frame *bar = &r->hud_bar;
  const int live = mr->live_start != 0;
  int dst_w = bar->width * scale_x;
  int dst_h = bar->height * scale_y;
  if (r->window_zoomed) {
    dst_w = (dst_w * MATCH_HUD_BAR_ZOOM) >> 16;
    dst_h = (dst_h * MATCH_HUD_BAR_ZOOM) >> 16;
  }
  for (int dy = 0; dy < dst_h; dy++) {
    int src_y = (int)(((int64_t)dy * bar->height) / dst_h);
    int py = bar_y + dy;
    if (py < 0 || py >= s->height) continue;
    for (int dx = 0; dx < dst_w; dx++) {
      int src_x = (int)(((int64_t)dx * bar->width) / dst_w);
      size_t src_off = (size_t)src_y * bar->width + (size_t)src_x;
      if (src_off >= bar->pixel_len) continue;
      uint8_t v = bar->pixels[src_off];
      int px = bar_x + dx;
      if (px < 0 || px >= s->width) continue;
      size_t dst_off = (size_t)py * (size_t)s->width + (size_t)px;
      if (!live) {
        if (v == 0) continue;            /* legacy FU-148 convention */
        s->indexed[dst_off] = v;
        continue;
      }
      if (v == 0xFFu) continue;          /* transparent */
      if (v == 0xFEu) {                  /* destination shade */
        uint8_t dst = s->indexed[dst_off];
        s->indexed[dst_off] = r->hud_colors_ready ? r->hud_shade[dst]
                                                  : (uint8_t)(dst >> 1);
      } else {
        s->indexed[dst_off] = v;         /* 0x00..0xFD opaque */
      }
    }
  }
}

/* Two-pass text: the shadow at (+1,+1) then the body at (x,y) (FUN_000A06FC,
 * FU-148 §1.5). FU-343 (P4): the live HUD passes outline 0x00 / body
 * `hud_ink`; the non-live passes keep the legacy colour-6 outline / colour-0
 * body. The other presenters (sub/replay/overlay) keep the legacy operands.
 * OL-T11-71 errata: a non-NULL `shades` selects FUN_0009FFF0's shaded arm for
 * the body pass (the native `0x15B998` ramp); the shadow pass stays flat. */
static void match_run_hud_text(struct fifa96_surface *s, const fifa96_font *font,
                               const char *str, int x, int y, uint8_t body,
                               uint8_t shadow, const uint8_t *shades) {
  (void)fifa96_font_blit(s->indexed, s->width, s->height, font, str, x + 1, y + 1,
                         shadow);
  if (shades) {
    (void)fifa96_font_blit_shaded(s->indexed, s->width, s->height, font, str, x, y,
                                  shades, 16);
  } else {
    (void)fifa96_font_blit(s->indexed, s->width, s->height, font, str, x, y, body);
  }
}

/* FUN_00055BA8: centred cell text, `x += (cell_w-width)/2` (truncating),
 * `y += max(0,(cell_h-12)/2)`, then the outline/main pair. */
static void match_run_hud_centered(struct fifa96_surface *s, const fifa96_font *font,
                                   const char *str, int cell_x, int cell_y, int cell_w,
                                   int cell_h, uint8_t body, uint8_t shadow,
                                   const uint8_t *shades) {
  int w = fifa96_font_text_width(font, str);
  int x = cell_x + (cell_w - w) / 2;
  int y = cell_y;
  int pad = (cell_h - 12) / 2;
  if (pad > 0) y += pad;
  match_run_hud_text(s, font, str, x, y, body, shadow, shades);
}

/* P13: the 640 live HUD is a uniform 2x native panel, not an x-stretched 320
 * strip. Render the glyphs once into a small transparent scratch and expand
 * them; the 320 path keeps the exact existing blit. */
#define MATCH_RUN_HUD_SCRATCH_W 512
#define MATCH_RUN_HUD_SCRATCH_H 40

static void match_run_hud_text_scaled(struct fifa96_surface *s,
                                      const fifa96_font *font, const char *str,
                                      int x, int y, uint8_t body, uint8_t shadow,
                                      const uint8_t *shades, int scale_x, int scale_y) {
  if (scale_x <= 1 && scale_y <= 1) {
    match_run_hud_text(s, font, str, x, y, body, shadow, shades);
    return;
  }
  uint8_t scratch[MATCH_RUN_HUD_SCRATCH_W * MATCH_RUN_HUD_SCRATCH_H];
  memset(scratch, 0, sizeof scratch);
  if (fifa96_font_blit_mask(scratch, MATCH_RUN_HUD_SCRATCH_W,
                            MATCH_RUN_HUD_SCRATCH_H, font, str, 4, 4) != FIFA96_OK)
    return;
  for (int pass = 0; pass < 2; pass++) {
    int ox = pass == 0 ? scale_x : 0;
    int oy = pass == 0 ? scale_y : 0;
    for (int sy = 0; sy < MATCH_RUN_HUD_SCRATCH_H; sy++) {
      for (int sx = 0; sx < MATCH_RUN_HUD_SCRATCH_W; sx++) {
        unsigned value = scratch[sy * MATCH_RUN_HUD_SCRATCH_W + sx];
        if (value == 0) continue;
        uint8_t colour;
        if (pass == 0) {
          colour = shadow;
        } else if (shades && value < 16u) {
          colour = shades[value];
        } else {
          colour = body;
        }
        for (int dy = 0; dy < scale_y; dy++) {
          int py = y + (sy - 4) * scale_y + oy + dy;
          if (py < 0 || py >= s->height) continue;
          uint8_t *row = s->indexed + (size_t)py * (size_t)s->width;
          for (int dx = 0; dx < scale_x; dx++) {
            int px = x + (sx - 4) * scale_x + ox + dx;
            if (px >= 0 && px < s->width) row[px] = colour;
          }
        }
      }
    }
  }
}

static void match_run_hud_centered_scaled(struct fifa96_surface *s,
                                          const fifa96_font *font, const char *str,
                                          int cell_x, int cell_y, int cell_w,
                                          int cell_h, uint8_t body, uint8_t shadow,
                                          const uint8_t *shades, int scale_x,
                                          int scale_y) {
  if (scale_x <= 1 && scale_y <= 1) {
    match_run_hud_centered(s, font, str, cell_x, cell_y, cell_w, cell_h, body,
                           shadow, shades);
    return;
  }
  int w = fifa96_font_text_width(font, str) * scale_x;
  int x = cell_x + (cell_w - w) / 2;
  int y = cell_y;
  int pad = (cell_h - 12 * scale_y) / 2;
  if (pad > 0) y += pad;
  match_run_hud_text_scaled(s, font, str, x, y, body, shadow, shades, scale_x,
                            scale_y);
}

/* The HUD pass: bar -> name0 -> name1 -> score0 -> score1 -> period -> clock
 * (FU-148 §1.4). `mr->score[2]`/`state.total_seconds`/`state.period` are the
 * engine seam for the native `0x157AC5/7`, `0x157AB4` and `0x157AC2`. */
static void match_run_draw_hud(const struct fifa96_match_run *mr, struct fifa96_surface *s) {
  const struct fifa96_match_run_render *r = &mr->render;
  if (!match_run_hud_ready(r)) return;
  if (r->display.suspend) return;                 /* [0x14E688] */
  if (mr->state.period >= 4) return;              /* FUN_0004B5F4() < 4 */
  /* FU-343 P5: the live POST/OVER screen replaces the HUD with the result
   * panel. The native POST screen is a separate screen install (A5 §1); the
   * port's HUD period cell is `period+1`, which would read 3 on the post
   * sentinel. Non-live runs keep the historical HUD path (M2 v29). */
  if (mr->live_start &&
      (mr->lc.screen == FIFA96_MATCH_SCREEN_OVER ||
       mr->lc.screen == FIFA96_MATCH_SCREEN_POST))
    return;

  const int zoomed = r->window_zoomed;
  const struct fifa96_font *font = match_run_hud_font(r);
  const uint8_t body = mr->live_start ? r->hud_ink : 0x00;
  const uint8_t shadow = mr->live_start ? 0x00 : 0x06;
  const uint8_t *shades = mr->live_start ? r->hud_text_shades : NULL;
  const int have_window = r->window.box.w > 0 && r->window.box.h > 0;
  int x0 = have_window ? r->window.box.x0 : 0;
  int y1 = have_window ? r->window.box.y1 : s->height;
  if (y1 > s->height) y1 = s->height;

  const int ui_x = (s->width >= 640 && mr->live_start) ? 2 : 1;
  const int ui_y = (s->width >= 640 && mr->live_start) ? 2 : 1;
  const int row_pitch = (zoomed ? 9 : 12) * ui_y;
  const int x_left = x0 + (zoomed ? 3 : 6) * ui_x;
  int clock_x = x0 + (zoomed ? 0x23 : 0x32) * ui_x;
  const int bar_x = x0 + (zoomed ? 1 : 2) * ui_x;
  int bar_h = r->hud_bar_height;
  if (zoomed) bar_h = (bar_h * MATCH_HUD_BAR_ZOOM + 0x8000) >> 16;
  bar_h *= ui_y;
  const int bar_y = y1 - bar_h - (zoomed ? 1 : 2) * ui_y;
  const int y2 = bar_y + 2 * ui_y;
  const int cells_y = bar_y + 2 * row_pitch + (zoomed ? 2 : 3) * ui_y;

  char clock_buf[16];
  char score_buf[2][8];
  char period_buf[8];
  (void)snprintf(clock_buf, sizeof clock_buf, "%02d:%02d",
                 (int)(mr->state.total_seconds / 60u),
                 (int)(mr->state.total_seconds % 60u));
  (void)snprintf(score_buf[0], sizeof score_buf[0], "%d", (int)mr->score[0]);
  (void)snprintf(score_buf[1], sizeof score_buf[1], "%d", (int)mr->score[1]);
  (void)snprintf(period_buf, sizeof period_buf, "%d", (int)mr->state.period + 1);

  /* Wide-name/score adjustment (0x55D5A..0x55E76): either score >= 100 or a
   * staged name wider than 0x20 shifts the score column and widens the clock
   * cell by 6 (zoomed) / 8 (full). */
  int wide = mr->score[0] >= 100 || mr->score[1] >= 100 ||
             fifa96_font_text_width(font, r->hud_name[0]) > 0x20 ||
             fifa96_font_text_width(font, r->hud_name[1]) > 0x20;
  int clock_cell_w = (zoomed ? 0x12 : 0x18) * ui_x;
  if (wide) {
    clock_x += (zoomed ? 6 : 8) * ui_x;
    clock_cell_w += (zoomed ? 6 : 8) * ui_x;
  }

  match_run_hud_bar(s, mr, bar_x, bar_y, ui_x, ui_y);
  match_run_hud_text_scaled(s, font, r->hud_name[0], x_left, y2, body, shadow,
                            shades, ui_x, ui_y);
  match_run_hud_text_scaled(s, font, r->hud_name[1], x_left,
                            y2 + row_pitch - 2 * ui_y, body, shadow, shades,
                            ui_x, ui_y);
  match_run_hud_text_scaled(
      s, font, score_buf[0],
      clock_x - fifa96_font_text_width(font, score_buf[0]) * ui_x, y2, body,
      shadow, shades, ui_x, ui_y);
  match_run_hud_text_scaled(
      s, font, score_buf[1],
      clock_x - fifa96_font_text_width(font, score_buf[1]) * ui_x,
      y2 + row_pitch - 2 * ui_y, body, shadow, shades, ui_x, ui_y);
  match_run_hud_centered_scaled(s, font, period_buf, x0, cells_y, row_pitch - 1,
                                row_pitch - 1, body, shadow, shades, ui_x, ui_y);
  match_run_hud_centered_scaled(s, font, clock_buf, x0 + (zoomed ? 0xE : 0x11),
                                cells_y, clock_cell_w, row_pitch - 1, body, shadow,
                                shades, ui_x, ui_y);
}

/* FU-343 (P4): the pause and FULL TIME panels. Both are engine stand-ins for
 * the unported native presentation (A5 §3.1 mode 7/screen 0x3DE for pause;
 * A5-L1/A5-L2 the POST timer and the period-overlay armer for the result).
 * They draw only on the live path over the frozen pitch, using the staged HUD
 * font and the live colour pair (body = palette near-white, shadow = 0). */
#define MATCH_RUN_PAUSE_PANEL_W 150
#define MATCH_RUN_PAUSE_PANEL_H 52
#define MATCH_RUN_RESULT_PANEL_W 200
#define MATCH_RUN_RESULT_PANEL_H 68

static void match_run_panel_fill(struct fifa96_surface *s, int x, int y, int w, int h,
                                 uint8_t border) {
  int x0 = x < 0 ? 0 : x;
  int y0 = y < 0 ? 0 : y;
  int x1 = x + w;
  int y1 = y + h;
  if (x1 > s->width) x1 = s->width;
  if (y1 > s->height) y1 = s->height;
  if (x1 <= x0 || y1 <= y0) return;
  for (int py = y0; py < y1; py++) {
    uint8_t *row = s->indexed + (size_t)py * (size_t)s->width;
    for (int px = x0; px < x1; px++)
      row[px] = (py == y0 || py == y1 - 1 || px == x0 || px == x1 - 1) ? border : 0;
  }
}

static void match_run_panel_text(struct fifa96_surface *s, const fifa96_font *font,
                                 const char *str, int panel_x, int panel_w, int y,
                                 uint8_t body, uint8_t shadow,
                                 const uint8_t *shades) {
  int w = fifa96_font_text_width(font, str);
  match_run_hud_text(s, font, str, panel_x + (panel_w - w) / 2, y, body, shadow,
                     shades);
}

static void match_run_draw_pause(const struct fifa96_match_run *mr,
                                 struct fifa96_surface *s) {
  const struct fifa96_match_run_render *r = &mr->render;
  if (!match_run_hud_font_ready(r)) return;
  const fifa96_font *font = match_run_hud_font(r);
  const uint8_t ink = r->hud_colors_ready ? r->hud_ink : FIFA96_MATCH_HUD_INK_DEFAULT;
  const int x = (s->width - MATCH_RUN_PAUSE_PANEL_W) / 2;
  const int y = (s->height - MATCH_RUN_PAUSE_PANEL_H) / 2;
  match_run_panel_fill(s, x, y, MATCH_RUN_PAUSE_PANEL_W, MATCH_RUN_PAUSE_PANEL_H, ink);
  match_run_panel_text(s, font,
                       mr->sub_notice_ticks != 0u ? mr->sub_notice : "PAUSED",
                       x, MATCH_RUN_PAUSE_PANEL_W, y + 7, ink, 0x00,
                       r->hud_text_shades);
  match_run_panel_text(s, font, "ENTER: RESUME", x, MATCH_RUN_PAUSE_PANEL_W, y + 22,
                       ink, 0x00, r->hud_text_shades);
  match_run_panel_text(s, font, "S: SUBSTITUTE  X: QUIT", x, MATCH_RUN_PAUSE_PANEL_W,
                       y + 37, ink, 0x00, r->hud_text_shades);
}

/* P15 derived pause-substitution menu [D]. Native owner: FUN_00041494 case 5;
 * the two-step OUT/IN list, GK-only-GK and the three-sub cap are derived
 * player-facing rules over the PCCD roster (the native FSM/16-slot order are
 * P15-L1/L2). It is drawn only while the run is live and paused. */
#define MATCH_RUN_SUB_PANEL_W 260
#define MATCH_RUN_SUB_PANEL_H 184

static const char *match_run_sub_display_name(const struct fifa96_match_run *mr,
                                              uint8_t side, uint8_t roster,
                                              char *buf, size_t cap) {
  if (roster < FIFA96_PCCD_ROSTER_COUNT && mr->roster_name[side][roster][0])
    return mr->roster_name[side][roster];
  snprintf(buf, cap, "PLAYER %u", (unsigned)(roster + 1u));
  return buf;
}

static char match_run_sub_position(const struct fifa96_match_run *mr, uint8_t side,
                                   uint8_t roster) {
  if (roster >= FIFA96_PCCD_ROSTER_COUNT) return ' ';
  char pos = mr->roster_pos[side][roster];
  return pos ? pos : ' ';
}

static const char *match_run_sub_status_text(uint8_t status) {
  switch (status) {
    case FIFA96_MATCH_RUN_SUB_STATUS_GK_MISMATCH: return "GK FOR GK ONLY";
    case FIFA96_MATCH_RUN_SUB_STATUS_USED: return "PLAYER ALREADY USED";
    case FIFA96_MATCH_RUN_SUB_STATUS_LIMIT: return "NO SUBSTITUTIONS LEFT";
    case FIFA96_MATCH_RUN_SUB_STATUS_NO_MATCH: return "NO BENCH MATCH";
    default: return NULL;
  }
}

static void match_run_draw_sub_menu(const struct fifa96_match_run *mr,
                                    struct fifa96_surface *s) {
  const struct fifa96_match_run_render *r = &mr->render;
  if (!mr->sub_menu_open || !match_run_hud_font_ready(r)) return;
  const fifa96_font *font = match_run_hud_font(r);
  const uint8_t ink = r->hud_colors_ready ? r->hud_ink : FIFA96_MATCH_HUD_INK_DEFAULT;
  const int x = (s->width - MATCH_RUN_SUB_PANEL_W) / 2;
  const int y = (s->height - MATCH_RUN_SUB_PANEL_H) / 2;
  const uint8_t side = mr->sub_menu_side & 1u;
  char title[64];
  char row[40];
  char name[24];

  match_run_panel_fill(s, x, y, MATCH_RUN_SUB_PANEL_W, MATCH_RUN_SUB_PANEL_H, ink);
  (void)snprintf(title, sizeof title, "SUBSTITUTE %s (%u/%u)",
                 r->hud_name[side][0] ? r->hud_name[side] : "TEAM",
                 (unsigned)mr->sub_used[side], (unsigned)FIFA96_MATCH_RUN_SUB_LIMIT);
  match_run_panel_text(s, font, title, x, MATCH_RUN_SUB_PANEL_W, y + 6, ink, 0x00,
                       r->hud_text_shades);

  if (mr->sub_menu_stage == 0) {
    for (int pitch = 0; pitch < (int)FIFA96_MATCH_ENTITY_RECORDS; pitch++) {
      uint8_t roster = mr->sub_lineup[side][pitch];
      int row_y = y + 22 + pitch * 12;
      const char *display = match_run_sub_display_name(mr, side, roster, name,
                                                       sizeof name);
      (void)snprintf(row, sizeof row, "%c %2u %.20s %c",
                     mr->sub_menu_cursor == pitch ? '>' : ' ',
                     (unsigned)(roster < FIFA96_PCCD_ROSTER_COUNT
                                    ? mr->roster_number[side][roster]
                                    : roster + 1u),
                     display, match_run_sub_position(mr, side, roster));
      match_run_hud_text(s, font, row, x + 12, row_y, ink, 0x00,
                         r->hud_text_shades);
      if (mr->sub_menu_cursor == pitch)
        match_run_panel_fill(s, x + 12, row_y + 10, MATCH_RUN_SUB_PANEL_W - 24, 1,
                             ink);
    }
  } else {
    uint8_t out_roster = mr->sub_lineup[side][mr->sub_menu_out];
    const char *out_display = match_run_sub_display_name(mr, side, out_roster, name,
                                                         sizeof name);
    (void)snprintf(row, sizeof row, "OUT: %2u %.20s %c",
                   (unsigned)(out_roster < FIFA96_PCCD_ROSTER_COUNT
                                  ? mr->roster_number[side][out_roster]
                                  : out_roster + 1u),
                   out_display, match_run_sub_position(mr, side, out_roster));
    match_run_panel_text(s, font, row, x, MATCH_RUN_SUB_PANEL_W, y + 20, ink, 0x00,
                         r->hud_text_shades);
    int count = match_run_sub_in_count(mr, side, mr->sub_menu_out);
    for (int i = 0; i < count; i++) {
      uint8_t roster = FIFA96_MATCH_RUN_SUB_NONE;
      (void)match_run_sub_in_at(mr, side, mr->sub_menu_out, i, &roster);
      int row_y = y + 34 + i * 12;
      const char *display = match_run_sub_display_name(mr, side, roster, name,
                                                       sizeof name);
      (void)snprintf(row, sizeof row, "%c %2u %.20s %c",
                     mr->sub_menu_cursor == i ? '>' : ' ',
                     (unsigned)(roster < FIFA96_PCCD_ROSTER_COUNT
                                    ? mr->roster_number[side][roster]
                                    : roster + 1u),
                     display, match_run_sub_position(mr, side, roster));
      match_run_hud_text(s, font, row, x + 12, row_y, ink, 0x00,
                         r->hud_text_shades);
      if (mr->sub_menu_cursor == i)
        match_run_panel_fill(s, x + 12, row_y + 10, MATCH_RUN_SUB_PANEL_W - 24, 1,
                             ink);
    }
  }

  const char *status = match_run_sub_status_text(mr->sub_menu_status);
  if (status)
    match_run_panel_text(s, font, status, x, MATCH_RUN_SUB_PANEL_W, y + 158, ink,
                         0x00, r->hud_text_shades);
  match_run_panel_text(s, font,
                       mr->sub_menu_stage == 0 ? "ENTER: SELECT  X: BACK"
                                               : "ENTER: SUB  X: BACK",
                       x, MATCH_RUN_SUB_PANEL_W, y + 170, ink, 0x00,
                       r->hud_text_shades);
}

/* Phase 78 [D]: the live-only free-kick/penalty prompt. Screen-space text
 * keeps the renderer independent of the unported native aim/camera UI. */
#define MATCH_RUN_SP_PANEL_W 250
#define MATCH_RUN_SP_PANEL_H 58

static void match_run_draw_setpiece(const struct fifa96_match_run *mr,
                                    struct fifa96_surface *s) {
  const struct fifa96_match_run_render *r = &mr->render;
  char row[48];
  int x;
  int y;
  if (!mr->live_start || !mr->live_sp_active || !match_run_hud_font_ready(r))
    return;
  const fifa96_font *font = match_run_hud_font(r);
  const uint8_t ink = r->hud_colors_ready ? r->hud_ink : FIFA96_MATCH_HUD_INK_DEFAULT;
  x = (s->width - MATCH_RUN_SP_PANEL_W) / 2;
  y = s->height - MATCH_RUN_SP_PANEL_H - 12;
  if (y < 0) y = 0;
  match_run_panel_fill(s, x, y, MATCH_RUN_SP_PANEL_W, MATCH_RUN_SP_PANEL_H, ink);
  match_run_panel_text(s, font, mr->live_sp_kind == 6u ? "PENALTY" : "FREE KICK",
                       x, MATCH_RUN_SP_PANEL_W, y + 5, ink, 0x00,
                       r->hud_text_shades);
  (void)snprintf(row, sizeof row, "AIM %d  POWER %u",
                 (int)(mr->live_sp_aim_x / 0x20), (unsigned)mr->live_sp_power);
  match_run_panel_text(s, font, row, x, MATCH_RUN_SP_PANEL_W, y + 20, ink, 0x00,
                       r->hud_text_shades);
  match_run_panel_text(s, font, "LEFT/RIGHT AIM  UP/DOWN POWER", x,
                       MATCH_RUN_SP_PANEL_W, y + 34, ink, 0x00,
                       r->hud_text_shades);
  match_run_panel_text(s, font, "Z / ENTER: KICK", x, MATCH_RUN_SP_PANEL_W,
                       y + 46, ink, 0x00, r->hud_text_shades);
}

static void match_run_draw_halftime(const struct fifa96_match_run *mr,
                                    struct fifa96_surface *s) {
  const struct fifa96_match_run_render *r = &mr->render;
  if (!match_run_hud_font_ready(r)) return;
  const fifa96_font *font = match_run_hud_font(r);
  const uint8_t ink = r->hud_colors_ready ? r->hud_ink : FIFA96_MATCH_HUD_INK_DEFAULT;
  const int x = (s->width - 220) / 2;
  const int y = (s->height - 58) / 2;
  match_run_panel_fill(s, x, y, 220, 58, ink);
  match_run_panel_text(s, font, "HALF TIME", x, 220, y + 7, ink, 0x00,
                       r->hud_text_shades);
  match_run_panel_text(s, font, "KICK OR PASS TO START", x, 220, y + 24, ink, 0x00,
                       r->hud_text_shades);
  match_run_panel_text(s, font, "AUTO IN 3 SECONDS", x, 220, y + 41, ink, 0x00,
                       r->hud_text_shades);
}

static void match_run_draw_result(const struct fifa96_match_run *mr,
                                  struct fifa96_surface *s) {
  const struct fifa96_match_run_render *r = &mr->render;
  if (!match_run_hud_font_ready(r)) return;
  const fifa96_font *font = match_run_hud_font(r);
  const uint8_t ink = r->hud_colors_ready ? r->hud_ink : FIFA96_MATCH_HUD_INK_DEFAULT;
  const int x = (s->width - MATCH_RUN_RESULT_PANEL_W) / 2;
  const int y = (s->height - MATCH_RUN_RESULT_PANEL_H) / 2;
  char score[64];
  char period[24];
  if (r->hud_name[0][0] && r->hud_name[1][0])
    (void)snprintf(score, sizeof score, "%s %u - %u %s", r->hud_name[0],
                   (unsigned)mr->score[0], (unsigned)mr->score[1], r->hud_name[1]);
  else
    (void)snprintf(score, sizeof score, "%u - %u", (unsigned)mr->score[0],
                   (unsigned)mr->score[1]);
  /* The plan asks for the period on the FULL TIME panel. Use the post index
   * itself (2 after regulation), not the HUD's `period + 1` cell, which read
   * 3 on the post sentinel (the P5 fix removed the wrong line; this restores
   * the correct one). */
  (void)snprintf(period, sizeof period, "PERIOD %u",
                 (unsigned)(mr->state.period < 1u ? 1u : mr->state.period));
  match_run_panel_fill(s, x, y, MATCH_RUN_RESULT_PANEL_W, MATCH_RUN_RESULT_PANEL_H, ink);
  match_run_panel_text(s, font, "FULL TIME", x, MATCH_RUN_RESULT_PANEL_W, y + 6, ink,
                       0x00, r->hud_text_shades);
  match_run_panel_text(s, font, score, x, MATCH_RUN_RESULT_PANEL_W, y + 21, ink, 0x00,
                       r->hud_text_shades);
  match_run_panel_text(s, font, period, x, MATCH_RUN_RESULT_PANEL_W, y + 36, ink, 0x00,
                       r->hud_text_shades);
  match_run_panel_text(s, font, "ENTER: MENU", x, MATCH_RUN_RESULT_PANEL_W, y + 51,
                       ink, 0x00, r->hud_text_shades);
}

/* ------------------------------------------------------------------------- *
 * FU-152 §4.1 (P4): the presentation residual rows R1/R2/R3 and the dormant
 * R5 seam. Pure state helpers first, then the draw steps; R4 (HUD) is FU-148.
 * ------------------------------------------------------------------------- */

int fifa96_match_run_replay_family(const struct fifa96_match_run_replay *replay) {
  if (!replay) return 0;
  return (replay->state & 0x80u) != 0;   /* FUN_00063FD0 */
}

int fifa96_match_run_replay_live(const struct fifa96_match_run_replay *replay) {
  return replay && replay->state == FIFA96_MATCH_REPLAY_LIVE;   /* FUN_0006400C */
}

int fifa96_match_run_replay_armed(const struct fifa96_match_run_replay *replay) {
  return replay && replay->state == FIFA96_MATCH_REPLAY_ARMED;  /* FUN_00063FDC */
}

int fifa96_match_run_replay_playing(const struct fifa96_match_run_replay *replay) {
  return replay && replay->state == FIFA96_MATCH_REPLAY_IDLE;   /* FUN_0006401C */
}

int fifa96_match_run_replay_ramp_window(const struct fifa96_match_run_replay *replay) {
  /* FUN_00053D58: ([0x14E58C] - 0xF1 < 0x78) && [0x14E58C] != 0, on the
   * signed dword (phases 1..0xF0 and 0x169+ fall outside the loading window). */
  if (!replay || replay->phase_counter == 0) return 0;
  return replay->phase_counter - 0xF1 < 0x78;
}

uint32_t fifa96_match_run_replay_progress(const struct fifa96_match_run_replay *replay) {
  if (!replay || replay->cursor_limit == 0) return 0;   /* FUN_000642B0 */
  return (uint32_t)(((uint64_t)replay->cursor * 100u) / replay->cursor_limit);
}

int fifa96_match_run_replay_blink_step(struct fifa96_match_run_replay *replay,
                                       int32_t tick) {
  if (!replay) return 0;
  replay->blink += tick;                 /* FUN_000564A0 */
  if (replay->blink >= 0x14) {
    replay->blink = 0;
    return 1;
  }
  return replay->blink <= 9;
}

static int32_t match_run_replay_roster_id(const struct fifa96_match_run *mr,
                                          uint32_t side, uint32_t slot) {
  if (mr->roster_order_ok[side] != 0) return (int32_t)mr->roster_order[side][slot];
  /* `sub_lineup` maps the 11 pitch records only; the bench slots 11..15 keep
   * their own slot id (the previous unguarded read was out of bounds). */
  if (mr->sub_lineup_ready != 0 && slot < FIFA96_MATCH_ENTITY_RECORDS &&
      mr->sub_lineup[side][slot] < FIFA96_CAMERA_REPLAY_SLOTS)
    return (int32_t)mr->sub_lineup[side][slot];
  return (int32_t)slot;
}

/* RR-L2: the recorder overlay gate's two edges (see the header contract).
 *
 * `_disable` is FUN_00053DC4's edge: `0x53DC5 CMP [0x14E574],0; 0x53DD3 CALL
 * 0x64074; 0x53DD8 [0x14E574]=1`. It closes the recorder gate and latches the
 * overlay only when the latch is clear (a repeat disable is a no-op).
 *
 * `_reenable` is FUN_00053A48's state-5->6 edge: `0x53A95 CMP ESI,[0x14E574];
 * 0x53A9B JZ 0x53AAD; 0x53A9D CALL 0x64080; 0x53AA2 [0x14E574]=0`. It reopens
 * the gate and clears the latch only when the latch is set. */
int fifa96_match_run_recorder_overlay_disable(struct fifa96_match_run *mr) {
  if (!mr) return 0;
  if (mr->recorder_latch_14e574 != 0) return 0;
  fifa96_match_replay_ring_record(mr, 0);
  mr->recorder_latch_14e574 = 1;
  return 1;
}

int fifa96_match_run_recorder_overlay_reenable(struct fifa96_match_run *mr) {
  if (!mr) return 0;
  if (mr->recorder_latch_14e574 == 0) return 0;
  fifa96_match_replay_ring_record(mr, 1);
  mr->recorder_latch_14e574 = 0;
  return 1;
}

/* RR-L10: the 20-dword replay FIFO (FUN_00064EF0) and the drain gate
 * (FUN_00064EC0). The C ring keeps `head` = next write and `tail` = oldest,
 * both 0..19, preserving the native observable push/pop order/eviction without
 * the native's pre-increment address offset (push index 1..20 of 0x155C8C vs
 * the C index 0..19 of 0x155C90). See the header contract. */
void fifa96_match_run_replay_fifo_reset(struct fifa96_match_run *mr) {
  if (!mr) return;
  mr->replay_fifo.count = 0;
  mr->replay_fifo.head = 0;
  mr->replay_fifo.tail = 0;
}

int fifa96_match_run_replay_fifo_push(struct fifa96_match_run *mr, uint32_t value) {
  if (!mr) return 0;
  if (mr->replay_fifo.count == 20) {
    /* The native evicts the tail when count == 20 before storing the new
     * head slot. */
    mr->replay_fifo.tail = (mr->replay_fifo.tail + 1) % 20;
    mr->replay_fifo.count--;
  }
  mr->replay_fifo.value[mr->replay_fifo.head] = value;
  mr->replay_fifo.head = (mr->replay_fifo.head + 1) % 20;
  mr->replay_fifo.count++;
  return 1;
}

int fifa96_match_run_replay_fifo_pop(struct fifa96_match_run *mr, uint32_t *value) {
  if (!mr || !value) return 0;
  if (mr->replay_fifo.count == 0) return 0;
  *value = mr->replay_fifo.value[mr->replay_fifo.tail];
  mr->replay_fifo.tail = (mr->replay_fifo.tail + 1) % 20;
  mr->replay_fifo.count--;
  return 1;
}

int fifa96_match_run_replay_fifo_drain(struct fifa96_match_run *mr, uint32_t value) {
  if (!mr) return 0;
  if ((mr->render.replay.state & 0x80u) != 0) return 0;
  if (mr->replay_screen_id_105ffc == 5 || mr->replay_screen_id_105ffc == 7)
    return 0;
  /* The native drain then calls the push with the caller's value; the
   * `[0x10A254]` voice notify (FUN_00065CC0 -> 0xA7705) stays a numbered leg. */
  return fifa96_match_run_replay_fifo_push(mr, value);
}


int fifa96_match_run_replay_begin(struct fifa96_match_run *mr) {
  struct fifa96_match_run_replay *r;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running || mr->paused ||
      mr->lc.screen != FIFA96_MATCH_SCREEN_ACTIVE)
    return -FIFA96_ERR_STATE;
  /* FUN_00064270 stores 0x81 when the replay buffer [0x109AB0] is allocated
   * (FUN_00063EBC). The engine's embedded recorded-frame ring is reset at init
   * and armed at match begin, so the live ACTIVE run is the modeled gate; no
   * 0x80 store exists in the native. */
  r = &mr->render.replay;
  r->state = FIFA96_MATCH_REPLAY_ARMED;
  r->hud_armed = 1;
  r->phase_counter = 0;
  r->cursor = 0;
  r->advance = 0;
  r->camera_index = 0;
  /* C1: the arm drops any switch/tackle edge latched before it and primes a
   * fresh replay input poll; an exit pending from an earlier replay is
   * superseded. */
  r->input_buttons = 0;
  r->input_buttons_held = 0;
  r->input_primed = 0;
  mr->live_switch_press = 0;
  mr->live_tackle_press = 0;
  mr->input_resync = 0;
  mr->replay_audio_armed = 1;   /* C4: the engine audio step arms the duck */
  (void)fifa96_match_run_replay_camera_set(mr, 0);
  return FIFA96_OK;
}

int fifa96_match_run_replay_target(struct fifa96_match_run *mr) {
  struct fifa96_match_run_replay *r;
  fifa96_camera_replay_target target;
  fifa96_camera_replay_activity activity;
  fifa96_camera_replay_choice choice;
  int rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  r = &mr->render.replay;
  memset(&target, 0, sizeof target);
  memset(&activity, 0, sizeof activity);
  target.score[0] = mr->score[0];
  target.score[1] = mr->score[1];
  target.event_count = r->goal_count;
  if (target.event_count > FIFA96_CAMERA_REPLAY_EVENTS)
    target.event_count = FIFA96_CAMERA_REPLAY_EVENTS;
  for (uint32_t i = 0; i < FIFA96_CAMERA_REPLAY_EVENTS; i++) {
    target.event_side[i] = r->goal_side[i];
    target.event_id[i] = r->goal_roster[i];
  }
  for (uint32_t side = 0; side < 2u; side++) {
    for (uint32_t slot = 0; slot < FIFA96_CAMERA_REPLAY_SLOTS; slot++) {
      int32_t id = match_run_replay_roster_id(mr, side, slot);
      target.order_id[side][slot] = id;
      activity.id[side][slot] = id;
      activity.count[side][slot] = r->activity_count[side][slot];
    }
  }
  rc = fifa96_camera_replay_choice_select(&target, &activity, &choice);
  if (rc != FIFA96_OK) return rc;
  for (uint32_t side = 0; side < 2u; side++)
    for (uint32_t slot = 0; slot < FIFA96_CAMERA_REPLAY_SLOTS; slot++)
      r->activity_count[side][slot] = activity.count[side][slot];
  mr->render.camera_target.source = 4;
  mr->render.camera_target.index = choice.target_index;
  mr->render.camera_target.force_focus = 0;
  mr->render.camera_target.replay_active = 0;
  return 1;
}

int fifa96_match_run_replay_camera_set(struct fifa96_match_run *mr, uint32_t index) {
  int32_t kind, sub;
  int rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  /* FUN_0004D134 stores [0x107DDC] = param_1 on every path (including the
   * default); only a mapped index also re-selects the camera record. */
  mr->render.replay.record_index = index;
  if (index > 6u) return 0;
  rc = fifa96_camera_replay_select(index, &kind, &sub);
  if (rc != 1) return rc;
  mr->render.replay.camera_index = (uint8_t)index;
  mr->render.replay.camera_kind = (uint8_t)kind;
  mr->render.replay.camera_sub = (uint8_t)sub;
  return 1;
}

void fifa96_match_run_replay_exit(struct fifa96_match_run *mr) {
  if (!mr) return;
  /* FUN_00064E8C's reachable state: [0x109A98] = 0 and the [0x109AC4] arm
   * freed (the hero/selection release and the audio sinks stay legs). */
  mr->render.replay.state = FIFA96_MATCH_REPLAY_LIVE;
  mr->render.replay.hud_armed = 0;
  /* C4: the engine audio step sees the cleared cell and releases the duck. */
  mr->replay_audio_armed = 0;
  /* C1: the next non-replay input poll re-syncs the switch/tackle physical
   * levels and drops any edge made during the replay. */
  mr->input_resync = 1;
  /* FUN_00036B98's camera reset: the normal view resumes on source 0. */
  if (mr->render.camera_target.source == 4) {
    mr->render.camera_target.source = 0;
    mr->render.camera_target.index = -1;
  }
}

/* RR-L10: the native `FUN_000974D8` transition, run before a step/slow arm.
 * The body is unproved (leg); the port keeps the call site as a no-op. */
static void replay_transition(struct fifa96_match_run *mr) {
  (void)mr;
}

/* RR-L10/RR-L3: the native `FUN_0006428C` resync, run after each countdown
 * step. `MOV EAX,[0x155C4C]; SAR EAX,0x18` sign-extends the queue-pop byte
 * (`0x155C4F`) into EAX; `JZ` skips when it is zero, else `DEC EAX` (a
 * register-only decrement — the cell is never stored back) and `JMP 0x974DC`,
 * which runs the `0x64EC0` drain gate then `0x65CC0`. The `FUN_00065CC0`
 * notification body is a numbered leg (globals only, `0xA7705` call; audio
 * inferred). */
void fifa96_match_run_replay_resync(struct fifa96_match_run *mr) {
  int32_t value = (int32_t)(int8_t)mr->replay_queue_pop_155c4f; /* SAR 0x18 */
  if (value == 0) return;
  (void)fifa96_match_run_replay_fifo_drain(mr, (uint32_t)(value - 1));
}

int fifa96_match_run_replay_step(struct fifa96_match_run *mr, uint32_t buttons) {
  struct fifa96_match_run_replay *r;
  if (!mr) return -FIFA96_ERR_INVALID;
  r = &mr->render.replay;
  if (r->state == FIFA96_MATCH_REPLAY_LIVE) return 0;
  if (r->state == 0x80) r->state = FIFA96_MATCH_REPLAY_ARMED;
  if (r->state == FIFA96_MATCH_REPLAY_ARMED) {
    /* The 0x81 arm: reset the cursor and apply record 0 when the ring holds
     * one (advancing the cursor to 1); an empty ring leaves the cursor at 0.
     * The cursor limit mirrors the ring count, the advance/wait timers reset,
     * camera 0 reselects and the target chooser runs, then the state settles
     * at IDLE. */
    r->cursor = 0;
    if (mr->replay_ring.count > 0) {
      fifa96_match_replay_apply(mr, 0);
      r->cursor = 1;
    }
    r->cursor_limit = mr->replay_ring.count;
    r->advance = 0;
    r->wait_timer = 0;
    r->camera_index = 0;
    (void)fifa96_match_run_replay_camera_set(mr, 0);
    (void)fifa96_match_run_replay_target(mr);
    r->state = FIFA96_MATCH_REPLAY_IDLE;
    return 0;
  }
  /* The next-call reset: a step/slow state (or the engine-only 0x87) falls
   * back to IDLE at the top of the following call (R6). */
  if (r->state == FIFA96_MATCH_REPLAY_STEP_FWD ||
      r->state == FIFA96_MATCH_REPLAY_STEP_BACK ||
      r->state == FIFA96_MATCH_REPLAY_SLOW || r->state == 0x87)
    r->state = FIFA96_MATCH_REPLAY_IDLE;
  if (buttons == 1u) {
    /* The play/pause toggle with the end rewind: at the end the cursor wraps
     * to the start and record 0 is applied (the cursor advances to 1). */
    if (r->cursor_limit != 0 && r->cursor >= r->cursor_limit) {
      r->cursor = 0;
      fifa96_match_replay_apply(mr, 0);
      r->cursor = 1;
    }
    r->state = r->state == FIFA96_MATCH_REPLAY_IDLE
                   ? FIFA96_MATCH_REPLAY_PLAYING
                   : FIFA96_MATCH_REPLAY_IDLE;
    return 0;
  }
  if ((buttons & 0x80u) != 0) {   /* the exit arm */
    fifa96_match_run_replay_exit(mr);
    return 0;
  }
  if (buttons == 0x20u) {
    r->camera_index = (uint8_t)((r->camera_index + 1u) % 6u);
    (void)fifa96_match_run_replay_camera_set(mr, r->camera_index);
  } else if (buttons == 8u) {
    replay_transition(mr);
    r->state = FIFA96_MATCH_REPLAY_STEP_BACK;
  } else if (buttons == 4u) {
    replay_transition(mr);
    r->state = FIFA96_MATCH_REPLAY_STEP_FWD;
  } else if (buttons == 2u) {
    replay_transition(mr);
    r->state = FIFA96_MATCH_REPLAY_SLOW;
  }
  switch (r->state) {
    case FIFA96_MATCH_REPLAY_IDLE:
      r->wait_timer = 0;                  /* case 0x82 */
      break;
    case FIFA96_MATCH_REPLAY_PLAYING: {
      uint16_t stamp;
      r->advance += 1;                    /* case 0x83: the ring tick */
      stamp = fifa96_match_replay_stamp(mr);
      while (stamp > 0 && stamp <= (uint32_t)r->advance) {
        r->advance -= (int32_t)stamp;
        fifa96_match_replay_step_forward(mr);
        fifa96_match_run_replay_resync(mr);
        stamp = fifa96_match_replay_stamp(mr);
      }
      break;
    }
    case FIFA96_MATCH_REPLAY_STEP_FWD:
      fifa96_match_replay_step_forward(mr);
      fifa96_match_replay_step_forward(mr);
      break;
    case FIFA96_MATCH_REPLAY_STEP_BACK:
      fifa96_match_replay_step_back(mr);
      fifa96_match_replay_step_back(mr);
      break;
    case FIFA96_MATCH_REPLAY_SLOW:
      if (r->wait_timer > 0) {
        r->wait_timer--;                  /* case 0x86: the reload countdown */
      } else {
        fifa96_match_replay_step_forward(mr);
        r->wait_timer = r->wait_reload;
      }
      break;
    default:
      break;
  }
  return 0;
}

int fifa96_match_run_replay_row(const struct fifa96_match_run *mr) {
  const struct fifa96_match_run_replay *r;
  if (!mr) return FIFA96_MATCH_REPLAY_ROW_NONE;
  r = &mr->render.replay;
  /* FUN_000565BC R2: replay family, not the armed state, outside the ramp
   * window; the FUN_00064C34 row additionally needs the [0x109AC4] arm. */
  if (!fifa96_match_run_replay_family(r)) return FIFA96_MATCH_REPLAY_ROW_NONE;
  if (fifa96_match_run_replay_armed(r)) return FIFA96_MATCH_REPLAY_ROW_NONE;
  if (fifa96_match_run_replay_ramp_window(r)) return FIFA96_MATCH_REPLAY_ROW_NONE;
  if (r->phase_counter == 0)
    return r->hud_armed ? FIFA96_MATCH_REPLAY_ROW_HUD
                        : FIFA96_MATCH_REPLAY_ROW_NONE;
  return FIFA96_MATCH_REPLAY_ROW_BLINK;
}

int fifa96_match_run_sub_mark(const struct fifa96_match_run_sub *sub, int side,
                              int slot) {
  int k, idx;
  if (!sub || slot < 0 || slot >= 5 || side < 0 || side > 1) return 0;
  k = sub->cursor_raw % 5;               /* FUN_0004BD38 */
  idx = slot * 2 + side;
  if (slot == k) {
    if (sub->record_side == (uint8_t)side) {
      if (sub->mode == 5 && sub->special) return 2;
      return 3;
    }
    if (sub->current_side == (uint8_t)side) return 1 + (sub->flags[idx] != 0);
  } else if (slot < k) {
    return 1 + (sub->flags[idx] != 0);
  }
  return 0;
}

int fifa96_match_run_sub_numbers(const struct fifa96_match_run_sub *sub, int *p1,
                                 int *p2) {
  /* FUN_0004BDF8: [0x1587E7] < 5 takes the record bytes +1/+5, else the
   * +0x10/+0x11 bytes of the [0x1587D4] block. */
  if (!sub || !p1 || !p2) return -FIFA96_ERR_INVALID;
  if (sub->cursor_raw < 5) {
    *p1 = sub->record[1];       /* 0x1587D5 */
    *p2 = sub->record[5];       /* 0x1587D9 */
  } else {
    *p1 = sub->record[0x10];    /* 0x1587E4 */
    *p2 = sub->record[0x11];    /* 0x1587E5 */
  }
  return FIFA96_OK;
}

int fifa96_match_run_overlay_row(uint16_t id) {
  /* FUN_000550E4's switch table (fresh decompile §2.7). */
  switch (id) {
    case 0: return FIFA96_OVERLAY_ROW_PERIOD;
    case 1: case 4: return FIFA96_OVERLAY_ROW_PLAYER_LIST;
    case 5: case 0xB: case 0xE: return FIFA96_OVERLAY_ROW_SUBSTITUTION;
    case 6: return FIFA96_OVERLAY_ROW_RECORD_INFO;
    case 7: case 0xC: return FIFA96_OVERLAY_ROW_PERIOD_STRING;
    case 8: return FIFA96_OVERLAY_ROW_LIST;
    case 9: return FIFA96_OVERLAY_ROW_MESSAGE;
    case 0xA: return FIFA96_OVERLAY_ROW_LIST_MESSAGE;
    case 0xD: return FIFA96_OVERLAY_ROW_STATS;
    case 0xF: return FIFA96_OVERLAY_ROW_PLAYER_NAME;
    case 0x11: return FIFA96_OVERLAY_ROW_EXTRA_TIME;
    default: return FIFA96_OVERLAY_ROW_NONE;
  }
}

int fifa96_match_run_overlay_arm(struct fifa96_match_run_overlay *overlay, uint16_t id) {
  if (!overlay) return -FIFA96_ERR_INVALID;
  overlay->id = id;
  overlay->armed = 1;                    /* [0x14E674] = id | 0x8000 */
  overlay->timer = 0;
  switch (id) {
    case 5: case 9: case 0xB: case 0xC: case 0xD: case 0xE: case 0xF: case 0x12:
      overlay->second = 1;               /* the [0x14E5C8] seed set */
      break;
    default:
      overlay->second = 0;
      break;
  }
  return 1;
}

/* FUN_00053E08: flip the timer direction against the rate. */
static int match_run_overlay_flip(int32_t *direction, int32_t rate) {
  if (*direction == 0) {
    *direction = -rate;
    return 1;
  }
  if (*direction * rate > 0) {
    *direction = -*direction;
    return 1;
  }
  return 0;
}

int fifa96_match_run_overlay_timeout_step(struct fifa96_match_run_overlay *overlay,
                                          int32_t tick) {
  int flipped = 0;
  if (!overlay) return -FIFA96_ERR_INVALID;
  /* FUN_000542D4: the [0x14E698]*[0x14E69C] >= 0 gate (engine: the overlay's
   * own direction/rate), then the timeout compare + FUN_00053E08 flip. */
  if ((int64_t)overlay->direction * overlay->rate < 0) return 0;
  if (!overlay->second) {
    overlay->timer += tick;
    if (overlay->timer <= overlay->timeout) return 0;
    if (overlay->armed) {
      overlay->timer = 0;
      flipped |= match_run_overlay_flip(&overlay->direction, overlay->rate);
    }
  } else {
    overlay->second_timer += tick;
    if (overlay->second_timer <= overlay->second_timeout) return 0;
    if (overlay->armed) {
      overlay->timer = 0;
      flipped |= match_run_overlay_flip(&overlay->direction, overlay->rate);
    }
  }
  if (overlay->second) {
    overlay->second_timer = 0;
    flipped |= match_run_overlay_flip(&overlay->second_direction, overlay->second_rate);
  }
  return flipped;
}

int fifa96_match_run_overlay_visible(const struct fifa96_match_run *mr) {
  const struct fifa96_match_run_overlay *overlay;
  if (!mr) return 0;
  overlay = &mr->render.overlay;
  if (!overlay->armed) return 0;                     /* [0x14E674] & 0x8000 */
  if (mr->render.display.suspend) return 0;          /* [0x14E688] == 0 */
  if (fifa96_match_run_replay_family(&mr->render.replay)) return 0;
  if (overlay->extra_time && (overlay->id & 0xFFu) != 0x11u) return 0;
  return 1;
}

int fifa96_match_run_ball_row_reachable(const struct fifa96_match_run *mr) {
  /* FU-152 §2.5 leg 6: the only nonzero-capable writer FUN_00056690 is always
   * called with EDX=0 (0x56E1B..0x56E41), so the native row never draws. The
   * engine mirrors the native writer's zero with a NULL staged pointer. */
  return mr != NULL && mr->render.ball_row != NULL;
}

/* The row font follows the HUD's window branch (FUN_0004AFB8(0x35/0x36)). */
static const struct fifa96_font *match_run_row_font(
    const struct fifa96_match_run_render *r) {
  return r->window_zoomed ? &r->hud_font[1] : &r->hud_font[0];
}

static int match_run_row_font_ready(const struct fifa96_match_run_render *r) {
  return (r->window_zoomed ? r->hud_font_ready[1] : r->hud_font_ready[0]) != 0 &&
         (r->window_zoomed ? r->hud_font[1].data : r->hud_font[0].data) != NULL;
}

/* R1 (FUN_000560C8 + FUN_0005619C): the "%d - %d" pair centred on the
 * 0xA0/0x140 axis at `y_text + 2*scale`, the per-side 5 marks (FUN_0004BD38
 * states 1..3, staged glyph) and the team name in the x_left/x_right row. */
static void match_run_draw_sub_strip(const struct fifa96_match_run *mr,
                                     struct fifa96_surface *s) {
  const struct fifa96_match_run_render *r = &mr->render;
  const struct fifa96_match_run_sub *sub = &r->sub;
  fifa96_window_strip strip;
  int32_t scale_x = 0, scale_y = 0;
  const struct fifa96_font *font;
  int p1 = 0, p2 = 0;
  char buf[24];
  int y;
  if (!sub->active) return;
  /* R1 sits outside the [0x14E688] suspend block: the native gates are
   * [0x14E538] (asset readiness, covered by the font check), the menu probe,
   * FUN_0006400C (live) and the {0xC,0x13,0x14} mode exclusion. */
  if (!fifa96_match_run_replay_live(&r->replay)) return;   /* FUN_0006400C */
  if (sub->mode == 0xC || sub->mode == 0x13 || sub->mode == 0x14) return;
  if (!match_run_row_font_ready(r)) return;
  if (fifa96_window_strip_layout(&r->window, sub->frame5_height, sub->name_width,
                                 sub->wide, &strip) != FIFA96_OK)
    return;
  (void)fifa96_window_scale(&r->window, &scale_x, &scale_y);
  font = match_run_row_font(r);
  (void)fifa96_match_run_sub_numbers(sub, &p1, &p2);
  (void)snprintf(buf, sizeof buf, "%d - %d", p1, p2);
  y = strip.row[0].y_text + (int)(((int64_t)2 * scale_y + 0x8000) >> 16);
  (void)fifa96_font_draw_centered(s->indexed, s->width, s->height, font, buf, y,
                                  scale_x, sub->wide);
  if (sub->mark_sprite && sub->mark_sprite->pixels) {
    fifa96_render_clip clip = { 0, 0, s->width, s->height };
    for (int side = 0; side < 2; side++) {
      for (int slot = 0; slot < 5; slot++) {
        int mark = fifa96_match_run_sub_mark(sub, side, slot);
        if (mark < 1 || mark > 3) continue;
        int x = strip.row[side].x + slot * sub->mark_pitch;
        int my = strip.row[side].y0 + (mark - 1) * sub->mark_step;
        (void)match_run_draw_sprite(s, sub->mark_sprite, r->remap, x, my, 0x10000, 0,
                                    &clip);
      }
    }
  }
  for (int side = 0; side < 2; side++) {
    int w, scaled_w, max_w, x;
    if (!sub->name[side][0]) continue;
    w = fifa96_font_text_width(font, sub->name[side]);
    if (sub->wide) w >>= 1;               /* native FUN_00044BE0 halving */
    scaled_w = (int)(((int64_t)w * scale_x + 0x8000) >> 16);
    max_w = (int)(((int64_t)sub->name_width * scale_x + 0x8000) >> 16);
    x = strip.row[side].x + ((max_w - scaled_w) >> 1);
    if (x < sub->x_clamp_lo + 2) x = sub->x_clamp_lo + 2;
    if (x + scaled_w > sub->x_clamp_hi) x = sub->x_clamp_hi - scaled_w - 2;
    match_run_hud_text(s, font, sub->name[side], x, strip.row[side].y0, 0x00, 0x06,
                       NULL);
  }
}

/* R2 (FUN_00064C34 / FUN_000564A0): the staged caption at its cell; the
 * progress bar/button glyphs are asset legs (1/2/3). The blink row advances
 * the FUN_000564A0 counter once per drawn buffer. */
static void match_run_draw_replay(struct fifa96_match_run *mr,
                                  struct fifa96_surface *s) {
  struct fifa96_match_run_render *r = &mr->render;
  int row = fifa96_match_run_replay_row(mr);
  const char *caption;
  if (row == FIFA96_MATCH_REPLAY_ROW_NONE) return;
  if (!match_run_row_font_ready(r)) return;
  caption = r->replay.captions[r->replay.camera_index % 6u];
  if (!caption[0]) return;
  if (row == FIFA96_MATCH_REPLAY_ROW_BLINK &&
      !fifa96_match_run_replay_blink_step(&r->replay, 1))
    return;
  match_run_hud_text(s, match_run_row_font(r), caption, r->replay.caption_x,
                     r->replay.caption_y, 0x00, 0x06, NULL);
}

/* R3 (FUN_000550E4): the case row's staged lines; the per-case helpers and
 * ids are legs (7). */
static void match_run_draw_overlay(const struct fifa96_match_run *mr,
                                   struct fifa96_surface *s) {
  const struct fifa96_match_run_render *r = &mr->render;
  const struct fifa96_match_run_overlay *overlay = &r->overlay;
  if (!fifa96_match_run_overlay_visible(mr)) return;
  if (fifa96_match_run_overlay_row(overlay->id) == FIFA96_OVERLAY_ROW_NONE) return;
  if (!match_run_row_font_ready(r)) return;
  for (int i = 0; i < 4; i++) {
    if (!overlay->lines[i][0]) continue;
    match_run_hud_text(s, match_run_row_font(r), overlay->lines[i], overlay->line_x[i],
                       overlay->line_y[i], 0x00, 0x06, NULL);
  }
}

/* R5 (FUN_00056518): the dormant ball row. The native rect is the
 * FUN_00053240 block's `x1 - 0x2C*scale`, `y0 + 3*scale` cell; the staged
 * pointer stays NULL because the only writer passes 0 (leg 6). */
static void match_run_draw_ball_row(const struct fifa96_match_run *mr,
                                    struct fifa96_surface *s) {
  const struct fifa96_match_run_render *r = &mr->render;
  int32_t scale_x = 0, scale_y = 0;
  fifa96_render_clip clip = { 0, 0, s->width, s->height };
  int x, y;
  if (!r->ball_row || !r->ball_row->pixels) return;
  if (fifa96_window_scale(&r->window, &scale_x, &scale_y) != FIFA96_OK) return;
  x = r->window.box.x1 - (int)(((int64_t)0x2C * scale_x + 0x8000) >> 16);
  y = r->window.box.y0 + (int)(((int64_t)3 * scale_y + 0x8000) >> 16);
  (void)match_run_draw_sprite(s, r->ball_row, r->remap, x, y, 0x10000, 0, &clip);
}

/* P10 graphics: a soft ground shadow under each live sprite. The indexed
 * surface has no alpha; a small dark parabolic patch at the projected pivot is
 * enough to seat the players on the pitch. */
static void match_run_draw_shadow(struct fifa96_surface *s,
                                  const fifa96_render_clip *clip, int x, int y,
                                  int rw, int rh, const uint8_t *shade,
                                  uint8_t fallback) {
  if (s == NULL || clip == NULL || rw <= 0 || rh <= 0) return;
  int left = clip->left < 0 ? 0 : clip->left;
  int top = clip->top < 0 ? 0 : clip->top;
  int right = clip->right > s->width ? s->width : clip->right;
  int bottom = clip->bottom > s->height ? s->height : clip->bottom;
  for (int dy = -rh; dy <= rh; dy++) {
    int py = y + dy;
    if (py < top || py >= bottom) continue;
    int span = rw * (rh * rh - dy * dy) / (rh * rh);
    if (span <= 0) continue;
    int x0 = x - span;
    int x1 = x + span;
    if (x0 < left) x0 = left;
    if (x1 >= right) x1 = right - 1;
    if (x1 < x0) continue;
    uint8_t *row = s->indexed + (size_t)py * (size_t)s->width;
    for (int px = x0; px <= x1; px++)
      row[px] = shade != NULL ? shade[row[px]] : fallback;
  }
}

/* P12: the derived goal-box pass. The retail Net.fsh frame is a 64x64 indexed
 * mesh (index 0 transparent); the goal mouth is the native classifier's
 * |x| < 0xD0, height 0xA0, on the goal-line plane z = ±0xC1E. The plane is
 * inverse-projected exactly like the pitch floor, so the net follows the
 * camera without a screen-space hack. Posts/crossbar are the white marking
 * slot. This is a derived [D] back-net plane; the native 3D net/side/top
 * passes remain P10-L2. */
#define MATCH_RUN_GOAL_MOUTH_HALF 0xD0
#define MATCH_RUN_GOAL_HEIGHT 0xA0
#define MATCH_RUN_GOAL_LINE_Z 0xC1E
#define MATCH_RUN_GOAL_POST_W 8

static void match_run_draw_goal_net(const struct fifa96_match_run_render *r,
                                    struct fifa96_surface *s,
                                    const fifa96_render_pos *eye,
                                    const int32_t matrix[9],
                                    const fifa96_render_clip *clip,
                                    const fifa96_projection_point *center,
                                    int32_t fov_w, int32_t fov_h) {
  if (!r->goal_net_ready || r->goal_net_frame.pixels == NULL ||
      r->goal_net_frame.width <= 0 || r->goal_net_frame.height <= 0)
    return;
  const struct fifa96_sprite_frame *net = &r->goal_net_frame;
  int32_t mt[9];
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) mt[i * 3 + j] = matrix[j * 3 + i];
  for (int side = -1; side <= 1; side += 2) {
    const int32_t gz = side * MATCH_RUN_GOAL_LINE_Z;
    for (int32_t sy = clip->top; sy < clip->bottom; sy++) {
      uint8_t *row = s->indexed + (size_t)sy * (size_t)s->width;
      int32_t dy = (int32_t)(((int64_t)center->y - ((int64_t)sy << 16)) /
                             (int64_t)fov_h);
      for (int32_t sx = clip->left; sx < clip->right; sx++) {
        int32_t dx = (int32_t)((((int64_t)sx << 16) - (int64_t)center->x) /
                               (int64_t)fov_w);
        fifa96_projection_vec cam_dir = {dx, dy, 1 << 16};
        fifa96_projection_vec dir;
        if (fifa96_projection_transform(mt, &cam_dir, &dir) != FIFA96_OK ||
            dir.z == 0)
          continue;
        int64_t t = ((int64_t)(gz - eye->z) * 65536) / dir.z;
        /* The derived inverse-projected plane is only a far-goal backdrop; a
         * plane within 0x400 world units of the eye is the camera passing a
         * goal and must not paint a mesh across the pitch (P12 [D]). */
        if (t <= 0x400) continue;
        int32_t wx = eye->x + (int32_t)((t * dir.x) >> 16);
        int32_t wy = eye->y + (int32_t)((t * dir.y) >> 16);
        if (wx < -MATCH_RUN_GOAL_MOUTH_HALF || wx > MATCH_RUN_GOAL_MOUTH_HALF ||
            wy < 0 || wy > MATCH_RUN_GOAL_HEIGHT)
          continue;
        /* The goal interior is dark behind the white mesh; otherwise the net
         * vanishes against the white hoardings behind the goal line. */
        row[sx] = 0x00;
        if (wx <= -MATCH_RUN_GOAL_MOUTH_HALF + MATCH_RUN_GOAL_POST_W ||
            wx >= MATCH_RUN_GOAL_MOUTH_HALF - MATCH_RUN_GOAL_POST_W ||
            wy >= MATCH_RUN_GOAL_HEIGHT - MATCH_RUN_GOAL_POST_W) {
          row[sx] = r->pitch_art.line_index;
          continue;
        }
        int32_t u = (int32_t)(((int64_t)(wx + MATCH_RUN_GOAL_MOUTH_HALF) *
                               net->width) /
                              (2 * MATCH_RUN_GOAL_MOUTH_HALF));
        int32_t v = (int32_t)(((int64_t)(MATCH_RUN_GOAL_HEIGHT - wy) *
                               net->height) /
                              MATCH_RUN_GOAL_HEIGHT);
        if (u < 0) u = 0;
        if (u >= net->width) u = net->width - 1;
        if (v < 0) v = 0;
        if (v >= net->height) v = net->height - 1;
        uint8_t px = net->pixels[(size_t)v * (size_t)net->width + (size_t)u];
        /* The retail Net.fsh frame is a gray 187..255 mesh, not an index-0
         * transparent sprite: 187 is the mesh background and the >=200 values
         * are the white cords. Keep the goal interior dark and draw only the
         * cords, otherwise the frame is a solid white/red rectangle. */
        if (px >= 200u) row[sx] = r->pitch_art.line_index;
      }
    }
  }
}

int fifa96_match_run_render(struct fifa96_match_run *mr, struct fifa96_surface *s) {
  if (!mr || !s) return -FIFA96_ERR_INVALID;
  struct fifa96_match_run_render *r = &mr->render;
  if (!r->enabled) return 0;
  if (!r->frames || !r->banks || r->bank_count == 0 || !r->sprite_data ||
      r->sprite_data_len < 16u)
    return -FIFA96_ERR_STATE;

  /* OL-T11-6: install the staged native match palette on the presented target
   * before the plane conversion (the native match-data load installs the
   * palette before any match frame draws). */
  if (r->palette_ready) (void)fifa96_match_run_palette_install(mr, s);

  /* A zero window box means "not defined": fall back to the full surface (the
   * live match's window A/B selection is an open leg). */
  const int have_window = r->window.box.w > 0 && r->window.box.h > 0;

  /* FU-93 §1: window rect -> 16.16 zoom scale (the overlay/HUD coordinate
   * seam; the FU-88 entity projection has no zoom term, FU-88 §5/§8). */
  int32_t scale_x = 0;
  int32_t scale_y = 0;
  int zoomed = 0;
  if (have_window) {
    (void)fifa96_window_scale(&r->window, &scale_x, &scale_y);
    (void)fifa96_window_zoomed(&r->window, &zoomed);
  }
  r->window_scale_x = scale_x;
  r->window_scale_y = scale_y;
  r->window_zoomed = zoomed;

  fifa96_surface_clear(s, r->background);

  /* FU-92 §1/§5: the 16.16 clip rect is the render window; a zero box falls
   * back to the full surface. The projection centre is the window's
   * src_x + src_w/2. */
  fifa96_render_clip clip;
  fifa96_projection_point center;
  if (have_window) {
    clip.left = r->window.box.x0;
    clip.top = r->window.box.y0;
    clip.right = r->window.box.x1;
    clip.bottom = r->window.box.y1;
    center.x = r->window.cx_fix;
    center.y = r->window.cy_fix;
  } else {
    clip.left = 0;
    clip.top = 0;
    clip.right = s->width;
    clip.bottom = s->height;
    center.x = (s->width >> 1) << 16;
    center.y = (s->height >> 1) << 16;
  }
  /* The window's screen dimensions come from the engine surface at begin, but
   * s is a caller argument: clamp the clip to the target surface so the span
   * blit can never write past its logical width/height (or overflow the
   * column scratch). */
  if (clip.left < 0) clip.left = 0;
  if (clip.top < 0) clip.top = 0;
  if (clip.right > s->width) clip.right = s->width;
  if (clip.bottom > s->height) clip.bottom = s->height;

  int32_t matrix[9];
  int32_t recip_x[FIFA96_PROJECTION_RECIP_COUNT];
  int32_t recip_y[FIFA96_PROJECTION_RECIP_COUNT];
  /* Phase 81: the live pose uses the class-3 side envelope (`r->yaw` = 0xC000,
   * `r->pitch` = 0x2000) and the full surface lens. Non-live keeps the image
   * default 0xBF9C/200 and the pinned projection dimensions. The ratio 4608
   * remains the native camera field, not an entity FOV term (FU-88 §5). */
  int32_t lens_w = s->width;
  int32_t lens_h = s->height;
  int32_t view_yaw = r->yaw;
  int32_t view_pitch = r->pitch;
  fifa96_err_t err = fifa96_projection_matrix(view_yaw, view_pitch, matrix);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_projection_reciprocal(lens_w, recip_x);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_projection_reciprocal(lens_h, recip_y);
  if (err != FIFA96_OK) return (int)err;

  /* FU-85 §4 camera staging (FUN_00036C70's camera copy/bounds over the FU-71
   * block). Phase 81 [D]: the live path renders from the class-3 side envelope
   * (focus + (-500,420,0)); the non-live path keeps the FU-71 focus triple
   * unchanged (M1/M2). */
  fifa96_render_pos cam_src;
  fifa96_match_run_render_eye(mr, &cam_src);
  fifa96_render_pos cam;
  err = fifa96_render_camera_stage(&cam_src, &cam);
  if (err != FIFA96_OK) return (int)err;

  /* FU-343 (P1): the visible-match pitch, after the background clear and
   * before the entity sort so every sprite blits over the floor. Only the live
   * path draws it; the pitch and sprite passes share `cam`. */
  if (mr->live_start && r->pitch_art.staged) {
    r->pitch_art.shade = r->pitch_shade;
    r->pitch_art.palette = r->palette;
    err = (fifa96_err_t)fifa96_pitch_draw(&r->pitch_art, s, &cam, view_yaw, matrix,
                                          &clip, &center, lens_w, lens_h);
    if (err != FIFA96_OK) return (int)err;
    match_run_draw_goal_net(r, s, &cam, matrix, &clip, &center, lens_w, lens_h);
  }

  /* FU-89 §6/FUN_000589E0: the near-depth threshold `[0x54350]` is
   * `max((0x14<<16)/(2*camera +0x4C ratio), 0x78)`, or the 0x78 fallback when
   * the camera y is below 0x140 (see MATCH_RUN_NEAR_LIMIT). The gate below
   * compares it against each entry's jittered depth key. */
  int32_t near_threshold = 0;
  err = fifa96_scene_threshold(r->view_ratio, r->camera.pos_y,
                               FIFA96_SCENE_CLAMP_MIN, MATCH_RUN_NEAR_LIMIT,
                               &near_threshold);
  if (err != FIFA96_OK) return (int)err;

  uint32_t count = r->entity_count;
  if (count > FIFA96_MATCH_RUN_RENDER_SLOTS) count = FIFA96_MATCH_RUN_RENDER_SLOTS;

  fifa96_scene_slot slots[FIFA96_MATCH_RUN_RENDER_SLOTS];
  int32_t staged_x[FIFA96_MATCH_RUN_RENDER_SLOTS];
  int32_t staged_y[FIFA96_MATCH_RUN_RENDER_SLOTS];
  int32_t angles[FIFA96_MATCH_RUN_RENDER_SLOTS];
  /* Phase-84: the native 0xA2A10 view/position angle addend. The call site
   * (0x571AA..0x571CA) feeds the camera-relative world pair `[0x543A8+0/+8]`
   * (the pre-rotation `x`/`z`) into the 16-bit atan and adds the result to the
   * staged record angle before the 0..7 direction fold. Without it the sprite
   * direction is the record facing alone, which is only correct at one screen
   * position (the walking/facing defect). Live-only: the M1/M2 tapes keep the
   * recorded direction fold byte-exact. */
  int32_t view_addend[FIFA96_MATCH_RUN_RENDER_SLOTS];
  uint8_t drawable[FIFA96_MATCH_RUN_RENDER_SLOTS];
  int32_t jitter_z[FIFA96_MATCH_RUN_RENDER_SLOTS * FIFA96_SCENE_POSITION_DWORDS];
  uint32_t list[FIFA96_MATCH_RUN_RENDER_SLOTS];
  for (uint32_t i = 0; i < count; i++) {
    fifa96_render_slot slot;
    err = fifa96_render_slot_stage(&r->entities[i].stage, &slot);
    if (err != FIFA96_OK) return (int)err;
    staged_x[i] = slot.pos.x;
    staged_y[i] = slot.pos.y;
    angles[i] = slot.angle;
    fifa96_projection_vec rel;
    rel.x = (int32_t)((uint32_t)slot.pos.x - (uint32_t)cam.x);
    rel.y = (int32_t)((uint32_t)slot.pos.y - (uint32_t)cam.y);
    rel.z = (int32_t)((uint32_t)slot.pos.z - (uint32_t)cam.z);
    view_addend[i] = fifa96_match_run_view_addend(rel.x, rel.z);
    /* FU-89 §7: the jittered array adds `i%6 + 0x70` to y before rotation. */
    fifa96_projection_vec rel_jitter = rel;
    rel_jitter.y = (int32_t)((uint32_t)rel_jitter.y + (uint32_t)(0x70u + (i % 6u)));
    fifa96_projection_vec clean_rot;
    fifa96_projection_vec jitter_rot;
    err = fifa96_projection_transform(matrix, &rel, &clean_rot);
    if (err != FIFA96_OK) return (int)err;
    err = fifa96_projection_transform(matrix, &rel_jitter, &jitter_rot);
    if (err != FIFA96_OK) return (int)err;
    err = fifa96_scene_slot_project(recip_x, recip_y, &center, &clean_rot, &jitter_rot,
                                    &slots[i]);
    if (err != FIFA96_OK) return (int)err;
    jitter_z[i * FIFA96_SCENE_POSITION_DWORDS + 2] = jitter_rot.z;
    list[i] = i;
    drawable[i] = (uint8_t)(slots[i].clean_visible && slots[i].jitter_visible);
  }
  if (mr->live_start) {
    int shadow_scale = s->width / 320;
    if (shadow_scale < 1) shadow_scale = 1;
    for (uint32_t i = 0; i < count; i++) {
      if (!drawable[i]) continue;
      int sx = (int)(slots[i].clean.x >> 16);
      int sy = (int)(slots[i].clean.y >> 16);
      int rw = (i == 22u ? 4 : 14) * shadow_scale;
      int rh = (i == 22u ? 2 : 5) * shadow_scale;
      int ox = (i == 22u ? 2 : 7) * shadow_scale;
      int oy = (i == 22u ? 1 : 3) * shadow_scale;
      match_run_draw_shadow(s, &clip, sx - ox, sy + oy, rw, rh,
                            r->shadow_shade, 0x00);
    }
  }

  /* FU-89 §2 (corrected first-hand 0x57798..0x577AC): seed keys[k] =
   * z(jittered rotated point of list[k]) and shell-sort the tandem arrays
   * descending. The native list is the fixed 0x18-entry array pre-seeded with
   * the 1-based slot values 0..0x17 by FUN_00056CF4; value 0 is the empty
   * sentinel (skipped at 0x57CD2), so the staged 23 slots occupy the list's
   * values 1..23 and the engine builds the same 24 entries: `values[i] = i+1`
   * for the count slots plus the sentinel `values[count] = 0`. The sentinel's
   * native key is the scratch triple copied to [0x54370] (z of the block-B
   * +0xCB4 triple); the engine uses 0 (that producer is unmodeled). */
  int32_t keys[FIFA96_MATCH_RUN_RENDER_SLOTS + 1];
  uint32_t values[FIFA96_MATCH_RUN_RENDER_SLOTS + 1];
  for (uint32_t i = 0; i < count; i++) values[i] = i + 1;
  values[count] = 0;
  keys[count] = 0;
  err = fifa96_scene_build_keys(count, list, jitter_z, count, keys);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_scene_sort(count + 1, keys, values);
  if (err != FIFA96_OK) return (int)err;

  const fifa96_sprite_bank sprite_bank = {
    .data = r->sprite_data,
    .total_size = r->sprite_data_len,
    .count = 0,
    .tag = { 0, 0, 0, 0 },
  };
  for (uint32_t i = 0; i <= count; i++) {
    uint32_t v = values[i];
    if (v == 0) continue;     /* 0x57CD2: the 1-based empty slot */
    uint32_t slot = v - 1;
    if (slot >= count) continue;
    uint8_t visible = 0;
    /* FU-89 §3.2: the lateral gate reads the staged triple's x (`[0x9A74]` =
     * 0x155AB0, first-hand) and culls it when it is > 0x8E0. */
    err = fifa96_scene_slot_gate(near_threshold, keys[i], staged_y[slot], staged_x[slot],
                                 &visible);
    if (err != FIFA96_OK) return (int)err;
    if (!visible || !drawable[slot]) continue;
    int32_t jitter_y_out = slots[slot].jitter.y;
    uint8_t draw = 0;
    err = fifa96_scene_clip_edges((int32_t)((uint32_t)clip.left << 16),
                                  (int32_t)((uint32_t)clip.top << 16),
                                  (int32_t)((uint32_t)clip.right << 16),
                                  (int32_t)((uint32_t)clip.bottom << 16), slots[slot].clean.y,
                                  slots[slot].jitter.x, slots[slot].jitter.y, &jitter_y_out, &draw);
    if (err != FIFA96_OK) return (int)err;
    if (!draw) continue;

    /* FU-85 §2 direction: `(7 - (((angle - 0x1000) & 0xFFFF) >> 13)) & 7`.
     * Phase-84 ports the 0xA2A10 view/position addend: the 16-bit atan of the
     * camera-relative world pair is added before the fold (0x571CA). */
    int32_t dir_angle =
        (int32_t)((uint32_t)angles[slot] - 0x1000u +
                  (mr->live_start ? (uint32_t)view_addend[slot] : 0u));
    int32_t direction = (7 - (int32_t)((uint16_t)dir_angle >> 13)) & 7;
    fifa96_render_frame frame;
    const uint32_t ball_slot =
        (uint32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS);
    if (slot == ball_slot &&
        r->ball_bank != FIFA96_MATCH_RUN_BALL_BANK_NONE &&
        r->ball_bank < r->bank_count) {
      /* FU-158 §6/§8.3 (phase-13 T2): the ball slot's dedicated draw
       * (FUN_000572E8). The native presenter blits the captured GAMEART0
       * `ball.fsh` bank's frame `[0x155C4E] & 0xF` — sanitized to frame 0 when
       * it is >= the bank count (0x5739D) — at the ball's projected screen
       * pair, direction deliberately unfolded (no mirror). The engine draws it
       * at slot 22's FU-85 clean x/y/scale instead of resolving the FU-84
       * row-3 art; the presenter's own scale cell `[0x109000]` is unported
       * (leg L158.5). */
      memset(&frame, 0, sizeof frame);
      int32_t ball_frame = (int32_t)(r->ball_roll & 0x0Fu);
      if ((uint32_t)ball_frame >= r->banks[r->ball_bank].count) ball_frame = 0;
      frame.sprite = fifa96_render_bank_at(&r->banks[r->ball_bank], ball_frame);
    } else {
      err = fifa96_render_resolve(r->entities[slot].stage.anim_id,
                                  r->entities[slot].stage.frame, direction, r->frames,
                                  r->entities[slot].bank_index, r->banks, r->bank_count,
                                  r->fixed60, r->fixed61, r->mirror, &frame);
      if (err != FIFA96_OK) return (int)err;
    }
    if (!frame.sprite || (uintptr_t)frame.sprite < (uintptr_t)r->sprite_data) continue;
    fifa96_sprite_frame sprite;
    err = fifa96_sprite_frame_parse(&sprite_bank,
                                    (uint32_t)(frame.sprite - r->sprite_data), &sprite);
    if (err != FIFA96_OK) return (int)err;

    /* FU-85 §2 sprite size: scale = ((clean_y - jitter_y + 0x1000) & ~0xFFFF)
     *                       * 0x5D1 / 0x10000
     *   (the 0x571FD sequence); each frame header size scales by the signed
     *   factor with the 32x32->16 fixup (FUN_00057080's SHRD), i.e.
     *   `size * scale >> 16` (negative scale = mirrored). */
    uint32_t delta = (uint32_t)slots[slot].clean.y - (uint32_t)slots[slot].jitter.y;
    uint32_t size_base = (delta + 0x1000u) & 0xFFFF0000u;
    int32_t scale = (int32_t)(((int64_t)(int32_t)size_base * 0x5D1) >> 16);
    int32_t x = (int32_t)((int64_t)slots[slot].clean.x >> 16);
    int32_t y = (int32_t)((int64_t)slots[slot].clean.y >> 16);
    /* The native ball presenter FUN_000572E8 blits the 16x16 `ball.fsh` frame
     * 1:1 (FUN_00057278 sets the pivot to width/2, height; there is no
     * depth-scale term), unlike the entity walk's FU-85 depth scale. Using
     * the slot scale made the ball balloon as it approached the camera. Live
     * only: the M1/M2 tapes keep the recorded entity scale. */
    int32_t draw_scale = scale;
    if (slot == ball_slot && mr->live_start) {
      draw_scale = 0x10000;
      sprite.pivot_x = (uint16_t)(sprite.width >> 1);
      sprite.pivot_y = sprite.height;
    }
    /* FU-343 (P1): the live per-team kit translation (A7 §4). Only the 22
     * player slots select it; the ball (slot 22) and every HUD/overlay pass
     * keep the current `remap`. Non-live keeps `remap` for every slot. The
     * live ball selects its own identity map (index 0 is its opaque outline,
     * not the player sprites' colour key). */
    const uint8_t *sprite_remap = r->remap;
    if (slot == ball_slot && mr->live_start)
      sprite_remap = r->ball_remap;
    else if (mr->live_start && r->team_remap_ready &&
             slot < (uint32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
      sprite_remap = r->remap_entity[slot / FIFA96_MATCH_ENTITY_RECORDS]
                                    [slot % FIFA96_MATCH_ENTITY_RECORDS];
    /* The native projection already sizes players for the sideline rig; the
     * P10 2.5x live upscale was only compensation for the removed behind-goal
     * camera. Keep the native scale for every slot. */
    int rc = match_run_draw_sprite(s, &sprite, sprite_remap, x, y, draw_scale,
                                   frame.mirrored, &clip);
    if (rc != 0) return rc;
    /* FU-85 §2 second pass: composite animator classes resolve an overlay
     * frame (out2) and the original blits it with a second FUN_00057080 call
     * (0x57241) using the same scale/position. Draw it over the main frame;
     * an unparseable overlay is treated as absent, like the blitter's NULL
     * return. */
    if (frame.overlay && (uintptr_t)frame.overlay >= (uintptr_t)r->sprite_data) {
      fifa96_sprite_frame overlay;
      if (fifa96_sprite_frame_parse(&sprite_bank,
                                    (uint32_t)(frame.overlay - r->sprite_data),
                                    &overlay) == FIFA96_OK) {
        rc = match_run_draw_sprite(s, &overlay, sprite_remap, x, y, draw_scale,
                                   frame.mirrored, &clip);
        if (rc != 0) return rc;
      }
    }
  }
  /* OL-T11-7: the overlay presenter runs after the scene (the native
   * FUN_000495B0 -> FUN_00049830 -> FUN_000565BC order). FU-152 §3.1 rows:
   * R1 sub strip -> R2 replay -> R3 overlay -> R4 HUD -> R5 ball row. */
  match_run_draw_sub_strip(mr, s);
  match_run_draw_replay(mr, s);
  match_run_draw_overlay(mr, s);
  match_run_draw_hud(mr, s);
  match_run_draw_ball_row(mr, s);
  /* FU-343 (P4): the live pause / FULL TIME stand-in panels. Non-live runs
   * never set `paused` or hold POST, so the tape's frames stay byte-identical. */
  if (mr->live_start) {
    if (mr->paused) {
      if (mr->sub_menu_open)
        match_run_draw_sub_menu(mr, s);
      else
        match_run_draw_pause(mr, s);
    } else if (mr->lc.screen == FIFA96_MATCH_SCREEN_OVER ||
             mr->lc.screen == FIFA96_MATCH_SCREEN_POST)
      match_run_draw_result(mr, s);
    else if (mr->lc.screen == FIFA96_MATCH_SCREEN_ACTIVE &&
             mr->live_halftime_pending &&
             (mr->state.phase == 0x0Cu || mr->state.phase == 1u))
      match_run_draw_halftime(mr, s);
    else if (mr->lc.screen == FIFA96_MATCH_SCREEN_ACTIVE &&
             mr->live_sp_active)
      match_run_draw_setpiece(mr, s);
  }
  return 0;
}

/* OL-T11-6 (M2 playable-match Task 1): the native match palette. See the
 * header contract; the first-hand chain is in FU-144 and the constants above.
 * `FUN_00048B60`/`FUN_00048ED8` read the pre-remap snapshot and write
 * `pal[0x70F2[i]] = snapshot[0x70E8[i]]` (0x48BA9/0x48BBB, 0x48F57/0x48F69),
 * then copy the PALsys chunk's appended ranges over the result, and
 * `FUN_000479A0` converts 6->8 bit with `v << 2`. */
int fifa96_match_palette_from_bank(const uint8_t *bank_data, size_t bank_len,
                                   const uint8_t *base6, uint8_t rgb8[768]) {
  fifa96_sprite_bank bank;
  fifa96_sprite_entry entry;
  fifa96_sprite_chunk chunk;
  const uint8_t *rgb6 = NULL;
  uint16_t count = 0;
  uint8_t pal6[768];
  fifa96_err_t err;
  if (!bank_data || !rgb8) return -FIFA96_ERR_INVALID;
  err = fifa96_sprite_bank_parse(bank_data, bank_len, &bank);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_sprite_bank_entry(&bank, MATCH_RUN_PALETTE_FRAME, &entry);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_sprite_chunk_parse(&bank, entry.offset, &chunk);
  if (err != FIFA96_OK) return (int)err;
  err = fifa96_sprite_chunk_palette(&chunk, &rgb6, &count);
  if (err != FIFA96_OK) return (int)err;
  /* The native copies 0x300 bytes from the chunk unconditionally; the port
   * requires the full 256-entry form the retail PALsys.fsh frame 2 carries. */
  if (count != MATCH_RUN_PALETTE_COUNT) return -FIFA96_ERR_UNSUPPORTED;
  if (base6) {
    memcpy(pal6, base6, sizeof pal6);
  } else {
    /* Derived default: the native base buffer 0x14B200 is the previously
     * installed front-end palette and is not statically derivable; using the
     * chunk itself makes the native appends identity (recorded leg). */
    memcpy(pal6, rgb6, sizeof pal6);
  }
  (void)fifa96_sprite_palette_kit_remap(pal6, count);
  memcpy(pal6 + MATCH_RUN_PALETTE_APPEND_A_OFF,
         rgb6 + MATCH_RUN_PALETTE_APPEND_A_OFF, MATCH_RUN_PALETTE_APPEND_A_LEN);
  memcpy(pal6 + MATCH_RUN_PALETTE_APPEND_B_OFF,
         rgb6 + MATCH_RUN_PALETTE_APPEND_B_OFF, MATCH_RUN_PALETTE_APPEND_B_LEN);
  for (size_t i = 0; i < sizeof pal6; i++) rgb8[i] = (uint8_t)(pal6[i] << 2);
  return FIFA96_OK;
}

int fifa96_match_run_palette_install(struct fifa96_match_run *mr,
                                     struct fifa96_surface *s) {
  if (!mr || !s) return -FIFA96_ERR_INVALID;
  if (!mr->render.palette_ready) return -FIFA96_ERR_STATE;
  fifa96_surface_set_palette8(s, mr->render.palette);
  return 0;
}

/* FU-148 §4.2 (S4): the per-entity translation install
 * (FUN_00048DC0 -> FUN_000CE980 -> 0x114720). The native kit path fires when
 * `[0x1068E0]==1` (set by the match load FUN_00048ED8, cleared by the restore
 * FUN_00048FF4) and the entity is 0/0xB; every other entity (or an inactive
 * kit) installs its pool slot verbatim. The engine's analog for the load flag
 * is `render.palette_ready`; the pool is the caller-staged partition. */
int fifa96_match_run_translation_install(struct fifa96_match_run *mr, uint32_t entity) {
  const uint8_t *slot;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (entity >= 23u) return -FIFA96_ERR_INVALID;   /* the native indexes blindly */
  if (!mr->render.palette_pool.base) return -FIFA96_ERR_STATE;  /* pool identity leg 11 */
  slot = mr->render.palette_pool.slots23[entity];
  if (mr->render.palette_ready && (entity == 0u || entity == 0xBu))
    return (int)fifa96_palette_translate_kit(slot, mr->render.remap, entity);
  return (int)fifa96_palette_translate_slot(mr->render.remap, slot);
}

/* Task 2 asset staging (FU-84/85/86). The decode chain mirrors the game's
 * resource path: a container record (`[selector 0xFB BE24 size payload]`) is
 * decoded once, then the payload is a BIGF v2 directory (FU-41/FU-86 §2) whose
 * entries are raw SHPI .fsh slices or nested record chains that decode to SHPI
 * (FU-86 §3). Stage 1 is fed the container tail, not the bounded record slice:
 * the original huff reader is unbounded and 10 PLAYART .qfs entries need the
 * following bytes (FU-86 §3 caveat / leg 9); every other stage is exact. */
#define MATCH_STAGE_DECODE_STAGES 4
#define MATCH_STAGE_FRAME_SIZE 5
#define MATCH_STAGE_FRAME_DURATION 0x50u   /* FU-84 §4 row 1 (walk) */
/* FU-84's frame index is a byte and fifa96_render_resolve accepts the signed
 * non-negative half (0..0x7F, `fi = (int8_t)frame_index` in fifa96_render.c);
 * the staged table covers the whole resolver domain so no accepted index can
 * read past it. */
#define MATCH_STAGE_FRAMES_MAX 128u

/* One BIGF entry after decode: an owned SHPI image, or the unloaded slot the
 * renderer skips (FU-85 §1.2: a NULL bank handle). */
struct match_stage_item {
  uint8_t *data;
  uint32_t len;
  uint32_t count;
};

/* OL-T11-7: the HUD entries the loader table resolves in the GAMEART0
 * container (FU-148 §1.5): resource slot 0x35 "clockfnt.fsh" (full window),
 * 0x36 "playfnt.fsh" (zoomed) and slot 0x2F "Frames.fsh" (the bar bank
 * FUN_00053930 stores frames from at 0x14E624). */
#define MATCH_RUN_HUD_FONT_BANK "clockfnt.fsh"
#define MATCH_RUN_HUD_FONT_ZOOM_BANK "playfnt.fsh"
#define MATCH_RUN_HUD_BAR_BANK "Frames.fsh"
#define MATCH_RUN_HUD_BAR_FRAME 13u   /* the drawn frame ([0x14E658]) */
#define MATCH_RUN_HUD_BAR_LAYOUT_FRAME 4u   /* the 0x14E634 height anchor */

struct match_stage_set {
  struct match_stage_item *items;
  uint32_t count;       /* slots = BIGF entry count (indices stay stable) */
  uint32_t loaded;      /* slots with a decoded SHPI bank */
  int32_t palette_index; /* BIGF entry named PALsys.fsh, -1 when absent */
  int32_t frames_index;  /* BIGF entry named Frames.fsh, -1 when absent */
  int32_t clockfont_index; /* BIGF entry named clockfnt.fsh, -1 when absent */
  int32_t playfont_index;  /* BIGF entry named playfnt.fsh, -1 when absent */
  int32_t ball_index;      /* BIGF entry named ball.fsh, -1 when absent (T2) */
  int32_t net_index;       /* BIGF entry named Net.fsh, -1 when absent (P12) */
  int32_t post_index;      /* BIGF entry named Npost.fsh, -1 when absent (P12) */
};

/* The native by-name lookup (FUN_000A81A5) lowercases ASCII, so the staged
 * bank name matches are case-insensitive like the original's. */
static int match_run_name_eq(const char *a, const char *b) {
  while (*a && *b) {
    unsigned char ca = (unsigned char)*a;
    unsigned char cb = (unsigned char)*b;
    if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca + 0x20);
    if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb + 0x20);
    if (ca != cb) return 0;
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

static void match_stage_set_free(struct match_stage_set *set) {
  if (set->items) {
    for (uint32_t i = 0; i < set->count; i++) free(set->items[i].data);
    free(set->items);
  }
  memset(set, 0, sizeof *set);
}

/* Decode a whole bank container to its BIGF v2 bytes: a raw BIGF file or the
 * refpack record form both FU-86 containers use. Returns 0 with a
 * caller-freeable buffer, -1 when the port cannot decode it. */
static int match_stage_container(const uint8_t *file, size_t file_len, uint8_t **out,
                                 size_t *out_len) {
  *out = NULL;
  *out_len = 0;
  if (file_len >= 4 && memcmp(file, "BIGF", 4) == 0) {
    uint8_t *copy = malloc(file_len);
    if (!copy) return -1;
    memcpy(copy, file, file_len);
    *out = copy;
    *out_len = file_len;
    return 0;
  }
  if (file_len >= 5 && file[1] == 0xFB) {
    size_t declared = ((size_t)file[2] << 16) | ((size_t)file[3] << 8) | (size_t)file[4];
    if (declared == 0) return -1;
    uint8_t *buf = malloc(declared);
    if (!buf) return -1;
    size_t n = 0;
    if (fifa96_record_decode(file, file_len, buf, declared, &n) != 0) {
      free(buf);
      return -1;
    }
    *out = buf;
    *out_len = n;
    return 0;
  }
  return -1;
}

/* FU-199 §5.1 / §7.2 + FU-200 §1/§2 (M2 phase-18 T1): the OL-13 metadata bind.
 *
 * The native match begin `FUN_0007412C -> FUN_0008CE78 -> FUN_0006D9C4 ->
 * FUN_0006D920` stages each record's `[rec+0x8/+0xC/+0x10/+0x14]` from the
 * formation id's `.fmt` family: `FUN_0004C384(form*6 + {0,1,2,4})`, the loaded
 * GAMEART0 resource pointers. `FUN_0004A6BC` fills `DAT_0014BFC0` by
 * `sprintf("%s.fmt", table[k])` over the 0x107370 loader name table (fresh
 * `read_memory 0x107370`/`0x101A30` this slice: every 6-name group is
 * {ko,pl,pk,ps,freekick,sp}, and slot 4 is the shared `0x101A50` "freekick").
 * So the writer reads the ko/pl/pk/freekick files; only the three non-ko blobs
 * feed the pooled `phase_meta`/`phase_meta10`/`phase_meta14` fields (the ko
 * file is the by-value formation seed's). */
#define MATCH_RUN_META_PL_STRIDE 0x256u   /* 598 B/record -> [rec+0xC] */
#define MATCH_RUN_META_PK_STRIDE 8u       /* 2 dwords/record -> [rec+0x10] */
#define MATCH_RUN_META_FK_STRIDE 0x958u   /* 2392 B/record -> [rec+0x14] */
#define MATCH_RUN_META_PL_BYTES \
  (FIFA96_MATCH_ENTITY_RECORDS * MATCH_RUN_META_PL_STRIDE)
/* The pk segment is dword-addressed (phase_meta10 is `const int32_t *`), so
 * pad the byte segment to a 4-byte boundary (11*0x256 = 6578 -> 6580). */
#define MATCH_RUN_META_PL_PAD ((MATCH_RUN_META_PL_BYTES + 3u) & ~3u)
#define MATCH_RUN_META_PK_BYTES \
  (FIFA96_MATCH_ENTITY_RECORDS * MATCH_RUN_META_PK_STRIDE)
#define MATCH_RUN_META_FK_BYTES \
  (FIFA96_MATCH_ENTITY_RECORDS * MATCH_RUN_META_FK_STRIDE)
#define MATCH_RUN_META_SIDE_BYTES \
  (MATCH_RUN_META_PL_PAD + MATCH_RUN_META_PK_BYTES + MATCH_RUN_META_FK_BYTES)

/* The 0x107370 `.fmt` family names (slots 1, 2, 4) per formation id. */
static const char *const match_formation_family[5][3] = {
    {"352pl.fmt", "352pk.fmt", "freekick.fmt"},
    {"442pl.fmt", "442pk.fmt", "freekick.fmt"},
    {"swpl.fmt", "swpk.fmt", "freekick.fmt"},
    {"424pl.fmt", "424pk.fmt", "freekick.fmt"},
    {"433pl.fmt", "433pk.fmt", "freekick.fmt"},
};

/* Copy the leading `need` bytes of the named BIGF entry into `dst`. Returns 0 on
 * success, -1 when the entry is absent or shorter than `need`. */
static int match_run_meta_copy(const struct fifa96_bigf_info *info, const char *name,
                               uint8_t *dst, size_t need) {
  for (size_t i = 0; i < info->count; i++) {
    uint32_t off = 0;
    uint32_t size = 0;
    const char *entry = NULL;
    if (fifa96_bigf_record(info, i, &off, &size, &entry) != FIFA96_OK) return -1;
    if (entry && strcmp(entry, name) == 0) {
      if (size < need) return -1;
      memcpy(dst, info->src + off, need);
      return 0;
    }
  }
  return -1;
}

int fifa96_match_run_record_meta_bind(struct fifa96_match_run *mr) {
  uint8_t *bytes = NULL;
  size_t len = 0;
  uint8_t *container = NULL;
  size_t container_len = 0;
  uint8_t *owner = NULL;
  int bound = 0;
  if (!mr) return -FIFA96_ERR_INVALID;
  match_run_release_meta(mr);   /* clear the pool + free any prior blobs */
  if (!mr->engine || !mr->engine->assets) return 0;
  if (fifa96_asset_read(mr->engine->assets, MATCH_RUN_FORMATION_BANK, &bytes, &len) !=
      FIFA96_OK)
    return 0;
  if (match_stage_container(bytes, len, &container, &container_len) != 0) goto out;
  struct fifa96_bigf_info info;
  if (fifa96_bigf_parse(container, container_len, &info) != FIFA96_OK) goto out;
  owner = calloc(FIFA96_MATCH_ENTITY_TEAMS, MATCH_RUN_META_SIDE_BYTES);
  if (!owner) goto out;
  /* Per-field, per-side: copy each side's `.fmt` family independently and stage
   * whichever blobs loaded. A missing/short entry (or an out-of-range id)
   * clears only that field's pointer (the `fifa96_match_entities_bind_record_meta`
   * NULL-blob gate), never a sibling field or the other side — so each row's
   * `phase_meta*` gate is exactly its own `.fmt` blob (the FU-205 truth-table),
   * not a whole-family all-or-nothing. The owner is handed to the run (kept
   * alive) as soon as any field binds, and freed only when nothing bound (the
   * pool was already cleared above), so no pointer ever dangles into freed
   * memory. */
  for (uint32_t side = 0; side < FIFA96_MATCH_ENTITY_TEAMS; side++) {
    uint8_t *pl = owner + (size_t)side * MATCH_RUN_META_SIDE_BYTES;
    uint8_t *pk = pl + MATCH_RUN_META_PL_PAD;
    uint8_t *fk = pk + MATCH_RUN_META_PK_BYTES;
    const int8_t *plp = NULL;
    const int32_t *pkp = NULL;
    const int32_t *fkp = NULL;
    uint8_t id = mr->formation[side];   /* native (int8)byte[[team+0x7AE]] */
    if (id <= 4u) {                     /* hardened (native has no range check) */
      const char *const *family = match_formation_family[id];
      if (match_run_meta_copy(&info, family[0], pl, MATCH_RUN_META_PL_BYTES) == 0)
        plp = (const int8_t *)pl;
      if (match_run_meta_copy(&info, family[1], pk, MATCH_RUN_META_PK_BYTES) == 0)
        pkp = (const int32_t *)pk;
      if (match_run_meta_copy(&info, family[2], fk, MATCH_RUN_META_FK_BYTES) == 0)
        fkp = (const int32_t *)fk;
    }
    (void)fifa96_match_entities_bind_record_meta(&mr->entities, side, plp, pkp, fkp);
    if (plp || pkp || fkp) bound = 1;
  }
  if (bound) {
    mr->meta_owner = owner;   /* hand the blobs to the run */
    owner = NULL;
  }
out:
  free(owner);
  free(container);
  fifa96_asset_free(bytes);
  return bound;
}

/* Decode one BIGF entry to a SHPI image. `entry_len` is the entry's bounded
 * BIGF record size (used for the raw SHPI form, whose declared total equals it,
 * FU-86 §4) and `raw_len` the container tail from the entry's offset (the
 * stage-1 over-read allowance noted above). Returns 1 with an owned copy and
 * its frame count, 0 when the entry is not a sprite bank (fonts, .dat tables),
 * -1 when it is SHPI/record shaped but the port cannot decode it. */
static int match_stage_entry(const uint8_t *raw, size_t entry_len, size_t raw_len,
                             uint8_t **out, uint32_t *out_len, uint32_t *out_count) {
  *out = NULL;
  *out_len = 0;
  *out_count = 0;
  if (raw_len >= 4 && memcmp(raw, "SHPI", 4) == 0) {
    fifa96_sprite_bank bank;
    if (fifa96_sprite_bank_parse(raw, entry_len, &bank) != FIFA96_OK) return -1;
    uint8_t *copy = malloc(bank.total_size);
    if (!copy) return -1;
    memcpy(copy, raw, bank.total_size);
    *out = copy;
    *out_len = bank.total_size;
    *out_count = bank.count;
    return 1;
  }
  if (raw_len < 5 || raw[1] != 0xFB) return 0;
  const uint8_t *src = raw;
  size_t src_len = raw_len;
  uint8_t *cur = NULL;
  for (int stage = 0; stage < MATCH_STAGE_DECODE_STAGES; stage++) {
    if (src_len >= 4 && memcmp(src, "SHPI", 4) == 0) {
      fifa96_sprite_bank bank;
      if (fifa96_sprite_bank_parse(src, src_len, &bank) != FIFA96_OK) {
        free(cur);
        return -1;
      }
      uint8_t *copy = malloc(bank.total_size);
      if (!copy) {
        free(cur);
        return -1;
      }
      memcpy(copy, src, bank.total_size);
      free(cur);
      *out = copy;
      *out_len = bank.total_size;
      *out_count = bank.count;
      return 1;
    }
    if (src_len < 5 || src[1] != 0xFB) {
      free(cur);
      return -1;
    }
    size_t declared = ((size_t)src[2] << 16) | ((size_t)src[3] << 8) | (size_t)src[4];
    if (declared == 0) {
      free(cur);
      return -1;
    }
    uint8_t *dst = malloc(declared);
    if (!dst) {
      free(cur);
      return -1;
    }
    size_t n = 0;
    if (fifa96_record_decode(src, src_len, dst, declared, &n) != 0) {
      free(dst);
      free(cur);
      return -1;
    }
    free(cur);
    cur = dst;
    src = dst;
    src_len = n;
  }
  free(cur);
  return -1;
}

/* Decode every BIGF entry of one container into a slot-stable set: slot i is
 * container entry i, so the player container's indices are exactly the FU-84
 * row +8 bank indices (observed 0..89). Entries the port cannot decode stay
 * unloaded slots, the renderer's skippable state (FU-85 §1.2). */
static int match_stage_set_build(struct match_stage_set *set, const uint8_t *file,
                                 size_t file_len) {
  memset(set, 0, sizeof *set);
  set->palette_index = -1;
  set->frames_index = -1;
  set->clockfont_index = -1;
  set->playfont_index = -1;
  set->ball_index = -1;
  set->net_index = -1;
  set->post_index = -1;
  uint8_t *container = NULL;
  size_t container_len = 0;
  if (match_stage_container(file, file_len, &container, &container_len) != 0) return -1;
  struct fifa96_bigf_info info;
  if (fifa96_bigf_parse(container, container_len, &info) != FIFA96_OK) goto fail;
  if (info.count == 0) goto fail;
  set->items = calloc(info.count, sizeof *set->items);
  if (!set->items) goto fail;
  set->count = (uint32_t)info.count;
  for (size_t i = 0; i < info.count; i++) {
    uint32_t off = 0;
    uint32_t size = 0;
    const char *name = NULL;
    if (fifa96_bigf_record(&info, i, &off, &size, &name) != FIFA96_OK) goto fail;
    if (name && match_run_name_eq(name, MATCH_RUN_PALETTE_BANK))
      set->palette_index = (int32_t)i;
    if (name && match_run_name_eq(name, MATCH_RUN_HUD_BAR_BANK))
      set->frames_index = (int32_t)i;
    if (name && match_run_name_eq(name, MATCH_RUN_HUD_FONT_BANK))
      set->clockfont_index = (int32_t)i;
    if (name && match_run_name_eq(name, MATCH_RUN_HUD_FONT_ZOOM_BANK))
      set->playfont_index = (int32_t)i;
    if (name && match_run_name_eq(name, MATCH_RUN_BALL_BANK))
      set->ball_index = (int32_t)i;
    if (name && match_run_name_eq(name, MATCH_RUN_NET_BANK))
      set->net_index = (int32_t)i;
    if (name && match_run_name_eq(name, MATCH_RUN_POST_BANK))
      set->post_index = (int32_t)i;
    uint8_t *bank = NULL;
    uint32_t bank_len = 0;
    uint32_t bank_count = 0;
    int got = match_stage_entry(container + off, size, container_len - off, &bank,
                                &bank_len, &bank_count);
    if (got == 1) {
      set->items[i].data = bank;
      set->items[i].len = bank_len;
      set->items[i].count = bank_count;
      set->loaded++;
    } else if (got < 0) {
      set->items[i].data = NULL;   /* shaped like a bank but undecodable */
    } else if ((int32_t)i == set->clockfont_index || (int32_t)i == set->playfont_index) {
      /* OL-T11-7: the FNTI font images are not SHPI banks, so the decode
       * refuses them (FU-148 §1.5); keep the raw entry bytes for the HUD
       * font parse. */
      uint8_t *copy = malloc(size);
      if (!copy) goto fail;
      memcpy(copy, container + off, size);
      set->items[i].data = copy;
      set->items[i].len = size;
      set->items[i].count = 0;
    }
  }
  free(container);
  return 0;
fail:
  free(container);
  match_stage_set_free(set);
  return -1;
}

/* Round an arena cursor up to `align`. SHPI copies need 4 for the int32 offset
 * table at SHPI+0x14 (FU-86 §4: directory entries {name[4], u32 offset}); the
 * bank record array needs its natural alignment for UBSan-clean access. */
static size_t match_stage_pad(size_t n, size_t align) {
  return (n + align - 1u) & ~(align - 1u);
}

int fifa96_match_run_stage(struct fifa96_match_run *mr, const struct fifa96_surface *s,
                           const char *player_bank, const char *pitch_bank) {
  struct match_stage_set player;
  struct match_stage_set pitch;
  struct fifa96_cache *cache;
  const uint8_t *file;
  size_t file_len;
  uint32_t lba = 0;
  uint32_t size = 0;
  size_t frames_len;
  size_t banks_off;
  size_t data_off;
  size_t data_len = 0;
  size_t arena_len;
  uint8_t *arena = NULL;
  struct fifa96_render_bank *banks;
  uint8_t *frames;
  uint32_t total;
  uint8_t palette[768];
  int palette_ready = 0;
  /* OL-T11-7: built locally and committed with the arena swap only on
   * success; the staged names persist (they are not container assets). */
  struct fifa96_font hud_font[2];
  uint8_t hud_font_ready[2] = {0, 0};
  struct fifa96_sprite_frame hud_bar;
  uint16_t hud_bar_height = 0;
  uint8_t hud_bar_ready = 0;
  struct fifa96_sprite_frame goal_net_frame;
  uint8_t goal_net_ready = 0;
  int rc;

  memset(hud_font, 0, sizeof hud_font);
  memset(&goal_net_frame, 0, sizeof goal_net_frame);
  memset(&hud_bar, 0, sizeof hud_bar);

  if (!mr || !s || !player_bank || !pitch_bank) return -FIFA96_ERR_INVALID;
  /* A live run guarantees the engine (and its asset table) outlives the arena
   * the run takes ownership of; staging after end would otherwise strand the
   * arena (end releases once) or reach a destroyed engine. */
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (!mr->engine || !mr->engine->assets) return -FIFA96_ERR_STATE;

  memset(&player, 0, sizeof player);
  memset(&pitch, 0, sizeof pitch);
  cache = fifa96_cache_create(mr->engine->assets);
  if (!cache) return -FIFA96_ERR_UNSUPPORTED;

  /* Player animation container (FU-86 §1: PLAYART, 91 banks). */
  if (fifa96_asset_lookup(mr->engine->assets, player_bank, &lba, &size) != FIFA96_OK) {
    rc = -FIFA96_ERR_NOT_FOUND;
    goto out_cache;
  }
  file = fifa96_cache_get(cache, player_bank, &file_len);
  if (!file || file_len == 0 || match_stage_set_build(&player, file, file_len) != 0 ||
      player.loaded == 0) {
    rc = -FIFA96_ERR_UNSUPPORTED;
    goto out_sets;
  }

  /* Second (pitch/match art) container (FU-86 §1: GAMEART0; identity open). */
  if (fifa96_asset_lookup(mr->engine->assets, pitch_bank, &lba, &size) != FIFA96_OK) {
    rc = -FIFA96_ERR_NOT_FOUND;
    goto out_sets;
  }
  file = fifa96_cache_get(cache, pitch_bank, &file_len);
  if (!file || file_len == 0 || match_stage_set_build(&pitch, file, file_len) != 0 ||
      pitch.loaded == 0) {
    rc = -FIFA96_ERR_UNSUPPORTED;
    goto out_sets;
  }

  /* One arena owns the frame table, the bank records (bases/offset tables
   * point into the SHPI copies) and the SHPI images. Nothing touches mr until
   * the whole build succeeds. */
  total = player.count + pitch.count;
  const size_t bank_align = _Alignof(struct fifa96_render_bank);
  frames_len = (size_t)MATCH_STAGE_FRAMES_MAX * MATCH_STAGE_FRAME_SIZE;
  banks_off = match_stage_pad(frames_len, bank_align);
  data_off = match_stage_pad(banks_off + (size_t)total * sizeof *banks, 4u);
  for (uint32_t i = 0; i < player.count; i++)
    if (player.items[i].data) data_len += match_stage_pad(player.items[i].len, 4u);
  for (uint32_t i = 0; i < pitch.count; i++)
    if (pitch.items[i].data) data_len += match_stage_pad(pitch.items[i].len, 4u);
  if (data_off + data_len > 0xFFFFFFFFu) {
    rc = -FIFA96_ERR_UNSUPPORTED;   /* sprite_data_len is 32-bit */
    goto out_sets;
  }
  arena_len = data_off + data_len;
  arena = calloc(1, arena_len);
  if (!arena) {
    rc = -FIFA96_ERR_UNSUPPORTED;
    goto out_sets;
  }
  frames = arena;
  banks = (struct fifa96_render_bank *)(void *)(arena + banks_off);

  /* FU-84 §4 frame records: the row-1 walk evidence stores sprite = frame
   * index with duration 0x50, so staging derives that identity table across
   * the resolver's whole 128-index domain; the real per-row tables are
   * executable object-4 data (0x10E200..0x10EF00), not ISO assets (open leg). */
  for (uint32_t i = 0; i < MATCH_STAGE_FRAMES_MAX; i++) {
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 0] = (uint8_t)MATCH_STAGE_FRAME_DURATION;
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 1] = 0;
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 2] = 0;
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 3] = 0;
    frames[(size_t)i * MATCH_STAGE_FRAME_SIZE + 4] = (uint8_t)i;
  }

  size_t cursor = data_off;
  uint32_t slot = 0;
  uint32_t base_slot = 0;
  const struct match_stage_set *sets[2] = { &player, &pitch };
  for (int which = 0; which < 2; which++) {
    const struct match_stage_set *set = sets[which];
    for (uint32_t i = 0; i < set->count; i++, slot++) {
      struct fifa96_render_bank *b = &banks[slot];
      if (!set->items[i].data) continue;   /* unloaded slot stays zeroed */
      uint8_t *copy = arena + cursor;
      memcpy(copy, set->items[i].data, set->items[i].len);
      /* OL-T11-6: extract the derived match palette from the pitch container's
       * PALsys.fsh bank while the arena copy is at hand. */
      if (which == 1 && (int32_t)i == pitch.palette_index &&
          fifa96_match_palette_from_bank(copy, set->items[i].len, NULL, palette) ==
              FIFA96_OK)
        palette_ready = 1;
      /* OL-T11-7: parse the HUD assets from the pitch container's arena
       * copies — the FNTI fonts (FUN_000984B4) and the Frames.fsh bar frames
       * (frame 13 drawn, frame 4's height for the layout anchor). */
      if (which == 1) {
        if ((int32_t)i == pitch.clockfont_index) {
          if (fifa96_font_parse(copy, set->items[i].len, &hud_font[0]) == FIFA96_OK)
            hud_font_ready[0] = 1;
        } else if ((int32_t)i == pitch.playfont_index) {
          if (fifa96_font_parse(copy, set->items[i].len, &hud_font[1]) == FIFA96_OK)
            hud_font_ready[1] = 1;
        } else if ((int32_t)i == pitch.frames_index) {
          fifa96_sprite_bank bar_bank;
          fifa96_sprite_entry bar_entry;
          if (fifa96_sprite_bank_parse(copy, set->items[i].len, &bar_bank) == FIFA96_OK) {
            int bar_ok = fifa96_sprite_bank_entry(&bar_bank, MATCH_RUN_HUD_BAR_FRAME,
                                                  &bar_entry) == FIFA96_OK &&
                         fifa96_sprite_frame_parse(&bar_bank, bar_entry.offset, &hud_bar) ==
                             FIFA96_OK;
            int layout_ok =
                fifa96_sprite_bank_entry(&bar_bank, MATCH_RUN_HUD_BAR_LAYOUT_FRAME,
                                         &bar_entry) == FIFA96_OK;
            if (layout_ok) {
              fifa96_sprite_frame layout_frame;
              layout_ok = fifa96_sprite_frame_parse(&bar_bank, bar_entry.offset,
                                                    &layout_frame) == FIFA96_OK;
              if (layout_ok) hud_bar_height = layout_frame.height;
            }
            /* Both the drawn frame and the layout anchor are required (the
             * native pointers are always both set before the HUD can run). */
            hud_bar_ready = (uint8_t)(bar_ok && layout_ok);
          }
        } else if ((int32_t)i == pitch.net_index) {
          /* P12: the GAMEART0 Net.fsh 64x64 mesh frame (the retail bank has
           * five frames; frame 4 is the full mesh). The derived goal plane
           * samples this indexed frame with index 0 transparent. */
          fifa96_sprite_bank net_bank;
          fifa96_sprite_entry net_entry;
          if (fifa96_sprite_bank_parse(copy, set->items[i].len, &net_bank) ==
              FIFA96_OK) {
            uint32_t frame = net_bank.count > 4u ? 4u : 0u;
            if (fifa96_sprite_bank_entry(&net_bank, frame, &net_entry) == FIFA96_OK &&
                fifa96_sprite_frame_parse(&net_bank, net_entry.offset,
                                          &goal_net_frame) == FIFA96_OK)
              goal_net_ready = 1;
          }
        }
      }
      b->base = copy;
      b->offsets = (const int32_t *)(const void *)(copy + 0x14);
      b->count = set->items[i].count;
      /* The FU-86 §4.1 stride switch indexes the animator-record array of the
       * container the bank came from (the player container is the derived
       * 0x57DE8 array; indices 0..90). Appended containers have no derived
       * animator identity (open leg), so their banks use a container-local
       * index instead of leaking into the player switch's classes (the old
       * global slot 0x5B hit /2 by accident). */
      b->step = fifa96_sprite_stride(slot - base_slot, set->items[i].count);
      cursor += match_stage_pad(set->items[i].len, 4u);
    }
    base_slot += set->count;
  }

  free(mr->stage_owner);
  mr->stage_owner = arena;
  struct fifa96_match_run_render *r = &mr->render;
  r->frames = frames;
  r->banks = banks;
  r->bank_count = total;
  /* FU-158 §8.1 (phase-13 T2): capture the GAMEART0 `ball.fsh` bank (the
   * dedicated draw's source) at the player-container offset; a pitch container
   * without the entry, an undecoded slot (`data == NULL`, the zeroed bank) or
   * an index past the byte bank domain keeps the 0xFF "absent" sentinel and
   * the FU-84 resolve (the render guard keeps the 0xFF-slot sentinel
   * collision benign). */
  r->ball_bank = FIFA96_MATCH_RUN_BALL_BANK_NONE;
  if (pitch.ball_index >= 0 && pitch.items[pitch.ball_index].data != NULL) {
    uint32_t ball_slot = player.count + (uint32_t)pitch.ball_index;
    if (ball_slot <= 0xFFu) r->ball_bank = (uint8_t)ball_slot;
  }
  r->ball_roll = 0;
  r->fixed60 = NULL;   /* runtime-populated records, not ISO assets (FU-85 leg 8) */
  r->fixed61 = NULL;
  r->mirror = NULL;    /* 0x10F2E7 is executable data, not on the ISO (FU-85 §1.3) */
  r->sprite_data = arena;
  r->sprite_data_len = (uint32_t)arena_len;
  r->palette_ready = palette_ready;
  if (palette_ready) memcpy(r->palette, palette, sizeof r->palette);
  r->hud_font[0] = hud_font[0];
  r->hud_font[1] = hud_font[1];
  r->hud_font_ready[0] = hud_font_ready[0];
  r->hud_font_ready[1] = hud_font_ready[1];
  r->hud_bar = hud_bar;
  r->hud_bar_height = hud_bar_height;
  r->hud_bar_ready = hud_bar_ready;
  r->goal_net_frame = goal_net_frame;
  r->goal_net_ready = goal_net_ready;
  /* FU-343 (P4): derive the live HUD colours from the palette this stage
   * captured; stage_pitch refreshes them again after the final live palette. */
  match_run_hud_colors_refresh(mr);
  (void)fifa96_window_init(&r->window, s->width, s->height);
  (void)fifa96_window_define_full(&r->window, s->width, s->height);
  r->enabled = 1;
  arena = NULL;
  rc = 0;

out_sets:
  match_stage_set_free(&player);
  match_stage_set_free(&pitch);
out_cache:
  free(arena);
  fifa96_cache_destroy(cache);
  return rc;
}

/* FU-343 (P1) / Phase-86: the live kit palette/remap stage. PALteam.fsh
 * frame 0/2/4 supply the team colour ramps; the live base is PALsys
 * **frame 0**, whose Fplain grass indices 1..29 are green (frame 2 is magenta
 * there). Phase-86 writes the kit ramps into the native low destinations
 * 37/48/59 (home) and 70/81/92 (away) from FUN_00047298's 0x106B70 table and
 * leaves the native skin/hair ramps at 0xDB..0xFD intact. */
#define MATCH_RUN_KIT_BANK "PALteam.fsh"

static int match_run_palette_frame(const uint8_t *bank, size_t bank_len, uint32_t frame,
                                   uint8_t rgb6[768]) {
  fifa96_sprite_bank parsed;
  fifa96_sprite_entry entry;
  fifa96_sprite_chunk chunk;
  const uint8_t *rgb = NULL;
  uint16_t count = 0;
  if (fifa96_sprite_bank_parse(bank, bank_len, &parsed) != FIFA96_OK) return 0;
  if (fifa96_sprite_bank_entry(&parsed, frame, &entry) != FIFA96_OK) return 0;
  if (fifa96_sprite_chunk_parse(&parsed, entry.offset, &chunk) != FIFA96_OK) return 0;
  if (fifa96_sprite_chunk_palette(&chunk, &rgb, &count) != FIFA96_OK) return 0;
  if (!rgb || count < MATCH_RUN_PALETTE_COUNT) return 0;
  memcpy(rgb6, rgb, 768);
  return 1;
}

/* P6: fill one sprite kit band from a PALteam kit colour index. The native
 * FUN_0004808C mapping is frame 2*(idx/16), ramp idx%16, 16 6-bit RGB entries
 * at `ramp*0x30`; the first 11 entries are the real shade ramp (the tail is
 * the shared padding). `dst` is the palette band start, `len` its measured
 * bank-0 size (shirt 7, shorts 9, socks 6). A missing frame leaves the band
 * untouched and returns 0. */
static int match_run_kit_band(uint8_t *palette, int dst, int len, uint8_t index,
                              const uint8_t frames[3][768], const uint8_t ok[3]) {
  uint32_t group = (uint32_t)index / 16u;
  uint32_t ramp = (uint32_t)index % 16u;
  if (group > 2u || !ok[group] || len <= 0) return 0;
  if (len > 11) len = 11;
  const uint8_t *rgb = frames[group] + ramp * 0x30u;
  for (int i = 0; i < len; i++) {
    palette[(dst + i) * 3] = (uint8_t)(rgb[i * 3] << 2);
    palette[(dst + i) * 3 + 1] = (uint8_t)(rgb[i * 3 + 1] << 2);
    palette[(dst + i) * 3 + 2] = (uint8_t)(rgb[i * 3 + 2] << 2);
  }
  return 1;
}

/* Fallback kit band: copy an already-6-bit PALteam frame band from its old
 * engine destination (the pre-P6 208/195/224 layout) into the native low
 * destination used by the new player remap. */
static void match_run_copy_kit_band(uint8_t palette[768], int dst, int len,
                                    const uint8_t src[768], int src_base) {
  for (int i = 0; i < len; i++) {
    palette[(dst + i) * 3] = (uint8_t)(src[(src_base + i) * 3] << 2);
    palette[(dst + i) * 3 + 1] = (uint8_t)(src[(src_base + i) * 3 + 1] << 2);
    palette[(dst + i) * 3 + 2] = (uint8_t)(src[(src_base + i) * 3 + 2] << 2);
  }
}

/* Live player appearance remap (phase-86; source bands phase-85, destinations
 * now the native kit bands).
 *
 * The native per-entity translation pool FUN_00047298 (0x47298) builds one
 * 256-byte table per record from a 0x8E-byte appearance template plus the
 * per-player skin/hair classes. Its kit-band table at 0x106B70 is
 * {destination, length, source-group} x 6: destinations 37, 48, 59, 70, 81,
 * 92 (len 0xB) and 101 (len 9). Those low palette slots are the match kit
 * colours; the native palette leaves the skin/hair ramps at 0xDB..0xFD
 * untouched. The engine previously overlaid PALteam over 0x1E..0xE5 and moved
 * the kit bands to 195..229, which corrupted the skin ramps at 0xE0..0xE5 and
 * forced every player onto one ramp. This builder uses the native low band
 * destinations and composes the player's own skin/hair classes.
 *
 * Source bands (PLAYART `walk.fsh`, phase-85 histogram):
 *   0x50..0x65, 0x8A..0x9D, 0xCC..0xD6   shirt/sleeves/torso -> shirt band
 *   0xC5..0xCB                            shorts             -> shorts band
 *   0x66..0x70, 0x9E..0xA8, 0xE4..0xE5    sock trim/boot     -> sock band
 *   0x71..0x89, 0xDB..0xE3                arms/legs/face     -> skin ramp
 *   0xD7..0xDA                            hair               -> hair ramp
 *   0xC2..0xC4                            collar/trim        -> white 0xFE
 *   0x4E                                  eye                -> black 0x00
 *
 * The native skin table DAT_001070E4 gives four 9-entry ramps whose bases are
 * 0xF6, 0xED, 0xE4, 0xDB (skin classes 0..3). The native hair table
 * DAT_001070D4 gives the four 4-entry ramps below (FUN_0004C00C classes
 * 0..3). The last skin entry would be 0xFE (the engine's white line slot), so
 * the ninth source index wraps to the eighth shade; the sprite face/legs use
 * the first eight entries.
 *
 * Live-only; M1/M2 never call this. */
#define MATCH_RUN_KIT_SHIRT_HOME 37
#define MATCH_RUN_KIT_SHORTS_HOME 48
#define MATCH_RUN_KIT_SOCKS_HOME 59
#define MATCH_RUN_KIT_SHIRT_AWAY 70
#define MATCH_RUN_KIT_SHORTS_AWAY 81
#define MATCH_RUN_KIT_SOCKS_AWAY 92
#define MATCH_RUN_KIT_SHIRT_LEN 7
#define MATCH_RUN_KIT_SHORTS_LEN 9
#define MATCH_RUN_KIT_SOCKS_LEN 6

static const uint8_t match_run_hair_ramps[4][4] = {
  { 0x00, 0x00, 0x00, 0x00 },   /* class 0: black */
  { 0xEB, 0xEC, 0xFC, 0xFD },   /* class 1 */
  { 0xF0, 0xF1, 0xF2, 0xF3 },   /* class 2: brown */
  { 0xD7, 0xD8, 0xD9, 0xDA },   /* class 3: dark brown */
};
static const uint8_t match_run_skin_bases[4] = { 0xF6, 0xED, 0xE4, 0xDB };

/* FUN_0004C00C (0x4C00C): PCCD hair digit -> native class. */
static uint8_t match_run_hair_class(uint8_t code) {
  if (code == '0') return 3;
  if (code == '1') return 2;
  if (code == '4') return 0;
  return 1;
}

/* FUN_0004C098 (0x4C098): PCCD skin digit -> native class. The undecoded
 * digit falls back to the hair class (3 stays 3, everything else 0). */
static uint8_t match_run_skin_class(uint8_t code, uint8_t hair_class) {
  if (code == '0' || code == '2') return 1;
  if (code == '1') return 3;
  if (code == '3') return 2;
  if (code == '4') return 0;
  return hair_class == 3u ? 3u : 0u;
}

static void match_run_remap_band(uint8_t remap[256], int src_lo, int src_hi,
                                 int dst_lo, int dst_len) {
  if (dst_len <= 0) return;
  for (int src = src_lo; src <= src_hi; src++)
    remap[src] = (uint8_t)(dst_lo + ((src - src_lo) % dst_len));
}

static void match_run_apply_player_remap(uint8_t remap[256],
                                         int shirt, int shorts, int socks,
                                         uint8_t skin_class,
                                         uint8_t hair_class) {
  if (skin_class > 3u) skin_class = 0u;
  if (hair_class > 3u) hair_class = 3u;
  /* Shirt: sleeves/shoulders, side panels and torso. */
  match_run_remap_band(remap, 0x50, 0x65, shirt, MATCH_RUN_KIT_SHIRT_LEN);
  match_run_remap_band(remap, 0x8A, 0x9D, shirt, MATCH_RUN_KIT_SHIRT_LEN);
  match_run_remap_band(remap, 0xCC, 0xD6, shirt, MATCH_RUN_KIT_SHIRT_LEN);
  /* Shorts (0xC5..0xCB; 0xC2..0xC4 is the collar). */
  match_run_remap_band(remap, 0xC5, 0xCB, shorts, MATCH_RUN_KIT_SHORTS_LEN);
  /* Sock trim and boot/sock tip -> the selected sock ramp. */
  match_run_remap_band(remap, 0x66, 0x70, socks, MATCH_RUN_KIT_SOCKS_LEN);
  match_run_remap_band(remap, 0x9E, 0xA8, socks, MATCH_RUN_KIT_SOCKS_LEN);
  /* 0xE4/0xE5 are the boot pixels; the native template does not route them
   * through the sock band. Black keeps them neutral without inventing a kit
   * colour the source bank does not carry. */
  remap[0xE4] = 0x00;
  remap[0xE5] = 0x00;
  /* Bare arms/legs/face -> this player's native skin ramp. */
  {
    int base = match_run_skin_bases[skin_class];
    for (int src = 0x71; src <= 0x89; src++)
      remap[src] = (uint8_t)(base + ((src - 0x71) % 8));
    for (int src = 0xDB; src <= 0xE3; src++)
      remap[src] = (uint8_t)(base + ((src - 0xDB) % 8));
  }
  /* Hair: this player's native 4-entry ramp. */
  for (int i = 0; i < 4; i++)
    remap[0xD7 + i] = match_run_hair_ramps[hair_class][i];
  /* Collar/neck trim -> the live white slot; eye -> black. */
  for (int i = 0xC2; i <= 0xC4; i++) remap[i] = FIFA96_PITCH_LINE_INDEX;
  remap[0x4E] = 0x00;
}

/* Phase-86 / FU-348: build one remap per pitch record from the native
 * FUN_00047298 23-block pool, then overlay the six kit source bands from the
 * team table so the shirt/shorts/socks follow the live kit while the
 * skin/hair ramps (0xD7..0xE3) stay native. `remap_team` is still derived by
 * match_run_apply_player_remap (the legacy helpers below); the caller owns the
 * destination so the stage path can stay atomic and the substitution path can
 * rebuild the committed table in place. */
static void match_run_overlay_team_kit(uint8_t dst[256],
                                       const uint8_t team[256]) {
  static const uint8_t bands[][2] = {
      {0x50, 0x65}, {0x8A, 0x9D}, {0xCC, 0xD6},
      {0xC5, 0xCB}, {0x66, 0x70}, {0x9E, 0xA8},
  };
  for (size_t i = 0; i < sizeof bands / sizeof bands[0]; i++)
    memcpy(dst + bands[i][0], team + bands[i][0],
           (size_t)(bands[i][1] - bands[i][0] + 1));
}

static void match_run_build_entity_remaps(
    const struct fifa96_match_run *mr,
    const uint8_t remap_team[2][256],
    uint8_t remap_entity[2][FIFA96_MATCH_ENTITY_RECORDS][256],
    uint8_t remap_extra[256]) {
  _Static_assert(FIFA96_MATCH_ENTITY_RECORDS == 11,
                 "remap builder expects 11 pitch records");
  uint8_t pool[FIFA96_MATCH_REMAP_BLOCKS][FIFA96_MATCH_REMAP_BYTES];
  uint8_t class_a[2][11];
  uint8_t class_b[2][11];
  for (int side = 0; side < 2; side++)
    for (int pitch = 0; pitch < (int)FIFA96_MATCH_ENTITY_RECORDS; pitch++) {
      uint8_t hair_code = '0';   /* the existing default */
      uint8_t skin_code = '4';
      uint8_t roster = mr->sub_lineup[side][pitch];
      if (roster < FIFA96_PCCD_ROSTER_COUNT) {
        hair_code = mr->roster_hair_code[side][roster];
        skin_code = mr->roster_skin_code[side][roster];
      }
      class_a[side][pitch] = match_run_hair_class(hair_code);
      class_b[side][pitch] =
          match_run_skin_class(skin_code, class_a[side][pitch]);
    }
  fifa96_match_remap_build(pool, class_a, class_b, 0, 0);  /* gates: leg */
  for (int side = 0; side < 2; side++)
    for (int pitch = 0; pitch < (int)FIFA96_MATCH_ENTITY_RECORDS; pitch++) {
      uint8_t *dst = remap_entity[side][pitch];
      memcpy(dst, pool[side * FIFA96_MATCH_ENTITY_RECORDS + pitch], 256);
      match_run_overlay_team_kit(dst, remap_team[side]);
      dst[0] = 0xFF;   /* the port's source-0 render key (native is 0x00) */
    }
  memcpy(remap_extra, pool[22], 256);
}

static void match_run_rebuild_entity_remaps(struct fifa96_match_run *mr) {
  if (!mr || !mr->render.team_remap_ready) return;
  match_run_build_entity_remaps(mr, mr->render.remap_team,
                                mr->render.remap_entity,
                                mr->render.remap_extra);
}

static int match_run_stage_live_kit(struct fifa96_match_run *mr,
                                    struct fifa96_engine *eng) {
  uint8_t *bytes = NULL;
  size_t len = 0;
  uint8_t *container = NULL;
  size_t container_len = 0;
  uint8_t kit_frames[3][768];
  uint8_t kit_ok[3] = {0, 0, 0};
  uint8_t sys0[768];
  uint8_t sys2[768];
  /* FU-343 P3 fix: build the live palette and per-team remap into locals and
   * commit them only after the full success path. A pitch stage that succeeds
   * while the kit stage fails (missing PALteam or a bad frame) must not leave a
   * half-installed live palette behind. */
  uint8_t palette[768];
  uint8_t remap_team[2][256];
  uint8_t remap_entity[2][FIFA96_MATCH_ENTITY_RECORDS][256];
  uint8_t remap_extra_local[256];
  int have_team = 0;
  int have_sys = 0;
  struct fifa96_match_run_render *r = &mr->render;
  if (!r->palette_ready) return -FIFA96_ERR_STATE;
  if (fifa96_asset_read(eng->assets, MATCH_RUN_FORMATION_BANK, &bytes, &len) !=
      FIFA96_OK)
    return -FIFA96_ERR_NOT_FOUND;
  if (match_stage_container(bytes, len, &container, &container_len) != 0) {
    fifa96_asset_free(bytes);
    return -FIFA96_ERR_UNSUPPORTED;
  }
  struct fifa96_bigf_info info;
  if (fifa96_bigf_parse(container, container_len, &info) != FIFA96_OK) {
    free(container);
    fifa96_asset_free(bytes);
    return -FIFA96_ERR_UNSUPPORTED;
  }
  for (size_t i = 0; i < info.count && !(have_team && have_sys); i++) {
    uint32_t off = 0;
    uint32_t size = 0;
    const char *name = NULL;
    uint8_t *bank = NULL;
    uint32_t bank_len = 0;
    uint32_t bank_count = 0;
    if (fifa96_bigf_record(&info, i, &off, &size, &name) != FIFA96_OK) continue;
    if (!name) continue;
    int is_team = !have_team && match_run_name_eq(name, MATCH_RUN_KIT_BANK);
    int is_sys = !have_sys && match_run_name_eq(name, MATCH_RUN_PALETTE_BANK);
    if (!is_team && !is_sys) continue;
    int got = match_stage_entry(container + off, size, container_len - off, &bank,
                                &bank_len, &bank_count);
    if (got != 1) continue;
    if (is_team) {
      /* P6: the kit indices select frames 0/2/4 (2*(idx/16)); frame 4 is the
       * third group (grayscale) and is optional for the legacy fallback. */
      for (uint32_t g = 0; g < 3; g++)
        kit_ok[g] = (uint8_t)match_run_palette_frame(bank, bank_len, 2u * g,
                                                     kit_frames[g]);
      if (kit_ok[0] && kit_ok[1]) have_team = 1;
    } else {
      if (match_run_palette_frame(bank, bank_len, 0u, sys0) &&
          match_run_palette_frame(bank, bank_len, 2u, sys2))
        have_sys = 1;
    }
    free(bank);
  }
  free(container);
  fifa96_asset_free(bytes);
  if (!have_sys || !have_team) return -FIFA96_ERR_UNSUPPORTED;

  /* The live base is PALsys frame 0 (the full match palette: the Fplain grass
   * indices 1..29 are green there). The derived non-live default uses the
   * frame-2 chunk itself (OL-T11-6 leg), which is magenta at those indices —
   * measured by rendering P1's first live frame pink. The native base is the
   * previously installed front-end palette (0x14B200, not statically
   * derivable); frame 0 is the derived live stand-in. The frame-2 appends and
   * the FU-98 kit remap follow the native order. */
  uint8_t pal6[768];
  memcpy(pal6, sys0, sizeof pal6);
  (void)fifa96_sprite_palette_kit_remap(pal6, MATCH_RUN_PALETTE_COUNT);
  memcpy(pal6 + MATCH_RUN_PALETTE_APPEND_A_OFF,
         sys2 + MATCH_RUN_PALETTE_APPEND_A_OFF, MATCH_RUN_PALETTE_APPEND_A_LEN);
  memcpy(pal6 + MATCH_RUN_PALETTE_APPEND_B_OFF,
         sys2 + MATCH_RUN_PALETTE_APPEND_B_OFF, MATCH_RUN_PALETTE_APPEND_B_LEN);
  for (int i = 0; i < 256; i++) {
    palette[i * 3] = (uint8_t)(pal6[i * 3] << 2);
    palette[i * 3 + 1] = (uint8_t)(pal6[i * 3 + 1] << 2);
    palette[i * 3 + 2] = (uint8_t)(pal6[i * 3 + 2] << 2);
  }

  /* Phase-86: the native kit-band destinations (0x106B70) are the low slots
   * 37/48/59 (home shirt/shorts/socks) and 70/81/92 (away). Build the default
   * team table with skin class 0 / hair class 3 (the phase-85 look) and then
   * one table per pitch record with that player's native classes. */
  for (int side = 0; side < 2; side++) {
    int shirt = side == 0 ? MATCH_RUN_KIT_SHIRT_HOME : MATCH_RUN_KIT_SHIRT_AWAY;
    int shorts = side == 0 ? MATCH_RUN_KIT_SHORTS_HOME : MATCH_RUN_KIT_SHORTS_AWAY;
    int socks = side == 0 ? MATCH_RUN_KIT_SOCKS_HOME : MATCH_RUN_KIT_SOCKS_AWAY;
    for (int i = 0; i < 256; i++)
      remap_team[side][i] = (uint8_t)(i == 0 ? 0xFF : i);
    match_run_apply_player_remap(remap_team[side], shirt, shorts, socks, 0, 3);
  }
  match_run_build_entity_remaps(mr, remap_team, remap_entity,
                                remap_extra_local);
  if (mr->matchup_active && mr->matchup.kit_ready) {
    /* P6: the selected kit triples select PALteam frame 2*(idx/16) and ramp
     * idx%16 (the native FUN_0004808C mapping). Brazil kit 0 (26,4,7) =
     * yellow/blue/green; Germany (30,33,21) = white/black/green. */
    const uint8_t *home = mr->matchup.kit[0][0];
    const uint8_t *away = mr->matchup.kit[1][0];
    (void)match_run_kit_band(palette, MATCH_RUN_KIT_SHIRT_HOME,
                             MATCH_RUN_KIT_SHIRT_LEN, home[0], kit_frames, kit_ok);
    (void)match_run_kit_band(palette, MATCH_RUN_KIT_SHORTS_HOME,
                             MATCH_RUN_KIT_SHORTS_LEN, home[1], kit_frames, kit_ok);
    (void)match_run_kit_band(palette, MATCH_RUN_KIT_SOCKS_HOME,
                             MATCH_RUN_KIT_SOCKS_LEN, home[2], kit_frames, kit_ok);
    (void)match_run_kit_band(palette, MATCH_RUN_KIT_SHIRT_AWAY,
                             MATCH_RUN_KIT_SHIRT_LEN, away[0], kit_frames, kit_ok);
    (void)match_run_kit_band(palette, MATCH_RUN_KIT_SHORTS_AWAY,
                             MATCH_RUN_KIT_SHORTS_LEN, away[1], kit_frames, kit_ok);
    (void)match_run_kit_band(palette, MATCH_RUN_KIT_SOCKS_AWAY,
                             MATCH_RUN_KIT_SOCKS_LEN, away[2], kit_frames, kit_ok);
  } else {
    /* No PCCD identity (synthetic/no-ISO fixtures): keep the pre-P6 derived
     * fallback. The old engine bands were home 208/195/224 and away 208/195/
     * 224 from PALteam frame 2; copy them into the native destinations. */
    match_run_copy_kit_band(palette, MATCH_RUN_KIT_SHIRT_HOME,
                            MATCH_RUN_KIT_SHIRT_LEN, kit_frames[0], 208);
    match_run_copy_kit_band(palette, MATCH_RUN_KIT_SHORTS_HOME,
                            MATCH_RUN_KIT_SHORTS_LEN, kit_frames[0], 195);
    match_run_copy_kit_band(palette, MATCH_RUN_KIT_SOCKS_HOME,
                            MATCH_RUN_KIT_SOCKS_LEN, kit_frames[0], 224);
    match_run_copy_kit_band(palette, MATCH_RUN_KIT_SHIRT_AWAY,
                            MATCH_RUN_KIT_SHIRT_LEN, kit_frames[2], 208);
    match_run_copy_kit_band(palette, MATCH_RUN_KIT_SHORTS_AWAY,
                            MATCH_RUN_KIT_SHORTS_LEN, kit_frames[2], 195);
    match_run_copy_kit_band(palette, MATCH_RUN_KIT_SOCKS_AWAY,
                            MATCH_RUN_KIT_SOCKS_LEN, kit_frames[2], 224);
  }
  /* Phase-85: the native hair class-3 ramp (PALsys frame-0 0xF0..0xF3) is
   * staged into the 0xD7..0xDA slots. The kit overlay no longer touches the
   * 0xDB..0xFD skin ramps, so classes 0..3 read the PALsys colours directly. */
  for (int i = 0; i < 4; i++) {
    palette[(0xD7 + i) * 3] = (uint8_t)(sys0[(0xF0 + i) * 3] << 2);
    palette[(0xD7 + i) * 3 + 1] = (uint8_t)(sys0[(0xF0 + i) * 3 + 1] << 2);
    palette[(0xD7 + i) * 3 + 2] = (uint8_t)(sys0[(0xF0 + i) * 3 + 2] << 2);
  }
  /* The v1 marking index stays white; 0xFE is also the native skin class-0
   * ninth entry, which the remap wraps to the eighth shade. */
  palette[FIFA96_PITCH_LINE_INDEX * 3] = 0xFF;
  palette[FIFA96_PITCH_LINE_INDEX * 3 + 1] = 0xFF;
  palette[FIFA96_PITCH_LINE_INDEX * 3 + 2] = 0xFF;
  /* Full success: commit the palette + remap atomically. */
  memcpy(r->palette, palette, sizeof r->palette);
  memcpy(r->remap_team, remap_team, sizeof r->remap_team);
  memcpy(r->remap_entity, remap_entity, sizeof r->remap_entity);
  memcpy(r->remap_extra, remap_extra_local, sizeof r->remap_extra);
  r->team_remap_ready = 1;
  /* FU-343 (P4): the final live palette is now installed; derive the HUD body
   * and 0xFE shade from it (the stage-time PALsys frame-2 values are stale). */
  match_run_hud_colors_refresh(mr);
  return 0;
}

int fifa96_match_run_stage_pitch(struct fifa96_match_run *mr, struct fifa96_engine *eng) {
  int rc;
  if (!mr || !eng) return -FIFA96_ERR_INVALID;
  if (!mr->running) return -FIFA96_ERR_STATE;
  if (!eng->assets) return -FIFA96_ERR_STATE;
  /* A1 §1: the field index table is [1]fplain [3]fhstrip. GAMEFLD3 is the
   * retail session's field (FU-2) but is the *striped* FHstrip container; the
   * plain FplainH/V tile the P1 brief names lives in GAMEFLD1. Try the plain
   * field first and fall back to the retail FHstrip field, so either binding
   * renders. */
  rc = fifa96_pitch_stage(&mr->render.pitch_art, eng, "/ART/GAMEFLD1.PVI",
                          "/ART/GAMESTD1.PVI");
  if (rc != 0)
    rc = fifa96_pitch_stage(&mr->render.pitch_art, eng, "/ART/GAMEFLD3.PVI",
                            "/ART/GAMESTD1.PVI");
  if (rc != 0) return rc;
  return match_run_stage_live_kit(mr, eng);
}

/* FU-150 P2 — fouls / referee / offside: the engine seam over the
 * `fifa96_referee` module. The module owns the native cells (registrar,
 * decision, the two sequence machines); the engine owns the machine dispatch,
 * the staged inputs (settings config, the pool record views, the derived
 * nearest queries), the act-2 free-kick/penalty hand-off and the frame
 * cadence. See port/src/fifa96_engine/fifa96_match_run.h for the contracts and
 * port/spec/FU150_fouls_referee_offside.md for the native evidence. */

/* The engine record view of one encoded entity id. `duel_ok` is staged 1: the
 * native 0x14C360/0x157BD2 compare tables have no ported producer (FU-150 leg
 * 4-adjacent); their producers are unported, so the derived equality holds. */
static int match_run_ref_view(const struct fifa96_match_run *mr, int32_t id,
                              struct fifa96_ref_record *view) {
  const struct fifa96_match_entity *e;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  e = &mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS]
           .records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
  view->id = id;
  view->side = mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS].side;
  view->player = e->index;
  view->active = e->active;
  view->duel_ok = 1;
  return 1;
}

/* FU-343 P3 fix: record one native `0x974DC(0x1E)` whistle assertion. The
 * staged slot `ref_whistle` is a level cleared by the referee step at the head
 * of each granted frame, so a clear + re-set inside one frame body would leave
 * the same byte at the audio observation. `whistle_seq` is the monotone write
 * count the audio module consumes; every writer of 0x1E must go through here. */
static void match_run_whistle_event(struct fifa96_match_run *mr) {
  mr->ref_whistle = 0x1Eu;
  mr->whistle_seq++;
}

/* Apply one sequence step's requests: the whistle/speech observation slots,
 * the phase write with its FU-149 arm (the native FUN_000740A0 runs the
 * per-team FUN_0008D098 immediately), the record install on rec_first and the
 * situation dispatch through the shared set-piece entry (BX=1, the native
 * FUN_0008A938 argument; its direct path may start the act-2 machine). The
 * native 0x4C374/0x651F0/0x974F0/0x92040/0x6E724/0x7D388 sink calls and the
 * dropped 0x14C3A0 stats write stay unported (FU-150 legs 3/10). */
static int match_run_ref_apply(struct fifa96_match_run *mr,
                               const struct fifa96_ref_sequence_out *out) {
  int rc;
  if (out->whistle != 0u) match_run_whistle_event(mr);
  if (out->speech_code != 0u) mr->ref_speech = out->speech_code;
  /* The native stage-0 order: the phase write runs before the record install
   * (0x8A091 vs 0x8A0A0), and the installer's +0x98 clear reads the new phase
   * cell. FUN_000740A0 stores the side byte at [0x157AAF] (=
   * `[0x157AAC]>>24`, the arm's controlled-side gate 0x8D728) before the
   * per-team FUN_0008D098 arms, so the arm installs on the request's side. */
  if (out->phase_write != 0xFFu) {
    mr->phase_machine.side_controlled = out->phase_side;
    rc = fifa96_match_run_write_phase(mr, out->phase_write);
    if (rc != 0) return rc;
    rc = fifa96_match_run_phase_arm(mr, out->phase_write);
    if (rc != 0) return rc;
  }
  if (out->install_action != 0u) {
    int32_t id = mr->referee.rec_first;
    if (id >= 0 &&
        id < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
      struct fifa96_match_entity *e =
          &mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS]
               .records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
      (void)fifa96_match_entities_install(e, mr->state.phase,
                                          out->install_action, 0);
    }
  }
  if (out->situation != 0xFFu) {
    rc = fifa96_match_run_set_piece(mr, out->situation, out->situation_side, 1u);
    if (rc != 0) return rc;
  }
  return 0;
}

/* The derived act-2 (phase-0x18) hand-off, started by the sit-9/sit-0xA table
 * rows with the row's stage (0 for sit 9, 1 for sit 0xA — the native
 * FUN_000888FC(2, stage, 1)):
 *  - stage 0 (0x89318/0x8935A): the soft-foul whistle then phase 0xA on the
 *    fouled side (the native then cascades through the stage 1/2 camera-lead
 *    gates; the port derives one stage per granted frame, FU-150 erratum);
 *  - stage 1 (0x893E2..0x89487 = native stage 2): the ball-row install and
 *    its `[0x158882]` latch (`= (contact_kind == 3)`) then the **session
 *    gate**;
 *  - stage 2 (0x894A2..0x895AC = native stage 3): the latch/kind/session gate
 *    then the decision.
 *
 * **The two gates are ordered differently and must not be flattened** (FU-310
 * §3.2; the review erratum item 2 as corrected by its append-only erratum):
 *  - native stage 2 increments the stage byte first (`0x89472 MOV
 *    CH,[0x158829]; 0x89478 INC CH; 0x8947F MOV [0x158829],CH`), then tests
 *    the session byte: `0x8947A MOV AL,[0x14C32A]; 0x89485 TEST AL,AL;
 *    0x89487 JZ 0x89615`. A byte-0 session **aborts before** the kind/flag
 *    terms; the shared epilogue skips the `0x895E6` teardown, and because the
 *    stage byte already reads 3 the next driver call dispatches
 *    `table[3] = 0x894A2` (fresh `read_memory 0x89214` entry 3 =
 *    `a2 94 08 00`), i.e. the engine's stage 2. The engine models the abort as
 *    the stage 1 -> 2 split hold. (A non-zero session then runs the
 *    `0x8948D..0x8949C` decision-word check; that cell stays the FU166-L6
 *    leg — R3 §4 proves the read is of a stale callee-local word, not a
 *    caller parameter.)
 *  - native stage 2 also installs the ball anim row from the contact kind
 *    (`0x89401/0x8941D` keep/reroll rows 0/0x62..0x65, flags 0x07; the
 *    non-3 `0x8944F/0x89464` row 0x49, flags 0x00), and `FUN_0006E724`
 *    writes `row_flags & 1` to `[0x158882]` at `0x6E7E7`. The engine latches
 *    the net effect at its stage-1 body: `ball_row_flag_158882 =
 *    (contact_kind == 3)`, before the byte-0 split and for both sessions.
 *  - native stage 3 tests the ball-row latch (`0x894A2/0x894A9`), then
 *    `kind == 3` (`0x894AD/0x894B2/0x894B5` -> the decision at `0x894C4`),
 *    then the session (`0x894B7/0x894BE`). Because the latch is read first,
 *    a contact that was kind 3 when the stage-2 body ran still decides after
 *    a later collision rewrites `[0x15888C]` — the engine's stage 2 passes on
 *    `flag || kind == 3 || session != 0`, independent of how the stage was
 *    reached. A byte-0 + kind-3 contact is the native **two-call decision**
 *    (split on one call, decide on the next).
 *  - A non-zero session reaches the same stage-3 gate directly from stage 1
 *    (the native stage-2 body falls through); the session term then passes
 *    every kind, subject only to the unmodelled decision-word check above.
 * Phase 7 by default; phase 6 when the contact kind is not 3, |incident x| <
 * 0x420 and z lies in the fouler-side band [-0xB10,-0x7B0] side 0 /
 * [0x7B0,0xB10] side 1 (0x894DB..0x89550, verified). Speech 0x23 (6) /
 * 0x2A (7) on rec_second, [0x15882A] cleared (0x8955F) and the FU-149 phase
 * arm installs the taker/keeper. */
/* Phase 70 — the derived ordinary-match ball-settle release [D].
 *
 * Native chain (fresh Ghidra): `FUN_0008AF38 0x8B65B` calls `FUN_0008BAF0`
 * only when `[0x14C32A] == 0`; its tail `0x8C05A` calls `FUN_0006FE84` on the
 * ball block, and the row-0x49 settle animation installs successor row 0x00
 * (flags 0x07) through `FUN_0006E724` at `0x6FF67..0x6FF85`, writing
 * `[0x158882] = 1`. The row-0x49 frame durations are 0x80/0x40/0x1C0 = 0x280
 * (`read_memory 0x10E9FC`), and the native timer add is `[frame_delta] << 4`
 * (`0x6FFA5..0x6FFAE`). The engine models the release only; the visual frame
 * walk stays the L14.8.b leg. */
#define MATCH_RUN_BALL_SETTLE_UNITS 0x280u

static void match_run_ball_settle_clear(struct fifa96_match_run *mr) {
  if (!mr) return;
  mr->ball_settle_timer = 0;
  mr->ball_settle_active = 0;
}

static void match_run_ball_settle_start(struct fifa96_match_run *mr) {
  if (!mr) return;
  mr->ball_settle_timer = MATCH_RUN_BALL_SETTLE_UNITS;
  mr->ball_settle_active = 1;
}

static void match_run_ball_settle_step(struct fifa96_match_run *mr) {
  uint32_t step;
  if (!mr || mr->ball_settle_active == 0u) return;
  /* The native machine is skipped under the Practise session; the act-2 gate
   * already passes there through the session term. */
  if (mr->session_gate_14c32a != 0u) {
    match_run_ball_settle_clear(mr);
    return;
  }
  step = (uint32_t)mr->state.frame_delta << 4;
  if (step >= mr->ball_settle_timer) {
    mr->ball_settle_timer = 0;
    mr->ball_settle_active = 0;
    mr->ball_row_flag_158882 = 1u;   /* successor row 0x00, flags 0x07 */
  } else {
    mr->ball_settle_timer = (uint16_t)(mr->ball_settle_timer - step);
  }
}

static int match_run_ref_step_restart(struct fifa96_match_run *mr) {
  int rc;
  if (mr->ref_restart_stage == 0u) {
    if (mr->referee.foul_kind == 0u) match_run_whistle_event(mr);
    /* 0x8935A: FUN_000740A0(0xA, fouled side) — the side lands in [0x157AAF]
     * before the arm. */
    mr->phase_machine.side_controlled = (uint8_t)((mr->referee.rec_first_side ^ 1u) & 1u);
    rc = fifa96_match_run_write_phase(mr, 0x0Au);
    if (rc != 0) return rc;
    mr->ref_restart_stage = 1u;
    return 1;
  }
  /* Native stage 2 (0x893E2..0x89487): the ball-row install latches
   * `[0x158882] = (contact_kind == 3)` (see `ball_row_flag_158882` in the
   * header: the keep/reroll rows 0/0x62..0x65 have flags 0x07, the non-3
   * row-0x49 install at 0x8944F/0x89464 has 0x00), for both sessions,
   * before the stage byte is incremented (`0x8947F`) and the session-0
   * abort (`0x89487`). The engine's stage 1 is that body, so a byte-0 frame
   * performs the split and does not decide. */
  if (mr->ref_restart_stage == 1u) {
    mr->ball_row_flag_158882 = mr->referee.contact_kind == 3u ? 1u : 0u;
    if (mr->session_gate_14c32a == 0u) {
      /* Phase 70: the native stage-2 row-0x49 install leaves the latch 0; the
       * ordinary ball appearance pass installs row 0x00 after the settle
       * duration and the next stage-3 tick passes. Kind 3 keeps the latch 1
       * and needs no settle. */
      if (mr->referee.contact_kind != 3u)
        match_run_ball_settle_start(mr);
      mr->ref_restart_stage = 2u;
      return 1;
    }
  }
  /* Native stage 3 (0x894A2..0x894BE; also reached from stage 1 with a
   * non-zero session): the stage-2 latch -> kind == 3 -> session. The latch
   * is read first (0x894A2/0x894A9), so a byte-0 contact that was kind 3 when
   * the stage-2 body ran still decides after a later collision rewrites
   * `[0x15888C]`; `kind == 3` passes at 0x894B2/0x894B5 before the 0x894B7
   * session test. */
  if (!(mr->ball_row_flag_158882 != 0u || mr->referee.contact_kind == 3u ||
        mr->session_gate_14c32a != 0u)) {
    return 1;
  }
  {
    uint8_t phase = 7u;
    if (mr->referee.contact_kind != 3u) {
      int32_t ax = mr->incident_x;
      if (ax < 0) ax = -ax;
      if (ax < 0x420) {
        int32_t z = mr->incident_z;
        if (mr->referee.rec_first_side == 0u) {
          if (z >= -0xB10 && z <= -0x7B0) phase = 6u;
        } else {
          if (z >= 0x7B0 && z <= 0xB10) phase = 6u;
        }
      }
    }
    /* 0x89590..0x895AC: team byte of [0x15888F] ^ 1 passed to FUN_000740A0
     * (the fouled side) before the phase-7/6 taker arm gates on it. */
    mr->phase_machine.side_controlled = (uint8_t)((mr->referee.rec_first_side ^ 1u) & 1u);
    rc = fifa96_match_run_write_phase(mr, phase);
    if (rc != 0) return rc;
    rc = fifa96_match_run_phase_arm(mr, phase);
    if (rc != 0) return rc;
    mr->ref_speech = phase == 6u ? 0x23u : 0x2Au;
    mr->global_5882a = 0;                          /* 0x8955F */
    mr->render.camera_target.source = 0;           /* 0x895B8 / 0x895F2 */
  }
  match_run_ball_settle_clear(mr);     /* Phase 70: the release is consumed */
  mr->ref_machine = FIFA96_MATCH_RUN_REF_NONE;
  /* FU-183 §5 (phase-16 T1): the act machine owns phase 0x18 — clear the act-2
   * slot at the terminal (the retired `ref_machine = REF_RESTART` route no
   * longer does, so nothing else would clear it). */
  if (mr->act.id == 2) {
    mr->act.id = 0;
    mr->act.stage = 0;
    mr->act.timer = 0;
  }
  return 1;
}

/* The `0x36200(2)` target for the phase-0x18/0x19 entities: the native
 * stores a record pointer in [0x157AA3], so the engine resolves the entity
 * id to its pool record triple. */
static void match_run_camera_source2_entity(struct fifa96_match_run *mr,
                                            int32_t id) {
  uint32_t team, index;
  const struct fifa96_match_entity *e;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return;
  team = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  index = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  e = &mr->entities.team[team].records[index];
  mr->render.camera_target.source = 2;
  mr->render.camera_target.record[0] = e->pos_x;
  mr->render.camera_target.record[1] = e->pos_y;
  mr->render.camera_target.record[2] = e->pos_z;
}

/* FU-183 §5/§6 rank 2 (phase-16 T1): the phase-0x18 / act-2 row body — the
 * derived free-kick / penalty hand-off. The native act-2 handler `0x8922C` is
 * the per-record phase-0x18 timeline; the derived body is the ported
 * `match_run_ref_step_restart` (stages 0/1/2, FU-150 erratum). The act machine
 * owns the row: `match_run_situation_side` dispatches act 2 invoke-now (the
 * native `FUN_000888FC(2, stage, BX=1)`) and the `0x8B669` clock-tail tick
 * advances it each granted frame (FU175-L7's ad-hoc `ref_machine` double-run is
 * retired). The head accumulates the act timer (`word[0x158818] +=
 * word[0x157A64]`, the 0x8922C head); the terminal (inside the body) clears the
 * act slot. Returns FIFA96_OK (the phase-row convention; the native
 * `FUN_000888FC`/clock-tail invoke ignores the handler's return), or
 * -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_action_phase_18(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  mr->act.timer = (int16_t)(mr->act.timer + (int16_t)mr->state.frame_delta);
  /* 0x892D7..0x892F9: contact kind 3 selects rec_first, else rec_second;
   * [0x157AA3] = that record and 0x36200(2). */
  match_run_camera_source2_entity(
      mr, mr->referee.contact_kind == 3u ? mr->referee.rec_first
                                         : mr->referee.rec_second);
  (void)match_run_ref_step_restart(mr);
  return FIFA96_OK;
}

/* FU-187 §2/§5 rank 7 (phase-16 T4): the phase-0x19 / act-3 row body — the
 * 7-stage foul sequence. The native handler `0x89FA4` reads the ACT slot's
 * stage/timer (`[0x158829]`/`[0x158818]`); the derived body aliases
 * `mr->act.stage/timer` <-> `referee.stage/timer` around the ported
 * `fifa96_ref_foul_sequence_step`, so the native head add (`[0x158818] +=
 * [0x157A64]`, `0x89FC8..0x89FDC`) is performed exactly once by the step's
 * `state->timer += state->delta` (FU187-L3 reconciled — no double count). The
 * step's requests apply through `match_run_ref_apply`: stage 0 writes phase
 * 0xF (`0x8A091`) and installs action 0x16 on `rec_first` (`0x8A0A0`), stage 6
 * dispatches situation 0xA (`0x8A3F0`). On the terminal the act slot is cleared
 * BEFORE the situation apply (the native `0x8A3D0/0x8A3D6/0x8A3EA` order), so
 * the clock-tail tick stops. Returns FIFA96_OK (the phase-row convention), or
 * -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_action_phase_19(struct fifa96_match_run *mr) {
  struct fifa96_ref_sequence_out out;
  int rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  /* 0x89FA9..0x89FC3: [0x157AA3] = rec_first and 0x36200(2). */
  match_run_camera_source2_entity(mr, mr->referee.rec_first);
  mr->referee.stage = mr->act.stage;               /* the act slot is the authority */
  mr->referee.timer = (uint16_t)mr->act.timer;
  mr->referee.delta = mr->state.frame_delta;
  rc = fifa96_ref_foul_sequence_step(&mr->referee, &out);
  if (rc < 0) return rc;
  mr->act.stage = mr->referee.stage;               /* mirror the advanced stage back */
  mr->act.timer = (int16_t)mr->referee.timer;
  if (out.done != 0u) {                            /* native 0x8A3D0/0x8A3D6/0x8A3EA */
    mr->act.id = 0;
    mr->act.stage = 0;
    mr->act.timer = 0;
  }
  rc = match_run_ref_apply(mr, &out);
  if (rc != 0) return rc;
  return FIFA96_OK;
}

/* FU-188 §2/§6 (phase-16 T4): the phase-0x20 / act-0xA row body (the code-0x24
 * timeline). Its phase-table slot stays NULL (uncounted): the sole producer is
 * situation 0 (FU188-L1). Derived stages (the native `0x8B688`):
 *  - head (`0x8B691`): `mr->act.timer += frame_delta`;
 *  - stage 0 (`0x8B6C3`): install code 0x24 on both teams' 11 records (the
 *    native invoke-now loops `0x8B750`/`0x8B78B`, FU-188 §3). The engine has no
 *    generic invoke-now (FU188-L4), so only the install lands (the timeline
 *    cells and the `0x700F4` camera reset are FU188-L2 legs); timer = 0,
 *    stage -> 1;
 *  - stage 1 (`0x8B7CB`): the `0x974D8(0x3E8)` wait (derived: timer >= 0x3E8;
 *    the sink is FU188-L2), then stage -> 2;
 *  - stage 2 (`0x8B7F6`): clear the act slot (`0x8B819..0x8B82C`; the
 *    `word[0x15881A] >= 0x3C` / `0x45001` gates are FU188-L2 legs) and
 *    re-dispatch situation 1 (`0x8B85D`).
 * Returns FIFA96_OK, or -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_action_phase_20(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  mr->act.timer = (int16_t)(mr->act.timer + (int16_t)mr->state.frame_delta);
  if (mr->act.stage == 0u) {                       /* 0x8B6C3 */
    for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
      for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
        (void)fifa96_match_entities_install(&mr->entities.team[t].records[i],
                                            (uint8_t)mr->state.phase, 0x24u, 0);
    mr->act.timer = 0;                             /* 0x8B7B4 */
    mr->act.stage = 1u;                            /* 0x8B7BB */
    return FIFA96_OK;
  }
  if (mr->act.stage == 1u) {                       /* 0x8B7CB */
    if (mr->act.timer < 0x3E8) return FIFA96_OK;   /* the 0x974D8(0x3E8) wait */
    mr->act.timer = 0;
    mr->act.stage = 2u;
    return FIFA96_OK;
  }
  /* stage 2 terminal (0x8B7F6): clear the act slot, then situation 1 (BX=0),
   * then 0x36200(0) at 0x8B864. */
  mr->act.id = 0;
  mr->act.stage = 0;
  mr->act.timer = 0;
  {
    int rc = (int)fifa96_match_run_situation(mr, 1u);
    mr->render.camera_target.source = 0;
    return rc;
  }
}

/* FU-193 §2/§6.1 (M2 phase-17 T4) + FU-203 (M2 phase-18 T4): the phase-0x17 /
 * act-1 row body — the native kickoff / phase-1 entry `0x88DC8` (the
 * phase/act dual identity: `0x110794[0x17] == 0x1107EC[1] == 0x88DC8`).
 * COUNTED (T4): the row is registered and the producer is live — the phase-0x20
 * body's stage-2 terminal re-dispatches situation 1, arming act 1, and the
 * clock-tail `fifa96_match_act_tick` dispatches this body. The begin-time
 * kickoff is entered through the same act-1 route (`fifa96_match_act_dispatch(
 * mr, 1, 0, 1)`), so the derived ad-hoc call is retired.
 * Body: head `[0x158818] += [0x157A64]`; stage 0 clears `[0x15882A]` and runs
 * the derived kickoff entry (the phase-1 write), timer 0, stage++; stage 1 hands
 * off to act 4 (0x88EE8) on `[0x157A4C] != 1 && a residual mismatch`, else at
 * the derived `state.tick_total >= 0x78` sets `[0x15882A] = 1` and clears the
 * slot. The head guard (goal-log words `[0x157AC5]`/`[0x157AC7]` + `[0x157AC2]`)
 * and the presentation sinks are FU193-L7 legs. NAME COLLISION: this PHASE row
 * 0x17 is distinct from ACTION row 0x17 (the act-4 formation-arm installer,
 * `fifa96_match_action_table[0x17]`, NULL/leg) — FU-203 §2.4. */
int fifa96_match_action_phase_17(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  mr->act.timer = (int16_t)(mr->act.timer + (int16_t)mr->state.frame_delta);  /* 0x88DD7 */
  if (mr->act.stage == 0u) {
    /* 0x88E56 clear the kickoff gate, then the FUN_000740A0(1) phase-1 write +
     * placement commit (the derived kickoff entry); timer = 0, stage++ (0x88EAA/
     * 0x88EB1). */
    mr->global_5882a = 0u;
    (void)fifa96_match_run_kickoff_entry(mr);
    mr->act.timer = 0;
    mr->act.stage = 1u;
    return FIFA96_OK;
  }
  if (mr->act.stage == 1u) {
    /* 0x88EBC..0x88EE8: the formation hand-off to act 4 (invoke-now) when the
     * goal-side flag [0x157A4C] != 1 and a residual mismatch exists. */
    if (mr->global_157a4c != 1u &&
        (mr->formation_mismatch[0] != 0u || mr->formation_mismatch[1] != 0u))
      return fifa96_match_act_dispatch(mr, 4, 0, 1);
    /* 0x88EF3: the shared timeline gate word[0x15881A] >= 0x78 (the alias
     * [0x158816]>>16; the engine's whole-delta `state.tick_total`). */
    if ((int32_t)mr->state.tick_total >= 0x78) {
      mr->global_5882a = 1u;                 /* 0x88F07 */
      mr->act.id = 0;                        /* 0x88F14 */
      mr->act.stage = 0;                     /* 0x88F24 */
      mr->act.timer = 0;                     /* 0x88F0D */
    }
    return FIFA96_OK;
  }
  return FIFA96_OK;   /* stage > 1: the 0x88E46 -> 0x88F2A tail */
}

/* FU-193 §3 / FU-197 §6 (M2 phase-17 T4): the phase-0x1A tail (0x897AE) — clear
 * the act slot and re-dispatch the stored situation/side with BX=1. */
static int match_run_phase_1A_end(struct fifa96_match_run *mr) {
  mr->act.timer = 0;                                       /* 0x897BD */
  mr->act.id = 0;                                          /* 0x897C4 */
  mr->act.stage = 0;                                       /* 0x897CA */
  return (int)fifa96_match_run_set_piece(mr, mr->store_15881e,
                                         mr->store_158820, 1u);  /* 0x897E3 */
}

/* FU-193 §3/§6.2 + FU-197 §6/§9.4 + FU-201 §6 (M2 phase-17 T4 / phase-18 T2):
 * the phase-0x1A / act-4 row
 * body — the native formation-order / line-up walk `0x89620`. Its slot stays
 * NULL/uncounted (no act-4 producer: FU-201 §6/R49 proves the T2 arm is not live
 * even under a faithful matrix fill — the permutation residual after the
 * desired→current reset copy is 0, and the real trigger is a runtime
 * non-permutation, FU201-L1; FU193-L3). Stage 0 walks the records
 * for the first current/desired mismatch, accepts the current order on an
 * occupied record, else writes phase 0xB and installs action row 0x17 on the
 * active record; the end block re-dispatches the stored situation. The walk is
 * gated on `mr->formation_order_ok`. */
int fifa96_match_action_phase_1A(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  mr->act.timer = (int16_t)(mr->act.timer + (int16_t)mr->state.frame_delta);  /* 0x89629 */
  if (mr->act.stage == 0u) {
    int32_t s;
    if (mr->formation_order_ok == 0u) return FIFA96_OK;    /* dormant (FU201-L4) */
    /* 0x89655..0x8965c: side select = (byte[0x157B8E] == 0); 0x8966A: decrement
     * that side's residual counter. */
    s = (mr->formation_mismatch[0] == 0u) ? 1 : 0;
    if (mr->formation_mismatch[(uint32_t)s] != 0u)
      mr->formation_mismatch[(uint32_t)s]--;
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      uint32_t ds;
      struct fifa96_match_entity *rec;
      if (mr->formation_current[s][i] == mr->formation_order[s][i]) continue;
      /* 0x896B5 CALL 0x741B4: the display side (side ^ [0x157ABE]) & 1. */
      ds = ((uint32_t)s ^ mr->side_swap) & 1u;
      rec = &mr->entities.team[ds].records[i];
      if (rec->skip_9a != 0u) {
        /* 0x896D1/0x896D7: the record is occupied -> accept the current order
         * for the slot, then the end block. */
        mr->formation_order[s][i] = mr->formation_current[s][i];
        break;
      }
      /* 0x896E2 install: write phase 0xB on the record's team side (0x8972F),
       * then install action row 0x17 (0x89740; the generic invoke-now has no
       * engine port, FU197-L6); timer = 0, stage++ (0x89747/0x8974E). */
      if (rec->team >= FIFA96_MATCH_ENTITY_TEAMS) return FIFA96_OK;
      mr->phase_machine.side_controlled =
          (uint8_t)mr->entities.team[rec->team].side;       /* [[rec]+0x826] */
      (void)fifa96_match_run_write_phase(mr, 0x0Bu);
      (void)fifa96_match_run_phase_arm(mr, 0x0Bu);
      (void)fifa96_match_entities_install(rec, 0x0Bu, 0x17u, 0u);
      mr->act.timer = 0;
      mr->act.stage = 1u;
      return FIFA96_OK;
    }
    /* 0x89763 ESI == 0xB: the loop exhausted (or the occupied branch) -> the
     * end block. */
    return match_run_phase_1A_end(mr);
  }
  if (mr->act.stage == 1u) {
    /* 0x89775: the installed-record table gate ([rec+0x8E] -> [0x110680] & 1,
     * FU193-L8 leg) and the word[0x15881A] > 0xF0 timeline gate (derived on
     * `state.tick_total`); on pass, the end block. */
    if ((int32_t)mr->state.tick_total <= 0xF0) return FIFA96_OK;
    return match_run_phase_1A_end(mr);
  }
  return FIFA96_OK;   /* stage > 1: the 0x89650 -> 0x897E8 epilogue */
}

/* FU-193 §4/§6.3 (M2 phase-17 T4): the phase-0x1C / act-6 row body — the native
 * pre-restart timeline `0x89110`. Its slot stays NULL/uncounted (no act-6
 * producer; the offside machine is the existing driver — a double-model,
 * FU193-L4). The native handler is already ported as
 * `fifa96_ref_offside_sequence_step`; this body aliases the act slot's
 * stage/timer to the referee state around the step (the phase-0x19 FU187-L3
 * pattern) so the head add runs exactly once, clears the act slot on the
 * terminal, and applies the requests (phase 0xA + whistle on stage 0; the
 * situation-9 re-dispatch on stage 2) through `match_run_ref_apply`. */
int fifa96_match_action_phase_1C(struct fifa96_match_run *mr) {
  struct fifa96_ref_sequence_out out;
  int rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  mr->referee.stage = mr->act.stage;               /* the act slot is the authority */
  mr->referee.timer = (uint16_t)mr->act.timer;
  mr->referee.delta = mr->state.frame_delta;
  rc = fifa96_ref_offside_sequence_step(&mr->referee, &out);
  if (rc < 0) return rc;
  mr->act.stage = mr->referee.stage;               /* mirror the advanced stage back */
  mr->act.timer = (int16_t)mr->referee.timer;
  if (out.done != 0u) {                            /* native 0x891CC/0x891E8/0x891EE */
    mr->act.id = 0;
    mr->act.stage = 0;
    mr->act.timer = 0;
  }
  rc = match_run_ref_apply(mr, &out);
  if (rc != 0) return rc;
  return FIFA96_OK;
}

/* FU-83 §2 / FU-195 §5 (phase-36, fresh `0x890EC..0x8910D`): the phase-0x1B /
 * act-5 pure act-slot reset. Four stores only: word `[0x158818]` (timer),
 * byte `[0x158828]` (id), byte `[0x158829]` (stage), dword `[0x158808]`
 * (handler; derived from id in the port). No CALL, no phase write; the native
 * clobbers AH but no engine cell mirrors that. The phase-table slot stays NULL:
 * a fresh 11-site `CALL 0x888FC` census passes no EAX=5 and all 27
 * `FUN_000740A0` sites emit no phase 0x1B, so the row is ported-but-dormant. */
int fifa96_match_action_phase_1B(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  mr->act.timer = 0;                                  /* 0x890F1 word */
  mr->act.id = 0;                                     /* 0x890F8 */
  mr->act.stage = 0;                                  /* 0x89100 */
  return FIFA96_OK;                                   /* 0x8910D RET */
}

/* FU-209 §2 arm-2/6/8 helper: the native actor record `[0x1587D4]` (the head's
 * `[ESP+8]` at 0x89876). Returns NULL when the cell is NONE or out of pool range
 * — the derived guard for a path the native would take on a garbage pointer. */
static struct fifa96_match_entity *match_1d_actor(struct fifa96_match_run *mr) {
  if (mr->actor_1587d4 < 0 ||
      mr->actor_1587d4 >=
          (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return NULL;
  return &mr->entities
              .team[(uint32_t)mr->actor_1587d4 / FIFA96_MATCH_ENTITY_RECORDS]
              .records[(uint32_t)mr->actor_1587d4 % FIFA96_MATCH_ENTITY_RECORDS];
}

/* FU-194 §3/§6.1 (M2 phase-17 T5) + FU-208/FU-209 §6.4 (M2 phase-19 T2) +
 * FU-209 §2 (arms 2..8, this round): the phase-0x1D / act-7 row body — the
 * native 10-arm set-piece timeline `0x89868` (arms `0x898B4..0x89EAF`, the CS
 * arm table `0x89840`). Its phase-table slot is REGISTERED and COUNTED as of
 * T2: the T1 period-index roll (`global_157ac2 == 4`) drives the mode-0 sit-8
 * selector in `phase_drive`, and the clock-tail act tick dispatches this body.
 * The row is one coupled unit with action row 0x20 (arm 0 carries the row-0x20
 * installer `0x899A2`).
 *
 * FU-209 §2 (fresh `disassemble_bytes 0x89868`, 1608 B, 427 insns): the arms
 * are a FALL-THROUGH CASCADE, not a set of RETURN-terminated cases — a passing
 * arm advances `byte[0x158829]` and runs the next arm body in the SAME call; a
 * failed wait gate returns through the shared epilogue `0x89EA6`. Arm 3 is the
 * exception: both of its exits RETURN (`0x89AAE JMP 0x89EA6` and the `0x89AB3`
 * tail's own RET), so arm 4 runs on the next call. The loop below mirrors that.
 *
 * Arm table (addresses fresh this round; full derivation FU-209 §2.1..§2.10):
 *   0 `0x898B4` install frame: `[0x15882A]=0` (and `[0x157AB1]=0`, no engine
 *     cell, OL-13), phase 0x10 when `state.phase != 0x10` (FU194-L9), row 0x19
 *     on both team bases (`0x89903`/`0x8990A`, outside the 11-record loop) and
 *     row 0x20 on the actor (`0x899A2`, deferred); timer 0, stage 1.
 *   1 `0x899EE` whistle wait: the shared timeline `word[0x15881A]` (derived
 *     `state.tick_total`) >= 0x78 -> `0x974DC(0x1E)`, `[0x15882A]=1`, stage 2.
 *   2 `0x89A4E` actor/referee contact wait: the actor record == `[0x158730]`
 *     (the ball-pair actor) and `byte[0x158746]` (the pair ack) != 0 -> stage 3.
 *   3 `0x89A7A` restart-line gate: `[0x15781D]` (goal_armed) != 0 and
 *     `[0x15781E]` (goal_zone) == 0 -> `0x651F0(6)`/`0x974F0(0x5DC)`, stage 4,
 *     RETURN; else the `0x89AB3` raise-then-lower tail on `word[0x1577BE]`
 *     (camera.speed) / `word[0x1577F0]` (camera.event_param) -> stage 4, RETURN.
 *   4 `0x89AEA` both camera words zero -> stage 5, falls into 5.
 *   5 `0x89B16` `word[0x15881A] >= 0x3C` and (`>= 0x78` or `0x45001()`) ->
 *     stage 6, falls into 6.
 *   6 `0x89B50` actor code (`+0x91`) != 0x20 and ball record `+0x44` ->
 *     `0x4C374(0x1A)`, stage 7, falls into 7.
 *   7 `0x89B94` `0x4BEC8()` (fresh: `XOR EAX,EAX; RET`, always ready) -> stage
 *     8, falls into 8.
 *   8 `0x89BB6..0x89E6D` the substitution-strip selection block -> event
 *     `0x8F188(0xBA,0x20,...)`, `0x4C374(7)`, stage 9, falls into 9; the
 *     no-winner arm rebinds the actor from `[0x1587CC+(side^1)*4]` and resets
 *     to stage 0 (`0x89DCF`/`0x89DBB`, FU217-L2).
 *   9 `0x89E6E` terminal (no situation re-dispatch): clear the act slot and
 *     `[0x157A4C]`.
 *
 * Remaining legs: the `[0x15881A]` timeline word has no dedicated engine cell,
 * so `state.tick_total` is the derived substitute arms 1/5 share with phase
 * 0x17/0x1C; the `0x651F0`/`0x974F0` sinks (arm 3) are unported derived no-ops
 * and the `0x8F188` event-ring post (arm 8) is the OL-27 leg, recorded by value
 * as `match1d_event_id`; the
 * `0x45001()` input gate (`byte[0x1067EC] & 0xF0`, no engine cell) is the
 * FU184-L3/FU188-L2 sibling leg; `0x4C374` only clears `word[0x157A68]`
 * (ignores its EAX, FU194-L3/L4, no reader); the ball record `[[actor]+0x7A6]`
 * is the record-11 spill slot the pool does not model (FU-142 OL-49), so its
 * `+0x44` gate is derived ready. Arm 9's native `[0x157AC0]=1` write is
 * mirrored (`0x89E8C`); FU209-L5 is closed. Returns FIFA96_OK, or
 * -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_action_phase_1D(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  mr->act.timer = (int16_t)(mr->act.timer + (int16_t)mr->state.frame_delta); /* 0x89882/0x89899 */
  for (;;) {
    switch (mr->act.stage) {
      case 0u:   /* arm 0 (0x898B4): the install frame */
        mr->global_5882a = 0u;                       /* 0x898B4 clear [0x15882A] */
        /* 0x898B4 also clears [0x157AB1] — an unported process flag with no engine
         * cell (FU-89 §11 / FU-189 §3); leg (OL-13). */
        if (mr->state.phase != 0x10u) {              /* 0x898B6 CMP [0x157A4A]>>24,0x10 */
          (void)fifa96_match_run_write_phase(mr, 0x10u);    /* 0x898CF..0x898DD FUN_000740A0(0x10) */
        }
        /* 0x89903/0x8990A: install action row 0x19 on the team base (EBP) once
         * per team. The native `CALL 0x7D9A4` sits OUTSIDE the 11-record ESI
         * loop (whose only install is row 3 on inactive records — FU194-L7 leg);
         * the engine convention maps a native team-base install to records[0]
         * (the phase-8/9 arm, `fifa96_match_run_phase_arm`). */
        for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
          (void)fifa96_match_entities_install(&mr->entities.team[t].records[0],
                                              mr->state.phase, 0x19u, 0);
        /* 0x899A2 (the coupled row-0x20 install on the actor record [0x1587D4]):
         * the native `FUN_0007D9A4(rec, 0x20, 0, 0)` (deferred, stage 0, invoke
         * 0). The target cell's producer — the sit-8 arm's `[0x1587CC+side*4] ->
         * [0x1587D4]` bind (`0x8AE4B`) — now LANDS LIVE (FU-217/T3: the
         * `match_run_free_record` selector + the `set_piece` sit-8 re-init, which
         * runs before act 7's invoke-now dispatch on the same frame), so
         * `mr->actor_1587d4` is a real free outfield record and this install
         * fires: code 0x20 lands on the actor and action row 0x20 is COUNTED.
         * The `actor_1587d4` gate is kept (a NONE cell — the pre-bind path — is a
         * null no-op). 0x899CD (the ball-record row-0x1F install) and 0x7A028
         * (the referee block reset) stay FU194-L7/L8 legs. */
        {
          struct fifa96_match_entity *actor = match_1d_actor(mr);
          if (actor != NULL)
            (void)fifa96_match_entities_install(actor, mr->state.phase, 0x20u, 0u);
        }
        mr->act.timer = 0;                           /* 0x899D7 */
        mr->act.stage = 1u;                          /* 0x899ED */
        continue;                                    /* falls into arm 1 */
      case 1u:  /* arm 1 (0x899EE): the wait -> whistle + [0x15882A]=1 */
        /* The native gates read word[rec+0x65] (unported record cells, FU194-L7)
         * and the shared timeline word[0x15881A] (derived state.tick_total). */
        if ((int32_t)mr->state.tick_total < 0x78) return FIFA96_OK;
        match_run_whistle_event(mr);                 /* 0x974DC(0x1E) */
        mr->global_5882a = 1u;                       /* [0x15882A]=1 */
        mr->act.timer = 0;
        mr->act.stage = 2u;
        continue;                                    /* falls into arm 2 */
      case 2u: {  /* arm 2 (0x89A4E): the actor/referee contact wait */
        /* 0x89A52: the actor record against the staged ball actor [0x158730]
         * (both cells hold pool ids in the engine); 0x89A5E/0x89A65: the
         * byte[0x158746] ack (the ball-pair stage's `ack`, written by 0x7A2F0). */
        if (mr->actor_1587d4 != mr->entities.ball.pair.actor) return FIFA96_OK;
        if (mr->entities.ball.pair.ack == 0u) return FIFA96_OK;
        mr->act.timer = 0;                           /* 0x89A6D */
        mr->act.stage = 3u;                          /* 0x89A74 */
        continue;                                    /* falls into arm 3 */
      }
      case 3u: {  /* arm 3 (0x89A7A): the restart-line gate + the 0x89AB3 tail */
        /* 0x89A81/0x89A8A: [0x15781D] = goal_armed, [0x15781E] = goal_zone. */
        if (mr->goal_armed != 0u && mr->goal_zone == 0u) {
          /* 0x89A8C/0x89A96: 0x651F0(6) (the sit dispatch) and 0x974F0(0x5DC)
           * (the audio id). Both sinks are unported (FU194-L3); derived no-op. */
          mr->act.timer = 0;                         /* 0x89AA0 */
          mr->act.stage = 4u;                        /* 0x89AA8 */
          return FIFA96_OK;                          /* 0x89AAE JMP 0x89EA6 */
        }
        /* 0x89AB3: the raise-then-lower handshake tail. Either camera word high
         * advances to stage 4; both zero waits. word[0x1577BE]/word[0x1577F0]
         * are the LIVE camera.speed/camera.event_param cells (FU-300 fix wave). */
        if (mr->render.camera.speed == 0u && mr->render.camera.event_param == 0u)
          return FIFA96_OK;                          /* 0x89AC5 JZ 0x89EA6 */
        mr->act.timer = 0;                           /* 0x89AD4 */
        mr->act.stage = 4u;                          /* 0x89ADB */
        return FIFA96_OK;                            /* 0x89AE0..0x89AE9 RET */
      }
      case 4u:  /* arm 4 (0x89AEA): the camera-event clear wait */
        if (mr->render.camera.speed != 0u) return FIFA96_OK;      /* 0x89AF4 */
        if (mr->render.camera.event_param != 0u) return FIFA96_OK;/* 0x89B02 */
        mr->act.timer = 0;                           /* 0x89B0A */
        mr->act.stage = 5u;                          /* 0x89B11 */
        continue;                                    /* falls into arm 5 */
      case 5u:  /* arm 5 (0x89B16): the shared-timeline gate */
        if ((int32_t)mr->state.tick_total < 0x3C) return FIFA96_OK; /* 0x89B21 */
        if ((int32_t)mr->state.tick_total < 0x78) {
          /* 0x89B2C CALL 0x45001 = `byte[0x1067EC] & 0xF0` (fresh 20-insn body).
           * The cell has no engine home (the FU184-L3/FU188-L2 sibling leg), so
           * the derived predicate is false and this middle window holds; the
           * >= 0x78 arm below still advances. */
          return FIFA96_OK;                          /* 0x89B33 JZ 0x89EA6 */
        }
        mr->act.timer = 0;                           /* 0x89B43 */
        mr->act.stage = 6u;                          /* 0x89B4A */
        continue;                                    /* falls into arm 6 */
      case 6u: {  /* arm 6 (0x89B50): the code/ball gate */
        struct fifa96_match_entity *actor = match_1d_actor(mr);
        if (actor == NULL) return FIFA96_OK;         /* derived guard */
        /* 0x89B50..0x89B5D: `dword[actor+0x8E]>>24` = byte[actor+0x91] (code). */
        if (actor->code == 0x20u) return FIFA96_OK;  /* 0x89B60 JZ 0x89EA6 */
        /* 0x89B66..0x89B6E: `byte[ballrec+0x44] == 0` waits. The native ball
         * record is the record-11 spill slot `[[actor]+0x7A6]` (team base +
         * 11*0xB2); the pool models records 0..10 only (FU-142 OL-49) and has no
         * `+0x44` cell for it, so the derived value is ready (the row-0x1F
         * installer `0x899CD` that would produce it is the FU194-L8 leg). */
        /* 0x89B74 CALL 0x4C374(0x1A): `word[0x157A68] = 0`, a cell with no
         * reader (FU194-L3/L4) -> derived no-op. */
        mr->act.timer = 0;                           /* 0x89B88 */
        mr->act.stage = 7u;                          /* 0x89B8E */
        continue;                                    /* falls into arm 7 */
      }
      case 7u:  /* arm 7 (0x89B94): the 0x4BEC8 wait */
        /* Fresh `disassemble_function 0x4BEC8` = `XOR EAX,EAX; RET` (2 insns):
         * the wait gate is always ready. */
        mr->act.timer = 0;                           /* 0x89BAA */
        mr->act.stage = 8u;                          /* 0x89BB0 */
        continue;                                    /* falls into arm 8 */
      case 8u: {  /* arm 8 (0x89BB6..0x89E6D): the selection block */
        /* FU-194 §7 (FU194-L7): the substitution-strip ranking block. Its state
         * is the native 0x1587D4 block, which the engine keeps in `render.sub`
         * (FU-152 §2.6/§3.2): `record[]` = bytes [0x1587D4..0x1587E5],
         * `current_side` = byte[0x1587E6], `cursor_raw` = byte[0x1587E7],
         * `special` = byte[0x1587E8], `rank_side` = byte[0x1587E9],
         * `flags[i*2+side]` = byte[0x1587DA + i*2 + side]. */
        struct fifa96_match_run_sub *sub = &mr->render.sub;
        struct fifa96_match_entity *actor = match_1d_actor(mr);
        int32_t winner = -1;                         /* EDI */
        uint8_t side;                                /* byte[actorTeam+0x826] */
        uint8_t sel = sub->cursor_raw;               /* byte[0x1587E7] */
        if (actor == NULL) return FIFA96_OK;         /* derived guard */
        side = (uint8_t)(mr->entities
                             .team[(uint32_t)mr->actor_1587d4 /
                                   FIFA96_MATCH_ENTITY_RECORDS]
                             .side &
                         1u);
        sub->rank_side = 0xFFu;                      /* 0x89BC5 [0x1587E9]=0xFF */
        if ((int8_t)sel >= 5) {
          /* 0x89CA8: the cursor >= 5 arm. */
          if (side != sub->current_side) {
            int8_t p1 = (int8_t)sub->record[0x10];   /* byte[0x1587E4] */
            int8_t p2 = (int8_t)sub->record[0x11];   /* byte[0x1587E5] */
            if (p1 > p2) winner = 0;                 /* 0x89CD2 */
            else if (p2 > p1) winner = 1;            /* 0x89CDA */
            if (winner < 0) {                        /* 0x89CE2 JGE inverse */
              sub->record[0x11] = 0u;                /* 0x89CE6 */
              sub->record[0x10] = 0u;                /* 0x89CEC */
            }
          } else {
            /* 0x89CF4: byte[0x1587E4 + current_side] picks the output side (the
             * modeled domain of current_side is a side 0/1). */
            sub->rank_side = (sub->record[0x10u + (sub->current_side & 1u)] != 0u)
                                 ? side
                                 : (uint8_t)(side ^ 1u);
          }
        } else {
          /* 0x89BD4..0x89C1A: the per-side rank words. rank[side] = 4-sel and
           * rank[side^1] = the same, +1 when the actor's side is the current
           * strip side. */
          int16_t rank[2];
          int16_t base = (int16_t)(4 - (int32_t)(int8_t)sel);
          int32_t a, b;
          rank[side] = base;                         /* word[ESP + side*2] */
          rank[side ^ 1u] = base;                    /* word[ESP + (side^1)*2] */
          if (side == sub->current_side)             /* 0x89C12 CMP AX,BX */
            rank[side ^ 1u] = (int16_t)(base + 1);   /* 0x89C19 INC */
          /* 0x89C1E..0x89C55: the signed rank compares on the two strip numbers
           * byte[0x1587D8+side] (the FUN_0004BDF8 pair, FU-152 §2.6). */
          a = (int8_t)sub->record[4u + (side ^ 1u)];
          b = (int8_t)sub->record[4u + side];
          if (b > a + (int32_t)rank[side ^ 1u]) winner = side;       /* 0x89C44 */
          else if (a > b + (int32_t)rank[side]) winner = side ^ 1u;  /* 0x89C55 */
          /* 0x89C57..0x89CA3: the byte[0x1587E9] output-side pick. The cell is
           * write-only (fresh `get_xrefs_to 0x1587E9` = 4 WRITE, 0 READ). */
          if (a + 1 > b + (int32_t)rank[side])             /* 0x89C86 */
            sub->rank_side = (uint8_t)(side ^ 1u);
          else if (b > a + (int32_t)rank[side ^ 1u] - 1)   /* 0x89D0D */
            sub->rank_side = side;
        }
        /* 0x89D12..0x89D77: the cursor rotation, only when the actor's side
         * differs from the strip's current side. */
        if (side != sub->current_side) {
          uint8_t next = (uint8_t)(sub->cursor_raw + 1u); /* 0x89D2C INC */
          if ((int8_t)next % 5 == 0) {                    /* 0x89D45 IDIV */
            next = 5u;                                    /* 0x89D4F */
            for (uint32_t i = 0; i < 5u; i++) {           /* 0x89D59..0x89D77 */
              sub->flags[i * 2u] = 0u;
              sub->flags[i * 2u + 1u] = 0u;
            }
          }
          sub->cursor_raw = next;
        }
        if (winner < 0) {
          /* 0x89D7E..0x89DDD: the no-winner reset. `FUN_000897F4(side,
           * (int8)[actor+0x8D] - 1)` rebinds the side's free record (0x89832),
           * then `[0x1587D4] = [0x1587CC + (side^1)*4]` (0x89DCF), stage 0 and
           * timer 0. */
          int32_t start = (int32_t)(int8_t)actor->active - 1; /* 0x89D82/0x89D8A */
          mr->actor_1587cc[side] = match_run_free_record(mr, side, start);
          mr->actor_1587d4 = mr->actor_1587cc[side ^ 1u];     /* 0x89DCF */
          mr->act.stage = 0u;                                 /* 0x89DBB */
          mr->act.timer = 0;                                  /* 0x89DC8 */
          return FIFA96_OK;                                   /* 0x89DDD RET */
        }
        /* 0x89DDE..0x89E00: the winner. `FUN_000741B4(x)` = `(x ^ [0x157ABE]) &
         * 1`; the native writes the score pair `word[0x157AC5 + 2*idx] = 1` and
         * `word[0x157AC5 + 2*(idx^1)] = 0` (the FU-146 score words). */
        {
          uint32_t idx = ((uint32_t)winner ^ mr->side_swap) & 1u;
          mr->score[idx] = 1u;
          mr->score[idx ^ 1u] = 0u;
        }
        /* 0x89E08..0x89E1A: `[0x1587E8] = (actor record == [0x157A9F])`. */
        sub->special = (mr->actor_1587d4 == mr->goal_record) ? 1u : 0u;
        /* 0x89E43: `0x8F188(EAX=0xBA, EBX=0x20, EDX=actor record when
         * [0x1587D4]==[0x157A9F], else the winner team base)`. The event-ring
         * sink is unported (OL-27); the id is recorded. */
        mr->match1d_event_id = 0xBAu;
        /* 0x89E4D CALL 0x4C374(7): the word[0x157A68] clear (FU194-L3, no
         * reader) -> derived no-op. */
        mr->act.timer = 0;                           /* 0x89E61 */
        mr->act.stage = 9u;                          /* 0x89E68 */
        continue;                                    /* falls into arm 9 */
      }
      case 9u:  /* arm 9 (0x89E6E): the terminal (no situation re-dispatch) */
        mr->global_157a4c = 0u;                      /* [0x157A4C]=0 */
        mr->global_157ac0 = 1u;                      /* 0x89E8C [0x157AC0]=1 */
        mr->act.id = 0;                              /* [0x158828] */
        mr->act.stage = 0;                           /* [0x158829] */
        mr->act.timer = 0;                           /* [0x158818] */
        return FIFA96_OK;
      default:
        return FIFA96_OK;                            /* stage > 9: 0x898A2 JA 0x89EA6 */
    }
  }
}

/* FU-241 §6.3 (M2 phase-23 T4; formerly FU-194 §4/§6.2): the phase-0x1E / act-8
 * row body — the native 2-team x 11-record restart sweep `0x8A798` (stages
 * 0/1/2). Its phase-table slot is now REGISTERED/COUNTED (the de-compression):
 * `fifa96_match_run_set_piece`'s BX!=0 branch dispatches act 8 invoke-now, whose
 * clock-tail tick drives this row; the former compressed in-frame re-dispatch is
 * retired. FU-241 §6.1/§6.2 proves the sweep render-invisible and dormant on M2
 * (no state=3), so no presented frame moves. Head: no timer add (the native head
 * reads the stage directly,
 * 0x8A79B). Stage 0 (0x8A7B2) resets the walk cursor and falls into stage 1;
 * stage 1 (0x8A7D6) sweeps one record per call and, on a record with
 * `code [rec+0x91] == 0xC`, `byte[rec+0x99] != 0`, `byte[rec+0x9A] == 0`,
 * stores it in `act8_selected` (0x8A831) — the `0x4C374` `[0x157A68]` clear is
 * the FU194-L3 erratum leg — advancing to stage 2 after both teams; stage 2
 * (0x8A895) clears the act slot and re-dispatches the stored situation/side
 * (`store_15882b`/`store_15882c`, BX=0) then clears `store_15882b`. The
 * `0x4BEC8` wait gates are derived ready (the referee leg-10 precedent).
 * Returns FIFA96_OK, or -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_action_phase_1E(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  if (mr->act.stage == 0u) {                       /* 0x8A7B2: reset + fall through */
    mr->act8_team = 0u;                            /* byte[0x15882E] */
    mr->act8_record = 0u;                          /* byte[0x15882F] */
    mr->act.timer = 0;                             /* word[0x158818]=0 */
    mr->act.stage = 1u;
  }
  if (mr->act.stage == 1u) {                       /* 0x8A7D6: the record sweep */
    uint32_t t, i;
    struct fifa96_match_entity *rec;
    if (mr->act8_team >= FIFA96_MATCH_ENTITY_TEAMS) {
      mr->act.timer = 0;
      mr->act.stage = 2u;                          /* 0x8A87A..0x8A88B */
      return FIFA96_OK;
    }
    t = mr->act8_team;
    i = mr->act8_record;
    if (i < FIFA96_MATCH_ENTITY_RECORDS) {
      rec = &mr->entities.team[t].records[i];
      mr->act8_selected = NULL;                    /* 0x8A814: clear before every gate test */
      if (rec->code == 0x0Cu && rec->skip_99 != 0u && rec->skip_9a == 0u) {
        mr->act8_selected = rec;                   /* [0x1587F0] = rec (0x8A831) */
        /* MOV EAX,0x19; CALL 0x4C374 = the word[0x157A68] clear (FU194-L3
         * erratum); no engine cell -> leg. */
      }
      mr->act8_record = (uint8_t)(i + 1u);         /* 0x8A840 */
      if (mr->act8_record < FIFA96_MATCH_ENTITY_RECORDS) return FIFA96_OK;
    }
    mr->act8_team = (uint8_t)(t + 1u);             /* 0x8A857 */
    mr->act8_record = 0u;
    if (mr->act8_team <= 1u) return FIFA96_OK;     /* 0x8A878 JLE */
    mr->act.timer = 0;
    mr->act.stage = 2u;                            /* 0x8A87A..0x8A88B */
    return FIFA96_OK;
  }
  if (mr->act.stage == 2u) {                       /* 0x8A895: the terminal */
    uint8_t side = mr->store_15882c;               /* 0x8A8BD byte[0x15882C] */
    uint8_t sit = mr->store_15882b;                /* 0x8A8C3 byte[0x15882B] */
    int rc;
    mr->act.timer = 0;                             /* 0x8A89F */
    mr->act.id = 0;                                /* 0x8A8AB [0x158808]=0 */
    mr->act.stage = 0;                             /* 0x8A8B1 [0x158829]=0 */
    /* 0x8A8B7 byte[0x158828]=0 (the act id, same as id=0); 0x8A8CE the
     * stored-situation re-dispatch (BX=0); 0x8A8D3 store_15882b = 0xFF. */
    rc = fifa96_match_run_set_piece(mr, sit, side, 0u);
    mr->store_15882b = 0xFFu;
    return rc;
  }
  return FIFA96_OK;
}

/* FU-195 §2/§6.1 (M2 phase-17 T6): the phase-0x21 / act-0xB row body — the
 * native extra-time-CLEAR period-end timer `0x8B874` (act slot `0x1107EC[0xB]`).
 * This is the ONE counted row of T6 (the producer `0x8BAE7` is live via
 * `phase_drive` -> `out.act = 0x0B` when the extra-time flag is clear; the flag
 * is live since T1/FU-213, so `out.act` may instead be `0x0C` on the
 * extra-time arm) -> `fifa96_match_act_dispatch`). Raw disasm
 * (no Ghidra function: table-only target): head `word[0x158818] +=
 * word[0x157A64]` (`0x8B884`); stage 0 `0x8B89E` fires the `0x974DC(0x1E)`
 * event, resets the timer (`0x8B8B2`) and advances (`0x8B8B9`); stage 1
 * `0x8B8C3` waits on `word[0x15881A] >= 0x3C` (alias trap `[0x158816]>>16`; the
 * derived model gates on the act timer, FU195-L4), then fires the event and
 * clears the act slot (`0x8B8DE..0x8B8F3`); stage > 1 is the `0x8B89A` POP/RET
 * no-op. The `0x974DC(0x1E)` audio sink is staged as `mr->ref_whistle` (the
 * sibling phase-1C/1D convention; the ring-push/audio-handle remainder is
 * FU195-L3). No record, no phase write. Returns FIFA96_OK, or
 * -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_action_phase_21(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  mr->act.timer = (int16_t)(mr->act.timer + (int16_t)mr->state.frame_delta); /* 0x8B884 */
  if (mr->act.stage == 0u) {                       /* 0x8B89E */
    match_run_whistle_event(mr);                   /* 0x974DC(0x1E) */
    mr->act.timer = 0;                             /* 0x8B8B2 */
    mr->act.stage = 1u;                            /* 0x8B8B9 */
    return FIFA96_OK;
  }
  if (mr->act.stage == 1u) {                       /* 0x8B8C3 */
    if (mr->act.timer < 0x3C) return FIFA96_OK;    /* the word[0x15881A] gate (FU195-L4) */
    match_run_whistle_event(mr);                   /* 0x974DC(0x1E) */
    mr->act.id = 0;                                /* 0x8B8ED [0x158808]/[0x158828] */
    mr->act.stage = 0;                             /* 0x8B8F3 */
    mr->act.timer = 0;                             /* 0x8B8DE */
    return FIFA96_OK;
  }
  return FIFA96_OK;                                /* stage > 1: 0x8B89A POP/RET */
}

/* FU-195 §3/§6.1/§7 (M2 phase-17 T6; updated by M2 phase-19 T1/FU-213): the
 * phase-0x22 / act-0xC row body — the
 * native extra-time-SET analogue `0x8B900` (act slot `0x1107EC[0xC]`), the
 * three-stage form of `phase_21`. Three-way live since T4/FU-211: the OL-85 flag
 * `[0x157AC0]` producer LANDED in T1 (`mr->global_157ac0`, passed live by
 * `phase_drive`, so the chooser yields act 0xC; the former producer-dormancy leg
 * FU195-L2 is RESOLVED) and T4 registers the phase-0x22 table slot
 * (`fifa96_match_handlers.c`), so the act dispatch reaches this body. Raw disasm: head `word[0x158818] += word[0x157A64]` (`0x8B911`);
 * stage 0 (`0x8B92E`) event + timer 0 + stage 1; stage 1 (`0x8B95C`) waits on
 * `word[0x15881A] >= 0x3C` (derived: the act timer, FU195-L4), then event +
 * timer 0 + stage 2; stage 2 (`0x8B98F`) waits the same gate, then event and
 * clears the act slot (`0x8B9AA..0x8B9BF`); stage > 2 is the `0x8B929` RET.
 * Returns FIFA96_OK, or -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_action_phase_22(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  mr->act.timer = (int16_t)(mr->act.timer + (int16_t)mr->state.frame_delta); /* 0x8B911 */
  if (mr->act.stage == 0u) {                       /* 0x8B92E */
    match_run_whistle_event(mr);                   /* 0x974DC(0x1E) */
    mr->act.timer = 0;                             /* 0x8B951 */
    mr->act.stage = 1u;
    return FIFA96_OK;
  }
  if (mr->act.stage == 1u) {                       /* 0x8B95C */
    if (mr->act.timer < 0x3C) return FIFA96_OK;    /* the word[0x15881A] gate (FU195-L4) */
    match_run_whistle_event(mr);                   /* 0x974DC(0x1E) */
    mr->act.timer = 0;                             /* 0x8B98E */
    mr->act.stage = 2u;
    return FIFA96_OK;
  }
  if (mr->act.stage == 2u) {                       /* 0x8B98F */
    if (mr->act.timer < 0x3C) return FIFA96_OK;    /* the word[0x15881A] gate (FU195-L4) */
    match_run_whistle_event(mr);                   /* 0x974DC(0x1E) */
    mr->act.timer = 0;                             /* 0x8B9AA */
    mr->act.id = 0;                                /* 0x8B9B3 */
    mr->act.stage = 0;                             /* 0x8B9BF */
    return FIFA96_OK;
  }
  return FIFA96_OK;                                /* stage > 2: 0x8B929 RET */
}

int fifa96_match_run_contact(struct fifa96_match_run *mr, uint8_t kind,
                             int32_t rec_a, int32_t rec_b, const int32_t point[3]) {
  struct fifa96_ref_record fouler;
  struct fifa96_ref_record victim;
  struct fifa96_ref_decision_out out;
  uint16_t draw = 0;
  int rc;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!match_run_ref_view(mr, rec_a, &fouler)) return -FIFA96_ERR_INVALID;
  if (!match_run_ref_view(mr, rec_b, &victim)) memset(&victim, 0, sizeof victim);
  rc = fifa96_ref_contact_register(&mr->referee, kind, rec_a,
                                   rec_b >= 0 ? rec_b : 0, point);
  if (rc != 0) return rc;
  mr->referee.phase = mr->state.phase;
  if (mr->config.field_4c306 == 0) {          /* 0x81E8A settings gate */
    mr->referee.recall_consumed = 0;
    return 0;
  }
  (void)fifa96_rng_step(&mr->rng, &draw);     /* 0x81EA6: the 1-in-8 skip */
  if ((draw & 7u) == 0u) {
    mr->referee.recall_consumed = 0;          /* 0x81ECD */
    return 0;
  }
  /* The row-0x0C re-call literal kind 1 (0x81EB4) and its severity draw
   * (FUN_00092AC8; consumed only when severity can run). */
  mr->referee.contact_kind = 1u;
  if (mr->config.field_4c306 > 1) (void)fifa96_rng_step(&mr->rng, &draw);
  memset(&out, 0, sizeof out);
  rc = fifa96_ref_foul_decide(&mr->referee, &mr->config, &fouler, &victim, point,
                              (uint8_t)draw, &out);
  if (rc < 0) return rc;
  mr->referee.recall_consumed = 1;            /* 0x81EC4 */
  mr->incident_x = mr->referee.point[0];      /* [0x158897] mirror */
  mr->incident_y = mr->referee.point[1];      /* [0x15889B] mirror */
  mr->incident_z = mr->referee.point[2];      /* [0x15889F] mirror */
  if (out.sequence == FIFA96_REF_SEQ_ACT3) {
    /* FU-187 §5 (phase-16 T4): the act-3 producer routes through the act seam
     * — the native `0x8A6FE FUN_000888FC(3,0,1)`. The phase-0x19 body runs
     * stage 0 invoke-now (install action 0x16 on the fouler + phase 0xF) and
     * the clock-tail tick advances it; the former ad-hoc FOUL machine (the
     * FU175-L7 double-run) is retired. */
    mr->referee.sequence = FIFA96_REF_SEQ_ACT3;
    rc = fifa96_match_act_dispatch(mr, 3, 0, 1);
    if (rc != 0) return rc;
    return 1;
  }
  if (out.restart != 0u) {
    /* 0x8A729: FUN_0008A938(9, fouled side, BX=1) through the P1 shared entry
     * (whose direct path may start the act-2 machine). */
    rc = fifa96_match_run_set_piece(mr, 9u, out.restart_side, 1u);
    if (rc != 0) return rc;
    return 1;
  }
  return 0;
}

/* Phase 70: the receive-arm entry with the native `0x157770` reception query
 * point threaded in. The public wrapper below keeps the ball-based behavior
 * for the direct geometry tests; the live receive arm passes the target that
 * `fifa96_match_run_ball_receive` already computes (the engine's 0x157770
 * stand-in). The own-nearest query is the piece that made a live pass miss:
 * at the synchronous receive the ball is still at the passer, so the native
 * target — not the ball — is the receiver-scan anchor. */
static int match_run_offside_reception_at(struct fifa96_match_run *mr,
                                          int32_t receiver,
                                          int32_t query_x, int32_t query_z,
                                          const struct fifa96_ref_metric *metric,
                                          int32_t camera_ref, uint8_t mirror,
                                          uint8_t *offside) {
  fifa96_entity_candidate candidates[FIFA96_MATCH_ENTITY_RECORDS];
  struct fifa96_ref_receiver rec;
  struct fifa96_ref_record event_rec;
  struct fifa96_ref_event_out ev_out;
  const struct fifa96_match_entity *own;
  int32_t rec_team, other_team, own_idx = -1, own_id = -1, last_defender_z = 0;
  int16_t best = 0;
  uint16_t draw = 0;
  int idx, rc;
  uint8_t result = 0;
  if (!mr || !metric || !offside) return -FIFA96_ERR_INVALID;
  *offside = 0;
  if (receiver < 0 ||
      receiver >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  rec_team = receiver / FIFA96_MATCH_ENTITY_RECORDS;
  other_team = rec_team ^ 1;
  /* The native FUN_00079D5C pre-gates (0x79D68/0x79D79/0x79D87) run before
   * the RNG draw; keep the same order so no draw is consumed on a gate. */
  mr->referee.phase = mr->state.phase;
  if (mr->state.phase != 2u) return 0;
  if (mr->referee.offside_suppress != 0) return 0;
  if (mr->config.field_4c2f2 == 0) return 0;
  memset(&rec, 0, sizeof rec);
  rec.z = mr->entities.team[rec_team].records[receiver % FIFA96_MATCH_ENTITY_RECORDS].pos_z;
  rec.side = mr->entities.team[rec_team].side;
  /* The derived FUN_0008DE8C(&0x157770, own team, 0): the own-team nearest to
   * the reception target (the derived stand-in for 0x157770, FU-150 leg 8),
   * skip index 0. The live receive arm passes its computed target; the direct
   * wrapper passes the ball. */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[rec_team].records[i];
    candidates[i].x = (int16_t)e->pos_x;
    candidates[i].y = (int16_t)e->pos_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  idx = fifa96_entity_find_nearest(candidates, FIFA96_MATCH_ENTITY_RECORDS, 0,
                                   (int16_t)query_x, (int16_t)query_z, &best);
  if (idx >= 0) {
    own = &mr->entities.team[rec_team].records[idx];
    own_idx = idx;
    own_id = (int32_t)(rec_team * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)idx);
    rec.own_nearest_valid = 1;
    rec.own_nearest_is_receiver = idx == (int)(receiver % FIFA96_MATCH_ENTITY_RECORDS);
    rec.own_nearest_z = own->pos_z;
    rec.own_distance = best;
    /* the 0x79EEF..0x79F1D record-state gate (the [0x158777] identity is
     * staged 0/unported). */
    rec.eligible = (own->code != 0x11u && own->code != 0x10u &&
                    own->code != 0x1Du && own->code != 0x1Eu)
                       ? 1u
                       : 0u;
  }
  /* The derived FUN_0008DE28(0xB10|-0xB10, other team, 0): the opponent
   * nearest to (0, +0xB10) for a side-0 receiver and (0, -0xB10) for side 1. */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[other_team].records[i];
    candidates[i].x = (int16_t)e->pos_x;
    candidates[i].y = (int16_t)e->pos_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  idx = fifa96_entity_find_nearest(
      candidates, FIFA96_MATCH_ENTITY_RECORDS, 0, 0,
      rec.side == 0u ? (int16_t)0xB10 : (int16_t)-0xB10, &best);
  if (idx >= 0)
    last_defender_z = mr->entities.team[other_team].records[idx].pos_z;
  (void)fifa96_rng_step(&mr->rng, &draw);     /* the tolerance draw */
  rc = fifa96_ref_offside_check(&mr->referee, &mr->config, &rec, metric,
                                last_defender_z, camera_ref, mirror, (uint8_t)draw,
                                &result);
  if (rc != 0) return rc;
  if (result == 0u) return 0;
  if (own_id < 0) return 0;   /* the check requires a valid own-nearest */
  /* The kind-3 event fires on the own-team nearest (first-hand 0x79F29
   * `MOV EDX,ESI`, the slice's receiver reading is the recorded erratum). */
  memset(&event_rec, 0, sizeof event_rec);
  event_rec.id = own_id;
  event_rec.side = mr->entities.team[rec_team].side;
  event_rec.player = mr->entities.team[rec_team].records[own_idx].index;
  event_rec.active = mr->entities.team[rec_team].records[own_idx].active;
  event_rec.duel_ok = 1;
  {
    int32_t point[3];
    point[0] = mr->entities.team[rec_team].records[own_idx].pos_x;
    point[1] = mr->entities.team[rec_team].records[own_idx].pos_y;
    point[2] = mr->entities.team[rec_team].records[own_idx].pos_z;
    rc = fifa96_ref_offside_event(&mr->referee, &mr->config, &event_rec, point,
                                  &ev_out);
    if (rc < 0) return rc;
  }
  mr->incident_x = mr->referee.point[0];
  mr->incident_y = mr->referee.point[1];
  mr->incident_z = mr->referee.point[2];
  /* phase-36 R9: the offside machine is the act-6 producer; dispatch the row
   * invoke-now (the native 0x8A784/0x8A789 kind-3 arm). The retired direct
   * `ref_machine`/`referee_step` route would double-run the same sequence. */
  rc = fifa96_match_act_dispatch(mr, 6, 0, 1);  /* FUN_000888FC(6,..,1) */
  if (rc < 0) return rc;
  /* The native 0x8F188(0x15) request (0x8A743) precedes the act-6 invocation
   * and the step's head clears the request slots, so re-assert it after the
   * synchronous stage-0 call (the observation slot otherwise loses it). */
  mr->ref_speech = ev_out.speech_code;
  *offside = 1;
  return 1;
}

int fifa96_match_run_offside_reception(struct fifa96_match_run *mr, int32_t receiver,
                                       const struct fifa96_ref_metric *metric,
                                       int32_t camera_ref, uint8_t mirror,
                                       uint8_t *offside) {
  if (!mr) return -FIFA96_ERR_INVALID;
  return match_run_offside_reception_at(mr, receiver, mr->entities.ball.x,
                                        mr->entities.ball.z, metric,
                                        camera_ref, mirror, offside);
}

int fifa96_match_run_referee_step(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  mr->ref_whistle = 0;
  mr->ref_speech = 0;
  mr->referee.phase = mr->state.phase;
  mr->referee.delta = mr->state.frame_delta;
  /* The [0x157A6A] countdown (0x7438D..0x743A5, `SUB word, AX` with the frame
   * delta; the port saturates instead of wrapping). */
  if (mr->referee.offside_suppress != 0u) {
    uint16_t d = mr->state.frame_delta;
    if (d >= mr->referee.offside_suppress) mr->referee.offside_suppress = 0;
    else mr->referee.offside_suppress =
        (uint16_t)(mr->referee.offside_suppress - d);
  }
  /* phase-36 R9: the act-6 phase row owns the offside sequence; the former
   * REF_OFFSIDE arm here was the duplicate driver and is retired. */
  return 0;
}
