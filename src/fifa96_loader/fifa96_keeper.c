#include "fifa96_loader/fifa96_keeper.h"

int fifa96_dispatch_begin(fifa96_dispatch_iter *iter, const fifa96_dispatch_team *teams,
                          uint32_t team_count) {
  uint32_t i;
  if (!iter || !teams || team_count == 0) return -FIFA96_ERR_INVALID;
  for (i = 0; i < team_count; i++) {
    if (teams[i].count != 0 && !teams[i].records) return -FIFA96_ERR_INVALID;
  }
  iter->teams = teams;
  iter->team_count = team_count;
  iter->team_index = 0;
  iter->record_index = 0;
  return FIFA96_OK;
}

int fifa96_dispatch_next(fifa96_dispatch_iter *iter, uint32_t *team_index,
                         uint32_t *record_index, int *is_keeper) {
  if (!iter || !iter->teams || !team_index || !record_index) return -FIFA96_ERR_INVALID;
  while (iter->team_index < iter->team_count) {
    const fifa96_dispatch_team *team = &iter->teams[iter->team_index];
    uint32_t index;
    if (iter->record_index >= team->count) {
      iter->team_index++;
      iter->record_index = 0;
      continue;
    }
    index = iter->record_index;
    iter->record_index++;
    if (index == 0) {
      *team_index = iter->team_index;
      *record_index = 0;
      if (is_keeper) *is_keeper = 1;
      return 1;
    }
    if (team->records[index].skip_9a == 0) {
      *team_index = iter->team_index;
      *record_index = index;
      if (is_keeper) *is_keeper = 0;
      return 1;
    }
  }
  return 0;
}

int fifa96_keeper_select_action(const fifa96_keeper_state *state, uint8_t type_gate,
                                uint8_t current_action, uint8_t *next_action) {
  uint8_t code;
  if (!state || !next_action) return -FIFA96_ERR_INVALID;
  if (type_gate == 0) return 0;
  if (state->timer != 0) {
    code = 0;
  } else if (state->phase != 2) {
    code = 0x19;
  } else if (state->controlled == 0) {
    code = 0x19;
  } else if (state->opponent_controlled != 0 && state->opponent_type_5 != 0) {
    code = 0x19;
  } else if (state->own_type_5 != 0) {
    code = current_action;
  } else {
    code = 4;
  }
  if (code == current_action) return 0;
  *next_action = code;
  return 1;
}

fifa96_err_t fifa96_keeper_distance(int16_t x, int16_t z, int16_t *distance) {
  int32_t ax;
  int32_t az;
  int32_t mx;
  int32_t mn;
  if (!distance) return -FIFA96_ERR_INVALID;
  ax = x < 0 ? -(int32_t)x : (int32_t)x;
  az = z < 0 ? -(int32_t)z : (int32_t)z;
  mx = ax > az ? ax : az;
  mn = ax > az ? az : ax;
  *distance = (int16_t)(mx + (((mn >> 2) + (mn >> 1)) >> 1));
  return FIFA96_OK;
}

fifa96_err_t fifa96_keeper_vec_from_delta(const fifa96_keeper_point *from,
                                          const fifa96_keeper_point *to,
                                          fifa96_keeper_vec *out) {
  int16_t dx;
  int16_t dz;
  if (!from || !to || !out) return -FIFA96_ERR_INVALID;
  dx = (int16_t)(to->x - from->x);
  dz = (int16_t)(to->z - from->z);
  out->dx = dx;
  out->dz = dz;
  return fifa96_keeper_distance(dx, dz, &out->distance);
}

fifa96_err_t fifa96_keeper_clear_vector(uint32_t rng_x, uint32_t rng_z, int8_t range_attr,
                                        uint8_t side, fifa96_keeper_vec *out) {
  int32_t range;
  int32_t z;
  if (!out) return -FIFA96_ERR_INVALID;
  out->dx = (int16_t)((int32_t)(rng_x % 0x780u) - 0x3C0);
  range = 0x780 + 2 * (int32_t)range_attr;
  z = (int32_t)(rng_z % (uint32_t)range) + 0x3C0;
  if (side == 1) z = -z;
  out->dz = (int16_t)z;
  return fifa96_keeper_distance(out->dx, out->dz, &out->distance);
}

