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
 * 0x27/0x29/0x2B/0x2C rows are FU-137 open legs (no static install arm found,
 * OL-15). Phase 0x16 is the native zero/INT3 slot and returns
 * -FIFA96_ERR_NOT_FOUND. Later G2 clusters replace a NULL fn with their derived
 * body and update the evidence string; they must not change the code/class of a
 * row without an FU-doc errata. FU-139 (cluster B) derives the ball
 * staging/resolver/possession/kick helpers (tested in
 * test_ball_pairing/test_action_handlers) and leaves action rows
 * 05/06/07/0F/18/21/23 unwired with their arms (FU-139 OL-29..OL-32; the pool
 * they also waited on is now FU-141); their evidence strings cite FU-139. */
#include <stddef.h>

#include "fifa96_engine/fifa96_match_handlers.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_loader/fifa96_action_handlers.h"
#include "fifa96_loader/fifa96_keeper.h"

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

const struct fifa96_match_handler fifa96_match_action_table[FIFA96_MATCH_ACTION_ROWS] = {
    {0x00, fifa96_match_action_00,
     "FU-138 §4/FU-141: row 00 ported over the entity pool; install/ran drained by the pool installer"},
    {0x01, NULL,
     "FU-137 §6: FU-136 row 01: not ported (partial); sequence_select/stage + FU-138 marker_target/stage_wait; OL-17"},
    {0x02, NULL,
     "FU-137 §6: FU-136 row 02: not ported (partial); locomotion_restart_target + FU-138 restart_wait; OL-18"},
    {0x03, NULL,
     "FU-137 §6: FU-136 row 03: not ported (partial); hold/clamp + FU-138 counter/phase1_clamp; OL-19"},
    {0x04, NULL, "FU-137 §6: FU-136 row 04: not ported (partial); locomotion_camera_lead; OL-8"},
    {0x05, NULL,
     "FU-139 §2/§5: row 05 not ported (partial); possession_reset/claim/timer/dribble_dir; carrier arms OL-29"},
    {0x06, NULL,
     "FU-139 §2/§5: row 06 not ported; FU-77 §2.6 597-insn pursuit body unported; OL-30"},
    {0x07, NULL,
     "FU-139 §2/§5: row 07 not ported (partial); kick_angle/apply + FU-139 resolver/stage target; kick machine OL-31"},
    {0x08, NULL, "FU-137 §6: FU-136 row 08: not ported (partial); chase-gate installer only; OL-8"},
    {0x09, NULL, "FU-137 §6: FU-136 row 09: not ported (partial); FU-81 arm table 0x809F0; OL-9"},
    {0x0A, NULL, "FU-137 §6: FU-136 row 0A: not ported; installer 0x7CDD8 has no xrefs; OL-14"},
    {0x0B, NULL, "FU-137 §6: FU-136 row 0B: not ported (partial); sequence_duel_event; OL-9"},
    {0x0C, NULL, "FU-137 §6: FU-136 row 0C: not ported (partial); FU-81 7-arm table 0x81C74; OL-9"},
    {0x0D, NULL,
     "FU-137 §6: FU-136 row 0D: not ported (partial); FU-82 4-arm 0x8250C + FU-138 velocity_scale; OL-22"},
    {0x0E, NULL, "FU-137 §6: FU-136 row 0E: not ported; FU-81 gate/head, no body port; OL-9"},
    {0x0F, NULL,
     "FU-139 §2/§5: row 0F not ported; KICK 0x7B9C4 body (FU-76 §2/§3.3); machine OL-31"},
    {0x10, NULL, "FU-137 §6: FU-136 row 10: not ported (partial); FU-81 7-arm table 0x855B8; OL-9"},
    {0x11, NULL, "FU-137 §6: FU-136 row 11: not ported (partial); FU-81 10-arm table 0x85DA0; OL-9"},
    {0x12, NULL, "FU-137 §6: FU-136 row 12: not ported (partial); FU-81 tables 0x83D2C/0x83D4C; OL-9"},
    {0x13, NULL, "FU-137 §6: FU-136 row 13: not ported (partial); FU-81 7-arm table 0x84AE4; OL-9"},
    {0x14, NULL, "FU-137 §6: FU-136 row 14: not ported (partial); scatter_celebration helpers; OL-9"},
    {0x15, NULL, "FU-137 §6: FU-136 row 15: not ported; head mis-decoded, stub bucket; OL-14"},
    {0x16, NULL, "FU-137 §6: FU-136 row 16: not ported (partial); sequence_marker/rng_event; OL-9"},
    {0x17, NULL, "FU-137 §6: FU-136 row 17: not ported (partial); FU-81 4-arm table 0x84720; OL-9"},
    {0x18, NULL,
     "FU-139 §2/§5: row 18 not ported (partial); duel_step/duel_split; NSEARCH/SWAP + pool OL-32"},
    {0x19, NULL, "FU-140 §2/§3: row 19 not ported (partial); keeper_hold_* + FU-140 fallback; OL-33"},
    {0x1A, NULL, "FU-140 §2: row 1A not ported (partial); keeper_reposition_a_gate; OL-34"},
    {0x1B, NULL, "FU-140 §2: row 1B not ported (partial); keeper_reposition_b_finish; OL-34"},
    {0x1C, NULL, "FU-140 §2: row 1C not ported (partial); keeper_lunge_track; OL-35"},
    {0x1D, NULL, "FU-140 §2: row 1D not ported (partial); keeper_clear_vector; OL-35"},
    {0x1E, fifa96_match_action_1E,
     "FU-140 §4/FU-141: row 1E ported over the entity pool; helper/place/actor requests drained; OL-37 arm bodies"},
    {0x1F, NULL, "FU-140 §2/§3: row 1F not ported (partial); keeper_dive_target/arm_step + input_decide; OL-36"},
    {0x20, NULL, "FU-137 §6: FU-136 row 20: not ported (partial); FU-82 7-arm table 0x84ED0; OL-9"},
    {0x21, NULL,
     "FU-139 §2/§5: row 21 not ported (partial); action_receive_step; claim arm + pool OL-32"},
    {0x22, NULL, "FU-137 §6: FU-136 row 22: not ported (partial); sequence_press_event; OL-9"},
    {0x23, NULL,
     "FU-139 §2/§5: row 23 not ported (partial); tackle_step/tackle_attempt; target arm + pool OL-32"},
    {0x24, NULL, "FU-137 §6: FU-136 row 24: not ported (partial); sequence_lane/anim_byte; OL-9"},
    {0x25, NULL, "FU-137 §6: FU-136 row 25: not ported (partial); FU-82 7-arm table 0x880B0; OL-9"},
    {0x26, NULL, "FU-137 arm 0x8D74D (phase 13/14 non-controlled side); body 0x0866F4 unanalyzed; OL-15"},
    {0x27, NULL, "FU-137 open leg: no install arm found (EDX=0x27 at 0x756D5 is an anim arg); body 0x086820 cut; OL-15"},
    {0x28, NULL, "FU-137 arm 0x8D7CF (phase 13/14 player side); body 0x0870E8 unanalyzed; OL-15"},
    {0x29, NULL, "FU-137 open leg: no install arm or match-code 0x29 reference found; OL-15"},
    {0x2A, NULL, "FU-137 arm 0x8D807 (scan [rec+0x9A], store [team+0x831]); body 0x086A34; OL-15"},
    {0x2B, NULL, "FU-137 open leg: native body is one-byte RET 0x87738; no install arm found; OL-15"},
    {0x2C, NULL, "FU-137 open leg: prologue-only body 0x084598; no install arm found; OL-15"},
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
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (code >= FIFA96_MATCH_ACTION_ROWS) return -FIFA96_ERR_NOT_FOUND;
  return fifa96_match_dispatch_row(mr, &fifa96_match_action_table[code]);
}

int fifa96_match_dispatch_phase(struct fifa96_match_run *mr, uint8_t phase) {
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (phase >= FIFA96_MATCH_PHASE_ROWS) return -FIFA96_ERR_NOT_FOUND;
  if (phase == FIFA96_MATCH_PHASE_INT3_SLOT) return -FIFA96_ERR_NOT_FOUND;
  return fifa96_match_dispatch_row(mr, &fifa96_match_phase_table[phase]);
}
