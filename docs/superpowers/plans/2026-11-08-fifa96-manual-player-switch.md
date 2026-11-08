# FIFA 96 — derived manual player switch (Phase 76)

Goal: give the live player a keyboard/gamepad switch key that cycles the
controlled outfield record, without pretending to port the native FU-295
auto-switch/AI caller and without changing M1/M2.

## Evidence

- `match_run_live_control_update` always picks the own ball carrier or nearest
  own outfield; the native nearest-select `fifa96_match_run_select_step`
  (`0x7D1D4`) exists but is overwritten by the live follow every frame.
- The native switch handler is a nearest-select plus `0x786A0` swap, not a
  player-facing cycle; it remains a separate leg.
- `Q`/pad LB have no match binding; SDL re-presents held keys, so a new edge
  latch is required.

## Steps

1. Add `FIFA96_ENGINE_KEY_SWITCH` (12), map SDL `Q` and `LEFT_SHOULDER`, extend
   the held-key sample table, and keep the key out of `match_run_map_key`.
2. Add `live_manual_id`/`live_manual_frames`/`live_switch_down`/
   `live_switch_press`; edge-latch the physical level only in live, unpaused,
   ACTIVE phase 2 outside the opening kick.
3. Add a cycle helper over records 1..10 that skips the keeper and
   `+0x98`/`+0x9A` excluded records and starts after the current controlled,
   auto or team-target record.
4. Apply the override in the control follow before the existing bind; hold for
   90 granted frames; yield to a near own carrier; clear on init/begin,
   restart/phase changes, out-of-play, bind failure and expiry.
5. Tests: cycle/held/skip/expiry, carrier priority, non-FU61 input separation,
   SDL Q/LB mapping and held sample.
6. Run `make check` and M1/M2 `cmp`; keep the census unchanged.

## Result

All six steps landed. The switch is live-only and derived `[D]`; no native
selector is claimed. `make check` 124/124; M1/M2 exact by `cmp`; census 78/78
bodies, 61/80 natural, 64/80 install-live.

## Non-goals

- No FU-295 auto-switch/AI port, no native `0x7D1D4`/`0x786A0` body.
- No camera-director or AI policy change.
- No M1/M2 re-pin.
- The 90-frame hold and cycle order are derived usability choices.
