# FU-139: M2 action cluster B — ball staging/resolver, kick trajectory, possession rows

Task 6 of the M2 child plan (`docs/superpowers/plans/2026-10-07-fifa96-m2-match.md`).
The spec's cluster B is "Ball — staging, resolver, pairing, kick/trajectory,
possession/tackle" (design §3.3); this slice re-verifies the ball-related action
rows read-only in the authoritative program, derives the pure parts of the ball
staging block, the reception target, the event-row resolver, the kick range/
resolver path and row 07's stage-0 target, ports them into
`fifa96_ball_pairing` / `fifa96_action_handlers` with tests, and records why no
cluster-B row is wired into the FU-137 dispatch seam.

Result in one line: **rows `05`/`06`/`07`/`0F` and the possession/tackle rows
`18`/`21`/`23` re-verify against `0x1106E0`; the ball staging block
`0x158730..0x158746` (clear `FUN_0007A028`, core stage `FUN_0007A490`), the
`FUN_0007A084` camera-led reception target, the `FUN_0007AE70` event-row
resolver (class/sector/idx/band → one of four 10-byte tables: `0x1102FE`,
`0x11016E`, `0x110196`, `0x11024A`), `FUN_0007B9C4`'s negative-mode range band,
the `row[0]`/actor-action append selector (flat `0x7AE38`) and row 07's stage-0
target are ported as seven tested pure functions; no row is wired because every
cluster-B body still needs unported arms and/or the absent entity/ball pool
(FU-139 OL-29..OL-32, carrying FU-137 OL-1/OL-16), so the FU-137 action
table stays `1 × OK + 44 × UNSUPPORTED`.**

## Method

* Authoritative program **`/FIFA96.EXE`** (explicit in every call). Ghidra
  **read-only**: no renames, comments, labels, function creation, scripts or
  project saves. `/fifa96_le.bin` and the MZ loader were not used.
* Tool calls made this slice:
  * `read_memory /FIFA96.EXE 0x1106E0` (180 B) — the full action table,
    matching FU-136 §1.1/FU-137 §1.1/FU-138 §1 dword for dword;
  * `disassemble_bytes` — row `05` head `0x7F194..0x7F2D0`; row `06` head
    `0x801B4..0x80270`; row `07` `0x814B0..0x815D0`; row `0F`
    `0x82AD0..0x82B60` and `0x82B60..0x82DD0`; row `18` head
    `0x849B0..0x84A40`; row `21` head `0x85214..0x852B0`; row `23` head
    `0x82F84..0x83010`;
  * support bodies: `FUN_0007A028`/`FUN_0007A084` head
    (`0x7A028..0x7A4F0`), `FUN_0007A490` head+mid (`0x7A4F0..0x7A940`,
    `0x7A940..0x7AA30`), `FUN_0007AE70` (`0x7AE70..0x7AF20`,
    `0x7AEF0..0x7B1A0`), `FUN_0007B9C4` (`0x7BBB0..0x7BC60`,
    `0x7BE00..0x7BF10`);
  * `read_memory 0x7AE38` (56 B, the inline jump table), `read_memory
    0x1104CA` (24 B, the sector bitmask table).
* **Address mapping.** In `/FIFA96.EXE` the loader's object-4 fixups are
  already applied, so data immediates render at their resolved flat addresses
  (`[0x00158730]` = the ball block, matching FU-137/FU-138's native convention;
  FU-73/FU-76/FU-78 wrote the pre-loader `0x58730` form). The action table at
  `0x1106E0` is read directly; code addresses are identical to
  `/fifa96_le.bin` (FU-130 §6). `FIFA96.EXE` defines no functions at the table
  targets, so every row citation is `disassemble_bytes` output.
* Baseline at task start: `make check` = **98/98** (FU-138 commit `7a2c7b0`);
  after this task 98/98 with the extended library tests (no new binary).
* Evidence chain: FU-136 §2 (row inventory/tasks), FU-137 §6.1 (row classes),
  FU-73 (ball block/pairing), FU-76 §3 (kick path/resolver), FU-77 §2.5–2.7
  (rows 05/06/07), FU-78 (possession/tackle/duel family). This doc re-verifies
  the slices it ports and refines three prior claims (see Errata).

## 1. Table re-verification (`0x1106E0`)

`read_memory 0x1106E0` (180 B = 45 dwords) returns, for the cluster-B slots:

```
0x05 0x07F194  0x06 0x0801B4  0x07 0x0814B0  0x0F 0x082AD0
0x18 0x0849B0  0x21 0x085214  0x23 0x082F84
```

exactly FU-136 §1.1/FU-137 §1.1. The sole reader remains `FUN_0007D9A4
@ 0x7DA77` (`EAX = code << 2; ADD EAX,0x1106E0`), and the call contract is
`EAX = record` only (FU-137 §2).

## 2. Cluster-B per-row derivation

Windows marked ✓ were re-read first-hand this slice; the body column is the
strongest of the fresh read and the named FU derivation. Field numbers are the
native record offsets.

