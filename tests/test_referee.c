/* tests/test_referee.c — FU-150 P2: fouls / referee / offside module (loader).
 *
 * Requirements source: docs/ghidra/FU150_fouls_referee_offside.md. Every
 * expected value below is either a byte-exact read of the native bodies
 * (0x8A3FC registrar, 0x8A43C decision, 0x89FA4 foul machine, 0x89110
 * offside machine, 0x79D5C check) or a derived value marked in the FU-150
 * leg list. The engine-chain tests live in test_engine_match_frame.c. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_referee.h"

static struct fifa96_referee_state fresh_state(void) {
  struct fifa96_referee_state st;
  memset(&st, 0, sizeof st);
  st.phase = 2;   /* the live in-play phase the native gates require */
  return st;
}

static struct fifa96_match_config fresh_cfg(void) {
  struct fifa96_match_config cfg;
  memset(&cfg, 0, sizeof cfg);
  return cfg;
}

static struct fifa96_ref_record record(int32_t id, uint8_t side, uint8_t player) {
  struct fifa96_ref_record r;
  memset(&r, 0, sizeof r);
  r.id = id;
  r.side = side;
  r.player = player;
  r.active = 1;
  r.duel_ok = 1;
  return r;
}

/* E2: the registrar stores kind/A/B and the 12-byte point triple; a NULL
 * point is the derived zero triple (the native rec+0x59 fallback needs record
 * memory the module does not own). */
static void test_contact_register(void) {
  struct fifa96_referee_state st = fresh_state();
  const int32_t point[3] = {0x100, 0x200, 0x300};
  assert(fifa96_ref_contact_register(&st, 1, 7, 8, point) == 0);
  assert(st.contact_kind == 1u && st.rec_first == 7 && st.rec_second == 8);
  assert(st.point[0] == 0x100 && st.point[1] == 0x200 && st.point[2] == 0x300);
  assert(fifa96_ref_contact_register(&st, 2, 9, 10, NULL) == 0);
  assert(st.contact_kind == 2u && st.rec_first == 9 && st.rec_second == 10);
  assert(st.point[0] == 0 && st.point[1] == 0 && st.point[2] == 0);
  assert(fifa96_ref_contact_register(NULL, 1, 0, 0, point) == -FIFA96_ERR_INVALID);
}

/* E3 head gates: phase 2, a non-NULL fouler and settings 0xA != 0; anything
 * else returns 0 before storing (0x8A44C/0x8A45E/0x8A47A). */
static void test_foul_decide_gates(void) {
  struct fifa96_referee_state st = fresh_state();
  struct fifa96_match_config cfg = fresh_cfg();
  struct fifa96_ref_record fouler = record(5, 0, 3);
  struct fifa96_ref_record victim = record(6, 1, 4);
  struct fifa96_ref_decision_out out;
  const int32_t point[3] = {1, 2, 3};
  st.phase = 1;
  cfg.field_4c306 = 2;
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0, &out) == 0);
  assert(st.contact_kind == 0u && st.rec_first == 0);
  st.phase = 2;
  cfg.field_4c306 = 0;
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0, &out) == 0);
  assert(st.contact_kind == 0u);
  assert(fifa96_ref_foul_decide(&st, &cfg, NULL, &victim, point, 0, &out) == 0);
  assert(fifa96_ref_foul_decide(NULL, &cfg, &fouler, &victim, point, 0, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_ref_foul_decide(&st, NULL, &fouler, &victim, point, 0, &out) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0, NULL) ==
         -FIFA96_ERR_INVALID);
}

/* E3 soft path (settings 0xA == 1): the foul kind stays 0, the point is
 * stored with y forced 0 and the FUN_0007D3E4 clamp, and the outcome is the
 * situation-9 restart on the fouled side (rec_first side ^ 1). */
