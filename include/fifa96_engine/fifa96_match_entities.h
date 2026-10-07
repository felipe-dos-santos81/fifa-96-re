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
 *     (carrier bit), control-slot pointer +0x20, timer pair +0x79/+0x7B
 *     (FU-67 §3.2, FU-74 §2, FU-137 §2, FU-138/FU-140 row maps);
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

#define FIFA96_MATCH_ENTITY_TEAMS 2u
#define FIFA96_MATCH_ENTITY_RECORDS 11u
#define FIFA96_MATCH_ENTITY_RECORD_STRIDE 0xB2u
#define FIFA96_MATCH_ENTITY_TEAM_STRIDE 0x835u
#define FIFA96_MATCH_ENTITY_SELECT_VECTORS 3u

/* Unset entity pointer (the native NULL), used by every selection field. */
#define FIFA96_MATCH_ENTITY_NONE (-1)

/* One derived native record (stride 0xB2). The native `[rec+0]` team
 * back-pointer is `team`; `index` is the record's position in its team block
 * (record 0 follows the keeper machine, FU-67 §3.2). */
struct fifa96_match_entity {
  uint8_t team;             /* owning team index, native [rec+0] */
  uint8_t index;            /* 0..10, the record's block position */
  uint8_t active;           /* +0x8D */
  uint8_t code;             /* +0x91 current action code */
  uint8_t stage;            /* +0x8F >> 24 */
  uint8_t stage92;          /* +0x92 stage latch (reset 0xFF, FU-137 §4.3) */
  uint8_t type;             /* +0x8E >> 24 */
  uint8_t actor_type;       /* +0x8B >> 24 */
  uint8_t ran;              /* +0x9E, set by row 00, cleared by the installer */
  uint8_t carrier;          /* +0x9F bit 0 */
  uint8_t has_ball;         /* +0x9B */
  uint8_t skip_98;          /* +0x98 exclusion */
  uint8_t skip_9a;          /* +0x9A exclusion (occupied record) */
  uint8_t has_slot;         /* +0x20 control slot bound */
  uint8_t timer93;          /* +0x93 countdown byte */
  uint16_t timer81;         /* +0x81 countdown word */
  uint16_t timer7b;         /* +0x7B */
  uint16_t timer79;         /* +0x79 */
  uint32_t timer7f;         /* +0x7F, countdown limit = >>16 */
  int32_t timer89;          /* +0x89 action timer */
  int32_t pos_x, pos_y, pos_z;          /* +0x59 / +0x5D / +0x61 */
  int32_t target_x, target_y, target_z; /* +0x4D / +0x51 / +0x55 */
  int32_t vel_x, vel_z;                 /* +0x71 / +0x73 (16.16 pair) */
  int16_t lane_x, lane_z;               /* +0x6B / +0x6D lane words */
  int32_t lane;                         /* +0x69, lane = >>16 */
  int8_t dir_x, dir_z;                  /* bound slot direction +0x20/+0x21 */
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
};

struct fifa96_match_team {
  uint8_t side;             /* +0x826 */
  uint8_t slot_pool;        /* +0x828 slot-merge source flag */
  uint8_t search_gate;      /* +0x829 */
  uint8_t mode_82a;         /* +0x82A */
  uint8_t update_count;     /* +0x82C, wraps at 0xB */
  uint16_t timer820;        /* +0x820 countdown */
  uint32_t timer7cb;        /* +0x7CB enable */
  uint32_t timer81e;        /* +0x81E limit = >>16 */
  uint8_t flag7be;          /* +0x7BE interception band flag */
  int32_t target;           /* +0x7B2 encoded entity id or NONE */
  int32_t second;           /* +0x7B6 encoded entity id or NONE */
  int32_t chosen;           /* +0x7BF encoded entity id or NONE */
  int32_t intercept;        /* +0x7BA encoded entity id or NONE */
  struct fifa96_match_entity records[FIFA96_MATCH_ENTITY_RECORDS];
  uint8_t flag830;          /* +0x830 team-tail byte (FU-142 §2) */
  int32_t chosen831;        /* +0x831 encoded entity id or NONE (0x2A arm) */
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
 * 0x157754. The producing camera/track block is FU-67 S3 (open leg), so callers
 * supply them; the engine passes its FU-71 camera triple for all three vectors
 * and zero timers/leads. */
struct fifa96_match_entities_frame {
  uint8_t phase;
  uint16_t delta;
  int32_t select_vector[FIFA96_MATCH_ENTITY_SELECT_VECTORS][3];
  int32_t lead_x, lead_z;  /* raw dwords 0x1577BE/0x1577C0 */
  int16_t timer_a;         /* word 0x1577FA */
  int16_t timer_b;         /* word 0x157800 */
  int16_t timer_c;         /* word 0x157806 */
  int16_t cam_z;           /* word 0x157754 */
  int16_t intercept_x[FIFA96_MATCH_ENTITY_TEAMS]; /* word 0x10F37C+side*0xC */
  int16_t intercept_y[FIFA96_MATCH_ENTITY_TEAMS]; /* word +8 of the same record */
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

/* The FU-137 §2 installer FUN_0007D9A4 over a derived record: `code` is the
 * requested action, `staged` the BL stage byte. Rejects an occupied record
 * (+0x9A) and the same code; clears +0x98 in the unported animation arm; coerces
 * an inactive code 3 to 0x19; sets/clears the +0x9F carrier bit for codes
 * 5/0x21; stages the code, zeroes +0x89, clears +0x9E, writes +0x92 and copies
 * +0x79 -> +0x7B. Returns 1 when a code was staged, 0 when rejected. */
int fifa96_match_entities_install(struct fifa96_match_entity *entity, uint8_t phase,
                                  uint8_t code, uint8_t staged);

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

/* The FU-67 §4.4 / FUN_0007D430 pairing at the chain tail: phase 2 only, both
 * team targets present, the FU-139-tested fifa96_ball_pair_decide writes the
 * team-0 target's output triple on the found condition. Returns FIFA96_OK or a
 * propagated negative error. */
int fifa96_match_entities_ball_pair(struct fifa96_match_entities *pool);
