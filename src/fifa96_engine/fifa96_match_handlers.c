/* src/fifa96_engine/fifa96_match_handlers.c — FU-137 dispatch tables and seam.
 *
 * The per-row evidence strings are the compact form of the FU-137 §6.1
 * (action) and §6.2 (phase) classification tables; FU-136 §2/§3 hold the full
 * evidence cells (docs, tested C symbols, estimated port group). Every row is
 * either `not ported` (fn NULL, UNSUPPORTED with its port group) or the two
 * `unwired` rows 00/1E (fn NULL, tested library bodies that need the
 * not-yet-ported record/entity model before they can bind to a match run —
 * open leg OL-1); the 0x27/0x29/0x2B/0x2C rows are FU-137 open legs (no static
 * install arm found, OL-15). Phase 0x16 is the native zero/INT3 slot and is
 * NOT_FOUND. Later G2 clusters replace a NULL fn with their derived body and
 * update the evidence string; they must not change the code/class of a row
 * without an FU-doc errata. */
#include <stddef.h>

#include "fifa96_engine/fifa96_match_handlers.h"

const struct fifa96_match_handler fifa96_match_action_table[FIFA96_MATCH_ACTION_ROWS] = {
    {0x00, NULL, "FU-136 row 00: unwired; fifa96_action_move_step/_target tested; OL-1"},
    {0x01, NULL, "FU-136 row 01: not ported (partial); sequence_select/stage helpers; OL-9"},
    {0x02, NULL, "FU-136 row 02: not ported (partial); locomotion_restart_target; OL-9"},
    {0x03, NULL, "FU-136 row 03: not ported (partial); locomotion_hold/clamp_placement; OL-8"},
    {0x04, NULL, "FU-136 row 04: not ported (partial); locomotion_camera_lead; OL-8"},
    {0x05, NULL, "FU-136 row 05: not ported (partial); possession_reset/claim/timer; OL-8"},
    {0x06, NULL, "FU-136 row 06: not ported; FU-77 §2.6 derived, no port; OL-8"},
    {0x07, NULL, "FU-136 row 07: not ported (partial); kick_angle/kick_apply; OL-8"},
    {0x08, NULL, "FU-136 row 08: not ported (partial); chase-gate installer only; OL-8"},
    {0x09, NULL, "FU-136 row 09: not ported (partial); FU-81 arm table 0x809F0; OL-9"},
    {0x0A, NULL, "FU-136 row 0A: not ported; installer 0x7CDD8 has no xrefs; OL-14"},
    {0x0B, NULL, "FU-136 row 0B: not ported (partial); sequence_duel_event; OL-9"},
    {0x0C, NULL, "FU-136 row 0C: not ported (partial); FU-81 7-arm table 0x81C74; OL-9"},
    {0x0D, NULL, "FU-136 row 0D: not ported (partial); FU-82 4-arm table 0x8250C; OL-9"},
    {0x0E, NULL, "FU-136 row 0E: not ported; FU-81 gate/head, no body port; OL-9"},
    {0x0F, NULL, "FU-136 row 0F: not ported; FU-76 KICK 0x7B9C4 body; OL-8"},
    {0x10, NULL, "FU-136 row 10: not ported (partial); FU-81 7-arm table 0x855B8; OL-9"},
    {0x11, NULL, "FU-136 row 11: not ported (partial); FU-81 10-arm table 0x85DA0; OL-9"},
    {0x12, NULL, "FU-136 row 12: not ported (partial); FU-81 tables 0x83D2C/0x83D4C; OL-9"},
    {0x13, NULL, "FU-136 row 13: not ported (partial); FU-81 7-arm table 0x84AE4; OL-9"},
    {0x14, NULL, "FU-136 row 14: not ported (partial); scatter_celebration helpers; OL-9"},
    {0x15, NULL, "FU-136 row 15: not ported; head mis-decoded, stub bucket; OL-14"},
    {0x16, NULL, "FU-136 row 16: not ported (partial); sequence_marker/rng_event; OL-9"},
    {0x17, NULL, "FU-136 row 17: not ported (partial); FU-81 4-arm table 0x84720; OL-9"},
    {0x18, NULL, "FU-136 row 18: not ported (partial); duel_step/duel_split; OL-11"},
    {0x19, NULL, "FU-136 row 19: not ported (partial); keeper_hold_* helpers; OL-10"},
    {0x1A, NULL, "FU-136 row 1A: not ported (partial); keeper_reposition_a_gate; OL-10"},
    {0x1B, NULL, "FU-136 row 1B: not ported (partial); keeper_reposition_b_finish; OL-10"},
    {0x1C, NULL, "FU-136 row 1C: not ported (partial); keeper_lunge_track; OL-10"},
    {0x1D, NULL, "FU-136 row 1D: not ported (partial); keeper_clear_vector; OL-10"},
    {0x1E, NULL, "FU-136 row 1E: unwired; fifa96_keeper_claim_place tested; OL-1"},
    {0x1F, NULL, "FU-136 row 1F: not ported (partial); keeper_dive_target/arm_step; OL-10"},
    {0x20, NULL, "FU-136 row 20: not ported (partial); FU-82 7-arm table 0x84ED0; OL-9"},
    {0x21, NULL, "FU-136 row 21: not ported (partial); action_receive_step; OL-11"},
    {0x22, NULL, "FU-136 row 22: not ported (partial); sequence_press_event; OL-9"},
    {0x23, NULL, "FU-136 row 23: not ported (partial); tackle_step/tackle_attempt; OL-11"},
    {0x24, NULL, "FU-136 row 24: not ported (partial); sequence_lane/anim_byte; OL-9"},
    {0x25, NULL, "FU-136 row 25: not ported (partial); FU-82 7-arm table 0x880B0; OL-9"},
    {0x26, NULL, "FU-137 arm 0x8D74D (phase 13/14 non-controlled side); body 0x0866F4 unanalyzed; OL-15"},
    {0x27, NULL, "FU-137 open leg: no install arm found (EDX=0x27 at 0x756D5 is an anim arg); body 0x086820 cut; OL-15"},
    {0x28, NULL, "FU-137 arm 0x8D7CF (phase 13/14 player side); body 0x0870E8 unanalyzed; OL-15"},
    {0x29, NULL, "FU-137 open leg: no install arm or match-code 0x29 reference found; OL-15"},
    {0x2A, NULL, "FU-137 arm 0x8D807 (scan [rec+0x9A], store [team+0x831]); body 0x086A34; OL-15"},
    {0x2B, NULL, "FU-137 open leg: native body is one-byte RET 0x87738; no install arm found; OL-15"},
    {0x2C, NULL, "FU-137 open leg: prologue-only body 0x084598; no install arm found; OL-15"},
};

