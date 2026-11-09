# FIFA 96 — native per-player appearance and visible dribble ball (Phase 86)

Goal: finish the live gameplay report "the soccer players physics, walking,
kicking and skin colour are bad" by replacing the one-ramp-per-team skin/hair
stand-in with the native per-player classes, moving the kit colours to the
native low palette bands so they stop overwriting the skin ramps, and keeping
the carried ball visible in front of the carrier when the pad is neutral.

## Evidence

### 1. Native appearance classes

`FUN_0004C00C` (`0x4C00C`) converts a PCCD player digit to a hair class 0..3:

```
0x4C00C  iVar1 = dword[player + 0x1A] >> 24      ; byte +0x1D
         0x30 -> 3, 0x31 -> 2, 0x34 -> 0, else 1
```

`FUN_0004C098` (`0x4C098`) converts the skin digit:

```
0x4C098  iVar1 = dword[player + 0x1B] >> 24      ; byte +0x1E
         0x30/0x32 -> 1, 0x31 -> 3, 0x33 -> 2, 0x34 -> 0
         else -> (FUN_0004C00C() == 3 ? 3 : 0)
```

The retail PCCD player record is 0x24 bytes; the same two ASCII digits sit at
record +0x1C (hair) and +0x1D (skin). The native lineup matrix points one byte
below the PCCD record, so its +0x1D/+0x1E are the PCCD +0x1C/+0x1D. Retail
samples (`fifa96_team_data` ISO probe):

```
Brazil  slot 0 rec 2047 hair '2' skin '4'   (all Brazil: skin '4')
Germany slot 0 rec  931 hair '0' skin '2'   (Germany: hair '0', skin varies)
```

### 2. Native ramps and kit destinations

`FUN_00047298` (`0x47298`) builds one 0x8E-byte translation table per record.
The native tables (fresh `read_memory`):

```
0x1070D4 hair, four 4-byte ramps indexed by the class:
  class 0  00 00 00 00
  class 1  EB EC FC FD
  class 2  F0 F1 F2 F3
  class 3  D7 D8 D9 DA
0x1070E4 skin base, one byte per class:
  class 0 F6 | class 1 ED | class 2 E4 | class 3 DB
```

The skin write is `remap[0xDB+i] = base + i` for i = 0..8; the hair write is
`remap[0xD7+i] = ramp[i]` for i = 0..3. The palette at `0xDB..0xFD` is the
native four-ramp skin table (light through dark), not a kit band.

The native kit-band table at `0x106B70` is six `{destination, length, group}`
rows with destinations `37, 48, 59, 70, 81, 92` (length 0xB) and `101`
(length 9). Phase 85 had moved the kit bands to `195..229` and overlaid
PALteam over palette `0x1E..0xE5`; that overwrote skin entries `0xE0..0xE5`
and forced every player onto the `0xF6` ramp. The native bands are the low
slots, and the skin ramps at `0xDB..0xFD` stay untouched.

### 3. Carried ball

The native possession staging `0x7A490` is still unported; the Phase-83 live
glue pinned the ball at `carrier.pos + dir*0x20`. When the pad was neutral
(`dir = 0`) that put the ball at the carrier's centre, where the sprite hid
it. The carrier still has a facing octant (`record.type`), so the glue now
uses the same `0x20` step along `fifa96_match_dir_x/z[type]` when the pad
direction is zero. This is the documented [D] stand-in, not a native claim.

## Implementation

All live-only (`live_start`); M1/M2 never set it.

- `fifa96_pccd_player` carries `hair_code`/`skin_code` from record +0x1C/+0x1D.
- `fifa96_team_data_appearance` resolves one roster slot's two digits.
- The bridge stores the chosen LANG ids in `matchup.team_id[]`, fills
  `roster_hair_code`/`roster_skin_code` from the PCCD roster, and builds
  `sub_lineup` (the derived pitch -> roster map).
- `match_run_stage_live_kit` writes the six team kit bands at
  `37/48/59` (home) and `70/81/92` (away), no longer overlaying PALteam over
  the skin range. It builds `remap_team` (default skin 0 / hair 3) and
  `remap_entity[side][pitch]` with the player's native hair/skin classes.
- The live sprite draw selects `remap_entity[side][pitch]`; the ball and HUD
  keep their existing maps.
- Boot pixels `0xE4/0xE5` map to black (the native template does not route
  them through a kit band; black is a neutral boot).
- The neutral-pad ball glue falls back to the facing octant.

## Verification

- `test_engine_team_data` asserts Brazil slot 0 `hair '2'/skin '4'` and Germany
  slot 0 `hair '0'/skin '2'`.
- `test_engine_match_bridge` asserts the Team Select ids reach the run and the
  PCCD appearance digits land in `roster_*_code`.
- `test_engine_match_render` pins the six native kit destinations, the native
  skin ramps at `0xF6`/`0xDB`, the default/per-pitch tables and a composed
  player (skin '3' -> base 0xE4, hair '4' -> black).
- `test_engine_live_control` pins the neutral-pad ball offset.
- `make check` **124/124**; M1 byte-exact `09b726b7…`; M2 byte-exact
  `41521559…` (both `cmp`-verified).
- Screenshots: `docs/screens/p86-live-appearance.png` (real windowed match,
  Brazil dark skin/Germany light skin, ball visible),
  `docs/screens/p86-live-dribble.png` (Brazil dribbling, ball in front),
  `docs/screens/p86-kick-pose.png` (native kick row playing).

## Open legs

- The native lineup matrix `0x14C360` (`FUN_00012010`) is not ported; the
  pitch -> roster map is the derived `sub_lineup` builder. A substitution
  after the kit stage does not rebuild `remap_entity` yet.
- The native template pattern pass (`FUN_00047298` `0x8E`-byte templates,
  five template classes, `FUN_0001CAEC(9/0xD)` gates) remains open.
- Native possession staging `0x7A490` and row-05 target algebra
  `0x7F3A1..0x7F57B` remain open; the ball offset is derived.
- Native vertical ball staging and the class-3 camera integrator remain open.

## Provenance

Ghidra MCP, read-only, program **`/FIFA96.EXE`**:

- `decompile_function 0x4C098` and `0x4C00C` (the two digit -> class
  conversions).
- `decompile_function 0x47298` (the per-record translation builder and its
  `0x8E`-byte template pass).
- `read_memory 0x1070D4` (20 B hair ramps) =
  `00 00 00 00 eb ec fc fd f0 f1 f2 43 d7 d8 d9 da f6 ed e4 db`.
- `read_memory 0x1070E4` (16 B skin bases) =
  `f6 ed e4 db 84 87 8c 8f 92 96 99 9e a1 a4`.
- `read_memory 0x106B70` (72 B kit rows) =
  destinations `25 00 00 00`, `30 00 00 00`, `3b 00 00 00`,
  `46 00 00 00`, `51 00 00 00`, `5c 00 00 00`, `65 00 00 00`; lengths
  `0b 00 00 00` (x6) and `09 00 00 00`.
- `read_memory 0x1083CC` / `0x1083D8` (the class-3 side block, retained as the
  open camera leg; not used by this slice).
- Retail data: `fifa96_team_data` ISO probe printed the Brazil/Germany PCCD
  player records at `+0x1C/+0x1D` (see §1).

Engine reads before editing: `fifa96_pccd.c` `fifa96_pccd_player`,
`fifa96_team_data.c` `team_data_player_at`, `fifa96_match_bridge.c` roster
handoff, `fifa96_match_run.c` `match_run_stage_live_kit` /
`match_run_apply_player_remap` / the possession glue.