static void test_foul_decide_soft(void) {
  struct fifa96_referee_state st = fresh_state();
  struct fifa96_match_config cfg = fresh_cfg();
  struct fifa96_ref_record fouler = record(5, 0, 3);
  struct fifa96_ref_record victim = record(6, 1, 4);
  struct fifa96_ref_decision_out out;
  const int32_t point[3] = {0x800, 0x777, 0x2000};
  cfg.field_4c306 = 1;
  st.contact_kind = 1;                           /* the row-0x0C re-call kind */
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0x3F, &out) == 1);
  assert(out.sequence == FIFA96_REF_SEQ_NONE);
  assert(out.foul_kind == 0u && out.restart == 1u && out.restart_side == 1u);
  assert(st.foul_kind == 0u);
  assert(st.contact_kind == 1u);                 /* the row-0x0C re-call kind */
  assert(st.rec_first == 5 && st.rec_first_side == 0u && st.rec_first_player == 3u);
  assert(st.rec_second == 6);
  assert(st.point[0] == 0x720 && st.point[1] == 0 && st.point[2] == 0xB10);
  assert(st.log_count == 0u);                    /* kind 0 is not logged */
  /* the fouled side mirrors the fouler side */
  fouler = record(5, 1, 3);
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0, &out) == 1);
  assert(out.restart_side == 0u);
}

/* E4 severity, byte-exact (0x8A563..0x8A639): rng & 0x3F < 0x12 -> kind 1
 * when the accumulator is 0, kind 2 when it is not and the team count > 8;
 * [0x12,0x16) with contact kind 2 and count > 8 -> kind 3; the active and
 * duel_ok preconditions gate the whole block; kind != 0 appends the log ring
 * and requests act 3. */
static void test_foul_decide_severity(void) {
  struct fifa96_referee_state st = fresh_state();
  struct fifa96_match_config cfg = fresh_cfg();
  struct fifa96_ref_record fouler = record(5, 0, 3);
  struct fifa96_ref_record victim = record(6, 1, 4);
  struct fifa96_ref_decision_out out;
  const int32_t point[3] = {0x100, 0, 0};
  cfg.field_4c306 = 2;

  st.contact_kind = 1;                           /* the staged entry kind */
  st.team_count[0] = 0;                          /* acc == 0 suffices */
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0x05, &out) == 1);
  assert(out.sequence == FIFA96_REF_SEQ_ACT3 && out.foul_kind == 1u);
  assert(st.foul_kind == 1u && st.log_count == 1u);
  assert(st.log[0].kind == 1u && st.log[0].team == 0u && st.log[0].player == 5);
  assert(st.log[0].minute == st.game_minute);

  /* acc != 0 and a full count -> kind 2 */
  st.severity[0][3] = 0x40;
  st.team_count[0] = 9;
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0x05, &out) == 1);
  assert(out.foul_kind == 2u && st.log_count == 2u && st.log[1].kind == 2u);

  /* acc != 0 and a short count -> kind stays 0 (soft foul) */
  st.team_count[0] = 8;
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0x05, &out) == 1);
  assert(out.foul_kind == 0u && out.restart == 1u && st.log_count == 2u);

  /* the kind-3 band needs contact kind 2 and a full count */
  st.team_count[0] = 9;
  st.contact_kind = 2;                           /* staged entry kind 2 */
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0x12, &out) == 1);
  assert(out.foul_kind == 3u && st.log_count == 3u && st.log[2].kind == 3u);
  /* contact kind 1 never takes the kind-3 band */
  st.contact_kind = 1;
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0x12, &out) == 1);
  assert(out.foul_kind == 0u);
  /* 0x16 and above leaves kind 0 */
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0x16, &out) == 1);
  assert(out.foul_kind == 0u);

  /* the active / duel_ok preconditions gate the severity block (0x8A556 /
   * 0x8A550) */
  fouler.active = 0;
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0x05, &out) == 1);
  assert(out.foul_kind == 0u);
  fouler.active = 1;
  fouler.duel_ok = 0;
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0x05, &out) == 1);
  assert(out.foul_kind == 0u);
}

/* The log ring shifts down at the 10-entry wrap (0x8A654..0x8A6A7) and the
 * team byte is the FUN_000741B4 side-swap fold. */