const struct fifa96_match_handler fifa96_match_phase_table[FIFA96_MATCH_PHASE_ROWS] = {
    {0x00, NULL, "FU-136 phase 00: not ported; FU-83 held-position copy; OL-13"},
    {0x01, NULL, "FU-136 phase 01: not ported (partial); fifa96_action_phase_cell; OL-13"},
    {0x02, NULL, "FU-136 phase 02: not ported (partial); phase_cell, ptr-2 lookup unported; OL-13"},
    {0x03, NULL, "FU-136 phase 03: not ported (partial); fifa96_action_phase_slot; OL-13"},
    {0x04, NULL, "FU-136 phase 04: not ported (partial); same body as phase 03; OL-13"},
    {0x05, NULL, "FU-136 phase 05: not ported; FU-83 distance line; OL-13"},
    {0x06, NULL, "FU-136 phase 06: not ported (partial); phase_ball_entry/ball_line; OL-13"},
    {0x07, NULL, "FU-136 phase 07: not ported (partial); same body as phase 03; OL-13"},
    {0x08, NULL, "FU-136 phase 08: not ported (partial); wrapper -> 0x6DCC8; phase_cell; OL-13"},
    {0x09, NULL, "FU-136 phase 09: not ported (partial); same as phase 08; OL-13"},
    {0x0A, NULL, "FU-136 phase 0A: not ported; same body as phase 00; OL-13"},
    {0x0B, NULL, "FU-136 phase 0B: not ported; same body as phase 00; OL-13"},
    {0x0C, NULL, "FU-136 phase 0C: not ported (partial); phase_line_timer/restart_line; OL-13"},
    {0x0D, NULL, "FU-136 phase 0D: not ported; same body as phase 00; OL-13"},
    {0x0E, NULL, "FU-136 phase 0E: not ported; same body as phase 00; OL-13"},
    {0x0F, NULL, "FU-136 phase 0F: not ported; same body as phase 00; OL-13"},
    {0x10, NULL, "FU-136 phase 10: not ported; FU-83 variant tables 0x105E7/0x105E8; OL-13"},
    {0x11, NULL, "FU-136 phase 11: not ported (partial); falls into 0x6E1D0; OL-13"},
    {0x12, NULL, "FU-136 phase 12: not ported (partial); same as phase 01; OL-13"},
    {0x13, NULL, "FU-136 phase 13: not ported; FU-83 camera-bound scatter; OL-13"},
    {0x14, NULL, "FU-136 phase 14: not ported; same as phase 13; OL-13"},
    {0x15, NULL, "FU-136 phase 15: not ported (partial); same as phase 02; OL-13"},
    {FIFA96_MATCH_PHASE_INT3_SLOT, NULL,
     "FU-137 §1.2: native 0x110794[0x16]=0 loader INT3 stub; no body by design; NOT_FOUND"},
    {0x17, NULL, "FU-136 phase 17: not ported; FU-83 timeline; OL-13"},
    {0x18, NULL, "FU-136 phase 18: not ported; FU-83 timeline; OL-13"},
    {0x19, NULL, "FU-136 phase 19: not ported; FU-83 timeline, installs action 0x16; OL-13"},
    {0x1A, NULL, "FU-136 phase 1A: not ported; FU-83 timeline; OL-13"},
    {0x1B, NULL, "FU-136 phase 1B: not ported; FU-83 pure reset; OL-13"},
    {0x1C, NULL, "FU-136 phase 1C: not ported; FU-83 timeline; OL-13"},
    {0x1D, NULL, "FU-136 phase 1D: not ported; FU-83 timeline, installs 0x19/3; OL-13"},
    {0x1E, NULL, "FU-136 phase 1E: not ported; FU-83 timeline; OL-13"},
    {0x1F, NULL, "FU-136 phase 1F: not ported; FU-83 timeline; OL-13"},
    {0x20, NULL, "FU-136 phase 20: not ported; FU-83 timeline, installs 0x24; OL-13"},
    {0x21, NULL, "FU-136 phase 21: not ported; FU-83 timeline; OL-13"},
    {0x22, NULL, "FU-136 phase 22: not ported; FU-83 timeline; OL-13"},
};

int fifa96_match_dispatch_row(struct fifa96_match_run *mr,
                              const struct fifa96_match_handler *row) {
  if (mr == NULL || row == NULL) return -FIFA96_ERR_INVALID;
  if (row->fn == NULL) return FIFA96_ERR_UNSUPPORTED;
  return row->fn(mr);
}

int fifa96_match_dispatch_action(struct fifa96_match_run *mr, uint8_t code) {
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (code >= FIFA96_MATCH_ACTION_ROWS) return FIFA96_ERR_NOT_FOUND;
  return fifa96_match_dispatch_row(mr, &fifa96_match_action_table[code]);
}

int fifa96_match_dispatch_phase(struct fifa96_match_run *mr, uint8_t phase) {
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (phase >= FIFA96_MATCH_PHASE_ROWS) return FIFA96_ERR_NOT_FOUND;
  if (phase == FIFA96_MATCH_PHASE_INT3_SLOT) return FIFA96_ERR_NOT_FOUND;
  return fifa96_match_dispatch_row(mr, &fifa96_match_phase_table[phase]);
}
