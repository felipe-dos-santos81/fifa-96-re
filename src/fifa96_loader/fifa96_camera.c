#include "fifa96_loader/fifa96_camera.h"
#include "fifa96_loader/fifa96_entity_update.h"

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
  return FIFA96_OK;
}

int fifa96_camera_update(fifa96_camera *cam, int16_t delta, int view_class, int input_bit2) {
  if (!cam) return -FIFA96_ERR_INVALID;
  if (cam->paused) return FIFA96_OK;
  if ((int16_t)cam->speed != 0 || (int16_t)cam->event_param != 0) {
    cam->timer = (uint16_t)(cam->timer + (uint16_t)delta);
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
      /* The default arm calls (&0x108B80)[selector] (w7-b4 §2.8 leg 9). */
      return 0;
  }
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

int fifa96_camera_event_set(fifa96_camera *cam, int16_t seed_x, int16_t seed_z,
                            int16_t height) {
  int16_t target_y;
  int16_t f2, f4, f6, f8;
  int16_t vel_x, vel_z, step_x, step_z;
  if (!cam) return -FIFA96_ERR_INVALID;
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
  /* 71C94 0x71cdb..0x71cf8: clamp the height to [target y, 0x640]. */
  if (height < target_y) height = target_y;
  if (height > 0x640) height = 0x640;
  cam->event_param = (uint16_t)height;
  /* FUN_00070544 0x70558..0x705ee: the height ramp. */
  if (height < 1) {
    f2 = 6;
    f4 = 0xC;
    f6 = 0;
    f8 = 0xC;
  } else {
    f2 = fifa96_camera_ramp(height);
    f4 = (int16_t)(f2 * 2);
    /* param 0 -> the native NEG path: F6 = F2 - ramp(height - origin y). */
    f6 = (int16_t)(f2 - fifa96_camera_ramp((int16_t)(height - target_y)));
    f8 = (int16_t)(f4 - f6);
  }
  cam->timer_limit = (uint16_t)f4;
  cam->timer = (uint16_t)f8;
  /* 0x70630..0x706d9: the fast path (timer != 0 and the reset [0x157821]==0):
   * vel = the 0x1577BA/0x1577BC seed products / timer, signed. */
  vel_x = f8 != 0 ? (int16_t)((int)seed_x / (int)f8) : 0;
  vel_z = f8 != 0 ? (int16_t)((int)seed_z / (int)f8) : 0;
  cam->vel_x = vel_x;
  cam->vel_z = vel_z;
  /* 0x706f9: bearing = FUN_0008DC68(vel_x, vel_z), stored at 0x1577BE (the
   * engine's `speed`; the integrator's integration gate). Leg: the > 0x19
   * FUN_000CD474 atan walk. */
  cam->speed = (uint16_t)(int16_t)fifa96_entity_distance(vel_x, vel_z);
  /* 0x707c4..0x707eb: the step products against the new timer. */
  step_x = (int16_t)(vel_x * f8);
  step_z = (int16_t)(vel_z * f8);
  /* 0x707f1..0x7081d: anchor path A (0x157770 = origin + step, y = 0). */
  cam->anchor_x = cam->target_x + step_x;
  cam->anchor_y = 0;
  cam->anchor_z = cam->target_z + step_z;
  /* Anchor times: the native height <= 0x70 path stores F4 into [0x158000];
   * the > 0x70 [0x157A6D] branch is a leg (the timer is carried instead). */
  cam->anchor_time = (uint16_t)f4;
  /* 0x7090c..0x70996: anchor path B. */
  if (height >= 0x50) {
    int16_t a2_time = (int16_t)(f2 + fifa96_camera_ramp((int16_t)(height - 0x50)));
    int16_t span = (int16_t)(a2_time - f6);
    if (span >= 1) {
      /* 0x157794/0x15779C = origin + vel * span; [0x157798] keeps the reset y. */
      cam->anchor2_x = cam->target_x + (int16_t)(vel_x * span);
      cam->anchor2_z = cam->target_z + (int16_t)(vel_z * span);
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
  return FIFA96_OK;
}