static void test_foul_log_wrap(void) {
  struct fifa96_referee_state st = fresh_state();
  struct fifa96_match_config cfg = fresh_cfg();
  struct fifa96_ref_record fouler = record(5, 0, 3);
  struct fifa96_ref_record victim = record(6, 1, 4);
  struct fifa96_ref_decision_out out;
  const int32_t point[3] = {0, 0, 0};
  cfg.field_4c306 = 2;
  st.side_swap = 1;
  for (int i = 0; i < 10; i++) {
    st.game_minute = (uint16_t)(0x10 + i);
    st.team_count[0] = 9;
    st.severity[1][3] = (i & 1) ? 1u : 0u;       /* side ^ side_swap 1 */
    st.contact_kind = 1;
    (void)fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0x05, &out);
  }
  assert(st.log_count == 10u);
  for (int i = 0; i < 10; i++) {
    assert(st.log[i].minute == (uint16_t)(0x10 + i));
    assert(st.log[i].kind == ((i & 1) ? 2u : 1u));
  }
  st.game_minute = 0x77;
  st.team_count[0] = 0;
  st.severity[1][3] = 0;
  assert(fifa96_ref_foul_decide(&st, &cfg, &fouler, &victim, point, 0x05, &out) == 1);
  assert(st.log_count == 10u);                   /* saturated */
  assert(st.log[0].kind == 2u);                  /* entry 0 := old entry 1 */
  assert(st.log[0].minute == 0x11u);
  assert(st.log[8].kind == 2u);                  /* entry 8 := old entry 9 */
  assert(st.log[8].minute == 0x19u);
  assert(st.log[9].kind == 1u && st.log[9].minute == 0x77u);
  assert(st.log[9].team == 1u);                  /* side 0 ^ side_swap 1 */
}

/* E5: the phase-0x19 7-stage machine. Stage 0 = whistle + foul counter
 * (side ^ side_swap index) + phase 0xF on the fouled side + install 0x16;
 * stage 2 speech by kind; stage 4 = severity sum -> install 0x18 when
 * (sum & 0x7F) >= 2, else stage += 2 (skipping stage 5); stage 5 = team-count
 * decrement only while the fouler is held; stage 6 = the referee-object gate
 * then situation 0xA on the fouled side. */
static void test_foul_sequence_kind1(void) {
  struct fifa96_referee_state st = fresh_state();
  struct fifa96_ref_sequence_out out;
  st.sequence = FIFA96_REF_SEQ_ACT3;
  st.stage = 0;
  st.delta = 2;
  st.rec_first = 5;
  st.rec_first_side = 0;
  st.rec_first_player = 3;
  st.contact_kind = 1;
  st.foul_kind = 1;
  st.team_count[0] = 11;
  st.team_count[1] = 11;

  memset(&out, 0, sizeof out);
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);
  assert(out.whistle == 1u && out.foul_counter == 1u);
  assert(out.phase_write == 0x0Fu && out.phase_side == 1u);
  assert(out.install_action == 0x16u);
  assert(st.fouls_by_side[0] == 1u && st.fouls_by_side[1] == 0u);
  assert(st.stage == 1u && st.timer == 0u);

  memset(&out, 0, sizeof out);
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);   /* stage 1 gate */
  assert(st.stage == 2u && out.phase_write == 0xFFu);

  memset(&out, 0, sizeof out);
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);   /* stage 2 speech */
  assert(out.speech_code == 0x0Du && st.stage == 3u);

  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);   /* stage 3 gate */
  assert(st.stage == 4u);

  memset(&out, 0, sizeof out);
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);   /* stage 4 sum */
  assert(out.install_action == 0u);              /* sum 1 < 2 */
  assert(st.severity[0][3] == 1u);
  assert(st.stage == 6u);                        /* stage 5 skipped */

  memset(&out, 0, sizeof out);
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);   /* stage 6 */
  assert(out.situation == 0x0Au && out.situation_side == 1u);
  assert(out.done == 1u);
  assert(st.sequence == FIFA96_REF_SEQ_NONE && st.stage == 0u);
  assert(st.team_count[0] == 11u);               /* never decremented */
}

