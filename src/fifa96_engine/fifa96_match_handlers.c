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
 * row 1E's record-visible bodies are now consumed end to end (FU-141 §4).
 * M2 playable-match Task 2 wires the kickoff taker row 01
 * (`fifa96_match_action_01`: the native `0x7DBC0..0x7DFC8` phase-1 stage walk;
 * its situation-0xB call runs `fifa96_match_run_situation` and writes the live
 * phase 2 — FU-143 §11). Every
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
 * 0x7F194..0x7F665) with tests; M2 phase-9 T2 then wires row 05
 * (`fifa96_match_action_05`): the claim/team-target/control/camera/dirs/
 * stages/hand-off are applied over the pool, while the stage-0 target
 * algebra (0x7F3A1..0x7F57B) and the `FUN_0007F7E0` fallback stay unported
 * OL-63 residual legs (named requests). FU-142b (cluster G) wires row 26: the FU-142a arm 0x8D74D, the ported
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
#include "fifa96_engine/fifa96_match_act.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_loader/fifa96_action_handlers.h"
#include "fifa96_loader/fifa96_arm_bodies.h"
#include "fifa96_loader/fifa96_entity_update.h"
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

/* ===== FU-143 §11 (M2 playable-match Task 2 / OL-84 residual): row 01 =====
 *
 * `fifa96_match_action_01` ports the kickoff taker row (native
 * `0x7DBC0..0x7DFC8`, FU-81 §2.1; the `phase == 1` gate at `0x7DBD3`). Per
 * call:
 *  - `phase != 1` -> `0x7DFB8` = the unconditional `FUN_0007DAB4` reset
 *    (`MOV EAX,EBP; CALL 0x7DAB4`; the jump lands past the stage-3 `+0x44`
 *    test), modelled by `match_row_reset`;
 *  - marker `(int8)stage92 < 2` (`0x7DBDC`: `MOV EAX,[EBP+0x8F]; SAR
 *    EAX,0x18` — the top byte of the dword at `+0x8F` **is byte `+0x92`**,
 *    the engine's `stage92`; the never-written `record.stage` is not the
 *    native source, T2-review erratum): the `FUN_000700F4` camera
 *    reset with the constant kickoff triple `[0x10F328/2C/30]` = `(0,0,0)`
 *    (first-hand `read_memory 0x10F328`; derived as the `place_*` camera
 *    request), `[0x157A83] = rec` (`controlled`), the ±0x30 kickoff x from the
 *    current target's sign, target z = 0, and the `FUN_0007876C` merge
 *    request; otherwise the position triple is copied to the target
 *    (`0x7DC31..0x7DC39`);
 *  - `timer89 += delta` (`0x7DC3A..0x7DC50`);
 *  - the `+0x92` stage walk (table `0x7DBB0`; `> 3` -> tail):
 *    stage 0 (`0x7DC6B`): `ran = 1`; `[0x5882A] == 0` or `timer89 < 0x3C` ->
 *    tail; else `timer89 = 0`, stage 1 and fall into stage 1;
 *    stage 1 (`0x7DCAF`): the `FUN_0008DE8C` nearest of the record's team from
 *    the camera triple with skip = the record's `+0x8D` (active); the lane
 *    word `[+0x69]>>16 <= 0x40` gates for both the record and the nearest; the
 *    slot arm (`+0x20` with `word[slot+6] & 0x70`) reads the live FU-70 slot
 *    release word when this record owns the slot (the setup bind + merge now
 *    attach it, M2 interactive Task 1), otherwise the no-slot
 *    `timer89 > 0x78` arm decides; when it fires `timer89 = 0`, stage 2 and
 *    fall into stage 2;
 *    stage 2 (`0x7DD29..0x7DF95`): the nearest from the record position
 *    (`0x7DEFA`), `team+0x7B2 = nearest` (`0x7DF5B`), the conditional merge
 *    (`0x7DF61..0x7DF75`; live now that the setup bind increments `+0x828`,
 *    consumed by the frame drain), the `FUN_0008A938` situation 0xB
 *    call with the record's team side (`0x7DF7A..0x7DF90`) -> the derived
 *    live phase 2, then `timer89 = 0`, stage 3 (`0x7DF9A..0x7DFAC`).
 * The unported sinks on this path are the camera-place call body, the stage-0
 * sound event `0x974DC(0x1E)`, the stage-2 events `0x8F188`/`0x92820`, the
 * `0x8DCD4` metric, the `0x7A490` ball staging and the `0x4C380` no-op
 * (OL-84 legs). */
static void match_row_reset(struct fifa96_match_run *mr, struct fifa96_match_entity *e);

static int32_t match_row01_nearest(struct fifa96_match_run *mr, uint32_t team,
                                   int16_t from_x, int16_t from_z,
                                   uint8_t skip_index) {
  fifa96_entity_candidate candidates[FIFA96_MATCH_ENTITY_RECORDS];
  int16_t best = 0;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[team].records[i];
    candidates[i].x = (int16_t)e->pos_x;
    candidates[i].y = (int16_t)e->pos_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  return fifa96_entity_find_nearest(candidates, FIFA96_MATCH_ENTITY_RECORDS,
                                    skip_index, from_x, from_z, &best);
}

static int fifa96_match_action_01(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_team *team;
  int32_t id = r->entity_id;
  uint8_t stage;
  uint32_t t;
  uint32_t idx;
  int rc;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  t = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  idx = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  team = &mr->entities.team[t];
  if (mr->state.phase != 1u) {                   /* 0x7DBD3 JNZ 0x7DFB8 */
    match_row_reset(mr, &team->records[idx]);
    return FIFA96_OK;
  }
  if ((int8_t)r->stage92 < 2) {                  /* 0x7DBDC: [EBP+0x8F]>>24 = byte +0x92 */
    /* 0x7DBEA..0x7DC01: FUN_000700F4([0x10F328],[0x10F32C],[0x10F330], 0);
     * the constant kickoff triple is (0,0,0), so the derived FUN_000700F4
     * reset is the zero place request. */
    r->place_x = 0;
    r->place_y = 0;
    r->place_z = 0;
    r->place_valid = 1;
    r->controlled = 1;                           /* 0x7DC18 [0x157A83] = rec */
    r->target_x = r->target_x < 0 ? -0x30 : 0x30; /* 0x7DC06..0x7DC1E */
    r->target_z = 0;                             /* 0x7DC23 */
    /* 0x7DC2A CALL 0x7876C. The native helper no-ops while the requester
     * already holds the slot (the `requester->has_slot != 0` early return in
     * `fifa96_match_entities_merge_slot`); the engine defers the request to
     * the frame drain, which runs after this dispatch's 0x7DF61 stage-2 merge
     * (`0x7DEFA`), so the native call-site precondition is evaluated here. */
    r->helper_request = r->has_slot == 0 ? 1u : 0u;
  } else {                                       /* 0x7DC31..0x7DC39 */
    r->target_x = r->pos_x;
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
  }
  r->timer89 += r->delta;                        /* 0x7DC3A..0x7DC50 */
  stage = r->stage92;
  if (stage == 0u) {
    r->ran = 1;                                  /* 0x7DC71 */
    if (mr->global_5882a == 0) return FIFA96_OK;              /* 0x7DC7A */
    if (r->timer89 < 0x3C) return FIFA96_OK;                  /* 0x7DC87 */
    r->timer89 = 0;                              /* 0x7DC9D */
    r->stage92 = 1u;                             /* 0x7DCA9 */
    stage = 1u;                                  /* falls into 0x7DCAF */
  }
  if (stage == 1u) {
    int16_t lane = (int16_t)(r->lane >> 16);     /* 0x7DCCC [+0x69]>>16 */
    int16_t near_lane = 0;
    int32_t near = match_row01_nearest(mr, t, (int16_t)mr->render.camera.pos_x,
                                       (int16_t)mr->render.camera.pos_z,
                                       r->active);      /* 0x7DCC7 */
    uint8_t ready = 0;
    if (near >= 0) near_lane = (int16_t)(team->records[near].lane >> 16);
    if (near >= 0 && lane <= 0x40 && near_lane <= 0x40) {   /* 0x7DCD5/0x7DCE0 */
      if (r->has_slot != 0) {
        /* 0x7DCE9..0x7DCF6: `word[slot+6] & 0x70` — the FU-70 §1.2 release
         * word. M2 interactive Task 1 binds the live slot (setup bind +
         * `FUN_0007876C` merge), so the arm now fires on a released button
         * exactly as the native; the pre-bind staged-zero stand-in that made
         * this unreachable is gone. Guarded on the slot being this record's. */
        if (mr->slot.entity == id && (mr->slot.released & 0x70u) != 0u)
          ready = 1;
      } else if (r->timer89 > 0x78) {            /* 0x7DCFA..0x7DD03 */
        ready = 1;
      }
    }
    if (!ready) return FIFA96_OK;                /* 0x7DD0B */
    r->timer89 = 0;
    r->stage92 = 2u;
    stage = 2u;                                  /* falls into 0x7DD29 */
  }
  if (stage == 2u) {
    int32_t near = match_row01_nearest(mr, t, (int16_t)r->pos_x,
                                       (int16_t)r->pos_z, r->active); /* 0x7DF0B */
    if (near >= 0)
      team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)near);
    /* 0x7DF61..0x7DF75: merge when the nearest holds no slot and `+0x828` is
     * set. M2 interactive Task 1: the setup bind increments `+0x828`, so this
     * arm is live; the recorded merge is consumed by the frame drain
     * (`match_run_entity_drain`) and rebinds the FU-70 slot to the nearest. */
    if (near >= 0 && team->slot_pool != 0 && team->records[near].has_slot == 0)
      (void)fifa96_match_entities_merge_slot(&mr->entities, t, (uint32_t)near);
    rc = fifa96_match_run_situation(mr, 0x0Bu);  /* 0x7DF90 */
    if (rc != 0) return rc;
    r->timer89 = 0;                              /* 0x7DFA0 */
    r->stage92 = 3u;                             /* 0x7DFAC */
  }
  return FIFA96_OK;
}

/* ===== M2 phase-9 T1 / FU-75 L4.6 (FU-138 OL-18): row 02 restart/placement ==
 *
 * `fifa96_match_action_02` ports the row-02 state machine (native
 * `0x7DFCC..0x7E1A2`, first-hand `disassemble_bytes` this task; FU-77 §2.2):
 *  - `0x7DFD7` `byte[+0x9E] = 1` (ran);
 *  - phase 1 (`0x7DFE6..0x7E016`): `EAX = [[rec]+0x7B2]` (the team's
 *    `[team+0x7B2]` controlled entity), `EAX = dword[EAX+0x59]`, target z dword
 *    `+0x55 = 0`, `+0x89 = 0`, `byte[+0x92] = 0`, `NEG EAX`,
 *    `dword[+0x4D] = EAX` (the tested
 *    `fifa96_action_locomotion_restart_target` mirror; the y dword `+0x51` is
 *    untouched), then the tail return;
 *  - phase 2 (`0x7E01B`): `dword[team+0x7B2] = rec`; with no slot and
 *    `byte[team+0x828] != 0` the `FUN_0007876C` merge request
 *    (`0x7E02D..0x7E045`); with a slot `ECX=1, EDX=4, EBX=0,
 *    CALL 0x7D9A4` and a direct return (`0x7E046..0x7E068`) — install 4
 *    invoke-now; without a slot the camera triple `0x15774C` target, the
 *    dword `+0x89 += delta` and the `+0x92` stage dispatch
 *    (`0x7E069..0x7E09C`): stage 0 waits per
 *    `fifa96_action_locomotion_restart_wait` (`lane > 0x40 ? 0x78 : 0xA`,
 *    `0x7E0B0..0x7E0D8`), the ready `lane > 0x40` path runs `FUN_0007DAB4`
 *    first (`0x7E0C8`), then the shared advance zeroes `+0x89` and increments
 *    `byte[+0x92]` (`0x7E0DE..0x7E0F5`, which falls into the stage-1 arm);
 *  - any other phase (`0x7E192`): `FUN_0007DAB4` reset.
 * The stage-1 arm (`0x7E0F6..0x7E184`: the stack position copy, the side
 * `+/-0x1E0` z offset, the `0x8DE8C` nearest, the `0x8DCD4` metric into
 * 0x158738, the `0x92820` event and the `0x7A490` ball staging) and the
 * stage-2 snap (`0x7E185`, `0x79B1C` plus the `+0x44` reset) stay OL-18 legs;
 * they write no `rec+0x4D` target, so the camera target survives the leg. The
 * `FUN_0007876C` merge rides `helper_request` (the native immediate call; the
 * frame drain runs it, the row-01 convention). */
static int fifa96_match_action_02(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_team *team;
  struct fifa96_match_entity *e;
  int32_t id = r->entity_id;
  uint32_t t;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  t = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  team = &mr->entities.team[t];
  e = &team->records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
  r->ran = 1;                                  /* 0x7DFD7 */
  if (mr->state.phase == 1u) {                 /* 0x7DFE6 */
    int32_t controlled_x = 0;
    int32_t cid = team->target;                /* [team+0x7B2] */
    if (cid >= 0 &&
        cid < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
      controlled_x =
          mr->entities.team[(uint32_t)cid / FIFA96_MATCH_ENTITY_RECORDS]
              .records[(uint32_t)cid % FIFA96_MATCH_ENTITY_RECORDS]
              .pos_x;                          /* dword [controlled+0x59] */
    (void)fifa96_action_locomotion_restart_target(
        controlled_x, &r->target_x, &r->target_z);   /* 0x7DFF7..0x7E013 */
    r->timer89 = 0;                            /* 0x7DFFE */
    r->stage92 = 0;                            /* 0x7E008 */
    return FIFA96_OK;                          /* 0x7E016 -> tail */
  }
  if (mr->state.phase != 2u) {                 /* 0x7E01B -> 0x7E192 */
    match_row_reset(mr, e);
    return FIFA96_OK;
  }
  team->target = id;                           /* 0x7E027 [team+0x7B2] = rec */
  if (r->has_slot == 0) {                      /* 0x7E02D */
    if (team->slot_pool != 0)                  /* 0x7E036 byte[team+0x828] */
      r->helper_request = 1;                   /* 0x7E041 CALL 0x7876C */
  }
  if (r->has_slot != 0) {                      /* 0x7E046 */
    if (fifa96_match_entities_install(e, 2u, 4u, 0) == 1) {
      /* 0x7E04C..0x7E05A: install 4 invoke-now; re-stage the installer's
       * writes so the invoked row sees them (the T3 seam's invoke pair). */
      r->code = e->code;
      r->stage92 = e->stage92;
      r->timer89 = e->timer89;
      r->ran = e->ran;
      (void)fifa96_match_dispatch_action(mr, 4u);
    }
    return FIFA96_OK;                          /* 0x7E05F..0x7E068 */
  }
  r->target_x = mr->render.camera.pos_x;       /* 0x7E069..0x7E089 */
  r->target_y = mr->render.camera.pos_y;
  r->target_z = mr->render.camera.pos_z;
  r->timer89 += r->delta;                      /* 0x7E06B..0x7E081 */
  if ((uint8_t)r->stage92 == 0u) {             /* 0x7E08A..0x7E0AA */
    uint8_t ready = 0;
    uint8_t reset = 0;
    (void)fifa96_action_locomotion_restart_wait((int16_t)(r->lane >> 16),
                                                r->timer89, &ready, &reset);
    if (!ready) return FIFA96_OK;              /* 0x7E199 */
    if (reset != 0) match_row_reset(mr, e);    /* 0x7E0C8 FUN_0007DAB4 */
    r->timer89 = 0;                            /* 0x7E0DE */
    r->stage92 = (uint8_t)(r->stage92 + 1u);   /* 0x7E0EE..0x7E0F0 */
  }
  return FIFA96_OK;                            /* stage 1/2 arms OL-18 */
}

/* ===== FU-151 P3 (M2 phase-7 Task 3): the two full keeper machines =========
 *
 * Row 1E (`0x7550C`, stage table `0x754E4`, ten stages) and row 1D
 * (`0x74EB0`, stage table `0x74E9C`, five stages) are ported as the tested
 * loader machines `fifa96_keeper_claim_step`/`fifa96_keeper_closedown_step`
 * (FU-151 §Port contract items 1/2; FU-79 §6/§7 for 1D stages 0-2 and the
 * claim head; FU-151 §2.4/§2.5 for the rest, all first-hand this slice).
 * These binders stage one pool record plus the run's keeper process cells
 * (0x15774C/50/54 focus, 0x157A77 reset triple, 0x157C30 vector, 0x157C36
 * saved point, 0x157C42 gauge, [0x157AB2] latch), run one dispatch and apply
 * the bounded requests: camera place -> `place_*` (the FU-71 reset drain),
 * slot merge -> `helper_request` (FUN_0007876C), actor -> `controlled`
 * ([0x157A83]), install -> `record.install` (FUN_0007D9A4 drain), reset ->
 * `match_row_reset` (FUN_0007DAB4), restart -> `fifa96_match_run_situation`
 * (the shared situation-0xB entry; phase 2). The unported sinks (0x744D4
 * slot fill, 0x74CDC guard, 0x79B1C snap, 0x7B878 vector build, 0x74E2C
 * clear vector body callers, 0x36200/0x361A4/0x361B0/0x4C31C/0x4C320/0x4C380
 * UI/audio, 0x92820/0x71C94, 0x7A490 staging, 0x8DE8C+0x786A0 hand-off,
 * 0x8F188 ring, the stage-6 [0x157820]/[0x157822] consumers) stay request
 * bits on the loader out structs (FU-151 §5 legs 3/4/12/13/15); the engine
 * records only the stage-6 flags on the run. The `+0x8E` sector write-back
 * lands on `record.type` (the FU-151 erratum: byte +0x8E is the face octant;
 * the action code +0x91 stays `record.code`). */
static struct fifa96_match_entity *match_keeper_entity(struct fifa96_match_run *mr,
                                                       struct fifa96_match_team **team_out) {
  int32_t id = mr->record.entity_id;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return NULL;
  if (team_out != NULL)
    *team_out = &mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS];
  return &mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS]
              .records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
}

static void match_keeper_candidates(struct fifa96_match_team *team,
                                    fifa96_entity_candidate *mates) {
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &team->records[i];
    mates[i].x = (int16_t)e->pos_x;
    mates[i].y = (int16_t)e->pos_z;
    mates[i].skip_98 = e->skip_98;
    mates[i].skip_9a = e->skip_9a;
  }
}

static void match_keeper_claim_stage(struct fifa96_match_run *mr,
                                     struct fifa96_match_team *team,
                                     fifa96_entity_candidate *mates,
                                     fifa96_keeper_claim *s) {
  memset(s, 0, sizeof *s);
  match_keeper_candidates(team, mates);
  s->pos.x = mr->record.pos_x;
  s->pos.y = mr->record.pos_y;
  s->pos.z = mr->record.pos_z;
  s->target_x = mr->record.target_x;
  s->target_z = mr->record.target_z;
  s->cam_x = mr->keeper_cam_x;
  s->cam_y = mr->keeper_cam_y;
  s->cam_z = mr->keeper_cam_z;
  s->reset_x = mr->keeper_reset_x;
  s->reset_y = mr->keeper_reset_y;
  s->reset_z = mr->keeper_reset_z;
  s->vec_band = mr->keeper_vec_band;
  s->vec_dx = mr->keeper_vec_dx;
  s->vec_dz = mr->keeper_vec_dz;
  s->saved_x = mr->keeper_saved_x;
  s->saved_z = mr->keeper_saved_z;
  s->gauge = mr->keeper_gauge;
  s->latch_157ab2 = mr->keeper_latch_157ab2;
  s->timer89 = mr->record.timer89;
  s->delta = mr->record.delta;
  s->stage92 = mr->record.stage92;
  s->side = team->side;
  s->has_ball = mr->record.has_ball;
  s->has_slot = mr->record.has_slot;
  s->slot_edge = (uint8_t)(mr->record.has_slot != 0 &&
                                   mr->slot.entity == mr->record.entity_id
                               ? mr->slot.released
                               : 0);
  s->slot_dir_x = mr->record.dir_x;
  s->slot_dir_z = mr->record.dir_z;
  s->sector = mr->record.type;
  s->frame = mr->record.frame;
  s->anim_row = mr->record.anim_id;
  s->row44 = mr->record.row44;
  s->vel71_nonzero = mr->record.speed71 != 0 ? 1u : 0u;
  s->offset_x = mr->record.place_offset_x;
  s->offset_z = mr->record.place_offset_z;
  s->mates = mates;
  s->mate_count = FIFA96_MATCH_ENTITY_RECORDS;
  s->skip_index = mr->record.active;   /* [rec+0x8A]>>24 = byte +0x8D */
  s->range_attr = 0;                   /* rec[+4][+0xD] unmodeled (leg) */
  s->rng = &mr->rng;
}

static int match_keeper_claim_apply(struct fifa96_match_run *mr,
                                    struct fifa96_match_entity *e,
                                    const fifa96_keeper_claim *s,
                                    const fifa96_keeper_claim_out *out) {
  mr->record.stage92 = s->stage92;
  mr->record.timer89 = s->timer89;
  mr->record.timer7b = (uint16_t)s->timer7b;
  mr->record.has_ball = s->has_ball;
  mr->record.target_x = s->target_x;
  mr->record.target_z = s->target_z;
  mr->record.type = s->sector;
  if (out->ran != 0) mr->record.ran = 1;
  mr->record.helper_request = out->helper != 0 ? 1u : 0u;
  if (out->controlled != 0) mr->record.controlled = 1;
  if (out->install != 0) mr->record.install = out->install;
  mr->record.place_valid = out->place != 0 ? 1u : 0u;
  if (out->place != 0) {
    mr->record.place_x = out->place_x;
    mr->record.place_y = out->place_y;
    mr->record.place_z = out->place_z;
  }
  mr->keeper_cam_x = s->cam_x;
  mr->keeper_cam_y = s->cam_y;
  mr->keeper_cam_z = s->cam_z;
  mr->keeper_reset_x = s->reset_x;
  mr->keeper_reset_y = s->reset_y;
  mr->keeper_reset_z = s->reset_z;
  mr->keeper_vec_band = s->vec_band;
  mr->keeper_vec_dx = s->vec_dx;
  mr->keeper_vec_dz = s->vec_dz;
  mr->keeper_saved_x = s->saved_x;
  mr->keeper_saved_z = s->saved_z;
  mr->keeper_gauge = s->gauge;
  mr->keeper_latch_157ab2 = s->latch_157ab2;
  if (out->flag_write != 0) {
    mr->flag_157820 = out->flag_157820;
    mr->flag_157822 = out->flag_157822;
  }
  /* M2 phase-9 T3 (FU-148 §2.1(c) 0x75DC4): the stage-5 release
   * (0x75D17..0x75DD1) cuts the FU-71 camera to the staged release triple
   * (0x15774C/50/54 = pos + 0x10F331/39[type8]<<6, y 0x50), calls
   * FUN_00092820(rec, 5) then FUN_00071C94(rec, zero-vec, EBX = word[0x157750]
   * = 0x50, ECX 0) and writes `[0x157A83] = rec` + `rec+0x9B = 0`. The
   * `released`/`scenario` pair is the stage-5-only arm (stage 7's `scenario`
   * carries the 0x92820(4) staging without a FUN_00071C94 call). */
  if (out->scenario != 0 && out->released != 0) {
    mr->render.camera.pos_x = s->cam_x;
    mr->render.camera.pos_y = s->cam_y;
    mr->render.camera.pos_z = s->cam_z;
    {
      int rc = fifa96_camera_event_set(&mr->render.camera, 0, 0, 0x50, 0);
      if (rc < 0) return rc;
    }
    (void)fifa96_camera_set_tracked(&mr->render.camera, mr->record.entity_id);
  }
  if (out->reset != 0 && e != NULL) match_row_reset(mr, e);
  if (out->situation_0b != 0) return fifa96_match_run_situation(mr, 0x0Bu);
  return FIFA96_OK;
}

static int fifa96_match_action_1E(struct fifa96_match_run *mr) {
  fifa96_keeper_claim s;
  fifa96_keeper_claim_out out;
  fifa96_entity_candidate mates[FIFA96_MATCH_ENTITY_RECORDS];
  struct fifa96_match_team *team = &mr->entities.team[0];
  struct fifa96_match_entity *e = match_keeper_entity(mr, &team);
  int rc;
  match_keeper_claim_stage(mr, team, mates, &s);
  rc = fifa96_keeper_claim_step(&s, &out);
  if (rc != FIFA96_OK) return rc;
  return match_keeper_claim_apply(mr, e, &s, &out);
}

static void match_keeper_closedown_stage(struct fifa96_match_run *mr,
                                         struct fifa96_match_team *team,
                                         fifa96_entity_candidate *mates,
                                         fifa96_keeper_closedown *s) {
  memset(s, 0, sizeof *s);
  match_keeper_candidates(team, mates);
  s->pos.x = mr->record.pos_x;
  s->pos.y = mr->record.pos_y;
  s->pos.z = mr->record.pos_z;
  s->target_x = mr->record.target_x;
  s->target_z = mr->record.target_z;
  s->cam_x = mr->keeper_cam_x;
  s->cam_y = mr->keeper_cam_y;
  s->cam_z = mr->keeper_cam_z;
  s->reset_x = mr->keeper_reset_x;
  s->reset_y = mr->keeper_reset_y;
  s->reset_z = mr->keeper_reset_z;
  s->vec.distance = mr->keeper_vec_band;
  s->vec.dx = mr->keeper_vec_dx;
  s->vec.dz = mr->keeper_vec_dz;
  s->timer89 = mr->record.timer89;
  s->delta = mr->record.delta;
  s->lane = mr->record.lane;
  s->stage92 = mr->record.stage92;
  s->side = team->side;
  s->has_ball = mr->record.has_ball;
  s->has_slot = mr->record.has_slot;
  s->slot_edge = (uint8_t)(mr->record.has_slot != 0 &&
                                   mr->slot.entity == mr->record.entity_id
                               ? mr->slot.released
                               : 0);
  s->slot_pressed = (uint8_t)(mr->record.has_slot != 0 &&
                                      mr->slot.entity == mr->record.entity_id
                                  ? mr->slot.pressed
                                  : 0);
  s->sector = mr->record.type;
  s->row44 = mr->record.row44;
  s->session_gate = mr->session_gate_14c32a;
  s->latch_157ab2 = mr->keeper_latch_157ab2;
  s->offset_x = mr->record.place_offset_x;
  s->offset_z = mr->record.place_offset_z;
  s->cam_off_z = 0;   /* the 0x157C5E per-side table (producer unported, leg) */
  s->mates = mates;
  s->mate_count = FIFA96_MATCH_ENTITY_RECORDS;
  s->skip_index = mr->record.active;
  s->range_attr = 0;
  s->rng = &mr->rng;
}

static int match_keeper_closedown_apply(struct fifa96_match_run *mr,
                                        struct fifa96_match_entity *e,
                                        const fifa96_keeper_closedown *s,
                                        const fifa96_keeper_closedown_out *out) {
  mr->record.pos_x = s->pos.x;
  mr->record.pos_y = s->pos.y;
  mr->record.pos_z = s->pos.z;
  mr->record.target_x = s->target_x;
  mr->record.target_z = s->target_z;
  mr->record.stage92 = s->stage92;
  mr->record.timer89 = s->timer89;
  mr->record.type = s->sector;
  if (out->ran != 0) mr->record.ran = 1;
  mr->record.helper_request = out->helper != 0 ? 1u : 0u;
  if (out->controlled != 0) mr->record.controlled = 1;
  mr->record.place_valid = out->place != 0 ? 1u : 0u;
  if (out->place != 0) {
    mr->record.place_x = out->place_x;
    mr->record.place_y = out->place_y;
    mr->record.place_z = out->place_z;
  }
  if (out->commit != 0 && e != NULL) (void)fifa96_match_entities_place(e);
  mr->keeper_cam_x = s->cam_x;
  mr->keeper_cam_y = s->cam_y;
  mr->keeper_cam_z = s->cam_z;
  mr->keeper_reset_x = s->reset_x;
  mr->keeper_reset_y = s->reset_y;
  mr->keeper_reset_z = s->reset_z;
  mr->keeper_vec_band = s->vec.distance;
  mr->keeper_vec_dx = s->vec.dx;
  mr->keeper_vec_dz = s->vec.dz;
  mr->keeper_latch_157ab2 = s->latch_157ab2;
  if (out->reset != 0 && e != NULL) match_row_reset(mr, e);
  if (out->situation_0b != 0) return fifa96_match_run_situation(mr, 0x0Bu);
  return FIFA96_OK;
}

