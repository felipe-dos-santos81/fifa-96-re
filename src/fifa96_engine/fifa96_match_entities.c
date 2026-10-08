/* src/fifa96_engine/fifa96_match_entities.c — M2 Task 8 / FU-141: the
 * entity/ball pool and the FU-67 per-frame update chain. Evidence and open
 * legs: docs/ghidra/FU141_action_cluster_de.md (read-only /FIFA96.EXE). */
#include <string.h>

#include "fifa96_engine/fifa96_match_entities.h"
#include "fifa96_loader/fifa96_arm_helpers.h"
#include "fifa96_loader/fifa96_entity_update.h"

#define ENTITY_ID(pool, team, index) \
  ((int32_t)((uint32_t)(team) * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)(index)))

static struct fifa96_match_entity *entity_by_id(struct fifa96_match_entities *pool,
                                                int32_t id) {
  if (id < 0 || id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return NULL;
  return &pool->team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS]
             .records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
}

static int32_t entity_abs(int32_t value) {
  return value < 0 ? -value : value;
}

int fifa96_match_entities_init(struct fifa96_match_entities *pool) {
  if (!pool) return -FIFA96_ERR_INVALID;
  memset(pool, 0, sizeof *pool);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    struct fifa96_match_team *team = &pool->team[t];
    team->side = (uint8_t)t;
    team->target = FIFA96_MATCH_ENTITY_NONE;
    team->second = FIFA96_MATCH_ENTITY_NONE;
    team->chosen = FIFA96_MATCH_ENTITY_NONE;
    team->intercept = FIFA96_MATCH_ENTITY_NONE;
    team->chosen831 = FIFA96_MATCH_ENTITY_NONE;
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      struct fifa96_match_entity *e = &team->records[i];
      e->team = (uint8_t)t;
      e->index = (uint8_t)i;
      e->stage92 = 0xFF; /* FUN_0007DAB4 0x7DABA */
      e->code = 0;       /* FUN_0007DAB4 0x7DAFB installs action 0 */
    }
  }
  pool->controlled = FIFA96_MATCH_ENTITY_NONE;
  pool->slot_merge = FIFA96_MATCH_ENTITY_NONE;
  pool->ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  return FIFA96_OK;
}

int fifa96_match_entities_release(struct fifa96_match_entities *pool) {
  if (!pool) return -FIFA96_ERR_INVALID;
  memset(pool, 0, sizeof *pool);
  return FIFA96_OK;
}

int fifa96_match_entities_install(struct fifa96_match_entity *entity, uint8_t phase,
                                  uint8_t code, uint8_t staged) {
  if (!entity) return -FIFA96_ERR_INVALID;
  if (entity->skip_9a != 0) return 0;                       /* 0x7D9BE */
  if ((int8_t)entity->code == (int8_t)code) return 0;       /* 0x7D9D6 */
  if (entity->skip_98 != 0 && code != 0x0Cu && phase != 2u && phase != 0x0Au &&
      phase != 0x0Fu) {
    /* The native FUN_0006E598(rec, 0x0F, type8, 0) animation select is
     * unported (FU-141 leg); the derived +0x98 clear is kept. */
    entity->skip_98 = 0;
  }
  if (entity->active == 0 && code == 3u) code = 0x19u;      /* 0x7DA26 */
  if (code == 5u || code == 0x21u) entity->carrier |= 1u;   /* 0x7DA42 */
  else entity->carrier &= (uint8_t)~1u;
  entity->code = code;                                      /* 0x7DA63 */
  entity->timer89 = 0;                                      /* 0x7DA7E */
  entity->ran = 0;                                          /* 0x7DA88 */
  entity->stage92 = staged;                                 /* 0x7DA95 */
  entity->timer7b = entity->timer79;                        /* 0x7DA9B */
  return 1;
}

