/* tests/test_engine_match_entities.c — M2 Task 8 / FU-141: the derived
 * entity/ball pool and the FU-67 per-frame update chain.
 *
 * The pool mirrors the native 0xB2-stride record pool of the two `0x835`-stride
 * team blocks (`0x1588A4` / `0x1590D9` in /FIFA96.EXE, FU-67 §3), the team
 * fields, the ball record (`0x15880C`, FU-120 §2) and the ball staging block
 * (`0x158730`, FU-139 §3.1). The update chain follows the native frame order:
 * per-team update counter, phase-2 nearest selection (`FUN_0008D8EC`
 * `0x8D929..0x8D9B7`), phase-2 interception selection (`0x8D9BD..0x8DAE9`),
 * team timer decay (`0x8DAF3`), keeper record 0 + outfield records 1..10 in
 * order (`0x8DB2E`/`0x8DB3A`), then the ball/possession pairing
 * (`FUN_0004B100 0x4B2D4` -> `FUN_0007D430`). The action bodies are driven
 * through a callback so a counter fixture can observe the order; the production
 * frame body supplies the FU-137 dispatcher.
 *
 * Drains pinned here: the FU-137 §2 installer `FUN_0007D9A4` consumes
 * `install` and clears `ran`; the FU-87/FU-139 slot merge `FUN_0007876C` +
 * `FUN_00078670` consumes `helper_request`; the row-1E `controlled` request
 * feeds the pool's actor; the row-1E placement triple becomes a take-once
 * camera-place request (`FUN_000700F4`'s first effect, ported by FU-71 as
 * `fifa96_camera_init`). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_asset.h"
#include "fifa96_engine/fifa96_match_entities.h"
#include "fifa96_loader/fifa96_entity_update.h"

#define NONE FIFA96_MATCH_ENTITY_NONE
#define TEAM0 0u
#define TEAM1 1u

static struct fifa96_match_entities_frame zero_frame(void) {
  struct fifa96_match_entities_frame f;
  memset(&f, 0, sizeof f);
  return f;
}

/* init: two team blocks with sides 0/1, every selection pointer unset, the
 * native reset seed `[rec+0x92]=0xFF` (FUN_0007DAB4 0x7DABA), records stamped
 * with their owning team/index, the reset-installed action code 0
 * (FUN_0007DAB4 0x7DAFB -> FUN_0007D9A4 code 0), the FU-147 S1 `+0x8D` active
 * seed (`FUN_0008C2E0` `0x8C329`: the byte is the record ordinal) and the
 * ball/actor state unset. */
static void test_init_resets_pool(void) {
  struct fifa96_match_entities pool;
  memset(&pool, 0xAA, sizeof pool);
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  assert(pool.team[TEAM0].side == 0 && pool.team[TEAM1].side == 1);
  assert(pool.team[TEAM0].target == NONE && pool.team[TEAM1].target == NONE);
  assert(pool.team[TEAM0].second == NONE && pool.team[TEAM0].intercept == NONE);
  assert(pool.team[TEAM0].chosen == NONE);
  assert(pool.team[TEAM0].tracker7c7 == NONE);
  assert(pool.team[TEAM1].tracker7c7 == NONE);
  assert(pool.team[TEAM0].update_count == 0);
  assert(pool.controlled == NONE);
  assert(pool.slot_merge == NONE);
  assert(pool.place_pending == 0);
  assert(pool.ball.carrier == NONE);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      const struct fifa96_match_entity *e = &pool.team[t].records[i];
      assert(e->team == t && e->index == i);
      assert(e->code == 0);
      assert(e->stage92 == 0xFF);
      assert(e->timer89 == 0 && e->timer81 == 0 && e->timer93 == 0);
      assert(e->active == i && e->ran == 0 && e->carrier == 0);
      assert(e->skip_98 == 0 && e->skip_9a == 0 && e->has_slot == 0);
      assert(e->anim_id == 0 && e->frame == 0);
      assert(e->pos_x == 0 && e->pos_z == 0 && e->target_x == 0);
      assert(e->lane_x == 0 && e->lane_z == 0 && e->bound == 0 && e->cam_dz6f == 0);
      assert(e->install == 0 && e->helper_request == 0 && e->controlled == 0);
      assert(e->place_valid == 0);
    }
  }
  assert(fifa96_match_entities_init(NULL) == -FIFA96_ERR_INVALID);
}

/* release zeroes the whole pool (no -1/0xFF seeds survive) so a re-init is
 * required before reuse; NULL is rejected. */
static void test_release_clears(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  pool.team[TEAM0].target = 5;
  pool.controlled = 6;
  pool.ball.carrier = 7;
  pool.place_pending = 1;
  assert(fifa96_match_entities_release(&pool) == FIFA96_OK);
  struct fifa96_match_entities zeroed;
  memset(&zeroed, 0, sizeof zeroed);
  assert(memcmp(&pool, &zeroed, sizeof pool) == 0);
  assert(fifa96_match_entities_release(NULL) == -FIFA96_ERR_INVALID);
}

/* Record field round-trips pin the derived record map: position triple
 * +0x59/+0x5D/+0x61, output triple +0x4D/+0x51/+0x55, velocity pair
 * +0x71/+0x73, lane +0x69, the timer cluster +0x7F/+0x81/+0x89/+0x93 and the
 * OL-80 animation inputs `anim_id` (native byte[[rec+0x28]], 0x36D44) and
 * `frame` (native byte[rec+0x3D], 0x36D4F). */
static void test_record_fields_roundtrip(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  struct fifa96_match_entity *e = &pool.team[TEAM1].records[7];
  e->pos_x = 0x1234;
  e->pos_y = -0x20;
  e->pos_z = 0x5678;
  e->target_x = 1;
  e->target_y = 2;
  e->target_z = 3;
  e->vel_x = 0x18000;
  e->vel_z = -0x10000;
  e->lane = 0x40;
  e->anim_id = 0x28;
  e->frame = 0x0D;
  e->timer7f = 0x003C0000u;
  e->timer81 = 0x0B;
  e->timer89 = -9;
  e->timer93 = 0x10;
  e->timer7b = 4;
  e->timer79 = 6;
  e->type = 5;
  e->actor_type = 2;
  e->stage = 3;
  e->stage92 = 0x12;
  e->has_ball = 1;
  e->has_slot = 1;
  e->skip_98 = 1;
  e->skip_9a = 1;
  e->lane_x = -3;
  e->lane_z = 9;
  assert(e->pos_x == 0x1234 && e->pos_y == -0x20 && e->pos_z == 0x5678);
  assert(e->target_z == 3 && e->vel_x == 0x18000 && e->lane == 0x40);
  assert(e->anim_id == 0x28 && e->frame == 0x0D);
  assert(e->timer81 == 0x0B && e->timer89 == -9 && e->timer93 == 0x10);
  assert(e->timer7b == 4 && e->timer79 == 6 && e->type == 5 && e->stage92 == 0x12);
  assert(e->lane_x == -3 && e->lane_z == 9);
}

/* FU-137 §2 installer rejections: an occupied record (+0x9A) and the
 * same-code case are no-ops (`0x7D9BE`, `0x7D9D6`). */
static void test_install_rejects(void) {
  struct fifa96_match_entity e;
  memset(&e, 0, sizeof e);
  e.code = 7;
  e.skip_9a = 1;
  assert(fifa96_match_entities_install(&e, 2, 4, 0) == 0);
  assert(e.code == 7);
  e.skip_9a = 0;
  assert(fifa96_match_entities_install(&e, 2, 7, 0) == 0);
  assert(e.code == 7);
  assert(fifa96_match_entities_install(NULL, 2, 1, 0) == -FIFA96_ERR_INVALID);
}

