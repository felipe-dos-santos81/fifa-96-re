/* include/fifa96_loader/fifa96_referee.h — FU-150 P2: fouls / referee / offside
 * (M2 phase-7 ports).
 *
 * The derived loader module for the live-match referee system, from the frozen
 * slice docs/ghidra/FU150_fouls_referee_offside.md (the requirements source;
 * key claims re-verified first-hand on /FIFA96.EXE for this port):
 *
 *  - contact registrar FUN_0008A3FC (0x8A3FC..0x8A43B): stores the contact
 *    kind [0x15888C], record A [0x15888F], record B [0x158893] and the 12-byte
 *    point triple 0x158897/0x15889B/0x15889F;
 *  - foul decision FUN_0008A43C (0x8A43C..0x8A794): phase-2 gate, settings
 *    [0x14C306] (0 = no decision, 1 = kind stays 0, >1 = severity RNG
 *    `0x92AC8 & 0x3F`), severity accumulator [0x157B90], team count
 *    [team+0x827], point y forced 0 + FUN_0007D3E4 clamp; kind 0 -> the
 *    dispatcher situation 9 (fouled side, BX=1) or kind != 0 -> the foul log
 *    ring [0x157B3E] + act 3 (phase 0x19); the AX==3 arm (offside) speaks
 *    0x15 and requests act 6 (phase 0x1C) behind the settings [0x14C2F2] gate;
 *  - the phase-0x19 7-stage machine 0x89FA4 (stage jump table 0x89F88):
 *    stage 0 whistle + foul counter + phase 0xF on the fouled side + install
 *    0x16; stage 2 speech by foul kind; stage 4 severity sum -> install 0x18
 *    when `(sum & 0x7F) >= 2` (else stage += 2 skipping stage 5); stage 5
 *    team-count decrement only while the fouler is held ([rec+0x9A] != 0);
 *    stage 6 the referee-object gate then situation 0xA on the fouled side;
 *  - the phase-0x1C 3-stage machine 0x89110: stage 0 whistle + phase 0xA side
 *    0; stage 2 situation 9 on the opponent of the offside record;
 *  - the offside check FUN_00079D5C (0x79D5C..0x79F38): phase-2/suppression
 *    [0x157A6A]/settings 0x10 gates, camera-mirror gate ([0x157754]/
 *    [0x157823]), the staged metric block 0x158738 side gate, the own-team
 *    nearest query and the ±0xB10 last-defender query with the 6-bit RNG
 *    tolerance, the record-state gate, and the kind-3 event.
 *
 * Caller-owned state; all functions return 0 or a negated -fifa96_err_t.
 * Unreachable machinery stays numbered legs in FU-150 (settings labels, RNG
 * identity, whistle/sound mapping, referee-object identity, team-count
 * predicate, downed-vs-sent-off, [0x15888E] lifecycle, kind-3 geometry
 * inputs, 0x14C360/0x157BD2 severity tables, act-2 gate [0x158882]). */
#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_settings.h"

#define FIFA96_REF_FOUL_LOG 10u
#define FIFA96_REF_SEVERITY_PLAYERS 11u
#define FIFA96_REF_TEAMS 2u

/* The derived active-sequence states (`fifa96_ref_foul_decide` out and the
 * engine dispatcher). NONE = the severity-0 restart fork (situation 9). */
enum fifa96_ref_sequence {
  FIFA96_REF_SEQ_NONE = 0,
  FIFA96_REF_SEQ_ACT3 = 1, /* foul sequence, phase 0x19 (FUN_00089FA4) */
  FIFA96_REF_SEQ_ACT6 = 2  /* offside sequence, phase 0x1C (FUN_00089110) */
};

/* The engine record view the native bodies dereference: `id` is the caller's
 * record identity (the engine entity id, opaque to the module); `side` is the
 * raw team byte [team+0x826]; `player` the [rec+0x8A]>>24 player index;
 * `active` the [rec+0x8D] severity precondition; `duel_ok` the native
 * `[0x14C360][side*0x7B + player] == [0x157BD2][side*0x2C + player]` compare
 * (0x8A543/0x8A549) whose two table producers are outside this slice (leg:
 * callers stage 1 for the image-default equality). */
struct fifa96_ref_record {
  int32_t id;
  uint8_t side;
  uint8_t player;
  uint8_t active;
  uint8_t duel_ok;
};

/* One foul-log ring entry (native minute word 0x157B3E+2i, kind byte
 * 0x157B52+i, side-swapped team byte 0x157B5C+i, player dword 0x157B66+4i). */
struct fifa96_ref_foul_log_entry {
  uint16_t minute;
  uint8_t kind;
  uint8_t team;
  int32_t player;
};

