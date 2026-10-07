#include "fifa96_loader/fifa96_action_handlers.h"

#include <string.h>

#include "fifa96_loader/fifa96_entity_update.h"
#include "fifa96_loader/fifa96_rng.h"

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

fifa96_err_t fifa96_action_kick_range_band(int8_t mode, int16_t ball_x,
                                           uint8_t *band) {
  if (!band) return -FIFA96_ERR_INVALID;
  if (mode >= 0) {
    *band = (uint8_t)mode;
    return FIFA96_OK;
  }
  if (ball_x < 0x5A0) *band = 0x20;
  else if (ball_x < 0x780) *band = 0x30;
  else *band = 0x10;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_kick_event_row(const fifa96_action_kick_event *event,
                                          const uint8_t *sector_table,
                                          fifa96_action_kick_event_out *out) {
  int32_t class_id;
  int16_t sector;
  uint32_t idx;
  int16_t d;
  if (!event || !sector_table || !out) return -FIFA96_ERR_INVALID;
  out->found = 0;
  out->table = 0;
  out->index = 0;
  class_id = event->code == 0x40 ? 2 : ((event->code & 0x10) ? 0 : 1);
  if (event->x == 0 && event->z == 0) {
    sector = (int16_t)(int8_t)event->sector_byte;
  } else {
    int32_t angle = 0;
    fifa96_err_t rc = fifa96_action_kick_angle(event->x, event->z, &angle);
    if (rc != FIFA96_OK) return rc;
    sector = (int16_t)((uint16_t)((angle + 0x40) & 0x3FF) >> 7);
  }
  if (event->code == 0x40) {
    idx = 0;
  } else {
    idx = ((uint32_t)sector_table[event->subtype] >>
           ((uint8_t)sector & 0x1Fu)) &
          1u;
  }
  d = (int16_t)(event->ball_height - (int16_t)event->height);
  if (event->height != 0) {
    if (d < 0x38) return FIFA96_OK;
    out->table = FIFA96_ACTION_KICK_EVENT_TABLE_HEIGHT;
    out->index = (uint32_t)(2 * class_id) + idx;
  } else if (event->has_slot != 0 && event->active != 0 &&
             event->phase == 2 && class_id == 1 && event->slot_counter < 7) {
    out->table = FIFA96_ACTION_KICK_EVENT_TABLE_CARRY;
    out->index = idx;
  } else if (event->has_slot != 0 && event->code == 0x60) {
    out->table = FIFA96_ACTION_KICK_EVENT_TABLE_CARRY;
    out->index = idx + 2;
  } else {
    int32_t band = d < 0x20 ? 0 : (d < 0x70 ? 1 : 2);
    out->table = event->active != 0 ? FIFA96_ACTION_KICK_EVENT_TABLE_ACTIVE
                                    : FIFA96_ACTION_KICK_EVENT_TABLE_IDLE;
    out->index = (uint32_t)(2 * band + 6 * class_id) + idx;
  }
  out->found = 1;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_kick_event_append(uint8_t actor_action, uint8_t row0,
                                             fifa96_action_kick_append *out) {
  static const uint8_t row_codes[14] = {
      0x0D, 0x0F, 0x11, 0x0A, 0x09, 0x14, 0x13,
      0x16, 0x16, 0x03, 0x04, 0x04, 0x10, 0x0C,
  };
  if (!out) return -FIFA96_ERR_INVALID;
  out->direct = 0;
  out->code = 0;
  switch (actor_action) {
    case 0x11: out->code = 2; return FIFA96_OK;
    case 0x12: out->code = 7; return FIFA96_OK;
    case 0x13:
    case 0x20: out->code = 8; return FIFA96_OK;
    case 1:
      out->direct = 1;
      out->code = 1;
      return FIFA96_OK;
    case 0x10: out->code = 3; return FIFA96_OK;
    default: break;
  }
  if (row0 >= 1 && row0 <= 0x0E) {
    out->code = row_codes[row0 - 1];
  } else {
    out->direct = 1;
    out->code = 0;
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_kick_stage_target(uint8_t has_slot, uint16_t slot_word,
                                             uint8_t type8,
                                             const fifa96_action_vec3 *camera,
                                             const int8_t *offset_x,
                                             const int8_t *offset_z,
                                             fifa96_action_vec3 *out,
                                             uint8_t *resolved) {
  if (!camera || !offset_x || !offset_z || !out || !resolved)
    return -FIFA96_ERR_INVALID;
  if (has_slot != 0 && (slot_word == 0x60 || slot_word == 0x8000)) {
    out->x = (int32_t)((uint32_t)camera->x +
                       ((uint32_t)(int32_t)offset_x[type8] << 4));
    out->z = (int32_t)((uint32_t)camera->z +
                       ((uint32_t)(int32_t)offset_z[type8] << 4));
    *resolved = 1;
  } else {
    out->x = camera->x;
    out->y = camera->y;
    out->z = camera->z;
    *resolved = 0;
  }
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

fifa96_err_t fifa96_action_locomotion_restart_wait(int16_t lane, int32_t timer89, uint8_t *ready,
                                                   uint8_t *reset) {
  int32_t threshold;
  if (!ready || !reset) return -FIFA96_ERR_INVALID;
  threshold = lane > 0x40 ? 0x78 : 0xA;
  *ready = timer89 >= threshold ? 1u : 0u;
  *reset = (*ready != 0 && lane > 0x40) ? 1u : 0u;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_locomotion_placement_counter(int32_t move_attr, uint16_t limit,
                                                        uint16_t *counter) {
  uint16_t value;
  if (!counter) return -FIFA96_ERR_INVALID;
  value = (uint16_t)((move_attr >> 22) + 1);
  if (value > limit) value = limit;
  *counter = value;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_locomotion_phase1_clamp(uint8_t side, int32_t *target_z) {
  if (!target_z) return -FIFA96_ERR_INVALID;
  if (side == 0) {
    if (*target_z > -0x20) *target_z = -0x20;
  } else if (*target_z < 0x20) {
    *target_z = 0x20;
  }
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

fifa96_err_t fifa96_action_carrier_arm(fifa96_action_possession *state,
                                       fifa96_action_carrier *carrier,
                                       const int8_t *type_dir_x,
                                       const int8_t *type_dir_z,
                                       fifa96_action_carrier_out *out) {
  int claimed = 0;
  if (!state || !carrier || !out) return -FIFA96_ERR_INVALID;
  memset(out, 0, sizeof *out);
  out->stage = carrier->stage92;   /* unchanged unless the latch machine writes */
  /* 0x7F19F: byte[EAX+0x9E] = 1, unconditional, before the phase check (the
   * engine `ran` field). */
  out->ran_set = 1;
  /* 0x7F1A6..0x7F1BA: phase != 2 -> FUN_0007DAB4 and return. */
  if (carrier->phase != 2) {
    out->reset = 1;
    return FIFA96_OK;
  }
  /* 0x7F1BF..0x7F205: claim the 0x58724 block for the actor. */
  if (fifa96_action_possession_claim(state, carrier->actor, &claimed) != FIFA96_OK) {
    return -FIFA96_ERR_INVALID;
  }
  out->claim = (uint8_t)claimed;
  /* 0x7F20B..0x7F217: team+0x7B2 = rec; team+0x7B6 = 0. */
  out->team_target = 1;
  /* 0x7F221..0x7F23A: the capped additive timer. */
  if (fifa96_action_possession_timer(&carrier->timer89, carrier->delta) != FIFA96_OK) {
    return -FIFA96_ERR_INVALID;
  }
  /* 0x7F240..0x7F248: word+0x81 != 0 returns. */
  if (carrier->timer81 != 0) return FIFA96_OK;
  /* 0x7F24E..0x7F258: the camera triple copy to +0x4D/+0x51/+0x55. */
  out->target_camera = 1;
  switch (carrier->stage92) {
  case 0:
    /* 0x7F274..0x7F27D: lane > 0x40 clears the controlled actor. */
    if (carrier->lane > 0x40) {
      out->clear_control = 1;
      return FIFA96_OK;
    }
    /* 0x7F291..0x7F2A2: [0x157A83] = rec, then close > bound waits. */
    out->set_control = 1;
    if ((int16_t)carrier->lane > carrier->bound_word) return FIFA96_OK;
    /* 0x7F2A8: an active release countdown waits. */
    if (state->release_timer > 0) return FIFA96_OK;
    /* 0x7F2B5: an airborne record waits. */
    if (carrier->airborne != 0) return FIFA96_OK;
    {
      fifa96_action_dribble_dir dir;
      if (fifa96_action_possession_dribble_dir(carrier->type8, carrier->ball_height,
                                               carrier->has_slot, carrier->slot_dir_x,
                                               carrier->slot_dir_z, type_dir_x, type_dir_z,
                                               &dir) != FIFA96_OK) {
        return -FIFA96_ERR_INVALID;
      }
      if (dir.resolved != 0) {
        out->dirs = 1;
        out->dir_x = (uint8_t)dir.dir_x;
        out->dir_z = (uint8_t)dir.dir_z;
        /* 0x7F386..0x7F57B: the dir-byte writes and the unported stage-0
         * target algebra (0x92820/0x71C94/0x79CCC/0x6DA64 and the record
         * target writes, OL-63). */
        out->tail = 1;
      } else if (carrier->team_slot_pool != 0 && carrier->team_chosen == 0 &&
                 (carrier->team_search_gate != 0 || carrier->active != 0)) {
        /* 0x7F32D..0x7F35F: the team gates reach FUN_0007876C (request); the
         * native continuation is the merge result (a slot -> 0x7F30A slot arm
         * -> tail, no slot -> the FUN_0007F7E0 fallback + type8 gate), so the
         * caller owns whether `tail` runs. */
        out->slot_merge = 1;
      } else {
        /* 0x7F361..0x7F36F: FUN_0007F7E0 (unported); 0x7F374..0x7F380 gates
         * the continuation on `[rec+0x8E]>>24 == 5`, so only that case
         * reaches the dir-byte writes/tail. */
        out->fallback = 1;
        if (carrier->type8 == 5u) out->tail = 1;
      }
    }
    return FIFA96_OK;
  case 1:
    /* 0x7F57C..0x7F585: FUN_00092820(rec, 0x26) (presentation-side, OL-27). */
    out->sink = 1;
    /* 0x7F58A..0x7F598: camera velocity 0x1577BE/C0/C2 = 0 (derived). */
    out->camera_zero = 1;
    /* 0x7F598..0x7F5C2: anim code active ? 6 : 0x30 through 0x6E598 (both
     * constants are below the helper's 0x6F clamp, so the clamp never fires).
     * The id-resolution subset is `fifa96_arm_anim_select`
     * (fifa96_arm_helpers.c); inline for the link-cycle reason above. */
    out->anim = carrier->active != 0 ? 6u : 0x30u;
    /* 0x7F5C7..0x7F5D9: timer 0, latch +1. */
    carrier->timer89 = 0;
    carrier->stage92 = (uint8_t)(carrier->stage92 + 1);
    out->stage = carrier->stage92;
    return FIFA96_OK;
  case 2:
    /* 0x7F5E9..0x7F5ED: no control slot waits. */
    if (carrier->has_slot == 0) return FIFA96_OK;
    /* 0x7F5EF..0x7F5F1: FUN_00079B1C snap (target = pos, lane/velocity zero). */
    out->snap = 1;
    /* 0x7F5F6..0x7F607: the 0x79C50 face over slot[+0x1D]/[+0x1E]. This is the
     * same fold as `fifa96_arm_face` (fifa96_arm_helpers.c); it is inline here
     * because `fifa96_arm_helpers` already links this library (a call would
     * create a static-library cycle). Keep the two in sync. */
    {
      int32_t dx = carrier->slot_dir_x;
      int32_t dz = carrier->slot_dir_z;
      out->face = carrier->facing;
      if (dx != 0 || dz != 0) {
        int32_t angle = 0;
        if (fifa96_action_kick_angle(dx, dz, &angle) != FIFA96_OK) {
          return -FIFA96_ERR_INVALID;
        }
        out->face = (uint8_t)(((uint32_t)(angle + 0x40) & 0x3FFu) >> 7u);
      }
      carrier->facing = out->face;
    }
    /* 0x7F60C..0x7F614: word[slot+6] == 0 waits. */
    if (carrier->slot_live == 0) return FIFA96_OK;
    /* 0x7F616..0x7F61A: latch loops to 0. */
    carrier->stage92 = 0;
    out->stage = 0;
    return FIFA96_OK;
  case 3:
    /* 0x7F627..0x7F62B: byte+0x44 == 0 waits. */
    if (carrier->event_flag44 == 0) return FIFA96_OK;
    /* 0x7F62D..0x7F630: latch 0. */
    carrier->stage92 = 0;
    out->stage = 0;
    /* 0x7F637..0x7F657: only the team target hands the ball actor to code 4
     * and arms the receiver timer (FUN_00079B58). */
    if (carrier->is_team_target != 0) out->handoff = 1;
    return FIFA96_OK;
  default:
    /* 0x7F261..0x7F267: CMP AL,3 / JA -> 0x7F65C return. */
    return FIFA96_OK;
  }
}

fifa96_err_t fifa96_action_receive_step(fifa96_action_receive *state,
                                        const fifa96_entity_candidate *team_candidates,
                                        uint32_t candidate_count,
                                        fifa96_action_receive_out *out) {
  if (!state || !out) return -FIFA96_ERR_INVALID;
  out->reset = 0;
  out->advance = 0;
  out->handoff = 0;
  out->stage = state->stage;
  out->ran = 0;
  out->nearest = -1;
  out->anim = 0;
  /* 0x8521F..0x8523D: phase != 2 or rec != [0x157A83] -> FUN_0007DAB4. */
  if (state->phase != 2u || state->tracked == 0) {
    out->reset = 1;
    return FIFA96_OK;
  }
  if (state->stage == 0) {
    out->ran = 1;   /* 0x8526F byte[+0x9E] = 1 */
    if (state->active == 0) {
      out->reset = 1;   /* 0x85278..0x8528A */
      return FIFA96_OK;
    }
    if (state->offset_word > 0x40) {
      /* 0x8528B..0x852AC: the timer-gated reset path. */
      if (state->timer89 > 0x3C) {
        out->reset = 1;
        out->handoff = state->is_team_target;
      }
      return FIFA96_OK;
    }
    /* 0x852AD..0x8534C: the 0x8DE8C nearest arm. The native then computes
     * `FUN_0008DCD4(pos -> nearest)` and the `FUN_000CD474` angle, but both
     * compares on the results are dead (no conditional jump between
     * 0x8530C/0x85320 and the 0x6E598 call); only the nearest selection and
     * the constant 0x4A animation id are observable. */
    if (!team_candidates || candidate_count == 0) return -FIFA96_ERR_INVALID;
    {
      int16_t best = 0;
      out->nearest = fifa96_entity_find_nearest(team_candidates, candidate_count,
                                                0xFFFFFFFFu, (int16_t)state->pos_x,
                                                (int16_t)state->pos_z, &best);
    }
    out->anim = 0x4A;
    state->timer89 = 0;
    state->stage = (uint8_t)(state->stage + 1u);
    out->advance = 1;
    out->stage = state->stage;
  }
  if (state->stage == 1) {
    /* 0x85352..0x85361: +0x44 or lane > 0x40 -> the reset path. */
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
  /* 0x82E85..0x82E96: pos_x > 0x210 returns only when camera_x > pos_x. */
  if (state->pos_x > 0x210 && state->camera_x > state->pos_x) return FIFA96_OK;
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
  out->ran = 0;
  out->target_set = 0;
  out->target_x = 0;
  out->target_y = 0;
  out->target_z = 0;
  out->receiver_timer = 0;
  if (state->phase != 2) {
    out->reset = 1;   /* 0x82F97..0x83156 */
    return FIFA96_OK;
  }
  state->timer89 += state->delta;
  if (state->stage == 0) {
    out->ran = 1;   /* 0x82FCC byte[+0x9E] = 1 */
    if (state->active == 0) {
      out->reset = 1;   /* 0x82FD5 -> 0x83156 */
      return FIFA96_OK;
    }
    state->timer89 = 0;
    state->stage = 1;
    out->stage = 1;
  }
  if (state->stage == 1) {
    int install = 0;
    /* 0x82FF3..0x8305A: the target arm runs before the window gates. */
    if (state->cam_f0 > 0x70 && state->cam_f8 < state->cam_fe) {
      if (state->slot_button_40 != 0) {
        out->target_x = state->vec794_x;
        out->target_y = state->vec794_y;
        out->target_z = state->vec794_z;
        if (state->side == 0) out->target_z -= 0xC0;
        else out->target_z += 0xC0;
      } else {
        out->target_x = state->vec788_x;
        out->target_y = state->vec788_y;
        out->target_z = state->vec788_z;
      }
      out->target_set = 1;
      if (state->flag99 == 0) out->receiver_timer = 1;   /* 0x79B58 */
    }
    if (state->lob != 0 || state->timer89 > 0x78) {
      out->reset = 1;
      goto tackle_tail;
    }
    (void)fifa96_action_tackle_attempt(state, &install);
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
  out->occupied = 0;
  out->bind = 0;
  out->target_set = 0;
  out->target_x = 0;
  out->target_z = 0;
  state->stride = 2;   /* 0x849B7 word[+0x7B] = 2 */
  state->timer89 += (int32_t)delta;
  if (state->stage == 0) {
    if (state->animation == 0x55 || state->animation == 0x6A) return FIFA96_OK;
    state->timer89 = 0;
    state->stage = 1;
  }
  if (state->stage == 1) {
    /* 0x84A28..0x84A67: target (0x900, 0, 0), the 0x8DCD4 metric into
     * +0x65/+0x67/+0x69, timer 0 and the latch +1. */
    int32_t dx = low16(0x900 - state->pos_x);
    int32_t dz = low16(0 - state->pos_z);
    out->target_set = 1;
    out->target_x = 0x900;
    out->target_z = 0;
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
    /* 0x84A94 resolution: FUN_0004C324 bind, the +0x9A latch and, with a
     * control slot, the NSEARCH/SWAP arm; 0x7DAB4 resets. */
    out->bind = 1;
    out->occupied = 1;
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

#define FIFA96_DUEL_SEARCH_MAX 11u

fifa96_err_t fifa96_action_duel_search(const fifa96_action_duel_search_in *in,
                                       const fifa96_action_duel_candidate *records,
                                       uint32_t count, int32_t *index) {
  int32_t values[FIFA96_DUEL_SEARCH_MAX];
  int32_t indices[FIFA96_DUEL_SEARCH_MAX];
  uint32_t found = 0;
  uint32_t i;
  uint32_t gap;
  if (!in || !records || !index || count == 0 || count > FIFA96_DUEL_SEARCH_MAX)
    return -FIFA96_ERR_INVALID;
  *index = -1;
  for (i = 0; i < count; i++) {
    const fifa96_action_duel_candidate *rec = &records[i];
    int32_t dx, dz, d;
    if (rec->has_slot != 0) continue;                      /* 0x8DB89 */
    if ((int32_t)i == (int32_t)in->skip_index) continue;   /* 0x8DB8F */
    if (rec->is_chosen != 0) continue;                     /* 0x8DB9A */
    if (i == 0 && in->record0_gate == 0) continue;         /* 0x8DBA2 */
    if (rec->skip_9a != 0) continue;                       /* 0x8DBAF */
    if (rec->skip_98 != 0) continue;                       /* 0x8DBB8 */
    dx = (int16_t)((uint16_t)in->x - (uint16_t)rec->pos_x);
    dz = (int16_t)((uint16_t)in->z - (uint16_t)rec->pos_z);
    d = (int32_t)(int16_t)fifa96_entity_distance(dx, dz);
    values[found] = -d;              /* 0x8DBE8 NEG */
    indices[found] = (int32_t)i;
    found++;
  }
  if (found != 0) {
    /* The native shell sort (0xA1860 called from 0x8DC08): gap = found/2 down
     * to 1, insert each value into its gap-ordered slot while
     * `values[j] < values[j+gap]` (equal values are not swapped), mirroring
     * every value swap on the index array; the first index slot wins. */
    gap = found >> 1;
    while (gap > 0u) {
      for (i = gap; i < found; i++) {
        uint32_t j = i - gap;
        for (;;) {
          int32_t tmp;
          if (values[j] >= values[j + gap]) break;
          tmp = values[j];
          values[j] = values[j + gap];
          values[j + gap] = tmp;
          tmp = indices[j];
          indices[j] = indices[j + gap];
          indices[j + gap] = tmp;
          if (j < gap) break;
          j -= gap;
        }
      }
      gap >>= 1;
    }
    *index = indices[0];
    return FIFA96_OK;
  }
  if (in->fallback != 0) {
    /* 0x8DC1B..0x8DC41: the first record with +0x20 == 0 and +0x9A == 0. */
    for (i = 0; i < count; i++) {
      if (records[i].has_slot == 0 && records[i].skip_9a == 0) {
        *index = (int32_t)i;
        return FIFA96_OK;
      }
    }
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_duel_swap(fifa96_action_duel_slot *from,
                                     fifa96_action_duel_slot *to) {
  if (!from || !to) return -FIFA96_ERR_INVALID;
  /* 0x786A2/0x786AC: only from-held and to-free. */
  if (from->has_slot == 0 || to->has_slot != 0) return FIFA96_OK;
  to->has_slot = 1;
  from->has_slot = 0;
  return FIFA96_OK;
}

/* `FUN_0007C990` forced-decision bounded code set {3,4,6} is the kick
 * machine's `kick_reset` model; rows 18/21/23 do not need it here (the engine
 * handler owns the predicate inputs). */

fifa96_err_t fifa96_action_duel_bind(const fifa96_action_duel_bind_in *in,
                                     fifa96_action_duel_bind_out *out) {
  if (!in || !out) return -FIFA96_ERR_INVALID;
  out->bound = 0;
  out->stub_36200 = 0;
  if (in->mode_157ac2 >= 4u && in->team_present != 0) {
    out->bound = (int32_t)in->side_826;   /* zero-extended byte */
    out->stub_36200 = 1;                  /* 0x4C359 FUN_00036200(0) */
    return FIFA96_OK;
  }
  out->bound = (int32_t)(int8_t)in->fallback_e6;   /* 0x4C363..0x4C36B */
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_stage_enter(fifa96_action_stage *state, const uint8_t *gates,
                                       uint8_t gate_count, fifa96_action_stage_out *out) {
  uint8_t i;
  if (!state || !gates || !out || gate_count == 0) return -FIFA96_ERR_INVALID;
  out->allowed = 0;
  out->reset = 0;
  out->advance = 0;
  out->stage = state->stage;
  for (i = 0; i < gate_count; i++) {
    if (gates[i] == state->phase) {
      out->allowed = 1;
      return FIFA96_OK;
    }
  }
  out->reset = 1;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_stage_tick(fifa96_action_stage *state) {
  if (!state) return -FIFA96_ERR_INVALID;
  state->timer89 = (int32_t)((uint32_t)state->timer89 + (uint32_t)state->delta);
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_stage_advance(fifa96_action_stage *state, fifa96_action_stage_out *out) {
  if (!state || !out) return -FIFA96_ERR_INVALID;
  state->timer89 = 0;
  state->stage = (uint8_t)(state->stage + 1u);
  out->allowed = 1;
  out->reset = 0;
  out->advance = 1;
  out->stage = state->stage;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_stage_finish(fifa96_action_stage *state, fifa96_action_stage_out *out) {
  if (!state || !out) return -FIFA96_ERR_INVALID;
  out->allowed = 1;
  out->reset = state->occupied != 0;
  out->advance = 0;
  out->stage = state->stage;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_stage_marker(int32_t marker, uint8_t *set_leader, uint8_t *hold) {
  if (!set_leader || !hold) return -FIFA96_ERR_INVALID;
  *set_leader = marker < 3 ? 1 : 0;
  *hold = marker < 5 ? 1 : 0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_stage_wait(int32_t timer89, int32_t threshold, uint8_t *ready) {
  if (!ready) return -FIFA96_ERR_INVALID;
  *ready = timer89 >= threshold ? 1u : 0u;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_phase_select(uint8_t phase, const uint32_t *table, uint32_t count,
                                        uint32_t *entry) {
  if (!table || !entry || count == 0) return -FIFA96_ERR_INVALID;
  if ((uint32_t)phase >= count) return -FIFA96_ERR_INVALID;
  *entry = table[phase];
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_phase_install(fifa96_action_phase_record *records, uint32_t count,
                                         uint8_t phase, const uint32_t *table,
                                         uint32_t table_count) {
  uint32_t i;
  if ((!records && count != 0) || !table || table_count == 0) return -FIFA96_ERR_INVALID;
  if ((uint32_t)phase >= table_count) return -FIFA96_ERR_INVALID;
  for (i = 0; i < count; i++) records[i].handler = table[phase];
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_phase_drive(uint8_t active, uint8_t *drive) {
  if (!drive) return -FIFA96_ERR_INVALID;
  *drive = active == 0 ? 1 : 0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_phase_cell(int8_t x, int8_t z, uint8_t side, fifa96_action_vec3 *out) {
  int32_t vx;
  int32_t vz;
  if (!out) return -FIFA96_ERR_INVALID;
  vx = (int32_t)x * 0x26;
  vz = (int32_t)z * 0x21;
  out->x = side ? -vx : vx;
  out->y = 0;
  out->z = side ? -vz : vz;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_phase_slot(int16_t x, int16_t z, uint8_t side, fifa96_action_vec3 *out) {
  int32_t vx;
  int32_t vz;
  if (!out) return -FIFA96_ERR_INVALID;
  vx = (int32_t)x;
  vz = (int32_t)z;
  vx += vx >> 2;
  vz += vz >> 2;
  out->x = side ? -vx : vx;
  out->y = 0;
  out->z = side ? -vz : vz;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_phase_ball_entry(int32_t ball_z, uint8_t side, uint32_t *index) {
  if (!index) return -FIFA96_ERR_INVALID;
  if (side == 0) {
    *index = ball_z > 0 ? 1u : 0u;
  } else {
    *index = ball_z < 0 ? 1u : 0u;
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_phase_ball_line(const int16_t *entries, int32_t ball_z, uint8_t side,
                                           fifa96_action_vec3 *out) {
  uint32_t index;
  int32_t vx;
  int32_t vz;
  if (!entries || !out) return -FIFA96_ERR_INVALID;
  fifa96_action_phase_ball_entry(ball_z, side, &index);
  vx = entries[index * 2];
  vz = entries[index * 2 + 1];
  vx += vx >> 2;
  vz += vz >> 2;
  out->x = side ? -vx : vx;
  out->y = 0;
  out->z = side ? -vz : vz;
  if (ball_z > 0) {
    out->z -= 0x60;
  } else {
    out->z += 0x60;
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_phase_line_timer(int32_t *timer89, uint16_t delta, uint8_t *ready) {
  if (!timer89 || !ready) return -FIFA96_ERR_INVALID;
  *timer89 = (int32_t)((uint32_t)*timer89 + (uint32_t)delta);
  *ready = *timer89 >= 0x3C ? 1u : 0u;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_phase_restart_line(uint8_t axis, int32_t offset, int16_t lateral,
                                              fifa96_action_vec3 *out) {
  int32_t delta;
  if (!out) return -FIFA96_ERR_INVALID;
  delta = (offset >> 25) * 5;
  if (axis == 0) delta = -delta;
  out->x = 0x780;
  out->z = delta;
  if (lateral < 0x20 && lateral > -0x20) {
    out->x = 0xCC0;
    out->z = 0;
  }
  return FIFA96_OK;
}

static int32_t fifa96_action_sequence_clamp_x(int32_t value) {
  if (value > 0x720) return 0x720;
  if (value < -0x720) return -0x720;
  return value;
}

static int32_t fifa96_action_sequence_clamp_z(int32_t value) {
  if (value > 0xB10) return 0xB10;
  if (value < -0xB10) return -0xB10;
  return value;
}

fifa96_err_t fifa96_action_sequence_select(uint8_t stage, const uint32_t *arms, uint32_t count,
                                           uint32_t *arm) {
  if (!arms || !arm || count == 0) return -FIFA96_ERR_INVALID;
  if ((uint32_t)stage >= count) return -FIFA96_ERR_INVALID;
  *arm = arms[stage];
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_sequence_event(uint8_t anim_byte, uint8_t event_id, uint8_t *post) {
  if (!post) return -FIFA96_ERR_INVALID;
  *post = anim_byte != event_id ? 1 : 0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_sequence_marker_target(uint8_t marker, const fifa96_action_vec3 *pos,
                                                  int32_t lead_x, fifa96_action_vec3 *out) {
  if (!pos || !out) return -FIFA96_ERR_INVALID;
  if (marker >= 2) {
    *out = *pos;
    return FIFA96_OK;
  }
  out->x = lead_x < 0 ? -0x30 : 0x30;
  out->z = 0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_sequence_marker(uint8_t marker, uint8_t want, uint8_t *match) {
  if (!match) return -FIFA96_ERR_INVALID;
  *match = marker == want ? 1 : 0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_sequence_rng_event(uint32_t rng, uint8_t even_id, uint8_t odd_id,
                                              uint8_t *event_id) {
  if (!event_id) return -FIFA96_ERR_INVALID;
  *event_id = (rng & 1u) ? odd_id : even_id;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_sequence_countdown(uint32_t rng, uint16_t *countdown) {
  if (!countdown) return -FIFA96_ERR_INVALID;
  *countdown = (uint16_t)((rng & 0x7Fu) + 0x20u);
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_sequence_anim_byte(const uint8_t *table, uint8_t index,
                                              uint16_t *value) {
  if (!table || !value) return -FIFA96_ERR_INVALID;
  *value = (uint16_t)((uint16_t)table[index] - 2u);
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_sequence_lane(uint8_t subtype, uint8_t side, int32_t boost,
                                         fifa96_action_sequence_lane_out *out) {
  int32_t block;
  if (!out) return -FIFA96_ERR_INVALID;
  block = (int32_t)subtype * 6;
  out->z = side ? block : -block;
  out->x = 0x7E0 + block;
  out->threshold = ((int32_t)subtype * ((boost >> 25) * 60)) >> 4;
  if (side) out->threshold += 60;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_sequence_scatter_celebration(const fifa96_action_vec3 *base,
                                                        int32_t dir_x, int32_t dir_z,
                                                        const uint32_t *rng,
                                                        fifa96_action_vec3 *points) {
  uint32_t i;
  if (!base || !rng || !points) return -FIFA96_ERR_INVALID;
  points[0] = *base;
  points[1].x = (int32_t)((uint32_t)base->x + ((rng[0] & 0x7Fu) + 0xA0u) * (uint32_t)dir_x);
  points[1].y = base->y;
  points[1].z = (int32_t)((uint32_t)base->z + ((rng[1] & 0x7Fu) + 0xA0u) * (uint32_t)dir_z);
  points[2].x = (int32_t)((uint32_t)points[1].x + ((rng[2] & 0x7Fu) + 0x140u) * (uint32_t)dir_x);
  points[2].y = base->y;
  points[2].z = (int32_t)((uint32_t)points[1].z + ((rng[3] & 0x7Fu) + 0x20u) * (uint32_t)dir_z);
  points[3].x = (int32_t)((0x5E0u - (rng[4] & 0x1FFu)) * (uint32_t)dir_x);
  points[3].y = base->y;
  points[3].z = (int32_t)((uint32_t)points[2].z + ((rng[5] & 0xFFu) + 0x140u) * (uint32_t)dir_z);
  points[4].x = (int32_t)((uint32_t)points[3].x + ((rng[6] & 0x7Fu) + 0x50u) * (uint32_t)dir_x);
  points[4].y = base->y;
  points[4].z = (int32_t)((uint32_t)points[3].z + ((rng[7] & 0x1FFu) + 0x280u) * (uint32_t)dir_z);
  for (i = 0; i < FIFA96_ACTION_SEQUENCE_SCATTER_POINTS; i++) {
    points[i].x = fifa96_action_sequence_clamp_x(points[i].x);
    points[i].z = fifa96_action_sequence_clamp_z(points[i].z);
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_sequence_scatter_stats(const fifa96_action_vec3 *base, int32_t dir_x,
                                                  int32_t dir_z, const uint32_t *rng,
                                                  fifa96_action_vec3 *points) {
  uint32_t i;
  if (!base || !rng || !points) return -FIFA96_ERR_INVALID;
  points[0] = *base;
  points[1].x = (int32_t)((rng[0] % 0xF0u) * (uint32_t)dir_x);
  points[1].y = base->y;
  points[1].z = (int32_t)((uint32_t)base->z + ((rng[1] & 0xFFu) + 0x1E0u) * (uint32_t)dir_z);
  points[2].x = (int32_t)((uint32_t)points[1].x + ((rng[2] & 0x7Fu) + 0x20u) * (uint32_t)dir_x);
  points[2].y = base->y;
  points[2].z = (int32_t)((uint32_t)points[1].z + ((rng[3] & 0xFFu) + 0x140u) * (uint32_t)dir_z);
  points[3].x = points[2].x;
  points[3].y = base->y;
  points[3].z = (int32_t)((uint32_t)points[2].z + ((rng[4] & 0xFFu) + 0x3C0u) * (uint32_t)dir_z);
  points[4].x = (int32_t)((uint32_t)points[3].x + ((rng[5] & 0x7Fu) + 0x50u) * (uint32_t)dir_x);
  points[4].y = base->y;
  points[4].z = (int32_t)((uint32_t)points[3].z + ((rng[6] & 0x1FFu) + 0x3C0u) * (uint32_t)dir_z);
  for (i = 0; i < FIFA96_ACTION_SEQUENCE_SCATTER_POINTS; i++) {
    points[i].x = fifa96_action_sequence_clamp_x(points[i].x);
    points[i].z = fifa96_action_sequence_clamp_z(points[i].z);
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_sequence_duel_event(int16_t delta_angle, int16_t aim, int16_t facing,
                                               int16_t atan_delta,
                                               fifa96_action_sequence_duel *out) {
  int16_t diff;
  if (!out) return -FIFA96_ERR_INVALID;
  out->reset = 0;
  out->event_id = 0;
  if (delta_angle <= 0x20 || delta_angle >= 0x70) {
    out->reset = 1;
    return FIFA96_OK;
  }
  if (aim <= 4) {
    out->event_id = 0x0C;
    return FIFA96_OK;
  }
  diff = (int16_t)(facing - atan_delta);
  if (diff < 0) diff = (int16_t)-diff;
  out->event_id = diff < 0x1000 ? 0x59 : 0x0C;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_sequence_press_event(int16_t height, int32_t timer89,
                                                fifa96_action_sequence_press *out) {
  if (!out) return -FIFA96_ERR_INVALID;
  out->fire = height <= 0x30 ? 1 : 0;
  out->reset = (!out->fire && timer89 > 0x1E) ? 1 : 0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_sequence_event_ids(const uint8_t *t344, const uint8_t *t346,
                                              const uint8_t *t349, const uint8_t *t34c,
                                              const uint32_t *rng, uint8_t *ids) {
  if (!t344 || !t346 || !t349 || !t34c || !rng || !ids) return -FIFA96_ERR_INVALID;
  ids[0] = t344[rng[0] & 1u];
  if (ids[0] == 0x67 && (rng[1] & 1u)) {
    ids[1] = 0x67;
    ids[2] = 0x67;
    ids[3] = 0x67;
    ids[4] = 0x67;
    return FIFA96_OK;
  }
  ids[1] = ids[0];
  ids[2] = t349[rng[2] % 3u];
  ids[3] = t34c[rng[3] % 9u];
  ids[4] = ids[3];
  if (rng[4] & 1u) ids[1] = t346[rng[5] % 3u];
  if (ids[3] == 0x58 || ids[3] == 0x5B || ids[3] == 0x5F || ids[3] == 0x6B) {
    if ((rng[6] & 3u) == 0) ids[4] = 0x68;
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_action_sequence_velocity_scale(int16_t type_x, int16_t type_z,
                                                   int16_t *vel_x, int16_t *vel_z,
                                                   int16_t *speed) {
  int32_t scale;
  if (!vel_x || !vel_z || !speed) return -FIFA96_ERR_INVALID;
  scale = (type_x != 0 && type_z != 0) ? 3 : 4;
  *vel_x = (int16_t)((int32_t)type_x * scale);
  *vel_z = (int16_t)((int32_t)type_z * scale);
  *speed = (int16_t)fifa96_entity_distance(*vel_x, *vel_z);
  return FIFA96_OK;
}

/* ===== FU-139 §9 (Task 11): rows 07/0F kick machines =====
 *
 * First-hand evidence: /FIFA96.EXE `disassemble_bytes` `0x814B0..0x81760`
 * (row 07, 186+ insns), `0x82AD0..0x82B60` + `0x82B60..0x82DD0` (row 0F);
 * helper decompiles `0x7DAB4` (reset), `0x7C990` (forced-decision install),
 * `0x7E600` (the defender decision), `0x79B1C` (snap), `0x79B6C`
 * (re-anchor/face), `0x79B58` (receiver timer), `0x78A84`/`0x78AA4`
 * (slot backup/restore), `0x78B00` (slot clear); `read_memory 0x110680` (the
 * per-type decision gate bytes `03 00 00 03 03 03 03 02 ...`); Ghidra
 * read-only. */

/* `0x110680[type]`: the FUN_0007E600 per-type decision gate (bytes 0..25). */
static const uint8_t kick_decision_gate[26] = {
    0x03, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
};

/* `FUN_0007DAB4` (`0x7DAB4..0x7DB0C`) plus the `FUN_0007C990` forced-decision
 * install (`0x7C990..0x7CA4C`). `plain` forces the non-0x7C990 path
 * (`FUN_0007D9A4(rec, 0, 0, 0)`). The native slot callback `FUN_00078B00` is
 * the `slot_callback` request (presentation-side, OL-65). */
static fifa96_err_t kick_reset(const fifa96_action_kick *state,
                               fifa96_action_kick_out *out) {
  uint8_t code = 0;
  out->reset = 1;
  out->reset_install = 1;
  out->reset_code = 0;
  if (state->has_slot != 0) out->slot_callback = 1;
  /* 0x7DAEF: only phase 2 + an active record reaches FUN_0007C990; every
   * other path takes the FUN_0007D9A4(rec, 0, 0, 0) install. */
  if (state->phase != 2u || state->active == 0u) return FIFA96_OK;
  if (state->is_team_target) {
    if (state->opp_target_present == 0 || state->opp_target_carrier == 0) {
      if (state->type == 5u) {
        out->reset_install = 0;    /* 0x7C9C8: type 5 returns without install */
        return FIFA96_OK;
      }
      code = 4;
    } else {
      code = 6;
    }
    out->reset_code = code;
    return FIFA96_OK;
  }
  if (state->is_team_second) {
    if (state->opp_target_present != 0 && state->opp_target_carrier != 0) {
      out->reset_code = 6;
      return FIFA96_OK;
    }
    if (state->team_target_present == 0 || state->team_target_carrier == 0) {
      out->reset_code = 4;
      return FIFA96_OK;
    }
  }
  out->reset_code = 3;
  return FIFA96_OK;
}

/* `FUN_0007E600` (`0x7E600..0x7E7C5`, 176 insns): the defender decision that
 * installs action `0x0E` on the record. Bounded inputs: phase, the 0x110680
 * type gate, `[0x1577CA]`, the lane `0x180` bound, the `0x71B9C` predictor
 * triple (caller input), the `0x8DCD4` pos/predictor distance <= 0xF0 and
 * <= lane, the camera x bounds, the side/pos_z bounds, the `0x8DD70` angle
 * inside +/-0x100 (side 0) or outside (side 1) and the `|angle - word[+0x7D]|`
 * `0x100` gate. */
static uint8_t kick_decision(const fifa96_action_kick *s) {
  int16_t dx, dz;
  int32_t distance;
  int32_t angle = 0;
  int32_t diff;
  if (s->phase != 2) return 0;
  if (s->type >= sizeof kick_decision_gate ||
      (kick_decision_gate[s->type] & 1u) == 0)
    return 0;
  if (s->decision_excluded != 0) return 0;
  if (s->lane_word > 0x180) return 0;
  if (s->predictor_y > 0x60 || s->predictor_y < 0x20) return 0;
  dx = (int16_t)((uint16_t)s->predictor_x - (uint16_t)s->pos_x);
  dz = (int16_t)((uint16_t)s->predictor_z - (uint16_t)s->pos_z);
  distance = fifa96_entity_distance(dx, dz);
  if (distance > 0xF0) return 0;
  if ((int16_t)distance > s->lane_word) return 0;
  if (s->pos_x < -0x1E0 && s->pos_x > s->camera_x) return 0;
  if (s->pos_x > 0x1E0 && s->camera_x > s->pos_x) return 0;   /* 0x7E6C8 */
  if (s->side == 0u) {
    if ((int16_t)s->pos_z < 0x7B0) return 0;
  } else if (s->side == 1u) {
    if ((int16_t)s->pos_z > -0x7B0) return 0;
  }
  if (fifa96_action_kick_angle(dx, dz, &angle) != FIFA96_OK) return 0;
  if (s->side == 0u) {
    if (angle < -0x100 || angle > 0x100) return 0;
  } else if (angle > -0x100 && angle < 0x100) {
    return 0;
  }
  diff = (int32_t)(((uint16_t)((uint16_t)angle - (uint16_t)s->face_word7d)) & 0x3FFu);
  if (diff > 0x200) diff = 0x400 - diff;
  if (diff > 0x100) return 0;
  return 1;
}

/* The `0x79C50` face fold: stores the angle at +0x7D and the octant at +0x8E
 * (the tested `fifa96_arm_face` semantics; inlined for the static-link cycle
 * with `fifa96_arm_helpers`). */
static fifa96_err_t kick_face(fifa96_action_kick *s, int16_t dx, int16_t dz) {
  int32_t angle = 0;
  if (dx == 0 && dz == 0) return FIFA96_OK;
  if (fifa96_action_kick_angle(dx, dz, &angle) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  s->face_word7d = (int16_t)angle;
  s->facing = (uint8_t)(((uint32_t)(angle + 0x40) & 0x3FFu) >> 7u);
  return FIFA96_OK;
}

/* The row-07 reset tail `0x81702`: FUN_0007DAB4 then, for the team target,
 * the ball-actor install 4 ([0x158730]) and the FUN_00079B58 receiver timer
 * ([0x158734]). */
static fifa96_err_t kick_tail_07(fifa96_action_kick *s,
                                 fifa96_action_kick_out *out) {
  if (kick_reset(s, out) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  if (s->is_team_target != 0) {
    out->ball_install = 1;
    out->receiver_timer = 1;
  }
  return FIFA96_OK;
}

static fifa96_err_t kick_machine_07(fifa96_action_kick *s,
                                    fifa96_action_kick_out *out) {
  if (s->kick_done == 0u) {
    if (s->phase != 2u || s->timer81 != 0u) return kick_tail_07(s, out);
    s->timer89 = (int32_t)((uint32_t)s->timer89 + (uint32_t)(uint16_t)s->delta);
  }
  switch (s->stage92) {
  case 0: {
    int16_t t = (int16_t)((uint16_t)s->pos_y_word + 0x70u);
    out->ran = 1;                                  /* 0x81512 */
    if (s->lane_word > 0x40 || (int32_t)t < (int32_t)s->ball_height) {
      if (s->timer89 > 0x3C) return kick_tail_07(s, out);  /* 0x81538 */
      out->stage = s->stage92;
      return FIFA96_OK;
    }
    if (s->type_off_x == NULL || s->type_off_z == NULL)
      return -FIFA96_ERR_INVALID;
    {
      fifa96_action_vec3 camera;
      fifa96_action_vec3 target;
      uint8_t resolved = 0;
      /* The native resolved arm leaves +0x51 untouched, so seed the out
       * triple with the record's current target. */
      target.x = s->target_x;
      target.y = s->target_y;
      target.z = s->target_z;
      camera.x = s->camera_x;
      camera.y = s->camera_y;
      camera.z = s->camera_z;
      if (fifa96_action_kick_stage_target(s->has_slot, (uint16_t)s->slot_word6,
                                          s->type8, &camera, s->type_off_x,
                                          s->type_off_z, &target,
                                          &resolved) != FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      s->target_x = target.x;
      s->target_y = target.y;
      s->target_z = target.z;
      s->target_resolved = resolved;
    }
    s->timer89 = 0;                                /* 0x815B5 */
    s->stage92 = (uint8_t)(s->stage92 + 1u);
    out->stage = s->stage92;
    return FIFA96_OK;
  }
  case 1: {
    int16_t si = 0;
    if (s->kick_done == 0u) {
    if (s->is_team_cb != 0u) si = 0x40;
    else if (s->has_slot != 0u) si = s->slot_word6;
    else if (s->staged_code == 3u) si = 0x40;
    else si = -1;
    if (si == 0x40) {
      if (kick_decision(s) != 0) {                 /* 0x81605..0x81617 */
        out->defender_install = 1;
        out->stage = s->stage92;
        return FIFA96_OK;
      }
    }
    if (s->downgrade_gate != 0u && s->downgrade_word == 4 && si == 0x40)
      si = 0x20;                                   /* 0x8161D..0x81637 */
    out->kick = 1;
    out->kick_mode = (uint8_t)(uint16_t)si;
    out->stage = s->stage92;
    return FIFA96_OK;
    }
    /* 0x8164B..0x816E4: the post-kick opponent invoke. */
    if (s->kick_staged != 0u && s->kick_traj < 0x30 && s->timer89 < 5 &&
        s->opp_present != 0u && s->opp_type == 6u && s->opp_has_slot == 0u &&
        s->opp_lane_word < 0xD0) {
      int32_t angle = 0;
      int32_t diff;
      if (fifa96_action_kick_angle(s->opp_angle_x, s->opp_angle_z, &angle) !=
          FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      diff = (int32_t)(((uint16_t)((uint16_t)angle -
                                   (uint16_t)s->opp_face_word)) &
                       0x3FFu);
      if (diff > 0x200) diff = 0x400 - diff;
      if (diff < 0x55) out->opponent_invoke = 1;
    }
    s->timer89 = 0;
    s->stage92 = (uint8_t)(s->stage92 + 1u);
    out->stage = s->stage92;
    return FIFA96_OK;
  }
  case 2:
    if (s->byte44 != 0u) return kick_tail_07(s, out);
    out->stage = s->stage92;
    return FIFA96_OK;
  default:
    out->stage = s->stage92;
    return FIFA96_OK;
  }
}

static fifa96_err_t kick_machine_0F(fifa96_action_kick *s,
                                    fifa96_action_kick_out *out) {
  if (s->kick_done == 0u) {
    if (s->phase != 2u) return kick_reset(s, out);
    s->timer89 = (int32_t)((uint32_t)s->timer89 + (uint32_t)(uint16_t)s->delta);
  }
  switch (s->stage92) {
  case 0: {
    if (s->active == 0u) return kick_reset(s, out);   /* 0x82B21 */
    if (s->word85 != 0u) return kick_reset(s, out);   /* 0x82B3B */
    {
      /* 0x82B49: 0x8DCD4(pos, 0x157788, local); below 0x50 the position
       * advances by the half vector (SAR 17 of the dword addends). */
      int16_t dx = (int16_t)((uint16_t)s->stage_target_x - (uint16_t)s->pos_x);
      int16_t dz = (int16_t)((uint16_t)s->stage_target_z - (uint16_t)s->pos_z);
      int16_t distance = (int16_t)fifa96_entity_distance(dx, dz);
      if (distance < 0x50) {
        s->pos_x = (int32_t)((uint32_t)s->pos_x + (uint32_t)(dx >> 1));
        s->pos_z = (int32_t)((uint32_t)s->pos_z + (uint32_t)(dz >> 1));
      }
    }
    /* 0x82B84: 0x79B6C re-anchor: target = pos, y = 0, lane/vel zero, the
     * camera-facing fold and the anim (inactive -> 0x26; active -> 0). */
    s->target_x = s->pos_x;
    s->target_y = 0;
    s->target_z = s->pos_z;
    s->face_word7d = 0;
    if (kick_face(s, (int16_t)((uint16_t)s->camera_x - (uint16_t)s->pos_x),
                  (int16_t)((uint16_t)s->camera_z - (uint16_t)s->pos_z)) !=
        FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    out->camera_face = 1;
    out->anim = 0;   /* 0x6E598(active path); 0x26 is unreachable (0x82B21) */
    out->ran = 1;                                  /* 0x82BAA */
    s->timer89 = 0;
    s->stage92 = (uint8_t)(s->stage92 + 1u);
    out->stage = s->stage92;
    return FIFA96_OK;
  }
  case 1: {
    if (s->kick_done == 1u || s->kick_done == 2u) goto kick_reload;
    /* 0x82BC9: 0x79B1C snap. */
    s->target_x = s->pos_x;
    s->target_y = s->pos_y;
    s->target_z = s->pos_z;
    out->snap = 1;
    if (s->byte44 != 0u) return kick_reset(s, out);   /* 0x82BD0 */
    if (s->timer81 != 0u) {                              /* 0x82BDD */
      out->stage = s->stage92;
      return FIFA96_OK;
    }
    if (s->has_slot == 0u) {                             /* 0x82BF1 */
      if (s->team_slot_pool != 0u && s->lane_word < 0xF0 &&
          s->merge_gate_1586d7 == 0u)
        out->slot_merge = 1;
    }
    if (s->has_slot != 0u && s->slot_word6 != 0)         /* 0x82C19 */
      out->slot_backup = 1;
    if (s->lane_word > s->bound_word) {                  /* 0x82C30 */
      out->stage = s->stage92;
      return FIFA96_OK;
    }
    if (s->opp2_present != 0u) {                         /* 0x82C3E */
      int16_t self_margin =
          (int16_t)((uint16_t)s->ball_height -
                    (uint16_t)((uint16_t)s->pos_y_word + 0x70u));
      int16_t opp_margin =
          (int16_t)((uint16_t)s->ball_height -
                    (uint16_t)((uint16_t)s->opp2_pos_y_word + 0x70u));
      if (self_margin > opp_margin && s->lane_word > s->opp2_lane_word) {
        out->stage = s->stage92;
        return FIFA96_OK;
      }
    }
    if (s->lane_word > 0x30) {                           /* 0x82C85 */
      out->stage = s->stage92;
      return FIFA96_OK;
    }
    if ((int16_t)((uint16_t)s->pos_y_word + 0x80u) < s->ball_height) {
      out->stage = s->stage92;
      return FIFA96_OK;
    }
    if (s->has_slot != 0u) {                             /* 0x82CAC */
      out->slot_restore = 1;
      if (s->slot_word6 != 0) {
        out->kick = 1;
        out->kick_mode = (uint8_t)(uint16_t)s->slot_word6;
        out->stage = s->stage92;
        return FIFA96_OK;
      }
    }
    {
      /* 0x82CDA: the predictor distance vs the lane. */
      int16_t dx = (int16_t)((uint16_t)s->predictor_x - (uint16_t)s->pos_x);
      int16_t dz = (int16_t)((uint16_t)s->predictor_z - (uint16_t)s->pos_z);
      int16_t distance = (int16_t)fifa96_entity_distance(dx, dz);
      if (distance <= s->lane_word) {   /* BX == 0 -> 0x82DA5 returns */
        out->stage = s->stage92;
        return FIFA96_OK;
      }
    }
    /* 0x82D19..0x82D81: the corner staging (0x6DBCC code + cell, the camera
     * triple, the code-3/RNG mode and the face) then kick 2 with the slot
     * temporarily nulled. */
    {
      int16_t dx = (int16_t)((uint16_t)s->corner_x - (uint16_t)s->camera_x);
      int16_t dz = (int16_t)((uint16_t)s->corner_z - (uint16_t)s->camera_z);
      int16_t distance = (int16_t)fifa96_entity_distance(dx, dz);
      s->kick_vec_x = distance;
      s->kick_vec_height = dx;
      s->kick_vec_z = dz;
      out->corner_kick = 1;
      s->staged_code = s->corner_code;
      if (s->corner_code == 3u) {
        out->corner_kick_mode = 0x40;
      } else {
        uint16_t r = 0;
        out->corner_kick_mode = 0x20;
        if (s->rng == NULL) return -FIFA96_ERR_INVALID;
        if (fifa96_rng_step(s->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
        if ((r & 7u) != 0) {
          if (kick_face(s, s->kick_vec_height, s->kick_vec_z) != FIFA96_OK)
            return -FIFA96_ERR_INVALID;
          out->corner_face = 1;
        }
      }
      out->stage = s->stage92;
      return FIFA96_OK;
    }
  kick_reload:
    /* 0x82DA2..0x82DBE: word[+0x81] = 2*word[+0x85] - word[+0x87] + 0x1E. */
    out->timer81_reload = (int16_t)(uint16_t)(((uint16_t)s->word85 << 1) -
                                              (uint16_t)s->word87 + 0x1Eu);
    out->timer81_set = 1;
    s->timer81 = (uint16_t)out->timer81_reload;
    out->stage = s->stage92;
    return FIFA96_OK;
  }
  default:
    out->stage = s->stage92;
    return FIFA96_OK;
  }
}

fifa96_err_t fifa96_action_kick_machine(fifa96_action_kick *state,
                                        fifa96_action_kick_out *out) {
  if (!state || !out) return -FIFA96_ERR_INVALID;
  memset(out, 0, sizeof *out);
  out->stage = state->stage92;
  /* The reset consequences are applied here (FUN_0007DAB4 0x7DABA/0x7DAC4). */
  if (state->row == 0x07u) {
    fifa96_err_t rc = kick_machine_07(state, out);
    if (rc != FIFA96_OK) return rc;
  } else if (state->row == 0x0Fu) {
    fifa96_err_t rc = kick_machine_0F(state, out);
    if (rc != FIFA96_OK) return rc;
  } else {
    return -FIFA96_ERR_INVALID;
  }
  if (out->reset != 0) {
    state->stage92 = 0xFFu;
    state->timer89 = 0;
    out->stage = state->stage92;
  } else {
    out->stage = state->stage92;
  }
  return FIFA96_OK;
}