/* The kind-2 path reaches stage 5: a held fouler decrements the team count, an
 * unheld one stalls the machine with the count untouched (the no-cards
 * negative: no booking state exists, only the held gate, E10/leg 6). */
static void test_foul_sequence_kind2_held(void) {
  struct fifa96_referee_state st = fresh_state();
  struct fifa96_ref_sequence_out out;
  st.sequence = FIFA96_REF_SEQ_ACT3;
  st.stage = 4;
  st.rec_first_side = 0;
  st.rec_first_player = 2;
  st.foul_kind = 2;
  st.severity[0][2] = 0;                         /* sum 2 -> install 0x18 */
  st.team_count[0] = 11;
  st.team_count[1] = 11;
  st.rec_first_held = 0;

  memset(&out, 0, sizeof out);
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);
  assert(out.install_action == 0x18u && st.stage == 5u);
  memset(&out, 0, sizeof out);
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);
  assert(out.team_count_dec == 0u && st.stage == 5u);      /* stall */
  assert(st.team_count[0] == 11u);
  st.rec_first_held = 1;
  memset(&out, 0, sizeof out);
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);
  assert(out.team_count_dec == 1u && st.stage == 6u);
  assert(st.team_count[0] == 10u && st.team_count[1] == 11u);
  /* stage 6 waits while the referee object code is 0x48 (0x8A39F `JZ`) */
  st.referee_object_code = 0x48;
  memset(&out, 0, sizeof out);
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);
  assert(out.situation == 0xFFu && out.done == 0u && st.stage == 6u);
  st.referee_object_code = 0x10;
  memset(&out, 0, sizeof out);
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);
  assert(out.situation == 0x0Au && out.done == 1u);
}

/* Stage 2 speech select (0x8A191..0x8A23C): kind 1 -> 0xD, kind 2 -> 0xE,
 * kind 3 -> 0x13 when both team counts are full (0xB) else 0xF. */
static void test_foul_sequence_speech(void) {
  struct fifa96_referee_state st = fresh_state();
  struct fifa96_ref_sequence_out out;
  st.sequence = FIFA96_REF_SEQ_ACT3;
  st.rec_first_side = 1;
  st.foul_kind = 2;
  st.stage = 2;
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);
  assert(out.speech_code == 0x0Eu);
  st.stage = 2;
  st.foul_kind = 3;
  st.team_count[0] = 0xB;
  st.team_count[1] = 0xB;
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);
  assert(out.speech_code == 0x13u);
  st.stage = 2;
  st.team_count[1] = 0xA;
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);
  assert(out.speech_code == 0x0Fu);
  /* the foul counter index folds through side_swap and the machine bound */
  st.side_swap = 1;
  st.stage = 0;
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);
  assert(st.fouls_by_side[0] == 1u);             /* side 1 ^ swap 1 */
  st.stage = 7;
  memset(&out, 0, sizeof out);
  assert(fifa96_ref_foul_sequence_step(&st, &out) == 0);
  assert(out.whistle == 0u && out.done == 0u);
  assert(fifa96_ref_foul_sequence_step(NULL, &out) == -FIFA96_ERR_INVALID);
  assert(fifa96_ref_foul_sequence_step(&st, NULL) == -FIFA96_ERR_INVALID);
}