fifa96_err_t fifa96_keeper_claim_place(const fifa96_keeper_point *pos, int8_t offset_x,
                                       int8_t offset_z, uint8_t stage, uint8_t has_ball,
                                       uint8_t has_slot, fifa96_keeper_point *place,
                                       uint8_t *helper_request, uint8_t *claimed) {
  if (!pos || !place || !helper_request || !claimed) return -FIFA96_ERR_INVALID;
  *helper_request = (stage < 6 && has_slot == 0) ? 1 : 0;
  *claimed = 0;
  if (stage < 3 && has_ball == 0) {
    place->x = pos->x + (int32_t)offset_x * 16;
    place->y = pos->y + 0x38;
    place->z = pos->z + (int32_t)offset_z * 16;
    *claimed = 1;
  }
  return FIFA96_OK;
}

int fifa96_keeper_hold_track(int16_t deepest_x, int16_t candidate_x) {
  return candidate_x < deepest_x ? 1 : 0;
}

fifa96_err_t fifa96_keeper_hold_guard(const fifa96_keeper_point *cam, uint8_t side,
                                      fifa96_keeper_point *out) {
  if (!cam || !out) return -FIFA96_ERR_INVALID;
  *out = *cam;
  if (out->z > 0xAE0 || out->z < -0xAE0) out->z = side == 0 ? -0xAE0 : 0xAE0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_keeper_hold_intercept(const fifa96_keeper_point *cam, int32_t distance,
                                          int16_t carrier_vx, int16_t carrier_vz,
                                          fifa96_keeper_point *out) {
  int32_t scale;
  if (!cam || !out) return -FIFA96_ERR_INVALID;
  scale = distance >> 20;
  out->x = cam->x + scale * carrier_vx;
  out->y = 0;
  out->z = cam->z + scale * carrier_vz;
  return FIFA96_OK;
}

fifa96_err_t fifa96_keeper_guard_clamp(fifa96_keeper_point *p, uint8_t side) {
  if (!p) return -FIFA96_ERR_INVALID;
  if (p->x < -0x390) p->x = -0x390;
  else if (p->x > 0x390) p->x = 0x390;
  if (side == 0) {
    if (p->z < -0xAE0) p->z = -0xAE0;
    else if (p->z > -0x7E0) p->z = -0x7E0;
  } else {
    if (p->z > 0xAE0) p->z = 0xAE0;
    else if (p->z < 0x7E0) p->z = 0x7E0;
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_keeper_dive_target(const fifa96_keeper_point *pos, uint32_t rng_a,
                                       uint32_t divisor, uint32_t rng_b, int16_t trajectory_z,
                                       uint32_t rng_c, uint32_t rng_d,
                                       fifa96_keeper_dive *out) {
  int32_t sign;
  if (!pos || !out || divisor == 0) return -FIFA96_ERR_INVALID;
  if (rng_a % divisor < 0x19) {
    sign = trajectory_z > 0 ? 1 : -1;
  } else {
    sign = (rng_b & 1u) ? 1 : -1;
  }
  out->target.x = (int16_t)(sign * (int32_t)((rng_c & 0x3Fu) + 0x30u));
  out->target.y = (int16_t)((rng_d % 0x30u) + 0x20u);
  out->target.z = pos->z + (pos->z > 0 ? -0x30 : 0x30);
  out->install_code = 0x1B;
  out->invoke = 1;
  return FIFA96_OK;
}

fifa96_err_t fifa96_keeper_arm_step(uint8_t stage, uint8_t event_flag, uint8_t ball_actor,
                                    uint8_t ball_flag, uint8_t has_slot, uint8_t slot_pressed,
                                    uint8_t type8, uint8_t phase_latch,
                                    fifa96_keeper_arm_out *out) {
  if (!out) return -FIFA96_ERR_INVALID;
  out->stage = stage;
  out->event_code = 0;
  out->install_code = 0;
  out->invoke = 0;
  out->copy_pos = 0;
  out->run_handler = 0;
  out->reset = 0;
  out->flag_9e = 0;
  if (stage > 3) return FIFA96_OK;
  if (stage == 0) {
    out->flag_9e = 1;
    if (event_flag == 0) return FIFA96_OK;
    out->event_code = 0x28;
    stage = 1;
  }
  if (stage == 1) {
    if (ball_actor == 0 || ball_flag == 0) {
      out->stage = 1;
      return FIFA96_OK;
    }
    stage = 2;
  }
  if (stage == 2) {
    if (has_slot == 0) {
      out->stage = 2;
      out->install_code = 0x1B;
      out->invoke = 1;
      return FIFA96_OK;
    }
    stage = 3;
  }
  out->stage = 3;
  out->copy_pos = 1;
  out->run_handler = (has_slot != 0 && slot_pressed != 0) ? 1 : 0;
  out->reset = (type8 == 0x1F && phase_latch == 0) ? 1 : 0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_keeper_reposition_a_gate(uint8_t has_tracked_teammate,
                                             fifa96_keeper_rep_a_out *out) {
  if (!out) return -FIFA96_ERR_INVALID;
  if (has_tracked_teammate != 0) {
    out->install_code = 4;
    out->reset = 1;
    out->stage_advance = 0;
    out->flag_9e = 0;
  } else {
    out->install_code = 0;
    out->reset = 0;
    out->stage_advance = 2;
    out->flag_9e = 1;
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_keeper_reposition_b_finish(uint8_t is_controlled, uint8_t human_side,
                                               uint8_t *install_code, uint8_t *notify_code) {
  if (!install_code || !notify_code) return -FIFA96_ERR_INVALID;
  if (is_controlled != 0) {
    *install_code = 0;
    *notify_code = human_side != 0 ? 0 : 5;
  } else {
    *install_code = 0x19;
    *notify_code = 0;
  }
  return FIFA96_OK;
}

static int32_t keeper_trunc_shift(int16_t value, unsigned shift) {
  int32_t v = value;
  if (v < 0) return -((-v) >> shift);
  return v >> shift;
}

fifa96_err_t fifa96_keeper_lunge_track(const fifa96_keeper_vec *delta, int16_t angle,
                                       fifa96_keeper_point *pos, uint8_t *on_target,
                                       int16_t *steer_x, int16_t *steer_z, uint8_t *event_code) {
  int32_t a;
  if (!delta || !pos || !on_target || !steer_x || !steer_z || !event_code)
    return -FIFA96_ERR_INVALID;
  *on_target = 0;
  *event_code = 0;
  *steer_x = 0;
  *steer_z = 0;
  a = angle < 0 ? -(int32_t)angle : angle;
  if (a >= 0x55) return FIFA96_OK;
  pos->x += keeper_trunc_shift(delta->dx, 1);
  pos->z += keeper_trunc_shift(delta->dz, 1);
  *on_target = 1;
  *event_code = 0x3C;
  *steer_x = 0x80;
  *steer_z = 0x40;
  return FIFA96_OK;
}

fifa96_err_t fifa96_keeper_hold_fallback(const fifa96_keeper_point *cam, uint8_t side,
                                         int32_t cam_vel_z, int32_t lane, int32_t dir,
                                         uint16_t dir_word, int32_t vel_x, int32_t lead_x,
                                         fifa96_keeper_point *out) {
  int32_t base;
  int32_t x;
  if (!cam || !out) return -FIFA96_ERR_INVALID;
  base = side == 0 ? -0x9F0 : 0x9F0;
  base += keeper_trunc_shift((int16_t)(cam_vel_z >> 16), 4);
  if (base > 0xAE0) base = 0xAE0;
  else if (base < -0xAE0) base = -0xAE0;
  if (lane >= 0x780 || (dir < 0 ? (int32_t)(0u - (uint32_t)dir) : dir) <= 0x9F0 ||
      dir_word == 0) {
    int16_t w = (int16_t)cam->x;
    if (w >= 0) {
      x = ((int32_t)w >> 3) + ((int32_t)w >> 4) + ((int32_t)w >> 5);
    } else {
      int32_t nw = (int16_t)(-(int32_t)w);
      x = -((nw >> 3) + (nw >> 4) + (nw >> 5));
    }
  } else {
    uint32_t ubase = (uint32_t)base;
    uint32_t ucamz = (uint32_t)cam->z;
    int32_t absbase = base < 0 ? (int32_t)(0u - ubase) : (int32_t)ubase;
    int32_t abscamz = cam->z < 0 ? (int32_t)(0u - ucamz) : (int32_t)ucamz;
    int32_t num = absbase - abscamz;
    int32_t den = (int32_t)((uint32_t)(vel_x >> 16) * 16u);
    int32_t quot;
    int32_t lead;
    if (den == 0) return -FIFA96_ERR_INVALID;
    quot = num / (den < 0 ? -den : den);
    lead = (int32_t)((uint32_t)(lead_x >> 16) * 16u);
    x = (int32_t)((uint32_t)cam->x + (uint32_t)lead * (uint32_t)quot);
    if (x > 0xC0) x = 0xC0;
    else if (x < -0xC0) x = -0xC0;
  }
  out->x = x;
  out->y = 0;
  out->z = base;
  return FIFA96_OK;
}

fifa96_err_t fifa96_keeper_arm_camera(uint8_t stage, uint8_t team_side, uint8_t phase,
                                      fifa96_keeper_point *target, uint8_t *camera_hook) {
  if (!target || !camera_hook) return -FIFA96_ERR_INVALID;
  *camera_hook = 0;
  if (stage >= 2) return FIFA96_OK;
  target->x = 0;
  target->y = 0;
  target->z = (team_side != 0 || phase == 0x10) ? 0xB10 : -0xB10;
  if (stage < 1) *camera_hook = 1;
  return FIFA96_OK;
}

fifa96_err_t fifa96_keeper_input_decide(const fifa96_keeper_input *in,
                                        fifa96_keeper_decision *out) {
  uint8_t inside;
  if (!in || !out) return -FIFA96_ERR_INVALID;
  out->install = 0;
  out->invoke = 0;
  out->copy_cam = 0;
  out->copy_pos = 0;
  out->clear_c5c = 0;
  out->target_x = 0;
  out->target_y = 0;
  out->target_z = 0;
  if (in->has_slot == 0) return FIFA96_OK;
  if (in->phase == 2) {
    uint32_t ux = (uint32_t)in->cam_x;
    int32_t ax = in->cam_x < 0 ? (int32_t)(0u - ux) : (int32_t)ux;
    if (ax > 0x420) inside = 0;
    else if (in->team_side != 0 ? in->cam_z >= 0x7B0 : in->cam_z <= -0x7B0) inside = 1;
    else inside = 0;
  } else {
    if (in->human_phase != 1 || in->event_flag == 0) return FIFA96_OK;
    inside = 1;
  }
  if (in->type8 != 0x1F && (in->type_gate & 1u) == 0) return FIFA96_OK;
  if (in->is_actor != 0 || in->type8 == 5) return FIFA96_OK;
  if (inside == 0) {
    if (in->lane > 0x60) return FIFA96_OK;
    out->install = 4;
    out->invoke = 1;
    return FIFA96_OK;
  }
  if (in->lane <= 0x40) {
    out->copy_cam = 1;
    out->install = 0x19;
    out->invoke = 1;
    out->target_x = in->cam_x;
    out->target_y = in->cam_y;
    out->target_z = in->cam_z;
    return FIFA96_OK;
  }
  if (in->slot_dir_x == 0 && in->slot_dir_z == 0) {
    out->copy_pos = 1;
    out->install = 0x1C;
    out->invoke = 1;
    out->target_x = in->pos_x;
    out->target_y = 0xA0;
    out->target_z = in->pos_z;
    return FIFA96_OK;
  }
  out->target_x = in->pos_x + (int32_t)in->slot_dir_x * 0x70;
  out->target_z = in->pos_z + (int32_t)in->slot_dir_z * 0x70;
  if (out->target_z > 0xAF0) out->target_z = 0xAF0;
  else if (out->target_z < -0xAF0) out->target_z = -0xAF0;
  out->target_y = 0x40;
  out->clear_c5c = 1;
  {
    int32_t dx = out->target_x - in->pos_x;
    int32_t dz = out->target_z - in->pos_z;
    int16_t dist = 0;
    fifa96_err_t rc = fifa96_keeper_distance((int16_t)dx, (int16_t)dz, &dist);
    if (rc != FIFA96_OK) return rc;
    out->install = (uint8_t)(0x1B + (dist >= 0x70 ? 1 : 0));
  }
  out->invoke = 1;
  return FIFA96_OK;
}
