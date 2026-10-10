#include "fifa96_loader/fifa96_camera.h"
#include "fifa96_loader/fifa96_entity_update.h"
#include "fifa96_loader/fifa96_rng.h"

#include <string.h>

static int32_t camera_abs(int32_t value) {
  uint32_t magnitude = (uint32_t)value;
  if (value < 0) magnitude = 0u - magnitude;
  return (int32_t)magnitude;
}

int fifa96_camera_init(fifa96_camera *cam, int32_t x, int32_t y, int32_t z) {
  if (!cam) return -FIFA96_ERR_INVALID;
  cam->pos_x = x;
  cam->pos_y = y;
  cam->pos_z = z;
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
  cam->paused = 0;
  cam->event_suspended = 0;
  cam->ramp_divisor = 0;
  cam->pan_counter = 0;
  cam->pan_decay = 0;
  cam->pan_rate_hi = 0;
  cam->pan_rate_lo = 0;
  cam->event_step_x = 0;
  cam->event_step_z = 0;
  cam->tracked = FIFA96_CAMERA_TRACKED_NONE;   /* 0x700F4 0x70258 */
  cam->follow_speed = 0;
  return FIFA96_OK;
}

int fifa96_camera_set_tracked(fifa96_camera *cam, int32_t id) {
  if (!cam || id < FIFA96_CAMERA_TRACKED_NONE) return -FIFA96_ERR_INVALID;
  cam->tracked = id;                            /* 0x71D27 [0x1577CA] = rec */
  return FIFA96_OK;
}

/* FU-152 §2.9 (T2): FUN_000709D0 is defined below the integrator; the native
 * FUN_000736AC calls it in-line (0x737da) when `timer > timer_limit`. */
static int camera_pan_step_internal(fifa96_camera *cam, struct fifa96_rng *rng,
                                    int walk_gate, int32_t *class_of,
                                    int32_t *param);

int fifa96_camera_update(fifa96_camera *cam, int16_t delta, int view_class, int input_bit2) {
  return fifa96_camera_update_walk(cam, NULL, delta, view_class, input_bit2, 0);
}

