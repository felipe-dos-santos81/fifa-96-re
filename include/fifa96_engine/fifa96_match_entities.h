/* include/fifa96_engine/fifa96_match_entities.h — M2 Task 8 / FU-141: the
 * derived entity/ball pool and the FU-67 per-frame update chain.
 *
 * Native layout (re-verified read-only in /FIFA96.EXE this slice):
 *   - two team blocks at flat 0x1588A4 (side 0) and 0x1590D9 (side 1), stride
 *     0x835, each holding 11 player records at stride 0xB2 (FU-67 §3,
 *     FUN_0004B100 0x4B14A/0x4B169);
 *   - a record's fields: position +0x59/+0x5D/+0x61, output triple
 *     +0x4D/+0x51/+0x55, velocity pair +0x71/+0x73, lane +0x69, timers
 *     +0x7F/+0x81/+0x89/+0x93, flags +0x8D (active) / +0x8E (type) / +0x8B
 *     (actor type) / +0x8F (stage) / +0x91 (code) / +0x92 (stage latch) /
 *     +0x98, +0x9A (exclusions) / +0x9B (has ball) / +0x9E (ran) / +0x9F
 *     (carrier bit), control-slot pointer +0x20, timer pair +0x79/+0x7B,
 *     animation id `byte[[rec+0x28]]` (row 0 of the selector's row pointer)
 *     and frame +0x3D (OL-80; staged by `FUN_00036C70` at
 *     0x36D44/0x36D4F);
 *   - team fields: side +0x826, slot-pool byte +0x828, search gate +0x829,
 *     mode +0x82A, update counter +0x82C, timer cluster +0x7CB/+0x81E/+0x820,
 *     target pointer +0x7B2 (the `[0x157A83]` controlled actor is a separate
 *     global), secondary +0x7B6, chosen +0x7BF,
 *     interception +0x7BA, flag +0x7BE (FU-67 §3.1/§4.2, FU-75 §4),
 *     team-tail byte +0x830 and chosen-record cache +0x831 (FU-142 §2, the
 *     0x2A arm writes +0x831);
 *   - the ball record at flat 0x15880C (x/y/z, heading, appearance; FU-120 §2)
 *     and the FU-139 §3.1 staging block (0x158730).
 *
 * The update chain follows FUN_0004B100's order (control slots -> camera/track
 * -> clock -> team 0 -> team 1 -> ball pairing) and FUN_0008D8EC's body:
 * update counter, phase-2 nearest target selection (`0x8D929..0x8D9B7`),
 * phase-2 interception selection (`0x8D9BD..0x8DAE9`), team timer decay
 * (`0x8DAF3`), keeper record 0 + outfield records 1..10 (`0x8DB2E`/`0x8DB3A`).
 * FU281 §2's "no engine equivalent of FUN_0008D8EC" is corrected: this walk IS
 * the pre-pass (`fifa96_match_entities_team_update` hosts the `+0x82C` counter
 * through the `0xB2`-stride dispatch), so phase-26c T6 reconciles it by
 * AUGMENT — the pre-pass state rides the FU-141 dispatch and no standalone
 * pre-pass is wired (a second call would double-dispatch every record,
 * FUN_000782D0/FUN_0007CA54 -> `CALL dword[rec+0x18]`).
 * The action bodies run through the caller's callback (the engine binds the
 * FU-137 dispatcher); the record requests are drained by this module:
 * `install` -> the FU-137 §2 installer, `helper_request` -> the FU-138/FU-139
 * slot merge `FUN_0007876C` + `FUN_00078670`, `controlled` -> the pool actor
 * `[0x157A83]`, `place_valid` -> a take-once camera-place request (the FU-71
 * port of `FUN_000700F4`'s first effect). */
#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_ball_pairing.h"
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_keeper_cpu.h"
#include "fifa96_loader/fifa96_scene.h"

#define FIFA96_MATCH_ENTITY_TEAMS 2u
#define FIFA96_MATCH_ENTITY_RECORDS 11u
#define FIFA96_MATCH_ENTITY_RECORD_STRIDE 0xB2u
#define FIFA96_MATCH_ENTITY_TEAM_STRIDE 0x835u
#define FIFA96_MATCH_ENTITY_SELECT_VECTORS 3u

/* Unset entity pointer (the native NULL), used by every selection field. */
#define FIFA96_MATCH_ENTITY_NONE (-1)

/* The `aux_seen` re-arm sentinel: `anim_id` stays < 0x70, so 0xFFFF can
 * never equal a live `(anim_id << 8) | frame` key (row install/successor
 * force the next resolve edge to re-arm). */
#define FIFA96_MATCH_ENTITY_AUX_SEEN_NONE 0xFFFFu

/* One derived native record (stride 0xB2). The native `[rec+0]` team
 * back-pointer is `team`; `index` is the record's position in its team block
 * (record 0 follows the keeper machine, FU-67 §3.2). */
