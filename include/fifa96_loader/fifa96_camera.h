#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

struct fifa96_rng;

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
  /* FU-148 §2.1(c)/§5.2 + FU-152 §2.9 (T2): the FUN_000719C4/FUN_000700F4
   * event cells the pan bodies read (first-hand):
   * - event_suspended: [0x157A6C] != 0 -> FUN_00071C94 returns at 0x71C99;
   * - ramp_divisor: [0x1577F8].lo = F8, the fast-path velocity divisor, and
   *   the stored step pair [0x1577BA/BC] = vel * F8;
   * - pan_counter: [0x157821], advanced by FUN_000709D0 (cap 100) and the
   *   fast/slow path selector (fast only while 0);
   * - pan_decay: [0x157819], FUN_000709D0's height decay rate (/0x20);
   * - pan_rate_hi/lo: [0x15781A]/[0x15781B], the slow-path velocity scale
   *   k (/0x20) for the height > 0 / height <= 0 arms. The three rate bytes
   *   are the FUN_000700F4 table (0x1104AB + [0x14C2FE]*8 + [0x14C2FA]*2)
   *   image defaults 10/16/8. */
  uint8_t event_suspended;
  int16_t ramp_divisor;
  uint8_t pan_counter;
  uint8_t pan_decay;
  uint8_t pan_rate_hi;
  uint8_t pan_rate_lo;
  int16_t event_step_x, event_step_z;   /* [0x1577BA]/[0x1577BC] */
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

/* FU-152 §2.8 (P4): the 0x107508 camera-type record cells the pose feed reads
 * (native +4 handler selector, +0xC behavior index). The poser maps a +4 == 3
 * to behavior index 5 and clamps negative selectors to 0; the image's -1
 * behavior cells are the degenerate `[0x108B60]` read the engine clamps to 0
 * (slice §2.8 leg 8). */
typedef struct fifa96_camera_type_record {
  int32_t handler;   /* +4 */
  int32_t behavior;  /* +0xC of that record */
} fifa96_camera_type_record;

/* Resolve {handler, behavior} for the staged type records: handler =
 * clamp(records[selector].handler, 0); the behavior record index is the
 * handler remapped 3 -> 5, whose `.behavior` is clamped >= 0. Returns
 * FIFA96_OK, -FIFA96_ERR_INVALID (NULL out-params or selector out of range). */
int fifa96_camera_type(const fifa96_camera_type_record *records, uint32_t count,
                       uint32_t selector, int32_t *handler, int32_t *behavior);

/* FU-152 §2.8 (P4): the four 0x108B80 handler bodies (0x4E834 steady, 0x4E3A8
 * sidetrack, 0x4EC9C staged, 0x4DB38 action). The engine ports the quoted
 * constant/clamp level; the integrator helpers FUN_0004E248/D698/DF34/C7D0/
 * D668 and the pose targets stay leg 9. `handler_block` carries the behavior
 * block fields the handlers read: +0x08 class_of, +0x00 target0, +0x04
 * target1, +0x30 const30 (native dword 0xC), +0x34 const34 (dword 0xD), the
 * +0xC dword (`field3` = native `param_2[3]`, the FUN_0004EC9C class-gate
 * cell), and the sub-record yaw bounds (native `[block+0x50]+0x18/+0x1C`). */
typedef struct fifa96_camera_handler_block {
  int32_t class_of;   /* +0x08 */
  int32_t target0;    /* +0x00 */
  int32_t target1;    /* +0x04 */
  int32_t const30;    /* +0x30 (dword 0xC) */
  int32_t const34;    /* +0x34 (dword 0xD) */
  int32_t field3;     /* the dword at +0xC (block[3], class-gate cell) */
  int32_t yaw_lo;     /* sub-record +0x18 */
  int32_t yaw_hi;     /* sub-record +0x1C */
} fifa96_camera_handler_block;

typedef struct fifa96_camera_handler_state {
  int32_t pos_x, pos_y, pos_z;       /* camera record +0x10/+0x14/+0x18 */
  int32_t track_x, track_y, track_z; /* the staged replay triple +0x34/38/3C */
  int32_t ratio;                     /* +0x4C */
  int32_t yaw;                       /* +0x58 */
  int32_t pitch;                     /* +0x5C */
  int32_t brake_lo, brake_hi;        /* +0x5A/+0x5E */
} fifa96_camera_handler_state;