static int fifa96_match_action_1D(struct fifa96_match_run *mr) {
  fifa96_keeper_closedown s;
  fifa96_keeper_closedown_out out;
  fifa96_entity_candidate mates[FIFA96_MATCH_ENTITY_RECORDS];
  struct fifa96_match_team *team = &mr->entities.team[0];
  struct fifa96_match_entity *e = match_keeper_entity(mr, &team);
  int rc;
  match_keeper_closedown_stage(mr, team, mates, &s);
  rc = fifa96_keeper_closedown_step(&s, &out);
  if (rc != FIFA96_OK) return rc;
  return match_keeper_closedown_apply(mr, e, &s, &out);
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
  rec.anim_sel = mr->record.anim_id;   /* OL-80: byte[[rec+0x28]] live id */
  rec.rng = &mr->rng;
  rc = fifa96_arm_28_step(&rec, mr->record.stage92);
  if (rc != FIFA96_OK) return rc;
  mr->record.anim_id = rec.anim_sel;   /* OL-80: selector write-back */
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
  rec.anim_sel = mr->record.anim_id;   /* OL-80: byte[[rec+0x28]] live id */
  rec.rng = &mr->rng;
  rc = fifa96_arm_2a_step(&rec, mr->record.stage92);
  if (rc != FIFA96_OK) return rc;
  mr->record.anim_id = rec.anim_sel;   /* OL-80: selector write-back */
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
  /* FU-159 §4.1/T3: +0x44 is the current anim row's flags bit 0 (derived by
   * the dispatch staging); the kick-07 stage-2 reset gates on it. */
  s->byte44 = r->row44;
  /* T3: the native stage-1 mode source is the live `word[slot+6]` (the FU-70
   * released word; `0x7BA29` latches the mode into it, `0x82CAC` reads it for
   * the row-0F kick). The slot belongs to this record only when it is the
   * bound one; other records carry no slot. */
  s->slot_word6 = (mr->slot.entity == r->entity_id) ? (int16_t)mr->slot.released : 0;
  s->side = mr->entities.team[team].side;
  s->timer89 = r->timer89;
  s->timer81 = r->timer81;
  s->delta = r->delta;
  s->pos_x = r->pos_x;
  s->pos_y = r->pos_y;
  s->pos_y_word = (int16_t)r->pos_y;
  s->pos_z = r->pos_z;
  s->lane_word = (int16_t)r->lane;
  /* M2 phase-11 T1 (FU-148 §14.5 / L14.7): the native event's ball height is
   * `word[0x157750]` (FUN_0007AE70 0x7AEFD `MOV DX,[0x157750]; CX=[rec+0x5D];
   * SUB EDX,ECX`). The camera Y is that word's producer (the T1 port stores
   * it in `camera.pos_y`), so the band now reads the live camera height, not
   * the FU-75 §1.6 ball-record stand-in. */
  s->ball_height = (int16_t)mr->render.camera.pos_y;
  /* Pool-derived team/target state; the native +0x44/+0x85/+0x87 bytes, the
   * 0x1577CA exclusion and the roster descriptor are unported (OL-65), and the
   * 0x157A4D/0x14C2F6/0x14C326/0x14C32A mode gates default to their
   * zero-image values (OL-66). */
  {
    int32_t id = r->entity_id;
    int32_t tid = mr->entities.team[team].target;
    int32_t sid = mr->entities.team[team].second;
    int32_t oid = mr->entities.team[1u - team].target;
    s->staged_code = (uint8_t)mr->entities.ball.pair.code;
    s->is_team_target = (id >= 0 && id == tid) ? 1u : 0u;
    s->is_team_second = (id >= 0 && id == sid) ? 1u : 0u;
    /* 0x815D0 `CMP EBP,[EAX+0x7CB]`: the code-7 stage-1 long-ball arm asks
     * whether this record is the team's +0x7CB long-ball callback (set by
     * FUN_0007CD60, M2 phase-11 T1 / FU-75 §1.4). The native compares record
     * pointers, and a record pointer is never NULL, so `[team+0x7CB] == 0`
     * (no request armed) can never select the 0x40 mode — the `!= 0` guard
     * is the native NULL comparison (final review: team-0 record-0 would
     * otherwise alias the 0 = none sentinel). */
    s->is_team_cb = (id >= 0 && mr->entities.team[team].timer7cb != 0 &&
                     (uint32_t)id == mr->entities.team[team].timer7cb)
                        ? 1u
                        : 0u;
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
      /* FU185-L1 (phase-16 T2): the native row-07 opponent gate reads the
       * record's action **code** byte `+0x91` (`0x81683 MOV EAX,[ESI+0x8E];
       * SAR EAX,0x18` = byte +0x91), not the `+0x8E` face octant. The pool
       * `code` field is that byte (the same mapping the acting record uses at
       * `s->type = r->code`). */
      s->opp_type = opp->code;
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
  int rc;
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
  rc = fifa96_ball_kick_target(&mr->entities.ball.pair, &a, &slot, input, &ctx,
                               &mr->rng, mode, out);
  if (rc != FIFA96_OK) return rc;
  /* M2 phase-10 T1 (FU-148 §14 / FU-142 OL-88): the staged kick's receive arm
   * — the native FUN_0007A490 tail's 0x7A8C7/0x7A8EF `FUN_0007A084` call,
   * gated by the code-keyed receive flag the tail reports. Runs the camera
   * event/tracked bind (the high-ball follow producer), the receiver scan and
   * the team-target writes from the staged snapshot (the call point's block,
   * before the native 0x7A934 inactive clear); a cleared tail re-applies the
   * whole-block reset after, matching the native order. */
  if (out->receive != 0) {
    rc = fifa96_match_run_ball_receive(mr, out->receive_actor,
                                       &out->receive_vector, out->receive_traj);
    if (rc != FIFA96_OK) return rc;
    if (out->cleared != 0)
      (void)fifa96_ball_pair_clear(&mr->entities.ball.pair);
  }
  return FIFA96_OK;
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

/* ===== FU-149 L13 (M2 phase-8 T1): the set-piece taker row machines =======
 *
 * The four rows the P1 set-piece arms install (FU-149 §6 codes): 0x10 throw-in
 * taker (`0x855F0..0x85DE3`, gate {2,3}, stage table 0x855B8, 7 arms), 0x11
 * corner taker (`0x85DE4..0x864FF`, gate {2,4}, table 0x85DA0, 10 arms), 0x12
 * free-kick taker (`0x83D68..0x84597`, gate {2,7}, tables 0x83D2C/0x83D4C,
 * 8/7 arms) and 0x13 penalty taker (`0x84B00..0x84EEB`, gate {2,6}, table
 * 0x84AE4, 7 arms). Each arm is cited by its first-hand window in the body
 * comments. The ported subset carries every gate, the record/camera-visible
 * writes, the RNG draws and both resolution exits (the shared situation-0xB
 * phase-2 hand-back and row 0x10's situation-2 re-queue); the presentation and
 * staging bodies (`FUN_000832A8`, the 0x158730/0x15879C/0x1587AC throw block,
 * `FUN_00083428`'s input-driven slot arm, the 0x8F188/0x974DC/0x6E598/0x4C3xx/
 * 0x918CC sinks, the kick event sub-tables and the 0x10F331/0x10F339 ball-line
 * block) are numbered L13 legs.
 *
 * Documented stand-ins: the `0x15777C` snapshot triple = `mr->goal_snap_*`
 * (the FU-145 armer's frozen pan triple), the `0x15774C` focus = the engine
 * camera pos (`0x157764` = its target, FU-147 leg 13), the `0x157770`
 * camera-led reception triple = the camera triple (FU-67 §4.1), the `0x8DE8C`
 * picks = `fifa96_entity_find_nearest` over the pool record positions, the
 * `0x8DCD4` triple = the FU-142 helper layout {distance, dx, dz}, and the
 * `0xCD474`/`0x114E04`/`0x795A4` angle idiom = `fifa96_action_kick_angle` +
 * `fifa96_ball_fold`. */
static struct fifa96_match_entity *match_sp_entity(struct fifa96_match_run *mr,
                                                   uint32_t *team_out,
                                                   uint32_t *idx_out) {
  int32_t id = mr->record.entity_id;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return NULL;
  if (team_out != NULL) *team_out = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  if (idx_out != NULL) *idx_out = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  return &mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS]
              .records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
}

/* The shared camera stand-in triple (`[0x15779x]`): row 0x0E's stage-0/1
 * hatches both copy it (native per-hatch `MOV` triples at `0x8276A..` and
 * `0x827B1..`); named so the two arms cannot drift. */
static void match_target_from_camera(const struct fifa96_match_run *mr,
                                     struct fifa96_match_run_record *r) {
  r->target_x = mr->render.camera.pos_x;
  r->target_y = mr->render.camera.pos_y;
  r->target_z = mr->render.camera.pos_z;
}

/* FU-167 §3 / FU-168 §2.2 (phase-14 T1): the row-0x0E stage-2 **else arm**
 * (`0x82820..0x82A9E`, fresh `disassemble_bytes` this task). Returns 1 when
 * the body completed — the native `CL = 1` (`0x82A95`) fall-through into the
 * advance — and 0 when an in-body gate failed (the `0x82A9C` tail: the record
 * stays at stage 2 and retries).
 *
 * Gates: `0x20 < (int32)[0x157750] < 0x70` (`0x82828..0x82834`, the camera Y)
 * and `(int16)word[+0x6B] < 0x60` (`0x8283a..0x82843`). The lane read is the
 * **live +0x6B word**, the pool `lane_x`: the FU-168 §9.4 erratum allows
 * either that or `r->lane >> 16`, and only `lane_x` is refreshed by the BF20
 * lane block (`match_run.c:1444` writes `e->lane_x`; the `e->lane` dword's
 * high word is a separate stale view — FU168-L7(d)).
 *
 * Payload (every cell per FU-167 §4): the focus re-placement
 * `rec.pos + 0x40*0x10F334/3C[type]` — the direct `[0x15774C]/[0x157754]`
 * store, **not** the `place_*` drain, whose same-frame `camera_init` clears
 * `tracked` (FU-168 §7.2 hazard); the slot-dir/RNG lateral seed
 * (`0x82899..0x828CE`); the `0x8DCD4` dx/dz pair (its octagonal distance is
 * dead, FU-167 §3.2) and the `0xCD474` angle turned ±0x100 about `face7d`
 * (`0x82902..0x82956`); the amplitude (`0x82964..0x829CB`); the polar vector
 * (`0x829D2..0x82A43`); then `FUN_00071C94` (`0x82A6C`) through the split
 * engine pair (`fifa96_camera_event_set` + `fifa96_camera_set_tracked`, the
 * row-04/receive convention at `handlers.c:2540-2546`), which is the
 * **live follow trigger** (`[0x1577CA] = rec`). The native also clears the
 * record's `+0x9B` on the bind path (`0x71D2D`).
 *
 * Legs: the `0x92820(rec,0x0B)` replay latch, the `0x8F188` event-0x6C
 * queue and the `0x651F0` x2 / `0x974F0` sinks (FU167-L3); the descriptor
 * term `[[rec+4]+0x15]<<6` (FU167-L1, stand-in 0); the `[0x157815]` camera
 * direction byte (FU167-L2); the `0x71C94` tail cells (FU167-L5). */
static int match_sp_pan_arm(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_entity *e = match_sp_entity(mr, NULL, NULL);
  fifa96_camera *cam = &mr->render.camera;
  int32_t cam_y;
  int32_t focus_x, focus_z;
  int32_t target_x, target_z;
  int32_t dx, dz, angle = 0, turn, sum, height_factor, amp;
  int16_t amp_word, vec_x, vec_z;
  uint16_t draw = 0;
  int ev;
  if (e == NULL) return 0;
  cam_y = cam->pos_y;
  if (cam_y <= 0x20 || cam_y >= 0x70) return 0;        /* 0x82828..0x82834 */
  if ((int16_t)e->lane_x >= 0x60) return 0;            /* 0x8283a..0x82843 */
  /* 0x82849..0x82893: focus = rec.pos + 0x40*(int8)dir[type]. */
  focus_x = r->pos_x + (int32_t)fifa96_match_dir_x[r->type & 7u] * 0x40;
  focus_z = r->pos_z + (int32_t)fifa96_match_dir_z[r->type & 7u] * 0x40;
  cam->pos_x = focus_x;
  cam->pos_z = focus_z;
  /* 0x82899..0x828CE: the lateral target x — slot dir << 7 when the slot is
   * present with a nonzero dir pair, else 0x80 - (RNG & 0xFF). */
  if (r->has_slot != 0u && (r->dir_x != 0 || r->dir_z != 0)) {
    target_x = (int32_t)r->dir_x << 7;
  } else {
    if (fifa96_rng_step(&mr->rng, &draw) != FIFA96_OK) return 0;
    target_x = 0x80 - (int32_t)(draw & 0xFF);
  }
  /* 0x828D1..0x828E9: target z = rec.pos.z ± 0x1E0 by the record's team side
   * (`[[rec]+0x826]`, the pool `side`). */
  target_z = r->pos_z + (r->side == 0u ? 0x1E0 : -0x1E0);
  /* 0x82902..0x82910: the metric's dx/dz words (16-bit truncation). */
  dx = (int16_t)(uint16_t)((uint16_t)target_x - (uint16_t)focus_x);
  dz = (int16_t)(uint16_t)((uint16_t)target_z - (uint16_t)focus_z);
  if (fifa96_entity_angle(dx, dz, &angle) != FIFA96_OK) return 0;
  /* 0x82915..0x8295E: the ±0x100 turn clamp around face7d (0x3FF space). */
  turn = (int32_t)(uint16_t)((uint16_t)angle - (uint16_t)r->face7d) & 0x3FF;
  if (turn > 0x200) turn -= 0x400;
  if (turn > 0x100) turn = 0x100;
  else if (turn < -0x100) turn = -0x100;
  turn = (turn + (int32_t)r->face7d) & 0x3FF;
  if (turn > 0x200) turn -= 0x400;
  /* 0x82964..0x829CB: the amplitude. The low-word clamp/compare order is the
   * native's (`MOVSX EDX,BX` on the 32-bit sum; the product's low word is
   * what the multiply keeps). */
  sum = (int32_t)(int16_t)(uint16_t)(2u * (uint16_t)r->speed71 +
                                     (uint16_t)cam->speed);
  if (sum > 0x1E) sum = 0x1E;
  height_factor = 0x30 - cam_y;
  if (height_factor < 0) height_factor = -height_factor;
  height_factor = 0x60 - height_factor;
  if ((int16_t)height_factor < 0x0A) height_factor = 0x0A;
  amp = (int16_t)(uint16_t)((uint16_t)sum * (uint16_t)height_factor);
  /* FU167-L1: the `[[rec+4]+0x15]<<6` descriptor term is 0 (unmodeled). */
  if (amp > 0x780) amp = 0x780;
  amp_word = (int16_t)amp;
  /* 0x829D2..0x82A43: the polar vector (the `0x795A4` products). */
  vec_x = (int16_t)(((int64_t)amp_word * fifa96_entity_sine(turn) + 0x8000) >> 16);
  vec_z = (int16_t)(((int64_t)amp_word * fifa96_entity_sine(turn + 0x100) + 0x8000) >> 16);
  /* 0x82A4A: the 0x92820(rec,0x0B) replay-latch request (FU167-L3). */
  if (fifa96_rng_step(&mr->rng, &draw) != FIFA96_OK) return 0;
  /* 0x82A4F..0x82A6C: height = (draw & 0x3F) + camera Y (word) and the
   * FUN_00071C94 event. The native's vector word 0 (the amplitude, stored at
   * 0x1577B8) has no engine consumer (FU167-L4). */
  ev = fifa96_camera_event_set(
      cam, vec_x, vec_z,
      (int16_t)((uint16_t)(draw & 0x3F) + (uint16_t)cam_y), 0);
  if (ev < 0) return 0;
  if (ev == FIFA96_OK) {
    (void)fifa96_camera_set_tracked(cam, r->entity_id);   /* 0x71D27 */
    e->has_ball = 0;                                      /* 0x71D2D */
    r->has_ball = 0;
  }
  /* 0x82A71..0x82A97: the 0x8F188 / 0x651F0 x2 / 0x974F0 sinks (FU167-L3). */
  return 1;
}

/* Row 0x0E `0x82710..0x82ACF` (FU-160 §2, fresh `disassemble_bytes` this
 * task; FU-137 table entry 14): the **live jump selector** — the installed
 * entity's code is drained into the pool by the row-23 tackle arm
 * (`out.install_0e -> install = 0x0E`, `fifa96_match_handlers.c:2203`) and
 * this body installs anim row 0x12 on itself, whose funnel frame (f2,
 * aux 0x0E) runs the already-ported jump arm + BF20 arc.
 *
 * Ported per the fresh bytes (all citations this task):
 *  - head `0x8271B..0x82742`: phase != 2 -> `FUN_0007DAB4` reset; else
 *    `dword[+0x89] += (uint16)[0x157A64]` (timer89, zero-extended delta);
 *  - stage > 3 (`0x82748/0x8274A`) -> tail (no-op);
 *  - stage 0 `0x8275D..0x827AB`: `byte[+0x8D] == 0` -> reset; the
 *    `0x71B9C(0x12, &+0x4D)` target build (the fresh decompile writes the
 *    `0x15774C/50/54` camera triple + per-kind offsets -> the engine camera
 *    stand-in, the row-23 convention), the `0x8DCD4` metric (`+0x65` out)
 *    and the `0x79C50` face toward `(word[+0x67], word[+0x69])`
 *    (`fifa96_arm_dist_stage`/`fifa96_arm_face`); `timer89 = 0`, stage -> 1;
 *  - stage 1 `0x827B1..0x82800`: target := the `0x157794` triple (the FU-141
 *    second selection vector; the engine's camera stand-in), the
 *    `FUN_00079B58(self)` receiver timer (`if byte[+0x99] == 0 -> +0x93 =
 *    0x10`; byte99 staged 0, the row-04 convention), wait `timer89 >= 0x0C`,
 *    then `FUN_0006E598(self, 0x12, frame 0)` (`0x827E2`, the fresh
 *    `record_anim_install`; the `ECX = byte[+0x8E]` load is dead, FU-160
 *    §1.4), `byte[+0x9E] = 1` (ran), `timer89 = 0`, stage -> 2;
 *  - stage 2 `0x82806..0x82AB2`: `record == [0x1577CA]` (the camera-tracked
 *    record, engine `mr->render.camera.tracked`; the separate controlled cell
 *    is `[0x157A83]`) or `word[+0x85] != 0` -> `timer89 = 0`, stage -> 3; the
 *    else arm (`0x82820..0x82A9C`, the camera-follow/slot-dir block) is
 *    FU160-L4;
 *  - stage 3 `0x82AB8..0x82AC4`: `byte[+0x44] != 0` -> `FUN_0007DAB4` reset
 *    (the completed-animation reset; row44 is the live anim-row terminal
 *    flag, so the record frees when the successor reaches a terminal row). */
static int fifa96_match_action_0E(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  uint32_t team = 0;
  uint32_t idx = 0;
  struct fifa96_match_entity *e = match_sp_entity(mr, &team, &idx);
  if (e == NULL) return -FIFA96_ERR_INVALID;
  if (mr->state.phase != 2u) {                       /* 0x8271B..0x82726 */
    match_row_reset(mr, e);
    return FIFA96_OK;
  }
  /* 0x8272E..0x82742: `XOR EAX,EAX; MOV AX,[0x157A64]; ADD EDX,EAX` — the
   * delta word is **zero-extended** (the T3 review minor 3; observable only
   * at delta >= 0x8000). */
  r->timer89 += (uint16_t)r->delta;
  if ((uint8_t)r->stage92 > 3u) return FIFA96_OK;    /* 0x82748/0x8274A */
  if (r->stage92 == 0u) {                            /* 0x8275D */
    if (r->active == 0u) {
      match_row_reset(mr, e);
      return FIFA96_OK;
    }
    match_target_from_camera(mr, r);                 /* 0x8276A FUN_00071B9C */
    {
      fifa96_arm_vec from = { r->pos_x, r->pos_y, r->pos_z };
      fifa96_arm_vec to = { r->target_x, r->target_y, r->target_z };
      int32_t dz = 0;
      uint8_t octant = r->type;
      (void)fifa96_arm_dist_stage(&from, &to, &r->distance, &dz);  /* 0x82781 */
      if (fifa96_arm_face(&from, &to, &octant) == FIFA96_OK) r->type = octant;
    }
    r->timer89 = 0;                                  /* 0x8279F */
    r->stage92 = 1u;                                 /* 0x827AB */
  }
  if (r->stage92 == 1u) {                            /* 0x827B1 */
    match_target_from_camera(mr, r);                 /* [0x157794] stand-in */
    e->timer93 = 0x10;                               /* 0x827BE FUN_00079B58 */
    if (r->timer89 < 0x0C) return FIFA96_OK;         /* 0x827C3/0x827CA */
    if (fifa96_match_run_record_anim_install(mr, e, 0x12, 0) != FIFA96_OK)
      return FIFA96_OK;
    r->anim_id = e->anim_id;   /* the install lands on the staging drain */
    r->frame = e->frame;
    r->row44 = e->row44;
    r->ran = 1;                                      /* 0x827E7 +0x9E = 1 */
    r->timer89 = 0;                                  /* 0x827F4 */
    r->stage92 = 2u;                                 /* 0x82800 */
  }
  if (r->stage92 == 2u) {                            /* 0x82806/0x82812 */
    if (mr->render.camera.tracked == r->entity_id || r->jump_base != 0) {
      r->timer89 = 0;                                /* 0x82AA6 */
      r->stage92 = 3u;                               /* 0x82AB2 */
    } else if (match_sp_pan_arm(mr) != 0) {
      /* FU-167/FU-168: the camera-follow/slot-dir else arm completed; the
       * gates failing leave the record at stage 2 (the retry semantics). */
      r->timer89 = 0;                                /* 0x82AA6 */
      r->stage92 = 3u;                               /* 0x82AB2 */
    }
  }
  if (r->stage92 == 3u) {                            /* 0x82AB8 */
    if (r->row44 != 0u) match_row_reset(mr, e);
  }
  return FIFA96_OK;
}

/* The `FUN_0008DE8C` pick over the pool (positions, skip index, the record's
 * transient `+0x9A` self stamp the natives set around the call). */
static int32_t match_sp_nearest(struct fifa96_match_run *mr, uint32_t team,
                                uint8_t skip_self, uint32_t idx, uint8_t skip_index,
                                int16_t from_x, int16_t from_z) {
  fifa96_entity_candidate candidates[FIFA96_MATCH_ENTITY_RECORDS];
  int16_t best = 0;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[team].records[i];
    candidates[i].x = (int16_t)e->pos_x;
    candidates[i].y = (int16_t)e->pos_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  if (skip_self != 0 && idx < FIFA96_MATCH_ENTITY_RECORDS)
    candidates[idx].skip_9a = 1;   /* 0x85C79 / 0x844B4 / 0x86401 stamp */
  return fifa96_entity_find_nearest(candidates, FIFA96_MATCH_ENTITY_RECORDS,
                                    skip_index, from_x, from_z, &best);
}

/* `FUN_00079B6C` re-anchor subset: the argument triple becomes the record's
 * position and target (y forced 0), lane/velocity cleared (FU-139 §11.1). */
static void match_sp_anchor(struct fifa96_match_run_record *r, int32_t x, int32_t y,
                            int32_t z) {
  r->pos_x = x;
  r->pos_y = y;
  r->pos_z = z;
  r->target_x = x;
  r->target_y = y;
  r->target_z = z;
  r->lane = 0;
  r->vel_x = 0;
  r->vel_z = 0;
  r->speed71 = 0;
  r->vel73 = 0;
  r->vel75 = 0;
}

/* `FUN_00073E08` placement commit subset (the engine's
 * `fifa96_match_entities_place` on the staged record). */
static void match_sp_commit(struct fifa96_match_run_record *r) {
  r->pos_x = r->target_x;
  r->pos_y = 0;
  r->pos_z = r->target_z;
  r->target_x = r->pos_x;
  r->target_y = 0;
  r->target_z = r->pos_z;
  r->lane = (int32_t)((uint32_t)r->lane & 0xFFFF0000u);
  r->vel_x = 0;
  r->vel_z = 0;
  r->speed71 = 0;
  r->vel73 = 0;
  r->vel75 = 0;
}

/* `FUN_00079B1C` snap subset: target = position (the keeper row-1E mapping). */
static void match_sp_snap(struct fifa96_match_run_record *r) {
  r->target_x = r->pos_x;
  r->target_y = r->pos_y;
  r->target_z = r->pos_z;
}

/* `FUN_000832A8` derived stand-in (L13 leg): the native clears the 0x158784
 * throw gate (`0x832BC`), latches the 0x1587B0/0x1587B2 staging words and the
 * 0x15879C triple, runs the `FUN_00083164`/`FUN_0008DB6C` slot block and the
 * 0x361xx presentation calls, and clears the record timer (`0x833xx`). Only the
 * two cells the row stages observe are modelled. */
static void match_sp_setup_832a8(struct fifa96_match_run *mr,
                                 struct fifa96_match_run_record *r) {
  mr->sp_flag_158784 = 0;
  r->timer89 = 0;
}

/* `FUN_00085498` probe, derived completion (L13 leg for the slot arm). The
 * native no-slot arm (record slot empty and the staged 0x1587B0 latch set by
 * `FUN_000832A8`) fires once when the 0x78 timer is crossed, draws
 * `FUN_00092AC8() % span` into the record's `+0x7E7` and latches the delivery
 * row `[0x1587B4]` 3 (| >= 3) or 7; the slot arm runs the input-driven
 * `FUN_00083428` machine. The derived completion applies the same timer gate
 * plus draw so an armed row still resolves; the RNG width is the engine's
 * 16-bit draw (native 32-bit, cadence leg). */
static int match_sp_probe_85498(struct fifa96_match_run *mr, uint16_t span) {
  uint16_t draw = 0;
  if (mr->record.timer89 <= 0x78) return 0;
  if (fifa96_rng_step(&mr->rng, &draw) != 0) return 0;
  mr->sp_delivery = (uint8_t)((draw % span) < 3u ? 3u : 7u);
  return 1;
}

/* The shared resolution: `FUN_0008A938(0xB, rec_team, 0)` hands the taker back
 * to phase 2 through the engine's single shared situation entry. Rows 0x10
 * (`0x85D19`) and 0x11 (`0x863D8..0x863DF`) additionally write
 * `[0x157A6A] = 0x12C` (the offside suppression timer) before the call; rows
 * 0x12 (`0x84480..0x84495`) and 0x13 (`0x84E79..0x84E8F`) resolve without
 * touching the cell. */
static int match_sp_resolve(struct fifa96_match_run *mr, uint8_t set_offside_timer) {
  int rc;
  if (set_offside_timer != 0) mr->referee.offside_suppress = 0x12C;
  rc = fifa96_match_run_situation(mr, 0x0Bu);
  return rc;
}

/* One `FUN_0007B9C4` delivery through the ported kick path. The action-kick
 * staging record supplies the actor/ctx inputs (the row-07/0F binding). */
static int match_sp_kick(struct fifa96_match_run *mr,
                         const fifa96_ball_pair_vector *input, uint8_t mode,
                         uint8_t slot_present) {
  fifa96_action_kick s;
  fifa96_ball_kick_out out;
  match_kick_from_record(mr, mr->record.code, &s);
  return match_kick_run(mr, &s, input, mode, slot_present, &out);
}

/* The `FUN_0008DCD4` distance word of (record position -> target) for the
 * row-0x13 word-at-+0x65 gates. */
static int32_t match_sp_target_distance(const struct fifa96_match_run_record *r,
                                        int32_t to_x, int32_t to_z) {
  fifa96_arm_vec from;
  fifa96_arm_vec to;
  int32_t distance = 0;
  int32_t dz = 0;
  from.x = r->pos_x;
  from.y = r->pos_y;
  from.z = r->pos_z;
  to.x = to_x;
  to.y = 0;
  to.z = to_z;
  (void)fifa96_arm_dist_stage(&from, &to, &distance, &dz);
  return distance;
}

/* The shared taker pick + `install 4` (`0x85C6D`, `0x86403`, `0x844AF`,
 * `0x84E26`): the derived `0x8DE8C` nearest over the camera-led triple, the
 * team-target latch and the relay-record code 4 install. */
static void match_sp_pick_install(struct fifa96_match_run *mr, uint32_t team,
                                  uint32_t idx, uint8_t skip_index, int16_t from_x,
                                  int16_t from_z) {
  int32_t pick = match_sp_nearest(mr, team, 1u, idx, skip_index, from_x, from_z);
  if (pick < 0) return;
  mr->entities.team[team].target =
      (int32_t)(team * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)pick);
  (void)fifa96_match_entities_install(&mr->entities.team[team].records[pick],
                                      (uint8_t)mr->state.phase, 4u, 0);
}

/* Row 0x10 `0x855F0..0x85DE3` (FU-149 §1.6, FU-81 §2.1): the throw-in taker.
 * Stages: 0 placement at the 0x15777C snapshot (`0x8566D`), 1 the
 * `FUN_000832A8` setup (`0x857D6`), 2 the `FUN_00085498` delivery probe
 * (`0x8580D`), 3 the ball-line/pick/install-4 block (`0x858E4`), 4 the
 * resolution/requeue (`0x85CC4`/`0x85D19`), 5 the `+0x44` wait (`0x85D5A`)
 * and 6 the reset (`0x85D8D`). */
static int fifa96_match_action_10(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  uint32_t team = 0;
  uint32_t idx = 0;
  struct fifa96_match_entity *e = match_sp_entity(mr, &team, &idx);
  uint8_t stage;
  int rc;
  if (e == NULL) return -FIFA96_ERR_INVALID;
  if (mr->state.phase != 2u && mr->state.phase != 3u) {   /* 0x85603/0x8560B */
    match_row_reset(mr, e);
    return FIFA96_OK;
  }
  r->timer89 += (int16_t)r->delta;                        /* 0x85613..0x8562A */
  if ((int8_t)r->stage92 < 3) r->controlled = 1;          /* 0x85630..0x85639 */
  if ((int8_t)r->stage92 < 5) {                           /* 0x8563B..0x85651 */
    r->target_x = r->pos_x;
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
  }
  stage = r->stage92;
  if (stage > 6u) return FIFA96_OK;                       /* 0x8565A JA */
  if (stage == 0u) {                                      /* 0x8566D */
    /* 0x8566D..0x8567B: [0x157AAF] = team side (unmodeled, leg). */
    r->helper_request = r->has_slot == 0 ? 1u : 0u;       /* 0x85682 FUN_0007876C */
    r->target_x = mr->goal_snap_x;                        /* 0x8568A..0x85692 */
    r->target_y = mr->goal_snap_y;
    r->target_z = mr->goal_snap_z;
    if (mr->session_gate_14c32a == 0u && r->timer89 < 0x3C &&
        (int16_t)(r->lane >> 16) > 0x20)
      return FIFA96_OK;                                   /* 0x85693..0x856A9 */
    /* 0x856AF..0x857D0: the FUN_000700F4 snapshot reset, the FUN_00073E08
     * commit and the record-position write at the snapshot ±0x10. The
     * 0x8570A..0x857BE dropped bodies (0x79B6C re-anchor, 0x8DCD4/0x79C50
     * face, 0x6E598, the 0x157A77/0x15879C halves, 0x8CFAC, 0x4C324,
     * 0x918CC) are L13 legs; the record-visible outcome is kept. */
    (void)fifa96_camera_init(&mr->render.camera, mr->goal_snap_x, mr->goal_snap_y,
                             mr->goal_snap_z);            /* 0x856CB */
    r->place_x = mr->goal_snap_x;
    r->place_y = mr->goal_snap_y;
    r->place_z = mr->goal_snap_z;
    r->place_valid = 1;
    r->pos_x = mr->goal_snap_x;                           /* 0x856D8 */
    r->pos_y = mr->goal_snap_y;
    r->pos_z = mr->goal_snap_z;
    r->pos_x += (mr->goal_snap_x > 0 ? 0x10 : -0x10);     /* 0x856DB..0x856FC */
    match_sp_anchor(r, r->pos_x, 0, r->pos_z);            /* 0x85705 FUN_00079B6C */
    r->timer89 = 0;                                       /* 0x857C4 */
    r->stage92 = 1u;                                      /* 0x857D0 */
    stage = 1u;
  }
  if (stage == 1u) {                                      /* 0x857D6 */
    /* 0x857DF: the 0x974DC(0x1E) sound when the session gate is 0 (leg). */
    match_sp_setup_832a8(mr, r);                          /* 0x857F0 FUN_000832A8 */
    r->timer89 = 0;                                       /* 0x857FB */
    r->stage92 = 2u;                                      /* 0x85807 */
    stage = 2u;
  }
  if (stage == 2u) {                                      /* 0x8580D */
    /* 0x8582E..0x85837: the 0x15774C focus := the 0x157764 camera target. */
    mr->render.camera.pos_x = mr->render.camera.target_x;
    mr->render.camera.pos_y = mr->render.camera.target_y;
    mr->render.camera.pos_z = mr->render.camera.target_z;
    if (mr->sp_flag_158784 != 0u) {                       /* 0x8583B..0x85875 */
      match_sp_setup_832a8(mr, r);
      return FIFA96_OK;
    }
    /* 0x8587A: the 0xA5 timer event (leg). */
    if (match_sp_probe_85498(mr, 4u) == 0) return FIFA96_OK;  /* 0x858B7 */
    r->timer89 = 0;                                       /* 0x858C4 */
    r->stage92 = 3u;                                      /* 0x858DE */
    stage = 3u;
  }
  if (stage == 3u) {                                      /* 0x858E4 */
    /* 0x858E4: the 0x4C380 no-op (leg); 0x858E9..0x858F2 the animation-frame
     * gate on the record's byte +0x3D. */
    if (r->frame < 4u) return FIFA96_OK;
    /* 0x858F8..0x85966: the 0x18 event and the 0x10F334/0x10F33C face-octant
     * ball-line build, then the FUN_000700F4 reset at the line (the derived
     * subset applies the octant offsets to the record position, the tables are
     * the ported `match_kick_dir_x/z`). */
    {
      /* FU-162 §3.3.A (T3 fix): the native indexes the 0x10F334/0x10F33C byte
       * tables with the **live face octant** `byte[rec+0x8E]` (fresh
       * `0x8591e MOV EAX,[EBP+0x8b]; SAR EAX,0x18` — the `+0x8E` byte, the
       * port's `record.type`), not `r->actor_type` (native `+0x8B`), which no
       * code writes (OL-83). The port masks to the 32-entry derived tables
       * (hardening divergence). */
      uint32_t dir_idx = (uint32_t)r->type & 0x1Fu;
      /* The native `SHL EDX,6` shifts the sign-extended dword (two's
       * complement); the unsigned shift keeps that exact 32-bit wrap without
       * the C signed-overflow UB (UBSan-clean). */
      int32_t dx = (int32_t)((uint32_t)match_kick_dir_x[dir_idx] << 6);
      int32_t dz = (int32_t)((uint32_t)match_kick_dir_z[dir_idx] << 6);
      /* FU-157 §4.2/§5 (T1): the native cut writes `[0x157750] := 0x80`
       * (0x85911/0x85919) and hands the just-written value to FUN_000700F4
       * as its Y (0x85950/0x85958; CALL at 0x8596B), so the place is
       * (rec.x + dx, 0x80, rec.z + dz) — the Y is preserved, not zero. */
      (void)fifa96_camera_init(&mr->render.camera, r->pos_x + dx, 0x80,
                               r->pos_z + dz);
    }
    /* 0x85966..0x85CA1: the dropped 0x8DCD4 clamp/folds/0x79C50/0x92820/
     * 0x7A490 staging (L13 legs) and the derived pick + relay install 4. */
    match_sp_pick_install(mr, team, idx, 0u,
                          (int16_t)mr->render.camera.pos_x,
                          (int16_t)mr->render.camera.pos_z);
    r->timer89 = 0;                                       /* 0x85CA6 */
    r->stage92 = 4u;                                      /* 0x85CC4 */
    stage = 4u;
  }
  if (stage == 4u) {                                      /* 0x85CC4 */
    int32_t bx;
    match_sp_snap(r);                                     /* 0x85CC6 FUN_00079B1C */
    bx = mr->render.camera.pos_x;                         /* 0x85CCB [0x15774C] */
    if (bx < 0) bx = -bx;
    if (bx >= 0x720) {                                    /* 0x85CDD..0x85CE2 */
      if ((int8_t)mr->sp_157821 <= 0) return FIFA96_OK;   /* 0x85CE4..0x85CEB */
      /* 0x85CF1..0x85D0A: re-queue situation 2 with the record's team side. */
      return fifa96_match_run_set_piece(mr, 2u,
                                        (uint8_t)mr->entities.team[team].side, 1u);
    }
    /* 0x85D19..0x85D54: the resolution tail (offside timer + phase 2). */
    rc = match_sp_resolve(mr, 1);
    if (rc != 0) return rc;
    r->timer89 = 0;                                       /* 0x85D48 */
    r->stage92 = 5u;                                      /* 0x85D54 */
    stage = 5u;
  }
  if (stage == 5u) {                                      /* 0x85D5A */
    match_sp_snap(r);                                     /* 0x85D5C */
    if (r->row44 == 0u) return FIFA96_OK;                 /* 0x85D61..0x85D65 */
    r->timer89 = 0;                                       /* 0x85D67 */
    r->stage92 = 6u;                                      /* 0x85D87 */
    stage = 6u;
  }
  if (stage == 6u) {                                      /* 0x85D8D */
    match_row_reset(mr, e);
  }
  return FIFA96_OK;
}