static void entity_drain(struct fifa96_match_entities *pool,
                         struct fifa96_match_entity *e) {
  if (e->install != 0) {
    (void)fifa96_match_entities_install(e, pool->phase, e->install, 0);
    e->install = 0;
  }
  if (e->helper_request != 0) {
    (void)fifa96_match_entities_merge_slot(pool, e->team, e->index);
    e->helper_request = 0;
  }
  if (e->controlled != 0) {
    pool->controlled = ENTITY_ID(pool, e->team, e->index);
    e->controlled = 0;
  }
  if (e->place_valid != 0) {
    pool->place_pending = 1;
    pool->place_x = e->place_x;
    pool->place_y = e->place_y;
    pool->place_z = e->place_z;
    e->place_valid = 0;
  }
}

int fifa96_match_entities_team_select(struct fifa96_match_entities *pool, uint32_t team,
                                      int16_t target_x, int16_t target_y) {
  fifa96_entity_candidate candidates[FIFA96_MATCH_ENTITY_RECORDS];
  int16_t best = 0;
  int index;
  if (!pool || team >= FIFA96_MATCH_ENTITY_TEAMS) return -FIFA96_ERR_INVALID;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &pool->team[team].records[i];
    candidates[i].x = (int16_t)e->pos_x;
    candidates[i].y = (int16_t)e->pos_z; /* record +0x61, the search's Y */
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  /* FUN_0008D8EC 0x8D9AA calls FUN_0008DE8C(skip 0). */
  index = fifa96_entity_find_nearest(candidates, FIFA96_MATCH_ENTITY_RECORDS, 0,
                                     target_x, target_y, &best);
  pool->team[team].target =
      index < 0 ? FIFA96_MATCH_ENTITY_NONE : ENTITY_ID(pool, team, index);
  return index;
}

int fifa96_match_entities_merge_slot(struct fifa96_match_entities *pool, uint32_t team,
                                     uint32_t record) {
  struct fifa96_match_team *t;
  struct fifa96_match_entity *requester;
  int best = -1;
  int16_t best_distance = 0;
  uint32_t slot_holders = 0;
  if (!pool || team >= FIFA96_MATCH_ENTITY_TEAMS ||
      record >= FIFA96_MATCH_ENTITY_RECORDS)
    return -FIFA96_ERR_INVALID;
  t = &pool->team[team];
  requester = &t->records[record];
  if (requester->has_slot != 0) return 0;                    /* 0x7877D */
  if (t->slot_pool == 0) return 0;                           /* 0x78789 */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *candidate = &t->records[i];
    int32_t distance;
    if (candidate->has_slot == 0) continue;                  /* 0x787A3/A7 */
    distance = (int32_t)(int16_t)fifa96_entity_distance(
        (int16_t)(requester->pos_x - candidate->pos_x),
        (int16_t)(requester->pos_z - candidate->pos_z));
    /* The native counter (`INC ECX` at 0x787E4) increments only for
     * slot-holding candidates; `TEST CX,CX` (0x787C6) therefore takes the
     * *first slot-holder* unconditionally, and later holders only when their
     * signed-word distance is strictly greater. */
    if (slot_holders == 0 || (int16_t)distance > best_distance) {
      best = (int)i;
      best_distance = (int16_t)distance;
    }
    slot_holders++;
  }
  if (best < 0) return 0;
  requester->has_slot = 1;
  t->records[best].has_slot = 0;
  pool->slot_merge = ENTITY_ID(pool, team, record);
  return 1;
}

int32_t fifa96_match_entities_take_slot_merge(struct fifa96_match_entities *pool) {
  int32_t merged;
  if (!pool) return FIFA96_MATCH_ENTITY_NONE;
  merged = pool->slot_merge;
  pool->slot_merge = FIFA96_MATCH_ENTITY_NONE;
  return merged;
}

int fifa96_match_entities_take_place(struct fifa96_match_entities *pool, int32_t *x,
                                     int32_t *y, int32_t *z) {
  if (!pool || !x || !y || !z) return -FIFA96_ERR_INVALID;
  if (pool->place_pending == 0) return 0;
  *x = pool->place_x;
  *y = pool->place_y;
  *z = pool->place_z;
  pool->place_pending = 0;
  return 1;
}