| code | handler | gate / entry (verified) | timer | stage / arms | record-visible body | class / open leg |
|---|---|---|---|---|---|---|
| 05 | `07F194` | ✓ `[+0x9E]=1` then phase `!=2` → `FUN_0007DAB4`, return (`0x7F19F..0x7F1BA`) | ✓ `+0x89 < 0x4B0` then `+= [0x157A64]` (`0x7F221..0x7F23A`) | ✓ `+0x92 <= 3` jump table flat `0x7F184`; stage 0 head `0x7F274..` | carrier claim (`[0x158724] != rec` → block clear + set, `0x7F1BF..0x7F205`); team `+0x7B2=rec`/`+0x7B6=0`; camera triple copy `→+0x4D..55`; stage 0 gates (`+0x6B > 0x40`, `[0x57A83]`, `+0x6B > +0x77`, release timer `0x15872D`, airborne `+0x5D`) + type-table dribble dir (`0x10F334`/`0x10F33C`, speed `0x60`/`0x30`); stages 1–3 (animation, snap, hand-off 4) per FU-78 §3.2–3.4 | not ported (partial): `possession_reset/claim/timer/dribble_dir` tested; carrier arms OL-29 |
| 06 | `0801B4` | ✓ `[+0x9E]=1`; phase `!=2` → `FUN_0007DAB4` + clear `team+0x7B2/0x7B6`, return (`0x801BF..0x80223`); ✓ `[+0x8D]==0` → same reset (`0x801E5`) | — | — | ✓ carrier gate `[0x158724]==0` or carrier `[+0x69]>>16 > 0x90` or `[0x157750] > 0x70` → `FUN_0007D9A4(rec,4,invoke-now)` (`0x8022D..0x80263`); else 597-insn pursuit target algebra (metric/atan/vector/RNG, installs 8/9/4) per FU-77 §2.6 | not ported: body OL-30 |
| 07 | `0814B0` | ✓ phase `!=2` or `word[+0x81]!=0` → tail `0x81702` (`0x814B8..0x814D1`) | ✓ `+0x89 += [0x157A64]` (`0x814D9..0x814ED`) | ✓ `+0x92` 0/1/2/tail (`0x814F3..0x8150C`) | stage 0 (`0x81512..0x815B5`): `[+0x9E]=1`; gate `([+0x69]>>16 > 0x40` or `(int16)(word[+0x5D]+0x70) < [0x157750])` with `+0x89 > 0x3C` → tail else wait; target = slot word `0x60`/`0x8000` arm (camera + type-offset table bytes
`0x10F334`/`0x10F33C` << 4) or camera triple; stage 1 KICK `FUN_0007B9C4` + opponent `0x22` invoke + `[+0x89]=0,+0x92++`; tail resets and installs `4` on `[0x158730]` | not ported (partial): `kick_angle/apply` + FU-139 resolver/stage target; machine OL-31 |
| 0F | `082AD0` | ✓ phase `!=2` → `FUN_0007DAB4`, return (`0x82ADB..0x82AEF`) | ✓ `+0x89 += [0x157A64]` (`0x82AF6..0x82B0A`) | ✓ `+0x92` 0/1/2 (`0x82B10..0x82B1C`) | stage 0: active gate, `word[+0x85]!=0` → reset; metric to vector `0x157788`, pos nudge `>>0x11`, face/anim 4; stage 1: predictor `FUN_00071B9C([0x157A62]>>16)`, distance compare, first/second `FUN_0007B9C4` (slot temporarily nulled, mode from slot or RNG `0x40`/`0x20`), then `word[+0x81] = 2*word[+0x85] - word[+0x87] + 0x1E` (`0x82B61..0x82DBE`) | not ported: machine OL-31 |
| 18 | `0849B0` | ✓ none (`+0x7B=2` at entry, `0x849B7`); `[+0x89] += [0x157A64]` (`0x849BD..0x849D3`) | ✓ | ✓ `+0x92` 0/1/2 (`0x849D9..0x849E7`) | stage 0 animation abort `0x55`/`0x6A` (`0x849F4..0x84A0A`); stage 1 target `(0x900,0)` + metric `FUN_0008DCD4`; stage 2 window `0x78..0x12C` + input byte + distance `< 0x20`; resolution NSEARCH `FUN_0008DB6C` + SWAP `FUN_000786A0` (FU-78 §7) | not ported (partial): `duel_step/split` tested; resolution OL-32 |
| 21 | `085214` | ✓ phase `!=2` or `rec != [0x157A83]` → `FUN_0007DAB4`, return (`0x8521F..0x8523D`); ✓ camera triple copy (`0x85242..0x8524C`) | ✓ stage 0/1 gates (`0x85269..0x85361`) | ✓ `+0x92` 0/1/default (`0x85253..0x8525F`) | stage 0 inactive → reset (no hand-off); offset `+0x6B > 0x40` with `+0x89 > 0x3C` → reset path; else nearest `FUN_0008DE8C` + metric + angle + event `0x4A` and `+0x89=0,+0x92++`; stage 1 event flag `+0x44` or offset `+0x41` → reset path with ball-actor install 4 + `FUN_00079B58` (FU-78 §4) | not ported (partial): `action_receive_step` tested; claim arm OL-32 |
| 23 | `082F84` | ✓ phase `!=2` → `FUN_0007DAB4`, return (`0x82F8C..0x82F97`) | ✓ `+0x89 += [0x157A64]` (`0x82F9F..0x82FB3`) | ✓ `+0x92` 0/1/default (`0x82FB9..0x82FC1`) | stage 0 `[+0x9E]=1`, inactive → reset, else `+0x89=0,+0x92++`; stage 1 target arm `0x82FF3..0x83055` (camera words `0x1577EE`/`0x1577FA`/`0x157800`, slot button `0x40`, vectors `0x157794`/`0x157788`, ±0xC0 side nudge, `FUN_00079B58`); window gates + attempt `FUN_00082DD0` → install `0x0E`; success → install `0x0F` (FU-78 §6) | not ported (partial): `tackle_step/attempt` tested; target arm OL-32 |

Cross-family state shared by the table: `+0x89` action timer, `+0x92` stage
byte, `+0x8D` active flag, `+0x9E` "ran" byte, `+0x44` animation-row terminal
bit, `+0x81` event/timer latch, `[0x157A4A]>>24` phase, `[0x157A64]` frame
delta, `[0x157A83]` controlled-record pointer, `[0x158724]` event carrier,
`[0x158730..0x158746]` ball staging block.

## 3. Support-item derivations and the ported pure functions

### 3.1 Ball block clear and stage core