/* FU-137 §2 staging: `[rec+0x91]=code`, `[rec+0x89]=0`, `[rec+0x9E]=0`,
 * `[rec+0x92]=staged`, `[rec+0x7B]=[rec+0x79]` (0x7DA63..0x7DA9F). The
 * engine stores the code (the `[rec+0x18]` handler slot is the code-indexed
 * FU-137 dispatch table). */
static void test_install_stages(void) {
  struct fifa96_match_entity e;
  memset(&e, 0, sizeof e);
  e.code = 0;
  e.active = 1;
  e.ran = 1;
  e.timer89 = 77;
  e.timer79 = 0x1234;
  e.timer7b = 0;
  assert(fifa96_match_entities_install(&e, 2, 0x0B, 0x21) == 1);
  assert(e.code == 0x0B);
  assert(e.timer89 == 0);
  assert(e.ran == 0);
  assert(e.stage92 == 0x21);
  assert(e.timer7b == 0x1234);
}

/* Code coercion (0x7DA26..0x7DA3B): an inactive record receiving action 3 is
 * staged as 0x19. */
static void test_install_coerces_inactive_code3(void) {
  struct fifa96_match_entity e;
  memset(&e, 0, sizeof e);
  e.active = 0;
  assert(fifa96_match_entities_install(&e, 2, 3, 0) == 1);
  assert(e.code == 0x19);
  e.code = 0;
  e.active = 1;
  assert(fifa96_match_entities_install(&e, 2, 3, 0) == 1);
  assert(e.code == 3);
}

/* Carrier bit (+0x9F bit 0, 0x7DA42..0x7DA5A): set for codes 5/0x21, cleared
 * for every other code. */
static void test_install_carrier_bit(void) {
  struct fifa96_match_entity e;
  memset(&e, 0, sizeof e);
  assert(fifa96_match_entities_install(&e, 2, 5, 0) == 1);
  assert(e.carrier == 1);
  assert(fifa96_match_entities_install(&e, 2, 0x21, 0) == 1);
  assert(e.carrier == 1);
  assert(fifa96_match_entities_install(&e, 2, 4, 0) == 1);
  assert(e.carrier == 0);
}

/* The pre-stage animation arm (0x7D9DC..0x7DA1F): `[rec+0x98]` is cleared when
 * the incoming code is not 0x0C and the phase is not 2/0xA/0xF; the native
 * FUN_0006E598 call is the unported animation side effect (FU-141 leg). */
static void test_install_skip98_clear(void) {
  struct fifa96_match_entity e;
  memset(&e, 0, sizeof e);
  e.skip_98 = 1;
  assert(fifa96_match_entities_install(&e, 2, 9, 0) == 1);
  assert(e.skip_98 == 1); /* phase 2: arm skipped */
  e.skip_98 = 1;
  e.code = 0;
  assert(fifa96_match_entities_install(&e, 3, 9, 0) == 1);
  assert(e.skip_98 == 0); /* phase 3: arm ran */
  e.skip_98 = 1;
  e.code = 0;
  assert(fifa96_match_entities_install(&e, 3, 0x0C, 0) == 1);
  assert(e.skip_98 == 1); /* code 0x0C: arm skipped */
}

/* Phase-2 nearest selection (FUN_0008D8EC 0x8D929..0x8D9B7): the nearest of
 * the 11 records excluding record 0 (skip 0), honoring +0x98/+0x9A; none
 * found leaves the native NULL -> -1. */
static void test_team_select_nearest(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  struct fifa96_match_team *team = &pool.team[TEAM0];
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    team->records[i].pos_x = 0x400;
  team->records[0].pos_x = 0;      /* keeper: skipped by skip 0 */
  team->records[0].pos_z = 0;
  team->records[3].pos_x = 0x100;
  team->records[3].pos_z = 0x100;
  team->records[6].pos_x = 0x10;
  team->records[6].pos_z = 0x20;
  assert(fifa96_match_entities_team_select(&pool, TEAM0, 0, 0) == 6);
  assert(team->target == 6);
  /* exclusions force the next candidate */
  team->records[6].skip_98 = 1;
  assert(fifa96_match_entities_team_select(&pool, TEAM0, 0, 0) == 3);
  team->records[6].skip_98 = 0;
  team->records[6].skip_9a = 1;
  assert(fifa96_match_entities_team_select(&pool, TEAM0, 0, 0) == 3);
  assert(fifa96_match_entities_team_select(NULL, TEAM0, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_entities_team_select(&pool, 2, 0, 0) == -FIFA96_ERR_INVALID);
}

static void test_team_select_none(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    pool.team[TEAM0].records[i].skip_98 = 1;
  assert(fifa96_match_entities_team_select(&pool, TEAM0, 0, 0) == NONE);
  assert(pool.team[TEAM0].target == NONE);
}

/* FU-151 P3: `FUN_0008DDE0` (`0x8DDE0`) — the ranked unsigned `+0x6B` lane
 * pick. Skip -1 admits record 0; skip 0 excludes it; +0x98/+0x9A exclude; the
 * unsigned compare keeps the first record on ties; -1/2 are rejected. */
static void test_team_pick_ranked_lane(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    pool.team[TEAM0].records[i].lane_x = (int16_t)(100 + i);
  pool.team[TEAM0].records[9].lane_x = 1;      /* the smallest */
  assert(fifa96_match_entities_team_pick(&pool, TEAM0, -1) == 9);
  assert(fifa96_match_entities_team_pick(&pool, TEAM0, 9) == 0);   /* 100 smallest */
  pool.team[TEAM0].records[0].skip_9a = 1;
  assert(fifa96_match_entities_team_pick(&pool, TEAM0, 9) == 1);
  pool.team[TEAM0].records[1].skip_98 = 1;
  assert(fifa96_match_entities_team_pick(&pool, TEAM0, 9) == 2);
  /* a negative lane word is unsigned-large, never chosen over 0x0064 */
  pool.team[TEAM0].records[2].lane_x = -1;
  assert(fifa96_match_entities_team_pick(&pool, TEAM0, 9) == 3);
  assert(fifa96_match_entities_team_pick(NULL, TEAM0, -1) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_entities_team_pick(&pool, 2, -1) == -FIFA96_ERR_INVALID);
}

/* FU-151 P3: `FUN_0008C33C` + `FUN_0007997C` — the derived reset-lane pass:
 * per-record reset (+0x9B/code/stage92/bound := fresh lane), the lane refresh
 * against the camera, then the team pick into +0x7B2 and +0x7C7 and the
 * +0x7B6/+0x7CB clears. */
static void test_reset_lane_pass(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  /* every record starts far from the camera (origin); record 5 sits at the
   * origin, holds the ball and a stale lane, so it becomes the pick. */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    pool.team[TEAM0].records[i].pos_x = 0x2000;
    pool.team[TEAM0].records[i].pos_z = 0;
    pool.team[TEAM0].records[i].lane_x = 0x700;
  }
  pool.team[TEAM0].records[5].has_ball = 1;
  pool.team[TEAM0].records[5].code = 0x19;
  pool.team[TEAM0].records[5].stage92 = 7;
  pool.team[TEAM0].records[5].pos_x = 0;
  pool.team[TEAM0].records[5].pos_z = 0;
  pool.team[TEAM0].second = 4;
  pool.team[TEAM0].timer7cb = 9;
  assert(fifa96_match_entities_reset_lane(&pool, TEAM0, 0, 0) == FIFA96_OK);
  assert(pool.team[TEAM0].records[5].has_ball == 0);
  assert(pool.team[TEAM0].records[5].code == 3);       /* active -> install 3 */
  assert(pool.team[TEAM0].records[5].stage92 == 0);
  assert(pool.team[TEAM0].records[5].lane_x == 0);     /* refreshed vs camera */
  assert(pool.team[TEAM0].records[5].bound == 0);
  assert(pool.team[TEAM0].records[3].lane_x == 0x2000); /* refreshed vs camera */
  assert(pool.team[TEAM0].timer7cb == 0);
  assert(pool.team[TEAM0].second == NONE);
  assert(pool.team[TEAM0].target == 5);
  assert(pool.team[TEAM0].tracker7c7 == 5);
  /* record 0 inactive -> the forced install is 0x19 (via the installer). */
  assert(pool.team[TEAM0].records[0].code == 0x19);
  /* Off-axis discriminating case: the 0x795B4 band is the `FUN_000CD514`
   * folded-angle hypot (0x286 for dx=0x240, dz=0x468), NOT the 0x8DC68
   * octagonal metric (0x540). */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    pool.team[TEAM0].records[i].pos_x = 0x2000;
    pool.team[TEAM0].records[i].pos_z = 0;
  }
  pool.team[TEAM0].records[6].pos_x = -0x240;
  pool.team[TEAM0].records[6].pos_z = -0x468;
  assert(fifa96_match_entities_reset_lane(&pool, TEAM0, 0, 0) == FIFA96_OK);
  assert(pool.team[TEAM0].records[6].lane_x == 0x286);
  assert(pool.team[TEAM0].records[6].lane_z == 0x240);   /* cam.x - pos.x */
  assert(pool.team[TEAM0].records[6].cam_dz6f == 0x468); /* cam.z - pos.z */
  assert(pool.team[TEAM0].target == 6);
  assert(pool.team[TEAM0].tracker7c7 == 6);
  assert(fifa96_match_entities_reset_lane(NULL, TEAM0, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_entities_reset_lane(&pool, 2, 0, 0) == -FIFA96_ERR_INVALID);
}