struct fifa96_match_entity {
  uint8_t team;             /* owning team index, native [rec+0] */
  uint8_t index;            /* 0..10, the record's block position */
  uint8_t active;           /* +0x8D */
  uint8_t code;             /* +0x91 current action code */
  uint8_t anim_id;          /* native byte[[rec+0x28]]: the current animation
                             * row id the FU-84 selector stores (`FUN_0006E598`
                             * 0x6E6B1) and `FUN_00036C70` stages at 0x36D44
                             * (OL-80). The pool models the byte value, not the
                             * native row pointer. */
  uint8_t frame;            /* native +0x3D animation frame index, staged by
                             * `FUN_00036C70` at 0x36D4F; the FU-84 per-frame
                             * driver `FUN_0008E008` advances it (OL-80). */
  uint8_t stage;            /* +0x8F >> 24 */
  uint8_t stage92;          /* +0x92 stage latch (reset 0xFF, FU-137 §4.3) */
  uint8_t type;             /* native byte +0x8E: the one face octant (OL-83;
                             * the former `actor_type` alias — `dword[+0x8B]>>
                             * 24` — named the same byte and is retired) */
  uint8_t ran;              /* +0x9E, set by row 00, cleared by the installer */
  uint8_t carrier;          /* +0x9F bit 0 */
  uint8_t has_ball;         /* +0x9B */
  uint8_t skip_98;          /* +0x98 exclusion */
  /* FU-194 §4.1 (M2 phase-17 T5): the native `byte[rec+0x99]` — the second
   * term of the act-8 (phase-0x1E) restart-sweep selection gate
   * (`code [rec+0x91] == 0xC` AND `+0x99 != 0` AND `+0x9A == 0`, the fresh
   * `0x8A81F`/`0x8A828` compares). The engine had no cell for it; init/begin
   * zero it (the native producer is unported, FU194-L2). Read only by the
   * unwired `fifa96_match_action_phase_1E` body. */
  uint8_t skip_99;          /* +0x99 */
  uint8_t skip_9a;          /* +0x9A exclusion (occupied record) */
  uint8_t row43;            /* native +0x43: the anim-row "steer the facing"
                             * latch (row_flags & 0x02, values {0,2}; FU319
                             * §2/§5, phase-33 probe). Written by the row
                             * installs (0x6E6BF..0x6E6C4 / 0x8DFB6..0x8DFBA),
                             * the kickoff narrow install (0x79C13) and the
                             * row-0x22 stage-0 clear (0x8540C); read only by
                             * the locomotion turn (0x8E29C/0x7BF89). Not
                             * derivable from anim_id: it survives the
                             * 0x799BF row-pointer clear. */
  uint8_t row44;            /* native +0x44 (animation/event ack byte; the
                             * keeper stage gates read it; FU-159 §4.1/T3: the
                             * producer is the row install's `row_flags & 1`
                             * (0x6E6CC/0x8DFC1), derived from `anim_id` by
                             * `fifa96_match_run_record_anim_install` and the
                             * frame-wrap successor) */
  uint16_t aux_word;        /* native +0x36: the resolved frame's aux word.
                             * The FUN_0006E8E8 dispatch consumes its two bytes
                             * as two passes; one-shot arms clear their own
                             * byte (FU-161 §5.4). Re-armed on the (anim_id,
                             * frame) resolve edge; `aux_seen =
                             * FIFA96_MATCH_ENTITY_AUX_SEEN_NONE` forces a
                             * re-arm (row install). */
  uint16_t aux_seen;        /* derived: (anim_id << 8) | frame of the last
                             * aux resolve */
  uint8_t frame_freeze;     /* derived: the aux-0x2B frame-timer freeze
                             * (native `word[+0x32] = 0`, 0x6EA03), consumed by
                             * `match_run_scene_stage` */
  uint8_t has_slot;         /* +0x20 control slot bound */
  uint8_t timer93;          /* +0x93 countdown byte */
  uint16_t timer81;         /* +0x81 countdown word */
  uint16_t timer7b;         /* +0x7B */
  uint16_t timer79;         /* +0x79 */
  /* FU-330/FU-331 (phase-35 P1): the placement-limit producer's two native
   * inputs. `desc_d` is `byte[[rec+4]+0xD]` — the per-player roster descriptor
   * (the `0x10F394` tier index; the native read is unguarded, FU330-L2); its
   * producer is unported (FU301-L3/FU330-L1) and the pool stages 0.
   * `byte9d` is the native `byte[+0x9D]` OR term (OL-72); its only writer is
   * the unported lineup rebind and the pool stages 0. With both at 0 the
   * producer yields `0x10F394[0] = 6` on every record — a declared staging
   * value, not a native per-record measurement. */
  uint8_t desc_d;           /* native byte[[rec+4]+0xD] */
  uint8_t byte9d;           /* native byte[+0x9D] */
  uint32_t timer7f;         /* +0x7F, countdown limit = >>16 */
  int32_t timer89;          /* +0x89 action timer */
  int32_t pos_x, pos_y, pos_z;          /* +0x59 / +0x5D / +0x61 */
  /* FU-153 / M2 phase-12 T1 (15.a): the FUN_0007BF20 jump arc state — word
   * +0x85 (the arm-staged ramp time = the arc peak; set by the FUN_0006E8E8
   * jump arms at `0x6ED1F`/`0x6EDFD`/`0x6FB46` to `ramp(word[+0x83])` — the
   * arm's just-stored marker read through the `dword[+0x81] >> 16` alias
   * (0x10/0x18/0x50; in particular NOT the timer81 countdown word at +0x81,
   * `0x7CA5C`), cleared by the landing edge)
   * and word +0x87 (the running time, advanced by the whole-frame delta at the
   * `0x7C072` arc entry and zeroed at the `0x7C0AE` landing). While
   * `+0x85 != 0` (`0x7C05F JZ`) the arc writes `pos_y` (+0x5D) from
   * `fifa96_camera_arc_height(+0x85, +0x87)` (`0x7C072..0x7C101`) and lands
   * (`0x7C096 JLE` arm: pos_y 0, both words 0, target := position) when
   * `+0x87 > 2*(+0x85)`. `+0x83` is written by the arms but read only by the
   * unported FUN_0006E8E8 event gates and the arm's own ramp argument (leg). */
  int16_t jump_base;    /* +0x85 */
  int16_t jump_timer;   /* +0x87 */
  int32_t target_x, target_y, target_z; /* +0x4D / +0x51 / +0x55 */
  int32_t vel_x, vel_z;                 /* +0x71 / +0x73 (16.16 pair) */
  int16_t lane_x, lane_z;               /* +0x6B lane (the HIGH word of the
                                         * native packed `dword[+0x69]`) / +0x6D
                                         * cam-minus-pos x word (the FU-151
                                         * `cam_dx6d`) */
  int16_t bound;                        /* +0x77 old-lane bound word (FU-147 S1:
                                         * `FUN_0007BF20` 0x7C77E stores the old
                                         * `+0x6B` before the lane refresh) */
  int16_t cam_dz6f;                     /* +0x6F camera-minus-position z word
                                         * (the FU-147 S1 track's `cam_dz`) */
  int32_t lane;                         /* +0x69 LOW word (the dz word),
                                         * sign-extended. The native cell is the
                                         * packed `dword[+0x69]`: its high word is
                                         * the `+0x6B` lane (`lane_x` above).
                                         * Readers of the native `>>16` form use
                                         * `lane_x`, never `lane >> 16` (FU311-L5). */
  int8_t dir_x, dir_z;                  /* bound slot direction +0x20/+0x21 (the
                                         * T2/T3 chain `FUN_00078950` writes; row
                                         * 00 reads them through the unaligned
                                         * dwords `[slot+0x1D]`/`[slot+0x1E]`) */
  /* FU-77 `FUN_0007BF20` shared mover state (M2 interactive Task 1): the
   * record fields the per-frame integrator reads/writes and that persist
   * across frames. `face7d` = +0x7D facing, `speed71` = +0x71 speed metric,
   * `vel73`/`vel75` = +0x73/+0x75 velocity words, `body_timer9c` = +0x9C
   * stride accumulator (threshold 2 with a slot). */
  int16_t face7d;
  int16_t speed71;
  int16_t vel73, vel75;
  uint8_t body_timer9c;
  /* Row-28 derived scratch cells (native record +0xA0..+0xAE; FU-142d
   * Appendix G). The native body keeps its stage gates there, so the pool
   * carries them across frames. */
  int32_t scratch_a2;      /* +0xA2 */
  int32_t scratch_a6;      /* +0xA6 */
  int32_t scratch_aa;      /* +0xAA */
  int32_t scratch_ae;      /* +0xAE */
  uint8_t scratch_a0;      /* +0xA0 */
  uint8_t scratch_a1;      /* +0xA1 */
  uint8_t install;       /* derived installer request of the last dispatch */
  uint8_t helper_request;/* derived slot-merge request (keeper claim) */
  uint8_t controlled;    /* derived actor request ([0x157A83] = rec) */
  uint8_t place_valid;   /* derived camera-place request of the last dispatch */
  int32_t place_x, place_y, place_z;
  int8_t place_offset_x; /* caller-supplied 0x10F334[type8] */
  int8_t place_offset_z; /* caller-supplied 0x10F33C[type8] */
  /* FU-191 §6 / FU-192 §3 + FU-199 §7.2 / FU-200 §1 (M2 phase-18 T1): the
   * native record `[rec+0xC]` — the per-record direction-byte pair the phase
   * handler `0x6DCC8` (phase rows `0x02`/`0x15`) indexes and scales (FU-198
   * §5.1). The native pointer is staged at match begin by `FUN_0006D920` (via
   * `FUN_0007412C -> FUN_0008CE78 -> FUN_0006D9C4`) from
   * `FUN_0004C384(form*6+1) + (int8)byte[rec+0x8D] * 0x256`, keyed by the
   * formation id. The engine now stages it live via
   * `fifa96_match_entities_bind_record_meta` (the `match_run_record_meta_bind`
   * caller); an unset pointer (no assets, or an unbound record) still makes the
   * body a documented dispatched-but-gated no-op (FU-177 §7). A fixture may
   * stage a byte pair to exercise the arithmetic directly. `NULL` means
   * "unbound". */
  const int8_t *phase_meta;  /* native [rec+0xC] */
  /* FU-192 §2.3/§2.4 + FU-199 §7.2 / FU-200 §1 (M2 phase-18 T1): the
   * per-record metadata pointers the phase bodies `0x6DD9C` (row 06) and
   * `0x6DE44` (rows 03/04/07) index. The native sources are the OL-13 `.fmt`
   * family (`FUN_0004C384(form*6+2)` stride 8 and `FUN_0004C384(form*6+4)`
   * stride 0x958); the engine stages both live through the same
   * `fifa96_match_entities_bind_record_meta` bind. An unset pointer is the
   * documented gate. `NULL` means "unbound". */
  const int32_t *phase_meta10; /* native [rec+0x10] (row 06, dword pairs) */
  const int32_t *phase_meta14; /* native [rec+0x14] (rows 03/04/07, dword pairs) */
};

