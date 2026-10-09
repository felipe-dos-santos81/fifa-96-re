/* src/fifa96_loader/fifa96_referee.c — FU-150 P2: fouls / referee / offside.
 *
 * Derived from the frozen slice docs/ghidra/FU150_fouls_referee_offside.md;
 * key claims re-verified first-hand on /FIFA96.EXE for this port (registrar
 * 0x8A3FC, decision 0x8A43C, foul machine 0x89FA4, offside machine 0x89110,
 * check 0x79D5C, clamp 0x7D3E4). Unreachable machinery stays numbered legs in
 * FU-150. See include/fifa96_loader/fifa96_referee.h for the per-function
 * contracts. */
#include "fifa96_loader/fifa96_referee.h"
#include <string.h>

/* FUN_0007D3E4: x clamps to [-0x720, 0x720], z to [-0xB10, 0xB10]; y passes
 * untouched (the normal path zeroes it separately). */
static void ref_clamp_point(int32_t point[3]) {
  if (point[0] >= 0x721) point[0] = 0x720;
  else if (point[0] < -0x720) point[0] = -0x720;
  if (point[2] > 0xB10) point[2] = 0xB10;
  else if (point[2] < -0xB10) point[2] = -0xB10;
}

static uint8_t ref_side_index(const struct fifa96_referee_state *state, uint8_t side) {
  return (uint8_t)((side ^ state->side_swap) & 1u);
}

int fifa96_ref_contact_register(struct fifa96_referee_state *state, uint8_t kind,
                                int32_t rec_a, int32_t rec_b, const int32_t point[3]) {
  if (!state) return -FIFA96_ERR_INVALID;
  state->contact_kind = kind;
  state->rec_first = rec_a;
  state->rec_second = rec_b;
  if (point) {
    state->point[0] = point[0];
    state->point[1] = point[1];
    state->point[2] = point[2];
  } else {
    /* The native NULL fallback copies rec_b/rec_a +0x59; the module owns no
     * records, so the derived fallback is the zero triple (documented). */
    state->point[0] = 0;
    state->point[1] = 0;
    state->point[2] = 0;
  }
  return 0;
}