`FUN_0007A028` (`0x7A028..0x7A081`) is the block clear (✓ read this slice):
`[0x158730]=0` and `[0x158734]=0`; words `0x158738`, `0x15873C`, `0x15873A`,
`0x15873E`, `0x158740` = 0; then `byte 0x158743 = 2`, `byte 0x158742 = 0x20`,
bytes `0x158744`/`0x158745`/`0x158746` = 0. Ported as
`fifa96_ball_pair_clear`.

`FUN_0007A490` is the staging writer (✓ read `0x7A4D3..0x7A4EA` and the
`0x7A8D1..0x7AA2B` tail). The entry core (EAX = actor, EDX = 6-byte vector,
BX = trajectory word, CL = event code) sets `[0x158730]=actor`, copies the
6 bytes to `0x158738`, writes `[0x15873E]=BX` and `byte 0x158743=CL`. Ported as
`fifa96_ball_pair_stage`. The entry also parks a stack sub-code word and gates
`[rec+0x99]`/`[0x157A6C]` (`0x7A498..0x7A4C6`), and the code-keyed tail
(`0x7A8D1` reads `byte 0x158743` into DX; `0x7A941` sets sub-code `0x30` for
code 2, `0x31` for 1/3/6, and the **inactive** 4/5/7 arm at `0x7A97F` resets
the whole block via `0x7A987..0x7A9D9`; the **active** arm at `0x7A9DE` calls
`FUN_00079C50` for 1/2/3/6 and `FUN_0006E598` for the rest) is unported:
**OL-26**.

### 3.2 Reception `FUN_0007A084`

Already ported (FU-73 §5): the skip rule (`[+0x8E]>>24==1` or `[+0x91] ∈
{0x10,0x11,0x12}` → skip = `(int8)[+0x8D]`, else 0, `0x7A335..0x7A369`),
`FUN_0008DE8C` nearest (`0x7A376`) and the assign/clear pair
(`0x7A37F..0x7A3A2`). The special class `0x18E2` arm (`0x7A0D6..0x7A207`,
RNG/scatter/angle fold into the staging block), the action-code sound arms
(`0x7A234..0x7A2E1`) and the carrier countdown write (`0x7A40A..0x7A412`) stay
unported: **OL-32**.

### 3.3 Reception camera-led target

`0x7A2FF..0x7A331` (✓): copies the `0x157770` triple (three MOVSD = 12 bytes,
32-bit fields) and adds `([0x1577BE]>>16)<<5` to x and `([0x1577C0]>>16)<<5`
to z (arithmetic shift, so the addend is signed), y untouched. Ported as
`fifa96_ball_pair_receive_target(base, lead_x, lead_z, out)` (the port shifts
the sign-preserving bit pattern in unsigned arithmetic, reproducing the native
`SAR`+`SHL` without a UBSan-negative-shift diagnostic).

### 3.4 Kick range band

`FUN_0007B9C4` `0x7BBE4..0x7BC15` (✓): `if ((int8)byte[0x158742] >= 0) return;
x = (int16)word[0x158738]; band = x < 0x5A0 ? 0x20 : (x < 0x780 ? 0x30 :
0x10); byte[0x158742] = band`. Ported as `fifa96_action_kick_range_band`.

The rest of `FUN_0007B9C4` keeps its FU-73/FU-76 status: the row application
`0x7BCB6..0x7BE0B` is the existing `fifa96_action_kick_apply`; the tail
`0x7BE0B..0x7BEDE` (✓ reread) is `traj += row[+6]` (done), then for
`x != 0 && code != 3`: code 4 RNG branch (`0x7BE45..0x7BE99`, `FUN_00092AC8`)
and `if (divisor != 0) traj = comp_z + speed/divisor` (`0x7BE9C..0x7BEC0`,
signed `IDIV` of `[0x158736]>>16`), then the `0x460` cap. The divisor branch and
the code-4 RNG arm are unported: **OL-28**.

### 3.5 Event-row resolver `FUN_0007AE70`

Inputs EAX = actor, EDX = code, EBX = x word, ECX = z word (✓
`0x7AE70..0x7AF20` and `0x7AEF0..0x7B1A0`):

* **class** (`0x7AE79..0x7AEA2`): `code == 0x40` → 2; else `(code & 0x10) ? 0 : 1`.
* **sector** (`0x7AEA2..0x7AECA`): `x == 0 && z == 0` → `(int8)actor[+0x8E]`;
  else `((FUN_000CD474(x,z) + 0x40) & 0x3FF) >> 7` (0..7).
* **idx** (`0x7AED2..0x7AEFA`): `code == 0x40` → 0; else
  `(byte[0x1104CA + subtype] >> sector) & 1` (x86 shift-count masking).
* **d / height** (`0x7AEFD..0x7AF1A`): `d = (int16)(word[0x157750] -
  word[actor+0x5D])`; if the **dword** `actor[+0x5D] != 0` and `d < 0x38`,
  return NULL (no row).
* **table/index**:
  * height present: `table = 0x1102FE`, `index = 2*class + idx` (`0x7AF20..0x7AF3A`);
  * else fast carry (`has_slot && active && phase==2 && class==1 &&
    slot[+0x23] < 7`): `table = 0x11016E`, `index = idx` (`0x7AF3F..0x7AF96`);
  * else `has_slot && code == 0x60`: `table = 0x11016E`, `index = idx + 2`
    (`0x7AF9B..0x7AFBD`);
  * else band `= d < 0x20 ? 0 : (d < 0x70 ? 1 : 2)`, `table = active ?
    0x110196 : 0x11024A`, `index = 2*band + 6*class + idx`
    (`0x7AFBF..0x7B016`);
  * row = `table + 10*index` (`0x7B016..0x7B018`).