/* FU-89 §kickoff placement / OL-T11-8: the native setup/restart record commit
 * `FUN_00079B6C` (`0x79B6C..0x79C1C`, single RET at `0x79C1C`; first-hand
 * get_function_by_address 0x79BB5 -> body_end 0x79C1C). Commit block: the
 * position triple takes the target triple (`MOVSD x3` 0x79B77..0x79B79),
 * position.y is zeroed (0x79B7A), the target is then the committed position
 * (`MOVSD x3` 0x79B87..0x79B89), and the words +0x69/+0x67/+0x65/+0x71/
 * +0x73/+0x75 and the byte +0x9C are zeroed (0x79B8A..0x79BB1). The pool's
 * `lane`/`vel_x`/`vel_z` are dwords over those words: `lane` (+0x69/+0x6B)
 * keeps its high word (the native only clears +0x69), while `vel_x` (+0x71/
 * +0x73) and `vel_z` (+0x73/+0x75) are fully cleared by the three velocity
 * word stores. */
int fifa96_match_entities_place(struct fifa96_match_entity *entity) {
  if (!entity) return -FIFA96_ERR_INVALID;
  entity->pos_x = entity->target_x;
  entity->pos_y = entity->target_y;
  entity->pos_z = entity->target_z;
  entity->pos_y = 0;
  entity->target_x = entity->pos_x;
  entity->target_y = entity->pos_y;
  entity->target_z = entity->pos_z;
  entity->lane = (int32_t)((uint32_t)entity->lane & 0xFFFF0000u);
  entity->vel_x = 0;
  entity->vel_z = 0;
  return FIFA96_OK;
}

/* FU-89 §11 / OL-T11-8: the phase-cell seed (`FUN_0006E1D0`) over both teams'
 * records. The formation file is indexed by the record block position (the
 * native `byte[rec+0x8D]` that `FUN_0008C2E0` initializes to the record
 * index), so the loop passes the pool `index` directly. */
int fifa96_match_entities_seed_formation(struct fifa96_match_entities *pool,
                                         const fifa96_scene_formation *formation,
                                         uint8_t controlled_side) {
  if (!pool || !formation) return -FIFA96_ERR_INVALID;
  if (!formation->loaded) return FIFA96_ERR_NOT_FOUND;
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      struct fifa96_match_entity *e = &pool->team[t].records[i];
      fifa96_err_t err = fifa96_scene_formation_place(
          formation, i, (uint8_t)t, controlled_side, &e->target_x, &e->target_y,
          &e->target_z);
      if (err != FIFA96_OK) return (int)err;
    }
  }
  return FIFA96_OK;
}

/* The derived kickoff pass (see the header contract). The native tail of
 * `FUN_00079B6C` (`0x79BB5..0x79C13`) runs per record after the commit:
 *  - `FUN_00079C50(camera - target)` face (0x79BB5..0x79BCF): the ported
 *    `fifa96_arm_face` takes the committed target as the source and the
 *    camera as the destination; a zero delta leaves the +0x8E octant alone;
 *  - the conditional `FUN_0006E48C` write `byte[rec+0x3E] = byte[rec+0x8E]`
 *    (0x79BD4..0x79BEC) targets a byte the pool does not model (row-pointer
 *    and row `+0x44` gates also unmodeled) -> numbered leg;
 *  - the unconditional selector `FUN_0006E598(rec, active ? 0 : 0x26, 0)`
 *    (0x79BF1..0x79C13): `fifa96_arm_anim_select` maps the id (inactive ->
 *    0x26; active -> the keep/fallback rule for id 0) and the frame resolver's
 *    `[rec+0x3D] = 0` request lands on the pool `frame`. */
