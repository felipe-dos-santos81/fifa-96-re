# FIFA 96 — match-depth playable wave (P14, 2026-11-07)

**Status:** proposed; derived from the P13 functional completion. This is the
next tier of *playable match* depth, not another menu wave.

## Goal

A played cup/league match behaves like a contest rather than a
human-vs-chaser drill: the CPU can keep, pass and shoot; defenders can win the
ball; fouls/offside produce real free kicks; the player can substitute; and a
knockout draw is decided by extra time/penalties the player can influence.
Native camera/stadium work is a bounded presentation tail, not the definition
of playable.

## Phase A — CPU possession and keeper distribution

**Status: implemented this wave.** Files: `fifa96_match_run.{c,h}`,
`test_engine_live_control.c`. The opponent carrier now passes or shoots after
a bounded hold through the existing live flight; an opponent outfield record
can win a stationary human ball with a cooldown; the keeper releases a save to
a teammate through the same primitive. All derived [D] and live-only; tests
cover all three; M1/M2 exact.

## Phase B — live offside, fouls, free kicks/penalties

**Status: deferred except the existing option path.** The foul audit found
that a live tackle contact is kind 1 and the ported act-2 stage 2 holds at
phase 0x0A because the native release writer is missing; enabling contact
fouls without a new live restart would soft-lock. Offside is ported behind
`field_4c2f2` and reachable only through the receive arm; its real live-pass
test/metric producers remain legs. Do not fake a native contact foul. Keep the
Options Fouls/Offside values wired; record the missing release producer as the
next referee slice.

## Phase C — substitutions

**Status: derived pause path, scoped.** Add a pause substitutions action with
the PCCD roster names (16 slots), a max-three counter, and the existing R1
strip/overlay. Start with a deterministic auto-pick (S) rather than the full
native selection menu; the native `FUN_00041494` case-5 selection/timing
remains a leg. If the roster API or pause input cannot land safely, defer the
whole phase rather than fake player identities.

## Phase D — interactive penalty shootout

Files: new `fifa96_shootout.{h,c}` (pure), `fifa96_frontend_run.{c,h}`,
`fifa96_menu_model.{h,c}`, `fifa96_menu_art.c`, `test_engine_competition.c`
(or a new test target), `test_engine_frontend_modes.c`.
- Replace the silent cup-draw hash with a player-facing five-round kick/dive
  screen; the CPU is deterministic from the seed; sudden death until a winner.
- On completion, apply the winner to the cup fixture and advance; do not add
  ET or a save-format version in this wave.
- Acceptance: pure model tests; front-end flow reaches the screen and returns
  to the hub with the correct bracket winner; existing cup/league tests still
  pass; M1/M2 exact.

## Phase E — presentation tail (deferred)

Native type-0 live aim and the 35-quad/5x5 3dwpts bowl are separate bounded
slices. Do not mix them into the functional match/shootout wave.

## Phase F — gate

- `make check` green; test count updated if a new shootout target is added.
- M1/M2 exact by `cmp`; release smoke of CPU action, tackle, keeper outlet,
  shootout; docs/ENGINE/PROGRESS/screenshots; commit on `main`.

## Non-goals / legs

Full native 10-player AI, live contact fouls/free kicks (missing release
writer), native substitution selection/timing, native extra time/penalty
presentation, native 3dwpts full renderer, camera director, native competition
art/data/save/modem remain labelled legs.


## Completion record (2026-11-07)

- CPU: bounded opponent pass/shot, contact tackle with cooldown, keeper
  post-save outlet; tests in `test_engine_live_control`.
- Substitutions: derived pause S path, max three, PCCD names/numbers through
  `fifa96_team_data_player`/`_roster`, R1 strip fields and pause notice.
- Shootout: pure `fifa96_shootout` model + front-end screen; cup draws resolve
  through it and the bracket receives the winner; no ET/save-version change.
- Referee contact fouls and live offside proof were deferred rather than
  faked; the missing live release producer is recorded in ENGINE Phase 68.
- Gate: `make check` 124/124; M1/M2 exact; release smoke of pause substitution
  with real PCCD names.