/* The update chain order, observed by a counter callback: team 0 records
 * 0..10 then team 1 records 0..10 (FUN_0004B100 0x4B2A9/0x4B2B0 ->
 * FUN_0008D8EC 0x8DB2E record 0 then 0x8DB3A records 1..10). The phase-2
 * selection runs before the record walk, so the callback already sees the
 * chosen team target. Records 1..10 with +0x9A set are skipped. */
static uint8_t order_seen[32];
static uint32_t order_count;
static int order_cb(void *ctx, struct fifa96_match_entity *e) {
  (void)ctx;
  order_seen[order_count++] = (uint8_t)(e->team * FIFA96_MATCH_ENTITY_RECORDS + e->index);
  return FIFA96_OK;
}

static void test_update_chain_order(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f = zero_frame();
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  f.phase = 2;
  f.delta = 2;
  /* Team 0: only record 5 sits on the (bucket 2, zero-lead) target; the rest
   * are far away so the phase-2 selection picks 5 before the record walk. */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    pool.team[TEAM0].records[i].pos_x = 0x400;
  }
  pool.team[TEAM0].records[5].pos_x = 4;
  pool.team[TEAM0].records[5].pos_z = 0;
  pool.team[TEAM1].records[10].skip_9a = 1; /* outfield skip */
  order_count = 0;
  assert(fifa96_match_entities_update(&pool, &f, order_cb, NULL) == FIFA96_OK);
  /* Team 0: records 0..10; team 1: 0..9 (10 skipped). */
  assert(order_count == 21);
  for (uint32_t i = 0; i < 11; i++) assert(order_seen[i] == (uint8_t)i);
  for (uint32_t i = 0; i < 10; i++) assert(order_seen[11 + i] == (uint8_t)(11 + i));
  assert(pool.team[TEAM0].target == 5);
  /* Team 1's all-(0,0) tie picks the first non-keeper candidate (index 1,
   * encoded as 11+1). */
  assert(pool.team[TEAM1].target == (int32_t)(FIFA96_MATCH_ENTITY_RECORDS + 1));
  /* Removing the skip restores the skipped record's dispatch. */
  pool.team[TEAM1].records[10].skip_9a = 0;
  order_count = 0;
  assert(fifa96_match_entities_update(&pool, &f, order_cb, NULL) == FIFA96_OK);
  assert(order_count == 22);
  assert(order_seen[21] == (uint8_t)(11 + 10));
}

/* FUN_0008D8EC 0x8D948..0x8D9A6 selection buckets: word[0x1577FA] <
 * word[0x157800] takes the 0x157788 vector, else < word[0x157806] takes
 * 0x157794, else 0x157770 plus ([0x1577BE]>>16)<<5 on x and
 * ([0x1577C0]>>16)<<5 on the third axis. */
static void test_update_selection_buckets(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f = zero_frame();
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  f.phase = 2;
  f.select_vector[0][0] = 10;
  f.select_vector[0][2] = 0;
  f.select_vector[1][0] = 200;
  f.select_vector[1][2] = 0;
  f.select_vector[2][0] = 0;
  f.select_vector[2][2] = 200;
  f.lead_x = 0x00020000; /* >>16<<5 = 64 */
  f.lead_z = -0x00020000;
  pool.team[TEAM0].records[2].pos_x = 10;
  pool.team[TEAM0].records[2].pos_z = 0;
  pool.team[TEAM0].records[4].pos_x = 200;
  pool.team[TEAM0].records[4].pos_z = 0;
  pool.team[TEAM0].records[5].pos_x = 0;   /* pre-lead bucket-2 target */
  pool.team[TEAM0].records[5].pos_z = 200;
  pool.team[TEAM0].records[9].pos_x = 64;  /* camera + lead lands exactly here */
  pool.team[TEAM0].records[9].pos_z = 136;
  f.timer_a = 0;
  f.timer_b = 1;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].target == 2);
  pool.team[TEAM0].target = NONE;
  f.timer_a = 1;
  f.timer_b = 1;
  f.timer_c = 5;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].target == 4);
  pool.team[TEAM0].target = NONE;
  f.timer_a = 5;
  f.timer_c = 5;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].target == 9); /* third vector + lead adds */
}

/* FUN_0008D8EC 0x8D9BD..0x8DAE9 interception selection: only when the
 * controlled record exists, its team side matches, |[0x157754]| > 0x480 and
 * the sign of [0x157754] matches the side; skip = the *team target's*
 * ([team+0x7B2]) +0x8A>>24 byte, i.e. that record's +0x8D active byte
 * (0x8DA2C: `MOV EBX,[EBP+0x7B2]; MOV EBX,[EBX+0x8A]; SAR EBX,0x18` — not the
 * controlled actor [0x157A83]); a found record holding a control slot is
 * rejected. */
