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
    /* 0x858F8..0x85966: the 0x18 event and the 0x10F334/0x10F33C actor-type
     * ball-line build, then the FUN_000700F4 reset at the line (the derived
     * subset applies the type offsets to the record position, the tables are
     * the ported `match_kick_dir_x/z`). */
    {
      /* The native indexes the 0x10F334/0x10F33C byte tables with the raw
       * actor type (unbounded image read); the port masks to the 32-entry
       * derived tables (hardening divergence). */
      uint32_t dir_idx = (uint32_t)r->actor_type & 0x1Fu;
      int32_t dx = (int32_t)match_kick_dir_x[dir_idx] << 6;
      int32_t dz = (int32_t)match_kick_dir_z[dir_idx] << 6;
      (void)fifa96_camera_init(&mr->render.camera, r->pos_x + dx, 0,
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
  s.is_ball_track = id == controlled ? 1u : 0u;               /* [0x1577CA] stand-in */
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
  c.ball_height = mr->entities.ball.y;   /* dword [0x157750] (OL-65) */
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
  s.byte44 = 0;                     /* native +0x44 producer unported (OL-82) */
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
    {0x01, fifa96_match_action_01,
     "FU-143 §11 (M2 playable-match Task 2/OL-84): row 01 ported (0x7DBC0..0x7DFC8, phase==1 gate, stage 0/1/2 walk) — situation 0xB -> phase 2; camera/event/ball-stage sinks + rows 02/0x10..0x13 legs OL-84"},
    {0x02, fifa96_match_action_02,
     "FU-75 L4.6/FU-138 OL-18 (M2 phase-9 T1): row 02 ported (0x7DFCC..0x7E1A2, first-hand): the phase-1 restart mirror (locomotion_restart_target), the phase-2 [team+0x7B2] bind + merge request + install-4 invoke-now over the pool, the no-slot camera-target/timer + the restart-wait stage 0; the stage-1/2 arms (nearest/metric/0x92820/0x7A490/0x79B1C) stay OL-18"},
    {0x03, NULL,
     "FU-137 §6: FU-136 row 03: not ported (partial); hold/clamp + FU-138 counter/phase1_clamp; OL-19"},
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
    {0x09, NULL, "FU-137 §6: FU-136 row 09: not ported (partial); FU-81 arm table 0x809F0; OL-9"},
    {0x0A, NULL, "FU-137 §6: FU-136 row 0A: not ported; installer 0x7CDD8 has no xrefs; OL-14"},
    {0x0B, NULL, "FU-137 §6: FU-136 row 0B: not ported (partial); sequence_duel_event; OL-9"},
    {0x0C, NULL, "FU-137 §6: FU-136 row 0C: not ported (partial); FU-81 7-arm table 0x81C74; OL-9"},
    {0x0D, NULL,
     "FU-137 §6: FU-136 row 0D: not ported (partial); FU-82 4-arm 0x8250C + FU-138 velocity_scale; OL-22"},
    {0x0E, NULL, "FU-137 §6: FU-136 row 0E: not ported; FU-81 gate/head, no body port; OL-9"},
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
    {0x1D, fifa96_match_action_1D,
     "FU-151 §Port contract item 2/FU-79 §6/§2.5: row 1D ported (fifa96_keeper_closedown_step, 0x74EB0..0x754E1, stages 0..4) over the pool; place/commit/helper/reset/situation + the clearance staging (events 0x30/0x31) bound; 0x157C5E table, 0x744D4/0x74CDC/0x79B1C/0x7B878/0x8F188/0x7A490/0x8DE8C+0x786A0 and the UI/audio calls FU-151 legs"},
    {0x1E, fifa96_match_action_1E,
     "FU-151 §Port contract item 1/FU-147 §8: row 1E ported as the full 10-stage machine (fifa96_keeper_claim_step, 0x7550C..0x7612F, table 0x754E4): claim/hold/outlet/release/restart + the 0x157C3x/42 process cells; unported sinks are request bits (FU-151 legs 3/4/12/13/15)"},
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