* **append selector** (`0x7B01A..0x7B09F`): first the **actor action** byte
  `[+0x91]` is tested — `0x11`→ring(2), `0x12`→ring(7), `0x13`/`0x20`→ring(8),
  `1`→direct(1), `0x10`→ring(3); every other value falls to the switch on
  `row[0]-1` through the 14-dword table flat `0x7AE38`
  `{0x7B0A7,0x7B0BB,0x7B0CF,0x7B0E3,0x7B0F7,0x7B10B,0x7B11F,0x7B133,
  0x7B133,0x7B07D,0x7B147,0x7B147,0x7B15B,0x7B16F}` with
  `row[0]=0`/`>0x0E` → direct(0) (✓ table read at `0x7AE38`; arm bodies
  `0x7B0A7..0x7B183`): row 1→0x0D, 2→0x0F, 3→0x11, 4→0x0A, 5→0x09, 6→0x14,
  7→0x13, 8/9→0x16, 0x0A→3, 0x0B/0x0C→4, 0x0D→0x10, 0x0E→0x0C.

Ported as `fifa96_action_kick_event_row` (class/sector/idx/band/table/index)
and `fifa96_action_kick_event_append` (selector). The ring/direct sinks
(`FUN_000928F0` 25-entry ring at `0x5B440`, `FUN_00092820` store at `0x5B650`)
are presentation-side and unported: **OL-27**.

### 3.6 Row 07 stage-0 target

`0x8154C..0x815B5` (✓): when `[rec+0x20] != 0` and the slot word `[slot+6]` is
`0x60` or `0x8000`, `out.x = [0x15774C] + (int8)table[0x10F334 + type8] << 4`
and `out.z = [0x157754] + (int8)table[0x10F33C + type8] << 4` (y untouched);
otherwise the camera triple `0x15774C..54` is copied. The native addressing is
a dword load at `0x10F331 + type8` followed by `SAR 24` (`0x81571`/`0x81577`,
`0x8158E`/`0x81597`), i.e. the sign-extended byte three bytes into the record:
the x-offset base is `0x10F334` and the z-offset base `0x10F33C` — the same
bytes FU-138 §3 reads for the row-0D velocity tables, not the raw `0x10F331`/
`0x10F339` load addresses. `type8 = [rec+0x8B]>>24`. Ported as
`fifa96_action_kick_stage_target` (the caller supplies the two extracted
per-type byte tables, so the port is unaffected by the addressing). The
stage-0 gate above it (`0x81512..0x81548`) is documented in §2 but not ported
(OL-31).

### 3.7 Ported pure functions

All caller-owned, table-free where the native reads a table through an
argument, negative error on NULL `fifa96_err_t` (the FU-76/77/78/138 port
convention). Byte citations are §3's windows.

| function | native site | semantics |
|---|---|---|
| `fifa96_ball_pair_clear(&state)` | `FUN_0007A028 0x7A028..0x7A081` | zero pointers/vector/traj/angle; `flags=0x20`, `code=2`, tail bytes 0 |
| `fifa96_ball_pair_stage(&state, actor, &vector, traj, code)` | `FUN_0007A490 0x7A4D3..0x7A4EA` | record actor, copy the 6-byte vector, write traj and code; flags untouched |
| `fifa96_ball_pair_receive_target(&base, lead_x, lead_z, &out)` | `FUN_0007A084 0x7A2FF..0x7A331` | `out = base`; `x += (lead_x>>16)<<5`, `z += (lead_z>>16)<<5` |
| `fifa96_action_kick_range_band(mode, ball_x, &band)` | `FUN_0007B9C4 0x7BBE4..0x7BC15` | non-negative mode unchanged; else 0x20/0x30/0x10 by x |
| `fifa96_action_kick_event_row(&event, sector_table, &out)` | `FUN_0007AE70 0x7AE79..0x7B018` | class/sector/idx/d/band → `{found, table, index}` (four tables, 10-byte stride) |
| `fifa96_action_kick_event_append(actor_action, row0, &out)` | `FUN_0007AE70 0x7B01A..0x7B09F` + table `0x7AE38` | actor-action prefix then `row[0]-1` switch → `{direct, code}` |
| `fifa96_action_kick_stage_target(...)` | row 07 `0x8154C..0x815B5` | slot-word arm (camera + type table << 4, y untouched) or camera copy |

### 3.8 Tests

* `tests/test_ball_pairing.c`: layout `_Static_assert`s on the new structs;
  clear field-by-field; stage fill/preserve; receive-target lead shifts
  (positive/negative/sub-1 truncation); NULLs.
* `tests/test_action_handlers.c`: layout asserts; range-band boundaries
  (`0x59F`/`0x5A0`/`0x77F`/`0x780`, non-negative pass-through, negative x);
  resolver fixtures for the height table, fast carry, `code 0x60`, the three
  bands with both state tables, the angle-derived sector, the sign-extended
  `0xE1` sector mask (`& 0x1F`), the `d < 0x38` none path and NULLs; all 20
  append-selector branches (6 actor actions + 14 `row[0]` entries, plus the
  default/over-range direct paths); stage-target slot/camera arms and NULLs.
* The plan's "kick moves the ball per the derived trajectory table" is
  `test_kick_resolver_selects_row_and_moves_ball`: the resolver picks row index
  0 of the height table, the test reads that row from a synthetic 10-byte table
  and `fifa96_action_kick_apply` clamps the ball x to the row's upper bound and
  adds the row's trajectory field.

## 4. Engine wiring status (0 new rows)

No cluster-B row is wired in this task. The FU-138 rule is unchanged: a row is
wired only when its **full** record-visible body is covered by tested C
functions (row `00` was). Every cluster-B row still has unported arm support
and/or needs the absent entity/ball pool, so wiring it would silently skip
native side effects (the exact failure FU-138 §7 rejected):

