# P9 — contestable shots: aim, live keeper save, held long ball (2026-11-06)

**Status:** implemented and verified; this spec/plan is the retrospective SDD
record added after the P9 code review (the implementation ran before the
process files existed, recorded as a review finding and corrected here).

## 1. Goal

The Friendly match is already playable end to end (menu, options, teams,
kickoff, goal/restart, half-time, full time, result, menu). P9 makes shooting
contestable without making the match unwinnable:

- the human can aim the shot across the goal mouth;
- the defending keeper saves a central shot but can be beaten by an aimed
  shot or when displaced;
- a held long ball behaves like held kick/pass;
- the controls and docs state the real mappings.

## 2. Evidence

- `/tmp/opencode/play/P8-ai-recon.md` finding 4: the shipped keeper is at
  `(0,2538)` when the centre shot passes within `0x2F`; a keeper-inclusive
  `0x40` block would always stop the centre shot. There is no native
  open-play keeper save; the native claims are the row-1E loose-ball take and
  the phase-6 penalty dive.
- P8 shot block: live phase-2 KICK flight, defender-only, `0x40` reach,
  30-frame pin, no score/phase/goal writes.
- P9 reports `/tmp/opencode/play/P9-shot-report.md`,
  `P9-fix2-report.md`, `P9-review-fix-report.md` (when present).
- Aim axis: native movement maps Left/Right to the goal-side z axis; the
  goal-mouth width is the world-x depth axis, so **Up/Down** aim to ±0x80 and
  Left/Right do not. P9-fix2 corrected the initial Left/Right mapping.

## 3. Requirements

1. **Aim.** For a human KICK at the FU-73 latch, sample the held depth input:
   Up `0x01` → `x=+0x80`, Down `0x02` → `x=-0x80`, both/none → centre `0`.
   The goal-side `z` stays `±0xB30`; the target is inside the mouth
   (`|x| < 0xD0`). A real rising edge or direct held-bit sample prevents a
   stale aim; the aim is cleared on latch/save/flight end. Live-only, no
   score/phase writes.
2. **Keeper save.** Before the live goal armer, for a live KICK flight aimed
   into the mouth, the defending keeper (non-controlled record 0) saves when:
   it is goal-side of the ball, its `x` is within
   `FIFA96_MATCH_LIVE_KEEPER_SAVE_REACH` (`0x40`) of the target x, and it is
   within `FIFA96_MATCH_LIVE_KEEPER_SAVE_NEAR` (`0x180`) of the ball. A
   `±0x80` aim beats it; a ball already past it is not saved; a ball already
   inside the native mouth is not saved. The save pins the ball for 30 frames
   with no score/phase/goal write. It is a labelled stand-in for the unported
   native open-play keeper save.
3. **Held long ball.** `LONG_BALL` (`0x40`, V/pad West) is included in the
   SDL3 held re-presentation set and reaches `match_run_map_key` like KICK and
   PASS; menus are unchanged.
4. **Controls/docs.** The Controls page and README state Up/Down aim, the
   keeper's central-shot behaviour, the pad/keyboard dual roles, and the
   half-time C/PASS gate. The 15-line Controls page fits above the footer.

## 4. Invariants

- M1 `09b726b7…` immutable; M2 v29 `7c3874f7…` exact; census 78/78, 58/80,
  64/80; no golden re-pin.
- Every new branch is `live_start`-gated; no score/phase/goal cell is written
  by the aim/save stand-ins; no staged score in the shipped proof.
- Stand-ins are labelled; unknown native behaviour stays a numbered leg.

## 5. Legs / non-goals

- Native aim/camera director and per-code target algebra/loft/receive
  (P9-L7).
- Native open-play keeper save/dive/reach (P9-L8/L9).
- Keeper possession install on save (the save pins; the post-save chase is a
  leg, P9-L11).
- Half-time sit-1 gate still needs repeated KICK/C pulses in the windowed
  path (P9-L10).
- Row-05 `FUN_0007F7E0`/row 03/full 10-player AI; Practice/Leagues/
  Tournaments/Playoffs/Load Game/Modem Setup; replay/captions; rosters;
  commentary/music.

## 6. Acceptance

- Centre kickoff Z → keeper save, 0-0; Up/Down+Z → native goal, 1-0; full
  time reaches the result and returns to Game Select.
- `make check` 119/119; M1/M2 exact; census unchanged.
- Evidence: `p9-save2.png` (0-0; the deterministic save test is the proof),
  `p9-aim-goal2.png` (1-0), `p9-fulltime2.png` (FULL TIME 1-0).