static void test_update_intercept_select(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f = zero_frame();
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  f.phase = 2;
  f.cam_z = 0x500;
  f.intercept_x[0] = 0;
  f.intercept_y[0] = 0;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    pool.team[TEAM0].records[i].pos_x = 0x400;
  }
  pool.team[TEAM0].records[3].pos_x = 10; /* nearest if its rival is skipped */
  pool.team[TEAM0].records[3].pos_z = 0;
  pool.team[TEAM0].records[4].pos_x = 5; /* nearest overall, skipped */
  pool.team[TEAM0].records[4].pos_z = 0;
  pool.team[TEAM0].records[5].pos_x = 15;
  pool.team[TEAM0].records[5].pos_z = 0;
  /* The controlled actor and the team target differ: the skip must come from
   * the target (index 2, active 4), not the controlled record (index 1). */
  pool.controlled = TEAM0 * FIFA96_MATCH_ENTITY_RECORDS + 1;
  pool.team[TEAM0].target = TEAM0 * FIFA96_MATCH_ENTITY_RECORDS + 2;
  pool.team[TEAM0].records[1].active = 1;
  pool.team[TEAM0].records[2].active = 4;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].intercept == 3);
  /* the found record holds a slot -> rejected */
  pool.team[TEAM0].records[3].has_slot = 1;
  pool.team[TEAM0].intercept = 0;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].intercept == NONE);
  /* wrong cam_z sign for the side clears it */
  pool.team[TEAM0].records[3].has_slot = 0;
  pool.team[TEAM0].intercept = 5;
  f.cam_z = -0x500;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].intercept == NONE);
  /* |cam_z| <= 0x480 clears it */
  f.cam_z = 0x480;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].intercept == NONE);
  /* the sides compare the +0x826 bytes, not the block order */
  pool.team[TEAM0].side = 1;
  pool.team[TEAM1].side = 0;
  f.cam_z = -0x500;
  f.intercept_x[1] = 0; /* side 1 target */
  f.intercept_y[1] = 0;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].intercept == 3);
  assert(pool.team[TEAM1].intercept == NONE);
  /* a NONE team target with no selectable candidate is guarded (the native
   * would dereference NULL; the port skips the interception) */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    pool.team[TEAM0].records[i].skip_98 = 1;
  pool.team[TEAM0].target = NONE;
  pool.team[TEAM0].intercept = 5;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].target == NONE);
  assert(pool.team[TEAM0].intercept == NONE);
}

/* OL-41 (M2 Task 14, fix round 1): the interception band flag `team+0x7BE`
 * (`0x8DA7F..0x8DAE7`). The nearest passes the bind `FUN_0008D824` (writes its
 * +0x4D triple from the *controlled actor's* x/z; `0x8D82A MOV
 * EDI,[EAX+0x61]` with EAX = `[0x157A83]`), then `FUN_000795B4` runs on
 * (nearest+0x59, nearest+0x4D) with a stack scratch output
 * (`0x8DA88 LEA EBX,[ESP+0xC]`) — the record target is not written back. The
 * flag sets when the band word `< 0xF0` (signed) and the record's +0x6B lane
 * word `> 0x1E0` or `|pos.z| > |cam_z| + 0x90`. */
static void test_update_intercept_band_flag(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f = zero_frame();
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  f.phase = 2;
  f.cam_z = 0x500;
  f.intercept_x[0] = 0;
  f.intercept_y[0] = 0;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    pool.team[TEAM0].records[i].pos_x = 0x400;
    pool.team[TEAM0].records[i].pos_z = 0x400;
  }
  /* controlled actor at (0, 0x800): actor_x 0, actor_z 0x800, out of nearest
   * range */
  pool.controlled = TEAM0 * FIFA96_MATCH_ENTITY_RECORDS + 1;
  pool.team[TEAM0].records[1].pos_x = 0;
  pool.team[TEAM0].records[1].pos_z = 0x800;
  pool.team[TEAM0].target = TEAM0 * FIFA96_MATCH_ENTITY_RECORDS + 2;
  /* nearest: (0x240, 0x50), distance 0x254 < 0x500 */
  pool.team[TEAM0].records[3].pos_x = 0x240;
  pool.team[TEAM0].records[3].pos_z = 0x50;
  pool.team[TEAM0].records[3].lane_x = 0x1F0; /* > 0x1E0 */
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].intercept == 3);
  assert(pool.team[TEAM0].flag7be == 1);
  /* bind (actor_z 0x800): x=0x240, y=0,
   * z=0xB10-((0xB10-0x800)/2+0x120)=0x868; the band (dx=0, dz=0x818) is a
   * stack scratch and leaves the target triple as the bind output. */
  assert(pool.team[TEAM0].records[3].target_x == 0x240);
  assert(pool.team[TEAM0].records[3].target_y == 0);
  assert(pool.team[TEAM0].records[3].target_z == 0x868);
  /* lane fails and |pos.z| <= |cam_z| + 0x90 -> no flag (flag is cleared at
   * the top of each frame, 0x8D9C5) */
  pool.team[TEAM0].records[3].lane_x = 0;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].flag7be == 0);
  /* band >= 0xF0 refuses even with the lane gate open */
  pool.team[TEAM0].records[3].pos_x = 0;
  pool.team[TEAM0].records[3].pos_z = 0;
  pool.team[TEAM0].records[3].lane_x = 0x1F0;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].intercept == 3);
  assert(pool.team[TEAM0].flag7be == 0);
  /* the bind output survives: +0x4D = 0x240, +0x51 = 0, +0x55 = 0x868 (the
   * band word 0x254 for this geometry is only a stack value) */
  assert(pool.team[TEAM0].records[3].target_x == 0x240);
  assert(pool.team[TEAM0].records[3].target_y == 0);
  assert(pool.team[TEAM0].records[3].target_z == 0x868);
  /* a slot-rejected nearest clears the flag and skips the bind (the native
   * then computes the band over the NULL record; the derived model refuses —
   * OL-71) */
  pool.team[TEAM0].records[3].has_slot = 1;
  pool.team[TEAM0].records[3].lane_x = 0x1F0;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].intercept == NONE);
  assert(pool.team[TEAM0].flag7be == 0);
  /* non-phase-2 clears the flag (the pool update skips the interception) */
  f.phase = 0;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].flag7be == 0);
}

/* Update counter wraps at 0xB (0x8D8F7..0x8D912) and the record timer pair
 * decays by the frame delta with the +0x7F>>16 limit (FUN_0007CA54
 * 0x7CA5C..0x7CAB8); the keeper record 0 copies +0x79 -> +0x7B
 * (FUN_000782D0, FU-67 §4.3). */
static void test_update_counter_and_timers(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f = zero_frame();
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  f.phase = 0; /* no selection */
  f.delta = 3;
  struct fifa96_match_entity *e = &pool.team[TEAM0].records[2];
  e->timer7f = 0x00020000u; /* limit 2 <= delta 3: the native zeroes the word */
  e->timer81 = 5;
  e->timer93 = 2;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].update_count == 1);
  assert(e->timer81 == 0);
  assert(e->timer93 == 0);
  e->timer7f = 0x00040000u; /* limit 4 > delta 3: decay by 3 */
  e->timer81 = 9;
  e->timer93 = 9;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(e->timer81 == 6);
  assert(e->timer93 == 6);
  pool.team[TEAM0].records[0].timer79 = 0x2222;
  pool.team[TEAM0].records[0].timer7b = 0;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].records[0].timer7b == 0x2222);
  for (uint32_t i = 0; i < 10; i++)
    assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].update_count == 13 % 0xB);
  /* The limit/delta compares are signed-word against the zero-extended delta
   * word (0x7CA74/0x7CAA0): a 0x8000 delta is 32768, not -32768. */
  e->timer7f = 0x00070000u; /* limit 7 */
  e->timer81 = 5;
  e->timer93 = 5;
  f.delta = 0x8000;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(e->timer81 == 0);
  assert(e->timer93 == 0);
}

/* M2 phase-9 T2 (FU-142 OL-63): the possession-block release countdown
 * `[0x15872D]` decay, native `FUN_0004B100 0x4B163..0x4B17B` — a signed
 * positive byte decays by the zero-extended frame delta and can wrap to
 * 0xFF (which the signed `TEST CL,CL; JLE` then stops decaying). Runs on
 * every frame body, not only phase 2. */
static void test_possession_release_decay(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f = zero_frame();
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  assert(pool.ball.pos_release == 0);
  pool.ball.pos_release = 5;
  f.delta = 3;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.ball.pos_release == 2);
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.ball.pos_release == 0xFF);   /* 2 - 3 unsigned-byte wrap */
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.ball.pos_release == 0xFF);   /* signed byte now <= 0: frozen */
  pool.ball.pos_release = 1;
  f.delta = 1;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.ball.pos_release == 0);
}

