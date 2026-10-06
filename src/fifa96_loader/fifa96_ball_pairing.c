#include "fifa96_loader/fifa96_ball_pairing.h"

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
