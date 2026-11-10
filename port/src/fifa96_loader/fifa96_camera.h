#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

struct fifa96_rng;

typedef struct fifa96_camera {
  int32_t pos_x, pos_y, pos_z;
  /* FU316-L2: the native `[0x15775C]` landing cell — the previous frame's
   * camera Y, copied by the integrator before the 0x70B94 height update
   * (`0x73756`). `0x737C8..0x737DA` admits the pan step when
   * `prev_y > 0 && pos_y <= 0` even if the timer has not passed its limit. */
  int32_t prev_y;
  /* FU-71 leg 9.6 (phase-51 (FU-71)): the native `[0x157758]`/`[0x157760]` cells —
   * the previous frame's camera X/Z. `0x73756` calls `FUN_00072AC4` and the
   * three `MOVSD` at `0x7375b..0x7375d` copy the whole live triple
   * `0x15774C/50/54` to `0x157758/5C/60`, so `prev_x`/`prev_z` are captured
   * with `prev_y` in `fifa96_camera_update_walk`. The far-wall arm reads them
   * in its corridor checks (`|prevZ| >= 0xE74`, `|prevX| >= 0x8BC`,
   * `|prevX| >= 0x6D4`). Init zeroes them; the native init 0x700F4 is silent
   * on the prev triple, so the image-default zero is the fresh value. */
  int32_t prev_x;
  int32_t prev_z;
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
  /* FU303/FU294-L2 (phase-28 T8): the event-set pan-direction byte
   * `[0x157815]`. The native writers are FUN_00071C94's four stores
   * (0x71d6a default 0, 0x71db1 0xFF, 0x71d9e/0x71ddc the 0x10E169 compass
   * row or the 0x8DD70 bearing oct) and the init 0x702e6 (DL = 0); its only
   * readers are FUN_00071DF4's signed gate and its `<<3` row selector
   * (0x71ef0/0x71f35). `fifa96_camera_event_set` writes the default 0;
   * `fifa96_camera_event_dir` applies the actor arm. */
  int8_t dir_157815;
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
   *   image defaults: decay 10, hi 16, and lo = hi + (0x20 - hi)/2 = 24
   *   (0x70137..0x70146). */
  uint8_t event_suspended;
  int16_t ramp_divisor;
  uint8_t pan_counter;
  uint8_t pan_decay;
  uint8_t pan_rate_hi;
  uint8_t pan_rate_lo;
  int16_t event_step_x, event_step_z;   /* [0x1577BA]/[0x1577BC] */
  /* FU-152 §2.9 / FU-145 §1.2 (M2 phase-9 T3): [0x1577CA], the camera/stats
   * tracked entity (a pool record). Writers (fresh get_xrefs_to = 88, three
   * WRITE): FUN_00071C94 0x71D27 stores the event's actor record, and the two
   * resets clear it — FUN_000700F4 0x70258 and FUN_0006FFC0 0x70054 (the
   * `XOR EAX,EAX` store at 0x4C, reachable from FUN_00073D90 0x73D9B and
   * FUN_00088860 0x88883); every reader dereferences it as a record
   * (`[[rec]+4]` class, `[[rec]+0x20]` slot). The every-frame FUN_00071DF4
   * call site (FUN_000736AC 0x73B5B..0x73B6B) requires `tracked != 0` and
   * `[[rec]+0x20] != 0`; the first arm follows the tracked record's slot dir
   * bytes when the event height exceeds 0xF0. FUN-camera_init (the 0x700F4
   * port) clears it. `FIFA96_CAMERA_TRACKED_NONE` is the native NULL. */
  int32_t tracked;
  /* FU-152 §2.9 (T3): the derived `[0x1577BE]` value at the FUN_000736AC
   * 0x739CE follow gate point. The native gate sits after the pan step and
   * before the tail's `[0x577BE] = metric(vel)` recompute (0x73C38), so it
   * reads the pan step's speed when the step ran this frame, else the speed
   * the previous frame's tail left. `fifa96_camera_update_walk` captures it;
   * `fifa96_match_run_camera_follow` gates on it. */
  int16_t follow_speed;
  /* M2 phase-11 T1 (FU-148 §14.5 / L14.7): `word[0x1577F2]` = F2, the event
   * ramp's peak time. FUN_00070544 computes it (`F2 = FUN_000702F8(event
   * height)`); FUN_00070B94 reads it as the camera-height arc's apex. The
   * engine folds the stored F2 here at the event set (both the height and the
   * idle arm; the native idle arm stores 6). No earlier phase modeled the
   * cell, so `fifa96_camera_height` (the 0x157750 producer) needs it. */
  int16_t event_peak;
  /* FU303/FU292-L2 (phase-28 T8): `word[0x1577F6]` = F6, the ramp start
   * (`F2 +/- ramp(height - target_y)`, 0x705c4). The cell is not stored
   * natively as an independent producer result: `F8 = F4 - F6` (0x705e8), so
   * F6 = `timer_limit - ramp_divisor` exactly on both arms (the idle arm's
   * F6 is the 0x70544 argument, 0 on the engine path). `camera_event_ramp`
   * folds it here so the Seam A can bind the (FU303-L10) cell. */
  int16_t band_1577f6;
} fifa96_camera;

#define FIFA96_CAMERA_TRACKED_NONE (-1)

int fifa96_camera_init(fifa96_camera *cam, int32_t x, int32_t y, int32_t z);
/* `cursor_high` is the native `[0x14C2F6] != 0` selector at 0x73A80: the
 * event-cursor interpolation threshold is 0x90 when set, 0x70 when clear. */
int fifa96_camera_update(fifa96_camera *cam, int16_t delta, int cursor_high,
                         int input_bit2);

/* FU-148 §12.2 / FU-145 §1.8 (M2 phase-9 T3): the native FUN_000736AC in-line
 * update with the ([0x14C1D4]|[0x14C1D6]) & 4 walk gate and the match RNG
 * threaded to the pan step. `fifa96_camera_update` is the gate-0 wrapper (the
 * pre-T3 frame path); the engine computes the gate from its range words. */
int fifa96_camera_update_walk(fifa96_camera *cam, struct fifa96_rng *rng,
                              int16_t delta, int cursor_high, int input_bit2,
                              int walk_gate);

/* FU-152 §2.9 (T3): FUN_00071C94 0x71D27's `[0x1577CA] = rec` write. The id
 * is the engine pool id (team*11 + index) or FIFA96_CAMERA_TRACKED_NONE.
 * Returns FIFA96_OK, -FIFA96_ERR_INVALID on NULL or an id < -1. */
int fifa96_camera_set_tracked(fifa96_camera *cam, int32_t id);

int fifa96_camera_target(const fifa96_camera *cam, int16_t *x, int16_t *z);
int fifa96_camera_reflect(fifa96_camera *cam, int input_bit0, int transition_active);
int fifa96_camera_out_of_bounds(int32_t x, int32_t z);