| row | what is missing | leg |
|---|---|---|
| 05 | carrier stage machine (target algebra, `FUN_0007F7E0`, snap/hand-off animation) | OL-29 |
| 06 | 597-insn pursuit body (target algebra, RNG, installs 8/9/4) | OL-30 |
| 07 | kick machine (`FUN_0007E600`, opponent `0x22` invoke, ball-actor install 4, `FUN_0007DAB4`) + full `FUN_0007B9C4` target selection | OL-31/OL-28 |
| 0F | second kick machine (predictor, RNG mode, timer reload) | OL-31 |
| 18/21/23 | resolution/claim/target arms + NSEARCH/SWAP + team/opponent records | OL-32 |
| all | 0xB2 record pool, teams, `[0x158730]` actor binding | OL-16 (carried) |

The FU-137 classification is otherwise unchanged: `1 × OK` (action `00`),
`78 × -FIFA96_ERR_UNSUPPORTED`, `1 × -FIFA96_ERR_NOT_FOUND` (phase `0x16`).
The FU-137 §6.1 cells for rows 05/06/07/0F/18/21/23 gain FU-139 cross-refs and
the new legs via the errata at the end of this document and the in-place
FU-137 errata section; their class stays `not ported (partial)`/`not ported`.

## 5. Totals

| group | rows | state |
|---|---|---|
| ported + wired | `00` (cluster A) | unchanged `FIFA96_OK` |
| pure part advanced (cluster B) | `05`,`06`,`07`,`0F`,`18`,`21`,`23` | still `-FIFA96_ERR_UNSUPPORTED`; seven new tested pure functions |
| still unported | all other rows | per FU-137 §6 |

## 6. Open legs

* **OL-26 — ball staging tail.** The code-keyed sub-code/animation tail
  (`FUN_0007A490 0x7A8D1..0x7AE2F`: `0x7A941` sub-code `0x30`/`0x31`, inactive
  4/5/7 block reset `0x7A97F..0x7A9D9`, active `FUN_00079C50`/`FUN_0006E598`
  arms `0x7A9DE..0x7AA2B`, `[0x158744/45]` stack writes) is unported.
  **Status (Task 10): the bounded half `0x7A8D1..0x7AA2F` is ported as
  `fifa96_ball_pair_stage_tail` (§8.2); the residual target algebra
  `0x7AA3C..0x7AE2F` is OL-62 (the eligibility set is `{1,2,3,6,0xE}`, §8.2).**
* **OL-27 — event append sinks.** `FUN_000928F0` (25-entry ring `0x5B440`,
  stride 0x15) and `FUN_00092820` (`0x5B650`) are presentation-side and
  unported; only the selector mapping is ported.
* **OL-28 — full kick path.** `FUN_0007B9C4` target selection
  `0x7BA1E..0x7BBE4` (wing target `FUN_0008DCD4`, slot `FUN_0007B878`), the
  mode-bit-0x20 arms `FUN_0007B194`/`FUN_0007B57C` (`0x7BC34..0x7BC80`), the
  code-4 RNG branch and the divisor line (`0x7BE40..0x7BEC0`), the `0x114E04`
  angle fold and the `0x158736` speed source remain unported (FU-76 §7 leg 5
  extended).
* **OL-29 — row 05 carrier arms.** Stage 0 tail `0x7F3A1..0x7F57B`
  (`FUN_0007F7E0` fallback, camera target algebra, `FUN_00092820`,
  `FUN_00071C94`, `FUN_00079CCC`/`FUN_0006DA64`) and stages 1–3
  (`0x7F57C..0x7F665`: animation select, `FUN_00079B1C`/`FUN_00079C50`, ball
  actor/receiver hand-off) are unported (FU-78 §3.4/OL-2).
  **Status (Task 10): stages 0 head/1/2/3 and the stage-0 dir selection are
  ported as `fifa96_action_carrier_arm` (§8.3); the stage-0 target algebra and
  the `FUN_0007F7E0` fallback are OL-63, and the 0→1 latch edge is OL-64.**
* **OL-30 — row 06 pursuit.** The 597-instruction body (FU-77 §2.6): target
  construction, `0x114E04` folds, RNG gates, installs 8/9/4.
* **OL-31 — rows 07/0F kick machines.** `FUN_0007E600` decision, the opponent
  `0x22` invoke, the ball-actor install 4, the tail `FUN_0007DAB4`, the row-0F
  predictor/RNG/timer reload, and the stage-0 gate are unported.
* **OL-32 — reception/tackle/duel arms.** `FUN_0007A084` special-class/RNG
  arms, the action-code sound arms, `FUN_0004C324` (raw bytes), NSEARCH
  `FUN_0008DB6C` + SWAP `FUN_000786A0`, the row-21 claim arm, the row-23 target
  arm and the row-18 resolution (FU-78 §12 legs 3–9).
* **OL-16 (carried) — entity/record pool.** The 0xB2 pool, team blocks,
  `[0x157A83]`/`[0x158724]`/`[0x158730]` actor bindings and the `FUN_0007D9A4`
  installer remain the C8/C11 replacement for the one-record stopgap; every
  cluster-B handler binding waits on it (carries FU-137 OL-1).

## 7. Concerns

* The resolver's height-path table base `0x1102FE` was missing from FU-76 §3.4,
  and its `index` formula is `2*band + 6*class + idx` (FU-76 wrote `2*code`);
  both are corrected in the errata below. The append selector's explicit
  compares test the actor action `+0x91`, not `row[0]` — FU-76 §3.4 conflated
  them; the corrected mapping is in §3.5.
* The sub-code passed to `FUN_0007A490` comes from a stack argument
  (`[ESP+0x2E]`, `0x7A4D9`) and the tail switches on the event code byte read
  back from `0x158743` (`0x7A8D1`); FU-73 §1's "codes 1/3/4/5/6/7 clear"
  summarizes only the inactive arm. The port keeps the core (unconditional
  stage) and leaves the tail as OL-26.
