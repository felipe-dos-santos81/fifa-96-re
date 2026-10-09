#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

typedef struct fifa96_camera {
  int32_t pos_x, pos_y, pos_z;
  int32_t target_x, target_y, target_z;
  int32_t anchor_x, anchor_y, anchor_z;
  int32_t anchor2_x, anchor2_y, anchor2_z;
  int16_t vel_x, vel_z;
  uint16_t step;
  uint16_t acc_x, acc_z;
  uint16_t speed;
  uint16_t timer;
  uint16_t timer_limit;
  uint16_t event_param;
  uint16_t event_cursor;
  uint16_t anchor_time, anchor2_time;
  int8_t rate_x, rate_z;
  uint8_t paused;
} fifa96_camera;

int fifa96_camera_init(fifa96_camera *cam, int32_t x, int32_t y, int32_t z);
int fifa96_camera_update(fifa96_camera *cam, int16_t delta, int view_class, int input_bit2);
int fifa96_camera_target(const fifa96_camera *cam, int16_t *x, int16_t *z);
int fifa96_camera_reflect(fifa96_camera *cam, int input_bit0, int transition_active);
int fifa96_camera_out_of_bounds(int32_t x, int32_t z);

/* FU-148 §2.1(a)/§6.2 (S4 presentation completion): the FUN_000505D0 pose
 * feed. A pose record is the native 6-dword {x,y,z,yaw,pitch,angle}; the
 * camera behavior table 0x108B64 holds six 0x54-byte blocks with the class
 * dword at +8 and the two 8-record pose arrays (+0x48 alternate / +0x4C
 * default). First-hand: the image's per-side team flags ([0x1590CC], read by
 * FUN_0004B7D0) are zero, so the poser reads the +0x4C array — both FU-148
 * §2.1(a) ("+0x48 NULL") and w7-b4 §2.8 ("+0x4C when the predicates are
 * nonzero") misstate this. Only three distinct +0x4C arrays exist; the engine
 * pins those (blocks {0,3}: 0x107F1C class 3, {1,4}: 0x1080FC class 1,
 * {2,5}: 0x1082DC class 3); the +0x48 alternate is a leg (team-flag
 * producer unported). */
typedef struct fifa96_camera_pose {
  int32_t x, y, z, yaw, pitch, angle;
} fifa96_camera_pose;

typedef struct fifa96_camera_pose_block {
  int32_t class_of;                  /* block +8 (FUN_000505D0 *local_1c) */
  const fifa96_camera_pose *records; /* block +0x4C, 8 records */
} fifa96_camera_pose_block;

#define FIFA96_CAMERA_POSE_BLOCKS 6
extern const fifa96_camera_pose_block
    fifa96_camera_pose_blocks[FIFA96_CAMERA_POSE_BLOCKS];

typedef struct fifa96_camera_pose_args {
  uint32_t view_mode;   /* [0x14E57C] (getter FUN_00053D50) */
  uint32_t block;       /* behavior index (native [0x107514 + selector*0x70]) */
  uint32_t selector;    /* camera record selector [camera+4] (0..3 pose feed) */
  int32_t variant_x;    /* mode 3/4 variant predicate x (native *0x109A70) */
  int32_t sub;          /* replay sub index ([0x109A70+8]) */
  int mirror;           /* FUN_0004B818: 1 - side (caller-staged) */
} fifa96_camera_pose_args;

/* Write the six pose fields (FUN_000505D0's shared arm): camera
 * +0x10/+0x14/+0x18 (position), +0x58 (yaw), +0x5C (pitch), +0x4C (ratio ->
 * view_ratio). Returns 0 or -FIFA96_ERR_INVALID (NULL). */
int fifa96_camera_pose_apply(fifa96_camera *cam, int32_t *yaw, int32_t *pitch,
                             int32_t *view_ratio, const fifa96_camera_pose *pose);

/* Select and apply the FUN_000505D0 pose for the staged args. Landed arms
 * (first-hand): 1/0x12 record 0 with the selector 1/3 mirror and class fold;
 * 3/4 records {4|3}/{2|1} via FUN_000504E0/FUN_00050518; 6/0x10 record 7 with
 * the sub < 0 mirror; 8 records 6/5 by sub; 0x15 the fixed record 0x108714
 * with its yaw fold and z negate. The default arm (mode 0 and the unlisted
 * modes) is the handler call `(&0x108B80)[selector]` (w7-b4 §2.8 leg 9: the
 * four handler bodies are unported). Legs: the +0x48 alternate array, the
 * mode-3 replay camera copy/z clamp, the mode-4/8 mirror tails, the mode-0x15
 * camera+0x3c clamp and the `[0x107DD8]`/pad-idle/replay gates of the
 * per-frame driver FUN_0004D2D4. Returns 1 when a pose was applied, 0 for the
 * unported default arm, -FIFA96_ERR_INVALID (NULL or block out of range). */
int fifa96_camera_pose_feed(fifa96_camera *cam, int32_t *yaw, int32_t *pitch,
                            int32_t *view_ratio, const fifa96_camera_pose_args *args);

/* FU-148 §2.1(c)/§5.2 (S4): the FUN_000702F8 pan height ramp (EDX passthrough
 * dropped): 0x94 at >= 0x640, 3 below 1, else `0x94 - table0x10F4EE[(0x640-h)
 * >> 2]` clamped >= 0. The 400-byte table is the static image table
 * FUN_0006FFC0 installs at [0x157748]. */
int16_t fifa96_camera_ramp(int16_t height);

/* FU-148 §2.1(c)/§5.2 (S4): the derived FUN_00071C94 + FUN_00070544 subset —
 * the camera cut/event setter that feeds the FU-71 rate words. Preserves the
 * target (= the engine position; the native reset 0x700F4 is handed the
 * current target triple), resets the event state, copies the caller seed step
 * pair (the native 6-byte vec seed at 0x1577B8: {bearing, step_x, step_z}),
 * clamps the height to [target y, 0x640], runs the ramp (F2/F4/F6/F8), and
 * computes the signed fast-path velocity `seed / timer` plus the bearing
 * magnitude (`fifa96_entity_distance` = FUN_0008DC68), the step products and
 * the anchor A/B sets (0x157770/88/94). Returns 0 or -FIFA96_ERR_INVALID
 * (NULL). Legs: the > 0x19 bearing walk (FUN_000CD474 atan table 0x114E04),
 * FUN_000703E8's corner cells, the height > 0x70 anchor-time branch
 * ([0x157A6D]), the visual smoothing words 0x15780C/0E/10/12/14, the tracked
 * player cells 0x1577CA/CE and the event-rate bytes 0x157815/20/22 (the
 * FUN_00071C94 tail: player+0x20 sub-object, table 0x10E169), and the sound
 * sinks (FUN_00092820 etc.). */
int fifa96_camera_event_set(fifa96_camera *cam, int16_t seed_x, int16_t seed_z,
                            int16_t height);