int fifa96_camera_update_walk(fifa96_camera *cam, struct fifa96_rng *rng,
                              int16_t delta, int view_class, int input_bit2,
                              int walk_gate) {
  int16_t gate_speed;
  if (!cam) return -FIFA96_ERR_INVALID;
  if (cam->paused) {
    cam->follow_speed = (int16_t)cam->speed;
    return FIFA96_OK;
  }
  /* The 0x739CE follow gate reads [0x1577BE] before the tail recompute; the
   * pan step may overwrite it first. */
  gate_speed = (int16_t)cam->speed;
  if ((int16_t)cam->speed != 0 || (int16_t)cam->event_param != 0) {
    cam->timer = (uint16_t)(cam->timer + (uint16_t)delta);
    /* FU-152 §2.9 (T2) / FUN_000736AC 0x737b9..0x737da: the timer-driven pan
     * step runs before the same frame's integration; the walk gate is the
     * native ([0x14C1D4]|[0x14C1D6]) & 4 read at 0x70ab1..0x70ac2 (T3). */
    if ((int16_t)cam->timer > (int16_t)cam->timer_limit) {
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
        } else if ((int16_t)cam->event_cursor >= (view_class ? 0x90 : 0x70)) {
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
 * handler bodies (derived constant/clamp level; integrator helpers leg 9).
 * ------------------------------------------------------------------------- */

static int32_t camera_clamp_i32(int32_t value, int32_t lo, int32_t hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}

/* FU-152 §2.8: the 0x10896C behavior-block fields (first-hand read_memory;
 * layout per 84-byte block: +0x00/+0x04 targets, +0x08 class, +0xC the
 * class-gate dword, +0x30/+0x34 constants). */
const fifa96_camera_behavior_table_entry
    fifa96_camera_behavior_blocks[FIFA96_CAMERA_BEHAVIOR_BLOCKS] = {
        {0x340, 0xFA0, 3, 0, 0xEA6, 0},        /* 0x10896C */
        {0x340, 0x1200, 1, 0, 0x578, 0},       /* 0x1089C0 */
        {0x340, 0xD48, 3, 0xA7F8, 0x1130, 0},  /* 0x108A14 */
        {0x340, 0xFA0, 3, 0, 0xEA6, 0},        /* 0x108A68 */
        {0x340, 0x1200, 1, 0, 0x578, 0},       /* 0x108ABC */
        {0x340, 0xD48, 3, 0xA21C, 0x1130, 0},  /* 0x108B10 */
};

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
                                  const fifa96_camera_handler_block *block,
                                  int32_t rnd, fifa96_camera_handler_out *out) {
  if (!st || !block || !out) return -FIFA96_ERR_INVALID;
  /* 0x4E834: the yaw accumulator clamps into the sub-record bounds; the
   * pitch target clamps +-0x1620; the horizon jitter is rnd>>3 + 0xE09/0xAA1
   * (DAT_0014E4D8/D4). The class-4/else fold, the roll FUN_0004DF34, the
   * class-3 snap FUN_0004C7D0 and the +0x18/+0x10 lerps stay leg 9. */
  st->yaw = camera_clamp_i32(st->yaw, block->yaw_lo, block->yaw_hi);
  st->pitch = camera_clamp_i32(st->pitch, -0x1620, 0x1620);
  out->horizon_a = (rnd >> 3) + 0xE09;
  out->horizon_b = (rnd >> 3) + 0xAA1;
  return FIFA96_OK;
}

int fifa96_camera_behavior_sidetrack(fifa96_camera_handler_state *st,
                                     const fifa96_camera_handler_block *block,
                                     int32_t rnd, fifa96_camera_handler_out *out) {
  if (!st || !block || !out) return -FIFA96_ERR_INVALID;
  /* 0x4E3A8: the class-2 +0x68 / else -0x68 vertical bias; the yaw target
   * `track_z + block[0xD]` clamps +-0xB10; the pitch clamps +-0xE40; the
   * horizon jitter is rnd>>3 + 0xE3B/0xAD3. The class-1 +-0x4B0 fold and the
   * FUN_0004D698 writes stay leg 9. */
  st->track_y += (block->class_of == 2) ? 0x68 : -0x68;
  st->yaw = camera_clamp_i32(st->track_z + block->const34, -0xB10, 0xB10);
  st->pitch = camera_clamp_i32(st->pitch, -0xE40, 0xE40);
  out->horizon_a = (rnd >> 3) + 0xE3B;
  out->horizon_b = (rnd >> 3) + 0xAD3;
  return FIFA96_OK;
}

int fifa96_camera_behavior_staged(fifa96_camera_handler_state *st,
                                  const fifa96_camera_handler_block *block,
                                  int32_t rnd, fifa96_camera_handler_out *out) {
  int32_t class_of, pitch_target;
  if (!st || !block || !out) return -FIFA96_ERR_INVALID;
  /* 0x4EC9C: the class gate reads block[3] (the dword at +0xC): outside
   * 0x4000..0xC000 -> class 4, else class 3. The pitch target is the active
   * camera record's +0x18 (the engine's staged pos_z) plus block[0xC] in the
   * class-4 arm, minus it in the class-3 arm, clamped +-0x1620; the horizon
   * jitter is rnd>>3 + 0xE3B/0xAD3. Leg 9: the FUN_0004A1A60 sub-record
   * scale, the FUN_0004D698/FUN_0004DF34/FUN_0004E05C integrators and the
   * yaw-band snap (the native tests the accumulated `camera+0x58 + roll`
   * against 0x2000/0xA000 and uses 0x720/0xB10 as FUN_0004C7D0 snap-target
   * offsets — not a pan clamp). */
  class_of = (block->field3 < 0x4000 || block->field3 > 0xC000) ? 4 : 3;
  pitch_target = st->pos_z + (class_of == 4 ? block->const30 : -block->const30);
  st->pitch = camera_clamp_i32(pitch_target, -0x1620, 0x1620);
  out->horizon_a = (rnd >> 3) + 0xE3B;
  out->horizon_b = (rnd >> 3) + 0xAD3;
  out->class_of = class_of;
  return FIFA96_OK;
}

int fifa96_camera_behavior_action(fifa96_camera_handler_state *st, int live,
                                  int32_t speed, fifa96_camera_handler_out *out) {
  if (!st || !out) return -FIFA96_ERR_INVALID;
  /* 0x4DB38: `if (FUN_0006400C() == 0) param_4 += 100;` (faster when not
   * live); the staged pos x clamps +-0x420 and pos z +-0x810; the pitch
   * clamps to [0x2000, 63000]; the brake words +0x5A/+0x5E are zeroed. The
   * ball-angle slew (divisors 0x1E0000/0x3C0000) and the FUN_0004C7xx
   * tracking stay leg 9. */
  if (!live) speed += 100;
  out->speed = speed;
  st->pos_x = camera_clamp_i32(st->pos_x, -0x420, 0x420);
  st->pos_z = camera_clamp_i32(st->pos_z, -0x810, 0x810);
  st->pitch = camera_clamp_i32(st->pitch, 0x2000, 63000);
  st->brake_lo = 0;
  st->brake_hi = 0;
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
  if (!st) return 0;
  if (!out) {
    memset(&scratch, 0, sizeof scratch);
    out = &scratch;
  }
  switch (args->selector) {
    case 0:
      if (!args->handler_block) return 0;
      rc = fifa96_camera_behavior_steady(st, args->handler_block, args->handler_rnd, out);
      break;
    case 1:
      if (!args->handler_block) return 0;
      rc = fifa96_camera_behavior_sidetrack(st, args->handler_block, args->handler_rnd, out);
      break;
    case 2:
      if (!args->handler_block) return 0;
      rc = fifa96_camera_behavior_staged(st, args->handler_block, args->handler_rnd, out);
      break;
    case 3:
      rc = fifa96_camera_behavior_action(st, args->handler_live, args->handler_speed, out);
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

/* FUN_000795A4 (0x795A4..0x795B2): (a*b + 0x8000) >> 16, the low word. */
static int16_t camera_mul16(int32_t a, int32_t b) {
  int64_t v = ((int64_t)a * (int64_t)b + 0x8000) >> 16;
  return (int16_t)(uint16_t)(uint32_t)(int32_t)v;
}

/* FU-152 §2.9 (T2): FUN_00070544 — the ramp/velocity/anchor setup, split out
 * of FUN_00071C94 so FUN_000709D0 can re-run it. `seed_x`/`seed_z` are the
 * native stored step pair 0x1577BA/BC (the fast-path dividend); `ramp_param`
 * is the caller's FUN_00070544 EAX argument (0 -> the NEG F6 path). */
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
    cam->vel_x = camera_mul16(0x19, fifa96_entity_sine(angle));
    cam->vel_z = camera_mul16(0x19, fifa96_entity_sine(angle + 0x100));
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
  /* 0x70a3f..0x70a65: the [0x157821] counter, capped at 100. */
  if (cam->pan_counter < 100) cam->pan_counter++;
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
    cam->vel_x = camera_mul16(speed, fifa96_entity_sine(angle));
    cam->vel_z = camera_mul16(speed, fifa96_entity_sine(angle + 0x100));
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
static int32_t camera_shift_step(int32_t v, int n) {
  if (v >= 0) return v >> n;
  return -((-v) >> n);
}

/* FU-152 §2.9 (T2): FUN_00070DE0 (0x70de0..0x71310) — the derived core. */
int fifa96_camera_reposition(fifa96_camera *cam, int32_t prev_x, int32_t prev_y,
                             int32_t prev_z, const int8_t *type_off_x,
                             const int8_t *type_off_z, uint8_t *events,
                             uint8_t *event_code) {
  uint16_t flags_prev = 0, flags_cur = 0;
  uint16_t overlap;
  int32_t cur[3];
  int32_t prev[3] = {prev_x, prev_y, prev_z};
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
  overlap = (uint16_t)(flags_prev & flags_cur);
  if (overlap != 0) return 0;                    /* 0x70e2x: the AND return */
  if (((flags_prev ^ flags_cur) & 0x8u) == 0) {
    /* 0x70e56..0x710fd LAB: step the current triple back toward the previous
     * one (FUN_0008DC50(delta, 1) per axis) while the candidate keeps the
     * previous flags; adopt each adopted candidate. */
    int32_t adopted[3] = {prev_x, prev_y, prev_z};
    int32_t delta[3] = {cur[0] - prev[0], cur[1] - prev[1], cur[2] - prev[2]};
    for (;;) {
      int32_t candidate[3];
      delta[0] = camera_shift_step(delta[0], 1);
      delta[1] = camera_shift_step(delta[1], 1);
      delta[2] = camera_shift_step(delta[2], 1);
      if (delta[0] == 0 && delta[1] == 0 && delta[2] == 0) break;
      candidate[0] = adopted[0] + delta[0];
      candidate[1] = adopted[1] + delta[1];
      candidate[2] = adopted[2] + delta[2];
      {
        uint16_t candidate_flags = 0;
        (void)fifa96_camera_classify(candidate, &candidate_flags);
        if (candidate_flags == flags_prev) {
          adopted[0] = candidate[0];
          adopted[1] = candidate[1];
          adopted[2] = candidate[2];
        }
      }
    }
    cam->pos_x = adopted[0];
    cam->pos_y = adopted[1];
    cam->pos_z = adopted[2];
  }
  /* The mask effects, over the changed flags (`uVar5 = prev ^ cur`, the
   * native 0x710fd tail). `local_1c` is the PREVIOUS classify's return byte
   * (flags_prev == 0 when it returned 1): bit 4 zeroes vel_z only when
   * flags_prev == 0; bit 0x10 takes the y jitter when flags_prev != 0 and the
   * FUN_00070B94 elevation loop otherwise (leg, OL-T11-79). */
  {
    uint16_t changed = (uint16_t)(flags_prev ^ flags_cur);
    if ((changed & 3u) != 0) cam->vel_x = (int16_t)camera_shift_step(-cam->vel_x, 2);
    if ((changed & 0x4u) != 0 && flags_prev == 0) cam->vel_z = 0;
    if ((changed & 0x10u) != 0 && flags_prev != 0) {
      cam->pos_y += (cam->pos_y < 0 ? -4 : 4);   /* +-2*delta (delta 2 staged) */
    }
  }
  cam->speed = (uint16_t)(int16_t)fifa96_entity_distance(cam->vel_x, cam->vel_z);
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
   * the word store/re-read, >> 2 via 0x8DC50). */
  {
    int16_t dx =
        (int16_t)((uint16_t)cam->pos_x - (uint16_t)cam->target_x);
    int16_t dz =
        (int16_t)((uint16_t)cam->pos_z - (uint16_t)cam->target_z);
    v1 = (int16_t)camera_shift_step((int32_t)cam->event_step_x - (int32_t)dx, 2);
    v2 = (int16_t)camera_shift_step((int32_t)cam->event_step_z - (int32_t)dz, 2);
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
  /* 0x70D9D..0x70DCE: the 0x8DC68 distance into the vector's first word, the
   * 0x92820(rec, 0x1C) sink request, then FUN_00071C94(rec, {dist, v1, v2},
   * EBX = [0x1577F0], ECX = 0). */
  out->distance = (int16_t)fifa96_entity_distance(v1, v2);
  out->sink = 1;
  out->sink_code = 0x1C;
  out->vec_1 = v1;
  out->vec_2 = v2;
  {
    int ev = fifa96_camera_event_set(cam, v1, v2, (int16_t)cam->event_param, 0);
    if (ev < 0) return -FIFA96_ERR_INVALID;
    /* FUN_00071C94 0x71D27 stores its player argument into [0x1577CA] after
     * the 0x71C99 bail gate; the camera port leaves the bind to the caller
     * (which owns the record identity), so report whether the setter applied
     * (`applied == 0` on the bail, exactly like the T1 receive arm). */
    out->applied = ev == FIFA96_OK ? 1 : 0;
  }
  return 1;
}

/* FU-152 §2.9 (T2): FUN_00071DF4's table/keeper arm (derived). */
int fifa96_camera_rate_table(fifa96_camera *cam, int32_t event_height,
                             int32_t rate_byte, int32_t timer,
                             int32_t subobj_present, int8_t rate_x, int8_t rate_z,
                             int32_t event_byte, int32_t anchor_x, int32_t pos_z,
                             int32_t vel_z, uint8_t *keeper_code) {
  int applied = 0;
  if (!cam || !keeper_code) return -FIFA96_ERR_INVALID;
  *keeper_code = 0;
  if (rate_byte < 0) return 0;                   /* [0x157815] signed gate */
  if (timer > 0x1E) return 0;                    /* [0x1577FA] gate */
  if (subobj_present) {
    /* The caller resolves the 0x10E169 table + the 0x11042B/0x11042C rate
     * pair (leg OL-T11-79); a NULL pair is the native negative-byte return. */
    if (rate_x == 0 && rate_z == 0) return 0;
    cam->rate_x = rate_x;
    cam->rate_z = rate_z;
    applied = 1;
  }
  if (event_byte != 2) return applied;
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
