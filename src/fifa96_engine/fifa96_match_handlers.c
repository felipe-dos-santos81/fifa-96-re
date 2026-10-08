/* src/fifa96_engine/fifa96_match_handlers.c — FU-137 dispatch tables and seam.
 *
 * The per-row evidence strings are the compact form of the FU-137 §6.1
 * (action) and §6.2 (phase) classification tables; FU-136 §2/§3 hold the full
 * evidence cells (docs, tested C symbols, estimated port group). FU-138 wires
 * the first cluster-A row: 00 is ported (`fifa96_match_action_00` binds the
 * tested `fifa96_action_move_step`/`_target` bodies to `mr->record`, FU-138
 * §4). FU-140 (cluster C) wires the keeper's fully linear claim/throw row
 * 1E (`fifa96_match_action_1E` binds the tested `fifa96_keeper_claim_place`
 * body to `mr->record`; FU-140 §4). FU-141 (cluster D/E) lands the entity/ball
 * pool: the frame body walks the pool records, stages each into `mr->record`,
 * dispatches its action code here, and drains the requests (install/ran,
 * helper_request, controlled, place_valid) back into the pool — so row 00's and
 * row 1E's record-visible bodies are now consumed end to end (FU-141 §4). Every
 * other row is either `not ported` (fn NULL, -FIFA96_ERR_UNSUPPORTED with its
 * port group) or the six remaining keeper rows 19/1A/1B/1C/1D/1F (fn NULL,
 * tested pure parts but unported arms — FU-140 OL-33..OL-37); the
 * 0x2B row is a dead entry (the shared row-29 epilogue RET at 0x87738,
 * FU-142 §1.1; FU-142f census, OL-15) and rows 27 (Task 4) / 2C (Task 5) /
 * 29 (Task 6) have ported bodies but no installer invocation anywhere
 * (FU-142f Appendix I census of all 77 FUN_0007D9A4 call sites; OL-48).
 * Phase 0x16 is the native zero/INT3 slot and
 * returns -FIFA96_ERR_NOT_FOUND. Later G2 clusters replace a NULL fn with their derived
 * body and update the evidence string; they must not change the code/class of a
 * row without an FU-doc errata. FU-139 (cluster B) derives the ball
 * staging/resolver/possession/kick helpers (tested in
 * test_ball_pairing/test_action_handlers) and leaves action rows
 * 05/06/07/0F/18/21/23 unwired with their arms (FU-139 OL-29..OL-32; the pool
 * they also waited on is now FU-141); their evidence strings cite FU-139.
 * FU-139 §8 (Task 10) adds the ball staging tail
 * (`fifa96_ball_pair_stage_tail`, native 0x7A8D1..0x7AA2F) and the bounded
 * row-05 carrier machine (`fifa96_action_carrier_arm`, native
 * 0x7F194..0x7F665) with tests; row 05 stays unwired because its stage-0
 * target algebra (0x7F3A1..0x7F57B) and the `FUN_0007F7E0` fallback are
 * unported (OL-63), so the binding gate keeps `fn == NULL` and the evidence
 * names the leg. FU-142b (cluster G) wires row 26: the FU-142a arm 0x8D74D, the ported
 * `0x866F4..0x8681C` body (`fifa96_arm_26_step`, FU-142 Appendix C) and the
 * FU-141 pool binding are all bounded, so the row flips to `ported` and its
 * stage92/timer7b/lane results are repacked into the pool record by the frame
 * body. FU-142b Task 4 ports row 27's body (`fifa96_arm_27_step` + the
 * 0x79C50/0x6E598 helpers) but keeps the row unwired: no static entry exists
 * (the only reference to 0x86820 is the action-table slot itself, FU-142b
 * Appendix D.1), so the row's evidence names the FU-142f/OL-48 entry verdict.
 * FU-142d (Task 7) ports and wires row 28: the FU-142a arm 0x8D7CF installs
 * code 0x28, the `0x870E8..0x874E3` 4-arm machine plus its `0x87014` helper
 * are ported (`fifa96_arm_28_step`, FU-142 Appendix G) and the pool binding
 * (scratch gates, team flag830, resolved chosen831, globals) is bounded, so
 * the row flips to `ported`. FU-142e (Task 8) ports and wires row 2A: the arm
 * `0x8D807` installs code 0x2A, the `0x86A34..0x87010` 12-arm machine
 * (`fifa96_arm_2a_step`, FU-142 Appendix H) and the pool binding (staged
 * distance, team flag830, the two process globals) are bounded, so the row
 * flips to `ported`. FU-142f (Task 9) exhaustively classifies all 77
 * `FUN_0007D9A4` call sites: no call passes 0x27/0x29/0x2C and no stored
 * installer pointer exists, so the remaining cluster-G rows 27/29/2C stay
 * unwired (bodies ported in Tasks 4/5/6, entries OL-48 per FU-142 Appendix I)
 * and row 2B is recorded as a dead entry. FU-139 §9 (Task 11) closes OL-28
 * and OL-31: `fifa96_match_action_07`/`_0F` bind `fifa96_action_kick_machine`
 * (native `0x814B0..0x81737` / `0x82AD0..0x82DCF`) and the full
 * `FUN_0007B9C4` kick path (`fifa96_ball_kick_target`) to the record and the
 * pool ball block, so rows 07/0F flip to `ported`; the unmodeled record bytes
 * and external tables are the OL-65/OL-66 legs. FU-139 §10 (Task 12) closes
 * OL-27 and OL-32: `fifa96_match_action_18`/`_21`/`_23` bind the ported
 * row-18/21/23 machines (`fifa96_action_duel_step` + the
 * `fifa96_action_duel_bind`/`_search`/`_swap` resolution arms,
 * `fifa96_action_receive_step`, `fifa96_action_tackle_step`/`_attempt`) to
 * `mr->record`/the entity pool, so rows 18/21/23 flip to `ported`; the
 * residual record/presentation inputs are the OL-68 leg and the event ring
 * sinks are ported but not yet driven (OL-67). FU-139 §11 (Task 13) closes
 * OL-30: `fifa96_match_action_06` binds `fifa96_action_pursuit_step`
 * (native `0x801B4..0x809EF`: the `0x8DCD4`/`0x8DD70` camera metric, the
 * `0x114E04` folds, the RNG install gates 8/9 and the carrier-gate install 4,
 * the `0x8DE8C`/`0x79CCC` mate selections) to `mr->record` and the pool, so
 * row 06 flips to `ported`; the unmodeled record bytes/lead/swap are OL-69.
 * The plan's OL-30 span end `0x81067` covers the row-09 handler
 * (`0x80A00..0x81065`, the action-table slot `0x1106E0[9]`), which stays
 * unwired (OL-9). */
#include <stddef.h>
#include <string.h>

#include "fifa96_engine/fifa96_match_handlers.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_loader/fifa96_action_handlers.h"
#include "fifa96_loader/fifa96_arm_bodies.h"
#include "fifa96_loader/fifa96_keeper.h"
#include "fifa96_loader/fifa96_outfield.h"

/* FU-138 §4: action 00 — the FU-76 §3.1 generic outfield step. The derived
 * record core sets `[rec+0x9E]=1` (native `0x7DB1C`), runs
 * `fifa96_action_move_step` (timer89 decay, control-slot move gate, phase-2
 * install request `3`/`0x19`) and, when the slot moves, writes the FU-76 §4
 * clamped step target through `fifa96_action_move_target`. The native tail's
 * install goes through `FUN_0007D9A4` (FU-137 §2); the handler records the
 * request in `mr->record.install` and the C8/FU-141 pool update consumes it
 * with the derived installer (and clears `ran`). */
static int fifa96_match_action_00(struct fifa96_match_run *mr) {
  fifa96_action_move_state state;
  fifa96_action_move_out out;
  fifa96_action_vec3 target;
  int rc;
  mr->record.ran = 1;
  state.timer89 = mr->record.timer89;
  state.timer81 = mr->record.timer81;
  state.delta = mr->record.delta;
  state.phase = mr->state.phase;
  state.active = mr->record.active;
  state.has_slot = mr->record.has_slot;
  state.dir_x = mr->record.dir_x;
  state.dir_z = mr->record.dir_z;
  rc = fifa96_action_move_step(&state, &out);
  if (rc != FIFA96_OK) return rc;
  mr->record.timer89 = state.timer89;
  if (out.move != 0) {
    rc = fifa96_action_move_target(mr->record.pos_x, mr->record.pos_z, mr->record.dir_x,
                                   mr->record.dir_z, &target);
    if (rc != FIFA96_OK) return rc;
    mr->record.target_x = target.x;
    mr->record.target_z = target.z;
  }
  mr->record.install = out.install != 0 ? out.code : 0;
  return FIFA96_OK;
}

/* FU-140 §4: action 1E — the keeper claim/throw placement (FU-79 §7 body
 * `0x7550C..0x755D3`, the family's only fully linear body). The derived core
 * requests the `FUN_0007876C` slot merge while `stage < 6` and the record has
 * no control slot, then, while `stage < 3` and the record does not hold the
 * ball (`+0x9B == 0`), computes the placement triple (record position plus the
 * caller-supplied per-type offset bytes `0x10F334/0x10F33C[type8] << 4`, y +
 * 0x38), sets the possession flag and binds the record as the actor
 * (`[0x157A83] = rec`). The native camera-place call
 * `FUN_000700F4(place_x, place_y, place_z, 1)` (FU-141 erratum: four by-value
 * args, no record) is the derived `place_*` sink; the C8/FU-141 pool update
 * consumes the `helper_request` (slot merge `FUN_0007876C`), the `controlled`
 * actor binding and the placement triple (`place_valid`), leaving the
 * merge/camera call bodies and the per-type offset table as OL-37. The body
 * does not write `+0x9E`, so `ran` is untouched. */
static int fifa96_match_action_1E(struct fifa96_match_run *mr) {
  fifa96_keeper_point pos;
  fifa96_keeper_point place;
  uint8_t helper_request = 0;
  uint8_t claimed = 0;
  int rc;
  pos.x = mr->record.pos_x;
  pos.y = mr->record.pos_y;
  pos.z = mr->record.pos_z;
  rc = fifa96_keeper_claim_place(&pos, mr->record.place_offset_x, mr->record.place_offset_z,
                                 mr->record.stage, mr->record.has_ball, mr->record.has_slot,
                                 &place, &helper_request, &claimed);
  if (rc != FIFA96_OK) return rc;
  mr->record.helper_request = helper_request;
  mr->record.place_valid = claimed;
  if (claimed != 0) {
    mr->record.place_x = place.x;
    mr->record.place_y = place.y;
    mr->record.place_z = place.z;
    mr->record.has_ball = 1;
    mr->record.controlled = 1;
  }
  return FIFA96_OK;
}

/* FU-142b (FU-142 Appendix C): action 0x26 — the cluster-G placement machine
 * `0x866F4..0x8681C`. The derived step runs the FU-142a-installed record's
 * stage latch: stage 0 waits out `15 * rec[+4][+0xE]` on the [0x157A64]-fed
 * timer89, stage 1 aims the target at (0x780, ±6*(active>>1)) and, when the
 * 0x8DCD4 lane (|target.z - pos.z|) is inside 0x20, retargets (0xCC0, 0) and
 * advances to stage 2; stage > 1 only refreshes timer89/timer7b. `timer7b` is
 * the 0x10F394[rec[+4][+0xD]] >> 1 read. The native body requests no install,
 * so `install`/`ran` stay as staged; `player_d`/`player_e` stand in for the
 * native rec[+4] descriptor bytes (Appendix C.5 open leg). */
