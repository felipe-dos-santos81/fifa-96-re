#include "fifa96_loader/fifa96_action_handlers.h"
#include "fifa96_loader/fifa96_entity_update.h"

static const uint8_t kick_atan[257] = {
    0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0A,
    0x0A, 0x0B, 0x0B, 0x0C, 0x0D, 0x0D, 0x0E, 0x0F, 0x0F, 0x10, 0x10, 0x11, 0x12, 0x12, 0x13, 0x14,
    0x14, 0x15, 0x16, 0x16, 0x17, 0x17, 0x18, 0x19, 0x19, 0x1A, 0x1B, 0x1B, 0x1C, 0x1C, 0x1D, 0x1E,
    0x1E, 0x1F, 0x1F, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x24, 0x24, 0x25, 0x26, 0x26, 0x27, 0x27,
    0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x30, 0x30, 0x31,
    0x31, 0x32, 0x33, 0x33, 0x34, 0x34, 0x35, 0x35, 0x36, 0x37, 0x37, 0x38, 0x38, 0x39, 0x39, 0x3A,
    0x3A, 0x3B, 0x3C, 0x3C, 0x3D, 0x3D, 0x3E, 0x3E, 0x3F, 0x3F, 0x40, 0x41, 0x41, 0x42, 0x42, 0x43,
    0x43, 0x44, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x47, 0x48, 0x48, 0x49, 0x4A, 0x4A, 0x4B, 0x4B,
    0x4C, 0x4C, 0x4D, 0x4D, 0x4E, 0x4E, 0x4F, 0x4F, 0x50, 0x50, 0x51, 0x51, 0x52, 0x52, 0x53, 0x53,
    0x54, 0x54, 0x54, 0x55, 0x55, 0x56, 0x56, 0x57, 0x57, 0x58, 0x58, 0x59, 0x59, 0x5A, 0x5A, 0x5B,
    0x5B, 0x5B, 0x5C, 0x5C, 0x5D, 0x5D, 0x5E, 0x5E, 0x5F, 0x5F, 0x60, 0x60, 0x60, 0x61, 0x61, 0x62,
    0x62, 0x63, 0x63, 0x63, 0x64, 0x64, 0x65, 0x65, 0x66, 0x66, 0x66, 0x67, 0x67, 0x68, 0x68, 0x68,
    0x69, 0x69, 0x6A, 0x6A, 0x6A, 0x6B, 0x6B, 0x6C, 0x6C, 0x6C, 0x6D, 0x6D, 0x6E, 0x6E, 0x6E, 0x6F,
    0x6F, 0x70, 0x70, 0x70, 0x71, 0x71, 0x71, 0x72, 0x72, 0x73, 0x73, 0x73, 0x74, 0x74, 0x74, 0x75,
    0x75, 0x76, 0x76, 0x76, 0x77, 0x77, 0x77, 0x78, 0x78, 0x78, 0x79, 0x79, 0x79, 0x7A, 0x7A, 0x7A,
    0x7B, 0x7B, 0x7B, 0x7C, 0x7C, 0x7C, 0x7D, 0x7D, 0x7D, 0x7E, 0x7E, 0x7E, 0x7F, 0x7F, 0x7F, 0x80,
    0x80,
};

static int16_t low16(int32_t value) {
  return (int16_t)(uint16_t)value;
}

