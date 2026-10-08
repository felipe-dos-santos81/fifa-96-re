#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_OUTFIELD_CODE_COUNT 5

typedef struct fifa96_outfield_rule {
  uint16_t mask;
  uint16_t want;
  uint32_t handler;
} fifa96_outfield_rule;

typedef int (*fifa96_outfield_rule_fn)(uint32_t handler, void *context);

const fifa96_outfield_rule *fifa96_outfield_pressed_rules(uint8_t code);
const fifa96_outfield_rule *fifa96_outfield_released_rules(uint8_t code);
int fifa96_outfield_rule_match(const fifa96_outfield_rule *rule, uint16_t word);
int fifa96_outfield_rules_run(const fifa96_outfield_rule *rules, uint16_t word,
                              fifa96_outfield_rule_fn call, void *context,
                              uint32_t *handler);

typedef struct fifa96_outfield_edge {
  uint16_t pressed;
  uint16_t released;
  uint8_t type;
  uint8_t high_577ee_ge_50;
  uint8_t tracked;
  uint8_t user_absent_or_self;
  uint8_t chaser;
} fifa96_outfield_edge;

int fifa96_outfield_dispatch_code(const fifa96_outfield_edge *edge, uint8_t *code);

typedef struct fifa96_outfield_forced_state {
  uint8_t is_team_controlled;
  uint8_t is_team_second;
  uint8_t type_5;
  uint8_t opponent_has_ball;
  uint8_t controlled_has_ball;
} fifa96_outfield_forced_state;

int fifa96_outfield_forced_action(const fifa96_outfield_forced_state *state, uint8_t current,
                                  uint8_t *next);

typedef struct fifa96_outfield_chase_state {
  uint8_t phase;
  uint8_t type_gate;
  uint8_t not_team_controlled;
  uint8_t not_team_second;
  uint16_t distance;
  uint16_t camera;
  uint8_t user_present;
  uint8_t sides_differ;
  uint8_t unbound;
  uint16_t timer;
  uint8_t third_zero;
} fifa96_outfield_chase_state;

int fifa96_outfield_chase_action(const fifa96_outfield_chase_state *state, uint8_t current,
                                 uint8_t *next);

/* ===== FU-75 §1.2/§1.3/§1.7 + FU-137 §4.1 (M2 Task 14 / OL-38): the outfield
 * record machine's per-record decision `FUN_0007CA54` subset. The native
 * machine (`0x7CA54..0x7CD53`, FU-75 §1) runs, for one record:
 *   - the input-row dispatch (`0x7CB0D..0x7CC11`): when the record owns a
 *     control slot (`+0x20`, `0x7CABA`), the pre-gate `0x7CAC4..0x7CB08`
 *     (byte `[0x157AB0]`, `rec == [0x1587AC]`, type 3, the released bit
 *     `0x20`) may call the `0x7D1D4` selection handler directly; otherwise the
 *     raw slot words `[+4]`/`[+6]` select a dispatch code 0..4 (`0x7CB22`,
 *     `fifa96_outfield_dispatch_code`) and either the pressed word
 *     (`&0xFF0`, pressed table) or — only when the pressed word is zero — the
 *     released word (released table) runs the row scan. The scan stops (to the
 *     tail) on a matching row with handler 0 or a handler returning non-zero
 *     (`0x7CBAC..0x7CBB0`, `0x7CBC6`); it never runs both tables;
 *   - the no-edge arm (`0x7CC13..0x7CC7D`): both raw words zero, `(slot[+0x10]
 *     & 0xF0) != 0`, phase 2, and either `0x30 < (int16)(lane>>16) < 0x90` or
 *     the side-filtered `slot[+0x10] & 0xC0` path; fires the camera-triple copy
 *     into `+0x4D..+0x55` and `FUN_00079B58` (`receiver_timer`);
 *   - the phase-2 tail (`0x7CC82..0x7CD24`): the per-type gate flat
 *     `0x110680[type]&1`, then `FUN_0007C990` (`fifa96_outfield_forced_action`)
 *     and the code-8 chase gate (`fifa96_outfield_chase_gate`).
 * The machine's final `CALL [rec+0x18]` + `FUN_0006E8E8`/`FUN_00079B1C`/
 * `FUN_0007BF20` tail is the engine dispatch itself and is not re-modelled
 * here. Returns the file's `int` error convention (`-FIFA96_ERR_INVALID`); the
 * task brief's `fifa96_err_t` names the same negative values, and this file's
 * whole public API is `int`-typed. */