static int fifa96_match_action_26(struct fifa96_match_run *mr) {
  struct fifa96_arm_record rec;
  int rc;
  memset(&rec, 0, sizeof rec);
  rec.pos.x = mr->record.pos_x;
  rec.pos.y = mr->record.pos_y;
  rec.pos.z = mr->record.pos_z;
  rec.target.x = mr->record.target_x;
  rec.target.z = mr->record.target_z;
  rec.timer89 = mr->record.timer89;
  rec.timer7b = mr->record.timer7b;
  rec.delta = mr->record.delta;
  rec.active = mr->record.active;
  rec.stage92 = mr->record.stage92;
  rec.lane = mr->record.lane;
  rec.player_d = mr->record.player_d;
  rec.player_e = mr->record.player_e;
  rc = fifa96_arm_26_step(&rec);
  if (rc != FIFA96_OK) return rc;
  mr->record.target_x = rec.target.x;
  mr->record.target_z = rec.target.z;
  mr->record.timer89 = rec.timer89;
  mr->record.timer7b = rec.timer7b;
  mr->record.stage92 = rec.stage92;
  mr->record.lane = rec.lane;
  return FIFA96_OK;
}

/* FU-142d (FU-142 Appendix G): action 0x28 — the 4-arm stage machine
 * `0x870E8..0x874E3`. The derived step runs the prologue (`0x8DCD4` lane
 * triple + `0x79C50` face), then the stage92 jump: arm 0 builds the set-piece
 * target from the `[0x10F364/368]` globals + the `0x114E04` angle fold and
 * falls into arm 1; arm 1 waits on the `+0xA2` gate and, on fire, either syncs
 * `target = pos` (team `flag830` clear) or runs the `0x87014` gate setup and
 * latches to stage 2; arm 2 runs the `+0xAA`/`+0xAE` approach with the
 * `0x7D8B0`/`0x7D8C0` animation-id tables and the `[team+0x831]` chosen-record
 * copy. The record staging carries `vel_x/vel_z`, `type`, `side`, `flag830`,
 * `chosen_*`, the six scratch cells and the five process globals; the chosen
 * position is resolved by `match_run_dispatch_entity` from the pool's
 * `chosen831` (OL-58), the globals default to 0 (OL-56) and the `0x36200`/
 * `[0x157AA3]`/`anim_sel` stand-ins stay OL-51/OL-57. */
static int fifa96_match_action_28(struct fifa96_match_run *mr) {
  struct fifa96_arm_record rec;
  int rc;
  memset(&rec, 0, sizeof rec);
  rec.pos.x = mr->record.pos_x;
  rec.pos.y = mr->record.pos_y;
  rec.pos.z = mr->record.pos_z;
  rec.target.x = mr->record.target_x;
  rec.target.y = mr->record.target_y;
  rec.target.z = mr->record.target_z;
  rec.timer89 = mr->record.timer89;
  rec.delta = mr->record.delta;
  rec.active = mr->record.active;
  rec.stage92 = mr->record.stage92;
  rec.type = mr->record.type;
  rec.side = mr->record.side;
  rec.flag830 = mr->record.flag830;
  rec.scratch_a2 = mr->record.scratch_a2;
  rec.scratch_a6 = mr->record.scratch_a6;
  rec.scratch_aa = mr->record.scratch_aa;
  rec.scratch_ae = mr->record.scratch_ae;
  rec.scratch_a0 = mr->record.scratch_a0;
  rec.scratch_a1 = mr->record.scratch_a1;
  rec.global_10f358 = mr->record.global_10f358;
  rec.global_10f35c = mr->record.global_10f35c;
  rec.global_10f364 = mr->record.global_10f364;
  rec.global_10f368 = mr->record.global_10f368;
  rec.global_157ac2 = mr->record.global_157ac2;
  rec.chosen_pos.x = mr->record.chosen_x;
  rec.chosen_pos.y = mr->record.chosen_y;
  rec.chosen_pos.z = mr->record.chosen_z;
  rec.chosen_ok = mr->record.chosen_ok;
  rec.rng = &mr->rng;
  rc = fifa96_arm_28_step(&rec, mr->record.stage92);
  if (rc != FIFA96_OK) return rc;
  mr->record.target_x = rec.target.x;
  mr->record.target_y = rec.target.y;
  mr->record.target_z = rec.target.z;
  mr->record.timer89 = rec.timer89;
  mr->record.stage92 = rec.stage92;
  mr->record.type = rec.type;
  mr->record.lane = rec.lane;
  mr->record.vel_x = rec.vel_x;
  mr->record.vel_z = rec.vel_z;
  mr->record.scratch_a2 = rec.scratch_a2;
  mr->record.scratch_a6 = rec.scratch_a6;
  mr->record.scratch_aa = rec.scratch_aa;
  mr->record.scratch_ae = rec.scratch_ae;
  mr->record.scratch_a0 = rec.scratch_a0;
  mr->record.scratch_a1 = rec.scratch_a1;
  return FIFA96_OK;
}

/* FU-142e (FU-142 Appendix H): action 0x2A — the 12-arm stage machine
 * `0x86A34..0x87010`. The derived step runs the prologue (timer89 += delta
 * and, for a signed stage byte > 2, timer7b = 4 + the 0x36200 stub), then the
 * jump table: arm 0 clears the target/globals/team flag, arms 1/2 gate on the
 * staged `distance` (+0x65), arms 3..8 are the corner/return target algebra
 * (arm 8 draws four RNG words into the target), arm 9 syncs target = pos and
 * sets `[0x10F358]`, arm 10 runs the anim/face/timer sequence and sets
 * `[0x10F35C]`, and 11..255 take the epilogue. The record staging carries
 * `distance`, `type`, `flag830`, the velocity pair and the two globals; the
 * frame repack (match_run.c) lands `flag830` on the pool team and the globals
 * on the run. The `0x513EC` camera stop and the `[0x157AA3]` store stay
 * derived no-ops (Appendix H open legs); the `0x36200` call value (native
 * EAX=2) stays the OL-51 no-op. */
static int fifa96_match_action_2A(struct fifa96_match_run *mr) {
  struct fifa96_arm_record rec;
  int rc;
  memset(&rec, 0, sizeof rec);
  rec.pos.x = mr->record.pos_x;
  rec.pos.y = mr->record.pos_y;
  rec.pos.z = mr->record.pos_z;
  rec.target.x = mr->record.target_x;
  rec.target.y = mr->record.target_y;
  rec.target.z = mr->record.target_z;
  rec.distance = mr->record.distance;
  rec.timer89 = mr->record.timer89;
  rec.timer7b = mr->record.timer7b;
  rec.delta = mr->record.delta;
  rec.type = mr->record.type;
  rec.stage92 = mr->record.stage92;   /* the same byte the handler passes as arm */
  rec.flag830 = mr->record.flag830;
  rec.vel_x = mr->record.vel_x;
  rec.vel_z = mr->record.vel_z;
  rec.global_10f358 = mr->record.global_10f358;
  rec.global_10f35c = mr->record.global_10f35c;
  rec.rng = &mr->rng;
  rc = fifa96_arm_2a_step(&rec, mr->record.stage92);
  if (rc != FIFA96_OK) return rc;
  mr->record.target_x = rec.target.x;
  mr->record.target_y = rec.target.y;
  mr->record.target_z = rec.target.z;
  mr->record.timer89 = rec.timer89;
  mr->record.timer7b = rec.timer7b;
  mr->record.stage92 = rec.stage92;
  mr->record.type = rec.type;
  mr->record.flag830 = rec.flag830;
  mr->record.vel_x = rec.vel_x;
  mr->record.vel_z = rec.vel_z;
  mr->record.global_10f358 = rec.global_10f358;
  mr->record.global_10f35c = rec.global_10f35c;
  return FIFA96_OK;
}

/* ===== FU-139 §9 (Task 11): rows 07/0F kick machines over the pool =====
 *
 * The two machine requests `fifa96_action_kick_machine` returns (the
 * `FUN_0007B9C4` sample: slot released/0x40 for row 07, slot word/0x40/0x20
 * for row 0F) are run here through `fifa96_ball_kick_target` on the pool's
 * ball staging block (`mr->entities.ball.pair`), because the loader libraries
 * cannot link each other in a cycle. The EXE data tables below are first-hand
 * `read_memory` reads: the 0x1104CA resolver mask, the 0x1104BB recompute
 * eligibility table, the four 10-byte event row tables (0x11016E carry,
 * 0x110196 active, 0x11024A idle, 0x1102FE height) and the 0x10F334/0x10F33C
 * per-type kick direction bytes. The 0x158730/0x158734 ball actor/receiver
 * identities are the kick record and the pair receiver; the record's +0x44/
 * +0x99/+0x9D and the roster descriptor bytes are unmodeled (OL-65), and the
 * 0x71B9C predictor / 0x6DBCC corner tables are staged from the camera until
 * their producers are ported (OL-66). */
static const uint8_t match_kick_sector[24] = {
    0x38, 0x70, 0xE0, 0xC1, 0x83, 0x07, 0x0E, 0x1C,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02,
    0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04,
};
static const uint8_t match_kick_recompute[16] = {
    0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0,
};
static const uint8_t match_kick_rows_height[60] = {
    4, 8, 224, 1, 176, 4, 112, 0, 64, 9, 5, 4, 224, 1, 160, 5, 80, 0, 32, 20,
    4, 8, 240, 0, 208, 2, 112, 0, 64, 9, 5, 4, 224, 1, 160, 5, 80, 0, 32, 20,
    4, 8, 224, 1, 176, 4, 112, 0, 64, 9, 5, 4, 224, 1, 160, 5, 80, 0, 32, 20,
};
static const uint8_t match_kick_rows_carry[40] = {
    13, 4, 0, 0, 240, 0, 0, 0, 0, 6,
    13, 4, 0, 0, 240, 0, 0, 0, 0, 10,
    14, 8, 96, 0, 0, 3, 128, 0, 16, 77,
    14, 0, 0, 0, 0, 0, 128, 0, 0, 77,
};
static const uint8_t match_kick_rows_active[180] = {
    1, 8, 208, 2, 96, 9, 160, 0, 16, 7,
    1, 0, 0, 0, 0, 0, 160, 0, 0, 7,
    6, 8, 160, 5, 64, 11, 16, 0, 64, 27,
    5, 4, 224, 1, 160, 5, 80, 0, 32, 20,
    4, 8, 224, 1, 176, 4, 112, 0, 64, 9,
    5, 4, 224, 1, 160, 5, 80, 0, 32, 20,
    2, 8, 192, 0, 192, 3, 0, 0, 32, 6,
    7, 4, 96, 0, 240, 0, 0, 0, 0, 10,
    6, 8, 160, 5, 64, 11, 16, 0, 64, 27,
    5, 4, 224, 1, 160, 5, 80, 0, 32, 20,
    4, 8, 240, 0, 208, 2, 112, 0, 64, 9,
    5, 4, 224, 1, 160, 5, 80, 0, 32, 20,
    3, 8, 160, 5, 64, 11, 16, 0, 64, 8,
    7, 4, 96, 0, 240, 0, 0, 0, 0, 10,
    6, 8, 160, 5, 64, 11, 16, 0, 64, 27,
    5, 4, 224, 1, 160, 5, 80, 0, 32, 20,
    4, 8, 240, 0, 208, 2, 112, 0, 64, 9,
    5, 4, 224, 1, 160, 5, 80, 0, 32, 20,
};
static const uint8_t match_kick_rows_idle[180] = {
    1, 8, 224, 1, 96, 9, 160, 0, 16, 49,
    1, 0, 0, 0, 0, 0, 160, 0, 0, 49,
    1, 8, 224, 1, 96, 9, 160, 0, 16, 49,
    1, 0, 0, 0, 0, 0, 160, 0, 0, 49,
    1, 8, 224, 1, 96, 9, 160, 0, 16, 49,
    1, 0, 0, 0, 0, 0, 160, 0, 0, 49,
    2, 8, 192, 0, 160, 5, 0, 0, 32, 48,
    2, 8, 192, 0, 160, 5, 0, 0, 32, 48,
    2, 8, 192, 0, 160, 5, 0, 0, 32, 48,
    2, 8, 192, 0, 160, 5, 0, 0, 32, 48,
    2, 8, 192, 0, 160, 5, 0, 0, 32, 48,
    2, 8, 192, 0, 160, 5, 0, 0, 32, 48,
    3, 8, 192, 3, 96, 9, 16, 0, 64, 49,
    2, 8, 192, 0, 160, 5, 0, 0, 32, 48,
    3, 8, 192, 3, 96, 9, 16, 0, 64, 49,
    2, 8, 192, 0, 160, 5, 0, 0, 32, 48,
    3, 8, 192, 3, 96, 9, 16, 0, 64, 49,
    2, 8, 192, 0, 160, 5, 0, 0, 32, 48,
};
static const int8_t match_kick_dir_x[32] = {
    0, 1, 1, 1, 0, -1, -1, -1, 1, 1, 0, -1, -1, -1, 0, 1,
    21, 3, 25, 22, 26, 21, 103, 3, 88, 91, 107, 86, 86, 80, 80, 103,
};
static const int8_t match_kick_dir_z[32] = {
    1, 1, 0, -1, -1, -1, 0, 1, 21, 3, 25, 22, 26, 21, 103, 3,
    88, 91, 107, 86, 86, 80, 80, 103, 103, 9, 0, 120, 0, 0, 0, 0,
};