struct fifa96_match_team {
  uint8_t side;             /* +0x826 */
  uint8_t slot_pool;        /* +0x828 slot-merge source flag */
  uint8_t search_gate;      /* +0x829 */
  uint8_t mode_82a;         /* +0x82A */
  uint8_t update_count;     /* +0x82C, wraps at 0xB */
  uint16_t timer820;        /* +0x820 countdown (M2 phase-11 T1: the
                             * FUN_0007CD60 long-ball window, 0x3C frames;
                             * FUN_0008D8EC 0x8DAF3 decays it by the frame
                             * delta and clears the arm +0x7CB at 0) */
  uint32_t timer7cb;        /* +0x7CB the long-ball callback record id
                             * (FUN_0007CD60 `[[rec]+0x7B2]`/tracked swap; 0 =
                             * none). The code-7 stage-1 `rec == [team+0x7CB]`
                             * arm selects mode 0x40; FUN_0008C33C/0x8D098
                             * clear it on reset. The native stores a record
                             * pointer (never NULL); the id-0 store would
                             * alias the 0 = none sentinel (unreachable: no
                             * request actor is record 0 — final review). */
  uint8_t flag7be;          /* +0x7BE interception band flag */
  int32_t tracker7c7;       /* +0x7C7 camera-nearest tracker: the team-relative
                             * record index (native record pointer; FU-147 S1
                             * `FUN_0007BF20` 0x7C7C4/0x7C7CD replaces it when
                             * the fresh lane is strictly smaller, and the
                             * FU-151 reset pass `FUN_0008C33C` copies the
                             * `FUN_0008DDE0` pick into it) */
  int32_t target;           /* +0x7B2 encoded entity id or NONE */
  int32_t second;           /* +0x7B6 encoded entity id or NONE */
  int32_t chosen;           /* +0x7BF encoded entity id or NONE */
  int32_t intercept;        /* +0x7BA encoded entity id or NONE */
  struct fifa96_match_entity records[FIFA96_MATCH_ENTITY_RECORDS];
  uint8_t flag830;          /* +0x830 team-tail byte (FU-142 §2) */
  int32_t chosen831;        /* +0x831 encoded entity id or NONE (0x2A arm) */
  /* FU-169 (phase-14 T1): the two per-team contact counters the action-0x0B/
   * 0x0C bodies own. The duel counter `+0x7D7` is the row-04 rare-arm gate
   * (`0x7ED1B`, staged into `fifa96_outfield_row04_state.team_7d7`); the
   * stumble counter `+0x7CF` counts the action-0x0C `>= 0x30` installs (the
   * 1-in-7 commentary event `0xA2` is a leg, FU169-L2). */
  uint32_t duel_7d7;        /* +0x7D7 (0x81B83 increment) */
  uint32_t stumble_7cf;     /* +0x7CF (0x81D6B increment) */
};