fifa96_err_t fifa96_action_move_target(int32_t pos_x, int32_t pos_z, int8_t dir_x, int8_t dir_z,
                                       fifa96_action_vec3 *out) {
  if (!out) return -FIFA96_ERR_INVALID;
  out->x = pos_x + (int32_t)dir_x * 128;
  out->y = 0;
  out->z = pos_z + (int32_t)dir_z * 128;
  if (out->x > 0x720) out->x = 0x720;
  else if (out->x < -0x720) out->x = -0x720;
  if (out->z > 0xB10) out->z = 0xB10;
  else if (out->z < -0xB10) out->z = -0xB10;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_move_step(fifa96_action_move_state *state, fifa96_action_move_out *out) {
  if (!state || !out) return -FIFA96_ERR_INVALID;
  out->move = 0;
  out->install = 0;
  out->code = 0;
  if (state->timer89 > 0) state->timer89 -= state->delta;
  if (state->has_slot != 0 && state->phase != 6) out->move = 1;
  if (state->phase == 2 && state->timer81 == 0 && state->timer89 <= 0) {
    out->install = 1;
    out->code = state->active != 0 ? 3 : 0x19;
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_kick_angle(int32_t x, int32_t z, int32_t *angle) {
  uint32_t flags = 0;
  uint32_t ux, uz, mn = 0, mx = 0;
  int32_t value = -1;
  if (!angle) return -FIFA96_ERR_INVALID;
  ux = (uint32_t)x;
  uz = (uint32_t)z;
  if (x < 0) {
    flags |= 16;
    ux = 0u - ux;
  }
  if (z < 0) {
    flags |= 8;
    uz = 0u - uz;
  }
  if (ux >= uz) {
    if (ux == uz) {
      value = 0x80;
    } else {
      flags |= 4;
      mx = ux;
      mn = uz;
    }
  } else {
    mx = uz;
    mn = ux;
  }
  if (value < 0) {
    uint64_t quotient = ((uint64_t)mn << 32) / mx;
    uint32_t index = (uint32_t)(quotient >> 24) + (uint32_t)((quotient >> 23) & 1u);
    value = kick_atan[index];
  }
  switch (flags) {
    case 0: break;
    case 4: value = 0x100 - value; break;
    case 8: value = 0x200 - value; break;
    case 12: value = value + 0x100; break;
    case 16: value = -value; break;
    case 20: value = value - 0x100; break;
    case 24: value = value - 0x200; break;
    default: value = -value - 0x100; break;
  }
  *angle = value;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_kick_apply(fifa96_action_kick_ball *ball, const fifa96_action_kick_row *row,
                                      uint8_t mode, uint8_t event_code, uint8_t user_extend) {
  int32_t lo, hi, add;
  int16_t lo_word, hi_word;
  if (!ball || !row) return -FIFA96_ERR_INVALID;
  lo = (int16_t)row->lo;
  hi = (int16_t)row->hi;
  add = (int16_t)row->traj_add;
  if (mode == 0x30) {
    lo = 0x1C8;
    hi = 0x780;
    add = 0x30;
  }
  if (user_extend != 0 && (mode == 0x30 || event_code == 1 || event_code == 3)) {
    lo = lo + (lo >> 1);
    hi = hi + (hi >> 1);
  }
  lo_word = low16(lo);
  hi_word = low16(hi);
  if (lo_word > ball->x) ball->x = lo_word;
  else if (hi_word < ball->x) ball->x = hi_word;
  ball->traj = low16((int32_t)ball->traj + add);
  if (ball->comp_z > 0x460) ball->traj = 0x460;
  return FIFA96_OK;
}

static int16_t loco_facing_error(int16_t target, int16_t current) {
  int16_t d = (int16_t)((uint16_t)((uint16_t)target - (uint16_t)current) & 0x3FFu);
  if (d > 0x200) d = (int16_t)(d - 0x400);
  return d;
}

static int32_t loco_stride_error(int16_t target, int16_t current) {
  int32_t d = (int32_t)((uint16_t)((uint16_t)target - (uint16_t)current) & 0x3FFu);
  if (d > 0x200) d = 0x400 - d;
  return d;
}

fifa96_err_t fifa96_action_locomotion_step(fifa96_action_locomotion *state,
                                           const uint8_t *heading_table,
                                           const int16_t *stride_table) {
  int32_t dx, dz;
  if (!state || !heading_table || !stride_table) return -FIFA96_ERR_INVALID;
  dx = low16(state->target_x - state->pos_x);
  dz = low16(state->target_z - state->pos_z);
  state->delta_x = (int16_t)dx;
  state->delta_z = (int16_t)dz;
  state->distance = (int16_t)fifa96_entity_distance(dx, dz);
  if (state->distance != 0) {
    int32_t angle = 0;
    fifa96_action_kick_angle(dx, dz, &angle);
    state->desired_facing = (int16_t)angle;
  } else {
    state->desired_facing = state->facing;
  }
  if (state->direct_face != 0) {
    int16_t face_target;
    if ((state->move_attr >> 16) > 0x60) {
      face_target = state->desired_facing;
    } else if (state->face_x != 0 || state->face_z != 0) {
      int32_t fx = low16((int32_t)state->face_x - state->pos_x);
      int32_t fz = low16((int32_t)state->face_z - state->pos_z);
      int32_t angle = 0;
      fifa96_action_kick_angle(fx, fz, &angle);
      face_target = (int16_t)angle;
    } else {
      face_target = state->facing;
    }
    {
      int16_t err = loco_facing_error(face_target, state->facing);
      if (err != 0) {
        uint16_t limit = (uint16_t)((uint16_t)(0x10 - (uint16_t)state->speed)
                                    << (state->has_slot != 0 ? 4 : 2));
        int16_t turn;
        if (err > (int16_t)limit) turn = (int16_t)limit;
        else if ((int32_t)err < -(int32_t)(int16_t)limit) turn = (int16_t)(0u - limit);
        else turn = err;
        state->facing = (int16_t)((uint16_t)(state->facing + turn) & 0x3FFu);
        if (state->facing > 0x200) state->facing = (int16_t)(state->facing - 0x400);
        state->heading = heading_table[((uint16_t)state->facing & 0x3FFu) >> 5];
      }
    }
  }
  if (state->distance != 0 || state->speed != 0) {
    int32_t threshold = state->has_slot != 0 ? 2 : ((state->stride_rate >> 17) + 2);
    state->body_timer = (uint8_t)(state->body_timer + (uint8_t)state->delta);
    if ((int16_t)(int32_t)state->body_timer > (int16_t)threshold) {
      int32_t stride_len;
      int32_t attr;
      int32_t index;
      int32_t tvx, tvz;
      int16_t old_vx, old_vz;
      state->body_timer = 0;
      stride_len = (int32_t)(state->stride & 0x0F) -
                   (loco_stride_error(state->desired_facing, state->facing) >> 7);
      if (stride_len < 1) stride_len = 1;
      attr = state->move_attr >> 19;
      if ((int16_t)attr > (int16_t)stride_len) attr = stride_len;
      index = ((int32_t)state->desired_facing & 0x3F0) | attr;
      tvx = stride_table[index];
      tvz = stride_table[index + 0x100];
      old_vx = state->vel_x;
      old_vz = state->vel_z;
      if ((int16_t)tvx != old_vx) {
        int16_t dv = (int16_t)((int16_t)tvx - old_vx);
        state->vel_x = (dv >= -1 && dv <= 1) ? (int16_t)tvx
                                             : (int16_t)(old_vx + dv / 2);
      }
      if ((int16_t)tvz != old_vz) {
        int16_t dv = (int16_t)((int16_t)tvz - old_vz);
        state->vel_z = (dv >= -1 && dv <= 1) ? (int16_t)tvz
                                             : (int16_t)(old_vz + dv / 2);
      }
      if ((int16_t)tvx != old_vx || (int16_t)tvz != old_vz)
        state->speed = (int16_t)fifa96_entity_distance(state->vel_x, state->vel_z);
    }
    if (state->vel_x != 0) state->pos_x += (int32_t)state->delta * state->vel_x;
    if (state->vel_z != 0) state->pos_z += (int32_t)state->delta * state->vel_z;
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_locomotion_restart_target(int32_t controlled_x,
                                                     int32_t *target_x, int32_t *target_z) {
  if (!target_x || !target_z) return -FIFA96_ERR_INVALID;
  *target_x = -controlled_x;
  *target_z = 0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_locomotion_hold(uint8_t action_code,
                                           const fifa96_action_vec3 *pos,
                                           fifa96_action_vec3 *out, uint8_t *held) {
  if (!pos || !out || !held) return -FIFA96_ERR_INVALID;
  *held = 0;
  if (action_code == 0x10 || action_code == 0x11 || action_code == 0x12) {
    *out = *pos;
    *held = 1;
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_locomotion_clamp_placement(int32_t *target_z, int32_t pos_z,
                                                      int16_t bound_lo, int16_t bound_hi,
                                                      int32_t opponent_z, uint8_t side,
                                                      uint8_t settings_latch) {
  if (!target_z) return -FIFA96_ERR_INVALID;
  if (bound_lo > *target_z) {
    if (bound_lo < pos_z) *target_z = bound_lo;
  } else if (bound_hi < *target_z && bound_hi > pos_z) {
    *target_z = bound_hi;
  }
  if (settings_latch != 0) return FIFA96_OK;
  if (side == 0) {
    if (*target_z > opponent_z) *target_z = opponent_z - 0x60;
  } else if (*target_z < opponent_z) {
    *target_z = opponent_z + 0x60;
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_locomotion_camera_lead(int32_t cam_x, int32_t cam_y, int32_t cam_z,
                                                  int16_t cam_vel_x, int16_t cam_vel_z,
                                                  fifa96_action_vec3 *out) {
  if (!out) return -FIFA96_ERR_INVALID;
  out->x = cam_x + (int32_t)cam_vel_x * 4;
  out->y = cam_y;
  out->z = cam_z + (int32_t)cam_vel_z * 4;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_possession_reset(fifa96_action_possession *state) {
  if (!state) return -FIFA96_ERR_INVALID;
  state->carrier = 0;
  state->index = 0;
  state->rotation = 0;
  state->dir_x = 0;
  state->dir_z = 0;
  state->counter_c = 0;
  state->release_timer = 0;
  state->counter_e = 0;
  state->counter_f = 0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_possession_claim(fifa96_action_possession *state, int32_t actor,
                                            int *claimed) {
  if (!state || !claimed) return -FIFA96_ERR_INVALID;
  if (state->carrier == actor) {
    *claimed = 0;
    return FIFA96_OK;
  }
  fifa96_action_possession_reset(state);
  state->carrier = actor;
  *claimed = 1;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_possession_timer(int32_t *timer, uint16_t delta) {
  if (!timer) return -FIFA96_ERR_INVALID;
  if (*timer < 0x4B0) *timer = *timer + (int32_t)delta;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_possession_dribble_dir(uint8_t type8, int32_t distance, uint8_t has_slot,
                                                  int8_t slot_x, int8_t slot_z,
                                                  const int8_t *type_x, const int8_t *type_z,
                                                  fifa96_action_dribble_dir *out) {
  if (!out) return -FIFA96_ERR_INVALID;
  out->dir_x = 0;
  out->dir_z = 0;
  out->speed = 0;
  out->resolved = 0;
  if (distance > 0x38) {
    if (!type_x || !type_z) return -FIFA96_ERR_INVALID;
    out->dir_x = type_x[type8];
    out->dir_z = type_z[type8];
    out->speed = 0x60;
    out->resolved = 1;
  } else if (has_slot != 0) {
    out->dir_x = slot_x;
    out->dir_z = slot_z;
    out->speed = 0x30;
    out->resolved = 1;
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_receive_step(fifa96_action_receive *state,
                                        fifa96_action_receive_out *out) {
  if (!state || !out) return -FIFA96_ERR_INVALID;
  out->reset = 0;
  out->advance = 0;
  out->handoff = 0;
  out->stage = state->stage;
  if (state->stage == 0) {
    if (state->active == 0) {
      out->reset = 1;
      return FIFA96_OK;
    }
    if (state->offset_word > 0x40) {
      if (state->timer89 > 0x3C) {
        out->reset = 1;
        out->handoff = state->is_team_target;
      }
      return FIFA96_OK;
    }
    out->advance = 1;
    state->timer89 = 0;
    state->stage = 1;
    out->stage = 1;
  }
  if (state->stage == 1) {
    if (state->event_flag != 0 || state->offset_word > 0x40) {
      out->reset = 1;
      out->handoff = state->is_team_target;
    }
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_tackle_attempt(const fifa96_action_tackle *state, int *install_0e) {
  int32_t dx, dz, distance, angle = 0, diff;
  if (!state || !install_0e) return -FIFA96_ERR_INVALID;
  *install_0e = 0;
  if (state->phase != 2 || state->is_tracked == 0) return FIFA96_OK;
  if (state->cam_f8 < 0x14) return FIFA96_OK;
  if (state->close_word > 0x180) return FIFA96_OK;
  if (state->target_height < 0x20 || state->target_height > 0x60) return FIFA96_OK;
  dx = low16(state->target_x - state->pos_x);
  dz = low16(state->target_z - state->pos_z);
  distance = fifa96_entity_distance(dx, dz);
  if (distance > 0xF0) return FIFA96_OK;
  if (state->pos_x < -0x210 && state->pos_x > state->camera_x) return FIFA96_OK;
  if (state->pos_x > 0x210 && state->camera_x < state->pos_x) return FIFA96_OK;
  if (state->side == 0) {
    if (state->pos_z < 0x690) return FIFA96_OK;
  } else if (state->side == 1 && state->pos_z > -0x690) {
    return FIFA96_OK;
  }
  fifa96_action_kick_angle(dx, dz, &angle);
  if (state->side == 0) {
    if (angle < -0x100 || angle > 0x100) return FIFA96_OK;
  } else if (angle > -0x100 && angle < 0x100) {
    return FIFA96_OK;
  }
  diff = (int32_t)((uint16_t)((uint16_t)angle - state->facing) & 0x3FFu);
  if (diff > 0x200) diff = 0x400 - diff;
  if (diff > 0x100) return FIFA96_OK;
  *install_0e = 1;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_tackle_step(fifa96_action_tackle *state, fifa96_action_tackle_out *out) {
  int32_t dx, dz;
  if (!state || !out) return -FIFA96_ERR_INVALID;
  out->reset = 0;
  out->install_0e = 0;
  out->install_0f = 0;
  out->stage = state->stage;
  if (state->phase != 2) {
    out->reset = 1;
    return FIFA96_OK;
  }
  state->timer89 += state->delta;
  if (state->stage == 0) {
    if (state->active == 0) {
      out->reset = 1;
      return FIFA96_OK;
    }
    state->timer89 = 0;
    state->stage = 1;
    out->stage = 1;
  }
  if (state->stage == 1) {
    int install = 0;
    if (state->lob != 0 || state->timer89 > 0x78) {
      out->reset = 1;
      goto tackle_tail;
    }
    fifa96_action_tackle_attempt(state, &install);
    if (install != 0) {
      out->install_0e = 1;
      goto tackle_tail;
    }
    if (state->slot_button_40 != 0) goto tackle_tail;
    if (state->flag99 != 0) goto tackle_tail;
    if (state->field5d != 0) goto tackle_tail;
    if (state->cam_fa <= state->cam_f2) goto tackle_tail;
    if (state->is_own == 0) goto tackle_tail;
    if (state->opp_close >= 0x120) goto tackle_tail;
    if (state->opp_close > state->opp_bound) goto tackle_tail;
    if ((int32_t)state->cam_f8 + state->cam_100 < state->cam_fe) goto tackle_tail;
    dx = low16(state->vector_x - state->pos_x);
    dz = low16(state->vector_z - state->pos_z);
    if (fifa96_entity_distance(dx, dz) >= 0x60) goto tackle_tail;
    out->install_0f = 1;
    return FIFA96_OK;
  }
tackle_tail:
  if (state->close_word > state->own_bound) out->reset = 1;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_duel_step(fifa96_action_duel *state, uint16_t delta, uint8_t input_byte,
                                     fifa96_action_duel_out *out) {
  if (!state || !out) return -FIFA96_ERR_INVALID;
  out->wait = 0;
  out->handoff = 0;
  out->reset = 0;
  out->stage = state->stage;
  state->stride = 2;
  state->timer89 += (int32_t)delta;
  if (state->stage == 0) {
    if (state->animation == 0x55 || state->animation == 0x6A) return FIFA96_OK;
    state->timer89 = 0;
    state->stage = 1;
  }
  if (state->stage == 1) {
    int32_t dx = low16(0x900 - state->pos_x);
    int32_t dz = low16(0 - state->pos_z);
    state->distance = (int16_t)fifa96_entity_distance(dx, dz);
    state->delta_x = (int16_t)dx;
    state->delta_z = (int16_t)dz;
    state->timer89 = 0;
    state->stage = 2;
  }
  if (state->stage == 2) {
    if (state->timer89 < 0x78) {
      out->wait = 1;
      out->stage = state->stage;
      return FIFA96_OK;
    }
    if (state->timer89 <= 0x12C && (input_byte & 0xF0) == 0 && state->distance >= 0x20) {
      out->wait = 1;
      out->stage = state->stage;
      return FIFA96_OK;
    }
    out->handoff = state->has_slot;
    out->reset = 1;
  }
  out->stage = state->stage;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_duel_split(int16_t own_metric, int16_t opp_metric,
                                      uint8_t opp_is_duel_type, uint8_t *own_code,
                                      uint8_t *opp_code) {
  if (!own_code || !opp_code) return -FIFA96_ERR_INVALID;
  *own_code = 5;
  *opp_code = 0;
  if (opp_is_duel_type != 0 && opp_metric >= own_metric) *opp_code = 6;
  return FIFA96_OK;
}