/* The pool consumes each dispatched record's requests: install runs the FU-137
 * §2 installer, helper_request runs the slot merge, controlled binds the pool
 * actor, place_valid becomes a take-once camera-place request. */
static int request_cb(void *ctx, struct fifa96_match_entity *e) {
  (void)ctx;
  if (e->team == TEAM0 && e->index == 4) {
    e->install = 3;
    e->ran = 1;
    e->helper_request = 1;
    e->controlled = 1;
    e->place_valid = 1;
    e->place_x = 0x111;
    e->place_y = 0x222;
    e->place_z = 0x333;
  }
  return FIFA96_OK;
}

static void test_update_consumes_requests(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f = zero_frame();
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  f.phase = 2;
  pool.team[TEAM0].slot_pool = 1;
  pool.team[TEAM0].records[4].active = 1;
  pool.team[TEAM0].records[1].has_slot = 1; /* donor */
  pool.team[TEAM0].records[1].pos_x = 100;
  assert(fifa96_match_entities_update(&pool, &f, request_cb, NULL) == FIFA96_OK);
  struct fifa96_match_entity *e = &pool.team[TEAM0].records[4];
  assert(e->install == 0);          /* consumed + cleared */
  assert(e->code == 3);             /* staged by the installer */
  assert(e->ran == 0);              /* installer clear */
  assert(e->helper_request == 0);   /* consumed + cleared */
  assert(e->has_slot == 1);
  assert(pool.team[TEAM0].records[1].has_slot == 0);
  assert(pool.slot_merge == (int32_t)(TEAM0 * FIFA96_MATCH_ENTITY_RECORDS + 4));
  assert(e->controlled == 0 && pool.controlled == (int32_t)(TEAM0 * FIFA96_MATCH_ENTITY_RECORDS + 4));
  assert(e->place_valid == 0 && pool.place_pending == 1);
  int32_t px = 0, py = 0, pz = 0;
  assert(fifa96_match_entities_take_place(&pool, &px, &py, &pz) == 1);
  assert(px == 0x111 && py == 0x222 && pz == 0x333);
  assert(pool.place_pending == 0);
  assert(fifa96_match_entities_take_place(&pool, &px, &py, &pz) == 0);
  assert(fifa96_match_entities_take_slot_merge(&pool) ==
         (int32_t)(TEAM0 * FIFA96_MATCH_ENTITY_RECORDS + 4));
  assert(fifa96_match_entities_take_slot_merge(&pool) == NONE);
}

/* The ranked slot merge (FUN_0007876C + FUN_00078670): the first
 * slot-holding candidate is taken unconditionally (the native counter at
 * 0x787E4 increments only for slot-holders; no-slot candidates bypass it at
 * 0x787A7) and later slot-holders replace it only when their signed-word
 * distance is strictly greater; the requester takes the slot and the donor
 * loses it. */
static void test_merge_slot_ranked(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  struct fifa96_match_team *team = &pool.team[TEAM0];
  team->slot_pool = 1;
  team->records[0].has_slot = 1;
  team->records[0].pos_x = 100; /* d = 100 */
  team->records[4].has_slot = 1;
  team->records[4].pos_x = 0; /* d = 0, not greater */
  team->records[3].pos_x = 0;
  assert(fifa96_match_entities_merge_slot(&pool, TEAM0, 3) == 1);
  assert(team->records[3].has_slot == 1);
  assert(team->records[0].has_slot == 0);
  assert(team->records[4].has_slot == 1);
  assert(fifa96_match_entities_take_slot_merge(&pool) ==
         (int32_t)(TEAM0 * FIFA96_MATCH_ENTITY_RECORDS + 3));
  /* the larger distance wins the ranking */
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  team = &pool.team[TEAM0];
  team->slot_pool = 1;
  team->records[0].has_slot = 1;
  team->records[0].pos_x = 0;
  team->records[4].has_slot = 1;
  team->records[4].pos_x = 100;
  team->records[3].pos_x = 0;
  assert(fifa96_match_entities_merge_slot(&pool, TEAM0, 3) == 1);
  assert(team->records[3].has_slot == 1 && team->records[4].has_slot == 0);
  /* The native counter at 0x787E4 increments only for slot-holding
   * candidates, so the *first slot-holder* is taken unconditionally, even
   * when record 0 has no slot and that holder's distance is <= 0. */
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  team = &pool.team[TEAM0];
  team->slot_pool = 1;
  team->records[1].has_slot = 1; /* record 0 has no slot */
  team->records[1].pos_x = 0;    /* d = 0 */
  team->records[3].pos_x = 0;
  assert(fifa96_match_entities_merge_slot(&pool, TEAM0, 3) == 1);
  assert(team->records[3].has_slot == 1 && team->records[1].has_slot == 0);
  assert(fifa96_match_entities_take_slot_merge(&pool) ==
         (int32_t)(TEAM0 * FIFA96_MATCH_ENTITY_RECORDS + 3));
  /* gates: requester already has a slot, no pool flag, no donor */
  assert(fifa96_match_entities_merge_slot(&pool, TEAM0, 3) == 0);
  assert(fifa96_match_entities_merge_slot(&pool, TEAM1, 3) == 0); /* slot_pool 0 */
  team->records[3].has_slot = 0;
  team->records[0].has_slot = 0;
  team->records[1].has_slot = 0;
  team->records[4].has_slot = 0;
  assert(fifa96_match_entities_merge_slot(&pool, TEAM0, 3) == 0);
  assert(fifa96_match_entities_merge_slot(NULL, TEAM0, 3) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_entities_merge_slot(&pool, TEAM0, 11) == -FIFA96_ERR_INVALID);
}

/* M2 interactive Task 1 / FU-70 §1.3: the derived setup slot bind
 * (`FUN_000785E0` -> `FUN_0008DB6C` free-record pick). The native walk skips
 * record 0 while `[team+0x829] == 0`, skips `+0x98`/`+0x9A` and already-bound
 * holders, and picks the nearest record to the `0x5774C` point under the
 * `0x8DC68` metric; the bind sets `+0x20` and increments `team+0x828`. */
static void test_bind_slot(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  struct fifa96_match_team *team = &pool.team[TEAM0];

  /* Record 0 is at the origin but the search gate is clear: skipped. Record 5
   * is nearest but excluded by +0x9A; record 3 wins. */
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    team->records[i].pos_x = 100;
    team->records[i].pos_z = 100;
  }
  team->records[0].pos_x = 0;
  team->records[5].pos_x = 2;
  team->records[5].pos_z = 2;
  team->records[5].skip_9a = 1;
  team->records[3].pos_x = 10;
  team->records[3].pos_z = 10;
  assert(fifa96_match_entities_bind_slot(&pool, TEAM0, 0, 0) ==
         (int32_t)(TEAM0 * FIFA96_MATCH_ENTITY_RECORDS + 3));
  assert(team->records[3].has_slot == 1);
  assert(team->slot_pool == 1);

  /* A second bind skips the holder and takes the next nearest. */
  assert(fifa96_match_entities_bind_slot(&pool, TEAM0, 0, 0) ==
         (int32_t)(TEAM0 * FIFA96_MATCH_ENTITY_RECORDS + 1));
  assert(team->records[1].has_slot == 1);
  assert(team->slot_pool == 2);

  /* The record-0 gate: with the team search gate set, record 0 is eligible. */
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  pool.team[TEAM1].search_gate = 1;
  pool.team[TEAM1].records[0].pos_x = 0;   /* nearest to the (0,0) point */
  pool.team[TEAM1].records[3].pos_x = 9;
  assert(fifa96_match_entities_bind_slot(&pool, TEAM1, 0, 0) ==
         (int32_t)(TEAM1 * FIFA96_MATCH_ENTITY_RECORDS));
  assert(pool.team[TEAM0].records[0].has_slot == 0);   /* team 0 untouched */
  assert(pool.team[TEAM1].records[0].has_slot == 1);
  assert(pool.team[TEAM1].slot_pool == 1);

  assert(fifa96_match_entities_bind_slot(NULL, TEAM0, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_entities_bind_slot(&pool, 2, 0, 0) == -FIFA96_ERR_INVALID);
}