/* FU-120 §2 ball record plus the FU-139 §3.1 staging block. The update chain
 * itself writes neither (the derived FU-67 §4.4 pairing writes the two
 * controlled records); the fields are the pool's ball surface. */
struct fifa96_match_ball {
  int32_t x, y, z;      /* 0x15880C / 0x158810 / 0x158814 */
  int16_t heading;      /* 0x15885A */
  uint8_t appearance;   /* 0x158866 */
  int32_t carrier;      /* derived holder entity id or NONE */
  struct fifa96_ball_pair_state pair; /* 0x158730 staging block */
  /* FU-142 OL-63 (M2 phase-9 T2): the derived possession block
   * `0x158728..0x15872F`; `carrier` above is `[0x158724]`. The row-05 claim
   * resets the block and the stage-0 dir arm writes `pos_dir_x`/`pos_dir_z`;
   * `pos_release` is the `0x4B163` per-frame release countdown and
   * `pos_counter_f`/`pos_dir_x` feed the row-06 pursuit gates (FU-139 §11.7
   * OL-69). */
  uint8_t pos_index;     /* 0x158728 */
  uint8_t pos_rotation;  /* 0x158729 */
  int8_t pos_dir_x;      /* 0x15872A */
  int8_t pos_dir_z;      /* 0x15872B */
  uint8_t pos_counter_c; /* 0x15872C */
  uint8_t pos_release;   /* 0x15872D */
  uint8_t pos_counter_e; /* 0x15872E */
  uint8_t pos_counter_f; /* 0x15872F */
  /* M2 phase-11 T2 (L14.8): the FUN_0008BAF0 per-frame ball machine cells
   * (first-hand `disassemble_function 0x8BAF0`, 0x8BAF0..0x8C065). The block
   * base is 0x1587EC; the writes below are the exact native cells:
   *   target 0x158830/34/38 (the lead/commit triple FUN_0008C24C copies into
   *   x/y/z, the phase-lead writers and the per-frame phase-2 arm);
   *   speed 0x158848 (the metric of vel_x/vel_z), vel_x 0x15884A, vel_z
   *   0x15884C (the ±1-ramped velocity words, integrated by the tail);
   *   max_speed 0x15884E (5 default / 7 phase-2 offset arm / 3 reset);
   *   `heading` above is 0x15885A and visual_dir 0x158852 (the 0x1104D2
   *   byte table, read natively as `[EAX+0x1104CF]>>24`); target_heading
   *   0x15885E; ramp_timer
   *   0x158856;
   *   metric 0x15883C + metric_dx 0x15883E + metric_dz 0x158840 (the
   *   FUN_0008DCD4 metric triple); head_gate 0x158881 (the FUN_0006E724
   *   row-flags probe the reset sets from row 3) and head_error 0x158862
   *   (the FUN_0008C068 producer; image default 0). */
  int32_t target_x, target_y, target_z; /* 0x158830 / 0x158834 / 0x158838 */
  int16_t speed;                        /* 0x158848 */
  int16_t vel_x;                        /* 0x15884A */
  int16_t vel_z;                        /* 0x15884C */
  int16_t max_speed;                    /* 0x15884E */
  int16_t target_heading;               /* 0x15885E */
  int16_t ramp_timer;                   /* 0x158856 */
  int16_t metric;                       /* 0x15883C */
  int16_t metric_dx;                    /* 0x15883E */
  int16_t metric_dz;                    /* 0x158840 */
  uint8_t head_gate;                    /* 0x158881 */
  uint16_t head_error;                  /* 0x158862 */
  int8_t visual_dir;                    /* 0x158852 */
  uint8_t appear_row;                   /* the FUN_0006E724 row id (0x158866
                                         * pointer's row; reset installs 3) */
  uint8_t machine_ran;                  /* derived run observability */
};

struct fifa96_match_entities {
  struct fifa96_match_team team[FIFA96_MATCH_ENTITY_TEAMS];
  struct fifa96_match_ball ball;
  uint8_t phase;       /* last phase fed ([0x157A4A]>>24) */
  uint16_t delta;      /* last frame delta fed ([0x157A64]) */
  int32_t controlled;  /* [0x157A83] encoded entity id or NONE */
  uint8_t place_pending;         /* take-once camera-place request */
  int32_t place_x, place_y, place_z;
  int32_t slot_merge;  /* last successful merge requester id or NONE */
};

/* The camera/track inputs the phase-2 selection reads: the three vectors
 * 0x157788/0x157794/0x157770, the bucket timer words 0x1577FA against
 * 0x157800/0x157806, the lead dwords 0x1577BE/0x1577C0 and the camera z word
 * 0x157754. The engine passes the live FU-71 camera triple, `timer_a` =
 * `camera.timer` (word 0x1577FA) and the two lead dwords built from
 * `camera.speed`/`vel_x`/`vel_z`; only the sibling bucket timers
 * 0x157800/0x157806 and their FU-67 S3 producer remain unported/zero. */