typedef struct fifa96_camera_handler_out {
  int32_t horizon_a;   /* 0x14E4D8 (h0) / 0x14E4DC (h1/h2) */
  int32_t horizon_b;   /* 0x14E4D4 (h0) / 0x14E4D0 (h1/h2) */
  int32_t speed;       /* the action handler's param_4 (after the live bump) */
  int32_t class_of;    /* the staged handler's native param_2[2] (3 or 4) */
} fifa96_camera_handler_out;

/* Handler constants (fresh 0x4DB38): the yaw slew divisor 0x1E0000 and the
 * pitch slew divisor 0x3C0000. */
#define FIFA96_CAMERA_ACTION_YAW_DIVISOR 0x1E0000
#define FIFA96_CAMERA_ACTION_PITCH_DIVISOR 0x3C0000

int fifa96_camera_behavior_steady(fifa96_camera_handler_state *st,
                                  const fifa96_camera_handler_block *block,
                                  int32_t rnd, fifa96_camera_handler_out *out);
int fifa96_camera_behavior_sidetrack(fifa96_camera_handler_state *st,
                                     const fifa96_camera_handler_block *block,
                                     int32_t rnd, fifa96_camera_handler_out *out);
int fifa96_camera_behavior_staged(fifa96_camera_handler_state *st,
                                  const fifa96_camera_handler_block *block,
                                  int32_t rnd, fifa96_camera_handler_out *out);
int fifa96_camera_behavior_action(fifa96_camera_handler_state *st, int live,
                                  int32_t speed, fifa96_camera_handler_out *out);

/* FU-152 §2.8 (P4): the six 0x108B64 behavior-block fields the handlers read,
 * first-hand `read_memory 0x10896C` (504 B): +0x00 target0, +0x04 target1,
 * +0x08 class, +0x30 const30 (dword 0xC), +0x34 const34 (dword 0xD), and the
 * +0xC dword (`field3` = native `param_2[3]`, the FUN_0004EC9C class-gate
 * cell). Observed classes {3,1,3,3,1,3}; the +0xC cells are
 * {0,0,0xA7F8,0,0,0xA21C}, so blocks 2/5 take FUN_0004EC9C's class-3 arm
 * (0x4000..0xC000) and blocks 0/1/3/4 the class-4 arm; every +0x34 is 0
 * (handler 1's yaw target is the bare staged track z). */
typedef struct fifa96_camera_behavior_table_entry {
  int32_t target0;   /* +0x00 */
  int32_t target1;   /* +0x04 */
  int32_t class_of;  /* +0x08 */
  int32_t field3;    /* the dword at +0xC (block[3]) */
  int32_t const30;   /* +0x30 */
  int32_t const34;   /* +0x34 */
} fifa96_camera_behavior_table_entry;

#define FIFA96_CAMERA_BEHAVIOR_BLOCKS 6
extern const fifa96_camera_behavior_table_entry
    fifa96_camera_behavior_blocks[FIFA96_CAMERA_BEHAVIOR_BLOCKS];

typedef struct fifa96_camera_pose_args {
  uint32_t view_mode;   /* [0x14E57C] (getter FUN_00053D50) */
  uint32_t block;       /* behavior index (native [0x107514 + selector*0x70]) */
  uint32_t selector;    /* camera record selector [camera+4] (0..3 pose feed) */
  int32_t variant_x;    /* mode 3/4 variant predicate x (native *0x109A70) */
  int32_t sub;          /* replay sub index ([0x109A70+8]) */
  int mirror;           /* FUN_0004B818: 1 - side (caller-staged) */
  /* FU-152 §2.8 (P4): the default-arm handler call `(&0x108B80)[selector]`.
   * A caller stages the camera-record subset + behavior block; NULL keeps the
   * unported default (returns 0). `handler_rnd` feeds the horizon jitter,
   * `handler_live`/`handler_speed` the action handler's `FUN_0006400C` arm. */
  fifa96_camera_handler_state *handler_state;
  const fifa96_camera_handler_block *handler_block;
  fifa96_camera_handler_out *handler_out;
  int32_t handler_rnd;
  int32_t handler_live;
  int32_t handler_speed;
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
 * modes) dispatches `(&0x108B80)[selector]` when the caller stages the
 * camera-record subset + behavior block (FU-152 §2.8 P4); a NULL
 * `handler_state` keeps the unported result (0). Legs: the +0x48 alternate
 * array, the mode-3 replay camera copy/z clamp, the mode-4/8 mirror tails,
 * the mode-0x15 camera+0x3c clamp, the `[0x107DD8]`/pad-idle/replay gates of
 * the per-frame driver FUN_0004D2D4, and the handler integrator math (leg 9).
 * Returns 1 when a pose was applied, 0 for the unported default arm,
 * -FIFA96_ERR_INVALID (NULL or block out of range). */
