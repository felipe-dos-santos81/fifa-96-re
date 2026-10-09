#include "fifa96_loader/fifa96_keeper.h"

#include <string.h>

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
                                    uint8_t code, uint8_t phase_latch,
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
  out->reset = (code == 0x1F && phase_latch == 0) ? 1 : 0;
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
  if (in->code != 0x1F && (in->type_gate & 1u) == 0) return FIFA96_OK;
  if (in->is_actor != 0 || in->code == 5) return FIFA96_OK;
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

/* ===== FU-151 §Port contract item 1 — row-1E ten-stage machine ============= */

/* The `0x8DCD4`/`0x795B4` word-difference triple ({band,dx,dz}) the machine's
 * 0x157C30/0x158738 staging blocks use. */
static void keeper_stage_vec(const fifa96_keeper_point *from, const fifa96_keeper_point *to,
                             int16_t *band, int16_t *dx, int16_t *dz) {
  fifa96_keeper_vec v;
  (void)fifa96_keeper_vec_from_delta(from, to, &v);
  *band = v.distance;
  *dx = v.dx;
  *dz = v.dz;
}

/* `FUN_0008DC50` (0x8DC50..0x8DC67, first-hand): the truncating word shift
 * (the FU-140 `keeper_trunc_shift` above — non-negative takes the sign-extended
 * SAR, negative takes NEG/SAR/NEG, i.e. truncation toward zero). */

/* The `0x760DF` common exit: the latch camera request (0x4C31C) and the gauge
 * accumulation (`0x8DCD4(rec+0x59, 0x157C36)` -> `word[0x157C42] += dist`,
 * saved point := rec pos). */
static void keeper_claim_exit(fifa96_keeper_claim *s, fifa96_keeper_claim_out *out) {
  fifa96_keeper_point from;
  fifa96_keeper_point to;
  int16_t band = 0;
  int16_t dx = 0;
  int16_t dz = 0;
  if (s->latch_157ab2 != 0) out->ui |= 0x400u;    /* 0x760ee CALL 0x4C31C */
  from.x = s->pos.x;
  from.y = 0;
  from.z = s->pos.z;
  to.x = s->saved_x;
  to.y = 0;
  to.z = s->saved_z;
  keeper_stage_vec(&from, &to, &band, &dx, &dz);
  s->gauge = (int16_t)((uint16_t)s->gauge + (uint16_t)band);  /* 0x7611d */
  s->saved_x = (int16_t)s->pos.x;
  s->saved_z = (int16_t)s->pos.z;
}

