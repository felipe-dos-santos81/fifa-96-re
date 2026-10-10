#include "fifa96_loader/fifa96_outfield.h"

#include <string.h>

#include "fifa96_loader/fifa96_action_handlers.h"
#include "fifa96_loader/fifa96_entity_update.h"
#include "fifa96_loader/fifa96_rng.h"

static const fifa96_outfield_rule pressed_0[] = {
    {0x07FF, 0x0060, 0x7CE38},
    {0x07FF, 0x0040, 0x7CF20},
    {0x07FF, 0x0080, 0x7CF20},
    {0, 0, 0},
};

static const fifa96_outfield_rule pressed_1[] = {
    {0x07FF, 0x0080, 0x7CF20},
    {0, 0, 0},
};

static const fifa96_outfield_rule pressed_2[] = {
    {0x07FF, 0x0010, 0x7D174},
    {0x07FF, 0x0040, 0x7D174},
    {0x07FF, 0x0080, 0x7CF20},
    {0, 0, 0},
};

static const fifa96_outfield_rule pressed_3[] = {
    {0x07FF, 0x0010, 0x7CFD0},
    {0x07FF, 0x0040, 0x7CD60},
    {0x07FF, 0x0080, 0x7CF20},
    {0, 0, 0},
};

static const fifa96_outfield_rule pressed_4[] = {
    {0x07FF, 0x0010, 0x7D054},
    {0x07FF, 0x0040, 0x7D08C},
    {0x07FF, 0x0080, 0x7CF20},
    {0, 0, 0},
};

static const fifa96_outfield_rule released_0[] = {
    {0x07FF, 0x0030, 0x7CEB0},
    {0x07FF, 0x0020, 0x7D1D4},
    {0x07FF, 0x0020, 0x7CEB0},
    {0x07FF, 0x0010, 0x7CF54},
    {0x07FF, 0x0040, 0x7D0C4},
    {0x07FF, 0x0040, 0x7CD60},
    {0, 0, 0},
};

static const fifa96_outfield_rule released_1[] = {
    {0x07FF, 0x0050, 0x7D010},
    {0x07FF, 0x0030, 0x7D110},
    {0x07FF, 0x0010, 0x7D110},
    {0x07FF, 0x0020, 0x7D110},
    {0x07FF, 0x0040, 0x7D110},
    {0x07FF, 0x0060, 0x7D110},
    {0, 0, 0},
};

static const fifa96_outfield_rule released_2[] = {
    {0x07FF, 0x0020, 0x7D1D4},
    {0, 0, 0},
};

static const fifa96_outfield_rule released_3[] = {
    {0x07FF, 0x0020, 0x7D1D4},
    {0, 0, 0},
};

static const fifa96_outfield_rule released_4[] = {
    {0x07FF, 0x0020, 0x7D1D4},
    {0, 0, 0},
};

static const fifa96_outfield_rule *const pressed_tables[FIFA96_OUTFIELD_CODE_COUNT] = {
    pressed_0, pressed_1, pressed_2, pressed_3, pressed_4,
};

static const fifa96_outfield_rule *const released_tables[FIFA96_OUTFIELD_CODE_COUNT] = {
    released_0, released_1, released_2, released_3, released_4,
};

const fifa96_outfield_rule *fifa96_outfield_pressed_rules(uint8_t code) {
  if (code >= FIFA96_OUTFIELD_CODE_COUNT) return NULL;
  return pressed_tables[code];
}

const fifa96_outfield_rule *fifa96_outfield_released_rules(uint8_t code) {
  if (code >= FIFA96_OUTFIELD_CODE_COUNT) return NULL;
  return released_tables[code];
}

int fifa96_outfield_rule_match(const fifa96_outfield_rule *rule, uint16_t word) {
  if (!rule) return -FIFA96_ERR_INVALID;
  return (rule->mask & word) == rule->want;
}

int fifa96_outfield_rules_run(const fifa96_outfield_rule *rules, uint16_t word,
                              fifa96_outfield_rule_fn call, void *context, uint32_t *handler) {
  if (!rules || !call || !handler) return -FIFA96_ERR_INVALID;
  for (;;) {
    if (fifa96_outfield_rule_match(rules, word)) {
      if (rules->handler == 0) return 0;
      *handler = rules->handler;
      if (call(rules->handler, context) != 0) return 1;
    }
    rules++;
  }
}

int fifa96_outfield_dispatch_code(const fifa96_outfield_edge *edge, uint8_t *code) {
  uint8_t selected;
  if (!edge || !code) return -FIFA96_ERR_INVALID;
  if (edge->pressed == 0 && edge->released == 0) return 0;
  if (edge->high_577ee_ge_50) {
    selected = 2;
  } else if (edge->type == 5) {
    selected = 1;
  } else if (edge->tracked && edge->user_absent_or_self) {
    selected = 3;
  } else if (edge->chaser) {
    selected = 4;
  } else {
    selected = 0;
  }
  *code = selected;
  return 1;
}

int fifa96_outfield_forced_action(const fifa96_outfield_forced_state *state, uint8_t current,
                                  uint8_t *next) {
  uint8_t code;
  if (!state || !next) return -FIFA96_ERR_INVALID;
  if (state->is_team_controlled) {
    if (state->opponent_has_ball) {
      code = 6;
    } else if (state->type_5) {
      return 0;
    } else {
      code = 4;
    }
  } else if (state->is_team_second) {
    if (state->opponent_has_ball) {
      code = 6;
    } else if (state->controlled_has_ball) {
      code = 3;
    } else {
      code = 4;
    }
  } else {
    code = 3;
  }
  if (code == current) return 0;
  *next = code;
  return 1;
}