int fifa96_camera_pose_feed(fifa96_camera *cam, int32_t *yaw, int32_t *pitch,
                            int32_t *view_ratio, const fifa96_camera_pose_args *args);

/* FU-148 §2.1(c)/§5.2 (S4): the FUN_000702F8 pan height ramp (EDX passthrough
 * dropped): 0x94 at >= 0x640, 3 below 1, else `0x94 - table0x10F4EE[(0x640-h)
 * >> 2]` clamped >= 0. The 400-byte table is the static image table
 * FUN_0006FFC0 installs at [0x157748]. */
int16_t fifa96_camera_ramp(int16_t height);

/* FU-148 §2.1(c)/§5.2 (S4) + FU-152 §2.9 (T2): the FUN_00071C94 +
 * FUN_00070544 event setter — the camera cut/event setter that feeds the
 * FU-71 rate words. Preserves the target (= the engine position; the native
 * reset 0x700F4 is handed the current target triple), resets the event state,
 * copies the caller seed step pair (the native 6-byte vec seed at 0x1577B8:
 * {bearing, step_x, step_z}), clamps the height to [target y, 0x640], runs
 * the ramp (F2/F4/F6/F8) with the caller's FUN_00070544 sign param
 * (`ramp_param != 0` -> F6 = F2 + ramp(h - ty), the row-04 arm-A register;
 * 0 -> F6 = F2 - ramp(h - ty)), computes the velocity (fast path
 * `seed / F8` when F8 != 0 and pan_counter == 0, else the FUN_00070544 slow
 * path scaling each nonzero velocity by k/0x20), recomputes the bearing, then
 * runs the > 0x19 atan walk (FUN_000CD474 + the 0x114E04 sine fold + the
 * FUN_000795A4 product) which caps the velocity magnitude to 0x19 in the
 * current direction, and builds the anchor A/B sets (0x157770/88/94).
 * Returns FIFA96_OK on the applied path, 1 when the [0x157A6C] bail gate is
 * set (the native early return, 0x71C99), -FIFA96_ERR_INVALID on NULL.
 * Legs (OL-T11-79): the height > 0x70 [0x157A6D] anchor-B branch,
 * FUN_000703E8's corner cells, the FUN_00065CF8/FUN_00092820 sound sinks,
 * the tracked-player/event-rate tail ([player+0x20], table 0x10E169) and the
 * visual smoothing words 0x15780C/0E/10/12/14. */
int fifa96_camera_event_set(fifa96_camera *cam, int16_t seed_x, int16_t seed_z,
                            int16_t height, int ramp_param);

/* FU-152 §2.9 (T2): FUN_000709D0, the timer-driven pan step FUN_000736AC
 * calls when `timer > timer_limit`. Bands the event height into the
 * FUN_00065CF8 sink arguments (returned through `class_of`/`param`, left
 * untouched at height 0), advances [0x157821] (cap 100), decays the height by
 * pan_decay/0x20, runs the ([0x14C1D4]|[0x14C1D6]) & 4 random walk when
 * `walk_gate & 4` (the atan direction + the rng low byte jitter, velocity
 * magnitude kept; an absent `rng` skips the jitter and is a leg), then re-runs
 * FUN_00070544(0) over the stored step pair. Returns FIFA96_OK or
 * -FIFA96_ERR_INVALID (NULL cam). */
int fifa96_camera_pan_step(fifa96_camera *cam, struct fifa96_rng *rng,
                           int walk_gate, int32_t *class_of, int32_t *param);

/* FU-152 §2.9 (T2): FUN_00070DE0's derived core — the boundary/reposition
 * handler the armer FUN_0007131C calls at 0x7134F. Classifies the previous
 * (0x157758/5C/60) and current triples; on a nonzero flag overlap it is a
 * no-op (returns 0). Otherwise, when the XOR has bit 0x8 clear, it walks from
 * the previous point toward the current one (FUN_0008DC50(delta, 1) per axis)
 * while the candidate keeps the previous flags (the adopted point is the
 * farthest one whose classification still equals the previous triple's), then
 * applies the mask effects: current-flag bits 3 negate-quarter vel_x, bit 4
 * zeroes vel_z, bit 0x10 jitters y (the FUN_00070B94 elevation loop is a leg),
 * and recomputes the bearing. Returns 1 when the walk/reposition ran, 0 for
 * the overlap no-op, -FIFA96_ERR_INVALID on NULL. `type_off_x`/`type_off_z`
 * are the caller-staged 0x10F334/3C tables (unused by the derived core).
 * Legs (OL-T11-79): the bit-0x8 boundary arm (target z +-0xB11/+-0xB0F, the
 * 0x8ED40/0x8F188 corner events -> `events`/`event_code`, the target-y clamp),
 * the FUN_000974DC/FUN_000651F0/FUN_000974F0 sound sinks, the elevation loop
 * and the corner record classification. */