static uint32_t match_kick_team(const struct fifa96_match_run *mr) {
  int32_t id = mr->record.entity_id;
  if (id < 0 || id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  return (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
}

static void match_kick_from_record(struct fifa96_match_run *mr, uint8_t row,
                                   fifa96_action_kick *s) {
  const struct fifa96_match_run_record *r = &mr->record;
  uint32_t team = match_kick_team(mr);
  memset(s, 0, sizeof *s);
  s->row = row;
  s->phase = (uint8_t)mr->state.phase;
  s->stage92 = r->stage92;
  s->active = r->active;
  s->type8 = r->actor_type;
  s->type = r->code;   /* native +0x91: the 0x110680/0x7C990 gate byte (K.5.3) */
  s->target_x = r->target_x;   /* the machine writes only when it owns one */
  s->target_y = r->target_y;
  s->target_z = r->target_z;
  s->has_slot = r->has_slot;
  s->side = mr->entities.team[team].side;
  s->timer89 = r->timer89;
  s->timer81 = r->timer81;
  s->delta = r->delta;
  s->pos_x = r->pos_x;
  s->pos_y = r->pos_y;
  s->pos_y_word = (int16_t)r->pos_y;
  s->pos_z = r->pos_z;
  s->lane_word = (int16_t)r->lane;
  s->ball_height = (int16_t)mr->entities.ball.y;   /* derived 0x157750 (OL-65) */
  /* Pool-derived team/target state; the native +0x44/+0x85/+0x87 bytes, the
   * [team+0x7CB] callback record, the 0x1577CA exclusion and the roster
   * descriptor are unported (OL-65), and the 0x157A4D/0x14C2F6/0x14C326/
   * 0x14C32A mode gates default to their zero-image values (OL-66). */
  {
    int32_t id = r->entity_id;
    int32_t tid = mr->entities.team[team].target;
    int32_t sid = mr->entities.team[team].second;
    int32_t oid = mr->entities.team[1u - team].target;
    s->staged_code = (uint8_t)mr->entities.ball.pair.code;
    s->is_team_target = (id >= 0 && id == tid) ? 1u : 0u;
    s->is_team_second = (id >= 0 && id == sid) ? 1u : 0u;
    s->team_target_present = tid >= 0 ? 1u : 0u;
    s->opp_target_present = oid >= 0 ? 1u : 0u;
    if (tid >= 0 &&
        tid < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
      s->team_target_carrier =
          mr->entities.team[(uint32_t)tid / FIFA96_MATCH_ENTITY_RECORDS]
              .records[(uint32_t)tid % FIFA96_MATCH_ENTITY_RECORDS]
              .carrier != 0
              ? 1u
              : 0u;
    if (oid >= 0 &&
        oid < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
      const struct fifa96_match_entity *opp =
          &mr->entities.team[(uint32_t)oid / FIFA96_MATCH_ENTITY_RECORDS]
               .records[(uint32_t)oid % FIFA96_MATCH_ENTITY_RECORDS];
      s->opp_target_carrier = opp->carrier != 0 ? 1u : 0u;
      s->opp_present = 1;
      s->opp_type = opp->type;
      s->opp_has_slot = opp->has_slot;
      s->opp_lane_word = opp->lane_x;    /* +0x6B (<0xD0 gate) */
      s->opp_angle_x = opp->lane_z;      /* dword[+0x6D]>>16 = word[+0x6D] */
      s->opp_angle_z = 0;                /* word[+0x6F] unmodeled (OL-65) */
      s->opp_face_word = 0;              /* +0x7D facing word unmodeled (OL-65) */
    }
  }
  s->camera_x = mr->render.camera.pos_x;
  s->camera_y = mr->render.camera.pos_y;
  s->camera_z = mr->render.camera.pos_z;
  /* FU-141's frame staging shadows the 0x157788 target with the camera triple;
   * the 0x71B9C predictor source block is unported (OL-66). */
  s->stage_target_x = mr->render.camera.pos_x;
  s->stage_target_y = mr->render.camera.pos_y;
  s->stage_target_z = mr->render.camera.pos_z;
  s->predictor_x = mr->render.camera.pos_x;
  s->predictor_y = mr->render.camera.pos_y;
  s->predictor_z = mr->render.camera.pos_z;
  s->type_off_x = match_kick_dir_x;
  s->type_off_z = match_kick_dir_z;
  s->rng = &mr->rng;
}

static void match_kick_repack(struct fifa96_match_run *mr, const fifa96_action_kick *s,
                              const fifa96_action_kick_out *out) {
  struct fifa96_match_run_record *r = &mr->record;
  r->timer89 = s->timer89;
  r->timer81 = s->timer81;
  r->stage92 = s->stage92;
  r->target_x = s->target_x;
  r->target_y = s->target_y;
  r->target_z = s->target_z;
  /* +0x8E is the facing octant only when a face arm ran; otherwise the
   * record byte is untouched (the machine seeds `facing` 0, OL-65). */
  if (out->camera_face != 0 || out->corner_face != 0) r->type = s->facing;
  if (out->ran != 0) r->ran = 1;
  if (out->ball_install != 0) r->install = 4;
  else if (out->defender_install != 0) r->install = 0x0E;
  else if (out->reset_install != 0) r->install = out->reset_code;
  if (out->slot_merge != 0) r->helper_request = 1;
  if (out->snap != 0) {   /* 0x79B1C: target = pos */
    r->target_x = s->pos_x;
    r->target_y = s->pos_y;
    r->target_z = s->pos_z;
  }
}

/* One `fifa96_ball_kick_target` request from the machine. `input` is the
 * 0x158738 triple (NULL zeroes it, as kick 1 does); `slot_present` is 0 for
 * the row-0F corner kick (the native temporarily nulls `[rec+0x20]`). */
static int match_kick_run(struct fifa96_match_run *mr, const fifa96_action_kick *s,
                          const fifa96_ball_pair_vector *input, uint8_t mode,
                          uint8_t slot_present, fifa96_ball_kick_out *out) {
  fifa96_ball_kick_actor a;
  fifa96_ball_kick_slot slot;
  fifa96_ball_kick_ctx ctx;
  fifa96_entity_candidate cands[FIFA96_MATCH_ENTITY_RECORDS];
  fifa96_ball_kick_candidate full[FIFA96_MATCH_ENTITY_RECORDS];
  uint32_t team = match_kick_team(mr);
  uint32_t i;
  int32_t self = -1;
  const struct fifa96_match_entity *self_e = NULL;
  for (i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[team].records[i];
    cands[i].x = (int16_t)e->pos_x;
    cands[i].y = (int16_t)e->pos_z;
    cands[i].skip_98 = e->skip_98;
    cands[i].skip_9a = e->skip_9a;
    full[i].pos_x = e->pos_x;
    full[i].pos_z = e->pos_z;
    full[i].vel_x = (int16_t)(e->vel_x >> 16);
    full[i].vel_z = (int16_t)(e->vel_z >> 16);
    if ((int32_t)(team * FIFA96_MATCH_ENTITY_RECORDS + i) == mr->record.entity_id) {
      self = (int32_t)i;
      self_e = e;
    }
  }
  memset(&a, 0, sizeof a);
  a.id = mr->record.entity_id;
  a.pos_x = s->pos_x;
  a.pos_y = s->pos_y;
  a.pos_z = s->pos_z;
  a.vel_x = mr->record.vel_x;
  a.vel_z = mr->record.vel_z;
  if (self_e != NULL) {
    a.nudge_x = (int32_t)((uint16_t)self_e->lane_x |
                          ((uint32_t)(uint16_t)self_e->lane_z << 16));
    a.nudge_z = (int32_t)(uint16_t)self_e->lane_z;  /* +0x6F unmodeled, OL-62 */
  }
  a.active = s->active;
  a.type = s->type;
  a.actor_type = s->type8;
  a.side = s->side;
  a.facing = s->facing;
  memset(&slot, 0, sizeof slot);
  slot.present = slot_present;
  slot.word6 = s->slot_word6;
  slot.dir_x = mr->record.dir_x;
  slot.dir_z = mr->record.dir_z;
  memset(&ctx, 0, sizeof ctx);
  ctx.camera_x = s->camera_x;
  ctx.camera_y = s->camera_y;
  ctx.camera_z = s->camera_z;
  ctx.phase = s->phase;
  ctx.ball_height = s->ball_height;
  ctx.type_dir_x = s->type_off_x;
  ctx.type_dir_z = s->type_off_z;
  ctx.candidates = cands;
  ctx.candidate_count = FIFA96_MATCH_ENTITY_RECORDS;
  ctx.candidate_skip = (uint32_t)(uint16_t)(int16_t)s->active;
  ctx.self_index = self;
  ctx.team_records = full;
  ctx.sector_table = match_kick_sector;
  ctx.recompute_table = match_kick_recompute;
  ctx.event_rows[0] = match_kick_rows_height;
  ctx.event_rows[1] = match_kick_rows_carry;
  ctx.event_rows[2] = match_kick_rows_active;
  ctx.event_rows[3] = match_kick_rows_idle;
  return fifa96_ball_kick_target(&mr->entities.ball.pair, &a, &slot, input, &ctx,
                                 &mr->rng, mode, out);
}

/* The row-07 post-kick `FUN_0007D9A4(opp, 0x22, 0, 1)`: the opponent is the
 * other team's target record (team-target producer unported, OL-56). */
static void match_kick_invoke_opponent(struct fifa96_match_run *mr) {
  uint32_t team = match_kick_team(mr);
  int32_t id = mr->entities.team[1u - team].target;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return;
  (void)fifa96_match_entities_install(
      &mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS]
           .records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS],
      (uint8_t)mr->state.phase, 0x22, 0);
}

/* The row-07 tail `FUN_00079B58([0x158734])`: the receiver's +0x93 = 0x10. */
static void match_kick_receiver_timer(struct fifa96_match_run *mr) {
  int32_t id = mr->entities.ball.pair.receiver;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return;
  mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS]
      .records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS]
      .timer93 = 0x10;
}