/* FU-71 leg 9.6 (phase-51 (FU-71)): the far-wall damp/event arm, the native
 * `0x718A9..0x71B98` non-mirror branch of `FUN_0007131C`. Reached from the
 * `0x718A9` gate (`(int16)|camZ|_w > 0xB10 || (int16)|camX|_w > 0x720`) when
 * `[0x15781E] != 0` (`0x718EF`) or `((word[0x14C1D4]|word[0x14C1D6]) & 1) ==
 * 0` (`0x718FF`) — the dispatch bit is the front-end option row 2
 * (INVISIBLE WALLS: 0 = Disabled = this arm, 1 = Enabled = the mirror), so the
 * arm is the **default** path and the mirror is the option-enabled one.
 *
 * Body (first-hand `disassemble_bytes 0x719A0`, 148 instructions):
 *  - head `0x719A0`: `event_param == 0 && speed == 0` returns with no writes;
 *  - chain A (Z) `0x719CF..0x71A5A`: `(int16)|camZ|_w >= 0xE74`; when
 *    `height < 0x24` and `|prevZ| < 0xE74` it damps `vel_z =
 *    shift_step((int16)(-vel_z), 1)` instead; otherwise the threshold
 *    `t = 0x20` for `camZ < 0` else `((int16)((int16)(|camZ|_w - 0xE74)) >> 1)
 *    + 0x24` must satisfy `t >= height`, then `seed_z = ±0x180` and the
 *    travel height `= (int16)(word[pos_y] + 0x40)`;
 *  - chain B (X) `0x71A5D..0x71AF0`: `(int16)|camX|_w >= 0x8BC`; damps when
 *    `height <= 0x24` and `|prevX| < 0x8BC`, else the threshold `t = 0x20` for
 *    `camX >= 0` else the distance formula, `t >= height` -> `seed_x = ±0x180`;
 *  - small X `0x71AF2`: `0x6D4 <= |camX|_w < 0x8BC && height < 0x1E &&
 *    |prevX| < 0x6D4` damps `vel_x` (no flag change);
 *  - tail `0x71B2C`: flag 0 -> the `0x71972` store pair + `speed =
 *    (uint16)hypot(vel_x, vel_z)`; flag 1 -> `0x71B37` announce (the `0x974DC`
 *    request is 1 for entry speed 6..8, 2 for 9..0xA, 3 for >0xA, and skipped
 *    for <= 5) then the `0x71B60..0x71B8A` seed triple and `0x71C94` call.
 *
 * The seed pair is two laterals: `word[1] = seed_x`, `word[2] = seed_z`; the
 * native also stores `word[0] = CD514(seed_z, seed_x)` at `0x1577B8`, which no
 * consumer reads (FU168-L2), so it is not computed here.
 *
 * Returns 1 when the flag fired (the three chain outputs are filled and the
 * damped velocity words are **discarded**, exactly like the native, whose
 * `0x71C94` resets the camera and re-derives the velocities from the seeds),
 * 0 for the head return, a failed `0x718A9` gate, or the flag-0 tail (which
 * already stores `vel_x`/`vel_z` and the recomputed `speed`), and
 * -FIFA96_ERR_INVALID on NULL. */
typedef struct fifa96_camera_wall_event {
  int16_t seed_x;    /* chain B lateral (±0x180), 0 when unset */
  int16_t seed_z;    /* chain A lateral (±0x180), 0 when unset */
  int16_t height;    /* the 0x71C94 EBX arg = (int16)(word[pos_y] + 0x40) */
  uint8_t announce;  /* the 0x974DC request: 0 = none, else 1..3 */
} fifa96_camera_wall_event;

int fifa96_camera_wall_arm(fifa96_camera *cam, fifa96_camera_wall_event *out);

/* M2 phase-11 T1 (FU-148 §14.5 / L14.7): FUN_00070B94 (`0x70B94..0x70C05`,
 * first-hand `disassemble_bytes`) — the camera-height producer behind the
 * native `word[0x157750]`:
 *
 *   if ((int16)timer >= (int16)limit) return 0;
 *   if ((int16)height <= 0) return 0;
 *   y = max(0, T16[0x94 - |F2 - timer|] - T16[0x94 - F2])
 *
 * where T16 is the 149-word table at flat `0x10F3C4` (first-hand
 * `read_memory`, 298 B; FU-148 §17 corrected the window), height =
 * `word[0x1577F0]` (the engine `event_param`), timer = `word[0x1577FA]`
 * (the engine `timer`), limit = `word[0x1577F4]`
 * (`timer_limit`) and F2 = `word[0x1577F2]` (`event_peak`). FUN_000736AC
 * `0x737a1..0x737b4` stores the result into `0x157750` (the camera Y = the
 * engine `pos_y`) when the event height is nonzero. Callers: the engine's
 * `fifa96_camera_update_walk` (the native integrator store site). */
int16_t fifa96_camera_height(const fifa96_camera *cam);

/* FU-153 / M2 phase-12 T1 (15.a): the shared `[0x157744]` arc sample —
 * `T[0x94 - |peak - timer|] - T[0x94 - peak]`, clamped >= 0, over the same
 * 149-word `0x10F3C4` table FUN_00070B94 reads. The record jump arc
 * (FUN_0007BF20 `0x7C0BF..0x7C113`, first-hand `disassemble_bytes`) computes
 * its `+0x5D` height with exactly this formula, peak = `word[+0x85]` (the
 * arm-staged ramp time) and timer = `word[+0x87]` (the running time). */
int16_t fifa96_camera_arc_height(int16_t peak, int16_t timer);

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
 * sidetrack, 0x4EC9C staged, 0x4DB38 action). The steady handler runs the
 * FUN_0004E248 pan block, the two FUN_0004D698 slides, the FUN_0004DF34 yaw
 * step, the FUN_0004E834 yaw snap, the FUN_0004E05C pitch arm, the y/ratio
 * eases and the target-history copy. `handler_block` carries the behavior
 * block fields the handlers read: +0x08 class_of, +0x00 target0, +0x04
 * target1, +0x10 const10, +0x18 const18, +0x1C const1c, +0x20 const20,
 * +0x24 const24, +0x28 const28, +0x30 const30 (native dword 0xC), +0x34
 * const34 (dword 0xD), +0x38 const38, +0x3C const3c, +0x40 const40, the +0xC
 * dword (`field3` = native `param_2[3]`, the FUN_0004EC9C class-gate cell),
 * and the +0x50 sub-record (`c0..c3`, `sub10`, `sub14`, `sub18`, `sub1c`).
 * Handlers 0 and 2 mutate +0x18/+0x1C/+0x34/+0x38; handler 2 also writes
 * +0x08/+0x10 and the sub-record +0x18. Those cells live in the caller's
 * mutable copy, never in the const table row. */