* Rows 18/21/23 are FU-137 OL-11 rows, not OL-8; FU-139 re-verifies them for
  the possession/tackle half of cluster B but does not advance their port.
* `fifa96_action_kick_event_row` shifts by `sector & 0x1F` to reproduce the
  native `SAR EAX,CL` masking for the unsigned fallback sector byte; this is a
  deliberate x86-faithful detail, pinned by the tests.

## Errata (prior docs)

* **FU-76 §3.4** — "`AL = row[0]`; `AL-1` indexes the inline table" and the
  compares "intercept `row[0]`": the compares at `0x7B020..0x7B03C` test the
  **actor action** byte `[ESI+0x91]`; the `row[0]` switch at `0x7B08E` is the
  fallback (verified bytes this slice). The band formula is `2*band + 6*class
  + idx` (not `2*code`), the height-present path uses table `0x1102FE` with
  `2*class + idx` (not listed), and `d < 0x38` returns NULL only when the
  `actor[+0x5D]` dword is nonzero.
* **FU-73 §1** — the staging tail's clears (`0x7A987`) are reached on the
  **inactive** actor path with event codes 4/5/7; codes 2 and 1/3/6 set the
  sub-code `0x30`/`0x31`, and the active path calls `FUN_00079C50` (1/2/3/6)
  or `FUN_0006E598` (rest). The unconditional stage writes
  (`[0x158730]`/vector/`0x15873E`/`0x158743`) are unchanged.
* **FU-73 §3.3** — confirmed; the added detail is the 32-bit `0x157770` triple
  and the arithmetic `>>16<<5` lead (ported).

## Provenance

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit): `read_memory
0x1106E0` (180 B), `0x7AE38` (56 B), `0x1104CA` (24 B); `disassemble_bytes`
`0x7F194..0x7F2D0`, `0x801B4..0x80270`, `0x814B0..0x815D0`, `0x82AD0..0x82B60`,
`0x82B60..0x82DD0`, `0x849B0..0x84A40`, `0x85214..0x852B0`, `0x82F84..0x83010`,
`0x7A028..0x7A4F0`, `0x7A4F0..0x7A940`, `0x7A940..0x7AA30`,
`0x7AE70..0x7AF20`, `0x7AEF0..0x7B1A0`, `0x7BBB0..0x7BC60`,
`0x7BE00..0x7BF10`. No writes: no rename/comment/label/function/script/project
save. `/fifa96_le.bin` and `/fifa96.exe` untouched.

Repo: `make check` = **98/98** at task start and after (ASan/UBSan engine tests
included); M1 golden and pinned render hashes unchanged (no render path
touched). Port write set: `include/fifa96_loader/fifa96_ball_pairing.h`,
`src/fifa96_loader/fifa96_ball_pairing.c`, `tests/test_ball_pairing.c`,
`include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c`, `tests/test_action_handlers.c`,
`src/fifa96_engine/fifa96_match_handlers.c` (evidence strings only),
`tests/test_engine_match_handlers.c` (comments only),
`docs/ghidra/FU137_dispatch_mechanics.md` (errata). `game/FIFAPCCD96.iso`
untouched; `fifa96.rep/**` churn not staged.

## 8. Task 10 port — ball staging tail and row-05 carrier machine (FU-142-follow / M2 arms-and-wiring T10)

Reviewed read-only in `/FIFA96.EXE` (explicit; Ghidra MCP, no writes). This
section closes the *bounded* half of OL-26 and OL-29: the `FUN_0007A490`
code-keyed staging tail and the row-05 carrier machine stages 0-3, with the
staging-tail target algebra and the `FUN_0007F7E0` fallback left as numbered
legs (OL-62..OL-64). No row is wired (see §8.6).

### 8.1 Tool calls (first-hand, read-only)

* `disassemble_bytes 0x7A8D1..0x7A9E0` (70 insns), `0x7A9DE..0x7AB20`
  (95 insns), `0x7AB1F..0x7AB60` (18 insns), `0x7AB60..0x7AD00` (124 insns) —
  the `FUN_0007A490` tail `0x7A8D1..0x7AE2F`;
* `read_memory 0x1104A0` (48 B) — the eligibility byte table `0x1104BB`
  (`... 00 01 01 01 00 00 01 00 00 00 00 00 00 00 01` for offsets
  `0x1104BB..0x1104C9`, i.e. codes `0..0xE`);
* `read_memory 0x7A458` (56 B) — the per-code jump table (used below for the
  residual leg);
* `disassemble_bytes 0x7F194..0x7F390` (127 insns), `0x7F38F..0x7F580`
  (139 insns), `0x7F57C..0x7F7D0` (203 insns) — row `05` `0x7F194..0x7F665`
  plus the helpers at `0x7F666`/`0x7F6F0`;
* `read_memory 0x7F184` (16 B) — the row-05 stage table;
* `disassemble_bytes 0x71C94..0x71D00` + `0x71D00..0x71E20` (the
  `FUN_00071C94` camera/track family), `0x79CCC..0x79D59` (`FUN_00079CCC`),
  `0x79B1C..0x79B58` (`FUN_00079B1C` snap), `0x6DA64..0x6DB40`
  (`FUN_0006DA64` block swap), `0x7F7E0..0x7F9C0` (`FUN_0007F7E0` head;
  residual).

### 8.2 The staging tail `0x7A8D1..0x7AA2F` → `fifa96_ball_pair_stage_tail`

Native flow, site-annotated (all first-hand this slice):