int fifa96_ref_foul_decide(struct fifa96_referee_state *state,
                           const struct fifa96_match_config *cfg,
                           const struct fifa96_ref_record *fouler,
                           const struct fifa96_ref_record *victim,
                           const int32_t point[3], uint8_t rng_bits,
                           struct fifa96_ref_decision_out *out) {
  const int32_t *src;
  int32_t triple[3];
  uint8_t kind, side_idx, player;
  if (!state || !cfg || !out) return -FIFA96_ERR_INVALID;
  /* 0x8A44C phase-2 gate, 0x8A45E fouler gate, 0x8A47A settings gate: all
   * before any store. A NULL fouler is the native gate (no decision), not a
   * caller error. */
  if (!fouler) return 0;
  if (state->phase != 2u) return 0;
  if (cfg->field_4c306 == 0) return 0;
  kind = state->contact_kind;              /* the staged native AX entry kind */
  side_idx = ref_side_index(state, fouler->side);
  player = fouler->player < FIFA96_REF_SEVERITY_PLAYERS ? fouler->player : 0u;
  /* 0x8A487..0x8A4D7: contact/record stores and the point (source: the caller
   * point, else the stored point) with y forced 0 and the clamp. */
  state->contact_kind = kind;
  state->foul_kind = 0;
  state->rec_first = fouler->id;
  state->rec_first_side = (uint8_t)(fouler->side & 1u);
  state->rec_first_player = player;
  state->rec_second = victim ? victim->id : 0;
  src = point ? point : state->point;
  triple[0] = src[0];
  triple[1] = 0;
  triple[2] = src[2];
  ref_clamp_point(triple);
  state->point[0] = triple[0];
  state->point[1] = triple[1];
  state->point[2] = triple[2];
  /* 0x8A4DC `JLE` skips severity at settings 0xA == 1; the severity block
   * needs the duel-table compare (0x8A543/0x8A549, staged duel_ok) and the
   * [rec+0x8D] gate (0x8A556). */
  if (cfg->field_4c306 > 1 && fouler->duel_ok != 0 && fouler->active != 0 &&
      fouler->player < FIFA96_REF_SEVERITY_PLAYERS) {
    uint8_t draw = (uint8_t)(rng_bits & 0x3Fu);
    uint8_t acc = (uint8_t)(state->severity[side_idx][player] & 0x7Fu);
    uint8_t count = state->team_count[fouler->side & 1u];
    if (draw < 0x12u) {                    /* 0x8A571: 18/64 */
      if (acc == 0u) state->foul_kind = 1u;
      else if (count > 8u) state->foul_kind = 2u;
    } else if (draw < 0x16u && kind == 2u && count > 8u) {  /* 0x8A615: 4/64 */
      state->foul_kind = 3u;
    }
  }
  out->sequence = FIFA96_REF_SEQ_NONE;
  out->foul_kind = state->foul_kind;
  out->restart = 0;
  out->restart_side = 0;
  if (state->foul_kind == 0u) {            /* 0x8A70A: situation 9, BX=1 */
    out->restart = 1u;
    out->restart_side = (uint8_t)((fouler->side ^ 1u) & 1u);
    return 1;
  }
  /* 0x8A640..0x8A6FE: the foul-log ring (shift at the 10-entry wrap). */
  if (state->log_count >= FIFA96_REF_FOUL_LOG) {
    for (uint32_t i = 0; i + 1u < FIFA96_REF_FOUL_LOG; i++)
      state->log[i] = state->log[i + 1u];
    state->log[FIFA96_REF_FOUL_LOG - 1u].minute = state->game_minute;
    state->log[FIFA96_REF_FOUL_LOG - 1u].kind = state->foul_kind;
    state->log[FIFA96_REF_FOUL_LOG - 1u].team = ref_side_index(state, fouler->side);
    state->log[FIFA96_REF_FOUL_LOG - 1u].player = fouler->id;
  } else {
    uint8_t idx = state->log_count++;
    state->log[idx].minute = state->game_minute;
    state->log[idx].kind = state->foul_kind;
    state->log[idx].team = ref_side_index(state, fouler->side);
    state->log[idx].player = fouler->id;
  }
  out->sequence = FIFA96_REF_SEQ_ACT3;     /* 0x8A6FE FUN_000888FC(3,0,1) */
  return 1;
}

int fifa96_ref_offside_check(const struct fifa96_referee_state *state,
                             const struct fifa96_match_config *cfg,
                             const struct fifa96_ref_receiver *receiver,
                             const struct fifa96_ref_metric *metric,
                             int32_t last_defender_z, int32_t camera_ref,
                             uint8_t mirror, uint8_t rng_bits, uint8_t *offside) {
  uint32_t tol;
  uint8_t side;
  if (!state || !cfg || !receiver || !metric || !offside)
    return -FIFA96_ERR_INVALID;
  *offside = 0;
  /* 0x79D68 phase 2, 0x79D79 `word[0x157A6A] > 0` skip, 0x79D87 settings. */
  if (state->phase != 2u) return 0;
  if ((int16_t)state->offside_suppress > 0) return 0;
  if (cfg->field_4c2f2 == 0) return 0;
  /* 0x79DA4..0x79DBF: |[0x157754]| <= 0x990, else the mirror must be 0. The
   * magnitude is taken in 32-bit unsigned (the native NEG wraps INT_MIN). */
  {
    uint32_t mag = (uint32_t)camera_ref;
    if (camera_ref < 0) mag = 0u - mag;
    if (mag > 0x990u && mirror != 0) return 0;
  }
  /* 0x79DC5..0x79DE8: the metric+4 side gate. */
  side = (uint8_t)(receiver->side & 1u);
  if (side == 0u) {
    if (metric->side_gate <= 0) return 0;
  } else {
    if (metric->side_gate >= 0) return 0;
  }
  /* 0x79DF1..0x79E17 / 0x79E6C..0x79E92: own-nearest identity and depth. */
  if (!receiver->own_nearest_valid) return 0;
  if (receiver->own_nearest_is_receiver) return 0;
  if (side == 0u) {
    if (receiver->own_nearest_z <= 0x3B0) return 0;
  } else {
    if (receiver->own_nearest_z >= -0x3B0) return 0;
  }
  /* 0x79E30..0x79E36 / 0x79EAB..0x79EB1: receiver-vs-last-defender. */
  if (side == 0u) {
    if (receiver->z > last_defender_z) return 0;
  } else {
    if (receiver->z < last_defender_z) return 0;
  }
  /* 0x79E3C..0x79E4E / 0x79EB7..0x79EC5: the 6-bit tolerance (32-bit
   * unsigned, as the native `JC`/`JA`). */
  tol = (uint32_t)(rng_bits & 0x3Fu);
  if (side == 0u) {
    if ((uint32_t)receiver->own_nearest_z - tol < (uint32_t)last_defender_z)
      return 0;
  } else {
    if ((uint32_t)receiver->own_nearest_z + tol > (uint32_t)last_defender_z)
      return 0;
  }
  /* 0x79E54..0x79E64 / 0x79ECB..0x79EDB: the metric[0] x2 distance term. */
  if (side == 0u) {
    if ((int32_t)metric->tol2 * 2 + receiver->z < receiver->own_nearest_z)
      return 0;
  } else {
    if (receiver->z - (int32_t)metric->tol2 * 2 > receiver->own_nearest_z)
      return 0;
  }
  /* 0x79EE1..0x79EED: the nearest-query metric word <= 0x3C0. */
  if (receiver->own_distance > 0x3C0) return 0;
  /* 0x79EEF..0x79F1D: the event-record state gate. */
  if (!receiver->eligible) return 0;
  *offside = 1;
  return 0;
}

