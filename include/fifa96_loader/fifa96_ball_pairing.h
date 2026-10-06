#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_entity_update.h"

typedef struct fifa96_ball_pair_vector {
  int16_t x;
  int16_t height;
  int16_t z;
} fifa96_ball_pair_vector;

typedef struct fifa96_ball_pair_actor {
  fifa96_ball_pair_vector position;
  int16_t velocity_x;
  int16_t velocity_z;
  uint8_t has_ball;
  uint8_t kind;
  uint8_t action;
  uint8_t flag;
} fifa96_ball_pair_actor;

typedef struct fifa96_ball_pair_delta {
  int16_t distance;
  int16_t dx;
  int16_t dz;
} fifa96_ball_pair_delta;

typedef struct fifa96_ball_pair_targets {
  int32_t team0_target;
  int32_t team0_second;
  int32_t team1_target;
  int32_t team1_second;
} fifa96_ball_pair_targets;

int fifa96_ball_pair_offset(const fifa96_ball_pair_vector *from,
                            const fifa96_ball_pair_vector *to,
                            fifa96_ball_pair_delta *out);
int fifa96_ball_pair_decide(const fifa96_ball_pair_actor *interceptor,
                            const fifa96_ball_pair_actor *opponent,
                            int16_t delta,
                            fifa96_ball_pair_vector *out_position);
int fifa96_ball_pair_receive(const fifa96_ball_pair_actor *actor,
                             const fifa96_entity_candidate *candidates,
                             uint32_t count, int16_t target_x, int16_t target_y,
                             int32_t *receiver_index);
int fifa96_ball_pair_assign(fifa96_ball_pair_targets *targets, int team,
                            int32_t receiver_index);
int fifa96_ball_pair_possess(fifa96_ball_pair_actor *actor);
int fifa96_ball_pair_release(fifa96_ball_pair_actor *actor);