static int fifa96_match_action_07(struct fifa96_match_run *mr) {
  fifa96_action_kick s;
  fifa96_action_kick_out out;
  int rc;
  match_kick_from_record(mr, 0x07u, &s);
  rc = fifa96_action_kick_machine(&s, &out);
  if (rc != FIFA96_OK) return rc;
  if (out.kick != 0) {
    fifa96_ball_kick_out bo;
    /* 0x8163C: EBX = 0x158738, i.e. the staged vector is passed to
     * FUN_0007B9C4 unchanged (a self-copy). */
    rc = match_kick_run(mr, &s, &mr->entities.ball.pair.vector, out.kick_mode,
                        s.has_slot, &bo);
    if (rc != FIFA96_OK) return rc;
    s.kick_done = 1;
    s.kick_staged = bo.staged;
    s.kick_traj = (int16_t)mr->entities.ball.pair.traj;
    rc = fifa96_action_kick_machine(&s, &out);
    if (rc != FIFA96_OK) return rc;
  }
  match_kick_repack(mr, &s, &out);
  if (out.opponent_invoke != 0) match_kick_invoke_opponent(mr);
  if (out.ball_install != 0) match_kick_receiver_timer(mr);
  return FIFA96_OK;
}

static int fifa96_match_action_0F(struct fifa96_match_run *mr) {
  fifa96_action_kick s;
  fifa96_action_kick_out out;
  int rc;
  match_kick_from_record(mr, 0x0Fu, &s);
  rc = fifa96_action_kick_machine(&s, &out);
  if (rc != FIFA96_OK) return rc;
  if (out.kick != 0) {
    fifa96_ball_kick_out bo;
    rc = match_kick_run(mr, &s, NULL, out.kick_mode, s.has_slot, &bo);
    if (rc != FIFA96_OK) return rc;
    s.kick_done = 1;
    rc = fifa96_action_kick_machine(&s, &out);
    if (rc != FIFA96_OK) return rc;
  } else if (out.corner_kick != 0) {
    fifa96_ball_pair_vector input;
    fifa96_ball_kick_out bo;
    input.x = s.kick_vec_x;
    input.height = s.kick_vec_height;
    input.z = s.kick_vec_z;
    rc = match_kick_run(mr, &s, &input, out.corner_kick_mode, 0, &bo);
    if (rc != FIFA96_OK) return rc;
    s.kick_done = 2;
    rc = fifa96_action_kick_machine(&s, &out);
    if (rc != FIFA96_OK) return rc;
  }
  match_kick_repack(mr, &s, &out);
  if (out.slot_merge != 0) mr->record.helper_request = 1;
  return FIFA96_OK;
}

/* ===== FU-142 OL-32 / M2 arms-and-wiring Task 12: rows 18/21/23 over the pool
 *
 * The three machines share the native `FUN_0007DAB4` reset (0x7DABA
 * `+0x92 = 0xFF`, 0x7DAC4 `+0x89 = 0`, 0x7DAEF the `FUN_0007C990`
 * forced-decision install when phase 2 and active, else the
 * `FUN_0007D9A4(rec, 0, 0, 0)` code-0 install). The forced-decision predicate
 * codes {3,4,6} are the FU-141 OL-44/§3.2 bounded model (the same one the kick
 * machine's reset uses); the code-0 install cannot ride the `install` request
 * field (0 = none), so it is applied synchronously through the pool installer,
 * exactly as the native. The record-side `+0x44`/`+0x99`/`+0x85`/`+0x77` and
 * the slot word `+0x10` are the staged-zero inputs recorded on OL-68. */

static uint8_t match_duel_carrier(const struct fifa96_match_run *mr, int32_t id) {
  if (id < 0 || id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return 0;
  return mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS]
                 .records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS]
                 .carrier != 0
             ? 1u
             : 0u;
}

/* `FUN_0007C990` bounded code set, mirroring the kick machine's `kick_reset`
 * (FU-141 OL-44): 0 = the type-5 no-install return (0x7C9C8). */