int fifa96_camera_reposition(fifa96_camera *cam, int32_t prev_x, int32_t prev_y,
                             int32_t prev_z, const int8_t *type_off_x,
                             const int8_t *type_off_z, uint8_t *events,
                             uint8_t *event_code);

/* FU-152 §2.9 (T2): FUN_00071DF4's table/keeper arm (derived). Gates:
 * signed rate_byte >= 0, timer <= 0x1E, then when `subobj_present` the
 * caller-resolved rate pair (the 0x10E169 + 0x11042B/0x11042C lookup is a
 * leg) is written to rate_x/rate_z. The keeper gate (event_byte == 2, event
 * height >= 0xC1, |anchor_x| <= 0x23F, rate_z != 0) emits 0x1D for the
 * approaching cases and 0x1E otherwise through `keeper_code` (0 = none).
 * Returns 1 when the rates were applied or a keeper code emitted, 0 when
 * nothing applied, -FIFA96_ERR_INVALID (NULL cam/keeper_code). */
int fifa96_camera_rate_table(fifa96_camera *cam, int32_t event_height,
                             int32_t rate_byte, int32_t timer,
                             int32_t subobj_present, int8_t rate_x, int8_t rate_z,
                             int32_t event_byte, int32_t anchor_x, int32_t pos_z,
                             int32_t vel_z, uint8_t *keeper_code);

/* FU-152 §2.9 (P4): FUN_00070074's boundary classifier over the staged triple
 * {x, y, z}: |z| < 0xB10 -> 8, < 0xB90 -> 0, else 4; x < -0xD0 -> |1, x >
 * 0xCF -> |2; the z-window 0x10 bit when y exceeds the shrinking 0xA0
 * threshold within 0x30 past 0xB10. Returns flags == 0 (the native "inside"
 * result), writes *flags; -FIFA96_ERR_INVALID on NULL. */
int fifa96_camera_classify(const int32_t point[3], uint16_t *flags);

/* FU-152 §2.9 (P4): FUN_000709D0's pan-height band ladder (first-hand):
 * h < 0x29 -> (class 1, 2h+0x14); h < 0x65 -> (class 2, (h+0x50)/2);
 * h < 0x12D -> (class 3, (h+100)/4); else (class 3, 100). */
void fifa96_camera_pan_band(int32_t height, int32_t *class_of, int32_t *param);

/* FU-152 §2.9 (P4): FUN_00071DF4's first arm — a tracked sub-object at
 * height > 0xF0 writes the event rates `sub_object_rate * 15` clamped +-15
 * and recomputes the bearing magnitude. Returns 1 when applied, 0 when the
 * height gate fails, -FIFA96_ERR_INVALID on NULL. The keeper/player event
 * tail (FUN_00092998 -> FUN_0008F188 0x1D/0x1E) and the table arm stay legs. */
int fifa96_camera_rate_event(fifa96_camera *cam, int32_t rate_x, int32_t rate_z,
                             int32_t height);

/* FU-152 §2.3 (P4): FUN_0004D134's replay camera selector (fresh decompile):
 * 0 -> the `0x107968 + [0x107DD0]*0x70` record with sub 0; 1/2/3/6 -> the
 * 0x107CE8 record with sub 5/6/7/8 (FUN_0004DDA8); 4 -> 0x107B98 (the action
 * handler, FUN_0004DB38); 5 -> 0x107B28 (FUN_0004D98C). The native default
 * arm only records the index. Returns 1 when a camera was selected, 0 for the
 * default; -FIFA96_ERR_INVALID on NULL out-params. The per-mode pose
 * semantics (FUN_0004D98C/DDA8) stay leg 10. */
enum {
  FIFA96_CAMERA_REPLAY_POSE = 0,   /* [0x107968 + [0x107DD0]*0x70] */
  FIFA96_CAMERA_REPLAY_VIEW = 1,   /* 0x107CE8 */
  FIFA96_CAMERA_REPLAY_ACTION = 2, /* 0x107B98 */
  FIFA96_CAMERA_REPLAY_BALL = 3,   /* 0x107B28 */
};
int fifa96_camera_replay_select(uint32_t index, int32_t *kind, int32_t *sub);