typedef struct fifa96_outfield_input_state {
  uint8_t has_slot;      /* record +0x20 != 0 */
  uint8_t phase;         /* [0x157A4A]>>24 */
  uint16_t pressed;      /* slot[+4] raw word */
  uint16_t released;     /* slot[+6] raw word */
  uint16_t slot_word10;  /* slot[+0x10] word */
  int32_t lane;          /* dword record[+0x69] (>>16 = the lane word) */
  uint8_t user_present;  /* [0x157A83] != 0 */
  uint8_t user_side;     /* [[0x157A83]]+0x826 */
  uint8_t side;          /* record's team +0x826 */
  uint8_t type;          /* record +0x8E>>24 */
  uint8_t high_577ee_ge_50;
  uint8_t tracked;
  uint8_t user_absent_or_self;
  uint8_t chaser;
  uint8_t flag_157ab0;   /* byte [0x157AB0] != 0 (0x7CAC4) */
  uint8_t is_1578ac;     /* record == [0x1587AC] (0x7CACD) */
  fifa96_outfield_forced_state forced;
  uint8_t current_code;  /* record +0x91 */
  fifa96_outfield_chase_state chase;
} fifa96_outfield_input_state;

typedef struct fifa96_outfield_input_out {
  uint8_t direct_arm;    /* the 0x7D1D4 pre-gate fired (0x7CB03) */
  uint8_t no_edge;       /* both slot words zero */
  uint8_t scan_ran;      /* a pressed/released row scan ran */
  uint8_t scan_code;     /* the selected dispatch code 0..4 */
  uint8_t scan_stopped;  /* a row handler accepted (returned non-zero) */
  uint8_t no_edge_arm;   /* the camera copy + FUN_00079B58 request fired */
  uint8_t forced;        /* FUN_0007C990 install request */
  uint8_t forced_code;
  uint8_t chase;         /* the code-8 chase gate install request */
} fifa96_outfield_input_out;

int fifa96_outfield_input_row(const fifa96_outfield_input_state *state,
                              fifa96_outfield_rule_fn call, void *context,
                              fifa96_outfield_input_out *out);

/* The code-8 chase gate: the per-type gate flat `0x110680[type]&1` composed
 * with `fifa96_outfield_chase_action` (FU-75 §1.6). Returns 1 when code 8 is
 * requested, 0 when gated out or already current, -FIFA96_ERR_INVALID on NULL.
 * `type` beyond 0x19 returns 0 (the native reads adjacent image bytes there;
 * no real record type reaches it). */
int fifa96_outfield_chase_gate(const fifa96_outfield_chase_state *state, uint8_t type,
                               uint8_t current, uint8_t *next);