/* FUN_0007D430 pairing at the tail of the frame (FUN_0004B100 0x4B2B7..):
 * when the predicted closing distance is smaller and under 0x40, team 1's
 * nudged position lands in team 0's output triple. */
static void test_ball_pair(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f = zero_frame();
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  f.phase = 2;
  f.delta = 4;
  pool.controlled = 0; /* keep the preset targets (selection skipped) */
  pool.team[TEAM0].target = 2;
  pool.team[TEAM1].target = (int32_t)(FIFA96_MATCH_ENTITY_RECORDS + 5);
  pool.team[TEAM0].records[2].pos_x = 0;
  pool.team[TEAM0].records[2].pos_z = 0;
  pool.team[TEAM0].records[2].vel_x = 10;
  pool.team[TEAM0].records[2].vel_z = 0;
  /* predicted A = 40; B at (50, 0): current |A-B| = 50, closing |40-50| = 10 */
  pool.team[TEAM1].records[5].pos_x = 50;
  pool.team[TEAM1].records[5].pos_z = 0;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  /* out = B.pos with the closing-delta sign nudge: dx = -10 -> -0x40 on x,
   * dz = 0 -> +0x40 on z (the FU-139 `fifa96_ball_pair_decide` contract). */
  assert(pool.team[TEAM0].records[2].target_x == 50 - 0x40);
  assert(pool.team[TEAM0].records[2].target_z == 0 + 0x40);
  /* a far / receding pair writes nothing and a non-phase-2 frame skips */
  pool.team[TEAM0].records[2].target_x = -1;
  pool.team[TEAM1].records[5].pos_x = 5000;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].records[2].target_x == -1);
  f.phase = 3;
  pool.team[TEAM1].records[5].pos_x = 50;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].records[2].target_x == -1);
}

static void test_ball_state_roundtrip(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  pool.ball.x = 0x100;
  pool.ball.y = 7;
  pool.ball.z = -0x200;
  pool.ball.heading = 0x1234;
  pool.ball.appearance = 9;
  pool.ball.carrier = 4;
  assert(pool.ball.x == 0x100 && pool.ball.y == 7 && pool.ball.z == -0x200);
  assert(pool.ball.heading == 0x1234 && pool.ball.appearance == 9 && pool.ball.carrier == 4);
  assert(pool.ball.pair.flags == 0 && pool.ball.pair.code == 0);
}

/* FU-89 §kickoff placement / OL-T11-8: the native setup/restart commit
 * `FUN_00079B6C` (`0x79B6C..0x79C1C`, single RET at `0x79C1C`; first-hand
 * get_function_by_address 0x79BB5 -> body_end 0x79C1C) writes position
 * +0x59/+0x5D/+0x61 from the target triple +0x4D/+0x51/+0x55, zeroes
 * position.y, copies the position back into the target triple, then zeroes the
 * word fields +0x69 (dz), +0x67 (dx), +0x65 (distance), +0x71/+0x73/+0x75
 * (both velocity halves) and the byte +0x9C. The pool maps `lane`
 * (dword +0x69) and `vel_x`/`vel_z` (dwords +0x71/+0x73) so `lane` keeps its
 * high word (native only clears +0x69) while both velocities are fully
 * cleared; the +0x65/+0x67 words have no pool field (the dispatch staging
 * recomputes `distance` from pos/target per FU-142e). */
static void test_place_commits_target(void) {
  struct fifa96_match_entity e;
  memset(&e, 0, sizeof e);
  e.pos_x = 0x111;
  e.pos_y = 0x222;
  e.pos_z = 0x333;
  e.target_x = -0x720;
  e.target_y = 0x55;
  e.target_z = 0x840;
  e.lane = (int32_t)0xAAAA0040u;   /* low word +0x69 = dz, high word kept */
  /* The three velocity word views and their dword aliases are all staged so
   * the commit's word zeroing is discriminating (a dword-only zero would leave
   * the words stale and the mover would resurrect them). */
  e.speed71 = 0x0012;
  e.vel73 = (int16_t)0xBBBB;
  e.vel75 = 0x0CCC;
  e.vel_x = (int32_t)((uint32_t)(uint16_t)e.speed71 |
                      ((uint32_t)(uint16_t)e.vel73 << 16));
  e.vel_z = (int32_t)((uint32_t)(uint16_t)e.vel73 |
                      ((uint32_t)(uint16_t)e.vel75 << 16));
  assert(fifa96_match_entities_place(&e) == FIFA96_OK);
  assert(e.pos_x == -0x720 && e.pos_y == 0 && e.pos_z == 0x840);
  assert(e.target_x == -0x720 && e.target_y == 0 && e.target_z == 0x840);
  assert((uint32_t)e.lane == 0xAAAA0000u);
  assert(e.speed71 == 0 && e.vel73 == 0 && e.vel75 == 0);   /* words cleared */
  assert(e.vel_x == 0);
  assert(e.vel_z == 0);                                  /* alias lockstep */
  assert(fifa96_match_entities_place(NULL) == -FIFA96_ERR_INVALID);
}

/* The kickoff act-1 body (first-hand `FUN_0008A938` table entry 1 ->
 * `0x8ABAB..0x8ABDA`): `[0x158830] = 0x1E0`, `[0x158838] = 0` (the ball spawn
 * triple read by `FUN_0008C24C` at `FUN_00073E08`, which then writes
 * `[0x158810] = 0` at `0x8C299`) and `[0x157AB1] = 0` (a process global with
 * no derived home). The `FUN_0008CF60` per-record loop then commits each
 * target and runs the `FUN_00079B6C` tail: the camera-vs-target face and the
 * `0x79C13` selector (inactive records take row 0x26, frame reset to 0). The
 * derived kickoff pass places the ball at (0x1E0, 0, 0) with camera (0,0,0)
 * and commits each non-zero record target into the position (the formation
 * target source is the resource-loaded `0x14BFC0` table, OL-T11-8). */
static void test_kickoff_place_commits_records_and_ball(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  pool.ball.x = 0x999;   /* pre-existing spawn word is overwritten */
  pool.ball.y = 0x777;   /* cleared by FUN_0008C24C 0x8C299 */
  pool.ball.z = 0x888;
  pool.team[TEAM0].records[3].target_x = 0x780;
  pool.team[TEAM0].records[3].target_z = -0x21;
  pool.team[TEAM0].records[3].pos_x = 0x7FFFFFFF;   /* committed over */
  pool.team[TEAM1].records[1].target_x = -0x720;
  pool.team[TEAM1].records[1].target_z = 0x840;
  assert(fifa96_match_entities_kickoff_place(&pool, 0, 0, 0) == FIFA96_OK);
  assert(pool.ball.x == 0x1E0 && pool.ball.y == 0 && pool.ball.z == 0);
  assert(pool.team[TEAM0].records[3].pos_x == 0x780);
  assert(pool.team[TEAM0].records[3].pos_z == -0x21);
  assert(pool.team[TEAM0].records[3].pos_y == 0);
  assert(pool.team[TEAM0].records[3].target_x == 0x780);
  assert(pool.team[TEAM1].records[1].pos_x == -0x720);
  assert(pool.team[TEAM1].records[1].pos_z == 0x840);
  /* records with zero targets are the native phase-0 identity placement */
  assert(pool.team[TEAM0].records[0].pos_x == 0);
  assert(pool.team[TEAM1].records[10].pos_z == 0);
  /* 0x79C13 selector with the FU-147 S1 `+0x8D` active seed: an inactive
   * record (active == 0, i.e. record 0) takes row id 0x26; every other record
   * (active == index) takes id 0, which the keep rule resolves to the fresh 0
   * row byte, and +0x3D resets to 0 (FU-84 §1/§2). */
  assert(pool.team[TEAM0].records[0].anim_id == 0x26);
  assert(pool.team[TEAM0].records[3].anim_id == 0);
  assert(pool.team[TEAM1].records[1].anim_id == 0);
  assert(pool.team[TEAM1].records[1].frame == 0);
  assert(fifa96_match_entities_kickoff_place(NULL, 0, 0, 0) == -FIFA96_ERR_INVALID);
}

