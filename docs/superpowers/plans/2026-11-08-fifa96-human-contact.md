# FIFA 96 — deliberate human contact and stable human side (Phase 77)

Goal: let the human commit an adjudicated challenge through the ported
referee chain, and keep the human on their own team when a foul flips the
native restart-side cell, without re-routing the ordinary derived steal or
changing M1/M2.

## Evidence

- `fifa96_match_run_contact` is the ported contact registrar + row-0x0C
  decision seam; the live AI tackle already calls it.
- The derived `match_run_live_control_tackle` proximity steal writes only the
  ball possession cell and never registers contact.
- The native pad duel route (`0x7CE38` -> row `0x0B`) is unreachable for the
  live controlled record because it dispatches as code 3.
- `match_run_ref_step_restart` sets `side_controlled` to the fouled side
  (`rec_first_side ^ 1`), so a human foul would otherwise switch control to the
  AI team.

## Steps

1. Add `live_human_side`, captured at live begin, and use it for live control
   bind/team, restart bind, opponent/CPU drivers and pause substitution side;
   keep native `side_controlled` for referee/set-piece arms.
2. Add `FIFA96_ENGINE_KEY_TACKLE` (T / pad RB), excluded from FU-61, with a
   physical edge latch and live/paused/phase-2 acceptance.
3. Add `match_run_live_human_tackle`: opponent carrier within `0x40`, hidden
   checks, contact registrar with human as rec_a, clean steal when no decision
   runs, 45-frame cooldown, and no normal bind when a decision owns the phase.
4. Tests: Fouls Off clean steal; Fouls On decision fields/stoppage; stored human
   side after a native restart-side flip; T not FU-61; SDL T/RB map and held
   sample.
5. Run `make check` and M1/M2 `cmp`; keep the census unchanged.

## Result

All five steps landed. Fouls Off returns a clean human steal; Fouls On
registers the human foul and stoppage; the stored human side wins after the
native `side_controlled` flip. The automatic proximity steal remains a clean
derived claim. `make check` 124/124; M1/M2 exact by `cmp`; census 78/78 bodies,
61/80 natural, 64/80 install-live.

## Non-goals

- No native auto-switch/AI caller or pad-duel dispatch-code port (P7-L1/L2).
- No free-kick/penalty player interaction; the taker rows remain their current
  auto/derived flow.
- No card display; the proven image has no card state/asset.
- No M1/M2 re-pin.