int fifa96_match_entities_kickoff_place(struct fifa96_match_entities *pool, int32_t cam_x,
                                        int32_t cam_y, int32_t cam_z) {
  if (!pool) return -FIFA96_ERR_INVALID;
  pool->ball.x = FIFA96_MATCH_ENTITY_KICKOFF_BALL_X;
  pool->ball.y = 0;   /* FUN_0008C24C 0x8C299: `[0x158810] = 0` */
  pool->ball.z = 0;
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      struct fifa96_match_entity *e = &pool->team[t].records[i];
      uint8_t slot;
      (void)fifa96_match_entities_place(e);
      {
        fifa96_arm_vec from = { e->target_x, e->target_y, e->target_z };
        fifa96_arm_vec to = { cam_x, cam_y, cam_z };
        uint8_t face = e->type;
        if (fifa96_arm_face(&from, &to, &face) == FIFA96_OK) e->type = face;
      }
      if (fifa96_arm_anim_select(e->active != 0 ? 0u : 0x26u, e->anim_id, &slot) ==
          FIFA96_OK)
        e->anim_id = slot;
      e->frame = 0;   /* FUN_0006E490 with BX = 0 (0x79C13 selector request) */
    }
  }
  return FIFA96_OK;
}

static void entity_timer_decay(struct fifa96_match_entity *e, uint16_t delta) {
  /* The native compares the signed high word of the limit against the
   * zero-extended delta word (0x7CA74, 0x7CAA0). */
  int32_t d = (int32_t)delta;
  if (e->timer81 != 0) {
    int32_t limit = (int16_t)(e->timer7f >> 16);
    if (limit > d) e->timer81 = (uint16_t)(e->timer81 - (uint16_t)delta);
    else e->timer81 = 0;
  }
  if (e->timer93 != 0) {
    if ((int32_t)e->timer93 > d)
      e->timer93 = (uint8_t)(e->timer93 - (uint8_t)delta);
    else e->timer93 = 0;
  }
}

static void team_select_target(struct fifa96_match_entities *pool, uint32_t t,
                               const struct fifa96_match_entities_frame *frame) {
  const int32_t *vector;
  int32_t target_x;
  int32_t target_y;
  if (frame->timer_a < frame->timer_b) {
    vector = frame->select_vector[0];                       /* 0x157788 */
    target_x = vector[0];
    target_y = vector[2];
  } else if (frame->timer_a < frame->timer_c) {
    vector = frame->select_vector[1];                       /* 0x157794 */
    target_x = vector[0];
    target_y = vector[2];
  } else {
    vector = frame->select_vector[2];                       /* 0x157770 */
    target_x = vector[0] + (int32_t)((uint32_t)(frame->lead_x >> 16) << 5);
    target_y = vector[2] + (int32_t)((uint32_t)(frame->lead_z >> 16) << 5);
  }
  (void)fifa96_match_entities_team_select(pool, t, (int16_t)target_x,
                                          (int16_t)target_y);
}