typedef struct fifa96_camera_handler_block {
  int32_t class_of;   /* +0x08 */
  int32_t target0;    /* +0x00 */
  int32_t target1;    /* +0x04 */
  int32_t const10;    /* +0x10 (the x history-offset bound) */
  int32_t const18;    /* +0x18 (the z target offset initial value) */
  int32_t const1c;    /* +0x1C (the initial +0x18 step cell) */
  int32_t const20;    /* +0x20 (the FUN_0004E05C pitch rate) */
  int32_t const24;    /* +0x24 (the FUN_0004DF34 yaw rate) */
  int32_t const28;    /* +0x28 (the FUN_0004D668 slide rate) */
  int32_t const30;    /* +0x30 (dword 0xC) */
  int32_t const34;    /* +0x34 (dword 0xD; initial pan cell) */
  int32_t const38;    /* +0x38 (the initial pan-step cell) */
  int32_t const3c;    /* +0x3C (the +0x34 pan clamp) */
  int32_t const40;    /* +0x40 (the FUN_0004E248 pan rate) */
  int32_t field3;     /* the dword at +0xC (block[3], class-gate cell) */
  int32_t c0, c1, c2, c3; /* +0x50 record: x lo/hi, z lo/hi */
  int32_t sub10;      /* the +0x50 record +0x10 (x history offset) */
  int32_t sub14;      /* the +0x50 record +0x14 (FUN_0004A1A60 scale) */
  int32_t sub18, sub1c;   /* +0x50 record +0x18/+0x1C (the +0x18 clamp) */
  /* The native block is writable state. These four cells are the ones the
   * handlers mutate; a caller must pass a mutable copy of the table row, not
   * the const `fifa96_camera_behavior_blocks` image. */
  int32_t cell18;     /* +0x18 (mutated by the pan step) */
  int32_t cell1c;     /* +0x1C (the last pan step) */
  int32_t pan;        /* +0x34 (mutated by the pan step) */
  int32_t pan_step;   /* +0x38 (the last pan step) */
} fifa96_camera_handler_block;

typedef struct fifa96_camera_handler_state {
  int32_t pos_x, pos_y, pos_z;       /* camera record +0x10/+0x14/+0x18 */
  int32_t track_x, track_y, track_z; /* the staged replay triple +0x34/38/3C */
  int32_t ratio;                     /* +0x4C */
  int32_t yaw;                       /* +0x58 */
  int32_t pitch;                     /* +0x5C */
  int32_t hist_x, hist_y, hist_z;    /* +0x40/+0x44/+0x48 (target history) */
  int32_t view_variant;              /* [0x107DD4] 0/1 (FUN_0004D04C) */
  /* FU351-L2c-1: the computed 0x51056 handler arguments. The live
   * FUN_0004D908 driver passes (0,0); the FUN_000505D0 pose path stages
   * FUN_0004B8D4's side result and the 0x1577C2 lead word. */
  int32_t pan_axis_a;                /* native EBX (FUN_0004B8D4 result) */
  int32_t pan_axis_b;                /* native ECX (lead_z) */
  int32_t axis_ac2;                  /* staged [0x157AC2] */
  int32_t axis_present;              /* [0x157A83] != 0 */
  int32_t axis_roster;               /* [rec+0x8D] */
  int32_t axis_side;                 /* [rec+0x826] */
  int32_t axis_lead_z;               /* [0x1577C2] sign-extended */
  int32_t axis_latch;                /* [0x1074A8], persistent */
} fifa96_camera_handler_state;

typedef struct fifa96_camera_handler_out {
  int32_t horizon_a;   /* 0x14E4D8 (h0) / 0x14E4DC (h1/h2) */
  int32_t horizon_b;   /* 0x14E4D4 (h0) / 0x14E4D0 (h1/h2) */
  int32_t speed;       /* the action handler's param_4 (after the live bump) */
  int32_t class_of;    /* the staged handler's native param_2[2] (3 or 4) */
} fifa96_camera_handler_out;

/* FU-351-L1 (fresh 0x36C70, head): the live focus triple staged at
 * `0x155AA4/8/C` — the camera-relative basis, separate from the class-3
 * view pose. The source is `[0x157A6F]` (a camera-triple override seeded from
 * `0x15774C`; FU-154 §2.1) when it is set and `FUN_00063FD0()` (the replay
 * bit `[0x109A98] & 0x80`) is clear, else the FU-71 triple
 * `0x15774C/50/54`. The native body:
 *
 *   if (|x| > 6000 || |z| > 6000) { x = -4000; y = 0; z = -4000; }
 *   if (y < 0 || y > 16000) y = 0;
 *
 * (`0x36C76..0x36CFA`; the decompiler's 6000/16000 are signed dword compares).
 * The focus feeds the class-3 target on a view change (`FUN_0004D04C 0x4D0A3`
 * copies `[0x109A70]` into `camera+0x34/38/3C`), so it is the native live
 * target producer. Returns FIFA96_OK or -FIFA96_ERR_INVALID on NULL. */
typedef struct fifa96_camera_focus {
  int32_t x, y, z;
} fifa96_camera_focus;

int fifa96_camera_focus_clamp(int32_t x, int32_t y, int32_t z,
                              fifa96_camera_focus *out);

/* FU-351-L1: FUN_0004BE44's 23-slot table index for a record
 * (`0x4B99F..0x4B9FF`). `roster` is the record's byte `+0x8D`
 * (`[rec+0x8A] >> 24`), `side` its byte `+0x826`, and `ac2` the native
 * `[0x157AC2]` mode byte. The native adds `0xB` to one side:
 *
 *   if (ac2 & 1) side 0 gets +0xB when ac2 < 4, else side 1 does;
 *   if (!(ac2 & 1)) side 1 gets +0xB.
 *
 * The wrappers FUN_0004BAD8 (`[0x10F370]`) and FUN_0004BA00
 * (`[0x157A83]`, with the transient ac2=8 park when ac2 >= 5) are the
 * producer side; the row-0x17 installer is non-live (FU-201), so this
 * resolver is not wired into open play. */
int fifa96_camera_target_slot(int32_t roster, int32_t side, int32_t ac2);

/* FU351-L2c-1: FUN_0004B8D4 (`0x4B8D4..0x4B979`) — the side/axis result the
 * pose-feed handler call puts in EBX. `track_delta` is
 * `camera+0x3c - camera+0x48`; `event_slot` is FUN_0004BA00's 0..22 result
 * (or -1 when `[0x157A83]` is clear); `ac2` is `[0x157AC2]`. The native
 * writes the persistent `[0x1074A8]` latch through `latch`. Returns -1/0/1. */
int32_t fifa96_camera_pose_axis_a(int32_t track_delta, int32_t event_slot,
                                  int32_t ac2, int32_t *latch);

/* FU-351-L2: FUN_0004D668 (`0x4D668..0x4D695`) — the Q16 ramp factor
 * `floor(tick * 0x10000 / (rate * 15))`, where `tick` is the native
 * FUN_00049388 elapsed value and `rate` is the behavior block's `+0x28`
 * word (`param_2[10]`). The native body multiplies the rate by 15 in 32-bit
 * (`SHL EBX,4; SUB EBX,EAX`), shifts both operands by 16, and runs the
 * quotient/remainder IDIV pair; for the live positive domain this is the
 * floor above. `rate * 15` <= 0 or `tick <= 0` returns 0 (the native call
 * sites never pass those; the guard only keeps the division defined). */
int32_t fifa96_camera_ramp_factor(int32_t tick, int32_t rate);

/* FU-351-L2: FUN_0004D698 (`0x4D698..0x4D7E5`) — the one-axis slide the
 * class-0 handler runs for the camera x and z cells:
 *
 *   clamped = clamp(target - hist_offset, lo, hi);
 *   step    = ((clamped - cam) * ramp_factor(tick, rate) + 0x8000) >> 16;
 *   ... mode gates (-1/1/2) and the lo/hi clamp on cam + step;
 *
 * with the native mode constants `[0x108B90] = 0xA0` and `[0x108B94] = 1`
 * (both no-writer image cells, so mode 2's direction gate is skipped). The
 * strict inside `(lo, hi)` branch uses the block `rate`; the outside branch
 * takes the no-history path with a fixed rate 8 and does not clamp high.
 * `rate` is the behavior +0x28 word; `hist_offset` is the native param_7
 * (`+0x18` for z, the sign-adjusted target for x). */