int fifa96_outfield_chase_action(const fifa96_outfield_chase_state *state, uint8_t current,
                                 uint8_t *next) {
  if (!state || !next) return -FIFA96_ERR_INVALID;
  if (state->phase != 2 || state->type_gate == 0) return 0;
  if (!state->not_team_controlled || !state->not_team_second) return 0;
  if (state->distance >= 0x50 || state->camera >= 0x30) return 0;
  if (!state->user_present || !state->sides_differ) return 0;
  if (!state->unbound || state->timer != 0 || !state->third_zero) return 0;
  if (current == 8) return 0;
  *next = 8;
  return 1;
}

/* The per-type decision gate flat `0x110680[type]` (first-hand read this
 * task: 32 bytes {3,0,0,3,3,3,3,2,0,...,2,2,2,2,0,0,0,0,0,3,...}). The
 * handlers test the raw byte with `& 1` (most) or `& 3` (the `0x7CFD0`/
 * `0x7D054`/`0x7D08C` selection writes), so the raw bits are exposed and the
 * `&1` convenience stays. The native indexes unbounded; types beyond 0x19 are
 * not reachable from a real record and the adjacent image bytes are not part
 * of this table, so they return 0. */
uint8_t fifa96_outfield_type_bits(uint8_t type) {
  static const uint8_t table[26] = {
      3, 0, 0, 3, 3, 3, 3, 2, 0, 0, 0, 0, 0,
      0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 3,
  };
  if (type >= 26u) return 0;
  return table[type];
}

static uint8_t outfield_type_gate(uint8_t type) {
  return (uint8_t)(fifa96_outfield_type_bits(type) & 1u);
}

int fifa96_outfield_chase_gate(const fifa96_outfield_chase_state *state, uint8_t type,
                               uint8_t current, uint8_t *next) {
  fifa96_outfield_chase_state s;
  if (!state || !next) return -FIFA96_ERR_INVALID;
  s = *state;
  s.type_gate = outfield_type_gate(type);
  return fifa96_outfield_chase_action(&s, current, next);
}

int fifa96_outfield_input_row(const fifa96_outfield_input_state *state,
                              fifa96_outfield_rule_fn call, void *context,
                              fifa96_outfield_input_out *out) {
  uint16_t word;
  uint8_t next = 0;
  if (!state || !call || !out) return -FIFA96_ERR_INVALID;
  out->direct_arm = 0;
  out->no_edge = 0;
  out->scan_ran = 0;
  out->scan_code = 0;
  out->scan_stopped = 0;
  out->no_edge_arm = 0;
  out->forced = 0;
  out->forced_code = 0;
  out->chase = 0;
  if (state->has_slot != 0) {
    if (state->flag_157ab0 != 0) {
      /* 0x7CAC4..0x7CB08: the pre-gate to the 0x7D1D4 selection handler. */
      if (state->is_1578ac == 0 && state->type == 3u &&
          (state->released & 0x20u) != 0) {
        out->direct_arm = 1;
      }
    } else if (state->pressed == 0 && state->released == 0) {
      /* 0x7CB10..0x7CB1C -> the no-edge arm 0x7CC13..0x7CC7D. */
      out->no_edge = 1;
      if ((state->slot_word10 & 0xF0u) != 0 && state->phase == 2u) {
        int16_t d = (int16_t)(state->lane >> 16);
        if (d >= 0x90) {
          /* 0x7CC41 side filter */
          if (!(state->user_present != 0 && state->user_side == state->side) &&
              (state->slot_word10 & 0xC0u) != 0)
            out->no_edge_arm = 1;
        } else if (d > 0x30) {
          out->no_edge_arm = 1;   /* 0x7CC70 */
        } else {
          if (!(state->user_present != 0 && state->user_side == state->side) &&
              (state->slot_word10 & 0xC0u) != 0)
            out->no_edge_arm = 1;
        }
      }
    } else {
      fifa96_outfield_edge edge;
      uint32_t handler = 0;
      edge.pressed = state->pressed;
      edge.released = state->released;
      edge.type = state->type;
      edge.high_577ee_ge_50 = state->high_577ee_ge_50;
      edge.tracked = state->tracked;
      edge.user_absent_or_self = state->user_absent_or_self;
      edge.chaser = state->chaser;
      if (fifa96_outfield_dispatch_code(&edge, &out->scan_code) != 1)
        return -FIFA96_ERR_INVALID;
      out->scan_ran = 1;
      word = (uint16_t)(state->pressed & 0xFF0u);
      if (word != 0) {
        if (fifa96_outfield_rules_run(fifa96_outfield_pressed_rules(out->scan_code), word,
                                      call, context, &handler) == 1)
          out->scan_stopped = 1;
      } else {
        word = (uint16_t)(state->released & 0xFF0u);
        if (word != 0) {
          if (fifa96_outfield_rules_run(fifa96_outfield_released_rules(out->scan_code), word,
                                        call, context, &handler) == 1)
            out->scan_stopped = 1;
        }
      }
    }
  }
  /* 0x7CC82..0x7CD24: the phase-2 forced decision and the code-8 chase gate,
   * both behind the per-type gate. The two flags are computed for the caller;
   * the engine seam does not apply their record writes yet (legs L4.1/L4.2),
   * so they are inert outputs on the reachable path. */
  if (state->phase == 2u && outfield_type_gate(state->type) != 0) {
    if (fifa96_outfield_forced_action(&state->forced, state->current_code, &next) == 1) {
      out->forced = 1;
      out->forced_code = next;
    }
    if (fifa96_outfield_chase_gate(&state->chase, state->type, state->current_code,
                                   &next) == 1)
      out->chase = 1;
  }
  return FIFA96_OK;
}