/* Row 0x11 `0x85DE4..0x864FF` (FU-149 §1.6, FU-81 §2.1): the corner taker.
 * Stages: 0 the controlled/marker + 0x3C wait (`0x85E3C`), 1 the
 * `FUN_0007D360` corner probe/placement (`0x85E7D`), 2/3 pure advances
 * (`0x85FA2`/`0x85FBD`), 4 the setup (`0x85FD8`), 5 the delivery probe
 * (`0x86013`), 6 the target/lane gate (`0x860CB`), 7 the kick + resolution +
 * relay (`0x8610E`), 8 the `+0x44` wait (`0x8645B`) and 9 the target clamp
 * (`0x86496`). */
static int fifa96_match_action_11(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  uint32_t team = 0;
  uint32_t idx = 0;
  struct fifa96_match_entity *e = match_sp_entity(mr, &team, &idx);
  uint8_t stage;
  int rc;
  if (e == NULL) return -FIFA96_ERR_INVALID;
  if (mr->state.phase != 2u && mr->state.phase != 4u) {   /* 0x85DF2..0x85E02 */
    match_row_reset(mr, e);
    return FIFA96_OK;
  }
  r->timer89 += (int16_t)r->delta;                        /* 0x85E19 */
  stage = r->stage92;
  if (stage > 9u) return FIFA96_OK;                       /* 0x85E29 JA */
  if (stage == 0u) {                                      /* 0x85E3C */
    r->controlled = 1;                                    /* 0x85E4B */
    r->helper_request = r->has_slot == 0 ? 1u : 0u;       /* 0x85E51 */
    if (r->timer89 < 0x3C) return FIFA96_OK;              /* 0x85E56..0x85E5D */
    r->timer89 = 0;                                       /* 0x85E63 */
    r->stage92 = 1u;                                      /* 0x85E77 */
    stage = 1u;
  }
  if (stage == 1u) {                                      /* 0x85E7D */
    /* 0x85E7D..0x85E85: the FUN_0007D360 corner probe (±0x710,0,±0xB00 from
     * the snapshot signs), the FUN_000700F4 reset and the commit; the probe
     * ±0x50 by sign; the FUN_00079B6C re-anchor (the record position = the
     * probe point). The 0x8DCD4/0x79C50/0x6E598/0x8CFAC/0x4C324/0x918CC
     * bodies are L13 legs. */
    int32_t px = mr->goal_snap_x < 0 ? -0x710 : 0x710;
    int32_t pz = mr->goal_snap_z < 0 ? -0xB00 : 0xB00;
    (void)fifa96_camera_init(&mr->render.camera, px, 0, pz);   /* 0x85E98 */
    r->place_x = px;
    r->place_y = 0;
    r->place_z = pz;
    r->place_valid = 1;
    px += px > 0 ? 0x50 : -0x50;                          /* 0x85EA2..0x85EBD */
    pz += pz > 0 ? 0x50 : -0x50;                          /* 0x85EBF..0x85ED2 */
    match_sp_anchor(r, px, 0, pz);                        /* 0x85EE2 */
    r->timer89 = 0;                                       /* 0x85F90 */
    r->stage92 = 2u;                                      /* 0x85F9C */
    stage = 2u;
  }
  if (stage == 2u) {                                      /* 0x85FA2 */
    r->timer89 = 0;                                       /* 0x85FAB */
    r->stage92 = 3u;                                      /* 0x85FB7 */
    stage = 3u;
  }
  if (stage == 3u) {                                      /* 0x85FBD */
    r->timer89 = 0;                                       /* 0x85FC6 */
    r->stage92 = 4u;                                      /* 0x85FD2 */
    stage = 4u;
  }
  if (stage == 4u) {                                      /* 0x85FD8 */
    /* 0x85FD8: the 0x974DC(0x1E) sound when the session gate is 0 (leg). */
    match_sp_setup_832a8(mr, r);                          /* 0x85FF3 */
    r->timer89 = 0;                                       /* 0x85FFB */
    r->stage92 = 5u;                                      /* 0x8600D */
    stage = 5u;
  }
  if (stage == 5u) {                                      /* 0x86013 */
    /* 0x8601B..0x8602D: the 0x15774C focus := the 0x157764 camera target; the
     * record re-anchor at its own position (0x79B6C). */
    mr->render.camera.pos_x = mr->render.camera.target_x;
    mr->render.camera.pos_y = mr->render.camera.target_y;
    mr->render.camera.pos_z = mr->render.camera.target_z;
    match_sp_anchor(r, r->pos_x, 0, r->pos_z);            /* 0x8602E */
    if (mr->sp_flag_158784 != 0u) {                       /* 0x86033..0x86052 */
      match_sp_setup_832a8(mr, r);
      return FIFA96_OK;
    }
    /* 0x86057: the 0x4B0 timer event (leg). */
    if (match_sp_probe_85498(mr, 4u) == 0) return FIFA96_OK;  /* 0x8609F */
    r->timer89 = 0;                                       /* 0x860B7 */
    r->stage92 = 6u;                                      /* 0x860C5 */
    stage = 6u;
  }
  if (stage == 6u) {                                      /* 0x860CB */
    r->target_x = mr->render.camera.pos_x;                /* 0x860CB..0x860DB */
    r->target_y = mr->render.camera.pos_y;
    r->target_z = mr->render.camera.pos_z;
    if ((int16_t)(r->lane >> 16) > 0x40) return FIFA96_OK; /* 0x860E4..0x860ED */
    r->timer89 = 0;                                       /* 0x860FC */
    r->stage92 = 7u;                                      /* 0x86108 */
    stage = 7u;
  }
  if (stage == 7u) {                                      /* 0x8610E */
    /* 0x8610E..0x86382: the kick block. The native selects one of the seven
     * event sub-table 0x85DC8 arms by `[0x1587B4]-1` (or the per-team 0x1587E8
     * table when `[team+0x7E7] != 0`) and passes each arm's own vector to
     * FUN_0007B9C4; the ported subset emits one derived delivery from the
     * engine ball-staging vector with mode 0x10 (L13 legs: the arm/vector
     * selection). */
    (void)match_sp_kick(mr, &mr->entities.ball.pair.vector, 0x10u,
                        mr->record.has_slot);
    /* 0x86388..0x863D0: the FUN_00092AC8 draw and the 0x19/0x1C events
     * (legs); the draw is consumed for RNG cadence. */
    {
      uint16_t draw = 0;
      (void)fifa96_rng_step(&mr->rng, &draw);
    }
    rc = match_sp_resolve(mr, 1);                         /* 0x863D8..0x863F9 */
    if (rc != 0) return rc;
    match_sp_pick_install(mr, team, idx, 0u,              /* 0x86403..0x86437 */
                          (int16_t)mr->render.camera.pos_x,
                          (int16_t)mr->render.camera.pos_z);
    r->timer89 = 0;                                       /* 0x8644B */
    r->stage92 = 8u;                                      /* 0x86455 */
    stage = 8u;
  }
  if (stage == 8u) {                                      /* 0x8645B */
    match_sp_snap(r);                                     /* 0x8645E */
    if (r->row44 == 0u) return FIFA96_OK;                 /* 0x86466..0x8646A */
    r->timer89 = 0;                                       /* 0x86470 */
    r->stage92 = 9u;                                      /* 0x86490 */
    stage = 9u;
  }
  if (stage == 9u) {                                      /* 0x86496 */
    /* 0x86496..0x864E5: the target clamp (x to ±0x630, z to ±0x840 by side). */
    if (r->pos_x > 0x630) r->target_x = 0x630;
    else if (r->pos_x < -0x630) r->target_x = -0x630;
    else r->target_x = r->pos_x;
    r->target_z = mr->entities.team[team].side != 0u ? -0x840 : 0x840;
    if (r->distance < 0x30) {                             /* 0x864E8..0x864F1 */
      match_row_reset(mr, e);
      return FIFA96_OK;
    }
    if (r->timer89 > 0x78) {                              /* 0x864F3..0x864FD */
      match_row_reset(mr, e);
      return FIFA96_OK;
    }
    return FIFA96_OK;                                     /* 0x86507 wait */
  }
  return FIFA96_OK;
}

/* Row 0x12 `0x83D68..0x84597` (FU-149 §1.6, FU-81 §2.1): the free-kick taker.
 * Stages: 0 the incident-triple placement + wall point (`0x83E26`), 1 the
 * setup (`0x84077`), 2 the delivery probe (`0x840B2`), 3 the target/lane gate
 * (`0x8416A`), 4 the kick + resolution + relay (`0x841AD`), 5 the `+0x44`
 * wait (`0x8450C`), 6 the snap (`0x84543`) and 7 the reset (`0x84556`). */
static int fifa96_match_action_12(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  uint32_t team = 0;
  uint32_t idx = 0;
  struct fifa96_match_entity *e = match_sp_entity(mr, &team, &idx);
  uint8_t stage;
  int rc;
  if (e == NULL) return -FIFA96_ERR_INVALID;
  if (mr->state.phase != 2u && mr->state.phase != 7u) {   /* 0x83D7E..0x83D98 */
    match_row_reset(mr, e);
    return FIFA96_OK;
  }
  /* 0x83DA0..0x83DF2: the `+0x8F < 3` wall block (the per-side 0x1577D8 gate,
   * the FUN_00083B80 wall setup) and the 0x158747 five-dword clear are
   * unported staging (L13 leg). */
  r->timer89 += (int16_t)r->delta;                        /* 0x83E03 */
  stage = r->stage92;
  if (stage > 7u) {                                       /* 0x83E13 JA */
    /* 0x8455B..0x8458C: the phase-7 `+0x8F < 5` 0x157A77 refresh (leg). */
    return FIFA96_OK;
  }
  if (stage == 0u) {                                      /* 0x83E26 */
    /* 0x83E26..0x83E2E: [0x157AAF] = team side (unmodeled, leg). */
    r->helper_request = r->has_slot == 0 ? 1u : 0u;       /* 0x83E35 */
    r->controlled = 1;                                    /* 0x83E40 */
    if (mr->session_gate_14c32a == 0u && r->timer89 < 0x3C)
      return FIFA96_OK;                                   /* 0x83E3A..0x83E51 */
    r->timer89 = 0;                                       /* 0x83E60 */
    r->stage92 = 1u;                                      /* 0x83E6C */
    stage = 1u;
    /* 0x83E72..0x83EAB: the 0x15774C focus := the 0x158897 incident triple
     * with z clamped to ±0x9F0; the FUN_000700F4 reset. */
    {
      int32_t iz = mr->incident_z;
      if (iz > 0x9F0) iz = 0x9F0;
      else if (iz < -0x9F0) iz = -0x9F0;
      (void)fifa96_camera_init(&mr->render.camera, mr->incident_x, 0, iz);
      r->place_x = mr->incident_x;
      r->place_y = 0;
      r->place_z = iz;
      r->place_valid = 1;
    }
    match_sp_commit(r);                                   /* 0x83EC7 FUN_00073E08 */
    /* 0x83ECC..0x84065: the wall-point build: the (0,0,±0xB10 by side) probe,
     * the 0x8DCD4 triple, the 0xCD474 angle and the 0x114E04/0x795A4 folds
     * with the 0xA0 step, then the FUN_00079B6C re-anchor at the wall point.
     * The 0x7D388/0x4C31C/0x4C320/0x4C324 bodies are L13 legs. */
    {
      int32_t pz = mr->entities.team[team].side == 0u ? 0xB10 : -0xB10;
      int16_t dx = (int16_t)((uint16_t)0u - (uint16_t)mr->incident_x);
      int16_t dz = (int16_t)((uint16_t)pz - (uint16_t)r->place_z);
      int32_t angle = 0;
      if (fifa96_action_kick_angle(dx, dz, &angle) == FIFA96_OK) {
        int32_t a = (angle + 0x200) & 0x3FF;
        if (a > 0x200) a -= 0x400;
        match_sp_anchor(r, mr->incident_x + fifa96_ball_fold(0xA0, a), 0,
                        mr->incident_z + fifa96_ball_fold(0xA0, a + 0x100));
      }
    }
    if (mr->session_gate_14c32a == 0u) return FIFA96_OK;  /* 0x8406A..0x84071 */
    /* falls into stage 1 */
  }
  if (stage == 1u) {                                      /* 0x84077 */
    /* 0x84080: the 0x974DC(0x1E) sound when the session gate is 0 (leg). */
    match_sp_setup_832a8(mr, r);                          /* 0x84092 */
    r->timer89 = 0;                                       /* 0x840A0 */
    r->stage92 = 2u;                                      /* 0x840AC */
    stage = 2u;
  }
  if (stage == 2u) {                                      /* 0x840B2 */
    /* 0x840BF..0x840CD: the focus := the camera target; the record re-anchor
     * at its own position. */
    mr->render.camera.pos_x = mr->render.camera.target_x;
    mr->render.camera.pos_y = mr->render.camera.target_y;
    mr->render.camera.pos_z = mr->render.camera.target_z;
    match_sp_anchor(r, r->pos_x, 0, r->pos_z);            /* 0x840CD */
    if (mr->sp_flag_158784 != 0u) {                       /* 0x840D2..0x840F1 */
      match_sp_setup_832a8(mr, r);
      return FIFA96_OK;
    }
    /* 0x840F6: the 0x4B0 timer event (leg). */
    if (match_sp_probe_85498(mr, 3u) == 0) return FIFA96_OK;  /* 0x8413B */
    r->timer89 = 0;                                       /* 0x8415E */
    r->stage92 = 3u;                                      /* 0x84164 */
    stage = 3u;
  }
  if (stage == 3u) {                                      /* 0x8416A */
    r->target_x = mr->render.camera.pos_x;                /* 0x8416A..0x84182 */
    r->target_y = mr->render.camera.pos_y;
    r->target_z = mr->render.camera.pos_z;
    if ((int16_t)(r->lane >> 16) > 0x40) return FIFA96_OK; /* 0x84189..0x8418C */
    r->timer89 = 0;                                       /* 0x8419B */
    r->stage92 = 4u;                                      /* 0x841A7 */
    stage = 4u;
  }
  if (stage == 4u) {                                      /* 0x841AD */
    /* 0x841AD..0x84425: the kick block. The native selects the seven-arm event
     * sub-table 0x83D4C by `[0x1587B4]-1` (or the per-team 0x1587E8 table when
     * `[team+0x7E7] != 0`) and passes each arm's own vector; the
     * `FUN_0006DBCC==3` -> mode-0x40 conditional (`0x84425..0x84443`) selects a
     * separate delivery. The derived subset collapses all of it to one
     * delivery from the engine ball-staging vector with mode 0x10 (L13.3). */
    (void)match_sp_kick(mr, &mr->entities.ball.pair.vector, 0x10u,
                        mr->record.has_slot);
    /* 0x84471..0x84478: the 0x4C380/0x4C374 no-ops (legs). */
    rc = match_sp_resolve(mr, 0);                         /* 0x84480..0x84495 */
    if (rc != 0) return rc;
    /* 0x8449A..0x844E7: the relay install 4 unless the staging code byte
     * 0x158743 (the top byte of the 0x158740 dword) is 3. The `FUN_0008DE8C`
     * call passes EBX=0 (skip record 0) with the record's transient `+0x9A`
     * self stamp set (`0x844B4`). */
    if (mr->entities.ball.pair.code != 3u)
      match_sp_pick_install(mr, team, idx, 0u,
                            (int16_t)mr->render.camera.pos_x,
                            (int16_t)mr->render.camera.pos_z);
    r->timer89 = 0;                                       /* 0x844FC */
    r->stage92 = 5u;                                      /* 0x84506 */
    stage = 5u;
  }
  if (stage == 5u) {                                      /* 0x8450C */
    match_sp_snap(r);                                     /* 0x8450F */
    if (r->row44 == 0u) return FIFA96_OK;                 /* 0x84517..0x8451B */
    r->timer89 = 0;                                       /* 0x84533 */
    r->stage92 = 6u;                                      /* 0x8453D */
    stage = 6u;
  }
  if (stage == 6u) {                                      /* 0x84543 */
    match_sp_snap(r);                                     /* 0x84546 */
    stage = 7u;                                           /* 0x84550 falls into 7 */
  }
  if (stage == 7u) {                                      /* 0x84556 */
    match_row_reset(mr, e);
  }
  return FIFA96_OK;
}

/* Row 0x13 `0x84B00..0x84EEB` (FU-149 §1.6, FU-81 §2.1): the penalty taker.
 * Stages: 0 the commit + ball line (`0x84BAD`), 1 the spot aim + distance gate
 * (`0x84C48`), 2 the input/keeper/slot gate (`0x84CBA`), 3 the target/lane gate
 * (`0x84DF4`), 4 the kick (`0x84E26`), 5 the ball-ack gate + resolution
 * (`0x84E61`) and 6 the `+0x44` reset (`0x84EB1`). */
static int fifa96_match_action_13(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  uint32_t team = 0;
  uint32_t idx = 0;
  struct fifa96_match_entity *e = match_sp_entity(mr, &team, &idx);
  uint8_t stage;
  int rc;
  if (e == NULL) return -FIFA96_ERR_INVALID;
  if (mr->state.phase != 2u && mr->state.phase != 6u) {   /* 0x84B0B..0x84B1B */
    match_row_reset(mr, e);
    return FIFA96_OK;
  }
  if ((int8_t)r->stage92 < 5) {                           /* 0x84B21..0x84B6B */
    /* The FUN_0007876C merge request, the FUN_00073DC4 penalty-spot triple
     * into the 0x15774C focus and the FUN_000700F4 reset. */
    r->controlled = 1;                                    /* 0x84B31 */
    r->helper_request = r->has_slot == 0 ? 1u : 0u;       /* 0x84B37 */
    {
      int32_t pz = mr->entities.team[team].side == 1u ? -0x8D0 : 0x8D0;
      (void)fifa96_camera_init(&mr->render.camera, 0, 0, pz);  /* 0x84B6B */
      r->place_x = 0;
      r->place_y = 0;
      r->place_z = pz;
      r->place_valid = 1;
    }
  }
  /* 0x84B70..0x84B7C: [ESP] = [team+0x7A6] (the opponent team block the stage-2
   * word[+0x71] gate reads); the derived stand-in is the opponent keeper's
   * speed word. */
  r->timer89 += (int16_t)r->delta;                        /* 0x84B8A */
  stage = r->stage92;
  if (stage > 6u) return FIFA96_OK;                       /* 0x84B9A JA */
  if (stage == 0u) {                                      /* 0x84BAD */
    match_sp_commit(r);                                   /* 0x84BAD FUN_00073E08 */
    if (mr->session_gate_14c32a == 0u) {                  /* 0x84BB2..0x84BDF */
      /* 0x84BBB..0x84BDD: the FU-120 ball record 0x15880C := the spot, x
       * ±0x180 by the spot z sign. */
      mr->entities.ball.x =
          mr->render.camera.pos_x +
          (mr->render.camera.pos_z < 0 ? 0x180 : -0x180);
      mr->entities.ball.z = mr->render.camera.pos_z;
    }
    /* 0x84BE3..0x84C1C: the 0x8CFAC pair, the 0x4C324 bind, FUN_0007A028
     * (the 0x158730 block clear) and the 0x15882A/[+0x9E] latches. The derived
     * subset clears the ported staging block and sets `ran`. */
    (void)fifa96_ball_pair_clear(&mr->entities.ball.pair);
    r->ran = 1;                                           /* 0x84C23 */
    r->timer89 = 0;                                       /* 0x84C36 */
    r->stage92 = 1u;                                      /* 0x84C42 */
    stage = 1u;
  }
  if (stage == 1u) {                                      /* 0x84C48 */
    int32_t dist;
    r->target_x = mr->render.camera.pos_x;                /* 0x84C48..0x84C52 */
    r->target_y = mr->render.camera.pos_y;
    r->target_z = mr->render.camera.pos_z;
    r->target_z += mr->entities.team[team].side == 0u ? -0xF0 : 0xF0; /* 0x84C5C..0x84C6F */
    dist = match_sp_target_distance(r, r->target_x, r->target_z);
    if (dist > 0x30) {                                    /* 0x84C87..0x84C8A */
      /* 0x84C8C: the 0x4C31C snap-request body (leg). */
      return FIFA96_OK;
    }
    /* 0x84C98: the 0x974DC(0x1E) sound (leg). */
    r->timer89 = 0;                                       /* 0x84CA8 */
    r->stage92 = 2u;                                      /* 0x84CB4 */
    stage = 2u;
  }
  if (stage == 2u) {                                      /* 0x84CBA */
    int32_t dist = match_sp_target_distance(r, r->target_x, r->target_z);
    /* 0x84CBC..0x84D11: the word-at-+0x65 gate selects the 0x4C31C input bind
     * (the 0x4C114/0x4C118/0x4C11C input words, L13 legs) and calls it. */
    (void)dist;
    /* 0x84D16..0x84D1E: the opponent tracked-record word[+0x71] gate; the
     * native record pointer [team+0x7A6] is unported, the derived stand-in is
     * the opponent keeper's speed word. */
    if (mr->entities.team[1u - team].records[0].speed71 > 0) return FIFA96_OK;
    /* 0x84D29: [0x15882A] = 1 (the staging latch, leg). */
    if (r->has_slot != 0) {
      /* 0x84D37..0x84DD2: the slot gate. The native reads word[slot+6]: 0
       * waits, an 0x20 edge with a closed session hands the kick off (reset
       * self + install 0x13 on the next +0x9A-clear record), otherwise the
       * 0x78A84 restore then advance. The engine slot word[+6] is unported;
       * the derived stand-in advances the restore path when the slot is bound
       * (L13.7). */
    } else if (r->timer89 <= 0xF0) {                      /* 0x84DC6..0x84DD2 */
      return FIFA96_OK;
    }
    r->timer89 = 0;                                       /* 0x84DE2 */
    r->stage92 = 3u;                                      /* 0x84DEE */
    stage = 3u;
  }
  if (stage == 3u) {                                      /* 0x84DF4 */
    r->target_x = mr->render.camera.pos_x;                /* 0x84DF4..0x84DFE */
    r->target_y = mr->render.camera.pos_y;
    r->target_z = mr->render.camera.pos_z;
    if ((int16_t)(r->lane >> 16) > 0x40) return FIFA96_OK; /* 0x84E05..0x84E08 */
    r->timer89 = 0;                                       /* 0x84E14 */
    r->stage92 = 4u;                                      /* 0x84E20 */
    stage = 4u;
  }
  if (stage == 4u) {                                      /* 0x84E26 */
    /* 0x84E2D..0x84E36: the FUN_00078AA4 slot restore when word[slot+6] == 0
     * (leg); 0x84E3B..0x84E44 the FUN_0007B9C4 penalty strike with a NULL
     * input vector and mode 0x40. */
    rc = match_sp_kick(mr, NULL, 0x40u, 0u);
    if (rc != 0) return rc;
    r->timer89 = 0;                                       /* 0x84E4F */
    r->stage92 = 5u;                                      /* 0x84E5B */
    stage = 5u;
  }
  if (stage == 5u) {                                      /* 0x84E61 */
    match_sp_snap(r);                                     /* 0x84E63 */
    /* 0x84E68..0x84E77: resolve unless the staged ball actor is this record
     * with a clear 0x158746 ack. */
    if (mr->entities.ball.pair.actor != r->entity_id ||
        mr->entities.ball.pair.ack != 0u) {
      rc = match_sp_resolve(mr, 0);                       /* 0x84E79..0x84E8F */
      if (rc != 0) return rc;
      r->timer89 = 0;                                     /* 0x84E9F */
      r->stage92 = 6u;                                    /* 0x84EAB */
      stage = 6u;
    } else {
      return FIFA96_OK;                                   /* 0x84E77 wait */
    }
  }
  if (stage == 6u) {                                      /* 0x84EB1 */
    match_sp_snap(r);                                     /* 0x84EB3 */
    if (r->row44 == 0u) return FIFA96_OK;                 /* 0x84EB8..0x84EBC */
    match_row_reset(mr, e);                               /* 0x84EC0 */
  }
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
 * the shared reset. The native `[[rec+0x28]][0]` abort byte is the live OL-80
 * row id (staged from the pool); the native 0x158897 search origin (written by
 * FUN_0008A3FC from the record) is the duel record's own position. */
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
  d.animation = mr->record.anim_id;   /* OL-80: byte[[rec+0x28]] live id */
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
  s.row_byte = r->anim_id;          /* byte[[rec+0x28]] live row id (OL-80) */
  s.desc_e = 0;                     /* rec[+4][+0xE] (roster descriptor, OL-72) */
  s.timer81 = r->timer81;
  s.timer89 = r->timer89;
  s.delta = r->delta;
  s.pos_x = r->pos_x;
  s.pos_y = r->pos_y;
  s.pos_z = r->pos_z;
  s.lane = e->lane_x;               /* native word +0x6B (pool `lane_x`) */
  s.bound = e->bound;               /* FU-147 S1: native word +0x77 */
  s.word6d = e->lane_z;             /* native word +0x6D */
  s.word6f = e->cam_dz6f;           /* FU-147 S1: native word +0x6F */
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
  /* FU-171 §7 rec 2 (T2/FU171-L2): the native half-line gates 0x7EDA0/0x7EE83/
   * 0x7EFEE all read `[0x1577CA]` — the camera-tracked record — not the
   * controlled actor. Live now that `fifa96_camera_set_tracked` maintains the
   * cell (0x71D27); the controlled comparison stays only as the TRACKED_NONE
   * fallback. */
  {
    int32_t tracked_id = mr->render.camera.tracked;
    if (tracked_id != FIFA96_CAMERA_TRACKED_NONE)
      s.is_ball_track = tracked_id == id ? 1u : 0u;
    else
      s.is_ball_track = id == controlled ? 1u : 0u;
  }
  /* FU-147 S1: the BF20 lane block maintains `[team+0x7C7]`; the pool stores
   * it as the team-relative index. */
  s.is_team_7c7 = (mr->entities.team[team].tracker7c7 >= 0 &&
                   (int32_t)e->index == mr->entities.team[team].tracker7c7)
                      ? 1u
                      : 0u;
  s.is_team_7cb = 0;                /* [team+0x7CB] unported (OL-72) */
  s.user_present = controlled >= 0 ? 1u : 0u;
  s.user_is_self = id == controlled ? 1u : 0u;
  s.side = mr->entities.team[team].side;
  s.team_828 = mr->entities.team[team].slot_pool;
  s.team_7e7 = 0;                   /* [team+0x7E7] producer unported (OL-72) */
  s.team_7bf = mr->entities.team[team].chosen >= 0 ? 1u : 0u;
  s.merge_gate_1586d7 = 0;          /* [0x1586D7] producer unported (OL-72) */
  s.team_7d7 = (int32_t)mr->entities.team[team].duel_7d7;  /* [team+0x7D7] live
                                    * via the FU-169 action-0x0B arm 1 */
  s.opp_7d7 = (int32_t)mr->entities.team[opp].duel_7d7;    /* [[team+0x7A6]+0x7D7] */
  s.team_corner_z = 0;              /* team[+0x7E8+12n] producer unported (OL-72) */
  s.score_word[0] = (int16_t)mr->score[0];
  s.score_word[1] = (int16_t)mr->score[1];
  s.side_flip = 0;                  /* [0x157ABE] producer unported (OL-72) */
  /* FU-171 §7 rec 1 (T2/FU171-L3): native `dword[0x157750]` is the camera
   * triple's Y (the 0x7EDD8 admission and the 0x7EFFA seed switch), the same
   * cell the BF20 tail and the kick ctx already stage from `camera.pos_y`;
   * the FU-75 §1.6 ball-record stand-in is retired. */
  s.ball_height = (int16_t)mr->render.camera.pos_y;
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
  /* FU-171 §7 rec 2 (T2): the sound arm's `[tracked_owner+0x826]` compare
   * (0x7EE95/0x7EE9D) is the tracked record's team side; TRACKED_NONE keeps
   * the controlled stand-in. */
  {
    int32_t tracked_id = mr->render.camera.tracked;
    int32_t side_id = tracked_id != FIFA96_CAMERA_TRACKED_NONE ? tracked_id
                                                               : controlled;
    s.ball_track_side = side_id >= 0
                            ? mr->entities
                                  .team[(uint32_t)side_id / FIFA96_MATCH_ENTITY_RECORDS]
                                  .side
                            : 0;
  }
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
  /* FU-148 §2.1(c)/§6.2 + FU-152 §2.9 (T2): the row-04 half-line camera
   * event. The native call sites 0x7EFCF/0x7F0D1 pass FUN_00071C94 the
   * type-table step seed (<<6 / <<5) and height EBX = 0; arm A
   * (row byte 0x13, `event_track_reload`) also sets ECX = 1, the
   * FUN_00070544 ramp param, and its tail writes
   * `[0x1577FA] = F2 + F2/4` unconditionally (0x7EFD4 reads the high word of
   * [0x1577F0] = F2 = [0x1577F2]). The half-line branch sets `out.events`
   * with `event_code` 0 (its sinks are 0x974F0/0x651F0, not 71C94), so the
   * gate below is the event code. This is the natural gameplay-row pan origin
   * the FU-145 armer consumes. */
  if (out.events != 0 && out.event_code != 0) {
    int rc2 = fifa96_camera_event_set(&mr->render.camera, out.event_x, out.event_z,
                                      0, out.event_track_reload);
    if (rc2 < 0) return rc2;
    /* M2 phase-9 T3 (0x71D27): FUN_00071C94 stores the event's actor record as
     * the camera-tracked entity `[0x1577CA] = rec`. */
    (void)fifa96_camera_set_tracked(&mr->render.camera, mr->record.entity_id);
    if (out.event_track_reload != 0) {
      /* 0x7EFD4: [0x1577FA] = F2 + (F2 >> 2); F2 = ramp(event height), or 6
       * on the FUN_00070544 idle arm. */
      int16_t h = (int16_t)mr->render.camera.event_param;
      int16_t f2 = h > 0 ? fifa96_camera_ramp(h) : 6;
      mr->render.camera.timer = (uint16_t)(int16_t)(f2 + (f2 >> 2));
    }
  }
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
   * OL-72) and the half-line branch's `out.events` sinks (0x974F0/0x651F0;
   * the two 0x71C94 arms are wired above and carry event_code 0x15/0x1D) have
   * no derived consumer. */
  return FIFA96_OK;
}