int32_t fifa96_camera_slide(int32_t cam, int32_t target, int32_t hist,
                            int32_t mode, int32_t lo, int32_t hi,
                            int32_t hist_offset, int32_t rate, int32_t tick);

/* FU-351 §12: FUN_0004E248 (`0x4E248..0x4E3A5`) — the steady handler's
 * pan/roll step. The native call from `FUN_0004D908 0x4D932..0x4D948` is
 * `(camera, block, 0, 0)`; the computed-call path leaves the first two
 * register arguments caller-defined (numbered leg). The four Q16 factors are
 * `(tick * 20/32/32/rate * 0x1111 + 0x8000) >> 16`; the return is the
 * block+0x34 delta the handler adds before its +-block+0x3C clamp. */
int32_t fifa96_camera_steady_pan_step(int32_t axis_a, int32_t axis_b,
                                      int32_t pan, int32_t rate,
                                      int32_t track_z, int32_t tick);

/* FU-351 §12: FUN_0004E834 `0x4E9A2..0x4EA9B` — the class-3 yaw snap and
 * the non-class-3 mirror. The point is `(+-0x720, +-0xB60)`; the angle is
 * the negated FUN_0004C7D0 wrapper, the class-3 arm folds the full ratio,
 * the other arm the half ratio. Returns the raw 32-bit `angle +- ratio`
 * target the native stores in camera+0x58. */
int32_t fifa96_camera_yaw_snap(int32_t class_of, int32_t pos_x, int32_t pos_z,
                               int32_t track_z, int32_t yaw, int32_t ratio);

/* FU-351 §13: the 0x4E3A8/0x4EC9C pitch-target split. `track` is
 * `camera+0x34` for the sidetrack class-2 arm and `camera+0x3C` for the
 * staged class-3 arm; `horizon_a/b` are the two `(block[0] * -0xB/-0xD)>>3`
 * values. Both return the unclamped native dword:
 *
 *   a (class 2/3): track < 0 -> track - ((track>>1) + b);
 *                  else       -> track - (b + ((a-b)*track*0x23)>>16)
 *   b (other/4):   track <= 0 -> b + track + (((a-b)*-track*0x23)>>16);
 *                  else        -> (b + track) - (track>>1)
 *
 * The low 32 bits of the first product are used before the 0x23 multiply;
 * the `+0x8000 >>16` is the native SHRD rounding. */
int32_t fifa96_camera_pitch_target_a(int32_t track, int32_t horizon_a,
                                     int32_t horizon_b);
int32_t fifa96_camera_pitch_target_b(int32_t track, int32_t horizon_a,
                                     int32_t horizon_b);

/* FU-351 §13: the two handler snap arms, pure state transforms.
 *
 * `fifa96_camera_sidetrack_snap` (0x4E3A8 0x4E4A0..0x4E60E) is gated by
 * `track_x` (`< -0x360` for class 1, `< 0x360` otherwise), points at
 * `(+-0x720, pos_z +- 0x4B0)` (only the outer z side is clamped), and tests
 * the four `yaw`/ratio combinations against 0x4000..0xC000.
 *
 * `fifa96_camera_staged_snap` (0x4EC9C 0x4EEF2..0x4F00E) points at
 * `(+-0x720, +-0xB10)`, splits at the 0x2000..0xA000 band and uses the half
 * ratio; class 3 and class 4 swap the ratio sign. */
void fifa96_camera_sidetrack_snap(fifa96_camera_handler_state *st,
                                  const fifa96_camera_handler_block *block);
void fifa96_camera_staged_snap(fifa96_camera_handler_state *st,
                               int32_t class_of);

/* FU-351-L2: the native view preset/class resolver. `FUN_0004B308` maps the
 * mode byte `[0x157A4A] >> 24`, the options camera cell `[0x14C2F6]` and the
 * `[0x15B680]` selector to a preset:
 *
 *   phase 3  -> 1 when option == 1, else 2
 *   phase 4  -> 3 when option == 1, else 4
 *   phase 15 -> 5 when b680 == 3, 6 when b680 == 0, else 8
 *   else     -> 0
 *
 * `FUN_0004D04C` clamps the preset to 0..6 and selects the slot from
 * `{1,1,0,0,2,2,5}` and the variant from `{0,1,0,1,0,1,1}`; the selected
 * camera record's +0x04 class is `{slot 0:0, 1:1, 2:2, 5:3}` (slots 3/4 are
 * disabled). `class_of` is -1 only for a caller-visible slot 3/4, which the
 * table never selects. Returns FIFA96_OK or -FIFA96_ERR_INVALID on NULL. */
typedef struct fifa96_camera_view {
  int32_t preset;    /* 0..6 after the FUN_0004D04C clamp */
  int32_t slot;      /* 0/1/2/5 */
  int32_t variant;   /* 0/1 */
  int32_t class_of;  /* the camera record +0x04 class */
} fifa96_camera_view;

int fifa96_camera_view_select(int32_t phase, int32_t option, int32_t b680,
                              fifa96_camera_view *out);

/* Handler constants (fresh 0x4DB38): the yaw slew divisor 0x1E0000 and the
 * pitch slew divisor 0x3C0000. */
#define FIFA96_CAMERA_ACTION_YAW_DIVISOR 0x1E0000
#define FIFA96_CAMERA_ACTION_PITCH_DIVISOR 0x3C0000

/* FU-351-L2/§12: the steady handler's x/z cells run the two FUN_0004D698
 * slides, the yaw cell runs FUN_0004DF34, the pitch cell runs FUN_0004E05C,
 * and the pan/roll block runs FUN_0004E248. `tick` is FUN_00049388. The
 * handler mutates the native-writable block cells (`cell18/cell1c/pan/
 * pan_step`); callers pass a non-const copy. The native view driver calls it
 * with `(camera, block, 0, 0)`, so the pan inputs are zero on that path. */
int fifa96_camera_behavior_steady(fifa96_camera_handler_state *st,
                                  fifa96_camera_handler_block *block,
                                  int32_t tick,
                                  fifa96_camera_handler_out *out);
int fifa96_camera_behavior_sidetrack(fifa96_camera_handler_state *st,
                                     fifa96_camera_handler_block *block,
                                     int32_t tick, fifa96_camera_handler_out *out);
int fifa96_camera_behavior_staged(fifa96_camera_handler_state *st,
                                  fifa96_camera_handler_block *block,
                                  int32_t tick, fifa96_camera_handler_out *out);
int fifa96_camera_behavior_action(fifa96_camera_handler_state *st, int live,
                                  int32_t speed, int32_t tick,
                                  fifa96_camera_handler_out *out);