/* `FUN_0008DE8C` (0x8DE8C..0x8DEFF, first-hand): the nearest-record search
 * over the own team block. EAX = the origin triple (x at +0, z at +8), EDX =
 * team block, BX = the skip index, ECX = a distance out word (the row-04
 * carriers pass 0). Each of the team's records is skipped when its index
 * equals the sign-extended skip or its +0x98/+0x9A bytes are set; the metric
 * is `fifa96_entity_distance(origin.x - rec.x, origin.z - rec.z)` as 16-bit
 * words, strictly smaller wins (unsigned `JNC` keeps the earlier record on a
 * tie), the initial best is 0xFFFF. Returns the best index or NONE. The
 * tested twin is `fifa96_entity_find_nearest` (same native function). */
static int32_t row04_nearest(const fifa96_outfield_row04_state *s, uint32_t skip) {
  uint16_t best = 0xFFFFu;
  int32_t best_index = FIFA96_OUTFIELD_ROW04_NONE;
  uint32_t i;
  if (!s->mates) return FIFA96_OUTFIELD_ROW04_NONE;
  for (i = 0; i < s->mate_count; i++) {
    uint16_t d;
    if (i == (uint32_t)(uint16_t)skip) continue;                 /* 0x8DEAE */
    if (s->mates[i].skip_98 != 0u || s->mates[i].skip_9a != 0u) continue;
    d = (uint16_t)fifa96_entity_distance(                        /* 0x8DED7 */
        (int16_t)((uint16_t)s->vec5770_x - (uint16_t)s->mates[i].x),
        (int16_t)((uint16_t)s->vec5770_z - (uint16_t)s->mates[i].z));
    if (d < best) {                                              /* 0x8DEDF */
      best = d;
      best_index = (int32_t)i;
    }
  }
  return best_index;
}

/* `FUN_0008DDE0` (0x8DDE0..0x8DE26, first-hand): the ranked lane pick over
 * the opponent team block (EDX = 0 at the row-04 call site, so record 0 is
 * always skipped; +0x98/+0x9A excluded), smallest unsigned `+0x6B` word first
 * on ties (the initial best is 0xFFFF). Returns the picked index or NONE. */
static int32_t row04_ranked_pick(const fifa96_outfield_row04_state *s) {
  uint16_t best = 0xFFFFu;
  int32_t best_index = FIFA96_OUTFIELD_ROW04_NONE;
  uint32_t i;
  if (!s->opps) return FIFA96_OUTFIELD_ROW04_NONE;
  for (i = 0; i < s->opp_count; i++) {
    uint16_t lane;
    if (i == 0u) continue;                                       /* 0x8DDF4 */
    if (s->opps[i].skip_98 != 0u || s->opps[i].skip_9a != 0u) continue;
    lane = (uint16_t)s->opps[i].lane;
    if (lane < best) {                                           /* 0x8DE0F JNC */
      best = lane;
      best_index = (int32_t)i;
    }
  }
  return best_index;
}

/* The `0x8DCD4`/`0x8DC68` word-difference octagonal metric (distance only;
 * both native helpers were verified to the same formula, FU-142 A.2/K.2). */
static int32_t row04_metric(int32_t from_x, int32_t from_z, int32_t to_x, int32_t to_z) {
  return fifa96_entity_distance((int16_t)((uint16_t)to_x - (uint16_t)from_x),
                                (int16_t)((uint16_t)to_z - (uint16_t)from_z));
}

/* `FUN_0007E600` (`0x7E600..0x7E7C4`, 176 insns): the no-slot defender
 * decision, the same bounded model as the kick machine's `kick_decision`
 * (FU-139 §9/FU-142 K.1): phase 2, the flat `0x110680[byte +0x91] & 1` code
 * gate, the `[0x1577CA]` exclusion, lane <= 0x180, predictor y in
 * [0x20, 0x60], the `0x8DCD4` pos/predictor distance <= 0xF0 and <= lane,
 * the camera-x and side/pos_z gates, the `0x8DD70` angle inside +/-0x100
 * (side 0) or outside (side 1) and the `|angle - word[+0x7D]|` 0x100 gate.
 * The 0x71B9C(4) predictor triple is a caller input (OL-74). */
