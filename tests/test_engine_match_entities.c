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
 * (FUN_0007DAB4 0x7DAFB -> FUN_0007D9A4 code 0), and the ball/actor state
 * unset. */
static void test_init_resets_pool(void) {
  struct fifa96_match_entities pool;
  memset(&pool, 0xAA, sizeof pool);
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  assert(pool.team[TEAM0].side == 0 && pool.team[TEAM1].side == 1);
  assert(pool.team[TEAM0].target == NONE && pool.team[TEAM1].target == NONE);
  assert(pool.team[TEAM0].second == NONE && pool.team[TEAM0].intercept == NONE);
  assert(pool.team[TEAM0].chosen == NONE);
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
      assert(e->active == 0 && e->ran == 0 && e->carrier == 0);
      assert(e->skip_98 == 0 && e->skip_9a == 0 && e->has_slot == 0);
      assert(e->pos_x == 0 && e->pos_z == 0 && e->target_x == 0);
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
 * +0x71/+0x73, lane +0x69 and the timer cluster +0x7F/+0x81/+0x89/+0x93. */
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
  test_update_chain_order();
  test_update_selection_buckets();
  test_update_intercept_select();
  test_update_counter_and_timers();
  test_update_consumes_requests();
  test_merge_slot_ranked();
  test_ball_pair();
  test_ball_state_roundtrip();
  test_update_tolerates_unsupported();
  test_null_take_guards();
  puts("test_engine_match_entities OK");
  return 0;
}