/* FU-152 §2.8 (P4) + FU-351-L2: the six 0x108B64 behavior-block fields the
 * handlers read, first-hand `read_memory 0x10896C` (504 B): +0x00 target0,
 * +0x04 target1, +0x08 class, +0x10 const10, +0x18 const18, +0x1C const1c,
 * +0x20 const20, +0x24 const24, +0x28 const28, +0x30 const30 (dword 0xC),
 * +0x34 const34 (dword 0xD), +0x38 const38, +0x3C const3c, +0x40 const40,
 * the +0xC dword (`field3` = native `param_2[3]`, the FUN_0004EC9C class-gate
 * cell), and the +0x50 record (`c0..c3`, `sub10`, `sub14`, `sub18`, `sub1c`)
 * plus its variant-1 copy at `+0x20` (`v1_c0..v1_c3`,
 * `v1_sub10..v1_sub1c`). Observed classes
 * {3,1,3,3,1,3}; the +0xC cells are {0,0,0xA7F8,0,0,0xA21C}, so blocks 2/5
 * take FUN_0004EC9C's class-3 arm (0x4000..0xC000) and blocks 0/1/3/4 the
 * class-4 arm; every +0x34 is 0 (handler 1's yaw target is the bare staged
 * track z). The four `cell18/cell1c/pan/pan_step` fields are the mutable
 * copies the steady handler updates. */
typedef struct fifa96_camera_behavior_table_entry {
  int32_t target0;   /* +0x00 */
  int32_t target1;   /* +0x04 */
  int32_t class_of;  /* +0x08 */
  int32_t field3;    /* the dword at +0xC (block[3]) */
  int32_t const10;   /* +0x10 */
  int32_t const18;   /* +0x18 initial cell */
  int32_t const1c;   /* +0x1C initial step */
  int32_t const20;   /* +0x20 pitch rate */
  int32_t const24;   /* +0x24 yaw rate */
  int32_t const28;   /* +0x28 slide rate */
  int32_t const30;   /* +0x30 */
  int32_t const34;   /* +0x34 initial pan */
  int32_t const38;   /* +0x38 initial pan step */
  int32_t const3c;   /* +0x3C pan limit */
  int32_t const40;   /* +0x40 pan rate */
  int32_t c0, c1, c2, c3; /* +0x50 record: x lo/hi, z lo/hi */
  int32_t sub10, sub14;   /* +0x50 record +0x10/+0x14 */
  int32_t sub18, sub1c;   /* +0x50 record +0x18/+0x1C */
  int32_t v1_c0, v1_c1, v1_c2, v1_c3; /* +0x20 variant-1 C record */
  int32_t v1_sub10, v1_sub14, v1_sub18, v1_sub1c;
  int32_t cell18, cell1c, pan, pan_step; /* native writable block state */
} fifa96_camera_behavior_table_entry;

#define FIFA96_CAMERA_BEHAVIOR_BLOCKS 6
extern const fifa96_camera_behavior_table_entry
    fifa96_camera_behavior_blocks[FIFA96_CAMERA_BEHAVIOR_BLOCKS];

/* Copy table row `index` into the mutable handler block. The native table is
 * shared image state; a caller that runs a handler must use this copy so the
 * pan/cell writes do not alias the const rows. Returns FIFA96_OK or
 * -FIFA96_ERR_INVALID (NULL or index >= 6). */
int fifa96_camera_behavior_block(fifa96_camera_handler_block *out,
                                 uint32_t index);
/* FUN_0004F1C8's variant overlay: target0 = 0x340/0xB4 and the C record is
 * the selected sub-record's +0x00 or +0x20 half. `variant` is [0x107DD4].
 * The row's mutable pan/cell fields are left as-is. */
int fifa96_camera_behavior_block_variant(fifa96_camera_handler_block *out,
                                         uint32_t index, int32_t variant);

