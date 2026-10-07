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
  int rc = fifa96_match_run_begin(mr, eng, selector);
  if (rc != 0) return rc;
  eng->frontend.match_start = 0;   /* consumed: no instant re-entry */
  /* Stage the derived default pair so a live match renders (G1 Finding 2:
   * make game kept render.enabled == 0). A staging failure is not fatal — no
   * mounted ISO, an absent bank or an undecodable container leaves the run
   * untouched with rendering disabled (the M1 boot behavior) and the match
   * still starts; the engine layer has no logger yet, so the soft failure is
   * observable only as render.enabled == 0. */
  (void)fifa96_match_run_stage(mr, eng->surface, FIFA96_MATCH_BRIDGE_PLAYER_BANK,
                               FIFA96_MATCH_BRIDGE_PITCH_BANK);
  return 0;
}
