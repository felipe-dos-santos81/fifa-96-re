#pragma once
#include "fifa96_loader/fifa96_err.h"

struct fifa96_engine;
struct fifa96_match_run;

/* M2 bridge: hand the engine's front-end off to a match run.
 *
 * Call after fifa96_frontend_run_step reports the match-start exit
 * classification (FU-66 §4/§5, stand-in per C1-OL3): the panel's accepted
 * confirm drives the FU-66 §2 driver exit, whose port classification is the
 * code-8 FIFA96_FRONTEND_EXIT_STATE16 (the exit tail at 0x1EDFD dispatches
 * state 16, and the FU-64 §1.1 menu-driven start-match family enters the
 * match setup with selector 0). Begins `mr` with that selector and sets the
 * engine mode MATCH through fifa96_match_run_begin. Returns 0,
 * -FIFA96_ERR_INVALID (NULL arguments), or -FIFA96_ERR_STATE when the engine
 * is unbooted/quitting, the run is already live, or the front-end is not in
 * the startable state. Those refusal paths mutate nothing; a failure returned
 * by fifa96_match_run_begin itself (e.g. the engine's default backend finding
 * the 100 Hz tick table full) follows begin's contract and may leave the run
 * reset. The startable classification is consumed only on success, so a live
 * match cannot re-enter.
 *
 * G1: after a successful begin the bridge stages the derived default match art
 * pair (/ART/PLAYART.PVI player banks + /ART/GAMEART0.PVI match art, FU-86
 * §1/§2; the pitch-bank identity stays an open leg) against the engine
 * surface, so a live match renders. A staging failure is not fatal: the match
 * starts with rendering disabled and the failure is not propagated. */
int fifa96_match_bridge_from_frontend(struct fifa96_match_run *mr,
                                      struct fifa96_engine *eng);