struct fifa96_match_entities_frame {
  uint8_t phase;
  uint16_t delta;
  int32_t select_vector[FIFA96_MATCH_ENTITY_SELECT_VECTORS][3];
  int32_t lead_x, lead_z;  /* raw dwords 0x1577BE/0x1577C0 */
  int16_t timer_a;         /* word 0x1577FA = camera.timer (LIVE) */
  int16_t timer_b;         /* word 0x157800 (FU-67 S3, zero) */
  int16_t timer_c;         /* word 0x157806 (FU-67 S3, zero) */
  int16_t cam_z;           /* word 0x157754 */
  /* FU290 (phase-27 T5): the stage-select machine's live inputs the seam maps
   * onto the keeper-CPU record. The camera triple is the native
   * `0x15774C/0x157750/0x157754`; `follow_speed` the `0x1577BC >> 16` word
   * (word 0x1577BE); `pan_counter`/`global_157a4c` the bytes `0x157821`/
   * `0x157A4C`. The unported latch cells stay zero (legs). */
  int32_t cam_pos_x, cam_pos_y, cam_pos_z;
  int16_t follow_speed;
  uint8_t pan_counter;
  uint8_t global_157a4c;
  /* FU303 (phase-28 T8) / FU292-L2: the ramp-band cells the ladder reads.
   * `band_fa` = word[0x1577FA] (the camera timer) and
   * `band_f6` = word[0x1577F6] = timer_limit - ramp_divisor (the 0x705e8
   * `F8 = F4 - F6` identity). F6 is derivable, not a separately produced
   * cell (FU303-L7). */
  int16_t band_f6;
  int16_t band_fa;
  /* FU290 (phase-27 T5): the FU292 latch/geometry cells `[0x157C44]`/
   * `[0x157C48/4C/50]`. Their writer IS recovered as `fifa96_keeper_hold_predict`
   * (FU-179 §4) with the engine cells `mr->predict_c44`/`predict_c48_x/y/z`
   * (the projection sub-leg FU179-L2/L3 is unported, so both read zero live). */
  int32_t latch_c44;
  int32_t target_c48[3];
  int16_t intercept_x[FIFA96_MATCH_ENTITY_TEAMS]; /* word 0x10F37C+side*0xC */
  int16_t intercept_y[FIFA96_MATCH_ENTITY_TEAMS]; /* word +8 of the same record */
  /* FU296 Seam A (phase-27 T3): the record-0 machine `FUN_000782D0` view the
   * `i == 0` branch of the FU-141 walk steps before the single `action()`
   * dispatch. The caller (the match run) owns its storage and binds `side_xor`
   * per frame; NULL leaves the walk inert. */
  fifa96_keeper_cpu *keeper_cpu;
  /* FU299 (phase-28 T2, Option B): the run's process cells the per-step hug
   * binds/publishes around the record-0 step — the `[0x157C5D]` ladder-gate
   * cell, the `[0x157C5C]` latch and the `[0x157AB2]` force-install latch. The
   * caller (the match run) points them at `mr->global_157c5d`/
   * `mr->global_157c5c`/`mr->keeper_latch_157ab2`; the three travel together and
   * NULL (the fixture default) leaves the machine view caller-owned. */
  uint8_t *interlock_157c5d;
  uint8_t *latch_157c5c;
  uint8_t *latch_157ab2;
  /* FU300 (phase-28 T4, G2): the in-frame publish of the keeper ladder's
   * `[0x157C2C]` stamp (`ladder_out.stamp_157c2c`, the native 0x2C/0x2D/0x2E
   * writes before `CALL 0x7D9A4`). The caller points it at
   * `mr->global_157c2c`; NULL leaves the cell caller-owned. */
  uint8_t *stamp_157c2c;
};

/* A ported action-row body over one pool record. Returns FIFA96_OK on success;
 * -FIFA96_ERR_UNSUPPORTED names an unported row and is tolerated by the chain
 * (the record is skipped); any other negative value aborts the update. */
typedef int (*fifa96_match_entity_action_fn)(void *ctx, struct fifa96_match_entity *entity);

/* Zero-seed the pool: sides 0/1, every selection pointer unset, the native
 * reset seeds `[rec+0x92]=0xFF` and the reset-installed action code 0
 * (FUN_0007DAB4 0x7DABA/0x7DAFB), the ball/actor unset. NULL ->
 * -FIFA96_ERR_INVALID. */
int fifa96_match_entities_init(struct fifa96_match_entities *pool);

/* Clear the pool to all-zero so a fresh init is required before reuse. NULL ->
 * -FIFA96_ERR_INVALID. */
int fifa96_match_entities_release(struct fifa96_match_entities *pool);

/* One team's FU-67 chain step (counter, selection, interception, timer,
 * keeper record 0, outfield records 1..10 with the action callback and the
 * per-record request drains). Exposed for order tests; normally reached
 * through fifa96_match_entities_update. The phase/delta are latched from
 * `frame` here too, so a direct call is safe (the frame inputs are the same
 * ones fifa96_match_entities_update gates the ball pairing on). */
int fifa96_match_entities_team_update(struct fifa96_match_entities *pool, uint32_t team,
                                      const struct fifa96_match_entities_frame *frame,
                                      fifa96_match_entity_action_fn action, void *ctx);

/* The full FU-0004B100-derived chain: team 0, team 1, then the FU-67 §4.4
 * ball/possession pairing. Returns FIFA96_OK, -FIFA96_ERR_INVALID (NULL pool/
 * frame) or the action callback's propagated error (other than UNSUPPORTED). */
int fifa96_match_entities_update(struct fifa96_match_entities *pool,
                                 const struct fifa96_match_entities_frame *frame,
                                 fifa96_match_entity_action_fn action, void *ctx);

/* Phase-2 nearest selection over one team's 11 records with skip 0 and the
 * +0x98/+0x9A exclusions (FUN_0008D8EC 0x8D9AA -> FUN_0008DE8C). Returns the
 * chosen record index, NONE when none, or -FIFA96_ERR_INVALID. */
int fifa96_match_entities_team_select(struct fifa96_match_entities *pool, uint32_t team,
                                      int16_t target_x, int16_t target_y);

/* FU-151 §Port contract item 6 / §2.8: `FUN_0008DDE0` (`0x8DDE0..0x8DE26`,
 * first-hand this slice) — the ranked lane pick. Walks the team's 11 records
 * (stride 0xB2) skipping the sign-extended 16-bit `skip` index, `+0x9A` and
 * `+0x98`, keeping the **unsigned** smallest `word[+0x6B]` (lane) with the
 * first record winning ties (`JNC` on `>=`). Returns the picked index, NONE
 * when every record is excluded, or -FIFA96_ERR_INVALID. The reset pass
 * (`FUN_0008C33C`) calls it with skip -1 (no skip); row 04 calls it with
 * skip 0 (record 0 excluded). */
int fifa96_match_entities_team_pick(struct fifa96_match_entities *pool, uint32_t team,
                                    int32_t skip);