/* FU-89 §11 / OL-T11-8 (M2 visible-match Task 1): the formation seed. The
 * 44 bytes are the extracted `352ko.fmt` (GAMEART0.PVI BIGF entry 0, first
 * hand: `00 b4 00 b8 ...`); the phase cell `FUN_0006E1D0` maps record i's
 * own/opp byte pair to the target triple. Team 0 (side 0, controlled) uses the
 * file's +2 pair and keeps the sign; team 1 uses +0 and is negated. */
static const uint8_t entity_formation_352ko[FIFA96_SCENE_FORMATION_BYTES] = {
  0x00, 0xB4, 0x00, 0xB8, 0xE8, 0xD8, 0xE8, 0xDA, 0x00, 0xD4, 0x00, 0xD8,
  0x18, 0xD8, 0x18, 0xDA, 0xDF, 0xFB, 0xE0, 0xFE, 0xEC, 0xF0, 0xED, 0xED,
  0xFF, 0xE6, 0xFF, 0xEB, 0x14, 0xF1, 0x13, 0xED, 0x20, 0xFC, 0x21, 0xFE,
  0xFA, 0xF8, 0xFD, 0xFE, 0x06, 0xF8, 0x02, 0xFE,
};

static void test_seed_formation_targets(void) {
  struct fifa96_match_entities pool;
  fifa96_scene_formation f;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  memset(&f, 0, sizeof f);
  assert(fifa96_match_entities_seed_formation(&pool, &f, 0) ==
         FIFA96_ERR_NOT_FOUND);   /* not loaded: targets untouched */
  assert(pool.team[TEAM0].records[0].target_z == 0);
  memcpy(f.bytes, entity_formation_352ko, sizeof entity_formation_352ko);
  f.loaded = 1;
  assert(fifa96_match_entities_seed_formation(&pool, &f, 0) == FIFA96_OK);
  assert(pool.team[TEAM0].records[0].target_x == 0);
  assert(pool.team[TEAM0].records[0].target_y == 0);
  assert(pool.team[TEAM0].records[0].target_z == -2376);   /* own 0xB8 */
  assert(pool.team[TEAM1].records[0].target_x == 0);
  assert(pool.team[TEAM1].records[0].target_z == 2508);    /* opp 0xB4 negated */
  assert(pool.team[TEAM0].records[8].target_x == 1254);    /* own 0x21 */
  assert(pool.team[TEAM0].records[8].target_z == -66);
  assert(pool.team[TEAM1].records[8].target_x == -1216);   /* opp 0x20 negated */
  assert(pool.team[TEAM1].records[8].target_z == 132);     /* opp 0xFC negated */
  /* The kickoff commit then lands the positions (FUN_00079B6C). */
  assert(fifa96_match_entities_kickoff_place(&pool, 0, 0, 0) == FIFA96_OK);
  assert(pool.team[TEAM0].records[0].pos_x == 0);
  assert(pool.team[TEAM0].records[0].pos_y == 0);
  assert(pool.team[TEAM0].records[0].pos_z == -2376);
  assert(pool.team[TEAM1].records[0].pos_x == 0);
  assert(pool.team[TEAM1].records[0].pos_z == 2508);
  assert(pool.team[TEAM0].records[8].pos_x == 1254);
  /* controlled side 1: team 1 becomes the own-pair reader */
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  assert(fifa96_match_entities_seed_formation(&pool, &f, 1) == FIFA96_OK);
  assert(pool.team[TEAM0].records[0].target_z == -2508);   /* opp 0xB4 */
  assert(pool.team[TEAM1].records[0].target_z == 2376);    /* own 0xB8 negated */
  assert(fifa96_match_entities_seed_formation(NULL, &f, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_entities_seed_formation(&pool, NULL, 0) == -FIFA96_ERR_INVALID);
}

/* ISO fixture: the real GAMEART0.PVI contains `352ko.fmt` as its first BIGF
 * entry; the loader must parse it and the seed must reproduce the first-hand
 * kickoff positions. Skips without the ISO (the house convention). */
static int entity_file_exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  fclose(f);
  return 1;
}

static void test_formation_iso_fixture(void) {
  if (!entity_file_exists("game/FIFAPCCD96.iso")) {
    fprintf(stderr, "SKIP formation ISO fixture (no ISO)\n");
    return;
  }
  struct fifa96_asset_table *table = NULL;
  assert(fifa96_asset_mount_file("game/FIFAPCCD96.iso", &table) == FIFA96_OK);
  uint8_t *bytes = NULL;
  size_t len = 0;
  assert(fifa96_asset_read(table, "/ART/GAMEART0.PVI", &bytes, &len) == FIFA96_OK);
  fifa96_scene_formation f;
  assert(fifa96_scene_formation_load(bytes, len, "352ko.fmt", &f) == FIFA96_OK);
  assert(f.loaded == 1);
  assert(memcmp(f.bytes, entity_formation_352ko, sizeof entity_formation_352ko) == 0);
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  assert(fifa96_match_entities_seed_formation(&pool, &f, 0) == FIFA96_OK);
  assert(pool.team[TEAM0].records[0].target_z == -2376);
  assert(pool.team[TEAM1].records[0].target_z == 2508);
  fifa96_asset_free(bytes);
  fifa96_asset_unmount(table);
}

/* The `FUN_0008CF60` face (`FUN_00079B6C` 0x79BB5..0x79BCF -> `FUN_00079C50`):
 * the delta is camera minus the committed target; a nonzero delta writes the
 * `+0x8E` facing octant `((angle + 0x40) & 0x3FF) >> 7`. Fixture: camera
 * (0,0,0), target (-0x100, 0) -> delta (+0x100, 0), `fifa96_action_kick_angle`
 * 0x100, octant 2 (test_action_handlers pins the angle). Active records keep
 * their id when it is 0 or 0x62..0x65 (the derived keep rule); a non-special
 * id falls to the OL-52 fallback 0. */
static void test_kickoff_place_face_and_selector(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  struct fifa96_match_entity *e = &pool.team[TEAM0].records[5];
  e->target_x = -0x100;
  e->target_z = 0;
  e->active = 1;
  e->anim_id = 0x62;   /* special: kept by the derived selector */
  e->frame = 9;        /* reset by the selector's BX=0 frame request */
  e->type = 7;
  struct fifa96_match_entity *plain = &pool.team[TEAM0].records[6];
  plain->active = 1;
  plain->anim_id = 0x28;   /* non-special: OL-52 fallback 0 */
  assert(fifa96_match_entities_kickoff_place(&pool, 0, 0, 0) == FIFA96_OK);
  assert(e->type == 2);            /* camera - target = (+0x100, 0) -> octant */
  assert(e->anim_id == 0x62);      /* active keep rule */
  assert(e->frame == 0);
  assert(plain->anim_id == 0);     /* active non-special: derived fallback */
  assert(pool.team[TEAM1].records[0].type == 0);   /* zero delta: untouched */
}