/* E7 derived check, side 0: the pass case and every gate that can block it. */
static void test_offside_check_side0(void) {
  struct fifa96_referee_state st = fresh_state();
  struct fifa96_match_config cfg = fresh_cfg();
  struct fifa96_ref_receiver rec;
  struct fifa96_ref_metric metric;
  uint8_t offside = 0xAA;
  memset(&rec, 0, sizeof rec);
  metric.tol2 = 0;
  metric.side_gate = 1;
  cfg.field_4c2f2 = 1;
  rec.z = 0x3C0;
  rec.side = 0;
  rec.own_nearest_valid = 1;
  rec.own_nearest_is_receiver = 0;
  rec.own_nearest_z = 0x3C0;
  rec.own_distance = 0x3C0;
  rec.eligible = 1;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 1u);

  st.phase = 1;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  st.phase = 2;
  st.offside_suppress = 1;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  st.offside_suppress = 0;
  cfg.field_4c2f2 = 0;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  cfg.field_4c2f2 = 1;
  /* the camera gate: |ref| > 0x990 is only skipped while the mirror is 0 */
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0x991, 0, 0, &offside) == 0);
  assert(offside == 1u);
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0x991, 1, 0, &offside) == 0);
  assert(offside == 0u);
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, -0x990, 1, 0, &offside) == 0);
  assert(offside == 1u);
  /* the metric side gate */
  metric.side_gate = 0;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  metric.side_gate = 1;
  /* own-nearest identity / depth / valid */
  rec.own_nearest_is_receiver = 1;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  rec.own_nearest_is_receiver = 0;
  rec.own_nearest_z = 0x3B0;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  rec.own_nearest_z = 0x3C0;
  rec.own_nearest_valid = 0;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  rec.own_nearest_valid = 1;
  /* the receiver-vs-defender direction gate (receiver beyond the defender) */
  rec.z = 0x3C1;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  rec.z = 0x3C0;
  /* the 6-bit tolerance: own_nearest_z - tol must stay >= defender_z, with the
   * receiver inside the metric x2 window (tol2 > 0) */
  metric.tol2 = 0x100;
  rec.z = 0x200;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3B0, 0, 0, 0x3F, &offside) == 0);
  assert(offside == 0u);                         /* 0x3C0-0x3F == 0x381 */
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3B0, 0, 0, 0x10, &offside) == 0);
  assert(offside == 1u);                         /* 0x3C0-0x10 == 0x3B0 */
  metric.tol2 = 0;
  rec.z = 0x3C0;
  /* the metric tol2 x2 distance term */
  metric.tol2 = -0x100;                          /* 2*-0x100 + 0x3C0 < 0x3C0 */
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  metric.tol2 = 0;
  /* the own-distance and record-state gates */
  rec.own_distance = 0x3C1;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  rec.own_distance = 0x3C0;
  rec.eligible = 0;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  assert(fifa96_ref_offside_check(NULL, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, &offside) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, NULL, 0x3C0, 0, 0, 0, &offside) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, 0x3C0, 0, 0, 0, NULL) ==
         -FIFA96_ERR_INVALID);
}

/* E7 side 1 mirrors the direction, depth, tolerance and distance signs. */
static void test_offside_check_side1(void) {
  struct fifa96_referee_state st = fresh_state();
  struct fifa96_match_config cfg = fresh_cfg();
  struct fifa96_ref_receiver rec;
  struct fifa96_ref_metric metric;
  uint8_t offside = 0;
  memset(&rec, 0, sizeof rec);
  cfg.field_4c2f2 = 1;
  metric.tol2 = 0;
  metric.side_gate = -1;
  rec.z = -0x3C0;
  rec.side = 1;
  rec.own_nearest_valid = 1;
  rec.own_nearest_z = -0x3C0;
  rec.own_distance = 0x3C0;
  rec.eligible = 1;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, -0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 1u);
  /* a non-negative side gate blocks side 1 */
  metric.side_gate = 0;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, -0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  metric.side_gate = -1;
  /* depth: own_nearest_z must be < -0x3B0 */
  rec.own_nearest_z = -0x3B0;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, -0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  rec.own_nearest_z = -0x3C0;
  /* receiver must not be beyond the defender on the -z side */
  rec.z = -0x3C1;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, -0x3C0, 0, 0, 0, &offside) == 0);
  assert(offside == 0u);
  rec.z = -0x3C0;
  /* own_nearest_z + tol must stay <= defender_z */
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, -0x3B0, 0, 0, 0x3F, &offside) == 0);
  assert(offside == 0u);
  metric.tol2 = 0x100;
  rec.z = -0x200;
  assert(fifa96_ref_offside_check(&st, &cfg, &rec, &metric, -0x3B0, 0, 0, 0x10, &offside) == 0);
  assert(offside == 1u);                         /* -0x3C0+0x10 == -0x3B0 */
}