/* FU-151 §Port contract item 6 / §2.8/§3.4: the `FUN_0008C33C` +
 * `FUN_0007997C` team reset-lane pass (`0x8C33C..0x8C38B`, first-hand this
 * slice). Per team record: the derived `FUN_0007997C` reset — zero
 * timer89/timer81/timer93/timer7b-velocity words, code/has_ball/skip_98,
 * stage92 0, the `0x795B4` lane refresh against the camera focus
 * (`lane_x`=+0x6B, `lane_z`=+0x6D, `cam_dz6f`=+0x6F), then `bound` (+0x77) :=
 * the fresh lane and the forced `active ? 3 : 0x19` install through the
 * derived installer (the native's virtual `[rec+0x1C]` target restore and the
 * 0x79B6C commit are unported; the derived subset keeps the live position,
 * FU-151 §3.4 leg). Then the team tail: `+0x7CB = 0`, the `FUN_0008DDE0`
 * pick with skip -1, `+0x7B6 = 0` (NONE), `+0x7B2 = pick`, `+0x7C7 = pick`
 * (`target`/`tracker7c7`), and the FU295 §2.2 controlled-side actor lift
 * (`0x8C3BE`, `fifa96_match_entities_actor_lift`). Returns FIFA96_OK or
 * -FIFA96_ERR_INVALID.
 *
 * FU295 (phase-27 T6): `controlled_side` is the native `[0x157AAC]>>0x18` (the
 * process controlled-side byte; the engine's `mr->phase_machine.side_controlled`).
 * The native `0x741C4` driver / `FUN_0007417C` caller is unported (FU295-L1), so
 * this pass has no live engine caller — the lift is a LIVE-LEG today. */
int fifa96_match_entities_reset_lane(struct fifa96_match_entities *pool, uint32_t team,
                                     int32_t cam_x, int32_t cam_z, uint8_t controlled_side);

/* FU295 §2.2/§2.3 (phase-27 T6): the `FUN_0008C33C` controlled-side actor lift
 * (`0x8C3BE`, fresh `decompile_function 0x8C33C`): `if (([0x157AAC]>>0x18) ==
 * byte[team+0x826]) [0x157A83] = [team+0x7B2];` — the team's picked target
 * (`team->target`) is lifted into the actor cell (`pool->controlled`) only when
 * `team->side == controlled_side`. The remaining `FUN_0008C33C` tail writes
 * (`+0x7C3 = target`, `+0x7BF = 0`, the `+0x82x`/`+0x7E7` clears,
 * `FUN_0008C418`) stay unported legs. The native callers (the `0x741C4` driver
 * and `FUN_0007417C`) are unported (FU295-L1), so this is not wired live.
 * NULL pool / team >= 2 -> -FIFA96_ERR_INVALID. */
int fifa96_match_entities_actor_lift(struct fifa96_match_entities *pool, uint32_t team,
                                     uint8_t controlled_side);

/* FU-151 §2.8 / §Port contract item 8: the `0x10F37C` interception target
 * table (first-hand `read_memory`: two 12-byte triples `(0,0,0x990)` side 0 /
 * `(0,0,-0xA90)` side 1). `match_run_entity_frame` stages x/word0 into
 * `frame->intercept_x[]` and z/word8 into `frame->intercept_y[]`. */
#define FIFA96_MATCH_ENTITY_INTERCEPT_Z0 0x990
#define FIFA96_MATCH_ENTITY_INTERCEPT_Z1 (-0xA90)

/* FU-330 §3.1 / FU-331 §5 (phase-35 P1): the native placement-limit value
 * function (`FUN_0007997C` `0x79A29..0x79A4D`): `word[+0x79] =
 * byte[0x10F394 + desc_d] | byte[+0x9D]`. The table is the port's existing
 * `fifa96_arm_26_timer_table[32]` (`fifa96_arm_bodies.c`, flat `0x10F394`;
 * FU330 §4 re-read it byte for byte). Returns 1 and stores `*out` for an index
 * inside the modelled 32-byte tier (`desc_d <= 0x1F`; the table's rows are
 * 6..15). The native `+0x9D` OR term's real domain is 0/0x0F, so a native
 * in-range result is 6..15; the unit pin's `0x10`/`0x80` OR inputs are
 * synthetic discriminators deliberately outside that domain (the port models
 * the expression, not the flag's producer — OL-72). Returns 0
 * for the unmodelled `desc_d >= 0x20` continuation class (the native index is
 * unguarded and would read `0x10F3B4`/the `0x0FF4xx` wrap, FU330-L2) with
 * `*out` untouched. NULL `out` -> 0. */
int fifa96_match_entities_placement_limit(uint8_t desc_d, uint8_t byte9d,
                                          uint16_t *out);

/* FU-331 §5.3 (phase-35 P1): the native placement-reset store pair —
 * `0x79A4D` `word[+0x79] = limit` then `0x79A86` `word[+0x7B] = word[+0x79]`
 * — over one pool record. Both cells are written in that order, so no window
 * exists in which the counter holds a stale limit. Returns 1 when written and
 * 0 when the descriptor is outside the modelled tier table: the cells are
 * then left UNCHANGED (never zeroed: 0 is below the native floor `2..15` and
 * would re-create the FU323-L1 silent bind; FU331 review erratum 2). NULL ->
 * 0. */
int fifa96_match_entities_placement_seed(struct fifa96_match_entity *entity);

/* The FU-137 §2 installer FUN_0007D9A4 over a derived record: `code` is the
 * requested action, `staged` the BL stage byte. Rejects an occupied record
 * (+0x9A) and the same code; clears +0x98 in the unported animation arm; coerces
 * an inactive code 3 to 0x19; sets/clears the +0x9F carrier bit for codes
 * 5/0x21; stages the code, zeroes +0x89, clears +0x9E, writes +0x92 and copies
 * +0x79 -> +0x7B. Returns 1 when a code was staged, 0 when rejected. */
int fifa96_match_entities_install(struct fifa96_match_entity *entity, uint8_t phase,
                                  uint8_t code, uint8_t staged);

/* The derived match-setup slot bind `FUN_000785E0`/`FUN_0008DB6C` (M2
 * interactive Task 1 / FU-70 §1.3): bind the human control slot to a free
 * record of `team` — the engine's one-slot subset of the native
 * `FUN_00078824` four-slot loop. The native pick walks the team's 11 records
 * skipping `+0x20 != 0` holders, the search gate `[team+0x829]`-gated record 0
 * (`0x8DBA6`), `[team+0x7BF]` (the chosen pointer; NONE in the pool) and the
 * `+0x98`/`+0x9A` exclusions, and returns the record nearest the point
 * `0x5774C` (the camera reset triple). The derived pick substitutes
 * `fifa96_entity_find_nearest` (the same 0x8DC68 metric) over the records'
 * positions with `skip_index = team->search_gate == 0 ? 0 : -1`; the native
 * `FUN_000A1860` sort/tie order stays the carried leg. On success the record's
 * `has_slot` is set, `team->slot_pool` increments (the native `team+0x828`
 * bind ordinal) and the record's entity id is returned; NONE when no record is
 * eligible. NULL pool, team >= 2 -> -FIFA96_ERR_INVALID. */
