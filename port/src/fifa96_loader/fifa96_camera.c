#include "fifa96_loader/fifa96_camera.h"
#include "fifa96_loader/fifa96_entity_update.h"
#include "fifa96_loader/fifa96_projection.h"
#include "fifa96_loader/fifa96_rng.h"

#include <string.h>

static int32_t camera_abs(int32_t value) {
  uint32_t magnitude = (uint32_t)value;
  if (value < 0) magnitude = 0u - magnitude;
  return (int32_t)magnitude;
}

static int32_t camera_atan16(int32_t x, int32_t z);
static int32_t camera_atan_pitch16(int32_t dx, int32_t dy, int32_t dz);
static int32_t camera_pitch_delta(fifa96_camera_handler_state *st,
                                  int32_t point_x, int32_t point_y,
                                  int32_t point_z, int32_t rate,
                                  int32_t target1, int32_t tick);

int fifa96_camera_focus_clamp(int32_t x, int32_t y, int32_t z,
                              fifa96_camera_focus *out) {
  if (!out) return -FIFA96_ERR_INVALID;
  /* FUN_00036C70 0x36CA2..0x36CFA: the signed dword bounds. The fallback
   * replaces all three, then the Y band runs on the replacement. */
  if (x > 6000 || x < -6000 || z > 6000 || z < -6000) {
    x = -4000;
    y = 0;
    z = -4000;
  }
  if (y < 0 || y > 16000) y = 0;
  out->x = x;
  out->y = y;
  out->z = z;
  return FIFA96_OK;
}

int fifa96_camera_target_slot(int32_t roster, int32_t side, int32_t ac2) {
  /* FUN_0004BE44 0x4B99F..0x4B9FF. FUN_0004B678 is `ac2 >= 4`; the odd
   * `ac2 & 1` arm flips which side receives the away-bank addend. */
  int add = 0;
  if ((ac2 & 1) != 0) {
    if (ac2 < 4) add = side == 0;
    else add = side == 1;
  } else {
    add = side == 1;
  }
  return roster + (add ? 0xB : 0);
}

const int32_t *fifa96_camera_target_select(
    int32_t source, int32_t force_focus, int32_t replay_active,
    int32_t record_class, const int32_t *focus, const int32_t *pan,
    const int32_t *record, const int32_t *replay_fallback,
    const int32_t table[FIFA96_CAMERA_ACTION_SLOTS][3], int32_t index) {
  if (force_focus != 0) return focus;
  if (replay_active != 0 && record_class == -1) return replay_fallback;
  switch (source) {
    case 0:
      return focus;
    case 1:
      return pan;
    case 2:
      return record;
    case 3:
      return table != NULL ? table[22] : NULL;
    case 4:
      if (index >= 0 && index < 0x16)
        return table != NULL ? table[index] : NULL;
      return focus;
    default:
      return focus;
  }
}

int fifa96_camera_replay_choice_select(
    const fifa96_camera_replay_target *target,
    fifa96_camera_replay_activity *activity,
    fifa96_camera_replay_choice *out) {
  int32_t counts[2] = {0, 0};
  int32_t selected = -1;
  int32_t best_side = 0;
  int32_t best_id;
  int32_t best_count;
  int32_t slot = 0;
  int32_t r;
  int n;
  if (!target || !activity || !out) return -FIFA96_ERR_INVALID;
  if (target->score[0] != target->score[1]) {
    n = target->event_count;
    if (n > FIFA96_CAMERA_REPLAY_EVENTS) n = FIFA96_CAMERA_REPLAY_EVENTS;
    for (int i = 0; i < n; i++) {
      /* 0x4BC26: the selected index is the first event whose pre-add
       * counts are equal and nonzero — the start of a scoring run. */
      if (counts[0] == counts[1] && counts[0] != 0) selected = i;
      if (target->event_side[i] < 2u) counts[target->event_side[i]]++;
    }
  }
  /* 0x8EE04: bump the selected event's player by 0x32, then pick the
   * highest activity count. The initial best is entry [0][0]'s count and
   * the strict compare keeps the earliest entry on a tie. */
  best_count = activity->count[0][0];
  best_id = activity->id[0][0];
  for (int side = 0; side < 2; side++) {
    for (int i = 0; i < FIFA96_CAMERA_REPLAY_SLOTS; i++) {
      if (selected >= 0 && side == (int)target->event_side[selected] &&
          activity->id[side][i] == target->event_id[selected])
        activity->count[side][i] += 0x32;
      if (activity->count[side][i] > best_count) {
        best_count = activity->count[side][i];
        best_id = activity->id[side][i];
        best_side = side;
      }
    }
  }
  /* FUN_0004BBFC's 0x14C360 scan: the first slot with the winning id, else
   * slot 0 (the native `iVar4 == 0x10` reset at 0x4BCB0). */
  for (int i = 0; i < FIFA96_CAMERA_REPLAY_SLOTS; i++) {
    if (target->order_id[best_side][i] == best_id) {
      slot = i;
      break;
    }
  }
  r = best_side * 0x10 + slot;
  out->side = best_side;
  out->slot = slot;
  out->target_index = best_side * 0xB + slot;
  out->copy_slot = -1;
  out->source_index = 0;
  out->order_value = target->order_id[best_side][slot];
  if (r == 0) return FIFA96_OK;            /* 0x4BCD2 -> 0 */
  if (r == 0x10) {                         /* 0x4BCD4 -> 0xB */
    out->source_index = 0xB;
    out->target_index = 0xB;
    return FIFA96_OK;
  }
  /* 0x4BCE2..0x4BD31: copy the selected order entry to current slot 1 and
   * return side*0xB + 1 (the native 0x157BD6 write + FUN_0004728C call).
   * The engine targets the selected pitch slot directly instead of
   * reordering the pool; `copy_slot` records the native seam. */
  out->copy_slot = 1;
  out->source_index = best_side * 0xB + 1;
  return FIFA96_OK;
}

int32_t fifa96_camera_pose_axis_a(int32_t track_delta, int32_t event_slot,
                                  int32_t ac2, int32_t *latch) {
  int32_t out;
  if (ac2 > 3) {
    out = -1;
  } else if (event_slot < 0xB || event_slot > 0x15) {
    if (event_slot < 0) {
      if (track_delta < 0) out = -1;
      else if (track_delta > 0) out = 1;
      else out = 0;
    } else if (ac2 != 0) {
      out = -1;
    } else {
      out = 1;
    }
  } else if (ac2 == 0) {
    out = -1;
  } else {
    out = 1;
  }
  if (latch != NULL) *latch = out;
  return out;
}

int32_t fifa96_camera_ramp_factor(int32_t tick, int32_t rate) {
  /* 0x4D66B..0x4D670: the rate*15 is a 32-bit wrap. */
  int32_t divisor = (int32_t)((uint32_t)rate * 15u);
  if (tick <= 0 || divisor <= 0) return 0;
  return (int32_t)(((int64_t)tick * 0x10000) / divisor);
}

static int32_t camera_slide_step(int32_t delta, int32_t factor) {
  /* Native `IMUL EDX; ADD EAX,0x8000; ADC EDX,0; SHRD EAX,EDX,0x10`. */
  return (int32_t)(((int64_t)delta * factor + 0x8000) >> 16);
}

int32_t fifa96_camera_slide(int32_t cam, int32_t target, int32_t hist,
                            int32_t mode, int32_t lo, int32_t hi,
                            int32_t hist_offset, int32_t rate, int32_t tick) {
  int32_t clamped = (int32_t)((uint32_t)target - (uint32_t)hist_offset);

  if (clamped < lo) clamped = lo;
  else if (clamped > hi) clamped = hi;

  if (cam > lo && cam < hi) {
    int32_t factor = fifa96_camera_ramp_factor(tick, rate);
    int32_t delta = (int32_t)((uint32_t)clamped - (uint32_t)cam);
    int32_t step = camera_slide_step(delta, factor);
    int32_t next = (int32_t)((uint32_t)cam + (uint32_t)step);
    int32_t dir = (int32_t)((uint32_t)target - (uint32_t)hist);
    if (mode < 1) {
      if (mode != -1) return cam;
      if (dir < -0xA0 && step > 0) return cam;
      if (next < lo) return lo;
    } else if (mode < 2) {
      if (0xA0 < dir && step < 0) return cam;
      if (next < lo) return lo;
    } else {
      if (mode != 2) return cam;
      /* [0x108B94] = 1 (no writer), so the native direction gate is skipped. */
      if (next < lo) return lo;
    }
    if (next > hi) return hi;
    return next;
  }

  {
    int32_t factor = fifa96_camera_ramp_factor(tick, 8);
    int32_t delta = (int32_t)((uint32_t)clamped - (uint32_t)cam);
    return (int32_t)((uint32_t)cam +
                     (uint32_t)camera_slide_step(delta, factor));
  }
}

/* FUN_0004E248's four Q16 factors are `(tick * n * 0x1111 + 0x8000) >> 16`;
 * the `tick * n` multiply is the native 32-bit wrap. */
static int32_t camera_1111_step(int32_t tick, int32_t n) {
  int32_t product = (int32_t)((uint32_t)tick * (uint32_t)n);
  return (int32_t)(((int64_t)product * 0x1111 + 0x8000) >> 16);
}

int32_t fifa96_camera_steady_pan_step(int32_t axis_a, int32_t axis_b,
                                      int32_t pan, int32_t rate,
                                      int32_t track_z, int32_t tick) {
  int32_t f20 = camera_1111_step(tick, 20);
  int32_t f32 = camera_1111_step(tick, 32);
  int32_t frate = camera_1111_step(tick, rate);

  if (axis_a == 0) {
    int32_t mag = camera_abs(pan);
    if (mag < f20) return -pan;
    if (pan < 1) return pan < 0 ? f20 : 0;
    return -f20;
  }
  if (axis_b == 0) return axis_a < 0 ? -f32 : f32;
  if ((axis_a < 0 && axis_b > 0) || (axis_a > 0 && axis_b < 0)) {
    if (camera_abs(track_z) > 0x587) return 0;
    return axis_a > 0 ? -f32 : f32;
  }
  return axis_a > 0 ? frate : -frate;
}

int32_t fifa96_camera_yaw_snap(int32_t class_of, int32_t pos_x, int32_t pos_z,
                               int32_t track_z, int32_t yaw, int32_t ratio) {
  int32_t angle, target, diff;
  if (class_of == 3) {
    if (track_z <= 0x8A0) return yaw;
    if (yaw > 0x8000) {
      angle = (int32_t)((0u - (uint32_t)camera_atan16(0x720 - pos_x,
                                                      0xB60 - pos_z)) & 0xFFFFu);
      target = (int32_t)((uint32_t)angle - (uint32_t)ratio);
      diff = (int32_t)(((uint32_t)yaw - (uint32_t)target) & 0xFFFFu);
      return diff < 0x8000 ? target : yaw;
    }
    angle = (int32_t)((0u - (uint32_t)camera_atan16(-0x720 - pos_x,
                                                    0xB60 - pos_z)) & 0xFFFFu);
    target = (int32_t)((uint32_t)angle + (uint32_t)ratio);
    diff = (int32_t)(((uint32_t)yaw - (uint32_t)target) & 0xFFFFu);
    return diff > 0x8000 ? target : yaw;
  }
  if (track_z >= -0x8A0) return yaw;
  if (yaw > 0x8000) {
    angle = (int32_t)((0u - (uint32_t)camera_atan16(0x720 - pos_x,
                                                    -0xB60 - pos_z)) & 0xFFFFu);
    target = (int32_t)((uint32_t)angle + (uint32_t)(ratio >> 1));
    diff = (int32_t)(((uint32_t)yaw - (uint32_t)target) & 0xFFFFu);
    return diff > 0x8000 ? target : yaw;
  }
  angle = (int32_t)((0u - (uint32_t)camera_atan16(-0x720 - pos_x,
                                                  -0xB60 - pos_z)) & 0xFFFFu);
  target = (int32_t)((uint32_t)angle - (uint32_t)(ratio >> 1));
  diff = (int32_t)(((uint32_t)yaw - (uint32_t)target) & 0xFFFFu);
  return diff < 0x8000 ? target : yaw;
}

int32_t fifa96_camera_view_preset(int32_t phase, int32_t difficulty,
                                  int32_t option_15b680) {
  if (phase == 3) return difficulty == 1 ? 1 : 2;
  if (phase == 4) return difficulty == 1 ? 3 : 4;
  if (phase == 0x15) {
    if (option_15b680 == 3) return 5;
    if (option_15b680 == 0) return 6;
    return 8;
  }
  return 0;
}

int32_t fifa96_camera_view_select(int32_t phase, int32_t option, int32_t b680,
                                  fifa96_camera_view *out) {
  /* FUN_0004B308 (phase/option/0x15B680) + FUN_0004D04C (clamp/table). */
  static const int32_t slots[7] = {1, 1, 0, 0, 2, 2, 5};
  static const int32_t variants[7] = {0, 1, 0, 1, 0, 1, 1};
  static const int32_t classes[6] = {0, 1, 2, -1, -1, 3};
  int32_t preset;
  if (!out) return -FIFA96_ERR_INVALID;
  preset = fifa96_camera_view_preset(phase, option, b680);
  if (preset < 0 || preset > 6) preset = 0;   /* the FUN_0004D04C clamp */
  out->preset = preset;
  out->slot = slots[preset];
  out->variant = variants[preset];
  out->class_of = classes[out->slot];
  return FIFA96_OK;
}

int fifa96_camera_init(fifa96_camera *cam, int32_t x, int32_t y, int32_t z) {
  if (!cam) return -FIFA96_ERR_INVALID;
  cam->pos_x = x;
  cam->pos_y = y;
  cam->pos_z = z;
  cam->prev_y = 0;                    /* [0x15775C] starts clear */
  cam->prev_x = 0;                    /* [0x157758] */
  cam->prev_z = 0;                    /* [0x157760] */
  cam->target_x = x;
  cam->target_y = y;
  cam->target_z = z;
  cam->anchor_x = x;
  cam->anchor_y = y;
  cam->anchor_z = z;
  cam->anchor2_x = x;
  cam->anchor2_y = y;
  cam->anchor2_z = z;
  cam->vel_x = 0;
  cam->vel_z = 0;
  cam->step = 0;
  cam->acc_x = 0;
  cam->acc_z = 0;
  cam->speed = 0;
  cam->timer = 0;
  cam->timer_limit = 0;
  cam->event_param = 0;
  cam->event_cursor = 0;
  cam->anchor_time = 0;
  cam->anchor2_time = 0;
  cam->rate_x = 0;
  cam->rate_z = 0;
  cam->dir_157815 = 0;
  cam->paused = 0;
  cam->event_suspended = 0;
  cam->ramp_divisor = 0;
  cam->pan_counter = 0;
  /* FUN_000700F4 0x7011C/0x7013D/0x70146: the 0x1104AB table row selected by
   * `[0x14C2FE]*8 + [0x14C2FA]*2` (the image defaults are both zero) stores
   * pan_decay = 10, pan_rate_hi = 16 and pan_rate_lo = hi + (0x20 - hi)/2 = 24
   * (0x70137 SUB, 0x7013b SAR 1, 0x70142 ADD, 0x70146 store). */
  cam->pan_decay = 10;
  cam->pan_rate_hi = 16;
  cam->pan_rate_lo = (uint8_t)(cam->pan_rate_hi + (0x20 - cam->pan_rate_hi) / 2);
  cam->event_step_x = 0;
  cam->event_step_z = 0;
  cam->tracked = FIFA96_CAMERA_TRACKED_NONE;   /* 0x700F4 0x70258 */
  cam->follow_speed = 0;
  cam->event_peak = 0;                         /* 0x1577F2 */
  cam->band_1577f6 = 0;                        /* 0x1577F6 */
  cam->shadow_speed_157825 = 0;                /* [0x157825] */
  cam->shadow_vel_157827 = 0;                  /* [0x157827] */
  cam->shadow_vel_157829 = 0;                  /* [0x157829] */
  return FIFA96_OK;
}

int fifa96_camera_set_tracked(fifa96_camera *cam, int32_t id) {
  if (!cam || id < FIFA96_CAMERA_TRACKED_NONE) return -FIFA96_ERR_INVALID;
  cam->tracked = id;                            /* 0x71D27 [0x1577CA] = rec */
  return FIFA96_OK;
}

/* FU-152 §2.9 (T2) / FU316-L2: FUN_000709D0 is defined below the integrator;
 * the native FUN_000736AC calls it in-line (0x737da) when `timer >
 * timer_limit` or when the camera lands (`prev_y > 0 && pos_y <= 0`). */
static int camera_pan_step_internal(fifa96_camera *cam, struct fifa96_rng *rng,
                                    int walk_gate, int32_t *class_of,
                                    int32_t *param);

int fifa96_camera_update(fifa96_camera *cam, int16_t delta, int cursor_high,
                         int input_bit2) {
  return fifa96_camera_update_walk(cam, NULL, delta, cursor_high, input_bit2, 0);
}

int fifa96_camera_update_walk(fifa96_camera *cam, struct fifa96_rng *rng,
                              int16_t delta, int cursor_high, int input_bit2,
                              int walk_gate) {
  int16_t gate_speed;
  if (!cam) return -FIFA96_ERR_INVALID;
  /* FU316-L2: 0x73756/0x7375b copies the camera triple to 0x157758 before
   * the pause check; its +4 dword is the landing cell 0x15775C. The 0x7375d
   * third MOVSD leaves the camera Z in the far-wall arm's prev cell as well
   * (FU-71 leg 9.6). */
  cam->prev_x = cam->pos_x;
  cam->prev_y = cam->pos_y;
  cam->prev_z = cam->pos_z;
  if (cam->paused) {
    cam->follow_speed = (int16_t)cam->speed;
    return FIFA96_OK;
  }
  /* The 0x739CE follow gate reads [0x1577BE] before the tail recompute; the
   * pan step may overwrite it first. */
  gate_speed = (int16_t)cam->speed;
  if ((int16_t)cam->speed != 0 || (int16_t)cam->event_param != 0) {
    cam->timer = (uint16_t)(cam->timer + (uint16_t)delta);
    /* M2 phase-11 T1 (FU-148 §14.5 / L14.7): FUN_000736AC 0x737a1..0x737b4 —
     * with a live event height (`word[0x1577F0] != 0`) the camera Y (native
     * word[0x157750], this struct's `pos_y`) is the FUN_00070B94 arc. The
     * store precedes the pan-step check (0x737b9) and the integration. */
    if ((int16_t)cam->event_param != 0)
      cam->pos_y = fifa96_camera_height(cam);
    /* FU-152 §2.9 (T2) / FU316-L2 / FUN_000736AC 0x737b9..0x737da: the pan
     * step runs before the same frame's integration on the timer arm or the
     * landing arm (`prev_y > 0 && pos_y <= 0`); the walk gate is the native
     * ([0x14C1D4]|[0x14C1D6]) & 4 read at 0x70ab1..0x70ac2 (T3). */
    if ((int16_t)cam->timer > (int16_t)cam->timer_limit ||
        (cam->prev_y > 0 && cam->pos_y <= 0)) {
      (void)camera_pan_step_internal(cam, rng, walk_gate, NULL, NULL);
      gate_speed = (int16_t)cam->speed;
    }
    if ((int16_t)cam->speed != 0) {
      if (delta > 0) {
        cam->acc_x = (uint16_t)((uint16_t)cam->vel_x * (uint16_t)delta);
        cam->acc_z = (uint16_t)((uint16_t)cam->vel_z * (uint16_t)delta);
      } else {
        cam->acc_x = 0;
        cam->acc_z = 0;
      }
      cam->step = (uint16_t)fifa96_entity_distance((int16_t)cam->acc_x, (int16_t)cam->acc_z);
      cam->pos_x += (int16_t)cam->acc_x;
      cam->pos_z += (int16_t)cam->acc_z;
      if ((int16_t)cam->timer > (int16_t)cam->anchor_time) {
        cam->anchor_x = cam->pos_x;
        cam->anchor_y = cam->pos_y;
        cam->anchor_z = cam->pos_z;
        cam->anchor_time = cam->timer;
      }
      if ((int16_t)cam->timer > (int16_t)cam->anchor2_time) {
        cam->anchor2_x = cam->pos_x;
        cam->anchor2_y = cam->pos_y;
        cam->anchor2_z = cam->pos_z;
        cam->anchor2_time = cam->timer;
      }
      if ((int16_t)cam->event_param > 0x10 && (cam->rate_x != 0 || cam->rate_z != 0)) {
        int interpolate = 0;
        if (input_bit2) {
          interpolate = 1;
        } else if ((int16_t)cam->event_cursor >=
                   (cursor_high ? 0x90 : 0x70)) {   /* 0x73A80 [0x14C2F6] */
          interpolate = 1;
        } else {
          cam->event_cursor = (uint16_t)(cam->event_cursor + cam->step);
        }
        if (interpolate) {
          int16_t remaining = (int16_t)(uint16_t)(cam->timer_limit - cam->timer);
          int16_t dx = (int16_t)(cam->rate_x * remaining);
          int16_t dz = (int16_t)(cam->rate_z * remaining);
          cam->vel_x = (int16_t)((uint16_t)cam->vel_x + (uint16_t)cam->rate_x);
          cam->vel_z = (int16_t)((uint16_t)cam->vel_z + (uint16_t)cam->rate_z);
          cam->target_x += dx;
          cam->anchor_x += dx;
          cam->target_z += dz;
          cam->anchor_z += dz;
          cam->event_cursor = 0;
        }
      }
    }
  }
  cam->speed = (uint16_t)fifa96_entity_distance(cam->vel_x, cam->vel_z);
  cam->follow_speed = gate_speed;              /* the 0x739CE pre-tail value */
  return FIFA96_OK;
}