```
0x7A8D1  DX = (int8)byte[0x158743]              ; the staged event code
0x7A8D9  TEST DX,DX; JL 0x7A8EF                 ; negative -> receive
0x7A8E1  CMP EAX,0xF; JGE 0x7A8EF               ; >= 0xF   -> receive
0x7A8E6  CMP byte[EAX+0x1104BB],0; JNZ 0x7A8F6  ; eligibility table
0x7A8EF  CALL 0x7A084; JMP 0x7A934              ; reception (unported)
0x7A8F6  gate: (int32)[+0x69]>>16 < 0x60        ; == word[+0x6B], signed
0x7A901  +0x59 += (dword[+0x6B])>>17            ; arithmetic SAR 17
0x7A90C  +0x61 += (dword[+0x6D])>>17
0x7A91D  0x1577BE/C0/C2 = 0                     ; camera velocity
0x7A934  if (byte[+0x8D] == 0) {
0x7A941    code 2   -> byte[0x158744] = 0x30
0x7A955    code 1/3/6 -> 0x31
0x7A970    code 7/4/5 -> the whole-block reset 0x7A97F..0x7A9D9
0x7A9DE  }
0x7A9DE  if (code in {1,2,3,6})
0x7A9F5    DX = (int16)(dword[0x158738]>>16)    ; word 0x15873A
0x7AA01    BX = (int16)(dword[0x15873A]>>16)    ; word 0x15873C
0x7AA09    CALL 0x79C50                          ; face
0x7AA0E  EDX = (int8)byte[0x158744] (sub-code)
0x7AA14  EBX = (int8)byte[0x158745] (reserved45)
0x7AA1A  ECX = (int8)[+0x8B]>>24 (type8)
0x7AA2B  CALL 0x6E598                            ; animation id
0x7AA30  if ([+0x20]) CALL 0x78B00               ; slot callback
0x7AA3C..0x7AE2F                                 ; per-code target algebra
```

The `0x1104BB` bytes read this slice are
`{0,1,1,1,0,0,1,0,0,0,0,0,0,0,1}` for codes 0..0xE, so the eligibility arm is
`{1,2,3,6,0xE}` (not only `{1,2,3,6}` as the earlier FU-139 §3.1/§6 prose
implied). The inactive 4/5/7 reset is exactly the FU-73 §1 clear
(`0x158730`/`34` = 0, words `738/73A/73C/73E/740` = 0, flags `0x20`, code 2,
the three tail bytes 0) — ported by reusing `fifa96_ball_pair_clear`.

Ported: `fifa96_ball_pair_stage_tail(&state, &actor, recompute_table, &out)`
(`include/fifa96_loader/fifa96_ball_pairing.h`), with the caller-supplied
eligibility table, the nudge gate/addends, the active/inactive classification,
the clear, the face (through the tested `fifa96_arm_face`), the animation
(`fifa96_arm_anim_select`) and the slot-callback request. The `receive`,
`camera_zero` and `slot_cb` outputs are derived requests/no-ops for the
unported `FUN_0007A084`, the camera block and `FUN_00078B00` (OL-62).

### 8.3 Row `05` `0x7F194..0x7F665` → `fifa96_action_carrier_arm`

First-hand flow (site-annotated; the FU-78 §3 stage table `0x7F184` =
`{0x7F274, 0x7F57C, 0x7F5E9, 0x7F627}` re-read as bytes
`74f20700 7cf50700 e9f50700 27f60700`):

```
0x7F19F  byte[+0x9E] = 1
0x7F1A6  phase != 2 -> CALL 0x7DAB4; RET       ; reset
0x7F1BF  if ([0x58724] != rec) { block clear 0x7F1C7..0x7F205; }
0x7F20B  [rec][+0x7B2] = rec; [rec][+0x7B6] = 0
0x7F221  if (+0x89 < 0x4B0) +0x89 += zero-ext [0x157A64]
0x7F240  if (word[+0x81] != 0) RET
0x7F24E  rec+0x4D/51/55 = camera 0x15774C/50/54
0x7F259  AL = byte[+0x92]; if AL > 3 RET
0x7F274  stage 0: (int32)[+0x69]>>16 > 0x40 -> [0x157A83]=0; RET
0x7F291  [0x157A83] = rec; if word[+0x6B] > word[+0x77] RET
0x7F2A8  if (byte[0x15872D] > 0) RET
0x7F2B5  if (dword[+0x5D] != 0) RET
0x7F2BF  type arm (dword[0x157750] > 0x38): dir = 0x10F334/33C[type8], speed 0x60
0x7F304  slot arm: dir = slot[+0x20/+0x21], speed 0x30
0x7F32D  team gates (team+0x828 != 0 && team+0x7BF == 0 &&
         (team+0x829 != 0 || +0x8D != 0)) -> CALL 0x7876C (slot merge)
0x7F361  else CALL 0x7F7E0 (fallback, unported)
0x7F386  [0x5872A] = dir_x; [0x5872B] = dir_z
0x7F3A1..0x7F57B  the stage-0 target algebra (unported, OL-63)
0x7F57C  stage 1: 0x92820(rec,0x26); 0x1577BE/C0/C2 = 0;
         anim = (+0x8D != 0) ? 6 : 0x30 via 0x6E598; +0x89 = 0; +0x92++
0x7F5E9  stage 2: no slot RET; CALL 0x79B1C (snap);
         CALL 0x79C50(rec, slot[+0x1D]>>24, slot[+0x1E]>>24);
         word[slot+6] == 0 -> RET; +0x92 = 0
0x7F627  stage 3: byte[+0x44] == 0 -> RET; +0x92 = 0;
         if (rec == [rec][+0x7B2]) { 0x7D9A4([0x158730], 4, 0, 0);
         0x79B58([0x158734]) }
```

Ported: `fifa96_action_carrier_arm(&state, &carrier, type_dir_x, type_dir_z,
&out)` (`include/fifa96_loader/fifa96_action_handlers.h`) with the FU-78
`possession_claim`/`_timer`/`dribble_dir` helpers and the 0x79B1C snap,
0x79C50 face and 0x6E598 animation folds inline (the `fifa96_arm_helpers`
library already depends on `fifa96_action_handlers`, so reusing those symbols
here would create a link cycle); the unported calls are explicit request flags
(`sink`, `slot_merge`, `fallback`, `tail`, `camera_zero`) with the numbered
legs. The stage table is honoured exactly (0-3; >3 returns).