static void team_select_intercept(struct fifa96_match_entities *pool, uint32_t t,
                                  const struct fifa96_match_entities_frame *frame) {
  struct fifa96_match_team *team = &pool->team[t];
  struct fifa96_match_entity *controlled;
  struct fifa96_match_entity *target;
  fifa96_entity_candidate candidates[FIFA96_MATCH_ENTITY_RECORDS];
  uint32_t side;
  int16_t best = 0;
  int index;
  team->intercept = FIFA96_MATCH_ENTITY_NONE;
  if (pool->controlled == FIFA96_MATCH_ENTITY_NONE) return;
  controlled = entity_by_id(pool, pool->controlled);
  if (!controlled) return;
  /* The native compares the two blocks' +0x826 side bytes (0x8D9E3). */
  if (pool->team[controlled->team].side != team->side) return;
  if (entity_abs(frame->cam_z) <= 0x480) return;
  if (((frame->cam_z < 0) ? 1u : 0u) != team->side) return;
  /* `0x8DA2C MOV EBX,[EBP+0x7B2]; MOV EBX,[EBX+0x8A]; SAR EBX,0x18` reads the
   * *team target's* +0x8D byte, not the controlled actor's. */
  target = entity_by_id(pool, team->target);
  if (!target) return;
  side = team->side;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &team->records[i];
    candidates[i].x = (int16_t)e->pos_x;
    candidates[i].y = (int16_t)e->pos_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  /* Skip index = team target[+0x8A]>>24, i.e. its +0x8D byte; the target base
   * is 0x10F37C + side*0xC (0x8DA4D). */
  index = fifa96_entity_find_nearest(
      candidates, FIFA96_MATCH_ENTITY_RECORDS, (int16_t)(int8_t)target->active,
      frame->intercept_x[side], frame->intercept_y[side], &best);
  if (index >= 0 && team->records[index].has_slot != 0)
    index = FIFA96_MATCH_ENTITY_NONE;                       /* 0x8DA5D */
  team->intercept =
      index < 0 ? FIFA96_MATCH_ENTITY_NONE : ENTITY_ID(pool, t, index);
  /* OL-41 (M2 Task 14): the `FUN_0008D824` bind (`0x8DA72..0x8DA7A`, EAX =
   * the controlled actor `[0x157A83]`, EDX = the nearest, EBX = `nearest
   * +0x4D`) writes the nearest record's +0x4D triple from the *actor's*
   * x/z. Then `FUN_000795B4` (`0x8DA7F..0x8DA8F`) runs on (nearest+0x59,
   * nearest+0x4D) with a stack scratch output (`0x8DA88 LEA EBX,[ESP+0xC]`),
   * so the record's target triple is an input only — the band/dx/dz words are
   * never stored back. The band's `0x8DA94` gate and the
   * `0x8DAAE..0x8DAE7` lane/height arm then feed `team+0x7BE`.
   *
   * The slot-rejected path (index NONE) has the native call the helpers with
   * the NULL record (`[team+0x7BA]=0`), reading absolute low-memory words at
   * 0x4D..0x69; those have no derived value, so the derived model stages a
   * zero record there (band 0, dx/dz 0, lane/height 0) and the gate refuses
   * (`0 < 0xF0` but `0 <= |cam_z| + 0x90`). Recorded as OL-71. */
  if (index >= 0) {
    struct fifa96_match_entity *nearest = &team->records[index];
    fifa96_entity_intercept_target bound;
    fifa96_entity_intercept_band_out band;
    if (fifa96_entity_intercept_bind(controlled->pos_x, controlled->pos_z,
                                     pool->team[controlled->team].side,
                                     nearest->pos_x, &bound) == FIFA96_OK) {
      nearest->target_x = bound.x;
      nearest->target_y = bound.y;
      nearest->target_z = bound.z;
    }
    if (fifa96_entity_intercept_band(nearest->pos_x, nearest->pos_z,
                                     nearest->target_x, nearest->target_z,
                                     &band) == FIFA96_OK) {
      if ((int16_t)band.band < 0xF0) {                      /* 0x8DA9B */
        int16_t lane = nearest->lane_x;                     /* word +0x6B */
        int32_t nz = entity_abs(nearest->pos_z);            /* 0x8DAB6 */
        if (lane > 0x1E0 || nz > entity_abs(frame->cam_z) + 0x90) /* 0x8DAAE */
          team->flag7be = 1;                                /* 0x8DAE0 */
      }
    }
  }
}