int fifa96_camera_commit(fifa96_camera *cam) {
  if (!cam) return -FIFA96_ERR_INVALID;
  /* Slice-B Task 5: the camera-visible part of the native `0x73723 CALL
   * 0x92864` + `0x73728..0x73750` stores. The native order is: the commit
   * CALL (FUN_00092864) reads the live `speed`/`vel_x`/`vel_z` and compares
   * them against the previous frame's shadows, appending the 0x15B650 sink
   * record to the 0x15B440 event ring on a change (and clearing the sink
   * byte); then the three word stores re-copy the live triple into the
   * shadows. The ring append is a non-camera effect (the replay event ring,
   * FU-142 OL-27) the loader does not own -> numbered leg T5-L1. The three
   * shadow stores are ported here, byte-for-byte as the native 16-bit MOVs:
   *   [0x157825] = word[0x1577BE] (`speed`),
   *   [0x157827] = word[0x1577C0] (`vel_x`),
   *   [0x157829] = word[0x1577C2] (`vel_z`). */
  cam->shadow_speed_157825 = cam->speed;
  cam->shadow_vel_157827 = cam->vel_x;
  cam->shadow_vel_157829 = cam->vel_z;
  return FIFA96_OK;
}

/* Slice-B Task 6: the native `0x736B2..0x73723` walk head jitter. */
int fifa96_camera_walk_jitter(fifa96_camera *cam, struct fifa96_rng *rng) {
  int16_t speed;
  uint16_t value;
  if (!cam) return -FIFA96_ERR_INVALID;
  speed = (int16_t)cam->speed;               /* 0x736b2/0x736b7 */
  if (speed <= 0xC) return FIFA96_OK;        /* 0x736ba/0x736bd */
  if (cam->pos_y <= 0xA0) return FIFA96_OK;  /* 0x736c3/0x736cd */
  if (speed < 0x19) {                        /* 0x736cf/0x736d2 */
    value = 0;
    if (fifa96_rng_step(rng, &value) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    /* 0x736dd/0x736e2/0x736e4: DIV EBX with EBX = 0x21 - speed (EDX = 0). */
    if ((uint32_t)value % (0x21u - (uint32_t)speed) != 0u) return FIFA96_OK;
  }
  value = 0;
  if (fifa96_rng_step(rng, &value) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  if ((value & 7u) != 0u) return FIFA96_OK;  /* 0x736e8/0x736ed/0x736ef */
  value = 0;
  if (fifa96_rng_step(rng, &value) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  if (value & 1u)                            /* 0x736f1/0x736f6/0x736fa/0x73703 */
    cam->vel_x = (int16_t)((uint16_t)cam->vel_x + 1u);
  else
    cam->vel_x = (int16_t)((uint16_t)cam->vel_x - 1u);
  value = 0;
  if (fifa96_rng_step(rng, &value) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  if (value & 1u)                            /* 0x7370a/0x7370f/0x73713/0x7371c */
    cam->vel_z = (int16_t)((uint16_t)cam->vel_z + 1u);
  else
    cam->vel_z = (int16_t)((uint16_t)cam->vel_z - 1u);
  return FIFA96_OK;
}

int fifa96_camera_target(const fifa96_camera *cam, int16_t *x, int16_t *z) {
  if (!cam || !x || !z) return -FIFA96_ERR_INVALID;
  if ((int16_t)cam->timer < (int16_t)cam->anchor_time) {
    *x = (int16_t)cam->anchor_x;
    *z = (int16_t)cam->anchor_z;
    return 0;
  }
  if ((int16_t)cam->timer < (int16_t)cam->anchor2_time) {
    *x = (int16_t)cam->anchor2_x;
    *z = (int16_t)cam->anchor2_z;
    return 1;
  }
  *x = (int16_t)(cam->target_x + (int32_t)(int16_t)cam->vel_x * 32);
  *z = (int16_t)(cam->target_z + (int32_t)(int16_t)cam->vel_z * 32);
  return 2;
}

int fifa96_camera_reflect(fifa96_camera *cam, int input_bit0, int transition_active) {
  int32_t abs_x;
  int32_t abs_z;
  if (!cam) return -FIFA96_ERR_INVALID;
  abs_x = camera_abs(cam->pos_x);
  abs_z = camera_abs(cam->pos_z);
  if (!(abs_z > 0xB10 || abs_x > 0x720)) return 0;
  if (transition_active || !input_bit0) return 0;
  if (abs_x > 0x720) {
    cam->vel_x = (int16_t)(0 - (uint16_t)cam->vel_x);
    cam->pos_x = (cam->pos_x > 0 ? 0xE40 : -0xE40) - cam->pos_x;
  }
  if (abs_z > 0xB10) {
    cam->vel_z = (int16_t)(0 - (uint16_t)cam->vel_z);
    cam->pos_z = (cam->pos_z > 0 ? 0x1620 : -0x1620) - cam->pos_z;
  }
  return 1;
}

int fifa96_camera_out_of_bounds(int32_t x, int32_t z) {
  return (camera_abs(x) > 0x6C0) || (camera_abs(z) > 0xAB0);
}

/* FU-71 leg 9.6 (phase-51 (FU-71)): the sign-extended low word of a 32-bit
 * magnitude — the native `MOV word[ESP+..], AX` / `MOVSX` pair at
 * `0x7136C`/`0x71383` re-read as words (`dword[ESP+0xE] >> 16`). A magnitude
 * whose low word sign-extends negative (`>= 0x8000`) therefore fails the
 * chain's positive bound. */
static int16_t camera_abs_word(int32_t value) {
  uint32_t magnitude = (uint32_t)value;
  if (value < 0) magnitude = 0u - magnitude;
  return (int16_t)(uint16_t)magnitude;
}

/* The two native damp sites (`0x719F5` chain A, `0x71B1B` chain B) share the
 * shape `NEG reg; MOV EDX, 1; MOVSX EAX, reg16; CALL 0x8DC50`. The 16-bit NEG
 * matters only at 0x8000 (`NEG` leaves it 0x8000 and `MOVSX` reads -0x8000). */
static int16_t camera_damp_half(int16_t value) {
  int16_t negated = (int16_t)(uint16_t)(0u - (uint16_t)value);
  return (int16_t)fifa96_camera_shift_step((int32_t)negated, 1);
}

int fifa96_camera_wall_arm(fifa96_camera *cam, fifa96_camera_wall_event *out) {
  int32_t height;      /* dword[0x157750] */
  int16_t abs_x_w;     /* (int16)word[|camX|] */
  int16_t abs_z_w;     /* (int16)word[|camZ|] */
  int16_t height_word;
  int16_t vel_x;
  int16_t vel_z;
  int16_t seed_x = 0;
  int16_t seed_z = 0;
  uint16_t event_height = 0;
  int flag = 0;
  if (!cam || !out) return -FIFA96_ERR_INVALID;
  out->seed_x = 0;
  out->seed_z = 0;
  out->height = 0;
  out->announce = 0;
  /* 0x718A9: the inner word gate (the caller has already passed the outer
   * 0x73B70 bounds check). */
  abs_z_w = camera_abs_word(cam->pos_z);
  abs_x_w = camera_abs_word(cam->pos_x);
  if (!(abs_z_w > 0xB10 || abs_x_w > 0x720)) return 0;
  /* 0x719A0: both live cells zero -> return before the flag/seeds init. */
  if ((int16_t)cam->event_param == 0 && (int16_t)cam->speed == 0) return 0;
  height = cam->pos_y;
  height_word = (int16_t)(uint16_t)height;
  vel_x = cam->vel_x;
  vel_z = cam->vel_z;
  /* Chain A (Z), 0x719CF..0x71A5A. */
  if (abs_z_w >= 0xE74) {
    if (height < 0x24 && camera_abs(cam->prev_z) < 0xE74) {
      /* 0x719F5: the corridor damp; the flag stays clear. */
      vel_z = camera_damp_half(vel_z);
    } else {
      int16_t delta = (int16_t)(uint16_t)(abs_z_w - 0xE74);
      int32_t t = cam->pos_z < 0 ? 0x20 : (int32_t)(int16_t)((delta >> 1) + 0x24);
      if (t >= height) {
        flag = 1;
        seed_z = cam->pos_z > 0 ? -0x180 : 0x180;
        event_height = (uint16_t)((uint16_t)height_word + 0x40u);
      }
    }
  }
  /* Chain B (X), 0x71A5D..0x71AF0; the 0x71AF2 small-X overshoot shares the
   * `0x71B1B` damp. Note the corridor test is the strict `height > 0x24`
   * (chain A uses `>=`) and the fixed 0x20 sits on the positive side. */
  if (abs_x_w >= 0x8BC) {
    if (height > 0x24 || camera_abs(cam->prev_x) >= 0x8BC) {
      int16_t delta = (int16_t)(uint16_t)(abs_x_w - 0x8BC);
      int32_t t = cam->pos_x >= 0 ? 0x20 : (int32_t)(int16_t)((delta >> 1) + 0x24);
      if (t >= height) {
        flag = 1;
        seed_x = cam->pos_x > 0 ? -0x180 : 0x180;
        event_height = (uint16_t)((uint16_t)height_word + 0x40u);
      }
    } else {
      vel_x = camera_damp_half(vel_x);
    }
  } else if (abs_x_w >= 0x6D4 && height < 0x1E &&
             camera_abs(cam->prev_x) < 0x6D4) {
    vel_x = camera_damp_half(vel_x);
  }
  /* 0x71B2C: flag 0 takes the shared 0x71972 tail. */
  if (!flag) {
    /* 0x71972..0x71990: store the register pair, then overwrite the speed word
     * with the CD514 length. No arm/zone clear. */
    cam->vel_x = vel_x;
    cam->vel_z = vel_z;
    cam->speed = (uint16_t)fifa96_entity_hypot((int32_t)vel_x, (int32_t)vel_z);
    return 0;
  }
  /* 0x71B37..0x71B5B: the 0x974DC announce request. `speed <= 5` skips the
   * call entirely (`0x71B3D JLE 0x71B60`); the bucket args are 1/2/3. */
  {
    int16_t entry_speed = (int16_t)cam->speed;
    if (entry_speed > 0xA) {
      out->announce = 3;
    } else if (entry_speed > 8) {
      out->announce = 2;
    } else if (entry_speed > 5) {
      out->announce = 1;
    }
  }
  out->seed_x = seed_x;
  out->seed_z = seed_z;
  out->height = (int16_t)event_height;
  return 1;
}

/* ------------------------------------------------------------------------- *
 * FU-148 §2.1(a)/§6.2 (S4): the FUN_000505D0 pose feed.
 *
 * The behavior blocks live at 0x10896C..0x108B63 (6 x 0x54); their +0x4C
 * pointer arrays (the image-default selection: 0x1590CC team flags are zero)
 * are first-hand `read_memory`: 0x107F1C (blocks 0/3, class 3), 0x1080FC
 * (blocks 1/4, class 1), 0x1082DC (blocks 2/5, class 3), each 8 records of 6
 * dwords. The mode-0x15 fixed record is 0x108714.
 * ------------------------------------------------------------------------- */

/* 0x107F1C, 8 x 6 dwords (blocks 0/3; records 1..7 shared with 0x1080FC and
 * 0x1082DC). Format {x, y, z, yaw, pitch, angle}. */
static const fifa96_camera_pose camera_pose_a0[8] = {
    {-100, 488, 1100, 33900, 3900, 4608},
    {-2727, 740, 2693, 46675, 4749, 4608},
    {-1873, 712, 2013, 51827, 4512, 4608},
    {-2710, 640, -118, 49340, 3931, 4608},
    {-959, 668, -77, 49404, 4251, 4608},
    {-171, 920, 3829, 32320, 4550, 4608},
    {-4, 276, 1502, 32800, 1742, 3545},
    {-83, 320, 3631, 33732, 2203, 3712},
};

/* 0x1080FC (blocks 1/4): record 0 differs, records 1..7 = camera_pose_a0[1..]. */
static const fifa96_camera_pose camera_pose_a1[8] = {
    {-1099, 488, 393, 45508, 5083, 4608},
    {-2727, 740, 2693, 46675, 4749, 4608},
    {-1873, 712, 2013, 51827, 4512, 4608},
    {-2710, 640, -118, 49340, 3931, 4608},
    {-959, 668, -77, 49404, 4251, 4608},
    {-171, 920, 3829, 32320, 4550, 4608},
    {-4, 276, 1502, 32800, 1742, 3545},
    {-83, 320, 3631, 33732, 2203, 3712},
};

/* 0x1082DC (blocks 2/5): record 0 differs, records 1..7 equal. */
static const fifa96_camera_pose camera_pose_a2[8] = {
    {-600, 488, 1100, 37800, 3800, 4608},
    {-2727, 740, 2693, 46675, 4749, 4608},
    {-1873, 712, 2013, 51827, 4512, 4608},
    {-2710, 640, -118, 49340, 3931, 4608},
    {-959, 668, -77, 49404, 4251, 4608},
    {-171, 920, 3829, 32320, 4550, 4608},
    {-4, 276, 1502, 32800, 1742, 3545},
    {-83, 320, 3631, 33732, 2203, 3712},
};

/* 0x108714: the mode-0x15 fixed record. */
static const fifa96_camera_pose camera_pose_mode15 = {
    -66, 4300, 6700, 32768, 5900, 1696,
};

const fifa96_camera_pose_block fifa96_camera_pose_blocks[FIFA96_CAMERA_POSE_BLOCKS] = {
    {3, camera_pose_a0}, {1, camera_pose_a1}, {3, camera_pose_a2},
    {3, camera_pose_a0}, {1, camera_pose_a1}, {3, camera_pose_a2},
};

int fifa96_camera_pose_apply(fifa96_camera *cam, int32_t *yaw, int32_t *pitch,
                             int32_t *view_ratio, const fifa96_camera_pose *pose) {
  if (!cam || !yaw || !pitch || !view_ratio || !pose) return -FIFA96_ERR_INVALID;
  cam->pos_x = pose->x;
  cam->pos_y = pose->y;
  cam->pos_z = pose->z;
  *yaw = pose->yaw;
  *pitch = pose->pitch;
  *view_ratio = pose->angle;
  return FIFA96_OK;
}

/* FUN_000504E0 (record 3|4) and FUN_00050518 (record 1|2) share one predicate
 * (first-hand 0x504E0/0x50518): the "left" record when
 * `(class == 2 && x >= 1) || (class != 2 && x < 0)`, else the "right" one. */
static int camera_pose_left_variant(int32_t x, int32_t class_of) {
  if (class_of == 2) return x >= 1;
  return x < 0;
}

/* Defined after the behavior bodies below (FU-152 §2.8). */
static int camera_pose_handler_arm(fifa96_camera *cam, int32_t *yaw, int32_t *pitch,
                                   int32_t *view_ratio,
                                   const fifa96_camera_pose_args *args);

int fifa96_camera_pose_feed(fifa96_camera *cam, int32_t *yaw, int32_t *pitch,
                            int32_t *view_ratio, const fifa96_camera_pose_args *args) {
  const fifa96_camera_pose_block *block;
  int32_t record;
  if (!cam || !yaw || !pitch || !view_ratio || !args) return -FIFA96_ERR_INVALID;
  if (args->block >= FIFA96_CAMERA_POSE_BLOCKS) return -FIFA96_ERR_INVALID;
  if (args->handler_state != NULL) {
    fifa96_camera_handler_state *st = args->handler_state;
    int32_t slot = st->axis_present
                       ? fifa96_camera_target_slot(st->axis_roster,
                                                   st->axis_side, st->axis_ac2)
                       : -1;
    st->pan_axis_a = fifa96_camera_pose_axis_a(st->track_z - st->hist_z, slot,
                                               st->axis_ac2, &st->axis_latch);
    st->pan_axis_b = st->axis_lead_z;
  }
  block = &fifa96_camera_pose_blocks[args->block];
  switch (args->view_mode) {
    case 1u:
    case 0x12u:
      /* FUN_000505D0 arm 1/0x12: record 0 + the selector/class mirrors. */
      if (fifa96_camera_pose_apply(cam, yaw, pitch, view_ratio, &block->records[0]) !=
          FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      {
        int32_t mirror_yaw = (0x18000 - *yaw) & 0xffff;
        if (args->selector == 1u) {
          if (args->mirror) {
            cam->pos_z = -cam->pos_z;
            *yaw = mirror_yaw;
          }
          if (block->class_of == 2) {
            cam->pos_x = -cam->pos_x;
            *yaw = (-*yaw) & 0xffff;
          }
        } else if (args->selector == 3u) {
          if (args->mirror) {
            cam->pos_z = -cam->pos_z;
            *yaw = mirror_yaw;
          }
        } else if (block->class_of == 4) {
          cam->pos_z = -cam->pos_z;
          *yaw = mirror_yaw;
        }
      }
      return 1;
    case 3u:
    case 4u: {
      /* FUN_000505D0 arms 3/4: record selection via the variant predicates.
       * Legs: the mode-3 replay camera copy/offset/z clamp, the mode-4
       * `[0x109A70+8] < 0` mirror. */
      int left = camera_pose_left_variant(args->variant_x, block->class_of);
      record = args->view_mode == 3u ? (left ? 3 : 4) : (left ? 1 : 2);
      if (fifa96_camera_pose_apply(cam, yaw, pitch, view_ratio, &block->records[record]) !=
          FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      return 1;
    }
    case 6u:
    case 0x10u:
      /* FUN_000505D0 arm 6/0x10: record 7 + the sub < 0 mirror. */
      if (fifa96_camera_pose_apply(cam, yaw, pitch, view_ratio, &block->records[7]) !=
          FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      if (args->sub < 0) {
        cam->pos_z = -cam->pos_z;
        *yaw = (0x18000 - *yaw) & 0xffff;
      }
      return 1;
    case 8u:
      /* FUN_000505D0 arm 8 (default selector): record 6 when sub < 1, else 5.
       * Leg: the selector 1/3 branch and the mirror tails. */
      record = args->sub < 1 ? 6 : 5;
      if (fifa96_camera_pose_apply(cam, yaw, pitch, view_ratio, &block->records[record]) !=
          FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      return 1;
    case 0x15u:
      /* FUN_000505D0 arm 0x15: the fixed 0x108714 record, yaw fold, z negate.
       * Leg: the camera+0x3c / [0x108C38] z clamp. */
      if (fifa96_camera_pose_apply(cam, yaw, pitch, view_ratio, &camera_pose_mode15) !=
          FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      *yaw = (0x18000 - *yaw) & 0xffff;
      cam->pos_z = -cam->pos_z;
      return 1;
    default:
      /* The default arm calls (&0x108B80)[selector] (FU-152 §2.8 P4: the
       * handler bodies are staged through the args). */
      return camera_pose_handler_arm(cam, yaw, pitch, view_ratio, args);
  }
}

/* ------------------------------------------------------------------------- *
 * FU-152 §2.8 (P4): the 0x107508 camera-type mapping and the four 0x108B80
 * handler bodies. The class-0 steady body is complete (FU-351 §12); the
 * class-1/2 bodies are complete in FU-351 §13.
 * ------------------------------------------------------------------------- */

static int32_t camera_clamp_i32(int32_t value, int32_t lo, int32_t hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}

int fifa96_camera_pan_target_step(fifa96_camera_pan_target *target,
                                  int32_t device, int32_t source_105fc4,
                                  int32_t input_bits, int32_t speed,
                                  int32_t phase, int32_t difficulty,
                                  int32_t option_15b680, int32_t camera_z) {
  static const int32_t offsets[9] = {0x12C0, 0x2580, 0x2580, 0x2580, 0x2580,
                                     0x2580, 0x2580, 0x2580, 0};
  int32_t old_x, dx, dz, mode, off, lo, hi, zlo, zhi;
  if (!target) return -FIFA96_ERR_INVALID;
  if (device < 0 || source_105fc4 != 1) return 0;
  old_x = target->x;
  dx = 0;
  dz = 0;
  if ((input_bits & 0x70) == 0) {
    if ((input_bits & 1) != 0) dx = speed;
    else if ((input_bits & 2) != 0) dx = -speed;
    if ((input_bits & 8) != 0) dz = speed;
    else if ((input_bits & 4) != 0) dz = -speed;
  }
  target->x = (int32_t)((uint32_t)target->x + (uint32_t)dx);
  target->z = (int32_t)((uint32_t)target->z + (uint32_t)dz);
  mode = fifa96_camera_view_preset(phase, difficulty, option_15b680);
  off = offsets[mode];
  lo = -0x660;
  hi = 0x660;
  zlo = camera_z - off;
  zhi = camera_z + off;
  if (mode < 5) {
    if (mode < 1) {
      zlo = camera_clamp_i32(zlo, -0x9C0, 0x9C0);
      zhi = camera_clamp_i32(zhi, -0x9C0, 0x9C0);
    } else {
      zlo = camera_clamp_i32(zlo, -0xB10, 0xB10);
      zhi = camera_clamp_i32(zhi, -0xB10, 0xB10);
    }
  } else if (mode == 5) {
    lo = -0x720;
    hi = 0x720;
    zlo = 0;
    zhi = 0xB10;
    if (dx < 0) target->x = -0x720;
    else if (dx > 0) target->x = 0x720;
    if (target->x < 0) old_x = -target->x;
    else old_x = target->x;
    if (old_x != 0x720)
      target->z = target->z < 1 ? -0xB10 : 0xB10;
  } else if (mode == 6) {
    lo = -0x720;
    hi = 0x720;
    zlo = 0;
    zhi = 0xB10;
    if (dx < 0) target->x = -0x720;
    else if (dx > 0) target->x = 0x720;
    target->z = 0xB10;
  } else if (mode == 7) {
    lo = -0x720;
    hi = 0x720;
    zlo = 0;
    zhi = 0xB10;
  } else if (mode == 8) {
    lo = -0x630;
    hi = 0x630;
    zlo = 0;
    zhi = 0x9F0;
    if (old_x < 0) old_x = -old_x;
    if (old_x < 0x420) {
      int32_t az = target->z < 0 ? -target->z : target->z;
      if (az > 0x7AF) {
        if (old_x < 0x420) target->z = target->z < 1 ? -0x7AF : 0x7AF;
        else target->x = target->x < 1 ? -0x420 : 0x420;
      }
    }
  }
  target->x = camera_clamp_i32(target->x, lo, hi);
  target->z = camera_clamp_i32(target->z, zlo, zhi);
  if (target->x <= lo || target->x >= hi) dx = 0;
  if (target->z <= zlo || target->z >= zhi) dz = 0;
  target->dx = camera_clamp_i32(dx, -0xA0, 0xA0);
  target->dz = camera_clamp_i32(dz, -0xA0, 0xA0);
  return 1;
}

/* FUN_0004D5F4: hold the target angle inside the +/-ratio deadband around
 * atan1; param4 == 1 returns the atan2-side half-step. */
static int32_t camera_angle_blend16(int32_t atan1, int32_t atan2,
                                    int32_t ratio, int param4) {
  int32_t d = (int32_t)(((uint32_t)atan2 - (uint32_t)atan1) & 0xFFFFu);
  int32_t half = ratio >> 1;
  if (d > 0x8000) d -= 0x10000;
  if (d > 0) {
    int32_t next = (int32_t)(((uint32_t)atan1 + (uint32_t)half) & 0xFFFFu);
    if (param4 == 1 || d > ratio) return next;
    return (int32_t)(((uint32_t)atan1 + (uint32_t)(d >> 1)) & 0xFFFFu);
  }
  if (param4 == 1)
    return (int32_t)(((uint32_t)atan2 + (uint32_t)half) & 0xFFFFu);
  if (-d > ratio)
    return (int32_t)(((uint32_t)atan1 - (uint32_t)half) & 0xFFFFu);
  return (int32_t)(((uint32_t)atan1 - (uint32_t)((-d) >> 1)) & 0xFFFFu);
}

/* FUN_0004DF34: the shared yaw step. atan1 is the look-at of the staged
 * target (`camera+0x34`), atan2 the look-at of the caller's absolute point
 * (`point_x`, `point_z`); FUN_0004D5F4 blends them, then the native applies
 * the rate ramp, a +/-30 minimum step and the ratio/2+ratio/4 catch-up
 * band. */
static int32_t camera_yaw_delta(const fifa96_camera_handler_state *st,
                                int32_t point_x, int32_t point_z,
                                int32_t rate, int32_t tick) {
  int32_t atan1, atan2, blend, delta, factor, step, band;
  atan1 = (int32_t)(0u - (uint32_t)camera_atan16(st->track_x - st->pos_x,
                                                 st->track_z - st->pos_z)) &
          0xFFFF;
  atan2 = (int32_t)(0u - (uint32_t)camera_atan16(point_x - st->pos_x,
                                                 point_z - st->pos_z)) &
          0xFFFF;
  blend = camera_angle_blend16(atan1, atan2, st->ratio, 0);
  delta = (int32_t)(((uint32_t)blend - (uint32_t)st->yaw) & 0xFFFFu);
  if (delta > 0x8000) delta -= 0x10000;
  if (camera_abs(delta) < 30) return delta;

  factor = fifa96_camera_ramp_factor(tick, rate);
  step = (int32_t)(((int64_t)delta * factor + 0x8000) >> 16);
  if (step < 0 && step > -30) step = -30;
  else if (step > 0 && step < 30) step = 30;

  band = (int32_t)(((uint32_t)atan1 - (uint32_t)st->yaw) & 0xFFFFu);
  if (band > 0x8000) band -= 0x10000;
  band = camera_abs(band) - (st->ratio >> 1) - (st->ratio >> 2);
  if (band > 0) {
    if (step < 0 && step > -band) step = -band;
    else if (step > 0 && step < band) step = band;
  }
  return step;
}

/* The native pan accumulator shared by handlers 0/1/2: FUN_0004E248, the
 * +-block+0x3C clamp, then the camera-relative +-0xB10 band. The caller keeps
 * the mutable block. */
static int32_t camera_pan_update(fifa96_camera_handler_state *st,
                                 fifa96_camera_handler_block *block,
                                 int32_t tick) {
  int32_t delta = fifa96_camera_steady_pan_step(
      st->pan_axis_a, st->pan_axis_b, block->pan, block->const40, st->track_z,
      tick);
  block->pan_step = delta;
  block->pan += delta;
  block->pan = camera_clamp_i32(block->pan, -block->const3c, block->const3c);
  block->pan = camera_clamp_i32(block->pan, -st->track_z - 0xB10,
                                0xB10 - st->track_z);
  return delta;
}

/* The two 32-bit horizon products. `mul` is -0xB for h0 and -0xD for h1. */
static int32_t camera_horizon(int32_t block0, int32_t mul, int32_t add) {
  return (int32_t)(((int32_t)((uint32_t)block0 * (uint32_t)mul)) >> 3) + add;
}

/* FUN_0004C7D0 against the camera position: -atan2(point - eye). */
static int32_t camera_snap_angle(const fifa96_camera_handler_state *st,
                                 int32_t point_x, int32_t point_z) {
  return (int32_t)(0u - (uint32_t)camera_atan16(point_x - st->pos_x,
                                                point_z - st->pos_z)) &
         0xFFFF;
}

/* The 4E3A8 pitch target when block class is 2, and the 4EC9C target when the
 * computed class is 3: the negative track arm halves the track, the
 * non-negative arm scales the horizon difference by 0x23/0x10000. */
int32_t fifa96_camera_pitch_target_a(int32_t track, int32_t horizon_a,
                                     int32_t horizon_b) {
  int32_t base, scaled;
  if (track < 0) return track - ((track >> 1) + horizon_b);
  base = (int32_t)((uint32_t)(horizon_a - horizon_b) * (uint32_t)track);
  scaled = (int32_t)(((int64_t)base * 0x23 + 0x8000) >> 16);
  return track - (horizon_b + scaled);
}

/* The 4E3A8 non-class-2 arm and the 4EC9C class-4 arm. The track <= 0 arm
 * uses the negated track in the same 0x23/0x10000 horizon scale. */
int32_t fifa96_camera_pitch_target_b(int32_t track, int32_t horizon_a,
                                     int32_t horizon_b) {
  int32_t base, scaled;
  if (track <= 0) {
    base = (int32_t)((uint32_t)(horizon_a - horizon_b) * (uint32_t)(-track));
    scaled = (int32_t)(((int64_t)base * 0x23 + 0x8000) >> 16);
    return horizon_b + track + scaled;
  }
  return (horizon_b + track) - (track >> 1);
}

/* FU-152 §2.8: the 0x10896C behavior-block fields (first-hand read_memory;
 * layout per 84-byte block: +0x00/+0x04 targets, +0x08 class, +0xC the
 * class-gate dword, +0x10/+0x18/+0x1C/+0x20/+0x24/+0x28 constants,
 * +0x30/+0x34/+0x38/+0x3C/+0x40 fields and the +0x50 C record). */
const fifa96_camera_behavior_table_entry
    fifa96_camera_behavior_blocks[FIFA96_CAMERA_BEHAVIOR_BLOCKS] = {
        {.target0 = 0x340, .target1 = 0xFA0, .class_of = 3, .field3 = 0,
         .const10 = 0xC8, .const18 = 0x578, .const1c = 32, .const20 = 8,
         .const24 = 8, .const28 = 8, .const30 = 0xEA6, .const34 = 0,
         .const38 = 0, .const3c = 600, .const40 = 80,
         .c0 = 150, .c1 = 150, .c2 = 1000, .c3 = 3800,
         .sub10 = 0, .sub14 = 0,
         .sub18 = 1330, .sub1c = 1750,
         .v1_c0 = 450, .v1_c1 = 450, .v1_c2 = 1600, .v1_c3 = 3430,
         .v1_sub10 = 0, .v1_sub14 = 0, .v1_sub18 = 700, .v1_sub1c = 1150,
         .cell18 = 0x578, .cell1c = 32,
         .pan = 0, .pan_step = 0},
        {.target0 = 0x340, .target1 = 0x1200, .class_of = 1, .field3 = 0,
         .const10 = 0x4B0, .const18 = 0, .const1c = 0, .const20 = 8,
         .const24 = 8, .const28 = 0x20, .const30 = 0x578, .const34 = 0,
         .const38 = 0, .const3c = 600, .const40 = 80,
         .c0 = 600, .c1 = 2400, .c2 = 1500, .c3 = 1500,
         .sub10 = 0x437, .sub14 = 0x720,
         .sub18 = 0, .sub1c = 0,
         .v1_c0 = -200, .v1_c1 = 2100, .v1_c2 = 1600, .v1_c3 = 1600,
         .v1_sub10 = 700, .v1_sub14 = 1150, .v1_sub18 = 0, .v1_sub1c = 0,
         .cell18 = 0, .cell1c = 0,
         .pan = 0, .pan_step = 0},
        {.target0 = 0x340, .target1 = 0xD48, .class_of = 3, .field3 = 0xA7F8,
         .const10 = 0, .const18 = 0x578, .const1c = 20, .const20 = 8,
         .const24 = 8, .const28 = 8, .const30 = 0x1130, .const34 = 0,
         .const38 = 0, .const3c = 550, .const40 = 60,
         .c0 = 900, .c1 = 1900, .c2 = 900, .c3 = 3750,
         .sub10 = 0, .sub14 = 0x5DC,
         .sub18 = 1330, .sub1c = 2000,
         .v1_c0 = 200, .v1_c1 = 1860, .v1_c2 = 1600, .v1_c3 = 3400,
         .v1_sub10 = 0, .v1_sub14 = 1200, .v1_sub18 = 0, .v1_sub1c = 0,
         .cell18 = 0x578, .cell1c = 20,
         .pan = 0, .pan_step = 0},
        {.target0 = 0x340, .target1 = 0xFA0, .class_of = 3, .field3 = 0,
         .const10 = 0xC8, .const18 = 0x578, .const1c = 32, .const20 = 8,
         .const24 = 8, .const28 = 8, .const30 = 0xEA6, .const34 = 0,
         .const38 = 0, .const3c = 600, .const40 = 80,
         .c0 = 150, .c1 = 150, .c2 = 1000, .c3 = 3800,
         .sub10 = 0, .sub14 = 0,
         .sub18 = 1330, .sub1c = 1750,
         .v1_c0 = 450, .v1_c1 = 450, .v1_c2 = 1600, .v1_c3 = 3430,
         .v1_sub10 = 0, .v1_sub14 = 0, .v1_sub18 = 700, .v1_sub1c = 1150,
         .cell18 = 0x578, .cell1c = 32,
         .pan = 0, .pan_step = 0},
        {.target0 = 0x340, .target1 = 0x1200, .class_of = 1, .field3 = 0,
         .const10 = 0x4B0, .const18 = 0, .const1c = 0, .const20 = 8,
         .const24 = 8, .const28 = 0x20, .const30 = 0x578, .const34 = 0,
         .const38 = 0, .const3c = 600, .const40 = 80,
         .c0 = 600, .c1 = 2400, .c2 = 1500, .c3 = 1500,
         .sub10 = 0x437, .sub14 = 0x720,
         .sub18 = 0, .sub1c = 0,
         .v1_c0 = -200, .v1_c1 = 2100, .v1_c2 = 1600, .v1_c3 = 1600,
         .v1_sub10 = 700, .v1_sub14 = 1150, .v1_sub18 = 0, .v1_sub1c = 0,
         .cell18 = 0, .cell1c = 0,
         .pan = 0, .pan_step = 0},
        {.target0 = 0x340, .target1 = 0xD48, .class_of = 3, .field3 = 0xA21C,
         .const10 = 0, .const18 = 0x578, .const1c = 20, .const20 = 8,
         .const24 = 8, .const28 = 8, .const30 = 0x1130, .const34 = 0,
         .const38 = 0, .const3c = 550, .const40 = 60,
         .c0 = 900, .c1 = 1900, .c2 = 900, .c3 = 3750,
         .sub10 = 0, .sub14 = 0x5DC,
         .sub18 = 1330, .sub1c = 2000,
         .v1_c0 = 200, .v1_c1 = 1860, .v1_c2 = 1600, .v1_c3 = 3400,
         .v1_sub10 = 0, .v1_sub14 = 1200, .v1_sub18 = 0, .v1_sub1c = 0,
         .cell18 = 0x578, .cell1c = 20,
         .pan = 0, .pan_step = 0},
};

int fifa96_camera_behavior_block_variant(fifa96_camera_handler_block *out,
                                         uint32_t index, int32_t variant) {
  const fifa96_camera_behavior_table_entry *src;
  if (!out || index >= FIFA96_CAMERA_BEHAVIOR_BLOCKS)
    return -FIFA96_ERR_INVALID;
  src = &fifa96_camera_behavior_blocks[index];
  out->class_of = src->class_of;
  out->target0 = variant == 1 ? 0xB4 : src->target0;   /* 0x107E0C */
  out->target1 = src->target1;
  out->const10 = src->const10;
  out->const18 = src->const18;
  out->const1c = src->const1c;
  out->const20 = src->const20;
  out->const24 = src->const24;
  out->const28 = src->const28;
  out->const30 = src->const30;
  out->const34 = src->const34;
  out->const38 = src->const38;
  out->const3c = src->const3c;
  out->const40 = src->const40;
  out->field3 = src->field3;
  if (variant == 1) {
    out->c0 = src->v1_c0;
    out->c1 = src->v1_c1;
    out->c2 = src->v1_c2;
    out->c3 = src->v1_c3;
    out->sub10 = src->v1_sub10;
    out->sub14 = src->v1_sub14;
    out->sub18 = src->v1_sub18;
    out->sub1c = src->v1_sub1c;
  } else {
    out->c0 = src->c0;
    out->c1 = src->c1;
    out->c2 = src->c2;
    out->c3 = src->c3;
    out->sub10 = src->sub10;
    out->sub14 = src->sub14;
    out->sub18 = src->sub18;
    out->sub1c = src->sub1c;
  }
  out->cell18 = src->cell18;
  out->cell1c = src->cell1c;
  out->pan = src->pan;
  out->pan_step = src->pan_step;
  return FIFA96_OK;
}

int fifa96_camera_behavior_block(fifa96_camera_handler_block *out,
                                 uint32_t index) {
  return fifa96_camera_behavior_block_variant(out, index, 0);
}

int fifa96_camera_type(const fifa96_camera_type_record *records, uint32_t count,
                       uint32_t selector, int32_t *handler, int32_t *behavior) {
  int32_t handler_index, behavior_record, behavior_index;
  if (!records || !handler || !behavior) return -FIFA96_ERR_INVALID;
  if (selector >= count) return -FIFA96_ERR_INVALID;
  handler_index = records[selector].handler;
  if (handler_index < 0) handler_index = 0;          /* the native clamp */
  behavior_record = records[selector].handler;
  if (behavior_record == 3) behavior_record = 5;     /* the native 3 -> 5 remap */
  if (behavior_record < 0 || (uint32_t)behavior_record >= count) {
    /* The native indexes the record table blindly here; for the selector-3/5
     * records that read is the degenerate `[0x108B60]` u32 (slice leg 8), so
     * the engine clamps to record 0. */
    behavior_index = 0;
  } else {
    behavior_index = records[behavior_record].behavior;
  }
  if (behavior_index < 0) behavior_index = 0;        /* replay records carry -1 */
  *handler = handler_index;
  *behavior = behavior_index;
  return FIFA96_OK;
}

int fifa96_camera_behavior_steady(fifa96_camera_handler_state *st,
                                  fifa96_camera_handler_block *block,
                                  int32_t tick,
                                  fifa96_camera_handler_out *out) {
  int32_t pan_delta, mode, lo, hi, hist_off, tx, atx, x_off, point_z;
  int32_t yaw_delta, target_y, diff, factor, step;
  if (!st || !block || !out) return -FIFA96_ERR_INVALID;

  /* 0x4E84A..0x4E8C8: the pan/roll block. The view driver passes (0,0); the
   * returned step is stored at +0x38, added to +0x34, clamped to the +-0x3C
   * pad and then to the camera-relative +-0xB10 band. The same step moves the
   * mutable +0x18 cell, clamped to the +0x50 record's +0x18/+0x1C. */
  pan_delta = camera_pan_update(st, block, tick);
  block->cell1c = pan_delta;
  block->cell18 += pan_delta;
  if (block->cell18 < block->sub18) block->cell18 = block->sub18;
  else if (block->cell18 > block->sub1c) block->cell18 = block->sub1c;

  /* 0x4E8CB..0x4E92D: the z slide. Class 4 takes mode -1 with lo=-C[3],
   * hi=C[2] and hist_offset=+cell18; the other classes take mode 1 with
   * lo=-C[2], hi=C[3] and hist_offset=-cell18. */
  if (block->class_of == 4) {
    mode = -1;
    lo = -block->c3;
    hi = block->c2;
    hist_off = block->cell18;
  } else {
    mode = 1;
    lo = -block->c2;
    hi = block->c3;
    hist_off = -block->cell18;
  }
  st->pos_z = fifa96_camera_slide(st->pos_z, st->track_z, st->hist_z, mode,
                                  lo, hi, hist_off, block->const28, tick);

  /* 0x4E930..0x4E97D: the x slide, mode 2 with the sign-adjusted target. */
  tx = st->track_x;
  atx = camera_abs(tx);
  if (atx < block->const10) x_off = tx;
  else x_off = tx >= 0 ? -block->const10 : block->const10;
  st->pos_x = fifa96_camera_slide(st->pos_x, tx, st->hist_x, 2, -block->c0,
                                  block->c1, x_off, block->const28, tick);

  /* 0x4E97D..0x4E9A2: the FUN_0004DF34 yaw step. */
  point_z = block->class_of == 4 ? 0xB10 : -0xB10;
  yaw_delta = camera_yaw_delta(st, 0, point_z, block->const24, tick);
  st->yaw = (int32_t)(((uint32_t)st->yaw + (uint32_t)yaw_delta) & 0xFFFFu);

  /* 0x4E9A2..0x4EA9B: the class-3 yaw snap / non-class-3 mirror. */
  st->yaw = fifa96_camera_yaw_snap(block->class_of, st->pos_x, st->pos_z,
                                   st->track_z, st->yaw, st->ratio);

  /* 0x4EA9B..0x4EACF: the native horizon is built from block[0], not a random
   * argument: (-11 * block[0]) >> 3 + 0xE09 and the -13 sibling. The product
   * is the native 32-bit wrap. */
  out->horizon_a = camera_horizon(block->target0, -0xB, 0xE09);
  out->horizon_b = camera_horizon(block->target0, -0xD, 0xAA1);

  /* 0x4EB71..0x4EBE7: the pitch ground point is
   * `[0x107DC8]+0x18 + (class 4 ? +block[0xC] : -block[0xC])`, clamped
   * +-0x1620. FUN_0004E05C writes the +0x64 delta; the handler adds it to
   * the +0x5C pitch cell. */
  point_z = st->pos_z + (block->class_of == 4 ? block->const30
                                              : -block->const30);
  point_z = camera_clamp_i32(point_z, -0x1620, 0x1620);
  diff = camera_pitch_delta(st, st->track_x, 0, point_z, block->const20,
                            block->target1, tick);
  st->pitch = (int32_t)(((uint32_t)st->pitch + (uint32_t)diff) & 0xFFFFu);

  /* 0x4EBE7..0x4EC5A: the y ease. The `block[0] + 100` slope is taken only
   * while block[0] < 300 and camera+0x18 > 0xB10; both arms use the native
   * FUN_0004D668 rate 8 (the first arm's long division reduces to the same
   * tick*0x10000/120 Q16 factor). */
  target_y = block->target0;
  if (block->target0 < 300 && st->pos_z > 0xB10) target_y += 100;
  diff = target_y - st->pos_y;
  if (diff != 0) {
    factor = fifa96_camera_ramp_factor(tick, 8);
    step = (int32_t)(((int64_t)diff * factor + 0x8000) >> 16);
    st->pos_y += step;
  }

  /* 0x4EC5D..0x4EC87: the ratio ease toward block+0x04 at rate 8. */
  diff = block->target1 - st->ratio;
  if (diff != 0) {
    factor = fifa96_camera_ramp_factor(tick, 8);
    step = (int32_t)(((int64_t)diff * factor + 0x8000) >> 16);
    st->ratio += step;
  }

  /* 0x4EC8A..0x4EC92: the three-dword target history copy. */
  st->hist_x = st->track_x;
  st->hist_y = st->track_y;
  st->hist_z = st->track_z;
  return FIFA96_OK;
}

/* 0x4E3A8 0x4E4A0..0x4E60E: the class-1/non-class-1 yaw snap. The point is
 * the fixed +-0x720 with z = camera+0x18 -/+ 0x4B0; only the outer side of
 * the z window is clamped. The four arms differ in the ratio sign and the
 * 0x8000 diff test. */
void fifa96_camera_sidetrack_snap(fifa96_camera_handler_state *st,
                                  const fifa96_camera_handler_block *block) {
  int32_t point_x, point_z, angle, target, diff;
  if (block->class_of == 1) {
    if (st->track_x >= -0x360) return;
    point_x = -0x720;
    if (st->yaw > 0xC000 || st->yaw < 0x4000) {
      point_z = st->pos_z + 0x4B0;
      if (point_z > 0xB10) point_z = 0xB10;
      angle = camera_snap_angle(st, point_x, point_z);
      target = (int32_t)(((uint32_t)angle - (uint32_t)st->ratio) & 0xFFFFu);
      diff = (int32_t)(((uint32_t)st->yaw - (uint32_t)target) & 0xFFFFu);
      if (diff < 0x8000) st->yaw = target;
    } else {
      point_z = st->pos_z - 0x4B0;
      if (point_z < -0xB10) point_z = -0xB10;
      angle = camera_snap_angle(st, point_x, point_z);
      target = (int32_t)(((uint32_t)angle + (uint32_t)st->ratio) & 0xFFFFu);
      diff = (int32_t)(((uint32_t)st->yaw - (uint32_t)target) & 0xFFFFu);
      if (diff > 0x8000) st->yaw = target;
    }
  } else {
    if (st->track_x >= 0x360) return;
    point_x = 0x720;
    if (st->yaw > 0xC000 || st->yaw < 0x4000) {
      point_z = st->pos_z + 0x4B0;
      if (point_z > 0xB10) point_z = 0xB10;
      angle = camera_snap_angle(st, point_x, point_z);
      target = (int32_t)(((uint32_t)angle + (uint32_t)st->ratio) & 0xFFFFu);
      diff = (int32_t)(((uint32_t)st->yaw - (uint32_t)target) & 0xFFFFu);
      if (diff > 0x8000) st->yaw = target;
    } else {
      point_z = st->pos_z - 0x4B0;
      if (point_z < -0xB10) point_z = -0xB10;
      angle = camera_snap_angle(st, point_x, point_z);
      target = (int32_t)(((uint32_t)angle - (uint32_t)st->ratio) & 0xFFFFu);
      diff = (int32_t)(((uint32_t)st->yaw - (uint32_t)target) & 0xFFFFu);
      if (diff < 0x8000) st->yaw = target;
    }
  }
}

/* 0x4EC9C 0x4EEF2..0x4F00E: the staged class-3/class-4 yaw snap. The point
 * is (0x720,0xB10) or (-0x720,-0xB10); class 3 and class 4 swap the ratio
 * sign on each side of the 0x2000/0xA000 band. */
void fifa96_camera_staged_snap(fifa96_camera_handler_state *st,
                               int32_t class_of) {
  int32_t angle, target, diff;
  int32_t outside = st->yaw > 0xA000 || st->yaw < 0x2000;
  if (class_of == 3) {
    if (outside) {
      angle = camera_snap_angle(st, 0x720, 0xB10);
      target = (int32_t)(((uint32_t)angle -
                          (uint32_t)(st->ratio >> 1)) & 0xFFFFu);
      diff = (int32_t)(((uint32_t)st->yaw - (uint32_t)target) & 0xFFFFu);
      if (diff < 0x8000) st->yaw = target;
    } else {
      angle = camera_snap_angle(st, -0x720, -0xB10);
      target = (int32_t)(((uint32_t)angle +
                          (uint32_t)(st->ratio >> 1)) & 0xFFFFu);
      diff = (int32_t)(((uint32_t)st->yaw - (uint32_t)target) & 0xFFFFu);
      if (diff > 0x8000) st->yaw = target;
    }
  } else {
    if (outside) {
      angle = camera_snap_angle(st, 0x720, 0xB10);
      target = (int32_t)(((uint32_t)angle +
                          (uint32_t)(st->ratio >> 1)) & 0xFFFFu);
      diff = (int32_t)(((uint32_t)st->yaw - (uint32_t)target) & 0xFFFFu);
      if (diff > 0x8000) st->yaw = target;
    } else {
      angle = camera_snap_angle(st, -0x720, -0xB10);
      target = (int32_t)(((uint32_t)angle -
                          (uint32_t)(st->ratio >> 1)) & 0xFFFFu);
      diff = (int32_t)(((uint32_t)st->yaw - (uint32_t)target) & 0xFFFFu);
      if (diff < 0x8000) st->yaw = target;
    }
  }
}

int fifa96_camera_behavior_sidetrack(fifa96_camera_handler_state *st,
                                     fifa96_camera_handler_block *block,
                                     int32_t tick, fifa96_camera_handler_out *out) {
  int32_t offset, point_x, yaw_delta, pitch_target, point_y, pitch_delta;
  int32_t diff, factor, step;
  if (!st || !block || !out) return -FIFA96_ERR_INVALID;

  /* 0x4E3BE..0x4E470: pan step, +-block[0xF] clamp, camera-relative band. */
  camera_pan_update(st, block, tick);

  /* 0x4E412..0x4E46A: the yaw point x is the pan offset projected onto the
   * staged target x, then the class-2 +0x68 / else -0x68 bias. The native
   * multiplies only the low 32 bits before the IDIV. */
  offset = block->pan < 0 ? -0xB10 - st->track_z : 0xB10 - st->track_z;
  point_x = 0;
  if (offset != 0) {
    int32_t product = (int32_t)((uint32_t)(offset - block->pan) *
                                (uint32_t)st->track_x);
    point_x = product / offset;
  }
  point_x += block->class_of == 2 ? 0x68 : -0x68;

  /* 0x4E470..0x4E49D: yaw step at (point_x, 0, track_z + pan). */
  yaw_delta = camera_yaw_delta(st, point_x, st->track_z + block->pan,
                               block->const24, tick);
  st->yaw = (int32_t)(((uint32_t)st->yaw + (uint32_t)yaw_delta) & 0xFFFFu);
  fifa96_camera_sidetrack_snap(st, block);

  /* 0x4E60E..0x4E63D: the horizon pair from block[0]. */
  out->horizon_a = camera_horizon(block->target0, -0xB, 0xE3B);
  out->horizon_b = camera_horizon(block->target0, -0xD, 0xAD3);

  /* 0x4E642..0x4E700: the pitch target split by class 2 vs else, +-0xE40. */
  pitch_target = block->class_of == 2
                     ? fifa96_camera_pitch_target_a(st->track_x,
                                                    out->horizon_a,
                                                    out->horizon_b)
                     : fifa96_camera_pitch_target_b(st->track_x,
                                                    out->horizon_a,
                                                    out->horizon_b);
  pitch_target = camera_clamp_i32(pitch_target, -0xE40, 0xE40);
  point_y = camera_abs(pitch_target) >> 5;

  /* 0x4E700..0x4E738: pitch step with point (target, |target|/32, track_z). */
  pitch_delta = camera_pitch_delta(st, pitch_target, point_y, st->track_z,
                                   block->const20, block->target1, tick);
  st->pitch = (int32_t)(((uint32_t)st->pitch + (uint32_t)pitch_delta) & 0xFFFFu);

  /* 0x4E740..0x4E79B: the native stores the sub-record +0x10 in block+0x10,
   * then runs the x slide. Class 2 uses mode 1 with the positive C bounds;
   * every other class uses mode -1 with the reversed bounds. */
  block->const10 = block->sub10;
  if (block->class_of == 2) {
    st->pos_x = fifa96_camera_slide(st->pos_x, st->track_x, st->hist_x, 1,
                                    block->c0, block->c1, -block->sub10,
                                    block->const28, tick);
  } else {
    st->pos_x = fifa96_camera_slide(st->pos_x, st->track_x, st->hist_x, -1,
                                    -block->c1, -block->c0, block->sub10,
                                    block->const28, tick);
  }

  /* 0x4E79E..0x4E7C6: z slide, mode 2 with the mutable +0x18 offset. */
  st->pos_z = fifa96_camera_slide(st->pos_z, st->track_z, st->hist_z, 2,
                                  -block->c2, block->c3, block->cell18,
                                  block->const28, tick);

  /* 0x4E7C9..0x4E820: the y and ratio eases, both at native rate 8. */
  diff = block->target0 - st->pos_y;
  if (diff != 0) {
    factor = fifa96_camera_ramp_factor(tick, 8);
    step = (int32_t)(((int64_t)diff * factor + 0x8000) >> 16);
    st->pos_y += step;
  }
  diff = block->target1 - st->ratio;
  if (diff != 0) {
    factor = fifa96_camera_ramp_factor(tick, 8);
    step = (int32_t)(((int64_t)diff * factor + 0x8000) >> 16);
    st->ratio += step;
  }

  /* 0x4E823..0x4E82B: the target-history copy. */
  st->hist_x = st->track_x;
  st->hist_y = st->track_y;
  st->hist_z = st->track_z;
  return FIFA96_OK;
}

int fifa96_camera_behavior_staged(fifa96_camera_handler_state *st,
                                  fifa96_camera_handler_block *block,
                                  int32_t tick, fifa96_camera_handler_out *out) {
  int32_t sin16, cos16, scaled, pan_delta, class_of, yaw_delta, pitch_target;
  int32_t pitch_delta, diff, factor, step;
  if (!st || !block || !out) return -FIFA96_ERR_INVALID;

  /* 0x4ECB6..0x4ED25: FUN_0004A1A60's sin/cos scale of the +0x50 record
   * +0x14, stored as the abs value in block+0x10 (cos) and the sub-record
   * +0x18 (sin). */
  if (fifa96_projection_sincos(block->field3, &sin16, &cos16) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  scaled = (int32_t)(((int64_t)block->sub14 * sin16 + 0x8000) >> 16);
  block->sub18 = camera_abs(scaled);
  scaled = (int32_t)(((int64_t)block->sub14 * cos16 + 0x8000) >> 16);
  block->const10 = camera_abs(scaled);

  /* 0x4ED78..0x4EDF8: pan step plus the mutable +0x18 cell clamp. */
  pan_delta = camera_pan_update(st, block, tick);
  block->cell1c = pan_delta;
  block->cell18 += pan_delta;
  block->cell18 = camera_clamp_i32(block->cell18, block->sub18, block->sub1c);

  /* 0x4EDF8..0x4EE32: the block[3] class gate; the native stores it. */
  class_of = (block->field3 < 0x4000 || block->field3 > 0xC000) ? 4 : 3;
  block->class_of = class_of;

  /* 0x4EE09..0x4EE63: z slide. Class 4 takes the reversed bounds. */
  if (class_of == 4) {
    st->pos_z = fifa96_camera_slide(st->pos_z, st->track_z, st->hist_z, -1,
                                    -block->c3, block->c2, block->cell18,
                                    block->const28, tick);
  } else {
    st->pos_z = fifa96_camera_slide(st->pos_z, st->track_z, st->hist_z, 1,
                                    -block->c2, block->c3, block->cell18,
                                    block->const28, tick);
  }

  /* 0x4EE66..0x4EEBA: x slide split at block[3] 0x8000. */
  if (block->field3 < 0x8000) {
    st->pos_x = fifa96_camera_slide(st->pos_x, st->track_x, st->hist_x, 2,
                                    block->c0, block->c1, block->const10,
                                    block->const28, tick);
  } else {
    st->pos_x = fifa96_camera_slide(st->pos_x, st->track_x, st->hist_x, 2,
                                    -block->c1, -block->c0, block->const10,
                                    block->const28, tick);
  }

  /* 0x4EEBD..0x4EF75: yaw step at (0, 0, track_z + pan), then the snap. */
  yaw_delta = camera_yaw_delta(st, 0, st->track_z + block->pan,
                               block->const24, tick);
  st->yaw = (int32_t)(((uint32_t)st->yaw + (uint32_t)yaw_delta) & 0xFFFFu);
  fifa96_camera_staged_snap(st, class_of);

  /* 0x4F00E..0x4F03D: the horizon pair from block[0]. */
  out->horizon_a = camera_horizon(block->target0, -0xB, 0xE3B);
  out->horizon_b = camera_horizon(block->target0, -0xD, 0xAD3);

  /* 0x4F042..0x4F12E: the pitch target split by computed class, +-0x1620. */
  pitch_target = class_of == 3
                     ? fifa96_camera_pitch_target_a(st->track_z,
                                                    out->horizon_a,
                                                    out->horizon_b)
                     : fifa96_camera_pitch_target_b(st->track_z,
                                                    out->horizon_a,
                                                    out->horizon_b);
  pitch_target = camera_clamp_i32(pitch_target, -0x1620, 0x1620);

  /* 0x4F12E..0x4F15A: pitch step with point (track_x, 0, target). */
  pitch_delta = camera_pitch_delta(st, st->track_x, 0, pitch_target,
                                   block->const20, block->target1, tick);
  st->pitch = (int32_t)(((uint32_t)st->pitch + (uint32_t)pitch_delta) & 0xFFFFu);

  /* 0x4F15D..0x4F1B4: the y and ratio eases, both at native rate 8. */
  diff = block->target0 - st->pos_y;
  if (diff != 0) {
    factor = fifa96_camera_ramp_factor(tick, 8);
    step = (int32_t)(((int64_t)diff * factor + 0x8000) >> 16);
    st->pos_y += step;
  }
  diff = block->target1 - st->ratio;
  if (diff != 0) {
    factor = fifa96_camera_ramp_factor(tick, 8);
    step = (int32_t)(((int64_t)diff * factor + 0x8000) >> 16);
    st->ratio += step;
  }

  /* 0x4F1B7..0x4F1BF: the target-history copy. */
  st->hist_x = st->track_x;
  st->hist_y = st->track_y;
  st->hist_z = st->track_z;
  out->class_of = class_of;
  return FIFA96_OK;
}

/* The 16-bit angle form of FUN_000A2A10: the entity helper returns the
 * 0x400-circle angle (0x100 = 90 deg), and the camera's 0x10000-circle angle
 * is that value shifted by 0x40 (45 deg = 0x80 -> 0x2000). */
static int32_t camera_atan16(int32_t x, int32_t z) {
  int32_t angle = 0;
  if (fifa96_entity_angle(x, z, &angle) != FIFA96_OK) angle = 0;
  return (int32_t)(((uint32_t)angle * 0x40u) & 0xFFFFu);
}

static int32_t camera_mul16(int32_t a, int32_t b) {
  return (int32_t)(((int64_t)a * (int64_t)b) >> 16);
}

/* FUN_0004C77C (`0x4C77C..0x4C7CF`): pitch is `-FUN_000A2A10(dy, horiz)`
 * with `horiz = |dx| + |dz| - min(|dx|,|dz|)/2`. */
static int32_t camera_atan_pitch16(int32_t dx, int32_t dy, int32_t dz) {
  int32_t adx = camera_abs(dx);
  int32_t adz = camera_abs(dz);
  int32_t mn = adx < adz ? adx : adz;
  int32_t horiz = adx + adz - (mn >> 1);
  return (int32_t)(0u - (uint32_t)camera_atan16(dy, horiz)) & 0xFFFF;
}

/* FUN_0004E05C (`0x4E05C..0x4E222`, 153 instructions): the shared pitch
 * delta. The first two looks use the staged target before/after the native
 * `camera+0x38 = 0` write; the third uses the caller's absolute
 * `(point_x, point_y, point_z)` ground point. The D5F4 call passes param4=1;
 * the ratio correction and the low-altitude ground-point guard follow the
 * native order. */
static int32_t camera_pitch_delta(fifa96_camera_handler_state *st,
                                  int32_t point_x, int32_t point_y,
                                  int32_t point_z, int32_t rate,
                                  int32_t target1, int32_t tick) {
  int32_t dx = st->track_x - st->pos_x;
  int32_t dz = st->track_z - st->pos_z;
  int32_t angle1, angle2, angle3, atan1, blend, correction, prod, delta;
  int32_t floor_y, ground_y, point_z2, angle4, factor, step;

  angle1 = camera_atan_pitch16(dx, st->track_y - st->pos_y, dz);
  st->track_y = 0;   /* 0x4E091: the native permanently zeroes camera+0x38 */
  angle2 = camera_atan_pitch16(dx, -st->pos_y, dz);
  atan1 = (int32_t)(((uint32_t)angle1 + (uint32_t)angle2) & 0xFFFFu) >> 1;
  atan1 += st->ratio >> 3;

  angle3 = camera_atan_pitch16(point_x - st->pos_x, point_y - st->pos_y,
                                point_z - st->pos_z);
  blend = camera_angle_blend16(
      atan1, angle3, (int32_t)(((int64_t)st->ratio * 7) >> 2), 1);

  if (st->ratio != target1 && st->ratio != 0) {
    /* A2AD8/A2AEE: the product is truncated to 32 bits before the IDIV. */
    prod = (int32_t)((uint32_t)(st->ratio - target1) *
                     (uint32_t)(atan1 - angle3));
    correction = prod / st->ratio;
    blend = (int32_t)(((uint32_t)blend - (uint32_t)correction) & 0xFFFFu);
  }

  /* 0x4E109..: [0x107DD4] == 1 (the FUN_0004D04C variant) selects 0x2EE and
   * point y = camera_y << 5; variant 0 is point y = 0x140 and the
   * [0x108BB4] = 0x140 floor. */
  if (st->view_variant == 1) {
    floor_y = 0x2EE;
    ground_y = (int32_t)((uint32_t)st->pos_y << 5);
  } else {
    floor_y = 0x140;
    ground_y = 0x140;
  }
  if (st->pos_y < floor_y) {
    point_z2 = st->pos_z > 0 ? 0xB10 : -0xB10;
    angle4 = camera_atan_pitch16(-st->pos_x, ground_y - st->pos_y,
                                 point_z2 - st->pos_z);
    if (angle4 > 0x8000) {
      blend = st->ratio / 2;
    } else if ((blend - angle4) > st->ratio) {
      blend = st->ratio + angle4;
    }
    blend &= 0xFFFF;
  }

  delta = (int32_t)(((uint32_t)blend - (uint32_t)st->pitch) & 0xFFFFu);
  if (delta > 0x8000) delta -= 0x10000;
  if (camera_abs(delta) < 30) return delta;

  factor = fifa96_camera_ramp_factor(tick, rate);
  step = (int32_t)(((int64_t)delta * factor + 0x8000) >> 16);
  if (step < 0 && step > -30) step = -30;
  else if (step > 0 && step < 30) step = 30;
  return step;
}

/* FUN_0004DB38's pitch snap: (0x2000,0x8000) -> 0x2000 and
 * (0x8000,0xF618) -> 0xF618. */
static int32_t camera_action_pitch_snap(int32_t pitch) {
  if (pitch > 0x8000 && pitch < 0xF618) return 0xF618;
  if (pitch < 0x8000 && pitch > 0x2000) return 0x2000;
  return pitch;
}

/* The native slew rate is Q16: `floor(tick * 0x10000 / 30)` for yaw and
 * `/ 60` for pitch. 0x4DCCA/0x4DD2C build it with `SHL EAX,0x10` plus the
 * two remainder divisions; the shift is 32-bit, so a non-native tick that
 * wraps it (e.g. 0x3C0000) yields zero. */
static int32_t camera_action_slew_factor(int32_t tick, int32_t divisor) {
  int32_t shifted;
  if (tick <= 0) return 0;
  shifted = (int32_t)((uint32_t)tick << 16);
  return (int32_t)(((int64_t)shifted * 0x10000) / divisor);
}

static int32_t camera_action_slew(int32_t current, int32_t target,
                                  int32_t factor) {
  int32_t delta = (target - current) & 0xFFFF;
  if (delta > 0x8000) delta -= 0x10000;
  return (int32_t)(((uint32_t)current +
                    (uint32_t)(int32_t)(((int64_t)delta * factor + 0x8000) >>
                                        16)) &
                   0xFFFFu);
}

/* FUN_0004C7D0/77C: yaw is -atan2(dx,dz); pitch uses the native octagonal
 * horizontal range |dx|+|dz|-min/2. The caller applies the first-look pitch
 * snap; the post-clamp look-at at 0x4DC89 uses the raw pitch. */
static void camera_action_look_at(const fifa96_camera_handler_state *st,
                                  int32_t *yaw, int32_t *pitch) {
  int32_t dx = st->track_x - st->pos_x;
  int32_t dy = st->track_y - st->pos_y;
  int32_t dz = st->track_z - st->pos_z;
  int32_t adx = camera_abs(dx);
  int32_t adz = camera_abs(dz);
  int32_t mn = adx < adz ? adx : adz;
  int32_t horiz = adx + adz - (mn >> 1);
  *yaw = (int32_t)(0u - (uint32_t)camera_atan16(dx, dz)) & 0xFFFF;
  *pitch = (int32_t)(0u - (uint32_t)camera_atan16(dy, horiz)) & 0xFFFF;
}

int fifa96_camera_behavior_action(fifa96_camera_handler_state *st, int live,
                                  int32_t speed, int32_t tick,
                                  fifa96_camera_handler_out *out) {
  int32_t target_yaw, target_pitch, sin_y, cos_y, sin_p, cos_p;
  int32_t off_x, off_y, off_z, new_y, factor;
  if (!st || !out) return -FIFA96_ERR_INVALID;
  /* 0x4DB38: `if (FUN_0006400C() == 0) param_4 += 100;` (faster when not
   * live). The live Y floor is [0x108B98] = 100; replay uses
   * [0x108B9C] = 120. */
  if (!live) speed += 100;
  out->speed = speed;

  /* First look-at from the old eye (0x4DB6C..0x4DB81), then the target pitch
   * snap before the offset is built. */
  camera_action_look_at(st, &target_yaw, &target_pitch);
  target_pitch = camera_action_pitch_snap(target_pitch);

  /* FUN_0004C7F8: X(pitch) * Y(-yaw) applied to the native vector
   * {0,0,speed}. The resulting offset is (off_x, off_y, off_z); the camera
   * is target - offset. */
  if (fifa96_projection_sincos(target_yaw, &sin_y, &cos_y) != FIFA96_OK ||
      fifa96_projection_sincos(target_pitch, &sin_p, &cos_p) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  off_x = (int32_t)(((int64_t)speed *
                     camera_mul16(-cos_p, sin_y)) >> 16);
  off_y = (int32_t)(((int64_t)speed * -sin_p) >> 16);
  off_z = (int32_t)(((int64_t)speed *
                     camera_mul16(cos_p, cos_y)) >> 16);

  /* 0x4DBCA..0x4DBF3: when the old eye is at or outside any clamp bound, the
   * native scales the Y offset by 1/8 before the position update. */
  if (st->pos_x <= -0x420 || st->pos_x >= 0x420 ||
      st->pos_z <= -0x810 || st->pos_z >= 0x810)
    off_y >>= 3;

  /* Position: x/z snap to target - offset and clamp; y eases down by 1/8
   * when the new target is below the current Y, else snaps. */
  st->pos_x = st->track_x - off_x;
  st->pos_z = st->track_z - off_z;
  new_y = st->track_y - off_y;
  if (new_y < st->pos_y)
    st->pos_y += (new_y - st->pos_y) >> 3;
  else
    st->pos_y = new_y;
  if (st->pos_x < -0x420) st->pos_x = -0x420;
  else if (st->pos_x > 0x420) st->pos_x = 0x420;
  if (st->pos_z < -0x810) st->pos_z = -0x810;
  else if (st->pos_z > 0x810) st->pos_z = 0x810;
  if (st->pos_y < (live ? 100 : 120)) st->pos_y = (live ? 100 : 120);

  /* 0x4DC82..0x4DC96: the native re-runs the look-at from the clamped eye
   * before the yaw/pitch slew; this target pitch is not snapped. */
  camera_action_look_at(st, &target_yaw, &target_pitch);
  factor = camera_action_slew_factor(tick, FIFA96_CAMERA_ACTION_YAW_DIVISOR);
  st->yaw = camera_action_slew(st->yaw, target_yaw, factor);
  factor = camera_action_slew_factor(tick, FIFA96_CAMERA_ACTION_PITCH_DIVISOR);
  st->pitch = camera_action_slew(st->pitch, target_pitch, factor);
  st->pitch = camera_action_pitch_snap(st->pitch);

  /* 0x4DD95..0x4DD9D: camera +0x40/44/48 = the staged target triple. */
  st->hist_x = st->track_x;
  st->hist_y = st->track_y;
  st->hist_z = st->track_z;
  return FIFA96_OK;
}

int fifa96_camera_classify(const int32_t point[3], uint16_t *flags) {
  int32_t mag_z;
  int16_t z_word;
  int32_t threshold;
  int16_t edge;
  if (!point || !flags) return -FIFA96_ERR_INVALID;
  /* FUN_00070074 first-hand: iVar3 = |z| read as a signed short. */
  mag_z = point[2] < 0 ? -point[2] : point[2];
  z_word = (int16_t)mag_z;
  uint16_t result;
  if (z_word < 0xB10) {
    result = 8;
  } else if (z_word < 0xB90) {
    result = 0;
  } else {
    result = 4;
  }
  if (point[0] < -0xD0) {
    result |= 1;
  } else if (point[0] > 0xCF) {
    result |= 2;
  }
  edge = (int16_t)(z_word - 0xB10);
  if (edge < 0x31) {
    threshold = 0xA0;
  } else {
    threshold = 0xA0 - (edge - 0x30);
  }
  if (threshold < point[1]) result |= 0x10;
  *flags = result;
  return result == 0;
}

void fifa96_camera_pan_band(int32_t height, int32_t *class_of, int32_t *param) {
  if (!class_of || !param) return;
  if (height < 0x29) {
    *class_of = 1;
    *param = height * 2 + 0x14;
  } else if (height < 0x65) {
    *class_of = 2;
    *param = (height + 0x50) / 2;
  } else if (height < 0x12D) {
    *class_of = 3;
    *param = (height + 100) / 4;
  } else {
    *class_of = 3;
    *param = 100;
  }
}

int fifa96_camera_rate_event(fifa96_camera *cam, int32_t rate_x, int32_t rate_z,
                             int32_t height) {
  if (!cam) return -FIFA96_ERR_INVALID;
  if (height <= 0xF0) return 0;
  cam->vel_x = (int16_t)camera_clamp_i32(rate_x * 15, -15, 15);
  cam->vel_z = (int16_t)camera_clamp_i32(rate_z * 15, -15, 15);
  cam->speed = (uint16_t)(int16_t)fifa96_entity_distance(cam->vel_x, cam->vel_z);
  return 1;
}

int fifa96_camera_replay_select(uint32_t index, int32_t *kind, int32_t *sub) {
  if (!kind || !sub) return -FIFA96_ERR_INVALID;
  switch (index) {
    case 0:
      *kind = FIFA96_CAMERA_REPLAY_POSE;
      *sub = 0;
      return 1;
    case 1:
    case 2:
    case 3:
    case 6:
      *kind = FIFA96_CAMERA_REPLAY_VIEW;
      *sub = index == 1 ? 5 : (index == 2 ? 6 : (index == 3 ? 7 : 8));
      return 1;
    case 4:
      *kind = FIFA96_CAMERA_REPLAY_ACTION;
      *sub = 0;
      return 1;
    case 5:
      *kind = FIFA96_CAMERA_REPLAY_BALL;
      *sub = 0;
      return 1;
    default:
      return 0;   /* the native default arm only records the index */
  }
}

static int32_t camera_mul32(int32_t a, int32_t b) {
  return (int32_t)((uint32_t)a * (uint32_t)b);
}

static int32_t camera_div32(int32_t num, int32_t den) {
  return den == 0 ? 0 : (int32_t)((int64_t)num / (int64_t)den);
}

int32_t fifa96_camera_replay_cancel(fifa96_camera_replay_input *input) {
  int32_t nibble;
  if (input == NULL) return 0;
  if (input->latch != 0) return 0;
  nibble = input->input & 0xF0;
  if (nibble != 0) input->latch = 5;
  return nibble;
}

int32_t fifa96_camera_replay_ease(int32_t delta, int32_t in_time,
                                  int32_t out_time, int32_t elapsed) {
  int32_t num, den, t, q;
  if (elapsed < in_time) {
    num = camera_mul32(delta, camera_mul32(elapsed, elapsed));
    den = camera_mul32(2, camera_mul32(out_time, in_time));
    return camera_div32(num, den);
  }
  if (out_time < elapsed) {
    t = (int32_t)((uint32_t)out_time + (uint32_t)in_time -
                  (uint32_t)elapsed);
    num = camera_mul32(delta, camera_mul32(t, t));
    den = camera_mul32(2, camera_mul32(out_time, in_time));
    q = camera_div32(num, den);
    return delta - q;
  }
  num = camera_mul32(delta, (int32_t)(2u * (uint32_t)elapsed -
                                     (uint32_t)in_time));
  den = camera_mul32(2, out_time);
  return camera_div32(num, den);
}

void fifa96_camera_replay_snapshot(fifa96_camera_pose6 *snapshot,
                                   const fifa96_camera_pose6 *pose) {
  if (snapshot != NULL && pose != NULL) *snapshot = *pose;
}

int fifa96_camera_replay_begin(fifa96_camera_pose6 *pose,
                               const fifa96_camera_pose6 *start,
                               const fifa96_camera_pose6 *target,
                               fifa96_camera_replay_blend *state) {
  int32_t d;
  if (!pose || !target || !state) return -FIFA96_ERR_INVALID;
  if (start != NULL) *pose = *start;
  state->start = *pose;
  state->dx = target->x - pose->x;
  state->dy = target->y - pose->y;
  state->dz = target->z - pose->z;
  d = (target->yaw - pose->yaw) & 0xFFFF;
  if (d > 0x8000) d -= 0x10000;
  state->dyaw = d;
  d = (target->pitch - pose->pitch) & 0xFFFF;
  if (d > 0x8000) d -= 0x10000;
  state->dpitch = d;
  state->dratio = target->ratio - pose->ratio;
  state->elapsed = 0;
  state->in_time = FIFA96_CAMERA_REPLAY_IN_TICKS * 10;
  state->out_time = FIFA96_CAMERA_REPLAY_OUT_TICKS * 10;
  state->active = 1;   /* FUN_0004CF7C 0x4D037 */
  return FIFA96_OK;
}

int fifa96_camera_replay_begin_gated(fifa96_camera_pose6 *pose,
                                     const fifa96_camera_pose6 *start,
                                     const fifa96_camera_pose6 *target,
                                     fifa96_camera_replay_blend *state,
                                     int32_t screen_105ffc) {
  if (screen_105ffc == 7) return 0;
  return fifa96_camera_replay_begin(pose, start, target, state);
}

int fifa96_camera_replay_step(fifa96_camera_pose6 *pose, int32_t tick,
                              int32_t cancel_input, int32_t view_mode,
                              fifa96_camera_replay_blend *state) {
  int32_t total;
  if (!pose || !state) return -FIFA96_ERR_INVALID;
  total = state->out_time + state->in_time;
  if (total <= state->elapsed) {
    state->active = 0;
    return 1;
  }
  if (!state->active && cancel_input != 0 && view_mode != 0x14) {
    state->elapsed = 0;
    state->in_time = 0;
    state->out_time = 0;
    state->active = 0;
    return 1;
  }
  state->elapsed += tick;
  if (total < state->elapsed) state->elapsed = total;
  pose->x = state->start.x +
            fifa96_camera_replay_ease(state->dx, state->in_time,
                                      state->out_time, state->elapsed);
  pose->y = state->start.y +
            fifa96_camera_replay_ease(state->dy, state->in_time,
                                      state->out_time, state->elapsed);
  pose->z = state->start.z +
            fifa96_camera_replay_ease(state->dz, state->in_time,
                                      state->out_time, state->elapsed);
  pose->yaw = (state->start.yaw +
               fifa96_camera_replay_ease(state->dyaw, state->in_time,
                                         state->out_time, state->elapsed)) &
              0xFFFF;
  pose->pitch = (state->start.pitch +
                 fifa96_camera_replay_ease(state->dpitch, state->in_time,
                                           state->out_time,
                                           state->elapsed)) &
                0xFFFF;
  pose->ratio = state->start.ratio +
                fifa96_camera_replay_ease(state->dratio, state->in_time,
                                          state->out_time, state->elapsed);
  return 0;
}

void fifa96_camera_yaw_approach(fifa96_camera_handler_state *st) {
  int32_t angle, delta, step, t;
  if (!st) return;
  angle = (int32_t)((0u - (uint32_t)camera_atan16(st->track_x - st->pos_x,
                                                   st->track_z - st->pos_z)) &
                    0xFFFF);
  delta = (int32_t)(((uint32_t)angle - (uint32_t)st->yaw) & 0xFFFF);
  if (delta > 0x8000) delta -= 0x10000;
  step = camera_mul32(st->ratio, 3);
  step >>= 2;
  if (delta < 1) {
    t = step + delta;
    if (t < 0)
      st->yaw = (int32_t)(((uint32_t)st->yaw + (uint32_t)t) & 0xFFFF);
  } else {
    t = step - delta;
    if (t < 0)
      st->yaw = (int32_t)(((uint32_t)st->yaw - (uint32_t)t) & 0xFFFF);
  }
}

int fifa96_camera_action_target(
    fifa96_camera_handler_state *st,
    const int32_t table[FIFA96_CAMERA_ACTION_SLOTS][3], int32_t event_present,
    int32_t event_roster, int32_t event_side, int32_t ac2) {
  int32_t slot;
  if (!st || !table) return -FIFA96_ERR_INVALID;
  slot = event_present ? fifa96_camera_target_slot(event_roster, event_side, ac2)
                       : -1;
  if (slot < 0 || slot >= FIFA96_CAMERA_ACTION_SLOTS) return 0;
  st->track_x = table[slot][0];
  st->track_y = table[slot][1];
  st->track_z = table[slot][2];
  fifa96_camera_yaw_approach(st);
  return 1;
}

int fifa96_camera_action_step(
    fifa96_camera_handler_state *st,
    const int32_t table[FIFA96_CAMERA_ACTION_SLOTS][3], int32_t event_present,
    int32_t event_roster, int32_t event_side, int32_t ac2,
    fifa96_camera_pose6 *pose, int32_t tick, int32_t cancel_input,
    int32_t view_mode, fifa96_camera_replay_blend *blend) {
  int rc;
  if (!st || !table || !pose || !blend) return -FIFA96_ERR_INVALID;
  rc = fifa96_camera_replay_step(pose, tick, cancel_input, view_mode, blend);
  if (rc < 0) return rc;
  if (rc == 0) return 0;
  return fifa96_camera_action_target(st, table, event_present, event_roster,
                                     event_side, ac2);
}

/* The default-arm handler dispatch (FUN_000505D0's `(*&0x108B80)[iVar6]`).
 * The engine needs the staged camera-record subset + behavior block; without
 * them the arm stays unported (0) and the tape is unchanged. */
static int camera_pose_handler_arm(fifa96_camera *cam, int32_t *yaw, int32_t *pitch,
                                   int32_t *view_ratio,
                                   const fifa96_camera_pose_args *args) {
  fifa96_camera_handler_state *st = args->handler_state;
  fifa96_camera_handler_out *out = args->handler_out;
  fifa96_camera_handler_out scratch;
  int rc;
  if (!st || args->handler_skip) return 0;
  if (!out) {
    memset(&scratch, 0, sizeof scratch);
    out = &scratch;
  }
  switch (args->selector) {
    case 0:
      if (!args->handler_block) return 0;
      rc = fifa96_camera_behavior_steady(st, args->handler_block,
                                         args->handler_tick, out);
      break;
    case 1:
      if (!args->handler_block) return 0;
      rc = fifa96_camera_behavior_sidetrack(st, args->handler_block,
                                            args->handler_tick, out);
      break;
    case 2:
      if (!args->handler_block) return 0;
      rc = fifa96_camera_behavior_staged(st, args->handler_block,
                                         args->handler_tick, out);
      break;
    case 3:
      rc = fifa96_camera_behavior_action(st, args->handler_live,
                                         args->handler_speed,
                                         args->handler_tick, out);
      break;
    default:
      return 0;
  }
  if (rc != FIFA96_OK) return rc;
  cam->pos_x = st->pos_x;
  cam->pos_z = st->pos_z;
  *yaw = st->yaw;
  *pitch = st->pitch;
  *view_ratio = st->ratio;
  return 1;
}

/* FU-148 §2.1(c): FUN_000702F8 with the EDX passthrough dropped; the
 * 0x10F4EE table is the static image table FUN_0006FFC0 installs at
 * [0x157748] (0x10F4EE..0x10F67D, 400 bytes). */
static const uint8_t camera_ramp_table[400] = {
    0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,
    2,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,
    5,   6,   6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   8,   8,   8,
    8,   8,   9,   9,   9,   9,   9,   10,  10,  10,  10,  10,  11,  11,  11,
    11,  11,  12,  12,  12,  12,  12,  13,  13,  13,  13,  13,  14,  14,  14,
    14,  14,  15,  15,  15,  15,  16,  16,  16,  16,  16,  17,  17,  17,  17,
    17,  18,  18,  18,  18,  18,  19,  19,  19,  19,  19,  20,  20,  20,  20,
    21,  21,  21,  21,  21,  22,  22,  22,  22,  23,  23,  23,  23,  23,  24,
    24,  24,  24,  25,  25,  25,  25,  25,  26,  26,  26,  26,  27,  27,  27,
    27,  27,  28,  28,  28,  28,  29,  29,  29,  29,  29,  30,  30,  30,  30,
    31,  31,  31,  31,  32,  32,  32,  32,  33,  33,  33,  33,  34,  34,  34,
    34,  34,  35,  35,  35,  35,  36,  36,  36,  36,  37,  37,  37,  37,  38,
    38,  38,  38,  39,  39,  39,  39,  40,  40,  40,  40,  41,  41,  41,  41,
    42,  42,  42,  42,  43,  43,  43,  44,  44,  44,  44,  45,  45,  45,  45,
    46,  46,  46,  46,  47,  47,  47,  48,  48,  48,  48,  49,  49,  49,  49,
    50,  50,  50,  51,  51,  51,  51,  52,  52,  52,  53,  53,  53,  53,  54,
    54,  54,  55,  55,  55,  56,  56,  56,  56,  57,  57,  57,  58,  58,  58,
    59,  59,  59,  59,  60,  60,  60,  61,  61,  61,  62,  62,  62,  63,  63,
    63,  64,  64,  64,  65,  65,  65,  66,  66,  66,  67,  67,  67,  68,  68,
    68,  69,  69,  69,  70,  70,  70,  71,  71,  72,  72,  72,  73,  73,  73,
    74,  74,  74,  75,  75,  76,  76,  76,  77,  77,  78,  78,  78,  79,  79,
    79,  80,  80,  81,  81,  82,  82,  82,  83,  83,  84,  84,  85,  85,  85,
    86,  86,  87,  87,  88,  88,  89,  89,  89,  90,  90,  91,  91,  92,  92,
    93,  93,  94,  94,  95,  95,  96,  97,  97,  98,  98,  99,  99,  100, 100,
    101, 102, 102, 103, 103, 104, 105, 105, 106, 107, 107, 108, 109, 109, 110,
    111, 112, 113, 113, 114, 115, 116, 117, 118, 119, 119, 120, 121, 123, 124,
    125, 126, 127, 129, 130, 132, 133, 135, 138, 140,
};

int16_t fifa96_camera_ramp(int16_t height) {
  int32_t value;
  if (height >= 0x640) return 0x94;
  if (height < 1) return 3;
  value = 0x94 - (int32_t)camera_ramp_table[(0x640 - height) >> 2];
  if (value < 0) return 0;
  return (int16_t)value;
}

/* M2 phase-11 T1 (FU-148 §14.5 / L14.7): the FUN_00070B94 camera-height arc
 * table, flat 0x10F3C4 (first-hand `read_memory`, 149 x 16-bit; the caller's
 * index `0x94 - (...)`, max 0x94, needs entries 0..148; the 0x640/1600 top is
 * the arc's ceiling). */
static const uint16_t camera_height_arc[149] = {
       0,   21,   43,   64,   85,  106,  127,  148,  168,  189,
     209,  229,  249,  269,  288,  308,  327,  347,  366,  385,
     404,  422,  441,  459,  477,  495,  513,  531,  549,  566,
     584,  601,  618,  635,  651,  668,  684,  701,  717,  733,
     749,  765,  780,  796,  811,  826,  841,  856,  871,  885,
     900,  914,  928,  942,  956,  969,  983,  996, 1009, 1023,
    1036, 1048, 1061, 1073, 1086, 1098, 1110, 1122, 1134, 1145,
    1157, 1168, 1179, 1190, 1201, 1212, 1223, 1233, 1243, 1254,
    1264, 1273, 1283, 1293, 1302, 1311, 1320, 1329, 1338, 1347,
    1356, 1364, 1372, 1380, 1388, 1396, 1404, 1411, 1419, 1426,
    1433, 1440, 1447, 1453, 1460, 1466, 1472, 1478, 1484, 1490,
    1496, 1501, 1506, 1511, 1516, 1521, 1526, 1531, 1535, 1539,
    1544, 1548, 1551, 1555, 1559, 1562, 1565, 1568, 1571, 1574,
    1577, 1579, 1582, 1584, 1586, 1588, 1590, 1591, 1593, 1594,
    1596, 1597, 1597, 1598, 1599, 1599, 1600, 1600, 1600,
};

/* The shared `[0x157744]` arc sample (`0x10F3C4`):
 * `T[0x94 - |peak - timer|] - T[0x94 - peak]`, clamped >= 0. Two native
 * producers read the same installed table:
 *  - FUN_00070B94 (`0x70B94..0x70C05`, first-hand `disassemble_bytes`):
 *    `BX = word[0x1577F4]; if ((int16)timer >= (int16)BX) return 0;
 *     if ((int16)word[0x1577F0] <= 0) return 0;` then this formula;
 *  - the FU-153 / T1 record jump arc (FUN_0007BF20 `0x7C0BF..0x7C113`,
 *    first-hand `disassemble_bytes`): `EDX = 0x94 - |+0x85 - +0x87|`,
 *    `EBX = T16[EDX]`, `ECX = 0x94 - word[+0x85]`, `EAX = T16[ECX]`,
 *    `EBX -= EAX`, store +0x5D, clamp < 0 -> 0.
 * The native indexes are trusted (the producers guarantee
 * `|peak - timer| <= 0x94`); the clamp below is the defensive port bound. */
int16_t fifa96_camera_arc_height(int16_t peak, int16_t timer) {
  int32_t diff = (int32_t)peak - (int32_t)timer;
  int32_t index;
  int32_t value;
  if (diff < 0) diff = -diff;
  index = 0x94 - diff;
  if (index < 0) index = 0;
  else if (index > 0x94) index = 0x94;
  value = (int32_t)camera_height_arc[index];
  index = 0x94 - (int32_t)peak;
  if (index < 0) index = 0;
  else if (index > 0x94) index = 0x94;
  value -= (int32_t)camera_height_arc[index];
  if (value < 0) return 0;                              /* 0x70bfd JGE */
  return (int16_t)value;
}

/* M2 phase-11 T1 (FU-148 §14.5 / L14.7): FUN_00070B94
 * (`0x70B94..0x70C05`), first-hand `disassemble_bytes`. The body:
 *
 *   BX = word[0x1577F4]                    ; limit
 *   if ((int16)timer >= (int16)BX) return 0;
 *   if ((int16)word[0x1577F0] <= 0) return 0;
 *   ... the shared arc formula (fifa96_camera_arc_height)
 *
 * The engine field map: timer = word[0x1577FA], limit = word[0x1577F4],
 * height = word[0x1577F0] (the `[0x1577EE].hi` event height), F2 =
 * word[0x1577F2] = camera_event_ramp's stored `event_peak`. */
int16_t fifa96_camera_height(const fifa96_camera *cam) {
  int32_t timer;
  if (!cam) return 0;
  timer = (int16_t)cam->timer;
  if (timer >= (int16_t)cam->timer_limit) return 0;     /* 0x70ba3 JGE */
  if ((int16_t)cam->event_param <= 0) return 0;         /* 0x70ba8 JLE */
  return fifa96_camera_arc_height((int16_t)cam->event_peak, (int16_t)timer);
}

/* FUN_000795A4 (0x795A4..0x795B2): (a*b + 0x8000) >> 16, the low word. */
int16_t fifa96_camera_mul16(int32_t a, int32_t b) {
  int64_t v = ((int64_t)a * (int64_t)b + 0x8000) >> 16;
  return (int16_t)(uint16_t)(uint32_t)(int32_t)v;
}

/* FU-152 §2.9 (T2): FUN_00070544 — the ramp/velocity/anchor setup, split out
 * of FUN_00071C94 so FUN_000709D0 can re-run it. `seed_x`/`seed_z` are the
 * native stored step pair 0x1577BA/BC (the fast-path dividend); `ramp_param`
 * is the FUN_00071C94 ECX argument that reaches FUN_00070544 as its EAX sign
 * param (0 -> the NEG F6 path, the receive/row-04 arm; 1 -> the ADD path, the
 * chaser's `0x70DBA MOV ECX,1`, FU306 erratum 1). */
static int camera_event_ramp(fifa96_camera *cam, int16_t seed_x, int16_t seed_z,
                             int ramp_param) {
  int16_t f2, f4, f6, f8;
  int16_t ty = (int16_t)cam->pos_y;              /* native 0x157768 = 0x157750 */
  int has_height = (int16_t)cam->event_param > 0;
  /* 0x7054b..0x70557: origin := the current camera triple (0x157764/68/6C). */
  cam->target_x = cam->pos_x;
  cam->target_y = cam->pos_y;
  cam->target_z = cam->pos_z;
  if (has_height) {
    int16_t r;
    if ((int16_t)cam->event_param > 0x640) cam->event_param = 0x640;
    f2 = fifa96_camera_ramp((int16_t)cam->event_param);
    f4 = (int16_t)(f2 * 2);
    /* F6 = F2 +/- ramp(event_param - target y); the sign is the caller's
     * FUN_00070544 param (0 -> subtract, the FUN_000709D0 path). */
    r = fifa96_camera_ramp((int16_t)((int16_t)cam->event_param - ty));
    f6 = ramp_param != 0 ? (int16_t)(f2 + r) : (int16_t)(f2 - r);
    f8 = (int16_t)(f4 - f6);
    cam->timer_limit = (uint16_t)f4;
    cam->timer = (uint16_t)f6;
  } else {
    /* 0x705f0: the idle-height arm (F2=6, F4=0xC, F6=0, F8=0xC). */
    f2 = 6;
    f4 = 0xC;
    f6 = 0;
    f8 = 0xC;
    cam->event_param = 0;                        /* 0x705fe */
    cam->timer_limit = (uint16_t)f4;
    cam->timer = (uint16_t)f6;
  }
  cam->event_peak = f2;   /* 0x1577F2 (M2 phase-11 T1 / FUN_00070B94 input) */
  cam->band_1577f6 = f6;  /* 0x1577F6 = F4 - F8 (0x705e8; FU292-L2) */
  cam->ramp_divisor = f8;
  /* 0x70630..0x706d9: the fast path (F8 != 0 and [0x157821] == 0) divides the
   * stored step pair; the slow path scales each nonzero velocity by
   * k/0x20 (k = [0x15781A] for the height arm, [0x15781B] for the idle arm).
   * The FUN_00092820(0, 0x26) sound sink is a leg. */
  if (f8 != 0 && cam->pan_counter == 0) {
    cam->vel_x = (int16_t)(seed_x / f8);
    cam->vel_z = (int16_t)(seed_z / f8);
  } else {
    int32_t k = has_height ? cam->pan_rate_hi : cam->pan_rate_lo;
    if (cam->vel_x != 0) cam->vel_x = (int16_t)((k * (int32_t)cam->vel_x) / 0x20);
    if (cam->vel_z != 0) cam->vel_z = (int16_t)((k * (int32_t)cam->vel_z) / 0x20);
  }
  /* 0x706f9: the bearing the integrator gates on. */
  cam->speed = (uint16_t)(int16_t)fifa96_entity_distance(cam->vel_x, cam->vel_z);
  /* 0x70704..0x70798: the > 0x19 atan walk — rebuild the velocity from the
   * atan direction and cap the magnitude to 0x19. FUN_0008DC68's distance is
   * the bearing; FUN_000CD474 the direction; the 0x114E04 sine fold and the
   * FUN_000795A4 products (0x19 * sin, 0x19 * cos) rebuild the pair. */
  if ((int16_t)cam->speed > 0x19) {
    int32_t angle = 0;
    if (fifa96_entity_angle(cam->vel_x, cam->vel_z, &angle) != FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    cam->vel_x = fifa96_camera_mul16(0x19, fifa96_entity_sine(angle));
    cam->vel_z = fifa96_camera_mul16(0x19, fifa96_entity_sine(angle + 0x100));
    cam->speed = 0x19;
  }
  /* 0x707c4..0x707eb: the step products against the divisor. */
  cam->event_step_x = (int16_t)(cam->vel_x * f8);
  cam->event_step_z = (int16_t)(cam->vel_z * f8);
  /* 0x707f1..0x7081d: anchor path A (origin + step, y forced 0). */
  cam->anchor_x = cam->target_x + cam->event_step_x;
  cam->anchor_y = 0;
  cam->anchor_z = cam->target_z + cam->event_step_z;
  cam->anchor_time = (uint16_t)f4;
  /* 0x7090c..0x70996: anchor path B at height >= 0x50 (a2_time = F2 + ramp(
   * event_param - 0x50); span = a2_time - F6). The 0x70828..0x708e3 > 0x70
   * [0x157A6D] branch is a leg (OL-T11-79). */
  if (has_height && (int16_t)cam->event_param >= 0x50) {
    int16_t a2_time = (int16_t)(f2 + fifa96_camera_ramp((int16_t)((int16_t)cam->event_param - 0x50)));
    int16_t span = (int16_t)(a2_time - f6);
    if (span >= 1) {
      cam->anchor2_x = cam->target_x + (int16_t)(cam->vel_x * span);
      cam->anchor2_z = cam->target_z + (int16_t)(cam->vel_z * span);
      cam->anchor2_time = (uint16_t)a2_time;
    } else {
      cam->anchor2_x = cam->anchor_x;
      cam->anchor2_y = cam->anchor_y;
      cam->anchor2_z = cam->anchor_z;
      cam->anchor2_time = (uint16_t)f4;
    }
  } else {
    cam->anchor2_x = cam->anchor_x;
    cam->anchor2_y = cam->anchor_y;
    cam->anchor2_z = cam->anchor_z;
    cam->anchor2_time = (uint16_t)f4;
  }
  /* 0x70996..0x709c1: the tail clears the event cursor, the accelerator words
   * and the rate bytes 0x157816/17/18. */
  cam->event_cursor = 0;
  cam->acc_x = 0;
  cam->acc_z = 0;
  cam->rate_x = 0;
  cam->rate_z = 0;
  return FIFA96_OK;
}

/* FU303/FU294-L2 (phase-28 T8): the 0x10E169 sub-object dir compass and the
 * 0x11042B/0x11042C rate pairs, first-hand `read_memory` of /FIFA96.EXE:
 *
 *   0x10E169 base, index = sign-extended dir_x + 4*sign-extended dir_z;
 *   the live 3x3 rows (dir_z -1,0,1 x dir_x -1,0,1) read
 *   {5,4,3} {6,-1,2} {7,0,1} (the center 0xFF is the "no dir" marker).
 *
 * The native's signed index is unchecked; the live slot writer (+0x20/+0x21
 * = T2/T3 of the mapped pad, FU70 §1.2) only produces {-1,0,1} rows because
 * T1 never yields the dead T3 0x40/0 entries, so the compass is the exact
 * live table. Any other int8 pair reads neighbouring image tables and is the
 * FU303-L11 residual: it resolves to -1 ("no row"), which the setter stores
 * as the 0xFF marker and the rate arm refuses (0x71f2d's signed gate). */
static int32_t camera_dir_row(int32_t dir_x, int32_t dir_z) {
  static const int8_t compass[3][3] = {
      {5, 4, 3},    /* dir_z = -1 */
      {6, -1, 2},   /* dir_z = 0 */
      {7, 0, 1},    /* dir_z = 1 */
  };
  if (dir_x < -1 || dir_x > 1 || dir_z < -1 || dir_z > 1) return -1;
  return (int32_t)compass[dir_z + 1][dir_x + 1];
}

int fifa96_camera_event_dir(fifa96_camera *cam, int has_slot, int32_t dir_x,
                            int32_t dir_z) {
  if (!cam) return -FIFA96_ERR_INVALID;
  /* 0x71d70/0x71d7d: a NULL slot (`[rec+0x20] == 0`) falls through the native
   * 0x71DBA bearing oct; the port's early return keeps the 0x71d6a default 0
   * (the divergence is the carried FU303-family leg FU303-L12, phase-51 T6;
   * the natural M2 tape never reaches the wall arm). */
  if (!has_slot) return FIFA96_OK;
  /* 0x71d89/0x71d93: a non-zero dir pair takes the 0x10E169 compass row. */
  if (dir_x != 0 || dir_z != 0) {
    cam->dir_157815 = (int8_t)camera_dir_row(dir_x, dir_z);
    return FIFA96_OK;
  }
  /* 0x71da7: both dir bytes zero and the bearing word zero -> 0xFF
   * (0x71db1); otherwise the 0x8DD70 bearing oct (0x71dba..0x71ddc). */
  if ((int16_t)cam->speed == 0) {
    cam->dir_157815 = -1;
    return FIFA96_OK;
  }
  {
    int32_t angle = 0;
    if (fifa96_entity_angle((int32_t)(int16_t)cam->vel_x,
                            (int32_t)(int16_t)cam->vel_z, &angle) != FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    cam->dir_157815 =
        (int8_t)(((uint32_t)(angle + 0x40) & 0x3FFu) >> 7);
  }
  return FIFA96_OK;
}

/* 0x11042B/0x11042C — the 64-pair (8 dir-byte rows x 8 compass columns)
 * rate table, `read_memory 0x11042B` (128 B). The native index is
 * `(int8_t)(row + (dir_byte << 3))` (the 8-bit ADD AL,AH) and the pair is
 * `[0x11042B + 2*idx]`/`[0x11042C + 2*idx]`. Rows 4..7 repeat rows 0..3 byte
 * for byte (the image genuinely duplicates the second half; not a transcription
 * artefact) — the live dir-byte domain is `{-1,0,1}`, so `idx` never leaves
 * rows 0..1 in the reachable slots. */
static const int8_t camera_rate_pairs[64][2] = {
    { 0,  0}, { 1,  0}, { 1,  0}, { 1,  0}, { 0,  0}, {-1,  0}, {-1,  0}, {-1,  0},
    { 0,  1}, { 0,  0}, { 0, -1}, { 0, -1}, { 0, -1}, { 0,  0}, { 0,  1}, { 0,  1},
    { 0,  1}, { 0,  1}, { 0,  0}, { 0, -1}, { 0, -1}, { 0, -1}, { 0,  0}, { 0,  1},
    { 0,  1}, { 0,  1}, { 0,  1}, { 0,  0}, { 0, -1}, { 0, -1}, { 0, -1}, { 0,  0},
    { 0,  0}, { 1,  0}, { 1,  0}, { 1,  0}, { 0,  0}, {-1,  0}, {-1,  0}, {-1,  0},
    { 0,  1}, { 0,  0}, { 0, -1}, { 0, -1}, { 0, -1}, { 0,  0}, { 0,  1}, { 0,  1},
    { 0,  1}, { 0,  1}, { 0,  0}, { 0, -1}, { 0, -1}, { 0, -1}, { 0,  0}, { 0,  1},
    { 0,  1}, { 0,  1}, { 0,  1}, { 0,  0}, { 0, -1}, { 0, -1}, { 0, -1}, { 0,  0},
};

int fifa96_camera_event_set(fifa96_camera *cam, int16_t seed_x, int16_t seed_z,
                            int16_t height, int ramp_param) {
  int16_t target_y;
  if (!cam) return -FIFA96_ERR_INVALID;
  /* 0x71c99: the pan suspended with the match state bails before the reset. */
  if (cam->event_suspended) return 1;
  /* FUN_000700F4 subset (the native 71C94 call hands it the current target
   * triple and a zero flag): the target is preserved and the event state is
   * cleared; the origin/anchors all restart at the triple. */
  target_y = (int16_t)cam->pos_y;
  cam->target_x = cam->pos_x;
  cam->target_y = cam->pos_y;
  cam->target_z = cam->pos_z;
  cam->anchor_x = cam->pos_x;
  cam->anchor_y = cam->pos_y;
  cam->anchor_z = cam->pos_z;
  cam->anchor2_x = cam->pos_x;
  cam->anchor2_y = cam->pos_y;
  cam->anchor2_z = cam->pos_z;
  cam->vel_x = 0;
  cam->vel_z = 0;
  cam->speed = 0;
  cam->step = 0;
  cam->acc_x = 0;
  cam->acc_z = 0;
  cam->timer = 0;
  cam->timer_limit = 0;
  cam->event_param = 0;
  cam->event_cursor = 0;
  cam->anchor_time = 0;
  cam->anchor2_time = 0;
  cam->rate_x = 0;
  cam->rate_z = 0;
  cam->ramp_divisor = 0;
  cam->event_step_x = 0;
  cam->event_step_z = 0;
  cam->pan_counter = 0;                          /* 0x700f4 clears 0x157821 */
  cam->tracked = FIFA96_CAMERA_TRACKED_NONE;     /* 0x700f4 0x70258 clears 0x1577CA */
  /* 0x700f4 reads the 0x1104AB table at [0x14C2FE]*8 + [0x14C2FA]*2; the
   * image defaults are both zero -> [0x157819] = 10, [0x15781A] = 16, and
   * [0x15781B] = [0x15781A] + (0x20 - [0x15781A]) / 2 = 24 (0x70137 SUB,
   * 0x7013b SAR 1, 0x70142 ADD, 0x70146 store). */
  cam->pan_decay = 10;
  cam->pan_rate_hi = 16;
  cam->pan_rate_lo = (uint8_t)(cam->pan_rate_hi + (0x20 - cam->pan_rate_hi) / 2);
  /* 71C94 0x71cdb..0x71cf8: clamp the height to [target y, 0x640];
   * FUN_00070544 0x705f1/0x705fe zeroes the cell again when the clamped
   * value is < 1 (the idle arm). */
  if (height < target_y) height = target_y;
  if (height > 0x640) height = 0x640;
  if (height < 1) height = 0;
  cam->event_param = (uint16_t)height;
  /* 0x71d6a: the setter's default dir-byte store (the `FUN_00070544`-saved
   * DL is the caller's `0x71d63 XOR DL,DL` = 0). The actor arm
   * (`fifa96_camera_event_dir`) runs at the caller that holds the record. */
  cam->dir_157815 = 0;
  return camera_event_ramp(cam, seed_x, seed_z, ramp_param);
}

/* FU-152 §2.9 (T2): FUN_000709D0 (0x709d0..0x70b90) — the pan step. */
int fifa96_camera_pan_step(fifa96_camera *cam, struct fifa96_rng *rng,
                           int walk_gate, int32_t *class_of, int32_t *param) {
  if (!cam) return -FIFA96_ERR_INVALID;
  if ((int16_t)cam->event_param != 0) {
    int32_t c = 0, p = 0;
    fifa96_camera_pan_band((int32_t)(int16_t)cam->event_param, &c, &p);
    if (class_of) *class_of = c;                 /* FUN_00065CF8 sink (leg) */
    if (param) *param = p;
  }
  /* 0x70a3f..0x70a65: while [0x157821] < 100, a zero counter first
   * timestamps [0x1575C3] through FUN_000CB2A4, then [0x1575CB] and the pan
   * counter increment in that order. The engine stages tick_112e88 per frame. */
  if (cam->pan_counter < 100) {
    if (cam->pan_counter == 0)
      cam->save.previous_1575c3 = cam->save.tick_112e88;
    cam->save.count_1575cb++;
    cam->pan_counter++;
  }
  /* 0x70a6b..0x70a90: event_param = event_height * [0x157819] / 0x20. */
  cam->event_param =
      (uint16_t)(int16_t)(((int32_t)(int16_t)cam->event_param *
                           (int32_t)cam->pan_decay) / 0x20);
  /* 0x70a96..0x70b80: the ([0x14C1D4]|[0x14C1D6]) & 4 random walk. */
  if (cam->vel_x != 0 && cam->vel_z != 0 && (walk_gate & 4) != 0) {
    int32_t angle = 0;
    int32_t speed = fifa96_entity_distance(cam->vel_x, cam->vel_z);
    if (fifa96_entity_angle(cam->vel_x, cam->vel_z, &angle) != FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    if (rng) {
      uint16_t value = 0;
      if (fifa96_rng_step(rng, &value) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      angle += (int32_t)(value & 0xFF) - 0x80;   /* 0x70b05/0x70b07: low - 0x80 */
    }
    cam->vel_x = fifa96_camera_mul16(speed, fifa96_entity_sine(angle));
    cam->vel_z = fifa96_camera_mul16(speed, fifa96_entity_sine(angle + 0x100));
  }
  /* 0x70b86: FUN_00070544(0) over the stored step pair. */
  return camera_event_ramp(cam, cam->event_step_x, cam->event_step_z, 0);
}

/* The update's in-line call site (no rng/band staging on the frame path). */
static int camera_pan_step_internal(fifa96_camera *cam, struct fifa96_rng *rng,
                                    int walk_gate, int32_t *class_of,
                                    int32_t *param) {
  return fifa96_camera_pan_step(cam, rng, walk_gate, class_of, param);
}

/* FU-152 §2.9 (T2): FUN_0008DC50 (0x8dc50..0x8dc62) — |v| >> n, signed. */
int32_t fifa96_camera_shift_step(int32_t v, int n) {
  if (v >= 0) return v >> n;
  return -((-v) >> n);
}

/* Slice-B Task 4: append one stat event to the caller-supplied array. */
static void camera_stat_emit(fifa96_camera_stat_events *stats, uint32_t site,
                             int16_t delta, uint8_t entity) {
  if (!stats) return;
  if (stats->count < FIFA96_CAMERA_STAT_MAX) {
    stats->event[stats->count].site = site;
    stats->event[stats->count].delta = delta;
    stats->event[stats->count].entity = entity;
    stats->count++;
  } else {
    stats->overflow = 1;
  }
}

static int32_t camera_save_neg32(int32_t value) {
  return (int32_t)(0u - (uint32_t)value);
}

static int camera_save_entity_valid(int32_t id) {
  return id != FIFA96_CAMERA_SAVE_ENTITY_NONE && id >= 0;
}

static int32_t camera_save_add32(int32_t a, int32_t b) {
  return (int32_t)((uint32_t)a + (uint32_t)b);
}

static int32_t camera_save_sub32(int32_t a, int32_t b) {
  return (int32_t)((uint32_t)a - (uint32_t)b);
}

static int32_t camera_save_team_for_record(int32_t id) {
  return camera_save_entity_valid(id) && id < 22 ? id / 11
                                                 : FIFA96_CAMERA_SAVE_ENTITY_NONE;
}

static void camera_save_leg(fifa96_camera_save_state *s, uint8_t kind,
                            uint32_t site, int32_t a, int32_t b, int32_t c) {
  if (s->leg_count < FIFA96_CAMERA_SAVE_LEG_MAX) {
    fifa96_camera_save_leg *leg = &s->legs[s->leg_count++];
    leg->site = site;
    leg->a = a;
    leg->b = b;
    leg->c = c;
    leg->kind = kind;
  } else {
    s->leg_overflow = 1;
  }
}

static void camera_save_sit(fifa96_camera_save_state *s, uint32_t site,
                            int32_t value) {
  camera_save_leg(s, FIFA96_CAMERA_SAVE_LEG_SIT, site, value, 0, 0);
}

static void camera_save_post(fifa96_camera_save_state *s, uint32_t site,
                             int32_t id, int32_t target, int32_t code) {
  camera_save_leg(s, FIFA96_CAMERA_SAVE_LEG_POST, site, id, target, code);
}

static int camera_save_rng(fifa96_camera_save_state *s, uint16_t *value) {
  return fifa96_rng_step(s->rng_92ac8, value);
}

static void camera_save_copy_controlled_pos(fifa96_camera_save_state *s) {
  s->target_x_15772f = s->controlled_pos[0];
  s->target_y_157733 = s->controlled_pos[1];
  s->target_z_157737 = s->controlled_pos[2];
}

/* Shared 0x72CBC..0x72D53 (new controlled) and 0x73062..0x730F3 (changed
 * tracked) target block. `changed` selects only the writes present natively. */
static int camera_save_target_block(fifa96_camera *cam,
                                    fifa96_camera_save_state *s, int changed) {
  uint16_t draw = 0;
  int32_t magnitude;
  s->flag_157710 = 0;
  s->flag_157711 = 0;
  s->stamp_157712 = s->tick_112e88;       /* FUN_000CB2A4 */
  s->clear_157716 = 0;
  magnitude = camera_abs(cam->pos_z);
  if (camera_save_rng(s, &draw) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  s->delay_15771a = camera_save_add32(0x46 - (magnitude >> 7), draw & 0xFu);
  camera_save_copy_controlled_pos(s);
  s->controlled_15773b = s->controlled_157a83;
  s->side_15773b = s->controlled_side;
  s->opponent_157726 = s->opponent_target;
  s->latch_15772a = 1;
  if (changed) {
    s->age_157a97 = 0;
    s->target_157a8b = s->tracked_157a87;
    s->target_15771e = s->tracked_157a87;
  } else {
    s->controlled_1575de = s->controlled_157a83;
    s->target_15771e = FIFA96_CAMERA_SAVE_ENTITY_NONE;
    s->stat_actor_157722 = FIFA96_CAMERA_SAVE_ENTITY_NONE;
    s->previous_age_delta_157595 = s->age_delta_157a8f;
  }
  return FIFA96_OK;
}

static void camera_save_metric(fifa96_camera_save_state *s) {
  s->dx_1575ce = (int16_t)((uint16_t)s->current_x_1575ab -
                           (uint16_t)s->prev_x_15759f);
  s->dz_1575d0 = (int16_t)((uint16_t)s->current_z_1575b3 -
                           (uint16_t)s->prev_z_1575a7);
  s->band_1575cc = (int16_t)fifa96_entity_hypot(s->dx_1575ce, s->dz_1575d0);
}

/* 0x73114..0x732C6: opponent identity refresh and the two goal-line metric
 * comparisons. All four 0x8DC68 calls reuse fifa96_entity_distance. */
static void camera_save_opponent_compare(fifa96_camera *cam,
                                         fifa96_camera_save_state *s) {
  int32_t controlled_metric, opponent_metric;
  if (!camera_save_entity_valid(s->opponent_157726) ||
      s->opponent_157726 != s->opponent_target) {
    s->opponent_157726 = s->opponent_target;
    s->latch_15772a = 1;
    return;
  }
  if (s->controlled_side == 0) {
    if (cam->pos_z <= 0x660 || s->opponent_lane_x >= 0xC0) return;
  } else {
    if (cam->pos_z >= -0x660 || s->opponent_lane_x >= 0xC0) return;
  }
  controlled_metric = (int16_t)fifa96_entity_distance(
      (int16_t)s->controlled_pos[0],
      (int16_t)(0xB10 - (int16_t)s->controlled_pos[2]));
  opponent_metric = (int16_t)fifa96_entity_distance(
      (int16_t)s->opponent_pos[0],
      (int16_t)(0xB10 - (int16_t)s->opponent_pos[2]));
  if (opponent_metric > controlled_metric + 0x20) {
    if (s->latch_15772a == 0) {
      if (s->controlled_side == 0) {
        camera_save_sit(s, 0x731C3, 1);
        camera_save_sit(s, 0x731D2, 1);
        camera_save_post(s, 0x731E2, 0x65, s->controlled_157a83, 8);
      } else {
        camera_save_sit(s, 0x73274, 1);
        camera_save_sit(s, 0x73283, 1);
        camera_save_post(s, 0x73293, 0x65, s->controlled_157a83, 8);
      }
      s->latch_15772a = 1;
    }
  } else {
    s->latch_15772a = 0;
  }
}

/* 0x732CB..0x733BC: the goal-side position/velocity arm and its 0x741B4
 * selection. A successful post refreshes the target position triple. */
static void camera_save_goal_side(fifa96_camera_save_state *s) {
  /* 0x732CB..0x732DD reads the cached record [0x15773B], not the current
   * [0x157A83]. Its cached team side is staged with that record. */
  if (s->side_15773b == 1) {
    if (camera_save_sub32(s->controlled_pos[2], 0x690) > s->target_z_157737 &&
        s->controlled_vel75 > 0) {
      int32_t select = (1u ^ s->side_swap_157abe) & 1u;
      camera_save_sit(s, 0x73323, select != 0 ? 8 : 0xC);
      camera_save_post(s, 0x73338, 0x96, s->controlled_157a83, 4);
      camera_save_copy_controlled_pos(s);
    }
  } else {
    if (camera_save_add32(s->controlled_pos[2], 0x690) < s->target_z_157737 &&
        s->controlled_vel75 < 0) {
      int32_t select = s->side_swap_157abe & 1u;
      camera_save_sit(s, 0x73391, select != 0 ? 8 : 0xC);
      camera_save_post(s, 0x733A6, 0x96, s->controlled_157a83, 4);
      camera_save_copy_controlled_pos(s);
    }
  }
}

/* 0x73428..0x7368B: both age-crossing arms, the position/velocity gate, the
 * target-id/side gates, and the exact FUN_00092194 actor comparison. */
static void camera_save_late(fifa96_camera *cam, fifa96_camera_save_state *s) {
  int32_t previous_age = camera_save_sub32(s->age_157a97, s->frame_delta_157a64);
  if (s->age_157a97 > 0xB4 && previous_age <= 0xB4) {
    if (s->controlled_side != 0) {
      if (camera_save_sub32(s->target_z_157737, 0x3C0) > cam->pos_z &&
          cam->pos_z < 0x588) {
        camera_save_sit(s, 0x7347D, 8);
        camera_save_post(s, 0x7348D, 0xD9, s->controlled_157a83, 0x21);
      }
    } else {
      if (camera_save_add32(s->target_z_157737, 0x3C0) < cam->pos_z &&
          cam->pos_z > 0x588) {
        camera_save_sit(s, 0x734C7, 8);
        camera_save_post(s, 0x734D7, 0xD9, s->controlled_157a83, 0x21);
      }
    }
    /* A recognized 0xB4 crossing returns on BOTH inner-gate outcomes
     * (0x73461/0x7346D and 0x734AB/0x734B7); it never reaches the delay arm. */
    return;
  }

  if (s->age_157a97 <= s->delay_15771a || previous_age > s->delay_15771a)
    return;
  {
    int32_t abs_x = camera_abs(s->target_x_15772f);
    if (abs_x > 0x6E0 && abs_x < 0x720 &&
        camera_abs(s->target_z_157737) < 0xA80) {
      int fire = s->controlled_vel73 == 0;
      if (!fire)
        fire = (s->controlled_vel73 > 0) != (cam->pos_x > 0);
      if (fire) {
        camera_save_sit(s, 0x73596, 0xF);
        camera_save_post(s, 0x735A6, 0x68, s->controlled_157a83, 4);
        return;
      }
    }
  }
  if (camera_save_entity_valid(s->target_15771e)) {
    camera_save_sit(s, 0x735C5, 1);
    camera_save_post(s, 0x735D5, 0xB2, s->controlled_157a83, 0x21);
    return;
  }
  if (s->controlled_side != 0 && cam->pos_z < -0x660) {
    camera_save_sit(s, 0x73607, 8);
    camera_save_post(s, 0x73617, 0xD9, s->controlled_157a83, 0x21);
    return;
  }
  if (s->controlled_side == 0 && cam->pos_z > 0x660) {
    camera_save_sit(s, 0x73649, 8);
    camera_save_post(s, 0x73659, 0xD9, s->controlled_157a83, 0x21);
    return;
  }
  /* 0x73665: FUN_00092194 itself is only a getter, but the [0x15B33C]
   * producer is unported. Unknown actor state parks the comparison/post. */
  if (!s->actor_known) {
    camera_save_leg(s, FIFA96_CAMERA_SAVE_LEG_PRODUCER, 0x73665, 0, 0, 0);
    return;
  }
  if (s->actor_15b33c != s->controlled_157a83)
    camera_save_post(s, 0x73680, 0xD8, s->controlled_157a83, 0x21);
}

/* Slice-B Task 9: the complete FUN_00072AC4 after 0x72AE5. The labels mirror
 * native join 0x730F8 (publish cache/delta) and 0x73404 (common age tail). */
int fifa96_camera_save_tail(fifa96_camera *cam, fifa96_camera_stat_events *stats) {
  fifa96_camera_save_state *s;
  if (!cam) return -FIFA96_ERR_INVALID;
  s = &cam->save;
  s->leg_count = 0;
  s->leg_overflow = 0;

  /* 0x72AF4..0x72B19: the sanctioned FUN_000721C8 leg, at its native point. */
  if (s->flag_1575e8 == 0 && cam->tracked != FIFA96_CAMERA_TRACKED_NONE &&
      !camera_save_entity_valid(s->controlled_157a83) && (int16_t)cam->speed > 0)
    camera_save_leg(s, FIFA96_CAMERA_SAVE_LEG_SCAN, 0x72B19, 0, 0, 0);

  if (s->phase_157a4d != 2u) {
    s->tracked_157a87 = FIFA96_CAMERA_SAVE_ENTITY_NONE;
    s->target_157a8b = FIFA96_CAMERA_SAVE_ENTITY_NONE;
    return FIFA96_OK;
  }

  if (s->tracked_157a87 == s->controlled_157a83) goto same_tracked;
  if (!camera_save_entity_valid(s->controlled_157a83)) goto initialize;

  /* 0x72C6C..0x72C8D: [[controlled]+4][0] is an unported producer.
   * Unknown class parks only these two no-return-value situation calls. */
  if (!s->class_known) {
    camera_save_leg(s, FIFA96_CAMERA_SAVE_LEG_PRODUCER, 0x72C6C, 0, 0, 0);
  } else if (s->controlled_class == 0x18DAu) {
    camera_save_sit(s, 0x72C8D, 4);
  } else if (s->controlled_class == 0x18DEu) {
    camera_save_sit(s, 0x72C8D, 0xA);
  }

  if (camera_save_entity_valid(s->tracked_157a87)) goto changed_tracked;
  if (s->controlled_157a83 == s->target_157a8b) {
    s->age_157a97 = camera_save_add32(s->age_157a97, s->age_delta_157a8f);
    goto publish;
  }
  goto new_controlled;

initialize:
  /* 0x72B57..0x72C67: the complete no-controlled initialization. */
  s->mode_15758c = 1;
  s->flag_15759e = 0;
  s->flag_1575e8 = 0;
  s->approach_15758d = 0x7FFFFFFF;
  s->prev_x_15759f = cam->pos_x;
  s->prev_y_1575a3 = cam->pos_y;
  s->prev_z_1575a7 = cam->pos_z;
  s->target_x_1575b7 = cam->target_x;
  s->target_y_1575bb = cam->target_y;
  s->target_z_1575bf = cam->target_z;
  s->event_1575c7 = (int32_t)(int16_t)cam->event_param;
  s->count_1575cb = 0;
  s->rec_1575da = s->tracked_157a87;
  if (camera_save_entity_valid(s->rec_1575da)) {
    /* 0x72BB1..0x72BB7 and 0x72C09..0x72C1B are NULL-guarded stores. */
    s->team_1575d2 = camera_save_team_for_record(s->rec_1575da);
    s->rec_side_1575d2 = s->tracked_side;
    /* 0x72C13..0x72C1B stores [[rec]+0x7A6], the ball-record pointer.
     * Its producer is not projected into the engine, so record UNKNOWN rather
     * than manufacturing an opposing team/player id. */
    s->ball_record_1575d6 = FIFA96_CAMERA_SAVE_BALL_RECORD_UNKNOWN;
  }
  {
    /* 0x72BD5: the additional history-producer leg returns its staged native
     * record token/type; the store and all dependent branches stay intact. */
    int32_t native_result = s->history_record_157599;
    camera_save_leg(s, FIFA96_CAMERA_SAVE_LEG_HISTORY, 0x72BD5, 1, 2,
                    s->rec_side_1575d2);
    s->history_record_157599 = native_result;
  }
  if (s->rec_side_1575d2 != 0) {
    s->prev_z_1575a7 = camera_save_neg32(s->prev_z_1575a7);
    s->target_z_1575bf = camera_save_neg32(s->target_z_1575bf);
  }
  s->stamp_157591 = s->tick_112e88;           /* FUN_000CB2A4 */
  s->lane_best_a_1575e9 = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  s->lane_best_b_1575ed = FIFA96_CAMERA_SAVE_ENTITY_NONE;
  for (int i = 0; i < FIFA96_CAMERA_SAVE_LANES; i++) {
    s->lane[i].active = 0;
    s->lane[i].approach = 0;
    s->lane[i].distance = 0x3C0;
  }
  s->target_157a8b = s->tracked_157a87;
  goto publish;

changed_tracked:
  if (camera_save_target_block(cam, s, 1) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  goto publish;

new_controlled:
  if (camera_save_target_block(cam, s, 0) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  s->current_x_1575ab = cam->pos_x;
  s->current_y_1575af = cam->pos_y;
  s->current_z_1575b3 = cam->pos_z;
  if (s->rec_side_1575d2 != 0)
    s->current_z_1575b3 = camera_save_neg32(s->current_z_1575b3);
  camera_save_metric(s);
  s->age_157a97 = 0;

  if (camera_save_entity_valid(s->target_157a8b) &&
      camera_save_entity_valid(s->rec_1575da) &&
      s->controlled_side == s->rec_side_1575d2) {
    s->history_15772b = s->history_record_157599;
    s->stat_actor_157722 = s->rec_1575da;
    camera_stat_emit(stats, 0x72DE3, 1, FIFA96_CAMERA_STAT_REC_1575DA);

    /* 0x72DE8..0x72E10: both native situation-call shapes, including the
     * >0x660 arm's two consecutive EAX=0 calls. */
    if (s->current_z_1575b3 > 0x660) {
      camera_save_sit(s, 0x72DF8, 0);
      camera_save_sit(s, 0x72E10, 0);
    } else if (s->current_z_1575b3 > 0x1B0) {
      camera_save_sit(s, 0x72E10, 1);
    } else {
      camera_save_sit(s, 0x72E10, 0);
    }

    /* 0x72E15 is the FIRST history-dependent branch. Everything above it is
     * independently proven: the 0x72CF9 draw, target/metric writes, age clear,
     * current-Z situation calls, and the 0x72DE3 +1. Unknown history parks
     * only the early exits and their fall-through (including +2 and 0x72FCC),
     * then rejoins 0x730F8 with fresh target/age state for the late arm. */
    if (!s->history_known) {
      camera_save_leg(s, FIFA96_CAMERA_SAVE_LEG_PRODUCER, 0x72E15, 0, 0, 0);
      goto publish;
    }
    if (s->history_record_157599 != 0 && s->history_type_157599 == 0xA &&
        (int32_t)s->band_1575cc > 0xF0 && s->count_1575cb < 3u) {
      camera_save_post(s, 0x72E51, 0x6E, s->controlled_157a83, 4);
      goto publish;
    }
    if (s->history_record_157599 != 0 && s->history_type_157599 == 0x13 &&
        (int32_t)s->dz_1575d0 < -0x90 && s->current_z_1575b3 > 0x3C0) {
      camera_save_post(s, 0x72E98, 0x64, s->rec_1575da, 4);
      goto publish;
    }
    if (s->current_z_1575b3 > 0x750 &&
        camera_abs(s->current_x_1575ab) < 0x2D0) {
      camera_stat_emit(stats, 0x72ED1, 2, FIFA96_CAMERA_STAT_REC_1575DA);
      camera_save_sit(s, 0x72EDD, 0);
      camera_save_post(s, 0x72EED, 0x5A, s->rec_1575da, 4);
      goto publish;
    }

    /* 0x72EF7..0x72F64: the previous-delta / external-[0x1575C3] gate. */
    if ((int32_t)s->event_1575c7 > 0xC0 && (int32_t)s->dz_1575d0 > 0x5A0 &&
        camera_save_sub32(s->previous_age_delta_157595,
                          s->previous_1575c3) < 0x3C) {
      if (s->current_z_1575b3 > 0x3C0) {
        camera_save_sit(s, 0x72F35, 0);
        camera_save_post(s, 0x72F45, 0x59, s->rec_1575da, 4);
      } else {
        camera_save_post(s, 0x72F5F, 0x58, s->rec_1575da, 4);
      }
      goto publish;
    }

    /* 0x72F69..0x72FCC: FUN_00072270 is a sanctioned leg, but its two best
     * lane outputs are unknown until ported. Park both lane comparisons and
     * the downstream RNG draw instead of treating them as known-empty. */
    if (!s->lanes_known) {
      camera_save_leg(s, FIFA96_CAMERA_SAVE_LEG_PRODUCER, 0x72F69, 0, 0, 0);
      goto publish;
    }
    /* 0x72F69..0x72FC7: both best-lane pointers and their +9 distances. */
    if (camera_save_entity_valid(s->lane_best_a_1575e9) &&
        camera_save_entity_valid(s->lane_best_b_1575ed) &&
        s->lane_best_a_1575e9 < FIFA96_CAMERA_SAVE_LANES &&
        s->lane_best_b_1575ed < FIFA96_CAMERA_SAVE_LANES &&
        s->lane[s->lane_best_a_1575e9].distance < 0x60 &&
        s->lane[s->lane_best_b_1575ed].distance < 0x60 &&
        (int32_t)s->dz_1575d0 > 0xF0 && (int32_t)s->band_1575cc > 0x1E0 &&
        s->current_z_1575b3 > 0x3C0) {
      camera_save_post(s, 0x72FC2, 0x5C, s->rec_1575da, 4);
      goto publish;
    }

    /* 0x72FCC: the third distinct shared-RNG call site in this function. */
    {
      uint16_t draw = 0;
      if (camera_save_rng(s, &draw) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      if ((draw & 3u) == 0)
        camera_save_post(s, 0x72FE9, 0x55, s->rec_1575da, 4);
    }
    goto publish;
  }

  /* 0x72FF3..0x7305D: the opposite-side 0xB6 arm. */
  if (camera_save_entity_valid(s->target_157a8b) &&
      camera_save_entity_valid(s->rec_1575da) &&
      s->controlled_side != s->rec_side_1575d2 &&
      s->global_158743 == 2u && (int32_t)s->band_1575cc > 0x1E0)
    camera_save_post(s, 0x73058, 0xB6, s->controlled_157a83, 4);
  goto publish;

same_tracked:
  if (camera_save_entity_valid(s->controlled_157a83)) {
    camera_save_opponent_compare(cam, s);
    camera_save_goal_side(s);
  } else {
    if (s->flag_15759e == 0 &&
        (int16_t)cam->timer > (int16_t)cam->event_peak)
      s->flag_15759e = 1;
    if (s->mode_15758c != 0) s->mode_15758c = 0;
  }
  s->age_delta_157a8f = camera_save_add32(s->age_delta_157a8f,
                                          s->frame_delta_157a64);
  goto finish;

publish:
  s->tracked_157a87 = s->controlled_157a83;
  s->age_delta_157a8f = (int32_t)s->frame_delta_157a64;

finish:
  if (camera_save_entity_valid(s->controlled_157a83)) {
    s->age_157a97 = camera_save_add32(s->age_157a97, s->frame_delta_157a64);
    camera_save_late(cam, s);
  } else if (camera_save_entity_valid(s->target_157a8b)) {
    /* 0x73696..0x7369E passes [[0x157A8B]+0x7A6], the same opaque ball-record
     * kind as 0x72C1B; its unprojected producer stays explicitly UNKNOWN. */
    camera_save_leg(s, FIFA96_CAMERA_SAVE_LEG_LANES, 0x7369E,
                    FIFA96_CAMERA_SAVE_BALL_RECORD_UNKNOWN, 0, 0);
  }
  return FIFA96_OK;
}

/* FU-152 §2.9 (T2): FUN_00070DE0 (0x70de0..0x71310) — the derived core + the
 * bit-0x8 boundary arm (Slice-B Task 4). */
int fifa96_camera_reposition(fifa96_camera *cam, int32_t prev_x, int32_t prev_y,
                             int32_t prev_z, const int8_t *type_off_x,
                             const int8_t *type_off_z, uint8_t *events,
                             uint8_t *event_code,
                             fifa96_camera_stat_events *stats) {
  uint16_t flags_prev = 0, flags_cur = 0;
  uint16_t changed;
  int32_t cur[3];
  int32_t prev[3] = {prev_x, prev_y, prev_z};
  int32_t origin[3];
  int32_t bnd[3];
  uint16_t flags_origin;
  (void)type_off_x;
  (void)type_off_z;
  if (!cam || !events || !event_code) return -FIFA96_ERR_INVALID;
  cur[0] = cam->pos_x;
  cur[1] = cam->pos_y;
  cur[2] = cam->pos_z;
  *events = 0;
  *event_code = 0;
  (void)fifa96_camera_classify(prev, &flags_prev);
  (void)fifa96_camera_classify(cur, &flags_cur);
  if ((uint16_t)(flags_prev & flags_cur) != 0) return 0;   /* 0x70e2f */
  changed = (uint16_t)(flags_prev ^ flags_cur);            /* 0x70e3a */
  origin[0] = prev_x;
  origin[1] = prev_y;
  origin[2] = prev_z;
  flags_origin = flags_prev;

  /* 0x70e3c TEST AL,0x8: the bit-0x8 boundary arm (Slice-B Task 4). */
  if ((changed & 0x8u) != 0) {
    int32_t target_z;
    int32_t dz, dx, dy, quotient;
    int16_t dz_word;
    uint16_t flags_bnd = 0;
    /* 0x70e44..0x70e79: the boundary z — +-0xB0F when the previous triple was
     * "inside" (flags_prev == 0), else +-0xB11; the sign follows cur_z. */
    if (flags_prev == 0)
      target_z = (cur[2] >= 0) ? 0xB0F : -0xB0F;
    else
      target_z = (cur[2] >= 0) ? 0xB11 : -0xB11;
    /* 0x70e81..0x70ef9: project the prev->cur segment onto z = target_z. The
     * SUB/IMUL/ADD operations retain their low 32 bits before FUN_000A2AEE's
     * signed IDIV. Unsigned intermediates reproduce that wrap without C UB. */
    dz = (int32_t)((uint32_t)cur[2] - (uint32_t)prev[2]);
    dx = (int32_t)((uint32_t)cur[0] - (uint32_t)prev[0]);
    dy = (int32_t)((uint32_t)cur[1] - (uint32_t)prev[1]);
    dz_word = (int16_t)((uint16_t)target_z - (uint16_t)prev[2]);
    if (dz_word == 0 || dz == 0) {
      bnd[0] = prev[0];
      bnd[1] = prev[1];
      bnd[2] = target_z;
    } else {
      quotient = (int32_t)((uint32_t)dx * (uint32_t)(int32_t)dz_word) / dz;
      bnd[0] = (int32_t)((uint32_t)prev[0] + (uint32_t)quotient);
      quotient = (int32_t)((uint32_t)dy * (uint32_t)(int32_t)dz_word) / dz;
      bnd[1] = (int32_t)((uint32_t)prev[1] + (uint32_t)quotient);
      bnd[2] = target_z;
    }
    (void)fifa96_camera_classify(bnd, &flags_bnd);
    /* 0x70f0a..0x70f26: boundary_flags & 4 -> reclassify the boundary as the
     * new previous triple (0x710df); & 0x13 -> straight to the step-back
     * (0x710fd). Both re-enter the step-back below. */
    if ((flags_bnd & 0x4u) != 0) {
      origin[0] = bnd[0];
      origin[1] = bnd[1];
      origin[2] = bnd[2];
      flags_origin = flags_bnd;
      goto step_back;
    }
    if ((flags_bnd & 0x13u) != 0) goto step_back;
    /* 0x70f2c..0x70fbe: the two +3 tracked stat arms. */
    if (bnd[1] > 0x90) {
      camera_stat_emit(stats, 0x70F44, 3, FIFA96_CAMERA_STAT_TRACKED);
      *events = 0x49;      /* 0x70f66 CALL 0x8f188 (0x49, tracked, 0x40) */
      *event_code = 0x40;
      /* 0x70f6b..0x70fa1: vel_z = (cur_z >= 0 ? -1 : 1) * max(4, |vel_z|/2).
       * The magnitude is the sign-extended word[0x1577c2] folded through the
       * dword[0x1577c0]>>16 extract, |.| then >> 1 (0x70f89), floored at 4
       * (0x70f8e JGE / 0x70f93), negated only when cur_z >= 0 (0x70f98/0x70f9f),
       * and stored back to vel_z (0x70fa1). */
      {
        int32_t mag = (int32_t)cam->vel_z;
        if (mag < 0) mag = -mag;
        mag >>= 1;
        if (mag < 4) mag = 4;
        cam->vel_z = (int16_t)((cur[2] >= 0) ? -mag : mag);
      }
      goto reposition;
    }
    if (bnd[0] < -0xC0 || bnd[0] > 0xC1) {
      camera_stat_emit(stats, 0x70FCE, 3, FIFA96_CAMERA_STAT_TRACKED);
      *events = 0x48;      /* 0x70ff0 CALL 0x8f188 (0x48, tracked, 0x40) */
      *event_code = 0x40;
      goto reposition;
    }
    /* 0x710df: no stat event — reclassify the boundary point as the new
     * previous triple and fall into the step-back. */
    origin[0] = bnd[0];
    origin[1] = bnd[1];
    origin[2] = bnd[2];
    flags_origin = flags_bnd;
    goto step_back;
  }

step_back:
  /* 0x710fd/0x71101/0x71106: the step-back (and its mask tail) runs only when
   * the two classify words differ (`CMP AX,word[ESP+0x34]` / `JZ 0x71302`).
   * Same-zone triples jump straight to the store, leaving the camera at the
   * current point (the m 42 goal link is the live case: prev/cur both inside
   * the mouth band, so no step). */
  changed = (uint16_t)(flags_origin ^ flags_cur);
  if (changed != 0) {
    /* 0x710fd..0x711d7 LAB: step the current triple back toward the origin
     * (the previous triple, or the boundary point after a 0x710df reclassify)
     * by FUN_0008DC50(delta, 1) per axis while the candidate keeps the
     * origin's flags; adopt each adopted candidate. */
    int32_t adopted[3] = {origin[0], origin[1], origin[2]};
    int32_t delta[3] = {
        (int32_t)((uint32_t)cur[0] - (uint32_t)origin[0]),
        (int32_t)((uint32_t)cur[1] - (uint32_t)origin[1]),
        (int32_t)((uint32_t)cur[2] - (uint32_t)origin[2])};
    for (;;) {
      int32_t candidate[3];
      delta[0] = fifa96_camera_shift_step(delta[0], 1);
      delta[1] = fifa96_camera_shift_step(delta[1], 1);
      delta[2] = fifa96_camera_shift_step(delta[2], 1);
      if (delta[0] == 0 && delta[1] == 0 && delta[2] == 0) break;
      candidate[0] = (int32_t)((uint32_t)adopted[0] + (uint32_t)delta[0]);
      candidate[1] = (int32_t)((uint32_t)adopted[1] + (uint32_t)delta[1]);
      candidate[2] = (int32_t)((uint32_t)adopted[2] + (uint32_t)delta[2]);
      {
        uint16_t candidate_flags = 0;
        (void)fifa96_camera_classify(candidate, &candidate_flags);
        if (candidate_flags == flags_origin) {
          adopted[0] = candidate[0];
          adopted[1] = candidate[1];
          adopted[2] = candidate[2];
        }
      }
    }
    cam->pos_x = adopted[0];
    cam->pos_y = adopted[1];
    cam->pos_z = adopted[2];
    /* The mask effects, over the changed flags (`uVar5 = origin ^ cur`, the
     * native 0x710fd tail). `local_1c` is the ORIGIN classify's return byte
     * (flags_origin == 0 when it returned 1): bit 4 zeroes vel_z only when
     * flags_origin == 0; bit 0x10 takes the y jitter when flags_origin != 0
     * and the FUN_00070B94 elevation loop otherwise (leg, OL-T11-79). */
    {
      if ((changed & 3u) != 0) cam->vel_x = (int16_t)fifa96_camera_shift_step(-cam->vel_x, 2);
      if ((changed & 0x4u) != 0 && flags_origin == 0) cam->vel_z = 0;
      if ((changed & 0x10u) != 0 && flags_origin != 0) {
        cam->pos_y += (cam->pos_y < 0 ? -4 : 4);   /* +-2*delta (delta 2 staged) */
      }
    }
    cam->speed = (uint16_t)(int16_t)fifa96_entity_distance(cam->vel_x, cam->vel_z);
  }
  return 1;

reposition:
  /* 0x71092..0x710da: the reposition store — the camera lands on the boundary
   * point with z snapped to +-0xAD4 (the "target" clamp; sign from the
   * entering cur_z). The 0x974dc(8)/0x651f0(6)/0x974f0(0x3e8) sinks stay legs
   * (OL-T11-79), as do the corner velocity updates (the low arm's is the
   * unowned FUN_00092AC8 global RNG). */
  cam->pos_x = bnd[0];
  cam->pos_y = bnd[1];
  cam->pos_z = (cur[2] >= 0) ? 0xAD4 : -0xAD4;
  return 1;
}

/* M2 phase-10 T2 (FU-147 §3.3 leg 7 / FU-152 §2.9): FUN_00070C08 — the
 * per-record camera event FUN_0007BF20's tail calls for every record
 * (0x7C8FC). Every gate and operand is read first-hand from
 * `0x70C08..0x70DDD` this task. */
int fifa96_camera_record_event(fifa96_camera *cam, struct fifa96_rng *rng,
                               const fifa96_camera_record_event_in *in,
                               fifa96_camera_record_event_out *out) {
  int16_t v1;
  int16_t v2;
  if (!cam || !in || !out) return -FIFA96_ERR_INVALID;
  memset(out, 0, sizeof *out);
  /* 0x70C12/0x70C1F: the record gates. */
  if (in->skip_9a != 0) return 0;
  if (in->action_91 == 0x22u && in->ball_height < 0x20) return 0;
  /* 0x70C3A..0x70CA1: the two shift-stepped seed corrections (the native
   * `[0x1577BA/BC] - ([0x15774C/54] - [0x157764/6C])`, 16-bit subtracts then
   * the word store/re-read, >> 2 via 0x8DC50). FU306 §5.2 item 3 / L4: both
   * differences are word stores in the native (`0x70C58 word[ESP+2] = DX`,
   * `0x70C8A word[ESP+4] = AX`), and 0x8DC50 sign-tests the word (`CWDE`),
   * so the 16-bit wrap applies before the shift. */
  {
    int16_t dx =
        (int16_t)((uint16_t)cam->pos_x - (uint16_t)cam->target_x);
    int16_t dz =
        (int16_t)((uint16_t)cam->pos_z - (uint16_t)cam->target_z);
    int16_t sx = (int16_t)(uint16_t)((uint16_t)cam->event_step_x - (uint16_t)dx);
    int16_t sz = (int16_t)(uint16_t)((uint16_t)cam->event_step_z - (uint16_t)dz);
    v1 = (int16_t)fifa96_camera_shift_step((int32_t)sx, 2);
    v2 = (int16_t)fifa96_camera_shift_step((int32_t)sz, 2);
  }
  if (v1 == 0 && v2 == 0) {
    /* 0x70CB9..0x70CF7: three draws: t = (draw1 & 0x3F) + 0x30, then the
     * draw2/draw3 low bits pick each seed's sign. */
    uint16_t d1 = 0, d2 = 0, d3 = 0;
    int32_t t;
    if (!rng) return -FIFA96_ERR_INVALID;
    if (fifa96_rng_step(rng, &d1) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    if (fifa96_rng_step(rng, &d2) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    if (fifa96_rng_step(rng, &d3) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    t = (int32_t)(d1 & 0x3Fu) + 0x30;
    v1 = (int16_t)((d2 & 1u) ? t : -t);
    v2 = (int16_t)((d3 & 1u) ? t : -t);
  }
  {
    /* 0x70C4E/0x70C8F: BX = sext of the 16-bit sum word[rec+0x5D] + 0x70 (the
     * ramp target); 0x70C54/0x70C9E: CX = the same word + 0x68;
     * 0x70CFA..0x70D03: sext(CX) > dword[0x157750] -> the reflect arm
     * (JG 0x70D43), else the ramp. (T2-review erratum, fix round 1: the
     * pre-fix body split on `height_5d > ball_height` -> ramp — the inverse
     * mapping, missing both offsets; first-hand 0x70C8F..0x70D91.) */
    int32_t target =
        (int32_t)(int16_t)(uint16_t)((uint32_t)(uint16_t)in->height_5d + 0x70u);
    int32_t cx =
        (int32_t)(int16_t)(uint16_t)((uint32_t)(uint16_t)in->height_5d + 0x68u);
    if (cx > (int32_t)(int16_t)in->ball_height) {
      /* 0x70D43..0x70D91: reflect one seed by the |vel_x| vs |vel_z| compare,
       * then re-seed the height. e1 = |word[0x1577C0]| (0x70D43 CMP
       * word[0x1577C0],0; JL -> NEG; the 0x70D4D/0x70D58
       * `dword[0x1577BE]>>16` extract = word[0x1577C0], the same >>16 idiom
       * as the [0x1577EE]>>16 = [0x1577F0] read below), e2 = |word[0x1577C2]|
       * (0x70D63; 0x70D6D/0x70D77 `dword[0x1577C0]>>16` = word[0x1577C2]);
       * 0x70D81 CMP e1,e2; JLE -> NEG word[ESP+4] (v2), so the fall-through
       * (|vel_x| > |vel_z|) negates v1. (Fix round 2 errata-on-erratum:
       * round 1's e1 = ±[0x1577BE] speed / e2 = ±[0x1577C0] vel_x mis-read
       * the dword>>16 extracts; no word[0x1577BE] load exists in this arm.) */
      int32_t e1 = (cam->vel_x >= 0) ? (int32_t)cam->vel_x
                                    : -(int32_t)cam->vel_x;
      int32_t e2 = (cam->vel_z >= 0) ? (int32_t)cam->vel_z
                                    : -(int32_t)cam->vel_z;
      if (e1 > e2) v1 = (int16_t)(0 - (uint16_t)v1);
      else v2 = (int16_t)(0 - (uint16_t)v2);
      cam->event_param = (uint16_t)in->ball_height;
    } else {
      /* 0x70D05..0x70D41: the two-step `[0x1577F0]` ramp toward the target:
       * new = target + ((event - target) >> 1), then, while target < new,
       * new += (new - target) >> 1 (the second step re-reads the stored
       * word, 0x70D27..0x70D38). */
      int32_t ev = (int32_t)(int16_t)cam->event_param;
      int32_t nw = target + ((ev - target) >> 1);
      cam->event_param = (uint16_t)nw;
      if (target < nw) {
        int32_t cur = (int32_t)(int16_t)cam->event_param;
        cam->event_param = (uint16_t)(cur + ((cur - target) >> 1));
      }
    }
  }
  /* 0x70D9D..0x70DD1: the 0x8DC68 distance into the vector's first word, the
   * 0x92820(rec, 0x1C) sink request, then FUN_00071C94(rec, {dist, v1, v2},
   * EBX = [0x1577F0], ECX = 1). FU306 §5.2 item 1 / erratum 1: the native
   * `0x70DBA MOV ECX,1` survives the sink's PUSH/POP ECX and `0x71D60`
   * consumes it as the FUN_00070544 ramp sign (the ADD arm); the pre-fix
   * ECX=0 took the SUB arm. */
  out->distance = (int16_t)fifa96_entity_distance(v1, v2);
  out->sink = 1;
  out->sink_code = 0x1C;
  out->vec_1 = v1;
  out->vec_2 = v2;
  {
    int ev = fifa96_camera_event_set(cam, v1, v2, (int16_t)cam->event_param, 1);
    if (ev < 0) return -FIFA96_ERR_INVALID;
    /* FU303/FU294-L2 (phase-28 T8): FUN_00071C94's actor dir arm over the
     * record this event belongs to. */
    if (ev == FIFA96_OK)
      (void)fifa96_camera_event_dir(cam, in->has_slot, in->dir_x, in->dir_z);
    /* FUN_00071C94 0x71D27 stores its player argument into [0x1577CA] after
     * the 0x71C99 bail gate; the camera port leaves the bind to the caller
     * (which owns the record identity), so report whether the setter applied
     * (`applied == 0` on the bail, exactly like the T1 receive arm). */
    out->applied = ev == FIFA96_OK ? 1 : 0;
  }
  return 1;
}

/* FU-152 §2.9 (T2) + FU303/FU294-L2 (phase-28 T8): FUN_00071DF4's
 * table/keeper arm (`0x71ef0..0x7200f`). Order: the signed dir-byte gate,
 * the timer gate, the tracked sub-object (`[rec+0x20]`), the 0x10E169
 * compass row, the 0x11042B rate pair write, then the keeper gate over the
 * just-written `[0x157817]` (or the stored cell when the sub-object is
 * absent, 0x71fac). */
int fifa96_camera_rate_table(fifa96_camera *cam, int32_t event_height,
                             int32_t rate_byte, int32_t timer,
                             int32_t subobj_present, int32_t dir_x, int32_t dir_z,
                             int32_t event_byte, int32_t anchor_x, int32_t pos_z,
                             int32_t vel_z, uint8_t *keeper_code) {
  int applied = 0;
  int8_t rate_z;
  if (!cam || !keeper_code) return -FIFA96_ERR_INVALID;
  *keeper_code = 0;
  if (rate_byte < 0) return 0;                   /* 0x71ef7 JL */
  if (timer > 0x1E) return 0;                    /* 0x71f08 JG (word[0x1577FA]) */
  if (subobj_present) {
    int32_t row = camera_dir_row(dir_x, dir_z);
    int32_t idx;
    if (row < 0) return 0;                       /* 0x71f2f JL (no table row) */
    idx = (int32_t)(int8_t)(uint8_t)((uint32_t)row +
                                    ((uint32_t)rate_byte << 3));
    if (idx < 0 || idx > 63) return 0;   /* FU303-L11 out-of-domain residual */
    cam->rate_x = camera_rate_pairs[idx][0];
    cam->rate_z = camera_rate_pairs[idx][1];
    applied = 1;
  }
  rate_z = cam->rate_z;                          /* 0x71fac reads [0x157817] */
  if (event_byte != 2) return applied;           /* 0x92998 ring result (OL-27) */
  if (event_height < 0xC1) return applied;
  if (anchor_x > 0x23F || anchor_x < -0x23F) return applied;
  if (rate_z < 0) {
    if (pos_z < 0) {
      if (vel_z < 1) return applied;
      *keeper_code = 0x1D;
    } else {
      *keeper_code = 0x1E;
    }
  } else if (rate_z > 0) {
    if (pos_z > 0) {
      if (vel_z >= 0) return applied;
      *keeper_code = 0x1D;
    } else {
      *keeper_code = 0x1E;
    }
  } else {
    return applied;
  }
  return 1;
}