/* ===== M2 phase-9 T2 / FU-142 OL-63: the row-05 carrier machine =====
 *
 * `fifa96_match_action_05` binds the ported `fifa96_action_carrier_arm`
 * (native `0x7F194..0x7F665` stages 0-3, FU-139 §8) to `mr->record` and the
 * FU-141 pool. The claim (`0x7F1BF..0x7F205`) is the `[0x158724]` possession
 * carrier producer — the pool `ball.carrier` is the carrier identity and the
 * derived `0x158728..0x15872F` block (`ball.pos_*`) is reset on a new claim —
 * and the stage-0 dir arm writes the `0x15872A/B` dir bytes (`0x7F386`/
 * `0x7F397`); the `0x7F356` `FUN_0007876C` merge request is the slot-merge
 * drain. The head also binds the team target (`[team+0x7B2] = rec`,
 * `+0x7B6 = 0`), adds the capped `+0x89` timer and copies the camera triple
 * into `+0x4D/+0x51/+0x55`; the controlled actor (`[0x157A83]`) is the
 * drained `controlled` request (set) or a direct clear (`0x7F274`). The
 * stage 1-3 surfaces are applied as in the loader contract: the `0x79B1C`
 * snap, the `0x79C50` face (+0x8E) and the stage-3 team-target hand-off
 * (install code 4 on the staged ball actor `[0x158730]` + the `0x79B58`
 * receiver timer on `[0x158734]`).
 *
 * Residual OL-63 legs (no derived consumer): the stage-0 target algebra
 * `0x7F3A1..0x7F57B` (reported as `out.tail`) and the `FUN_0007F7E0`
 * fallback (`out.fallback`); the `0x92820` sink (`out.sink`, OL-27), the
 * unported `0x1577BE/C0/C2` camera-velocity zero (`out.camera_zero`) and the
 * `0x6E598` animation resolution (`out.anim`, OL-52) are presentation-side
 * requests. */