fifa96_err_t fifa96_keeper_claim_step(fifa96_keeper_claim *s,
                                      fifa96_keeper_claim_out *out) {
  int stage;
  int flag8 = 0;
  if (!s || !out) return -FIFA96_ERR_INVALID;
  memset(out, 0, sizeof *out);
  stage = (int)s->stage92;
  /* Head 0x7551A..0x755CF: helper gate, then the claim while stage < 3 and
   * the record has no ball. */
  if (stage < 6 && s->has_slot == 0) out->helper = 1;              /* 0x75536 */
  if (stage < 3 && s->has_ball == 0) {                             /* 0x75547/0x75553 */
    s->cam_x = s->pos.x + (int32_t)s->offset_x * 0x10;             /* 0x75565..0x75579 */
    s->cam_y = s->pos.y + 0x38;                                    /* 0x7559b */
    s->cam_z = s->pos.z + (int32_t)s->offset_z * 0x10;             /* 0x7557f..0x75593 */
    out->place = 1;                                                /* 0x755c7 */
    out->place_x = s->cam_x;
    out->place_y = s->cam_y;
    out->place_z = s->cam_z;
    s->has_ball = 1;                                               /* 0x755c0 */
    out->claimed = 1;
    out->controlled = 1;                                           /* 0x755cf */
  }
  s->timer7b = 3;                                                  /* 0x755d4 */
  s->timer89 += (int32_t)s->delta;                                 /* 0x755e2..0x755f6 */
  stage = (int)s->stage92;
  if (stage > 9) goto claim_exit;                                  /* 0x755fe JA */
  if (stage == 0) {                                                /* 0x75611 */
    if (s->row44 == 0) return FIFA96_OK;                           /* 0x75615 -> 0x76127 */
    s->saved_x = (int16_t)s->pos.x;                                /* 0x75628 */
    s->saved_z = (int16_t)s->pos.z;
    s->gauge = 0;                                                  /* 0x7562b */
    s->latch_157ab2 = 1;                                           /* 0x75634 */
    s->timer89 = 0;                                                /* 0x75648 */
    s->stage92 = 1;                                                /* 0x75654 */
    stage = 1;
  }
  if (stage == 1) {                                                /* 0x7565a */
    if (s->timer89 > 2) out->ran = 1;                              /* 0x75666 +0x9E */
    s->target_x = s->pos.x;                                        /* 0x75679 rec+0x4D := pos */
    s->target_z = s->pos.z;
    s->cam_x = s->pos.x;                                           /* 0x7568a 0x15774C := pos */
    s->cam_y = s->pos.y;
    s->cam_z = s->pos.z;
    s->cam_z += (s->side == 0) ? 0x20 : -0x20;                     /* 0x7569f/0x756aa */
    {
      int32_t face = 0;                                            /* 0x756d0 face(-pos.x,-pos.z) */
      (void)fifa96_entity_face(-(int32_t)(int16_t)s->pos.x,
                               -(int32_t)(int16_t)s->pos.z, &face, &s->sector);
    }
    out->event = 0x27;                                             /* 0x756e8 */
    if (s->timer89 < 0x3C) goto claim_exit;                        /* 0x756f7 */
    s->reset_x = 0;                                                /* 0x75702 [0x10F328]=(0,0,0) */
    s->reset_y = 0;
    s->reset_z = 0;
    if (s->has_slot != 0) {                                        /* 0x7570f */
      stage = 3;
    } else {
      uint16_t roll = 0;                                           /* 0x75715 FUN_00092AC8 */
      int bit = 0;
      if (s->rng != NULL && fifa96_rng_step(s->rng, &roll) == FIFA96_OK) bit = roll & 1;
      if (bit == 0) {                                              /* 0x7571c JZ 0x75795 */
        stage = 3;
      } else {
        int32_t face = 0;                                          /* 0x7573c */
        (void)fifa96_entity_face(-(int32_t)(int16_t)s->pos.x,
                                 -(int32_t)(int16_t)s->pos.z, &face, &s->sector);
        out->event = 0x32;                                         /* 0x75754 */
        s->timer89 = 0;                                            /* 0x75762 */
        s->stage92 = 2;                                            /* 0x7576a */
        stage = 2;
      }
    }
  }
  if (stage == 2) {                                                /* 0x75770 */
    if (s->row44 == 0) goto claim_exit;                            /* 0x75777 */
    s->timer89 = 0;                                                /* 0x75783 */
    s->stage92 = 3;                                                /* 0x7578f */
    stage = 3;
  }
  if (stage == 3) {                                                /* 0x75795 */
    s->stage92 = 3;                                                /* 0x7579a */
    s->timer7b = 2;                                                /* 0x757a7 */
    if (s->has_slot == 0) {
      /* 0x757be JZ 0x75992: with no slot the native skips the whole 0x758cd
       * block (and the slot-edge arms) and enters the out-of-line ladder,
       * regardless of `+0x9B`. */
      out->slot_fill = 1;                                          /* 0x757b1 CALL 0x744D4 */
      if (s->timer89 > 0x78) {                                     /* 0x75999 */
        flag8 = 1;                                                 /* 0x7599b [EBP-8]=1 -> 0x75b75 */
      } else if ((uint16_t)s->gauge < 0x90u) {                     /* 0x759b1 */
        s->target_x = s->pos.x;                                    /* 0x759c2 */
        s->target_z = s->pos.z;
        s->target_z += (s->side == 0) ? 0x10 : -0x10;              /* 0x759d0/0x759d7 */
      }
    } else {
      uint8_t edge = s->slot_edge;
      if ((edge & 0x10u) != 0u) {                                  /* 0x757d5 */
        flag8 = 1;
        out->ui |= 0x10u;                                          /* 0x36200/0x361a4 */
      } else if ((edge & 0x40u) != 0u) {                           /* 0x757fc */
        out->ui |= 0x40u;
        flag8 = 1;
      } else if ((edge & 0x20u) != 0u) {                           /* 0x75820 */
        s->latch_157ab2 = (uint8_t)(s->latch_157ab2 == 0);         /* 0x7582e SETZ */
        out->ui |= 0x20u;
        if (s->latch_157ab2 != 0) {
          out->ui |= 0x80u;                                        /* 0x4C380 + 0x36200/0x361a4 */
        } else {
          out->snap = 1;                                           /* 0x75850 0x79B1C */
          s->reset_x = 0;                                          /* 0x75879 [0x10F328] */
          s->reset_y = 0;
          s->reset_z = 0;
          out->ui |= 0x100u;                                       /* 0x4C320/0x361b0 */
        }
      }
      /* 0x758cd (slot path only): the latch ladder. */
      if (s->latch_157ab2 == 0) {                                  /* 0x758d4 */
        out->ui |= 0x200u;                                         /* 0x75980 0x4C31C(1, slot) */
      } else {
        s->target_x = s->pos.x;                                    /* 0x758e8 rec+0x4D := pos */
        s->target_z = s->pos.z;
        if ((uint16_t)s->gauge < 0x90u) {                          /* 0x758f6 */
          s->target_x += (int32_t)s->slot_dir_x << 4;              /* 0x75907 */
          s->target_z += (int32_t)s->slot_dir_z << 4;              /* 0x7591b */
        }
        if (s->has_ball != 0) {                                    /* 0x75930 hold-follow */
          s->cam_x = s->pos.x + ((int32_t)s->slot_dir_x << 5);     /* 0x75948 */
          s->cam_z = s->pos.z + ((int32_t)s->slot_dir_z << 5);     /* 0x7595c */
          out->place = 1;                                          /* 0x75976 CALL 0x700F4 */
          out->place_x = s->cam_x;
          out->place_y = s->cam_y;
          out->place_z = s->cam_z;
        }
        /* `+0x9B == 0` jumps straight to 0x759ea (0x75930 JZ): no timer/gauge
         * ladder on the slot path. */
      }
    }
    if (flag8 != 0) {                                              /* 0x759ef -> 0x75b75 */
      s->timer89 = 0;
      s->stage92 = 4;
      stage = 4;                                                   /* falls into stage 4 */
    } else {
      s->cam_x = s->pos.x;                                         /* 0x759fd 0x15774C := pos */
      s->cam_y = s->pos.y;
      s->cam_z = s->pos.z;
      s->cam_y = 0x38;                                             /* 0x75a0f */
      if (s->vel71_nonzero == 0) {                                 /* 0x75a18 */
        s->cam_z += (s->side == 0) ? 0x10 : -0x10;                 /* 0x75a25/0x75a2c */
      } else {
        s->cam_x = s->pos.x + (int32_t)s->offset_x * 0x10;         /* 0x75a39 sector tables */
        s->cam_z = s->pos.z + (int32_t)s->offset_z * 0x10;
      }
      if (s->has_ball != 0) out->guard = 1;                        /* 0x75a79 CALL 0x74CDC */
      if ((uint16_t)s->gauge > 0xF0u) {                            /* 0x75a87 */
        s->target_x = s->pos.x;                                    /* 0x75b10 */
        s->target_z = s->pos.z;
        if (s->has_slot != 0) {
          s->cam_x = s->pos.x + ((int32_t)s->offset_x << 6);
          s->cam_z = s->pos.z + ((int32_t)s->offset_z << 6);
        }
      }
      if (s->has_ball != 0) {                                      /* 0x75b29 -> 0x760df */
        out->held_exit = 1;
        goto claim_exit;
      }
      out->reset = 1;                                              /* 0x75b36 CALL 0x7DAB4 */
      out->situation_0b = 1;                                       /* 0x75b58 */
      out->install = 5;                                            /* 0x75b67 */
      return FIFA96_OK;                                            /* native RET (no gauge) */
    }
  }
  if (stage == 4) {                                                /* 0x75b90 */
    s->target_x = s->pos.x;                                        /* 0x75b9c rec+0x4D := pos */
    s->target_z = s->pos.z;
    if (s->has_slot == 0) {
      out->clear_vec = 1;                                          /* 0x75c36 CALL 0x74E2C */
      if (s->rng != NULL) {
        uint16_t a = 0;
        uint16_t b = 0;
        if (fifa96_rng_step(s->rng, &a) == FIFA96_OK &&
            fifa96_rng_step(s->rng, &b) == FIFA96_OK) {
          fifa96_keeper_vec v;
          if (fifa96_keeper_clear_vector(a, b, s->range_attr, s->side, &v) == FIFA96_OK) {
            s->vec_band = v.distance;
            s->vec_dx = v.dx;
            s->vec_dz = v.dz;
          }
        }
      } else {
        s->vec_band = 0;
        s->vec_dx = 0;
        s->vec_dz = 0;
      }
    } else if (s->latch_157ab2 != 0) {
      out->vector_build = 1;                                       /* 0x75c0d CALL 0x7B878 */
    } else {
      fifa96_keeper_point to;                                      /* 0x75c2a 0x8DCD4(pos, 0x157A77) */
      to.x = s->reset_x;
      to.y = s->reset_y;
      to.z = s->reset_z;
      keeper_stage_vec(&s->pos, &to, &s->vec_band, &s->vec_dx, &s->vec_dz);
    }
    if ((uint16_t)s->vec_band < 0x5A0u) {                          /* 0x75c43 */
      fifa96_keeper_point local;
      fifa96_keeper_point near_pos;
      int16_t best = 0;
      int idx;
      local.x = (int32_t)(int16_t)((uint16_t)s->pos.x + (uint16_t)s->vec_dx);
      local.y = 0;
      local.z = (int32_t)(int16_t)((uint16_t)s->pos.z + (uint16_t)s->vec_dz);
      idx = fifa96_entity_find_nearest(s->mates, s->mate_count, s->skip_index,
                                       (int16_t)local.x, (int16_t)local.z, &best);
      if (idx >= 0 && s->mates != NULL) {                          /* 0x75c86 + 0x8DCD4 */
        near_pos.x = (int32_t)s->mates[idx].x;
        near_pos.y = 0;
        near_pos.z = (int32_t)s->mates[idx].y;
        keeper_stage_vec(&s->pos, &near_pos, &s->vec_band, &s->vec_dx, &s->vec_dz);
      }
    }
    if ((uint16_t)s->vec_band < 0x5A0u) out->event = 0x44;         /* 0x75ca6 */
    else if ((uint16_t)s->vec_band < 0x780u) out->event = 0x2F;
    else out->event = 0x45;
    {
      int32_t face = 0;                                            /* 0x75cdc face(vec_dx, vec_dz) */
      (void)fifa96_entity_face(s->vec_dx, s->vec_dz, &face, &s->sector);
    }
    s->timer89 = 0;                                                /* 0x75d05 */
    s->stage92 = 5;                                                /* 0x75d11 */
    stage = 5;
  }
  if (stage == 5) {                                                /* 0x75d17 */
    if (s->anim_row == 0x45u) {                                    /* 0x75d24 */
      if ((int8_t)s->frame < 3) goto claim_exit;                   /* 0x75d39 MOVSX */
      s->cam_x = s->pos.x;                                         /* 0x75d52 */
      s->cam_y = s->pos.y;
      s->cam_z = s->pos.z;
      s->cam_y = 0x50;                                             /* 0x75d69 */
      s->cam_x = s->pos.x + ((int32_t)s->offset_x << 6);           /* 0x75d92 */
      s->cam_z = s->pos.z + ((int32_t)s->offset_z << 6);
      out->scenario = 1;                                           /* 0x92820(5)+0x71C94 */
      out->controlled = 1;                                         /* 0x75dcc */
      s->has_ball = 0;                                             /* 0x75dd1 RELEASE */
      out->released = 1;
    }
    s->timer89 = 0;                                                /* 0x75de1 */
    s->stage92 = 6;                                                /* 0x75ded */
    stage = 6;
  }
  if (stage == 6) {                                                /* 0x75df3 */
    int gate = 0;                                                  /* [EBP-4], zeroed at 0x75525 */
    if (s->anim_row == 0x44u || s->anim_row == 0x2Fu) {            /* 0x75e00/0x75e1b */
      gate = 5;
      out->flag_write = 1;
      out->flag_157820 = 1;
      out->flag_157822 = 1;
    } else if (s->anim_row == 0x45u) {                             /* 0x75e38 */
      gate = 3;
      out->flag_write = 1;
      out->flag_157820 = 0;
      out->flag_157822 = 0;
    }
    if ((int32_t)(int8_t)s->frame < gate) goto claim_exit;         /* 0x75e61 MOVSX JL */
    s->timer89 = 0;                                                /* 0x75e70 */
    s->stage92 = 7;                                                /* 0x75e7c */
    stage = 7;
  }
  if (stage == 7) {                                                /* 0x75e82 */
    uint8_t staging_event;
    if (s->anim_row == 0x44u) {                                    /* 0x75e8f */
      s->cam_x = s->pos.x;                                         /* 0x75ea6 */
      s->cam_y = s->pos.y;
      s->cam_z = s->pos.z;
      s->cam_y = 0x10;                                             /* 0x75eea */
      s->cam_x = s->pos.x + (int32_t)s->offset_x * 5 * 0x10;       /* 0x75ec4 (x*4+x)<<4 */
      s->cam_z = s->pos.z + (int32_t)s->offset_z * 5 * 0x10;       /* 0x75ef4 */
      staging_event = 0x0B;                                        /* 0x75f14 */
      out->scenario = 1;                                           /* 0x92820(4) */
      s->vec_dx = (int16_t)(s->vec_dx - keeper_trunc_shift(s->vec_dx, 2)); /* 0x75f2c */
      s->vec_dz = (int16_t)(s->vec_dz - keeper_trunc_shift(s->vec_dz, 2)); /* 0x75f45 */
    } else if (s->anim_row == 0x2Fu) {                             /* 0x75f56 */
      s->cam_x = s->pos.x;                                         /* 0x75f66 */
      s->cam_y = s->pos.y;
      s->cam_z = s->pos.z;
      s->cam_y = 0x80;                                             /* 0x75f86 */
      s->cam_x = s->pos.x + ((int32_t)s->offset_x << 6);           /* 0x75f9e */
      s->cam_z = s->pos.z + ((int32_t)s->offset_z << 6);
      staging_event = 0x0C;                                        /* 0x75fca */
      out->scenario = 1;                                           /* 0x92820(4, band>>5+0x80) */
    } else {
      staging_event = 0x01;                                        /* 0x75fdd */
      out->scenario = 1;                                           /* 0x92820(5, band>>2) */
    }
    /* 0x76012 the staged vector's face, then the 0x7A490 staging with
     * EBX = the sector the face returned and ECX = the event byte. */
    {
      int32_t face = 0;
      (void)fifa96_entity_face(s->vec_dx, s->vec_dz, &face, &s->sector);
    }
    out->ball_stage = 1;                                           /* 0x76030 CALL 0x7A490 */
    out->ball_event = staging_event;
    if (s->has_slot != 0) out->handoff = 1;                        /* 0x76049 + 0x786a0 */
    out->situation_0b = 1;                                         /* 0x76072 (CL=0) */
    s->latch_157ab2 = 0;                                           /* 0x76077 */
    s->timer89 = 0;                                                /* 0x7608b */
    s->stage92 = 8;                                                /* 0x76097 */
    stage = 8;
  }
  if (stage == 8) {                                                /* 0x7609d */
    out->snap = 1;                                                 /* 0x79B1C */
    if (s->row44 == 0) goto claim_exit;                            /* 0x760ac */
    s->timer89 = 0;                                                /* 0x760b4 */
    s->stage92 = 9;                                                /* 0x760c0 */
    stage = 9;
  }
  if (stage == 9) {                                                /* 0x760c6 */
    out->snap = 1;                                                 /* 0x79B1C */
    if (s->timer89 > 0x3C) out->reset = 1;                         /* 0x760d8 CALL 0x7DAB4 */
    /* falls to the common exit */
  }
claim_exit:
  keeper_claim_exit(s, out);
  return FIFA96_OK;
}