static int row04_defender_decision(const fifa96_outfield_row04_state *s) {
  int16_t dx;
  int16_t dz;
  int32_t distance;
  int32_t angle = 0;
  int32_t diff;
  if (s->phase != 2u) return 0;
  if (s->code >= 26u || (outfield_type_gate(s->code) & 1u) == 0u) return 0;
  if (s->is_ball_track != 0u) return 0;                          /* 0x7E62F */
  if (s->lane > 0x180) return 0;                                 /* 0x7E644 */
  if (s->predictor_y > 0x60 || s->predictor_y < 0x20) return 0;  /* 0x7E662 */
  dx = (int16_t)((uint16_t)s->predictor_x - (uint16_t)s->pos_x);
  dz = (int16_t)((uint16_t)s->predictor_z - (uint16_t)s->pos_z);
  distance = fifa96_entity_distance(dx, dz);
  if (distance > 0xF0) return 0;                                 /* 0x7E690 */
  if ((int16_t)distance > s->lane) return 0;                     /* 0x7E697 */
  if (s->pos_x < -0x1E0 && s->pos_x > s->camera_x) return 0;     /* 0x7E6B7 */
  if (s->pos_x > 0x1E0 && s->camera_x > s->pos_x) return 0;      /* 0x7E6C8 */
  if (s->side == 0u) {
    if ((int16_t)s->pos_z < 0x7B0) return 0;                     /* 0x7E6BF */
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

static void row04_add_install(fifa96_outfield_row04_out *out, uint8_t target, uint8_t code,
                              uint8_t staged, uint8_t invoke) {
  if (out->install_count >= FIFA96_OUTFIELD_ROW04_INSTALL_MAX) return;
  out->installs[out->install_count].target = target;
  out->installs[out->install_count].code = code;
  out->installs[out->install_count].staged = staged;
  out->installs[out->install_count].invoke = invoke;
  out->install_count++;
}

fifa96_err_t fifa96_outfield_row04_step(const fifa96_outfield_row04_state *state,
                                        fifa96_outfield_row04_out *out) {
  int32_t lane;
  int32_t target_set = 0;
  int32_t tx = 0;
  int32_t ty = 0;
  int32_t tz = 0;
  uint8_t ecx = 0;
  uint8_t ax_flag;
  int32_t other;
  if (!state || !out) return -FIFA96_ERR_INVALID;
  memset(out, 0, sizeof *out);
  out->team_target_index = FIFA96_OUTFIELD_ROW04_NONE;
  out->team_second_index = FIFA96_OUTFIELD_ROW04_NONE;
  out->timer89 = state->timer89;                       /* unchanged on early returns */
  out->ran = 1;                                        /* 0x7E7D3 */
  if (state->phase != 2u) {                            /* 0x7E7E2 */
    out->reset = 1;
    return FIFA96_OK;
  }
  if (state->timer81 != 0u) return FIFA96_OK;          /* 0x7E7F3 */
  lane = (int32_t)state->lane;

  if (state->is_carrier != 0u) {                       /* 0x7E80E */
    int32_t best = row04_nearest(state, (uint32_t)(uint16_t)(int16_t)state->active);
    out->team_target_set = 1;
    out->team_target_index = best;                     /* 0x7E82D */
    if (best == FIFA96_OUTFIELD_ROW04_NONE) return FIFA96_OK;
    if (outfield_type_gate(state->mates[best].code) == 0u) return FIFA96_OK; /* 0x7E84A */
    row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 4, 0, 1);     /* 0x7E85C */
    row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_SELF,
                      state->active != 0u ? 3u : 0x19u, 0, 1);               /* 0x7E883 */
    return FIFA96_OK;
  }

  if (state->active == 0u) {                           /* 0x7E888 */
    int32_t picked = state->opp_7c7_index;
    int16_t picked_lane;
    if (picked == FIFA96_OUTFIELD_ROW04_NONE) picked = row04_ranked_pick(state);
    if (picked != FIFA96_OUTFIELD_ROW04_NONE &&
        (uint32_t)picked >= state->opp_count) picked = FIFA96_OUTFIELD_ROW04_NONE;
    picked_lane = picked == FIFA96_OUTFIELD_ROW04_NONE ? 0 : state->opps[picked].lane;
    if ((int16_t)picked_lane > (int16_t)lane) goto main_targets;      /* 0x7E8B6 */
    row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 0x19, 0, 1); /* 0x7E8C8 */
    if (state->team_target_index == state->self_index) {              /* 0x7E8D3 */
      out->team_target_set = 1;
      out->team_target_index = row04_nearest(state, 0u);              /* 0x7E8E6 */
    } else if (state->team_second_index == state->self_index) {       /* 0x7E8FF */
      out->team_second_set = 1;
      out->team_second_index = row04_nearest(state, 0u);              /* 0x7E916 */
    }
    return FIFA96_OK;
  }

  if (state->team_second_index == state->self_index) { /* 0x7E939 */
    if (state->team_target_index == state->self_index) {
      out->team_second_set = 1;                        /* 0x7E94B */
      out->team_second_index = FIFA96_OUTFIELD_ROW04_NONE;
    } else if (state->team_target_index != FIFA96_OUTFIELD_ROW04_NONE) {
      int32_t ti = state->team_target_index;
      if (ti >= 0 && (uint32_t)ti < state->mate_count &&
          state->mates[ti].code == 4u &&                          /* 0x7E964 */
          (int16_t)state->mates[ti].lane <= (int16_t)lane) {      /* 0x7E971 */
        out->reset = 1;                                           /* 0x7E975 */
        return FIFA96_OK;
      }
    }
  }