/* E3 kind-3 arm: settings 0x10 gate, the kind-3 stores with y preserved and
 * the clamp, and the speech 0x15 + act-6 requests. */
static void test_offside_event(void) {
  struct fifa96_referee_state st = fresh_state();
  struct fifa96_match_config cfg = fresh_cfg();
  struct fifa96_ref_record rec = record(21, 1, 7);
  struct fifa96_ref_event_out out;
  const int32_t point[3] = {0x800, 0x55, 0x2000};
  cfg.field_4c2f2 = 0;
  assert(fifa96_ref_offside_event(&st, &cfg, &rec, point, &out) == 0);
  assert(st.contact_kind == 0u && out.sequence == FIFA96_REF_SEQ_NONE);
  cfg.field_4c2f2 = 1;
  assert(fifa96_ref_offside_event(&st, &cfg, &rec, point, &out) == 1);
  assert(st.contact_kind == 3u);
  assert(st.rec_first == 21 && st.rec_first_side == 1u && st.rec_first_player == 7u);
  assert(st.rec_second == 0);
  assert(st.point[0] == 0x720 && st.point[1] == 0x55 && st.point[2] == 0xB10);
  assert(st.sequence == FIFA96_REF_SEQ_ACT6 && st.stage == 0u);
  assert(out.speech_code == 0x15u && out.sequence == FIFA96_REF_SEQ_ACT6);
  assert(out.rec_id == 21);
  assert(fifa96_ref_offside_event(NULL, &cfg, &rec, point, &out) == -FIFA96_ERR_INVALID);
  assert(fifa96_ref_offside_event(&st, &cfg, &rec, point, NULL) == -FIFA96_ERR_INVALID);
}

/* E6: the phase-0x1C machine. Stage 0 = whistle + phase 0xA on side 0;
 * stage 2 = situation 9 on the opponent of the offside record. */
static void test_offside_sequence(void) {
  struct fifa96_referee_state st = fresh_state();
  struct fifa96_ref_sequence_out out;
  st.sequence = FIFA96_REF_SEQ_ACT6;
  st.stage = 0;
  st.rec_first_side = 1;
  st.delta = 2;
  memset(&out, 0, sizeof out);
  assert(fifa96_ref_offside_sequence_step(&st, &out) == 0);
  assert(out.whistle == 1u && out.phase_write == 0x0Au && out.phase_side == 0u);
  assert(st.stage == 1u && st.timer == 0u);
  memset(&out, 0, sizeof out);
  assert(fifa96_ref_offside_sequence_step(&st, &out) == 0);   /* stage 1 gate */
  assert(st.stage == 2u);
  memset(&out, 0, sizeof out);
  assert(fifa96_ref_offside_sequence_step(&st, &out) == 0);   /* stage 2 */
  assert(out.situation == 9u && out.situation_side == 0u);
  assert(out.done == 1u);
  assert(st.sequence == FIFA96_REF_SEQ_NONE && st.stage == 0u);
  /* out-of-range stages are a no-op */
  st.stage = 3;
  memset(&out, 0, sizeof out);
  assert(fifa96_ref_offside_sequence_step(&st, &out) == 0);
  assert(out.whistle == 0u && out.done == 0u);
  assert(fifa96_ref_offside_sequence_step(NULL, &out) == -FIFA96_ERR_INVALID);
  assert(fifa96_ref_offside_sequence_step(&st, NULL) == -FIFA96_ERR_INVALID);
}

int main(void) {
  test_contact_register();
  test_foul_decide_gates();
  test_foul_decide_soft();
  test_foul_decide_severity();
  test_foul_log_wrap();
  test_foul_sequence_kind1();
  test_foul_sequence_kind2_held();
  test_foul_sequence_speech();
  test_offside_check_side0();
  test_offside_check_side1();
  test_offside_event();
  test_offside_sequence();
  puts("test_referee: ok");
  return 0;
}
