# FIFA 96 — player-facing pause substitution menu (Phase 74)

Goal: replace the P14 auto-pick substitution with a two-step OUT -> IN menu over
the PCCD roster, without moving the M1/M2 goldens.

## Evidence

- Native owner: `FUN_00041494` case 5 (`0x418ED..0x41C80`). Status 3 is the
  GK mismatch (`0x41982`), status 5 the accepted pair (`0x41C80`) and status 8
  the used-player refusal (`0x41AE3`). The native 16-slot desired order
  (`0x14C360` / `FUN_00012010`), FSM timing (`0x14AEC8` timer `0x1E`, status 6
  at `0x416BF`, case-7 reconcile), desired-order swap
  (`0x41B12`/`0x41B5F`/`0x41B91`), per-line counters (`0x14C3D0`/`0x14C3D1`)
  and CPU substitutions remain numbered legs.
- PCCD player records carry position at `+0x09` and overall rating at `+0x1B`;
  the roster order is not GK-first (Brazil: slot 0 forward, slots 11/14
  keepers).
- The native formation layout `0x11033A` is the role source: 4 blocks of
  `{role, count, slots[5]}` (role 0 GK, 1 D, 2 M, 3 F).

## Steps

1. Extend the team-data handoff with position/rating bytes
   (`fifa96_team_data_roster_full`) and copy them in the match bridge.
2. Add the derived pitch-record -> roster-slot map: highest-rated unused
   player per formation role, then unused non-GK, then any unused; bench mask
   is the complement. Immutable once a substitution is applied.
3. Add the menu state and API: OPEN/UP/DOWN/CONFIRM/BACK, OUT -> IN two-step,
   GK-only-GK, used-bench and three-sub refusals.
4. Edge-latch the engine keys (CONFIRM/DECLINE/SUBST/UP/DOWN) so held keys do
   not repeat; ESC backs out one menu level; resume closes the menu.
5. Render a 260x184 panel with the staged HUD font and shade ramp; selected
   row underline; live-only.
6. Tests: role/rating lineup, bench mask, navigation/cancel, outfield and GK
   substitutions, refusal statuses, three-sub cap, resume close, no-PCCD
   identity fallback, panel geometry/non-live suppression, Brazil roster
   position/rating facts.
7. Run `make check`, M1/M2 byte comparisons, and a windowed pause/menu smoke.

## Result

All seven steps landed. The menu opens on pause **S**/pad North, lists the
starter names/numbers/positions, filters the IN list by GK class, applies the
pair, and returns to the pause plate with `OUT > IN`. `make check` 124/124;
M1/M2 exact by `cmp`; screenshots `docs/screens/p15-sub-menu.png` and
`docs/screens/p15-sub-applied.png`.

## Non-goals

- No claim of the native `FUN_00041494` FSM/timing or 16-slot order.
- No native jersey-number source; numbers remain the stable 1-based roster slot.
- No CPU substitution producer.
- No M1/M2 re-pin; the tapes never set `live_start` or `paused`.