int fifa96_match_entities_bind_slot(struct fifa96_match_entities *pool, uint32_t team,
                                    int16_t from_x, int16_t from_z);

/* The FU-138/FU-139 slot merge FUN_0007876C + FUN_00078670 over the pool:
 * requires the requester to have no slot and the team's +0x828 byte; ranks the
 * team's slot-holding records (the first slot-holder unconditionally, later
 * holders only by a strictly greater signed-word distance `FUN_0008DC68`; the
 * native counter at 0x787E4 counts slot-holders only); on success the
 * requester gains the slot, the donor loses it and the requester is recorded
 * for fifa96_match_entities_take_slot_merge. Returns 1 merged, 0 not,
 * -INVALID. */
int fifa96_match_entities_merge_slot(struct fifa96_match_entities *pool, uint32_t team,
                                     uint32_t record);

/* Consume the last successful slot merge: returns the requesting entity id and
 * clears the pending field, or NONE. */
int32_t fifa96_match_entities_take_slot_merge(struct fifa96_match_entities *pool);

/* Consume the take-once camera-place request: on 1 the triple is written and
 * the request cleared. NULL pointers -> -FIFA96_ERR_INVALID. */
int fifa96_match_entities_take_place(struct fifa96_match_entities *pool, int32_t *x,
                                     int32_t *y, int32_t *z);

/* The derived kickoff ball spawn x (native `[0x158830]`, the value the kickoff
 * act-1 body `0x8ABBA` stores; `FUN_0008C24C` at `FUN_00073E08` copies the
 * `0x158830/34/38` triple into the ball record 0x15880C/10/14). First-hand
 * /FIFA96.EXE `disassemble_function 0x8A938` / `0x73E08` / `0x8C24C`. */
#define FIFA96_MATCH_ENTITY_KICKOFF_BALL_X 0x1E0

/* The native setup/restart record placement `FUN_00079B6C`
 * (`0x79B6C..0x79C1C`, single RET at `0x79C1C`; first-hand
 * get_function_by_address 0x79BB5 -> body_end 0x79C1C): position
 * +0x59/+0x5D/+0x61 := the target triple +0x4D/+0x51/+0x55, position.y := 0,
 * the target triple := the committed position, then the word +0x69 (dz), word
 * +0x71/+0x73 (the velocity x pair) and +0x75 (velocity z high word), word
 * +0x67 (dx), word +0x65 (distance) and byte +0x9C := 0. The pool carries
 * `lane` (dword +0x69), the three velocity word views
 * (`speed71`/`vel73`/`vel75` = +0x71/+0x73/+0x75) and the dword views
 * `vel_x`/`vel_z` (+0x71/+0x73), so `lane`'s low word only is zeroed (native
 * word +0x69) while the three velocity words and both recomposed dwords are
 * zeroed (M2 interactive T1 alias sync: no stale word survives a commit); the
 * +0x65/+0x67 words have no pool field (the FU-141 dispatch staging
 * recomputes `distance`). NULL -> -FIFA96_ERR_INVALID. */
int fifa96_match_entities_place(struct fifa96_match_entity *entity);

/* FU-148 §3 (S4): the formation-id consumer contract. `FUN_0006D9C4` derives
 * `team+0x7AE = &0x11033A + id*0x1D` and walks 4 blocks of 7 bytes
 * {role, count, slots[5]} writing each slot record's +0x90 role. The table is
 * the static 0x11033A image data (5 rows x 0x1D, first-hand read): id 0 =
 * blocks {1,3,5,2} (the "352ko" family), id 1 = {1,4,4,2} ("442ko"). The
 * placement family name comes from the 0x14BFC0 `6*id` slot
 * (`FUN_0004A6BC` builds "%s.fmt" over the 0x107370 loader name table): id 0
 * "352ko.fmt", 1 "442ko.fmt", 2 "swko.fmt", 3 "424ko.fmt", 4 "433ko.fmt". */
typedef struct fifa96_match_formation_block {
  uint8_t role;
  uint8_t count;
  uint8_t slots[5];
} fifa96_match_formation_block;

/* Parse the pinned 0x11033A row for `id` into 4 blocks. Returns 4,
 * -FIFA96_ERR_INVALID (NULL out or id > 4). */
int fifa96_match_formation_layout(uint8_t id, fifa96_match_formation_block out[4]);

/* The derived 0x14BFC0 `6*id` placement name (a static string literal) or NULL
 * for id > 4. */
const char *fifa96_match_formation_fmt_name(uint8_t id);

/* FU-89 §11 / OL-T11-8 (M2 visible-match Task 1): seed each of the two
 * teams' 11 records' *target* triples from the resource-loaded formation —
 * the `FUN_0006E1D0` phase-cell placement (`fifa96_scene_formation_place`)
 * over the pool's records, using each team's side and `controlled_side`
 * (`[0x157AAC]>>24`). The subsequent `fifa96_match_entities_kickoff_place`
 * commit (`FUN_00079B6C` position := target) then gives the records their
 * non-zero kickoff positions. A not-loaded formation returns
 * FIFA96_ERR_NOT_FOUND with the targets untouched; NULL -> -FIFA96_ERR_INVALID. */
int fifa96_match_entities_seed_formation(struct fifa96_match_entities *pool,
                                         const fifa96_scene_formation *formation,
                                         uint8_t controlled_side);