/* ===== M2 playability-legs Task 1 / OL-70: row 04 record-visible body =====
 *
 * The native row-04 handler `FUN_0007E7C8` (`0x7E7C8..0x7F141`, RET at
 * `0x7F141`; FU-142 Appendix K.4 splits it from OL-38) is the chase/pressure
 * action, ported here as `fifa96_outfield_row04_step` from the first-hand
 * window (disassemble_bytes 0x7E7C8..0x7F141; helpers 0x8DE8C/0x8DDE0/
 * 0x8DCD4/0x8DC68/0x8DD70/0x7D3E4/0x79C20/0x79B58/0x7E528/0x7E600; installer
 * 0x7D9A4; everything cited in FU-142 Appendix K.5).
 *
 * The body in native order (byte/word widths as read):
 *   - `0x7E7D3` `byte[+0x9E] = 1`;
 *   - `0x7E7E2` phase `[0x157A4A]>>24 != 2` -> `reset` (FUN_0007DAB4);
 *   - `0x7E7F3` `word[+0x81] != 0` -> return;
 *   - `0x7E80E` actor == `[0x158777]` (carrier): `0x8DE8C` nearest over the
 *     own team (origin `0x157770`, skip = the actor's `+0x8D` byte) ->
 *     `[team+0x7B2]`; `[nearest+0x8E]>>24` (byte +0x91) gates the flat
 *     `0x110680&1`; on pass install 4 then `+0x8D ? 3 : 0x19` (both
 *     invoke-now), on fail return;
 *   - `0x7E888` `+0x8D == 0` (inactive): `FUN_0008DDE0(opponent team, 0)`
 *     (smallest `+0x6B` lane, first on ties) when `[opp+0x7C7] == 0`;
 *     `picked.lane > actor.lane` -> the active target arms, else install
 *     0x19 and recompute `[team+0x7B2]`/`[+0x7B6]` (`0x8DE8C`, skip 0) when
 *     the actor is that pointer;
 *   - `0x7E92F` active: actor == `[+0x7B6]` and `[+0x7B2]` -> clear `+0x7B6`;
 *     actor == `+0x7B6` with a non-NULL `+0x7B2` whose byte +0x91 == 4 and
 *     `+0x6B <= actor+0x6B` -> `reset` and return;
 *   - `0x7E984..0x7EB93` target arms (ECX = "carrier/camera lead" latch):
 *     the `[0x1577CA]`+`[0x157750]>0x50` flag takes the camera arm; else the
 *     `word[0x1577F0]` bands select the `0x157788` (bucket `0x1577FA <
 *     0x157800`, with the two ±0x10 RNG jitters), `0x157794` (`0x1577FA <
 *     0x157806`) or camera (`0x15774C` + `word[0x1577C0]/[0x1577C2]<<2`)
 *     triple; the `w > 0x70` band runs the `0xF` install gate
 *     (`0x7EA7B..0x7EB5C`); ECX == 0 takes the `0x79C20`
 *     `(pos + slot byte<<7)` target and its `0x7D3E4` clamp;
 *   - `0x7EC13` `0x7D3E4` clamp of the `+0x4D` triple (±0x720 x, ±0xB10 z);
 *   - `0x7EC1B` `timer89 += (uint16)[0x157A64]`;
 *   - `0x7EC3C` no-slot: `[team+0x828]!=0` gates the `0x7876C` merge request
 *     (`[team+0x7E7]==0`, lane `< 0xF0`, `[team+0x7BF]==0`, active,
 *     `[0x1586D7]==0`), else the `0x7E600` decision (install `0x0E` and
 *     return on hit); `0x7ED79` slot `+6 != 0` -> `0x78A84` backup request;
 *   - `0x7EC97..0x7ED6A` the RNG/score/`3*[team+0x7D7]` gate -> install `0xB`;
 *   - `0x7ED89` lane tail: `0x40 - timer81 >= lane` -> the bound/ball-height
 *     tail, else `[0x1577CA]`/lane `0x90` gates and `0x79B58`;
 *   - `0x7EDCE` `lane > word[+0x77]` return; `[0x157750] <= 0x50` -> the
 *     `+0x6D`/`+0x6F` angle arm, else active/`+0x5D`/`0x8DCD4(0x157770)`
 *     gates, row byte `[[rec+0x28]] == 0x13` return and the `0x6E598`
 *     request;
 *   - `0x7EE4B` angle arm: `0x8DD70(word[+0x6D], word[+0x6F])` masked to
 *     10 bits, mirrored over 0x200, `> 0xAB` returns; the `0x1577CA` side
 *     arm; if actor == `[team+0x7B6]` -> clear it and promote the actor to
 *     `[team+0x7B2]`; clear `[opp+0x7E7]`; `[team+0x7E7]` nonzero ->
 *     `0x7E528` (install 7, `+0x7E7` increment, `0x158743 = 2/3`) else the
 *     slot-restore/`[team+0x7CB]` install-7 arm;
 *   - `0x7EF42` row byte `0x13`: `0x974DC`/`0x8F188` + `0x92820` event 0x15
 *     and `word[0x1577FA] = word[0x1577F2] + word[0x1577F2]>>2`; otherwise
 *     the type-table `<<5` (ball height), slot `+0x20/+0x21 <<6` or velocity
 *     arm vector -> `0x92820` event 0x1D;
 *   - `0x7F0D6` tail: active and the opponent-team target present with byte
 *     +0x91 in {4,5} -> install 6 (on the actor when its `+0x6B` is greater,
 *     else on that record, both no-invoke), then install 5 (invoke-now).
 *
 * The `installs[]` sequence is applied in order by the engine binder (the
 * native invokes immediately; the record fields are the sequence's net, and
 * the derived installer's same-code/occupied guards reproduce the native
 * rejections). The `installs[].target` distinguishes the lone cross-record
 * install (the `0x7F113` code-6 on the opponent target). `events` marks the
 * `0x974DC`/`0x8F188`/`0x92820`/`0x71C94`/`0x974F0`/`0x651F0`
 * presentation/sound calls whose consumer blocks are unported (OL-72).
 * NULL `state`/`out` or a NULL `rng` -> -FIFA96_ERR_INVALID; a NULL
 * `mates`/`opps` array makes the corresponding search return NONE (the
 * bounded stand-in for the native low-memory read when no record qualifies). */