/* ===== FU-151 §Port contract item 2 — row-1D close-down machine =========== */

static void keeper_closedown_tail(fifa96_keeper_closedown *s,
                                  fifa96_keeper_closedown_out *out) {
  if (s->latch_157ab2 != 0 && (int8_t)s->stage92 > 0) out->tail_hold = 1; /* 0x754c3 */
}

fifa96_err_t fifa96_keeper_closedown_step(fifa96_keeper_closedown *s,
                                          fifa96_keeper_closedown_out *out) {
  int stage;
  int flag4 = 0;
  if (!s || !out) return -FIFA96_ERR_INVALID;
  memset(out, 0, sizeof *out);
  stage = (int)s->stage92;
  if (stage < 3) {                                                 /* 0x74f1a..0x74f27 */
    out->controlled = 1;
    out->helper = 1;                                               /* CALL 0x7876C */
  }
  s->timer89 += (int32_t)s->delta;                                 /* 0x74f2c */
  stage = (int)s->stage92;
  if (stage > 4) goto closedown_tail;                              /* 0x74f4d JA */
  if (stage == 0) {                                                /* 0x74f60 */
    if (s->row44 == 0) return FIFA96_OK;                           /* 0x74f64 -> RET */
    if (s->timer89 < 0x1E && s->session_gate == 0) goto closedown_tail; /* 0x74f74/0x74f7d */
    out->place = 1;                                                /* 0x74f99 CALL 0x700F4 */
    out->place_x = s->cam_x;
    out->place_y = s->cam_y;
    out->place_z = s->cam_z;
    s->pos.x = s->cam_x;                                           /* 0x74fa9 pos := 0x15774C */
    s->pos.y = s->cam_y;
    s->pos.z = s->cam_z;
    s->pos.z += (int32_t)s->cam_off_z;                             /* 0x74fc9 */
    out->commit = 1;                                               /* 0x74fe3 CALL 0x79B6C */
    out->event = 0x26;                                             /* 0x75000 */
    s->latch_157ab2 = 1;                                           /* 0x7501f */
    s->timer89 = 0;                                                /* 0x75033 */
    s->stage92 = 1;                                                /* 0x7503f */
    stage = 1;
  }
  if (stage == 1) {                                                /* 0x75045 */
    if (s->session_gate == 0) {
      if (s->timer89 > 2) out->ran = 1;                            /* 0x7508e +0x9E */
    } else {
      if (s->timer89 < 0x1E) goto closedown_tail;                  /* 0x75058 */
      if ((s->timer89 - (int32_t)s->delta) < 0x1E)
        out->ui |= 0x10u;                                          /* 0x974DC(0x1E) audio */
    }
    out->place = 1;                                                /* 0x750b5 CALL 0x700F4 */
    out->place_x = s->cam_x;
    out->place_y = s->cam_y;
    out->place_z = s->cam_z;
    s->pos.x = s->cam_x;                                           /* 0x750c0 pos := 0x15774C */
    s->pos.y = s->cam_y;
    s->pos.z = s->cam_z + (int32_t)s->cam_off_z;                   /* 0x750dd */
    out->commit = 1;                                               /* 0x750ed CALL 0x79B6C */
    if (s->has_slot == 0) {
      out->slot_fill = 1;                                          /* 0x750fb CALL 0x744D4 */
      if (s->timer89 > 0xB4) flag4 = 1;                            /* 0x75216 -> 0x75222 */
    } else {
      if ((s->slot_edge & 0x50u) != 0u) {                          /* 0x75116 byte[slot+6]&0x50 */
        out->ui |= 0x20u;                                          /* 0x36200/0x361a4 */
        flag4 = 1;                                                 /* [EBP-4]=1 (0x7512f) */
        /* JMP 0x751fb */
      } else if ((s->slot_pressed & 0x20u) != 0u) {                /* 0x75137 byte[slot+4]&0x20 */
        s->latch_157ab2 = (uint8_t)(s->latch_157ab2 == 0);         /* 0x75152 SETZ */
        if (s->latch_157ab2 != 0) {
          out->ui |= 0x40u;                                        /* 0x4C380/0x36200/0x361a4 */
        } else {
          out->snap = 1;                                           /* 0x75174 0x79B1C */
          s->reset_x = 0;                                          /* 0x7519e [0x10F328] */
          s->reset_y = 0;
          s->reset_z = 0;
          out->ui |= 0x80u;                                        /* 0x4C320/0x361b0 */
        }
        /* 0x751b9: the latched-edge block runs only after a 0x20 edge. */
        if (s->timer89 > 0x4B0) {                                  /* 0x751bc */
          if ((s->timer89 - (int32_t)s->delta) <= 0x4B0) out->sink_4b0 = 1; /* 0x8F188(0xA4,4,0) */
        }
        out->ui |= 0x100u;                                         /* 0x751f6 0x4C31C(0) */
      }
      /* 0x751fb (all slot sub-paths): the latch gate. */
      if (s->latch_157ab2 == 0) out->ui |= 0x200u;                 /* 0x7520f 0x4C31C(1, slot) */
    }
    if (flag4 == 0) goto closedown_tail;                           /* 0x7522a */
    s->timer89 = 0;                                                /* 0x75239 */
    s->stage92 = 2;                                                /* 0x75245 */
    stage = 2;
  }
  if (stage == 2) {                                                /* 0x7524b */
    s->target_x = s->cam_x;                                        /* 0x75259 rec+0x4D := 0x15774C */
    s->target_z = s->cam_z;
    if (s->lane > 0x40 && s->timer89 > 0xB4) {                     /* 0x75265/0x75274 */
      s->latch_157ab2 = 0;                                         /* 0x754ac */
      out->reset = 1;                                              /* 0x754b7 CALL 0x7DAB4 */
      goto closedown_tail;                                         /* 0x754bc */
    }
    s->timer89 = 0;                                                /* 0x75288 */
    s->stage92 = 3;                                                /* 0x75294 */
    stage = 3;
  }
  if (stage == 3) {                                                /* 0x7529a */
    out->scenario = 1;                                             /* 0x92820(rec,6) + [0x158743]=1 */
    if (s->has_slot != 0) {                                        /* 0x752c4 */
      fifa96_keeper_point to;
      to.x = s->reset_x;
      to.y = s->reset_y;
      to.z = s->reset_z;
      keeper_stage_vec(&s->pos, &to, &s->vec.distance, &s->vec.dx, &s->vec.dz);
    } else {
      out->clear_vec = 1;                                          /* 0x752d0 CALL 0x74E2C */
      if (s->rng != NULL) {
        uint16_t a = 0;
        uint16_t b = 0;
        if (fifa96_rng_step(s->rng, &a) == FIFA96_OK &&
            fifa96_rng_step(s->rng, &b) == FIFA96_OK) {
          fifa96_keeper_vec v;
          if (fifa96_keeper_clear_vector(a, b, s->range_attr, s->side, &v) == FIFA96_OK)
            s->vec = v;
        }
      } else {
        s->vec.distance = 0;
        s->vec.dx = 0;
        s->vec.dz = 0;
      }
    }
    if ((uint16_t)s->vec.distance < 0x5A0u) {                      /* 0x752dd */
      fifa96_keeper_point local;
      fifa96_keeper_point near_pos;
      int16_t best = 0;
      int idx;
      local.x = (int32_t)(int16_t)((uint16_t)s->pos.x + (uint16_t)s->vec.dx);
      local.y = 0;
      local.z = (int32_t)(int16_t)((uint16_t)s->pos.z + (uint16_t)s->vec.dz);
      idx = fifa96_entity_find_nearest(s->mates, s->mate_count, s->skip_index,
                                       (int16_t)local.x, (int16_t)local.z, &best);
      if (idx >= 0 && s->mates != NULL) {                          /* 0x7532a + 0x8DCD4 */
        near_pos.x = (int32_t)s->mates[idx].x;
        near_pos.y = 0;
        near_pos.z = (int32_t)s->mates[idx].y;
        keeper_stage_vec(&s->pos, &near_pos, &s->vec.distance, &s->vec.dx, &s->vec.dz);
      }
    }
    if ((uint16_t)s->vec.distance < 0x3C0u) {                      /* 0x7534a */
      int32_t angle = 0;
      (void)fifa96_entity_angle(s->vec.dx, s->vec.dz, &angle);     /* 0x8DD70 */
      s->vec.distance = 0x3C0;                                     /* 0x75375 */
      s->vec.dx = (int16_t)(((int64_t)0x3C0 * fifa96_entity_sine(angle) + 0x8000) >> 16);
      s->vec.dz = (int16_t)(((int64_t)0x3C0 * fifa96_entity_sine(angle + 0x100) + 0x8000) >> 16);
    }
    if ((uint16_t)s->vec.distance < 0x5A0u) {                      /* 0x753f3 band>>3, event 0x30 */
      out->ball_stage = 1;
      out->clearance_event = 0x30;
    } else {
      out->ball_stage = 1;                                         /* 0x7541c band>>4, event 0x31 */
      out->clearance_event = 0x31;
      out->ring = 1;                                               /* 0x8F188(0x22, rec, 4) */
    }
    if (s->has_slot != 0) out->handoff = 1;                        /* 0x75447 + 0x786a0 */
    out->situation_0b = 1;                                         /* 0x7546e */
    s->latch_157ab2 = 0;                                           /* 0x75475 */
    s->timer89 = 0;                                                /* 0x75489 */
    s->stage92 = 4;                                                /* 0x75495 */
    stage = 4;
  }
  if (stage == 4) {                                                /* 0x7549b */
    out->snap = 1;                                                 /* 0x79B1C */
    if (s->row44 == 0) goto closedown_tail;                        /* 0x754aa */
    s->latch_157ab2 = 0;                                           /* 0x754b1 */
    out->reset = 1;                                                /* 0x754b7 CALL 0x7DAB4 */
    /* falls to the tail */
  }
closedown_tail:
  keeper_closedown_tail(s, out);
  return FIFA96_OK;
}