static int fifa96_match_action_05(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_entity *e;
  struct fifa96_match_team *team;
  fifa96_action_carrier c;
  fifa96_action_carrier_out out;
  fifa96_action_possession p;
  int32_t id = r->entity_id;
  uint32_t team_index;
  int rc;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  team_index = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  team = &mr->entities.team[team_index];
  e = &team->records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
  memset(&c, 0, sizeof c);
  c.actor = id;
  c.timer89 = r->timer89;
  c.lane = e->lane_x;                    /* native dword[+0x69]>>16 = +0x6B */
  c.ball_height = mr->entities.ball.y;   /* [0x157750] stand-in, FU-75 §1.6
                                          * (leg 15.b; not the exact cell) */
  c.timer81 = (int16_t)r->timer81;
  c.close_word = e->lane_x;              /* native word +0x6B */
  c.bound_word = e->bound;               /* native word +0x77 */
  c.delta = r->delta;
  c.phase = (uint8_t)mr->state.phase;
  c.stage92 = r->stage92;
  c.active = r->active;
  c.airborne = r->pos_y != 0 ? 1u : 0u;  /* dword +0x5D != 0 */
  c.has_slot = r->has_slot;
  c.slot_live = (r->has_slot != 0 && mr->slot.entity == id &&
                 mr->slot.released != 0)
                    ? 1u
                    : 0u;  /* native word[slot+6]: the FU-70 released word */
  c.type8 = r->actor_type;             /* native byte +0x8E; the pool
                                        * actor_type writer is unported
                                        * (always 0, OL-83) */
  c.code = r->code;
  c.event_flag44 = r->row44;           /* native +0x44 */
  c.is_team_target = id == team->target ? 1u : 0u;
  c.facing = r->type;                  /* native +0x8E low byte */
  c.team_slot_pool = team->slot_pool;  /* byte [team+0x828] */
  c.team_chosen = team->chosen >= 0 ? 1u : 0u;   /* [team+0x7BF] != 0 */
  c.team_search_gate = team->search_gate;        /* byte [team+0x829] */
  c.slot_dir_x = r->dir_x;             /* slot[+0x1D]>>24 = slot+0x20 */
  c.slot_dir_z = r->dir_z;             /* slot[+0x1E]>>24 = slot+0x21 */
  p.carrier = mr->entities.ball.carrier;
  p.index = mr->entities.ball.pos_index;
  p.rotation = mr->entities.ball.pos_rotation;
  p.dir_x = mr->entities.ball.pos_dir_x;
  p.dir_z = mr->entities.ball.pos_dir_z;
  p.counter_c = mr->entities.ball.pos_counter_c;
  p.release_timer = (int8_t)mr->entities.ball.pos_release;
  p.counter_e = mr->entities.ball.pos_counter_e;
  p.counter_f = mr->entities.ball.pos_counter_f;
  rc = fifa96_action_carrier_arm(&p, &c, match_kick_dir_x, match_kick_dir_z, &out);
  if (rc != FIFA96_OK) return rc;
  if (out.ran_set != 0) r->ran = 1;      /* 0x7F19F */
  r->timer89 = c.timer89;                /* 0x7F221..0x7F23A */
  r->stage92 = out.stage;
  if (out.reset != 0) match_row_reset(mr, e);
  if (out.claim != 0) {                  /* 0x7F1C7/0x7F1FF */
    mr->entities.ball.carrier = p.carrier;
    mr->entities.ball.pos_index = p.index;
    mr->entities.ball.pos_rotation = p.rotation;
    mr->entities.ball.pos_dir_x = p.dir_x;
    mr->entities.ball.pos_dir_z = p.dir_z;
    mr->entities.ball.pos_counter_c = p.counter_c;
    mr->entities.ball.pos_release = (uint8_t)p.release_timer;
    mr->entities.ball.pos_counter_e = p.counter_e;
    mr->entities.ball.pos_counter_f = p.counter_f;
  }
  if (out.team_target != 0) {            /* 0x7F20B..0x7F217 */
    team->target = id;
    team->second = FIFA96_MATCH_ENTITY_NONE;
  }
  if (out.set_control != 0) r->controlled = 1;   /* 0x7F291 [0x157A83] = rec */
  if (out.clear_control != 0) {                  /* 0x7F274 [0x157A83] = 0 */
    mr->entities.controlled = FIFA96_MATCH_ENTITY_NONE;
    r->controlled = 0;
  }
  if (out.target_camera != 0) {          /* 0x7F24E..0x7F258 */
    r->target_x = mr->render.camera.pos_x;
    r->target_y = mr->render.camera.pos_y;
    r->target_z = mr->render.camera.pos_z;
  }
  if (out.dirs != 0) {                   /* 0x7F386/0x7F397 */
    mr->entities.ball.pos_dir_x = (int8_t)out.dir_x;
    mr->entities.ball.pos_dir_z = (int8_t)out.dir_z;
  }
  if (out.slot_merge != 0) r->helper_request = 1;   /* 0x7F356 */
  if (out.snap != 0) {                   /* 0x7F5F1 (0x79B1C) */
    r->target_x = r->pos_x;
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
  }
  if (out.face != 0) r->type = out.face; /* 0x7F607 (0x79C50); the loader's
                                          * face == 0 also means "no face
                                          * arm" (shared with rows 08/0F/28),
                                          * so a genuine octant 0 is dropped */
  if (out.handoff != 0) {                /* 0x7F637..0x7F657 */
    int32_t actor = mr->entities.ball.pair.actor;       /* [0x158730] */
    int32_t receiver = mr->entities.ball.pair.receiver; /* [0x158734] */
    if (actor >= 0 &&
        actor < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
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
  s.byte44 = r->row44;              /* FU-159 §4.1/T3: the anim-row terminal
                                     * flag (derived at the dispatch staging) */
  s.byte3d = r->frame;              /* native +0x3D, staged live (OL-80) */
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
  s.ball_height = mr->entities.ball.y;   /* [0x157750] stand-in, FU-75 §1.6
                                          * (leg 15.b; not the exact cell) */
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
  s.row_byte = mr->record.anim_id;        /* [[rec+0x28]] live id (OL-80) */
  s.byte99 = 0;                           /* +0x99 (OL-69) */
  s.byte9d = 0;                           /* +0x9D (OL-69) */
  s.desc_c = 0;                           /* rec[+4][+0xC] (OL-69) */
  s.desc_e = 0;                           /* rec[+4][+0xE] (OL-69) */
  s.byte90 = 0;                           /* +0x90 (OL-69) */
  s.parity = mr->pass_parity;             /* [0x157A4F] (0x4B11A) */
  s.byte_15872f = (int8_t)mr->entities.ball.pos_counter_f;  /* row-05 block */
  s.adjust_x = mr->entities.ball.pos_dir_x;                 /* 0x15872A (T2) */
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
  s.ball_height = mr->entities.ball.y;    /* [0x157750] stand-in, FU-75 §1.6
                                           * (leg 15.b; not the exact cell) */
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

/* ===== FU-169 (phase-14 T1): the live row-0x5E installer chain ============
 *
 * The only code that installs anim row 0x5E is the action-0x0C contact body's
 * stage-0 arm (`0x81DA4..0x81DB9`); its live entry is the action-0x0C install
 * on the challenged record by the action-0x0B duel arm 1 (`0x81BEB`), whose
 * own install producers are already wired (the pad tackle input arm and the
 * row-04 rare arm). The two new bodies below are the whole chain.
 *
 * `match_opponent_nearest` is the `FUN_0008DE8C` pick over the opponent team
 * block (`[[rec]+0x7A6]`): nearest to the point, `EBX = -1` (no skip), the
 * `+0x98`/`+0x9A` exclusions. The native returns the record pointer (0 when
 * no candidate); the engine returns the team-relative index or -1 (the
 * return-0 path is out-of-contract in the native — the callers deref it). */
static int32_t match_opponent_nearest(struct fifa96_match_run *mr, uint32_t team,
                                      int16_t from_x, int16_t from_z) {
  fifa96_entity_candidate candidates[FIFA96_MATCH_ENTITY_RECORDS];
  uint32_t opp = 1u - team;
  int16_t best = 0;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[opp].records[i];
    candidates[i].x = (int16_t)e->pos_x;
    candidates[i].y = (int16_t)e->pos_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  return fifa96_entity_find_nearest(candidates, FIFA96_MATCH_ENTITY_RECORDS,
                                    (uint32_t)0xFFFF, from_x, from_z, &best);
}

/* The encoded-id record resolver (the action-0x0C A/B reads). */
static struct fifa96_match_entity *match_entity_by_id(struct fifa96_match_run *mr,
                                                      int32_t id) {
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return NULL;
  return &mr->entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS]
              .records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
}

/* Row 0x0B `0x81908..0x81C71` (FU-169 §3, fresh `disassemble_bytes` this
 * task): the **duel body**. Arm 0 (`0x81951..0x81B19`) picks the contact anim
 * `{0x0C,0x59}` for the record and installs it on itself; arm 1
 * (`0x81B1F..0x81C4F`) registers the contact (kind 2) and installs action
 * 0x0C on the found opponent; arm 2 (`0x81C55`) snaps the record and frees it
 * through the `+0x44` terminal gate. The native's arm-0 fall-through into arm
 * 1 is inert (the fresh anim frame is 0, so the arm-1 frame gate tails); the
 * port skips it. Arm 1's `choice == 0x59` compare (`0x81B63`) reads the local
 * zeroed at the body head (`0x8191D`) on every entry, so it never fires — the
 * gate is the `<= 0x60` band alone (FU-169 status erratum).
 *
 * Legs: the ring-post branch `[[rec+4]] == 0x18DE` (FU169-L3, descriptor
 * unmodeled); the `%10` event 0xA1, the `0x8ED40(rec,-3)` impact accumulator
 * and the `0x741B4`/`0x92040`/`0x651F0`/`0x974F0` sinks (FU169-L2); the
 * `0x79B6C` tail's anim re-anchor `0x6E48C` and the transient row-0/0x26
 * install (FU169-L8). */
static int fifa96_match_action_0B(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  uint32_t team = 0;
  struct fifa96_match_entity *e = match_sp_entity(mr, &team, NULL);
  if (e == NULL) return -FIFA96_ERR_INVALID;
  if (mr->state.phase != 2u) {                       /* 0x8191B..0x81924 */
    match_row_reset(mr, e);
    return FIFA96_OK;
  }
  if (r->stage92 == 0u) {                            /* arm 0: 0x81951 */
    int32_t found_index;
    struct fifa96_match_entity *found;
    fifa96_entity_intercept_band_out band;
    int32_t point_x, point_z;
    int32_t face = r->face7d;
    uint8_t sector = r->type;
    uint8_t choice;
    if (r->active == 0u) {                           /* 0x81951/0x81958 */
      match_row_reset(mr, e);
      return FIFA96_OK;
    }
    /* 0x8195E..0x819B3: the `[[rec+4]] == 0x18DE` ring-post branch is
     * provably unreachable in the engine (descriptor staged 0, FU169-L3). */
    /* 0x819C2..0x81A02: the derived point = pos + dir[type]<<5. */
    point_x = r->pos_x + (int32_t)fifa96_match_dir_x[r->type & 7u] * 0x20;
    point_z = r->pos_z + (int32_t)fifa96_match_dir_z[r->type & 7u] * 0x20;
    found_index = match_opponent_nearest(mr, team, (int16_t)point_x, (int16_t)point_z);
    if (found_index < 0) { match_row_reset(mr, e); return FIFA96_OK; }
    found = &mr->entities.team[1u - team].records[found_index];
    /* 0x81A21..0x81A43: the band gate (0x20,0x70), strict. */
    (void)fifa96_entity_intercept_band(r->pos_x, r->pos_z, found->pos_x,
                                       found->pos_z, &band);
    if ((int32_t)band.band <= 0x20 || (int32_t)band.band >= 0x70) {
      match_row_reset(mr, e);
      return FIFA96_OK;
    }
    /* 0x81A49..0x81ACF: the speed/facing choice. */
    if (r->speed71 <= 4) {
      choice = 0x0Cu;
    } else {
      int32_t angle = 0;
      int32_t diff;
      if (fifa96_entity_angle(band.dx, band.dz, &angle) != FIFA96_OK)
        return FIFA96_OK;
      diff = (int32_t)r->face7d - angle;
      if (diff < 0) diff = -diff;
      choice = diff < 0x1000 ? 0x59u : 0x0Cu;
    }
    /* 0x81ADA/0x81AE2: FUN_00079B6C — the commit subset (pos y 0, target :=
     * pos, the lane low word / velocity / body-timer clears) and the
     * FUN_00079C50 face toward the found record. The tail's 0x6E48C anim
     * re-anchor and the transient row-0 install are FU169-L8. */
    r->pos_y = 0;
    r->target_x = r->pos_x;
    r->target_y = 0;
    r->target_z = r->pos_z;
    r->lane = (int32_t)((uint32_t)r->lane & 0xFFFF0000u);
    r->speed71 = 0;
    r->vel73 = 0;
    r->vel75 = 0;
    r->vel_x = 0;
    r->vel_z = 0;
    r->body_timer9c = 0;
    if (fifa96_entity_face(found->pos_x - r->pos_x, found->pos_z - r->pos_z,
                           &face, &sector) == FIFA96_OK) {
      r->face7d = (int16_t)face;
      r->type = sector;
    }
    /* 0x81AFB: the choice anim on self, frame 0. */
    if (fifa96_match_run_record_anim_install(mr, e, (int32_t)choice, 0) != FIFA96_OK)
      return FIFA96_OK;
    r->anim_id = e->anim_id;
    r->frame = e->frame;
    r->row44 = e->row44;
    r->ran = 1;                                      /* 0x81B00 */
    r->timer89 = 0;                                  /* 0x81B0D */
    r->stage92 = 1u;                                 /* 0x81B19 */
  }
  if (r->stage92 == 1u) {                            /* arm 1: 0x81B1F */
    int32_t found_index;
    int32_t found_id;
    struct fifa96_match_entity *found;
    fifa96_entity_intercept_band_out band;
    int32_t point[3];
    if (r->frame == 0u) return FIFA96_OK;            /* 0x81B1F/0x81B23 */
    found_index = match_opponent_nearest(mr, team, (int16_t)r->pos_x,
                                         (int16_t)r->pos_z);
    if (found_index < 0) { match_row_reset(mr, e); return FIFA96_OK; }
    found = &mr->entities.team[1u - team].records[found_index];
    found_id = (int32_t)((1u - team) * FIFA96_MATCH_ENTITY_RECORDS +
                         (uint32_t)found_index);
    if (found->active == 0u) {                       /* 0x81B43..0x81B4D */
      match_row_reset(mr, e);
      return FIFA96_OK;
    }
    (void)fifa96_entity_intercept_band(r->pos_x, r->pos_z, found->pos_x,
                                       found->pos_z, &band);
    if ((int32_t)band.band > 0x60) {                 /* 0x81B53..0x81B5D */
      match_row_reset(mr, e);
      return FIFA96_OK;
    }
    /* 0x81B63: the dead `[ESP+0x14] == 0x59` compare (head-zeroed local). */
    if (mr->session_gate_14c32a != 0u) {             /* 0x81B73 */
      match_row_reset(mr, e);
      return FIFA96_OK;
    }
    mr->entities.team[team].duel_7d7++;              /* 0x81B80/0x81B83 */
    /* 0x81B89..0x81BB7: the `%10` event 0xA1 request (FU169-L2).
     * 0x81BB7/0x81BBE: FUN_0008ED40(rec, -3) impact accumulator (FU169-L2). */
    point[0] = found->pos_x;
    point[1] = found->pos_y;
    point[2] = found->pos_z;
    if (fifa96_ref_contact_register(&mr->referee, 2u, r->entity_id, found_id,
                                    point) != FIFA96_OK)
      return FIFA96_OK;
    found->type = r->type;                           /* 0x81BDF..0x81BE3 */
    /* 0x81BEB: the action-0x0C install on the found record (no invoke). */
    (void)fifa96_match_entities_install(found, (uint8_t)mr->state.phase, 0x0Cu, 0);
    /* 0x81BF0..0x81C2E: the side sinks (FU169-L2). */
    r->timer89 = 0;                                  /* 0x81C43 */
    r->stage92 = 2u;                                 /* 0x81C4F */
    /* the native falls through into arm 2 (0x81C55) — handled below */
  }
  if (r->stage92 == 2u) {                            /* arm 2: 0x81C55 */
    /* FUN_00079B1C: target := pos; the movement words cleared. */
    r->target_x = r->pos_x;
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
    r->lane = 0;
    r->speed71 = 0;
    r->vel73 = 0;
    r->vel75 = 0;
    r->vel_x = 0;
    r->vel_z = 0;
    r->body_timer9c = 0;
    if (r->row44 != 0u) match_row_reset(mr, e);      /* 0x81C5C..0x81C64 */
  }
  return FIFA96_OK;
}

/* The action-0x0C stage-0 foul branch (`0x81DFE..0x81E86` + the
 * `0x81E8A..0x81ED4` settings/re-call block + the `0x81ED5..0x81F48` tail).
 * `a` is the registered initiator, `e` the challenged record's pool entity.
 * When A's row is not 0x11 the challenged record's velocity is written first
 * (`0x81E13..0x81E86`): **B's speed magnitude against A's facing** —
 * `0x81DFE MOV EAX,[0x15888F]` (A) -> `0x81E13 MOV EAX,[EAX+0x7B]; SAR 0x10`
 * = `word[A+0x7D]` (again `0x81E49` for the +0x100 term), while the
 * magnitude is `0x81E36 MOV EBX,[EBP+0x6F]; SAR 0x10` = B's speed71
 * (FU-160 §1.1's "EBP's facing-relative +0x73/+0x75 words" phrasing is the
 * erratum: the words are B's, the facing is A's).
 *
 * The re-call runs through the ported `fifa96_match_run_contact` with the
 * registered A/B/point — the disclosed kind-1 re-register (FU-169 §7 rec
 * #1). Its tail (`0x81ED5..0x81F48`) then runs on **every** foul exit: the
 * `+0x98` latch, one RNG draw -> the anim row 0x0D/0x52 install on B, `ran`,
 * `timer89 = 0` and the stage 0 -> 1 advance (otherwise B re-fouls every
 * frame). The whistle `0x974DC(0x1F)` (`0x81F06`) and the `[team+0x7B2]==B`
 * re-pick (`0x81F10..0x81F2F`) are unported (FU169-L9); the native falls
 * into the (leg) stage-1 arm after the advance. `a == NULL` is out-of-contract
 * in the native (`0x81DCF` dereferences the NULL slot) and returns as a
 * retry. */
static int match_sp_action_0c_foul(struct fifa96_match_run *mr,
                                   struct fifa96_match_entity *e,
                                   struct fifa96_match_entity *a) {
  struct fifa96_match_run_record *r = &mr->record;
  int32_t point[3];
  uint16_t draw = 0;
  if (a == NULL) return FIFA96_OK;
  if (a->anim_id != 0x11u) {
    int16_t vel73;
    int16_t vel75;
    /* 0x81E13..0x81E40 / 0x81E49..0x81E81: mul16(B.speed71, sine(A.face7d))
     * and the +0x100 term. */
    vel73 = (int16_t)(((int64_t)r->speed71 * fifa96_entity_sine(a->face7d) +
                       0x8000) >> 16);
    vel75 = (int16_t)(((int64_t)r->speed71 *
                           fifa96_entity_sine((int32_t)a->face7d + 0x100) +
                       0x8000) >> 16);
    r->vel73 = vel73;
    r->vel75 = vel75;
    r->vel_x = (int32_t)((uint32_t)(uint16_t)r->speed71 |
                         ((uint32_t)(uint16_t)vel73 << 16));
    r->vel_z = (int32_t)((uint32_t)(uint16_t)vel73 |
                         ((uint32_t)(uint16_t)vel75 << 16));
  }
  point[0] = mr->referee.point[0];
  point[1] = mr->referee.point[1];
  point[2] = mr->referee.point[2];
  (void)fifa96_match_run_contact(mr, 1u, mr->referee.rec_first,
                                 mr->referee.rec_second, point);
  /* 0x81ED5..0x81F48: the foul tail (both the 0x81ECB jump and the 0x81ECD
   * clear land here). */
  e->skip_98 = 1;                                       /* 0x81ED5 */
  if (fifa96_rng_step(&mr->rng, &draw) == FIFA96_OK) {  /* 0x81EE2 */
    uint8_t row = (draw & 3u) != 0u ? 0x0Du : 0x52u;    /* 0x81EEC..0x81EF7 */
    if (fifa96_match_run_record_anim_install(mr, e, (int32_t)row, 0) == FIFA96_OK) {
      r->anim_id = e->anim_id;                          /* 0x81F01 write-back */
      r->frame = e->frame;
      r->row44 = e->row44;
    }
  }
  r->ran = 1;                                           /* 0x81F2F */
  r->timer89 = 0;                                       /* 0x81F3C */
  r->stage92 = 1u;                                      /* 0x81F48 (0 -> 1) */
  return FIFA96_OK;
}

/* The action-0x0C stage-0 alternate gate (`0x81DCF..0x81DF8`): A not in row
 * 0x59. Row 0x11 needs frame >= 2, any other row frame >= 3; below the
 * threshold the record retries (the bare `0x82502` tail). */
static int match_sp_action_0c_alternate(struct fifa96_match_run *mr,
                                        struct fifa96_match_entity *e,
                                        struct fifa96_match_entity *a) {
  int32_t threshold;
  if (a == NULL) return FIFA96_OK;   /* the native NULL deref: bounded retry */
  threshold = a->anim_id == 0x11u ? 2 : 3;
  if ((int32_t)a->frame < threshold) return FIFA96_OK;
  return match_sp_action_0c_foul(mr, e, a);
}

/* Row 0x0C `0x81C90..0x8250B` (FU-169 §2, fresh `disassemble_bytes` this
 * task): the **contact-decision body** the duel installs on the challenged
 * record. Only the head, stage 0 and the foul tail are ported (the live
 * row-0x5E installer); stages 1..6 (`0x81F4E..0x8250B`, the stage table
 * `0x81C74` arms 1..6: `0x81F4E/0x81FA7/0x8244B/0x8247D/0x824CD/0x824F2`,
 * fresh this session; the doc's `..0x8251B` over-runs into the next body's
 * prologue at `0x8251C`) stay FU169-L4 legs. */
static int fifa96_match_action_0C(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_entity *e = match_sp_entity(mr, NULL, NULL);
  if (e == NULL) return -FIFA96_ERR_INVALID;
  /* 0x81C9D..0x81CA9: `timer89 += (uint16)[0x157A64]` (the FU-0x0E
   * zero-extension convention). */
  r->timer89 += (uint16_t)r->delta;
  /* 0x81CAB..0x81CC0: grounded -> target := pos. */
  if (r->pos_y == 0) {
    r->target_x = r->pos_x;
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
  }
  if ((uint8_t)r->stage92 > 6u) return FIFA96_OK;    /* 0x81CC1..0x81CC9 */
  if (r->stage92 == 0u) {                            /* 0x81CDC */
    struct fifa96_match_entity *a;
    fifa96_entity_intercept_band_out band;
    int32_t lead_x, lead_z;
    if (r->active == 0u) {                           /* 0x81CDC..0x81CE3 */
      match_row_reset(mr, e);
      return FIFA96_OK;
    }
    a = match_entity_by_id(mr, mr->referee.rec_first);   /* 0x81CE9 */
    if (a == NULL || a->anim_id != 0x59u)            /* 0x81CEF..0x81D07 */
      return match_sp_action_0c_alternate(mr, e, a);
    if ((int32_t)a->frame < 2) return FIFA96_OK;     /* 0x81D0D..0x81D16 */
    /* 0x81D1C..0x81D4F: the lead point A.pos + 2*A.vel (dword loads shifted
     * 16 -> the +0x73/+0x75 words) and the 0x795B4 band from B.pos. */
    lead_x = a->pos_x + 2 * (int32_t)a->vel73;
    lead_z = a->pos_z + 2 * (int32_t)a->vel75;
    (void)fifa96_entity_intercept_band(r->pos_x, r->pos_z, lead_x, lead_z,
                                       &band);
    if ((int32_t)band.band < 0x30)                   /* 0x81D54..0x81D5E */
      return match_sp_action_0c_foul(mr, e, a);
    /* 0x81D64/0x81D6B: the initiator team's +0x7CF stumble counter. */
    mr->entities.team[a->team].stumble_7cf++;
    /* 0x81D71..0x81D9F: the `%7 == 0 && [rec(B)+0x20] != 0` 0xA2 event
     * (FU169-L2). */
    /* 0x81DA4..0x81DB9: the anim row 0x5E install on A, frame 0. */
    if (fifa96_match_run_record_anim_install(mr, a, 0x5E, 0) != FIFA96_OK)
      return FIFA96_OK;
    match_row_reset(mr, e);                          /* 0x81DBE */
    return FIFA96_OK;
  }
  /* stages 1..6 (0x81F4E..0x8250B) are FU169-L4 legs. */
  return FIFA96_OK;
}

/* Row 17 `0x84730..0x849AF` (FU-181 §2, first-hand this slice; FU-81 §2 4-arm
 * table `0x84720`): the line-up walk. The phase gate `[0x157A4A]>>24 == 0xB`
 * (`0x84743`) else the `0x849A1` FUN_0007DAB4 reset tail; the `+0x89` timer
 * accumulates `[0x157A64]` (`0x8474F..0x84765`); the `+0x92` stage walk:
 *  - stage 0 (`0x84780`): stash the position triple into `0x1587FC`, target
 *    x := -0x680, target y := 0, target z := the ±0x50 facing from the
 *    position-z sign (`0x8479D..0x847C2`), the `0x8DCD4` commit, `+0x89` := 0,
 *    `+0x92`++ (falls into stage 1's timer wait);
 *  - stage 1 (`0x847E5`): wait `+0x89 >= 0x3C` with the `0x45001` ball-shape
 *    and `[+0x63]>>16 < 0x20` gates, then the `0x4C324` presentation exit and
 *    `+0x92`++ (falls into stage 2);
 *  - stage 2 (`0x8483F`): the per-team formation-matrix sync
 *    (`0x84842..0x848D1`), `0x8C758`/`0x6E598`, then position x := 0xFFFFF7A0
 *    (-0x860), y := 0, z := 0, the `0x1587FC` -> target copy, the `0x8DCD4`
 *    commit, `+0x89` := 0, `+0x92`++ (RET);
 *  - stage 3 (`0x8494C`): wait `+0x89 >= 0x3C` with the same gates, then the
 *    `0x4C324`/`0x4728C` presentations, the `0x1587FC` -> position copy and
 *    the `0x7DAB4` reset.
 * The formation matrices, `0x8DCD4`/`0x8C758`/`0x6E598`/`0x45001`/`0x4728C`/
 * `0x4C324` and the `0x10F370`/`0x1587FC` cells are FU181-L4 sinks: the derived
 * port keeps the derivable gate/timer/stage walk and the abort-free epilogue
 * (`0x8476D CMP AL,3; JA 0x849A6`). The row's only producer is act-4 stage 0
 * (`0x89740`, invoke-now), which the engine cannot reach until the
 * `FUN_0008A938` sit-2..4 formation-order gate `0x8AB08..0x8AB62` (FU-149 L12)
 * lands (FU181-L1): the action-table slot stays NULL (the rows-27/29/2C
 * precedent — a ported body with no engine installer is not counted) and the
 * body is reachable by direct call / the future act-4 seam. */
int fifa96_match_action_17(struct fifa96_match_run *mr) {
  static const uint8_t gates[1] = {0x0Bu};
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_team *team;
  struct fifa96_match_entity *e;
  fifa96_action_stage st;
  fifa96_action_stage_out out;
  int32_t id = r->entity_id;
  uint32_t t, idx;
  int rc;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  t = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  idx = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  team = &mr->entities.team[t];
  e = &team->records[idx];
  memset(&st, 0, sizeof st);
  st.phase = mr->state.phase;
  st.stage = r->stage92;
  st.timer89 = r->timer89;
  st.delta = r->delta;
  st.active = r->active;
  rc = fifa96_action_stage_enter(&st, gates, 1, &out);   /* 0x8473B..0x8474A */
  if (rc != FIFA96_OK) return rc;
  if (out.reset != 0) {                                   /* 0x8474A -> 0x849A1 */
    match_row_reset(mr, e);
    return FIFA96_OK;
  }
  fifa96_action_stage_tick(&st);                          /* 0x8474F..0x84765 */
  if (st.stage > 3u) {                                    /* 0x8476D JA */
    r->timer89 = st.timer89;
    return FIFA96_OK;
  }
  if (st.stage == 0u) {                                   /* 0x84780 */
    r->target_x = -0x680;
    r->target_y = 0;
    r->target_z = r->pos_z <= 0 ? -0x50 : 0x50;           /* 0x847A9..0x847C2 */
    st.timer89 = 0;
    st.stage = 1;                                         /* falls into stage 1 wait */
    r->timer89 = st.timer89;
    r->stage92 = st.stage;
    return FIFA96_OK;
  }
  if (st.stage == 1u && st.timer89 < 0x3C) {              /* 0x847E5..0x847EE */
    r->timer89 = st.timer89;
    r->stage92 = st.stage;
    return FIFA96_OK;
  }
  if (st.stage == 1u) {                                   /* 0x84813 advance */
    fifa96_action_stage_advance(&st, &out);               /* +0x89=0, stage 2 */
  }
  if (st.stage == 2u) {                                   /* 0x8483F */
    r->pos_x = -0x860;                                    /* 0x84918 */
    r->pos_y = 0;                                         /* 0x8490C */
    r->pos_z = 0;                                         /* 0x84903 */
    fifa96_action_stage_advance(&st, &out);               /* +0x89=0, stage 3 */
    r->timer89 = st.timer89;
    r->stage92 = st.stage;
    return FIFA96_OK;
  }
  /* stage 3 (0x8494C): wait then reset */
  if (st.timer89 >= 0x3C) {
    match_row_reset(mr, e);                               /* 0x849A1 */
    return FIFA96_OK;
  }
  r->timer89 = st.timer89;
  return FIFA96_OK;
}

/* ===== M2 phase-15 T2 / FU-176: row 0x1F keeper dive/arm ==================
 *
 * `fifa96_match_action_1F` binds the four ported keeper helpers to the native
 * row `0x76380..0x765C2` (FU-176 §2, first-hand this slice):
 *  - prologue slot-merge gate (`0x76388..0x7639B`): `[rec+0x20]==0 &&
 *    byte[team+0x828]!=0` -> `FUN_0007876C` (the `helper_request` drain);
 *  - `[0x157C5C]=0` (`0x763AB`);
 *  - `fifa96_keeper_arm_camera` (`0x763B1..0x76409`): stage < 2 writes the
 *    `(0,0,±0xB10)` target, stage < 1 arms the camera hook (the `0x79B6C`
 *    body is unported, FU176-L2);
 *  - the common timer `+0x89 += [0x157A64]` (`0x76409..0x7641F`);
 *  - `fifa96_keeper_arm_step` (`0x76425..0x76578`): the stage-0 event gate,
 *    the stage-1 ball-ack gate, the stage-2 slot branch and the stage-3 tail;
 *  - the stage-2 no-slot RNG dive (`0x764BA..0x7655C`): four RNG draws +
 *    `fifa96_keeper_dive_target`, then install code 0x1B invoke-now (the
 *    co-scoped second-order install);
 *  - the stage-3 tail input handler `FUN_00076130` (`0x76597`) via
 *    `fifa96_keeper_input_decide`, draining its 4/0x19/0x1C/0x1B installs
 *    invoke-now;
 *  - the `code==0x1F && [0x157A4C]==0` reset (`0x765AC..0x765B7`).
 * Sinks left unported: the `0x79B6C` camera hook, the `0x6E598` event 0x28 and
 * the `0x7876C` merge body ride request bits / are dropped (FU176-L2/L3). */
static int fifa96_match_action_1F(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_team *team;
  struct fifa96_match_entity *e;
  fifa96_keeper_point target;
  fifa96_keeper_arm_out arm;
  uint8_t stage;
  uint8_t slot_pressed;
  int32_t id = r->entity_id;
  uint32_t t, idx;
  int rc;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  t = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  idx = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  team = &mr->entities.team[t];
  e = &team->records[idx];

  if (r->has_slot == 0 && team->slot_pool != 0) r->helper_request = 1; /* 0x7639B */
  mr->global_157c5c = 0;                                              /* 0x763AB */

  stage = r->stage92;
  target.x = r->target_x;
  target.y = r->target_y;
  target.z = r->target_z;
  {
    uint8_t hook = 0;
    (void)fifa96_keeper_arm_camera(stage, team->side, mr->state.phase, &target, &hook);
    r->target_x = target.x;
    r->target_y = target.y;
    r->target_z = target.z;
  }
  r->timer89 += (int32_t)r->delta;                                    /* 0x7641F */

  slot_pressed = (uint8_t)(r->has_slot != 0 && mr->slot.entity == id &&
                                   mr->slot.pressed != 0
                               ? 1u
                               : 0u);
  rc = fifa96_keeper_arm_step(
      stage, mr->global_5882a,
      (uint8_t)(mr->entities.ball.pair.actor != 0 ? 1u : 0u),
      (uint8_t)(mr->entities.ball.pair.ack != 0 ? 1u : 0u), r->has_slot,
      slot_pressed, r->code, mr->global_157a4c, &arm);
  if (rc != FIFA96_OK) return rc;

  if (arm.flag_9e != 0) r->ran = 1;                                   /* 0x76440 */
  if (arm.stage != stage) r->timer89 = 0;   /* 0x7646C/0x7649E/0x7656E */
  r->stage92 = arm.stage;
  if (arm.copy_pos != 0) {                                           /* 0x76580 */
    r->target_x = r->pos_x;
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
  }

  if (arm.install_code == 0x1B && arm.invoke != 0) {
    fifa96_keeper_dive dive;
    uint16_t rng_a = 0, rng_b = 0, rng_c = 0, rng_d = 0;
    fifa96_keeper_point pos;
    pos.x = r->pos_x;
    pos.y = r->pos_y;
    pos.z = r->pos_z;
    (void)fifa96_rng_step(&mr->rng, &rng_a);
    (void)fifa96_rng_step(&mr->rng, &rng_b);
    (void)fifa96_rng_step(&mr->rng, &rng_c);
    (void)fifa96_rng_step(&mr->rng, &rng_d);
    /* divisor = 0x64 - 2*(int8)desc[0x12] - 2*(int8)desc[0x13] (`0x764C6..`); the
     * rec[+4] descriptor is unmodeled by the pool, so the derived base 0x64
     * stands (FU176-L4). */
    if (fifa96_keeper_dive_target(&pos, rng_a, 0x64u, rng_b,
                                  (int16_t)mr->entities.ball.pair.vector.height,
                                  rng_c, rng_d, &dive) == FIFA96_OK) {
      r->target_x = dive.target.x;
      r->target_y = dive.target.y;
      r->target_z = dive.target.z;
    }
    if (fifa96_match_entities_install(e, (uint8_t)mr->state.phase, 0x1Bu, 1u) == 1) {
      r->code = e->code;
      r->stage92 = e->stage92;
      r->timer89 = e->timer89;
      r->ran = e->ran;
      (void)fifa96_match_dispatch_action(mr, 0x1Bu);   /* the 0x1B chain */
    }
  }

  if (arm.run_handler != 0) {                                         /* 0x76597 */
    fifa96_keeper_input in;
    fifa96_keeper_decision dec;
    memset(&in, 0, sizeof in);
    in.has_slot = r->has_slot;
    in.phase = mr->state.phase;
    in.human_phase = mr->global_157a4c;
    in.event_flag = mr->global_5882a;
    in.team_side = team->side;
    in.code = r->code;
    in.type_gate = fifa96_outfield_type_bits(r->code);
    in.is_actor = (uint8_t)(mr->entities.controlled == id ? 1u : 0u);
    in.lane = (int16_t)(r->lane >> 16);
    in.cam_x = mr->keeper_cam_x;
    in.cam_y = mr->keeper_cam_y;
    in.cam_z = mr->keeper_cam_z;
    in.pos_x = r->pos_x;
    in.pos_z = r->pos_z;
    in.slot_dir_x = r->dir_x;
    in.slot_dir_z = r->dir_z;
    if (fifa96_keeper_input_decide(&in, &dec) == FIFA96_OK && dec.install != 0) {
      if (dec.copy_cam != 0 || dec.copy_pos != 0 || dec.clear_c5c != 0) {
        r->target_x = dec.target_x;
        r->target_y = dec.target_y;
        r->target_z = dec.target_z;
      }
      if (dec.clear_c5c != 0) mr->global_157c5c = 0;
      if (dec.invoke != 0) {
        uint8_t staged = (uint8_t)(dec.clear_c5c != 0 ? 1u : 0u); /* dir arm EBX=1 */
        if (fifa96_match_entities_install(e, (uint8_t)mr->state.phase, dec.install,
                                          staged) == 1) {
          r->code = e->code;
          r->stage92 = e->stage92;
          r->timer89 = e->timer89;
          r->ran = e->ran;
          (void)fifa96_match_dispatch_action(mr, dec.install);
        }
      }
    }
  }

  if (arm.reset != 0) match_row_reset(mr, e);                         /* 0x765B5 */
  return FIFA96_OK;
}

/* ===== M2 phase-15 T2 / FU-180: row 0x1B keeper reposition/rush ============
 *
 * `fifa96_match_action_1B` is the bounded derived body of `0x76D28..0x77727`
 * (FU-180 §2, first-hand this slice; 5-arm table `0x76D14` =
 * `{0x76DF6,0x76E3B,0x76FC7,0x7769F,0x776C1}`), co-scoped with row 0x1F (whose
 * no-slot dive installs it invoke-now). Derivable:
 *  - head (`0x76D31..0x76DC5`): the phase-0xD + `[0x157AAC]>>24` side gate,
 *    the per-sector camera place into `[0x15774C/50/54]`, `+0x9B=1` (take) and
 *    `[0x157A83]=rec`; then the `+0x89 += delta` timer and the `>4` tail;
 *  - stage 0 (`0x76DF6`): `[+0x9E]=1`, `[+0x89]=0`, `+0x92++`, clear the
 *    `0x157C54..0x157C5B` latches (FU180-L7);
 *  - stage 1 (`0x76E3B`): the derivable `+0x51 += 0x20`; the `0x8DCD4`/
 *    `0x79C50`/`0xCD474` metric/face/heading and the `0x114E04` sine rotation
 *    are FU180-L1/L2;
 *  - stage 2 (`0x76FC7` -> `0x77636`): the rush gates; all roads reach
 *    `0x77636`, which either tails on `+0x44==0` or advances to stage 3/4; the
 *    wide body incl. the `0x71C94` call (`0x77423`) is FU180-L1;
 *  - stage 3 (`0x7769F`): the `+0x44` gate then `+0x89=0`, `+0x92++`;
 *  - stage 4 (`0x776C1`): `+0x92=4`, `FUN_0007DAB4`, target=pos, then the
 *    ported `fifa96_keeper_reposition_b_finish` (not-controlled -> install
 *    0x19; controlled+CPU -> situation 5; controlled+human -> nothing).
 * The `FUN_00076B28` steering, the record-0 machine and the CPU escalation
 * ladder carrying the second installer stay FU180-L3/L4/L5. */
static int fifa96_match_action_1B(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_team *team;
  struct fifa96_match_entity *e;
  uint8_t stage;
  uint8_t actor_is_rec;
  int32_t id = r->entity_id;
  uint32_t t, idx;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  t = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  idx = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  team = &mr->entities.team[t];
  e = &team->records[idx];

  /* Head (0x76D31..0x76DBF). The per-sector offsets are the native
   * `[EAX+0x10F331]>>24` / `[EAX+0x10F339]>>24` (the byte at +3 = the 0x1B
   * tables `0x10F334[sector]` / `0x10F33C[sector]`, FU180 §2). */
  if (mr->state.phase == 0x0Du &&
      team->side == mr->phase_machine.side_controlled) {
    int32_t cam_x = r->pos_x + (int32_t)r->place_offset_x * 16;  /* 0x10F331[sector]>>24 <<4 */
    int32_t cam_z = r->pos_z + (int32_t)r->place_offset_z * 16;  /* 0x10F339[sector]>>24 <<4 */
    int32_t cam_y = r->pos_y + 0x38;                             /* 0x76D9F */
    mr->keeper_cam_x = cam_x;                                    /* 0x76D6A */
    mr->keeper_cam_z = cam_z;                                    /* 0x76D81 */
    mr->keeper_cam_y = cam_y;                                    /* 0x76DB5 */
    r->place_x = cam_x;                                          /* 0x76DBA FUN_000700F4 */
    r->place_y = cam_y;
    r->place_z = cam_z;
    r->place_valid = 1;
    r->has_ball = 1;                                             /* 0x76DAE */
    r->controlled = 1;                                           /* 0x76DBF */
  }
  /* [0x157A83] readback (0x7764D/0x776D8): the native reads the live actor
   * cell immediately; the head's `[rec+0x20]-independent` bind lands on
   * `record.controlled`, which the frame commit later drains into the pool
   * cell `entities.controlled`. Within one dispatch (install staged=1 can run
   * stage 1->2->4 in a single call) the pool cell still lags, so the
   * controlled-actor test is the OR of the head's record write and the
   * persisted pool value. */
  actor_is_rec = (uint8_t)(r->controlled != 0 || mr->entities.controlled == id
                               ? 1u
                               : 0u);
  r->timer89 += (int32_t)r->delta;                               /* 0x76DC5 */
  stage = r->stage92;
  if (stage > 4u) return FIFA96_OK;                              /* 0x76DE1 JA */
  if (stage == 0u) {                                             /* 0x76DF6 */
    r->ran = 1;
    r->timer89 = 0;
    /* 0x76E0F..0x76E2F: the `0x157C54..0x157C5B` latch clear. Fresh
     * `disassemble_bytes 0x76E0F` (phase-15 T3) gives the exact six byte stores
     * — 0x157C59/5B/5A/57/58 and 0x157C54 — so keeper_latch[1] = 0x157C55 and
     * keeper_latch[2] = 0x157C56 are **not** cleared (FU180 §2's "all eight" is
     * imprecise; see the FU-180 errata). FU180-L7: the stage-2 rush gate reads
     * `byte[0x157C55]`, so those two must survive. */
    mr->keeper_latch[0] = 0;   /* 0x157C54 */
    mr->keeper_latch[3] = 0;   /* 0x157C57 */
    mr->keeper_latch[4] = 0;   /* 0x157C58 */
    mr->keeper_latch[5] = 0;   /* 0x157C59 */
    mr->keeper_latch[6] = 0;   /* 0x157C5A */
    mr->keeper_latch[7] = 0;   /* 0x157C5B */
    stage = 1u;                                                  /* 0x76E2F..0x76E35 */
  }
  if (stage == 1u) {                                             /* 0x76E3B */
    r->target_y += 0x20;                                         /* 0x76E7D */
    stage = 2u;                                                  /* 0x76FC1 */
  }
  if (stage == 2u) {                                             /* 0x76FC7 */
    if (r->row44 == 0) { r->stage92 = stage; return FIFA96_OK; }  /* 0x7763A */
    if (mr->global_157a4c != 1u || actor_is_rec == 0)
      stage = 4u;                                                /* 0x7764B/0x77655 -> 0x776C1 */
    else
      stage = 3u;   /* the 0x79C50/0x6E598 actor event is FU180-L1 */
  }
  if (stage == 3u) {                                             /* 0x7769F */
    if (r->row44 == 0) { r->stage92 = stage; return FIFA96_OK; }  /* 0x776A3 */
    r->timer89 = 0;                                              /* 0x776AF */
    stage = 4u;                                                  /* 0x776B9 */
  }
  /* stage 4 (0x776C1). */
  r->stage92 = 4u;                                               /* 0x776C3 */
  match_row_reset(mr, e);                                        /* 0x776CA */
  r->target_x = r->pos_x;                                        /* 0x776D5 */
  r->target_y = r->pos_y;
  r->target_z = r->pos_z;
  {
    uint8_t install_code = 0;
    uint8_t notify_code = 0;
    (void)fifa96_keeper_reposition_b_finish(
        actor_is_rec, (uint8_t)(mr->global_157a4c == 1u ? 1u : 0u), &install_code,
        &notify_code);
    if (install_code != 0)                                       /* 0x7770E install 0x19 */
      (void)fifa96_match_entities_install(e, (uint8_t)mr->state.phase, install_code, 0);
    if (notify_code != 0)                                        /* 0x776F7 FUN_0008A938(5,side,1) */
      return fifa96_match_run_situation(mr, notify_code);
  }
  return FIFA96_OK;
}

/* ===== M2 phase-16 T3 / FU-186: rows 0x1A / 0x1C keeper chain =============
 *
 * `fifa96_match_action_1C` (row 0x1C, **counted**) is the
 * bounded derived body of `0x77728..` (FU-186 §2.2, first-hand
 * `disassemble_bytes 0x77728` this slice). The row's installer is **live**:
 * the wired `fifa96_match_action_1F` stage-3 tail drains
 * `fifa96_keeper_input_decide`, whose both-slot-dirs-zero arm emits
 * `install=0x1C, invoke=1` (`FUN_00076130` `0x76291`->`0x762A4`), and the tail
 * then calls `fifa96_match_entities_install(...,0x1C,...)` +
 * `fifa96_match_dispatch_action(mr,0x1C)` (FU-186 §4). So wiring this slot
 * makes the route execute end-to-end. Derivations (fresh this slice):
 *  - head `0x77735`: `timer89 += (uint16)delta`;
 *  - stage 0 `0x77762`: `ran=1` (`+0x9E`), `timer89=0`, `keeper_latch[2]=0`
 *    (`0x157C56`), `stage92` -> 1, then it falls through into stage 1;
 *  - stage 1 `0x7778E`: `fifa96_arm_dist_stage(pos,target,&distance,&lane)`
 *    (the `0x8DCD4` out triple `+0x65` distance / `+0x67` dx / `+0x69` dz), the
 *    `0x79C50` face (its inputs are the unmodeled `0x1577C0/C2` leads, and the
 *    `+0x7F` heading cell has no home — both FU186-L2 legs), then the
 *    `fifa96_keeper_lunge_track` on-target arm gated `|angle(distance,
 *    target_y)| < 0x55` (`0x777DE`): the `pos` shift
 *    `+= trunc_shift(dx|dz,1)` (`0x8DC50`) + event 0x3C. The `0x77856` rotated
 *    arm (`0x114E04` sine + `0x795A4`), the `0x6E598` anim install and the
 *    `0x76B28` steering are FU186-L2 legs; then `timer89=0`, `stage92` -> 2
 *    (`0x779B9`);
 *  - stage 2 `0x779D1`: `row44 != 0` -> `match_row_reset` (`0x7DAB4`);
 *  - the tail `0x779DE..0x77E8B` (the `word[+0x81]`/memory-mailbox block) is
 *    FU186-L2.
 *
 * `fifa96_match_action_1A` (row 0x1A) is the bounded head + stage-0 body of
 * `0x7662C..` (FU-186 §2.1, first-hand `0x7662C`/`0x76618`): the phase-0xD +
 * `[0x157AAC]>>24` side bind (`[0x157A83]=rec`, `0x76641`), the `+0x89 +=
 * delta` timer, the `+0x92 > 4` tail (`0x7667E`), and stage 0
 * (`fifa96_keeper_reposition_a_gate`, the `0x765C4` tracked-teammate branch:
 * the install-4 invoke-now arm `0x7669D` or the `+0x9E=1`/`+0x89=0`/`+0x92+=2`
 * arm `0x766BE` + the `0x79B1C` snap). The row's only installer is the
 * unported `FUN_00077EAC` CPU ladder (FU186-L5), so the **slot stays NULL**
 * (the rows-27/29/2C precedent: a ported body with no live engine installer is
 * uncounted) and the body is exported for direct testing. The
 * `[0x157C2C]` direction machine + movement/event block and stages 1-4 are
 * FU186-L1. */
static int fifa96_match_action_1C(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_team *team;
  struct fifa96_match_entity *e;
  uint32_t t, idx;
  int32_t id = r->entity_id;
  uint8_t stage;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  t = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  idx = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  team = &mr->entities.team[t];
  e = &team->records[idx];

  r->timer89 += (int32_t)r->delta;                  /* 0x77735 dword[+0x89] += */
  stage = r->stage92;
  if (stage == 0u) {                                /* 0x77762 */
    r->ran = 1;                                     /* 0x7776A +0x9E = 1 */
    r->timer89 = 0;                                 /* 0x77777 */
    mr->keeper_latch[2] = 0;                        /* 0x77783 [0x157C56] = 0 */
    r->stage92 = 1u;                                /* 0x77788 */
    stage = 1u;                                     /* fall through */
  }
  if (stage == 1u) {                                /* 0x7778E */
    fifa96_arm_vec from, to;
    int32_t distance = 0, lane = 0;
    int32_t angle = 0;
    from.x = r->pos_x;
    from.y = r->pos_y;
    from.z = r->pos_z;
    to.x = r->target_x;
    to.y = r->target_y;
    to.z = r->target_z;
    (void)fifa96_arm_dist_stage(&from, &to, &distance, &lane);   /* 0x77797 */
    /* 0x777B4 `0x79C50(rec, -[0x1577C0], -[0x1577C2])` and the 0x777CC `+0x7F`
     * heading are FU186-L2 (unmodeled inputs / no cell). */
    if (fifa96_entity_angle(distance, r->target_y, &angle) == FIFA96_OK) { /* 0x777DE */
      fifa96_keeper_vec delta_v;
      fifa96_keeper_point pos;
      uint8_t on_target = 0, event_code = 0;
      int16_t steer_x = 0, steer_z = 0;
      delta_v.distance = (int16_t)distance;
      delta_v.dx = (int16_t)((uint16_t)to.x - (uint16_t)from.x);   /* 0x8DCDD */
      delta_v.dz = (int16_t)((uint16_t)to.z - (uint16_t)from.z);
      pos.x = r->pos_x;
      pos.y = r->pos_y;
      pos.z = r->pos_z;
      (void)fifa96_keeper_lunge_track(&delta_v, (int16_t)angle, &pos, &on_target,
                                      &steer_x, &steer_z, &event_code);  /* 0x777F6 */
      if (on_target != 0) {                         /* the |angle| < 0x55 arm */
        r->pos_x = pos.x;
        r->pos_z = pos.z;
      }
      /* else the 0x77856 rotated arm -> FU186-L2. (staged, unused) */
      (void)event_code;
      (void)steer_x;
      (void)steer_z;
    }
    /* 0x77997 the `0x6E598` anim and the `0x76B28` steering -> FU186-L2. */
    r->timer89 = 0;                                 /* 0x779B9 */
    r->stage92 = 2u;                                /* 0x779CB */
    stage = 2u;
  }
  if (stage == 2u && r->row44 != 0u)                /* 0x779D1 `[+0x44] != 0` */
    match_row_reset(mr, e);                         /* 0x7DAB4 */
  return FIFA96_OK;                                 /* the 0x779DE tail -> FU186-L2 */
}

int fifa96_match_action_1A(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_team *team;
  struct fifa96_match_entity *e;
  uint32_t t, idx;
  int32_t id = r->entity_id;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  t = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  idx = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  team = &mr->entities.team[t];
  e = &team->records[idx];

  if (mr->state.phase == 0x0Du &&                     /* 0x76634 */
      team->side == mr->phase_machine.side_controlled) /* [0x157AAC]>>24 */
    r->controlled = 1;                                /* 0x76641 [0x157A83] = rec */
  r->timer89 += (int32_t)r->delta;                    /* 0x76662 */
  if (r->stage92 > 4u) return FIFA96_OK;              /* 0x7667E JA 0x76B21 */
  if (r->stage92 == 0u) {                             /* 0x76693 stage 0 */
    fifa96_keeper_rep_a_out g;
    (void)fifa96_keeper_reposition_a_gate(
        0u /* FUN_000765C4 tracked-teammate result, FU186-L4 */, &g);
    if (g.install_code != 0) {                        /* 0x7669D install 4 now */
      if (g.reset != 0) match_row_reset(mr, e);       /* 0x7669D FUN_0007DAB4 */
      if (fifa96_match_entities_install(e, (uint8_t)mr->state.phase, g.install_code, 0) == 1)
        (void)fifa96_match_dispatch_action(mr, g.install_code);     /* 0x7D9A4 */
      return FIFA96_OK;
    }
    if (g.flag_9e != 0) r->ran = 1;                   /* 0x766BE +0x9E = 1 */
    r->timer89 = 0;                                   /* 0x766BE */
    r->stage92 = (uint8_t)(r->stage92 + g.stage_advance);  /* 0x766BE +0x92 += 2 */
    r->target_x = r->pos_x;                           /* 0x766F7 0x79B1C snap */
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
    /* 0x766FC.. the `[0x157C2C]` direction machine + movement/event block and
     * stages 1-4 -> FU186-L1. */
  }
  return FIFA96_OK;
}

/* ===== M2 phase-15 T2 / FU-179: row 0x19 keeper hold ======================
 *
 * `fifa96_match_action_19` binds the ported `fifa96_keeper_hold_*` helpers to
 * the native body `0x746E4..0x74CB7` (FU-179 §3, first-hand this slice). The
 * row installs no code (its install producer, row-04 `0x7E866`/`0x7E8C1`, is
 * already live). Applied in body order: prologue (`+0x9E=1`,
 * `[0x157C5D]=1`, the deepest-keeper tracker via `hold_track`); the `+0x98`
 * pos-copy early-out; the `[rec+0x20]` slot arm (human-phase pos copy; the
 * `0x79C20` placement is FU179-L4); the phase gate (phase != 2 -> the no-slot
 * tail with the `[rec+0x1C]` phase-method request, FU179-L5, and the
 * `[0x157A49]>>24==1` `+0x7b` write); the phase-2 guard
 * (`hold_guard`/`guard_clamp`, all five `0x7479D..0x74831` rejections incl. the
 * tracked-opponent lane `0x747EA`); the predictor
 * (`fifa96_keeper_hold_predict`); the carrier dive (`hold_intercept`); the
 * weighted fallback (`hold_fallback`). The `FUN_00079B58` receiver-timer arms
 * (`0x74823` guard-pass, `0x74B63` carrier-dive) are the FU179-L7 leg. The six
 * `0x1577xx` camera leads have no ported producer
 * (FU179-L1), so the predictor conservatively bails and the row runs the
 * carrier/fallback exactly as the native does under the staged-zero camera. */
static int fifa96_match_action_19(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_team *team;
  struct fifa96_match_entity *e;
  fifa96_keeper_point cam, out;
  uint8_t side;
  int32_t lane;
  uint8_t bound;
  int32_t id = r->entity_id;
  uint32_t t, idx;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  t = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  idx = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  team = &mr->entities.team[t];
  e = &team->records[idx];
  side = team->side;

  r->ran = 1;                       /* 0x746F9 */
  mr->global_157c5d = 1;            /* 0x74707 */
  /* Deepest-keeper tracker (0x7470D..0x7472B): keep the smallest signed
   * `+0x6B` lane in team->tracker7c7. */
  if (team->tracker7c7 == FIFA96_MATCH_ENTITY_NONE ||
      fifa96_keeper_hold_track((int16_t)team->records[team->tracker7c7].lane_x,
                               (int16_t)e->lane_x) != 0)
    team->tracker7c7 = (int32_t)e->index;
  if (e->skip_98 != 0) {            /* 0x74731 the +0x98 latch early-out */
    r->target_x = r->pos_x;
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
    return FIFA96_OK;
  }
  if (r->has_slot != 0) {           /* 0x74748 slot arm */
    if (mr->global_157a4c == 1u) {  /* 0x7474F human-side phase -> pos copy */
      r->target_x = r->pos_x;
      r->target_y = r->pos_y;
      r->target_z = r->pos_z;
      return FIFA96_OK;
    }
    /* else the 0x79C20 slot-direction placement (FU179-L4, unported). */
  }
  if (mr->state.phase != 2u) {      /* 0x7478C -> the no-slot tail */
    /* 0x74C98 CALL dword[rec+0x1C] (the R23 phase seam, FU179-L5) is left an
     * unwired request; 0x74CA8 word[+0x7b]=1 on the human-side phase. */
    if (mr->global_157a4c == 1u) r->timer7b = 1;
    return FIFA96_OK;
  }
  cam.x = mr->keeper_cam_x;
  cam.y = mr->keeper_cam_y;
  cam.z = mr->keeper_cam_z;
  lane = (int16_t)(r->lane >> 16);  /* [+0x69]>>16 */
  bound = (uint8_t)(mr->entities.controlled != FIFA96_MATCH_ENTITY_NONE ? 1u : 0u);
  /* Phase-2 guard (0x7479D..0x74831): the five native rejections all jump to
   * the predictor (0x74832) — `bound` = `[0x157A83]` (0x747B3), lane
   * `[+0x69]>>16` (0x747C5), `[0x157750]` (0x747D2), the deepest-keeper
   * tracker `[team+0x7C7] == rec` (0x747DC), and (0x747EA) the tracked
   * opponent lane `word[deepest+0x6B] >= word[opp+0x6B]` where the opponent
   * record is `[[team+0x7A6]+0x7B2]` (the opponent team target). */
  {
    uint8_t guard_ok = (uint8_t)(bound == 0 && lane < 0x2D0 &&
                                 mr->keeper_cam_y == 0 &&
                                 team->tracker7c7 == (int32_t)idx);
    if (guard_ok != 0) {
      uint32_t opp = (uint32_t)(t ^ 1u);
      int32_t opp_id = mr->entities.team[opp].target;   /* [[team+0x7A6]+0x7B2] */
      if (opp_id >= 0 &&
          opp_id <
              (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS)) {
        int16_t opp_lane =
            (int16_t)(mr->entities.team[opp]
                          .records[(uint32_t)opp_id % FIFA96_MATCH_ENTITY_RECORDS]
                          .lane >> 16);
        if (lane >= opp_lane) guard_ok = 0;             /* 0x747EA JGE reject */
      }
    }
    if (guard_ok != 0 &&
        fifa96_keeper_hold_guard(&cam, side, &out) == FIFA96_OK) {
      (void)fifa96_keeper_guard_clamp(&out, side);
      r->target_x = out.x;
      r->target_y = out.y;
      r->target_z = out.z;
      /* 0x74823 FUN_00079B58(rec): the self receiver timer (+0x93 = 0x10 when
       * byte[+0x99] == 0). FU179-L7 leg — the +0x99 gate producer is unported
       * (the row-04/row-08 convention stages it 0). */
      return FIFA96_OK;
    }
  }
  /* Predictor (0x74832..0x74ACC). */
  {
    fifa96_keeper_predict pred;
    if (fifa96_keeper_hold_predict(&cam, side, 0, 0, 0, mr->keeper_cam_y,
                                   mr->sp_157821, bound, &pred) == FIFA96_OK &&
        pred.predicted != 0) {
      mr->predict_c44 = pred.scalar;
      mr->predict_c48_x = pred.target.x;
      mr->predict_c48_y = pred.target.y;
      mr->predict_c48_z = pred.target.z;
      r->target_x = pred.target.x;
      r->target_y = pred.target.y;
      r->target_z = pred.target.z;
      return FIFA96_OK;
    }
  }
  /* Carrier dive (0x74ACC..0x74B71). */
  if (r->has_slot != 0 && mr->entities.controlled == id) {
    if (fifa96_keeper_hold_intercept(&cam, r->lane, 0, 0, &out) == FIFA96_OK) {
      r->target_x = out.x;
      r->target_y = out.y;
      r->target_z = out.z;
      /* 0x74B63 FUN_00079B58(rec): the self receiver timer, FU179-L7 leg. */
      return FIFA96_OK;
    }
  }
  /* Weighted camera fallback (0x74B72..0x74C8D). */
  if (fifa96_keeper_hold_fallback(&cam, side, 0, r->lane, 0, 0, 0, 0, &out) ==
      FIFA96_OK) {
    r->target_x = out.x;
    r->target_y = out.y;
    r->target_z = out.z;
  }
  return FIFA96_OK;
}

/* ===== M2 phase-15 T3 / FU-178: row 09 sequence/steer (stage-0 slice) ======
 *
 * `fifa96_match_action_09` is the bounded stage-0 landing of the row-09 body
 * `0x80A00..0x81066` (FU-178 §3, first-hand this slice): the 4-arm table
 * `0x809F0` = `{0x80A3F, 0x80BFB, 0x80FCC, 0x8103A}`. Only arm 0 (stage 0,
 * `0x80A3F..0x80BFA`) is derived; arms 1/2/3 are **FU178-L1**.
 *
 * Contract (fresh `disassemble_bytes` this slice):
 *  - phase `[0x157A4A]>>24 != 2` -> the `FUN_0007DAB4` reset + tail
 *    (`0x80A0B..0x80A1F`);
 *  - stage `+0x92 > 3` -> tail (`0x80A2A`); stages 1/2/3 are the unported arms;
 *  - arm 0 clear (`+0x8D == 0`, `0x80A3F..0x80A86`): reset, then detach REC
 *    from `[team+0x7B2]`/`[team+0x7B6]`;
 *  - arm 0 set (`+0x8D != 0`, `0x80A87..0x80BFA`): the ball-relative face
 *    target `lead*0xA + (+0x6D/+0x6F)`, the `0x79C50` face, the
 *    `min(dword[+0x69]>>17, 0x30)` speed, the two `0x114E04` sine folds
 *    (`fifa96_entity_sine`) through `0x795A4` (the `match_row09_mul` port), the
 *    `+0x59/+0x61` position accumulate, `+0x7B = +0x79*2`, the `+0x73/+0x75`
 *    velocity pair + the `0x8DC68` speed (`+0x71`), the `+0x4D/+0x51/+0x55`
 *    target copy + `vel<<3` adds, the `0x11` anim install (`0x6E598`),
 *    `+0x9E=1`, `[0x15877C]=0`, `+0x89=0`, `+0x92++`.
 * Unmodelled cells are staged zero with their producers unported (the row-08
 * convention, OL-82): the `0x1577C0/C2` lead words (so the face target uses
 * only `+0x6D/+0x6F`), `word[+0x79]` (the `+0x7B` source) and the `[0x15877C]`
 * shared flag (FU178-L6, write-only here). The `0x79B58` receiver-timer call
 * (`0x80B4C`) applies the tested `0x79B58` effect (`+0x93 = 0x10`; the `+0x99`
 * callee gate is staged 0, as row 04). */
static int16_t match_row09_mul(int32_t a, int32_t b) {
  /* `FUN_000795A4` (`0x795A4..0x795B2`, fresh this slice): `(int64)a * b +
   * 0x8000` >> 16, low word. `fifa96_ball_fold` is the sine-pre-folded sibling
   * (it folds the `0x114E04` table first); row 09 folds the two sines itself
   * with `fifa96_entity_sine` and feeds them to this raw multiply. */
  int64_t v = ((int64_t)a * (int64_t)b + 0x8000) >> 16;
  return (int16_t)(uint16_t)(uint32_t)(int32_t)v;
}

static int fifa96_match_action_09(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_team *team;
  struct fifa96_match_entity *e;
  uint32_t t, idx;
  int32_t id = r->entity_id;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  t = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  idx = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  team = &mr->entities.team[t];
  e = &team->records[idx];

  if ((uint8_t)mr->state.phase != 2u) {              /* 0x80A0B..0x80A1F */
    match_row_reset(mr, e);
    return FIFA96_OK;
  }
  if (r->stage92 > 3u) return FIFA96_OK;             /* 0x80A2A JA tail */
  if (r->stage92 != 0u) {                            /* arms 1/2/3 = FU178-L1 */
    mr->match09_leg_stage = r->stage92;
    return FIFA96_OK;
  }
  if (r->active == 0u) {                             /* arm 0 clear, 0x80A3F */
    match_row_reset(mr, e);                          /* 0x80A48..0x80A4A */
    if (team->target == id)                          /* 0x80A52 [team+0x7B2] */
      team->target = FIFA96_MATCH_ENTITY_NONE;
    if (team->second == id)                          /* 0x80A67 [team+0x7B6] */
      team->second = FIFA96_MATCH_ENTITY_NONE;
    return FIFA96_OK;
  }
  {
    /* arm 0 set, 0x80A87.. */
    int32_t dx = (int32_t)e->lane_z;    /* lead_x*0xA + word[+0x6D]; lead 0 (OL-82) */
    int32_t dz = (int32_t)e->cam_dz6f;  /* lead_z*0xA + word[+0x6F]; lead 0 (OL-82) */
    int32_t speed = r->lane >> 17;      /* 0x80AB0 dword[+0x69]>>17 */
    int32_t face = r->face7d;
    uint8_t sector = r->type;
    int32_t sin1, t1;
    if (dx != 0 || dz != 0) {                        /* 0x80AA9 0x79C50 */
      (void)fifa96_entity_face(dx, dz, &face, &sector);
      r->face7d = (int16_t)face;
      r->type = sector;
    }
    if (speed > 0x30) speed = 0x30;                  /* 0x80AB9..0x80ABE */
    speed = (int16_t)speed;                          /* 0x80AEB MOVSX ESI,SI */
    sin1 = fifa96_entity_sine(face);                 /* 0x80AC3..0x80AE4 */
    t1 = match_row09_mul(speed, sin1);               /* 0x80B1D */
    r->pos_x += (int16_t)t1;                         /* 0x80B26..0x80B2F */
    r->pos_z += match_row09_mul(speed, (int16_t)t1); /* 0x80B32..0x80B49 */
    /* the second fold (0x80AE8..0x80B15, sine(face+0x100)) is dead on the
     * position path: 0x80B22 overwrites ECX with the first result before the
     * second 0x795A4 call, so 0x80B32 multiplies by (int16)t1, not sin2. */
    r->timer7b = 0;                                  /* 0x80B43 word[+0x79]*2, +0x79 zero (OL-82) */
    e->timer93 = 0x10;                               /* 0x80B4C 0x79B58 (+0x99 staged 0) */
    r->vel73 = match_row09_mul((int16_t)r->timer7b, sin1);        /* 0x80B51..0x80B5E */
    r->vel75 = match_row09_mul((int16_t)r->timer7b, (int16_t)t1); /* 0x80B62..0x80B6F */
    r->speed71 = (int16_t)fifa96_entity_distance(r->vel73, r->vel75); /* 0x80B7F */
    r->target_x = r->pos_x;                          /* 0x80B8E MOVSD x3 */
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
    r->target_x += (int32_t)r->vel73 * 8;            /* 0x80B91..0x80B9D */
    r->target_z += (int32_t)r->vel75 * 8;            /* 0x80B9F..0x80BB0 */
    (void)fifa96_match_run_record_anim_install(mr, e, 0x11, 0);  /* 0x80BC5 */
    r->ran = 1;                                      /* 0x80BCC */
    r->timer89 = 0;                                  /* 0x80BD9 */
    /* 0x80BD3 writes the `[0x15877C]` shared flag to 0 (FU178-L6, unmodelled,
     * write-only here). 0x80BDF..0x80BEB advances the stage. */
    r->stage92 = 1u;
  }
  return FIFA96_OK;
}

/* ===== FU-184 (phase-16 T2): the row-0x14 celebration/scatter body =========
 *
 * The native row-0x14 body `0x8784C..0x87CBC` (FU-184 §4; installer: the live
 * phase-5 goal arm `match_run.c:2494`). A 3-stage machine over the goal record
 * (`mr->record`): the head accumulates `timer89 += delta`; stage 0 builds a
 * five-point scatter + the six-slot buffer and arms the first point; stage 1
 * walks the remaining points; stage 2 resets the record and — unless the
 * record's team side is 1 (`0x87C1A`, FU184-L6) — releases to phase 1 at
 * `0x87C71` (the `match_run_kickoff_entry` T2-2 factoring). */

/* The shared arm block (`0x87A88` stage 0/re-entry, `0x87B53` stage 1): `points
 * [idx+1]` -> the record target `+0x4D`, the `0x8DCD4` metric into `+0x65` and
 * the `0x79C50` face into `+0x8E`. The `[0x1587B8+idx]` id-compare anim post
 * (`0x87AD1..0x87AFB`, `fifa96_action_sequence_event`) needs the celebration
 * id tables `0x10F344/46/49/4C`, whose producer is unported, so the anim post
 * is a numbered leg (FU184-L1 sibling) and only the target/metric/face payload
 * lands. */
static void match_celebration_arm(struct fifa96_match_run *mr,
                                  struct fifa96_match_run_record *r) {
  int32_t idx = mr->celebration.idx;
  const fifa96_action_vec3 *point;
  fifa96_arm_vec from;
  fifa96_arm_vec to;
  int32_t dz = 0;
  uint8_t octant = r->type;
  if (idx < 0 || idx + 1 > 5) idx = 4;   /* native idx in 0..4 (points[1..5]) */
  point = &mr->celebration.points[idx + 1];
  r->target_x = point->x;                /* 0x87AB6 MOVSD x3 */
  r->target_y = point->y;
  r->target_z = point->z;
  from.x = r->pos_x;
  from.y = r->pos_y;
  from.z = r->pos_z;
  to.x = r->target_x;
  to.y = r->target_y;
  to.z = r->target_z;
  (void)fifa96_arm_dist_stage(&from, &to, &r->distance, &dz);   /* 0x8DCD4 */
  if (fifa96_arm_face(&from, &to, &octant) == FIFA96_OK) r->type = octant; /* 0x79C50 */
}

static int fifa96_match_action_14(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_entity *e = match_sp_entity(mr, NULL, NULL);
  uint8_t stage;
  if (e == NULL) return -FIFA96_ERR_INVALID;
  /* head `0x87854..0x8787F`: timer89 += (zero-extended) delta; the
   * `[0x157AA3] = rec` write and the `0x974F0`/`0x36200` sinks are legs
   * (FU184-L4). */
  r->timer89 += (uint16_t)r->delta;                 /* 0x8785C */
  stage = r->stage92;
  if (stage == 0u) {                                /* 0x878A1 */
    fifa96_action_vec3 base;
    int32_t dir_x, dir_z;
    uint32_t rng8[8];
    uint32_t i;
    if (r->timer89 < 0x3C) return FIFA96_OK;        /* 0x878A9 timer >= 60 */
    /* The ball-record anim (`0x878B6..0x878F0`: `[[team+0x7A6]+0x28]` first
     * byte == 0x26 -> anim 0x46/0x47 by rng&1) needs the team's ball-record
     * pointer and its animation row; the engine ball is not a pool record and
     * the mapping is unproven (FU184-L7), so this install is a leg and draws
     * no RNG (the engine ball row is 3, never 0x26). */
    base.x = r->pos_x;                              /* 0x878F5 MOVSD x3 */
    base.y = r->pos_y;
    base.z = r->pos_z;
    dir_x = r->pos_x < 0 ? -1 : 1;                  /* 0x87900..0x87911 */
    dir_z = r->pos_z > 0 ? -1 : 1;                  /* 0x87912..0x87923 */
    if (r->side == 1u) dir_x = 0;                   /* 0x87924..0x87931 */
    for (i = 0; i < 8u; i++) {                      /* eight 0x92AC8 draws */
      uint16_t draw = 0;
      (void)fifa96_rng_step(&mr->rng, &draw);
      rng8[i] = draw;
    }
    (void)fifa96_action_sequence_scatter_celebration(&base, dir_x, dir_z, rng8,
                                                     mr->celebration.points);
    /* The native copies the pre-clamp points[4] to the 6th slot `[0x158714]`
     * BEFORE the clamp loop (`0x87A2A`); the ported helper clamps in place, so
     * the 6th slot carries the clamped points[4] — the recorded FU184-L1
     * divergence. */
    mr->celebration.points[5] = mr->celebration.points[4];
    mr->celebration.idx = 0;                        /* the fresh-scatter index */
    match_celebration_arm(mr, r);                   /* the idx-0 arm block */
    r->timer89 = 0;                                 /* 0x87B06 */
    r->stage92 = 1u;                                /* 0x87B12 stage++ */
    return FIFA96_OK;
  }
  if (stage == 1u) {                                /* 0x87B1F */
    int32_t idx = mr->celebration.idx;
    /* `0x87B1F..0x87B32`: `FUN_00045001()` = `[0x1067EC] & 0xF0`. The input
     * producer is unported (FU184-L3), so the derived gate is always false and
     * this early advance is dormant; the timing advance below carries. */
    match_celebration_arm(mr, r);                   /* 0x87B53 */
    if (r->distance < 0x20 ||                       /* 0x87B92 word[+0x65] */
        r->timer89 > 0x4B0 ||                       /* 0x87BA3 */
        (idx == 3 && r->timer89 > 0x12C) ||         /* 0x87BAB..0x87BB4 */
        (idx == 4 && r->timer89 > 0xB4)) {          /* 0x87BBC..0x87BD3 */
      idx++;
      mr->celebration.idx = idx;                    /* 0x87BDF */
      if (idx >= 5) {                               /* 0x87BE4 */
        r->timer89 = 0;                             /* 0x87BF8 */
        r->stage92 = 2u;                            /* 0x87C06 stage++ */
        return FIFA96_OK;
      }
      /* `0x87BE9..0x87BEF`: stage-- then re-enter the full arm block, which
       * re-increments the stage (the FU-82 §3.5 curious re-entry). */
      match_celebration_arm(mr, r);                 /* 0x87A88 re-entry */
      r->timer89 = 0;                               /* 0x87B06 */
      r->stage92 = 1u;
      return FIFA96_OK;
    }
    return FIFA96_OK;                               /* 0x87CB6 tail (wait) */
  }
  if (stage == 2u) {                                /* 0x87C13 */
    match_row_reset(mr, e);                         /* 0x7DAB4 */
    if (r->side != 1u) {                            /* 0x87C1A..0x87C25 (FU184-L6) */
      /* `0x87C2B..0x87C71`: the `[0x10F328/2C/30]` camera reset, the
       * `[0x157AAF] = side ^ 1` display swap (leg) and the phase-1 entry. */
      (void)fifa96_camera_init(&mr->render.camera, 0, 0, 0);   /* 0x700F4 */
      (void)fifa96_match_run_kickoff_entry(mr);                /* 0x87C71 */
      (void)fifa96_match_entities_kickoff_place(&mr->entities,
                                                mr->render.camera.pos_x,
                                                mr->render.camera.pos_y,
                                                mr->render.camera.pos_z); /* 0x73E08 */
      mr->celebration.idx = 0;                      /* 0x87C43 */
    }
    return FIFA96_OK;
  }
  return FIFA96_OK;                                 /* stage >= 3: tail */
}

/* ===== FU-185 (phase-16 T2): the row-0x22 sequence/press body ==============
 *
 * The native row-0x22 body `0x8539C..0x85481` (FU-185 §2; installer: the wired
 * row-07 `0x816DF` invoke-now on the opponent team target, `match_kick_invoke_
 * opponent`). A phase-2-only 3-stage machine over `mr->record`: the phase gate
 * (`dword[0x157A4A]>>24 == 2`), `timer89 += delta`, the `0x79B1C` snap
 * (target := pos + the word clears, FU185-L2), then stage 0 (active gate ->
 * `ran`/snap/timer=0/advance), stage 1 (the FU-136 `sequence_press_event`
 * band/timer decision -> anim 0x4F, or reset) and stage 2 (`+0x44` -> reset).
 * Every terminating arm is the shared `match_row_reset`. */
int fifa96_match_action_22(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_entity *e = match_sp_entity(mr, NULL, NULL);
  uint8_t stage;
  if (e == NULL) return -FIFA96_ERR_INVALID;
  if (mr->state.phase != 2u) {                      /* 0x853AB phase gate */
    match_row_reset(mr, e);                         /* 0x85478 */
    return FIFA96_OK;
  }
  r->timer89 += (uint16_t)r->delta;                 /* 0x853B6 timer89 += delta */
  /* `0x853CB CALL 0x79B1C` snap: target := pos (the FU185-L2 word-clear set is
   * the recorded residual). */
  r->target_x = r->pos_x;
  r->target_y = r->pos_y;
  r->target_z = r->pos_z;
  stage = r->stage92;
  if (stage == 0u) {                                /* 0x853E9 */
    if (r->active == 0u) {                          /* 0x853F1 */
      match_row_reset(mr, e);                       /* 0x85476 */
      return FIFA96_OK;
    }
    r->ran = 1;                                     /* 0x85400 +0x9E = 1 */
    r->target_x = r->pos_x;                         /* 0x85407 snap */
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
    /* 0x8540C byte[+0x43] = 0 (the FUN_0006E598 row-flags byte, FU185-L3). */
    r->timer89 = 0;                                 /* 0x85416 */
    r->stage92 = 1u;                                /* 0x85420 */
    return FIFA96_OK;
  }
  if (stage == 1u) {                                /* 0x85428 */
    int16_t band = (int16_t)(r->lane >> 16);        /* 0x85428 word[+0x6B] */
    fifa96_action_sequence_press p;
    (void)fifa96_action_sequence_press_event(band, r->timer89, &p);  /* FU-136 */
    if (p.fire != 0u) {                             /* band <= 0x30 */
      if (fifa96_match_run_record_anim_install(mr, e, 0x4F, 0) == FIFA96_OK) {
        r->anim_id = e->anim_id;                    /* 0x85453 0x6E598 */
        r->frame = e->frame;
        r->row44 = e->row44;
      }
      r->timer89 = 0;                               /* 0x8545E */
      r->stage92 = 2u;                              /* 0x85468 */
    } else if (p.reset != 0u) {                     /* band > 0x30 && timer > 0x1E */
      match_row_reset(mr, e);                       /* 0x85476 */
    }
    return FIFA96_OK;                               /* else: wait at 0x8543C */
  }
  if (stage == 2u) {                                /* 0x85470 */
    if (r->row44 != 0u) match_row_reset(mr, e);
    return FIFA96_OK;
  }
  return FIFA96_OK;                                 /* stage > 2: tail RET */
}

/* ===== FU-187 §1 (phase-16 T4): row 0x16 fouled-player face-the-ball ========
 *
 * The native row-0x16 body `0x84630..0x8471C` (FU-187 §1, first-hand): a
 * 3-stage machine over the record stage byte `+0x92` (`mr->record.stage92`),
 * entry EAX = record. Its only producer is the phase-0x19 stage-0 install
 * (`0x8A096 MOV EDX,0x16; 0x8A09B MOV EAX,[0x15888F]; 0x8A0A0 CALL 0x7D9A4`
 * on the fouler `[0x15888F]`, FU-187 §2). The engine's phase-0x19 row
 * (`fifa96_match_action_phase_19`) applies that install through
 * `match_run_ref_apply` and the per-record dispatch runs this body.
 *  - stage 0 (`0x84654..0x8468C`): copy the position triple `+0x59` -> the
 *    target triple `+0x4D` (12 B; the engine's pos -> target, the row-01 snap
 *    shape), then wait while the object cell `[0x158866]`'s code byte != `0x48`
 *    (`0x84671 JNZ` return). The engine stages that cell as
 *    `mr->referee.referee_object_code` (the loader stand-in; FU-187 §6 FU187-L1
 *    is the polarity leg vs phase-0x19 stage 6). On pass: `+0x89 = 0`,
 *    `+0x92 -> 1` (falls into stage 1).
 *  - stage 1 (`0x84692..0x846F7`): re-copy the triple; `FUN_000795B4(rec+0x59,
 *    0x15880C)` = `fifa96_entity_intercept_band` toward the ball record; the
 *    `FUN_00092AC8` RNG parity picks the anim (`0x6A` odd / `0x55` even);
 *    `FUN_0006E598(rec, anim, byte[+0x8E], 0)` = the anim install;
 *    `FUN_00079C50(rec, EDX, EBX)` = the face over the band out-args
 *    (`dword[ESP]>>16` = the word at `ESP+2` = `dx`, `dword[ESP+2]>>16` = the
 *    word at `ESP+4` = `dz`; FU-187 §6 FU187-L2 — the exact alias read); then
 *    `+0x89 = 0` and `+0x92 -> 2`.
 *  - stage 2 (`0x846FD..0x8470E`): re-copy the triple; when `byte[rec+0x44]`
 *    (`mr->record.row44`) is non-zero run `FUN_0007DAB4` (`match_row_reset`),
 *    else return (the terminal wait).
 * The anim/face writes are FU187-L4 presentation sinks (the `0x795B4` ->
 * `fifa96_entity_intercept_band` mapping is the engine's only 0x795B4 port);
 * the native body never writes `+0x9E` (ran). */
static int fifa96_match_action_16(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  struct fifa96_match_entity *e;
  uint32_t t, idx;
  int32_t id = r->entity_id;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  t = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  idx = (uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS;
  e = &mr->entities.team[t].records[idx];
  if (r->stage92 == 0u) {                    /* 0x84654 */
    r->target_x = r->pos_x;                  /* 0x84662 MOVSD x3 */
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
    if (mr->referee.referee_object_code != 0x48u)
      return FIFA96_OK;                      /* 0x84671 JNZ: wait */
    r->timer89 = 0;                          /* 0x84680 */
    r->stage92 = 1u;                         /* 0x8468c INC AH (fall into stage 1) */
  }
  if (r->stage92 == 1u) {                    /* 0x84692 */
    fifa96_entity_intercept_band_out band;
    uint16_t draw = 0;
    int32_t face = r->face7d;
    uint8_t sector = r->type;
    int32_t anim;
    r->target_x = r->pos_x;                  /* 0x846a2 MOVSD x3 */
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
    (void)fifa96_entity_intercept_band(r->pos_x, r->pos_z, mr->entities.ball.x,
                                       mr->entities.ball.z, &band); /* 0x846a5 */
    if (fifa96_rng_step(&mr->rng, &draw) != FIFA96_OK) return FIFA96_OK;
    anim = (draw & 1u) != 0u ? 0x6A : 0x55;  /* 0x846af..0x846ba */
    if (fifa96_match_run_record_anim_install(mr, e, anim, 0) == FIFA96_OK) {
      r->anim_id = e->anim_id;               /* 0x846c9 CALL 0x6E598 */
      r->frame = e->frame;
      r->row44 = e->row44;
    }
    if (fifa96_entity_face(band.dx, band.dz, &face, &sector) == FIFA96_OK) {
      r->face7d = (int16_t)face;             /* 0x846e0 CALL 0x79C50 */
      r->type = sector;
    }
    r->timer89 = 0;                          /* 0x846e5 */
    r->stage92 = 2u;                         /* 0x846f7 INC DL (fall into stage 2) */
  }
  if (r->stage92 == 2u) {                    /* 0x846FD */
    r->target_x = r->pos_x;                  /* 0x846fd MOVSD x3 */
    r->target_y = r->pos_y;
    r->target_z = r->pos_z;
    if (r->row44 != 0u)                      /* 0x84706 CMP byte[+0x44],0 */
      match_row_reset(mr, e);                /* 0x8470e CALL 0x7DAB4 */
  }
  return FIFA96_OK;
}

/* ===== FU-188 §4 (phase-16 T4): row 0x24 lane/anim ==========================
 *
 * The native row-0x24 body `0x86510..0x866F1` (FU-188 §4, first-hand). Its sole
 * producer is the phase-0x20 / act-0xA timeline's code-0x24 install on all 22
 * records (`0x8B750`/`0x8B78B` invoke-now, FU-188 §3), whose only producer is
 * situation 0 — **not dispatched by any engine source/test** (FU-188 §6), so
 * the phase-0x20 and row-0x24 table slots stay NULL (uncounted; the
 * rows-27/29/2C precedent) and this body is exported for direct testing
 * (declared in fifa96_match_handlers.h). The record field mapping follows the
 * entity header (`pos_x` = +0x59, `pos_z` = +0x61, `target_x` = +0x4D,
 * `target_z` = +0x55, `timer89` = +0x89, `timer7b` = +0x7B); FU-188 §4's
 * pos_x/pos_z labels are mirrored by that header and the header governs.
 *  - head (`0x86519..0x8654E`): `+0x89 += delta`; when active (`byte[+0x8D]`)
 *    refresh the `+0x7B` lane word from `table_0x157A38[byte[[rec+4]+0xD]] - 2`.
 *    The `0x157A38` table and the `rec[+4]` roster descriptor are unmodeled
 *    (FU-188 §6 FU188-L5 / OL-65), so the lane write is a numbered leg and the
 *    staged `player_d` (0) leaves `+0x7B` untouched.
 *  - stage 0 (`0x8656F..0x8664E`): seed `pos_x = 0x9F0` (`0x8657A`), the
 *    `±6*byte[[rec+4]+0xE]` facing into `pos_z` with the team-side sign
 *    (`0x86590..0x865AE`; `player_e` is staged 0 -> the term is 0), zero
 *    `+0x71/+0x73/+0x75`, run the `0x79C50` face, wait on the face-derived
 *    threshold, commit `target_z = pos_z` and `target_x = 0x7E0 + 6*byte`
 *    (`0x86611..0x8664B`), reset `+0x89` and stage to 1.
 *  - stage 1 (`0x8665C..0x866CF`): the `0x8EEC0`/`0x67800`/`0x8EEB4` ball
 *    gates (FU-188 §6 FU188-L3 — cited, gated on `[0x157AC2]`/`[0x14C32A]`),
 *    re-commit the target, wait while `word[+0x65]` (`distance`) >= `0x20`,
 *    then reset `+0x89` and stage to 2.
 *  - stage 2 (`0x866DD`): the `0x6E1D0` phase-cell release (FU-188 §6
 *    FU188-L3 — cited, no derived cell), then return.
 * The native never sets `+0x9E` (ran). Returns FIFA96_OK, or
 * -FIFA96_ERR_INVALID for an out-of-range `mr->record.entity_id`. */
int fifa96_match_action_24(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r = &mr->record;
  uint8_t stage;
  if (r->entity_id < 0 ||
      r->entity_id >=
          (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  r->timer89 += (int32_t)r->delta;                 /* 0x86519..0x8652D */
  if (r->active != 0u) {                           /* 0x86533 TEST AH,AH */
    /* 0x86537..0x8654E: lane := table_0x157A38[player_d] - 2. The table and
     * the roster descriptor byte are unported (FU188-L5); `player_d` is staged
     * 0, so the write is a leg and `+0x7B` is left as staged. */
  }
  stage = r->stage92;
  if (stage == 0u) {                               /* 0x8656F */
    int32_t facing = (int32_t)r->player_e * 6;     /* 0x86587: 6*[[rec+4]+0xE] */
    int32_t face = r->face7d;
    uint8_t sector = r->type;
    if (r->side != 0u) facing = -facing;           /* 0x865A0/0x865A2 CWDE;NEG */
    r->pos_x = 0x9F0;                              /* 0x8657A */
    r->speed71 = 0;                                /* 0x865A8 word[+0x71]=0 */
    r->pos_z = facing;                             /* 0x865AE dword[+0x61] */
    r->vel73 = 0;                                  /* 0x865BA word[+0x75]=0 */
    r->vel75 = 0;                                  /* 0x865C0 word[+0x73]=0 */
    if (fifa96_entity_face(r->pos_x, r->pos_z, &face, &sector) == FIFA96_OK) {
      r->face7d = (int16_t)face;                   /* 0x865C6 CALL 0x79C50 */
      r->type = sector;
    }
    /* 0x865CB..0x8660D: the wait is `timer89` against a term the `0x79C50`
     * result and the unmodeled `+0x8A`/`+0x89` words (dword[+0x8A]>>0x19)
     * produce; the derived model is a no-op wait (FU-188 §6 FU188-L3). */
    r->target_z = r->pos_z;                        /* 0x86611 [+0x55]=[+0x61] */
    r->target_x = 0x7E0 + 6 * (int32_t)r->player_e; /* 0x86617/0x8664B */
    r->timer89 = 0;                                /* 0x86639 */
    r->stage92 = 1u;                               /* 0x8664E INC AL */
    stage = 1u;                                    /* fall through */
  }
  if (stage == 1u) {                               /* 0x8665C */
    /* 0x8665C..0x86684: the `0x8EEC0`/`0x67800`/`0x8EEB4` ball gates (skipped
     * when `[0x157AC2]`/`[0x14C32A]` are set) are FU188-L3 legs; the engine
     * takes the no-op path (both cells staged 0 -> the gates would fire). */
    r->target_z = r->pos_z;                        /* 0x86687/0x8668D */
    r->target_x = 0x7E0 + 6 * (int32_t)r->player_e;
    if (r->distance >= 0x20) return FIFA96_OK;     /* 0x866B4..0x866BD JGE */
    r->timer89 = 0;                                /* 0x866C5 */
    r->stage92 = 2u;                               /* 0x866CF INC BL */
  }
  if (r->stage92 == 2u) {                          /* 0x866DD */
    /* 0x866E7 CALL 0x6E1D0(rec, rec+0x4D, -1): the phase-cell release; the
     * derived cell producer is unported (FU188-L3), so the release is a leg. */
  }
  return FIFA96_OK;
}

/* ===== FU-191/FU-198 (M2 phase-17 T1): the per-record phase seam + row 03 ==== */

/* FU-177 §3: the native per-record phase-handler call
 * `[rec+0x1C](EAX=rec, EDX=&rec[0x4D], EBX=-1)`. The engine resolves the
 * current phase's handler and threads the pool record through the walk cursor
 * (`fifa96_match_dispatch_phase_record`), so the row-03 staging record is synced
 * to/from the pool record around the call (the native row 03 reads and writes
 * the record directly and continues with the rewritten target). */
static void match_row_phase_call(struct fifa96_match_run *mr,
                                 struct fifa96_match_entity *e,
                                 struct fifa96_match_run_record *r) {
  e->target_x = r->target_x;
  e->target_y = r->target_y;
  e->target_z = r->target_z;
  (void)fifa96_match_dispatch_phase_record(mr, mr->state.phase, e, -1);
  r->target_x = e->target_x;
  r->target_y = e->target_y;
  r->target_z = e->target_z;
}

/* FU-191 §6 / FU-198 §5.1: the ported native `FUN_0006DCC8`
 * (`0x6DCC8..0x6DD3B`) — the per-record phase rows `0x02`/`0x15`. Fresh
 * disassembly: `EAX = [rec+0xC]`; `if (BX < 0) EBX = word[0x1577DE + side*4] >>
 * 0x10; else EBX = MOVSX BX`; `EAX += EBX*2`; `target[0] = MOVSX byte[EAX] *
 * 0x26`; `target[2] = MOVSX byte[EAX+1] * 0x21`; both negated when
 * `byte[[rec]+0x826] != 0`; `target[1] = 0`. */
/* FU-192 §2.2 (M2 phase-17 T3): the shared `0x6DCC8` core, factored so the
 * `0x6DD6C` wrapper (rows 08/09) can reuse it with an explicit direction index.
 * `edge < 0` selects the per-side default (`word[0x1577DE + side*4] >> 16`);
 * otherwise the explicit index. The `[rec+0xC]` metadata is the OL-13 gate
 * (FU192-L1). */
static void match_phase_dcc8_apply(struct fifa96_match_run *mr,
                                   struct fifa96_match_entity *rec, int32_t edge) {
  const int8_t *meta;
  uint32_t tt;
  int32_t side;
  int32_t idx;
  int32_t tx;
  int32_t tz;
  tt = rec->team;                                       /* native [rec] = team */
  if (tt >= FIFA96_MATCH_ENTITY_TEAMS) return;
  side = (int32_t)mr->entities.team[tt].side;           /* [team+0x826] */
  if (edge < 0)
    idx = mr->phase_dir_default[(uint32_t)side & 1u];   /* word[0x1577DE+side*4]>>16 */
  else
    idx = edge;                                          /* MOVSX BX */
  meta = rec->phase_meta;                               /* native [rec+0xC] */
  if (meta == NULL) return;                             /* OL-13 unported -> gated */
  tx = (int32_t)(int16_t)((int16_t)(int8_t)meta[idx * 2] * (int16_t)0x26);
  tz = (int32_t)(int16_t)((int16_t)(int8_t)meta[idx * 2 + 1] * (int16_t)0x21);
  if (side != 0) { tx = -tx; tz = -tz; }
  rec->target_x = tx;
  rec->target_y = 0;
  rec->target_z = tz;
}

int fifa96_match_phase_02(struct fifa96_match_run *mr) {
  struct fifa96_match_entity *rec;
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (mr->phase_walk_active == 0u) return FIFA96_OK;   /* not the walk's caller */
  rec = mr->phase_walk_rec;
  if (rec == NULL) return FIFA96_OK;
  match_phase_dcc8_apply(mr, rec, mr->phase_walk_edge);
  return FIFA96_OK;
}

/* FU-192 §2.1 (M2 phase-17 T3): `0x6DE34..0x6DE40` — the shared reset/hold.
 * `LEA ESI,[EAX+0x59]; MOVSD; MOVSD; MOVSD` copies the record position triple
 * (`+0x59/+0x5D/+0x61`) into the target triple (`EDX` = `+0x4D`). Pure: no
 * globals, no state (FU-83 held-position copy). */
int fifa96_match_phase_00(struct fifa96_match_run *mr) {
  struct fifa96_match_entity *rec;
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (mr->phase_walk_active == 0u) return FIFA96_OK;
  rec = mr->phase_walk_rec;
  if (rec == NULL) return FIFA96_OK;
  rec->target_x = rec->pos_x;
  rec->target_y = rec->pos_y;
  rec->target_z = rec->pos_z;
  return FIFA96_OK;
}

/* FU-192 §2.4 (M2 phase-17 T3): `0x6DE44..0x6DF4A` — rows `03 04 07`. The
 * per-side selector `word[0x1577D8 + gside*4]` (gside = `[0x157AAC]>>24`, the
 * engine's `phase_machine.side_controlled`; the runtime cell is unported,
 * staged 0 → the DX>=0 arm). The DX>=0 arm reads the `[rec+0x14]` dword pair at
 * index `word[0x1577D6 + gside*4] >> 16` and, when the controlled-side byte
 * `[0x157AAF]` differs from the record side, +4 bytes, then writes
 * `target[0] = ±(W + (W>>2))`, `target[2] = ±(D + (D>>4))` (side-flipped) and
 * `target[1] = 0`. The DX<0 cell-map arm (`0x6D870`/`0x6DCC8`, base
 * `0x15774C`) is FU192-L3. Gated on the unported `[rec+0x14]` (FU192-L1): the
 * live walk stages NULL, so this is a dispatched-but-gated no-op. NOTE (FU-192
 * §2.4 is a sketch): the fresh `disassemble_bytes` here refines it — the scale
 * is `W+(W>>2)` / `D+(D>>4)` (not the sketch's rough "×1.25"/"×1.0625") and the
 * `[0x157AAF] != side` `+4`-byte branch is now byte-level; the branch is
 * untested (needs `[rec+0x14]` + the unported side cell) and recorded as a
 * FU192-L1 leg. */
int fifa96_match_phase_04(struct fifa96_match_run *mr) {
  struct fifa96_match_entity *rec;
  const int32_t *p;
  static const int32_t sel_1577d6[2] = { 0, 0 };  /* word[0x1577D6+gside*4] (unported) */
  static const int32_t sel_1577d8[2] = { 0, 0 };  /* word[0x1577D8+gside*4] (unported) */
  int32_t side;
  int32_t gside;
  int32_t idx;
  int32_t off;
  int32_t dw;
  int32_t w;
  int32_t d;
  int32_t tx;
  int32_t tz;
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (mr->phase_walk_active == 0u) return FIFA96_OK;
  rec = mr->phase_walk_rec;
  if (rec == NULL) return FIFA96_OK;
  if (rec->team >= FIFA96_MATCH_ENTITY_TEAMS) return FIFA96_OK;
  side = (int32_t)mr->entities.team[rec->team].side;      /* [team+0x826] */
  gside = (int32_t)mr->phase_machine.side_controlled;     /* [0x157AAC]>>24 */
  if ((int16_t)(uint16_t)sel_1577d8[(uint32_t)gside & 1u] < 0)
    return FIFA96_OK;                                     /* DX<0 arm: leg FU192-L3 */
  idx = (int32_t)(sel_1577d6[(uint32_t)gside & 1u] >> 16);
  p = rec->phase_meta14;                                  /* native [rec+0x14] */
  if (p == NULL) return FIFA96_OK;                        /* OL-13 gate FU192-L1 */
  off = ((int8_t)mr->phase_machine.side_controlled != (int8_t)side) ? 1 : 0;
  dw = p[idx * 2 + off];                                  /* 0x6DE9A/0x6DEAE */
  w = (int32_t)(int16_t)(uint16_t)dw;
  d = (int32_t)(int16_t)(uint16_t)((uint32_t)dw >> 16);
  tx = w + (w >> 2);
  tz = d + (d >> 4);
  if (side != 0) { tx = -tx; tz = -tz; }
  rec->target_x = tx;
  rec->target_y = 0;
  rec->target_z = tz;
  return FIFA96_OK;
}

/* FU-192 §2.7 (M2 phase-17 T3): `0x6E05C..0x6E1C5` — row `05`. The derivable
 * arm is the `0x6E1B3` fall-through `MOVSD ×3`: `target := rec position`. The
 * ball-relative arm (`[0x157A9F]` ball-team compare, the `0x8DC68` metric, the
 * `0x114E04` cosine fold, the `0x157AC5`/`0x15881A`/`0x157AB6` cells) is
 * FU192-L1: the native ball record's `[+0]` team pointer has no engine model.
 * The native `0x6E072 JNZ 0x6E1B3` fires whenever `[rec] != [ball]`; staged as
 * the derived position copy. Off-tape: tape-neutral. NOTE: on the derivable
 * subset this body is byte-identical to `fifa96_match_phase_00` (`0x6DE34`) —
 * two distinct native bodies (`0x6E05C`'s ball-relative arm is the unported
 * part) that coincide here; kept separate so a future arm divergence does not
 * couple them. */
int fifa96_match_phase_05(struct fifa96_match_run *mr) {
  struct fifa96_match_entity *rec;
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (mr->phase_walk_active == 0u) return FIFA96_OK;
  rec = mr->phase_walk_rec;
  if (rec == NULL) return FIFA96_OK;
  rec->target_x = rec->pos_x;
  rec->target_y = rec->pos_y;
  rec->target_z = rec->pos_z;
  return FIFA96_OK;
}

/* FU-192 §2.3 (M2 phase-17 T3): `0x6DD9C..0x6DE31` — row `06`. Index
 * `([0x157754] > 0)` for side 0 / `([0x157754] < 0)` for side 1 (the runtime
 * cell is unported, staged 0 → index 0), reads the `[rec+0x10]` dword pair at
 * `p[idx]`: `target[0] = ±(W + (W>>2))`, `target[2] = ±(D + (D>>4))` with the
 * `[0x157754]`-sign `±0x60` z bias, `target[1] = 0`. Gated on the unported
 * `[rec+0x10]` (FU192-L1). */
int fifa96_match_phase_06(struct fifa96_match_run *mr) {
  struct fifa96_match_entity *rec;
  const int32_t *p;
  static const int32_t cell_157754 = 0;   /* [0x157754] (unported runtime cell) */
  int32_t side;
  int32_t idx;
  int32_t dw;
  int32_t w;
  int32_t d;
  int32_t tx;
  int32_t tz;
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (mr->phase_walk_active == 0u) return FIFA96_OK;
  rec = mr->phase_walk_rec;
  if (rec == NULL) return FIFA96_OK;
  if (rec->team >= FIFA96_MATCH_ENTITY_TEAMS) return FIFA96_OK;
  side = (int32_t)mr->entities.team[rec->team].side;
  p = rec->phase_meta10;                  /* native [rec+0x10] */
  if (p == NULL) return FIFA96_OK;        /* OL-13 gate FU192-L1 */
  idx = (side != 0) ? (cell_157754 < 0 ? 1 : 0) : (cell_157754 > 0 ? 1 : 0);
  dw = p[idx];
  w = (int32_t)(int16_t)(uint16_t)dw;
  d = (int32_t)(int16_t)(uint16_t)((uint32_t)dw >> 16);
  tx = w + (w >> 2);
  tz = d + (d >> 4);
  if (side != 0) { tx = -tx; tz = -tz; }
  if (cell_157754 > 0) tz -= 0x60; else tz += 0x60;   /* 0x6DE15 */
  rec->target_x = tx;
  rec->target_y = 0;
  rec->target_z = tz;
  return FIFA96_OK;
}

/* FU-192 §2.2 (M2 phase-17 T3): `0x6DD6C..0x6DD98` — rows `08 09`. The thin
 * wrapper: `idx = ([team+0x826] == [0x157AAC]>>24) ? 0xBC : 0x6F`, then
 * `0x6DCC8(rec, target, idx)` (the shared `match_phase_dcc8_apply`). */
int fifa96_match_phase_08(struct fifa96_match_run *mr) {
  struct fifa96_match_entity *rec;
  int32_t side;
  int32_t gside;
  int32_t idx;
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (mr->phase_walk_active == 0u) return FIFA96_OK;
  rec = mr->phase_walk_rec;
  if (rec == NULL) return FIFA96_OK;
  if (rec->team >= FIFA96_MATCH_ENTITY_TEAMS) return FIFA96_OK;
  side = (int32_t)mr->entities.team[rec->team].side;
  gside = (int32_t)mr->phase_machine.side_controlled;   /* [0x157AAC]>>24 */
  idx = (side == gside) ? 0xBC : 0x6F;                  /* 0x6DD80..0x6DD8E */
  match_phase_dcc8_apply(mr, rec, idx);
  return FIFA96_OK;
}

/* FU-192 §2.5 (M2 phase-17 T3): `0x6DF4C..0x6E001` — row `0C`, the stateful
 * restart-line timer. `[rec+0x89] += [0x157A64]` on every call; past the
 * `0x3C` gate the target becomes `(0x780, ·, 0)`, refined to `(0xCC0, ·, 0)`
 * when `|word[rec+0x69]| < 0x20`. The table/timer7b (`[rec+4]`→`[+0xA]`,
 * `0x157A38`), the angle term (`[rec+0x8A]`) and the `0x8DCD4` metric are
 * FU192-L1 (unported). Off-tape: tape-neutral. */
int fifa96_match_phase_0C(struct fifa96_match_run *mr) {
  struct fifa96_match_entity *rec;
  int32_t lane_word;
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (mr->phase_walk_active == 0u) return FIFA96_OK;
  rec = mr->phase_walk_rec;
  if (rec == NULL) return FIFA96_OK;
  rec->timer89 += (int32_t)(uint16_t)mr->record.delta;  /* 0x6DF55..0x6DF63 */
  if (rec->timer89 < 0x3C) return FIFA96_OK;            /* 0x6DF69 JL */
  rec->target_x = 0x780;                                /* 0x6DF8F */
  rec->target_z = 0;                                    /* 0x6DF95 + angle*0 */
  lane_word = (int32_t)(int16_t)(uint16_t)rec->lane;    /* native word[rec+0x69] */
  if (lane_word < 0) lane_word = -lane_word;            /* 0x6DFD5..0x6DFEA */
  if (lane_word < 0x20) {                               /* 0x6DFEC JL */
    rec->target_x = 0xCC0;                              /* 0x6DFF1 */
    rec->target_z = 0;
  }
  return FIFA96_OK;
}

/* FU-192 §2.6 (M2 phase-17 T3): `0x6E004..0x6E05B` — row `10`. `BL =
 * [rec+0x8D]`; `active == 0` → the `(0x3C0, 0, 0x840)` default; else the
 * `0x1105EA` 2-stride signed-byte table at `EBX = active-1` scaled `<< 5`, with
 * `target[2]` negated when `[team+0x826] != 0`, and `target[1] = 0`. The table
 * bytes are a fresh `read_memory 0x1105E0` of the LE image (FU-192 §2.6). Row
 * `0x10` is never written by an engine source (the 9 `match_run_write_phase`
 * call sites, FU-191 §10), so its slot stays NULL: an un-dispatched count would
 * be fake (the rows-27/29/2C precedent). DERIVED HARDENING (leg FU192-L1): the
 * native indexes the `0x1105EA` table blindly (`active` is the record ordinal
 * 0..10, so `k = active-1 ∈ [-1, 9]` and the native reads out of the table for
 * the sign-extended `k = -1`); the engine bounds `k` to `[0, 10]` and no-ops
 * otherwise. The row is unwired, so there is no live impact. */
int fifa96_match_phase_10(struct fifa96_match_run *mr) {
  static const int8_t variant[11][2] = {
      { -8, -2 }, { -4, -2 }, { 0, -2 }, { 4, -2 }, { 8, -2 }, { -5, -5 },
      { 0, -5 }, { 5, -5 }, { -3, -7 }, { 2, -7 }, { -16, 0 } };
  struct fifa96_match_entity *rec;
  int32_t active;
  int32_t k;
  int32_t tx;
  int32_t tz;
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  if (mr->phase_walk_active == 0u) return FIFA96_OK;
  rec = mr->phase_walk_rec;
  if (rec == NULL) return FIFA96_OK;
  active = (int32_t)(int8_t)rec->active;                /* BL = [rec+0x8D] */
  if (active == 0) {
    tx = 0x3C0;                                         /* 0x6E046 */
    tz = 0x840;
  } else {
    k = active - 1;                                     /* MOVSX BX,BL; DEC */
    if (k < 0 || k > 10) return FIFA96_OK;
    tx = (int32_t)variant[k][0] * 32;                   /* SAR 0x18; SHL 5 */
    tz = (int32_t)variant[k][1] * 32;
    if (rec->team < FIFA96_MATCH_ENTITY_TEAMS &&
        mr->entities.team[rec->team].side != 0u)
      tz = -tz;                                         /* 0x6E036..0x6E041 */
  }
  rec->target_x = tx;
  rec->target_y = 0;
  rec->target_z = tz;
  return FIFA96_OK;
}

/* FU-177 §2/§7 + FU-198 §5.3: the ported action row-03 body
 * `0x7E1A4..0x7E524`. The live path (phase 2) is: prologue ran, guard A, the
 * idle hold, the `[rec+0x1C]` phase call, the placement counter, the phase-2
 * clamp and the `FUN_00079F3C` camera place. The unported arms are named legs
 * in the evidence (FU177-L1..L6). */
int fifa96_match_action_03(struct fifa96_match_run *mr) {
  struct fifa96_match_run_record *r;
  struct fifa96_match_team *team;
  struct fifa96_match_entity *e;
  fifa96_action_vec3 pos;
  fifa96_action_vec3 out;
  int32_t id;
  uint32_t t;
  uint32_t phase;
  if (mr == NULL) return -FIFA96_ERR_INVALID;
  r = &mr->record;
  id = r->entity_id;
  if (id < 0 ||
      id >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    return -FIFA96_ERR_INVALID;
  t = (uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS;
  team = &mr->entities.team[t];
  e = &team->records[(uint32_t)id % FIFA96_MATCH_ENTITY_RECORDS];
  phase = (uint32_t)mr->state.phase;
  r->ran = 1;                                        /* 0x7E1AF */
  /* guard A (0x7E1B6): rec == [team+0x7BA] (the interception target) ->
   * FUN_00079B58(rec); exit. FU177-L3: the helper writes +0x93 = 0x10 when
   * +0x99 == 0 (the engine's `+0x99` is staged 0); the pool's timer93 is +0x93. */
  if (team->intercept == id) {
    e->timer93 = 0x10u;                              /* 0x79B58 */
    return FIFA96_OK;
  }
  /* phase 0x13/0x14 special arm (0x7E1CE..0x7E223): the target triple
   * (0x780,0,0) and the FUN_0008DCD4(&rec[0x59], &rec[0x4D]) fast-length metric
   * < 0x30 -> target_x = 0xAE0 are derived here, and the FUN_00079B58 +0x93
   * write is inlined (FU177-L3). FU177-L4 now covers only the arm's remaining
   * sub-parts, not the target writes. */
  if (phase == 0x13u || phase == 0x14u) {
    fifa96_arm_vec from = { r->pos_x, r->pos_y, r->pos_z };
    fifa96_arm_vec to = { 0x780, 0, 0 };
    int32_t metric = 0;
    int32_t lane_unused = 0;
    r->target_x = 0x780;
    r->target_y = 0;
    r->target_z = 0;
    (void)fifa96_arm_dist_stage(&from, &to, &metric, &lane_unused);
    if ((int16_t)metric < 0x30) r->target_x = 0xAE0;
    e->timer93 = 0x10u;                              /* FUN_00079B58 */
    return FIFA96_OK;
  }
  /* slot gate (0x7E224): slot != 0 && phase != 0xC -> the slot branch. */
  if (r->has_slot != 0 && phase != 0x0Cu) {
    /* slot branch A (0x7E230): the `[0x157A49]>>24` sub-gate (native byte
     * 0x157A4C; `mr->global_157a4c`) == 1, or phase 6 / phase 0x15 ->
     * CALL [rec+0x1C]; exit. */
    if (mr->global_157a4c == 1u || phase == 6u || phase == 0x15u) {
      match_row_phase_call(mr, e, r);
      return FIFA96_OK;
    }
    /* slot branch B (0x7E25F..0x7E27C): FUN_00079C20(rec, [slot+0x1D]>>24,
     * [slot+0x1E]>>24) then `word[rec+0x7B] = word[rec+0x79]`. FU177-L6/OL-19:
     * FUN_00079C20 is unported; the derived subset reloads the counter word from
     * +0x79 (the native `[rec+0x79]` source is unported on the staging record,
     * so the pool entity's timer79 stands in). */
    r->timer7b = e->timer79;
    return FIFA96_OK;
  }
  /* idle hold (0x7E281..0x7E2C9): the controlled (or tracked) record's code in
   * {0x10,0x11,0x12} -> `target := rec position`; exit. The native source is the
   * controlled record `[0x157A83]` (else the tracked `[0x1577CA]`, unported ->
   * FU177-L1 leg) with the `[0x157821] == 0` gate (staged 0). */
  {
    int32_t controlled = mr->entities.controlled;
    uint8_t code = 0;
    uint8_t held = 0;
    if (controlled >= 0 &&
        controlled < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
      code = mr->entities
                 .team[(uint32_t)controlled / FIFA96_MATCH_ENTITY_RECORDS]
                 .records[(uint32_t)controlled % FIFA96_MATCH_ENTITY_RECORDS]
                 .code;
    pos.x = r->pos_x;
    pos.y = r->pos_y;
    pos.z = r->pos_z;
    (void)fifa96_action_locomotion_hold(code, &pos, &out, &held);
    if (held != 0) {
      r->target_x = out.x;
      r->target_y = out.y;
      r->target_z = out.z;
      return FIFA96_OK;
    }
  }
  /* idle core (0x7E2D3..0x7E308): the `[rec+0x1C]` phase call, then the
   * placement counter `word[rec+0x7B] = min(([rec+0x63]>>22)+1, word[rec+0x79])`.
   * FU177-L1: `[rec+0x63]` (move_attr) is unported on the staging record ->
   * staged 0, so `(0>>22)+1 = 1`; the limit word is the pool entity's `timer79`
   * (`[rec+0x79]`), i.e. the derived counter is min(1, e->timer79). */
  match_row_phase_call(mr, e, r);
  {
    uint16_t counter = 0;
    (void)fifa96_action_locomotion_placement_counter(0, e->timer79, &counter);
    r->timer7b = counter;
  }
  /* phase-2 clamp (0x7E309..0x7E3A5): `bound_lo/hi = word[[rec]+0x80C+idx*4]
   * (+2)` and `opponent_z = [[0x1598xx]][+0x61]`. The team bound block
   * (FUN_0008C418) and the opponent selector are unported (FU177-L1/FU192-L3);
   * the derived clamp uses staged-0 bounds and opponent_z 0. */
  if (phase == 2u) {
    (void)fifa96_action_locomotion_clamp_placement(&r->target_z, r->pos_z, 0, 0, 0,
                                                   team->side, mr->session_gate_14c32a);
  }
  /* camera (0x7E3A6): CALL 0x79F3C -> the ported FIF-96 leg-5 whole-pool pass
   * (`fifa96_match_entities_camera_place`). Its phase gate `0x1106C3[phase]`
   * makes it a no-op on phase 2 (the byte at 0x1106C3+2 is 0). */
  (void)fifa96_match_entities_camera_place(&mr->entities,
                                           mr->phase_machine.side_controlled,
                                           (uint8_t)phase, mr->render.camera.pos_x,
                                           mr->render.camera.pos_z);
  /* phase-1 clamp (0x7E3CA): phase != 1 -> skip. */
  if (phase == 1u) {
    (void)fifa96_action_locomotion_phase1_clamp(team->side, &r->target_z);
  }
  /* phase 7/8/0xC tails (0x7E418..0x7E51A): the 0x158747 wall scan
   * (FU177-L2/FU-149 L13) and the phase-8/0xC clamps are legs (FU177-L6); the
   * derived subset performs the phase-0xC `word[rec+0x7B] = 2` only. */
  if (phase == 0x0Cu) r->timer7b = 2u;                /* 0x7E510 */
  return FIFA96_OK;
}

const struct fifa96_match_handler fifa96_match_action_table[FIFA96_MATCH_ACTION_ROWS] = {
    {0x00, fifa96_match_action_00,
     "FU-138 §4/FU-141: row 00 ported over the entity pool; install/ran drained by the pool installer"},
    {0x01, fifa96_match_action_01,
     "FU-143 §11 (M2 playable-match Task 2/OL-84): row 01 ported (0x7DBC0..0x7DFC8, phase==1 gate, stage 0/1/2 walk) — situation 0xB -> phase 2; camera/event/ball-stage sinks + rows 02/0x10..0x13 legs OL-84"},
    {0x02, fifa96_match_action_02,
     "FU-75 L4.6/FU-138 OL-18 (M2 phase-9 T1): row 02 ported (0x7DFCC..0x7E1A2, first-hand): the phase-1 restart mirror (locomotion_restart_target), the phase-2 [team+0x7B2] bind + merge request + install-4 invoke-now over the pool, the no-slot camera-target/timer + the restart-wait stage 0; the stage-1/2 arms (nearest/metric/0x92820/0x7A490/0x79B1C) stay OL-18"},
    {0x03, NULL,
     "FU-177 §2/§3/§7 + FU-198 §5.3 (phase-17 T1): row 03 hold/clamp body 0x7E1A4..0x7E524 ported (fifa96_match_action_03, over the FU-138 helpers fifa96_action_locomotion_hold/clamp_placement/placement_counter/phase1_clamp + fifa96_match_entities_camera_place for 0x79F3C) but NOT counted: the live phase-2 hot path's [rec+0x1C] = 0x6DCC8 target-rewrite chain is TAPE-VISIBLE — wiring the slot moved M2 at transcribed frame 29 (the forced code-3 record's row-03 body runs in the phase-2 window and its phase-0x13/0x14 special arm rewrites the target chain), so per controller R7 the row legs (owner FU177-L1) and the slot stays NULL/uncounted; the body is exported for direct unit testing. The per-record [rec+0x1C] seam (FU175-L1) is landed by T1 (fifa96_match_dispatch_phase_record + the walk cursor); remaining legs: the FUN_0008C418 team bound block (the phase-2 clamp inputs), FUN_00079C20 (slot branch B), FUN_00079B58 (+0x93, inlined on the special arm), the phase-0x13/0x14 special arm's remaining sub-parts (its target/metric are derived), the phase-7 0x158747 wall scan, the phase-8/0xC tails (FU177-L1..L6) and the OL-13 [rec+0xC] metadata (FU192-L1). OL-19"},
    {0x04, fifa96_match_action_04,
     "FU-142 App. K.5/FU-137 §6.1 (Task 1/OL-70): row 04 ported (fifa96_outfield_row04_step, 0x7E7C8..0x7F141) over the pool; the carrier/ranked-pick/forced/chase installs 4/0x19/0xF/0xE/0xB/7/6/5 and the target/timer/team writes are bound; the 0x7CA54 input-row machine stays a separate unwired seam; unmodeled record/team bytes, track words and sinks OL-65/OL-67/OL-72; the +0x8E record.type/actor_type split OL-83"},
    {0x05, fifa96_match_action_05,
     "FU-139 §8/§5 + M2 phase-9 T2/OL-63: row 05 carrier machine ported (0x7F194..0x7F665 stages 0-3) + staging tail 0x7A8D1..0x7AA2F; wired: the 0x7F1FF [0x158724] claim + the 0x158728..0x15872F block, team-target bind, capped timer, camera target, stage-0 control/dir gates, the 0x7876C merge and stages 1-3 (snap/face/hand-off) over the pool; stage-0 tail 0x7F3A1..0x7F57B + FUN_0007F7E0 fallback stay OL-63 legs (named requests); sinks 0x92820/0x6E598 OL-27/OL-52"},
    {0x06, fifa96_match_action_06,
     "FU-139 §11 (Task 13/OL-30): row 06 ported (0x801B4..0x809EF pursuit machine; 0x8DCD4 metric + 0x8DD70 angle + 0x114E04 folds, RNG installs 8/9, carrier-gate install 4, 0x8DE8C/0x79CCC selections) over the pool; row-09 0x80A00 boundary; record bytes/lead/swap OL-69"},
    {0x07, fifa96_match_action_07,
     "FU-139 §9/FU-137 §5.2: row 07 ported (0x814B0..0x81737 machine; 0x7B9C4 kick path) over the pool; defender 0x0E/opponent 0x22/ball 4 requests; descriptor bytes and predictor OL-65/OL-66"},
    {0x08, fifa96_match_action_08,
     "FU-142 App. K.6/FU-137 §6.1 (Task 2/OL-70a): row 08 ported (fifa96_outfield_row08_step, 0x81068..0x814AF) over the pool; the stage-0 camera/face, the +0x3D-gated projection scan, the 0x79B1C snap and the process-byte writes are bound; row 08 installs no code; unmodeled record/process bytes, the 0x7A490 staging and the event/sound sinks OL-52/OL-62/OL-82; the +0x8E record.type/actor_type split OL-83"},
    {0x09, fifa96_match_action_09,
     "FU-178 §3/§8 (phase-15 T3): row 09 stage-0 ported (fifa96_match_action_09, 0x80A00..0x80BFA arm 0 of table 0x809F0): the phase==2 gate (0x80A0B, else 0x7DAB4 reset), the stage>3 tail (0x80A2A), the arm-0 clear branch (reset + detach [team+0x7B2]/[team+0x7B6]) and the arm-0 set branch (the 0x79C50 ball-relative face, the two 0x114E04 sine folds via fifa96_entity_sine x 0x795A4, the min(+0x69>>17,0x30) speed, the +0x59/+0x61 accumulate, the +0x73/+0x75 velocity and 0x8DC68 speed, the +0x4D target copy, the 0x6E598 anim 0x11 install, +0x9E/[0x15877C]/+0x89/+0x92); arms 1/2/3 (stages 1-3, incl. the 0x0C/0x0D installs) are FU178-L1 (mr->match09_leg_stage records the unported stage); the 0x1577C0/C2 lead words, word[+0x79] and [0x15877C] have unported producers (OL-82); installer live (pad release 0x7CFB2 + wired row-06 RNG arm 0x807C1)"},
    {0x0A, NULL, "FU-137 §6: FU-136 row 0A: not ported; installer 0x7CDD8 has no xrefs; OL-14"},
    {0x0B, fifa96_match_action_0B,
     "FU-169 §3 (phase-14 T1): row 0B duel body ported (0x81908..0x81C71, fresh first-hand): arm 0 (index gate, the point pos+dir<<5, the nearest opponent, the strict (0x20,0x70) band, the speed/facing {0x0C,0x59} choice, the 0x79B6C snap+face, the anim install on self, ran, stage->1), arm 1 (the +0x3D frame gate, nearest, found active, band <= 0x60, the [0x14C32A] session gate, the +0x7D7 duel counter, the kind-2 registrar, the +0x8E type copy, the action-0x0C install on the found record, stage->2) + the same-dispatch arm-2 snap/+0x44 reset; the ring-post [[rec+4]]==0x18DE (FU169-L3), the events/sinks/0x8ED40 (FU169-L2), the installer tails and the arm-0 fall-through inertness are documented in the body"},
    {0x0C, fifa96_match_action_0C,
     "FU-169 §2 (phase-14 T1): row 0C contact body ported (head 0x81C90..0x81CD4 + stage 0 0x81CDC..0x81DC5, fresh first-hand): timer89 += delta, the pos_y==0 target copy, the active reset, A = [0x15888F], the A anim-0x59/frame>=2 gate, the 0x795B4 lead band (A.pos + 2*A.vel) >= 0x30 -> the +0x7CF stumble counter, the anim-0x5E install on A, the self reset; the <0x30 and alternate-gate foul branches call fifa96_match_run_contact (the disclosed kind-1 re-register) after the 0x81E13 B velocity write; stages 1..6 (0x81F4E..0x8250B) stay FU169-L4 legs"},
    {0x0D, NULL,
     "FU-180 §3/FU-82 §3.4 + FU-178/FU-190 §6.1 (phase-16 T5): row 0D body 0x8251C..0x826FE (4-arm table 0x8250C) is ported as fifa96_action_sequence_0d_step (fifa96_action_handlers.c) but NOT counted: its only installer is row-09's 0x80F61 (fresh bytes 0x80F61 = ba0d000000 MOV EDX,0xD; 0x80F6A = e835caffff CALL 0x7D9A4), inside row-09 arm 1 (stages 1-3, FU178-L1, unported). T5 (first-hand 0x80BFB..0x80FCB) confirms arm 1's install path is gated by unported producers ([0x15877C] FU178-L6, [rec+0x3D] the FU-84 frame, [rec+0x24] the owner, [[rec+4]+0x13]) and the unported arm-1 helpers/sinks (0x8f188/0x8ed40/0x92820/0x7a490/0x8a3fc) — the bounded arm is not shown reachable (FU-178 §6 ranks it heaviest, last), so the row stays NULL/uncounted (leg FU178-L1). A ported body with no ported installer stays unwired (the rows-27/29/2C precedent); the 0x702F8/0x6E598/0x79C50/0x8DCD4 sinks are FU180-L8; OL-22"},
    {0x0E, fifa96_match_action_0E,
     "FU-160 §2/FU-159 §3.3 + FU-167/FU-168 (phase-13 T3, completed phase-14 T1): row 0E ported (0x82710..0x82ACF, fresh first-hand): the phase gate + timer89 accumulate, stage 0 (index gate, 0x71B9C camera-target stand-in, 0x8DCD4 metric + 0x79C50 face), stage 1 (0x157794 target, 0x79B58 self receiver timer, timer89 >= 0x0C -> install row 0x12 on self via FUN_0006E598 frame 0, ran), stage 2 (tracked [0x1577CA]/jump_base -> 3; the else arm 0x82820..0x82A9E is wired: the (0x20,0x70) camY and lane_x < 0x60 gates -> retry, the focus pos+0x40*dir[type], the seed/angle/amplitude vector and the FUN_00071C94 event + [0x1577CA] tracked bind; FU-167/FU-168; the sinks/descriptor/direction-byte legs FU167-L1..L5, the live Y producer FU168-L5) and stage 3 (+0x44 reset) — the live jump selector: row-23 tackle -> install 0x0E -> row 0x12 -> the ported jump arm/arc"},
    {0x0F, fifa96_match_action_0F,
     "FU-139 §9/FU-137 §5.2: row 0F ported (0x82AD0..0x82DCF machine; 0x7B9C4 kick path + 0x79B1C snap/0x79B6C face) over the pool; corner/predictor table inputs OL-66"},
    {0x10, fifa96_match_action_10,
     "FU-149 L13/T1: row 10 ported (throw-in taker, 0x855F0..0x85DE3, gate {2,3}, table 0x855B8 7 arms): snapshot placement (0x8566D), FUN_000832A8 stand-in (0x857D6), FUN_00085498 delivery probe (0x8580D), ball-line/pick/install-4 (0x858E4), resolution/requeue (0x85CC4/0x85D19), +0x44 wait and reset; staging bodies, event sinks and the [0x157821] producer are FU-149 L13 legs"},
    {0x11, fifa96_match_action_11,
     "FU-149 L13/T1: row 11 ported (corner taker, 0x85DE4..0x864FF, gate {2,4}, table 0x85DA0 10 arms): 0x3C wait (0x85E3C), FUN_0007D360 corner probe ±0x50 placement (0x85E7D), advances, FUN_000832A8 (0x85FD8), delivery probe (0x86013), lane gate (0x860CB), kick + resolution + relay (0x8610E), +0x44 wait, target clamp (0x86496); the seven-arm kick vectors and sinks are FU-149 L13 legs"},
    {0x12, fifa96_match_action_12,
     "FU-149 L13/T1: row 12 ported (free-kick taker, 0x83D68..0x84597, gate {2,7}, tables 0x83D2C/0x83D4C): incident placement + wall-point fold (0x83E26), FUN_000832A8 (0x84077), delivery probe span 3 (0x840B2), lane gate (0x8416A), kick + resolution + relay (0x841AD), +0x44 wait, reset; the wall block 0x158747/0x83B80 and kick vectors are FU-149 L13 legs"},
    {0x13, fifa96_match_action_13,
     "FU-149 L13/T1: row 13 ported (penalty taker, 0x84B00..0x84EEB, gate {2,6}, table 0x84AE4 7 arms): spot focus + commit (0x84BAD), spot aim distance gate (0x84C48), opponent/slot gate (0x84CBA), lane gate (0x84DF4), penalty strike mode 0x40 (0x84E26), ball-ack gate + resolution (0x84E61), +0x44 reset; the 0x4C114 input words, 0x78A84/0x78AA4 slot bodies and the 0x20-edge hand-off are FU-149 L13 legs"},
    {0x14, fifa96_match_action_14,
     "FU-184 §4/§7 (phase-16 T2): row 14 celebration/scatter body ported (0x8784C..0x87CBC, 3 stages): head timer89 += delta; stage 0 = the 5-point fifa96_action_sequence_scatter_celebration + the 6-slot buffer (points[5] = the clamped points[4], FU184-L1) + the arm block (target/metric/face); stage 1 = the arm walk with the distance<0x20 / timer>0x4B0 / idx3>0x12C / idx4>0xB4 advance and the 0x87BE9 stage-- re-entry; stage 2 = match_row_reset + the release (camera reset + fifa96_match_run_kickoff_entry 0x87C71 + the 0x73E08 placement commit) unless side==1 (0x87C1A, FU184-L6). Installer live (phase-5 goal arm match_run.c:2494); legs: the ball-record anim (FU184-L7), the 0x10F344 id tables/anim post (FU184-L1 sibling), the 0x45001 input gate (FU184-L3), the head sinks (FU184-L4) and the release tails (FU184-L5)"},
    {0x15, NULL, "FU-137 §6: FU-136 row 15: not ported; head mis-decoded, stub bucket; OL-14"},
    {0x16, fifa96_match_action_16,
     "FU-187 §1/§2 (phase-16 T4): row 16 fouled-player face-the-ball ported (fifa96_match_action_16, 0x84630..0x8471C, 3 stages): stage 0 copies pos -> target then waits on the object code `[0x158866] == 0x48` (mr->referee.referee_object_code; FU187-L1 polarity leg), stage 1 the 0x795B4 band toward the ball + the 0x92AC8 anim pick (0x6A/0x55) + 0x6E598 install + 0x79C50 face, stage 2 the +0x44 -> 0x7DAB4 reset. PRODUCER LIVE: the phase-0x19 handler's stage-0 install (`0x8A0A0 EDX=0x16`) via the act-3 seam (match_run.c:contact -> fifa96_match_act_dispatch(3,0,1) -> phase 0x19 -> match_run_ref_apply install 0x16). The band out-args (FU187-L2), the anim/face sinks (FU187-L4) and the `[0x158866]` identity (FU187-L1) are legs"},
    {0x17, NULL,
     "FU-181 §2 (phase-15 T1): row 17 body ported (fifa96_match_action_17, 0x84730..0x849AF: the 0x84743 phase-0xB gate, the 0x84720 4-arm stage table, the -0x680/±0x50 facing walk and the 0x849A1 0x7DAB4 reset); NOT counted — no engine installer: the only producer act-4 stage 0 (0x89740) is behind the unported sit-2..4 formation-order arm 0x8AB08..0x8AB62 (FU-149 L12) and the unported formation matrices; the rows-27/29/2C precedent (ported body, entry unresolved); FU181-L1; OL-9; -UNSUPPORTED"},
    {0x18, fifa96_match_action_18,
     "FU-139 §10/FU-137: row 18 ported (0x849B0..0x84AE1 machine; 0x4C324 bind, 0x8DB6C NSEARCH + 0x786A0 SWAP, 0x7DAB4 reset) over the pool; install arm 0x8A32F; anim row byte and bind globals OL-68"},
    {0x19, fifa96_match_action_19,
     "FU-179 §3/§6 rank 1 (phase-15 T2): row 19 keeper-hold body ported (fifa96_match_action_19, 0x746E4..0x74CB7): +0x9E/[0x157C5D], the deepest-keeper tracker (hold_track), the +0x98 early-out, the slot/human-phase arm, the phase gate (no-slot 0x74C98 phase-method request FU179-L5), the phase-2 guard (hold_guard/guard_clamp, all five rejections incl. the tracked-opponent lane 0x747EA), the predictor (fifa96_keeper_hold_predict; six 0x1577xx leads staged zero, FU179-L1/L2/L3), the carrier dive (hold_intercept) and the weighted fallback (hold_fallback); the 0x79C20 slot placement FU179-L4 and the FUN_00079B58 receiver-timer arms 0x74823/0x74B63 FU179-L7; installer live (row-04 0x7E866/0x7E8C1)"},
    {0x1A, NULL,
     "FU-186 §2.1/§4.1 (phase-16 T3): row 1A keeper-reposition body ported (fifa96_match_action_1A, 0x7662C.. stage-0 slice: the phase-0xD+[0x157AAC] side bind, the +0x89 timer, the +0x92>4 tail and the fifa96_keeper_reposition_a_gate stage-0 arm + the 0x79B1C snap) but NOT counted: its only installer is the unported FUN_00077EAC CPU ladder (0x78227/0x7824F/0x78277, FU186-L5) on the unported FUN_0008D8EC -> FUN_000782D0 pre-pass; the rows-27/29/2C precedent (ported body, entry unresolved); the [0x157C2C] direction machine + movement/event block + stages 1-4 are FU186-L1; OL-34; -UNSUPPORTED"},
    {0x1B, fifa96_match_action_1B,
     "FU-180 §2 (phase-15 T2): row 1B reposition/rush bounded body ported (fifa96_match_action_1B, 0x76D28..0x77727 5-arm table {0x76DF6,0x76E3B,0x76FC7,0x7769F,0x776C1}): the phase-0xD+side head camera place/take/actor, the timer, stages 0/1/3/4 (0x76DF6/0x76E3B/0x7769F/0x776C1) and the 0x77636 rush exit + the ported fifa96_keeper_reposition_b_finish; the 0x14E04 sine rotation, the 0x76B28 steering, the stage-2 wide body (0x71C94) stay FU180-L1/L2/L3; installer is row-0x1F's ported no-slot dive (0x76557 invoke-now); the second installer 0x78299 (FUN_0008D8EC pre-pass) FU180-L4/L5"},
    {0x1C, fifa96_match_action_1C,
     "FU-186 §2.2/§4.1 (phase-16 T3): row 1C keeper-lunge body ported (fifa96_match_action_1C, 0x77728..: head timer89 += delta, stage 0 (ran/timer89=0/keeper_latch[2]=0/stage->1), stage 1 = the 0x8DCD4 metric + the fifa96_keeper_lunge_track on-target pos shift (|angle(distance,target_y)|<0x55) and stage 2 (row44 -> match_row_reset); installer LIVE: the wired row-0x1F stage-3 tail drains fifa96_keeper_input_decide whose both-slot-dirs-zero arm installs 0x1C invoke-now (FUN_00076130 0x76291->0x762A4), so the route executes end-to-end; the 0x79C50 face, the +0x7F heading, the 0x77856 rotated arm (0x114E04 sine + 0x795A4), the 0x6E598 anim, the 0x76B28 steering and the 0x779DE tail stay FU186-L2; the second installer FUN_00077EAC 0x782B6 stays FU186-L5"},
    {0x1D, fifa96_match_action_1D,
     "FU-151 §Port contract item 2/FU-79 §6/§2.5: row 1D ported (fifa96_keeper_closedown_step, 0x74EB0..0x754E1, stages 0..4) over the pool; place/commit/helper/reset/situation + the clearance staging (events 0x30/0x31) bound; 0x157C5E table, 0x744D4/0x74CDC/0x79B1C/0x7B878/0x8F188/0x7A490/0x8DE8C+0x786A0 and the UI/audio calls FU-151 legs"},
    {0x1E, fifa96_match_action_1E,
     "FU-151 §Port contract item 1/FU-147 §8: row 1E ported as the full 10-stage machine (fifa96_keeper_claim_step, 0x7550C..0x7612F, table 0x754E4): claim/hold/outlet/release/restart + the 0x157C3x/42 process cells; unported sinks are request bits (FU-151 legs 3/4/12/13/15)"},
    {0x1F, fifa96_match_action_1F,
     "FU-176 §4/§5 (phase-15 T2): row 1F keeper dive/arm ported (fifa96_match_action_1F, 0x76380..0x765C2): the slot-merge prologue, [0x157C5C]=0, arm_camera (0x763B1), the +0x89 timer, arm_step (0x76425..0x76578, FU176-L1 fixed), the stage-2 no-slot RNG dive (dive_target) with the 0x1B install invoke-now (the co-scoped chain), the stage-3 tail input_decide drain (4/0x19/0x1C/0x1B) and the code-0x1F reset; the 0x79B6C hook/0x6E598 event/rec[+4] divisor stay FU176-L2/L4; installer live (phase-6 arm 0x8D558)"},
    {0x20, NULL,
     "FU-189 §1/§3/§6 + FU-190 §6.2 (phase-16 T5): row 20 installer FOUND at 0x899a2 (fresh bytes ba20000000 MOV EDX,0x20; 8b442408 MOV EAX,[ESP+8]; e8f43fffff CALL 0x7D9A4) inside the phase-0x1D handler 0x89868, but 0x89868's sole reference is the phase-table slot 0x110808 ([0x1D], fresh get_xrefs_to=1), i.e. the act-7 route (act a -> phase 0x16+a); no engine source dispatches situation 8 (the act-7 producer; the dispatched set is 1/2/3/6/9/0xB/0xC) and the phase-0x1D engine slot is NULL, so the installer is phase-seam-gated and never fired — the row stays NULL/uncounted (leg FU189-L1, owner: the phase-0x1D route). Body 0x84EEC (FU-82 7-arm table 0x84ED0) not ported; OL-9"},
    {0x21, fifa96_match_action_21,
     "FU-139 §10/FU-137: row 21 ported (0x85214..0x8539B machine; camera copy, 0x8DE8C nearest + 0x4A anim arm, 0x7DAB4 reset, team-target ball 4/receiver) over the pool; install arms 0x7D046 (and row-05 0x7F791/0x7F7B6); +0x44 and anim record writes OL-68/OL-52"},
    {0x22, NULL,
     "FU-185 §2/§10 (phase-16 T2): row 22 sequence/press body ported (fifa96_match_action_22, 0x8539C..0x85481) but NOT counted (the rows-27/29/2C precedent — a ported body with no reachable engine installer stays unwired). Its only installer is the wired row-07 0x816DF invoke-now on the opponent team target, but the gate is structurally ungated in-engine: FU185-L6 (first-hand) shows the opponent word[+0x6F]/word[+0x7D] angle inputs are unmodeled (OL-65, staged 0), so fifa96_entity_angle(lane_z, 0) always yields +-0x100 and the folded diff against opp_face_word==0 can never be < 0x55 — a brute-force sweep proves out.opponent_invoke never fires. Uncounted until OL-65 lands; the body is exported for direct unit testing (declared in fifa96_match_handlers.h). OL-65"},
    {0x23, fifa96_match_action_23,
     "FU-139 §10/FU-137: row 23 ported (0x82F84..0x83163 machine; 0x82DD0 attempt, stage-1 target arm, 0x79B58 receiver, installs 0x0E/0x0F) over the pool; install arm 0x7D1B9; 0x1577xx camera/track inputs staged zero OL-68"},
    {0x24, NULL,
     "FU-188 §4/§6 (phase-16 T4): row 24 lane/anim body ported (fifa96_match_action_24, 0x86510..0x866F1, 3 stages: head timer89 += delta + the +0x7B lane word, stage 0 the pos_x=0x9F0/±6·[[rec+4]+0xE] facing + 0x79C50 face + target commit, stage 1 the ball gates + `word[+0x65] < 0x20` wait, stage 2 the 0x6E1D0 release) but NOT counted: its phase-0x20 / act-0xA builder is produced only by situation 0, which no engine source/test dispatches (the direct-set situations are 3/5/6/7/9/0xB/0xC/0x0B; no set_piece(0)/situation(0) caller). The rows-27/29/2C precedent (ported body, no live installer); the body is exported for direct unit testing (fifa96_match_handlers.h); the `0x157A38` lane table (FU188-L5), the 0x79C50 threshold arithmetic, the `0x8EEC0`/`0x67800`/`0x8EEB4` gates and the `0x6E1D0` release (FU188-L3) stay legs; the situation-0 producer (FU188-L1) is the owner; OL-9"},
    {0x25, NULL,
     "FU-189 §4/§6 + FU-190 §6.2 (phase-16 T5): row 25 has NO static installer — the bounded exhaustive literal scan (MOV EDX/DX,0x25 in the three encodings BA25000000/66BA2500/C7C225000000) finds no FUN_0007D9A4 site. The only default-space 0x7xxxx MOV EDX,0x25 (0x7E460) is the 0x6E598 anim sink; 0x1f527/0x22033/0xa9b41 feed other calls; the .image hits are overlay false positives. No installer exists (leg FU189-L2), so the row stays NULL/uncounted and no installer is invented; body 0x880CC (FU-82 7-arm table 0x880B0) not ported; OL-9"},
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
    {0x00, fifa96_match_phase_00,
     "FU-192 §2.1 (phase-17 T3): phase 00 = the ported native 0x6DE34 (0x6DE34..0x6DE40), the shared reset/hold `target := position` (3x MOVSD from +0x59/+0x5D/+0x61). Also rows 0x0A/0x0D/0x0F (same body); 0x0B/0x0E share it but are never written by an engine source, so they stay NULL (an un-dispatched count would be fake). COUNTED after the on-tape cmp pin (phase 0x00 = the tape's 304-frame window, R10); the write is render-invisible on the tape (targets are render-invisible, FU-191 §10)"},
    {0x01, NULL,
     "FU-192 §2.9/L7 (phase-17 T3): phase 01 = the native 0x6E1D0 (`fifa96_scene_formation_place`), already realized by the seeding path (fifa96_match_entities_seed_formation, FU-89 §11); wiring the phase row would double-run the formation placement (R10 'do not double-wire', FU192-L7). The slot stays NULL/uncounted; the [rec+8] formation blob is not persisted in the pool; OL-13"},
    {0x02, fifa96_match_phase_02,
     "FU-191 §6 + FU-198 §5.1 (phase-17 T1): phase 02 = the ported native FUN_0006DCC8 (0x6DCC8..0x6DD3B): reads [rec+0xC] (modelled as fifa96_match_entity.phase_meta), indexes the direction-byte pair by the walk's EBX=-1 (the per-side default word[0x1577DE+side*4]>>16) or an explicit index, and writes target_x = byte0*±0x26 / target_y=0 / target_z = byte1*±0x21 (side-flipped by [team+0x826]). The [rec+0xC] OL-13 source is unported (FU192-L1), so an unset phase_meta is a documented dispatched-but-gated no-op (FU-177 §7); phase 0x15 shares the body (FU-136 phase rows)"},
    {0x03, fifa96_match_phase_04,
     "FU-192 §2.4 (phase-17 T3): phase 03 = the ported native 0x6DE44 (0x6DE44..0x6DF4A); rows 03/04/07 share it (FU-192 erratum: the brief's family list omitted this body). The DX>=0 arm scales the [rec+0x14] dword pair (W+(W>>2) / D+(D>>4), side-flipped); the DX<0 cell-map arm (0x6D870/0x15774C) is FU192-L3. Gated on the unported [rec+0x14] (FU192-L1). Off-tape (row 03): tape-neutral"},
    {0x04, fifa96_match_phase_04,
     "FU-192 §2.4 (phase-17 T3): phase 04 = the native 0x6DE44 (same as row 03). ON-TAPE (303 frames, R10): COUNTED after the cmp pin; the body is dispatched-but-gated on the unported [rec+0x14] (FU192-L1), so the walk is a no-op on the tape (the FU-177 §7 precedent)"},
    {0x05, fifa96_match_phase_05,
     "FU-192 §2.7 (phase-17 T3): phase 05 = the ported native 0x6E05C (0x6E05C..0x6E1C5). The derivable arm is the 0x6E1B3 `target := rec position` copy; the ball-relative arm ([0x157A9F] ball-team compare, 0x8DC68 metric, 0x114E04 fold, 0x157AC5/0x15881A/0x157AB6) is FU192-L1 (the ball record's [+0] team pointer is unmodelled). Off-tape: tape-neutral"},
    {0x06, fifa96_match_phase_06,
     "FU-192 §2.3 (phase-17 T3): phase 06 = the ported native 0x6DD9C (0x6DD9C..0x6DE31): scales the [rec+0x10] dword pair by ×1.25/×1.0625 (W+(W>>2) / D+(D>>4)) with the [0x157754]-sign index and the ±0x60 z bias, side-flipped. Gated on the unported [rec+0x10] (FU192-L1); the [0x157754] runtime cell is staged 0. Off-tape: tape-neutral"},
    {0x07, fifa96_match_phase_04,
     "FU-192 §2.4 (phase-17 T3): phase 07 = the native 0x6DE44 (same as rows 03/04); FU-177's hot path. Off-tape (row 07): tape-neutral"},
    {0x08, fifa96_match_phase_08,
     "FU-192 §2.2 (phase-17 T3): phase 08 = the ported native 0x6DD6C (0x6DD6C..0x6DD98), the thin wrapper over 0x6DCC8 selecting idx 0xBC/0x6F by the [0x157AAC]>>24 controlled side. Rows 08/09 share it. Off-tape: tape-neutral"},
    {0x09, fifa96_match_phase_08,
     "FU-192 §2.2 (phase-17 T3): phase 09 = the native 0x6DD6C (same as row 08). Off-tape: tape-neutral"},
    {0x0A, fifa96_match_phase_00,
     "FU-192 §2.1 (phase-17 T3): phase 0A = the shared reset 0x6DE34 (same body as row 00). Off-tape: tape-neutral"},
    {0x0B, NULL,
     "FU-192 §1 (phase-17 T3): phase 0B = the shared reset 0x6DE34, but phase 0x0B is never written by an engine source (the 9 match_run_write_phase call sites, FU-191 §10), so wiring it would be an un-dispatched/fake count; the slot stays NULL; the rows-27/29/2C precedent; OL-13"},
    {0x0C, fifa96_match_phase_0C,
     "FU-192 §2.5 (phase-17 T3): phase 0C = the ported native 0x6DF4C (0x6DF4C..0x6E001), the stateful restart-line timer ([rec+0x89] += [0x157A64] every call, then the 0x780/0xCC0 target writes past the 0x3C gate with the |word[rec+0x69]| < 0x20 refinement). The [rec+4]/0x157A38 table, the [rec+0x8A] angle term and the 0x8DCD4 metric are FU192-L1. Off-tape: tape-neutral"},
    {0x0D, fifa96_match_phase_00,
     "FU-192 §2.1 (phase-17 T3): phase 0D = the shared reset 0x6DE34 (same body as row 00). Off-tape: tape-neutral"},
    {0x0E, NULL,
     "FU-192 §1 (phase-17 T3): phase 0E = the shared reset 0x6DE34, but phase 0x0E is never written by an engine source (FU-191 §10); the slot stays NULL to avoid an un-dispatched/fake count; OL-13"},
    {0x0F, fifa96_match_phase_00,
     "FU-192 §2.1 (phase-17 T3): phase 0F = the shared reset 0x6DE34 (same body as row 00). Off-tape: tape-neutral"},
    {0x10, NULL,
     "FU-192 §2.6 (phase-17 T3): phase 10 = the native 0x6E004 (the 0x1105EA variant table) is PORTED (fifa96_match_phase_10) but NOT counted: phase 0x10 is never written by an engine source (the 9 match_run_write_phase call sites, FU-191 §10), so a wired row would never dispatch (the rows-27/29/2C precedent). The body is exported for direct unit testing; OL-13"},
    {0x11, NULL,
     "FU-192 §2.8/L6 (phase-17 T3): phase 11 = the native 0x6E1C8 (8-byte shim: EBX=-1, EDX=&rec+0x4D, falls into 0x6E1D0); under the walk ABI it is identical to rows 01/12 (the formation placement), so it is not wired (the R10 'do not double-wire'; FU192-L6/L7); OL-13"},
    {0x12, NULL,
     "FU-192 §2.9 (phase-17 T3): phase 12 = the native 0x6E1D0 (formation placement; see row 01) and phase 0x12 is never written by an engine source (FU-191 §10); not wired (FU192-L6/L7); OL-13"},
    {0x13, NULL,
     "FU-192 §2.10/L2 (phase-17 T3): phase 13 = the native 0x6E244 (0x6E244..0x6E32A), the camera-scatter body. LEGGED (R10): it consumes the PRNG 0x92AC8 twice per active record and reads 0x10F364/0x10F368, whose producer is the unported FUN_0008D098 case-0x13/0x14 scatter; wiring it is not proven tape-inert (owner FU192-L2). The slot stays NULL/uncounted; OL-13"},
    {0x14, NULL,
     "FU-192 §2.10/L2 (phase-17 T3): phase 14 = the native 0x6E244 (same as row 13). LEGGED (R10, on-tape 40 frames): the RNG consumption + the unported 0x10F364/0x10F368 producer (FU192-L2). The slot stays NULL/uncounted; OL-13"},
    {0x15, fifa96_match_phase_02,
     "FU-192 §2.11/R1 (phase-17 T3): phase 15 shares the T1 body 0x6DCC8 (0x110794[0x15] = 0x6DCC8). Wired by T3 after the writer-path cmp pin (T1 deliberately deferred it, ENGINE.md phase-17 T1): ON-TAPE (100 frames); dispatched-but-gated on the unported [rec+0xC] (FU192-L1), so the walk is a no-op on the tape"},
    {FIFA96_MATCH_PHASE_INT3_SLOT, NULL,
     "FU-137 §1.2: native 0x110794[0x16]=0 loader INT3 stub; no body by design; NOT_FOUND"},
    {0x17, NULL,
     "FU-193 §2/§5/§6.1 (phase-17 T4): phase 17 = the act-1 kickoff/phase-1 entry (native 0x88DC8) is PORTED (fifa96_match_action_phase_17) but NOT counted (Ruling R8): its sole producer 0x8ABCF (the sit-1 arm) has no live engine driver (FU193-L1) and the live act-1 behaviour is the ad-hoc fifa96_match_run_kickoff_entry + the frame-body global_5882a arm, so wiring the row would double-run the kickoff (FU193-L2). The rows-27/29/2C precedent (ported body, no live producer); the body is exported for direct unit testing; the head guard (goal-log [0x157AC5]/[0x157AC7] + [0x157AC2]) and the presentation sinks are FU193-L7; OL-13"},
    {0x18, fifa96_match_action_phase_18,
     "FU-183 §5/§6 rank 2 (phase-16 T1): phase 18 = the act-2 free-kick/penalty hand-off (0x8922C) as the derived fifa96_match_action_phase_18 over the ported match_run_ref_step_restart; the producer is live (sit 9 via the referee contact chain / sit 0xA via the foul sequence, fifa96_match_run_set_piece -> match_run_situation_side), and the act machine owns the row (dispatch act 2 invoke-now + the clock-tail tick; the retired ref_machine route, FU175-L7); the FU-83 timeline remainder is OL-13"},
    {0x19, fifa96_match_action_phase_19,
     "FU-187 §2/§5 rank 7 (phase-16 T4): phase 19 = the act-3 foul sequence (fifa96_match_action_phase_19 over fifa96_ref_foul_sequence_step + match_run_ref_apply); the producer is LIVE (the row-0x0C contact -> fifa96_ref_foul_decide ACT3 -> fifa96_match_run_contact dispatches act 3 invoke-now; the clock tail ticks it), and the act machine owns the row (the former ref_machine FOUL route retired, FU175-L7). Stage 0 installs action 0x16 + phase 0xF; the FU187-L1 `[0x158866]` polarity, the FU187-L5 stage-1/3 gates and the FU187-L3 stage/timer aliasing are the legs"},
    {0x1A, NULL,
     "FU-193 §3/§5/§6.2 + FU-197 §6/§9.4 (phase-17 T4): phase 1A = the act-4 formation-order/line-up walk (native 0x89620) is PORTED (fifa96_match_action_phase_1A) but NOT counted (Ruling R8): the engine has no act-4 producer — the T2 sit-2..4 arm is gated dormant on the unported desired-order matrix (formation_order_ok stays 0, FU197-L4) and the act-1 stage-1 hand-off is itself unwired (FU193-L3). The rows-27/29/2C precedent (ported body, no live producer); the body is exported for direct unit testing; the walk is gated on formation_order_ok; the FUN_0007D9A4 invoke-now (FU197-L6) and the FUN_0008C974 matrices (FU197-L4) are legs; OL-13"},
    {0x1B, NULL, "FU-137 §6: FU-136 phase 1B: not ported; FU-83 pure reset; OL-13"},
    {0x1C, NULL,
     "FU-193 §4/§5/§6.3 (phase-17 T4): phase 1C = the act-6 pre-restart timeline (native 0x89110) is PORTED (fifa96_match_action_phase_1C) but NOT counted (Ruling R8): no engine source dispatches act 6 (FU193-L4) and the existing driver is the offside machine (fifa96_match_run_offside_reception -> fifa96_ref_offside_sequence_step), so the row would double-run it. The native handler is already ported as fifa96_ref_offside_sequence_step (FU-150/FU-165); the phase-0x1C row body wraps it (the phase-0x19 alias pattern); the rows-27/29/2C precedent (ported body, no live producer); the body is exported for direct unit testing; OL-13"},
    {0x1D, NULL, "FU-189 §3 + FU-137 §6 (phase-16 T5): phase 1D = the act-7 handler 0x89868 (fresh get_xrefs_to=1, the phase-table slot 0x110808). Its arm 0 installs rows 0x20 (0x899a2) and 0x1f (0x899cd) — the [0x1D] installs this slice found (the FU-137 note omitted them) — but the phase-1D / act-7 producer (situation 8) is not dispatched by any engine source, so the slot stays NULL and row 0x20's installer never fires (leg FU189-L1). FU-83 timeline, the arm-0 substage counter [0x158829], the 0/0x1f installs and the OL-13 phase-cell remainder are unported; OL-13"},
    {0x1E, NULL, "FU-137 §6: FU-136 phase 1E: not ported; FU-83 timeline; OL-13"},
    {0x1F, fifa96_match_action_phase_1F,
     "FU-175 §7 rank 1 / FU-181 §4 (phase-15 T1): phase 1F = the act-9 handler 0x88F4C as the derived 4-stage skeleton (stage-0 [0x15882A] clear and stage-3 timer>=0x3C -> [0x15882A]=1 + the act-slot clear); the 0x88F9A phase-0x15 write is the sit-0xC arm (FU-170 §7); the 0x73E28/0x93850/0x361B0/0x4C324 sinks and the word[0x15881A] gate producer stay FU181-L5/FU175-L5"},
    {0x20, NULL,
     "FU-188 §2/§6 (phase-16 T4): phase 20 = the act-0xA code-0x24 timeline ported (fifa96_match_action_phase_20, 0x8B688) but NOT counted: its only producer is situation 0 (0x8AB8E), which no engine source/test dispatches (FU188-L1; the direct-set situations are 3/5/6/7/9/0xB/0xC/0x0B). The rows-27/29/2C precedent (ported body, no live installer); the body is exported for direct unit testing (fifa96_match_run.h); the timeline cells/`word[0x15881A]` gate (FU188-L2) and the invoke-now install (FU188-L4) stay legs; OL-13"},
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

int fifa96_match_dispatch_phase_record(struct fifa96_match_run *mr, uint8_t phase,
                                       struct fifa96_match_entity *rec, int32_t edge) {
  const struct fifa96_match_handler *row;
  struct fifa96_match_entity *saved_rec;
  int32_t saved_edge;
  uint8_t saved_active;
  int rc;
  if (mr == NULL || rec == NULL) return -FIFA96_ERR_INVALID;
  if (phase >= FIFA96_MATCH_PHASE_ROWS) return -FIFA96_ERR_NOT_FOUND;
  if (phase == FIFA96_MATCH_PHASE_INT3_SLOT) return -FIFA96_ERR_NOT_FOUND;
  row = &fifa96_match_phase_table[phase];
  if (row->fn == NULL) return -FIFA96_ERR_UNSUPPORTED;
  /* Stage the native walk cursor (EAX=rec / EBX=edge) for the per-record body,
   * saving/restoring any enclosing cursor so a nested row-03 [rec+0x1C] call is
   * safe. */
  saved_rec = mr->phase_walk_rec;
  saved_edge = mr->phase_walk_edge;
  saved_active = mr->phase_walk_active;
  mr->phase_walk_rec = rec;
  mr->phase_walk_edge = edge;
  mr->phase_walk_active = 1u;
  rc = row->fn(mr);
  mr->phase_walk_rec = saved_rec;
  mr->phase_walk_edge = saved_edge;
  mr->phase_walk_active = saved_active;
  return rc;
}
