#include "fifa96_loader/fifa96_ball_pairing.h"

#include <string.h>

#include "fifa96_loader/fifa96_arm_helpers.h"

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
