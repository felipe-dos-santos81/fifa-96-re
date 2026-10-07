#include "fifa96_loader/fifa96_ball_pairing.h"

#include <string.h>

#include "fifa96_loader/fifa96_arm_helpers.h"
#include "fifa96_loader/fifa96_action_handlers.h"
#include "fifa96_loader/fifa96_rng.h"

int fifa96_ball_pair_offset(const fifa96_ball_pair_vector *from,
                            const fifa96_ball_pair_vector *to,
                            fifa96_ball_pair_delta *out) {
  int16_t dx;
  int16_t dz;
  if (!from || !to || !out) return -FIFA96_ERR_INVALID;
  dx = (int16_t)(to->x - from->x);
  dz = (int16_t)(to->z - from->z);
  out->dx = dx;
  out->dz = dz;
  out->distance = (int16_t)fifa96_entity_distance(dx, dz);
  return FIFA96_OK;
}

int fifa96_ball_pair_decide(const fifa96_ball_pair_actor *interceptor,
                            const fifa96_ball_pair_actor *opponent,
                            int16_t delta,
                            fifa96_ball_pair_vector *out_position) {
  fifa96_ball_pair_vector predicted;
  fifa96_ball_pair_delta current;
  fifa96_ball_pair_delta closing;
  if (!interceptor || !opponent || !out_position) return -FIFA96_ERR_INVALID;
  predicted.x = (int16_t)(interceptor->position.x +
                          (int32_t)interceptor->velocity_x * delta);
  predicted.height = interceptor->position.height;
  predicted.z = (int16_t)(interceptor->position.z +
                          (int32_t)interceptor->velocity_z * delta);
  if (fifa96_ball_pair_offset(&opponent->position, &interceptor->position,
                              &current) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  if (fifa96_ball_pair_offset(&opponent->position, &predicted, &closing) !=
      FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  if (closing.distance < current.distance && closing.distance < 0x40) {
    out_position->x =
        (int16_t)(opponent->position.x + (closing.dx < 0 ? -0x40 : 0x40));
    out_position->height = opponent->position.height;
    out_position->z =
        (int16_t)(opponent->position.z + (closing.dz < 0 ? -0x40 : 0x40));
    return 1;
  }
  return 0;
}

int fifa96_ball_pair_receive(const fifa96_ball_pair_actor *actor,
                             const fifa96_entity_candidate *candidates,
                             uint32_t count, int16_t target_x,
                             int16_t target_y, int32_t *receiver_index) {
  uint32_t skip = 0;
  int16_t best = 0;
  int index;
  if (!actor || !candidates || !receiver_index) return -FIFA96_ERR_INVALID;
  if (actor->kind == 1 || actor->action == 0x10 || actor->action == 0x11 ||
      actor->action == 0x12)
    skip = (uint16_t)(int16_t)(int8_t)actor->flag;
  index =
      fifa96_entity_find_nearest(candidates, count, skip, target_x, target_y,
                                 &best);
  if (index < 0) {
    *receiver_index = -1;
    return 0;
  }
  *receiver_index = (int32_t)index;
  return 1;
}

int fifa96_ball_pair_assign(fifa96_ball_pair_targets *targets, int team,
                            int32_t receiver_index) {
  if (!targets || (team != 0 && team != 1)) return -FIFA96_ERR_INVALID;
  if (team == 0) {
    targets->team0_target = receiver_index;
    targets->team0_second = 0;
    targets->team1_target = 0;
    targets->team1_second = 0;
  } else {
    targets->team0_target = 0;
    targets->team0_second = 0;
    targets->team1_target = receiver_index;
    targets->team1_second = 0;
  }
  return FIFA96_OK;
}

int fifa96_ball_pair_possess(fifa96_ball_pair_actor *actor) {
  if (!actor) return -FIFA96_ERR_INVALID;
  actor->has_ball = 1;
  return FIFA96_OK;
}

int fifa96_ball_pair_release(fifa96_ball_pair_actor *actor) {
  if (!actor) return -FIFA96_ERR_INVALID;
  actor->has_ball = 0;
  return FIFA96_OK;
}

int fifa96_ball_pair_clear(fifa96_ball_pair_state *state) {
  if (!state) return -FIFA96_ERR_INVALID;
  state->actor = 0;
  state->receiver = 0;
  state->vector.x = 0;
  state->vector.height = 0;
  state->vector.z = 0;
  state->traj = 0;
  state->angle = 0;
  state->flags = 0x20;
  state->code = 2;
  state->sub_code = 0;
  state->reserved45 = 0;
  state->ack = 0;
  return FIFA96_OK;
}

int fifa96_ball_pair_stage(fifa96_ball_pair_state *state, int32_t actor,
                           const fifa96_ball_pair_vector *vector, int16_t traj,
                           uint8_t code) {
  if (!state || !vector) return -FIFA96_ERR_INVALID;
  state->actor = actor;
  state->vector = *vector;
  state->traj = traj;
  state->code = code;
  return FIFA96_OK;
}

int fifa96_ball_pair_receive_target(const fifa96_ball_pair_vec3i *base,
                                    int32_t lead_x, int32_t lead_z,
                                    fifa96_ball_pair_vec3i *out) {
  if (!base || !out) return -FIFA96_ERR_INVALID;
  out->x = (int32_t)((uint32_t)base->x +
                     ((uint32_t)(lead_x >> 16) << 5));
  out->y = base->y;
  out->z = (int32_t)((uint32_t)base->z +
                     ((uint32_t)(lead_z >> 16) << 5));
  return FIFA96_OK;
}

int fifa96_ball_pair_stage_tail(fifa96_ball_pair_state *state,
                                fifa96_ball_stage_tail_actor *actor,
                                const uint8_t *recompute_table,
                                fifa96_ball_stage_tail_out *out) {
  int8_t code;
  if (!state || !actor || !recompute_table || !out) return -FIFA96_ERR_INVALID;
  memset(out, 0, sizeof *out);
  out->face = actor->facing;   /* the resulting +0x8E byte when no face runs */
  code = (int8_t)state->code;
  /* 0x7A8D1..0x7A8DC: MOVSX DX,[0x158743]; TEST DX,DX; JL -> 0x7A8EF.
   * 0x7A8DE..0x7A8ED: CMP EAX,0xF / JGE, then CMP byte[EAX+0x1104BB],0. */
  if (code < 0 || code >= 0x0F || recompute_table[(uint8_t)code] == 0) {
    out->receive = 1;     /* 0x7A8EF CALL 0x7A084 (unported, OL-62) */
  } else {
    out->recompute = 1;
    /* 0x7A8F6..0x7A91A: the sign-extended word[+0x6B] gate, then the two
     * unaligned dword addends shifted right 17 and added to +0x59/+0x61. */
    if (actor->lane_gate < 0x60) {
      actor->pos_x = (int32_t)((uint32_t)actor->pos_x + (uint32_t)(actor->nudge_x >> 17));
      actor->pos_z = (int32_t)((uint32_t)actor->pos_z + (uint32_t)(actor->nudge_z >> 17));
      out->nudge = 1;
    }
    out->camera_zero = 1; /* 0x7A91D..0x7A92D: 0x1577BE/C0/C2 = 0 (derived) */
  }
  /* 0x7A934..0x7A941: byte [EBP+0x8D] == 0. */
  if (actor->active == 0) {
    if (code == 2) {
      state->sub_code = 0x30;   /* 0x7A949 */
    } else if (code == 1 || code == 3 || code == 6) {
      state->sub_code = 0x31;   /* 0x7A964 */
    } else if (code == 7 || code == 4 || code == 5) {
      /* 0x7A97F..0x7A9D9: the inactive whole-block reset (FU-73 §1 clear). */
      out->cleared = 1;
      return fifa96_ball_pair_clear(state);
    }
  }
  /* 0x7A9DE..0x7AA0C: codes 1/2/3/6 call 0x79C50 with DX/BX = the staged
   * vector's second/third words (`0x7A9F5..0x7AA09`: dword 0x158738 >> 16 and
   * dword 0x15873A >> 16). */
  if (code == 1 || code == 2 || code == 3 || code == 6) {
    const fifa96_arm_vec from = {0, 0, 0};
    fifa96_arm_vec to;
    to.x = state->vector.height;
    to.y = 0;
    to.z = state->vector.z;
    if (fifa96_arm_face(&from, &to, &out->face) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    actor->facing = out->face;
  }
  /* 0x7AA0E..0x7AA2B: EDX = (int8)[0x158744] sub-code, EBX = (int8)[0x158745]
   * (the derived helper's row stand-in is 0; the native [rec+0x28] source and
   * RNG reroll stay OL-52), ECX = type8, then CALL 0x6E598. */
  if (fifa96_arm_anim_select(state->sub_code, 0, &out->anim) != FIFA96_OK) {
    return -FIFA96_ERR_INVALID;
  }
  /* 0x7AA30..0x7AA39: MOV EAX,[EBP+0x20]; TEST; JZ; CALL 0x78B00. */
  if (actor->has_slot != 0) out->slot_cb = 1;
  /* The 0x7AA3C..0x7AE2F per-code target algebra is unported (OL-62). */
  return FIFA96_OK;
}

/* ===== FU-139 §9 (Task 11): the FUN_0007B9C4 kick path =====
 *
 * First-hand evidence: /FIFA96.EXE, `disassemble_bytes` `0x7B9C4..0x7BC34` and
 * `0x7BC34..0x7BF17` (337 insns), `0x7B194..0x7B44F` + `0x7B44F..0x7B57C`
 * (the mode-0x40 arm, 219 insns) and `0x7B57C..0x7B878` (the mode arm,
 * 235 insns), `0x7B878..0x7B9C4` (the direction/vector helper, 109 insns);
 * `read_memory 0x114E04` (the 257-dword sine table, values 0, 402, 804, 1206,
 * 1608, ... matching the FU-88 table) and `0x14C1D4` (all-zero per-side range
 * words in the image). Ghidra read-only. */

/* 0x114E04: the 257-entry sine table (same values as the FU-88 projection
 * table; first-hand read of the first 16 dwords). The native fold indexes it
 * directly (no interpolation). */
static const int32_t kick_sin_table[257] = {
    0, 402, 804, 1206, 1608, 2010, 2412, 2814,
    3215, 3617, 4018, 4420, 4821, 5222, 5622, 6023,
    6423, 6823, 7223, 7623, 8022, 8421, 8819, 9218,
    9616, 10013, 10410, 10807, 11204, 11600, 11995, 12390,
    12785, 13179, 13573, 13966, 14359, 14751, 15142, 15533,
    15923, 16313, 16702, 17091, 17479, 17866, 18253, 18638,
    19024, 19408, 19792, 20175, 20557, 20938, 21319, 21699,
    22078, 22456, 22833, 23210, 23586, 23960, 24334, 24707,
    25079, 25450, 25820, 26189, 26557, 26925, 27291, 27656,
    28020, 28383, 28745, 29105, 29465, 29824, 30181, 30538,
    30893, 31247, 31600, 31952, 32302, 32651, 32999, 33346,
    33692, 34036, 34379, 34721, 35061, 35400, 35738, 36074,
    36409, 36743, 37075, 37406, 37736, 38064, 38390, 38716,
    39039, 39361, 39682, 40002, 40319, 40636, 40950, 41263,
    41575, 41885, 42194, 42501, 42806, 43110, 43412, 43712,
    44011, 44308, 44603, 44897, 45189, 45480, 45768, 46055,
    46340, 46624, 46906, 47186, 47464, 47740, 48015, 48288,
    48558, 48828, 49095, 49360, 49624, 49886, 50145, 50403,
    50659, 50914, 51166, 51416, 51665, 51911, 52155, 52398,
    52639, 52877, 53114, 53348, 53581, 53811, 54040, 54266,
    54491, 54713, 54933, 55152, 55368, 55582, 55794, 56004,
    56212, 56417, 56621, 56822, 57022, 57219, 57414, 57606,
    57797, 57986, 58172, 58356, 58538, 58718, 58895, 59070,
    59243, 59414, 59583, 59749, 59913, 60075, 60235, 60392,
    60547, 60700, 60850, 60998, 61144, 61288, 61429, 61568,
    61705, 61839, 61971, 62100, 62228, 62353, 62475, 62596,
    62714, 62829, 62942, 63053, 63162, 63268, 63371, 63473,
    63571, 63668, 63762, 63854, 63943, 64030, 64114, 64197,
    64276, 64353, 64428, 64501, 64571, 64638, 64703, 64766,
    64826, 64884, 64939, 64992, 65043, 65091, 65136, 65179,
    65220, 65258, 65294, 65327, 65358, 65386, 65412, 65436,
    65457, 65475, 65491, 65505, 65516, 65524, 65531, 65534,
    65536,
};

/* The `0x7BD97..0x7BDB6` (and `0x7B80A..0x7B827`, `0x7B4EB..0x7B508`,
 * `0x7B83B..0x7B85C`, `0x7B51A..0x7B546`) byte-shift quadrant idiom over
 * 0x114E04 decoded to the exact equivalent: bit 8 of the angle negates the
 * table index (`idx = 0x100 - (angle & 0xFF)`, giving 0x100 when the low byte
 * is 0 -- the table's 257th entry) and bit 9 negates the value. The three
 * native copies (B9C4 x2, B194 x2, B57C x2) are identical. */
static int32_t kick_sin(int32_t angle) {
  uint32_t u = (uint32_t)angle;
  int32_t idx = (int32_t)(u & 0xFFu);
  int32_t bit8 = (int32_t)((u >> 8) & 1u);
  int32_t bit9 = (int32_t)((u >> 9) & 1u);
  int32_t v;
  idx = (int32_t)(((uint32_t)idx ^ (0u - (uint32_t)bit8)) & 0xFFu);
  idx += bit8;   /* 0x7BDA9 SUB EAX,ECX with ECX = -bit8 */
  v = kick_sin_table[idx];
  return bit9 ? -v : v;
}

/* `FUN_000795A4` (`0x795A4..0x795C2`): `(int64)a * b + 0x8000 >> 16`, the low
 * word stored. Public so the FU-139 §11 row-06 pursuit machine (and any later
 * `0x114E04` consumer) shares this exact fold instead of duplicating the
 * table and the quadrant decode. */
int16_t fifa96_ball_fold(int32_t speed, int32_t angle) {
  int64_t v = ((int64_t)speed * (int64_t)kick_sin(angle) + 0x8000) >> 16;
  return (int16_t)(uint16_t)(uint32_t)(int32_t)v;
}

/* 32-bit IMUL low-word store (the native `IMUL` + word store pairs). */
static int16_t kick_low_mul(int32_t a, int32_t b) {
  return (int16_t)(uint16_t)((uint32_t)a * (uint32_t)b);
}

/* `FUN_0008DCD4` (`0x8DCD4..0x8DD5B`): camera base vs a local target into the
 * `{word distance, word dx, word dz}` triple (FU-142 Appendix C.3, reused via
 * the tested `fifa96_arm_dist_stage`). */
static void kick_triple(const fifa96_ball_kick_ctx *ctx, int32_t to_x, int32_t to_z,
                        fifa96_ball_pair_vector *out) {
  fifa96_arm_vec from;
  fifa96_arm_vec to;
  int32_t distance = 0;
  int32_t lane = 0;
  from.x = ctx->camera_x;
  from.y = ctx->camera_y;
  from.z = ctx->camera_z;
  to.x = to_x;
  to.y = 0;
  to.z = to_z;
  (void)fifa96_arm_dist_stage(&from, &to, &distance, &lane);
  out->x = (int16_t)distance;
  out->height = (int16_t)((uint16_t)to_x - (uint16_t)ctx->camera_x);
  out->z = (int16_t)lane;
}

/* The `FUN_0007AE70` event view the kick path builds (same fields as
 * `fifa96_action_kick_event`; `x`/`z` are the caller's resolver words). */
static void kick_event(const fifa96_ball_kick_actor *actor,
                       const fifa96_ball_kick_slot *slot,
                       const fifa96_ball_kick_ctx *ctx, uint8_t code, int16_t x,
                       int16_t z, fifa96_action_kick_event *event) {
  memset(event, 0, sizeof *event);
  event->code = code;
  event->subtype = actor->actor_type;   /* [actor+0x8B]>>24 */
  event->sector_byte = actor->facing;   /* [actor+0x8E] low byte */
  event->active = actor->active;
  event->has_slot = slot->present;
  event->slot_counter = slot->counter23;
  event->phase = ctx->phase;
  event->x = x;
  event->z = z;
  event->ball_height = ctx->ball_height;
  event->height = actor->pos_y;
}

/* `FUN_0007B878` (`0x7B878..0x7B9C3`, 109 insns): resolve the event row for the
 * direction pair and build the staged vector's speed/dx/dz. The resolver is
 * called with `code = (int16)slot_word6` and `x/z = dir_x/dir_z` (`0x7B8AD`);
 * the row pointer NULL arm zeroes the triple (`0x7B9AD`). The speed is
 * `min(slot23, 0x3C)^2 * (int8)row[1]` clamped to `row[2]..row[4]` (the
 * `0x14C1D4` side range 0x10 bit + slot word 0x50 bit extend row bounds by
 * 1.5x at `0x7B90C..0x7B93D`), the diagonal `*0xB5>>8` at `0x7B971`, then
 * dx/dz and `FUN_0008DC68` distance. */
static fifa96_err_t kick_dir_vector(const fifa96_ball_kick_actor *actor,
                                    const fifa96_ball_kick_slot *slot,
                                    const fifa96_ball_kick_ctx *ctx, int8_t dir_x,
                                    int8_t dir_z, fifa96_ball_pair_vector *out) {
  fifa96_action_kick_event event;
  fifa96_action_kick_event_out pick;
  const uint8_t *row;
  int32_t counter;
  int32_t lo;
  int32_t row2;
  int32_t row4;
  int32_t product;
  int32_t speed;
  if (!ctx->sector_table) return -FIFA96_ERR_INVALID;
  kick_event(actor, slot, ctx, (uint8_t)slot->word6, dir_x, dir_z, &event);
  if (fifa96_action_kick_event_row(&event, ctx->sector_table, &pick) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  if (pick.found == 0 || ctx->event_rows[pick.table] == NULL) {
    out->x = 0;
    out->height = 0;
    out->z = 0;
    return FIFA96_OK;
  }
  row = ctx->event_rows[pick.table] + 10u * pick.index;
  /* 0x7B8C1..0x7B8D5: counter = min(slot[+0x23], 0x3C). */
  counter = slot->counter23 > 0x3Cu ? 0x3Cu : (int32_t)slot->counter23;
  lo = (int32_t)(int8_t)row[1];                                  /* 0x7B8D5 */
  row2 = (int32_t)(int16_t)(uint16_t)(row[2] | ((uint16_t)row[3] << 8));
  row4 = (int32_t)(int16_t)(uint16_t)(row[4] | ((uint16_t)row[5] << 8));
  /* 0x7B90C..0x7B93D: the 1.5x extension. */
  if ((ctx->side_range & 0x10u) != 0 && (slot->word6 & 0x50) != 0) {
    lo += lo >> 1;
    row2 += row2 >> 1;
    row4 += row4 >> 1;
  }
  /* 0x7B93F..0x7B960: product = counter^2 * lo (low 16 compared signed). */
  product = (int32_t)(uint16_t)((uint32_t)((uint32_t)counter * (uint32_t)counter) *
                                (uint32_t)lo);
  if (product >= row2) {
    speed = (product <= row4) ? product : row4;
  } else {
    speed = row2;
  }
  /* 0x7B962..0x7B97C: diagonal scaling. */
  if (dir_x != 0 && dir_z != 0)
    speed = (int32_t)(uint16_t)((int32_t)(uint16_t)speed * 0xB5) >> 8;
  out->height = kick_low_mul(dir_x, speed);
  out->z = kick_low_mul(dir_z, speed);
  out->x = (int16_t)fifa96_entity_distance(out->height, out->z);
  return FIFA96_OK;
}

/* `FUN_0007B194` (`0x7B194..0x7B556`, 219 insns): the active mode-0x40 arm.
 * Builds a goal-line target ({x, y, ±0xB40}) from 12 RNG draws (no slot) or
 * the slot/mode-state clusters, runs `0x8DCD4(camera, target, vector)`,
 * re-derives the speed word `(int8)[rec[+4][0x10]]<<7 + 0x390 + (rng&0x3F)`
 * (halved when `+0x99`, 1.5x when the side range 0x10 bit), folds
 * `FUN_000CD474(distance, dx)` with the goal-side drift and the `0x14C2F6`
 * signed adjust, then writes dx/dz through the 0x114E04 fold. The `0x14C2F6`
 * gate and the goal side are caller inputs; `mode_state` is `[0x157A4D]`. */
static fifa96_err_t kick_arm_40(const fifa96_ball_kick_actor *actor,
                                const fifa96_ball_kick_slot *slot,
                                const fifa96_ball_kick_ctx *ctx,
                                struct fifa96_rng *rng,
                                fifa96_ball_pair_state *state) {
  fifa96_ball_pair_vector local;
  int32_t traj;
  int32_t angle = 0;
  int32_t vx;
  int32_t goal_z;
  int32_t drift;
  uint16_t r;
  int slot_mode6 = (ctx->mode_state == 6 || ctx->mode_state == 0x10);
  local.height = 0;
  local.z = (int16_t)((actor->sub_phase1 != 0 || actor->side == 0) ? 0xB40 : -0xB40);
  if (!slot->present) {
    if (actor->sub_phase1 != 0) {
      if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      local.x = (int16_t)((int16_t)(r & 1u) * 0x120) - 0x90;
    } else {
      uint16_t r2, r3;
      if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      if (fifa96_rng_step(rng, &r2) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      if ((r2 & 0xFu) == 0) {
        if (fifa96_rng_step(rng, &r3) != FIFA96_OK) return -FIFA96_ERR_INVALID;
        local.x = (int32_t)(r3 & 0x1FFu);
      } else {
        if (fifa96_rng_step(rng, &r3) != FIFA96_OK) return -FIFA96_ERR_INVALID;
        local.x = (int32_t)(r3 & 0x1Fu) + 0x80;
      }
      if (fifa96_rng_step(rng, &r3) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      if (r3 & 1u) local.x = -local.x;
    }
    if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    if ((r & 0xFu) == 0) {
      if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      traj = (int32_t)(r & 0x7Fu) + 0xA0;
    } else {
      uint16_t r2;
      if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      if (fifa96_rng_step(rng, &r2) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      traj = (int32_t)(r2 & 0x1Fu) + (int32_t)(r & 0x7Fu) + 0x20;
    }
  } else {
    if (slot_mode6) {
      if (slot->dir_x == 0) {
        if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
        local.x = (int32_t)(r & 0x7Fu) - 0x40;
      } else {
        int32_t divisor = ((int32_t)actor->desc_e + 0xA) >> 1;
        if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
        if (divisor != 0 && (int32_t)(r % (uint32_t)divisor) == 0) {
          local.x = (int32_t)slot->anim_1d * 0xF0;
        } else {
          if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
          local.x = (int32_t)slot->anim_1d * ((int32_t)(r & 0x1Fu) + 0x88);
        }
      }
    } else if (slot->dir_x == 0) {
      if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      local.x = (int32_t)(r & 0x1Fu);
      if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      if (r & 1u) local.x = -local.x;
    } else {
      if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      local.x = (int32_t)slot->anim_1d * ((int32_t)(r & 0x3Fu) + 0xA0);
    }
    if (slot_mode6) {
      if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      if ((r & 0xFu) == 0 && slot->dir_z == 1) {
        traj = 0xC0;
      } else {
        if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
        traj = (int32_t)(r & 0xFu) + (int32_t)slot->dir_z * 0x3C + 0x10;
      }
    } else {
      if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      traj = ((int32_t)(r & 0xFu) + 0x30) * (int32_t)slot->dir_z + 0x48;
    }
  }
  if ((int16_t)traj < 0x20) traj = 0x20;
  state->traj = (int16_t)traj;
  kick_triple(ctx, local.x, local.z, &state->vector);
  if (fifa96_action_kick_angle(state->vector.height, state->vector.z, &angle) !=
      FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  vx = (int32_t)actor->desc_10 * 0x80 + 0x390 + (int32_t)(r & 0x3Fu);
  if (actor->byte_99 != 0) {
    vx = (int16_t)vx >> 1;                               /* 0x7B405 SAR BX,1 */
  } else if ((ctx->side_range & 0x10u) != 0) {
    vx = (int16_t)((int16_t)vx + ((int16_t)vx >> 1));    /* 0x7B432..0x7B43B */
  }
  state->vector.x = (int16_t)vx;
  goal_z = (actor->sub_phase1 != 0 || actor->side == 0) ? 0xB10 : -0xB10;
  drift = (int16_t)((uint16_t)goal_z - (uint16_t)actor->pos_z);
  if (drift < 0) drift = -drift;
  drift >>= 7;
  if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  if (r & 1u) drift = -drift;
  angle = (int16_t)((uint16_t)angle + (uint16_t)drift);
  if (ctx->goal_gate == 1) {
    uint32_t gate = (uint32_t)actor->anim_9d | (uint32_t)(uint8_t)actor->desc_e;
    if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    if ((int32_t)(r & 0xFu) > (int32_t)gate) {
      int32_t adjust = 0x10 - ((int32_t)actor->anim_9d | (int32_t)actor->desc_11);
      if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      if (r & 1u) adjust = -adjust;
      angle = (int16_t)((uint16_t)angle + (uint16_t)adjust);
      angle = (int16_t)((uint16_t)angle & 0x3FFu);
      if (angle > 0x200) angle = (int16_t)(angle - 0x400);
    }
  }
  state->vector.height = fifa96_ball_fold(state->vector.x, angle);
  state->vector.z = fifa96_ball_fold(state->vector.x, angle + 0x100);
  return FIFA96_OK;
}

/* `FUN_0007B57C` (`0x7B57C..0x7B877`, 235 insns): the second mode arm. The
 * target starts at the camera plus the staged vector's dx/dz (or camera z
 * minus 0x60) and, when the nearest team record is a decoy within the angle
 * gate, its velocity-projected position replaces the vector; otherwise the
 * goal-line fallback (`±(0xB10 + rng&0xF)`, `±(0xD0 - rng&0x1F)`) is staged
 * with speed 0x5A0 and the 0x114E04 fold. `si` is the native EBX (0 for the
 * inactive arm, L1 for the active one). `0x14C326 > 0` returns unchanged. */
static fifa96_err_t kick_arm_20(const fifa96_ball_kick_actor *actor,
                                const fifa96_ball_kick_slot *slot,
                                const fifa96_ball_kick_ctx *ctx,
                                struct fifa96_rng *rng,
                                fifa96_ball_pair_state *state, int16_t si) {
  int32_t tx;
  int32_t tz;
  int32_t skip = -1;
  int32_t index;
  int16_t best = 0;
  uint16_t r;
  if (ctx->arm_gate > 0) return FIFA96_OK;
  if (si == 0) {
    tx = ctx->camera_x + (int16_t)state->vector.height;
    tz = ctx->camera_z + (int16_t)state->vector.z;
    if (actor->type != 0x12 && actor->type != 0x10 && actor->type != 0x11) {
      if (slot->present) {
        /* EDX stays -1 (0x7B607 -> 0x7B630) */
      } else {
        int32_t gate = (int32_t)actor->desc_15 | (int32_t)actor->anim_9d;
        if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
        if ((int32_t)(r & 0xFu) > gate) return FIFA96_OK;
        skip = (int32_t)(int16_t)actor->active;
      }
    } else {
      skip = (int32_t)(int16_t)actor->active;
    }
  } else {
    tx = ctx->camera_x + (int16_t)state->vector.height;
    tz = ctx->camera_z - 0x60;
    skip = (int32_t)(int16_t)actor->active;
  }
  index = fifa96_entity_find_nearest(ctx->candidates, ctx->candidate_count,
                                     (uint32_t)(uint16_t)(int16_t)skip, (int16_t)tx,
                                     (int16_t)tz, &best);
  if (index >= 0 && index != ctx->self_index) {
    const fifa96_ball_kick_candidate *found;
    fifa96_ball_pair_vector probe;
    int32_t angle1 = 0;
    int32_t angle2 = 0;
    if (ctx->team_records == NULL) return -FIFA96_ERR_INVALID;
    found = &ctx->team_records[index];
    if (fifa96_action_kick_angle(state->vector.height, state->vector.z, &angle1) !=
        FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    kick_triple(ctx, found->pos_x + ((int32_t)found->vel_x << 5),
                found->pos_z + ((int32_t)found->vel_z << 5), &probe);
    if (fifa96_action_kick_angle(probe.height, probe.z, &angle2) != FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    angle2 = (int16_t)((uint16_t)angle2 - (uint16_t)angle1);
    angle2 = (int16_t)((uint16_t)angle2 & 0x3FFu);
    if (angle2 > 0x200) angle2 = (int16_t)(angle2 - 0x400);
    /* 0x7B6C3..0x7B6CB: si==0 and a wide angle returns (found != actor). */
    if (si == 0 && angle2 >= 0x80) return FIFA96_OK;
    state->vector = probe;
    if (slot->present) return FIFA96_OK;
    /* 0x7B6DE..0x7B70B: the band over the copied vector.x. */
    if ((int16_t)state->vector.x < 0x5A0) state->flags = 0x20;
    else if ((int16_t)state->vector.x < 0x780) state->flags = 0x30;
    else state->flags = 0x10;
    return FIFA96_OK;
  }
  {
    int32_t abs_x = actor->pos_x < 0 ? -actor->pos_x : actor->pos_x;
    if (abs_x >= 0x5A0) return FIFA96_OK;
  }
  if (actor->side != 0) {
    if ((int16_t)actor->pos_z >= -0x750) return FIFA96_OK;
    if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    tz = -0xB10 - (int32_t)(r & 0xFu);
  } else {
    if ((int16_t)actor->pos_z <= 0x750) return FIFA96_OK;
    if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    tz = 0xB10 + (int32_t)(r & 0xFu);
  }
  tx = ctx->camera_x + (int16_t)state->vector.height;
  if (tx > 0) {
    if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    tx = 0xD0 - (int32_t)(r & 0x1Fu);
  } else {
    if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    tx = (int32_t)(r & 0x1Fu) - 0xD0;
  }
  if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  if ((int32_t)(r & 0xFu) > (int32_t)actor->desc_e) {
    if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    if (r & 1u) tx += 0x30;
    else tx -= 0x30;
  }
  kick_triple(ctx, tx, tz, &state->vector);
  state->vector.x = 0x5A0;
  {
    int32_t angle = 0;
    if (fifa96_action_kick_angle(state->vector.height, state->vector.z, &angle) !=
        FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    state->vector.height = fifa96_ball_fold(0x5A0, angle);
    state->vector.z = fifa96_ball_fold(0x5A0, angle + 0x100);
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_ball_kick_target(fifa96_ball_pair_state *state,
                                     fifa96_ball_kick_actor *actor,
                                     fifa96_ball_kick_slot *slot,
                                     const fifa96_ball_pair_vector *input,
                                     const fifa96_ball_kick_ctx *ctx,
                                     struct fifa96_rng *rng, uint8_t mode,
                                     fifa96_ball_kick_out *out) {
  uint8_t flags_now;
  uint8_t l1 = 0;
  uint8_t l2 = 0;
  int32_t type8;
  fifa96_action_kick_event event;
  fifa96_action_kick_event_out pick;
  const uint8_t *row;
  int16_t lo;
  int16_t hi;
  int16_t add;
  int32_t divisor;
  int32_t user_extend = 0;
  uint8_t code;
  if (!state || !actor || !slot || !ctx || !rng || !out) return -FIFA96_ERR_INVALID;
  memset(out, 0, sizeof *out);
  /* 0x7B9D4..0x7B9E5: actor latch + mode store. */
  if (actor->id != 0) state->actor = actor->id;
  else actor->id = state->actor;
  state->flags = mode;
  /* 0x7B9EB..0x7BA17: the 6-byte vector copy (or zero) and traj zero. */
  if (input != NULL) {
    state->vector = *input;
  } else {
    state->vector.x = 0;
    state->vector.height = 0;
    state->vector.z = 0;
  }
  state->traj = 0;
  if (slot->present) {
    int32_t mode8 = (int32_t)(int8_t)mode;
    /* 0x7BA29..0x7BA46: latch the signed mode into word[slot+6]. */
    if (mode8 != (int32_t)slot->word6) slot->word6 = (int16_t)mode8;
    /* 0x7BA4A..0x7BA85: L1. */
    if (actor->active != 0 && ctx->phase == 2 && (slot->word6 & 0x20) != 0 &&
        slot->counter23 < 7u)
      l1 = 1;
    /* 0x7BA8A..0x7BAB2: L2. */
    l2 = (ctx->phase == 2 && (slot->word6 & 0x10) != 0) ? 1u : 0u;
    /* 0x7BABD..0x7BB11: the wing target runs only for L2 && active with
     * |pos_x| > 0x420 and the side/pos_z gate; every other case takes the
     * normal dir arm at 0x7BB4B. */
    if (l2 != 0 && actor->active != 0) {
      int32_t abs_x = actor->pos_x < 0 ? -actor->pos_x : actor->pos_x;
      int wing = 0;
      if (abs_x > 0x420) {
        if (actor->side == 0) {
          if ((int16_t)actor->pos_z > 0x7B0) wing = 1;
        } else if (actor->side == 1) {
          if ((int16_t)actor->pos_z < -0x7B0) wing = 1;
        }
      }
      if (wing) {
        fifa96_ball_pair_vector local;
        local.x = (int16_t)(actor->pos_x < 0 ? 0xF0 : -0xF0);
        local.height = 0;
        local.z = (int16_t)actor->pos_z;
        kick_triple(ctx, local.x, local.z, &state->vector);
        goto kick_modes;
      }
    }
    {
      int8_t dir_x;
      int8_t dir_z;
      /* 0x7BB4B..0x7BB67: the slot direction pair. */
      dir_x = slot->dir_x;
      dir_z = slot->dir_z;
      if (dir_x == 0 && dir_z == 0) {
        if (actor->active == 0) goto kick_type_dir;   /* 0x7BB67 JZ 0x7BBB9 */
        if ((state->flags & 0x30) != 0 || ctx->ball_height != 0 ||
            actor->pos_y != 0) {
          /* 0x7BBAE..0x7BBB7: L2 with a grounded record takes the (zero)
           * slot direction arm, else the type table. */
          if (l2 != 0 && actor->pos_y == 0) {
            fifa96_err_t rc = kick_dir_vector(actor, slot, ctx, 0, 0,
                                              &state->vector);
            if (rc != FIFA96_OK) return rc;
            goto kick_band;
          }
          goto kick_type_dir;
        }
        if (actor->type == 0x12 || actor->type == 0x10 || actor->type == 0x11) {
          goto kick_type_dir;
        }
        out->staged = 0;
        return FIFA96_OK;
      }
      {
        fifa96_err_t rc = kick_dir_vector(actor, slot, ctx, dir_x, dir_z,
                                          &state->vector);
        if (rc != FIFA96_OK) return rc;
      }
      goto kick_band;
    kick_type_dir:
      if (ctx->type_dir_x == NULL || ctx->type_dir_z == NULL)
        return -FIFA96_ERR_INVALID;
      type8 = (int32_t)actor->actor_type;
      {
        fifa96_err_t rc =
            kick_dir_vector(actor, slot, ctx, ctx->type_dir_x[type8],
                            ctx->type_dir_z[type8], &state->vector);
        if (rc != FIFA96_OK) return rc;
      }
      goto kick_band;
    }
  }
kick_band:
  /* 0x7BBE4..0x7BC15: the negative-mode range band. */
  if ((int8_t)state->flags < 0) {
    uint8_t band = 0;
    if (fifa96_action_kick_range_band((int8_t)state->flags, state->vector.x,
                                      &band) != FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    state->flags = band;
  }
kick_modes:
  flags_now = state->flags;
  /* 0x7BC1A..0x7BC80: the mode arms. */
  if (actor->active == 0) {
    if ((flags_now & 0x20) != 0) {
      if (kick_arm_20(actor, slot, ctx, rng, state, 0) != FIFA96_OK)
        return -FIFA96_ERR_INVALID;
    }
  } else if (flags_now == 0x40) {
    if (kick_arm_40(actor, slot, ctx, rng, state) != FIFA96_OK)
      return -FIFA96_ERR_INVALID;
  } else if ((flags_now & 0x20) != 0 || l1 != 0) {
    if (kick_arm_20(actor, slot, ctx, rng, state, (int16_t)l1) != FIFA96_OK)
      return -FIFA96_ERR_INVALID;
  }
  /* 0x7BC80..0x7BCA2: the event-row resolver. */
  if (ctx->sector_table == NULL) return -FIFA96_ERR_INVALID;
  kick_event(actor, slot, ctx, state->flags, state->vector.height,
             state->vector.z, &event);
  if (fifa96_action_kick_event_row(&event, ctx->sector_table, &pick) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  if (pick.found == 0 || ctx->event_rows[pick.table] == NULL) {
    out->staged = 0;
    return FIFA96_OK;
  }
  row = ctx->event_rows[pick.table] + 10u * pick.index;
  if (row[0] == 0) {
    out->staged = 0;
    return FIFA96_OK;
  }
  /* 0x7BCB6..0x7BD55: the row staging and clamp. */
  code = row[0];
  state->code = code;
  state->sub_code = row[9];
  lo = (int16_t)(uint16_t)(row[2] | ((uint16_t)row[3] << 8));
  hi = (int16_t)(uint16_t)(row[4] | ((uint16_t)row[5] << 8));
  add = (int16_t)(uint16_t)(row[6] | ((uint16_t)row[7] << 8));
  divisor = (int32_t)row[8];
  if (state->flags == 0x30) {
    lo = 0x1C8;
    hi = 0x780;
    add = 0x30;
    divisor = 0x18;
    user_extend = 1;
  }
  if ((ctx->side_range & 0x10u) != 0 &&
      (user_extend != 0 || code == 1 || code == 3)) {
    lo = (int16_t)(lo + (lo >> 1));
    hi = (int16_t)(hi + (hi >> 1));
  }
  if (lo > state->vector.x) {
    state->vector.x = lo;
    {
      int32_t angle = 0;
      if (fifa96_action_kick_angle(state->vector.height, state->vector.z, &angle) !=
          FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      state->vector.height = fifa96_ball_fold(state->vector.x, angle);
      state->vector.z = fifa96_ball_fold(state->vector.x, angle + 0x100);
    }
  } else if (hi < state->vector.x) {
    state->vector.x = hi;
    {
      int32_t angle = 0;
      if (fifa96_action_kick_angle(state->vector.height, state->vector.z, &angle) !=
          FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      state->vector.height = fifa96_ball_fold(state->vector.x, angle);
      state->vector.z = fifa96_ball_fold(state->vector.x, angle + 0x100);
    }
  }
  /* 0x7BE0B..0x7BEC0: the trajectory add, code-4 RNG and divisor. */
  state->traj = (int16_t)(state->traj + add);
  if (state->vector.x != 0 && code != 3) {
    if (code == 4) {
      uint16_t r;
      if (fifa96_rng_step(rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      if (state->flags == 0x40) {
        state->traj = (int16_t)(0x90 + (int32_t)(r & 7u) * (0x10 - (int32_t)actor->desc_15));
        divisor = 0;
      } else {
        divisor = (int32_t)(r & 0x7Fu) + 3;
      }
    }
    if (divisor != 0) {
      state->traj =
          (int16_t)(state->traj + (int16_t)state->vector.x / (int16_t)divisor);
    }
  }
  if ((int16_t)state->traj > 0x460) state->traj = 0x460;
  /* 0x7BEDE..0x7BF0A: the FUN_0007A490 stage + code-keyed tail. */
  {
    fifa96_ball_pair_vector staged = state->vector;
    fifa96_ball_stage_tail_actor tail_actor;
    fifa96_ball_stage_tail_out tail_out;
    memset(&tail_out, 0, sizeof tail_out);
    if (fifa96_ball_pair_stage(state, state->actor, &staged, state->traj, code) !=
        FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    tail_actor.pos_x = actor->pos_x;
    tail_actor.pos_z = actor->pos_z;
    tail_actor.nudge_x = actor->nudge_x;
    tail_actor.nudge_z = actor->nudge_z;
    tail_actor.lane_gate = (int16_t)actor->nudge_x;
    tail_actor.active = actor->active;
    tail_actor.has_slot = slot->present;
    tail_actor.type8 = actor->actor_type;
    tail_actor.facing = actor->facing;
    if (fifa96_ball_pair_stage_tail(state, &tail_actor, ctx->recompute_table,
                                    &tail_out) != FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    actor->pos_x = tail_actor.pos_x;
    actor->pos_z = tail_actor.pos_z;
    actor->facing = tail_actor.facing;
    out->receive = tail_out.receive;
    out->cleared = tail_out.cleared;
    out->slot_cb = tail_out.slot_cb;
    out->nudge = tail_out.nudge;
    out->anim = tail_out.anim;
  }
  out->band = state->flags;
  out->staged = 1;
  return FIFA96_OK;
}