#define FIFA96_OUTFIELD_ROW04_INSTALL_SELF 0u
#define FIFA96_OUTFIELD_ROW04_INSTALL_OTHER 1u
#define FIFA96_OUTFIELD_ROW04_INSTALL_MAX 3u
#define FIFA96_OUTFIELD_ROW04_NONE (-1)

struct fifa96_rng;

typedef struct fifa96_outfield_row04_install {
  uint8_t target;   /* SELF or OTHER (the opponent team's target record) */
  uint8_t code;     /* native EDX code */
  uint8_t staged;   /* native BL stage byte (`+0x92` after install) */
  uint8_t invoke;   /* native ECX != 0 (invoke-now). Carried as evidence; the
                     * binder applies the whole sequence synchronously, so it
                     * is not consumed (the standing invoke-now convention) */
} fifa96_outfield_row04_install;

typedef struct fifa96_outfield_row04_mate {
  int16_t x;        /* +0x59 word (0x8DE8C nearest metric) */
  int16_t z;        /* +0x61 word */
  int32_t pos_x;    /* +0x59 dword (0x8DCD4 metric) */
  int32_t pos_z;    /* +0x61 dword */
  int16_t lane;     /* +0x6B word (0x8DDE0 pick, 0x7E969 compare) */
  uint8_t code;     /* +0x91 action code (0x7E964 ==4, 0x7F0E7 {4,5}) */
  uint8_t skip_98;  /* +0x98 exclusion */
  uint8_t skip_9a;  /* +0x9A exclusion */
} fifa96_outfield_row04_mate;

typedef struct fifa96_outfield_row04_state {
  uint8_t phase;             /* [0x157A4A]>>24 */
  uint8_t active;            /* +0x8D */
  uint8_t has_slot;          /* +0x20 != 0 */
  uint8_t byte99;            /* +0x99 */
  uint8_t byte9d;            /* +0x9D (install-0xB threshold OR) */
  uint8_t type8;             /* +0x8E byte (0x10F334/0x10F33C/0x6E598 arg) */
  uint8_t code;              /* +0x91 action code (0x110680/0x7E600/0x7E964) */
  uint8_t row_byte;          /* byte[[rec+0x28]] (the 0x13 gate) */
  uint8_t desc_e;            /* rec[+4][+0xE] byte (install-0xB threshold) */
  uint16_t timer81;          /* +0x81 */
  int32_t timer89;           /* +0x89 (in; the body adds `delta`) */
  uint16_t delta;            /* word [0x157A64] (zero-extended) */
  int32_t pos_x, pos_y, pos_z; /* +0x59/+0x5D/+0x61 */
  int16_t lane;              /* +0x6B word */
  int16_t bound;             /* +0x77 word */
  int16_t word6d, word6f;    /* words +0x6D/+0x6F (the angle arm) */
  int16_t vel_int_x, vel_int_z; /* word[+0x73]/[+0x75] (camera-arm vector) */
  int16_t face_word7d;       /* (int16)word[+0x7D] (0x7E600) */
  int8_t slot_dir_x, slot_dir_z; /* slot +0x20/+0x21 bytes */
  uint16_t slot_word10;      /* slot +0x10 word */
  uint16_t slot_word6;       /* slot +0x6 word (backup gate) */
  int32_t target_x, target_y, target_z; /* +0x4D/+0x51/+0x55 (in/out) */
  /* Derived identity stand-ins (native pointers not modelled by the pool). */
  int32_t self_index;        /* actor index in `mates`, or NONE */
  int32_t team_target_index; /* [team+0x7B2] index in `mates`, or NONE */
  int32_t team_second_index; /* [team+0x7B6] index in `mates`, or NONE */
  int32_t opp_target_index;  /* [[opp]+0x7B2] index in `opps`, or NONE */
  int32_t opp_7c7_index;     /* [[opp]+0x7C7] index in `opps`, or NONE */
  uint8_t is_carrier;        /* actor == [0x158777] */
  uint8_t is_ball_track;     /* actor == [0x1577CA] */
  uint8_t is_team_7c7;       /* actor == [team+0x7C7] */
  uint8_t is_team_7cb;       /* actor == [team+0x7CB] */
  uint8_t user_present;      /* [0x157A83] != 0 */
  uint8_t user_is_self;      /* actor == [0x157A83] */
  uint8_t side;              /* team+0x826 */
  uint8_t team_828;          /* byte[team+0x828] */
  uint8_t team_7e7;          /* byte[team+0x7E7] */
  uint8_t team_7bf;          /* dword[team+0x7BF] != 0 */
  uint8_t merge_gate_1586d7; /* byte[0x1586D7] != 0 */
  int32_t team_7d7;          /* dword[team+0x7D7] */
  int32_t opp_7d7;           /* dword[opp+0x7D7] */
  int32_t team_corner_z;     /* z of team[+0x7E8 + 12*byte(team+0x7E7)] (0x7E528) */
  int16_t score_word[2];     /* words [0x157AC5]/[0x157AC7] */
  uint8_t side_flip;         /* byte[0x157ABE] (0x741B4 xor) */
  int32_t ball_height;       /* dword [0x157750] */
  int32_t camera_x, camera_y, camera_z;      /* 0x15774C/50/54 */
  int32_t vec5770_x, vec5770_y, vec5770_z;   /* 0x157770 (search origin/copy) */
  int32_t vec5788_x, vec5788_y, vec5788_z;   /* 0x157788 */
  int32_t vec5794_x, vec5794_y, vec5794_z;   /* 0x157794 */
  int16_t lead_x, lead_z;    /* word[0x1577C0], word[0x1577C2] */
  uint16_t track_577f0;      /* word 0x1577F0 (band gate) */
  uint16_t track_577f2;      /* word 0x1577F2 (0xF gate / 0x7FA reload) */
  uint16_t track_577fa;      /* word 0x1577FA (bucket timer a) */
  uint16_t track_57800;      /* word 0x157800 (bucket timer b) */
  uint16_t track_57802;      /* word 0x157802 (0xF gate sum) */
  uint16_t track_57806;      /* word 0x157806 (bucket timer c) */
  int32_t predictor_x, predictor_y, predictor_z; /* 0x71B9C(4) (0x7E600) */
  uint8_t ball_track_side;   /* side of [0x1577CA] (0x7EE9D compare) */
  const int8_t *type_off_x;  /* 0x10F334 table (8-bit entries) */
  const int8_t *type_off_z;  /* 0x10F33C table */
  const fifa96_outfield_row04_mate *mates;   /* own team block */
  uint32_t mate_count;
  const fifa96_outfield_row04_mate *opps;    /* opponent team block */
  uint32_t opp_count;
  struct fifa96_rng *rng;
} fifa96_outfield_row04_state;