/* The caller-owned referee state; zero-init for a fresh match. Every field is
 * a native cell (FU-150 §Port contract) or a derived/staged input:
 *  - phase             staged [0x157A4A]>>24 (the engine stages the live phase);
 *  - side_swap         staged [0x157ABE] (the FU-000741B4 index fold);
 *  - offside_suppress  [0x157A6A] (the caller drives the 0x7438D countdown);
 *  - game_minute       staged [0x157AB4] foul-log minute;
 *  - delta             staged [0x157A64] frame delta (sequence timer head);
 *  - contact_kind      [0x15888C];
 *  - foul_kind         [0x15888D];
 *  - recall_consumed   [0x15888E] (row-0x0C re-call flag; engine writes it);
 *  - rec_first         [0x15888F]; rec_first_side/_player its staged team/player
 *                      (the module cannot dereference ids);
 *  - rec_second        [0x158893];
 *  - point             [0x158897/0x15889B/0x15889F];
 *  - rec_first_held    staged [rec+0x9A] (stage-5 send-off gate; the record
 *                      itself is engine-owned, leg 6);
 *  - referee_object_code staged [[0x158866]] code byte (stage 6 waits while 0x48);
 *  - sequence/stage    the active machine and [0x158829];
 *  - timer             [0x158818];
 *  - log_count/log     [0x157AC4] ring;
 *  - fouls_by_side     [0x157AD8];
 *  - team_count        [team+0x827];
 *  - severity          [0x157B90]. */
struct fifa96_referee_state {
  uint8_t phase;
  uint8_t side_swap;
  uint16_t offside_suppress;
  uint16_t game_minute;
  uint16_t delta;
  uint8_t contact_kind;
  uint8_t foul_kind;
  uint8_t recall_consumed;
  int32_t rec_first;
  uint8_t rec_first_side;
  uint8_t rec_first_player;
  int32_t rec_second;
  int32_t point[3];
  uint8_t rec_first_held;
  uint8_t referee_object_code;
  uint8_t sequence;
  uint8_t stage;
  uint16_t timer;
  uint8_t log_count;
  struct fifa96_ref_foul_log_entry log[FIFA96_REF_FOUL_LOG];
  uint16_t fouls_by_side[FIFA96_REF_TEAMS];
  uint8_t team_count[FIFA96_REF_TEAMS];
  uint8_t severity[FIFA96_REF_TEAMS][FIFA96_REF_SEVERITY_PLAYERS];
};

/* `FUN_0008A43C` normal-path outcome: `restart` set -> the severity-0 arm
 * emitted situation 9 on `restart_side` (the fouled side); otherwise
 * `sequence == FIFA96_REF_SEQ_ACT3` with the derived `foul_kind` and the foul
 * log appended. */
struct fifa96_ref_decision_out {
  uint8_t sequence;
  uint8_t foul_kind;
  uint8_t restart;
  uint8_t restart_side;
};

/* `FUN_0008A43C` kind-3 arm outcome: the speech 0x15 event request and the
 * act-6 (phase 0x1C) sequence request on `rec_id`. */
struct fifa96_ref_event_out {
  uint8_t speech_code;
  uint8_t sequence;
  int32_t rec_id;
};

/* One sequence step's derived requests. `whistle` is the 0x974DC(0x1E)
 * stoppage push, `speech_code` the 0x8F188 speaker code (0 = none),
 * `phase_write`/`phase_side` the FUN_000740A0 phase request (0xFF = none),
 * `install_action` the FUN_0007D9A4 action on rec_first (0 = none),
 * `foul_counter`/`team_count_dec` report the side effects the step already
 * applied to `fouls_by_side`/`team_count`, `situation`/`situation_side` the
 * FUN_0008A938 dispatch request (0xFF = none, BX=1), `done` the machine end
 * (the sequence field is cleared and the stage reset). */
struct fifa96_ref_sequence_out {
  uint8_t whistle;
  uint8_t speech_code;
  uint8_t phase_write;
  uint8_t phase_side;
  uint8_t install_action;
  uint8_t foul_counter;
  uint8_t team_count_dec;
  uint8_t situation;
  uint8_t situation_side;
  uint8_t done;
};

/* The staged receiver inputs of `fifa96_ref_offside_check`: `z`/`side` are the
 * receiver record's +0x61/team byte; the own-nearest fields are the derived
 * FUN_0008DE8C(&0x157770, own team, 0, out) result (`own_nearest_valid`,
 * `own_nearest_is_receiver`, `own_nearest_z`, `own_distance` = its out metric
 * word); `eligible` is the final record-state gate (0x79EE1..0x79F1D:
 * ESI != 0, != [0x158777], [rec+0x8E]>>24 != 0x11, code not in {0x10,0x1D,0x1E}). */
struct fifa96_ref_receiver {
  int32_t z;
  uint8_t side;
  uint8_t own_nearest_valid;
  uint8_t own_nearest_is_receiver;
  int32_t own_nearest_z;
  int16_t own_distance;
  uint8_t eligible;
};

/* The staged 0x158738 metric block reads (producers unported, FU-150 leg 8):
 * `tol2` = the signed word at +0 (the ×2 distance tolerance, 0x79E54/0x79EC7)
 * and `side_gate` = the signed word at +4 (the side pick, 0x79DCA/0x79DDD). */
struct fifa96_ref_metric {
  int16_t tol2;
  int16_t side_gate;
};