static uint8_t match_forced_decision_code(struct fifa96_match_run *mr,
                                          const struct fifa96_match_entity *e) {
  int32_t id = mr->record.entity_id;
  uint32_t team = id >= 0 ? (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS : e->team;
  const struct fifa96_match_team *t = &mr->entities.team[team];
  const struct fifa96_match_team *o = &mr->entities.team[1u - team];
  if (id >= 0 && id == t->target) {
    if (o->target < 0 || match_duel_carrier(mr, o->target) == 0) {
      if (e->type == 5u) return 0;
      return 4;
    }
    return 6;
  }
  if (id >= 0 && id == t->second) {
    if (o->target >= 0 && match_duel_carrier(mr, o->target) != 0) return 6;
    if (t->target < 0 || match_duel_carrier(mr, t->target) == 0) return 4;
  }
  return 3;
}

static void match_row_reset(struct fifa96_match_run *mr, struct fifa96_match_entity *e) {
  uint8_t code = 0;
  e->stage92 = 0xFF;   /* 0x7DABA */
  e->timer89 = 0;      /* 0x7DAC4 */
  mr->record.stage92 = 0xFF;
  mr->record.timer89 = 0;
  if (mr->state.phase == 2 && e->active != 0) {
    code = match_forced_decision_code(mr, e);
    if (code == 0) return;   /* 0x7C9C8: no install, the +0x92 = 0xFF stands */
  }
  if (fifa96_match_entities_install(e, (uint8_t)mr->state.phase, code, 0) != 0) {
    mr->record.stage92 = e->stage92;   /* the staged byte (0) */
    mr->record.timer89 = e->timer89;
    mr->record.ran = e->ran;
  }
}

/* Row 18 `0x849B0..0x84AE1` (FU-78 §7/FU-139 §2). The duel record is staged
 * into `fifa96_action_duel_step`; the resolution arms run over the pool: the
 * `FUN_0004C324` bind, the `+0x9A` latch, the NSEARCH/SWAP slot hand-off and
 * the shared reset. The native `[[rec+0x28]][0]` abort byte is staged zero
 * (OL-68), and the native 0x158897 search origin (written by FUN_0008A3FC from
 * the record) is the duel record's own position. */
static int fifa96_match_action_18(struct fifa96_match_run *mr) {
  fifa96_action_duel d;
  fifa96_action_duel_out out;
  int32_t id = mr->record.entity_id;
  int rc;
  memset(&d, 0, sizeof d);
  d.timer89 = mr->record.timer89;
  d.pos_x = mr->record.pos_x;
  d.pos_z = mr->record.pos_z;
  d.stage = mr->record.stage92;
  d.animation = 0;
  d.has_slot = mr->record.has_slot;
  /* The record's persisted +0x65 metric (written by the stage-1 0x8DCD4 arm;
   * the frame staging reproduces it from the persisted (0x900,0) target, the
   * FU-142e OL-60 model). */
  d.distance = (int16_t)mr->record.distance;
  rc = fifa96_action_duel_step(&d, mr->record.delta, (uint8_t)mr->input_state[0], &out);
  if (rc != FIFA96_OK) return rc;
  mr->record.timer89 = d.timer89;
  mr->record.stage92 = out.stage;
  mr->record.timer7b = d.stride;   /* native word[+0x7B] = 2 */
  if (out.target_set != 0) {
    mr->record.target_x = out.target_x;
    mr->record.target_y = 0;
    mr->record.target_z = out.target_z;
  }
  if (out.bind != 0) {
    fifa96_action_duel_bind_in bin;
    fifa96_action_duel_bind_out bout;
    memset(&bin, 0, sizeof bin);
    bin.mode_157ac2 = mr->global_157ac2;   /* producer unported (OL-68) */
    rc = fifa96_action_duel_bind(&bin, &bout);
    if (rc != FIFA96_OK) return rc;
    /* bout.bound/stub_36200 are derived for the loader tests and have no
     * engine consumer (OL-68). */
  }
  if (out.occupied != 0 && id >= 0 &&
      id < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
    uint32_t team = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
    struct fifa96_match_entity *e =
        &mr->entities.team[team].records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
    e->skip_9a = 1;   /* native byte[+0x9A] = 1 */
    if (out.handoff != 0) {
      fifa96_action_duel_candidate cands[FIFA96_MATCH_ENTITY_RECORDS];
      fifa96_action_duel_search_in sin;
      int32_t found = -1;
      uint32_t i;
      for (i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
        const struct fifa96_match_entity *c = &mr->entities.team[team].records[i];
        cands[i].pos_x = (int16_t)c->pos_x;
        cands[i].pos_z = (int16_t)c->pos_z;
        cands[i].has_slot = c->has_slot;
        cands[i].skip_98 = c->skip_98;
        cands[i].skip_9a = c->skip_9a;
        cands[i].is_chosen =
            mr->entities.team[team].chosen == (int32_t)(team * FIFA96_MATCH_ENTITY_RECORDS + i)
                ? 1u
                : 0u;
      }
      sin.x = (int16_t)mr->record.pos_x;
      sin.z = (int16_t)mr->record.pos_z;
      sin.skip_index = -1;
      sin.record0_gate = mr->entities.team[team].search_gate;
      sin.fallback = 1;
      rc = fifa96_action_duel_search(&sin, cands, FIFA96_MATCH_ENTITY_RECORDS, &found);
      if (rc != FIFA96_OK) return rc;
      if (found >= 0) {
        fifa96_action_duel_slot from;
        fifa96_action_duel_slot to;
        struct fifa96_match_entity *target = &mr->entities.team[team].records[found];
        from.has_slot = e->has_slot;
        to.has_slot = target->has_slot;
        rc = fifa96_action_duel_swap(&from, &to);
        if (rc != FIFA96_OK) return rc;
        e->has_slot = from.has_slot;
        target->has_slot = to.has_slot;
        mr->record.has_slot = e->has_slot;
      }
    }
    if (out.reset != 0) match_row_reset(mr, e);
  }
  return FIFA96_OK;
}

/* Row 21 `0x85214..0x8539B` (FU-78 §4/FU-139 §2). The phase/tracked head, the
 * camera copy, the stage gates and the 0x8DE8C/0x4A resolution arm run through
 * `fifa96_action_receive_step`; the reset path and the team-target
 * ball-actor install 4 + receiver timer are the pool side. */
static int fifa96_match_action_21(struct fifa96_match_run *mr) {
  fifa96_action_receive r;
  fifa96_action_receive_out out;
  fifa96_entity_candidate cands[FIFA96_MATCH_ENTITY_RECORDS];
  int32_t id = mr->record.entity_id;
  struct fifa96_match_entity *e = NULL;
  uint32_t team = 0;
  uint32_t i;
  int rc;
  if (id >= 0 && id < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
    team = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
    e = &mr->entities.team[team].records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
  }
  memset(&r, 0, sizeof r);
  r.timer89 = mr->record.timer89;
  r.offset_word = (int16_t)mr->record.lane;   /* native word[+0x6B] */
  r.stage = mr->record.stage92;
  r.active = mr->record.active;
  r.event_flag = 0;   /* native +0x44, staged zero (OL-68) */
  r.is_team_target =
      id >= 0 && id == mr->entities.team[team].target ? 1u : 0u;
  r.phase = (uint8_t)mr->state.phase;
  r.tracked = id >= 0 && id == mr->entities.controlled ? 1u : 0u;
  r.type8 = mr->record.actor_type;
  r.pos_x = mr->record.pos_x;
  r.pos_z = mr->record.pos_z;
  for (i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *c = &mr->entities.team[team].records[i];
    cands[i].x = (int16_t)c->pos_x;
    cands[i].y = (int16_t)c->pos_z;
    cands[i].skip_98 = c->skip_98;
    cands[i].skip_9a = c->skip_9a;
  }
  rc = fifa96_action_receive_step(&r, cands, FIFA96_MATCH_ENTITY_RECORDS, &out);
  if (rc != FIFA96_OK) return rc;
  /* out.nearest/out.anim are derived for the loader tests and have no engine
   * consumer (OL-52/OL-68). */
  /* 0x85242..0x8524C: the camera triple copy runs for every phase-2 tracked
   * call before the stage dispatch. */
  if (r.phase == 2 && r.tracked != 0) {
    mr->record.target_x = mr->render.camera.pos_x;
    mr->record.target_y = mr->render.camera.pos_y;
    mr->record.target_z = mr->render.camera.pos_z;
  }
  if (out.ran != 0) mr->record.ran = 1;
  mr->record.timer89 = r.timer89;
  mr->record.stage92 = out.stage;
  if (out.reset != 0 && e != NULL) match_row_reset(mr, e);
  if (out.handoff != 0) {
    /* 0x85375..0x8538D: install code 4 on the staged ball actor [0x158730]
     * and the FUN_00079B58 receiver timer on [0x158734]. */
    int32_t actor = mr->entities.ball.pair.actor;
    int32_t receiver = mr->entities.ball.pair.receiver;
    if (actor >= 0 && actor < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
      (void)fifa96_match_entities_install(
          &mr->entities.team[(uint32_t)actor / FIFA96_MATCH_ENTITY_RECORDS]
               .records[(uint32_t)actor % FIFA96_MATCH_ENTITY_RECORDS],
          (uint8_t)mr->state.phase, 4, 0);
    if (receiver >= 0 &&
        receiver < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
      mr->entities.team[(uint32_t)receiver / FIFA96_MATCH_ENTITY_RECORDS]
          .records[(uint32_t)receiver % FIFA96_MATCH_ENTITY_RECORDS]
          .timer93 = 0x10;
  }
  return FIFA96_OK;
}

/* Row 23 `0x82F84..0x83163` (FU-78 §6/FU-139 §2). The stage machine and the
 * target arm run through `fifa96_action_tackle_step`/`_attempt`; the
 * `0x1577xx` camera/track inputs are staged zero or from the engine camera
 * (the FU-141 frame shadows 0x157788/0x157794 with it) — OL-68. */
static int fifa96_match_action_23(struct fifa96_match_run *mr) {
  fifa96_action_tackle t;
  fifa96_action_tackle_out out;
  int32_t id = mr->record.entity_id;
  struct fifa96_match_entity *e = NULL;
  uint32_t team = 0;
  int rc;
  if (id >= 0 && id < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
    team = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
    e = &mr->entities.team[team].records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
  }
  memset(&t, 0, sizeof t);
  t.pos_x = mr->record.pos_x;
  t.pos_z = mr->record.pos_z;
  t.camera_x = mr->render.camera.pos_x;
  t.target_x = mr->render.camera.pos_x;   /* 0x71B9C predictor stand-in (OL-68) */
  t.target_z = mr->render.camera.pos_z;
  t.target_height = (int16_t)mr->render.camera.pos_y;
  t.close_word = (int16_t)mr->record.lane;   /* +0x6B */
  t.timer89 = mr->record.timer89;
  t.delta = mr->record.delta;
  t.phase = (uint8_t)mr->state.phase;
  t.is_tracked = 1;   /* native rec != [0x1577CA]; the exclusion is unmodeled */
  t.active = mr->record.active;
  t.side = mr->entities.team[team].side;
  t.stage = mr->record.stage92;
  t.has_slot = mr->record.has_slot;
  t.field5d = mr->record.pos_y != 0 ? 1u : 0u;   /* native dword +0x5D != 0 */
  t.vec788_x = mr->render.camera.pos_x;
  t.vec788_y = mr->render.camera.pos_y;
  t.vec788_z = mr->render.camera.pos_z;
  t.vec794_x = mr->render.camera.pos_x;
  t.vec794_y = mr->render.camera.pos_y;
  t.vec794_z = mr->render.camera.pos_z;
  t.vector_x = mr->render.camera.pos_x;   /* native word[0x157788] */
  t.vector_z = mr->render.camera.pos_z;   /* native word[0x157790] */
  rc = fifa96_action_tackle_step(&t, &out);
  if (rc != FIFA96_OK) return rc;
  mr->record.timer89 = t.timer89;
  mr->record.stage92 = out.stage;
  if (out.ran != 0) mr->record.ran = 1;
  if (out.install_0e != 0) mr->record.install = 0x0E;
  else if (out.install_0f != 0) mr->record.install = 0x0F;
  if (out.target_set != 0) {
    mr->record.target_x = out.target_x;
    mr->record.target_y = out.target_y;
    mr->record.target_z = out.target_z;
  }
  if (out.receiver_timer != 0 && e != NULL) e->timer93 = 0x10;
  if (out.reset != 0 && e != NULL) match_row_reset(mr, e);
  return FIFA96_OK;
}

/* Map a row-06 pursuit selection (NONE / SELF / mate index) to a pool id. */
static int32_t match_pursuit_id(uint32_t team, int32_t actor, int32_t index) {
  if (index == FIFA96_ACTION_PURSUIT_NONE) return FIFA96_MATCH_ENTITY_NONE;
  if (index == FIFA96_ACTION_PURSUIT_SELF) return actor;
  return (int32_t)(team * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)index);
}

/* FU-139 §11 (M2 arms-and-wiring Task 13 / OL-30): row 06 pursuit over the
 * pool. `fifa96_action_pursuit_step` runs the native target construction
 * (`0x8DCD4` metric, the `0x8DD70` angle, the `0x114E04` folds), the V4 RNG
 * install gates and the `0x8DE8C`/`0x79CCC` mate selections; the handler
 * resolves the team mate view, the `[0x158724]` carrier stand-in (the derived
 * pool ball carrier), the `team+0x7B2/+0x7B6` identities, the run scores and
 * the `[0x157A4F]` frame parity, then drains the requests: the target triple,
 * the install (4/8/9), the receiver timer, the team target/second writes, the
 * `FUN_0007DAB4` reset and the documented `0x6DA64` swap request. The record
 * `+0x90/+0x99/+0x9D`, the roster descriptor bytes, the row byte
 * `[[rec+0x28]]`, the row-05 `0x15872A/0x15872F` block and the camera-track
 * lead words are staged zero (OL-69); the `0x79CCC` callback position is the
 * record's own position stand-in (OL-69). */
/* ===== M2 playability-legs Task 1 / OL-70: row 04 chase/pressure =====
 *
 * `fifa96_match_action_04` binds `fifa96_outfield_row04_step` (the ported
 * `0x7E7C8..0x7F141` body, FU-142 Appendix K.5) to `mr->record` and the
 * FU-141 pool. The record staging carries the body's own fields (position,
 * target, timers, lane/bound, active/slot, the actor type +0x8E and the
 * +0x73/+0x75 velocity words); the pool supplies both team blocks for the
 * `0x8DE8C` nearest and `0x8DDE0` ranked searches and the opponent-team
 * target record for the `0x8DCD4` distance/tail arms. The native pointers
 * the pool does not model are staged as documented stand-ins: `[0x158777]`
 * is the pool ball carrier, `[0x1577CA]` the pool controlled entity,
 * `[team+0x828]` the pool slot-pool byte and `[team+0x7BF]` the pool chosen
 * record; `[team+0x7C7]`, `[team+0x7E7]`, the `+0x99/+0x9D/+0x44` record
 * bytes, the `rec[+4]` descriptor, the `0x1586D7` merge gate, the
 * `0x1577F0..0x157806` track words and the `0x71B9C` predictor stay zero /
 * camera-stand-in with their producers unported (OL-72). The step's install
 * sequence is applied in order (the native invokes immediately); the reset
 * request runs `match_row_reset` (the FU-142b `FUN_0007DAB4` model) and the
 * receiver-timer request applies the tested `0x79B58` effect. The
 * `fifa96_outfield_input_row`/`_chase_gate` machine subset ported by Task 14
 * belongs to the `FUN_0007CA54` record machine, not to the row-04 handler
 * body, so it stays unwired here (its own seam). */
static int32_t match_row04_index(uint32_t team, int32_t id) {
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return FIFA96_OUTFIELD_ROW04_NONE;
  if ((uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS != team)
    return FIFA96_OUTFIELD_ROW04_NONE;
  return id % (int32_t)FIFA96_MATCH_ENTITY_RECORDS;
}

static void match_row04_fill_mates(const struct fifa96_match_team *team,
                                   fifa96_outfield_row04_mate *out) {
  uint32_t i;
  for (i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &team->records[i];
    out[i].x = (int16_t)e->pos_x;
    out[i].z = (int16_t)e->pos_z;
    out[i].pos_x = e->pos_x;
    out[i].pos_z = e->pos_z;
    out[i].lane = e->lane_x;
    out[i].code = e->code;
    out[i].skip_98 = e->skip_98;
    out[i].skip_9a = e->skip_9a;
  }
}

static int fifa96_match_action_04(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  fifa96_outfield_row04_state s;
  fifa96_outfield_row04_out out;
  fifa96_outfield_row04_mate mates[FIFA96_MATCH_ENTITY_RECORDS];
  fifa96_outfield_row04_mate opps[FIFA96_MATCH_ENTITY_RECORDS];
  int32_t id = r->entity_id;
  uint32_t team = id >= 0 ? (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS : 0u;
  uint32_t opp = 1u - team;
  struct fifa96_match_entity *e = NULL;
  int32_t controlled = mr->entities.controlled;
  int rc;
  unsigned i;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  e = &mr->entities.team[team].records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
  match_row04_fill_mates(&mr->entities.team[team], mates);
  match_row04_fill_mates(&mr->entities.team[opp], opps);
  memset(&s, 0, sizeof s);
  s.phase = (uint8_t)mr->state.phase;
  s.active = r->active;
  s.has_slot = r->has_slot;
  s.byte99 = 0;                     /* +0x99 producer unported (OL-72) */
  s.byte9d = 0;                     /* +0x9D producer unported (OL-72) */
  s.type8 = r->actor_type;          /* native byte +0x8E */
  s.code = r->code;                 /* native byte +0x91 (the dispatched code) */
  s.row_byte = 0;                   /* byte[[rec+0x28]] (OL-52/OL-72) */
  s.desc_e = 0;                     /* rec[+4][+0xE] (roster descriptor, OL-72) */
  s.timer81 = r->timer81;
  s.timer89 = r->timer89;
  s.delta = r->delta;
  s.pos_x = r->pos_x;
  s.pos_y = r->pos_y;
  s.pos_z = r->pos_z;
  s.lane = e->lane_x;               /* native word +0x6B (pool `lane_x`) */
  s.bound = 0;                      /* native word +0x77 (producer unported) */
  s.word6d = e->lane_z;             /* native word +0x6D */
  s.word6f = 0;                     /* native word +0x6F unmodeled (OL-72) */
  s.vel_int_x = (int16_t)((uint32_t)r->vel_x >> 16);  /* word +0x73 */
  s.vel_int_z = (int16_t)((uint32_t)r->vel_z >> 16);  /* word +0x75 */
  s.face_word7d = 0;                /* word +0x7D unmodeled (OL-72) */
  s.slot_dir_x = r->dir_x;
  s.slot_dir_z = r->dir_z;
  s.slot_word10 = 0;                /* slot +0x10 unmodeled (OL-69/OL-72) */
  s.slot_word6 = 0;                 /* slot +0x6 unmodeled (OL-65/OL-72) */
  s.target_x = r->target_x;
  s.target_y = r->target_y;
  s.target_z = r->target_z;
  s.self_index = (int32_t)((uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS);
  s.team_target_index =
      match_row04_index(team, mr->entities.team[team].target);
  s.team_second_index =
      match_row04_index(team, mr->entities.team[team].second);
  s.opp_target_index = match_row04_index(opp, mr->entities.team[opp].target);
  s.opp_7c7_index = FIFA96_OUTFIELD_ROW04_NONE;   /* [opp+0x7C7] unported (OL-72) */
  s.is_carrier = id == mr->entities.ball.carrier ? 1u : 0u;   /* [0x158777] stand-in */
  s.is_ball_track = id == controlled ? 1u : 0u;               /* [0x1577CA] stand-in */
  s.is_team_7c7 = 0;                /* [team+0x7C7] unported (OL-72) */
  s.is_team_7cb = 0;                /* [team+0x7CB] unported (OL-72) */
  s.user_present = controlled >= 0 ? 1u : 0u;
  s.user_is_self = id == controlled ? 1u : 0u;
  s.side = mr->entities.team[team].side;
  s.team_828 = mr->entities.team[team].slot_pool;
  s.team_7e7 = 0;                   /* [team+0x7E7] producer unported (OL-72) */
  s.team_7bf = mr->entities.team[team].chosen >= 0 ? 1u : 0u;
  s.merge_gate_1586d7 = 0;          /* [0x1586D7] producer unported (OL-72) */
  s.team_7d7 = 0;                   /* [team+0x7D7] producer unported (OL-72) */
  s.opp_7d7 = 0;                    /* [[team+0x7A6]+0x7D7] (same) */
  s.team_corner_z = 0;              /* team[+0x7E8+12n] producer unported (OL-72) */
  s.score_word[0] = (int16_t)mr->score[0];
  s.score_word[1] = (int16_t)mr->score[1];
  s.side_flip = 0;                  /* [0x157ABE] producer unported (OL-72) */
  s.ball_height = mr->entities.ball.y;
  s.camera_x = mr->render.camera.pos_x;
  s.camera_y = mr->render.camera.pos_y;
  s.camera_z = mr->render.camera.pos_z;
  s.vec5770_x = mr->render.camera.pos_x;   /* FU-141 frame shadows (OL-66) */
  s.vec5770_y = mr->render.camera.pos_y;
  s.vec5770_z = mr->render.camera.pos_z;
  s.vec5788_x = mr->render.camera.pos_x;
  s.vec5788_y = mr->render.camera.pos_y;
  s.vec5788_z = mr->render.camera.pos_z;
  s.vec5794_x = mr->render.camera.pos_x;
  s.vec5794_y = mr->render.camera.pos_y;
  s.vec5794_z = mr->render.camera.pos_z;
  s.lead_x = 0;                     /* word[0x1577C0] (OL-72) */
  s.lead_z = 0;                     /* word[0x1577C2] (OL-72) */
  s.track_577f0 = 0;                /* 0x1577F0 track block (OL-72) */
  s.track_577f2 = 0;
  s.track_577fa = 0;
  s.track_57800 = 0;
  s.track_57802 = 0;
  s.track_57806 = 0;
  s.predictor_x = mr->render.camera.pos_x;  /* 0x71B9C(4) stand-in (OL-74) */
  s.predictor_y = mr->render.camera.pos_y;
  s.predictor_z = mr->render.camera.pos_z;
  s.ball_track_side = controlled >= 0
                          ? mr->entities
                                .team[(uint32_t)controlled / FIFA96_MATCH_ENTITY_RECORDS]
                                .side
                          : 0;
  s.type_off_x = match_kick_dir_x;
  s.type_off_z = match_kick_dir_z;
  s.mates = mates;
  s.mate_count = FIFA96_MATCH_ENTITY_RECORDS;
  s.opps = opps;
  s.opp_count = FIFA96_MATCH_ENTITY_RECORDS;
  s.rng = &mr->rng;
  rc = fifa96_outfield_row04_step(&s, &out);
  if (rc != FIFA96_OK) return rc;
  if (out.ran != 0) r->ran = 1;
  r->timer89 = out.timer89;
  if (out.target_set != 0) {
    r->target_x = out.target_x;
    r->target_y = out.target_y;
    r->target_z = out.target_z;
  }
  if (out.team_target_set != 0) {
    mr->entities.team[team].target =
        out.team_target_index == FIFA96_OUTFIELD_ROW04_NONE
            ? FIFA96_MATCH_ENTITY_NONE
            : (int32_t)(team * FIFA96_MATCH_ENTITY_RECORDS +
                        (uint32_t)out.team_target_index);
  }
  if (out.team_second_set != 0) {
    mr->entities.team[team].second =
        out.team_second_index == FIFA96_OUTFIELD_ROW04_NONE
            ? FIFA96_MATCH_ENTITY_NONE
            : (int32_t)(team * FIFA96_MATCH_ENTITY_RECORDS +
                        (uint32_t)out.team_second_index);
  }
  if (out.reset != 0) match_row_reset(mr, e);
  for (i = 0; i < out.install_count; i++) {
    if (out.installs[i].target == FIFA96_OUTFIELD_ROW04_INSTALL_OTHER) {
      int32_t oid = mr->entities.team[opp].target;
      if (oid >= 0 &&
          oid < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
        (void)fifa96_match_entities_install(
            &mr->entities.team[(uint32_t)oid / FIFA96_MATCH_ENTITY_RECORDS]
                 .records[(uint32_t)oid % FIFA96_MATCH_ENTITY_RECORDS],
            (uint8_t)mr->state.phase, out.installs[i].code, out.installs[i].staged);
    } else {
      (void)fifa96_match_entities_install(e, (uint8_t)mr->state.phase,
                                          out.installs[i].code, out.installs[i].staged);
    }
  }
  if (out.receiver_timer != 0 && s.byte99 == 0u) e->timer93 = 0x10;  /* 0x79B58 */
  if (out.slot_merge != 0) r->helper_request = 1;                    /* 0x7876C */
  /* out.anim (0x6E598, OL-52), out.slot_backup/slot_restore (0x78A84/
   * 0x78AA4, OL-65), out.corner/team7e7_inc/opp_7e7_clear (team block bytes,
   * OL-72) and out.events (the 0x974DC/0x8F188/0x92820/0x71C94/0x974F0/
   * 0x651F0 sinks, OL-67/OL-72) have no derived consumer. */
  return FIFA96_OK;
}

/* ===== M2 playability-legs Task 2 / OL-70a: row 08 approach/scan =====
 *
 * `fifa96_match_action_08` binds `fifa96_outfield_row08_step` (the ported
 * `0x81068..0x814AF` body, FU-142 Appendix K.6) to `mr->record` and the
 * FU-141 pool. The record staging carries the body's own fields (stage +0x92,
 * the +0x89 timer, position, active/slot, the +0x8E face octant the 0x79C50
 * call rewrites and the native +0x3D frame gate). The pool supplies the lane
 * words +0x6B/+0x6D, the ball height (0x157750), the [0x157A83] controlled
 * actor and the two team blocks for the inactive-arm clears. Row 08 installs
 * no code, so there is no install arm. The native pointers the pool does not
 * model are staged as documented stand-ins: [0x1577CA] is the pool controlled
 * entity, `[team+0x7B2]`/`[team+0x7B6]` are the pool target/second ids,
 * `[[team+0x7A6]+0x7B2]` the opponent team target, and the +0x44/+0x99
 * record bytes, the rec[+4] descriptor bytes, the +0x7D word, the
 * [0x15877D]/[0x15872F] process bytes and the 0x1577C0/C2 lead words stay
 * zero with their producers unported (OL-82); [0x15877D]/[0x15872F] writes
 * have no derived home (OL-82). The step's reset request runs
 * `match_row_reset` and the receiver-timer request applies the tested
 * `0x79B58` effect (the +0x99 callee gate is staged 0, as row 04). */
static int fifa96_match_action_08(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  fifa96_outfield_row08_state s;
  fifa96_outfield_row08_out out;
  int32_t id = r->entity_id;
  uint32_t team = 0;
  uint32_t opp = 1;
  struct fifa96_match_entity *e = NULL;
  int32_t controlled = mr->entities.controlled;
  int rc;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  team = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  opp = 1u - team;
  e = &mr->entities.team[team].records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
  memset(&s, 0, sizeof s);
  s.phase = (uint8_t)mr->state.phase;
  s.stage = r->stage92;
  s.active = r->active;
  s.has_slot = r->has_slot;
  s.byte44 = 0;                     /* native +0x44 producer unported (OL-82) */
  s.byte3d = r->frame;              /* native +0x3D (producer OL-80/OL-82) */
  s.byte_15877d = 0;                /* [0x15877D] producer unported (OL-82) */
  s.byte_15872f = 0;                /* [0x15872F] producer unported (OL-82) */
  s.is_team_target = id == mr->entities.team[team].target ? 1u : 0u;
  s.is_team_second = id == mr->entities.team[team].second ? 1u : 0u;
  s.user_present = controlled >= 0 ? 1u : 0u;
  s.user_is_opp_target =
      (controlled >= 0 && mr->entities.team[opp].target == controlled) ? 1u : 0u;
  s.user_row_byte = 0;              /* byte[[user+0x28]] unported (OL-82) */
  s.desc_c = 0;                     /* rec[+4] descriptor bytes (OL-82) */
  s.desc_16 = 0;
  s.desc_opp_c = 0;
  s.desc_opp_f = 0;
  s.type8 = r->type;                /* native +0x8E (the face octant) */
  s.timer89 = r->timer89;
  s.delta = r->delta;
  s.pos_x = r->pos_x;
  s.pos_y = r->pos_y;
  s.pos_z = r->pos_z;
  s.lane = e->lane_x;               /* native word +0x6B */
  s.word6d = e->lane_z;             /* native word +0x6D */
  s.word7d = 0;                     /* native +0x7D (the face write is not
                                     * persisted; producer unported, OL-82) */
  s.ball_height = mr->entities.ball.y;   /* dword 0x157750 (OL-65) */
  s.is_ball_track = id == controlled ? 1u : 0u;   /* [0x1577CA] stand-in */
  s.camera_x = mr->render.camera.pos_x;
  s.camera_y = mr->render.camera.pos_y;
  s.camera_z = mr->render.camera.pos_z;
  s.lead_x = 0;                     /* word[0x1577C0] (OL-82) */
  s.lead_z = 0;                     /* word[0x1577C2] (OL-82) */
  s.type_off_x = match_kick_dir_x;
  s.type_off_z = match_kick_dir_z;
  s.rng = &mr->rng;
  rc = fifa96_outfield_row08_step(&s, &out);
  if (rc != FIFA96_OK) return rc;
  if (out.ran != 0) r->ran = 1;
  r->timer89 = out.timer89;
  r->stage92 = out.stage92;
  if (out.target_set != 0) {
    r->target_x = out.target_x;
    r->target_y = out.target_y;
    r->target_z = out.target_z;
  }
  if (out.face != 0) r->type = out.face_octant;                 /* native +0x8E */
  if (out.clear_team_target != 0 && mr->entities.team[team].target == id)
    mr->entities.team[team].target = FIFA96_MATCH_ENTITY_NONE;
  if (out.clear_team_second != 0 && mr->entities.team[team].second == id)
    mr->entities.team[team].second = FIFA96_MATCH_ENTITY_NONE;
  if (out.reset != 0) match_row_reset(mr, e);
  if (out.receiver_timer != 0) e->timer93 = 0x10;   /* 0x79B58 (0x99 staged 0) */
  /* out.anim (0x6E598, OL-52), out.ball_stage (the 0x7A490 staging call,
   * OL-62), out.events/event_code/event_sound (the 0x8ED40/0x8F188/0x92820/
   * 0x974F0/0x651F0 sinks, OL-82), out.face_angle (+0x7D has no pool field,
   * OL-82), the out.byte_15877d/byte_15872f process-byte writes (OL-82) and
   * out.snap beyond the target copy (0x79B1C's lane/velocity zeroes, OL-82)
   * have no derived consumer. */
  return FIFA96_OK;
}

static int fifa96_match_action_06(struct fifa96_match_run *mr) {
  fifa96_action_pursuit s;
  fifa96_action_pursuit_out out;
  fifa96_action_pursuit_mate mates[FIFA96_MATCH_ENTITY_RECORDS];
  int32_t id = mr->record.entity_id;
  int32_t tid;
  uint32_t team = 0;
  struct fifa96_match_entity *e = NULL;
  const struct fifa96_match_entity *carrier = NULL;
  uint32_t i;
  int rc;
  if (id >= 0 && id < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
    team = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
    e = &mr->entities.team[team].records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
  }
  memset(&s, 0, sizeof s);
  s.actor = id;
  s.timer89 = mr->record.timer89;
  s.pos_x = mr->record.pos_x;
  s.pos_y = mr->record.pos_y;
  s.pos_z = mr->record.pos_z;
  /* native +0x71 (word = the anim speed) and the +0x69 dword: the low word is
   * the unported pre-frame walk's dz (recomputed from pos/target, FU-142e
   * OL-60) and the high word the record lane (+0x6B, pool `lane_x`). */
  s.vel_x = mr->record.vel_x;
  s.lane_dword = (int32_t)(((uint32_t)(uint16_t)(e != NULL ? e->lane_x : 0) << 16) |
                           (uint16_t)((uint16_t)mr->record.target_z -
                                      (uint16_t)mr->record.pos_z));
  s.word6d = e != NULL ? e->lane_z : 0;   /* +0x6D */
  s.word6f = 0;                           /* +0x6F unmodeled (OL-69) */
  s.timer81 = mr->record.timer81;
  s.delta = mr->record.delta;
  s.phase = (uint8_t)mr->state.phase;
  s.active = mr->record.active;
  s.has_slot = mr->record.has_slot;
  s.slot_gate = 0;                        /* byte[slot+0x10] & 0x30 (OL-69) */
  s.slot_dir_x = mr->record.dir_x;
  s.slot_dir_z = mr->record.dir_z;
  s.side = mr->entities.team[team].side;
  s.type8 = mr->record.actor_type;
  s.row_byte = 0;                         /* [[rec+0x28]] (OL-52/OL-69) */
  s.byte99 = 0;                           /* +0x99 (OL-69) */
  s.byte9d = 0;                           /* +0x9D (OL-69) */
  s.desc_c = 0;                           /* rec[+4][+0xC] (OL-69) */
  s.desc_e = 0;                           /* rec[+4][+0xE] (OL-69) */
  s.byte90 = 0;                           /* +0x90 (OL-69) */
  s.parity = mr->pass_parity;             /* [0x157A4F] (0x4B11A) */
  s.byte_15872f = 0;                      /* row-05 producer (OL-63/OL-69) */
  s.adjust_x = 0;                         /* byte[0x15872A] (OL-63/OL-69) */
  s.score_own = mr->score[team];          /* word[0x157AC5 + 2*side] */
  s.score_other = mr->score[1u - team];
  s.carrier = mr->entities.ball.carrier;  /* [0x158724] stand-in (OL-69) */
  if (s.carrier >= 0 &&
      s.carrier < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
    carrier = &mr->entities.team[(uint32_t)s.carrier / FIFA96_MATCH_ENTITY_RECORDS]
                   .records[(uint32_t)s.carrier % FIFA96_MATCH_ENTITY_RECORDS];
    s.carrier_lane = carrier->lane_x;
    s.carrier_speed = (int16_t)carrier->vel_x;
    s.carrier_pos_x = carrier->pos_x;
    s.carrier_pos_z = carrier->pos_z;
  }
  tid = mr->entities.team[team].target;
  s.team_target = tid;
  s.team_second = mr->entities.team[team].second;
  s.teammate_z = 0;
  if (tid >= 0 &&
      tid < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
    s.teammate_z = mr->entities.team[(uint32_t)tid / FIFA96_MATCH_ENTITY_RECORDS]
                       .records[(uint32_t)tid % FIFA96_MATCH_ENTITY_RECORDS]
                       .pos_z;
  }
  s.self_index = id >= 0 ? id % (int32_t)FIFA96_MATCH_ENTITY_RECORDS : 0;
  s.ball_height = mr->entities.ball.y;    /* derived 0x157750 (OL-65) */
  s.camera_x = mr->render.camera.pos_x;
  s.camera_y = mr->render.camera.pos_y;
  s.camera_z = mr->render.camera.pos_z;
  s.lead_x = 0;                           /* word[0x1577C0] (OL-69) */
  s.lead_z = 0;                           /* word[0x1577C2] */
  s.rng = &mr->rng;
  for (i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *m = &mr->entities.team[team].records[i];
    mates[i].x = (int16_t)m->pos_x;
    mates[i].z = (int16_t)m->pos_z;
    mates[i].pos_z = m->pos_z;
    mates[i].skip_98 = m->skip_98;
    mates[i].skip_9a = m->skip_9a;
  }
  rc = fifa96_action_pursuit_step(&s, mates, FIFA96_MATCH_ENTITY_RECORDS, &out);
  if (rc != FIFA96_OK) return rc;
  mr->record.timer89 = s.timer89;
  if (out.ran != 0) mr->record.ran = 1;
  if (out.target_set != 0) {
    mr->record.target_x = out.target_x;
    mr->record.target_y = out.target_y;
    mr->record.target_z = out.target_z;
  }
  if (out.install != 0) mr->record.install = out.install;
  if (out.receiver_timer != 0 && e != NULL) e->timer93 = 0x10;   /* 0x79B58 */
  if (out.team_target_set != 0) {
    mr->entities.team[team].target = match_pursuit_id(team, id, out.team_target_index);
  }
  if (out.team_second_set != 0) {
    mr->entities.team[team].second = match_pursuit_id(team, id, out.team_second_index);
  }
  if (out.clear_target != 0 && mr->entities.team[team].target == id)
    mr->entities.team[team].target = FIFA96_MATCH_ENTITY_NONE;
  if (out.clear_second != 0 && mr->entities.team[team].second == id)
    mr->entities.team[team].second = FIFA96_MATCH_ENTITY_NONE;
  if (out.reset != 0 && e != NULL) match_row_reset(mr, e);
  /* out.anim (0x6E598) and out.swap (0x6DA64) have no derived consumer
   * (OL-52/OL-69). */
  return FIFA96_OK;
}

const struct fifa96_match_handler fifa96_match_action_table[FIFA96_MATCH_ACTION_ROWS] = {
    {0x00, fifa96_match_action_00,
     "FU-138 §4/FU-141: row 00 ported over the entity pool; install/ran drained by the pool installer"},
    {0x01, NULL,
     "FU-137 §6: FU-136 row 01: not ported (partial); sequence_select/stage + FU-138 marker_target/stage_wait; OL-17"},
    {0x02, NULL,
     "FU-137 §6: FU-136 row 02: not ported (partial); locomotion_restart_target + FU-138 restart_wait; OL-18"},
    {0x03, NULL,
     "FU-137 §6: FU-136 row 03: not ported (partial); hold/clamp + FU-138 counter/phase1_clamp; OL-19"},
    {0x04, fifa96_match_action_04,
     "FU-142 App. K.5/FU-137 §6.1 (Task 1/OL-70): row 04 ported (fifa96_outfield_row04_step, 0x7E7C8..0x7F141) over the pool; the carrier/ranked-pick/forced/chase installs 4/0x19/0xF/0xE/0xB/7/6/5 and the target/timer/team writes are bound; the 0x7CA54 input-row machine stays a separate unwired seam; unmodeled record/team bytes, track words and sinks OL-65/OL-67/OL-72"},
    {0x05, NULL,
     "FU-139 §8/§5: row 05 carrier machine ported (0x7F194..0x7F665 stages 0-3) + staging tail 0x7A8D1..0x7AA2F; stage-0 tail 0x7F3A1..0x7F57B + FUN_0007F7E0 unbounded; unwired; -UNSUPPORTED; OL-63"},
    {0x06, fifa96_match_action_06,
     "FU-139 §11 (Task 13/OL-30): row 06 ported (0x801B4..0x809EF pursuit machine; 0x8DCD4 metric + 0x8DD70 angle + 0x114E04 folds, RNG installs 8/9, carrier-gate install 4, 0x8DE8C/0x79CCC selections) over the pool; row-09 0x80A00 boundary; record bytes/lead/swap OL-69"},
    {0x07, fifa96_match_action_07,
     "FU-139 §9/FU-137 §5.2: row 07 ported (0x814B0..0x81737 machine; 0x7B9C4 kick path) over the pool; defender 0x0E/opponent 0x22/ball 4 requests; descriptor bytes and predictor OL-65/OL-66"},
    {0x08, fifa96_match_action_08,
     "FU-142 App. K.6/FU-137 §6.1 (Task 2/OL-70a): row 08 ported (fifa96_outfield_row08_step, 0x81068..0x814AF) over the pool; the stage-0 camera/face, the +0x3D-gated projection scan, the 0x79B1C snap and the process-byte writes are bound; row 08 installs no code; unmodeled record/process bytes, the 0x7A490 staging and the event/sound sinks OL-52/OL-62/OL-82"},
    {0x09, NULL, "FU-137 §6: FU-136 row 09: not ported (partial); FU-81 arm table 0x809F0; OL-9"},
    {0x0A, NULL, "FU-137 §6: FU-136 row 0A: not ported; installer 0x7CDD8 has no xrefs; OL-14"},
    {0x0B, NULL, "FU-137 §6: FU-136 row 0B: not ported (partial); sequence_duel_event; OL-9"},
    {0x0C, NULL, "FU-137 §6: FU-136 row 0C: not ported (partial); FU-81 7-arm table 0x81C74; OL-9"},
    {0x0D, NULL,
     "FU-137 §6: FU-136 row 0D: not ported (partial); FU-82 4-arm 0x8250C + FU-138 velocity_scale; OL-22"},
    {0x0E, NULL, "FU-137 §6: FU-136 row 0E: not ported; FU-81 gate/head, no body port; OL-9"},
    {0x0F, fifa96_match_action_0F,
     "FU-139 §9/FU-137 §5.2: row 0F ported (0x82AD0..0x82DCF machine; 0x7B9C4 kick path + 0x79B1C snap/0x79B6C face) over the pool; corner/predictor table inputs OL-66"},
    {0x10, NULL, "FU-137 §6: FU-136 row 10: not ported (partial); FU-81 7-arm table 0x855B8; OL-9"},
    {0x11, NULL, "FU-137 §6: FU-136 row 11: not ported (partial); FU-81 10-arm table 0x85DA0; OL-9"},
    {0x12, NULL, "FU-137 §6: FU-136 row 12: not ported (partial); FU-81 tables 0x83D2C/0x83D4C; OL-9"},
    {0x13, NULL, "FU-137 §6: FU-136 row 13: not ported (partial); FU-81 7-arm table 0x84AE4; OL-9"},
    {0x14, NULL, "FU-137 §6: FU-136 row 14: not ported (partial); scatter_celebration helpers; OL-9"},
    {0x15, NULL, "FU-137 §6: FU-136 row 15: not ported; head mis-decoded, stub bucket; OL-14"},
    {0x16, NULL, "FU-137 §6: FU-136 row 16: not ported (partial); sequence_marker/rng_event; OL-9"},
    {0x17, NULL, "FU-137 §6: FU-136 row 17: not ported (partial); FU-81 4-arm table 0x84720; OL-9"},
    {0x18, fifa96_match_action_18,
     "FU-139 §10/FU-137: row 18 ported (0x849B0..0x84AE1 machine; 0x4C324 bind, 0x8DB6C NSEARCH + 0x786A0 SWAP, 0x7DAB4 reset) over the pool; install arm 0x8A32F; anim row byte and bind globals OL-68"},
    {0x19, NULL, "FU-140 §2/§3: row 19 not ported (partial); keeper_hold_* + FU-140 fallback; OL-33"},
    {0x1A, NULL, "FU-140 §2: row 1A not ported (partial); keeper_reposition_a_gate; OL-34"},
    {0x1B, NULL, "FU-140 §2: row 1B not ported (partial); keeper_reposition_b_finish; OL-34"},
    {0x1C, NULL, "FU-140 §2: row 1C not ported (partial); keeper_lunge_track; OL-35"},
    {0x1D, NULL, "FU-140 §2: row 1D not ported (partial); keeper_clear_vector; OL-35"},
    {0x1E, fifa96_match_action_1E,
     "FU-140 §4/FU-141: row 1E ported over the entity pool; helper/place/actor requests drained; OL-37 arm bodies"},
    {0x1F, NULL, "FU-140 §2/§3: row 1F not ported (partial); keeper_dive_target/arm_step + input_decide; OL-36"},
    {0x20, NULL, "FU-137 §6: FU-136 row 20: not ported (partial); FU-82 7-arm table 0x84ED0; OL-9"},
    {0x21, fifa96_match_action_21,
     "FU-139 §10/FU-137: row 21 ported (0x85214..0x8539B machine; camera copy, 0x8DE8C nearest + 0x4A anim arm, 0x7DAB4 reset, team-target ball 4/receiver) over the pool; install arms 0x7D046 (and row-05 0x7F791/0x7F7B6); +0x44 and anim record writes OL-68/OL-52"},
    {0x22, NULL, "FU-137 §6: FU-136 row 22: not ported (partial); sequence_press_event; OL-9"},
    {0x23, fifa96_match_action_23,
     "FU-139 §10/FU-137: row 23 ported (0x82F84..0x83163 machine; 0x82DD0 attempt, stage-1 target arm, 0x79B58 receiver, installs 0x0E/0x0F) over the pool; install arm 0x7D1B9; 0x1577xx camera/track inputs staged zero OL-68"},
    {0x24, NULL, "FU-137 §6: FU-136 row 24: not ported (partial); sequence_lane/anim_byte; OL-9"},
    {0x25, NULL, "FU-137 §6: FU-136 row 25: not ported (partial); FU-82 7-arm table 0x880B0; OL-9"},
    {0x26, fifa96_match_action_26,
     "FU-142b §C/FU-137 §5.2: row 26 ported (0x866F4..0x8681C + 0x8DCD4) over the pool; arm 0x8D74D; stage92/timer7b/lane repacked"},
    {0x27, NULL,
     "FU-142b App. D/FU-142f App. I/FU-137 §5.3: row 27 body 0x86820..0x86A02 ported; FU-142f census of all 77 FUN_0007D9A4 call sites finds no 0x27 invocation (constants and register-derived args; no stored installer pointer); entry unresolved (FU-142f/OL-48); -UNSUPPORTED"},
    {0x28, fifa96_match_action_28,
     "FU-142d App. G/FU-137 §5.2: row 28 ported (0x870E8..0x874E3, 4-arm table 0x870D8 + 0x87014 helper) over the pool; arm 0x8D7CF; scratch/target/type/vel repacked"},
    {0x29, NULL,
     "FU-142c App. F/FU-142f App. I/FU-137 §5.3: row 29 body 0x874E4..0x87738 ported (fifa96_arm_29_step; phase-5 machine, self-install 3); the action-table slot 0x110784 is the only body reference and the FU-142f census of all 77 FUN_0007D9A4 call sites finds no 0x29 invocation; entry unresolved (FU-142f/OL-48); -UNSUPPORTED"},
    {0x2A, fifa96_match_action_2A,
     "FU-142e App. H/FU-137 §5.2: row 2A ported (0x86A34..0x87010, 12-arm table 0x86A04 + 0x513EC camera-stop no-op) over the pool; arm 0x8D807; distance/flag830/global/target/vel repacked"},
    {0x2B, NULL,
     "FU-142f dead verdict/FU-142 §1.1: native entry 0x87738 is the shared row-29 epilogue RET, not a standalone stub; FU-142f census of all 77 FUN_0007D9A4 call sites finds no 0x2B invocation; dead entry; OL-15; -UNSUPPORTED"},
    {0x2C, NULL,
     "FU-142b App. E/FU-142f App. I/FU-137 §5.3: row 2C body 0x84598..0x8462D ported (fifa96_arm_2c_step + fifa96_arm_reset); the action-table slot is the only reference to the body; the FU-142f census of all 77 FUN_0007D9A4 call sites finds no 0x2C invocation; entry unresolved (FU-142f/OL-48); -UNSUPPORTED"},
};

const struct fifa96_match_handler fifa96_match_phase_table[FIFA96_MATCH_PHASE_ROWS] = {
    {0x00, NULL, "FU-137 §6: FU-136 phase 00: not ported; FU-83 held-position copy; OL-13"},
    {0x01, NULL, "FU-137 §6: FU-136 phase 01: not ported (partial); fifa96_action_phase_cell; OL-13"},
    {0x02, NULL, "FU-137 §6: FU-136 phase 02: not ported (partial); phase_cell, ptr-2 lookup unported; OL-13"},
    {0x03, NULL, "FU-137 §6: FU-136 phase 03: not ported (partial); fifa96_action_phase_slot; OL-13"},
    {0x04, NULL, "FU-137 §6: FU-136 phase 04: not ported (partial); same body as phase 03; OL-13"},
    {0x05, NULL, "FU-137 §6: FU-136 phase 05: not ported; FU-83 distance line; OL-13"},
    {0x06, NULL, "FU-137 §6: FU-136 phase 06: not ported (partial); phase_ball_entry/ball_line; OL-13"},
    {0x07, NULL, "FU-137 §6: FU-136 phase 07: not ported (partial); same body as phase 03; OL-13"},
    {0x08, NULL, "FU-137 §6: FU-136 phase 08: not ported (partial); wrapper -> 0x6DCC8; phase_cell; OL-13"},
    {0x09, NULL, "FU-137 §6: FU-136 phase 09: not ported (partial); same as phase 08; OL-13"},
    {0x0A, NULL, "FU-137 §6: FU-136 phase 0A: not ported; same body as phase 00; OL-13"},
    {0x0B, NULL, "FU-137 §6: FU-136 phase 0B: not ported; same body as phase 00; OL-13"},
    {0x0C, NULL, "FU-137 §6: FU-136 phase 0C: not ported (partial); phase_line_timer/restart_line; OL-13"},
    {0x0D, NULL, "FU-137 §6: FU-136 phase 0D: not ported; same body as phase 00; OL-13"},
    {0x0E, NULL, "FU-137 §6: FU-136 phase 0E: not ported; same body as phase 00; OL-13"},
    {0x0F, NULL, "FU-137 §6: FU-136 phase 0F: not ported; same body as phase 00; OL-13"},
    {0x10, NULL, "FU-137 §6: FU-136 phase 10: not ported; FU-83 variant tables 0x105E7/0x105E8; OL-13"},
    {0x11, NULL, "FU-137 §6: FU-136 phase 11: not ported (partial); falls into 0x6E1D0; OL-13"},
    {0x12, NULL, "FU-137 §6: FU-136 phase 12: not ported (partial); same as phase 01; OL-13"},
    {0x13, NULL, "FU-137 §6: FU-136 phase 13: not ported; FU-83 camera-bound scatter; OL-13"},
    {0x14, NULL, "FU-137 §6: FU-136 phase 14: not ported; same as phase 13; OL-13"},
    {0x15, NULL, "FU-137 §6: FU-136 phase 15: not ported (partial); same as phase 02; OL-13"},
    {FIFA96_MATCH_PHASE_INT3_SLOT, NULL,
     "FU-137 §1.2: native 0x110794[0x16]=0 loader INT3 stub; no body by design; NOT_FOUND"},
    {0x17, NULL, "FU-137 §6: FU-136 phase 17: not ported; FU-83 timeline; OL-13"},
    {0x18, NULL, "FU-137 §6: FU-136 phase 18: not ported; FU-83 timeline; OL-13"},
    {0x19, NULL, "FU-137 §6: FU-136 phase 19: not ported; FU-83 timeline, installs action 0x16; OL-13"},
    {0x1A, NULL, "FU-137 §6: FU-136 phase 1A: not ported; FU-83 timeline; OL-13"},
    {0x1B, NULL, "FU-137 §6: FU-136 phase 1B: not ported; FU-83 pure reset; OL-13"},
    {0x1C, NULL, "FU-137 §6: FU-136 phase 1C: not ported; FU-83 timeline; OL-13"},
    {0x1D, NULL, "FU-137 §6: FU-136 phase 1D: not ported; FU-83 timeline, installs 0x19/3; OL-13"},
    {0x1E, NULL, "FU-137 §6: FU-136 phase 1E: not ported; FU-83 timeline; OL-13"},
    {0x1F, NULL, "FU-137 §6: FU-136 phase 1F: not ported; FU-83 timeline; OL-13"},
    {0x20, NULL, "FU-137 §6: FU-136 phase 20: not ported; FU-83 timeline, installs 0x24; OL-13"},
    {0x21, NULL, "FU-137 §6: FU-136 phase 21: not ported; FU-83 timeline; OL-13"},
    {0x22, NULL, "FU-137 §6: FU-136 phase 22: not ported; FU-83 timeline; OL-13"},
};

int fifa96_match_dispatch_row(struct fifa96_match_run *mr,
                              const struct fifa96_match_handler *row) {
  if (mr == NULL || row == NULL) return -FIFA96_ERR_INVALID;
  if (row->fn == NULL) return -FIFA96_ERR_UNSUPPORTED;
  return row->fn(mr);
}

int fifa96_match_dispatch_action(struct fifa96_match_run *mr, uint8_t code) {
  int rc;
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (code >= FIFA96_MATCH_ACTION_ROWS) return -FIFA96_ERR_NOT_FOUND;
  rc = fifa96_match_dispatch_row(mr, &fifa96_match_action_table[code]);
  /* Task 15 / M2-B: record which wired rows actually dispatched OK during a
   * replay (the acceptance tape's wired-row assertion set). Bookkeeping only;
   * all 45 codes fit in the 64-bit mask. */
  if (rc == FIFA96_OK && code < 64u) mr->dispatched_ok |= 1ull << code;
  return rc;
}

int fifa96_match_dispatch_phase(struct fifa96_match_run *mr, uint8_t phase) {
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (phase >= FIFA96_MATCH_PHASE_ROWS) return -FIFA96_ERR_NOT_FOUND;
  if (phase == FIFA96_MATCH_PHASE_INT3_SLOT) return -FIFA96_ERR_NOT_FOUND;
  return fifa96_match_dispatch_row(mr, &fifa96_match_phase_table[phase]);
}