int fifa96_match_entities_team_update(struct fifa96_match_entities *pool, uint32_t team,
                                      const struct fifa96_match_entities_frame *frame,
                                      fifa96_match_entity_action_fn action, void *ctx) {
  struct fifa96_match_team *t;
  if (!pool || !frame || team >= FIFA96_MATCH_ENTITY_TEAMS) return -FIFA96_ERR_INVALID;
  /* Latch the frame inputs here so a direct team call is safe (the update
   * wrapper sets the same values before the ball pairing reads them). */
  pool->phase = frame->phase;
  pool->delta = frame->delta;
  t = &pool->team[team];
  t->flag7be = 0;                                            /* 0x8D9C5 */
  t->update_count = (uint8_t)(t->update_count + 1);          /* 0x8D8F7 */
  if (t->update_count >= 0xBu) t->update_count = 0;
  if (frame->phase == 2u) {
    if (t->target == FIFA96_MATCH_ENTITY_NONE ||
        pool->controlled == FIFA96_MATCH_ENTITY_NONE) {
      team_select_target(pool, team, frame);
    }
    team_select_intercept(pool, team, frame);
  } else {
    t->intercept = FIFA96_MATCH_ENTITY_NONE;
  }
  if (t->timer7cb != 0) {                                    /* 0x8DAF3 */
    int32_t limit = (int16_t)(t->timer81e >> 16);
    int32_t d = (int32_t)frame->delta;
    if (limit > d) t->timer820 = (uint16_t)(t->timer820 - (uint16_t)frame->delta);
    else {
      t->timer820 = 0;
      t->timer7cb = 0;
    }
  }
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *e = &t->records[i];
    if (i == 0) e->timer7b = e->timer79;                     /* 0x782D0 entry */
    else if (e->skip_9a != 0) continue;                      /* 0x8DB42 */
    entity_timer_decay(e, frame->delta);
    if (action) {
      int rc = action(ctx, e);
      if (rc != FIFA96_OK && rc != -FIFA96_ERR_UNSUPPORTED) return rc;
    }
    entity_drain(pool, e);
  }
  return FIFA96_OK;
}

int fifa96_match_entities_ball_pair(struct fifa96_match_entities *pool) {
  struct fifa96_match_entity *a;
  struct fifa96_match_entity *b;
  fifa96_ball_pair_actor interceptor;
  fifa96_ball_pair_actor opponent;
  fifa96_ball_pair_vector out;
  int rc;
  if (!pool) return -FIFA96_ERR_INVALID;
  if (pool->phase != 2u) return FIFA96_OK;                   /* 0x4B2BF */
  a = entity_by_id(pool, pool->team[0].target);
  b = entity_by_id(pool, pool->team[1].target);
  if (!a || !b) return FIFA96_OK;
  memset(&interceptor, 0, sizeof interceptor);
  memset(&opponent, 0, sizeof opponent);
  interceptor.position.x = (int16_t)a->pos_x;
  interceptor.position.height = (int16_t)a->pos_y;
  interceptor.position.z = (int16_t)a->pos_z;
  interceptor.velocity_x = (int16_t)a->vel_x;
  interceptor.velocity_z = (int16_t)a->vel_z;
  opponent.position.x = (int16_t)b->pos_x;
  opponent.position.height = (int16_t)b->pos_y;
  opponent.position.z = (int16_t)b->pos_z;
  opponent.velocity_x = (int16_t)b->vel_x;
  opponent.velocity_z = (int16_t)b->vel_z;
  rc = fifa96_ball_pair_decide(&interceptor, &opponent, (int16_t)pool->delta, &out);
  if (rc < 0) return rc;
  if (rc == 1) {
    a->target_x = out.x;
    a->target_y = out.height;
    a->target_z = out.z;
  }
  return FIFA96_OK;
}

int fifa96_match_entities_update(struct fifa96_match_entities *pool,
                                 const struct fifa96_match_entities_frame *frame,
                                 fifa96_match_entity_action_fn action, void *ctx) {
  int rc;
  if (!pool || !frame) return -FIFA96_ERR_INVALID;
  pool->phase = frame->phase;
  pool->delta = frame->delta;
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    rc = fifa96_match_entities_team_update(pool, t, frame, action, ctx);
    if (rc != FIFA96_OK) return rc;
  }
  return fifa96_match_entities_ball_pair(pool);
}