/* FU-96 leg 5 (M2 interactive T2): the per-record camera place
 * `FUN_00079F3C`. The derived sequence snaps a *non-controlled* record whose
 * octagonal camera distance is <= 0x180 onto the 0x180 ring along its current
 * direction, through the native primitives (`FUN_0008DCD4` metric,
 * `FUN_000CD474` angle, the `0x114E04` sine fold, the `(a*b+0x8000)>>16`
 * multiply). Hand-derived anchors: (0,0x60) -> (0,0x180); (0x60,0) ->
 * (0x180,0); the two 45-degree rays -> (+/-272,272); (0,-0x60) -> (0,-0x180)
 * (direction preserved, the record stays behind the camera). */
static void test_camera_place_snaps_near_non_controlled_records(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entity *c0;
  struct fifa96_match_entity *o0;
  struct fifa96_match_entity *o1;
  struct fifa96_match_entity *o2;
  struct fifa96_match_entity *o3;
  struct fifa96_match_entity *o4;
  struct fifa96_match_entity *o5;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  c0 = &pool.team[TEAM0].records[0];
  o0 = &pool.team[TEAM1].records[0];
  o1 = &pool.team[TEAM1].records[1];
  o2 = &pool.team[TEAM1].records[2];
  o3 = &pool.team[TEAM1].records[3];
  o4 = &pool.team[TEAM1].records[4];
  o5 = &pool.team[TEAM1].records[5];
  /* The controlled team is skipped even inside the radius (0x79F57). */
  c0->target_x = 0;
  c0->target_z = 0x60;
  /* Straight ahead: the +z ray at the ring radius. */
  o0->target_z = 0x60;
  /* Lateral: the +x ray. */
  o1->target_x = 0x60;
  /* The 45-degree rays (the 0x114E04 sine fold anchors). */
  o2->target_x = 0x60;
  o2->target_z = 0x60;
  o3->target_x = -0x60;
  o3->target_z = 0x60;
  /* Behind the camera: the direction is preserved, not flipped. */
  o4->target_z = -0x60;
  /* Beyond the octagonal 0x180 gate: fast length 0x183 (320 + 3/8*180) > 0x180
   * while the euclidean length is 0x16F, so this pins the native metric. */
  o5->target_x = 320;
  o5->target_z = 180;

  assert(fifa96_match_entities_camera_place(&pool, 0, 1, 0, 0) == FIFA96_OK);
  assert(c0->target_x == 0 && c0->target_z == 0x60);
  assert(o0->target_x == 0 && o0->target_z == 0x180);
  assert(o1->target_x == 0x180 && o1->target_z == 0);
  assert(o2->target_x == 272 && o2->target_z == 272);
  assert(o3->target_x == -272 && o3->target_z == 272);
  assert(o4->target_x == 0 && o4->target_z == -0x180);
  assert(o5->target_x == 320 && o5->target_z == 180);

  assert(fifa96_match_entities_camera_place(NULL, 0, 1, 0, 0) == -FIFA96_ERR_INVALID);

  /* The camera is the dword triple: the ring is centred on it, not on the
   * origin (0x79FD8/0x7A016 add to the `[0x15774C]`/`[0x157754]` dwords). */
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  o0 = &pool.team[TEAM1].records[0];
  o0->target_x = 1000;
  o0->target_z = -2000 + 0x60;
  assert(fifa96_match_entities_camera_place(&pool, 0, 1, 1000, -2000) == FIFA96_OK);
  assert(o0->target_x == 1000 && o0->target_z == -2000 + 0x180);
}

/* FU-96 leg 5: the two native gates. The phase gate is the first-hand
 * `0x1106C3` byte table (active phases 1/4/6/7/8 and the 0x1A..0x1C tail); the
 * side gate is `[0x157AAC]>>24`: with controlled side 1 it is team 1 that is
 * skipped and team 0 that is placed. */
static void test_camera_place_phase_and_side_gates(void) {
  static const uint8_t active[29] = {
    0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
  };
  struct fifa96_match_entities pool;
  for (unsigned phase = 0; phase < 29u; phase++) {
    assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
    pool.team[TEAM1].records[0].target_z = 0x60;
    assert(fifa96_match_entities_camera_place(&pool, 0, (uint8_t)phase, 0, 0) == FIFA96_OK);
    if (active[phase])
      assert(pool.team[TEAM1].records[0].target_z == 0x180);
    else
      assert(pool.team[TEAM1].records[0].target_z == 0x60);
  }
  /* Controlled side 1: team 1 is the controlled team, team 0 is placed. */
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  pool.team[TEAM0].records[0].target_z = 0x60;
  pool.team[TEAM1].records[0].target_z = 0x60;
  assert(fifa96_match_entities_camera_place(&pool, 1, 1, 0, 0) == FIFA96_OK);
  assert(pool.team[TEAM0].records[0].target_z == 0x180);
  assert(pool.team[TEAM1].records[0].target_z == 0x60);
}

/* An unported row is an explicit skip: the callback's
 * -FIFA96_ERR_UNSUPPORTED is tolerated and the chain continues; any other
 * error propagates. */
static int unsupported_cb(void *ctx, struct fifa96_match_entity *e) {
  (void)ctx;
  (void)e;
  return -FIFA96_ERR_UNSUPPORTED;
}

static int broken_cb(void *ctx, struct fifa96_match_entity *e) {
  (void)ctx;
  if (e->team == TEAM1 && e->index == 2) return -FIFA96_ERR_STATE;
  return FIFA96_OK;
}

static void test_update_tolerates_unsupported(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f = zero_frame();
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  f.phase = 0;
  assert(fifa96_match_entities_update(&pool, &f, unsupported_cb, NULL) == FIFA96_OK);
  assert(fifa96_match_entities_update(&pool, &f, broken_cb, NULL) == -FIFA96_ERR_STATE);
  assert(fifa96_match_entities_update(NULL, &f, NULL, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_entities_update(&pool, NULL, NULL, NULL) == -FIFA96_ERR_INVALID);
}

static void test_null_take_guards(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  assert(fifa96_match_entities_take_place(&pool, NULL, NULL, NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_entities_take_place(NULL, NULL, NULL, NULL) == -FIFA96_ERR_INVALID);
}

int main(void) {
  test_init_resets_pool();
  test_release_clears();
  test_record_fields_roundtrip();
  test_install_rejects();
  test_install_stages();
  test_install_coerces_inactive_code3();
  test_install_carrier_bit();
  test_install_skip98_clear();
  test_team_select_nearest();
  test_team_select_none();
  test_team_pick_ranked_lane();
  test_reset_lane_pass();
  test_update_chain_order();
  test_update_selection_buckets();
  test_update_intercept_select();
  test_update_intercept_band_flag();
  test_update_counter_and_timers();
  test_possession_release_decay();
  test_update_consumes_requests();
  test_merge_slot_ranked();
  test_bind_slot();
  test_ball_pair();
  test_ball_state_roundtrip();
  test_place_commits_target();
  test_seed_formation_targets();
  test_formation_iso_fixture();
  test_kickoff_place_commits_records_and_ball();
  test_kickoff_place_face_and_selector();
  test_camera_place_snaps_near_non_controlled_records();
  test_camera_place_phase_and_side_gates();
  test_update_tolerates_unsupported();
  test_null_take_guards();
  puts("test_engine_match_entities OK");
  return 0;
}