int fifa96_ref_offside_event(struct fifa96_referee_state *state,
                             const struct fifa96_match_config *cfg,
                             const struct fifa96_ref_record *rec,
                             const int32_t point[3],
                             struct fifa96_ref_event_out *out) {
  int32_t triple[3];
  if (!state || !cfg || !rec || !point || !out) return -FIFA96_ERR_INVALID;
  out->speech_code = 0;
  out->sequence = FIFA96_REF_SEQ_NONE;
  out->rec_id = 0;
  /* 0x8A73A: the settings 0x10 gate. */
  if (cfg->field_4c2f2 == 0) return 0;
  /* 0x8A743 speech event 0x15, 0x8A761.. stores, y preserved, clamp. */
  state->contact_kind = 3u;
  state->rec_first = rec->id;
  state->rec_first_side = (uint8_t)(rec->side & 1u);
  state->rec_first_player = rec->player < FIFA96_REF_SEVERITY_PLAYERS
                                ? rec->player
                                : 0u;
  state->rec_second = 0;
  triple[0] = point[0];
  triple[1] = point[1];
  triple[2] = point[2];
  ref_clamp_point(triple);
  state->point[0] = triple[0];
  state->point[1] = triple[1];
  state->point[2] = triple[2];
  state->sequence = FIFA96_REF_SEQ_ACT6;
  state->stage = 0;
  out->speech_code = 0x15u;
  out->sequence = FIFA96_REF_SEQ_ACT6;
  out->rec_id = rec->id;
  return 1;
}

static void ref_out_defaults(struct fifa96_ref_sequence_out *out) {
  memset(out, 0, sizeof *out);
  out->phase_write = 0xFFu;
  out->situation = 0xFFu;
}