/* `FUN_0008A3FC`: store the contact triple. `point` is the caller's 12-byte
 * source (NULL -> the derived zero triple; the native's NULL fallback reads
 * rec_b/rec_a +0x59, which the module cannot dereference). Returns 0 or
 * -FIFA96_ERR_INVALID (NULL state). */
int fifa96_ref_contact_register(struct fifa96_referee_state *state, uint8_t kind,
                                int32_t rec_a, int32_t rec_b, const int32_t point[3]);

/* `FUN_0008A43C` kind-1/2 normal path, first-hand byte-exact. The native
 * entry kind AX is staged in `state->contact_kind` (the native's only normal
 * caller, the row-0x0C re-call at 0x81EBF, passes 1; the engine stages that
 * literal after the registrar stores the original contact kind):
 *  - derives nothing unless `[0x157A4A]>>24 == 2` (staged `phase`), the fouler
 *    is non-NULL and `cfg->field_4c306 != 0` (return 0 = no decision);
 *  - stores contact_kind (the staged entry kind), rec_first/rec_second, the
 *    point (source: `point`, then the stored point) with y forced 0 and the
 *    FUN_0007D3E4 x/z clamp [-0x720,0x720]/[-0xB10,0xB10];
 *  - settings > 1: severity from `rng_bits & 0x3F` and the staged fouler view
 *    (active/duel_ok, severity accumulator, team count): `< 0x12` -> kind 1
 *    when `(acc & 0x7F) == 0`, else kind 2 when team_count > 8; `[0x12,0x16)`
 *    with contact kind 2 and team_count > 8 -> kind 3; otherwise kind 0;
 *  - kind 0 -> `restart` = 1, `restart_side` = rec_first_side ^ 1;
 *    kind != 0 -> the foul-log append (wrap at 10) and ACT3. */
int fifa96_ref_foul_decide(struct fifa96_referee_state *state,
                           const struct fifa96_match_config *cfg,
                           const struct fifa96_ref_record *fouler,
                           const struct fifa96_ref_record *victim,
                           const int32_t point[3], uint8_t rng_bits,
                           struct fifa96_ref_decision_out *out);

/* `FUN_00079D5C` derived check (first-hand inequalities, FU-150 E7):
 * phase 2, suppression not positive, settings 0x10, the camera gate
 * `|camera_ref| <= 0x990 || mirror == 0`, the side gate on metric->side_gate,
 * own-nearest != receiver with the ±0x3B0 depth gate, the receiver-vs-last-
 * defender direction gate, the 6-bit RNG tolerance on own_nearest_z, the
 * metric->tol2 ×2 distance term, the own_distance <= 0x3C0 gate and the
 * `eligible` record-state gate. `last_defender_z` is the caller's
 * FUN_0008DE28(±0xB10, other team, 0) result. Returns 0 with `*offside` set,
 * or -FIFA96_ERR_INVALID (NULL state/cfg/receiver/metric/offside). */
int fifa96_ref_offside_check(const struct fifa96_referee_state *state,
                             const struct fifa96_match_config *cfg,
                             const struct fifa96_ref_receiver *receiver,
                             const struct fifa96_ref_metric *metric,
                             int32_t last_defender_z, int32_t camera_ref,
                             uint8_t mirror, uint8_t rng_bits, uint8_t *offside);

/* `FUN_0008A43C` kind-3 arm: behind `cfg->field_4c2f2 != 0`, store the kind-3
 * event (contact_kind 3, rec_first = rec->id/_side/_player, rec_second 0, the
 * point with the FUN_0007D3E4 clamp and y preserved), clear the sequence to
 * ACT6 and report the speech 0x15 request. Returns 1 when the event was
 * stored (0 = settings gate), or -FIFA96_ERR_INVALID (NULL args). */
int fifa96_ref_offside_event(struct fifa96_referee_state *state,
                             const struct fifa96_match_config *cfg,
                             const struct fifa96_ref_record *rec,
                             const int32_t point[3],
                             struct fifa96_ref_event_out *out);

/* The phase-0x19 7-stage machine (`state->stage` 0..6, `state->sequence` must
 * hold FIFA96_REF_SEQ_ACT3). One call runs the current stage (the native jump
 * table dispatch); the stage-1/3 FUN_0004BEC8 gate and the stage-2 camera-lead
 * gates are derived ready (FU-150 leg 10). Stage 6 waits while
 * `referee_object_code == 0x48` (first-hand `JZ return`; the engine stages 0
 * by default, which proceeds). Returns 0 or -FIFA96_ERR_INVALID. */
int fifa96_ref_foul_sequence_step(struct fifa96_referee_state *state,
                                  struct fifa96_ref_sequence_out *out);

/* The phase-0x1C 3-stage machine (`state->sequence` must hold
 * FIFA96_REF_SEQ_ACT6). Stage 0 requests whistle + phase 0xA on side 0;
 * stage 2 dispatches situation 9 on `rec_first_side ^ 1`, clears the sequence
 * and resets the stage. Returns 0 or -FIFA96_ERR_INVALID. */
int fifa96_ref_offside_sequence_step(struct fifa96_referee_state *state,
                                     struct fifa96_ref_sequence_out *out);