main_targets:                                          /* 0x7E984 */
  other = (state->opp_target_index >= 0 &&
           (uint32_t)state->opp_target_index < state->opp_count)
              ? state->opp_target_index
              : FIFA96_OUTFIELD_ROW04_NONE;
  ax_flag = (state->is_ball_track != 0u && state->ball_height > 0x50) ? 1u : 0u; /* 0x7E9A0 */

  if (state->has_slot != 0u) {                         /* 0x7E9B7 */
    if ((state->slot_word10 & 0x20u) != 0u &&
        (int32_t)(int16_t)state->track_577f0 > 0x70) { /* 0x7E9D3 */
      tx = state->vec5770_x;                           /* 0x7E9E1 (0x157770) */
      ty = state->vec5770_y;
      tz = state->vec5770_z;
      target_set = 1;
      goto clamp;
    }
    if (ax_flag == 0u) {                               /* 0x7E9F1 */
      if (state->user_present == 0u || state->user_is_self != 0u) {
        if (lane < 0x60) ecx = 1u;                     /* 0x7EA0F */
      }
    }
  } else {
    ecx = 1u;                                          /* 0x7E9BB */
  }

  if (ax_flag != 0u) goto camera_arm;                  /* 0x7EA18 */
  {
    int32_t w = (int32_t)(int16_t)state->track_577f0;
    if (w > 0x70) {                                    /* 0x7EA21 */
      if ((int16_t)state->track_577fa < (int16_t)state->track_57800) { /* 0x7EA32 */
        uint16_t r;
        if (ecx == 0u) goto camera_arm;                /* 0x7EA44 */
        tx = state->vec5788_x;                         /* 0x7EA4A (0x157788) */
        ty = state->vec5788_y;
        tz = state->vec5788_z;
        target_set = 1;
        if (fifa96_rng_step(state->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
        tx += (int32_t)(r & 0x20u) - 0x10;             /* 0x7EA63 */
        if (fifa96_rng_step(state->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
        tz += (int32_t)(r & 0x20u) - 0x10;             /* 0x7EA76 */
      }
      /* 0x7EA7B..0x7EB5C: the install-0xF gate. */
      if (state->has_slot != 0u && (state->slot_word10 & 0x50u) == 0u) goto after_0f;
      if (state->active == 0u || state->byte99 != 0u || state->pos_y != 0) goto after_0f;
      if ((int16_t)state->track_577fa <= (int16_t)state->track_577f2) goto after_0f;
      if (state->is_team_7c7 == 0u) goto after_0f;     /* 0x7EADB */
      if (lane >= 0x120) goto after_0f;                /* 0x7EAE9 */
      if ((int16_t)lane > (int16_t)state->bound) goto after_0f;    /* 0x7EAF4 */
      if ((int32_t)(int16_t)state->track_577fa +
              (int32_t)(int16_t)state->track_57802 <
          (int32_t)(int16_t)state->track_57800) goto after_0f;     /* 0x7EB1D */
      if (row04_metric(state->pos_x, state->pos_z, state->vec5788_x,
                       state->vec5788_z) >= 0x60) goto after_0f;   /* 0x7EB45 */
      row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 0x0F, 0, 1);
      return FIFA96_OK;                                /* 0x7EB61 RET, before the clamp */
    }
    if (w > 0x50) {                                    /* 0x7EB6B */
      if ((int16_t)state->track_577fa >= (int16_t)state->track_57806)
        goto after_0f;                                 /* 0x7EB7D */
      if (ecx == 0u) goto camera_arm;                  /* 0x7EB86 */
      tx = state->vec5794_x;                           /* 0x7EB88 (0x157794) */
      ty = state->vec5794_y;
      tz = state->vec5794_z;
      target_set = 1;
      goto after_0f;
    }
  }

camera_arm:                                            /* 0x7EB95 */
  if (ecx == 0u) goto after_0f;
  tx = state->camera_x;                                /* 0x7EB9A (0x15774C) */
  ty = state->camera_y;
  tz = state->camera_z;
  tx += (int32_t)state->lead_x << 2;                   /* 0x7EBAD (word 0x1577C0) */
  tz += (int32_t)state->lead_z << 2;                   /* 0x7EBC8 (word 0x1577C2) */
  target_set = 1;
  if (lane < 0x3C0) {                                  /* 0x7EBCD */
    int32_t cz = state->camera_z;
    if (cz < 0) cz = -cz;
    if (cz > 0x570) out->receiver_timer = 1;           /* 0x7EBF1 */
  }

after_0f:                                              /* 0x7EBF8 */
  if (ecx == 0u) {
    tx = state->pos_x + (int32_t)state->slot_dir_x * 0x80;   /* 0x79C20 */
    ty = 0;
    tz = state->pos_z + (int32_t)state->slot_dir_z * 0x80;
    target_set = 1;
  }

clamp:                                                 /* 0x7EC13 */
  if (!target_set) {
    tx = state->target_x;
    ty = state->target_y;
    tz = state->target_z;
  }
  if (tx > 0x720) tx = 0x720;                          /* 0x7D3E4 */
  else if (tx < -0x720) tx = -0x720;
  if (tz > 0xB10) tz = 0xB10;
  else if (tz < -0xB10) tz = -0xB10;
  out->target_set = 1;
  out->target_x = tx;
  out->target_y = ty;
  out->target_z = tz;
  out->timer89 = state->timer89 + (int32_t)state->delta;   /* 0x7EC1B */

  if (state->has_slot != 0u) {                         /* 0x7EC34 */
    if (state->slot_word6 != 0u) out->slot_backup = 1; /* 0x7ED84 */
  } else if (state->team_828 != 0u) {                  /* 0x7EC40 */
    if (state->team_7e7 == 0u && lane < 0xF0 && state->team_7bf == 0u &&
        state->active != 0u && state->merge_gate_1586d7 == 0u)
      out->slot_merge = 1;                             /* 0x7EC80 */
  } else if (row04_defender_decision(state) != 0) {    /* 0x7EC89 */
    row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 0x0E, 0, 1);
    return FIFA96_OK;
  }

  if (state->active != 0u && other != FIFA96_OUTFIELD_ROW04_NONE) {  /* 0x7EC97 */
    uint16_t r;
    if (fifa96_rng_step(state->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    if ((r & 0x3Fu) == 0u) {                           /* 0x7ECB4 */
      int32_t d = row04_metric(state->pos_x, state->pos_z, state->opps[other].pos_x,
                               state->opps[other].pos_z);
      if (d <= 0x50) {                                 /* 0x7ECD4 */
        int32_t own = state->score_word[(state->side ^ state->side_flip) & 1u];
        int32_t oth =
            state->score_word[((state->side ^ 1u) ^ state->side_flip) & 1u];  /* 0x7ED09 */
        int32_t lhs = (int32_t)(3u * (uint32_t)state->team_7d7);              /* 0x7ED35 */
        if (own + 2 < oth || lhs < state->opp_7d7) {                           /* 0x7ED1B */
          int32_t thresh = (int32_t)(int8_t)state->desc_e | (int32_t)state->byte9d;
          uint16_t r2;
          if (fifa96_rng_step(state->rng, &r2) != FIFA96_OK) return -FIFA96_ERR_INVALID;
          if ((int32_t)(r2 & 0x1Fu) < thresh) {        /* 0x7ED58 */
            row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 0x0B, 0, 1);
            return FIFA96_OK;                          /* 0x7ED6F */
          }
        }
      }
    }
  }

  if ((int16_t)(0x40 - (int32_t)state->timer81) >= (int16_t)lane) goto bound_tail; /* 0x7ED9B */
  if (state->is_ball_track != 0u) return FIFA96_OK;    /* 0x7EDA0 */
  if (lane >= 0x90) return FIFA96_OK;                  /* 0x7EDB2 */
  out->receiver_timer = 1;                             /* 0x7EDBD 0x79B58 */
  return FIFA96_OK;

bound_tail:                                            /* 0x7EDCE */
  if ((int16_t)lane > (int16_t)state->bound) return FIFA96_OK;
  if (state->ball_height <= 0x50) goto half_line;      /* 0x7EDDF */
  if (state->active == 0u || state->pos_y != 0) return FIFA96_OK;
  if (row04_metric(state->pos_x, state->pos_z, state->vec5770_x,
                   state->vec5770_z) >= 0x30) return FIFA96_OK;  /* 0x7EE0E */
  if (state->row_byte == 0x13u) return FIFA96_OK;      /* 0x7EE24 */
  out->anim = 1;                                       /* 0x7EE3C 0x6E598(kind 0x13) */
  return FIFA96_OK;

half_line:                                             /* 0x7EE4B */
  {
    int32_t angle = 0;
    if (fifa96_entity_angle((int32_t)state->word6d, (int32_t)state->word6f, &angle) !=
        FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    angle = (int32_t)(int16_t)(uint16_t)((uint16_t)angle - (uint16_t)state->face_word7d);
    angle &= 0x3FF;                                    /* 0x7EE60 AND AH,3 */
    if (angle > 0x200) angle = 0x400 - angle;          /* 0x7EE75 */
    if (angle > 0xAB) return FIFA96_OK;                /* 0x7EE7D */
    if (state->is_ball_track == 0u && state->user_present != 0u &&
        state->user_is_self == 0u && state->side == state->ball_track_side) {
      uint16_t r;
      if (fifa96_rng_step(state->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      out->events = 1;                                 /* 0x974F0/0x651F0 (OL-72) */
    }
  }
  if (state->team_second_index == state->self_index) { /* 0x7EECA */
    out->team_second_set = 1;
    out->team_second_index = FIFA96_OUTFIELD_ROW04_NONE;
    out->team_target_set = 1;                          /* 0x7EED8 */
    out->team_target_index = state->self_index;
  }
  out->opp_7e7_clear = 1;                              /* 0x7EEE8 [opp+0x7E7] = 0 */
  if (state->team_7e7 != 0u) {                         /* 0x7EEEF */
    if (state->team_7e7 == 1u || state->team_7e7 == 2u) {   /* 0x7E528 gate */
      out->corner = 1;
      out->corner_code = (state->side == 0u)
                             ? (state->team_corner_z >= 0xB10 ? 3u : 2u)
                             : ((state->side != 1u || state->team_corner_z > -0xB10)
                                    ? 2u
                                    : 3u);
      out->team7e7_inc = 1;                            /* 0x7E5D1 */
      row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 7, 0, 1); /* 0x7E5C9 */
      return FIFA96_OK;
    }
  } else if (state->has_slot != 0u) {                  /* 0x7EF0E */
    out->slot_restore = 1;                             /* 0x7EF15 */
    if (state->is_team_7cb != 0u) {                    /* 0x7EF1A */
      row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 7, 1, 1); /* 0x7EF33 */
      return FIFA96_OK;
    }
  }

  if (state->row_byte == 0x13u) {                      /* 0x7EF42 */
    int16_t ex;
    int16_t ez;
    if (!state->type_off_x || !state->type_off_z) return -FIFA96_ERR_INVALID;
    ex = (int16_t)((int16_t)(int8_t)state->type_off_x[state->type8] << 6);  /* 0x7EF79 */
    ez = (int16_t)((int16_t)(int8_t)state->type_off_z[state->type8] << 6);  /* 0x7EF9A */
    out->events = 1;                                   /* 0x974DC/0x8F188/0x92820/0x71C94 */
    out->event_code = 0x15;
    out->event_x = ex;
    out->event_z = ez;
    out->event_dist = (int16_t)row04_metric(0, 0, ex, ez);   /* 0x8DC68 -> [ESP] */
    out->event_track_reload = 1;                       /* 0x7EFD4 */
  } else if (state->is_ball_track == 0u) {             /* 0x7EFEE */
    int16_t ex = 0;
    int16_t ez = 0;
    if (state->ball_height != 0) {                     /* 0x7F003 */
      if (!state->type_off_x || !state->type_off_z) return -FIFA96_ERR_INVALID;
      ex = (int16_t)((int16_t)(int8_t)state->type_off_x[state->type8] << 5);
      ez = (int16_t)((int16_t)(int8_t)state->type_off_z[state->type8] << 5);
    } else if (state->has_slot != 0u) {                /* 0x7F035 */
      ex = (int16_t)((int16_t)(int8_t)state->slot_dir_x << 6);
      ez = (int16_t)((int16_t)(int8_t)state->slot_dir_z << 6);
    } else {                                           /* 0x7F056 */
      int32_t ax = state->camera_x < 0 ? -state->camera_x : state->camera_x;
      if (ax <= 0x630) {
        int32_t az = state->camera_z < 0 ? -state->camera_z : state->camera_z;
        if (az <= 0xA20) {
          ex = (int16_t)((uint16_t)state->vel_int_x << 2);   /* word[+0x73]<<2 */
          ez = (int16_t)((uint16_t)state->vel_int_z << 2);   /* word[+0x75]<<2 */
        }
      }
    }
    out->events = 1;                                   /* 0x92820/0x71C94 */
    out->event_code = 0x1D;
    out->event_x = ex;
    out->event_z = ez;
    out->event_dist = (int16_t)row04_metric(0, 0, ex, ez);   /* 0x7F0A7 0x8DC68 -> [ESP] */
  }

  if (state->active != 0u && other != FIFA96_OUTFIELD_ROW04_NONE) {  /* 0x7F0D6 */
    uint8_t ocode = state->opps[other].code;
    if (ocode == 4u || ocode == 5u) {                  /* 0x7F0F0 */
      if ((int16_t)lane > (int16_t)state->opps[other].lane) {        /* 0x7F106 */
        row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 6, 0, 0);  /* 0x7F108 */
        return FIFA96_OK;
      }
      row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_OTHER, 6, 0, 0);   /* 0x7F120 */
    }
  }
  row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 5, 0, 1);       /* 0x7F133 */
  return FIFA96_OK;
}