int fifa96_ref_foul_sequence_step(struct fifa96_referee_state *state,
                                  struct fifa96_ref_sequence_out *out) {
  uint8_t side_idx;
  if (!state || !out) return -FIFA96_ERR_INVALID;
  ref_out_defaults(out);
  side_idx = ref_side_index(state, state->rec_first_side);
  /* 0x89FC8..0x89FF0: [0x158818] += [0x157A64], stage > 6 returns. */
  state->timer = (uint16_t)(state->timer + state->delta);
  switch (state->stage) {
    case 0:   /* 0x89FF8..0x8A0C9 */
      out->whistle = 1u;                                   /* 0x974DC(0x1E) */
      state->fouls_by_side[side_idx] = (uint16_t)(state->fouls_by_side[side_idx] + 1u);
      out->foul_counter = 1u;
      out->phase_write = 0x0Fu;                            /* FUN_000740A0 */
      out->phase_side = (uint8_t)((state->rec_first_side ^ 1u) & 1u);
      out->install_action = 0x16u;                         /* FUN_0007D9A4 */
      state->stage = 1u;
      state->timer = 0;
      break;
    case 1:   /* 0x8A0CA: the FUN_0004BEC8 gate, derived ready (leg 10) */
      state->stage = 2u;
      state->timer = 0;
      break;
    case 2:   /* 0x8A0FF..0x8A23E: the speech select */
      switch (state->foul_kind) {
        case 1u: out->speech_code = 0x0Du; break;
        case 2u: out->speech_code = 0x0Eu; break;
        case 3u:
          out->speech_code =
              (state->team_count[0] == 0x0Bu && state->team_count[1] == 0x0Bu)
                  ? 0x13u
                  : 0x0Fu;
          break;
        default: break;
      }
      state->stage = 3u;
      state->timer = 0;
      break;
    case 3:   /* 0x8A256: the FUN_0004BEC8 gate tail */
      state->stage = 4u;
      state->timer = 0;
      break;
    case 4: { /* 0x8A27E..0x8A366: severity sum -> 0x18 / the stage skip */
      uint8_t player = state->rec_first_player;
      if (player >= FIFA96_REF_SEVERITY_PLAYERS) {
        state->stage = 6u;                 /* hardened: the native indexes blindly */
        state->timer = 0;
        break;
      }
      uint8_t acc = (uint8_t)(state->severity[side_idx][player] + state->foul_kind);
      state->severity[side_idx][player] = acc;
      if ((acc & 0x7Fu) >= 2u) {
        out->install_action = 0x18u;
        state->stage = 5u;
      } else {
        state->stage = 6u;                 /* 0x8A35A: stage += 2 */
      }
      state->timer = 0;
      break;
    }
    case 5:   /* 0x8A367: the held gate (`[rec+0x9A] != 0`) */
      if (state->rec_first_held == 0u) return 0;   /* stall, timer keeps */
      state->team_count[state->rec_first_side & 1u] =
          (uint8_t)(state->team_count[state->rec_first_side & 1u] - 1u);
      out->team_count_dec = 1u;
      state->stage = 6u;
      state->timer = 0;
      break;
    case 6:   /* 0x8A398: the referee-object gate, then situation 0xA */
      if (state->referee_object_code == 0x48u) return 0;   /* 0x8A3A7 JZ */
      out->situation = 0x0Au;
      out->situation_side = (uint8_t)((state->rec_first_side ^ 1u) & 1u);
      state->stage = 0u;
      state->timer = 0;
      state->sequence = FIFA96_REF_SEQ_NONE;
      out->done = 1u;
      break;
    default:  /* stage > 6: the native 0x89FE5 JA return */
      break;
  }
  return 0;
}

int fifa96_ref_offside_sequence_step(struct fifa96_referee_state *state,
                                     struct fifa96_ref_sequence_out *out) {
  if (!state || !out) return -FIFA96_ERR_INVALID;
  ref_out_defaults(out);
  /* 0x89114..0x89141: timer head, stage 0/1/2 dispatch, else return. */
  state->timer = (uint16_t)(state->timer + state->delta);
  switch (state->stage) {
    case 0:   /* 0x89142..0x8918F */
      out->whistle = 1u;                       /* 0x974DC(0x1E) */
      out->phase_write = 0x0Au;                /* FUN_000740A0(0xA, 0) */
      out->phase_side = 0u;
      state->stage = 1u;
      state->timer = 0;
      break;
    case 1:   /* 0x89190: the FUN_0004BEC8 gate, derived ready (leg 10) */
      state->stage = 2u;
      state->timer = 0;
      break;
    case 2:   /* 0x891C0..0x89213: situation 9 on the opponent */
      out->situation = 9u;
      out->situation_side = (uint8_t)((state->rec_first_side ^ 1u) & 1u);
      state->stage = 0u;
      state->timer = 0;
      state->sequence = FIFA96_REF_SEQ_NONE;
      out->done = 1u;
      break;
    default:  /* stage > 2: the 0x8913D return */
      break;
  }
  return 0;
}