typedef struct fifa96_outfield_row04_out {
  uint8_t ran;               /* +0x9E latch */
  uint8_t reset;             /* FUN_0007DAB4 request */
  uint8_t target_set;        /* the +0x4D triple was clamped (always on the
                              * phase-2 path past 0x7EC13) */
  int32_t target_x, target_y, target_z;
  int32_t timer89;           /* +0x89 after the `+= delta` line (0x7EC1B) */
  uint8_t install_count;
  fifa96_outfield_row04_install installs[FIFA96_OUTFIELD_ROW04_INSTALL_MAX];
  uint8_t team_target_set;   /* [team+0x7B2] write */
  int32_t team_target_index; /* `mates` index or NONE */
  uint8_t team_second_set;   /* [team+0x7B6] write */
  int32_t team_second_index; /* `mates` index or NONE */
  uint8_t receiver_timer;    /* 0x79B58(actor): +0x93 = 0x10 when +0x99 == 0 */
  uint8_t slot_merge;        /* 0x7876C request */
  uint8_t slot_backup;       /* 0x78A84(slot) request */
  uint8_t slot_restore;      /* 0x78AA4(slot) request */
  uint8_t anim;              /* 0x6E598(kind 0x13, type8) request */
  uint8_t corner;            /* 0x7E528 ran */
  uint8_t corner_code;       /* byte 0x158743 staged (2/3) */
  uint8_t team7e7_inc;       /* team+0x7E7 incremented (wrap > 2 -> 0) */
  uint8_t opp_7e7_clear;     /* opponent team+0x7E7 = 0 */
  uint8_t events;            /* 0x974DC/0x8F188/0x92820/0x71C94/0x974F0/
                              * 0x651F0 presentation/sound calls ran (OL-72) */
  uint8_t event_code;        /* 0x92820 code 0x15 (row byte 0x13) / 0x1D */
  int16_t event_dist;        /* the 0x8DC68 distance word ([ESP]) */
  uint8_t event_track_reload;/* word[0x1577FA] = word[0x1577F2] +
                              * (word[0x1577F2] >> 2) ran (0x7EFD4) */
  int16_t event_x, event_z;  /* the event vector words ([ESP+2]/[ESP+4]) */
} fifa96_outfield_row04_out;

fifa96_err_t fifa96_outfield_row04_step(const fifa96_outfield_row04_state *state,
                                        fifa96_outfield_row04_out *out);