### 8.4 The staging-block vector erratum

`0x158738` holds the `FUN_0008DCD4` out triple `{word distance, word dx, word
dz}`: `0x7F666` (`0x7F6AF..0x7F6BB`) writes it with `EAX = rec+0x59`,
`EDX = &local`, `EBX = 0x158738`, and the staging tail reads dword
`0x158738`>>16 as the target-x addend (`0x7AA75`) and dword `0x15873A`>>16 as
the target-z addend (`0x7AA8C`/`0x7AA94`). FU-139 §3.3's "x/middle/z" C field
naming is therefore semantically `{distance, dx, dz}`; the struct is unchanged
(never repurpose a field) and `fifa96_ball_pair_stage_tail` documents the
mapping.

### 8.5 Tests

* `tests/test_ball_pairing.c`: `test_stage_tail_recompute_and_receive` (the
  eligibility table domain, negative/0xF wraps, the nudge gate/addends, the
  camera-zero arm), `test_stage_tail_inactive_subcode_and_clear` (0x30/0x31
  latches, the 4/5/7 clear field-by-field, the fall-through anim), and
  `test_stage_tail_face_anim_and_slot` (octants 2/0, the zero-direction seed,
  the 0x6F clamp, the slot callback) + NULLs.
* `tests/test_action_possession.c`: `test_carrier_phase_reset`,
  `test_carrier_claim_timer_and_timer81`, `test_carrier_stage0_gates` (lane
  0x40/0x41, close/bound, release countdown, airborne, type/slot/merge/fallback
  arms + table NULLs), `test_carrier_stage1` (sink/camera/anim 6 vs 0x30,
  latch), `test_carrier_stage2_and_stage3` (snap, slot face, `word[slot+6]`,
  `+0x44`, hand-off, stage > 3) + NULLs.
* `tests/test_engine_match_handlers.c`: `test_action_05_unwired_carrier` pins
  `fn == NULL`, the FU-139/OL-63/UNSUPPORTED evidence and `UNSUP` dispatch.

### 8.6 Wiring decision (evidence-gated)

Row `05` is **not wired**. The plan's wiring gate requires install arm + full
record-visible body + pool binding; the stage-0 target algebra
(`0x7F3A1..0x7F57B`) writes the record target triple and swaps a 20-byte block
via `FUN_0006DA64`, and the `FUN_0007F7E0` fallback installs a code — all
outside this slice. Per the T6/T9 reviewed precedent (conditional gate;
honest negative over an unsupported claim), `fifa96_match_action_table[0x05].fn`
stays NULL and the evidence names **OL-63**. The ported functions are
loader-level, tested symbols consumed by the future T11/T12 arms.

### 8.7 Open legs (numbered; registered in FU-142 §6)

* **OL-62 — staging-tail residual.** `FUN_0007A490 0x7AA3C..0x7AE2F`: the
  per-code jump table flat `0x7A458` (`{0x7AACB,0x7AB83,0x7AC57,0x7AD0F,
  0x7ADA0,0x7AE2F,0x7AE2F,0x7ADBA,0x7ADEC,0x7AE2F x5}`), the
  `0x78B00` slot callback (`0x7AA37`), `FUN_0007A084`'s body, the local target
  algebra with `0x92820`/`0x8F188` sinks, `0x92AC8` RNG draws and the
  `0x157736` speed source all remain unported; the nudge's second addend
  (dword `0x15873A`>>16 == `0x15873C`) is caller-supplied.
* **OL-63 — row-05 residual / non-wiring.** Stage 0's target algebra
  `0x7F3A1..0x7F57B` (the camera/local target copies, the `0x8DC68` metric
  accumulator, `FUN_00092820(rec,0x26)`, `FUN_00071C94`, the `0x15872D`
  write, the `0x157A4F` gate, `FUN_00079CCC` with `+0x9A`, `FUN_0006DA64`)
  and the `FUN_0007F7E0` fallback (`0x7F7E0..0x801B2`; installs code 7/0x11,
  rotates `0x158729`, calls `0x7E528`/`0x8DCD4`/`0x92AC8`/`0x6DBCC`/`0x741B4`/
  `0x8DD70`/`0x92820`/`0x7D9A4`) are unported; row 05 stays unwired with this
  leg.
* **OL-64 — stage-0 → stage-1 edge.** Row 05's body never writes `+0x92` on
  the stage-0 path (`0x7F274..0x7F57B` has no `+0x92` store; the only stores
  are `0x7F5C7`, `0x7F616`, `0x7F62D`), so the native 0→1 transition is an
  external re-install (the installer `FUN_0007D9A4` stages the BL byte into
  `+0x92`) whose caller is not statically located in this window; the port
  models the stage byte as an input.

### 8.8 Repo state (this task)

`make check` 103/103 (library tests also under ASan/UBSan); M1 golden and
pinned render hashes unchanged (no render path touched). Write set:
`include/fifa96_loader/fifa96_ball_pairing.h`,
`src/fifa96_loader/fifa96_ball_pairing.c`,
`include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c`,
`tests/test_ball_pairing.c`, `tests/test_action_possession.c`,
`src/fifa96_engine/fifa96_match_handlers.c`,
`tests/test_engine_match_handlers.c`, `CMakeLists.txt` (ball-pairing link adds
`fifa96_arm_helpers`), `docs/ghidra/FU139_action_cluster_b.md` (this section),
`docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 row 05, §7 note, Task-10
errata), `docs/ghidra/FU142_installer_arms_scope.md` (§6 leg registry).
`game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not staged.
