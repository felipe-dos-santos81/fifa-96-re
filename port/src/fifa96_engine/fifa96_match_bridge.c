#include <stdio.h>
#include <string.h>
#include "fifa96_engine/fifa96_match_bridge.h"
#include "fifa96_engine/fifa96_engine_internal.h"

/* G1 entry staging: the derived default match art pair from Task 2's FU-86
 * evidence — /ART/PLAYART.PVI is the player animation container (91 banks,
 * resource slot 0x47) and /ART/GAMEART0.PVI the match art container (slots
 * 0..0x3E). Which GAMEART0 banks serve the pitch stage is Task 2's recorded
 * open leg (report leg 1), so only the FU-86-quoted container pair is named
 * here; the staging interface keeps the pitch identity caller-supplied. */
#define FIFA96_MATCH_BRIDGE_PLAYER_BANK "/ART/PLAYART.PVI"
#define FIFA96_MATCH_BRIDGE_PITCH_BANK "/ART/GAMEART0.PVI"

int fifa96_match_bridge_from_frontend(struct fifa96_match_run *mr,
                                      struct fifa96_engine *eng) {
  if (!mr || !eng) return -FIFA96_ERR_INVALID;
  if (!eng->booted || eng->mode == FIFA96_ENGINE_MODE_QUIT) return -FIFA96_ERR_STATE;
  if (mr->running) return -FIFA96_ERR_STATE;
  if (!eng->frontend.match_start) return -FIFA96_ERR_STATE;
  /* The startable exit classification (FU-66 §4 STATE16) maps to the FU-64
   * §1.1 menu-driven start-match path: FUN_00018014 enters the match setup
   * with selector 0 (`XOR EAX,EAX` at 0x180A8). */
  uint32_t selector = 0;
  /* FU-343 (P1): only the windowed host opts into the live-start bootstrap
   * (skip the native pre-match situation-0/act-0xA collapse, enter phase 2,
   * seed the derived camera event/tracked, render the camera-0 eye and the
   * pitch). Headless runs (the M1/M2 tapes and every direct-begin fixture)
   * take the native begin order unchanged. */
  const int live = eng->surface != NULL && !eng->cfg.headless;
  int rc = fifa96_match_run_set_live_start(mr, live);
  if (rc != 0) return rc;
  eng->match_esc_held = 0;   /* FU-343 P4: fresh pause edge latch */
  /* P6: stage the Team Select handout before begin. begin explicitly resets
   * formation[] and seeds the kickoff placement, so the real per-side ids must
   * already be in the run; the post-begin set_formation below stays the live
   * id setter. Kit triples ride the same handout for the live-only kit stage. */
  if (eng->frontend.team_selected) {
    struct fifa96_match_matchup matchup;
    memset(&matchup, 0, sizeof matchup);
    for (uint32_t side = 0; side < 2; side++) {
      matchup.formation[side] = eng->frontend.formation[side];
      memcpy(matchup.kit[side], eng->frontend.kit[side], sizeof matchup.kit[side]);
    }
    matchup.kit_ready = (uint8_t)(eng->frontend.pccd_ready[0] &&
                                  eng->frontend.pccd_ready[1]);
    rc = fifa96_match_run_set_matchup(mr, &matchup);
    if (rc != 0) return rc;
  }
  rc = fifa96_match_run_begin(mr, eng, selector);
  if (rc != 0) return rc;
  rc = fifa96_match_run_set_mode(mr, eng->frontend.mode);
  if (rc != 0) return rc;
  /* P14 substitution roster: provisional labels until the PCCD player-name
   * handoff fills them. The native match still uses the same 11 record slots;
   * the menu is a derived bench aid. */
  for (uint32_t side = 0; side < FIFA96_MATCH_ENTITY_TEAMS; side++) {
    for (uint32_t slot = 0; slot < FIFA96_PCCD_ROSTER_COUNT; slot++) {
      if (mr->roster_name[side][slot][0] == '\0')
        (void)snprintf(mr->roster_name[side][slot],
                       sizeof mr->roster_name[side][slot], "PLAYER %u",
                       (unsigned)(slot + 1u));
      if (mr->roster_number[side][slot] == 0u)
        mr->roster_number[side][slot] = (uint8_t)(slot + 1u);
    }
  }
  if (eng->team_data.ready) {
    for (uint32_t side = 0; side < FIFA96_MATCH_ENTITY_TEAMS; side++) {
      char names[FIFA96_PCCD_ROSTER_COUNT][24];
      uint8_t numbers[FIFA96_PCCD_ROSTER_COUNT];
      int count = 0;
      if (fifa96_team_data_roster(&eng->team_data, eng->frontend.team_id[side],
                                  names, numbers, &count) == 0) {
        for (int slot = 0; slot < count && slot < (int)FIFA96_PCCD_ROSTER_COUNT;
             slot++) {
          (void)snprintf(mr->roster_name[side][slot],
                         sizeof mr->roster_name[side][slot], "%.23s",
                         names[slot]);
          mr->roster_number[side][slot] = numbers[slot];
        }
      }
    }
  }
  eng->match_halftime_steps = 0;
  eng->match_halftime_pressed = 0;
  eng->frontend.match_start = 0;   /* consumed: no instant re-entry */
  /* P2 handoff (A3 §3.2): when the native Team Select screen started the
   * match, stage the chosen formations and the derived 3-letter HUD names.
   * The native producers are FUN_0008EA70 (the formation id writer) and the
   * team-block name triple (FUN_00011BEC, P2-L4: the roster id is carried but
   * has no consumer yet). Both run after begin, when the run is live. */
  if (eng->frontend.team_selected) {
    /* P6: apply the Options block at match start (FUN_0003749C's shape).
     * begin installed the FU-68 default handoff above, so this replaces it
     * with the user's cells and the configured half length. Gated on
     * `team_selected`; the M1/M2 tapes and every direct-begin fixture never
     * run it and keep begin's 120/40 defaults. With defaults the copy and
     * set_period are a no-op (same cells, same lengths). */
    struct fifa96_match_config config;
    int config_rc = fifa96_settings_handoff(&eng->frontend.menu.settings, 0u, &config);
    if (config_rc != 0) return config_rc;
    rc = fifa96_match_run_set_config(mr, &config);
    if (rc != 0) return rc;
    for (uint32_t side = 0; side < 2; side++) {
      (void)fifa96_match_run_set_formation(mr, side, eng->frontend.formation[side]);
      memset(mr->render.hud_name[side], 0, sizeof mr->render.hud_name[side]);
      size_t len = strlen(eng->frontend.team_code[side]);
      if (len > sizeof mr->render.hud_name[side] - 1)
        len = sizeof mr->render.hud_name[side] - 1;
      if (len != 0) memcpy(mr->render.hud_name[side], eng->frontend.team_code[side], len);
    }
  }
  /* Stage the derived default pair so a live match renders (G1 Finding 2:
   * make game kept render.enabled == 0). A staging failure is not fatal — no
   * mounted ISO, an absent bank or an undecodable container leaves the run
   * untouched with rendering disabled (the M1 boot behavior) and the match
   * still starts; the engine layer has no logger yet, so the soft failure is
   * observable only as render.enabled == 0. */
  (void)fifa96_match_run_stage(mr, eng->surface, FIFA96_MATCH_BRIDGE_PLAYER_BANK,
                               FIFA96_MATCH_BRIDGE_PITCH_BANK);
  /* FU-343 (P1): the pitch/kit stage is live-only (it edits the kit palette and
   * the per-team remap); a failure leaves the sprite-only render in place. */
  if (live) (void)fifa96_match_run_stage_pitch(mr, eng);
  return 0;
}