/* FU-199 §7.2 / FU-200 §1 (M2 phase-18 T1): the per-record OL-13 metadata bind
 * — the engine mirror of the native `FUN_0006D920` (entered at match begin by
 * `FUN_0007412C -> FUN_0008CE78 -> FUN_0006D9C4 -> FUN_0006D920`). For every
 * record of `team` it stages the three resource pointers keyed by the record's
 * active index `idx = (int8_t)byte[rec+0x8D]` (the pool's `active`):
 *   `[rec+0xC]`  = `pl + idx*0x256`  (the `form*6+1` `.fmt` blob, 598 B/record)
 *   `[rec+0x10]` = `pk + idx*2`      (the `form*6+2` blob, 2 dwords/record,
 *                                    native stride 8 bytes)
 *   `[rec+0x14]` = `fk + idx*0x256`  (the `form*6+4` freekick blob, 2392 B
 *                                    = 0x958/4 dwords, per record)
 * A NULL blob leaves that pointer NULL (the native body's structural gate).
 * `pl` is a byte pair (`const int8_t *`); `pk`/`fk` are dword-pair blocks, so
 * their typed strides are 2 and 0x256 dwords. NULL `pool` or `team >= 2` ->
 * -FIFA96_ERR_INVALID; otherwise FIFA96_OK. */
int fifa96_match_entities_bind_record_meta(struct fifa96_match_entities *pool,
                                           uint32_t team, const int8_t *pl,
                                           const int32_t *pk, const int32_t *fk);

/* The derived kickoff placement pass, reproducing `FUN_00088DC8` stage 0 ->
 * `FUN_00073E08`:
 *  - the kickoff act-1 body (`FUN_0008A938` jump-table entry 1 ->
 *    `0x8ABAB..0x8ABDA`) stores the ball spawn x = 0x1E0, z = 0 and
 *    `[0x157AB1] = 0` (the last is a process global with no derived home; it
 *    stays unmodeled), then `FUN_0008C24C` copies the `0x158830/34/38` triple
 *    into the ball record and writes ball.y = 0 (`0x8C299`);
 *  - the `FUN_0008CF60` per-record loop runs `fifa96_match_entities_place`
 *    and then the `FUN_00079B6C` tail: the `FUN_00079C50` camera-vs-target
 *    face (`0x79BB5..0x79BCF`, camera triple passed in; the native kickoff
 *    camera is the `[0x10F328/2C/30]` reset triple = (0,0,0)), followed by the
 *    unconditional `FUN_0006E598(rec, active ? 0 : 0x26, 0)` selector
 *    (`0x79BF1..0x79C13`): an inactive record takes row id 0x26, an active
 *    record re-resolves id 0 (the derived `fifa96_arm_anim_select` keep/fallback
 *    rule), and the frame resolver resets `frame` (`+0x3D`) to 0.
 *  - The tail's conditional `FUN_0006E48C` write (`0x79BD4..0x79BEC`:
 *    `byte[rec+0x3E] = byte[rec+0x8E]` when the row pointer and the row's
 *    `+0x44` bit 0 are set) targets a field the pool does not model and stays
 *    a numbered leg.
 * The per-record target source is the resource-loaded `0x14BFC0` table
 * (`FUN_0004A6BC`); `fifa96_match_entities_seed_formation` seeds the targets
 * from the derived formation file first (OL-T11-8 landed), so the pass places
 * the ball spawn and commits/faces the formation targets. NULL `pool` ->
 * -FIFA96_ERR_INVALID. */
int fifa96_match_entities_kickoff_place(struct fifa96_match_entities *pool, int32_t cam_x,
                                        int32_t cam_y, int32_t cam_z);

/* FU-96 leg 5 (M2 interactive T2): the per-record camera place
 * `FUN_00079F3C` (`0x79F3C..0x7A027`, 74 instructions; first-hand
 * `disassemble_function 0x79F3C` on `/FIFA96.EXE`). `FUN_0008CF60` calls it
 * per record between the phase handler and the `FUN_00079B6C` commit, with the
 * camera triple the function reads itself at `0x15774C`/`0x157750`/`0x157754`
 * (the FU-71 camera; the caller's `EBX=0x15774C` is dead). Derived semantics:
 *  - gate 1 (`0x79F45..0x79F59`): the record's team side (`byte[[rec]+0x826]`)
 *    must differ from the controlled side (`[0x157AAC]>>24`), so only the
 *    non-controlled team's records are placed;
 *  - gate 2 (`0x79F5F..0x79F6E`): `byte[0x1106C3 + phase] != 0` (first-hand
 *    table at `0x1106C3`, 29 bytes `00 01 00 00 01 00 01 01 01 00 ... 03 01
 *    78`; the function reads the live phase `[0x157A4A]>>24`);
 *  - metric (`0x79F74..0x79F8F`): `FUN_0008DCD4(camera, &rec+0x4D)` gives the
 *    `dx`/`dz` target-minus-camera words and the octagonal fast length; a
 *    length `> 0x180` returns (records beyond the near ring are untouched);
 *  - place (`0x79F95..0x7A01D`): `angle = FUN_0008DD70(dx,dz)` =
 *    `FUN_000CD474`; `target.x := dword[0x15774C] + (int16)round16(0x180 *
 *    sine(angle))` and `target.z := dword[0x157754] + (int16)round16(0x180 *
 *    sine(angle + 0x100))` (`FUN_000795A4` = the `(a*b + 0x8000) >> 16`
 *    multiply), i.e. the record target snaps onto the 0x180-radius ring around
 *    the camera along its existing direction.
 * The engine primitives `fifa96_arm_dist_stage` (`FUN_0008DCD4`),
 * `fifa96_entity_angle` (`FUN_000CD474`) and `fifa96_entity_sine` (the
 * `0x114E04` fold) are the exact ports, so this pass is the derived native
 * sequence. The native interleaves the place per record before each commit;
 * the commit leaves every target untouched (position := target, then target :=
 * position), so the whole-pool pass before `fifa96_match_entities_kickoff_place`
 * is observationally identical. Phases outside the 29-byte table read the
 * adjacent action-pointer table in the native and stay a numbered leg (this
 * pass only runs on the kickoff path, phase 1). NULL `pool` ->
 * -FIFA96_ERR_INVALID. */
int fifa96_match_entities_camera_place(struct fifa96_match_entities *pool,
                                       uint8_t controlled_side, uint8_t phase,
                                       int32_t cam_x, int32_t cam_z);

/* The FU-67 §4.4 / FUN_0007D430 pairing at the chain tail: phase 2 only, both
 * team targets present, the FU-139-tested fifa96_ball_pair_decide writes the
 * team-0 target's output triple on the found condition. Returns FIFA96_OK or a
 * propagated negative error. */
int fifa96_match_entities_ball_pair(struct fifa96_match_entities *pool);
