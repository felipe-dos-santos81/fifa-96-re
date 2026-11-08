# FIFA 96 — player-controlled free kicks and penalties (Phase 78)

Goal: when the human side is awarded a free kick or penalty, bind the native
taker, hold its auto delivery, let the player aim/power/confirm, and launch a
derived live shot; keep the native auto row as the timeout fallback and do not
change M1/M2.

## Evidence

- `match_run_ref_step_restart` selects phase 7 (free kick) / 6 (penalty) and
  `match_run_phase_arm` installs taker code `0x12` / `0x13` on the controlled
  (fouled) team.
- `fifa96_match_action_12` stage 4 and `fifa96_match_action_13` stage 4 are
  the native kick points. The port currently auto-fires them.
- Fresh Ghidra at `0x84E44` shows the row-0x13 kick call with `EDX=0x40`,
  `EBX=0`, `EAX=record`; the record slot `[EBP+0x20]` is inspected/restored
  immediately before. The native `0x4C114/18/1C` input words are written at
  `0x84CCC..0x84D0C` and fed to the shipped no-op `0x4C31C`; they are not a
  proven aim vector.
- The live flight primitive already gives the ball machine a target and lets
  the existing goal armer/keeper save/out-of-play chain decide.

## Steps

1. [done] Add live set-piece state (`active`, `kind`, `taker`, `aim_x`,
   `power`, `frames`, input sample), reset it on init/begin/phase exit, and
   bind the native taker when phase 6/7 and the human side owns the taker.
   Evidence: live FK/penalty activation assertions pass.
2. [done] Hold rows 0x12/0x13 at their stage-4 kick until the player confirms
   or the timeout auto-delivers the derived strike. Evidence: live FK/penalty
   tests remain in phase 7/6 with the taker code and a staged-4 hold; the
   timeout test covers both phases.
3. [done] On confirm, reset the taker row, write the derived live flight target
   (aim x, opponent goal z) and return through situation 0xB to phase 2.
   Evidence: live tests see phase 2, `live_kick_active`, shot pending, target,
   and ball movement.
4. [done] Draw the live-only FREE KICK / PENALTY prompt with aim and power;
   document controls. Evidence: render plate/gate test passes.
5. [done] Run targeted tests, `make check` (124/124), M1/M2 `cmp` (exact),
   release smoke; update ENGINE/PROGRESS/README and commit on `main`.

## Result

All steps landed. The live phase-7/6 owner binds the installed taker, holds row
0x12/0x13 at stage 4, maps Left/Right to aim and Up/Down to power, and confirms
with KICK/CONFIRM. Confirm resets the taker, returns through situation `0xB`
to phase 2 and launches the existing live flight at the side-correct `0xB40`
goal. The 300-frame window auto-delivers the same derived strike (the native
row-0x13 ack wait is unported and would otherwise hold phase 6); an opponent
taker keeps the native auto row. The live-only prompt is a 250x58 screen-space
plate.
`make check` 124/124; M1/M2 exact by `cmp`; census 78/78 bodies, 61/80 natural,
64/80 install-live.

## Non-goals

- No claim that the native `0x4C114/18/1C` aim words are ported.
- No native slot `word[+6]` 0x20-edge claim; the derived confirm is explicit.
- No change to the native auto path when the human is not the taker.
- No card display (the image has no card state/asset).