/* `FUN_000795A4` (`0x795A4..0x795B2`, first-hand): EAX = a, EDX = b, the
 * signed 32x32 product + 0x8000 shifted right 16, the low word returned (the
 * row-08 scan passes the loop offset and the folded sine value). */
static int16_t row08_fold(int32_t a, int32_t b) {
  int64_t v = ((int64_t)a * (int64_t)b + 0x8000) >> 16;
  return (int16_t)(uint16_t)(uint32_t)(int32_t)v;
}

/* ===== M2 playability-legs Task 2 / OL-70a: row 08 record-visible body =====
 *
 * The port cites FU-142 Appendix K.6 (first-hand disassemble_bytes
 * 0x81068..0x8127F + 0x81280..0x814AF on /FIFA96.EXE); every branch and
 * operand is read from those bytes. `row04_metric` is the same native
 * `0x8DCD4` distance the scan uses. The native `0x7D9A4` installer never runs
 * (row 08 installs no code). */
fifa96_err_t fifa96_outfield_row08_step(const fifa96_outfield_row08_state *state,
                                        fifa96_outfield_row08_out *out) {
  uint8_t stage;
  uint8_t type8;
  int16_t word7d;
  if (!state || !out) return -FIFA96_ERR_INVALID;
  memset(out, 0, sizeof *out);
  type8 = state->type8;
  word7d = state->word7d;
  out->byte_15877d = state->byte_15877d;
  out->byte_15872f = (uint8_t)state->byte_15872f;
  out->timer89 = state->timer89;
  out->stage92 = state->stage;
  if (state->phase != 2u) {                            /* 0x81073 */
    out->reset = 1;                                    /* 0x81080 */
    return FIFA96_OK;
  }
  out->timer89 = state->timer89 + (int32_t)state->delta;   /* 0x8108E..0x810A2 */
  stage = state->stage;

  if (stage == 0u) {                                   /* 0x810A8 JC */
    if (state->active == 0u) {                         /* 0x810CC */
      out->reset = 1;                                  /* 0x810D5 */
      out->clear_team_target = state->is_team_target;  /* 0x810DF */
      out->clear_team_second = state->is_team_second;  /* 0x810F4 */
      return FIFA96_OK;
    }
    /* 0x81114: the 0x15774C camera triple + word[0x1577C0/C2] << 3. */
    out->target_set = 1;
    out->target_x = state->camera_x + ((int32_t)state->lead_x << 3);
    out->target_y = state->camera_y;
    out->target_z = state->camera_z + ((int32_t)state->lead_z << 3);
    if (state->has_slot == 0u) {                       /* 0x8114C */
      if ((int16_t)state->lane > 0x50 || state->ball_height > 0x38) {
        out->receiver_timer = 1;                       /* 0x79B58 call */
        if (out->timer89 > 0x3c) out->reset = 1;       /* 0x81167/0x81174 */
        return FIFA96_OK;
      }
    }
    if (state->lane != 0 || state->word6d != 0) {      /* 0x79C59 guard */
      int32_t angle = 0;
      if (fifa96_entity_angle((int32_t)state->lane, (int32_t)state->word6d, &angle) !=
          FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      word7d = (int16_t)angle;
      type8 = (uint8_t)(((uint32_t)(angle + 0x40) & 0x3FFu) >> 7u);  /* 0x79C8E */
      out->face = 1;
      out->face_angle = word7d;
      out->face_octant = type8;
    }
    out->anim = 1;                                     /* 0x6E598(rec,0xB,type8,0) */
    out->ran = 1;                                      /* 0x811AF */
    out->timer89 = 0;                                  /* 0x811B6 */
    stage = (uint8_t)(stage + 1u);                     /* 0x811C0..0x811D0 */
    out->byte_15877d = 0;                              /* 0x811CA */
    out->byte_15877d_set = 1;
  } else if (stage == 2u) {                            /* 0x8147C */
    out->byte_15877d = 0;                              /* 0x8147E */
    out->byte_15877d_set = 1;
    out->snap = 1;                                     /* 0x79B1C */
    out->target_set = 1;
    out->target_x = state->pos_x;
    out->target_y = state->pos_y;
    out->target_z = state->pos_z;
    if (state->byte44 != 0u) {                         /* 0x8148B */
      out->reset = 1;                                  /* 0x81493 */
      if (state->is_ball_track == 0u) {                /* 0x81498 */
        out->byte_15872f = (uint8_t)(state->byte_15872f + 1);   /* 0x814A0 */
        out->byte_15872f_set = 1;
      }
    }
    return FIFA96_OK;
  } else if (stage != 1u) {                            /* stage >= 3 */
    return FIFA96_OK;                                  /* 0x810BA plain return */
  }

  /* 0x811D6: the stage-1 gate. */
  if (out->byte_15877d != 0u || state->byte44 != 0u) { /* 0x811E5 */
    out->stage92 = (uint8_t)(stage + 1u);
    out->timer89 = 0;
    return FIFA96_OK;
  }
  if (state->byte3d != 1u) {                           /* 0x81207 */
    out->stage92 = stage;
    return FIFA96_OK;
  }

  /* 0x81216..0x81472: the projection scan. */
  {
    int32_t delta_desc;
    int32_t q_raw;
    int16_t q;
    int16_t sine1;
    int16_t sine2;
    int32_t ecx;
    int hit = 0;
    int16_t dist = 0;
    int16_t x_off = 0;
    int16_t z_off = 0;
    if (state->user_present != 0u && state->user_is_opp_target != 0u) {
      delta_desc = (int16_t)((int16_t)state->desc_c + (int16_t)state->desc_16) -
                   (int16_t)((int16_t)state->desc_opp_c + (int16_t)state->desc_opp_f);
    } else {
      delta_desc = (int16_t)((int16_t)state->desc_c + (int16_t)state->desc_16);
    }
    q_raw = delta_desc * 6 + ((int32_t)state->byte_15872f << 3) + 0x20;  /* 0x81266 */
    q = (int16_t)(uint16_t)q_raw;
    if (q > 0x40) q = 0x40;                            /* 0x8128A */
    else if (q < 8) q = 8;                             /* 0x81298 */
    sine1 = (int16_t)fifa96_entity_sine((int32_t)word7d);          /* 0x812A5 */
    sine2 = (int16_t)fifa96_entity_sine((int32_t)word7d + 0x100);  /* 0x812D3 */
    for (ecx = 0; ecx < 0x40; ecx += 0x10) {           /* 0x81466 */
      int32_t lx = state->pos_x + (int32_t)row08_fold(ecx, sine1);
      int32_t lz = state->pos_z + (int32_t)row08_fold(ecx, sine2);
      int16_t d = (int16_t)row04_metric(state->camera_x, state->camera_z, lx, lz);
      if (q <= d) continue;                            /* 0x81351 JLE */
      hit = 1;
      x_off = (int16_t)((int32_t)(int8_t)state->type_off_x[type8] * 0xA0);  /* 0x81357 */
      z_off = (int16_t)((int32_t)(int8_t)state->type_off_z[type8] * 0xA0);  /* 0x81373 */
      dist = (int16_t)fifa96_entity_distance((int32_t)d, (int32_t)x_off);   /* 0x8DC68 */
      break;
    }
    out->scan = 1;
    out->scan_hit = (uint8_t)hit;
    out->scan_dist = dist;
    if (hit) {
      if (state->user_present != 0u && state->user_row_byte == 0x4Au) {     /* 0x813A2 */
        out->user_row_4a = 1;
      } else {
        out->event_code = 0x16;                        /* 0x92820 */
        out->ball_stage = 1;                           /* 0x7A490 */
        out->ball_stage_dist = dist;
        out->ball_stage_x = x_off;
        out->ball_stage_z = z_off;
        out->byte_15877d = 1;                          /* 0x8141C */
        out->byte_15877d_set = 1;
      }
      out->events = 1;                                 /* 0x8ED40 + the sinks */
      {
        uint16_t r = 0;
        if (fifa96_rng_step(state->rng, &r) != FIFA96_OK)
          return -FIFA96_ERR_INVALID;
        out->event_sound = (int16_t)((int32_t)(r & 0x7Fu) + 0x190);   /* 0x974F0 */
      }
      out->stage92 = (uint8_t)(stage + 1u);            /* 0x81453 */
      out->timer89 = 0;                                /* 0x81447 */
      return FIFA96_OK;
    }
    out->stage92 = stage;
    return FIFA96_OK;
  }
}
