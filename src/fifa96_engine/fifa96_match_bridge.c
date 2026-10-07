#include "fifa96_engine/fifa96_match_bridge.h"
#include "fifa96_engine/fifa96_engine_internal.h"

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
  return 0;
}