typedef struct fifa96_camera_pose_args {
  uint32_t view_mode;   /* [0x14E57C] (getter FUN_00053D50) */
  uint32_t block;       /* behavior index (native [0x107514 + selector*0x70]) */
  uint32_t selector;    /* camera record selector [camera+4] (0..3 pose feed) */
  int32_t variant_x;    /* mode 3/4 variant predicate x (native *0x109A70) */
  int32_t sub;          /* replay sub index ([0x109A70+8]) */
  int mirror;           /* FUN_0004B818: 1 - side (caller-staged) */
  /* FU-152 §2.8 (P4): the default-arm handler call `(&0x108B80)[selector]`.
   * A caller stages the camera-record subset + behavior block; NULL keeps the
   * unported default (returns 0). The steady/sidetrack/staged horizon arms
   * read the native `block[0]` products; all three handlers use
   * `handler_tick` for FUN_0004E248/D668/DF34/E05C. `handler_live`/
   * `handler_speed` feed the action handler's `FUN_0006400C` arm. */
  fifa96_camera_handler_state *handler_state;
  fifa96_camera_handler_block *handler_block;
  fifa96_camera_handler_out *handler_out;
  /* FUN_000505D0 0x51047: the default-arm handler runs only when the
   * caller's ECX is nonzero. FUN_0004D2D4 passes 1 and FUN_0004CF7C passes
   * 0xA; FUN_0004CEF4 leaves ECX to its caller. Zero here keeps the call;
   * stage 1 for the zero-guard case (the axis/latch still run first). */
  int handler_skip;
  int32_t handler_live;
  int32_t handler_speed;
  /* FUN_00049388()'s `[0x107300]` tick fed to the handler integrators and
   * the action handler's yaw/pitch slew. The native value is the 100 Hz
   * elapsed time x12/5; the match body runs at 30 Hz, so the live caller
   * passes frame_delta * 4. */
  int32_t handler_tick;
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
 * the tracked-player event-rate tail's keeper sink (the FUN_00092998 ring)
 * and the visual smoothing words 0x15780C/0E/10/12/14. The `[0x157815]` dir
 * byte and the 0x10E169/0x11042B table arm are ported next to this setter
 * (FU303/FU294-L2, phase-28 T8): `fifa96_camera_event_dir` and
 * `fifa96_camera_rate_table`. */
int fifa96_camera_event_set(fifa96_camera *cam, int16_t seed_x, int16_t seed_z,
                            int16_t height, int ramp_param);

/* FU303/FU294-L2 (phase-28 T8): FUN_00071C94's actor dir arm
 * (`0x71d70..0x71ddc`), split from the setter because the native EBP record
 * is the caller's. `fifa96_camera_event_set` has already stored the 0x71d6a
 * default 0; this applies the actor arm over the post-ramp state:
 *   - `has_slot == 0` (`[rec+0x20] == 0`): the default 0 stands;
 *   - sub-object dir pair (dir_x, dir_z) non-zero: the 0x10E169 compass row
 *     (the same 3x3 the rate arm reads; out-of-domain pairs resolve to 0xFF,
 *     see the header note on the rate tables);
 *   - (0, 0): when the bearing `word[0x1577BE]` (this struct's `speed`)
 *     is zero the 0x71db1 store of 0xFF; else the 0x8DD70 bearing oct
 *     `((0xCD474(vel_x, vel_z) + 0x40) & 0x3FF) >> 7` over the post-ramp
 *     `word[0x1577C0]/[0x1577C2]` velocity pair.
 * Returns FIFA96_OK, -FIFA96_ERR_INVALID on NULL cam. */
int fifa96_camera_event_dir(fifa96_camera *cam, int has_slot, int32_t dir_x,
                            int32_t dir_z);

/* FU-152 §2.9 (T2) / FU301 §1.6 (phase-28 T6): FUN_0008DC50
 * (`0x8DC50..0x8DC62`) — the signed truncating shift `v >= 0 ? v >> n :
 * -((-v) >> n)` (`SAR` is not what the native does: it takes the magnitude
 * first). The row-`0x1B` wide Block E reduce and the reposition core consume
 * it. `n` is the caller's shift count (no bounds clamp in the native). */
int32_t fifa96_camera_shift_step(int32_t v, int n);

/* FU302 §4 (phase-28 T7): FUN_000795A4 (`0x795A4..0x795B2`) — the 16.16
 * product `(a * b + 0x8000) >> 16`, low word. The event setter's velocity
 * rebuild and the row-0x1B/0x1C steering (`0x76C10`/`0x76C47`/`0x76CBA`/
 * `0x76CEA`) consume it. */
int16_t fifa96_camera_mul16(int32_t a, int32_t b);

/* FU-152 §2.9 (T2) / FU316-L2: FUN_000709D0, the pan step FUN_000736AC
 * calls when `timer > timer_limit` or the camera lands (`prev_y > 0 &&
 * pos_y <= 0`). Bands the event height into the
 * FUN_00065CF8 sink arguments (returned through `class_of`/`param`, left
 * untouched at height 0), advances [0x157821] (cap 100), decays the height by
 * pan_decay/0x20, runs the ([0x14C1D4]|[0x14C1D6]) & 4 random walk when
 * `walk_gate & 4` (the atan direction + `rng_low - 0x80` jitter, velocity
 * magnitude kept; an absent `rng` skips the jitter), then re-runs
 * FUN_00070544(0) over the stored step pair. Returns FIFA96_OK or
 * -FIFA96_ERR_INVALID (NULL cam). Leg (OL-T11-79): the engine frame path
 * passes `walk_gate` 0 because the [0x14C1D4]/[0x14C1D6] producer is
 * unported, so the random walk is only reachable from callers that stage the
 * gate (the tests). */
int fifa96_camera_pan_step(fifa96_camera *cam, struct fifa96_rng *rng,
                           int walk_gate, int32_t *class_of, int32_t *param);

/* FU-152 §2.9 (T2): FUN_00070DE0's derived core — the boundary/reposition
 * handler the armer FUN_0007131C calls at 0x7134F. Classifies the previous
 * (0x157758/5C/60) and current triples; on a nonzero flag overlap it is a
 * no-op (returns 0). Otherwise, when the XOR has bit 0x8 clear, it walks from
 * the previous point toward the current one (FUN_0008DC50(delta, 1) per axis)
 * while the candidate keeps the previous flags (the adopted point is the
 * farthest one whose classification still equals the previous triple's), then
 * applies the mask effects over the changed flags (`prev ^ cur`): bits 3
 * negate-quarter vel_x, bit 4 zeroes vel_z when the previous classify was
 * inside (flags_prev == 0), bit 0x10 jitters y when flags_prev != 0 (the
 * FUN_00070B94 elevation loop is the else leg), and recomputes the bearing. Returns 1 when the walk/reposition ran, 0 for
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

/* M2 phase-10 T2 (FU-147 §3.3 leg 7 / FU-152 §2.9): FUN_00070C08
 * (`0x70C08..0x70DDD`) — the per-record camera event FUN_0007BF20's tail
 * (0x7C8FC) runs for every record. First-hand body: skip when `rec+0x9A != 0`
 * or (`rec+0x91 == 0x22` and ball height < 0x20); two shift-stepped seeds
 * `v1 = shift2(word(event_step_x - (pos_x - target_x)))`, `v2 =
 * shift2(word(event_step_z - (pos_z - target_z)))` (the native 0x1577BA/BC
 * step pair minus the `[0x15774C] - [0x157764]` origin correction; both
 * differences are word stores and `0x8DC50` sign-tests the word, FU306
 * §5.2 item 3); when both are zero, three RNG
 * draws give `t = (draw1 & 0x3F) + 0x30` and the two signs (draw2/draw3);
 * then either the re-seed `event_param := ball_height` with one seed
 * reflected (when `sext(height_5d + 0x68) > ball_height`, 0x70CFA..0x70D03:
 * `e1 = |vel_x|` (the `dword[0x1577BE]>>16` extract = word[0x1577C0],
 * NEG-gated by its own sign, 0x70D43..0x70D61) and `e2 = |vel_z|` (the
 * `dword[0x1577C0]>>16` extract = word[0x1577C2], 0x70D63..0x70D7F);
 * `|vel_x| > |vel_z|` negates `v1`, else `v2` — 0x70D81..0x70D8C; fix round 2
 * errata-on-erratum: round 1's `e1 = ±speed`/`e2 = ±vel_x` mis-read the
 * dword>>16 extracts) or the two-step
 * event-height ramp toward `height_5d + 0x70` (`new = target + ((event -
 * target) >> 1)`, then `new += (new - target) >> 1` while `target < new`,
 * 0x70D05..0x70D41; T2-review erratum, fix round 1 — the pre-fix comment
 * split `rec+0x5D > ball_height -> ramp` and negated `v2` on `e1 > e2`);
 * then the `0x8DC68` distance and
 * `fifa96_camera_event_set(cam, v1, v2, event_param, 1)` (the native
 * FUN_00071C94 with the vector `{distance, v1, v2}` and ECX 1; FU306 erratum
 * 1 — the native `0x70DBA MOV ECX,1` survives the `0x92820` sink and takes
 * the FUN_00070544 ADD sign). `cam` supplies
 * the `[0x1577BA]/[0x1577BC]` step pair (`event_step_x/z`), the target/origin
 * correction (`target_*`), the `[0x1577C0]/[0x1577C2]` velocities, the
 * `[0x1577BE]` bearing (`speed`) and the `[0x1577F0]` event height
 * (`event_param`). The `FUN_00092820(rec, 0x1C)` sink is reported
 * (`out->sink`, OL-27). Returns 1 when the event ran, 0 on a record gate,
 * -FIFA96_ERR_INVALID (NULL cam/in/out or a NULL rng on the jitter path). */
typedef struct fifa96_camera_record_event_in {
  uint8_t skip_9a;      /* rec +0x9A */
  uint8_t action_91;    /* rec +0x91 (the dword +0x8E >> 24 byte) */
  int16_t height_5d;    /* rec +0x5D word */
  int16_t ball_height;  /* [0x157750] word */
  /* FU303/FU294-L2 (phase-28 T8): the record's sub-object dir arm inputs
   * (rec +0x20 pointer -> the slot +0x20/+0x21 dir bytes). */
  uint8_t has_slot;     /* rec +0x20 != 0 */
  int8_t dir_x, dir_z;
} fifa96_camera_record_event_in;

typedef struct fifa96_camera_record_event_out {
  uint8_t applied;      /* the setter applied (0 when the 0x71C99 bail fired) */
  uint8_t sink;         /* FUN_00092820(rec, 0x1C) request (OL-27) */
  uint8_t sink_code;    /* 0x1C */
  int16_t distance;     /* the 0x8DC68 word stored to the vector's first word */
  int16_t vec_1;        /* vector word 1 (the event seed pair) */
  int16_t vec_2;        /* vector word 2 */
} fifa96_camera_record_event_out;

int fifa96_camera_record_event(fifa96_camera *cam, struct fifa96_rng *rng,
                               const fifa96_camera_record_event_in *in,
                               fifa96_camera_record_event_out *out);

/* FU-152 §2.9 (T2) + FU303/FU294-L2 (phase-28 T8): FUN_00071DF4's
 * table/keeper arm. Gates: signed `rate_byte` >= 0, timer <= 0x1E, then when
 * `subobj_present` the rate pair is resolved from the two static tables and
 * written to `rate_x`/`rate_z`:
 *
 *   row  = 0x10E169[dir_x + 4*dir_z]          (both bytes sign-extended)
 *   idx  = (int8_t)(row + (rate_byte << 3))   (the native 8-bit ADD AL,AH)
 *   rate_x = byte[0x11042B + 2*idx]; rate_z = byte[0x11042C + 2*idx]
 *
 * The native first arm (0x71e1c..0x71eef) returns early; this arm is its
 * fall-through. The keeper gate (event_byte == 2, event height >= 0xC1,
 * |anchor_x| <= 0x23F, rate_z != 0) emits 0x1D for the approaching cases and
 * 0x1E otherwise through `keeper_code` (0 = none). The `event_byte` is the
 * FUN_00092998(1,2,-1) ring result (unported, OL-27); the frame path stages
 * 0 so the keeper sink stays a leg while the rate write is live.
 * Returns 1 when the rates were applied or a keeper code emitted, 0 when
 * nothing applied, -FIFA96_ERR_INVALID (NULL cam/keeper_code).
 *
 * Table domain: the live slot writer's dir bytes are T2/T3 of the mapped pad
 * (`+0x20`/`+0x21`, FU70 §1.2) whose rows only produce {-1,0,1} (T1 never
 * yields the dead T3 0x40/0 entries), so the 3x3 compass and the 8x8 rate
 * rows are the exact live tables. The native's unchecked signed index reads
 * neighbouring image data for any other pair; that read is the FU303-L11
 * residual and resolves to "no row" (-1) here. */
int fifa96_camera_rate_table(fifa96_camera *cam, int32_t event_height,
                             int32_t rate_byte, int32_t timer,
                             int32_t subobj_present, int32_t dir_x, int32_t dir_z,
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
 * height gate fails, -FIFA96_ERR_INVALID on NULL. The fall-through table arm
 * (FU303/FU294-L2, phase-28 T8) is `fifa96_camera_rate_table`; the keeper
 * event sink (FUN_00092998 -> FUN_0008F188 0x1D/0x1E) stays a leg (OL-27). */
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

/* FU351-L2d: FUN_0004CF7C's replay-camera transition body. The native
 * FUN_0004F8C8 copies the saved start pose into the camera, computes the
 * six deltas to the target pose, then interpolates with FUN_0004F830 at
 * `param_4*10` in-ticks and `param_5*10` out-ticks. FUN_0004CF7C passes
 * ECX=0xA and 0x1E, so the transition is 100/300 ticks. The target pose is
 * the staged `0x14E470/4/8` + `0x14E4B8/C` + `0x14E4AC` block; the native
 * producers of those globals stay the replay-camera leg. */
#define FIFA96_CAMERA_REPLAY_IN_TICKS 10
#define FIFA96_CAMERA_REPLAY_OUT_TICKS 30

typedef struct fifa96_camera_pose6 {
  int32_t x, y, z;
  int32_t yaw, pitch, ratio;
} fifa96_camera_pose6;

/* FUN_00045069 (`0x45069..0x450B6`): the replay-cancel input latch. While
 * `latch` is clear, a nonzero high nibble of `input` (native `[0x1067F3]`)
 * sets `latch = 5` and is returned (the `cancel_input` for
 * `fifa96_camera_replay_step`); otherwise 0. */
typedef struct fifa96_camera_replay_input {
  uint8_t input;  /* native [0x1067F3] */
  int32_t latch;  /* native [0x1067F4] */
} fifa96_camera_replay_input;

int32_t fifa96_camera_replay_cancel(fifa96_camera_replay_input *input);

typedef struct fifa96_camera_replay_blend {
  fifa96_camera_pose6 start; /* native 0x14E4F8 */
  int32_t dx, dy, dz;        /* native 0x108BD4/D8/DC */
  int32_t dyaw, dpitch;      /* native 0x108BE0/E4, signed 16-bit deltas */
  int32_t dratio;            /* native 0x108BE8 */
  int32_t elapsed;           /* native 0x107DE0 */
  int32_t in_time;           /* native 0x107DE4 */
  int32_t out_time;          /* native 0x107DE8 */
  int32_t active;            /* native [0x107DD8] */
} fifa96_camera_replay_blend;

/* FUN_0004F830: the native integer ease. `elapsed < in_time` is
 * `delta*elapsed^2/(2*in*out)`, `in_time <= elapsed <= out_time` is
 * `delta*(2*elapsed-in)/(2*out)`, and `elapsed > out_time` is
 * `delta - delta*(in+out-elapsed)^2/(2*in*out)`. Products are truncated to
 * 32 bits before the signed divide, as in the native EAX-only chain. A
 * zero denominator returns 0 (the native would fault; no live caller passes
 * one). */
int32_t fifa96_camera_replay_ease(int32_t delta, int32_t in_time,
                                  int32_t out_time, int32_t elapsed);

/* FUN_0004CF60 (`0x4CF60..0x4CF78`): copy the active camera record to
 * `0x14E460`; its +0x10/+0x14/+0x18/+0x58/+0x5C/+0x4C cells are the replay
 * target at `0x14E470/4/8`, `0x14E4B8/C` and `0x14E4AC`. The caller keeps
 * this snapshot across the replay transition. */
void fifa96_camera_replay_snapshot(fifa96_camera_pose6 *snapshot,
                                   const fifa96_camera_pose6 *pose);

/* FUN_0004F8C8's start arm with the FUN_0004CF7C 10/30 in/out constants.
 * `start` may be NULL (the native then snapshots `pose`); `target` is
 * required. Copies the start pose into `pose`, stores the deltas, clears
 * elapsed and sets the active latch. Returns FIFA96_OK. */
int fifa96_camera_replay_begin(fifa96_camera_pose6 *pose,
                               const fifa96_camera_pose6 *start,
                               const fifa96_camera_pose6 *target,
                               fifa96_camera_replay_blend *state);

/* FUN_0004CF7C's head gate: FUN_00036BC8(7) is `[0x105FFC] == 7`; when it
 * matches, the native selects the 0x1078F8 record and does not start the
 * blend. Returns 0 without touching `pose`/`state` on the gate; otherwise
 * the `fifa96_camera_replay_begin` result. */
int fifa96_camera_replay_begin_gated(fifa96_camera_pose6 *pose,
                                     const fifa96_camera_pose6 *start,
                                     const fifa96_camera_pose6 *target,
                                     fifa96_camera_replay_blend *state,
                                     int32_t screen_105ffc);

#define FIFA96_CAMERA_ACTION_SLOTS 23

/* FU351-L2f: FUN_000365B0 (`0x365B0..0x3665A`) — the `[0x105FC4]` target
 * source switch. 0 -> `focus` (`[0x109A70]`), 1 -> `pan` (`0x105FB8`),
 * 2 -> `record` (`FUN_0004B97C`'s `0x14C454` copy), 3 -> table slot 22
 * (`0x109A74 + 0x108`), 4 -> `table[[0x108E04]]` when the index is 0..21.
 * The early arms: `force_focus` (native `FUN_00063FD0` =
 * `[0x109A98] & 0x80`) makes the function return `focus` without looking at
 * the source; otherwise `replay_active` (`FUN_00037AE4`) plus the active
 * record's `+4` class `-1` returns `replay_fallback` (`0x105FE8`). Any other
 * source/index returns `focus`. The sources are staged; this function does
 * not invent the pan or record producers. Returns NULL on a NULL selected
 * pointer. */
/* FUN_0004B308 (`0x4B308..0x4B375`): the phase/difficulty/option preset
 * used by both FUN_0004D04C and FUN_00036208. 3 -> 1/2, 4 -> 3/4,
 * 0x15 -> 5/6/8, else 0. */
int32_t fifa96_camera_view_preset(int32_t phase, int32_t difficulty,
                                  int32_t option_15b680);

/* FUN_00036208 (`0x36208..0x36540`): the `0x105FB8/0x105FC0` pan target
 * step, active only when the staged device is >= 0 and `[0x105FC4] == 1`.
 * `input_bits`/`speed` are FUN_0004511D's low byte/high dword; `camera_z`
 * is the `0x15774C+8` place. The mode comes from `fifa96_camera_view_preset`
 * and the `0x105FC8` offsets are `{0x12C0, 0x2580×7, 0}`. Returns 1 when
 * applied, 0 on the device/source gate, -FIFA96_ERR_INVALID on NULL. */
typedef struct fifa96_camera_pan_target {
  int32_t x, z;   /* native 0x105FB8 / 0x105FC0 */
  int32_t dx, dz; /* native 0x14AC20 / 0x14AC1C */
} fifa96_camera_pan_target;

int fifa96_camera_pan_target_step(fifa96_camera_pan_target *target,
                                  int32_t device, int32_t source_105fc4,
                                  int32_t input_bits, int32_t speed,
                                  int32_t phase, int32_t difficulty,
                                  int32_t option_15b680, int32_t camera_z);

const int32_t *fifa96_camera_target_select(
    int32_t source, int32_t force_focus, int32_t replay_active,
    int32_t record_class, const int32_t *focus, const int32_t *pan,
    const int32_t *record, const int32_t *replay_fallback,
    const int32_t table[FIFA96_CAMERA_ACTION_SLOTS][3], int32_t index);

/* FU351-L2i: FUN_0004BBFC (`0x4BBFC..0x4BC1`) + FUN_0004BCC4
 * (`0x4BCC4..0x4BD35`) — the replay camera's target-player choice. The
 * `0x515A6` replay camera state machine calls FUN_0004BCC4 at `0x5179F`
 * and stores its return at `0x108E04`, the source-4 index. FUN_0004BBFC
 * scans the `0x157B02` goal-side ring for the first event after the last
 * nonzero level when the `0x157AC5/7` score words differ, bumps its player in the
 * `0x15A998/9E` 2x16 activity table by 0x32, then finds that player's
 * `0x14C360` order slot. FUN_0004BCC4 copies the selected order entry to
 * current slot 1 and returns `side*0xB + 1`, with 0 -> 0 and 0x10 -> 0xB
 * special cases. `order_id` is the dereferenced `0x14C360` entry (the
 * player id); `event_id` is the dereferenced `0x157B16` entry. */
#define FIFA96_CAMERA_REPLAY_EVENTS 10
#define FIFA96_CAMERA_REPLAY_SLOTS 16

typedef struct fifa96_camera_replay_target {
  uint16_t score[2];       /* 0x157AC5 / 0x157AC7 (word compare) */
  uint8_t event_count;     /* 0x157AC9 */
  uint8_t event_side[FIFA96_CAMERA_REPLAY_EVENTS]; /* 0x157B02 */
  int32_t event_id[FIFA96_CAMERA_REPLAY_EVENTS];   /* [[0x157B16]] */
  int32_t order_id[2][FIFA96_CAMERA_REPLAY_SLOTS]; /* [[0x14C360 + ...]] */
} fifa96_camera_replay_target;

typedef struct fifa96_camera_replay_activity {
  int32_t id[2][FIFA96_CAMERA_REPLAY_SLOTS];    /* 0x15A998 */
  int32_t count[2][FIFA96_CAMERA_REPLAY_SLOTS]; /* 0x15A99E */
} fifa96_camera_replay_activity;

typedef struct fifa96_camera_replay_choice {
  int32_t side;         /* selected 0x14C360 side (0/1) */
  int32_t slot;         /* selected 0x14C360 slot (0..15) */
  int32_t target_index; /* 0x155AB0 index side*11 + slot */
  int32_t source_index; /* FUN_0004BCC4 return stored at 0x108E04 */
  int32_t copy_slot;    /* 0x157BD2 copy target (1), or -1 for a special */
  int32_t order_value;  /* selected order entry */
} fifa96_camera_replay_choice;

int fifa96_camera_replay_choice_select(
    const fifa96_camera_replay_target *target,
    fifa96_camera_replay_activity *activity,
    fifa96_camera_replay_choice *out);

/* FUN_00036ACC (`0x36ACC..0x36B31`): the camera yaw approach for the
 * action-table target. It looks at `(track_x, track_z)` from
 * `(pos_x, pos_z)`, computes the signed delta to `yaw`, and if
 * `|delta| > (ratio*3)>>2` leaves the cell alone; otherwise it moves yaw by
 * the remaining signed amount. The native reads camera+0x4C/0x58 and the
 * +0x10/+0x18/+0x34/+0x38 triple. */
void fifa96_camera_yaw_approach(fifa96_camera_handler_state *st);

/* FU-351-L2e: FUN_000505D0 state 0xB's tail after FUN_0004F8C8 reports the
 * blend finished. FUN_0004BAD8 resolves the `[0x10F370]` record through
 * FUN_0004BE44; the selected 12-byte slot of the `0x155AB0` table becomes
 * camera+0x34/38/3C, then FUN_00036ACC runs. The row-0x17 producer is
 * non-live (FU-201), so the event inputs and table are staged. Returns 1
 * when applied, 0 for no record/slot, -FIFA96_ERR_INVALID on NULL. */
int fifa96_camera_action_target(fifa96_camera_handler_state *st,
                                const int32_t table[FIFA96_CAMERA_ACTION_SLOTS][3],
                                int32_t event_present, int32_t event_roster,
                                int32_t event_side, int32_t ac2);

/* The state-0xB composition: advance the replay blend; once it reports
 * finished, apply the action-table target. Returns 1 when the target was
 * applied, 0 while the blend is still running, -FIFA96_ERR_INVALID on NULL. */
int fifa96_camera_action_step(fifa96_camera_handler_state *st,
                              const int32_t table[FIFA96_CAMERA_ACTION_SLOTS][3],
                              int32_t event_present, int32_t event_roster,
                              int32_t event_side, int32_t ac2,
                              fifa96_camera_pose6 *pose, int32_t tick,
                              int32_t cancel_input, int32_t view_mode,
                              fifa96_camera_replay_blend *blend);

/* FUN_0004F8C8's per-frame arms. `tick` is FUN_00049388; `cancel_input` is
 * the FUN_00045069 high-nibble result and `view_mode` the FUN_00053D50
 * value. Returns 1 when the transition has finished (or the cancel gate
 * fired), 0 while it is still blending. The yaw/pitch writes mask to the
 * native low 16 bits (the 0x5A/0x5E high-word clears). The native
 * FUN_00037114(1) call is a no-op: FUN_000974D8 is a bare RET. */
int fifa96_camera_replay_step(fifa96_camera_pose6 *pose, int32_t tick,
                              int32_t cancel_input, int32_t view_mode,
                              fifa96_camera_replay_blend *state);
